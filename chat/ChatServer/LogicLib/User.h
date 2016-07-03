#pragma once
#include <string>

namespace NLogicLib
{
	class User
	{
	public:
		enum class DOMAIN_STATE {
			NONE = 0,
			LOGIN = 1,
			LOBBY = 2,
			ROOM = 3,
		};

	public:
		User() {}
		virtual ~User() {}

		void Init(const short index)
		{
			m_Index = index;
		}

		void Clear()
		{
			//클라이언트 세션의 인덱스
			m_SessionIndex = 0;
			m_ID = "";
			m_IsConfirm = false;
			m_CurDomainState = DOMAIN_STATE::NONE;

			//몇번 로비, 룸에있는지 넣어둠
			//-1은 할당이 아직 안됨을 의미
			m_LobbyIndex = -1;
			m_RoomIndex = -1;
		}

		//클라이언트 세션, 아이디, 현재상태 세팅(초기화)
		void Set(const int sessionIndex, const char* pszID)
		{
			m_IsConfirm = true;
			m_CurDomainState = DOMAIN_STATE::LOGIN;

			m_SessionIndex = sessionIndex;
			m_ID = pszID;

		}

		short GetIndex() { return m_Index; }

		int GetSessioIndex() { return m_SessionIndex; }

		std::string& GetID() { return m_ID; }

		bool IsConfirm() { return m_IsConfirm; }

		short GetLobbyIndex() { return m_LobbyIndex; }

		//로비에 입장할때 상태 변환 적용
		void EnterLobby(const short lobbyIndex)
		{
			m_LobbyIndex = lobbyIndex;
			m_CurDomainState = DOMAIN_STATE::LOBBY;
		}

		//로비를 벗어날 때 상태 변환 적용 로그아윳??
		void LeaveLobby()
		{
			m_CurDomainState = DOMAIN_STATE::LOGIN;
		}


		short GetRoomIndex() { return m_RoomIndex; }

		//로비에서 방으로 입장할 때 상태변환 적용
		void EnterRoom(const short lobbyIndex, const short roomIndex)
		{
			m_LobbyIndex = lobbyIndex;
			m_RoomIndex = roomIndex;
			m_CurDomainState = DOMAIN_STATE::ROOM;
		}

		bool IsCurDomainInLogIn() {
			return m_CurDomainState == DOMAIN_STATE::LOGIN ? true : false;
		}

		bool IsCurDomainInLobby() {
			return m_CurDomainState == DOMAIN_STATE::LOBBY ? true : false;
		}

		bool IsCurDomainInRoom() {
			return m_CurDomainState == DOMAIN_STATE::ROOM ? true : false;
		}


	protected:
		short m_Index = -1;

		int m_SessionIndex = -1;

		std::string m_ID;

		bool m_IsConfirm = false;

		DOMAIN_STATE m_CurDomainState = DOMAIN_STATE::NONE;

		short m_LobbyIndex = -1;

		short m_RoomIndex = -1;
	};
}