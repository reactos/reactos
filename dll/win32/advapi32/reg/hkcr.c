/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/reg/hkcr.c
 * PURPOSE:         Registry functions - HKEY_CLASSES_ROOT abstraction
 * PROGRAMMER:      Jerôme Gardou (jerome.gardou@reactos.org)
 */

#include <advapi32.h>

#include <ndk/cmfuncs.h>
#include <pseh/pseh2.h>

#include "reg.h"

WINE_DEFAULT_DEBUG_CHANNEL(reg);

static const UNICODE_STRING HKLM_ClassesPath = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\Classes");

static
LONG
GetKeyName(HKEY hKey, PUNICODE_STRING KeyName)
{
    UNICODE_STRING InfoName;
    PKEY_NAME_INFORMATION NameInformation;
    ULONG InfoLength;
    NTSTATUS Status;

    /* Get info length */
    InfoLength = 0;
    Status = NtQueryKey(hKey, KeyNameInformation, NULL, 0, &InfoLength);
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        ERR("NtQueryKey returned unexpected Status: 0x%08x\n", Status);
        return RtlNtStatusToDosError(Status);
    }

    /* Get it for real */
    NameInformation = RtlAllocateHeap(RtlGetProcessHeap(), 0, InfoLength);
    if (NameInformation == NULL)
    {
        ERR("Failed to allocate %lu bytes\n", InfoLength);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Status = NtQueryKey(hKey, KeyNameInformation, NameInformation, InfoLength, &InfoLength);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, NameInformation);
        ERR("NtQueryKey failed: 0x%08x\n", Status);
        return RtlNtStatusToDosError(Status);
    }

    /* Make it a proper UNICODE_STRING */
    InfoName.Length = NameInformation->NameLength;
    InfoName.MaximumLength = NameInformation->NameLength;
    InfoName.Buffer = NameInformation->Name;

    Status = RtlDuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE, &InfoName, KeyName);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, NameInformation);
        ERR("RtlDuplicateUnicodeString failed: 0x%08x\n", Status);
        return RtlNtStatusToDosError(Status);
    }

    RtlFreeHeap(RtlGetProcessHeap(), 0, NameInformation);

    return ERROR_SUCCESS;
}

static
LONG
GetKeySam(
    _In_ HKEY hKey,
    _Out_ REGSAM* RegSam)
{
    NTSTATUS Status;
    OBJECT_BASIC_INFORMATION ObjectInfo;

    Status = NtQueryObject(hKey, ObjectBasicInformation, &ObjectInfo, sizeof(ObjectInfo), NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("NtQueryObject failed, Status %x08x\n", Status);
        return RtlNtStatusToDosError(Status);
    }

    *RegSam = ObjectInfo.GrantedAccess;
    return ERROR_SUCCESS;
}

/*
 * Gets a HKLM key from an HKCU key.
 */
static
LONG
GetFallbackHKCRKey(
    _In_ HKEY hKey,
    _Out_ HKEY* MachineKey,
    _In_ BOOL MustCreate)
{
    UNICODE_STRING KeyName;
    LPWSTR SubKeyName;
    LONG ErrorCode;
    REGSAM SamDesired;

    /* Get the key name */
    ErrorCode = GetKeyName(hKey, &KeyName);
    if (ErrorCode != ERROR_SUCCESS)
        return ErrorCode;

    /* See if we really need a conversion */
    if (RtlPrefixUnicodeString(&HKLM_ClassesPath, &KeyName, TRUE))
    {
        RtlFreeUnicodeString(&KeyName);
        *MachineKey = hKey;
        return ERROR_SUCCESS;
    }

    SubKeyName = KeyName.Buffer + 15; /* 15 == wcslen(L"\\Registry\\User\\") */
    /* Skip the user token */
    while (*SubKeyName++ != L'\\')
    {
        if (!*SubKeyName)
        {
            ERR("Key name %S is invalid!\n", KeyName.Buffer);
            return ERROR_INTERNAL_ERROR;
        }
    }

    /* Use the same access mask than the original key */
    ErrorCode = GetKeySam(hKey, &SamDesired);
    if (ErrorCode != ERROR_SUCCESS)
    {
        RtlFreeUnicodeString(&KeyName);
        return ErrorCode;
    }

    if (MustCreate)
    {
        ErrorCode = RegCreateKeyExW(
            HKEY_LOCAL_MACHINE,
            SubKeyName,
            0,
            NULL,
            0,
            SamDesired,
            NULL,
            MachineKey,
            NULL);
    }
    else
    {
        /* Open the key. */
        ErrorCode = RegOpenKeyExW(
            HKEY_LOCAL_MACHINE,
            SubKeyName,
            0,
            SamDesired,
            MachineKey);
    }

    RtlFreeUnicodeString(&KeyName);

    return ErrorCode;
}

/* Get the HKCU key (if it exists) from an HKCR key */
static
LONG
GetPreferredHKCRKey(
    _In_ HKEY hKey,
    _Out_ HKEY* PreferredKey)
{
    UNICODE_STRING KeyName;
    LPWSTR SubKeyName;
    LONG ErrorCode;
    REGSAM SamDesired;

    /* Get the key name */
    ErrorCode = GetKeyName(hKey, &KeyName);
    if (ErrorCode != ERROR_SUCCESS)
        return ErrorCode;

    /* See if we really need a conversion */
    if (!RtlPrefixUnicodeString(&HKLM_ClassesPath, &KeyName, TRUE))
    {
        RtlFreeUnicodeString(&KeyName);
        *PreferredKey = hKey;
        return ERROR_SUCCESS;
    }

    /* 18 == wcslen(L"\\Registry\\Machine\\") */
    SubKeyName = KeyName.Buffer + 18;

    /* Use the same access mask than the original key */
    ErrorCode = GetKeySam(hKey, &SamDesired);
    if (ErrorCode != ERROR_SUCCESS)
    {
        RtlFreeUnicodeString(&KeyName);
        return ErrorCode;
    }

    /* Open the key. */
    ErrorCode = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        SubKeyName,
        0,
        SamDesired,
        PreferredKey);

    RtlFreeUnicodeString(&KeyName);

    return ErrorCode;
}

/* HKCR version of RegCreateKeyExW. */
LONG
WINAPI
CreateHKCRKey(
    _In_ HKEY hKey,
    _In_ LPCWSTR lpSubKey,
    _In_ DWORD Reserved,
    _In_opt_ LPWSTR lpClass,
    _In_ DWORD dwOptions,
    _In_ REGSAM samDesired,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _Out_ PHKEY phkResult,
    _Out_opt_ LPDWORD lpdwDisposition)
{
    LONG ErrorCode;
    HKEY QueriedKey, TestKey;

    ASSERT(IsHKCRKey(hKey));

    /* Remove the HKCR flag while we're working */
    hKey = (HKEY)(((ULONG_PTR)hKey) & ~0x2);

    ErrorCode = GetPreferredHKCRKey(hKey, &QueriedKey);

    if (ErrorCode == ERROR_FILE_NOT_FOUND)
    {
        /* The current key doesn't exist on HKCU side, so we can only create it in HKLM */
        ErrorCode = RegCreateKeyExW(
            hKey,
            lpSubKey,
            Reserved,
            lpClass,
            dwOptions,
            samDesired,
            lpSecurityAttributes,
            phkResult,
            lpdwDisposition);
        if (ErrorCode == ERROR_SUCCESS)
            MakeHKCRKey(phkResult);
        return ErrorCode;
    }

    if (ErrorCode != ERROR_SUCCESS)
    {
        /* Somehow we failed for another reason (maybe deleted key or whatever) */
        return ErrorCode;
    }

    /* See if the subkey already exists in HKCU. */
    ErrorCode = RegOpenKeyExW(QueriedKey, lpSubKey, 0, READ_CONTROL, &TestKey);
    if (ErrorCode != ERROR_FILE_NOT_FOUND)
    {
        if (ErrorCode == ERROR_SUCCESS)
        {
            /* Great. Close the test one and do the real create operation */
            RegCloseKey(TestKey);
            ErrorCode = RegCreateKeyExW(
                QueriedKey,
                lpSubKey,
                Reserved,
                lpClass,
                dwOptions,
                samDesired,
                lpSecurityAttributes,
                phkResult,
                lpdwDisposition);
            if (ErrorCode == ERROR_SUCCESS)
                MakeHKCRKey(phkResult);
        }
        if (QueriedKey != hKey)
            RegCloseKey(QueriedKey);

        return ERROR_SUCCESS;
    }

    if (QueriedKey != hKey)
        RegCloseKey(QueriedKey);

    /* So we must do the create operation in HKLM, creating the missing parent keys if needed. */
    ErrorCode = GetFallbackHKCRKey(hKey, &QueriedKey, TRUE);
    if (ErrorCode != ERROR_SUCCESS)
        return ErrorCode;

    /* Do the key creation */
    ErrorCode = RegCreateKeyEx(
        QueriedKey,
        lpSubKey,
        Reserved,
        lpClass,
        dwOptions,
        samDesired,
        lpSecurityAttributes,
        phkResult,
        lpdwDisposition);

    if (QueriedKey != hKey)
        RegCloseKey(QueriedKey);

    if (ErrorCode == ERROR_SUCCESS)
        MakeHKCRKey(phkResult);

    return ErrorCode;
}

/* Same as RegOpenKeyExW, but for HKEY_CLASSES_ROOT subkeys */
LONG
WINAPI
OpenHKCRKey(
    _In_ HKEY hKey,
    _In_ LPCWSTR lpSubKey,
    _In_ DWORD ulOptions,
    _In_ REGSAM samDesired,
    _In_ PHKEY phkResult)
{
    HKEY QueriedKey;
    LONG ErrorCode;

    ASSERT(IsHKCRKey(hKey));

    /* Remove the HKCR flag while we're working */
    hKey = (HKEY)(((ULONG_PTR)hKey) & ~0x2);

    ErrorCode = GetPreferredHKCRKey(hKey, &QueriedKey);

    if (ErrorCode == ERROR_FILE_NOT_FOUND)
    {
        /* The key doesn't exist on HKCU side, no chance for a subkey */
        ErrorCode = RegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);
        if (ErrorCode == ERROR_SUCCESS)
            MakeHKCRKey(phkResult);
        return ErrorCode;
    }

    if (ErrorCode != ERROR_SUCCESS)
    {
        /* Somehow we failed for another reason (maybe deleted key or whatever) */
        return ErrorCode;
    }

    /* Try on the HKCU side */
    ErrorCode = RegOpenKeyExW(QueriedKey, lpSubKey, ulOptions, samDesired, phkResult);
    if (ErrorCode == ERROR_SUCCESS)
        MakeHKCRKey(phkResult);

    /* Close it if we must */
    if (QueriedKey != hKey)
    {
        RegCloseKey(QueriedKey);
    }

    /* Anything else than ERROR_FILE_NOT_FOUND means that we found it, even if it is with failures. */
    if (ErrorCode != ERROR_FILE_NOT_FOUND)
        return ErrorCode;

    /* If we're here, we must open from HKLM key. */
    ErrorCode = GetFallbackHKCRKey(hKey, &QueriedKey, FALSE);
    if (ErrorCode != ERROR_SUCCESS)
    {
        /* Maybe the key doesn't exist in the HKLM view */
        return ErrorCode;
    }

    ErrorCode = RegOpenKeyExW(QueriedKey, lpSubKey, ulOptions, samDesired, phkResult);
    if (ErrorCode == ERROR_SUCCESS)
        MakeHKCRKey(phkResult);

    /* Close it if we must */
    if (QueriedKey != hKey)
    {
        RegCloseKey(QueriedKey);
    }

    return ErrorCode;
}

/* HKCR version of RegDeleteKeyExW */
LONG
WINAPI
DeleteHKCRKey(
    _In_ HKEY hKey,
    _In_ LPCWSTR lpSubKey,
    _In_ REGSAM RegSam,
    _In_ DWORD Reserved)
{
    HKEY QueriedKey;
    LONG ErrorCode;

    ASSERT(IsHKCRKey(hKey));

    /* Remove the HKCR flag while we're working */
    hKey = (HKEY)(((ULONG_PTR)hKey) & ~0x2);

    ErrorCode = GetPreferredHKCRKey(hKey, &QueriedKey);

    if (ErrorCode == ERROR_FILE_NOT_FOUND)
    {
        /* The key doesn't exist on HKCU side, no chance for a subkey */
        return RegDeleteKeyExW(hKey, lpSubKey, RegSam, Reserved);
    }

    if (ErrorCode != ERROR_SUCCESS)
    {
        /* Somehow we failed for another reason (maybe deleted key or whatever) */
        return ErrorCode;
    }

    ErrorCode = RegDeleteKeyExW(QueriedKey, lpSubKey, RegSam, Reserved);

    /* Close it if we must */
    if (QueriedKey != hKey)
    {
        /* The original key is on the machine view */
        RegCloseKey(QueriedKey);
    }

    /* Anything else than ERROR_FILE_NOT_FOUND means that we found it, even if it is with failures. */
    if (ErrorCode != ERROR_FILE_NOT_FOUND)
        return ErrorCode;

    /* If we're here, we must open from HKLM key. */
    ErrorCode = GetFallbackHKCRKey(hKey, &QueriedKey, FALSE);
    if (ErrorCode != ERROR_SUCCESS)
    {
        /* Maybe the key doesn't exist in the HKLM view */
        return ErrorCode;
    }

    ErrorCode = RegDeleteKeyExW(QueriedKey, lpSubKey, RegSam, Reserved);

    /* Close it if we must */
    if (QueriedKey != hKey)
    {
        RegCloseKey(QueriedKey);
    }

    return ErrorCode;
}

/* HKCR version of RegQueryValueExW */
LONG
WINAPI
QueryHKCRValue(
    _In_ HKEY hKey,
    _In_ LPCWSTR Name,
    _In_ LPDWORD Reserved,
    _In_ LPDWORD Type,
    _In_ LPBYTE Data,
    _In_ LPDWORD Count)
{
    HKEY QueriedKey;
    LONG ErrorCode;

    ASSERT(IsHKCRKey(hKey));

    /* Remove the HKCR flag while we're working */
    hKey = (HKEY)(((ULONG_PTR)hKey) & ~0x2);

    ErrorCode = GetPreferredHKCRKey(hKey, &QueriedKey);

    if (ErrorCode == ERROR_FILE_NOT_FOUND)
    {
        /* The key doesn't exist on HKCU side, no chance for a value in it */
        return RegQueryValueExW(hKey, Name, Reserved, Type, Data, Count);
    }

    if (ErrorCode != ERROR_SUCCESS)
    {
        /* Somehow we failed for another reason (maybe deleted key or whatever) */
        return ErrorCode;
    }

    ErrorCode = RegQueryValueExW(QueriedKey, Name, Reserved, Type, Data, Count);

    /* Close it if we must */
    if (QueriedKey != hKey)
    {
        /* The original key is on the machine view */
        RegCloseKey(QueriedKey);
    }

    /* Anything else than ERROR_FILE_NOT_FOUND means that we found it, even if it is with failures. */
    if (ErrorCode != ERROR_FILE_NOT_FOUND)
        return ErrorCode;

    /* If we're here, we must open from HKLM key. */
    ErrorCode = GetFallbackHKCRKey(hKey, &QueriedKey, FALSE);
    if (ErrorCode != ERROR_SUCCESS)
    {
        /* Maybe the key doesn't exist in the HKLM view */
        return ErrorCode;
    }

    ErrorCode = RegQueryValueExW(QueriedKey, Name, Reserved, Type, Data, Count);

    /* Close it if we must */
    if (QueriedKey != hKey)
    {
        RegCloseKey(QueriedKey);
    }

    return ErrorCode;
}

/* HKCR version of RegSetValueExW */
LONG
WINAPI
SetHKCRValue(
    _In_ HKEY hKey,
    _In_ LPCWSTR Name,
    _In_ DWORD Reserved,
    _In_ DWORD Type,
    _In_ CONST BYTE* Data,
    _In_ DWORD DataSize)
{
    HKEY QueriedKey;
    LONG ErrorCode;

    ASSERT(IsHKCRKey(hKey));

    /* Remove the HKCR flag while we're working */
    hKey = (HKEY)(((ULONG_PTR)hKey) & ~0x2);

    ErrorCode = GetPreferredHKCRKey(hKey, &QueriedKey);

    if (ErrorCode == ERROR_FILE_NOT_FOUND)
    {
        /* The key doesn't exist on HKCU side, no chance to put a value in it */
        return RegSetValueExW(hKey, Name, Reserved, Type, Data, DataSize);
    }

    if (ErrorCode != ERROR_SUCCESS)
    {
        /* Somehow we failed for another reason (maybe deleted key or whatever) */
        return ErrorCode;
    }

    /* Check if the value already exists in the preferred key */
    ErrorCode = RegQueryValueExW(QueriedKey, Name, NULL, NULL, NULL, NULL);
    if (ErrorCode != ERROR_FILE_NOT_FOUND)
    {
        if (ErrorCode == ERROR_SUCCESS)
        {
            /* Yes, so we have the right to modify it */
            ErrorCode = RegSetValueExW(QueriedKey, Name, Reserved, Type, Data, DataSize);
        }
        if (QueriedKey != hKey)
            RegCloseKey(QueriedKey);
        return ErrorCode;
    }
    if (QueriedKey != hKey)
        RegCloseKey(QueriedKey);

    /* So we must set the value in the HKLM version */
    ErrorCode = GetPreferredHKCRKey(hKey, &QueriedKey);
    if (ErrorCode == ERROR_FILE_NOT_FOUND)
    {
        /* No choice: put this in HKCU */
        return RegSetValueExW(hKey, Name, Reserved, Type, Data, DataSize);
    }
    else if (ErrorCode != ERROR_SUCCESS)
    {
        return ErrorCode;
    }

    ErrorCode = RegSetValueExW(QueriedKey, Name, Reserved, Type, Data, DataSize);

    if (QueriedKey != hKey)
        RegCloseKey(QueriedKey);

    return ErrorCode;
}

/* HKCR version of RegEnumKeyExW */
LONG
WINAPI
EnumHKCRKey(
    _In_ HKEY hKey,
    _In_ DWORD dwIndex,
    _Out_ LPWSTR lpName,
    _Inout_ LPDWORD lpcbName,
    _Reserved_ LPDWORD lpReserved,
    _Out_opt_ LPWSTR lpClass,
    _Inout_opt_ LPDWORD lpcbClass,
    _Out_opt_ PFILETIME lpftLastWriteTime)
{
    HKEY PreferredKey, FallbackKey;
    DWORD NumPreferredSubKeys;
    DWORD MaxFallbackSubKeyLen;
    DWORD FallbackIndex;
    WCHAR* FallbackSubKeyName = NULL;
    LONG ErrorCode;

    ASSERT(IsHKCRKey(hKey));

    /* Remove the HKCR flag while we're working */
    hKey = (HKEY)(((ULONG_PTR)hKey) & ~0x2);

    /* Get the preferred key */
    ErrorCode = GetPreferredHKCRKey(hKey, &PreferredKey);
    if (ErrorCode != ERROR_SUCCESS)
    {
        if (ErrorCode == ERROR_FILE_NOT_FOUND)
        {
            /* Only the HKLM key exists */
            return RegEnumKeyExW(
                hKey,
                dwIndex,
                lpName,
                lpcbName,
                lpReserved,
                lpClass,
                lpcbClass,
                lpftLastWriteTime);
        }
        return ErrorCode;
    }

    /* Get the fallback key */
    ErrorCode = GetFallbackHKCRKey(hKey, &FallbackKey, FALSE);
    if (ErrorCode != ERROR_SUCCESS)
    {
        if (PreferredKey != hKey)
            RegCloseKey(PreferredKey);
        if (ErrorCode == ERROR_FILE_NOT_FOUND)
        {
            /* Only the HKCU key exists */
            return RegEnumKeyExW(
                hKey,
                dwIndex,
                lpName,
                lpcbName,
                lpReserved,
                lpClass,
                lpcbClass,
                lpftLastWriteTime);
        }
        return ErrorCode;
    }

    /* Get some info on the HKCU side */
    ErrorCode = RegQueryInfoKeyW(
        PreferredKey,
        NULL,
        NULL,
        NULL,
        &NumPreferredSubKeys,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL);
    if (ErrorCode != ERROR_SUCCESS)
        goto Exit;

    if (dwIndex < NumPreferredSubKeys)
    {
        /* HKCU side takes precedence */
        ErrorCode = RegEnumKeyExW(
            PreferredKey,
            dwIndex,
            lpName,
            lpcbName,
            lpReserved,
            lpClass,
            lpcbClass,
            lpftLastWriteTime);
        goto Exit;
    }

    /* Here it gets tricky. We must enumerate the values from the HKLM side,
     * without reporting those which are present on the HKCU side */

    /* Squash out the indices from HKCU */
    dwIndex -= NumPreferredSubKeys;

    /* Get some info */
    ErrorCode = RegQueryInfoKeyW(
        FallbackKey,
        NULL,
        NULL,
        NULL,
        NULL,
        &MaxFallbackSubKeyLen,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL);
    if (ErrorCode != ERROR_SUCCESS)
    {
        ERR("Could not query info of key %p (Err: %d)\n", FallbackKey, ErrorCode);
        goto Exit;
    }

    MaxFallbackSubKeyLen++;
    TRACE("Maxfallbacksubkeylen: %d\n", MaxFallbackSubKeyLen);

    /* Allocate our buffer */
    FallbackSubKeyName = RtlAllocateHeap(
        RtlGetProcessHeap(), 0, MaxFallbackSubKeyLen * sizeof(WCHAR));
    if (!FallbackSubKeyName)
    {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    /* We must begin at the very first subkey of the fallback key,
     * and then see if we meet keys that already are in the preferred key.
     * In that case, we must bump dwIndex, as otherwise we would enumerate a key we already
     * saw in a previous call.
     */
    FallbackIndex = 0;
    while (TRUE)
    {
        HKEY PreferredSubKey;
        DWORD FallbackSubkeyLen = MaxFallbackSubKeyLen;

        /* Try enumerating */
        ErrorCode = RegEnumKeyExW(
            FallbackKey,
            FallbackIndex,
            FallbackSubKeyName,
            &FallbackSubkeyLen,
            NULL,
            NULL,
            NULL,
            NULL);
        if (ErrorCode != ERROR_SUCCESS)
        {
            if (ErrorCode != ERROR_NO_MORE_ITEMS)
                ERR("Returning %d.\n", ErrorCode);
            goto Exit;
        }
        FallbackSubKeyName[FallbackSubkeyLen] = L'\0';

        /* See if there is such a value on HKCU side */
        ErrorCode = RegOpenKeyExW(
            PreferredKey,
            FallbackSubKeyName,
            0,
            READ_CONTROL,
            &PreferredSubKey);

        if (ErrorCode == ERROR_SUCCESS)
        {
            RegCloseKey(PreferredSubKey);
            /* So we already enumerated it on HKCU side. */
            dwIndex++;
        }
        else if (ErrorCode != ERROR_FILE_NOT_FOUND)
        {
            ERR("Got error %d while querying for %s on HKCU side.\n", ErrorCode, FallbackSubKeyName);
            goto Exit;
        }

        /* See if we caught up */
        if (FallbackIndex == dwIndex)
            break;

        FallbackIndex++;
    }

    /* We can finally enumerate on the fallback side */
    ErrorCode = RegEnumKeyExW(
        FallbackKey,
        dwIndex,
        lpName,
        lpcbName,
        lpReserved,
        lpClass,
        lpcbClass,
        lpftLastWriteTime);

Exit:
    if (PreferredKey != hKey)
        RegCloseKey(PreferredKey);
    if (FallbackKey != hKey)
        RegCloseKey(FallbackKey);
    if (FallbackSubKeyName)
        RtlFreeHeap(RtlGetProcessHeap(), 0, FallbackSubKeyName);

    return ErrorCode;
}

/* HKCR version of RegEnumValueW */
LONG
WINAPI
EnumHKCRValue(
    _In_ HKEY hKey,
    _In_ DWORD dwIndex,
    _Out_ LPWSTR lpName,
    _Inout_ PDWORD lpcbName,
    _Reserved_ PDWORD lpReserved,
    _Out_opt_ PDWORD lpdwType,
    _Out_opt_ LPBYTE lpData,
    _Inout_opt_ PDWORD lpcbData)
{
    HKEY PreferredKey, FallbackKey;
    DWORD NumPreferredValues;
    DWORD MaxFallbackValueNameLen;
    DWORD FallbackIndex;
    WCHAR* FallbackValueName = NULL;
    LONG ErrorCode;

    ASSERT(IsHKCRKey(hKey));

    /* Remove the HKCR flag while we're working */
    hKey = (HKEY)(((ULONG_PTR)hKey) & ~0x2);

    /* Get the preferred key */
    ErrorCode = GetPreferredHKCRKey(hKey, &PreferredKey);
    if (ErrorCode != ERROR_SUCCESS)
    {
        if (ErrorCode == ERROR_FILE_NOT_FOUND)
        {
            /* Only the HKLM key exists */
            return RegEnumValueW(
                hKey,
                dwIndex,
                lpName,
                lpcbName,
                lpReserved,
                lpdwType,
                lpData,
                lpcbData);
        }
        return ErrorCode;
    }

    /* Get the fallback key */
    ErrorCode = GetFallbackHKCRKey(hKey, &FallbackKey, FALSE);
    if (ErrorCode != ERROR_SUCCESS)
    {
        if (PreferredKey != hKey)
            RegCloseKey(PreferredKey);
        if (ErrorCode == ERROR_FILE_NOT_FOUND)
        {
            /* Only the HKCU key exists */
            return RegEnumValueW(
                hKey,
                dwIndex,
                lpName,
                lpcbName,
                lpReserved,
                lpdwType,
                lpData,
                lpcbData);
        }
        return ErrorCode;
    }

    /* Get some info on the HKCU side */
    ErrorCode = RegQueryInfoKeyW(
        PreferredKey,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        &NumPreferredValues,
        NULL,
        NULL,
        NULL,
        NULL);
    if (ErrorCode != ERROR_SUCCESS)
        goto Exit;

    if (dwIndex < NumPreferredValues)
    {
        /* HKCU side takes precedence */
        return RegEnumValueW(
            PreferredKey,
            dwIndex,
            lpName,
            lpcbName,
            lpReserved,
            lpdwType,
            lpData,
            lpcbData);
        goto Exit;
    }

    /* Here it gets tricky. We must enumerate the values from the HKLM side,
     * without reporting those which are present on the HKCU side */

    /* Squash out the indices from HKCU */
    dwIndex -= NumPreferredValues;

    /* Get some info */
    ErrorCode = RegQueryInfoKeyW(
        FallbackKey,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        &MaxFallbackValueNameLen,
        NULL,
        NULL,
        NULL);
    if (ErrorCode != ERROR_SUCCESS)
    {
        ERR("Could not query info of key %p (Err: %d)\n", FallbackKey, ErrorCode);
        goto Exit;
    }

    MaxFallbackValueNameLen++;
    TRACE("Maxfallbacksubkeylen: %d\n", MaxFallbackValueNameLen);

    /* Allocate our buffer */
    FallbackValueName = RtlAllocateHeap(
        RtlGetProcessHeap(), 0, MaxFallbackValueNameLen * sizeof(WCHAR));
    if (!FallbackValueName)
    {
        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    /* We must begin at the very first subkey of the fallback key,
     * and then see if we meet keys that already are in the preferred key.
     * In that case, we must bump dwIndex, as otherwise we would enumerate a key we already
     * saw in a previous call.
     */
    FallbackIndex = 0;
    while (TRUE)
    {
        DWORD FallbackValueNameLen = MaxFallbackValueNameLen;

        /* Try enumerating */
        ErrorCode = RegEnumValueW(
            FallbackKey,
            FallbackIndex,
            FallbackValueName,
            &FallbackValueNameLen,
            NULL,
            NULL,
            NULL,
            NULL);
        if (ErrorCode != ERROR_SUCCESS)
        {
            if (ErrorCode != ERROR_NO_MORE_ITEMS)
                ERR("Returning %d.\n", ErrorCode);
            goto Exit;
        }
        FallbackValueName[FallbackValueNameLen] = L'\0';

        /* See if there is such a value on HKCU side */
        ErrorCode = RegQueryValueExW(
            PreferredKey,
            FallbackValueName,
            NULL,
            NULL,
            NULL,
            NULL);

        if (ErrorCode == ERROR_SUCCESS)
        {
            /* So we already enumerated it on HKCU side. */
            dwIndex++;
        }
        else if (ErrorCode != ERROR_FILE_NOT_FOUND)
        {
            ERR("Got error %d while querying for %s on HKCU side.\n", ErrorCode, FallbackValueName);
            goto Exit;
        }

        /* See if we caught up */
        if (FallbackIndex == dwIndex)
            break;

        FallbackIndex++;
    }

    /* We can finally enumerate on the fallback side */
    ErrorCode = RegEnumValueW(
        FallbackKey,
        dwIndex,
        lpName,
        lpcbName,
        lpReserved,
        lpdwType,
        lpData,
        lpcbData);

Exit:
    if (PreferredKey != hKey)
        RegCloseKey(PreferredKey);
    if (FallbackKey != hKey)
        RegCloseKey(FallbackKey);
    if (FallbackValueName)
        RtlFreeHeap(RtlGetProcessHeap(), 0, FallbackValueName);

    return ErrorCode;
}
