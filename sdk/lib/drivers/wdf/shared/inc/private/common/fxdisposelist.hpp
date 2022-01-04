/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDisposalList.hpp

Abstract:

    This class implements a Disposal list for deferring Dispose
    processing from dispatch to passive level.

Author:






Environment:

    Both kernel and user mode

Revision History:


--*/

#ifndef _FXDISPOSELIST_H_
#define _FXDISPOSELIST_H_

/*
 * Some objects can perform dispose/cleanup processing at DISPATCH_LEVEL,
 * and must defer cleanup processing to PASSIVE_LEVEL.
 *
 * This is either due to use of page-able data or code, or a passive
 * level callback constraint on the object.
 *
 * This class supports this by providing a list to enqueue objects on
 * that need dispose, and an event that may be used by a calling thread
 * such as the Pnp DeviceRemove thread that must synchronize all object
 * rundown associated with an FxDevice before returning.
 *
 * This is designed to operate in an allocation free manner, and re-uses
 * an FxObject entry that is not longer in use when an object is in
 * a deferred dispose state.
 *
 */

class FxDisposeList : public FxNonPagedObject {

private:

    //
    // The List of items to cleanup
    //
    SINGLE_LIST_ENTRY        m_List;

    //
    // Pointer to the end of the list so appending does not require traversal
    // of the entire list
    //
    SINGLE_LIST_ENTRY** m_ListEnd;

    //
    // This is a pointer to thread object that invoked our workitem
    // callback. This value will be used to avoid deadlock when we try
    // to flush the workitem.
    //
    MxThread          m_WorkItemThread;

    FxSystemWorkItem*  m_SystemWorkItem;

    //
    // WDM PDRIVER or PDEVICE_OBJECT for allocating PIO_WORKITEMS
    //
    PVOID             m_WdmObject;

private:
    static
    VOID
    _WorkItemThunk(
        __in PVOID Parameter
        );

    VOID
    DrainListLocked(
        PKIRQL PreviousIrql
        );

    virtual
    BOOLEAN
    Dispose(
        VOID
        );

public:

    static
    NTSTATUS
    _Create(
        PFX_DRIVER_GLOBALS FxDriverGlobals,
        PVOID              WdmObject,
        FxDisposeList**    pObject
        );

    FxDisposeList(
        PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    ~FxDisposeList(
        );

    NTSTATUS
    Initialize(
        PVOID wdmObject
        );

    //
    // Add an object to the list.
    //
    // The object's m_DisposeListEntry is used and must be initialized
    //
    VOID
    Add(
        FxObject* object
        );

    //
    // Waits until the list is empty
    //
    VOID
    WaitForEmpty(
        VOID
        );

    DECLARE_INTERNAL_NEW_OPERATOR();
};

#endif // _FXDISPOSELIST_H_

