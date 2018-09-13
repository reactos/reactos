#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "pgTglKey.h"

CToggleKeysPg::CToggleKeysPg(
	LPPROPSHEETPAGE ppsp
	) : WizardPage(ppsp, IDS_WIZTOGGLEKEYSTITLE, IDS_WIZTOGGLEKEYSSUBTITLE)
{
	m_dwPageId = IDD_KBDWIZTOGGLEKEYS;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
}


CToggleKeysPg::~CToggleKeysPg(
	VOID
	)
{
}

LRESULT
CToggleKeysPg::OnInitDialog(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{
	Button_SetCheck(GetDlgItem(m_hwnd, IDC_TK_ENABLE), g_Options.m_schemePreview.m_TOGGLEKEYS.dwFlags & TKF_TOGGLEKEYSON);
	UpdateControls();
	return 1;
}


void CToggleKeysPg::UpdateControls()
{
	// No options for toggle keys

}


LRESULT
CToggleKeysPg::OnCommand(
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
CToggleKeysPg::OnPSN_WizNext(
						   HWND hwnd,
						   INT idCtl,
						   LPPSHNOTIFY pnmh
						   )
{
	BOOL bUseToggleKeys = Button_GetCheck(GetDlgItem(m_hwnd, IDC_TK_ENABLE));

	if(bUseToggleKeys)
		g_Options.m_schemePreview.m_TOGGLEKEYS.dwFlags |= TKF_TOGGLEKEYSON;
	else
		g_Options.m_schemePreview.m_TOGGLEKEYS.dwFlags &= ~TKF_TOGGLEKEYSON;

	g_Options.ApplyPreview();

	return WizardPage::OnPSN_WizNext(hwnd, idCtl, pnmh);
}
