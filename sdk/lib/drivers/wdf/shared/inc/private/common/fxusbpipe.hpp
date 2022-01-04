//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef _FXUSBPIPE_H_
#define _FXUSBPIPE_H_

#include "fxusbrequestcontext.hpp"
#include "fxusbinterface.hpp"

//
// Technically, EHCI can support 4MB, but the usb driver stack doesn't
// allocate enough TDs for such a transfer, here I arbitrarily chose 2MB
//
enum FxUsbPipeMaxTransferSize {
    FxUsbPipeHighSpeedMaxTransferSize = 2*1024*1024 ,
    FxUsbPipeLowSpeedMaxTransferSize = 256 * 1024,
    FxUsbPipeControlMaxTransferSize =  4*1024
};

struct FxUsbPipeTransferContext : public FxUsbRequestContext {
    FxUsbPipeTransferContext(
        __in FX_URB_TYPE UrbType
        );

    ~FxUsbPipeTransferContext(
        VOID
        );

    __checkReturn
    NTSTATUS
    AllocateUrb(
        __in USBD_HANDLE USBDHandle
        );

    virtual
    VOID
    Dispose(
        VOID
        );

    virtual
    VOID
    CopyParameters(
        __in FxRequestBase* Request
        );

    virtual
    VOID
    StoreAndReferenceMemory(
        __in FxRequestBuffer* Buffer
        );

    virtual
    VOID
    ReleaseAndRestore(
        __in FxRequestBase* Request
        );

    VOID
    SetUrbInfo(
        __in USBD_PIPE_HANDLE PipeHandle,
        __in ULONG TransferFlags
        );

    USBD_STATUS
    GetUsbdStatus(
        VOID
        );

    ULONG
    GetUrbTransferLength(
        VOID
        )
    {
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
        return m_Urb->TransferBufferLength;
#elif (FX_CORE_MODE == FX_CORE_USER_MODE)
        return m_UmUrb.UmUrbBulkOrInterruptTransfer.TransferBufferLength;
#endif
    }

private:
    USBD_HANDLE m_USBDHandle;

public:
    _URB_BULK_OR_INTERRUPT_TRANSFER m_UrbLegacy;

    //
    // m_Urb will either point to m_UrbLegacy or one allocated by USBD_UrbAllocate
    //
    _URB_BULK_OR_INTERRUPT_TRANSFER* m_Urb;

    PMDL m_PartialMdl;

    BOOLEAN m_UnlockPages;
};

struct FxUsbUrbContext : public FxUsbRequestContext {
    FxUsbUrbContext(
        VOID
        );

    USBD_STATUS
    GetUsbdStatus(
        VOID
        );

    virtual
    VOID
    StoreAndReferenceMemory(
        __in FxRequestBuffer* Buffer
        );

    virtual
    VOID
    ReleaseAndRestore(
        __in FxRequestBase* Request
        );

    PURB m_pUrb;
};

struct FxUsbPipeRequestContext : public FxUsbRequestContext {
    FxUsbPipeRequestContext(
        __in FX_URB_TYPE FxUrbType
        );

    ~FxUsbPipeRequestContext(
        VOID
        );

    __checkReturn
    NTSTATUS
    AllocateUrb(
        __in USBD_HANDLE USBDHandle
        );

    virtual
    VOID
    Dispose(
        VOID
        );

    VOID
    SetInfo(
        __in WDF_USB_REQUEST_TYPE Type,
        __in USBD_PIPE_HANDLE PipeHandle,
        __in USHORT Function
        );

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    VOID
    SetInfo(
        __in WDF_USB_REQUEST_TYPE Type,
        __in WINUSB_INTERFACE_HANDLE WinUsbHandle,
        __in UCHAR PipeId,
        __in USHORT Function
        );
#endif

    USBD_STATUS
    GetUsbdStatus(
        VOID
        );

private:
    USBD_HANDLE m_USBDHandle;

public:
    _URB_PIPE_REQUEST m_UrbLegacy;

    //
    // m_Urb will either point to m_UrbLegacy or one allocated by USBD_UrbAllocate
    //
    _URB_PIPE_REQUEST* m_Urb;

};

struct FxUsbPipeRepeatReader {
    //
    // Request used to send IRPs
    //
    FxRequest* Request;

    //
    // IRP out of Request.  Store it off so we don't have to call
    // Request->GetSubmitIrp() everytime.
    //
    MdIrp RequestIrp;

    //
    // The containing parent
    //
    FxUsbPipeContinuousReader* Parent;

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    //
    // DPC to queue ourselves to when a read completes so that we don't spin in
    // the same thread repeatedly.
    //
    KDPC Dpc;
#elif (FX_CORE_MODE == FX_CORE_USER_MODE)
    //
    // Workitem to queue ourselves to when a read completes so that we don't recurse in
    // the same thread repeatedly.
    //
    MxWorkItem m_ReadWorkItem;

    //
    // Check if the CR got called on a recursive call
    //
    LONG  ThreadOwnerId;
#endif

    //
    // Event that is set when the reader has completed and is not
    //
    MxEvent ReadCompletedEvent;
};

#define NUM_PENDING_READS_DEFAULT   (2)
#define NUM_PENDING_READS_MAX       (10)

//
// Work-item callback flags
//
#define FX_USB_WORKITEM_IN_PROGRESS     (0x00000001)
#define FX_USB_WORKITEM_RERUN           (0x00000002)

//
// In theory this can be a base class independent of bus type, but this is
// easier for now and there is no need on another bus type yet.
//
struct FxUsbPipeContinuousReader : public FxStump {
public:
    FxUsbPipeContinuousReader(
        __in FxUsbPipe* Pipe,
        __in UCHAR NumReaders
        );

    ~FxUsbPipeContinuousReader();

    _Must_inspect_result_
    NTSTATUS
    Config(
        __in PWDF_USB_CONTINUOUS_READER_CONFIG Config,
        __in size_t TotalBufferLength
        );

    PVOID
    operator new(
        __in size_t Size,
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __range(1, NUM_PENDING_READS_MAX) ULONG NumReaders
        );

    _Must_inspect_result_
    NTSTATUS
    FormatRepeater(
        __in FxUsbPipeRepeatReader* Repeater
        );

    VOID
    CancelRepeaters(
        VOID
        );

    ULONG
    ResubmitRepeater(
        __in FxUsbPipeRepeatReader* Repeater,
        __out NTSTATUS* Status
        );

protected:
    VOID
    DeleteMemory(
        __in FxRequestBase* Request
        )
    {
        FxRequestContext* pContext;

        pContext = Request->GetContext();
        if (pContext != NULL && pContext->m_RequestMemory != NULL) {
            pContext->m_RequestMemory->Delete();
            //
            // NOTE: Don't NULL out the m_RequestMemory member as this will
            // prevent Reuse from releasing a reference on the m_RequestMemory object
            // and hence these memory objects will not be freed.
            //
        }
    }

    BOOLEAN
    QueueWorkItemLocked(
        __in FxUsbPipeRepeatReader* Repeater
        );

    __inline
    VOID
    FxUsbPipeRequestWorkItemHandler(
        __in FxUsbPipeRepeatReader* FailedRepeater
        );

    static
    MdDeferredRoutineType
    _FxUsbPipeContinuousReadDpc;

    static
    MX_WORKITEM_ROUTINE
    _ReadWorkItem;


    static
    EVT_WDF_REQUEST_COMPLETION_ROUTINE
    _FxUsbPipeRequestComplete;

    static
    EVT_SYSTEMWORKITEM
    _FxUsbPipeRequestWorkItemThunk;

public:
    //
    // Completion routine for the client
    //
    PFN_WDF_USB_READER_COMPLETION_ROUTINE m_ReadCompleteCallback;

    //
    // Context for completion routine
    //
    WDFCONTEXT m_ReadCompleteContext;

    //
    // Callback to invoke when a reader fails
    //
    PFN_WDF_USB_READERS_FAILED m_ReadersFailedCallback;

    //
    // The owning pipe
    //
    FxUsbPipe* m_Pipe;

    //
    // Lookaside list from which we will allocate buffers for each new read
    //
    FxLookasideList* m_Lookaside;

    //
    // The devobj we are sending requests to
    //
    MdDeviceObject m_TargetDevice;

    //
    // Offsets and length into the memory buffers created by the lookaside list
    //
    WDFMEMORY_OFFSET m_Offsets;

    //
    // Work item to queue when we hit various errors
    //
    FxSystemWorkItem* m_WorkItem;

    //
    // Work item re-run context.
    //
    PVOID m_WorkItemRerunContext;

    //
    // This is a pointer to the work-item's thread object. This value is
    // used for not deadlocking when misbehaved drivers (< v1.9) call
    // WdfIoTargetStop from EvtUsbTargetPipeReadersFailed callback.
    //
    volatile POINTER_ALIGNMENT MxThread m_WorkItemThread;

    //
    // Work item flags (see FX_USB_WORKITEM_Xxx defines).
    //
    ULONG m_WorkItemFlags;

    //
    // Number of readers who have failed due to internal allocation errors
    //
    UCHAR m_NumFailedReaders;

    //
    // Number of readers
    //
    UCHAR m_NumReaders;

    //
    // Value to use with InterlockedXxx to test to see if a work item has been
    // queued or not
    //
    BOOLEAN m_WorkItemQueued;

    //
    // Track whether the readers should be submitted when moving into the start
    // state.  We cannot just track a start -> start transition and not send
    // the readers on that particular state transition because the first time
    // we need to send the readers, the target is already in the started state
    //
    BOOLEAN m_ReadersSubmitted;

    //
    // Open ended array of readers.  MUST be the last element in this structure.
    //
    FxUsbPipeRepeatReader m_Readers[1];
};

class FxUsbPipe : public FxIoTarget {
public:
    friend FxUsbDevice;
    friend FxUsbInterface;
    friend FxUsbPipeContinuousReader;

    FxUsbPipe(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in FxUsbDevice* UsbDevice
        );

    VOID
    InitPipe(
        __in PUSBD_PIPE_INFORMATION PipeInfo,
        __in UCHAR InterfaceNumber,
        __in FxUsbInterface* UsbInterface
        );

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    VOID
    InitPipe(
        __in PWINUSB_PIPE_INFORMATION PipeInfo,
        __in UCHAR InterfaceNumber,
        __in FxUsbInterface* UsbInterface
        );
#endif

    _Must_inspect_result_
    virtual
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

    VOID
    GotoPurgeState(
        __in WDF_IO_TARGET_PURGE_IO_ACTION  Action,
        __in PLIST_ENTRY                    PendedRequestListHead,
        __in PSINGLE_LIST_ENTRY             SentRequestListHead,
        __out PBOOLEAN                      Wait,
        __in BOOLEAN                        LockSelf
        );

    virtual
    VOID
    GotoRemoveState(
        __in WDF_IO_TARGET_STATE    NewState,
        __in PLIST_ENTRY            PendedRequestListHead,
        __in PSINGLE_LIST_ENTRY     SentRequestListHead,
        __in BOOLEAN                Lock,
        __out PBOOLEAN              Wait
        );

    virtual
    VOID
    WaitForSentIoToComplete(
        VOID
        );

    __inline
    VOID
    SetNoCheckPacketSize(
        VOID
        )
    {
        m_CheckPacketSize = FALSE;
    }

    VOID
    GetInformation(
        __out PWDF_USB_PIPE_INFORMATION PipeInformation
        );

    BOOLEAN
    IsType(
        __in WDF_USB_PIPE_TYPE Type
        );


    WDF_USB_PIPE_TYPE
    GetType(
        VOID
        );

    WDFUSBPIPE
    GetHandle(
        VOID
        )
    {
        return (WDFUSBPIPE) GetObjectHandle();
    }

    __inline
    BOOLEAN
    IsInEndpoint(
        VOID
        )
    {
        //
        // USB_ENDPOINT_DIRECTION_IN just does a bitwise compre so it could
        // return 0 or some non zero value.  Make sure the non zero value is
        // TRUE
        //
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
        return USB_ENDPOINT_DIRECTION_IN(m_PipeInformation.EndpointAddress) ? TRUE : FALSE;
#elif (FX_CORE_MODE == FX_CORE_USER_MODE)
        return USB_ENDPOINT_DIRECTION_IN(m_PipeInformationUm.PipeId) ? TRUE : FALSE;
#endif
    }

    __inline
    BOOLEAN
    IsOutEndpoint(
        VOID
        )
    {
        //
        // USB_ENDPOINT_DIRECTION_OUT just does a bitwise compre so it could
        // return 0 or some non zero value.  Make sure the non zero value is
        // TRUE
        //
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
        return USB_ENDPOINT_DIRECTION_OUT(m_PipeInformation.EndpointAddress) ? TRUE : FALSE;
#elif (FX_CORE_MODE == FX_CORE_USER_MODE)
        return USB_ENDPOINT_DIRECTION_OUT(m_PipeInformationUm.PipeId) ? TRUE : FALSE;
#endif
    }

    _Must_inspect_result_
    NTSTATUS
    InitContinuousReader(
        __in PWDF_USB_CONTINUOUS_READER_CONFIG Config,
        __in size_t TotalBufferLength
        );

    ULONG
    GetMaxPacketSize(
        VOID
        )
    {
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
        return  m_PipeInformation.MaximumPacketSize;
#elif (FX_CORE_MODE == FX_CORE_USER_MODE)
        return  m_PipeInformationUm.MaximumPacketSize;
#endif
    }

    _Must_inspect_result_
    NTSTATUS
    ValidateTransferLength(
        __in size_t Length
        )
    {
        //
        // Assumes this is not a control pipe
        //
        if (m_CheckPacketSize &&
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
            (Length % m_PipeInformation.MaximumPacketSize) != 0) {
#elif (FX_CORE_MODE == FX_CORE_USER_MODE)
            (Length % m_PipeInformationUm.MaximumPacketSize) != 0) {
#endif
            return STATUS_INVALID_BUFFER_SIZE;
        }
        else {
            return STATUS_SUCCESS;
        }
    }

    _Must_inspect_result_
    NTSTATUS
    FormatTransferRequest(
        __in FxRequestBase* Request,
        __in FxRequestBuffer* Buffer,
        __in ULONG TransferFlags = 0
        );

    _Must_inspect_result_
    NTSTATUS
    FormatAbortRequest(
        __in FxRequestBase* Request
        );

    _Must_inspect_result_
    NTSTATUS
    FormatResetRequest(
        __in FxRequestBase* Request
        );

    static
    _Must_inspect_result_
    NTSTATUS
    _FormatTransfer(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in WDFUSBPIPE Pipe,
        __in WDFREQUEST Request,
        __in_opt WDFMEMORY TransferMemory,
        __in_opt PWDFMEMORY_OFFSET TransferOffsets,
        __in ULONG Flags
        );

    static
    _Must_inspect_result_
    NTSTATUS
    _SendTransfer(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in WDFUSBPIPE Pipe,
        __in_opt WDFREQUEST Request,
        __in_opt PWDF_REQUEST_SEND_OPTIONS RequestOptions,
        __in_opt PWDF_MEMORY_DESCRIPTOR MemoryDescriptor,
        __out_opt PULONG BytesTransferred,
        __in ULONG Flags
        );

    USBD_PIPE_HANDLE
    WdmGetPipeHandle(
        VOID
        )
    {
        return m_PipeInformation.PipeHandle;
    }

    static
    WDF_USB_PIPE_TYPE
    _UsbdPipeTypeToWdf(
         __in USBD_PIPE_TYPE UsbdPipeType
         )
    {
        const static WDF_USB_PIPE_TYPE types[] = {
            WdfUsbPipeTypeControl,              // UsbdPipeTypeControl
            WdfUsbPipeTypeIsochronous,          // UsbdPipeTypeIsochronous
            WdfUsbPipeTypeBulk,                 // UsbdPipeTypeBulk
            WdfUsbPipeTypeInterrupt,            // UsbdPipeTypeInterrupt
        };

        if (UsbdPipeType < sizeof(types)/sizeof(types[0])) {
            return types[UsbdPipeType];
        }
        else {
            return WdfUsbPipeTypeInvalid;
        }
    }

    NTSTATUS
    Reset(
        VOID
        );

    USBD_HANDLE
    GetUSBDHandle(
        VOID
        )
    {
        return m_USBDHandle;
    }

    FX_URB_TYPE
    GetUrbType(
        VOID
        )
    {
        return m_UrbType;
    }

public:
    //
    // Link for FxUsbDevice to use to hold a list of Pipes
    //
    LIST_ENTRY m_ListEntry;

protected:
    ~FxUsbPipe();

    virtual
    BOOLEAN
    Dispose(
        VOID
        );

    FxUsbDevice* m_UsbDevice;

    FxUsbInterface* m_UsbInterface;

    //
    // If the pipe does not have a continuous reader, this field is NULL.
    // It is also cleared within the pipe's Dispose function after deleting
    // the continuous reader to prevent misbehaved drivers from
    // crashing the system when they call WdfIoTargetStop from their usb pipe's
    // destroy callback.
    //
    FxUsbPipeContinuousReader* m_Reader;

    //
    // Information about this pipe
    //
    USBD_PIPE_INFORMATION  m_PipeInformation;

#if (FX_CORE_MODE == FX_CORE_USER_MODE)



    WINUSB_PIPE_INFORMATION m_PipeInformationUm;
#endif

    //
    // Interface associated with this pipe
    //
    UCHAR m_InterfaceNumber;

    //
    // Indicates if we should check that the buffer being trasnfered is of a
    // multiple of max packet size.
    //
    BOOLEAN m_CheckPacketSize;

    //
    // The USBD_HANDLE exchanged by FxUsbDevice
    //
    USBD_HANDLE m_USBDHandle;

    //
    // If the client driver submits an URB to do a USB transfer, this field indicates
    // the type of that Urb
    //
    FX_URB_TYPE m_UrbType;

};

#endif // _FXUSBPIPE_H_
