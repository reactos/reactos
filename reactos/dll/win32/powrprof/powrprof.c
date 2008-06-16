/*
 * Copyright (C) 2005 Benjamin Cutler
 * Copyright (C) 2008 Dmitry Chapyshev
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */


#include <stdarg.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <winternl.h>
#include <powrprof.h>
#include <wchar.h>
#include <stdio.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(powrprof);


static const WCHAR szPowerCfgSubKey[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\PowerCfg";
static const WCHAR szSemaphoreName[] = L"PowerProfileRegistrySemaphore";
static const WCHAR szDiskMax[] = L"DiskSpindownMax";
static const WCHAR szDiskMin[] = L"DiskSpindownMin";
static const WCHAR szLastID[] = L"LastID";

HANDLE PPRegSemaphore = NULL;

NTSTATUS WINAPI
CallNtPowerInformation(POWER_INFORMATION_LEVEL InformationLevel,
                       PVOID lpInputBuffer,
                       ULONG nInputBufferSize,
                       PVOID lpOutputBuffer,
                       ULONG nOutputBufferSize)
{
    return NtPowerInformation(InformationLevel,
                              lpInputBuffer,
                              nInputBufferSize,
                              lpOutputBuffer,
                              nOutputBufferSize);
}


BOOLEAN WINAPI
CanUserWritePwrScheme(VOID)
{
    HKEY hKey = NULL;
    LONG Ret;
    BOOLEAN bSuccess = TRUE;

    TRACE("()\n");

    Ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE, szPowerCfgSubKey, 0, KEY_READ | KEY_WRITE, &hKey);

    if (Ret != ERROR_SUCCESS)
    {
        TRACE("RegOpenKeyEx failed: %d\n", Ret);
        bSuccess = FALSE;
    }

    SetLastError(Ret);
    RegCloseKey(hKey);
    return bSuccess;
}


BOOLEAN WINAPI
DeletePwrScheme(UINT uiIndex)
{
    WCHAR Buf[MAX_PATH];
    UINT Current;
    LONG Err;

    swprintf(Buf, L"Control Panel\\PowerCfg\\PowerPolicies\\%u", uiIndex);

    if (GetActivePwrScheme(&Current))
    {
        if (Current == uiIndex)
        {
            SetLastError(ERROR_ACCESS_DENIED);
            return FALSE;
        }
        else
        {
            Err = RegDeleteKey(HKEY_CURRENT_USER, (LPCTSTR) Buf);
            if (Err != ERROR_SUCCESS)
            {
                TRACE("RegDeleteKey failed: %d\n", Err);
                SetLastError(Err);
                return FALSE;
            }
            else
            {
                SetLastError(ERROR_SUCCESS);
                return TRUE;
            }
        }
    }

    return FALSE;
}


static BOOLEAN
POWRPROF_GetUserPowerPolicy(HKEY hKey, LPWSTR szNum,
                            USER_POWER_POLICY userPwrPolicy,
                            DWORD *dwName, LPWSTR szName,
                            DWORD *dwDesc, LPWSTR szDesc)
{
    HKEY hSubKey;
    DWORD dwSize;
    LONG Err;

    Err = RegOpenKeyW(hKey, szNum, &hSubKey);
    if (Err != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyW failed: %d\n", Err);
        SetLastError(Err);
        return FALSE;
    }

    *dwName = MAX_PATH * sizeof(WCHAR);
    Err = RegQueryValueExW(hSubKey, L"Name", NULL, NULL, (LPBYTE)szName, dwName);
    if (Err != ERROR_SUCCESS)
    {
        ERR("RegQueryValueExW failed: %d\n", Err);
        SetLastError(Err);
        return FALSE;
    }

    *dwDesc = MAX_PATH * sizeof(WCHAR);
    Err = RegQueryValueExW(hSubKey, L"Description", NULL, NULL, (LPBYTE)szDesc, dwDesc);
    if (Err != ERROR_SUCCESS)
    {
        ERR("RegQueryValueExW failed: %d\n", Err);
        SetLastError(Err);
        return FALSE;
    }

    dwSize = sizeof(USER_POWER_POLICY);
    Err = RegQueryValueExW(hSubKey, L"Policies", NULL, NULL, (LPBYTE)&userPwrPolicy, &dwSize);
    if (Err != ERROR_SUCCESS)
    {
        ERR("RegQueryValueExW failed: %d\n", Err);
        SetLastError(Err);
        return FALSE;
    }

    return TRUE;
}

static BOOLEAN
POWRPROF_GetMachinePowerPolicy(LPWSTR szNum, MACHINE_POWER_POLICY machinePwrPolicy)
{
    HKEY hKey;
    LONG Err;
    WCHAR szPath[MAX_PATH];
    DWORD dwSize;

    swprintf(szPath, L"Software\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\PowerCfg\\PowerPolicies\\%s", szNum);

    Err = RegOpenKeyW(HKEY_LOCAL_MACHINE, szPath, &hKey);
    if (Err != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyW failed: %d\n", Err);
        SetLastError(Err);
        return FALSE;
    }

    dwSize = sizeof(MACHINE_POWER_POLICY);
    Err = RegQueryValueExW(hKey, L"Policies", NULL, NULL, (LPBYTE)&machinePwrPolicy, &dwSize);
    if (Err != ERROR_SUCCESS)
    {
        ERR("RegQueryValueExW failed: %d\n", Err);
        SetLastError(Err);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN WINAPI
EnumPwrSchemes(PWRSCHEMESENUMPROC lpfnPwrSchemesEnumProc,
               LPARAM lParam)
{
    HKEY hKey;
    LONG Err;
    DWORD dwSize, dwNameSize, dwDescSize, dwIndex = 0;
    WCHAR szNum[3 + 1], szName[MAX_PATH], szDesc[MAX_PATH];
    PPOWER_POLICY pPwrPolicy;
    USER_POWER_POLICY userPwrPolicy;
    MACHINE_POWER_POLICY machinePwrPolicy;

    if (!lpfnPwrSchemesEnumProc)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Err = RegOpenKeyW(HKEY_CURRENT_USER, L"Control Panel\\PowerCfg\\PowerPolicies", &hKey);
    if (Err != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyW failed: %d\n", Err);
        SetLastError(Err);
        return FALSE;
    }

    ReleaseSemaphore(PPRegSemaphore, 1, NULL);

    dwSize = sizeof(szNum) / sizeof(WCHAR);

    while (RegEnumKeyExW(hKey, dwIndex, szNum, &dwSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        if (!POWRPROF_GetUserPowerPolicy(hKey, szNum, userPwrPolicy,
                                         &dwNameSize, szName,
                                         &dwDescSize, szDesc))
        {
            RegCloseKey(hKey);
            ReleaseSemaphore(PPRegSemaphore, 1, NULL);
            return FALSE;
        }

        if (!POWRPROF_GetMachinePowerPolicy(szNum, machinePwrPolicy))
        {
            RegCloseKey(hKey);
            ReleaseSemaphore(PPRegSemaphore, 1, NULL);
            return FALSE;
        }

        ZeroMemory(&pPwrPolicy, sizeof(PPOWER_POLICY));
        pPwrPolicy->user = userPwrPolicy;
        pPwrPolicy->mach = machinePwrPolicy;

        if (!lpfnPwrSchemesEnumProc(_wtoi(szNum), dwNameSize, szName, dwDescSize, szDesc, pPwrPolicy, lParam))
            break;

        dwSize = sizeof(szNum) / sizeof(WCHAR);
        dwIndex++;
    }

    RegCloseKey(hKey);
    ReleaseSemaphore(PPRegSemaphore, 1, NULL);
    SetLastError(ERROR_SUCCESS);

    return TRUE;
}


BOOLEAN WINAPI
GetActivePwrScheme(PUINT puiID)
{
    HKEY hKey;
    WCHAR szBuf[MAX_PATH];
    DWORD dwSize;
    LONG Err;

    TRACE("GetActivePwrScheme(%u)", puiID);

    Err = RegOpenKeyW(HKEY_CURRENT_USER, L"Control Panel\\PowerCfg", &hKey);
    if (Err != ERROR_SUCCESS)
    {
        ERR("RegOpenKey failed: %d\n", Err);
        SetLastError(Err);
        return FALSE;
    }

    dwSize = MAX_PATH;
    Err = RegQueryValueExW(hKey, L"CurrentPowerPolicy",
                           NULL, NULL,
                           (LPBYTE)&szBuf, &dwSize);
    if (Err != ERROR_SUCCESS)
    {
        ERR("RegQueryValueEx failed: %d\n", Err);
        RegCloseKey(hKey);
        SetLastError(Err);
        return FALSE;
    }

    *puiID = _wtoi(szBuf);

    RegCloseKey(hKey);
    SetLastError(ERROR_SUCCESS);
    return TRUE;
}


BOOLEAN WINAPI
GetCurrentPowerPolicies(PGLOBAL_POWER_POLICY pGlobalPowerPolicy,
                        PPOWER_POLICY pPowerPolicy)
{
    SYSTEM_POWER_POLICY ACPower, DCPower;

    FIXME("(%p, %p) stub!\n", pGlobalPowerPolicy, pPowerPolicy);

    NtPowerInformation(SystemPowerPolicyAc, 0, 0, &ACPower, sizeof(SYSTEM_POWER_POLICY));
    NtPowerInformation(SystemPowerPolicyDc, 0, 0, &DCPower, sizeof(SYSTEM_POWER_POLICY));

    return FALSE;
}


BOOLEAN WINAPI
GetPwrCapabilities(PSYSTEM_POWER_CAPABILITIES lpSystemPowerCapabilities)
{
    NTSTATUS Ret;

    TRACE("(%p)\n", lpSystemPowerCapabilities);

    if (!lpSystemPowerCapabilities)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Ret = NtPowerInformation(SystemPowerCapabilities, 0, 0, lpSystemPowerCapabilities, sizeof(SYSTEM_POWER_CAPABILITIES));

    SetLastError(RtlNtStatusToDosError(Ret));

    if (Ret == STATUS_SUCCESS)
        return TRUE;
    else
        return FALSE;
}


BOOLEAN WINAPI
GetPwrDiskSpindownRange(PUINT RangeMax, PUINT RangeMin)
{
    HKEY hKey;
    BYTE lpValue[40];
    LONG Ret;
    DWORD cbValue = sizeof(lpValue);

    TRACE("(%p, %p)\n", RangeMax, RangeMin);

    if (RangeMax == NULL || RangeMin == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    SetLastError(ERROR_SUCCESS);

    WaitForSingleObject(PPRegSemaphore, INFINITE);

    Ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE, szPowerCfgSubKey, 0, KEY_READ, &hKey);
    if (Ret != ERROR_SUCCESS)
    {
        TRACE("RegOpenKeyEx failed: %d\n", Ret);
        TRACE("Using defaults: 3600, 3\n");
        *RangeMax = 3600;
        *RangeMin = 3;
        ReleaseSemaphore(PPRegSemaphore, 1, NULL);
        return TRUE;
    }

    Ret = RegQueryValueExW(hKey, szDiskMax, 0, 0, lpValue, &cbValue);
    if (Ret != ERROR_SUCCESS)
    {
        TRACE("Couldn't open DiskSpinDownMax: %d\n", Ret);
        TRACE("Using default: 3600\n");
        *RangeMax = 3600;
    }
    else
    {
        *RangeMax = _wtoi((LPCWSTR)lpValue);
    }

    cbValue = sizeof(lpValue);

    Ret = RegQueryValueExW(hKey, szDiskMin, 0, 0, lpValue, &cbValue);
    if (Ret != ERROR_SUCCESS)
    {
        TRACE("Couldn't open DiskSpinDownMin: %d\n", Ret);
        TRACE("Using default: 3\n");
        *RangeMin = 3;
    }
    else
    {
        *RangeMin = _wtoi((LPCWSTR)lpValue);
    }

    RegCloseKey(hKey);

    ReleaseSemaphore(PPRegSemaphore, 1, NULL);
    SetLastError(ERROR_SUCCESS);

    return TRUE;
}


BOOLEAN WINAPI
IsAdminOverrideActive(PADMINISTRATOR_POWER_POLICY p)
{
    FIXME("( %p) stub!\n", p);
    return FALSE;
}


BOOLEAN WINAPI
IsPwrHibernateAllowed(VOID)
{
    SYSTEM_POWER_CAPABILITIES PowerCaps;
    FIXME("() stub!\n");
    NtPowerInformation(SystemPowerCapabilities, NULL, 0, &PowerCaps, sizeof(PowerCaps));
    return FALSE;
}


BOOLEAN WINAPI
IsPwrShutdownAllowed(VOID)
{
    SYSTEM_POWER_CAPABILITIES PowerCaps;
    FIXME("() stub!\n");
    NtPowerInformation(SystemPowerCapabilities, NULL, 0, &PowerCaps, sizeof(PowerCaps));
    return FALSE;
}


BOOLEAN WINAPI
IsPwrSuspendAllowed(VOID)
{
    SYSTEM_POWER_CAPABILITIES PowerCaps;
    FIXME("() stub!\n");
    NtPowerInformation(SystemPowerCapabilities, NULL, 0, &PowerCaps, sizeof(PowerCaps));
    return FALSE;
}


BOOLEAN WINAPI
ReadGlobalPwrPolicy(PGLOBAL_POWER_POLICY pGlobalPowerPolicy)
{
    FIXME("(%p) stub!\n", pGlobalPowerPolicy);
    SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}


BOOLEAN WINAPI
ReadProcessorPwrScheme(UINT uiID,
                       PMACHINE_PROCESSOR_POWER_POLICY pMachineProcessorPowerPolicy)
{
    FIXME("(%d, %p) stub!\n", uiID, pMachineProcessorPowerPolicy);
    SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}


BOOLEAN WINAPI
ReadPwrScheme(UINT uiID,
    PPOWER_POLICY pPowerPolicy)
{
    FIXME("(%d, %p) stub!\n", uiID, pPowerPolicy);
    SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}


BOOLEAN WINAPI
SetActivePwrScheme(UINT uiID,
                   PGLOBAL_POWER_POLICY lpGlobalPowerPolicy,
                   PPOWER_POLICY lpPowerPolicy)
{
    FIXME("(%d, %p, %p) stub!\n", uiID, lpGlobalPowerPolicy, lpPowerPolicy);
    SetLastError(ERROR_FILE_NOT_FOUND);
    return FALSE;
}


BOOLEAN WINAPI
SetSuspendState(BOOLEAN Hibernate,
                BOOLEAN ForceCritical,
                BOOLEAN DisableWakeEvent)
{
    FIXME("(%d, %d, %d) stub!\n", Hibernate, ForceCritical, DisableWakeEvent);
    return TRUE;
}


BOOLEAN WINAPI
WriteGlobalPwrPolicy(PGLOBAL_POWER_POLICY pGlobalPowerPolicy)
{
    FIXME("(%p) stub!\n", pGlobalPowerPolicy);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


BOOLEAN WINAPI
WriteProcessorPwrScheme(UINT ID,
                        PMACHINE_PROCESSOR_POWER_POLICY pMachineProcessorPowerPolicy)
{
    FIXME("(%d, %p) stub!\n", ID, pMachineProcessorPowerPolicy);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


BOOLEAN WINAPI
WritePwrScheme(PUINT puiID,
               LPWSTR lpszName,
               LPWSTR lpszDescription,
               PPOWER_POLICY pPowerPolicy)
{
    FIXME("(%p, %s, %s, %p) stub!\n", puiID, debugstr_w(lpszName), debugstr_w(lpszDescription), pPowerPolicy);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


BOOLEAN WINAPI
ValidatePowerPolicies(PGLOBAL_POWER_POLICY pGPP, PPOWER_POLICY pPP)
{
    FIXME("(%p, %p) stub!\n", pGPP, pPP);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return TRUE;
}


BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    FIXME("(%p, %d, %p) not fully implemented\n", hinstDLL, fdwReason, lpvReserved);

    switch(fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {

            HKEY hKey;
            LONG r;

            DisableThreadLibraryCalls(hinstDLL);

            r = RegOpenKeyExW(HKEY_LOCAL_MACHINE, szPowerCfgSubKey, 0, KEY_READ | KEY_WRITE, &hKey);

            if (r != ERROR_SUCCESS)
            {
                TRACE("Couldn't open registry key HKLM\\%s, using some sane(?) defaults\n", debugstr_w(szPowerCfgSubKey));
            }
            else
            {
                BYTE lpValue[40];
                DWORD cbValue = sizeof(lpValue);
                r = RegQueryValueExW(hKey, szLastID, 0, 0, lpValue, &cbValue);
                if (r != ERROR_SUCCESS)
                {
                    TRACE("Couldn't open registry entry HKLM\\%s\\LastID, using some sane(?) defaults\n", debugstr_w(szPowerCfgSubKey));
                }
                RegCloseKey(hKey);
            }

            PPRegSemaphore = CreateSemaphoreW(NULL, 1, 1, szSemaphoreName);
            if (PPRegSemaphore == NULL)
            {
                ERR("Couldn't create Semaphore: %d\n", GetLastError());
                return FALSE;
            }
            break;
        }
        case DLL_PROCESS_DETACH:
            CloseHandle(PPRegSemaphore);
            break;
    }
    return TRUE;
}
