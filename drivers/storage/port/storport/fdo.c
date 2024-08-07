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

    // DPRINT1("PortFdoInterruptRoutine(%p %p)\n",
    //         Interrupt, ServiceContext);

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

    DPRINT("PortFdoConnectInterrupt(%p)\n",
            DeviceExtension);

    /* No resources, no interrupt. Done! */
    if (DeviceExtension->AllocatedResources == NULL ||
        DeviceExtension->TranslatedResources == NULL)
    {
        DPRINT("Checkpoint\n");
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
        DPRINT("GetResourceListInterrupt() failed (Status 0x%08lx)\n", Status);
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

    DPRINT1("PortFdoStartMiniport(%p)\n", DeviceExtension);

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

    // FIXME: Find an appropriate place
    DeviceExtension->OutstandingRequestMax = DeviceExtension->Miniport.PortConfig.MaxNumberOfIO;

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
    PCONFIGURATION_INFORMATION ConfigInfo;
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
        if (IoForwardIrpSynchronously(DeviceExtension->LowerDevice, Irp))
        {
            Status = Irp->IoStatus.Status;
        }
        else
        {
            Status = STATUS_UNSUCCESSFUL;
        }

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Lower device failed the IRP (Status 0x%08lx)\n", Status);
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

    /* Claim and increase SCSI port number */
    /* TODO: Reverse this when stopping adapter */
    ConfigInfo = IoGetConfigurationInformation();
    DeviceExtension->ScsiPortNumber = ConfigInfo->ScsiPortCount;
    ConfigInfo->ScsiPortCount++;

    return Status;
}


static
NTSTATUS
PortSendReportLuns(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension,
    _Out_ PULONG LunCount)
{
    NTSTATUS Status;
    SCSI_REQUEST_BLOCK Srb;
    PCDB Cdb;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION IrpStack;
    KEVENT Event;
    PSENSE_DATA SenseBuffer;
    ULONG BufferSize;
    PREPORT_LUNS_DATA ReportLunsData;
    BOOLEAN KeepTrying = TRUE;
    BOOLEAN BufferReallocated = FALSE;
    ULONG RetryCount = 0;

    DPRINT("PortSendReportLuns(%p)\n", PdoExtension);
    
    /* Allocate sense buffer */
    /* TODO: Reuse sense buffers */
    /* FIXME: Do we need sense buffer here? Can I pass a NULL? */
    SenseBuffer = ExAllocatePoolWithTag(NonPagedPool, SENSE_BUFFER_SIZE, TAG_SENSE_DATA);
    if (SenseBuffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Allocate REPORT LUNS return buffer. Use buffer size for the case of a single LUN */
    BufferSize = sizeof(REPORT_LUNS_DATA);
    ReportLunsData = 
        (PREPORT_LUNS_DATA)ExAllocatePoolWithTag(NonPagedPool, BufferSize, TAG_REPORT_LUN_DATA);
    if (ReportLunsData == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    while (KeepTrying)
    {
        /* Initialize event for waiting */
        KeInitializeEvent(&Event,
                          NotificationEvent,
                          FALSE);

        /* Create an IRP */
        Irp = IoBuildDeviceIoControlRequest(IOCTL_SCSI_EXECUTE_IN,
                                            PdoExtension->Device,
                                            NULL,
                                            0,
                                            ReportLunsData,
                                            BufferSize,
                                            TRUE,
                                            &Event,
                                            &IoStatusBlock);
        if (Irp == NULL)
        {
            DPRINT("IoBuildDeviceIoControlRequest() failed\n");

            /* Quit the loop */
            Status = STATUS_INSUFFICIENT_RESOURCES;
            KeepTrying = FALSE;
            continue;
        }

        /* Prepare SRB */
        RtlZeroMemory(&Srb, sizeof(SCSI_REQUEST_BLOCK));

        Srb.Length = sizeof(SCSI_REQUEST_BLOCK);
        Srb.OriginalRequest = Irp;
        Srb.PathId = PdoExtension->Bus;
        Srb.TargetId = PdoExtension->Target;
        Srb.Lun = PdoExtension->Lun;
        Srb.Function = SRB_FUNCTION_EXECUTE_SCSI;
        Srb.SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;
        Srb.TimeOutValue = 4;
        Srb.CdbLength = 12;

        Srb.SenseInfoBuffer = SenseBuffer;
        Srb.SenseInfoBufferLength = SENSE_BUFFER_SIZE;

        Srb.DataBuffer = ReportLunsData;
        Srb.DataTransferLength = BufferSize;

        /* Attach Srb to the Irp */
        IrpStack = IoGetNextIrpStackLocation(Irp);
        IrpStack->Parameters.Scsi.Srb = &Srb;

        /* Fill in CDB */
        Cdb = (PCDB)Srb.Cdb;
        Cdb->REPORT_LUNS.OperationCode = SCSIOP_REPORT_LUNS;
        Cdb->REPORT_LUNS.AllocationLength[0] = (BufferSize >> (0*8)) & 0xFF;
        Cdb->REPORT_LUNS.AllocationLength[1] = (BufferSize >> (1*8)) & 0xFF;
        Cdb->REPORT_LUNS.AllocationLength[2] = (BufferSize >> (2*8)) & 0xFF;
        Cdb->REPORT_LUNS.AllocationLength[3] = (BufferSize >> (3*8)) & 0xFF;

        /* Call the driver */
        Status = IoCallDriver(PdoExtension->Device, Irp);

        /* Wait for it to complete */
        if (Status == STATUS_PENDING || Srb.SrbStatus == SRB_STATUS_PENDING)
        {
            DPRINT1("PortSendReportLuns(): Waiting for the driver to process request...\n");
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            Status = IoStatusBlock.Status;
        }

        DPRINT("PortSendReportLuns(): Request processed by driver, status = 0x%08X\n", Status);

        if (SRB_STATUS(Srb.SrbStatus) == SRB_STATUS_SUCCESS)
        {
            ULONG DataSize = 0;

            /* Extract LUN count from the return data. This is the sole purpose of this function */
            DataSize |= (((ULONG)ReportLunsData->LunListLength[0]) << (8*0));
            DataSize |= (((ULONG)ReportLunsData->LunListLength[1]) << (8*1));
            DataSize |= (((ULONG)ReportLunsData->LunListLength[2]) << (8*2));
            DataSize |= (((ULONG)ReportLunsData->LunListLength[3]) << (8*3));
            *LunCount = ((DataSize - FIELD_OFFSET(REPORT_LUNS_DATA, LunDescriptor)) / 
                         sizeof(LUN_DESCRIPTOR));

            /* Quit the loop */
            Status = STATUS_SUCCESS;
            KeepTrying = FALSE;
            continue;
        }

        DPRINT1("REPORT_LUNS SRB failed with SrbStatus 0x%08X Status %08X\n",
                Srb.SrbStatus, Status);

        /* Check if the queue is frozen */
        if (Srb.SrbStatus & SRB_STATUS_QUEUE_FROZEN)
        {
            /* Something weird happened, deal with it (unfreeze the queue) */
            KeepTrying = FALSE;

            DPRINT1("PortSendReportLuns(): the queue is frozen at TargetId %d\n", Srb.TargetId);
            /* TODO: What do we do with this crap */
//            LunExtension = SpiGetLunExtension(DeviceExtension,
//                                              LunInfo->PathId,
//                                              LunInfo->TargetId,
//                                              LunInfo->Lun);

            /* Clear frozen flag */
//            LunExtension->Flags &= ~LUNEX_FROZEN_QUEUE;

            /* Acquire the spinlock */
//            KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);

            /* Process the request */
//            SpiGetNextRequestFromLun(DeviceObject->DeviceExtension, LunExtension);

            /* SpiGetNextRequestFromLun() releases the spinlock,
                so we just lower irql back to what it was before */
//            KeLowerIrql(Irql);
        }

        /* Check if data overrun happened, then resize buffer once */
        /* FIXME: Needs coverage test */
        if (SRB_STATUS(Srb.SrbStatus) == SRB_STATUS_DATA_OVERRUN)
        {
            /* If the buffer has already been reallocated we ditch the device */
            if (BufferReallocated)
            {
                DPRINT1("Data overrun again (!) at TargetId %d, give up\n", PdoExtension->Target);

                Status = STATUS_IO_DEVICE_ERROR;
                KeepTrying = FALSE;
                continue;
            }

            /* Miniport will return how many bytes should we actually allocate */
            BufferSize = 0;
            BufferSize |= (((ULONG)ReportLunsData->LunListLength[0]) << (8*0));
            BufferSize |= (((ULONG)ReportLunsData->LunListLength[1]) << (8*1));
            BufferSize |= (((ULONG)ReportLunsData->LunListLength[2]) << (8*2));
            BufferSize |= (((ULONG)ReportLunsData->LunListLength[3]) << (8*3));

            DPRINT1("Data overrun at TargetId %d, %d bytes needed\n",
                   PdoExtension->Target,
                   BufferSize);
            
            /* Allocate a larger buffer */
            ExFreePoolWithTag(ReportLunsData, TAG_REPORT_LUN_DATA);
            ReportLunsData = ExAllocatePoolWithTag(NonPagedPool, BufferSize, TAG_REPORT_LUN_DATA);
            if (ReportLunsData == NULL)
            {
                DPRINT1("Cannot reallocate REPORT_LUNS buffer\n");

                Status = STATUS_INSUFFICIENT_RESOURCES;
                KeepTrying = FALSE;
                continue;
            }
            BufferReallocated = TRUE;

            /* Continue trying with a larger buffer */
            continue;
        }else
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
    }

    /* Free Sense data and Report LUNs buffer */
    ExFreePoolWithTag(ReportLunsData, TAG_REPORT_LUN_DATA);
    ExFreePoolWithTag(SenseBuffer, TAG_SENSE_DATA);

    DPRINT1("PortSendReportLuns() done with Status 0x%08X\n", Status);

    return Status;
}


static
NTSTATUS
PortSendInquiry(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension)
{
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION IrpStack;
    KEVENT Event;
//    KIRQL Irql;
    PIRP Irp;
    NTSTATUS Status;
    PSENSE_DATA SenseBuffer;
    BOOLEAN KeepTrying = TRUE;
    ULONG RetryCount = 0;
    SCSI_REQUEST_BLOCK Srb;
    PCDB Cdb;
//    PSCSI_PORT_LUN_EXTENSION LunExtension;
//    PFDO_DEVICE_EXTENSION DeviceExtension;

    DPRINT("PortSendInquiry(%p)\n", PdoExtension);

    if (PdoExtension->InquiryBuffer == NULL)
    {
        PdoExtension->InquiryBuffer = ExAllocatePoolWithTag(NonPagedPool, INQUIRYDATABUFFERSIZE, TAG_INQUIRY_DATA);
        if (PdoExtension->InquiryBuffer == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;
    }

    SenseBuffer = ExAllocatePoolWithTag(NonPagedPool, SENSE_BUFFER_SIZE, TAG_SENSE_DATA);
    if (SenseBuffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    while (KeepTrying)
    {
        /* Initialize event for waiting */
        KeInitializeEvent(&Event,
                          NotificationEvent,
                          FALSE);

        /* Create an IRP */
        Irp = IoBuildDeviceIoControlRequest(IOCTL_SCSI_EXECUTE_IN,
                                            PdoExtension->Device,
                                            NULL,
                                            0,
                                            PdoExtension->InquiryBuffer,
                                            INQUIRYDATABUFFERSIZE,
                                            TRUE,
                                            &Event,
                                            &IoStatusBlock);
        if (Irp == NULL)
        {
            DPRINT1("IoBuildDeviceIoControlRequest() failed\n");

            /* Quit the loop */
            Status = STATUS_INSUFFICIENT_RESOURCES;
            KeepTrying = FALSE;
            continue;
        }

        /* Prepare SRB */
        RtlZeroMemory(&Srb, sizeof(SCSI_REQUEST_BLOCK));

        Srb.Length = sizeof(SCSI_REQUEST_BLOCK);
        Srb.OriginalRequest = Irp;
        Srb.PathId = PdoExtension->Bus;
        Srb.TargetId = PdoExtension->Target;
        Srb.Lun = PdoExtension->Lun;
        Srb.Function = SRB_FUNCTION_EXECUTE_SCSI;
        Srb.SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;
        Srb.TimeOutValue = 4;
        Srb.CdbLength = 6;

        Srb.SenseInfoBuffer = SenseBuffer;
        Srb.SenseInfoBufferLength = SENSE_BUFFER_SIZE;

        Srb.DataBuffer = PdoExtension->InquiryBuffer;
        Srb.DataTransferLength = INQUIRYDATABUFFERSIZE;

        /* Attach Srb to the Irp */
        IrpStack = IoGetNextIrpStackLocation(Irp);
        IrpStack->Parameters.Scsi.Srb = &Srb;

        /* Fill in CDB */
        Cdb = (PCDB)Srb.Cdb;
        Cdb->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;
        Cdb->CDB6INQUIRY.LogicalUnitNumber = PdoExtension->Lun;
        Cdb->CDB6INQUIRY.AllocationLength = INQUIRYDATABUFFERSIZE;

        /* Call the driver */
        Status = IoCallDriver(PdoExtension->Device, Irp);

        /* Wait for it to complete */
        if (Status == STATUS_PENDING)
        {
            DPRINT1("PortSendInquiry(): Waiting for the driver to process request...\n");
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            Status = IoStatusBlock.Status;
        }

        DPRINT1("PortSendInquiry(): Request processed by driver, status = 0x%08X\n", Status);

        if (SRB_STATUS(Srb.SrbStatus) == SRB_STATUS_SUCCESS)
        {
            DPRINT1("Found a device!\n");

            /* Quit the loop */
            Status = STATUS_SUCCESS;
            KeepTrying = FALSE;
            continue;
        }

        DPRINT1("Inquiry SRB failed with SrbStatus 0x%08X\n", Srb.SrbStatus);

        /* Check if the queue is frozen */
        if (Srb.SrbStatus & SRB_STATUS_QUEUE_FROZEN)
        {
            /* Something weird happened, deal with it (unfreeze the queue) */
            KeepTrying = FALSE;

            DPRINT1("SpiSendInquiry(): the queue is frozen at TargetId %d\n", Srb.TargetId);

//            LunExtension = SpiGetLunExtension(DeviceExtension,
//                                              LunInfo->PathId,
//                                              LunInfo->TargetId,
//                                              LunInfo->Lun);

            /* Clear frozen flag */
//            LunExtension->Flags &= ~LUNEX_FROZEN_QUEUE;

            /* Acquire the spinlock */
//            KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);

            /* Process the request */
//            SpiGetNextRequestFromLun(DeviceObject->DeviceExtension, LunExtension);

            /* SpiGetNextRequestFromLun() releases the spinlock,
                so we just lower irql back to what it was before */
//            KeLowerIrql(Irql);
        }

        /* Check if data overrun happened */
        if (SRB_STATUS(Srb.SrbStatus) == SRB_STATUS_DATA_OVERRUN)
        {
            DPRINT("Data overrun at TargetId %d\n", PdoExtension->Target);

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
    }

    /* Free the sense buffer */
    ExFreePoolWithTag(SenseBuffer, TAG_SENSE_DATA);

    DPRINT1("PortSendInquiry() done with Status 0x%08X\n", Status);

    return Status;
}



static
NTSTATUS
PortFdoScanBus(
    _In_ PFDO_DEVICE_EXTENSION DeviceExtension)
{
    PPDO_DEVICE_EXTENSION PdoExtension;
    ULONG Bus, Target; //, Lun;
    NTSTATUS Status;
    ULONG LunCount = 0;

    DPRINT("PortFdoScanBus(%p)\n", DeviceExtension);

    DPRINT("NumberOfBuses: %lu\n", DeviceExtension->Miniport.PortConfig.NumberOfBuses);
    DPRINT("MaximumNumberOfTargets: %lu\n", DeviceExtension->Miniport.PortConfig.MaximumNumberOfTargets);
    DPRINT("MaximumNumberOfLogicalUnits: %lu\n", DeviceExtension->Miniport.PortConfig.MaximumNumberOfLogicalUnits);

    /* Scan all buses */
    for (Bus = 0; Bus < DeviceExtension->Miniport.PortConfig.NumberOfBuses; Bus++)
    {
        DPRINT("Scanning bus %ld\n", Bus);

        /* Scan all targets */
        for (Target = 0; Target < DeviceExtension->Miniport.PortConfig.MaximumNumberOfTargets; Target++)
        {
            DPRINT("  Scanning target %ld:%ld\n", Bus, Target);

            DPRINT("    Scanning logical unit %ld:%ld:%ld\n", Bus, Target, 0);
            Status = PortCreatePdo(DeviceExtension, Bus, Target, 0, &PdoExtension);
            if (NT_SUCCESS(Status))
            {
                /* Send Report LUNs first */
                PortSendReportLuns(PdoExtension, &LunCount);
            
                /* Scan LUN 0 */
                Status = PortSendInquiry(PdoExtension);
                DPRINT("PortSendInquiry returned 0x%08lx\n", Status);
                if (!NT_SUCCESS(Status))
                {
                    PortDeletePdo(PdoExtension);
                }
                else
                {
                    DPRINT("VendorId: %.8s\n", PdoExtension->InquiryBuffer->VendorId);
                    DPRINT("ProductId: %.16s\n", PdoExtension->InquiryBuffer->ProductId);
                    DPRINT("ProductRevisionLevel: %.4s\n", PdoExtension->InquiryBuffer->ProductRevisionLevel);
                    DPRINT("VendorSpecific: %.20s\n", PdoExtension->InquiryBuffer->VendorSpecific);
                }
            }

#if 0
            /* Scan all logical units */
            for (Lun = 1; Lun < DeviceExtension->Miniport.PortConfig.MaximumNumberOfLogicalUnits; Lun++)
            {
                DPRINT("    Scanning logical unit %ld:%ld:%ld\n", Bus, Target, Lun);
                Status = PortSendInquiry(DeviceExtension->Device, Bus, Target, Lun);
                DPRINT("PortSendInquiry returned 0x%08lx\n", Status);
                if (!NT_SUCCESS(Status))
                    break;
            }
#endif
        }
    }

    DPRINT("PortFdoScanBus() done!\n");

    return STATUS_SUCCESS;
}


static
NTSTATUS
PortFdoQueryBusRelations(
    _In_ PFDO_DEVICE_EXTENSION DeviceExtension,
    _Out_ PULONG_PTR Information)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PDEVICE_RELATIONS DeviceRelations = NULL;
    PPDO_DEVICE_EXTENSION PdoExtension;
    PLIST_ENTRY PdoEntry;
    ULONG PdoIndex = 0;

    DPRINT("PortFdoQueryBusRelations(%p %p)\n",
            DeviceExtension, Information);

    Status = PortFdoScanBus(DeviceExtension);

    DPRINT1("Units found: %lu\n", DeviceExtension->PdoCount);

    /* Following part referred to SCSIport */
    do
    {
        /* Allocate device relations object */
        DeviceRelations =
            ExAllocatePoolWithTag(PagedPool,
                                  (sizeof(DEVICE_RELATIONS) +
                                   sizeof(PDEVICE_OBJECT) * (DeviceExtension->PdoCount - 1)),
                                  TAG_DEVICE_RELATION);

        if (!DeviceRelations)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        /* Set PDO count and PDO pointers */
        DeviceRelations->Count = DeviceExtension->PdoCount;
        PdoEntry = DeviceExtension->PdoListHead.Flink;
        while (PdoEntry != &DeviceExtension->PdoListHead)
        {
            PdoExtension = CONTAINING_RECORD(PdoEntry,
                                             PDO_DEVICE_EXTENSION,
                                             PdoListEntry);

            DeviceRelations->Objects[PdoIndex] = PdoExtension->Device;

            /* Next one */
            ++PdoIndex;
            PdoEntry = PdoEntry->Flink;
        }
    } while (0);

    *Information = (ULONG_PTR)DeviceRelations;

    return Status;
}


static
NTSTATUS
PortFdoFilterRequirements(
    PFDO_DEVICE_EXTENSION DeviceExtension,
    PIRP Irp)
{
    PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList;

    DPRINT("PortFdoFilterRequirements(%p %p)\n", DeviceExtension, Irp);

    /* Get the bus number and the slot number */
    RequirementsList =(PIO_RESOURCE_REQUIREMENTS_LIST)Irp->IoStatus.Information;
    if (RequirementsList != NULL)
    {
        DeviceExtension->BusNumber = RequirementsList->BusNumber;
        DeviceExtension->SlotNumber = RequirementsList->SlotNumber;
    }

    return STATUS_SUCCESS;
}


PPDO_DEVICE_EXTENSION
FdoFindLun(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _In_ ULONG Bus,
    _In_ ULONG Target,
    _In_ ULONG Lun)
{
    PPDO_DEVICE_EXTENSION PdoExtension = NULL;
    KLOCK_QUEUE_HANDLE LockHandle;
    PLIST_ENTRY PdoListHead = &FdoExtension->PdoListHead, PdoEntry;

    KeAcquireInStackQueuedSpinLock(&FdoExtension->PdoListLock, &LockHandle);

    if (!IsListEmpty(&FdoExtension->PdoListHead))
    {
        PdoEntry = PdoListHead->Flink;

        do
        {
            PdoExtension = CONTAINING_RECORD(PdoEntry, PDO_DEVICE_EXTENSION, PdoListEntry);

            if (PdoExtension->Bus == Bus && PdoExtension->Target == Target &&
                PdoExtension->Lun == Lun)
            {
                break;
            }

            PdoEntry = PdoEntry->Flink;
        }
        while (PdoEntry != PdoListHead);
    }

    KeReleaseInStackQueuedSpinLock(&LockHandle);
    return PdoExtension;
}


NTSTATUS
NTAPI
FdoDeviceControlQueryProperty(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PFDO_DEVICE_EXTENSION FdoExtension;
    PMINIPORT Miniport;
    PSTORAGE_ADAPTER_DESCRIPTOR_WIN8 AdapterDescriptor;
    PSTORAGE_PROPERTY_QUERY Query;
    NTSTATUS Status;

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    FdoExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    Query = (PSTORAGE_PROPERTY_QUERY)Irp->AssociatedIrp.SystemBuffer;
    Miniport = &FdoExtension->Miniport;

    do
    {
        /* check property type (handle only StorageAdapterProperty) */
        if (Query->PropertyId != StorageAdapterProperty)
        {
            if (Query->PropertyId == StorageDeviceProperty ||
                Query->PropertyId == StorageDeviceIdProperty)
            {
                Status = STATUS_INVALID_DEVICE_REQUEST;
            }
            else
            {
                Status = STATUS_INVALID_PARAMETER_1;
            }

            break;
        }

        /* check query type */
        if (Query->QueryType == PropertyExistsQuery)
        {
            /* device property / adapter property is supported */
            Status = STATUS_SUCCESS;
            break;
        }

        if (Query->QueryType != PropertyStandardQuery)
        {
            /* only standard query and exists query are supported */
            Status = STATUS_INVALID_PARAMETER_2;
            break;
        }

        /* Check buffer length */
        if (IoStack->Parameters.DeviceIoControl.OutputBufferLength <
            sizeof(STORAGE_ADAPTER_DESCRIPTOR_WIN8))
        {
            PSTORAGE_DESCRIPTOR_HEADER DescriptorHeader = Irp->AssociatedIrp.SystemBuffer;

            /* Fail request if the buffer is simply too small */
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(STORAGE_DESCRIPTOR_HEADER))
            {
                Status = STATUS_BUFFER_TOO_SMALL; /* FIXME: Is this code appropriate? */
                break;
            }

            /* Return required size */
            DescriptorHeader->Version = sizeof(STORAGE_ADAPTER_DESCRIPTOR_WIN8);
            DescriptorHeader->Size = sizeof(STORAGE_ADAPTER_DESCRIPTOR_WIN8);
            Irp->IoStatus.Information = sizeof(STORAGE_DESCRIPTOR_HEADER);
            Status = STATUS_SUCCESS;
            break;
        }

        /* Return AdapterDescriptor */
        AdapterDescriptor = Irp->AssociatedIrp.SystemBuffer;
        *AdapterDescriptor = (STORAGE_ADAPTER_DESCRIPTOR_WIN8) {
            .Version = sizeof(STORAGE_ADAPTER_DESCRIPTOR_WIN8),
            .Size = sizeof(STORAGE_ADAPTER_DESCRIPTOR_WIN8),
            .MaximumTransferLength = Miniport->PortConfig.MaximumTransferLength,
            .MaximumPhysicalPages = Miniport->PortConfig.NumberOfPhysicalBreaks,
            .AlignmentMask = Miniport->PortConfig.AlignmentMask,
            .AdapterUsesPio = FALSE, /* Storport requirement */
            .AdapterScansDown = Miniport->PortConfig.AdapterScansDown,
            .CommandQueueing = TRUE, /* Storport requirement */
            .AcceleratedTransfer = TRUE,
            .BusType = BusTypeSata, /* FIXME: ＲＥＡＤ　ＦＲＯＭ　ＲＥＧＩＳＴＲＹ */
            .BusMajorVersion = 2,
            .BusMinorVersion = 0,
            // .SrbType = SRB_TYPE_SCSI_REQUEST_BLOCK /* This is actually important */
        };
        Irp->IoStatus.Information = sizeof(STORAGE_ADAPTER_DESCRIPTOR_WIN8);
        Status = STATUS_SUCCESS;
    }
    while (0);

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
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

    DPRINT("PortFdoScsi(%p %p)\n", DeviceObject, Irp);

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
            DPRINT("IRP_MJ_PNP / IRP_MN_START_DEVICE\n");
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
            switch (Stack->Parameters.QueryDeviceRelations.Type)
            {
                case BusRelations:
                    DPRINT("    IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / BusRelations\n");
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
            DPRINT("IRP_MJ_PNP / IRP_MN_FILTER_RESOURCE_REQUIREMENTS\n");
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

// IOCTL_SCSI_GET_ADDRESS
NTSTATUS
NTAPI
PortFdoDeviceControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    switch (IoStack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_STORAGE_QUERY_PROPERTY:
            return FdoDeviceControlQueryProperty(DeviceObject, Irp);
        
        default:
            // __debugbreak();
            Status = STATUS_NOT_IMPLEMENTED;
            break;
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

/* EOF */
