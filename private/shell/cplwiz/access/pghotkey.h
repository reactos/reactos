#ifndef _INC_PGHOTKEY_H
#define _INC_PGHOTKEY_H

#include "pgbase.h"

class CHotKeysPg : public WizardPage
{
public:
    CHotKeysPg(LPPROPSHEETPAGE ppsp);
    ~CHotKeysPg(VOID);

protected:
	void UpdateControls();

	LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnPSN_WizNext(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);
};

#endif // _INC_PGHOTKEY_H

