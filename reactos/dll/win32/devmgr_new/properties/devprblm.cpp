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
 * PROJECT:         ReactOS devmgr.dll
 * FILE:            lib/devmgr/devprblm.c
 * PURPOSE:         ReactOS Device Manager
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 * UPDATE HISTORY:
 *      04-04-2004  Created
 */

#include "precomp.h"
#include <devmgr/devmgr.h>
#include "properties.h"
#include "resource.h"


BOOL
ShowDeviceProblemWizard(IN HWND hWndParent  OPTIONAL,
                        IN HDEVINFO hDevInfo,
                        IN PSP_DEVINFO_DATA DevInfoData,
                        IN HMACHINE hMachine  OPTIONAL)
{
    WCHAR szDeviceInstanceId[256];
    CONFIGRET cr;
    ULONG Status, ProblemNumber;
    DWORD dwReboot;
    BOOL Ret = FALSE;

    /* Get the device instance id */
    if (!SetupDiGetDeviceInstanceId(hDevInfo,
                                    DevInfoData,
                                    szDeviceInstanceId,
                                    256,
                                    NULL))
        return FALSE;

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
                InstallDevInst(hWndParent, szDeviceInstanceId, TRUE, &dwReboot);
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
                InstallDevInst(hWndParent, szDeviceInstanceId, TRUE, &dwReboot);
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


static const UINT ProblemStringId[NUM_CM_PROB] =
{
    IDS_DEV_NO_PROBLEM,
    IDS_DEV_DEVLOADER_FAILED,
    IDS_DEV_NOT_CONFIGURED,
    IDS_DEV_OUT_OF_MEMORY,
    IDS_DEV_ENTRY_IS_WRONG_TYPE,
    IDS_DEV_LACKED_ARBITRATOR,
    IDS_DEV_BOOT_CONFIG_CONFLICT,
    IDS_DEV_FAILED_FILTER,
    IDS_DEV_DEVLOADER_NOT_FOUND,
    IDS_DEV_INVALID_DATA,
    IDS_DEV_FAILED_START,
    IDS_DEV_LIAR,
    IDS_DEV_NORMAL_CONFLICT,
    IDS_DEV_NOT_VERIFIED,
    IDS_DEV_NEED_RESTART,
    IDS_DEV_REENUMERATION,
    IDS_DEV_PARTIAL_LOG_CONF,
    IDS_DEV_UNKNOWN_RESOURCE,
    IDS_DEV_REINSTALL,
    IDS_DEV_REGISTRY,
    IDS_UNKNOWN, /* CM_PROB_VXDLDR, not used on NT */
    IDS_DEV_WILL_BE_REMOVED,
    IDS_DEV_DISABLED,
    IDS_DEV_DEVLOADER_NOT_READY,
    IDS_DEV_DEVICE_NOT_THERE,
    IDS_DEV_MOVED,
    IDS_DEV_TOO_EARLY,
    IDS_DEV_NO_VALID_LOG_CONF,
    IDS_DEV_FAILED_INSTALL,
    IDS_DEV_HARDWARE_DISABLED,
    IDS_DEV_CANT_SHARE_IRQ,
    IDS_DEV_FAILED_ADD,
    IDS_DEV_DISABLED_SERVICE,
    IDS_DEV_TRANSLATION_FAILED,
    IDS_DEV_NO_SOFTCONFIG,
    IDS_DEV_BIOS_TABLE,
    IDS_DEV_IRQ_TRANSLATION_FAILED,
    IDS_DEV_FAILED_DRIVER_ENTRY,
    IDS_DEV_DRIVER_FAILED_PRIOR_UNLOAD,
    IDS_DEV_DRIVER_FAILED_LOAD,
    IDS_DEV_DRIVER_SERVICE_KEY_INVALID,
    IDS_DEV_LEGACY_SERVICE_NO_DEVICES,
    IDS_DEV_DUPLICATE_DEVICE,
    IDS_DEV_FAILED_POST_START,
    IDS_DEV_HALTED,
    IDS_DEV_PHANTOM,
    IDS_DEV_SYSTEM_SHUTDOWN,
    IDS_DEV_HELD_FOR_EJECT,
    IDS_DEV_DRIVER_BLOCKED,
    IDS_DEV_REGISTRY_TOO_LARGE,
    IDS_DEV_SETPROPERTIES_FAILED
};


/***************************************************************************
 * NAME                                                         EXPORTED
 *      DeviceProblemTextA
 *
 * DESCRIPTION
 *   Gets the problem text from a problem number displayed in the properties dialog
 *
 * ARGUMENTS
 *   hMachine:   Machine handle or NULL for the local machine
 *   DevInst:    Device instance handle
 *   uProblemId: Specifies the problem ID
 *   lpString:   Pointer to a buffer where the string is to be copied to. If the buffer
 *               is too small, the return value is the required string length in characters,
 *               excluding the NULL-termination.
 *   uMaxString: Size of the buffer in characters
 *
 * RETURN VALUE
 *   The return value is the length of the string in characters.
 *   It returns 0 if an error occured.
 *
 * @implemented
 */
UINT
WINAPI
DeviceProblemTextA(IN HMACHINE hMachine  OPTIONAL,
                   IN DEVINST dnDevInst,
                   IN ULONG uProblemId,
                   OUT LPSTR lpString,
                   IN UINT uMaxString)
{
    LPWSTR lpBuffer = NULL;
    UINT Ret = 0;

    if (uMaxString != 0)
    {
        lpBuffer = (LPWSTR)HeapAlloc(GetProcessHeap(),
                                     0,
                                     (uMaxString + 1) * sizeof(WCHAR));
        if (lpBuffer == NULL)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return 0;
        }
    }

    Ret = DeviceProblemTextW(hMachine,
                             dnDevInst,
                             uProblemId,
                             lpBuffer,
                             uMaxString);

    if (lpBuffer != NULL)
    {
        if (Ret)
        {
            WideCharToMultiByte(CP_ACP,
                                0,
                                lpBuffer,
                                (int)Ret,
                                lpString,
                                (int)uMaxString,
                                NULL,
                                NULL);
        }

        HeapFree(GetProcessHeap(),
                 0,
                 lpBuffer);
    }

    return Ret;
}


/***************************************************************************
 * NAME                                                         EXPORTED
 *      DeviceProblemTextW
 *
 * DESCRIPTION
 *   Gets the problem text from a problem number displayed in the properties dialog
 *
 * ARGUMENTS
 *   hMachine:   Machine handle or NULL for the local machine
 *   DevInst:    Device instance handle
 *   uProblemId: Specifies the problem ID
 *   lpString:   Pointer to a buffer where the string is to be copied to. If the buffer
 *               is too small, the return value is the required string length in characters,
 *               excluding the NULL-termination.
 *   uMaxString: Size of the buffer in characters
 *
 * RETURN VALUE
 *   The return value is the length of the string in characters.
 *   It returns 0 if an error occured.
 *
 * @implemented
 */
UINT
WINAPI
DeviceProblemTextW(IN HMACHINE hMachine  OPTIONAL,
                   IN DEVINST dnDevInst,
                   IN ULONG uProblemId,
                   OUT LPWSTR lpString,
                   IN UINT uMaxString)
{
    UINT MessageId = IDS_UNKNOWN;
    UINT Ret = 0;

    if (uProblemId < sizeof(ProblemStringId) / sizeof(ProblemStringId[0]))
        MessageId = ProblemStringId[uProblemId];

    if (uProblemId == 0)
    {
        if (uMaxString != 0)
        {
            Ret = LoadString(hDllInstance,
                             MessageId,
                             lpString,
                             (int)uMaxString);
        }
        else
        {
            Ret = (UINT)LengthOfStrResource(hDllInstance,
                                            MessageId);
        }
    }
    else
    {
        LPWSTR szProblem, szInfo = L"FIXME";
        DWORD dwRet;
        BOOL AdvFormat = FALSE;
        UINT StringIDs[] =
        {
            MessageId,
            IDS_DEVCODE,
        };

        switch (uProblemId)
        {
            case CM_PROB_DEVLOADER_FAILED:
            {
                /* FIXME - if not a root bus devloader then use IDS_DEV_DEVLOADER_FAILED2 */
                /* FIXME - get the type string (ie. ISAPNP, PCI or BIOS for root bus devloaders,
                           or FLOP, ESDI, SCSI, etc for others */
                AdvFormat = TRUE;
                break;
            }

            case CM_PROB_DEVLOADER_NOT_FOUND:
            {
                /* FIXME - 4 cases:
                   1) if it's a missing system devloader:
                      - get the system devloader name
                   2) if it's not a system devloader but still missing:
                      - get the devloader name (file name?)
                   3) if it's not a system devloader but the file can be found:
                      - use IDS_DEV_DEVLOADER_NOT_FOUND2
                   4) if it's a missing or empty software key
                      - use IDS_DEV_DEVLOADER_NOT_FOUND3
                      - AdvFormat = FALSE!
                 */
                AdvFormat = TRUE;
                break;
            }

            case CM_PROB_INVALID_DATA:
                /* FIXME - if the device isn't enumerated by the BIOS/ACPI use IDS_DEV_INVALID_DATA2 */
                AdvFormat = FALSE;
                break;

            case CM_PROB_NORMAL_CONFLICT:
                /* FIXME - get resource type (IRQ, DMA, Memory or I/O) */
                AdvFormat = TRUE;
                break;

            case CM_PROB_UNKNOWN_RESOURCE:
                /* FIXME - get the .inf file name */
                AdvFormat = TRUE;
                break;

            case CM_PROB_DISABLED:
                /* FIXME - if the device was disabled by the system use IDS_DEV_DISABLED2 */
                break;

            case CM_PROB_FAILED_ADD:
                /* FIXME - get the name of the sub-device with the error */
                AdvFormat = TRUE;
                break;
        }

        if (AdvFormat)
        {
            StringIDs[1] = IDS_DEVCODE2;
            dwRet = LoadAndFormatStringsCat(hDllInstance,
                                            StringIDs,
                                            sizeof(StringIDs) / sizeof(StringIDs[0]),
                                            &szProblem,
                                            szInfo,
                                            uProblemId);
        }
        else
        {
            dwRet = LoadAndFormatStringsCat(hDllInstance,
                                            StringIDs,
                                            sizeof(StringIDs) / sizeof(StringIDs[0]),
                                            &szProblem,
                                            uProblemId);
        }

        if (dwRet != 0)
        {
            if (uMaxString != 0 && uMaxString >= dwRet)
            {
                wcscpy(lpString,
                       szProblem);
            }

            LocalFree((HLOCAL)szProblem);

            Ret = dwRet;
        }
    }

    return Ret;
}
