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
		if (GetCurSceenType() != CLIENT_SCEEN_TYPE::ROOM) {
			return;
		}

		SetUserListGui();

	}


	void Init()
	{
		SetCurSceenType(CLIENT_SCEEN_TYPE::ROOM);
		m_MaxUserCount = 35;


		m_IsUserListWorking = true;

		m_UserList.clear();
	}

	
	bool ProcessPacket(const short packetId, char* pData) override
	{
		switch (packetId)
		{
		case (short)PACKET_ID::ROOM_ENTER_RES:
		{
			auto pktRes = (NCommon::PktRoomEnterRes*)pData;
			if (pktRes->ErrorCode == (short)NCommon::ERROR_CODE::NONE)
			{
				Init();
				RequestUserList(0);
			}
			else
			{
				std::cout << "[LOBBY_ENTER_RES] ErrorCode: " << pktRes->ErrorCode << std::endl;
			}

			break;
		}

		case (short)PACKET_ID::ROOM_ENTER_USER_LIST_RES:
		{
			auto pktRes = (NCommon::PktRoomUserListRes*)pData;
			for (int i = 0; i < pktRes->Count; ++i)
			{
				UpdateUserInfo(false, pktRes->UserInfo[i].UserID);
			}
			if (pktRes->IsEnd == false)
			{

				RequestUserList(pktRes->UserInfo[pktRes->Count - 1].LobbyUserIndex + 1);
			}
			else
			{
				SetUserListGui();
			}
			break;
		}
		case (short)PACKET_ID::ROOM_ENTER_USER_NTF:
		{
			auto pktRes = (NCommon::PktRoomEnterUserInfoNtf*)pData;
			UpdateUserInfo(false, pktRes->UserID);
		}
		break;
		case (short)PACKET_ID::ROOM_LEAVE_USER_NTF:
		{
			auto pktRes = (NCommon::PktRoomLeaveUserInfoNtf*)pData;
			UpdateUserInfo(true, pktRes->UserID);
		}
		break;
		case (short)PACKET_ID::ROOM_CHAT_RES:
		{
			auto pktRes = (NCommon::PktLobbyChatRes*)pData;
			if (pktRes->ErrorCode == (short)NCommon::ERROR_CODE::NONE)
			{
				ShowChatMessage(L"___", m_beforeInputMessage);
				m_beforeInputMessage.clear();
			}
			else
			{
				std::cout << "[ROOM_CHAT_RES] ErrorCode: " << pktRes->ErrorCode << std::endl;
			}

			break;
		}

		case (short)PACKET_ID::ROOM_CHAT_NTF:
		{
			auto pktRes = (NCommon::PktLobbyChatNtf*)pData;
			ShowChatMessage(pktRes->UserID, pktRes->Msg);
			break;
		}
		case (short)PACKET_ID::ROOM_LEAVE_RES:
		{
			auto pktRes = (NCommon::PktRoomLeaveRes*)pData;
			if (pktRes->ErrorCode == (short)NCommon::ERROR_CODE::NONE)
			{
				SetCurSceenType(CLIENT_SCEEN_TYPE::LOBBY);
				m_UserList.clear();
				m_RoomUserList->clear();
				m_chatBox->clear();
			}
			else
			{
				std::cout << "[ROOM_LEAVE_RES] ErrorCode: " << pktRes->ErrorCode << std::endl;
			}
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

		m_chatBox = std::make_shared<listbox>((form&)*m_pForm, nana::rectangle(150, 10, 500, 550));
		m_chatBox->append_header("UserID", 50);
		m_chatBox->append_header("Text", 350);


		m_RoomUserList = std::make_shared<listbox>((form&)*m_pForm, nana::rectangle(10, 10, 120, 650));
		m_RoomUserList->append_header("UserID", 90);

		m_ExitButton = std::make_shared<button>((form&)*m_pForm, nana::rectangle(720, 10, 102, 23));
		m_ExitButton->caption("Exit");
		m_ExitButton->events().click([&]() {
			this->RequestRoomLeave();
			//this->LogInOut();
		});
		m_ExitButton->enabled(true);

		m_sendMessage = std::make_shared<textbox>((form&)*m_pForm, nana::rectangle(150, 570, 400, 23));
		m_sendMessage->caption("Send Message..");

		m_chatBtn = std::make_shared<button>((form&)*m_pForm, nana::rectangle(150, 600, 102, 23));
		m_chatBtn->caption("Chat");
		m_chatBtn->events().click([&]() {
			this->RequestSendMessage();
			//this->LogInOut();
		});
		m_chatBtn->enabled(true);

	}


	void UIShow()
	{
		m_RoomUserList->show();
		m_ExitButton->show();
		m_chatBox->show();
		m_sendMessage->show();
		m_chatBtn->show();
	}
	void UIBlind()
	{
		m_RoomUserList->hide();
		m_ExitButton->hide();
		m_chatBox->hide();
		m_sendMessage->hide();
		m_chatBtn->hide();
	}


private:


	void RequestRoomLeave()
	{
		NCommon::PktRoomLeaveReq reqPkt;
		m_pRefNetwork->SendPacket((short)PACKET_ID::ROOM_LEAVE_REQ, sizeof(reqPkt), (char*)&reqPkt);
	}

	void RequestSendMessage()
	{
		NCommon::PktLobbyChatReq reqPkt;
		std::wstring szMsg = m_sendMessage->caption_wstring();
		m_beforeInputMessage.assign(szMsg.begin(), szMsg.end());
		wcsncpy_s(reqPkt.Msg, NCommon::MAX_LOBBY_CHAT_MSG_SIZE + 1, szMsg.c_str(), NCommon::MAX_LOBBY_CHAT_MSG_SIZE);

		m_pRefNetwork->SendPacket((short)PACKET_ID::ROOM_CHAT_REQ, sizeof(reqPkt), (char*)&reqPkt);
	}

	void RequestUserList(const short startIndex)
	{
		NCommon::PktRoomUserListReq reqPkt;
		reqPkt.StartUserIndex = startIndex;
		m_pRefNetwork->SendPacket((short)PACKET_ID::ROOM_ENTER_USER_LIST_REQ, sizeof(reqPkt), (char*)&reqPkt);
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
				for (auto& user : m_RoomUserList->at(0))
				{
					if (user.text(0) == userID) {
						return;
					}
				}

				m_RoomUserList->at(0).append(userID);
			}
			else
			{
				auto i = 0;
				for (auto& user : m_RoomUserList->at(0))
				{
					if (user.text(0) == userID)
					{
						m_RoomUserList->erase(user);
						return;
					}
				}
			}
		}
	}

	void SetUserListGui()
	{
		m_IsUserListWorking = false;

		for (auto & userId : m_UserList)
		{
			m_RoomUserList->at(0).append({ userId });
		}

		m_UserList.clear();
	}

	void ShowChatMessage(std::string userID, wchar_t* msg)
	{
		std::wstring wsTmp(userID.begin(), userID.end());


		//std::string se = "(se";
		//std::string strID = userID /*+ se*/;
		std::wstring strMsg = msg;
		m_chatBox->at(0)->append({ wsTmp , strMsg });
	}

	void ShowChatMessage(std::wstring userID, std::wstring msg)
	{
		//std::string strID = userID;
		//std::string strMsg;
		//strMsg.assign(msg.begin(), msg.end());
		m_chatBox->at(0)->append({ userID , msg });
	}/*
	void ShowChatMessage(char* userID, wchar_t* msg)
	{
		std::string strID = userID;
		char szMsg[64] = { 0, };
		UnicodeToAnsi(msg, 64, szMsg);
		std::string strMsg = szMsg;
		m_chatBox->at(0).append({ strID , szMsg });
	}
	void ShowChatMessage(char* userID, std::wstring msg)
	{
		std::string strID = userID;
		std::string strMsg;
		strMsg.assign(msg.begin(), msg.end());
		m_chatBox->at(0).append({ strID , strMsg });
	}*/
private:

	form* m_pForm = nullptr;

	//std::shared_ptr<label> m_RoomTitleLb;
	bool m_IsUserListWorking = false;
	std::shared_ptr<listbox> m_RoomUserList;

	std::shared_ptr<button> m_ExitButton;
	std::shared_ptr<listbox> m_chatBox;
	std::shared_ptr<textbox> m_sendMessage;
	std::shared_ptr<button> m_chatBtn;

	int m_MaxUserCount;
	std::list<std::string> m_UserList;
	std::wstring m_beforeInputMessage;

	std::chrono::system_clock::time_point m_TimeLastedReqLobbyList = std::chrono::system_clock::now();
};