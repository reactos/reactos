/*
 * PROJECT:         ReactOS Kernel Streaming Mixer
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/wdm/audio/filters/kmixer/kmixer.c
 * PURPOSE:         Pin functions
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "kmixer.h"

#define NDEBUG
#include <debug.h>

const GUID KSPROPSETID_Connection              = {0x1D58C920L, 0xAC9B, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};

NTSTATUS
NTAPI
Pin_fnDeviceIoControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PKSP_PIN Property;
    //DPRINT1("Pin_fnDeviceIoControl called DeviceObject %p Irp %p\n", DeviceObject);

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.DeviceIoControl.InputBufferLength == sizeof(KSP_PIN) && IoStack->Parameters.DeviceIoControl.OutputBufferLength == sizeof(KSDATAFORMAT_WAVEFORMATEX))
    {
        Property = (PKSP_PIN)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;

        if (IsEqualGUIDAligned(&Property->Property.Set, &KSPROPSETID_Connection))
        {
            if (Property->Property.Id == KSPROPERTY_CONNECTION_DATAFORMAT && Property->Property.Flags == KSPROPERTY_TYPE_SET)
            {
                PKSDATAFORMAT_WAVEFORMATEX Formats;
                PKSDATAFORMAT_WAVEFORMATEX WaveFormat;

                Formats = (PKSDATAFORMAT_WAVEFORMATEX)IoStack->FileObject->FsContext2;
                WaveFormat = (PKSDATAFORMAT_WAVEFORMATEX)Irp->UserBuffer;

                ASSERT(Property->PinId == 0 || Property->PinId == 1);
                ASSERT(Formats);
                ASSERT(WaveFormat);

                Formats[Property->PinId].WaveFormatEx.nChannels = WaveFormat->WaveFormatEx.nChannels;
                Formats[Property->PinId].WaveFormatEx.wBitsPerSample = WaveFormat->WaveFormatEx.wBitsPerSample;
                Formats[Property->PinId].WaveFormatEx.nSamplesPerSec = WaveFormat->WaveFormatEx.nSamplesPerSec;

                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_SUCCESS;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_SUCCESS;
            }
        }
    }
    DPRINT1("Size %u Expected %u\n",IoStack->Parameters.DeviceIoControl.OutputBufferLength,  sizeof(KSDATAFORMAT_WAVEFORMATEX));
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
Pin_fnRead(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    UNIMPLEMENTED;

    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
Pin_fnWrite(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    UNIMPLEMENTED;

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
Pin_fnFlush(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    UNIMPLEMENTED;

    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
Pin_fnClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    UNIMPLEMENTED;

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
Pin_fnQuerySecurity(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    UNIMPLEMENTED;

    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
Pin_fnSetSecurity(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{

    UNIMPLEMENTED;

    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

BOOLEAN
NTAPI
Pin_fnFastDeviceIoControl(
    PFILE_OBJECT FileObject,
    BOOLEAN Wait,
    PVOID InputBuffer,
    ULONG InputBufferLength,
    PVOID OutputBuffer,
    ULONG OutputBufferLength,
    ULONG IoControlCode,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED;


    return FALSE;
}


BOOLEAN
NTAPI
Pin_fnFastRead(
    PFILE_OBJECT FileObject,
    PLARGE_INTEGER FileOffset,
    ULONG Length,
    BOOLEAN Wait,
    ULONG LockKey,
    PVOID Buffer,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED;
    return FALSE;

}

BOOLEAN
NTAPI
Pin_fnFastWrite(
    PFILE_OBJECT FileObject,
    PLARGE_INTEGER FileOffset,
    ULONG Length,
    BOOLEAN Wait,
    ULONG LockKey,
    PVOID Buffer,
    PIO_STATUS_BLOCK IoStatus,
    PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED;
    return FALSE;
}

static KSDISPATCH_TABLE PinTable =
{
    Pin_fnDeviceIoControl,
    Pin_fnRead,
    Pin_fnWrite,
    Pin_fnFlush,
    Pin_fnClose,
    Pin_fnQuerySecurity,
    Pin_fnSetSecurity,
    Pin_fnFastDeviceIoControl,
    Pin_fnFastRead,
    Pin_fnFastWrite,
};

NTSTATUS
CreatePin(
    IN PIRP Irp)
{
    NTSTATUS Status;
    KSOBJECT_HEADER ObjectHeader;
    PKSDATAFORMAT DataFormat;
    PIO_STACK_LOCATION IoStack;


    DataFormat = ExAllocatePool(NonPagedPool, sizeof(KSDATAFORMAT_WAVEFORMATEX) * 2);
    if (!DataFormat)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(DataFormat, sizeof(KSDATAFORMAT_WAVEFORMATEX) * 2);

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    IoStack->FileObject->FsContext2 = (PVOID)DataFormat;

    /* allocate object header */
    Status = KsAllocateObjectHeader(&ObjectHeader, 0, NULL, Irp, &PinTable);
    return Status;
}

void * calloc(size_t Elements, size_t ElementSize)
{
    ULONG Index;
    PUCHAR Block = ExAllocatePool(NonPagedPool, Elements * ElementSize);
    if (!Block)
        return NULL;

    for(Index = 0; Index < Elements * ElementSize; Index++)
        Block[Index] = 0;

    return Block;
}

void free(PVOID Block)
{
    ExFreePool(Block);
}
