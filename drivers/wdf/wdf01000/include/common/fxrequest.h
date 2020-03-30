/*
Abstract:

    This is the request object for the driver frameworks.

    The request object wraps the IRP, containing persistent
    information required by the driver frameworks.

Environment:

    Both kernel and user mode

--*/

#ifndef _FXREQUEST_H_
#define _FXREQUEST_H_

#include "common/fxrequestbase.h"
#include "common/fxirp.h"
#include "common/ifxmemory.h"


#define KMDF_ONLY_CODE_PATH_ASSERT()

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
#define WDF_REQUEST_SEND_OPTION_IMPERSONATION_FLAGS                   \
        (WDF_REQUEST_SEND_OPTION_IMPERSONATE_CLIENT |                 \
         WDF_REQUEST_SEND_OPTION_IMPERSONATION_IGNORE_FAILURE)

#define FxIrpStackFlagsFromSendFlags(sendFlags)                       \
               ((WUDFX_IRP_STACK_FLAGS)                               \
                    ((sendFlags) & WDF_REQUEST_SEND_OPTION_IMPERSONATION_FLAGS))
#endif


class FxIoQueue;
class FxRequest;

typedef
NTSTATUS
(*PFN_FX_QUEUE_REQUEST_COMPLETE) (
    __in FxRequest* Request,
    __in FxIoQueue* Queue,
    __in_opt WDFCONTEXT Context
    );

struct FxRequestSystemBuffer : public IFxMemory {
    
    friend FxRequest;

    FxRequestSystemBuffer(
        VOID
        )
    {
        m_Buffer = NULL;
    }

    __inline
    BOOLEAN
    IsBufferSet(
        VOID
        )
    {
        return m_Buffer != NULL ? TRUE : FALSE;
    }

    _Must_inspect_result_
    virtual
    PVOID
    GetBuffer(
        VOID
        );

    virtual
    size_t
    GetBufferSize(
        VOID
        );
    
    _Must_inspect_result_
    virtual
    PMDL
    GetMdl(
        VOID
        );

    virtual
    WDFMEMORY
    GetHandle(
        VOID
        );

    virtual
    PFX_DRIVER_GLOBALS
    GetDriverGlobals(
        VOID
        );

protected:

    __inline
    VOID
    SetBuffer(
        PVOID Buffer
        )
    {
        ASSERT(m_Buffer == NULL);
        m_Buffer = Buffer;
    }

    __inline
    VOID
    SetMdl(
        PMDL Mdl
        )
    {
        ASSERT(m_Mdl == NULL);
        m_Mdl = Mdl;
    }

    FxRequest*
    GetRequest(
        VOID
        );

    //
    // The current irp stack location indicates which type to use
    //
    // The buffer / MDL is cached away as a copy instead of using the PIRP values
    // directly because we want to capture the current state of the irp when
    // returning the WDFMEMORY.  For instance, if used the PIRP value directly
    // when implementing GetBuffer(), we are subject to the PIRP being formatted
    // for the next stack location and changing the buffer pointer, or worse,
    // changing the MDL value and have the resulting MDL not be mapped, and then
    // a call to MmGetSystemAddressForMdlSafe can return NULL, and thus GetBuffer(),
    // return NULL, which would violate the contract for GetBuffer().
    //
    // As an example, let's
    // 1) the WDFREQUEST/PIRP comes in as a read on a direct io device object,
    //    so Irp->MdlAddress = <read MDL>
    // 2) This WDFMEMORY will return <read MDL VA> in GetBuffer()
    // 3) the WDFREQUEST is formatted for an IOCTL which is METHOD_OUT_DIRECT
    //    with a new output buffer.  Irp->MdlAddres = <IOCTL MDL> now.
    // 4) This same WDFMEMORY will now return <IOCTL MDL VA> in GetBuffer()
    //
    // Essentialy, formatting the WDFREQUEST causes unintended side affects for
    // the WDFMEMORYs it returns.  To eliminate those side affects, we capture
    // the original buffer.
    //
    union {
        PVOID m_Buffer;
        PMDL m_Mdl;
    };

};

struct FxRequestOutputBuffer : public IFxMemory {
    friend FxRequest;

    FxRequestOutputBuffer(
        VOID
        )
    {
        m_Buffer = NULL;
    }

    __inline
    BOOLEAN
    IsBufferSet(
        VOID
        )
    {
        return m_Buffer != NULL ? TRUE : FALSE;
    }

    virtual
    PVOID
    GetBuffer(
        VOID
        );

    virtual
    size_t
    GetBufferSize(
        VOID
        );

    _Must_inspect_result_
    virtual
    PMDL
    GetMdl(
        VOID
        );

    virtual
    WDFMEMORY
    GetHandle(
        VOID
        );

    virtual
    PFX_DRIVER_GLOBALS
    GetDriverGlobals(
        VOID
        );

protected:

    __inline
    VOID
    SetBuffer(
        __in PVOID Buffer
        )
    {
        ASSERT(m_Buffer == NULL);
        m_Buffer = Buffer;
    }

    __inline
    VOID
    SetMdl(
        __in PMDL Mdl
        )
    {
        ASSERT(m_Mdl == NULL);
        m_Mdl = Mdl;
    }

    FxRequest*
    GetRequest(
        VOID
        );

    //
    // The current irp stack location indicates which type to use
    //
    // See comments in FxRequestSystemBuffer's union for why we capture the
    // values vs using them directly from the PIRP.
    //
    union {
        PVOID m_Buffer;
        PMDL m_Mdl;
    };

};

//
// Magic number to differentiate between default value and caller provided one
//
#define USE_DEFAULT_PRIORITY_BOOST ((CHAR) 0x7F)

//
// This tag is used to set and clear the completion callback state as the
// ownership of the request transfers from I/O pkg-to-queue or queue-to-queue.
//
#define FXREQUEST_STATE_TAG (PVOID) 'tatS'

//
// This tag is used when the request is added and removed from FxIrpQueue.
//
#define FXREQUEST_QUEUE_TAG (PVOID) 'ueuQ'

//
// This tag is used to take a reference in the completion path.
//
#define FXREQUEST_COMPLETE_TAG (PVOID) 'pmoC'

//
// Use this tag when you want to temporarily hold the object from
// disappearing between unlock and lock operation.
//
#define FXREQUEST_HOLD_TAG (PVOID) 'dloH'

//
// This tag is used to take a reference in the completion path.
//
#define FXREQUEST_FWDPRG_TAG (PVOID) 'PdwF'

//
// This tag is used to take a reference in the completion path for driver created 
// requests that support completion operations.
//
#define FXREQUEST_DCRC_TAG (PVOID) 'CRCD'

// begin_wpp enum
enum FxListEntryNames {
    FxListEntryNameCleanup = 0,

    // this entry is used when the request is owned by the framework
    FxListEntryQueueOwned,

    // this entry is used when the request is presented to the driver
    FxListEntryDriverOwned,

    // this entry is used for forward progress
    FxListEntryForwardProgress
};

enum  FxRequestPowerStopState {
    FxRequestPowerStopUnknown = 0, // Initial state

    // Set when the driver calls WdfRequestStopAcknowledge
    FxRequestPowerStopAcknowledged = 0x1,

    // Set when the driver WdfRequestStopAcknowledge with requeue option
    FxRequestPowerStopAcknowledgedWithRequeue = 0x2,
};



class FxRequest : public FxRequestBase {

    friend FxIoQueue;
    friend FxRequestOutputBuffer;
    friend FxRequestSystemBuffer;

protected:

    FxRequest(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in MdIrp Irp,
        __in FxRequestIrpOwnership Ownership,
        __in FxRequestConstructorCaller Caller,
        __in USHORT ObjectSize
        );

    #if DBG
    FxRequest::~FxRequest(
        VOID
        );
    #endif // DBG

    WDFMEMORY
    GetMemoryHandle(
        __in USHORT Offset
        )
    {
        ULONG_PTR handle;

        //
        // The offset into this object must be self relative.
        //
        ASSERT(*((PUSHORT) WDF_PTR_ADD_OFFSET(this, Offset)) == Offset);

        handle = (ULONG_PTR) WDF_PTR_ADD_OFFSET(this, Offset);

        //
        // Make sure initial value has the flag bits cleared.
        //
        ASSERT((handle  & FxHandleFlagMask) == 0);

        //
        // We always apply the mask.
        //
        handle = handle ^ FxHandleValueMask;

        //
        // Make sure final value (before we set the flag) has the flag bits
        // cleared.
        //
        ASSERT((handle  & FxHandleFlagMask) == 0);

        //
        // This handle is an offset
        handle |= FxHandleFlagIsOffset;

        return (WDFMEMORY) handle;
    }

    _Must_inspect_result_
    virtual
    NTSTATUS
    QueryInterface(
        __in FxQueryInterfaceParams* Params
        );
    
    //
    // This field points to the queue that the request is currently 
    // associated with.
    //
    FxIoQueue* m_IoQueue;

    FxRequestSystemBuffer m_SystemBuffer;

    FxRequestOutputBuffer m_OutputBuffer;

    //
    // This is for use by the owner of the FxRequest which is FxIoQueue OR FxIoTarget
    //
    LIST_ENTRY m_OwnerListEntry;

    LIST_ENTRY m_OwnerListEntry2;

    //
    // This is used by the queue to keep track of all forward progress requests 
    //
    LIST_ENTRY m_ForwardProgressList;

    //
    // Used when the request is a reserved request to track the queue it was 
    // originally allocated for, so that it can be returned to the forward 
    // progress queue for reuse when the request is completed.
    //
    FxIoQueue *m_ForwardProgressQueue;

    //
    // Generic context exposed to other modules.
    //
    PVOID m_InternalContext;
    
    //
    // If TRUE, the client driver has been presented with this WDFREQUEST at
    // least once.
    //
    BOOLEAN m_Presented;

    //
    // For tracking whether the driver has acknowledged power stop/purge notifications.
    //
    BYTE m_PowerStopState;

    //
    // If TRUE, this is a reserved request 
    //
    BOOLEAN   m_Reserved;

    //
    //  If TRUE, this is used to determine how to free the request -
    //  either to the lookaside list or using ExFreePool
    //
    BOOLEAN   m_ForwardRequestToParent;

public:

    // Factory functions to create FxRequest* objects
    _Must_inspect_result_
    static
    NTSTATUS
    _CreateForPackage(
        __in CfxDevice* Device,
        __in PWDF_OBJECT_ATTRIBUTES RequestAttributes,
        __in MdIrp Irp,
        __deref_out FxRequest** Request
        );

    VOID
    FreeRequest(
        VOID
        );

    VOID
    __inline
    SetPresented(
        VOID
        )
    {
        //
        // No need to synchronize setting this value with checking it because
        // we check it in the complete path.  We will not be about to present
        // and completing the request in 2 simultaneous contexts.
        //
        m_Presented = TRUE;
    }

    __inline
    BOOLEAN 
    IsReserved(
        )
    {
        return m_Reserved;
    }

    __inline
    VOID
    SetInternalContext(
        PVOID Context
        )
    {
        ASSERT(NULL == m_InternalContext);
        m_InternalContext = Context;
    }

    FxRequestCompletionState
    SetCompletionState(
        __in FxRequestCompletionState NewState
        );

    //__inline
    NTSTATUS
    Complete(
        __in NTSTATUS Status
    );

    __inline
    NTSTATUS
    CompleteWithInformation(
        __in NTSTATUS Status,
        __in ULONG_PTR Information
        )
    {
        //
        // Complete the request object. If the status is success, get the
        // priority boost for the owning device type, and complete the request.
        //
        m_Irp.SetInformation(Information);
        return Complete(Status);
    }

    _Must_inspect_result_
    NTSTATUS
    GetIrp(
        __deref_out MdIrp* ppIrp
        )
    {
        if (GetDriverGlobals()->FxVerifierIO)
        {
            NTSTATUS status;
            KIRQL irql;

            Lock(&irql);

            status = VerifyRequestIsNotCompleted(GetDriverGlobals());
            if (!NT_SUCCESS(status))
            {
                *ppIrp = NULL;
                status = STATUS_INVALID_DEVICE_REQUEST;
            }
            else
            {
                *ppIrp = m_Irp.GetIrp();
            }

            Unlock(irql);

            return status;
        }
        else
        {
            *ppIrp = m_Irp.GetIrp();
            return STATUS_SUCCESS;
        }
    }

    // Do not specify argument names
    FX_DECLARE_VF_FUNCTION(
    NTSTATUS, 
    VerifyRequestIsNotCompleted
    );

    FxIrp*
    GetFxIrp(
        VOID
        )
    {
        return &m_Irp;
    }

    //
    // Insert the request in the supplied FxIrpQueue
    // and associated it with it.
    //
    _Must_inspect_result_
    NTSTATUS
    InsertTailIrpQueue(
        __in FxIrpQueue* IrpQueue,
        __out_opt ULONG*      pRequestCount
        );

    // Do not specify argument names
    FX_DECLARE_VF_FUNCTION_P1(
    NTSTATUS, 
    VerifyInsertIrpQueue, 
        _In_ FxIrpQueue*
        );

    //
    // Mark that this request is no longer on the IrpQueue
    //
    __inline
    VOID
    MarkRemovedFromIrpQueue(
        VOID
        )
    {
        m_IrpQueue = NULL;
        return;
    }

    __inline
    VOID
    SetCurrentQueue(
        __in FxIoQueue *Queue
        )
    {
        m_IoQueue = Queue;
    }

    //
    // Function to return an FxRequest from an FxIrpQueue
    //
    _Must_inspect_result_
    static
    FxRequest*
    GetNextRequest(
        __in FxIrpQueue*  IrpQueue
        );

    //
    // Normally, this is available by the object implementing
    // IFxListEntry, but currently all callers of this know they
    // are dealing with an FxRequest*.
    //
    // If FxRequests must go on a general typeless list, then
    // the IFxListEntry interface should be added to FxRequest.
    //
    __inline
    PLIST_ENTRY
    GetListEntry(
        __in FxListEntryNames Index
        )
    {
        switch (Index) {
        case FxListEntryQueueOwned:  return &m_OwnerListEntry;
        case FxListEntryDriverOwned:  return &m_OwnerListEntry2;
        case FxListEntryForwardProgress: return &m_ForwardProgressList;       
        default: ASSERT(FALSE); return NULL;
        }
    }

    static
    FxRequest*
    _FromOwnerListEntry(
        __in FxListEntryNames Index,
        __in PLIST_ENTRY OwnerListEntry
        )
    {
        switch (Index) {
        case FxListEntryQueueOwned:
            return CONTAINING_RECORD(OwnerListEntry, FxRequest, m_OwnerListEntry);
        case FxListEntryDriverOwned:
            return CONTAINING_RECORD(OwnerListEntry, FxRequest, m_OwnerListEntry2);
        case FxListEntryForwardProgress:
            return CONTAINING_RECORD(OwnerListEntry, FxRequest, m_ForwardProgressList);
        default:
            ASSERT(FALSE);
            return NULL;
        }
    }

    //
    // Return the FxRequest's CsqContext address
    //
    __inline
    PMdIoCsqIrpContext
    GetCsqContext(
        VOID
        )
    {
        return &m_CsqContext;
    }

    __inline
    BOOLEAN
    IsInIrpQueue(
        __in FxIrpQueue* pIrpQueue
        )
    {
        return pIrpQueue->IsIrpInQueue(GetCsqContext());
    }

    VOID
    ClearPowerStopState(
        VOID
        )
    {
        m_PowerStopState = FxRequestPowerStopUnknown;
    }

    // Do not specify argument names
    FX_DECLARE_VF_FUNCTION(
    VOID, 
    VerifierBreakpoint_RequestEarlyDisposeDeferred
    );

    __inline
    BOOLEAN
    IsPowerStopAcknowledged(
        VOID
        )
    {
        return ((m_PowerStopState == FxRequestPowerStopAcknowledged)
                ||
                (m_PowerStopState == FxRequestPowerStopAcknowledgedWithRequeue))? TRUE : FALSE;
    }

    //
    // Get the IoStack location of the request.
    //
    // Since this returns the pointer to the underlying IRP
    // IO_STACK_LOCATION, it can not be called in a situation
    // which the request is completed out from underneath us.
    //
    // Note: Must implemention a version for the drivers use.
    //       Must interact with completion events from the
    //       driver due to cancel as well.
    //
    //
    PIO_STACK_LOCATION
    GetCurrentIrpStackLocation(
        VOID
        )
    {
        if (GetDriverGlobals()->FxVerifierIO)
        {
            PIO_STACK_LOCATION ios;
            KIRQL irql;
            NTSTATUS status;

            Lock(&irql);
            status = VerifyRequestIsNotCompleted(GetDriverGlobals());
            if (!NT_SUCCESS(status))
            {
                ios = NULL;
            }
            else
            {
                ios = m_Irp.GetCurrentIrpStackLocation();
            }
            Unlock(irql);

            return ios;
        }
        else
        {
            return m_Irp.GetCurrentIrpStackLocation();
        }
    }

    __inline
    BOOLEAN
    IsPowerStopAcknowledgedWithRequeue(
        VOID
        )
    {
        return (m_PowerStopState == FxRequestPowerStopAcknowledgedWithRequeue) ? TRUE : FALSE;
    }

    _Must_inspect_result_
    NTSTATUS
    InsertHeadIrpQueue(
        __in FxIrpQueue* IrpQueue,
        __out_opt ULONG* pRequestCount
        );

    //
    // Internal function to retrieve the FxRequest
    // structure from a pointer to its CsqContext
    // member.
    //
    __inline
    static
    FxRequest*
    RetrieveFromCsqContext(
        __in PMdIoCsqIrpContext pCsqContext
        )
    {
        return CONTAINING_RECORD(pCsqContext, FxRequest, m_CsqContext);
    }

    _Must_inspect_result_
    static
    NTSTATUS
    GetNextRequest(
        __in FxIrpQueue*        IrpQueue,
        __in_opt MdFileObject   FileObject,
        __in_opt FxRequest*     TagRequest,
        __deref_out FxRequest** ppOutRequest
        );

    // Do not specify argument names
    FX_DECLARE_VF_FUNCTION(
    NTSTATUS, 
    VerifyRequestIsDriverOwned
    );
    
    // Do not specify argument names
    FX_DECLARE_VF_FUNCTION(
    NTSTATUS, 
    VerifyRequestIsCancelable
    );
    
    // Do not specify argument names
    FX_DECLARE_VF_FUNCTION(
    NTSTATUS, 
    VerifyRequestIsNotCancelable
    );

    // Do not specify argument names
    FX_DECLARE_VF_FUNCTION(
    NTSTATUS, 
    VerifyRequestIsTagRequest
    );

    __inline
    BOOLEAN
    IsRequestForwardedToParent(
        VOID
        )
    {
        return m_ForwardRequestToParent;
    }

    _Must_inspect_result_
    NTSTATUS
    GetMemoryObject(
        __deref_out IFxMemory** Memory,
        __out PVOID* Buffer,
        __out size_t* Length
        );

private:

    NTSTATUS
    CompleteInternal(
        __in NTSTATUS Status
        );

    VOID
    AssignMemoryBuffers(
        __in WDF_DEVICE_IO_TYPE IoType
    )
    {    
        switch (m_Irp.GetMajorFunction()) {
        case IRP_MJ_DEVICE_CONTROL:
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:

          switch (m_Irp.GetParameterIoctlCodeBufferMethod()) {
          case METHOD_BUFFERED:
              //
              // Set the buffer in the memory interface. For kernel mode, 
              // GetOutputBuffer is same as GetSystemBuffer, but for user-mode, 
              // host provides separate buffers, so that input buffer can only be
              // read, and output buffer can only be written to. 
              //
              m_SystemBuffer.SetBuffer(m_Irp.GetSystemBuffer());
              m_OutputBuffer.SetBuffer(m_Irp.GetOutputBuffer());
              break;

          case METHOD_IN_DIRECT:
              //
              // InputBuffer is in SystemBuffer
              // OutputBuffer is in MdlAddress with read access
              //
              KMDF_ONLY_CODE_PATH_ASSERT();
              m_SystemBuffer.SetBuffer(m_Irp.GetSystemBuffer());
              break;

          case METHOD_OUT_DIRECT:
              //
              // InputBuffer is in SystemBuffer
              // OutputBuffer is in MdlAddress with write access
              //
              KMDF_ONLY_CODE_PATH_ASSERT();
              m_SystemBuffer.SetBuffer(m_Irp.GetSystemBuffer());
              break;

          case METHOD_NEITHER:
              //
              // Internal device controls are kernel mode to kernel mode, and deal
              // with direct unmapped pointers.
              //
              // In addition, a normal device control with
              // RequestorMode == KernelMode is also treated as kernel mode
              // to kernel mode since the I/O Manager will not generate requests
              // with this setting from a user mode request.
              //
              KMDF_ONLY_CODE_PATH_ASSERT();
              if (m_Irp.GetMajorFunction() == IRP_MJ_INTERNAL_DEVICE_CONTROL ||
                  (m_Irp.GetRequestorMode() == KernelMode))
                {
                  m_SystemBuffer.SetBuffer(
                      m_Irp.GetParameterIoctlType3InputBuffer()
                      );
                  m_OutputBuffer.SetBuffer(m_Irp.GetUserBuffer());
              }
              else {
                  return;
              }
              break;
          }
          break;

          case IRP_MJ_READ:
          case IRP_MJ_WRITE:
              switch (IoType) {
              case WdfDeviceIoBuffered:
                  m_SystemBuffer.SetBuffer(m_Irp.GetSystemBuffer());
                  break;

              case WdfDeviceIoNeither:
                  KMDF_ONLY_CODE_PATH_ASSERT();
                  if (m_Irp.GetRequestorMode() == KernelMode) {
                       m_SystemBuffer.SetBuffer(m_Irp.GetUserBuffer());
                  }
                  else {
                      return;
                  }
              break;

              default:
                  return;
              }
              break;

          default:
              return;
          }

          if (m_SystemBuffer.IsBufferSet())
          {
              m_RequestBaseStaticFlags |= FxRequestBaseStaticSystemBufferValid;
          }
          if (m_OutputBuffer.IsBufferSet())
          {
              m_RequestBaseStaticFlags |= FxRequestBaseStaticOutputBufferValid;
          }
    }

    FX_DECLARE_VF_FUNCTION_P1(
        VOID, 
        VerifyCompleteInternal,
            _In_ NTSTATUS
            );

    VOID
    PreProcessCompletionForDriverRequest(
        __in FxRequestCompletionState State,
        __in FxIoQueue* Queue
        );

    VOID
    PostProcessCompletion(
        __in FxRequestCompletionState State,
        __in FxIoQueue* Queue
        );

    VOID
    PostProcessCompletionForReserved(
        __in FxRequestCompletionState State,
        __in FxIoQueue* Queue
        );

    VOID
    PostProcessCompletionForDriverRequest(
        __in FxRequestCompletionState State,
        __in FxIoQueue* Queue
        );

};

class FxRequestFromLookaside : public FxRequest {

public:
    FxRequestFromLookaside(
        __in CfxDevice* Device,
        __in MdIrp Irp
        );

    PVOID
    operator new(
        __in size_t Size,
        __in CfxDevice* Device,
        __in_opt PWDF_OBJECT_ATTRIBUTES Attributes
        );

protected:
    //
    // FxObject override
    //
    VOID
    SelfDestruct(
        VOID
        );
};

#endif //_FXREQUEST_H_
