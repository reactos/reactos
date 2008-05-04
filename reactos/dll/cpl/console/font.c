/* $Id$
 *
 * PROJECT:         ReactOS Console Configuration DLL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/console/font.c
 * PURPOSE:         displays font dialog
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@student.tugraz.at)
 */


#include "console.h"

INT_PTR
CALLBACK
FontProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
	LPDRAWITEMSTRUCT drawItem;
	PConsoleInfo pConInfo = (PConsoleInfo)GetWindowLongPtr(hwndDlg, DWLP_USER);

	UNREFERENCED_PARAMETER(hwndDlg);
	UNREFERENCED_PARAMETER(wParam);


	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			pConInfo = (PConsoleInfo) ((LPPROPSHEETPAGE)lParam)->lParam;
			SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pConInfo);
			return TRUE;
		}
		case WM_DRAWITEM:
		{
			drawItem = (LPDRAWITEMSTRUCT)lParam;
			if (drawItem->CtlID == IDC_STATIC_FONT_WINDOW_PREVIEW)
			{
				PaintConsole(drawItem, pConInfo);
			}
			else if (drawItem->CtlID == IDC_STATIC_SELECT_FONT_PREVIEW)
			{
				PaintText(drawItem, pConInfo);
			}
			return TRUE;
		}
		default:
		{
			break;
		}
	}

	return FALSE;
}
