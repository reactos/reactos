/*
 *  ReactOS applications
 *  Copyright (C) 2004-2008 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS GUI first stage setup application
 * FILE:        subsys/system/reactos/reactos.c
 * PROGRAMMERS: Eric Kohl, Matthias Kupfer
 */

#include <windows.h>
#include <commctrl.h>
#include <tchar.h>

#include "resource.h"

/* GLOBALS ******************************************************************/

HFONT hTitleFont;

/* FUNCTIONS ****************************************************************/

static VOID
CenterWindow(HWND hWnd)
{
  HWND hWndParent;
  RECT rcParent;
  RECT rcWindow;

  hWndParent = GetParent(hWnd);
  if (hWndParent == NULL)
    hWndParent = GetDesktopWindow();

  GetWindowRect(hWndParent, &rcParent);
  GetWindowRect(hWnd, &rcWindow);

  SetWindowPos(hWnd,
	       HWND_TOP,
	       ((rcParent.right - rcParent.left) - (rcWindow.right - rcWindow.left)) / 2,
	       ((rcParent.bottom - rcParent.top) - (rcWindow.bottom - rcWindow.top)) / 2,
	       0,
	       0,
	       SWP_NOSIZE);
}

static HFONT
CreateTitleFont(VOID)
{
  NONCLIENTMETRICS ncm;
  LOGFONT LogFont;
  HDC hdc;
  INT FontSize;
  HFONT hFont;

  ncm.cbSize = sizeof(NONCLIENTMETRICS);
  SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

  LogFont = ncm.lfMessageFont;
  LogFont.lfWeight = FW_BOLD;
  _tcscpy(LogFont.lfFaceName, _T("MS Shell Dlg"));

  hdc = GetDC(NULL);
  FontSize = 12;
  LogFont.lfHeight = 0 - GetDeviceCaps (hdc, LOGPIXELSY) * FontSize / 72;
  hFont = CreateFontIndirect(&LogFont);
  ReleaseDC(NULL, hdc);

  return hFont;
}

static INT_PTR CALLBACK
StartDlgProc(HWND hwndDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
  switch (uMsg)
  {
	  case WM_INITDIALOG:
	  {
		HWND hwndControl;
		DWORD dwStyle;

          	hwndControl = GetParent(hwndDlg);

		/* Center the wizard window */
                CenterWindow (hwndControl);

          	dwStyle = GetWindowLong(hwndControl, GWL_STYLE);
	        SetWindowLong(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);
		
		/* Hide and disable the 'Cancel' button at the moment,
		 * later we use this button to cancel the setup process
		 * like F3 in usetup
		 */
		hwndControl = GetDlgItem(GetParent(hwndDlg), IDCANCEL);
		ShowWindow (hwndControl, SW_HIDE);
		EnableWindow (hwndControl, FALSE);
	        
		/* Set title font */
		SendDlgItemMessage(hwndDlg,
                             IDC_STARTTITLE,
                             WM_SETFONT,
                             (WPARAM)hTitleFont,
                             (LPARAM)TRUE);
}
	  break;
	  case WM_NOTIFY:
	  {
          LPNMHDR lpnm = (LPNMHDR)lParam;

		  switch (lpnm->code)
		  {		
		      case PSN_SETACTIVE: // Only "Finish" for closing the App
			PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_FINISH);
			break;
		      default:
			break;
		  }
	  break;
	  default:
	  	break;
	  }

  }
  return FALSE;
}

int WINAPI
WinMain(HINSTANCE hInst,
	HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine,
	int nCmdShow)
{
  PROPSHEETHEADER psh;
  HPROPSHEETPAGE ahpsp[1];
  PROPSHEETPAGE psp = {0};
  UINT nPages = 0;

  /* Create the Start page */
  psp.dwSize = sizeof(PROPSHEETPAGE);
  psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
  psp.hInstance = hInst;
  psp.lParam = 0;
  psp.pfnDlgProc = StartDlgProc;
  psp.pszTemplate = MAKEINTRESOURCE(IDD_STARTPAGE);
  ahpsp[nPages++] = CreatePropertySheetPage(&psp);

  // Here we can add the next pages and switch on later

  /* Create the property sheet */
  psh.dwSize = sizeof(PROPSHEETHEADER);
  psh.dwFlags = PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
  psh.hInstance = hInst;
  psh.hwndParent = NULL;
  psh.nPages = nPages;
  psh.nStartPage = 0;
  psh.phpage = ahpsp;
  psh.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK);
  psh.pszbmHeader = MAKEINTRESOURCE(IDB_HEADER);

  /* Create title font */
  hTitleFont = CreateTitleFont();

  /* Display the wizard */
  PropertySheet(&psh);

  DeleteObject(hTitleFont);

  return 0;

}

/* EOF */
