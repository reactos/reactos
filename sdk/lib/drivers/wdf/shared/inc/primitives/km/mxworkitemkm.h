/*++

Copyright (c) Microsoft Corporation

ModuleName:

    MxWorkItemKm.h

Abstract:

    Kernel mode implementation of work item
    class defined in MxWorkItem.h

Author:



Revision History:



--*/

#pragma once

typedef IO_WORKITEM_ROUTINE MX_WORKITEM_ROUTINE, *PMX_WORKITEM_ROUTINE;
typedef PIO_WORKITEM MdWorkItem;

#include "MxWorkItem.h"

__inline
MxWorkItem::MxWorkItem(
    )
{
    m_WorkItem = NULL;
}

_Must_inspect_result_
__inline
NTSTATUS
MxWorkItem::Allocate(
    __in MdDeviceObject DeviceObject,
    __in_opt PVOID ThreadPoolEnv
    )
{
    UNREFERENCED_PARAMETER(ThreadPoolEnv);

    m_WorkItem = IoAllocateWorkItem(DeviceObject);
    if (NULL == m_WorkItem) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

__inline
VOID
MxWorkItem::Enqueue(
    __in PMX_WORKITEM_ROUTINE Callback,
    __in PVOID Context
    )
{
    IoQueueWorkItem(
        m_WorkItem,
        Callback,
        DelayedWorkQueue,
        Context
        );
}

__inline
MdWorkItem
MxWorkItem::GetWorkItem(
    )
{
    return m_WorkItem;
}

__inline
VOID
MxWorkItem::_Free(
    __in MdWorkItem Item
    )
{
    IoFreeWorkItem(Item);
}

__inline
VOID
MxWorkItem::Free(
    )
{
    if (NULL != m_WorkItem) {
        MxWorkItem::_Free(m_WorkItem);
        m_WorkItem = NULL;
    }
}

//
// FxAutoWorkitem
//
__inline
MxAutoWorkItem::~MxAutoWorkItem(
    )
{
    this->Free();
}


