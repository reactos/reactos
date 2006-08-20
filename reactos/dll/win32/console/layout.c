/* $Id$
 *
 * PROJECT:         ReactOS Console Configuration DLL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/console/layout.c
 * PURPOSE:         displays layout dialog
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@student.tugraz.at)
 */

#include "console.h"

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
			pConInfo = (PConsoleInfo) ((LPPROPSHEETPAGE)lParam)->lParam;
			SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pConInfo);
			return TRUE;
		}
		default:
			break;
	}

	return FALSE;
}
