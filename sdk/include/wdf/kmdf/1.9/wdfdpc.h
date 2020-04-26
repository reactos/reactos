/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

_WdfVersionBuild_

Module Name:

    wdfdpc.h

Abstract:

    This is the C header for driver frameworks DPC object

Revision History:


--*/

#ifndef _WDFDPC_H_
#define _WDFDPC_H_



#if (NTDDI_VERSION >= NTDDI_WIN2K)



//
// This is the function that gets called back into the driver
// when the DPC fires.
//
typedef
__drv_functionClass(EVT_WDF_DPC)
__drv_sameIRQL
__drv_requiresIRQL(DISPATCH_LEVEL)
VOID
EVT_WDF_DPC(
    __in
    WDFDPC Dpc
    );

typedef EVT_WDF_DPC *PFN_WDF_DPC;

typedef struct _WDF_DPC_CONFIG {
    ULONG       Size;
    PFN_WDF_DPC EvtDpcFunc;

    //
    // If this is TRUE, the DPC will automatically serialize
    // with the event callback handlers of its Parent Object.
    //
    // Parent Object's callback constraints should be compatible
    // with the DPC (DISPATCH_LEVEL), or the request will fail.
    //
    BOOLEAN     AutomaticSerialization;

} WDF_DPC_CONFIG, *PWDF_DPC_CONFIG;

VOID
FORCEINLINE
WDF_DPC_CONFIG_INIT(
    __out PWDF_DPC_CONFIG Config,
    __in  PFN_WDF_DPC     EvtDpcFunc
    )
{
    RtlZeroMemory(Config, sizeof(WDF_DPC_CONFIG));
    Config->Size = sizeof(WDF_DPC_CONFIG);
    Config->EvtDpcFunc = EvtDpcFunc;

    Config->AutomaticSerialization = TRUE;
}

//
// WDF Function: WdfDpcCreate
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFDPCCREATE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDF_DPC_CONFIG Config,
    __in
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFDPC* Dpc
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfDpcCreate(
    __in
    PWDF_DPC_CONFIG Config,
    __in
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFDPC* Dpc
    )
{
    return ((PFN_WDFDPCCREATE) WdfFunctions[WdfDpcCreateTableIndex])(WdfDriverGlobals, Config, Attributes, Dpc);
}

//
// WDF Function: WdfDpcEnqueue
//
typedef
__drv_maxIRQL(HIGH_LEVEL)
WDFAPI
BOOLEAN
(*PFN_WDFDPCENQUEUE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDPC Dpc
    );

__drv_maxIRQL(HIGH_LEVEL)
BOOLEAN
FORCEINLINE
WdfDpcEnqueue(
    __in
    WDFDPC Dpc
    )
{
    return ((PFN_WDFDPCENQUEUE) WdfFunctions[WdfDpcEnqueueTableIndex])(WdfDriverGlobals, Dpc);
}

//
// WDF Function: WdfDpcCancel
//
typedef
__drv_when(Wait == __true, __drv_maxIRQL(PASSIVE_LEVEL))
__drv_when(Wait == __false, __drv_maxIRQL(HIGH_LEVEL))
WDFAPI
BOOLEAN
(*PFN_WDFDPCCANCEL)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDPC Dpc,
    __in
    BOOLEAN Wait
    );

__drv_when(Wait == __true, __drv_maxIRQL(PASSIVE_LEVEL))
__drv_when(Wait == __false, __drv_maxIRQL(HIGH_LEVEL))
BOOLEAN
FORCEINLINE
WdfDpcCancel(
    __in
    WDFDPC Dpc,
    __in
    BOOLEAN Wait
    )
{
    return ((PFN_WDFDPCCANCEL) WdfFunctions[WdfDpcCancelTableIndex])(WdfDriverGlobals, Dpc, Wait);
}

//
// WDF Function: WdfDpcGetParentObject
//
typedef
__drv_maxIRQL(HIGH_LEVEL)
WDFAPI
WDFOBJECT
(*PFN_WDFDPCGETPARENTOBJECT)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDPC Dpc
    );

__drv_maxIRQL(HIGH_LEVEL)
WDFOBJECT
FORCEINLINE
WdfDpcGetParentObject(
    __in
    WDFDPC Dpc
    )
{
    return ((PFN_WDFDPCGETPARENTOBJECT) WdfFunctions[WdfDpcGetParentObjectTableIndex])(WdfDriverGlobals, Dpc);
}

//
// WDF Function: WdfDpcWdmGetDpc
//
typedef
__drv_maxIRQL(HIGH_LEVEL)
WDFAPI
PKDPC
(*PFN_WDFDPCWDMGETDPC)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDPC Dpc
    );

__drv_maxIRQL(HIGH_LEVEL)
PKDPC
FORCEINLINE
WdfDpcWdmGetDpc(
    __in
    WDFDPC Dpc
    )
{
    return ((PFN_WDFDPCWDMGETDPC) WdfFunctions[WdfDpcWdmGetDpcTableIndex])(WdfDriverGlobals, Dpc);
}



#endif // (NTDDI_VERSION >= NTDDI_WIN2K)


#endif // _WDFDPC_H_

