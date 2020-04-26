/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

_WdfVersionBuild_

Module Name:

    wdfdriver.h

Abstract:

    This is the interfaces for the Windows Driver Framework Driver object

Environment:

    kernel mode only

Revision History:

--*/

#ifndef _WDFDRIVER_H_
#define _WDFDRIVER_H_



#if (NTDDI_VERSION >= NTDDI_WIN2K)

typedef enum _WDF_DRIVER_INIT_FLAGS {
    WdfDriverInitNonPnpDriver = 0x00000001, //  If set, no Add Device routine is assigned.
    WdfDriverInitNoDispatchOverride = 0x00000002, //  Useful for miniports.
    WdfVerifyOn = 0x00000004, //  Controls whether WDFVERIFY macros are live.
    WdfVerifierOn = 0x00000008, //  Top level verififer flag.
} WDF_DRIVER_INIT_FLAGS;



#define WDF_TRACE_ID ('TRAC')

//
// Callbacks for FxDriver
//

typedef
__drv_functionClass(EVT_WDF_DRIVER_DEVICE_ADD)
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
EVT_WDF_DRIVER_DEVICE_ADD(
    __in
    WDFDRIVER Driver,
    __inout
    PWDFDEVICE_INIT DeviceInit
    );

typedef EVT_WDF_DRIVER_DEVICE_ADD *PFN_WDF_DRIVER_DEVICE_ADD;

typedef
__drv_functionClass(EVT_WDF_DRIVER_UNLOAD)
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
VOID
EVT_WDF_DRIVER_UNLOAD(
    __in
    WDFDRIVER Driver
    );

typedef EVT_WDF_DRIVER_UNLOAD *PFN_WDF_DRIVER_UNLOAD;


//
// Used by WPP Tracing (modeled after WPP's WppTraceCallback (in km-init.tpl))
//
typedef
__drv_functionClass(EVT_WDF_TRACE_CALLBACK)
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
EVT_WDF_TRACE_CALLBACK(
    __in
    UCHAR   minorFunction,
    __in_opt
    PVOID   dataPath,
    __in
    ULONG   bufferLength,
    __inout_bcount(bufferLength)
    PVOID   buffer,
    __in
    PVOID   context,
    __out
    PULONG  size
    );

typedef EVT_WDF_TRACE_CALLBACK *PFN_WDF_TRACE_CALLBACK;

typedef struct _WDF_DRIVER_CONFIG {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    //
    // Event callbacks
    //
    PFN_WDF_DRIVER_DEVICE_ADD EvtDriverDeviceAdd;

    PFN_WDF_DRIVER_UNLOAD    EvtDriverUnload;

    //
    // Combination of WDF_DRIVER_INIT_FLAGS values
    //
    ULONG DriverInitFlags;

    //
    // Pool tag to use for all allocations made by the framework on behalf of
    // the client driver.
    //
    ULONG DriverPoolTag;

} WDF_DRIVER_CONFIG, *PWDF_DRIVER_CONFIG;

VOID
FORCEINLINE
WDF_DRIVER_CONFIG_INIT(
    __out PWDF_DRIVER_CONFIG Config,
    __in_opt PFN_WDF_DRIVER_DEVICE_ADD EvtDriverDeviceAdd
    )
{
    RtlZeroMemory(Config, sizeof(WDF_DRIVER_CONFIG));

    Config->Size = sizeof(WDF_DRIVER_CONFIG);
    Config->EvtDriverDeviceAdd = EvtDriverDeviceAdd;
}

typedef struct _WDF_DRIVER_VERSION_AVAILABLE_PARAMS {
    //
    // Size of the structure in bytes
    //
    ULONG Size;

    //
    // Major Version requested
    //
    ULONG MajorVersion;

    //
    // Minor Version requested
    //
    ULONG MinorVersion;

} WDF_DRIVER_VERSION_AVAILABLE_PARAMS, *PWDF_DRIVER_VERSION_AVAILABLE_PARAMS;

VOID
FORCEINLINE
WDF_DRIVER_VERSION_AVAILABLE_PARAMS_INIT(
    __out PWDF_DRIVER_VERSION_AVAILABLE_PARAMS Params,
    __in ULONG MajorVersion,
    __in ULONG MinorVersion
    )
{
    RtlZeroMemory(Params, sizeof(WDF_DRIVER_VERSION_AVAILABLE_PARAMS));

    Params->Size = sizeof(WDF_DRIVER_VERSION_AVAILABLE_PARAMS);
    Params->MajorVersion = MajorVersion;
    Params->MinorVersion = MinorVersion;
}

WDFDRIVER
FORCEINLINE
WdfGetDriver(
    VOID
    )
{
    return WdfDriverGlobals->Driver;
}

//
// WDF Function: WdfDriverCreate
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFDRIVERCREATE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PDRIVER_OBJECT DriverObject,
    __in
    PCUNICODE_STRING RegistryPath,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES DriverAttributes,
    __in
    PWDF_DRIVER_CONFIG DriverConfig,
    __out_opt
    WDFDRIVER* Driver
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfDriverCreate(
    __in
    PDRIVER_OBJECT DriverObject,
    __in
    PCUNICODE_STRING RegistryPath,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES DriverAttributes,
    __in
    PWDF_DRIVER_CONFIG DriverConfig,
    __out_opt
    WDFDRIVER* Driver
    )
{
    return ((PFN_WDFDRIVERCREATE) WdfFunctions[WdfDriverCreateTableIndex])(WdfDriverGlobals, DriverObject, RegistryPath, DriverAttributes, DriverConfig, Driver);
}

//
// WDF Function: WdfDriverGetRegistryPath
//
typedef
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
PWSTR
(*PFN_WDFDRIVERGETREGISTRYPATH)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDRIVER Driver
    );

__drv_maxIRQL(PASSIVE_LEVEL)
PWSTR
FORCEINLINE
WdfDriverGetRegistryPath(
    __in
    WDFDRIVER Driver
    )
{
    return ((PFN_WDFDRIVERGETREGISTRYPATH) WdfFunctions[WdfDriverGetRegistryPathTableIndex])(WdfDriverGlobals, Driver);
}

//
// WDF Function: WdfDriverWdmGetDriverObject
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
PDRIVER_OBJECT
(*PFN_WDFDRIVERWDMGETDRIVEROBJECT)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDRIVER Driver
    );

__drv_maxIRQL(DISPATCH_LEVEL)
PDRIVER_OBJECT
FORCEINLINE
WdfDriverWdmGetDriverObject(
    __in
    WDFDRIVER Driver
    )
{
    return ((PFN_WDFDRIVERWDMGETDRIVEROBJECT) WdfFunctions[WdfDriverWdmGetDriverObjectTableIndex])(WdfDriverGlobals, Driver);
}

//
// WDF Function: WdfDriverOpenParametersRegistryKey
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFDRIVEROPENPARAMETERSREGISTRYKEY)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDRIVER Driver,
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
WdfDriverOpenParametersRegistryKey(
    __in
    WDFDRIVER Driver,
    __in
    ACCESS_MASK DesiredAccess,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES KeyAttributes,
    __out
    WDFKEY* Key
    )
{
    return ((PFN_WDFDRIVEROPENPARAMETERSREGISTRYKEY) WdfFunctions[WdfDriverOpenParametersRegistryKeyTableIndex])(WdfDriverGlobals, Driver, DesiredAccess, KeyAttributes, Key);
}

//
// WDF Function: WdfWdmDriverGetWdfDriverHandle
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
WDFDRIVER
(*PFN_WDFWDMDRIVERGETWDFDRIVERHANDLE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PDRIVER_OBJECT DriverObject
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFDRIVER
FORCEINLINE
WdfWdmDriverGetWdfDriverHandle(
    __in
    PDRIVER_OBJECT DriverObject
    )
{
    return ((PFN_WDFWDMDRIVERGETWDFDRIVERHANDLE) WdfFunctions[WdfWdmDriverGetWdfDriverHandleTableIndex])(WdfDriverGlobals, DriverObject);
}

//
// WDF Function: WdfDriverRegisterTraceInfo
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFDRIVERREGISTERTRACEINFO)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PDRIVER_OBJECT DriverObject,
    __in
    PFN_WDF_TRACE_CALLBACK EvtTraceCallback,
    __in
    PVOID ControlBlock
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfDriverRegisterTraceInfo(
    __in
    PDRIVER_OBJECT DriverObject,
    __in
    PFN_WDF_TRACE_CALLBACK EvtTraceCallback,
    __in
    PVOID ControlBlock
    )
{
    return ((PFN_WDFDRIVERREGISTERTRACEINFO) WdfFunctions[WdfDriverRegisterTraceInfoTableIndex])(WdfDriverGlobals, DriverObject, EvtTraceCallback, ControlBlock);
}

//
// WDF Function: WdfDriverRetrieveVersionString
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFDRIVERRETRIEVEVERSIONSTRING)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDRIVER Driver,
    __in
    WDFSTRING String
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfDriverRetrieveVersionString(
    __in
    WDFDRIVER Driver,
    __in
    WDFSTRING String
    )
{
    return ((PFN_WDFDRIVERRETRIEVEVERSIONSTRING) WdfFunctions[WdfDriverRetrieveVersionStringTableIndex])(WdfDriverGlobals, Driver, String);
}

//
// WDF Function: WdfDriverIsVersionAvailable
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
BOOLEAN
(*PFN_WDFDRIVERISVERSIONAVAILABLE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDRIVER Driver,
    __in
    PWDF_DRIVER_VERSION_AVAILABLE_PARAMS VersionAvailableParams
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
BOOLEAN
FORCEINLINE
WdfDriverIsVersionAvailable(
    __in
    WDFDRIVER Driver,
    __in
    PWDF_DRIVER_VERSION_AVAILABLE_PARAMS VersionAvailableParams
    )
{
    return ((PFN_WDFDRIVERISVERSIONAVAILABLE) WdfFunctions[WdfDriverIsVersionAvailableTableIndex])(WdfDriverGlobals, Driver, VersionAvailableParams);
}



#endif // (NTDDI_VERSION >= NTDDI_WIN2K)


#endif // _WDFDRIVER_H_

