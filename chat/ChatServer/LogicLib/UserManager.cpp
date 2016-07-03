#include <algorithm>
#include "../../Common/ErrorCode.h"
#include "User.h"
#include "UserManager.h"

namespace NLogicLib
{
	UserManager::UserManager()
	{
	}

	UserManager::~UserManager()
	{
	}

	//초기화 작업
	void UserManager::Init(const int maxUserCount)
	{
		//MAx User Count만큼 유저 클래스를 만들어놓는다.
		for (int i = 0; i < maxUserCount; ++i)
		{
			User user;
			user.Init((short)i);

			m_UserObjPool.push_back(user);
			m_UserObjPoolIndex.push_back(i);
		}
	}

	//유저에게 각각의 인덱스 할당
	User* UserManager::AllocUserObjPoolIndex()
	{
		if (m_UserObjPoolIndex.empty()) {
			return nullptr;
		}

		int index = m_UserObjPoolIndex.front();
		m_UserObjPoolIndex.pop_front();
		return &m_UserObjPool[index];
	}

	//유저 인덱스를 해제
	void UserManager::ReleaseUserObjPoolIndex(const int index)
	{
		m_UserObjPoolIndex.push_back(index);
		m_UserObjPool[index].Clear();
	}

	//유저 매니저에 유저를 추가한다
	ERROR_CODE UserManager::AddUser(const int sessionIndex, const char* pszID)
	{
		if (FindUser(pszID) != nullptr) {
			return ERROR_CODE::USER_MGR_ID_DUPLICATION;
		}

		//최대 유저를 넘어서면 알려주고 못 들어가게 (혹은 대기열 등)
		auto pUser = AllocUserObjPoolIndex();
		if (pUser == nullptr) {
			return ERROR_CODE::USER_MGR_MAX_USER_COUNT;
		}

		pUser->Set(sessionIndex, pszID);

		m_UserSessionDic.insert({ sessionIndex, pUser });
		//특정유저들에 행동할 때 ID이용
		m_UserIDDic.insert({ pszID, pUser });

		return ERROR_CODE::NONE;
	}

	//유저 매니저에 유저를 제거한다.
	ERROR_CODE UserManager::RemoveUser(const int sessionIndex)
	{
		auto pUser = FindUser(sessionIndex);

		if (pUser == nullptr) {
			return ERROR_CODE::USER_MGR_REMOVE_INVALID_SESSION;
		}

		auto index = pUser->GetIndex();
		auto pszID = pUser->GetID();

		m_UserSessionDic.erase(sessionIndex);
		m_UserIDDic.erase(pszID.c_str());
		ReleaseUserObjPoolIndex(index);

		return ERROR_CODE::NONE;
	}

	//Client Session에 해당하는 유저를 가져온다.
	std::tuple<ERROR_CODE, User*> UserManager::GetUser(const int sessionIndex)
	{
		auto pUser = FindUser(sessionIndex);

		if (pUser == nullptr) {
			return{ std::tuple<ERROR_CODE, User*> {ERROR_CODE::USER_MGR_INVALID_SESSION_INDEX, nullptr} };
		}

		if (pUser->IsConfirm() == false) {
			return{ std::tuple<ERROR_CODE, User*>{ERROR_CODE::USER_MGR_NOT_CONFIRM_USER, nullptr} };
		}

		return{ std::tuple<ERROR_CODE, User*>{ERROR_CODE::NONE, pUser} };
	}

	//Client Session에 해당하는 유저를 찾는다
	User* UserManager::FindUser(const int sessionIndex)
	{
		auto findIter = m_UserSessionDic.find(sessionIndex);

		if (findIter == m_UserSessionDic.end()) {
			return nullptr;
		}

		//auto pUser = (User*)&findIter->second;
		return (User*)findIter->second;
	}

	//ID(char)에 해당하는 유저를 찾는다.
	User* UserManager::FindUser(const char* pszID)
	{
		auto findIter = m_UserIDDic.find(pszID);

		if (findIter == m_UserIDDic.end()) {
			return nullptr;
		}

		return (User*)findIter->second;
	}
}