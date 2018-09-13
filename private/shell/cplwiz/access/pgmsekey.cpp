#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "pgMseKey.h"

CMouseKeysPg::CMouseKeysPg(
	LPPROPSHEETPAGE ppsp
	) : WizardPage(ppsp, IDS_MSEWIZMOUSEKEYSTITLE, IDS_MSEWIZMOUSEKEYSSUBTITLE)
{
	m_dwPageId = IDD_MSEWIZMOUSEKEYS;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
}


CMouseKeysPg::~CMouseKeysPg(
	VOID
	)
{
}

static UINT g_nSpeedTable[] = { 10, 20, 30, 40, 60, 80, 120, 180, 360 };
static UINT g_nAccelTable[] = { 5000, 4500, 4000, 3500, 3000, 2500, 2000, 1500, 1000 };


LRESULT
CMouseKeysPg::OnInitDialog(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{
	Button_SetCheck(GetDlgItem(m_hwnd, IDC_MK_ENABLE), g_Options.m_schemePreview.m_MOUSEKEYS.dwFlags & MKF_MOUSEKEYSON);
	Button_SetCheck(GetDlgItem(m_hwnd, IDC_MK_USEMODKEYS), g_Options.m_schemePreview.m_MOUSEKEYS.dwFlags & MKF_MODIFIERS);


	if(g_Options.m_schemePreview.m_MOUSEKEYS.dwFlags & MKF_REPLACENUMBERS)
		Button_SetCheck(GetDlgItem(m_hwnd, IDC_MK_NLON), TRUE);
	else
		Button_SetCheck(GetDlgItem(m_hwnd, IDC_MK_NLOFF), TRUE);

	SendDlgItemMessage(m_hwnd,IDC_MK_TOPSPEED, TBM_SETRANGE, TRUE,MAKELONG(0,8));
	SendDlgItemMessage(m_hwnd,IDC_MK_ACCEL, TBM_SETRANGE, TRUE,MAKELONG(0,8));

	int nIndex = 0;
	for(int i=8;i>=0;i--)
	{
		if(g_nSpeedTable[i] >= g_Options.m_schemePreview.m_MOUSEKEYS.iMaxSpeed)
			nIndex = i;
		else
			break;
	}
	SendDlgItemMessage(m_hwnd,IDC_MK_TOPSPEED, TBM_SETPOS, TRUE, nIndex);

	for(i=8;i>=0;i--)
	{
		if(g_nAccelTable[i] <= g_Options.m_schemePreview.m_MOUSEKEYS.iTimeToMaxSpeed)
			nIndex = i;
		else
			break;
	}
	SendDlgItemMessage(m_hwnd,IDC_MK_ACCEL, TBM_SETPOS, TRUE, nIndex);

	UpdateControls();
	return 1;
}


void CMouseKeysPg::UpdateControls()
{
	BOOL bUseMouseKeys = Button_GetCheck(GetDlgItem(m_hwnd, IDC_MK_ENABLE));

	EnableWindow(GetDlgItem(m_hwnd, IDC_MK_LBL1), bUseMouseKeys);
	EnableWindow(GetDlgItem(m_hwnd, IDC_MK_LBL2), bUseMouseKeys);
	EnableWindow(GetDlgItem(m_hwnd, IDC_MK_LBL3), bUseMouseKeys);
	EnableWindow(GetDlgItem(m_hwnd, IDC_MK_LBL4), bUseMouseKeys);
	EnableWindow(GetDlgItem(m_hwnd, IDC_MK_LBL5), bUseMouseKeys);
	EnableWindow(GetDlgItem(m_hwnd, IDC_MK_LBL6), bUseMouseKeys);
	EnableWindow(GetDlgItem(m_hwnd, IDC_MK_LBL7), bUseMouseKeys);

	EnableWindow(GetDlgItem(m_hwnd, IDC_MK_TOPSPEED), bUseMouseKeys);
	EnableWindow(GetDlgItem(m_hwnd, IDC_MK_ACCEL), bUseMouseKeys);
	EnableWindow(GetDlgItem(m_hwnd, IDC_MK_USEMODKEYS), bUseMouseKeys);
	EnableWindow(GetDlgItem(m_hwnd, IDC_MK_NLOFF), bUseMouseKeys);
	EnableWindow(GetDlgItem(m_hwnd, IDC_MK_NLON), bUseMouseKeys);

}


LRESULT
CMouseKeysPg::OnCommand(
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
	case IDC_MK_ENABLE:
		// These commands require us to re-enable/disable the appropriate controls
		UpdateControls();
		lResult = 0;
		break;

	default:
		break;
	}

	return lResult;
}

LRESULT
CMouseKeysPg::OnPSN_WizNext(
						   HWND hwnd,
						   INT idCtl,
						   LPPSHNOTIFY pnmh
						   )
{
	BOOL bUseMouseKeys = Button_GetCheck(GetDlgItem(m_hwnd, IDC_MK_ENABLE));

	if(bUseMouseKeys)
	{
		g_Options.m_schemePreview.m_MOUSEKEYS.dwFlags |= MKF_MOUSEKEYSON;

		if(Button_GetCheck(GetDlgItem(m_hwnd, IDC_MK_USEMODKEYS)))
			g_Options.m_schemePreview.m_MOUSEKEYS.dwFlags |= MKF_MODIFIERS;
		else
			g_Options.m_schemePreview.m_MOUSEKEYS.dwFlags &= ~MKF_MODIFIERS;

		if(Button_GetCheck(GetDlgItem(m_hwnd, IDC_MK_NLON)))
			g_Options.m_schemePreview.m_MOUSEKEYS.dwFlags |= MKF_REPLACENUMBERS;
		else
			g_Options.m_schemePreview.m_MOUSEKEYS.dwFlags &= ~MKF_REPLACENUMBERS;

		int nIndex;
		nIndex = SendDlgItemMessage(m_hwnd, IDC_MK_TOPSPEED, TBM_GETPOS, 0, 0);
		g_Options.m_schemePreview.m_MOUSEKEYS.iMaxSpeed = g_nSpeedTable[nIndex];
		nIndex = SendDlgItemMessage(m_hwnd, IDC_MK_ACCEL, TBM_GETPOS, 0, 0);
		g_Options.m_schemePreview.m_MOUSEKEYS.iTimeToMaxSpeed = g_nAccelTable[nIndex];

#pragma message("Handle THis!")
			// 3/15/95 -
			// Always init the control speed to 1/8 of the screen width/
//			g_mk.iCtrlSpeed = GetSystemMetrics(SM_CXSCREEN) / 16;

	}
	else
		g_Options.m_schemePreview.m_MOUSEKEYS.dwFlags &= ~MKF_MOUSEKEYSON;

	g_Options.ApplyPreview();

	return WizardPage::OnPSN_WizNext(hwnd, idCtl, pnmh);
}
