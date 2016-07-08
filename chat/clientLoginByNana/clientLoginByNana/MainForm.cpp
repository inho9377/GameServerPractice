
#include "TcpNetwork.h"
#include "ClientSceen.h"
#include "ClientSceenLogin.h"
#include "ClientSceenLobby.h"
#include "ClientSceenRoom.h"
#include "MainForm.h"


using PACKET_ID = NCommon::PACKET_ID;


MainForm::MainForm() {}

MainForm::~MainForm()
{
	if (m_Network)
	{
		m_Network->DisConnect();
	}
}

void MainForm::Init()
{
	m_Network = std::make_unique<TcpNetwork>();

	m_pClientSceen = std::make_shared<ClientSceen>();
	m_pClientSceen->SetNetwork(m_Network.get());

	m_pClientSceenLogin = std::make_shared<ClientSceenLogin>();
	m_pClientSceenLogin->SetNetwork(m_Network.get());

	m_pClientSceenLobby = std::make_shared<ClientSceenLobby>();
	m_pClientSceenLobby->SetNetwork(m_Network.get());

	m_pClientSceenRoom = std::make_shared<ClientSceenRoom>();
	m_pClientSceenRoom->SetNetwork(m_Network.get());

	//m_current_scene = m_pClientSceen;
}

void MainForm::CreateGUI()
{
	// https://moqups.com/ 

	m_fm = std::make_unique<form>(API::make_center(900, 700));
	m_fm->caption("Chat Client");

	m_pClientSceen->CreateUI(m_fm.get());

	m_pClientSceenLogin->CreateUI(m_fm.get());

	m_pClientSceenLobby->CreateUI(m_fm.get());

	m_pClientSceenRoom->CreateUI(m_fm.get());


	//m_ptxtCurState = std::make_unique<textbox>((form&)*m_fm.get(), nana::rectangle(450, 15, 120, 20));
	//m_ptxtCurState->caption("State: Disconnect");

	//m_RoomUserList = std::make_shared<listbox>((form&)*m_fm.get(), nana::rectangle(22, 522, 120, 166));
	//m_RoomUserList->append_header("UserID", 90);

	m_timer.elapse([&]() { PacketProcess(); });
	m_timer.interval(32);
	m_timer.start();

	m_UI_timer.elapse([&]() { UIProcess(); });
	m_UI_timer.interval(32);
	m_UI_timer.start();
}

void MainForm::ShowModal()
{
	m_fm->show();
	
	exec();
}

void MainForm::UIProcess()
{
	IClientSceen currentScene;
	switch (IClientSceen::GetCurSceenType())
	{
	case CLIENT_SCEEN_TYPE::CONNECT:
		m_pClientSceen->UIShow();
		m_pClientSceenLobby->UIBlind();
		m_pClientSceenLogin->UIBlind();
		m_pClientSceenRoom->UIBlind();
		break;

	case CLIENT_SCEEN_TYPE::LOGIN:
		m_pClientSceen->UIBlind();
		m_pClientSceenLobby->UIBlind();
		m_pClientSceenLogin->UIShow();
		m_pClientSceenRoom->UIBlind();
		break;

	case CLIENT_SCEEN_TYPE::LOBBY:
		m_pClientSceen->UIBlind();
		m_pClientSceenLobby->UIShow();
		m_pClientSceenLogin->UIBlind();
		m_pClientSceenRoom->UIBlind();
		break;

	case CLIENT_SCEEN_TYPE::ROOM:
		m_pClientSceen->UIBlind();
		m_pClientSceenLobby->UIBlind();
		m_pClientSceenLogin->UIBlind();
		m_pClientSceenRoom->UIShow();
		break;

	default:
		break;
	}
}

/*void MainForm::SceneChange(CLIENT_SCEEN_TYPE change_Scene_type)
{
	
	//current_SCENE_Type = change_Scene_type;
}

void MainForm::SceneChange(std::shared_ptr<IClientSceen> change_scene)
{
	m_current_scene->UIBlind();

	m_current_scene = change_scene;

	m_current_scene->UIShow();
}*/

void MainForm::PacketProcess()
{
	if (!m_Network) {
		return;
	}


	auto packet = m_Network->GetPacket();

	if (packet.PacketId != 0)
	{
		m_pClientSceen->ProcessPacket(packet.PacketId, packet.pData);
		m_pClientSceenLogin->ProcessPacket(packet.PacketId, packet.pData);
		m_pClientSceenLobby->ProcessPacket(packet.PacketId, packet.pData);
		m_pClientSceenRoom->ProcessPacket(packet.PacketId, packet.pData);

		if (packet.pData != nullptr) {
			delete[] packet.pData;
		}
	}

	//IClientSceen::GetCurSceenType();
	m_pClientSceen->Update();
	m_pClientSceenLogin->Update();
	m_pClientSceenLobby->Update();
	m_pClientSceenRoom->Update();
}

