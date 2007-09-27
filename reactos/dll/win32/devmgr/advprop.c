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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include <precomp.h>

#define NDEBUG
#include <debug.h>

typedef INT_PTR (WINAPI *PPROPERTYSHEETW)(LPCPROPSHEETHEADERW);
typedef HPROPSHEETPAGE (WINAPI *PCREATEPROPERTYSHEETPAGEW)(LPCPROPSHEETPAGEW);
typedef BOOL (WINAPI *PDESTROYPROPERTYSHEETPAGE)(HPROPSHEETPAGE);

typedef struct _DEVADVPROP_INFO
{
    HWND hWndGeneralPage;
    HWND hWndParent;
    WNDPROC ParentOldWndProc;
    HICON hDevIcon;

    HDEVINFO DeviceInfoSet;
    SP_DEVINFO_DATA DeviceInfoData;
    HDEVINFO CurrentDeviceInfoSet;
    SP_DEVINFO_DATA CurrentDeviceInfoData;
    DEVINST ParentDevInst;
    HMACHINE hMachine;
    LPCWSTR lpMachineName;

    HINSTANCE hComCtl32;
    PCREATEPROPERTYSHEETPAGEW pCreatePropertySheetPageW;
    PDESTROYPROPERTYSHEETPAGE pDestroyPropertySheetPage;

    DWORD PropertySheetType;
    DWORD nDevPropSheets;
    HPROPSHEETPAGE *DevPropSheets;

    union
    {
        UINT Flags;
        struct
        {
            UINT FreeDevPropSheets : 1;
            UINT CanDisable : 1;
            UINT DeviceStarted : 1;
            UINT DeviceUsageChanged : 1;
            UINT CloseDevInst : 1;
            UINT IsAdmin : 1;
            UINT DoDefaultDevAction : 1;
            UINT PageInitialized : 1;
            UINT ShowRemotePages : 1;
            UINT HasDriverPage : 1;
            UINT HasResourcePage : 1;
            UINT HasPowerPage : 1;
        };
    };

    WCHAR szDevName[255];
    WCHAR szTemp[255];
    WCHAR szDeviceID[1];
    /* struct may be dynamically expanded here! */
} DEVADVPROP_INFO, *PDEVADVPROP_INFO;


typedef struct _ENUMDRIVERFILES_CONTEXT
{
    HWND hDriversListView;
    UINT nCount;
} ENUMDRIVERFILES_CONTEXT, *PENUMDRIVERFILES_CONTEXT;

#define PM_INITIALIZE (WM_APP + 0x101)


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
                }
            }

            SetupCloseFileQueue(queueHandle);
        }
    }
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
                }

                Ret = TRUE;
                break;
            }
        }
    }

    return Ret;
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
                    {
                        DialogBoxParam(hDllInstance,
                                       MAKEINTRESOURCE(IDD_DRIVERDETAILS),
                                       hwndDlg,
                                       DriverDetailsDlgProc,
                                       (ULONG_PTR)dap);
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
            DPRINT1("Failed to enable/disable device! LastError: %d\n",
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
    if (GetDeviceLocationString(DeviceInfoData->DevInst,
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
        dap->nDevPropSheets += nDriverPages;
    }
    else
    {
        nDriverPages = 0;
    }

    /* include the driver page */
    if (dap->HasDriverPage)
        dap->nDevPropSheets++;

    /* add the device property sheets */
    if (dap->nDevPropSheets != 0)
    {
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
                         iPage != nDriverPages;
                         iPage++)
                    {
                        if (PropSheet_AddPage(hPropSheetDlg,
                                              dap->DevPropSheets[iPage]))
                        {
                            RecalcPages = TRUE;
                        }
                    }

                    dap->FreeDevPropSheets = TRUE;
                }
                else
                {
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
            DPRINT1("SetupDiGetDeviceInstanceId unexpectedly returned TRUE!\n");
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
            if(!wcsnicmp(lpString, L"DeviceID", 8))
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
            else if (!wcsnicmp(lpString, L"MachineName", 11))
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

    if (dwFlags & ~(DPF_UNKNOWN | DPF_DEVICE_STATUS_ACTION))
    {
        DPRINT1("DevPropertiesExW: Invalid flags: 0x%x\n",
                dwFlags & ~(DPF_UNKNOWN | DPF_DEVICE_STATUS_ACTION));
        SetLastError(ERROR_INVALID_FLAGS);
        return -1;
    }

    if (bShowDevMgr)
    {
        DPRINT("DevPropertiesExW doesn't support bShowDevMgr!\n");
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
                               0,
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
                               0,
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
        DPRINT1("DeviceProperties_RunDLLW DeviceID: %S, MachineName: %S\n", szDeviceID, szMachineName);
        return;
    }

    DevicePropertiesW(hWndParent,
                      szMachineName,
                      szDeviceID,
                      FALSE);
}
