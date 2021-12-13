/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/pnpmgr/pnproot.c
 * PURPOSE:         PnP manager root device
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                  Copyright 2007 Herv? Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

#define ENUM_NAME_ROOT L"Root"

/* DATA **********************************************************************/

typedef struct _PNPROOT_DEVICE
{
    // Entry on device list
    LIST_ENTRY ListEntry;
    // Physical Device Object of device
    PDEVICE_OBJECT Pdo;
    // Device ID
    UNICODE_STRING DeviceID;
    // Instance ID
    UNICODE_STRING InstanceID;
    // Device description
    UNICODE_STRING DeviceDescription;
    // Resource requirement list
    PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirementsList;
    // Associated resource list
    PCM_RESOURCE_LIST ResourceList;
    ULONG ResourceListSize;
} PNPROOT_DEVICE, *PPNPROOT_DEVICE;

typedef enum
{
    dsStopped,
    dsStarted,
    dsPaused,
    dsRemoved,
    dsSurpriseRemoved
} PNPROOT_DEVICE_STATE;

typedef struct _PNPROOT_COMMON_DEVICE_EXTENSION
{
    // Wether this device extension is for an FDO or PDO
    BOOLEAN IsFDO;
} PNPROOT_COMMON_DEVICE_EXTENSION, *PPNPROOT_COMMON_DEVICE_EXTENSION;

/* Physical Device Object device extension for a child device */
typedef struct _PNPROOT_PDO_DEVICE_EXTENSION
{
    // Common device data
    PNPROOT_COMMON_DEVICE_EXTENSION Common;
    // Informations about the device
    PPNPROOT_DEVICE DeviceInfo;
} PNPROOT_PDO_DEVICE_EXTENSION, *PPNPROOT_PDO_DEVICE_EXTENSION;

/* Physical Device Object device extension for the Root bus device object */
typedef struct _PNPROOT_FDO_DEVICE_EXTENSION
{
    // Common device data
    PNPROOT_COMMON_DEVICE_EXTENSION Common;
    // Lower device object
    PDEVICE_OBJECT Ldo;
    // Current state of the driver
    PNPROOT_DEVICE_STATE State;
    // Namespace device list
    LIST_ENTRY DeviceListHead;
    // Number of (not removed) devices in device list
    ULONG DeviceListCount;
    // Lock for namespace device list
    KGUARDED_MUTEX DeviceListLock;
} PNPROOT_FDO_DEVICE_EXTENSION, *PPNPROOT_FDO_DEVICE_EXTENSION;

typedef struct _BUFFER
{
    PVOID *Data;
    PULONG Length;
} BUFFER, *PBUFFER;

static PDEVICE_OBJECT PnpRootDeviceObject = NULL;

/* FUNCTIONS *****************************************************************/

static NTSTATUS
LocateChildDevice(
    IN PPNPROOT_FDO_DEVICE_EXTENSION DeviceExtension,
    IN PCUNICODE_STRING DeviceId,
    IN PCWSTR InstanceId,
    OUT PPNPROOT_DEVICE* ChildDevice OPTIONAL)
{
    PPNPROOT_DEVICE Device;
    UNICODE_STRING InstanceIdU;
    PLIST_ENTRY NextEntry;

    /* Initialize the string to compare */
    RtlInitUnicodeString(&InstanceIdU, InstanceId);

    /* Start looping */
    for (NextEntry = DeviceExtension->DeviceListHead.Flink;
         NextEntry != &DeviceExtension->DeviceListHead;
         NextEntry = NextEntry->Flink)
    {
        /* Get the entry */
        Device = CONTAINING_RECORD(NextEntry, PNPROOT_DEVICE, ListEntry);

        /* See if the strings match */
        if (RtlEqualUnicodeString(DeviceId, &Device->DeviceID, TRUE) &&
            RtlEqualUnicodeString(&InstanceIdU, &Device->InstanceID, TRUE))
        {
            /* They do, so set the pointer and return success */
            if (ChildDevice)
                *ChildDevice = Device;
            return STATUS_SUCCESS;
        }
    }

    /* No device found */
    return STATUS_NO_SUCH_DEVICE;
}

NTSTATUS
PnpRootRegisterDevice(
    IN PDEVICE_OBJECT DeviceObject)
{
    PPNPROOT_FDO_DEVICE_EXTENSION DeviceExtension = PnpRootDeviceObject->DeviceExtension;
    PPNPROOT_DEVICE Device;
    PDEVICE_NODE DeviceNode;
    PWSTR InstancePath;
    UNICODE_STRING InstancePathCopy;

    Device = ExAllocatePoolWithTag(PagedPool, sizeof(PNPROOT_DEVICE), TAG_PNP_ROOT);
    if (!Device) return STATUS_NO_MEMORY;

    DeviceNode = IopGetDeviceNode(DeviceObject);
    if (!RtlCreateUnicodeString(&InstancePathCopy, DeviceNode->InstancePath.Buffer))
    {
        ExFreePoolWithTag(Device, TAG_PNP_ROOT);
        return STATUS_NO_MEMORY;
    }

    InstancePath = wcsrchr(InstancePathCopy.Buffer, L'\\');
    ASSERT(InstancePath);

    if (!RtlCreateUnicodeString(&Device->InstanceID, InstancePath + 1))
    {
        RtlFreeUnicodeString(&InstancePathCopy);
        ExFreePoolWithTag(Device, TAG_PNP_ROOT);
        return STATUS_NO_MEMORY;
    }

    InstancePath[0] = UNICODE_NULL;

    if (!RtlCreateUnicodeString(&Device->DeviceID, InstancePathCopy.Buffer))
    {
        RtlFreeUnicodeString(&InstancePathCopy);
        RtlFreeUnicodeString(&Device->InstanceID);
        ExFreePoolWithTag(Device, TAG_PNP_ROOT);
        return STATUS_NO_MEMORY;
    }

    InstancePath[0] = L'\\';

    Device->Pdo = DeviceObject;

    KeAcquireGuardedMutex(&DeviceExtension->DeviceListLock);
    InsertTailList(&DeviceExtension->DeviceListHead,
                   &Device->ListEntry);
    DeviceExtension->DeviceListCount++;
    KeReleaseGuardedMutex(&DeviceExtension->DeviceListLock);

    RtlFreeUnicodeString(&InstancePathCopy);

    return STATUS_SUCCESS;
}

/* Creates a new PnP device for a legacy driver */
NTSTATUS
PnpRootCreateDevice(
    IN PUNICODE_STRING ServiceName,
    IN OPTIONAL PDRIVER_OBJECT DriverObject,
    OUT PDEVICE_OBJECT *PhysicalDeviceObject,
    OUT OPTIONAL PUNICODE_STRING FullInstancePath)
{
    PPNPROOT_FDO_DEVICE_EXTENSION DeviceExtension;
    PPNPROOT_PDO_DEVICE_EXTENSION PdoDeviceExtension;
    UNICODE_STRING DevicePath;
    WCHAR InstancePath[5];
    PPNPROOT_DEVICE Device = NULL;
    NTSTATUS Status;
    UNICODE_STRING PathSep = RTL_CONSTANT_STRING(L"\\");
    ULONG NextInstance;
    UNICODE_STRING EnumKeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\" REGSTR_PATH_SYSTEMENUM);
    HANDLE EnumHandle, DeviceKeyHandle = NULL, InstanceKeyHandle;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    OBJECT_ATTRIBUTES ObjectAttributes;

    DeviceExtension = PnpRootDeviceObject->DeviceExtension;
    KeAcquireGuardedMutex(&DeviceExtension->DeviceListLock);

    DPRINT("Creating a PnP root device for service '%wZ'\n", ServiceName);

    DevicePath.Length = 0;
    DevicePath.MaximumLength = sizeof(REGSTR_KEY_ROOTENUM) + sizeof(L'\\') + ServiceName->Length;
    DevicePath.Buffer = ExAllocatePoolWithTag(PagedPool,
                                              DevicePath.MaximumLength,
                                              TAG_PNP_ROOT);
    if (DevicePath.Buffer == NULL)
    {
        DPRINT1("ExAllocatePoolWithTag() failed\n");
        Status = STATUS_NO_MEMORY;
        goto cleanup;
    }
    RtlAppendUnicodeToString(&DevicePath, REGSTR_KEY_ROOTENUM L"\\");
    RtlAppendUnicodeStringToString(&DevicePath, ServiceName);

    /* Initialize a PNPROOT_DEVICE structure */
    Device = ExAllocatePoolWithTag(PagedPool, sizeof(PNPROOT_DEVICE), TAG_PNP_ROOT);
    if (!Device)
    {
        DPRINT("ExAllocatePoolWithTag() failed\n");
        Status = STATUS_NO_MEMORY;
        goto cleanup;
    }
    RtlZeroMemory(Device, sizeof(PNPROOT_DEVICE));
    Device->DeviceID = DevicePath;
    RtlInitEmptyUnicodeString(&DevicePath, NULL, 0);

    Status = IopOpenRegistryKeyEx(&EnumHandle, NULL, &EnumKeyName, KEY_READ);
    if (NT_SUCCESS(Status))
    {
        InitializeObjectAttributes(&ObjectAttributes,
                                   &Device->DeviceID,
                                   OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                                   EnumHandle,
                                   NULL);
        Status = ZwCreateKey(&DeviceKeyHandle, KEY_SET_VALUE, &ObjectAttributes, 0, NULL, REG_OPTION_NON_VOLATILE, NULL);
        ObCloseHandle(EnumHandle, KernelMode);
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open registry key\n");
        goto cleanup;
    }

tryagain:
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].Name = L"NextInstance";
    QueryTable[0].EntryContext = &NextInstance;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;

    Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                    (PWSTR)DeviceKeyHandle,
                                    QueryTable,
                                    NULL,
                                    NULL);
    for (NextInstance = 0; NextInstance <= 9999; NextInstance++)
    {
        _snwprintf(InstancePath, sizeof(InstancePath) / sizeof(WCHAR), L"%04lu", NextInstance);
        Status = LocateChildDevice(DeviceExtension, &Device->DeviceID, InstancePath, NULL);
        if (Status == STATUS_NO_SUCH_DEVICE)
            break;
    }

    if (NextInstance > 9999)
    {
        DPRINT1("Too many legacy devices reported for service '%wZ'\n", ServiceName);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    _snwprintf(InstancePath, sizeof(InstancePath) / sizeof(WCHAR), L"%04lu", NextInstance);
    Status = LocateChildDevice(DeviceExtension, &Device->DeviceID, InstancePath, NULL);
    if (Status != STATUS_NO_SUCH_DEVICE || NextInstance > 9999)
    {
        DPRINT1("NextInstance value is corrupt! (%lu)\n", NextInstance);
        RtlDeleteRegistryValue(RTL_REGISTRY_HANDLE,
                               (PWSTR)DeviceKeyHandle,
                               L"NextInstance");
        goto tryagain;
    }

    NextInstance++;
    Status = RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
                                   (PWSTR)DeviceKeyHandle,
                                   L"NextInstance",
                                   REG_DWORD,
                                   &NextInstance,
                                   sizeof(NextInstance));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to write new NextInstance value! (0x%x)\n", Status);
        goto cleanup;
    }

    if (!RtlCreateUnicodeString(&Device->InstanceID, InstancePath))
    {
        Status = STATUS_NO_MEMORY;
        goto cleanup;
    }

    /* Finish creating the instance path in the registry */
    InitializeObjectAttributes(&ObjectAttributes,
                               &Device->InstanceID,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               DeviceKeyHandle,
                               NULL);
    Status = ZwCreateKey(&InstanceKeyHandle, KEY_QUERY_VALUE, &ObjectAttributes, 0, NULL, REG_OPTION_NON_VOLATILE, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create instance path (0x%x)\n", Status);
        goto cleanup;
    }

    /* Just close the handle */
    ObCloseHandle(InstanceKeyHandle, KernelMode);

    if (FullInstancePath)
    {
        FullInstancePath->MaximumLength = Device->DeviceID.Length + PathSep.Length + Device->InstanceID.Length;
        FullInstancePath->Length = 0;
        FullInstancePath->Buffer = ExAllocatePool(PagedPool, FullInstancePath->MaximumLength);
        if (!FullInstancePath->Buffer)
        {
            Status = STATUS_NO_MEMORY;
            goto cleanup;
        }

        RtlAppendUnicodeStringToString(FullInstancePath, &Device->DeviceID);
        RtlAppendUnicodeStringToString(FullInstancePath, &PathSep);
        RtlAppendUnicodeStringToString(FullInstancePath, &Device->InstanceID);
    }

    /* Initialize a device object */
    Status = IoCreateDevice(
        DriverObject ? DriverObject : PnpRootDeviceObject->DriverObject,
        sizeof(PNPROOT_PDO_DEVICE_EXTENSION),
        NULL,
        FILE_DEVICE_CONTROLLER,
        FILE_AUTOGENERATED_DEVICE_NAME,
        FALSE,
        &Device->Pdo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IoCreateDevice() failed with status 0x%08lx\n", Status);
        Status = STATUS_NO_MEMORY;
        goto cleanup;
    }

    PdoDeviceExtension = (PPNPROOT_PDO_DEVICE_EXTENSION)Device->Pdo->DeviceExtension;
    RtlZeroMemory(PdoDeviceExtension, sizeof(PNPROOT_PDO_DEVICE_EXTENSION));
    PdoDeviceExtension->Common.IsFDO = FALSE;
    PdoDeviceExtension->DeviceInfo = Device;

    Device->Pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;
    Device->Pdo->Flags &= ~DO_DEVICE_INITIALIZING;

    InsertTailList(
        &DeviceExtension->DeviceListHead,
        &Device->ListEntry);
    DeviceExtension->DeviceListCount++;

    *PhysicalDeviceObject = Device->Pdo;
    DPRINT("Created PDO %p (%wZ\\%wZ)\n", *PhysicalDeviceObject, &Device->DeviceID, &Device->InstanceID);
    Device = NULL;
    Status = STATUS_SUCCESS;

cleanup:
    KeReleaseGuardedMutex(&DeviceExtension->DeviceListLock);
    if (Device)
    {
        if (Device->Pdo)
            IoDeleteDevice(Device->Pdo);
        RtlFreeUnicodeString(&Device->DeviceID);
        RtlFreeUnicodeString(&Device->InstanceID);
        ExFreePoolWithTag(Device, TAG_PNP_ROOT);
    }
    RtlFreeUnicodeString(&DevicePath);
    if (DeviceKeyHandle != NULL)
        ObCloseHandle(DeviceKeyHandle, KernelMode);
    return Status;
}

static NTSTATUS NTAPI
QueryStringCallback(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext)
{
    PUNICODE_STRING Destination = (PUNICODE_STRING)EntryContext;
    UNICODE_STRING Source;

    if (ValueType != REG_SZ || ValueLength == 0 || ValueLength % sizeof(WCHAR) != 0)
    {
        Destination->Length = 0;
        Destination->MaximumLength = 0;
        Destination->Buffer = NULL;
        return STATUS_SUCCESS;
    }

    Source.MaximumLength = Source.Length = (USHORT)ValueLength;
    Source.Buffer = ValueData;

    return RtlDuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE, &Source, Destination);
}

static NTSTATUS NTAPI
QueryBinaryValueCallback(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext)
{
    PBUFFER Buffer = (PBUFFER)EntryContext;
    PVOID BinaryValue;

    if (ValueLength == 0)
    {
        *Buffer->Data = NULL;
        return STATUS_SUCCESS;
    }

    BinaryValue = ExAllocatePoolWithTag(PagedPool, ValueLength, TAG_PNP_ROOT);
    if (BinaryValue == NULL)
        return STATUS_NO_MEMORY;
    RtlCopyMemory(BinaryValue, ValueData, ValueLength);
    *Buffer->Data = BinaryValue;
    if (Buffer->Length) *Buffer->Length = ValueLength;
    return STATUS_SUCCESS;
}

static
NTSTATUS
CreateDeviceFromRegistry(
    _Inout_ PPNPROOT_FDO_DEVICE_EXTENSION DeviceExtension,
    _Inout_ PUNICODE_STRING DevicePath,
    _In_ PCWSTR InstanceId,
    _In_ HANDLE SubKeyHandle)
{
    NTSTATUS Status;
    PPNPROOT_DEVICE Device;
    HANDLE DeviceKeyHandle = NULL;
    RTL_QUERY_REGISTRY_TABLE QueryTable[4];
    BUFFER Buffer1, Buffer2;

    /* If the device already exists, there's nothing to do */
    Status = LocateChildDevice(DeviceExtension, DevicePath, InstanceId, &Device);
    if (Status != STATUS_NO_SUCH_DEVICE)
    {
        return STATUS_SUCCESS;
    }

    /* Create a PPNPROOT_DEVICE object, and add it to the list of known devices */
    Device = ExAllocatePoolWithTag(PagedPool, sizeof(PNPROOT_DEVICE), TAG_PNP_ROOT);
    if (!Device)
    {
        DPRINT("ExAllocatePoolWithTag() failed\n");
        Status = STATUS_NO_MEMORY;
        goto cleanup;
    }
    RtlZeroMemory(Device, sizeof(PNPROOT_DEVICE));

    /* Fill device ID and instance ID */
    Device->DeviceID = *DevicePath;
    RtlInitEmptyUnicodeString(DevicePath, NULL, 0);
    if (!RtlCreateUnicodeString(&Device->InstanceID, InstanceId))
    {
        DPRINT1("RtlCreateUnicodeString() failed\n");
        Status = STATUS_NO_MEMORY;
        goto cleanup;
    }

    /* Open registry key to fill other informations */
    Status = IopOpenRegistryKeyEx(&DeviceKeyHandle, SubKeyHandle, &Device->InstanceID, KEY_READ);
    if (!NT_SUCCESS(Status))
    {
        /* If our key disappeared, let the caller go on */
        DPRINT1("IopOpenRegistryKeyEx() failed for '%wZ' with status 0x%lx\n",
                &Device->InstanceID, Status);
        Status = STATUS_SUCCESS;
        goto cleanup;
    }

    /* Fill information from the device instance key */
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = QueryStringCallback;
    QueryTable[0].Name = L"DeviceDesc";
    QueryTable[0].EntryContext = &Device->DeviceDescription;

    RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                           (PCWSTR)DeviceKeyHandle,
                           QueryTable,
                           NULL,
                           NULL);

    /* Fill information from the LogConf subkey */
    Buffer1.Data = (PVOID *)&Device->ResourceRequirementsList;
    Buffer1.Length = NULL;
    Buffer2.Data = (PVOID *)&Device->ResourceList;
    Buffer2.Length = &Device->ResourceListSize;
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
    QueryTable[0].Name = L"LogConf";
    QueryTable[1].QueryRoutine = QueryBinaryValueCallback;
    QueryTable[1].Name = L"BasicConfigVector";
    QueryTable[1].EntryContext = &Buffer1;
    QueryTable[2].QueryRoutine = QueryBinaryValueCallback;
    QueryTable[2].Name = L"BootConfig";
    QueryTable[2].EntryContext = &Buffer2;

    if (!NT_SUCCESS(RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                           (PCWSTR)DeviceKeyHandle,
                                           QueryTable,
                                           NULL,
                                           NULL)))
    {
        /* Non-fatal error */
        DPRINT1("Failed to read the LogConf key for %wZ\\%S\n", &Device->DeviceID, InstanceId);
    }

    /* Insert the newly created device into the list */
    InsertTailList(&DeviceExtension->DeviceListHead,
                   &Device->ListEntry);
    DeviceExtension->DeviceListCount++;
    Device = NULL;

cleanup:
    if (DeviceKeyHandle != NULL)
    {
        ZwClose(DeviceKeyHandle);
    }
    if (Device != NULL)
    {
        /* We have a device that has not been added to device list. We need to clean it up */
        RtlFreeUnicodeString(&Device->DeviceID);
        RtlFreeUnicodeString(&Device->InstanceID);
        ExFreePoolWithTag(Device, TAG_PNP_ROOT);
    }
    return Status;
}

static NTSTATUS
IopShouldProcessDevice(
    IN HANDLE SubKey,
    IN PCWSTR InstanceID)
{
    UNICODE_STRING DeviceReportedValue = RTL_CONSTANT_STRING(L"DeviceReported");
    UNICODE_STRING Control = RTL_CONSTANT_STRING(L"Control");
    UNICODE_STRING InstanceIDU;
    PKEY_VALUE_FULL_INFORMATION pKeyValueFullInformation;
    HANDLE InstanceKey, ControlKey;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG Size, DeviceReported, ResultLength;
    NTSTATUS Status;

    Size = 128;
    pKeyValueFullInformation = ExAllocatePool(PagedPool, Size);
    if (!pKeyValueFullInformation)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Open Instance key */
    RtlInitUnicodeString(&InstanceIDU, InstanceID);
    InitializeObjectAttributes(&ObjectAttributes,
                               &InstanceIDU,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               SubKey,
                               NULL);
    Status = ZwOpenKey(&InstanceKey,
                       KEY_QUERY_VALUE,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        ExFreePool(pKeyValueFullInformation);
        return Status;
    }

    /* Read 'DeviceReported' Key */
    Status = ZwQueryValueKey(InstanceKey, &DeviceReportedValue, KeyValueFullInformation, pKeyValueFullInformation, Size, &ResultLength);
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        ZwClose(InstanceKey);
        ExFreePool(pKeyValueFullInformation);
        DPRINT("No 'DeviceReported' value\n");
        return STATUS_SUCCESS;
    }
    else if (!NT_SUCCESS(Status))
    {
        ZwClose(InstanceKey);
        ExFreePool(pKeyValueFullInformation);
        return Status;
    }
    if (pKeyValueFullInformation->Type != REG_DWORD || pKeyValueFullInformation->DataLength != sizeof(DeviceReported))
    {
        ZwClose(InstanceKey);
        ExFreePool(pKeyValueFullInformation);
        return STATUS_UNSUCCESSFUL;
    }
    RtlCopyMemory(&DeviceReported, (PVOID)((ULONG_PTR)pKeyValueFullInformation + pKeyValueFullInformation->DataOffset), sizeof(DeviceReported));
    /* FIXME: Check DeviceReported value? */
    ASSERT(DeviceReported == 1);

    /* Open Control key */
    InitializeObjectAttributes(&ObjectAttributes,
                               &Control,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               InstanceKey,
                               NULL);
    Status = ZwOpenKey(&ControlKey,
                       KEY_QUERY_VALUE,
                       &ObjectAttributes);
    ZwClose(InstanceKey);
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        DPRINT("No 'Control' key\n");
        return STATUS_NO_SUCH_DEVICE;
    }
    else if (!NT_SUCCESS(Status))
    {
        ExFreePool(pKeyValueFullInformation);
        return Status;
    }

    /* Read 'DeviceReported' Key */
    Status = ZwQueryValueKey(ControlKey, &DeviceReportedValue, KeyValueFullInformation, pKeyValueFullInformation, Size, &ResultLength);
    ZwClose(ControlKey);
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        ExFreePool(pKeyValueFullInformation);
        DPRINT("No 'DeviceReported' value\n");
        return STATUS_NO_SUCH_DEVICE;
    }
    else if (!NT_SUCCESS(Status))
    {
        ExFreePool(pKeyValueFullInformation);
        return Status;
    }
    if (pKeyValueFullInformation->Type != REG_DWORD || pKeyValueFullInformation->DataLength != sizeof(DeviceReported))
    {
        ExFreePool(pKeyValueFullInformation);
        return STATUS_UNSUCCESSFUL;
    }
    RtlCopyMemory(&DeviceReported, (PVOID)((ULONG_PTR)pKeyValueFullInformation + pKeyValueFullInformation->DataOffset), sizeof(DeviceReported));
    /* FIXME: Check DeviceReported value? */
    ASSERT(DeviceReported == 1);

    ExFreePool(pKeyValueFullInformation);
    return STATUS_SUCCESS;
}

static NTSTATUS
EnumerateDevices(
    IN PDEVICE_OBJECT DeviceObject)
{
    PPNPROOT_FDO_DEVICE_EXTENSION DeviceExtension;
    PKEY_BASIC_INFORMATION KeyInfo = NULL, SubKeyInfo = NULL;
    UNICODE_STRING LegacyU = RTL_CONSTANT_STRING(L"LEGACY_");
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\" REGSTR_PATH_SYSTEMENUM L"\\" REGSTR_KEY_ROOTENUM);
    UNICODE_STRING SubKeyName;
    UNICODE_STRING DevicePath;
    HANDLE KeyHandle = NULL;
    HANDLE SubKeyHandle = NULL;
    ULONG KeyInfoSize, SubKeyInfoSize;
    ULONG ResultSize;
    ULONG Index1, Index2;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    DPRINT("EnumerateDevices(FDO %p)\n", DeviceObject);

    DeviceExtension = (PPNPROOT_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    KeAcquireGuardedMutex(&DeviceExtension->DeviceListLock);

    /* Should hold most key names, but we reallocate below if it's too small */
    KeyInfoSize = FIELD_OFFSET(KEY_BASIC_INFORMATION, Name) + 64 * sizeof(WCHAR);
    KeyInfo = ExAllocatePoolWithTag(PagedPool,
                                    KeyInfoSize + sizeof(UNICODE_NULL),
                                    TAG_PNP_ROOT);
    if (!KeyInfo)
    {
        DPRINT("ExAllocatePoolWithTag() failed\n");
        Status = STATUS_NO_MEMORY;
        goto cleanup;
    }
    SubKeyInfoSize = KeyInfoSize;
    SubKeyInfo = ExAllocatePoolWithTag(PagedPool,
                                       SubKeyInfoSize + sizeof(UNICODE_NULL),
                                       TAG_PNP_ROOT);
    if (!SubKeyInfo)
    {
        DPRINT("ExAllocatePoolWithTag() failed\n");
        Status = STATUS_NO_MEMORY;
        goto cleanup;
    }

    Status = IopOpenRegistryKeyEx(&KeyHandle, NULL, &KeyName, KEY_ENUMERATE_SUB_KEYS);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopOpenRegistryKeyEx(%wZ) failed with status 0x%08lx\n", &KeyName, Status);
        goto cleanup;
    }

    /* Devices are sub-sub-keys of 'KeyName'. KeyName is already opened as
     * KeyHandle. We'll first do a first enumeration to have first level keys,
     * and an inner one to have the real devices list.
     */
    Index1 = 0;
    while (TRUE)
    {
        Status = ZwEnumerateKey(
            KeyHandle,
            Index1,
            KeyBasicInformation,
            KeyInfo,
            KeyInfoSize,
            &ResultSize);
        if (Status == STATUS_NO_MORE_ENTRIES)
        {
            Status = STATUS_SUCCESS;
            break;
        }
        else if (Status == STATUS_BUFFER_OVERFLOW ||
                 Status == STATUS_BUFFER_TOO_SMALL)
        {
            ASSERT(KeyInfoSize < ResultSize);
            KeyInfoSize = ResultSize;
            ExFreePoolWithTag(KeyInfo, TAG_PNP_ROOT);
            KeyInfo = ExAllocatePoolWithTag(PagedPool,
                                            KeyInfoSize + sizeof(UNICODE_NULL),
                                            TAG_PNP_ROOT);
            if (!KeyInfo)
            {
                DPRINT1("ExAllocatePoolWithTag(%lu) failed\n", KeyInfoSize);
                Status = STATUS_NO_MEMORY;
                goto cleanup;
            }
            continue;
        }
        else if (!NT_SUCCESS(Status))
        {
            DPRINT("ZwEnumerateKey() failed with status 0x%08lx\n", Status);
            goto cleanup;
        }

        /* Terminate the string */
        KeyInfo->Name[KeyInfo->NameLength / sizeof(WCHAR)] = 0;

        /* Check if it is a legacy driver */
        RtlInitUnicodeString(&SubKeyName, KeyInfo->Name);
        if (RtlPrefixUnicodeString(&LegacyU, &SubKeyName, FALSE))
        {
            DPRINT("Ignoring legacy driver '%wZ'\n", &SubKeyName);
            Index1++;
            continue;
        }

        /* Open the key */
        Status = IopOpenRegistryKeyEx(&SubKeyHandle, KeyHandle, &SubKeyName, KEY_ENUMERATE_SUB_KEYS);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("IopOpenRegistryKeyEx() failed for '%wZ' with status 0x%lx\n",
                   &SubKeyName, Status);
            break;
        }

        /* Enumerate the sub-keys */
        Index2 = 0;
        while (TRUE)
        {
            Status = ZwEnumerateKey(
                SubKeyHandle,
                Index2,
                KeyBasicInformation,
                SubKeyInfo,
                SubKeyInfoSize,
                &ResultSize);
            if (Status == STATUS_NO_MORE_ENTRIES)
            {
                break;
            }
            else if (Status == STATUS_BUFFER_OVERFLOW ||
                     Status == STATUS_BUFFER_TOO_SMALL)
            {
                ASSERT(SubKeyInfoSize < ResultSize);
                SubKeyInfoSize = ResultSize;
                ExFreePoolWithTag(SubKeyInfo, TAG_PNP_ROOT);
                SubKeyInfo = ExAllocatePoolWithTag(PagedPool,
                                                   SubKeyInfoSize + sizeof(UNICODE_NULL),
                                                   TAG_PNP_ROOT);
                if (!SubKeyInfo)
                {
                    DPRINT1("ExAllocatePoolWithTag(%lu) failed\n", SubKeyInfoSize);
                    Status = STATUS_NO_MEMORY;
                    goto cleanup;
                }
                continue;
            }
            else if (!NT_SUCCESS(Status))
            {
                DPRINT("ZwEnumerateKey() failed with status 0x%08lx\n", Status);
                break;
            }

            /* Terminate the string */
            SubKeyInfo->Name[SubKeyInfo->NameLength / sizeof(WCHAR)] = 0;

            /* Compute device ID */
            DevicePath.Length = 0;
            DevicePath.MaximumLength = sizeof(REGSTR_KEY_ROOTENUM) + sizeof(L'\\') + SubKeyName.Length;
            DevicePath.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                      DevicePath.MaximumLength,
                                                      TAG_PNP_ROOT);
            if (DevicePath.Buffer == NULL)
            {
                DPRINT1("ExAllocatePoolWithTag() failed\n");
                Status = STATUS_NO_MEMORY;
                goto cleanup;
            }

            RtlAppendUnicodeToString(&DevicePath, REGSTR_KEY_ROOTENUM L"\\");
            RtlAppendUnicodeStringToString(&DevicePath, &SubKeyName);
            DPRINT("Found device %wZ\\%S!\n", &DevicePath, SubKeyInfo->Name);

            Status = IopShouldProcessDevice(SubKeyHandle, SubKeyInfo->Name);
            if (NT_SUCCESS(Status))
            {
                Status = CreateDeviceFromRegistry(DeviceExtension,
                                                  &DevicePath,
                                                  SubKeyInfo->Name,
                                                  SubKeyHandle);

                /* If CreateDeviceFromRegistry didn't take ownership and zero this,
                 * we need to free it
                 */
                RtlFreeUnicodeString(&DevicePath);

                if (!NT_SUCCESS(Status))
                {
                    goto cleanup;
                }
            }
            else if (Status == STATUS_NO_SUCH_DEVICE)
            {
                DPRINT("Skipping device %wZ\\%S (not reported yet)\n", &DevicePath, SubKeyInfo->Name);
            }
            else
            {
                goto cleanup;
            }

            Index2++;
        }

        ZwClose(SubKeyHandle);
        SubKeyHandle = NULL;
        Index1++;
    }

cleanup:
    if (SubKeyHandle != NULL)
        ZwClose(SubKeyHandle);
    if (KeyHandle != NULL)
        ZwClose(KeyHandle);
    if (KeyInfo)
        ExFreePoolWithTag(KeyInfo, TAG_PNP_ROOT);
    if (SubKeyInfo)
        ExFreePoolWithTag(SubKeyInfo, TAG_PNP_ROOT);
    KeReleaseGuardedMutex(&DeviceExtension->DeviceListLock);
    return Status;
}

/* FUNCTION: Handle IRP_MN_QUERY_DEVICE_RELATIONS IRPs for the root bus device object
 * ARGUMENTS:
 *     DeviceObject = Pointer to functional device object of the root bus driver
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
static NTSTATUS
PnpRootQueryDeviceRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PPNPROOT_PDO_DEVICE_EXTENSION PdoDeviceExtension;
    PPNPROOT_FDO_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_RELATIONS Relations = NULL, OtherRelations = (PDEVICE_RELATIONS)Irp->IoStatus.Information;
    PPNPROOT_DEVICE Device = NULL;
    ULONG Size;
    NTSTATUS Status;
    PLIST_ENTRY NextEntry;

    DPRINT("PnpRootQueryDeviceRelations(FDO %p, Irp %p)\n", DeviceObject, Irp);

    Status = EnumerateDevices(DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("EnumerateDevices() failed with status 0x%08lx\n", Status);
        return Status;
    }

    DeviceExtension = (PPNPROOT_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    Size = FIELD_OFFSET(DEVICE_RELATIONS, Objects) + sizeof(PDEVICE_OBJECT) * DeviceExtension->DeviceListCount;
    if (OtherRelations)
    {
        /* Another bus driver has already created a DEVICE_RELATIONS
         * structure so we must merge this structure with our own */

        Size += sizeof(PDEVICE_OBJECT) * OtherRelations->Count;
    }
    Relations = (PDEVICE_RELATIONS)ExAllocatePool(PagedPool, Size);
    if (!Relations)
    {
        DPRINT("ExAllocatePoolWithTag() failed\n");
        Status = STATUS_NO_MEMORY;
        goto cleanup;
    }
    RtlZeroMemory(Relations, Size);
    if (OtherRelations)
    {
        Relations->Count = OtherRelations->Count;
        RtlCopyMemory(Relations->Objects, OtherRelations->Objects, sizeof(PDEVICE_OBJECT) * OtherRelations->Count);
    }

    KeAcquireGuardedMutex(&DeviceExtension->DeviceListLock);

    /* Start looping */
    for (NextEntry = DeviceExtension->DeviceListHead.Flink;
         NextEntry != &DeviceExtension->DeviceListHead;
         NextEntry = NextEntry->Flink)
    {
        /* Get the entry */
        Device = CONTAINING_RECORD(NextEntry, PNPROOT_DEVICE, ListEntry);

        if (!Device->Pdo)
        {
            /* Create a physical device object for the
             * device as it does not already have one */
            Status = IoCreateDevice(
                DeviceObject->DriverObject,
                sizeof(PNPROOT_PDO_DEVICE_EXTENSION),
                NULL,
                FILE_DEVICE_CONTROLLER,
                FILE_AUTOGENERATED_DEVICE_NAME,
                FALSE,
                &Device->Pdo);
            if (!NT_SUCCESS(Status))
            {
                DPRINT("IoCreateDevice() failed with status 0x%08lx\n", Status);
                break;
            }

            PdoDeviceExtension = (PPNPROOT_PDO_DEVICE_EXTENSION)Device->Pdo->DeviceExtension;
            RtlZeroMemory(PdoDeviceExtension, sizeof(PNPROOT_PDO_DEVICE_EXTENSION));
            PdoDeviceExtension->Common.IsFDO = FALSE;
            PdoDeviceExtension->DeviceInfo = Device;

            Device->Pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;
            Device->Pdo->Flags &= ~DO_DEVICE_INITIALIZING;
        }

        /* Reference the physical device object. The PnP manager
         will dereference it again when it is no longer needed */
        ObReferenceObject(Device->Pdo);

        Relations->Objects[Relations->Count++] = Device->Pdo;
    }
    KeReleaseGuardedMutex(&DeviceExtension->DeviceListLock);

    Irp->IoStatus.Information = (ULONG_PTR)Relations;

cleanup:
    if (!NT_SUCCESS(Status))
    {
        if (OtherRelations)
            ExFreePool(OtherRelations);
        if (Relations)
            ExFreePool(Relations);
        if (Device && Device->Pdo)
        {
            IoDeleteDevice(Device->Pdo);
            Device->Pdo = NULL;
        }
    }

    return Status;
}

/*
 * FUNCTION: Handle Plug and Play IRPs for the root bus device object
 * ARGUMENTS:
 *     DeviceObject = Pointer to functional device object of the root bus driver
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
static NTSTATUS
PnpRootFdoPnpControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PPNPROOT_FDO_DEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;

    DeviceExtension = (PPNPROOT_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    Status = Irp->IoStatus.Status;
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (IrpSp->MinorFunction)
    {
        case IRP_MN_QUERY_DEVICE_RELATIONS:
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS\n");
            Status = PnpRootQueryDeviceRelations(DeviceObject, Irp);
            break;

        case IRP_MN_START_DEVICE:
            DPRINT("IRP_MJ_PNP / IRP_MN_START_DEVICE\n");
            if (!IoForwardIrpSynchronously(DeviceExtension->Ldo, Irp))
                Status = STATUS_UNSUCCESSFUL;
            else
            {
                Status = Irp->IoStatus.Status;
                if (NT_SUCCESS(Status))
                    DeviceExtension->State = dsStarted;
            }

            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;

         case IRP_MN_STOP_DEVICE:
             DPRINT("IRP_MJ_PNP / IRP_MN_STOP_DEVICE\n");
             /* Root device cannot be stopped */
             Irp->IoStatus.Status = Status = STATUS_INVALID_DEVICE_REQUEST;
             IoCompleteRequest(Irp, IO_NO_INCREMENT);
             return Status;

        default:
            DPRINT("IRP_MJ_PNP / Unknown minor function 0x%lx\n", IrpSp->MinorFunction);
            break;
    }

    if (Status != STATUS_PENDING)
    {
       Irp->IoStatus.Status = Status;
       IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return Status;
}

static NTSTATUS
PdoQueryDeviceRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp)
{
    PDEVICE_RELATIONS Relations;
    NTSTATUS Status = Irp->IoStatus.Status;

    if (IrpSp->Parameters.QueryDeviceRelations.Type != TargetDeviceRelation)
        return Status;

    DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / TargetDeviceRelation\n");
    Relations = (PDEVICE_RELATIONS)ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS));
    if (!Relations)
    {
        DPRINT("ExAllocatePoolWithTag() failed\n");
        Status = STATUS_NO_MEMORY;
    }
    else
    {
        ObReferenceObject(DeviceObject);
        Relations->Count = 1;
        Relations->Objects[0] = DeviceObject;
        Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = (ULONG_PTR)Relations;
    }

    return Status;
}

static NTSTATUS
PdoQueryCapabilities(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp)
{
    PDEVICE_CAPABILITIES DeviceCapabilities;

    DeviceCapabilities = IrpSp->Parameters.DeviceCapabilities.Capabilities;

    if (DeviceCapabilities->Version != 1)
        return STATUS_REVISION_MISMATCH;

    DeviceCapabilities->UniqueID = TRUE;
    /* FIXME: Fill other fields */

    return STATUS_SUCCESS;
}

static NTSTATUS
PdoQueryResources(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp)
{
    PPNPROOT_PDO_DEVICE_EXTENSION DeviceExtension;
    PCM_RESOURCE_LIST ResourceList;

    DeviceExtension = (PPNPROOT_PDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if (DeviceExtension->DeviceInfo->ResourceList)
    {
        /* Copy existing resource requirement list */
        ResourceList = ExAllocatePool(
            PagedPool,
            DeviceExtension->DeviceInfo->ResourceListSize);
        if (!ResourceList)
            return STATUS_NO_MEMORY;

        RtlCopyMemory(
            ResourceList,
            DeviceExtension->DeviceInfo->ResourceList,
            DeviceExtension->DeviceInfo->ResourceListSize);

        Irp->IoStatus.Information = (ULONG_PTR)ResourceList;

        return STATUS_SUCCESS;
    }
    else
    {
        /* No resources so just return without changing the status */
        return Irp->IoStatus.Status;
    }
}

static NTSTATUS
PdoQueryResourceRequirements(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp)
{
    PPNPROOT_PDO_DEVICE_EXTENSION DeviceExtension;
    PIO_RESOURCE_REQUIREMENTS_LIST ResourceList;

    DeviceExtension = (PPNPROOT_PDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if (DeviceExtension->DeviceInfo->ResourceRequirementsList)
    {
        /* Copy existing resource requirement list */
        ResourceList = ExAllocatePool(PagedPool, DeviceExtension->DeviceInfo->ResourceRequirementsList->ListSize);
        if (!ResourceList)
            return STATUS_NO_MEMORY;

        RtlCopyMemory(
            ResourceList,
            DeviceExtension->DeviceInfo->ResourceRequirementsList,
            DeviceExtension->DeviceInfo->ResourceRequirementsList->ListSize);

        Irp->IoStatus.Information = (ULONG_PTR)ResourceList;

        return STATUS_SUCCESS;
    }
    else
    {
        /* No resource requirements so just return without changing the status */
        return Irp->IoStatus.Status;
    }
}

static NTSTATUS
PdoQueryDeviceText(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp)
{
    PPNPROOT_PDO_DEVICE_EXTENSION DeviceExtension;
    DEVICE_TEXT_TYPE DeviceTextType;
    NTSTATUS Status = Irp->IoStatus.Status;

    DeviceExtension = (PPNPROOT_PDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    DeviceTextType = IrpSp->Parameters.QueryDeviceText.DeviceTextType;

    switch (DeviceTextType)
    {
        case DeviceTextDescription:
        {
            UNICODE_STRING String;
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / DeviceTextDescription\n");

            if (DeviceExtension->DeviceInfo->DeviceDescription.Buffer != NULL)
            {
                Status = RtlDuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
                                                   &DeviceExtension->DeviceInfo->DeviceDescription,
                                                   &String);
                Irp->IoStatus.Information = (ULONG_PTR)String.Buffer;
            }
            break;
        }

        case DeviceTextLocationInformation:
        {
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / DeviceTextLocationInformation\n");
            break;
        }

        default:
        {
            DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / unknown query id type 0x%lx\n", DeviceTextType);
        }
    }

    return Status;
}

static NTSTATUS
PdoQueryId(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp)
{
    PPNPROOT_PDO_DEVICE_EXTENSION DeviceExtension;
    BUS_QUERY_ID_TYPE IdType;
    NTSTATUS Status = Irp->IoStatus.Status;

    DeviceExtension = (PPNPROOT_PDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    IdType = IrpSp->Parameters.QueryId.IdType;

    switch (IdType)
    {
        case BusQueryDeviceID:
        {
            UNICODE_STRING String;
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryDeviceID\n");

            Status = RtlDuplicateUnicodeString(
                RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
                &DeviceExtension->DeviceInfo->DeviceID,
                &String);
            Irp->IoStatus.Information = (ULONG_PTR)String.Buffer;
            break;
        }

        case BusQueryHardwareIDs:
        case BusQueryCompatibleIDs:
        {
            /* Optional, do nothing */
            break;
        }

        case BusQueryInstanceID:
        {
            UNICODE_STRING String;
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryInstanceID\n");

            Status = RtlDuplicateUnicodeString(
                RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
                &DeviceExtension->DeviceInfo->InstanceID,
                &String);
            Irp->IoStatus.Information = (ULONG_PTR)String.Buffer;
            break;
        }

        default:
        {
            DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_ID / unknown query id type 0x%lx\n", IdType);
        }
    }

    return Status;
}

static NTSTATUS
PdoQueryBusInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp)
{
    PPNP_BUS_INFORMATION BusInfo;
    NTSTATUS Status;

    BusInfo = (PPNP_BUS_INFORMATION)ExAllocatePoolWithTag(PagedPool, sizeof(PNP_BUS_INFORMATION), TAG_PNP_ROOT);
    if (!BusInfo)
        Status = STATUS_NO_MEMORY;
    else
    {
        RtlCopyMemory(
            &BusInfo->BusTypeGuid,
            &GUID_BUS_TYPE_INTERNAL,
            sizeof(BusInfo->BusTypeGuid));
        BusInfo->LegacyBusType = PNPBus;
        /* We're the only root bus enumerator on the computer */
        BusInfo->BusNumber = 0;
        Irp->IoStatus.Information = (ULONG_PTR)BusInfo;
        Status = STATUS_SUCCESS;
    }

    return Status;
}

/*
 * FUNCTION: Handle Plug and Play IRPs for the child device
 * ARGUMENTS:
 *     DeviceObject = Pointer to physical device object of the child device
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
static NTSTATUS
PnpRootPdoPnpControl(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
{
    PPNPROOT_PDO_DEVICE_EXTENSION DeviceExtension;
    PPNPROOT_FDO_DEVICE_EXTENSION FdoDeviceExtension;
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;

    DeviceExtension = DeviceObject->DeviceExtension;
    FdoDeviceExtension = PnpRootDeviceObject->DeviceExtension;
    Status = Irp->IoStatus.Status;
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (IrpSp->MinorFunction)
    {
        case IRP_MN_START_DEVICE: /* 0x00 */
            DPRINT("IRP_MJ_PNP / IRP_MN_START_DEVICE\n");
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS: /* 0x07 */
            Status = PdoQueryDeviceRelations(DeviceObject, Irp, IrpSp);
            break;

        case IRP_MN_QUERY_CAPABILITIES: /* 0x09 */
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_CAPABILITIES\n");
            Status = PdoQueryCapabilities(DeviceObject, Irp, IrpSp);
            break;

        case IRP_MN_QUERY_RESOURCES: /* 0x0a */
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_RESOURCES\n");
            Status = PdoQueryResources(DeviceObject, Irp, IrpSp);
            break;

        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS: /* 0x0b */
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n");
            Status = PdoQueryResourceRequirements(DeviceObject, Irp, IrpSp);
            break;

        case IRP_MN_QUERY_DEVICE_TEXT: /* 0x0c */
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n");
            Status = PdoQueryDeviceText(DeviceObject, Irp, IrpSp);
            break;

        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS: /* 0x0d */
            DPRINT("IRP_MJ_PNP / IRP_MN_FILTER_RESOURCE_REQUIREMENTS\n");
            break;

        case IRP_MN_REMOVE_DEVICE:
            /* Remove the device from the device list and decrement the device count*/
            KeAcquireGuardedMutex(&FdoDeviceExtension->DeviceListLock);
            RemoveEntryList(&DeviceExtension->DeviceInfo->ListEntry);
            FdoDeviceExtension->DeviceListCount--;
            KeReleaseGuardedMutex(&FdoDeviceExtension->DeviceListLock);

            /* Free some strings we created */
            RtlFreeUnicodeString(&DeviceExtension->DeviceInfo->DeviceDescription);
            RtlFreeUnicodeString(&DeviceExtension->DeviceInfo->DeviceID);
            RtlFreeUnicodeString(&DeviceExtension->DeviceInfo->InstanceID);

            /* Free the resource requirements list */
            if (DeviceExtension->DeviceInfo->ResourceRequirementsList != NULL)
            ExFreePool(DeviceExtension->DeviceInfo->ResourceRequirementsList);

            /* Free the boot resources list */
            if (DeviceExtension->DeviceInfo->ResourceList != NULL)
            ExFreePool(DeviceExtension->DeviceInfo->ResourceList);

            /* Free the device info */
            ExFreePool(DeviceExtension->DeviceInfo);

            /* Finally, delete the device object */
            IoDeleteDevice(DeviceObject);

            /* Return success */
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_ID: /* 0x13 */
            Status = PdoQueryId(DeviceObject, Irp, IrpSp);
            break;

        case IRP_MN_QUERY_PNP_DEVICE_STATE: /* 0x14 */
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_PNP_DEVICE_STATE\n");
            break;

        case IRP_MN_QUERY_BUS_INFORMATION: /* 0x15 */
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_BUS_INFORMATION\n");
            Status = PdoQueryBusInformation(DeviceObject, Irp, IrpSp);
            break;

        default:
            DPRINT1("IRP_MJ_PNP / Unknown minor function 0x%lx\n", IrpSp->MinorFunction);
            break;
    }

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return Status;
}

/*
 * FUNCTION: Handle Plug and Play IRPs
 * ARGUMENTS:
 *     DeviceObject = Pointer to PDO or FDO
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
static NTSTATUS NTAPI
PnpRootPnpControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PPNPROOT_COMMON_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;

    DeviceExtension = (PPNPROOT_COMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if (DeviceExtension->IsFDO)
        Status = PnpRootFdoPnpControl(DeviceObject, Irp);
    else
        Status = PnpRootPdoPnpControl(DeviceObject, Irp);

    return Status;
}

/*
 * FUNCTION: Handle Power IRPs
 * ARGUMENTS:
 *     DeviceObject = Pointer to PDO or FDO
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
static NTSTATUS NTAPI
PnpRootPowerControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PPNPROOT_FDO_DEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;

    DeviceExtension = DeviceObject->DeviceExtension;
    Status = Irp->IoStatus.Status;
    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    if (DeviceExtension->Common.IsFDO)
    {
        ASSERT(!DeviceExtension->Common.IsFDO);
        PoStartNextPowerIrp(Irp);
        IoCopyCurrentIrpStackLocationToNext(Irp);
        Status = PoCallDriver(DeviceExtension->Ldo, Irp);
    }
    else
    {
        switch (IrpSp->MinorFunction)
        {
            case IRP_MN_QUERY_POWER:
            case IRP_MN_SET_POWER:
                Status = STATUS_SUCCESS;
                break;
        }
        Irp->IoStatus.Status = Status;
        PoStartNextPowerIrp(Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return Status;
}

NTSTATUS
NTAPI
PnpRootAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    PPNPROOT_FDO_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;

    DPRINT("PnpRootAddDevice(DriverObject %p, Pdo %p)\n", DriverObject, PhysicalDeviceObject);

    if (!PhysicalDeviceObject)
    {
        DPRINT("PhysicalDeviceObject 0x%p\n", PhysicalDeviceObject);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        KeBugCheckEx(PHASE1_INITIALIZATION_FAILED, Status, 0, 0, 0);
    }

    Status = IoCreateDevice(
        DriverObject,
        sizeof(PNPROOT_FDO_DEVICE_EXTENSION),
        NULL,
        FILE_DEVICE_BUS_EXTENDER,
        FILE_DEVICE_SECURE_OPEN,
        TRUE,
        &PnpRootDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IoCreateDevice() failed with status 0x%08lx\n", Status);
        KeBugCheckEx(PHASE1_INITIALIZATION_FAILED, Status, 0, 0, 0);
    }
    DPRINT("Created FDO %p\n", PnpRootDeviceObject);

    DeviceExtension = (PPNPROOT_FDO_DEVICE_EXTENSION)PnpRootDeviceObject->DeviceExtension;
    RtlZeroMemory(DeviceExtension, sizeof(PNPROOT_FDO_DEVICE_EXTENSION));

    DeviceExtension->Common.IsFDO = TRUE;
    DeviceExtension->State = dsStopped;
    InitializeListHead(&DeviceExtension->DeviceListHead);
    DeviceExtension->DeviceListCount = 0;
    KeInitializeGuardedMutex(&DeviceExtension->DeviceListLock);

    Status = IoAttachDeviceToDeviceStackSafe(
        PnpRootDeviceObject,
        PhysicalDeviceObject,
        &DeviceExtension->Ldo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IoAttachDeviceToDeviceStackSafe() failed with status 0x%08lx\n", Status);
        KeBugCheckEx(PHASE1_INITIALIZATION_FAILED, Status, 0, 0, 0);
    }

    PnpRootDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    DPRINT("Done AddDevice()\n");

    return STATUS_SUCCESS;
}

#if MI_TRACE_PFNS
PDEVICE_OBJECT IopPfnDumpDeviceObject;

NTSTATUS NTAPI
PnpRootCreateClose(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;

    if (DeviceObject != IopPfnDumpDeviceObject)
    {
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    if (IoStack->MajorFunction == IRP_MJ_CREATE)
    {
        MmDumpArmPfnDatabase(TRUE);
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}
#endif

NTSTATUS NTAPI
PnpRootDriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath)
{
#if MI_TRACE_PFNS
    NTSTATUS Status;
    UNICODE_STRING PfnDumpDeviceName = RTL_CONSTANT_STRING(L"\\Device\\PfnDump");
#endif

    DPRINT("PnpRootDriverEntry(%p %wZ)\n", DriverObject, RegistryPath);

    IopRootDriverObject = DriverObject;

    DriverObject->DriverExtension->AddDevice = PnpRootAddDevice;

#if MI_TRACE_PFNS
    DriverObject->MajorFunction[IRP_MJ_CREATE] = PnpRootCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = PnpRootCreateClose;
#endif
    DriverObject->MajorFunction[IRP_MJ_PNP] = PnpRootPnpControl;
    DriverObject->MajorFunction[IRP_MJ_POWER] = PnpRootPowerControl;

#if MI_TRACE_PFNS
    Status = IoCreateDevice(DriverObject,
                            0,
                            &PfnDumpDeviceName,
                            FILE_DEVICE_UNKNOWN,
                            0,
                            FALSE,
                            &IopPfnDumpDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Creating PFN Dump device failed with %lx\n", Status);
    }
    else
    {
        IopPfnDumpDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    }
#endif

    return STATUS_SUCCESS;
}
