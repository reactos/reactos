/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PnP manager device manipulation functions
 * COPYRIGHT:   Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              2007 Hervé Poussineau (hpoussin@reactos.org)
 *              2014-2017 Thomas Faber (thomas.faber@reactos.org)
 *              2020 Victor Perevertkin (victor.perevertkin@reactos.org)
 */

/* Device tree is a resource shared among all system services: hal, kernel, drivers etc.
 * Thus all code which interacts with the tree needs to be synchronized.
 * Here it's done via a list of DEVICE_ACTION_REQUEST structures, which represents
 * the device action queue. It is being processed exclusively by the PipDeviceActionWorker.
 *
 * Operation queuing can be done with the PiQueueDeviceAction function or with
 * the PiPerfomSyncDeviceAction for synchronous operations.
 * All device manipulation like starting, removing, enumeration (see DEVICE_ACTION enum)
 * have to be done with the PiQueueDeviceAction in order to avoid race conditions.
 *
 * Note: there is one special operation here - PiActionEnumRootDevices. It is meant to be done
 * during initialization process (and be the first device tree operation executed) and
 * is always executed synchronously.
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

extern ERESOURCE IopDriverLoadResource;
extern BOOLEAN PnpSystemInit;
extern PDEVICE_NODE IopRootDeviceNode;
extern BOOLEAN PnPBootDriversLoaded;
extern BOOLEAN PnPBootDriversInitialized;

#define MAX_DEVICE_ID_LEN          200
#define MAX_SEPARATORS_INSTANCEID  0
#define MAX_SEPARATORS_DEVICEID    1

/* DATA **********************************************************************/

LIST_ENTRY IopDeviceActionRequestList;
WORK_QUEUE_ITEM IopDeviceActionWorkItem;
BOOLEAN IopDeviceActionInProgress;
KSPIN_LOCK IopDeviceActionLock;
KEVENT PiEnumerationFinished;
static const WCHAR ServicesKeyName[] = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\";

/* TYPES *********************************************************************/

typedef struct _DEVICE_ACTION_REQUEST
{
    LIST_ENTRY RequestListEntry;
    PDEVICE_OBJECT DeviceObject;
    PKEVENT CompletionEvent;
    NTSTATUS *CompletionStatus;
    DEVICE_ACTION Action;
} DEVICE_ACTION_REQUEST, *PDEVICE_ACTION_REQUEST;

typedef enum _ADD_DEV_DRIVER_TYPE
{
    LowerFilter,
    LowerClassFilter,
    DeviceDriver,
    UpperFilter,
    UpperClassFilter
} ADD_DEV_DRIVER_TYPE;

typedef struct _ADD_DEV_DRIVERS_LIST
{
    LIST_ENTRY ListEntry;
    PDRIVER_OBJECT DriverObject;
    ADD_DEV_DRIVER_TYPE DriverType;
} ADD_DEV_DRIVERS_LIST, *PADD_DEV_DRIVERS_LIST;

typedef struct _ATTACH_FILTER_DRIVERS_CONTEXT
{
    ADD_DEV_DRIVER_TYPE DriverType;
    PDEVICE_NODE DeviceNode;
    PLIST_ENTRY DriversListHead;
} ATTACH_FILTER_DRIVERS_CONTEXT, *PATTACH_FILTER_DRIVERS_CONTEXT;

/* FUNCTIONS *****************************************************************/

PDEVICE_OBJECT
IopGetDeviceObjectFromDeviceInstance(PUNICODE_STRING DeviceInstance);

NTSTATUS
IopGetParentIdPrefix(PDEVICE_NODE DeviceNode, PUNICODE_STRING ParentIdPrefix);

USHORT
NTAPI
IopGetBusTypeGuidIndex(LPGUID BusTypeGuid);

NTSTATUS
IopSetDeviceInstanceData(HANDLE InstanceKey, PDEVICE_NODE DeviceNode);

VOID
NTAPI
IopInstallCriticalDevice(PDEVICE_NODE DeviceNode);

static
VOID
IopCancelPrepareDeviceForRemoval(PDEVICE_OBJECT DeviceObject);

static
NTSTATUS
IopPrepareDeviceForRemoval(PDEVICE_OBJECT DeviceObject, BOOLEAN Force);

static
NTSTATUS
IopSetServiceEnumData(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ HANDLE InstanceHandle);

static
BOOLEAN
IopValidateID(
    _In_ PWCHAR Id,
    _In_ BUS_QUERY_ID_TYPE QueryType)
{
    PWCHAR PtrChar;
    PWCHAR StringEnd;
    WCHAR Char;
    ULONG SeparatorsCount = 0;
    PWCHAR PtrPrevChar = NULL;
    ULONG MaxSeparators;
    BOOLEAN IsMultiSz;

    PAGED_CODE();

    switch (QueryType)
    {
        case BusQueryDeviceID:
            MaxSeparators = MAX_SEPARATORS_DEVICEID;
            IsMultiSz = FALSE;
            break;
        case BusQueryInstanceID:
            MaxSeparators = MAX_SEPARATORS_INSTANCEID;
            IsMultiSz = FALSE;
            break;

        case BusQueryHardwareIDs:
        case BusQueryCompatibleIDs:
            MaxSeparators = MAX_SEPARATORS_DEVICEID;
            IsMultiSz = TRUE;
            break;

        default:
            DPRINT1("IopValidateID: Not handled QueryType - %x\n", QueryType);
            return FALSE;
    }

    StringEnd = Id + MAX_DEVICE_ID_LEN;

    for (PtrChar = Id; PtrChar < StringEnd; PtrChar++)
    {
        Char = *PtrChar;

        if (Char == UNICODE_NULL)
        {
            if (!IsMultiSz || (PtrPrevChar && PtrChar == PtrPrevChar + 1))
            {
                if (MaxSeparators == SeparatorsCount || IsMultiSz)
                {
                    return TRUE;
                }

                DPRINT1("IopValidateID: SeparatorsCount - %lu, MaxSeparators - %lu\n",
                        SeparatorsCount, MaxSeparators);
                goto ErrorExit;
            }

            StringEnd = PtrChar + MAX_DEVICE_ID_LEN + 1;
            PtrPrevChar = PtrChar;
            SeparatorsCount = 0;
        }
        else if (Char < ' ' || Char > 0x7F || Char == ',')
        {
            DPRINT1("IopValidateID: Invalid character - %04X\n", Char);
            goto ErrorExit;
        }
        else if (Char == ' ')
        {
            *PtrChar = '_';
        }
        else if (Char == '\\')
        {
            SeparatorsCount++;

            if (SeparatorsCount > MaxSeparators)
            {
                DPRINT1("IopValidateID: SeparatorsCount - %lu, MaxSeparators - %lu\n",
                        SeparatorsCount, MaxSeparators);
                goto ErrorExit;
            }
        }
    }

    DPRINT1("IopValidateID: Not terminated ID\n");

ErrorExit:
    // FIXME logging
    return FALSE;
}

static
NTSTATUS
IopCreateDeviceInstancePath(
    _In_ PDEVICE_NODE DeviceNode,
    _Out_ PUNICODE_STRING InstancePath)
{
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING DeviceId;
    UNICODE_STRING InstanceId;
    IO_STACK_LOCATION Stack;
    NTSTATUS Status;
    UNICODE_STRING ParentIdPrefix = { 0, 0, NULL };
    DEVICE_CAPABILITIES DeviceCapabilities;
    BOOLEAN IsValidID;

    DPRINT("Sending IRP_MN_QUERY_ID.BusQueryDeviceID to device stack\n");

    Stack.Parameters.QueryId.IdType = BusQueryDeviceID;
    Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
                               &IoStatusBlock,
                               IRP_MN_QUERY_ID,
                               &Stack);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopInitiatePnpIrp(BusQueryDeviceID) failed (Status %x)\n", Status);
        return Status;
    }

    IsValidID = IopValidateID((PWCHAR)IoStatusBlock.Information, BusQueryDeviceID);

    if (!IsValidID)
    {
        DPRINT1("Invalid DeviceID. DeviceNode - %p\n", DeviceNode);
    }

    /* Save the device id string */
    RtlInitUnicodeString(&DeviceId, (PWSTR)IoStatusBlock.Information);

    DPRINT("Sending IRP_MN_QUERY_CAPABILITIES to device stack (after enumeration)\n");

    Status = IopQueryDeviceCapabilities(DeviceNode, &DeviceCapabilities);
    if (!NT_SUCCESS(Status))
    {
        if (Status != STATUS_NOT_SUPPORTED)
        {
            DPRINT1("IopQueryDeviceCapabilities() failed (Status 0x%08lx)\n", Status);
        }
        RtlFreeUnicodeString(&DeviceId);
        return Status;
    }

    /* This bit is only check after enumeration */
    if (DeviceCapabilities.HardwareDisabled)
    {
        /* FIXME: Cleanup device */
        RtlFreeUnicodeString(&DeviceId);
        return STATUS_PLUGPLAY_NO_DEVICE;
    }

    if (!DeviceCapabilities.UniqueID)
    {
        /* Device has not a unique ID. We need to prepend parent bus unique identifier */
        DPRINT("Instance ID is not unique\n");
        Status = IopGetParentIdPrefix(DeviceNode, &ParentIdPrefix);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IopGetParentIdPrefix() failed (Status 0x%08lx)\n", Status);
            RtlFreeUnicodeString(&DeviceId);
            return Status;
        }
    }

    DPRINT("Sending IRP_MN_QUERY_ID.BusQueryInstanceID to device stack\n");

    Stack.Parameters.QueryId.IdType = BusQueryInstanceID;
    Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
                               &IoStatusBlock,
                               IRP_MN_QUERY_ID,
                               &Stack);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopInitiatePnpIrp(BusQueryInstanceID) failed (Status %lx)\n", Status);
        ASSERT(IoStatusBlock.Information == 0);
    }

    if (IoStatusBlock.Information)
    {
        IsValidID = IopValidateID((PWCHAR)IoStatusBlock.Information, BusQueryInstanceID);

        if (!IsValidID)
        {
            DPRINT1("Invalid InstanceID. DeviceNode - %p\n", DeviceNode);
        }
    }

    RtlInitUnicodeString(&InstanceId,
                         (PWSTR)IoStatusBlock.Information);

    InstancePath->Length = 0;
    InstancePath->MaximumLength = DeviceId.Length + sizeof(WCHAR) +
                                  ParentIdPrefix.Length +
                                  InstanceId.Length +
                                  sizeof(UNICODE_NULL);
    if (ParentIdPrefix.Length && InstanceId.Length)
    {
        InstancePath->MaximumLength += sizeof(WCHAR);
    }

    InstancePath->Buffer = ExAllocatePoolWithTag(PagedPool,
                                                 InstancePath->MaximumLength,
                                                 TAG_IO);
    if (!InstancePath->Buffer)
    {
        RtlFreeUnicodeString(&InstanceId);
        RtlFreeUnicodeString(&ParentIdPrefix);
        RtlFreeUnicodeString(&DeviceId);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Start with the device id */
    RtlCopyUnicodeString(InstancePath, &DeviceId);
    RtlAppendUnicodeToString(InstancePath, L"\\");

    /* Add information from parent bus device to InstancePath */
    RtlAppendUnicodeStringToString(InstancePath, &ParentIdPrefix);
    if (ParentIdPrefix.Length && InstanceId.Length)
    {
        RtlAppendUnicodeToString(InstancePath, L"&");
    }

    /* Finally, add the id returned by the driver stack */
    RtlAppendUnicodeStringToString(InstancePath, &InstanceId);

    /*
     * FIXME: Check for valid characters, if there is invalid characters
     * then bugcheck
     */

    RtlFreeUnicodeString(&InstanceId);
    RtlFreeUnicodeString(&DeviceId);
    RtlFreeUnicodeString(&ParentIdPrefix);

    return STATUS_SUCCESS;
}

/**
 * @brief      Loads and/or returns the driver associated with the registry entry if the driver
 *             is enabled. In case of an error, sets up a corresponding Problem to the DeviceNode
 */
static
NTSTATUS
NTAPI
PiAttachFilterDriversCallback(
    PWSTR ValueName,
    ULONG ValueType,
    PVOID ValueData,
    ULONG ValueLength,
    PVOID Ctx,
    PVOID EntryContext)
{
    PATTACH_FILTER_DRIVERS_CONTEXT context = Ctx;
    PDRIVER_OBJECT DriverObject;
    NTSTATUS Status;
    BOOLEAN loadDrivers = (BOOLEAN)(ULONG_PTR)EntryContext;

    PAGED_CODE();

    // No filter value present
    if (ValueType != REG_SZ)
        return STATUS_SUCCESS;

    if (ValueLength <= sizeof(WCHAR))
        return STATUS_OBJECT_NAME_NOT_FOUND;

    // open the service registry key
    UNICODE_STRING serviceName = { .Length = 0 }, servicesKeyName;
    RtlInitUnicodeString(&serviceName, ValueData);
    RtlInitUnicodeString(&servicesKeyName, ServicesKeyName);

    HANDLE ccsServicesHandle, serviceHandle = NULL;

    Status = IopOpenRegistryKeyEx(&ccsServicesHandle, NULL, &servicesKeyName, KEY_READ);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open a registry key for \"%wZ\" (status %x)\n", &serviceName, Status);
        return Status;
    }

    Status = IopOpenRegistryKeyEx(&serviceHandle, ccsServicesHandle, &serviceName, KEY_READ);
    ZwClose(ccsServicesHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open a registry key for \"%wZ\" (status %x)\n", &serviceName, Status);
        return Status;
    }

    PADD_DEV_DRIVERS_LIST driverEntry = ExAllocatePoolWithTag(PagedPool,
                                                              sizeof(*driverEntry),
                                                              TAG_PNP_DEVACTION);

    if (!driverEntry)
    {
        DPRINT1("Failed to allocate driverEntry for \"%wZ\"\n", &serviceName);
        ZwClose(serviceHandle);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // check if the driver is disabled
    PKEY_VALUE_FULL_INFORMATION kvInfo;
    SERVICE_LOAD_TYPE startType = DisableLoad;

    Status = IopGetRegistryValue(serviceHandle, L"Start", &kvInfo);
    if (NT_SUCCESS(Status))
    {
        if (kvInfo->Type == REG_DWORD)
        {
            RtlMoveMemory(&startType,
                          (PVOID)((ULONG_PTR)kvInfo + kvInfo->DataOffset),
                          sizeof(startType));
        }

        ExFreePool(kvInfo);
    }

    // TODO: take into account other start types (like SERVICE_DEMAND_START)
    if (startType >= DisableLoad)
    {
        if (!(context->DeviceNode->Flags & DNF_HAS_PROBLEM))
        {
            PiSetDevNodeProblem(context->DeviceNode, CM_PROB_DISABLED_SERVICE);
        }

        DPRINT("Service \"%wZ\" is disabled (start type %u)\n", &serviceName, startType);
        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    // check if the driver is already loaded
    UNICODE_STRING driverName;
    Status = IopGetDriverNames(serviceHandle, &driverName, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Unable to obtain the driver name for \"%wZ\"\n", &serviceName);
        goto Cleanup;
    }

    // try to open it
    Status = ObReferenceObjectByName(&driverName,
                                     OBJ_OPENIF | OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                                     NULL, /* PassedAccessState */
                                     0, /* DesiredAccess */
                                     IoDriverObjectType,
                                     KernelMode,
                                     NULL, /* ParseContext */
                                     (PVOID*)&DriverObject);
    RtlFreeUnicodeString(&driverName);

    // the driver was not probably loaded, try to load
    if (!NT_SUCCESS(Status))
    {
        if (loadDrivers)
        {
            Status = IopLoadDriver(serviceHandle, &DriverObject);
        }
        else
        {
            DPRINT("Service \"%wZ\" will not be loaded now\n", &serviceName);
            // return failure, the driver will be loaded later (in a subsequent call)
            Status = STATUS_UNSUCCESSFUL;
            goto Cleanup;
        }
    }

    if (NT_SUCCESS(Status))
    {
        driverEntry->DriverObject = DriverObject;
        driverEntry->DriverType = context->DriverType;
        InsertTailList(context->DriversListHead, &driverEntry->ListEntry);
        ZwClose(serviceHandle);
        return STATUS_SUCCESS;
    }
    else
    {
        if (!(context->DeviceNode->Flags & DNF_HAS_PROBLEM))
        {
            switch (Status)
            {
                case STATUS_INSUFFICIENT_RESOURCES:
                    PiSetDevNodeProblem(context->DeviceNode, CM_PROB_OUT_OF_MEMORY);
                    break;
                case STATUS_FAILED_DRIVER_ENTRY:
                    PiSetDevNodeProblem(context->DeviceNode, CM_PROB_FAILED_DRIVER_ENTRY);
                    break;
                case STATUS_ILL_FORMED_SERVICE_ENTRY:
                    PiSetDevNodeProblem(context->DeviceNode, CM_PROB_DRIVER_SERVICE_KEY_INVALID);
                    break;
                default:
                    PiSetDevNodeProblem(context->DeviceNode, CM_PROB_DRIVER_FAILED_LOAD);
                    break;
            }
        }

        DPRINT1("Failed to load driver \"%wZ\" for %wZ (status %x)\n",
            &serviceName, &context->DeviceNode->InstancePath, Status);
    }

Cleanup:
    ExFreePoolWithTag(driverEntry, TAG_PNP_DEVACTION);
    if (serviceHandle)
    {
        ZwClose(serviceHandle);
    }
    return Status;
}


/**
 * @brief      Calls PiAttachFilterDriversCallback for filter drivers (if any)
 */
static
NTSTATUS
PiAttachFilterDrivers(
    PLIST_ENTRY DriversListHead,
    PDEVICE_NODE DeviceNode,
    HANDLE EnumSubKey,
    HANDLE ClassKey,
    BOOLEAN Lower,
    BOOLEAN LoadDrivers)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2] = { { NULL, 0, NULL, NULL, 0, NULL, 0 }, };
    ATTACH_FILTER_DRIVERS_CONTEXT routineContext;
    NTSTATUS Status;

    PAGED_CODE();

    routineContext.DriversListHead = DriversListHead;
    routineContext.DeviceNode = DeviceNode;

    // First add device filters
    routineContext.DriverType = Lower ? LowerFilter : UpperFilter;
    QueryTable[0] = (RTL_QUERY_REGISTRY_TABLE){
        .QueryRoutine = PiAttachFilterDriversCallback,
        .Name = Lower ? L"LowerFilters" : L"UpperFilters",
        .DefaultType = REG_NONE,
        .EntryContext = (PVOID)(ULONG_PTR)LoadDrivers
    };

    Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                    (PWSTR)EnumSubKey,
                                    QueryTable,
                                    &routineContext,
                                    NULL);
    if (ClassKey == NULL)
    {
        return Status;
    }

    // Then add device class filters
    routineContext.DriverType = Lower ? LowerClassFilter : UpperClassFilter;
    QueryTable[0] = (RTL_QUERY_REGISTRY_TABLE){
        .QueryRoutine = PiAttachFilterDriversCallback,
        .Name = Lower ? L"LowerFilters" : L"UpperFilters",
        .DefaultType = REG_NONE,
        .EntryContext = (PVOID)(ULONG_PTR)LoadDrivers
    };

    Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                    (PWSTR)ClassKey,
                                    QueryTable,
                                    &routineContext,
                                    NULL);
    return Status;
}

/**
 * @brief      Loads all drivers for a device node (actual service and filters)
 *             and calls their AddDevice routine
 *
 * @param[in]  DeviceNode   The device node
 * @param[in]  LoadDrivers  Whether to load drivers if they are not loaded yet
 *                          (used when storage subsystem is not yet initialized)
 */
static
NTSTATUS
PiCallDriverAddDevice(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ BOOLEAN LoadDrivers)
{
    NTSTATUS Status;
    HANDLE EnumRootKey, SubKey;
    HANDLE ClassKey = NULL;
    UNICODE_STRING EnumRoot = RTL_CONSTANT_STRING(ENUM_ROOT);
    static UNICODE_STRING ccsControlClass =
    RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Class");
    PKEY_VALUE_FULL_INFORMATION kvInfo = NULL;

    PAGED_CODE();

    // open the enumeration root key
    Status = IopOpenRegistryKeyEx(&EnumRootKey, NULL, &EnumRoot, KEY_READ);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IopOpenRegistryKeyEx() failed for \"%wZ\" (status %x)\n", &EnumRoot, Status);
        return Status;
    }

    // open an instance subkey
    Status = IopOpenRegistryKeyEx(&SubKey, EnumRootKey, &DeviceNode->InstancePath, KEY_READ);
    ZwClose(EnumRootKey);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open a devnode instance key for \"%wZ\" (status %x)\n",
                &DeviceNode->InstancePath, Status);
        return Status;
    }

    // try to get the class GUID of an instance and its registry key
    Status = IopGetRegistryValue(SubKey, REGSTR_VAL_CLASSGUID, &kvInfo);
    if (NT_SUCCESS(Status))
    {
        if (kvInfo->Type == REG_SZ && kvInfo->DataLength > sizeof(WCHAR))
        {
            UNICODE_STRING classGUID = {
                .MaximumLength = kvInfo->DataLength,
                .Length = kvInfo->DataLength - sizeof(UNICODE_NULL),
                .Buffer = (PVOID)((ULONG_PTR)kvInfo + kvInfo->DataOffset)
            };
            HANDLE ccsControlHandle;

            Status = IopOpenRegistryKeyEx(&ccsControlHandle, NULL, &ccsControlClass, KEY_READ);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("IopOpenRegistryKeyEx() failed for \"%wZ\" (status %x)\n",
                        &ccsControlClass, Status);
            }
            else
            {
                // open the CCS\Control\Class\<ClassGUID> key
                Status = IopOpenRegistryKeyEx(&ClassKey, ccsControlHandle, &classGUID, KEY_READ);
                ZwClose(ccsControlHandle);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("Failed to open class key \"%wZ\" (status %x)\n", &classGUID, Status);
                }
            }

            if (ClassKey)
            {
                // Check the Properties key of a class too
                // Windows fills some device properties from this key (which is protected)
                // TODO: add the device properties from this key

                UNICODE_STRING properties = RTL_CONSTANT_STRING(REGSTR_KEY_DEVICE_PROPERTIES);
                HANDLE propertiesHandle;

                Status = IopOpenRegistryKeyEx(&propertiesHandle, ClassKey, &properties, KEY_READ);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT("Properties key failed to open for \"%wZ\" (status %x)\n",
                           &classGUID, Status);
                }
                else
                {
                    ZwClose(propertiesHandle);
                }
            }
        }

        ExFreePool(kvInfo);
    }

    // the driver loading order:
    // 1. LowerFilters
    // 2. LowerClassFilters
    // 3. Device driver (only one service!)
    // 4. UpperFilters
    // 5. UpperClassFilters

    LIST_ENTRY drvListHead;
    InitializeListHead(&drvListHead);

    // lower (class) filters
    Status = PiAttachFilterDrivers(&drvListHead, DeviceNode, SubKey, ClassKey, TRUE, LoadDrivers);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    ATTACH_FILTER_DRIVERS_CONTEXT routineContext = {
        .DriversListHead = &drvListHead,
        .DriverType = DeviceDriver,
        .DeviceNode = DeviceNode
    };

    RTL_QUERY_REGISTRY_TABLE queryTable[2] = {{
        .QueryRoutine = PiAttachFilterDriversCallback,
        .Name = L"Service",
        .Flags = RTL_QUERY_REGISTRY_REQUIRED,
        .DefaultType = REG_SZ, // REG_MULTI_SZ is not allowed here
        .DefaultData = L"",
        .EntryContext = (PVOID)(ULONG_PTR)LoadDrivers
    },};

    // device driver
    Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                    (PWSTR)SubKey,
                                    queryTable,
                                    &routineContext,
                                    NULL);
    if (NT_SUCCESS(Status))
    {
        // do nothing
    }
    // if a driver is not found, but a device allows raw access -> proceed
    else if (Status == STATUS_OBJECT_NAME_NOT_FOUND &&
             (DeviceNode->CapabilityFlags & 0x00000040)) // CM_DEVCAP_RAWDEVICEOK
    {
        // add a dummy entry to the drivers list (need for later processing)
        PADD_DEV_DRIVERS_LIST driverEntry = ExAllocatePoolZero(PagedPool,
                                                               sizeof(*driverEntry),
                                                               TAG_PNP_DEVACTION);
        driverEntry->DriverType = DeviceDriver;
        InsertTailList(&drvListHead, &driverEntry->ListEntry);
        DPRINT("No service for \"%wZ\" (RawDeviceOK)\n", &DeviceNode->InstancePath);
    }
    else
    {
        if (Status == STATUS_OBJECT_TYPE_MISMATCH && !(DeviceNode->Flags & DNF_HAS_PROBLEM))
        {
            PiSetDevNodeProblem(DeviceNode, CM_PROB_REGISTRY);
        }
        DPRINT("No service for \"%wZ\" (loadDrv: %u)\n", &DeviceNode->InstancePath, LoadDrivers);
        goto Cleanup;
    }

    // upper (class) filters
    Status = PiAttachFilterDrivers(&drvListHead, DeviceNode, SubKey, ClassKey, FALSE, LoadDrivers);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    // finally loop through the stack and call AddDevice for every driver
    for (PLIST_ENTRY listEntry = drvListHead.Flink;
         listEntry != &drvListHead;
         listEntry = listEntry->Flink)
    {
        PADD_DEV_DRIVERS_LIST driverEntry;
        driverEntry = CONTAINING_RECORD(listEntry, ADD_DEV_DRIVERS_LIST, ListEntry);
        PDRIVER_OBJECT driverObject = driverEntry->DriverObject;

        // FIXME: ReactOS is not quite ready for this assert
        // (legacy drivers should not have AddDevice routine)
        // ASSERT(!(DriverObject->Flags & DRVO_LEGACY_DRIVER));

        if (driverObject && driverObject->DriverExtension->AddDevice)
        {
            Status = driverObject->DriverExtension->AddDevice(driverEntry->DriverObject,
                                                              DeviceNode->PhysicalDeviceObject);
        }
        else if (driverObject == NULL)
        {
            // valid only for DeviceDriver
            ASSERT(driverEntry->DriverType == DeviceDriver);
            ASSERT(DeviceNode->CapabilityFlags & 0x00000040); // CM_DEVCAP_RAWDEVICEOK
            Status = STATUS_SUCCESS;
        }
        else
        {
            // HACK: the driver doesn't have a AddDevice routine. We shouldn't be here,
            // but ReactOS' PnP stack is not that correct yet
            DeviceNode->Flags |= DNF_LEGACY_DRIVER;
            Status = STATUS_UNSUCCESSFUL;
        }

        // for filter drivers we don't care about the AddDevice result
        if (driverEntry->DriverType == DeviceDriver)
        {
            if (NT_SUCCESS(Status))
            {
                PDEVICE_OBJECT fdo = IoGetAttachedDeviceReference(DeviceNode->PhysicalDeviceObject);

                // HACK: Check if we have a ACPI device (needed for power management)
                if (fdo->DeviceType == FILE_DEVICE_ACPI)
                {
                    static BOOLEAN SystemPowerDeviceNodeCreated = FALSE;

                    // There can be only one system power device
                    if (!SystemPowerDeviceNodeCreated)
                    {
                        PopSystemPowerDeviceNode = DeviceNode;
                        ObReferenceObject(PopSystemPowerDeviceNode->PhysicalDeviceObject);
                        SystemPowerDeviceNodeCreated = TRUE;
                    }
                }

                ObDereferenceObject(fdo);
                PiSetDevNodeState(DeviceNode, DeviceNodeDriversAdded);
            }
            else
            {
                // lower filters (if already started) will be removed upon this request
                PiSetDevNodeProblem(DeviceNode, CM_PROB_FAILED_ADD);
                PiSetDevNodeState(DeviceNode, DeviceNodeAwaitingQueuedRemoval);
                break;
            }
        }

#if DBG
        PDEVICE_OBJECT attachedDO = IoGetAttachedDevice(DeviceNode->PhysicalDeviceObject);
        if (attachedDO->Flags & DO_DEVICE_INITIALIZING)
        {
            DPRINT1("DO_DEVICE_INITIALIZING is not cleared on a device 0x%p!\n", attachedDO);
        }
#endif
    }

Cleanup:
    while (!IsListEmpty(&drvListHead))
    {
        PLIST_ENTRY listEntry = RemoveHeadList(&drvListHead);
        PADD_DEV_DRIVERS_LIST driverEntry;
        driverEntry = CONTAINING_RECORD(listEntry, ADD_DEV_DRIVERS_LIST, ListEntry);

        // drivers which don't have any devices (in case of failure) will be cleaned up
        if (driverEntry->DriverObject)
        {
            ObDereferenceObject(driverEntry->DriverObject);
        }
        ExFreePoolWithTag(driverEntry, TAG_PNP_DEVACTION);
    }

    ZwClose(SubKey);
    if (ClassKey != NULL)
    {
        ZwClose(ClassKey);
    }

    return Status;
}

NTSTATUS
NTAPI
IopQueryDeviceCapabilities(PDEVICE_NODE DeviceNode,
                           PDEVICE_CAPABILITIES DeviceCaps)
{
    IO_STATUS_BLOCK StatusBlock;
    IO_STACK_LOCATION Stack;
    NTSTATUS Status;
    HANDLE InstanceKey;
    UNICODE_STRING ValueName;

    /* Set up the Header */
    RtlZeroMemory(DeviceCaps, sizeof(DEVICE_CAPABILITIES));
    DeviceCaps->Size = sizeof(DEVICE_CAPABILITIES);
    DeviceCaps->Version = 1;
    DeviceCaps->Address = -1;
    DeviceCaps->UINumber = -1;

    /* Set up the Stack */
    RtlZeroMemory(&Stack, sizeof(IO_STACK_LOCATION));
    Stack.Parameters.DeviceCapabilities.Capabilities = DeviceCaps;

    /* Send the IRP */
    Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
                               &StatusBlock,
                               IRP_MN_QUERY_CAPABILITIES,
                               &Stack);
    if (!NT_SUCCESS(Status))
    {
        if (Status != STATUS_NOT_SUPPORTED)
        {
            DPRINT1("IRP_MN_QUERY_CAPABILITIES failed with status 0x%lx\n", Status);
        }
        return Status;
    }

    /* Map device capabilities to capability flags */
    DeviceNode->CapabilityFlags = 0;
    if (DeviceCaps->LockSupported)
        DeviceNode->CapabilityFlags |= 0x00000001;    // CM_DEVCAP_LOCKSUPPORTED

    if (DeviceCaps->EjectSupported)
        DeviceNode->CapabilityFlags |= 0x00000002;    // CM_DEVCAP_EJECTSUPPORTED

    if (DeviceCaps->Removable)
        DeviceNode->CapabilityFlags |= 0x00000004;    // CM_DEVCAP_REMOVABLE

    if (DeviceCaps->DockDevice)
        DeviceNode->CapabilityFlags |= 0x00000008;    // CM_DEVCAP_DOCKDEVICE

    if (DeviceCaps->UniqueID)
        DeviceNode->CapabilityFlags |= 0x00000010;    // CM_DEVCAP_UNIQUEID

    if (DeviceCaps->SilentInstall)
        DeviceNode->CapabilityFlags |= 0x00000020;    // CM_DEVCAP_SILENTINSTALL

    if (DeviceCaps->RawDeviceOK)
        DeviceNode->CapabilityFlags |= 0x00000040;    // CM_DEVCAP_RAWDEVICEOK

    if (DeviceCaps->SurpriseRemovalOK)
        DeviceNode->CapabilityFlags |= 0x00000080;    // CM_DEVCAP_SURPRISEREMOVALOK

    if (DeviceCaps->HardwareDisabled)
        DeviceNode->CapabilityFlags |= 0x00000100;    // CM_DEVCAP_HARDWAREDISABLED

    if (DeviceCaps->NonDynamic)
        DeviceNode->CapabilityFlags |= 0x00000200;    // CM_DEVCAP_NONDYNAMIC

    if (DeviceCaps->NoDisplayInUI)
        DeviceNode->UserFlags |= DNUF_DONT_SHOW_IN_UI;
    else
        DeviceNode->UserFlags &= ~DNUF_DONT_SHOW_IN_UI;

    Status = IopCreateDeviceKeyPath(&DeviceNode->InstancePath, REG_OPTION_NON_VOLATILE, &InstanceKey);
    if (NT_SUCCESS(Status))
    {
        /* Set 'Capabilities' value */
        RtlInitUnicodeString(&ValueName, L"Capabilities");
        Status = ZwSetValueKey(InstanceKey,
                               &ValueName,
                               0,
                               REG_DWORD,
                               &DeviceNode->CapabilityFlags,
                               sizeof(ULONG));

        /* Set 'UINumber' value */
        if (DeviceCaps->UINumber != MAXULONG)
        {
            RtlInitUnicodeString(&ValueName, L"UINumber");
            Status = ZwSetValueKey(InstanceKey,
                                   &ValueName,
                                   0,
                                   REG_DWORD,
                                   &DeviceCaps->UINumber,
                                   sizeof(ULONG));
        }

        ZwClose(InstanceKey);
    }

    return Status;
}

static
NTSTATUS
IopQueryHardwareIds(PDEVICE_NODE DeviceNode,
                    HANDLE InstanceKey)
{
    IO_STACK_LOCATION Stack;
    IO_STATUS_BLOCK IoStatusBlock;
    PWSTR Ptr;
    UNICODE_STRING ValueName;
    NTSTATUS Status;
    ULONG Length, TotalLength;
    BOOLEAN IsValidID;

    DPRINT("Sending IRP_MN_QUERY_ID.BusQueryHardwareIDs to device stack\n");

    RtlZeroMemory(&Stack, sizeof(Stack));
    Stack.Parameters.QueryId.IdType = BusQueryHardwareIDs;
    Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
                               &IoStatusBlock,
                               IRP_MN_QUERY_ID,
                               &Stack);
    if (NT_SUCCESS(Status))
    {
        IsValidID = IopValidateID((PWCHAR)IoStatusBlock.Information, BusQueryHardwareIDs);

        if (!IsValidID)
        {
            DPRINT1("Invalid HardwareIDs. DeviceNode - %p\n", DeviceNode);
        }

        TotalLength = 0;

        Ptr = (PWSTR)IoStatusBlock.Information;
        DPRINT("Hardware IDs:\n");
        while (*Ptr)
        {
            DPRINT("  %S\n", Ptr);
            Length = (ULONG)wcslen(Ptr) + 1;

            Ptr += Length;
            TotalLength += Length;
        }
        DPRINT("TotalLength: %hu\n", TotalLength);
        DPRINT("\n");

        RtlInitUnicodeString(&ValueName, L"HardwareID");
        Status = ZwSetValueKey(InstanceKey,
                               &ValueName,
                               0,
                               REG_MULTI_SZ,
                               (PVOID)IoStatusBlock.Information,
                               (TotalLength + 1) * sizeof(WCHAR));
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ZwSetValueKey() failed (Status %lx)\n", Status);
        }
    }
    else
    {
        DPRINT("IopInitiatePnpIrp() failed (Status %x)\n", Status);
    }

    return Status;
}

static
NTSTATUS
IopQueryCompatibleIds(PDEVICE_NODE DeviceNode,
                      HANDLE InstanceKey)
{
    IO_STACK_LOCATION Stack;
    IO_STATUS_BLOCK IoStatusBlock;
    PWSTR Ptr;
    UNICODE_STRING ValueName;
    NTSTATUS Status;
    ULONG Length, TotalLength;
    BOOLEAN IsValidID;

    DPRINT("Sending IRP_MN_QUERY_ID.BusQueryCompatibleIDs to device stack\n");

    RtlZeroMemory(&Stack, sizeof(Stack));
    Stack.Parameters.QueryId.IdType = BusQueryCompatibleIDs;
    Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
                               &IoStatusBlock,
                               IRP_MN_QUERY_ID,
                               &Stack);
    if (NT_SUCCESS(Status) && IoStatusBlock.Information)
    {
        IsValidID = IopValidateID((PWCHAR)IoStatusBlock.Information, BusQueryCompatibleIDs);

        if (!IsValidID)
        {
            DPRINT1("Invalid CompatibleIDs. DeviceNode - %p\n", DeviceNode);
        }

        TotalLength = 0;

        Ptr = (PWSTR)IoStatusBlock.Information;
        DPRINT("Compatible IDs:\n");
        while (*Ptr)
        {
            DPRINT("  %S\n", Ptr);
            Length = (ULONG)wcslen(Ptr) + 1;

            Ptr += Length;
            TotalLength += Length;
        }
        DPRINT("TotalLength: %hu\n", TotalLength);
        DPRINT("\n");

        RtlInitUnicodeString(&ValueName, L"CompatibleIDs");
        Status = ZwSetValueKey(InstanceKey,
                               &ValueName,
                               0,
                               REG_MULTI_SZ,
                               (PVOID)IoStatusBlock.Information,
                               (TotalLength + 1) * sizeof(WCHAR));
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ZwSetValueKey() failed (Status %lx) or no Compatible ID returned\n", Status);
        }
    }
    else
    {
        DPRINT("IopInitiatePnpIrp() failed (Status %x)\n", Status);
    }

    return Status;
}

/**
 * @brief      Sets the DeviceNode's DeviceDesc and LocationInformation registry values
 */
VOID
PiSetDevNodeText(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ HANDLE InstanceKey)
{
    PAGED_CODE();

    LCID localeId;

    // Get the Locale ID
    NTSTATUS status = ZwQueryDefaultLocale(FALSE, &localeId);
    if (!NT_SUCCESS(status))
    {
        DPRINT1("ZwQueryDefaultLocale() failed with status %x\n", status);
        return;
    }

    // Step 1: Write the DeviceDesc value if does not exist

    UNICODE_STRING valDeviceDesc = RTL_CONSTANT_STRING(L"DeviceDesc");
    ULONG len;

    status = ZwQueryValueKey(InstanceKey, &valDeviceDesc, KeyValueBasicInformation, NULL, 0, &len);
    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        PWSTR deviceDesc = NULL;
        status = PiIrpQueryDeviceText(DeviceNode, localeId, DeviceTextDescription, &deviceDesc);

        if (deviceDesc && deviceDesc[0] != UNICODE_NULL)
        {
            status = ZwSetValueKey(InstanceKey,
                                   &valDeviceDesc,
                                   0,
                                   REG_SZ,
                                   deviceDesc,
                                   ((ULONG)wcslen(deviceDesc) + 1) * sizeof(WCHAR));

            if (!NT_SUCCESS(status))
            {
                DPRINT1("ZwSetValueKey() failed (Status %x)\n", status);
            }
        }
        else
        {
            // This key is mandatory, so even if the Irp fails, we still write it
            UNICODE_STRING unknownDeviceDesc = RTL_CONSTANT_STRING(L"Unknown device");
            DPRINT("Driver didn't return DeviceDesc (status %x)\n", status);

            status = ZwSetValueKey(InstanceKey,
                                   &valDeviceDesc,
                                   0,
                                   REG_SZ,
                                   unknownDeviceDesc.Buffer,
                                   unknownDeviceDesc.MaximumLength);
            if (!NT_SUCCESS(status))
            {
                DPRINT1("ZwSetValueKey() failed (Status %x)\n", status);
            }
        }

        if (deviceDesc)
        {
            ExFreePoolWithTag(deviceDesc, 0);
        }
    }

    // Step 2: LocaltionInformation is overwritten unconditionally

    PWSTR deviceLocationInfo = NULL;
    status = PiIrpQueryDeviceText(DeviceNode,
                                  localeId,
                                  DeviceTextLocationInformation,
                                  &deviceLocationInfo);

    if (deviceLocationInfo && deviceLocationInfo[0] != UNICODE_NULL)
    {
        UNICODE_STRING valLocationInfo = RTL_CONSTANT_STRING(L"LocationInformation");

        status = ZwSetValueKey(InstanceKey,
                               &valLocationInfo,
                               0,
                               REG_SZ,
                               deviceLocationInfo,
                               ((ULONG)wcslen(deviceLocationInfo) + 1) * sizeof(WCHAR));
        if (!NT_SUCCESS(status))
        {
            DPRINT1("ZwSetValueKey() failed (Status %x)\n", status);
        }
    }

    if (deviceLocationInfo)
    {
        ExFreePoolWithTag(deviceLocationInfo, 0);
    }
    else
    {
        DPRINT("Driver didn't return LocationInformation (status %x)\n", status);
    }
}

static
NTSTATUS
PiInitializeDevNode(
    _In_ PDEVICE_NODE DeviceNode)
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    HANDLE InstanceKey = NULL;
    UNICODE_STRING InstancePathU;
    PDEVICE_OBJECT OldDeviceObject;

    DPRINT("PiProcessNewDevNode(%p)\n", DeviceNode);
    DPRINT("PDO 0x%p\n", DeviceNode->PhysicalDeviceObject);

    /*
     * FIXME: For critical errors, cleanup and disable device, but always
     * return STATUS_SUCCESS.
     */

    Status = IopCreateDeviceInstancePath(DeviceNode, &InstancePathU);
    if (!NT_SUCCESS(Status))
    {
        if (Status != STATUS_PLUGPLAY_NO_DEVICE)
        {
            DPRINT1("IopCreateDeviceInstancePath() failed with status 0x%lx\n", Status);
        }
        return Status;
    }

    /* Verify that this is not a duplicate */
    OldDeviceObject = IopGetDeviceObjectFromDeviceInstance(&InstancePathU);
    if (OldDeviceObject != NULL)
    {
        PDEVICE_NODE OldDeviceNode = IopGetDeviceNode(OldDeviceObject);

        DPRINT1("Duplicate device instance '%wZ'\n", &InstancePathU);
        DPRINT1("Current instance parent: '%wZ'\n", &DeviceNode->Parent->InstancePath);
        DPRINT1("Old instance parent: '%wZ'\n", &OldDeviceNode->Parent->InstancePath);

        KeBugCheckEx(PNP_DETECTED_FATAL_ERROR,
                     0x01,
                     (ULONG_PTR)DeviceNode->PhysicalDeviceObject,
                     (ULONG_PTR)OldDeviceObject,
                     0);
    }

    DeviceNode->InstancePath = InstancePathU;

    DPRINT("InstancePath is %S\n", DeviceNode->InstancePath.Buffer);

    /*
     * Create registry key for the instance id, if it doesn't exist yet
     */
    Status = IopCreateDeviceKeyPath(&DeviceNode->InstancePath, REG_OPTION_NON_VOLATILE, &InstanceKey);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create the instance key! (Status %lx)\n", Status);

        /* We have to return success otherwise we abort the traverse operation */
        return STATUS_SUCCESS;
    }

    IopQueryHardwareIds(DeviceNode, InstanceKey);

    IopQueryCompatibleIds(DeviceNode, InstanceKey);

    DeviceNode->Flags |= DNF_IDS_QUERIED;

    // Set the device's DeviceDesc and LocationInformation fields
    PiSetDevNodeText(DeviceNode, InstanceKey);

    DPRINT("Sending IRP_MN_QUERY_BUS_INFORMATION to device stack\n");

    Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
                               &IoStatusBlock,
                               IRP_MN_QUERY_BUS_INFORMATION,
                               NULL);
    if (NT_SUCCESS(Status) && IoStatusBlock.Information)
    {
        PPNP_BUS_INFORMATION BusInformation = (PPNP_BUS_INFORMATION)IoStatusBlock.Information;

        DeviceNode->ChildBusNumber = BusInformation->BusNumber;
        DeviceNode->ChildInterfaceType = BusInformation->LegacyBusType;
        DeviceNode->ChildBusTypeIndex = IopGetBusTypeGuidIndex(&BusInformation->BusTypeGuid);
        ExFreePoolWithTag(BusInformation, 0);
    }
    else
    {
        DPRINT("IopInitiatePnpIrp() failed (Status %x) or IoStatusBlock.Information=NULL\n", Status);

        DeviceNode->ChildBusNumber = 0xFFFFFFF0;
        DeviceNode->ChildInterfaceType = InterfaceTypeUndefined;
        DeviceNode->ChildBusTypeIndex = -1;
    }

    DPRINT("Sending IRP_MN_QUERY_RESOURCES to device stack\n");

    Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
                               &IoStatusBlock,
                               IRP_MN_QUERY_RESOURCES,
                               NULL);
    if (NT_SUCCESS(Status) && IoStatusBlock.Information)
    {
        DeviceNode->BootResources = (PCM_RESOURCE_LIST)IoStatusBlock.Information;
        IopDeviceNodeSetFlag(DeviceNode, DNF_HAS_BOOT_CONFIG);
    }
    else
    {
        DPRINT("IopInitiatePnpIrp() failed (Status %x) or IoStatusBlock.Information=NULL\n", Status);
        DeviceNode->BootResources = NULL;
    }

    DPRINT("Sending IRP_MN_QUERY_RESOURCE_REQUIREMENTS to device stack\n");

    Status = IopInitiatePnpIrp(DeviceNode->PhysicalDeviceObject,
                               &IoStatusBlock,
                               IRP_MN_QUERY_RESOURCE_REQUIREMENTS,
                               NULL);
    if (NT_SUCCESS(Status))
    {
        DeviceNode->ResourceRequirements = (PIO_RESOURCE_REQUIREMENTS_LIST)IoStatusBlock.Information;
    }
    else
    {
        DPRINT("IopInitiatePnpIrp() failed (Status %08lx)\n", Status);
        DeviceNode->ResourceRequirements = NULL;
    }

    if (InstanceKey != NULL)
    {
        IopSetDeviceInstanceData(InstanceKey, DeviceNode);
    }

    // Try installing a critical device, so its Service key is populated
    // then call IopSetServiceEnumData to populate service's Enum key.
    // That allows us to start devices during an early boot
    IopInstallCriticalDevice(DeviceNode);
    IopSetServiceEnumData(DeviceNode, InstanceKey);

    ZwClose(InstanceKey);

    PiSetDevNodeState(DeviceNode, DeviceNodeInitialized);

    if (!IopDeviceNodeHasFlag(DeviceNode, DNF_LEGACY_DRIVER))
    {
        /* Report the device to the user-mode pnp manager */
        IopQueueTargetDeviceEvent(&GUID_DEVICE_ENUMERATED,
                                  &DeviceNode->InstancePath);
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
IopSetServiceEnumData(
    _In_ PDEVICE_NODE DeviceNode,
    _In_ HANDLE InstanceHandle)
{
    UNICODE_STRING ServicesKeyPath = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
    UNICODE_STRING ServiceKeyName;
    UNICODE_STRING EnumKeyName;
    UNICODE_STRING ValueName;
    UNICODE_STRING ServiceName;
    PKEY_VALUE_FULL_INFORMATION KeyValueInformation, kvInfo2;
    HANDLE ServiceKey = NULL, ServiceEnumKey = NULL;
    ULONG Disposition;
    ULONG Count = 0, NextInstance = 0;
    WCHAR ValueBuffer[6];
    NTSTATUS Status = STATUS_SUCCESS;

    // obtain the device node's ServiceName
    Status = IopGetRegistryValue(InstanceHandle, L"Service", &kvInfo2);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (kvInfo2->Type != REG_SZ || kvInfo2->DataLength <= sizeof(WCHAR))
    {
        ExFreePool(kvInfo2);
        return STATUS_UNSUCCESSFUL;
    }

    ServiceName.MaximumLength = kvInfo2->DataLength;
    ServiceName.Length = kvInfo2->DataLength - sizeof(UNICODE_NULL);
    ServiceName.Buffer = (PVOID)((ULONG_PTR)kvInfo2 + kvInfo2->DataOffset);

    DPRINT("IopSetServiceEnumData(%p)\n", DeviceNode);
    DPRINT("Instance: %wZ\n", &DeviceNode->InstancePath);
    DPRINT("Service: %wZ\n", &ServiceName);

    ServiceKeyName.MaximumLength = ServicesKeyPath.Length + ServiceName.Length + sizeof(UNICODE_NULL);
    ServiceKeyName.Length = 0;
    ServiceKeyName.Buffer = ExAllocatePool(PagedPool, ServiceKeyName.MaximumLength);
    if (ServiceKeyName.Buffer == NULL)
    {
        DPRINT1("No ServiceKeyName.Buffer!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlAppendUnicodeStringToString(&ServiceKeyName, &ServicesKeyPath);
    RtlAppendUnicodeStringToString(&ServiceKeyName, &ServiceName);

    DPRINT("ServiceKeyName: %wZ\n", &ServiceKeyName);

    Status = IopOpenRegistryKeyEx(&ServiceKey, NULL, &ServiceKeyName, KEY_CREATE_SUB_KEY);
    if (!NT_SUCCESS(Status))
    {
        goto done;
    }

    Status = RtlDuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING,
                                       &ServiceName,
                                       &DeviceNode->ServiceName);
    if (!NT_SUCCESS(Status))
    {
        goto done;
    }

    RtlInitUnicodeString(&EnumKeyName, L"Enum");
    Status = IopCreateRegistryKeyEx(&ServiceEnumKey,
                                    ServiceKey,
                                    &EnumKeyName,
                                    KEY_SET_VALUE,
                                    REG_OPTION_VOLATILE,
                                    &Disposition);
    if (NT_SUCCESS(Status))
    {
        if (Disposition == REG_OPENED_EXISTING_KEY)
        {
            /* Read the NextInstance value */
            Status = IopGetRegistryValue(ServiceEnumKey,
                                         L"Count",
                                         &KeyValueInformation);
            if (!NT_SUCCESS(Status))
                goto done;

            if ((KeyValueInformation->Type == REG_DWORD) &&
                (KeyValueInformation->DataLength))
            {
                /* Read it */
                Count = *(PULONG)((ULONG_PTR)KeyValueInformation +
                                  KeyValueInformation->DataOffset);
            }

            ExFreePool(KeyValueInformation);
            KeyValueInformation = NULL;

            /* Read the NextInstance value */
            Status = IopGetRegistryValue(ServiceEnumKey,
                                         L"NextInstance",
                                         &KeyValueInformation);
            if (!NT_SUCCESS(Status))
                goto done;

            if ((KeyValueInformation->Type == REG_DWORD) &&
                (KeyValueInformation->DataLength))
            {
                NextInstance = *(PULONG)((ULONG_PTR)KeyValueInformation +
                                         KeyValueInformation->DataOffset);
            }

            ExFreePool(KeyValueInformation);
            KeyValueInformation = NULL;
        }

        /* Set the instance path */
        swprintf(ValueBuffer, L"%lu", NextInstance);
        RtlInitUnicodeString(&ValueName, ValueBuffer);
        Status = ZwSetValueKey(ServiceEnumKey,
                               &ValueName,
                               0,
                               REG_SZ,
                               DeviceNode->InstancePath.Buffer,
                               DeviceNode->InstancePath.MaximumLength);
        if (!NT_SUCCESS(Status))
            goto done;

        /* Increment Count and NextInstance */
        Count++;
        NextInstance++;

        /* Set the new Count value */
        RtlInitUnicodeString(&ValueName, L"Count");
        Status = ZwSetValueKey(ServiceEnumKey,
                               &ValueName,
                               0,
                               REG_DWORD,
                               &Count,
                               sizeof(Count));
        if (!NT_SUCCESS(Status))
            goto done;

        /* Set the new NextInstance value */
        RtlInitUnicodeString(&ValueName, L"NextInstance");
        Status = ZwSetValueKey(ServiceEnumKey,
                               &ValueName,
                               0,
                               REG_DWORD,
                               &NextInstance,
                               sizeof(NextInstance));
    }

done:
    if (ServiceEnumKey != NULL)
        ZwClose(ServiceEnumKey);

    if (ServiceKey != NULL)
        ZwClose(ServiceKey);

    ExFreePool(ServiceKeyName.Buffer);
    ExFreePool(kvInfo2);

    return Status;
}

static
NTSTATUS
PiStartDeviceFinal(
    _In_ PDEVICE_NODE DeviceNode)
{
    DEVICE_CAPABILITIES DeviceCapabilities;
    NTSTATUS Status;

    if (!(DeviceNode->Flags & DNF_IDS_QUERIED))
    {
        // query ids (for reported devices)
        UNICODE_STRING enumRoot = RTL_CONSTANT_STRING(ENUM_ROOT);
        HANDLE enumRootHandle, instanceHandle;

        // open the enumeration root key
        Status = IopOpenRegistryKeyEx(&enumRootHandle, NULL, &enumRoot, KEY_READ);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IopOpenRegistryKeyEx() failed for \"%wZ\" (status %x)\n", &enumRoot, Status);
            return Status;
        }

        // open an instance subkey
        Status = IopOpenRegistryKeyEx(&instanceHandle, enumRootHandle, &DeviceNode->InstancePath, KEY_READ);
        ZwClose(enumRootHandle);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to open a devnode instance key for \"%wZ\" (status %x)\n",
                    &DeviceNode->InstancePath, Status);
            return Status;
        }

        IopQueryHardwareIds(DeviceNode, instanceHandle);
        IopQueryCompatibleIds(DeviceNode, instanceHandle);

        DeviceNode->Flags |= DNF_IDS_QUERIED;
        ZwClose(instanceHandle);
    }

    // we're about to start - needs enumeration
    DeviceNode->Flags |= DNF_REENUMERATE;

    DPRINT("Sending IRP_MN_QUERY_CAPABILITIES to device stack (after start)\n");

    Status = IopQueryDeviceCapabilities(DeviceNode, &DeviceCapabilities);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopInitiatePnpIrp() failed (Status 0x%08lx)\n", Status);
    }

    /* Invalidate device state so IRP_MN_QUERY_PNP_DEVICE_STATE is sent */
    IoInvalidateDeviceState(DeviceNode->PhysicalDeviceObject);

    DPRINT("Sending GUID_DEVICE_ARRIVAL %wZ\n", &DeviceNode->InstancePath);
    IopQueueTargetDeviceEvent(&GUID_DEVICE_ARRIVAL, &DeviceNode->InstancePath);

    PiSetDevNodeState(DeviceNode, DeviceNodeStarted);

    return STATUS_SUCCESS;
}

/* PUBLIC FUNCTIONS **********************************************************/

/**
 * @brief      Sends one of the remove IRPs to the device stack
 *
 * If there is a mounted VPB attached to a one of the stack devices, the IRP
 * should be send to a VPB's DeviceObject first (which belongs to a FS driver).
 * FS driver will then forward it down to the volume device.
 * While walking the device stack, the function sets (or unsets) VPB_REMOVE_PENDING flag
 * thus blocking all further mounts on a soon-to-be-removed devices
 */
static
NTSTATUS
PiIrpSendRemoveCheckVpb(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ UCHAR MinorFunction)
{
    KIRQL oldIrql;

    ASSERT(MinorFunction == IRP_MN_QUERY_REMOVE_DEVICE ||
           MinorFunction == IRP_MN_CANCEL_REMOVE_DEVICE ||
           MinorFunction == IRP_MN_SURPRISE_REMOVAL ||
           MinorFunction == IRP_MN_REMOVE_DEVICE);

    PDEVICE_OBJECT vpbDevObj = DeviceObject, targetDevice = DeviceObject;

    // walk the device stack down, stop on a first mounted device
    do
    {
        if (vpbDevObj->Vpb)
        {
            // two locks are needed here
            KeWaitForSingleObject(&vpbDevObj->DeviceLock, Executive, KernelMode, FALSE, NULL);
            IoAcquireVpbSpinLock(&oldIrql);

            if (MinorFunction == IRP_MN_CANCEL_REMOVE_DEVICE)
            {
                vpbDevObj->Vpb->Flags &= ~VPB_REMOVE_PENDING;
            }
            else
            {
                vpbDevObj->Vpb->Flags |= VPB_REMOVE_PENDING;
            }

            BOOLEAN isMounted = (_Bool)(vpbDevObj->Vpb->Flags & VPB_MOUNTED);

            if (isMounted)
            {
                targetDevice = vpbDevObj->Vpb->DeviceObject;
            }

            IoReleaseVpbSpinLock(oldIrql);
            KeSetEvent(&vpbDevObj->DeviceLock, IO_NO_INCREMENT, FALSE);

            if (isMounted)
            {
                break;
            }
        }

        oldIrql = KeAcquireQueuedSpinLock(LockQueueIoDatabaseLock);
        vpbDevObj = vpbDevObj->AttachedDevice;
        KeReleaseQueuedSpinLock(LockQueueIoDatabaseLock, oldIrql);
    } while (vpbDevObj);

    ASSERT(targetDevice);

    PVOID info;
    IO_STACK_LOCATION stack = {.MajorFunction = IRP_MJ_PNP, .MinorFunction = MinorFunction};

    return IopSynchronousCall(targetDevice, &stack, &info);
}

NTSTATUS
IopUpdateResourceMapForPnPDevice(
    IN PDEVICE_NODE DeviceNode);

static
VOID
NTAPI
IopSendRemoveDevice(IN PDEVICE_OBJECT DeviceObject)
{
    PDEVICE_NODE DeviceNode = IopGetDeviceNode(DeviceObject);

    ASSERT(DeviceNode->State == DeviceNodeAwaitingQueuedRemoval);

    /* Drivers should never fail a IRP_MN_REMOVE_DEVICE request */
    PiIrpSendRemoveCheckVpb(DeviceObject, IRP_MN_REMOVE_DEVICE);

    /* Start of HACK: update resources stored in registry, so IopDetectResourceConflict works */
    if (DeviceNode->ResourceList)
    {
        ASSERT(DeviceNode->ResourceListTranslated);
        DeviceNode->ResourceList->Count = 0;
        DeviceNode->ResourceListTranslated->Count = 0;
        IopUpdateResourceMapForPnPDevice(DeviceNode);
    }
    /* End of HACK */

    PiSetDevNodeState(DeviceNode, DeviceNodeRemoved);
    PiNotifyTargetDeviceChange(&GUID_TARGET_DEVICE_REMOVE_COMPLETE, DeviceObject, NULL);
    LONG_PTR refCount = ObDereferenceObject(DeviceObject);
    if (refCount != 0)
    {
        DPRINT1("Leaking device %wZ, refCount = %d\n", &DeviceNode->InstancePath, (INT32)refCount);
    }
}

static
VOID
IopSendRemoveDeviceRelations(PDEVICE_RELATIONS DeviceRelations)
{
    /* This function DOES dereference the device objects in all cases */

    ULONG i;

    for (i = 0; i < DeviceRelations->Count; i++)
    {
        IopSendRemoveDevice(DeviceRelations->Objects[i]);
        DeviceRelations->Objects[i] = NULL;
    }

    ExFreePool(DeviceRelations);
}

static
VOID
IopSendRemoveChildDevices(PDEVICE_NODE ParentDeviceNode)
{
    PDEVICE_NODE ChildDeviceNode, NextDeviceNode;
    KIRQL OldIrql;

    KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);
    ChildDeviceNode = ParentDeviceNode->Child;
    while (ChildDeviceNode != NULL)
    {
        NextDeviceNode = ChildDeviceNode->Sibling;
        KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);

        IopSendRemoveDevice(ChildDeviceNode->PhysicalDeviceObject);

        ChildDeviceNode = NextDeviceNode;

        KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);
    }
    KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);
}

static
VOID
NTAPI
IopSendSurpriseRemoval(IN PDEVICE_OBJECT DeviceObject)
{
    ASSERT(IopGetDeviceNode(DeviceObject)->State == DeviceNodeAwaitingQueuedRemoval);
    /* Drivers should never fail a IRP_MN_SURPRISE_REMOVAL request */
    PiIrpSendRemoveCheckVpb(DeviceObject, IRP_MN_SURPRISE_REMOVAL);
}

static
VOID
NTAPI
IopCancelRemoveDevice(IN PDEVICE_OBJECT DeviceObject)
{
    /* Drivers should never fail a IRP_MN_CANCEL_REMOVE_DEVICE request */
    PiIrpSendRemoveCheckVpb(DeviceObject, IRP_MN_CANCEL_REMOVE_DEVICE);

    PiNotifyTargetDeviceChange(&GUID_TARGET_DEVICE_REMOVE_CANCELLED, DeviceObject, NULL);
}

static
VOID
IopCancelRemoveChildDevices(PDEVICE_NODE ParentDeviceNode)
{
    PDEVICE_NODE ChildDeviceNode, NextDeviceNode;
    KIRQL OldIrql;

    KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);
    ChildDeviceNode = ParentDeviceNode->Child;
    while (ChildDeviceNode != NULL)
    {
        NextDeviceNode = ChildDeviceNode->Sibling;
        KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);

        IopCancelPrepareDeviceForRemoval(ChildDeviceNode->PhysicalDeviceObject);

        ChildDeviceNode = NextDeviceNode;

        KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);
    }
    KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);
}

static
VOID
IopCancelRemoveDeviceRelations(PDEVICE_RELATIONS DeviceRelations)
{
    /* This function DOES dereference the device objects in all cases */

    ULONG i;

    for (i = 0; i < DeviceRelations->Count; i++)
    {
        IopCancelPrepareDeviceForRemoval(DeviceRelations->Objects[i]);
        ObDereferenceObject(DeviceRelations->Objects[i]);
        DeviceRelations->Objects[i] = NULL;
    }

    ExFreePool(DeviceRelations);
}

static
VOID
IopCancelPrepareDeviceForRemoval(PDEVICE_OBJECT DeviceObject)
{
    IO_STACK_LOCATION Stack;
    IO_STATUS_BLOCK IoStatusBlock;
    PDEVICE_RELATIONS DeviceRelations;
    NTSTATUS Status;

    IopCancelRemoveDevice(DeviceObject);

    Stack.Parameters.QueryDeviceRelations.Type = RemovalRelations;

    Status = IopInitiatePnpIrp(DeviceObject,
                               &IoStatusBlock,
                               IRP_MN_QUERY_DEVICE_RELATIONS,
                               &Stack);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopInitiatePnpIrp() failed with status 0x%08lx\n", Status);
        DeviceRelations = NULL;
    }
    else
    {
        DeviceRelations = (PDEVICE_RELATIONS)IoStatusBlock.Information;
    }

    if (DeviceRelations)
        IopCancelRemoveDeviceRelations(DeviceRelations);
}

static
NTSTATUS
NTAPI
IopQueryRemoveDevice(IN PDEVICE_OBJECT DeviceObject)
{
    PDEVICE_NODE DeviceNode = IopGetDeviceNode(DeviceObject);
    NTSTATUS Status;

    ASSERT(DeviceNode);

    IopQueueTargetDeviceEvent(&GUID_DEVICE_REMOVE_PENDING,
                              &DeviceNode->InstancePath);

    Status = PiIrpSendRemoveCheckVpb(DeviceObject, IRP_MN_QUERY_REMOVE_DEVICE);

    PiNotifyTargetDeviceChange(&GUID_TARGET_DEVICE_QUERY_REMOVE, DeviceObject, NULL);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Removal vetoed by %wZ\n", &DeviceNode->InstancePath);
        IopQueueTargetDeviceEvent(&GUID_DEVICE_REMOVAL_VETOED,
                                  &DeviceNode->InstancePath);
    }

    return Status;
}

static
NTSTATUS
IopQueryRemoveChildDevices(PDEVICE_NODE ParentDeviceNode, BOOLEAN Force)
{
    PDEVICE_NODE ChildDeviceNode, NextDeviceNode, FailedRemoveDevice;
    NTSTATUS Status;
    KIRQL OldIrql;

    KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);
    ChildDeviceNode = ParentDeviceNode->Child;
    while (ChildDeviceNode != NULL)
    {
        NextDeviceNode = ChildDeviceNode->Sibling;
        KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);
        PiSetDevNodeState(ChildDeviceNode, DeviceNodeAwaitingQueuedRemoval);

        Status = IopPrepareDeviceForRemoval(ChildDeviceNode->PhysicalDeviceObject, Force);
        if (!NT_SUCCESS(Status))
        {
            FailedRemoveDevice = ChildDeviceNode;
            goto cleanup;
        }

        KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);
        ChildDeviceNode = NextDeviceNode;
    }
    KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);

    return STATUS_SUCCESS;

cleanup:
    KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);
    ChildDeviceNode = ParentDeviceNode->Child;
    while (ChildDeviceNode != NULL)
    {
        NextDeviceNode = ChildDeviceNode->Sibling;
        KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);

        IopCancelPrepareDeviceForRemoval(ChildDeviceNode->PhysicalDeviceObject);

        /* IRP_MN_CANCEL_REMOVE_DEVICE is also sent to the device
         * that failed the IRP_MN_QUERY_REMOVE_DEVICE request */
        if (ChildDeviceNode == FailedRemoveDevice)
            return Status;

        ChildDeviceNode = NextDeviceNode;

        KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);
    }
    KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);

    return Status;
}

static
NTSTATUS
IopQueryRemoveDeviceRelations(PDEVICE_RELATIONS DeviceRelations, BOOLEAN Force)
{
    /* This function DOES NOT dereference the device objects on SUCCESS
     * but it DOES dereference device objects on FAILURE */

    ULONG i, j;
    NTSTATUS Status;

    for (i = 0; i < DeviceRelations->Count; i++)
    {
        Status = IopPrepareDeviceForRemoval(DeviceRelations->Objects[i], Force);
        if (!NT_SUCCESS(Status))
        {
            j = i;
            goto cleanup;
        }
    }

    return STATUS_SUCCESS;

cleanup:
    /* IRP_MN_CANCEL_REMOVE_DEVICE is also sent to the device
     * that failed the IRP_MN_QUERY_REMOVE_DEVICE request */
    for (i = 0; i <= j; i++)
    {
        IopCancelPrepareDeviceForRemoval(DeviceRelations->Objects[i]);
        ObDereferenceObject(DeviceRelations->Objects[i]);
        DeviceRelations->Objects[i] = NULL;
    }
    for (; i < DeviceRelations->Count; i++)
    {
        ObDereferenceObject(DeviceRelations->Objects[i]);
        DeviceRelations->Objects[i] = NULL;
    }
    ExFreePool(DeviceRelations);

    return Status;
}

static
NTSTATUS
IopPrepareDeviceForRemoval(IN PDEVICE_OBJECT DeviceObject, BOOLEAN Force)
{
    PDEVICE_NODE DeviceNode = IopGetDeviceNode(DeviceObject);
    IO_STACK_LOCATION Stack;
    IO_STATUS_BLOCK IoStatusBlock;
    PDEVICE_RELATIONS DeviceRelations;
    NTSTATUS Status;

    if ((DeviceNode->UserFlags & DNUF_NOT_DISABLEABLE) && !Force)
    {
        DPRINT1("Removal not allowed for %wZ\n", &DeviceNode->InstancePath);
        return STATUS_UNSUCCESSFUL;
    }

    if (!Force && IopQueryRemoveDevice(DeviceObject) != STATUS_SUCCESS)
    {
        DPRINT1("Removal vetoed by failing the query remove request\n");

        IopCancelRemoveDevice(DeviceObject);

        return STATUS_UNSUCCESSFUL;
    }

    Stack.Parameters.QueryDeviceRelations.Type = RemovalRelations;

    Status = IopInitiatePnpIrp(DeviceObject,
                               &IoStatusBlock,
                               IRP_MN_QUERY_DEVICE_RELATIONS,
                               &Stack);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopInitiatePnpIrp() failed with status 0x%08lx\n", Status);
        DeviceRelations = NULL;
    }
    else
    {
        DeviceRelations = (PDEVICE_RELATIONS)IoStatusBlock.Information;
    }

    if (DeviceRelations)
    {
        Status = IopQueryRemoveDeviceRelations(DeviceRelations, Force);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    Status = IopQueryRemoveChildDevices(DeviceNode, Force);
    if (!NT_SUCCESS(Status))
    {
        if (DeviceRelations)
            IopCancelRemoveDeviceRelations(DeviceRelations);
        return Status;
    }

    if (DeviceRelations)
        IopSendRemoveDeviceRelations(DeviceRelations);
    IopSendRemoveChildDevices(DeviceNode);

    return STATUS_SUCCESS;
}

static
NTSTATUS
IopRemoveDevice(PDEVICE_NODE DeviceNode)
{
    NTSTATUS Status;

    // This function removes the device subtree, with the root in DeviceNode
    // atm everyting is in fact done inside this function, which is completely wrong.
    // The right implementation should have a separate removal worker thread and
    // properly do device node state transitions

    DPRINT("Removing device: %wZ\n", &DeviceNode->InstancePath);

    BOOLEAN surpriseRemoval = (_Bool)(DeviceNode->Flags & DNF_DEVICE_GONE);

    Status = IopPrepareDeviceForRemoval(DeviceNode->PhysicalDeviceObject, surpriseRemoval);

    if (surpriseRemoval)
    {
        IopSendSurpriseRemoval(DeviceNode->PhysicalDeviceObject);
        IopQueueTargetDeviceEvent(&GUID_DEVICE_SURPRISE_REMOVAL, &DeviceNode->InstancePath);
    }

    if (NT_SUCCESS(Status))
    {
        IopSendRemoveDevice(DeviceNode->PhysicalDeviceObject);
        if (surpriseRemoval)
        {
            IopQueueTargetDeviceEvent(&GUID_DEVICE_SAFE_REMOVAL, &DeviceNode->InstancePath);
        }
        return STATUS_SUCCESS;
    }

    return Status;
}

/*
 * @implemented
 */
VOID
NTAPI
IoInvalidateDeviceState(IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    PDEVICE_NODE DeviceNode = IopGetDeviceNode(PhysicalDeviceObject);
    PNP_DEVICE_STATE PnPFlags;
    NTSTATUS Status;

    Status = PiIrpQueryPnPDeviceState(DeviceNode, &PnPFlags);
    if (!NT_SUCCESS(Status))
    {
        if (Status != STATUS_NOT_SUPPORTED)
        {
            DPRINT1("IRP_MN_QUERY_PNP_DEVICE_STATE failed with status 0x%lx\n", Status);
        }
        return;
    }

    if (PnPFlags & PNP_DEVICE_NOT_DISABLEABLE)
        DeviceNode->UserFlags |= DNUF_NOT_DISABLEABLE;
    else
        DeviceNode->UserFlags &= ~DNUF_NOT_DISABLEABLE;

    if (PnPFlags & PNP_DEVICE_DONT_DISPLAY_IN_UI)
        DeviceNode->UserFlags |= DNUF_DONT_SHOW_IN_UI;
    else
        DeviceNode->UserFlags &= ~DNUF_DONT_SHOW_IN_UI;

    if ((PnPFlags & PNP_DEVICE_REMOVED) ||
        ((PnPFlags & PNP_DEVICE_FAILED) && !(PnPFlags & PNP_DEVICE_RESOURCE_REQUIREMENTS_CHANGED)))
    {
        /* Flag it if it's failed */
        if (PnPFlags & PNP_DEVICE_FAILED)
        {
            PiSetDevNodeProblem(DeviceNode, CM_PROB_FAILED_POST_START);
        }

        DeviceNode->Flags |= DNF_DEVICE_GONE;
        PiSetDevNodeState(DeviceNode, DeviceNodeAwaitingQueuedRemoval);
    }
    // it doesn't work anyway. A real resource rebalancing should be implemented
#if 0
    else if ((PnPFlags & PNP_DEVICE_FAILED) && (PnPFlags & PNP_DEVICE_RESOURCE_REQUIREMENTS_CHANGED))
    {
        /* Stop for resource rebalance */
        Status = IopStopDevice(DeviceNode);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to stop device for rebalancing\n");

            /* Stop failed so don't rebalance */
            PnPFlags &= ~PNP_DEVICE_RESOURCE_REQUIREMENTS_CHANGED;
        }
    }

    /* Resource rebalance */
    if (PnPFlags & PNP_DEVICE_RESOURCE_REQUIREMENTS_CHANGED)
    {
        DPRINT("Sending IRP_MN_QUERY_RESOURCES to device stack\n");

        Status = IopInitiatePnpIrp(PhysicalDeviceObject,
                                   &IoStatusBlock,
                                   IRP_MN_QUERY_RESOURCES,
                                   NULL);
        if (NT_SUCCESS(Status) && IoStatusBlock.Information)
        {
            DeviceNode->BootResources =
            (PCM_RESOURCE_LIST)IoStatusBlock.Information;
            IopDeviceNodeSetFlag(DeviceNode, DNF_HAS_BOOT_CONFIG);
        }
        else
        {
            DPRINT("IopInitiatePnpIrp() failed (Status %x) or IoStatusBlock.Information=NULL\n", Status);
            DeviceNode->BootResources = NULL;
        }

        DPRINT("Sending IRP_MN_QUERY_RESOURCE_REQUIREMENTS to device stack\n");

        Status = IopInitiatePnpIrp(PhysicalDeviceObject,
                                   &IoStatusBlock,
                                   IRP_MN_QUERY_RESOURCE_REQUIREMENTS,
                                   NULL);
        if (NT_SUCCESS(Status))
        {
            DeviceNode->ResourceRequirements =
            (PIO_RESOURCE_REQUIREMENTS_LIST)IoStatusBlock.Information;
        }
        else
        {
            DPRINT("IopInitiatePnpIrp() failed (Status %08lx)\n", Status);
            DeviceNode->ResourceRequirements = NULL;
        }

        /* IRP_MN_FILTER_RESOURCE_REQUIREMENTS is called indirectly by IopStartDevice */
        if (IopStartDevice(DeviceNode) != STATUS_SUCCESS)
        {
            DPRINT1("Restart after resource rebalance failed\n");

            DeviceNode->Flags &= ~(DNF_STARTED | DNF_START_REQUEST_PENDING);
            DeviceNode->Flags |= DNF_START_FAILED;

            IopRemoveDevice(DeviceNode);
        }
    }
#endif
}

static
NTSTATUS
PiEnumerateDevice(
    _In_ PDEVICE_NODE DeviceNode)
{
    PDEVICE_OBJECT ChildDeviceObject;
    PDEVICE_NODE ChildDeviceNode;
    ULONG i;

    // bus relations are already obtained for this device node

    if (!NT_SUCCESS(DeviceNode->CompletionStatus))
    {
        DPRINT("QDR request failed for %wZ, status %x\n",
            &DeviceNode->InstancePath, DeviceNode->CompletionStatus);
        // treat as if there are no child objects
    }

    PDEVICE_RELATIONS DeviceRelations = DeviceNode->OverUsed1.PendingDeviceRelations;
    DeviceNode->OverUsed1.PendingDeviceRelations = NULL;

    // it's acceptable not to have PDOs
    if (!DeviceRelations)
    {
        PiSetDevNodeState(DeviceNode, DeviceNodeStarted);
        DPRINT("No PDOs\n");
        return STATUS_SUCCESS;
    }

    // mark children nodes as non-present (those not returned in DR request will be removed)
    for (PDEVICE_NODE child = DeviceNode->Child; child != NULL; child = child->Sibling)
    {
        child->Flags &= ~DNF_ENUMERATED;
    }

    DPRINT("PiEnumerateDevice: enumerating %u children\n", DeviceRelations->Count);

    // create device nodes for all new children and set DNF_ENUMERATED back for old ones
    for (i = 0; i < DeviceRelations->Count; i++)
    {
        ChildDeviceObject = DeviceRelations->Objects[i];
        ASSERT((ChildDeviceObject->Flags & DO_DEVICE_INITIALIZING) == 0);

        ChildDeviceNode = IopGetDeviceNode(ChildDeviceObject);
        if (!ChildDeviceNode)
        {
            /* One doesn't exist, create it */
            ChildDeviceNode = PipAllocateDeviceNode(ChildDeviceObject);
            if (ChildDeviceNode)
            {
                PiInsertDevNode(ChildDeviceNode, DeviceNode);

                /* Mark the node as enumerated */
                ChildDeviceNode->Flags |= DNF_ENUMERATED;

                /* Mark the DO as bus enumerated */
                ChildDeviceObject->Flags |= DO_BUS_ENUMERATED_DEVICE;
            }
            else
            {
                /* Ignore this DO */
                DPRINT1("PipAllocateDeviceNode() failed. Skipping PDO %u\n", i);
                ObDereferenceObject(ChildDeviceObject);
            }
        }
        else
        {
            /* Mark it as enumerated */
            ChildDeviceNode->Flags |= DNF_ENUMERATED;
            ObDereferenceObject(ChildDeviceObject);
        }
    }
    ExFreePool(DeviceRelations);

    // time to remove non-reported devices
    for (PDEVICE_NODE child = DeviceNode->Child; child != NULL; child = child->Sibling)
    {
        if (!(child->Flags & (DNF_ENUMERATED|DNF_DEVICE_GONE)))
        {
            // this flag indicates that this is a surprise removal
            child->Flags |= DNF_DEVICE_GONE;
            PiSetDevNodeState(child, DeviceNodeAwaitingQueuedRemoval);
        }
    }

    PiSetDevNodeState(DeviceNode, DeviceNodeStarted);
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
IopSendEject(IN PDEVICE_OBJECT DeviceObject)
{
    IO_STACK_LOCATION Stack;
    PVOID Dummy;

    RtlZeroMemory(&Stack, sizeof(IO_STACK_LOCATION));
    Stack.MajorFunction = IRP_MJ_PNP;
    Stack.MinorFunction = IRP_MN_EJECT;

    return IopSynchronousCall(DeviceObject, &Stack, &Dummy);
}

/*
 * @implemented
 */
VOID
NTAPI
IoRequestDeviceEject(IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    PDEVICE_NODE DeviceNode = IopGetDeviceNode(PhysicalDeviceObject);
    PDEVICE_RELATIONS DeviceRelations;
    IO_STATUS_BLOCK IoStatusBlock;
    IO_STACK_LOCATION Stack;
    DEVICE_CAPABILITIES Capabilities;
    NTSTATUS Status;

    IopQueueTargetDeviceEvent(&GUID_DEVICE_KERNEL_INITIATED_EJECT,
                              &DeviceNode->InstancePath);

    if (IopQueryDeviceCapabilities(DeviceNode, &Capabilities) != STATUS_SUCCESS)
    {
        goto cleanup;
    }

    Stack.Parameters.QueryDeviceRelations.Type = EjectionRelations;

    Status = IopInitiatePnpIrp(PhysicalDeviceObject,
                               &IoStatusBlock,
                               IRP_MN_QUERY_DEVICE_RELATIONS,
                               &Stack);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IopInitiatePnpIrp() failed with status 0x%08lx\n", Status);
        DeviceRelations = NULL;
    }
    else
    {
        DeviceRelations = (PDEVICE_RELATIONS)IoStatusBlock.Information;
    }

    if (DeviceRelations)
    {
        Status = IopQueryRemoveDeviceRelations(DeviceRelations, FALSE);
        if (!NT_SUCCESS(Status))
            goto cleanup;
    }

    Status = IopQueryRemoveChildDevices(DeviceNode, FALSE);
    if (!NT_SUCCESS(Status))
    {
        if (DeviceRelations)
            IopCancelRemoveDeviceRelations(DeviceRelations);
        goto cleanup;
    }

    if (IopPrepareDeviceForRemoval(PhysicalDeviceObject, FALSE) != STATUS_SUCCESS)
    {
        if (DeviceRelations)
            IopCancelRemoveDeviceRelations(DeviceRelations);
        IopCancelRemoveChildDevices(DeviceNode);
        goto cleanup;
    }

    if (DeviceRelations)
        IopSendRemoveDeviceRelations(DeviceRelations);
    IopSendRemoveChildDevices(DeviceNode);

    DeviceNode->Problem = CM_PROB_HELD_FOR_EJECT;
    if (Capabilities.EjectSupported)
    {
        if (IopSendEject(PhysicalDeviceObject) != STATUS_SUCCESS)
        {
            goto cleanup;
        }
    }
    else
    {
        // DeviceNode->Flags |= DNF_DISABLED;
    }

    IopQueueTargetDeviceEvent(&GUID_DEVICE_EJECT,
                              &DeviceNode->InstancePath);

    return;

cleanup:
    IopQueueTargetDeviceEvent(&GUID_DEVICE_EJECT_VETOED,
                              &DeviceNode->InstancePath);
}

static
VOID
PiDevNodeStateMachine(
    _In_ PDEVICE_NODE RootNode)
{
    NTSTATUS status;
    BOOLEAN doProcessAgain;
    PDEVICE_NODE currentNode = RootNode;
    PDEVICE_OBJECT referencedObject;

    do
    {
        doProcessAgain = FALSE;

        // The device can be removed during processing, but we still need its Parent and Sibling
        // links to continue the tree traversal. So keep the link till the and of a cycle
        referencedObject = currentNode->PhysicalDeviceObject;
        ObReferenceObject(referencedObject);

        // Devices with problems are skipped (unless they are not being removed)
        if (currentNode->Flags & DNF_HAS_PROBLEM &&
            currentNode->State != DeviceNodeAwaitingQueuedRemoval)
        {
            goto skipEnum;
        }

        switch (currentNode->State)
        {
            case DeviceNodeUnspecified: // this state is not used
                break;
            case DeviceNodeUninitialized:
                DPRINT("DeviceNodeUninitialized %wZ\n", &currentNode->InstancePath);
                status = PiInitializeDevNode(currentNode);
                doProcessAgain = NT_SUCCESS(status);
                break;
            case DeviceNodeInitialized:
                DPRINT("DeviceNodeInitialized %wZ\n", &currentNode->InstancePath);
                status = PiCallDriverAddDevice(currentNode, PnPBootDriversInitialized);
                doProcessAgain = NT_SUCCESS(status);
                break;
            case DeviceNodeDriversAdded:
                DPRINT("DeviceNodeDriversAdded %wZ\n", &currentNode->InstancePath);
                status = IopAssignDeviceResources(currentNode);
                doProcessAgain = NT_SUCCESS(status);
                break;
            case DeviceNodeResourcesAssigned:
                DPRINT("DeviceNodeResourcesAssigned %wZ\n", &currentNode->InstancePath);
                // send IRP_MN_START_DEVICE
                PiIrpStartDevice(currentNode);

                // skip DeviceNodeStartPending, it is probably used for an async IRP_MN_START_DEVICE
                PiSetDevNodeState(currentNode, DeviceNodeStartCompletion);
                doProcessAgain = TRUE;
                break;
            case DeviceNodeStartPending: // skipped on XP/2003
                break;
            case DeviceNodeStartCompletion:
                DPRINT("DeviceNodeStartCompletion %wZ\n", &currentNode->InstancePath);
                status = currentNode->CompletionStatus;
                doProcessAgain = TRUE;
                if (!NT_SUCCESS(status))
                {
                    UINT32 problem = (status == STATUS_PNP_REBOOT_REQUIRED)
                                     ? CM_PROB_NEED_RESTART
                                     : CM_PROB_FAILED_START;

                    PiSetDevNodeProblem(currentNode, problem);
                    PiSetDevNodeState(currentNode, DeviceNodeAwaitingQueuedRemoval);
                }
                else
                {
                    // TODO: IopDoDeferredSetInterfaceState and IopAllocateLegacyBootResources
                    // are called here too

                    PiSetDevNodeState(currentNode, DeviceNodeStartPostWork);
                }
                break;
            case DeviceNodeStartPostWork:
                DPRINT("DeviceNodeStartPostWork %wZ\n", &currentNode->InstancePath);
                status = PiStartDeviceFinal(currentNode);
                doProcessAgain = TRUE;
                break;
            case DeviceNodeStarted:
                if (currentNode->Flags & DNF_REENUMERATE)
                {
                    DPRINT("DeviceNodeStarted REENUMERATE %wZ\n", &currentNode->InstancePath);
                    currentNode->Flags &= ~DNF_REENUMERATE;
                    status = PiIrpQueryDeviceRelations(currentNode, BusRelations);

                    // again, skip DeviceNodeEnumeratePending as with the starting sequence
                    PiSetDevNodeState(currentNode, DeviceNodeEnumerateCompletion);
                    doProcessAgain = TRUE;
                }
                break;
            case DeviceNodeQueryStopped:
                // we're here after sending IRP_MN_QUERY_STOP_DEVICE
                status = currentNode->CompletionStatus;
                if (NT_SUCCESS(status))
                {
                    PiSetDevNodeState(currentNode, DeviceNodeStopped);
                }
                else
                {
                    PiIrpCancelStopDevice(currentNode);
                    PiSetDevNodeState(currentNode, DeviceNodeStarted);
                }
                break;
            case DeviceNodeStopped:
                // TODO: do resource rebalance (not implemented)
                ASSERT(FALSE);
                break;
            case DeviceNodeRestartCompletion:
                break;
            case DeviceNodeEnumeratePending: // skipped on XP/2003
                break;
            case DeviceNodeEnumerateCompletion:
                DPRINT("DeviceNodeEnumerateCompletion %wZ\n", &currentNode->InstancePath);
                status = PiEnumerateDevice(currentNode);
                doProcessAgain = TRUE;
                break;
            case DeviceNodeAwaitingQueuedDeletion:
                break;
            case DeviceNodeAwaitingQueuedRemoval:
                DPRINT("DeviceNodeAwaitingQueuedRemoval %wZ\n", &currentNode->InstancePath);
                status = IopRemoveDevice(currentNode);
                break;
            case DeviceNodeQueryRemoved:
                break;
            case DeviceNodeRemovePendingCloses:
                break;
            case DeviceNodeRemoved:
                break;
            case DeviceNodeDeletePendingCloses:
                break;
            case DeviceNodeDeleted:
                break;
            default:
                break;
        }

skipEnum:
        if (!doProcessAgain)
        {
            KIRQL OldIrql;
            KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);
            /* If we have a child, simply go down the tree */
            if (currentNode->State != DeviceNodeRemoved && currentNode->Child != NULL)
            {
                ASSERT(currentNode->Child->Parent == currentNode);
                currentNode = currentNode->Child;
            }
            else
            {
                while (currentNode != RootNode)
                {
                    /* All children processed -- go sideways */
                    if (currentNode->Sibling != NULL)
                    {
                        ASSERT(currentNode->Sibling->Parent == currentNode->Parent);
                        currentNode = currentNode->Sibling;
                        break;
                    }
                    else
                    {
                        /* We're the last sibling -- go back up */
                        ASSERT(currentNode->Parent->LastChild == currentNode);
                        currentNode = currentNode->Parent;
                    }
                    /* We already visited the parent and all its children, so keep looking */
                }
            }
            KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);
        }
        ObDereferenceObject(referencedObject);
    } while (doProcessAgain || currentNode != RootNode);
}

#ifdef DBG
static
PCSTR
ActionToStr(
    _In_ DEVICE_ACTION Action)
{
    switch (Action)
    {
        case PiActionEnumDeviceTree:
            return "PiActionEnumDeviceTree";
        case PiActionEnumRootDevices:
            return "PiActionEnumRootDevices";
        case PiActionResetDevice:
            return "PiActionResetDevice";
        case PiActionAddBootDevices:
            return "PiActionAddBootDevices";
        case PiActionStartDevice:
            return "PiActionStartDevice";
        default:
            return "(request unknown)";
    }
}
#endif

static
VOID
NTAPI
PipDeviceActionWorker(
    _In_opt_ PVOID Context)
{
    PLIST_ENTRY ListEntry;
    PDEVICE_ACTION_REQUEST Request;
    KIRQL OldIrql;
    PDEVICE_NODE deviceNode;
    NTSTATUS status;

    KeAcquireSpinLock(&IopDeviceActionLock, &OldIrql);
    while (!IsListEmpty(&IopDeviceActionRequestList))
    {
        ListEntry = RemoveHeadList(&IopDeviceActionRequestList);
        KeReleaseSpinLock(&IopDeviceActionLock, OldIrql);
        Request = CONTAINING_RECORD(ListEntry, DEVICE_ACTION_REQUEST, RequestListEntry);

        ASSERT(Request->DeviceObject);

        deviceNode = IopGetDeviceNode(Request->DeviceObject);
        ASSERT(deviceNode);

        status = STATUS_SUCCESS;

        DPRINT("Processing PnP request %p: DeviceObject - %p, Action - %s\n",
               Request, Request->DeviceObject, ActionToStr(Request->Action));

        switch (Request->Action)
        {
            case PiActionAddBootDevices:
            {
                if (deviceNode->State == DeviceNodeInitialized &&
                    !(deviceNode->Flags & DNF_HAS_PROBLEM))
                {
                    status = PiCallDriverAddDevice(deviceNode, PnPBootDriversInitialized);
                }
                break;
            }
            case PiActionEnumRootDevices:
            case PiActionEnumDeviceTree:
                deviceNode->Flags |= DNF_REENUMERATE;
                PiDevNodeStateMachine(deviceNode);
                break;

            case PiActionResetDevice:
                // TODO: the operation is a no-op for everything except removed nodes
                // for removed nodes, it returns them back to DeviceNodeUninitialized
                status = STATUS_SUCCESS;
                break;

            case PiActionStartDevice:
                // This action is triggered from usermode, when a driver is installed
                // for a non-critical PDO
                if (deviceNode->State == DeviceNodeInitialized &&
                    !(deviceNode->Flags & DNF_HAS_PROBLEM))
                {
                    PiDevNodeStateMachine(deviceNode);
                }
                else
                {
                    DPRINT1("NOTE: attempt to start an already started/uninitialized device %wZ\n",
                            &deviceNode->InstancePath);
                    status = STATUS_UNSUCCESSFUL;
                }
                break;

            default:
                DPRINT1("Unimplemented device action %u\n", Request->Action);
                status = STATUS_NOT_IMPLEMENTED;
                break;
        }

        if (Request->CompletionStatus)
        {
            *Request->CompletionStatus = status;
        }

        if (Request->CompletionEvent)
        {
            KeSetEvent(Request->CompletionEvent, IO_NO_INCREMENT, FALSE);
        }

        DPRINT("Finished processing PnP request %p\n", Request);
        ObDereferenceObject(Request->DeviceObject);
        ExFreePoolWithTag(Request, TAG_IO);
        KeAcquireSpinLock(&IopDeviceActionLock, &OldIrql);
    }
    IopDeviceActionInProgress = FALSE;
    KeSetEvent(&PiEnumerationFinished, IO_NO_INCREMENT, FALSE);
    KeReleaseSpinLock(&IopDeviceActionLock, OldIrql);
}

/**
 * @brief      Queue a device operation to a worker thread.
 *
 * @param[in]  DeviceObject      The device object
 * @param[in]  Action            The action
 * @param[in]  CompletionEvent   The completion event object (optional)
 * @param[out] CompletionStatus  Status returned be the action will be written here
 */

VOID
PiQueueDeviceAction(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ DEVICE_ACTION Action,
    _In_opt_ PKEVENT CompletionEvent,
    _Out_opt_ NTSTATUS *CompletionStatus)
{
    PDEVICE_ACTION_REQUEST Request;
    KIRQL OldIrql;

    Request = ExAllocatePoolWithTag(NonPagedPoolMustSucceed, sizeof(*Request), TAG_IO);

    DPRINT("PiQueueDeviceAction: DeviceObject - %p, Request - %p, Action - %s\n",
        DeviceObject, Request, ActionToStr(Action));

    ObReferenceObject(DeviceObject);

    Request->DeviceObject = DeviceObject;
    Request->Action = Action;
    Request->CompletionEvent = CompletionEvent;
    Request->CompletionStatus = CompletionStatus;

    KeAcquireSpinLock(&IopDeviceActionLock, &OldIrql);
    InsertTailList(&IopDeviceActionRequestList, &Request->RequestListEntry);

    if (Action == PiActionEnumRootDevices || Action == PiActionAddBootDevices)
    {
        ASSERT(!IopDeviceActionInProgress);

        IopDeviceActionInProgress = TRUE;
        KeClearEvent(&PiEnumerationFinished);
        KeReleaseSpinLock(&IopDeviceActionLock, OldIrql);

        PipDeviceActionWorker(NULL);
        return;
    }

    if (IopDeviceActionInProgress || !PnPBootDriversLoaded)
    {
        KeReleaseSpinLock(&IopDeviceActionLock, OldIrql);
        return;
    }
    IopDeviceActionInProgress = TRUE;
    KeClearEvent(&PiEnumerationFinished);
    KeReleaseSpinLock(&IopDeviceActionLock, OldIrql);

    ExInitializeWorkItem(&IopDeviceActionWorkItem, PipDeviceActionWorker, NULL);
    ExQueueWorkItem(&IopDeviceActionWorkItem, DelayedWorkQueue);
}

/**
 * @brief      Perfom a device operation synchronously via PiQueueDeviceAction
 *
 * @param[in]  DeviceObject  The device object
 * @param[in]  Action        The action
 *
 * @return     Status of the operation
 */

NTSTATUS
PiPerformSyncDeviceAction(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ DEVICE_ACTION Action)
{
    KEVENT opFinished;
    NTSTATUS status;

    KeInitializeEvent(&opFinished, SynchronizationEvent, FALSE);
    PiQueueDeviceAction(DeviceObject, Action, &opFinished, &status);
    KeWaitForSingleObject(&opFinished, Executive, KernelMode, FALSE, NULL);

    return status;
}
