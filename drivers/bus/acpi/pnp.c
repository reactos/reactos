#include "precomp.h"

#define NDEBUG
#include <debug.h>

NTSTATUS
Bus_PlugInDevice (
    struct acpi_device *Device,
    PFDO_DEVICE_DATA            FdoData
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

    PAGED_CODE ();

    irpStack = IoGetCurrentIrpStackLocation (Irp);
    ASSERT (IRP_MJ_PNP == irpStack->MajorFunction);

    commonData = (PCOMMON_DEVICE_DATA) DeviceObject->DeviceExtension;


    if (commonData->IsFDO) {
        DPRINT("FDO %s IRP:0x%p\n",
                      PnPMinorFunctionString(irpStack->MinorFunction),
                      Irp);
        //
        // Request is for the bus FDO
        //
        status = Bus_FDO_PnP (
                    DeviceObject,
                    Irp,
                    irpStack,
                    (PFDO_DEVICE_DATA) commonData);
    } else {
        DPRINT("PDO %s IRP: 0x%p\n",
                      PnPMinorFunctionString(irpStack->MinorFunction),
                      Irp);
        //
        // Request is for the child PDO.
        //
        status = Bus_PDO_PnP (
                    DeviceObject,
                    Irp,
                    irpStack,
                    (PPDO_DEVICE_DATA) commonData);
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
    NTSTATUS            status;
    ULONG               length, prevcount, numPdosPresent;
    PLIST_ENTRY         entry; 
    PPDO_DEVICE_DATA    pdoData;
    PDEVICE_RELATIONS   relations, oldRelations;

    PAGED_CODE ();

    switch (IrpStack->MinorFunction) {

    case IRP_MN_START_DEVICE:

        status = Bus_StartFdo (DeviceData, Irp);


        //
        // We must now complete the IRP, since we stopped it in the
        // completion routine with MORE_PROCESSING_REQUIRED.
        //

        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);

        return status;

    case IRP_MN_QUERY_STOP_DEVICE:

        //
        // The PnP manager is trying to stop the device
        // for resource rebalancing. 
        //
        SET_NEW_PNP_STATE(DeviceData->Common, StopPending);
        Irp->IoStatus.Status = STATUS_SUCCESS;
        break;

    case IRP_MN_CANCEL_STOP_DEVICE:

        //
        // The PnP Manager sends this IRP, at some point after an
        // IRP_MN_QUERY_STOP_DEVICE, to inform the drivers for a
        // device that the device will not be stopped for
        // resource reconfiguration.
        //
        //
        // First check to see whether you have received cancel-stop
        // without first receiving a query-stop. This could happen if
        //  someone above us fails a query-stop and passes down the subsequent
        // cancel-stop.
        //

        if (StopPending == DeviceData->Common.DevicePnPState)
        {
            //
            // We did receive a query-stop, so restore.
            //
            RESTORE_PREVIOUS_PNP_STATE(DeviceData->Common);
            ASSERT(DeviceData->Common.DevicePnPState == Started);
        }
        Irp->IoStatus.Status = STATUS_SUCCESS; // We must not fail the IRP.
        break;

    case IRP_MN_QUERY_DEVICE_RELATIONS:

        DPRINT("\tQueryDeviceRelation Type: %s\n",
                    DbgDeviceRelationString(\
                    IrpStack->Parameters.QueryDeviceRelations.Type));

        if (BusRelations != IrpStack->Parameters.QueryDeviceRelations.Type) {
            //
            // We don't support any other Device Relations
            //
            break;
        }


        ExAcquireFastMutex (&DeviceData->Mutex);

        oldRelations = (PDEVICE_RELATIONS) Irp->IoStatus.Information;
        if (oldRelations) {
            prevcount = oldRelations->Count;
            if (!DeviceData->NumPDOs) {
                //
                // There is a device relations struct already present and we have
                // nothing to add to it, so just call IoSkip and IoCall
                //
                ExReleaseFastMutex (&DeviceData->Mutex);
                break;
            }
        }
        else  {
            prevcount = 0;
        }

        //
        // Calculate the number of PDOs actually present on the bus
        //
        numPdosPresent = 0;
        for (entry = DeviceData->ListOfPDOs.Flink;
             entry != &DeviceData->ListOfPDOs;
             entry = entry->Flink) {
            pdoData = CONTAINING_RECORD (entry, PDO_DEVICE_DATA, Link);
            numPdosPresent++;
        }

        //
        // Need to allocate a new relations structure and add our
        // PDOs to it.
        //

        length = sizeof(DEVICE_RELATIONS) +
                (((numPdosPresent + prevcount) - 1) * sizeof (PDEVICE_OBJECT));

        relations = ExAllocatePoolWithTag(PagedPool, length, 'IpcA');

        if (NULL == relations) {
            //
            // Fail the IRP
            //
            ExReleaseFastMutex (&DeviceData->Mutex);
            Irp->IoStatus.Status = status = STATUS_INSUFFICIENT_RESOURCES;
            IoCompleteRequest (Irp, IO_NO_INCREMENT);
            return status;

        }

        //
        // Copy in the device objects so far
        //
        if (prevcount) {
            RtlCopyMemory (relations->Objects, oldRelations->Objects,
                                      prevcount * sizeof (PDEVICE_OBJECT));
        }

        relations->Count = prevcount + numPdosPresent;

        //
        // For each PDO present on this bus add a pointer to the device relations
        // buffer, being sure to take out a reference to that object.
        // The Plug & Play system will dereference the object when it is done
        // with it and free the device relations buffer.
        //

        for (entry = DeviceData->ListOfPDOs.Flink;
             entry != &DeviceData->ListOfPDOs;
             entry = entry->Flink) {

            pdoData = CONTAINING_RECORD (entry, PDO_DEVICE_DATA, Link);
            relations->Objects[prevcount] = pdoData->Common.Self;
            ObReferenceObject (pdoData->Common.Self);
            prevcount++;
        }

        DPRINT("\t#PDOs present = %d\n\t#PDOs reported = %d\n",
                             DeviceData->NumPDOs, relations->Count);

        //
        // Replace the relations structure in the IRP with the new
        // one.
        //
        if (oldRelations) {
            ExFreePoolWithTag(oldRelations, 0);
        }
        Irp->IoStatus.Information = (ULONG_PTR) relations;

        ExReleaseFastMutex (&DeviceData->Mutex);

        //
        // Set up and pass the IRP further down the stack
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
        break;

    default:

        //
        // In the default case we merely call the next driver.
        // We must not modify Irp->IoStatus.Status or complete the IRP.
        //

        break;
    }

    IoSkipCurrentIrpStackLocation (Irp);
    status = IoCallDriver (DeviceData->NextLowerDriver, Irp);
    return STATUS_SUCCESS;
}

NTSTATUS
Bus_StartFdo (
    PFDO_DEVICE_DATA            FdoData,
    PIRP   Irp )
{
    NTSTATUS status = STATUS_SUCCESS;
    POWER_STATE powerState;
    ACPI_STATUS AcpiStatus;

    PAGED_CODE ();

    FdoData->Common.DevicePowerState = PowerDeviceD0;
    powerState.DeviceState = PowerDeviceD0;
    PoSetPowerState ( FdoData->Common.Self, DevicePowerState, powerState );

    SET_NEW_PNP_STATE(FdoData->Common, Started);

    AcpiStatus = AcpiInitializeSubsystem();
    if(ACPI_FAILURE(AcpiStatus)){
        DPRINT1("Unable to AcpiInitializeSubsystem\n");
        return STATUS_UNSUCCESSFUL;
    }
    
    
	AcpiStatus = AcpiInitializeTables(NULL, 16, 0);
    if (ACPI_FAILURE(status)){
        DPRINT1("Unable to AcpiInitializeSubsystem\n");
		return STATUS_UNSUCCESSFUL;
    }

    AcpiStatus = AcpiLoadTables();
    if(ACPI_FAILURE(AcpiStatus)){
        DPRINT1("Unable to AcpiLoadTables\n");
        AcpiTerminate();
        return STATUS_UNSUCCESSFUL;
    }

	DPRINT("Acpi subsystem init\n");
    /* Initialize ACPI bus manager */
    AcpiStatus = acpi_init();
    if (!ACPI_SUCCESS(AcpiStatus)) {
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
    PIRP Irp
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
                           TRUE
                           );

    status = IoCallDriver(DeviceObject, Irp);

    //
    // Wait for lower drivers to be done with the Irp.
    // Important thing to note here is when you allocate
    // the memory for an event in the stack you must do a
    // KernelMode wait instead of UserMode to prevent
    // the stack from getting paged out.
    //

    if (status == STATUS_PENDING) {
       KeWaitForSingleObject(&event,
                             Executive,
                             KernelMode,
                             FALSE,
                             NULL
                             );
       status = Irp->IoStatus.Status;
    }

    return status;
}

NTSTATUS
NTAPI
Bus_CompletionRoutine(
    PDEVICE_OBJECT   DeviceObject,
    PIRP             Irp,
    PVOID            Context
    )
{
    UNREFERENCED_PARAMETER (DeviceObject);

    //
    // If the lower driver didn't return STATUS_PENDING, we don't need to
    // set the event because we won't be waiting on it.
    // This optimization avoids grabbing the dispatcher lock and improves perf.
    //
    if (Irp->PendingReturned == TRUE) {

        KeSetEvent ((PKEVENT) Context, IO_NO_INCREMENT, FALSE);
    }
    return STATUS_MORE_PROCESSING_REQUIRED; // Keep this IRP
}

NTSTATUS
Bus_DestroyPdo (
    PDEVICE_OBJECT      Device,
    PPDO_DEVICE_DATA    PdoData
    )
{
    PAGED_CODE ();

    //
    // BusEnum does not queue any irps at this time so we have nothing to do.
    //

    //
    // Free any resources.
    //

    if (PdoData->HardwareIDs) {
        ExFreePoolWithTag(PdoData->HardwareIDs, 'DpcA');
        PdoData->HardwareIDs = NULL;
    }

    DPRINT("\tDeleting PDO: 0x%p\n", Device);
    IoDeleteDevice (Device);
    return STATUS_SUCCESS;
}


VOID
Bus_InitializePdo (
    PDEVICE_OBJECT      Pdo,
    PFDO_DEVICE_DATA    FdoData
    )
{
    PPDO_DEVICE_DATA pdoData;
    int acpistate;
    DEVICE_POWER_STATE ntState;

    PAGED_CODE ();

    pdoData = (PPDO_DEVICE_DATA)  Pdo->DeviceExtension;

    DPRINT("pdo 0x%p, extension 0x%p\n", Pdo, pdoData);

    if (pdoData->AcpiHandle)
        acpi_bus_get_power(pdoData->AcpiHandle, &acpistate);
    else
        acpistate = ACPI_STATE_D0;

    switch(acpistate)
    {
        case ACPI_STATE_D0:
            ntState = PowerDeviceD0;
            break;
        case ACPI_STATE_D1:
            ntState = PowerDeviceD1;
            break;
        case ACPI_STATE_D2:
            ntState = PowerDeviceD2;
            break;
        case ACPI_STATE_D3:
            ntState = PowerDeviceD3;
            break;
        default:
            DPRINT1("Unknown power state (%d) returned by acpi\n",acpistate);
            ntState = PowerDeviceUnspecified;
            break;
    }

    //
    // Initialize the rest
    //
    pdoData->Common.IsFDO = FALSE;
    pdoData->Common.Self =  Pdo;

    pdoData->ParentFdo = FdoData->Common.Self;


    INITIALIZE_PNP_STATE(pdoData->Common);

    pdoData->Common.DevicePowerState = ntState;
    pdoData->Common.SystemPowerState = FdoData->Common.SystemPowerState;

    Pdo->Flags |= DO_POWER_PAGABLE;

    ExAcquireFastMutex (&FdoData->Mutex);
    InsertTailList(&FdoData->ListOfPDOs, &pdoData->Link);
    FdoData->NumPDOs++;
    ExReleaseFastMutex (&FdoData->Mutex);

    // This should be the last step in initialization.
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


