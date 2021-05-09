/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    EventQueueUm.cpp

Abstract:

    This module implements user mode specific functionality of event queue

    This functionality needed to be separated out since the KM portion takes
    a reference on driver object because KM MxWorkItem::_Free does not wait for
    callback to return (i.e. rundown synchronously).

    MxWorkItem::_Free in UM currently waits for callback to return (i.e. runs
    down synchronously) hence does not need a reference on driver/devstack
    object (see comments on top of MxWorkItemUm.h file).

    In future if UM work-item is made similar to km workitem, UMDF may need to
    ensure that modules stay loaded, though the mechanism to ensure that would
    likely be different from a reference on the driver. It would likely be a
    reference on the devicestack object.

Environment:

    User mode only

Revision History:




--*/

#include "pnppriv.hpp"

VOID
FxWorkItemEventQueue::QueueWorkItem(
    VOID
    )
{
    //
    // In user mode WorkItem::_Free waits for workitem callbacks to return
    // So we don't need outstanding reference on driver object or the devstack
    //

    m_WorkItem.Enqueue(
        (PMX_WORKITEM_ROUTINE) _WorkItemCallback,
        (FxEventQueue*) this);
}

VOID
FxWorkItemEventQueue::_WorkItemCallback(
    __in MdDeviceObject DeviceObject,
    __in PVOID Context
    )
/*++

Routine Description:
    This is the work item that attempts to run the machine on a thread
    separate from the one the caller was using.  It implements step 9 above.

--*/
{
    FxWorkItemEventQueue* This = (FxWorkItemEventQueue*) Context;

    UNREFERENCED_PARAMETER(DeviceObject);

    //
    // In user mode WorkItem::_Free waits for workitem callbacks to return
    // So we don't need outstanding reference on driver object
    //

    This->EventQueueWorker();
}
