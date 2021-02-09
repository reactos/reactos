/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxRegKey.cpp

Abstract:

Author:

Environment:

    kernel mode only

Revision History:

--*/

#include "fxsupportpch.hpp"

extern "C" {
// #include "FxRegKeyKM.tmh"
}

#define AT_PASSIVE()     ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL)

FxRegKey::FxRegKey(
    PFX_DRIVER_GLOBALS FxDriverGlobals
    ) :
    FxPagedObject(FX_TYPE_REG_KEY, sizeof(FxRegKey), FxDriverGlobals),
    m_Key(NULL),
    m_Globals(FxDriverGlobals)
{
}

__drv_maxIRQL(PASSIVE_LEVEL)
FxRegKey::~FxRegKey()
{
    if (m_Key != NULL) {
        ZwClose(m_Key);
        m_Key = NULL;
    }
}

__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FxRegKey::_Close(
    __in HANDLE Key
    )
{
    return ZwClose(Key);
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FxRegKey::_Create(
    __in_opt  HANDLE ParentKey,
    __in  PCUNICODE_STRING KeyName,
    __out HANDLE* NewKey,
    __in  ACCESS_MASK DesiredAccess,
    __in  ULONG CreateOptions,
    __out_opt PULONG CreateDisposition
    )
{
    OBJECT_ATTRIBUTES oa;

    AT_PASSIVE();

    //
    // Force OBJ_KERNEL_HANDLE because we are never passing the handle back
    // up to a process and we don't want to create a handle in an arbitrary
    // process from which that process can close the handle out from underneath
    // us.
    //
    InitializeObjectAttributes(
        &oa,
        (PUNICODE_STRING) KeyName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        ParentKey,
        NULL);

    return ZwCreateKey(NewKey,
                       DesiredAccess,
                       &oa,
                       0,
                       0,
                       CreateOptions,
                       CreateDisposition);
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FxRegKey::_OpenKey(
    __in_opt  HANDLE ParentKey,
    __in  PCUNICODE_STRING KeyName,
    __out HANDLE* Key,
    __in  ACCESS_MASK DesiredAccess
    )
{
    OBJECT_ATTRIBUTES oa;

    AT_PASSIVE();

    //
    // Force OBJ_KERNEL_HANDLE because we are never passing the handle back
    // up to a process and we don't want to create a handle in an arbitrary
    // process from which that process can close the handle out from underneath
    // us.
    //
    InitializeObjectAttributes(
        &oa,
        (PUNICODE_STRING)KeyName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        ParentKey,
        NULL);

    return ZwOpenKey(Key, DesiredAccess, &oa);
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FxRegKey::_SetValue(
    _In_ HANDLE Key,
    __in PCUNICODE_STRING ValueName,
    __in ULONG ValueType,
    __in_bcount(ValueLength) PVOID Value,
    __in ULONG ValueLength
    )
{
    AT_PASSIVE();

    return ZwSetValueKey(Key,
                         (PUNICODE_STRING)ValueName,
                         0,
                         ValueType,
                         Value,
                         ValueLength);
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FxRegKey::_QueryValue(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in HANDLE Key,
    __in PCUNICODE_STRING ValueName,
    __in ULONG ValueLength,
    __out_bcount_opt(ValueLength) PVOID Value,
    __out_opt PULONG ValueLengthQueried,
    __out_opt PULONG ValueType
    )
{
    KEY_VALUE_PARTIAL_INFORMATION *pPartial, partial;
    NTSTATUS status;
    ULONG length;

    if (Value == NULL) {
        //
        // Caller wants just the length
        //
        pPartial = &partial;
        length = _ComputePartialSize(0);
        RtlZeroMemory(&partial, length);
    }
    else {
        length = _ComputePartialSize(ValueLength);
        pPartial = (PKEY_VALUE_PARTIAL_INFORMATION)
            MxMemory::MxAllocatePoolWithTag(PagedPool, length, FxDriverGlobals->Tag);

        if (pPartial == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    // We always pass a buffer of at least sizeof(KEY_VALUE_PARTIAL_INFORMATION)
    // to ZwQueryValueKey. This means that ZwQueryValueKey will write at least
    // some information to the buffer it receives, even if the user-supplied data
    // buffer is NULL or too small.
    //
    // According to ZwQueryValueKey's contract, this means that it will never return
    // STATUS_BUFFER_TOO_SMALL (returned when no data is written). Therefore, if the
    // user passes a NULL or insufficient buffer and the value exists in the registry,
    // ZwQueryValueKey will return STATUS_BUFFER_OVERFLOW.
    //
    status = ZwQueryValueKey(Key,
                             (PUNICODE_STRING)ValueName,
                             KeyValuePartialInformation,
                             pPartial,
                             length,
                             &length);

    if (NT_SUCCESS(status) && Value != NULL && (ValueLength >= pPartial->DataLength)) {
        RtlCopyMemory(Value, &pPartial->Data[0], pPartial->DataLength);
    }

    if (NT_SUCCESS(status) || status == STATUS_BUFFER_OVERFLOW) {
        if (ValueLengthQueried != NULL) {
            *ValueLengthQueried = pPartial->DataLength;
        }
        if (ValueType != NULL) {
            *ValueType = pPartial->Type;
        }
    }

    if (pPartial != &partial) {
        MxMemory::MxFreePool(pPartial);
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FxRegKey::_QueryULong(
    __in  HANDLE Key,
    __in  PCUNICODE_STRING ValueName,
    __out PULONG Value
    )
{
    NTSTATUS status;
    ULONG length;

    PKEY_VALUE_PARTIAL_INFORMATION pPartial;
    UCHAR buffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data)+(sizeof(ULONG))];

    length = sizeof(buffer);
    pPartial = (PKEY_VALUE_PARTIAL_INFORMATION) &buffer[0];

    status = ZwQueryValueKey(Key,
                             (PUNICODE_STRING)ValueName,
                             KeyValuePartialInformation,
                             pPartial,
                             length,
                             &length);

    if ((NT_SUCCESS(status) || status == STATUS_BUFFER_OVERFLOW) &&
         pPartial->Type != REG_DWORD) {
        status = STATUS_OBJECT_TYPE_MISMATCH;
    }

    if (NT_SUCCESS(status)) {
        ASSERT(sizeof(ULONG) == pPartial->DataLength);

        RtlCopyMemory(Value, &pPartial->Data[0], sizeof(ULONG));
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FxRegKey::_QueryQuadWord(
    __in  HANDLE Key,
    __in  PCUNICODE_STRING ValueName,
    __out PLARGE_INTEGER Value
    )
{
    NTSTATUS status;
    ULONG length;

    PKEY_VALUE_PARTIAL_INFORMATION pPartial;
    UCHAR buffer[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data)+(sizeof(LARGE_INTEGER))];

    length = sizeof(buffer);
    pPartial = (PKEY_VALUE_PARTIAL_INFORMATION) &buffer[0];

    status = ZwQueryValueKey(Key,
                             (PUNICODE_STRING)ValueName,
                             KeyValuePartialInformation,
                             pPartial,
                             length,
                             &length);

    if ((NT_SUCCESS(status) || status == STATUS_BUFFER_OVERFLOW) &&
         pPartial->Type != REG_QWORD) {
        status = STATUS_OBJECT_TYPE_MISMATCH;
    }

    if (NT_SUCCESS(status)) {
        ASSERT(sizeof(LARGE_INTEGER) == pPartial->DataLength);

        RtlCopyMemory(Value, &pPartial->Data[0], sizeof(LARGE_INTEGER));
    }

    return status;
}


