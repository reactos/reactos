////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////

#include "regtools.h"

#ifndef WIN_32_MODE

NTSTATUS
RegTGetKeyHandle(
    IN HKEY hRootKey,
    IN PCWSTR KeyName,
    OUT HKEY* hKey
    )
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING NameString;
    NTSTATUS status;

    //UDFPrint(("RegTGetKeyHandle: h=%x, %S\n", hRootKey, KeyName));

    RtlInitUnicodeString(&NameString, KeyName);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &NameString,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        hRootKey,
        NULL
        );

    status = ZwOpenKey(
                hKey,
                KEY_WRITE | KEY_READ,
                &ObjectAttributes
                );

    if(!NT_SUCCESS(status)) {
        //UDFPrint(("    status %x\n", status));
        *hKey = NULL;
    }

    return status;
} // end RegTGetKeyHandle()

VOID
RegTCloseKeyHandle(
    IN HKEY hKey
)
{
    ZwClose(hKey);
} // end RegTCloseKeyHandle()

#else //WIN_32_MODE

NTSTATUS
RegTGetKeyHandle(
    IN HKEY hRootKey,
    IN PWCHAR KeyName,
    OUT HKEY* hKey
    )
{
    LONG status;

    if(!hRootKey)
        hRootKey = HKEY_LOCAL_MACHINE;

    status = RegOpenKeyExW(
                hRootKey,
                KeyName,
                0,
                KEY_WRITE | KEY_READ,
                hKey
                );

    if(status != ERROR_SUCCESS) {
        *hKey = NULL;
    }

    return status;
} // end RegTGetKeyHandle()

VOID
RegTCloseKeyHandle(
    IN HKEY hKey
)
{
    if(!hKey) {
        return;
    }
    RegCloseKey(hKey);
} // end RegTCloseKeyHandle()

#endif //WIN_32_MODE

BOOLEAN
RegTGetDwordValue(
    IN HKEY hRootKey,
    IN PCWSTR RegistryPath,
    IN PCWSTR Name,
    IN PULONG pUlong
    )
{
#ifndef WIN_32_MODE
    UNICODE_STRING NameString;
    PKEY_VALUE_PARTIAL_INFORMATION ValInfo;
#endif //WIN_32_MODE
    ULONG len;
    NTSTATUS status;
    HKEY hKey;
    BOOLEAN retval = FALSE;
    BOOLEAN free_h = FALSE;

#ifdef WIN_32_MODE
    if(!hRootKey)
        hRootKey = HKEY_LOCAL_MACHINE;
#endif //WIN_32_MODE

    if(RegistryPath && RegistryPath[0]) {
        status = RegTGetKeyHandle(hRootKey, RegistryPath, &hKey);
#ifdef WIN_32_MODE
        if(status != ERROR_SUCCESS)
#else //WIN_32_MODE
        if(!NT_SUCCESS(status))
#endif //WIN_32_MODE
            return FALSE;
        free_h = TRUE;
    } else {
        hKey = hRootKey;
    }
    if(!hKey)
        return FALSE;

#ifndef WIN_32_MODE
/*
    UDFPrint(("h=%x|%S, %S (%x)\n",
        hRootKey, RegistryPath, Name, *pUlong));
*/
    len = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG) + 0x20;
    ValInfo = (PKEY_VALUE_PARTIAL_INFORMATION)
        MyAllocatePool__(NonPagedPool, len);
    if(!ValInfo) {
        if(free_h) {
            RegTCloseKeyHandle(hKey);
        }
        return FALSE;
    }

    RtlInitUnicodeString(&NameString, Name);

    status = ZwQueryValueKey(hKey,
                             &NameString,
                             KeyValuePartialInformation,
                             ValInfo,
                             len,
                             &len);
    if(NT_SUCCESS(status) &&
       ValInfo->DataLength == sizeof(ULONG)) {
        RtlCopyMemory(pUlong, ValInfo->Data, sizeof(ULONG));
        retval = TRUE;
        //UDFPrint(("  -> %x\n",*pUlong));
    } else {
        //UDFPrint(("  err %x\n",status));
    }

    MyFreePool__(ValInfo);
#else //WIN_32_MODE
    len = sizeof(ULONG);
    if (ERROR_SUCCESS == RegQueryValueExW(
        hKey,               // handle of key to query
        Name,            // address of name of value to query
        0,                  // reserved
        NULL,            // address of buffer for value type
        (BYTE *)pUlong,    // address of data buffer
        &len          // address of data buffer size
        ) && len == sizeof(ULONG)) {
        retval = TRUE;
    }
#endif //WIN_32_MODE
    if(free_h) {
        RegTCloseKeyHandle(hKey);
    }
    return retval;
} // end RegTGetDwordValue()

BOOLEAN
RegTGetStringValue(
    IN HKEY hRootKey,
    IN PCWSTR RegistryPath,
    IN PCWSTR Name,
    IN PWCHAR pStr,
    IN ULONG MaxLen
    )
{
#ifndef WIN_32_MODE
    UNICODE_STRING NameString;
    PKEY_VALUE_PARTIAL_INFORMATION ValInfo;
#endif //USER_MODE
    ULONG len;
    NTSTATUS status;
    HKEY hKey;
    BOOLEAN retval = FALSE;
    BOOLEAN free_h = FALSE;

#ifdef WIN_32_MODE
    if(!hRootKey)
        hRootKey = HKEY_LOCAL_MACHINE;
#endif //WIN_32_MODE

    if(RegistryPath && RegistryPath[0]) {
        status = RegTGetKeyHandle(hRootKey, RegistryPath, &hKey);
#ifdef WIN_32_MODE
        if(status != ERROR_SUCCESS)
#else //WIN_32_MODE
        if(!NT_SUCCESS(status))
#endif //WIN_32_MODE
            return FALSE;
        free_h = TRUE;
    } else {
        hKey = hRootKey;
    }
    if(!hKey)
        return FALSE;

    pStr[0] = 0;
#ifndef WIN_32_MODE
    len = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + MaxLen + 0x20;
    ValInfo = (PKEY_VALUE_PARTIAL_INFORMATION)
        MyAllocatePool__(NonPagedPool, len);
    if(!ValInfo) {
        if(free_h) {
            RegTCloseKeyHandle(hKey);
        }
        return FALSE;
    }

    RtlInitUnicodeString(&NameString, Name);

    status = ZwQueryValueKey(hKey,
                             &NameString,
                             KeyValuePartialInformation,
                             ValInfo,
                             len,
                             &len);
    if(NT_SUCCESS(status) &&
       ValInfo->DataLength) {
        RtlCopyMemory(pStr, ValInfo->Data, min(ValInfo->DataLength, MaxLen) );
        if(pStr[(ValInfo->DataLength)/sizeof(WCHAR)-1]) {
            pStr[(ValInfo->DataLength)/sizeof(WCHAR)-1] = 0;
        }
        retval = TRUE;
    }

    MyFreePool__(ValInfo);
#else //WIN_32_MODE
    len = MaxLen;
    if (ERROR_SUCCESS == RegQueryValueExW(
        hKey,               // handle of key to query
        Name,            // address of name of value to query
        0,                  // reserved
        NULL,            // address of buffer for value type
        (BYTE *)pStr,    // address of data buffer
        &len             // address of data buffer size
        ) && len) {
        if(pStr[len-1]) {
            pStr[len-1] = 0;
        }
        retval = TRUE;
    }
#endif //WIN_32_MODE

    if(free_h) {
        RegTCloseKeyHandle(hKey);
    }
    return retval;
} // end RegTGetStringValue()
