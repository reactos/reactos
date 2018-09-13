#ifndef _INC_PGTGLKEY_H
#define _INC_PGTGLKEY_H

#include "pgbase.h"

class CToggleKeysPg : public WizardPage
{
public:
    CToggleKeysPg(LPPROPSHEETPAGE ppsp);
    ~CToggleKeysPg(VOID);

protected:
	void UpdateControls();

	LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnPSN_WizNext(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);
};

#endif // _INC_PGTGLKEY_H

