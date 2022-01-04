/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbccgp/fdo.c
 * PURPOSE:     USB  device driver.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 *              Cameron Gutman
 */

#include "usbccgp.h"

#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
FDO_QueryCapabilitiesCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    /* Set event */
    KeSetEvent((PRKEVENT)Context, 0, FALSE);

    /* Completion is done in the HidClassFDO_QueryCapabilities routine */
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
FDO_QueryCapabilities(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PDEVICE_CAPABILITIES Capabilities)
{
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    PIO_STACK_LOCATION IoStack;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;

    /* Get device extension */
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(FDODeviceExtension->Common.IsFDO);

    /* Init event */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* Now allocate the irp */
    Irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (!Irp)
    {
        /* No memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Get next stack location */
    IoStack = IoGetNextIrpStackLocation(Irp);

    /* Init stack location */
    IoStack->MajorFunction = IRP_MJ_PNP;
    IoStack->MinorFunction = IRP_MN_QUERY_CAPABILITIES;
    IoStack->Parameters.DeviceCapabilities.Capabilities = Capabilities;

    /* Set completion routine */
    IoSetCompletionRoutine(Irp,
                           FDO_QueryCapabilitiesCompletionRoutine,
                           (PVOID)&Event,
                           TRUE,
                           TRUE,
                           TRUE);

    /* Init capabilities */
    RtlZeroMemory(Capabilities, sizeof(DEVICE_CAPABILITIES));
    Capabilities->Size = sizeof(DEVICE_CAPABILITIES);
    Capabilities->Version = 1; // FIXME hardcoded constant
    Capabilities->Address = MAXULONG;
    Capabilities->UINumber = MAXULONG;

    /* Pnp irps have default completion code */
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    /* Call lower device */
    Status = IoCallDriver(FDODeviceExtension->NextDeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        /* Wait for completion */
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
    }

    /* Get status */
    Status = Irp->IoStatus.Status;

    /* Complete request */
    IoFreeIrp(Irp);

    /* Done */
    return Status;
}

NTSTATUS
FDO_DeviceRelations(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    ULONG DeviceCount = 0;
    ULONG Index;
    PDEVICE_RELATIONS DeviceRelations;
    PIO_STACK_LOCATION IoStack;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;

    /* Get device extension */
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* Get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* Check if relation type is BusRelations */
    if (IoStack->Parameters.QueryDeviceRelations.Type != BusRelations)
    {
        /* FDO always only handles bus relations */
        return STATUS_SUCCESS;
    }

    /* Go through array and count device objects */
    for(Index = 0; Index < FDODeviceExtension->FunctionDescriptorCount; Index++)
    {
        if (FDODeviceExtension->ChildPDO[Index])
        {
            /* Child pdo */
            DeviceCount++;
        }
    }

    /* Allocate device relations */
    DeviceRelations = (PDEVICE_RELATIONS)AllocateItem(PagedPool,
                                                      sizeof(DEVICE_RELATIONS) + (DeviceCount > 1 ? (DeviceCount-1) * sizeof(PDEVICE_OBJECT) : 0));
    if (!DeviceRelations)
    {
        /* No memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Add device objects */
    for(Index = 0; Index < FDODeviceExtension->FunctionDescriptorCount; Index++)
    {
        if (FDODeviceExtension->ChildPDO[Index])
        {
            /* Store child pdo */
            DeviceRelations->Objects[DeviceRelations->Count] = FDODeviceExtension->ChildPDO[Index];

            /* Add reference */
            ObReferenceObject(FDODeviceExtension->ChildPDO[Index]);

            /* Increment count */
            DeviceRelations->Count++;
        }
    }

    /* Store result */
    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    /* Request completed successfully */
    return STATUS_SUCCESS;
}

NTSTATUS
FDO_CreateChildPdo(
    IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    PDEVICE_OBJECT PDODeviceObject;
    PPDO_DEVICE_EXTENSION PDODeviceExtension;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    ULONG Index;

    /* Get device extension */
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(FDODeviceExtension->Common.IsFDO);

    /* Lets create array for the child PDO */
    FDODeviceExtension->ChildPDO = AllocateItem(NonPagedPool,
                                                sizeof(PDEVICE_OBJECT) * FDODeviceExtension->FunctionDescriptorCount);
    if (!FDODeviceExtension->ChildPDO)
    {
        /* No memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Create pdo for each function */
    for(Index = 0; Index < FDODeviceExtension->FunctionDescriptorCount; Index++)
    {
        /* Create the PDO */
        Status = IoCreateDevice(FDODeviceExtension->DriverObject,
                                sizeof(PDO_DEVICE_EXTENSION),
                                NULL,
                                FILE_DEVICE_USB,
                                FILE_AUTOGENERATED_DEVICE_NAME,
                                FALSE,
                                &PDODeviceObject);
        if (!NT_SUCCESS(Status))
        {
            /* Failed to create device object */
            DPRINT1("IoCreateDevice failed with %x\n", Status);
            return Status;
        }

        /* Store in array */
        FDODeviceExtension->ChildPDO[Index] = PDODeviceObject;

        /* Get device extension */
        PDODeviceExtension = (PPDO_DEVICE_EXTENSION)PDODeviceObject->DeviceExtension;
        RtlZeroMemory(PDODeviceExtension, sizeof(PDO_DEVICE_EXTENSION));

        /* Init device extension */
        PDODeviceExtension->Common.IsFDO = FALSE;
        PDODeviceExtension->FunctionDescriptor = &FDODeviceExtension->FunctionDescriptor[Index];
        PDODeviceExtension->NextDeviceObject = DeviceObject;
        PDODeviceExtension->FunctionIndex = Index;
        PDODeviceExtension->FDODeviceExtension = FDODeviceExtension;
        PDODeviceExtension->InterfaceList = FDODeviceExtension->InterfaceList;
        PDODeviceExtension->InterfaceListCount = FDODeviceExtension->InterfaceListCount;
        PDODeviceExtension->ConfigurationHandle = FDODeviceExtension->ConfigurationHandle;
        PDODeviceExtension->ConfigurationDescriptor = FDODeviceExtension->ConfigurationDescriptor;
        RtlCopyMemory(&PDODeviceExtension->Capabilities, &FDODeviceExtension->Capabilities, sizeof(DEVICE_CAPABILITIES));
        RtlCopyMemory(&PDODeviceExtension->DeviceDescriptor, FDODeviceExtension->DeviceDescriptor, sizeof(USB_DEVICE_DESCRIPTOR));

        /* Patch the stack size */
        PDODeviceObject->StackSize = DeviceObject->StackSize + 1;

        /* Set device flags */
        PDODeviceObject->Flags |= DO_DIRECT_IO | DO_MAP_IO_BUFFER;

        /* Device is initialized */
        PDODeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    }

    /* Done */
    return STATUS_SUCCESS;
}

NTSTATUS
FDO_StartDevice(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    NTSTATUS Status;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;

    /* Get device extension */
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(FDODeviceExtension->Common.IsFDO);

    /* First start lower device */
    if (IoForwardIrpSynchronously(FDODeviceExtension->NextDeviceObject, Irp))
    {
        Status = Irp->IoStatus.Status;
    }
    else
    {
        Status = STATUS_UNSUCCESSFUL;
    }

    if (!NT_SUCCESS(Status))
    {
        /* Failed to start lower device */
        DPRINT1("FDO_StartDevice lower device failed to start with %x\n", Status);
        return Status;
    }

    /* Get descriptors */
    Status = USBCCGP_GetDescriptors(DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        /* Failed to start lower device */
        DPRINT1("FDO_StartDevice failed to get descriptors with %x\n", Status);
        return Status;
    }

    /* Get capabilities */
    Status = FDO_QueryCapabilities(DeviceObject,
                                   &FDODeviceExtension->Capabilities);
    if (!NT_SUCCESS(Status))
    {
        /* Failed to start lower device */
        DPRINT1("FDO_StartDevice failed to get capabilities with %x\n", Status);
        return Status;
    }

    /* Now select the configuration */
    Status = USBCCGP_SelectConfiguration(DeviceObject, FDODeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        /* Failed to select interface */
        DPRINT1("FDO_StartDevice failed to get capabilities with %x\n", Status);
        return Status;
    }

    /* Query bus interface */
    USBCCGP_QueryInterface(FDODeviceExtension->NextDeviceObject,
                           &FDODeviceExtension->BusInterface);

    /* Now enumerate the functions */
    Status = USBCCGP_EnumerateFunctions(DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        /* Failed to enumerate functions */
        DPRINT1("Failed to enumerate functions with %x\n", Status);
        return Status;
    }

    /* Sanity checks */
    ASSERT(FDODeviceExtension->FunctionDescriptorCount);
    ASSERT(FDODeviceExtension->FunctionDescriptor);
    DumpFunctionDescriptor(FDODeviceExtension->FunctionDescriptor,
                           FDODeviceExtension->FunctionDescriptorCount);

    /* Now create the pdo */
    Status = FDO_CreateChildPdo(DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        /* Failed */
        DPRINT1("FDO_CreateChildPdo failed with %x\n", Status);
        return Status;
    }

    /* Inform pnp manager of new device objects */
    IoInvalidateDeviceRelations(FDODeviceExtension->PhysicalDeviceObject,
                                BusRelations);

    /* Done */
    DPRINT("[USBCCGP] FDO initialized successfully\n");
    return Status;
}

NTSTATUS
FDO_CloseConfiguration(
    IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    PURB Urb;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;

    /* Get device extension */
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(FDODeviceExtension->Common.IsFDO);

    /* Nothing to do if we're not configured */
    if (FDODeviceExtension->ConfigurationDescriptor == NULL ||
        FDODeviceExtension->InterfaceList == NULL)
    {
        return STATUS_SUCCESS;
    }

    /* Now allocate the urb */
    Urb = USBD_CreateConfigurationRequestEx(FDODeviceExtension->ConfigurationDescriptor,
                                            FDODeviceExtension->InterfaceList);
    if (!Urb)
    {
        /* No memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Clear configuration descriptor to make it an unconfigure request */
    Urb->UrbSelectConfiguration.ConfigurationDescriptor = NULL;

    /* Submit urb */
    Status = USBCCGP_SyncUrbRequest(FDODeviceExtension->NextDeviceObject, Urb);
    if (!NT_SUCCESS(Status))
    {
        /* Failed to set configuration */
        DPRINT1("USBCCGP_SyncUrbRequest failed to unconfigure device\n", Status);
    }

    ExFreePool(Urb);
    return Status;
}


NTSTATUS
FDO_HandlePnp(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;

    /* Get device extension */
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(FDODeviceExtension->Common.IsFDO);


    /* Get stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    DPRINT("[USBCCGP] PnP Minor %x\n", IoStack->MinorFunction);
    switch(IoStack->MinorFunction)
    {
        case IRP_MN_REMOVE_DEVICE:
        {
            // Unconfigure device */
            DPRINT1("[USBCCGP] FDO IRP_MN_REMOVE\n");
            FDO_CloseConfiguration(DeviceObject);

            /* Send the IRP down the stack */
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(FDODeviceExtension->NextDeviceObject, Irp);

            /* Detach from the device stack */
            IoDetachDevice(FDODeviceExtension->NextDeviceObject);

            /* Delete the device object */
            IoDeleteDevice(DeviceObject);

            /* Request completed */
            break;
        }
        case IRP_MN_START_DEVICE:
        {
            /* Start the device */
            Status = FDO_StartDevice(DeviceObject, Irp);
            break;
        }
        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            /* Handle device relations */
            Status = FDO_DeviceRelations(DeviceObject, Irp);
            if (!NT_SUCCESS(Status))
            {
                break;
            }

            /* Forward irp to next device object */
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(FDODeviceExtension->NextDeviceObject, Irp);
        }
        case IRP_MN_QUERY_CAPABILITIES:
        {
            /* Copy capabilities */
            RtlCopyMemory(IoStack->Parameters.DeviceCapabilities.Capabilities,
                          &FDODeviceExtension->Capabilities,
                          sizeof(DEVICE_CAPABILITIES));
            Status = STATUS_UNSUCCESSFUL;

            if (IoForwardIrpSynchronously(FDODeviceExtension->NextDeviceObject, Irp))
            {
                Status = Irp->IoStatus.Status;
                if (NT_SUCCESS(Status))
                {
                    IoStack->Parameters.DeviceCapabilities.Capabilities->SurpriseRemovalOK = TRUE;
                }
            }
            break;
       }
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_QUERY_STOP_DEVICE:
        {
            /* Sure */
            Irp->IoStatus.Status = STATUS_SUCCESS;

            /* Forward irp to next device object */
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(FDODeviceExtension->NextDeviceObject, Irp);
        }
       default:
       {
            /* Forward irp to next device object */
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(FDODeviceExtension->NextDeviceObject, Irp);
       }

    }

    /* Complete request */
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
FDO_HandleResetCyclePort(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;
    PLIST_ENTRY ListHead, Entry;
    LIST_ENTRY TempList;
    PUCHAR ResetActive;
    PIRP ListIrp;
    KIRQL OldLevel;

    /* Get device extension */
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(FDODeviceExtension->Common.IsFDO);

    /* Get stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    DPRINT("FDO_HandleResetCyclePort IOCTL %x\n", IoStack->Parameters.DeviceIoControl.IoControlCode);

    if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_INTERNAL_USB_RESET_PORT)
    {
        /* Use reset port list */
        ListHead = &FDODeviceExtension->ResetPortListHead;
        ResetActive = &FDODeviceExtension->ResetPortActive;
    }
    else
    {
        /* Use cycle port list */
        ListHead = &FDODeviceExtension->CyclePortListHead;
        ResetActive = &FDODeviceExtension->CyclePortActive;
    }

    /* Acquire lock */
    KeAcquireSpinLock(&FDODeviceExtension->Lock, &OldLevel);

    if (*ResetActive)
    {
        /* Insert into pending list */
        InsertTailList(ListHead, &Irp->Tail.Overlay.ListEntry);

        /* Mark irp pending */
        IoMarkIrpPending(Irp);
        Status = STATUS_PENDING;

        /* Release lock */
        KeReleaseSpinLock(&FDODeviceExtension->Lock, OldLevel);
    }
    else
    {
        /* Mark reset active */
        *ResetActive = TRUE;

        /* Release lock */
        KeReleaseSpinLock(&FDODeviceExtension->Lock, OldLevel);

        /* Forward request synchronized */
        NT_VERIFY(IoForwardIrpSynchronously(FDODeviceExtension->NextDeviceObject, Irp));

        /* Reacquire lock */
        KeAcquireSpinLock(&FDODeviceExtension->Lock, &OldLevel);

        /* Mark reset as completed */
        *ResetActive = FALSE;

        /* Move all requests into temporary list */
        InitializeListHead(&TempList);
        while(!IsListEmpty(ListHead))
        {
            Entry = RemoveHeadList(ListHead);
            InsertTailList(&TempList, Entry);
        }

        /* Release lock */
        KeReleaseSpinLock(&FDODeviceExtension->Lock, OldLevel);

        /* Complete pending irps */
        while(!IsListEmpty(&TempList))
        {
            Entry = RemoveHeadList(&TempList);
            ListIrp = (PIRP)CONTAINING_RECORD(Entry, IRP, Tail.Overlay.ListEntry);

            /* Complete request with status success */
            ListIrp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(ListIrp, IO_NO_INCREMENT);
        }

        /* Status success */
        Status = STATUS_SUCCESS;
    }

    return Status;
}



NTSTATUS
FDO_HandleInternalDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;

    /* Get device extension */
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(FDODeviceExtension->Common.IsFDO);

    /* Get stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_INTERNAL_USB_RESET_PORT ||
        IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_INTERNAL_USB_CYCLE_PORT)
    {
        /* Handle reset / cycle ports */
        Status = FDO_HandleResetCyclePort(DeviceObject, Irp);
        DPRINT("FDO_HandleResetCyclePort Status %x\n", Status);
        if (Status != STATUS_PENDING)
        {
            /* Complete request */
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }
        return Status;
    }

    /* Forward and forget request */
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(FDODeviceExtension->NextDeviceObject, Irp);
}

NTSTATUS
FDO_HandleSystemControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PFDO_DEVICE_EXTENSION FDODeviceExtension;

    /* Get device extension */
    FDODeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(FDODeviceExtension->Common.IsFDO);

    /* Forward and forget request */
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(FDODeviceExtension->NextDeviceObject, Irp);
}

NTSTATUS
FDO_Dispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    PFDO_DEVICE_EXTENSION FDODeviceExtension;

    /* Get device extension */
    FDODeviceExtension = DeviceObject->DeviceExtension;
    ASSERT(FDODeviceExtension->Common.IsFDO);

    /* Get stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    switch(IoStack->MajorFunction)
    {
        case IRP_MJ_PNP:
            return FDO_HandlePnp(DeviceObject, Irp);
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
            return FDO_HandleInternalDeviceControl(DeviceObject, Irp);
        case IRP_MJ_POWER:
            PoStartNextPowerIrp(Irp);
            IoSkipCurrentIrpStackLocation(Irp);
            return PoCallDriver(FDODeviceExtension->NextDeviceObject, Irp);
        case IRP_MJ_SYSTEM_CONTROL:
            return FDO_HandleSystemControl(DeviceObject, Irp);
        default:
            DPRINT1("FDO_Dispatch Function %x not implemented\n", IoStack->MajorFunction);
            ASSERT(FALSE);
            Status = Irp->IoStatus.Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
    }

}
