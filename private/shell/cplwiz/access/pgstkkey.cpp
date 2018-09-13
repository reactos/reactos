#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "pgStkKey.h"

CStickyKeysPg::CStickyKeysPg(
	LPPROPSHEETPAGE ppsp
	) : WizardPage(ppsp, IDS_WIZSTICKYKEYSTITLE, IDS_WIZSTICKYKEYSSUBTITLE)
{
	m_dwPageId = IDD_KBDWIZSTICKYKEYS;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
}


CStickyKeysPg::~CStickyKeysPg(
	VOID
	)
{
}

LRESULT
CStickyKeysPg::OnInitDialog(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{
	Button_SetCheck(GetDlgItem(m_hwnd, IDC_STK_ENABLE), g_Options.m_schemePreview.m_STICKYKEYS.dwFlags & SKF_STICKYKEYSON);
	Button_SetCheck(GetDlgItem(m_hwnd, IDC_STK_LOCK), g_Options.m_schemePreview.m_STICKYKEYS.dwFlags & SKF_TRISTATE);
	Button_SetCheck(GetDlgItem(m_hwnd, IDC_STK_2KEYS), g_Options.m_schemePreview.m_STICKYKEYS.dwFlags & SKF_TWOKEYSOFF);
	Button_SetCheck(GetDlgItem(m_hwnd, IDC_STK_SOUNDMOD), g_Options.m_schemePreview.m_STICKYKEYS.dwFlags & SKF_AUDIBLEFEEDBACK);

	UpdateControls();
	return 1;
}


void CStickyKeysPg::UpdateControls()
{
	BOOL bUseStickyKeys = Button_GetCheck(GetDlgItem(m_hwnd, IDC_STK_ENABLE));

	EnableWindow(GetDlgItem(m_hwnd, IDC_STK_LOCK), bUseStickyKeys);
	EnableWindow(GetDlgItem(m_hwnd, IDC_STK_2KEYS), bUseStickyKeys);
	EnableWindow(GetDlgItem(m_hwnd, IDC_STK_SOUNDMOD), bUseStickyKeys);

}


LRESULT
CStickyKeysPg::OnCommand(
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
	case IDC_STK_ENABLE:
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
CStickyKeysPg::OnPSN_WizNext(
						   HWND hwnd,
						   INT idCtl,
						   LPPSHNOTIFY pnmh
						   )
{
	;

	if(Button_GetCheck(GetDlgItem(m_hwnd, IDC_STK_ENABLE)))
	{
		g_Options.m_schemePreview.m_STICKYKEYS.dwFlags |= SKF_STICKYKEYSON;
		// Set options if we are turning the feature on

		// Clear option flags
		g_Options.m_schemePreview.m_STICKYKEYS.dwFlags &= ~(SKF_TRISTATE | SKF_TWOKEYSOFF | SKF_AUDIBLEFEEDBACK);

		// Turn selected flags on
		if(Button_GetCheck(GetDlgItem(m_hwnd, IDC_STK_LOCK)))
			g_Options.m_schemePreview.m_STICKYKEYS.dwFlags |= SKF_TRISTATE;
		if(Button_GetCheck(GetDlgItem(m_hwnd, IDC_STK_2KEYS)))
			g_Options.m_schemePreview.m_STICKYKEYS.dwFlags |= SKF_TWOKEYSOFF;
		if(Button_GetCheck(GetDlgItem(m_hwnd, IDC_STK_SOUNDMOD)))
			g_Options.m_schemePreview.m_STICKYKEYS.dwFlags |= SKF_AUDIBLEFEEDBACK;
	}
	else
		g_Options.m_schemePreview.m_STICKYKEYS.dwFlags &= ~SKF_STICKYKEYSON;

	g_Options.ApplyPreview();

	return WizardPage::OnPSN_WizNext(hwnd, idCtl, pnmh);
}
