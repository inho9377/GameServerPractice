#pragma once

namespace NCommon
{


	// 21 이상부터 시작해야 한다!!!
	enum class PACKET_ID : short
	{
		LOGIN_IN_REQ = 21,
		LOGIN_IN_RES = 22,

		LOBBY_LIST_REQ = 26,
		LOBBY_LIST_RES = 27,

		LOBBY_ENTER_REQ = 31,
		LOBBY_ENTER_RES = 32,
		LOBBY_ENTER_USER_NTF = 33,

		LOBBY_ENTER_ROOM_LIST_REQ = 41,
		LOBBY_ENTER_ROOM_LIST_RES = 42,

		LOBBY_ENTER_USER_LIST_REQ = 43,
		LOBBY_ENTER_USER_LIST_RES = 44,

		LOBBY_LEAVE_REQ = 46,
		LOBBY_LEAVE_RES = 47,
		LOBBY_LEAVE_USER_NTF = 48,

		LOBBY_CHAT_REQ = 50,
		LOBBY_CHAT_RES = 51,
		LOBBY_CHAT_NTF = 52,

		LOBBY_SECRET_CHAT_REQ = 55,
		LOBBY_SECRET_CHAT_RES = 56,
		LOBBY_SECRET_CHAT_NTF = 57,

		ROOM_ENTER_REQ = 61,
		ROOM_ENTER_RES = 62,
		ROOM_ENTER_USER_NTF = 63,

		ROOM_LEAVE_REQ = 66,
		ROOM_LEAVE_RES = 67,
		ROOM_LEAVE_USER_NTF = 68,

		ROOM_CHANGED_INFO_NTF = 71,

		ROOM_CHAT_REQ = 76,
		ROOM_CHAT_RES = 77,
		ROOM_CHAT_NTF = 78,

		ROOM_ENTER_USER_LIST_REQ = 86,
		ROOM_ENTER_USER_LIST_RES = 87,

		MAX = 256
	};

}