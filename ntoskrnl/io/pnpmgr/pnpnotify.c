/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Plug & Play notification functions
 * COPYRIGHT:   Copyright 2003 Filip Navara <xnavara@volny.cz>
 *              Copyright 2005-2006 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2010 Pierre Schweitzer <pierre@reactos.org>
 *              Copyright 2020 Victor Perevertkin <victor.perevertkin@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* DATA **********************************************************************/

KGUARDED_MUTEX PiNotifyTargetDeviceLock;
KGUARDED_MUTEX PiNotifyHwProfileLock;
KGUARDED_MUTEX PiNotifyDeviceInterfaceLock;

_Guarded_by_(PiNotifyHwProfileLock)
LIST_ENTRY PiNotifyHwProfileListHead;

_Guarded_by_(PiNotifyDeviceInterfaceLock)
LIST_ENTRY PiNotifyDeviceInterfaceListHead;

/* TYPES *********************************************************************/

typedef struct _PNP_NOTIFY_ENTRY
{
    LIST_ENTRY PnpNotifyList;
    PVOID Context;
    PDRIVER_OBJECT DriverObject;
    PDRIVER_NOTIFICATION_CALLBACK_ROUTINE PnpNotificationProc;
    union
    {
        GUID Guid; // for EventCategoryDeviceInterfaceChange
        struct
        {
            PFILE_OBJECT FileObject; // for EventCategoryTargetDeviceChange
            PDEVICE_OBJECT DeviceObject;
        };
    };
    IO_NOTIFICATION_EVENT_CATEGORY EventCategory;
    UINT8 RefCount;
    BOOLEAN Deleted;
} PNP_NOTIFY_ENTRY, *PPNP_NOTIFY_ENTRY;

/* FUNCTIONS *****************************************************************/

CODE_SEG("INIT")
VOID
PiInitializeNotifications(VOID)
{
    KeInitializeGuardedMutex(&PiNotifyTargetDeviceLock);
    KeInitializeGuardedMutex(&PiNotifyHwProfileLock);
    KeInitializeGuardedMutex(&PiNotifyDeviceInterfaceLock);
    InitializeListHead(&PiNotifyHwProfileListHead);
    InitializeListHead(&PiNotifyDeviceInterfaceListHead);
}

static
CODE_SEG("PAGE")
VOID
PiDereferencePnpNotifyEntry(
    _In_ PPNP_NOTIFY_ENTRY Entry)
{
    PAGED_CODE();
    ASSERT(Entry->RefCount > 0);

    ObDereferenceObject(Entry->DriverObject);
    Entry->RefCount--;
    if (Entry->RefCount == 0)
    {
        ASSERT(Entry->Deleted);

        RemoveEntryList(&Entry->PnpNotifyList);
        if (Entry->EventCategory == EventCategoryTargetDeviceChange)
        {
            // IopGetRelatedTargetDevice referenced the device upon the notification registration
            ObDereferenceObject(Entry->DeviceObject);
        }
        ExFreePoolWithTag(Entry, TAG_PNP_NOTIFY);
    }
}

static
CODE_SEG("PAGE")
VOID
PiReferencePnpNotifyEntry(
    _In_ PPNP_NOTIFY_ENTRY Entry)
{
    PAGED_CODE();
    ObReferenceObject(Entry->DriverObject);
    Entry->RefCount++;
}

/**
 * @brief Calls PnP notification routine and makes some checks to detect faulty drivers
 */
static
CODE_SEG("PAGE")
VOID
PiCallNotifyProc(
    _In_ PDRIVER_NOTIFICATION_CALLBACK_ROUTINE Proc,
    _In_ PVOID NotificationStructure,
    _In_ PVOID Context)
{
    PAGED_CODE();
#if DBG
    KIRQL oldIrql = KeGetCurrentIrql();
    ULONG oldApcDisable = KeGetCurrentThread()->CombinedApcDisable;
#endif

    Proc(NotificationStructure, Context);

    ASSERT(oldIrql == KeGetCurrentIrql() &&
           oldApcDisable == KeGetCurrentThread()->CombinedApcDisable);
}

static
CODE_SEG("PAGE")
_Requires_lock_held_(Lock)
VOID
PiProcessSingleNotification(
    _In_ PPNP_NOTIFY_ENTRY Entry,
    _In_ PVOID NotificationStructure,
    _In_ PKGUARDED_MUTEX Lock,
    _Out_ PLIST_ENTRY *NextEntry)
{
    PAGED_CODE();

    // the notification may be unregistered inside the procedure
    // thus reference the entry so we may proceed
    PiReferencePnpNotifyEntry(Entry);

    // release the lock because the notification routine has to be called without any
    // limitations regarding APCs
    KeReleaseGuardedMutex(Lock);
    PiCallNotifyProc(Entry->PnpNotificationProc, NotificationStructure, Entry->Context);
    KeAcquireGuardedMutex(Lock);

    // take the next entry link only after the callback finishes
    // the lock is not held there, so Entry may have changed at this point
    *NextEntry = Entry->PnpNotifyList.Flink;
    PiDereferencePnpNotifyEntry(Entry);
}

/**
 * @brief      Delivers the event to all drivers subscribed to
 *             EventCategoryDeviceInterfaceChange
 *
 * @param[in]  Event               The PnP event GUID
 * @param[in]  InterfaceClassGuid  The GUID of an interface class
 * @param[in]  SymbolicLinkName    Pointer to a string identifying the device interface name
 */
CODE_SEG("PAGE")
VOID
PiNotifyDeviceInterfaceChange(
    _In_ LPCGUID Event,
    _In_ LPCGUID InterfaceClassGuid,
    _In_ PUNICODE_STRING SymbolicLinkName)
{
    PAGED_CODE();

    PDEVICE_INTERFACE_CHANGE_NOTIFICATION notifyStruct;
    notifyStruct = ExAllocatePoolWithTag(PagedPool, sizeof(*notifyStruct), TAG_PNP_NOTIFY);
    if (!notifyStruct)
    {
        return;
    }

    *notifyStruct = (DEVICE_INTERFACE_CHANGE_NOTIFICATION) {
        .Version = 1,
        .Size = sizeof(DEVICE_INTERFACE_CHANGE_NOTIFICATION),
        .Event = *Event,
        .InterfaceClassGuid = *InterfaceClassGuid,
        .SymbolicLinkName = SymbolicLinkName
    };

    DPRINT("Delivering a DeviceInterfaceChange PnP event\n");

    KeAcquireGuardedMutex(&PiNotifyDeviceInterfaceLock);

    PLIST_ENTRY entry = PiNotifyDeviceInterfaceListHead.Flink;
    while (entry != &PiNotifyDeviceInterfaceListHead)
    {
        PPNP_NOTIFY_ENTRY nEntry = CONTAINING_RECORD(entry, PNP_NOTIFY_ENTRY, PnpNotifyList);

        if (!IsEqualGUID(&notifyStruct->InterfaceClassGuid, &nEntry->Guid))
        {
            entry = entry->Flink;
            continue;
        }

        PiProcessSingleNotification(nEntry, notifyStruct, &PiNotifyDeviceInterfaceLock, &entry);
    }

    KeReleaseGuardedMutex(&PiNotifyDeviceInterfaceLock);
    ExFreePoolWithTag(notifyStruct, TAG_PNP_NOTIFY);
}


/**
 * @brief      Delivers the event to all drivers subscribed to
 *             EventCategoryHardwareProfileChange PnP event
 *
 * @param[in]  Event  The PnP event GUID
 */
CODE_SEG("PAGE")
VOID
PiNotifyHardwareProfileChange(
    _In_ LPCGUID Event)
{
    PAGED_CODE();

    PHWPROFILE_CHANGE_NOTIFICATION notifyStruct;
    notifyStruct = ExAllocatePoolWithTag(PagedPool, sizeof(*notifyStruct), TAG_PNP_NOTIFY);
    if (!notifyStruct)
    {
        return;
    }

    *notifyStruct = (HWPROFILE_CHANGE_NOTIFICATION) {
        .Version = 1,
        .Size = sizeof(HWPROFILE_CHANGE_NOTIFICATION),
        .Event = *Event
    };

    DPRINT("Delivering a HardwareProfileChange PnP event\n");

    KeAcquireGuardedMutex(&PiNotifyHwProfileLock);

    PLIST_ENTRY entry = PiNotifyHwProfileListHead.Flink;
    while (entry != &PiNotifyHwProfileListHead)
    {
        PPNP_NOTIFY_ENTRY nEntry = CONTAINING_RECORD(entry, PNP_NOTIFY_ENTRY, PnpNotifyList);

        PiProcessSingleNotification(nEntry, notifyStruct, &PiNotifyHwProfileLock, &entry);
    }

    KeReleaseGuardedMutex(&PiNotifyHwProfileLock);
    ExFreePoolWithTag(notifyStruct, TAG_PNP_NOTIFY);
}

/**
 * @brief      Delivers the event to all drivers subscribed to
 *             EventCategoryTargetDeviceChange PnP event
 *
 * @param[in]  Event               The PnP event GUID
 * @param[in]  DeviceObject        The (target) device object
 * @param[in]  CustomNotification  Pointer to a custom notification for GUID_PNP_CUSTOM_NOTIFICATION
 */
CODE_SEG("PAGE")
VOID
PiNotifyTargetDeviceChange(
    _In_ LPCGUID Event,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PTARGET_DEVICE_CUSTOM_NOTIFICATION CustomNotification)
{
    PAGED_CODE();

    PVOID notificationStruct;
    // just in case our device is removed during the operation
    ObReferenceObject(DeviceObject);

    PDEVICE_NODE deviceNode = IopGetDeviceNode(DeviceObject);
    ASSERT(deviceNode);

    if (!IsEqualGUID(Event, &GUID_PNP_CUSTOM_NOTIFICATION))
    {
        PTARGET_DEVICE_REMOVAL_NOTIFICATION notifStruct;
        notifStruct = ExAllocatePoolWithTag(PagedPool, sizeof(*notifStruct), TAG_PNP_NOTIFY);
        if (!notifStruct)
        {
            return;
        }

        *notifStruct = (TARGET_DEVICE_REMOVAL_NOTIFICATION) {
            .Version = 1,
            .Size = sizeof(TARGET_DEVICE_REMOVAL_NOTIFICATION),
            .Event = *Event
        };

        notificationStruct = notifStruct;

        DPRINT("Delivering a (non-custom) TargetDeviceChange PnP event\n");
    }
    else
    {
        ASSERT(CustomNotification);
        // assuming everythng else is correct

        notificationStruct = CustomNotification;

        DPRINT("Delivering a (custom) TargetDeviceChange PnP event\n");
    }

    KeAcquireGuardedMutex(&PiNotifyTargetDeviceLock);

    PLIST_ENTRY entry = deviceNode->TargetDeviceNotify.Flink;
    while (entry != &deviceNode->TargetDeviceNotify)
    {
        PPNP_NOTIFY_ENTRY nEntry = CONTAINING_RECORD(entry, PNP_NOTIFY_ENTRY, PnpNotifyList);

        // put the file object from our saved entry to this particular notification's struct
        ((PTARGET_DEVICE_REMOVAL_NOTIFICATION)notificationStruct)->FileObject = nEntry->FileObject;
        // so you don't need to look at the definition ;)
        C_ASSERT(FIELD_OFFSET(TARGET_DEVICE_REMOVAL_NOTIFICATION, FileObject)
                 == FIELD_OFFSET(TARGET_DEVICE_CUSTOM_NOTIFICATION, FileObject));

        PiProcessSingleNotification(nEntry, notificationStruct, &PiNotifyTargetDeviceLock, &entry);
    }

    KeReleaseGuardedMutex(&PiNotifyTargetDeviceLock);
    if (notificationStruct != CustomNotification)
        ExFreePoolWithTag(notificationStruct, TAG_PNP_NOTIFY);
    ObDereferenceObject(DeviceObject);
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @unimplemented
 */
ULONG
NTAPI
IoPnPDeliverServicePowerNotification(
    _In_ ULONG VetoedPowerOperation,
    _In_ ULONG PowerNotificationCode,
    _In_ ULONG PowerNotificationData,
    _In_ BOOLEAN Synchronous)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @implemented
 */
CODE_SEG("PAGE")
NTSTATUS
NTAPI
IoRegisterPlugPlayNotification(
    _In_ IO_NOTIFICATION_EVENT_CATEGORY EventCategory,
    _In_ ULONG EventCategoryFlags,
    _In_opt_ PVOID EventCategoryData,
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDRIVER_NOTIFICATION_CALLBACK_ROUTINE CallbackRoutine,
    _Inout_opt_ PVOID Context,
    _Out_ PVOID *NotificationEntry)
{
    PPNP_NOTIFY_ENTRY Entry;
    PWSTR SymbolicLinkList;
    NTSTATUS Status;
    PAGED_CODE();

    DPRINT("%s(EventCategory 0x%x, EventCategoryFlags 0x%lx, DriverObject %p) called.\n",
           __FUNCTION__, EventCategory, EventCategoryFlags, DriverObject);

    ObReferenceObject(DriverObject);

    /* Try to allocate entry for notification before sending any notification */
    Entry = ExAllocatePoolWithTag(PagedPool, sizeof(PNP_NOTIFY_ENTRY), TAG_PNP_NOTIFY);
    if (!Entry)
    {
        DPRINT("ExAllocatePool() failed\n");
        ObDereferenceObject(DriverObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *Entry = (PNP_NOTIFY_ENTRY) {
        .PnpNotificationProc = CallbackRoutine,
        .Context = Context,
        .DriverObject = DriverObject,
        .EventCategory = EventCategory,
        .RefCount = 1
    };

    switch (EventCategory)
    {
        case EventCategoryDeviceInterfaceChange:
        {
            Entry->Guid = *(LPGUID)EventCategoryData;

            // first register the notification
            KeAcquireGuardedMutex(&PiNotifyDeviceInterfaceLock);
            InsertTailList(&PiNotifyDeviceInterfaceListHead, &Entry->PnpNotifyList);
            KeReleaseGuardedMutex(&PiNotifyDeviceInterfaceLock);

            // then process existing interfaces if asked
            if (EventCategoryFlags & PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES)
            {
                DEVICE_INTERFACE_CHANGE_NOTIFICATION NotificationInfos;
                UNICODE_STRING SymbolicLinkU;
                PWSTR SymbolicLink;

                Status = IoGetDeviceInterfaces((LPGUID)EventCategoryData,
                                               NULL, /* PhysicalDeviceObject OPTIONAL */
                                               0, /* Flags */
                                               &SymbolicLinkList);
                if (NT_SUCCESS(Status))
                {
                    /* Enumerate SymbolicLinkList */
                    NotificationInfos.Version = 1;
                    NotificationInfos.Size = sizeof(DEVICE_INTERFACE_CHANGE_NOTIFICATION);
                    NotificationInfos.Event = GUID_DEVICE_INTERFACE_ARRIVAL;
                    NotificationInfos.InterfaceClassGuid = *(LPGUID)EventCategoryData;
                    NotificationInfos.SymbolicLinkName = &SymbolicLinkU;

                    for (SymbolicLink = SymbolicLinkList;
                         *SymbolicLink;
                         SymbolicLink += (SymbolicLinkU.Length / sizeof(WCHAR)) + 1)
                    {
                        RtlInitUnicodeString(&SymbolicLinkU, SymbolicLink);
                        DPRINT("Calling callback routine for %S\n", SymbolicLink);
                        PiCallNotifyProc(CallbackRoutine, &NotificationInfos, Context);
                    }

                    ExFreePool(SymbolicLinkList);
                }
            }
            break;
        }
        case EventCategoryHardwareProfileChange:
        {
            KeAcquireGuardedMutex(&PiNotifyHwProfileLock);
            InsertTailList(&PiNotifyHwProfileListHead, &Entry->PnpNotifyList);
            KeReleaseGuardedMutex(&PiNotifyHwProfileLock);
            break;
        }
        case EventCategoryTargetDeviceChange:
        {
            PDEVICE_NODE deviceNode;
            Entry->FileObject = (PFILE_OBJECT)EventCategoryData;

            // NOTE: the device node's PDO is referenced here
            Status = IopGetRelatedTargetDevice(Entry->FileObject, &deviceNode);
            if (!NT_SUCCESS(Status))
            {
                ObDereferenceObject(DriverObject);
                ExFreePoolWithTag(Entry, TAG_PNP_NOTIFY);
                return Status;
            }
            // save it so we can dereference it later
            Entry->DeviceObject = deviceNode->PhysicalDeviceObject;

            // each DEVICE_NODE has its own registered notifications list
            KeAcquireGuardedMutex(&PiNotifyTargetDeviceLock);
            InsertTailList(&deviceNode->TargetDeviceNotify, &Entry->PnpNotifyList);
            KeReleaseGuardedMutex(&PiNotifyTargetDeviceLock);
            break;
        }
        default:
        {
            DPRINT1("%s: unknown EventCategory 0x%x UNIMPLEMENTED\n",
                    __FUNCTION__, EventCategory);

            ObDereferenceObject(DriverObject);
            ExFreePoolWithTag(Entry, TAG_PNP_NOTIFY);
            return STATUS_NOT_SUPPORTED;
        }
    }

    DPRINT("%s returns NotificationEntry %p\n", __FUNCTION__, Entry);

    *NotificationEntry = Entry;

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
CODE_SEG("PAGE")
NTSTATUS
NTAPI
IoUnregisterPlugPlayNotification(
    _In_ PVOID NotificationEntry)
{
    PPNP_NOTIFY_ENTRY Entry = NotificationEntry;
    PKGUARDED_MUTEX Lock;

    PAGED_CODE();

    DPRINT("%s(NotificationEntry %p) called\n", __FUNCTION__, Entry);

    switch (Entry->EventCategory)
    {
        case EventCategoryDeviceInterfaceChange:
            Lock = &PiNotifyDeviceInterfaceLock;
            break;
        case EventCategoryHardwareProfileChange:
            Lock = &PiNotifyHwProfileLock;
            break;
        case EventCategoryTargetDeviceChange:
            Lock = &PiNotifyTargetDeviceLock;
            break;
        default:
            UNREACHABLE;
            return STATUS_NOT_SUPPORTED;
    }

    KeAcquireGuardedMutex(Lock);
    if (!Entry->Deleted)
    {
        Entry->Deleted = TRUE; // so it can't be unregistered two times
        PiDereferencePnpNotifyEntry(Entry);
    }
    else
    {
        DPRINT1("IoUnregisterPlugPlayNotification called two times for 0x%p\n", NotificationEntry);
    }
    KeReleaseGuardedMutex(Lock);

    return STATUS_SUCCESS;
}
