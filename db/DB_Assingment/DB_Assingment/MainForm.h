
#include <nana/gui/wvl.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/timer.hpp>
#include <nana/gui.hpp>
#include <memory>


using namespace nana;


class MainForm
{
public:
	MainForm();
	~MainForm();



	void Init();
	void CreateGUI();
	void Process();

private:

	void RegisterUser();
	void LoginCheck();
	void DeleteUser();

	void PopUp(std::string message);

	std::unique_ptr<form> main_form;

	std::unique_ptr<button> register_button;
	std::unique_ptr<button> loginCheck_button;
	std::unique_ptr<button> deleteUser_button;

	std::unique_ptr<textbox> inputId_tb;
	std::unique_ptr<textbox> inputPw_tb;

	


};

