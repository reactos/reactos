/*
 *  ReactOS
 *  Copyright (C) 2004 ReactOS Team
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
/* $Id: mouse.c,v 1.1 2004/10/30 12:35:16 ekohl Exp $
 *
 * PROJECT:         ReactOS Main Control Panel
 * FILE:            lib/cpl/main/mouse.c
 * PURPOSE:         Mouse Control Panel
 * PROGRAMMER:      Eric Kohl
 */

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>

#include "main.h"
#include "resource.h"



/* Property page dialog callback */
static INT_PTR CALLBACK
Page1Proc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:
      break;
  }
  return FALSE;
}

/* Property page dialog callback */
static INT_PTR CALLBACK
Page2Proc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:
      break;
  }
  return FALSE;
}

/* Property page dialog callback */
static INT_PTR CALLBACK
Page3Proc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:
      break;
  }
  return FALSE;
}


LONG APIENTRY
MouseApplet(HWND hwnd, UINT uMsg, LONG lParam1, LONG lParam2)
{
  PROPSHEETPAGE psp[3];
  PROPSHEETHEADER psh;
  TCHAR Caption[256];

  LoadString(hApplet, IDS_CPLNAME_1, Caption, sizeof(Caption) / sizeof(TCHAR));

  ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
  psh.dwSize = sizeof(PROPSHEETHEADER);
  psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_PROPTITLE;
  psh.hwndParent = NULL;
  psh.hInstance = hApplet;
  psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDC_CPLICON_1));
  psh.pszCaption = Caption;
  psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
  psh.nStartPage = 0;
  psh.ppsp = psp;

  InitPropSheetPage(&psp[0], IDD_PROPPAGE1, Page1Proc);
  InitPropSheetPage(&psp[1], IDD_PROPPAGE2, Page2Proc);
  InitPropSheetPage(&psp[2], IDD_PROPPAGE3, Page3Proc);

  return (LONG)(PropertySheet(&psh) != -1);
}

/* EOF */
