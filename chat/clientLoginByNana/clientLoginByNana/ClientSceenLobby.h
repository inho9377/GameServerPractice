#pragma once

#include <list>
#include <string>
#include <vector>
#include <algorithm>

#include "IClientSceen.h"



class ClientSceenLobby : public IClientSceen
{
public:
	ClientSceenLobby() {}
	virtual ~ClientSceenLobby() {}

	virtual void Update() override 
	{
	}

	bool ProcessPacket(const short packetId, char* pData) override 
	{ 
		switch (packetId)
		{
		case (short)PACKET_ID::LOBBY_ENTER_RES:
		{
			auto pktRes = (NCommon::PktLobbyEnterRes*)pData;

			if (pktRes->ErrorCode == (short)NCommon::ERROR_CODE::NONE)
			{
				Init(pktRes->MaxUserCount);

				RequestRoomList(0);
				RequestUserList(0);
			}
			else
			{
				std::cout << "[LOBBY_ENTER_RES] ErrorCode: " << pktRes->ErrorCode << std::endl;
			}
		}
			break;
		case (short)PACKET_ID::LOBBY_ENTER_ROOM_LIST_RES:
		{
			auto pktRes = (NCommon::PktLobbyRoomListRes*)pData;

			if (pktRes->IsEnd == false)
			{
				for (int i = 0; i < pktRes->Count; ++i)
				{
					UpdateRoomInfo(&pktRes->RoomInfo[i]);
				}

				RequestRoomList(pktRes->RoomInfo[pktRes->Count - 1].RoomIndex + 1);
			}
			else
			{
				SetRoomListGui();
			}
		}
			break;
		case (short)PACKET_ID::LOBBY_ENTER_USER_LIST_RES:
		{
			auto pktRes = (NCommon::PktLobbyUserListRes*)pData;

			if (pktRes->IsEnd == false)
			{
				for (int i = 0; i < pktRes->Count; ++i)
				{
					UpdateUserInfo(false, pktRes->UserInfo[i].UserID);
				}
				
				RequestUserList(pktRes->UserInfo[pktRes->Count - 1].LobbyUserIndex + 1);
			}
			else
			{
				SetUserListGui();
			}
		}
			break;
		case (short)PACKET_ID::ROOM_CHANGED_INFO_NTF:
		{
			auto pktRes = (NCommon::PktChangedRoomInfoNtf*)pData;
			UpdateRoomInfo(&pktRes->RoomInfo);
		}
			break;
		case (short)PACKET_ID::LOBBY_ENTER_USER_NTF:
		{
			auto pktRes = (NCommon::PktLobbyNewUserInfoNtf*)pData;
			UpdateUserInfo(false, pktRes->UserID);
		}
			break;
		case (short)PACKET_ID::LOBBY_LEAVE_USER_NTF:
		{
			auto pktRes = (NCommon::PktLobbyLeaveUserInfoNtf*)pData;
			UpdateUserInfo(true, pktRes->UserID);
		}
			break;

		case (short)PACKET_ID::LOBBY_CHAT_RES:
		{
			auto pktRes = (NCommon::PktLobbyChatRes*)pData;
			if (pktRes->ErrorCode == (short)NCommon::ERROR_CODE::NONE)
			{

			}
			else
			{
				std::cout << "[LOBBY_CHAT_RES] ErrorCode: " << pktRes->ErrorCode << std::endl;
			}

			break;
		}

		case (short)PACKET_ID::LOBBY_CHAT_NTF:
		{
			auto pktRes = (NCommon::PktLobbyChatNtf*)pData;
			//std::string resultMsg = pktRes->UserID + " : " + Msg;

			break;
		}
		case (short)PACKET_ID::LOBBY_SECRET_CHAT_RES:
		{
			auto pktRes = (NCommon::PktLobbySecretChatRes*)pData;
			if (pktRes->ErrorCode == (short)NCommon::ERROR_CODE::NONE)
			{

			}
			else
			{
				std::cout << "[LOBBY_SECRET_CHAT_RES] ErrorCode: " << pktRes->ErrorCode << std::endl;
			}
			break;
		}

		case (short)PACKET_ID::LOBBY_SECRET_CHAT_NTF:
		{
			auto pktRes = (NCommon::PktLobbySecretChatNtf*)pData;

			break;
		}
		default:
			return false;
		}

		return true;
	}

	void CreateUI(form* pform)
	{
		m_pForm = pform;

		m_LobbyRoomList = std::make_shared<listbox>((form&)*m_pForm, nana::rectangle(10, 10, 800, 300));
		m_LobbyRoomList->append_header(L"RoomId", 50);
		m_LobbyRoomList->append_header(L"Title", 600);
		m_LobbyRoomList->append_header(L"Cur", 30);
		m_LobbyRoomList->append_header(L"Max", 30);

		m_RoomTitleTxt = std::make_shared<textbox>((form&)*m_pForm, nana::rectangle(10, 320, 180, 20));
		m_RoomTitleTxt->caption("The Room");

		m_RoomCreateBtn = std::make_shared<button>((form&)*m_pForm, nana::rectangle(210, 320, 102, 23));
		m_RoomCreateBtn->caption("RoomCreate");
		m_RoomCreateBtn->events().click([&]() {
			this->CreateRoom();
			//this->LogInOut();
		});
		m_RoomCreateBtn->enabled(true);
		
		m_LobbyUserList = std::make_shared<listbox>((form&)*m_pForm, nana::rectangle(10, 360, 120, 383));
		m_LobbyUserList->append_header("UserID", 90);


		m_chatBox = std::make_unique<listbox>((form&)*m_pForm, nana::rectangle(140, 360, 650, 260));
		m_chatBox->append_header("UserID", 100);
		m_chatBox->append_header("Text", 530);

		m_sendMessage = std::make_unique<textbox>((form&)*m_pForm, nana::rectangle(140, 640, 650, 23));
		m_sendMessage->caption("Send Message..");

		m_chatBtn = std::make_unique<button>((form&)*m_pForm, nana::rectangle(300, 670, 52, 23));
		m_chatBtn->caption("chat");
		m_chatBtn->events().click([&]() {
			this->RequestSendMessage();
		});


		m_toUserID = std::make_unique<textbox>((form&)*m_pForm, nana::rectangle(140, 670, 150, 23));
		m_toUserID->caption(defaultToUserID);

		UIBlind();
	}

	void UIBlind()
	{
		m_LobbyRoomList->hide();
		m_LobbyUserList->hide();
		m_RoomTitleTxt->hide();
		m_RoomCreateBtn->hide();
		m_chatBox->hide();
		m_chatBtn->hide();
		m_sendMessage->hide();
		m_toUserID->hide();
	}

	void UIShow()
	{
		int aabc = m_chatBox->caption_wstring().compare(L"\0");
		int babab = m_toUserID->caption_wstring().compare(L"\0");
		int c = m_RoomTitleTxt->caption_wstring().compare(L"\0");

		if (m_RoomTitleTxt->caption_wstring().compare(L"\0") == 0)
			m_RoomCreateBtn->enabled(false);
		else
			m_RoomCreateBtn->enabled(true);


		if ((m_sendMessage->caption_wstring().compare(L"\0") == 0) || (m_toUserID->caption_wstring().compare(L"\0") == 0))
			m_chatBtn->enabled(false);
		else
			m_chatBtn->enabled(true);



		m_LobbyRoomList->show();
		m_LobbyUserList->show();
		m_RoomTitleTxt->show();
		m_RoomCreateBtn->show();
		m_chatBox->show();
		m_chatBtn->show();
		m_sendMessage->show();
		m_toUserID->show();
	}

	void CreateRoom()
	{
		//NCommon::RoomSmallInfo newRoom;
		//newRoom.RoomIndex = 0;
		//wcscpy_s(newRoom.RoomTitle, sizeof(m_RoomTitleTxt->caption_wstring().c_str()), m_RoomTitleTxt->caption_wstring().c_str());
		//wcscpy(newRoom.RoomTitle, m_RoomTitleTxt->caption_wstring().c_str());
		
		//newRoom.RoomUserCount = 0;

		NCommon::PktRoomEnterReq resPkt;
		resPkt.IsCreate = true;
		resPkt.RoomIndex = 0;
		wcscpy(resPkt.RoomTitle, m_RoomTitleTxt->caption_wstring().c_str());
		//resPkt.RoomInfo = newRoom;

		m_pRefNetwork->SendPacket((short)PACKET_ID::ROOM_ENTER_REQ, sizeof(resPkt), (char*)&resPkt);
	}

	void Init(const int maxUserCount)
	{
		SetCurSceenType(CLIENT_SCEEN_TYPE::LOBBY);
		m_MaxUserCount = maxUserCount;

		m_IsRoomListWorking = true;
		m_IsUserListWorking = true;

		m_RoomList.clear();
		m_UserList.clear();
	}
		
	void RequestRoomList(const short startIndex)
	{
		NCommon::PktLobbyRoomListReq reqPkt;
		reqPkt.StartRoomIndex = startIndex;
		m_pRefNetwork->SendPacket((short)PACKET_ID::LOBBY_ENTER_ROOM_LIST_REQ, sizeof(reqPkt), (char*)&reqPkt);
	}

	void RequestUserList(const short startIndex)
	{
		NCommon::PktLobbyUserListReq reqPkt;
		reqPkt.StartUserIndex = startIndex;
		m_pRefNetwork->SendPacket((short)PACKET_ID::LOBBY_ENTER_USER_LIST_REQ, sizeof(reqPkt), (char*)&reqPkt);
	}
	
	void SetRoomListGui()
	{
		m_IsRoomListWorking = false;

		for (auto & room : m_RoomList)
		{
			m_LobbyRoomList->at(0).append({ std::to_wstring(room.RoomIndex),
				room.RoomTitle,
				std::to_wstring(room.RoomUserCount),
				std::to_wstring(m_MaxUserCount) });
		}

		m_RoomList.clear();
	}

	void SetUserListGui()
	{
		m_IsUserListWorking = false;

		for (auto & userId : m_UserList)
		{
			m_LobbyUserList->at(0).append({ userId });
		}

		m_UserList.clear();
	}

	void UpdateRoomInfo(NCommon::RoomSmallInfo* pRoomInfo)
	{
		NCommon::RoomSmallInfo newRoom;
		memcpy(&newRoom, pRoomInfo, sizeof(NCommon::RoomSmallInfo));
		
		bool IsRemove = newRoom.RoomUserCount == 0 ? true : false;

		if (m_IsRoomListWorking)
		{
			if (IsRemove == false)
			{
				auto findIter = std::find_if(std::begin(m_RoomList), std::end(m_RoomList), [&newRoom](auto& room) { return room.RoomIndex == newRoom.RoomIndex; });

				if (findIter != std::end(m_RoomList))
				{
					wcsncpy_s(findIter->RoomTitle, NCommon::MAX_ROOM_TITLE_SIZE + 1, newRoom.RoomTitle, NCommon::MAX_ROOM_TITLE_SIZE);
					findIter->RoomUserCount = newRoom.RoomUserCount;
				}
				else
				{
					m_RoomList.push_back(newRoom);
				}
			}
			else
			{
				m_RoomList.remove_if([&newRoom](auto& room) { return room.RoomIndex == newRoom.RoomIndex; });
			}
		}
		else
		{
			std::string roomIndex(std::to_string(newRoom.RoomIndex));

			if (IsRemove == false)
			{
				for (auto& room : m_LobbyRoomList->at(0))
				{
					if (room.text(0) == roomIndex) 
					{
						room.text(1, newRoom.RoomTitle);
						room.text(2, std::to_wstring(newRoom.RoomUserCount));
						return;
					}
				}

				m_LobbyRoomList->at(0).append({ std::to_wstring(newRoom.RoomIndex),
											newRoom.RoomTitle,
										std::to_wstring(newRoom.RoomUserCount),
										std::to_wstring(m_MaxUserCount) });
			}
			else
			{
				for (auto& room : m_LobbyRoomList->at(0))
				{
					if (room.text(0) == roomIndex)
					{
						m_LobbyRoomList->erase(room);
						return;
					}
				}
			}
		}
	}

	void UpdateUserInfo(bool IsRemove, std::string userID)
	{		
		if (m_IsUserListWorking)
		{
			if (IsRemove == false)
			{
				auto findIter = std::find_if(std::begin(m_UserList), std::end(m_UserList), [&userID](auto& ID) { return ID == userID; });

				if (findIter == std::end(m_UserList))
				{
					m_UserList.push_back(userID);
				}
			}
			else
			{
				m_UserList.remove_if([&userID](auto& ID) { return ID == userID; });
			}
		}
		else
		{
			if (IsRemove == false)
			{
				for (auto& user : m_LobbyUserList->at(0))
				{
					if (user.text(0) == userID) {
						return;
					}
				}

				m_LobbyUserList->at(0).append(userID);
			}
			else
			{
				auto i = 0;
				for (auto& user : m_LobbyUserList->at(0))
				{
					if (user.text(0) == userID)
					{
						m_LobbyUserList->erase(user);
						return;
					}
				}
			}
		}
	}


	private:
		void RequestSendMessage()
		{
			char szID[64] = { 0, };
			UnicodeToAnsi(m_toUserID->caption_wstring().c_str(), 64, szID);

			if ((defaultToUserID.compare(szID) == 0) || (szID[0] == '\0'))
			{
				SendAllMessage();
			}
			else
			{
				SendSecretMessage(szID);
			}
		}

		void SendSecretMessage(char toUserID[])
		{
			NCommon::PktLobbySecretChatReq reqPkt;
			std::wstring szMsg = m_sendMessage->caption_wstring();
			wcsncpy_s(reqPkt.Msg, NCommon::MAX_LOBBY_CHAT_MSG_SIZE + 1, szMsg.c_str(), NCommon::MAX_LOBBY_CHAT_MSG_SIZE);

			strncpy_s(reqPkt.UserID, NCommon::MAX_USER_ID_SIZE + 1, toUserID, NCommon::MAX_USER_ID_SIZE);
			m_pRefNetwork->SendPacket((short)PACKET_ID::LOBBY_SECRET_CHAT_REQ, sizeof(reqPkt), (char*)&reqPkt);

		}

		void SendAllMessage()
		{

			NCommon::PktLobbyChatReq reqPkt;
			std::wstring szMsg = m_sendMessage->caption_wstring();
			wcsncpy_s(reqPkt.Msg, NCommon::MAX_LOBBY_CHAT_MSG_SIZE + 1, szMsg.c_str(), NCommon::MAX_LOBBY_CHAT_MSG_SIZE);

			m_pRefNetwork->SendPacket((short)PACKET_ID::LOBBY_CHAT_REQ, sizeof(reqPkt), (char*)&reqPkt);
		}


private:
	form* m_pForm = nullptr;
	std::shared_ptr<listbox> m_LobbyRoomList;
	std::shared_ptr<listbox> m_LobbyUserList;
	std::shared_ptr<button> m_RoomCreateBtn;
	std::shared_ptr<button> m_RoomEnterBtn;
	std::shared_ptr<textbox> m_RoomTitleTxt;
	std::shared_ptr<label> m_RoomTitleLb;
	
	int m_MaxUserCount = 0;

	bool m_IsRoomListWorking = false;
	std::list<NCommon::RoomSmallInfo> m_RoomList;

	bool m_IsUserListWorking = false;
	std::list<std::string> m_UserList;

	std::unique_ptr<listbox> m_chatBox;
	std::unique_ptr<textbox> m_sendMessage;
	std::unique_ptr<button> m_chatBtn;
	std::unique_ptr<textbox> m_toUserID;


	const std::string defaultToUserID = "Secret Chat ID";


};
