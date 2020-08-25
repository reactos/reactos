/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Plug & Play notification functions
 * COPYRIGHT:   2003 Filip Navara (xnavara@volny.cz)
 *              2005-2006 Hervé Poussineau (hpoussin@reactos.org)
 *              2010 Pierre Schweitzer (pierre@reactos.org)
 *              2020 Victor Perevertkin (victor.perevertkin@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* TYPES *******************************************************************/

typedef struct _PNP_NOTIFY_ENTRY
{
    LIST_ENTRY PnpNotifyList;
    PVOID Context;
    PDRIVER_OBJECT DriverObject;
    PDRIVER_NOTIFICATION_CALLBACK_ROUTINE PnpNotificationProc;
    union
    {
        GUID Guid; // for EventCategoryDeviceInterfaceChange
        struct // for EventCategoryTargetDeviceChange
        {
            PFILE_OBJECT FileObject;
            PDEVICE_OBJECT FileObjectPDO;
        };
    };
    IO_NOTIFICATION_EVENT_CATEGORY EventCategory;
    volatile LONG Deleted;
} PNP_NOTIFY_ENTRY, *PPNP_NOTIFY_ENTRY;

KGUARDED_MUTEX PiNotificationInProgressLock;
KSPIN_LOCK PiNotifyAdditionLock;
LIST_ENTRY PiNotifyListHead;
LIST_ENTRY PiAddedNotifyListHead;

/* FUNCTIONS *****************************************************************/

CODE_SEG("INIT")
VOID
PipInitializeNotifications()
{
    KeInitializeGuardedMutex(&PiNotificationInProgressLock);
    KeInitializeSpinLock(&PiNotifyAdditionLock);
    InitializeListHead(&PiNotifyListHead);
    InitializeListHead(&PiAddedNotifyListHead);
}

static
VOID
PipProcessSingleNotification(
    _In_ PPNP_NOTIFY_ENTRY Entry,
    _In_ IO_NOTIFICATION_EVENT_CATEGORY EventCategory,
    _In_ PVOID NotificationStructure,
    _In_opt_ PDEVICE_OBJECT DeviceObject)
{
    PAGED_CODE();

    // remove deleted entries
    if (InterlockedAnd(&Entry->Deleted, 1))
    {
        RemoveEntryList(&Entry->PnpNotifyList);
        ObDereferenceObject(Entry->DriverObject);
        ExFreePoolWithTag(Entry, TAG_PNP_NOTIFY);
        return;
    }

    // skip ones not corresponding to our category
    if (Entry->EventCategory != EventCategory)
    {
        return;
    }

    switch (EventCategory)
    {
        case EventCategoryDeviceInterfaceChange:
        {
            PDEVICE_INTERFACE_CHANGE_NOTIFICATION notifStruct = NotificationStructure;

            if (!IsEqualGUID(&notifStruct->InterfaceClassGuid, &Entry->Guid))
            {
                return;
            }
            break;
        }
        case EventCategoryTargetDeviceChange:
        {
            PTARGET_DEVICE_REMOVAL_NOTIFICATION notifStruct = NotificationStructure;

            if (DeviceObject != Entry->FileObjectPDO)
            {
                return;
            }
            notifStruct->FileObject = Entry->FileObject;
            break;
        }
        default:
        {
            break;
        }
    }

    /* Call entry into new allocated memory */
    DPRINT("IopNotifyPlugPlayNotification(): found suitable callback %p\n", Entry);

    KeReleaseGuardedMutex(&PiNotificationInProgressLock);
    (Entry->PnpNotificationProc)(NotificationStructure, Entry->Context);
    KeAcquireGuardedMutex(&PiNotificationInProgressLock);
}

static
VOID
PipProcessNotification(
    _In_ PVOID NotificationStruct,
    _In_ IO_NOTIFICATION_EVENT_CATEGORY EventCategory,
    _In_opt_ PDEVICE_OBJECT DeviceObject)
{
    PLIST_ENTRY ListEntry;
    PPNP_NOTIFY_ENTRY ChangeEntry;

    PAGED_CODE();

    KeAcquireGuardedMutex(&PiNotificationInProgressLock);

    // first, process the main list
    ListEntry = PiNotifyListHead.Flink;
    while (ListEntry != &PiNotifyListHead)
    {
        ChangeEntry = CONTAINING_RECORD(ListEntry, PNP_NOTIFY_ENTRY, PnpNotifyList);

        ListEntry = ListEntry->Flink; // in case this entry would be deleted

        PipProcessSingleNotification(ChangeEntry, EventCategory, NotificationStruct, DeviceObject);
    }

    // Now process added elements (if any)
    ListEntry = PiAddedNotifyListHead.Flink;
    while ((ListEntry = ExInterlockedRemoveHeadList(&PiAddedNotifyListHead, &PiNotifyAdditionLock)))
    {
        // append the entry to PiNotifyListHead while we're holding PiNotificationInProgressLock
        InsertHeadList(&PiNotifyListHead, ListEntry);

        ChangeEntry = CONTAINING_RECORD(ListEntry, PNP_NOTIFY_ENTRY, PnpNotifyList);

        PipProcessSingleNotification(ChangeEntry, EventCategory, NotificationStruct, DeviceObject);
    }

    KeReleaseGuardedMutex(&PiNotificationInProgressLock);

    ExFreePoolWithTag(NotificationStruct, TAG_PNP_NOTIFY);
}

/**
 * @brief      Delivers the event to all drivers subscribed to
 *             EventCategoryDeviceInterfaceChange
 *
 * @param[in]  Event               The PnP event GUID
 * @param[in]  InterfaceClassGuid  The GUID of an interface class
 * @param[in]  SymbolicLinkName    Pointer to a string identifying the device interface name
 */
VOID
PiNotifyDeviceInterfaceChange(
    _In_ LPCGUID Event,
    _In_ LPCGUID InterfaceClassGuid,
    _In_ PUNICODE_STRING SymbolicLinkName)
{
    PDEVICE_INTERFACE_CHANGE_NOTIFICATION notifStruct;
    notifStruct = ExAllocatePoolWithTag(PagedPool, sizeof(*notifStruct), TAG_PNP_NOTIFY);
    if (!notifStruct)
    {
        return;
    }

    *notifStruct = (DEVICE_INTERFACE_CHANGE_NOTIFICATION) {
        .Version = 1,
        .Size = sizeof(DEVICE_INTERFACE_CHANGE_NOTIFICATION),
        .Event = *Event,
        .InterfaceClassGuid = *InterfaceClassGuid,
        .SymbolicLinkName = SymbolicLinkName
    };

    DPRINT("Delivering a DeviceInterfaceChange PnP event\n");

    PipProcessNotification(notifStruct, EventCategoryDeviceInterfaceChange, NULL);
}


/**
 * @brief      Delivers the event to all drivers subscribed to
 *             EventCategoryHardwareProfileChange PnP event
 *
 * @param[in]  Event  The PnP event GUID
 */
VOID
PiNotifyHardwareProfileChange(
    _In_ LPCGUID Event)
{
    PHWPROFILE_CHANGE_NOTIFICATION notifStruct;
    notifStruct = ExAllocatePoolWithTag(PagedPool, sizeof(*notifStruct), TAG_PNP_NOTIFY);
    if (!notifStruct)
    {
        return;
    }

    *notifStruct = (HWPROFILE_CHANGE_NOTIFICATION) {
        .Version = 1,
        .Size = sizeof(HWPROFILE_CHANGE_NOTIFICATION),
        .Event = *Event
    };

    DPRINT("Delivering a HardwareProfileChange PnP event\n");

    PipProcessNotification(notifStruct, EventCategoryHardwareProfileChange, NULL);
}

/**
 * @brief      Delivers the event to all drivers subscribed to
 *             EventCategoryTargetDeviceChange PnP event
 *
 * @param[in]  Event               The PnP event GUID
 * @param[in]  DeviceObject        The (target) device object
 * @param[in]  CustomNotification  Pointer to a custom notification for GUID_PNP_CUSTOM_NOTIFICATION
 */
VOID
PiNotifyTargetDeviceChange(
    _In_ LPCGUID Event,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PTARGET_DEVICE_CUSTOM_NOTIFICATION CustomNotification)
{
    PVOID notificationStruct;
    // just in case our device is removed during the operation
    ObReferenceObject(DeviceObject);

    // TODO: DEVICE_NODE.TargetDeviceNotify field seems to be the place for registered listeners
    // (not the global list)

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

    PipProcessNotification(notificationStruct, EventCategoryTargetDeviceChange, DeviceObject);

    ObDereferenceObject(DeviceObject);
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @unimplemented
 */
ULONG
NTAPI
IoPnPDeliverServicePowerNotification(ULONG VetoedPowerOperation OPTIONAL,
                                     ULONG PowerNotification,
                                     ULONG Unknown OPTIONAL,
                                     BOOLEAN Synchronous)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoRegisterPlugPlayNotification(IN IO_NOTIFICATION_EVENT_CATEGORY EventCategory,
                               IN ULONG EventCategoryFlags,
                               IN PVOID EventCategoryData OPTIONAL,
                               IN PDRIVER_OBJECT DriverObject,
                               IN PDRIVER_NOTIFICATION_CALLBACK_ROUTINE CallbackRoutine,
                               IN PVOID Context,
                               OUT PVOID *NotificationEntry)
{
    PPNP_NOTIFY_ENTRY Entry;
    PWSTR SymbolicLinkList;
    NTSTATUS Status;
    PAGED_CODE();

    DPRINT("%s(EventCategory 0x%x, EventCategoryFlags 0x%lx, DriverObject %p) called.\n",
           __FUNCTION__,
           EventCategory,
           EventCategoryFlags,
           DriverObject);

    ObReferenceObject(DriverObject);

    /* Try to allocate entry for notification before sending any notification */
    Entry = ExAllocatePoolZero(NonPagedPool, sizeof(PNP_NOTIFY_ENTRY), TAG_PNP_NOTIFY);

    if (!Entry)
    {
        DPRINT("ExAllocatePool() failed\n");
        ObDereferenceObject(DriverObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Entry->PnpNotificationProc = CallbackRoutine;
    Entry->EventCategory = EventCategory;
    Entry->Context = Context;
    Entry->DriverObject = DriverObject;

    switch (EventCategory)
    {
        case EventCategoryDeviceInterfaceChange:
        {
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
                    RtlCopyMemory(&NotificationInfos.Event,
                                  &GUID_DEVICE_INTERFACE_ARRIVAL,
                                  sizeof(GUID));
                    RtlCopyMemory(&NotificationInfos.InterfaceClassGuid,
                                  EventCategoryData,
                                  sizeof(GUID));
                    NotificationInfos.SymbolicLinkName = &SymbolicLinkU;

                    for (SymbolicLink = SymbolicLinkList;
                         *SymbolicLink;
                         SymbolicLink += (SymbolicLinkU.Length / sizeof(WCHAR)) + 1)
                    {
                        RtlInitUnicodeString(&SymbolicLinkU, SymbolicLink);
                        DPRINT("Calling callback routine for %S\n", SymbolicLink);
                        (*CallbackRoutine)(&NotificationInfos, Context);
                    }

                    ExFreePool(SymbolicLinkList);
                }
            }

            RtlCopyMemory(&Entry->Guid, EventCategoryData, sizeof(Entry->Guid));
            break;
        }
        case EventCategoryHardwareProfileChange:
        {
            /* nothing to do */
           break;
        }
        case EventCategoryTargetDeviceChange:
        {
            Entry->FileObject = (PFILE_OBJECT)EventCategoryData;
            Status = IoGetRelatedTargetDevice((PFILE_OBJECT)EventCategoryData, &Entry->FileObjectPDO);

            if (!NT_SUCCESS(Status))
            {
                ExFreePoolWithTag(Entry, TAG_PNP_NOTIFY);
                ObDereferenceObject(DriverObject);
                return Status;
            }
            break;
        }
        default:
        {
            DPRINT1("%s: unknown EventCategory 0x%x UNIMPLEMENTED\n",
                    __FUNCTION__, EventCategory);

            ExFreePoolWithTag(Entry, TAG_PNP_NOTIFY);
            ObDereferenceObject(DriverObject);
            return STATUS_NOT_SUPPORTED;
        }
    }

    ExInterlockedInsertTailList(&PiAddedNotifyListHead,
                                &Entry->PnpNotifyList,
                                &PiNotifyAdditionLock);

    DPRINT("%s returns NotificationEntry %p\n", __FUNCTION__, Entry);

    *NotificationEntry = Entry;

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoUnregisterPlugPlayNotification(IN PVOID NotificationEntry)
{
    PPNP_NOTIFY_ENTRY Entry;
    PAGED_CODE();

    Entry = (PPNP_NOTIFY_ENTRY)NotificationEntry;
    DPRINT("%s(NotificationEntry %p) called\n", __FUNCTION__, Entry);

    InterlockedOr(&Entry->Deleted, 1);

    return STATUS_SUCCESS;
}
