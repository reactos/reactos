/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/io/pnpmgr/pnpinit.c
 * PURPOSE:         PnP Initialization Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

typedef struct _IOPNP_DEVICE_EXTENSION
{
    PWCHAR CompatibleIdList;
    ULONG CompatibleIdListSize;
} IOPNP_DEVICE_EXTENSION, *PIOPNP_DEVICE_EXTENSION;

PUNICODE_STRING PiInitGroupOrderTable;
USHORT PiInitGroupOrderTableCount;
INTERFACE_TYPE PnpDefaultInterfaceType;
BOOLEAN PnPBootDriversLoaded = FALSE;
BOOLEAN PnPBootDriversInitialized = FALSE;

ARBITER_INSTANCE IopRootBusNumberArbiter;
ARBITER_INSTANCE IopRootIrqArbiter;
ARBITER_INSTANCE IopRootDmaArbiter;
ARBITER_INSTANCE IopRootMemArbiter;
ARBITER_INSTANCE IopRootPortArbiter;

extern KEVENT PiEnumerationFinished;

NTSTATUS NTAPI IopPortInitialize(VOID);
NTSTATUS NTAPI IopMemInitialize(VOID);
NTSTATUS NTAPI IopDmaInitialize(VOID);
NTSTATUS NTAPI IopIrqInitialize(VOID);
NTSTATUS NTAPI IopBusNumberInitialize(VOID);

/* FUNCTIONS ******************************************************************/

INTERFACE_TYPE
NTAPI
IopDetermineDefaultInterfaceType(VOID)
{
    /* FIXME: ReactOS doesn't support MicroChannel yet */
    return Isa;
}

NTSTATUS
NTAPI
IopInitializeArbiters(VOID)
{
    NTSTATUS Status;

    Status = IopPortInitialize();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopPortInitialize() return %X\n", Status);
        return Status;
    }

    Status = IopMemInitialize();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopMemInitialize() return %X\n", Status);
        return Status;
    }

    Status = IopDmaInitialize();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopDmaInitialize() return %X\n", Status);
        return Status;
    }

    Status = IopIrqInitialize();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopIrqInitialize() return %X\n", Status);
        return Status;
    }

    Status = IopBusNumberInitialize();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopBusNumberInitialize() return %X\n", Status);
    }

    return Status;
}


CODE_SEG("INIT")
NTSTATUS
NTAPI
PiInitCacheGroupInformation(VOID)
{
    HANDLE KeyHandle;
    NTSTATUS Status;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;
    PUNICODE_STRING GroupTable;
    ULONG Count;
    UNICODE_STRING GroupString =
        RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet"
                            L"\\Control\\ServiceGroupOrder");

    /* Open the registry key */
    Status = IopOpenRegistryKeyEx(&KeyHandle,
                                  NULL,
                                  &GroupString,
                                  KEY_READ);
    if (NT_SUCCESS(Status))
    {
        /* Get the list */
        Status = IopGetRegistryValue(KeyHandle, L"List", &KeyValueInformation);
        ZwClose(KeyHandle);

        /* Make sure we got it */
        if (NT_SUCCESS(Status))
        {
            /* Make sure it's valid */
            if ((KeyValueInformation->Type == REG_MULTI_SZ) &&
                (KeyValueInformation->DataLength))
            {
                /* Convert it to unicode strings */
                Status = PnpRegMultiSzToUnicodeStrings(KeyValueInformation,
                                                       &GroupTable,
                                                       &Count);

                /* Cache it for later */
                PiInitGroupOrderTable = GroupTable;
                PiInitGroupOrderTableCount = (USHORT)Count;
            }
            else
            {
                /* Fail */
                Status = STATUS_UNSUCCESSFUL;
            }

            /* Free the information */
            ExFreePool(KeyValueInformation);
        }
    }

    /* Return status */
    return Status;
}

USHORT
NTAPI
PpInitGetGroupOrderIndex(IN HANDLE ServiceHandle)
{
    NTSTATUS Status;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation;
    USHORT i;
    PVOID Buffer;
    UNICODE_STRING Group;
    PAGED_CODE();

    /* Make sure we have a cache */
    if (!PiInitGroupOrderTable) return -1;

    /* If we don't have a handle, the rest is easy -- return the count */
    if (!ServiceHandle) return PiInitGroupOrderTableCount + 1;

    /* Otherwise, get the group value */
    Status = IopGetRegistryValue(ServiceHandle, L"Group", &KeyValueInformation);
    if (!NT_SUCCESS(Status)) return PiInitGroupOrderTableCount;

    /* Make sure we have a valid string */
    ASSERT(KeyValueInformation->Type == REG_SZ);
    ASSERT(KeyValueInformation->DataLength);

    /* Convert to unicode string */
    Buffer = (PVOID)((ULONG_PTR)KeyValueInformation + KeyValueInformation->DataOffset);
    PnpRegSzToString(Buffer, KeyValueInformation->DataLength, &Group.Length);
    Group.MaximumLength = (USHORT)KeyValueInformation->DataLength;
    Group.Buffer = Buffer;

    /* Loop the groups */
    for (i = 0; i < PiInitGroupOrderTableCount; i++)
    {
        /* Try to find a match */
        if (RtlEqualUnicodeString(&Group, &PiInitGroupOrderTable[i], TRUE)) break;
    }

    /* We're done */
    ExFreePool(KeyValueInformation);
    return i;
}

USHORT
NTAPI
PipGetDriverTagPriority(IN HANDLE ServiceHandle)
{
    NTSTATUS Status;
    HANDLE KeyHandle = NULL;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation = NULL;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformationTag;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformationGroupOrderList;
    PVOID Buffer;
    UNICODE_STRING Group;
    PULONG GroupOrder;
    ULONG Count, Tag = 0;
    USHORT i = -1;
    UNICODE_STRING GroupString =
    RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet"
                        L"\\Control\\ServiceGroupOrder");

    /* Open the key */
    Status = IopOpenRegistryKeyEx(&KeyHandle, NULL, &GroupString, KEY_READ);
    if (!NT_SUCCESS(Status)) goto Quickie;

    /* Read the group */
    Status = IopGetRegistryValue(ServiceHandle, L"Group", &KeyValueInformation);
    if (!NT_SUCCESS(Status)) goto Quickie;

    /* Make sure we have a group */
    if ((KeyValueInformation->Type == REG_SZ) &&
        (KeyValueInformation->DataLength))
    {
        /* Convert to unicode string */
        Buffer = (PVOID)((ULONG_PTR)KeyValueInformation + KeyValueInformation->DataOffset);
        PnpRegSzToString(Buffer, KeyValueInformation->DataLength, &Group.Length);
        Group.MaximumLength = (USHORT)KeyValueInformation->DataLength;
        Group.Buffer = Buffer;
    }

    /* Now read the tag */
    Status = IopGetRegistryValue(ServiceHandle, L"Tag", &KeyValueInformationTag);
    if (!NT_SUCCESS(Status)) goto Quickie;

    /* Make sure we have a tag */
    if ((KeyValueInformationTag->Type == REG_DWORD) &&
        (KeyValueInformationTag->DataLength))
    {
        /* Read it */
        Tag = *(PULONG)((ULONG_PTR)KeyValueInformationTag +
                        KeyValueInformationTag->DataOffset);
    }

    /* We can get rid of this now */
    ExFreePool(KeyValueInformationTag);

    /* Now let's read the group's tag order */
    Status = IopGetRegistryValue(KeyHandle,
                                 Group.Buffer,
                                 &KeyValueInformationGroupOrderList);

    /* We can get rid of this now */
Quickie:
    if (KeyValueInformation) ExFreePool(KeyValueInformation);
    if (KeyHandle) NtClose(KeyHandle);
    if (!NT_SUCCESS(Status)) return -1;

    /* We're on the success path -- validate the tag order*/
    if ((KeyValueInformationGroupOrderList->Type == REG_BINARY) &&
        (KeyValueInformationGroupOrderList->DataLength))
    {
        /* Get the order array */
        GroupOrder = (PULONG)((ULONG_PTR)KeyValueInformationGroupOrderList +
                              KeyValueInformationGroupOrderList->DataOffset);

        /* Get the count */
        Count = *GroupOrder;
        ASSERT(((Count + 1) * sizeof(ULONG)) <=
               KeyValueInformationGroupOrderList->DataLength);

        /* Now loop each tag */
        GroupOrder++;
        for (i = 1; i <= Count; i++)
        {
            /* If we found it, we're out */
            if (Tag == *GroupOrder) break;

            /* Try the next one */
            GroupOrder++;
        }
    }

    /* Last buffer to free */
    ExFreePool(KeyValueInformationGroupOrderList);
    return i;
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
IopInitializePlugPlayServices(VOID)
{
    NTSTATUS Status;
    ULONG Disposition;
    HANDLE KeyHandle, EnumHandle, ParentHandle, TreeHandle, ControlHandle;
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET");
    UNICODE_STRING PnpManagerDriverName = RTL_CONSTANT_STRING(DRIVER_ROOT_NAME L"PnpManager");
    PDEVICE_OBJECT Pdo;

    /* Initialize locks and such */
    KeInitializeSpinLock(&IopDeviceTreeLock);
    KeInitializeSpinLock(&IopDeviceActionLock);
    InitializeListHead(&IopDeviceActionRequestList);
    KeInitializeEvent(&PiEnumerationFinished, NotificationEvent, TRUE);

    /* Get the default interface */
    PnpDefaultInterfaceType = IopDetermineDefaultInterfaceType();

    /* Initialize arbiters */
    Status = IopInitializeArbiters();
    if (!NT_SUCCESS(Status)) return Status;

    /* Setup the group cache */
    Status = PiInitCacheGroupInformation();
    if (!NT_SUCCESS(Status)) return Status;

    /* Open the current control set */
    Status = IopOpenRegistryKeyEx(&KeyHandle,
                                  NULL,
                                  &KeyName,
                                  KEY_ALL_ACCESS);
    if (!NT_SUCCESS(Status)) return Status;

    /* Create the control key */
    RtlInitUnicodeString(&KeyName, L"Control");
    Status = IopCreateRegistryKeyEx(&ControlHandle,
                                    KeyHandle,
                                    &KeyName,
                                    KEY_ALL_ACCESS,
                                    REG_OPTION_NON_VOLATILE,
                                    &Disposition);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check if it's a new key */
    if (Disposition == REG_CREATED_NEW_KEY)
    {
        HANDLE DeviceClassesHandle;

        /* Create the device classes key */
        RtlInitUnicodeString(&KeyName, L"DeviceClasses");
        Status = IopCreateRegistryKeyEx(&DeviceClassesHandle,
                                        ControlHandle,
                                        &KeyName,
                                        KEY_ALL_ACCESS,
                                        REG_OPTION_NON_VOLATILE,
                                        &Disposition);
        if (!NT_SUCCESS(Status)) return Status;

        ZwClose(DeviceClassesHandle);
    }

    ZwClose(ControlHandle);

    /* Create the enum key */
    RtlInitUnicodeString(&KeyName, REGSTR_KEY_ENUM);
    Status = IopCreateRegistryKeyEx(&EnumHandle,
                                    KeyHandle,
                                    &KeyName,
                                    KEY_ALL_ACCESS,
                                    REG_OPTION_NON_VOLATILE,
                                    &Disposition);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check if it's a new key */
    if (Disposition == REG_CREATED_NEW_KEY)
    {
        /* FIXME: DACLs */
    }

    /* Create the root key */
    ParentHandle = EnumHandle;
    RtlInitUnicodeString(&KeyName, REGSTR_KEY_ROOTENUM);
    Status = IopCreateRegistryKeyEx(&EnumHandle,
                                    ParentHandle,
                                    &KeyName,
                                    KEY_ALL_ACCESS,
                                    REG_OPTION_NON_VOLATILE,
                                    &Disposition);
    NtClose(ParentHandle);
    if (!NT_SUCCESS(Status)) return Status;
    NtClose(EnumHandle);

    /* Open the root key now */
    RtlInitUnicodeString(&KeyName, L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\ENUM");
    Status = IopOpenRegistryKeyEx(&EnumHandle,
                                  NULL,
                                  &KeyName,
                                  KEY_ALL_ACCESS);
    if (NT_SUCCESS(Status))
    {
        /* Create the root dev node */
        RtlInitUnicodeString(&KeyName, REGSTR_VAL_ROOT_DEVNODE);
        Status = IopCreateRegistryKeyEx(&TreeHandle,
                                        EnumHandle,
                                        &KeyName,
                                        KEY_ALL_ACCESS,
                                        REG_OPTION_NON_VOLATILE,
                                        NULL);
        NtClose(EnumHandle);
        if (NT_SUCCESS(Status)) NtClose(TreeHandle);
    }

    /* Create the root driver */
    Status = IoCreateDriver(&PnpManagerDriverName, PnpRootDriverEntry);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoCreateDriverObject() failed\n");
        KeBugCheckEx(PHASE1_INITIALIZATION_FAILED, Status, 0, 0, 0);
    }

    /* Create the root PDO */
    Status = IoCreateDevice(IopRootDriverObject,
                            sizeof(IOPNP_DEVICE_EXTENSION),
                            NULL,
                            FILE_DEVICE_CONTROLLER,
                            0,
                            FALSE,
                            &Pdo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoCreateDevice() failed\n");
        KeBugCheckEx(PHASE1_INITIALIZATION_FAILED, Status, 0, 0, 0);
    }

    /* This is a bus enumerated device */
    Pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;

    /* Create the root device node */
    IopRootDeviceNode = PipAllocateDeviceNode(Pdo);

    /* Set flags */
    IopRootDeviceNode->Flags |= DNF_MADEUP | DNF_ENUMERATED |
                                DNF_IDS_QUERIED | DNF_NO_RESOURCE_REQUIRED;

    /* Create instance path */
    if (!RtlCreateUnicodeString(&IopRootDeviceNode->InstancePath, REGSTR_VAL_ROOT_DEVNODE))
    {
        DPRINT1("RtlCreateUnicodeString() failed\n");
        KeBugCheckEx(PHASE1_INITIALIZATION_FAILED, Status, 0, 0, 0);
    }

    /* Call the add device routine */
    IopRootDriverObject->DriverExtension->AddDevice(IopRootDriverObject,
                                                    IopRootDeviceNode->PhysicalDeviceObject);

    PiSetDevNodeState(IopRootDeviceNode, DeviceNodeStarted);

    /* Initialize PnP-Event notification support */
    Status = IopInitPlugPlayEvents();
    if (!NT_SUCCESS(Status)) return Status;

    /* Initialize the Bus Type GUID List */
    PnpBusTypeGuidList = ExAllocatePool(PagedPool, sizeof(IO_BUS_TYPE_GUID_LIST));
    RtlZeroMemory(PnpBusTypeGuidList, sizeof(IO_BUS_TYPE_GUID_LIST));
    ExInitializeFastMutex(&PnpBusTypeGuidList->Lock);

    /* Initialize PnP root relations (this is a syncronous operation) */
    PiQueueDeviceAction(IopRootDeviceNode->PhysicalDeviceObject, PiActionEnumRootDevices, NULL, NULL);

    /* Launch the firmware mapper */
    Status = IopUpdateRootKey();
    if (!NT_SUCCESS(Status)) return Status;

    /* Close the handle to the control set */
    NtClose(KeyHandle);

    /* Initialize PnP root relations (this is a syncronous operation) */
    PiQueueDeviceAction(IopRootDeviceNode->PhysicalDeviceObject, PiActionEnumRootDevices, NULL, NULL);

    /* We made it */
    return STATUS_SUCCESS;
}

/* EOF */
