#include "MainForm.h"
#include "Odbc.h"





MainForm::MainForm()
{
}


MainForm::~MainForm()
{
}


void MainForm::Init()
{
	main_form = std::make_unique<form>(API::make_center(900, 700));
	main_form->caption("Login Client");

	if (! Odbc::GetInstance()->Connect(L"mysqlUser", L"nextuser", L"a"))
	{
		PopUp("ConnectError!");
	}
}

void MainForm::CreateGUI()
{
	register_button = std::make_unique<button>((form&)*main_form, nana::rectangle(483, 220, 102, 23));
	register_button->caption("Register");
	register_button->events().click([&]() {
		this->RegisterUser();
	});


	loginCheck_button = std::make_unique<button>((form&)*main_form, nana::rectangle(483, 260, 102, 23));
	loginCheck_button->caption("LoginCheck");
	loginCheck_button->events().click([&]() {
		this->LoginCheck();
	});

	deleteUser_button = std::make_unique<button>((form&)*main_form, nana::rectangle(483, 300, 102, 23));
	deleteUser_button->caption("Delete");
	deleteUser_button->events().click([&]() {
		this->DeleteUser();
	});

	inputId_tb = std::make_unique<textbox>((form&)*main_form, nana::rectangle(150, 220, 200, 23));
	inputId_tb->caption("ID");

	inputPw_tb = std::make_unique<textbox>((form&)*main_form, nana::rectangle(150, 240, 200, 23));
	inputPw_tb->caption("password");

	
}

void MainForm::Process()
{
	main_form->show();
	exec();
}

void MainForm::RegisterUser()
{
	std::wstring UserID = inputId_tb->caption_wstring();
	std::wstring UserPW = inputPw_tb->caption_wstring();
	//std::string user = inputId_tb->caption();
	if (Odbc::GetInstance()->AddUser(UserID, UserPW))
	{
		PopUp("User Register Success!");
	}
	else
	{
		PopUp("Already registered User");
	}

}

void MainForm::LoginCheck()
{
	std::wstring UserID = inputId_tb->caption_wstring();
	std::wstring UserPW = inputPw_tb->caption_wstring();
	if (Odbc::GetInstance()->CheckLogin(UserID, UserPW))
	{
		PopUp("User Login Ok!");
	}
	else
	{
		PopUp("Not Exist User");
	}
}

void MainForm::DeleteUser()
{
	std::wstring UserID = inputId_tb->caption_wstring();
	std::wstring UserPW = inputPw_tb->caption_wstring();
	if (Odbc::GetInstance()->DeleteUser(UserID, UserPW))
	{
		PopUp("User Delete Succeee!");
	}
	else
	{
		PopUp("Not Exist User");
	}
}

void MainForm::PopUp(std::string message)
{
	msgbox m((form&)*main_form, message, nana::msgbox::ok);
	m.icon(m.icon_warning);
	m.show();
}


