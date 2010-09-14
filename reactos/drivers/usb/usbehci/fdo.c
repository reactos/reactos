/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/fdo.c
 * PURPOSE:     USB EHCI device driver.
 * PROGRAMMERS:
 *              Michael Martin (mjmartin@reactos.org)
 */

/* INCLUDES *******************************************************************/
#include "usbehci.h"
#include <stdio.h>

VOID NTAPI
TimerDefferedRoutine(PKDPC Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    PFDO_DEVICE_EXTENSION FdoDeviceExtension;
    PPDO_DEVICE_EXTENSION PdoDeviceExtension;
    ULONG tmp;
    ULONG Base;
    LONG i;

    FdoDeviceExtension = (PFDO_DEVICE_EXTENSION) DeferredContext;
    PdoDeviceExtension = (PPDO_DEVICE_EXTENSION) FdoDeviceExtension->Pdo->DeviceExtension;

    Base = (ULONG)FdoDeviceExtension->ResourceMemory;

    for (i = 0; i < FdoDeviceExtension->ECHICaps.HCSParams.PortCount; i++)
    {
        tmp = READ_REGISTER_ULONG((PULONG) ((Base + EHCI_PORTSC) + (4 * i)));

        if ((tmp & 0x04) && (!(PdoDeviceExtension->Ports[i].PortChange & USB_PORT_STATUS_RESET)))
        {
            DPRINT("Port %x is enabled\n", i);
            PdoDeviceExtension->Ports[i].PortChange |= USB_PORT_STATUS_RESET;
            CompletePendingURBRequest(PdoDeviceExtension);
        }
    }
    return;
}

VOID NTAPI
EhciDefferedRoutine(PKDPC Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    PFDO_DEVICE_EXTENSION FdoDeviceExtension;
    PPDO_DEVICE_EXTENSION PdoDeviceExtension;
    ULONG CStatus;
    ULONG tmp;
    ULONG Base;
    LONG i;

    FdoDeviceExtension = (PFDO_DEVICE_EXTENSION) DeferredContext;

    if (!FdoDeviceExtension->Pdo)
    {
        DPRINT1("PDO not set yet!\n");
        return;
    }

    PdoDeviceExtension = (PPDO_DEVICE_EXTENSION) FdoDeviceExtension->Pdo->DeviceExtension;

    Base = (ULONG)FdoDeviceExtension->ResourceMemory;

    CStatus = (ULONG) SystemArgument2;

    /* Port Change */
    if (CStatus & EHCI_STS_PCD)
    {
        /* Loop through the ports */
        for (i = 0; i < FdoDeviceExtension->ECHICaps.HCSParams.PortCount; i++)
        {
            tmp = READ_REGISTER_ULONG((PULONG) ((Base + EHCI_PORTSC) + (4 * i)));

            /* Check for port change on this port */
            if (tmp & 0x02)
            {
                /* Connect or Disconnect? */
                if (tmp & 0x01)
                {
                    DPRINT1("Device connected on port %d\n", i);

                    /* Check if a companion host controller exists */
                    if (FdoDeviceExtension->ECHICaps.HCSParams.CHCCount)
                    {
                        tmp = READ_REGISTER_ULONG((PULONG)((Base + EHCI_PORTSC) + (4 * i)));

                        /* Port should be in disabled state, as per USB 2.0 specs */
                        if (tmp & 0x04)
                        {
                            DPRINT1("Warning: The port the device has just connected to is not disabled!\n");
                        }

                        /* Is this non high speed device */
                        if (tmp & 0x400)
                        {
                            DPRINT1("Releasing ownership to companion host controller!\n");
                            /* Release ownership to companion host controller */
                            WRITE_REGISTER_ULONG((PULONG) ((Base + EHCI_PORTSC) + (4 * i)), 0x4000);
                            continue;
                        }
                    }

                    KeStallExecutionProcessor(30);

                    /* As per USB 2.0 Specs, 9.1.2. Reset the port and clear the status change */
                    //tmp |= 0x100 | 0x02;
                    /* Sanity, Disable port */
                    //tmp &= ~0x04;

                    //WRITE_REGISTER_ULONG((PULONG) ((Base + EHCI_PORTSC) + (4 * i)), tmp);

                    //KeStallExecutionProcessor(20);

                    tmp = READ_REGISTER_ULONG((PULONG)((Base + EHCI_PORTSC) + (4 * i)));

                    PdoDeviceExtension->ChildDeviceCount++;
                    PdoDeviceExtension->Ports[i].PortStatus &= ~0x8000;
                    PdoDeviceExtension->Ports[i].PortStatus |= USB_PORT_STATUS_CONNECT;
                    PdoDeviceExtension->Ports[i].PortChange |= USB_PORT_STATUS_CONNECT;

                    CompletePendingURBRequest(PdoDeviceExtension);
                }
                else
                {
                    DPRINT1("Device disconnected on port %d\n", i);

                    /* Clear status change */
                    tmp = READ_REGISTER_ULONG((PULONG)((Base + EHCI_PORTSC) + (4 * i)));
                    tmp |= 0x02;
                    WRITE_REGISTER_ULONG((PULONG) ((Base + EHCI_PORTSC) + (4 * i)), tmp);
                }
            }
        }
    }
}

BOOLEAN NTAPI
InterruptService(PKINTERRUPT Interrupt, PVOID ServiceContext)
{
    PFDO_DEVICE_EXTENSION FdoDeviceExtension;
    PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT) ServiceContext;
    ULONG CurrentFrame;
    ULONG Base;
    ULONG CStatus = 0;

    FdoDeviceExtension = (PFDO_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

    Base = (ULONG)FdoDeviceExtension->ResourceMemory;

    /* Read device status */
    CStatus = READ_REGISTER_ULONG ((PULONG) (Base + EHCI_USBSTS));
    CurrentFrame = READ_REGISTER_ULONG((PULONG) (Base + EHCI_FRINDEX));

    CStatus &= (EHCI_ERROR_INT | EHCI_STS_INT | EHCI_STS_IAA | EHCI_STS_PCD | EHCI_STS_FLR);

    if ((!CStatus) || (FdoDeviceExtension->DeviceState == 0))
    {
        /* This interrupt isnt for us or not ready for it. */
        return FALSE;
    }

    /* Clear status */
    WRITE_REGISTER_ULONG((PULONG) (Base + EHCI_USBSTS), CStatus);

    if (CStatus & EHCI_ERROR_INT)
    {
        DPRINT1("EHCI Status=0x%x\n", CStatus);
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

    if (CStatus & EHCI_STS_INT)
    {
       FdoDeviceExtension->AsyncComplete = TRUE;
    }

    KeInsertQueueDpc(&FdoDeviceExtension->DpcObject, FdoDeviceExtension, (PVOID)CStatus);

    return TRUE;
}

BOOLEAN
ResetPort(PFDO_DEVICE_EXTENSION FdoDeviceExtension, UCHAR Port)
{
    ULONG Base;
    ULONG tmp;
    LARGE_INTEGER DueTime;

    DPRINT("Reset Port %x\n", Port);

    Base = (ULONG)FdoDeviceExtension->ResourceMemory;

    tmp = READ_REGISTER_ULONG((PULONG) ((Base + EHCI_PORTSC) + (4 * Port)));
    tmp |= 0x100;
    WRITE_REGISTER_ULONG((PULONG) ((Base + EHCI_PORTSC) + (4 * Port)), tmp);

    DueTime.QuadPart = -100;

    KeSetTimerEx(&FdoDeviceExtension->UpdateTimer, DueTime, 0, &FdoDeviceExtension->TimerDpcObject);
    return TRUE;
}

VOID
StopEhci(PDEVICE_OBJECT DeviceObject)
{
    PFDO_DEVICE_EXTENSION FdoDeviceExtension;
    PEHCI_USBCMD_CONTENT UsbCmd;
    ULONG base;
    LONG tmp;

    DPRINT("Stopping Ehci controller\n");
    FdoDeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    base = (ULONG)FdoDeviceExtension->ResourceMemory;

    WRITE_REGISTER_ULONG((PULONG) (base + EHCI_USBINTR), 0);

    tmp = READ_REGISTER_ULONG((PULONG) (base + EHCI_USBCMD));
    UsbCmd = (PEHCI_USBCMD_CONTENT) & tmp;
    UsbCmd->Run = 0;
    WRITE_REGISTER_ULONG((PULONG) (base + EHCI_USBCMD), tmp);
}

VOID
StartEhci(PDEVICE_OBJECT DeviceObject)
{
    PFDO_DEVICE_EXTENSION FdoDeviceExtension;
    PEHCI_USBCMD_CONTENT UsbCmd;
    PEHCI_USBSTS_CONTEXT usbsts;
    NTSTATUS Status;
    LONG tmp;
    LONG tmp2;
    ULONG base;

    DPRINT("Starting Ehci controller\n");
    FdoDeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    base = (ULONG)FdoDeviceExtension->ResourceMemory;

    tmp = READ_REGISTER_ULONG ((PULONG)(base + EHCI_USBCMD));

    /* Stop the device */
    UsbCmd = (PEHCI_USBCMD_CONTENT) &tmp;
    UsbCmd->Run = 0;
    WRITE_REGISTER_ULONG ((PULONG)(base + EHCI_USBCMD), tmp);

    /* Wait for the device to stop */
    for (;;)
    {
        KeStallExecutionProcessor(10);
        tmp = READ_REGISTER_ULONG((PULONG)(base + EHCI_USBSTS));
        usbsts = (PEHCI_USBSTS_CONTEXT)&tmp;

        if (usbsts->HCHalted)
        {
            break;
        }
        DPRINT("Waiting for Halt, USBSTS: %x\n", READ_REGISTER_ULONG ((PULONG)(base + EHCI_USBSTS)));
    }

    tmp = READ_REGISTER_ULONG ((PULONG)(base + EHCI_USBCMD));

    /* Reset the device */
    UsbCmd = (PEHCI_USBCMD_CONTENT) &tmp;
    UsbCmd->HCReset = TRUE;
    WRITE_REGISTER_ULONG ((PULONG)(base + EHCI_USBCMD), tmp);

    /* Wait for the device to reset */
    for (;;)
    {
        KeStallExecutionProcessor(10);
        tmp = READ_REGISTER_ULONG((PULONG)(base + EHCI_USBCMD));
        UsbCmd = (PEHCI_USBCMD_CONTENT)&tmp;

        if (!UsbCmd->HCReset)
        {
            break;
        }
        DPRINT("Waiting for reset, USBCMD: %x\n", READ_REGISTER_ULONG ((PULONG)(base + EHCI_USBCMD)));
    }

    UsbCmd = (PEHCI_USBCMD_CONTENT) &tmp;

    /* Disable Interrupts on the device */
    WRITE_REGISTER_ULONG((PULONG)(base + EHCI_USBINTR), 0);
    /* Clear the Status */
    WRITE_REGISTER_ULONG((PULONG)(base + EHCI_USBSTS), 0x0000001f);

    WRITE_REGISTER_ULONG((PULONG)(base + EHCI_CTRLDSSEGMENT), 0);

    /* Set the Periodic Frame List */
    WRITE_REGISTER_ULONG((PULONG)(base + EHCI_PERIODICLISTBASE), FdoDeviceExtension->PeriodicFramListPhysAddr.LowPart);
    /* Set the Async List Queue */
    WRITE_REGISTER_ULONG((PULONG) (base + EHCI_ASYNCLISTBASE), FdoDeviceExtension->AsyncListQueueHeadPtrPhysAddr.LowPart & ~(0x1f));

    /* Set the ansync and periodic to disable */
    UsbCmd->PeriodicEnable = 0;
    UsbCmd->AsyncEnable = 0;
    WRITE_REGISTER_ULONG((PULONG)(base + EHCI_USBCMD), tmp);

    /* Set the threshold */
    UsbCmd->IntThreshold = 1;
    WRITE_REGISTER_ULONG((PULONG)(base + EHCI_USBCMD), tmp);

    KeInitializeDpc(&FdoDeviceExtension->DpcObject,
                    EhciDefferedRoutine,
                    FdoDeviceExtension);

    KeInitializeDpc(&FdoDeviceExtension->TimerDpcObject,
                    TimerDefferedRoutine,
                    FdoDeviceExtension);


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

    /* Turn back on interrupts */
    WRITE_REGISTER_ULONG((PULONG)(base + EHCI_USBINTR),
                      EHCI_USBINTR_ERR | EHCI_USBINTR_ASYNC | EHCI_USBINTR_HSERR
                      | EHCI_USBINTR_FLROVR  | EHCI_USBINTR_PC);
    WRITE_REGISTER_ULONG((PULONG)(base + EHCI_USBINTR),
                      EHCI_USBINTR_INTE | EHCI_USBINTR_ERR | EHCI_USBINTR_ASYNC | EHCI_USBINTR_HSERR
                      | EHCI_USBINTR_FLROVR  | EHCI_USBINTR_PC);

    UsbCmd->Run = 1;
    WRITE_REGISTER_ULONG((PULONG)(base + EHCI_USBCMD), tmp);

    /* Wait for the device to start */
    for (;;)
    {
        KeStallExecutionProcessor(10);
        tmp2 = READ_REGISTER_ULONG((PULONG)(base + EHCI_USBSTS));
        usbsts = (PEHCI_USBSTS_CONTEXT)&tmp2;

        if (!usbsts->HCHalted)
        {
            break;
        }
        DPRINT("Waiting for start, USBSTS: %x\n", READ_REGISTER_ULONG ((PULONG)(base + EHCI_USBSTS)));
    }

    /* Set all port routing to ECHI controller */
    WRITE_REGISTER_ULONG((PULONG)(base + EHCI_CONFIGFLAG), 1);
}

VOID
GetCapabilities(PFDO_DEVICE_EXTENSION DeviceExtension, ULONG Base)
{
    PEHCI_CAPS PCap;
    PEHCI_HCS_CONTENT PHCS;
    LONG i;

    if (!DeviceExtension)
        return;

    PCap = &DeviceExtension->ECHICaps;

    PCap->Length = READ_REGISTER_UCHAR((PUCHAR)Base);
    PCap->Reserved = READ_REGISTER_UCHAR((PUCHAR)(Base + 1));
    PCap->HCIVersion = READ_REGISTER_USHORT((PUSHORT)(Base + 2));
    PCap->HCSParamsLong = READ_REGISTER_ULONG((PULONG)(Base + 4));
    PCap->HCCParams = READ_REGISTER_ULONG((PULONG)(Base + 8));

    DPRINT("Length %d\n", PCap->Length);
    DPRINT("Reserved %d\n", PCap->Reserved);
    DPRINT("HCIVersion %x\n", PCap->HCIVersion);
    DPRINT("HCSParams %x\n", PCap->HCSParamsLong);
    DPRINT("HCCParams %x\n", PCap->HCCParams);

    if (PCap->HCCParams & 0x02)
        DPRINT1("Frame list size is configurable\n");

    if (PCap->HCCParams & 0x01)
        DPRINT1("64bit address mode not supported!\n");

    DPRINT1("Number of Ports: %d\n", PCap->HCSParams.PortCount);

    if (PCap->HCSParams.PortPowerControl)
        DPRINT1("Port Power Control is enabled\n");

    if (!PCap->HCSParams.CHCCount)
    {
        DPRINT1("Number of Companion Host controllers %x\n", PCap->HCSParams.CHCCount);
        DPRINT1("Number of Ports Per CHC: %d\n", PCap->HCSParams.PortPerCHC);
    }

    /* Copied from USBDRIVER in trunk */
    PHCS = (PEHCI_HCS_CONTENT)&DeviceExtension->ECHICaps.HCSParams;
    if (PHCS->PortRouteRules)
    {
        for (i = 0; i < PCap->HCSParams.PortCount; i++)
        {
            PCap->PortRoute[i] = READ_REGISTER_UCHAR((PUCHAR) (Base + 12 + i));
        }
    }
}

NTSTATUS
StartDevice(PDEVICE_OBJECT DeviceObject, PCM_PARTIAL_RESOURCE_LIST raw, PCM_PARTIAL_RESOURCE_LIST translated)
{
    PFDO_DEVICE_EXTENSION FdoDeviceExtension;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR resource;
    DEVICE_DESCRIPTION DeviceDescription;
    ULONG NumberResources;
    ULONG iCount;
    ULONG DeviceAddress;
    ULONG PropertySize;
    ULONG BusNumber;
    NTSTATUS Status;

    FdoDeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    RtlZeroMemory(&DeviceDescription, sizeof(DEVICE_DESCRIPTION));
    DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
    DeviceDescription.Master = TRUE;
    DeviceDescription.ScatterGather = TRUE;
    DeviceDescription.Dma32BitAddresses = TRUE;
    DeviceDescription.DmaWidth = 2;
    DeviceDescription.InterfaceType = PCIBus;
    DeviceDescription.MaximumLength = EHCI_MAX_SIZE_TRANSFER;

    FdoDeviceExtension->pDmaAdapter = IoGetDmaAdapter(FdoDeviceExtension->LowerDevice,
                                                      &DeviceDescription,
                                                      &FdoDeviceExtension->MapRegisters);

    if (FdoDeviceExtension->pDmaAdapter == NULL)
    {
        DPRINT1("IoGetDmaAdapter failed!\n");
        ASSERT(FALSE);
    }

    /* Allocate Common Buffer for Periodic Frame List */
    FdoDeviceExtension->PeriodicFramList =
        FdoDeviceExtension->pDmaAdapter->DmaOperations->AllocateCommonBuffer(FdoDeviceExtension->pDmaAdapter,
        sizeof(ULONG) * 1024, &FdoDeviceExtension->PeriodicFramListPhysAddr, FALSE);

    if (FdoDeviceExtension->PeriodicFramList == NULL)
    {
        DPRINT1("FdoDeviceExtension->PeriodicFramList is null\n");
        return STATUS_UNSUCCESSFUL;
    }

    /* Zeroize it */
    RtlZeroMemory(FdoDeviceExtension->PeriodicFramList, sizeof(ULONG) * 1024);

    ExInitializeFastMutex(&FdoDeviceExtension->FrameListMutex);

    /* Allocate Common Buffer for Async List Head Queue */
    FdoDeviceExtension->AsyncListQueueHeadPtr =
        FdoDeviceExtension->pDmaAdapter->DmaOperations->AllocateCommonBuffer(FdoDeviceExtension->pDmaAdapter,
        /* FIXME: Memory Size should be calculated using
                  structures sizes needed for queue head + 20480 (max data transfer */
        20800,
        &FdoDeviceExtension->AsyncListQueueHeadPtrPhysAddr, FALSE);

    if (FdoDeviceExtension->AsyncListQueueHeadPtr == NULL)
    {
        DPRINT1("Failed to allocate common buffer for AsyncListQueueHeadPtr!\n");
        return STATUS_UNSUCCESSFUL;
    }

    /* Zeroize it */
    RtlZeroMemory(FdoDeviceExtension->AsyncListQueueHeadPtr,
                  /* FIXME: Same as FIXME above */
                  20800);

    ExInitializeFastMutex(&FdoDeviceExtension->AsyncListMutex);

    Status = IoGetDeviceProperty(FdoDeviceExtension->LowerDevice,
                                 DevicePropertyAddress,
                                 sizeof(ULONG),
                                 &DeviceAddress,
                                 &PropertySize);
    if (NT_SUCCESS(Status))
    {
        DPRINT1("--->DeviceAddress: %x\n", DeviceAddress);
    }

    Status = IoGetDeviceProperty(FdoDeviceExtension->LowerDevice,
                                 DevicePropertyBusNumber,
                                 sizeof(ULONG),
                                 &BusNumber,
                                 &PropertySize);
    if (NT_SUCCESS(Status))
    {
        DPRINT1("--->BusNumber: %x\n", BusNumber);
    }

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
                ULONG ResourceBase = 0;
                ULONG MemLength;

                DPRINT("Mem Start: %x\n", resource->u.Memory.Start);
                DPRINT("Mem Length: %d\n", resource->u.Memory.Length);

                ResourceBase = (ULONG) MmMapIoSpace(resource->u.Memory.Start, resource->u.Memory.Length, FALSE);
                DPRINT("ResourceBase %x\n", ResourceBase);

                FdoDeviceExtension->ResourceBase = (PULONG) ResourceBase;
                GetCapabilities(FdoDeviceExtension, (ULONG)ResourceBase);
                FdoDeviceExtension->ResourceMemory = (PULONG)((ULONG)ResourceBase + FdoDeviceExtension->ECHICaps.Length);
                DPRINT("ResourceMemory %x\n", FdoDeviceExtension->ResourceMemory);
                if (FdoDeviceExtension->ResourceBase  == NULL)
                {
                    DPRINT1("MmMapIoSpace failed!!!!!!!!!\n");
                }
                MemLength = resource->u.Memory.Length;
                FdoDeviceExtension->Size = MemLength;

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

    StartEhci(DeviceObject);
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
            DPRINT1("UsbEhci: Failed to create PDO %wZ, Status %x\n", &DeviceName, Status);
            return Status;
        }
    }

    PdoDeviceExtension = (PPDO_DEVICE_EXTENSION)Pdo->DeviceExtension;
    RtlZeroMemory(PdoDeviceExtension, sizeof(PDO_DEVICE_EXTENSION));
    PdoDeviceExtension->Common.IsFdo = FALSE;

    PdoDeviceExtension->ControllerFdo = DeviceObject;
    PdoDeviceExtension->DeviceObject = Pdo;
    PdoDeviceExtension->NumberOfPorts = DeviceExtension->ECHICaps.HCSParams.PortCount;

    InitializeListHead(&PdoDeviceExtension->IrpQueue);
    KeInitializeSpinLock(&PdoDeviceExtension->IrpQueueLock);

    KeInitializeEvent(&PdoDeviceExtension->QueueDrainedEvent, SynchronizationEvent, TRUE);

    ExInitializeFastMutex(&PdoDeviceExtension->ListLock);

    Pdo->Flags &= ~DO_DEVICE_INITIALIZING;

    DeviceExtension->Pdo = Pdo;

    DeviceRelations = (PDEVICE_RELATIONS)ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS));

    if (!DeviceRelations)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DeviceRelations->Count = 1;
    DeviceRelations->Objects[0] = Pdo;
    ObReferenceObject(Pdo);

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
    PFDO_DEVICE_EXTENSION FdoDeviceExtension;
    PPDO_DEVICE_EXTENSION PdoDeviceExtension;
    PIO_STACK_LOCATION Stack = NULL;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    ULONG_PTR Information = 0;
    PUSB_DEVICE UsbDevice = NULL;
    URB *Urb;

    /*FIXME: This should never be called by a miniport as the miniport should only be dealing with the pdo */

    FdoDeviceExtension = (PFDO_DEVICE_EXTENSION) DeviceObject->DeviceExtension;
    PdoDeviceExtension = (PPDO_DEVICE_EXTENSION) FdoDeviceExtension->Pdo->DeviceExtension;

    ASSERT(FdoDeviceExtension->Common.IsFdo == TRUE);

    Stack =  IoGetCurrentIrpStackLocation(Irp);

    ASSERT(Stack->Parameters.DeviceIoControl.IoControlCode == IOCTL_INTERNAL_USB_SUBMIT_URB);

    Urb = (PURB) Stack->Parameters.Others.Argument1;
    DPRINT("Header Length %d\n", Urb->UrbHeader.Length);
    DPRINT("Header Function %d\n", Urb->UrbHeader.Function);

    UsbDevice = DeviceHandleToUsbDevice(PdoDeviceExtension, Urb->UrbHeader.UsbdDeviceHandle);

    if (!UsbDevice)
    {
        DPRINT1("Invalid DeviceHandle or device not connected\n");
        return STATUS_DEVICE_NOT_CONNECTED;
    }
    switch (Urb->UrbHeader.Function)
    {
        case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
        {
            DPRINT1("Ehci: URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:\n");
            break;
        }
        case URB_FUNCTION_GET_STATUS_FROM_DEVICE:
        {
            DPRINT1("Ehci: URB_FUNCTION_GET_STATUS_FROM_DEVICE\n");
            break;
        }
        case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
        {
            DPRINT1("Ehci: URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE\n");
            switch(Urb->UrbControlDescriptorRequest.DescriptorType)
            {
                case USB_DEVICE_DESCRIPTOR_TYPE:
                {
                    /* FIXME: This probably not used for FDO and should be removed? */
                    DPRINT1("USB DEVICE DESC\n");
                    break;
                }
                case USB_CONFIGURATION_DESCRIPTOR_TYPE:
                    DPRINT1("USB CONFIG DESC\n");
                    //break;
                case USB_STRING_DESCRIPTOR_TYPE:
                    DPRINT1("Usb String Descriptor\n");
                {
                    USB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;
                    PUSB_STRING_DESCRIPTOR UsbString;
                    BOOLEAN ResultOk;

                    CtrlSetup.bmRequestType._BM.Recipient = BMREQUEST_TO_DEVICE;
                    CtrlSetup.bmRequestType._BM.Type = BMREQUEST_STANDARD;
                    CtrlSetup.bmRequestType._BM.Reserved = 0;
                    CtrlSetup.bmRequestType._BM.Dir = BMREQUEST_DEVICE_TO_HOST;
                    CtrlSetup.bRequest = USB_REQUEST_GET_DESCRIPTOR;
                    CtrlSetup.wValue.LowByte = Urb->UrbControlDescriptorRequest.Index;
                    CtrlSetup.wValue.HiByte = Urb->UrbControlDescriptorRequest.DescriptorType;

                    if (Urb->UrbControlDescriptorRequest.DescriptorType == USB_STRING_DESCRIPTOR_TYPE)
                    {
                        CtrlSetup.wIndex.W = Urb->UrbControlDescriptorRequest.LanguageId;
                        RtlZeroMemory(Urb->UrbControlDescriptorRequest.TransferBuffer, Urb->UrbControlDescriptorRequest.TransferBufferLength-1);
                    }
                    else
                        CtrlSetup.wIndex.W = 0;

                    CtrlSetup.wLength = Urb->UrbControlDescriptorRequest.TransferBufferLength;

                    ResultOk = ExecuteControlRequest(FdoDeviceExtension, &CtrlSetup, UsbDevice->Address, UsbDevice->Port,
                                Urb->UrbControlDescriptorRequest.TransferBuffer, Urb->UrbControlDescriptorRequest.TransferBufferLength);

                    if (Urb->UrbControlDescriptorRequest.DescriptorType == USB_STRING_DESCRIPTOR_TYPE)
                    {
                        UsbString = Urb->UrbControlDescriptorRequest.TransferBuffer;
                        DPRINT1("Index %x\n", Urb->UrbControlDescriptorRequest.Index);
                        DPRINT1("BufferLength %x\n", Urb->UrbControlDescriptorRequest.TransferBufferLength);
                        DPRINT1("Length %x\n", UsbString->bLength);
                        if (Urb->UrbControlDescriptorRequest.Index == 0)
                        {
                            DPRINT1("%x\n", (ULONG)Urb->UrbControlDescriptorRequest.TransferBuffer);
                        }
                        else
                            DPRINT1("String %S\n", &UsbString->bString);
                    }
                    UsbString = Urb->UrbControlDescriptorRequest.TransferBuffer;
                    Urb->UrbHeader.Status = USBD_STATUS_SUCCESS;
                    Status = STATUS_SUCCESS;
                    Information = UsbString->bLength;
                    break;
                }
                default:
                {
                    DPRINT1("Descriptor Type %x not supported!\n", Urb->UrbControlDescriptorRequest.DescriptorType);
                }
            }
            break;
        }
        case URB_FUNCTION_SELECT_CONFIGURATION:
        {
            DPRINT1("Ehci: URB_FUNCTION_SELECT_CONFIGURATION\n");
            DPRINT1("Urb->UrbSelectConfiguration.ConfigurationHandle %x\n",Urb->UrbSelectConfiguration.ConfigurationHandle);
            break;
        }
        case URB_FUNCTION_CLASS_DEVICE:
        {
            DPRINT1("Ehci: URB_FUNCTION_CLASS_DEVICE %x\n",Urb->UrbControlVendorClassRequest.Request);
            switch (Urb->UrbControlVendorClassRequest.Request)
            {
                case USB_REQUEST_GET_DESCRIPTOR:
                {
                    DPRINT1("TransferFlags %x\n", Urb->UrbControlVendorClassRequest.TransferFlags);
                    DPRINT1("Urb->UrbControlVendorClassRequest.Value %x\n", Urb->UrbControlVendorClassRequest.Value);

                    switch (Urb->UrbControlVendorClassRequest.Value >> 8)
                    {
                        case USB_DEVICE_CLASS_AUDIO:
                        {
                            DPRINT1("USB_DEVICE_CLASS_AUDIO\n");
                            break;
                        }
                        case USB_DEVICE_CLASS_COMMUNICATIONS:
                        {
                            DPRINT1("USB_DEVICE_CLASS_COMMUNICATIONS\n");
                            break;
                        }
                        case USB_DEVICE_CLASS_HUMAN_INTERFACE:
                        {
                            DPRINT1("USB_DEVICE_CLASS_HUMAN_INTERFACE\n");
                            break;
                        }
                        case USB_DEVICE_CLASS_MONITOR:
                        {
                            DPRINT1("USB_DEVICE_CLASS_MONITOR\n");
                            break;
                        }
                        case USB_DEVICE_CLASS_PHYSICAL_INTERFACE:
                        {
                            DPRINT1("USB_DEVICE_CLASS_PHYSICAL_INTERFACE\n");
                            break;
                        }
                        case USB_DEVICE_CLASS_POWER:
                        {
                            DPRINT1("USB_DEVICE_CLASS_POWER\n");
                            break;
                        }
                        case USB_DEVICE_CLASS_PRINTER:
                        {
                            DPRINT1("USB_DEVICE_CLASS_PRINTER\n");
                            break;
                        }
                        case USB_DEVICE_CLASS_STORAGE:
                        {
                            DPRINT1("USB_DEVICE_CLASS_STORAGE\n");
                            break;
                        }
                        case USB_DEVICE_CLASS_RESERVED:
                            DPRINT1("Reserved!!!\n");
                        case USB_DEVICE_CLASS_HUB:
                        {
                            DPRINT1("USB_DEVICE_CLASS_HUB request\n");
                            break;
                        }
                        default:
                        {
                            DPRINT1("Unknown UrbControlVendorClassRequest Value\n");
                        }
                    }
                    break;
                }
                case USB_REQUEST_GET_STATUS:
                {
                    DPRINT1("DEVICE: USB_REQUEST_GET_STATUS for port %d\n", Urb->UrbControlVendorClassRequest.Index);
                    break;
                }
                default:
                {
                    DPRINT1("Unhandled URB request for class device\n");
                    //Urb->UrbHeader.Status = USBD_STATUS_INVALID_URB_FUNCTION;
                }
            }
            break;
        }
        case URB_FUNCTION_CLASS_OTHER:
        {
            DPRINT1("Ehci: URB_FUNCTION_CLASS_OTHER\n");
            switch (Urb->UrbControlVendorClassRequest.Request)
            {
                case USB_REQUEST_GET_STATUS:
                {
                    DPRINT1("OTHER: USB_REQUEST_GET_STATUS for port %d\n", Urb->UrbControlVendorClassRequest.Index);
                    break;
                }
                case USB_REQUEST_CLEAR_FEATURE:
                {
                    DPRINT1("USB_REQUEST_CLEAR_FEATURE Port %d, value %x\n", Urb->UrbControlVendorClassRequest.Index,
                        Urb->UrbControlVendorClassRequest.Value);
                    switch (Urb->UrbControlVendorClassRequest.Value)
                    {
                        case C_PORT_CONNECTION:
                            DPRINT1("Clearing Connect\n");
                            break;
                        case C_PORT_RESET:
                            DPRINT1("Clearing Reset\n");
                            break;
                        default:
                            DPRINT1("Unknown Value for Clear Feature %x \n", Urb->UrbControlVendorClassRequest.Value);
                            break;
                    }
                    break;
                }
                case USB_REQUEST_SET_FEATURE:
                {
                    DPRINT1("USB_REQUEST_SET_FEATURE Port %d, value %x\n", Urb->UrbControlVendorClassRequest.Index,
                        Urb->UrbControlVendorClassRequest.Value);

                    switch(Urb->UrbControlVendorClassRequest.Value)
                    {
                        case PORT_RESET:
                        {
                            DPRINT1("Port reset\n");
                            break;
                        }
                        case PORT_ENABLE:
                        {
                            DPRINT1("Unhandled Set Feature\n");
                            break;
                        }
                        default:
                        {
                            DPRINT1("Unknown Set Feature!\n");
                            break;
                        }
                    }
                    break;
                }
                case USB_REQUEST_SET_ADDRESS:
                {
                    DPRINT1("USB_REQUEST_SET_ADDRESS\n");
                    break;
                }
                case USB_REQUEST_GET_DESCRIPTOR:
                {
                    DPRINT1("USB_REQUEST_GET_DESCRIPTOR\n");
                    break;
                }
                case USB_REQUEST_SET_DESCRIPTOR:
                {
                    DPRINT1("USB_REQUEST_SET_DESCRIPTOR\n");
                    break;
                }
                case USB_REQUEST_GET_CONFIGURATION:
                {
                    DPRINT1("USB_REQUEST_GET_CONFIGURATION\n");
                    break;
                }
                case USB_REQUEST_SET_CONFIGURATION:
                {
                    DPRINT1("USB_REQUEST_SET_CONFIGURATION\n");
                    break;
                }
                case USB_REQUEST_GET_INTERFACE:
                {
                    DPRINT1("USB_REQUEST_GET_INTERFACE\n");
                    break;
                }
                case USB_REQUEST_SET_INTERFACE:
                {
                    DPRINT1("USB_REQUEST_SET_INTERFACE\n");
                    break;
                }
                case USB_REQUEST_SYNC_FRAME:
                {
                    DPRINT1("USB_REQUEST_SYNC_FRAME\n");
                    break;
                }
                default:
                {
                    DPRINT1("Unknown Function Class Unknown request\n");
                    break;
                }
            }
            break;
        }
        default:
        {
            DPRINT1("Ehci: Unhandled URB %x\n", Urb->UrbHeader.Function);
            //Urb->UrbHeader.Status = USBD_STATUS_INVALID_URB_FUNCTION;
        }
    }

    Irp->IoStatus.Information = Information;

    if (Status != STATUS_PENDING)
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}
