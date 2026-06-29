/*
 * PROJECT:     ReactOS NT Layer/System API
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     DLL Load Notification Implementation
 * COPYRIGHT:   Copyright 2024 Ratin Gao <ratin@knsoft.org>
 */

#include "ntdll_vista.h"

/* GLOBALS *******************************************************************/

typedef struct _LDR_DLL_NOTIFICATION_ENTRY
{
    LIST_ENTRY List;
    PLDR_DLL_NOTIFICATION_FUNCTION Callback;
    PVOID Context;
} LDR_DLL_NOTIFICATION_ENTRY, *PLDR_DLL_NOTIFICATION_ENTRY;

static RTL_STATIC_LIST_HEAD(LdrpDllNotificationList);

/* Initialize critical section statically */
static RTL_CRITICAL_SECTION LdrpDllNotificationLock;
static RTL_CRITICAL_SECTION_DEBUG LdrpDllNotificationLockDebug = {
    .CriticalSection = &LdrpDllNotificationLock
};
static RTL_CRITICAL_SECTION LdrpDllNotificationLock = {
    &LdrpDllNotificationLockDebug,
    -1,
    0,
    0,
    0,
    0
};

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
LdrRegisterDllNotification(
    _In_ ULONG Flags,
    _In_ PLDR_DLL_NOTIFICATION_FUNCTION NotificationFunction,
    _In_opt_ PVOID Context,
    _Out_ PVOID *Cookie)
{
    PLDR_DLL_NOTIFICATION_ENTRY NewEntry;

    /* Check input parameters */
    if (Flags != 0 || NotificationFunction == NULL || Cookie == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Allocate new entry and assign input values */
    NewEntry = RtlAllocateHeap(LdrpHeap, 0, sizeof(*NewEntry));
    if (NewEntry == NULL)
    {
        return STATUS_NO_MEMORY;
    }
    NewEntry->Callback = NotificationFunction;
    NewEntry->Context = Context;

    /* Add node to the end of global list */
    RtlEnterCriticalSection(&LdrpDllNotificationLock);
    InsertTailList(&LdrpDllNotificationList, &NewEntry->List);
    RtlLeaveCriticalSection(&LdrpDllNotificationLock);

    /* Cookie is address of the new entry */
    *Cookie = NewEntry;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
LdrUnregisterDllNotification(
    _In_ PVOID Cookie)
{
    NTSTATUS Status = STATUS_DLL_NOT_FOUND;
    PLIST_ENTRY Entry;

    /* Find entry to remove */
    RtlEnterCriticalSection(&LdrpDllNotificationLock);
    for (Entry = LdrpDllNotificationList.Flink;
         Entry != &LdrpDllNotificationList;
         Entry = Entry->Flink)
    {
        if (Entry == Cookie)
        {
            RemoveEntryList(Entry);
            Status = STATUS_SUCCESS;
            break;
        }
    }
    RtlLeaveCriticalSection(&LdrpDllNotificationLock);

    if (NT_SUCCESS(Status))
    {
        RtlFreeHeap(LdrpHeap, 0, Entry);
    }
    return Status;
}

VOID
NTAPI
LdrpSendDllNotifications(
    _In_ PLDR_DATA_TABLE_ENTRY DllEntry,
    _In_ ULONG NotificationReason)
{
    PLIST_ENTRY Entry;
    PLDR_DLL_NOTIFICATION_ENTRY NotificationEntry;
    LDR_DLL_NOTIFICATION_DATA NotificationData;

    /*
     * LDR_DLL_LOADED_NOTIFICATION_DATA and LDR_DLL_UNLOADED_NOTIFICATION_DATA
     * currently are the same. Use C_ASSERT to ensure it, then fill either of them.
     */
#define LdrpAssertDllNotificationDataMember(x)\
    C_ASSERT(FIELD_OFFSET(LDR_DLL_NOTIFICATION_DATA, Loaded.x) ==\
             FIELD_OFFSET(LDR_DLL_NOTIFICATION_DATA, Unloaded.x))

    C_ASSERT(sizeof(NotificationData.Loaded) == sizeof(NotificationData.Unloaded));
    LdrpAssertDllNotificationDataMember(Flags);
    LdrpAssertDllNotificationDataMember(FullDllName);
    LdrpAssertDllNotificationDataMember(BaseDllName);
    LdrpAssertDllNotificationDataMember(DllBase);
    LdrpAssertDllNotificationDataMember(SizeOfImage);

#undef LdrpAssertDllNotificationDataMember

    NotificationData.Loaded.Flags = 0; /* Reserved and always 0, not DllEntry->Flags */
    NotificationData.Loaded.FullDllName = &DllEntry->FullDllName;
    NotificationData.Loaded.BaseDllName = &DllEntry->BaseDllName;
    NotificationData.Loaded.DllBase = DllEntry->DllBase;
    NotificationData.Loaded.SizeOfImage = DllEntry->SizeOfImage;

    /* Send notification to all registered callbacks */
    RtlEnterCriticalSection(&LdrpDllNotificationLock);
    _SEH2_TRY
    {
        for (Entry = LdrpDllNotificationList.Flink;
             Entry != &LdrpDllNotificationList;
             Entry = Entry->Flink)
        {
            NotificationEntry = CONTAINING_RECORD(Entry, LDR_DLL_NOTIFICATION_ENTRY, List);
            NotificationEntry->Callback(NotificationReason,
                                        &NotificationData,
                                        NotificationEntry->Context);
        }
    }
    _SEH2_FINALLY
    {
        RtlLeaveCriticalSection(&LdrpDllNotificationLock);
    }
    _SEH2_END;
}
