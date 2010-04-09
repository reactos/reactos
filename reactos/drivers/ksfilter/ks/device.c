/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/device.c
 * PURPOSE:         KS IKsDevice interface functions
 * PROGRAMMER:      Johannes Anderwald
 */


#include "priv.h"

NTSTATUS
NTAPI
IKsDevice_fnQueryInterface(
    IN IKsDevice * iface,
    REFIID refiid,
    PVOID* Output)
{
    PKSIDEVICE_HEADER This = (PKSIDEVICE_HEADER)CONTAINING_RECORD(iface, KSIDEVICE_HEADER, lpVtblIKsDevice);

    if (IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtblIKsDevice;
        _InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }

    return STATUS_NOT_SUPPORTED;
}

ULONG
NTAPI
IKsDevice_fnAddRef(
    IN IKsDevice * iface)
{
    PKSIDEVICE_HEADER This = (PKSIDEVICE_HEADER)CONTAINING_RECORD(iface, KSIDEVICE_HEADER, lpVtblIKsDevice);

    return InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IKsDevice_fnRelease(
    IN IKsDevice * iface)
{
    PKSIDEVICE_HEADER This = (PKSIDEVICE_HEADER)CONTAINING_RECORD(iface, KSIDEVICE_HEADER, lpVtblIKsDevice);

    InterlockedDecrement(&This->ref);

    return This->ref;
}



PKSDEVICE
NTAPI
IKsDevice_fnGetStruct(
    IN IKsDevice * iface)
{
    PKSIDEVICE_HEADER This = (PKSIDEVICE_HEADER)CONTAINING_RECORD(iface, KSIDEVICE_HEADER, lpVtblIKsDevice);

    return &This->KsDevice;
}

NTSTATUS
NTAPI
IKsDevice_fnInitializeObjectBag(
    IN IKsDevice * iface,
    IN PKSIOBJECT_BAG Bag,
    IN PRKMUTEX Mutex)
{
    PKSIDEVICE_HEADER This = (PKSIDEVICE_HEADER)CONTAINING_RECORD(iface, KSIDEVICE_HEADER, lpVtblIKsDevice);

    if (!Mutex)
    {
        /* use device mutex */
        Mutex = &This->BagMutex;
    }

    /* initialize object bag */
    Bag->BagMutex = Mutex;
    Bag->DeviceHeader = (PKSIDEVICE_HEADER)This;
    InitializeListHead(&Bag->ObjectList);

    /* insert bag into device list */
    InsertTailList(&This->ObjectBags, &Bag->Entry);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IKsDevice_fnAcquireDevice(
    IN IKsDevice * iface)
{
    PKSIDEVICE_HEADER This = (PKSIDEVICE_HEADER)CONTAINING_RECORD(iface, KSIDEVICE_HEADER, lpVtblIKsDevice);

    return KeWaitForSingleObject(&This->DeviceMutex, Executive, KernelMode, FALSE, NULL);
}

NTSTATUS
NTAPI
IKsDevice_fnReleaseDevice(
    IN IKsDevice * iface)
{
    PKSIDEVICE_HEADER This = (PKSIDEVICE_HEADER)CONTAINING_RECORD(iface, KSIDEVICE_HEADER, lpVtblIKsDevice);

    return KeReleaseMutex(&This->DeviceMutex, FALSE);
}

NTSTATUS
NTAPI
IKsDevice_fnGetAdapterObject(
    IN IKsDevice * iface,
    IN PADAPTER_OBJECT * Object,
    IN PULONG Unknown1,
    IN PULONG Unknown2)
{
    PKSIDEVICE_HEADER This = (PKSIDEVICE_HEADER)CONTAINING_RECORD(iface, KSIDEVICE_HEADER, lpVtblIKsDevice);

    *Object = This->AdapterObject;

    return STATUS_SUCCESS;

}

NTSTATUS
NTAPI
IKsDevice_fnAddPowerEntry(
    IN IKsDevice * iface,
    IN struct KSPOWER_ENTRY * Entry,
    IN IKsPowerNotify* Notify)
{
    //PKSIDEVICE_HEADER This = (PKSIDEVICE_HEADER)CONTAINING_RECORD(iface, KSIDEVICE_HEADER, lpVtblIKsDevice);

    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IKsDevice_fnRemovePowerEntry(
    IN IKsDevice * iface,
    IN struct KSPOWER_ENTRY * Entry)
{
    //PKSIDEVICE_HEADER This = (PKSIDEVICE_HEADER)CONTAINING_RECORD(iface, KSIDEVICE_HEADER, lpVtblIKsDevice);

    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;

}

NTSTATUS
NTAPI
IKsDevice_fnPinStateChange(
    IN IKsDevice * iface,
    IN KSPIN Pin,
    IN PIRP Irp,
    IN KSSTATE OldState,
    IN KSSTATE NewState)
{
    //PKSIDEVICE_HEADER This = (PKSIDEVICE_HEADER)CONTAINING_RECORD(iface, KSIDEVICE_HEADER, lpVtblIKsDevice);

    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;

}

NTSTATUS
NTAPI
IKsDevice_fnArbitrateAdapterChannel(
    IN IKsDevice * iface,
    IN ULONG NumberOfMapRegisters,
    IN PDRIVER_CONTROL ExecutionRoutine,
    IN PVOID Context)
{
    PKSIDEVICE_HEADER This = (PKSIDEVICE_HEADER)CONTAINING_RECORD(iface, KSIDEVICE_HEADER, lpVtblIKsDevice);
    NTSTATUS Status;

    DPRINT("IKsDevice_fnArbitrateAdapterChannel NumberOfMapRegisters %lu ExecutionRoutine %p Context %p Irql %lu\n", NumberOfMapRegisters, ExecutionRoutine, Context, KeGetCurrentIrql());

    /* sanity check */
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(This->AdapterObject);

    /* allocate adapter channel */
    Status = IoAllocateAdapterChannel(This->AdapterObject, This->KsDevice.FunctionalDeviceObject, NumberOfMapRegisters, ExecutionRoutine, Context);

    /* done */
    return Status;
}

NTSTATUS
NTAPI
IKsDevice_fnCheckIoCapability(
    IN IKsDevice * iface,
    IN ULONG Unknown)
{
    //PKSIDEVICE_HEADER This = (PKSIDEVICE_HEADER)CONTAINING_RECORD(iface, KSIDEVICE_HEADER, lpVtblIKsDevice);

    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

static IKsDeviceVtbl vt_IKsDevice = 
{
    IKsDevice_fnQueryInterface,
    IKsDevice_fnAddRef,
    IKsDevice_fnRelease,
    IKsDevice_fnGetStruct,
    IKsDevice_fnInitializeObjectBag,
    IKsDevice_fnAcquireDevice,
    IKsDevice_fnReleaseDevice,
    IKsDevice_fnGetAdapterObject,
    IKsDevice_fnAddPowerEntry,
    IKsDevice_fnRemovePowerEntry,
    IKsDevice_fnPinStateChange,
    IKsDevice_fnArbitrateAdapterChannel,
    IKsDevice_fnCheckIoCapability
};


VOID
NTAPI
IKsDevice_PnpPostStart(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PVOID  Context)
{
    NTSTATUS Status;
    PPNP_POSTSTART_CONTEXT Ctx = (PPNP_POSTSTART_CONTEXT)Context;

    /* call driver pnp post routine */
    Status = Ctx->DeviceHeader->KsDevice.Descriptor->Dispatch->PostStart(&Ctx->DeviceHeader->KsDevice);

    if (!NT_SUCCESS(Status))
    {
        /* set state to disabled */
        Ctx->DeviceHeader->TargetState  = KSTARGET_STATE_DISABLED;
    }
    else
    {
        /* set state to enabled */
        Ctx->DeviceHeader->TargetState = KSTARGET_STATE_ENABLED;
        Status = KspSetFilterFactoriesState(Ctx->DeviceHeader, TRUE);
    }

    /* free work item */
    IoFreeWorkItem(Ctx->WorkItem);

    /* free work context */
    FreeItem(Ctx);

    DPRINT("IKsDevice_PnpPostStart: PostStart Routine returned %x\n", Status);
}

NTSTATUS
NTAPI
IKsDevice_PnpStartDevice(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PDEVICE_EXTENSION DeviceExtension;
    PKSIDEVICE_HEADER DeviceHeader;
    PPNP_POSTSTART_CONTEXT Ctx = NULL;
    NTSTATUS Status;
    PCM_RESOURCE_LIST TranslatedResourceList;
    PCM_RESOURCE_LIST UntranslatedResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor, UnPartialDescriptor;
    ULONG Index;

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    /* get device extension */
    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    /* get device header */
    DeviceHeader = DeviceExtension->DeviceHeader;

    DPRINT("IKsDevice_PnpStartDevice DeviceHeader %p\n", DeviceHeader);

    /* first forward irp to lower device object */
    Status = KspForwardIrpSynchronous(DeviceObject, Irp);

    /* check for success */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NextDevice object failed to start with %x\n", Status);
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    TranslatedResourceList = IoStack->Parameters.StartDevice.AllocatedResourcesTranslated;
    UntranslatedResourceList = IoStack->Parameters.StartDevice.AllocatedResources;

    DPRINT("ResourceDescriptorCount %lu\n", TranslatedResourceList->List[0].PartialResourceList.Count);
    for (Index = 0; Index < TranslatedResourceList->List[0].PartialResourceList.Count; Index ++ )
    {
        PartialDescriptor = &TranslatedResourceList->List[0].PartialResourceList.PartialDescriptors[Index];
        UnPartialDescriptor = &UntranslatedResourceList->List[0].PartialResourceList.PartialDescriptors[Index];
        DPRINT("Descriptor Type %u\n", PartialDescriptor->Type);

        if (PartialDescriptor->Type == CmResourceTypeInterrupt)
        {
            DPRINT("CmResourceTypeInterrupt Index %u TRANS   Interrupt Number Affinity %x Level %u Vector %u Flags %x Share %x\n", Index, PartialDescriptor->u.Interrupt.Affinity, PartialDescriptor->u.Interrupt.Level, PartialDescriptor->u.Interrupt.Vector, PartialDescriptor->Flags, PartialDescriptor->ShareDisposition);
            DPRINT("CmResourceTypeInterrupt Index %u UNTRANS Interrupt Number Affinity %x Level %u Vector %u Flags %x Share %x\\n", Index, UnPartialDescriptor->u.Interrupt.Affinity, UnPartialDescriptor->u.Interrupt.Level, UnPartialDescriptor->u.Interrupt.Vector, UnPartialDescriptor->Flags, UnPartialDescriptor->ShareDisposition);

        }
        else if (PartialDescriptor->Type == CmResourceTypePort)
        {
            DPRINT("CmResourceTypePort Index %u TRANS    Port Length %u Start %u %u Flags %x Share %x\n", Index, PartialDescriptor->u.Port.Length, PartialDescriptor->u.Port.Start.HighPart, PartialDescriptor->u.Port.Start.LowPart, PartialDescriptor->Flags, PartialDescriptor->ShareDisposition);
            DPRINT("CmResourceTypePort Index %u UNTRANS  Port Length %u Start %u %u Flags %x Share %x\n", Index, UnPartialDescriptor->u.Port.Length, UnPartialDescriptor->u.Port.Start.HighPart, UnPartialDescriptor->u.Port.Start.LowPart, UnPartialDescriptor->Flags, UnPartialDescriptor->ShareDisposition);
        }
        else if (PartialDescriptor->Type == CmResourceTypeMemory)
        {
            DPRINT("CmResourceTypeMemory Index %u TRANS  Start %x Length %u Flags %x Share %x\n", Index, PartialDescriptor->u.Memory.Start.LowPart, PartialDescriptor->u.Memory.Length, PartialDescriptor->Flags, PartialDescriptor->ShareDisposition);
            DPRINT("CmResourceTypeMemory Index %u TRANS  Start %x Length %u Flags %x Share %x\n", Index, UnPartialDescriptor->u.Memory.Start.LowPart, UnPartialDescriptor->u.Memory.Length, UnPartialDescriptor->Flags, UnPartialDescriptor->ShareDisposition);
        }
    }

    ASSERT(DeviceHeader->KsDevice.Descriptor);
    ASSERT(DeviceHeader->KsDevice.Descriptor->Dispatch);
    ASSERT(DeviceHeader->KsDevice.Descriptor->Dispatch->Start);


    /* do we have a device descriptor */
    if (DeviceHeader->KsDevice.Descriptor)
    {
        /* does the device want pnp notifications */
        if (DeviceHeader->KsDevice.Descriptor->Dispatch)
        {
            /* does the driver care about IRP_MN_START_DEVICE */
            if (DeviceHeader->KsDevice.Descriptor->Dispatch->Start)
            {
                /* call driver start device routine */
                Status = DeviceHeader->KsDevice.Descriptor->Dispatch->Start(&DeviceHeader->KsDevice, Irp,
                                                                   TranslatedResourceList,
                                                                   UntranslatedResourceList);


                DPRINT("IKsDevice_PnpStartDevice Start %p, Context %p\n", DeviceHeader->KsDevice.Descriptor->Dispatch->Start, DeviceHeader->KsDevice.Context);
                ASSERT(Status != STATUS_PENDING);

                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("Driver: failed to start %x\n", Status);
                    Irp->IoStatus.Status = Status;
                    IoCompleteRequest(Irp, IO_NO_INCREMENT);
                    return Status;
                }

                /* set state to run */
                DeviceHeader->KsDevice.Started = TRUE;

            }

            /* does the driver need post start routine */
            if (DeviceHeader->KsDevice.Descriptor->Dispatch->PostStart)
            {
                /* allocate pnp post workitem context */
                Ctx = (PPNP_POSTSTART_CONTEXT)AllocateItem(NonPagedPool, sizeof(PNP_POSTSTART_CONTEXT));
                if (!Ctx)
                {
                    /* no memory */
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
                else
                {
                    /* allocate a work item */
                    Ctx->WorkItem = IoAllocateWorkItem(DeviceObject);

                    if (!Ctx->WorkItem)
                    {
                         /* no memory */
                        FreeItem(Ctx);
                        Ctx = NULL;
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                    else
                    {
                        /* store device header for post-start pnp processing */
                        Ctx->DeviceHeader = DeviceHeader;
                    }
                }
            }
            else
            {
                /* set state to enabled, IRP_MJ_CREATE request may now succeed */
                DeviceHeader->TargetState = KSTARGET_STATE_ENABLED;
                Status = KspSetFilterFactoriesState(DeviceHeader, TRUE);
            }
        }
    }

    /* store result */
    Irp->IoStatus.Status = Status;
    /* complete request */
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    if (Ctx)
    {
        /* queue a work item for driver post start routine */
        IoQueueWorkItem(Ctx->WorkItem, IKsDevice_PnpPostStart, DelayedWorkQueue, (PVOID)Ctx);
    }

    /* return result */
    DPRINT1("IKsDevice_PnpStartDevice Status %x PostStartRoutine %p\n", Status, Ctx);
    return Status;
}

NTSTATUS
NTAPI
IKsDevice_Pnp(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PDEVICE_EXTENSION DeviceExtension;
    PKSIDEVICE_HEADER DeviceHeader;
    PKSDEVICE_DISPATCH Dispatch = NULL;
    NTSTATUS Status;

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* get device extension */
    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    /* get device header */
    DeviceHeader = DeviceExtension->DeviceHeader;

    /* do we have a device descriptor */
    if (DeviceHeader->KsDevice.Descriptor && DeviceHeader->KsDevice.Descriptor->Dispatch)
    {
        /* does the device want pnp notifications */
        Dispatch = (PKSDEVICE_DISPATCH)DeviceHeader->KsDevice.Descriptor->Dispatch;
    }

    switch (IoStack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
        {
            return IKsDevice_PnpStartDevice(DeviceObject, Irp);
        }

        case IRP_MN_QUERY_STOP_DEVICE:
        {
            Status = STATUS_SUCCESS;
            /* check for pnp notification support */
            if (Dispatch)
            {
                /* check for query stop support */
                if (Dispatch->QueryStop)
                {
                    /* call driver's query stop */
                    Status = Dispatch->QueryStop(&DeviceHeader->KsDevice, Irp);
                    ASSERT(Status != STATUS_PENDING);
                }
            }

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Driver: query stop failed %x\n", Status);
                Irp->IoStatus.Status = Status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return Status;
            }

            /* pass the irp down the driver stack */
            Status = KspForwardIrpSynchronous(DeviceObject, Irp);

            DPRINT("Next Device: Status %x\n", Status);

            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }

        case IRP_MN_REMOVE_DEVICE:
        {
            /* Clean up */
            if (Dispatch)
            {
                /* check for remove support */
                if (Dispatch->Remove)
                {
                    /* call driver's stop routine */
                    Dispatch->Remove(&DeviceHeader->KsDevice, Irp);
                }
            }

            /* pass the irp down the driver stack */
            Status = KspForwardIrpSynchronous(DeviceObject, Irp);

            DPRINT("Next Device: Status %x\n", Status);

            /* FIXME delete device resources */


            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }
        case IRP_MN_QUERY_INTERFACE:
        {
            Status = STATUS_UNSUCCESSFUL;
            /* check for pnp notification support */
            if (Dispatch)
            {
                /* check for query interface support */
                if (Dispatch->QueryInterface)
                {
                    /* call driver's query interface */
                    Status = Dispatch->QueryInterface(&DeviceHeader->KsDevice, Irp);
                    ASSERT(Status != STATUS_PENDING);
                }
            }

            if (NT_SUCCESS(Status))
            {
                /* driver supports a private interface */
                DPRINT1("IRP_MN_QUERY_INTERFACE Device supports interface\n");
                Irp->IoStatus.Status = Status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return Status;
            }

            /* pass the irp down the driver stack */
            Status = KspForwardIrpSynchronous(DeviceObject, Irp);

            DPRINT1("IRP_MN_QUERY_INTERFACE Next Device: Status %x\n", Status);
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }
        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            /* pass the irp down the driver stack */
            Status = KspForwardIrpSynchronous(DeviceObject, Irp);

            DPRINT("IRP_MN_QUERY_DEVICE_RELATIONS Next Device: Status %x\n", Status);

            //Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }
        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
        {
            /* pass the irp down the driver stack */
            //Status = KspForwardIrpSynchronous(DeviceObject, Irp);
            Status = Irp->IoStatus.Status;
            DPRINT("IRP_MN_FILTER_RESOURCE_REQUIREMENTS Next Device: Status %x\n", Status);

            //Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }
       case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
       {
            /* pass the irp down the driver stack */
            Status = KspForwardIrpSynchronous(DeviceObject, Irp);

            DPRINT("IRP_MN_QUERY_RESOURCE_REQUIREMENTS Next Device: Status %x\n", Status);

            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
       }
       default:
          DPRINT1("unhandled function %u\n", IoStack->MinorFunction);
          /* pass the irp down the driver stack */
          Status = KspForwardIrpSynchronous(DeviceObject, Irp);

          Irp->IoStatus.Status = Status;
          IoCompleteRequest(Irp, IO_NO_INCREMENT);
          return Status;
    }
}

NTSTATUS
NTAPI
IKsDevice_Power(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP Irp)
{
    UNIMPLEMENTED

    /* TODO */

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IKsDevice_Create(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP Irp)
{
    PCREATE_ITEM_ENTRY CreateItemEntry;
    PIO_STACK_LOCATION IoStack;
    PDEVICE_EXTENSION DeviceExtension;
    PKSIDEVICE_HEADER DeviceHeader;
    PKSIOBJECT_HEADER ObjectHeader;
    NTSTATUS Status;

    DPRINT("IKsDevice_Create\n");
    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    /* get device extension */
    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
    /* get device header */
    DeviceHeader = DeviceExtension->DeviceHeader;

    /* acquire list lock */
    IKsDevice_fnAcquireDevice((IKsDevice*)&DeviceHeader->lpVtblIKsDevice);

    /* sanity check */
    ASSERT(IoStack->FileObject);

    /* check if the request is relative */
    if (IoStack->FileObject->RelatedFileObject != NULL)
    {
        /* request is to instantiate a pin / node / clock / allocator */
        ObjectHeader = (PKSIOBJECT_HEADER)IoStack->FileObject->RelatedFileObject->FsContext2;

        /* sanity check */
        ASSERT(ObjectHeader);

        /* find a matching a create item */
        Status = FindMatchingCreateItem(&ObjectHeader->ItemList, IoStack->FileObject->FileName.Length, IoStack->FileObject->FileName.Buffer, &CreateItemEntry);
    }
    else
    {
        /* request to create a filter */
        Status = FindMatchingCreateItem(&DeviceHeader->ItemList, IoStack->FileObject->FileName.Length, IoStack->FileObject->FileName.Buffer, &CreateItemEntry);
    }

    if (NT_SUCCESS(Status))
    {
        /* set object create item */
        KSCREATE_ITEM_IRP_STORAGE(Irp) = CreateItemEntry->CreateItem;

        /* call create function */
        Status = CreateItemEntry->CreateItem->Create(DeviceObject, Irp);

        if (NT_SUCCESS(Status))
        {
            /* increment create item reference count */
            InterlockedIncrement(&CreateItemEntry->ReferenceCount);
        }
    }

    /* acquire list lock */
    IKsDevice_fnReleaseDevice((IKsDevice*)&DeviceHeader->lpVtblIKsDevice);

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Information = 0;
        /* set return status */
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return Status;


}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsInitializeDevice(
    IN PDEVICE_OBJECT FunctionalDeviceObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PDEVICE_OBJECT NextDeviceObject,
    IN const KSDEVICE_DESCRIPTOR* Descriptor OPTIONAL)
{
    PDEVICE_EXTENSION DeviceExtension;
    PKSIDEVICE_HEADER Header;
    ULONG Index;
    PKSIOBJECT_BAG Bag;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("KsInitializeDevice Descriptor %p\n", Descriptor);

    /* get device extension */
    DeviceExtension = (PDEVICE_EXTENSION)FunctionalDeviceObject->DeviceExtension;

    /* first allocate device header */
    Status = KsAllocateDeviceHeader((KSDEVICE_HEADER*)&DeviceExtension->DeviceHeader, 0, NULL);

    /* point to allocated header */
    Header = DeviceExtension->DeviceHeader;

    DPRINT("DeviceHeader %p\n", DeviceExtension->DeviceHeader);

    if (Descriptor && Descriptor->Dispatch)
    {
        DPRINT("Descriptor Add %p\n", Descriptor->Dispatch->Add);
        DPRINT("Descriptor Start %p\n", Descriptor->Dispatch->Start);
        DPRINT("Descriptor PostStart %p\n", Descriptor->Dispatch->PostStart);
        DPRINT("Descriptor QueryStop %p\n", Descriptor->Dispatch->QueryStop);
        DPRINT("Descriptor CancelStop %p\n", Descriptor->Dispatch->CancelStop);
        DPRINT("Descriptor Stop %p\n", Descriptor->Dispatch->Stop);
        DPRINT("Descriptor QueryRemove %p\n", Descriptor->Dispatch->QueryRemove);
        DPRINT("Descriptor CancelRemove %p\n", Descriptor->Dispatch->CancelRemove);
        DPRINT("Descriptor Remove %p\n", Descriptor->Dispatch->Remove);
        DPRINT("Descriptor QueryCapabilities %p\n", Descriptor->Dispatch->QueryCapabilities);
        DPRINT("Descriptor SurpriseRemoval %p\n", Descriptor->Dispatch->SurpriseRemoval);
        DPRINT("Descriptor QueryPower %p\n", Descriptor->Dispatch->QueryPower);
        DPRINT("Descriptor SetPower %p\n", Descriptor->Dispatch->SetPower);
        DPRINT("Descriptor QueryInterface %p\n", Descriptor->Dispatch->QueryInterface);
    }

    /* check for success */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("KsInitializeDevice Failed to allocate device header with %x\n", Status);
        return Status;
    }

    /* initialize IKsDevice interface */
    Header->lpVtblIKsDevice = &vt_IKsDevice;
    Header->ref = 1;

    /* allocate object bag */
    Header->KsDevice.Bag = AllocateItem(NonPagedPool, sizeof(KSIOBJECT_BAG));
    if (!Header->KsDevice.Bag)
    {
        /* no memory */
        KsFreeDeviceHeader((KSDEVICE_HEADER*)&DeviceExtension->DeviceHeader);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* initialize object bag */
    KeInitializeMutex(&Header->BagMutex, 0);
    KeInitializeMutex(&Header->DeviceMutex, 0);

    Bag = (PKSIOBJECT_BAG)Header->KsDevice.Bag;
    Bag->BagMutex = &Header->BagMutex;
    InitializeListHead(&Header->ObjectBags);
    InitializeListHead(&Bag->ObjectList);
    Bag->DeviceHeader = (PVOID)Header;

    /* insert bag into device list */
    InsertTailList(&Header->ObjectBags, &Bag->Entry);

    /* initialize device header */
    Header->KsDevice.FunctionalDeviceObject = FunctionalDeviceObject;
    Header->KsDevice.PhysicalDeviceObject = PhysicalDeviceObject;
    Header->KsDevice.NextDeviceObject = NextDeviceObject;
    Header->KsDevice.Descriptor = Descriptor;
    Header->KsDevice.SystemPowerState = PowerSystemWorking;
    Header->KsDevice.DevicePowerState = PowerDeviceD0;
    Header->KsDevice.Started = FALSE;
    Header->KsDevice.Context = NULL;
    KsSetDevicePnpAndBaseObject(Header, PhysicalDeviceObject, NextDeviceObject);



    if (Descriptor)
    {
        /* create a filter factory for each filter descriptor */
        DPRINT("KsInitializeDevice FilterDescriptorCount %lu\n", Descriptor->FilterDescriptorsCount);
        for(Index = 0; Index < Descriptor->FilterDescriptorsCount; Index++)
        {
            Status = KspCreateFilterFactory(FunctionalDeviceObject, Descriptor->FilterDescriptors[Index], NULL, NULL, 0, NULL, NULL, NULL);

            DPRINT("KsInitializeDevice Index %lu KspCreateFilterFactory Status %lx\n", Index, Status);
            /* check for success */
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("KspCreateFilterFactory failed with %x\n", Status);
                /* FIXME memory leak */
                return Status;
            }
        }

        /* does the driver pnp notification */
        if (Descriptor->Dispatch)
        {
            /* does the driver care about the add device */
            Status = Descriptor->Dispatch->Add(&Header->KsDevice);

            DPRINT("Driver: AddHandler Status %x\n", Status);
            Header->KsDevice.Descriptor = Descriptor;
        }
    }


   return Status;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsReferenceSoftwareBusObject(
    IN KSDEVICE_HEADER  Header)
{
     IKsDevice * Device;
     PKSIDEVICE_HEADER DeviceHeader = (PKSIDEVICE_HEADER)Header;

     /* get device interface */
     Device = (IKsDevice*)DeviceHeader->lpVtblIKsDevice;

     if (Device)
     {
         /* reference device interface */
         Device->lpVtbl->AddRef(Device);
     }

    return STATUS_SUCCESS;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsReferenceBusObject(
    IN  KSDEVICE_HEADER Header)
{
     IKsDevice * Device;
     PKSIDEVICE_HEADER DeviceHeader = (PKSIDEVICE_HEADER)Header;

     /* get device interface */
     Device = (IKsDevice*)DeviceHeader->lpVtblIKsDevice;

     if (Device)
     {
         /* reference device interface */
         Device->lpVtbl->AddRef(Device);
     }

    return STATUS_SUCCESS;

}

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsDereferenceBusObject(
    IN  KSDEVICE_HEADER Header)
{
     IKsDevice * Device;
     PKSIDEVICE_HEADER DeviceHeader = (PKSIDEVICE_HEADER)Header;

     /* get device interface */
     Device = (IKsDevice*)DeviceHeader->lpVtblIKsDevice;

     if (Device)
     {
         /* release device interface */
         Device->lpVtbl->Release(Device);
     }
}

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsDereferenceSoftwareBusObject(
    IN KSDEVICE_HEADER  Header)
{
     IKsDevice * Device;
     PKSIDEVICE_HEADER DeviceHeader = (PKSIDEVICE_HEADER)Header;

     DPRINT1("KsDereferenceSoftwareBusObject DeviceHeader %p\n", Header);

     /* get device interface */
     Device = (IKsDevice*)DeviceHeader->lpVtblIKsDevice;

     if (Device)
     {
         /* release device interface */
         Device->lpVtbl->Release(Device);
     }
}
