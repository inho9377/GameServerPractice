#pragma once

#include <iostream>
#include <chrono>
#include "IClientSceen.h"

class ClientSceenRoom : public IClientSceen
{
#pragma once


public:
	ClientSceenRoom() {}
	virtual ~ClientSceenRoom() {}


	virtual void Update() override
	{
		if (GetCurSceenType() != CLIENT_SCEEN_TYPE::LOGIN) {
			return;
		}

		auto curTime = std::chrono::system_clock::now();

		auto diffTimeSec = std::chrono::duration_cast<std::chrono::seconds>(curTime - m_TimeLastedReqLobbyList);

		if (diffTimeSec.count() > 3)
		{
			m_TimeLastedReqLobbyList = curTime;

			//RequestLobbyList();
		}
	}


	
	bool ProcessPacket(const short packetId, char* pData) override
	{
		switch (packetId)
		{
		case (short)PACKET_ID::LOBBY_ENTER_ROOM_LIST_RES:
		{
			auto pktRes = (NCommon::PktLobbyRoomListRes*)pData;

			if (pktRes->ErrorCode == (short)NCommon::ERROR_CODE::NONE)
			{
				//m_RoomList->clear();

				for (int i = 0; i < pktRes->Count; ++i)
				{
					auto& pRoom = pktRes->RoomInfo[i];

					m_RoomList->at(0).append({ std::to_string(pRoom.RoomIndex),
										
										std::to_string(pRoom.RoomUserCount) });
					
					/*m_RoomList->at(0).append({ std::to_string(),
						std::to_string(pLobby.LobbyUserCount),
						std::to_string(50) });*/
				}
			}
			else
			{
				std::cout << "[ROOM_LIST_RES] ErrorCode: " << pktRes->ErrorCode << std::endl;
			}
		}
		break;
		default:
			return false;
		}

		return true;
	}


	void UIShow()
	{
		//m_btnEnterRoom->show();
		//m_RoomList->show();
	}

	void UIBlind()
	{
		//m_btnEnterRoom->hide();
		//m_RoomList->hide();
	}


private:

	void RequestRoomList()
	{
		m_pRefNetwork->SendPacket((short)PACKET_ID::LOBBY_ENTER_ROOM_LIST_REQ, 0, nullptr);
	}


private:

	form* m_pForm = nullptr;

	std::unique_ptr<button> m_btnEnterRoom;
	std::unique_ptr<listbox> m_RoomList;

	std::chrono::system_clock::time_point m_TimeLastedReqLobbyList = std::chrono::system_clock::now();
};