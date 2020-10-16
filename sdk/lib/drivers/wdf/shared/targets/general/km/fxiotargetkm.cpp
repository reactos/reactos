/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxIoTargetKm.cpp

Abstract:

    This module implements the IO Target APIs

Author:

Environment:

    kernel mode only

Revision History:

--*/


#include "../../fxtargetsshared.hpp"

extern "C" {
#if defined(EVENT_TRACING)
#include "FxIoTargetKm.tmh"
#endif
}

_Must_inspect_result_
NTSTATUS
FxIoTarget::FormatIoRequest(
    __inout FxRequestBase* Request,
    __in UCHAR MajorCode,
    __in FxRequestBuffer* IoBuffer,
    __in_opt PLONGLONG DeviceOffset,
    _In_opt_ FxFileObject* FileObject
    )
{
    FxIoContext* pContext;
    PVOID pBuffer;
    NTSTATUS status;
    ULONG ioLength;
    BOOLEAN freeSysBuf;
    BOOLEAN setBufferAndLength;
    FxIrp* irp;

    UNREFERENCED_PARAMETER(FileObject);

    ASSERT(MajorCode == IRP_MJ_WRITE || MajorCode == IRP_MJ_READ);

    freeSysBuf = FALSE;
    pBuffer = NULL;

    status = Request->ValidateTarget(this);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (Request->HasContextType(FX_RCT_IO)) {
        pContext = (FxIoContext*) Request->GetContext();
    }
    else {
        pContext = new(GetDriverGlobals()) FxIoContext();
        if (pContext == NULL) {
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                                "could not allocate context for request");

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Since we can error out and return, remember the allocation before
        // we do anything so we can free it later.
        //
        Request->SetContext(pContext);
    }

    //
    // Save away any references to IFxMemory pointers that are passed
    //
    pContext->StoreAndReferenceMemory(IoBuffer);

    irp = Request->GetSubmitFxIrp();
    irp->ClearNextStackLocation();

    CopyFileObjectAndFlags(Request);

    //
    // Note that by convention "Set" methods of FxIrp apply to next stack
    // location unless specified otherwise in the name.
    //
    irp->SetMajorFunction(MajorCode);
    pContext->m_MajorFunction = MajorCode;

    //
    // Anytime we return here and we allocated the context above, the context
    // will be freed when the FxRequest is freed or reformatted.
    //

    ioLength = IoBuffer->GetBufferLength();

    pContext->CaptureState(irp);

    switch (m_TargetIoType) {
    case WdfDeviceIoBuffered:
        irp->SetUserBuffer(NULL);

        if (ioLength != 0) {


            if ((pContext->m_BufferToFreeLength >= ioLength) &&
                (pContext->m_BufferToFree != NULL)) {
                    irp->SetSystemBuffer(pContext->m_BufferToFree);
                    setBufferAndLength = FALSE;
            }
            else {
                irp->SetSystemBuffer(FxPoolAllocate(GetDriverGlobals(),
                                   NonPagedPool,
                                   ioLength));
                if (irp->GetSystemBuffer() == NULL) {
                    DoTraceLevelMessage(
                        GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                        "Could not allocate common buffer");

                    status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                setBufferAndLength = TRUE;
                freeSysBuf = TRUE;
            }

            status = IoBuffer->GetBuffer(&pBuffer);

            if (!NT_SUCCESS(status)) {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                    "Could not retrieve io buffer, %!STATUS!", status);
                break;
            }

            //
            // If its a write, copy into the double buffer now, otherwise,
            // no copy into the buffer is needed for a read.
            //
            if (MajorCode == IRP_MJ_WRITE) {
                if (pBuffer != NULL) {
                    RtlCopyMemory(irp->GetSystemBuffer(),
                                  pBuffer,
                                  ioLength);
                }
            }
            else {
                irp->SetUserBuffer(pBuffer);
            }

            //
            // On reads, copy back to the double buffer after the read has
            // completed.
            //
            if (setBufferAndLength) {
                pContext->SetBufferAndLength(irp->GetSystemBuffer(),
                                    ioLength,
                                    (MajorCode == IRP_MJ_READ) ? TRUE : FALSE);

                freeSysBuf = FALSE; // FxIoContext will free the buffer.
            }
            else {
                pContext->m_CopyBackToBuffer = MajorCode == IRP_MJ_READ  ?
                                                                TRUE : FALSE;
            }
        }
        else {
            //
            // This field was captured and will be restored by the context
            // later.
            //
            irp->SetSystemBuffer(NULL);
        }
        break;
    case WdfDeviceIoDirect:
    {
        BOOLEAN reuseMdl;

        reuseMdl = FALSE;

        if (pContext->m_MdlToFree != NULL) {
            reuseMdl = TRUE;
        }

        status = IoBuffer->GetOrAllocateMdl(
            GetDriverGlobals(),
            irp->GetMdlAddressPointer(),
            &pContext->m_MdlToFree,
            &pContext->m_UnlockPages,
            (MajorCode == IRP_MJ_READ) ? IoWriteAccess : IoReadAccess,
            reuseMdl,
            &pContext->m_MdlToFreeSize
            );

        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Could not retrieve io buffer as a PMDL, %!STATUS!",
                status);
            break;
        }
        break;
    }
    case WdfDeviceIoNeither:
        //
        // Neither MDL nor buffered
        //
        status = IoBuffer->GetBuffer(&pBuffer);

        if (NT_SUCCESS(status)) {
            irp->SetUserBuffer(pBuffer);
        }
        else {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Could not retrieve io buffer as a PVOID, %!STATUS!",
                status);
        }
        break;

    case WdfDeviceIoUndefined:
    default:
        status = STATUS_INVALID_DEVICE_STATE;
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Trying to format closed WDFIOTARGET %p, %!STATUS!",
            GetHandle(), status);
        break;
    }

    //
    // We are assuming  the read and write parts of the Parameters union
    // are at the same offset.  If this is FALSE, WDFCASSERT will not allow
    // this file to compile, so keep these WDFCASSERTs here as long as the
    // assumption is being made.
    //
    WDFCASSERT(FIELD_OFFSET(IO_STACK_LOCATION, Parameters.Write.ByteOffset)
               ==
               FIELD_OFFSET(IO_STACK_LOCATION, Parameters.Read.ByteOffset));

    WDFCASSERT(FIELD_OFFSET(IO_STACK_LOCATION, Parameters.Write.Length)
               ==
               FIELD_OFFSET(IO_STACK_LOCATION, Parameters.Read.Length));

    if (NT_SUCCESS(status)) {

        irp->SetNextParameterWriteLength(ioLength);
        if (DeviceOffset != NULL) {
            irp->SetNextParameterWriteByteOffsetQuadPart(*DeviceOffset);
        }
        else {
            irp->SetNextParameterWriteByteOffsetQuadPart(0);
        }

        Request->VerifierSetFormatted();
    }
    else {
        if (freeSysBuf) {
            FxPoolFree(irp->GetSystemBuffer());
            irp->SetSystemBuffer(NULL);
        }

        Request->ContextReleaseAndRestore();
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxIoTarget::FormatIoctlRequest(
    __in FxRequestBase* Request,
    __in ULONG Ioctl,
    __in BOOLEAN Internal,
    __in FxRequestBuffer* InputBuffer,
    __in FxRequestBuffer* OutputBuffer,
    _In_opt_ FxFileObject* FileObject
    )
{
    FxIoContext* pContext;
    NTSTATUS status;
    PVOID pBuffer;
    ULONG inLength, outLength;
    BOOLEAN freeSysBuf;
    BOOLEAN setBufferAndLength;
    FxIrp* irp;

    UNREFERENCED_PARAMETER(FileObject);

    irp = Request->GetSubmitFxIrp();
    freeSysBuf = FALSE;

    status = Request->ValidateTarget(this);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (Request->HasContextType(FX_RCT_IO)) {
        pContext = (FxIoContext*) Request->GetContext();
    }
    else {
        pContext = new(GetDriverGlobals()) FxIoContext();
        if (pContext == NULL) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Could not allocate context for request");

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Request->SetContext(pContext);
    }

    pContext->CaptureState(irp);

    irp->ClearNextStackLocation();

    //
    // Save away any references to IFxMemory pointers that are passed
    //
    pContext->StoreAndReferenceMemory(InputBuffer);
    pContext->StoreAndReferenceOtherMemory(OutputBuffer);

    UCHAR majorFunction;
    if (Internal) {
        majorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    }
    else {
        majorFunction = IRP_MJ_DEVICE_CONTROL;
    }

    irp->SetMajorFunction(majorFunction);

    pContext->m_MajorFunction = majorFunction;

    CopyFileObjectAndFlags(Request);

    inLength = InputBuffer->GetBufferLength();
    outLength = OutputBuffer->GetBufferLength();

    irp->SetParameterIoctlCode(Ioctl);
    irp->SetParameterIoctlInputBufferLength(inLength);
    irp->SetParameterIoctlOutputBufferLength(outLength);


    //
    // Anytime we return here and we allocated the context above, the context
    // will be freed when the FxRequest is freed or reformatted.
    //
    switch (METHOD_FROM_CTL_CODE(Ioctl)) {
    case METHOD_BUFFERED:

        if (inLength != 0 || outLength != 0) {
            ULONG allocationLength;

            allocationLength = (inLength > outLength ? inLength : outLength);

            if ((pContext->m_BufferToFreeLength >= allocationLength) &&
                (pContext->m_BufferToFree != NULL)) {
                    irp->SetSystemBuffer(pContext->m_BufferToFree);
                    setBufferAndLength = FALSE;
            }
            else {
                irp->SetSystemBuffer(FxPoolAllocate(GetDriverGlobals(),
                                   NonPagedPool,
                                   allocationLength));
                if (irp->GetSystemBuffer() == NULL) {
                    DoTraceLevelMessage(
                        GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                        "Could not allocate common buffer");
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }
                setBufferAndLength = TRUE;
                freeSysBuf = TRUE;
            }

            status = InputBuffer->GetBuffer(&pBuffer);
            if (!NT_SUCCESS(status)) {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                    "Could not retrieve input buffer, %!STATUS!",
                    status);
                break;
            }

            if (pBuffer != NULL) {
                RtlCopyMemory(irp->GetSystemBuffer(),
                              pBuffer,
                              inLength);
            }

            status = OutputBuffer->GetBuffer(&pBuffer);
            if (!NT_SUCCESS(status)) {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                    "Could not retrieve output buffer, %!STATUS!",
                    status);
                break;
            }

            irp->SetUserBuffer(pBuffer);
            if (setBufferAndLength) {
                pContext->SetBufferAndLength(irp->GetSystemBuffer(),
                                             allocationLength,
                                             outLength > 0  ? TRUE : FALSE);
                freeSysBuf = FALSE; // FxIoContext will free the buffer.
            } else {
                pContext->m_CopyBackToBuffer = outLength > 0  ? TRUE : FALSE;
            }

        }
        else {
            //
            // These fields were captured and will be restored by the context
            // later.
            //
            irp->SetUserBuffer(NULL);
            irp->SetSystemBuffer(NULL);
        }

        break;

    case METHOD_DIRECT_TO_HARDWARE:   // METHOD_IN_DIRECT
    case METHOD_DIRECT_FROM_HARDWARE: // METHOD_OUT_DIRECT
    {
        BOOLEAN reuseMdl;

        reuseMdl = FALSE;

        status = InputBuffer->GetBuffer(&pBuffer);
        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Could not retrieve input buffer as a PVOID, %!STATUS!",
                status);
            break;
        }

        irp->SetSystemBuffer(pBuffer);

        //
        // NOTE: There is no need to compare the operation type since that
        // applies only to the Pages locked in memory and not the MDL data
        // structure itself per se.
        // Also, note that if the size of the Outbuf need not be equal to the
        // size of the MdlToFree as long as the number of page entries match.
        //
        if (pContext->m_MdlToFree != NULL) {
            reuseMdl = TRUE;
        }

        status = OutputBuffer->GetOrAllocateMdl(
            GetDriverGlobals(),
            irp->GetMdlAddressPointer(),
            &pContext->m_MdlToFree,
            &pContext->m_UnlockPages,
            (METHOD_FROM_CTL_CODE(Ioctl) == METHOD_DIRECT_TO_HARDWARE) ? IoReadAccess : IoWriteAccess,
            reuseMdl,
            &pContext->m_MdlToFreeSize
            );

        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Could not retrieve output buffer as a PMDL, %!STATUS!",
                status);
            break;
        }
        break;
    }

    case METHOD_NEITHER:
        status = OutputBuffer->GetBuffer(&pBuffer);

        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Could not retrieve output buffer as a PVOID, %!STATUS!",
                status);
            break;
        }

        irp->SetUserBuffer(pBuffer);

        status = InputBuffer->GetBuffer(&pBuffer);

        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Could not retrieve input buffer as a PVOID, %!STATUS!",
                status);

            break;
        }

        irp->SetParameterIoctlType3InputBuffer(pBuffer);
        break;
    }

    if (NT_SUCCESS(status)) {
        Request->VerifierSetFormatted();
    }
    else {
        if (freeSysBuf) {
            FxPoolFree(irp->GetSystemBuffer());
            irp->SetSystemBuffer(NULL);
        }

        Request->ContextReleaseAndRestore();
    }

    return status;
}

