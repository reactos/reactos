
#include "resource.h"
#include "precomp.h"

#define CX_BITMAP 18
#define CY_BITMAP 18

DIRSIZE sz;
DLG_VAR dv;
WCHAR_VAR wcv;
BOOL_VAR bv;

BOOL CALLBACK StartDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	WCHAR LogicalDrives[MAX_PATH] = { 0 };
	int ItemIndex = 0;
	static HBITMAP hbmDrive;
	static HBITMAP hbmMask;
	DWORD dwIndex = 0;

	bv.sysDrive = FALSE;
	
	DWORD drives = GetLogicalDriveStringsW(MAX_PATH, LogicalDrives);
	if (drives == 0)
	{
		MessageBoxW(NULL, L"GetLogicalDriveStringsW() failed!", L"Error", MB_OK | MB_ICONERROR);
		return FALSE;
	}
	
	switch(Message)
    {
		case WM_INITDIALOG:

			hbmDrive = LoadBitmapW(dv.hInst, MAKEINTRESOURCE(IDB_DRIVE));
			hbmMask = LoadBitmapW(dv.hInst, MAKEINTRESOURCE(IDB_DRIVE));

			if (drives <= MAX_PATH)
			{
				WCHAR* SingleDrive = LogicalDrives;
				WCHAR RealDrive[MAX_PATH] = { 0 };
				while (*SingleDrive)
				{
					if (GetDriveTypeW(SingleDrive) == 3)
					{
						StringCchCopyW(RealDrive, MAX_PATH, SingleDrive);
						RealDrive[wcslen(RealDrive) - 1] = '\0';
						dwIndex = SendMessageW(GetDlgItem(hwnd, IDC_DRIVE), CB_ADDSTRING, 0, (LPARAM)RealDrive);
						if (SendMessageW(GetDlgItem(hwnd, IDC_DRIVE), CB_SETITEMDATA, dwIndex, (LPARAM)hbmDrive) == CB_ERR)
						{
							return FALSE;
						}
						memset(RealDrive, 0, sizeof RealDrive);
					}
					SingleDrive += wcslen(SingleDrive) + 1;
				}
			}
			return TRUE;

		case WM_NOTIFY:
			return ThemeHandler(hwnd, (LPNMCUSTOMDRAW)lParam);

		case WM_THEMECHANGED:
			InvalidateRect(hwnd, NULL, FALSE);
			break;
		
		case WM_MEASUREITEM:
		{
			// Set the height of the items in the food groups combo box.
			LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lParam;

			if (lpmis->itemHeight < 18 + 2)
				lpmis->itemHeight = 18 + 2;

			break;
		}

		case WM_DRAWITEM:
		{
			COLORREF clrBackground;
			COLORREF clrForeground;
			TEXTMETRIC tm;
			int x;
			int y;
			size_t cch;
			HBITMAP hbmIcon;
			WCHAR achTemp[256] = { 0 };

			LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;

			if (lpdis->itemID == -1)
				break;

			hbmIcon = (HBITMAP)lpdis->itemData;

			clrForeground = SetTextColor(lpdis->hDC,
				GetSysColor(lpdis->itemState & ODS_SELECTED ?
					COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));

			clrBackground = SetBkColor(lpdis->hDC,
				GetSysColor(lpdis->itemState & ODS_SELECTED ?
					COLOR_HIGHLIGHT : COLOR_WINDOW));

			GetTextMetrics(lpdis->hDC, &tm);
			y = (lpdis->rcItem.bottom + lpdis->rcItem.top - tm.tmHeight) / 2;
			x = LOWORD(GetDialogBaseUnits()) / 4;

			SendMessage(lpdis->hwndItem, CB_GETLBTEXT,
				lpdis->itemID, (LPARAM)achTemp);


			StringCchLength(achTemp, _countof(achTemp), &cch);

			ExtTextOut(lpdis->hDC, CX_BITMAP + 2 * x, y,
				ETO_CLIPPED | ETO_OPAQUE, &lpdis->rcItem,
				achTemp, (UINT)cch, NULL);

			SetTextColor(lpdis->hDC, clrForeground);
			SetBkColor(lpdis->hDC, clrBackground);

			HDC hdc = CreateCompatibleDC(lpdis->hDC);
			if (hdc == NULL)
				break;

			SelectObject(hdc, hbmMask);
			BitBlt(lpdis->hDC, x, lpdis->rcItem.top + 1,
				CX_BITMAP, CY_BITMAP, hdc, 0, 0, SRCAND);

			SelectObject(hdc, hbmIcon);
			BitBlt(lpdis->hDC, x, lpdis->rcItem.top + 1,
				CX_BITMAP, CY_BITMAP, hdc, 0, 0, SRCPAINT);

			DeleteDC(hdc);

			if (lpdis->itemState & ODS_FOCUS)
				DrawFocusRect(lpdis->hDC, &lpdis->rcItem);

			break;
		}

		case WM_COMMAND:
			if(HIWORD(wParam) == CBN_SELCHANGE)
			{
				ItemIndex = SendMessageW(GetDlgItem(hwnd, IDC_DRIVE), CB_GETCURSEL, 0, 0);
				if (ItemIndex == CB_ERR)
				{
					MessageBoxW(NULL, L"SendMessageW failed!", L"Error", MB_OK | MB_ICONERROR);
					return FALSE;
				}
				SendMessageW(GetDlgItem(hwnd, IDC_DRIVE), CB_GETLBTEXT, (WPARAM)ItemIndex, (LPARAM)wcv.driveLetter);
			}

			switch(LOWORD(wParam))
			{
				case IDOK:
					if (wcslen(wcv.driveLetter) == 0)
					{
						MessageBoxW(hwnd, L"Select a drive!", L"Warning", MB_OK | MB_ICONWARNING);
						break;
					}
					DeleteObject(hbmDrive);
					DeleteObject(hbmMask);
					EndDialog(hwnd, IDOK);
					break;
				case IDCANCEL:
					DeleteObject(hbmDrive);
					DeleteObject(hbmMask);
					EndDialog(hwnd, IDCANCEL);
					break;
            }
			break;

		case WM_CLOSE:
			EndDialog(hwnd, IDCANCEL);
			break;	

		default:
			return FALSE;
    }
	return TRUE;
}

BOOL CALLBACK ProgressDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static HANDLE threadOBJ = NULL;
	WCHAR tempText[MAX_PATH] = { 0 };
	WCHAR fullText[MAX_PATH] = { 0 };

	switch(Message)
    {
        case WM_INITDIALOG:
			LoadStringW(GetModuleHandleW(NULL), IDS_SCAN, tempText, _countof(tempText));
			StringCchPrintfW(fullText, _countof(fullText), tempText, wcv.driveLetter);
			dv.hwndDlg = hwnd;
			SetDlgItemTextW(hwnd, IDC_STATIC_SCAN, fullText);
			threadOBJ = CreateThread(NULL, 0, &sizeCheck, NULL, 0, NULL);
			return TRUE;
		
		case WM_NOTIFY:
			return ThemeHandler(hwnd, (LPNMCUSTOMDRAW)lParam);

		case WM_THEMECHANGED:
			InvalidateRect(hwnd, NULL, FALSE);
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDCANCEL:
					CloseHandle(threadOBJ);
					EndDialog(hwnd, IDCANCEL);
					break;
			}
		case WM_CLOSE:
			CloseHandle(threadOBJ);
			EndDialog(hwnd, IDCANCEL);
			break;
        case WM_DESTROY:
			CloseHandle(threadOBJ);
			EndDialog(hwnd, 0);
			break;
		default:
            return FALSE;
    }
    return TRUE;
}

BOOL CALLBACK ChoiceDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	LPNMHDR pnmh;
	int mesgBox;
	
	WCHAR tempText[MAX_PATH] = { 0 };
	WCHAR fullText[MAX_PATH] = { 0 };

	switch (Message)
	{
		case WM_INITDIALOG:
			LoadStringW(GetModuleHandleW(NULL), IDS_TITLE, tempText, _countof(tempText));
			StringCchPrintfW(fullText, _countof(fullText), tempText, wcv.driveLetter);
			SetWindowTextW(hwnd, fullText);
			return OnCreate(hwnd);

		case WM_NOTIFY:
			pnmh = (LPNMHDR)lParam;
			if ((pnmh->hwndFrom == dv.hTab) &&
				(pnmh->idFrom == IDC_TAB) &&
				(pnmh->code == TCN_SELCHANGE))
			{
				MsConfig_OnTabWndSelChange();
			}
			return ThemeHandler(hwnd, (LPNMCUSTOMDRAW)lParam);

		case WM_THEMECHANGED:
			InvalidateRect(hwnd, NULL, FALSE);
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					if (bv.downloadClean == FALSE && bv.recycleClean == FALSE && bv.downloadClean == FALSE && bv.rappsClean == FALSE && bv.tempClean == FALSE)
					{
						MessageBoxW(hwnd, L"Select an option!", L"Warning", MB_OK | MB_ICONWARNING);
						break;
					}
			
					mesgBox = MessageBoxW(hwnd, L"Are you sure to delete these files?", L"Warning", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
					switch (mesgBox)
					{
						case IDYES:
							EndDialog(hwnd, IDOK);
							break;

						case IDNO:
							break;
					}
					break;

				case IDCANCEL:
					if(dv.hChoicePage)
						DestroyWindow(dv.hChoicePage);

					if (dv.hOptionsPage)
						DestroyWindow(dv.hOptionsPage);
					
					EndDialog(hwnd, IDCANCEL);
					break;
			}
			break;
	
		case WM_CLOSE:
			if(dv.hChoicePage)
				DestroyWindow(dv.hChoicePage);

			if (dv.hOptionsPage)
				DestroyWindow(dv.hOptionsPage);

			EndDialog(hwnd, IDCANCEL);
			break;
		
		default:
			return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK ProgressEndDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static HANDLE threadOBJ = NULL;
	WCHAR tempText[MAX_PATH] = { 0 };
	WCHAR fullText[MAX_PATH] = { 0 };

	switch (Message)
	{
	case WM_INITDIALOG:
		LoadStringW(GetModuleHandleW(NULL), IDS_REMOVAL, tempText, _countof(tempText));
		StringCchPrintfW(fullText, _countof(fullText), tempText, wcv.driveLetter);
		dv.hwndDlg = hwnd;
		SetDlgItemTextW(hwnd, IDC_STATIC_REMOVAL, fullText);
		threadOBJ = CreateThread(NULL, 0, &folderRemoval, NULL, 0, NULL);
		return TRUE;

	case WM_NOTIFY:
		return ThemeHandler(hwnd, (LPNMCUSTOMDRAW)lParam);

	case WM_THEMECHANGED:
		InvalidateRect(hwnd, NULL, FALSE);
		break;
	
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			case IDCANCEL:
				CloseHandle(threadOBJ);
				EndDialog(hwnd, 0);
				break;
		}
		break;

	case WM_CLOSE:
		CloseHandle(threadOBJ);
		EndDialog(hwnd, 0);
		break;

	case WM_DESTROY:
		CloseHandle(threadOBJ);
		EndDialog(hwnd, 0);
		break;

	default:
		return FALSE;
	}
	return TRUE;
}
