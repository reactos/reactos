/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    FxIoTargetUm.hpp

Abstract:

Author:



Environment:


Revision History:

--*/

#ifndef _FXIOTARGETUM_H_
#define _FXIOTARGETUM_H_

__inline
FxIoContext::FxIoContext(
    VOID
    ) :
    FxRequestContext(FX_RCT_IO),
    m_OtherMemory(NULL),
    m_RestoreState(FALSE)
{
    ZeroMemory(&m_OriginalBufferInfo, sizeof(m_OriginalBufferInfo));
}

__inline
FxIoContext::~FxIoContext(
    VOID
    )
{
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
        irp->GetIoIrp()->RestoreCurrentBuffer(&m_OriginalBufferInfo);
        m_RestoreState = FALSE;
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
    __super::ReleaseAndRestore(Request);
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
    case UMINT::WdfRequestInternalIoctl:
        m_CompletionParams.Parameters.Ioctl.Output.Length =
            m_CompletionParams.IoStatus.Information;
        break;














    default:
        FX_VERIFY(INTERNAL, CHECK("Non Io Irp passed to CopyParameters", FALSE));
    }
}

__inline
VOID
FxIoContext::SwapIrpBuffer(
    _In_ FxRequestBase* Request,
    _In_ ULONG NewInputBufferCb,
    _In_reads_bytes_opt_(NewInputBufferCb) PVOID NewInputBuffer,
    _In_ ULONG NewOutputBufferCb,
    _In_reads_bytes_opt_(NewOutputBufferCb) PVOID NewOutputBuffer
    )
{
    FxIrp* irp = NULL;

    irp = Request->GetSubmitFxIrp();

    m_RestoreState = TRUE;
    irp->GetIoIrp()->SwapCurrentBuffer(
                                       true,
                                       NewInputBufferCb,
                                       NewInputBuffer,
                                       NewOutputBufferCb,
                                       NewOutputBuffer,
                                       &m_OriginalBufferInfo
                                       );
}

__inline
BOOLEAN
FxIoTarget::HasValidStackSize(
    VOID
    )
{






    return TRUE;
}

__inline
VOID
FxIoTarget::Send(
    _In_ MdIrp Irp
    )
{
    Forward(Irp);
}

#endif // _FXIOTARGETUM_H_
