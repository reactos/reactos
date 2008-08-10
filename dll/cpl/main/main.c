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
 * PROJECT:         ReactOS Sample Control Panel
 * FILE:            lib/cpl/main/main.c
 * PURPOSE:         ReactOS Main Control Panel
 * PROGRAMMER:      Eric Kohl
 * UPDATE HISTORY:
 *      05-01-2004  Created
 */
#include <windows.h>
#include <initguid.h>
#include <devguid.h>
#include <commctrl.h>
#include <cpl.h>
#include <cplext.h>

#include "main.h"
#include "resource.h"


#define NUM_APPLETS	(2)


HINSTANCE hApplet = 0;


/* Applets */
APPLET Applets[NUM_APPLETS] =
{
  {IDC_CPLICON_1, IDS_CPLNAME_1, IDS_CPLDESCRIPTION_1, MouseApplet},
  {IDC_CPLICON_2, IDS_CPLNAME_2, IDS_CPLDESCRIPTION_2, KeyboardApplet}
};


BOOL
InitPropSheetPage(PROPSHEETHEADER *ppsh, WORD idDlg, DLGPROC DlgProc)
{
    HPROPSHEETPAGE hPage;
    PROPSHEETPAGE psp;

    if (ppsh->nPages < MAX_CPL_PAGES)
    {
        ZeroMemory(&psp, sizeof(psp));
        psp.dwSize = sizeof(psp);
        psp.dwFlags = PSP_DEFAULT;
        psp.hInstance = hApplet;
        psp.pszTemplate = MAKEINTRESOURCE(idDlg);
        psp.pfnDlgProc = DlgProc;

        hPage = CreatePropertySheetPage(&psp);
        if (hPage != NULL)
        {
            return PropSheetAddPage(hPage, (LPARAM)ppsh);
        }
    }

    return FALSE;
}

BOOL CALLBACK
PropSheetAddPage(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    PROPSHEETHEADER *ppsh = (PROPSHEETHEADER *)lParam;
    if (ppsh != NULL && ppsh->nPages < MAX_CPL_PAGES)
    {
        ppsh->phpage[ppsh->nPages++] = hpage;
        return TRUE;
    }

    return FALSE;
}


/* Control Panel Callback */
LONG CALLBACK
CPlApplet(HWND hwndCpl,
          UINT uMsg,
          LPARAM lParam1,
          LPARAM lParam2)
{
    switch(uMsg)
    {
        case CPL_INIT:
            return TRUE;

        case CPL_GETCOUNT:
            return NUM_APPLETS;

        case CPL_INQUIRE:
        {
            CPLINFO *CPlInfo = (CPLINFO*)lParam2;
            UINT uAppIndex = (UINT)lParam1;

            CPlInfo->lData = lParam1;
            CPlInfo->idIcon = Applets[uAppIndex].idIcon;
            CPlInfo->idName = Applets[uAppIndex].idName;
            CPlInfo->idInfo = Applets[uAppIndex].idDescription;
            break;
        }

        case CPL_DBLCLK:
        {
            UINT uAppIndex = (UINT)lParam1;
            Applets[uAppIndex].AppletProc(hwndCpl, uMsg, lParam1, lParam2);
            break;
        }
    }

    return FALSE;
}


BOOL STDCALL
DllMain(HINSTANCE hinstDLL,
        DWORD dwReason,
        LPVOID lpReserved)
{
    INITCOMMONCONTROLSEX InitControls;
    UNREFERENCED_PARAMETER(lpReserved);

    switch(dwReason)
    {
        case DLL_PROCESS_ATTACH:
            InitControls.dwSize = sizeof(INITCOMMONCONTROLSEX);
            InitControls.dwICC = ICC_LISTVIEW_CLASSES | ICC_UPDOWN_CLASS | ICC_BAR_CLASSES;
            InitCommonControlsEx(&InitControls);

            hApplet = hinstDLL;
            break;
    }

    return TRUE;
}

