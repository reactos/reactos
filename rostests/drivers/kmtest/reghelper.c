/*
 * Driver Regression Tests
 *
 * Copyright 2009 Michael Martin <martinmnet@hotmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; see the file COPYING.LIB.
 * If not, write to the Free Software Foundation,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* INCLUDES *******************************************************************/

#include "kmtest.h"

/*
    Adds a service registry entry for a driver
    The driver must reside in the same path as this loaded driver
    The caller is resposible for releasing memory
*/
PWCHAR CreateLowerDeviceRegistryKey(PUNICODE_STRING RegistryPath, PWCHAR NewDriver)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Name;
    UNICODE_STRING Value;
    UNICODE_STRING NewDriverRegPath;
    PKEY_VALUE_PARTIAL_INFORMATION ValuePartialInfo = NULL;
    HANDLE ServiceKey;
    NTSTATUS Status;
    ULONG Disposition;
    ULONG ServiceDWordValue;
    ULONG ResultLength = 0;
    ULONG Length = 0;
    PWCHAR ReturnPath = NULL;
    /* Now lets find out where we were loaded from by using registry */
    InitializeObjectAttributes(&ObjectAttributes, RegistryPath, OBJ_CASE_INSENSITIVE, NULL, NULL);
    Status = ZwOpenKey(&ServiceKey, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("ZwOpenKey () failed (Status %x)\n", Status);
        return NULL;
    }

    RtlInitUnicodeString(&Name, L"ImagePath");

    /* First query how much memory we need */
    Status = ZwQueryValueKey(ServiceKey, &Name, KeyValuePartialInformation, 0, 0, &ResultLength);

    ResultLength += sizeof(KEY_VALUE_PARTIAL_INFORMATION);
    ValuePartialInfo = ExAllocatePool(PagedPool, ResultLength);
    if (!ValuePartialInfo)
    {
        DbgPrint("Out of memory!\n");
        goto cleanup;
    }

    Length = ResultLength;
    Status = ZwQueryValueKey(ServiceKey, &Name, KeyValuePartialInformation, (PVOID)ValuePartialInfo, Length, &ResultLength);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("ZwQueryValueKey() failed (Status %lx)\n", Status);
        goto cleanup;
    }

    /* Remove the current driver name from the string */
    /* FIXME: Dont use hard coded driver name, determine it from the string returned from the above Query */
    Length = (wcslen((PWCHAR)ValuePartialInfo->Data) * 2) - (wcslen(L"kmtest.sys") * 2);
    RtlZeroMemory((PVOID)((ULONG)ValuePartialInfo->Data + Length),
    wcslen(L"drvtests.sys") * 2);
    ZwClose(ServiceKey);

    /* Now add a registry entry for the driver */

    NewDriverRegPath.Length = 0;
    NewDriverRegPath.MaximumLength = (wcslen(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\") +
                                     wcslen(NewDriver) + 1) * sizeof(WCHAR);
    NewDriverRegPath.Buffer = ExAllocatePool(PagedPool, NewDriverRegPath.MaximumLength);
    if (!NewDriverRegPath.Buffer)
    {
        DbgPrint("Out of memory!\n");
        ExFreePool(NewDriverRegPath.Buffer);
        goto cleanup;
    }

    RtlAppendUnicodeToString(&NewDriverRegPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
    RtlAppendUnicodeToString(&NewDriverRegPath, NewDriver);

    InitializeObjectAttributes(&ObjectAttributes,
                               &NewDriverRegPath,
                               OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                               0,
                               NULL);

    Status = ZwCreateKey(&ServiceKey, 
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         &Disposition);

    if (!NT_SUCCESS(Status))
    {
        DbgPrint("ZwCreateKey() failed (Status %lx)\n", Status);
        ExFreePool(NewDriverRegPath.Buffer);
        goto cleanup;
    }

    ReturnPath = NewDriverRegPath.Buffer;
    RtlInitUnicodeString(&Name, L"ImagePath");

    Value.Length = 0;
    Value.MaximumLength = (wcslen((PWCHAR)ValuePartialInfo->Data) + 
                          wcslen(NewDriver) + 5) * sizeof(WCHAR);
    Value.Buffer = ExAllocatePool(PagedPool, Value.MaximumLength);

    if (!Value.Buffer)
    {
        DbgPrint("Out of memory!\n");
        ExFreePool(Value.Buffer);
        goto cleanup;
    }

    RtlAppendUnicodeToString(&Value, (PWCHAR)ValuePartialInfo->Data);
    RtlAppendUnicodeToString(&Value, NewDriver);
    RtlAppendUnicodeToString(&Value, L".sys");

    Status = ZwSetValueKey(ServiceKey,
                           &Name,
                           0,
                           REG_SZ,
                           Value.Buffer,
                           (wcslen(Value.Buffer)+1) * sizeof(WCHAR));
    ExFreePool(Value.Buffer);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("ZwCreateKey() failed (Status %lx)\n", Status);
        goto cleanup;
    }

    RtlInitUnicodeString(&Name, L"DisplayName");
    RtlInitUnicodeString(&Value, NewDriver);

    Status = ZwSetValueKey(ServiceKey,
                           &Name,
                           0,
                           REG_SZ,
                           Value.Buffer,
                           (wcslen(Value.Buffer)+1) * sizeof(WCHAR));

    if (!NT_SUCCESS(Status))
    {
        DbgPrint("ZwCreateKey() failed (Status %lx)\n", Status);
        goto cleanup;
    }

    RtlInitUnicodeString(&Name, L"ErrorControl");
    ServiceDWordValue = 0;

    Status = ZwSetValueKey(ServiceKey,
                           &Name,
                           0,
                           REG_DWORD,
                           &ServiceDWordValue,
                           sizeof(ULONG));

    if (!NT_SUCCESS(Status))
    {
        DbgPrint("ZwCreateKey() failed (Status %lx)\n", Status);
        goto cleanup;
    }

    RtlInitUnicodeString(&Name, L"Start");
    ServiceDWordValue = 3;
    Status = ZwSetValueKey(ServiceKey,
                           &Name,
                           0,
                           REG_DWORD,
                           &ServiceDWordValue,
                           sizeof(ULONG));

    if (!NT_SUCCESS(Status))
    {
        DbgPrint("ZwCreateKey() failed (Status %lx)\n", Status);
        goto cleanup;
    }

    RtlInitUnicodeString(&Name, L"Type");
    ServiceDWordValue = 0;
    Status = ZwSetValueKey(ServiceKey,
                           &Name,
                           0,
                           REG_DWORD,
                           &ServiceDWordValue,
                           sizeof(ULONG));

    if (!NT_SUCCESS(Status))
    {
        DbgPrint("ZwCreateKey() failed (Status %lx)\n", Status);
        goto cleanup;
    }

cleanup:
    ZwClose(ServiceKey);
    if (ValuePartialInfo) ExFreePool(ValuePartialInfo);

    return ReturnPath;

}
