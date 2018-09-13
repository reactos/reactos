#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "pgWizWiz.h"

CWizWizPg::CWizWizPg(
	LPPROPSHEETPAGE ppsp
	) : WizardPage(ppsp, IDS_GENERICPAGETITLE, IDS_GENERICPAGESUBTITLE)
{
	m_dwPageId = IDD_WIZWIZ;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
}


CWizWizPg::~CWizWizPg(
	VOID
	)
{
}

LRESULT
CWizWizPg::OnInitDialog(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{
	ComboBox_AddString(GetDlgItem(m_hwnd, IDC_CMINTEXT), __TEXT("Type 1 - Up/Down"));
	ComboBox_AddString(GetDlgItem(m_hwnd, IDC_CMINTEXT), __TEXT("Type 2 - Slider"));
	ComboBox_AddString(GetDlgItem(m_hwnd, IDC_CMINTEXT), __TEXT("Type 3 - List Box"));
	ComboBox_AddString(GetDlgItem(m_hwnd, IDC_CMINTEXT), __TEXT("Type 4 - Old Style (3 lines of text)"));
	ComboBox_SetCurSel(GetDlgItem(m_hwnd, IDC_CMINTEXT), g_Options.m_nTypeMinText);

	ComboBox_AddString(GetDlgItem(m_hwnd, IDC_CSCROLLBAR), __TEXT("Type 1 - Up/Down"));
	ComboBox_AddString(GetDlgItem(m_hwnd, IDC_CSCROLLBAR), __TEXT("Type 2 - Slider"));
	ComboBox_AddString(GetDlgItem(m_hwnd, IDC_CSCROLLBAR), __TEXT("Type 3 - Old Style (samples with Big Blue Rectangle)"));
	ComboBox_SetCurSel(GetDlgItem(m_hwnd, IDC_CSCROLLBAR), g_Options.m_nTypeScrollBar);

	ComboBox_AddString(GetDlgItem(m_hwnd, IDC_CBORDER), __TEXT("Type 1 - Up/Down"));
	ComboBox_AddString(GetDlgItem(m_hwnd, IDC_CBORDER), __TEXT("Type 2 - Slider"));
	ComboBox_SetCurSel(GetDlgItem(m_hwnd, IDC_CBORDER), g_Options.m_nTypeBorder);

	ComboBox_AddString(GetDlgItem(m_hwnd, IDC_CACCTIMEOUT), __TEXT("Type 1 - Check Box"));
	ComboBox_AddString(GetDlgItem(m_hwnd, IDC_CACCTIMEOUT), __TEXT("Type 2 - Radio Button"));
	ComboBox_SetCurSel(GetDlgItem(m_hwnd, IDC_CACCTIMEOUT), g_Options.m_nTypeAccTimeOut);


	UpdateControls();
	return 1;
}


void CWizWizPg::UpdateControls()
{
}


LRESULT
CWizWizPg::OnCommand(
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

LRESULT CWizWizPg::OnPSN_KillActive(
						 HWND hwnd,
						 INT idCtl,
						 LPPSHNOTIFY pnmh
						 )
{
	g_Options.m_nTypeMinText = ComboBox_GetCurSel(GetDlgItem(m_hwnd, IDC_CMINTEXT));
	g_Options.m_nTypeScrollBar = ComboBox_GetCurSel(GetDlgItem(m_hwnd, IDC_CSCROLLBAR));
	g_Options.m_nTypeBorder = ComboBox_GetCurSel(GetDlgItem(m_hwnd, IDC_CBORDER));
	g_Options.m_nTypeAccTimeOut = ComboBox_GetCurSel(GetDlgItem(m_hwnd, IDC_CACCTIMEOUT));

	return TRUE;
}