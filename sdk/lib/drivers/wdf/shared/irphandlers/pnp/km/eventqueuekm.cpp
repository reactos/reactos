/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    EventQueueKm.cpp

Abstract:

    This module implements kernel mode specific functionality of event queue

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

    Kernel mode only

Revision History:




--*/

#include "pnppriv.hpp"

VOID
FxWorkItemEventQueue::QueueWorkItem(
    VOID
    )
{
    //
    // The work item will take a reference on KMDF itself.  This will keep KMDF's
    // image around (but not necessarily prevent DriverUnload from being called)
    // so that the code after we set the done event will be in memory and not
    // unloaded.  We must do this because there is no explicit reference between
    // the client driver and KMDF, so when the io manager calls the client's
    // DriverUnload, it has no way of managing KMDF's ref count to stay in memory
    // when the loader unloads KMDF explicitly in response to DriverUnload.
    //
    // We manually take a reference on the client so that we provide the same
    // functionality that IO workitems do.  The client driver's image will have
    // a reference on it after it has returned.
    //




    Mx::MxReferenceObject(m_PkgPnp->GetDriverGlobals()->Driver->GetDriverObject());

    //
    // Prevent FxDriverGlobals from being deleted until the workitem finishes
    // its work. In case of a bus driver with a PDO in removed
    // state, if the bus is removed, the removal of PDO may happen in a workitem
    // and may result in unload routine being called before the PDO package is
    // deallocated. Since FxPool deallocation code touches FxDriverGlobals
    // (a pointer of which is located in the header of each FxPool allocated
    // object), taking this ref prevents the globals from going away until a
    // corresponding deref at the end of workitem.
    //
    m_PkgPnp->GetDriverGlobals()->ADDREF((PVOID)_WorkItemCallback);

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
    PFX_DRIVER_GLOBALS  pFxDriverGlobals;

    UNREFERENCED_PARAMETER(DeviceObject);

    MdDriverObject pDriverObject;

    pFxDriverGlobals = This->m_PkgPnp->GetDriverGlobals();

    //
    // Capture the driver object before we call the EventQueueWoker() because
    // the time it returns, This could be freed.



    pDriverObject = pFxDriverGlobals->Driver->GetDriverObject();

    This->EventQueueWorker();

    //
    // Release the ref on FxDriverGlobals taken before queuing this workitem.
    //
    pFxDriverGlobals->RELEASE((PVOID)_WorkItemCallback);

    Mx::MxDereferenceObject(pDriverObject);
}

