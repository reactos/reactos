/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/fdo.c
 * PURPOSE:     USB EHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 */

#include "hwiface.h"
#include "usbehci.h"
#include "physmem.h"
#include <stdio.h>

VOID NTAPI
EhciDefferedRoutine(PKDPC Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    PFDO_DEVICE_EXTENSION FdoDeviceExtension;
    PPDO_DEVICE_EXTENSION PdoDeviceExtension;
    ULONG CStatus;
    ULONG tmp;
    ULONG OpRegisters;
    PEHCI_HOST_CONTROLLER hcd;
    int i;

    FdoDeviceExtension = (PFDO_DEVICE_EXTENSION) DeferredContext;

    /* Nothing is valid if the Pdo is NULL */
    if (!FdoDeviceExtension->Pdo)
    {
        DPRINT1("PDO not set yet!\n");
        return;
    }

    PdoDeviceExtension = (PPDO_DEVICE_EXTENSION) FdoDeviceExtension->Pdo->DeviceExtension;

    OpRegisters = (ULONG)FdoDeviceExtension->hcd.OpRegisters;

    hcd = &FdoDeviceExtension->hcd;

    CStatus = (ULONG) SystemArgument2;

    /* If Reclamation (The QueueHead list has been transveresed twice),
       look through the queuehead list and find queue heads that have been 
       1. Halted due to error
       2. Transfer completion.
       Move these QueueHeads to a temporary list that is used to pend memory release.
       Either an Event is signalled or an Irp is completed depending on what was set during transfer request
       setup. Next software issue a "DoorBell" that informs the controller the Asynchronous List is about to 
       be modified.
       After the controller acks this with interrupt, the memory for queueheads are released. */

    if (CStatus & (EHCI_STS_RECL| EHCI_STS_INT | EHCI_ERROR_INT))
    {
        PQUEUE_HEAD CurrentQH;
        PQUEUE_TRANSFER_DESCRIPTOR CurrentTD;
        BOOLEAN QueueHeadCompleted;

        /* Go through the list and delink completed (not active) QueueHeads */
        CurrentQH = hcd->AsyncListQueue;
        CurrentQH = CurrentQH->NextQueueHead;

        while ((CurrentQH) && (CurrentQH != hcd->AsyncListQueue))
        {
            DPRINT1("Checking QueueHead %x, Next %x\n", CurrentQH, CurrentQH->NextQueueHead);
            DPRINT1("Active %d, Halted %d\n", CurrentQH->Token.Bits.Active, CurrentQH->Token.Bits.Halted);

            /* if the QueueHead has completed */
            if (!CurrentQH->Token.Bits.Active)
            {
                /* Assume success */
                USBD_STATUS UrbStatus = USBD_STATUS_SUCCESS;

                QueueHeadCompleted = TRUE;

                /* Check the Status of the QueueHead */
                if (CurrentQH->Token.Bits.Halted)
                {
                    if (CurrentQH->Token.Bits.DataBufferError)
                    {
                        DPRINT1("Data buffer error\n");
                        UrbStatus = USBD_STATUS_DATA_BUFFER_ERROR;
                    }
                    else if (CurrentQH->Token.Bits.BabbleDetected)
                    {
                        DPRINT1("Babble Detected\n");
                        UrbStatus = USBD_STATUS_BABBLE_DETECTED;
                    }
                    else
                    {
                        DPRINT1("Stall PID\n");
                        UrbStatus = USBD_STATUS_STALL_PID;
                    }
                }
                
                /* Check the Descriptors */
                CurrentTD = CurrentQH->FirstTransferDescriptor;
                while (CurrentTD)
                {
                    /* FIXME: What needs to happen if the QueueHead was marked as complete but descriptors was not */
                    if ((CurrentTD->Token.Bits.Active) || (CurrentTD->Token.Bits.Halted))
                    {
                        /* The descriptor was not completed */
                        QueueHeadCompleted = FALSE;
                        DPRINT1("QueueHead was marked as completed but contains descriptors that were not completed\n");
                        ASSERT(FALSE);
                        break;
                    }
                    CurrentTD = CurrentTD->NextDescriptor;
                }

                if ((QueueHeadCompleted) || (CurrentQH->Token.Bits.Halted))
                {
                    PQUEUE_HEAD FreeQH;
                    
                    FreeQH = CurrentQH;
                    CurrentQH = CurrentQH->NextQueueHead;
                    DPRINT1("QueueHead %x has completed. Removing\n", FreeQH);
                    /* Move it into the completed list */
                    UnlinkQueueHead(hcd, FreeQH);
                    LinkQueueHeadToCompletedList(hcd, FreeQH);
                    DPRINT1("Remove done\n");

                    /* If the Event is set then the caller is waiting on completion */
                    if (FreeQH->Event)
                    {
                        KeSetEvent(FreeQH->Event, IO_NO_INCREMENT, FALSE);
                    }

                    /* If there is an IrpToComplete then the caller did not wait on completion
                       and the IRP was marked as PENDING. Complete it now. */
                    if (FreeQH->IrpToComplete)
                    {
                        PIRP Irp;
                        PIO_STACK_LOCATION Stack;
                        PURB Urb;

                        Irp = FreeQH->IrpToComplete;
                        Stack = IoGetCurrentIrpStackLocation(Irp);
                        ASSERT(Stack);
                        Urb = (PURB) Stack->Parameters.Others.Argument1;
                        ASSERT(Urb);

                        /* Check for error */
                        if (CStatus & EHCI_ERROR_INT)
                        {
                            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
                            Irp->IoStatus.Information = 0;
                            /* Set BufferLength to 0 as there was error */
                            if (Urb->UrbHeader.Function == URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE)
                            {
                                Urb->UrbControlDescriptorRequest.TransferBufferLength = 0;
                            }
                            DPRINT1("There was an Error, TransferBufferLength set to 0\n");
                        }
                        else
                        {
                            if (Urb->UrbHeader.Function == URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE)
                            {
                                if (Urb->UrbControlDescriptorRequest.TransferBufferLength >=
                                    ((PUSB_COMMON_DESCRIPTOR)(Urb->UrbControlDescriptorRequest.TransferBuffer))->bLength)
                                {
                                    Urb->UrbControlDescriptorRequest.TransferBufferLength =
                                        ((PUSB_COMMON_DESCRIPTOR)(Urb->UrbControlDescriptorRequest.TransferBuffer))->bLength;
                                }
                            }

                            Irp->IoStatus.Status = STATUS_SUCCESS;
                            Irp->IoStatus.Information = 0;
                            DPRINT1("Completing Irp\n");
                        }
                        Urb->UrbHeader.Status = UrbStatus;
                        IoCompleteRequest(Irp,  IO_NO_INCREMENT);
                    }

                    /* FIXME: Move to static function */
                    PEHCI_USBCMD_CONTENT UsbCmd;
                    /* Ring the DoorBell so that host controller knows a QueueHead was removed */
                    DPRINT1("Ringing Doorbell\n");
                    tmp = READ_REGISTER_ULONG((PULONG) (OpRegisters + EHCI_USBCMD));
                    UsbCmd = (PEHCI_USBCMD_CONTENT) &tmp;
                    UsbCmd->DoorBell = TRUE;
                    WRITE_REGISTER_ULONG((PULONG) (OpRegisters +  EHCI_USBCMD), tmp);
                    continue;
                }
            }

            CurrentQH = CurrentQH->NextQueueHead;
        }
    }

    
    /* Port Change. */
    /* FIXME: Use EnumControllerPorts instead */
    if (CStatus & EHCI_STS_PCD)
    {
        /* Loop through the ports */
        for (i = 0; i < hcd->ECHICaps.HCSParams.PortCount; i++)
        {
            tmp = READ_REGISTER_ULONG((PULONG) ((OpRegisters + EHCI_PORTSC) + (4 * i)));

            /* Check for port change on this port */
            if (tmp & 0x02)
            {
                /* Clear status change */
                tmp = READ_REGISTER_ULONG((PULONG)((OpRegisters + EHCI_PORTSC) + (4 * i)));
                tmp |= 0x02;
                WRITE_REGISTER_ULONG((PULONG) ((OpRegisters + EHCI_PORTSC) + (4 * i)), tmp);
                
                /* Connect or Disconnect? */
                if (tmp & 0x01)
                {
                    DPRINT1("Device connected on port %d\n", i);

                    /* Check if a companion host controller exists */
                    if (hcd->ECHICaps.HCSParams.CHCCount)
                    {
                        tmp = READ_REGISTER_ULONG((PULONG)((OpRegisters + EHCI_PORTSC) + (4 * i)));

                        /* Port should be in disabled state, as per USB 2.0 specs */
                        if (tmp & 0x04)
                        {
                            DPRINT1("Warning: The port the device has just connected to is not disabled!\n");
                        }

                        /* Is this non high speed device */
                        if (tmp & 0x400)
                        {
                            DPRINT1("Non HighSpeed device connected. Releasing ownership.\n");
                            /* Release ownership to companion host controller */
                            WRITE_REGISTER_ULONG((PULONG) ((OpRegisters + EHCI_PORTSC) + (4 * i)), 0x2000);
                            continue;
                        }
                    }

                    KeStallExecutionProcessor(30);

                    /* FIXME: Hub driver does this also, is it needed here? */
                    /* As per USB 2.0 Specs, 9.1.2. Reset the port and clear the status change */
                    //tmp |= 0x100 | 0x02;
                    /* Sanity, Disable port */
                    //tmp &= ~0x04;

                    //WRITE_REGISTER_ULONG((PULONG) ((Base + EHCI_PORTSC) + (4 * i)), tmp);

                    //KeStallExecutionProcessor(20);

                    tmp = READ_REGISTER_ULONG((PULONG)((OpRegisters + EHCI_PORTSC) + (4 * i)));

                    PdoDeviceExtension->ChildDeviceCount++;
                    hcd->Ports[i].PortStatus &= ~0x8000;
                    hcd->Ports[i].PortStatus |= USB_PORT_STATUS_HIGH_SPEED;
                    hcd->Ports[i].PortStatus |= USB_PORT_STATUS_CONNECT;
                    hcd->Ports[i].PortChange |= USB_PORT_STATUS_CONNECT;
                    DPRINT1("Completing URB\n");
                    CompletePendingURBRequest(PdoDeviceExtension);
                }
                else
                {
                    DPRINT1("Device disconnected on port %d\n", i);
                }
            }
        }
    }

    /* Asnyc Advance */
    if (CStatus & EHCI_STS_IAA)
    {
        DPRINT1("Async Advance!\n");
        CleanupAsyncList(hcd);
    }
}

BOOLEAN NTAPI
InterruptService(PKINTERRUPT Interrupt, PVOID ServiceContext)
{
    PFDO_DEVICE_EXTENSION FdoDeviceExtension;
    PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT) ServiceContext;
    PEHCI_HOST_CONTROLLER hcd;
    ULONG CStatus = 0;

    FdoDeviceExtension = (PFDO_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

    hcd = &FdoDeviceExtension->hcd;

    /* Read device status */
    CStatus = ReadControllerStatus(hcd);

    CStatus &= (EHCI_ERROR_INT | EHCI_STS_INT | EHCI_STS_IAA | EHCI_STS_PCD | EHCI_STS_FLR | EHCI_STS_RECL);

    if ((!CStatus) || (FdoDeviceExtension->DeviceState == 0))
    {
        /* This interrupt isnt for us or not ready for it. */
        return FALSE;
    }
    
    /* Clear status */
    ClearControllerStatus(hcd, CStatus);

    if (CStatus & EHCI_STS_RECL)
    {
        DPRINT("Reclamation\n");
    }

    if (CStatus & EHCI_ERROR_INT)
    {
        DPRINT1("EHCI Status=0x%x\n", CStatus);
        /* This check added in case the NT USB Driver is still loading.
           It will cause this error condition at every device connect. */
        if(CStatus & EHCI_STS_PCD)
        {
            DPRINT1("EHCI Error: Another driver may be interfering with proper operation of this driver\n");
            DPRINT1("  Hint: Ensure that the old NT Usb Driver has been removed!\n");
            ASSERT(FALSE);
        }
    }

    if (CStatus & EHCI_STS_FATAL)
    {
        DPRINT1("EHCI: Host System Error. Possible PCI problems.\n");
        ASSERT(FALSE);
    }

    if (CStatus & EHCI_STS_HALT)
    {
        DPRINT1("EHCI: Host Controller unexpected halt.\n");
        /* FIXME: Reset the controller */
    }

    KeInsertQueueDpc(&FdoDeviceExtension->DpcObject, FdoDeviceExtension, (PVOID)CStatus);
    return TRUE;
}

NTSTATUS
StartDevice(PDEVICE_OBJECT DeviceObject, PCM_PARTIAL_RESOURCE_LIST raw, PCM_PARTIAL_RESOURCE_LIST translated)
{
    PFDO_DEVICE_EXTENSION FdoDeviceExtension;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR resource;
    DEVICE_DESCRIPTION DeviceDescription;
    PEHCI_HOST_CONTROLLER hcd;
    ULONG NumberResources;
    ULONG iCount;
    ULONG DeviceAddress;
    ULONG PropertySize;
    ULONG BusNumber;
    NTSTATUS Status;

    FdoDeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    hcd = &FdoDeviceExtension->hcd;

    /* Sanity Checks */
    Status = IoGetDeviceProperty(FdoDeviceExtension->LowerDevice,
                                 DevicePropertyAddress,
                                 sizeof(ULONG),
                                 &DeviceAddress,
                                 &PropertySize);

    Status = IoGetDeviceProperty(FdoDeviceExtension->LowerDevice,
                                 DevicePropertyBusNumber,
                                 sizeof(ULONG),
                                 &BusNumber,
                                 &PropertySize);


    /* Get the resources the PNP Manager gave */
    NumberResources = translated->Count;
    DPRINT("NumberResources %d\n", NumberResources);
    for (iCount = 0; iCount < NumberResources; iCount++)
    {
        DPRINT("Resource Info %d:\n", iCount);
        resource = &translated->PartialDescriptors[iCount];
        switch(resource->Type)
        {
            case CmResourceTypePort:
            {
                DPRINT("Port Start: %x\n", resource->u.Port.Start);
                DPRINT("Port Length %d\n", resource->u.Port.Length);
                /* FIXME: Handle Ports */
                break;
            }
            case CmResourceTypeInterrupt:
            {
                DPRINT("Interrupt Vector: %x\n", resource->u.Interrupt.Vector);
                FdoDeviceExtension->Vector = resource->u.Interrupt.Vector;
                FdoDeviceExtension->Irql = resource->u.Interrupt.Level;
                FdoDeviceExtension->Affinity = resource->u.Interrupt.Affinity;
                FdoDeviceExtension->Mode = (resource->Flags == CM_RESOURCE_INTERRUPT_LATCHED) ? Latched : LevelSensitive;
                FdoDeviceExtension->IrqShared = resource->ShareDisposition == CmResourceShareShared;
                break;
            }
            case CmResourceTypeMemory:
            {
                PVOID ResourceBase = 0;

                DPRINT("Mem Start: %x\n", resource->u.Memory.Start);
                DPRINT("Mem Length: %d\n", resource->u.Memory.Length);

                ResourceBase = MmMapIoSpace(resource->u.Memory.Start, resource->u.Memory.Length, FALSE);
                DPRINT("ResourceBase %x\n", ResourceBase);
                if (ResourceBase  == NULL)
                {
                    DPRINT1("MmMapIoSpace failed!!!!!!!!!\n");
                }

                GetCapabilities(&FdoDeviceExtension->hcd.ECHICaps, (ULONG)ResourceBase);
                DPRINT1("hcd.ECHICaps.Length %x\n", FdoDeviceExtension->hcd.ECHICaps.Length);
                FdoDeviceExtension->hcd.OpRegisters = (ULONG)((ULONG)ResourceBase + FdoDeviceExtension->hcd.ECHICaps.Length);
                break;
            }
            case CmResourceTypeDma:
            {
                DPRINT("Dma Channel: %x\n", resource->u.Dma.Channel);
                DPRINT("Dma Port: %d\n", resource->u.Dma.Port);
                break;
            }
            case CmResourceTypeDevicePrivate:
            {
                /* Windows does this. */
                DPRINT1("CmResourceTypeDevicePrivate not handled\n");
                break;
            }
            default:
            {
                DPRINT1("PNP Manager gave resource type not handled!! Notify Developers!\n");
                break;
            }
        }
    }

    for (iCount = 0; iCount < hcd->ECHICaps.HCSParams.PortCount; iCount++)
    {
        hcd->Ports[iCount].PortStatus = 0x8000;
        hcd->Ports[iCount].PortChange = 0;
        
        if (hcd->ECHICaps.HCSParams.PortPowerControl)
            hcd->Ports[iCount].PortStatus |= USB_PORT_STATUS_POWER;
    }

    KeInitializeDpc(&FdoDeviceExtension->DpcObject,
                    EhciDefferedRoutine,
                    FdoDeviceExtension);

    RtlZeroMemory(&DeviceDescription, sizeof(DEVICE_DESCRIPTION));

    DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
    DeviceDescription.Master = TRUE;
    DeviceDescription.ScatterGather = TRUE;
    DeviceDescription.Dma32BitAddresses = TRUE;
    DeviceDescription.DmaWidth = 2;
    DeviceDescription.InterfaceType = PCIBus;
    DeviceDescription.MaximumLength = EHCI_MAX_SIZE_TRANSFER;

    hcd->pDmaAdapter = IoGetDmaAdapter(FdoDeviceExtension->LowerDevice,
                                       &DeviceDescription,
                                       &hcd->MapRegisters);

    if (hcd->pDmaAdapter == NULL)
    {
        DPRINT1("Ehci: IoGetDmaAdapter failed!\n");
        ASSERT(FALSE);
    }

    DPRINT1("MapRegisters %x\n", hcd->MapRegisters);

    /* Allocate Common Buffer for Periodic Frame List */
    FdoDeviceExtension->PeriodicFrameList.VirtualAddr =
        hcd->pDmaAdapter->DmaOperations->AllocateCommonBuffer(hcd->pDmaAdapter,
                                                              sizeof(ULONG) * 1024,
                                                              &FdoDeviceExtension->PeriodicFrameList.PhysicalAddr,
                                                              FALSE);

    if (FdoDeviceExtension->PeriodicFrameList.VirtualAddr == NULL)
    {
        DPRINT1("Ehci: FdoDeviceExtension->PeriodicFramList is null\n");
        return STATUS_UNSUCCESSFUL;
    }

    /* Zeroize it */
    RtlZeroMemory(FdoDeviceExtension->PeriodicFrameList.VirtualAddr, sizeof(ULONG) * 1024);

    ExInitializeFastMutex(&FdoDeviceExtension->FrameListMutex);

    /* Allocate initial page for queueheads and descriptors */
    FdoDeviceExtension->hcd.CommonBufferVA[0] =
        hcd->pDmaAdapter->DmaOperations->AllocateCommonBuffer(hcd->pDmaAdapter,
                                                              PAGE_SIZE,
                                                              &FdoDeviceExtension->hcd.CommonBufferPA[0],
                                                              FALSE);

    if (FdoDeviceExtension->hcd.CommonBufferVA[0] == 0)
    {
        DPRINT1("Ehci: Failed to allocate common buffer!\n");
        return STATUS_UNSUCCESSFUL;
    }

    hcd->CommonBufferSize = PAGE_SIZE * 16;

    /* Zeroize it */
    RtlZeroMemory(FdoDeviceExtension->hcd.CommonBufferVA[0],
                  PAGE_SIZE);

    /* Init SpinLock for host controller device lock */
    KeInitializeSpinLock(&hcd->Lock);

    /* Reserved a Queue Head that will always be in the AsyncList Address Register. By setting it as the Head of Reclamation
       the controller can know when it has reached the end of the QueueHead list */
    hcd->AsyncListQueue = CreateQueueHead(hcd);

    hcd->AsyncListQueue->HorizontalLinkPointer = hcd->AsyncListQueue->PhysicalAddr | QH_TYPE_QH;
    hcd->AsyncListQueue->EndPointCharacteristics.QEDTDataToggleControl = FALSE;
    hcd->AsyncListQueue->Token.Bits.InterruptOnComplete = FALSE;
    hcd->AsyncListQueue->EndPointCharacteristics.HeadOfReclamation = TRUE;
    hcd->AsyncListQueue->Token.Bits.Halted = TRUE;
    hcd->AsyncListQueue->NextQueueHead = hcd->AsyncListQueue;
    hcd->AsyncListQueue->PreviousQueueHead = hcd->AsyncListQueue;
    
    /* Reserve a Queue Head thats only purpose is for linking completed Queue Heads.
       Completed QueueHeads are moved to this temporary. As the memory must still be valid
       up until the controllers doorbell is rang to let it know info has been removed from QueueHead list */
    hcd->CompletedListQueue = CreateQueueHead(hcd);
    hcd->CompletedListQueue->NextQueueHead = hcd->CompletedListQueue;
    hcd->CompletedListQueue->PreviousQueueHead = hcd->CompletedListQueue;
    
    /* Ensure the controller is stopped */
    StopEhci(hcd);
    
    SetAsyncListQueueRegister(hcd, hcd->AsyncListQueue->PhysicalAddr);

    /* FIXME: Implement Periodic Frame List */

    Status = IoConnectInterrupt(&FdoDeviceExtension->EhciInterrupt,
                                InterruptService,
                                FdoDeviceExtension->DeviceObject,
                                NULL,
                                FdoDeviceExtension->Vector,
                                FdoDeviceExtension->Irql,
                                FdoDeviceExtension->Irql,
                                FdoDeviceExtension->Mode,
                                FdoDeviceExtension->IrqShared,
                                FdoDeviceExtension->Affinity,
                                FALSE);

    StartEhci(hcd);
    FdoDeviceExtension->DeviceState = DEVICESTARTED;
    return STATUS_SUCCESS;
}

NTSTATUS
FdoQueryBusRelations(
    PDEVICE_OBJECT DeviceObject,
    PDEVICE_RELATIONS* pDeviceRelations)
{
    PFDO_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_RELATIONS DeviceRelations = NULL;
    PDEVICE_OBJECT Pdo;
    PPDO_DEVICE_EXTENSION PdoDeviceExtension;
    NTSTATUS Status;
    ULONG UsbDeviceNumber = 0;
    WCHAR CharDeviceName[64];

    UNICODE_STRING DeviceName;

    DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    DPRINT1("Ehci: QueryBusRelations\n");

    /* FIXME: Currently only support for one ehci controller */
    if (DeviceExtension->Pdo)
        goto Done;

    /* Create the PDO with the next available number */
    while (TRUE)
    {
        /* FIXME: Use safe string */
        /* RtlStringCchPrintfW(CharDeviceName, 64, L"USBPDO-%d", UsbDeviceNumber); */
        swprintf(CharDeviceName, L"\\Device\\USBPDO-%d", UsbDeviceNumber);
        RtlInitUnicodeString(&DeviceName, CharDeviceName);
        DPRINT("DeviceName %wZ\n", &DeviceName);

        Status = IoCreateDevice(DeviceObject->DriverObject,
                                sizeof(PDO_DEVICE_EXTENSION),
                                &DeviceName,
                                FILE_DEVICE_BUS_EXTENDER,
                                0,
                                FALSE,
                                &Pdo);

        if (NT_SUCCESS(Status))
            break;

        if ((Status == STATUS_OBJECT_NAME_EXISTS) || (Status == STATUS_OBJECT_NAME_COLLISION))
        {
            /* Try the next name */
            UsbDeviceNumber++;
            continue;
        }

        /* Bail on any other error */
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Ehci: Failed to create PDO %wZ, Status %x\n", &DeviceName, Status);
            return Status;
        }
    }

    PdoDeviceExtension = (PPDO_DEVICE_EXTENSION)Pdo->DeviceExtension;
    RtlZeroMemory(PdoDeviceExtension, sizeof(PDO_DEVICE_EXTENSION));
    PdoDeviceExtension->Common.IsFdo = FALSE;

    PdoDeviceExtension->ControllerFdo = DeviceObject;
    PdoDeviceExtension->DeviceObject = Pdo;
    //PdoDeviceExtension->NumberOfPorts = DeviceExtension->hcd.ECHICaps.HCSParams.PortCount;

    InitializeListHead(&PdoDeviceExtension->IrpQueue);
    
    KeInitializeSpinLock(&PdoDeviceExtension->IrpQueueLock);

    KeInitializeEvent(&PdoDeviceExtension->QueueDrainedEvent, SynchronizationEvent, TRUE);

    ExInitializeFastMutex(&PdoDeviceExtension->ListLock);

    Pdo->Flags &= ~DO_DEVICE_INITIALIZING;

    DeviceExtension->Pdo = Pdo;
Done:
    DeviceRelations = (PDEVICE_RELATIONS)ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS));

    if (!DeviceRelations)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DeviceRelations->Count = 1;
    DeviceRelations->Objects[0] = DeviceExtension->Pdo;
    ObReferenceObject(DeviceExtension->Pdo);

    *pDeviceRelations = DeviceRelations;
    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
FdoDispatchPnp(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack = NULL;
    PCM_PARTIAL_RESOURCE_LIST raw;
    PCM_PARTIAL_RESOURCE_LIST translated;
    ULONG_PTR Information = 0;

    Stack =  IoGetCurrentIrpStackLocation(Irp);

    switch(Stack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
        {
            DPRINT1("Ehci: START_DEVICE\n");

            Irp->IoStatus.Status = STATUS_SUCCESS;
            Status = ForwardAndWait(DeviceObject, Irp);

            raw = &Stack->Parameters.StartDevice.AllocatedResources->List[0].PartialResourceList;
            translated = &Stack->Parameters.StartDevice.AllocatedResourcesTranslated->List[0].PartialResourceList;
            Status = StartDevice(DeviceObject, raw, translated);
            break;
        }
        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            DPRINT1("Ehci: IRP_MN_QUERY_DEVICE_RELATIONS\n");
            switch(Stack->Parameters.QueryDeviceRelations.Type)
            {
                case BusRelations:
                {
                    PDEVICE_RELATIONS DeviceRelations = NULL;
                    DPRINT1("Ehci: BusRelations\n");
                    Status = FdoQueryBusRelations(DeviceObject, &DeviceRelations);
                    Information = (ULONG_PTR)DeviceRelations;
                    break;
                }
                default:
                {
                    DPRINT1("Ehci: Unknown query device relations type\n");
                    Status = STATUS_NOT_IMPLEMENTED;
                    break;
                }
             }
             break;
        }
        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
        {
            DPRINT1("Ehci: IRP_MN_FILTER_RESOURCE_REQUIREMENTS\n");
            return ForwardIrpAndForget(DeviceObject, Irp);
            break;
        }
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
        {
            DPRINT1("Ehci: IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n");
        }
        case IRP_MN_QUERY_INTERFACE:
        {
            DPRINT1("Ehci: IRP_MN_QUERY_INTERFACE\n");
            Status = STATUS_SUCCESS;
            Information = 0;
            Status = ForwardIrpAndForget(DeviceObject, Irp);
            return Status;
            break;
        }
        default:
        {
            DPRINT1("Ehci: IRP_MJ_PNP / Unhandled minor function 0x%lx\n", Stack->MinorFunction);
            return ForwardIrpAndForget(DeviceObject, Irp);
        }
    }

    Irp->IoStatus.Information = Information;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS NTAPI
AddDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT Pdo)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PDEVICE_OBJECT Fdo;
    ULONG UsbDeviceNumber = 0;
    WCHAR CharDeviceName[64];
    WCHAR CharSymLinkName[64];
    UNICODE_STRING DeviceName;
    UNICODE_STRING SymLinkName;
    UNICODE_STRING InterfaceSymLinkName;
    ULONG BytesRead;
    PCI_COMMON_CONFIG PciConfig;

    PFDO_DEVICE_EXTENSION FdoDeviceExtension;

    DPRINT1("Ehci: AddDevice\n");

    /* Create the FDO with next available number */
    while (TRUE)
    {
        /* FIXME: Use safe string sprintf*/
        /* RtlStringCchPrintfW(CharDeviceName, 64, L"USBFDO-%d", UsbDeviceNumber); */
        swprintf(CharDeviceName, L"\\Device\\USBFDO-%d", UsbDeviceNumber);
        RtlInitUnicodeString(&DeviceName, CharDeviceName);
        DPRINT("DeviceName %wZ\n", &DeviceName);

        Status = IoCreateDevice(DriverObject,
                                sizeof(FDO_DEVICE_EXTENSION),
                                &DeviceName,
                                FILE_DEVICE_CONTROLLER,
                                0,
                                FALSE,
                                &Fdo);

        if (NT_SUCCESS(Status))
            break;

        if ((Status == STATUS_OBJECT_NAME_EXISTS) || (Status == STATUS_OBJECT_NAME_COLLISION))
        {
            /* Try the next name */
            UsbDeviceNumber++;
            continue;
        }

        /* Bail on any other error */
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("UsbEhci: Failed to create %wZ, Status %x\n", &DeviceName, Status);
            return Status;
        }
    }

    swprintf(CharSymLinkName, L"\\Device\\HCD%d", UsbDeviceNumber);
    RtlInitUnicodeString(&SymLinkName, CharSymLinkName);
    Status = IoCreateSymbolicLink(&SymLinkName, &DeviceName);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Warning: Unable to create symbolic link for ehci host controller!\n");
    }

    FdoDeviceExtension = (PFDO_DEVICE_EXTENSION) Fdo->DeviceExtension;
    RtlZeroMemory(FdoDeviceExtension, sizeof(PFDO_DEVICE_EXTENSION));

    KeInitializeTimerEx(&FdoDeviceExtension->UpdateTimer, SynchronizationTimer);

    FdoDeviceExtension->Common.IsFdo = TRUE;
    FdoDeviceExtension->DeviceObject = Fdo;

    FdoDeviceExtension->LowerDevice = IoAttachDeviceToDeviceStack(Fdo, Pdo);

    if (FdoDeviceExtension->LowerDevice == NULL)
    {
        DPRINT1("UsbEhci: Failed to attach to device stack!\n");
        IoDeleteSymbolicLink(&SymLinkName);
        IoDeleteDevice(Fdo);

        return STATUS_NO_SUCH_DEVICE;
    }

    Fdo->Flags |= DO_BUFFERED_IO;// | DO_POWER_PAGABLE;

    ASSERT(FdoDeviceExtension->LowerDevice == Pdo);

    /* Get the EHCI Device ID and Vendor ID */
    Status = GetBusInterface(FdoDeviceExtension->LowerDevice, &FdoDeviceExtension->BusInterface);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("GetBusInterface() failed with %x\n", Status);
        IoDetachDevice(FdoDeviceExtension->LowerDevice);
        IoDeleteSymbolicLink(&SymLinkName);
        IoDeleteDevice(Fdo);
        return Status;
    }

    BytesRead = (*FdoDeviceExtension->BusInterface.GetBusData)(
        FdoDeviceExtension->BusInterface.Context,
        PCI_WHICHSPACE_CONFIG,
        &PciConfig,
        0,
        PCI_COMMON_HDR_LENGTH);


    if (BytesRead != PCI_COMMON_HDR_LENGTH)
    {
        DPRINT1("GetBusData failed!\n");
        IoDetachDevice(FdoDeviceExtension->LowerDevice);
        IoDeleteSymbolicLink(&SymLinkName);
        IoDeleteDevice(Fdo);
        return STATUS_UNSUCCESSFUL;
    }

    if (PciConfig.Command & PCI_ENABLE_IO_SPACE)
        DPRINT("PCI_ENABLE_IO_SPACE\n");

    if (PciConfig.Command & PCI_ENABLE_MEMORY_SPACE)
        DPRINT("PCI_ENABLE_MEMORY_SPACE\n");

    if (PciConfig.Command & PCI_ENABLE_BUS_MASTER)
        DPRINT("PCI_ENABLE_BUS_MASTER\n");

    DPRINT("BaseAddress[0] %x\n", PciConfig.u.type0.BaseAddresses[0]);
    DPRINT1("Vendor %x\n", PciConfig.VendorID);
    DPRINT1("Device %x\n", PciConfig.DeviceID);

    FdoDeviceExtension->VendorId = PciConfig.VendorID;
    FdoDeviceExtension->DeviceId = PciConfig.DeviceID;

    FdoDeviceExtension->DeviceState = DEVICEINTIALIZED;

    Status = IoRegisterDeviceInterface(Pdo, &GUID_DEVINTERFACE_USB_HOST_CONTROLLER, NULL, &InterfaceSymLinkName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Unable to register device interface!\n");
        return Status;
    }
    else
    {
        Status = IoSetDeviceInterfaceState(&InterfaceSymLinkName, TRUE);
        DPRINT1("SetInterfaceState %x\n", Status);
        if (!NT_SUCCESS(Status))
            return Status;
    }
    Fdo->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

NTSTATUS NTAPI
FdoDispatchInternalDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    /*FIXME: This should never be called by upper drivers as they should only be dealing with the pdo. */
    DPRINT1("Upper Level Device Object shouldnt be calling this!!!!!!!!!!!!\n");
    ASSERT(FALSE);
    return STATUS_UNSUCCESSFUL;
}
