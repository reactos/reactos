/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

_WdfVersionBuild_

Module Name:

    wdfworkitem.h

Abstract:

    This is the Windows Driver Framework work item DDIs

Revision History:


--*/

#ifndef _WDFWORKITEM_H_
#define _WDFWORKITEM_H_



#if (NTDDI_VERSION >= NTDDI_WIN2K)



//
// This is the function that gets called back into the driver
// when the WorkItem fires.
//
typedef
__drv_functionClass(EVT_WDF_WORKITEM)
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
VOID
EVT_WDF_WORKITEM(
    __in
    WDFWORKITEM WorkItem
    );

typedef EVT_WDF_WORKITEM *PFN_WDF_WORKITEM;

typedef struct _WDF_WORKITEM_CONFIG {

    ULONG Size;

    PFN_WDF_WORKITEM EvtWorkItemFunc;

    //
    // If this is TRUE, the workitem will automatically serialize
    // with the event callback handlers of its Parent Object.
    //
    // Parent Object's callback constraints should be compatible
    // with the work item (PASSIVE_LEVEL), or the request will fail.
    //
    BOOLEAN AutomaticSerialization;

} WDF_WORKITEM_CONFIG, *PWDF_WORKITEM_CONFIG;


VOID
FORCEINLINE
WDF_WORKITEM_CONFIG_INIT(
    __out PWDF_WORKITEM_CONFIG Config,
    __in PFN_WDF_WORKITEM     EvtWorkItemFunc
    )
{
    RtlZeroMemory(Config, sizeof(WDF_WORKITEM_CONFIG));
    Config->Size = sizeof(WDF_WORKITEM_CONFIG);
    Config->EvtWorkItemFunc = EvtWorkItemFunc;

    Config->AutomaticSerialization = TRUE;
}


//
// WDF Function: WdfWorkItemCreate
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFWORKITEMCREATE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDF_WORKITEM_CONFIG Config,
    __in
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFWORKITEM* WorkItem
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfWorkItemCreate(
    __in
    PWDF_WORKITEM_CONFIG Config,
    __in
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFWORKITEM* WorkItem
    )
{
    return ((PFN_WDFWORKITEMCREATE) WdfFunctions[WdfWorkItemCreateTableIndex])(WdfDriverGlobals, Config, Attributes, WorkItem);
}

//
// WDF Function: WdfWorkItemEnqueue
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFWORKITEMENQUEUE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWORKITEM WorkItem
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfWorkItemEnqueue(
    __in
    WDFWORKITEM WorkItem
    )
{
    ((PFN_WDFWORKITEMENQUEUE) WdfFunctions[WdfWorkItemEnqueueTableIndex])(WdfDriverGlobals, WorkItem);
}

//
// WDF Function: WdfWorkItemGetParentObject
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
WDFOBJECT
(*PFN_WDFWORKITEMGETPARENTOBJECT)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWORKITEM WorkItem
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFOBJECT
FORCEINLINE
WdfWorkItemGetParentObject(
    __in
    WDFWORKITEM WorkItem
    )
{
    return ((PFN_WDFWORKITEMGETPARENTOBJECT) WdfFunctions[WdfWorkItemGetParentObjectTableIndex])(WdfDriverGlobals, WorkItem);
}

//
// WDF Function: WdfWorkItemFlush
//
typedef
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
VOID
(*PFN_WDFWORKITEMFLUSH)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWORKITEM WorkItem
    );

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
FORCEINLINE
WdfWorkItemFlush(
    __in
    WDFWORKITEM WorkItem
    )
{
    ((PFN_WDFWORKITEMFLUSH) WdfFunctions[WdfWorkItemFlushTableIndex])(WdfDriverGlobals, WorkItem);
}



#endif // (NTDDI_VERSION >= NTDDI_WIN2K)


#endif // _WDFWORKITEM_H_

