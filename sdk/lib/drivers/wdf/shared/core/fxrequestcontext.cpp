/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxRequestContext.cpp

Abstract:

    This module implements FxRequest object

Author:



Environment:

    Both kernel and user mode

Revision History:



--*/

#include "coreprivshared.hpp"

// Tracing support
extern "C" {
#include "FxRequestContext.tmh"
}

FxRequestContext::FxRequestContext(
    __in FX_REQUEST_CONTEXT_TYPE Type
    ) :
    m_RequestType(Type),
    m_RequestMemory(NULL)

/*++

Routine Description:
    Constructs an FxRequestContext and initialized the m_RequestType field

Arguments:
    Type - The type of this request.





Return Value:
    None.

  --*/
{
    InitCompletionParams();
}

FxRequestContext::~FxRequestContext()
/*++

Routine Description:
    Destruct for an FxRequestContext.  Releases all outstanding references.

Arguments:
    None

Return Value:
    None

  --*/
{
    ASSERT(m_RequestMemory == NULL);
}

VOID
FxRequestContext::StoreAndReferenceMemory(
    __in FxRequestBuffer* Buffer
    )
{
    _StoreAndReferenceMemoryWorker(this, &m_RequestMemory, Buffer);
}

VOID
FxRequestContext::ReleaseAndRestore(
    __in FxRequestBase* Request
    )
/*++

Routine Description:
    This routine releases any outstanding references taken on the previous
    format call and restores any fields in the PIRP that were overwritten
    when the formatting occurred.

Arguments:
    Irp

Return Value:


  --*/

{
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    Request->FreeMdls();
#else
    UNREFERENCED_PARAMETER(Request);
#endif

    if (m_RequestMemory != NULL) {
        m_RequestMemory->RELEASE(this);
        m_RequestMemory = NULL;
    }

    InitCompletionParams();
}

VOID
FxRequestContext::_StoreAndReferenceMemoryWorker(
    __in PVOID Tag,
    __deref_out_opt IFxMemory** PPMemory,
    __in FxRequestBuffer* Buffer
    )
{
    ASSERT(*PPMemory == NULL);

    switch (Buffer->DataType) {
    case FxRequestBufferMemory:
        Buffer->u.Memory.Memory->ADDREF(Tag);
        *PPMemory = Buffer->u.Memory.Memory;
        break;

    case FxRequestBufferReferencedMdl:
        Buffer->u.RefMdl.Memory->ADDREF(Tag);
        *PPMemory = Buffer->u.RefMdl.Memory;
        break;

    default:
        *PPMemory = NULL;
    }
}

VOID
FxRequestContext::FormatWriteParams(
    __in_opt IFxMemory* WriteMemory,
    __in_opt PWDFMEMORY_OFFSET WriteOffsets
    )
{
    m_CompletionParams.Type = WdfRequestTypeWrite;

    if (WriteMemory != NULL) {
        m_CompletionParams.Parameters.Write.Buffer = WriteMemory->GetHandle();
    }

    if (WriteOffsets != NULL) {
        m_CompletionParams.Parameters.Write.Offset =
            WriteOffsets->BufferOffset;
    }
    else {
        m_CompletionParams.Parameters.Write.Offset = 0;
    }
}

VOID
FxRequestContext::FormatReadParams(
    __in_opt IFxMemory* ReadMemory,
    __in_opt PWDFMEMORY_OFFSET ReadOffsets
    )
{
    m_CompletionParams.Type = WdfRequestTypeRead;

    if (ReadMemory != NULL) {
        m_CompletionParams.Parameters.Read.Buffer = ReadMemory->GetHandle();
    }

    if (ReadOffsets != NULL) {
        m_CompletionParams.Parameters.Read.Offset =
            ReadOffsets->BufferOffset;
    }
    else {
        m_CompletionParams.Parameters.Read.Offset = 0;
    }
}

VOID
FxRequestContext::FormatOtherParams(
    __in FxInternalIoctlParams *InternalIoctlParams
    )
{
    m_CompletionParams.Type = WdfRequestTypeOther;
    m_CompletionParams.Parameters.Others.Argument1.Ptr = InternalIoctlParams->Argument1;
    m_CompletionParams.Parameters.Others.Argument2.Ptr = InternalIoctlParams->Argument2;
    m_CompletionParams.Parameters.Others.Argument4.Ptr = InternalIoctlParams->Argument4;
}
