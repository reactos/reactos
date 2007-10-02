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
#define LARGEINT_PROTOS
#define LargeIntegerDivide RtlLargeIntegerDivide
#define ExtendedIntegerMultiply RtlExtendedIntegerMultiply
#define ConvertUlongToLargeInteger RtlConvertUlongToLargeInteger
#define LargeIntegerSubtract RtlLargeIntegerSubtract
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
#include <largeint.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

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
LARGE_INTEGER
GetFreeBytesShare(LARGE_INTEGER TotalNumberOfFreeBytes, LARGE_INTEGER TotalNumberOfBytes)
{
   LARGE_INTEGER Temp, Result, Remainder;

   Temp = LargeIntegerDivide(TotalNumberOfBytes, ConvertUlongToLargeInteger(100), &Remainder);
   if (Temp.QuadPart >= TotalNumberOfFreeBytes.QuadPart)
   {
      Result = ConvertUlongToLargeInteger(1);
   }else
   {
      Result = LargeIntegerDivide(TotalNumberOfFreeBytes, Temp, &Remainder);      
   }

   return Result;
}

static
void
PaintStaticControls(HWND hwndDlg, LPDRAWITEMSTRUCT drawItem)
{
   HBRUSH hBrush;

   if (drawItem->CtlID == 14013)
   {
      hBrush = CreateSolidBrush(RGB(0, 0, 255));
      if (hBrush)
      {
         FillRect(drawItem->hDC, &drawItem->rcItem, hBrush);
         DeleteObject((HGDIOBJ)hBrush);
      }
   }else if (drawItem->CtlID == 14014)
   {
      hBrush = CreateSolidBrush(RGB(255, 0, 255));
      if (hBrush)
      {
         FillRect(drawItem->hDC, &drawItem->rcItem, hBrush);
         DeleteObject((HGDIOBJ)hBrush);
      }
   }
   else if (drawItem->CtlID == 14015)
   {
      HBRUSH hBlueBrush;
      HBRUSH hMagBrush;
      RECT rect;
      LONG horzsize;
      LARGE_INTEGER Result;

      hBlueBrush = CreateSolidBrush(RGB(0, 0, 255));
      hMagBrush = CreateSolidBrush(RGB(255, 0, 255));
      
      Result.QuadPart = GetWindowLongPtr(hwndDlg, DWLP_USER);

      CopyRect(&rect, &drawItem->rcItem);
      horzsize = rect.right - rect.left;
      Result.QuadPart = (Result.QuadPart * horzsize) / 100;

      rect.right = rect.left + Result.QuadPart;
      FillRect(drawItem->hDC, &rect, hMagBrush);
      rect.left = rect.right;
      rect.right = drawItem->rcItem.right;
      FillRect(drawItem->hDC, &rect, hBlueBrush);
      DeleteObject(hBlueBrush);
      DeleteObject(hMagBrush);
   }
}

static
void
InitializeGeneralDriveDialog(HWND hwndDlg, WCHAR * szDrive)
{
   WCHAR szVolumeName[MAX_PATH+1] = {0};
   DWORD MaxComponentLength = 0;
   DWORD FileSystemFlags = 0;
   WCHAR FileSystemName[MAX_PATH+1] = {0};
   WCHAR szFormat[50];
   WCHAR szBuffer[128];
   BOOL ret;
   UINT DriveType;
   ULARGE_INTEGER FreeBytesAvailable;
   LARGE_INTEGER TotalNumberOfFreeBytes;
   LARGE_INTEGER TotalNumberOfBytes;

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

      if(GetDiskFreeSpaceExW(szDrive, &FreeBytesAvailable, (PULARGE_INTEGER)&TotalNumberOfBytes, (PULARGE_INTEGER)&TotalNumberOfFreeBytes))
      {
         WCHAR szResult[128];
         LARGE_INTEGER Result;
#ifdef IOCTL_DISK_GET_LENGTH_INFO_IMPLEMENTED
         HANDLE hVolume;
         DWORD BytesReturned = 0;

         sprintfW(szResult, L"\\\\.\\%c:", towupper(szDrive[0]));
         hVolume = CreateFileW(szResult, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
         if (hVolume != INVALID_HANDLE_VALUE)
         {
            ret = DeviceIoControl(hVolume, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, (LPVOID)&TotalNumberOfBytes, sizeof(ULARGE_INTEGER), &BytesReturned, NULL);
            if (ret && StrFormatByteSizeW(LengthInformation.Length.QuadPart, szResult, sizeof(szResult) / sizeof(WCHAR)))
               SendDlgItemMessageW(hwndDlg, 14008, WM_SETTEXT, (WPARAM)NULL, (LPARAM)szResult);

            CloseHandle(hVolume);
         }
         TRACE("szResult %s hVOlume %p ret %d LengthInformation %ul Bytesreturned %d\n", debugstr_w(szResult), hVolume, ret, LengthInformation.Length.QuadPart, BytesReturned);
#else
            if (ret && StrFormatByteSizeW(TotalNumberOfBytes.QuadPart, szResult, sizeof(szResult) / sizeof(WCHAR)))
               SendDlgItemMessageW(hwndDlg, 14008, WM_SETTEXT, (WPARAM)NULL, (LPARAM)szResult);
#endif

         if (StrFormatByteSizeW(TotalNumberOfBytes.QuadPart - FreeBytesAvailable.QuadPart, szResult, sizeof(szResult) / sizeof(WCHAR)))
             SendDlgItemMessageW(hwndDlg, 14004, WM_SETTEXT, (WPARAM)NULL, (LPARAM)szResult);

         if (StrFormatByteSizeW(FreeBytesAvailable.QuadPart, szResult, sizeof(szResult) / sizeof(WCHAR)))
             SendDlgItemMessageW(hwndDlg, 14006, WM_SETTEXT, (WPARAM)NULL, (LPARAM)szResult);

         Result = GetFreeBytesShare(TotalNumberOfFreeBytes, TotalNumberOfBytes);
         /* set free bytes percentage */
         sprintfW(szResult, L"%02d%%", Result.QuadPart);
         SendDlgItemMessageW(hwndDlg, 14007, WM_SETTEXT, (WPARAM)0, (LPARAM)szResult);
         /* store free share amount */
         SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)Result.QuadPart);
         /* store used share amount */
         Result = LargeIntegerSubtract(ConvertUlongToLargeInteger(100), Result);
         sprintfW(szResult, L"%02d%%", Result.QuadPart);
         SendDlgItemMessageW(hwndDlg, 14005, WM_SETTEXT, (WPARAM)0, (LPARAM)szResult);
         if (LoadStringW(shell32_hInstance, IDS_DRIVE_FIXED, szBuffer, sizeof(szBuffer) / sizeof(WCHAR)))
             SendDlgItemMessageW(hwndDlg, 14002, WM_SETTEXT, (WPARAM)0, (LPARAM)szBuffer);

      }
   }
   /* set drive description */
   SendDlgItemMessageW(hwndDlg, 14010, WM_GETTEXT, (WPARAM)50, (LPARAM)szFormat);
   sprintfW(szBuffer, szFormat, szDrive);
   SendDlgItemMessageW(hwndDlg, 14010, WM_SETTEXT, (WPARAM)NULL, (LPARAM)szBuffer);
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
	LPDRAWITEMSTRUCT drawItem;

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
	case WM_DRAWITEM:
		drawItem = (LPDRAWITEMSTRUCT)lParam;
	    if (drawItem->CtlID >= 14013 && drawItem->CtlID <= 14015)
        {
			PaintStaticControls(hwndDlg, drawItem);
            return TRUE;
        }

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
   STARTUPINFOW si;
   PROCESS_INFORMATION pi;
   WCHAR szPath[MAX_PATH];
   WCHAR szArg[MAX_PATH];
   WCHAR * szDrive;
   UINT length;
   LPPROPSHEETPAGEW ppsp;

   switch (uMsg)
   {
   case WM_INITDIALOG:
      ppsp = (LPPROPSHEETPAGEW)lParam;
      SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)ppsp->lParam);
      return TRUE;
   case WM_COMMAND:
      ZeroMemory( &si, sizeof(si) );
      si.cb = sizeof(si);
      ZeroMemory( &pi, sizeof(pi) );
      if (!GetSystemDirectoryW(szPath, MAX_PATH))
          break;
      szDrive = (WCHAR*)GetWindowLongPtr(hwndDlg, DWLP_USER);
      switch(LOWORD(wParam))
      {
         case 14000:
            ///
            /// FIXME
            /// show checkdsk dialog
            ///
            break;
         case 14001:
            szArg[0] = L'"';
            wcscpy(&szArg[1], szPath);
            wcscat(szPath, L"\\mmc.exe");
            wcscat(szArg, L"\\dfrg.msc\" ");
            length = wcslen(szArg);
            szArg[length] = szDrive[0];
            szArg[length+1] = L':';
            szArg[length+2] = L'\0';
            if (CreateProcessW(szPath, szArg, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
            {
               CloseHandle(pi.hProcess);
               CloseHandle(pi.hThread);
            }
            break;
         case 14002:
            wcscat(szPath, L"\\ntbackup.exe");
            if (CreateProcessW(szPath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
            {
               CloseHandle(pi.hProcess);
               CloseHandle(pi.hThread);
            }
      }
      break;
   }
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
