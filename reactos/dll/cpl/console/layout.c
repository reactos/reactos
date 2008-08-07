/*
 * PROJECT:         ReactOS Console Configuration DLL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/console/layout.c
 * PURPOSE:         displays layout dialog
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@student.tugraz.at)
 */

#include "console.h"



void PaintConsole(LPDRAWITEMSTRUCT drawItem, PConsoleInfo pConInfo)
{
	HBRUSH hBrush;
	RECT cRect, fRect;
	DWORD startx, starty;
	DWORD endx, endy;
	DWORD sizex, sizey;

	FillRect(drawItem->hDC, &drawItem->rcItem, GetSysColorBrush(COLOR_BACKGROUND));

	sizex = drawItem->rcItem.right - drawItem->rcItem.left;
	sizey = drawItem->rcItem.bottom - drawItem->rcItem.top;

	if (pConInfo->WindowPosition == UINT_MAX)
	{
		startx = sizex / 3;
		starty = sizey / 3;
	}
	else
	{
		//TODO
		// calculate pos correctly when console centered
		startx = sizex / 3;
		starty = sizey / 3;
	}

	//TODO
	// strech console when bold fonts are selected
	endx = drawItem->rcItem.right - startx + 15;
	endy = starty + sizey / 3;

	/* draw console size */
	SetRect(&cRect, startx, starty, endx, endy);
	FillRect(drawItem->hDC, &cRect, GetSysColorBrush(COLOR_WINDOWFRAME));

	/* draw console border */
	SetRect(&fRect, startx + 1, starty + 1, cRect.right - 1, cRect.bottom - 1);
	FrameRect(drawItem->hDC, &fRect, GetSysColorBrush(COLOR_ACTIVEBORDER));

	/* draw left box */
	SetRect(&fRect, startx + 3, starty + 3, startx + 5, starty + 5);
	FillRect(drawItem->hDC, &fRect, GetSysColorBrush(COLOR_ACTIVEBORDER));

	/* draw window title */
	SetRect(&fRect, startx + 7, starty + 3, cRect.right - 9, starty + 5);
	FillRect(drawItem->hDC, &fRect, GetSysColorBrush(COLOR_ACTIVECAPTION));

	/* draw first right box */
	SetRect(&fRect, fRect.right + 1, starty + 3, fRect.right + 3, starty + 5);
	FillRect(drawItem->hDC, &fRect, GetSysColorBrush(COLOR_ACTIVEBORDER));

	/* draw second right box */
	SetRect(&fRect, fRect.right + 1, starty + 3, fRect.right + 3, starty + 5);
	FillRect(drawItem->hDC, &fRect, GetSysColorBrush(COLOR_ACTIVEBORDER));

	/* draw scrollbar */
	SetRect(&fRect, cRect.right - 5, fRect.bottom + 1, cRect.right - 3, cRect.bottom - 3);
	FillRect(drawItem->hDC, &fRect, GetSysColorBrush(COLOR_SCROLLBAR));

	/* draw console background */
	hBrush = CreateSolidBrush(pConInfo->ScreenBackground);
	SetRect(&fRect, startx + 3, starty + 6, cRect.right - 6, cRect.bottom - 3);
	FillRect(drawItem->hDC, &fRect, hBrush);
	DeleteObject((HGDIOBJ)hBrush);
}

void PaintText(LPDRAWITEMSTRUCT drawItem, PConsoleInfo pConInfo)
{
	COLORREF pbkColor, ptColor;
	COLORREF nbkColor, ntColor;
	HBRUSH hBrush = NULL;
	TCHAR szText[1024];

	ZeroMemory(szText, sizeof(szText));
	LoadString(hApplet, IDS_SCREEN_TEXT, szText, sizeof(szText) / sizeof(TCHAR));

	if (drawItem->CtlID == IDC_STATIC_SCREEN_COLOR)
	{
		nbkColor = pConInfo->ScreenBackground;
		hBrush = CreateSolidBrush(nbkColor);
		ntColor = pConInfo->ScreenText;
	}
	else if (drawItem->CtlID == IDC_STATIC_POPUP_COLOR)
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
	if (ntColor == nbkColor)
	{
		/* text has same color -> invisible */
		return;
	}

	ptColor = SetTextColor(drawItem->hDC, ntColor);
	pbkColor = SetBkColor(drawItem->hDC, nbkColor);
	DrawText(drawItem->hDC, szText, _tcslen(szText), &drawItem->rcItem, 0);
	SetTextColor(drawItem->hDC, ptColor);
	SetBkColor(drawItem->hDC, pbkColor);
	DeleteObject((HGDIOBJ)hBrush);
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
	LPNMUPDOWN lpnmud;
	LPPSHNOTIFY lppsn;
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
			SendMessage(GetDlgItem(hwndDlg, IDC_UPDOWN_SCREEN_BUFFER_HEIGHT), UDM_SETRANGE, 0, (LPARAM)MAKELONG(9999, 1));
			SendMessage(GetDlgItem(hwndDlg, IDC_UPDOWN_SCREEN_BUFFER_WIDTH), UDM_SETRANGE, 0, (LPARAM)MAKELONG(9999, 1));
			SendMessage(GetDlgItem(hwndDlg, IDC_UPDOWN_WINDOW_SIZE_HEIGHT), UDM_SETRANGE, 0, (LPARAM)MAKELONG(9999, 1));
			SendMessage(GetDlgItem(hwndDlg, IDC_UPDOWN_WINDOW_SIZE_WIDTH), UDM_SETRANGE, 0, (LPARAM)MAKELONG(9999, 1));

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
		case WM_NOTIFY:
		{
			lpnmud = (LPNMUPDOWN) lParam;
			lppsn = (LPPSHNOTIFY) lParam;

			if (lppsn->hdr.code == UDN_DELTAPOS)
			{
				DWORD wheight, wwidth;
				DWORD sheight, swidth;
				DWORD left, top;

				if (lppsn->hdr.idFrom == IDC_UPDOWN_WINDOW_SIZE_WIDTH)
				{
					wwidth = lpnmud->iPos + lpnmud->iDelta;
				}
				else
				{
					wwidth = GetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_SIZE_WIDTH, NULL, FALSE);
				}

				if (lppsn->hdr.idFrom == IDC_UPDOWN_WINDOW_SIZE_HEIGHT)
				{
					wheight = lpnmud->iPos + lpnmud->iDelta;
				}
				else
				{
					wheight = GetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_SIZE_HEIGHT, NULL, FALSE);
				}

				if (lppsn->hdr.idFrom == IDC_UPDOWN_SCREEN_BUFFER_WIDTH)
				{
					swidth = lpnmud->iPos + lpnmud->iDelta;
				}
				else
				{
					swidth = GetDlgItemInt(hwndDlg, IDC_EDIT_SCREEN_BUFFER_WIDTH, NULL, FALSE);
				}

				if (lppsn->hdr.idFrom == IDC_UPDOWN_SCREEN_BUFFER_HEIGHT)
				{
					sheight = lpnmud->iPos + lpnmud->iDelta;
				}
				else
				{
					sheight = GetDlgItemInt(hwndDlg, IDC_EDIT_SCREEN_BUFFER_HEIGHT, NULL, FALSE);
				}

				if (lppsn->hdr.idFrom == IDC_UPDOWN_WINDOW_POS_LEFT)
				{
					left = lpnmud->iPos + lpnmud->iDelta;
				}
				else
				{
					left = GetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_POS_LEFT, NULL, FALSE);
				}

				if (lppsn->hdr.idFrom == IDC_UPDOWN_WINDOW_POS_TOP)
				{
					top = lpnmud->iPos + lpnmud->iDelta;
				}
				else
				{
					top = GetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_POS_TOP, NULL, FALSE);
				}

				if (lppsn->hdr.idFrom == IDC_UPDOWN_WINDOW_SIZE_WIDTH || lppsn->hdr.idFrom == IDC_UPDOWN_WINDOW_SIZE_HEIGHT)
				{
					/* automatically adjust screen buffer size when window size enlarges */
					if (wwidth >= swidth)
					{
						SetDlgItemInt(hwndDlg, IDC_EDIT_SCREEN_BUFFER_WIDTH, wwidth, TRUE);
						swidth = wwidth;
					}

					if (wheight >= sheight)
					{
						SetDlgItemInt(hwndDlg, IDC_EDIT_SCREEN_BUFFER_HEIGHT, wheight, TRUE);
						sheight = wheight;
					}
				}
				swidth = max(swidth, 1);
				sheight = max(sheight, 1);

				if (lppsn->hdr.idFrom == IDC_UPDOWN_SCREEN_BUFFER_WIDTH || lppsn->hdr.idFrom == IDC_UPDOWN_SCREEN_BUFFER_HEIGHT)
				{
					/* automatically adjust window size when screen buffer decreases */
					if (wwidth > swidth)
					{
						SetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_SIZE_WIDTH, swidth, TRUE);
						wwidth = swidth;
					}

					if (wheight > sheight)
					{
						SetDlgItemInt(hwndDlg, IDC_EDIT_WINDOW_SIZE_HEIGHT, sheight, TRUE);
						wheight = sheight;
					}
				}

				pConInfo->ScreenBuffer = MAKELONG(swidth, sheight);
				pConInfo->WindowSize = MAKELONG(wwidth, wheight);
				pConInfo->WindowPosition = MAKELONG(left, top);
				PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
			}
			break;
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
						pConInfo->WindowPosition = UINT_MAX;
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
