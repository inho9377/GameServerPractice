#pragma once
#include <memory>
#include <nana/gui/wvl.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/timer.hpp>
#include <nana/gui.hpp>


using namespace nana;

class TcpNetwork;

class IClientSceen;
class ClientSceen;
class ClientSceenLogin;
class ClientSceenLobby;


class MainForm
{
public:
	MainForm();
	~MainForm();

	void Init();

	void CreateGUI();

	void ShowModal();


private:
	void PacketProcess();

private:
	std::unique_ptr<TcpNetwork> m_Network;

	bool m_IsLogined = false;


private:
	std::unique_ptr<form> m_fm;

	timer m_timer;

	std::unique_ptr<textbox> m_ptxtCurState;

	std::shared_ptr<listbox> m_RoomUserList;

	std::shared_ptr<ClientSceen> m_pClientSceen;
	std::shared_ptr<ClientSceenLogin> m_pClientSceenLogin;
	std::shared_ptr<ClientSceenLobby> m_pClientSceenLobby;
};