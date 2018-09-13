#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "pgShwHlp.h"

CShowKeyboardHelpPg::CShowKeyboardHelpPg(
	LPPROPSHEETPAGE ppsp
	) : WizardPage(ppsp, IDS_WIZSHOWEXTRAKEYBOARDHELPTITLE, IDS_WIZSHOWEXTRAKEYBOARDHELPSUBTITLE)
{
	m_dwPageId = IDD_KBDWIZSHOWEXTRAKEYBOARDHELP;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
}


CShowKeyboardHelpPg::~CShowKeyboardHelpPg(
	VOID
	)
{
}

LRESULT
CShowKeyboardHelpPg::OnInitDialog(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{
	Button_SetCheck(GetDlgItem(m_hwnd, IDC_SHOWEXTRAKEYBOARDHELP_ENABLE), g_Options.m_schemePreview.m_bShowExtraKeyboardHelp);
	UpdateControls();
	return 1;
}


void CShowKeyboardHelpPg::UpdateControls()
{
	// No options for show extra keyboard help
}


LRESULT
CShowKeyboardHelpPg::OnCommand(
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
	case IDC_SHOWEXTRAKEYBOARDHELP_ENABLE:
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
CShowKeyboardHelpPg::OnPSN_WizNext(
						   HWND hwnd,
						   INT idCtl,
						   LPPSHNOTIFY pnmh
						   )
{
	g_Options.m_schemePreview.m_bShowExtraKeyboardHelp = Button_GetCheck(GetDlgItem(m_hwnd, IDC_SHOWEXTRAKEYBOARDHELP_ENABLE));
	g_Options.ApplyPreview();

	return WizardPage::OnPSN_WizNext(hwnd, idCtl, pnmh);
}
