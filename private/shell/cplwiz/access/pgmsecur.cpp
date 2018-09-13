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

	TCHAR szPath[_MAX_PATH];
	GetWindowsDirectory(szPath, _MAX_PATH);
	lstrcat(szPath, __TEXT("\\cursors\\"));

	HCURSOR hCursor = NULL;
	switch(wCtlID)
	{
	case IDC_RADIO3:
		lstrcat(szPath, __TEXT("arrow_m.cur"));
		hCursor = LoadCursorFromFile(szPath);
		lResult = 0;
		break;
	case IDC_RADIO4:
		lstrcat(szPath, __TEXT("arrow_l.cur"));
		hCursor = LoadCursorFromFile(szPath);
		lResult = 0;
		break;
	case IDC_RADIO5:
		lstrcat(szPath, __TEXT("arrow_r.cur"));
		hCursor = LoadCursorFromFile(szPath);
		lResult = 0;
		break;
	case IDC_RADIO6:
		lstrcat(szPath, __TEXT("arrow_rm.cur"));
		hCursor = LoadCursorFromFile(szPath);
		lResult = 0;
		break;
	case IDC_RADIO7:
		lstrcat(szPath, __TEXT("arrow_rl.cur"));
		hCursor = LoadCursorFromFile(szPath);
		lResult = 0;
		break;
	case IDC_RADIO8:
		lstrcat(szPath, __TEXT("arrow_i.cur"));
		hCursor = LoadCursorFromFile(szPath);
		lResult = 0;
		break;
	case IDC_RADIO9:
		lstrcat(szPath, __TEXT("arrow_im.cur"));
		hCursor = LoadCursorFromFile(szPath);
		lResult = 0;
		break;
	case IDC_RADIO10:
		lstrcat(szPath, __TEXT("arrow_il.cur"));
		hCursor = LoadCursorFromFile(szPath);
		lResult = 0;
		break;

	default:
		break;
	}
#define OCR_NORMAL          32512
#define OCR_IBEAM           32513
	if(hCursor)
		SetSystemCursor(hCursor, OCR_NORMAL);


	return lResult;
}
