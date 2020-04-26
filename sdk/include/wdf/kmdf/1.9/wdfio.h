/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

_WdfVersionBuild_

Module Name:

    wdfio.h

Abstract:

    This module contains contains the Windows Driver Framework I/O
    interfaces.

Environment:

    kernel mode only

Revision History:


--*/

#ifndef _WDFIO_H_
#define _WDFIO_H_



#if (NTDDI_VERSION >= NTDDI_WIN2K)

// 
// Types
// 

// 
// This defines the dispatch type of the queue. This controls how
// the queue raises I/O events to the driver through the registered
// callbacks.
// 
// Sequential allows the driver to have the queue automatically dispatch
// one request at a time, and will hold requests until a current request
// is completed.
// 
// Parallel has the queue dispatch requests to the driver as they arrive
// at the queue, and the queue is in a processing state. The driver can
// look at the requests in real time, and decide to service them, forward them
// to another queue, pend them, or return a status to have the queue held
// due to a hardware or other resource limit.
// 
// Manual allows the driver to create multiple queues for requests, and control
// when it wants to retrieve requests from the queue by calling the queues
// WdfIoQueueRetrieveNextRequest() API.
// 

typedef enum _WDF_IO_QUEUE_DISPATCH_TYPE {
    WdfIoQueueDispatchInvalid = 0,
    WdfIoQueueDispatchSequential,
    WdfIoQueueDispatchParallel,
    WdfIoQueueDispatchManual,
    WdfIoQueueDispatchMax,
} WDF_IO_QUEUE_DISPATCH_TYPE;

// 
// This defines the status of the queue.
// 
// WdfIoQueueAcceptRequests   - If TRUE, Queue will accept requests from WDM
// dispatch through WdfDeviceConfigureRequestDispatching,
// or from WdfRequestForwardToIoQueue.
// 
// If FALSE, Queue will complete requests with
// STATUS_CANCELLED from WdfDeviceConfigureRequestDispatching,
// and fail WdfRequestForwardToIoQueue with
// STATUS_WDF_BUSY.
// 
// 
// WdfIoQueueDispatchRequests - If TRUE, and the Queue is configured for
// automatic dispatch as either
// WdfIoQueueDispatchSequential,
// or WdfIoQueueDispatchParallel, the Queue will
// present the requests to the driver according
// to the drivers configuration.
// 
// If FALSE, requests are not automatically
// presented to the device driver.
// 
// This has no effect on the drivers ability to
// retrieve requests with WdfIoQueueRetrieveNextRequest.
// 
// WdfIoQueueNoRequests       - If TRUE, the Queue has no requests to present
// or return to the device driver.
// 
// WdfIoQueueDriverNoRequests - If TRUE, the device driver is not operating
// on any requests retrieved from or presented
// by this Queue.
// 
// WdfIoQueuePnpHeld          - The Framework PnP stage has requested that
// the device driver stop receiving new requests.
// 
// Automatic request dispatch stops, and
// WdfIoQueueRetrieveNextRequest returns STATUS_WDF_BUSY.
// 

typedef enum _WDF_IO_QUEUE_STATE {
    WdfIoQueueAcceptRequests = 0x01,
    WdfIoQueueDispatchRequests = 0x02,
    WdfIoQueueNoRequests = 0x04,
    WdfIoQueueDriverNoRequests = 0x08,
    WdfIoQueuePnpHeld = 0x10,
} WDF_IO_QUEUE_STATE;



//
// These macros represent some common Queue states
//

//
// A Queue is idle if it has no requests, and the driver
// is not operating on any.
//

BOOLEAN
FORCEINLINE
WDF_IO_QUEUE_IDLE(
    __in WDF_IO_QUEUE_STATE State
    )
{
    return ((State & WdfIoQueueNoRequests) &&
            (State & WdfIoQueueDriverNoRequests)) ? TRUE: FALSE;
}

//
// A Queue is ready if it can accept and dispatch requests and
// queue is not held by PNP
//
BOOLEAN
FORCEINLINE
WDF_IO_QUEUE_READY(
    __in WDF_IO_QUEUE_STATE State
    )
{
    return ((State & WdfIoQueueDispatchRequests) &&
        (State & WdfIoQueueAcceptRequests) &&
        ((State & WdfIoQueuePnpHeld)==0)) ? TRUE: FALSE;
}

//
// A Queue is stopped if it can accept new requests, but
// is not automatically delivering them to the device driver,
// and the queue is idle.
//
BOOLEAN
FORCEINLINE
WDF_IO_QUEUE_STOPPED(
    __in WDF_IO_QUEUE_STATE State
    )
{
    return (((State & WdfIoQueueDispatchRequests) == 0) &&
        (State & WdfIoQueueAcceptRequests) &&
        (State & WdfIoQueueDriverNoRequests)) ? TRUE: FALSE;

}

//
// A Queue is drained if it can not accept new requests but
// can dispatch existing requests, and there are no requests
// either in the Queue or the device driver.
//

BOOLEAN
FORCEINLINE
WDF_IO_QUEUE_DRAINED(
    __in WDF_IO_QUEUE_STATE State
    )
{
    return ( ((State & WdfIoQueueAcceptRequests)==0) &&
          (State & WdfIoQueueDispatchRequests) &&
          (State & WdfIoQueueNoRequests)  &&
          (State & WdfIoQueueDriverNoRequests) ) ? TRUE: FALSE;

}

//
// A Queue is purged if it can not accept new requests
// and there are no requests either in the Queue or
// the device driver.
//
BOOLEAN
FORCEINLINE
WDF_IO_QUEUE_PURGED(
    __in WDF_IO_QUEUE_STATE State
    )
{
    return ( ((State & WdfIoQueueAcceptRequests)==0) &&
          (State & WdfIoQueueNoRequests) &&
          (State & WdfIoQueueDriverNoRequests) ) ? TRUE: FALSE;

}

//
// Event callback definitions
//

typedef
__drv_functionClass(EVT_WDF_IO_QUEUE_IO_DEFAULT)
__drv_sameIRQL
__drv_maxIRQL(DISPATCH_LEVEL)
VOID
EVT_WDF_IO_QUEUE_IO_DEFAULT(
    __in
    WDFQUEUE Queue,
    __in
    WDFREQUEST Request
    );

typedef EVT_WDF_IO_QUEUE_IO_DEFAULT *PFN_WDF_IO_QUEUE_IO_DEFAULT;


typedef
__drv_functionClass(EVT_WDF_IO_QUEUE_IO_STOP)
__drv_sameIRQL
__drv_maxIRQL(DISPATCH_LEVEL)
VOID
EVT_WDF_IO_QUEUE_IO_STOP(
    __in
    WDFQUEUE Queue,
    __in
    WDFREQUEST Request,
    __in
    ULONG ActionFlags
    );

typedef EVT_WDF_IO_QUEUE_IO_STOP *PFN_WDF_IO_QUEUE_IO_STOP;

typedef
__drv_functionClass(EVT_WDF_IO_QUEUE_IO_RESUME)
__drv_sameIRQL
__drv_maxIRQL(DISPATCH_LEVEL)
VOID
EVT_WDF_IO_QUEUE_IO_RESUME(
    __in
    WDFQUEUE Queue,
    __in
    WDFREQUEST Request
    );

typedef EVT_WDF_IO_QUEUE_IO_RESUME *PFN_WDF_IO_QUEUE_IO_RESUME;

typedef
__drv_functionClass(EVT_WDF_IO_QUEUE_IO_READ)
__drv_sameIRQL
__drv_maxIRQL(DISPATCH_LEVEL)
VOID
EVT_WDF_IO_QUEUE_IO_READ(
    __in
    WDFQUEUE Queue,
    __in
    WDFREQUEST Request,
    __in
    size_t Length
    );

typedef EVT_WDF_IO_QUEUE_IO_READ *PFN_WDF_IO_QUEUE_IO_READ;

typedef
__drv_functionClass(EVT_WDF_IO_QUEUE_IO_WRITE)
__drv_sameIRQL
__drv_maxIRQL(DISPATCH_LEVEL)
VOID
EVT_WDF_IO_QUEUE_IO_WRITE(
    __in
    WDFQUEUE Queue,
    __in
    WDFREQUEST Request,
    __in
    size_t Length
    );

typedef EVT_WDF_IO_QUEUE_IO_WRITE *PFN_WDF_IO_QUEUE_IO_WRITE;

typedef
__drv_functionClass(EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL)
__drv_sameIRQL
__drv_maxIRQL(DISPATCH_LEVEL)
VOID
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL(
    __in
    WDFQUEUE Queue,
    __in
    WDFREQUEST Request,
    __in
    size_t OutputBufferLength,
    __in
    size_t InputBufferLength,
    __in
    ULONG IoControlCode
    );

typedef EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL *PFN_WDF_IO_QUEUE_IO_DEVICE_CONTROL;

typedef
__drv_functionClass(EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL)
__drv_sameIRQL
__drv_maxIRQL(DISPATCH_LEVEL)
VOID
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL(
    __in
    WDFQUEUE Queue,
    __in
    WDFREQUEST Request,
    __in
    size_t OutputBufferLength,
    __in
    size_t InputBufferLength,
    __in
    ULONG IoControlCode
    );

typedef EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL *PFN_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL;


typedef
__drv_functionClass(EVT_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE)
__drv_sameIRQL
__drv_maxIRQL(DISPATCH_LEVEL)
VOID
EVT_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE(
    __in
    WDFQUEUE Queue,
    __in
    WDFREQUEST Request
    );

typedef EVT_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE *PFN_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE;


typedef
__drv_functionClass(EVT_WDF_IO_QUEUE_STATE)
__drv_sameIRQL
__drv_maxIRQL(DISPATCH_LEVEL)
VOID
EVT_WDF_IO_QUEUE_STATE(
    __in
    WDFQUEUE Queue,
    __in
    WDFCONTEXT Context
    );

typedef EVT_WDF_IO_QUEUE_STATE *PFN_WDF_IO_QUEUE_STATE;

//
// This is the structure used to configure an IoQueue and
// register callback events to it.
//

typedef struct _WDF_IO_QUEUE_CONFIG {

    ULONG                                       Size;

    WDF_IO_QUEUE_DISPATCH_TYPE                  DispatchType;

    WDF_TRI_STATE                               PowerManaged;

    BOOLEAN                                     AllowZeroLengthRequests;

    BOOLEAN                                     DefaultQueue;

    PFN_WDF_IO_QUEUE_IO_DEFAULT                 EvtIoDefault;

    PFN_WDF_IO_QUEUE_IO_READ                    EvtIoRead;

    PFN_WDF_IO_QUEUE_IO_WRITE                   EvtIoWrite;

    PFN_WDF_IO_QUEUE_IO_DEVICE_CONTROL          EvtIoDeviceControl;

    PFN_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL EvtIoInternalDeviceControl;

    PFN_WDF_IO_QUEUE_IO_STOP                    EvtIoStop;

    PFN_WDF_IO_QUEUE_IO_RESUME                  EvtIoResume;

    PFN_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE       EvtIoCanceledOnQueue;

    union {
        struct {
            ULONG NumberOfPresentedRequests;
        } Parallel;
    } Settings;

} WDF_IO_QUEUE_CONFIG, *PWDF_IO_QUEUE_CONFIG;

VOID
FORCEINLINE
WDF_IO_QUEUE_CONFIG_INIT(
    __out PWDF_IO_QUEUE_CONFIG      Config,
    __in WDF_IO_QUEUE_DISPATCH_TYPE DispatchType
    )
{
    RtlZeroMemory(Config, sizeof(WDF_IO_QUEUE_CONFIG));

    Config->Size = sizeof(WDF_IO_QUEUE_CONFIG);
    Config->PowerManaged = WdfUseDefault;
    Config->DispatchType = DispatchType;
    if (Config->DispatchType == WdfIoQueueDispatchParallel) {
        Config->Settings.Parallel.NumberOfPresentedRequests = (ULONG)-1;    
    }
}

VOID
FORCEINLINE
WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
    __out PWDF_IO_QUEUE_CONFIG      Config,
    __in WDF_IO_QUEUE_DISPATCH_TYPE DispatchType
    )
{
    RtlZeroMemory(Config, sizeof(WDF_IO_QUEUE_CONFIG));

    Config->Size = sizeof(WDF_IO_QUEUE_CONFIG);
    Config->PowerManaged = WdfUseDefault;
    Config->DefaultQueue = TRUE;
    Config->DispatchType = DispatchType;
    if (Config->DispatchType == WdfIoQueueDispatchParallel) {
        Config->Settings.Parallel.NumberOfPresentedRequests = (ULONG)-1;    
    }    
}

typedef enum _WDF_IO_FORWARD_PROGRESS_ACTION {
    WdfIoForwardProgressActionInvalid = 0x0,
    WdfIoForwardProgressActionFailRequest,
    WdfIoForwardProgressActionUseReservedRequest
} WDF_IO_FORWARD_PROGRESS_ACTION;

typedef enum _WDF_IO_FORWARD_PROGRESS_RESERVED_POLICY  {
  WdfIoForwardProgressInvalidPolicy =0x0,
  WdfIoForwardProgressReservedPolicyAlwaysUseReservedRequest,
  WdfIoForwardProgressReservedPolicyUseExamine,
  WdfIoForwardProgressReservedPolicyPagingIO
} WDF_IO_FORWARD_PROGRESS_RESERVED_POLICY;

typedef
__drv_functionClass(EVT_WDF_IO_ALLOCATE_RESOURCES_FOR_RESERVED_REQUEST)
__drv_sameIRQL
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
EVT_WDF_IO_ALLOCATE_RESOURCES_FOR_RESERVED_REQUEST(
    __in
    WDFQUEUE Queue,
    __in
    WDFREQUEST Request
    );

typedef EVT_WDF_IO_ALLOCATE_RESOURCES_FOR_RESERVED_REQUEST *PFN_WDF_IO_ALLOCATE_RESOURCES_FOR_RESERVED_REQUEST;

typedef
__drv_functionClass(EVT_WDF_IO_ALLOCATE_REQUEST_RESOURCES)
__drv_sameIRQL
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
EVT_WDF_IO_ALLOCATE_REQUEST_RESOURCES(
    __in
    WDFQUEUE Queue,
    __in
    WDFREQUEST Request
    );

typedef EVT_WDF_IO_ALLOCATE_REQUEST_RESOURCES *PFN_WDF_IO_ALLOCATE_REQUEST_RESOURCES;

typedef
__drv_functionClass(EVT_WDF_IO_WDM_IRP_FOR_FORWARD_PROGRESS)
__drv_sameIRQL
__drv_maxIRQL(DISPATCH_LEVEL)
WDF_IO_FORWARD_PROGRESS_ACTION
EVT_WDF_IO_WDM_IRP_FOR_FORWARD_PROGRESS(
    __in
    WDFQUEUE Queue,
    __in
    PIRP Irp
    );

typedef EVT_WDF_IO_WDM_IRP_FOR_FORWARD_PROGRESS *PFN_WDF_IO_WDM_IRP_FOR_FORWARD_PROGRESS;

typedef  struct _WDF_IO_FORWARD_PROGRESS_RESERVED_POLICY_SETTINGS {
    union {

        struct {
          PFN_WDF_IO_WDM_IRP_FOR_FORWARD_PROGRESS     EvtIoWdmIrpForForwardProgress;
        } ExaminePolicy;

    } Policy;
} WDF_IO_FORWARD_PROGRESS_RESERVED_POLICY_SETTINGS;

typedef struct _WDF_IO_QUEUE_FORWARD_PROGRESS_POLICY {
    ULONG  Size;

    ULONG TotalForwardProgressRequests;

    //
    // Specify the type of the policy here.
    //
    WDF_IO_FORWARD_PROGRESS_RESERVED_POLICY ForwardProgressReservedPolicy;
    
    //
    // Structure which contains the policy specific fields
    //
    WDF_IO_FORWARD_PROGRESS_RESERVED_POLICY_SETTINGS ForwardProgressReservePolicySettings;

    //
    // Callback for reserved request given at initialization time
    //
    PFN_WDF_IO_ALLOCATE_RESOURCES_FOR_RESERVED_REQUEST EvtIoAllocateResourcesForReservedRequest;

    //
    // Callback for reserved request given at run time
    //
    PFN_WDF_IO_ALLOCATE_REQUEST_RESOURCES  EvtIoAllocateRequestResources;       

}  WDF_IO_QUEUE_FORWARD_PROGRESS_POLICY, *PWDF_IO_QUEUE_FORWARD_PROGRESS_POLICY;

VOID
FORCEINLINE
WDF_IO_QUEUE_FORWARD_PROGRESS_POLICY_DEFAULT_INIT(
    __out PWDF_IO_QUEUE_FORWARD_PROGRESS_POLICY Policy,
    __in ULONG TotalForwardProgressRequests
    )
{
    RtlZeroMemory(Policy, sizeof(WDF_IO_QUEUE_FORWARD_PROGRESS_POLICY));

    Policy->Size = sizeof(WDF_IO_QUEUE_FORWARD_PROGRESS_POLICY);
    Policy->TotalForwardProgressRequests = TotalForwardProgressRequests;
    Policy->ForwardProgressReservedPolicy = WdfIoForwardProgressReservedPolicyAlwaysUseReservedRequest;
}


VOID
FORCEINLINE
WDF_IO_QUEUE_FORWARD_PROGRESS_POLICY_EXAMINE_INIT(
    __out PWDF_IO_QUEUE_FORWARD_PROGRESS_POLICY      Policy,
    __in ULONG TotalForwardProgressRequests,
    __in PFN_WDF_IO_WDM_IRP_FOR_FORWARD_PROGRESS EvtIoWdmIrpForForwardProgress
    )
{
    RtlZeroMemory(Policy, sizeof(WDF_IO_QUEUE_FORWARD_PROGRESS_POLICY));

    Policy->Size = sizeof(WDF_IO_QUEUE_FORWARD_PROGRESS_POLICY);
    Policy->TotalForwardProgressRequests = TotalForwardProgressRequests;
    Policy->ForwardProgressReservedPolicy =  WdfIoForwardProgressReservedPolicyUseExamine;
    Policy->ForwardProgressReservePolicySettings.Policy.ExaminePolicy.EvtIoWdmIrpForForwardProgress =
            EvtIoWdmIrpForForwardProgress;
}

VOID
FORCEINLINE
WDF_IO_QUEUE_FORWARD_PROGRESS_POLICY_PAGINGIO_INIT(
    __out PWDF_IO_QUEUE_FORWARD_PROGRESS_POLICY      Policy,
    __in ULONG TotalForwardProgressRequests
    )
{
    RtlZeroMemory(Policy, sizeof(WDF_IO_QUEUE_FORWARD_PROGRESS_POLICY));

    Policy->Size = sizeof(WDF_IO_QUEUE_FORWARD_PROGRESS_POLICY);
    Policy->TotalForwardProgressRequests = TotalForwardProgressRequests;
    Policy->ForwardProgressReservedPolicy = WdfIoForwardProgressReservedPolicyPagingIO;
}


//
// WDF Function: WdfIoQueueCreate
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFIOQUEUECREATE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PWDF_IO_QUEUE_CONFIG Config,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES QueueAttributes,
    __out_opt
    WDFQUEUE* Queue
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfIoQueueCreate(
    __in
    WDFDEVICE Device,
    __in
    PWDF_IO_QUEUE_CONFIG Config,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES QueueAttributes,
    __out_opt
    WDFQUEUE* Queue
    )
{
    return ((PFN_WDFIOQUEUECREATE) WdfFunctions[WdfIoQueueCreateTableIndex])(WdfDriverGlobals, Device, Config, QueueAttributes, Queue);
}

//
// WDF Function: WdfIoQueueGetState
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
WDF_IO_QUEUE_STATE
(*PFN_WDFIOQUEUEGETSTATE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue,
    __out_opt
    PULONG QueueRequests,
    __out_opt
    PULONG DriverRequests
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDF_IO_QUEUE_STATE
FORCEINLINE
WdfIoQueueGetState(
    __in
    WDFQUEUE Queue,
    __out_opt
    PULONG QueueRequests,
    __out_opt
    PULONG DriverRequests
    )
{
    return ((PFN_WDFIOQUEUEGETSTATE) WdfFunctions[WdfIoQueueGetStateTableIndex])(WdfDriverGlobals, Queue, QueueRequests, DriverRequests);
}

//
// WDF Function: WdfIoQueueStart
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFIOQUEUESTART)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfIoQueueStart(
    __in
    WDFQUEUE Queue
    )
{
    ((PFN_WDFIOQUEUESTART) WdfFunctions[WdfIoQueueStartTableIndex])(WdfDriverGlobals, Queue);
}

//
// WDF Function: WdfIoQueueStop
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFIOQUEUESTOP)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue,
    __drv_when(Context != 0, __in)
    __drv_when(Context == 0, __in_opt)
    PFN_WDF_IO_QUEUE_STATE StopComplete,
    __drv_when(StopComplete != 0, __in)
    __drv_when(StopComplete == 0, __in_opt)
    WDFCONTEXT Context
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfIoQueueStop(
    __in
    WDFQUEUE Queue,
    __drv_when(Context != 0, __in)
    __drv_when(Context == 0, __in_opt)
    PFN_WDF_IO_QUEUE_STATE StopComplete,
    __drv_when(StopComplete != 0, __in)
    __drv_when(StopComplete == 0, __in_opt)
    WDFCONTEXT Context
    )
{
    ((PFN_WDFIOQUEUESTOP) WdfFunctions[WdfIoQueueStopTableIndex])(WdfDriverGlobals, Queue, StopComplete, Context);
}

//
// WDF Function: WdfIoQueueStopSynchronously
//
typedef
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
VOID
(*PFN_WDFIOQUEUESTOPSYNCHRONOUSLY)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue
    );

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
FORCEINLINE
WdfIoQueueStopSynchronously(
    __in
    WDFQUEUE Queue
    )
{
    ((PFN_WDFIOQUEUESTOPSYNCHRONOUSLY) WdfFunctions[WdfIoQueueStopSynchronouslyTableIndex])(WdfDriverGlobals, Queue);
}

//
// WDF Function: WdfIoQueueGetDevice
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
(*PFN_WDFIOQUEUEGETDEVICE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFDEVICE
FORCEINLINE
WdfIoQueueGetDevice(
    __in
    WDFQUEUE Queue
    )
{
    return ((PFN_WDFIOQUEUEGETDEVICE) WdfFunctions[WdfIoQueueGetDeviceTableIndex])(WdfDriverGlobals, Queue);
}

//
// WDF Function: WdfIoQueueRetrieveNextRequest
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFIOQUEUERETRIEVENEXTREQUEST)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue,
    __out
    WDFREQUEST* OutRequest
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfIoQueueRetrieveNextRequest(
    __in
    WDFQUEUE Queue,
    __out
    WDFREQUEST* OutRequest
    )
{
    return ((PFN_WDFIOQUEUERETRIEVENEXTREQUEST) WdfFunctions[WdfIoQueueRetrieveNextRequestTableIndex])(WdfDriverGlobals, Queue, OutRequest);
}

//
// WDF Function: WdfIoQueueRetrieveRequestByFileObject
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFIOQUEUERETRIEVEREQUESTBYFILEOBJECT)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue,
    __in
    WDFFILEOBJECT FileObject,
    __out
    WDFREQUEST* OutRequest
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfIoQueueRetrieveRequestByFileObject(
    __in
    WDFQUEUE Queue,
    __in
    WDFFILEOBJECT FileObject,
    __out
    WDFREQUEST* OutRequest
    )
{
    return ((PFN_WDFIOQUEUERETRIEVEREQUESTBYFILEOBJECT) WdfFunctions[WdfIoQueueRetrieveRequestByFileObjectTableIndex])(WdfDriverGlobals, Queue, FileObject, OutRequest);
}

//
// WDF Function: WdfIoQueueFindRequest
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFIOQUEUEFINDREQUEST)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue,
    __in_opt
    WDFREQUEST FoundRequest,
    __in_opt
    WDFFILEOBJECT FileObject,
    __inout_opt
    PWDF_REQUEST_PARAMETERS Parameters,
    __out
    WDFREQUEST* OutRequest
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfIoQueueFindRequest(
    __in
    WDFQUEUE Queue,
    __in_opt
    WDFREQUEST FoundRequest,
    __in_opt
    WDFFILEOBJECT FileObject,
    __inout_opt
    PWDF_REQUEST_PARAMETERS Parameters,
    __out
    WDFREQUEST* OutRequest
    )
{
    return ((PFN_WDFIOQUEUEFINDREQUEST) WdfFunctions[WdfIoQueueFindRequestTableIndex])(WdfDriverGlobals, Queue, FoundRequest, FileObject, Parameters, OutRequest);
}

//
// WDF Function: WdfIoQueueRetrieveFoundRequest
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFIOQUEUERETRIEVEFOUNDREQUEST)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue,
    __in
    WDFREQUEST FoundRequest,
    __out
    WDFREQUEST* OutRequest
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfIoQueueRetrieveFoundRequest(
    __in
    WDFQUEUE Queue,
    __in
    WDFREQUEST FoundRequest,
    __out
    WDFREQUEST* OutRequest
    )
{
    return ((PFN_WDFIOQUEUERETRIEVEFOUNDREQUEST) WdfFunctions[WdfIoQueueRetrieveFoundRequestTableIndex])(WdfDriverGlobals, Queue, FoundRequest, OutRequest);
}

//
// WDF Function: WdfIoQueueDrainSynchronously
//
typedef
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
VOID
(*PFN_WDFIOQUEUEDRAINSYNCHRONOUSLY)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue
    );

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
FORCEINLINE
WdfIoQueueDrainSynchronously(
    __in
    WDFQUEUE Queue
    )
{
    ((PFN_WDFIOQUEUEDRAINSYNCHRONOUSLY) WdfFunctions[WdfIoQueueDrainSynchronouslyTableIndex])(WdfDriverGlobals, Queue);
}

//
// WDF Function: WdfIoQueueDrain
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFIOQUEUEDRAIN)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue,
    __drv_when(Context != 0, __in)
    __drv_when(Context == 0, __in_opt)
    PFN_WDF_IO_QUEUE_STATE DrainComplete,
    __drv_when(DrainComplete != 0, __in)
    __drv_when(DrainComplete == 0, __in_opt)
    WDFCONTEXT Context
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfIoQueueDrain(
    __in
    WDFQUEUE Queue,
    __drv_when(Context != 0, __in)
    __drv_when(Context == 0, __in_opt)
    PFN_WDF_IO_QUEUE_STATE DrainComplete,
    __drv_when(DrainComplete != 0, __in)
    __drv_when(DrainComplete == 0, __in_opt)
    WDFCONTEXT Context
    )
{
    ((PFN_WDFIOQUEUEDRAIN) WdfFunctions[WdfIoQueueDrainTableIndex])(WdfDriverGlobals, Queue, DrainComplete, Context);
}

//
// WDF Function: WdfIoQueuePurgeSynchronously
//
typedef
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
VOID
(*PFN_WDFIOQUEUEPURGESYNCHRONOUSLY)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue
    );

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
FORCEINLINE
WdfIoQueuePurgeSynchronously(
    __in
    WDFQUEUE Queue
    )
{
    ((PFN_WDFIOQUEUEPURGESYNCHRONOUSLY) WdfFunctions[WdfIoQueuePurgeSynchronouslyTableIndex])(WdfDriverGlobals, Queue);
}

//
// WDF Function: WdfIoQueuePurge
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFIOQUEUEPURGE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue,
    __drv_when(Context != 0, __in)
    __drv_when(Context == 0, __in_opt)
    PFN_WDF_IO_QUEUE_STATE PurgeComplete,
    __drv_when(PurgeComplete != 0, __in)
    __drv_when(PurgeComplete == 0, __in_opt)
    WDFCONTEXT Context
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfIoQueuePurge(
    __in
    WDFQUEUE Queue,
    __drv_when(Context != 0, __in)
    __drv_when(Context == 0, __in_opt)
    PFN_WDF_IO_QUEUE_STATE PurgeComplete,
    __drv_when(PurgeComplete != 0, __in)
    __drv_when(PurgeComplete == 0, __in_opt)
    WDFCONTEXT Context
    )
{
    ((PFN_WDFIOQUEUEPURGE) WdfFunctions[WdfIoQueuePurgeTableIndex])(WdfDriverGlobals, Queue, PurgeComplete, Context);
}

//
// WDF Function: WdfIoQueueReadyNotify
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFIOQUEUEREADYNOTIFY)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue,
    __in_opt
    PFN_WDF_IO_QUEUE_STATE QueueReady,
    __in_opt
    WDFCONTEXT Context
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfIoQueueReadyNotify(
    __in
    WDFQUEUE Queue,
    __in_opt
    PFN_WDF_IO_QUEUE_STATE QueueReady,
    __in_opt
    WDFCONTEXT Context
    )
{
    return ((PFN_WDFIOQUEUEREADYNOTIFY) WdfFunctions[WdfIoQueueReadyNotifyTableIndex])(WdfDriverGlobals, Queue, QueueReady, Context);
}

//
// WDF Function: WdfIoQueueAssignForwardProgressPolicy
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFIOQUEUEASSIGNFORWARDPROGRESSPOLICY)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFQUEUE Queue,
    __in
    PWDF_IO_QUEUE_FORWARD_PROGRESS_POLICY ForwardProgressPolicy
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfIoQueueAssignForwardProgressPolicy(
    __in
    WDFQUEUE Queue,
    __in
    PWDF_IO_QUEUE_FORWARD_PROGRESS_POLICY ForwardProgressPolicy
    )
{
    return ((PFN_WDFIOQUEUEASSIGNFORWARDPROGRESSPOLICY) WdfFunctions[WdfIoQueueAssignForwardProgressPolicyTableIndex])(WdfDriverGlobals, Queue, ForwardProgressPolicy);
}



#endif // (NTDDI_VERSION >= NTDDI_WIN2K)


#endif // _WDFIO_H_

