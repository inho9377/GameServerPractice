#include "../../Common/Packet.h"
#include "../ServerNetLib/TcpNetwork.h"
#include "../../Common/ErrorCode.h"
#include "User.h"
#include "UserManager.h"
#include "Lobby.h"
#include "LobbyManager.h"
#include "PacketProcess.h"

using PACKET_ID = NCommon::PACKET_ID;

//로그인 성공시 서버에 LOBBY_LIST_REQ를 보내고
//서버는 그걸 받고 LOBBY_LIST_RES를 보냄
namespace NLogicLib
{
	ERROR_CODE PacketProcess::LobbyEnter(PacketInfo packetInfo)
	{
		CHECK_START
			// 현재 위치 상태는 로그인이 맞나?
			// 로비에 들어간다.
			// 기존 로비에 있는 사람에게 새 사람이 들어왔다고 알려준다

			//클라이언트는 로비 아이디를 보내고 입장을 요청
			auto reqPkt = (NCommon::PktLobbyEnterReq/*로비 아이디*/*)packetInfo.pRefData;
		NCommon::PktLobbyEnterRes resPkt;

		//유저 매니저로부터 해당 패킷에 해당하는 유저를 가져온다.
		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);
		//에러코드 체크
		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);
		//로그인된 상태여야됨
		if (pUser->IsCurDomainInLogIn() == false) {
			CHECK_ERROR(ERROR_CODE::LOBBY_ENTER_INVALID_DOMAIN);
		}
		//해당 번호 로비가 있는지 확인
		auto pLobby = m_pRefLobbyMgr->GetLobby(reqPkt->LobbyId);
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::LOBBY_ENTER_INVALID_LOBBY_INDEX);
		}
		//Lobby로 유저가 입장
		auto enterRet = pLobby->EnterUser(pUser);
		if (enterRet != ERROR_CODE::NONE) {
			CHECK_ERROR(enterRet);
		}
		//유저들에게? 입장함을 알려줌
		pLobby->NotifyLobbyEnterUserInfo(pUser);

		//보낼 패킷에 정보 추가
		resPkt.MaxUserCount = pLobby->MaxUserCount();
		resPkt.MaxRoomCount = pLobby->MaxRoomCount();
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::LOBBY_ENTER_RES, sizeof(NCommon::PktLobbyEnterRes), (char*)&resPkt);
		return ERROR_CODE::NONE;

		//에러 처리
	CHECK_ERR:
		resPkt.SetError(__result);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::LOBBY_ENTER_RES, sizeof(NCommon::PktLobbyEnterRes), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}

	ERROR_CODE PacketProcess::LobbyRoomList(PacketInfo packetInfo)
	{
		CHECK_START
			// 현재 로비에 있는지 조사한다.
			// 룸 리스트를 보내준다.

			//해당 패킷에 해당하는 유저를 가져온다.
			auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);

		//유저가 Lobby에 있는지 조사
		if (pUser->IsCurDomainInLobby() == false) {
			CHECK_ERROR(ERROR_CODE::LOBBY_ROOM_LIST_INVALID_DOMAIN);
		}

		//로비 매니저로부터 유저가 들어있는 로비 가져옴
		auto pLobby = m_pRefLobbyMgr->GetLobby(pUser->GetLobbyIndex());
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::LOBBY_ROOM_LIST_INVALID_LOBBY_INDEX);
		}

		auto reqPkt = (NCommon::PktLobbyRoomListReq*)packetInfo.pRefData;

		pLobby->SendRoomList(pUser->GetSessioIndex(), reqPkt->StartRoomIndex);

		return ERROR_CODE::NONE;

	CHECK_ERR:
		NCommon::PktLobbyRoomListRes resPkt;
		resPkt.SetError(__result);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::LOBBY_ENTER_ROOM_LIST_RES, sizeof(NCommon::PktBase), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}

	ERROR_CODE PacketProcess::LobbyUserList(PacketInfo packetInfo)
	{
		CHECK_START
			// 현재 로비에 있는지 조사한다.
			// 유저 리스트를 보내준다.

			auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);

		if (pUser->IsCurDomainInLobby() == false) {
			CHECK_ERROR(ERROR_CODE::LOBBY_USER_LIST_INVALID_DOMAIN);
		}

		auto pLobby = m_pRefLobbyMgr->GetLobby(pUser->GetLobbyIndex());
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::LOBBY_USER_LIST_INVALID_LOBBY_INDEX);
		}

		auto reqPkt = (NCommon::PktLobbyUserListReq*)packetInfo.pRefData;

		pLobby->SendUserList(pUser->GetSessioIndex(), reqPkt->StartUserIndex);

		return ERROR_CODE::NONE;
	CHECK_ERR:
		NCommon::PktLobbyUserListRes resPkt;
		resPkt.SetError(__result);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::LOBBY_ENTER_USER_LIST_RES, sizeof(NCommon::PktBase), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}


	ERROR_CODE PacketProcess::LobbyLeave(PacketInfo packetInfo)
	{
		CHECK_START
			// 현재 로비에 있는지 조사한다.
			// 로비에서 나간다
			// 기존 로비에 있는 사람에게 나가는 사람이 있다고 알려준다.
			NCommon::PktLobbyLeaveRes resPkt;

		//해당 패킷의 유저(나갈 유저)를 가져옴
		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);
		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<
			1>(pUserRet);

		if (pUser->IsCurDomainInLobby() == false) {
			CHECK_ERROR(ERROR_CODE::LOBBY_LEAVE_INVALID_DOMAIN);
		}
		//해당 로비가 존재하는지 체크
		auto pLobby = m_pRefLobbyMgr->GetLobby(pUser->GetLobbyIndex());
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::LOBBY_LEAVE_INVALID_LOBBY_INDEX);
		}
		//로비에서 해당 유저를 나가게 함
		auto enterRet = pLobby->LeaveUser(pUser->GetIndex());
		if (enterRet != ERROR_CODE::NONE) {
			CHECK_ERROR(enterRet);
		}
		//로비에서 나갔음을 알림
		pLobby->NotifyLobbyLeaveUserInfo(pUser);
		//네트워크를 통해 해당 데이터를 전송
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::LOBBY_LEAVE_RES, sizeof(NCommon::PktLobbyLeaveRes), (char*)&resPkt);

		return ERROR_CODE::NONE;
	CHECK_ERR:
		resPkt.SetError(__result);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::LOBBY_LEAVE_RES, sizeof(NCommon::PktLobbyLeaveRes), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}
}