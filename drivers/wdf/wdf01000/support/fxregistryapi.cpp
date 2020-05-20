/*
 * PROJECT:     ReactOS Wdf01000 driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Registry api functions
 * COPYRIGHT:   Copyright 2020 mrmks04 (mrmks04@yandex.ru)
 */


#include "wdf.h"



extern "C" {

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryOpenKey)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in_opt
    WDFKEY ParentKey,
    __in
    PCUNICODE_STRING KeyName,
    __in
    ACCESS_MASK DesiredAccess,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES KeyAttributes,
    __out
    WDFKEY* Key
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryCreateKey)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in_opt
    WDFKEY ParentKey,
    __in
    PCUNICODE_STRING KeyName,
    __in
    ACCESS_MASK DesiredAccess,
    __in
    ULONG CreateOptions,
    __out_opt
    PULONG CreateDisposition,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES KeyAttributes,
    __out
    WDFKEY* Key
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
WDFEXPORT(WdfRegistryClose)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(PASSIVE_LEVEL)
HANDLE
WDFEXPORT(WdfRegistryWdmGetHandle)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key
    )
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryRemoveKey)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryRemoveValue)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryQueryValue)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    ULONG ValueLength,
    __out_bcount_opt( ValueLength)
    PVOID Value,
     __out_opt
    PULONG ValueLengthQueried,
     __out_opt
    PULONG ValueType
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryQueryMemory)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    __drv_strictTypeMatch( 1)
    POOL_TYPE PoolType,
     __in_opt
    PWDF_OBJECT_ATTRIBUTES MemoryAttributes,
    __out
    WDFMEMORY* Memory,
    __out_opt
    PULONG ValueType
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryQueryMultiString)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES StringsAttributes,
    __in
    WDFCOLLECTION Collection
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryQueryUnicodeString)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __out_opt
    PUSHORT ValueByteLength,
    __inout_opt
    PUNICODE_STRING Value
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryQueryString)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    WDFSTRING String
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryQueryULong)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __out
    PULONG Value
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}


_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryAssignValue)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    ULONG ValueType,
    __in
    ULONG ValueLength,
    __in_ecount( ValueLength)
    PVOID Value
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}


_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryAssignMemory)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    ULONG ValueType,
    __in
    WDFMEMORY Memory,
    __in_opt
    PWDFMEMORY_OFFSET MemoryOffsets
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryAssignULong)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    ULONG Value
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryAssignUnicodeString)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    PCUNICODE_STRING Value
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryAssignString)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    WDFSTRING String
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfRegistryAssignMultiString)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    WDFCOLLECTION StringsCollection
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

} // extern "C"
