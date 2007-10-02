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
#define YDEBUG
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
#include <initguid.h>
#include <devguid.h>
#include <winioctl.h>

typedef enum
{
    HWPD_STANDARDLIST = 0,
    HWPD_LARGELIST,
    HWPD_MAX = HWPD_LARGELIST
} HWPAGE_DISPLAYMODE, *PHWPAGE_DISPLAYMODE;

HWND WINAPI
DeviceCreateHardwarePageEx(HWND hWndParent,
                           LPGUID lpGuids,
                           UINT uNumberOfGuids,
                           HWPAGE_DISPLAYMODE DisplayMode);

#define DRIVE_PROPERTY_PAGES (3)

static
void
InitializeGeneralDriveDialog(HWND hwndDlg, WCHAR * szDrive)
{
    WCHAR szVolumeName[MAX_PATH+1] = {0};
   DWORD MaxComponentLength = 0;
   DWORD FileSystemFlags = 0;
   WCHAR FileSystemName[MAX_PATH+1] = {0};
   BOOL ret;
   UINT DriveType;
   ULARGE_INTEGER FreeBytesAvailable;
   ULARGE_INTEGER TotalNumberOfBytes;
   ULARGE_INTEGER TotalNumberOfFreeBytes;

   ret = GetVolumeInformationW(szDrive, szVolumeName, MAX_PATH+1, NULL, &MaxComponentLength, &FileSystemFlags, FileSystemName, MAX_PATH+1);
   if (ret)
   {
      /* set volume label */
      SendDlgItemMessageW(hwndDlg, 14001, WM_SETTEXT, (WPARAM)NULL, (LPARAM)szVolumeName);

      /* set filesystem type */
      SendDlgItemMessageW(hwndDlg, 14003, WM_SETTEXT, (WPARAM)NULL, (LPARAM)FileSystemName);

   }

   DriveType = GetDriveTypeW(szDrive);
   if (DriveType == DRIVE_FIXED)
   {
      if(GetDiskFreeSpaceExW(szDrive, &FreeBytesAvailable, &TotalNumberOfBytes, &TotalNumberOfFreeBytes))
      {
         WCHAR szResult[128];
         HANDLE hVolume;
         DWORD BytesReturned = 0;
         GET_LENGTH_INFORMATION LengthInformation;
         if (StrFormatByteSizeW(TotalNumberOfBytes.QuadPart - FreeBytesAvailable.QuadPart, szResult, sizeof(szResult) / sizeof(WCHAR)))
             SendDlgItemMessageW(hwndDlg, 14004, WM_SETTEXT, (WPARAM)NULL, (LPARAM)szResult);

         if (StrFormatByteSizeW(FreeBytesAvailable.QuadPart, szResult, sizeof(szResult) / sizeof(WCHAR)))
             SendDlgItemMessageW(hwndDlg, 14006, WM_SETTEXT, (WPARAM)NULL, (LPARAM)szResult);
#if 0
         sprintfW(szResult, L"\\\\.\\%c:", towupper(szDrive[0]));
         hVolume = CreateFileW(szResult, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
         if (hVolume != INVALID_HANDLE_VALUE)
         {
            RtlZeroMemory(&LengthInformation, sizeof(GET_LENGTH_INFORMATION));
            ret = DeviceIoControl(hVolume, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, (LPVOID)&LengthInformation, sizeof(GET_LENGTH_INFORMATION), &BytesReturned, NULL);
            if (ret && StrFormatByteSizeW(LengthInformation.Length.QuadPart, szResult, sizeof(szResult) / sizeof(WCHAR)))
               SendDlgItemMessageW(hwndDlg, 14008, WM_SETTEXT, (WPARAM)NULL, (LPARAM)szResult);

            CloseHandle(hVolume);
         }
         TRACE("szResult %s hVOlume %p ret %d LengthInformation %ul Bytesreturned %d\n", debugstr_w(szResult), hVolume, ret, LengthInformation.Length.QuadPart, BytesReturned);
#else
            if (ret && StrFormatByteSizeW(TotalNumberOfBytes.QuadPart, szResult, sizeof(szResult) / sizeof(WCHAR)))
               SendDlgItemMessageW(hwndDlg, 14008, WM_SETTEXT, (WPARAM)NULL, (LPARAM)szResult);
#endif
      }
   }
}


INT_PTR 
CALLBACK 
DriveGeneralDlg(   
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    LPPROPSHEETPAGEW ppsp;

    WCHAR * lpstr;
    switch(uMsg)
    {
    case WM_INITDIALOG:
        ppsp = (LPPROPSHEETPAGEW)lParam;
        if (ppsp == NULL)
            break;
        TRACE("WM_INITDIALOG hwnd %p lParam %p ppsplParam %S\n",hwndDlg, lParam, ppsp->lParam);

        lpstr = (WCHAR *)ppsp->lParam;
        InitializeGeneralDriveDialog(hwndDlg, lpstr);
        return TRUE;       
   }


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
    GUID Guids[1];
    Guids[0] = GUID_DEVCLASS_DISKDRIVE;

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);

    switch(uMsg)
    {
        case WM_INITDIALOG:
            /* create the hardware page */
            DeviceCreateHardwarePageEx(hwndDlg,
                                       Guids,
                                       sizeof(Guids) / sizeof(Guids[0]),
                                       0);
            break;
    }

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
