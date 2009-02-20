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


NTSTATUS
SetIrpIoStatus(
    IN PIRP Irp,
    IN NTSTATUS Status,
    IN ULONG Length)
{
    Irp->IoStatus.Information = Length;
    Irp->IoStatus.Status = Status;
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
        ClientInfo->NumDevices = 1;
        ClientInfo->Devices[0] = DeviceNumber;
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
    /* increase usage count */
    Entry->NumberOfClients++;

    /* copy device count array */
    RtlMoveMemory(Index, ClientInfo->Devices, ClientInfo->NumDevices * sizeof(ULONG));
    Index[ClientInfo->NumDevices] = DeviceNumber;
    ExFreePool(ClientInfo->Devices);
    ClientInfo->NumDevices++;
    ClientInfo->Devices = Index;

    return SetIrpIoStatus(Irp, STATUS_SUCCESS, 0);

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

    if (IsEqualGUIDAligned(&Property->Set, &KSPROPSETID_Sysaudio))
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
            Status = KsSynchronousIoControlDevice(Entry->FileObject, KernelMode, IOCTL_KS_WRITE_STREAM, (PVOID)&PropertyRequest, sizeof(KSPROPERTY), (PVOID)&ComponentId, sizeof(KSCOMPONENTID), &BytesReturned);
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
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(SYSAUDIO_INSTANCE_INFO))
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
    }

    RtlStringFromGUID(&Property->Set, &GuidString);
    DPRINT1("Unhandeled property Set |%S| Id %u Flags %x\n", GuidString.Buffer, Property->Id, Property->Flags);
    DbgBreakPoint();
    RtlFreeUnicodeString(&GuidString);

    return Status;
}


