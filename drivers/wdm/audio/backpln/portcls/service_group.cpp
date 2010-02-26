/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/service_group.cpp
 * PURPOSE:         ServiceGroup object implementation
 * PROGRAMMER:      Johannes Anderwald
 */


#include "private.hpp"

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

class CServiceGroup : public IServiceGroup
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    STDMETHODIMP_(ULONG) AddRef()
    {
        InterlockedIncrement(&m_Ref);
        return m_Ref;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        InterlockedDecrement(&m_Ref);

        if (!m_Ref)
        {
            delete this;
            return 0;
        }
        return m_Ref;
    }

    IMP_IServiceGroup;
    CServiceGroup(IUnknown * OuterUnknown);
    virtual ~CServiceGroup() {}

protected:

    LIST_ENTRY m_ServiceSinkHead;

    BOOL m_Initialized;
    BOOL m_TimerActive;
    KTIMER m_Timer;
    KDPC m_Dpc;
    KEVENT m_Event;
    LONG m_ThreadActive;

    friend VOID NTAPI IServiceGroupDpc(IN struct _KDPC  *Dpc, IN PVOID  DeferredContext, IN PVOID  SystemArgument1, IN PVOID  SystemArgument2);

    LONG m_Ref;

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
        DPRINT("CServiceGroup::QueryInterface no interface!!! iface %S\n", GuidString.Buffer);
        RtlFreeUnicodeString(&GuidString);
    }

    return STATUS_UNSUCCESSFUL;
}
//---------------------------------------------------------------
// IServiceSink methods
//

CServiceGroup::CServiceGroup(IUnknown * OuterUnknown)
{
    KeInitializeDpc(&m_Dpc, IServiceGroupDpc, (PVOID)this);
    KeSetImportanceDpc(&m_Dpc, HighImportance);
    KeInitializeEvent(&m_Event, NotificationEvent, FALSE);
    InitializeListHead(&m_ServiceSinkHead);
}

VOID
NTAPI
CServiceGroup::RequestService()
{
    KIRQL OldIrql;

    DPRINT("CServiceGroup::RequestService() Dpc at Level %u\n", KeGetCurrentIrql());

    if (KeGetCurrentIrql() > DISPATCH_LEVEL)
    {
        KeInsertQueueDpc(&m_Dpc, NULL, NULL);
        return;
    }

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    KeInsertQueueDpc(&m_Dpc, NULL, NULL);
    KeLowerIrql(OldIrql);
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

    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    Entry = (PGROUP_ENTRY)AllocateItem(NonPagedPool, sizeof(GROUP_ENTRY), TAG_PORTCLASS);
    if (!Entry)
        return STATUS_INSUFFICIENT_RESOURCES;

    Entry->pServiceSink = pServiceSink;
    pServiceSink->AddRef();

    InsertTailList(&m_ServiceSinkHead, &Entry->Entry);

    return STATUS_SUCCESS;
}

VOID
NTAPI
CServiceGroup::RemoveMember(
    IN PSERVICESINK pServiceSink)
{
    PLIST_ENTRY CurEntry;
    PGROUP_ENTRY Entry;

    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    CurEntry = m_ServiceSinkHead.Flink;
    while (CurEntry != &m_ServiceSinkHead)
    {
        Entry = CONTAINING_RECORD(CurEntry, GROUP_ENTRY, Entry);
        if (Entry->pServiceSink == pServiceSink)
        {
            RemoveEntryList(&Entry->Entry);
            pServiceSink->Release();
            FreeItem(Entry, TAG_PORTCLASS);
            return;
        }
        CurEntry = CurEntry->Flink;
    }

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

    CurEntry = This->m_ServiceSinkHead.Flink;
    while (CurEntry != &This->m_ServiceSinkHead)
    {
        Entry = (PGROUP_ENTRY)CONTAINING_RECORD(CurEntry, GROUP_ENTRY, Entry);
        Entry->pServiceSink->RequestService();
        CurEntry = CurEntry->Flink;
    }
}


#if 0
VOID
NTAPI
ServiceGroupThread(IN PVOID StartContext)
{
    NTSTATUS Status;
    KWAIT_BLOCK WaitBlockArray[2];
    PVOID WaitObjects[2];
    CServiceGroup * This = (CServiceGroup*)StartContext;

    // Set thread state
    InterlockedIncrement(&This->m_ThreadActive);

    // Setup the wait objects
    WaitObjects[0] = &m_Timer;
    WaitObjects[1] = &m_Event;

    do
    {
        // Wait on our objects
        Status = KeWaitForMultipleObjects(2, WaitObjects, WaitAny, Executive, KernelMode, FALSE, NULL, WaitBlockArray);

        switch(Status)
        {
            case STATUS_WAIT_0:
                IServiceGroupDpc(&This->m_Dpc, (PVOID)This, NULL, NULL);
                break;
            case STATUS_WAIT_1:
                PsTerminateSystemThread(STATUS_SUCCESS);
                return;
        }
    }while(TRUE);
}

#endif
VOID
NTAPI
CServiceGroup::SupportDelayedService()
{
    //NTSTATUS Status;
    //HANDLE ThreadHandle;

    PC_ASSERT_IRQL(DISPATCH_LEVEL);

    if (m_Initialized)
        return;

    KeInitializeTimerEx(&m_Timer, NotificationTimer);

#if 0
    Status = PsCreateSystemThread(&ThreadHandle, THREAD_ALL_ACCESS, NULL, 0, NULL, ServiceGroupThread, (PVOID)This);
    if (NT_SUCCESS(Status))
    {
        ZwClose(ThreadHandle);
        m_Initialized = TRUE;
    }
#endif
}

VOID
NTAPI
CServiceGroup::RequestDelayedService(
    IN ULONGLONG ullDelay)
{
    LARGE_INTEGER DueTime;

    PC_ASSERT_IRQL(DISPATCH_LEVEL);

    DueTime.QuadPart = ullDelay;

    if (m_Initialized)
    {
        if (KeGetCurrentIrql() <= DISPATCH_LEVEL)
            KeSetTimer(&m_Timer, DueTime, &m_Dpc);
        else
            KeInsertQueueDpc(&m_Dpc, NULL, NULL);
    }
}

VOID
NTAPI
CServiceGroup::CancelDelayedService()
{
    PC_ASSERT_IRQL(DISPATCH_LEVEL);

    if (m_Initialized)
    {
        KeCancelTimer(&m_Timer);
    }
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

    This = new(NonPagedPool, TAG_PORTCLASS)CServiceGroup(OuterUnknown);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = This->QueryInterface(IID_IServiceSink, (PVOID*)OutServiceGroup);

    if (!NT_SUCCESS(Status))
    {
        delete This;
        return Status;
    }

    return Status;
}
