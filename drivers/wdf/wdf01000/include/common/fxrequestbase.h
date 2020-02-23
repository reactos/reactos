#ifndef _FXREQUESTBASE_H_
#define _FXREQUESTBASE_H_

#include "common/fxnonpagedobject.h"
#include "common/fxrequestcontext.h"
#include "common/mxtimer.h"
#include "common/fxirpqueue.h"
#include "common/fxrequestcallbacks.h"
#include "common/fxioqueuecallbacks.h"


//
// COMPLETED - set when the request's i/o completion routine has executed
//
// PENDED - set when the request has been put onto the target's CSQ
//
// TIMER_SET - set when a timer has been queued along with sending the request
//             down to the target
//
// CANCELLED_FROM_TIME - set by the timer to indicate that the request was
//                       cancelled by the timer DPC
//
// IGNORE_STATE - set when the request is going to be sent ignoring the 
//                state of the target.
//
enum FxRequestTargetFlags {
    FX_REQUEST_COMPLETED                = 0x01,
    FX_REQUEST_PENDED                   = 0x02,
    FX_REQUEST_TIMER_SET                = 0x04,
    FX_REQUEST_CANCELLED_FROM_TIMER     = 0x08,
    FX_REQUEST_IGNORE_STATE             = 0x10,
};

//
// internal private constraints
//
#define WDF_REQUEST_SEND_INTERNAL_OPTION_FAIL_ON_PEND (0x80000000)

#define WDF_REQUEST_INTERNAL_CONSTRAINTS_VALID_FLAGS \
    (WDF_REQUEST_SEND_INTERNAL_OPTION_FAIL_ON_PEND)

class FxIoTarget;
class FxRequest;
//
// Completion event callback prototype
//
typedef
VOID
(*PFN_COMPLETE_COPY_ROUTINE)(
    __in FxIoTarget* This,
    __in FxRequest* Request,
    __in_opt FxRequestContext* Context
    );


struct FxRequestTimer : public FxStump {
    MxTimer Timer;
};

enum FxRequestAllocationSource {
    REQUEST_ALLOCATED_FROM_IO = 0,  // Irp came from the I/O package
    REQUEST_ALLOCATED_INTERNAL = 1, // Irp was allocated internally and should be freed by the request
    REQUEST_ALLOCATED_DRIVER = 2,   // Irp was given to the request by the driver writer
};

enum FxRequestIrpOwnership {
    FxRequestOwnsIrp = 1,
    FxRequestDoesNotOwnIrp,
};

// begin_wpp config
// CUSTOM_TYPE(FxRequestIrpOwnership, ItemEnum(FxRequestIrpOwnership));
// end_wpp

enum FxRequestConstructorCaller {
    FxRequestConstructorCallerIsFx = 1,
    FxRequestConstructorCallerIsDriver,
};

//
// These defines are for VerifierFlags
//
enum FxRequestVerifierFlags {
    // Request has been passed to the Driver
    FXREQUEST_FLAG_DRIVER_OWNED             = 0x0001,

    // Request was returned as a "Tag" request to WdfIoQueuePeekNextRequest.
    FXREQUEST_FLAG_TAG_REQUEST              = 0x0002,

    // Request has been forwarded from one queue to another
    FXREQUEST_FLAG_FORWARDED                = 0x0004,

    // Request is being EvtIoDefault to the driver
    FXREQUEST_FLAG_DRIVER_DISPATCH          = 0x0008,

    // The driver has specified the request as cancelable
    FXREQUEST_FLAG_DRIVER_CANCELABLE        = 0x0010,

    FXREQUEST_FLAG_DRIVER_INPROCESS_CONTEXT = 0x0020,

    // The request has been cancelled
    FXREQUEST_FLAG_CANCELLED                = 0x0040,

    // the next stack location has been formatted
    FXREQUEST_FLAG_FORMATTED                = 0x0080,

    // the request has been sent on an I/O target and is in the target's sent list
    FXREQUEST_FLAG_SENT_TO_TARGET           = 0x0100,

    // used to make sure the driver stop acknowledges in the context of EvtIoStop
    FXREQUEST_FLAG_DRIVER_IN_EVTIOSTOP_CONTEXT = 0x0200,

    // used to indicate whether the Reserved Request is in use or on Free list
    FXREQUEST_FLAG_RESERVED_REQUEST_ASSOCIATED_WITH_IRP = 0x0400,
};

enum FxRequestBaseFlags {
    FxRequestBaseSystemMdlMapped  = 0x1,
    FxRequestBaseOutputMdlMapped  = 0x2,
    FxRequestBaseSyncCleanupContext = 0x10,
};

enum FxRequestBaseStaticFlags {
    FxRequestBaseStaticSystemBufferValid  = 0x1,
    FxRequestBaseStaticOutputBufferValid  = 0x2,
};

//
// Designed to fit into a byte.  FxRequestCompletionPkgFlag is a bit value
// used to distinguish between calling back into the io package or the current
// queue.  When calling back into the current queue, we assume m_IoQueue is valid
//
enum FxRequestCompletionState {
    FxRequestCompletionStateIoPkgFlag = 0x80,

    FxRequestCompletionStateNone  = 0x00,
    FxRequestCompletionStateQueue = 0x01,
    FxRequestCompletionStateIoPkg = 0x02 | FxRequestCompletionStateIoPkgFlag,
};

class FxRequestBase : public FxNonPagedObject {

    friend FxIoTarget;

public:

    __inline
    WDFREQUEST
    GetHandle(
        VOID
        )
    {
        return (WDFREQUEST) GetObjectHandle();
    }

    __inline
    static
    FxRequestBase*
    _FromCsqContext(
        __in PMdIoCsqIrpContext Context
        )
    {
        return CONTAINING_RECORD(Context, FxRequestBase, m_CsqContext);
    }

    __inline
    PVOID
    GetTraceObjectHandle(
        VOID
        )
    {
        PVOID handle;

        handle = GetObjectHandle();

        if (handle != NULL)
        {
            return handle;
        }
        else
        {
            return (PVOID) this;
        }
    }

    __inline
    UCHAR
    GetTargetFlags(
        VOID
        )
    {
        // Assumes caller is holding appropriate lock
        return m_TargetFlags;
    }

    __inline
    ULONG
    ClearTargetFlags(
        __in UCHAR Flags
        )
    {
        ULONG oldFlags;

        oldFlags = m_TargetFlags;

        // Assumes caller is holding appropriate lock
        m_TargetFlags &= ~Flags;

        return oldFlags;
    }

    __inline
    FxIrp*
    GetSubmitFxIrp(
        VOID
        )
    {
        return &m_Irp;
    }

    __inline
    VOID
    SetTargetFlags(
        __in UCHAR Flags
        )
    {
        // Assumes caller is holding appropriate lock
        m_TargetFlags |= Flags;
    }

    _Must_inspect_result_
    BOOLEAN
    CancelTimer(
        VOID
        );

    BOOLEAN
    __inline
    IsCanComplete(
        VOID
        )
    {
        return m_CanComplete;
    }

    VOID
    CompleteSubmitted(
        VOID
        );

    __inline
    BOOLEAN
    CanComplete(
        VOID
        )
    {
        LONG count;
        count = InterlockedDecrement(&m_IrpCompletionReferenceCount);
        ASSERT(count >= 0);
        return count == 0 ? TRUE : FALSE;
    }

    __inline
    VOID
    ClearVerifierFlagsLocked(
        __in SHORT Flags
        )
    {
        m_VerifierFlags &= ~Flags;
    }

    __inline
    VOID
    ClearVerifierFlags(
        __in SHORT Flags
        )
    {
        KIRQL irql;

        ASSERT(GetDriverGlobals()->FxVerifierOn);

        Lock(&irql);
        ClearVerifierFlagsLocked(Flags);
        Unlock(irql);
    }

    __inline
    VOID
    VerifierClearFormatted(
        VOID
        )
    {
        if (GetDriverGlobals()->FxVerifierOn &&
            GetDriverGlobals()->FxVerifierIO)
        {
            ClearVerifierFlags(FXREQUEST_FLAG_FORMATTED);
        }
    }

    PFN_WDF_REQUEST_COMPLETION_ROUTINE
    ClearCompletionRoutine(
        VOID
        )
    {
        PFN_WDF_REQUEST_COMPLETION_ROUTINE pRoutine;

        pRoutine = m_CompletionRoutine.m_Completion;
        m_CompletionRoutine.m_Completion = NULL;

        return pRoutine;
    }

    WDFCONTEXT
    ClearCompletionContext(
        VOID
        )
    {
        WDFCONTEXT pContext;

        pContext = m_TargetCompletionContext;
        m_TargetCompletionContext = NULL;

        return pContext;
    }

    __inline
    VOID
    SetVerifierFlagsLocked(
        __in SHORT Flags
        )
    {
        m_VerifierFlags |= Flags;
    }

    __inline
    VOID
    SetVerifierFlags(
        __in SHORT Flags
        )
    {
        KIRQL irql;

        ASSERT(GetDriverGlobals()->FxVerifierOn);

        Lock(&irql);
        SetVerifierFlagsLocked(Flags);
        Unlock(irql);
    }

    VOID
    __inline
    SetPriorityBoost(
        CCHAR PriorityBoost
        )
    {
        m_PriorityBoost = PriorityBoost;
    }

    SHORT
    GetVerifierFlagsLocked(
        VOID
        )
    {
        ASSERT(GetDriverGlobals()->FxVerifierOn);
        return m_VerifierFlags;
    }

    __inline
    SHORT
    GetVerifierFlags(
        VOID
        )
    {
        SHORT flags;
        KIRQL irql;

        Lock(&irql);
        flags =  GetVerifierFlagsLocked();
        Unlock(irql);

        return flags;
    }

    BOOLEAN
    __inline
    IsAllocatedFromIo(
        VOID
        )
    {
        return m_IrpAllocation == REQUEST_ALLOCATED_FROM_IO ? TRUE : FALSE;
    }

    __inline
    BOOLEAN
    IsCancelRoutineSet(
        VOID
        )
    {
        return (m_CancelRoutine.m_Cancel != NULL) ?
                    TRUE : FALSE;
    }

    CCHAR
    __inline
    GetPriorityBoost(
        VOID
        )
    {
        return m_PriorityBoost;
    }

protected:
    FxRequestBase(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in USHORT ObjectSize,
        __in_opt MdIrp Irp,
        __in FxRequestIrpOwnership Ownership,
        __in FxRequestConstructorCaller Caller,
        __in FxObjectType ObjectType = FxObjectTypeExternal
        );

    virtual
    ~FxRequestBase(
        VOID
        );

    VOID
    ZeroOutDriverContext(
        VOID
        )
    {
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    RtlZeroMemory(GetSubmitFxIrp()->GetDriverContext(),
                  GetSubmitFxIrp()->GetDriverContextSize());
#else 
    //
    // UMDF host doesn't expose any easier way to zero out the contexts so
    // set context to NULL one by one.
    //
    GetSubmitFxIrp()->SetContext(0, NULL);
    GetSubmitFxIrp()->SetContext(1, NULL);
    GetSubmitFxIrp()->SetContext(2, NULL);
    GetSubmitFxIrp()->SetContext(3, NULL);
#endif
    }

    VOID
    CompleteSubmittedNoContext(
        VOID
        );


public:

    union {
        //
        // The cancel safe queues use this context to identify
        // the request in a race free manner.
        //
        MdIoCsqIrpContext m_CsqContext;

        //
        // IoTargets uses this to track the request when it is sent to the target.
        // Since the request cannot be on an CSQ and sent to a target at the
        // same time, we can unionize this with the CSQ context.
        //
        LIST_ENTRY   m_ListEntry;
    };

    union {
        //
        // IoTargest uses this when it needs to create a list of requests to cancel
        // when making a state transition
        //
        SINGLE_LIST_ENTRY m_DrainSingleEntry;

        //
        // If TRUE, the driver formatted the request by copying the current stack
        // location to next or by manually passing in an IO_STACK_LOCATION.  This
        // is union'ed with m_DrainSingleEntry b/c it is only relevant during
        // send and forget and the request is never enqueued on the target in
        // this case and m_DrainSingleEntry is used when tracking requests sent
        // on a target for cancelation due to a target state change.
        //
        BOOLEAN m_NextStackLocationFormatted;
    };


protected:

    //
    // The NT IRP is wrapped by a frameworks FxIrp
    //
    // Note: If m_Irp is NULL after initialization, this means
    //       the IRP was cancelled by a FxIrpQueue cancellation
    //       callback, or completed.
    //
    FxIrp              m_Irp;

    //
    // Target of the request.  Access to this field is unguarded.  The following
    // have access to this field
    // o The owning target itself
    // o _TimerDPC()
    // o Cancel() IFF it has successfully incremented the irp completion ref count
    //
    FxIoTarget* m_Target;

    FxRequestContext* m_RequestContext;

    FxRequestTimer* m_Timer;

    //
    // Client driver completion routine to call when the request has come back
    // from the target device.
    //
    FxRequestCancelCallback m_CancelRoutine;

    //
    // Synchronization for this field is through Interlocked operations
    //
    LONG m_IrpCompletionReferenceCount;

    FxRequestCompletionCallback m_CompletionRoutine;

    //
    // Context to pass to CompletionRoutine when called
    //
    WDFCONTEXT m_TargetCompletionContext;

    //
    // Access to flags guarded by FxIoTarget::Lock
    //
    // Values defined in the enum FxRequestTargetFlags
    //
    union {
        UCHAR m_TargetFlags;

        //
        // These are used purely for debugging, not in live code anywhere!
        // NOTE:  if FxRequestTargetFlagschanges, so this this union
        //
        struct {
            UCHAR Completed : 1;
            UCHAR FlagsPended : 1;
            UCHAR TimerSet : 1;
            UCHAR CancelledFromTimer : 1;
            UCHAR IgnoreState : 1;
        } m_TargetFlagsByName;
    };

    //
    // Contains a value from the enum type FxRequestAllocationSource describing
    // how the irp was allocated
    //
    UCHAR m_IrpAllocation;

    BOOLEAN m_Completed;

    BOOLEAN m_Canceled;

    WDFOBJECT_OFFSET_ALIGNED m_SystemBufferOffset;

    union {
        //
        // These are flags used by verifier.  Set with values from the enum
        // FxRequestVerifierFlags
        //
        SHORT m_VerifierFlags;

        struct {
            SHORT DriverOwned : 1;
            SHORT TagRequest : 1;
            SHORT Forwarded : 1;
            SHORT DriverDispatch : 1;
            SHORT DriverCancelable : 1;
            SHORT DriverInprocessContext : 1;
            SHORT Cancelled : 1;
            SHORT Formatted : 1;
            SHORT SentToTarget : 1;
            SHORT DriverInEvtIoStopContext : 1;
        } m_VeriferFlagsByName;
    };

    //
    // If this is !=0, its an indication of outstanding references
    // on WDM IRP fields such as any system buffers.
    //
    LONG m_IrpReferenceCount;

    // This field !=NULL if the request is on an IrpQueue.
    FxIrpQueue*        m_IrpQueue;

    WDFOBJECT_OFFSET_ALIGNED m_OutputBufferOffset;

    union {
        //
        // Bit field.  Set with values from the enum FxRequestBaseFlags
        //
        UCHAR m_RequestBaseFlags;

        struct {
            UCHAR SystemMdlMapped : 1;
            UCHAR OutputMdlMapped : 1;
            UCHAR SyncCleanupContext : 1;
        } m_RequestBaseFlagsByName;
    };

    union {
        //
        // Bit field.  Set with values from the enum FxRequestBaseStaticFlags
        //

        //
        UCHAR m_RequestBaseStaticFlags;

        struct {
            UCHAR SystemBufferValid  : 1;
            UCHAR OutputBufferValid  : 1;
        } m_RequestBaseStaticFlagsByName;
    };

    //
    // Priority boost.
    //
    CCHAR m_PriorityBoost;
    
    //
    // Contains values from FxRequestCompletionState
    //
    BYTE m_CompletionState;

    //
    // TRUE, request can be completed by the driver.
    //
    BOOLEAN m_CanComplete;
    
    //
    // If !=NULL, MDL allocated and assocated with the request
    //
    PMDL m_AllocatedMdl;

};

#endif //_FXREQUESTBASE_H_
