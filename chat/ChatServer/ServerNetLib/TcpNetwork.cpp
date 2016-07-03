#include <stdio.h>
#include <vector>
#include <deque>

#include "ILog.h"
#include "TcpNetwork.h"


namespace NServerNetLib
{
	TcpNetwork::TcpNetwork() {}

	TcpNetwork::~TcpNetwork()
	{
		for (auto& client : m_ClientSessionPool)
		{
			if (client.pRecvBuffer) {
				delete[] client.pRecvBuffer;
			}

			if (client.pSendBuffer) {
				delete[] client.pSendBuffer;
			}
		}
	}


	NET_ERROR_CODE TcpNetwork::Init(const ServerConfig* pConfig, ILog* pLogger)
	{
		//serverconfig의 메모리를 pConfig(멤버변수)에 카피
		memcpy(&m_Config, pConfig, sizeof(ServerConfig));

		//로그 클래스를 멤버 변수에 카피
		m_pRefLogger = pLogger;

		//서버 소켓 초기화
		auto initRet = InitServerSocket();
		if (initRet != NET_ERROR_CODE::NONE)
		{
			return initRet;
		}

		//소켓 bind&Listen
		auto bindListenRet = BindListen(pConfig->Port, pConfig->BackLogCount);
		if (bindListenRet != NET_ERROR_CODE::NONE)
		{
			return bindListenRet;
		}


		//Socket Set들 초기화
		FD_ZERO(&m_Readfds);
		FD_SET(m_ServerSockfd, &m_Readfds);

		//ClientSessionPool 초기화
		CreateSessionPool(pConfig->MaxClientCount + pConfig->ExtraClientCount);

		return NET_ERROR_CODE::NONE;

	}

	//패킷 큐의 가장 앞쪽 패킷을 가져옴
	RecvPacketInfo TcpNetwork::GetPacketInfo()
	{
		RecvPacketInfo packetInfo;

		if (m_PacketQueue.empty() == false)
		{
			packetInfo = m_PacketQueue.front();
			m_PacketQueue.pop_front();
		}

		return packetInfo;
	}

	//동작 함수
	void TcpNetwork::Run()
	{
		//멤버 변수 FDS를 각 용도에 맞게 세팅..
		//왜 같은 멤버변수(set)로 하는거지?
		auto read_set = m_Readfds;
		auto write_set = m_Readfds;
		auto exc_set = m_Readfds;


		timeval timeout{ 0, 1000 }; //tv_sec, tv_usec
									//Select함수 실행해 현재 상태 확인
		auto selectResult = select(0, &read_set, &write_set, &exc_set, &timeout);

		//select의 결과를 확인한다
		auto isFDSetChanged = RunCheckSelectResult(selectResult);
		if (isFDSetChanged == false)
		{
			return;
		}

		//서버에 전달된 디스크립터가 있다면 새로운 세션을 만든다.
		if (FD_ISSET(m_ServerSockfd, &read_set))
		{
			NewSession();
		}
		else // clients
		{
			//없다면 클라이언트 세션 전부를 확인?
			RunCheckSelectClients(exc_set, read_set, write_set);
		}



	}


	bool TcpNetwork::RunCheckSelectResult(const int result)
	{
		if (result == 0)
		{
			return false;
		}
		else if (result == -1)
		{
			// 로그 남기면 좋다
			return false;
		}

		return true;
	}



	//select Client ?
	void TcpNetwork::RunCheckSelectClients(fd_set& exc_set, fd_set& read_set, fd_set& write_set)
	{
		for (int i = 0; i < m_ClientSessionPool.size(); ++i)
		{
			auto& session = m_ClientSessionPool[i];

			//세션이 연결되어있지 않다면 다음 세션으로 진행 (continue)
			if (session.IsConnected() == false)
				continue;


			//선택한 세션의 인덱스와 소켓을 가져옴
			SOCKET fd = session.SocketFD;
			auto sessionIndex = session.Index;


			//에러가 있는지 체크한다. (에러가 있으면  Exc_SET에 세팅됨?)
			if (FD_ISSET(fd, &exc_set))
			{
				//에러가 있으면 정보를 보내고 해당 세션을 종료
				CloseSession(SOCKET_CLOSE_CASE::SELECT_ERROR, fd, sessionIndex);
				continue;
			}

			//Read인지 체크한다. ( receive)
			auto retReceive = RunProcessReceive(sessionIndex, fd, read_set);
			if (retReceive == false) {
				continue;
			}

			//write인지 체크한다.
			RunProcessWrite(sessionIndex, fd, write_set);



		}
	}


	bool TcpNetwork::RunProcessReceive(const int sessionIndex, const SOCKET fd, fd_set& read_set)
	{
		//디스크립터 정보 없으면 true 반환? 반대아님? 이미 열려있으면?
		if (!FD_ISSET(fd, &read_set))
		{
			return true;
		}

		//해당 세션에 해당하는 소켓을 Receive
		auto ret = RecvSocket(sessionIndex);
		if (ret != NET_ERROR_CODE::NONE)
		{
			//에러있으면 세션 종료
			CloseSession(SOCKET_CLOSE_CASE::SOCKET_RECV_ERROR, fd, sessionIndex);
			return false;
		}

		//해당 세션에 해당하는 버퍼를 Recv
		ret = RecvBufferProcess(sessionIndex);
		if (ret != NET_ERROR_CODE::NONE)
		{
			//에러 있으면 세션 종료
			CloseSession(SOCKET_CLOSE_CASE::SOCKET_RECV_BUFFER_PROCESS_ERROR, fd, sessionIndex);
			return false;
		}




		//이상무
		return true;
	}

	//특정 세션(인덱스)에 해당하는 패킷을 보냄
	NET_ERROR_CODE TcpNetwork::SendData(const int sessionIndex, const short packetId, const short size, const char* pMsg)
	{
		auto& session = m_ClientSessionPool[sessionIndex];
		auto pos = session.SendSize;

		//버퍼의 크기가 맥시멈을 넘어서면 에러 반환
		if ((pos + size + PACKET_HEADER_SIZE) > m_Config.MaxClientSendBufferSize)
			return NET_ERROR_CODE::CLIENT_SEND_BUFFER_FULL;


		//주어진 정보로 패킷 헤더 구성
		PacketHeader pktHeader{ packetId, size };


		//해당 세션의 SendBuffer에 패킷의 헤더와 정보를 입력
		memcpy(&session.pSendBuffer[pos], (char*)&pktHeader, PACKET_HEADER_SIZE);
		memcpy(&session.pSendBuffer[pos + PACKET_HEADER_SIZE], pMsg, size);
		//사이즈도 패킷헤더를 더해서 갱신
		session.SendSize += (size + PACKET_HEADER_SIZE);

		return NET_ERROR_CODE::NONE;

	}

	//세션 풀을 만든다. 초기화 작업
	void TcpNetwork::CreateSessionPool(const int maxClientCount)
	{
		for (int i = 0; i < maxClientCount; ++i)
		{
			//세션이 가지는 정보 : 인덱스(구분), Receive Buffer, SendBuffer 
			ClientSession session;
			ZeroMemory(&session, sizeof(session));
			session.Index = i;
			session.pRecvBuffer = new char[m_Config.MaxClientRecvBufferSize];
			session.pSendBuffer = new char[m_Config.MaxClientSendBufferSize];

			//클라이언트 세션의 포인터가 아닌 이유가 있나?
			m_ClientSessionPool.push_back(session);
			m_ClientSessionPoolIndex.push_back(session.Index);
		}
	}

	//클라이언트 세션에 인덱스를 할당한다. 초기화 작업??
	int TcpNetwork::AllocClientSessionIndex()
	{
		//클라이언트 세션이 생성 된 뒤에 작업되어야 함. 비어있으면 에러 반환
		if (m_ClientSessionPoolIndex.empty())
			return -1;

		//??
		int index = m_ClientSessionPoolIndex.front();
		m_ClientSessionPoolIndex.pop_front();
		return index;
	}

	//해당 인덱스의 세션풀인덱스와 세션을 해제
	void TcpNetwork::ReleaseSessionIndex(const int index)
	{
		m_ClientSessionPoolIndex.push_back(index);
		m_ClientSessionPool[index].Clear();
	}

	//서버 소켓을 초기화한다.
	NET_ERROR_CODE TcpNetwork::InitServerSocket()
	{
		//2.2 version
		WORD wVersionRequested = MAKEWORD(2, 2);
		WSADATA wsaData;
		WSAStartup(wVersionRequested, &wsaData);

		//서버 소켓 생성
		m_ServerSockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_ServerSockfd < 0)
		{
			return NET_ERROR_CODE::SERVER_SOCKET_CREATE_FAIL;
		}

		auto n = 1;
		//버퍼 크기등 소켓 정보 설정 (버퍼 크기 늘리는 경우도 이 함수 사용)
		if (setsockopt(m_ServerSockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof(n)) < 0)
		{
			return NET_ERROR_CODE::SERVER_SOCKET_SO_REUSEADDR_FAIL;
		}

		//이상무
		return NET_ERROR_CODE::NONE;
	}

	//해당 포트로 bind & Listen상태로
	NET_ERROR_CODE TcpNetwork::BindListen(short port, int backlogCount)
	{
		//UNION(공용체). 알아서 형변환 IPv4 주소 구조체
		SOCKADDR_IN server_addr;
		ZeroMemory(&server_addr, sizeof(server_addr));
		server_addr.sin_family = AF_INET; //주소체계
		server_addr.sin_addr.s_addr = htonl(INADDR_ANY); //32비트 IP정보 (네트워크 바이트 오더)
		server_addr.sin_port = htons(port); //16비트 포트 정보 (네트워크 바이트 오더)

											//바인드
		if (bind(m_ServerSockfd, (SOCKADDR*)&server_addr, sizeof(server_addr)) < 0)
		{
			return NET_ERROR_CODE::SERVER_SOCKET_BIND_FAIL;
		}

		//리슨
		if (listen(m_ServerSockfd, backlogCount) == SOCKET_ERROR)
		{
			return NET_ERROR_CODE::SERVER_SOCKET_LISTEN_FAIL;
		}

		//리슨 성공 정보 로그로 남김
		m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | Listen. ServerSockfd(%d)", __FUNCTION__, m_ServerSockfd);

		//이상무
		return NET_ERROR_CODE::NONE;
	}

	NET_ERROR_CODE TcpNetwork::NewSession()
	{
		SOCKADDR_IN client_addr;
		auto client_len = static_cast<int>(sizeof(client_addr));
		auto client_sockfd = accept(m_ServerSockfd, (SOCKADDR*)&client_addr, &client_len);

		//클라이언트 소켓 FD가 없으면
		if (client_sockfd < 0)
		{
			//로그 남기고 리턴
			m_pRefLogger->Write(LOG_TYPE::L_ERROR, "%s | Wrong socket %d cannot accept", __FUNCTION__, client_sockfd);
			return NET_ERROR_CODE::ACCEPT_API_ERROR;
		}

		//클라이언트 세션 인덱스가 없으면
		auto newSessionIndex = AllocClientSessionIndex();
		if (newSessionIndex < 0)
		{
			//로그 남기고 수용 불가하므로 세션 종료
			m_pRefLogger->Write(LOG_TYPE::L_WARN, "%s | client_sockfd(%d)  >= MAX_SESSION", __FUNCTION__, client_sockfd);
			CloseSession(SOCKET_CLOSE_CASE::SESSION_POOL_EMPTY, client_sockfd, -1);
			return NET_ERROR_CODE::ACCEPT_MAX_SESSION_COUNT;
		}

		//여기에 정보중 하나로 현재 시간을 박아넣음

		char clientIP[MAX_IP_LEN] = { 0, };
		inet_ntop(AF_INET, &(client_addr.sin_addr), clientIP, MAX_IP_LEN - 1);


		SetSockOption(client_sockfd);

		FD_SET(client_sockfd, &m_Readfds);

		ConnectedSession(newSessionIndex, (int)client_sockfd, clientIP);

		return NET_ERROR_CODE::NONE;

		//루프돌면서 isconnected가 true인 경우 시간 확인하고 날림


	}

	//해당 세션에 접속
	void TcpNetwork::ConnectedSession(const int sessionIndex, const int fd, const char* pIP)
	{
		++m_ConnectSeq;

		auto& session = m_ClientSessionPool[sessionIndex];
		session.Seq = m_ConnectSeq;
		session.SocketFD = fd;
		memcpy(session.IP, pIP, MAX_IP_LEN - 1);

		++m_ConnectedSessionCount;

		//로그 찍음
		m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | New Session. FD(%d), m_ConnectSeq(%d), IP(%s)", __FUNCTION__, fd, m_ConnectSeq, pIP);
	}

	//해당 소켓에 사이즈 등 미리 정해둔 각종 옵션 세팅
	void TcpNetwork::SetSockOption(const SOCKET fd)
	{
		linger ling;
		ling.l_onoff = 0;
		ling.l_linger = 0;
		setsockopt(fd, SOL_SOCKET, SO_LINGER, (char*)&ling, sizeof(ling));

		int size1 = m_Config.MaxClientSockOptRecvBufferSize;
		int size2 = m_Config.MaxClientSockOptSendBufferSize;
		setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*)&size1, sizeof(size1));
		setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&size2, sizeof(size2));
	}

	//해당 세션을 종료한다
	void TcpNetwork::CloseSession(const SOCKET_CLOSE_CASE closeCase, const SOCKET sockFD, const int sessionIndex)
	{
		//세션이 닫히는 케이스가 풀이 비어있으면 소켓 닫고 셋 최소화함
		if (closeCase == SOCKET_CLOSE_CASE::SESSION_POOL_EMPTY)
		{
			closesocket(sockFD);
			FD_CLR(sockFD, &m_Readfds);
			return;
		}

		//클라이언트 세션이 연결되어있지 않으면 리턴
		if (m_ClientSessionPool[sessionIndex].IsConnected() == false)
			return;

		//소켓을 닫고 디스크립터도 삭제
		closesocket(sockFD);
		FD_CLR(sockFD, &m_Readfds);

		//클라이언트 세션풀에서도 삭제
		m_ClientSessionPool[sessionIndex].Clear();
		--m_ConnectedSessionCount;
		ReleaseSessionIndex(sessionIndex);


		//패킷큐에 추가??
		AddPacketQueue(sessionIndex, (short)PACKET_ID::NTF_SYS_CLOSE_SESSION, 0, nullptr);
	}

	//해당 세션의 소켓을 Receive
	NET_ERROR_CODE TcpNetwork::RecvSocket(const int sessionIndex)
	{
		auto& session = m_ClientSessionPool[sessionIndex];
		auto fd = static_cast<SOCKET>(session.SocketFD);

		//세션이 연결되어있지 않으면 에러
		if (session.IsConnected() == false)
		{
			return NET_ERROR_CODE::RECV_PROCESS_NOT_CONNECTED;
		}



		int recvPos = 0;

		//세션에 남아있는 데이터를 원래대로
		if (session.RemainingDataSize > 0)
		{
			recvPos += session.RemainingDataSize;
			session.RemainingDataSize = 0;
		}
		auto recvSize = recv(fd, &session.pRecvBuffer[recvPos], (MAX_PACKET_SIZE * 2), 0);


		//에러 체크
		if (recvSize == 0)
		{
			return NET_ERROR_CODE::RECV_REMOTE_CLOSE;
		}
		if (recvSize < 0)
		{
			auto error = WSAGetLastError();
			if (error != WSAEWOULDBLOCK)
			{
				return NET_ERROR_CODE::RECV_API_ERROR;
			}
			else
			{
				return NET_ERROR_CODE::NONE;
			}
		}

		session.RemainingDataSize += recvSize;
		return NET_ERROR_CODE::NONE;
	}

	//Receive Buffer Process
	NET_ERROR_CODE TcpNetwork::RecvBufferProcess(const int sessionIndex)
	{
		//세션 지정
		auto& session = m_ClientSessionPool[sessionIndex];

		auto readPos = 0;
		const auto dataSize = session.RemainingDataSize;
		PacketHeader* pPktHeader;

		//패킷 헤더 사이즈보다 커질때까지 루프
		while ((dataSize - readPos) >= PACKET_HEADER_SIZE)
		{
			pPktHeader = (PacketHeader*)&session.pRecvBuffer[readPos];
			readPos += PACKET_HEADER_SIZE;

			if (pPktHeader->BodySize > 0)
			{
				if (pPktHeader->BodySize < (dataSize - readPos))
				{
					break;
				}

				if (pPktHeader->BodySize > MAX_PACKET_SIZE)
				{
					return NET_ERROR_CODE::RECV_CLIENT_MAX_PACKET;
				}
			}

			AddPacketQueue(sessionIndex, pPktHeader->Id, pPktHeader->BodySize, &session.pRecvBuffer[readPos]);

			readPos += pPktHeader->BodySize;
		}

		session.RemainingDataSize -= readPos;

		if (session.RemainingDataSize > 0)
		{
			memcpy(session.pRecvBuffer, &session.pRecvBuffer[readPos], session.RemainingDataSize);
		}

		return NET_ERROR_CODE::NONE;
	}

	//패킷큐에 패킷(세션)을 추가
	void TcpNetwork::AddPacketQueue(const int sessionIndex, const short pktId, const short bodySize, char* pDataPos)
	{
		RecvPacketInfo packetInfo;
		packetInfo.SessionIndex = sessionIndex;
		packetInfo.PacketId = pktId;
		packetInfo.PacketBodySize = bodySize;
		packetInfo.pRefData = pDataPos;

		m_PacketQueue.push_back(packetInfo);
	}

	//Write process
	void TcpNetwork::RunProcessWrite(const int sessionIndex, const SOCKET fd, fd_set& write_set)
	{
		//Write Set이 SET되지 않았으면 리턴
		if (!FD_ISSET(fd, &write_set))
		{
			return;
		}

		auto retsend = FlushSendBuff(sessionIndex);
		if (retsend.Error != NET_ERROR_CODE::NONE)
		{
			CloseSession(SOCKET_CLOSE_CASE::SOCKET_SEND_ERROR, fd, sessionIndex);
		}
	}

	NetError TcpNetwork::FlushSendBuff(const int sessionIndex)
	{
		auto& session = m_ClientSessionPool[sessionIndex];
		auto fd = static_cast<SOCKET>(session.SocketFD);

		if (session.IsConnected() == false)
		{
			return NetError(NET_ERROR_CODE::CLIENT_FLUSH_SEND_BUFF_REMOTE_CLOSE);
		}

		auto result = SendSocket(fd, session.pSendBuffer, session.SendSize);

		if (result.Error != NET_ERROR_CODE::NONE) {
			return result;
		}

		auto sendSize = result.Vlaue;
		if (sendSize < session.SendSize)
		{
			memmove(&session.pSendBuffer[0],
				&session.pSendBuffer[sendSize],
				session.SendSize - sendSize);

			session.SendSize -= sendSize;
		}
		else
		{
			session.SendSize = 0;
		}
		return result;
	}


	NetError TcpNetwork::SendSocket(const SOCKET fd, const char* pMsg, const int size)
	{
		NetError result(NET_ERROR_CODE::NONE);
		auto rfds = m_Readfds;

		// 접속 되어 있는지 또는 보낼 데이터가 있는지
		if (size <= 0)
		{
			return result;
		}

		// 이것은 안해도 될 듯
		/*if (!FD_ISSET(fd, &rfds))
		{
		_snwprintf_s(result.Msg, _countof(result.Msg), _TRUNCATE, L"Send Msg! User %d", (int)fd);
		result.Error = NET_ERROR_CODE::SEND_CLOSE_SOCKET;
		return result;
		}*/

		result.Vlaue = send(fd, pMsg, size, 0);

		if (result.Vlaue <= 0)
		{
			result.Error = NET_ERROR_CODE::SEND_SIZE_ZERO;
		}

		return result;
	}
}