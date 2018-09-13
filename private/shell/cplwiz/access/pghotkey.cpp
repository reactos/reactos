#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "pgHotKey.h"

CHotKeysPg::CHotKeysPg(
	LPPROPSHEETPAGE ppsp
	) : WizardPage(ppsp, IDS_WIZHOTKEYANDNOTIFICATIONTITLE, IDS_WIZHOTKEYANDNOTIFICATIONSUBTITLE)
{
	m_dwPageId = IDD_WIZHOTKEYANDNOTIFICATION;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
}


CHotKeysPg::~CHotKeysPg(
	VOID
	)
{
}

LRESULT
CHotKeysPg::OnInitDialog(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{
//	Button_SetCheck(GetDlgItem(m_hwnd, IDC_HOTKEY_STK), g_Options.m_schemePreview.m_STICKYKEYS.dwFlags & SKF_HOTKEYACTIVE);
//	Button_SetCheck(GetDlgItem(m_hwnd, IDC_HOTKEY_FK), g_Options.m_schemePreview.m_FILTERKEYS.dwFlags & FKF_HOTKEYACTIVE);
//	Button_SetCheck(GetDlgItem(m_hwnd, IDC_HOTKEY_TK), g_Options.m_schemePreview.m_TOGGLEKEYS.dwFlags & TKF_HOTKEYACTIVE);
//	Button_SetCheck(GetDlgItem(m_hwnd, IDC_HOTKEY_HC), g_Options.m_schemePreview.m_HIGHCONTRAST.dwFlags & HCF_HOTKEYACTIVE);
	Button_SetCheck(GetDlgItem(m_hwnd, IDC_HOTKEY_STK), TRUE);
	Button_SetCheck(GetDlgItem(m_hwnd, IDC_HOTKEY_FK), TRUE);
	Button_SetCheck(GetDlgItem(m_hwnd, IDC_HOTKEY_TK), TRUE);
	Button_SetCheck(GetDlgItem(m_hwnd, IDC_HOTKEY_HC), TRUE);

	UpdateControls();
	return 1;
}


void CHotKeysPg::UpdateControls()
{

}


LRESULT
CHotKeysPg::OnCommand(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{
	LRESULT lResult = 1;

	WORD wNotifyCode = HIWORD(wParam);
	WORD wCtlID      = LOWORD(wParam);
	HWND hwndCtl     = (HWND)lParam;

	return lResult;
}

LRESULT
CHotKeysPg::OnPSN_WizNext(
						   HWND hwnd,
						   INT idCtl,
						   LPPSHNOTIFY pnmh
						   )
{
	BOOL bUseToggleKeys = Button_GetCheck(GetDlgItem(m_hwnd, IDC_TK_ENABLE));


	if(Button_GetCheck(GetDlgItem(m_hwnd, IDC_HOTKEY_STK)))
		g_Options.m_schemePreview.m_STICKYKEYS.dwFlags |= SKF_HOTKEYACTIVE;
	else
		g_Options.m_schemePreview.m_STICKYKEYS.dwFlags &= ~SKF_HOTKEYACTIVE;

	if(Button_GetCheck(GetDlgItem(m_hwnd, IDC_HOTKEY_FK)))
		g_Options.m_schemePreview.m_FILTERKEYS.dwFlags |= FKF_HOTKEYACTIVE;
	else
		g_Options.m_schemePreview.m_FILTERKEYS.dwFlags &= ~FKF_HOTKEYACTIVE;

	if(Button_GetCheck(GetDlgItem(m_hwnd, IDC_HOTKEY_TK)))
		g_Options.m_schemePreview.m_TOGGLEKEYS.dwFlags |= TKF_HOTKEYACTIVE;
	else
		g_Options.m_schemePreview.m_TOGGLEKEYS.dwFlags &= ~TKF_HOTKEYACTIVE;

	if(Button_GetCheck(GetDlgItem(m_hwnd, IDC_HOTKEY_HC)))
		g_Options.m_schemePreview.m_HIGHCONTRAST.dwFlags |= HCF_HOTKEYACTIVE;
	else
		g_Options.m_schemePreview.m_HIGHCONTRAST.dwFlags &= ~HCF_HOTKEYACTIVE;

#pragma message("TODO: FORCE Notifications on")

	g_Options.ApplyPreview();

	return WizardPage::OnPSN_WizNext(hwnd, idCtl, pnmh);
}
