/*
 *                 Shell Library Functions
 *
 * Copyright 2005 Johannes Anderwald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"
#include "commdlg.h"
#include "wine/debug.h"

#include "shellapi.h"
#include <shlwapi.h>
#include "shlobj.h"
#include "shell32_main.h"
#include "shresdef.h"
#include "undocshell.h"
#include <prsht.h>

#define DRIVE_PROPERTY_PAGES (3)

INT_PTR 
CALLBACK 
DriveGeneralDlg(   
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{



   return FALSE;
}

INT_PTR 
CALLBACK 
DriveExtraDlg(   
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{

   return FALSE;
}

INT_PTR 
CALLBACK 
DriveHardwareDlg(   
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{


  return FALSE;
}

static 
const
struct
{
   LPSTR resname;
   DLGPROC dlgproc;
} PropPages[] =
{
    { "DRIVE_GENERAL_DLG", DriveGeneralDlg },
    { "DRIVE_EXTRA_DLG", DriveExtraDlg },
    { "DRIVE_HARDWARE_DLG", DriveHardwareDlg },
};

BOOL
CALLBACK
AddPropSheetPageProc(HPROPSHEETPAGE hpage, LPARAM lParam)
{
    PROPSHEETHEADER *ppsh = (PROPSHEETHEADER *)lParam;
    if (ppsh != NULL && ppsh->nPages < MAX_PROPERTY_SHEET_PAGE)
    {
        ppsh->phpage[ppsh->nPages++] = hpage;
        return TRUE;
    }
    return FALSE;
}

BOOL
SH_ShowDriveProperties(WCHAR * drive)
{
   HPSXA hpsx;
   HPROPSHEETPAGE hpsp[MAX_PROPERTY_SHEET_PAGE];
   PROPSHEETHEADERW psh;
   BOOL ret;
   UINT i;

   ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
   psh.dwSize = sizeof(PROPSHEETHEADER);
   //psh.dwFlags = PSH_USECALLBACK | PSH_PROPTITLE;
   psh.hwndParent = NULL;
   psh.nStartPage = 0;
   psh.phpage = hpsp;

   for (i = 0; i < DRIVE_PROPERTY_PAGES; i++)
   {
       HPROPSHEETPAGE hprop = SH_CreatePropertySheetPage(PropPages[i].resname, PropPages[i].dlgproc, (LPARAM)drive);
       if (hprop)
       {
          hpsp[psh.nPages] = hprop;
          psh.nPages++;
       }
   }

   hpsx = SHCreatePropSheetExtArray(HKEY_CLASSES_ROOT,
                                    L"Drive",
                                    MAX_PROPERTY_SHEET_PAGE-DRIVE_PROPERTY_PAGES);

   SHAddFromPropSheetExtArray(hpsx,
                              (LPFNADDPROPSHEETPAGE)AddPropSheetPageProc,
                              (LPARAM)&psh);

   ret = PropertySheetW(&psh);
   if (ret < 0)
       return FALSE;
   else
       return TRUE;
}
