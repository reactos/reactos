/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxWorkItemUm.cpp

Abstract:

    Work item callback thunk implementation for user mode

    We need this thunk to wire the um callback to a mode agnostic work item
    callback.

Author:



Revision History:



--*/

#include "Mx.h"

VOID
CALLBACK
MxWorkItem::_WorkerThunk (
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID Parameter,
    _Inout_ PTP_WAIT Wait,
    _In_ TP_WAIT_RESULT WaitResult
    )
{
    MdWorkItem workItem = (MdWorkItem) Parameter;

    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Wait);
    UNREFERENCED_PARAMETER(WaitResult);

    (*workItem->Callback)(
        workItem->DeviceObject,
        workItem->Context
        );

    return;
}

VOID
MxWorkItem::WaitForCallbacksToComplete(
    VOID
    )
{
    Mx::MxAssert(NULL != m_WorkItem->WaitBlock);

    //
    // Wait for outstanding callbacks to complete.
    //
    WaitForThreadpoolWaitCallbacks(m_WorkItem->WaitBlock,
                           FALSE  // donot cancel pending waits
                           );

}

