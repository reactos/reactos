#ifndef _INC_PGSVEDEF_H
#define _INC_PGSVEDEF_H

#include "pgbase.h"

class CSaveForDefaultUserPg : public WizardPage
{
public:
	CSaveForDefaultUserPg(LPPROPSHEETPAGE ppsp);
	~CSaveForDefaultUserPg(VOID);
	
private:
	LRESULT OnPSN_WizNext(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);
	LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
	
};

#endif // _INC_PGSVEDEF_H

