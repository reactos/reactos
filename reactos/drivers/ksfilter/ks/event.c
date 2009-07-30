/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/event.c
 * PURPOSE:         KS Event functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "priv.h"

BOOLEAN
KspSynchronizedEventRoutine(
    IN KSEVENTS_LOCKTYPE EventsFlags,
    IN PVOID EventsLock,
    IN PKSEVENT_SYNCHRONIZED_ROUTINE SynchronizedRoutine,
    IN PKSEVENT_CTX Ctx)
{
    BOOLEAN Result = FALSE;
    KIRQL OldLevel;

    if (EventsFlags == KSEVENTS_NONE)
    {
        /* no synchronization required */
        Result = SynchronizedRoutine(Ctx);
    }
    else if (EventsFlags == KSEVENTS_SPINLOCK)
    {
        /* use spin lock */
        KeAcquireSpinLock((PKSPIN_LOCK)EventsLock, &OldLevel);
        Result = SynchronizedRoutine(Ctx);
        KeReleaseSpinLock((PKSPIN_LOCK)EventsLock, OldLevel);
    }
    else if (EventsFlags == KSEVENTS_MUTEX)
    {
        /* use a mutex */
        KeWaitForSingleObject(EventsLock, Executive, KernelMode, FALSE, NULL);
        Result = SynchronizedRoutine(Ctx);
        KeReleaseMutex((PRKMUTEX)EventsLock, FALSE);
    }
    else if (EventsFlags == KSEVENTS_FMUTEX)
    {
        /* use a fast mutex */
        ExAcquireFastMutex((PFAST_MUTEX)EventsLock);
        Result = SynchronizedRoutine(Ctx);
        ExReleaseFastMutex((PFAST_MUTEX)EventsLock);
    }
    else if (EventsFlags == KSEVENTS_FMUTEXUNSAFE)
    {
        /* acquire fast mutex unsafe */
        KeEnterCriticalRegion();
        ExAcquireFastMutexUnsafe((PFAST_MUTEX)EventsLock);
        Result = SynchronizedRoutine(Ctx);
        ExReleaseFastMutexUnsafe((PFAST_MUTEX)EventsLock);
        KeLeaveCriticalRegion();
    }
    else if (EventsFlags == KSEVENTS_INTERRUPT)
    {
        /* use interrupt for locking */
        Result = KeSynchronizeExecution((PKINTERRUPT)EventsLock, (PKSYNCHRONIZE_ROUTINE)SynchronizedRoutine, (PVOID)Ctx);
    }
    else if (EventsFlags == KSEVENTS_ERESOURCE)
    {
        /* use an eresource */
        KeEnterCriticalRegion();
        ExAcquireResourceExclusiveLite((PERESOURCE)EventsLock, TRUE);
        Result = SynchronizedRoutine(Ctx);
        ExReleaseResourceLite((PERESOURCE)EventsLock);
        KeLeaveCriticalRegion();
    }

    return Result;
}


/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsEnableEvent(
    IN  PIRP Irp,
    IN  ULONG EventSetsCount,
    IN  KSEVENT_SET* EventSet,
    IN  OUT PLIST_ENTRY EventsList OPTIONAL,
    IN  KSEVENTS_LOCKTYPE EventsFlags OPTIONAL,
    IN  PVOID EventsLock OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsEnableEventWithAllocator(
    IN  PIRP Irp,
    IN  ULONG EventSetsCount,
    IN  PKSEVENT_SET EventSet,
    IN  OUT PLIST_ENTRY EventsList OPTIONAL,
    IN  KSEVENTS_LOCKTYPE EventsFlags OPTIONAL,
    IN  PVOID EventsLock OPTIONAL,
    IN  PFNKSALLOCATOR Allocator OPTIONAL,
    IN  ULONG EventItemSize OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

BOOLEAN
NTAPI
KspDisableEvent(
    IN PKSEVENT_CTX Ctx)
{
    PIO_STACK_LOCATION IoStack;
    PKSEVENTDATA EventData;
    PKSEVENT_ENTRY EventEntry;
    PLIST_ENTRY Entry;

    /* get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Ctx->Irp);

    /* get event data */
    EventData = (PKSEVENTDATA)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;

    if (!Ctx || !Ctx->List || !Ctx->FileObject || !Ctx->Irp)
    {
        /* invalid parameter */
        return FALSE;
    }

    /* point to first entry */
    Entry = Ctx->List->Flink;

    while(Entry != Ctx->List)
    {
        /* get event entry */
        EventEntry = (PKSEVENT_ENTRY)CONTAINING_RECORD(Entry, KSEVENT_ENTRY, ListEntry);

        if (EventEntry->EventData == EventData && EventEntry->FileObject == Ctx->FileObject)
        {
            /* found the entry */
            RemoveEntryList(&EventEntry->ListEntry);
            Ctx->EventEntry = EventEntry;
            return TRUE;
        }

        /* move to next item */
        Entry = Entry->Flink;
    }
    /* entry not found */
    return TRUE;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsDisableEvent(
    IN  PIRP Irp,
    IN  OUT PLIST_ENTRY EventsList,
    IN  KSEVENTS_LOCKTYPE EventsFlags,
    IN  PVOID EventsLock)
{
    PIO_STACK_LOCATION IoStack;
    KSEVENT_CTX Ctx;

    /* get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* is there a event entry */
    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(KSEVENTDATA))
    {
        if (IoStack->Parameters.DeviceIoControl.InputBufferLength == 0)
        {
            /* caller wants to free event items */
            KsFreeEventList(IoStack->FileObject, EventsList, EventsFlags, EventsLock);
            return STATUS_SUCCESS;
        }
        /* invalid parameter */
        return STATUS_INVALID_BUFFER_SIZE;
    }

    /* setup event ctx */
    Ctx.List = EventsList;
    Ctx.FileObject = IoStack->FileObject;
    Ctx.Irp = Irp;
    Ctx.EventEntry = NULL;

    if (KspSynchronizedEventRoutine(EventsFlags, EventsLock, KspDisableEvent, &Ctx))
    {
        /* was the event entry found */
        if (Ctx.EventEntry)
        {
            /* discard event */
            KsDiscardEvent(Ctx.EventEntry);
            return STATUS_SUCCESS;
        }
        /* event was not found */
        return STATUS_UNSUCCESSFUL;
    }

    /* invalid parameters */
    return STATUS_INVALID_PARAMETER;
}

/*
    @unimplemented
*/
KSDDKAPI
VOID
NTAPI
KsDiscardEvent(
    IN  PKSEVENT_ENTRY EventEntry)
{
    UNIMPLEMENTED;
}


BOOLEAN
NTAPI
KspFreeEventList(
    IN PKSEVENT_CTX Ctx)
{
    PLIST_ENTRY Entry;
    PKSEVENT_ENTRY EventEntry;

    /* check valid input */
    if (!Ctx || !Ctx->List)
        return FALSE;

    /* remove first entry */
    Entry = RemoveHeadList(Ctx->List);
    if (!Entry)
    {
        /* list is empty, bye-bye */
        return FALSE;
    }

    /* get event entry */
    EventEntry = (PKSEVENT_ENTRY)CONTAINING_RECORD(Entry, KSEVENT_ENTRY, ListEntry);

    /* store event entry */
    Ctx->EventEntry = EventEntry;
    /* return success */
    return TRUE;
}


/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsFreeEventList(
    IN  PFILE_OBJECT FileObject,
    IN  OUT PLIST_ENTRY EventsList,
    IN  KSEVENTS_LOCKTYPE EventsFlags,
    IN  PVOID EventsLock)
{
    KSEVENT_CTX Ctx;

    /* setup event ctx */
    Ctx.List = EventsList;
    Ctx.FileObject = FileObject;
    Ctx.EventEntry = NULL;

    while(KspSynchronizedEventRoutine(EventsFlags, EventsLock, KspFreeEventList, &Ctx))
    {
        if (Ctx.EventEntry)
        {
            KsDiscardEvent(Ctx.EventEntry);
        }
    }
}


/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsGenerateEvent(
    IN  PKSEVENT_ENTRY EntryEvent)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsGenerateDataEvent(
    IN  PKSEVENT_ENTRY EventEntry,
    IN  ULONG DataSize,
    IN  PVOID Data)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI
VOID
NTAPI
KsGenerateEventList(
    IN GUID* Set OPTIONAL,
    IN ULONG EventId,
    IN PLIST_ENTRY EventsList,
    IN KSEVENTS_LOCKTYPE EventsFlags,
    IN PVOID EventsLock)
{
    UNIMPLEMENTED
}

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsAddEvent(
    IN PVOID Object,
    IN PKSEVENT_ENTRY EventEntry)
{
    PKSBASIC_HEADER Header = (PKSBASIC_HEADER)((ULONG_PTR)Object - sizeof(KSBASIC_HEADER));

    ExInterlockedInsertTailList(&Header->EventList, &EventEntry->ListEntry, &Header->EventListLock);
}

/*
    @implemented
*/
NTSTATUS
NTAPI
KsDefaultAddEventHandler(
    IN PIRP  Irp,
    IN PKSEVENTDATA  EventData,
    IN OUT PKSEVENT_ENTRY  EventEntry)
{
    PIO_STACK_LOCATION IoStack;
    PKSIOBJECT_HEADER ObjectHeader;
    PKSBASIC_HEADER Header;

    /* first get the io stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* now get the object header */
    ObjectHeader =(PKSIOBJECT_HEADER)IoStack->FileObject->FsContext;

    /* sanity check */
    ASSERT(ObjectHeader->ObjectType);

    /* obtain basic header */
    Header = (PKSBASIC_HEADER)((ULONG_PTR)ObjectHeader->ObjectType - sizeof(KSBASIC_HEADER));

    /* now insert the event entry */
    ExInterlockedInsertTailList(&Header->EventList, &EventEntry->ListEntry, &Header->EventListLock);

    /* done */
    return STATUS_SUCCESS;
}



/*
    @unimplemented
*/
KSDDKAPI
void
NTAPI
KsGenerateEvents(
    IN PVOID Object,
    IN const GUID* EventSet OPTIONAL,
    IN ULONG EventId,
    IN ULONG DataSize,
    IN PVOID Data OPTIONAL,
    IN PFNKSGENERATEEVENTCALLBACK CallBack OPTIONAL,
    IN PVOID CallBackContext OPTIONAL)
{
    UNIMPLEMENTED
}
