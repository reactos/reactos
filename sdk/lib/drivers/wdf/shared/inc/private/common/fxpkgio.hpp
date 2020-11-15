/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxPkgIo.hpp

Abstract:

    This module implements the I/O package for the driver frameworks.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef _FXPKGIO_H_
#define _FXPKGIO_H_




#include "fxpkgioshared.hpp"
#include "fxirpdynamicdispatchinfo.hpp"
#include "fxdevicecallbacks.hpp"
#include "fxcxdeviceinfo.hpp"

//
// This flag is or-ed with a pointer value that is ptr aligned, only lower 2 bits are available.
//
#define FX_IN_DISPATCH_CALLBACK     0x00000001

enum FxIoIteratorList {
    FxIoQueueIteratorListInvalid = 0,
    FxIoQueueIteratorListPowerOn,
    FxIoQueueIteratorListPowerOff,
};

#define IO_ITERATOR_FLUSH_TAG (PVOID) 'sulf'
#define IO_ITERATOR_POWER_TAG (PVOID) 'ewop'

//
// This class is allocated by the driver frameworks manager
// PER DEVICE, and is not package global, but per device global,
// data.
//
// This is similar to the NT DeviceExtension.
//
class FxPkgIo : public FxPackage
{

private:

    FxIoQueue*  m_DefaultQueue;

    //
    // This is the list of IoQueue objects allocated by the driver
    // and associated with this device. The IoPackage instance
    // will release these references automatically when the device
    // is removed.
    //
    LIST_ENTRY  m_IoQueueListHead;

    //
    // This is the forwarding table
    //
    FxIoQueue*  m_DispatchTable[IRP_MJ_MAXIMUM_FUNCTION+1];

    //
    // This is the seed value used to pass to the
    // FxRandom for testing forward progress
    //
    ULONG       m_RandomSeed;

    // TRUE if behave as a filter (forward default requests)
    BOOLEAN     m_Filter;

    BOOLEAN     m_PowerStateOn;

    //
    // TRUE if queues are shutting down (surprise_remove/remove in progress).
    //
    BOOLEAN     m_QueuesAreShuttingDown;

    //
    // We'll maintain the dynamic dispatch table "per device" so that it is possible
    // to have different callbacks for each device.
    // Note that each device may be associted with multiple class extension in the future.
    //
    LIST_ENTRY  m_DynamicDispatchInfoListHead;

    //
    // If !=NULL, a pre-process callback was registered
    //
    FxIoInCallerContext m_InCallerContextCallback;

public:

    FxPkgIo(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in CfxDevice *Device
        );

    ~FxPkgIo();

    //
    // Package manager dispatch entries
    //
    _Must_inspect_result_
    virtual
    NTSTATUS
    Dispatch(
        __inout MdIrp Irp
        );

    //
    // Returns the Top level queue for the Io based on the Irp MajorFunction
    //
    FxIoQueue*
    GetDispatchQueue(
        _In_ UCHAR MajorFunction
        )
    {
        return m_DispatchTable[MajorFunction];
    }

    // Do not specify argument names
    FX_DECLARE_VF_FUNCTION_P1(
    NTSTATUS,
    VerifyDispatchContext,
        _In_ WDFCONTEXT
        );

    //
    // Api's supplied by this package
    //
    _Must_inspect_result_
    NTSTATUS
    __fastcall
    DispatchStep1(
        __inout MdIrp        Irp,
        __in    WDFCONTEXT  DispatchContext
        );

    _Must_inspect_result_
    NTSTATUS
    __fastcall
    DispatchStep2(
        __inout  MdIrp        Irp,
        __in_opt FxIoInCallerContext* IoInCallerCtx,
        __in_opt FxIoQueue*  Queue
        );

/*++

    Routine Description:

    Initializes the default queue, and allows the driver to
    pass configuration information.

    The driver callbacks registered in this call are used
    to supply the callbacks for the driver default I/O queue.

Arguments:

    hDevice - Pointer Device Object

Return Value:

    NTSTATUS

--*/
    _Must_inspect_result_
    NTSTATUS
    InitializeDefaultQueue(
        __in    CfxDevice               * Device,
        __inout FxIoQueue               * Queue
        );

    //
    // Register the I/O in-caller context callback
    //
    __inline
    VOID
    SetIoInCallerContextCallback(
        __inout PFN_WDF_IO_IN_CALLER_CONTEXT       EvtIoInCallerContext
        )
    {
        m_InCallerContextCallback.m_Method = EvtIoInCallerContext;
    }

    // Do not specify argument names
    FX_DECLARE_VF_FUNCTION_P2(
    NTSTATUS,
    VerifyEnqueueRequestUpdateFlags,
        _In_ FxRequest*,
        _Inout_ SHORT*
        );

    // Do not specify argument names
    FX_DECLARE_VF_FUNCTION_P2(
    VOID,
    VerifyEnqueueRequestRestoreFlags,
        _In_ FxRequest*,
        _In_ SHORT
        );

    //
    // Enqueue a request to the I/O processing pipeline
    // from the device driver
    //
    _Must_inspect_result_
    NTSTATUS
    EnqueueRequest(
        __in    CfxDevice* Device,
        __inout FxRequest* pRequest
        );

    FxDriver*
    GetDriver(
        VOID
        );

    __inline
    CfxDevice*
    GetDevice(
        VOID
        )
    {
        return m_Device;
    }

    __inline
    FxIoQueue*
    GetDefaultQueue(
        VOID
        )
    {
        return m_DefaultQueue;
    }

    __inline
    BOOLEAN
    IsFilter(
        VOID
        )
    {
        return m_Filter;
    }

    //
    // This is called as a result of a power management state
    // that requests that all I/O in progress stop.
    //
    //
    // FxIoStopProcessingForPowerHold:
    // the function returns when the driver has acknowledged that it has
    // stopped all I/O processing, but may have outstanding "in-flight" requests
    // that have not been completed.
    //
    // FxIoStopProcessingForPowerPurge:
    // the function returns when all requests have been completed and/or
    // cancelled., and there are no more in-flight requests.
    //
    // Any queues not marked as PowerManaged will be left alone.
    //
    // This is called on a PASSIVE_LEVEL thread that can block until
    // I/O has been stopped, or completed/cancelled.
    //
    _Must_inspect_result_
    NTSTATUS
    StopProcessingForPower(
        __in FxIoStopProcessingForPowerAction Action
        );

    //
    // This is called to start, or resume processing when PNP/Power
    // resources have been supplied to the device driver.
    //
    // The driver is notified of resumption for any in-flight I/O.
    //
    _Must_inspect_result_
    NTSTATUS
    ResumeProcessingForPower();

    //
    // This is called on a device which has been restarted from the removed
    // state.  It will reset purged queues so that they can accept new requests
    // when ResumeProcessingForPower is called afterwards.
    //
    VOID
    ResetStateForRestart(
        VOID
        );

    //
    // Called by CfxDevice when WdfDeviceSetFilter is called.
    //
    _Must_inspect_result_
    NTSTATUS
    SetFilter(
        __in BOOLEAN Value
        );

    //
    // Create an IoQueue and associate it with the FxIoPkg per device
    // instance.
    //
    _Must_inspect_result_
    NTSTATUS
    CreateQueue(
        __in  PWDF_IO_QUEUE_CONFIG     Config,
        __in  PWDF_OBJECT_ATTRIBUTES   QueueAttributes,
        __in_opt FxDriver*             Caller,
        __deref_out FxIoQueue**        ppQueue
        );

    VOID
    RemoveQueueReferences(
        __inout FxIoQueue* pQueue
        );

    _Must_inspect_result_
    NTSTATUS
    ConfigureDynamicDispatching(
        __in UCHAR               MajorFunction,
        __in_opt FxCxDeviceInfo* CxDeviceInfo,
        __in PFN_WDFDEVICE_WDM_IRP_DISPATCH EvtDeviceWdmIrpDispatch,
        __in_opt WDFCONTEXT      DriverContext
        );

    _Must_inspect_result_
    NTSTATUS
    ConfigureForwarding(
        __inout FxIoQueue* TargetQueue,
        __in    WDF_REQUEST_TYPE RequestType
        );

    _Must_inspect_result_
    NTSTATUS
    FlushAllQueuesByFileObject(
        __in MdFileObject FileObject
        );

    __inline
    VOID
    RequestCompletedCallback(
        __in FxRequest* Request
        )
    {
        UNREFERENCED_PARAMETER(Request);

        //
        // If this is called, the driver called WDFREQUEST.Complete
        // from the PreProcess callback handler.
        //
        // Since we have a handler, the FXREQUEST object will
        // dereference itself upon return from this callback, which
        // will destroy the final reference count.
        //
    }

    __inline
    BOOLEAN
    IsTopLevelQueue(
        __in FxIoQueue* Queue
        )
    {
        UCHAR index;

        for (index = 0; index <= IRP_MJ_MAXIMUM_FUNCTION; index++) {
          if (m_DispatchTable[index] == Queue) {
              return TRUE;
          }
        }
        return FALSE;
    }

    NTSTATUS
    DispathToInCallerContextCallback(
        __in    FxIoInCallerContext *InCallerContextInfo,
        __in    FxRequest *Request,
        __inout MdIrp      Irp
        );

    __inline
    FxIoInCallerContext*
    GetIoInCallerContextCallback(
        __in_opt FxCxDeviceInfo* CxDeviceInfo
        )
    {
        if (CxDeviceInfo != NULL) {
            return &CxDeviceInfo->IoInCallerContextCallback;
        }
        else {
            return &m_InCallerContextCallback;
        }
    }

private:

    VOID
    AddIoQueue(
        __inout FxIoQueue* IoQueue
        );

    VOID
    RemoveIoQueue(
        __inout FxIoQueue* IoQueue
        );

    FxIoQueue*
    GetFirstIoQueueLocked(
        __in FxIoQueueNode* QueueBookmark,
        __in PVOID Tag
        );

    FxIoQueue*
    GetNextIoQueueLocked(
        __in FxIoQueueNode* QueueBookmark,
        __in PVOID Tag
        );

    VOID
    GetIoQueueListLocked(
        __in    PSINGLE_LIST_ENTRY SListHead,
        __inout FxIoIteratorList ListType
        );

    _Must_inspect_result_
    NTSTATUS
    VerifierFreeRequestToTestForwardProgess(
        __in FxRequest* Request
        );

};

#endif // _FXPKGIO_H
