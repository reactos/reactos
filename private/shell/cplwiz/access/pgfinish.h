#ifndef __FINISH_WIZARD_PAGE_H
#define __FINISH_WIZARD_PAGE_H

#include "pgbase.h"

class FinishWizPg : public WizardPage
{
public:
	FinishWizPg(LPPROPSHEETPAGE ppsp);
	~FinishWizPg(VOID);
	
private:
	
	LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnPSN_WizFinish(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);
	LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
	
};

#endif // __FINISH_WIZARD_PAGE_H

