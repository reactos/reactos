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
    HANDLE EventHandle;
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
    HANDLE EventHandle,
    POLICY_NOTIFICATION_INFORMATION_CLASS InformationClass)
{
    PLIST_ENTRY NotificationEntry;
    PLSA_NOTIFICATION_ENTRY CurrentNotification;

    NotificationEntry = NotificationListHead.Flink;
    while (NotificationEntry != &NotificationListHead)
    {
        CurrentNotification = CONTAINING_RECORD(NotificationEntry, LSA_NOTIFICATION_ENTRY, Entry);

        if ((CurrentNotification->EventHandle == EventHandle) &&
            (CurrentNotification->InformationClass == InformationClass))
            return CurrentNotification;

        NotificationEntry = NotificationEntry->Flink;
    }

    return NULL;
}


NTSTATUS
LsapRegisterNotification(
    PLSA_API_MSG pRequestMsg)
{
    PLSA_NOTIFICATION_ENTRY pEntry;
    NTSTATUS Status = STATUS_SUCCESS;

    FIXME("LsapRegisterNotification(%p)\n", pRequestMsg);

    /* Acquire the notification list lock exclusively */
    RtlAcquireResourceExclusive(&NotificationListLock, TRUE);

    if (pRequestMsg->PolicyChangeNotify.Request.Register)
    {
        /* Register the notification event */
        pEntry = RtlAllocateHeap(RtlGetProcessHeap(),
                                 HEAP_ZERO_MEMORY,
                                 sizeof(LSA_NOTIFICATION_ENTRY));
        if (pEntry == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }

        pEntry->InformationClass = pRequestMsg->PolicyChangeNotify.Request.InformationClass;
        pEntry->EventHandle = pRequestMsg->PolicyChangeNotify.Request.NotificationEventHandle;

        InsertHeadList(&NotificationListHead,
                       &pEntry->Entry);
    }
    else
    {
        /* Unregister the notification event */
        pEntry = LsapGetNotificationEntry(pRequestMsg->PolicyChangeNotify.Request.NotificationEventHandle,
                                          pRequestMsg->PolicyChangeNotify.Request.InformationClass);
        if (pEntry == NULL)
        {
            Status = STATUS_INVALID_HANDLE;
            goto done;
        }

        RemoveEntryList(&pEntry->Entry);
        RtlFreeHeap(RtlGetProcessHeap(), 0, pEntry);
    }

done:
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

    FIXME("LsapNotifyPolicyChange(%lu)\n", InformationClass);

    /* Acquire the notification list lock shared */
    RtlAcquireResourceShared(&NotificationListLock, TRUE);

    NotificationEntry = NotificationListHead.Flink;
    while (NotificationEntry != &NotificationListHead)
    {
        CurrentNotification = CONTAINING_RECORD(NotificationEntry, LSA_NOTIFICATION_ENTRY, Entry);

        if (CurrentNotification->InformationClass == InformationClass)
        {
            FIXME("Notify event %p\n", CurrentNotification->EventHandle);

        }

        NotificationEntry = NotificationEntry->Flink;
    }

    /* Release the notification list lock */
    RtlReleaseResource(&NotificationListLock);
}

/* EOF */
