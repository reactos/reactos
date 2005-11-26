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

typedef struct _DEVADVPROP_INFO
{
    HDEVINFO DeviceInfoSet;
    PSP_DEVINFO_DATA DeviceInfoData;
    HINSTANCE hComCtl32;
    WCHAR szDevName[255];
    WCHAR szTemp[255];
} DEVADVPROP_INFO, *PDEVADVPROP_INFO;


static INT_PTR
CALLBACK
AdvPropGeneralDlgProc(IN HWND hwndDlg,
                      IN UINT uMsg,
                      IN WPARAM wParam,
                      IN LPARAM lParam)
{
    PDEVADVPROP_INFO dap;

    dap = (PDEVADVPROP_INFO)GetWindowLongPtr(hwndDlg,
                                             DWL_USER);

    if (dap != NULL || uMsg == WM_INITDIALOG)
    {
        switch (uMsg)
        {
            case WM_INITDIALOG:
            {
                dap = (PDEVADVPROP_INFO)((LPPROPSHEETPAGE)lParam)->lParam;
                if (dap != NULL)
                {
                    HICON hIcon;

                    SetWindowLongPtr(hwndDlg,
                                     DWL_USER,
                                     (DWORD_PTR)dap);

                    /* set the device image */
                    if (SetupDiLoadClassIcon(&dap->DeviceInfoData->ClassGuid,
                                             &hIcon,
                                             NULL))
                    {
                        SendDlgItemMessage(hwndDlg,
                                           IDC_DEVICON,
                                           STM_SETICON,
                                           (WPARAM)hIcon,
                                           0);
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
                    if (GetDeviceStatusString(dap->DeviceInfoSet,
                                              dap->DeviceInfoData,
                                              dap->szTemp,
                                              sizeof(dap->szTemp) / sizeof(dap->szTemp[0])))
                    {
                        SetDlgItemText(hwndDlg,
                                       IDC_DEVSTATUS,
                                       dap->szTemp);
                    }
                }
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

    return FALSE;
}


INT_PTR
DisplayDeviceAdvancedProperties(IN HWND hWndParent,
                                IN HDEVINFO DeviceInfoSet,
                                IN PSP_DEVINFO_DATA DeviceInfoData,
                                IN HINSTANCE hComCtl32)
{
    DWORD RegDataType;
    PROPSHEETHEADER psh = {0};
    PROPSHEETPAGE pspGeneral = {0};
    DWORD nPropSheets = 0;
    DWORD nDevSheetsStart = 0;
    PPROPERTYSHEETW pPropertySheetW;
    PCREATEPROPERTYSHEETPAGEW pCreatePropertySheetPageW;
    PDESTROYPROPERTYSHEETPAGE pDestroyPropertySheetPage;
    PDEVADVPROP_INFO DevAdvPropInfo;
    UINT nPages = 0;
    union
    {
        ULONG Mask;
        struct
        {
            ULONG General : 1;
            ULONG Device : 1;
        } Page;
    } DelPropSheets = {0};
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

    /* create the internal structure associated with the "General",
       "Driver", ... pages */
    DevAdvPropInfo = HeapAlloc(GetProcessHeap(),
                               0,
                               sizeof(DEVADVPROP_INFO));
    if (DevAdvPropInfo == NULL)
    {
        return -1;
    }

    DevAdvPropInfo->DeviceInfoSet = DeviceInfoSet;
    DevAdvPropInfo->DeviceInfoData = DeviceInfoData;
    DevAdvPropInfo->hComCtl32 = hComCtl32;
    DevAdvPropInfo->szDevName[0] = L'\0';

    /* get the device name */
    if ((SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                          DeviceInfoData,
                                          SPDRP_FRIENDLYNAME,
                                          &RegDataType,
                                          (PBYTE)DevAdvPropInfo->szDevName,
                                          sizeof(DevAdvPropInfo->szDevName),
                                          NULL) ||
         SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                          DeviceInfoData,
                                          SPDRP_DEVICEDESC,
                                          &RegDataType,
                                          (PBYTE)DevAdvPropInfo->szDevName,
                                          sizeof(DevAdvPropInfo->szDevName),
                                          NULL)) &&
        RegDataType == REG_SZ)
    {
        /* FIXME - check string for NULL termination! */

        psh.dwSize = sizeof(PROPSHEETHEADER);
        psh.dwFlags =  PSH_PROPTITLE;
        psh.hwndParent = hWndParent;
        psh.pszCaption = DevAdvPropInfo->szDevName;

        /* find out how many property sheets we need */
        if (SetupDiGetClassDevPropertySheets(DeviceInfoSet,
                                             DeviceInfoData,
                                             &psh,
                                             0,
                                             &nPropSheets,
                                             DIGCDP_FLAG_ADVANCED) &&
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
            DelPropSheets.Page.General = TRUE;
            nDevSheetsStart++;
            nPages++;
        }

        if (nPropSheets != 0)
        {
            /* create the device property sheets but don't overwrite
               the "General" property sheet handle */
            psh.phpage += nDevSheetsStart;
            if (!SetupDiGetClassDevPropertySheets(DeviceInfoSet,
                                                  DeviceInfoData,
                                                  &psh,
                                                  nPropSheets,
                                                  NULL,
                                                  DIGCDP_FLAG_ADVANCED))
            {
                goto Cleanup;
            }
            psh.phpage -= nDevSheetsStart;

            DelPropSheets.Page.Device = TRUE;
            nPages += nPropSheets;
        }

        psh.nPages = nPages;

        /* FIXME - add the "Driver" property sheet if necessary */

        Ret = pPropertySheetW(&psh);

        /* no need to destroy the property sheets anymore */
        DelPropSheets.Mask = 0;

Cleanup:
        /* in case of failure the property sheets must be destroyed */
        if (DelPropSheets.Mask != 0)
        {
            if (DelPropSheets.Page.General && psh.phpage[0] != NULL)
            {
                pDestroyPropertySheetPage(psh.phpage[0]);
            }

            if (DelPropSheets.Page.Device)
            {
                UINT i;
                for (i = 0;
                     i < nPropSheets;
                     i++)
                {
                    if (psh.phpage[i + 1] != NULL)
                    {
                        pDestroyPropertySheetPage(psh.phpage[i + 1]);
                    }
                }
            }
        }

        HeapFree(GetProcessHeap(),
                 0,
                 psh.phpage);
    }

    HeapFree(GetProcessHeap(),
             0,
             DevAdvPropInfo);

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

    /* FIXME - handle remote device properties */

    UNREFERENCED_PARAMETER(lpMachineName);

    /* dynamically load comctl32 */
    hComCtl32 = LoadAndInitComctl32();
    if (hComCtl32 != NULL)
    {
        hDevInfo = SetupDiCreateDeviceInfoList(NULL,
                                               hWndParent);
        if (hDevInfo != INVALID_HANDLE_VALUE)
        {
            DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
            if (SetupDiOpenDeviceInfo(hDevInfo,
                                      lpDeviceID,
                                      hWndParent,
                                      0,
                                      &DevInfoData))
            {

                /* create the image list */
                Ret = DisplayDeviceAdvancedProperties(hWndParent,
                                                      hDevInfo,
                                                      &DevInfoData,
                                                      hComCtl32);
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
    HeapFree(GetProcessHeap(),
             0,
             lpMachineNameW);
    HeapFree(GetProcessHeap(),
             0,
             lpDeviceIDW);

    return Ret;
}
