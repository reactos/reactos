#ifndef _INC_PGSHWHLP_H
#define _INC_PGSHWHLP_H

#include "pgbase.h"

class CShowKeyboardHelpPg : public WizardPage
{
public:
    CShowKeyboardHelpPg(LPPROPSHEETPAGE ppsp);
    ~CShowKeyboardHelpPg(VOID);

protected:
	void UpdateControls();

	LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnPSN_WizNext(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);
};

#endif // _INC_PGSHWHLP_H

