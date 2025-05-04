/*
 *                 Shell Library Functions
 *
 * Copyright 2005 Johannes Anderwald
 * Copyright 2017 Katayama Hirofumi MZ
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"
#include <process.h>
#include <ndk/obfuncs.h> // For NtQueryObject

WINE_DEFAULT_DEBUG_CHANNEL(shell);

extern BOOL IsDriveFloppyW(LPCWSTR pszDriveRoot);

typedef struct
{
    WCHAR   Drive; // Note: This is the drive number, not a drive letter
    UINT    Options;
    UINT Result;
    BOOL bFormattingNow;
    HWND hWndMain;
    HWND hWndTip, hWndTipTrigger;
    struct tagTip { UNICODE_STRING Name; WCHAR Buffer[400]; } Tip;
} FORMAT_DRIVE_CONTEXT, *PFORMAT_DRIVE_CONTEXT;

static inline BOOL
DevIoCtl(HANDLE hDevice, UINT Code, LPVOID pIn, UINT cbIn, LPVOID pOut, UINT cbOut)
{
    DWORD cb = 0;
    return DeviceIoControl(hDevice, Code, pIn, cbIn, pOut, cbOut, &cb, NULL);
}

static BOOL IsFloppy(PCWSTR pszDrive)
{
    return GetDriveTypeW(pszDrive) == DRIVE_REMOVABLE && IsDriveFloppyW(pszDrive);
}

/*
 * TODO: In Windows the Shell doesn't know by itself if a drive is
 * a system one or not but rather a packet message is being sent by
 * FMIFS library code and further translated into specific packet
 * status codes in the Shell, the packet being _FMIFS_PACKET_TYPE.
 *
 * With that being said, most of this code as well as FMIFS library code
 * have to be refactored in order to comply with the way Windows works.
 *
 * See the enum definition for more details:
 * https://github.com/microsoft/winfile/blob/master/src/fmifs.h#L23
 */
static BOOL
IsSystemDrive(PFORMAT_DRIVE_CONTEXT pContext)
{
    WCHAR wszSystemDrv[MAX_PATH];
    wszSystemDrv[0] = UNICODE_NULL;
    GetSystemDirectory(wszSystemDrv, _countof(wszSystemDrv));
    return (wszSystemDrv[0] | 32) == pContext->Drive + L'a';
}

static HANDLE
OpenLogicalDriveHandle(WORD DriveNumber)
{
    const WCHAR szPath[] = { '\\', '\\', '?', '\\', WCHAR(DriveNumber + 'A'), ':', '\0' };
    return CreateFileW(szPath, FILE_READ_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                       NULL, OPEN_EXISTING, 0, NULL);
}

static BOOL
GetLogicalDriveSize(WORD DriveNumber, ULARGE_INTEGER &Result)
{
    BOOL bSuccess = FALSE;
    const WCHAR szDrivePath[] = { WCHAR(DriveNumber + 'A'), ':', '\\', '\0' };

    HANDLE hDevice = OpenLogicalDriveHandle(DriveNumber);
    if (hDevice != INVALID_HANDLE_VALUE)
    {
        GET_LENGTH_INFORMATION LengthInfo;
        bSuccess = DevIoCtl(hDevice, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &LengthInfo, sizeof(LengthInfo));
        Result.QuadPart = LengthInfo.Length.QuadPart;
        if (!bSuccess && GetDriveTypeW(szDrivePath) == DRIVE_REMOVABLE) // Blank floppy
        {
            DISK_GEOMETRY dg;
            bSuccess = DevIoCtl(hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &dg, sizeof(dg));
            if (bSuccess)
                Result.QuadPart = dg.Cylinders.QuadPart * dg.TracksPerCylinder * dg.SectorsPerTrack * dg.BytesPerSector;
        }
        CloseHandle(hDevice);
    }
    if (!bSuccess || !Result.QuadPart)
        bSuccess = GetDiskFreeSpaceExW(szDrivePath, NULL, &Result, NULL); // Note: Not exact if NTFS quotas are in effect
    return bSuccess;
}

static PWSTR
CreateTipText(FORMAT_DRIVE_CONTEXT &Ctx)
{
    HANDLE hDevice = OpenLogicalDriveHandle(Ctx.Drive);
    if (hDevice == INVALID_HANDLE_VALUE)
        return NULL;

    ULONG cb;
    ZeroMemory(&Ctx.Tip, sizeof(Ctx.Tip));
    NtQueryObject(hDevice, ObjectNameInformation, &Ctx.Tip, sizeof(Ctx.Tip), &cb);
    if (Ctx.Tip.Name.Buffer && Ctx.Tip.Name.Buffer[0] == '\\')
        StringCbCatW(Ctx.Tip.Name.Buffer, sizeof(Ctx.Tip) - sizeof(UNICODE_STRING), L"\n");
    else
        (Ctx.Tip.Name.Buffer = Ctx.Tip.Buffer)[0] = UNICODE_NULL;

    PARTITION_INFORMATION_EX pie;
    if (!DevIoCtl(hDevice, IOCTL_DISK_GET_PARTITION_INFO_EX, NULL, 0, &pie, sizeof(pie)))
    {
        pie.PartitionStyle = PARTITION_STYLE_RAW;
        PARTITION_INFORMATION pi;
        if (DevIoCtl(hDevice, IOCTL_DISK_GET_PARTITION_INFO, NULL, 0, &pi, sizeof(pi)))
        {
            pie.PartitionStyle = PARTITION_STYLE_MBR;
            pie.PartitionNumber = pi.PartitionNumber;
        }
    }
    CloseHandle(hDevice);

    WCHAR szBuf[150], szGuid[39], *pszTip = Ctx.Tip.Name.Buffer;
    szBuf[0] = UNICODE_NULL;
    if (pie.PartitionStyle == PARTITION_STYLE_GPT)
    {
        StringFromGUID2(pie.Gpt.PartitionId, szGuid, _countof(szGuid));
        StringCchPrintfW(szBuf, _countof(szBuf), L"GPT %s %s", szGuid, pie.Gpt.Name);
    }
    if (pie.PartitionStyle == PARTITION_STYLE_MBR)
    {
        StringCchPrintfW(szBuf, _countof(szBuf), L"MBR (%d)", pie.PartitionNumber);
    }
    StringCbCatW(pszTip, sizeof(Ctx.Tip) - sizeof(Ctx.Tip.Name), szBuf);
    return pszTip;
}

static BOOL
GetDefaultClusterSize(LPWSTR szFs, PDWORD pClusterSize, PULARGE_INTEGER TotalNumberOfBytes)
{
    DWORD ClusterSize;

    if (!_wcsicmp(szFs, L"FAT16") ||
        !_wcsicmp(szFs, L"FAT")) // REACTOS HACK
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
    else if (!_wcsicmp(szFs, L"FAT32"))
    {
        if (TotalNumberOfBytes->QuadPart <= (64 * 1024 * 1024))
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
    else if (!_wcsicmp(szFs, L"FATX"))
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
        else if (TotalNumberOfBytes->QuadPart <= (8192LL * 1024LL * 1024LL))
            ClusterSize = 2048;
        else if (TotalNumberOfBytes->QuadPart <= (16384LL * 1024LL * 1024LL))
            ClusterSize = 8192;
        else if (TotalNumberOfBytes->QuadPart <= (32768LL * 1024LL * 1024LL))
            ClusterSize = 16384;
        else
            return FALSE;
    }
    else if (!_wcsicmp(szFs, L"NTFS"))
    {
        if (TotalNumberOfBytes->QuadPart <= (512 * 1024 * 1024))
            ClusterSize = 512;
        else if (TotalNumberOfBytes->QuadPart <= (1024 * 1024 * 1024))
            ClusterSize = 1024;
        else if (TotalNumberOfBytes->QuadPart <= (2048LL * 1024LL * 1024LL))
            ClusterSize = 2048;
        else
            ClusterSize = 2048;
    }
    else if (!_wcsicmp(szFs, L"EXT2"))
    {
        // auto block size calculation
        ClusterSize = 0;
    }
    else if (!_wcsicmp(szFs, L"BtrFS"))
    {
        // auto block size calculation
        ClusterSize = 0;
    }
    else
        return FALSE;

    *pClusterSize = ClusterSize;
    return TRUE;
}

static VOID
InsertDefaultClusterSizeForFs(HWND hwndDlg, PFORMAT_DRIVE_CONTEXT pContext)
{
    WCHAR wszBuf[100] = {0};
    WCHAR wszDefaultSize[100] = {0};
    PCWSTR pwszFsSizeLimit;
    INT iSelIndex;
    ULARGE_INTEGER TotalNumberOfBytes;
    DWORD ClusterSize;
    LRESULT lIndex;
    HWND hDlgCtrl;

    hDlgCtrl = GetDlgItem(hwndDlg, 28677);
    iSelIndex = SendMessage(hDlgCtrl, CB_GETCURSEL, 0, 0);
    if (iSelIndex == CB_ERR)
        return;

    if (SendMessageW(hDlgCtrl, CB_GETLBTEXT, iSelIndex, (LPARAM)wszBuf) == CB_ERR)
        return;

    if (!GetLogicalDriveSize(pContext->Drive, TotalNumberOfBytes))
        return;

    if (!_wcsicmp(wszBuf, L"FAT16") ||
        !_wcsicmp(wszBuf, L"FAT")) // REACTOS HACK
    {
        pwszFsSizeLimit = L"4GB";
    }
    else if (!_wcsicmp(wszBuf, L"FAT32"))
    {
        pwszFsSizeLimit = L"32GB";
    }
    else if (!_wcsicmp(wszBuf, L"FATX"))
    {
        pwszFsSizeLimit = L"1GB/32GB";
    }
    else if (!_wcsicmp(wszBuf, L"NTFS"))
    {
        pwszFsSizeLimit = L"256TB";
    }
    else if (!_wcsicmp(wszBuf, L"EXT2"))
    {
        pwszFsSizeLimit = L"32TB";
    }
    else
    {
        pwszFsSizeLimit = L"16EB";
    }

    if (!_wcsicmp(wszBuf, L"FAT16") ||
        !_wcsicmp(wszBuf, L"FAT") || // REACTOS HACK
        !_wcsicmp(wszBuf, L"FAT32") ||
        !_wcsicmp(wszBuf, L"FATX") ||
        !_wcsicmp(wszBuf, L"NTFS") ||
        !_wcsicmp(wszBuf, L"EXT2") ||
        !_wcsicmp(wszBuf, L"BtrFS"))
    {
        if (!GetDefaultClusterSize(wszBuf, &ClusterSize, &TotalNumberOfBytes))
        {
            TRACE("%S is not supported on drive larger than %S, current size: %lu\n", wszBuf, pwszFsSizeLimit, TotalNumberOfBytes.QuadPart);
            SendMessageW(hDlgCtrl, CB_DELETESTRING, iSelIndex, 0);
            return;
        }

        if (LoadStringW(shell32_hInstance, IDS_DEFAULT_CLUSTER_SIZE, wszDefaultSize, _countof(wszDefaultSize)))
        {
            hDlgCtrl = GetDlgItem(hwndDlg, 28680); // Get the window handle of "allocation unit size" combobox
            SendMessageW(hDlgCtrl, CB_RESETCONTENT, 0, 0);
            lIndex = SendMessageW(hDlgCtrl, CB_ADDSTRING, 0, (LPARAM)wszDefaultSize);
            if (lIndex != CB_ERR)
                SendMessageW(hDlgCtrl, CB_SETITEMDATA, lIndex, (LPARAM)ClusterSize);
            SendMessageW(hDlgCtrl, CB_SETCURSEL, 0, 0);
        }

        if (!_wcsicmp(wszBuf, L"NTFS"))
        {
            ClusterSize = 512;
            for (lIndex = 0; lIndex < 4; lIndex++)
            {
                TotalNumberOfBytes.QuadPart = ClusterSize;
                if (StrFormatByteSizeW(TotalNumberOfBytes.QuadPart, wszDefaultSize, _countof(wszDefaultSize)))
                {
                    lIndex = SendMessageW(hDlgCtrl, CB_ADDSTRING, 0, (LPARAM)wszDefaultSize);
                    if (lIndex != CB_ERR)
                        SendMessageW(hDlgCtrl, CB_SETITEMDATA, lIndex, (LPARAM)ClusterSize);
                }
                ClusterSize *= 2;
            }
        }

        SendMessageW(GetDlgItem(hwndDlg, 28675), BM_SETCHECK, BST_UNCHECKED, 0);
#if 0 // TODO: Call EnableVolumeCompression if checked
        if (!_wcsicmp(wszBuf, L"EXT2") ||
            !_wcsicmp(wszBuf, L"BtrFS") ||
            !_wcsicmp(wszBuf, L"NTFS"))
        {
            /* Enable the "Enable Compression" button */
            EnableWindow(GetDlgItem(hwndDlg, 28675), TRUE);
        }
        else
#endif
        {
            /* Disable the "Enable Compression" button */
            EnableWindow(GetDlgItem(hwndDlg, 28675), FALSE);
        }
    }
    else
    {
        FIXME("Unknown filesystem: %ls\n", wszBuf);
        SendDlgItemMessageW(hwndDlg, 28680, CB_RESETCONTENT, iSelIndex, 0);
        return;
    }
}

static VOID
EnableFormatDriveDlgControls(HWND hwndDlg, int EnableState)
{
    BOOL CanClose = EnableState != 0, Enable = EnableState > 0;
    HMENU hSysMenu = GetSystemMenu(hwndDlg, FALSE);
    EnableMenuItem(hSysMenu, SC_CLOSE, MF_BYCOMMAND | (CanClose ? MF_ENABLED : MF_GRAYED));
    EnableWindow(GetDlgItem(hwndDlg, IDCANCEL), CanClose);
    static const WORD id[] = { IDOK, 28673, 28677, 28680, 28679, 28674 };
    for (UINT i = 0; i < _countof(id); ++i)
        EnableWindow(GetDlgItem(hwndDlg, id[i]), Enable);
}

static VOID
InitializeFormatDriveDlg(HWND hwndDlg, PFORMAT_DRIVE_CONTEXT pContext)
{
    WCHAR szDrive[] = { WCHAR(pContext->Drive + 'A'), ':', '\\', '\0' };
    WCHAR szText[120], szFs[30];
    SIZE_T cchText;
    ULARGE_INTEGER TotalNumberOfBytes;
    DWORD dwIndex, dwDefault;
    UCHAR uMinor, uMajor;
    BOOLEAN Latest;
    HWND hwndFileSystems;

    pContext->hWndMain = hwndDlg;
    pContext->hWndTipTrigger = GetDlgItem(hwndDlg, 30000);
    TTTOOLINFOW tool;
    tool.cbSize = sizeof(tool);
    tool.hwnd = hwndDlg;
    tool.uFlags = TTF_SUBCLASS | TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
    tool.uId = (UINT_PTR)pContext->hWndTipTrigger;
    tool.lpszText = LPSTR_TEXTCALLBACKW;
    pContext->hWndTip = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, NULL, WS_POPUP |
                                        TTS_BALLOON | TTS_NOPREFIX | TTS_ALWAYSTIP,
                                        0, 0, 0, 0, hwndDlg, NULL, NULL, NULL);
    SendMessageW(pContext->hWndTip, TTM_ADDTOOLW, 0, (LPARAM)&tool);
    UINT nIcoSize = GetSystemMetrics(SM_CXSMICON);
    HICON hIco = (HICON)LoadImageW(shell32_hInstance, MAKEINTRESOURCEW(IDI_SHELL_IDEA), IMAGE_ICON, nIcoSize, nIcoSize, LR_SHARED);
    SendMessageW(pContext->hWndTipTrigger, STM_SETICON, (WPARAM)hIco, 0);

    cchText = GetWindowTextW(hwndDlg, szText, _countof(szText) - 1);
    szText[cchText++] = L' ';
    szFs[0] = UNICODE_NULL;
    if (GetVolumeInformationW(szDrive, &szText[cchText], _countof(szText) - cchText, NULL, NULL, NULL, szFs, _countof(szFs)))
    {
        if (szText[cchText] == UNICODE_NULL)
        {
            /* load default volume label */
            cchText += LoadStringW(shell32_hInstance, IDS_DRIVE_FIXED, &szText[cchText], _countof(szText) - cchText);
        }
        else
        {
            /* set volume label */
            SetDlgItemTextW(hwndDlg, 28679, &szText[cchText]);
            cchText += wcslen(&szText[cchText]);
        }
    }

    StringCchPrintfW(szText + cchText, _countof(szText) - cchText, L" (%c:)", szDrive[0]);

    /* set window text */
    SetWindowTextW(hwndDlg, szText);

    if (GetLogicalDriveSize(pContext->Drive, TotalNumberOfBytes))
    {
        if (StrFormatByteSizeW(TotalNumberOfBytes.QuadPart, szText, _countof(szText)))
        {
            /* add drive capacity */
            SendDlgItemMessageW(hwndDlg, 28673, CB_ADDSTRING, 0, (LPARAM)szText);
            SendDlgItemMessageW(hwndDlg, 28673, CB_SETCURSEL, 0, (LPARAM)0);
        }
    }
    else
    {
        /* No known size, don't allow format (no partition or no floppy) */
        EnableFormatDriveDlgControls(hwndDlg, -1);
    }

    if (pContext->Options & SHFMT_OPT_FULL)
    {
        /* check quick format button */
        SendDlgItemMessageW(hwndDlg, 28674, BM_SETCHECK, BST_CHECKED, 0);
    }

    /* enumerate all available filesystems */
    dwIndex = 0;
    dwDefault = 0;
    hwndFileSystems = GetDlgItem(hwndDlg, 28677);

    int iForceDefault = -1;
    while (QueryAvailableFileSystemFormat(dwIndex, szText, &uMajor, &uMinor, &Latest))
    {
        if (!_wcsicmp(szText, szFs))
            iForceDefault = dwDefault = dwIndex; /* default to the same filesystem */

        if (iForceDefault < 0 && !_wcsicmp(szText, L"NTFS") && !IsFloppy(szDrive))
            dwDefault = dwIndex;

        SendMessageW(hwndFileSystems, CB_ADDSTRING, 0, (LPARAM)szText);
        dwIndex++;
    }

    if (!dwIndex)
    {
        ERR("no filesystem providers\n");
        return;
    }

    /* select default filesys */
    SendMessageW(hwndFileSystems, CB_SETCURSEL, dwDefault, 0);
    /* setup cluster combo */
    InsertDefaultClusterSizeForFs(hwndDlg, pContext);
}

static inline PFORMAT_DRIVE_CONTEXT
GetFormatContext()
{
    // FormatEx does not allow us to specify a context parameter so we have to store it in the thread
    return (PFORMAT_DRIVE_CONTEXT)NtCurrentTeb()->NtTib.ArbitraryUserPointer;
}

static BOOLEAN NTAPI
FormatExCB(
    IN CALLBACKCOMMAND Command,
    IN ULONG SubAction,
    IN PVOID ActionInfo)
{
    PFORMAT_DRIVE_CONTEXT pCtx = GetFormatContext();
    const WCHAR szDrive[] = { WCHAR(pCtx->Drive + 'A'), ':', '\\', '\0' };
    WCHAR szLabel[40];
    PDWORD Progress;
    PBOOLEAN pSuccess;
    switch(Command)
    {
        case PROGRESS:
            Progress = (PDWORD)ActionInfo;
            SendDlgItemMessageW(pCtx->hWndMain, 28678, PBM_SETPOS, (WPARAM)*Progress, 0);
            break;
        case DONE:
            pSuccess = (PBOOLEAN)ActionInfo;
            pCtx->Result = (*pSuccess);
            SendDlgItemMessageW(pCtx->hWndMain, 28679, WM_GETTEXT, _countof(szLabel), (LPARAM)szLabel);
            SetVolumeLabelW(szDrive, *szLabel ? szLabel : NULL);
            ShellMessageBoxW(shell32_hInstance, pCtx->hWndMain, MAKEINTRESOURCEW(IDS_FORMAT_COMPLETE),
                             MAKEINTRESOURCEW(IDS_FORMAT_TITLE), MB_OK | MB_ICONINFORMATION);
            SendDlgItemMessageW(pCtx->hWndMain, 28678, PBM_SETPOS, 0, 0);
            break;

        case VOLUMEINUSE:
        case INSUFFICIENTRIGHTS:
        case FSNOTSUPPORTED:
        case CLUSTERSIZETOOSMALL:
            pCtx->Result = FALSE;
            FIXME("Unsupported command in FormatExCB\n");
            break;

        default:
            break;
    }

    return TRUE;
}

static VOID
FormatDrive(HWND hwndDlg, PFORMAT_DRIVE_CONTEXT pContext)
{
    WCHAR szDrive[] = { WCHAR(pContext->Drive + 'A'), ':', '\\', '\0' };
    WCHAR szFileSys[40];
    WCHAR szLabel[40];
    INT iSelIndex;
    UINT Length;
    HWND hDlgCtrl;
    BOOL QuickFormat;
    DWORD ClusterSize;
    DWORD DriveType;
    FMIFS_MEDIA_FLAG MediaFlag = FMIFS_HARDDISK;

    /* get filesystem */
    hDlgCtrl = GetDlgItem(hwndDlg, 28677);
    iSelIndex = SendMessageW(hDlgCtrl, CB_GETCURSEL, 0, 0);
    if (iSelIndex == CB_ERR)
    {
        ERR("Unable to get file system selection\n");
        return;
    }
    Length = SendMessageW(hDlgCtrl, CB_GETLBTEXTLEN, iSelIndex, 0);
    if ((int)Length == CB_ERR || Length + 1 > _countof(szFileSys))
    {
        ERR("Unable to get file system selection\n");
        return;
    }

    /* retrieve the file system */
    SendMessageW(hDlgCtrl, CB_GETLBTEXT, iSelIndex, (LPARAM)szFileSys);
    szFileSys[_countof(szFileSys)-1] = L'\0';

    /* retrieve the volume label */
    hDlgCtrl = GetWindow(hwndDlg, 28679);
    Length = SendMessageW(hDlgCtrl, WM_GETTEXTLENGTH, 0, 0);
    if (Length + 1 > _countof(szLabel))
    {
        ERR("Unable to get volume label\n");
        return;
    }
    SendMessageW(hDlgCtrl, WM_GETTEXT, _countof(szLabel), (LPARAM)szLabel);
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
    if ((int)ClusterSize == CB_ERR)
    {
        FIXME("\n");
        return;
    }

    hDlgCtrl = GetDlgItem(hwndDlg, 28680);
    SendMessageW(hDlgCtrl, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

    /* See if the drive is removable or not */
    DriveType = GetDriveTypeW(szDrive);
    switch (DriveType)
    {
        case DRIVE_UNKNOWN:
        case DRIVE_REMOTE:
        case DRIVE_CDROM:
        case DRIVE_NO_ROOT_DIR:
        {
            FIXME("\n");
            return;
        }

        case DRIVE_REMOVABLE:
            MediaFlag = FMIFS_FLOPPY;
            break;

        case DRIVE_FIXED:
        case DRIVE_RAMDISK:
            MediaFlag = FMIFS_HARDDISK;
            break;
    }

    /* Format the drive */
    pContext->Result = FALSE;
    NT_TIB &tib = NtCurrentTeb()->NtTib;
    PVOID BakArbitraryUserPointer = tib.ArbitraryUserPointer;
    tib.ArbitraryUserPointer = pContext;

    FormatEx(szDrive,
             MediaFlag,
             szFileSys,
             szLabel,
             QuickFormat,
             ClusterSize,
             FormatExCB);

    tib.ArbitraryUserPointer = BakArbitraryUserPointer;
    if (!pContext->Result)
    {
        pContext->Result = SHFMT_ERROR;
    }
    else if (QuickFormat)
    {
        pContext->Result = SHFMT_OPT_FULL;
    }
    else
    {
        pContext->Result = FALSE;
    }
}



static DWORD CALLBACK
FormatDriveThread(PVOID pThreadParameter)
{
    PFORMAT_DRIVE_CONTEXT pContext = (PFORMAT_DRIVE_CONTEXT)pThreadParameter;
    HWND hwndDlg = pContext->hWndMain;
    WCHAR szDrive[] = { WCHAR(pContext->Drive + 'A'), ':', '\\', '\0' };

    /* Disable controls during format */
    EnableFormatDriveDlgControls(hwndDlg, FALSE);

    SHChangeNotify(SHCNE_MEDIAREMOVED, SHCNF_PATHW | SHCNF_FLUSH, szDrive, NULL);

    FormatDrive(hwndDlg, pContext);

    if (pContext->Result != SHFMT_ERROR)
        SHChangeNotify(SHCNE_MEDIAINSERTED, SHCNF_PATHW, szDrive, NULL);
    SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATHW, szDrive, NULL);

    /* Re-enable controls after format */
    EnableFormatDriveDlgControls(hwndDlg, TRUE);
    pContext->bFormattingNow = FALSE;

    return 0;
}

static INT_PTR CALLBACK
FormatDriveDlg(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PFORMAT_DRIVE_CONTEXT pContext = (PFORMAT_DRIVE_CONTEXT)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch(uMsg)
    {
        case WM_INITDIALOG:
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)lParam);
            InitializeFormatDriveDlg(hwndDlg, (PFORMAT_DRIVE_CONTEXT)lParam);
            return TRUE;
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    if (pContext->bFormattingNow)
                        break;

                    if (ShellMessageBoxW(shell32_hInstance, hwndDlg,
                                         MAKEINTRESOURCEW(IDS_FORMAT_WARNING),
                                         MAKEINTRESOURCEW(IDS_FORMAT_TITLE),
                                         MB_OKCANCEL | MB_ICONWARNING) == IDOK)
                    {
                        pContext->bFormattingNow = TRUE;
                        SHCreateThread(FormatDriveThread, pContext, CTF_COINIT | CTF_PROCESS_REF | CTF_FREELIBANDEXIT, NULL);
                    }
                    break;
                case IDCANCEL:
                    if (pContext->bFormattingNow)
                        break;

                    EndDialog(hwndDlg, pContext->Result);
                    break;
                case 28677: // filesystem combo
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        if (pContext->bFormattingNow)
                            break;

                        InsertDefaultClusterSizeForFs(hwndDlg, pContext);
                    }
                    break;
            }
            break;
        case WM_NOTIFY:
        {
            NMTTDISPINFO &ttdi = *(NMTTDISPINFO*)lParam;
            if (ttdi.hdr.code == TTN_NEEDTEXTW)
            {
                ttdi.uFlags |= TTF_DI_SETITEM;
                if (PWSTR pszTip = CreateTipText(*pContext))
                    ttdi.lpszText = pszTip;
            }
            break;
        }
        case WM_NEXTDLGCTL:
            PostMessage(hwndDlg, WM_APP, TRUE, 0); // Delay our action until after focus has changed
            break;
        case WM_APP: // Show/Hide tip if tabbed to the info icon
        {
            TTTOOLINFOW tool;
            tool.cbSize = sizeof(tool);
            tool.hwnd = hwndDlg;
            tool.uId = (UINT_PTR)pContext->hWndTipTrigger;
            if (wParam && GetFocus() == pContext->hWndTipTrigger)
            {
                RECT r;
                GetWindowRect(pContext->hWndTipTrigger, &r);
                r.left += (r.right - r.left) / 2;
                r.top += (r.bottom - r.top) / 2;
                SendMessageW(pContext->hWndTip, TTM_TRACKPOSITION, 0, MAKELONG(r.left, r.top));
                SendMessageW(pContext->hWndTip, TTM_TRACKACTIVATE, TRUE, (LPARAM)&tool);
            }
            else
            {
                SendMessageW(pContext->hWndTip, TTM_TRACKACTIVATE, FALSE, (LPARAM)&tool);
            }
            break;
        }
        case WM_MOVING:
            SendMessage(hwndDlg, WM_APP, FALSE, 0);
            break;
    }
    return FALSE;
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

    Context.Drive = drive;
    Context.Options = options;
    Context.Result = FALSE;
    Context.bFormattingNow = FALSE;

    if (!IsSystemDrive(&Context))
    {
        result = DialogBoxParamW(shell32_hInstance, MAKEINTRESOURCEW(IDD_FORMAT_DRIVE), hwnd, FormatDriveDlg, (LPARAM)&Context);
    }
    else
    {
        result = SHFMT_ERROR;
        ShellMessageBoxW(shell32_hInstance, hwnd, MAKEINTRESOURCEW(IDS_NO_FORMAT), MAKEINTRESOURCEW(IDS_NO_FORMAT_TITLE), MB_OK | MB_ICONWARNING);
        TRACE("SHFormatDrive(): The provided drive for format is a system volume! Aborting...\n");
    }

    return result;
}
