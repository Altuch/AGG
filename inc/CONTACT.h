
#ifndef _CONTACT_H_
#define _CONTACT_H_

#include <FBase.h>
#include <FUi.h>



class CONTACT :
	public Osp::Ui::Controls::Form
{

// Construction
public:
	CONTACT(void);
	virtual ~CONTACT(void);
	bool Initialize();
	result OnInitializing(void);
	result OnTerminating(void);

// Implementation
protected:

// Generated call-back functions
public:

};

#endif
