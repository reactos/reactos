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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"

#define MAX_PROPERTY_SHEET_PAGE 32

WINE_DEFAULT_DEBUG_CHANNEL(shell);

typedef struct
{
    WCHAR   Drive;
    UINT    Options;
    UINT Result;
} FORMAT_DRIVE_CONTEXT, *PFORMAT_DRIVE_CONTEXT;

EXTERN_C HPSXA WINAPI SHCreatePropSheetExtArrayEx(HKEY hKey, LPCWSTR pszSubKey, UINT max_iface, IDataObject *pDataObj);
HPROPSHEETPAGE SH_CreatePropertySheetPage(LPCSTR resname, DLGPROC dlgproc, LPARAM lParam, LPWSTR szTitle);

static BOOL
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
    else if (!wcsicmp(szFs, L"NTFS"))
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
    else
        return FALSE;

    *pClusterSize = ClusterSize;
    return TRUE;
}

static BOOL CALLBACK
AddPropSheetPageCallback(HPROPSHEETPAGE hPage, LPARAM lParam)
{
    PROPSHEETHEADER *ppsh = (PROPSHEETHEADER *)lParam;
    if (ppsh->nPages < MAX_PROPERTY_SHEET_PAGE)
    {
        ppsh->phpage[ppsh->nPages++] = hPage;
        return TRUE;
    }
    return FALSE;
}

typedef struct _DRIVE_PROP_PAGE
{
    LPCSTR resname;
    DLGPROC dlgproc;
    UINT DriveType;
} DRIVE_PROP_PAGE;

BOOL
SH_ShowDriveProperties(WCHAR *pwszDrive, LPCITEMIDLIST pidlFolder, LPCITEMIDLIST *apidl)
{
    HPSXA hpsx = NULL;
    HPROPSHEETPAGE hpsp[MAX_PROPERTY_SHEET_PAGE];
    PROPSHEETHEADERW psh;
    CComObject<CDrvDefExt> *pDrvDefExt = NULL;
    
    ZeroMemory(&psh, sizeof(PROPSHEETHEADERW));
    psh.dwSize = sizeof(PROPSHEETHEADERW);
    psh.dwFlags = 0; // FIXME: make it modeless
    psh.hwndParent = NULL;
    psh.nStartPage = 0;
    psh.phpage = hpsp;

    WCHAR wszName[256];
    if (GetVolumeInformationW(pwszDrive, wszName, sizeof(wszName) / sizeof(WCHAR), NULL, NULL, NULL, NULL, 0))
    {
        psh.pszCaption = wszName;
        psh.dwFlags |= PSH_PROPTITLE;
        if (wszName[0] == UNICODE_NULL)
        {
            /* FIXME: check if disk is a really a local hdd */
            UINT i = LoadStringW(shell32_hInstance, IDS_DRIVE_FIXED, wszName, sizeof(wszName) / sizeof(WCHAR) - 6);
            StringCchPrintf(wszName + i, sizeof(wszName) / sizeof(WCHAR) - i, L" (%s)", pwszDrive);
        }
    }

    CComPtr<IDataObject> pDataObj;
    HRESULT hr = SHCreateDataObject(pidlFolder, 1, apidl, NULL, IID_PPV_ARG(IDataObject, &pDataObj));

    if (SUCCEEDED(hr))
    {
        hr = CComObject<CDrvDefExt>::CreateInstance(&pDrvDefExt);
        if (SUCCEEDED(hr))
        {
            pDrvDefExt->AddRef(); // CreateInstance returns object with 0 ref count
            hr = pDrvDefExt->Initialize(pidlFolder, pDataObj, NULL);
            if (SUCCEEDED(hr))
            {
                hr = pDrvDefExt->AddPages(AddPropSheetPageCallback, (LPARAM)&psh);
                if (FAILED(hr))
                    ERR("AddPages failed\n");
            } else
                ERR("Initialize failed\n");
        }

        hpsx = SHCreatePropSheetExtArrayEx(HKEY_CLASSES_ROOT, L"Drive", MAX_PROPERTY_SHEET_PAGE, pDataObj);
        if (hpsx)
            SHAddFromPropSheetExtArray(hpsx, (LPFNADDPROPSHEETPAGE)AddPropSheetPageCallback, (LPARAM)&psh);
    }

    HWND hwnd = (HWND)PropertySheetW(&psh);

    if (hpsx)
        SHDestroyPropSheetExtArray(hpsx);
    if (pDrvDefExt)
        pDrvDefExt->Release();

    if (!hwnd)
        return FALSE;
    return TRUE;
}

static VOID
InsertDefaultClusterSizeForFs(HWND hwndDlg, PFORMAT_DRIVE_CONTEXT pContext)
{
    WCHAR wszBuf[100] = {0};
    WCHAR szDrive[] = L"C:\\";
    INT iSelIndex;
    ULARGE_INTEGER FreeBytesAvailableUser, TotalNumberOfBytes;
    DWORD ClusterSize;
    LRESULT lIndex;
    HWND hDlgCtrl;

    hDlgCtrl = GetDlgItem(hwndDlg, 28677);
    iSelIndex = SendMessage(hDlgCtrl, CB_GETCURSEL, 0, 0);
    if (iSelIndex == CB_ERR)
        return;

    if (SendMessageW(hDlgCtrl, CB_GETLBTEXT, iSelIndex, (LPARAM)wszBuf) == CB_ERR)
        return;

    szDrive[0] = pContext->Drive + 'A';

    if (!GetDiskFreeSpaceExW(szDrive, &FreeBytesAvailableUser, &TotalNumberOfBytes, NULL))
        return;

    if (!wcsicmp(wszBuf, L"FAT16") ||
        !wcsicmp(wszBuf, L"FAT")) //REACTOS HACK
    {
        if (!GetDefaultClusterSize(wszBuf, &ClusterSize, &TotalNumberOfBytes))
        {
            TRACE("FAT16 is not supported on hdd larger than 4G current %lu\n", TotalNumberOfBytes.QuadPart);
            SendMessageW(hDlgCtrl, CB_DELETESTRING, iSelIndex, 0);
            return;
        }

        if (LoadStringW(shell32_hInstance, IDS_DEFAULT_CLUSTER_SIZE, wszBuf, sizeof(wszBuf) / sizeof(WCHAR)))
        {
            hDlgCtrl = GetDlgItem(hwndDlg, 28680);
            SendMessageW(hDlgCtrl, CB_RESETCONTENT, 0, 0);
            lIndex = SendMessageW(hDlgCtrl, CB_ADDSTRING, 0, (LPARAM)wszBuf);
            if (lIndex != CB_ERR)
                SendMessageW(hDlgCtrl, CB_SETITEMDATA, lIndex, (LPARAM)ClusterSize);
            SendMessageW(hDlgCtrl, CB_SETCURSEL, 0, 0);
        }
    }
    else if (!wcsicmp(wszBuf, L"FAT32"))
    {
        if (!GetDefaultClusterSize(wszBuf, &ClusterSize, &TotalNumberOfBytes))
        {
            TRACE("FAT32 is not supported on hdd larger than 32G current %lu\n", TotalNumberOfBytes.QuadPart);
            SendMessageW(hDlgCtrl, CB_DELETESTRING, iSelIndex, 0);
            return;
        }

        if (LoadStringW(shell32_hInstance, IDS_DEFAULT_CLUSTER_SIZE, wszBuf, sizeof(wszBuf) / sizeof(WCHAR)))
        {
            hDlgCtrl = GetDlgItem(hwndDlg, 28680);
            SendMessageW(hDlgCtrl, CB_RESETCONTENT, 0, 0);
            lIndex = SendMessageW(hDlgCtrl, CB_ADDSTRING, 0, (LPARAM)wszBuf);
            if (lIndex != CB_ERR)
                SendMessageW(hDlgCtrl, CB_SETITEMDATA, lIndex, (LPARAM)ClusterSize);
            SendMessageW(hDlgCtrl, CB_SETCURSEL, 0, 0);
        }
    }
    else if (!wcsicmp(wszBuf, L"NTFS"))
    {
        if (!GetDefaultClusterSize(wszBuf, &ClusterSize, &TotalNumberOfBytes))
        {
            TRACE("NTFS is not supported on hdd larger than 2TB current %lu\n", TotalNumberOfBytes.QuadPart);
            SendMessageW(hDlgCtrl, CB_DELETESTRING, iSelIndex, 0);
            return;
        }

        hDlgCtrl = GetDlgItem(hwndDlg, 28680);
        if (LoadStringW(shell32_hInstance, IDS_DEFAULT_CLUSTER_SIZE, wszBuf, sizeof(wszBuf) / sizeof(WCHAR)))
        {
            SendMessageW(hDlgCtrl, CB_RESETCONTENT, 0, 0);
            lIndex = SendMessageW(hDlgCtrl, CB_ADDSTRING, 0, (LPARAM)wszBuf);
            if (lIndex != CB_ERR)
                SendMessageW(hDlgCtrl, CB_SETITEMDATA, lIndex, (LPARAM)ClusterSize);
            SendMessageW(hDlgCtrl, CB_SETCURSEL, 0, 0);
        }
        ClusterSize = 512;
        for (lIndex = 0; lIndex < 4; lIndex++)
        {
            TotalNumberOfBytes.QuadPart = ClusterSize;
            if (StrFormatByteSizeW(TotalNumberOfBytes.QuadPart, wszBuf, sizeof(wszBuf) / sizeof(WCHAR)))
            {
                lIndex = SendMessageW(hDlgCtrl, CB_ADDSTRING, 0, (LPARAM)wszBuf);
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

static VOID
InitializeFormatDriveDlg(HWND hwndDlg, PFORMAT_DRIVE_CONTEXT pContext)
{
    WCHAR szText[120];
    WCHAR szDrive[] = L"C:\\";
    WCHAR szFs[30] = L"";
    INT cchText;
    ULARGE_INTEGER FreeBytesAvailableUser, TotalNumberOfBytes;
    DWORD dwIndex, dwDefault;
    UCHAR uMinor, uMajor;
    BOOLEAN Latest;
    HWND hwndFileSystems;

    cchText = GetWindowTextW(hwndDlg, szText, sizeof(szText) / sizeof(WCHAR) - 1);
    if (cchText < 0)
        cchText = 0;
    szText[cchText++] = L' ';
    szDrive[0] = pContext->Drive + L'A';
    if (GetVolumeInformationW(szDrive, &szText[cchText], (sizeof(szText) / sizeof(WCHAR)) - cchText, NULL, NULL, NULL, szFs, sizeof(szFs) / sizeof(WCHAR)))
    {
        if (szText[cchText] == UNICODE_NULL)
        {
            /* load default volume label */
            cchText += LoadStringW(shell32_hInstance, IDS_DRIVE_FIXED, &szText[cchText], (sizeof(szText) / sizeof(WCHAR)) - cchText);
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

    if (GetDiskFreeSpaceExW(szDrive, &FreeBytesAvailableUser, &TotalNumberOfBytes, NULL))
    {
        if (StrFormatByteSizeW(TotalNumberOfBytes.QuadPart, szText, sizeof(szText) / sizeof(WCHAR)))
        {
            /* add drive capacity */
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
    hwndFileSystems = GetDlgItem(hwndDlg, 28677);

    while(QueryAvailableFileSystemFormat(dwIndex, szText, &uMajor, &uMinor, &Latest))
    {
        if (!wcsicmp(szText, szFs))
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
    /* hide progress control */
    ShowWindow(GetDlgItem(hwndDlg, 28678), SW_HIDE);
}

static HWND FormatDrvDialog = NULL;
static BOOLEAN bSuccess = FALSE;

static BOOLEAN NTAPI
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
    if ((int)Length == CB_ERR || Length + 1 > sizeof(szFileSys) / sizeof(WCHAR))
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
    if (Length + 1 > sizeof(szLabel) / sizeof(WCHAR))
    {
        FIXME("\n");
        return;
    }
    SendMessageW(hDlgCtrl, WM_GETTEXT, sizeof(szLabel) / sizeof(WCHAR), (LPARAM)szLabel);
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
    ShowWindow(hDlgCtrl, SW_SHOW);
    SendMessageW(hDlgCtrl, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    bSuccess = FALSE;

    /* FIXME
     * will cause display problems
     * when performing more than one format
     */
    FormatDrvDialog = hwndDlg;

    FormatEx(szDrive,
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

static INT_PTR CALLBACK
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

    result = DialogBoxParamW(shell32_hInstance, MAKEINTRESOURCEW(IDD_FORMAT_DRIVE), hwnd, FormatDriveDlg, (LPARAM)&Context);

    return result;
}


