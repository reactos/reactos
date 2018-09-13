#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "pgFltKey.h"



/***************************************/

//
// Times are in milliseconds
//
#define BOUNCESIZE 5
UINT BounceTable[BOUNCESIZE] = {
    {  500 },
    {  700 },
    { 1000 },
    { 1500 },
    { 2000 }
};

/***************************************/

//
// Times are in milliseconds
//
#define ACCEPTSIZE 7
UINT AcceptTable[ACCEPTSIZE] = {
    {    0 },
    {  300 },
    {  500 },
    {  700 },
    { 1000 },
    { 1400 },
    { 2000 },
};




CFilterKeysPg::CFilterKeysPg(
	LPPROPSHEETPAGE ppsp
	) : WizardPage(ppsp, IDS_WIZFILTERKEYSTITLE, IDS_WIZFILTERKEYSSUBTITLE)
{
	m_dwPageId = IDD_KBDWIZFILTERKEYS;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
}


CFilterKeysPg::~CFilterKeysPg(
	VOID
	)
{
}

LRESULT
CFilterKeysPg::OnInitDialog(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{
	Button_SetCheck(GetDlgItem(m_hwnd, IDC_FK_ENABLE), g_Options.m_schemePreview.m_FILTERKEYS.dwFlags & FKF_FILTERKEYSON);
	Button_SetCheck(GetDlgItem(m_hwnd, IDC_FK_SOUND), g_Options.m_schemePreview.m_FILTERKEYS.dwFlags & FKF_CLICKON);

	if(g_Options.m_schemePreview.m_FILTERKEYS.iBounceMSec)
		Button_SetCheck(GetDlgItem(m_hwnd, IDC_FK_BOUNCE), TRUE);
	else
		Button_SetCheck(GetDlgItem(m_hwnd, IDC_FK_REPEAT), TRUE);



	// Set slider for bounce rate
	SendDlgItemMessage(m_hwnd,IDC_BK_BOUNCERATE, TBM_SETRANGE,
						 TRUE,MAKELONG(1,BOUNCESIZE));

	// Set slider for accept rate
	SendDlgItemMessage(m_hwnd,IDC_RK_ACCEPTRATE, TBM_SETRANGE,
						 TRUE,MAKELONG(1,ACCEPTSIZE));

	// Figure out initial settings
	// Make sure initial slider settings is not SMALLER than current setting
	int nIndex = 0;
	for(int i=BOUNCESIZE - 1;i>=0;i--)
	{
		if(BounceTable[i] >= g_Options.m_schemePreview.m_FILTERKEYS.iBounceMSec)
			nIndex = i;
		else
			break;
	}
	SendDlgItemMessage(m_hwnd,IDC_BK_BOUNCERATE, TBM_SETPOS, TRUE, nIndex+1);

	for(i=ACCEPTSIZE - 1;i>=0;i--)
	{
		if(AcceptTable[i] >= g_Options.m_schemePreview.m_FILTERKEYS.iWaitMSec)
			nIndex = i;
		else
			break;
	}
	SendDlgItemMessage(m_hwnd,IDC_RK_ACCEPTRATE, TBM_SETPOS, TRUE, nIndex+1);

	
	UpdateControls();
	UpdateSliders();
	return 1;
}


void CFilterKeysPg::UpdateControls()
{
	BOOL bUseFilterKeys = Button_GetCheck(GetDlgItem(m_hwnd, IDC_FK_ENABLE));
	BOOL bUseBounce = Button_GetCheck(GetDlgItem(m_hwnd, IDC_FK_BOUNCE));

	// Bounce Keys
	EnableWindow(GetDlgItem(m_hwnd, IDC_FK_BOUNCE), bUseFilterKeys);
	EnableWindow(GetDlgItem(m_hwnd, IDC_BK_TIME), bUseFilterKeys && bUseBounce);
	EnableWindow(GetDlgItem(m_hwnd, IDC_BK_TIME_LBL2), bUseFilterKeys && bUseBounce);
	EnableWindow(GetDlgItem(m_hwnd, IDC_BK_TIME_LBL_SHORT), bUseFilterKeys && bUseBounce);
	EnableWindow(GetDlgItem(m_hwnd, IDC_BK_BOUNCERATE), bUseFilterKeys && bUseBounce);
	EnableWindow(GetDlgItem(m_hwnd, IDC_BK_TIME_LBL_LONG), bUseFilterKeys && bUseBounce);

	// Repeat Keys
	EnableWindow(GetDlgItem(m_hwnd, IDC_FK_REPEAT), bUseFilterKeys);
	EnableWindow(GetDlgItem(m_hwnd, IDC_RK_WAITTIME), bUseFilterKeys && !bUseBounce);
	EnableWindow(GetDlgItem(m_hwnd, IDC_RK_WAITTIME_LBL2), bUseFilterKeys && !bUseBounce);
	EnableWindow(GetDlgItem(m_hwnd, IDC_RK_WAITTIME_LBL_SHORT), bUseFilterKeys && !bUseBounce);
	EnableWindow(GetDlgItem(m_hwnd, IDC_RK_ACCEPTRATE), bUseFilterKeys && !bUseBounce);
	EnableWindow(GetDlgItem(m_hwnd, IDC_RK_WAITTIME_LBL_LONG), bUseFilterKeys && !bUseBounce);

	// Other Controls
	EnableWindow(GetDlgItem(m_hwnd, IDC_FK_SOUND), bUseFilterKeys);
	EnableWindow(GetDlgItem(m_hwnd, IDC_FK_LBL_TESTBOX), bUseFilterKeys);
	EnableWindow(GetDlgItem(m_hwnd, IDC_FK_TESTBOX), bUseFilterKeys);

}


LRESULT
CFilterKeysPg::OnCommand(
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
	case IDC_FK_ENABLE:
	case IDC_FK_BOUNCE:
	case IDC_FK_REPEAT:
		// These commands require us to re-enable/disable the appropriate controls
		UpdateControls();
		lResult = 0;
		break;

	default:
		break;
	}

	return lResult;
}

void
CFilterKeysPg::UpdateSliders()
{
	int nBounceRate = SendDlgItemMessage(m_hwnd,IDC_BK_BOUNCERATE, TBM_GETPOS, 0,0);
	int nAcceptRate = SendDlgItemMessage(m_hwnd,IDC_RK_ACCEPTRATE, TBM_GETPOS, 0,0);
	if(nBounceRate < 1 || nBounceRate > BOUNCESIZE)
		nBounceRate = 1;
	if(nAcceptRate < 1 || nAcceptRate > ACCEPTSIZE)
		nAcceptRate = 1;

	// Look up in table
	nBounceRate = BounceTable[nBounceRate - 1];
	nAcceptRate = AcceptTable[nAcceptRate - 1];

	TCHAR buf[10], buf2[10];
	wsprintf(buf,__TEXT("%d.%d"),nBounceRate/1000,	(nBounceRate%1000)/100);
	GetNumberFormat(LOCALE_USER_DEFAULT,0,buf,NULL,buf2,6);
	SetDlgItemText(m_hwnd, IDC_BK_TIME, buf2);

	wsprintf(buf,__TEXT("%d.%d"),nAcceptRate/1000,	(nAcceptRate%1000)/100);
	GetNumberFormat(LOCALE_USER_DEFAULT,0,buf,NULL,buf2,6);
	SetDlgItemText(m_hwnd, IDC_RK_WAITTIME, buf2);
}

LRESULT
CFilterKeysPg::HandleMsg(
	HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam
	)
{
	switch(uMsg)
	{
	case WM_HSCROLL:
		UpdateSliders();
		break;
	default:
		break;
	}
	return 0;
}

LRESULT
CFilterKeysPg::OnPSN_WizNext(
						   HWND hwnd,
						   INT idCtl,
						   LPPSHNOTIFY pnmh
						   )
{
	BOOL bUseFilterKeys = Button_GetCheck(GetDlgItem(m_hwnd, IDC_FK_ENABLE));
	

	if(bUseFilterKeys)
	{
		g_Options.m_schemePreview.m_FILTERKEYS.dwFlags |= FKF_FILTERKEYSON;
		BOOL bClick = Button_GetCheck(GetDlgItem(m_hwnd, IDC_FK_SOUND));

		if(bClick)
			g_Options.m_schemePreview.m_FILTERKEYS.dwFlags |= FKF_CLICKON;
		else
			g_Options.m_schemePreview.m_FILTERKEYS.dwFlags &= ~FKF_CLICKON;

		if(Button_GetCheck(GetDlgItem(m_hwnd, IDC_FK_BOUNCE)))
		{
			// Bounce Keys
			int nIndex = SendDlgItemMessage(m_hwnd, IDC_BK_BOUNCERATE, TBM_GETPOS, 0, 0);
			g_Options.m_schemePreview.m_FILTERKEYS.iWaitMSec = 0;
			g_Options.m_schemePreview.m_FILTERKEYS.iDelayMSec = 0;
			g_Options.m_schemePreview.m_FILTERKEYS.iRepeatMSec = 0;
			g_Options.m_schemePreview.m_FILTERKEYS.iBounceMSec = BounceTable[nIndex - 1];
		}
		else
		{
			// Repeat Keys
			int nIndex = SendDlgItemMessage(m_hwnd, IDC_RK_ACCEPTRATE, TBM_GETPOS, 0, 0);
#pragma message("HACK - Don't Hard Code Delay and Repeat rate")
			g_Options.m_schemePreview.m_FILTERKEYS.iWaitMSec = AcceptTable[nIndex - 1];
			g_Options.m_schemePreview.m_FILTERKEYS.iDelayMSec = 1000;
			g_Options.m_schemePreview.m_FILTERKEYS.iRepeatMSec = 500;
			g_Options.m_schemePreview.m_FILTERKEYS.iBounceMSec = 0;
		}


	}
	else
		g_Options.m_schemePreview.m_FILTERKEYS.dwFlags &= ~FKF_FILTERKEYSON;

	g_Options.ApplyPreview();

	return WizardPage::OnPSN_WizNext(hwnd, idCtl, pnmh);
}

