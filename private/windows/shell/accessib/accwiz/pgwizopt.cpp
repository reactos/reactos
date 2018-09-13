#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "pgWizOpt.h"

BOOL IsAdmin();

CWizardOptionsPg::CWizardOptionsPg( 
    LPPROPSHEETPAGE ppsp
	) : WizardPage(ppsp, IDS_WIZOPTTITLE, IDS_WIZOPTSUBTITLE)
{
	m_dwPageId = IDD_WIZOPTIONS;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
    restoreOpt = FALSE;
}


CWizardOptionsPg::~CWizardOptionsPg(
    VOID
    )
{
}


DWORD g_rgdwWizNoOptionsSelected[] = {IDD_WIZNOOPTIONSSELECTED};

DWORD g_rgdwWizDoBlind[] = {
							IDD_FNTWIZSCROLLBAR, // scroll bar
							IDD_PREV_ICON2,
							IDD_PREV_COLOR,
                            IDD_MSEWIZMOUSECURSOR
//							IDD_CLRWIZHIGHCONTRAST
								};
DWORD g_rgdwWizDoSounds[] = {
							IDD_SNDWIZSENTRYSHOWSOUNDS,
							IDD_SNDWIZSENTRYSHOWSOUNDS2,
								};
DWORD g_rgdwWizDoKeyboardMouse[] = {
							IDD_KBDWIZSTICKYKEYS,
//							IDD_KBDWIZFILTERKEYS,
							IDD_KBDWIZFILTERKEYS1,
							IDD_KBDWIZFILTERKEYS2,
							IDD_KBDWIZTOGGLEKEYS,
							IDD_KBDWIZSHOWEXTRAKEYBOARDHELP,
//							IDD_KBDWIZSERIALKEYS
							IDD_MSEWIZMOUSEKEYS,
							IDD_MSEWIZMOUSECURSOR,
							IDD_MSEWIZBUTTONCONFIG,
							IDD_MSEWIZMOUSESPEED
//							IDD_MSEWIZMOUSETRAILS // Note not supported in NT5.0
								};


DWORD g_rgdwWizDoAdmin[] = {
//						IDD_WIZHOTKEYANDNOTIFICATION,
						IDD_WIZACCESSTIMEOUT,
						IDD_WIZWORKSTATIONDEFAULT,
						IDD_WIZSAVETOFILE
//						IDD_WIZLAUNCHMAGNIFY,
							};
DWORD g_rgdwWizFinalPages[] = {
						IDD_WIZSAVETOFILE,
						IDD_WIZFINISH
							};


BOOL CWizardOptionsPg::AdjustWizPageOrder()
{
	BOOL bDoBlind = Button_GetCheck(GetDlgItem(m_hwnd, IDC_DOBLIND));
	BOOL bDoSounds = Button_GetCheck(GetDlgItem(m_hwnd, IDC_DOSOUND));
	BOOL bDoKeyboardMouse = Button_GetCheck(GetDlgItem(m_hwnd, IDC_DOKEYBOARDMOUSE));
	BOOL bDoAdmin = Button_GetCheck(GetDlgItem(m_hwnd, IDC_DOADMIN));

	BOOL bDoNoOptions = (!bDoBlind && !bDoSounds && !bDoKeyboardMouse && !bDoAdmin && !restoreOpt);

	// First remove all possible pages since we want to insert them in the correct order
	// Return value does not matter since the pages may not be in the array
	sm_WizPageOrder.RemovePages(g_rgdwWizNoOptionsSelected, ARRAYSIZE(g_rgdwWizNoOptionsSelected));
	sm_WizPageOrder.RemovePages(g_rgdwWizDoBlind, ARRAYSIZE(g_rgdwWizDoBlind));
	sm_WizPageOrder.RemovePages(g_rgdwWizDoSounds, ARRAYSIZE(g_rgdwWizDoSounds));
	sm_WizPageOrder.RemovePages(g_rgdwWizDoKeyboardMouse, ARRAYSIZE(g_rgdwWizDoKeyboardMouse));
	sm_WizPageOrder.RemovePages(g_rgdwWizDoAdmin, ARRAYSIZE(g_rgdwWizDoAdmin));
	sm_WizPageOrder.RemovePages(g_rgdwWizFinalPages, ARRAYSIZE(g_rgdwWizFinalPages));

	// Then Add in pages in groups in the reverse order that we want them to appear.
	// We do them this way since they are inserted after this page, so the first group inserted
	// will be the last group at the end of this.

	// NOTE: We do not care about the return value from AddPages() in the sense
	// that we they do not allocate or free memory so it does not hurt to keep calling them.  We
	// Will propogate a return value of FALSE if any of them fail.

	BOOL bSuccess = TRUE;

	// Add Final Pages
	bSuccess = bSuccess && sm_WizPageOrder.AddPages(m_dwPageId, g_rgdwWizFinalPages, ARRAYSIZE(g_rgdwWizFinalPages));

	if(bDoNoOptions)
		bSuccess = bSuccess && sm_WizPageOrder.AddPages(m_dwPageId, g_rgdwWizNoOptionsSelected, ARRAYSIZE(g_rgdwWizNoOptionsSelected));

	if(bDoAdmin)
    {
		bSuccess = bSuccess && sm_WizPageOrder.AddPages(m_dwPageId, g_rgdwWizDoAdmin, ARRAYSIZE(g_rgdwWizDoAdmin));
        // Incase you are NOT an admin Remove admin page
        if ( !IsAdmin() )
	        sm_WizPageOrder.RemovePages(&g_rgdwWizDoAdmin[1], 1);
    }

	if(bDoKeyboardMouse)
		bSuccess = bSuccess && sm_WizPageOrder.AddPages(m_dwPageId, g_rgdwWizDoKeyboardMouse, ARRAYSIZE(g_rgdwWizDoKeyboardMouse));

	if(bDoSounds)
		bSuccess = bSuccess && sm_WizPageOrder.AddPages(m_dwPageId, g_rgdwWizDoSounds, ARRAYSIZE(g_rgdwWizDoSounds));

	if(bDoBlind)
		bSuccess = bSuccess && sm_WizPageOrder.AddPages(m_dwPageId, g_rgdwWizDoBlind, ARRAYSIZE(g_rgdwWizDoBlind));

	return bSuccess;
}


LRESULT
CWizardOptionsPg::OnCommand(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{
	LRESULT lResult = 1;

	WORD wNotifyCode = HIWORD(wParam);
	WORD wCtlID      = LOWORD(wParam);
	HWND hwndCtl     = (HWND)lParam;

	switch(wCtlID)
	{
	case IDC_BTNRESTORETODEFAULT:
		g_Options.ApplyWindowsDefault();
        restoreOpt = TRUE;
		break;

	default:
		break;
	}

	return lResult;
}

BOOL IsAdmin()
{
    HKEY hkey;
    BOOL fOk = (ERROR_SUCCESS == RegOpenKeyExA( HKEY_USERS, ".Default", 0, KEY_ALL_ACCESS, &hkey ));

    if(fOk)
    {
        RegCloseKey(hkey);
    }
    
	return fOk;
}

