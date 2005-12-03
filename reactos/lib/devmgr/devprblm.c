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
 * FILE:            lib/devmgr/devprblm.c
 * PURPOSE:         ReactOS Device Manager
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 * UPDATE HISTORY:
 *      04-04-2004  Created
 */
#include <precomp.h>

#define NDEBUG
#include <debug.h>


BOOL
ShowDeviceProblemWizard(IN HWND hWndParent  OPTIONAL,
                        IN HDEVINFO hDevInfo,
                        IN PSP_DEVINFO_DATA DevInfoData,
                        IN HMACHINE hMachine  OPTIONAL)
{
    CONFIGRET cr;
    ULONG Status, ProblemNumber;
    BOOL Ret = FALSE;

    cr = CM_Get_DevNode_Status_Ex(&Status,
                                  &ProblemNumber,
                                  DevInfoData->DevInst,
                                  0,
                                  hMachine);
    if (cr == CR_SUCCESS && (Status & DN_HAS_PROBLEM))
    {
        switch (ProblemNumber)
        {
            case CM_PROB_DISABLED:
            {
                /* FIXME - display the "Enable Device" wizard */
                break;
            }

            default:
            {
                /* FIXME - troubleshoot the device */
                break;
            }
        }
    }

    return Ret;
}


/***************************************************************************
 * NAME                                                         EXPORTED
 *      DeviceProblemWizardA
 *
 * DESCRIPTION
 *   Calls the device problem wizard
 *
 * ARGUMENTS
 *   hWndParent:    Handle to the parent window
 *   lpMachineName: Machine Name, NULL is the local machine
 *   lpDeviceID:    Specifies the device, also see NOTEs
 *
 * RETURN VALUE
 *   TRUE:  if no errors occured
 *   FALSE: if errors occured
 *
 * @implemented
 */
BOOL
WINAPI
DeviceProblemWizardA(IN HWND hWndParent  OPTIONAL,
                     IN LPCSTR lpMachineName  OPTIONAL,
                     IN LPCSTR lpDeviceID)
{
    LPWSTR lpMachineNameW = NULL;
    LPWSTR lpDeviceIDW = NULL;
    BOOL Ret = FALSE;

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

    Ret = DeviceProblemWizardW(hWndParent,
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
 *      DeviceProblemWizardW
 *
 * DESCRIPTION
 *   Calls the device problem wizard
 *
 * ARGUMENTS
 *   hWndParent:    Handle to the parent window
 *   lpMachineName: Machine Name, NULL is the local machine
 *   lpDeviceID:    Specifies the device, also see NOTEs
 *
 * RETURN VALUE
 *   TRUE:  if no errors occured
 *   FALSE: if errors occured
 *
 * @unimplemented
 */
BOOL
WINAPI
DeviceProblemWizardW(IN HWND hWndParent  OPTIONAL,
                     IN LPCWSTR lpMachineName  OPTIONAL,
                     IN LPCWSTR lpDeviceID)
{
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA DevInfoData;
    HINSTANCE hComCtl32;
    CONFIGRET cr;
    HMACHINE hMachine;
    BOOL Ret = FALSE;

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
            cr = CM_Connect_Machine(lpMachineName,
                                    &hMachine);
            if (cr == CR_SUCCESS)
            {
                DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
                if (SetupDiOpenDeviceInfo(hDevInfo,
                                          lpDeviceID,
                                          hWndParent,
                                          0,
                                          &DevInfoData))
                {
                    Ret = ShowDeviceProblemWizard(hWndParent,
                                                  hDevInfo,
                                                  &DevInfoData,
                                                  hMachine);
                }

                CM_Disconnect_Machine(hMachine);
            }

            SetupDiDestroyDeviceInfoList(hDevInfo);
        }

        FreeLibrary(hComCtl32);
    }

    return Ret;
}
