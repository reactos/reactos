/*
 * PROJECT:     ReactOS HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Standard x86 HAL PnP root driver
 * COPYRIGHT:   Copyright 2023 Victor Perevertkin (victor.perevertkin@reactos.org)
 */

#include <hal.h>
#define NDEBUG
#include <debug.h>

typedef enum _EXTENSION_TYPE
{
    PdoExtensionType,
    FdoExtensionType
} EXTENSION_TYPE;

typedef struct _FDO_EXTENSION
{
    EXTENSION_TYPE ExtensionType;
    PDEVICE_OBJECT DeviceObject;
    SINGLE_LIST_ENTRY PdoListHead;
    UINT32 PdoCount;
} FDO_EXTENSION, *PFDO_EXTENSION;

typedef struct _PDO_EXTENSION
{
    EXTENSION_TYPE ExtensionType;
    PDEVICE_OBJECT DeviceObject;
    SINGLE_LIST_ENTRY ListEntry;
    WCHAR PnPID[8];
    PCM_RESOURCE_LIST BootResources;
    UINT32 BootResourcesLength;
} PDO_EXTENSION, *PPDO_EXTENSION;

PDRIVER_OBJECT HalpDriverObject;

#if defined(SARCH_XBOX)
static WCHAR HalHardwareIdString[] = L"xbox";
#elif defined(SARCH_PC98)
static WCHAR HalHardwareIdString[] = L"pc98_up";
#else
static WCHAR HalHardwareIdString[] = L"e_isa_up";
#endif


static
CODE_SEG("PAGE")
NTSTATUS
HalpFdoQueryDeviceRelations(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ DEVICE_RELATION_TYPE RelationType,
    _Out_ PDEVICE_RELATIONS* DeviceRelations)
{
    PAGED_CODE();

    if (RelationType == BusRelations)
    {
        PFDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
        PDEVICE_RELATIONS fdoRelations = ExAllocatePoolWithTag(
            PagedPool,
            FIELD_OFFSET(DEVICE_RELATIONS, Objects)
                + (sizeof(PDEVICE_OBJECT) * fdoExtension->PdoCount),
            TAG_HAL);
        
        if (!fdoRelations)
            return STATUS_INSUFFICIENT_RESOURCES;

        SIZE_T i = 0;
        for (PSINGLE_LIST_ENTRY curEntry = fdoExtension->PdoListHead.Next;
             curEntry != NULL;
             curEntry = curEntry->Next)
        {
            PPDO_EXTENSION pdoExtension = CONTAINING_RECORD(curEntry, PDO_EXTENSION, ListEntry);
            fdoRelations->Objects[i++] = pdoExtension->DeviceObject;
            ObReferenceObject(pdoExtension->DeviceObject);
        }
        fdoRelations->Count = i;
        ASSERT(fdoExtension->PdoCount == i);
        
        *DeviceRelations = fdoRelations;
        return STATUS_SUCCESS;
    }

    return STATUS_NOT_SUPPORTED;
}

static
CODE_SEG("PAGE")
NTSTATUS
HalpFdoQueryId(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ BUS_QUERY_ID_TYPE IdType,
    _Out_ PWSTR *BusQueryId)
{
    PAGED_CODE();

    if (IdType == BusQueryHardwareIDs)
    {
        // Put HalHardwareIdString as our HardwareID
        PWSTR buffer = ExAllocatePoolWithTag(
            PagedPool,
            sizeof(HalHardwareIdString) + sizeof(UNICODE_NULL),
            TAG_HAL);
        if (!buffer)
            return STATUS_INSUFFICIENT_RESOURCES;

        RtlCopyMemory(buffer, HalHardwareIdString, sizeof(HalHardwareIdString));
        buffer[sizeof(HalHardwareIdString) / sizeof(WCHAR)] = UNICODE_NULL;

        *BusQueryId = buffer;
        return STATUS_SUCCESS;
    }
    else
    {
        return STATUS_NOT_SUPPORTED;
    }
}

static
CODE_SEG("PAGE")
NTSTATUS
HalpPdoQueryDeviceRelations(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ DEVICE_RELATION_TYPE RelationType,
    _Out_ PDEVICE_RELATIONS* DeviceRelations)
{
    if (RelationType == TargetDeviceRelation)
    {
        PDEVICE_RELATIONS pdoRelations = ExAllocatePoolWithTag(
            PagedPool,
            sizeof(DEVICE_RELATIONS),
            TAG_HAL);

        if (!pdoRelations)
            return STATUS_INSUFFICIENT_RESOURCES;

        pdoRelations->Count = 1;
        pdoRelations->Objects[0] = DeviceObject;
        ObReferenceObject(DeviceObject);

        *DeviceRelations = pdoRelations;
        return STATUS_SUCCESS;
    }

    return STATUS_NOT_SUPPORTED;
}

static
CODE_SEG("PAGE")
NTSTATUS
HalpPdoQueryResources(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Out_ PCM_RESOURCE_LIST *Resources)
{
    PAGED_CODE();

    PPDO_EXTENSION pdoExtension = DeviceObject->DeviceExtension;

    if (pdoExtension->BootResources != NULL)
    {
        PCM_RESOURCE_LIST resourceList = ExAllocatePoolWithTag(
            PagedPool,
            pdoExtension->BootResourcesLength,
            TAG_HAL);

        if (!resourceList)
            return STATUS_INSUFFICIENT_RESOURCES;

        RtlCopyMemory(
            resourceList,
            pdoExtension->BootResources,
            pdoExtension->BootResourcesLength);

        *Resources = resourceList;
    }

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
HalpPdoQueryId(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ BUS_QUERY_ID_TYPE IdType,
    _Out_ PWSTR *BusQueryId)
{
    PAGED_CODE();

    PPDO_EXTENSION pdoExtension = DeviceObject->DeviceExtension;
    WCHAR id[30];
    SIZE_T offset;

    switch (IdType)
    {
        case BusQueryDeviceID:
            offset = 0;
            offset += swprintf(&id[offset], L"PCI_HAL\\%.7s", pdoExtension->PnPID);
            break;

        case BusQueryInstanceID:
            offset = 0;
            offset += swprintf(&id[offset], L"0");
            break;

        case BusQueryHardwareIDs:
            // For HardwareIDs, we are going to return two strings (REG_MULTI_SZ):
            // PCI_HAL\\PNP0A03
            // *PNP0A03
            offset = 0;
            offset += swprintf(&id[offset], L"PCI_HAL\\%.7s", pdoExtension->PnPID);
            offset += 1;
            offset += swprintf(&id[offset], L"*%.7s", pdoExtension->PnPID);
            offset += 1;
            id[offset] = UNICODE_NULL;
            break;

        default:
            return STATUS_NOT_SUPPORTED;
    }

    SIZE_T length = (offset + 1) * sizeof(WCHAR);
    PWSTR buffer = ExAllocatePoolWithTag(PagedPool, length, TAG_HAL);
    if (!buffer)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlCopyMemory(buffer, id, length);
    *BusQueryId = buffer;

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
HalpQueryCapabilities(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Out_ PDEVICE_CAPABILITIES Capabilities)
{
    PAGED_CODE();

    ASSERT(Capabilities->Version == 1);

    // Set the UniqueID and SilentInstall
    *Capabilities = (DEVICE_CAPABILITIES) {
        .Version = 1,
        .UniqueID = TRUE,
        .SilentInstall = TRUE,
        .Address = MAXULONG,
        .UINumber = MAXULONG,
        .DeviceState = {
            [PowerSystemWorking] = PowerDeviceD0,
            [PowerSystemHibernate] = PowerDeviceD3,
            [PowerSystemShutdown] = PowerDeviceD3,
            [PowerSystemSleeping3] = PowerDeviceD3
        }
    };

    return STATUS_SUCCESS;
}

static
CODE_SEG("PAGE")
NTSTATUS
NTAPI
HalpDispatchPnp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    PFDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = Irp->IoStatus.Status;

    if (fdoExtension->ExtensionType == FdoExtensionType)
    {
        switch (ioStack->MinorFunction)
        {
            case IRP_MN_QUERY_DEVICE_RELATIONS:
                status = HalpFdoQueryDeviceRelations(
                    DeviceObject,
                    ioStack->Parameters.QueryDeviceRelations.Type,
                    (PVOID)&Irp->IoStatus.Information);
                break;

            case IRP_MN_QUERY_ID:
                status = HalpFdoQueryId(
                    DeviceObject,
                    ioStack->Parameters.QueryId.IdType,
                     (PVOID)&Irp->IoStatus.Information);
                break;

            case IRP_MN_QUERY_CAPABILITIES:
                status = HalpQueryCapabilities(
                    DeviceObject,
                    ioStack->Parameters.DeviceCapabilities.Capabilities);
                break;
        }
    }
    else
    {
        ASSERT(fdoExtension->ExtensionType == PdoExtensionType);

        switch (ioStack->MinorFunction)
        {
            case IRP_MN_START_DEVICE:
                status = STATUS_SUCCESS;
                break;

            case IRP_MN_QUERY_DEVICE_RELATIONS:
                status = HalpPdoQueryDeviceRelations(
                    DeviceObject,
                    ioStack->Parameters.QueryDeviceRelations.Type,
                    (PVOID)&Irp->IoStatus.Information);
                break;

            case IRP_MN_QUERY_RESOURCES:
                status = HalpPdoQueryResources(
                    DeviceObject,
                    (PVOID)&Irp->IoStatus.Information);
                break;

            case IRP_MN_QUERY_ID:
                status = HalpPdoQueryId(
                    DeviceObject,
                    ioStack->Parameters.QueryId.IdType,
                    (PVOID)&Irp->IoStatus.Information);
                break;

            case IRP_MN_QUERY_CAPABILITIES:
                status = HalpQueryCapabilities(
                    DeviceObject,
                    ioStack->Parameters.DeviceCapabilities.Capabilities);
                break;
        }
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
HalpAddDevice(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT TargetDevice)
{
    // Must not be called
    ASSERT(FALSE);
    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
HalpDispatchPower(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status = Irp->IoStatus.Status;

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

    return Status;
}

CODE_SEG("INIT")
NTSTATUS
HalpCreatePdoDeviceObject(
    _In_ WCHAR PnPID[],
    _In_ PCM_RESOURCE_LIST BootResources,
    _In_ UINT32 BootResourcesLength,
    _In_ PDEVICE_OBJECT Fdo)
{
    PFDO_EXTENSION fdoExtension = Fdo->DeviceExtension;
    PDEVICE_OBJECT devObject;    

    NTSTATUS status = IoCreateDevice(
        HalpDriverObject,
        sizeof(PDO_EXTENSION),
        NULL,
        FILE_DEVICE_BUS_EXTENDER,
        FILE_AUTOGENERATED_DEVICE_NAME,
        FALSE,
        &devObject);

    if (!NT_SUCCESS(status))
    {
        DPRINT1("HAL: Could not create a PDO status=0x%08x\n", status);
        return status;
    }

    PPDO_EXTENSION pdoExtension = devObject->DeviceExtension;
    pdoExtension->ExtensionType = PdoExtensionType;
    pdoExtension->DeviceObject = devObject;
    RtlCopyMemory(pdoExtension->PnPID, PnPID, sizeof(pdoExtension->PnPID));
    pdoExtension->BootResources = BootResources;
    pdoExtension->BootResourcesLength = BootResourcesLength;
    PushEntryList(&fdoExtension->PdoListHead, &pdoExtension->ListEntry);
    fdoExtension->PdoCount++;

    devObject->Flags &= ~DO_DEVICE_INITIALIZING;

    DPRINT("HAL: Created a PDO %.7S\n", pdoExtension->PnPID);

    return status;
}

static
CODE_SEG("INIT")
NTSTATUS
NTAPI
HalpDriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    NTSTATUS status;
    PDEVICE_OBJECT halRootPdo = NULL, halRootFdo, tempDevice;
    PFDO_EXTENSION fdoExtension;

    HalpDriverObject = DriverObject;

    DriverObject->DriverExtension->AddDevice = HalpAddDevice;
    DriverObject->MajorFunction[IRP_MJ_PNP] = HalpDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = HalpDispatchPower;

    // Ask the kernel to create a PDO for HAL (attached to root bus)
    status = IoReportDetectedDevice(
        DriverObject,
        InterfaceTypeUndefined,
        -1,
        -1,
        NULL,
        NULL,
        FALSE,
        &halRootPdo);

    if (!NT_SUCCESS(status))
        return status;

    halRootPdo->Flags &= ~DO_DEVICE_INITIALIZING;

    // Now create the actual HAL FDO
    status = IoCreateDevice(
        DriverObject,
        sizeof(FDO_EXTENSION),
        NULL,
        FILE_DEVICE_BUS_EXTENDER,
        0,
        FALSE,
        &halRootFdo);

    if (!NT_SUCCESS(status))
    {
        // TODO: bug check?
        return status;
    }

    // Set up the FDO
    fdoExtension = halRootFdo->DeviceExtension;
    *fdoExtension = (FDO_EXTENSION) {
        .ExtensionType = FdoExtensionType,
    };
    
    halRootFdo->Flags &= ~DO_DEVICE_INITIALIZING;

    tempDevice = IoAttachDeviceToDeviceStack(halRootFdo, halRootPdo);
    if (!tempDevice)
    {
        return STATUS_NO_SUCH_DEVICE;
    }

    // Create and add the PCI PDO

    // Prepare the resource list for it
    PCM_RESOURCE_LIST pciResourceList = ExAllocatePoolWithTag(
        PagedPool,
        sizeof(CM_RESOURCE_LIST),
        TAG_HAL);
    
    if (!pciResourceList)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *pciResourceList = (CM_RESOURCE_LIST) {
        .Count = 1,
        .List = {
            {
                .BusNumber = 0,
                .InterfaceType = PCIBus,
                .PartialResourceList.Version = 1,
                .PartialResourceList.Revision = 1,
                .PartialResourceList.Count = 0,
            }
        }
    };

    // Create and add the object to HAL's FDO
    status = HalpCreatePdoDeviceObject(
        L"PNP0A03",
        pciResourceList,
        sizeof(*pciResourceList),
        halRootFdo);

    return status;
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
HaliInitPnpDriver(VOID)
{
    PAGED_CODE();

    UNICODE_STRING DriverString = RTL_CONSTANT_STRING(L"\\Driver\\PCI_HAL");
    return IoCreateDriver(&DriverString, HalpDriverEntry);
}
