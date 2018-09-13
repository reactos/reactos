#ifndef _INC_PGSTKKEY_H
#define _INC_PGSTKKEY_H

#include "pgbase.h"

class CStickyKeysPg : public WizardPage
{
public:
    CStickyKeysPg(LPPROPSHEETPAGE ppsp);
    ~CStickyKeysPg(VOID);

protected:
	void UpdateControls();

	LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnPSN_WizNext(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);
};

#endif // _INC_PGSTKKEY_H

