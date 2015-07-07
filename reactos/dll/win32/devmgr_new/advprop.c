/*
 * ReactOS Device Manager Applet
 * Copyright (C) 2004 - 2005 ReactOS Team
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
/*
 *
 * PROJECT:         ReactOS devmgr.dll
 * FILE:            lib/devmgr/advprop.c
 * PURPOSE:         ReactOS Device Manager
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 *                  Ged Murphy <gedmurphy@reactos.org>
 * UPDATE HISTORY:
 *      04-04-2004  Created
 */

#include "precomp.h"

#include <winver.h>


static UINT WINAPI
EnumDeviceDriverFilesCallback(IN PVOID Context,
                              IN UINT Notification,
                              IN UINT_PTR Param1,
                              IN UINT_PTR Param2)
{
    LVITEM li;
    PENUMDRIVERFILES_CONTEXT EnumDriverFilesContext = (PENUMDRIVERFILES_CONTEXT)Context;

    li.mask = LVIF_TEXT | LVIF_STATE;
    li.iItem = EnumDriverFilesContext->nCount++;
    li.iSubItem = 0;
    li.state = (li.iItem == 0 ? LVIS_SELECTED : 0);
    li.stateMask = LVIS_SELECTED;
    li.pszText = (LPWSTR)Param1;
    (void)ListView_InsertItem(EnumDriverFilesContext->hDriversListView,
                              &li);
    return NO_ERROR;
}


static VOID
UpdateDriverDetailsDlg(IN HWND hwndDlg,
                       IN HWND hDriversListView,
                       IN PDEVADVPROP_INFO dap)
{
    HDEVINFO DeviceInfoSet;
    PSP_DEVINFO_DATA DeviceInfoData;
    SP_DRVINFO_DATA DriverInfoData;
    ENUMDRIVERFILES_CONTEXT EnumDriverFilesContext;

    if (dap->CurrentDeviceInfoSet != INVALID_HANDLE_VALUE)
    {
        DeviceInfoSet = dap->CurrentDeviceInfoSet;
        DeviceInfoData = &dap->CurrentDeviceInfoData;
    }
    else
    {
        DeviceInfoSet = dap->DeviceInfoSet;
        DeviceInfoData = &dap->DeviceInfoData;
    }

    /* set the device image */
    SendDlgItemMessage(hwndDlg,
                       IDC_DEVICON,
                       STM_SETICON,
                       (WPARAM)dap->hDevIcon,
                       0);

    /* set the device name edit control text */
    SetDlgItemText(hwndDlg,
                   IDC_DEVNAME,
                   dap->szDevName);

    /* fill the driver files list view */
    EnumDriverFilesContext.hDriversListView = hDriversListView;
    EnumDriverFilesContext.nCount = 0;

    (void)ListView_DeleteAllItems(EnumDriverFilesContext.hDriversListView);
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if (FindCurrentDriver(DeviceInfoSet,
                          DeviceInfoData,
                          &DriverInfoData) &&
        SetupDiSetSelectedDriver(DeviceInfoSet,
                                 DeviceInfoData,
                                 &DriverInfoData))
    {
        HSPFILEQ queueHandle;

        queueHandle = SetupOpenFileQueue();
        if (queueHandle != (HSPFILEQ)INVALID_HANDLE_VALUE)
        {
            SP_DEVINSTALL_PARAMS DeviceInstallParams = {0};
            DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
            if (SetupDiGetDeviceInstallParams(DeviceInfoSet,
                                              DeviceInfoData,
                                              &DeviceInstallParams))
            {
                DeviceInstallParams.FileQueue = queueHandle;
                DeviceInstallParams.Flags |= DI_NOVCP;

                if (SetupDiSetDeviceInstallParams(DeviceInfoSet,
                                                  DeviceInfoData,
                                                  &DeviceInstallParams) &&
                    SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES,
                                              DeviceInfoSet,
                                              DeviceInfoData))
                {
                    DWORD scanResult;
                    RECT rcClient;
                    LVCOLUMN lvc;

                    /* enumerate the driver files */
                    SetupScanFileQueue(queueHandle,
                                       SPQ_SCAN_USE_CALLBACK,
                                       NULL,
                                       EnumDeviceDriverFilesCallback,
                                       &EnumDriverFilesContext,
                                       &scanResult);

                    /* update the list view column width */
                    GetClientRect(hDriversListView,
                                  &rcClient);
                    lvc.mask = LVCF_WIDTH;
                    lvc.cx = rcClient.right;
                    (void)ListView_SetColumn(hDriversListView,
                                             0,
                                             &lvc);

                    /* highlight the first item from list */
                    if (ListView_GetSelectedCount(hDriversListView) != 0)
                    {
                        ListView_SetItemState(hDriversListView,
                                              0,
                                              LVIS_FOCUSED | LVIS_SELECTED,
                                              LVIS_FOCUSED | LVIS_SELECTED);
                    }
                }
            }

            SetupCloseFileQueue(queueHandle);
        }
    }
}


static VOID
UpdateDriverVersionInfoDetails(IN HWND hwndDlg,
                               IN LPCWSTR lpszDriverPath)
{
    DWORD dwHandle;
    DWORD dwVerInfoSize;
    LPVOID lpData = NULL;
    LPVOID lpInfo;
    UINT uInfoLen;
    DWORD dwLangId;
    WCHAR szLangInfo[255];
    WCHAR szLangPath[MAX_PATH];
    LPWSTR lpCompanyName = NULL;
    LPWSTR lpFileVersion = NULL;
    LPWSTR lpLegalCopyright = NULL;
    LPWSTR lpDigitalSigner = NULL;
    UINT uBufLen;
    WCHAR szNotAvailable[255];

    /* extract version info from selected file */
    dwVerInfoSize = GetFileVersionInfoSize(lpszDriverPath,
                                           &dwHandle);
    if (!dwVerInfoSize)
        goto done;

    lpData = HeapAlloc(GetProcessHeap(),
                       HEAP_ZERO_MEMORY,
                       dwVerInfoSize);
    if (!lpData)
        goto done;

    if (!GetFileVersionInfo(lpszDriverPath,
                            dwHandle,
                            dwVerInfoSize,
                            lpData))
        goto done;

    if (!VerQueryValue(lpData,
                       L"\\VarFileInfo\\Translation",
                       &lpInfo,
                       &uInfoLen))
        goto done;

    dwLangId = *(LPDWORD)lpInfo;
    swprintf(szLangInfo, L"\\StringFileInfo\\%04x%04x\\",
             LOWORD(dwLangId), HIWORD(dwLangId));

    /* read CompanyName */
    wcscpy(szLangPath, szLangInfo);
    wcscat(szLangPath, L"CompanyName");

    VerQueryValue(lpData,
                  szLangPath,
                  (void **)&lpCompanyName,
                  (PUINT)&uBufLen);

    /* read FileVersion */
    wcscpy(szLangPath, szLangInfo);
    wcscat(szLangPath, L"FileVersion");

    VerQueryValue(lpData,
                  szLangPath,
                  (void **)&lpFileVersion,
                  (PUINT)&uBufLen);

    /* read LegalTrademarks */
    wcscpy(szLangPath, szLangInfo);
    wcscat(szLangPath, L"LegalCopyright");

    VerQueryValue(lpData,
                  szLangPath,
                  (void **)&lpLegalCopyright,
                  (PUINT)&uBufLen);

    /* TODO: read digital signer info */

done:
    if (!LoadString(hDllInstance,
                    IDS_NOTAVAILABLE,
                    szNotAvailable,
                    sizeof(szNotAvailable) / sizeof(WCHAR)))
    {
        wcscpy(szNotAvailable, L"n/a");
    }

    /* update labels */
    if (!lpCompanyName)
        lpCompanyName = szNotAvailable;
    SetDlgItemText(hwndDlg,
                   IDC_FILEPROVIDER,
                   lpCompanyName);

    if (!lpFileVersion)
        lpFileVersion = szNotAvailable;
    SetDlgItemText(hwndDlg,
                   IDC_FILEVERSION,
                   lpFileVersion);

    if (!lpLegalCopyright)
        lpLegalCopyright = szNotAvailable;
    SetDlgItemText(hwndDlg,
                   IDC_FILECOPYRIGHT,
                   lpLegalCopyright);

    if (!lpDigitalSigner)
        lpDigitalSigner = szNotAvailable;
    SetDlgItemText(hwndDlg,
                   IDC_DIGITALSIGNER,
                   lpDigitalSigner);

    /* release version info */
    if (lpData)
        HeapFree(GetProcessHeap(),
                 0,
                 lpData);
}


static INT_PTR
CALLBACK
DriverDetailsDlgProc(IN HWND hwndDlg,
                     IN UINT uMsg,
                     IN WPARAM wParam,
                     IN LPARAM lParam)
{
    PDEVADVPROP_INFO dap;
    INT_PTR Ret = FALSE;

    dap = (PDEVADVPROP_INFO)GetWindowLongPtr(hwndDlg,
                                             DWL_USER);

    if (dap != NULL || uMsg == WM_INITDIALOG)
    {
        switch (uMsg)
        {
            case WM_COMMAND:
            {
                switch (LOWORD(wParam))
                {
                    case IDOK:
                    case IDCANCEL:
                    {
                        EndDialog(hwndDlg,
                                  IDOK);
                        break;
                    }
                }
                break;
            }

            case WM_CLOSE:
            {
                EndDialog(hwndDlg,
                          IDCANCEL);
                break;
            }

            case WM_INITDIALOG:
            {
                LV_COLUMN lvc;
                HWND hDriversListView;
                WCHAR szBuffer[260];

                dap = (PDEVADVPROP_INFO)lParam;
                if (dap != NULL)
                {
                    SetWindowLongPtr(hwndDlg,
                                     DWL_USER,
                                     (DWORD_PTR)dap);

                    hDriversListView = GetDlgItem(hwndDlg,
                                                  IDC_DRIVERFILES);

                    /* add a column to the list view control */
                    lvc.mask = LVCF_FMT | LVCF_WIDTH;
                    lvc.fmt = LVCFMT_LEFT;
                    lvc.cx = 0;
                    (void)ListView_InsertColumn(hDriversListView,
                                                0,
                                                &lvc);

                    UpdateDriverDetailsDlg(hwndDlg,
                                           hDriversListView,
                                           dap);

                    if (ListView_GetItemCount(hDriversListView) == 0)
                    {
                        if(LoadStringW(hDllInstance, IDS_NODRIVERS, szBuffer, _countof(szBuffer)))
                            MessageBoxW(hwndDlg, szBuffer, dap->szDevName, MB_OK);
                        EndDialog(hwndDlg, IDCANCEL);
                    }
                }

                Ret = TRUE;
                break;
            }

            case WM_NOTIFY:
            {
                LPNMHDR pnmhdr = (LPNMHDR)lParam;

                switch (pnmhdr->code)
                {
                case LVN_ITEMCHANGED:
                    {
                        LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
                        HWND hDriversListView = GetDlgItem(hwndDlg,
                                                           IDC_DRIVERFILES);

                        if (ListView_GetSelectedCount(hDriversListView) == 0)
                        {
                            /* nothing is selected - empty the labels */
                            SetDlgItemText(hwndDlg,
                                           IDC_FILEPROVIDER,
                                           NULL);
                            SetDlgItemText(hwndDlg,
                                           IDC_FILEVERSION,
                                           NULL);
                            SetDlgItemText(hwndDlg,
                                           IDC_FILECOPYRIGHT,
                                           NULL);
                            SetDlgItemText(hwndDlg,
                                           IDC_DIGITALSIGNER,
                                           NULL);
                        }
                        else if (pnmv->uNewState != 0)
                        {
                            /* extract version info and update the labels */
                            WCHAR szDriverPath[MAX_PATH];

                            ListView_GetItemText(hDriversListView,
                                                 pnmv->iItem,
                                                 pnmv->iSubItem,
                                                 szDriverPath,
                                                 MAX_PATH);

                            UpdateDriverVersionInfoDetails(hwndDlg,
                                                           szDriverPath);
                        }
                    }
                }
                break;
            }
        }
    }

    return Ret;
}


static
VOID
UpdateDriver(
    IN HWND hwndDlg,
    IN PDEVADVPROP_INFO dap)
{
    TOKEN_PRIVILEGES Privileges;
    HANDLE hToken;
    DWORD dwReboot;
    BOOL NeedReboot = FALSE;

    // Better use InstallDevInst:
    //     BOOL
    //     WINAPI
    //     InstallDevInst(
    //         HWND hWnd,
    //         LPWSTR wszDeviceId,
    //         BOOL bUpdate,
    //         DWORD *dwReboot);
    // See: http://comp.os.ms-windows.programmer.win32.narkive.com/J8FTd4KK/signature-of-undocumented-installdevinstex

    if (!InstallDevInst(hwndDlg, dap->szDeviceID, TRUE, &dwReboot))
        return;

    if (NeedReboot == FALSE)
        return;

    //FIXME: load text from resource file
    if (MessageBoxW(hwndDlg, L"Reboot now?", L"Reboot required", MB_YESNO | MB_ICONQUESTION) != IDYES)
        return;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
    {
        ERR("OpenProcessToken failed\n");
        return;
    }

    /* Get the LUID for the Shutdown privilege */
    if (!LookupPrivilegeValueW(NULL, SE_SHUTDOWN_NAME, &Privileges.Privileges[0].Luid))
    {
        ERR("LookupPrivilegeValue failed\n");
        CloseHandle(hToken);
        return;
    }

    /* Assign the Shutdown privilege to our process */
    Privileges.PrivilegeCount = 1;
    Privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hToken, FALSE, &Privileges, 0, NULL, NULL))
    {
        ERR("AdjustTokenPrivileges failed\n");
        CloseHandle(hToken);
        return;
    }

    /* Finally shut down the system */
    if (!ExitWindowsEx(EWX_REBOOT, SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED))
    {
        ERR("ExitWindowsEx failed\n");
        CloseHandle(hToken);
    }
}


static VOID
UpdateDriverDlg(IN HWND hwndDlg,
                IN PDEVADVPROP_INFO dap)
{
    HDEVINFO DeviceInfoSet;
    PSP_DEVINFO_DATA DeviceInfoData;

    if (dap->CurrentDeviceInfoSet != INVALID_HANDLE_VALUE)
    {
        DeviceInfoSet = dap->CurrentDeviceInfoSet;
        DeviceInfoData = &dap->CurrentDeviceInfoData;
    }
    else
    {
        DeviceInfoSet = dap->DeviceInfoSet;
        DeviceInfoData = &dap->DeviceInfoData;
    }

    /* set the device image */
    SendDlgItemMessage(hwndDlg,
                       IDC_DEVICON,
                       STM_SETICON,
                       (WPARAM)dap->hDevIcon,
                       0);

    /* set the device name edit control text */
    SetDlgItemText(hwndDlg,
                   IDC_DEVNAME,
                   dap->szDevName);

    /* query the driver provider */
    if (GetDriverProviderString(DeviceInfoSet,
                                DeviceInfoData,
                                dap->szTemp,
                                sizeof(dap->szTemp) / sizeof(dap->szTemp[0])))
    {
        SetDlgItemText(hwndDlg,
                       IDC_DRVPROVIDER,
                       dap->szTemp);
    }

    /* query the driver date */
    if (GetDriverDateString(DeviceInfoSet,
                            DeviceInfoData,
                            dap->szTemp,
                            sizeof(dap->szTemp) / sizeof(dap->szTemp[0])))
    {
        SetDlgItemText(hwndDlg,
                       IDC_DRVDATE,
                       dap->szTemp);
    }

    /* query the driver version */
    if (GetDriverVersionString(DeviceInfoSet,
                               DeviceInfoData,
                               dap->szTemp,
                               sizeof(dap->szTemp) / sizeof(dap->szTemp[0])))
    {
        SetDlgItemText(hwndDlg,
                       IDC_DRVVERSION,
                       dap->szTemp);
    }
}


static INT_PTR
CALLBACK
AdvProcDriverDlgProc(IN HWND hwndDlg,
                     IN UINT uMsg,
                     IN WPARAM wParam,
                     IN LPARAM lParam)
{
    PDEVADVPROP_INFO dap;
    INT_PTR Ret = FALSE;

    dap = (PDEVADVPROP_INFO)GetWindowLongPtr(hwndDlg,
                                             DWL_USER);

    if (dap != NULL || uMsg == WM_INITDIALOG)
    {
        switch (uMsg)
        {
            case WM_COMMAND:
            {
                switch (LOWORD(wParam))
                {
                    case IDC_DRIVERDETAILS:
                        DialogBoxParam(hDllInstance,
                                       MAKEINTRESOURCE(IDD_DRIVERDETAILS),
                                       hwndDlg,
                                       DriverDetailsDlgProc,
                                       (ULONG_PTR)dap);
                        break;

                    case IDC_UPDATEDRIVER:
                        UpdateDriver(hwndDlg, dap);
                        break;
                }
                break;
            }

            case WM_NOTIFY:
            {
                NMHDR *hdr = (NMHDR*)lParam;
                switch (hdr->code)
                {
                    case PSN_APPLY:
                        break;
                }
                break;
            }

            case WM_INITDIALOG:
            {
                dap = (PDEVADVPROP_INFO)((LPPROPSHEETPAGE)lParam)->lParam;
                if (dap != NULL)
                {
                    SetWindowLongPtr(hwndDlg,
                                     DWL_USER,
                                     (DWORD_PTR)dap);

                    UpdateDriverDlg(hwndDlg,
                                    dap);
                }
                Ret = TRUE;
                break;
            }
        }
    }

    return Ret;
}


static VOID
SetListViewText(HWND hwnd,
                INT iItem,
                LPWSTR lpText)
{
    LVITEM li;

    li.mask = LVIF_TEXT | LVIF_STATE;
    li.iItem = iItem;
    li.iSubItem = 0;
    li.state = 0; //(li.iItem == 0 ? LVIS_SELECTED : 0);
    li.stateMask = LVIS_SELECTED;
    li.pszText = lpText;
    (void)ListView_InsertItem(hwnd,
                              &li);
}


static VOID
UpdateDetailsDlg(IN HWND hwndDlg,
                 IN PDEVADVPROP_INFO dap)
{
    HWND hwndComboBox;
    HWND hwndListView;
    LV_COLUMN lvc;
    RECT rcClient;

    UINT i;
    UINT Properties[] =
    {
        IDS_PROP_DEVICEID,
        IDS_PROP_HARDWAREIDS,
        IDS_PROP_COMPATIBLEIDS,
        IDS_PROP_MATCHINGDEVICEID,
        IDS_PROP_SERVICE,
        IDS_PROP_ENUMERATOR,
        IDS_PROP_CAPABILITIES,
        IDS_PROP_DEVNODEFLAGS,
        IDS_PROP_CONFIGFLAGS,
        IDS_PROP_CSCONFIGFLAGS,
        IDS_PROP_EJECTIONRELATIONS,
        IDS_PROP_REMOVALRELATIONS,
        IDS_PROP_BUSRELATIONS,
        IDS_PROP_DEVUPPERFILTERS,
        IDS_PROP_DEVLOWERFILTERS,
        IDS_PROP_CLASSUPPERFILTERS,
        IDS_PROP_CLASSLOWERFILTERS,
        IDS_PROP_CLASSINSTALLER,
        IDS_PROP_CLASSCOINSTALLER,
        IDS_PROP_DEVICECOINSTALLER,
        IDS_PROP_FIRMWAREREVISION,
        IDS_PROP_CURRENTPOWERSTATE,
        IDS_PROP_POWERCAPABILITIES,
        IDS_PROP_POWERSTATEMAPPINGS
    };


    /* set the device image */
    SendDlgItemMessage(hwndDlg,
                       IDC_DEVICON,
                       STM_SETICON,
                       (WPARAM)dap->hDevIcon,
                       0);

    /* set the device name edit control text */
    SetDlgItemText(hwndDlg,
                   IDC_DEVNAME,
                   dap->szDevName);


    hwndComboBox = GetDlgItem(hwndDlg,
                              IDC_DETAILSPROPNAME);

    hwndListView = GetDlgItem(hwndDlg,
                              IDC_DETAILSPROPVALUE);

    for (i = 0; i != sizeof(Properties) / sizeof(Properties[0]); i++)
    {
        /* fill in the device usage combo box */
        if (LoadString(hDllInstance,
                       Properties[i],
                       dap->szTemp,
                       sizeof(dap->szTemp) / sizeof(dap->szTemp[0])))
        {
            SendMessage(hwndComboBox,
                        CB_ADDSTRING,
                        0,
                        (LPARAM)dap->szTemp);
        }
    }


    GetClientRect(hwndListView,
                  &rcClient);

    /* add a column to the list view control */
    lvc.mask = LVCF_FMT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.cx = rcClient.right;
    (void)ListView_InsertColumn(hwndListView,
                                0,
                                &lvc);

    SendMessage(hwndComboBox,
                CB_SETCURSEL,
                0,
                0);

    SetListViewText(hwndListView, 0, dap->szDeviceID);

    SetFocus(hwndComboBox);
}


static VOID
DisplayDevicePropertyText(IN PDEVADVPROP_INFO dap,
                          IN HWND hwndListView,
                          IN DWORD dwProperty)
{
    HDEVINFO DeviceInfoSet;
    PSP_DEVINFO_DATA DeviceInfoData;
    DWORD dwType;
    DWORD dwSize;
    DWORD dwValue;
    LPBYTE lpBuffer;
    LPWSTR lpStr;
    INT len;
    INT index;

    if (dap->CurrentDeviceInfoSet != INVALID_HANDLE_VALUE)
    {
        DeviceInfoSet = dap->CurrentDeviceInfoSet;
        DeviceInfoData = &dap->CurrentDeviceInfoData;
    }
    else
    {
        DeviceInfoSet = dap->DeviceInfoSet;
        DeviceInfoData = &dap->DeviceInfoData;
    }

    dwSize = 0;
    SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                     DeviceInfoData,
                                     dwProperty,
                                     &dwType,
                                     NULL,
                                     0,
                                     &dwSize);
    if (dwSize == 0)
    {
        if (GetLastError() != ERROR_FILE_NOT_FOUND)
        {
            swprintf(dap->szTemp, L"Error: Getting the size failed! (Error: %ld)", GetLastError());
            SetListViewText(hwndListView, 0, dap->szTemp);
        }
        return;
    }

    if (dwType == REG_SZ)
        dwSize += sizeof(WCHAR);

    lpBuffer = HeapAlloc(GetProcessHeap(),
                         HEAP_ZERO_MEMORY,
                         dwSize);
    if (lpBuffer == NULL)
    {
        SetListViewText(hwndListView, 0, L"Error: Allocating the buffer failed!");
        return;
    }

    if (SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                         DeviceInfoData,
                                         dwProperty,
                                         &dwType,
                                         lpBuffer,
                                         dwSize,
                                         &dwSize))
    {
        if (dwType == REG_SZ)
        {
            SetListViewText(hwndListView, 0, (LPWSTR)lpBuffer);
        }
        else if (dwType == REG_MULTI_SZ)
        {
            lpStr = (LPWSTR)lpBuffer;
            index = 0;
            while (*lpStr != 0)
            {
                len = wcslen(lpStr) + 1;

                SetListViewText(hwndListView, index, lpStr);

                lpStr += len;
                index++;
            }
        }
        else if (dwType == REG_DWORD)
        {
            dwValue = *(DWORD *) lpBuffer;

            switch (dwProperty)
            {
                case SPDRP_CAPABILITIES:
                    index = 0;
                    if (dwValue & CM_DEVCAP_LOCKSUPPORTED)
                        SetListViewText(hwndListView, index++, L"CM_DEVCAP_LOCKSUPPORTED");
                    if (dwValue & CM_DEVCAP_EJECTSUPPORTED)
                        SetListViewText(hwndListView, index++, L"CM_DEVCAP_EJECTSUPPORTED");
                    if (dwValue & CM_DEVCAP_REMOVABLE)
                        SetListViewText(hwndListView, index++, L"CM_DEVCAP_REMOVABLE");
                    if (dwValue & CM_DEVCAP_DOCKDEVICE)
                        SetListViewText(hwndListView, index++, L"CM_DEVCAP_DOCKDEVICE");
                    if (dwValue & CM_DEVCAP_UNIQUEID)
                        SetListViewText(hwndListView, index++, L"CM_DEVCAP_UNIQUEID");
                    if (dwValue & CM_DEVCAP_SILENTINSTALL)
                        SetListViewText(hwndListView, index++, L"CM_DEVCAP_SILENTINSTALL");
                    if (dwValue & CM_DEVCAP_RAWDEVICEOK)
                        SetListViewText(hwndListView, index++, L"CM_DEVCAP_RAWDEVICEOK");
                    if (dwValue & CM_DEVCAP_SURPRISEREMOVALOK)
                        SetListViewText(hwndListView, index++, L"CM_DEVCAP_SURPRISEREMOVALOK");
                    if (dwValue & CM_DEVCAP_HARDWAREDISABLED)
                        SetListViewText(hwndListView, index++, L"CM_DEVCAP_HARDWAREDISABLED");
                    if (dwValue & CM_DEVCAP_NONDYNAMIC)
                        SetListViewText(hwndListView, index++, L"CM_DEVCAP_NONDYNAMIC");
                    break;

                case SPDRP_CONFIGFLAGS:
                    index = 0;
                    if (dwValue & CONFIGFLAG_DISABLED)
                        SetListViewText(hwndListView, index++, L"CONFIGFLAG_DISABLED");
                    if (dwValue & CONFIGFLAG_REMOVED)
                        SetListViewText(hwndListView, index++, L"CONFIGFLAG_REMOVED");
                    if (dwValue & CONFIGFLAG_MANUAL_INSTALL)
                        SetListViewText(hwndListView, index++, L"CONFIGFLAG_MANUAL_INSTALL");
                    if (dwValue & CONFIGFLAG_IGNORE_BOOT_LC)
                        SetListViewText(hwndListView, index++, L"CONFIGFLAG_IGNORE_BOOT_LC");
                    if (dwValue & CONFIGFLAG_NET_BOOT)
                        SetListViewText(hwndListView, index++, L"CONFIGFLAG_NET_BOOT");
                    if (dwValue & CONFIGFLAG_REINSTALL)
                        SetListViewText(hwndListView, index++, L"CONFIGFLAG_REINSTALL");
                    if (dwValue & CONFIGFLAG_FAILEDINSTALL)
                        SetListViewText(hwndListView, index++, L"CONFIGFLAG_FAILEDINSTALL");
                    if (dwValue & CONFIGFLAG_CANTSTOPACHILD)
                        SetListViewText(hwndListView, index++, L"CONFIGFLAG_CANTSTOPACHILD");
                    if (dwValue & CONFIGFLAG_OKREMOVEROM)
                        SetListViewText(hwndListView, index++, L"CONFIGFLAG_OKREMOVEROM");
                    if (dwValue & CONFIGFLAG_NOREMOVEEXIT)
                        SetListViewText(hwndListView, index++, L"CONFIGFLAG_NOREMOVEEXIT");
                    break;

                default:
                    swprintf(dap->szTemp, L"0x%08lx", dwValue);
                    SetListViewText(hwndListView, 0, dap->szTemp);
                    break;
            }
        }
        else
        {
            SetListViewText(hwndListView, 0, L"Error: Unsupported value type!");

        }
    }
    else
    {
        SetListViewText(hwndListView, 0, L"Error: Retrieving the value failed!");
    }

    HeapFree(GetProcessHeap(),
             0,
             lpBuffer);
}

static VOID
DisplayDevNodeFlags(IN PDEVADVPROP_INFO dap,
                    IN HWND hwndListView)
{
    DWORD dwStatus = 0;
    DWORD dwProblem = 0;
    INT index;

    CM_Get_DevNode_Status_Ex(&dwStatus,
                             &dwProblem,
                             dap->DeviceInfoData.DevInst,
                             0,
                             dap->hMachine);

    index = 0;
    if (dwStatus & DN_ROOT_ENUMERATED)
        SetListViewText(hwndListView, index++, L"DN_ROOT_ENUMERATED");
    if (dwStatus & DN_DRIVER_LOADED)
        SetListViewText(hwndListView, index++, L"DN_DRIVER_LOADED");
    if (dwStatus & DN_ENUM_LOADED)
        SetListViewText(hwndListView, index++, L"DN_ENUM_LOADED");
    if (dwStatus & DN_STARTED)
        SetListViewText(hwndListView, index++, L"DN_STARTED");
    if (dwStatus & DN_MANUAL)
        SetListViewText(hwndListView, index++, L"DN_MANUAL");
    if (dwStatus & DN_NEED_TO_ENUM)
        SetListViewText(hwndListView, index++, L"DN_NEED_TO_ENUM");
    if (dwStatus & DN_DRIVER_BLOCKED)
        SetListViewText(hwndListView, index++, L"DN_DRIVER_BLOCKED");
    if (dwStatus & DN_HARDWARE_ENUM)
        SetListViewText(hwndListView, index++, L"DN_HARDWARE_ENUM");
    if (dwStatus & DN_NEED_RESTART)
        SetListViewText(hwndListView, index++, L"DN_NEED_RESTART");
    if (dwStatus & DN_CHILD_WITH_INVALID_ID)
        SetListViewText(hwndListView, index++, L"DN_CHILD_WITH_INVALID_ID");
    if (dwStatus & DN_HAS_PROBLEM)
        SetListViewText(hwndListView, index++, L"DN_HAS_PROBLEM");
    if (dwStatus & DN_FILTERED)
        SetListViewText(hwndListView, index++, L"DN_FILTERED");
    if (dwStatus & DN_LEGACY_DRIVER)
        SetListViewText(hwndListView, index++, L"DN_LEGACY_DRIVER");
    if (dwStatus & DN_DISABLEABLE)
        SetListViewText(hwndListView, index++, L"DN_DISABLEABLE");
    if (dwStatus & DN_REMOVABLE)
        SetListViewText(hwndListView, index++, L"DN_REMOVABLE");
    if (dwStatus & DN_PRIVATE_PROBLEM)
        SetListViewText(hwndListView, index++, L"DN_PRIVATE_PROBLEM");
    if (dwStatus & DN_MF_PARENT)
        SetListViewText(hwndListView, index++, L"DN_MF_PARENT");
    if (dwStatus & DN_MF_CHILD)
        SetListViewText(hwndListView, index++, L"DN_MF_CHILD");
    if (dwStatus & DN_WILL_BE_REMOVED)
        SetListViewText(hwndListView, index++, L"DN_WILL_BE_REMOVED");

    if (dwStatus & DN_NOT_FIRST_TIMEE)
        SetListViewText(hwndListView, index++, L"DN_NOT_FIRST_TIMEE");
    if (dwStatus & DN_STOP_FREE_RES)
        SetListViewText(hwndListView, index++, L"DN_STOP_FREE_RES");
    if (dwStatus & DN_REBAL_CANDIDATE)
        SetListViewText(hwndListView, index++, L"DN_REBAL_CANDIDATE");
    if (dwStatus & DN_BAD_PARTIAL)
        SetListViewText(hwndListView, index++, L"DN_BAD_PARTIAL");
    if (dwStatus & DN_NT_ENUMERATOR)
        SetListViewText(hwndListView, index++, L"DN_NT_ENUMERATOR");
    if (dwStatus & DN_NT_DRIVER)
        SetListViewText(hwndListView, index++, L"DN_NT_DRIVER");

    if (dwStatus & DN_NEEDS_LOCKING)
        SetListViewText(hwndListView, index++, L"DN_NEEDS_LOCKING");
    if (dwStatus & DN_ARM_WAKEUP)
        SetListViewText(hwndListView, index++, L"DN_ARM_WAKEUP");
    if (dwStatus & DN_APM_ENUMERATOR)
        SetListViewText(hwndListView, index++, L"DN_APM_ENUMERATOR");
    if (dwStatus & DN_APM_DRIVER)
        SetListViewText(hwndListView, index++, L"DN_APM_DRIVER");
    if (dwStatus & DN_SILENT_INSTALL)
        SetListViewText(hwndListView, index++, L"DN_SILENT_INSTALL");
    if (dwStatus & DN_NO_SHOW_IN_DM)
        SetListViewText(hwndListView, index++, L"DN_NO_SHOW_IN_DM");
    if (dwStatus & DN_BOOT_LOG_PROB)
        SetListViewText(hwndListView, index++, L"DN_BOOT_LOG_PROB");

//    swprintf(dap->szTemp, L"0x%08x", dwStatus);
//    SetListViewText(hwndListView, 0, dap->szTemp);
}


static VOID
DisplayDevNodeEnumerator(IN PDEVADVPROP_INFO dap,
                         IN HWND hwndListView)
{
    PSP_DEVINFO_DATA DeviceInfoData;

    DWORD dwType = 0;
    WCHAR szBuffer[256];
    DWORD dwSize = 256 * sizeof(WCHAR);

    if (dap->CurrentDeviceInfoSet != INVALID_HANDLE_VALUE)
    {
        DeviceInfoData = &dap->CurrentDeviceInfoData;
    }
    else
    {
        DeviceInfoData = &dap->DeviceInfoData;
    }

    CM_Get_DevNode_Registry_Property_ExW(DeviceInfoData->DevInst,
                                         CM_DRP_ENUMERATOR_NAME,
                                         &dwType,
                                         &szBuffer,
                                         &dwSize,
                                         0,
                                         dap->hMachine);

    SetListViewText(hwndListView, 0, szBuffer);
}


static VOID
DisplayCsFlags(IN PDEVADVPROP_INFO dap,
               IN HWND hwndListView)
{
    DWORD dwValue = 0;
    INT index;

    CM_Get_HW_Prof_Flags_Ex(dap->szDevName,
                            0, /* current hardware profile */
                            &dwValue,
                            0,
                            dap->hMachine);

    index = 0;
    if (dwValue & CSCONFIGFLAG_DISABLED)
        SetListViewText(hwndListView, index++, L"CSCONFIGFLAG_DISABLED");

    if (dwValue & CSCONFIGFLAG_DO_NOT_CREATE)
        SetListViewText(hwndListView, index++, L"CSCONFIGFLAG_DO_NOT_CREATE");

    if (dwValue & CSCONFIGFLAG_DO_NOT_START)
        SetListViewText(hwndListView, index++, L"CSCONFIGFLAG_DO_NOT_START");
}


static VOID
DisplayMatchingDeviceId(IN PDEVADVPROP_INFO dap,
                        IN HWND hwndListView)
{
    HDEVINFO DeviceInfoSet;
    PSP_DEVINFO_DATA DeviceInfoData;
    WCHAR szBuffer[256];
    HKEY hKey;
    DWORD dwSize;
    DWORD dwType;

    if (dap->CurrentDeviceInfoSet != INVALID_HANDLE_VALUE)
    {
        DeviceInfoSet = dap->CurrentDeviceInfoSet;
        DeviceInfoData = &dap->CurrentDeviceInfoData;
    }
    else
    {
        DeviceInfoSet = dap->DeviceInfoSet;
        DeviceInfoData = &dap->DeviceInfoData;
    }

    hKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                DeviceInfoData,
                                DICS_FLAG_GLOBAL,
                                0,
                                DIREG_DRV,
                                KEY_QUERY_VALUE);
    if (hKey != INVALID_HANDLE_VALUE)
    {
        dwSize = 256 * sizeof(WCHAR);
        if (RegQueryValueEx(hKey,
                            L"MatchingDeviceId",
                            NULL,
                            &dwType,
                            (LPBYTE)szBuffer,
                            &dwSize) == ERROR_SUCCESS)
        {
            SetListViewText(hwndListView, 0, szBuffer);
        }

        RegCloseKey(hKey);
    }
}


static VOID
DisplayClassCoinstallers(IN PDEVADVPROP_INFO dap,
                          IN HWND hwndListView)
{
    HDEVINFO DeviceInfoSet;
    PSP_DEVINFO_DATA DeviceInfoData;
    WCHAR szClassGuid[45];
    HKEY hKey = INVALID_HANDLE_VALUE;
    DWORD dwSize;
    DWORD dwType;
    LPBYTE lpBuffer = NULL;
    LPWSTR lpStr;
    INT index;
    INT len;
    LONG lError;

    if (dap->CurrentDeviceInfoSet != INVALID_HANDLE_VALUE)
    {
        DeviceInfoSet = dap->CurrentDeviceInfoSet;
        DeviceInfoData = &dap->CurrentDeviceInfoData;
    }
    else
    {
        DeviceInfoSet = dap->DeviceInfoSet;
        DeviceInfoData = &dap->DeviceInfoData;
    }

    dwSize = 45 * sizeof(WCHAR);
    if (!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                          DeviceInfoData,
                                          SPDRP_CLASSGUID,
                                          &dwType,
                                          (LPBYTE)szClassGuid,
                                          dwSize,
                                          &dwSize))
        return;

    lError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"SYSTEM\\CurrentControlSet\\Control\\CoDeviceInstallers",
                           0,
                           GENERIC_READ,
                           &hKey);
    if (lError != ERROR_SUCCESS)
        return;

    dwSize = 0;
    lError = RegQueryValueEx(hKey,
                             szClassGuid,
                             NULL,
                             &dwType,
                             NULL,
                             &dwSize);
    if (lError != ERROR_SUCCESS)
        goto done;

    if (dwSize == 0)
        goto done;

    lpBuffer = HeapAlloc(GetProcessHeap(),
                         HEAP_ZERO_MEMORY,
                         dwSize);

    RegQueryValueEx(hKey,
                    szClassGuid,
                    NULL,
                    &dwType,
                    lpBuffer,
                    &dwSize);

    lpStr = (LPWSTR)lpBuffer;
    index = 0;
    while (*lpStr != 0)
    {
        len = wcslen(lpStr) + 1;

        SetListViewText(hwndListView, index, lpStr);

        lpStr += len;
        index++;
    }

done:
    if (lpBuffer != NULL)
        HeapFree(GetProcessHeap(), 0, lpBuffer);

    if (hKey != INVALID_HANDLE_VALUE)
        RegCloseKey(hKey);
}


static VOID
DisplayDeviceCoinstallers(IN PDEVADVPROP_INFO dap,
                          IN HWND hwndListView)
{
    HDEVINFO DeviceInfoSet;
    PSP_DEVINFO_DATA DeviceInfoData;
    HKEY hKey;
    DWORD dwSize;
    DWORD dwType;
    LPBYTE lpBuffer;
    LPWSTR lpStr;
    INT index;
    INT len;

    if (dap->CurrentDeviceInfoSet != INVALID_HANDLE_VALUE)
    {
        DeviceInfoSet = dap->CurrentDeviceInfoSet;
        DeviceInfoData = &dap->CurrentDeviceInfoData;
    }
    else
    {
        DeviceInfoSet = dap->DeviceInfoSet;
        DeviceInfoData = &dap->DeviceInfoData;
    }

    hKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                DeviceInfoData,
                                DICS_FLAG_GLOBAL,
                                0,
                                DIREG_DRV,
                                KEY_QUERY_VALUE);
    if (hKey != INVALID_HANDLE_VALUE)
    {
        dwSize = 0;
        if (RegQueryValueEx(hKey,
                            L"CoInstallers32",
                            NULL,
                            &dwType,
                            NULL,
                            &dwSize) == ERROR_SUCCESS &&
            dwSize > 0)
        {

            lpBuffer = HeapAlloc(GetProcessHeap(),
                                 HEAP_ZERO_MEMORY,
                                 dwSize);

            RegQueryValueEx(hKey,
                            L"CoInstallers32",
                            NULL,
                            &dwType,
                            lpBuffer,
                            &dwSize);

            lpStr = (LPWSTR)lpBuffer;
            index = 0;
            while (*lpStr != 0)
            {
                len = wcslen(lpStr) + 1;

                SetListViewText(hwndListView, index, lpStr);

                lpStr += len;
                index++;
            }

            HeapFree(GetProcessHeap(),
                     0,
                     lpBuffer);
        }

        RegCloseKey(hKey);
    }
}


static VOID
DisplayClassProperties(IN PDEVADVPROP_INFO dap,
                       IN HWND hwndListView,
                       IN LPWSTR lpProperty)
{
    HDEVINFO DeviceInfoSet;
    PSP_DEVINFO_DATA DeviceInfoData;
    WCHAR szClassGuid[45];
    DWORD dwSize;
    DWORD dwType;
    HKEY hKey;
    GUID ClassGuid;
    LPBYTE lpBuffer;
    LPWSTR lpStr;
    INT index = 0;
    INT len;

    if (dap->CurrentDeviceInfoSet != INVALID_HANDLE_VALUE)
    {
        DeviceInfoSet = dap->CurrentDeviceInfoSet;
        DeviceInfoData = &dap->CurrentDeviceInfoData;
    }
    else
    {
        DeviceInfoSet = dap->DeviceInfoSet;
        DeviceInfoData = &dap->DeviceInfoData;
    }

    dwSize = 45 * sizeof(WCHAR);
    if (!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                          DeviceInfoData,
                                          SPDRP_CLASSGUID,
                                          &dwType,
                                          (LPBYTE)szClassGuid,
                                          dwSize,
                                          &dwSize))
        return;

    pSetupGuidFromString(szClassGuid,
                         &ClassGuid);

    hKey = SetupDiOpenClassRegKey(&ClassGuid,
                                  KEY_QUERY_VALUE);
    if (hKey != INVALID_HANDLE_VALUE)
    {
        dwSize = 0;
        if (RegQueryValueEx(hKey,
                            lpProperty,
                            NULL,
                            &dwType,
                            NULL,
                            &dwSize) == ERROR_SUCCESS &&
            dwSize > 0)
        {
            lpBuffer = HeapAlloc(GetProcessHeap(),
                                 HEAP_ZERO_MEMORY,
                                 dwSize);

            RegQueryValueEx(hKey,
                            lpProperty,
                            NULL,
                            &dwType,
                            lpBuffer,
                            &dwSize);

            if (dwType == REG_SZ)
            {
                SetListViewText(hwndListView, 0, (LPWSTR)lpBuffer);
            }
            else if (dwType == REG_MULTI_SZ)
            {
                lpStr = (LPWSTR)lpBuffer;
                index = 0;
                while (*lpStr != 0)
                {
                    len = wcslen(lpStr) + 1;

                    SetListViewText(hwndListView, index, lpStr);

                    lpStr += len;
                    index++;
                }
            }

            HeapFree(GetProcessHeap(),
                     0,
                     lpBuffer);
        }

        RegCloseKey(hKey);
    }
}


static VOID
DisplayDeviceRelations(
    IN PDEVADVPROP_INFO dap,
    IN HWND hwndListView,
    IN ULONG ulFlags)
{
    ULONG ulLength = 0;
    LPWSTR pszBuffer = NULL, pszStr;
    INT index = 0, len;

    CONFIGRET ret;

    ret = CM_Get_Device_ID_List_Size_ExW(&ulLength,
                                         dap->szDeviceID,
                                         ulFlags,
                                         NULL);
    if (ret != CR_SUCCESS)
        return;

    pszBuffer = HeapAlloc(GetProcessHeap(),
                          HEAP_ZERO_MEMORY,
                          ulLength);
    if (pszBuffer == NULL)
        return;

    ret = CM_Get_Device_ID_List_ExW(dap->szDeviceID,
                                    pszBuffer,
                                    ulLength,
                                    ulFlags,
                                    NULL);
    if (ret != CR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, pszBuffer);
        return;
    }

    pszStr = pszBuffer;
    index = 0;
    while (*pszStr != 0)
    {
        len = wcslen(pszStr) + 1;

        SetListViewText(hwndListView, index, pszStr);

        pszStr += len;
        index++;
    }

    HeapFree(GetProcessHeap(), 0, pszBuffer);
}


static VOID
DisplayDeviceProperties(IN PDEVADVPROP_INFO dap,
                        IN HWND hwndComboBox,
                        IN HWND hwndListView)
{
    INT Index;

    Index = (INT)SendMessage(hwndComboBox,
                             CB_GETCURSEL,
                             0,
                             0);
    if (Index == CB_ERR)
        return;

    (void)ListView_DeleteAllItems(hwndListView);

    switch (Index)
    {
        case 0:
            SetListViewText(hwndListView, 0, dap->szDeviceID);
            break;

        case 1: /* Hardware ID */
            DisplayDevicePropertyText(dap,
                                      hwndListView,
                                      SPDRP_HARDWAREID);
            break;

        case 2: /* Compatible IDs */
            DisplayDevicePropertyText(dap,
                                      hwndListView,
                                      SPDRP_COMPATIBLEIDS);
            break;

        case 3: /* Matching ID */
            DisplayMatchingDeviceId(dap,
                                    hwndListView);
            break;

        case 4: /* Service */
            DisplayDevicePropertyText(dap,
                                      hwndListView,
                                      SPDRP_SERVICE);
            break;

        case 5: /* Enumerator */
            DisplayDevNodeEnumerator(dap,
                                     hwndListView);
            break;

        case 6: /* Capabilities */
            DisplayDevicePropertyText(dap,
                                      hwndListView,
                                      SPDRP_CAPABILITIES);
            break;

        case 7: /* Devnode Flags */
            DisplayDevNodeFlags(dap,
                                hwndListView);
            break;

        case 8: /* Config Flags */
            DisplayDevicePropertyText(dap,
                                      hwndListView,
                                      SPDRP_CONFIGFLAGS);
            break;

        case 9: /* CSConfig Flags */
            DisplayCsFlags(dap,
                           hwndListView);
            break;

        case 10: /* Ejection relation */
            DisplayDeviceRelations(dap,
                                   hwndListView,
                                   CM_GETIDLIST_FILTER_EJECTRELATIONS);
            break;

        case 11: /* Removal relations */
            DisplayDeviceRelations(dap,
                                   hwndListView,
                                   CM_GETIDLIST_FILTER_REMOVALRELATIONS);
            break;

        case 12: /* Bus relation */
            DisplayDeviceRelations(dap,
                                   hwndListView,
                                   CM_GETIDLIST_FILTER_BUSRELATIONS);
            break;

        case 13: /* Device Upper Filters */
            DisplayDevicePropertyText(dap,
                                      hwndListView,
                                      SPDRP_UPPERFILTERS);
            break;

        case 14: /* Device Lower Filters */
            DisplayDevicePropertyText(dap,
                                      hwndListView,
                                      SPDRP_LOWERFILTERS);
            break;

        case 15: /* Class Upper Filters */
            DisplayClassProperties(dap,
                                   hwndListView,
                                   L"UpperFilters");
            break;

        case 16: /* Class Lower Filters */
            DisplayClassProperties(dap,
                                   hwndListView,
                                   L"LowerFilters");
            break;

        case 17: /* Class Installer */
            DisplayClassProperties(dap,
                                   hwndListView,
                                   L"Installer32");
            break;

        case 18: /* Class Coinstaller */
            DisplayClassCoinstallers(dap,
                                     hwndListView);
            break;

        case 19: /* Device Coinstaller */
            DisplayDeviceCoinstallers(dap,
                                      hwndListView);
            break;

#if 0
        case 20: /* Firmware Revision */
            break;

        case 21: /* Current Power State */
            break;

        case 20: /* Power Capabilities */
            break;

        case 21: /* Power State Mappings */
            break;
#endif

        default:
            SetListViewText(hwndListView, 0, L"<Not implemented yet>");
            break;
    }
}


static INT_PTR
CALLBACK
AdvProcDetailsDlgProc(IN HWND hwndDlg,
                      IN UINT uMsg,
                      IN WPARAM wParam,
                      IN LPARAM lParam)
{
    PDEVADVPROP_INFO dap;
    INT_PTR Ret = FALSE;

    dap = (PDEVADVPROP_INFO)GetWindowLongPtr(hwndDlg,
                                             DWL_USER);

    if (dap != NULL || uMsg == WM_INITDIALOG)
    {
        switch (uMsg)
        {
            case WM_COMMAND:
            {
                switch (LOWORD(wParam))
                {
                    case IDC_DETAILSPROPNAME:
                        if (HIWORD(wParam) == CBN_SELCHANGE)
                        {
                            DisplayDeviceProperties(dap,
                                                    GetDlgItem(hwndDlg, IDC_DETAILSPROPNAME),
                                                    GetDlgItem(hwndDlg, IDC_DETAILSPROPVALUE));
                        }
                        break;
                }
                break;
            }

            case WM_NOTIFY:
            {
                NMHDR *hdr = (NMHDR*)lParam;
                switch (hdr->code)
                {
                    case PSN_APPLY:
                        break;
                }
                break;
            }

            case WM_INITDIALOG:
            {
                dap = (PDEVADVPROP_INFO)((LPPROPSHEETPAGE)lParam)->lParam;
                if (dap != NULL)
                {
                    SetWindowLongPtr(hwndDlg,
                                     DWL_USER,
                                     (DWORD_PTR)dap);

                    UpdateDetailsDlg(hwndDlg,
                                     dap);
                }
                Ret = TRUE;
                break;
            }
        }
    }

    return Ret;
}


static VOID
InitDevUsageActions(IN HWND hwndDlg,
                    IN HWND hComboBox,
                    IN PDEVADVPROP_INFO dap)
{
    INT Index;
    UINT i;
    UINT Actions[] =
    {
        IDS_ENABLEDEVICE,
        IDS_DISABLEDEVICE,
    };

    for (i = 0;
         i != sizeof(Actions) / sizeof(Actions[0]);
         i++)
    {
        /* fill in the device usage combo box */
        if (LoadString(hDllInstance,
                       Actions[i],
                       dap->szTemp,
                       sizeof(dap->szTemp) / sizeof(dap->szTemp[0])))
        {
            Index = (INT)SendMessage(hComboBox,
                                     CB_ADDSTRING,
                                     0,
                                     (LPARAM)dap->szTemp);
            if (Index != CB_ERR)
            {
                SendMessage(hComboBox,
                            CB_SETITEMDATA,
                            (WPARAM)Index,
                            (LPARAM)Actions[i]);

                switch (Actions[i])
                {
                    case IDS_ENABLEDEVICE:
                        if (dap->DeviceStarted)
                        {
                            SendMessage(hComboBox,
                                        CB_SETCURSEL,
                                        (WPARAM)Index,
                                        0);
                        }
                        break;

                    case IDS_DISABLEDEVICE:
                        if (!dap->DeviceStarted)
                        {
                            SendMessage(hComboBox,
                                        CB_SETCURSEL,
                                        (WPARAM)Index,
                                        0);
                        }
                        break;

                    default:
                        break;
                }
            }
        }
    }
}


static UINT
GetSelectedUsageAction(IN HWND hComboBox)
{
    INT Index;
    UINT Ret = 0;

    Index = (INT)SendMessage(hComboBox,
                             CB_GETCURSEL,
                             0,
                             0);
    if (Index != CB_ERR)
    {
        INT iRet = (INT) SendMessage(hComboBox,
                               CB_GETITEMDATA,
                               (WPARAM)Index,
                               0);
        if (iRet != CB_ERR)
        {
            Ret = (UINT)iRet;
        }
    }

    return Ret;
}


static BOOL
ApplyGeneralSettings(IN HWND hwndDlg,
                     IN PDEVADVPROP_INFO dap)
{
    BOOL Ret = FALSE;

    if (dap->DeviceUsageChanged && dap->IsAdmin && dap->CanDisable)
    {
        UINT SelectedUsageAction;
        BOOL NeedReboot = FALSE;

        SelectedUsageAction = GetSelectedUsageAction(GetDlgItem(hwndDlg,
                                                                IDC_DEVUSAGE));
        switch (SelectedUsageAction)
        {
            case IDS_ENABLEDEVICE:
                if (!dap->DeviceStarted)
                {
                    Ret = EnableDevice(dap->DeviceInfoSet,
                                       &dap->DeviceInfoData,
                                       TRUE,
                                       0,
                                       &NeedReboot);
                }
                break;

            case IDS_DISABLEDEVICE:
                if (dap->DeviceStarted)
                {
                    Ret = EnableDevice(dap->DeviceInfoSet,
                                       &dap->DeviceInfoData,
                                       FALSE,
                                       0,
                                       &NeedReboot);
                }
                break;

            default:
                break;
        }

        if (Ret)
        {
            if (NeedReboot)
            {
                /* make PropertySheet() return PSM_REBOOTSYSTEM */
                PropSheet_RebootSystem(hwndDlg);
            }
        }
        else
        {
            /* FIXME - display an error message */
            FIXME("Failed to enable/disable device! LastError: %d\n",
                  GetLastError());
        }
    }
    else
        Ret = !dap->DeviceUsageChanged;

    /* disable the apply button */
    PropSheet_UnChanged(GetParent(hwndDlg),
                        hwndDlg);
    dap->DeviceUsageChanged = FALSE;
    return Ret;
}


static VOID
UpdateDevInfo(IN HWND hwndDlg,
              IN PDEVADVPROP_INFO dap,
              IN BOOL ReOpen)
{
    HWND hDevUsage, hPropSheetDlg, hDevProbBtn;
    CONFIGRET cr;
    ULONG Status, ProblemNumber;
    SP_DEVINSTALL_PARAMS_W InstallParams;
    UINT TroubleShootStrId = IDS_TROUBLESHOOTDEV;
    BOOL bFlag, bDevActionAvailable = TRUE;
    BOOL bDrvInstalled = FALSE;
    DWORD iPage;
    HDEVINFO DeviceInfoSet = NULL;
    PSP_DEVINFO_DATA DeviceInfoData = NULL;
    PROPSHEETHEADER psh;
    DWORD nDriverPages = 0;
    BOOL RecalcPages = FALSE;

    TRACE("UpdateDevInfo()\n");

    hPropSheetDlg = GetParent(hwndDlg);

    if (dap->PageInitialized)
    {
        /* switch to the General page */
        PropSheet_SetCurSelByID(hPropSheetDlg,
                                IDD_DEVICEGENERAL);

        /* remove and destroy the existing device property sheet pages */
        if (dap->DevPropSheets != NULL)
        {
            for (iPage = 0;
                 iPage != dap->nDevPropSheets;
                 iPage++)
            {
                if (dap->DevPropSheets[iPage] != NULL)
                {
                    PropSheet_RemovePage(hPropSheetDlg,
                                         (WPARAM) -1,
                                         dap->DevPropSheets[iPage]);
                    RecalcPages = TRUE;
                }
            }
        }
    }

    iPage = 0;

    if (dap->FreeDevPropSheets)
    {
        /* don't free the array if it's the one allocated in
           DisplayDeviceAdvancedProperties */
        HeapFree(GetProcessHeap(),
                 0,
                 dap->DevPropSheets);

        dap->FreeDevPropSheets = FALSE;
    }

    dap->DevPropSheets = NULL;
    dap->nDevPropSheets = 0;

    if (ReOpen)
    {
        /* create a new device info set and re-open the device */
        if (dap->CurrentDeviceInfoSet != INVALID_HANDLE_VALUE)
        {
            SetupDiDestroyDeviceInfoList(dap->CurrentDeviceInfoSet);
        }

        dap->ParentDevInst = 0;
        dap->CurrentDeviceInfoSet = SetupDiCreateDeviceInfoListEx(NULL,
                                                                  hwndDlg,
                                                                  dap->lpMachineName,
                                                                  NULL);
        if (dap->CurrentDeviceInfoSet != INVALID_HANDLE_VALUE)
        {
            if (SetupDiOpenDeviceInfo(dap->CurrentDeviceInfoSet,
                                      dap->szDeviceID,
                                      hwndDlg,
                                      0,
                                      &dap->CurrentDeviceInfoData))
            {
                if (dap->CloseDevInst)
                {
                    SetupDiDestroyDeviceInfoList(dap->DeviceInfoSet);
                }

                dap->CloseDevInst = TRUE;
                dap->DeviceInfoSet = dap->CurrentDeviceInfoSet;
                dap->DeviceInfoData = dap->CurrentDeviceInfoData;
                dap->CurrentDeviceInfoSet = INVALID_HANDLE_VALUE;
            }
            else
                goto GetParentNode;
        }
        else
        {
GetParentNode:
            /* get the parent node from the initial devinst */
            CM_Get_Parent_Ex(&dap->ParentDevInst,
                             dap->DeviceInfoData.DevInst,
                             0,
                             dap->hMachine);
        }

        if (dap->CurrentDeviceInfoSet != INVALID_HANDLE_VALUE)
        {
            DeviceInfoSet = dap->CurrentDeviceInfoSet;
            DeviceInfoData = &dap->CurrentDeviceInfoData;
        }
        else
        {
            DeviceInfoSet = dap->DeviceInfoSet;
            DeviceInfoData = &dap->DeviceInfoData;
        }
    }
    else
    {
        DeviceInfoSet = dap->DeviceInfoSet;
        DeviceInfoData = &dap->DeviceInfoData;
    }

    dap->HasDriverPage = FALSE;
    dap->HasResourcePage = FALSE;
    dap->HasPowerPage = FALSE;
    if (IsDriverInstalled(DeviceInfoData->DevInst,
                          dap->hMachine,
                          &bDrvInstalled) &&
        bDrvInstalled)
    {
        if (SetupDiCallClassInstaller((dap->ShowRemotePages ?
                                           DIF_ADDREMOTEPROPERTYPAGE_ADVANCED :
                                           DIF_ADDPROPERTYPAGE_ADVANCED),
                                      DeviceInfoSet,
                                      DeviceInfoData))
        {
            /* get install params */
            InstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS_W);
            if (!SetupDiGetDeviceInstallParamsW(DeviceInfoSet,
                                                DeviceInfoData,
                                                &InstallParams))
            {
                /* zero the flags */
                InstallParams.Flags = 0;
            }

            dap->HasDriverPage = !(InstallParams.Flags & DI_DRIVERPAGE_ADDED);
            dap->HasResourcePage = !(InstallParams.Flags & DI_RESOURCEPAGE_ADDED);
            dap->HasPowerPage = !(InstallParams.Flags & DI_FLAGSEX_POWERPAGE_ADDED);
        }
    }

    /* get the device icon */
    if (dap->hDevIcon != NULL)
    {
        DestroyIcon(dap->hDevIcon);
        dap->hDevIcon = NULL;
    }
    if (!SetupDiLoadClassIcon(&DeviceInfoData->ClassGuid,
                              &dap->hDevIcon,
                              NULL))
    {
        dap->hDevIcon = NULL;
    }

    /* get the device name */
    if (GetDeviceDescriptionString(DeviceInfoSet,
                                   DeviceInfoData,
                                   dap->szDevName,
                                   sizeof(dap->szDevName) / sizeof(dap->szDevName[0])))
    {
        PropSheet_SetTitle(hPropSheetDlg,
                           PSH_PROPTITLE,
                           dap->szDevName);
    }

    /* set the device image */
    SendDlgItemMessage(hwndDlg,
                       IDC_DEVICON,
                       STM_SETICON,
                       (WPARAM)dap->hDevIcon,
                       0);

    /* set the device name edit control text */
    SetDlgItemText(hwndDlg,
                   IDC_DEVNAME,
                   dap->szDevName);

    /* set the device type edit control text */
    if (GetDeviceTypeString(DeviceInfoData,
                            dap->szTemp,
                            sizeof(dap->szTemp) / sizeof(dap->szTemp[0])))
    {
        SetDlgItemText(hwndDlg,
                       IDC_DEVTYPE,
                       dap->szTemp);
    }

    /* set the device manufacturer edit control text */
    if (GetDeviceManufacturerString(DeviceInfoSet,
                                    DeviceInfoData,
                                    dap->szTemp,
                                    sizeof(dap->szTemp) / sizeof(dap->szTemp[0])))
    {
        SetDlgItemText(hwndDlg,
                       IDC_DEVMANUFACTURER,
                       dap->szTemp);
    }

    /* set the device location edit control text */
    if (GetDeviceLocationString(DeviceInfoSet,
                                DeviceInfoData,
                                dap->ParentDevInst,
                                dap->szTemp,
                                sizeof(dap->szTemp) / sizeof(dap->szTemp[0])))
    {
        SetDlgItemText(hwndDlg,
                       IDC_DEVLOCATION,
                       dap->szTemp);
    }

    /* set the device status edit control text */
    if (GetDeviceStatusString(DeviceInfoData->DevInst,
                              dap->hMachine,
                              dap->szTemp,
                              sizeof(dap->szTemp) / sizeof(dap->szTemp[0])))
    {
        SetDlgItemText(hwndDlg,
                       IDC_DEVSTATUS,
                       dap->szTemp);
    }

    /* set the device troubleshoot button text and disable it if necessary */
    hDevProbBtn = GetDlgItem(hwndDlg,
                             IDC_DEVPROBLEM);
    cr = CM_Get_DevNode_Status_Ex(&Status,
                                  &ProblemNumber,
                                  DeviceInfoData->DevInst,
                                  0,
                                  dap->hMachine);
    if (cr == CR_SUCCESS && (Status & DN_HAS_PROBLEM))
    {
        switch (ProblemNumber)
        {
            case CM_PROB_DEVLOADER_FAILED:
            {
                /* FIXME - only if it's not a root bus devloader,
                           disable the button otherwise */
                TroubleShootStrId = IDS_UPDATEDRV;
                break;
            }

            case CM_PROB_OUT_OF_MEMORY:
            case CM_PROB_ENTRY_IS_WRONG_TYPE:
            case CM_PROB_LACKED_ARBITRATOR:
            case CM_PROB_FAILED_START:
            case CM_PROB_LIAR:
            case CM_PROB_UNKNOWN_RESOURCE:
            {
                TroubleShootStrId = IDS_UPDATEDRV;
                break;
            }

            case CM_PROB_BOOT_CONFIG_CONFLICT:
            case CM_PROB_NORMAL_CONFLICT:
            case CM_PROB_REENUMERATION:
            {
                /* FIXME - Troubleshoot conflict */
                break;
            }

            case CM_PROB_FAILED_FILTER:
            case CM_PROB_REINSTALL:
            case CM_PROB_FAILED_INSTALL:
            {
                TroubleShootStrId = IDS_REINSTALLDRV;
                break;
            }

            case CM_PROB_DEVLOADER_NOT_FOUND:
            {
                /* FIXME - 4 cases:
                   1) if it's a missing system devloader:
                      - disable the button (Reinstall Driver)
                   2) if it's not a system devloader but still missing:
                      - Reinstall Driver
                   3) if it's not a system devloader but the file can be found:
                      - Update Driver
                   4) if it's a missing or empty software key
                      - Update Driver
                 */
                break;
            }

            case CM_PROB_INVALID_DATA:
            case CM_PROB_PARTIAL_LOG_CONF:
            case CM_PROB_NO_VALID_LOG_CONF:
            case CM_PROB_HARDWARE_DISABLED:
            case CM_PROB_CANT_SHARE_IRQ:
            case CM_PROB_TRANSLATION_FAILED:
            case CM_PROB_SYSTEM_SHUTDOWN:
            case CM_PROB_PHANTOM:
                bDevActionAvailable = FALSE;
                break;

            case CM_PROB_NOT_VERIFIED:
            case CM_PROB_DEVICE_NOT_THERE:
                /* FIXME - search hardware */
                break;

            case CM_PROB_NEED_RESTART:
            case CM_PROB_WILL_BE_REMOVED:
            case CM_PROB_MOVED:
            case CM_PROB_TOO_EARLY:
            case CM_PROB_DISABLED_SERVICE:
                TroubleShootStrId = IDS_REBOOT;
                break;

            case CM_PROB_REGISTRY:
                /* FIXME - check registry? */
                break;

            case CM_PROB_DISABLED:
                /* if device was disabled by the user: */
                TroubleShootStrId = IDS_ENABLEDEV;
                /* FIXME - otherwise disable button because the device was
                           disabled by the system*/
                break;

            case CM_PROB_DEVLOADER_NOT_READY:
                /* FIXME - if it's a graphics adapter:
                           - if it's a a secondary adapter and the main adapter
                             couldn't be found
                             - disable  button
                           - else
                             - Properties
                         - else
                           - Update driver
                 */
                break;

            case CM_PROB_FAILED_ADD:
                TroubleShootStrId = IDS_PROPERTIES;
                break;
        }
    }

    if (LoadString(hDllInstance,
                   TroubleShootStrId,
                   dap->szTemp,
                   sizeof(dap->szTemp) / sizeof(dap->szTemp[0])) != 0)
    {
        SetWindowText(hDevProbBtn,
                      dap->szTemp);
    }
    EnableWindow(hDevProbBtn,
                 dap->IsAdmin && bDevActionAvailable);

    /* check if the device can be enabled/disabled */
    hDevUsage = GetDlgItem(hwndDlg,
                           IDC_DEVUSAGE);

    dap->CanDisable = FALSE;
    dap->DeviceStarted = FALSE;

    if (CanDisableDevice(DeviceInfoData->DevInst,
                         dap->hMachine,
                         &bFlag))
    {
        dap->CanDisable = bFlag;
    }

    if (IsDeviceStarted(DeviceInfoData->DevInst,
                        dap->hMachine,
                        &bFlag))
    {
        dap->DeviceStarted = bFlag;
    }

    /* enable/disable the device usage controls */
    EnableWindow(GetDlgItem(hwndDlg,
                            IDC_DEVUSAGELABEL),
                 dap->CanDisable && dap->IsAdmin);
    EnableWindow(hDevUsage,
                 dap->CanDisable && dap->IsAdmin);

    /* clear the combobox */
    SendMessage(hDevUsage,
                CB_RESETCONTENT,
                0,
                0);
    if (dap->CanDisable)
    {
        InitDevUsageActions(hwndDlg,
                            hDevUsage,
                            dap);
    }

    /* find out how many new device property sheets to add.
       fake a PROPSHEETHEADER structure, we don't plan to
       call PropertySheet again!*/
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = 0;
    psh.nPages = 0;

    /* get the number of device property sheets for the device */
    if (!SetupDiGetClassDevPropertySheets(DeviceInfoSet,
                                          DeviceInfoData,
                                          &psh,
                                          0,
                                          &nDriverPages,
                                          dap->PropertySheetType) &&
        nDriverPages != 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
TRACE("Count %d additional pages!\n", nDriverPages);
        dap->nDevPropSheets += nDriverPages;
    }
    else
    {
        nDriverPages = 0;
    }

    /* include the driver page */
    if (dap->HasDriverPage)
        dap->nDevPropSheets++;

    /* include the details page */
    if (dap->Extended)
        dap->nDevPropSheets++;

    if (dap->HasResourcePage)
        dap->nDevPropSheets++;

    /* add the device property sheets */
    if (dap->nDevPropSheets != 0)
    {
TRACE("Show %d pages!\n", dap->nDevPropSheets);
        dap->DevPropSheets = HeapAlloc(GetProcessHeap(),
                                       HEAP_ZERO_MEMORY,
                                       dap->nDevPropSheets * sizeof(HPROPSHEETPAGE));
        if (dap->DevPropSheets != NULL)
        {
            if (nDriverPages != 0)
            {
                psh.phpage = dap->DevPropSheets;

                /* query the device property sheet pages to add */
                if (SetupDiGetClassDevPropertySheets(DeviceInfoSet,
                                                     DeviceInfoData,
                                                     &psh,
                                                     dap->nDevPropSheets,
                                                     NULL,
                                                     dap->PropertySheetType))
                {
                    /* add the property sheets */
                    for (iPage = 0;
                         iPage < nDriverPages;
                         iPage++)
                    {
TRACE("Add page %d\n", iPage);
TRACE("Sheet %p\n", dap->DevPropSheets[iPage]);

                        if (PropSheet_AddPage(hPropSheetDlg,
                                              dap->DevPropSheets[iPage]))
                        {
                            RecalcPages = TRUE;
                        }
                        else
                        {
TRACE("PropSheet_AddPage() failed\n");
                        }
                    }

                    dap->FreeDevPropSheets = TRUE;
                }
                else
                {
TRACE("SetupDiGetClassDevPropertySheets() failed\n");
                    /* cleanup, we were unable to get the device property sheets */
                    iPage = nDriverPages;
                    dap->nDevPropSheets -= nDriverPages;
                    nDriverPages = 0;
                }
            }
            else
                iPage = 0;

            /* add the driver page if necessary */
            if (dap->HasDriverPage)
            {
                PROPSHEETPAGE pspDriver = {0};
                pspDriver.dwSize = sizeof(PROPSHEETPAGE);
                pspDriver.dwFlags = PSP_DEFAULT;
                pspDriver.hInstance = hDllInstance;
                pspDriver.pszTemplate = (LPCWSTR)MAKEINTRESOURCE(IDD_DEVICEDRIVER);
                pspDriver.pfnDlgProc = AdvProcDriverDlgProc;
                pspDriver.lParam = (LPARAM)dap;
                dap->DevPropSheets[iPage] = dap->pCreatePropertySheetPageW(&pspDriver);
                if (dap->DevPropSheets[iPage] != NULL)
                {
                    if (PropSheet_AddPage(hPropSheetDlg,
                                          dap->DevPropSheets[iPage]))
                    {
                        iPage++;
                        RecalcPages = TRUE;
                    }
                    else
                    {
                        dap->pDestroyPropertySheetPage(dap->DevPropSheets[iPage]);
                        dap->DevPropSheets[iPage] = NULL;
                    }
                }
            }

            if (dap->Extended)
            {
                /* Add the details page */
                PROPSHEETPAGE pspDetails = {0};
                pspDetails.dwSize = sizeof(PROPSHEETPAGE);
                pspDetails.dwFlags = PSP_DEFAULT;
                pspDetails.hInstance = hDllInstance;
                pspDetails.pszTemplate = (LPCWSTR)MAKEINTRESOURCE(IDD_DEVICEDETAILS);
                pspDetails.pfnDlgProc = AdvProcDetailsDlgProc;
                pspDetails.lParam = (LPARAM)dap;
                dap->DevPropSheets[iPage] = dap->pCreatePropertySheetPageW(&pspDetails);
                if (dap->DevPropSheets[iPage] != NULL)
                {
                    if (PropSheet_AddPage(hPropSheetDlg,
                                          dap->DevPropSheets[iPage]))
                    {
                        iPage++;
                        RecalcPages = TRUE;
                    }
                    else
                    {
                        dap->pDestroyPropertySheetPage(dap->DevPropSheets[iPage]);
                        dap->DevPropSheets[iPage] = NULL;
                    }
                }
            }

            if (dap->HasResourcePage)
            {
                PROPSHEETPAGE pspDriver = {0};
                pspDriver.dwSize = sizeof(PROPSHEETPAGE);
                pspDriver.dwFlags = PSP_DEFAULT;
                pspDriver.hInstance = hDllInstance;
                pspDriver.pszTemplate = (LPCWSTR)MAKEINTRESOURCE(IDD_DEVICERESOURCES);
                pspDriver.pfnDlgProc = ResourcesProcDriverDlgProc;
                pspDriver.lParam = (LPARAM)dap;
                dap->DevPropSheets[iPage] = dap->pCreatePropertySheetPageW(&pspDriver);
                if (dap->DevPropSheets[iPage] != NULL)
                {
                    if (PropSheet_AddPage(hPropSheetDlg,
                                          dap->DevPropSheets[iPage]))
                    {
                        iPage++;
                        RecalcPages = TRUE;
                    }
                    else
                    {
                        dap->pDestroyPropertySheetPage(dap->DevPropSheets[iPage]);
                        dap->DevPropSheets[iPage] = NULL;
                    }
                }
            }
            /* FIXME: Add the power page */
        }
        else
            dap->nDevPropSheets = 0;
    }

    if (RecalcPages)
    {
        PropSheet_RecalcPageSizes(hPropSheetDlg);
    }

    /* finally, disable the apply button */
    PropSheet_UnChanged(hPropSheetDlg,
                        hwndDlg);
    dap->DeviceUsageChanged = FALSE;
}


static LRESULT
CALLBACK
DlgParentSubWndProc(IN HWND hwnd,
                    IN UINT uMsg,
                    IN WPARAM wParam,
                    IN LPARAM lParam)
{
    PDEVADVPROP_INFO dap;

    dap = (PDEVADVPROP_INFO)GetProp(hwnd,
                                    L"DevMgrDevChangeSub");
    if (dap != NULL)
    {
        if (uMsg == WM_DEVICECHANGE && !IsWindowVisible(dap->hWndGeneralPage))
        {
            SendMessage(dap->hWndGeneralPage,
                        WM_DEVICECHANGE,
                        wParam,
                        lParam);
        }

        /* pass the message the the old window proc */
        return CallWindowProc(dap->ParentOldWndProc,
                              hwnd,
                              uMsg,
                              wParam,
                              lParam);
    }
    else
    {
        /* this is not a good idea if the subclassed window was an ansi
           window, but we failed finding out the previous window proc
           so we can't use CallWindowProc. This should rarely - if ever -
           happen. */

        return DefWindowProc(hwnd,
                             uMsg,
                             wParam,
                             lParam);
    }
}


static INT_PTR
CALLBACK
AdvPropGeneralDlgProc(IN HWND hwndDlg,
                      IN UINT uMsg,
                      IN WPARAM wParam,
                      IN LPARAM lParam)
{
    PDEVADVPROP_INFO dap;
    INT_PTR Ret = FALSE;

    dap = (PDEVADVPROP_INFO)GetWindowLongPtr(hwndDlg,
                                             DWL_USER);

    if (dap != NULL || uMsg == WM_INITDIALOG)
    {
        switch (uMsg)
        {
            case WM_COMMAND:
            {
                switch (LOWORD(wParam))
                {
                    case IDC_DEVUSAGE:
                    {
                        if (HIWORD(wParam) == CBN_SELCHANGE)
                        {
                            PropSheet_Changed(GetParent(hwndDlg),
                                              hwndDlg);
                            dap->DeviceUsageChanged = TRUE;
                        }
                        break;
                    }

                    case IDC_DEVPROBLEM:
                    {
                        if (dap->IsAdmin)
                        {
                            /* display the device problem wizard */
                            ShowDeviceProblemWizard(hwndDlg,
                                                    dap->DeviceInfoSet,
                                                    &dap->DeviceInfoData,
                                                    dap->hMachine);
                        }
                        break;
                    }
                }
                break;
            }

            case WM_NOTIFY:
            {
                NMHDR *hdr = (NMHDR*)lParam;
                switch (hdr->code)
                {
                    case PSN_APPLY:
                        ApplyGeneralSettings(hwndDlg,
                                             dap);
                        break;
                }
                break;
            }

            case WM_INITDIALOG:
            {
                dap = (PDEVADVPROP_INFO)((LPPROPSHEETPAGE)lParam)->lParam;
                if (dap != NULL)
                {
                    HWND hWndParent;

                    dap->hWndGeneralPage = hwndDlg;

                    SetWindowLongPtr(hwndDlg,
                                     DWL_USER,
                                     (DWORD_PTR)dap);

                    /* subclass the parent window to always receive
                       WM_DEVICECHANGE messages */
                    hWndParent = GetParent(hwndDlg);
                    if (hWndParent != NULL)
                    {
                        /* subclass the parent window. This is not safe
                           if the parent window belongs to another thread! */
                        dap->ParentOldWndProc = (WNDPROC)SetWindowLongPtr(hWndParent,
                                                                          GWLP_WNDPROC,
                                                                          (LONG_PTR)DlgParentSubWndProc);

                        if (dap->ParentOldWndProc != NULL &&
                            SetProp(hWndParent,
                                    L"DevMgrDevChangeSub",
                                    (HANDLE)dap))
                        {
                            dap->hWndParent = hWndParent;
                        }
                    }

                    /* do not call UpdateDevInfo directly in here because it modifies
                       the pages of the property sheet! */
                    PostMessage(hwndDlg,
                                PM_INITIALIZE,
                                0,
                                0);
                }
                Ret = TRUE;
                break;
            }

            case WM_DEVICECHANGE:
            {
                /* FIXME - don't call UpdateDevInfo for all events */
                UpdateDevInfo(hwndDlg,
                              dap,
                              TRUE);
                Ret = TRUE;
                break;
            }

            case PM_INITIALIZE:
            {
                UpdateDevInfo(hwndDlg,
                              dap,
                              FALSE);
                dap->PageInitialized = TRUE;
                break;
            }

            case WM_DESTROY:
            {
                /* restore the old window proc of the subclassed parent window */
                if (dap->hWndParent != NULL && dap->ParentOldWndProc != NULL)
                {
                    if (SetWindowLongPtr(dap->hWndParent,
                                         GWLP_WNDPROC,
                                         (LONG_PTR)dap->ParentOldWndProc) == (LONG_PTR)DlgParentSubWndProc)
                    {
                        RemoveProp(dap->hWndParent,
                                   L"DevMgrDevChangeSub");
                    }
                }
                break;
            }
        }
    }

    return Ret;
}


INT_PTR
DisplayDeviceAdvancedProperties(IN HWND hWndParent,
                                IN LPCWSTR lpDeviceID  OPTIONAL,
                                IN HDEVINFO DeviceInfoSet,
                                IN PSP_DEVINFO_DATA DeviceInfoData,
                                IN HINSTANCE hComCtl32,
                                IN LPCWSTR lpMachineName,
                                IN DWORD dwFlags)
{
    PROPSHEETHEADER psh = {0};
    PROPSHEETPAGE pspGeneral = {0};
    PPROPERTYSHEETW pPropertySheetW;
    PCREATEPROPERTYSHEETPAGEW pCreatePropertySheetPageW;
    PDESTROYPROPERTYSHEETPAGE pDestroyPropertySheetPage;
    PDEVADVPROP_INFO DevAdvPropInfo;
    HMACHINE hMachine = NULL;
    DWORD DevIdSize = 0;
    INT_PTR Ret = -1;

    /* we don't want to statically link against comctl32, so find the
       functions we need dynamically */
    pPropertySheetW =
        (PPROPERTYSHEETW)GetProcAddress(hComCtl32,
                                        "PropertySheetW");
    pCreatePropertySheetPageW =
        (PCREATEPROPERTYSHEETPAGEW)GetProcAddress(hComCtl32,
                                                  "CreatePropertySheetPageW");
    pDestroyPropertySheetPage =
        (PDESTROYPROPERTYSHEETPAGE)GetProcAddress(hComCtl32,
                                                  "DestroyPropertySheetPage");
    if (pPropertySheetW == NULL ||
        pCreatePropertySheetPageW == NULL ||
        pDestroyPropertySheetPage == NULL)
    {
        return -1;
    }

    if (lpDeviceID == NULL)
    {
        /* find out how much size is needed for the device id */
        if (SetupDiGetDeviceInstanceId(DeviceInfoSet,
                                       DeviceInfoData,
                                       NULL,
                                       0,
                                       &DevIdSize))
        {
            ERR("SetupDiGetDeviceInstanceId unexpectedly returned TRUE!\n");
            return -1;
        }

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            return -1;
        }
    }
    else
    {
        DevIdSize = (DWORD)wcslen(lpDeviceID) + 1;
    }

    if (lpMachineName != NULL && lpMachineName[0] != L'\0')
    {
        CONFIGRET cr = CM_Connect_Machine(lpMachineName,
                                          &hMachine);
        if (cr != CR_SUCCESS)
        {
            return -1;
        }
    }

    /* create the internal structure associated with the "General",
       "Driver", ... pages */
    DevAdvPropInfo = HeapAlloc(GetProcessHeap(),
                               HEAP_ZERO_MEMORY,
                               FIELD_OFFSET(DEVADVPROP_INFO,
                                            szDeviceID) +
                                   (DevIdSize * sizeof(WCHAR)));
    if (DevAdvPropInfo == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Cleanup;
    }

    if (lpDeviceID == NULL)
    {
        /* read the device instance id */
        if (!SetupDiGetDeviceInstanceId(DeviceInfoSet,
                                        DeviceInfoData,
                                        DevAdvPropInfo->szDeviceID,
                                        DevIdSize,
                                        NULL))
        {
            goto Cleanup;
        }
    }
    else
    {
        /* copy the device instance id supplied by the caller */
        wcscpy(DevAdvPropInfo->szDeviceID,
               lpDeviceID);
    }

    DevAdvPropInfo->DeviceInfoSet = DeviceInfoSet;
    DevAdvPropInfo->DeviceInfoData = *DeviceInfoData;
    DevAdvPropInfo->CurrentDeviceInfoSet = INVALID_HANDLE_VALUE;
    DevAdvPropInfo->CurrentDeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    DevAdvPropInfo->ShowRemotePages = (lpMachineName != NULL && lpMachineName[0] != L'\0');
    DevAdvPropInfo->hMachine = hMachine;
    DevAdvPropInfo->lpMachineName = lpMachineName;
    DevAdvPropInfo->szDevName[0] = L'\0';
    DevAdvPropInfo->hComCtl32 = hComCtl32;
    DevAdvPropInfo->pCreatePropertySheetPageW = pCreatePropertySheetPageW;
    DevAdvPropInfo->pDestroyPropertySheetPage = pDestroyPropertySheetPage;

    DevAdvPropInfo->IsAdmin = IsUserAdmin();
    DevAdvPropInfo->DoDefaultDevAction = ((dwFlags & DPF_DEVICE_STATUS_ACTION) != 0);
    DevAdvPropInfo->Extended = ((dwFlags & DPF_EXTENDED) != 0);

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPTITLE | PSH_NOAPPLYNOW;
    psh.hwndParent = hWndParent;
    psh.pszCaption = DevAdvPropInfo->szDevName;

    DevAdvPropInfo->PropertySheetType = DevAdvPropInfo->ShowRemotePages ?
                                            DIGCDP_FLAG_REMOTE_ADVANCED :
                                            DIGCDP_FLAG_ADVANCED;

    psh.phpage = HeapAlloc(GetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           1 * sizeof(HPROPSHEETPAGE));
    if (psh.phpage == NULL)
    {
        goto Cleanup;
    }

    /* add the "General" property sheet */
    pspGeneral.dwSize = sizeof(PROPSHEETPAGE);
    pspGeneral.dwFlags = PSP_DEFAULT;
    pspGeneral.hInstance = hDllInstance;
    pspGeneral.pszTemplate = (LPCWSTR)MAKEINTRESOURCE(IDD_DEVICEGENERAL);
    pspGeneral.pfnDlgProc = AdvPropGeneralDlgProc;
    pspGeneral.lParam = (LPARAM)DevAdvPropInfo;
    psh.phpage[psh.nPages] = pCreatePropertySheetPageW(&pspGeneral);
    if (psh.phpage[psh.nPages] != NULL)
    {
        psh.nPages++;
    }

    DevAdvPropInfo->nDevPropSheets = psh.nPages;

    if (psh.nPages != 0)
    {
        Ret = pPropertySheetW(&psh);

        /* NOTE: no need to destroy the property sheets anymore! */
    }
    else
    {
        UINT i;

Cleanup:
        /* in case of failure the property sheets must be destroyed */
        if (psh.phpage != NULL)
        {
            for (i = 0;
                 i < psh.nPages;
                 i++)
            {
                if (psh.phpage[i] != NULL)
                {
                    pDestroyPropertySheetPage(psh.phpage[i]);
                }
            }
        }
    }

    if (DevAdvPropInfo != NULL)
    {
        if (DevAdvPropInfo->FreeDevPropSheets)
        {
            /* don't free the array if it's the one allocated in
               DisplayDeviceAdvancedProperties */
            HeapFree(GetProcessHeap(),
                     0,
                     DevAdvPropInfo->DevPropSheets);
        }

        if (DevAdvPropInfo->CloseDevInst)
        {
            /* close the device info set in case a new one was created */
            SetupDiDestroyDeviceInfoList(DevAdvPropInfo->DeviceInfoSet);
        }

        if (DevAdvPropInfo->CurrentDeviceInfoSet != INVALID_HANDLE_VALUE)
        {
            SetupDiDestroyDeviceInfoList(DevAdvPropInfo->CurrentDeviceInfoSet);
        }

        if (DevAdvPropInfo->hDevIcon != NULL)
        {
            DestroyIcon(DevAdvPropInfo->hDevIcon);
        }

        HeapFree(GetProcessHeap(),
                 0,
                 DevAdvPropInfo);
    }

    if (psh.phpage != NULL)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 psh.phpage);
    }

    if (hMachine != NULL)
    {
        CM_Disconnect_Machine(hMachine);
    }

    return Ret;
}


static BOOL
GetDeviceAndComputerName(LPWSTR lpString,
                         WCHAR szDeviceID[],
                         WCHAR szMachineName[])
{
    BOOL ret = FALSE;

    szDeviceID[0] = L'\0';
    szMachineName[0] = L'\0';

    while (*lpString != L'\0')
    {
        if (*lpString == L'/')
        {
            lpString++;
            if(!_wcsnicmp(lpString, L"DeviceID", 8))
            {
                lpString += 9;
                if (*lpString != L'\0')
                {
                    int i = 0;
                    while ((*lpString != L' ') &&
                           (*lpString != L'\0') &&
                           (i <= MAX_DEVICE_ID_LEN))
                    {
                        szDeviceID[i++] = *lpString++;
                    }
                    szDeviceID[i] = L'\0';
                    ret = TRUE;
                }
            }
            else if (!_wcsnicmp(lpString, L"MachineName", 11))
            {
                lpString += 12;
                if (*lpString != L'\0')
                {
                    int i = 0;
                    while ((*lpString != L' ') &&
                           (*lpString != L'\0') &&
                           (i <= MAX_COMPUTERNAME_LENGTH))
                    {
                        szMachineName[i++] = *lpString++;
                    }
                    szMachineName[i] = L'\0';
                }
            }
            /* knock the pointer back one and let the next
             * pointer deal with incrementing, otherwise we
             * go past the end of the string */
             lpString--;
        }
        lpString++;
    }

    return ret;
}


/***************************************************************************
 * NAME                                                         EXPORTED
 *      DeviceAdvancedPropertiesW
 *
 * DESCRIPTION
 *   Invokes the device properties dialog, this version may add some property pages
 *   for some devices
 *
 * ARGUMENTS
 *   hWndParent:    Handle to the parent window
 *   lpMachineName: Machine Name, NULL is the local machine
 *   lpDeviceID:    Specifies the device whose properties are to be shown
 *
 * RETURN VALUE
 *   Always returns -1, a call to GetLastError returns 0 if successful
 *
 * @implemented
 */
INT_PTR
WINAPI
DeviceAdvancedPropertiesW(IN HWND hWndParent  OPTIONAL,
                          IN LPCWSTR lpMachineName  OPTIONAL,
                          IN LPCWSTR lpDeviceID)
{
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA DevInfoData;
    HINSTANCE hComCtl32;
    INT_PTR Ret = -1;

    if (lpDeviceID == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* dynamically load comctl32 */
    hComCtl32 = LoadAndInitComctl32();
    if (hComCtl32 != NULL)
    {
        hDevInfo = SetupDiCreateDeviceInfoListEx(NULL,
                                                 hWndParent,
                                                 lpMachineName,
                                                 NULL);
        if (hDevInfo != INVALID_HANDLE_VALUE)
        {
            DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
            if (SetupDiOpenDeviceInfo(hDevInfo,
                                      lpDeviceID,
                                      hWndParent,
                                      0,
                                      &DevInfoData))
            {
                Ret = DisplayDeviceAdvancedProperties(hWndParent,
                                                      lpDeviceID,
                                                      hDevInfo,
                                                      &DevInfoData,
                                                      hComCtl32,
                                                      lpMachineName,
                                                      0);
            }

            SetupDiDestroyDeviceInfoList(hDevInfo);
        }

        FreeLibrary(hComCtl32);
    }

    return Ret;
}


/***************************************************************************
 * NAME                                                         EXPORTED
 *      DeviceAdvancedPropertiesA
 *
 * DESCRIPTION
 *   Invokes the device properties dialog, this version may add some property pages
 *   for some devices
 *
 * ARGUMENTS
 *   hWndParent:    Handle to the parent window
 *   lpMachineName: Machine Name, NULL is the local machine
 *   lpDeviceID:    Specifies the device whose properties are to be shown
 *
 * RETURN VALUE
 *   Always returns -1, a call to GetLastError returns 0 if successful
 *
 * @implemented
 */
INT_PTR
WINAPI
DeviceAdvancedPropertiesA(IN HWND hWndParent  OPTIONAL,
                          IN LPCSTR lpMachineName  OPTIONAL,
                          IN LPCSTR lpDeviceID)
{
    LPWSTR lpMachineNameW = NULL;
    LPWSTR lpDeviceIDW = NULL;
    INT_PTR Ret = -1;

    if (lpMachineName != NULL)
    {
        if (!(lpMachineNameW = ConvertMultiByteToUnicode(lpMachineName,
                                                         CP_ACP)))
        {
            goto Cleanup;
        }
    }
    if (lpDeviceID != NULL)
    {
        if (!(lpDeviceIDW = ConvertMultiByteToUnicode(lpDeviceID,
                                                      CP_ACP)))
        {
            goto Cleanup;
        }
    }

    Ret = DeviceAdvancedPropertiesW(hWndParent,
                                    lpMachineNameW,
                                    lpDeviceIDW);

Cleanup:
    if (lpMachineNameW != NULL)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 lpMachineNameW);
    }
    if (lpDeviceIDW != NULL)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 lpDeviceIDW);
    }

    return Ret;
}


/***************************************************************************
 * NAME                                                         EXPORTED
 *      DevicePropertiesExA
 *
 * DESCRIPTION
 *   Invokes the extended device properties dialog
 *
 * ARGUMENTS
 *   hWndParent:    Handle to the parent window
 *   lpMachineName: Machine Name, NULL is the local machine
 *   lpDeviceID:    Specifies the device whose properties are to be shown, optional if
 *                  bShowDevMgr is nonzero
 *   dwFlags:       This parameter can be a combination of the following flags:
 *                  * DPF_DEVICE_STATUS_ACTION: Only valid if bShowDevMgr, causes
 *                                              the default device status action button
 *                                              to be clicked (Troubleshoot, Enable
 *                                              Device, etc)
 *   bShowDevMgr:   If non-zero it displays the device manager instead of
 *                  the advanced device property dialog
 *
 * RETURN VALUE
 *   1:  if bShowDevMgr is non-zero and no error occured
 *   -1: a call to GetLastError returns 0 if successful
 *
 * @implemented
 */
INT_PTR
WINAPI
DevicePropertiesExA(IN HWND hWndParent  OPTIONAL,
                    IN LPCSTR lpMachineName  OPTIONAL,
                    IN LPCSTR lpDeviceID  OPTIONAL,
                    IN DWORD dwFlags  OPTIONAL,
                    IN BOOL bShowDevMgr)
{
    LPWSTR lpMachineNameW = NULL;
    LPWSTR lpDeviceIDW = NULL;
    INT_PTR Ret = -1;

    if (lpMachineName != NULL)
    {
        if (!(lpMachineNameW = ConvertMultiByteToUnicode(lpMachineName,
                                                         CP_ACP)))
        {
            goto Cleanup;
        }
    }
    if (lpDeviceID != NULL)
    {
        if (!(lpDeviceIDW = ConvertMultiByteToUnicode(lpDeviceID,
                                                      CP_ACP)))
        {
            goto Cleanup;
        }
    }

    Ret = DevicePropertiesExW(hWndParent,
                              lpMachineNameW,
                              lpDeviceIDW,
                              dwFlags,
                              bShowDevMgr);

Cleanup:
    if (lpMachineNameW != NULL)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 lpMachineNameW);
    }
    if (lpDeviceIDW != NULL)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 lpDeviceIDW);
    }

    return Ret;
}


/***************************************************************************
 * NAME                                                         EXPORTED
 *      DevicePropertiesExW
 *
 * DESCRIPTION
 *   Invokes the extended device properties dialog
 *
 * ARGUMENTS
 *   hWndParent:    Handle to the parent window
 *   lpMachineName: Machine Name, NULL is the local machine
 *   lpDeviceID:    Specifies the device whose properties are to be shown, optional if
 *                  bShowDevMgr is nonzero
 *   dwFlags:       This parameter can be a combination of the following flags:
 *                  * DPF_DEVICE_STATUS_ACTION: Only valid if bShowDevMgr, causes
 *                                              the default device status action button
 *                                              to be clicked (Troubleshoot, Enable
 *                                              Device, etc)
 *   bShowDevMgr:   If non-zero it displays the device manager instead of
 *                  the advanced device property dialog
 *
 * RETURN VALUE
 *   1:  if bShowDevMgr is non-zero and no error occured
 *   -1: a call to GetLastError returns 0 if successful
 *
 * @implemented
 */
INT_PTR
WINAPI
DevicePropertiesExW(IN HWND hWndParent  OPTIONAL,
                    IN LPCWSTR lpMachineName  OPTIONAL,
                    IN LPCWSTR lpDeviceID  OPTIONAL,
                    IN DWORD dwFlags  OPTIONAL,
                    IN BOOL bShowDevMgr)
{
    INT_PTR Ret = -1;

    if (dwFlags & ~(DPF_EXTENDED | DPF_DEVICE_STATUS_ACTION))
    {
        FIXME("DevPropertiesExW: Invalid flags: 0x%x\n",
                dwFlags & ~(DPF_EXTENDED | DPF_DEVICE_STATUS_ACTION));
        SetLastError(ERROR_INVALID_FLAGS);
        return -1;
    }

    if (bShowDevMgr)
    {
        FIXME("DevPropertiesExW doesn't support bShowDevMgr!\n");
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    }
    else
    {
        HDEVINFO hDevInfo;
        SP_DEVINFO_DATA DevInfoData;
        HINSTANCE hComCtl32;

        if (lpDeviceID == NULL)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return -1;
        }

        /* dynamically load comctl32 */
        hComCtl32 = LoadAndInitComctl32();
        if (hComCtl32 != NULL)
        {
            hDevInfo = SetupDiCreateDeviceInfoListEx(NULL,
                                                     hWndParent,
                                                     lpMachineName,
                                                     NULL);
            if (hDevInfo != INVALID_HANDLE_VALUE)
            {
                DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
                if (SetupDiOpenDeviceInfo(hDevInfo,
                                          lpDeviceID,
                                          hWndParent,
                                          0,
                                          &DevInfoData))
                {
                    Ret = DisplayDeviceAdvancedProperties(hWndParent,
                                                          lpDeviceID,
                                                          hDevInfo,
                                                          &DevInfoData,
                                                          hComCtl32,
                                                          lpMachineName,
                                                          dwFlags);
                }

                SetupDiDestroyDeviceInfoList(hDevInfo);
            }

            FreeLibrary(hComCtl32);
        }
    }

    return Ret;
}


/***************************************************************************
 * NAME                                                         EXPORTED
 *      DevicePropertiesA
 *
 * DESCRIPTION
 *   Invokes the device properties dialog directly
 *
 * ARGUMENTS
 *   hWndParent:    Handle to the parent window
 *   lpMachineName: Machine Name, NULL is the local machine
 *   lpDeviceID:    Specifies the device whose properties are to be shown
 *   bShowDevMgr:   If non-zero it displays the device manager instead of
 *                  the device property dialog
 *
 * RETURN VALUE
 *   >=0: if no errors occured
 *   -1:  if errors occured
 *
 * REVISIONS
 *
 * @implemented
 */
int
WINAPI
DevicePropertiesA(HWND hWndParent,
                  LPCSTR lpMachineName,
                  LPCSTR lpDeviceID,
                  BOOL bShowDevMgr)
{
    return DevicePropertiesExA(hWndParent,
                               lpMachineName,
                               lpDeviceID,
                               DPF_EXTENDED,
                               bShowDevMgr);
}


/***************************************************************************
 * NAME                                                         EXPORTED
 *      DevicePropertiesW
 *
 * DESCRIPTION
 *   Invokes the device properties dialog directly
 *
 * ARGUMENTS
 *   hWndParent:    Handle to the parent window
 *   lpMachineName: Machine Name, NULL is the local machine
 *   lpDeviceID:    Specifies the device whose properties are to be shown
 *   bShowDevMgr:   If non-zero it displays the device manager instead of
 *                  the device property dialog
 *
 * RETURN VALUE
 *   >=0: if no errors occured
 *   -1:  if errors occured
 *
 * REVISIONS
 *
 * @implemented
 */
int
WINAPI
DevicePropertiesW(HWND hWndParent,
                  LPCWSTR lpMachineName,
                  LPCWSTR lpDeviceID,
                  BOOL bShowDevMgr)
{
    return DevicePropertiesExW(hWndParent,
                               lpMachineName,
                               lpDeviceID,
                               DPF_EXTENDED,
                               bShowDevMgr);
}


/***************************************************************************
 * NAME                                                         EXPORTED
 *      DeviceProperties_RunDLLA
 *
 * DESCRIPTION
 *   Invokes the device properties dialog
 *
 * ARGUMENTS
 *   hWndParent:  Handle to the parent window
 *   hInst:       Handle to the application instance
 *   lpDeviceCmd: A command that includes the DeviceID of the properties to be shown,
 *                also see NOTEs
 *   nCmdShow:    Specifies how the window should be shown
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * NOTE
 *   - lpDeviceCmd is a string in the form of "/MachineName MACHINE /DeviceID DEVICEPATH"
 *     (/MachineName is optional). This function only parses this string and eventually
 *     calls DeviceProperties().
 *
 * @implemented
 */
VOID
WINAPI
DeviceProperties_RunDLLA(HWND hWndParent,
                         HINSTANCE hInst,
                         LPCSTR lpDeviceCmd,
                         int nCmdShow)
{
    LPWSTR lpDeviceCmdW = NULL;

    if (lpDeviceCmd != NULL)
    {
        if ((lpDeviceCmdW = ConvertMultiByteToUnicode(lpDeviceCmd,
                                                      CP_ACP)))
        {
            DeviceProperties_RunDLLW(hWndParent,
                                     hInst,
                                     lpDeviceCmdW,
                                     nCmdShow);
        }
    }

    if (lpDeviceCmdW != NULL)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 lpDeviceCmdW);
    }
}


/***************************************************************************
 * NAME                                                         EXPORTED
 *      DeviceProperties_RunDLLW
 *
 * DESCRIPTION
 *   Invokes the device properties dialog
 *
 * ARGUMENTS
 *   hWndParent:  Handle to the parent window
 *   hInst:       Handle to the application instance
 *   lpDeviceCmd: A command that includes the DeviceID of the properties to be shown,
 *                also see NOTEs
 *   nCmdShow:    Specifies how the window should be shown
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * NOTE
 *   - lpDeviceCmd is a string in the form of "/MachineName MACHINE /DeviceID DEVICEPATH"
 *     (/MachineName is optional). This function only parses this string and eventually
 *     calls DeviceProperties().
 *
 * @implemented
 */
VOID
WINAPI
DeviceProperties_RunDLLW(HWND hWndParent,
                         HINSTANCE hInst,
                         LPCWSTR lpDeviceCmd,
                         int nCmdShow)
{
    WCHAR szDeviceID[MAX_DEVICE_ID_LEN+1];
    WCHAR szMachineName[MAX_COMPUTERNAME_LENGTH+1];
    LPWSTR lpString = (LPWSTR)lpDeviceCmd;

    if (!GetDeviceAndComputerName(lpString,
                                  szDeviceID,
                                  szMachineName))
    {
        ERR("DeviceProperties_RunDLLW DeviceID: %S, MachineName: %S\n", szDeviceID, szMachineName);
        return;
    }

    DevicePropertiesW(hWndParent,
                      szMachineName,
                      szDeviceID,
                      FALSE);
}
