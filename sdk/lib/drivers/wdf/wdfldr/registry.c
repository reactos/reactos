/*
 * PROJECT:     ReactOS WdfLdr driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WdfLdr driver - registry functions
 * COPYRIGHT:   Copyright 2019 Max Korostil <mrmks04@yandex.ru>
 *              Copyright 2021 Victor Perevertkin <victor.perevertkin@reactos.org>
 */

#include "wdfloader.h"


/********************************************
 * 
 * Open framework version registry key
 * 
 * Params:
 *    BindInfo - bind information
 *    HandleRegKey - opened key handle
 * 
 * Result:
 *    Operation status
 * 
*********************************************/
NTSTATUS
NTAPI
GetVersionRegistryHandle(
    _In_ PWDF_BIND_INFO BindInfo,
    _Out_ PHANDLE HandleRegKey)
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE handle = NULL;
    HANDLE keyHandle = NULL;
    DECLARE_UNICODE_STRING_SIZE(string, 11);

    
    status = RtlUnicodeStringPrintf(&string,
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Wdf\\Kmdf\\%s\\Versions",
        BindInfo->Component); // Component name for kmdf driver - 'KmdfLibrary'

    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("ERROR: RtlUnicodeStringPrintf failed with Status 0x%x\n", status));
        goto end;
    }

    __DBGPRINT(("Component path %wZ\n", &string));

    InitializeObjectAttributes(&objectAttributes, &string, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
    status = ZwOpenKey(&keyHandle, KEY_QUERY_VALUE, &objectAttributes);
    
    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("ERROR: ZwOpenKey (%wZ) failed with Status 0x%x\n", &string, status));
        goto end;
    }

    status = RtlIntegerToUnicodeString(BindInfo->Version.Major, 10, &string);
        
    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("ERROR: ConvertUlongToWString failed with Status 0x%x\n", status));
        goto end;
    }

    InitializeObjectAttributes(&objectAttributes, &string, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, keyHandle, NULL);
    status = ZwOpenKey(&handle, KEY_READ, &objectAttributes);
        
    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("ERROR: ZwOpenKey (%wZ) failed with Status 0x%x\n", string, status));
    }

end:
    *HandleRegKey = handle;
    
    if (keyHandle != NULL)
    {
        ZwClose(keyHandle);
    }

    //FreeString(&string);

    return status;
}

/********************************************
 * 
 * Create service path by bind info
 * 
 * Params:
 *    BindInfo - bind information
 *    RegistryPath - created path
 * 
 * Result:
 *    Operation status
 * 
*********************************************/
NTSTATUS
NTAPI
GetDefaultServiceName(
    _In_ PWDF_BIND_INFO BindInfo,
    _Out_ PUNICODE_STRING RegistryPath
)
{
    NTSTATUS status;
    static const SIZE_T regPathLength = sizeof(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Wdf01000") - sizeof(WCHAR);

    if (RegistryPath == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    RegistryPath->Buffer = (PWCHAR)ExAllocatePoolWithTag(PagedPool, regPathLength, WDFLDR_TAG);
    
    if (RegistryPath->Buffer == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        __DBGPRINT(("ERROR: ExAllocatePoolWithTag failed with status 0x%x\n", status));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RegistryPath->Length = 0;
    RegistryPath->MaximumLength = regPathLength;

    status = RtlUnicodeStringPrintf(RegistryPath,
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Wdf%02d000",
        BindInfo->Version.Major);

    if (NT_SUCCESS(status))
    {
        __DBGPRINT(("Couldn't find control Key -- using default service name %wZ\n", RegistryPath));
        status = STATUS_SUCCESS;
    }
    else
    {
        __DBGPRINT(("ERROR: RtlUnicodeStringCopyString failed with Status 0x%x\n", status));
        
        ExFreePoolWithTag(RegistryPath->Buffer, WDFLDR_TAG);
        RegistryPath->Length = 0;
        RegistryPath->Buffer = NULL;
    }
    
    return status;
}

/********************************************
 * 
 * Create service path by bind info
 * 
 * Params:
 *    BindInfo - bind information
 *    ServicePath - service path in registry
 * 
 * Result:
 *    Operation status
 * 
*********************************************/
NTSTATUS
GetVersionServicePath(
    _In_ PWDF_BIND_INFO BindInfo,
    _Out_ PUNICODE_STRING ServicePath)
{
    NTSTATUS status;
    PKEY_VALUE_PARTIAL_INFORMATION pKeyVal = NULL;
    HANDLE handleRegKey = NULL;
    UNICODE_STRING ValueName = RTL_CONSTANT_STRING(L"Service");

    status = GetVersionRegistryHandle(BindInfo, &handleRegKey);

    if (!NT_SUCCESS(status))
    {
        __DBGPRINT(("ERROR: GetVersionRegistryHandle failed with Status 0x%x\n", status));
    }
    else
    {
        // get service name
        status = FxLdrQueryData(handleRegKey, &ValueName, WDFLDR_TAG, &pKeyVal);
        
        if (!NT_SUCCESS(status))
        {
            __DBGPRINT(("ERROR: QueryData failed with status 0x%x\n", status));
        }
        else
        {            
            status = BuildServicePath(pKeyVal, ServicePath);
        }
    }

    if (!NT_SUCCESS(status))
    {
        status = GetDefaultServiceName(BindInfo, ServicePath);
        
        if (!NT_SUCCESS(status))
        {
            __DBGPRINT(("ERROR: GetVersionServicePath failed with Status 0x%x\n", status));
        }
    }
    else
    {
        __DBGPRINT(("(%wZ)\n", ServicePath));
    }

    if (handleRegKey != NULL)
        ZwClose(handleRegKey);
    if (pKeyVal != NULL)
        ExFreePoolWithTag(pKeyVal, WDFLDR_TAG);

    return status;
}

BOOLEAN
ServiceCheckBootStart(
    _In_ PUNICODE_STRING Service)
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE keyHandle = NULL;
    BOOLEAN result = FALSE;
    ULONG value;
    UNICODE_STRING valueName = RTL_CONSTANT_STRING(L"Start");

    InitializeObjectAttributes(&objectAttributes, Service, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
    status = ZwOpenKey(&keyHandle, KEY_READ, &objectAttributes);    

    if (status != STATUS_OBJECT_NAME_NOT_FOUND) 
    {
        if (NT_SUCCESS(status)) 
        {
            status = FxLdrQueryUlong(keyHandle, &valueName, &value);
            if (NT_SUCCESS(status))
            {
                result = value == 0;
            }
        }
        else
        {
            __DBGPRINT(("ZwOpenKey(%wZ) failed: %08X\n", Service, status));
        }
    }

    if (keyHandle)
        ZwClose(keyHandle);

    return result;
}


NTSTATUS
FxLdrQueryUlong(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING ValueName,
    _Out_  PULONG Value)
{
    NTSTATUS status;
    ULONG resultLength;
    KEY_VALUE_PARTIAL_INFORMATION keyValue;

    keyValue.DataLength = 0;
    keyValue.TitleIndex = 0;
    keyValue.Type = 0;
    keyValue.Data[0] = 0;
    status = ZwQueryValueKey(KeyHandle, ValueName, KeyValuePartialInformation, &keyValue, sizeof(keyValue), &resultLength);

    if (NT_SUCCESS(status)) 
    {
        if (keyValue.Type != REG_DWORD || keyValue.DataLength != 4) 
        {
            status = STATUS_INVALID_BUFFER_SIZE;
        }
        else 
        {
            *Value = keyValue.Data[0];
            status = STATUS_SUCCESS;
        }
    }
    else 
    {
        __DBGPRINT(("ERROR: ZwQueryValueKey failed with Status 0x%x\n", status));
    }

    return status;
}


NTSTATUS
FxLdrQueryData(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING ValueName,
    _In_ ULONG Tag,
    _Out_  PKEY_VALUE_PARTIAL_INFORMATION* KeyValPartialInfo)
{
    PKEY_VALUE_PARTIAL_INFORMATION pKeyInfo;
    NTSTATUS status;
    ULONG resultLength;

    *KeyValPartialInfo = NULL;
    for (;;)
    {
        status = ZwQueryValueKey(KeyHandle, ValueName, KeyValuePartialInformation, NULL, 0, &resultLength);
        if (status != STATUS_BUFFER_TOO_SMALL) 
        {
            if (!NT_SUCCESS(status)) 
            {
                __DBGPRINT(("ERROR: ZwQueryValueKey failed with status 0x%x\n", status));
            }

            return status;
        }

        status = RtlULongAdd(resultLength, 0xCu, &resultLength);
        if (!NT_SUCCESS(status)) 
        {
            __DBGPRINT(("ERROR: Computing length of data under %wZ failed with status 0x%x\n", ValueName, status));

            return status;
        }

        pKeyInfo = ExAllocatePoolWithTag(PagedPool, resultLength, Tag);

        if (pKeyInfo == NULL)
        {
            break;
        }

        RtlZeroMemory(pKeyInfo, resultLength);
        status = ZwQueryValueKey(
            KeyHandle,
            ValueName,
            KeyValuePartialInformation,
            pKeyInfo,
            resultLength,
            &resultLength);

        if (NT_SUCCESS(status)) 
        {
            *KeyValPartialInfo = pKeyInfo;
            return status;
        }

        ExFreePoolWithTag(pKeyInfo, WDFLDR_TAG);

        if (status != STATUS_BUFFER_TOO_SMALL) 
        {
            __DBGPRINT(("ERROR: ZwQueryValueKey (%wZ) failed with Status 0x%x\n", ValueName, status));

            return status;
        }
    }

    __DBGPRINT(("ERROR: ExAllocatePoolWithTag failed with Status 0x%x\n", STATUS_INSUFFICIENT_RESOURCES));

    return STATUS_INSUFFICIENT_RESOURCES;
}


NTSTATUS
BuildServicePath(
    _In_ PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation,
    _In_ PUNICODE_STRING ServicePath)
{
    NTSTATUS status;
    PWCHAR buffer;
    PWCHAR lastSymbol;
    UNICODE_STRING name;
    CONST WCHAR regPath[] = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\%wZ";


    if (KeyValueInformation->Type != REG_SZ &&
        KeyValueInformation->Type != REG_EXPAND_SZ) 
    {
        status = STATUS_OBJECT_TYPE_MISMATCH;
        __DBGPRINT(("ERROR: BuildServicePath failed with status 0x%x\n", status));
        return status;
    }

    if (KeyValueInformation->DataLength == 0 ||
        KeyValueInformation->DataLength > 0xFFFF) 
    {
        status = STATUS_INVALID_PARAMETER;
        __DBGPRINT(("ERROR: BuildServicePath failed with status 0x%x\n", status));
        return status;
    }

    name.Buffer = (PWCH)KeyValueInformation->Data;
    name.Length = (USHORT)KeyValueInformation->DataLength;
    name.MaximumLength = (USHORT)KeyValueInformation->DataLength;

    lastSymbol = ((wchar_t*)KeyValueInformation->Data) + KeyValueInformation->DataLength / 2;
    if (KeyValueInformation->DataLength >= 2 &&    *lastSymbol == 0) 
    {
        name.Length = (USHORT)KeyValueInformation->DataLength - 2;
    }
        
    buffer = ExAllocatePoolWithTag(PagedPool, name.Length + sizeof(regPath), WDFLDR_TAG);

    if (buffer != NULL)
    {
        ServicePath->Length = 0;
        ServicePath->MaximumLength = name.Length + sizeof(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");//106;
        ServicePath->Buffer = buffer;
        RtlZeroMemory(ServicePath->Buffer, ServicePath->MaximumLength);
        status = RtlUnicodeStringPrintf(ServicePath, regPath, &name);

        if (!NT_SUCCESS(status)) 
        {
            ExFreePoolWithTag(buffer, WDFLDR_TAG);
            ServicePath->Length = 0;
            ServicePath->Buffer = NULL;
        }
    }
    else 
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        __DBGPRINT(("ERROR: ExAllocatePoolWithTag failed with Status 0x%x\n", status));
    }

    return status;
}
