#pragma once
#include <vector>
#include <unordered_map>
//??
#include "../ServerNetLib/ITcpNetwork.h"

namespace NServerNetLib
{
	class TcpNetwork;
}

namespace NServerNetLib
{
	class ILog;
}

namespace NLogicLib
{
	//로비매니저의 정보 로비의 수, 유저 최대수, 룸 최대수, 룸이 가질수있는 유저의 최대수를 지정
	struct LobbyManagerConfig
	{
		int MaxLobbyCount;
		int MaxLobbyUserCount;
		int MaxRoomCountByLobby;
		int MaxRoomUserCount;
	};


	//로비의 정보 축약??
	struct LobbySmallInfo
	{
		short Num;
		short UserCount;
	};

	class Lobby;

	class LobbyManager
	{
		using TcpNet = NServerNetLib::ITcpNetwork;
		using ILog = NServerNetLib::ILog;

	public:
		LobbyManager();
		virtual ~LobbyManager();

		void Init(const LobbyManagerConfig config, TcpNet* pNetwork, ILog* pLogger);

		Lobby* GetLobby(short lobbyId);


	public:
		void SendLobbyListInfo(const int sessionIndex);





	private:
		ILog* m_pRefLogger;
		TcpNet* m_pRefNetwork;

		//로비들의 리스트를 갖고있다.
		std::vector<Lobby> m_LobbyList;

	};
}