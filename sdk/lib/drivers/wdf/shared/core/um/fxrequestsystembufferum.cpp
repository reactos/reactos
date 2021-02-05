/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxRequestSystemBuffer.cpp

Abstract:

    This module implements class representing the system buffer in an FxRequest

Author:




Environment:

    User mode only

Revision History:




--*/

#include "coreprivshared.hpp"

// Tracing support
extern "C" {
#include "FxRequestSystemBufferUm.tmh"
}

_Must_inspect_result_
PVOID
FxRequestSystemBuffer::GetBuffer(
    VOID
    )
/*++

Routine Description:
    Returns the system buffer that has been cached away by the call to SetBuffer()

Arguments:
    None

Return Value:
    Valid memory or NULL on error

  --*/
{
    FxIrp* irp = GetRequest()->GetFxIrp();

    //
    // For UMDF, the buffer is always stored in m_Buffer.
    //
    switch (irp->GetMajorFunction()) {
    case IRP_MJ_DEVICE_CONTROL:
    case IRP_MJ_INTERNAL_DEVICE_CONTROL:
    case IRP_MJ_READ:
    case IRP_MJ_WRITE:
        return m_Buffer;

    default:
        ASSERT(FALSE);
        return NULL;
    }
}


