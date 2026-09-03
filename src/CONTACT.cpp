
#include "CONTACT.h"

using namespace Osp::Base;
using namespace Osp::Ui;
using namespace Osp::Ui::Controls;


CONTACT::CONTACT(void)
{
}

CONTACT::~CONTACT(void)
{
}

bool
CONTACT::Initialize()
{
	Form::Construct(L"IDF_FORM1");

	return true;
}

result
CONTACT::OnInitializing(void)
{
	result r = E_SUCCESS;

	// TODO: Add your initialization code here

	return r;
}

result
CONTACT::OnTerminating(void)
{
	result r = E_SUCCESS;

	// TODO: Add your termination code here

	return r;
}


