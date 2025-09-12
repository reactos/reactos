/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/event.c
 * PURPOSE:         KS Event functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

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

BOOLEAN
NTAPI
SyncAddEvent(
    PKSEVENT_CTX Context)
{
    InsertTailList(Context->List, &Context->EventEntry->ListEntry);
    return TRUE;
}

NTSTATUS
KspEnableEvent(
    IN  PIRP Irp,
    IN  ULONG EventSetsCount,
    IN  const KSEVENT_SET* EventSet,
    IN  OUT PLIST_ENTRY EventsList OPTIONAL,
    IN  KSEVENTS_LOCKTYPE EventsFlags OPTIONAL,
    IN  PVOID EventsLock OPTIONAL,
    IN  PFNKSALLOCATOR Allocator OPTIONAL,
    IN  ULONG EventItemSize OPTIONAL)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    KSEVENT Event;
    PKSEVENT_ITEM EventItem, FoundEventItem;
    PKSEVENTDATA EventData;
    const KSEVENT_SET *FoundEventSet;
    PKSEVENT_ENTRY EventEntry;
    ULONG Index, SubIndex, Size;
    PVOID Object;
    KSEVENT_CTX Ctx;
    LPGUID Guid;

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(KSEVENT))
    {
        /* invalid parameter */
        return STATUS_NOT_SUPPORTED;
    }

    if (Irp->RequestorMode == UserMode)
    {
        _SEH2_TRY
        {
           ProbeForRead(IoStack->Parameters.DeviceIoControl.Type3InputBuffer, sizeof(KSEVENT), sizeof(UCHAR));
           ProbeForRead(Irp->UserBuffer, IoStack->Parameters.DeviceIoControl.OutputBufferLength, sizeof(UCHAR));
           RtlMoveMemory(&Event, IoStack->Parameters.DeviceIoControl.Type3InputBuffer, sizeof(KSEVENT));
           Status = STATUS_SUCCESS;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Exception, get the error code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        /* check for success */
        if (!NT_SUCCESS(Status))
        {
            /* failed to probe parameters */
            return Status;
        }
    }
    else
    {
        /* copy event struct */
        RtlMoveMemory(&Event, IoStack->Parameters.DeviceIoControl.Type3InputBuffer, sizeof(KSEVENT));
    }

    FoundEventItem = NULL;
    FoundEventSet = NULL;


    if (IsEqualGUIDAligned(&Event.Set, &GUID_NULL) && Event.Id == 0 && Event.Flags == KSEVENT_TYPE_SETSUPPORT)
    {
        // store output size
        Irp->IoStatus.Information = sizeof(GUID) * EventSetsCount;
        if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(GUID) * EventSetsCount)
        {
            // buffer too small
            return STATUS_MORE_ENTRIES;
        }

        // get output buffer
        Guid = (LPGUID)Irp->UserBuffer;

       // copy property guids from property sets
       for(Index = 0; Index < EventSetsCount; Index++)
       {
           RtlMoveMemory(&Guid[Index], EventSet[Index].Set, sizeof(GUID));
       }
       return STATUS_SUCCESS;
    }

    /* now try to find event set */
    for(Index = 0; Index < EventSetsCount; Index++)
    {
        if (IsEqualGUIDAligned(&Event.Set, EventSet[Index].Set))
        {
            EventItem = (PKSEVENT_ITEM)EventSet[Index].EventItem;

            /* sanity check */
            ASSERT(EventSet[Index].EventsCount);
            ASSERT(EventItem);

            /* now find matching event id */
            for(SubIndex = 0; SubIndex < EventSet[Index].EventsCount; SubIndex++)
            {
                if (EventItem[SubIndex].EventId == Event.Id)
                {
                    /* found event item */
                    FoundEventItem = &EventItem[SubIndex];
                    FoundEventSet = &EventSet[Index];
                    break;
                }
            }

            if (FoundEventSet)
                break;
        }
    }

    if (!FoundEventSet)
    {
        UNICODE_STRING GuidString;

        RtlStringFromGUID(&Event.Set, &GuidString);

        DPRINT("Guid %S Id %u Flags %x not found\n", GuidString.Buffer, Event.Id, Event.Flags);
        RtlFreeUnicodeString(&GuidString);
        return STATUS_PROPSET_NOT_FOUND;


    }

    if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < FoundEventItem->DataInput)
    {
        /* buffer too small */
        DPRINT1("Got %u expected %u\n", IoStack->Parameters.DeviceIoControl.OutputBufferLength, FoundEventItem->DataInput);
        return STATUS_SUCCESS;
    }

    if (!FoundEventItem->AddHandler && !EventsList)
    {
        /* no add handler and no list to add the new entry to */
        return STATUS_INVALID_PARAMETER;
    }

    /* get event data */
    EventData = Irp->UserBuffer;

    /* sanity check */
    ASSERT(EventData);

    if (Irp->RequestorMode == UserMode)
    {
        if (EventData->NotificationType == KSEVENTF_SEMAPHORE_HANDLE)
        {
            /* get semaphore object handle */
            Status = ObReferenceObjectByHandle(EventData->SemaphoreHandle.Semaphore, SEMAPHORE_MODIFY_STATE, *ExSemaphoreObjectType, Irp->RequestorMode, &Object, NULL);

            if (!NT_SUCCESS(Status))
            {
                /* invalid semaphore handle */
                return STATUS_INVALID_PARAMETER;
            }
        }
        else if (EventData->NotificationType == KSEVENTF_EVENT_HANDLE)
        {
            /* get event object handle */
            Status = ObReferenceObjectByHandle(EventData->EventHandle.Event, EVENT_MODIFY_STATE, *ExEventObjectType, Irp->RequestorMode, &Object, NULL);

            if (!NT_SUCCESS(Status))
            {
                /* invalid event handle */
                return STATUS_INVALID_PARAMETER;
            }
        }
        else
        {
            /* user mode client can only pass an event or semaphore handle */
            return STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        if (EventData->NotificationType != KSEVENTF_EVENT_OBJECT &&
            EventData->NotificationType != KSEVENTF_SEMAPHORE_OBJECT &&
            EventData->NotificationType != KSEVENTF_DPC &&
            EventData->NotificationType != KSEVENTF_WORKITEM &&
            EventData->NotificationType != KSEVENTF_KSWORKITEM)
        {
            /* invalid type requested */
            return STATUS_INVALID_PARAMETER;
        }
    }


    /* calculate request size */
    Size = sizeof(KSEVENT_ENTRY) + FoundEventItem->ExtraEntryData;

    /* do we have an allocator */
    if (Allocator)
    {
        /* allocate event entry */
        Status = Allocator(Irp, Size, FALSE);

        if (!NT_SUCCESS(Status))
        {
            /* failed */
            return Status;
        }

        /* assume the caller put it there */
        EventEntry = KSEVENT_ENTRY_IRP_STORAGE(Irp);

    }
    else
    {
        /* allocate it from nonpaged pool */
        EventEntry = AllocateItem(NonPagedPool, Size);
    }

    if (!EventEntry)
    {
        /* not enough memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* zero event entry */
    RtlZeroMemory(EventEntry, Size);

    /* initialize event entry */
    EventEntry->EventData = EventData;
    EventEntry->NotificationType = EventData->NotificationType;
    EventEntry->EventItem = FoundEventItem;
    EventEntry->EventSet = FoundEventSet;
    EventEntry->FileObject = IoStack->FileObject;

    switch(EventEntry->NotificationType)
    {
        case KSEVENTF_EVENT_HANDLE:
            EventEntry->Object = Object;
            EventEntry->Reserved = 0;
            break;
        case KSEVENTF_SEMAPHORE_HANDLE:
            EventEntry->Object = Object;
            EventEntry->SemaphoreAdjustment = EventData->SemaphoreHandle.Adjustment;
            EventEntry->Reserved = 0;
            break;
        case KSEVENTF_EVENT_OBJECT:
            EventEntry->Object = EventData->EventObject.Event;
            EventEntry->Reserved = EventData->EventObject.Increment;
            break;
        case KSEVENTF_SEMAPHORE_OBJECT:
            EventEntry->Object = EventData->SemaphoreObject.Semaphore;
            EventEntry->SemaphoreAdjustment = EventData->SemaphoreObject.Adjustment;
            EventEntry->Reserved = EventData->SemaphoreObject.Increment;
            break;
        case KSEVENTF_DPC:
            EventEntry->Object = EventData->Dpc.Dpc;
            EventData->Dpc.ReferenceCount = 0;
            break;
        case KSEVENTF_WORKITEM:
            EventEntry->Object = EventData->WorkItem.WorkQueueItem;
            EventEntry->BufferItem = (PKSBUFFER_ITEM)UlongToPtr(EventData->WorkItem.WorkQueueType);
            break;
        case KSEVENTF_KSWORKITEM:
            EventEntry->Object = EventData->KsWorkItem.KsWorkerObject;
            EventEntry->DpcItem = (PKSDPC_ITEM)EventData->KsWorkItem.WorkQueueItem;
            break;
        default:
            /* should not happen */
            ASSERT(0);
    }

    if (FoundEventItem->AddHandler)
    {
        /* now add the event */
        Status = FoundEventItem->AddHandler(Irp, EventData, EventEntry);

        if (!NT_SUCCESS(Status))
        {
            /* discard event entry */
            KsDiscardEvent(EventEntry);
        }
    }
    else
    {
        /* setup context */
        Ctx.List = EventsList;
        Ctx.EventEntry = EventEntry;

         /* add the event */
        (void)KspSynchronizedEventRoutine(EventsFlags, EventsLock, SyncAddEvent, &Ctx);

        Status = STATUS_SUCCESS;
    }

    /* done */
    return Status;
}

/*
    @implemented
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
    return KspEnableEvent(Irp, EventSetsCount, EventSet, EventsList, EventsFlags, EventsLock, NULL, 0);
}

/*
    @implemented
*/
_IRQL_requires_max_(PASSIVE_LEVEL)
KSDDKAPI
NTSTATUS
NTAPI
KsEnableEventWithAllocator(
    _In_ PIRP Irp,
    _In_ ULONG EventSetsCount,
    _In_reads_(EventSetsCount) const KSEVENT_SET* EventSet,
    _Inout_opt_ PLIST_ENTRY EventsList,
    _In_opt_ KSEVENTS_LOCKTYPE EventsFlags,
    _In_opt_ PVOID EventsLock,
    _In_opt_ PFNKSALLOCATOR Allocator,
    _In_opt_ ULONG EventItemSize)
{
    return KspEnableEvent(Irp, EventSetsCount, EventSet, EventsList, EventsFlags, EventsLock, Allocator, EventItemSize);
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

    if (!Ctx || !Ctx->List || !Ctx->FileObject || !Ctx->Irp)
    {
        /* invalid parameter */
        return FALSE;
    }

    /* get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Ctx->Irp);

    /* get event data */
    EventData = (PKSEVENTDATA)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;

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
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsDiscardEvent(
    IN  PKSEVENT_ENTRY EventEntry)
{
    /* sanity check */
    ASSERT(EventEntry->Object);

    if (EventEntry->NotificationType == KSEVENTF_SEMAPHORE_HANDLE || EventEntry->NotificationType == KSEVENTF_EVENT_HANDLE)
    {
        /* release object */
        ObDereferenceObject(EventEntry->Object);
    }

    /* free event entry */
    FreeItem(EventEntry);
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

    if (IsListEmpty(Ctx->List))
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
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsGenerateEvent(
    IN  PKSEVENT_ENTRY EntryEvent)
{
    if (EntryEvent->NotificationType == KSEVENTF_EVENT_HANDLE || EntryEvent->NotificationType == KSEVENTF_EVENT_OBJECT)
    {
        /* signal event */
        KeSetEvent(EntryEvent->Object, EntryEvent->Reserved, FALSE);
    }
    else if (EntryEvent->NotificationType == KSEVENTF_SEMAPHORE_HANDLE || EntryEvent->NotificationType == KSEVENTF_SEMAPHORE_OBJECT)
    {
        /* release semaphore */
        KeReleaseSemaphore(EntryEvent->Object, EntryEvent->Reserved, EntryEvent->SemaphoreAdjustment, FALSE);
    }
    else if (EntryEvent->NotificationType == KSEVENTF_DPC)
    {
        /* increment reference count to indicate dpc is pending */
        InterlockedIncrement((PLONG)&EntryEvent->EventData->Dpc.ReferenceCount);
        /* queue dpc */
        KeInsertQueueDpc((PRKDPC)EntryEvent->Object, NULL, NULL);
    }
    else if (EntryEvent->NotificationType == KSEVENTF_WORKITEM)
    {
        /* queue work item */
        ExQueueWorkItem((PWORK_QUEUE_ITEM)EntryEvent->Object, PtrToUlong(EntryEvent->BufferItem));
    }
    else if (EntryEvent->NotificationType == KSEVENTF_KSWORKITEM)
    {
        /* queue work item of ks worker */
        return KsQueueWorkItem((PKSWORKER)EntryEvent->Object, (PWORK_QUEUE_ITEM)EntryEvent->DpcItem);
    }
    else
    {
        /* unsupported type requested */
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
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
    UNIMPLEMENTED;
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
    ObjectHeader =(PKSIOBJECT_HEADER)IoStack->FileObject->FsContext2;

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
    UNIMPLEMENTED;
}
