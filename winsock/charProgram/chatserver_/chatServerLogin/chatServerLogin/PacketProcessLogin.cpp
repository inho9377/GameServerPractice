#include "../../../../Common/Packet.h"
#include "../../servernetLib/TcpNetwork.h"
#include "../../../../Common/ErrorCode.h"
#include "User.h"
#include "UserManager.h"
#include "LobbyManager.h"
#include "PacketProcess.h"
//대부분의 로직은 패킷 프로세스 안에 존재
//룸매니저, 로비매니저, 네트워크매니저, 패킷프로세스매니저
using PACKET_ID = NCommon::PACKET_ID;

namespace NLogicLib
{
	ERROR_CODE PacketProcess::Login(PacketInfo packetInfo)
	{

	CHECK_START
		//TODO: 받은 데이터가 PktLogInReq 크기만큼인지 조사해야 한다.
		// 패스워드는 무조건 pass 해준다.
		// ID 중복이라면 에러 처리한다.

		NCommon::PktLogInRes resPkt; //최대 34바이트까지 보낼 수 있다고 보고 처리함 (딱 34바이트를 보냈다고 생각함)
	//패킷이 패킷의 최소 최대 데이터에 맞는지 부터 확인함
	// 반 정도는 체크하는 로직
		auto reqPkt = (NCommon::PktLogInReq*)packetInfo.pRefData;
		//유저매니저에 정상적으로 유저등록
		auto addRet = m_pRefUserMgr->AddUser(packetInfo.SessionIndex, reqPkt->szID);

		if (addRet != ERROR_CODE::NONE) {
			CHECK_ERROR(addRet);
		}

		resPkt.ErrorCode = (short)addRet;
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::LOGIN_IN_RES, sizeof(NCommon::PktLogInRes), (char*)&resPkt);

		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::LOGIN_IN_RES, sizeof(NCommon::PktLogInRes), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}

	ERROR_CODE PacketProcess::LobbyList(PacketInfo packetInfo)
	{
	CHECK_START
		// 인증 받은 유저인가?
		// 아직 로비에 들어가지 않은 유저인가?
		
		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}
	
		auto pUser = std::get<1>(pUserRet);
		
		if (pUser->IsCurDomainInLogIn() == false) {
			CHECK_ERROR(ERROR_CODE::LOBBY_LIST_INVALID_DOMAIN);
		}
		
		m_pRefLobbyMgr->SendLobbyListInfo(packetInfo.SessionIndex);
		
		return ERROR_CODE::NONE;

		//에러를 한 곳에서 처리하기 위해 macro-goto문으로 처리
		//scoped-exit (boost)도 같은 역할을 해줌
	CHECK_ERR:
		NCommon::PktLobbyListRes resPkt;
		resPkt.SetError(__result);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::LOBBY_LIST_RES, sizeof(NCommon::PktLogInRes), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}
}