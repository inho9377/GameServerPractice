#include <algorithm>

#include "../ServerNetLib/ILog.h"
#include "../ServerNetLib/TcpNetwork.h"
#include "../../Common/Packet.h"
#include "../../Common/ErrorCode.h"

#include "User.h"
#include "Room.h"

using PACKET_ID = NCommon::PACKET_ID;

namespace NLogicLib
{
	Room::Room() {}

	Room::~Room() {}



	//Const붙일수있으면다붙임
	void Room::Init(const short index, const short maxUserCount)
	{
		//룸 초기화
		m_Index = index;
		m_MaxUserCount = maxUserCount;
	}

	//네트워크 세팅후 적용
	void Room::SetNetwork(TcpNet* pNetwork, ILog* pLogger)
	{
		m_pRefLogger = pLogger;
		m_pRefNetwork = pNetwork;
	}

	//룸 삭제
	void Room::Clear()
	{
		m_IsUsed = false;
		m_Title = L"";
		m_UserList.clear();
	}

	//룸을 생성한다.
	ERROR_CODE Room::CreateRoom(const wchar_t* pRoomTitle)
	{
		if (m_IsUsed) {
			return ERROR_CODE::ROOM_ENTER_CREATE_FAIL;
		}

		m_IsUsed = true;
		m_Title = pRoomTitle;

		return ERROR_CODE::NONE;
	}

	//유저 입장
	ERROR_CODE Room::EnterUser(User* pUser)
	{
		if (m_IsUsed == false) {
			return ERROR_CODE::ROOM_ENTER_NOT_CREATED;
		}

		if (m_UserList.size() == m_MaxUserCount) {
			return ERROR_CODE::ROOM_ENTER_MEMBER_FULL;
		}

		m_UserList.push_back(pUser);
		return ERROR_CODE::NONE;
	}

	//유저가 방에서 로비로 떠남
	ERROR_CODE Room::LeaveUser(const short userIndex)
	{
		if (m_IsUsed == false) {
			return ERROR_CODE::ROOM_ENTER_NOT_CREATED;
		}

		//유저가 목록에 존재하면 제외
		auto iter = std::find_if(std::begin(m_UserList), std::end(m_UserList), [userIndex](auto pUser) { return pUser->GetIndex() == userIndex; });
		if (iter == std::end(m_UserList)) {
			return ERROR_CODE::ROOM_LEAVE_NOT_MEMBER;
		}

		m_UserList.erase(iter);

		if (m_UserList.empty())
		{
			Clear();
		}

		return ERROR_CODE::NONE;
	}

	//모든 유저에게 데이터를 보냄 (채팅)
	void Room::SendToAllUser(const short packetId, const short dataSize, char* pData, const int passUserindex)
	{
		for (auto pUser : m_UserList)
		{
			if (pUser->GetIndex() == passUserindex) {
				continue;
			}

			m_pRefNetwork->SendData(pUser->GetSessioIndex(), packetId, dataSize, pData);
		}
	}

	//유저가 입장했을때 모든 다른 유저에게 그 사실을 알림
	void Room::NotifyEnterUserInfo(const int userIndex, const char* pszUserID)
	{
		NCommon::PktRoomEnterUserInfoNtf pkt;
		strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserID, NCommon::MAX_USER_ID_SIZE);

		SendToAllUser((short)PACKET_ID::ROOM_ENTER_USER_NTF, sizeof(pkt), (char*)&pkt, userIndex);
	}

	//유저가 퇴장했을때 모든 다른 유저에게 그 사실을 알림
	void Room::NotifyLeaveUserInfo(const char* pszUserID)
	{
		if (m_IsUsed == false) {
			return;
		}

		NCommon::PktRoomLeaveUserInfoNtf pkt;
		strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserID, NCommon::MAX_USER_ID_SIZE);

		SendToAllUser((short)PACKET_ID::ROOM_LEAVE_USER_NTF, sizeof(pkt), (char*)&pkt);
	}

	//채팅
	void Room::NotifyChat(const int sessionIndex, const char* pszUserID, const wchar_t* pszMsg)
	{
		NCommon::PktRoomChatNtf pkt;
		strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserID, NCommon::MAX_USER_ID_SIZE);
		wcsncpy_s(pkt.Msg, NCommon::MAX_ROOM_CHAT_MSG_SIZE + 1, pszMsg, NCommon::MAX_ROOM_CHAT_MSG_SIZE);

		SendToAllUser((short)PACKET_ID::ROOM_CHAT_NTF, sizeof(pkt), (char*)&pkt, sessionIndex);
	}
}