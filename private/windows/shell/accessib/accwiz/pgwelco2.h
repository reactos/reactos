#ifndef _INC_PGMINTX2_H
#define _INC_PGMINTX2_H

#include "pgbase.h"

struct CDisplayModeInfo
{
	CDisplayModeInfo() {memset(&m_DevMode, 0, sizeof(m_DevMode));m_bCanUse = FALSE;}
	DEVMODE m_DevMode;
	BOOL m_bCanUse;
};

class CWelcome2Pg : public WizardPage
{
public:
	CWelcome2Pg(LPPROPSHEETPAGE ppsp);
	~CWelcome2Pg(VOID);
	
protected:
	void UpdateControls();
	void SetCheckBoxesFromWelcomePageInfo();
	CDisplayModeInfo *m_pDisplayModes;
	int m_nDisplayModes;
	int m_nBestDisplayMode;

	// MOVE THIS STUFF TO THE ACC WIZ OPTIONS
	DEVMODE m_dvmdOrig;
	NONCLIENTMETRICS m_ncmOrig;
	LOGFONT m_lfIconOrig;


	BOOL m_bMagnifierRun;
	BOOL m_bResolutionSwitched;
	BOOL m_bFontsChanged;
	BOOL m_IntlVal;

	LRESULT OnInitDialog(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnCommand(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnPSN_SetActive(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);
	LRESULT OnPSN_WizNext(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);
	LRESULT OnPSN_WizBack(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh);
	
private:
	
};

#endif // _INC_PGMINTX2_H

