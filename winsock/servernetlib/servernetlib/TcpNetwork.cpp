#include <stdio.h>
#include <vector>
#include <deque>

#include "TcpNetwork.h"
#include "ILog.h"


namespace NServerNetLib
{
	TcpNetwork::TcpNetwork(){}
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
		memcpy(&m_Config, pConfig, sizeof(ServerConfig));

		m_pRefLogger = pLogger;

		auto initRet = InitServerSocket();
		if (initRet != NET_ERROR_CODE::NONE)
			return initRet;

		auto bindListenRet = BindListen(pConfig->Port, pConfig->BackLogCount);
		if (bindListenRet != NET_ERROR_CODE::NONE)
			return bindListenRet;

		FD_ZERO(&m_Readfds);
		FD_SET(m_ServerSockfd, &m_Readfds);

		CreateSessionPool(pConfig->MaxClientCount + pConfig->ExtraClientCount);

		return NET_ERROR_CODE::NONE;
	}


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

	void TcpNetwork::Run()
	{
		auto read_set = m_Readfds;
		auto write_set = m_Readfds;
		auto exc_set = m_Readfds;

		timeval timeout{ 0,1000 };
		auto selectResult = select(0, &read_set, &write_set, &exc_set, &timeout);

		auto isFDSetChanged = RunCheckSelectResult(selectResult);
		if (isFDSetChanged == false)
			return;

		if (FD_ISSET(m_ServerSockfd, &read_set))
		{
			NewSession();
		}

		else
		{
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
			return false;
		}

		return true;
	}

	void TcpNetwork::RunCheckSelectClients(fd_set& exc_set, fd_set& read_set, fd_set& write_set)
	{
		for (int i = 0; i < m_ClientSessionPool.size(); ++i)
		{
			auto& session = m_ClientSessionPool[i];

			if (session.IsConnected() == false)
				continue;

			SOCKET fd = session.SocketFD;
			auto sessionIndex = session.Index;

			if (FD_ISSET(fd, &exc_set))
			{
				CloseSession(SOCKET_CLOSE_CASE::SELECT_ERROR, fd, sessionIndex);
				continue;
			}

			auto retReceive = RunProcessReceive(sessionIndex, fd, read_set);
			if (retReceive == false)
				continue;

			RunProcessWrite(sessionIndex, fd, write_set);
		}
	}


	bool TcpNetwork::RunProcessReceive(const int sessionIndex, const SOCKET fd, fd_set& read_set)
	{
		if (!FD_ISSET(fd, &read_set))
			return true;
	}
}



