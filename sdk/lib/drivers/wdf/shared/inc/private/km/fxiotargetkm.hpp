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

    Kernel mode only

Revision History:

--*/

#ifndef _FXIOTARGETKM_H_
#define _FXIOTARGETKM_H_

__inline
FxIoContext::FxIoContext(
    VOID
    ) :
    FxRequestContext(FX_RCT_IO),
    m_MdlToFree(NULL),
    m_OriginalMdl(NULL),
    m_BufferToFree(NULL),
    m_OriginalSystemBuffer(NULL),
    m_OriginalUserBuffer(NULL),
    m_OtherMemory(NULL),
    m_CopyBackToBuffer(FALSE),
    m_UnlockPages(FALSE),
    m_RestoreState(FALSE),
    m_BufferToFreeLength(0),
    m_MdlToFreeSize(0)
{
}

__inline
FxIoContext::~FxIoContext(
    VOID
    )
{
    //
    // Free the buffer allocated for the request, reset m_CopyBackToBuffer
    // to FALSE.
    // NOTE: We delay the freeing of the buffer on purpose.
    //
    ClearBuffer();

    //
    // Free the MDL allocated for the request
    //
    if (m_MdlToFree != NULL) {
        //
        // Being defensive here, MmUnlockPages should have been done in
        // ReleaseAndRestore.
        //
        if (m_UnlockPages) {
            MmUnlockPages(m_MdlToFree);
            m_UnlockPages = FALSE;
        }

        FxMdlFree(m_DriverGlobals, m_MdlToFree);
        m_MdlToFree = NULL;
    }
}

__inline
VOID
FxIoContext::ReleaseAndRestore(
    __in FxRequestBase* Request
    )
{
    FxIrp* irp = NULL;

    irp = Request->GetSubmitFxIrp();

    if (m_RestoreState) {
        irp->SetSystemBuffer(m_OriginalSystemBuffer);
        irp->SetUserBuffer(m_OriginalUserBuffer);
        irp->SetMdlAddress(m_OriginalMdl);
        irp->SetFlags(m_OriginalFlags);
        m_OriginalSystemBuffer = NULL;
        m_OriginalUserBuffer = NULL;
        m_OriginalMdl = NULL;
        m_OriginalFlags = NULL;

        m_RestoreState = FALSE;
    }

    //
    // If there was a buffer present don't free the buffer here so that
    // it can be reused for any request with the same size.
    // Similarly if there was an MDL to be freed unlock the pages but dont free
    // the Mdl so that it can be reused.
    //
    if (m_MdlToFree != NULL) {
        if (m_UnlockPages) {
            MmUnlockPages(m_MdlToFree);
            m_UnlockPages = FALSE;
        }


        m_DriverGlobals = Request->GetDriverGlobals();
    }

    //
    // Release the 2ndary buffer if we have an outstanding reference
    //
    if (m_OtherMemory != NULL) {
        m_OtherMemory->RELEASE(this);
        m_OtherMemory = NULL;
    }

    //
    // Release the other buffer and all __super related fields
    //
    FxRequestContext::ReleaseAndRestore(Request); // __super call
}

__inline
VOID
FxIoContext::ClearBuffer(
    VOID
    )
{
    if (m_BufferToFree != NULL) {
        FxPoolFree(m_BufferToFree);
        m_BufferToFree = NULL;
    }

    m_BufferToFreeLength = 0;
    m_CopyBackToBuffer = FALSE;
}

__inline
VOID
FxIoContext::CopyParameters(
    __in FxRequestBase* Request
    )
{
    switch (m_MajorFunction) {
    case IRP_MJ_WRITE:
        m_CompletionParams.Parameters.Write.Length =
            m_CompletionParams.IoStatus.Information;
        break;

    case IRP_MJ_READ:
        m_CompletionParams.Parameters.Read.Length =
            m_CompletionParams.IoStatus.Information;
        break;

    case IRP_MJ_DEVICE_CONTROL:
    case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        m_CompletionParams.Parameters.Ioctl.Output.Length =
            m_CompletionParams.IoStatus.Information;
        break;

    default:
        ASSERT(FALSE);
    }

    if (m_BufferToFree == NULL) {
        return;
    }

    if (m_CopyBackToBuffer) {
        FxIrp* irp = Request->GetSubmitFxIrp();

        if (irp->GetUserBuffer() != NULL) {
            //
            // UserBuffer contains the caller's original output buffer.
            // Copy the results back into the original buffer.
            //
            if (m_MajorFunction == IRP_MJ_DEVICE_CONTROL ||
                m_MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL) {
                ASSERT(irp->GetInformation() <= m_BufferToFreeLength);
            }

            RtlCopyMemory(irp->GetUserBuffer(),
                          m_BufferToFree,
                          irp->GetInformation());
            m_CopyBackToBuffer = FALSE;
        }
    }
}

__inline
VOID
FxIoContext::CaptureState(
    __in FxIrp* Irp
    )
{
    m_RestoreState = TRUE;
    m_OriginalSystemBuffer = Irp->GetSystemBuffer();
    m_OriginalUserBuffer = Irp->GetUserBuffer();
    m_OriginalMdl = Irp->GetMdl();
    m_OriginalFlags = Irp->GetFlags();
}

__inline
VOID
FxIoContext::SetBufferAndLength(
    __in PVOID Buffer,
    __in size_t   BufferLength,
    __in BOOLEAN CopyBackToBuffer
    )
{
    PVOID pOldBuffer;

    pOldBuffer = m_BufferToFree;
    m_BufferToFree = Buffer;
    m_BufferToFreeLength = BufferLength;
    m_CopyBackToBuffer = CopyBackToBuffer;

    if (pOldBuffer != NULL) {
        FxPoolFree(pOldBuffer);
    }
}


__inline
_Must_inspect_result_
NTSTATUS
FxIoTarget::InitModeSpecific(
    __in CfxDeviceBase* Device
    )
{
    UNREFERENCED_PARAMETER(Device);

    DO_NOTHING();

    return STATUS_SUCCESS;
}

__inline
BOOLEAN
FxIoTarget::HasValidStackSize(
    VOID
    )
{
    return (m_TargetStackSize == 0 ? FALSE : TRUE);
}

__inline
VOID
FxIoTarget::Send(
    _In_ MdIrp Irp
    )
{
    //
    // Ignore the return value because once we have sent the request, we
    // want all processing to be done in the completion routine.
    //
    (void) IoCallDriver(m_TargetDevice, Irp);
}

#endif // _FXIOTARGETKM_H_
