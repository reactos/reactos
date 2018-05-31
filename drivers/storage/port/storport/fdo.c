/*
 * PROJECT:     ReactOS Storport Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Storport FDO code
 * COPYRIGHT:   Copyright 2017 Eric Kohl (eric.kohl@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ******************************************************************/

static
BOOLEAN
NTAPI
PortFdoInterruptRoutine(
    _In_ PKINTERRUPT Interrupt,
    _In_ PVOID ServiceContext)
{
    PFDO_DEVICE_EXTENSION DeviceExtension;

    DPRINT1("PortFdoInterruptRoutine(%p %p)\n",
            Interrupt, ServiceContext);

    DeviceExtension = (PFDO_DEVICE_EXTENSION)ServiceContext;

    return MiniportHwInterrupt(&DeviceExtension->Miniport);
}


static
NTSTATUS
PortFdoConnectInterrupt(
    _In_ PFDO_DEVICE_EXTENSION DeviceExtension)
{
    ULONG Vector;
    KIRQL Irql;
    KINTERRUPT_MODE InterruptMode;
    BOOLEAN ShareVector;
    KAFFINITY Affinity;
    NTSTATUS Status;

    DPRINT1("PortFdoConnectInterrupt(%p)\n",
            DeviceExtension);

    /* No resources, no interrupt. Done! */
    if (DeviceExtension->AllocatedResources == NULL ||
        DeviceExtension->TranslatedResources == NULL)
    {
        DPRINT1("Checkpoint\n");
        return STATUS_SUCCESS;
    }

    /* Get the interrupt data from the resource list */
    Status = GetResourceListInterrupt(DeviceExtension,
                                      &Vector,
                                      &Irql,
                                      &InterruptMode,
                                      &ShareVector,
                                      &Affinity);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("GetResourceListInterrupt() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    DPRINT1("Vector: %lu\n", Vector);
    DPRINT1("Irql: %lu\n", Irql);

    DPRINT1("Affinity: 0x%08lx\n", Affinity);

    /* Connect the interrupt */
    Status = IoConnectInterrupt(&DeviceExtension->Interrupt,
                                PortFdoInterruptRoutine,
                                DeviceExtension,
                                NULL,
                                Vector,
                                Irql,
                                Irql,
                                InterruptMode,
                                ShareVector,
                                Affinity,
                                FALSE);
    if (NT_SUCCESS(Status))
    {
        DeviceExtension->InterruptIrql = Irql;
    }
    else
    {
        DPRINT1("IoConnectInterrupt() failed (Status 0x%08lx)\n", Status);
    }

    return Status;
}


static
NTSTATUS
PortFdoStartMiniport(
    _In_ PFDO_DEVICE_EXTENSION DeviceExtension)
{
    PHW_INITIALIZATION_DATA InitData;
    INTERFACE_TYPE InterfaceType;
    NTSTATUS Status;

    DPRINT1("PortFdoStartDevice(%p)\n", DeviceExtension);

    /* Get the interface type of the lower device */
    InterfaceType = GetBusInterface(DeviceExtension->LowerDevice);
    if (InterfaceType == InterfaceTypeUndefined)
        return STATUS_NO_SUCH_DEVICE;

    /* Get the driver init data for the given interface type */
    InitData = PortGetDriverInitData(DeviceExtension->DriverExtension,
                                     InterfaceType);
    if (InitData == NULL)
        return STATUS_NO_SUCH_DEVICE;

    /* Initialize the miniport */
    Status = MiniportInitialize(&DeviceExtension->Miniport,
                                DeviceExtension,
                                InitData);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("MiniportInitialize() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Call the miniports FindAdapter function */
    Status = MiniportFindAdapter(&DeviceExtension->Miniport);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("MiniportFindAdapter() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Connect the configured interrupt */
    Status = PortFdoConnectInterrupt(DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("PortFdoConnectInterrupt() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Call the miniports HwInitialize function */
    Status = MiniportHwInitialize(&DeviceExtension->Miniport);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("MiniportHwInitialize() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Call the HwPassiveInitRoutine function, if available */
    if (DeviceExtension->HwPassiveInitRoutine != NULL)
    {
        DPRINT1("Calling HwPassiveInitRoutine()\n");
        if (!DeviceExtension->HwPassiveInitRoutine(&DeviceExtension->Miniport.MiniportExtension->HwDeviceExtension))
        {
            DPRINT1("HwPassiveInitRoutine() failed\n");
            return STATUS_UNSUCCESSFUL;
        }
    }

    return STATUS_SUCCESS;
}


static
NTSTATUS
NTAPI
PortFdoStartDevice(
    _In_ PFDO_DEVICE_EXTENSION DeviceExtension,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    NTSTATUS Status;

    DPRINT1("PortFdoStartDevice(%p %p)\n",
            DeviceExtension, Irp);

    ASSERT(DeviceExtension->ExtensionType == FdoExtension);

    /* Get the current stack location */
    Stack = IoGetCurrentIrpStackLocation(Irp);

    /* Start the lower device if the FDO is in 'stopped' state */
    if (DeviceExtension->PnpState == dsStopped)
    {
        Status = ForwardIrpAndWait(DeviceExtension->LowerDevice, Irp);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ForwardIrpAndWait() failed (Status 0x%08lx)\n", Status);
            return Status;
        }
    }

    /* Change to the 'started' state */
    DeviceExtension->PnpState = dsStarted;

    /* Copy the raw and translated resource lists into the device extension */
    if (Stack->Parameters.StartDevice.AllocatedResources != NULL &&
        Stack->Parameters.StartDevice.AllocatedResourcesTranslated != NULL)
    {
        DeviceExtension->AllocatedResources = CopyResourceList(NonPagedPool,
                                                               Stack->Parameters.StartDevice.AllocatedResources);
        if (DeviceExtension->AllocatedResources == NULL)
            return STATUS_NO_MEMORY;

        DeviceExtension->TranslatedResources = CopyResourceList(NonPagedPool,
                                                                Stack->Parameters.StartDevice.AllocatedResourcesTranslated);
        if (DeviceExtension->TranslatedResources == NULL)
            return STATUS_NO_MEMORY;
    }

    /* Get the bus interface of the lower (bus) device */
    Status = QueryBusInterface(DeviceExtension->LowerDevice,
                               (PGUID)&GUID_BUS_INTERFACE_STANDARD,
                               sizeof(BUS_INTERFACE_STANDARD),
                               1,
                               &DeviceExtension->BusInterface,
                               NULL);
    DPRINT1("Status: 0x%08lx\n", Status);
    if (NT_SUCCESS(Status))
    {
        DPRINT1("Context: %p\n", DeviceExtension->BusInterface.Context);
        DeviceExtension->BusInitialized = TRUE;
    }

    /* Start the miniport (FindAdapter & Initialize) */
    Status = PortFdoStartMiniport(DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("FdoStartMiniport() failed (Status 0x%08lx)\n", Status);
        DeviceExtension->PnpState = dsStopped;
    }

    return Status;
}


static NTSTATUS
SpiSendInquiry(IN PDEVICE_OBJECT DeviceObject,
               ULONG Bus, ULONG Target, ULONG Lun)
{
//    IO_STATUS_BLOCK IoStatusBlock;
//    PIO_STACK_LOCATION IrpStack;
//    KEVENT Event;
//    KIRQL Irql;
//    PIRP Irp;
    NTSTATUS Status;
    PINQUIRYDATA InquiryBuffer;
    PUCHAR /*PSENSE_DATA*/ SenseBuffer;
//    BOOLEAN KeepTrying = TRUE;
//    ULONG RetryCount = 0;
    SCSI_REQUEST_BLOCK Srb;
    PCDB Cdb;
//    PSCSI_PORT_LUN_EXTENSION LunExtension;
//    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;

PFDO_DEVICE_EXTENSION DeviceExtension;
    PVOID SrbExtension = NULL;
    BOOLEAN ret;

    DPRINT1("SpiSendInquiry() called\n");

    DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    InquiryBuffer = ExAllocatePoolWithTag(NonPagedPool, INQUIRYDATABUFFERSIZE, TAG_INQUIRY_DATA);
    if (InquiryBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    SenseBuffer = ExAllocatePoolWithTag(NonPagedPool, SENSE_BUFFER_SIZE, TAG_SENSE_DATA);
    if (SenseBuffer == NULL)
    {
        ExFreePoolWithTag(InquiryBuffer, TAG_INQUIRY_DATA);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (DeviceExtension->Miniport.PortConfig.SrbExtensionSize != 0)
    {
        SrbExtension = ExAllocatePoolWithTag(NonPagedPool, DeviceExtension->Miniport.PortConfig.SrbExtensionSize, TAG_SENSE_DATA);
        if (SrbExtension == NULL)
        {
            ExFreePoolWithTag(SenseBuffer, TAG_SENSE_DATA);
            ExFreePoolWithTag(InquiryBuffer, TAG_INQUIRY_DATA);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

//    while (KeepTrying)
    {
        /* Initialize event for waiting */
//        KeInitializeEvent(&Event,
//                          NotificationEvent,
//                          FALSE);

        /* Create an IRP */
//        Irp = IoBuildDeviceIoControlRequest(IOCTL_SCSI_EXECUTE_IN,
//                                            DeviceObject,
//                                            NULL,
//                                            0,
//                                            InquiryBuffer,
//                                            INQUIRYDATABUFFERSIZE,
//                                            TRUE,
//                                            &Event,
//                                            &IoStatusBlock);
//        if (Irp == NULL)
//        {
//            DPRINT1("IoBuildDeviceIoControlRequest() failed\n");

            /* Quit the loop */
//            Status = STATUS_INSUFFICIENT_RESOURCES;
//            KeepTrying = FALSE;
//            continue;
//        }

        /* Prepare SRB */
        RtlZeroMemory(&Srb, sizeof(SCSI_REQUEST_BLOCK));

        Srb.Length = sizeof(SCSI_REQUEST_BLOCK);
//        Srb.OriginalRequest = Irp;
        Srb.PathId = Bus;
        Srb.TargetId = Target;
        Srb.Lun = Lun;
        Srb.Function = SRB_FUNCTION_EXECUTE_SCSI;
        Srb.SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;
        Srb.TimeOutValue = 4;
        Srb.CdbLength = 6;

        Srb.SenseInfoBuffer = SenseBuffer;
        Srb.SenseInfoBufferLength = SENSE_BUFFER_SIZE;

        Srb.DataBuffer = InquiryBuffer;
        Srb.DataTransferLength = INQUIRYDATABUFFERSIZE;

        Srb.SrbExtension = SrbExtension;

        /* Attach Srb to the Irp */
//        IrpStack = IoGetNextIrpStackLocation(Irp);
//        IrpStack->Parameters.Scsi.Srb = &Srb;

        /* Fill in CDB */
        Cdb = (PCDB)Srb.Cdb;
        Cdb->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;
        Cdb->CDB6INQUIRY.LogicalUnitNumber = Lun;
        Cdb->CDB6INQUIRY.AllocationLength = INQUIRYDATABUFFERSIZE;

        /* Call the driver */


        ret = MiniportStartIo(&DeviceExtension->Miniport,
                              &Srb);
DPRINT1("MiniportStartIo returned %u\n", ret);

//        Status = IoCallDriver(DeviceObject, Irp);

        /* Wait for it to complete */
//        if (Status == STATUS_PENDING)
//        {
//            DPRINT1("SpiSendInquiry(): Waiting for the driver to process request...\n");
//            KeWaitForSingleObject(&Event,
//                                  Executive,
//                                  KernelMode,
//                                  FALSE,
//                                  NULL);
//            Status = IoStatusBlock.Status;
//        }

//        DPRINT1("SpiSendInquiry(): Request processed by driver, status = 0x%08X\n", Status);

        if (SRB_STATUS(Srb.SrbStatus) == SRB_STATUS_SUCCESS)
        {
            /* All fine, copy data over */
//            RtlCopyMemory(LunInfo->InquiryData,
//                          InquiryBuffer,
//                          INQUIRYDATABUFFERSIZE);

            /* Quit the loop */
            Status = STATUS_SUCCESS;
//            KeepTrying = FALSE;
//            continue;
        }

        DPRINT("Inquiry SRB failed with SrbStatus 0x%08X\n", Srb.SrbStatus);
#if 0
        /* Check if the queue is frozen */
        if (Srb.SrbStatus & SRB_STATUS_QUEUE_FROZEN)
        {
            /* Something weird happened, deal with it (unfreeze the queue) */
            KeepTrying = FALSE;

            DPRINT("SpiSendInquiry(): the queue is frozen at TargetId %d\n", Srb.TargetId);

            LunExtension = SpiGetLunExtension(DeviceExtension,
                                              LunInfo->PathId,
                                              LunInfo->TargetId,
                                              LunInfo->Lun);

            /* Clear frozen flag */
            LunExtension->Flags &= ~LUNEX_FROZEN_QUEUE;

            /* Acquire the spinlock */
            KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);

            /* Process the request */
            SpiGetNextRequestFromLun(DeviceObject->DeviceExtension, LunExtension);

            /* SpiGetNextRequestFromLun() releases the spinlock,
                so we just lower irql back to what it was before */
            KeLowerIrql(Irql);
        }

        /* Check if data overrun happened */
        if (SRB_STATUS(Srb.SrbStatus) == SRB_STATUS_DATA_OVERRUN)
        {
            DPRINT("Data overrun at TargetId %d\n", LunInfo->TargetId);

            /* Nothing dramatic, just copy data, but limiting the size */
            RtlCopyMemory(LunInfo->InquiryData,
                            InquiryBuffer,
                            (Srb.DataTransferLength > INQUIRYDATABUFFERSIZE) ?
                            INQUIRYDATABUFFERSIZE : Srb.DataTransferLength);

            /* Quit the loop */
            Status = STATUS_SUCCESS;
            KeepTrying = FALSE;
        }
        else if ((Srb.SrbStatus & SRB_STATUS_AUTOSENSE_VALID) &&
                 SenseBuffer->SenseKey == SCSI_SENSE_ILLEGAL_REQUEST)
        {
            /* LUN is not valid, but some device responds there.
                Mark it as invalid anyway */

            /* Quit the loop */
            Status = STATUS_INVALID_DEVICE_REQUEST;
            KeepTrying = FALSE;
        }
        else
        {
            /* Retry a couple of times if no timeout happened */
            if ((RetryCount < 2) &&
                (SRB_STATUS(Srb.SrbStatus) != SRB_STATUS_NO_DEVICE) &&
                (SRB_STATUS(Srb.SrbStatus) != SRB_STATUS_SELECTION_TIMEOUT))
            {
                RetryCount++;
                KeepTrying = TRUE;
            }
            else
            {
                /* That's all, quit the loop */
                KeepTrying = FALSE;

                /* Set status according to SRB status */
                if (SRB_STATUS(Srb.SrbStatus) == SRB_STATUS_BAD_FUNCTION ||
                    SRB_STATUS(Srb.SrbStatus) == SRB_STATUS_BAD_SRB_BLOCK_LENGTH)
                {
                    Status = STATUS_INVALID_DEVICE_REQUEST;
                }
                else
                {
                    Status = STATUS_IO_DEVICE_ERROR;
                }
            }
        }
#endif
    }

    /* Free buffers */
    if (SrbExtension != NULL)
        ExFreePoolWithTag(SrbExtension, TAG_SENSE_DATA);

    ExFreePoolWithTag(SenseBuffer, TAG_SENSE_DATA);
    ExFreePoolWithTag(InquiryBuffer, TAG_INQUIRY_DATA);

    DPRINT("SpiSendInquiry() done with Status 0x%08X\n", Status);

    return Status;
}


static
NTSTATUS
PortFdoScanBus(
    _In_ PFDO_DEVICE_EXTENSION DeviceExtension)
{
    ULONG Bus, Target, Lun;
    NTSTATUS Status;


    DPRINT1("PortFdoScanBus(%p)\n",
            DeviceExtension);

    DPRINT1("NumberOfBuses: %lu\n", DeviceExtension->Miniport.PortConfig.NumberOfBuses);
    DPRINT1("MaximumNumberOfTargets: %lu\n", DeviceExtension->Miniport.PortConfig.MaximumNumberOfTargets);
    DPRINT1("MaximumNumberOfLogicalUnits: %lu\n", DeviceExtension->Miniport.PortConfig.MaximumNumberOfLogicalUnits);

    /* Scan all buses */
    for (Bus = 0; Bus < DeviceExtension->Miniport.PortConfig.NumberOfBuses; Bus++)
    {
        DPRINT1("Scanning bus %ld\n", Bus);

        /* Scan all targets */
        for (Target = 0; Target < DeviceExtension->Miniport.PortConfig.MaximumNumberOfTargets; Target++)
        {
            DPRINT1("  Scanning target %ld:%ld\n", Bus, Target);

            /* Scan all logical units */
            for (Lun = 0; Lun < DeviceExtension->Miniport.PortConfig.MaximumNumberOfLogicalUnits; Lun++)
            {
                DPRINT1("    Scanning logical unit %ld:%ld:%ld\n", Bus, Target, Lun);

                Status = SpiSendInquiry(DeviceExtension->Device, Bus, Target, Lun);
                DPRINT1("SpiSendInquiry returned 0x%08lx\n", Status);
            }
        }
    }

    DPRINT1("Done!\n");

    return STATUS_SUCCESS;
}


static
NTSTATUS
PortFdoQueryBusRelations(
    _In_ PFDO_DEVICE_EXTENSION DeviceExtension,
    _Out_ PULONG_PTR Information)
{
    NTSTATUS Status = STATUS_SUCCESS;;

    DPRINT1("PortFdoQueryBusRelations(%p %p)\n",
            DeviceExtension, Information);

    Status = PortFdoScanBus(DeviceExtension);

    *Information = 0;

    return Status;
}


static
NTSTATUS
PortFdoFilterRequirements(
    PFDO_DEVICE_EXTENSION DeviceExtension,
    PIRP Irp)
{
    PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList;

    DPRINT1("PortFdoFilterRequirements(%p %p)\n", DeviceExtension, Irp);

    /* Get the bus number and the slot number */
    RequirementsList =(PIO_RESOURCE_REQUIREMENTS_LIST)Irp->IoStatus.Information;
    if (RequirementsList != NULL)
    {
        DeviceExtension->BusNumber = RequirementsList->BusNumber;
        DeviceExtension->SlotNumber = RequirementsList->SlotNumber;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
PortFdoScsi(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PFDO_DEVICE_EXTENSION DeviceExtension;
//    PIO_STACK_LOCATION Stack;
    ULONG_PTR Information = 0;
    NTSTATUS Status = STATUS_NOT_SUPPORTED;

    DPRINT1("PortFdoScsi(%p %p)\n",
            DeviceObject, Irp);

    DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(DeviceExtension);
    ASSERT(DeviceExtension->ExtensionType == FdoExtension);

//    Stack = IoGetCurrentIrpStackLocation(Irp);


    Irp->IoStatus.Information = Information;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}


NTSTATUS
NTAPI
PortFdoPnp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PFDO_DEVICE_EXTENSION DeviceExtension;
    PIO_STACK_LOCATION Stack;
    ULONG_PTR Information = 0;
    NTSTATUS Status = STATUS_NOT_SUPPORTED;

    DPRINT1("PortFdoPnp(%p %p)\n",
            DeviceObject, Irp);

    DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(DeviceExtension);
    ASSERT(DeviceExtension->ExtensionType == FdoExtension);

    Stack = IoGetCurrentIrpStackLocation(Irp);

    switch (Stack->MinorFunction)
    {
        case IRP_MN_START_DEVICE: /* 0x00 */
            DPRINT1("IRP_MJ_PNP / IRP_MN_START_DEVICE\n");
            Status = PortFdoStartDevice(DeviceExtension, Irp);
            break;

        case IRP_MN_QUERY_REMOVE_DEVICE: /* 0x01 */
            DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_REMOVE_DEVICE\n");
            break;

        case IRP_MN_REMOVE_DEVICE: /* 0x02 */
            DPRINT1("IRP_MJ_PNP / IRP_MN_REMOVE_DEVICE\n");
            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE: /* 0x03 */
            DPRINT1("IRP_MJ_PNP / IRP_MN_CANCEL_REMOVE_DEVICE\n");
            break;

        case IRP_MN_STOP_DEVICE: /* 0x04 */
            DPRINT1("IRP_MJ_PNP / IRP_MN_STOP_DEVICE\n");
            break;

        case IRP_MN_QUERY_STOP_DEVICE: /* 0x05 */
            DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_STOP_DEVICE\n");
            break;

        case IRP_MN_CANCEL_STOP_DEVICE: /* 0x06 */
            DPRINT1("IRP_MJ_PNP / IRP_MN_CANCEL_STOP_DEVICE\n");
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS: /* 0x07 */
            DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS\n");
            switch (Stack->Parameters.QueryDeviceRelations.Type)
            {
                case BusRelations:
                    DPRINT1("    IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / BusRelations\n");
                    Status = PortFdoQueryBusRelations(DeviceExtension, &Information);
                    break;

                case RemovalRelations:
                    DPRINT1("    IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / RemovalRelations\n");
                    return ForwardIrpAndForget(DeviceExtension->LowerDevice, Irp);

                default:
                    DPRINT1("    IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / Unknown type 0x%lx\n",
                            Stack->Parameters.QueryDeviceRelations.Type);
                    return ForwardIrpAndForget(DeviceExtension->LowerDevice, Irp);
            }
            break;

        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS: /* 0x0d */
            DPRINT1("IRP_MJ_PNP / IRP_MN_FILTER_RESOURCE_REQUIREMENTS\n");
            PortFdoFilterRequirements(DeviceExtension, Irp);
            return ForwardIrpAndForget(DeviceExtension->LowerDevice, Irp);

        case IRP_MN_QUERY_PNP_DEVICE_STATE: /* 0x14 */
            DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_PNP_DEVICE_STATE\n");
            break;

        case IRP_MN_DEVICE_USAGE_NOTIFICATION: /* 0x16 */
            DPRINT1("IRP_MJ_PNP / IRP_MN_DEVICE_USAGE_NOTIFICATION\n");
            break;

        case IRP_MN_SURPRISE_REMOVAL: /* 0x17 */
            DPRINT1("IRP_MJ_PNP / IRP_MN_SURPRISE_REMOVAL\n");
            break;

        default:
            DPRINT1("IRP_MJ_PNP / Unknown IOCTL 0x%lx\n", Stack->MinorFunction);
            return ForwardIrpAndForget(DeviceExtension->LowerDevice, Irp);
    }

    Irp->IoStatus.Information = Information;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

/* EOF */
