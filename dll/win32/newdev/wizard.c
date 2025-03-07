/*
 * New device installer (newdev.dll)
 *
 * Copyright 2006 Hervé Poussineau (hpoussin@reactos.org)
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

#include "newdev_private.h"

#include <wincon.h>
#include <cfgmgr32.h>
#include <shlobj.h>
#include <shlwapi.h>

HANDLE hThread;

static VOID
CenterWindow(
    IN HWND hWnd)
{
    HWND hWndParent;
    RECT rcParent;
    RECT rcWindow;

    hWndParent = GetParent(hWnd);
    if (hWndParent == NULL)
        hWndParent = GetDesktopWindow();

    GetWindowRect(hWndParent, &rcParent);
    GetWindowRect(hWnd, &rcWindow);

    /* Check if the child window fits inside the parent window */
    if (rcWindow.left < rcParent.left || rcWindow.top < rcParent.top ||
        rcWindow.right > rcParent.right || rcWindow.bottom > rcParent.bottom)
    {
        return;
    }

    SetWindowPos(
        hWnd,
        HWND_TOP,
        ((rcParent.right - rcParent.left) - (rcWindow.right - rcWindow.left)) / 2,
        ((rcParent.bottom - rcParent.top) - (rcWindow.bottom - rcWindow.top)) / 2,
        0,
        0,
        SWP_NOSIZE);
}

static BOOL
SetFailedInstall(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DevInfoData OPTIONAL,
    IN BOOLEAN Set)
{
    DWORD dwType, dwSize, dwFlags = 0;

    dwSize = sizeof(dwFlags);
    if (!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                          DevInfoData,
                                          SPDRP_CONFIGFLAGS,
                                          &dwType,
                                          (PBYTE)&dwFlags,
                                          dwSize,
                                          &dwSize))
    {
        return FALSE;
    }

    if (Set)
        dwFlags |= CONFIGFLAG_FAILEDINSTALL;
    else
        dwFlags &= ~CONFIGFLAG_FAILEDINSTALL;

    if (!SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
                                          DevInfoData,
                                          SPDRP_CONFIGFLAGS,
                                          (PBYTE)&dwFlags,
                                          dwSize))
    {

        return FALSE;
    }

    if (Set)
    {
        /* Set the 'Unknown' device class */
        PWSTR pszUnknown = L"Unknown";
        SetupDiSetDeviceRegistryPropertyW(DeviceInfoSet,
                                          DevInfoData,
                                          SPDRP_CLASS,
                                          (PBYTE)pszUnknown,
                                          (wcslen(pszUnknown) + 1) * sizeof(WCHAR));

        PWSTR pszUnknownGuid = L"{4D36E97E-E325-11CE-BFC1-08002BE10318}";
        SetupDiSetDeviceRegistryPropertyW(DeviceInfoSet,
                                          DevInfoData,
                                          SPDRP_CLASSGUID,
                                          (PBYTE)pszUnknownGuid,
                                          (wcslen(pszUnknownGuid) + 1) * sizeof(WCHAR));

        /* Set device problem code CM_PROB_FAILED_INSTALL */
        CM_Set_DevNode_Problem(DevInfoData->DevInst,
                               CM_PROB_FAILED_INSTALL,
                               CM_SET_DEVNODE_PROBLEM_OVERRIDE);
    }

    return TRUE;
}

static BOOL
CanDisableDevice(
    IN DEVINST DevInst,
    IN HMACHINE hMachine,
    OUT BOOL *CanDisable)
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
        *CanDisable = ((Status & DN_DISABLEABLE) != 0);
        Ret = TRUE;
    }

    return Ret;
}

static BOOL
IsDeviceStarted(
    IN DEVINST DevInst,
    IN HMACHINE hMachine,
    OUT BOOL *IsEnabled)
{
    CONFIGRET cr;
    ULONG Status, ProblemNumber;
    BOOL Ret = FALSE;

    cr = CM_Get_DevNode_Status_Ex(
        &Status,
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
StartDevice(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DevInfoData OPTIONAL,
    IN BOOL bEnable,
    IN DWORD HardwareProfile OPTIONAL,
    OUT BOOL *bNeedReboot OPTIONAL)
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
        if (SetupDiSetClassInstallParams(
            DeviceInfoSet,
            DevInfoData,
            &pcp.ClassInstallHeader,
            sizeof(SP_PROPCHANGE_PARAMS)))
        {
            SetupDiCallClassInstaller(
                DIF_PROPERTYCHANGE,
                DeviceInfoSet,
                DevInfoData);
        }
        SetLastError(LastErr);
    }

    /* try config-specific */
    pcp.StateChange = (bEnable ? DICS_ENABLE : DICS_DISABLE);
    pcp.Scope = DICS_FLAG_CONFIGSPECIFIC;

    if (SetupDiSetClassInstallParams(
            DeviceInfoSet,
            DevInfoData,
            &pcp.ClassInstallHeader,
            sizeof(SP_PROPCHANGE_PARAMS)) &&
        SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,
            DeviceInfoSet,
            DevInfoData))
    {
        dp.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        if (SetupDiGetDeviceInstallParams(
            DeviceInfoSet,
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

static DWORD WINAPI
FindDriverProc(
    IN LPVOID lpParam)
{
    PDEVINSTDATA DevInstData;
    BOOL result = FALSE;

    DevInstData = (PDEVINSTDATA)lpParam;

    result = ScanFoldersForDriver(DevInstData);

    if (result)
    {
        PostMessage(DevInstData->hDialog, WM_SEARCH_FINISHED, 1, 0);
    }
    else
    {
        if (!DevInstData->bUpdate)
        {
            /* Update device configuration */
            SetFailedInstall(DevInstData->hDevInfo,
                             &DevInstData->devInfoData,
                             TRUE);
        }
        PostMessage(DevInstData->hDialog, WM_SEARCH_FINISHED, 0, 0);
    }
    return 0;
}

static DWORD WINAPI
InstallDriverProc(
    IN LPVOID lpParam)
{
    PDEVINSTDATA DevInstData;
    BOOL res;

    DevInstData = (PDEVINSTDATA)lpParam;
    res = InstallCurrentDriver(DevInstData);
    PostMessage(DevInstData->hDialog, WM_INSTALL_FINISHED, res ? 0 : 1, 0);
    return 0;
}

static VOID
PopulateCustomPathCombo(
    IN HWND hwndCombo)
{
    HKEY hKey = NULL;
    DWORD dwRegType;
    DWORD dwPathLength = 0;
    LPWSTR Buffer = NULL;
    LPCWSTR Path;
    LONG rc;

    (void)ComboBox_ResetContent(hwndCombo);

    /* RegGetValue would have been better... */
    rc = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        REGSTR_PATH_SETUP REGSTR_KEY_SETUP,
        0,
        KEY_QUERY_VALUE,
        &hKey);
    if (rc != ERROR_SUCCESS)
    {
        TRACE("RegOpenKeyEx() failed with error 0x%lx\n", rc);
        goto cleanup;
    }
    rc = RegQueryValueExW(
        hKey,
        L"Installation Sources",
        NULL,
        &dwRegType,
        NULL,
        &dwPathLength);
    if (rc != ERROR_SUCCESS || dwRegType != REG_MULTI_SZ)
    {
        TRACE("RegQueryValueEx() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    /* Allocate enough space to add 2 NULL chars at the end of the string */
    Buffer = HeapAlloc(GetProcessHeap(), 0, dwPathLength + 2 * sizeof(WCHAR));
    if (!Buffer)
    {
        TRACE("HeapAlloc() failed\n");
        goto cleanup;
    }
    rc = RegQueryValueExW(
        hKey,
        L"Installation Sources",
        NULL,
        NULL,
        (LPBYTE)Buffer,
        &dwPathLength);
    if (rc != ERROR_SUCCESS)
    {
        TRACE("RegQueryValueEx() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    Buffer[dwPathLength / sizeof(WCHAR)] = UNICODE_NULL;
    Buffer[dwPathLength / sizeof(WCHAR) + 1] = UNICODE_NULL;

    /* Populate combo box */
    for (Path = Buffer; *Path; Path += wcslen(Path) + 1)
    {
        (void)ComboBox_AddString(hwndCombo, Path);
        if (Path == Buffer)
            (void)ComboBox_SetCurSel(hwndCombo, 0);
    }

cleanup:
    if (hKey != NULL)
        RegCloseKey(hKey);
    HeapFree(GetProcessHeap(), 0, Buffer);
}

static VOID
SaveCustomPath(
    IN HWND hwndCombo)
{
    LPWSTR CustomPath = NULL;
    DWORD CustomPathLength;
    LPWSTR Buffer = NULL;
    LPWSTR pBuffer; /* Pointer into Buffer */
    int ItemsCount, Length;
    int i;
    DWORD TotalLength = 0;
    BOOL UseCustomPath = TRUE;
    HKEY hKey = NULL;
    LONG rc;

    /* Get custom path */
    Length = ComboBox_GetTextLength(hwndCombo) + 1;
    CustomPath = HeapAlloc(GetProcessHeap(), 0, Length * sizeof(WCHAR));
    if (!CustomPath)
    {
        TRACE("HeapAlloc() failed\n");
        goto cleanup;
    }
    CustomPathLength = GetWindowTextW(hwndCombo, CustomPath, Length) + 1;

    /* Calculate length of the buffer */
    ItemsCount = ComboBox_GetCount(hwndCombo);
    if (ItemsCount == CB_ERR)
    {
        TRACE("ComboBox_GetCount() failed\n");
        goto cleanup;
    }
    for (i = 0; i < ItemsCount; i++)
    {
        Length = ComboBox_GetLBTextLen(hwndCombo, i);
        if (Length == CB_ERR)
        {
            TRACE("ComboBox_GetLBTextLen() failed\n");
            goto cleanup;
        }
        TotalLength += Length + 1;
    }
    TotalLength++; /* Final NULL char */

    /* Allocate buffer */
    Buffer = HeapAlloc(GetProcessHeap(), 0, (CustomPathLength + TotalLength + 1) * sizeof(WCHAR));
    if (!Buffer)
    {
        TRACE("HeapAlloc() failed\n");
        goto cleanup;
    }

    /* Fill the buffer */
    pBuffer = &Buffer[CustomPathLength];
    for (i = 0; i < ItemsCount; i++)
    {
        Length = ComboBox_GetLBText(hwndCombo, i, pBuffer);
        if (Length == CB_ERR)
        {
            TRACE("ComboBox_GetLBText() failed\n");
            goto cleanup;
        }
        else if (UseCustomPath && _wcsicmp(CustomPath, pBuffer) == 0)
            UseCustomPath = FALSE;
        pBuffer += 1 + Length;
    }
    *pBuffer = '\0'; /* Add final NULL char */

    if (!UseCustomPath)
    {
        /* Nothing to save to registry */
        goto cleanup;
    }

    TotalLength += CustomPathLength;
    wcscpy(Buffer, CustomPath);

    /* Save the buffer */
    /* RegSetKeyValue would have been better... */
    rc = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        REGSTR_PATH_SETUP REGSTR_KEY_SETUP,
        0,
        KEY_SET_VALUE,
        &hKey);
    if (rc != ERROR_SUCCESS)
    {
        TRACE("RegOpenKeyEx() failed with error 0x%lx\n", rc);
        goto cleanup;
    }
    rc = RegSetValueExW(
        hKey,
        L"Installation Sources",
        0,
        REG_MULTI_SZ,
        (const BYTE*)Buffer,
        TotalLength * sizeof(WCHAR));
    if (rc != ERROR_SUCCESS)
    {
        TRACE("RegSetValueEx() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

cleanup:
    if (hKey != NULL)
        RegCloseKey(hKey);
    HeapFree(GetProcessHeap(), 0, CustomPath);
    HeapFree(GetProcessHeap(), 0, Buffer);
}

static INT_PTR CALLBACK
WelcomeDlgProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PDEVINSTDATA DevInstData;
    UNREFERENCED_PARAMETER(wParam);

    /* Retrieve pointer to the global setup data */
    DevInstData = (PDEVINSTDATA)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HWND hwndControl;
            DWORD dwStyle;

            /* Get pointer to the global setup data */
            DevInstData = (PDEVINSTDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (DWORD_PTR)DevInstData);

            hwndControl = GetParent(hwndDlg);

            /* Center the wizard window */
            CenterWindow(hwndControl);

            /* Hide the system menu */
            dwStyle = GetWindowLongPtr(hwndControl, GWL_STYLE);
            SetWindowLongPtr(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);

            /* Set title font */
            SendDlgItemMessage(
                hwndDlg,
                IDC_WELCOMETITLE,
                WM_SETFONT,
                (WPARAM)DevInstData->hTitleFont,
                (LPARAM)TRUE);

            SendDlgItemMessage(
                hwndDlg,
                IDC_DEVICE,
                WM_SETTEXT,
                0,
                (LPARAM)DevInstData->buffer);

            SendDlgItemMessage(
                hwndDlg,
                IDC_RADIO_AUTO,
                BM_SETCHECK,
                (WPARAM)TRUE,
                (LPARAM)0);

            if (!DevInstData->bUpdate)
            {
                SetFailedInstall(DevInstData->hDevInfo,
                                 &DevInstData->devInfoData,
                                 TRUE);
            }
            break;
        }

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
                    if (SendDlgItemMessage(hwndDlg, IDC_RADIO_AUTO, BM_GETCHECK, (WPARAM)0, (LPARAM)0) == BST_CHECKED)
                    {
                        if (PrepareFoldersToScan(DevInstData, TRUE, FALSE, NULL))
                            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_SEARCHDRV);
                        else
                            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_INSTALLFAILED);
                    }
                    return TRUE;

                default:
                    break;
            }
            break;
        }

        default:
            break;
    }

    return FALSE;
}

static void
IncludePath(HWND Dlg, BOOL Enabled)
{
    EnableWindow(GetDlgItem(Dlg, IDC_COMBO_PATH), Enabled);
    EnableWindow(GetDlgItem(Dlg, IDC_BROWSE), Enabled);
}

static void
AutoDriver(HWND Dlg, BOOL Enabled)
{
    EnableWindow(GetDlgItem(Dlg, IDC_CHECK_MEDIA), Enabled);
    EnableWindow(GetDlgItem(Dlg, IDC_CHECK_PATH), Enabled);
    IncludePath(Dlg, Enabled & IsDlgButtonChecked(Dlg, IDC_CHECK_PATH));
}

static INT CALLBACK
BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    BOOL bValid = FALSE;

    switch (uMsg)
    {
        case BFFM_INITIALIZED:
        {
            PCWSTR pszPath = ((PDEVINSTDATA)lpData)->CustomSearchPath;

            bValid = CheckBestDriver((PDEVINSTDATA)lpData, pszPath);
            SendMessageW(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)pszPath);
            SendMessageW(hwnd, BFFM_ENABLEOK, 0, bValid);
            break;
        }

        case BFFM_SELCHANGED:
        {
            WCHAR szDir[MAX_PATH];

            if (SHGetPathFromIDListW((LPITEMIDLIST)lParam, szDir))
            {
                bValid = CheckBestDriver((PDEVINSTDATA)lpData, szDir);
            }
            PostMessageW(hwnd, BFFM_ENABLEOK, 0, bValid);
            break;
        }
    }
    return 0;
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
    DevInstData = (PDEVINSTDATA)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HWND hwndControl, hwndCombo;
            DWORD dwStyle;
            COMBOBOXINFO info = { sizeof(info) };

            /* Get pointer to the global setup data */
            DevInstData = (PDEVINSTDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (DWORD_PTR)DevInstData);

            hwndControl = GetParent(hwndDlg);

            /* Center the wizard window */
            CenterWindow(hwndControl);

            /* Hide the system menu */
            dwStyle = GetWindowLongPtr(hwndControl, GWL_STYLE);
            SetWindowLongPtr(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);

            hwndCombo = GetDlgItem(hwndDlg, IDC_COMBO_PATH);
            PopulateCustomPathCombo(hwndCombo);

            GetComboBoxInfo(hwndCombo, &info);
            SHAutoComplete(info.hwndItem, SHACF_FILESYS_DIRS);

            SendDlgItemMessage(
                hwndDlg,
                IDC_RADIO_SEARCHHERE,
                BM_SETCHECK,
                (WPARAM)TRUE,
                (LPARAM)0);
            AutoDriver(hwndDlg, TRUE);
            IncludePath(hwndDlg, FALSE);

            /* Disable manual driver choice for now */
            EnableWindow(GetDlgItem(hwndDlg, IDC_RADIO_CHOOSE), FALSE);

            break;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_RADIO_SEARCHHERE:
                    AutoDriver(hwndDlg, TRUE);
                    return TRUE;

                case IDC_RADIO_CHOOSE:
                    AutoDriver(hwndDlg, FALSE);
                    return TRUE;

                case IDC_CHECK_PATH:
                    IncludePath(hwndDlg, IsDlgButtonChecked(hwndDlg, IDC_CHECK_PATH));
                    return TRUE;

                case IDC_BROWSE:
                {
                    BROWSEINFOW bi = { 0 };
                    LPITEMIDLIST pidl;
                    WCHAR Title[MAX_PATH];
                    WCHAR CustomSearchPath[MAX_PATH] = { 0 };
                    INT len, idx = (INT)SendDlgItemMessageW(hwndDlg, IDC_COMBO_PATH, CB_GETCURSEL, 0, 0);
                    LoadStringW(hDllInstance, IDS_BROWSE_FOR_FOLDER_TITLE, Title, _countof(Title));

                    if (idx == CB_ERR)
                        len = GetWindowTextLengthW(GetDlgItem(hwndDlg, IDC_COMBO_PATH));
                    else
                        len = (INT)SendDlgItemMessageW(hwndDlg, IDC_COMBO_PATH, CB_GETLBTEXTLEN, idx, 0);

                    if (len < _countof(CustomSearchPath))
                    {
                        if (idx == CB_ERR)
                            GetWindowTextW(GetDlgItem(hwndDlg, IDC_COMBO_PATH), CustomSearchPath, _countof(CustomSearchPath));
                        else
                            SendDlgItemMessageW(hwndDlg, IDC_COMBO_PATH, CB_GETLBTEXT, idx, (LPARAM)CustomSearchPath);
                    }
                    DevInstData->CustomSearchPath = CustomSearchPath;

                    bi.hwndOwner = hwndDlg;
                    bi.ulFlags = BIF_USENEWUI | BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT | BIF_NONEWFOLDERBUTTON;
                    bi.lpszTitle = Title;
                    bi.lpfn = BrowseCallbackProc;
                    bi.lParam = (LPARAM)DevInstData;
                    pidl = SHBrowseForFolderW(&bi);
                    if (pidl)
                    {
                        WCHAR Directory[MAX_PATH];
                        IMalloc* malloc;

                        if (SHGetPathFromIDListW(pidl, Directory))
                        {
                            /* Set the IDC_COMBO_PATH text */
                            SetWindowTextW(GetDlgItem(hwndDlg, IDC_COMBO_PATH), Directory);
                        }

                        /* Free memory, if possible */
                        if (SUCCEEDED(SHGetMalloc(&malloc)))
                        {
                            IMalloc_Free(malloc, pidl);
                            IMalloc_Release(malloc);
                        }
                        return TRUE;
                    }
                    break;
                }
            }
            break;
        }

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
                    if (IsDlgButtonChecked(hwndDlg, IDC_RADIO_SEARCHHERE))
                    {
                        SaveCustomPath(GetDlgItem(hwndDlg, IDC_COMBO_PATH));
                        HeapFree(GetProcessHeap(), 0, DevInstData->CustomSearchPath);
                        DevInstData->CustomSearchPath = NULL;
                        if (PrepareFoldersToScan(
                            DevInstData,
                            IsDlgButtonChecked(hwndDlg, IDC_CHECK_MEDIA),
                            IsDlgButtonChecked(hwndDlg, IDC_CHECK_PATH),
                            GetDlgItem(hwndDlg, IDC_COMBO_PATH)))
                        {
                            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_SEARCHDRV);
                        }
                        else
                        {
                            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_INSTALLFAILED);
                        }
                    }
                    else
                    {
                        /* FIXME */;
                    }
                    return TRUE;

                default:
                    break;
            }
            break;
        }

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
    DevInstData = (PDEVINSTDATA)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HWND hwndControl;
            DWORD dwStyle;

            /* Get pointer to the global setup data */
            DevInstData = (PDEVINSTDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (DWORD_PTR)DevInstData);

            DevInstData->hDialog = hwndDlg;
            hwndControl = GetParent(hwndDlg);

            /* Center the wizard window */
            CenterWindow(hwndControl);

            SendDlgItemMessage(
                hwndDlg,
                IDC_DEVICE,
                WM_SETTEXT,
                0,
                (LPARAM)DevInstData->buffer);

            /* Hide the system menu */
            dwStyle = GetWindowLongPtr(hwndControl, GWL_STYLE);
            SetWindowLongPtr(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);
            break;
        }

        case WM_SEARCH_FINISHED:
        {
            CloseHandle(hThread);
            hThread = 0;
            if (wParam == 0)
                PropSheet_SetCurSelByID(GetParent(hwndDlg), IDD_NODRIVER);
            else
                PropSheet_SetCurSelByID(GetParent(hwndDlg), IDD_INSTALLDRV);
            break;
        }

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hwndDlg), !PSWIZB_NEXT | !PSWIZB_BACK);
                    /* Yes, we can safely ignore the problem (if any) */
                    SetupDiDestroyDriverInfoList(
                        DevInstData->hDevInfo,
                        &DevInstData->devInfoData,
                        SPDIT_COMPATDRIVER);
                    hThread = CreateThread(NULL, 0, FindDriverProc, DevInstData, 0, &dwThreadId);
                    break;

                case PSN_KILLACTIVE:
                    if (hThread != 0)
                    {
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                        return TRUE;
                    }
                    break;

                case PSN_WIZNEXT:
                    /* Handle a Next button click, if necessary */
                    break;

                default:
                    break;
            }
            break;
        }

        default:
            break;
    }

    return FALSE;
}

static INT_PTR CALLBACK
InstallDrvDlgProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PDEVINSTDATA DevInstData;
    DWORD dwThreadId;

    /* Retrieve pointer to the global setup data */
    DevInstData = (PDEVINSTDATA)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HWND hwndControl;
            DWORD dwStyle;

            /* Get pointer to the global setup data */
            DevInstData = (PDEVINSTDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (DWORD_PTR)DevInstData);

            DevInstData->hDialog = hwndDlg;
            hwndControl = GetParent(hwndDlg);

            /* Center the wizard window */
            CenterWindow(hwndControl);

            SendDlgItemMessage(
                hwndDlg,
                IDC_DEVICE,
                WM_SETTEXT,
                0,
                (LPARAM)DevInstData->drvInfoData.Description);

            /* Hide the system menu */
            dwStyle = GetWindowLongPtr(hwndControl, GWL_STYLE);
            SetWindowLongPtr(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);
            break;
        }

        case WM_INSTALL_FINISHED:
        {
            CloseHandle(hThread);
            hThread = 0;
            if (wParam == 0)
            {
                SP_DEVINSTALL_PARAMS installParams;

                SetFailedInstall(DevInstData->hDevInfo,
                                 &DevInstData->devInfoData,
                                 FALSE);

                /* Should we reboot? */
                installParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
                if (SetupDiGetDeviceInstallParams(
                    DevInstData->hDevInfo,
                    &DevInstData->devInfoData,
                    &installParams))
                {
                    if (installParams.Flags & (DI_NEEDRESTART | DI_NEEDREBOOT))
                    {
                        PropSheet_SetCurSelByID(GetParent(hwndDlg), IDD_NEEDREBOOT);
                    }
                    else
                        PropSheet_SetCurSelByID(GetParent(hwndDlg), IDD_FINISHPAGE);
                    break;
                }
            }
            PropSheet_SetCurSelByID(GetParent(hwndDlg), IDD_INSTALLFAILED);
            break;
        }

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hwndDlg), !PSWIZB_NEXT | !PSWIZB_BACK);
                    hThread = CreateThread(NULL, 0, InstallDriverProc, DevInstData, 0, &dwThreadId);
                    break;

                case PSN_KILLACTIVE:
                    if (hThread != 0)
                    {
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                        return TRUE;
                    }
                    break;

                case PSN_WIZNEXT:
                    /* Handle a Next button click, if necessary */
                    break;

                default:
                    break;
            }
            break;
        }

        default:
            break;
    }

    return FALSE;
}

static INT_PTR CALLBACK
NoDriverDlgProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PDEVINSTDATA DevInstData;
    HWND hwndControl;

    UNREFERENCED_PARAMETER(wParam);

    /* Get pointer to the global setup data */
    DevInstData = (PDEVINSTDATA)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            BOOL DisableableDevice = FALSE;

            /* Get pointer to the global setup data */
            DevInstData = (PDEVINSTDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (DWORD_PTR)DevInstData);

            /* Center the wizard window */
            CenterWindow(GetParent(hwndDlg));

            hwndControl = GetDlgItem(GetParent(hwndDlg), IDCANCEL);
            ShowWindow(hwndControl, SW_HIDE);
            EnableWindow(hwndControl, FALSE);

            /* Set title font */
            SendDlgItemMessage(
                hwndDlg,
                IDC_FINISHTITLE,
                WM_SETFONT,
                (WPARAM)DevInstData->hTitleFont,
                (LPARAM)TRUE);

            /* disable the "do not show this dialog anymore" checkbox
             if the device cannot be disabled */
            CanDisableDevice(
                DevInstData->devInfoData.DevInst,
                NULL,
                &DisableableDevice);
            EnableWindow(
                GetDlgItem(hwndDlg, IDC_DONOTSHOWDLG),
                DisableableDevice);
            break;
        }

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
                    /* Handle a Back button click, if necessary */
                    hwndControl = GetDlgItem(GetParent(hwndDlg), IDCANCEL);
                    ShowWindow(hwndControl, SW_SHOW);
                    EnableWindow(hwndControl, TRUE);
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_CHSOURCE);
                    return TRUE;

                case PSN_WIZFINISH:
                {
                    BOOL DisableableDevice = FALSE;
                    BOOL IsStarted = FALSE;

                    if (CanDisableDevice(DevInstData->devInfoData.DevInst,
                            NULL,
                            &DisableableDevice) &&
                        DisableableDevice &&
                        IsDeviceStarted(
                            DevInstData->devInfoData.DevInst,
                            NULL,
                            &IsStarted) &&
                        !IsStarted &&
                        SendDlgItemMessage(
                            hwndDlg,
                            IDC_DONOTSHOWDLG,
                            BM_GETCHECK,
                            (WPARAM)0, (LPARAM)0) == BST_CHECKED)
                    {
                        /* disable the device */
                        StartDevice(
                            DevInstData->hDevInfo,
                            &DevInstData->devInfoData,
                            FALSE,
                            0,
                            NULL);
                    }
                    else
                    {
                        SetFailedInstall(DevInstData->hDevInfo,
                                         &DevInstData->devInfoData,
                                         FALSE);
                    }
                    break;
                }

                default:
                    break;
            }
            break;
        }

        default:
            break;
    }

    return FALSE;
}

static INT_PTR CALLBACK
InstallFailedDlgProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PDEVINSTDATA DevInstData;
    UNREFERENCED_PARAMETER(wParam);

    /* Retrieve pointer to the global setup data */
    DevInstData = (PDEVINSTDATA)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HWND hwndControl;

            /* Get pointer to the global setup data */
            DevInstData = (PDEVINSTDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (DWORD_PTR)DevInstData);

            /* Center the wizard window */
            CenterWindow(GetParent(hwndDlg));

            hwndControl = GetDlgItem(GetParent(hwndDlg), IDCANCEL);
            ShowWindow(hwndControl, SW_HIDE);
            EnableWindow(hwndControl, FALSE);

            SendDlgItemMessage(
                hwndDlg,
                IDC_DEVICE,
                WM_SETTEXT,
                0,
                (LPARAM)DevInstData->drvInfoData.Description);

            /* Set title font */
            SendDlgItemMessage(
                hwndDlg,
                IDC_FINISHTITLE,
                WM_SETFONT,
                (WPARAM)DevInstData->hTitleFont,
                (LPARAM)TRUE);
            break;
        }

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
            break;
        }

        default:
            break;
    }

    return FALSE;
}

static INT_PTR CALLBACK
NeedRebootDlgProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PDEVINSTDATA DevInstData;
    UNREFERENCED_PARAMETER(wParam);

    /* Retrieve pointer to the global setup data */
    DevInstData = (PDEVINSTDATA)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HWND hwndControl;

            /* Get pointer to the global setup data */
            DevInstData = (PDEVINSTDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (DWORD_PTR)DevInstData);

            /* Center the wizard window */
            CenterWindow(GetParent(hwndDlg));

            hwndControl = GetDlgItem(GetParent(hwndDlg), IDCANCEL);
            ShowWindow(hwndControl, SW_HIDE);
            EnableWindow(hwndControl, FALSE);

            SendDlgItemMessage(
                hwndDlg,
                IDC_DEVICE,
                WM_SETTEXT,
                0,
                (LPARAM)DevInstData->drvInfoData.Description);

            /* Set title font */
            SendDlgItemMessage(
                hwndDlg,
                IDC_FINISHTITLE,
                WM_SETFONT,
                (WPARAM)DevInstData->hTitleFont,
                (LPARAM)TRUE);
            break;
        }

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
            break;
        }

        default:
            break;
    }

    return FALSE;
}

static INT_PTR CALLBACK
FinishDlgProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PDEVINSTDATA DevInstData;
    UNREFERENCED_PARAMETER(wParam);

    /* Retrieve pointer to the global setup data */
    DevInstData = (PDEVINSTDATA)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HWND hwndControl;

            /* Get pointer to the global setup data */
            DevInstData = (PDEVINSTDATA)((LPPROPSHEETPAGE)lParam)->lParam;
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (DWORD_PTR)DevInstData);

            /* Center the wizard window */
            CenterWindow(GetParent(hwndDlg));

            hwndControl = GetDlgItem(GetParent(hwndDlg), IDCANCEL);
            ShowWindow(hwndControl, SW_HIDE);
            EnableWindow(hwndControl, FALSE);

            SendDlgItemMessage(
                hwndDlg,
                IDC_DEVICE,
                WM_SETTEXT,
                0,
                (LPARAM)DevInstData->drvInfoData.Description);

            /* Set title font */
            SendDlgItemMessage(
                hwndDlg,
                IDC_FINISHTITLE,
                WM_SETFONT,
                (WPARAM)DevInstData->hTitleFont,
                (LPARAM)TRUE);
            break;
        }

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
            break;
        }

        default:
            break;
    }

    return FALSE;
}

static HFONT
CreateTitleFont(VOID)
{
    NONCLIENTMETRICSW ncm;
    LOGFONTW LogFont;
    HDC hdc;
    INT FontSize;
    HFONT hFont;

    ncm.cbSize = sizeof(NONCLIENTMETRICSW);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

    LogFont = ncm.lfMessageFont;
    LogFont.lfWeight = FW_BOLD;
    wcscpy(LogFont.lfFaceName, L"MS Shell Dlg");

    hdc = GetDC(NULL);
    FontSize = 12;
    LogFont.lfHeight = 0 - GetDeviceCaps (hdc, LOGPIXELSY) * FontSize / 72;
    hFont = CreateFontIndirectW(&LogFont);
    ReleaseDC(NULL, hdc);

    return hFont;
}

BOOL
DisplayWizard(
    IN PDEVINSTDATA DevInstData,
    IN HWND hwndParent,
    IN UINT startPage)
{
    PROPSHEETHEADER psh = {0};
    HPROPSHEETPAGE ahpsp[IDD_MAXIMUMPAGE + 1];
    PROPSHEETPAGE psp = {0};
    HRESULT hr = CoInitialize(NULL); /* for SHAutoComplete */

    /* zero based index */
    startPage -= IDD_FIRSTPAGE;

    /* Create the Welcome page */
    ZeroMemory(&psp, sizeof(PROPSHEETPAGE));
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER | PSP_USETITLE;
    psp.hInstance = hDllInstance;
    psp.lParam = (LPARAM)DevInstData;
    psp.pszTitle = MAKEINTRESOURCE(DevInstData->bUpdate ? IDS_UPDATEWIZARDTITLE : IDS_INSTALLWIZARDTITLE);
    psp.pfnDlgProc = WelcomeDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_WELCOMEPAGE);
    ahpsp[IDD_WELCOMEPAGE-IDD_FIRSTPAGE] = CreatePropertySheetPage(&psp);

    /* Create the Select Source page */
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USETITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_CHSOURCE_TITLE);
    psp.pfnDlgProc = CHSourceDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_CHSOURCE);
    ahpsp[IDD_CHSOURCE-IDD_FIRSTPAGE] = CreatePropertySheetPage(&psp);

    /* Create the Search driver page */
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USETITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_SEARCHDRV_TITLE);
    psp.pfnDlgProc = SearchDrvDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_SEARCHDRV);
    ahpsp[IDD_SEARCHDRV-IDD_FIRSTPAGE] = CreatePropertySheetPage(&psp);

    /* Create the Install driver page */
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USETITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_INSTALLDRV_TITLE);
    psp.pfnDlgProc = InstallDrvDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_INSTALLDRV);
    ahpsp[IDD_INSTALLDRV-IDD_FIRSTPAGE] = CreatePropertySheetPage(&psp);

    /* Create the No driver page */
    psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER | PSP_USETITLE;
    psp.pfnDlgProc = NoDriverDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_NODRIVER);
    ahpsp[IDD_NODRIVER-IDD_FIRSTPAGE] = CreatePropertySheetPage(&psp);

    /* Create the Install failed page */
    psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER | PSP_USETITLE;
    psp.pfnDlgProc = InstallFailedDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_INSTALLFAILED);
    ahpsp[IDD_INSTALLFAILED-IDD_FIRSTPAGE] = CreatePropertySheetPage(&psp);

    /* Create the Need reboot page */
    psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER | PSP_USETITLE;
    psp.pfnDlgProc = NeedRebootDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_NEEDREBOOT);
    ahpsp[IDD_NEEDREBOOT-IDD_FIRSTPAGE] = CreatePropertySheetPage(&psp);

    /* Create the Finish page */
    psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER | PSP_USETITLE;
    psp.pfnDlgProc = FinishDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_FINISHPAGE);
    ahpsp[IDD_FINISHPAGE-IDD_FIRSTPAGE] = CreatePropertySheetPage(&psp);

    /* Create the property sheet */
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
    psh.hInstance = hDllInstance;
    psh.hwndParent = hwndParent;
    psh.nPages = IDD_MAXIMUMPAGE + 1;
    psh.nStartPage = startPage;
    psh.phpage = ahpsp;
    psh.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK);
    psh.pszbmHeader = MAKEINTRESOURCE(IDB_HEADER);

    /* Create title font */
    DevInstData->hTitleFont = CreateTitleFont();

    /* Display the wizard */
    PropertySheet(&psh);

    DeleteObject(DevInstData->hTitleFont);

    if (SUCCEEDED(hr))
        CoUninitialize();
    return TRUE;
}
