/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/swenum.c
 * PURPOSE:         KS Software BUS functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "precomp.h"

#include <stdio.h>
#include <swenum.h>

#define NDEBUG
#include <debug.h>

LONG KsDeviceCount = 0;

typedef NTSTATUS (NTAPI *PKSP_BUS_ENUM_CALLBACK)(
    IN PHANDLE hKey,
    IN PBUS_ENUM_DEVICE_EXTENSION BusDeviceExtension,
    IN PBUS_DEVICE_ENTRY DummyEntry,
    IN LPWSTR RootName,
    IN LPWSTR DirectoryName);

NTSTATUS
KspCreatePDO(
    IN PBUS_ENUM_DEVICE_EXTENSION BusDeviceExtension,
    IN PBUS_DEVICE_ENTRY DeviceEntry,
    OUT PDEVICE_OBJECT * OutDeviceObject)
{
    PDEVICE_OBJECT DeviceObject;
    WCHAR Buffer[50];
    ULONG CurDeviceId;
    UNICODE_STRING DeviceName;
    NTSTATUS Status;
    PCOMMON_DEVICE_EXTENSION DeviceExtension;

    /* increment device count */
    CurDeviceId = InterlockedIncrement(&KsDeviceCount);

    /* generate new device id */
    swprintf(Buffer, L"\\Device\\KSENUM%08x", CurDeviceId);

    /* initialize new device name */
    RtlInitUnicodeString(&DeviceName, Buffer);

    /* create new device object */
    Status = IoCreateDevice(BusDeviceExtension->BusDeviceObject->DriverObject, sizeof(PVOID), &DeviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);

    /* check for success */
    if (!NT_SUCCESS(Status))
    {
        /* failed to create pdo */
        DPRINT("KspCreatePDO failed with %x\n", Status);
        return Status;
    }

    /* now allocate device extension */
    DeviceExtension = (PCOMMON_DEVICE_EXTENSION)AllocateItem(NonPagedPool, sizeof(COMMON_DEVICE_EXTENSION));
    if (!DeviceExtension)
    {
        /* no memory */
        IoDeleteDevice(DeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* store device extension */
    *((PVOID*)DeviceObject->DeviceExtension) = DeviceExtension;

    /* initialize device extension */
    DeviceExtension->IsBus = FALSE;
    DeviceExtension->DeviceObject = DeviceObject;
    DeviceExtension->DeviceEntry = DeviceEntry;
    DeviceExtension->BusDeviceExtension = BusDeviceExtension;

    /* not started yet*/
    DeviceEntry->DeviceState = NotStarted;

    /* get current time */
    KeQueryTickCount(&DeviceEntry->TimeCreated);

    /* setup flags */
    DeviceObject->Flags |= DO_POWER_PAGABLE;
    DeviceObject->Flags &= ~ DO_DEVICE_INITIALIZING;
    /* TODO: fire time when expired */

    *OutDeviceObject = DeviceObject;

    return STATUS_SUCCESS;
}

NTSTATUS
KspRegisterDeviceAssociation(
    IN PBUS_ENUM_DEVICE_EXTENSION BusDeviceExtension,
    IN PBUS_DEVICE_ENTRY DeviceEntry,
    IN OUT PBUS_INSTANCE_ENTRY BusInstanceEntry)
{
    NTSTATUS Status;
    UNICODE_STRING ReferenceString;

    /* initialize reference string */
    RtlInitUnicodeString(&ReferenceString, DeviceEntry->DeviceName);

    /* register device interface */
    Status = IoRegisterDeviceInterface(BusDeviceExtension->PhysicalDeviceObject, &BusInstanceEntry->InterfaceGuid, &ReferenceString, &BusInstanceEntry->SymbolicLink);

    /* check for success */
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        return Status;
    }

    /* now enable the interface */
    Status = IoSetDeviceInterfaceState(&BusInstanceEntry->SymbolicLink, TRUE);

    /* check for success */
    if (!NT_SUCCESS(Status))
    {
        /* failed, free memory */
        FreeItem(BusInstanceEntry->SymbolicLink.Buffer);
        return Status;
    }

    DPRINT("Registered DeviceInterface %wZ\n", &BusInstanceEntry->SymbolicLink);


    /* done */
    return Status;
}

VOID
KspRemoveDeviceAssociations(
    IN PBUS_DEVICE_ENTRY DeviceEntry)
{
    PLIST_ENTRY Entry;
    PBUS_INSTANCE_ENTRY CurEntry;

    /* remove all entries */
    Entry = DeviceEntry->DeviceInterfaceList.Flink;

    while(Entry != &DeviceEntry->DeviceInterfaceList)
    {
         /* get offset */
         CurEntry = (PBUS_INSTANCE_ENTRY)CONTAINING_RECORD(Entry, BUS_INSTANCE_ENTRY, Entry);

         /* sanity check */
         ASSERT(CurEntry->SymbolicLink.Buffer);

         /* de-register interface */
         IoSetDeviceInterfaceState(&CurEntry->SymbolicLink, FALSE);

         /* free symbolic link buffer */
         FreeItem(CurEntry->SymbolicLink.Buffer);

         /* remove entry from list */
         RemoveEntryList(Entry);

         /* move to next entry */
         Entry = Entry->Flink;

         /* free entry */
         FreeItem(CurEntry);
    }
}

NTSTATUS
KspEnumerateBusRegistryKeys(
    IN HANDLE hKey,
    IN LPWSTR ReferenceString,
    IN PKSP_BUS_ENUM_CALLBACK Callback,
    IN PBUS_ENUM_DEVICE_EXTENSION BusDeviceExtension,
    IN PBUS_DEVICE_ENTRY DeviceEntry)
{
    UNICODE_STRING String;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hNewKey;
    NTSTATUS Status;
    ULONG ResultLength, Index, KeyInfoLength;
    KEY_FULL_INFORMATION KeyInformation;
    PKEY_BASIC_INFORMATION KeyInfo;

    /* initialize key name */
    RtlInitUnicodeString(&String, ReferenceString);

    /* initialize object attributes */
    InitializeObjectAttributes(&ObjectAttributes, &String, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, hKey, NULL);

    /* open new key */
    Status = ZwOpenKey(&hNewKey, GENERIC_READ, &ObjectAttributes);

    /* check for success */
    if (!NT_SUCCESS(Status))
    {
        /* failed to open key */

        return Status;
    }

    /* query key stats */
    Status = ZwQueryKey(hNewKey, KeyFullInformation, &KeyInformation, sizeof(KeyInformation), &ResultLength);

    if (!NT_SUCCESS(Status))
    {
        /* close key */
        ZwClose(hNewKey);

        /* done */
        return Status;
    }

    /* calculate key info length */
    KeyInfoLength = KeyInformation.MaxNameLen + sizeof(KEY_BASIC_INFORMATION) + 1 * sizeof(WCHAR);

    /* allocate buffer */
    KeyInfo = (PKEY_BASIC_INFORMATION)AllocateItem(NonPagedPool, KeyInfoLength);
    if (!KeyInfo)
    {

        /* no memory */
        ZwClose(hNewKey);

        /* done */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* enumerate all keys */
    for(Index = 0; Index < KeyInformation.SubKeys; Index++)
    {

        /* query sub key */
        Status = ZwEnumerateKey(hNewKey, Index, KeyBasicInformation, (PVOID)KeyInfo, KeyInfoLength, &ResultLength);

        /* check for success */
        if (NT_SUCCESS(Status))
        {
            /* perform callback */
            Status = Callback(hNewKey, BusDeviceExtension, DeviceEntry, KeyInfo->Name, ReferenceString);

            /* should enumeration stop */
            if (!NT_SUCCESS(Status))
                break;
        }
    }

    /* free info buffer */
    FreeItem(KeyInfo);

    /* close key */
    ZwClose(hNewKey);

    /* done */
    return Status;
}

NTSTATUS
NTAPI
KspCreateDeviceAssociation(
    IN PHANDLE hKey,
    IN PBUS_ENUM_DEVICE_EXTENSION BusDeviceExtension,
    IN PBUS_DEVICE_ENTRY DeviceEntry,
    IN LPWSTR InterfaceString,
    IN LPWSTR ReferenceString)
{
    GUID InterfaceGUID;
    NTSTATUS Status;
    PLIST_ENTRY Entry;
    PBUS_INSTANCE_ENTRY CurEntry;
    UNICODE_STRING DeviceName;

    /* initialize interface string */
    RtlInitUnicodeString(&DeviceName, InterfaceString);

    /* first convert device name to guid */
    RtlGUIDFromString(&DeviceName, &InterfaceGUID);

    /* check if the device is already present */
    Entry = DeviceEntry->DeviceInterfaceList.Flink;
    DPRINT("KspCreateDeviceAssociation ReferenceString %S\n", ReferenceString);
    DPRINT("KspCreateDeviceAssociation InterfaceString %S\n", InterfaceString);

    while(Entry != &DeviceEntry->DeviceInterfaceList)
    {
         /* get offset */
         CurEntry = (PBUS_INSTANCE_ENTRY)CONTAINING_RECORD(Entry, BUS_INSTANCE_ENTRY, Entry);

         if (IsEqualGUIDAligned(&CurEntry->InterfaceGuid, &InterfaceGUID))
         {
             /* entry already exists */
             return STATUS_SUCCESS;
         }

         /* move to next entry */
         Entry = Entry->Flink;
    }

    /* time to allocate new entry */
    CurEntry = (PBUS_INSTANCE_ENTRY)AllocateItem(NonPagedPool, sizeof(BUS_INSTANCE_ENTRY));

    if (!CurEntry)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* store guid */
    RtlMoveMemory(&CurEntry->InterfaceGuid, &InterfaceGUID, sizeof(GUID));

    /* now register the association */
    Status = KspRegisterDeviceAssociation(BusDeviceExtension, DeviceEntry, CurEntry);

    /* check for success */
    if (NT_SUCCESS(Status))
    {
        /* store entry */
        InsertTailList(&DeviceEntry->DeviceInterfaceList, &CurEntry->Entry);
    }
    else
    {
        /* failed to associated device */
        FreeItem(CurEntry);
    }

     /* done */
     return Status;
}

NTSTATUS
NTAPI
KspCreateDeviceReference(
    IN PHANDLE hKey,
    IN PBUS_ENUM_DEVICE_EXTENSION BusDeviceExtension,
    IN PBUS_DEVICE_ENTRY DummyEntry,
    IN LPWSTR InterfaceId,
    IN LPWSTR DeviceId)
{
    LPWSTR DeviceName;
    SIZE_T Length;
    PLIST_ENTRY Entry;
    PBUS_DEVICE_ENTRY DeviceEntry = NULL; /* GCC warning */
    BOOLEAN ItemExists = FALSE;
    UNICODE_STRING String;
    NTSTATUS Status;
    KIRQL OldLevel;

    /* first construct device name & reference guid */
    Length = wcslen(DeviceId) + wcslen(InterfaceId);

    /* append '&' and null byte */
    Length += 2;

    /* allocate device name */
    DeviceName = AllocateItem(NonPagedPool, Length * sizeof(WCHAR));

    if (!DeviceName)
    {
        /* not enough memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* construct device name */
    wcscpy(DeviceName, DeviceId);
    wcscat(DeviceName, L"&");
    wcscat(DeviceName, InterfaceId);

    /* scan list and check if it is already present */
    Entry = BusDeviceExtension->Common.Entry.Flink;

    while(Entry != &BusDeviceExtension->Common.Entry)
    {
        /* get real offset */
        DeviceEntry = (PBUS_DEVICE_ENTRY)CONTAINING_RECORD(Entry, BUS_DEVICE_ENTRY, Entry);

        /* check if name matches */
        if (!_wcsicmp(DeviceEntry->DeviceName, DeviceName))
        {
            /* item already exists */
            ItemExists = TRUE;
            break;
        }

        /* move to next entry */
        Entry = Entry->Flink;
    }

    if (!ItemExists)
    {
        /* allocate new device entry */
        DeviceEntry = AllocateItem(NonPagedPool, sizeof(BUS_DEVICE_ENTRY));
        if (!DeviceEntry)
        {
            /* no memory */
            FreeItem(DeviceName);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* initialize device entry */
        InitializeListHead(&DeviceEntry->DeviceInterfaceList);
        InitializeListHead(&DeviceEntry->IrpPendingList);

        /* copy device guid */
        RtlInitUnicodeString(&String, DeviceId);
        RtlGUIDFromString(&String, &DeviceEntry->DeviceGuid);

        /* copy device names */
        DeviceEntry->DeviceName = DeviceName;
        DeviceEntry->Instance = (DeviceName + wcslen(DeviceId) + 1);

        /* copy name */
        DeviceEntry->BusId = AllocateItem(NonPagedPool, (wcslen(DeviceId) + 1) * sizeof(WCHAR));
        if (!DeviceEntry->BusId)
        {
            /* no memory */
            FreeItem(DeviceName);
            FreeItem(DeviceEntry);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        wcscpy(DeviceEntry->BusId, DeviceId);
    }

    /* now enumerate the interfaces */
    Status = KspEnumerateBusRegistryKeys(hKey, InterfaceId, KspCreateDeviceAssociation, BusDeviceExtension, DeviceEntry);

    /* check if list is empty */
    if (IsListEmpty(&DeviceEntry->DeviceInterfaceList))
    {
        /* invalid device settings */
        FreeItem(DeviceEntry->BusId);
        FreeItem(DeviceEntry->DeviceName);
        FreeItem(DeviceEntry);

        ASSERT(ItemExists == FALSE);

        return STATUS_INVALID_DEVICE_STATE;
    }

    /* check if enumeration failed */
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        KspRemoveDeviceAssociations(DeviceEntry);
        FreeItem(DeviceEntry->BusId);
        FreeItem(DeviceEntry->DeviceName);
        FreeItem(DeviceEntry);

        ASSERT(ItemExists == FALSE);

        /* done */
        return Status;
    }

    if (!ItemExists)
    {
        /* acquire lock */
        KeAcquireSpinLock(&BusDeviceExtension->Lock, &OldLevel);

        /* successfully initialized entry */
        InsertTailList(&BusDeviceExtension->Common.Entry, &DeviceEntry->Entry);

        /* release lock */
        KeReleaseSpinLock(&BusDeviceExtension->Lock, OldLevel);
    }

    /* done */
    return Status;
}

NTSTATUS
NTAPI
KspCreateDeviceReferenceTrampoline(
    IN PHANDLE hKey,
    IN PBUS_ENUM_DEVICE_EXTENSION BusDeviceExtension,
    IN PBUS_DEVICE_ENTRY DummyEntry,
    IN LPWSTR DeviceCategory,
    IN LPWSTR ReferenceString)
{
    return KspEnumerateBusRegistryKeys(hKey, DeviceCategory, KspCreateDeviceReference, BusDeviceExtension, DummyEntry);
}


NTSTATUS
KspOpenBusRegistryKey(
    IN PBUS_ENUM_DEVICE_EXTENSION BusDeviceExtension,
    OUT PHANDLE hKey)
{
    OBJECT_ATTRIBUTES ObjectAttributes;

    /* initialize object attributes */
    InitializeObjectAttributes(&ObjectAttributes, &BusDeviceExtension->ServicePath, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

    return ZwCreateKey(hKey, GENERIC_READ | GENERIC_WRITE, &ObjectAttributes, 0, NULL, 0, NULL);
}

NTSTATUS
KspScanBus(
    IN PBUS_ENUM_DEVICE_EXTENSION BusDeviceExtension)
{
    HANDLE hKey;
    NTSTATUS Status;

    /* first open key */
    Status = KspOpenBusRegistryKey(BusDeviceExtension, &hKey);

    /* check for success */
    if (!NT_SUCCESS(Status))
    {
        /* no success */

        return Status;
    }

    /* TODO clear reference marks */

    /* construct device entries */
    Status = KspEnumerateBusRegistryKeys(hKey, NULL, KspCreateDeviceReferenceTrampoline, BusDeviceExtension, NULL);

    /* TODO: delete unreferenced devices */

    /* close handle */
    ZwClose(hKey);

    /* done */
    return Status;
}


NTSTATUS
NTAPI
KspBusQueryReferenceString(
    IN PVOID Context,
    IN OUT PWCHAR *String)
{
    LPWSTR Name;
    SIZE_T Length;
    PBUS_ENUM_DEVICE_EXTENSION BusDeviceExtension = (PBUS_ENUM_DEVICE_EXTENSION)Context;

    /* sanity checks */
    ASSERT(BusDeviceExtension);
    ASSERT(BusDeviceExtension->BusIdentifier);

    /* calculate length */
    Length = wcslen(BusDeviceExtension->BusIdentifier) + 1;

    /* allocate buffer */
    Name = AllocateItem(PagedPool, Length * sizeof(WCHAR));

    if (!Name)
    {
        /* failed to allocate buffer */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* copy buffer */
    wcscpy(Name, BusDeviceExtension->BusIdentifier);

    /* store result */
    *String = Name;

    /* done */
    return STATUS_SUCCESS;
}

VOID
NTAPI
KspBusDeviceReference(
    IN PVOID Context)
{
    PCOMMON_DEVICE_EXTENSION ChildDeviceExtension = (PCOMMON_DEVICE_EXTENSION)Context;

    /* reference count */
    InterlockedIncrement((PLONG)&ChildDeviceExtension->DeviceReferenceCount);
}

VOID
NTAPI
KspBusDeviceDereference(
    IN PVOID Context)
{
    PCOMMON_DEVICE_EXTENSION ChildDeviceExtension = (PCOMMON_DEVICE_EXTENSION)Context;

    /* reference count */
    InterlockedDecrement((PLONG)&ChildDeviceExtension->DeviceReferenceCount);
}

VOID
NTAPI
KspBusReferenceDeviceObject(
    IN PVOID Context)
{
    PCOMMON_DEVICE_EXTENSION ChildDeviceExtension = (PCOMMON_DEVICE_EXTENSION)Context;

    /* reference count */
    InterlockedIncrement((PLONG)&ChildDeviceExtension->DeviceObjectReferenceCount);
}

VOID
NTAPI
KspBusDereferenceDeviceObject(
    IN PVOID Context)
{
    PCOMMON_DEVICE_EXTENSION ChildDeviceExtension = (PCOMMON_DEVICE_EXTENSION)Context;

    /* reference count */
    InterlockedDecrement((PLONG)&ChildDeviceExtension->DeviceObjectReferenceCount);
}

NTSTATUS
KspQueryBusDeviceInterface(
    IN PCOMMON_DEVICE_EXTENSION ChildDeviceExtension,
    IN PIRP Irp)
{
    PBUS_INTERFACE_SWENUM Interface;
    PIO_STACK_LOCATION IoStack;

    /* get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* sanity checks */
    ASSERT(IoStack->Parameters.QueryInterface.Size == sizeof(BUS_INTERFACE_SWENUM));
    ASSERT(IoStack->Parameters.QueryInterface.Interface);

    /* fill in interface */
    Interface = (PBUS_INTERFACE_SWENUM)IoStack->Parameters.QueryInterface.Interface;
    Interface->Interface.Size = sizeof(BUS_INTERFACE_SWENUM);
    Interface->Interface.Version = BUS_INTERFACE_SWENUM_VERSION;
    Interface->Interface.Context = ChildDeviceExtension;
    Interface->Interface.InterfaceReference = KspBusDeviceReference;
    Interface->Interface.InterfaceDereference = KspBusDeviceDereference;
    Interface->ReferenceDeviceObject = KspBusReferenceDeviceObject;
    Interface->DereferenceDeviceObject = KspBusDereferenceDeviceObject;
    Interface->QueryReferenceString = KspBusQueryReferenceString;

    return STATUS_SUCCESS;
}

NTSTATUS
KspEnableBusDeviceInterface(
    PBUS_DEVICE_ENTRY DeviceEntry,
    BOOLEAN bEnable)
{
    PLIST_ENTRY Entry;
    PBUS_INSTANCE_ENTRY InstanceEntry;
    NTSTATUS Status = STATUS_SUCCESS;

    /* enable now all interfaces */
    Entry = DeviceEntry->DeviceInterfaceList.Flink;

    while(Entry != &DeviceEntry->DeviceInterfaceList)
    {
        /* get bus instance entry */
        InstanceEntry = (PBUS_INSTANCE_ENTRY)CONTAINING_RECORD(Entry, BUS_INSTANCE_ENTRY, Entry);
        DPRINT("Enabling %u %wZ Irql %u\n", bEnable, &InstanceEntry->SymbolicLink, KeGetCurrentIrql());

        /* set interface state */
        Status = IoSetDeviceInterfaceState(&InstanceEntry->SymbolicLink, bEnable);

        if (!NT_SUCCESS(Status))
        {
            /* failed to set interface */
            break;
        }

        /* move to next entry */
        Entry = Entry->Flink;
    }

    /* done */
    return Status;
}

NTSTATUS
KspDoReparseForIrp(
    PIRP Irp,
    PBUS_DEVICE_ENTRY DeviceEntry)
{
    SIZE_T Length;
    LPWSTR Buffer;
    PIO_STACK_LOCATION IoStack;

    /* get stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* sanity checks */
    ASSERT(DeviceEntry->PDODeviceName);
    ASSERT(DeviceEntry->Instance);
    ASSERT(IoStack->FileObject);
    ASSERT(IoStack->FileObject->FileName.Buffer);

    /* calculate length */
    Length = wcslen(DeviceEntry->PDODeviceName);
    Length += wcslen(DeviceEntry->Instance);

    /* zero byte and '\\' */
    Length += 2;

    /* allocate buffer */
    Buffer = ExAllocatePoolWithTag(NonPagedPool, Length * sizeof(WCHAR), 'mNoI');
    if (!Buffer)
    {
        /* no resources */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* construct buffer */
    swprintf(Buffer, L"%s\\%s", DeviceEntry->PDODeviceName, DeviceEntry->Instance);

    /* free old buffer*/
    ExFreePoolWithTag(IoStack->FileObject->FileName.Buffer, 'mNoI');

    /* store new file name */
    RtlInitUnicodeString(&IoStack->FileObject->FileName, Buffer);

    /* done */
    return STATUS_REPARSE;
}

VOID
KspCompletePendingIrps(
    IN PBUS_DEVICE_ENTRY DeviceEntry,
    IN OUT NTSTATUS ResultCode)
{
    PLIST_ENTRY Entry;
    PIRP Irp;
    NTSTATUS Status;

    /* go through list */
    while(!IsListEmpty(&DeviceEntry->IrpPendingList))
    {
        /* get first entry */
        Entry = RemoveHeadList(&DeviceEntry->IrpPendingList);

        /* get irp */
        Irp = (PIRP)CONTAINING_RECORD(Entry, IRP, Tail.Overlay.ListEntry);

        if (ResultCode == STATUS_REPARSE)
        {
            /* construct reparse information */
            Status = KspDoReparseForIrp(Irp, DeviceEntry);
        }
        else
        {
            /* use default code */
            Status = ResultCode;
        }

        /* store result code */
        Irp->IoStatus.Status = Status;

        DPRINT("Completing IRP %p Status %x\n", Irp, Status);

        /* complete the request */
        CompleteRequest(Irp, IO_NO_INCREMENT);
    }

}



NTSTATUS
KspStartBusDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PCOMMON_DEVICE_EXTENSION ChildDeviceExtension,
    IN PIRP Irp)
{
    WCHAR PDOName[256];
    NTSTATUS Status;
    ULONG ResultLength;
    LPWSTR Name;
    ULONG NameLength;
    PBUS_DEVICE_ENTRY DeviceEntry;

    /* FIXME handle pending remove */

    /* get full device name */
    Status = IoGetDeviceProperty(DeviceObject, DevicePropertyPhysicalDeviceObjectName, sizeof(PDOName), PDOName, &ResultLength);

    if (!NT_SUCCESS(Status))
    {
        /* failed to get device name */
        return Status;
    }

    /* allocate device name buffer */
    NameLength = ResultLength + sizeof(UNICODE_NULL);
    Name = AllocateItem(NonPagedPool, NameLength);
    if (!Name)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* copy name */
    NT_VERIFY(NT_SUCCESS(RtlStringCbCopyW(Name, NameLength, PDOName)));

    /* TODO: time stamp creation time */

    /* get device entry */
    DeviceEntry = (PBUS_DEVICE_ENTRY)ChildDeviceExtension->DeviceEntry;

    /* sanity check */
    ASSERT(DeviceEntry);

    /* store device name */
    DeviceEntry->PDODeviceName = Name;

    /* mark device as started */
    DeviceEntry->DeviceState = Started;

    /* reference start time */
    KeQueryTickCount(&DeviceEntry->TimeCreated);

    DPRINT1("KspStartBusDevice Name %S DeviceName %S Instance %S Started\n", Name, DeviceEntry->DeviceName, DeviceEntry->Instance);

    /* enable device classes */
    KspEnableBusDeviceInterface(DeviceEntry, TRUE);

    /* done */
    return STATUS_SUCCESS;
}

NTSTATUS
KspQueryBusDeviceCapabilities(
    IN PCOMMON_DEVICE_EXTENSION ChildDeviceExtension,
    IN PIRP Irp)
{
    PDEVICE_CAPABILITIES Capabilities;
    PIO_STACK_LOCATION IoStack;

    /* get stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* get capabilities */
    Capabilities = IoStack->Parameters.DeviceCapabilities.Capabilities;

    RtlZeroMemory(Capabilities, sizeof(DEVICE_CAPABILITIES));

    /* setup capabilities */
    Capabilities->UniqueID = TRUE;
    Capabilities->SilentInstall = TRUE;
    Capabilities->SurpriseRemovalOK = TRUE;
    Capabilities->Address = 0;
    Capabilities->UINumber = 0;
    Capabilities->SystemWake = PowerSystemWorking; /* FIXME common device extension */
    Capabilities->DeviceWake = PowerDeviceD0;

    /* done */
    return STATUS_SUCCESS;
}

NTSTATUS
KspQueryBusInformation(
    IN PCOMMON_DEVICE_EXTENSION ChildDeviceExtension,
    IN PIRP Irp)
{
    PPNP_BUS_INFORMATION BusInformation;

    /* allocate bus information */
    BusInformation = (PPNP_BUS_INFORMATION)AllocateItem(PagedPool, sizeof(PNP_BUS_INFORMATION));

    if (!BusInformation)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* return info */
    BusInformation->BusNumber = 0;
    BusInformation->LegacyBusType = InterfaceTypeUndefined;
    RtlMoveMemory(&BusInformation->BusTypeGuid, &KSMEDIUMSETID_Standard, sizeof(GUID));

    /* store result */
    Irp->IoStatus.Information = (ULONG_PTR)BusInformation;

    /* done */
    return STATUS_SUCCESS;
}

NTSTATUS
KspQueryBusDevicePnpState(
    IN PCOMMON_DEVICE_EXTENSION ChildDeviceExtension,
    IN PIRP Irp)
{
    /* set device flags */
    Irp->IoStatus.Information = PNP_DEVICE_DONT_DISPLAY_IN_UI | PNP_DEVICE_NOT_DISABLEABLE;

    /* done */
    return STATUS_SUCCESS;
}

NTSTATUS
KspQueryId(
    IN PCOMMON_DEVICE_EXTENSION ChildDeviceExtension,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PBUS_DEVICE_ENTRY DeviceEntry;
    PBUS_ENUM_DEVICE_EXTENSION BusDeviceExtension;
    LPWSTR Name;
    SIZE_T Length;

    /* get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.QueryId.IdType == BusQueryInstanceID)
    {
        /* get device entry */
        DeviceEntry = (PBUS_DEVICE_ENTRY) ChildDeviceExtension->DeviceEntry;

        /* sanity check */
        ASSERT(DeviceEntry);
        ASSERT(DeviceEntry->Instance);

        /* calculate length */
        Length = wcslen(DeviceEntry->Instance) + 2;

        /* allocate buffer */
        Name = AllocateItem(PagedPool, Length * sizeof(WCHAR));

        if (!Name)
        {
            /* failed to allocate buffer */
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* copy buffer */
        wcscpy(Name, DeviceEntry->Instance);

        /* store result */
        Irp->IoStatus.Information = (ULONG_PTR)Name;

        /* done */
        return STATUS_SUCCESS;
    }
    else if (IoStack->Parameters.QueryId.IdType == BusQueryDeviceID ||
             IoStack->Parameters.QueryId.IdType == BusQueryHardwareIDs)
    {
        /* get device entry */
        DeviceEntry = (PBUS_DEVICE_ENTRY) ChildDeviceExtension->DeviceEntry;

        /* get bus device extension */
        BusDeviceExtension = (PBUS_ENUM_DEVICE_EXTENSION) ChildDeviceExtension->BusDeviceExtension;

        /* sanity check */
        ASSERT(DeviceEntry);
        ASSERT(DeviceEntry->BusId);
        ASSERT(BusDeviceExtension);
        ASSERT(BusDeviceExtension->BusIdentifier);

        /* calculate length */
        Length = wcslen(BusDeviceExtension->BusIdentifier);
        Length += wcslen(DeviceEntry->BusId);

        /* extra length for '\\' and 2 zero bytes */
        Length += 4;

        /* allocate buffer */
        Name = AllocateItem(PagedPool, Length * sizeof(WCHAR));
        if (!Name)
        {
            /* failed to allocate buffer */
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* construct id */
        wcscpy(Name, BusDeviceExtension->BusIdentifier);
        wcscat(Name, L"\\");
        wcscat(Name, DeviceEntry->BusId);
        //swprintf(Name, L"%s\\%s", BusDeviceExtension->BusIdentifier, DeviceEntry->BusId);

        /* store result */
        Irp->IoStatus.Information = (ULONG_PTR)Name;

        /* done */
        return STATUS_SUCCESS;
    }
    else
    {
        /* other ids are not supported */
        //DPRINT1("Not Supported ID Type %x\n", IoStack->Parameters.QueryId.IdType);
        return Irp->IoStatus.Status;
    }
}

NTSTATUS
KspInstallInterface(
    IN PBUS_ENUM_DEVICE_EXTENSION BusDeviceExtension,
    IN PSWENUM_INSTALL_INTERFACE InstallInterface)
{
    SIZE_T Length, Index;
    UNICODE_STRING DeviceString, InterfaceString, ReferenceString;
    HANDLE hKey, hDeviceKey, hInterfaceKey, hReferenceKey;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;

    /* sanity check */
    ASSERT(InstallInterface);

    /* calculate length */
    Length = wcslen(InstallInterface->ReferenceString);

    /* check for invalid characters */
    for(Index = 0; Index < Length; Index++)
    {
        if (InstallInterface->ReferenceString[Index] <= L' ' ||
            InstallInterface->ReferenceString[Index] > L'~' ||
            InstallInterface->ReferenceString[Index] == L',' ||
            InstallInterface->ReferenceString[Index] == L'\\' ||
            InstallInterface->ReferenceString[Index] == L'/')
        {
            /* invalid character */
            return STATUS_INVALID_PARAMETER;
        }
    }

    /* open bus key */
    Status = KspOpenBusRegistryKey(BusDeviceExtension, &hKey);
    if (NT_SUCCESS(Status))
    {
        /* convert device guid to string */
        Status = RtlStringFromGUID(&InstallInterface->DeviceId, &DeviceString);
        if (NT_SUCCESS(Status))
        {
            /* initialize object attributes */
            InitializeObjectAttributes(&ObjectAttributes, &DeviceString, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, hKey, NULL);

            /* construct device key */
            Status = ZwCreateKey(&hDeviceKey, GENERIC_WRITE, &ObjectAttributes, 0, NULL, 0, NULL);
            if (NT_SUCCESS(Status))
            {
                /* convert interface guid to string */
                Status = RtlStringFromGUID(&InstallInterface->InterfaceId, &InterfaceString);
                if (NT_SUCCESS(Status))
                {
                    /* initialize object attributes */
                    InitializeObjectAttributes(&ObjectAttributes, &InterfaceString, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, hDeviceKey, NULL);

                    /* construct device key */
                    Status = ZwCreateKey(&hInterfaceKey, GENERIC_WRITE, &ObjectAttributes, 0, NULL, 0, NULL);
                    if (NT_SUCCESS(Status))
                    {
                        /* initialize reference string */
                        RtlInitUnicodeString(&ReferenceString, InstallInterface->ReferenceString);

                        /* initialize object attributes */
                        InitializeObjectAttributes(&ObjectAttributes, &ReferenceString, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, hInterfaceKey, NULL);

                        /* construct device key */
                        Status = ZwCreateKey(&hReferenceKey, GENERIC_WRITE, &ObjectAttributes, 0, NULL, 0, NULL);
                        if (NT_SUCCESS(Status))
                        {
                            /* close key */
                            ZwClose(hReferenceKey);
                        }
                    }
                    /* free interface string */
                    RtlFreeUnicodeString(&InterfaceString);

                    /* close reference key */
                    ZwClose(hInterfaceKey);
                }
                /* close device key */
                ZwClose(hDeviceKey);
            }
            /* free device string */
            RtlFreeUnicodeString(&DeviceString);
        }
        /* close bus key */
        ZwClose(hKey);
    }

    /* done */
    return Status;
 }

VOID
NTAPI
KspInstallBusEnumInterface(
    IN PVOID Ctx)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    PLIST_ENTRY Entry;
    PBUS_DEVICE_ENTRY DeviceEntry;
    PSWENUM_INSTALL_INTERFACE InstallInterface;
    KIRQL OldLevel;
    PBUS_INSTALL_ENUM_CONTEXT Context = (PBUS_INSTALL_ENUM_CONTEXT)Ctx;

    /* get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Context->Irp);

    /* get install request */
    InstallInterface = (PSWENUM_INSTALL_INTERFACE)Context->Irp->AssociatedIrp.SystemBuffer;

    /* sanity check */
    ASSERT(InstallInterface);
    ASSERT(Context->BusDeviceExtension);

    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SWENUM_INSTALL_INTERFACE))
    {
        /* buffer too small */
        Context->Status = STATUS_INVALID_PARAMETER;

        /* signal completion */
        KeSetEvent(&Context->Event, 0, FALSE);

        /* done */
        return;
    }

    /* FIXME locks */

    /* now install the interface */
    Status = KspInstallInterface(Context->BusDeviceExtension, InstallInterface);
    if (!NT_SUCCESS(Status))
    {
        /* failed to install interface */
        Context->Status = Status;

        /* signal completion */
        KeSetEvent(&Context->Event, 0, FALSE);

        /* done */
        return;
    }

    /* now scan the bus */
    Status = KspScanBus(Context->BusDeviceExtension);
    // FIXME: We may need to check for success here, and properly fail...
    ASSERT(NT_SUCCESS(Status));

    /* acquire device entry lock */
    KeAcquireSpinLock(&Context->BusDeviceExtension->Lock, &OldLevel);

    /* now iterate all device entries */
    ASSERT(!IsListEmpty(&Context->BusDeviceExtension->Common.Entry));
    Entry = Context->BusDeviceExtension->Common.Entry.Flink;
    while(Entry != &Context->BusDeviceExtension->Common.Entry)
    {
        /* get device entry */
        DeviceEntry = (PBUS_DEVICE_ENTRY)CONTAINING_RECORD(Entry, BUS_DEVICE_ENTRY, Entry);
        if (IsEqualGUIDAligned(&DeviceEntry->DeviceGuid, &InstallInterface->DeviceId))
        {
            if (!DeviceEntry->PDO)
            {
                /* release device entry lock */
                KeReleaseSpinLock(&Context->BusDeviceExtension->Lock, OldLevel);

                /* create pdo */
                Status = KspCreatePDO(Context->BusDeviceExtension, DeviceEntry, &DeviceEntry->PDO);

                /* acquire device entry lock */
                KeAcquireSpinLock(&Context->BusDeviceExtension->Lock, &OldLevel);

                /* done */
                break;
            }
        }

        /* move to next entry */
        Entry = Entry->Flink;
    }

    /* release device entry lock */
    KeReleaseSpinLock(&Context->BusDeviceExtension->Lock, OldLevel);

    /* signal that bus driver relations has changed */
    IoInvalidateDeviceRelations(Context->BusDeviceExtension->PhysicalDeviceObject, BusRelations);

    /* update status */
    Context->Status = Status;

    /* signal completion */
    KeSetEvent(&Context->Event, 0, FALSE);
}


VOID
NTAPI
KspBusWorkerRoutine(
  IN PVOID Parameter)
{
    PBUS_ENUM_DEVICE_EXTENSION BusDeviceExtension;
    PBUS_DEVICE_ENTRY DeviceEntry;
    PLIST_ENTRY Entry;
    LARGE_INTEGER Time, Diff;
    BOOLEAN DoInvalidate = FALSE;
    KIRQL OldLevel;

    /* get device extension */
    BusDeviceExtension = (PBUS_ENUM_DEVICE_EXTENSION)Parameter;

    /* acquire lock */
    KeAcquireSpinLock(&BusDeviceExtension->Lock, &OldLevel);

    /* get current time */
    KeQueryTickCount(&Time);

    /* enumerate all device entries */
    Entry = BusDeviceExtension->Common.Entry.Flink;
    while(Entry != &BusDeviceExtension->Common.Entry)
    {
        /* get offset to device entry */
        DeviceEntry = (PBUS_DEVICE_ENTRY)CONTAINING_RECORD(Entry, BUS_DEVICE_ENTRY, Entry);

        /* sanity check */
        ASSERT(DeviceEntry);

        //DPRINT1("DeviceEntry %p PDO %p State %x\n", DeviceEntry, DeviceEntry->PDO, DeviceEntry->DeviceState);

        if (DeviceEntry->PDO)
        {
            if (DeviceEntry->DeviceState == NotStarted)
            {
                Diff.QuadPart = (Time.QuadPart - DeviceEntry->TimeCreated.QuadPart) * KeQueryTimeIncrement();

                /* wait for 15 sec */
                if (Diff.QuadPart > Int32x32To64(15000, 10000))
                {
                     /* release spin lock */
                     KeReleaseSpinLock(&BusDeviceExtension->Lock, OldLevel);

                     DPRINT1("DeviceID %S Instance %S TimeCreated %I64u Now %I64u Diff %I64u hung\n",
                        DeviceEntry->DeviceName,
                        DeviceEntry->Instance,
                        DeviceEntry->TimeCreated.QuadPart * KeQueryTimeIncrement(),
                        Time.QuadPart * KeQueryTimeIncrement(),
                        Diff.QuadPart);

                     /* deactivate interfaces */
                     KspEnableBusDeviceInterface(DeviceEntry, FALSE);

                     /* re-acquire lock */
                     KeAcquireSpinLock(&BusDeviceExtension->Lock, &OldLevel);

                     /* pending remove device object */
                     DeviceEntry->DeviceState = StopPending;

                     /* perform invalidation */
                     DoInvalidate = TRUE;
                }
            }
            else if (DeviceEntry->DeviceState == Started)
            {
                /* release spin lock */
                KeReleaseSpinLock(&BusDeviceExtension->Lock, OldLevel);

                /* found pending irps */
                KspCompletePendingIrps(DeviceEntry, STATUS_REPARSE);

                /* re-acquire lock */
                KeAcquireSpinLock(&BusDeviceExtension->Lock, &OldLevel);
            }
        }


        /* move to next */
        Entry = Entry->Flink;
    }

   /* release lock */
    KeReleaseSpinLock(&BusDeviceExtension->Lock, OldLevel);

    if (DoInvalidate)
    {
        /* invalidate device relations */
        IoInvalidateDeviceRelations(BusDeviceExtension->PhysicalDeviceObject, BusRelations);
    }

    Time.QuadPart = Int32x32To64(5000, -10000);
    KeSetTimer(&BusDeviceExtension->Timer, Time, &BusDeviceExtension->Dpc);
}

VOID
NTAPI
KspBusDpcRoutine(
    IN PKDPC Dpc,
    IN PVOID DeferredContext OPTIONAL,
    IN PVOID SystemArgument1 OPTIONAL,
    IN PVOID SystemArgument2 OPTIONAL)
{
    /* get device extension */
    PBUS_ENUM_DEVICE_EXTENSION BusDeviceExtension = (PBUS_ENUM_DEVICE_EXTENSION)DeferredContext;

    /* queue the item */
    ExQueueWorkItem(&BusDeviceExtension->WorkItem, DelayedWorkQueue);
}

VOID
NTAPI
KspRemoveBusInterface(
    PVOID Ctx)
{
    PBUS_INSTALL_ENUM_CONTEXT Context =(PBUS_INSTALL_ENUM_CONTEXT)Ctx;

    /* TODO
     * get SWENUM_INSTALL_INTERFACE struct
     * open device key and delete the keys
     */

    UNIMPLEMENTED;

    /* set status */
    Context->Status = STATUS_NOT_IMPLEMENTED;


    /* signal completion */
    KeSetEvent(&Context->Event, IO_NO_INCREMENT, FALSE);
}

NTSTATUS
KspQueryBusRelations(
    IN PBUS_ENUM_DEVICE_EXTENSION BusDeviceExtension,
    IN PIRP Irp)
{
    PDEVICE_RELATIONS DeviceRelations;
    PLIST_ENTRY Entry;
    PBUS_DEVICE_ENTRY DeviceEntry;
    ULONG Count = 0, Length;
    KIRQL OldLevel;

    /* acquire lock */
    KeAcquireSpinLock(&BusDeviceExtension->Lock, &OldLevel);

    /* first scan all device entries */
    Entry = BusDeviceExtension->Common.Entry.Flink;

    while(Entry != &BusDeviceExtension->Common.Entry)
    {
        /* get offset to device entry */
        DeviceEntry = (PBUS_DEVICE_ENTRY)CONTAINING_RECORD(Entry, BUS_DEVICE_ENTRY, Entry);

        /* is there a pdo yet */
        if (DeviceEntry->PDO && (DeviceEntry->DeviceState == NotStarted || DeviceEntry->DeviceState == Started))
        {
            /* increment count */
            Count++;
        }

        /* move to next entry */
        Entry = Entry->Flink;
    }

    /* calculate length */
    Length = sizeof(DEVICE_RELATIONS) + (Count > 1 ? sizeof(PDEVICE_OBJECT) * (Count-1) : 0);

    /* allocate device relations */
    DeviceRelations = (PDEVICE_RELATIONS)AllocateItem(NonPagedPool, Length);

    if (!DeviceRelations)
    {
        /* not enough memory */
        KeReleaseSpinLock(&BusDeviceExtension->Lock, OldLevel);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* rescan device entries */
    Entry = BusDeviceExtension->Common.Entry.Flink;

    while(Entry != &BusDeviceExtension->Common.Entry)
    {
        /* get offset to device entry */
        DeviceEntry = (PBUS_DEVICE_ENTRY)CONTAINING_RECORD(Entry, BUS_DEVICE_ENTRY, Entry);

        /* is there a pdo yet */
        if (DeviceEntry->PDO && (DeviceEntry->DeviceState == NotStarted || DeviceEntry->DeviceState == Started))
        {
            /* store pdo */
            DeviceRelations->Objects[DeviceRelations->Count] = DeviceEntry->PDO;

            /* reference device object */
            ObReferenceObject(DeviceEntry->PDO);

            /* increment pdo count */
            DeviceRelations->Count++;
        }

        /* move to next entry */
        Entry = Entry->Flink;
    }

    /* release lock */
    KeReleaseSpinLock(&BusDeviceExtension->Lock, OldLevel);

    /* FIXME handle existing device relations */
    ASSERT(Irp->IoStatus.Information == 0);

    /* store device relations */
    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;

    /* done */
    return STATUS_SUCCESS;
}

//------------------------------------------------------------------------------------

/*
    @implemented
*/

KSDDKAPI
NTSTATUS
NTAPI
KsGetBusEnumIdentifier(
    IN PIRP Irp)
{
    PDEV_EXTENSION DeviceExtension;
    PBUS_ENUM_DEVICE_EXTENSION BusDeviceExtension;
    PIO_STACK_LOCATION IoStack;
    SIZE_T Length;
    NTSTATUS Status;
    LPWSTR Buffer;

    DPRINT("KsGetBusEnumIdentifier\n");

    /* get stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* sanity checks */
    ASSERT(IoStack->DeviceObject);
    ASSERT(IoStack->DeviceObject->DeviceExtension);

    /* get device extension */
    DeviceExtension = (PDEV_EXTENSION)IoStack->DeviceObject->DeviceExtension;

    /* get bus device extension */
    BusDeviceExtension = (PBUS_ENUM_DEVICE_EXTENSION)DeviceExtension->Ext;

    /* sanity checks */
    ASSERT(BusDeviceExtension);
    ASSERT(BusDeviceExtension->Common.IsBus);

    if (!BusDeviceExtension)
    {
        /* invalid parameter */
        return STATUS_INVALID_PARAMETER;
    }

    /* get length */
    Length = (wcslen(BusDeviceExtension->BusIdentifier)+1) * sizeof(WCHAR);

    /* is there an output buffer provided */
    if (IoStack->Parameters.DeviceIoControl.InputBufferLength)
    {
        if (Length > IoStack->Parameters.DeviceIoControl.InputBufferLength)
        {
            /* buffer is too small */
            return STATUS_BUFFER_TOO_SMALL;
        }

        /* now allocate buffer */
        Buffer = AllocateItem(NonPagedPool, Length);
        if (!Buffer)
        {
            /* no memory */
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            /* copy bus identifier */
            wcscpy(Buffer, BusDeviceExtension->BusIdentifier);

            /* store buffer */
            Irp->AssociatedIrp.SystemBuffer = Buffer;

            /* set flag that buffer gets copied back */
            Irp->Flags |= IRP_BUFFERED_IO | IRP_DEALLOCATE_BUFFER | IRP_INPUT_OPERATION;

            /* done */
            Status = STATUS_SUCCESS;
        }
    }
    else
    {
        /* no buffer provided */
        Status = STATUS_BUFFER_OVERFLOW;
    }

    /* done */
    Irp->IoStatus.Status = Status;
    return Status;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsGetBusEnumParentFDOFromChildPDO(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PDEVICE_OBJECT *FunctionalDeviceObject)
{
    PDEV_EXTENSION DeviceExtension;

    DPRINT("KsGetBusEnumParentFDOFromChildPDO\n");

    /* get device extension */
    DeviceExtension = (PDEV_EXTENSION)DeviceObject->DeviceExtension;

    /* check if this is child pdo */
    if (DeviceExtension->Ext->IsBus == FALSE)
    {
        /* return bus device object */
        *FunctionalDeviceObject = DeviceExtension->Ext->BusDeviceExtension->BusDeviceObject;

        /* done */
        return STATUS_SUCCESS;
    }

    /* invalid parameter */
    return STATUS_INVALID_PARAMETER;
}


/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsCreateBusEnumObject(
    IN PWCHAR BusIdentifier,
    IN PDEVICE_OBJECT BusDeviceObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PDEVICE_OBJECT PnpDeviceObject OPTIONAL,
    IN REFGUID InterfaceGuid OPTIONAL,
    IN PWCHAR ServiceRelativePath OPTIONAL)
{
    SIZE_T Length;
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING ServiceKeyPath = RTL_CONSTANT_STRING(L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Services\\");
    PBUS_ENUM_DEVICE_EXTENSION BusDeviceExtension;
    PDEV_EXTENSION DeviceExtension;
    PBUS_DEVICE_ENTRY DeviceEntry;
    PLIST_ENTRY Entry;
    KIRQL OldLevel;

    DPRINT1("KsCreateBusEnumObject %S BusDeviceObject %p\n", ServiceRelativePath, BusDeviceObject);

    /* calculate sizeof bus enum device extension */
    Length = wcslen(BusIdentifier) * sizeof(WCHAR);
    Length += sizeof(BUS_ENUM_DEVICE_EXTENSION);

    BusDeviceExtension = AllocateItem(NonPagedPool, Length);
    if (!BusDeviceExtension)
    {
        /* not enough memory */

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* get device extension */
    DeviceExtension = (PDEV_EXTENSION)BusDeviceObject->DeviceExtension;

    /* store bus device extension */
    DeviceExtension->Ext = (PCOMMON_DEVICE_EXTENSION)BusDeviceExtension;

    DPRINT("DeviceExtension %p BusDeviceExtension %p\n", DeviceExtension, DeviceExtension->Ext);


    /* zero device extension */
    RtlZeroMemory(BusDeviceExtension, sizeof(BUS_ENUM_DEVICE_EXTENSION));

    /* initialize bus device extension */
    wcscpy(BusDeviceExtension->BusIdentifier, BusIdentifier);

    /* allocate service path string */
    Length = ServiceKeyPath.MaximumLength;
    Length += BusDeviceObject->DriverObject->DriverExtension->ServiceKeyName.MaximumLength;

    if (ServiceRelativePath)
    {
        /* relative path for devices */
        Length += (wcslen(ServiceRelativePath) + 2) * sizeof(WCHAR);
    }

    BusDeviceExtension->ServicePath.Length = 0;
    BusDeviceExtension->ServicePath.MaximumLength = (USHORT)Length;
    BusDeviceExtension->ServicePath.Buffer = AllocateItem(NonPagedPool, Length);

    if (!BusDeviceExtension->ServicePath.Buffer)
    {
        /* not enough memory */
        FreeItem(BusDeviceExtension);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlAppendUnicodeStringToString(&BusDeviceExtension->ServicePath, &ServiceKeyPath);
    RtlAppendUnicodeStringToString(&BusDeviceExtension->ServicePath, &BusDeviceObject->DriverObject->DriverExtension->ServiceKeyName);

    if (ServiceRelativePath)
    {
        RtlAppendUnicodeToString(&BusDeviceExtension->ServicePath, L"\\");
        RtlAppendUnicodeToString(&BusDeviceExtension->ServicePath, ServiceRelativePath);
    }

    if (InterfaceGuid)
    {
        /* register an device interface */
        Status = IoRegisterDeviceInterface(PhysicalDeviceObject, InterfaceGuid, NULL, &BusDeviceExtension->DeviceInterfaceLink);

        /* check for success */
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IoRegisterDeviceInterface failed Status %lx\n", Status);
            FreeItem(BusDeviceExtension->ServicePath.Buffer);
            FreeItem(BusDeviceExtension);
            return Status;
        }

        /* now enable device interface */
        Status = IoSetDeviceInterfaceState(&BusDeviceExtension->DeviceInterfaceLink, TRUE);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IoSetDeviceInterfaceState failed Status %lx\n", Status);
            FreeItem(BusDeviceExtension->ServicePath.Buffer);
            FreeItem(BusDeviceExtension);
            return Status;
        }
    }

    /* initialize common device extension */
    BusDeviceExtension->Common.BusDeviceExtension = NULL;
    BusDeviceExtension->Common.DeviceObjectReferenceCount = 1;
    BusDeviceExtension->Common.DeviceReferenceCount = 1;
    BusDeviceExtension->Common.IsBus = TRUE;
    InitializeListHead(&BusDeviceExtension->Common.Entry);

    /* store device objects */
    BusDeviceExtension->BusDeviceObject = BusDeviceObject;
    BusDeviceExtension->PhysicalDeviceObject = PhysicalDeviceObject;

    /* initialize lock */
    KeInitializeSpinLock(&BusDeviceExtension->Lock);

    /* initialize timer */
    KeInitializeTimer(&BusDeviceExtension->Timer);

    /* initialize dpc */
    KeInitializeDpc(&BusDeviceExtension->Dpc, KspBusDpcRoutine, (PVOID)BusDeviceExtension);

    /* initialize event */
    KeInitializeEvent(&BusDeviceExtension->Event, SynchronizationEvent, FALSE);

    /* initialize work item */
    ExInitializeWorkItem(&BusDeviceExtension->WorkItem, KspBusWorkerRoutine, (PVOID)BusDeviceExtension);

    if (!PnpDeviceObject)
    {
        /* attach device */
        BusDeviceExtension->PnpDeviceObject = IoAttachDeviceToDeviceStack(BusDeviceObject, PhysicalDeviceObject);

        if (!BusDeviceExtension->PnpDeviceObject)
        {
            /* failed to attach device */
            DPRINT1("IoAttachDeviceToDeviceStack failed with %x\n", Status);
            if (BusDeviceExtension->DeviceInterfaceLink.Buffer)
            {
                IoSetDeviceInterfaceState(&BusDeviceExtension->DeviceInterfaceLink, FALSE);
                RtlFreeUnicodeString(&BusDeviceExtension->DeviceInterfaceLink);
            }

            /* free device extension */
            FreeItem(BusDeviceExtension->ServicePath.Buffer);
            FreeItem(BusDeviceExtension);

            return STATUS_DEVICE_REMOVED;
        }

        /* mark device as attached */
        BusDeviceExtension->DeviceAttached = TRUE;
    }
    else
    {
        /* directly attach */
        BusDeviceExtension->PnpDeviceObject = PnpDeviceObject;
    }

    /* now scan the bus */
    Status = KspScanBus(BusDeviceExtension);

    /* check for success */
    if (!NT_SUCCESS(Status))
    {
        /* failed to scan bus */
        if (BusDeviceExtension->DeviceInterfaceLink.Buffer)
        {
            IoSetDeviceInterfaceState(&BusDeviceExtension->DeviceInterfaceLink, FALSE);
            RtlFreeUnicodeString(&BusDeviceExtension->DeviceInterfaceLink);
        }

        if (BusDeviceExtension->DeviceAttached)
        {
            /* detach device */
            IoDetachDevice(BusDeviceExtension->PnpDeviceObject);
        }

        /* free device extension */
        FreeItem(BusDeviceExtension->ServicePath.Buffer);
        FreeItem(BusDeviceExtension);

        return Status;
    }

    /* acquire device entry lock */
    KeAcquireSpinLock(&BusDeviceExtension->Lock, &OldLevel);

    /* now iterate all device entries */
    Entry = BusDeviceExtension->Common.Entry.Flink;
    while(Entry != &BusDeviceExtension->Common.Entry)
    {
        /* get device entry */
        DeviceEntry = (PBUS_DEVICE_ENTRY)CONTAINING_RECORD(Entry, BUS_DEVICE_ENTRY, Entry);
        if (!DeviceEntry->PDO)
        {
            /* release device entry lock */
            KeReleaseSpinLock(&BusDeviceExtension->Lock, OldLevel);

            /* create pdo */
            Status = KspCreatePDO(BusDeviceExtension, DeviceEntry, &DeviceEntry->PDO);

            /* acquire device entry lock */
            KeAcquireSpinLock(&BusDeviceExtension->Lock, &OldLevel);
        }
        /* move to next entry */
        Entry = Entry->Flink;
    }

    /* release device entry lock */
    KeReleaseSpinLock(&BusDeviceExtension->Lock, OldLevel);


    /* invalidate device relations */
    IoInvalidateDeviceRelations(BusDeviceExtension->PhysicalDeviceObject, BusRelations);
    DPRINT("KsCreateBusEnumObject Status %x\n", Status);
    /* done */
    return Status;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsGetBusEnumPnpDeviceObject(
    IN PDEVICE_OBJECT DeviceObject,
    IN PDEVICE_OBJECT *PnpDeviceObject)
{
    PDEV_EXTENSION DeviceExtension;
    PCOMMON_DEVICE_EXTENSION CommonDeviceExtension;
    PBUS_ENUM_DEVICE_EXTENSION BusDeviceExtension;

    DPRINT("KsGetBusEnumPnpDeviceObject\n");

    if (!DeviceObject->DeviceExtension)
    {
        /* invalid parameter */
        return STATUS_INVALID_PARAMETER;
    }

    /* get device extension */
    DeviceExtension = (PDEV_EXTENSION)DeviceObject->DeviceExtension;

    /* get common device extension */
    CommonDeviceExtension = DeviceExtension->Ext;

    if (!CommonDeviceExtension)
    {
        /* invalid parameter */
        return STATUS_INVALID_PARAMETER;
    }

    if (!CommonDeviceExtension->IsBus)
    {
        /* getting pnp device object is only supported for software bus device object */
       return STATUS_INVALID_PARAMETER;
    }

    /* sanity checks */
    ASSERT(CommonDeviceExtension);
    ASSERT(CommonDeviceExtension->IsBus);

    /* cast to bus device extension */
    BusDeviceExtension = (PBUS_ENUM_DEVICE_EXTENSION)CommonDeviceExtension;

    /* store result */
    *PnpDeviceObject = BusDeviceExtension->PnpDeviceObject;

    /* done */
    return STATUS_SUCCESS;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsInstallBusEnumInterface(
    PIRP Irp)
{
    BUS_INSTALL_ENUM_CONTEXT Context;
    KPROCESSOR_MODE Mode;
    LUID luid;
    PIO_STACK_LOCATION IoStack;
    PDEV_EXTENSION DeviceExtension;
    PBUS_ENUM_DEVICE_EXTENSION BusDeviceExtension;

    DPRINT("KsInstallBusEnumInterface\n");

    /* get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* get previous mode */
    Mode = ExGetPreviousMode();

    /* convert to luid */
    luid = RtlConvertUlongToLuid(SE_LOAD_DRIVER_PRIVILEGE);

    /* perform access check */
    if (!SeSinglePrivilegeCheck(luid, Mode))
    {
        /* FIXME insufficient privileges */
        //return STATUS_PRIVILEGE_NOT_HELD;
    }

    /* get device extension */
    DeviceExtension = (PDEV_EXTENSION)IoStack->DeviceObject->DeviceExtension;

    /* get bus device extension */
    BusDeviceExtension = (PBUS_ENUM_DEVICE_EXTENSION)DeviceExtension->Ext;


    /* initialize context */
    Context.Irp = Irp;
    KeInitializeEvent(&Context.Event, NotificationEvent, FALSE);
    Context.BusDeviceExtension = BusDeviceExtension;
    ExInitializeWorkItem(&Context.WorkItem, KspInstallBusEnumInterface, (PVOID)&Context);

    /* queue the work item */
    ExQueueWorkItem(&Context.WorkItem, DelayedWorkQueue);
    /* wait for completion */
    KeWaitForSingleObject(&Context.Event, Executive, KernelMode, FALSE, NULL);

    /* store result */
    Irp->IoStatus.Status = Context.Status;

    /* done */
    return Context.Status;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsIsBusEnumChildDevice(
    IN PDEVICE_OBJECT DeviceObject,
    OUT PBOOLEAN ChildDevice)
{
    PDEV_EXTENSION DeviceExtension;
    PBUS_ENUM_DEVICE_EXTENSION BusDeviceExtension;

    DPRINT("KsIsBusEnumChildDevice %p\n", DeviceObject);

    /* get device extension */
    DeviceExtension = (PDEV_EXTENSION)DeviceObject->DeviceExtension;

    /* get bus device extension */
    BusDeviceExtension = (PBUS_ENUM_DEVICE_EXTENSION)DeviceExtension->Ext;

    if (!BusDeviceExtension)
    {
        /* not a bus device */
        return STATUS_INVALID_PARAMETER;
    }

    /* store result */
    *ChildDevice = (BusDeviceExtension->Common.IsBus == FALSE);

    return STATUS_SUCCESS;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsServiceBusEnumCreateRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp)
{
    PLIST_ENTRY Entry;
    PBUS_DEVICE_ENTRY DeviceEntry = NULL; /* fix gcc */
    PIO_STACK_LOCATION IoStack;
    BOOLEAN ItemExists = FALSE;
    PDEV_EXTENSION DeviceExtension;
    PBUS_ENUM_DEVICE_EXTENSION BusDeviceExtension;
    //PCOMMON_DEVICE_EXTENSION ChildDeviceExtension;
    NTSTATUS Status;
    LARGE_INTEGER Time;

    /* FIXME: locks */

    /* get device extension */
    DeviceExtension = (PDEV_EXTENSION)DeviceObject->DeviceExtension;

    /* get bus device extension */
    BusDeviceExtension = (PBUS_ENUM_DEVICE_EXTENSION)DeviceExtension->Ext;

    /* get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* sanity checks */
    ASSERT(IoStack->FileObject);
    if (IoStack->FileObject->FileName.Buffer == NULL)
    {
         DPRINT("KsServiceBusEnumCreateRequest PNP Hack\n");
         Irp->IoStatus.Status = STATUS_SUCCESS;
         return STATUS_SUCCESS;
    }

    ASSERT(IoStack->FileObject->FileName.Buffer);

    DPRINT1("KsServiceBusEnumCreateRequest IRP %p Name %wZ\n", Irp, &IoStack->FileObject->FileName);

    /* scan the bus for new devices */
    Status = KspScanBus(BusDeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("KsServiceBusEnumCreateRequest failed to scan bus %x\n", Status);
        Irp->IoStatus.Status = Status;
        return Status;
    }

    /* scan list and check if it is already present */
    Entry = BusDeviceExtension->Common.Entry.Flink;

    while(Entry != &BusDeviceExtension->Common.Entry)
    {
        /* get real offset */
        DeviceEntry = (PBUS_DEVICE_ENTRY)CONTAINING_RECORD(Entry, BUS_DEVICE_ENTRY, Entry);

        /* check if name matches */
        if (!_wcsicmp(DeviceEntry->DeviceName, IoStack->FileObject->FileName.Buffer + 1))
        {
            /* item already exists */
            ItemExists = TRUE;
            break;
        }

        /* move to next entry */
        Entry = Entry->Flink;
    }

    if (!ItemExists)
    {
        /* interface not registered */
        DPRINT1("Interface %wZ not registered\n", &IoStack->FileObject->FileName);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    /*  is there a pdo yet */
    if (DeviceEntry->PDO)
    {
        if (DeviceEntry->DeviceState == Started)
        {
            /* issue reparse */
            Status =  KspDoReparseForIrp(Irp, DeviceEntry);
            DPRINT("REPARSE Irp %p '%wZ'\n", Irp, &IoStack->FileObject->FileName);

            Irp->IoStatus.Status = Status;
            Irp->IoStatus.Information = IO_REPARSE;
            return Status;
        }

        /* delay processing until pnp is finished with enumeration */
        IoMarkIrpPending(Irp);

        /* insert into irp pending list */
        InsertTailList(&DeviceEntry->IrpPendingList, &Irp->Tail.Overlay.ListEntry);

        Time.QuadPart = Int32x32To64(1500, -10000);
        DbgPrint("PENDING Irp %p %wZ DeviceState %d\n", Irp, &IoStack->FileObject->FileName, DeviceEntry->DeviceState);


        /* set timer */
        KeSetTimer(&BusDeviceExtension->Timer, Time, &BusDeviceExtension->Dpc);

        /* done for now */
        return STATUS_PENDING;

    }
    else
    {
        /* time to create PDO */
        Status = KspCreatePDO(BusDeviceExtension, DeviceEntry, &DeviceEntry->PDO);

        if (!NT_SUCCESS(Status))
        {
            /* failed to create PDO */
            DPRINT1("KsServiceBusEnumCreateRequest failed to create PDO with %x\n", Status);
            return Status;
        }
        DPRINT("PENDING CREATE Irp %p %wZ\n", Irp, &IoStack->FileObject->FileName);

        /* delay processing until pnp is finished with enumeration */
        IoMarkIrpPending(Irp);

        /* insert into irp pending list */
        InsertTailList(&DeviceEntry->IrpPendingList, &Irp->Tail.Overlay.ListEntry);

        /* invalidate device relations */
        IoInvalidateDeviceRelations(BusDeviceExtension->PhysicalDeviceObject, BusRelations);

        /* done for now */
        return STATUS_PENDING;
    }
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsServiceBusEnumPnpRequest(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp)
{
    PDEV_EXTENSION DeviceExtension;
    PBUS_ENUM_DEVICE_EXTENSION BusDeviceExtension;
    PCOMMON_DEVICE_EXTENSION ChildDeviceExtension;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    LARGE_INTEGER Time;
    PDEVICE_RELATIONS DeviceRelation;
    PBUS_DEVICE_ENTRY DeviceEntry;

    /* get device extension */
    DeviceExtension = (PDEV_EXTENSION)DeviceObject->DeviceExtension;

    /* get bus device extension */
    BusDeviceExtension = (PBUS_ENUM_DEVICE_EXTENSION)DeviceExtension->Ext;

    /* get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (BusDeviceExtension->Common.IsBus)
    {
        if (IoStack->MinorFunction == IRP_MN_START_DEVICE)
        {
            /* no op for bus driver */
            Status = STATUS_SUCCESS;
        }
        else if (IoStack->MinorFunction == IRP_MN_QUERY_DEVICE_RELATIONS)
        {
            /* handle bus device relations */
            ASSERT(IoStack->Parameters.QueryDeviceRelations.Type == BusRelations);

            Status = KspQueryBusRelations(BusDeviceExtension, Irp);
        }
        else
        {
            /* get default status */
            Status = Irp->IoStatus.Status;
        }
    }
    else
    {
        /* get child device extension */
        ChildDeviceExtension = DeviceExtension->Ext;

        /* get bus device extension */
        BusDeviceExtension = ChildDeviceExtension->BusDeviceExtension;

        if (IoStack->MinorFunction == IRP_MN_QUERY_ID)
        {
            /* query id */
            Status = KspQueryId(ChildDeviceExtension, Irp);
        }
        else if (IoStack->MinorFunction == IRP_MN_REMOVE_DEVICE)
        {
            ASSERT(ChildDeviceExtension->DeviceEntry->DeviceState != Started || ChildDeviceExtension->DeviceEntry->DeviceState == NotStarted);
            ASSERT(ChildDeviceExtension->DeviceEntry->PDO == DeviceObject);

            /* backup device entry */
            DeviceEntry = ChildDeviceExtension->DeviceEntry;

            /* clear PDO reference */
            DeviceEntry->PDO = NULL;

            /* delete the device */
            IoDeleteDevice(DeviceObject);

            if (DeviceEntry->PDODeviceName)
            {
                /* delete pdo device name */
                FreeItem(DeviceEntry->PDODeviceName);

                /* set to null */
                DeviceEntry->PDODeviceName = NULL;
            }

            /* set state no notstarted */
            DeviceEntry->DeviceState = NotStarted;

            /* complete pending irps */
            KspCompletePendingIrps(DeviceEntry, STATUS_DEVICE_REMOVED);

            /* free device extension */
            FreeItem(ChildDeviceExtension);

            /* FIXME this should only be done while installing the drivers */
            /* lets recreate the PDO */
            Status = KspCreatePDO(BusDeviceExtension, DeviceEntry, &DeviceEntry->PDO);
            /* restart enumeration */
            IoInvalidateDeviceRelations(BusDeviceExtension->PhysicalDeviceObject, BusRelations);
            /* done */
            Status = STATUS_SUCCESS;
        }
        else if (IoStack->MinorFunction == IRP_MN_QUERY_BUS_INFORMATION)
        {
            /* query bus information */
            Status = KspQueryBusInformation(ChildDeviceExtension, Irp);
        }
        else if (IoStack->MinorFunction == IRP_MN_QUERY_RESOURCES)
        {
            /* no op */
            Status = STATUS_SUCCESS;
        }
        else if (IoStack->MinorFunction == IRP_MN_QUERY_RESOURCE_REQUIREMENTS)
        {
            /* no op */
            Status = STATUS_SUCCESS;
        }
        else if (IoStack->MinorFunction == IRP_MN_START_DEVICE)
        {
            /* start bus */
            Status = KspStartBusDevice(DeviceObject, ChildDeviceExtension, Irp);
            if (NT_SUCCESS(Status))
            {
                /* complete pending irps*/
                KspCompletePendingIrps(ChildDeviceExtension->DeviceEntry, STATUS_REPARSE);
            }

            /* set time out */
            Time.QuadPart = Int32x32To64(1500, -10000);

            /* sanity check */
            ASSERT(BusDeviceExtension);

            /* set timer */
            KeSetTimer(&BusDeviceExtension->Timer, Time, &BusDeviceExtension->Dpc);
        }
        else if (IoStack->MinorFunction == IRP_MN_QUERY_CAPABILITIES)
        {
            /* query capabilities */
            Status = KspQueryBusDeviceCapabilities(ChildDeviceExtension, Irp);
        }
        else if (IoStack->MinorFunction == IRP_MN_QUERY_PNP_DEVICE_STATE)
        {
            /* query pnp state */
            Status = KspQueryBusDevicePnpState(ChildDeviceExtension, Irp);
        }
        else if (IoStack->MinorFunction == IRP_MN_QUERY_INTERFACE)
        {
            /* query interface */
            Status = KspQueryBusDeviceInterface(ChildDeviceExtension, Irp);
        }
        else if (IoStack->MinorFunction == IRP_MN_QUERY_DEVICE_RELATIONS && IoStack->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation)
        {
            /* handle target device relations */
            ASSERT(Irp->IoStatus.Information == 0);

            /* allocate device relation */
            DeviceRelation = AllocateItem(PagedPool, sizeof(DEVICE_RELATIONS));
            if (DeviceRelation)
            {
                DeviceRelation->Count = 1;
                DeviceRelation->Objects[0] = DeviceObject;

                /* reference self */
                ObReferenceObject(DeviceObject);

                /* store result */
                Irp->IoStatus.Information = (ULONG_PTR)DeviceRelation;

                /* done */
                Status = STATUS_SUCCESS;
            }
            else
            {
                /* no memory */
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else
        {
            /* get default status */
            Status = Irp->IoStatus.Status;
        }
    }

    DPRINT("KsServiceBusEnumPnpRequest %p Bus %u Function %x Status %x\n", DeviceObject, BusDeviceExtension->Common.IsBus, IoStack->MinorFunction, Status);
    Irp->IoStatus.Status = Status;
    return Status;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsRemoveBusEnumInterface(
    IN PIRP Irp)
{
    KPROCESSOR_MODE Mode;
    LUID luid;
    BUS_INSTALL_ENUM_CONTEXT Ctx;
    PDEV_EXTENSION DeviceExtension;
    PBUS_ENUM_DEVICE_EXTENSION BusDeviceExtension;
    PIO_STACK_LOCATION IoStack;

    DPRINT("KsRemoveBusEnumInterface\n");

    /* get io stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* get device extension */
    DeviceExtension = (PDEV_EXTENSION)IoStack->DeviceObject->DeviceExtension;

    /* get bus device extension */
    BusDeviceExtension = DeviceExtension->Ext->BusDeviceExtension;

    /* get previous mode */
    Mode = ExGetPreviousMode();

    /* convert to luid */
    luid = RtlConvertUlongToLuid(SE_LOAD_DRIVER_PRIVILEGE);

    /* perform access check */
    if (!SeSinglePrivilegeCheck(luid, Mode))
    {
        /* insufficient privileges */
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    /* initialize context */
    KeInitializeEvent(&Ctx.Event, NotificationEvent, FALSE);
    Ctx.Irp = Irp;
    Ctx.BusDeviceExtension = BusDeviceExtension;
    ExInitializeWorkItem(&Ctx.WorkItem, KspRemoveBusInterface, (PVOID)&Ctx);

    /* now queue the work item */
    ExQueueWorkItem(&Ctx.WorkItem, DelayedWorkQueue);

    /* wait for completion */
    KeWaitForSingleObject(&Ctx.Event, Executive, KernelMode, FALSE, NULL);

    /* return result */
    return Ctx.Status;
}
