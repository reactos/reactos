/*
 * PROJECT:     Local Security Authority Server DLL
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     LSA policy change notifications
 * COPYRIGHT:   Eric Kohl 2018
 */

#include "lsasrv.h"

typedef struct _LSA_NOTIFICATION_ENTRY
{
    LIST_ENTRY Entry;
    POLICY_NOTIFICATION_INFORMATION_CLASS InformationClass;
    CLIENT_ID ClientId;
    HANDLE EventHandle;
    HANDLE MappedEventHandle;
} LSA_NOTIFICATION_ENTRY, *PLSA_NOTIFICATION_ENTRY;

/* GLOBALS *****************************************************************/

static LIST_ENTRY NotificationListHead;
static RTL_RESOURCE NotificationListLock;


/* FUNCTIONS ***************************************************************/

VOID
LsapInitNotificationList(VOID)
{
    InitializeListHead(&NotificationListHead);
    RtlInitializeResource(&NotificationListLock);
}


static
PLSA_NOTIFICATION_ENTRY
LsapGetNotificationEntry(
    PLSA_API_MSG pRequestMsg)
{
    PLIST_ENTRY NotificationEntry;
    PLSA_NOTIFICATION_ENTRY CurrentNotification;

    NotificationEntry = NotificationListHead.Flink;
    while (NotificationEntry != &NotificationListHead)
    {
        CurrentNotification = CONTAINING_RECORD(NotificationEntry, LSA_NOTIFICATION_ENTRY, Entry);

        if ((CurrentNotification->ClientId.UniqueProcess == pRequestMsg->h.ClientId.UniqueProcess) &&
            (CurrentNotification->ClientId.UniqueThread == pRequestMsg->h.ClientId.UniqueThread) &&
            (CurrentNotification->InformationClass == pRequestMsg->PolicyChangeNotify.Request.InformationClass) &&
            (CurrentNotification->EventHandle == pRequestMsg->PolicyChangeNotify.Request.NotificationEventHandle))
            return CurrentNotification;

        NotificationEntry = NotificationEntry->Flink;
    }

    return NULL;
}


static
NTSTATUS
LsapAddNotification(
    PLSA_API_MSG pRequestMsg)
{
    PLSA_NOTIFICATION_ENTRY pEntry;
    HANDLE hProcess = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("LsapAddNotification(%p)\n", pRequestMsg);

    /* Allocate a new notification list entry */
    pEntry = RtlAllocateHeap(RtlGetProcessHeap(),
                             HEAP_ZERO_MEMORY,
                             sizeof(LSA_NOTIFICATION_ENTRY));
    if (pEntry == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    /* Copy the notification data */
    pEntry->InformationClass = pRequestMsg->PolicyChangeNotify.Request.InformationClass;
    pEntry->EventHandle = pRequestMsg->PolicyChangeNotify.Request.NotificationEventHandle;
    pEntry->ClientId = pRequestMsg->h.ClientId;

    /* Open the client process */
    Status = NtOpenProcess(&hProcess,
                           PROCESS_DUP_HANDLE,
                           NULL,
                           pEntry->ClientId.UniqueProcess);
    if (!NT_SUCCESS(Status))
    {
        ERR("NtOpenProcess() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    /* Duplicate the event handle into the current process */
    Status = NtDuplicateObject(hProcess,
                               pEntry->EventHandle,
                               NtCurrentProcess(),
                               &pEntry->MappedEventHandle,
                               0,
                               0,
                               DUPLICATE_SAME_ACCESS);
    if (!NT_SUCCESS(Status))
    {
        ERR("NtDuplicateObject() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    /* Insert the new entry into the notification list */
    InsertHeadList(&NotificationListHead,
                   &pEntry->Entry);

done:
    if (hProcess != NULL)
        NtClose(hProcess);

    if (!NT_SUCCESS(Status))
    {
        if (pEntry != NULL)
            RtlFreeHeap(RtlGetProcessHeap(), 0, pEntry);
    }

    return Status;
}


static
NTSTATUS
LsapRemoveNotification(
    PLSA_API_MSG pRequestMsg)
{
    PLSA_NOTIFICATION_ENTRY pEntry;

    TRACE("LsapRemoveNotification(%p)\n", pRequestMsg);

    pEntry = LsapGetNotificationEntry(pRequestMsg);
    if (pEntry == NULL)
    {
        return STATUS_INVALID_HANDLE;
    }

    /* Remove the  notification entry from the notification list */
    RemoveEntryList(&pEntry->Entry);

    /* Close the mapped event handle */
    NtClose(pEntry->MappedEventHandle);

    /* Release the notification entry */
    RtlFreeHeap(RtlGetProcessHeap(), 0, pEntry);

    return STATUS_SUCCESS;
}


NTSTATUS
LsapRegisterNotification(
    PLSA_API_MSG pRequestMsg)
{
    NTSTATUS Status;

    TRACE("LsapRegisterNotification(%p)\n", pRequestMsg);

    /* Acquire the notification list lock exclusively */
    RtlAcquireResourceExclusive(&NotificationListLock, TRUE);

    if (pRequestMsg->PolicyChangeNotify.Request.Register)
    {
        /* Register the notification event */
        Status = LsapAddNotification(pRequestMsg);
    }
    else
    {
        /* Unregister the notification event */
        Status = LsapRemoveNotification(pRequestMsg);
    }

    /* Release the notification list lock */
    RtlReleaseResource(&NotificationListLock);

    return Status;
}


VOID
LsapNotifyPolicyChange(
    POLICY_NOTIFICATION_INFORMATION_CLASS InformationClass)
{
    PLIST_ENTRY NotificationEntry;
    PLSA_NOTIFICATION_ENTRY CurrentNotification;

    TRACE("LsapNotifyPolicyChange(%lu)\n", InformationClass);

    /* Acquire the notification list lock shared */
    RtlAcquireResourceShared(&NotificationListLock, TRUE);

    NotificationEntry = NotificationListHead.Flink;
    while (NotificationEntry != &NotificationListHead)
    {
        CurrentNotification = CONTAINING_RECORD(NotificationEntry, LSA_NOTIFICATION_ENTRY, Entry);

        if (CurrentNotification->InformationClass == InformationClass)
        {
            TRACE("Notify event %p\n", CurrentNotification->MappedEventHandle);
            NtSetEvent(CurrentNotification->MappedEventHandle, NULL);
        }

        NotificationEntry = NotificationEntry->Flink;
    }

    /* Release the notification list lock */
    RtlReleaseResource(&NotificationListLock);
}

/* EOF */
