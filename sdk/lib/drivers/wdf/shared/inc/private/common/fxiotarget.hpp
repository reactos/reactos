/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    FxIoTarget.hpp

Abstract:

    Encapsulation of the target to which FxRequest are sent to.  For example,
    an FxTarget could represent the next device object in the pnp stack.
    Derivations from this class could include bus specific formatters or device
    objects outside of the pnp stack of the device.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef _FXIOTARGET_H_
#define _FXIOTARGET_H_


struct FxIoContext : public FxRequestContext {

    FxIoContext(
        VOID
        );

    virtual
    ~FxIoContext(
        VOID
        );

    VOID
    StoreAndReferenceOtherMemory(
        __in FxRequestBuffer* Buffer
        )
    {
        _StoreAndReferenceMemoryWorker(this, &m_OtherMemory, Buffer);
    }

    virtual
    VOID
    ReleaseAndRestore(
        __in FxRequestBase* Request
        );

    VOID
    ClearBuffer(
        VOID
        );

    VOID
    SetBufferAndLength(
        __in PVOID Buffer,
        __in size_t   BufferLength,
        __in BOOLEAN CopyBackToBuffer
        );

    VOID
    CopyParameters(
        __in FxRequestBase* Request
        );

    VOID
    CaptureState(
        __in FxIrp* Irp
        );

    VOID
    SwapIrpBuffer(
        _In_ FxRequestBase* Request,
        _In_ ULONG NewInputBufferCb,
        _In_reads_bytes_opt_(NewInputBufferCb) PVOID NewInputBuffer,
        _In_ ULONG NewOutputBufferCb,
        _In_reads_bytes_opt_(NewOutputBufferCb) PVOID NewOutputBuffer
        );

public:

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    PVOID m_BufferToFree;
    PVOID m_OriginalSystemBuffer;
    PVOID m_OriginalUserBuffer;
    PMDL m_MdlToFree;
    union {
        PMDL m_OriginalMdl;
        PFX_DRIVER_GLOBALS m_DriverGlobals;
    };

    ULONG m_OriginalFlags;

    size_t  m_BufferToFreeLength;
    size_t  m_MdlToFreeSize;
    BOOLEAN m_CopyBackToBuffer;
    BOOLEAN m_UnlockPages;
#else
    //
    // Captured state of the IRP before buffers are modified by Format
    //
    WUDFX_IRP_BUFFER_INFO m_OriginalBufferInfo;
#endif

    BOOLEAN m_RestoreState;
    UCHAR m_MajorFunction;
    IFxMemory* m_OtherMemory;
};

struct FxInternalIoctlOthersContext : public FxRequestContext {

    FxInternalIoctlOthersContext(
        VOID
        ) :
        FxRequestContext(FX_RCT_INTERNAL_IOCTL_OTHERS)
    {
        RtlZeroMemory(&m_MemoryObjects[0], sizeof(m_MemoryObjects));
    }

    VOID
    StoreAndReferenceOtherMemories(
        __in FxRequestBuffer* Buffer1,
        __in FxRequestBuffer* Buffer2,
        __in FxRequestBuffer* Buffer4
        )
    {
        StoreAndReferenceMemory(Buffer1);
        _StoreAndReferenceMemoryWorker(this, &m_MemoryObjects[0], Buffer2);
        _StoreAndReferenceMemoryWorker(this, &m_MemoryObjects[1], Buffer4);
    }

    virtual
    VOID
    ReleaseAndRestore(
        __in FxRequestBase* Request
        )
    {
        ULONG i;

        for (i = 0;
             i < sizeof(m_MemoryObjects)/sizeof(m_MemoryObjects[0]);
             i++) {

            if (m_MemoryObjects[i] != NULL) {
                m_MemoryObjects[i]->RELEASE(this);
                m_MemoryObjects[i] = NULL;
            }
        }

        __super::ReleaseAndRestore(Request);
    }

private:
    virtual
    VOID
    StoreAndReferenceMemory(
        __in FxRequestBuffer* Buffer
        )
    {
        __super::StoreAndReferenceMemory(Buffer);
    }

public:
    //
    // __super has a field for one IFxMemory, so we don't need to store all
    // 3 in the derivative, reuse the __super's field for one of them.
    //
    IFxMemory* m_MemoryObjects[FX_REQUEST_NUM_OTHER_PARAMS-1];
};

struct FxTargetSubmitSyncParams {
    //
    // Event to set if the request is synchronous after the request has completed
    //
    FxCREvent SynchEvent;

    //
    // Status of the request if it was synchronous
    //
    NTSTATUS Status;

    //
    // Original completion routine to be called in the synchronous case
    //
    PFN_WDF_REQUEST_COMPLETION_ROUTINE OrigTargetCompletionRoutine;

    //
    // Original completion context to be passed in the synchronous case
    //
    WDFCONTEXT OrigTargetCompletionContext;
};

enum SubmitActionFlags {
    SubmitSend                  = 0x00000001,
    SubmitQueued                = 0x00000002,
    SubmitSent                  = 0x00000004,
    SubmitWait                  = 0x00000008,
    SubmitTimeout               = 0x00000010,
    SubmitSyncCallCompletion    = 0x00000020,
};

class FxIoTarget : public FxNonPagedObject {

    friend FxRequestBase;

public:

    FxIoTarget(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in USHORT ObjectSize
        );

    FxIoTarget(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in USHORT ObjectSize,
        __in WDFTYPE WdfType
        );

    virtual
    _Must_inspect_result_
    NTSTATUS
    Start(
        VOID
        );

    virtual
    VOID
    Stop(
        __in WDF_IO_TARGET_SENT_IO_ACTION Action
        );

    virtual
    VOID
    Purge(
        __in WDF_IO_TARGET_PURGE_IO_ACTION Action
        );

    virtual
    VOID
    Remove(
        VOID
        );

    //
    // IFxObject override
    //
    NTSTATUS
    _Must_inspect_result_
    QueryInterface(
        __inout FxQueryInterfaceParams* Params
        );

    __inline
    WDF_IO_TARGET_STATE
    GetState(
        VOID
        )
    {
        return m_State;
    }

    __inline
    MdDeviceObject
    GetTargetDevice(
        VOID
        )
    {
        return m_TargetDevice;
    }

    __inline
    MdDeviceObject
    GetTargetPDO(
        VOID
        )
    {
        return m_TargetPdo;
    }

    __inline
    MdFileObject
    GetTargetFileObject(
        VOID
        )
    {
        return m_TargetFileObject;
    }

    __inline
    WDFDEVICE
    GetDeviceHandle(
        VOID
        )
    {
        return m_Device->GetHandle();
    }

    WDFIOTARGET
    GetHandle(
        VOID
        )
    {
        return (WDFIOTARGET) GetObjectHandle();
    }

    __inline
    FxDriver*
    GetDriver(
        VOID
        )
    {
        return m_Driver;
    }

    virtual
    _Must_inspect_result_
    MdDeviceObject
    GetTargetDeviceObject(
        _In_ CfxDeviceBase* Device
        )
    {
        return Device->GetAttachedDevice();
    }

    _Must_inspect_result_
    NTSTATUS
    Init(
        __in CfxDeviceBase* Device
        );

    ULONG
    Submit(
        __in  FxRequestBase* Request,
        __in_opt PWDF_REQUEST_SEND_OPTIONS Options,
        __in_opt ULONG Flags
        );

    // Do not specify argument names
    FX_DECLARE_VF_FUNCTION_P1(
    NTSTATUS,
    VerifySubmitLocked,
        _In_ FxRequestBase*
        );

    ULONG
    SubmitLocked(
        __in  FxRequestBase* Request,
        __in_opt PWDF_REQUEST_SEND_OPTIONS Options,
        __in ULONG Flags
        );

    _Must_inspect_result_
    NTSTATUS
    SubmitSync(
        __in FxRequestBase* Request,
        __in_opt PWDF_REQUEST_SEND_OPTIONS Options = NULL,
        __out_opt PULONG Action = NULL
        );

    VOID
    TimerCallback(
        __in FxRequestBase* Request
        );

    VOID
    CompleteCanceledRequest(
        __in FxRequestBase* Request
        );

    VOID
    SubmitPendedRequest(
        __in FxRequestBase* Request
        );

    VOID
    CompletePendedRequest(
        __in FxRequestBase* Request
        );

    static
    VOID
    _CancelSentRequest(
        __in FxRequestBase* Request
        );

    BOOLEAN
    __inline
    HasEnoughStackLocations(
        __in FxIrp* Irp
        )
    {
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
        //
        // Check to make sure there are enough current stack locations available.
        // When a IRP is initially created, Irp->CurrentLocation is set to
        // StackSize + 1.  When comparing against the target device, subtract
        // off the extra space to see how many locations are left.
        //
        // Say Target->m_TargetStackSize == 1, then:
        // irp = IoAllocateIrp(Target->m_TargetStackSize, FALSE);
        // ASSERT(irp->CurrentLocation == 2);
        //
        return (Irp->GetCurrentIrpStackLocationIndex() - 1 >= m_TargetStackSize) ? TRUE : FALSE;
#else // FX_CORE_USER_MODE
        //
        // For UMDF, host does the necessary checks to ensure there are enough
        // stack locations. In addition, UMDF drivers can't create WDM IRPs
        // so they don't get to dictate the number of stack locations in the irp
        // so this kind of check in framework for UMDF is redundant. Return TRUE
        // always.
        //
        return TRUE;
#endif
    }

    _Must_inspect_result_
    NTSTATUS
    FormatIoRequest(
        __inout FxRequestBase* Request,
        __in UCHAR MajorCode,
        __in FxRequestBuffer* IoBuffer,
        __in_opt PLONGLONG StartingOffset,
        __in_opt FxFileObject* FileObject = NULL
        );

    _Must_inspect_result_
    NTSTATUS
    FormatIoctlRequest(
        __in FxRequestBase* Request,
        __in ULONG Ioctl,
        __in BOOLEAN Internal,
        __in FxRequestBuffer* InputBuffer,
        __in FxRequestBuffer* OutputBuffer,
        __in_opt FxFileObject* FileObject = NULL
        );

    _Must_inspect_result_
    NTSTATUS
    FormatInternalIoctlOthersRequest(
        __in FxRequestBase* Request,
        __in ULONG Ioctl,
        __in FxRequestBuffer* Buffers
        );

    static
    FxIoTarget*
    _FromEntry(
        __in FxTransactionedEntry* Entry
        )
    {
        return CONTAINING_RECORD(Entry, FxIoTarget, m_TransactionedEntry);
    }

    VOID
    CancelSentIo(
        VOID
    );

    _Must_inspect_result_
    NTSTATUS
    SubmitSyncRequestIgnoreTargetState(
        __in FxRequestBase* Request,
        __in_opt PWDF_REQUEST_SEND_OPTIONS RequestOptions
        );

    VOID
    UpdateTargetIoType(
        VOID
        );

    BOOLEAN
    HasValidStackSize(
        VOID
        );

    virtual
    VOID
    Send(
        _In_ MdIrp Irp
        );

protected:
    //
    // Hide destructor since we are reference counted object
    //
    ~FxIoTarget();

    _Must_inspect_result_
    NTSTATUS
    InitModeSpecific(
        __in CfxDeviceBase* Device
        );

    // FxObject overrides
    virtual
    BOOLEAN
    Dispose(
        VOID
        );
    // FxObject overrides

    VOID
    FailPendedRequest(
        __in FxRequestBase* Request,
        __in NTSTATUS Status
        );

    VOID
    DrainPendedRequestsLocked(
        __in PLIST_ENTRY RequestListHead,
        __in BOOLEAN RequestWillBeResent
        );

    VOID
    CompletePendedRequestList(
        __in PLIST_ENTRY RequestListHead
        );

    VOID
    SubmitPendedRequests(
        __in PLIST_ENTRY RequestListHeadHead
        );

    VOID
    GetSentRequestsListLocked(
        __in PSINGLE_LIST_ENTRY RequestListHead,
        __in PLIST_ENTRY SendList,
        __out PBOOLEAN AddedToList
        );

    static
    VOID
    _CancelSentRequests(
        __in PSINGLE_LIST_ENTRY RequestListHead
        );

    virtual
    _Must_inspect_result_
    NTSTATUS
    GotoStartState(
        __in PLIST_ENTRY    RequestListHead,
        __in BOOLEAN        Lock = TRUE
        );

    virtual
    VOID
    GotoStopState(
        __in WDF_IO_TARGET_SENT_IO_ACTION   Action,
        __in PSINGLE_LIST_ENTRY             SentRequestListHead,
        __out PBOOLEAN                      Wait,
        __in BOOLEAN                        LockSelf
        );

    virtual
    VOID
    GotoPurgeState(
        __in WDF_IO_TARGET_PURGE_IO_ACTION  Action,
        __in PLIST_ENTRY                    PendedRequestListHead,
        __in PSINGLE_LIST_ENTRY             SentRequestListHead,
        __out PBOOLEAN                      Wait,
        __in BOOLEAN                        LockSelf
        );

    _Must_inspect_result_
    NTSTATUS
    PendRequestLocked(
        __in FxRequestBase* Request
        );

    __inline
    VOID
    CompleteRequest(
        __in FxRequestBase* Request
        )
    {
        //
        // This will remove the reference taken by this object on the request
        //
        Request->CompleteSubmitted();
    }

    //
    // Completion routine to handle the case when re-submitting a pended
    // request fails.
    //
    VOID
    HandleFailedResubmit(
        __in FxRequestBase* Request
        );

    //
    // Generic I/O completion routine and its static caller.
    //
    VOID
    RequestCompletionRoutine(
        __in FxRequestBase* Request
        );

    static
    MdCompletionRoutineType
    _RequestCompletionRoutine;

    BOOLEAN
    RemoveCompletedRequestLocked(
        __in FxRequestBase* Request
        );

    virtual
    VOID
    ClearTargetPointers(
        VOID
        )
    {
        m_TargetDevice = NULL;
        m_TargetPdo = NULL;
        m_TargetFileObject = NULL;

        m_TargetStackSize = 0;
        m_TargetIoType = WdfDeviceIoUndefined;
    }

    UCHAR
    GetTargetIoType(
        VOID
        )
    {
        ULONG flags;
        MxDeviceObject deviceObject(m_TargetDevice);

        flags = deviceObject.GetFlags();

        if (flags & DO_BUFFERED_IO) {
            return WdfDeviceIoBuffered;
        }
        else if (flags & DO_DIRECT_IO) {
            return WdfDeviceIoDirect;
        }
        else {
            return WdfDeviceIoNeither;
        }
    }

    static
    VOID
    _RequestCancelled(
        __in FxIrpQueue* Queue,
        __in MdIrp Irp,
        __in PMdIoCsqIrpContext pCsqContext,
        __in KIRQL CallerIrql
        );

    static
    EVT_WDF_REQUEST_COMPLETION_ROUTINE
    _SyncCompletionRoutine;

    virtual
    VOID
    GotoRemoveState(
        __in WDF_IO_TARGET_STATE NewState,
        __in PLIST_ENTRY PendedRequestListHead,
        __in PSINGLE_LIST_ENTRY SentRequestListHead,
        __in BOOLEAN Lock,
        __out PBOOLEAN Wait
        );

    virtual
    VOID
    WaitForSentIoToComplete(
        VOID
        )
    {
        ASSERT(Mx::MxGetCurrentIrql() == PASSIVE_LEVEL);

        m_SentIoEvent.EnterCRAndWaitAndLeave();
    }

    virtual
    VOID
    WaitForDisposeEvent(
        VOID
        );

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    //
    //Making it a virtual function so that derived classes can override it
    //For example, CWdfIoTargetLocal overrides it to set the file object
    //before forwarding the request
    //
    virtual
    VOID
    Forward(
        __in MdIrp Irp
        )
    {
        //
        // Ignore the return value because once we have sent the request, we
        // want all processing to be done in the completion routine.
        //
        (void) Irp->Forward();
    }
#endif































    __inline
    VOID
    CopyFileObjectAndFlags(
        __in FxRequestBase* Request
        )
    {
        FxIrp* irp = Request->GetSubmitFxIrp();

        if (Request->IsAllocatedFromIo()) {
            irp->SetNextStackFlags(irp->GetCurrentStackFlags());
            irp->SetNextStackFileObject(irp->GetCurrentStackFileObject());
        }

        //
        // Use the target's fileobject if present, otherwise use the current
        // stack location's fileobject (if there is a current stack location).
        //
        if (m_InStack == FALSE) {
            irp->SetNextStackFileObject(m_TargetFileObject);
        }
    }



    __inline
    VOID
    IncrementIoCount(
        VOID
        )
    {
        LONG ret;

        ret = InterlockedIncrement(&m_IoCount);

#if DBG
        ASSERT(ret > 1);
#else
        UNREFERENCED_PARAMETER(ret);
#endif
    }


    __inline
    VOID
    DecrementIoCount(
        VOID
        )
    {
        LONG ret;

        ret = InterlockedDecrement(&m_IoCount);
        ASSERT(ret >= 0);

        if (ret == 0) {
            PrintDisposeMessage();
            ASSERT(m_DisposeEvent != NULL);
            m_DisposeEvent->Set();
        }
    }

    VOID
    PrintDisposeMessage(
        VOID
        );

private:

    VOID
    Construct(
        VOID
        );

    VOID
    ClearCompletedRequestVerifierFlags(
        __in FxRequestBase* Request
        )
    {
        if (GetDriverGlobals()->FxVerifierOn &&
            GetDriverGlobals()->FxVerifierIO) {
            KIRQL irql;

            Request->Lock(&irql);
            //
            // IF we are completing a request that was pended in the target,
            // this flag was not set.
            //
            // ASSERT(Request->GetVerifierFlagsLocked() & FXREQUEST_FLAG_SENT_TO_TARGET);
            Request->ClearVerifierFlagsLocked(FXREQUEST_FLAG_SENT_TO_TARGET);
            Request->Unlock(irql);
        }
    }

    VOID
    SetCompletionRoutine(
        __in FxRequestBase* Request
        )
    {
        FxIrp* irp = Request->GetSubmitFxIrp();

        irp->SetCompletionRoutineEx(
                m_InStackDevice,
                _RequestCompletionRoutine,
                Request,
                TRUE,
                TRUE,
                TRUE);
    }

public:
    //
    // Transaction entry for FxDevice to queue this target on
    //
    FxTransactionedEntry m_TransactionedEntry;

    BOOLEAN m_InStack;

    //
    // TRUE when FxDevice::AddIoTarget has been called
    //
    BOOLEAN m_AddedToDeviceList;

    static const PVOID m_SentRequestTag;

protected:
    //
    // List of requests that have been sent to the target
    //
    LIST_ENTRY m_SentIoListHead;

    //
    // List of requests which were sent ignoring the state of the target
    //
    LIST_ENTRY m_IgnoredIoListHead;

    //
    // Event used to wait for sent I/O to complete
    //
    FxCREvent m_SentIoEvent;

    //
    // Event used to wait by Dispose to make sure all I/O's are completed.
    // This is required to make sure that all the I/O are completed before
    // disposing the target. This acts like remlock.
    //
    FxCREvent *m_DisposeEvent;

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    //
    // Eventy initialization can fail in user-mode so we define one as
    // part of object.
    //
    FxCREvent m_DisposeEventUm;
#endif

    FxIrpQueue m_PendedQueue;

    //
    // Back link to the object that represents our devobj
    //
    FxDriver* m_Driver;

    //
    // The PDEVICE_OBJECT that is owned by m_Device
    //
    MdDeviceObject m_InStackDevice;

    //
    // The device object which is our "target"
    //
    MdDeviceObject m_TargetDevice;

    //
    // The PDO for m_TargetDevice.  For this class, it would be the same PDO
    // as the owning WDFDEVICE.  In a derived class (like FxIoTargetRemote),
    // this would not be the PDO of the owning WDFDEVICE, rather the PDO for
    // the other stack.
    //
    MdDeviceObject m_TargetPdo;

    //
    // File object that is attached to all I/O sent to m_TargetDevice
    //
    MdFileObject m_TargetFileObject;

    //
    // Current state
    //
    WDF_IO_TARGET_STATE m_State;

    //
    // This is used to track the I/O's sent to the lower driver
    // and is used to make sure all I/Os are completed before disposing the
    // Iotarget.
    //
    LONG  m_IoCount;

    //
    // Cached value of m_TargetDevice->StackSize.  The value is cached so that
    // we can still format to the target during query remove transitions.
    //
    CCHAR m_TargetStackSize;

    //
    // Cached value of m_TargetDevice->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO)
    // which uses WDF_DEVICE_IO_TYPE to indicate state.
    //
    UCHAR m_TargetIoType;

    //
    // TRUE if we are in the processing of stopping/purging and there are
    // requests that have been sent and must be waited upon for completion.
    //
    BOOLEAN m_WaitingForSentIo;

    BOOLEAN m_Removing;

};


#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
#include "FxIoTargetKm.hpp"
#else if ((FX_CORE_MODE)==(FX_CORE_USER_MODE))
#include "FxIoTargetUm.hpp"
#endif

#endif //_FXIOTARGET_H_
