#pragma once

#include <vector>
#include <string>

#include "User.h"

namespace NServerNetLib { class ITcpNetwork; }
namespace NServerNetLib { class ILog; }
namespace NCommon { enum class ERROR_CODE :short; }

using ERROR_CODE = NCommon::ERROR_CODE;

namespace NLogicLib
{
	using TcpNet = NServerNetLib::ITcpNetwork;
	using ILog = NServerNetLib::ILog;

	class Room
	{
	public:
		Room();
		virtual ~Room();

		void Init(const short index, const short maxUserCount);

		void SetNetwork(TcpNet* pNetwork, ILog* pLogger);

		void Clear();

		short GetIndex() { return m_Index; }

		bool IsUsed() { return m_IsUsed; }

		const wchar_t* GetTitle() { return m_Title.c_str(); }

		short MaxUserCount() { return m_MaxUserCount; }

		short GetUserCount() { return (short)m_UserList.size(); }

		ERROR_CODE CreateRoom(const wchar_t* pRoomTitle);

		ERROR_CODE EnterUser(User* pUser);

		ERROR_CODE LeaveUser(const short userIndex);


		void SendToAllUser(const short packetId, const short dataSize, char* pData, const int passUserindex = -1);

		void NotifyEnterUserInfo(const int userIndex, const char* pszUserID);

		void NotifyLeaveUserInfo(const char* pszUserID);

		void NotifyChat(const int sessionIndex, const char* pszUserID, const wchar_t* pszMsg);

	private:
		//�α���� ���� Ŭ����
		ILog* m_pRefLogger;

		//��Ʈ��ũ Ŭ����
		TcpNet* m_pRefNetwork;

		//���� �ε���
		short m_Index = -1;
		//�� ���� �ִ� ������
		short m_MaxUserCount;

		//��밡���Ѱ��� ����
		bool m_IsUsed = false;

		//���� �̸�
		std::wstring m_Title;

		//������ �������� ���
		std::vector<User*> m_UserList;
	};
}