#ifndef _INC_PGWIZWIZ_H
#define _INC_PGWIZWIZ_H

#include "pgbase.h"

class CWizWizPg : public WizardPage
{
public:
    CWizWizPg(LPPROPSHEETPAGE ppsp);
    ~CWizWizPg(VOID);

protected:
	void UpdateControls();

	LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);

	virtual LRESULT OnPSN_KillActive(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);
};

#endif // _INC_PGWIZWIZ_H

