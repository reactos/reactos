#ifndef _INC_PGMSEKEY_H
#define _INC_PGMSEKEY_H

#include "pgbase.h"

class CMouseKeysPg : public WizardPage
{
public:
    CMouseKeysPg(LPPROPSHEETPAGE ppsp);
    ~CMouseKeysPg(VOID);

protected:
	void UpdateControls();

	LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnPSN_WizNext(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);
};

#endif // _INC_PGMSEKEY_H

