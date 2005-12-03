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
/* $Id: devmgr.c 12852 2005-01-06 13:58:04Z mf $
 *
 * PROJECT:         ReactOS devmgr.dll
 * FILE:            lib/devmgr/misc.c
 * PURPOSE:         ReactOS Device Manager
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 * UPDATE HISTORY:
 *      2005/11/24  Created
 */
#include <precomp.h>

HINSTANCE hDllInstance = NULL;


static INT
LengthOfStrResource(IN HINSTANCE hInst,
                    IN UINT uID)
{
    HRSRC hrSrc;
    HGLOBAL hRes;
    LPWSTR lpName, lpStr;

    if (hInst == NULL)
    {
        return -1;
    }

    /* There are always blocks of 16 strings */
    lpName = (LPWSTR)MAKEINTRESOURCE((uID >> 4) + 1);

    /* Find the string table block */
    if ((hrSrc = FindResourceW(hInst, lpName, (LPWSTR)RT_STRING)) &&
        (hRes = LoadResource(hInst, hrSrc)) &&
        (lpStr = LockResource(hRes)))
    {
        UINT x;

        /* Find the string we're looking for */
        uID &= 0xF; /* position in the block, same as % 16 */
        for (x = 0; x < uID; x++)
        {
            lpStr += (*lpStr) + 1;
        }

        /* Found the string */
        return (int)(*lpStr);
    }
    return -1;
}


static INT
AllocAndLoadString(OUT LPWSTR *lpTarget,
                   IN HINSTANCE hInst,
                   IN UINT uID)
{
    INT ln;

    ln = LengthOfStrResource(hInst,
                             uID);
    if (ln++ > 0)
    {
        (*lpTarget) = (LPWSTR)LocalAlloc(LMEM_FIXED,
                                         ln * sizeof(WCHAR));
        if ((*lpTarget) != NULL)
        {
            INT Ret;
            if (!(Ret = LoadStringW(hInst, uID, *lpTarget, ln)))
            {
                LocalFree((HLOCAL)(*lpTarget));
            }
            return Ret;
        }
    }
    return 0;
}


static INT
AllocAndLoadStringsCat(OUT LPWSTR *lpTarget,
                       IN HINSTANCE hInst,
                       IN UINT *uID,
                       IN UINT nIDs)
{
    INT ln = 0;
    UINT i;

    for (i = 0;
         i != nIDs;
         i++)
    {
        ln += LengthOfStrResource(hInst,
                                  uID[i]);
    }

    if (ln != 0)
    {
        (*lpTarget) = (LPWSTR)LocalAlloc(LMEM_FIXED,
                                         (ln + 1) * sizeof(WCHAR));
        if ((*lpTarget) != NULL)
        {
            LPWSTR s = *lpTarget;
            INT Ret = 0;

            for (i = 0;
                 i != nIDs;
                 i++)
            {
                if (!(Ret = LoadStringW(hInst, uID[i], s, ln)))
                {
                    LocalFree((HLOCAL)(*lpTarget));
                }

                s += Ret;
            }

            return s - *lpTarget;
        }
    }
    return 0;
}


DWORD
LoadAndFormatString(IN HINSTANCE hInstance,
                    IN UINT uID,
                    OUT LPWSTR *lpTarget,
                    ...)
{
    DWORD Ret = 0;
    LPWSTR lpFormat;
    va_list lArgs;

    if (AllocAndLoadString(&lpFormat,
                           hInstance,
                           uID) != 0)
    {
        va_start(lArgs, lpTarget);
        /* let's use FormatMessage to format it because it has the ability to allocate
           memory automatically */
        Ret = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                             lpFormat,
                             0,
                             0,
                             (LPWSTR)lpTarget,
                             0,
                             &lArgs);
        va_end(lArgs);

        LocalFree((HLOCAL)lpFormat);
    }

    return Ret;
}


DWORD
LoadAndFormatStringsCat(IN HINSTANCE hInstance,
                        IN UINT *uID,
                        IN UINT nIDs,
                        OUT LPWSTR *lpTarget,
                        ...)
{
    DWORD Ret = 0;
    LPWSTR lpFormat;
    va_list lArgs;

    if (AllocAndLoadStringsCat(&lpFormat,
                               hInstance,
                               uID,
                               nIDs) != 0)
    {
        va_start(lArgs, lpTarget);
        /* let's use FormatMessage to format it because it has the ability to allocate
           memory automatically */
        Ret = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                             lpFormat,
                             0,
                             0,
                             (LPWSTR)lpTarget,
                             0,
                             &lArgs);
        va_end(lArgs);

        LocalFree((HLOCAL)lpFormat);
    }

    return Ret;
}


LPARAM
ListViewGetSelectedItemData(IN HWND hwnd)
{
    int Index;

    Index = ListView_GetNextItem(hwnd,
                                 -1,
                                 LVNI_SELECTED);
    if (Index != -1)
    {
        LVITEM li;

        li.mask = LVIF_PARAM;
        li.iItem = Index;
        li.iSubItem = 0;

        if (ListView_GetItem(hwnd,
                             &li))
        {
            return li.lParam;
        }
    }

    return 0;
}


LPWSTR
ConvertMultiByteToUnicode(IN LPCSTR lpMultiByteStr,
                          IN UINT uCodePage)
{
    LPWSTR lpUnicodeStr;
    INT nLength;

    nLength = MultiByteToWideChar(uCodePage,
                                  0,
                                  lpMultiByteStr,
                                  -1,
                                  NULL,
                                  0);
    if (nLength == 0)
        return NULL;

    lpUnicodeStr = HeapAlloc(GetProcessHeap(),
                             0,
                             nLength * sizeof(WCHAR));
    if (lpUnicodeStr == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    if (!MultiByteToWideChar(uCodePage,
                             0,
                             lpMultiByteStr,
                             nLength,
                             lpUnicodeStr,
                             nLength))
    {
        HeapFree(GetProcessHeap(),
                 0,
                 lpUnicodeStr);
        return NULL;
    }

    return lpUnicodeStr;
}


BOOL
GetDeviceManufacturerString(IN HDEVINFO DeviceInfoSet,
                            IN PSP_DEVINFO_DATA DeviceInfoData,
                            OUT LPWSTR szBuffer,
                            IN DWORD BufferSize)
{
    DWORD RegDataType;
    BOOL Ret = FALSE;

    if (!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                          DeviceInfoData,
                                          SPDRP_MFG,
                                          &RegDataType,
                                          (PBYTE)szBuffer,
                                          BufferSize * sizeof(WCHAR),
                                          NULL) ||
        RegDataType != REG_SZ)
    {
        szBuffer[0] = L'\0';
        if (LoadString(hDllInstance,
                       IDS_UNKNOWN,
                       szBuffer,
                       BufferSize))
        {
            Ret = TRUE;
        }
    }
    else
    {
        /* FIXME - check string for NULL termination! */
        Ret = TRUE;
    }

    return Ret;
}


BOOL
GetDeviceLocationString(IN DEVINST dnDevInst  OPTIONAL,
                        IN DEVINST dnParentDevInst  OPTIONAL,
                        OUT LPWSTR szBuffer,
                        IN DWORD BufferSize)
{
    DWORD RegDataType;
    ULONG DataSize;
    CONFIGRET cRet;
    LPWSTR szFormatted;
    BOOL Ret = FALSE;

    DataSize = BufferSize * sizeof(WCHAR);
    szBuffer[0] = L'\0';
    if (dnParentDevInst != 0)
    {
        /* query the parent node name */
        if (CM_Get_DevNode_Registry_Property(dnParentDevInst,
                                             CM_DRP_DEVICEDESC,
                                             &RegDataType,
                                             szBuffer,
                                             &DataSize,
                                             0) == CR_SUCCESS &&
             RegDataType == REG_SZ &&
             LoadAndFormatString(hDllInstance,
                                 IDS_DEVONPARENT,
                                 &szFormatted,
                                 szBuffer) != 0)
        {
            wcsncpy(szBuffer,
                    szFormatted,
                    BufferSize - 1);
            szBuffer[BufferSize - 1] = L'\0';
            LocalFree((HLOCAL)szFormatted);
            Ret = TRUE;
        }
    }
    else if (dnDevInst != 0)
    {
        cRet = CM_Get_DevNode_Registry_Property(dnDevInst,
                                                CM_DRP_LOCATION_INFORMATION,
                                                &RegDataType,
                                                szBuffer,
                                                &DataSize,
                                                0);
        if (cRet == CR_SUCCESS && RegDataType == REG_SZ)
        {
            /* FIXME - check string for NULL termination! */
            Ret = TRUE;
        }

        if (Ret && szBuffer[0] >= L'0' && szBuffer[0] <= L'9')
        {
            /* convert the string to an integer value and create a
               formatted string */
            ULONG ulLocation = (ULONG)wcstoul(szBuffer,
                                              NULL,
                                              10);
            if (LoadAndFormatString(hDllInstance,
                                    IDS_LOCATIONSTR,
                                    &szFormatted,
                                    ulLocation,
                                    szBuffer) != 0)
            {
                wcsncpy(szBuffer,
                        szFormatted,
                        BufferSize - 1);
                szBuffer[BufferSize - 1] = L'\0';
                LocalFree((HLOCAL)szFormatted);
            }
            else
                Ret = FALSE;
        }
    }

    if (!Ret &&
        LoadString(hDllInstance,
                   IDS_UNKNOWN,
                   szBuffer,
                   BufferSize))
    {
        Ret = TRUE;
    }

    return Ret;
}


static const UINT ProblemStringId[] =
{
    IDS_DEV_NO_PROBLEM,
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
    IDS_DEV_SETPROPERTIES_FAILED,
};


BOOL
GetDeviceStatusString(IN DEVINST DevInst,
                      IN HMACHINE hMachine,
                      OUT LPWSTR szBuffer,
                      IN DWORD BufferSize)
{
    CONFIGRET cr;
    ULONG Status, ProblemNumber;
    UINT MessageId = IDS_UNKNOWN;
    BOOL Ret = FALSE;

    szBuffer[0] = L'\0';
    cr = CM_Get_DevNode_Status_Ex(&Status,
                                  &ProblemNumber,
                                  DevInst,
                                  0,
                                  hMachine);
    if (cr == CR_SUCCESS)
    {
        if (Status & DN_HAS_PROBLEM)
        {
            if (ProblemNumber < sizeof(ProblemStringId) / sizeof(ProblemStringId[0]))
                MessageId = ProblemStringId[ProblemNumber];

            if (ProblemNumber == 0)
            {
                goto GeneralMessage;
            }
            else
            {
                LPWSTR szProblem;
                UINT StringIDs[] =
                {
                    MessageId,
                    IDS_DEVCODE,
                };

                if (LoadAndFormatStringsCat(hDllInstance,
                                            StringIDs,
                                            sizeof(StringIDs) / sizeof(StringIDs[0]),
                                            &szProblem,
                                            ProblemNumber))
                {
                    wcsncpy(szBuffer,
                            szProblem,
                            BufferSize - 1);
                    szBuffer[BufferSize - 1] = L'\0';

                    LocalFree((HLOCAL)szProblem);

                    Ret = TRUE;
                }
            }
        }
        else
        {
            if (!(Status & (DN_DRIVER_LOADED | DN_STARTED)))
            {
                MessageId = IDS_NODRIVERLOADED;
            }
            else
            {
                MessageId = IDS_DEV_NO_PROBLEM;
            }

            goto GeneralMessage;
        }
    }
    else
    {
GeneralMessage:
        if (LoadString(hDllInstance,
                       MessageId,
                       szBuffer,
                       BufferSize))
        {
            Ret = TRUE;
        }
    }

    return Ret;
}


BOOL
IsDeviceHidden(IN DEVINST DevInst,
               IN HMACHINE hMachine,
               OUT BOOL *IsHidden)
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
        *IsHidden = ((Status & DN_NO_SHOW_IN_DM) != 0);
        Ret = TRUE;
    }

    return Ret;
}


BOOL
CanDisableDevice(IN DEVINST DevInst,
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


BOOL
IsDeviceEnabled(IN DEVINST DevInst,
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


BOOL
GetDeviceTypeString(IN PSP_DEVINFO_DATA DeviceInfoData,
                    OUT LPWSTR szBuffer,
                    IN DWORD BufferSize)
{
    BOOL Ret = FALSE;

    if (!SetupDiGetClassDescription(&DeviceInfoData->ClassGuid,
                                    szBuffer,
                                    BufferSize,
                                    NULL))
    {
        szBuffer[0] = L'\0';
        if (LoadString(hDllInstance,
                       IDS_UNKNOWN,
                       szBuffer,
                       BufferSize))
        {
            Ret = TRUE;
        }
    }
    else
    {
        /* FIXME - check string for NULL termination! */
        Ret = TRUE;
    }

    return Ret;
}


BOOL
GetDeviceDescriptionString(IN HDEVINFO DeviceInfoSet,
                           IN PSP_DEVINFO_DATA DeviceInfoData,
                           OUT LPWSTR szBuffer,
                           IN DWORD BufferSize)
{
    DWORD RegDataType;
    BOOL Ret = FALSE;

    if ((SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                          DeviceInfoData,
                                          SPDRP_FRIENDLYNAME,
                                          &RegDataType,
                                          (PBYTE)szBuffer,
                                          BufferSize * sizeof(WCHAR),
                                          NULL) ||
         SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                          DeviceInfoData,
                                          SPDRP_DEVICEDESC,
                                          &RegDataType,
                                          (PBYTE)szBuffer,
                                          BufferSize * sizeof(WCHAR),
                                          NULL)) &&
        RegDataType == REG_SZ)
    {
        /* FIXME - check string for NULL termination! */
        Ret = TRUE;
    }
    else
    {
        szBuffer[0] = L'\0';
        if (LoadString(hDllInstance,
                       IDS_UNKNOWNDEVICE,
                       szBuffer,
                       BufferSize))
        {
            Ret = TRUE;
        }
    }

    return Ret;
}


HINSTANCE
LoadAndInitComctl32(VOID)
{
    typedef VOID (WINAPI *PINITCOMMONCONTROLS)(VOID);
    PINITCOMMONCONTROLS pInitCommonControls;
    HINSTANCE hComCtl32;

    hComCtl32 = LoadLibrary(L"comctl32.dll");
    if (hComCtl32 != NULL)
    {
        /* initialize the common controls */
        pInitCommonControls = (PINITCOMMONCONTROLS)GetProcAddress(hComCtl32,
                                                                  "InitCommonControls");
        if (pInitCommonControls == NULL)
        {
            FreeLibrary(hComCtl32);
            return NULL;
        }

        pInitCommonControls();
    }

    return hComCtl32;
}


BOOL
STDCALL
DllMain(IN HINSTANCE hinstDLL,
        IN DWORD dwReason,
	    IN LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            hDllInstance = hinstDLL;
            break;
    }

    return TRUE;
}
