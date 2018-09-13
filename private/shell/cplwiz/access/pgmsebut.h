#ifndef _INC_PGMSEBUT_H
#define _INC_PGMSEBUT_H

#include "pgbase.h"

class CMouseButtonPg : public WizardPage
{
public:
    CMouseButtonPg(LPPROPSHEETPAGE ppsp);
    ~CMouseButtonPg(VOID);

protected:
	POINT m_ptRight;
	POINT m_ptLeft;
	void UpdateControls();

	LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnPSN_WizNext(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);
};

#endif // _INC_PGMSEBUT_H

