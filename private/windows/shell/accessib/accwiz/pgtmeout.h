#ifndef _INC_PGTMEOUT_H
#define _INC_PGTMEOUT_H

#include "pgbase.h"

class CAccessTimeOutPg : public WizardPage
{
public:
	CAccessTimeOutPg(LPPROPSHEETPAGE ppsp);
	~CAccessTimeOutPg(VOID);

protected:
	void UpdateControls();

	LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnPSN_WizNext(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);
};



#endif // _INC_PGTMEOUT_H

