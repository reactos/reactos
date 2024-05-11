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

WINE_DEFAULT_DEBUG_CHANNEL(shell);

typedef struct
{
    WCHAR   Drive;
    UINT    Options;
    UINT Result;
    BOOL bFormattingNow;
} FORMAT_DRIVE_CONTEXT, *PFORMAT_DRIVE_CONTEXT;

EXTERN_C HPSXA WINAPI SHCreatePropSheetExtArrayEx(HKEY hKey, LPCWSTR pszSubKey, UINT max_iface, IDataObject *pDataObj);
HPROPSHEETPAGE SH_CreatePropertySheetPage(LPCSTR resname, DLGPROC dlgproc, LPARAM lParam, LPWSTR szTitle);

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
    WCHAR wszDriveLetter[6], wszSystemDrv[6];

    wszDriveLetter[0] = pContext->Drive + L'A';
    StringCchCatW(wszDriveLetter, _countof(wszDriveLetter), L":");

    if (!GetEnvironmentVariableW(L"SystemDrive", wszSystemDrv, _countof(wszSystemDrv)))
        return FALSE;

    if (!_wcsicmp(wszDriveLetter, wszSystemDrv))
        return TRUE;

    return FALSE;
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

typedef struct _DRIVE_PROP_PAGE
{
    LPCSTR resname;
    DLGPROC dlgproc;
    UINT DriveType;
} DRIVE_PROP_PAGE;

struct DRIVE_PROP_DATA
{
    PWSTR pwszDrive;
    IStream *pStream;
};

static DWORD WINAPI
ShowDrivePropThreadProc(LPVOID pParam)
{
    CHeapPtr<DRIVE_PROP_DATA, CComAllocator> pPropData((DRIVE_PROP_DATA *)pParam);
    CHeapPtr<WCHAR, CComAllocator> pwszDrive(pPropData->pwszDrive);

    // Unmarshall IDataObject from IStream
    CComPtr<IDataObject> pDataObj;
    CoGetInterfaceAndReleaseStream(pPropData->pStream, IID_PPV_ARG(IDataObject, &pDataObj));

    HPSXA hpsx = NULL;
    HPROPSHEETPAGE hpsp[MAX_PROPERTY_SHEET_PAGE];
    CComObject<CDrvDefExt> *pDrvDefExt = NULL;

    CDataObjectHIDA cida(pDataObj);
    if (FAILED_UNEXPECTEDLY(cida.hr()))
        return FAILED(cida.hr());

    RECT rcPosition = {CW_USEDEFAULT, CW_USEDEFAULT, 0, 0};
    POINT pt;
    if (SUCCEEDED(DataObject_GetOffset(pDataObj, &pt)))
    {
        rcPosition.left = pt.x;
        rcPosition.top = pt.y;
    }

    DWORD style = WS_DISABLED | WS_CLIPSIBLINGS | WS_CAPTION;
    DWORD exstyle = WS_EX_WINDOWEDGE | WS_EX_APPWINDOW;
    CStubWindow32 stub;
    if (!stub.Create(NULL, rcPosition, NULL, style, exstyle))
    {
        ERR("StubWindow32 creation failed\n");
        return FALSE;
    }

    PROPSHEETHEADERW psh = {sizeof(PROPSHEETHEADERW)};
    psh.dwFlags = PSH_PROPTITLE;
    psh.pszCaption = pwszDrive;
    psh.hwndParent = stub;
    psh.nStartPage = 0;
    psh.phpage = hpsp;

    HRESULT hr = CComObject<CDrvDefExt>::CreateInstance(&pDrvDefExt);
    if (SUCCEEDED(hr))
    {
        pDrvDefExt->AddRef(); // CreateInstance returns object with 0 ref count
        hr = pDrvDefExt->Initialize(HIDA_GetPIDLFolder(cida), pDataObj, NULL);
        if (SUCCEEDED(hr))
        {
            hr = pDrvDefExt->AddPages(AddPropSheetPageCallback, (LPARAM)&psh);
            if (FAILED(hr))
                ERR("AddPages failed\n");
        }
        else
        {
            ERR("Initialize failed\n");
        }
    }

    hpsx = SHCreatePropSheetExtArrayEx(HKEY_CLASSES_ROOT, L"Drive", MAX_PROPERTY_SHEET_PAGE, pDataObj);
    if (hpsx)
        SHAddFromPropSheetExtArray(hpsx, (LPFNADDPROPSHEETPAGE)AddPropSheetPageCallback, (LPARAM)&psh);

    INT_PTR ret = PropertySheetW(&psh);

    if (hpsx)
        SHDestroyPropSheetExtArray(hpsx);
    if (pDrvDefExt)
        pDrvDefExt->Release();

    stub.DestroyWindow();

    return ret != -1;
}

BOOL
SH_ShowDriveProperties(WCHAR *pwszDrive, IDataObject *pDataObj)
{
    HRESULT hr = SHStrDupW(pwszDrive, &pwszDrive);
    if (FAILED_UNEXPECTEDLY(hr))
        return FALSE;

    // Prepare data for thread
    DRIVE_PROP_DATA *pData = (DRIVE_PROP_DATA *)SHAlloc(sizeof(*pData));
    if (!pData)
    {
        SHFree(pwszDrive);
        return FALSE;
    }
    pData->pwszDrive = pwszDrive;

    // Marshall IDataObject to IStream
    hr = CoMarshalInterThreadInterfaceInStream(IID_IDataObject, pDataObj, &pData->pStream);
    if (SUCCEEDED(hr))
    {
        // Run a property sheet in another thread
        if (SHCreateThread(ShowDrivePropThreadProc, pData, CTF_COINIT, NULL))
            return TRUE; // Success

        pData->pStream->Release();
    }
    SHFree(pData);
    SHFree(pwszDrive);
    return FALSE; // Failed
}

static VOID
InsertDefaultClusterSizeForFs(HWND hwndDlg, PFORMAT_DRIVE_CONTEXT pContext)
{
    WCHAR wszBuf[100] = {0};
    WCHAR wszDefaultSize[100] = {0};
    PCWSTR pwszFsSizeLimit;
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

    szDrive[0] = pContext->Drive + L'A';

    if (!GetDiskFreeSpaceExW(szDrive, &FreeBytesAvailableUser, &TotalNumberOfBytes, NULL))
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
        if (!_wcsicmp(wszBuf, L"EXT2") ||
            !_wcsicmp(wszBuf, L"BtrFS") ||
            !_wcsicmp(wszBuf, L"NTFS"))
        {
            /* Enable the "Enable Compression" button */
            EnableWindow(GetDlgItem(hwndDlg, 28675), TRUE);
        }
        else
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

    cchText = GetWindowTextW(hwndDlg, szText, _countof(szText) - 1);
    if (cchText < 0)
        cchText = 0;
    szText[cchText++] = L' ';
    szDrive[0] = pContext->Drive + L'A';
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

    if (GetDiskFreeSpaceExW(szDrive, &FreeBytesAvailableUser, &TotalNumberOfBytes, NULL))
    {
        if (StrFormatByteSizeW(TotalNumberOfBytes.QuadPart, szText, _countof(szText)))
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
        if (!_wcsicmp(szText, szFs))
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
            ShellMessageBoxW(shell32_hInstance, FormatDrvDialog, MAKEINTRESOURCEW(IDS_FORMAT_COMPLETE), MAKEINTRESOURCEW(IDS_FORMAT_TITLE), MB_OK | MB_ICONINFORMATION);
            SendDlgItemMessageW(FormatDrvDialog, 28678, PBM_SETPOS, 0, 0);
            break;

        case VOLUMEINUSE:
        case INSUFFICIENTRIGHTS:
        case FSNOTSUPPORTED:
        case CLUSTERSIZETOOSMALL:
            bSuccess = FALSE;
            FIXME("Unsupported command in FormatExCB\n");
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
    DWORD DriveType;
    FMIFS_MEDIA_FLAG MediaFlag = FMIFS_HARDDISK;

    /* set volume path */
    szDrive[0] = pContext->Drive + L'A';

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
    bSuccess = FALSE;

    /* FIXME
     * will cause display problems
     * when performing more than one format
     */
    FormatDrvDialog = hwndDlg;

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
    FormatEx(szDrive,
             MediaFlag,
             szFileSys,
             szLabel,
             QuickFormat,
             ClusterSize,
             FormatExCB);

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
        pContext->Result = FALSE;
    }
}

struct FORMAT_DRIVE_PARAMS
{
    HWND hwndDlg;
    PFORMAT_DRIVE_CONTEXT pContext;
};

static unsigned __stdcall DoFormatDrive(void *args)
{
    FORMAT_DRIVE_PARAMS *pParams = reinterpret_cast<FORMAT_DRIVE_PARAMS *>(args);
    HWND hwndDlg = pParams->hwndDlg;
    PFORMAT_DRIVE_CONTEXT pContext = pParams->pContext;

	/* Disable controls during format */
    HMENU hSysMenu = GetSystemMenu(hwndDlg, FALSE);
    EnableMenuItem(hSysMenu, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
    EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, IDCANCEL), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, 28673), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, 28677), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, 28680), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, 28679), FALSE);
    EnableWindow(GetDlgItem(hwndDlg, 28674), FALSE);

    FormatDrive(hwndDlg, pContext);

	/* Re-enable controls after format */
    EnableWindow(GetDlgItem(hwndDlg, IDOK), TRUE);
    EnableWindow(GetDlgItem(hwndDlg, IDCANCEL), TRUE);
    EnableWindow(GetDlgItem(hwndDlg, 28673), TRUE);
    EnableWindow(GetDlgItem(hwndDlg, 28677), TRUE);
    EnableWindow(GetDlgItem(hwndDlg, 28680), TRUE);
    EnableWindow(GetDlgItem(hwndDlg, 28679), TRUE);
    EnableWindow(GetDlgItem(hwndDlg, 28674), TRUE);
    EnableMenuItem(hSysMenu, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);
    pContext->bFormattingNow = FALSE;

    delete pParams;
    return 0;
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
                    if (pContext->bFormattingNow)
                        break;

                    if (ShellMessageBoxW(shell32_hInstance, hwndDlg,
                                         MAKEINTRESOURCEW(IDS_FORMAT_WARNING),
                                         MAKEINTRESOURCEW(IDS_FORMAT_TITLE),
                                         MB_OKCANCEL | MB_ICONWARNING) == IDOK)
                    {
                        pContext->bFormattingNow = TRUE;

                        FORMAT_DRIVE_PARAMS *pParams = new FORMAT_DRIVE_PARAMS;
                        pParams->hwndDlg = hwndDlg;
                        pParams->pContext = pContext;

                        unsigned tid;
                        HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, DoFormatDrive, pParams, 0, &tid);
                        CloseHandle(hThread);
                    }
                    break;
                case IDCANCEL:
                    pContext = (PFORMAT_DRIVE_CONTEXT)GetWindowLongPtr(hwndDlg, DWLP_USER);
                    if (pContext->bFormattingNow)
                        break;

                    EndDialog(hwndDlg, pContext->Result);
                    break;
                case 28677: // filesystem combo
                    if (HIWORD(wParam) == CBN_SELENDOK)
                    {
                        pContext = (PFORMAT_DRIVE_CONTEXT)GetWindowLongPtr(hwndDlg, DWLP_USER);
                        if (pContext->bFormattingNow)
                            break;

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
