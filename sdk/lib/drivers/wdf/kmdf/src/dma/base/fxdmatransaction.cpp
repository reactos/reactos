/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDmaTransaction.cpp

Abstract:

    WDF DMA Transaction Object

Environment:

    Kernel mode only.

Notes:


Revision History:

--*/

#include "fxdmapch.hpp"

extern "C" {
// #include "FxDmaTransaction.tmh"
}

FxDmaTransactionBase::FxDmaTransactionBase(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in USHORT ObjectSize,
    __in USHORT ExtraSize,
    __in FxDmaEnabler *DmaEnabler
    ) :
    FxNonPagedObject(
        FX_TYPE_DMA_TRANSACTION,
        ExtraSize == 0 ? ObjectSize : COMPUTE_OBJECT_SIZE(ObjectSize, ExtraSize),
        FxDriverGlobals)
{
    m_DmaEnabler            = DmaEnabler;
    m_EncodedRequest        = NULL;
    m_MaxFragmentLength     = 0;
    m_DmaDirection          = WdfDmaDirectionReadFromDevice;
    m_DmaAcquiredContext    = NULL;
    m_CurrentFragmentMdl    = NULL;
    m_CurrentFragmentOffset = 0;
    m_StartOffset           = NULL;
    m_StartMdl              = NULL;
    m_Remaining             = 0;
    m_CurrentFragmentLength = 0;
    m_TransactionLength     = 0;
    m_Transferred           = 0;
    m_Flags                 = 0;

    m_DmaAcquiredFunction.Method.ProgramDma = NULL;

    m_State = FxDmaTransactionStateCreated;

    if (ExtraSize == 0) {
        m_TransferContext = NULL;
    } else {
        m_TransferContext = WDF_PTR_ADD_OFFSET_TYPE(
                                this,
                                COMPUTE_RAW_OBJECT_SIZE(ObjectSize),
                                PVOID
                                );
    }

    MarkDisposeOverride(ObjectDoNotLock);
}

BOOLEAN
FxDmaTransactionBase::Dispose(
    VOID
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals = GetDriverGlobals();

    //
    // Must not be in transfer state.
    //
    if (m_State == FxDmaTransactionStateTransfer) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                    "WDFDMATRANSACTION %p state %!FxDmaTransactionState! "
                    "is invalid", GetHandle(), m_State);

        if (pFxDriverGlobals->IsVerificationEnabled(1, 9, OkForDownLevel)) {
            FxVerifierBugCheck(pFxDriverGlobals,               // globals
                               WDF_DMA_FATAL_ERROR,            // type
                               (ULONG_PTR) GetObjectHandle(),  // parm 2
                               (ULONG_PTR) m_State);           // parm 3
        }
    }

    m_State = FxDmaTransactionStateDeleted;

    //
    // Release resources for this Dma Transaction.
    //
    ReleaseResources(TRUE);

    if (m_EncodedRequest != NULL) {
        ClearRequest();
    }

    return TRUE;
}

_Must_inspect_result_
NTSTATUS
FxDmaTransactionBase::Initialize(
    __in PFN_WDF_PROGRAM_DMA     ProgramDmaFunction,
    __in WDF_DMA_DIRECTION       DmaDirection,
    __in PMDL                    Mdl,
    __in size_t                  Offset,
    __in ULONG                   Length
    )
{
    NTSTATUS status;
    PFX_DRIVER_GLOBALS pFxDriverGlobals = GetDriverGlobals();

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                        "Enter WDFDMATRANSACTION %p", GetHandle());
    //
    // Must be in Reserve, Created or Released state.
    //
    if (m_State != FxDmaTransactionStateCreated &&
        m_State != FxDmaTransactionStateReserved &&
        m_State != FxDmaTransactionStateReleased) {

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                    "WDFDMATRANSACTION %p state %!FxDmaTransactionState! "
                    "is invalid", GetHandle(), m_State);

        FxVerifierBugCheck(pFxDriverGlobals,                // globals
                           WDF_DMA_FATAL_ERROR, // specific type
                           (ULONG_PTR) GetObjectHandle(),  // parm 2
                           (ULONG_PTR) m_State);           // parm 3
    }

    if (DmaDirection == WdfDmaDirectionReadFromDevice) {
        m_AdapterInfo = m_DmaEnabler->GetReadDmaDescription();
    } else {
        m_AdapterInfo = m_DmaEnabler->GetWriteDmaDescription();
    }

    //
    // Initialize the DmaTransaction object
    //

    m_MaxFragmentLength  = m_AdapterInfo->MaximumFragmentLength;
    m_DmaDirection       = DmaDirection;
    m_StartMdl           = Mdl;
    m_StartOffset        = Offset;
    m_CurrentFragmentMdl = Mdl;
    m_CurrentFragmentOffset = Offset;
    m_Remaining          = Length;
    m_TransactionLength  = Length;
    m_DmaAcquiredFunction.Method.ProgramDma = ProgramDmaFunction;

    //
    // If needed, initialize the transfer context.
    //

    if (m_DmaEnabler->UsesDmaV3()) {
        m_DmaEnabler->InitializeTransferContext(GetTransferContext(),
                                                m_DmaDirection);
    }

    status = InitializeResources();
    if (NT_SUCCESS(status)) {
        m_State = FxDmaTransactionStateInitialized;
    } else {
        ReleaseForReuse(FALSE);
    }

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                        "Exit WDFDMATRANSACTION %p, %!STATUS!",
                        GetHandle(), status);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxDmaTransactionBase::Execute(
    __in PVOID          Context
    )
{
    NTSTATUS status;
    PFX_DRIVER_GLOBALS pFxDriverGlobals = GetDriverGlobals();

    //
    // Must be in Initialized state.
    //
    if (m_State != FxDmaTransactionStateInitialized) {

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                    "WDFDMATRANSACTION %p state %!FxDmaTransactionState! "
                    "is invalid", GetHandle(), m_State);

        FxVerifierBugCheck(pFxDriverGlobals,                // globals
                           WDF_DMA_FATAL_ERROR, // specific type
                           (ULONG_PTR) GetObjectHandle(),  // parm 2
                           (ULONG_PTR) m_State);           // parm 3
    }

    //
    // If this was initialized with a request, then reference the
    // request now.
    //
    if (m_EncodedRequest != NULL) {
        ReferenceRequest();
    }

    //
    // Set state to Transfer.
    // This is necessary because the Execute path complete
    // all the way to DmaCompleted before returning to this point.
    //
    m_State = FxDmaTransactionStateTransfer;

    //
    // Save the caller's context
    //
    m_DmaAcquiredContext = Context;

    ASSERT(m_Transferred == 0);
    ASSERT(m_CurrentFragmentLength == 0);

    status = StartTransfer();
    if (!NT_SUCCESS(status)) {
        m_State = FxDmaTransactionStateTransferFailed;
        m_DmaAcquiredContext = NULL;

        if (m_EncodedRequest != NULL) {
            ReleaseButRetainRequest();
        }
    }

    //
    // StartTransfer results in a call to the EvtProgramDma routine
    // where driver could complete and delete the object.  So
    // don't touch the object beyond this point.
    //

    return status;
}

BOOLEAN
FxDmaTransactionBase::DmaCompleted(
    __in  size_t      TransferredLength,
    __out NTSTATUS  * ReturnStatus,
    __in  FxDmaCompletionType CompletionType
    )
{
    BOOLEAN         hasTransitioned;
    NTSTATUS        status;
    PFX_DRIVER_GLOBALS pFxDriverGlobals = GetDriverGlobals();
    WDFDMATRANSACTION dmaTransaction;

    //
    // In the case of partial completion, we will start a new transfer
    // from with in this function by calling StageTransfer. After that
    // call, we lose ownership of the object.  Since we need the handle
    // for tracing purposes, we will save the value in a local variable and
    // use that.
    //
    dmaTransaction = GetHandle();

    if (pFxDriverGlobals->FxVerifierOn) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                        "Enter WDFDMATRANSACTION %p, length %d",
                        dmaTransaction, (ULONG)TransferredLength);
    }

    //
    // Must be in Transfer state.
    //
    if (m_State != FxDmaTransactionStateTransfer) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                    "WDFDMATRANSACTION %p state %!FxDmaTransactionState! "
                    "is invalid", dmaTransaction, m_State);

        FxVerifierBugCheck(pFxDriverGlobals,                // globals
                            WDF_DMA_FATAL_ERROR, // specific type
                            (ULONG_PTR) dmaTransaction,  // parm 2
                            (ULONG_PTR) m_State);           // parm 3
    }

    if (TransferredLength > m_CurrentFragmentLength) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                    "WDFDMATRANSACTION %p Transfered Length %I64d can't be more "
                    "than the length asked to transfer %I64d "
                    "%!STATUS!", dmaTransaction, TransferredLength,
                    m_CurrentFragmentLength, status);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        goto End;
    }


    if (CompletionType == FxDmaCompletionTypePartial ||
        CompletionType == FxDmaCompletionTypeAbort) {
        //
        // Tally this DMA tranferred byte count into the accumulator.
        //
        m_Transferred += TransferredLength;

        //
        // Adjust the remaining length to account for the partial transfer.
        //
        m_Remaining += (m_CurrentFragmentLength - TransferredLength);

        //
        // Update CurrentDmaLength to reflect actual transfer because
        // we need to FlushAdapterBuffers based on this value in
        // TransferCompleted for packet based transfer.
        //
        m_CurrentFragmentLength = TransferredLength;

    } else {
        //
        // Tally this DMA tranferred byte count into the accumulator.
        //
        m_Transferred += m_CurrentFragmentLength;
    }

    ASSERT(m_Transferred <= m_TransactionLength);

    //
    // Inform the derived object that transfer is completed so it
    // can release resources specific to last transfer.
    //
    status = TransferCompleted();
    if (!NT_SUCCESS(status)) {
        goto End;
    }

    //
    // If remaining DmaTransaction length is zero or if the driver wants
    // this to be the last transfer then free the map registers and
    // change the state to completed.
    //
    if (m_Remaining == 0 || CompletionType == FxDmaCompletionTypeAbort) {
        status = STATUS_SUCCESS;
        goto End;
    }

    //
    // Stage the next packet for this DmaTransaction...
    //
    status = StageTransfer();

    if (NT_SUCCESS(status)) {
        //
        // StageTransfer results in a call to the EvtProgramDma routine
        // where driver could complete and delete the object.  So
        // don't touch the object beyond this point.
        //
        status = STATUS_MORE_PROCESSING_REQUIRED;
    }
    else {
        //
        // The error will be returned to the caller of
        // WdfDmaTransactionDmaComplete*()
        //
    }

End:

    if (status != STATUS_MORE_PROCESSING_REQUIRED) {
        //
        // Failed or succeeded. Either way free
        // map registers and release the device.
        //
        if (NT_SUCCESS(status)) {
            m_State = FxDmaTransactionStateTransferCompleted;
        } else {
            m_State = FxDmaTransactionStateTransferFailed;
        }

        if (pFxDriverGlobals->FxVerifierOn) {
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                                "WDFDMATRANSACTION %p completed with status %!STATUS! - "
                                "releasing DMA resources",
                                GetHandle(),
                                status);
        }

        ReleaseResources(FALSE);

        if (m_EncodedRequest != NULL) {
            ReleaseButRetainRequest();
        }

        m_CurrentFragmentLength = 0;

        hasTransitioned = TRUE;
    } else {
        hasTransitioned = FALSE;
    }

    *ReturnStatus = status;

    if (pFxDriverGlobals->FxVerifierOn) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                        "Exit WDFDMATRANSACTION %p "
                        "Transitioned(%!BOOLEAN!)",
                        dmaTransaction, hasTransitioned);
    }

    return hasTransitioned;
}

VOID
FxDmaTransactionBase::ReleaseForReuse(
    __in BOOLEAN ForceRelease
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals = GetDriverGlobals();

    if (ForceRelease == FALSE)
    {
        if (m_State == FxDmaTransactionStateReleased) {

            //
            // Double release is probably due to cancel during early in transaction
            // initialization.  DC2 on very slow machines shows this behavior.
            // The double release case is rare and benign.
            //
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_WARNING, TRACINGDMA,
                                "WDFDMATRANSACTION %p is already released, "
                                "%!STATUS!", GetHandle(), STATUS_SUCCESS);

            return;  // already released.
        }

        //
        // Must not be in transfer state.
        //
        if (m_State == FxDmaTransactionStateTransfer) {
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                        "WDFDMATRANSACTION %p state %!FxDmaTransactionState! "
                        "is invalid (release transaction)", GetHandle(), m_State);

            if (pFxDriverGlobals->IsVerificationEnabled(1, 11, OkForDownLevel)) {
                FxVerifierBugCheck(pFxDriverGlobals,               // globals
                                   WDF_DMA_FATAL_ERROR,            // type
                                   (ULONG_PTR) GetObjectHandle(),  // parm 2
                                   (ULONG_PTR) m_State);           // parm 3
            }
        }
    }

    m_State = FxDmaTransactionStateReleased;

    ReleaseResources(ForceRelease);

    //
    // Except DMA enabler field and adapter info everything else should be
    // cleared.  Adapter info is cleared by ReleaseResources above.
    //
    m_DmaAcquiredContext = NULL;

    if (m_EncodedRequest != NULL) {
        ClearRequest();
    }

    m_StartMdl = NULL;
    m_CurrentFragmentMdl = NULL;
    m_StartOffset = 0;
    m_CurrentFragmentOffset = 0;
    m_CurrentFragmentLength = 0;
    m_Transferred = 0;
    m_Remaining = 0;
    m_MaxFragmentLength = 0;
    m_TransactionLength = 0;
    m_Flags = 0;

    m_DmaAcquiredFunction.Method.ProgramDma = NULL;

}

VOID
FxDmaTransactionBase::SetImmediateExecution(
    __in BOOLEAN Value
    )
{
    if (m_State != FxDmaTransactionStateCreated &&
        m_State != FxDmaTransactionStateInitialized &&
        m_State != FxDmaTransactionStateReleased) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDMA,
            "Must set immediate execution flag for WDFDMATRANSACTION "
            "%p before calling AllocateResources or Execute (current "
            "state is %!FxDmaTransactionState!)",
            GetHandle(),
            m_State
            );
        FxVerifierDbgBreakPoint(GetDriverGlobals());
    }

    if (Value) {
        m_Flags |= DMA_SYNCHRONOUS_CALLBACK;
    }
    else {
        m_Flags &= ~DMA_SYNCHRONOUS_CALLBACK;
    }
}

BOOLEAN
FxDmaTransactionBase::CancelResourceAllocation(
    VOID
    )
{
    if ((m_State == FxDmaTransactionStateCreated) ||
        (m_State == FxDmaTransactionStateReleased) ||
        (m_State == FxDmaTransactionStateDeleted)) {

        DoTraceLevelMessage(
            GetDriverGlobals(),
            TRACE_LEVEL_ERROR,
            TRACINGDMA,
            "WDFDMATRANSACTION %p cannot be cancelled in state "
            "%!FxDmaTransactionState!",
            GetHandle(),
            m_State
            );

        FxVerifierBugCheck(GetDriverGlobals(),
                           WDF_DMA_FATAL_ERROR,
                           (ULONG_PTR) GetObjectHandle(),
                           (ULONG_PTR) m_State);
        // unreachable code
    }

    PDMA_OPERATIONS dmaOperations =
        m_AdapterInfo->AdapterObject->DmaOperations;

    BOOLEAN result;

    result = dmaOperations->CancelAdapterChannel(
                            m_AdapterInfo->AdapterObject,
                            m_DmaEnabler->m_FDO,
                            GetTransferContext()
                            );

    if (result) {
        m_State = FxDmaTransactionStateTransferFailed;

        if (m_EncodedRequest != NULL) {
            ReleaseButRetainRequest();
        }
    }

    return result;
}

VOID
FxDmaTransactionBase::_ComputeNextTransferAddress(
    __in PMDL CurrentMdl,
    __in size_t CurrentOffset,
    __in ULONG Transferred,
    __deref_out PMDL  *NextMdl,
    __out size_t *NextOffset
    )
/*++

Routine Description:

    This function computes the next mdl and offset given the current MDL,
    offset and bytes transfered.

Arguments:

    CurrentMdl - Mdl where the transfer currently took place.

    CurrentVa - Current virtual address in the buffer

    Transfered - Bytes transfered or to be transfered

    NextMdl - Mdl where the next transfer will take place

    NextVA - Offset within NextMdl where the transfer will start

--*/
{
    size_t transfered, mdlSize;
    PMDL mdl;

    mdlSize = MmGetMdlByteCount(CurrentMdl) - CurrentOffset;

    if (Transferred < mdlSize) {
        //
        // We are still in the first MDL
        //
        *NextMdl = CurrentMdl;
        *NextOffset = CurrentOffset + Transferred;
        return;
    }

    //
    // We have transfered the content of the first MDL.
    // Move to the next one.
    //
    transfered = Transferred - mdlSize;
    mdl = CurrentMdl->Next;
    ASSERT(mdl != NULL);

    while (transfered >= MmGetMdlByteCount(mdl)) {
        //
        // We have transfered the content of this MDL.
        // Move to the next one.
        //
        transfered -= MmGetMdlByteCount(mdl);
        mdl = mdl->Next;
        ASSERT(mdl != NULL);
    }

    //
    // This is the mdl where the last transfer occured.
    //
    *NextMdl = mdl;
    *NextOffset = transfered;

    return;
}

_Must_inspect_result_
NTSTATUS
FxDmaTransactionBase::_CalculateRequiredMapRegisters(
    __in PMDL Mdl,
    __in size_t CurrentOffset,
    __in ULONG Length,
    __in ULONG AvailableMapRegisters,
    __out_opt PULONG PossibleTransferLength,
    __out PULONG MapRegistersRequired
    )
/*++

Routine Description:

    Used on Windows 2000 to compute number of map registered required
    for this transfer. This is derived from HalCalculateScatterGatherListSize.

Arguments:

    Mdl - Pointer to the MDL that describes the pages of memory that are being
        read or written.

    CurrentVa - Current virtual address in the buffer described by the MDL
        that the transfer is being done to or from.

    Length - Supplies the length of the transfer.

    AvailableMapRegisters - Map registers available to do the transfer

    PossibleTransferLength - Length that can transfered for the

    MapRegistersRequired - Map registers required to the entire transfer

Return Value:

    NTSTATUS

Notes:

--*/
{
    PMDL tempMdl;
    ULONG requiredMapRegisters;
    ULONG transferLength;
    ULONG mdlLength;
    ULONG pageOffset;
    ULONG possTransferLength;

    //
    // Calculate the number of required map registers.
    //
    tempMdl = Mdl;
    transferLength = (ULONG) MmGetMdlByteCount(tempMdl) - (ULONG) CurrentOffset;
    mdlLength = transferLength;

    pageOffset = BYTE_OFFSET(GetStartVaFromOffset(tempMdl, CurrentOffset));
    requiredMapRegisters = 0;
    possTransferLength = 0;

    //
    // The virtual address should fit in the first MDL.
    //

    ASSERT(CurrentOffset <= tempMdl->ByteCount);

    //
    // Loop through chained MDLs, accumulating the required
    // number of map registers.
    //

    while (transferLength < Length && tempMdl->Next != NULL) {

        //
        // With pageOffset and length, calculate number of pages spanned by
        // the buffer.
        //
        requiredMapRegisters += (pageOffset + mdlLength + PAGE_SIZE - 1) >>
                                    PAGE_SHIFT;

        if (requiredMapRegisters <= AvailableMapRegisters) {
            possTransferLength = transferLength;
        }

        tempMdl = tempMdl->Next;
        pageOffset = tempMdl->ByteOffset;
        mdlLength = tempMdl->ByteCount;
        transferLength += mdlLength;
    }

    if ((transferLength + PAGE_SIZE) < (Length + pageOffset )) {
        ASSERT(transferLength >= Length);
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Calculate the last number of map registers based on the requested
    // length not the length of the last MDL.
    //

    ASSERT( transferLength <= mdlLength + Length );

    requiredMapRegisters += (pageOffset + Length + mdlLength - transferLength +
                             PAGE_SIZE - 1) >> PAGE_SHIFT;

    if (requiredMapRegisters <= AvailableMapRegisters) {
        possTransferLength += (Length + mdlLength - transferLength);
    }

    if (PossibleTransferLength != NULL) {
        *PossibleTransferLength = possTransferLength;
    }

    ASSERT(*PossibleTransferLength);

    *MapRegistersRequired = requiredMapRegisters;

    return STATUS_SUCCESS;
}

// ----------------------------------------------------------------------------
// ------------------- Scatter/Gather DMA Section -----------------------------
// ----------------------------------------------------------------------------

FxDmaScatterGatherTransaction::FxDmaScatterGatherTransaction(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in USHORT ExtraSize,
    __in FxDmaEnabler *DmaEnabler
    ) :
    FxDmaTransactionBase(FxDriverGlobals,
                         sizeof(FxDmaScatterGatherTransaction),
                         ExtraSize,
                         DmaEnabler)
{
    m_LookasideBuffer       = NULL;
    m_SGList                = NULL;
}

_Must_inspect_result_
NTSTATUS
FxDmaScatterGatherTransaction::_Create(
    __in  PFX_DRIVER_GLOBALS      FxDriverGlobals,
    __in  PWDF_OBJECT_ATTRIBUTES  Attributes,
    __in  FxDmaEnabler*           DmaEnabler,
    __out WDFDMATRANSACTION*      Transaction
    )
{
    FxDmaScatterGatherTransaction* pTransaction;
    WDFOBJECT hTransaction;
    NTSTATUS status;

    pTransaction = new (FxDriverGlobals, Attributes, DmaEnabler->GetTransferContextSize())
                FxDmaScatterGatherTransaction(FxDriverGlobals,
                                              DmaEnabler->GetTransferContextSize(),
                                              DmaEnabler);

    if (pTransaction == NULL) {
        status =  STATUS_INSUFFICIENT_RESOURCES;
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
            "Could not allocate memory for WDFTRANSACTION, %!STATUS!", status);
        return status;
    }

    //
    // Commit and apply the attributes
    //
    status = pTransaction->Commit(Attributes, &hTransaction, DmaEnabler);

    if (NT_SUCCESS(status) && DmaEnabler->m_IsSGListAllocated) {

        //
        // Allocate buffer for SGList from lookaside list.
        //
        pTransaction->m_LookasideBuffer = (SCATTER_GATHER_LIST *)
            FxAllocateFromNPagedLookasideList(
                &DmaEnabler->m_SGList.ScatterGatherProfile.Lookaside
                );

        if (pTransaction->m_LookasideBuffer == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                                "Unable to allocate memory for SG List, "
                                "WDFDMATRANSACTION %p, %!STATUS! ",
                                pTransaction->GetHandle(), status);
        }
        else {
            //
            // Take a reference on the enabler to ensure that it remains valid
            // if the transaction's disposal is deferred.
            //
            DmaEnabler->ADDREF(pTransaction);
        }
    }

    if (NT_SUCCESS(status)) {
        *Transaction = (WDFDMATRANSACTION)hTransaction;
    }
    else {
        //
        // This will properly clean up the target's state and free it
        //
        pTransaction->DeleteFromFailedCreate();
    }

    return status;
}

BOOLEAN
FxDmaScatterGatherTransaction::Dispose(
    VOID
    )
{
    BOOLEAN ret;

    ret = FxDmaTransactionBase::Dispose(); // __super call

    //
    // Free Lookaside Buffer which held SGList
    //
    if (m_LookasideBuffer != NULL) {

        FxFreeToNPagedLookasideList(
            &m_DmaEnabler->m_SGList.ScatterGatherProfile.Lookaside,
            m_LookasideBuffer
            );
        m_LookasideBuffer = NULL;
        m_DmaEnabler->RELEASE(this);
    }

    return ret;
}

_Must_inspect_result_
NTSTATUS
FxDmaScatterGatherTransaction::InitializeResources(
    VOID
    )
{
    NTSTATUS status;
    PMDL  nextMdl;
    size_t nextOffset;
    ULONG mapRegistersRequired;
    size_t remLength, transferLength, transferred, possibleLength=0;
    PFX_DRIVER_GLOBALS pFxDriverGlobals = GetDriverGlobals();

    status = STATUS_SUCCESS;

    //
    // If the caller has specified a limit on the number of scatter-gather
    // elements each transfer can support then make sure it's within the
    // limit by breaking up the whole transfer into m_MaxFragmentLength and
    // computing the number of map-registers required for each fragment.
    // This check may not be valid if the driver starts to do partial
    // transfers. So driver that do partial transfer with sg-element limit
    // should be capable of handling STATUS_WDF_TOO_FRAGMENTED failures during
    // dma execution.
    //
    remLength = m_TransactionLength;
    transferred = 0;
    nextMdl = m_StartMdl;
    nextOffset = m_StartOffset;
    transferLength = 0;

    while (remLength != 0) {

        _ComputeNextTransferAddress(nextMdl,
                                    nextOffset,
                                    (ULONG) transferLength,
                                    &nextMdl,
                                    &nextOffset);

        transferLength = FxSizeTMin(remLength, m_MaxFragmentLength);

        status = _CalculateRequiredMapRegisters(nextMdl,
                                     nextOffset,
                                     (ULONG) transferLength,
                                     m_AdapterInfo->NumberOfMapRegisters,
                                     (PULONG) &possibleLength,
                                     &mapRegistersRequired
                                     );

        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                "CalculateScatterGatherList failed for "
                "WDFDMATRANSACTION %p, %!STATUS!", GetHandle(), status);
            FxVerifierDbgBreakPoint(pFxDriverGlobals);
            return status;
        }

        if (mapRegistersRequired > m_DmaEnabler->m_MaxSGElements) {
            status = STATUS_WDF_TOO_FRAGMENTED;
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                        "WDFDMATRANSACTION %p for MDL %p is more fragmented (%d) "
                        "than the limit (%I64d) specified by the driver, %!STATUS! ",
                        GetHandle(), nextMdl, mapRegistersRequired,
                        m_DmaEnabler->m_MaxSGElements, status);
            return status;
        }

        transferred += transferLength;
        remLength -= transferLength;
    }

    return status;
}

VOID
FxDmaScatterGatherTransaction::ReleaseResources(
    __in BOOLEAN /* ForceRelease */
    )
{
    if (m_SGList != NULL) {
        PutScatterGatherList(m_SGList);
        m_SGList = NULL;
    }
    m_AdapterInfo = NULL;
}

_Must_inspect_result_
NTSTATUS
FxDmaScatterGatherTransaction::StartTransfer(
    VOID
    )
{
    ASSERT(m_CurrentFragmentMdl == m_StartMdl);
    ASSERT(m_CurrentFragmentOffset == m_StartOffset);
    ASSERT(m_CurrentFragmentLength == 0);
    ASSERT(m_Transferred == 0);

    return StageTransfer();
}

_Must_inspect_result_
NTSTATUS
FxDmaScatterGatherTransaction::StageTransfer(
    VOID
    )
{
    NTSTATUS           status;
    ULONG   mapRegistersRequired;
    WDFDMATRANSACTION dmaTransaction;
    PFX_DRIVER_GLOBALS pFxDriverGlobals = GetDriverGlobals();

    //
    // Use an invalid value to make the function fail if the var is not
    // updated correctly below.
    //
    mapRegistersRequired = 0xFFFFFFFF;

    //
    // Client driver could complete and delete the object in
    // EvtProgramDmaFunction. So, save the handle because we need it
    // for tracing after we invoke the callback.
    //
    dmaTransaction = GetHandle();

    if (pFxDriverGlobals->FxVerifierOn) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                        "Enter WDFDMATRANSACTION %p ", GetHandle());
    }

    //
    // Given the first MDL and the bytes transfered, find the next MDL
    // and byteoffset within that MDL.
    //
    _ComputeNextTransferAddress(m_CurrentFragmentMdl,
                                m_CurrentFragmentOffset,
                                (ULONG) m_CurrentFragmentLength,
                                &m_CurrentFragmentMdl,
                                &m_CurrentFragmentOffset);

    //
    // Get the next possible transfer size.
    //
    m_CurrentFragmentLength = FxSizeTMin(m_Remaining, m_MaxFragmentLength);

    //
    // Fix m_CurrentFragmentLength to meet the map registers limit. This is done
    // in case the MDL is a chained MDL for an highly fragmented buffer.
    //
    status = _CalculateRequiredMapRegisters(m_CurrentFragmentMdl,
                                   m_CurrentFragmentOffset,
                                   (ULONG) m_CurrentFragmentLength,
                                   m_AdapterInfo->NumberOfMapRegisters,
                                   (PULONG)&m_CurrentFragmentLength,
                                   &mapRegistersRequired);
    //
    // We have already validated the entire transfer during initialize
    // to see each transfer meets the sglimit. So this call shouldn't fail.
    // But, if the driver does partial transfer and changes the fragment
    // boundaries then it's possible for the sg-elements to vary. So, check
    // one more time to see if we are within the bounds before building
    // the sglist and calling into the driver.
    //
    ASSERT(NT_SUCCESS(status));

    if (mapRegistersRequired > m_DmaEnabler->m_MaxSGElements) {
        status = STATUS_WDF_TOO_FRAGMENTED;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                    "WDFDMATRANSACTION %p for MDL %p is more fragmented (%d) "
                    "than the limit (%I64d) specified by the driver, %!STATUS! ",
                    dmaTransaction, m_CurrentFragmentMdl, mapRegistersRequired,
                    m_DmaEnabler->m_MaxSGElements, status);
        return status;
    }


    m_Remaining -= m_CurrentFragmentLength;

    if (m_DmaEnabler->m_IsSGListAllocated) {

        ASSERT(m_LookasideBuffer != NULL);
        status = BuildScatterGatherList(m_CurrentFragmentMdl,
                                        m_CurrentFragmentOffset,
                                        (ULONG) m_CurrentFragmentLength,
#pragma prefast(suppress: __WARNING_CLASS_MISMATCH_NONE, "This warning requires a wrapper class for the DRIVER_LIST_CONTROL type.")
                                        _AdapterListControl,
                                        this,
                                        m_LookasideBuffer,
                                        (ULONG) m_AdapterInfo->PreallocatedSGListSize);

    } else {

        status = GetScatterGatherList(m_CurrentFragmentMdl,
                                      m_CurrentFragmentOffset,
                                      (ULONG) m_CurrentFragmentLength,
#pragma prefast(suppress: __WARNING_CLASS_MISMATCH_NONE, "This warning requires a wrapper class for the DRIVER_LIST_CONTROL type.")
                                      _AdapterListControl,
                                      this);
    }

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "Build or GetScatterGatherList failed for "
                            "WDFDMATRANSACTION %p, %!STATUS!",
                            dmaTransaction, status);
        //
        // Readjust remaining bytes transfered.
        //
        m_Remaining += m_CurrentFragmentLength;
        return status;
    }

    //
    // Before GetScatterGatherList returns, _AdapterListControl can get called
    // on another thread and the driver could delete the transaction object.
    // So don't touch the object after this point.
    //

    if (pFxDriverGlobals->FxVerifierOn) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                        "Exit WDFDMATRANSACTION %p, "
                        "%!STATUS!", dmaTransaction, status);
    }

    return status;
}


VOID
FxDmaScatterGatherTransaction::_AdapterListControl(
    __in  PDEVICE_OBJECT         DeviceObject,
    __in  PIRP                   Irp,            // UNUSED
    __in  PSCATTER_GATHER_LIST   SgList,
    __in  PVOID                  Context
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    WDFDMATRANSACTION dmaTransaction;
    FxDmaScatterGatherTransaction * pDmaTransaction;

    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(DeviceObject);

    pDmaTransaction = (FxDmaScatterGatherTransaction*) Context;
    pFxDriverGlobals = pDmaTransaction->GetDriverGlobals();
    dmaTransaction = pDmaTransaction->GetHandle();

    if (pFxDriverGlobals->FxVerifierOn) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                        "Enter WDFDMATRANSACTION %p",
                        dmaTransaction);
    }

    ASSERT(pDmaTransaction != NULL);
    ASSERT(pDmaTransaction->m_DmaAcquiredFunction.Method.ProgramDma != NULL);

    ASSERT(SgList->NumberOfElements <= pDmaTransaction->m_DmaEnabler->GetMaxSGElements());

    pDmaTransaction->m_SGList = SgList;

    //
    // We ignore the return value. The pattern we want the driver to follow is
    // that if it fails to program DMA transfer, it should call DmaCompletedFinal
    // to abort the transfer.
    //
    (VOID) pDmaTransaction->m_DmaAcquiredFunction.InvokeProgramDma(
                dmaTransaction,
                pDmaTransaction->m_DmaEnabler->m_DeviceBase->GetHandle(),
                pDmaTransaction->m_DmaAcquiredContext,
                pDmaTransaction->m_DmaDirection,
                SgList);

    if (pFxDriverGlobals->FxVerifierOn) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                        "Exit WDFDMATRANSACTION %p",
                        dmaTransaction);
    }
}

_Must_inspect_result_
NTSTATUS
FxDmaScatterGatherTransaction::TransferCompleted(
    VOID
    )
{
    //
    // All we have to do is release the scatter-gather list.
    //
    if (m_SGList != NULL) {

        PutScatterGatherList(m_SGList);
        m_SGList = NULL;
    }

    return STATUS_SUCCESS;
}


// ----------------------------------------------------------------------------
// ------------------- PACKET DMA SECTION -------------------------------------
// ----------------------------------------------------------------------------

FxDmaPacketTransaction::FxDmaPacketTransaction(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in USHORT ObjectSize,
    __in USHORT ExtraSize,
    __in FxDmaEnabler *DmaEnabler
    ) :
    FxDmaTransactionBase(FxDriverGlobals, ObjectSize, ExtraSize, DmaEnabler)
{
    m_MapRegistersNeeded  = 0;
    m_MapRegisterBase     = NULL;
    m_MapRegisterBaseSet = FALSE;
    m_DeviceAddressOffset = 0;
    m_MapRegistersReserved = 0;

    m_IsCancelled = FALSE;

    m_TransferState.CurrentStagingThread = NULL;
    m_TransferState.RerunStaging = FALSE;
    m_TransferState.RerunCompletion = FALSE;
    m_TransferState.CompletionStatus = UNDEFINED_DMA_COMPLETION_STATUS;
}

_Must_inspect_result_
NTSTATUS
FxDmaPacketTransaction::_Create(
    __in  PFX_DRIVER_GLOBALS      FxDriverGlobals,
    __in  PWDF_OBJECT_ATTRIBUTES  Attributes,
    __in  FxDmaEnabler*           DmaEnabler,
    __out WDFDMATRANSACTION*      Transaction
    )
{
    FxDmaPacketTransaction* pTransaction;
    WDFOBJECT hTransaction;
    NTSTATUS status;

    pTransaction = new (FxDriverGlobals, Attributes, DmaEnabler->GetTransferContextSize())
                FxDmaPacketTransaction(FxDriverGlobals,
                                       sizeof(FxDmaPacketTransaction),
                                       DmaEnabler->GetTransferContextSize(),
                                       DmaEnabler);

    if (pTransaction == NULL) {
        status =  STATUS_INSUFFICIENT_RESOURCES;
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
            "Could not allocate memory for WDFTRANSACTION, %!STATUS!", status);
        return status;
    }

    //
    // Commit and apply the attributes
    //
    status = pTransaction->Commit(Attributes, &hTransaction, DmaEnabler);
    if (NT_SUCCESS(status)) {
        *Transaction = (WDFDMATRANSACTION)hTransaction;
    }
    else {
        //
        // This will properly clean up the target's state and free it
        //
        pTransaction->DeleteFromFailedCreate();
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxDmaPacketTransaction::InitializeResources(
    VOID
    )
{
    KIRQL oldIrql;
    m_DeviceAddressOffset = 0;
    LockTransferState(&oldIrql);
    m_IsCancelled = FALSE;
    UnlockTransferState(oldIrql);
    return STATUS_SUCCESS;
}

VOID
FxDmaPacketTransaction::ReleaseResources(
    __in BOOLEAN ForceRelease
    )
{
    //
    // If the map register base hasn't been assigned, then just
    // skip this.
    //

    if (IsMapRegisterBaseSet() == FALSE) {
        return;
    }

    //
    // Map registers are reserved.  Unless the caller is forcing
    // us to free them, just return.  Otherwise updated the
    // number of map registers that FreeMapRegistersAndAdapter
    // is going to look at.
    //
    if ((m_MapRegistersReserved > 0) && (ForceRelease == FALSE))
    {
        return;
    }

    //
    // Free the map registers and release the device.
    //
    FreeMapRegistersAndAdapter();

    ClearMapRegisterBase();

    ReleaseDevice();

    m_AdapterInfo = NULL;
    m_MapRegistersReserved = 0;
}

_Must_inspect_result_
NTSTATUS
FxDmaPacketTransaction::ReserveAdapter(
    __in     ULONG               NumberOfMapRegisters,
    __in     WDF_DMA_DIRECTION   DmaDirection,
    __in     PFN_WDF_RESERVE_DMA Callback,
    __in_opt PVOID               Context
    )
{
    NTSTATUS status;
    PFX_DRIVER_GLOBALS pFxDriverGlobals = GetDriverGlobals();
    WDFDMATRANSACTION dmaTransaction = GetHandle();

    if (pFxDriverGlobals->FxVerifierOn) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                            "Enter WDFDMATRANSACTION %p", dmaTransaction);
    }

    //
    // If caller doesn't supply a map register count then we get the count
    // out of the transaction.  So the transaction must be initialized.
    //
    // Otherwise the transaction can't be executing.
    //
    if (NumberOfMapRegisters == 0) {
        if (m_State != FxDmaTransactionStateInitialized) {
            status = STATUS_INVALID_PARAMETER;

            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                                "RequiredMapRegisters cannot be 0 because "
                                "WDFDMATRANSACTION %p is not initialized ("
                                "state is %!FxDmaTransactionState!) - %!STATUS!",
                                GetHandle(),
                                m_State,
                                status);
            FxVerifierBugCheck(pFxDriverGlobals,               // globals
                               WDF_DMA_FATAL_ERROR,            // specific type
                               (ULONG_PTR) GetObjectHandle(),  // parm 2
                               (ULONG_PTR) m_State);           // parm 3
        }
    }
    else if (m_State != FxDmaTransactionStateCreated &&
             m_State != FxDmaTransactionStateInitialized &&
             m_State != FxDmaTransactionStateReleased) {

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                    "WDFDMATRANSACTION %p state %!FxDmaTransactionState! "
                    "is invalid", dmaTransaction, m_State);

        FxVerifierBugCheck(pFxDriverGlobals,               // globals
                           WDF_DMA_FATAL_ERROR,            // specific type
                           (ULONG_PTR) GetObjectHandle(),  // parm 2
                           (ULONG_PTR) m_State);           // parm 3
    }

    //
    // Must not already have reserved map registers
    //
    if (m_MapRegistersReserved != 0)
    {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                    "WDFDMATRANSACTION %p already has allocated map registers.",
                    dmaTransaction);

        FxVerifierBugCheck(pFxDriverGlobals,               // globals
                           WDF_DMA_FATAL_ERROR,            // specific type
                           (ULONG_PTR) GetObjectHandle(),  // parm 2
                           (ULONG_PTR) m_State);           // parm 3
    }

    //
    // Get the adapter
    //
    if (DmaDirection == WdfDmaDirectionReadFromDevice) {
        m_AdapterInfo = m_DmaEnabler->GetReadDmaDescription();
    } else {
        m_AdapterInfo = m_DmaEnabler->GetWriteDmaDescription();
    }

    //
    // Save the number of map registers being reserved.
    //
    if (NumberOfMapRegisters != 0) {

        //
        // Use the number the caller passed us
        //
        m_MapRegistersReserved = NumberOfMapRegisters;
    }
    else if (m_DmaEnabler->IsBusMaster() == FALSE) {

        //
        // For system DMA use all the map registers we have
        //
        m_MapRegistersReserved = m_AdapterInfo->NumberOfMapRegisters;

    } else {

        //
        // Compute the number of map registers required based on
        // the MDL and length passed in
        //
        status = _CalculateRequiredMapRegisters(
                    m_StartMdl,
                    m_StartOffset,
                    (ULONG) m_TransactionLength,
                    m_AdapterInfo->NumberOfMapRegisters,
                    NULL,
                    &m_MapRegistersReserved
                    );

        if (!NT_SUCCESS(status)) {
            ReleaseForReuse(TRUE);
            goto End;
        }
    }

    //
    // Initialize the DmaTransaction object with enough data to
    // trick StartTransfer into allocating the adapter channel for us.
    //
    m_DmaDirection       = DmaDirection;
    m_StartMdl           = NULL;
    m_StartOffset        = 0;
    m_CurrentFragmentMdl = NULL;
    m_CurrentFragmentOffset = 0;
    m_Remaining          = 0;
    m_TransactionLength  = 0;

    //
    // Save the callback and context
    //
    m_DmaAcquiredFunction.Method.ReserveDma = Callback;
    m_DmaAcquiredContext = Context;

    //
    // If needed, initialize the transfer context.
    //
    if (m_DmaEnabler->UsesDmaV3()) {
        m_DmaEnabler->InitializeTransferContext(GetTransferContext(),
                                                m_DmaDirection);
    }

    status = InitializeResources();
    if (NT_SUCCESS(status)) {
        //
        // Set the state to reserved so _AdapterControl knows which
        // callback to invoke.
        //
        m_State = FxDmaTransactionStateReserved;
    } else {
        ReleaseForReuse(TRUE);
        goto End;
    }

    //
    // Start the adapter channel allocation through StartTransfer
    //
    status = StartTransfer();

End:
    if (!NT_SUCCESS(status)) {
        m_State = FxDmaTransactionStateTransferFailed;
        m_DmaAcquiredFunction.Method.ReserveDma = NULL;
        m_DmaAcquiredContext = NULL;
        m_MapRegistersReserved = 0;

        if (m_EncodedRequest != NULL) {
            ReleaseButRetainRequest();
        }
    }

    if (pFxDriverGlobals->FxVerifierOn) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                            "Exit WDFDMATRANSACTION %p, %!STATUS!",
                            dmaTransaction, status);
    }

    return status;
}

VOID
FxDmaPacketTransaction::ReleaseAdapter(
    VOID
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals = GetDriverGlobals();
    WDFDMATRANSACTION dmaTransaction = GetHandle();

    if (pFxDriverGlobals->FxVerifierOn) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                            "Enter WDFDMATRANSACTION %p", dmaTransaction);
    }

    //
    // Must not be in invalid, created, transfer or deleted state
    //
    if (m_State == FxDmaTransactionStateInvalid ||
        m_State == FxDmaTransactionStateCreated ||
        m_State == FxDmaTransactionStateTransfer ||
        m_State == FxDmaTransactionStateDeleted) {

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                    "WDFDMATRANSACTION %p state %!FxDmaTransactionState! "
                    "is invalid", dmaTransaction, m_State);

        FxVerifierBugCheck(pFxDriverGlobals,               // globals
                           WDF_DMA_FATAL_ERROR,            // specific type
                           (ULONG_PTR) GetObjectHandle(),  // parm 2
                           (ULONG_PTR) m_State);           // parm 3
    }

    //
    // The caller wants to free the reserved map registers, so force their
    // release.
    //
    ReleaseForReuse(TRUE);

    if (pFxDriverGlobals->FxVerifierOn) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                            "Exit WDFDMATRANSACTION %p",
                            dmaTransaction);
    }

    return;
}

_Must_inspect_result_
NTSTATUS
FxDmaPacketTransaction::StartTransfer(
    VOID
    )
{
    NTSTATUS           status;
    PFX_DRIVER_GLOBALS pFxDriverGlobals = GetDriverGlobals();
    WDFDMATRANSACTION  dmaTransaction;

    //
    // Client driver could complete and delete the object in
    // EvtProgramDmaFunction. So, save the handle because we need it
    // for tracing after we invoke the callback.
    //
    dmaTransaction = GetHandle();

    if (pFxDriverGlobals->FxVerifierOn) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                            "Enter WDFDMATRANSACTION %p",
                            dmaTransaction);

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                            "Starting WDFDMATRANSACTION %p - MDL %#p, "
                            "offset %I64x, length %I64x",
                            dmaTransaction,
                            m_StartMdl,
                            m_StartOffset,
                            m_TransactionLength);
    }

    //
    // Reference the device when using DMA v2.  For DMA v3 we can support
    // concurrent attempts to allocate the channel.
    //
    status = AcquireDevice();
    if (!NT_SUCCESS(status)) {

        NT_ASSERTMSG("AcquireDevice should never fail when DMAv3 is in use",
                     m_DmaEnabler->UsesDmaV3());

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "Only one transaction can be queued "
                            "at one time on a packet based WDFDMAENABLER %p "
                            "%!STATUS!", m_DmaEnabler->GetHandle(),
                            status);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return status;
    }

    //
    // Calculate the initial DMA transfer length.
    //
    m_CurrentFragmentLength = FxSizeTMin(m_Remaining, m_MaxFragmentLength);

    m_CurrentFragmentOffset = m_StartOffset;

    if (m_State == FxDmaTransactionStateReserved) {
        //
        // Caller is simply reserving the DMA adapter for later use.  Ask for
        // as many map registers as the driver requested.
        //
        m_MapRegistersNeeded = m_MapRegistersReserved;

        ASSERT(m_MapRegistersNeeded <= m_AdapterInfo->NumberOfMapRegisters);

        status = AllocateAdapterChannel(FALSE);

    }
    else {

        if (m_DmaEnabler->IsBusMaster() == FALSE) {

            //
            // Use as many map registers as we were granted.
            //
            m_MapRegistersNeeded = m_AdapterInfo->NumberOfMapRegisters;
        } else {

            //
            // If the transfer is the size of the transaction then use the offset
            // to determine the number of map registers needed.  If it's smaller
            // then use the worst-case offset to make sure we ask for enough MR's
            // to account for a bigger offset in one of the later transfers.
            //
            // Example:
            //  Transaction is 8 KB and is page aligned
            //      if max transfer is >= 8KB then this will be one transfer and only
            //      requires two map registers.  Even if the driver completes a partial
            //      transfer and we have to do the rest in a second transfer it will
            //      fit within two map registers becuase the overall transaction does
            //      (and a partial transfer can't take more map registers than the
            //       whole transaction would).
            //
            //      If max transfer is 2KB then this nominally requires 4 2KB transfers.
            //      In this case however, a partial completion of one of those transfers
            //      would leave us attempting a second 2KB transfer starting on an
            //      unaligned address.  For example, we might transfer 2KB, then 1KB
            //      then 2KB.  Even though the first transfer was page aligned, the
            //      3rd transfer isn't and could cross a page boundary, requiring two
            //      map registers rather than one.
            //
            // To account for this second case, ignore the actual MDL offset and
            // instead compute the maximum number of map registers than an N byte
            // transfer could take (with worst-case alignment).
            //
            //
            m_MapRegistersNeeded =
                (ULONG) ADDRESS_AND_SIZE_TO_SPAN_PAGES(
                    ((m_CurrentFragmentLength == m_Remaining) ?
                        GetStartVaFromOffset(m_CurrentFragmentMdl,
                                             m_CurrentFragmentOffset) :
                        (PVOID)(ULONG_PTR) (PAGE_SIZE -1)),
                    m_CurrentFragmentLength
                    );


            ASSERT(m_MapRegistersNeeded <= m_AdapterInfo->NumberOfMapRegisters);
        }

        //
        // NOTE: the number of map registers needed for this transfer may
        //       exceed the number that we've reserved.  StageTransfer will
        //       take care of fragmenting the transaction accordingly.
        //
        status = AllocateAdapterChannel(m_MapRegistersReserved > 0);
    }

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "AllocateAdapterChannel failed for "
                            "WDFDMATRANSACTION %p, %!STATUS!",
                            dmaTransaction, status);
        ReleaseDevice();
    }

    //
    // Before AllocateAdapterChannel returns, _AdapterControl can get called
    // on another thread and the driver could delete the transaction object.
    // So don't touch the object after this point.
    //
    if (pFxDriverGlobals->FxVerifierOn) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                        "Exit WDFDMATRANSACTION %p, "
                        "%!STATUS!", dmaTransaction, status);
    }

    return status;
}

IO_ALLOCATION_ACTION
FxDmaPacketTransaction::_AdapterControl(
    __in PDEVICE_OBJECT  DeviceObject,
    __in PIRP            Irp,
    __in PVOID           MapRegisterBase,
    __in PVOID           Context
    )
{
    FxDmaPacketTransaction * pDmaTransaction;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    IO_ALLOCATION_ACTION action;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(DeviceObject);

    pDmaTransaction = (FxDmaPacketTransaction*) Context;
    ASSERT(pDmaTransaction);

    pFxDriverGlobals = pDmaTransaction->GetDriverGlobals();

    //
    // Cache the return value while we can still touch the transaction
    //
    action = pDmaTransaction->GetAdapterControlReturnValue();

    //
    // Save the MapRegister base, unless it was previously set
    // during a reserve.
    //
    if (pDmaTransaction->IsMapRegisterBaseSet() == FALSE) {
        pDmaTransaction->SetMapRegisterBase(MapRegisterBase);
    }
    else {
        NT_ASSERTMSG("Caller was expected to use existing map register base",
                     MapRegisterBase == pDmaTransaction->m_MapRegisterBase);
    }

    if (pFxDriverGlobals->FxVerifierOn) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                            "Map registers for WDFDMATRANSACTION %p allocated "
                            "(base %p)",
                            pDmaTransaction->GetHandle(),
                            MapRegisterBase);
    }

    //
    // NOTE: KMDF used to call KeFlushIoBuffers() here to "ensure the
    //       data buffers were flushed."  However KeFlushIoBuffers did
    //       nothing on x86 & amd64 (which are cache coherent WRT DMA)
    //       and calling FlushAdapterBuffers() does any necessary
    //       flushing anyway.  Plus on non-cache-coherent architectures
    //       (such as ARM) the flush operation has to be cache-line aligned
    //       to avoid cache line tearing.  So the flush is not necessary
    //       and has been removed.

    //
    // Check the state of the transaction.  If it's reserve then call the
    // reserve callback and return.  Otherwise stage the first fragment.
    //
    if (pDmaTransaction->m_State == FxDmaTransactionStateReserved)
    {
        FxDmaTransactionProgramOrReserveDma callback;

        //
        // Save off and clear the callback before calling it.
        //
        callback = pDmaTransaction->m_DmaAcquiredFunction;
        pDmaTransaction->m_DmaAcquiredFunction.Clear();

        ASSERTMSG("Mismatch between map register counts",
                  (pDmaTransaction->m_MapRegistersReserved ==
                   pDmaTransaction->m_MapRegistersNeeded));

        //
        // Invoke the callback.  Note that from here the driver may initialize
        // and execute the transaction.
        //
        callback.InvokeReserveDma(
            pDmaTransaction->GetHandle(),
            pDmaTransaction->m_DmaAcquiredContext
            );
    }
    else {

        //
        // Stage next fragment
        //
        status = pDmaTransaction->StageTransfer();

        if (!NT_SUCCESS(status)) {

            DMA_COMPLETION_STATUS dmaStatus =
                (status == STATUS_CANCELLED ? DmaCancelled : DmaError);
            FxDmaSystemTransaction* systemTransaction = (FxDmaSystemTransaction*) pDmaTransaction;

            //
            // Map transfer failed.  There will be no DMA completion callback
            // and no call to EvtProgramDma.  And we have no way to hand this
            // status back directly to the driver.  Fake a DMA completion with
            // the appropriate status.
            //
            // This should only happen for system DMA (and there most likely
            // only during cancelation, though we leave the possibility that
            // the DMA extension may fail the transfer)
            //
            ASSERTMSG("Unexpected failure of StageTransfer for packet based "
                      "DMA",
                       (pDmaTransaction->GetDmaEnabler()->IsBusMaster() == false));

            if (pFxDriverGlobals->FxVerifierOn) {
                DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                                    "Invoking DmaCompleted callback %p (context %p) "
                                    "for WDFDMATRANSACTION %p (status %x) "
                                    "due to staging failure (%!STATUS!)",
                                    systemTransaction->m_TransferCompleteFunction.Method,
                                    systemTransaction->m_TransferCompleteContext,
                                    pDmaTransaction->GetHandle(),
                                    dmaStatus,
                                    status);
            }

            pDmaTransaction->CallEvtDmaCompleted(
                status == STATUS_CANCELLED ? DmaCancelled : DmaError
                );
        }
    }

    //
    // Indicate that MapRegs are to be kept
    //
    return action;
}

_Must_inspect_result_
NTSTATUS
FxDmaPacketTransaction::StageTransfer(
    VOID
    )
{
    PSCATTER_GATHER_LIST sgList;

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    UCHAR_MEMORY_ALIGNED sgListBuffer[sizeof(SCATTER_GATHER_LIST)
                            + sizeof(SCATTER_GATHER_ELEMENT)];
    WDFDMATRANSACTION dmaTransaction;

    KIRQL oldIrql;
    BOOLEAN stagingNeeded;

    NTSTATUS status = STATUS_SUCCESS;

    //
    // Client driver could complete and delete the object in
    // EvtProgramDmaFunction. So, save the handle because we need it
    // for tracing after we invoke the callback.
    //
    pFxDriverGlobals = GetDriverGlobals();
    dmaTransaction = GetHandle();

    if (pFxDriverGlobals->FxVerifierOn) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                        "Enter WDFDMATRANSACTION %p ", dmaTransaction);
    }

    //
    // For packet base DMA, current and startMDL will always be
    // same.  For V2 DMA we don't support MDL chains.  For V3 DMA
    // we use the HAL's support for MDL chains and don't walk through
    // the MDL chain on our own.
    //
    ASSERT(m_CurrentFragmentMdl == m_StartMdl);

    LockTransferState(&oldIrql);

    if (m_TransferState.CurrentStagingThread != NULL) {

        //
        // Staging in progress.  Indicate that another staging will
        // be needed.
        //
        m_TransferState.RerunStaging = TRUE;

        stagingNeeded = FALSE;

        if (pFxDriverGlobals->FxVerifierOn) {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                "Staging next fragment of WDFDMATRANSACTION %p "
                "deferred",
                dmaTransaction
                );
        }
    }
    else {
        //
        // Staging isn't in progress anyplace else.  Indicate that it's
        // running now so that any parallel attempt is blocked.
        //
        m_TransferState.CurrentStagingThread = KeGetCurrentThread();

        ASSERTMSG("The thread which was staging didn't clear "
                  "RerunStaging",
                  (m_TransferState.RerunStaging == FALSE));

        stagingNeeded = TRUE;
    }

    UnlockTransferState(oldIrql);

    //
    // Take a reference on the transaction so that we can safely
    // manipulate the transfer state even after it's destroyed.
    //
    AddRef(&pFxDriverGlobals);

    //
    // Loop for as long as staging is required
    //
    while (stagingNeeded) {

        //
        // Calculate length for this packet.
        //
        m_CurrentFragmentLength = FxSizeTMin(m_Remaining, m_MaxFragmentLength);

        //
        // Calculate address for this packet.
        //
        m_CurrentFragmentOffset = m_StartOffset + m_Transferred;

        //
        // Adjust the fragment length for the number of reserved map registers.
        //
        if ((m_MapRegistersReserved > 0) &&
            (m_MapRegistersNeeded > m_MapRegistersReserved))
        {
            size_t currentOffset = m_CurrentFragmentOffset;
            size_t currentPageOffset;
            PMDL mdl;

            for (mdl = m_CurrentFragmentMdl; mdl != NULL; mdl = mdl->Next)
            {
                //
                // For packet/system transfers of chained MDLs, m_CurrentFragmentMdl
                // is never adjusted, and m_CurrentFragmentOFfset is the offset
                // into the entire chain.
                //
                // Locate the MDL which contains the current fragment.
                //
                ULONG mdlBytes = MmGetMdlByteCount(mdl);
                if (mdlBytes >= currentOffset)
                {
                    //
                    // This MDL is larger than the remaining offset, so it
                    // contains the start address.
                    //
                    break;
                }

                currentOffset -= mdlBytes;
            }

            ASSERT(mdl != NULL);

            //
            // Compute page offset from current MDL's initial page offset
            // and the offset into that MDL
            //

            currentPageOffset = BYTE_OFFSET(MmGetMdlByteOffset(mdl) +
                                            currentOffset);

            //
            // Compute the maximum number of bytes we can transfer with
            // the number of map registers we have reserved, taking into
            // account the offset of the first page.
            //
            size_t l =  ((PAGE_SIZE * (m_MapRegistersReserved - 1)) +
                         (PAGE_SIZE - currentPageOffset));

            m_CurrentFragmentLength = FxSizeTMin(m_CurrentFragmentLength, l);
        }

        m_Remaining -= m_CurrentFragmentLength;

        if (pFxDriverGlobals->FxVerifierOn) {
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                                "Initiating %s transfer for WDFDMATRANSACTION %p.  "
                                "Mdl %p, Offset %I64x, Length %I64x",
                                m_Transferred == 0 ? "first" : "next",
                                GetHandle(),
                                m_CurrentFragmentMdl,
                                m_Transferred,
                                m_CurrentFragmentLength);
        }

        //
        // Check for a pending cancellation.  This can happen if the cancel
        // occurred between DMA completion and FlushAdapterBuffers -
        // FlushAdapterBuffers will clear the canceled bit in the transfer
        // context (TC), which would allow MapTransfer to succeed.
        //
        // An unprotected check of IsCancelled here is safe.  A concurrent
        // cancel at this point would mark the TC cancelled such that
        // MapTransfer will fail.
        //
        if (m_IsCancelled == TRUE) {
            status = STATUS_CANCELLED;
            goto End;
        }

        //
        // Profile specific work before mapping the transfer.  if this
        // fails consider 'this' invalid.
        //
        if (PreMapTransfer() == FALSE) {
            status = STATUS_SUCCESS;
            goto End;
        }

        //
        // Map this packet for transfer.
        //

        //
        // For packet based DMA we use a single entry on-stack SGL.  This
        // allows us to map multiple packet-based requests concurrently and
        // we know packet base DMA only requires a single SGL
        //
        // NOTE: It turns out the HAL doesn't handle chained MDLs in packet
        //       mode correctly.  It makes each MDL continguous, but returns
        //       a list of SG elements - one for each MDL.  That's scatter
        //       gather DMA, not packet DMA.
        //
        //       So it's actually very important in Win8 that we only use a
        //       single entry SGL when calling MapTransferEx.  This ensures
        //       we only map the first MDL in the chain and thus get a
        //       contiguous buffer
        //
        // For system DMA we use the SystemSGList stored in the DMA enabler
        // We can use a shared one in that case because we won't be mapping
        // multiple system DMA requests concurrently (HAL doesn't allow it)
        //
        FxDmaEnabler* enabler = GetDmaEnabler();
        size_t sgListSize;

        if (enabler->IsBusMaster()) {
            sgList = (PSCATTER_GATHER_LIST)sgListBuffer;
            sgListSize = sizeof(sgListBuffer);
        } else {
            sgList = enabler->m_SGList.SystemProfile.List;
            sgListSize = enabler->m_SGListSize;
        }

        ULONG mappedBytes;

        status = MapTransfer(sgList,
                             (ULONG) sgListSize,
                             GetTransferCompletionRoutine(),
                             this,
                             &mappedBytes);

        NT_ASSERTMSG("Unexpected failure of MapTransfer",
                     ((NT_SUCCESS(status) == TRUE) ||
                      (status == STATUS_CANCELLED)));

        if (NT_SUCCESS(status)) {

            NT_ASSERTMSG("unexpected number of mapped bytes",
                         ((mappedBytes > 0) &&
                          (mappedBytes <= m_CurrentFragmentLength)));

            //
            // Adjust the remaining byte count if the HAL mapped less data than we
            // requested.
            //
            if (mappedBytes < m_CurrentFragmentLength) {
                m_Remaining += m_CurrentFragmentLength - mappedBytes;
                m_CurrentFragmentLength = mappedBytes;
            }

            //
            // Do client PFN_WDF_PROGRAM_DMA callback.
            //
            if (m_DmaAcquiredFunction.Method.ProgramDma != NULL) {

                if (pFxDriverGlobals->FxVerifierOn) {
                    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                                        "Invoking ProgramDma callback %p (context %p) for "
                                        "WDFDMATRANSACTION %p.",
                                        m_DmaAcquiredFunction.Method.ProgramDma,
                                        m_DmaAcquiredContext,
                                        GetHandle()
                                        );
                }

                //
                // Call program DMA
                //
                (VOID) m_DmaAcquiredFunction.InvokeProgramDma(
                                                GetHandle(),
                                                m_DmaEnabler->m_DeviceBase->GetHandle(),
                                                m_DmaAcquiredContext,
                                                m_DmaDirection,
                                                sgList
                                                );
            }
        }

End:
        //
        // Process any pending completion or nested staging.
        //
        {
            LockTransferState(&oldIrql);

            //
            // While staging we could either have deferred a call to the
            // completion routine or deferred another call to stage the
            // next fragment.  We should not ever have to do both - this
            // would imply that the driver didn't wait for its DMA completion
            // routine to run when calling TransferComplete*.
            //
            ASSERTMSG("driver called TransferComplete with pending DMA "
                      "completion callback",
                      !((m_TransferState.RerunCompletion == TRUE) &&
                        (m_TransferState.RerunStaging == TRUE)));

            //
            // Check for pending completion.  save the status away and clear it
            // before dropping the lock.
            //
            if (m_TransferState.RerunCompletion == TRUE) {
                DMA_COMPLETION_STATUS completionStatus;
                FxDmaSystemTransaction* systemTransaction = (FxDmaSystemTransaction*) this;

                //
                // Save the completion status for when we drop the lock.
                //
                completionStatus = m_TransferState.CompletionStatus;

                ASSERTMSG("completion needed, but status was not set or was "
                          "already cleared",
                          completionStatus != UNDEFINED_DMA_COMPLETION_STATUS);

                ASSERTMSG("completion needed, but mapping failed so there shouldn't "
                          "be any parallel work going on",
                          NT_SUCCESS(status));

                //
                // Clear the completion needed state.
                //
                m_TransferState.RerunCompletion = FALSE;
                m_TransferState.CompletionStatus = UNDEFINED_DMA_COMPLETION_STATUS;

                //
                // Drop the lock, call the completion routine, then take the
                // lock again.
                //
                UnlockTransferState(oldIrql);
                if (pFxDriverGlobals->FxVerifierOn) {
                    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                                        "Invoking DmaCompleted callback %p (context %p) "
                                        "for WDFDMATRANSACTION %p (status %x) "
                                        "after deferral",
                                        systemTransaction->m_TransferCompleteFunction.Method,
                                        systemTransaction->m_TransferCompleteContext,
                                        GetHandle(),
                                        completionStatus);
                }
                CallEvtDmaCompleted(completionStatus);
                LockTransferState(&oldIrql);

                //
                // Staging is blocked, which means we aren't starting up the
                // next transfer.  Therefore we cannot have a queued completion.
                //
                ASSERTMSG("RerunCompletion should not be set on an unstaged "
                          "transaction",
                          m_TransferState.RerunCompletion == FALSE);
            }

            //
            // Capture whether another staging is needed.  If none is needed
            // then we can clear staging in progress.
            //
            if (m_TransferState.RerunStaging == TRUE) {
                stagingNeeded = TRUE;
                m_TransferState.RerunStaging = FALSE;
            }
            else {
                m_TransferState.CurrentStagingThread = NULL;
                stagingNeeded = FALSE;
            }

            UnlockTransferState(oldIrql);
        }

#if DBG
        if (!NT_SUCCESS(status)) {
            ASSERTMSG("MapTransfer returned an error - there should not be any "
                      "deferred work.",
                      (stagingNeeded == FALSE));
        }
#endif

    } // while(stagingNeeded)

    Release(&pFxDriverGlobals);

    if (pFxDriverGlobals->FxVerifierOn) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                            "Exit WDFDMATRANSACTION %p, "
                            "%!STATUS!", dmaTransaction,
                            status);
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxDmaPacketTransaction::TransferCompleted(
    VOID
    )
{
    NTSTATUS status;
    PFX_DRIVER_GLOBALS pFxDriverGlobals = GetDriverGlobals();

    //
    // Flush the buffers
    //
    status = FlushAdapterBuffers();

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "FlushAdapterBuffers on WDFDMATRANSACTION %p "
                            "failed, %!STATUS!",
                            GetHandle(), status);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
    }

    return status;
}

// ----------------------------------------------------------------------------
// ------------------- SYSTEM DMA SECTION -------------------------------------
// ----------------------------------------------------------------------------

FxDmaSystemTransaction::FxDmaSystemTransaction(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in USHORT ExtraSize,
    __in FxDmaEnabler *DmaEnabler
    ) :
    FxDmaPacketTransaction(FxDriverGlobals, sizeof(FxDmaSystemTransaction), ExtraSize, DmaEnabler)
{
    return;
}

_Must_inspect_result_
NTSTATUS
FxDmaSystemTransaction::_Create(
    __in  PFX_DRIVER_GLOBALS      FxDriverGlobals,
    __in  PWDF_OBJECT_ATTRIBUTES  Attributes,
    __in  FxDmaEnabler*           DmaEnabler,
    __out WDFDMATRANSACTION*      Transaction
    )
{
    FxDmaPacketTransaction* pTransaction;
    WDFOBJECT hTransaction;
    NTSTATUS status;

    pTransaction = new (FxDriverGlobals, Attributes, DmaEnabler->GetTransferContextSize())
                FxDmaSystemTransaction(FxDriverGlobals, DmaEnabler->GetTransferContextSize(), DmaEnabler);

    if (pTransaction == NULL) {
        status =  STATUS_INSUFFICIENT_RESOURCES;
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
            "Could not allocate memory for WDFTRANSACTION, %!STATUS!", status);
        return status;
    }

    //
    // Commit and apply the attributes
    //
    status = pTransaction->Commit(Attributes, &hTransaction, DmaEnabler);
    if (NT_SUCCESS(status)) {
        *Transaction = (WDFDMATRANSACTION)hTransaction;
    }
    else {
        //
        // This will properly clean up the target's state and free it
        //
        pTransaction->DeleteFromFailedCreate();
    }

    return status;
}

BOOLEAN
FxDmaSystemTransaction::PreMapTransfer(
    VOID
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals = GetDriverGlobals();
    BOOLEAN result = TRUE;

    if (m_ConfigureChannelFunction.Method != NULL) {
        //
        // Invoke the callback.  If it returns false then the driver has
        // completed the transaction in the callback and we must abort
        // processing.
        //

        if (pFxDriverGlobals->FxVerifierOn) {
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                                "Invoking ConfigureChannel callback %p (context "
                                "%p) for WDFDMATRANSACTION %p.",
                                m_ConfigureChannelFunction.Method,
                                m_ConfigureChannelContext,
                                GetHandle());
        }

        result = m_ConfigureChannelFunction.Invoke(
                    GetHandle(),
                    m_DmaEnabler->m_DeviceBase->GetHandle(),
                    m_ConfigureChannelContext,
                    m_CurrentFragmentMdl,
                    m_CurrentFragmentOffset,
                    m_CurrentFragmentLength
                    );
    }

    return result;
}


PDMA_COMPLETION_ROUTINE
FxDmaSystemTransaction::GetTransferCompletionRoutine(
    VOID
    )
{
    if (m_TransferCompleteFunction.Method == NULL) {
        return NULL;
    }
    else {
        return _SystemDmaCompletion;
    }
}

VOID
FxDmaSystemTransaction::CallEvtDmaCompleted(
    __in DMA_COMPLETION_STATUS Status
    )
{
    //
    // Call the TransferComplete callback to indicate that the
    // transfer was aborted.
    //
    m_TransferCompleteFunction.Invoke(
        GetHandle(),
        m_DmaEnabler->m_Device->GetHandle(),
        m_TransferCompleteContext,
        m_DmaDirection,
        Status
        );
}

VOID
FxDmaTransactionBase::GetTransferInfo(
    __out_opt ULONG *MapRegisterCount,
    __out_opt ULONG *ScatterGatherElementCount
    )
{
    if (m_State != FxDmaTransactionStateInitialized) {
        PFX_DRIVER_GLOBALS pFxDriverGlobals = GetDriverGlobals();

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                    "WDFDMATRANSACTION %p state %!FxDmaTransactionState! "
                    "is invalid", GetHandle(), m_State);

        FxVerifierBugCheck(pFxDriverGlobals,               // globals
                           WDF_DMA_FATAL_ERROR,            // specific type
                           (ULONG_PTR) GetObjectHandle(),  // parm 2
                           (ULONG_PTR) m_State);           // parm 3
    }

    DMA_TRANSFER_INFO info = {0};






    if (m_DmaEnabler->UsesDmaV3()) {

        //
        // Ask the HAL for information about the MDL and how many resources
        // it will require to transfer.
        //
        m_AdapterInfo->AdapterObject->DmaOperations->GetDmaTransferInfo(
            m_AdapterInfo->AdapterObject,
            m_StartMdl,
            m_StartOffset,
            (ULONG) this->m_TransactionLength,
            this->m_DmaDirection == WDF_DMA_DIRECTION::WdfDmaDirectionWriteToDevice,
            &info
            );
    } else {
        size_t offset = m_StartOffset;
        size_t length = m_TransactionLength;

        //
        // Walk through the MDL chain and make a worst-case computation of
        // the number of scatter gather entries and map registers the
        // transaction would require.
        //
        for(PMDL mdl = m_StartMdl;
            mdl != NULL && length != 0;
            mdl = mdl->Next) {

            size_t byteCount = MmGetMdlByteCount(mdl);
            if (byteCount <= offset) {
                offset -= byteCount;
            } else {
                ULONG_PTR startVa = (ULONG_PTR) MmGetMdlVirtualAddress(mdl);

                startVa += offset;
                byteCount -= offset;

                info.V1.MapRegisterCount +=
                    (ULONG) ADDRESS_AND_SIZE_TO_SPAN_PAGES(
                        startVa,
                        min(byteCount, length)
                        );

                length -= min(byteCount, length);
            }
        }

        info.V1.ScatterGatherElementCount = info.V1.MapRegisterCount;
    }

    if (ARGUMENT_PRESENT(MapRegisterCount)) {
        *MapRegisterCount = info.V1.MapRegisterCount;
    }

    if (ARGUMENT_PRESENT(ScatterGatherElementCount)) {
        *ScatterGatherElementCount = info.V1.ScatterGatherElementCount;
    }

    return;
}

VOID
FxDmaSystemTransaction::_SystemDmaCompletion(
    __in PDMA_ADAPTER          /* DmaAdapter */,
    __in PDEVICE_OBJECT        /* DeviceObject */,
    __in PVOID                 CompletionContext,
    __in DMA_COMPLETION_STATUS Status
    )
{
    FxDmaSystemTransaction* transaction = (FxDmaSystemTransaction*) CompletionContext;
    PFX_DRIVER_GLOBALS pFxDriverGlobals = transaction->GetDriverGlobals();
    KIRQL oldIrql;
    BOOLEAN completionDeferred;

    //
    // Lock the transfer state so that a staging or cancelling thread
    // cannot change it.
    //
    transaction->LockTransferState(&oldIrql);

    ASSERTMSG("Completion state was already set",
              (transaction->m_TransferState.CompletionStatus ==
               UNDEFINED_DMA_COMPLETION_STATUS));
    ASSERTMSG("Deferred completion is already pending",
              (transaction->m_TransferState.RerunCompletion == FALSE));

    //
    // If a staging is in progress then defer the completion.
    //
    if (transaction->m_TransferState.CurrentStagingThread != NULL) {
        transaction->m_TransferState.CompletionStatus = Status;
        transaction->m_TransferState.RerunCompletion = TRUE;
        completionDeferred = TRUE;
    }
    else {
        completionDeferred = FALSE;
    }

    transaction->UnlockTransferState(oldIrql);

    //
    // Process the old state.
    //
    if (completionDeferred == TRUE) {
        //
        // The staging thread has not moved past EvtProgramDma.  The staging thread
        // will detect the state change and call the completion routine.
        //
        // Nothing to do in this case.
        //
        if (pFxDriverGlobals->FxVerifierOn) {
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                                "Deferring DmaCompleted callback for WDFDMATRANSACTION %p"
                                "(status %x)",
                                transaction->GetHandle(),
                                Status);
        }
    }
    else {
        //
        // Completion occurred while the transfer was running or
        // being cancelled.  Call the completion routine.
        //
        // Note: a cancel when in programming state leaves the
        //       state as programming.  that we're not in programming
        //       means we don't need to worry about racing with
        //       EvtProgramDma.
        //

        if (pFxDriverGlobals->FxVerifierOn) {
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDMA,
                                "Invoking DmaCompleted callback %p (context %p) "
                                "for WDFDMATRANSACTION %p (status %x)",
                                transaction->m_TransferCompleteFunction.Method,
                                transaction->m_TransferCompleteContext,
                                transaction->GetHandle(),
                                Status);
        }
        transaction->CallEvtDmaCompleted(Status);
    }
}

VOID
FxDmaSystemTransaction::StopTransfer(
    VOID
    )
{
    //
    // Mark the transfer cancelled so we have a record of it even if
    // a racing call to FlushAdapterBuffers clears the TC.
    //
    m_IsCancelled = TRUE;

    //
    // Cancel the system DMA transfer.  This arranges for one of two things
    // to happen:
    //  * the next call to MapTransfer will fail
    //  * the DMA completion routine will run
    //
    if (CancelMappedTransfer() == FALSE) {

        //
        // The cancel failed.  Someone has already stopped this transfer.
        // That's illegal.
        //
        PFX_DRIVER_GLOBALS pFxDriverGlobals = GetDriverGlobals();

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "WDFDMATRANSACTION %p has already been stopped",
                            GetHandle());

        if (pFxDriverGlobals->IsVerificationEnabled(1, 11, OkForDownLevel)) {
            FxVerifierBugCheck(pFxDriverGlobals,               // globals
                               WDF_DMA_FATAL_ERROR,            // type
                               (ULONG_PTR) GetObjectHandle(),  // parm 2
                               (ULONG_PTR) m_State);           // parm 3
        }
    }
}
