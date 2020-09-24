/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDmaTransactionAPI.cpp

Abstract:

    Base for WDF DMA Transaction APIs

Environment:

    Kernel mode only.

Notes:


Revision History:

--*/

#include "FxDmaPCH.hpp"

extern "C" {
#include "FxDmaTransactionAPI.tmh"
}

//
// Extern "C" the entire file
//
extern "C" {

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfDmaTransactionCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMAENABLER DmaEnabler,
    __in_opt
    WDF_OBJECT_ATTRIBUTES * Attributes,
    __out
    WDFDMATRANSACTION * DmaTransactionHandle
    )
{
    NTSTATUS status;
    FxDmaEnabler* pDmaEnabler;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DmaEnabler,
                                   FX_TYPE_DMA_ENABLER,
                                   (PVOID *) &pDmaEnabler,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, DmaTransactionHandle);

    *DmaTransactionHandle = NULL;

    status = FxValidateObjectAttributes(pFxDriverGlobals,
                                        Attributes,
                                        FX_VALIDATE_OPTION_PARENT_NOT_ALLOWED
                                        );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    switch (pDmaEnabler->GetProfile())
    {
        case WdfDmaProfilePacket:
        case WdfDmaProfilePacket64:
            status = FxDmaPacketTransaction::_Create(pFxDriverGlobals,
                                                     Attributes,
                                                     pDmaEnabler,
                                                     DmaTransactionHandle);
            break;

        case WdfDmaProfileScatterGather:
        case WdfDmaProfileScatterGather64:
        case WdfDmaProfileScatterGatherDuplex:
        case WdfDmaProfileScatterGather64Duplex:
            status = FxDmaScatterGatherTransaction::_Create(
                                                        pFxDriverGlobals,
                                                        Attributes,
                                                        pDmaEnabler,
                                                        DmaTransactionHandle
                                                        );
            break;

        case WdfDmaProfileSystem:
        case WdfDmaProfileSystemDuplex:
            status = FxDmaSystemTransaction::_Create(pFxDriverGlobals,
                                                     Attributes,
                                                     pDmaEnabler,
                                                     DmaTransactionHandle);
            break;

        default:
            NT_ASSERTMSG("Unknown profile for DMA enabler", FALSE);
            status = STATUS_UNSUCCESSFUL;
            break;
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfDmaTransactionInitializeUsingRequest)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction,
    __in
    WDFREQUEST Request,
    __in
    PFN_WDF_PROGRAM_DMA EvtProgramDmaFunction,
    __in
    WDF_DMA_DIRECTION DmaDirection
    )
{
    NTSTATUS status;
    FxDmaTransactionBase* pDmaTrans;
    FxRequest* pReqObj;
    MDL* mdl = NULL;
    PIO_STACK_LOCATION stack;
    ULONG reqLength;
    PFX_DRIVER_GLOBALS  pFxDriverGlobals;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DmaTransaction,
                                   FX_TYPE_DMA_TRANSACTION,
                                   (PVOID *) &pDmaTrans,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, EvtProgramDmaFunction);

    if (DmaDirection != WdfDmaDirectionReadFromDevice &&
        DmaDirection != WdfDmaDirectionWriteToDevice) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
            "Initialization of WDFDMATRANSACTION 0x%p using WDFREQUEST %p, "
            "DmaDirection 0x%x is an invalid value, %!STATUS!",
            DmaTransaction, Request, DmaDirection, status);
        return status;
    }

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         Request,
                         FX_TYPE_REQUEST,
                         (PVOID *) &pReqObj);

    reqLength = 0;

    stack = pReqObj->GetFxIrp()->GetCurrentIrpStackLocation();

    //
    // Get the MDL and Length from the request.
    //
    switch (stack->MajorFunction) {

    case IRP_MJ_READ:

        if (DmaDirection != WdfDmaDirectionReadFromDevice) {
            status = STATUS_INVALID_DEVICE_REQUEST;

            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                                "Dma direction %!WDF_DMA_DIRECTION! of WDFTRANSACTION "
                                "0x%p doesn't match with the WDFREQUEST 0x%p type "
                                "%!WDF_REQUEST_TYPE! %!STATUS!",
                                DmaDirection, DmaTransaction, Request,
                                stack->MajorFunction, status);

            return status;
        }

        reqLength = stack->Parameters.Read.Length;

        status = pReqObj->GetMdl(&mdl);
        break;

    case IRP_MJ_WRITE:

        if (DmaDirection != WdfDmaDirectionWriteToDevice) {
            status = STATUS_INVALID_DEVICE_REQUEST;

            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                                "Dma direction %!WDF_DMA_DIRECTION! of WDFTRANSACTION "
                                "0x%p doesn't match with the WDFREQUEST 0x%p type "
                                "%!WDF_REQUEST_TYPE! %!STATUS!",
                                DmaDirection, DmaTransaction, Request,
                                stack->MajorFunction, status);

            return status;
        }

        reqLength = stack->Parameters.Write.Length;

        status = pReqObj->GetMdl(&mdl);
        break;

    case IRP_MJ_DEVICE_CONTROL:
    case IRP_MJ_INTERNAL_DEVICE_CONTROL:

        switch (METHOD_FROM_CTL_CODE(stack->Parameters.DeviceIoControl.IoControlCode)) {
            case METHOD_BUFFERED:

                if (DmaDirection == WdfDmaDirectionWriteToDevice) {
                    reqLength = stack->Parameters.DeviceIoControl.InputBufferLength;
                } else {
                    reqLength = stack->Parameters.DeviceIoControl.OutputBufferLength;
                }

                //
                // In this case both input buffer and output buffer map
                // to the same MDL and it's probed for read & write access.
                // So it's okay for DMA transfer in either direction.
                //
                status = pReqObj->GetMdl(&mdl);
                break;

            case METHOD_IN_DIRECT:
                //
                // For this type, the output buffer is probed for read access.
                // So the direction of DMA transfer is WdfDmaDirectionWriteToDevice.
                //
                if (DmaDirection != WdfDmaDirectionWriteToDevice) {

                    status = STATUS_INVALID_DEVICE_REQUEST;

                    DoTraceLevelMessage(pFxDriverGlobals,
                                        TRACE_LEVEL_ERROR, TRACINGDMA,
                                        "Dma direction %!WDF_DMA_DIRECTION! of WDFTRANSACTION "
                                        "0x%p doesn't match with WDFREQUEST 0x%p ioctl type "
                                        "METHOD_IN_DIRECT %!STATUS!",
                                        DmaDirection, DmaTransaction, Request, status);
                    return status;
                }

                reqLength = stack->Parameters.DeviceIoControl.OutputBufferLength;

                status = pReqObj->GetDeviceControlOutputMdl(&mdl);

                break;

            case METHOD_OUT_DIRECT:
                //
                // For this type, the output buffer is probed for write access.
                // So the direction of DMA transfer is WdfDmaDirectionReadFromDevice.
                //
                if (DmaDirection != WdfDmaDirectionReadFromDevice) {

                    status = STATUS_INVALID_DEVICE_REQUEST;

                    DoTraceLevelMessage(pFxDriverGlobals,
                                        TRACE_LEVEL_ERROR, TRACINGDMA,
                                        "Dma direction %!WDF_DMA_DIRECTION! of WDFTRANSACTION "
                                        "0x%p doesn't match with WDFREQUEST 0x%p ioctl type "
                                        "METHOD_OUT_DIRECT %!STATUS!",
                                        DmaDirection, DmaTransaction, Request, status);

                    return status;
                }

                reqLength = stack->Parameters.DeviceIoControl.OutputBufferLength;

                status = pReqObj->GetDeviceControlOutputMdl(&mdl);

                break;
            default:

                status = STATUS_INVALID_DEVICE_REQUEST;

                DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                                    "Invalid ioctl code in WDFREQUEST 0x%p %!STATUS!",
                                    Request, status);

                FxVerifierDbgBreakPoint(pFxDriverGlobals);
                break;

        }// End of switch(ioctType)
        break;

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;

    }

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "Couldn't retrieve mdl from WDFREQUEST 0x%p for "
                            "WDFTRANSACTION 0x%p %!STATUS!",
                            Request, DmaTransaction, status);
        return status;
    }

    if (reqLength == 0) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
            "Zero length request, %!STATUS!", status);
        return status;
    }

    //
    // If the DMA enabler is packet based, make sure the virtual address and
    // the length of transfer are within bounds. Basically, we are checking
    // to see if the length of data to be transferred doesn't span multiple
    // MDLs, because packet based DMA doesn't support chained MDLs.
    //
    if (pDmaTrans->GetDmaEnabler()->SupportsChainedMdls() == FALSE) {
        ULONG  length;

        length = MmGetMdlByteCount(mdl);

        if (reqLength > length) {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                "WDFREQUEST %p transfer length (%d) is out of bounds of MDL "
                "Byte count (%d), %!STATUS!",
                Request, reqLength, length, status);

            return status;
        }
    }

    //
    // Parms appear OK, so initialize this instance.
    //
    status = pDmaTrans->Initialize(EvtProgramDmaFunction,
                                   DmaDirection,
                                   mdl,
                                   0,
                                   reqLength);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "WDFTANSACTION 0x%p initialization failed: "
                            "%!STATUS!", DmaTransaction, status);
        return status;
    }

    //
    // Set this Request in the new DmaTransaction.  The request will
    // take a reference on this request when it starts executing.
    //
    pDmaTrans->SetRequest(pReqObj);

    return STATUS_SUCCESS;
}


_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfDmaTransactionInitializeUsingOffset)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction,
    __in
    PFN_WDF_PROGRAM_DMA EvtProgramDmaFunction,
    __in
    WDF_DMA_DIRECTION DmaDirection,
    __in
    PMDL Mdl,
    __in
    size_t Offset,
    __in
    __drv_when(Length == 0, __drv_reportError(Length cannot be zero))
    size_t Length
    )
{
    //
    // Stub this out by calling the regular initialize method.  Eventually
    // the regular initialize method will call this instead.
    //

    return WDFEXPORT(WdfDmaTransactionInitialize)(
            DriverGlobals,
            DmaTransaction,
            EvtProgramDmaFunction,
            DmaDirection,
            Mdl,
            (PVOID) (((ULONG_PTR) MmGetMdlVirtualAddress(Mdl)) + Offset),
            Length
            );
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfDmaTransactionInitialize)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction,
    __in
    PFN_WDF_PROGRAM_DMA EvtProgramDmaFunction,
    __in
    WDF_DMA_DIRECTION DmaDirection,
    __in
    PMDL Mdl,

    //__drv_when(DmaDirection == WdfDmaDirectionReadFromDevice, __out_bcount(Length))
    //__drv_when(DmaDirection == WdfDmaDirectionWriteToDevice, __in_bcount(Length))
    __in
    PVOID VirtualAddress,
    __in
    __drv_when(Length == 0, __drv_reportError(Length cannot be zero))
    size_t Length
    )
{
    NTSTATUS           status;
    FxDmaTransactionBase * pDmaTrans;
    PUCHAR pVA;
    ULONG  mdlLength;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DmaTransaction,
                                   FX_TYPE_DMA_TRANSACTION,
                                   (PVOID *) &pDmaTrans,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, EvtProgramDmaFunction);
    FxPointerNotNull(pFxDriverGlobals, Mdl);

    if (Length == 0) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "Can't initialize WDFDMATRANSACTION 0x%p with "
                            "zero length transfer", DmaTransaction);
        return status;
    }

    if (DmaDirection != WdfDmaDirectionReadFromDevice &&
        DmaDirection != WdfDmaDirectionWriteToDevice) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
            "Initialization of WDFDMATRANSACTION 0x%p,DmaDirection 0x%x is an "
            "invalid value, %!STATUS!", DmaTransaction, DmaDirection, status);
        return status;
    }

    //
    // Make sure the VirtualAddress is within the first MDL bounds.
    //
    pVA  = (PUCHAR) MmGetMdlVirtualAddress(Mdl);
    mdlLength = MmGetMdlByteCount(Mdl);

    if (VirtualAddress < pVA ||
        VirtualAddress >= WDF_PTR_ADD_OFFSET(pVA, mdlLength)) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
            "VirtualAddress %p is not within the Mdl bounds, StartVA (%p) + "
            "ByteCount (0x%x), %!STATUS! ",
            VirtualAddress, pVA, mdlLength, status);

        return status;
    }

    //
    // Get the DmaEnabler
    //
    FxObjectHandleGetPtr(pFxDriverGlobals,
                         DmaTransaction,
                         FX_TYPE_DMA_TRANSACTION,
                         (PVOID *) &pDmaTrans);

    if (pDmaTrans->GetDmaEnabler()->SupportsChainedMdls() == FALSE) {
        //
        // Make sure the MDL is not a chained MDL by checking
        // to see if the virtual address and the length
        // are within bounds.
        //
        if (WDF_PTR_ADD_OFFSET(VirtualAddress, Length) >
            WDF_PTR_ADD_OFFSET(pVA, mdlLength)) {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                "VirtualAddress+Length (%p+%I64d) is out of bounds of MDL VA + MDL "
                "Byte count (max address is %p). "
                "Possibly a chained MDL, %!STATUS!",
                VirtualAddress, Length,
                WDF_PTR_ADD_OFFSET(pVA, mdlLength), status);

            return status;
        }
    }

    status = pDmaTrans->Initialize(EvtProgramDmaFunction,
                                   DmaDirection,
                                   Mdl,
                                   WDF_PTR_GET_OFFSET(
                                    MmGetMdlVirtualAddress(Mdl),
                                    VirtualAddress
                                    ),
                                   (ULONG) Length);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "WDFTANSACTION 0x%p initialization failed: "
                            "%!STATUS!", DmaTransaction, status);
    }

    return status;
}


_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfDmaTransactionExecute)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction,
    __in_opt
    WDFCONTEXT Context
    )
{
    FxDmaTransactionBase* pDmaTrans;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         DmaTransaction,
                         FX_TYPE_DMA_TRANSACTION,
                         (PVOID *) &pDmaTrans);

    return pDmaTrans->Execute(Context);
}

__success(TRUE)
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfDmaTransactionRelease)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION  DmaTransaction
    )
{
    FxDmaTransactionBase* pDmaTrans;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         DmaTransaction,
                         FX_TYPE_DMA_TRANSACTION,
                         (PVOID *) &pDmaTrans);

    //
    // Release map registers allocated for this specific transaction,
    // but not map registers which were allocated through
    // AllocateResources.
    //
    pDmaTrans->ReleaseForReuse(FALSE);
    return STATUS_SUCCESS;
}


__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
WDFEXPORT(WdfDmaTransactionDmaCompleted)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction,
    __out
    NTSTATUS * pStatus
    )
{
    FxDmaTransactionBase    * pDmaTrans;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         DmaTransaction,
                         FX_TYPE_DMA_TRANSACTION,
                         (PVOID *) &pDmaTrans);

    FxPointerNotNull(pDmaTrans->GetDriverGlobals(), pStatus);


    //
    // Indicate this DMA has been completed.
    //
    return pDmaTrans->DmaCompleted(0, pStatus, FxDmaCompletionTypeFull);
}

__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
WDFEXPORT(WdfDmaTransactionDmaCompletedWithLength)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction,
    __in
    size_t TransferredLength,
    __out
    NTSTATUS * pStatus
    )
{
    FxDmaTransactionBase* pDmaTrans;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         DmaTransaction,
                         FX_TYPE_DMA_TRANSACTION,
                         (PVOID *) &pDmaTrans);

    FxPointerNotNull(pDmaTrans->GetDriverGlobals(), pStatus);

    //
    // Indicate this DMA transfer has been completed.
    //
    return pDmaTrans->DmaCompleted(TransferredLength,
                                   pStatus,
                                   FxDmaCompletionTypePartial);
}

__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
WDFEXPORT(WdfDmaTransactionDmaCompletedFinal)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction,
    __in
    size_t FinalTransferredLength,
    __out
    NTSTATUS * pStatus
    )
{
    FxDmaTransactionBase* pDmaTrans;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         DmaTransaction,
                         FX_TYPE_DMA_TRANSACTION,
                         (PVOID *) &pDmaTrans);

    FxPointerNotNull(pDmaTrans->GetDriverGlobals(), pStatus);

    //
    // Indicate this DMA FinalLength has completed.
    //
    return pDmaTrans->DmaCompleted(FinalTransferredLength,
                                   pStatus,
                                   FxDmaCompletionTypeAbort);
}


__drv_maxIRQL(DISPATCH_LEVEL)
size_t
WDFEXPORT(WdfDmaTransactionGetBytesTransferred)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction
    )
{
    FxDmaTransactionBase* pDmaTrans;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         DmaTransaction,
                         FX_TYPE_DMA_TRANSACTION,
                         (PVOID *) &pDmaTrans);

    return pDmaTrans->GetBytesTransferred();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDmaTransactionSetMaximumLength)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction,
    __in
    size_t MaximumLength
    )
{
    FxDmaTransactionBase* pDmaTrans;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         DmaTransaction,
                         FX_TYPE_DMA_TRANSACTION,
                         (PVOID *) &pDmaTrans);

    pDmaTrans->SetMaximumFragmentLength(MaximumLength);
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFREQUEST
WDFEXPORT(WdfDmaTransactionGetRequest)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction
    )
{
    FxDmaTransactionBase* pDmaTrans;
    FxRequest* pRequest;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         DmaTransaction,
                         FX_TYPE_DMA_TRANSACTION,
                         (PVOID *) &pDmaTrans);

    pRequest = pDmaTrans->GetRequest();

    if (pRequest != NULL) {
        return pRequest->GetHandle();
    }
    else {
        return NULL;
    }
}

__drv_maxIRQL(DISPATCH_LEVEL)
size_t
WDFEXPORT(WdfDmaTransactionGetCurrentDmaTransferLength)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction
    )
{
    FxDmaTransactionBase* pDmaTrans;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         DmaTransaction,
                         FX_TYPE_DMA_TRANSACTION,
                         (PVOID *) &pDmaTrans);

    return pDmaTrans->GetCurrentFragmentLength();
}


__drv_maxIRQL(DISPATCH_LEVEL)
WDFDEVICE
WDFEXPORT(WdfDmaTransactionGetDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction
    )
{
    FxDmaTransactionBase* pDmaTrans;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         DmaTransaction,
                         FX_TYPE_DMA_TRANSACTION,
                         (PVOID *) &pDmaTrans);

    return pDmaTrans->GetDmaEnabler()->GetDeviceHandle();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDmaTransactionSetChannelConfigurationCallback)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction,
    __in_opt
    PFN_WDF_DMA_TRANSACTION_CONFIGURE_DMA_CHANNEL ConfigureRoutine,
    __in_opt
    PVOID ConfigureContext
    )
{
    FxDmaTransactionBase* pDmaTrans;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DmaTransaction,
                                   FX_TYPE_DMA_TRANSACTION,
                                   (PVOID *) &pDmaTrans,
                                   &pFxDriverGlobals);

    //
    // Verify that the transaction belongs to a system profile enabler.
    //

    WDF_DMA_PROFILE profile = pDmaTrans->GetDmaEnabler()->GetProfile();
    if ((profile != WdfDmaProfileSystem) &&
        (profile != WdfDmaProfileSystemDuplex)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "Cannot call %!FUNC! on non-system-profile "
                            "WDFDMATRANSACTION (%p) (transaction profile "
                            "is %!WDF_DMA_PROFILE!).",
                            DmaTransaction,
                            profile);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    //
    // Cast the transaction to the right sub-type now that we've verified the
    // profile.
    //

    FxDmaSystemTransaction* systemTransaction = (FxDmaSystemTransaction*) pDmaTrans;
    systemTransaction->SetConfigureChannelCallback(ConfigureRoutine,
                                                   ConfigureContext);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDmaTransactionSetTransferCompleteCallback)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction,
    __in_opt
    PFN_WDF_DMA_TRANSACTION_DMA_TRANSFER_COMPLETE DmaCompletionRoutine,
    __in_opt
    PVOID DmaCompletionContext
    )
{
    FxDmaTransactionBase* pDmaTrans;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DmaTransaction,
                                   FX_TYPE_DMA_TRANSACTION,
                                   (PVOID *) &pDmaTrans,
                                   &pFxDriverGlobals);

    //
    // Verify that the transaction belongs to a system profile enabler.
    //

    WDF_DMA_PROFILE profile = pDmaTrans->GetDmaEnabler()->GetProfile();
    if ((profile != WdfDmaProfileSystem) &&
        (profile != WdfDmaProfileSystemDuplex)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "Cannot call %!FUNC! on non-system-profile "
                            "WDFDMATRANSACTION (%p) (transaction profile "
                            "is %!WDF_DMA_PROFILE!).",
                            DmaTransaction,
                            profile);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    //
    // Cast the transaction to the right sub-type now that we've verified the
    // profile.
    //

    FxDmaSystemTransaction* systemTransaction = (FxDmaSystemTransaction*) pDmaTrans;
    systemTransaction->SetTransferCompleteCallback(DmaCompletionRoutine,
                                                   DmaCompletionContext);

}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDmaTransactionSetDeviceAddressOffset)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction,
    __in
    ULONG Offset
    )
{
    FxDmaTransactionBase* pDmaTrans;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DmaTransaction,
                                   FX_TYPE_DMA_TRANSACTION,
                                   (PVOID *) &pDmaTrans,
                                   &pFxDriverGlobals);

    //
    // Verify that the transaction belongs to a system profile enabler.
    //

    WDF_DMA_PROFILE profile = pDmaTrans->GetDmaEnabler()->GetProfile();
    if ((profile != WdfDmaProfileSystem) &&
        (profile != WdfDmaProfileSystemDuplex)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "Cannot call %!FUNC! on non-system-profile "
                            "WDFDMATRANSACTION (%p) (transaction profile "
                            "is %!WDF_DMA_PROFILE!).",
                            DmaTransaction,
                            profile);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    //
    // Cast the transaction to the right sub-type now that we've verified the
    // profile.
    //

    FxDmaSystemTransaction* systemTransaction = (FxDmaSystemTransaction*) pDmaTrans;
    systemTransaction->SetDeviceAddressOffset(Offset);
}

__drv_maxIRQL(DISPATCH_LEVEL)
PVOID
WDFEXPORT(WdfDmaTransactionWdmGetTransferContext)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction
    )
{
    FxDmaTransactionBase* pDmaTrans;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DmaTransaction,
                                   FX_TYPE_DMA_TRANSACTION,
                                   (PVOID *) &pDmaTrans,
                                   &pFxDriverGlobals);

    if (pDmaTrans->GetDmaEnabler()->UsesDmaV3() == FALSE) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "Cannot call %!FUNC! for WDFDMATRANSACTION %p "
                            "because the parent WDFDMAENABLER (%p) is not "
                            "configured to use DMA version 3.",
                            DmaTransaction,
                            pDmaTrans->GetDmaEnabler()->GetHandle());
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return NULL;
    }

    FxDmaTransactionState state = pDmaTrans->GetTransactionState();

    if ((state == FxDmaTransactionStateInvalid) ||
        (state == FxDmaTransactionStateCreated) ||
        (state == FxDmaTransactionStateReleased) ||
        (state == FxDmaTransactionStateDeleted)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "Cannot call %!FUNC! on WDFDMATRANSACTION %p "
                            "becuase it is uninitialized, reused, deleted "
                            "(state is %!FxDmaTransactionState!).",
                            DmaTransaction,
                            state
                            );
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return NULL;
    }

    return pDmaTrans->GetTransferContext();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDmaTransactionGetTransferInfo)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction,
    __out_opt
    ULONG* MapRegisterCount,
    __out_opt
    ULONG* ScatterGatherElementCount
    )
{
    FxDmaTransactionBase* pDmaTrans;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DmaTransaction,
                                   FX_TYPE_DMA_TRANSACTION,
                                   (PVOID *) &pDmaTrans,
                                   &pFxDriverGlobals);

    pDmaTrans->GetTransferInfo(MapRegisterCount, ScatterGatherElementCount);
}

//
// Stubbed WDF 1.11 DMA DDIs start here.
//

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDmaTransactionSetImmediateExecution)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction,
    __in
    BOOLEAN UseImmediateExecution
    )
{
    FxDmaTransactionBase* pDmaTrans;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DmaTransaction,
                                   FX_TYPE_DMA_TRANSACTION,
                                   (PVOID *) &pDmaTrans,
                                   &pFxDriverGlobals);

    if (pDmaTrans->GetDmaEnabler()->UsesDmaV3() == FALSE)
    {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "Cannot call %!FUNC! for WDFDMATRANSACTION %p "
                            "because the parent WDFDMAENABLER (%p) is not "
                            "configured to use DMA version 3.",
                            DmaTransaction,
                            pDmaTrans->GetDmaEnabler()->GetHandle());
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    pDmaTrans->SetImmediateExecution(UseImmediateExecution);
}

__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfDmaTransactionAllocateResources)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction,
    __in
    WDF_DMA_DIRECTION DmaDirection,
    __in
    ULONG RequiredMapRegisters,
    __in
    PFN_WDF_RESERVE_DMA EvtReserveDmaFunction,
    __in
    PVOID EvtReserveDmaContext
    )
{
    FxDmaPacketTransaction* pDmaTrans;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DmaTransaction,
                                   FX_TYPE_DMA_TRANSACTION,
                                   (PVOID *) &pDmaTrans,
                                   &pFxDriverGlobals);

    //
    // Only valid if DMA V3 is enabled.
    //

    if (pDmaTrans->GetDmaEnabler()->UsesDmaV3() == FALSE)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "Cannot call %!FUNC! on WDFDMATRANSACTION %p "
                            "because WDFDMAENABLER %p was not configured "
                            "for DMA version 3 - %!STATUS!.",
                            DmaTransaction,
                            pDmaTrans->GetDmaEnabler()->GetHandle(),
                            status);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return status;
    }

    //
    // Only valid for packet or system profile transactions.
    //
    WDF_DMA_PROFILE profile = pDmaTrans->GetDmaEnabler()->GetProfile();
    if ((profile != WdfDmaProfilePacket) &&
        (profile != WdfDmaProfilePacket64) &&
        (profile != WdfDmaProfileSystem) &&
        (profile != WdfDmaProfileSystemDuplex)) {

        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "Cannot call %!FUNC! on non packet or system "
                            "profile WDFDMATRANSACTION (%p) (transaction "
                            "profile is %!WDF_DMA_PROFILE!) - %!STATUS!.",
                            DmaTransaction,
                            profile,
                            status);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return status;
    }

    //
    // Validate the direction value.
    //
    if (DmaDirection != WdfDmaDirectionReadFromDevice &&
        DmaDirection != WdfDmaDirectionWriteToDevice) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
            "Allocation of DMA adapter for WDFDMATRANSACTION 0x%p, "
            "DmaDirection 0x%x is an invalid value, %!STATUS!",
            DmaTransaction, DmaDirection, status);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return status;
    }

    FxPointerNotNull(pFxDriverGlobals, EvtReserveDmaFunction);

    status = pDmaTrans->ReserveAdapter(RequiredMapRegisters,
                                       DmaDirection,
                                       EvtReserveDmaFunction,
                                       EvtReserveDmaContext);

    return status;
}
__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDmaTransactionFreeResources)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction
    )
{
    FxDmaPacketTransaction* pDmaTrans;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DmaTransaction,
                                   FX_TYPE_DMA_TRANSACTION,
                                   (PVOID *) &pDmaTrans,
                                   &pFxDriverGlobals);

    //
    // Only valid for packet or system profile transactions.
    //
    WDF_DMA_PROFILE profile = pDmaTrans->GetDmaEnabler()->GetProfile();
    if ((profile != WdfDmaProfilePacket) &&
        (profile != WdfDmaProfilePacket64) &&
        (profile != WdfDmaProfileSystem) &&
        (profile != WdfDmaProfileSystemDuplex)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "Cannot call %!FUNC! on non packet or system "
                            "profile WDFDMATRANSACTION (%p) (transaction "
                            "profile is %!WDF_DMA_PROFILE!).",
                            DmaTransaction,
                            profile);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    //
    // Only valid if DMA V3 is enabled.
    //

    if (pDmaTrans->GetDmaEnabler()->UsesDmaV3() == FALSE)
    {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "Cannot call %!FUNC! on WDFDMATRANSACTION %p "
                            "because WDFDMAENABLER %p was not configured "
                            "for DMA version 3",
                            DmaTransaction,
                            pDmaTrans->GetDmaEnabler()->GetHandle());
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    pDmaTrans->ReleaseAdapter();

    return;
}
__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
WDFEXPORT(WdfDmaTransactionCancel)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction
    )
{
    FxDmaTransactionBase* pDmaTrans;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DmaTransaction,
                                   FX_TYPE_DMA_TRANSACTION,
                                   (PVOID *) &pDmaTrans,
                                   &pFxDriverGlobals);

    //
    // Only valid if the enabler uses DMA v3
    //

    if (pDmaTrans->GetDmaEnabler()->UsesDmaV3() == FALSE) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "Cannot call %!FUNC! WDFDMATRANSACTION (%p) "
                            "because enabler is not using DMA version 3",
                            DmaTransaction);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return FALSE;
    }

    return pDmaTrans->CancelResourceAllocation();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDmaTransactionStopSystemTransfer)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDMATRANSACTION DmaTransaction
    )
{
    FxDmaTransactionBase* pDmaTrans;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   DmaTransaction,
                                   FX_TYPE_DMA_TRANSACTION,
                                   (PVOID *) &pDmaTrans,
                                   &pFxDriverGlobals);

    //
    // Verify that the transaction belongs to a system profile enabler.
    //

    WDF_DMA_PROFILE profile = pDmaTrans->GetDmaEnabler()->GetProfile();
    if ((profile != WdfDmaProfileSystem) &&
        (profile != WdfDmaProfileSystemDuplex)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDMA,
                            "Cannot call %!FUNC! on non-system-profile "
                            "WDFDMATRANSACTION (%p) (transaction profile "
                            "is %!WDF_DMA_PROFILE!).",
                            DmaTransaction,
                            profile);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    //
    // Cast the transaction to the right sub-type now that we've verified the
    // profile.
    //

    FxDmaSystemTransaction* systemTransaction = (FxDmaSystemTransaction*) pDmaTrans;
    systemTransaction->StopTransfer();
    return;
}



} // extern "C"
