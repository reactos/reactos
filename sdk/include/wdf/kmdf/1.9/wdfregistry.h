/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

_WdfVersionBuild_

Module Name:

    WdfRegistry.h

Abstract:

    This is the interface to registry access.

Environment:

    kernel mode only

Revision History:

--*/

#ifndef _WDFREGISTRY_H_
#define _WDFREGISTRY_H_



#if (NTDDI_VERSION >= NTDDI_WIN2K)



//
// WDF Function: WdfRegistryOpenKey
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFREGISTRYOPENKEY)(
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
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfRegistryOpenKey(
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
    return ((PFN_WDFREGISTRYOPENKEY) WdfFunctions[WdfRegistryOpenKeyTableIndex])(WdfDriverGlobals, ParentKey, KeyName, DesiredAccess, KeyAttributes, Key);
}

//
// WDF Function: WdfRegistryCreateKey
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFREGISTRYCREATEKEY)(
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
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfRegistryCreateKey(
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
    return ((PFN_WDFREGISTRYCREATEKEY) WdfFunctions[WdfRegistryCreateKeyTableIndex])(WdfDriverGlobals, ParentKey, KeyName, DesiredAccess, CreateOptions, CreateDisposition, KeyAttributes, Key);
}

//
// WDF Function: WdfRegistryClose
//
typedef
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
VOID
(*PFN_WDFREGISTRYCLOSE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key
    );

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
FORCEINLINE
WdfRegistryClose(
    __in
    WDFKEY Key
    )
{
    ((PFN_WDFREGISTRYCLOSE) WdfFunctions[WdfRegistryCloseTableIndex])(WdfDriverGlobals, Key);
}

//
// WDF Function: WdfRegistryWdmGetHandle
//
typedef
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
HANDLE
(*PFN_WDFREGISTRYWDMGETHANDLE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key
    );

__drv_maxIRQL(PASSIVE_LEVEL)
HANDLE
FORCEINLINE
WdfRegistryWdmGetHandle(
    __in
    WDFKEY Key
    )
{
    return ((PFN_WDFREGISTRYWDMGETHANDLE) WdfFunctions[WdfRegistryWdmGetHandleTableIndex])(WdfDriverGlobals, Key);
}

//
// WDF Function: WdfRegistryRemoveKey
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFREGISTRYREMOVEKEY)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfRegistryRemoveKey(
    __in
    WDFKEY Key
    )
{
    return ((PFN_WDFREGISTRYREMOVEKEY) WdfFunctions[WdfRegistryRemoveKeyTableIndex])(WdfDriverGlobals, Key);
}

//
// WDF Function: WdfRegistryRemoveValue
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFREGISTRYREMOVEVALUE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfRegistryRemoveValue(
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName
    )
{
    return ((PFN_WDFREGISTRYREMOVEVALUE) WdfFunctions[WdfRegistryRemoveValueTableIndex])(WdfDriverGlobals, Key, ValueName);
}

//
// WDF Function: WdfRegistryQueryValue
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFREGISTRYQUERYVALUE)(
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
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfRegistryQueryValue(
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
    return ((PFN_WDFREGISTRYQUERYVALUE) WdfFunctions[WdfRegistryQueryValueTableIndex])(WdfDriverGlobals, Key, ValueName, ValueLength, Value, ValueLengthQueried, ValueType);
}

//
// WDF Function: WdfRegistryQueryMemory
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFREGISTRYQUERYMEMORY)(
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
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfRegistryQueryMemory(
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
    return ((PFN_WDFREGISTRYQUERYMEMORY) WdfFunctions[WdfRegistryQueryMemoryTableIndex])(WdfDriverGlobals, Key, ValueName, PoolType, MemoryAttributes, Memory, ValueType);
}

//
// WDF Function: WdfRegistryQueryMultiString
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFREGISTRYQUERYMULTISTRING)(
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
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfRegistryQueryMultiString(
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
    return ((PFN_WDFREGISTRYQUERYMULTISTRING) WdfFunctions[WdfRegistryQueryMultiStringTableIndex])(WdfDriverGlobals, Key, ValueName, StringsAttributes, Collection);
}

//
// WDF Function: WdfRegistryQueryUnicodeString
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFREGISTRYQUERYUNICODESTRING)(
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
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfRegistryQueryUnicodeString(
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
    return ((PFN_WDFREGISTRYQUERYUNICODESTRING) WdfFunctions[WdfRegistryQueryUnicodeStringTableIndex])(WdfDriverGlobals, Key, ValueName, ValueByteLength, Value);
}

//
// WDF Function: WdfRegistryQueryString
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFREGISTRYQUERYSTRING)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    WDFSTRING String
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfRegistryQueryString(
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    WDFSTRING String
    )
{
    return ((PFN_WDFREGISTRYQUERYSTRING) WdfFunctions[WdfRegistryQueryStringTableIndex])(WdfDriverGlobals, Key, ValueName, String);
}

//
// WDF Function: WdfRegistryQueryULong
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFREGISTRYQUERYULONG)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __out
    PULONG Value
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfRegistryQueryULong(
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __out
    PULONG Value
    )
{
    return ((PFN_WDFREGISTRYQUERYULONG) WdfFunctions[WdfRegistryQueryULongTableIndex])(WdfDriverGlobals, Key, ValueName, Value);
}

//
// WDF Function: WdfRegistryAssignValue
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFREGISTRYASSIGNVALUE)(
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
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfRegistryAssignValue(
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
    return ((PFN_WDFREGISTRYASSIGNVALUE) WdfFunctions[WdfRegistryAssignValueTableIndex])(WdfDriverGlobals, Key, ValueName, ValueType, ValueLength, Value);
}

//
// WDF Function: WdfRegistryAssignMemory
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFREGISTRYASSIGNMEMORY)(
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
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfRegistryAssignMemory(
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
    return ((PFN_WDFREGISTRYASSIGNMEMORY) WdfFunctions[WdfRegistryAssignMemoryTableIndex])(WdfDriverGlobals, Key, ValueName, ValueType, Memory, MemoryOffsets);
}

//
// WDF Function: WdfRegistryAssignMultiString
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFREGISTRYASSIGNMULTISTRING)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    WDFCOLLECTION StringsCollection
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfRegistryAssignMultiString(
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    WDFCOLLECTION StringsCollection
    )
{
    return ((PFN_WDFREGISTRYASSIGNMULTISTRING) WdfFunctions[WdfRegistryAssignMultiStringTableIndex])(WdfDriverGlobals, Key, ValueName, StringsCollection);
}

//
// WDF Function: WdfRegistryAssignUnicodeString
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFREGISTRYASSIGNUNICODESTRING)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    PCUNICODE_STRING Value
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfRegistryAssignUnicodeString(
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    PCUNICODE_STRING Value
    )
{
    return ((PFN_WDFREGISTRYASSIGNUNICODESTRING) WdfFunctions[WdfRegistryAssignUnicodeStringTableIndex])(WdfDriverGlobals, Key, ValueName, Value);
}

//
// WDF Function: WdfRegistryAssignString
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFREGISTRYASSIGNSTRING)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    WDFSTRING String
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfRegistryAssignString(
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    WDFSTRING String
    )
{
    return ((PFN_WDFREGISTRYASSIGNSTRING) WdfFunctions[WdfRegistryAssignStringTableIndex])(WdfDriverGlobals, Key, ValueName, String);
}

//
// WDF Function: WdfRegistryAssignULong
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFREGISTRYASSIGNULONG)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    ULONG Value
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfRegistryAssignULong(
    __in
    WDFKEY Key,
    __in
    PCUNICODE_STRING ValueName,
    __in
    ULONG Value
    )
{
    return ((PFN_WDFREGISTRYASSIGNULONG) WdfFunctions[WdfRegistryAssignULongTableIndex])(WdfDriverGlobals, Key, ValueName, Value);
}



#endif // (NTDDI_VERSION >= NTDDI_WIN2K)


#endif // _WDFREGISTRY_H_

