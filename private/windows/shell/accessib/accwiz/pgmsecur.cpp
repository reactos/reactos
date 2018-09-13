#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "pgMseCur.h"


CMouseCursorPg::CMouseCursorPg(
	LPPROPSHEETPAGE ppsp
	) : WizardPage(ppsp, IDS_MSEWIZMOUSECURSORTITLE, IDS_MSEWIZMOUSECURSORSUBTITLE)
{
	m_dwPageId = IDD_MSEWIZMOUSECURSOR;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
}


CMouseCursorPg::~CMouseCursorPg(
	VOID
	)
{
}

LRESULT
CMouseCursorPg::OnInitDialog(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{
	HICON hIconSmallWhite = LoadIcon(g_hInstDll, MAKEINTRESOURCE(IDI_CURSOR_SMALL_WHITE));
	HICON hIconMediumWhite = LoadIcon(g_hInstDll, MAKEINTRESOURCE(IDI_CURSOR_MEDIUM_WHITE));
	HICON hIconLargeWhite = LoadIcon(g_hInstDll, MAKEINTRESOURCE(IDI_CURSOR_LARGE_WHITE));

	HICON hIconSmallBlack = LoadIcon(g_hInstDll, MAKEINTRESOURCE(IDI_CURSOR_SMALL_BLACK));
	HICON hIconMediumBlack = LoadIcon(g_hInstDll, MAKEINTRESOURCE(IDI_CURSOR_MEDIUM_BLACK));
	HICON hIconLargeBlack = LoadIcon(g_hInstDll, MAKEINTRESOURCE(IDI_CURSOR_LARGE_BLACK));

	HICON hIconSmallInverting = LoadIcon(g_hInstDll, MAKEINTRESOURCE(IDI_CURSOR_SMALL_INVERTING));
	HICON hIconMediumInverting = LoadIcon(g_hInstDll, MAKEINTRESOURCE(IDI_CURSOR_MEDIUM_INVERTING));
	HICON hIconLargeInverting = LoadIcon(g_hInstDll, MAKEINTRESOURCE(IDI_CURSOR_LARGE_INVERTING));

#pragma message ("Need to destroy bitmaps in OnDestroy")
	SendMessage(GetDlgItem(m_hwnd, IDC_RADIO2), BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIconSmallWhite);
	SendMessage(GetDlgItem(m_hwnd, IDC_RADIO3), BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIconMediumWhite);
	SendMessage(GetDlgItem(m_hwnd, IDC_RADIO4), BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIconLargeWhite);

	SendMessage(GetDlgItem(m_hwnd, IDC_RADIO5), BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIconSmallBlack);
	SendMessage(GetDlgItem(m_hwnd, IDC_RADIO6), BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIconMediumBlack);
	SendMessage(GetDlgItem(m_hwnd, IDC_RADIO7), BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIconLargeBlack);

	SendMessage(GetDlgItem(m_hwnd, IDC_RADIO8), BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIconSmallInverting);
	SendMessage(GetDlgItem(m_hwnd, IDC_RADIO9), BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIconMediumInverting);
	SendMessage(GetDlgItem(m_hwnd, IDC_RADIO10), BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIconLargeInverting);

	Button_SetCheck(GetDlgItem(m_hwnd, IDC_RADIO1), TRUE);

	UpdateControls();
	return 1;
}


void CMouseCursorPg::UpdateControls()
{
	// Nothing to do
}


LRESULT
CMouseCursorPg::OnCommand(
	HWND hwnd,
	WPARAM wParam,
	LPARAM lParam
	)
{
	LRESULT lResult = 1;

	WORD wNotifyCode = HIWORD(wParam);
	WORD wCtlID      = LOWORD(wParam);
	HWND hwndCtl     = (HWND)lParam;

	HCURSOR hCursor = NULL;
	switch(wCtlID)
	{
	case IDC_RADIO1:
		g_Options.m_schemePreview.m_nCursorScheme = 0;
		break;
	case IDC_RADIO2:
		g_Options.m_schemePreview.m_nCursorScheme = 1;
		break;
	case IDC_RADIO3:
		g_Options.m_schemePreview.m_nCursorScheme = 2;
		break;
	case IDC_RADIO4:
		g_Options.m_schemePreview.m_nCursorScheme = 3;
		break;
	case IDC_RADIO5:
		g_Options.m_schemePreview.m_nCursorScheme = 4;
		break;
	case IDC_RADIO6:
		g_Options.m_schemePreview.m_nCursorScheme = 5;
		break;
	case IDC_RADIO7:
		g_Options.m_schemePreview.m_nCursorScheme = 6;
		break;
	case IDC_RADIO8:
		g_Options.m_schemePreview.m_nCursorScheme = 7;
		break;
	case IDC_RADIO9:
		g_Options.m_schemePreview.m_nCursorScheme = 8;
		break;
	case IDC_RADIO10:
		g_Options.m_schemePreview.m_nCursorScheme = 9;
		break;
	default:
		break;
	}
	g_Options.ApplyPreview();

	return lResult;
}
