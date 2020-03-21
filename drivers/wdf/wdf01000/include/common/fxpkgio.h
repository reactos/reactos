#ifndef _FXPKGIO_H_
#define _FXPKGIO_H_

#include "common/fxpackage.h"
#include "common/fxdevicecallbacks.h"
#include "common/fxioqueue.h"
#include "common/fxrequest.h"

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
class FxPkgIo : public FxPackage {

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

    __inline
    FxIoQueue*
    GetDefaultQueue(
        VOID
        ) 
    {
        return m_DefaultQueue;
    }

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

    //
    // Called by CfxDevice when WdfDeviceSetFilter is called.
    //
    _Must_inspect_result_
    NTSTATUS
    SetFilter( 
        __in BOOLEAN Value
        );

    // Do not specify argument names
    FX_DECLARE_VF_FUNCTION_P1(
    NTSTATUS, 
    VerifyDispatchContext, 
        _In_ WDFCONTEXT
        );

    __inline
    FxIoInCallerContext*
    GetIoInCallerContextCallback(
        __in_opt FxCxDeviceInfo* CxDeviceInfo
        )
    {
        if (CxDeviceInfo != NULL)
        {
            return &CxDeviceInfo->IoInCallerContextCallback;
        }
        else
        {
            return &m_InCallerContextCallback;
        }
    }

    NTSTATUS 
    DispathToInCallerContextCallback(
        __in    FxIoInCallerContext *InCallerContextInfo,
        __in    FxRequest *Request,
        __inout MdIrp      Irp
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
    // This is called to start, or resume processing when PNP/Power
    // resources have been supplied to the device driver.
    //
    // The driver is notified of resumption for any in-flight I/O.
    //
    _Must_inspect_result_
    NTSTATUS
    ResumeProcessingForPower();

private:

    _Must_inspect_result_
    NTSTATUS
    VerifierFreeRequestToTestForwardProgess(
        __in FxRequest* Request
        );

    VOID
    AddIoQueue(
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
};

#endif //_FXPKGIO_H_