#ifndef _INC_PGTMEOUT_H
#define _INC_PGTMEOUT_H

#include "pgbase.h"

class CAccessTimeOutPg : public WizardPage
{
public:
	CAccessTimeOutPg(LPPROPSHEETPAGE ppsp, int dwPageId);
	~CAccessTimeOutPg(VOID);

protected:
	void UpdateControls();

	LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnPSN_WizNext(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);
};

class CAccessTimeOutPg1 : public CAccessTimeOutPg
{
public:
	CAccessTimeOutPg1(LPPROPSHEETPAGE ppsp) : CAccessTimeOutPg(ppsp, IDD_WIZACCESSTIMEOUT1) {};
};

class CAccessTimeOutPg2 : public CAccessTimeOutPg
{
public:
	CAccessTimeOutPg2(LPPROPSHEETPAGE ppsp) : CAccessTimeOutPg(ppsp, IDD_WIZACCESSTIMEOUT2) {};
};


#endif // _INC_PGTMEOUT_H

