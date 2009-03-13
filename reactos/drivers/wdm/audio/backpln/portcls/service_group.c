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
    KEVENT DpcEvent;
    KDPC Dpc;

}IServiceGroupImpl;



//---------------------------------------------------------------
// IUnknown methods
//


NTSTATUS
STDMETHODCALLTYPE
IServiceGroup_fnQueryInterface(
    IServiceGroup* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    WCHAR Buffer[100];
    IServiceGroupImpl * This = (IServiceGroupImpl*)iface;
    if (IsEqualGUIDAligned(refiid, &IID_IServiceGroup) ||
        IsEqualGUIDAligned(refiid, &IID_IServiceSink) ||
        IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtbl;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }

    StringFromCLSID(refiid, Buffer);
    DPRINT1("IPortWaveCyclic_fnQueryInterface no interface!!! iface %S\n", Buffer);

    return STATUS_UNSUCCESSFUL;
}

ULONG
STDMETHODCALLTYPE
IServiceGroup_fnAddRef(
    IServiceGroup* iface)
{
    IServiceGroupImpl * This = (IServiceGroupImpl*)iface;

    return InterlockedIncrement(&This->ref);
}

ULONG
STDMETHODCALLTYPE
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
        KeWaitForSingleObject(&This->DpcEvent, Executive, KernelMode, FALSE, NULL);
        KeCancelTimer(&This->Timer);
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
    PLIST_ENTRY CurEntry;
    PGROUP_ENTRY Entry;
    IServiceGroupImpl * This = (IServiceGroupImpl*)iface;

    CurEntry = This->ServiceSinkHead.Flink;
    while (CurEntry != &This->ServiceSinkHead)
    {
        Entry = CONTAINING_RECORD(CurEntry, GROUP_ENTRY, Entry);
        Entry->pServiceSink->lpVtbl->RequestService(Entry->pServiceSink);
        CurEntry = CurEntry->Flink;
    }
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

    Entry = AllocateItem(NonPagedPool, sizeof(GROUP_ENTRY), TAG_PORTCLASS);
    if (!Entry)
        return STATUS_INSUFFICIENT_RESOURCES;

    Entry->pServiceSink = pServiceSink;
    pServiceSink->lpVtbl->AddRef(pServiceSink);

    //FIXME
    //check if Dpc is active
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

    //FIXME
    //check if Dpc is active
    //

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
    IServiceGroupImpl * This = (IServiceGroupImpl*)DeferredContext;
    IServiceGroup_fnRequestService((IServiceGroup*)DeferredContext);
    KeSetEvent(&This->DpcEvent, IO_SOUND_INCREMENT, FALSE);
}


VOID
NTAPI
IServiceGroup_fnSupportDelayedService(
    IN IServiceGroup * iface)
{
    IServiceGroupImpl * This = (IServiceGroupImpl*)iface;

    if (!This->Initialized)
    {
        KeInitializeEvent(&This->DpcEvent, SynchronizationEvent, FALSE);
        KeInitializeTimerEx(&This->Timer, NotificationTimer);
        KeInitializeDpc(&This->Dpc, IServiceGroupDpc, (PVOID)This);
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

    DueTime.QuadPart = ullDelay;

    if (This->Initialized)
    {
        if (KeGetCurrentIrql() <= DISPATCH_LEVEL)
            KeSetTimer(&This->Timer, DueTime, &This->Dpc);
        else
            KeInsertQueueDpc(&This->Dpc, NULL, NULL);

        KeClearEvent(&This->DpcEvent);
    }
}

VOID
NTAPI
IServiceGroup_fnCancelDelayedService(
    IN IServiceGroup * iface)
{
    IServiceGroupImpl * This = (IServiceGroupImpl*)iface;

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

    DPRINT1("PcNewServiceGroup entered\n");

    This = AllocateItem(NonPagedPool, sizeof(IServiceGroupImpl), TAG_PORTCLASS);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->lpVtbl = &vt_IServiceGroup;
    This->ref = 1;
    InitializeListHead(&This->ServiceSinkHead);
    *OutServiceGroup = (PSERVICEGROUP)&This->lpVtbl;

    return STATUS_SUCCESS;
}
