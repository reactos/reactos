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

INT_PTR
DisplayDeviceAdvancedProperties(IN HWND hWndParent,
                                IN HDEVINFO DeviceInfoSet,
                                IN PSP_DEVINFO_DATA DeviceInfoData,
                                IN HINSTANCE hComCtl32)
{
    WCHAR szDevName[255];
    DWORD RegDataType;
    PROPSHEETHEADER psh = {0};
    DWORD nPropSheets = 0;
    PPROPERTYSHEETW pPropertySheetW;
    INT_PTR Ret = -1;

    pPropertySheetW = (PPROPERTYSHEETW)GetProcAddress(hComCtl32,
                                                      "PropertySheetW");
    if (pPropertySheetW == NULL)
    {
        return -1;
    }

    /* get the device name */
    if ((SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                          DeviceInfoData,
                                          SPDRP_FRIENDLYNAME,
                                          &RegDataType,
                                          (PBYTE)szDevName,
                                          sizeof(szDevName),
                                          NULL) ||
         SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                          DeviceInfoData,
                                          SPDRP_DEVICEDESC,
                                          &RegDataType,
                                          (PBYTE)szDevName,
                                          sizeof(szDevName),
                                          NULL)) &&
        RegDataType == REG_SZ)
    {
        /* FIXME - check string for NULL termination! */

        psh.dwSize = sizeof(PROPSHEETHEADER);
        psh.dwFlags =  PSH_PROPTITLE;
        psh.hwndParent = hWndParent;
        psh.pszCaption = szDevName;

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

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            goto Cleanup;
        }

        psh.phpage = HeapAlloc(GetProcessHeap(),
                               0,
                               nPropSheets * sizeof(HPROPSHEETPAGE));
        if (psh.phpage == NULL)
        {
            goto Cleanup;
        }

        /* FIXME - add the "General" and "Driver" pages */

        if (!SetupDiGetClassDevPropertySheets(DeviceInfoSet,
                                              DeviceInfoData,
                                              &psh,
                                              nPropSheets,
                                              NULL,
                                              DIGCDP_FLAG_ADVANCED))
        {
            goto Cleanup;
        }

        Ret = pPropertySheetW(&psh);

Cleanup:
        HeapFree(GetProcessHeap(),
                 0,
                 psh.phpage);
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
