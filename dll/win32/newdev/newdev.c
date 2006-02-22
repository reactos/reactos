/*
 * New device installer (newdev.dll)
 *
 * Copyright 2005 Hervé Poussineau (hpoussin@reactos.org)
 *           2005 Christoph von Wittich (Christoph@ActiveVB.de)
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

#define YDEBUG
#include "newdev.h"

WINE_DEFAULT_DEBUG_CHANNEL(newdev);

static BOOL SearchDriver ( PDEVINSTDATA DevInstData, LPCTSTR Path );
static BOOL InstallDriver ( PDEVINSTDATA DevInstData );
static DWORD WINAPI FindDriverProc( LPVOID lpParam );
static BOOL FindDriver ( PDEVINSTDATA DevInstData );

static DEVINSTDATA DevInstData;
HINSTANCE hDllInstance;
HANDLE hThread;

static BOOL
CanDisableDevice(IN DEVINST DevInst,
                 IN HMACHINE hMachine,
                 OUT BOOL *CanDisable)
{
#if 0
    /* hpoussin, Dec 2005. I've disabled this code because
     * ntoskrnl never sets the DN_DISABLEABLE flag.
     */
    CONFIGRET cr;
    ULONG Status, ProblemNumber;
    BOOL Ret = FALSE;

    cr = CM_Get_DevNode_Status_Ex(&Status,
                                  &ProblemNumber,
                                  DevInst,
                                  0,
                                  hMachine);
    if (cr == CR_SUCCESS)
    {
        *CanDisable = ((Status & DN_DISABLEABLE) != 0);
        Ret = TRUE;
    }

    return Ret;
#else
    *CanDisable = TRUE;
    return TRUE;
#endif
}


static BOOL
IsDeviceStarted(IN DEVINST DevInst,
                IN HMACHINE hMachine,
                OUT BOOL *IsEnabled)
{
    CONFIGRET cr;
    ULONG Status, ProblemNumber;
    BOOL Ret = FALSE;

    cr = CM_Get_DevNode_Status_Ex(&Status,
                                  &ProblemNumber,
                                  DevInst,
                                  0,
                                  hMachine);
    if (cr == CR_SUCCESS)
    {
        *IsEnabled = ((Status & DN_STARTED) != 0);
        Ret = TRUE;
    }

    return Ret;
}


static BOOL
StartDevice(IN HDEVINFO DeviceInfoSet,
            IN PSP_DEVINFO_DATA DevInfoData  OPTIONAL,
            IN BOOL bEnable,
            IN DWORD HardwareProfile  OPTIONAL,
            OUT BOOL *bNeedReboot  OPTIONAL)
{
    SP_PROPCHANGE_PARAMS pcp;
    SP_DEVINSTALL_PARAMS dp;
    DWORD LastErr;
    BOOL Ret = FALSE;

    pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
    pcp.HwProfile = HardwareProfile;

    if (bEnable)
    {
        /* try to enable the device on the global profile */
        pcp.StateChange = DICS_ENABLE;
        pcp.Scope = DICS_FLAG_GLOBAL;

        /* ignore errors */
        LastErr = GetLastError();
        if (SetupDiSetClassInstallParams(DeviceInfoSet,
                                         DevInfoData,
                                         &pcp.ClassInstallHeader,
                                         sizeof(SP_PROPCHANGE_PARAMS)))
        {
            SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,
                                      DeviceInfoSet,
                                      DevInfoData);
        }
        SetLastError(LastErr);
    }

    /* try config-specific */
    pcp.StateChange = (bEnable ? DICS_ENABLE : DICS_DISABLE);
    pcp.Scope = DICS_FLAG_CONFIGSPECIFIC;

    if (SetupDiSetClassInstallParams(DeviceInfoSet,
                                     DevInfoData,
                                     &pcp.ClassInstallHeader,
                                     sizeof(SP_PROPCHANGE_PARAMS)) &&
        SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,
                                  DeviceInfoSet,
                                  DevInfoData))
    {
        dp.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        if (SetupDiGetDeviceInstallParams(DeviceInfoSet,
                                          DevInfoData,
                                          &dp))
        {
            if (bNeedReboot != NULL)
            {
                *bNeedReboot = ((dp.Flags & (DI_NEEDRESTART | DI_NEEDREBOOT)) != 0);
            }

            Ret = TRUE;
        }
    }
    return Ret;
}

/*
* @unimplemented
*/
BOOL WINAPI
UpdateDriverForPlugAndPlayDevicesW(
            IN HWND hwndParent,
            IN LPCWSTR HardwareId,
            IN LPCWSTR FullInfPath,
            IN DWORD InstallFlags,
            OUT PBOOL bRebootRequired OPTIONAL)
{
    UNIMPLEMENTED;
    SetLastError(ERROR_GEN_FAILURE);
    return FALSE;
}

/*
* @implemented
*/
BOOL WINAPI
UpdateDriverForPlugAndPlayDevicesA(
               IN HWND hwndParent,
               IN LPCSTR HardwareId,
               IN LPCSTR FullInfPath,
               IN DWORD InstallFlags,
               OUT PBOOL bRebootRequired OPTIONAL)
{
    BOOL Result;
    LPWSTR HardwareIdW = NULL;
    LPWSTR FullInfPathW = NULL;

    int len = MultiByteToWideChar(CP_ACP, 0, HardwareId, -1, NULL, 0);
    HardwareIdW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!HardwareIdW)
    {
        SetLastError(ERROR_GEN_FAILURE);
        return FALSE;
    }
    MultiByteToWideChar(CP_ACP, 0, HardwareId, -1, HardwareIdW, len);

    len = MultiByteToWideChar(CP_ACP, 0, FullInfPath, -1, NULL, 0);
    FullInfPathW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    if (!FullInfPathW)
    {
        HeapFree(GetProcessHeap(), 0, HardwareIdW);
        SetLastError(ERROR_GEN_FAILURE);
        return FALSE;
    }
    MultiByteToWideChar(CP_ACP, 0, FullInfPath, -1, FullInfPathW, len);

    Result = UpdateDriverForPlugAndPlayDevicesW(hwndParent,
        HardwareIdW,
        FullInfPathW,
        InstallFlags,
        bRebootRequired);

    HeapFree(GetProcessHeap(), 0, HardwareIdW);
    HeapFree(GetProcessHeap(), 0, FullInfPathW);

    return Result;
}


static HFONT
CreateTitleFont(VOID)
{
    NONCLIENTMETRICS ncm;
    LOGFONT LogFont;
    HDC hdc;
    INT FontSize;
    HFONT hFont;

    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

    LogFont = ncm.lfMessageFont;
    LogFont.lfWeight = FW_BOLD;
    _tcscpy(LogFont.lfFaceName, _T("MS Shell Dlg"));

    hdc = GetDC(NULL);
    FontSize = 12;
    LogFont.lfHeight = 0 - GetDeviceCaps (hdc, LOGPIXELSY) * FontSize / 72;
    hFont = CreateFontIndirect(&LogFont);
    ReleaseDC(NULL, hdc);

    return hFont;
}

static VOID
CenterWindow(HWND hWnd)
{
    HWND hWndParent;
    RECT rcParent;
    RECT rcWindow;

    hWndParent = GetParent(hWnd);
    if (hWndParent == NULL)
        hWndParent = GetDesktopWindow();

    GetWindowRect(hWndParent, &rcParent);
    GetWindowRect(hWnd, &rcWindow);

    SetWindowPos(hWnd,
        HWND_TOP,
        ((rcParent.right - rcParent.left) - (rcWindow.right - rcWindow.left)) / 2,
        ((rcParent.bottom - rcParent.top) - (rcWindow.bottom - rcWindow.top)) / 2,
        0,
        0,
        SWP_NOSIZE);
}

static INT_PTR CALLBACK
WelcomeDlgProc(
               IN HWND hwndDlg,
               IN UINT uMsg,
               IN WPARAM wParam,
               IN LPARAM lParam)
{

    PDEVINSTDATA DevInstData;

    /* Retrieve pointer to the global setup data */
    DevInstData = (PDEVINSTDATA)GetWindowLongPtr (hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND hwndControl;
            DWORD dwStyle;

            /* Get pointer to the global setup data */
            DevInstData = (PDEVINSTDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)DevInstData);

            hwndControl = GetParent(hwndDlg);

            /* Center the wizard window */
            CenterWindow (hwndControl);

            /* Hide the system menu */
            dwStyle = GetWindowLong(hwndControl, GWL_STYLE);
            SetWindowLong(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);

            /* Set title font */
            SendDlgItemMessage(hwndDlg,
                IDC_WELCOMETITLE,
                WM_SETFONT,
                (WPARAM)DevInstData->hTitleFont,
                (LPARAM)TRUE);

            SendDlgItemMessage(hwndDlg,
                IDC_DEVICE,
                WM_SETTEXT,
                0,
                (LPARAM) DevInstData->buffer);

            SendDlgItemMessage(hwndDlg,
                IDC_RADIO_AUTO,
                BM_SETCHECK,
                (WPARAM) TRUE,
                (LPARAM) 0);


        }
        break;


    case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
            case PSN_SETACTIVE:
                /* Enable the Next button */
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
                break;

            case PSN_WIZNEXT:
                /* Handle a Next button click, if necessary */

                if (SendDlgItemMessage(hwndDlg, IDC_RADIO_AUTO, BM_GETCHECK, (WPARAM) 0, (LPARAM) 0) == BST_CHECKED)
                    PropSheet_SetCurSel(GetParent(hwndDlg), 0, IDD_SEARCHDRV);

                break;

            default:
                break;
            }
        }
        break;

    default:
        break;
    }

    return FALSE;
}

static INT_PTR CALLBACK
CHSourceDlgProc(
                IN HWND hwndDlg,
                IN UINT uMsg,
                IN WPARAM wParam,
                IN LPARAM lParam)
{

    PDEVINSTDATA DevInstData;

    /* Retrieve pointer to the global setup data */
    DevInstData = (PDEVINSTDATA)GetWindowLongPtr (hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND hwndControl;
            DWORD dwStyle;

            /* Get pointer to the global setup data */
            DevInstData = (PDEVINSTDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)DevInstData);

            hwndControl = GetParent(hwndDlg);

            /* Center the wizard window */
            CenterWindow (hwndControl);

            /* Hide the system menu */
            dwStyle = GetWindowLong(hwndControl, GWL_STYLE);
            SetWindowLong(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);

            SendDlgItemMessage(hwndDlg,
                IDC_RADIO_SEARCHHERE,
                BM_SETCHECK,
                (WPARAM) TRUE,
                (LPARAM) 0);

        }
        break;


    case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
            case PSN_SETACTIVE:
                /* Enable the Next and Back buttons */
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
                break;

            case PSN_WIZNEXT:
                /* Handle a Next button click, if necessary */
                PropSheet_SetCurSel(GetParent(hwndDlg), 0, 4);
                break;

            default:
                break;
            }
        }
        break;

    default:
        break;
    }

    return FALSE;
}

static INT_PTR CALLBACK
SearchDrvDlgProc(
                 IN HWND hwndDlg,
                 IN UINT uMsg,
                 IN WPARAM wParam,
                 IN LPARAM lParam)
{

    PDEVINSTDATA DevInstData;
    DWORD dwThreadId;

    /* Retrieve pointer to the global setup data */
    DevInstData = (PDEVINSTDATA)GetWindowLongPtr (hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND hwndControl;
            DWORD dwStyle;

            /* Get pointer to the global setup data */
            DevInstData = (PDEVINSTDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)DevInstData);

            DevInstData->hDialog = hwndDlg;
            hwndControl = GetParent(hwndDlg);

            /* Center the wizard window */
            CenterWindow (hwndControl);

            SendDlgItemMessage(hwndDlg,
                IDC_DEVICE,
                WM_SETTEXT,
                0,
                (LPARAM) DevInstData->buffer);

            /* Hide the system menu */
            dwStyle = GetWindowLong(hwndControl, GWL_STYLE);
            SetWindowLong(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);
        }
        break;

    case WM_SEARCH_FINISHED:
        {
            CloseHandle(hThread);
            hThread = 0;
            if (wParam == 0)
                PropSheet_SetCurSel(GetParent(hwndDlg), 0, IDD_NODRIVER);
            else
                PropSheet_SetCurSel(GetParent(hwndDlg), 0, IDD_FINISHPAGE);
            break;
        }
    case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
            case PSN_SETACTIVE:
                PropSheet_SetWizButtons(GetParent(hwndDlg), !PSWIZB_NEXT | !PSWIZB_BACK);
                hThread = CreateThread( NULL, 0, FindDriverProc, DevInstData, 0, &dwThreadId);
                break;

            case PSN_KILLACTIVE:
                if (hThread != 0)
                {
                    SetWindowLong ( hwndDlg, DWL_MSGRESULT, TRUE);
                    return TRUE;
                }
                break;
            case PSN_WIZNEXT:
                /* Handle a Next button click, if necessary */
                break;

            default:
                break;
            }
        }
        break;

    default:
        break;
    }

    return FALSE;
}

static DWORD WINAPI
FindDriverProc(
               IN LPVOID lpParam)
{
    TCHAR drive[] = {'?',':',0};
    size_t nType;
    DWORD dwDrives;
    PDEVINSTDATA DevInstData;
    DWORD config_flags;
    UINT i = 1;

    DevInstData = (PDEVINSTDATA)lpParam;

    dwDrives = GetLogicalDrives();
    for (drive[0] = 'A'; drive[0] <= 'Z'; drive[0]++)
    {
        if (dwDrives & i)
        {
            nType = GetDriveType( drive );
            if ((nType == DRIVE_CDROM))
                //if ((nType == DRIVE_CDROM) || (nType == DRIVE_FIXED))
            {
                /* search for inf file */
                if (SearchDriver ( DevInstData, drive ))
                {
                    /* if we found a valid driver inf... */
                    if (FindDriver ( DevInstData ))
                    {
                        InstallDriver ( DevInstData );
                        PostMessage(DevInstData->hDialog, WM_SEARCH_FINISHED, 1, 0);
                        return 0;
                    }
                }
            }
        }
        i <<= 1;
    }

    /* update device configuration */
    if(SetupDiGetDeviceRegistryProperty(DevInstData->hDevInfo,
        &DevInstData->devInfoData,
        SPDRP_CONFIGFLAGS,
        NULL,
        (BYTE *)&config_flags,
        sizeof(config_flags),
        NULL))
    {
        config_flags |= CONFIGFLAG_FAILEDINSTALL;
        SetupDiSetDeviceRegistryProperty(
            DevInstData->hDevInfo,
            &DevInstData->devInfoData,
            SPDRP_CONFIGFLAGS,
            (BYTE *)&config_flags, sizeof(config_flags) );
    }

    PostMessage(DevInstData->hDialog, WM_SEARCH_FINISHED, 0, 0);
    return 0;
}

static INT_PTR CALLBACK
FinishDlgProc(
              IN HWND hwndDlg,
              IN UINT uMsg,
              IN WPARAM wParam,
              IN LPARAM lParam)
{

    PDEVINSTDATA DevInstData;

    /* Retrieve pointer to the global setup data */
    DevInstData = (PDEVINSTDATA)GetWindowLongPtr (hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND hwndControl;

            /* Get pointer to the global setup data */
            DevInstData = (PDEVINSTDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)DevInstData);

            hwndControl = GetDlgItem(GetParent(hwndDlg), IDCANCEL);
            ShowWindow (hwndControl, SW_HIDE);
            EnableWindow (hwndControl, FALSE);

            SendDlgItemMessage(hwndDlg,
                IDC_DEVICE,
                WM_SETTEXT,
                0,
                (LPARAM) DevInstData->drvInfoData.Description);

            /* Set title font */
            SendDlgItemMessage(hwndDlg,
                IDC_FINISHTITLE,
                WM_SETFONT,
                (WPARAM)DevInstData->hTitleFont,
                (LPARAM)TRUE);
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
            case PSN_SETACTIVE:
                /* Enable the correct buttons on for the active page */
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_FINISH);
                break;

            case PSN_WIZBACK:
                /* Handle a Back button click, if necessary */
                break;

            case PSN_WIZFINISH:
                /* Handle a Finish button click, if necessary */
                break;

            default:
                break;
            }
        }
        break;

    default:
        break;
    }

    return FALSE;
}

static INT_PTR CALLBACK
InstFailDlgProc(
                IN HWND hwndDlg,
                IN UINT uMsg,
                IN WPARAM wParam,
                IN LPARAM lParam)
{

    PDEVINSTDATA DevInstData;

    /* Get pointer to the global setup data */
    DevInstData = (PDEVINSTDATA)GetWindowLongPtr (hwndDlg, GWL_USERDATA);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND hwndControl;
            BOOL DisableableDevice = FALSE;

            DevInstData = (PDEVINSTDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWL_USERDATA, (DWORD_PTR)DevInstData);

            hwndControl = GetDlgItem(GetParent(hwndDlg), IDCANCEL);
            ShowWindow (hwndControl, SW_HIDE);
            EnableWindow (hwndControl, FALSE);

            /* Set title font */
            SendDlgItemMessage(hwndDlg,
                IDC_FINISHTITLE,
                WM_SETFONT,
                (WPARAM)DevInstData->hTitleFont,
                (LPARAM)TRUE);

            /* disable the "do not show this dialog anymore" checkbox
               if the device cannot be disabled */
            CanDisableDevice(DevInstData->devInfoData.DevInst,
                             NULL,
                             &DisableableDevice);
            EnableWindow(GetDlgItem(hwndDlg,
                                    IDC_DONOTSHOWDLG),
                         DisableableDevice);
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
            case PSN_SETACTIVE:
                /* Enable the correct buttons on for the active page */
                PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_FINISH);
                break;

            case PSN_WIZBACK:
                PropSheet_SetCurSel(GetParent(hwndDlg), 0, IDD_WELCOMEPAGE);
                /* Handle a Back button click, if necessary */
                break;

            case PSN_WIZFINISH:
            {
                BOOL DisableableDevice = FALSE;
                BOOL IsStarted = FALSE;

                if (CanDisableDevice(DevInstData->devInfoData.DevInst,
                                     NULL,
                                     &DisableableDevice) &&
                    DisableableDevice &&
                    IsDeviceStarted(DevInstData->devInfoData.DevInst,
                                    NULL,
                                    &IsStarted) &&
                    !IsStarted &&
                    SendDlgItemMessage(hwndDlg, IDC_DONOTSHOWDLG, BM_GETCHECK, (WPARAM) 0, (LPARAM) 0) == BST_CHECKED)
                {
                    /* disable the device */
                    StartDevice(DevInstData->hDevInfo,
                                &DevInstData->devInfoData,
                                FALSE,
                                0,
                                NULL);
                }
                break;
            }

            default:
                break;
            }
        }
        break;

    default:
        break;
    }

    return FALSE;
}


static BOOL
FindDriver(
           IN PDEVINSTDATA DevInstData)
{
    SP_DEVINSTALL_PARAMS DevInstallParams = {0,};
    BOOL ret;

    DevInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (!SetupDiGetDeviceInstallParams(DevInstData->hDevInfo, &DevInstData->devInfoData, &DevInstallParams))
    {
        TRACE("SetupDiGetDeviceInstallParams() failed with error 0x%lx\n", GetLastError());
        return FALSE;
    }
    DevInstallParams.FlagsEx |= DI_FLAGSEX_ALLOWEXCLUDEDDRVS;
    if (!SetupDiSetDeviceInstallParams(DevInstData->hDevInfo, &DevInstData->devInfoData, &DevInstallParams))
    {
        TRACE("SetupDiSetDeviceInstallParams() failed with error 0x%lx\n", GetLastError());
        return FALSE;
    }

    ret = SetupDiBuildDriverInfoList(DevInstData->hDevInfo, &DevInstData->devInfoData, SPDIT_COMPATDRIVER);
    if (!ret)
    {
        TRACE("SetupDiBuildDriverInfoList() failed with error 0x%lx\n", GetLastError());
        return FALSE;
    }

    DevInstData->drvInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    ret = SetupDiEnumDriverInfo(
        DevInstData->hDevInfo,
        &DevInstData->devInfoData,
        SPDIT_COMPATDRIVER,
        0,
        &DevInstData->drvInfoData);
    if (!ret)
    {
        if (GetLastError() == ERROR_NO_MORE_ITEMS)
            return FALSE;
        TRACE("SetupDiEnumDriverInfo() failed with error 0x%lx\n", GetLastError());
        return FALSE;
    }
    TRACE("Installing driver %S: %S\n", DevInstData->drvInfoData.MfgName, DevInstData->drvInfoData.Description);

    return TRUE;
}


static BOOL
IsDots(IN LPCTSTR str)
{
    if(_tcscmp(str, _T(".")) && _tcscmp(str, _T(".."))) return FALSE;
    return TRUE;
}

static LPTSTR
GetFileExt(IN LPTSTR FileName)
{
    if (FileName == 0)
        return _T("");

    int i = _tcsclen(FileName);
    while ((i >= 0) && (FileName[i] != _T('.')))
        i--;

    FileName = _tcslwr(FileName);

    if (i >= 0)
        return &FileName[i];
    else
        return _T("");
}

static BOOL
SearchDriver(
             IN PDEVINSTDATA DevInstData,
             IN LPCTSTR Path)
{
    WIN32_FIND_DATA wfd;
    SP_DEVINSTALL_PARAMS DevInstallParams;
    TCHAR DirPath[MAX_PATH];
    TCHAR FileName[MAX_PATH];
    TCHAR FullPath[MAX_PATH];
    TCHAR LastDirPath[MAX_PATH] = _T("");
    TCHAR PathWithPattern[MAX_PATH];
    BOOL ok = TRUE;
    BOOL ret;
    HANDLE hFindFile;

    _tcscpy(DirPath, Path);

    if (DirPath[_tcsclen(DirPath) - 1] != '\\')
        _tcscat(DirPath, _T("\\"));

    _tcscpy(PathWithPattern, DirPath);
    _tcscat(PathWithPattern, _T("\\*"));

    for (hFindFile = FindFirstFile(PathWithPattern, &wfd); ((hFindFile != INVALID_HANDLE_VALUE) && ok); ok = FindNextFile(hFindFile, &wfd))
    {

        _tcscpy(FileName, wfd.cFileName);
        if (IsDots(FileName)) continue;

        if((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            _tcscpy(FullPath, DirPath);
            _tcscat(FullPath, FileName);
            if(SearchDriver(DevInstData, FullPath))
                break;
        }
        else
        {
            LPCTSTR pszExtension = GetFileExt(FileName);

            if ((_tcscmp(pszExtension, _T(".inf")) == 0) && (_tcscmp(LastDirPath, DirPath) != 0))
            {
                _tcscpy(LastDirPath, DirPath);
                ZeroMemory (&DevInstallParams, sizeof(SP_DEVINSTALL_PARAMS));
                DevInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

                ret = SetupDiGetDeviceInstallParams(
                    DevInstData->hDevInfo,
                    &DevInstData->devInfoData,
                    &DevInstallParams);

                if (_tcsclen(DirPath) <= MAX_PATH)
                {
                    memcpy(DevInstallParams.DriverPath, DirPath, (_tcsclen(DirPath) + 1) *  sizeof(TCHAR));
                }

                ret = SetupDiSetDeviceInstallParams(
                    DevInstData->hDevInfo,
                    &DevInstData->devInfoData,
                    &DevInstallParams);

                if ( FindDriver ( DevInstData ) )
                {
                    if (hFindFile != INVALID_HANDLE_VALUE)
                        FindClose(hFindFile);
                    return TRUE;
                }

            }
        }
    }

    if (hFindFile != INVALID_HANDLE_VALUE)
        FindClose(hFindFile);

    return FALSE;
}

static BOOL
InstallDriver(
              IN PDEVINSTDATA DevInstData)
{

    BOOL ret;

    ret = SetupDiCallClassInstaller(
        DIF_SELECTBESTCOMPATDRV,
        DevInstData->hDevInfo,
        &DevInstData->devInfoData);
    if (!ret)
    {
        TRACE("SetupDiCallClassInstaller(DIF_SELECTBESTCOMPATDRV) failed with error 0x%lx\n", GetLastError());
        return FALSE;
    }

    ret = SetupDiCallClassInstaller(
        DIF_ALLOW_INSTALL,
        DevInstData->hDevInfo,
        &DevInstData->devInfoData);
    if (!ret)
    {
        TRACE("SetupDiCallClassInstaller(DIF_ALLOW_INSTALL) failed with error 0x%lx\n", GetLastError());
        return FALSE;
    }

    ret = SetupDiCallClassInstaller(
        DIF_NEWDEVICEWIZARD_PREANALYZE,
        DevInstData->hDevInfo,
        &DevInstData->devInfoData);
    if (!ret)
    {
        TRACE("SetupDiCallClassInstaller(DIF_NEWDEVICEWIZARD_PREANALYZE) failed with error 0x%lx\n", GetLastError());
        return FALSE;
    }

    ret = SetupDiCallClassInstaller(
        DIF_NEWDEVICEWIZARD_POSTANALYZE,
        DevInstData->hDevInfo,
        &DevInstData->devInfoData);
    if (!ret)
    {
        TRACE("SetupDiCallClassInstaller(DIF_NEWDEVICEWIZARD_POSTANALYZE) failed with error 0x%lx\n", GetLastError());
        return FALSE;
    }

    ret = SetupDiCallClassInstaller(
        DIF_INSTALLDEVICEFILES,
        DevInstData->hDevInfo,
        &DevInstData->devInfoData);
    if (!ret)
    {
        TRACE("SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES) failed with error 0x%lx\n", GetLastError());
        return FALSE;
    }

    ret = SetupDiCallClassInstaller(
        DIF_REGISTER_COINSTALLERS,
        DevInstData->hDevInfo,
        &DevInstData->devInfoData);
    if (!ret)
    {
        TRACE("SetupDiCallClassInstaller(DIF_REGISTER_COINSTALLERS) failed with error 0x%lx\n", GetLastError());
        return FALSE;
    }

    ret = SetupDiCallClassInstaller(
        DIF_INSTALLINTERFACES,
        DevInstData->hDevInfo,
        &DevInstData->devInfoData);
    if (!ret)
    {
        TRACE("SetupDiCallClassInstaller(DIF_INSTALLINTERFACES) failed with error 0x%lx\n", GetLastError());
        return FALSE;
    }

    ret = SetupDiCallClassInstaller(
        DIF_INSTALLDEVICE,
        DevInstData->hDevInfo,
        &DevInstData->devInfoData);
    if (!ret)
    {
        TRACE("SetupDiCallClassInstaller(DIF_INSTALLDEVICE) failed with error 0x%lx\n", GetLastError());
        return FALSE;
    }

    ret = SetupDiCallClassInstaller(
        DIF_NEWDEVICEWIZARD_FINISHINSTALL,
        DevInstData->hDevInfo,
        &DevInstData->devInfoData);
    if (!ret)
    {
        TRACE("SetupDiCallClassInstaller(DIF_NEWDEVICEWIZARD_FINISHINSTALL) failed with error 0x%lx\n", GetLastError());
        return FALSE;
    }

    ret = SetupDiCallClassInstaller(
        DIF_DESTROYPRIVATEDATA,
        DevInstData->hDevInfo,
        &DevInstData->devInfoData);
    if (!ret)
    {
        TRACE("SetupDiCallClassInstaller(DIF_DESTROYPRIVATEDATA) failed with error 0x%lx\n", GetLastError());
        return FALSE;
    }

    return TRUE;

}

static VOID
CleanUp(VOID)
{

    if (DevInstData.devInfoData.cbSize != 0)
    {
        if (!SetupDiDestroyDriverInfoList(DevInstData.hDevInfo, &DevInstData.devInfoData, SPDIT_COMPATDRIVER))
            TRACE("SetupDiDestroyDriverInfoList() failed with error 0x%lx\n", GetLastError());
    }

    if (DevInstData.hDevInfo != INVALID_HANDLE_VALUE)
    {
        if (!SetupDiDestroyDeviceInfoList(DevInstData.hDevInfo))
            TRACE("SetupDiDestroyDeviceInfoList() failed with error 0x%lx\n", GetLastError());
    }

    if (DevInstData.buffer)
        HeapFree(GetProcessHeap(), 0, DevInstData.buffer);

}

/*
* @implemented
*/
BOOL WINAPI
DevInstallW(
            IN HWND hWndParent,
            IN HINSTANCE hInstance,
            IN LPCWSTR InstanceId,
            IN INT Show)
{

    PROPSHEETHEADER psh;
    HPROPSHEETPAGE ahpsp[5];
    PROPSHEETPAGE psp;
    BOOL ret;
    DWORD config_flags;

    if (!IsUserAdmin())
    {
        /* XP kills the process... */
        ExitProcess(ERROR_ACCESS_DENIED);
    }

    /* Clear devinst data */
    ZeroMemory(&DevInstData, sizeof(DEVINSTDATA));
    DevInstData.devInfoData.cbSize = 0; /* Tell if the devInfoData is valid */


    DevInstData.hDevInfo = SetupDiCreateDeviceInfoListExW(NULL, NULL, NULL, NULL);
    if (DevInstData.hDevInfo == INVALID_HANDLE_VALUE)
    {
        TRACE("SetupDiCreateDeviceInfoListExW() failed with error 0x%lx\n", GetLastError());
        CleanUp();
        return FALSE;
    }

    DevInstData.devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    ret = SetupDiOpenDeviceInfoW(
        DevInstData.hDevInfo,
        InstanceId,
        NULL,
        0, /* Open flags */
        &DevInstData.devInfoData);
    if (!ret)
    {
        TRACE("SetupDiOpenDeviceInfoW() failed with error 0x%lx (InstanceId %S)\n", GetLastError(), InstanceId);
        DevInstData.devInfoData.cbSize = 0;
        CleanUp();
        return FALSE;
    }

    SetLastError(ERROR_GEN_FAILURE);
    ret = SetupDiGetDeviceRegistryProperty(
        DevInstData.hDevInfo,
        &DevInstData.devInfoData,
        SPDRP_DEVICEDESC,
        &DevInstData.regDataType,
        NULL, 0,
        &DevInstData.requiredSize);

    if (!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER && DevInstData.regDataType == REG_SZ)
    {
        DevInstData.buffer = HeapAlloc(GetProcessHeap(), 0, DevInstData.requiredSize);
        if (!DevInstData.buffer)
        {
            TRACE("HeapAlloc() failed\n");
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }
        else
        {
            ret = SetupDiGetDeviceRegistryProperty(
                DevInstData.hDevInfo,
                &DevInstData.devInfoData,
                SPDRP_DEVICEDESC,
                &DevInstData.regDataType,
                DevInstData.buffer, DevInstData.requiredSize,
                &DevInstData.requiredSize);
        }
    }
    if (!ret)
    {
        TRACE("SetupDiGetDeviceRegistryProperty() failed with error 0x%lx (InstanceId %S)\n", GetLastError(), InstanceId);
        CleanUp();
        return FALSE;
    }

    if(SetupDiGetDeviceRegistryProperty(DevInstData.hDevInfo,
        &DevInstData.devInfoData,
        SPDRP_CONFIGFLAGS,
        NULL,
        (BYTE *)&config_flags,
        sizeof(config_flags),
        NULL))
    {
        if (config_flags & CONFIGFLAG_FAILEDINSTALL)
        {
            CleanUp();
            return TRUE;
        }   
    }
    /*
    else 
    {
        swprintf(buf, _T("%ld"), GetLastError()); 
        MessageBox(0,buf,buf,0);
    }
    */

    TRACE("Installing %S (%S)\n", DevInstData.buffer, InstanceId);

    if ((!FindDriver(&DevInstData)) && (Show != SW_HIDE))
    {

        /* Create the Welcome page */
        ZeroMemory (&psp, sizeof(PROPSHEETPAGE));
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
        psp.hInstance = hDllInstance;
        psp.lParam = (LPARAM)&DevInstData;
        psp.pfnDlgProc = WelcomeDlgProc;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_WELCOMEPAGE);
        ahpsp[IDD_WELCOMEPAGE] = CreatePropertySheetPage(&psp);

        /* Create the Select Source page */
        psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pfnDlgProc = CHSourceDlgProc;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_CHSOURCE);
        ahpsp[IDD_CHSOURCE] = CreatePropertySheetPage(&psp);

        /* Create the Search driver page */
        psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pfnDlgProc = SearchDrvDlgProc;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_SEARCHDRV);
        ahpsp[IDD_SEARCHDRV] = CreatePropertySheetPage(&psp);

        /* Create the Finish page */
        psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
        psp.pfnDlgProc = FinishDlgProc;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_FINISHPAGE);
        ahpsp[IDD_FINISHPAGE] = CreatePropertySheetPage(&psp);

        /* Create the Install failed page */
        psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
        psp.pfnDlgProc = InstFailDlgProc;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_NODRIVER);
        ahpsp[IDD_NODRIVER] = CreatePropertySheetPage(&psp);

        /* Create the property sheet */
        psh.dwSize = sizeof(PROPSHEETHEADER);
        psh.dwFlags = PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
        psh.hInstance = hDllInstance;
        psh.hwndParent = NULL;
        psh.nPages = 5;
        psh.nStartPage = 0;
        psh.phpage = ahpsp;
        psh.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK);
        psh.pszbmHeader = MAKEINTRESOURCE(IDB_HEADER);

        /* Create title font */
        DevInstData.hTitleFont = CreateTitleFont();

        /* Display the wizard */
        PropertySheet(&psh);

        DeleteObject(DevInstData.hTitleFont);

    }
    else
    {
        InstallDriver ( &DevInstData );
    }

    CleanUp();
    return TRUE;
}

/*
* @unimplemented
*/
BOOL WINAPI
ClientSideInstallW(IN HWND hWndOwner,
                   IN DWORD dwUnknownFlags,
                   IN LPWSTR lpNamedPipeName)
{
    /* NOTE: pNamedPipeName is in the format:
     *       "\\.\pipe\PNP_Device_Install_Pipe_0.{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
     */
    return FALSE;
}

BOOL WINAPI
DllMain(
        IN HINSTANCE hInstance,
        IN DWORD dwReason,
        IN LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        INITCOMMONCONTROLSEX InitControls;

        DisableThreadLibraryCalls(hInstance);

        InitControls.dwSize = sizeof(INITCOMMONCONTROLSEX);
        InitControls.dwICC = ICC_PROGRESS_CLASS;
        InitCommonControlsEx(&InitControls);
        hDllInstance = hInstance;
    }

    return TRUE;
}
