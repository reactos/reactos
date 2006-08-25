/* $Id$
 *
 * PROJECT:         ReactOS Console Configuration DLL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/console/layout.c
 * PURPOSE:         displays layout dialog
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@student.tugraz.at)
 */

#include "console.h"



void PaintConsole(LPDRAWITEMSTRUCT drawItem, PConsoleInfo pConInfo)
{
	COLORREF bkColor;
	HBRUSH hBrush;

	bkColor = GetSysColor(COLOR_BACKGROUND);
	hBrush = CreateSolidBrush(bkColor);

	FillRect(drawItem->hDC, &drawItem->rcItem, hBrush);
	//TODO draw console image
	//MoveToEx(drawItem->hDC, 0, 0, NULL);
	//LineTo(drawItem->hDC, 10, 10);
	//MoveToEx(drawItem->hDC, 30, 30, NULL);
	//LineTo(drawItem->hDC, 40, 40);

	DeleteObject((HGDIOBJ)hBrush);
}

void PaintText(LPDRAWITEMSTRUCT drawItem, PConsoleInfo pConInfo)
{
	COLORREF pbkColor, ptColor;
	COLORREF nbkColor, ntColor;
	HBRUSH hBrush;
	TCHAR szText[1024];
	
	ZeroMemory(szText, sizeof(szText));
	LoadString(hApplet, IDS_SCREEN_TEXT, szText, sizeof(szText) / sizeof(TCHAR));

	if (drawItem->CtlID == IDC_STATIC_SCREEN_COLOR)
	{
		nbkColor = pConInfo->ScreenBackground;
		hBrush = CreateSolidBrush(nbkColor);
		ntColor = pConInfo->ScreenText;
	}
	else
	{
		nbkColor = pConInfo->PopupBackground;
		hBrush = CreateSolidBrush(nbkColor);
		ntColor = pConInfo->PopupText;
	}

	if (!hBrush)
	{
		return;
	}

	FillRect(drawItem->hDC, &drawItem->rcItem, hBrush);
	DeleteObject((HGDIOBJ)hBrush);
	ptColor = SetTextColor(drawItem->hDC, ntColor);
	pbkColor = SetBkColor(drawItem->hDC, nbkColor);
	if (ntColor != nbkColor)
	{
		/* hide text when it has same background color as text color */
		DrawText(drawItem->hDC, szText, _tcslen(szText), &drawItem->rcItem, 0);
	}
	SetTextColor(drawItem->hDC, ptColor);
	SetBkColor(drawItem->hDC, pbkColor);
}



INT_PTR 
CALLBACK
LayoutProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
	PConsoleInfo pConInfo = (PConsoleInfo)GetWindowLongPtr(hwndDlg, DWLP_USER);

	UNREFERENCED_PARAMETER(hwndDlg);
	UNREFERENCED_PARAMETER(wParam);

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			DWORD xres, yres;
			HDC hDC;
			pConInfo = (PConsoleInfo) ((LPPROPSHEETPAGE)lParam)->lParam;
			SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pConInfo);
			SetDlgItemInt(hwndDlg, IDC_EDIT_SCREEN_BUFFER_HEIGHT, HIWORD(pConInfo->ScreenBuffer), FALSE);
			SetDlgItemInt(hwndDlg, IDC_EDIT_SCREEN_BUFFER_WIDTH, LOWORD(pConInfo->ScreenBuffer), FALSE);
			SetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_SIZE_HEIGHT, HIWORD(pConInfo->WindowSize), FALSE);
			SetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_SIZE_WIDTH, LOWORD(pConInfo->WindowSize), FALSE);
			SendMessage(GetDlgItem(hwndDlg, IDC_UPDOWN_SCREEN_BUFFER_HEIGHT), UDM_SETRANGE, 0, (LPARAM)MAKELONG(9999, 0));
			SendMessage(GetDlgItem(hwndDlg, IDC_UPDOWN_SCREEN_BUFFER_WIDTH), UDM_SETRANGE, 0, (LPARAM)MAKELONG(9999, 0));
			SendMessage(GetDlgItem(hwndDlg, IDC_UPDOWN_WINDOW_SIZE_HEIGHT), UDM_SETRANGE, 0, (LPARAM)MAKELONG(9999, 0));
			SendMessage(GetDlgItem(hwndDlg, IDC_UPDOWN_WINDOW_SIZE_WIDTH), UDM_SETRANGE, 0, (LPARAM)MAKELONG(9999, 0));
			
			hDC = GetDC(NULL);
			xres = GetDeviceCaps(hDC, HORZRES);
			yres = GetDeviceCaps(hDC, VERTRES);
			SendMessage(GetDlgItem(hwndDlg, IDC_UPDOWN_WINDOW_POS_LEFT), UDM_SETRANGE, 0, (LPARAM)MAKELONG(xres, 0));
			SendMessage(GetDlgItem(hwndDlg, IDC_UPDOWN_WINDOW_POS_TOP), UDM_SETRANGE, 0, (LPARAM)MAKELONG(yres, 0));

			if (pConInfo->WindowPosition != -1)
			{
				SetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_POS_LEFT, LOWORD(pConInfo->WindowPosition), FALSE);
				SetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_POS_TOP, HIWORD(pConInfo->WindowPosition), FALSE);
			}
			else
			{
				//FIXME calculate window pos from xres, yres
				SetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_POS_LEFT, 88, FALSE);
				SetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_POS_TOP, 88, FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_WINDOW_POS_LEFT), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_WINDOW_POS_TOP), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_UPDOWN_WINDOW_POS_LEFT), FALSE);
				EnableWindow(GetDlgItem(hwndDlg, IDC_UPDOWN_WINDOW_POS_TOP), FALSE);
				SendMessage(GetDlgItem(hwndDlg, IDC_CHECK_SYSTEM_POS_WINDOW), BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
			}
			
			return TRUE;
		}
		case WM_DRAWITEM:
		{
			PaintConsole((LPDRAWITEMSTRUCT)lParam, pConInfo);
			return TRUE;
		}
		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case IDC_CHECK_SYSTEM_POS_WINDOW:
				{
					LONG res = SendMessage((HWND)lParam, BM_GETCHECK, 0, 0);
					if (res == BST_CHECKED)
					{
						ULONG left, top;

						left = GetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_POS_LEFT, NULL, FALSE);
						top = GetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_POS_TOP, NULL, FALSE);
						pConInfo->WindowPosition = MAKELONG(left, top);
						SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
						EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_WINDOW_POS_LEFT), TRUE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_WINDOW_POS_TOP), TRUE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_UPDOWN_WINDOW_POS_LEFT), TRUE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_UPDOWN_WINDOW_POS_TOP), TRUE);
					}
					else if (res == BST_UNCHECKED)
					{
						pConInfo->WindowPosition = -1;
						SendMessage((HWND)lParam, BM_SETCHECK, (WPARAM)BST_CHECKED, 0);
						EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_WINDOW_POS_LEFT), FALSE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT_WINDOW_POS_TOP), FALSE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_UPDOWN_WINDOW_POS_LEFT), FALSE);
						EnableWindow(GetDlgItem(hwndDlg, IDC_UPDOWN_WINDOW_POS_TOP), FALSE);
					}
				}
			}
		}
		default:
			break;
	}

	return FALSE;
}
