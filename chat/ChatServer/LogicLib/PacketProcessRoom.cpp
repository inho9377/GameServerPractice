//1 header 2 cpp
//패킷 ID로 패킷 구분

#include "../../Common/Packet.h"
#include "../ServerNetLib/TcpNetwork.h"
#include "../../Common/ErrorCode.h"
#include "User.h"
#include "UserManager.h"
#include "LobbyManager.h"
#include "Lobby.h"
#include "Room.h"
#include "PacketProcess.h"

using PACKET_ID = NCommon::PACKET_ID;

namespace NLogicLib
{
	//룸에 입장
	ERROR_CODE PacketProcess::RoomEnter(PacketInfo packetInfo)
	{
		CHECK_START
			auto reqPkt = (NCommon::PktRoomEnterReq*)packetInfo.pRefData;
		NCommon::PktRoomEnterRes resPkt;

		//패킷에 해당하는 유저의 정보를 가져온다.
		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) 
		{
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);
		//현재 위치가 로비가 아니라면 에러 체크
		if (pUser->IsCurDomainInLobby() == false) 
		{
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_DOMAIN);
		}
		//로비에 해당하는 정보를 가져온다.
		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
		//로비가 없으면 에러 체크
		if (pLobby == nullptr) 
		{
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_LOBBY_INDEX);
		}
		//룸에 해당하는 정보를 미리 가져온다.
		auto pRoom = pLobby->GetRoom(reqPkt->RoomIndex);
		//룸이 없으면 에러 체크
		if (pRoom == nullptr) 
		{
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
		}

		//룸을 만들어야 하면 룸을 만든다.
		if (reqPkt->IsCreate)
		{
			auto ret = pRoom->CreateRoom(reqPkt->RoomTitle);
			if (ret != ERROR_CODE::NONE) 
			{
				CHECK_ERROR(ret);
			}
		}

		//룸에 들어가는데 성공하는지 체크
		auto enterRet = pRoom->EnterUser(pUser);
		if (enterRet != ERROR_CODE::NONE) 
		{
			CHECK_ERROR(enterRet);
		}

		//유저의 상태 변경 (룸 입장)
		pUser->EnterRoom(lobbyIndex, pRoom->GetIndex());

		//로비에 알림 (유저가 나감)
		pLobby->NotifyLobbyLeaveUserInfo(pUser);

		//로비에 Notify (유저가 방에 들어옴)
		pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());

		//룸에 Notify (유저 들어옴)
		pRoom->NotifyEnterUserInfo(pUser->GetIndex(), pUser->GetID().c_str());

		//데이터를 보냄
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(resPkt), (char*)&resPkt);
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(resPkt), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}

	ERROR_CODE PacketProcess::RoomLeave(PacketInfo packetInfo)
	{
		//룸을 떠남을 체크한다
		CHECK_START
			NCommon::PktRoomLeaveRes resPkt;

		//패킷에 해당하는 유저 정보를 가져옴
		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) 
		{
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);
		auto userIndex = pUser->GetIndex();
		//현재 상태가 룸이 아니라면 에러 체크
		if (pUser->IsCurDomainInRoom() == false) {
			CHECK_ERROR(ERROR_CODE::ROOM_LEAVE_INVALID_DOMAIN);
		}
		//현재 로비, 룸 정보를 가져와서 에러 체크 (존재하는지)
		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
		if (pLobby == nullptr) 
		{
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_LOBBY_INDEX);
		}

		auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
		if (pRoom == nullptr) 
		{
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
		}
		//유저가 방을 나감
		auto leaveRet = pRoom->LeaveUser(userIndex);
		if (leaveRet != ERROR_CODE::NONE) 
		{
			CHECK_ERROR(leaveRet);
		}

		//유저 정보를 로비입장으로 변ㄱㅇ
		pUser->EnterLobby(lobbyIndex);

		//룸에 Notyfy (유저 나감)
		pRoom->NotifyLeaveUserInfo(pUser->GetID().c_str());

		//로비에 Notify (유저 들어옴)
		pLobby->NotifyLobbyEnterUserInfo(pUser);

		//로비에 Notify (방의 정보가 변경됨)
		pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());

		//데이터 전송
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_LEAVE_RES, sizeof(resPkt), (char*)&resPkt);
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_LEAVE_RES, sizeof(resPkt), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}

	ERROR_CODE PacketProcess::RoomChat(PacketInfo packetInfo)
	{
		CHECK_START
			auto reqPkt = (NCommon::PktRoomChatReq*)packetInfo.pRefData;
		NCommon::PktRoomChatRes resPkt;

		//패킷에 해당하는 유저 정보를 불러옴
		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) 
		{
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);
		//현재 상태가 방이 아니라면 에러 체크
		if (pUser->IsCurDomainInRoom() == false) 
		{
			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_DOMAIN);
		}

		//로비와 룸에 대한 정보를 가져오고 에러 체크 (존재하는지)
		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
		if (pLobby == nullptr) 
		{
			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_LOBBY_INDEX);
		}

		auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
		if (pRoom == nullptr) 
		{
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
		}

		//채팅한다.
		pRoom->NotifyChat(pUser->GetSessioIndex(), pUser->GetID().c_str(), reqPkt->Msg);
		//해당 데이터 전송
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(resPkt), (char*)&resPkt);
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(resPkt), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}
}