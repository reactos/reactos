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

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#define NTOS_MODE_USER
#include <ndk/pofuncs.h>
#include <ndk/rtlfuncs.h>
#include <ndk/setypes.h>
#include <powrprof.h>
#include <wine/debug.h>
#include <wine/unicode.h>

WINE_DEFAULT_DEBUG_CHANNEL(powrprof);


static const WCHAR szPowerCfgSubKey[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\PowerCfg";
static const WCHAR szUserPowerConfigSubKey[] =
    L"Control Panel\\PowerCfg";
static const WCHAR szCurrentPowerPolicies[] =
    L"CurrentPowerPolicy";
static const WCHAR szPolicies[] = L"Policies";
static const WCHAR szName[] = L"Name";
static const WCHAR szDescription[] = L"Description";
static const WCHAR szSemaphoreName[] = L"PowerProfileRegistrySemaphore";
static const WCHAR szDiskMax[] = L"DiskSpindownMax";
static const WCHAR szDiskMin[] = L"DiskSpindownMin";
static const WCHAR szLastID[] = L"LastID";

UINT g_LastID = (UINT)-1;

BOOLEAN WINAPI WritePwrPolicy(PUINT puiID, PPOWER_POLICY pPowerPolicy);

HANDLE PPRegSemaphore = NULL;

NTSTATUS WINAPI
CallNtPowerInformation(POWER_INFORMATION_LEVEL InformationLevel,
                       PVOID lpInputBuffer,
                       ULONG nInputBufferSize,
                       PVOID lpOutputBuffer,
                       ULONG nOutputBufferSize)
{
    BOOLEAN old;

	//Lohnegrim: In order to get the right results, we have to adjust our Privileges
    RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE, FALSE, &old);
    RtlAdjustPrivilege(SE_CREATE_PAGEFILE_PRIVILEGE, TRUE, FALSE, &old);

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

    TRACE("()\n");

    Ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE, szPowerCfgSubKey, 0, KEY_READ | KEY_WRITE, &hKey);
    if (Ret != ERROR_SUCCESS)
    {
        TRACE("RegOpenKeyEx failed: %d\n", Ret);
        SetLastError(Ret);
        return FALSE;
    }

    RegCloseKey(hKey);
    return TRUE;
}

BOOLEAN WINAPI
DeletePwrScheme(UINT uiIndex)
{
    WCHAR Buf[MAX_PATH];
    UINT Current;
    LONG Err;

    swprintf(Buf, L"Control Panel\\PowerCfg\\PowerPolicies\\%d", uiIndex);

    if (!GetActivePwrScheme(&Current))
        return FALSE;

    if (Current == uiIndex)
    {
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    Err = RegDeleteKey(HKEY_CURRENT_USER, (LPCTSTR)Buf);
    if (Err != ERROR_SUCCESS)
    {
        TRACE("RegDeleteKey failed: %d\n", Err);
        SetLastError(Err);
        return FALSE;
    }

    return TRUE;
}

static BOOLEAN
POWRPROF_GetUserPowerPolicy(LPWSTR szNum,
                            PUSER_POWER_POLICY puserPwrPolicy,
                            DWORD cchName, LPWSTR szName,
                            DWORD cchDesc, LPWSTR szDesc)
{
    HKEY hSubKey = NULL;
    DWORD dwSize;
    LONG Err;
    WCHAR szPath[MAX_PATH];
    BOOL bRet = FALSE;

    swprintf(szPath, L"Control Panel\\PowerCfg\\PowerPolicies\\%s", szNum);

    Err = RegOpenKeyExW(HKEY_CURRENT_USER, szPath, 0, KEY_READ, &hSubKey);
    if (Err != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW failed: %d\n", Err);
        SetLastError(Err);
        return FALSE;
    }

    dwSize = cchName * sizeof(WCHAR);
    Err = RegQueryValueExW(hSubKey, L"Name", NULL, NULL, (LPBYTE)szName, &dwSize);
    if (Err != ERROR_SUCCESS)
    {
        ERR("RegQueryValueExW failed: %d\n", Err);
        SetLastError(Err);
        goto cleanup;
    }

    dwSize = cchDesc * sizeof(WCHAR);
    Err = RegQueryValueExW(hSubKey, L"Description", NULL, NULL, (LPBYTE)szDesc, &dwSize);
    if (Err != ERROR_SUCCESS)
    {
        ERR("RegQueryValueExW failed: %d\n", Err);
        SetLastError(Err);
        goto cleanup;
    }

    dwSize = sizeof(USER_POWER_POLICY);
    Err = RegQueryValueExW(hSubKey, L"Policies", NULL, NULL, (LPBYTE)puserPwrPolicy, &dwSize);
    if (Err != ERROR_SUCCESS)
    {
        ERR("RegQueryValueExW failed: %d\n", Err);
        SetLastError(Err);
        goto cleanup;
    }

    bRet = TRUE;

cleanup:
    RegCloseKey(hSubKey);

    return bRet;
}

static BOOLEAN
POWRPROF_GetMachinePowerPolicy(LPWSTR szNum, PMACHINE_POWER_POLICY pmachinePwrPolicy)
{
    HKEY hKey;
    LONG Err;
    WCHAR szPath[MAX_PATH];
    DWORD dwSize;

    swprintf(szPath, L"Software\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\PowerCfg\\PowerPolicies\\%s", szNum);

    Err = RegOpenKeyExW(HKEY_LOCAL_MACHINE, szPath, 0, KEY_READ, &hKey);
    if (Err != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW failed: %d\n", Err);
        SetLastError(Err);
        return FALSE;
    }

    dwSize = sizeof(MACHINE_POWER_POLICY);
    Err = RegQueryValueExW(hKey, L"Policies", NULL, NULL, (LPBYTE)pmachinePwrPolicy, &dwSize);

    if (Err != ERROR_SUCCESS)
    {
        ERR("RegQueryValueExW failed: %d\n", Err);
        SetLastError(Err);
        RegCloseKey(hKey);
        return FALSE;
    }

    RegCloseKey(hKey);

    return TRUE;
}

BOOLEAN WINAPI
EnumPwrSchemes(PWRSCHEMESENUMPROC lpfnPwrSchemesEnumProc,
               LPARAM lParam)
{
    HKEY hKey;
    LONG Err;
    DWORD dwSize, dwNameSize = MAX_PATH, dwDescSize = MAX_PATH, dwIndex = 0;
    WCHAR szNum[3 + 1], szName[MAX_PATH], szDesc[MAX_PATH];
    POWER_POLICY PwrPolicy;
    USER_POWER_POLICY userPwrPolicy;
    MACHINE_POWER_POLICY machinePwrPolicy;
    BOOLEAN bRet = FALSE;

    if (!lpfnPwrSchemesEnumProc)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Err = RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\PowerCfg\\PowerPolicies", 0, KEY_READ, &hKey);
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
        if (!POWRPROF_GetUserPowerPolicy(szNum, &userPwrPolicy,
                                         dwNameSize, szName,
                                         dwDescSize, szDesc))
        {
            WARN("POWRPROF_GetUserPowerPolicy failed\n");
            goto cleanup;
        }

        if (!POWRPROF_GetMachinePowerPolicy(szNum, &machinePwrPolicy))
        {
            WARN("POWRPROF_GetMachinePowerPolicy failed\n");
            goto cleanup;
        }

        memcpy(&PwrPolicy.user, &userPwrPolicy, sizeof(USER_POWER_POLICY));
        memcpy(&PwrPolicy.mach, &machinePwrPolicy, sizeof(MACHINE_POWER_POLICY));

        if (!lpfnPwrSchemesEnumProc(_wtoi(szNum), (wcslen(szName) + 1) * sizeof(WCHAR), szName, (wcslen(szDesc) + 1) * sizeof(WCHAR), szDesc, &PwrPolicy, lParam))
            goto cleanup;
        else
            bRet = TRUE;

        dwSize = sizeof(szNum) / sizeof(WCHAR);
        dwIndex++;
    }

cleanup:
    RegCloseKey(hKey);
    ReleaseSemaphore(PPRegSemaphore, 1, NULL);

    return bRet;
}

BOOLEAN WINAPI
GetActivePwrScheme(PUINT puiID)
{
    HKEY hKey;
    WCHAR szBuf[MAX_PATH];
    DWORD dwSize;
    LONG Err;

    TRACE("GetActivePwrScheme(%u)", puiID);

    Err = RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\PowerCfg", 0, KEY_READ, &hKey);
    if (Err != ERROR_SUCCESS)
    {
        ERR("RegOpenKey failed: %d\n", Err);
        SetLastError(Err);
        return FALSE;
    }

    dwSize = sizeof(szBuf);
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

    RegCloseKey(hKey);
    *puiID = _wtoi(szBuf);

    return TRUE;
}

BOOLEAN WINAPI
GetCurrentPowerPolicies(PGLOBAL_POWER_POLICY pGlobalPowerPolicy,
                        PPOWER_POLICY pPowerPolicy)
{
    /*
    SYSTEM_POWER_POLICY ACPower, DCPower;

    FIXME("(%p, %p) stub!\n", pGlobalPowerPolicy, pPowerPolicy);

    NtPowerInformation(SystemPowerPolicyAc, 0, 0, &ACPower, sizeof(SYSTEM_POWER_POLICY));
    NtPowerInformation(SystemPowerPolicyDc, 0, 0, &DCPower, sizeof(SYSTEM_POWER_POLICY));

    return FALSE;
    */
/*
   Lohnegrim: I don't know why this Function should call NtPowerInformation, because as far as I know,
      it simply returns the GlobalPowerPolicy and the AktivPowerScheme!
 */
    UINT uiID;

    if (pGlobalPowerPolicy != NULL)
    {
        if (!ReadGlobalPwrPolicy(pGlobalPowerPolicy))
            return FALSE;
    }
    if (pPowerPolicy != NULL)
    {
        if (!GetActivePwrScheme(&uiID))
            return FALSE;

        if (!ReadPwrScheme(uiID, pPowerPolicy))
            return FALSE;
    }

    return TRUE;
}

BOOLEAN WINAPI
GetPwrCapabilities(PSYSTEM_POWER_CAPABILITIES lpSystemPowerCapabilities)
{
    NTSTATUS Status;

    TRACE("(%p)\n", lpSystemPowerCapabilities);

    if (!lpSystemPowerCapabilities)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Status = NtPowerInformation(SystemPowerCapabilities, 0, 0, lpSystemPowerCapabilities, sizeof(SYSTEM_POWER_CAPABILITIES));
    if(!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
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
    NTSTATUS Status;
    BOOLEAN old;

    RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE, FALSE, &old);

    Status = NtPowerInformation(SystemPowerCapabilities, NULL, 0, &PowerCaps, sizeof(PowerCaps));
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return PowerCaps.SystemS4 && PowerCaps.HiberFilePresent; // IsHiberfilPresent();
}

BOOLEAN WINAPI
IsPwrShutdownAllowed(VOID)
{
    SYSTEM_POWER_CAPABILITIES PowerCaps;
    NTSTATUS Status;
    BOOLEAN old;

    RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE, FALSE, &old);

    Status = NtPowerInformation(SystemPowerCapabilities, NULL, 0, &PowerCaps, sizeof(PowerCaps));
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return PowerCaps.SystemS5;
}

BOOLEAN WINAPI
IsPwrSuspendAllowed(VOID)
{
    SYSTEM_POWER_CAPABILITIES PowerCaps;
    NTSTATUS Status;
    BOOLEAN old;

    RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE, FALSE, &old);

    Status = NtPowerInformation(SystemPowerCapabilities, NULL, 0, &PowerCaps, sizeof(PowerCaps));
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return PowerCaps.SystemS1 || PowerCaps.SystemS2 || PowerCaps.SystemS3;
}

DWORD WINAPI
PowerGetActiveScheme(HKEY UserRootPowerKey, GUID **polguid)
{
   FIXME("(%p,%p) stub!\n", UserRootPowerKey, polguid);
   return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI
PowerReadDCValue(HKEY RootPowerKey, const GUID *Scheme, const GUID *SubGroup, const GUID *PowerSettings, PULONG Type, PUCHAR Buffer, DWORD *BufferSize)
{
   FIXME("(%p,%s,%s,%s,%p,%p,%p) stub!\n", RootPowerKey, debugstr_guid(Scheme), debugstr_guid(SubGroup), debugstr_guid(PowerSettings), Type, Buffer, BufferSize);
   return ERROR_CALL_NOT_IMPLEMENTED;
}

BOOLEAN WINAPI
ReadGlobalPwrPolicy(PGLOBAL_POWER_POLICY pGlobalPowerPolicy)
{
    GLOBAL_MACHINE_POWER_POLICY glMachPwrPolicy;
    GLOBAL_USER_POWER_POLICY glUserPwrPolicy;
    HKEY hKey = NULL;
    DWORD dwSize;
    LONG Err;
    BOOL bRet = FALSE;

    ReleaseSemaphore(PPRegSemaphore, 1, NULL);

    // Getting user global power policy
    Err = RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\PowerCfg\\GlobalPowerPolicy", 0, KEY_READ, &hKey);
    if (Err != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyW failed: %d\n", Err);
        SetLastError(Err);
        goto cleanup;
    }

    dwSize = sizeof(glUserPwrPolicy);
    Err = RegQueryValueExW(hKey, L"Policies", NULL, NULL, (LPBYTE)&glUserPwrPolicy, &dwSize);
    if (Err != ERROR_SUCCESS)
    {
        ERR("RegQueryValueExW failed: %d\n", Err);
        SetLastError(Err);
        goto cleanup;
    }

    RegCloseKey(hKey);

    // Getting machine global power policy
    Err = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\PowerCfg\\GlobalPowerPolicy", 0, KEY_READ, &hKey);
    if (Err != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyW failed: %d\n", Err);
        SetLastError(Err);
        goto cleanup;
    }

    dwSize = sizeof(glMachPwrPolicy);
    Err = RegQueryValueExW(hKey, L"Policies", NULL, NULL, (LPBYTE)&glMachPwrPolicy, &dwSize);
    if (Err != ERROR_SUCCESS)
    {
        ERR("RegQueryValueExW failed: %d\n", Err);
        SetLastError(Err);
        goto cleanup;
    }

    memcpy(&pGlobalPowerPolicy->user, &glUserPwrPolicy, sizeof(GLOBAL_USER_POWER_POLICY));
    memcpy(&pGlobalPowerPolicy->mach, &glMachPwrPolicy, sizeof(GLOBAL_MACHINE_POWER_POLICY));
    bRet = TRUE;

cleanup:
    if(hKey)
        RegCloseKey(hKey);
    ReleaseSemaphore(PPRegSemaphore, 1, NULL);

    return bRet;
}


BOOLEAN WINAPI
ReadProcessorPwrScheme(UINT uiID,
                       PMACHINE_PROCESSOR_POWER_POLICY pMachineProcessorPowerPolicy)
{
    HKEY hKey;
    WCHAR szPath[MAX_PATH];
    DWORD dwSize = sizeof(MACHINE_PROCESSOR_POWER_POLICY);

    swprintf(szPath, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\PowerCfg\\ProcessorPolicies\\%i", uiID);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return FALSE;

    if (RegQueryValueExW(hKey, szPolicies, NULL, 0, (LPBYTE)pMachineProcessorPowerPolicy, &dwSize) == ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return TRUE;
    }

    RegCloseKey(hKey);
    if (uiID != 0)
        return ReadProcessorPwrScheme(0, pMachineProcessorPowerPolicy);

    return FALSE;
}


BOOLEAN WINAPI
ReadPwrScheme(UINT uiID,
    PPOWER_POLICY pPowerPolicy)
{
    USER_POWER_POLICY userPwrPolicy;
    MACHINE_POWER_POLICY machinePwrPolicy;
    WCHAR szNum[16]; // max number - 999

    ReleaseSemaphore(PPRegSemaphore, 1, NULL);

    swprintf(szNum, L"%d", uiID);

    if (!POWRPROF_GetUserPowerPolicy(szNum, &userPwrPolicy, 0, NULL, 0, NULL))
    {
        ReleaseSemaphore(PPRegSemaphore, 1, NULL);
        return FALSE;
    }

    if (!POWRPROF_GetMachinePowerPolicy(szNum, &machinePwrPolicy))
    {
        ReleaseSemaphore(PPRegSemaphore, 1, NULL);
        return FALSE;
    }

    memcpy(&pPowerPolicy->user, &userPwrPolicy, sizeof(userPwrPolicy));
    memcpy(&pPowerPolicy->mach, &machinePwrPolicy, sizeof(machinePwrPolicy));

    ReleaseSemaphore(PPRegSemaphore, 1, NULL);

    return TRUE;
}

BOOLEAN WINAPI
SetActivePwrScheme(UINT uiID,
                   PGLOBAL_POWER_POLICY lpGlobalPowerPolicy,
                   PPOWER_POLICY lpPowerPolicy)
{
    POWER_POLICY tmp;
    HKEY hKey;
    WCHAR Buf[16];

    if (!ReadPwrScheme(uiID, &tmp))
        return FALSE;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, szUserPowerConfigSubKey, 0, KEY_WRITE, &hKey) != ERROR_SUCCESS)
        return FALSE;

    swprintf(Buf, L"%i", uiID);

    if (RegSetValueExW(hKey, szCurrentPowerPolicies, 0, REG_SZ, (PBYTE)Buf, strlenW(Buf)*sizeof(WCHAR)) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return FALSE;
    }
    RegCloseKey(hKey);

    if (lpGlobalPowerPolicy != NULL || lpPowerPolicy != NULL)
    {
        if (!ValidatePowerPolicies(lpGlobalPowerPolicy, lpPowerPolicy))
            return FALSE;

        if (lpGlobalPowerPolicy != NULL && !WriteGlobalPwrPolicy(lpGlobalPowerPolicy))
            return FALSE;

        if (lpPowerPolicy != NULL && !WritePwrPolicy(&uiID,lpPowerPolicy))
            return FALSE;
    }

    return TRUE;
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
    HKEY hKey;
    GLOBAL_USER_POWER_POLICY gupp;
    GLOBAL_MACHINE_POWER_POLICY gmpp;

    gupp = pGlobalPowerPolicy->user;
    gmpp = pGlobalPowerPolicy->mach;

    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                    L"Control Panel\\PowerCfg\\GlobalPowerPolicy",
                    0,
                    KEY_WRITE,
                    &hKey) != ERROR_SUCCESS)
        return FALSE;

    if (RegSetValueExW(hKey, szPolicies, 0, REG_BINARY, (PBYTE)&gupp, sizeof(gupp)) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    RegCloseKey(hKey);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\PowerCfg\\GlobalPowerPolicy",
                     0,
                     KEY_ALL_ACCESS,
                     &hKey))
        return FALSE;

    if (RegSetValueExW(hKey,szPolicies, 0, REG_BINARY, (PBYTE)&gmpp, sizeof(gmpp)) != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    RegCloseKey(hKey);
    return TRUE;
}

BOOLEAN WINAPI
WriteProcessorPwrScheme(UINT ID,
                        PMACHINE_PROCESSOR_POWER_POLICY pMachineProcessorPowerPolicy)
{
    WCHAR Buf[MAX_PATH];
    HKEY hKey;

    swprintf(Buf, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\PowerCfg\\ProcessorPolicies\\%i", ID);

    if (RegCreateKey(HKEY_LOCAL_MACHINE, Buf, &hKey) != ERROR_SUCCESS)
        return FALSE;

    RegSetValueExW(hKey, szPolicies, 0, REG_BINARY, (PBYTE)pMachineProcessorPowerPolicy, sizeof(MACHINE_PROCESSOR_POWER_POLICY));
    RegCloseKey(hKey);
    return TRUE;
}

static VOID
SetLastID(VOID)
{
    WCHAR Buf[16];
    HKEY hKey;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    szPowerCfgSubKey,
                    0,
                    KEY_WRITE,
                    &hKey) != ERROR_SUCCESS)
        return;
    swprintf(Buf, L"%i", g_LastID);
    RegSetValueExW(hKey, szLastID, 0, REG_SZ, (PBYTE)Buf, strlenW(Buf)*sizeof(WCHAR));
    RegCloseKey(hKey);
}

BOOLEAN WINAPI
WritePwrScheme(PUINT puiID,
               LPWSTR lpszName,
               LPWSTR lpszDescription,
               PPOWER_POLICY pPowerPolicy)
{
    WCHAR Buf[MAX_PATH];
    HKEY hKey;

    if (*puiID == -1)
    {
        g_LastID++;
        *puiID = g_LastID;
        SetLastID();
    }

    swprintf(Buf, L"Control Panel\\PowerCfg\\PowerPolicies\\%i", *puiID);

    if (RegCreateKey(HKEY_CURRENT_USER, Buf, &hKey) != ERROR_SUCCESS)
        return FALSE;

    RegSetValueExW(hKey, szName, 0, REG_SZ, (PBYTE)lpszName, strlenW(lpszName)*sizeof(WCHAR));
    RegSetValueExW(hKey, szDescription, 0, REG_SZ, (PBYTE)lpszDescription, strlenW(lpszDescription)*sizeof(WCHAR));
    RegCloseKey(hKey);
    return WritePwrPolicy(puiID, pPowerPolicy);
}

static BOOLEAN
CheckPowerActionPolicy(PPOWER_ACTION_POLICY pPAP, SYSTEM_POWER_CAPABILITIES PowerCaps)
{
/*
   Lohnegrim: this is an Helper function, it checks if the POWERACTIONPOLICY is valid
   Also, if the System doesn't support Hibernation, then change the PowerAction
*/
    switch (pPAP->Action)
    {
    case PowerActionNone:
        return TRUE;
    case PowerActionReserved:
        if (PowerCaps.SystemS1 || PowerCaps.SystemS2 || PowerCaps.SystemS3)
            pPAP->Action = PowerActionSleep;
        else
            pPAP->Action = PowerActionReserved;
    case PowerActionSleep:
        return TRUE;
    case PowerActionHibernate:
        if (!(PowerCaps.SystemS4 && PowerCaps.HiberFilePresent))
        {
            if (PowerCaps.SystemS1 || PowerCaps.SystemS2 || PowerCaps.SystemS3)
                pPAP->Action = PowerActionSleep;
            else
                pPAP->Action = PowerActionReserved;
        }
    case PowerActionShutdown:
    case PowerActionShutdownReset:
    case PowerActionShutdownOff:
    case PowerActionWarmEject:
        return TRUE;
    default:
        SetLastError(ERROR_INVALID_DATA);
        return FALSE;
    };
}

/**
 * @brief
 * Creates a security descriptor for the power
 * management registry semaphore.
 *
 * @param[out] PowrProfSd
 * A pointer to an allocated security descriptor
 * for the semaphore.
 *
 * @return
 * Returns TRUE if the function succeeds, otherwise
 * FALSE is returned.
 *
 * @remarks
 * Authenticated users are only given a subset of specific
 * rights for the semaphore access, local system and admins
 * have full power.
 */
static BOOLEAN
CreatePowrProfSemaphoreSecurity(_Out_ PSECURITY_DESCRIPTOR *PowrProfSd)
{
    BOOLEAN Success = FALSE;
    PACL Dacl;
    ULONG DaclSize, RelSDSize = 0;
    PSID AuthenticatedUsersSid = NULL, SystemSid = NULL, AdminsSid = NULL;
    SECURITY_DESCRIPTOR AbsSd;
    PSECURITY_DESCRIPTOR RelSd = NULL;
    static SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};

    if (!AllocateAndInitializeSid(&NtAuthority,
                                  1,
                                  SECURITY_AUTHENTICATED_USER_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &AuthenticatedUsersSid))
    {
        return FALSE;
    }

    if (!AllocateAndInitializeSid(&NtAuthority,
                                  1,
                                  SECURITY_LOCAL_SYSTEM_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &SystemSid))
    {
        goto Quit;
    }

    if (!AllocateAndInitializeSid(&NtAuthority,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0,
                                  &AdminsSid))
    {
        goto Quit;
    }

    if (!InitializeSecurityDescriptor(&AbsSd, SECURITY_DESCRIPTOR_REVISION))
    {
        goto Quit;
    }

    DaclSize = sizeof(ACL) +
               sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(AuthenticatedUsersSid) +
               sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(SystemSid) +
               sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(AdminsSid);

    Dacl = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, DaclSize);
    if (!Dacl)
    {
        goto Quit;
    }

    if (!InitializeAcl(Dacl, DaclSize, ACL_REVISION))
    {
        goto Quit;
    }

    if (!AddAccessAllowedAce(Dacl,
                             ACL_REVISION,
                             SYNCHRONIZE | STANDARD_RIGHTS_READ | 0x3,
                             AuthenticatedUsersSid))
    {
        goto Quit;
    }

    if (!AddAccessAllowedAce(Dacl,
                             ACL_REVISION,
                             SEMAPHORE_ALL_ACCESS,
                             SystemSid))
    {
        goto Quit;
    }

    if (!AddAccessAllowedAce(Dacl,
                             ACL_REVISION,
                             SEMAPHORE_ALL_ACCESS,
                             AdminsSid))
    {
        goto Quit;
    }

    if (!SetSecurityDescriptorDacl(&AbsSd, TRUE, Dacl, FALSE))
    {
        goto Quit;
    }

    if (!SetSecurityDescriptorOwner(&AbsSd, AdminsSid, FALSE))
    {
        goto Quit;
    }

    if (!SetSecurityDescriptorGroup(&AbsSd, SystemSid, FALSE))
    {
        goto Quit;
    }

    if (!MakeSelfRelativeSD(&AbsSd, NULL, &RelSDSize) && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        RelSd = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, RelSDSize);
        if (RelSd == NULL)
        {
            goto Quit;
        }

        if (!MakeSelfRelativeSD(&AbsSd, RelSd, &RelSDSize))
        {
            goto Quit;
        }
    }

    *PowrProfSd = RelSd;
    Success = TRUE;

Quit:
    if (AuthenticatedUsersSid)
    {
        FreeSid(AuthenticatedUsersSid);
    }

    if (SystemSid)
    {
        FreeSid(SystemSid);
    }

    if (AdminsSid)
    {
        FreeSid(AdminsSid);
    }

    if (Dacl)
    {
        HeapFree(GetProcessHeap(), 0, Dacl);
    }

    if (!Success)
    {
        if (RelSd)
        {
            HeapFree(GetProcessHeap(), 0, RelSd);
        }
    }

    return Success;
}

static VOID
FixSystemPowerState(PSYSTEM_POWER_STATE Psps, SYSTEM_POWER_CAPABILITIES PowerCaps)
{
	//Lohnegrim: If the System doesn't support the Powerstates, then we have to change them
    if (!PowerCaps.SystemS1 && *Psps == PowerSystemSleeping1)
        *Psps = PowerSystemSleeping2;
    if (!PowerCaps.SystemS2 && *Psps == PowerSystemSleeping2)
        *Psps = PowerSystemSleeping3;
    if (!PowerCaps.SystemS3 && *Psps == PowerSystemSleeping3)
        *Psps = PowerSystemHibernate;
    if (!(PowerCaps.SystemS4 && PowerCaps.HiberFilePresent) && *Psps == PowerSystemHibernate)
        *Psps = PowerSystemSleeping2;
    if (!PowerCaps.SystemS1 && *Psps == PowerSystemSleeping1)
        *Psps = PowerSystemSleeping2;
    if (!PowerCaps.SystemS2 && *Psps == PowerSystemSleeping2)
        *Psps = PowerSystemSleeping3;
    if (!PowerCaps.SystemS3 && *Psps == PowerSystemSleeping3)
        *Psps = PowerSystemShutdown;

}

BOOLEAN WINAPI
ValidatePowerPolicies(PGLOBAL_POWER_POLICY pGPP, PPOWER_POLICY pPP)
{
    SYSTEM_POWER_CAPABILITIES PowerCaps;
    NTSTATUS ret;
    BOOLEAN old;

    RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE, FALSE, &old);
    ret = NtPowerInformation(SystemPowerCapabilities, NULL, 0, &PowerCaps, sizeof(PowerCaps));
    if (ret != STATUS_SUCCESS)
    {
        SetLastError(RtlNtStatusToDosError(ret));
        return FALSE;
    }

    if (pGPP)
    {
        if (pGPP->user.Revision != 1 || pGPP->mach.Revision != 1)
        {
            SetLastError(ERROR_REVISION_MISMATCH);
            return FALSE;
        }
        if (pGPP->mach.LidOpenWakeAc == PowerSystemUnspecified)
        {
            SetLastError(ERROR_GEN_FAILURE);
            return FALSE;
        }
        if ((int)pGPP->mach.LidOpenWakeAc > PowerSystemShutdown)
        {
            SetLastError(ERROR_GEN_FAILURE);
            return FALSE;
        }
        if (pGPP->mach.LidOpenWakeDc < PowerSystemWorking)
        {
            SetLastError(ERROR_GEN_FAILURE);
            return FALSE;
        }
        if ((int)pGPP->mach.LidOpenWakeDc > PowerSystemShutdown)
        {
            SetLastError(ERROR_GEN_FAILURE);
            return FALSE;
        }
		//Lohnegrim: unneeded
        /*if ((pGPP->mach.LidOpenWakeDc < PowerSystemWorking) || (pGPP->mach.LidOpenWakeDc >= PowerSystemMaximum))
        {
            SetLastError(ERROR_GEN_FAILURE);
            return FALSE;
        }*/
        if (!CheckPowerActionPolicy(&pGPP->user.LidCloseAc,PowerCaps))
        {
            return FALSE;
        }
        if (!CheckPowerActionPolicy(&pGPP->user.LidCloseDc,PowerCaps))
        {
            return FALSE;
        }
        if (!CheckPowerActionPolicy(&pGPP->user.PowerButtonAc,PowerCaps))
        {
            return FALSE;
        }
        if (!CheckPowerActionPolicy(&pGPP->user.PowerButtonDc,PowerCaps))
        {
            return FALSE;
        }
        if (!CheckPowerActionPolicy(&pGPP->user.SleepButtonAc,PowerCaps))
        {
            return FALSE;
        }
        if (!CheckPowerActionPolicy(&pGPP->user.SleepButtonDc,PowerCaps))
        {
            return FALSE;
        }
        //Lohnegrim: The BroadcastCapacityResolution presents the Powerlevel in Percent, if invalid set th 100 == FULL
        if (pGPP->mach.BroadcastCapacityResolution > 100)
            pGPP->mach.BroadcastCapacityResolution = 100;

		//Lohnegrim: I have no idea, if they are really needed, or if they are specific for my System, or what they mean, so I removed them
        //pGPP->user.DischargePolicy[1].PowerPolicy.EventCode = pGPP->user.DischargePolicy[1].PowerPolicy.EventCode | 0x010000;
        //pGPP->user.DischargePolicy[2].PowerPolicy.EventCode = pGPP->user.DischargePolicy[2].PowerPolicy.EventCode | 0x020000;
        //pGPP->user.DischargePolicy[3].PowerPolicy.EventCode = pGPP->user.DischargePolicy[3].PowerPolicy.EventCode | 0x030000;

        FixSystemPowerState(&pGPP->mach.LidOpenWakeAc,PowerCaps);
        FixSystemPowerState(&pGPP->mach.LidOpenWakeDc,PowerCaps);

    }

    if (pPP)
    {
        if (pPP->user.Revision != 1 || pPP->mach.Revision != 1)
        {
            SetLastError(ERROR_REVISION_MISMATCH);
            return FALSE;
        }

		//Lohnegrim: unneeded
        //if (pPP->mach.MinSleepAc < PowerSystemWorking)
        //{
        //    SetLastError(ERROR_GEN_FAILURE);
        //    return FALSE;
        //}
        if ((int)pPP->mach.MinSleepAc >= PowerSystemShutdown)
        {
            SetLastError(ERROR_GEN_FAILURE);
            return FALSE;
        }
		//Lohnegrim: unneeded
        //if (pPP->mach.MinSleepDc < PowerSystemWorking)
        //{
        //    SetLastError(ERROR_GEN_FAILURE);
        //    return FALSE;
        //}
        if ((int)pPP->mach.MinSleepDc >= PowerSystemShutdown)
        {
            SetLastError(ERROR_GEN_FAILURE);
            return FALSE;
        }
        if ((int)pPP->mach.ReducedLatencySleepAc == PowerSystemUnspecified)
        {
            SetLastError(ERROR_GEN_FAILURE);
            return FALSE;
        }
        if ((int)pPP->mach.ReducedLatencySleepAc > PowerSystemShutdown)
        {
            SetLastError(ERROR_GEN_FAILURE);
            return FALSE;
        }
        if ((int)pPP->mach.ReducedLatencySleepDc < PowerSystemWorking)
        {
            SetLastError(ERROR_GEN_FAILURE);
            return FALSE;
        }
        if ((int)pPP->mach.ReducedLatencySleepDc > PowerSystemShutdown)
        {
            SetLastError(ERROR_GEN_FAILURE);
            return FALSE;
        }

        if (!CheckPowerActionPolicy(&pPP->mach.OverThrottledAc,PowerCaps))
        {
            return FALSE;
        }
        if (!CheckPowerActionPolicy(&pPP->mach.OverThrottledDc,PowerCaps))
        {
            return FALSE;
        }
        if (!CheckPowerActionPolicy(&pPP->user.IdleAc,PowerCaps))
        {
            return FALSE;
        }
        if (!CheckPowerActionPolicy(&pPP->user.IdleDc,PowerCaps))
        {
            return FALSE;
        }
        if (pPP->user.MaxSleepAc < PowerSystemWorking)
        {
            SetLastError(ERROR_GEN_FAILURE);
            return FALSE;
        }
		//Lohnegrim: unneeded
        /*if ((int)pPP->user.MaxSleepAc > PowerSystemShutdown)
        {
            SetLastError(ERROR_GEN_FAILURE);
            return FALSE;
        }*/
        if (pPP->user.MaxSleepDc < PowerSystemWorking)
        {
            SetLastError(ERROR_GEN_FAILURE);
            return FALSE;
        }
		//Lohnegrim: unneeded
        /*if ((int)pPP->user.MaxSleepDc >= PowerSystemShutdown)
        {
            SetLastError(ERROR_GEN_FAILURE);
            return FALSE;
        }*/
        if (PowerCaps.SystemS1)
        {
            pPP->mach.MinSleepAc=PowerSystemSleeping1;
            pPP->mach.MinSleepDc=PowerSystemSleeping1;
        }
        else if (PowerCaps.SystemS2)
        {
            pPP->mach.MinSleepAc=PowerSystemSleeping2;
            pPP->mach.MinSleepDc=PowerSystemSleeping2;
        }
        else if (PowerCaps.SystemS3)
        {
            pPP->mach.MinSleepAc=PowerSystemSleeping3;
            pPP->mach.MinSleepDc=PowerSystemSleeping3;
        }

        if (PowerCaps.SystemS4)
        {
            pPP->user.MaxSleepAc=PowerSystemSleeping3;
            pPP->user.MaxSleepDc=PowerSystemSleeping3;
        }
        else if (PowerCaps.SystemS3)
        {
            pPP->user.MaxSleepAc=PowerSystemSleeping2;
            pPP->user.MaxSleepDc=PowerSystemSleeping2;
        }
        else if (PowerCaps.SystemS1)
        {
            pPP->user.MaxSleepAc=PowerSystemSleeping1;
            pPP->user.MaxSleepDc=PowerSystemSleeping1;
        }
		//Lohnegrim: I don't know where to get this info from, so I removed it
        //pPP->user.OptimizeForPowerAc=TRUE;
        //pPP->user.OptimizeForPowerDc=TRUE;

        FixSystemPowerState(&pPP->mach.ReducedLatencySleepAc,PowerCaps);
        FixSystemPowerState(&pPP->mach.ReducedLatencySleepDc,PowerCaps);
    }

    SetLastError(ERROR_SUCCESS);
    return TRUE;
}

BOOLEAN WINAPI WritePwrPolicy(PUINT puiID, PPOWER_POLICY pPowerPolicy)
{
    WCHAR Buf[MAX_PATH];
    HKEY hKey;

    swprintf(Buf, L"Control Panel\\PowerCfg\\PowerPolicies\\%i", *puiID);

    if (RegCreateKey(HKEY_CURRENT_USER, Buf, &hKey) != ERROR_SUCCESS)
        return FALSE;

    RegSetValueExW(hKey, szPolicies, 0, REG_BINARY, (const unsigned char *)&pPowerPolicy->user, sizeof(USER_POWER_POLICY));
    RegCloseKey(hKey);

    swprintf(Buf, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Controls Folder\\PowerCfg\\PowerPolicies\\%i", *puiID);

    if (RegCreateKey(HKEY_LOCAL_MACHINE, Buf, &hKey) != ERROR_SUCCESS)
        return FALSE;

    RegSetValueExW(hKey, szPolicies, 0, REG_BINARY, (const unsigned char *)&pPowerPolicy->mach, sizeof(MACHINE_POWER_POLICY));
    RegCloseKey(hKey);

    return TRUE;
}

BOOL WINAPI
DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch(fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            HKEY hKey;
            LONG Err;
            SECURITY_ATTRIBUTES SecAttrs;
            PSECURITY_DESCRIPTOR Sd;

            DisableThreadLibraryCalls(hinstDLL);

            Err = RegOpenKeyExW(HKEY_LOCAL_MACHINE, szPowerCfgSubKey, 0, KEY_READ, &hKey);

            if (Err != ERROR_SUCCESS)
            {
                TRACE("Couldn't open registry key HKLM\\%s, using some sane(?) defaults\n", debugstr_w(szPowerCfgSubKey));
            }
            else
            {
                WCHAR lpValue[MAX_PATH];
                DWORD cbValue = sizeof(lpValue);

                Err = RegQueryValueExW(hKey, szLastID, 0, 0, (BYTE*)lpValue, &cbValue);
                if (Err == ERROR_SUCCESS)
                {
                    g_LastID = _wtoi(lpValue);
                }
                else
                {
                    TRACE("Couldn't open registry entry HKLM\\%s\\LastID, using some sane(?) defaults\n", debugstr_w(szPowerCfgSubKey));
                }
                RegCloseKey(hKey);
            }

            if (!CreatePowrProfSemaphoreSecurity(&Sd))
            {
                ERR("Couldn't create POWRPROF semaphore security descriptor!\n");
                return FALSE;
            }

            SecAttrs.nLength = sizeof(SECURITY_ATTRIBUTES);
            SecAttrs.lpSecurityDescriptor = Sd;
            SecAttrs.bInheritHandle = FALSE;

            PPRegSemaphore = CreateSemaphoreW(&SecAttrs, 1, 1, szSemaphoreName);
            HeapFree(GetProcessHeap(), 0, Sd);
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
