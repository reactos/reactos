/*
 * PROJECT:     ReactOS Storage Stack / SCSIPORT storage port library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Adapter device object (FDO) support routines
 * COPYRIGHT:   Eric Kohl (eric.kohl@reactos.org)
 *              Aleksey Bragin (aleksey@reactos.org)
 *              2020 Victor Perevertkin (victor.perevertkin@reactos.org)
 */

#include "scsiport.h"

#define NDEBUG
#include <debug.h>


static
NTSTATUS
FdoSendInquiry(
    _In_ PDEVICE_OBJECT DeviceObject)
{
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION IrpStack;
    KEVENT Event;
    KIRQL Irql;
    PIRP Irp;
    NTSTATUS Status;
    PINQUIRYDATA InquiryBuffer;
    PSENSE_DATA SenseBuffer;
    BOOLEAN KeepTrying = TRUE;
    ULONG RetryCount = 0;
    SCSI_REQUEST_BLOCK Srb;
    PCDB Cdb;

    DPRINT("FdoSendInquiry() called\n");

    PSCSI_PORT_LUN_EXTENSION LunExtension = DeviceObject->DeviceExtension;
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension =
        LunExtension->Common.LowerDevice->DeviceExtension;

    InquiryBuffer = ExAllocatePoolWithTag(NonPagedPool, INQUIRYDATABUFFERSIZE, TAG_SCSIPORT);
    if (InquiryBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    SenseBuffer = ExAllocatePoolWithTag(NonPagedPool, SENSE_BUFFER_SIZE, TAG_SCSIPORT);
    if (SenseBuffer == NULL)
    {
        ExFreePoolWithTag(InquiryBuffer, TAG_SCSIPORT);
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
                                            DeviceObject,
                                            NULL,
                                            0,
                                            InquiryBuffer,
                                            INQUIRYDATABUFFERSIZE,
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
        Srb.PathId = LunExtension->PathId;
        Srb.TargetId = LunExtension->TargetId;
        Srb.Lun = LunExtension->Lun;
        Srb.Function = SRB_FUNCTION_EXECUTE_SCSI;
        Srb.SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;
        Srb.TimeOutValue = 4;
        Srb.CdbLength = 6;

        Srb.SenseInfoBuffer = SenseBuffer;
        Srb.SenseInfoBufferLength = SENSE_BUFFER_SIZE;

        Srb.DataBuffer = InquiryBuffer;
        Srb.DataTransferLength = INQUIRYDATABUFFERSIZE;

        /* Attach Srb to the Irp */
        IrpStack = IoGetNextIrpStackLocation (Irp);
        IrpStack->Parameters.Scsi.Srb = &Srb;

        /* Fill in CDB */
        Cdb = (PCDB)Srb.Cdb;
        Cdb->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;
        Cdb->CDB6INQUIRY.LogicalUnitNumber = LunExtension->Lun;
        Cdb->CDB6INQUIRY.AllocationLength = INQUIRYDATABUFFERSIZE;

        /* Call the driver */
        Status = IoCallDriver(DeviceObject, Irp);

        /* Wait for it to complete */
        if (Status == STATUS_PENDING)
        {
            DPRINT("FdoSendInquiry(): Waiting for the driver to process request...\n");
            KeWaitForSingleObject(&Event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            Status = IoStatusBlock.Status;
        }

        DPRINT("FdoSendInquiry(): Request processed by driver, status = 0x%08X\n", Status);

        if (SRB_STATUS(Srb.SrbStatus) == SRB_STATUS_SUCCESS)
        {
            /* All fine, copy data over */
            RtlCopyMemory(&LunExtension->InquiryData,
                          InquiryBuffer,
                          INQUIRYDATABUFFERSIZE);

            /* Quit the loop */
            Status = STATUS_SUCCESS;
            KeepTrying = FALSE;
            continue;
        }

        DPRINT("Inquiry SRB failed with SrbStatus 0x%08X\n", Srb.SrbStatus);

        /* Check if the queue is frozen */
        if (Srb.SrbStatus & SRB_STATUS_QUEUE_FROZEN)
        {
            /* Something weird happened, deal with it (unfreeze the queue) */
            KeepTrying = FALSE;

            DPRINT("FdoSendInquiry(): the queue is frozen at TargetId %d\n", Srb.TargetId);

            /* Clear frozen flag */
            LunExtension->Flags &= ~LUNEX_FROZEN_QUEUE;

            /* Acquire the spinlock */
            KeAcquireSpinLock(&DeviceExtension->SpinLock, &Irql);

            /* Process the request. SpiGetNextRequestFromLun will unlock for us */
            SpiGetNextRequestFromLun(DeviceExtension, LunExtension, &Irql);
        }

        /* Check if data overrun happened */
        if (SRB_STATUS(Srb.SrbStatus) == SRB_STATUS_DATA_OVERRUN)
        {
            DPRINT("Data overrun at TargetId %d\n", LunExtension->TargetId);

            /* Nothing dramatic, just copy data, but limiting the size */
            RtlCopyMemory(&LunExtension->InquiryData,
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
    }

    /* Free buffers */
    ExFreePoolWithTag(InquiryBuffer, TAG_SCSIPORT);
    ExFreePoolWithTag(SenseBuffer, TAG_SCSIPORT);

    DPRINT("FdoSendInquiry() done with Status 0x%08X\n", Status);

    return Status;
}

/* Scans all SCSI buses */
VOID
FdoScanAdapter(
    _In_ PSCSI_PORT_DEVICE_EXTENSION PortExtension)
{
    NTSTATUS status;
    UINT32 totalLUNs = PortExtension->TotalLUCount;

    DPRINT("FdoScanAdapter() called\n");

    /* Scan all buses */
    for (UINT8 pathId = 0; pathId < PortExtension->NumberOfBuses; pathId++)
    {
        DPRINT("    Scanning bus/pathID %u\n", pathId);

        /* Get pointer to the scan information */
        PSCSI_BUS_INFO currentBus = &PortExtension->Buses[pathId];

        /* And send INQUIRY to every target */
        for (UINT8 targetId = 0;
             targetId < PortExtension->PortConfig->MaximumNumberOfTargets;
             targetId++)
        {
            BOOLEAN targetFound = FALSE;

            /* TODO: Support scan bottom-up */

            /* Skip if it's the same address */
            if (targetId == currentBus->BusIdentifier)
                continue;

            /* Scan all logical units */
            for (UINT8 lun = 0; lun < PortExtension->MaxLunCount; lun++)
            {
                PSCSI_PORT_LUN_EXTENSION lunExt;

                /* Skip invalid lun values */
                if (lun >= PortExtension->PortConfig->MaximumNumberOfLogicalUnits)
                    continue;

                // try to find an existing device
                lunExt = GetLunByPath(PortExtension,
                                      pathId,
                                      targetId,
                                      lun);

                if (lunExt)
                {
                    // check if the device still exists
                    status = FdoSendInquiry(lunExt->Common.DeviceObject);
                    if (!NT_SUCCESS(status))
                    {
                        // remove the device
                        UNIMPLEMENTED;
                        __debugbreak();
                    }

                    if (lunExt->InquiryData.DeviceTypeQualifier == DEVICE_QUALIFIER_NOT_SUPPORTED)
                    {
                        // remove the device
                        UNIMPLEMENTED;
                        __debugbreak();
                    }

                    /* Decide whether we are continuing or not */
                    if (status == STATUS_INVALID_DEVICE_REQUEST)
                        continue;
                    else
                        break;
                }

                // create a new LUN device
                PDEVICE_OBJECT lunPDO = PdoCreateLunDevice(PortExtension);
                if (!lunPDO)
                {
                    continue;
                }

                lunExt = lunPDO->DeviceExtension;

                lunExt->PathId = pathId;
                lunExt->TargetId = targetId;
                lunExt->Lun = lun;

                DPRINT("Add PDO to list: PDO: %p, FDOExt: %p, PDOExt: %p\n", lunPDO, PortExtension, lunExt);

                /* Set flag to prevent race conditions */
                lunExt->Flags |= SCSI_PORT_SCAN_IN_PROGRESS;

                /* Finally send the inquiry command */
                status = FdoSendInquiry(lunPDO);

                if (NT_SUCCESS(status))
                {
                    /* Let's see if we really found a device */
                    PINQUIRYDATA InquiryData = &lunExt->InquiryData;

                    /* Check if this device is unsupported */
                    if (InquiryData->DeviceTypeQualifier == DEVICE_QUALIFIER_NOT_SUPPORTED)
                    {
                        IoDeleteDevice(lunPDO);
                        continue;
                    }

                    /* Clear the "in scan" flag */
                    lunExt->Flags &= ~SCSI_PORT_SCAN_IN_PROGRESS;

                    DPRINT1("Found device of type %d at controller %d bus %d tid %d lun %d, PDO: %p\n",
                        InquiryData->DeviceType, PortExtension->PortNumber, pathId, targetId, lun, lunPDO);

                    InsertTailList(&currentBus->LunsListHead, &lunExt->LunEntry);

                    totalLUNs++;
                    currentBus->LogicalUnitsCount++;
                    targetFound = TRUE;
                }
                else
                {
                    /* Decide whether we are continuing or not */
                    if (status == STATUS_INVALID_DEVICE_REQUEST)
                        continue;
                    else
                        break;
                }
            }

            if (targetFound)
            {
                currentBus->TargetsCount++;
            }
        }
    }

    PortExtension->TotalLUCount = totalLUNs;
}

/**
 * @brief      Calls HwInitialize routine of the miniport and sets up interrupts
 *             Should be called inside ScsiPortInitialize (for legacy drivers)
 *             or inside IRP_MN_START_DEVICE for pnp drivers
 *
 * @param[in]  DeviceExtension  The device extension
 *
 * @return     NTSTATUS of the operation
 */
NTSTATUS
FdoCallHWInitialize(
    _In_ PSCSI_PORT_DEVICE_EXTENSION DeviceExtension)
{
    PPORT_CONFIGURATION_INFORMATION PortConfig = DeviceExtension->PortConfig;
    NTSTATUS Status;
    KIRQL OldIrql;

    /* Deal with interrupts */
    if (DeviceExtension->HwInterrupt == NULL ||
        (PortConfig->BusInterruptLevel == 0 && PortConfig->BusInterruptVector == 0))
    {
        /* No interrupts */
        DeviceExtension->InterruptCount = 0;

        DPRINT1("Interrupt Count: 0\n");

        UNIMPLEMENTED;

        /* This code path will ALWAYS crash so stop it now */
        __debugbreak();
    }
    else
    {
        BOOLEAN InterruptShareable;
        KINTERRUPT_MODE InterruptMode[2];
        ULONG InterruptVector[2], i, MappedIrq[2];
        KIRQL Dirql[2], MaxDirql;
        KAFFINITY Affinity[2];

        DeviceExtension->InterruptLevel[0] = PortConfig->BusInterruptLevel;
        DeviceExtension->InterruptLevel[1] = PortConfig->BusInterruptLevel2;

        InterruptVector[0] = PortConfig->BusInterruptVector;
        InterruptVector[1] = PortConfig->BusInterruptVector2;

        InterruptMode[0] = PortConfig->InterruptMode;
        InterruptMode[1] = PortConfig->InterruptMode2;

        DeviceExtension->InterruptCount =
            (PortConfig->BusInterruptLevel2 != 0 ||
             PortConfig->BusInterruptVector2 != 0) ? 2 : 1;

        for (i = 0; i < DeviceExtension->InterruptCount; i++)
        {
            /* Register an interrupt handler for this device */
            MappedIrq[i] = HalGetInterruptVector(
                PortConfig->AdapterInterfaceType, PortConfig->SystemIoBusNumber,
                DeviceExtension->InterruptLevel[i], InterruptVector[i], &Dirql[i],
                &Affinity[i]);
        }

        if (DeviceExtension->InterruptCount == 1 || Dirql[0] > Dirql[1])
        {
            MaxDirql = Dirql[0];
        }
        else
        {
            MaxDirql = Dirql[1];
        }

        for (i = 0; i < DeviceExtension->InterruptCount; i++)
        {
            /* Determine IRQ sharability as usual */
            if (PortConfig->AdapterInterfaceType == MicroChannel ||
                InterruptMode[i] == LevelSensitive)
            {
                InterruptShareable = TRUE;
            }
            else
            {
                InterruptShareable = FALSE;
            }

            Status = IoConnectInterrupt(&DeviceExtension->Interrupt[i],
                                        ScsiPortIsr,
                                        DeviceExtension,
                                        &DeviceExtension->IrqLock,
                                        MappedIrq[i], Dirql[i],
                                        MaxDirql,
                                        InterruptMode[i],
                                        InterruptShareable,
                                        Affinity[i],
                                        FALSE);

            if (!(NT_SUCCESS(Status)))
            {
                DPRINT1("Could not connect interrupt %d\n", InterruptVector[i]);
                DeviceExtension->Interrupt[i] = NULL;
                return Status;
            }
        }
    }

    /* Save IoAddress (from access ranges) */
    if (PortConfig->NumberOfAccessRanges != 0)
    {
        DeviceExtension->IoAddress = ((*(PortConfig->AccessRanges))[0]).RangeStart.LowPart;

        DPRINT("Io Address %x\n", DeviceExtension->IoAddress);
    }

    /* Set flag that it's allowed to disconnect during this command */
    DeviceExtension->Flags |= SCSI_PORT_DISCONNECT_ALLOWED;

    /* Initialize counter of active requests (-1 means there are none) */
    DeviceExtension->ActiveRequestCounter = -1;

    /* Analyze what we have about DMA */
    if (DeviceExtension->AdapterObject != NULL && PortConfig->Master &&
        PortConfig->NeedPhysicalAddresses)
    {
        DeviceExtension->MapRegisters = TRUE;
    }
    else
    {
        DeviceExtension->MapRegisters = FALSE;
    }

    /* Call HwInitialize at DISPATCH_LEVEL */
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    if (!KeSynchronizeExecution(
            DeviceExtension->Interrupt[0], DeviceExtension->HwInitialize,
            DeviceExtension->MiniPortDeviceExtension))
    {
        DPRINT1("HwInitialize() failed!\n");
        KeLowerIrql(OldIrql);
        return STATUS_ADAPTER_HARDWARE_ERROR;
    }

    /* Check if a notification is needed */
    if (DeviceExtension->InterruptData.Flags & SCSI_PORT_NOTIFICATION_NEEDED)
    {
        /* Call DPC right away, because we're already at DISPATCH_LEVEL */
        ScsiPortDpcForIsr(NULL, DeviceExtension->Common.DeviceObject, NULL, NULL);
    }

    /* Lower irql back to what it was */
    KeLowerIrql(OldIrql);

    return STATUS_SUCCESS;
}

NTSTATUS
FdoRemoveAdapter(
    _In_ PSCSI_PORT_DEVICE_EXTENSION DeviceExtension)
{
    IoStopTimer(DeviceExtension->Common.DeviceObject);

    // release device interface
    if (DeviceExtension->InterfaceName.Buffer)
    {
        IoSetDeviceInterfaceState(&DeviceExtension->InterfaceName, FALSE);

        RtlFreeUnicodeString(&DeviceExtension->InterfaceName);
        RtlInitUnicodeString(&DeviceExtension->InterfaceName, NULL);
    }

    // remove the dos device link
    WCHAR dosNameBuffer[12];
    UNICODE_STRING dosDeviceName;

    swprintf(dosNameBuffer, L"\\??\\Scsi%lu:", DeviceExtension->PortNumber);
    RtlInitUnicodeString(&dosDeviceName, dosNameBuffer);

    IoDeleteSymbolicLink(&dosDeviceName); // don't check the result

    // decrease the port count
    if (DeviceExtension->DeviceStarted)
    {
        PCONFIGURATION_INFORMATION sysConfig = IoGetConfigurationInformation();
        sysConfig->ScsiPortCount--;
    }

    // disconnect the interrupts
    while (DeviceExtension->InterruptCount)
    {
        if (DeviceExtension->Interrupt[--DeviceExtension->InterruptCount])
            IoDisconnectInterrupt(DeviceExtension->Interrupt[DeviceExtension->InterruptCount]);
    }

    // FIXME: delete LUNs
    if (DeviceExtension->Buses)
    {
        for (UINT8 pathId = 0; pathId < DeviceExtension->NumberOfBuses; pathId++)
        {
            PSCSI_BUS_INFO bus = &DeviceExtension->Buses[pathId];
            if (bus->RegistryMapKey)
            {
                ZwDeleteKey(bus->RegistryMapKey);
                ZwClose(bus->RegistryMapKey);
                bus->RegistryMapKey = NULL;
            }
        }

        ExFreePoolWithTag(DeviceExtension->Buses, TAG_SCSIPORT);
    }

    /* Free PortConfig */
    if (DeviceExtension->PortConfig)
    {
        ExFreePoolWithTag(DeviceExtension->PortConfig, TAG_SCSIPORT);
    }

    /* Free common buffer (if it exists) */
    if (DeviceExtension->SrbExtensionBuffer != NULL && DeviceExtension->CommonBufferLength != 0)
    {
        if (!DeviceExtension->AdapterObject)
        {
            ExFreePoolWithTag(DeviceExtension->SrbExtensionBuffer, TAG_SCSIPORT);
        }
        else
        {
            HalFreeCommonBuffer(DeviceExtension->AdapterObject,
                                DeviceExtension->CommonBufferLength,
                                DeviceExtension->PhysicalAddress,
                                DeviceExtension->SrbExtensionBuffer,
                                FALSE);
        }
    }

    /* Free SRB info */
    if (DeviceExtension->SrbInfo != NULL)
        ExFreePoolWithTag(DeviceExtension->SrbInfo, TAG_SCSIPORT);

    /* Unmap mapped addresses */
    while (DeviceExtension->MappedAddressList != NULL)
    {
        MmUnmapIoSpace(DeviceExtension->MappedAddressList->MappedAddress,
                       DeviceExtension->MappedAddressList->NumberOfBytes);

        PVOID ptr = DeviceExtension->MappedAddressList;
        DeviceExtension->MappedAddressList = DeviceExtension->MappedAddressList->NextMappedAddress;

        ExFreePoolWithTag(ptr, TAG_SCSIPORT);
    }

    IoDeleteDevice(DeviceExtension->Common.DeviceObject);

    return STATUS_SUCCESS;
}

NTSTATUS
FdoStartAdapter(
    _In_ PSCSI_PORT_DEVICE_EXTENSION PortExtension)
{
    WCHAR dosNameBuffer[12];
    UNICODE_STRING dosDeviceName;
    NTSTATUS status;

    // Start our timer
    IoStartTimer(PortExtension->Common.DeviceObject);

    // Create the dos device link
    swprintf(dosNameBuffer, L"\\??\\Scsi%u:", PortExtension->PortNumber);
    RtlInitUnicodeString(&dosDeviceName, dosNameBuffer);
    status = IoCreateSymbolicLink(&dosDeviceName, &PortExtension->DeviceName);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    // start building a device map
    RegistryInitAdapterKey(PortExtension);

    // increase the port count
    PCONFIGURATION_INFORMATION sysConfig = IoGetConfigurationInformation();
    sysConfig->ScsiPortCount++;

    // Register and enable the device interface
    status = IoRegisterDeviceInterface(PortExtension->Common.DeviceObject,
                                       &StoragePortClassGuid,
                                       NULL,
                                       &PortExtension->InterfaceName);
    DPRINT("IoRegisterDeviceInterface status: %x, InterfaceName: %wZ\n",
        status, &PortExtension->InterfaceName);

    if (NT_SUCCESS(status))
    {
        IoSetDeviceInterfaceState(&PortExtension->InterfaceName, TRUE);
    }

    PortExtension->DeviceStarted = TRUE;

    return STATUS_SUCCESS;
}

static
NTSTATUS
FdoHandleDeviceRelations(
    _In_ PSCSI_PORT_DEVICE_EXTENSION PortExtension,
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);

    // FDO always only handles bus relations
    if (ioStack->Parameters.QueryDeviceRelations.Type == BusRelations)
    {
        FdoScanAdapter(PortExtension);
        DPRINT("Found %u PD objects, FDOExt: %p\n", PortExtension->TotalLUCount, PortExtension);

        // check that no filter driver has messed up this
        ASSERT(Irp->IoStatus.Information == 0);

        PDEVICE_RELATIONS deviceRelations =
            ExAllocatePoolWithTag(PagedPool,
                                  (sizeof(DEVICE_RELATIONS) +
                                   sizeof(PDEVICE_OBJECT) * (PortExtension->TotalLUCount - 1)),
                                  TAG_SCSIPORT);

        if (!deviceRelations)
        {
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        deviceRelations->Count = 0;

        for (UINT8 pathId = 0; pathId < PortExtension->NumberOfBuses; pathId++)
        {
            PSCSI_BUS_INFO bus = &PortExtension->Buses[pathId];

            for (PLIST_ENTRY lunEntry = bus->LunsListHead.Flink;
                 lunEntry != &bus->LunsListHead;
                 lunEntry = lunEntry->Flink)
            {
                PSCSI_PORT_LUN_EXTENSION lunExt =
                    CONTAINING_RECORD(lunEntry, SCSI_PORT_LUN_EXTENSION, LunEntry);

                deviceRelations->Objects[deviceRelations->Count++] = lunExt->Common.DeviceObject;
                ObReferenceObject(lunExt->Common.DeviceObject);
            }
        }

        ASSERT(deviceRelations->Count == PortExtension->TotalLUCount);

        Irp->IoStatus.Information = (ULONG_PTR)deviceRelations;
        Irp->IoStatus.Status = STATUS_SUCCESS;
    }

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(PortExtension->Common.LowerDevice, Irp);
}

static
NTSTATUS
FdoHandleQueryCompatibleId(
    _Inout_ PZZWSTR* PwIds)
{
    static WCHAR GenScsiAdapterId[] = L"GEN_SCSIADAPTER";
    PWCHAR Ids = *PwIds, NewIds;
    ULONG Length = 0;

    if (Ids)
    {
        /* Calculate the length of existing MULTI_SZ value line by line */
        while (*Ids)
        {
            Ids += wcslen(Ids) + 1;
        }
        Length = Ids - *PwIds;
        Ids = *PwIds;
    }

    /* New MULTI_SZ with added identifier and finalizing zeros */
    NewIds = ExAllocatePoolZero(PagedPool,
                                Length * sizeof(WCHAR) + sizeof(GenScsiAdapterId) + sizeof(UNICODE_NULL),
                                TAG_SCSIPORT);
    if (!NewIds)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (Length)
    {
        RtlCopyMemory(NewIds, Ids, Length * sizeof(WCHAR));
    }
    RtlCopyMemory(&NewIds[Length], GenScsiAdapterId, sizeof(GenScsiAdapterId));

    /* Finally replace identifiers */
    if (Ids)
    {
        ExFreePool(Ids);
    }
    *PwIds = NewIds;

    return STATUS_SUCCESS;
}

NTSTATUS
FdoDispatchPnp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_PORT_DEVICE_EXTENSION portExt = DeviceObject->DeviceExtension;
    NTSTATUS status;

    ASSERT(portExt->Common.IsFDO);

    DPRINT("FDO PnP request %s\n", GetIRPMinorFunctionString(ioStack->MinorFunction));

    switch (ioStack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
        {
            // as we don't support PnP yet, this is a no-op for us
            // (FdoStartAdapter is being called during initialization for legacy miniports)
            status = STATUS_SUCCESS;
            // status = FdoStartAdapter(DeviceExtension);
            break;
        }
        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            return FdoHandleDeviceRelations(portExt, Irp);
        }
        case IRP_MN_QUERY_ID:
        {
            if (ioStack->Parameters.QueryId.IdType == BusQueryCompatibleIDs)
            {
                Irp->IoStatus.Information = 0;
                IoForwardIrpSynchronously(portExt->Common.LowerDevice, Irp);
                status = FdoHandleQueryCompatibleId((PZZWSTR*)&Irp->IoStatus.Information);
                break;
            }
            // otherwise fall through the default case
        }
        default:
        {
            // forward irp to next device object
            IoCopyCurrentIrpStackLocationToNext(Irp);
            return IoCallDriver(portExt->Common.LowerDevice, Irp);
        }
    }

    if (status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return status;
}
