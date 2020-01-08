#ifndef _MXWORKITEM_H_
#define _MXWORKITEM_H_


#include <ntddk.h>

typedef IO_WORKITEM_ROUTINE MX_WORKITEM_ROUTINE, *PMX_WORKITEM_ROUTINE;
typedef PIO_WORKITEM MdWorkItem;

class MxWorkItem {

public:

    __inline
    MxWorkItem(
        )
    {
        m_WorkItem = NULL;
    }

     __inline
    ~MxWorkItem(
        )
    {
    }

    __inline
    MdWorkItem
    GetWorkItem(
        )
    {
        return m_WorkItem;
    }

    __inline
    VOID
    _Free(
        __in MdWorkItem Item
        )
    {
        IoFreeWorkItem(Item);
    }

    __inline
    VOID
    Free(
        )
    {
        if (NULL != m_WorkItem)
        {
            MxWorkItem::_Free(m_WorkItem);
            m_WorkItem = NULL;
        }
    }

    __inline
    VOID
    Enqueue(
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

protected:
    MdWorkItem m_WorkItem;

};

#endif //_MXWORKITEM_H_