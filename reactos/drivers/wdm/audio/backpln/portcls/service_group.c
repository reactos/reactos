/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/service_group.c
 * PURPOSE:         ServiceGroup object implementation
 * PROGRAMMER:      Johannes Anderwald
 */


#include "private.h"

typedef struct
{
    LIST_ENTRY Entry;
    IN PSERVICESINK pServiceSink;
}GROUP_ENTRY, *PGROUP_ENTRY;

typedef struct
{
    IServiceGroupVtbl *lpVtbl;

    LONG ref;
    LIST_ENTRY ServiceSinkHead;

    BOOL Initialized;
    BOOL TimerActive;
    KTIMER Timer;
    KDPC Dpc;
    KEVENT Event;
    LONG ThreadActive;
}IServiceGroupImpl;



//---------------------------------------------------------------
// IUnknown methods
//


NTSTATUS
NTAPI
IServiceGroup_fnQueryInterface(
    IServiceGroup* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    UNICODE_STRING GuidString;
    IServiceGroupImpl * This = (IServiceGroupImpl*)iface;

    if (IsEqualGUIDAligned(refiid, &IID_IServiceGroup) ||
        IsEqualGUIDAligned(refiid, &IID_IServiceSink) ||
        IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtbl;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }

    if (RtlStringFromGUID(refiid, &GuidString) == STATUS_SUCCESS)
    {
        DPRINT1("IServiceGroup_fnQueryInterface no interface!!! iface %S\n", GuidString.Buffer);
        RtlFreeUnicodeString(&GuidString);
    }

    return STATUS_UNSUCCESSFUL;
}

ULONG
NTAPI
IServiceGroup_fnAddRef(
    IServiceGroup* iface)
{
    IServiceGroupImpl * This = (IServiceGroupImpl*)iface;

    return InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IServiceGroup_fnRelease(
    IServiceGroup* iface)
{
    PLIST_ENTRY CurEntry;
    PGROUP_ENTRY Entry;
    IServiceGroupImpl * This = (IServiceGroupImpl*)iface;

    InterlockedDecrement(&This->ref);

    if (This->ref == 0)
    {
        while(!IsListEmpty(&This->ServiceSinkHead))
        {
            CurEntry = RemoveHeadList(&This->ServiceSinkHead);
            Entry = CONTAINING_RECORD(CurEntry, GROUP_ENTRY, Entry);
            Entry->pServiceSink->lpVtbl->Release(Entry->pServiceSink);
            FreeItem(Entry, TAG_PORTCLASS);
        }
        KeCancelTimer(&This->Timer);
        if (This->ThreadActive)
        {
            KeSetEvent(&This->Event, 0, TRUE);
        }
        FreeItem(This, TAG_PORTCLASS);
        return 0;
    }
    /* Return new reference count */
    return This->ref;
}



//---------------------------------------------------------------
// IServiceSink methods
//

VOID
NTAPI
IServiceGroup_fnRequestService(
    IN IServiceGroup * iface)
{
    KIRQL OldIrql;
    IServiceGroupImpl * This = (IServiceGroupImpl*)iface;

    if (KeGetCurrentIrql() > DISPATCH_LEVEL)
    {
        KeInsertQueueDpc(&This->Dpc, NULL, NULL);
        return;
    }

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    KeInsertQueueDpc(&This->Dpc, NULL, NULL);
    KeLowerIrql(OldIrql);
}

//---------------------------------------------------------------
// IServiceGroup methods
//

NTSTATUS
NTAPI
IServiceGroup_fnAddMember(
    IN IServiceGroup * iface,
    IN PSERVICESINK pServiceSink)
{
    PGROUP_ENTRY Entry;
    IServiceGroupImpl * This = (IServiceGroupImpl*)iface;

    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    Entry = AllocateItem(NonPagedPool, sizeof(GROUP_ENTRY), TAG_PORTCLASS);
    if (!Entry)
        return STATUS_INSUFFICIENT_RESOURCES;

    Entry->pServiceSink = pServiceSink;
    pServiceSink->lpVtbl->AddRef(pServiceSink);

    InsertTailList(&This->ServiceSinkHead, &Entry->Entry);

    return STATUS_SUCCESS;
}

VOID
NTAPI
IServiceGroup_fnRemoveMember(
    IN IServiceGroup * iface,
    IN PSERVICESINK pServiceSink)
{
    PLIST_ENTRY CurEntry;
    PGROUP_ENTRY Entry;
    IServiceGroupImpl * This = (IServiceGroupImpl*)iface;

    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    CurEntry = This->ServiceSinkHead.Flink;
    while (CurEntry != &This->ServiceSinkHead)
    {
        Entry = CONTAINING_RECORD(CurEntry, GROUP_ENTRY, Entry);
        if (Entry->pServiceSink == pServiceSink)
        {
            RemoveEntryList(&Entry->Entry);
            pServiceSink->lpVtbl->Release(pServiceSink);
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
    IServiceGroupImpl * This = (IServiceGroupImpl*)DeferredContext;

    CurEntry = This->ServiceSinkHead.Flink;
    while (CurEntry != &This->ServiceSinkHead)
    {
        Entry = CONTAINING_RECORD(CurEntry, GROUP_ENTRY, Entry);
        Entry->pServiceSink->lpVtbl->RequestService(Entry->pServiceSink);
        CurEntry = CurEntry->Flink;
    }
}


VOID
NTAPI
ServiceGroupThread(IN PVOID StartContext)
{
    NTSTATUS Status;
    KWAIT_BLOCK WaitBlockArray[2];
    PVOID WaitObjects[2];
    IServiceGroupImpl * This = (IServiceGroupImpl*)StartContext;

    /* Set thread state */
    InterlockedIncrement(&This->ThreadActive);

    /* Setup the wait objects */
    WaitObjects[0] = &This->Timer;
    WaitObjects[1] = &This->Event;

    do
    {
        /* Wait on our objects */
        Status = KeWaitForMultipleObjects(2, WaitObjects, WaitAny, Executive, KernelMode, FALSE, NULL, WaitBlockArray);

        switch(Status)
        {
            case STATUS_WAIT_0:
                IServiceGroupDpc(&This->Dpc, (PVOID)This, NULL, NULL);
                break;
            case STATUS_WAIT_1:
                PsTerminateSystemThread(STATUS_SUCCESS);
                break;
        }
    }while(TRUE);
}

VOID
NTAPI
IServiceGroup_fnSupportDelayedService(
    IN IServiceGroup * iface)
{
    NTSTATUS Status;
    HANDLE ThreadHandle;
    IServiceGroupImpl * This = (IServiceGroupImpl*)iface;

    ASSERT_IRQL(DISPATCH_LEVEL);

    if (This->Initialized)
        return;

    KeInitializeTimerEx(&This->Timer, NotificationTimer);

    Status = PsCreateSystemThread(&ThreadHandle, THREAD_ALL_ACCESS, NULL, 0, NULL, ServiceGroupThread, (PVOID)This);
    if (NT_SUCCESS(Status))
    {
        ZwClose(ThreadHandle);
        This->Initialized = TRUE;
    }
}

VOID
NTAPI
IServiceGroup_fnRequestDelayedService(
    IN IServiceGroup * iface,
    IN ULONGLONG ullDelay)
{
    LARGE_INTEGER DueTime;
    IServiceGroupImpl * This = (IServiceGroupImpl*)iface;

    ASSERT_IRQL(DISPATCH_LEVEL);

    DueTime.QuadPart = ullDelay;

    if (This->Initialized)
    {
        if (KeGetCurrentIrql() <= DISPATCH_LEVEL)
            KeSetTimer(&This->Timer, DueTime, &This->Dpc);
        else
            KeInsertQueueDpc(&This->Dpc, NULL, NULL);
    }
}

VOID
NTAPI
IServiceGroup_fnCancelDelayedService(
    IN IServiceGroup * iface)
{
    IServiceGroupImpl * This = (IServiceGroupImpl*)iface;

    ASSERT_IRQL(DISPATCH_LEVEL);

    if (This->Initialized)
    {
        KeCancelTimer(&This->Timer);
    }
}

static IServiceGroupVtbl vt_IServiceGroup = 
{
    /* IUnknown methods */
    IServiceGroup_fnQueryInterface,
    IServiceGroup_fnAddRef,
    IServiceGroup_fnRelease,
    IServiceGroup_fnRequestService,
    IServiceGroup_fnAddMember,
    IServiceGroup_fnRemoveMember,
    IServiceGroup_fnSupportDelayedService,
    IServiceGroup_fnRequestDelayedService,
    IServiceGroup_fnCancelDelayedService
};

/*
 * @implemented
 */
NTSTATUS NTAPI
PcNewServiceGroup(
    OUT PSERVICEGROUP* OutServiceGroup,
    IN  PUNKNOWN OuterUnknown OPTIONAL)
{
    IServiceGroupImpl * This;

    DPRINT("PcNewServiceGroup entered\n");

    This = AllocateItem(NonPagedPool, sizeof(IServiceGroupImpl), TAG_PORTCLASS);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->lpVtbl = &vt_IServiceGroup;
    This->ref = 1;
    KeInitializeDpc(&This->Dpc, IServiceGroupDpc, (PVOID)This);
    KeSetImportanceDpc(&This->Dpc, HighImportance);
    KeInitializeEvent(&This->Event, NotificationEvent, FALSE);
    InitializeListHead(&This->ServiceSinkHead);
    *OutServiceGroup = (PSERVICEGROUP)&This->lpVtbl;

    return STATUS_SUCCESS;
}
