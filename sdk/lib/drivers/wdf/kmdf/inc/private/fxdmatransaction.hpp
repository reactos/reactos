//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef _FXDMATRANSACTION_HPP_
#define _FXDMATRANSACTION_HPP_

extern "C" {
// #include "FxDmaTransaction.hpp.tmh"
}

#include "fxdmatransactioncallbacks.hpp"

//
// This type is used to allocate scatter-gather list of 1 element on the stack.
//
typedef DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) UCHAR UCHAR_MEMORY_ALIGNED;

// begin_wpp enum

//
// FxDmaTransactionStateCreated when the object is created.
// FxDmaTransactionStateInitialized when object is initialized using with
//    Mdl/VA/Length.
// FxDmaTransactionStateReserved when driver calls AllocateResources until
//    the adapter control routine returns
// FxDmaTransactionStateTransfer is called when the driver call Execute
//    to start the DMA transfer.
// FxDmaTransactionStateTransferCompleted is when transfer is completed or
//    aborted
// FxDmaTransactionStateTransferFailed is set if the framework is not able
//    to start the transfer due to error.
// FxDmaTransactionStateReleased is set when the object is reinitailized for reuse
// FxDmaTransactionStateDeleted is set in the Dipose due to WdfObjectDelete
//
enum FxDmaTransactionState {
    FxDmaTransactionStateInvalid = 0,
    FxDmaTransactionStateCreated,
    FxDmaTransactionStateReserved,
    FxDmaTransactionStateInitialized,
    FxDmaTransactionStateTransfer,
    FxDmaTransactionStateTransferCompleted,
    FxDmaTransactionStateTransferFailed,
    FxDmaTransactionStateReleased,
    FxDmaTransactionStateDeleted,
};

//
// FxDmaCompletionTypeFull is used when the driver calls WdfDmaTransactionDmaComplete
//      to indicate that last framgement has been transmitted fully and to initiate
//      the transfer of next fragment.
// FxDmaCompletionTypePartial is used when the driver completes the transfer and
//      specifies a amount of bytes it has transfered, and to initiate the next transfer
//      from the untransmitted portion of the buffer.
// FxDmaCompletionTypeAbort i used when the driver calls DmaCompleteFinal to indicate
//      that's the final transfer and not initiate anymore transfers for the remaining
//      data.
//
enum FxDmaCompletionType {
    FxDmaCompletionTypeFull = 1,
    FxDmaCompletionTypePartial,
    FxDmaCompletionTypeAbort,
};

// end_wpp

//
// This tag is used to track whether the request pointer in the transaction
// has a reference taken on it.  Since the pointer is guaranteed to be
// 4-byte aligned this can be set and cleared without destroying the pointer.
//
#define FX_STRONG_REF_TAG               0x1

//
// Simple set of macros to encode and decode tagged pointers.
//
#define FX_ENCODE_POINTER(T,p,tag)      ((T*) ((ULONG_PTR) p | (ULONG_PTR) tag))
#define FX_DECODE_POINTER(T,p,tag)      ((T*) ((ULONG_PTR) p & ~(ULONG_PTR) tag))
#define FX_IS_POINTER_ENCODED(p,tag)    ((((ULONG_PTR) p & (ULONG_PTR) tag) == tag) ? TRUE : FALSE)

//
// An uninitialized value for Dma completion status
//

#define UNDEFINED_DMA_COMPLETION_STATUS ((DMA_COMPLETION_STATUS) -1)

class FxDmaTransactionBase : public FxNonPagedObject {

    friend class FxDmaEnabler;

public:

    FxDmaTransactionBase(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in USHORT ObjectSize,
        __in USHORT ExtraSize,
        __in FxDmaEnabler *DmaEnabler
        );

    static
    VOID
    _ComputeNextTransferAddress(
        __in PMDL CurrentMdl,
        __in size_t CurrentOffset,
        __in ULONG Transferred,
        __deref_out PMDL *NextMdl,
        __out size_t *NextOffset
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _CalculateRequiredMapRegisters(
        __in PMDL Mdl,
        __in size_t CurrentOffset,
        __in ULONG Length,
        __in ULONG AvailableMapRegisters,
        __out_opt PULONG PossibleTransferLength,
        __out PULONG MapRegistersRequired
        );

    virtual
    BOOLEAN
    Dispose(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    Initialize(
        __in PFN_WDF_PROGRAM_DMA     ProgramDmaFunction,
        __in WDF_DMA_DIRECTION       DmaDirection,
        __in PMDL                    Mdl,
        __in size_t                  Offset,
        __in ULONG                   Length
        );

    _Must_inspect_result_
    virtual
    NTSTATUS
    InitializeResources(
        VOID
        )=0;

    _Must_inspect_result_
    NTSTATUS
    Execute(
        __in PVOID   Context
        );

    _Must_inspect_result_
    virtual
    NTSTATUS
    StartTransfer(
        VOID
        )=0;

    _Must_inspect_result_
    virtual
    NTSTATUS
    StageTransfer(
        VOID
        )=0;

    BOOLEAN
    DmaCompleted(
        __in  size_t      TransferredLength,
        __out NTSTATUS  * ReturnStatus,
        __in  FxDmaCompletionType CompletionType
        );

    _Must_inspect_result_
    virtual
    NTSTATUS
    TransferCompleted(
        VOID
        )=0;

    VOID
    ReleaseForReuse(
        __in BOOLEAN ForceRelease
        );

    virtual
    VOID
    ReleaseResources(
        __in BOOLEAN ForceRelease
        )=0;

    VOID
    GetTransferInfo(
        __out_opt ULONG *MapRegisterCount,
        __out_opt ULONG *ScatterGatherElementCount
        );

    __forceinline
    size_t
    GetBytesTransferred(
        VOID
        )
    {
        return m_Transferred;
    }

    __forceinline
    FxDmaEnabler *
    GetDmaEnabler(
        VOID
        )
    {
        return m_DmaEnabler;
    }

    __forceinline
    FxRequest *
    GetRequest(
        VOID
        )
    {
        //
        // Strip out the strong reference tag if it's set
        //
        return FX_DECODE_POINTER(FxRequest,
                                 m_EncodedRequest,
                                 FX_STRONG_REF_TAG);
    }

    __forceinline
    BOOLEAN
    IsRequestReferenced(
        VOID
        )
    {
        return FX_IS_POINTER_ENCODED(m_EncodedRequest, FX_STRONG_REF_TAG);
    }

    __forceinline
    VOID
    SetRequest(
        __in FxRequest* Request
        )
    {
        ASSERT(m_EncodedRequest == NULL);

        //
        // Make sure the pointer doesn't have the strong ref flag set already
        //
        ASSERT(FX_IS_POINTER_ENCODED(Request, FX_STRONG_REF_TAG) == FALSE);

        m_EncodedRequest = Request;
    }

    __forceinline
    VOID
    ReferenceRequest(
        VOID
        )
    {
        ASSERT(m_EncodedRequest != NULL);
        ASSERT(IsRequestReferenced() == false);

        //
        // Take a reference on the irp to catch completion of request
        // when there is a pending DMA transaction.
        // While there is no need to take a reference on request itself,
        // I'm keeping it to avoid regression as we are so close to
        // shipping this.
        //
        m_EncodedRequest->AddIrpReference();

        //
        // Increment reference to this Request.
        // See complementary section in WdfDmaTransactionDelete
        // and WdfDmaTransactionRelease.
        //
        m_EncodedRequest->ADDREF( this );

        m_EncodedRequest = FX_ENCODE_POINTER(FxRequest,
                                             m_EncodedRequest,
                                             FX_STRONG_REF_TAG);
    }

    __forceinline
    VOID
    ReleaseButRetainRequest(
        VOID
        )
    {
        ASSERT(m_EncodedRequest != NULL);
        ASSERT(IsRequestReferenced());

        //
        // Clear the referenced bit on the encoded request.
        //
        m_EncodedRequest = FX_DECODE_POINTER(FxRequest,
                                             m_EncodedRequest,
                                             FX_STRONG_REF_TAG);

        //
        // Release this reference to the Irp and FxRequest.
        //
        m_EncodedRequest->ReleaseIrpReference();

        m_EncodedRequest->RELEASE( this );
    }

    __forceinline
    VOID
    ClearRequest(
        VOID
        )
    {
        if (IsRequestReferenced()) {
            ReleaseButRetainRequest();
        }
        m_EncodedRequest = NULL;
    }

    __forceinline
    size_t
    GetMaximumFragmentLength(
        VOID
        )
    {
        return m_MaxFragmentLength;
    }

    __forceinline
    VOID
    SetMaximumFragmentLength(
        size_t  MaximumFragmentLength
        )
    {
        if (MaximumFragmentLength < m_AdapterInfo->MaximumFragmentLength) {
            m_MaxFragmentLength = MaximumFragmentLength;
        }
    }

    __forceinline
    size_t
    GetCurrentFragmentLength(
        VOID
        )
    {
        return m_CurrentFragmentLength;
    }

    __forceinline
    WDFDMATRANSACTION
    GetHandle(
        VOID
        )
    {
        return (WDFDMATRANSACTION) GetObjectHandle();
    }

    PVOID
    GetTransferContext(
        VOID
        )
    {
        return m_TransferContext;
    }

    VOID
    SetImmediateExecution(
        __in BOOLEAN Value
        );

    BOOLEAN
    CancelResourceAllocation(
        VOID
        );

    FxDmaTransactionState
    GetTransactionState(
        VOID
        )
    {
        return m_State;
    }

protected:

    FxDmaTransactionState         m_State;

    WDF_DMA_DIRECTION             m_DmaDirection;

    FxDmaEnabler*                 m_DmaEnabler;

    //
    // Depending on the direction of the transfer, this one
    // points to either m_ReadAdapterInfo or m_WriteAdapterInfo
    // structure of the DMA enabler.
    //
    FxDmaDescription*             m_AdapterInfo;

    //
    // Request associated with this transfer.  Encoding uses the
    // FX_[EN|DE]CODE_POINTER macros with the FX_STRONG_REF_TAG
    // to indicate whether the reference has been taken or not
    //
    FxRequest*                    m_EncodedRequest;

    //
    // Callback and context for ProgramDma function
    //
    // The callback is overloaded to also hold the callback for
    // Packet & System transfer's Reserve callback (to save space.)
    // This is possible because the driver may not call execute
    // and reserve in parallel on the same transaction.  Disambiguate
    // using the state of the transaction.
    //
    FxDmaTransactionProgramOrReserveDma    m_DmaAcquiredFunction;
    PVOID                                  m_DmaAcquiredContext;

    //
    // The DMA transfer context (when using V3 DMA)
    //
    PVOID                         m_TransferContext;

    //
    // This is the first MDL of the transaction.
    //
    PMDL                          m_StartMdl;

    //
    // This is the MDL where the current transfer is being executed.
    // If the data spans multiple MDL then this would be different
    // from the startMDL when we stage large transfers and also
    // if the driver performs partial transfers.
    //
    PMDL                          m_CurrentFragmentMdl;

    //
    // Starting offset in the first m_StartMdl. This might be same as
    // m_StartMdl->StartVA.
    //
    size_t                        m_StartOffset;

    //
    // Points to address where the next transfer will begin.
    //
    size_t                        m_CurrentFragmentOffset;

    //
    // This is maximum length of transfer that can be made. This is
    // computed based on the available map registers and driver
    // configuration.
    //
    size_t                        m_MaxFragmentLength;

    //
    // Length of the whole transaction.
    //
    size_t                        m_TransactionLength;

    //
    // Number of bytes pending to be transfered.
    //
    size_t                        m_Remaining;

    //
    // Total number of bytes transfered.
    //
    size_t                        m_Transferred;

    //
    // Number of bytes transfered in the last transaction.
    //
    size_t                        m_CurrentFragmentLength;

    //
    // DMA flags for passing to GetScatterGatherListEx &
    // AllocateAdapterChannelEx
    //

    ULONG m_Flags;

    static
    PVOID
    GetStartVaFromOffset(
        __in PMDL   Mdl,
        __in size_t Offset
        )
    {
        return ((PUCHAR) MmGetMdlVirtualAddress(Mdl)) + Offset;
    }

    virtual
    VOID
    Reuse(
        VOID
        )
    {
        return;
    }

};

class FxDmaScatterGatherTransaction : public FxDmaTransactionBase {

public:

    FxDmaScatterGatherTransaction(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in USHORT ExtraSize,
        __in FxDmaEnabler *DmaEnabler
        );

    virtual
    BOOLEAN
    Dispose(
        VOID
        );

    _Must_inspect_result_
    virtual
    NTSTATUS
    InitializeResources(
        VOID
        );

    _Must_inspect_result_
    virtual
    NTSTATUS
    StartTransfer(
        VOID
        );

    _Must_inspect_result_
    virtual
    NTSTATUS
    TransferCompleted(
        VOID
        );

    _Must_inspect_result_
    virtual
    NTSTATUS
    StageTransfer(
        VOID
        );

    virtual
    VOID
    ReleaseResources(
        __in BOOLEAN ForceRelease
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _Create(
        __in  PFX_DRIVER_GLOBALS      FxDriverGlobals,
        __in  PWDF_OBJECT_ATTRIBUTES  Attributes,
        __in  FxDmaEnabler*           DmaEnabler,
        __out WDFDMATRANSACTION*      Transaction
        );

protected:

    //
    // Scatter-Gather list provided by the system.
    //
    PSCATTER_GATHER_LIST         m_SGList;

    //
    // Preallocated memory from lookaside buffer for the
    // scatter-gather list when running on XP and later OSes.
    //
    PVOID                        m_LookasideBuffer;


private:

    static
    VOID
    _AdapterListControl(
        __in  DEVICE_OBJECT         * DeviceObject,
        __in  IRP                   * Irp,
        __in  SCATTER_GATHER_LIST   * SgList,
        __in  VOID                  * Context
        );

    _Must_inspect_result_
    NTSTATUS
    GetScatterGatherList (
        __in PMDL  Mdl,
        __in size_t CurrentOffset,
        __in ULONG  Length,
        __in PDRIVER_LIST_CONTROL  ExecutionRoutine,
        __in PVOID  Context
        )
    {
        NTSTATUS status;
        KIRQL irql;

        KeRaiseIrql(DISPATCH_LEVEL, &irql);

        if (m_DmaEnabler->UsesDmaV3())
        {
            PDMA_OPERATIONS dmaOperations =
                m_AdapterInfo->AdapterObject->DmaOperations;

            status = dmaOperations->GetScatterGatherListEx(
                                               m_AdapterInfo->AdapterObject,
                                               m_DmaEnabler->m_FDO,
                                               GetTransferContext(),
                                               Mdl,
                                               CurrentOffset,
                                               Length,
                                               m_Flags,
                                               ExecutionRoutine,
                                               Context,
                                               (BOOLEAN) m_DmaDirection,
                                               NULL,
                                               NULL,
                                               NULL
                                               );
        }
        else
        {
            status = m_AdapterInfo->AdapterObject->DmaOperations->
                        GetScatterGatherList(m_AdapterInfo->AdapterObject,
                                             m_DmaEnabler->m_FDO,
                                             Mdl,
                                             GetStartVaFromOffset(Mdl, CurrentOffset),
                                             Length,
                                             ExecutionRoutine,
                                             Context,
                                             (BOOLEAN) m_DmaDirection);
        }

        KeLowerIrql(irql);

        return status;
    }

    VOID
    PutScatterGatherList(
        __in PSCATTER_GATHER_LIST  ScatterGather
        )
    {
        KIRQL irql;

        KeRaiseIrql(DISPATCH_LEVEL, &irql);

        m_AdapterInfo->AdapterObject->DmaOperations->
                PutScatterGatherList(m_AdapterInfo->AdapterObject,
                                     ScatterGather,
                                     (BOOLEAN) m_DmaDirection);
        KeLowerIrql(irql);

        return;
    }

    _Must_inspect_result_
    NTSTATUS
    BuildScatterGatherList(
        __in PMDL  Mdl,
        __in size_t CurrentOffset,
        __in ULONG  Length,
        __in PDRIVER_LIST_CONTROL  ExecutionRoutine,
        __in PVOID  Context,
        __in PVOID  ScatterGatherBuffer,
        __in ULONG  ScatterGatherBufferLength
        )
    {
        NTSTATUS status;
        KIRQL irql;

        KeRaiseIrql(DISPATCH_LEVEL, &irql);

        if (m_DmaEnabler->UsesDmaV3()) {

            PDMA_OPERATIONS dmaOperations =
                m_AdapterInfo->AdapterObject->DmaOperations;
            ULONG flags = 0;

            if (GetDriverGlobals()->IsVersionGreaterThanOrEqualTo(1,15)) {
                //
                // Though the correct behavior is to pass the m_Flags to the
                // BuildScatterGatherListEx function, the code was not doing it
                // for versions <= 1.13. To reduce any chance of regression,
                // the m_Flags is honored for drivers that are 1.15
                // or newer.
                //
                flags = m_Flags;
            }

            status = dmaOperations->BuildScatterGatherListEx(
                                                 m_AdapterInfo->AdapterObject,
                                                 m_DmaEnabler->m_FDO,
                                                 GetTransferContext(),
                                                 Mdl,
                                                 CurrentOffset,
                                                 Length,
                                                 flags,
                                                 ExecutionRoutine,
                                                 Context,
                                                 (BOOLEAN) m_DmaDirection,
                                                 ScatterGatherBuffer,
                                                 ScatterGatherBufferLength,
                                                 NULL,
                                                 NULL,
                                                 NULL
                                                 );
        }
        else {

            status = m_AdapterInfo->AdapterObject->DmaOperations->
                        BuildScatterGatherList(m_AdapterInfo->AdapterObject,
                                               m_DmaEnabler->m_FDO,
                                               Mdl,
                                               GetStartVaFromOffset(Mdl, CurrentOffset),
                                               Length,
                                               ExecutionRoutine,
                                               Context,
                                               (BOOLEAN) m_DmaDirection,
                                               ScatterGatherBuffer,
                                               ScatterGatherBufferLength);
        }


        KeLowerIrql(irql);

        return status;
    }
};

class FxDmaPacketTransaction : public FxDmaTransactionBase {

protected:
    FxDmaPacketTransaction(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in USHORT ObjectSize,
        __in USHORT ExtraSize,
        __in FxDmaEnabler *DmaEnabler
        );

public:

    _Must_inspect_result_
    virtual
    NTSTATUS
    InitializeResources(
        VOID
        );

    _Must_inspect_result_
    NTSTATUS
    ReserveAdapter(
        __in     ULONG               NumberOfMapRegisters,
        __in     WDF_DMA_DIRECTION   Direction,
        __in     PFN_WDF_RESERVE_DMA Callback,
        __in_opt PVOID               Context
        );

    VOID
    ReleaseAdapter(
        VOID
        );

    _Must_inspect_result_
    virtual
    NTSTATUS
    StartTransfer(
        VOID
        );

    _Must_inspect_result_
    virtual
    NTSTATUS
    TransferCompleted(
        VOID
        );

    _Must_inspect_result_
    virtual
    NTSTATUS
    StageTransfer(
        VOID
        );

    virtual
    VOID
    ReleaseResources(
        __in BOOLEAN ForceRelease
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _Create(
        __in  PFX_DRIVER_GLOBALS      FxDriverGlobals,
        __in  PWDF_OBJECT_ATTRIBUTES  Attributes,
        __in  FxDmaEnabler*           DmaEnabler,
        __out WDFDMATRANSACTION*      Transaction
        );

    VOID
    SetDeviceAddressOffset(
        __in ULONG Offset
        )
    {
        m_DeviceAddressOffset = Offset;
    }

protected:

    //
    // Number of map registers to be used in this transfer.
    // This value is the least of the number of map registers
    // needed to satisfy the current transfer request, and the
    // number of available map registers returned by IoGetDmaAdapter.
    //
    ULONG                        m_MapRegistersNeeded;

    //
    // Opaque-value represents the map registers that the system has
    // assigned for this transfer operation. We pass this value in
    // FlushAdapterBuffers, FreeMapRegisters, and MapTransfer.
    //
    PVOID                         m_MapRegisterBase;

    //
    // TRUE when the map register base above is valid.  The HAL can give
    // us a NULL map register base when double buffering isn't required,
    // so we can't just do a NULL test on m_MapRegisterBase to know if
    // the DMA channel is allocated.
    //
    BOOLEAN                       m_MapRegisterBaseSet;

    //
    // For system-DMA this provides the offset from the original
    // DeviceAddress used to compute the device register to or from which
    // DMA should occur.
    //
    ULONG                         m_DeviceAddressOffset;

    //
    // 0 if the transaction has not reserved the enabler.  Otherwise
    // this is the number of map registers requested for the reservation.
    // This value persists across a reuse and reinitialization of the
    // transaction, and is only cleared when the enabler is released.
    //
    ULONG                         m_MapRegistersReserved;

    //
    // These fields are used to defer completion or staging while another
    // thread/CPU is already staging.  They are protected by the
    // transfer state lock.
    //
    // These values are cleared when checked, not in InitializeResources.
    // It's possible for a transaction being staged to complete on another
    // CPU, get reused and reinitialized.  Clearing these values in
    // InitializeResources would destroy the state the prior call to
    // StageTransfer depends on.
    //
    struct {

        //
        // Non-null when a staging operation is in progress on some CPU.
        // When set any attempt to call the DMA completion routine or
        // stage the transfer again (due to a call to TransferComplete)
        // will be deferred to this thread.
        //
        PKTHREAD                      CurrentStagingThread;

        //
        // Indicates that a nested or concurrent attempt to stage
        // the transaction was deferred.  The CurrentStagingThread
        // will restage the transaction.
        //
        BOOLEAN                       RerunStaging;

        //
        // Indicates that a nested or concurrent attempt to call the
        // DMA completion routine occurred.  The CurrentStagingThread
        // will call the DMA completion routine when it unwinds providing
        // the saved CompletionStatus
        //
        BOOLEAN                       RerunCompletion;
        DMA_COMPLETION_STATUS         CompletionStatus;

    } m_TransferState;

    //
    // Indicates that the DMA transfer has been cancelled.
    // Set during StopTransfer and cleared during InitializeResources
    // Checked during StageTransfer.  Stop and Initialize should never
    // race (it's not valid for the driver to stop an uninitialized
    // transaction).  Stop and Stage can race but in that case Stop
    // will mark the transaction context such that MapTransfer fails
    // (and that's protected by a HAL lock).
    //
    // So this field does not need to be volatile or interlocked, but
    // it needs to be sized to allow atomic writes.
    //
    ULONG                           m_IsCancelled;


    virtual
    VOID
    Reuse(
        VOID
        )
    {
        return;
    }

protected:

    inline
    void
    SetMapRegisterBase(
        __in PVOID Value
        )
    {
        NT_ASSERTMSG("Map register base is already set",
                     m_MapRegisterBaseSet == FALSE);

        m_MapRegisterBase = Value;
        m_MapRegisterBaseSet = TRUE;
    }

    inline
    void
    ClearMapRegisterBase(
        VOID
        )
    {
        NT_ASSERTMSG("Map register base was not set",
                     m_MapRegisterBaseSet == TRUE);
        m_MapRegisterBaseSet = FALSE;
    }

    inline
    BOOLEAN
    IsMapRegisterBaseSet(
        VOID
        )
    {
        return m_MapRegisterBaseSet;
    }

    inline
    PVOID
    GetMapRegisterBase(
        VOID
        )
    {
        NT_ASSERTMSG("Map register base is not set",
                     m_MapRegisterBaseSet == TRUE);
        return m_MapRegisterBase;
    }

    virtual
    IO_ALLOCATION_ACTION
    GetAdapterControlReturnValue(
        VOID
        )
    {
        return DeallocateObjectKeepRegisters;
    }

    virtual
    BOOLEAN
    PreMapTransfer(
        VOID
        )
    {
        return TRUE;
    }

    _Acquires_lock_(this)
    VOID
    __drv_raisesIRQL(DISPATCH_LEVEL)
#pragma prefast(suppress:__WARNING_FAILING_TO_ACQUIRE_MEDIUM_CONFIDENCE, "transferring lock name to 'this->TransferStateLock'")
#pragma prefast(suppress:__WARNING_FAILING_TO_RELEASE_MEDIUM_CONFIDENCE, "transferring lock name to 'this->TransferStateLock'")
    LockTransferState(
        __out __drv_deref(__drv_savesIRQL) KIRQL *OldIrql
        )
    {
        Lock(OldIrql);
    }

    _Requires_lock_held_(this)
    _Releases_lock_(this)
    VOID
#pragma prefast(suppress:__WARNING_FAILING_TO_RELEASE_MEDIUM_CONFIDENCE, "transferring lock name from 'this->TransferStateLock'")
    UnlockTransferState(
        __in __drv_restoresIRQL KIRQL OldIrql
        )
    {
#pragma prefast(suppress:__WARNING_CALLER_FAILING_TO_HOLD, "transferring lock name from 'this->TransferStateLock'")
        Unlock(OldIrql);
    }

    virtual
    PDMA_COMPLETION_ROUTINE
    GetTransferCompletionRoutine(
        VOID
        )
    {
        return NULL;
    }

    static
    IO_ALLOCATION_ACTION
    STDCALL
    _AdapterControl(
        __in PDEVICE_OBJECT  DeviceObject,
        __in PIRP            Irp,
        __in PVOID           MapRegisterBase,
        __in PVOID           Context
        );

    _Must_inspect_result_
    NTSTATUS
    AcquireDevice(
        VOID
        )
    {
        if (m_DmaEnabler->UsesDmaV3() == FALSE)
        {
            return m_DmaEnabler->GetDevice()->AcquireDmaPacketTransaction();
        }
        else
        {
            return STATUS_SUCCESS;
        }
    }

    FORCEINLINE
    VOID
    ReleaseDevice(
        VOID
        )
    {
        if (m_DmaEnabler->UsesDmaV3() == FALSE)
        {
            m_DmaEnabler->GetDevice()->ReleaseDmaPacketTransaction();
        }
    }

    _Must_inspect_result_
    NTSTATUS
    AllocateAdapterChannel(
        __in BOOLEAN MapRegistersReserved
        )
    {
        NTSTATUS status;
        KIRQL irql;

        KeRaiseIrql(DISPATCH_LEVEL, &irql);

        if (GetDriverGlobals()->FxVerifierOn) {

            if (MapRegistersReserved == FALSE) {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGDMA,
                    "Allocating %d map registers for "
                    "WDFDMATRANSACTION %p",
                    m_MapRegistersNeeded,
                    GetHandle()
                    );
            }
            else {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGDMA,
                    "Using %d reserved map registers for "
                    "WDFDMATRANSACTION %p",
                    m_MapRegistersNeeded,
                    GetHandle()
                    );
            }
        }

        if (m_DmaEnabler->UsesDmaV3()) {
            PDMA_OPERATIONS dmaOperations =
                m_AdapterInfo->AdapterObject->DmaOperations;

            if (MapRegistersReserved == FALSE)
            {
                status = dmaOperations->AllocateAdapterChannelEx(
                                        m_AdapterInfo->AdapterObject,
                                        m_DmaEnabler->m_FDO,
                                        GetTransferContext(),
                                        m_MapRegistersNeeded,
                                        m_Flags,
#pragma prefast(suppress: __WARNING_CLASS_MISMATCH_NONE, "This warning requires a wrapper class for the DRIVER_CONTROL type.")
                                        _AdapterControl,
                                        this,
                                        NULL
                                        );
            }
            else {
#pragma prefast(suppress:__WARNING_PASSING_FUNCTION_UNEXPECTED_NULL, "_AdapterControl does not actually use the IRP parameter.");
                _AdapterControl(m_DmaEnabler->m_FDO,
                                NULL,
                                GetMapRegisterBase(),
                                this);
                status = STATUS_SUCCESS;
            }
        }
        else {

            ASSERTMSG("Prereserved map registers are not compatible with DMA V2",
                      MapRegistersReserved == FALSE);

            status = m_AdapterInfo->AdapterObject->DmaOperations->
                        AllocateAdapterChannel(m_AdapterInfo->AdapterObject,
                                    m_DmaEnabler->m_FDO,
                                    m_MapRegistersNeeded,
#pragma prefast(suppress: __WARNING_CLASS_MISMATCH_NONE, "This warning requires a wrapper class for the DRIVER_CONTROL type.")
                                    _AdapterControl,
                                    this);
        }

        KeLowerIrql(irql);

        if (!NT_SUCCESS(status))
        {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDMA,
                "Allocating DMA resources (%d map registers) for WDFDMATRANSACTION %p "
                "returned %!STATUS!",
                m_MapRegistersNeeded,
                GetHandle(),
                status
                );
        }

        return status;
    }

    FORCEINLINE
    NTSTATUS
    MapTransfer(
        __out_bcount_opt(ScatterGatherListCb)
                 PSCATTER_GATHER_LIST     ScatterGatherList,
        __in     ULONG                    ScatterGatherListCb,
        __in_opt PDMA_COMPLETION_ROUTINE  CompletionRoutine,
        __in_opt PVOID                    CompletionContext,
        __out    ULONG                   *TransferLength
        )
    {
        //
        // Cache globals & object handle since call to MapTransferEx could
        // result in a DmaComplete callback before returning.
        //
        PFX_DRIVER_GLOBALS pFxDriverGlobals = GetDriverGlobals();
        WDFDMATRANSACTION handle = GetHandle();
#if DBG
        ULONG_PTR mapRegistersRequired;

        mapRegistersRequired = ADDRESS_AND_SIZE_TO_SPAN_PAGES(
                                    GetStartVaFromOffset(m_CurrentFragmentMdl,
                                                         m_CurrentFragmentOffset),
                                    m_CurrentFragmentLength
                                    );
        NT_ASSERTMSG("Mapping requires too many map registers",
                     mapRegistersRequired <= m_MapRegistersNeeded);
#endif

        NTSTATUS status;

        //
        // Assume we're going to transfer the entire current fragment.
        // MapTransfer may say otherwise.
        //

        *TransferLength = (ULONG) m_CurrentFragmentLength;

        //
        // Map the transfer.
        //

        if (pFxDriverGlobals->FxVerifierOn) {
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                                "Mapping transfer for WDFDMATRANSACTION %p.  "
                                "MDL %p, Offset %I64x, Length %x, MapRegisterBase %p",
                                handle,
                                m_CurrentFragmentMdl,
                                m_CurrentFragmentOffset,
                                *TransferLength,
                                GetMapRegisterBase());
        }

        if (m_DmaEnabler->UsesDmaV3()) {

            PDMA_OPERATIONS dmaOperations =
                m_AdapterInfo->AdapterObject->DmaOperations;

            status = dmaOperations->MapTransferEx(
                                 m_AdapterInfo->AdapterObject,
                                 m_CurrentFragmentMdl,
                                 GetMapRegisterBase(),
                                 m_CurrentFragmentOffset,
                                 m_DeviceAddressOffset,
                                 TransferLength,
                                 (BOOLEAN) m_DmaDirection,
                                 ScatterGatherList,
                                 ScatterGatherListCb,
                                 CompletionRoutine,
                                 CompletionContext
                                 );

            NT_ASSERTMSG(
                "With these parameters, MapTransferEx should never fail",
                NT_SUCCESS(status) || status == STATUS_CANCELLED
                );
        }
        else {
            NT_ASSERTMSG("cannot use DMA completion routine with DMAv2",
                         CompletionRoutine == NULL);

            NT_ASSERTMSG(
                "scatter gather list length must be large enough for at least one element",
                (ScatterGatherListCb >= (sizeof(SCATTER_GATHER_LIST) +
                                         sizeof(SCATTER_GATHER_ELEMENT)))
                );

            //
            // This matches the assertion above.  There's no way to explain to
            // prefast that this code path requires the caller to provide a buffer
            // of sufficient size to store the SGL.  The only case which doesn't
            // provide any buffer is system-mode DMA and that uses DMA v3 and so
            // won't go through this path.
            //

            __assume((ScatterGatherListCb >= (sizeof(SCATTER_GATHER_LIST) +
                                              sizeof(SCATTER_GATHER_ELEMENT))));

            ScatterGatherList->NumberOfElements = 1;
            ScatterGatherList->Reserved = 0;
            ScatterGatherList->Elements[0].Address =
                m_AdapterInfo->AdapterObject->DmaOperations->
                        MapTransfer(m_AdapterInfo->AdapterObject,
                                    m_CurrentFragmentMdl,
                                    GetMapRegisterBase(),
                                    GetStartVaFromOffset(m_CurrentFragmentMdl,
                                                         m_CurrentFragmentOffset),
                                    TransferLength,
                                    (BOOLEAN) m_DmaDirection);
            ScatterGatherList->Elements[0].Length = *TransferLength;

            status = STATUS_SUCCESS;
        }

        if (pFxDriverGlobals->FxVerifierOn) {
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                                "MapTransfer mapped next %d bytes of "
                                "WDFDMATRANSACTION %p - status %!STATUS!",
                                *TransferLength,
                                handle,
                                status);
        }

        return status;
    }

    FORCEINLINE
    NTSTATUS
    FlushAdapterBuffers(
        VOID
        )
    {
        PDMA_OPERATIONS dmaOperations =
            m_AdapterInfo->AdapterObject->DmaOperations;

        NTSTATUS status;

#if DBG
        ULONG_PTR mapRegistersRequired;

        mapRegistersRequired = ADDRESS_AND_SIZE_TO_SPAN_PAGES(
                                    GetStartVaFromOffset(m_CurrentFragmentMdl,
                                                         m_CurrentFragmentOffset),
                                    m_CurrentFragmentLength
                                    );
        NT_ASSERTMSG("Mapping requires too many map registers",
                     mapRegistersRequired <= m_MapRegistersNeeded);
#endif

        if (GetDriverGlobals()->FxVerifierOn) {
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGDMA,
                                "Flushing DMA buffers for WDFDMATRANSACTION %p.  "
                                "MDL %p, Offset %I64x, Length %I64x",
                                GetHandle(),
                                m_CurrentFragmentMdl,
                                m_CurrentFragmentOffset,
                                m_CurrentFragmentLength);
        }

        if (m_DmaEnabler->UsesDmaV3()) {
            status = dmaOperations->FlushAdapterBuffersEx(
                                              m_AdapterInfo->AdapterObject,
                                              m_CurrentFragmentMdl,
                                              GetMapRegisterBase(),
                                              m_CurrentFragmentOffset,
                                              (ULONG) m_CurrentFragmentLength,
                                              (BOOLEAN) m_DmaDirection
                                              );
        }
        else if (dmaOperations->FlushAdapterBuffers(
                                m_AdapterInfo->AdapterObject,
                                m_CurrentFragmentMdl,
                                GetMapRegisterBase(),
                                GetStartVaFromOffset(m_CurrentFragmentMdl,
                                                     m_CurrentFragmentOffset),
                                (ULONG) m_CurrentFragmentLength,
                                (BOOLEAN) m_DmaDirection) == FALSE) {
                status = STATUS_UNSUCCESSFUL;
        }
        else {
            status = STATUS_SUCCESS;
        }

        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDMA,
                                "Flushing DMA buffers for WDFDMATRANSACTION %p ("
                                "MDL %p, Offset %I64x, Length %I64x)"
                                "completed with %!STATUS!",
                                GetHandle(),
                                m_CurrentFragmentMdl,
                                m_CurrentFragmentOffset,
                                m_CurrentFragmentLength,
                                status);
        }

        return status;
    }

    virtual
    VOID
    FreeMapRegistersAndAdapter(
        VOID
        )
    {
        KIRQL irql;

        PVOID mapRegisterBase = GetMapRegisterBase();

        //
        // It's illegal to free a NULL map register base, even if the HAL gave it
        // to us.
        //
        if (mapRegisterBase == NULL) {
            if (GetDriverGlobals()->FxVerifierOn) {
                DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGDMA,
                                    "Skipping free of %d map registers for WDFDMATRANSACTION %p "
                                    "because base was NULL",
                                    m_MapRegistersNeeded,
                                    GetHandle());
            }

            return;
        }

        //
        // Free the map registers
        //
        KeRaiseIrql(DISPATCH_LEVEL, &irql);

        if (GetDriverGlobals()->FxVerifierOn) {
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGDMA,
                                "Freeing %d map registers for WDFDMATRANSACTION %p "
                                "(base %p)",
                                m_MapRegistersNeeded,
                                GetHandle(),
                                mapRegisterBase);
        }

        //
        // If we pre-reserved map registers then Reserved contains
        // the number to free.  Otherwise Needed is the number allocated
        // for the last transaction, which is the number to free.
        //
        m_AdapterInfo->AdapterObject->DmaOperations->
                    FreeMapRegisters(m_AdapterInfo->AdapterObject,
                                     mapRegisterBase,
                                     (m_MapRegistersReserved > 0 ?
                                        m_MapRegistersReserved :
                                        m_MapRegistersNeeded));
        KeLowerIrql(irql);

        return;
    }

    virtual
    VOID
    CallEvtDmaCompleted(
        __in DMA_COMPLETION_STATUS /* Status */
        )
    {
        //
        // Packet mode DMA doesn't support cancellation or
        // completion routines.  So this should never run.
        //
        ASSERTMSG("EvtDmaCompleted is not a valid callback for "
                  "a packet-mode transaction",
                  FALSE);
        return;
    }

};

class FxDmaSystemTransaction: public FxDmaPacketTransaction {

    friend FxDmaPacketTransaction;

public:

    FxDmaSystemTransaction(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in USHORT ExtraSize,
        __in FxDmaEnabler *DmaEnabler
        );

    _Must_inspect_result_
    static
    NTSTATUS
    _Create(
        __in  PFX_DRIVER_GLOBALS      FxDriverGlobals,
        __in  PWDF_OBJECT_ATTRIBUTES  Attributes,
        __in  FxDmaEnabler*           DmaEnabler,
        __out WDFDMATRANSACTION*      Transaction
        );

    VOID
    SetConfigureChannelCallback(
        __in_opt PFN_WDF_DMA_TRANSACTION_CONFIGURE_DMA_CHANNEL Callback,
        __in_opt PVOID Context
        )
    {
        m_ConfigureChannelFunction.Method = Callback;
        m_ConfigureChannelContext = Context;
    }

    VOID
    SetTransferCompleteCallback(
        __in_opt PFN_WDF_DMA_TRANSACTION_DMA_TRANSFER_COMPLETE Callback,
        __in_opt PVOID Context
        )
    {
        m_TransferCompleteFunction.Method = Callback;
        m_TransferCompleteContext = Context;
    }

    VOID
    StopTransfer(
        VOID
        );

protected:

    //
    // Callback and context for configure channel callback
    //
    FxDmaTransactionConfigureChannel m_ConfigureChannelFunction;
    PVOID                            m_ConfigureChannelContext;

    //
    // Callback and context for DMA completion callback
    //
    FxDmaTransactionTransferComplete m_TransferCompleteFunction;
    PVOID                            m_TransferCompleteContext;

    IO_ALLOCATION_ACTION
    GetAdapterControlReturnValue(
        VOID
        )
    {
        return KeepObject;
    }

    VOID
    FreeMapRegistersAndAdapter(
        VOID
        )
    {
        PFX_DRIVER_GLOBALS pFxDriverGlobals = GetDriverGlobals();
        KIRQL irql;

        KeRaiseIrql(DISPATCH_LEVEL, &irql);

        if (pFxDriverGlobals->FxVerifierOn) {
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                                "Freeing adapter channel for WDFDMATRANSACTION %p",
                                GetHandle());
        }

        m_AdapterInfo->AdapterObject->DmaOperations->
                    FreeAdapterChannel(m_AdapterInfo->AdapterObject);
        KeLowerIrql(irql);

        return;
    }

    BOOLEAN
    CancelMappedTransfer(
        VOID
        )
    {
        NTSTATUS status;

        ASSERT(m_DmaEnabler->UsesDmaV3());

        //
        // Cancel the transfer.  if it's not yet mapped this will mark the
        // TC so that mapping will fail.  If it's running this will invoke the
        // DMA completion routine.
        //
        status =
            m_AdapterInfo->AdapterObject->DmaOperations->CancelMappedTransfer(
                m_AdapterInfo->AdapterObject,
                GetTransferContext()
                );

        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGDMA,
                            "Stopping WDFDMATRANSACTION %p returned status %!STATUS!",
                            GetHandle(),
                            status);

        return NT_SUCCESS(status);
    }

    VOID
    Reuse(
        VOID
        )
    {
        FxDmaPacketTransaction::Reuse(); // __super call
        m_ConfigureChannelFunction.Method = NULL;
        m_ConfigureChannelContext = NULL;

        m_TransferCompleteFunction.Method = NULL;
        m_TransferCompleteContext = NULL;
    }

    VOID
    CallEvtDmaCompleted(
        __in DMA_COMPLETION_STATUS Status
        );

    virtual
    BOOLEAN
    PreMapTransfer(
        VOID
        );

    virtual
    PDMA_COMPLETION_ROUTINE
    GetTransferCompletionRoutine(
        VOID
        );

    static DMA_COMPLETION_ROUTINE _SystemDmaCompletion;
};

#endif // _FXDMATRANSACTION_HPP_
