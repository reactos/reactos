#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "pgWizOpt.h"


CWizardOptionsPg::CWizardOptionsPg( 
    LPPROPSHEETPAGE ppsp
	) : WizardPage(ppsp, 0, 0)
{
	m_dwPageId = IDD_WIZOPTIONS;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
}


CWizardOptionsPg::~CWizardOptionsPg(
    VOID
    )
{
}

DWORD g_rgdwWizNoOptionsSelected[] = {IDD_WIZNOOPTIONSSELECTED};

DWORD g_rgdwWizDoFonts[] = {
							0, // min text size
/*							IDD_PREV_MINTEXT1,
							IDD_PREV_MINTEXT2,
							IDD_PREV_MINTEXT3,
							IDD_FNTWIZMINTEXT,
*/
							IDD_FNTWIZMINTEXT2,
							0, // scroll bar
/*
							IDD_PREV_SCROLL1,
							IDD_PREV_SCROLL2,
							IDD_FNTWIZSCROLLBAR,
*/
							0, // border
/*
							IDD_PREV_BORDER1,
							IDD_PREV_BORDER2,
*/
							IDD_PREV_ICON1
								};
DWORD g_rgdwWizDoColors[] = {
							IDD_PREV_COLOR,
//							IDD_CLRWIZHIGHCONTRAST
								};
DWORD g_rgdwWizDoSounds[] = {
							IDD_SNDWIZSENTRYSHOWSOUNDS,
								};
DWORD g_rgdwWizDoKeyboard[] = {
							IDD_KBDWIZSTICKYKEYS,
							IDD_KBDWIZFILTERKEYS,
							IDD_KBDWIZTOGGLEKEYS,
							IDD_KBDWIZSHOWEXTRAKEYBOARDHELP,
//							IDD_KBDWIZSERIALKEYS
								};
DWORD g_rgdwWizDoMouse[] = {
							IDD_MSEWIZMOUSEKEYS,
							IDD_MSEWIZMOUSECURSOR,
							IDD_MSEWIZBUTTONCONFIG
								};

DWORD g_rgdwWizFinalPages[] = {
						IDD_WIZHOTKEYANDNOTIFICATION,
						0,
/* // For usability tests
						IDD_WIZACCESSTIMEOUT1,
						IDD_WIZACCESSTIMEOUT2,
*/
						IDD_WIZWORKSTATIONDEFAULT,
						IDD_WIZSAVETOFILE,
						IDD_WIZFINISH
							};


BOOL CWizardOptionsPg::AdjustWizPageOrder()
{
	// HACK TO DYNAMICALLY CHANGE WHICH PAGES ARE USED BY THE WIZARD
	switch(g_Options.m_nTypeMinText)
	{
	case 0: g_rgdwWizDoFonts[0] =  IDD_PREV_MINTEXT1;break;
	case 1: g_rgdwWizDoFonts[0] =  IDD_PREV_MINTEXT2;break;
	case 2: g_rgdwWizDoFonts[0] =  IDD_PREV_MINTEXT3;break;
	case 3: g_rgdwWizDoFonts[0] =  IDD_FNTWIZMINTEXT;break;
	}
	switch(g_Options.m_nTypeScrollBar)
	{
	case 0: g_rgdwWizDoFonts[2] =  IDD_PREV_SCROLL1;break;
	case 1: g_rgdwWizDoFonts[2] =  IDD_PREV_SCROLL2;break;
	case 2: g_rgdwWizDoFonts[2] =  IDD_FNTWIZSCROLLBAR;break;
	}
	switch(g_Options.m_nTypeBorder)
	{
	case 0: g_rgdwWizDoFonts[3] =  IDD_PREV_BORDER1;break;
	case 1: g_rgdwWizDoFonts[3] =  IDD_PREV_BORDER2;break;
	}
	switch(g_Options.m_nTypeAccTimeOut)
	{
	case 0: g_rgdwWizFinalPages[1] =  IDD_WIZACCESSTIMEOUT1;break;
	case 1: g_rgdwWizFinalPages[1] =  IDD_WIZACCESSTIMEOUT2;break;
	}



	BOOL bDoFonts = Button_GetCheck(GetDlgItem(m_hwnd, IDC_DOFONTS));
	BOOL bDoColors = Button_GetCheck(GetDlgItem(m_hwnd, IDC_DOCOLORS));
	BOOL bDoSounds = Button_GetCheck(GetDlgItem(m_hwnd, IDC_DOSOUND));
	BOOL bDoKeyboard = Button_GetCheck(GetDlgItem(m_hwnd, IDC_DOKEYBOARD));
	BOOL bDoMouse = Button_GetCheck(GetDlgItem(m_hwnd, IDC_DOMOUSE));

	BOOL bDoNoOptions = (!bDoColors && !bDoFonts && !bDoSounds && !bDoKeyboard && !bDoMouse);

	// First remove all possible pages since we want to insert them in the correct order
	// Return value does not matter since the pages may not be in the array
	sm_WizPageOrder.RemovePages(g_rgdwWizNoOptionsSelected, ARRAYSIZE(g_rgdwWizNoOptionsSelected));
	sm_WizPageOrder.RemovePages(g_rgdwWizDoFonts, ARRAYSIZE(g_rgdwWizDoFonts));
	sm_WizPageOrder.RemovePages(g_rgdwWizDoColors, ARRAYSIZE(g_rgdwWizDoColors));
	sm_WizPageOrder.RemovePages(g_rgdwWizDoSounds, ARRAYSIZE(g_rgdwWizDoSounds));
	sm_WizPageOrder.RemovePages(g_rgdwWizDoKeyboard, ARRAYSIZE(g_rgdwWizDoKeyboard));
	sm_WizPageOrder.RemovePages(g_rgdwWizDoMouse, ARRAYSIZE(g_rgdwWizDoMouse));
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

	if(bDoMouse)
		bSuccess = bSuccess && sm_WizPageOrder.AddPages(m_dwPageId, g_rgdwWizDoMouse, ARRAYSIZE(g_rgdwWizDoMouse));

	if(bDoKeyboard)
		bSuccess = bSuccess && sm_WizPageOrder.AddPages(m_dwPageId, g_rgdwWizDoKeyboard, ARRAYSIZE(g_rgdwWizDoKeyboard));

	// If keyboard was selected, but mouse was not selected, add the mousekeys page at the end of the keyboard page
	if(bDoKeyboard && !bDoMouse)
	{
		DWORD dw = IDD_MSEWIZMOUSEKEYS;
		bSuccess = bSuccess && sm_WizPageOrder.AddPages(g_rgdwWizDoKeyboard[ARRAYSIZE(g_rgdwWizDoKeyboard) - 1], &dw, 1);
	}

	if(bDoSounds)
		bSuccess = bSuccess && sm_WizPageOrder.AddPages(m_dwPageId, g_rgdwWizDoSounds, ARRAYSIZE(g_rgdwWizDoSounds));

	if(bDoColors & !bDoFonts) // Don't add colors here if we have fonts to add
		bSuccess = bSuccess && sm_WizPageOrder.AddPages(m_dwPageId, g_rgdwWizDoColors, ARRAYSIZE(g_rgdwWizDoColors));

	if(bDoFonts)
		bSuccess = bSuccess && sm_WizPageOrder.AddPages(m_dwPageId, g_rgdwWizDoFonts, ARRAYSIZE(g_rgdwWizDoFonts));

	if(bDoFonts && bDoColors) // Add the colors here if we also have fonts
		bSuccess = bSuccess && sm_WizPageOrder.AddPages(IDD_FNTWIZMINTEXT2, g_rgdwWizDoColors, ARRAYSIZE(g_rgdwWizDoColors));

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
		MessageBox(m_hwnd, __TEXT("Not Yet Implemented"), __TEXT("Error"), MB_OK);
//		g_Options.ApplyWindowsDefault();
		break;

	default:
		break;
	}

	return lResult;
}
