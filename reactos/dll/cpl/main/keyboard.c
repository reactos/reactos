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
/* $Id$
 *
 * PROJECT:         ReactOS Main Control Panel
 * FILE:            lib/cpl/main/keyboard.c
 * PURPOSE:         Keyboard Control Panel
 * PROGRAMMER:      Eric Kohl
 */

#include <windows.h>
#include <devguid.h>
#include <commctrl.h>
#include <prsht.h>
#include <cpl.h>

#include "main.h"
#include "resource.h"


/* Property page dialog callback */
static INT_PTR CALLBACK
KeybSpeedProc(IN HWND hwndDlg,
	      IN UINT uMsg,
	      IN WPARAM wParam,
	      IN LPARAM lParam)
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
KeybHardwareProc(IN HWND hwndDlg,
	         IN UINT uMsg,
	         IN WPARAM wParam,
	         IN LPARAM lParam)
{
    GUID Guids[1];
    Guids[0] = GUID_DEVCLASS_KEYBOARD;

    switch(uMsg)
    {
        case WM_INITDIALOG:
        {


            /* create the hardware page */
            DeviceCreateHardwarePageEx(hwndDlg,
                                       Guids,
                                       sizeof(Guids) / sizeof(Guids[0]),
                                       HWPD_STANDARDLIST);
            break;
        }
    }

    return FALSE;
}


LONG APIENTRY
KeyboardApplet(HWND hwnd, UINT uMsg, LONG wParam, LONG lParam)
{
  PROPSHEETPAGE psp[2];
  PROPSHEETHEADER psh;
  TCHAR Caption[256];

  LoadString(hApplet, IDS_CPLNAME_2, Caption, sizeof(Caption) / sizeof(TCHAR));

  ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
  psh.dwSize = sizeof(PROPSHEETHEADER);
  psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_PROPTITLE;
  psh.hwndParent = NULL;
  psh.hInstance = hApplet;
  psh.hIcon = LoadIcon(hApplet, MAKEINTRESOURCE(IDC_CPLICON_2));
  psh.pszCaption = Caption;
  psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
  psh.nStartPage = 0;
  psh.ppsp = psp;

  InitPropSheetPage(&psp[0], IDD_KEYBSPEED, KeybSpeedProc);
  InitPropSheetPage(&psp[1], IDD_HARDWARE, KeybHardwareProc);

  return (LONG)(PropertySheet(&psh) != -1);
}

/* EOF */
