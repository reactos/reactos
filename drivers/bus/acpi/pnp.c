#include "precomp.h"

#define NDEBUG
#include <debug.h>

NTSTATUS
Bus_PlugInDevice (
    struct acpi_device *Device,
    PFDO_DEVICE_DATA    FdoData
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, Bus_PnP)
#pragma alloc_text (PAGE, Bus_PlugInDevice)
#pragma alloc_text (PAGE, Bus_InitializePdo)
#pragma alloc_text (PAGE, Bus_DestroyPdo)
#pragma alloc_text (PAGE, Bus_FDO_PnP)
#pragma alloc_text (PAGE, Bus_StartFdo)
#pragma alloc_text (PAGE, Bus_SendIrpSynchronously)
#endif

//
// Forward decls used below
//
NTSTATUS
Bus_PDO_PnP(
    PDEVICE_OBJECT      DeviceObject,
    PIRP                Irp,
    PIO_STACK_LOCATION  IrpStack,
    PPDO_DEVICE_DATA    PdoData
    );

NTSTATUS
NTAPI
Bus_CompletionRoutine(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp,
    PVOID           Context
    );

NTSTATUS
NTAPI
Bus_PnP (
    PDEVICE_OBJECT   DeviceObject,
    PIRP             Irp
    )
{
    PIO_STACK_LOCATION      irpStack;
    NTSTATUS                status;
    PCOMMON_DEVICE_DATA     commonData;

    PAGED_CODE();

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(IRP_MJ_PNP == irpStack->MajorFunction);

    commonData = (PCOMMON_DEVICE_DATA)DeviceObject->DeviceExtension;

    if (commonData->IsFDO)
    {
        DPRINT("FDO %s IRP:0x%p\n",
               PnPMinorFunctionString(irpStack->MinorFunction), Irp);

        status = Bus_FDO_PnP(
                     DeviceObject,
                     Irp,
                     irpStack,
                     (PFDO_DEVICE_DATA)commonData);
    }
    else
    {
        DPRINT("PDO %s IRP: 0x%p\n",
               PnPMinorFunctionString(irpStack->MinorFunction), Irp);

        status = Bus_PDO_PnP(
                     DeviceObject,
                     Irp,
                     irpStack,
                     (PPDO_DEVICE_DATA)commonData);
    }

    return status;
}

NTSTATUS
Bus_FDO_PnP (
    PDEVICE_OBJECT       DeviceObject,
    PIRP                 Irp,
    PIO_STACK_LOCATION   IrpStack,
    PFDO_DEVICE_DATA     DeviceData
    )
{
    NTSTATUS            status = STATUS_SUCCESS;
    ULONG               length, prevcount, numPdosPresent;
    PLIST_ENTRY         entry;
    PPDO_DEVICE_DATA    pdoData;
    PDEVICE_RELATIONS   relations, oldRelations;

    PAGED_CODE();

    switch (IrpStack->MinorFunction)
    {
    case IRP_MN_START_DEVICE:
        //
        // Start the FDO (Bus_StartFdo will forward START down first).
        //
        status = Bus_StartFdo(DeviceData, Irp);

        //
        // We must complete the IRP here (Bus_StartFdo returned and we don't
        // want it to go further down the stack again).
        //
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;

    case IRP_MN_QUERY_STOP_DEVICE:
        SET_NEW_PNP_STATE(DeviceData->Common, StopPending);
        Irp->IoStatus.Status = STATUS_SUCCESS;
        break;

    case IRP_MN_CANCEL_STOP_DEVICE:
        if (StopPending == DeviceData->Common.DevicePnPState)
        {
            RESTORE_PREVIOUS_PNP_STATE(DeviceData->Common);
            ASSERT(DeviceData->Common.DevicePnPState == Started);
        }
        Irp->IoStatus.Status = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_DEVICE_RELATIONS:
        DPRINT("\tQueryDeviceRelation Type: %s\n",
               DbgDeviceRelationString(IrpStack->Parameters.QueryDeviceRelations.Type));

        if (BusRelations != IrpStack->Parameters.QueryDeviceRelations.Type)
        {
            // Not ours; just pass down.
            break;
        }

        //
        // Build the bus's child list and complete locally.
        //
        ExAcquireFastMutex(&DeviceData->Mutex);

        oldRelations = (PDEVICE_RELATIONS)Irp->IoStatus.Information;
        if (oldRelations)
        {
            prevcount = oldRelations->Count;
        }
        else
        {
            prevcount = 0;
        }

        // Count PDOs actually present
        numPdosPresent = 0;
        for (entry = DeviceData->ListOfPDOs.Flink;
             entry != &DeviceData->ListOfPDOs;
             entry = entry->Flink)
        {
            pdoData = CONTAINING_RECORD(entry, PDO_DEVICE_DATA, Link);
            numPdosPresent++;
        }

        // If nothing to add and something already there, just return success with what was there
        if (oldRelations && DeviceData->NumPDOs == 0)
        {
            ExReleaseFastMutex(&DeviceData->Mutex);
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;
        }

        length = sizeof(DEVICE_RELATIONS) +
                 (((numPdosPresent + prevcount) - 1) * sizeof(PDEVICE_OBJECT));

        relations = ExAllocatePoolWithTag(PagedPool, length, 'IpcA');
        if (relations == NULL)
        {
            ExReleaseFastMutex(&DeviceData->Mutex);
            Irp->IoStatus.Status = status = STATUS_INSUFFICIENT_RESOURCES;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;
        }

        // Copy any existing objects (do NOT free oldRelations here; we didn't allocate it)
        if (prevcount)
        {
            RtlCopyMemory(relations->Objects,
                          oldRelations->Objects,
                          prevcount * sizeof(PDEVICE_OBJECT));
        }

        relations->Count = prevcount + numPdosPresent;

        // Append our PDOs and reference them
        for (entry = DeviceData->ListOfPDOs.Flink;
             entry != &DeviceData->ListOfPDOs;
             entry = entry->Flink)
        {
            pdoData = CONTAINING_RECORD(entry, PDO_DEVICE_DATA, Link);
            relations->Objects[prevcount] = pdoData->Common.Self;
            ObReferenceObject(pdoData->Common.Self);
            prevcount++;
        }

        DPRINT("\t#PDOs present = %d\n\t#PDOs reported = %d\n",
               DeviceData->NumPDOs, relations->Count);

        // Replace in IRP and complete locally
        Irp->IoStatus.Information = (ULONG_PTR)relations;
        ExReleaseFastMutex(&DeviceData->Mutex);

        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;

    default:
        //
        // Not explicitly handled here; we'll just pass down.
        //
        break;
    }

    //
    // Pass-through: do not overwrite IoStatus; return lower status.
    //
    IoSkipCurrentIrpStackLocation(Irp);
    status = IoCallDriver(DeviceData->NextLowerDriver, Irp);
    return status;
}

NTSTATUS
Bus_StartFdo (
    PFDO_DEVICE_DATA FdoData,
    PIRP             Irp
    )
{
    NTSTATUS     status;
    POWER_STATE  powerState;
    ACPI_STATUS  AcpiStatus;

    PAGED_CODE();

    //
    // IMPORTANT: Forward IRP_MN_START_DEVICE to the next lower driver
    // and wait for it to succeed before we do ACPI/enum work.
    //
    status = Bus_SendIrpSynchronously(FdoData->NextLowerDriver, Irp);
    if (!NT_SUCCESS(status))
    {
        DPRINT1("Lower driver START_DEVICE failed: 0x%08lx\n", status);
        return status;
    }

    FdoData->Common.DevicePowerState = PowerDeviceD0;
    powerState.DeviceState = PowerDeviceD0;
    PoSetPowerState(FdoData->Common.Self, DevicePowerState, powerState);

    SET_NEW_PNP_STATE(FdoData->Common, Started);

    //
    // Initialize ACPICA robustly (let it allocate as needed).
    //
    AcpiStatus = AcpiInitializeSubsystem();
    if (ACPI_FAILURE(AcpiStatus))
    {
        DPRINT1("Unable to AcpiInitializeSubsystem\n");
        return STATUS_UNSUCCESSFUL;
    }

    // Use dynamic table sizing, allow ACPICA to allocate (third param TRUE)
    AcpiStatus = AcpiInitializeTables(NULL, 0, TRUE);
    if (ACPI_FAILURE(AcpiStatus))
    {
        DPRINT1("Unable to AcpiInitializeTables\n");
        return STATUS_UNSUCCESSFUL;
    }

    AcpiStatus = AcpiLoadTables();
    if (ACPI_FAILURE(AcpiStatus))
    {
        DPRINT1("Unable to AcpiLoadTables\n");
        AcpiTerminate();
        return STATUS_UNSUCCESSFUL;
    }

    status = acpi_create_volatile_registry_tables();
    if (!NT_SUCCESS(status))
    {
        DPRINT1("Unable to create ACPI tables in registry (0x%08lx)\n", status);
        // Non-fatal for bring-up; continue.
    }

    DPRINT("Acpi subsystem init\n");

    AcpiStatus = acpi_init();
    if (!ACPI_SUCCESS(AcpiStatus))
    {
        DPRINT1("acpi_init() failed with status 0x%X\n", AcpiStatus);
        AcpiTerminate();
        return STATUS_UNSUCCESSFUL;
    }

    status = ACPIEnumerateDevices(FdoData);
    return status;
}

NTSTATUS
Bus_SendIrpSynchronously (
    PDEVICE_OBJECT DeviceObject,
    PIRP           Irp
    )
{
    KEVENT   event;
    NTSTATUS status;

    PAGED_CODE();

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    IoCopyCurrentIrpStackLocationToNext(Irp);

    IoSetCompletionRoutine(Irp,
                           Bus_CompletionRoutine,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE);

    status = IoCallDriver(DeviceObject, Irp);

    if (status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        status = Irp->IoStatus.Status;
    }

    return status;
}

NTSTATUS
NTAPI
Bus_CompletionRoutine(
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp,
    PVOID           Context
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);

    if (Irp->PendingReturned != FALSE)
    {
        KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
    }
    return STATUS_MORE_PROCESSING_REQUIRED; // Keep this IRP
}

NTSTATUS
Bus_DestroyPdo (
    PDEVICE_OBJECT      Device,
    PPDO_DEVICE_DATA    PdoData
    )
{
    PAGED_CODE();

    if (PdoData->HardwareIDs)
    {
        ExFreePoolWithTag(PdoData->HardwareIDs, 'DpcA');
        PdoData->HardwareIDs = NULL;
    }

    DPRINT("\tDeleting PDO: 0x%p\n", Device);
    IoDeleteDevice(Device);
    return STATUS_SUCCESS;
}

VOID
Bus_InitializePdo (
    PDEVICE_OBJECT      Pdo,
    PFDO_DEVICE_DATA    FdoData
    )
{
    PPDO_DEVICE_DATA pdoData;
    int              acpistate;
    DEVICE_POWER_STATE ntState;
    ACPI_HANDLE      handle = 0;
    ACPI_STATUS      status = 0;

    PAGED_CODE();

    pdoData = (PPDO_DEVICE_DATA)Pdo->DeviceExtension;

    DPRINT("pdo 0x%p, extension 0x%p\n", Pdo, pdoData);

    if (pdoData->AcpiHandle)
        acpi_bus_get_power(pdoData->AcpiHandle, &acpistate);
    else
        acpistate = ACPI_STATE_D0;

    switch (acpistate)
    {
    case ACPI_STATE_D0: ntState = PowerDeviceD0; break;
    case ACPI_STATE_D1: ntState = PowerDeviceD1; break;
    case ACPI_STATE_D2: ntState = PowerDeviceD2; break;
    case ACPI_STATE_D3: ntState = PowerDeviceD3; break;
    default:
        DPRINT1("Unknown power state (%d) returned by acpi\n", acpistate);
        ntState = PowerDeviceUnspecified;
        break;
    }

    //
    // Initialize the rest
    //
    pdoData->Common.IsFDO = FALSE;
    pdoData->Common.Self  = Pdo;

    pdoData->ParentFdo = FdoData->Common.Self;

    INITIALIZE_PNP_STATE(pdoData->Common);

    pdoData->Common.DevicePowerState = ntState;
    pdoData->Common.SystemPowerState = FdoData->Common.SystemPowerState;

    /* Identify the dock device */
    if (pdoData->AcpiHandle)
    {
        status = AcpiGetHandle(pdoData->AcpiHandle, "_DCK", &handle);
        if (ACPI_SUCCESS(status))
        {
            DPRINT("Found _DCK method!");
            pdoData->DockDevice = TRUE;
        }
    }

    ExAcquireFastMutex(&FdoData->Mutex);
    InsertTailList(&FdoData->ListOfPDOs, &pdoData->Link);
    FdoData->NumPDOs++;
    ExReleaseFastMutex(&FdoData->Mutex);

    // Last step
    Pdo->Flags &= ~DO_DEVICE_INITIALIZING;
}

#if DBG

PCHAR
PnPMinorFunctionString (
    UCHAR MinorFunction
)
{
    switch (MinorFunction)
    {
    case IRP_MN_START_DEVICE:
        return "IRP_MN_START_DEVICE";
    case IRP_MN_QUERY_REMOVE_DEVICE:
        return "IRP_MN_QUERY_REMOVE_DEVICE";
    case IRP_MN_REMOVE_DEVICE:
        return "IRP_MN_REMOVE_DEVICE";
    case IRP_MN_CANCEL_REMOVE_DEVICE:
        return "IRP_MN_CANCEL_REMOVE_DEVICE";
    case IRP_MN_STOP_DEVICE:
        return "IRP_MN_STOP_DEVICE";
    case IRP_MN_QUERY_STOP_DEVICE:
        return "IRP_MN_QUERY_STOP_DEVICE";
    case IRP_MN_CANCEL_STOP_DEVICE:
        return "IRP_MN_CANCEL_STOP_DEVICE";
    case IRP_MN_QUERY_DEVICE_RELATIONS:
        return "IRP_MN_QUERY_DEVICE_RELATIONS";
    case IRP_MN_QUERY_INTERFACE:
        return "IRP_MN_QUERY_INTERFACE";
    case IRP_MN_QUERY_CAPABILITIES:
        return "IRP_MN_QUERY_CAPABILITIES";
    case IRP_MN_QUERY_RESOURCES:
        return "IRP_MN_QUERY_RESOURCES";
    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
        return "IRP_MN_QUERY_RESOURCE_REQUIREMENTS";
    case IRP_MN_QUERY_DEVICE_TEXT:
        return "IRP_MN_QUERY_DEVICE_TEXT";
    case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
        return "IRP_MN_FILTER_RESOURCE_REQUIREMENTS";
    case IRP_MN_READ_CONFIG:
        return "IRP_MN_READ_CONFIG";
    case IRP_MN_WRITE_CONFIG:
        return "IRP_MN_WRITE_CONFIG";
    case IRP_MN_EJECT:
        return "IRP_MN_EJECT";
    case IRP_MN_SET_LOCK:
        return "IRP_MN_SET_LOCK";
    case IRP_MN_QUERY_ID:
        return "IRP_MN_QUERY_ID";
    case IRP_MN_QUERY_PNP_DEVICE_STATE:
        return "IRP_MN_QUERY_PNP_DEVICE_STATE";
    case IRP_MN_QUERY_BUS_INFORMATION:
        return "IRP_MN_QUERY_BUS_INFORMATION";
    case IRP_MN_DEVICE_USAGE_NOTIFICATION:
        return "IRP_MN_DEVICE_USAGE_NOTIFICATION";
    case IRP_MN_SURPRISE_REMOVAL:
        return "IRP_MN_SURPRISE_REMOVAL";
    case IRP_MN_QUERY_LEGACY_BUS_INFORMATION:
        return "IRP_MN_QUERY_LEGACY_BUS_INFORMATION";
    default:
        return "unknown_pnp_irp";
    }
}

PCHAR
DbgDeviceRelationString(
    DEVICE_RELATION_TYPE Type
    )
{
    switch (Type)
    {
    case BusRelations:
        return "BusRelations";
    case EjectionRelations:
        return "EjectionRelations";
    case RemovalRelations:
        return "RemovalRelations";
    case TargetDeviceRelation:
        return "TargetDeviceRelation";
    default:
        return "UnKnown Relation";
    }
}

PCHAR
DbgDeviceIDString(
    BUS_QUERY_ID_TYPE Type
    )
{
    switch (Type)
    {
    case BusQueryDeviceID:
        return "BusQueryDeviceID";
    case BusQueryHardwareIDs:
        return "BusQueryHardwareIDs";
    case BusQueryCompatibleIDs:
        return "BusQueryCompatibleIDs";
    case BusQueryInstanceID:
        return "BusQueryInstanceID";
    case BusQueryDeviceSerialNumber:
        return "BusQueryDeviceSerialNumber";
    default:
        return "UnKnown ID";
    }
}

#endif
