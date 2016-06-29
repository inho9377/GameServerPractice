#include "../../../../Common/Packet.h"
#include "../../servernetLib/TcpNetwork.h"
#include "../../../../Common/ErrorCode.h"
#include "User.h"
#include "UserManager.h"
#include "LobbyManager.h"
#include "PacketProcess.h"
//��κ��� ������ ��Ŷ ���μ��� �ȿ� ����
//��Ŵ���, �κ�Ŵ���, ��Ʈ��ũ�Ŵ���, ��Ŷ���μ����Ŵ���
using PACKET_ID = NCommon::PACKET_ID;

namespace NLogicLib
{
	ERROR_CODE PacketProcess::Login(PacketInfo packetInfo)
	{

	CHECK_START
		//TODO: ���� �����Ͱ� PktLogInReq ũ�⸸ŭ���� �����ؾ� �Ѵ�.
		// �н������ ������ pass ���ش�.
		// ID �ߺ��̶�� ���� ó���Ѵ�.

		NCommon::PktLogInRes resPkt; //�ִ� 34����Ʈ���� ���� �� �ִٰ� ���� ó���� (�� 34����Ʈ�� ���´ٰ� ������)
	//��Ŷ�� ��Ŷ�� �ּ� �ִ� �����Ϳ� �´��� ���� Ȯ����
	// �� ������ üũ�ϴ� ����
		auto reqPkt = (NCommon::PktLogInReq*)packetInfo.pRefData;
		//�����Ŵ����� ���������� �������
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
		// ���� ���� �����ΰ�?
		// ���� �κ� ���� ���� �����ΰ�?
		
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

		//������ �� ������ ó���ϱ� ���� macro-goto������ ó��
		//scoped-exit (boost)�� ���� ������ ����
	CHECK_ERR:
		NCommon::PktLobbyListRes resPkt;
		resPkt.SetError(__result);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::LOBBY_LIST_RES, sizeof(NCommon::PktLogInRes), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}
}