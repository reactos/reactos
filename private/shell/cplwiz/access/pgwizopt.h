#ifndef _INC_PGWIZOPT_H
#define _INC_PGWIZOPT_H

#include "pgbase.h"

class CWizardOptionsPg : public WizardPage
{
public:
	CWizardOptionsPg(LPPROPSHEETPAGE ppsp);
	~CWizardOptionsPg(VOID);

protected:
	BOOL AdjustWizPageOrder();
	LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);

};

#endif // _INC_PGWIZOPT_H

