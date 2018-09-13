#ifndef _INC_PGFLTKEY_H
#define _INC_PGFLTKEY_H

#include "pgbase.h"

class CFilterKeysPg : public WizardPage
{
public:
    CFilterKeysPg(LPPROPSHEETPAGE ppsp);
    ~CFilterKeysPg(VOID);

protected:
	void UpdateControls();
	void UpdateSliders();

	LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT HandleMsg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnPSN_WizNext(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);
};

#endif // _INC_PGFLTKEY_H

