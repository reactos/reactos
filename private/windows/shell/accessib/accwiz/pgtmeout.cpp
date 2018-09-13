#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "pgTmeOut.h"

CAccessTimeOutPg::CAccessTimeOutPg(
	LPPROPSHEETPAGE ppsp
	) : WizardPage(ppsp, IDS_WIZACCESSTIMEOUTTITLE, IDS_WIZACCESSTIMEOUTSUBTITLE)
{
	m_dwPageId = IDD_WIZACCESSTIMEOUT;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
}


CAccessTimeOutPg::~CAccessTimeOutPg(
	VOID
	)
{
}

int g_nTimeOuts = 6;
DWORD g_rgdwTimeOuts[] = {5*60000, 10*60000, 15*60000, 20*60000, 25*60000, 30*60000};

LRESULT
CAccessTimeOutPg::OnInitDialog(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{
	HWND hwndTimeOut = GetDlgItem(m_hwnd, IDC_TO_TIMEOUTVAL);


	// JMC: TODO: Maybe move these into the string table

	// Set timeouts for 5 to 30 minutes
	int i;
	for (i= 0; i < g_nTimeOuts; i++)
	{
		TCHAR buf[256];
		wsprintf(buf,__TEXT("%d"),g_rgdwTimeOuts[i]/60000);
		ComboBox_InsertString(hwndTimeOut, i, buf);
	}



	BOOL bEnable = g_Options.m_schemePreview.m_ACCESSTIMEOUT.dwFlags & ATF_TIMEOUTON;
	if(bEnable)
	{
		Button_SetCheck(GetDlgItem(m_hwnd, IDC_TO_ENABLE), TRUE);
		EnableWindow (GetDlgItem(m_hwnd,IDC_TO_TIMEOUTVAL),TRUE);
	}
	else
	{
		// Hack for radio buttons
		if(GetDlgItem(m_hwnd, IDC_TO_DISABLE))
			Button_SetCheck(GetDlgItem(m_hwnd, IDC_TO_DISABLE), TRUE);
		EnableWindow (GetDlgItem(m_hwnd,IDC_TO_TIMEOUTVAL),FALSE);
	}

	// Figure out the time to use as default
	int nIndex = 0;
	for(i = g_nTimeOuts - 1;i>=0;i--)
	{
		// Brute Force find the largest value
		if(g_rgdwTimeOuts[i] >= g_Options.m_schemePreview.m_ACCESSTIMEOUT.iTimeOutMSec)
			nIndex = i;
		else
			break;
	}
	ComboBox_SetCurSel(hwndTimeOut, nIndex);

	return 1;
}


void CAccessTimeOutPg::UpdateControls()
{
	// enable/disable the combo box depending on which radio
	// button is selected
	if(Button_GetCheck(GetDlgItem(m_hwnd, IDC_TO_ENABLE)))
	{
		EnableWindow (GetDlgItem(m_hwnd,IDC_TO_TIMEOUTVAL),TRUE);
	}
	else
	{
		EnableWindow (GetDlgItem(m_hwnd,IDC_TO_TIMEOUTVAL),FALSE);
	}

}


LRESULT
CAccessTimeOutPg::OnCommand(
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
	case IDC_TO_DISABLE:
	case IDC_TO_ENABLE:
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
CAccessTimeOutPg::OnPSN_WizNext(
						   HWND hwnd,
						   INT idCtl,
						   LPPSHNOTIFY pnmh
						   )
{
	BOOL bUseAccessTimeOut= Button_GetCheck(GetDlgItem(m_hwnd, IDC_TO_ENABLE));

	if(bUseAccessTimeOut)
		g_Options.m_schemePreview.m_ACCESSTIMEOUT.dwFlags |= ATF_TIMEOUTON;
	else
		g_Options.m_schemePreview.m_ACCESSTIMEOUT.dwFlags &= ~ATF_TIMEOUTON;

	int nIndex = ComboBox_GetCurSel(GetDlgItem(m_hwnd, IDC_TO_TIMEOUTVAL));
	g_Options.m_schemePreview.m_ACCESSTIMEOUT.iTimeOutMSec = g_rgdwTimeOuts[nIndex];


	g_Options.ApplyPreview();

	return WizardPage::OnPSN_WizNext(hwnd, idCtl, pnmh);
}
