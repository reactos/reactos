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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * PROJECT:         ReactOS Sample Control Panel
 * FILE:            dll/cpl/main/main.c
 * PURPOSE:         ReactOS Main Control Panel
 * PROGRAMMER:      Eric Kohl
 * UPDATE HISTORY:
 *      05-01-2004  Created
 */

#include "main.h"

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
    INT i = (INT)lParam1;

    switch(uMsg)
    {
        case CPL_INIT:
            return TRUE;

        case CPL_GETCOUNT:
            return NUM_APPLETS;

        case CPL_INQUIRE:
            if (0 <= i && i < NUM_APPLETS)
            {
                CPLINFO *CPlInfo = (CPLINFO*)lParam2;
                CPlInfo->lData = lParam1;
                CPlInfo->idIcon = Applets[i].idIcon;
                CPlInfo->idName = Applets[i].idName;
                CPlInfo->idInfo = Applets[i].idDescription;
            }
            else
            {
                return TRUE;
            }
            break;

        case CPL_DBLCLK:
            if (0 <= i && i < NUM_APPLETS)
                Applets[i].AppletProc(hwndCpl, uMsg, lParam1, lParam2);
            else
                return TRUE;
            break;

        case CPL_STARTWPARMSW:
            if (0 <= i && i < NUM_APPLETS)
                return Applets[i].AppletProc(hwndCpl, uMsg, lParam1, lParam2);
            break;
    }

    return FALSE;
}


BOOL WINAPI
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
