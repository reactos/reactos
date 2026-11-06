/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/service_group.cpp
 * PURPOSE:         ServiceGroup object implementation
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

#define NDEBUG
#include <debug.h>

VOID
NTAPI
IServiceGroupDpc(
    IN struct _KDPC  *Dpc,
    IN PVOID  DeferredContext,
    IN PVOID  SystemArgument1,
    IN PVOID  SystemArgument2
    );

typedef struct
{
    LIST_ENTRY Entry;
    IN PSERVICESINK pServiceSink;
}GROUP_ENTRY, *PGROUP_ENTRY;

class CServiceGroup : public CUnknownImpl<IServiceGroup>
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    IMP_IServiceGroup;
    CServiceGroup(IUnknown * OuterUnknown);
    virtual ~CServiceGroup() {}

protected:

    LIST_ENTRY m_ServiceSinkHead;

    BOOL m_TimerInitialized;
    KTIMER m_Timer;
    KDPC m_Dpc;
    KSPIN_LOCK m_Lock;

    friend VOID NTAPI IServiceGroupDpc(IN struct _KDPC  *Dpc, IN PVOID  DeferredContext, IN PVOID  SystemArgument1, IN PVOID  SystemArgument2);
};

//---------------------------------------------------------------
// IUnknown methods
//

NTSTATUS
NTAPI
CServiceGroup::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    UNICODE_STRING GuidString;

    if (IsEqualGUIDAligned(refiid, IID_IServiceGroup) ||
        IsEqualGUIDAligned(refiid, IID_IServiceSink) ||
        IsEqualGUIDAligned(refiid, IID_IUnknown))
    {
        *Output = PVOID(PSERVICEGROUP(this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }

    if (RtlStringFromGUID(refiid, &GuidString) == STATUS_SUCCESS)
    {
        DPRINT1("CServiceGroup::QueryInterface no interface!!! iface %S\n", GuidString.Buffer);
        RtlFreeUnicodeString(&GuidString);
    }

    return STATUS_UNSUCCESSFUL;
}
//---------------------------------------------------------------
// IServiceSink methods
//

CServiceGroup::CServiceGroup(IUnknown * OuterUnknown)
{
    // initialize dpc
    KeInitializeDpc(&m_Dpc, IServiceGroupDpc, (PVOID)this);

    // set highest importance
    KeSetImportanceDpc(&m_Dpc, HighImportance);

    // initialize service group list lock
    KeInitializeSpinLock(&m_Lock);

    // initialize service group list
    InitializeListHead(&m_ServiceSinkHead);
}

VOID
NTAPI
CServiceGroup::RequestService()
{
    KIRQL OldIrql;

    DPRINT("CServiceGroup::RequestService() Dpc at Level %u\n", KeGetCurrentIrql());

    if (m_TimerInitialized)
    {
        LARGE_INTEGER DueTime;

        // no due time
        DueTime.QuadPart = 0LL;

        // delayed service requested
        KeSetTimer(&m_Timer, DueTime, &m_Dpc);
    }
    else
    {
        // check current irql
        if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
        {
            //insert dpc to queue
            KeInsertQueueDpc(&m_Dpc, NULL, NULL);
        }
        else
        {
            // raise irql to dispatch level to make dpc fire immediately
            KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
            // insert dpc to queue
            KeInsertQueueDpc(&m_Dpc, NULL, NULL);
            // lower irql to old level
            KeLowerIrql(OldIrql);
        }
    }
}

//---------------------------------------------------------------
// IServiceGroup methods
//

NTSTATUS
NTAPI
CServiceGroup::AddMember(
    IN PSERVICESINK pServiceSink)
{
    PGROUP_ENTRY Entry;
    KIRQL OldLevel;

    // sanity check
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    // allocate service sink entry
    Entry = (PGROUP_ENTRY)AllocateItem(NonPagedPool, sizeof(GROUP_ENTRY), TAG_PORTCLASS);
    if (!Entry)
    {
        // out of memory
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // initialize service sink entry
    Entry->pServiceSink = pServiceSink;
    // increment reference count
    pServiceSink->AddRef();

    // acquire service group list lock
    KeAcquireSpinLock(&m_Lock, &OldLevel);

    // insert into service sink list
    InsertTailList(&m_ServiceSinkHead, &Entry->Entry);

    // release service group list lock
    KeReleaseSpinLock(&m_Lock, OldLevel);

    return STATUS_SUCCESS;
}

VOID
NTAPI
CServiceGroup::RemoveMember(
    IN PSERVICESINK pServiceSink)
{
    PLIST_ENTRY CurEntry;
    PGROUP_ENTRY Entry;
    KIRQL OldLevel;

    // sanity check
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    // acquire service group list lock
    KeAcquireSpinLock(&m_Lock, &OldLevel);

    // grab first entry
    CurEntry = m_ServiceSinkHead.Flink;

    // loop list until the passed entry is found
    while (CurEntry != &m_ServiceSinkHead)
    {
        // grab entry
        Entry = CONTAINING_RECORD(CurEntry, GROUP_ENTRY, Entry);

        // check if it matches the passed entry
        if (Entry->pServiceSink == pServiceSink)
        {
            // remove entry from list
            RemoveEntryList(&Entry->Entry);

            // release service sink reference
            pServiceSink->Release();

            // free service sink entry
            FreeItem(Entry, TAG_PORTCLASS);

            // leave loop
            break;
        }
        // move to next entry
        CurEntry = CurEntry->Flink;
    }

    // release service group list lock
    KeReleaseSpinLock(&m_Lock, OldLevel);

}

VOID
NTAPI
IServiceGroupDpc(
    IN struct _KDPC  *Dpc,
    IN PVOID  DeferredContext,
    IN PVOID  SystemArgument1,
    IN PVOID  SystemArgument2
    )
{
    PLIST_ENTRY CurEntry;
    PGROUP_ENTRY Entry;
    CServiceGroup * This = (CServiceGroup*)DeferredContext;

    // acquire service group list lock
    KeAcquireSpinLockAtDpcLevel(&This->m_Lock);

    // grab first entry
    CurEntry = This->m_ServiceSinkHead.Flink;

    // loop the list and call the attached service sink/group
    while (CurEntry != &This->m_ServiceSinkHead)
    {
        //grab current entry
        Entry = (PGROUP_ENTRY)CONTAINING_RECORD(CurEntry, GROUP_ENTRY, Entry);

        // call service sink/group
        Entry->pServiceSink->RequestService();

        // move to next entry
        CurEntry = CurEntry->Flink;
    }

    // release service group list lock
    KeReleaseSpinLockFromDpcLevel(&This->m_Lock);
}

VOID
NTAPI
CServiceGroup::SupportDelayedService()
{
    PC_ASSERT_IRQL(DISPATCH_LEVEL);

    // initialize the timer
    KeInitializeTimer(&m_Timer);

    // use the timer to perform service requests
    m_TimerInitialized = TRUE;
}

VOID
NTAPI
CServiceGroup::RequestDelayedService(
    IN ULONGLONG ullDelay)
{
    LARGE_INTEGER DueTime;

    // sanity check
    PC_ASSERT_IRQL(DISPATCH_LEVEL);
    PC_ASSERT(m_TimerInitialized);

    DueTime.QuadPart = ullDelay;

    // set the timer
    KeSetTimer(&m_Timer, DueTime, &m_Dpc);
}

VOID
NTAPI
CServiceGroup::CancelDelayedService()
{
    PC_ASSERT_IRQL(DISPATCH_LEVEL);
    PC_ASSERT(m_TimerInitialized);

    // cancel the timer
    KeCancelTimer(&m_Timer);
}

NTSTATUS
NTAPI
PcNewServiceGroup(
    OUT PSERVICEGROUP* OutServiceGroup,
    IN  PUNKNOWN OuterUnknown OPTIONAL)
{
    CServiceGroup * This;
    NTSTATUS Status;

    DPRINT("PcNewServiceGroup entered\n");

    //FIXME support aggregation
    PC_ASSERT(OuterUnknown == NULL);

    // allocate a service group object
    This = new(NonPagedPool, TAG_PORTCLASS)CServiceGroup(OuterUnknown);

    if (!This)
    {
        // out of memory
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // request IServiceSink interface
    Status = This->QueryInterface(IID_IServiceSink, (PVOID*)OutServiceGroup);

    if (!NT_SUCCESS(Status))
    {
        // failed to acquire service sink interface
        delete This;
        return Status;
    }

    // done
    return Status;
}
