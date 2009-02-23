/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/sysaudio/control.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Johannes Anderwald
 */

#include <ntifs.h>
#include <ntddk.h>
#include <portcls.h>
#include <ks.h>
#include <ksmedia.h>
#include <math.h>
#define YDEBUG
#include <debug.h>
#include "sysaudio.h"

const GUID KSPROPSETID_Sysaudio                 = {0xCBE3FAA0L, 0xCC75, 0x11D0, {0xB4, 0x65, 0x00, 0x00, 0x1A, 0x18, 0x18, 0xE6}};
const GUID KSPROPSETID_Sysaudio_Pin             = {0xA3A53220L, 0xC6E4, 0x11D0, {0xB4, 0x65, 0x00, 0x00, 0x1A, 0x18, 0x18, 0xE6}};
const GUID KSPROPSETID_General                  = {0x1464EDA5L, 0x6A8F, 0x11D1, {0x9A, 0xA7, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSPROPSETID_Pin                     = {0x8C134960L, 0x51AD, 0x11CF, {0x87, 0x8A, 0x94, 0xF8, 0x01, 0xC1, 0x00, 0x00}};
const GUID KSPROPSETID_Connection              = {0x1D58C920L, 0xAC9B, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};


NTSTATUS
SetIrpIoStatus(
    IN PIRP Irp,
    IN NTSTATUS Status,
    IN ULONG Length)
{
    Irp->IoStatus.Information = Length;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;

}

PKSAUDIO_DEVICE_ENTRY
GetListEntry(
    IN PLIST_ENTRY Head,
    IN ULONG Index)
{
    PLIST_ENTRY Entry = Head->Flink;

    while(Index-- && Entry != Head)
        Entry = Entry->Flink;

    if (Entry == Head)
        return NULL;

    return (PKSAUDIO_DEVICE_ENTRY)CONTAINING_RECORD(Entry, KSAUDIO_DEVICE_ENTRY, Entry);
}

NTSTATUS
SysAudioOpenVirtualDevice(
    IN PIRP Irp,
    IN ULONG DeviceNumber,
    PSYSAUDIODEVEXT DeviceExtension)
{
    PULONG Index;
    PHANDLE Handle;
    ULONG Count;
    PSYSAUDIO_CLIENT ClientInfo;
    PKSAUDIO_DEVICE_ENTRY Entry;
    PKSOBJECT_CREATE_ITEM CreateItem;

    /* access the create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);
    ASSERT(CreateItem);

    if (DeviceNumber >= DeviceExtension->NumberOfKsAudioDevices)
    {
        /* invalid device index */
        return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
    }

    /* get device context */
    Entry = GetListEntry(&DeviceExtension->KsAudioDeviceList, DeviceNumber);
    ASSERT(Entry != NULL);

    /* get client context */
    ClientInfo = (PSYSAUDIO_CLIENT)CreateItem->Context;
    /* does the client already use a device */
    if (!ClientInfo->NumDevices)
    {
        /* first device to be openend */
        ClientInfo->Devices = ExAllocatePool(NonPagedPool, sizeof(ULONG));
        if (!ClientInfo->Devices)
        {
            /* no memory */
            return SetIrpIoStatus(Irp, STATUS_NO_MEMORY, 0);
        }

        ClientInfo->Handels = ExAllocatePool(NonPagedPool, sizeof(HANDLE));
        if (!ClientInfo->Devices)
        {
            /* no memory */
            return SetIrpIoStatus(Irp, STATUS_NO_MEMORY, 0);
        }

        ClientInfo->NumDevices = 1;
        ClientInfo->Devices[0] = DeviceNumber;
        ClientInfo->Handels[0] = NULL;
        /* increase usage count */
        Entry->NumberOfClients++;
        return SetIrpIoStatus(Irp, STATUS_SUCCESS, 0);
    }

    /* check if device has already been openend */
    for(Count = 0; Count < ClientInfo->NumDevices; Count++)
    {
        if (ClientInfo->Devices[Count] == DeviceNumber)
        {
            /* device has already been opened */
            return SetIrpIoStatus(Irp, STATUS_SUCCESS, 0);
        }
    }
    /* new device to be openend */
    Index = ExAllocatePool(NonPagedPool, sizeof(ULONG) * (ClientInfo->NumDevices + 1));
    if (!Index)
    {
        /* no memory */
        return SetIrpIoStatus(Irp, STATUS_NO_MEMORY, 0);
    }

    Handle = ExAllocatePool(NonPagedPool, sizeof(HANDLE) * (ClientInfo->NumDevices + 1));
    if (!Handle)
    {
        /* no memory */
        ExFreePool(Index);
        return SetIrpIoStatus(Irp, STATUS_NO_MEMORY, 0);
    }

    /* increase usage count */
    Entry->NumberOfClients++;

    /* copy device count array */
    if (ClientInfo->NumDevices)
	{
        RtlMoveMemory(Index, ClientInfo->Devices, ClientInfo->NumDevices * sizeof(ULONG));
        RtlMoveMemory(Handle, ClientInfo->Handels, ClientInfo->NumDevices * sizeof(HANDLE));
	}

    Index[ClientInfo->NumDevices] = DeviceNumber;
    Handle[ClientInfo->NumDevices] = NULL;
	ExFreePool(ClientInfo->Handels);
    ExFreePool(ClientInfo->Devices);
    ClientInfo->NumDevices++;
    ClientInfo->Devices = Index;
	ClientInfo->Handels = Handle;

    return SetIrpIoStatus(Irp, STATUS_SUCCESS, 0);
}

VOID
NTAPI
CreatePinWorkerRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID  Context)
{
    NTSTATUS Status;
    HANDLE PinHandle;
    HANDLE Filter;
    PFILE_OBJECT FileObject;
    PPIN_WORKER_CONTEXT WorkerContext = (PPIN_WORKER_CONTEXT)Context;
    Filter = WorkerContext->PinConnect->PinToHandle;

    WorkerContext->PinConnect->PinToHandle = NULL;


    if (WorkerContext->CreateRealPin)
    {
        /* create the real pin */
        Status = KsCreatePin(WorkerContext->Entry->Handle, WorkerContext->PinConnect, GENERIC_READ | GENERIC_WRITE, &PinHandle);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to create Pin with %x\n", Status);
            SetIrpIoStatus(WorkerContext->Irp, STATUS_UNSUCCESSFUL, 0);
            ExFreePool(WorkerContext);
            return;
        }

        /* get pin file object */
        Status = ObReferenceObjectByHandle(PinHandle,
                                           GENERIC_READ | GENERIC_WRITE, 
                                           IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);

        WorkerContext->Entry->Pins[WorkerContext->PinConnect->PinId].PinHandle = PinHandle;
        WorkerContext->Entry->Pins[WorkerContext->PinConnect->PinId].References = 1;
        WorkerContext->DispatchContext->Handle = PinHandle;
        WorkerContext->DispatchContext->PinId = WorkerContext->PinConnect->PinId;
        WorkerContext->DispatchContext->AudioEntry = WorkerContext->Entry;

        if (NT_SUCCESS(Status))
            WorkerContext->DispatchContext->FileObject = FileObject;
        else
            WorkerContext->DispatchContext->FileObject = NULL;
    }
    else
    {
        WorkerContext->DispatchContext->AudioEntry = WorkerContext->Entry;
        WorkerContext->DispatchContext->Handle = WorkerContext->Entry->Pins[WorkerContext->PinConnect->PinId].PinHandle;
        WorkerContext->DispatchContext->PinId = WorkerContext->PinConnect->PinId;

        /* get pin file object */
        Status = ObReferenceObjectByHandle(PinHandle,
                                           GENERIC_READ | GENERIC_WRITE, 
                                           IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to get file object with %x\n", Status);
            SetIrpIoStatus(WorkerContext->Irp, STATUS_UNSUCCESSFUL, 0);
            ExFreePool(WorkerContext);
            return;
        }
        WorkerContext->DispatchContext->FileObject = FileObject;
    }

    DPRINT1("creating virtual pin\n");
    /* now create the virtual audio pin which is exposed to wdmaud */
    Status = KsCreatePin(Filter, WorkerContext->PinConnect, GENERIC_READ | GENERIC_WRITE, &PinHandle);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create virtual pin with %x\n", Status);
        if (WorkerContext->CreateRealPin)
        {
            /* mark pin as free to use */
            WorkerContext->Entry->Pins[WorkerContext->PinConnect->PinId].References = 0;
        }

        SetIrpIoStatus(WorkerContext->Irp, STATUS_UNSUCCESSFUL, 0);
        ExFreePool(WorkerContext);
        return;
    }

   /* get pin file object */
    Status = ObReferenceObjectByHandle(PinHandle,
                                      GENERIC_READ | GENERIC_WRITE, 
                                      IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get file object with %x\n", Status);
        if (WorkerContext->CreateRealPin)
        {
            /* mark pin as free to use */
            WorkerContext->Entry->Pins[WorkerContext->PinConnect->PinId].References = 0;
        }

        ZwClose(PinHandle);
        SetIrpIoStatus(WorkerContext->Irp, STATUS_UNSUCCESSFUL, 0);
        ExFreePool(WorkerContext);
        return;
    }

    ASSERT(WorkerContext->DispatchContext);
    ASSERT(WorkerContext->DispatchContext->AudioEntry != NULL);
    ASSERT(WorkerContext->DispatchContext->FileObject != NULL);
    ASSERT(WorkerContext->DispatchContext->Handle != NULL);
    ASSERT(WorkerContext->AudioClient);
    ASSERT(WorkerContext->AudioClient->Handels);
    ASSERT(WorkerContext->AudioClient->Handels[WorkerContext->AudioClient->NumDevices -1] == NULL);

    /* store pin context */
    FileObject->FsContext2 = (PVOID)WorkerContext->DispatchContext;

    /* store pin handle in client specific struct */
    WorkerContext->AudioClient->Handels[WorkerContext->AudioClient->NumDevices-1] = PinHandle;

    DPRINT1("Successfully created Pin %p\n", WorkerContext->Irp);
    *((PHANDLE)WorkerContext->Irp->UserBuffer) = PinHandle;

    SetIrpIoStatus(WorkerContext->Irp, STATUS_SUCCESS, sizeof(HANDLE));
}


NTSTATUS
SysAudioHandleProperty(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
    KSPROPERTY PropertyRequest;
    KSCOMPONENTID ComponentId;
    PULONG Index;
    PKSPROPERTY Property;
    PSYSAUDIODEVEXT DeviceExtension;
    PKSAUDIO_DEVICE_ENTRY Entry;
    PSYSAUDIO_INSTANCE_INFO InstanceInfo;
    PSYSAUDIO_CLIENT ClientInfo;
    ULONG Count, BytesReturned;
    PKSOBJECT_CREATE_ITEM CreateItem;
    UNICODE_STRING GuidString;
    ULONG Length;
    KSPIN_CONNECT * PinConnect;
    KSP_PIN PinRequest;
    KSPIN_CINSTANCES PinInstances;
    PPIN_WORKER_CONTEXT WorkerContext;
    PDISPATCH_CONTEXT DispatchContext;
    PIO_WORKITEM WorkItem;
    PFILE_OBJECT FileObject;

    /* access the create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);


    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(KSPROPERTY))
    {
        /* buffer must be atleast of sizeof KSPROPERTY */
        return SetIrpIoStatus(Irp, STATUS_BUFFER_TOO_SMALL, sizeof(KSPROPERTY));
    }

    Property = (PKSPROPERTY)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;
    DeviceExtension = (PSYSAUDIODEVEXT)DeviceObject->DeviceExtension;

    if (IsEqualGUIDAligned(&Property->Set, &KSPROPSETID_Pin))
    {
        /* ros specific request */
        if (Property->Id == KSPROPERTY_PIN_DATARANGES)
        {
            if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(KSP_PIN))
            {
                /* too small buffer */
                return SetIrpIoStatus(Irp, STATUS_BUFFER_TOO_SMALL, sizeof(KSPROPERTY) + sizeof(ULONG));
            }

            Entry = GetListEntry(&DeviceExtension->KsAudioDeviceList, ((KSP_PIN*)Property)->Reserved);
            if (!Entry)
            {
                /* too small buffer */
                return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
            }

            Status = KsSynchronousIoControlDevice(Entry->FileObject, KernelMode, IOCTL_KS_PROPERTY,
                                                  (PVOID)IoStack->Parameters.DeviceIoControl.Type3InputBuffer,
                                                  IoStack->Parameters.DeviceIoControl.InputBufferLength,
                                                  Irp->UserBuffer,
                                                  IoStack->Parameters.DeviceIoControl.OutputBufferLength,
                                                  &BytesReturned);
            Irp->IoStatus.Status = Status;
            Irp->IoStatus.Information = BytesReturned;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }
    }
    else if (IsEqualGUIDAligned(&Property->Set, &KSPROPSETID_Sysaudio))
    {
        if (Property->Id == KSPROPERTY_SYSAUDIO_COMPONENT_ID)
        {
            if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(KSPROPERTY) + sizeof(ULONG))
            {
                /* too small buffer */
                return SetIrpIoStatus(Irp, STATUS_BUFFER_TOO_SMALL, sizeof(KSPROPERTY) + sizeof(ULONG));
            }

            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(KSCOMPONENTID))
            {
                /* too small buffer */
                return SetIrpIoStatus(Irp, STATUS_BUFFER_TOO_SMALL, sizeof(KSCOMPONENTID));
            }

            Index = (PULONG)(Property + 1);

            if (DeviceExtension->NumberOfKsAudioDevices <= *Index)
            {
                /* invalid index */
                return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
            }
            Entry = GetListEntry(&DeviceExtension->KsAudioDeviceList, *Index);
            ASSERT(Entry != NULL);

            PropertyRequest.Set = KSPROPSETID_General;
            PropertyRequest.Id = KSPROPERTY_GENERAL_COMPONENTID;
            PropertyRequest.Flags = KSPROPERTY_TYPE_GET;

            /* call the filter */
            Status = KsSynchronousIoControlDevice(Entry->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&PropertyRequest, sizeof(KSPROPERTY), (PVOID)&ComponentId, sizeof(KSCOMPONENTID), &BytesReturned);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("KsSynchronousIoControlDevice failed with %x for KSPROPERTY_GENERAL_COMPONENTID\n", Status);
                return SetIrpIoStatus(Irp, Status, 0);
            }
            RtlMoveMemory(Irp->UserBuffer, &ComponentId, sizeof(KSCOMPONENTID));
            return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(KSCOMPONENTID));
        }
        else if (Property->Id == KSPROPERTY_SYSAUDIO_DEVICE_COUNT)
        {
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG))
            {
                /* too small buffer */
                return SetIrpIoStatus(Irp, STATUS_BUFFER_TOO_SMALL, sizeof(ULONG));
            }
            *((PULONG)Irp->UserBuffer) = DeviceExtension->NumberOfKsAudioDevices;
            return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(ULONG));
        }
        else if (Property->Id == KSPROPERTY_SYSAUDIO_DEVICE_INSTANCE)
        {
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG))
            {
                /* too small buffer */
                return SetIrpIoStatus(Irp, STATUS_BUFFER_TOO_SMALL, sizeof(KSCOMPONENTID));
            }

            if (Property->Flags & KSPROPERTY_TYPE_SET)
            {
                Index = (PULONG)Irp->UserBuffer;
                return SysAudioOpenVirtualDevice(Irp, *Index, DeviceExtension);
            }
            else if (Property->Flags & KSPROPERTY_TYPE_GET)
            {
                Index = (PULONG)Irp->UserBuffer;
                /* get client context */
                ClientInfo = (PSYSAUDIO_CLIENT)CreateItem->Context;
                ASSERT(ClientInfo);
                /* does the client already use a device */
                if (!ClientInfo->NumDevices)
                {
                    /* no device open */
                    return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
                }
                /* store last opened device number */
                *Index = ClientInfo->Devices[ClientInfo->NumDevices-1];
                /* found no device with that device index open */
                return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(ULONG));
            }
        }
        else if (Property->Id == KSPROPERTY_SYSAUDIO_INSTANCE_INFO)
        {
            if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SYSAUDIO_INSTANCE_INFO))
            {
                /* too small buffer */
                return SetIrpIoStatus(Irp, STATUS_BUFFER_TOO_SMALL, sizeof(SYSAUDIO_INSTANCE_INFO));
            }

            /* get input parameter */
            InstanceInfo = (PSYSAUDIO_INSTANCE_INFO)Property;

            if (Property->Flags & KSPROPERTY_TYPE_SET)
            {
                return SysAudioOpenVirtualDevice(Irp, InstanceInfo->DeviceNumber, DeviceExtension);
            }
            else if (Property->Flags & KSPROPERTY_TYPE_GET)
            {
                /* get client context */
                ClientInfo = (PSYSAUDIO_CLIENT)CreateItem->Context;
                ASSERT(ClientInfo);
                /* does the client already use a device */
                if (!ClientInfo->NumDevices)
                {
                    /* no device open */
                    return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
                }
                for(Count = 0; Count < ClientInfo->NumDevices; Count++)
                {
                    if (ClientInfo->Devices[Count] == InstanceInfo->DeviceNumber)
                    {
                        /* specified device is open */
                        return SetIrpIoStatus(Irp, STATUS_SUCCESS, 0);
                    }
                }
                /* found no device with that device index open */
                return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
            }
        }
        else if (Property->Id == (ULONG)-1)
        {
            /* ros specific pin creation request */
            DPRINT1("Initiating create request\n");

            Length = sizeof(KSDATAFORMAT) + sizeof(KSPIN_CONNECT) + sizeof(SYSAUDIO_INSTANCE_INFO);
            if (IoStack->Parameters.DeviceIoControl.InputBufferLength < Length ||
                IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(HANDLE))
            {
                /* invalid parameters */
                return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
            }

            /* get input parameter */
            InstanceInfo = (PSYSAUDIO_INSTANCE_INFO)Property;
            if (DeviceExtension->NumberOfKsAudioDevices <= InstanceInfo->DeviceNumber)
            {
                /* invalid parameters */
                return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
            }

            /* get client context */
            ClientInfo = (PSYSAUDIO_CLIENT)CreateItem->Context;
            ASSERT(ClientInfo);
            ASSERT(ClientInfo->NumDevices >= 1);
            ASSERT(ClientInfo->Devices != NULL);
            ASSERT(ClientInfo->Devices[ClientInfo->NumDevices-1] == InstanceInfo->DeviceNumber);

            /* get sysaudio entry */
            Entry = GetListEntry(&DeviceExtension->KsAudioDeviceList, InstanceInfo->DeviceNumber);
            ASSERT(Entry != NULL);

            if (!Entry->Pins)
            {
                PropertyRequest.Set = KSPROPSETID_Pin;
                PropertyRequest.Flags = KSPROPERTY_TYPE_GET;
                PropertyRequest.Id = KSPROPERTY_PIN_CTYPES;

                /* query for num of pins */
                Status = KsSynchronousIoControlDevice(Entry->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&PropertyRequest, sizeof(KSPROPERTY), (PVOID)&Count, sizeof(ULONG), &BytesReturned);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT("Property Request KSPROPERTY_PIN_CTYPES failed with %x\n", Status);
                    return SetIrpIoStatus(Irp, Status, 0);
                }
                DPRINT("KSPROPERTY_TYPE_GET num pins %d\n", Count);

                Entry->Pins = ExAllocatePool(NonPagedPool, Count * sizeof(PIN_INFO));
                if (!Entry->Pins)
                {
                    /* invalid parameters */
                    return SetIrpIoStatus(Irp, STATUS_NO_MEMORY, 0);
                }
                /* clear array */
                RtlZeroMemory(Entry->Pins, sizeof(PIN_INFO) * Count);
                Entry->NumberOfPins = Count;

            }

            /* get connect details */
            PinConnect = (KSPIN_CONNECT*)(InstanceInfo + 1);

            if (Entry->NumberOfPins <= PinConnect->PinId)
            {
                DPRINT("Invalid PinId %x\n", PinConnect->PinId);
                return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
            }

            PinRequest.PinId = PinConnect->PinId;
            PinRequest.Property.Set = KSPROPSETID_Pin;
            PinRequest.Property.Flags = KSPROPERTY_TYPE_GET;
            PinRequest.Property.Id = KSPROPERTY_PIN_CINSTANCES;

            //RtlZeroMemory(&PinInstances, sizeof(KSPIN_CINSTANCES));
            Status = KsSynchronousIoControlDevice(Entry->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&PinRequest, sizeof(KSP_PIN), (PVOID)&PinInstances, sizeof(KSPIN_CINSTANCES), &BytesReturned);
            if (!NT_SUCCESS(Status))
            {
                DPRINT("Property Request KSPROPERTY_PIN_GLOBALCINSTANCES failed with %x\n", Status);
                return SetIrpIoStatus(Irp, Status, 0);
            }
            DPRINT1("PinInstances Current %u Max %u\n", PinInstances.CurrentCount, PinInstances.PossibleCount);

            WorkItem = IoAllocateWorkItem(DeviceObject);
            if (!WorkItem)
            {
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            /* create worker context */
            WorkerContext = ExAllocatePool(NonPagedPool, sizeof(PIN_WORKER_CONTEXT));
            if (!WorkerContext)
            {
                /* invalid parameters */
                IoFreeWorkItem(WorkItem);
                return SetIrpIoStatus(Irp, STATUS_NO_MEMORY, 0);
            }

            /* create worker context */
            DispatchContext = ExAllocatePool(NonPagedPool, sizeof(DISPATCH_CONTEXT));
            if (!DispatchContext)
            {
                /* invalid parameters */
                IoFreeWorkItem(WorkItem);
                ExFreePool(WorkerContext);
                return SetIrpIoStatus(Irp, STATUS_NO_MEMORY, 0);
            }
            /* prepare context */
            RtlZeroMemory(WorkerContext, sizeof(PIN_WORKER_CONTEXT));
            RtlZeroMemory(DispatchContext, sizeof(DISPATCH_CONTEXT));

            if (PinInstances.CurrentCount == PinInstances.PossibleCount)
            {
                /* pin already exists */
                DPRINT1("Pins %p\n", Entry->Pins);
                DbgBreakPoint();
                ASSERT(Entry->Pins[PinConnect->PinId].PinHandle != NULL);

                if (Entry->Pins[PinConnect->PinId].References != 0)
                {
                    /* FIXME need ksmixer */
                    DPRINT1("Device %u Pin %u is already occupied, try later\n", InstanceInfo->DeviceNumber, PinConnect->PinId);
                    IoFreeWorkItem(WorkItem);
                    ExFreePool(WorkerContext);
                    ExFreePool(DispatchContext);
                    return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
                }
                /* re-using pin */
                PropertyRequest.Set = KSPROPSETID_Connection;
                PropertyRequest.Flags = KSPROPERTY_TYPE_SET;
                PropertyRequest.Id = KSPROPERTY_CONNECTION_DATAFORMAT;

                /* get pin file object */
                Status = ObReferenceObjectByHandle(Entry->Pins[PinConnect->PinId].PinHandle,
                                                   GENERIC_READ | GENERIC_WRITE, 
                                                   IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);

                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("Failed to get pin file object with %x\n", Status);
                    IoFreeWorkItem(WorkItem);
                    ExFreePool(WorkerContext);
                    ExFreePool(DispatchContext);
                    return SetIrpIoStatus(Irp, Status, 0);
                }

                Length -= sizeof(KSPIN_CONNECT) + sizeof(SYSAUDIO_INSTANCE_INFO);
                Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&PropertyRequest, sizeof(KSPROPERTY), 
                                                      (PVOID)(PinConnect + 1), Length, &BytesReturned);

                ObDereferenceObject(FileObject);

                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("Failed to set format with Status %x\n", Status);
                    IoFreeWorkItem(WorkItem);
                    ExFreePool(WorkerContext);
                    ExFreePool(DispatchContext);
                    return SetIrpIoStatus(Irp, Status, 0);
                }

            }
            else
            {
                /* create the real pin */
                WorkerContext->CreateRealPin = TRUE;
            }

            /* set up context */

            WorkerContext->DispatchContext = DispatchContext;
            WorkerContext->Entry = Entry;
            WorkerContext->Irp = Irp;
            WorkerContext->PinConnect = PinConnect;
            WorkerContext->AudioClient = ClientInfo;

            DPRINT("Queing Irp %p\n", Irp);
            /* queue the work item */
            IoMarkIrpPending(Irp);
            Irp->IoStatus.Status = STATUS_PENDING;
            Irp->IoStatus.Information = 0;
            IoQueueWorkItem(WorkItem, CreatePinWorkerRoutine, DelayedWorkQueue, (PVOID)WorkerContext);

            /* mark irp as pending */
            return STATUS_PENDING;
        }
    }

    RtlStringFromGUID(&Property->Set, &GuidString);
    DPRINT1("Unhandeled property Set |%S| Id %u Flags %x\n", GuidString.Buffer, Property->Id, Property->Flags);
    DbgBreakPoint();
    RtlFreeUnicodeString(&GuidString);

    return Status;
}
