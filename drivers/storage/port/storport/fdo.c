/*
 * PROJECT:     ReactOS Storport Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Storport FDO code
 * COPYRIGHT:   Copyright 2017 Eric Kohl (eric.kohl@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include "precomp.h"
#include "inline.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ******************************************************************/

/* Starved request queue, a cancel safe queue over STARVED_REQUEST_QUEUE */

static
VOID
NTAPI
PortStarvedCsqInsertIrp(
    _In_ PIO_CSQ Csq,
    _In_ PIRP Irp)
{
    PSTARVED_REQUEST_QUEUE Queue = CONTAINING_RECORD(Csq, STARVED_REQUEST_QUEUE, Csq);

    InsertTailList(&Queue->ListHead, &Irp->Tail.Overlay.ListEntry);
}

static
VOID
NTAPI
PortStarvedCsqRemoveIrp(
    _In_ PIO_CSQ Csq,
    _In_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(Csq);

    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
}

static
PIRP
NTAPI
PortStarvedCsqPeekNextIrp(
    _In_ PIO_CSQ Csq,
    _In_ PIRP Irp,
    _In_ PVOID PeekContext)
{
    PSTARVED_REQUEST_QUEUE Queue = CONTAINING_RECORD(Csq, STARVED_REQUEST_QUEUE, Csq);
    PLIST_ENTRY Entry;

    UNREFERENCED_PARAMETER(PeekContext);

    Entry = (Irp == NULL) ? Queue->ListHead.Flink : Irp->Tail.Overlay.ListEntry.Flink;

    if (Entry == &Queue->ListHead)
    {
        return NULL;
    }

    return CONTAINING_RECORD(Entry, IRP, Tail.Overlay.ListEntry);
}

static
VOID
NTAPI
PortStarvedCsqAcquireLock(
    _In_ PIO_CSQ Csq,
    _Out_ PKIRQL Irql)
{
    PSTARVED_REQUEST_QUEUE Queue = CONTAINING_RECORD(Csq, STARVED_REQUEST_QUEUE, Csq);

    KeAcquireSpinLock(&Queue->Lock, Irql);
}

static
VOID
NTAPI
PortStarvedCsqReleaseLock(
    _In_ PIO_CSQ Csq,
    _In_ KIRQL Irql)
{
    PSTARVED_REQUEST_QUEUE Queue = CONTAINING_RECORD(Csq, STARVED_REQUEST_QUEUE, Csq);

    KeReleaseSpinLock(&Queue->Lock, Irql);
}

static
VOID
NTAPI
PortStarvedCsqCompleteCanceledIrp(
    _In_ PIO_CSQ Csq,
    _In_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(Csq);

    StorpCompleteRequest(Irp, SRB_STATUS_ABORTED, STATUS_CANCELLED);
}

NTSTATUS
PortInitializeStarvedRequestQueue(
    _In_ PSTARVED_REQUEST_QUEUE Queue)
{
    KeInitializeSpinLock(&Queue->Lock);
    InitializeListHead(&Queue->ListHead);

    return IoCsqInitialize(&Queue->Csq,
                           PortStarvedCsqInsertIrp,
                           PortStarvedCsqRemoveIrp,
                           PortStarvedCsqPeekNextIrp,
                           PortStarvedCsqAcquireLock,
                           PortStarvedCsqReleaseLock,
                           PortStarvedCsqCompleteCanceledIrp);
}

/**
 * @brief Queues a request that could not allocate its private contexts.
 */
VOID
PortQueueStarvedRequest(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _In_ PIRP Irp)
{
    IoCsqInsertIrp(&FdoExtension->StarvedRequests.Csq, Irp, NULL);
}

/**
 * @brief Retries one queued request, so each completion lets exactly one through.
 */
VOID
PortRetryStarvedRequest(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension)
{
    PIRP Irp;

    Irp = IoCsqRemoveNextIrp(&FdoExtension->StarvedRequests.Csq, NULL);
    if (Irp == NULL)
    {
        return;
    }

    PortPdoScsi(IoGetCurrentIrpStackLocation(Irp)->DeviceObject, Irp);
}

/**
 * @brief Fails every queued request, for when the adapter goes away.
 */
VOID
PortFlushStarvedRequests(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension)
{
    PIRP Irp;

    while ((Irp = IoCsqRemoveNextIrp(&FdoExtension->StarvedRequests.Csq, NULL)) != NULL)
    {
        StorpCompleteRequest(Irp, SRB_STATUS_NO_DEVICE, STATUS_NO_SUCH_DEVICE);
    }
}

/**
 * @brief Reserves the memory a single request needs, so IO can still be issued out of pool.
 */
VOID
PortAllocateRequestReserve(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension)
{
    ULONG SrbExtensionSize = FdoExtension->HwInitData->SrbExtensionSize;

    FdoExtension->ReservedRequestReference =
        ExAllocatePoolWithTag(NonPagedPool,
                              REQUEST_REFERENCE_ALLOCATION_SIZE,
                              TAG_QUEUED_REQUEST);
    if (FdoExtension->ReservedRequestReference == NULL)
    {
        DPRINT1("Failed to reserve a request packet\n");
        return;
    }

    if (SrbExtensionSize == 0)
    {
        return;
    }

    FdoExtension->ReservedSrbExtension = ExAllocatePoolWithTag(NonPagedPool,
                                                               SrbExtensionSize,
                                                               TAG_SRB_EXTENSION);
    if (FdoExtension->ReservedSrbExtension == NULL)
    {
        DPRINT1("Failed to reserve an SRB extension\n");

        /* A packet alone is useless if the miniport wants an SRB extension */
        ExFreePoolWithTag(FdoExtension->ReservedRequestReference, TAG_QUEUED_REQUEST);
        FdoExtension->ReservedRequestReference = NULL;
    }
}

/**
 * @brief Releases the reserve. No request may be using it anymore.
 */
VOID
PortFreeRequestReserve(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension)
{
    if (FdoExtension->ReservedSrbExtension != NULL)
    {
        ExFreePoolWithTag(FdoExtension->ReservedSrbExtension, TAG_SRB_EXTENSION);
        FdoExtension->ReservedSrbExtension = NULL;
    }

    if (FdoExtension->ReservedRequestReference != NULL)
    {
        ExFreePoolWithTag(FdoExtension->ReservedRequestReference, TAG_QUEUED_REQUEST);
        FdoExtension->ReservedRequestReference = NULL;
    }
}

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
BOOLEAN
NTAPI
PortFdoMessageInterruptRoutine(
    _In_ PKINTERRUPT Interrupt,
    _In_ PVOID ServiceContext,
    _In_ ULONG MessageId)
{
    PFDO_DEVICE_EXTENSION DeviceExtension;

    UNREFERENCED_PARAMETER(Interrupt);

    DeviceExtension = (PFDO_DEVICE_EXTENSION)ServiceContext;

    return MiniportHwMSInterrupt(&DeviceExtension->Miniport, MessageId);
}


/**
 * @brief Connects the adapter on message signaled interrupts, falling back to a line interrupt.
 */
static
NTSTATUS
NTAPI
PortFdoConnectMessageInterrupt(
    _In_ PFDO_DEVICE_EXTENSION DeviceExtension)
{
    IO_CONNECT_INTERRUPT_PARAMETERS Parameters;
    NTSTATUS Status;

    RtlZeroMemory(&Parameters, sizeof(Parameters));

    Parameters.Version = CONNECT_MESSAGE_BASED;
    Parameters.MessageBased.PhysicalDeviceObject = DeviceExtension->PhysicalDevice;
    Parameters.MessageBased.ConnectionContext.InterruptMessageTable = &DeviceExtension->MessageInfo;
    Parameters.MessageBased.MessageServiceRoutine = PortFdoMessageInterruptRoutine;
    Parameters.MessageBased.ServiceContext = DeviceExtension;
    Parameters.MessageBased.SpinLock = NULL;
    Parameters.MessageBased.SynchronizeIrql = 0;
    Parameters.MessageBased.FloatingSave = FALSE;
    Parameters.MessageBased.FallBackServiceRoutine = PortFdoInterruptRoutine;

    Status = IoConnectInterruptEx(&Parameters);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoConnectInterruptEx() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    if (Parameters.Version == CONNECT_LINE_BASED)
    {
        /* No MSI, the line interrupt object came back in the same union */
        DeviceExtension->Interrupt = (PKINTERRUPT)DeviceExtension->MessageInfo;
        DeviceExtension->MessageInfo = NULL;

        DPRINT1("Adapter fell back to a line interrupt\n");
        return STATUS_SUCCESS;
    }

    DeviceExtension->MessageInterrupts = TRUE;
    DeviceExtension->InterruptIrql = DeviceExtension->MessageInfo->UnifiedIrql;

    /* The synchronization helpers work on a single interrupt object */
    if (DeviceExtension->MessageInfo->MessageCount > 0)
        DeviceExtension->Interrupt = DeviceExtension->MessageInfo->MessageInfo[0].InterruptObject;

    DPRINT1("Connected %lu message interrupts at IRQL %lu\n",
            DeviceExtension->MessageInfo->MessageCount,
            DeviceExtension->InterruptIrql);

    return STATUS_SUCCESS;
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

    /* A miniport that can take messages gets them in preference to a line */
    if (DeviceExtension->Miniport.PortConfig.HwMSInterruptRoutine != NULL)
        return PortFdoConnectMessageInterrupt(DeviceExtension);

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
VOID
NTAPI
PortFdoDisconnectInterrupt(
    _In_ PFDO_DEVICE_EXTENSION DeviceExtension)
{
    IO_DISCONNECT_INTERRUPT_PARAMETERS Parameters;

    DPRINT("PortFdoDisconnectInterrupt(%p)\n", DeviceExtension);

    if (DeviceExtension->MessageInterrupts)
    {
        Parameters.Version = CONNECT_MESSAGE_BASED;
        Parameters.ConnectionContext.InterruptMessageTable = DeviceExtension->MessageInfo;
        IoDisconnectInterruptEx(&Parameters);

        DeviceExtension->MessageInterrupts = FALSE;
        DeviceExtension->MessageInfo = NULL;
    }
    else if (DeviceExtension->Interrupt != NULL)
    {
        IoDisconnectInterrupt(DeviceExtension->Interrupt);
    }

    DeviceExtension->Interrupt = NULL;
    DeviceExtension->InterruptIrql = 0;
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

    /* Query the physical device, a filter may sit between us and it */
    InterfaceType = GetBusInterface(DeviceExtension->PhysicalDevice);
    if (InterfaceType == InterfaceTypeUndefined)
    {
        DPRINT1("The bus this adapter sits on is unknown\n");
        return STATUS_NO_SUCH_DEVICE;
    }

    /* Get the driver init data for the given interface type */
    InitData = PortGetDriverInitData(DeviceExtension->DriverExtension,
                                     InterfaceType);
    if (InitData == NULL)
    {
        DPRINT1("No init data registered for bus type %lu\n", InterfaceType);
        return STATUS_NO_SUCH_DEVICE;
    }

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

    /* Find out which adapter control requests the miniport takes */
    MiniportQueryAdapterControl(&DeviceExtension->Miniport);
    MiniportQueryUnitControl(&DeviceExtension->Miniport);

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
    else
    {
        /* SrbExtensionSize is only known once the miniport is up */
        PortAllocateRequestReserve(DeviceExtension);
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
    _Out_ PULONG LunCount,
    _Out_writes_(LunListSize) PUCHAR LunList,
    _In_ ULONG LunListSize)
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
        Cdb->REPORT_LUNS.AllocationLength[0] = (BufferSize >> (3*8)) & 0xFF;
        Cdb->REPORT_LUNS.AllocationLength[1] = (BufferSize >> (2*8)) & 0xFF;
        Cdb->REPORT_LUNS.AllocationLength[2] = (BufferSize >> (1*8)) & 0xFF;
        Cdb->REPORT_LUNS.AllocationLength[3] = (BufferSize >> (0*8)) & 0xFF;

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
            ULONG Reported;
            ULONG Index;

            /* The reported length covers the descriptors alone */
            DataSize |= (((ULONG)ReportLunsData->LunListLength[0]) << (8*3));
            DataSize |= (((ULONG)ReportLunsData->LunListLength[1]) << (8*2));
            DataSize |= (((ULONG)ReportLunsData->LunListLength[2]) << (8*1));
            DataSize |= (((ULONG)ReportLunsData->LunListLength[3]) << (8*0));
            Reported = DataSize / sizeof(*ReportLunsData->LunDescriptor);

            /* A device with more units than the first guess held is asked again */
            if ((DataSize > BufferSize - FIELD_OFFSET(REPORT_LUNS_DATA, LunDescriptor)) &&
                !BufferReallocated)
            {
                ExFreePoolWithTag(ReportLunsData, TAG_REPORT_LUN_DATA);

                BufferSize = FIELD_OFFSET(REPORT_LUNS_DATA, LunDescriptor) + DataSize;
                ReportLunsData = ExAllocatePoolWithTag(NonPagedPool, BufferSize, TAG_REPORT_LUN_DATA);
                if (ReportLunsData == NULL)
                {
                    DPRINT1("Cannot reallocate REPORT_LUNS buffer\n");
                    ExFreePoolWithTag(SenseBuffer, TAG_SENSE_DATA);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                BufferReallocated = TRUE;
                continue;
            }

            /* Only what actually arrived can be read back out */
            Reported = min(Reported,
                           (BufferSize - FIELD_OFFSET(REPORT_LUNS_DATA, LunDescriptor)) /
                               sizeof(*ReportLunsData->LunDescriptor));

            *LunCount = 0;
            for (Index = 0; Index < Reported; Index++)
            {
                PLUN_DESCRIPTOR Descriptor = &ReportLunsData->LunDescriptor[Index];

                /* Only units named the flat way fit in a single byte */
                if (Descriptor->AddressMethod != 0)
                    continue;

                if (*LunCount >= LunListSize)
                    break;

                LunList[(*LunCount)++] = Descriptor->Level1Address;
            }

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
#if 0
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
#endif
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
            BufferSize |= (((ULONG)ReportLunsData->LunListLength[0]) << (8*3));
            BufferSize |= (((ULONG)ReportLunsData->LunListLength[1]) << (8*2));
            BufferSize |= (((ULONG)ReportLunsData->LunListLength[2]) << (8*1));
            BufferSize |= (((ULONG)ReportLunsData->LunListLength[3]) << (8*0));
            BufferSize += FIELD_OFFSET(REPORT_LUNS_DATA, LunDescriptor);

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
    ULONG Bus, Target, Index;
    NTSTATUS Status;
    ULONG LunCount = 0;

    /* A unit named by one byte is all the flat addressing method carries */
    UCHAR LunList[256];

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
            if (!NT_SUCCESS(Status))
                continue;

            /* Ask the target which units it has before probing any of them */
            LunCount = 0;
            PortSendReportLuns(PdoExtension, &LunCount, LunList, RTL_NUMBER_OF(LunList));

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

            /* Scan the rest of the units the target reported */
            for (Index = 0; Index < LunCount; Index++)
            {
                ULONG Lun = LunList[Index];

                /* Unit zero was done above, and the report may not name it */
                if (Lun == 0)
                    continue;

                DPRINT("    Scanning logical unit %ld:%ld:%ld\n", Bus, Target, Lun);
                Status = PortCreatePdo(DeviceExtension, Bus, Target, Lun, &PdoExtension);
                if (!NT_SUCCESS(Status))
                    continue;

                Status = PortSendInquiry(PdoExtension);
                DPRINT("PortSendInquiry returned 0x%08lx\n", Status);
                if (!NT_SUCCESS(Status))
                    PortDeletePdo(PdoExtension);
            }
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
    KLOCK_QUEUE_HANDLE LockHandle;
    PLIST_ENTRY PdoEntry;
    ULONG PdoCount, PdoIndex = 0;

    DPRINT("PortFdoQueryBusRelations(%p %p)\n",
            DeviceExtension, Information);

    Status = PortFdoScanBus(DeviceExtension);

    PdoCount = DeviceExtension->PdoCount;

    DPRINT1("Units found: %lu\n", PdoCount);

    /* Following part referred to SCSIport */
    DeviceRelations = ExAllocatePoolWithTag(PagedPool,
                                            FIELD_OFFSET(DEVICE_RELATIONS, Objects[PdoCount]),
                                            TAG_DEVICE_RELATION);
    if (!DeviceRelations)
    {
        *Information = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Set PDO pointers */
    KeAcquireInStackQueuedSpinLock(&DeviceExtension->PdoListLock, &LockHandle);

    for (PdoEntry = DeviceExtension->PdoListHead.Flink;
         PdoEntry != &DeviceExtension->PdoListHead && PdoIndex < PdoCount;
         PdoEntry = PdoEntry->Flink)
    {
        PdoExtension = CONTAINING_RECORD(PdoEntry,
                                         PDO_DEVICE_EXTENSION,
                                         PdoListEntry);

        if (PdoExtension->PnpState == dsRemoved ||
            PdoExtension->PnpState == dsSurpriseRemoved)
        {
            continue;
        }

        ObReferenceObject(PdoExtension->Device);

        DeviceRelations->Objects[PdoIndex] = PdoExtension->Device;

        /* Next one */
        ++PdoIndex;
    }

    KeReleaseInStackQueuedSpinLock(&LockHandle);

    DeviceRelations->Count = PdoIndex;

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
    PPDO_DEVICE_EXTENSION PdoExtension;
    KLOCK_QUEUE_HANDLE LockHandle;
    PLIST_ENTRY PdoEntry;

    KeAcquireInStackQueuedSpinLock(&FdoExtension->PdoListLock, &LockHandle);

    for (PdoEntry = FdoExtension->PdoListHead.Flink;
         PdoEntry != &FdoExtension->PdoListHead;
         PdoEntry = PdoEntry->Flink)
    {
        PdoExtension = CONTAINING_RECORD(PdoEntry, PDO_DEVICE_EXTENSION, PdoListEntry);

        if (PdoExtension->Bus == Bus && PdoExtension->Target == Target &&
            PdoExtension->Lun == Lun)
        {
            KeReleaseInStackQueuedSpinLock(&LockHandle);
            return PdoExtension;
        }
    }

    KeReleaseInStackQueuedSpinLock(&LockHandle);
    return NULL;
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
            MiniportAdapterControl(&DeviceExtension->Miniport, ScsiStopAdapter, NULL);
            PortFdoDisconnectInterrupt(DeviceExtension);
            PortFlushStarvedRequests(DeviceExtension);
            PortFreeRequestReserve(DeviceExtension);
            ExDeleteNPagedLookasideList(&DeviceExtension->RequestReferenceLookaside);
            DeviceExtension->PnpState = dsRemoved;
            break;

        case IRP_MN_CANCEL_REMOVE_DEVICE: /* 0x03 */
            DPRINT1("IRP_MJ_PNP / IRP_MN_CANCEL_REMOVE_DEVICE\n");
            break;

        case IRP_MN_STOP_DEVICE: /* 0x04 */
            DPRINT1("IRP_MJ_PNP / IRP_MN_STOP_DEVICE\n");
            MiniportAdapterControl(&DeviceExtension->Miniport, ScsiStopAdapter, NULL);
            PortFdoDisconnectInterrupt(DeviceExtension);
            DeviceExtension->PnpState = dsStopped;
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

/* EOF */
