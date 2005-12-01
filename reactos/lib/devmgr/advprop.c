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
/* $Id: hwpage.c 19599 2005-11-26 02:12:58Z weiden $
 *
 * PROJECT:         ReactOS devmgr.dll
 * FILE:            lib/devmgr/advprop.c
 * PURPOSE:         ReactOS Device Manager
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 * UPDATE HISTORY:
 *      04-04-2004  Created
 */
#include <precomp.h>

#define NDEBUG
#include <debug.h>

typedef INT_PTR (WINAPI *PPROPERTYSHEETW)(LPCPROPSHEETHEADERW);
typedef HPROPSHEETPAGE (WINAPI *PCREATEPROPERTYSHEETPAGEW)(LPCPROPSHEETPAGEW);
typedef BOOL (WINAPI *PDESTROYPROPERTYSHEETPAGE)(HPROPSHEETPAGE);

typedef enum
{
    DEA_DISABLE = 0,
    DEA_ENABLE,
    DEA_UNKNOWN
} DEVENABLEACTION;

typedef struct _DEVADVPROP_INFO
{
    HDEVINFO DeviceInfoSet;
    PSP_DEVINFO_DATA DeviceInfoData;
    HINSTANCE hComCtl32;
    HANDLE hMachine;
    BOOL CanDisable;
    BOOL DeviceEnabled;
    WCHAR szDevName[255];
    WCHAR szTemp[255];
    WCHAR szDeviceID[1];
    /* struct may be dynamically expanded here! */
} DEVADVPROP_INFO, *PDEVADVPROP_INFO;


static VOID
InitDevUsageActions(IN HWND hwndDlg,
                    IN HWND hComboBox,
                    IN PDEVADVPROP_INFO dap)
{
    INT Index;
    UINT i;
    struct
    {
        UINT szId;
        DEVENABLEACTION Action;
    } Actions[] =
    {
        {IDS_ENABLEDEVICE, DEA_ENABLE},
        {IDS_DISABLEDEVICE, DEA_DISABLE},
    };

    for (i = 0;
         i != sizeof(Actions) / sizeof(Actions[0]);
         i++)
    {
        /* fill in the device usage combo box */
        if (LoadString(hDllInstance,
                       Actions[i].szId,
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
                            (LPARAM)Actions[i].Action);

                switch (Actions[i].Action)
                {
                    case DEA_ENABLE:
                        if (dap->DeviceEnabled)
                        {
                            SendMessage(hComboBox,
                                        CB_SETCURSEL,
                                        (WPARAM)Index,
                                        0);
                        }
                        break;

                    case DEA_DISABLE:
                        if (!dap->DeviceEnabled)
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


static DEVENABLEACTION
GetSelectedUsageAction(IN HWND hComboBox)
{
    INT Index;
    DEVENABLEACTION Ret = DEA_UNKNOWN;

    Index = (INT)SendMessage(hComboBox,
                             CB_GETCURSEL,
                             0,
                             0);
    if (Index != CB_ERR)
    {
        INT iRet = SendMessage(hComboBox,
                               CB_GETITEMDATA,
                               (WPARAM)Index,
                               0);
        if (iRet != CB_ERR && iRet < (INT)DEA_UNKNOWN)
        {
            Ret = (DEVENABLEACTION)iRet;
        }
    }

    return Ret;
}


static VOID
ApplyGeneralSettings(IN HWND hwndDlg,
                     IN PDEVADVPROP_INFO dap)
{
    DEVENABLEACTION SelectedUsageAction;

    SelectedUsageAction = GetSelectedUsageAction(GetDlgItem(hwndDlg,
                                                            IDC_DEVUSAGE));
    if (SelectedUsageAction != DEA_UNKNOWN)
    {
        switch (SelectedUsageAction)
        {
            case DEA_ENABLE:
                if (!dap->DeviceEnabled)
                {
                    /* FIXME - enable device */
                }
                break;

            case DEA_DISABLE:
                if (dap->DeviceEnabled)
                {
                    /* FIXME - disable device */
                }
                break;

            default:
                break;
        }

        /* disable the apply button */
        PropSheet_UnChanged(GetParent(hwndDlg),
                            hwndDlg);
    }
}


static VOID
UpdateDevInfo(IN HWND hwndDlg,
              IN PDEVADVPROP_INFO dap,
              IN BOOL ReOpenDevice)
{
    HICON hIcon;
    HWND hDevUsage;

    if (ReOpenDevice)
    {
        /* note, we ignore the fact that SetupDiOpenDeviceInfo could fail here,
           in case of failure we're still going to query information from it even
           though those calls are going to fail. But they return readable strings
           even in that case. */
        SetupDiOpenDeviceInfo(dap->DeviceInfoSet,
                              dap->szDeviceID,
                              hwndDlg,
                              0,
                              dap->DeviceInfoData);
    }

    /* get the device name */
    if (GetDeviceDescriptionString(dap->DeviceInfoSet,
                                   dap->DeviceInfoData,
                                   dap->szDevName,
                                   sizeof(dap->szDevName) / sizeof(dap->szDevName[0])))
    {
        PropSheet_SetTitle(GetParent(hwndDlg),
                           PSH_PROPTITLE,
                           dap->szDevName);
    }

    /* set the device image */
    if (SetupDiLoadClassIcon(&dap->DeviceInfoData->ClassGuid,
                             &hIcon,
                             NULL))
    {
        HICON hOldIcon = (HICON)SendDlgItemMessage(hwndDlg,
                                                   IDC_DEVICON,
                                                   STM_SETICON,
                                                   (WPARAM)hIcon,
                                                   0);
        if (hOldIcon != NULL)
        {
            DestroyIcon(hOldIcon);
        }
    }

    /* set the device name edit control text */
    SetDlgItemText(hwndDlg,
                   IDC_DEVNAME,
                   dap->szDevName);

    /* set the device type edit control text */
    if (GetDeviceTypeString(dap->DeviceInfoData,
                            dap->szTemp,
                            sizeof(dap->szTemp) / sizeof(dap->szTemp[0])))
    {
        SetDlgItemText(hwndDlg,
                       IDC_DEVTYPE,
                       dap->szTemp);
    }

    /* set the device manufacturer edit control text */
    if (GetDeviceManufacturerString(dap->DeviceInfoSet,
                                    dap->DeviceInfoData,
                                    dap->szTemp,
                                    sizeof(dap->szTemp) / sizeof(dap->szTemp[0])))
    {
        SetDlgItemText(hwndDlg,
                       IDC_DEVMANUFACTURER,
                       dap->szTemp);
    }

    /* set the device location edit control text */
    if (GetDeviceLocationString(dap->DeviceInfoData->DevInst,
                                dap->szTemp,
                                sizeof(dap->szTemp) / sizeof(dap->szTemp[0])))
    {
        SetDlgItemText(hwndDlg,
                       IDC_DEVLOCATION,
                       dap->szTemp);
    }

    /* set the device status edit control text */
    if (GetDeviceStatusString(dap->DeviceInfoData->DevInst,
                              dap->hMachine,
                              dap->szTemp,
                              sizeof(dap->szTemp) / sizeof(dap->szTemp[0])))
    {
        SetDlgItemText(hwndDlg,
                       IDC_DEVSTATUS,
                       dap->szTemp);
    }

    /* check if the device can be enabled/disabled */
    hDevUsage = GetDlgItem(hwndDlg,
                           IDC_DEVUSAGE);

    if (!CanDisableDevice(dap->DeviceInfoData->DevInst,
                          dap->hMachine,
                          &dap->CanDisable))
    {
        dap->CanDisable = FALSE;
    }

    if (!IsDeviceEnabled(dap->DeviceInfoData->DevInst,
                         dap->hMachine,
                         &dap->DeviceEnabled))
    {
        dap->DeviceEnabled = FALSE;
    }

    /* enable/disable the device usage controls */
    EnableWindow(GetDlgItem(hwndDlg,
                            IDC_DEVUSAGELABEL),
                 dap->CanDisable);
    EnableWindow(hDevUsage,
                 dap->CanDisable);

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

    /* finally, disable the apply button */
    PropSheet_UnChanged(GetParent(hwndDlg),
                        hwndDlg);
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
                    SetWindowLongPtr(hwndDlg,
                                     DWL_USER,
                                     (DWORD_PTR)dap);

                    UpdateDevInfo(hwndDlg,
                                  dap,
                                  FALSE);
                }
                Ret = TRUE;
                break;
            }

            case WM_DEVICECHANGE:
            {
                /* FIXME - don't call UpdateDevInfo in all events */
                UpdateDevInfo(hwndDlg,
                              dap,
                              TRUE);
                break;
            }

            case WM_DESTROY:
            {
                HICON hDevIcon;

                /* destroy the device icon */
                hDevIcon = (HICON)SendDlgItemMessage(hwndDlg,
                                                     IDC_DEVICON,
                                                     STM_GETICON,
                                                     0,
                                                     0);
                if (hDevIcon != NULL)
                {
                    DestroyIcon(hDevIcon);
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
                                IN LPCWSTR lpMachineName)
{
    PROPSHEETHEADER psh = {0};
    PROPSHEETPAGE pspGeneral = {0};
    DWORD nPropSheets = 0;
    PPROPERTYSHEETW pPropertySheetW;
    PCREATEPROPERTYSHEETPAGEW pCreatePropertySheetPageW;
    PDESTROYPROPERTYSHEETPAGE pDestroyPropertySheetPage;
    PDEVADVPROP_INFO DevAdvPropInfo;
    DWORD PropertySheetType;
    HANDLE hMachine = NULL;
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
            DPRINT1("SetupDiGetDeviceInterfaceDetail unexpectedly returned TRUE!\n");
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

    if (lpMachineName != NULL)
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
                               0,
                               FIELD_OFFSET(DEVADVPROP_INFO,
                                            szDeviceID) +
                                   (DevIdSize * sizeof(WCHAR)));
    if (DevAdvPropInfo == NULL)
    {
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

    DevAdvPropInfo->DeviceInfoSet = DeviceInfoSet;
    DevAdvPropInfo->DeviceInfoData = DeviceInfoData;
    DevAdvPropInfo->hComCtl32 = hComCtl32;
    DevAdvPropInfo->hMachine = hMachine;
    DevAdvPropInfo->szDevName[0] = L'\0';

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPTITLE | PSH_NOAPPLYNOW;
    psh.hwndParent = hWndParent;
    psh.pszCaption = DevAdvPropInfo->szDevName;

    PropertySheetType = lpMachineName != NULL ?
                            DIGCDP_FLAG_REMOTE_ADVANCED :
                            DIGCDP_FLAG_ADVANCED;

    /* find out how many property sheets we need */
    if (SetupDiGetClassDevPropertySheets(DeviceInfoSet,
                                         DeviceInfoData,
                                         &psh,
                                         0,
                                         &nPropSheets,
                                         PropertySheetType) &&
        nPropSheets != 0)
    {
        DPRINT1("SetupDiGetClassDevPropertySheets unexpectedly returned TRUE!\n");
        goto Cleanup;
    }

    if (nPropSheets != 0 && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        goto Cleanup;
    }

    psh.phpage = HeapAlloc(GetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           (nPropSheets + 1) * sizeof(HPROPSHEETPAGE));
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
    psh.phpage[0] = pCreatePropertySheetPageW(&pspGeneral);
    if (psh.phpage[0] != NULL)
    {
        psh.nPages++;
    }

    if (nPropSheets != 0)
    {
        /* create the device property sheets */
        if (!SetupDiGetClassDevPropertySheets(DeviceInfoSet,
                                              DeviceInfoData,
                                              &psh,
                                              nPropSheets + psh.nPages,
                                              NULL,
                                              PropertySheetType))
        {
            goto Cleanup;
        }
    }

    /* FIXME - add the "Driver" property sheet if necessary */

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

    HeapFree(GetProcessHeap(),
             0,
             psh.phpage);

    HeapFree(GetProcessHeap(),
             0,
             DevAdvPropInfo);

    if (hMachine != NULL)
    {
        CM_Disconnect_Machine(hMachine);
    }

    return Ret;
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
 *   -1: if errors occured
 *
 * REVISIONS
 *
 * NOTE
 *
 * @implemented
 */
INT_PTR
WINAPI
DeviceAdvancedPropertiesW(HWND hWndParent,
                          LPCWSTR lpMachineName,
                          LPCWSTR lpDeviceID)
{
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA DevInfoData;
    HINSTANCE hComCtl32;
    INT_PTR Ret = -1;

    /* dynamically load comctl32 */
    hComCtl32 = LoadAndInitComctl32();
    if (hComCtl32 != NULL)
    {
        if (lpMachineName != NULL)
        {
            hDevInfo = SetupDiCreateDeviceInfoListEx(NULL,
                                                     hWndParent,
                                                     lpMachineName,
                                                     NULL);
        }
        else
        {
            hDevInfo = SetupDiCreateDeviceInfoList(NULL,
                                                   hWndParent);
        }

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
                                                      lpMachineName);
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
 *   -1: if errors occured
 *
 * REVISIONS
 *
 * NOTE
 *
 * @implemented
 */
INT_PTR
WINAPI
DeviceAdvancedPropertiesA(HWND hWndParent,
                          LPCSTR lpMachineName,
                          LPCSTR lpDeviceID)
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
