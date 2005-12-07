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
            case CM_PROB_DEVLOADER_FAILED:
            {
                /* FIXME - only if it's not a root bus devloader */
                /* FIXME - display the update driver wizard */
                break;
            }

            case CM_PROB_OUT_OF_MEMORY:
            case CM_PROB_ENTRY_IS_WRONG_TYPE:
            case CM_PROB_LACKED_ARBITRATOR:
            case CM_PROB_FAILED_START:
            case CM_PROB_LIAR:
            case CM_PROB_UNKNOWN_RESOURCE:
            {
                /* FIXME - display the update driver wizard */
                break;
            }

            case CM_PROB_BOOT_CONFIG_CONFLICT:
            case CM_PROB_NORMAL_CONFLICT:
            case CM_PROB_REENUMERATION:
            {
                /* FIXME - display the conflict wizard */
                break;
            }

            case CM_PROB_FAILED_FILTER:
            case CM_PROB_REINSTALL:
            case CM_PROB_FAILED_INSTALL:
            {
                /* FIXME - display the driver (re)installation wizard */
                break;
            }

            case CM_PROB_DEVLOADER_NOT_FOUND:
            {
                /* FIXME - 4 cases:
                   1) if it's a missing system devloader:
                      - fail
                   2) if it's not a system devloader but still missing:
                      - display the driver reinstallation wizard
                   3) if it's not a system devloader but the file can be found:
                      - display the update driver wizard
                   4) if it's a missing or empty software key
                      - display the update driver wizard
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
                /* FIXME - do nothing */
                break;

            case CM_PROB_NOT_VERIFIED:
            case CM_PROB_DEVICE_NOT_THERE:
                /* FIXME - display search hardware wizard */
                break;

            case CM_PROB_NEED_RESTART:
            case CM_PROB_WILL_BE_REMOVED:
            case CM_PROB_MOVED:
            case CM_PROB_TOO_EARLY:
            case CM_PROB_DISABLED_SERVICE:
                /* FIXME - reboot computer */
                break;

            case CM_PROB_REGISTRY:
                /* FIXME - check registry */
                break;

            case CM_PROB_DISABLED:
            {
                /* FIXME - if device was disabled by user display the "Enable Device" wizard,
                           otherwise Troubleshoot because the device was disabled by the system */
                break;
            }

            case CM_PROB_DEVLOADER_NOT_READY:
            {
                /* FIXME - if it's a graphics adapter:
                           - if it's a a secondary adapter and the main adapter
                             couldn't be found
                             - do nothing or default action
                           - else
                             - display the Properties
                         - else
                           - Update driver
                 */
                break;
            }

            case CM_PROB_FAILED_ADD:
            {
                /* FIXME - display the properties of the sub-device */
                break;
            }

            case CM_PROB_NO_SOFTCONFIG:
            case CM_PROB_IRQ_TRANSLATION_FAILED:
            case CM_PROB_FAILED_DRIVER_ENTRY:
            case CM_PROB_DRIVER_FAILED_PRIOR_UNLOAD:
            case CM_PROB_DRIVER_FAILED_LOAD:
            case CM_PROB_DRIVER_SERVICE_KEY_INVALID:
            case CM_PROB_LEGACY_SERVICE_NO_DEVICES:
            case CM_PROB_DUPLICATE_DEVICE:
            case CM_PROB_FAILED_POST_START:
            case CM_PROB_HALTED:
            case CM_PROB_HELD_FOR_EJECT:
            case CM_PROB_DRIVER_BLOCKED:
            case CM_PROB_REGISTRY_TOO_LARGE:
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
