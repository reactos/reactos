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

#define LARGEINT_PROTOS
#define LargeIntegerDivide RtlLargeIntegerDivide
#define ExtendedIntegerMultiply RtlExtendedIntegerMultiply
#define ConvertUlongToLargeInteger RtlConvertUlongToLargeInteger
#define LargeIntegerSubtract RtlLargeIntegerSubtract
#define MAX_PROPERTY_SHEET_PAGE 32

#define WIN32_NO_STATUS
#define NTOS_MODE_USER
#define UNICODE
#define _UNICODE
#define COBJMACROS
#include <windows.h>
#include <ndk/ntndk.h>
#include <fmifs/fmifs.h>
#include <largeint.h>

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

typedef enum
{
    HWPD_STANDARDLIST = 0,
    HWPD_LARGELIST,
    HWPD_MAX = HWPD_LARGELIST
} HWPAGE_DISPLAYMODE, *PHWPAGE_DISPLAYMODE;

typedef
BOOLEAN 
(NTAPI *INITIALIZE_FMIFS)(
	IN PVOID hinstDll,
	IN DWORD dwReason,
	IN PVOID reserved
);
typedef
BOOLEAN
(NTAPI *QUERY_AVAILABLEFSFORMAT)(
	IN DWORD Index,
	IN OUT PWCHAR FileSystem,
	OUT UCHAR* Major,
	OUT UCHAR* Minor,
	OUT BOOLEAN* LastestVersion
);
typedef
BOOLEAN
(NTAPI *ENABLEVOLUMECOMPRESSION)(
	IN PWCHAR DriveRoot,
	IN USHORT Compression
);

typedef
VOID 
(NTAPI *FORMAT_EX)(
	IN PWCHAR DriveRoot,
	IN FMIFS_MEDIA_FLAG MediaFlag,
	IN PWCHAR Format,
	IN PWCHAR Label,
	IN BOOLEAN QuickFormat,
	IN ULONG ClusterSize,
	IN PFMIFSCALLBACK Callback
);

typedef
VOID 
(NTAPI *CHKDSK)(
	IN PWCHAR DriveRoot,
	IN PWCHAR Format,
	IN BOOLEAN CorrectErrors,
	IN BOOLEAN Verbose,
	IN BOOLEAN CheckOnlyIfDirty,
	IN BOOLEAN ScanDrive,
	IN PVOID Unused2,
	IN PVOID Unused3,
	IN PFMIFSCALLBACK Callback
);


typedef struct
{
    WCHAR   Drive;
    UINT    Options;
    HMODULE hLibrary;
    QUERY_AVAILABLEFSFORMAT QueryAvailableFileSystemFormat;
    FORMAT_EX FormatEx;
    ENABLEVOLUMECOMPRESSION EnableVolumeCompression;
    CHKDSK Chkdsk;
    UINT Result;
}FORMAT_DRIVE_CONTEXT, *PFORMAT_DRIVE_CONTEXT;

BOOL InitializeFmifsLibrary(PFORMAT_DRIVE_CONTEXT pContext);
BOOL GetDefaultClusterSize(LPWSTR szFs, PDWORD pClusterSize, PULARGE_INTEGER TotalNumberOfBytes);
HPSXA WINAPI SHCreatePropSheetExtArrayEx(HKEY hKey, LPCWSTR pszSubKey, UINT max_iface, IDataObject *pDataObj);
HWND WINAPI
DeviceCreateHardwarePageEx(HWND hWndParent,
                           LPGUID lpGuids,
                           UINT uNumberOfGuids,
                           HWPAGE_DISPLAYMODE DisplayMode);

HPROPSHEETPAGE SH_CreatePropertySheetPage(LPSTR resname, DLGPROC dlgproc, LPARAM lParam, LPWSTR szTitle);

#define DRIVE_PROPERTY_PAGES (3)

static const GUID GUID_DEVCLASS_DISKDRIVE = {0x4d36e967L, 0xe325, 0x11ce, {0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18}};


VOID
GetDriveNameWithLetter(LPWSTR szText, UINT Length, WCHAR Drive)
{
   WCHAR szDrive[] = {'C',':','\\', 0};
   DWORD dwMaxComp, dwFileSys, TempLength = 0;

    szDrive[0] = Drive;
    if (GetVolumeInformationW(szDrive, szText, Length, NULL, &dwMaxComp, &dwFileSys, NULL, 0))
    {
        szText[Length-1] = L'\0';
        TempLength = wcslen(szText);
        if (!TempLength)
        {
            /* load default volume label */
            TempLength = LoadStringW(shell32_hInstance, IDS_DRIVE_FIXED, &szText[Length+1], (sizeof(szText)/sizeof(WCHAR))- Length - 2);
        }
    }
    if (TempLength + 4 < Length)
    {
        szText[TempLength] = L' ';
        szText[TempLength+1] = L'(';
        szText[TempLength+2] = szDrive[0];
        szText[TempLength+3] = L')';
        TempLength +=4;
    }

    if (TempLength < Length)
        szText[TempLength] = L'\0';
    else
        szText[Length-1] = L'\0';
}


VOID
InitializeChkDskDialog(HWND hwndDlg, PFORMAT_DRIVE_CONTEXT pContext)
{
    WCHAR szText[100];
    UINT Length;
    SetWindowLongPtr(hwndDlg, DWLP_USER, (INT_PTR)pContext);

    Length = GetWindowTextW(hwndDlg, szText, sizeof(szText)/sizeof(WCHAR));

    GetDriveNameWithLetter(&szText[Length +1], (sizeof(szText)/sizeof(WCHAR))-Length-1, pContext->Drive);
    szText[Length] = L' ';
    szText[(sizeof(szText)/sizeof(WCHAR))-1] = L'\0';
    SetWindowText(hwndDlg, szText);
}

HWND ChkdskDrvDialog = NULL;
BOOLEAN bChkdskSuccess = FALSE;

BOOLEAN
NTAPI
ChkdskCallback(
	IN CALLBACKCOMMAND Command,
	IN ULONG SubAction,
	IN PVOID ActionInfo)
{
    PDWORD Progress;
    PBOOLEAN pSuccess;
    switch(Command)
    {
        case PROGRESS:
            Progress = (PDWORD)ActionInfo;
            SendDlgItemMessageW(ChkdskDrvDialog, 14002, PBM_SETPOS, (WPARAM)*Progress, 0);
            break;
        case DONE:
            pSuccess = (PBOOLEAN)ActionInfo;
            bChkdskSuccess = (*pSuccess);
            break;

        case VOLUMEINUSE:
        case INSUFFICIENTRIGHTS:
        case FSNOTSUPPORTED:
        case CLUSTERSIZETOOSMALL:
            bChkdskSuccess = FALSE;
            FIXME("\n");
            break;

        default:
            break;
    }

    return TRUE;
}

VOID
ChkDskNow(HWND hwndDlg, PFORMAT_DRIVE_CONTEXT pContext)
{
    DWORD ClusterSize = 0, dwMaxComponentLength, FileSystemFlags;
    WCHAR szFs[30];
    WCHAR szDrive[] = {'C',':','\\', 0};
    WCHAR szVolumeLabel[40];
    ULARGE_INTEGER TotalNumberOfFreeBytes, FreeBytesAvailableUser;
    BOOLEAN bCorrectErrors = FALSE, bScanDrive = FALSE;

    szDrive[0] = pContext->Drive;
    if(!GetVolumeInformationW(szDrive, szVolumeLabel, sizeof(szVolumeLabel)/sizeof(WCHAR), NULL, &dwMaxComponentLength, &FileSystemFlags, szFs, sizeof(szFs)/sizeof(WCHAR)))
    {
        FIXME("failed to get drive fs type\n");
        return;
    }

    if (!GetDiskFreeSpaceExW(szDrive, &FreeBytesAvailableUser, &TotalNumberOfFreeBytes, NULL))
    {
        FIXME("failed to get drive space type\n");
        return;
    }

    if (!GetDefaultClusterSize(szFs, &ClusterSize, &TotalNumberOfFreeBytes))
    {
        FIXME("invalid cluster size\n");
        return;
    }

    if (SendDlgItemMessageW(hwndDlg, 14000, BM_GETCHECK, 0, 0) == BST_CHECKED)
        bCorrectErrors = TRUE;

    if (SendDlgItemMessageW(hwndDlg, 14001, BM_GETCHECK, 0, 0) == BST_CHECKED)
        bScanDrive = TRUE;

    ChkdskDrvDialog = hwndDlg;
    bChkdskSuccess = FALSE;
    SendDlgItemMessageW(hwndDlg, 14002, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    pContext->Chkdsk(szDrive, szFs, bCorrectErrors, TRUE, FALSE, bScanDrive, NULL, NULL, ChkdskCallback);
    
    ChkdskDrvDialog = NULL;
    pContext->Result = bChkdskSuccess;
    bChkdskSuccess = FALSE;

}

INT_PTR
CALLBACK
ChkDskDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    PFORMAT_DRIVE_CONTEXT pContext;
    switch(uMsg)
    {
        case WM_INITDIALOG:
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)lParam);
            InitializeChkDskDialog(hwndDlg, (PFORMAT_DRIVE_CONTEXT)lParam);
            return TRUE;
       case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDCANCEL:
                    EndDialog(hwndDlg, 0);
                    break;
                case IDOK:
                    pContext = (PFORMAT_DRIVE_CONTEXT) GetWindowLongPtr(hwndDlg, DWLP_USER);
                    ChkDskNow(hwndDlg, pContext);
                    break;
            }
            break;
    }

    return FALSE;
}


static
LARGE_INTEGER
GetFreeBytesShare(LARGE_INTEGER TotalNumberOfFreeBytes, LARGE_INTEGER TotalNumberOfBytes)
{
   LARGE_INTEGER Temp, Result, Remainder;

   if (TotalNumberOfFreeBytes.QuadPart == 0LL)
   {
      return ConvertUlongToLargeInteger(0);
   }

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
      WCHAR szBuffer[20];

      hBlueBrush = CreateSolidBrush(RGB(0, 0, 255));
      hMagBrush = CreateSolidBrush(RGB(255, 0, 255));

      SendDlgItemMessageW(hwndDlg, 14007, WM_GETTEXT, 20, (LPARAM)szBuffer);
      Result.QuadPart = _wtoi(szBuffer);

      CopyRect(&rect, &drawItem->rcItem);
      horzsize = rect.right - rect.left;
      Result.QuadPart = (Result.QuadPart * horzsize) / 100;

      rect.right = drawItem->rcItem.right - Result.QuadPart;
      FillRect(drawItem->hDC, &rect, hBlueBrush);
      rect.left = rect.right;
      rect.right = drawItem->rcItem.right;
      FillRect(drawItem->hDC, &rect, hMagBrush);
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
   if (DriveType == DRIVE_FIXED || DriveType == DRIVE_CDROM)
   {

      if(GetDiskFreeSpaceExW(szDrive, &FreeBytesAvailable, (PULARGE_INTEGER)&TotalNumberOfBytes, (PULARGE_INTEGER)&TotalNumberOfFreeBytes))
      {
         WCHAR szResult[128];
         LARGE_INTEGER Result;
#ifdef IOCTL_DISK_GET_LENGTH_INFO_IMPLEMENTED
         HANDLE hVolume;
         DWORD BytesReturned = 0;

         swprintf(szResult, L"\\\\.\\%c:", towupper(szDrive[0]));
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
         swprintf(szResult, L"%02d%%", Result.QuadPart);
         SendDlgItemMessageW(hwndDlg, 14007, WM_SETTEXT, (WPARAM)0, (LPARAM)szResult);
         /* store used share amount */
         Result = LargeIntegerSubtract(ConvertUlongToLargeInteger(100), Result);
         swprintf(szResult, L"%02d%%", Result.QuadPart);
         SendDlgItemMessageW(hwndDlg, 14005, WM_SETTEXT, (WPARAM)0, (LPARAM)szResult);
         if (LoadStringW(shell32_hInstance, IDS_DRIVE_FIXED, szBuffer, sizeof(szBuffer) / sizeof(WCHAR)))
             SendDlgItemMessageW(hwndDlg, 14002, WM_SETTEXT, (WPARAM)0, (LPARAM)szBuffer);

      }
   }
   /* set drive description */
   SendDlgItemMessageW(hwndDlg, 14010, WM_GETTEXT, (WPARAM)50, (LPARAM)szFormat);
   swprintf(szBuffer, szFormat, szDrive);
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
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    WCHAR * lpstr;
    WCHAR szPath[MAX_PATH];
    UINT length;
    LPPSHNOTIFY lppsn;

    switch(uMsg)
    {
    case WM_INITDIALOG:
        ppsp = (LPPROPSHEETPAGEW)lParam;
        if (ppsp == NULL)
            break;
        lpstr = (WCHAR *)ppsp->lParam;
        SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)lpstr);
        InitializeGeneralDriveDialog(hwndDlg, lpstr);
        return TRUE;
    case WM_DRAWITEM:
        drawItem = (LPDRAWITEMSTRUCT)lParam;
        if (drawItem->CtlID >= 14013 && drawItem->CtlID <= 14015)
        {
            PaintStaticControls(hwndDlg, drawItem);
            return TRUE;
        }
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == 14011)
        {
           lpstr = (WCHAR*)GetWindowLongPtr(hwndDlg, DWLP_USER);
           ZeroMemory( &si, sizeof(si) );
           si.cb = sizeof(si);
           ZeroMemory( &pi, sizeof(pi) );
           if (!GetSystemDirectoryW(szPath, MAX_PATH))
              break;
           wcscat(szPath, L"\\cleanmgr.exe /D ");
           length = wcslen(szPath);
           szPath[length] = lpstr[0];
           szPath[length+1] = L'\0';
           if (CreateProcessW(NULL, szPath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
           {
              CloseHandle(pi.hProcess);
              CloseHandle(pi.hThread);
           }
           break;
        }
    case WM_NOTIFY:
        lppsn = (LPPSHNOTIFY) lParam;
        if (LOWORD(wParam) == 14001)
        {
           if (HIWORD(wParam) == EN_CHANGE)
           {
              PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
           }
           break;
        }
        if (lppsn->hdr.code == PSN_APPLY)
        {
           lpstr = (LPWSTR)GetWindowLong(hwndDlg, DWLP_USER);
           if (lpstr && SendDlgItemMessageW(hwndDlg, 14001, WM_GETTEXT, sizeof(szPath)/sizeof(WCHAR), (LPARAM)szPath))
           {
              szPath[(sizeof(szPath)/sizeof(WCHAR))-1] = L'\0';
              SetVolumeLabelW(lpstr, szPath);
           }
           SetWindowLong( hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR );
           return TRUE;
        }
        break;

    default:
        break;
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
   WCHAR szPath[MAX_PATH + 10];
   WCHAR szArg[MAX_PATH];
   WCHAR * szDrive;
   LPPROPSHEETPAGEW ppsp;
   DWORD dwSize;
   FORMAT_DRIVE_CONTEXT Context;

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

      szDrive = (WCHAR*)GetWindowLongPtr(hwndDlg, DWLP_USER);
      switch(LOWORD(wParam))
      {
         case 14000:
            if (InitializeFmifsLibrary(&Context))
            {
                Context.Drive = szDrive[0];
                DialogBoxParamW(shell32_hInstance, L"CHKDSK_DLG", hwndDlg, ChkDskDlg, (LPARAM)&Context);
                FreeLibrary(Context.hLibrary);
            }
            break;
         case 14001:
            dwSize = sizeof(szPath);
            if (RegGetValueW(HKEY_LOCAL_MACHINE, 
                             L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MyComputer\\DefragPath",
                             NULL,
                             RRF_RT_REG_EXPAND_SZ,
                             NULL,
                             (PVOID)szPath,
                             &dwSize) == S_OK)
            {
                swprintf(szArg, szPath, szDrive[0]);
               if (!GetSystemDirectoryW(szPath, MAX_PATH))
                   break;
               szDrive = PathAddBackslashW(szPath);
               if (!szDrive)
                   break;

               wcscat(szDrive, L"mmc.exe");
               if (CreateProcessW(szPath, szArg, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
               {
                  CloseHandle(pi.hProcess);
                  CloseHandle(pi.hThread);
               }
            }
            break;
         case 14002:
            dwSize = sizeof(szPath);
            if (RegGetValueW(HKEY_LOCAL_MACHINE, 
                             L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MyComputer\\BackupPath",
                             NULL,
                             RRF_RT_REG_EXPAND_SZ,
                             NULL,
                             (PVOID)szPath,
                             &dwSize) == S_OK)
            {
               if (CreateProcessW(szPath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
               {
                  CloseHandle(pi.hProcess);
                  CloseHandle(pi.hThread);
               }
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
   UINT DriveType;
} PropPages[] =
{
    { "DRIVE_GENERAL_DLG", DriveGeneralDlg, -1},
    { "DRIVE_EXTRA_DLG", DriveExtraDlg, DRIVE_FIXED},
    { "DRIVE_HARDWARE_DLG", DriveHardwareDlg, -1},
};

HRESULT
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
SH_ShowDriveProperties(WCHAR * drive, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST * apidl)
{
   HPSXA hpsx = NULL;
   HPROPSHEETPAGE hpsp[MAX_PROPERTY_SHEET_PAGE];
   PROPSHEETHEADERW psh;
   BOOL ret;
   UINT i;
   WCHAR szName[MAX_PATH+6];
   DWORD dwMaxComponent, dwFileSysFlags;
   IDataObject * pDataObj = NULL;
   UINT DriveType;

   ZeroMemory(&psh, sizeof(PROPSHEETHEADERW));
   psh.dwSize = sizeof(PROPSHEETHEADERW);
   //psh.dwFlags = PSH_USECALLBACK | PSH_PROPTITLE;
   psh.hwndParent = NULL;
   psh.nStartPage = 0;
   psh.phpage = hpsp;

   if (GetVolumeInformationW(drive, szName, sizeof(szName)/sizeof(WCHAR), NULL, &dwMaxComponent,
                             &dwFileSysFlags, NULL, 0))
   {
      psh.pszCaption = szName;
      psh.dwFlags |= PSH_PROPTITLE;
      if (!wcslen(szName))
      {
          /* FIXME
           * check if disk is a really a local hdd 
           */
          i = LoadStringW(shell32_hInstance, IDS_DRIVE_FIXED, szName, sizeof(szName)/sizeof(WCHAR)-6);
          if (i > 0 && i < (sizeof(szName)/sizeof(WCHAR)) - 6)
          {
              szName[i] = L' ';
              szName[i+1] = L'(';
              wcscpy(&szName[i+2], drive);
              szName[i+4] = L')';
              szName[i+5] = L'\0';
          }
      }
   }

   DriveType = GetDriveTypeW(drive);
   for (i = 0; i < DRIVE_PROPERTY_PAGES; i++)
   {
       if (PropPages[i].DriveType == (UINT)-1 || (PropPages[i].DriveType != (UINT)-1 &&  PropPages[i].DriveType == DriveType))
       {
           HPROPSHEETPAGE hprop = SH_CreatePropertySheetPage(PropPages[i].resname, PropPages[i].dlgproc, (LPARAM)drive, NULL);
           if (hprop)
           {
              hpsp[psh.nPages] = hprop;
              psh.nPages++;
           }
       }
   }

   if (SHCreateDataObject(pidlFolder, 1, apidl, NULL, &IID_IDataObject, (void**)&pDataObj) == S_OK)
   {
       hpsx = SHCreatePropSheetExtArrayEx(HKEY_CLASSES_ROOT, L"Drive", MAX_PROPERTY_SHEET_PAGE-DRIVE_PROPERTY_PAGES, pDataObj);
       if (hpsx)
       {
           SHAddFromPropSheetExtArray(hpsx, (LPFNADDPROPSHEETPAGE)AddPropSheetPageProc, (LPARAM)&psh);
       }
   }

   ret = PropertySheetW(&psh);
   if (pDataObj)
       IDataObject_Release(pDataObj);

   if (hpsx)
       SHDestroyPropSheetExtArray(hpsx);

   if (ret < 0)
       return FALSE;
   else
       return TRUE;
}

BOOL
GetDefaultClusterSize(LPWSTR szFs, PDWORD pClusterSize, PULARGE_INTEGER TotalNumberOfBytes)
{
    DWORD ClusterSize;

    if (!wcsicmp(szFs, L"FAT16") ||
        !wcsicmp(szFs, L"FAT")) //REACTOS HACK
    {
        if (TotalNumberOfBytes->QuadPart <= (16 * 1024 * 1024))
            ClusterSize = 2048;
        else if (TotalNumberOfBytes->QuadPart <= (32 * 1024 * 1024))
            ClusterSize = 512;
        else if (TotalNumberOfBytes->QuadPart <= (64 * 1024 * 1024))
            ClusterSize = 1024;
        else if (TotalNumberOfBytes->QuadPart <= (128 * 1024 * 1024))
            ClusterSize = 2048;
        else if (TotalNumberOfBytes->QuadPart <= (256 * 1024 * 1024))
            ClusterSize = 4096;
        else if (TotalNumberOfBytes->QuadPart <= (512 * 1024 * 1024))
            ClusterSize = 8192;
        else if (TotalNumberOfBytes->QuadPart <= (1024 * 1024 * 1024))
            ClusterSize = 16384;
        else if (TotalNumberOfBytes->QuadPart <= (2048LL * 1024LL * 1024LL))
            ClusterSize = 32768;
        else if (TotalNumberOfBytes->QuadPart <= (4096LL * 1024LL * 1024LL))
            ClusterSize = 8192;
        else 
            return FALSE;
    }
    else if (!wcsicmp(szFs, L"FAT32"))
    {
        if (TotalNumberOfBytes->QuadPart <=(64 * 1024 * 1024))
            ClusterSize = 512;
        else if (TotalNumberOfBytes->QuadPart <= (128   * 1024 * 1024))
            ClusterSize = 1024;
        else if (TotalNumberOfBytes->QuadPart <= (256   * 1024 * 1024))
            ClusterSize = 2048;
        else if (TotalNumberOfBytes->QuadPart <= (8192LL  * 1024LL * 1024LL))
            ClusterSize = 2048;
        else if (TotalNumberOfBytes->QuadPart <= (16384LL * 1024LL * 1024LL))
            ClusterSize = 8192;
        else if (TotalNumberOfBytes->QuadPart <= (32768LL * 1024LL * 1024LL))
            ClusterSize = 16384;
        else 
            return FALSE;
   }
    else if (!wcsicmp(szFs, L"NTFS"))
    {
        if (TotalNumberOfBytes->QuadPart <=(512 * 1024 * 1024))
            ClusterSize = 512;
        else if (TotalNumberOfBytes->QuadPart <= (1024 * 1024 * 1024))
            ClusterSize = 1024;
        else if (TotalNumberOfBytes->QuadPart <= (2048LL * 1024LL * 1024LL))
            ClusterSize = 2048;
        else
            ClusterSize = 2048;
   }
   else
        return FALSE;

   *pClusterSize = ClusterSize;
   return TRUE;
}


VOID
InsertDefaultClusterSizeForFs(HWND hwndDlg, PFORMAT_DRIVE_CONTEXT pContext)
{
    WCHAR szFs[100] = {0};
    WCHAR szDrive[4] = { L'C', ':', '\\', 0 };
    INT iSelIndex;
    ULARGE_INTEGER FreeBytesAvailableUser, TotalNumberOfBytes;
    DWORD ClusterSize;
    LRESULT lIndex;
    HWND hDlgCtrl;

    hDlgCtrl = GetDlgItem(hwndDlg, 28677);
    iSelIndex = SendMessage(hDlgCtrl, CB_GETCURSEL, 0, 0);
    if (iSelIndex == CB_ERR)
        return;

    if (SendMessageW(hDlgCtrl, CB_GETLBTEXT, iSelIndex, (LPARAM)szFs) == CB_ERR)
        return;

    szFs[(sizeof(szFs)/sizeof(WCHAR))-1] = L'\0';
    szDrive[0] = pContext->Drive + 'A';

    if (!GetDiskFreeSpaceExW(szDrive, &FreeBytesAvailableUser, &TotalNumberOfBytes, NULL))
        return;

    if (!wcsicmp(szFs, L"FAT16") ||
        !wcsicmp(szFs, L"FAT")) //REACTOS HACK
    {
        if (!GetDefaultClusterSize(szFs, &ClusterSize, &TotalNumberOfBytes))
        {
            TRACE("FAT16 is not supported on hdd larger than 4G current %lu\n", TotalNumberOfBytes.QuadPart);
            SendMessageW(hDlgCtrl, CB_DELETESTRING, iSelIndex, 0);
            return;
        }

        if (LoadStringW(shell32_hInstance, IDS_DEFAULT_CLUSTER_SIZE, szFs, sizeof(szFs)/sizeof(WCHAR)))
        {
            hDlgCtrl = GetDlgItem(hwndDlg, 28680);
            szFs[(sizeof(szFs)/sizeof(WCHAR))-1] = L'\0';
            SendMessageW(hDlgCtrl, CB_RESETCONTENT, 0, 0);
            lIndex = SendMessageW(hDlgCtrl, CB_ADDSTRING, 0, (LPARAM)szFs);
            if (lIndex != CB_ERR)
                SendMessageW(hDlgCtrl, CB_SETITEMDATA, lIndex, (LPARAM)ClusterSize);
            SendMessageW(hDlgCtrl, CB_SETCURSEL, 0, 0);
        }
    }
    else if (!wcsicmp(szFs, L"FAT32"))
    {
        if (!GetDefaultClusterSize(szFs, &ClusterSize, &TotalNumberOfBytes))
        {
            TRACE("FAT32 is not supported on hdd larger than 32G current %lu\n", TotalNumberOfBytes.QuadPart);
            SendMessageW(hDlgCtrl, CB_DELETESTRING, iSelIndex, 0);
            return;
        }

        if (LoadStringW(shell32_hInstance, IDS_DEFAULT_CLUSTER_SIZE, szFs, sizeof(szFs)/sizeof(WCHAR)))
        {
            hDlgCtrl = GetDlgItem(hwndDlg, 28680);
            szFs[(sizeof(szFs)/sizeof(WCHAR))-1] = L'\0';
            SendMessageW(hDlgCtrl, CB_RESETCONTENT, 0, 0);
            lIndex = SendMessageW(hDlgCtrl, CB_ADDSTRING, 0, (LPARAM)szFs);
            if (lIndex != CB_ERR)
                SendMessageW(hDlgCtrl, CB_SETITEMDATA, lIndex, (LPARAM)ClusterSize);
            SendMessageW(hDlgCtrl, CB_SETCURSEL, 0, 0);
        }
    }
    else if (!wcsicmp(szFs, L"NTFS"))
    {
        if (!GetDefaultClusterSize(szFs, &ClusterSize, &TotalNumberOfBytes))
        {
            TRACE("NTFS is not supported on hdd larger than 2TB current %lu\n", TotalNumberOfBytes.QuadPart);
            SendMessageW(hDlgCtrl, CB_DELETESTRING, iSelIndex, 0);
            return;
        }

        hDlgCtrl = GetDlgItem(hwndDlg, 28680);
        if (LoadStringW(shell32_hInstance, IDS_DEFAULT_CLUSTER_SIZE, szFs, sizeof(szFs)/sizeof(WCHAR)))
        {
            szFs[(sizeof(szFs)/sizeof(WCHAR))-1] = L'\0';
            SendMessageW(hDlgCtrl, CB_RESETCONTENT, 0, 0);
            lIndex = SendMessageW(hDlgCtrl, CB_ADDSTRING, 0, (LPARAM)szFs);
            if (lIndex != CB_ERR)
                SendMessageW(hDlgCtrl, CB_SETITEMDATA, lIndex, (LPARAM)ClusterSize);
            SendMessageW(hDlgCtrl, CB_SETCURSEL, 0, 0);
        }
        ClusterSize = 512;
        for (lIndex = 0; lIndex < 4; lIndex++)
        {
            TotalNumberOfBytes.QuadPart = ClusterSize;
            if (StrFormatByteSizeW(TotalNumberOfBytes.QuadPart, szFs, sizeof(szFs)/sizeof(WCHAR)))
            {
                lIndex = SendMessageW(hDlgCtrl, CB_ADDSTRING, 0, (LPARAM)szFs);
                if (lIndex != CB_ERR)
                    SendMessageW(hDlgCtrl, CB_SETITEMDATA, lIndex, (LPARAM)ClusterSize);
            }
            ClusterSize *= 2;
        }
    }
    else
    {
        FIXME("unknown fs\n");
        SendDlgItemMessageW(hwndDlg, 28680, CB_RESETCONTENT, iSelIndex, 0);
        return;
    }
}

VOID
InitializeFormatDriveDlg(HWND hwndDlg, PFORMAT_DRIVE_CONTEXT pContext)
{
    WCHAR szText[120];
    WCHAR szDrive[4] = { L'C', ':', '\\', 0 };
    WCHAR szFs[30] = {0};
    INT Length, TempLength;
    DWORD dwSerial, dwMaxComp, dwFileSys;
    ULARGE_INTEGER FreeBytesAvailableUser, TotalNumberOfBytes;
    DWORD dwIndex, dwDefault;
    UCHAR uMinor, uMajor;
    BOOLEAN Latest;
    HWND hDlgCtrl;

    Length = GetWindowTextW(hwndDlg, szText, sizeof(szText)/sizeof(WCHAR));
    if (Length < 0)
        Length = 0;
    szDrive[0] = pContext->Drive + L'A';
    if (GetVolumeInformationW(szDrive, &szText[Length+1], (sizeof(szText)/sizeof(WCHAR))- Length - 2, &dwSerial, &dwMaxComp, &dwFileSys, szFs, sizeof(szFs)/sizeof(WCHAR)))
    {
        szText[Length] = L' ';
        szText[(sizeof(szText)/sizeof(WCHAR))-1] = L'\0';
        TempLength = wcslen(&szText[Length+1]);
        if (!TempLength)
        {
            /* load default volume label */
            TempLength = LoadStringW(shell32_hInstance, IDS_DRIVE_FIXED, &szText[Length+1], (sizeof(szText)/sizeof(WCHAR))- Length - 2);
        }
        else
        {
            /* set volume label */
            szText[(sizeof(szText)/sizeof(WCHAR))-1] = L'\0';
            SendDlgItemMessageW(hwndDlg, 28679, WM_SETTEXT, 0, (LPARAM)&szText[Length+1]);
        }
        Length += TempLength + 1;
    }

    if (Length + 4 < (sizeof(szText)/sizeof(WCHAR)))
    {
        szText[Length] = L' ';
        szText[Length+1] = L'(';
        szText[Length+2] = szDrive[0];
        szText[Length+3] = L')';
        Length +=4;
    }

    if (Length < (sizeof(szText)/sizeof(WCHAR)))
        szText[Length] = L'\0';
    else
        szText[(sizeof(szText)/sizeof(WCHAR))-1] = L'\0';

    /* set window text */
    SetWindowTextW(hwndDlg, szText);

    if (GetDiskFreeSpaceExW(szDrive, &FreeBytesAvailableUser, &TotalNumberOfBytes, NULL))
    {
        if (StrFormatByteSizeW(TotalNumberOfBytes.QuadPart, szText, sizeof(szText)/sizeof(WCHAR)))
        {
            /* add drive capacity */
            szText[(sizeof(szText)/sizeof(WCHAR))-1] = L'\0';
            SendDlgItemMessageW(hwndDlg, 28673, CB_ADDSTRING, 0, (LPARAM)szText);
            SendDlgItemMessageW(hwndDlg, 28673, CB_SETCURSEL, 0, (LPARAM)0);
        }
    }

    if (pContext->Options & SHFMT_OPT_FULL)
    {
        /* check quick format button */
        SendDlgItemMessageW(hwndDlg, 28674, BM_SETCHECK, BST_CHECKED, 0);
    }

    /* enumerate all available filesystems */
    dwIndex = 0;
    dwDefault = 0;
    hDlgCtrl = GetDlgItem(hwndDlg, 28677);

    while(pContext->QueryAvailableFileSystemFormat(dwIndex, szText, &uMajor, &uMinor, &Latest))
    {
        szText[(sizeof(szText)/sizeof(WCHAR))-1] = L'\0';
        if (!wcsicmp(szText, szFs))
            dwDefault = dwIndex;

         SendMessageW(hDlgCtrl, CB_ADDSTRING, 0, (LPARAM)szText);
         dwIndex++;
    }

    if (!dwIndex)
    {
        ERR("no filesystem providers\n");
        return;
    }

    /* select default filesys */
    SendMessageW(hDlgCtrl, CB_SETCURSEL, dwDefault, 0);
    /* setup cluster combo */
    InsertDefaultClusterSizeForFs(hwndDlg, pContext);
    /* hide progress control */
    ShowWindow(GetDlgItem(hwndDlg, 28678), SW_HIDE);
}

HWND FormatDrvDialog = NULL;
BOOLEAN bSuccess = FALSE;


BOOLEAN
NTAPI
FormatExCB(
	IN CALLBACKCOMMAND Command,
	IN ULONG SubAction,
	IN PVOID ActionInfo)
{
    PDWORD Progress;
    PBOOLEAN pSuccess;
    switch(Command)
    {
        case PROGRESS:
            Progress = (PDWORD)ActionInfo;
            SendDlgItemMessageW(FormatDrvDialog, 28678, PBM_SETPOS, (WPARAM)*Progress, 0);
            break;
        case DONE:
            pSuccess = (PBOOLEAN)ActionInfo;
            bSuccess = (*pSuccess);
            break;

        case VOLUMEINUSE:
        case INSUFFICIENTRIGHTS:
        case FSNOTSUPPORTED:
        case CLUSTERSIZETOOSMALL:
            bSuccess = FALSE;
            FIXME("\n");
            break;

        default:
            break;
    }

    return TRUE;
}





VOID
FormatDrive(HWND hwndDlg, PFORMAT_DRIVE_CONTEXT pContext)
{
	WCHAR szDrive[4] = { L'C', ':', '\\', 0 };
    WCHAR szFileSys[40] = {0};
    WCHAR szLabel[40] = {0};
    INT iSelIndex;
    UINT Length;
    HWND hDlgCtrl;
    BOOL QuickFormat;
    DWORD ClusterSize;

    /* set volume path */
    szDrive[0] = pContext->Drive;

    /* get filesystem */
    hDlgCtrl = GetDlgItem(hwndDlg, 28677);
    iSelIndex = SendMessageW(hDlgCtrl, CB_GETCURSEL, 0, 0);
    if (iSelIndex == CB_ERR)
    {
        FIXME("\n");
        return;
    }
    Length = SendMessageW(hDlgCtrl, CB_GETLBTEXTLEN, iSelIndex, 0);
    if (Length == CB_ERR || Length + 1> sizeof(szFileSys)/sizeof(WCHAR))
    {
        FIXME("\n");
        return;
    }

    /* retrieve the file system */
    SendMessageW(hDlgCtrl, CB_GETLBTEXT, iSelIndex, (LPARAM)szFileSys);
    szFileSys[(sizeof(szFileSys)/sizeof(WCHAR))-1] = L'\0';

    /* retrieve the volume label */
    hDlgCtrl = GetWindow(hwndDlg, 28679);
    Length = SendMessageW(hDlgCtrl, WM_GETTEXTLENGTH, 0, 0);
    if (Length + 1 > sizeof(szLabel)/sizeof(WCHAR))
    {
        FIXME("\n");
        return;
    }
    SendMessageW(hDlgCtrl, WM_GETTEXT, sizeof(szLabel)/sizeof(WCHAR), (LPARAM)szLabel);
    szLabel[(sizeof(szLabel)/sizeof(WCHAR))-1] = L'\0';

    /* check for quickformat */
    if (SendDlgItemMessageW(hwndDlg, 28674, BM_GETCHECK, 0, 0) == BST_CHECKED)
        QuickFormat = TRUE;
    else
        QuickFormat = FALSE;

    /* get the cluster size */
    hDlgCtrl = GetDlgItem(hwndDlg, 28680);
    iSelIndex = SendMessageW(hDlgCtrl, CB_GETCURSEL, 0, 0);
    if (iSelIndex == CB_ERR)
    {
        FIXME("\n");
        return;
    }
    ClusterSize = SendMessageW(hDlgCtrl, CB_GETITEMDATA, iSelIndex, 0);
    if (ClusterSize == CB_ERR)
    {
        FIXME("\n");
        return;
    }

    hDlgCtrl = GetDlgItem(hwndDlg, 28680);
    ShowWindow(hDlgCtrl, SW_SHOW);
    SendMessageW(hDlgCtrl, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    bSuccess = FALSE;

    /* FIXME
     * will cause display problems
     * when performing more than one format
     */
    FormatDrvDialog = hwndDlg;

    pContext->FormatEx(szDrive,
                       FMIFS_HARDDISK, /* FIXME */
                       szFileSys,
                       szLabel,
                       QuickFormat,
                       ClusterSize,
                       FormatExCB);

    ShowWindow(hDlgCtrl, SW_HIDE);
    FormatDrvDialog = NULL;
    if (!bSuccess)
    {
        pContext->Result = SHFMT_ERROR;
    }
    else if (QuickFormat)
    {
        pContext->Result = SHFMT_OPT_FULL;
    }
    else
    {
        pContext->Result =  FALSE;
    }
}


BOOL 
CALLBACK 
FormatDriveDlg(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PFORMAT_DRIVE_CONTEXT pContext;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            InitializeFormatDriveDlg(hwndDlg, (PFORMAT_DRIVE_CONTEXT)lParam);
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)lParam);
            return TRUE;
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    pContext = (PFORMAT_DRIVE_CONTEXT)GetWindowLongPtr(hwndDlg, DWLP_USER);
                    FormatDrive(hwndDlg, pContext);
                    break;
                case IDCANCEL:
                    pContext = (PFORMAT_DRIVE_CONTEXT)GetWindowLongPtr(hwndDlg, DWLP_USER);
                    EndDialog(hwndDlg, pContext->Result);
                    break;
                case 28677: // filesystem combo
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        pContext = (PFORMAT_DRIVE_CONTEXT)GetWindowLongPtr(hwndDlg, DWLP_USER);
                        InsertDefaultClusterSizeForFs(hwndDlg, pContext);
                    }
                    break;
            }
    }
    return FALSE;
}


BOOL
InitializeFmifsLibrary(PFORMAT_DRIVE_CONTEXT pContext)
{
    INITIALIZE_FMIFS InitFmifs;
    BOOLEAN ret;
    HMODULE hLibrary;

    hLibrary = pContext->hLibrary = LoadLibraryW(L"fmifs.dll");
    if(!hLibrary)
    {
        ERR("failed to load fmifs.dll\n");
        return FALSE;
    }

    InitFmifs = (INITIALIZE_FMIFS)GetProcAddress(hLibrary, "InitializeFmIfs");
    if (!InitFmifs)
    {
        ERR("InitializeFmIfs export is missing\n");
        FreeLibrary(hLibrary);
        return FALSE;
    }

    ret = (*InitFmifs)(NULL, DLL_PROCESS_ATTACH, NULL);
    if (!ret)
    {
        ERR("fmifs failed to initialize\n");
        FreeLibrary(hLibrary);
        return FALSE;
    }

    pContext->QueryAvailableFileSystemFormat = (QUERY_AVAILABLEFSFORMAT)GetProcAddress(hLibrary, "QueryAvailableFileSystemFormat");
    if (!pContext->QueryAvailableFileSystemFormat)
    {
        ERR("QueryAvailableFileSystemFormat export is missing\n");
        FreeLibrary(hLibrary);
        return FALSE;
    }

    pContext->FormatEx = (FORMAT_EX) GetProcAddress(hLibrary, "FormatEx");
    if (!pContext->FormatEx)
    {
        ERR("FormatEx export is missing\n");
        FreeLibrary(hLibrary);
        return FALSE;
    }

    pContext->EnableVolumeCompression = (ENABLEVOLUMECOMPRESSION) GetProcAddress(hLibrary, "EnableVolumeCompression");
    if (!pContext->FormatEx)
    {
        ERR("EnableVolumeCompression export is missing\n");
        FreeLibrary(hLibrary);
        return FALSE;
    }

    pContext->Chkdsk = (CHKDSK) GetProcAddress(hLibrary, "Chkdsk");
    if (!pContext->Chkdsk)
    {
        ERR("Chkdsk export is missing\n");
        FreeLibrary(hLibrary);
        return FALSE;
    }

    return TRUE;
}

/*************************************************************************
 *              SHFormatDrive (SHELL32.@)
 */

DWORD 
WINAPI
SHFormatDrive(HWND hwnd, UINT drive, UINT fmtID, UINT options)
{
    FORMAT_DRIVE_CONTEXT Context;
    int result;

    TRACE("%p, 0x%08x, 0x%08x, 0x%08x - stub\n", hwnd, drive, fmtID, options);

    if (!InitializeFmifsLibrary(&Context))
    {
        ERR("failed to initialize fmifs\n");
        return SHFMT_NOFORMAT;
    }

    Context.Drive = drive;
    Context.Options = options;

    result = DialogBoxParamW(shell32_hInstance, L"FORMAT_DLG", hwnd, FormatDriveDlg, (LPARAM)&Context);

    FreeLibrary(Context.hLibrary);
    return result;
}


