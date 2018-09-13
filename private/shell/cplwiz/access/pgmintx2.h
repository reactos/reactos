#ifndef _INC_PGMINTX2_H
#define _INC_PGMINTX2_H

#include "pgbase.h"

struct CDisplayModeInfo
{
	CDisplayModeInfo() {memset(&m_DevMode, 0, sizeof(m_DevMode));m_bCanUse = FALSE;}
	DEVMODE m_DevMode;
	BOOL m_bCanUse;
};

class CMinText2Pg : public WizardPage
{
public:
	CMinText2Pg(LPPROPSHEETPAGE ppsp);
	~CMinText2Pg(VOID);
	
protected:
	void UpdateControls();
	CDisplayModeInfo *m_pDisplayModes;
	int m_nDisplayModes;
	int m_nBestDisplayMode;
	
	LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnPSN_SetActive(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);
	LRESULT OnPSN_WizNext(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);
	
private:
	
};

#endif // _INC_PGMINTX2_H

