/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

_WdfVersionBuild_

Module Name:

    WdfWMI.h

Abstract:

    This is the C interface for WMI support

Environment:

    kernel mode only

Revision History:

--*/

#ifndef _WDFWMI_H_
#define _WDFWMI_H_



#if (NTDDI_VERSION >= NTDDI_WIN2K)

typedef enum _WDF_WMI_PROVIDER_CONTROL {
    WdfWmiControlInvalid = 0,
    WdfWmiEventControl,
    WdfWmiInstanceControl,
} WDF_WMI_PROVIDER_CONTROL;

// 
// WdfWmiProviderExpensive:
// The provider's operations are expensive in terms of resources
// 
// WdfWmiProviderReadOnly:
// The provider is read only. No set or set data item callbacks will be
// made on all instances of this provider.
// 
// WdfWmiProviderEventOnly:
// Data item is being used to fire events only.  It will not receive any
// callbacks on it to get / set / execute buffers.
// 
typedef enum _WDF_WMI_PROVIDER_FLAGS {
    WdfWmiProviderEventOnly = 0x0001,
    WdfWmiProviderExpensive = 0x0002,
    WdfWmiProviderTracing =   0x0004,
    WdfWmiProviderValidFlags = WdfWmiProviderEventOnly | WdfWmiProviderExpensive | WdfWmiProviderTracing,
} WDF_WMI_PROVIDER_FLAGS;



typedef
__drv_functionClass(EVT_WDF_WMI_INSTANCE_QUERY_INSTANCE)
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
EVT_WDF_WMI_INSTANCE_QUERY_INSTANCE(
    __in
    WDFWMIINSTANCE WmiInstance,
    __in
    ULONG OutBufferSize,
    __out_bcount_part(OutBufferSize, *BufferUsed)
    PVOID OutBuffer,
    __out
    PULONG BufferUsed
    );

typedef EVT_WDF_WMI_INSTANCE_QUERY_INSTANCE *PFN_WDF_WMI_INSTANCE_QUERY_INSTANCE;

typedef
__drv_functionClass(EVT_WDF_WMI_INSTANCE_SET_INSTANCE)
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
EVT_WDF_WMI_INSTANCE_SET_INSTANCE(
    __in
    WDFWMIINSTANCE WmiInstance,
    __in
    ULONG InBufferSize,
    __in_bcount(InBufferSize)
    PVOID InBuffer
    );

typedef EVT_WDF_WMI_INSTANCE_SET_INSTANCE *PFN_WDF_WMI_INSTANCE_SET_INSTANCE;

typedef
__drv_functionClass(EVT_WDF_WMI_INSTANCE_SET_ITEM)
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
EVT_WDF_WMI_INSTANCE_SET_ITEM(
    __in
    WDFWMIINSTANCE WmiInstance,
    __in
    ULONG DataItemId,
    __in
    ULONG InBufferSize,
    __in_bcount(InBufferSize)
    PVOID InBuffer
    );

typedef EVT_WDF_WMI_INSTANCE_SET_ITEM *PFN_WDF_WMI_INSTANCE_SET_ITEM;

typedef
__drv_functionClass(EVT_WDF_WMI_INSTANCE_EXECUTE_METHOD)
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
EVT_WDF_WMI_INSTANCE_EXECUTE_METHOD(
    __in
    WDFWMIINSTANCE WmiInstance,
    __in
    ULONG MethodId,
    __in
    ULONG InBufferSize,
    __in
    ULONG OutBufferSize,
    __drv_when(InBufferSize >= OutBufferSize, __inout_bcount(InBufferSize))
    __drv_when(InBufferSize < OutBufferSize, __inout_bcount(OutBufferSize))
    PVOID Buffer,
    __out
    PULONG BufferUsed
    );

typedef EVT_WDF_WMI_INSTANCE_EXECUTE_METHOD *PFN_WDF_WMI_INSTANCE_EXECUTE_METHOD;

typedef
__drv_functionClass(EVT_WDF_WMI_PROVIDER_FUNCTION_CONTROL)
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
EVT_WDF_WMI_PROVIDER_FUNCTION_CONTROL(
    __in
    WDFWMIPROVIDER WmiProvider,
    __in
    WDF_WMI_PROVIDER_CONTROL Control,
    __in
    BOOLEAN Enable
    );

typedef EVT_WDF_WMI_PROVIDER_FUNCTION_CONTROL *PFN_WDF_WMI_PROVIDER_FUNCTION_CONTROL;

typedef struct _WDF_WMI_PROVIDER_CONFIG {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    //
    // The GUID being registered
    //
    GUID Guid;

    //
    // Combination of values from the enum WDF_WMI_PROVIDER_FLAGS
    //
    ULONG Flags;

    //
    // Minimum expected buffer size for query and set instance requests.
    // Ignored if WdfWmiProviderEventOnly is set in Flags.
    //
    ULONG MinInstanceBufferSize;

    //
    // Callback when caller is opening a provider which ha been marked as
    // expensive or when a caller is interested in events.
    //
    PFN_WDF_WMI_PROVIDER_FUNCTION_CONTROL EvtWmiProviderFunctionControl;

} WDF_WMI_PROVIDER_CONFIG, *PWDF_WMI_PROVIDER_CONFIG;

VOID
FORCEINLINE
WDF_WMI_PROVIDER_CONFIG_INIT(
    __out PWDF_WMI_PROVIDER_CONFIG Config,
    __in CONST GUID* Guid
    )
{
    RtlZeroMemory(Config, sizeof(WDF_WMI_PROVIDER_CONFIG));

    Config->Size = sizeof(WDF_WMI_PROVIDER_CONFIG);
    RtlCopyMemory(&Config->Guid, Guid, sizeof(GUID));
}

typedef struct _WDF_WMI_INSTANCE_CONFIG {
    //
    // Size of the structure in bytes
    //
    ULONG Size;

    //
    // Optional parameter.  If NULL, ProviderConfig must be set to a valid pointer
    // value.   If specified, indicates the provider to create an instance for.
    //
    WDFWMIPROVIDER Provider;

    //
    // Optional parameter.  If NULL, Provider must be set to a valid handle
    // value.  If specifeid, indicates the configuration for a provider to be
    // created and for this instance to be associated with.
    //
    PWDF_WMI_PROVIDER_CONFIG ProviderConfig;

    //
    // If the Provider is configured as read only and this field is set to TRUE,
    // the EvtWmiInstanceQueryInstance is ignored and WDF will blindly copy the
    // context associated with this instance (using RtlCopyMemory, with no locks
    // held) into the query buffer.
    //
    BOOLEAN UseContextForQuery;

    //
    // If TRUE, the instance will be registered as well as created.
    //
    BOOLEAN Register;

    //
    // Callback when caller wants to query the entire data item's buffer.
    //
    PFN_WDF_WMI_INSTANCE_QUERY_INSTANCE EvtWmiInstanceQueryInstance;

    //
    // Callback when caller wants to set the entire data item's buffer.
    //
    PFN_WDF_WMI_INSTANCE_SET_INSTANCE EvtWmiInstanceSetInstance;

    //
    // Callback when caller wants to set a single field in the data item's buffer
    //
    PFN_WDF_WMI_INSTANCE_SET_ITEM EvtWmiInstanceSetItem;

    //
    // Callback when caller wants to execute a method on the data item.
    //
    PFN_WDF_WMI_INSTANCE_EXECUTE_METHOD EvtWmiInstanceExecuteMethod;

} WDF_WMI_INSTANCE_CONFIG, *PWDF_WMI_INSTANCE_CONFIG;

VOID
FORCEINLINE
WDF_WMI_INSTANCE_CONFIG_INIT_PROVIDER(
    __out PWDF_WMI_INSTANCE_CONFIG Config,
    __in WDFWMIPROVIDER Provider
    )
{
    RtlZeroMemory(Config, sizeof(WDF_WMI_INSTANCE_CONFIG));
    Config->Size = sizeof(WDF_WMI_INSTANCE_CONFIG);

    Config->Provider = Provider;
}

VOID
FORCEINLINE
WDF_WMI_INSTANCE_CONFIG_INIT_PROVIDER_CONFIG(
    __out PWDF_WMI_INSTANCE_CONFIG Config,
    __in PWDF_WMI_PROVIDER_CONFIG ProviderConfig
    )
{
    RtlZeroMemory(Config, sizeof(WDF_WMI_INSTANCE_CONFIG));
    Config->Size = sizeof(WDF_WMI_INSTANCE_CONFIG);

    Config->ProviderConfig = ProviderConfig;
}

NTSTATUS
FORCEINLINE
WDF_WMI_BUFFER_APPEND_STRING(
    __out_bcount(BufferLength) PVOID Buffer,
    __in ULONG BufferLength,
    __in PCUNICODE_STRING String,
    __out PULONG RequiredSize
    )
{
    //
    // Compute the length of buffer we need to use.  Upon error the caller can
    // use this length to report the required length.  On success, the caller
    // can use this length to know how many bytes were written.
    //
    *RequiredSize = String->Length + sizeof(USHORT);

    //
    // UNICODE_STRING.Length is the length of the string in bytes, not characters
    //

    // First check to see if there is enough space
    // 1)  to store the length of the string
    // 2)  to store the string itself
    //
    if (BufferLength < (String->Length + sizeof(USHORT))) {
        //
        // Not enough room in the string, report back how big a buffer is
        // required.
        //
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Store the length of the string
    //
    *(USHORT *) Buffer = String->Length;

    //
    // Copy the string to the buffer
    //
    RtlCopyMemory(WDF_PTR_ADD_OFFSET(Buffer, sizeof(USHORT)),
                  String->Buffer,
                  String->Length);

    return STATUS_SUCCESS;
}

//
// WDF Function: WdfWmiProviderCreate
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFWMIPROVIDERCREATE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PWDF_WMI_PROVIDER_CONFIG WmiProviderConfig,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES ProviderAttributes,
    __out
    WDFWMIPROVIDER* WmiProvider
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfWmiProviderCreate(
    __in
    WDFDEVICE Device,
    __in
    PWDF_WMI_PROVIDER_CONFIG WmiProviderConfig,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES ProviderAttributes,
    __out
    WDFWMIPROVIDER* WmiProvider
    )
{
    return ((PFN_WDFWMIPROVIDERCREATE) WdfFunctions[WdfWmiProviderCreateTableIndex])(WdfDriverGlobals, Device, WmiProviderConfig, ProviderAttributes, WmiProvider);
}

//
// WDF Function: WdfWmiProviderGetDevice
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
(*PFN_WDFWMIPROVIDERGETDEVICE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWMIPROVIDER WmiProvider
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFDEVICE
FORCEINLINE
WdfWmiProviderGetDevice(
    __in
    WDFWMIPROVIDER WmiProvider
    )
{
    return ((PFN_WDFWMIPROVIDERGETDEVICE) WdfFunctions[WdfWmiProviderGetDeviceTableIndex])(WdfDriverGlobals, WmiProvider);
}

//
// WDF Function: WdfWmiProviderIsEnabled
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
BOOLEAN
(*PFN_WDFWMIPROVIDERISENABLED)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWMIPROVIDER WmiProvider,
    __in
    WDF_WMI_PROVIDER_CONTROL ProviderControl
    );

__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
FORCEINLINE
WdfWmiProviderIsEnabled(
    __in
    WDFWMIPROVIDER WmiProvider,
    __in
    WDF_WMI_PROVIDER_CONTROL ProviderControl
    )
{
    return ((PFN_WDFWMIPROVIDERISENABLED) WdfFunctions[WdfWmiProviderIsEnabledTableIndex])(WdfDriverGlobals, WmiProvider, ProviderControl);
}

//
// WDF Function: WdfWmiProviderGetTracingHandle
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
ULONGLONG
(*PFN_WDFWMIPROVIDERGETTRACINGHANDLE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWMIPROVIDER WmiProvider
    );

__drv_maxIRQL(DISPATCH_LEVEL)
ULONGLONG
FORCEINLINE
WdfWmiProviderGetTracingHandle(
    __in
    WDFWMIPROVIDER WmiProvider
    )
{
    return ((PFN_WDFWMIPROVIDERGETTRACINGHANDLE) WdfFunctions[WdfWmiProviderGetTracingHandleTableIndex])(WdfDriverGlobals, WmiProvider);
}

//
// WDF Function: WdfWmiInstanceCreate
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFWMIINSTANCECREATE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PWDF_WMI_INSTANCE_CONFIG InstanceConfig,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES InstanceAttributes,
    __out_opt
    WDFWMIINSTANCE* Instance
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfWmiInstanceCreate(
    __in
    WDFDEVICE Device,
    __in
    PWDF_WMI_INSTANCE_CONFIG InstanceConfig,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES InstanceAttributes,
    __out_opt
    WDFWMIINSTANCE* Instance
    )
{
    return ((PFN_WDFWMIINSTANCECREATE) WdfFunctions[WdfWmiInstanceCreateTableIndex])(WdfDriverGlobals, Device, InstanceConfig, InstanceAttributes, Instance);
}

//
// WDF Function: WdfWmiInstanceRegister
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFWMIINSTANCEREGISTER)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWMIINSTANCE WmiInstance
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfWmiInstanceRegister(
    __in
    WDFWMIINSTANCE WmiInstance
    )
{
    return ((PFN_WDFWMIINSTANCEREGISTER) WdfFunctions[WdfWmiInstanceRegisterTableIndex])(WdfDriverGlobals, WmiInstance);
}

//
// WDF Function: WdfWmiInstanceDeregister
//
typedef
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
VOID
(*PFN_WDFWMIINSTANCEDEREGISTER)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWMIINSTANCE WmiInstance
    );

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
FORCEINLINE
WdfWmiInstanceDeregister(
    __in
    WDFWMIINSTANCE WmiInstance
    )
{
    ((PFN_WDFWMIINSTANCEDEREGISTER) WdfFunctions[WdfWmiInstanceDeregisterTableIndex])(WdfDriverGlobals, WmiInstance);
}

//
// WDF Function: WdfWmiInstanceGetDevice
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
(*PFN_WDFWMIINSTANCEGETDEVICE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWMIINSTANCE WmiInstance
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFDEVICE
FORCEINLINE
WdfWmiInstanceGetDevice(
    __in
    WDFWMIINSTANCE WmiInstance
    )
{
    return ((PFN_WDFWMIINSTANCEGETDEVICE) WdfFunctions[WdfWmiInstanceGetDeviceTableIndex])(WdfDriverGlobals, WmiInstance);
}

//
// WDF Function: WdfWmiInstanceGetProvider
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
WDFWMIPROVIDER
(*PFN_WDFWMIINSTANCEGETPROVIDER)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWMIINSTANCE WmiInstance
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFWMIPROVIDER
FORCEINLINE
WdfWmiInstanceGetProvider(
    __in
    WDFWMIINSTANCE WmiInstance
    )
{
    return ((PFN_WDFWMIINSTANCEGETPROVIDER) WdfFunctions[WdfWmiInstanceGetProviderTableIndex])(WdfDriverGlobals, WmiInstance);
}

//
// WDF Function: WdfWmiInstanceFireEvent
//
typedef
__checkReturn
__drv_maxIRQL(APC_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFWMIINSTANCEFIREEVENT)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWMIINSTANCE WmiInstance,
    __in_opt
    ULONG EventDataSize,
    __in_bcount_opt(EventDataSize)
    PVOID EventData
    );

__checkReturn
__drv_maxIRQL(APC_LEVEL)
NTSTATUS
FORCEINLINE
WdfWmiInstanceFireEvent(
    __in
    WDFWMIINSTANCE WmiInstance,
    __in_opt
    ULONG EventDataSize,
    __in_bcount_opt(EventDataSize)
    PVOID EventData
    )
{
    return ((PFN_WDFWMIINSTANCEFIREEVENT) WdfFunctions[WdfWmiInstanceFireEventTableIndex])(WdfDriverGlobals, WmiInstance, EventDataSize, EventData);
}



#endif // (NTDDI_VERSION >= NTDDI_WIN2K)


#endif // _WDFWMI_H_

