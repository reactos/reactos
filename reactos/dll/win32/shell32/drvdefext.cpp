/*
 * Provides default drive shell extension
 *
 * Copyright 2005 Johannes Anderwald
 * Copyright 2012 Rafal Harabien
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

#include <precomp.h>

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

static VOID
GetDriveNameWithLetter(LPWSTR pwszText, UINT cchTextMax, LPCWSTR pwszDrive)
{
    DWORD dwMaxComp, dwFileSys, cchText = 0;

    if (GetVolumeInformationW(pwszDrive, pwszText, cchTextMax, NULL, &dwMaxComp, &dwFileSys, NULL, 0))
    {
        cchText = wcslen(pwszText);
        if (cchText == 0)
        {
            /* load default volume label */
            cchText = LoadStringW(shell32_hInstance, IDS_DRIVE_FIXED, pwszText, cchTextMax);
        }
    }

    StringCchPrintfW(pwszText + cchText, cchTextMax - cchText, L" (%c)", pwszDrive[0]);
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
        HBRUSH hBlueBrush;
        HBRUSH hMagBrush;
        RECT rect;
        LONG horzsize;
        LONGLONG Result;
        WCHAR szBuffer[20];

        hBlueBrush = CreateSolidBrush(RGB(0, 0, 255));
        hMagBrush = CreateSolidBrush(RGB(255, 0, 255));

        GetDlgItemTextW(hwndDlg, 14006, szBuffer, 20);
        Result = _wtoi(szBuffer);

        CopyRect(&rect, &pDrawItem->rcItem);
        horzsize = rect.right - rect.left;
        Result = (Result * horzsize) / 100;

        rect.right = pDrawItem->rcItem.right - Result;
        FillRect(pDrawItem->hDC, &rect, hBlueBrush);
        rect.left = rect.right;
        rect.right = pDrawItem->rcItem.right;
        FillRect(pDrawItem->hDC, &rect, hMagBrush);
        DeleteObject(hBlueBrush);
        DeleteObject(hMagBrush);
    }
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
        /* set volume label */
        SetDlgItemTextW(hwndDlg, 14000, wszVolumeName);

        /* set filesystem type */
        SetDlgItemTextW(hwndDlg, 14002, wszFileSystem);
    }

    UINT DriveType = GetDriveTypeW(m_wszDrive);
    if (DriveType == DRIVE_FIXED || DriveType == DRIVE_CDROM)
    {
        ULARGE_INTEGER FreeBytesAvailable, TotalNumberOfBytes, TotalNumberOfFreeBytes;
        if(GetDiskFreeSpaceExW(m_wszDrive, &FreeBytesAvailable, &TotalNumberOfBytes, &TotalNumberOfFreeBytes))
        {
            ULONG SpacePercent;
            HANDLE hVolume;
            DWORD BytesReturned = 0;

            swprintf(wszBuf, L"\\\\.\\%c:", towupper(m_wszDrive[0]));
            hVolume = CreateFileW(wszBuf, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
            if (hVolume != INVALID_HANDLE_VALUE)
            {
                bRet = DeviceIoControl(hVolume, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, (LPVOID)&TotalNumberOfBytes, sizeof(ULARGE_INTEGER), &BytesReturned, NULL);
                if (bRet && StrFormatByteSizeW(TotalNumberOfBytes.QuadPart, wszBuf, sizeof(wszBuf) / sizeof(WCHAR)))
                    SetDlgItemTextW(hwndDlg, 14007, wszBuf);

                CloseHandle(hVolume);
            }

            TRACE("wszBuf %s hVolume %p bRet %d LengthInformation %ul BytesReturned %d\n", debugstr_w(wszBuf), hVolume, bRet, TotalNumberOfBytes.QuadPart, BytesReturned);

            if (StrFormatByteSizeW(TotalNumberOfBytes.QuadPart - FreeBytesAvailable.QuadPart, wszBuf, sizeof(wszBuf) / sizeof(WCHAR)))
                SetDlgItemTextW(hwndDlg, 14003, wszBuf);

            if (StrFormatByteSizeW(FreeBytesAvailable.QuadPart, wszBuf, sizeof(wszBuf) / sizeof(WCHAR)))
                SetDlgItemTextW(hwndDlg, 14005, wszBuf);

            SpacePercent = (ULONG)(TotalNumberOfFreeBytes.QuadPart*100ull/TotalNumberOfBytes.QuadPart);
            /* set free bytes percentage */
            swprintf(wszBuf, L"%u%%", SpacePercent);
            SetDlgItemTextW(hwndDlg, 14006, wszBuf);
            /* store used share amount */
            SpacePercent = 100 - SpacePercent;
            swprintf(wszBuf, L"%u%%", SpacePercent);
            SetDlgItemTextW(hwndDlg, 14004, wszBuf);
            if (DriveType == DRIVE_FIXED)
            {
                if (LoadStringW(shell32_hInstance, IDS_DRIVE_FIXED, wszBuf, sizeof(wszBuf) / sizeof(WCHAR)))
                    SetDlgItemTextW(hwndDlg, 14001, wszBuf);
            }
            else /* DriveType == DRIVE_CDROM) */
            {
                if (LoadStringW(shell32_hInstance, IDS_DRIVE_CDROM, wszBuf, sizeof(wszBuf) / sizeof(WCHAR)))
                    SetDlgItemTextW(hwndDlg, 14001, wszBuf);
            }
        }
    }
    /* set drive description */
    WCHAR wszFormat[50];
    GetDlgItemTextW(hwndDlg, 14009, wszFormat, _countof(wszFormat));
    swprintf(wszBuf, wszFormat, m_wszDrive);
    SetDlgItemTextW(hwndDlg, 14009, wszBuf);
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

            CDrvDefExt *pDrvDefExt = (CDrvDefExt*)ppsp->lParam;
            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pDrvDefExt);
            pDrvDefExt->InitGeneralPage(hwndDlg);
            return TRUE;
        }
        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT pDrawItem = (LPDRAWITEMSTRUCT)lParam;

            if (pDrawItem->CtlID >= 14013 && pDrawItem->CtlID <= 14015)
            {
                CDrvDefExt::PaintStaticControls(hwndDlg, pDrawItem);
                return TRUE;
            }
            break;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == 14010) /* Disk Cleanup */
            {
                CDrvDefExt *pDrvDefExt = (CDrvDefExt*)GetWindowLongPtr(hwndDlg, DWLP_USER);
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
            break;
        case WM_NOTIFY:
            if (LOWORD(wParam) == 14000) // Label
            {
                if (HIWORD(wParam) == EN_CHANGE)
                {
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                }
                break;
            }
            else if (((LPNMHDR)lParam)->hwndFrom == GetParent(hwndDlg))
            {
                /* Property Sheet */
                LPPSHNOTIFY lppsn = (LPPSHNOTIFY)lParam;
                
                if (lppsn->hdr.code == PSN_APPLY)
                {
                    CDrvDefExt *pDrvDefExt = (CDrvDefExt*)GetWindowLongPtr(hwndDlg, DWLP_USER);
                    WCHAR wszBuf[256];

                    if (GetDlgItemTextW(hwndDlg, 14000, wszBuf, sizeof(wszBuf) / sizeof(WCHAR)))
                        SetVolumeLabelW(pDrvDefExt->m_wszDrive, wszBuf);
                    SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR);
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
            CDrvDefExt *pDrvDefExt = (CDrvDefExt*)GetWindowLongPtr(hwndDlg, DWLP_USER);

            switch(LOWORD(wParam))
            {
                case 14000:
                    DialogBoxParamW(shell32_hInstance, L"CHKDSK_DLG", hwndDlg, ChkDskDlg, (LPARAM)pDrvDefExt->m_wszDrive);
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
                                     RRF_RT_REG_EXPAND_SZ,
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
CDrvDefExt::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pDataObj, HKEY hkeyProgID)
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

    hPage = SH_CreatePropertySheetPage("DRIVE_GENERAL_DLG",
                                       GeneralPageProc,
                                       (LPARAM)this,
                                       NULL);
    if (hPage)
        pfnAddPage(hPage, lParam);

    if (GetDriveTypeW(m_wszDrive) == DRIVE_FIXED)
    {
        hPage = SH_CreatePropertySheetPage("DRIVE_EXTRA_DLG",
                                           ExtraPageProc,
                                           (LPARAM)this,
                                           NULL);
        if (hPage)
            pfnAddPage(hPage, lParam);
    }

    hPage = SH_CreatePropertySheetPage("DRIVE_HARDWARE_DLG",
                                       HardwarePageProc,
                                       (LPARAM)this,
                                       NULL);
    if (hPage)
        pfnAddPage(hPage, lParam);

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
