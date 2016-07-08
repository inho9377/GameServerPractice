#pragma once
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <deque>
#include <memory>
#include <thread>
#include <mutex>
#include "..\..\Common\conmanip.h"
#include "..\..\Common\ErrorCode.h"
#include "..\..\Common\Packet.h"
#include "..\..\Common\PacketID.h"



//��Ŷ�� ������ MAX SIZE����
const int MAX_PACKET_SIZE = 1024;
const int MAX_SOCK_RECV_BUFFER = 8016;

#pragma pack(push, 1)
struct PacketHeader
{
	short id;
	short BodySize;
};
#pragma pack(pop)


const int PACKET_HEADER_SIZE = sizeof(PacketHeader);

//��Ŷ�� BodySize(char data��), ID
struct RecvPacketInfo
{
	RecvPacketInfo() {}

	short PacketId = 0;
	short PacketBodySize = 0;
	char* pData = nullptr;
};

class TcpNetwork
{ 
public:
	TcpNetwork(){}
	~TcpNetwork() {}

	//�ش� IP, port�� ����
	bool ConnectTo(const char* hostIP, int port)
	{
		//start ����
		WSADATA wsa;
		if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
			return false;

		//���� ����
		m_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (m_sock == INVALID_SOCKET)
			return false;

		//��Ʈ, IP�� ���� �ּ� ����ü�� ����
		SOCKADDR_IN s_addr_in;
		ZeroMemory(&s_addr_in, sizeof(s_addr_in));
		s_addr_in.sin_family = AF_INET;
		s_addr_in.sin_port = htons(port);
		
		inet_pton(AF_INET, hostIP, (void*)&s_addr_in.sin_addr.s_addr);

		//connect
		if (connect(m_sock, (SOCKADDR*)&s_addr_in, sizeof(s_addr_in)) != 0)
			return false;

		m_IsConnected = true;


		NonBlock(m_sock);

		//������ ����
		m_Thread = std::thread([&](){ Update(); });

		return true;
	}

	bool IsConnected() { return m_IsConnected; }

	void DisConnect()
	{
		if (m_IsConnected)
		{
			closesocket(m_sock);

			Clear();
		}

		if (m_Thread.joinable())
			m_Thread.join();
	}

	void SendPacket(const short packetId, const short dataSize, char* pData)
	{
		char data[MAX_PACKET_SIZE] = { 0, };

		PacketHeader pktHeader{ packetId, dataSize };

		memcpy(&data[0], (char*)&pktHeader, PACKET_HEADER_SIZE);

		if (dataSize > 0)
			memcpy(&data[PACKET_HEADER_SIZE], pData, dataSize);

		send(m_sock, data, dataSize + PACKET_HEADER_SIZE, 0);
	}

	void Update()
	{
		//����ȵ��� ����
		while (m_IsConnected)
		{
			RecvData();
			RecvBufferProcess();
		}
	}


	RecvPacketInfo GetPacket()
	{
		std::lock_guard<std::mutex> guard(m_mutex);
		if (m_PacketQueue.empty())
			return RecvPacketInfo();

		auto packet = m_PacketQueue.front();
		m_PacketQueue.pop_front();
		return packet;
	}





private:

	void NonBlock(SOCKET s)
	{
		u_long u10n = 1L;

		//�ͺ��ŷ�������� ����
		ioctlsocket(s, FIONBIO, (unsigned long*)&u10n);
	}

	//�����͸� �޴´�.
	void RecvData()
	{
		fd_set read_set;
		timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 100;

		FD_ZERO(&read_set);
		FD_SET(m_sock, &read_set);
		if (select(m_sock + 1, &read_set, NULL, NULL, &tv) < 0) {
			return;
		}

		if (FD_ISSET(m_sock, &read_set))
		{
			char recvBuff[MAX_PACKET_SIZE];

			//Ŀ�� API�� �� ����ؾ� �� (recv�� Ŀ�� API)
			auto recvsize = recv(m_sock, recvBuff, MAX_PACKET_SIZE, 0);

			if (recvsize == 0)
				return;

			if (recvsize < 0)
				if (WSAGetLastError() != WSAEWOULDBLOCK)
					return;

			if ((m_recvSize + recvsize) >= MAX_SOCK_RECV_BUFFER)
				return;

			memcpy(&m_RecvBuffer[m_recvSize], recvBuff, recvsize);
			m_recvSize += recvsize;
		}
	}

	void RecvBufferProcess()
	{
		auto readPos = 0;
		const auto dataSize = m_recvSize;
		PacketHeader* pPktHeader;

		while ((dataSize - readPos) > PACKET_HEADER_SIZE)
		{
			pPktHeader = (PacketHeader*)&m_RecvBuffer[readPos];
			readPos += PACKET_HEADER_SIZE;

			if (pPktHeader->BodySize > (dataSize - readPos))
			{
				readPos -= PACKET_HEADER_SIZE;
				break;
			}

			if (pPktHeader->BodySize > MAX_PACKET_SIZE)
			{
				readPos -= PACKET_HEADER_SIZE;
				return;// NET_ERROR_CODE::RECV_CLIENT_MAX_PACKET;
			}

			AddPacketQueue(pPktHeader->id, pPktHeader->BodySize, &m_RecvBuffer[readPos]);

			readPos += pPktHeader->BodySize;
		}

		m_recvSize -= readPos;

		if (m_recvSize > 0)
		{
			memcpy(m_RecvBuffer, &m_RecvBuffer[readPos], m_recvSize);
		}
	}

	/*void RecvBufferProcess()
	{
		auto readPos = 0;
		const auto dataSize = m_recvSize;
		PacketHeader* pPktHeader;

		while ((dataSize - readPos) > PACKET_HEADER_SIZE)
		{
			pPktHeader = (PacketHeader*)&m_RecvBuffer[readPos];
			readPos += PACKET_HEADER_SIZE;

			if (pPktHeader->BodySize > (dataSize - readPos))
			{
				//BUG
				readPos -= PACKET_HEADER_SIZE;
				break;
			}
				

			if (pPktHeader->BodySize > MAX_PACKET_SIZE)
				return;

			AddPacketQueue(pPktHeader->id, pPktHeader->BodySize, &m_RecvBuffer[readPos]);
			//BUG
			readPos += pPktHeader->BodySize;
		}

		m_recvSize -= readPos;

		if (m_recvSize > 0)
			memcpy(m_RecvBuffer, &m_RecvBuffer[readPos], m_recvSize);



	}
	*/


	void AddPacketQueue(const short pktId, const short bodySize, char* pDataPos)
	{
		RecvPacketInfo packetInfo;
		packetInfo.PacketId = pktId;
		packetInfo.PacketBodySize = bodySize;
		packetInfo.pData = new char[bodySize];
		memcpy(packetInfo.pData, pDataPos, bodySize);

		std::lock_guard<std::mutex> guard(m_mutex);
		m_PacketQueue.push_back(packetInfo);
	}

	void Clear()
	{
		m_IsConnected = false;
		m_recvSize = 0;
		m_PacketQueue.clear();
	}




	bool m_IsConnected = false;

	std::thread m_Thread;
	std::mutex m_mutex;

	SOCKET m_sock;

	int m_recvSize = 0;
	char m_RecvBuffer[MAX_SOCK_RECV_BUFFER] = { 0, };

	std::deque<RecvPacketInfo> m_PacketQueue;
};