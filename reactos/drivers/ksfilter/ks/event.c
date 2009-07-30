/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/event.c
 * PURPOSE:         KS Event functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "priv.h"


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


/*
    @unimplemented
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
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
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


/*
    @unimplemented
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
    UNIMPLEMENTED;
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
    @unimplemented
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
