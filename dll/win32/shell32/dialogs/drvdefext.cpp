/*
 * Provides default drive shell extension
 *
 * Copyright 2005 Johannes Anderwald
 * Copyright 2012 Rafal Harabien
 * Copyright 2020 Katayama Hirofumi MZ
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "precomp.h"

#define _USE_MATH_DEFINES
#include <math.h>

#define NTOS_MODE_USER
#include <ndk/iofuncs.h>
#include <ndk/obfuncs.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

static const GUID GUID_DEVCLASS_DISKDRIVE = {0x4d36e967L, 0xe325, 0x11ce, {0xbf, 0xc1, 0x08, 0x00, 0x2b, 0xe1, 0x03, 0x18}};

typedef enum
{
    HWPD_STANDARDLIST = 0,
    HWPD_LARGELIST,
    HWPD_MAX = HWPD_LARGELIST
} HWPAGE_DISPLAYMODE, *PHWPAGE_DISPLAYMODE;

EXTERN_C HWND WINAPI
DeviceCreateHardwarePageEx(HWND hWndParent,
                           LPGUID lpGuids,
                           UINT uNumberOfGuids,
                           HWPAGE_DISPLAYMODE DisplayMode);
UINT SH_FormatByteSize(LONGLONG cbSize, LPWSTR pwszResult, UINT cchResultMax);

static VOID
GetDriveNameWithLetter(LPWSTR pwszText, UINT cchTextMax, LPCWSTR pwszDrive)
{
    DWORD dwMaxComp, dwFileSys;
    SIZE_T cchText = 0;

    if (GetVolumeInformationW(pwszDrive, pwszText, cchTextMax, NULL, &dwMaxComp, &dwFileSys, NULL, 0))
    {
        cchText = wcslen(pwszText);
        if (cchText == 0)
        {
            /* load default volume label */
            cchText = LoadStringW(shell32_hInstance, IDS_DRIVE_FIXED, pwszText, cchTextMax);
        }
    }

    StringCchPrintfW(pwszText + cchText, cchTextMax - cchText, L" (%c:)", pwszDrive[0]);
}

static VOID
InitializeChkDskDialog(HWND hwndDlg, LPCWSTR pwszDrive)
{
    WCHAR wszText[100];
    UINT Length;
    SetWindowLongPtr(hwndDlg, DWLP_USER, (INT_PTR)pwszDrive);

    Length = GetWindowTextW(hwndDlg, wszText, sizeof(wszText) / sizeof(WCHAR));
    wszText[Length] = L' ';
    GetDriveNameWithLetter(&wszText[Length + 1], (sizeof(wszText) / sizeof(WCHAR)) - Length - 1, pwszDrive);
    SetWindowText(hwndDlg, wszText);
}

static HWND hChkdskDrvDialog = NULL;
static BOOLEAN bChkdskSuccess = FALSE;

static BOOLEAN NTAPI
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
            SendDlgItemMessageW(hChkdskDrvDialog, 14002, PBM_SETPOS, (WPARAM)*Progress, 0);
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

static VOID
ChkDskNow(HWND hwndDlg, LPCWSTR pwszDrive)
{
    //DWORD ClusterSize = 0;
    WCHAR wszFs[30];
    ULARGE_INTEGER TotalNumberOfFreeBytes, FreeBytesAvailableUser;
    BOOLEAN bCorrectErrors = FALSE, bScanDrive = FALSE;

    if(!GetVolumeInformationW(pwszDrive, NULL, 0, NULL, NULL, NULL, wszFs, _countof(wszFs)))
    {
        FIXME("failed to get drive fs type\n");
        return;
    }

    if (!GetDiskFreeSpaceExW(pwszDrive, &FreeBytesAvailableUser, &TotalNumberOfFreeBytes, NULL))
    {
        FIXME("failed to get drive space type\n");
        return;
    }

    /*if (!GetDefaultClusterSize(wszFs, &ClusterSize, &TotalNumberOfFreeBytes))
    {
        FIXME("invalid cluster size\n");
        return;
    }*/

    if (SendDlgItemMessageW(hwndDlg, 14000, BM_GETCHECK, 0, 0) == BST_CHECKED)
        bCorrectErrors = TRUE;

    if (SendDlgItemMessageW(hwndDlg, 14001, BM_GETCHECK, 0, 0) == BST_CHECKED)
        bScanDrive = TRUE;

    hChkdskDrvDialog = hwndDlg;
    bChkdskSuccess = FALSE;
    SendDlgItemMessageW(hwndDlg, 14002, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    Chkdsk((LPWSTR)pwszDrive, (LPWSTR)wszFs, bCorrectErrors, TRUE, FALSE, bScanDrive, NULL, NULL, ChkdskCallback); // FIXME: casts

    hChkdskDrvDialog = NULL;
    bChkdskSuccess = FALSE;
}

static INT_PTR CALLBACK
ChkDskDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)lParam);
            InitializeChkDskDialog(hwndDlg, (LPCWSTR)lParam);
            return TRUE;
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDCANCEL:
                    EndDialog(hwndDlg, 0);
                    break;
                case IDOK:
                {
                    LPCWSTR pwszDrive = (LPCWSTR)GetWindowLongPtr(hwndDlg, DWLP_USER);
                    ChkDskNow(hwndDlg, pwszDrive);
                    break;
                }
            }
            break;
    }

    return FALSE;
}

VOID
CDrvDefExt::PaintStaticControls(HWND hwndDlg, LPDRAWITEMSTRUCT pDrawItem)
{
    HBRUSH hBrush;

    if (pDrawItem->CtlID == 14013)
    {
        hBrush = CreateSolidBrush(RGB(0, 0, 255));
        if (hBrush)
        {
            FillRect(pDrawItem->hDC, &pDrawItem->rcItem, hBrush);
            DeleteObject((HGDIOBJ)hBrush);
        }
    }
    else if (pDrawItem->CtlID == 14014)
    {
        hBrush = CreateSolidBrush(RGB(255, 0, 255));
        if (hBrush)
        {
            FillRect(pDrawItem->hDC, &pDrawItem->rcItem, hBrush);
            DeleteObject((HGDIOBJ)hBrush);
        }
    }
    else if (pDrawItem->CtlID == 14015)
    {
        HBRUSH hBlueBrush = CreateSolidBrush(RGB(0, 0, 255));
        HBRUSH hMagBrush = CreateSolidBrush(RGB(255, 0, 255));
        HBRUSH hbrOld;
        HPEN hDarkBluePen = CreatePen(PS_SOLID, 1, RGB(0, 0, 128));
        HPEN hDarkMagPen = CreatePen(PS_SOLID, 1, RGB(128, 0, 128));
        HPEN hOldPen = (HPEN)SelectObject(pDrawItem->hDC, hDarkMagPen);
        INT xCenter = (pDrawItem->rcItem.left + pDrawItem->rcItem.right) / 2;
        INT yCenter = (pDrawItem->rcItem.top + pDrawItem->rcItem.bottom - 10) / 2;
        INT cx = pDrawItem->rcItem.right - pDrawItem->rcItem.left;
        INT cy = pDrawItem->rcItem.bottom - pDrawItem->rcItem.top - 10;
        INT xRadial = xCenter + (INT)(cos(M_PI + m_FreeSpacePerc / 100.0f * M_PI * 2.0f) * cx / 2);
        INT yRadial = yCenter - (INT)(sin(M_PI + m_FreeSpacePerc / 100.0f * M_PI * 2.0f) * cy / 2);

        TRACE("FreeSpace %u a %f cx %d\n", m_FreeSpacePerc, M_PI+m_FreeSpacePerc / 100.0f * M_PI * 2.0f, cx);

        for (INT x = pDrawItem->rcItem.left; x < pDrawItem->rcItem.right; ++x)
        {
            double cos_val = (x - xCenter) * 2.0f / cx;
            INT y = yCenter + (INT)(sin(acos(cos_val)) * cy / 2) - 1;

            if (m_FreeSpacePerc < 50 && x == xRadial)
                SelectObject(pDrawItem->hDC, hDarkBluePen);

            MoveToEx(pDrawItem->hDC, x, y, NULL);
            LineTo(pDrawItem->hDC, x, y + 10);
        }

        SelectObject(pDrawItem->hDC, hOldPen);

        if (m_FreeSpacePerc > 50)
        {
            hbrOld = (HBRUSH)SelectObject(pDrawItem->hDC, hMagBrush);

            Ellipse(pDrawItem->hDC, pDrawItem->rcItem.left, pDrawItem->rcItem.top,
                    pDrawItem->rcItem.right, pDrawItem->rcItem.bottom - 10);

            SelectObject(pDrawItem->hDC, hBlueBrush);

            if (m_FreeSpacePerc < 100)
            {
                Pie(pDrawItem->hDC, pDrawItem->rcItem.left, pDrawItem->rcItem.top, pDrawItem->rcItem.right,
                    pDrawItem->rcItem.bottom - 10, xRadial, yRadial, pDrawItem->rcItem.left, yCenter);
            }
        }
        else
        {
            hbrOld = (HBRUSH)SelectObject(pDrawItem->hDC, hBlueBrush);

            Ellipse(pDrawItem->hDC, pDrawItem->rcItem.left, pDrawItem->rcItem.top,
                    pDrawItem->rcItem.right, pDrawItem->rcItem.bottom - 10);

            SelectObject(pDrawItem->hDC, hMagBrush);

            if (m_FreeSpacePerc > 0)
            {
                Pie(pDrawItem->hDC, pDrawItem->rcItem.left, pDrawItem->rcItem.top, pDrawItem->rcItem.right,
                    pDrawItem->rcItem.bottom - 10, pDrawItem->rcItem.left, yCenter, xRadial, yRadial);
            }
        }

        SelectObject(pDrawItem->hDC, hbrOld);

        DeleteObject(hBlueBrush);
        DeleteObject(hMagBrush);
        DeleteObject(hDarkBluePen);
        DeleteObject(hDarkMagPen);
    }
}

// https://stackoverflow.com/questions/3098696/get-information-about-disk-drives-result-on-windows7-32-bit-system/3100268#3100268
static BOOL
GetDriveTypeAndCharacteristics(HANDLE hDevice, DEVICE_TYPE *pDeviceType, ULONG *pCharacteristics)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_FS_DEVICE_INFORMATION DeviceInfo;

    Status = NtQueryVolumeInformationFile(hDevice, &IoStatusBlock,
                                          &DeviceInfo, sizeof(DeviceInfo),
                                          FileFsDeviceInformation);
    if (Status == NO_ERROR)
    {
        *pDeviceType = DeviceInfo.DeviceType;
        *pCharacteristics = DeviceInfo.Characteristics;
        return TRUE;
    }

    return FALSE;
}

BOOL IsDriveFloppyW(LPCWSTR pszDriveRoot)
{
    LPCWSTR RootPath = pszDriveRoot;
    WCHAR szRoot[16], szDeviceName[16];
    UINT uType;
    HANDLE hDevice;
    DEVICE_TYPE DeviceType;
    ULONG ulCharacteristics;
    BOOL ret;

    lstrcpynW(szRoot, RootPath, _countof(szRoot));

    if (L'a' <= szRoot[0] && szRoot[0] <= 'z')
    {
        szRoot[0] += ('A' - 'a');
    }

    if ('A' <= szRoot[0] && szRoot[0] <= L'Z' &&
        szRoot[1] == L':' && szRoot[2] == 0)
    {
        // 'C:' --> 'C:\'
        szRoot[2] = L'\\';
        szRoot[3] = 0;
    }

    if (!PathIsRootW(szRoot))
    {
        return FALSE;
    }

    uType = GetDriveTypeW(szRoot);
    if (uType == DRIVE_REMOVABLE)
    {
        if (szRoot[0] == L'A' || szRoot[0] == L'B')
            return TRUE;
    }
    else
    {
        return FALSE;
    }

    lstrcpynW(szDeviceName, L"\\\\.\\", _countof(szDeviceName));
    szDeviceName[4] = szRoot[0];
    szDeviceName[5] = L':';
    szDeviceName[6] = UNICODE_NULL;

    hDevice = CreateFileW(szDeviceName, FILE_READ_ATTRIBUTES,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    ret = FALSE;
    if (GetDriveTypeAndCharacteristics(hDevice, &DeviceType, &ulCharacteristics))
    {
        if ((ulCharacteristics & FILE_FLOPPY_DISKETTE) == FILE_FLOPPY_DISKETTE)
            ret = TRUE;
    }

    CloseHandle(hDevice);

    return ret;
}

BOOL IsDriveFloppyA(LPCSTR pszDriveRoot)
{
    WCHAR szRoot[8];
    MultiByteToWideChar(CP_ACP, 0, pszDriveRoot, -1, szRoot, _countof(szRoot));
    return IsDriveFloppyW(szRoot);
}

VOID
CDrvDefExt::InitGeneralPage(HWND hwndDlg)
{
    WCHAR wszVolumeName[MAX_PATH+1] = {0};
    WCHAR wszFileSystem[MAX_PATH+1] = {0};
    WCHAR wszBuf[128];
    BOOL bRet;

    bRet = GetVolumeInformationW(m_wszDrive, wszVolumeName, _countof(wszVolumeName), NULL, NULL, NULL, wszFileSystem, _countof(wszFileSystem));
    if (bRet)
    {
        /* Set volume label and filesystem */
        SetDlgItemTextW(hwndDlg, 14000, wszVolumeName);
        SetDlgItemTextW(hwndDlg, 14002, wszFileSystem);
    }
    else
    {
        LoadStringW(shell32_hInstance, IDS_FS_UNKNOWN, wszFileSystem, _countof(wszFileSystem));
        SetDlgItemTextW(hwndDlg, 14002, wszFileSystem);
    }

    /* Set drive type and icon */
    UINT DriveType = GetDriveTypeW(m_wszDrive);
    UINT IconId, TypeStrId = 0;
    switch (DriveType)
    {
        case DRIVE_REMOVABLE:
            if (IsDriveFloppyW(m_wszDrive))
                IconId = IDI_SHELL_3_14_FLOPPY;
            else
                IconId = IDI_SHELL_REMOVEABLE;
            break;
        case DRIVE_CDROM: IconId = IDI_SHELL_CDROM; TypeStrId = IDS_DRIVE_CDROM; break;
        case DRIVE_REMOTE: IconId = IDI_SHELL_NETDRIVE; TypeStrId = IDS_DRIVE_NETWORK; break;
        case DRIVE_RAMDISK: IconId = IDI_SHELL_RAMDISK; break;
        default: IconId = IDI_SHELL_DRIVE; TypeStrId = IDS_DRIVE_FIXED;
    }

    if (DriveType == DRIVE_CDROM || DriveType == DRIVE_REMOTE)
    {
        /* volume label textbox */
        SendMessage(GetDlgItem(hwndDlg, 14000), EM_SETREADONLY, TRUE, 0);

        /* disk compression */
        ShowWindow(GetDlgItem(hwndDlg, 14011), FALSE);

        /* index */
        ShowWindow(GetDlgItem(hwndDlg, 14012), FALSE);
    }

    HICON hIcon = (HICON)LoadImage(shell32_hInstance, MAKEINTRESOURCE(IconId), IMAGE_ICON, 32, 32, LR_SHARED);
    if (hIcon)
        SendDlgItemMessageW(hwndDlg, 14016, STM_SETICON, (WPARAM)hIcon, 0);
    if (TypeStrId && LoadStringW(shell32_hInstance, TypeStrId, wszBuf, _countof(wszBuf)))
        SetDlgItemTextW(hwndDlg, 14001, wszBuf);

    ULARGE_INTEGER FreeBytesAvailable, TotalNumberOfBytes;
    if(GetDiskFreeSpaceExW(m_wszDrive, &FreeBytesAvailable, &TotalNumberOfBytes, NULL))
    {
        /* Init free space percentage used for drawing piechart */
        m_FreeSpacePerc = (UINT)(FreeBytesAvailable.QuadPart * 100ull / TotalNumberOfBytes.QuadPart);

        /* Used space */
        if (SH_FormatByteSize(TotalNumberOfBytes.QuadPart - FreeBytesAvailable.QuadPart, wszBuf, _countof(wszBuf)))
            SetDlgItemTextW(hwndDlg, 14003, wszBuf);

        if (StrFormatByteSizeW(TotalNumberOfBytes.QuadPart - FreeBytesAvailable.QuadPart, wszBuf, _countof(wszBuf)))
            SetDlgItemTextW(hwndDlg, 14004, wszBuf);

        /* Free space */
        if (SH_FormatByteSize(FreeBytesAvailable.QuadPart, wszBuf, _countof(wszBuf)))
            SetDlgItemTextW(hwndDlg, 14005, wszBuf);

        if (StrFormatByteSizeW(FreeBytesAvailable.QuadPart, wszBuf, _countof(wszBuf)))
            SetDlgItemTextW(hwndDlg, 14006, wszBuf);

        /* Total space */
        if (SH_FormatByteSize(TotalNumberOfBytes.QuadPart, wszBuf, _countof(wszBuf)))
            SetDlgItemTextW(hwndDlg, 14007, wszBuf);

        if (StrFormatByteSizeW(TotalNumberOfBytes.QuadPart, wszBuf, _countof(wszBuf)))
            SetDlgItemTextW(hwndDlg, 14008, wszBuf);
    }
    else
    {
        m_FreeSpacePerc = 0;

        if (SH_FormatByteSize(0, wszBuf, _countof(wszBuf)))
        {
            SetDlgItemTextW(hwndDlg, 14003, wszBuf);
            SetDlgItemTextW(hwndDlg, 14005, wszBuf);
            SetDlgItemTextW(hwndDlg, 14007, wszBuf);
        }
        if (StrFormatByteSizeW(0, wszBuf, _countof(wszBuf)))
        {
            SetDlgItemTextW(hwndDlg, 14004, wszBuf);
            SetDlgItemTextW(hwndDlg, 14006, wszBuf);
            SetDlgItemTextW(hwndDlg, 14008, wszBuf);
        }
    }

    /* Set drive description */
    WCHAR wszFormat[50];
    GetDlgItemTextW(hwndDlg, 14009, wszFormat, _countof(wszFormat));
    swprintf(wszBuf, wszFormat, m_wszDrive[0]);
    SetDlgItemTextW(hwndDlg, 14009, wszBuf);

    /* show disk cleanup button only for fixed drives */
    ShowWindow(GetDlgItem(hwndDlg, 14010), DriveType == DRIVE_FIXED);
}

INT_PTR CALLBACK
CDrvDefExt::GeneralPageProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            LPPROPSHEETPAGEW ppsp = (LPPROPSHEETPAGEW)lParam;
            if (ppsp == NULL)
                break;

            CDrvDefExt *pDrvDefExt = reinterpret_cast<CDrvDefExt *>(ppsp->lParam);
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pDrvDefExt);
            pDrvDefExt->InitGeneralPage(hwndDlg);
            return TRUE;
        }
        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT pDrawItem = (LPDRAWITEMSTRUCT)lParam;

            if (pDrawItem->CtlID >= 14013 && pDrawItem->CtlID <= 14015)
            {
                CDrvDefExt *pDrvDefExt = reinterpret_cast<CDrvDefExt *>(GetWindowLongPtr(hwndDlg, DWLP_USER));
                pDrvDefExt->PaintStaticControls(hwndDlg, pDrawItem);
                return TRUE;
            }
            break;
        }
        case WM_PAINT:
            break;
        case WM_COMMAND:
            if (LOWORD(wParam) == 14010) /* Disk Cleanup */
            {
                CDrvDefExt *pDrvDefExt = reinterpret_cast<CDrvDefExt *>(GetWindowLongPtr(hwndDlg, DWLP_USER));
                WCHAR wszBuf[256];
                DWORD cbBuf = sizeof(wszBuf);

                if (RegGetValueW(HKEY_LOCAL_MACHINE,
                                 L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MyComputer\\CleanupPath",
                                 NULL,
                                 RRF_RT_REG_SZ,
                                 NULL,
                                 (PVOID)wszBuf,
                                 &cbBuf) == ERROR_SUCCESS)
                {
                    WCHAR wszCmd[MAX_PATH];

                    StringCbPrintfW(wszCmd, sizeof(wszCmd), wszBuf, pDrvDefExt->m_wszDrive[0]);

                    if (ShellExecuteW(hwndDlg, NULL, wszCmd, NULL, NULL, SW_SHOW) <= (HINSTANCE)32)
                        ERR("Failed to create cleanup process %ls\n", wszCmd);
                }
            }
            else if (LOWORD(wParam) == 14000) /* Label */
            {
                if (HIWORD(wParam) == EN_CHANGE)
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            }
            break;
        case WM_NOTIFY:
            if (((LPNMHDR)lParam)->hwndFrom == GetParent(hwndDlg))
            {
                /* Property Sheet */
                LPPSHNOTIFY lppsn = (LPPSHNOTIFY)lParam;

                if (lppsn->hdr.code == PSN_APPLY)
                {
                    CDrvDefExt *pDrvDefExt = reinterpret_cast<CDrvDefExt *>(GetWindowLongPtr(hwndDlg, DWLP_USER));
                    WCHAR wszBuf[256];

                    if (GetDlgItemTextW(hwndDlg, 14000, wszBuf, _countof(wszBuf)))
                        SetVolumeLabelW(pDrvDefExt->m_wszDrive, wszBuf);
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                    return TRUE;
                }
            }
            break;

        default:
            break;
    }

    return FALSE;
}

INT_PTR CALLBACK
CDrvDefExt::ExtraPageProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            LPPROPSHEETPAGEW ppsp = (LPPROPSHEETPAGEW)lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)ppsp->lParam);
            return TRUE;
        }
        case WM_COMMAND:
        {
            WCHAR wszBuf[MAX_PATH];
            DWORD cbBuf = sizeof(wszBuf);
            CDrvDefExt *pDrvDefExt = reinterpret_cast<CDrvDefExt *>(GetWindowLongPtr(hwndDlg, DWLP_USER));

            switch(LOWORD(wParam))
            {
                case 14000:
                    DialogBoxParamW(shell32_hInstance, MAKEINTRESOURCEW(IDD_CHECK_DISK), hwndDlg, ChkDskDlg, (LPARAM)pDrvDefExt->m_wszDrive);
                    break;
                case 14001:
                    if (RegGetValueW(HKEY_LOCAL_MACHINE,
                                     L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MyComputer\\DefragPath",
                                     NULL,
                                     RRF_RT_REG_SZ,
                                     NULL,
                                     (PVOID)wszBuf,
                                     &cbBuf) == ERROR_SUCCESS)
                    {
                        WCHAR wszCmd[MAX_PATH];

                        StringCbPrintfW(wszCmd, sizeof(wszCmd), wszBuf, pDrvDefExt->m_wszDrive[0]);

                        if (ShellExecuteW(hwndDlg, NULL, wszCmd, NULL, NULL, SW_SHOW) <= (HINSTANCE)32)
                            ERR("Failed to create defrag process %ls\n", wszCmd);
                    }
                    break;
                case 14002:
                    if (RegGetValueW(HKEY_LOCAL_MACHINE,
                                     L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MyComputer\\BackupPath",
                                     NULL,
                                     RRF_RT_REG_SZ,
                                     NULL,
                                     (PVOID)wszBuf,
                                     &cbBuf) == ERROR_SUCCESS)
                    {
                        if (ShellExecuteW(hwndDlg, NULL, wszBuf, NULL, NULL, SW_SHOW) <= (HINSTANCE)32)
                            ERR("Failed to create backup process %ls\n", wszBuf);
                    }
            }
            break;
        }
    }
    return FALSE;
}

INT_PTR CALLBACK
CDrvDefExt::HardwarePageProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);

    switch(uMsg)
    {
        case WM_INITDIALOG:
        {
            GUID Guid = GUID_DEVCLASS_DISKDRIVE;

            /* create the hardware page */
            DeviceCreateHardwarePageEx(hwndDlg, &Guid, 1, HWPD_STANDARDLIST);
            break;
        }
    }

    return FALSE;
}

CDrvDefExt::CDrvDefExt()
{
    m_wszDrive[0] = L'\0';
}

CDrvDefExt::~CDrvDefExt()
{

}

HRESULT WINAPI
CDrvDefExt::Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject *pDataObj, HKEY hkeyProgID)
{
    FORMATETC format;
    STGMEDIUM stgm;
    HRESULT hr;

    TRACE("%p %p %p %p\n", this, pidlFolder, pDataObj, hkeyProgID);

    if (!pDataObj)
        return E_FAIL;

    format.cfFormat = CF_HDROP;
    format.ptd = NULL;
    format.dwAspect = DVASPECT_CONTENT;
    format.lindex = -1;
    format.tymed = TYMED_HGLOBAL;

    hr = pDataObj->GetData(&format, &stgm);
    if (FAILED(hr))
        return hr;

    if (!DragQueryFileW((HDROP)stgm.hGlobal, 0, m_wszDrive, _countof(m_wszDrive)))
    {
        ERR("DragQueryFileW failed\n");
        ReleaseStgMedium(&stgm);
        return E_FAIL;
    }

    ReleaseStgMedium(&stgm);
    TRACE("Drive properties %ls\n", m_wszDrive);

    return S_OK;
}

HRESULT WINAPI
CDrvDefExt::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI
CDrvDefExt::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI
CDrvDefExt::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI
CDrvDefExt::AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam)
{
    HPROPSHEETPAGE hPage;

    hPage = SH_CreatePropertySheetPage(IDD_DRIVE_PROPERTIES,
                                       GeneralPageProc,
                                       (LPARAM)this,
                                       NULL);
    if (hPage)
        pfnAddPage(hPage, lParam);

    if (GetDriveTypeW(m_wszDrive) == DRIVE_FIXED)
    {
        hPage = SH_CreatePropertySheetPage(IDD_DRIVE_TOOLS,
                                           ExtraPageProc,
                                           (LPARAM)this,
                                           NULL);
        if (hPage)
            pfnAddPage(hPage, lParam);
    }

    if (GetDriveTypeW(m_wszDrive) != DRIVE_REMOTE)
    {
        hPage = SH_CreatePropertySheetPage(IDD_DRIVE_HARDWARE,
                                           HardwarePageProc,
                                           (LPARAM)this,
                                           NULL);
        if (hPage)
            pfnAddPage(hPage, lParam);
    }

    return S_OK;
}

HRESULT WINAPI
CDrvDefExt::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplacePage, LPARAM lParam)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI
CDrvDefExt::SetSite(IUnknown *punk)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT WINAPI
CDrvDefExt::GetSite(REFIID iid, void **ppvSite)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}
