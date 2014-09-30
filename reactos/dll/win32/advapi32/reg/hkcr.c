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
        ERR("Failed to allocate %lu bytes", InfoLength);
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
    _Out_ HKEY* MachineKey)
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

    /* Open the key. */
    ErrorCode = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        SubKeyName,
        0,
        SamDesired,
        MachineKey);

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
    ErrorCode = GetFallbackHKCRKey(hKey, &QueriedKey);
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
    ErrorCode = GetFallbackHKCRKey(hKey, &QueriedKey);
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
