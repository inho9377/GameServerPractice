#pragma once

#include <nana/gui/wvl.hpp>
#include <nana/gui/widgets/label.hpp>
#include <memory>
using namespace nana;


class CUiManager
{
public:
	CUiManager();
	~CUiManager();


private:
	std::unique_ptr<form> m_pForm;
	std::shared_ptr<label> m_lb;
	//std::shared_ptr<textbox>
};

