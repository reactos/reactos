/*
 * PROJECT:         ReactOS Kernel Streaming Mixer
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/wdm/audio/filters/kmixer/kmixer.c
 * PURPOSE:         Pin functions
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */

#include "kmixer.h"

const GUID KSPROPSETID_Connection              = {0x1D58C920L, 0xAC9B, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};

NTSTATUS
PerformQualityConversion(
    PUCHAR Buffer,
    ULONG BufferLength,
    ULONG OldWidth,
    ULONG NewWidth,
    PVOID * Result,
    PULONG ResultLength)
{
    ULONG Samples;
    ULONG Index;

    ASSERT(OldWidth != NewWidth);

    Samples = BufferLength / (OldWidth / 8);
    //DPRINT("Samples %u BufferLength %u\n", Samples, BufferLength);

    if (OldWidth == 8 && NewWidth == 16)
    {
         USHORT Sample;
         PUSHORT BufferOut = ExAllocatePool(NonPagedPool, Samples * sizeof(USHORT));
         if (!BufferOut)
             return STATUS_INSUFFICIENT_RESOURCES;

          for(Index = 0; Index < Samples; Index++)
          {
              Sample = Buffer[Index];
              Sample *= 2;
#ifdef _X86_
              Sample = _byteswap_ushort(Sample);
#endif
              BufferOut[Index] = Sample;
          }
          *Result = BufferOut;
          *ResultLength = Samples * sizeof(USHORT);
    }
    else if (OldWidth == 8 && NewWidth == 32)
    {
         ULONG Sample;
         PULONG BufferOut = ExAllocatePool(NonPagedPool, Samples * sizeof(ULONG));
         if (!BufferOut)
             return STATUS_INSUFFICIENT_RESOURCES;

          for(Index = 0; Index < Samples; Index++)
          {
              Sample = Buffer[Index];
              Sample *= 16777216;
#ifdef _X86_
              Sample = _byteswap_ulong(Sample);
#endif
              BufferOut[Index] = Sample;
          }
          *Result = BufferOut;
          *ResultLength = Samples * sizeof(ULONG);
    }
    else if (OldWidth == 16 && NewWidth == 32)
    {
         ULONG Sample;
         PUSHORT BufferIn = (PUSHORT)Buffer;
         PULONG BufferOut = ExAllocatePool(NonPagedPool, Samples * sizeof(ULONG));
         if (!BufferOut)
             return STATUS_INSUFFICIENT_RESOURCES;

          for(Index = 0; Index < Samples; Index++)
          {
              Sample = BufferIn[Index];
              Sample *= 65536;
#ifdef _X86_
              Sample = _byteswap_ulong(Sample);
#endif
              BufferOut[Index] = Sample;
          }
          *Result = BufferOut;
          *ResultLength = Samples * sizeof(ULONG);
    }

    else if (OldWidth == 16 && NewWidth == 8)
    {
         USHORT Sample;
         PUSHORT BufferIn = (PUSHORT)Buffer;
         PUCHAR BufferOut = ExAllocatePool(NonPagedPool, Samples * sizeof(UCHAR));
         if (!BufferOut)
             return STATUS_INSUFFICIENT_RESOURCES;

          for(Index = 0; Index < Samples; Index++)
          {
              Sample = BufferIn[Index];
#ifdef _X86_
              Sample = _byteswap_ushort(Sample);
#endif
              Sample /= 256;
              BufferOut[Index] = (Sample & 0xFF);
          }
          *Result = BufferOut;
          *ResultLength = Samples * sizeof(UCHAR);
    }
    else if (OldWidth == 32 && NewWidth == 8)
    {
         ULONG Sample;
         PULONG BufferIn = (PULONG)Buffer;
         PUCHAR BufferOut = ExAllocatePool(NonPagedPool, Samples * sizeof(UCHAR));
         if (!BufferOut)
             return STATUS_INSUFFICIENT_RESOURCES;

          for(Index = 0; Index < Samples; Index++)
          {
              Sample = BufferIn[Index];
#ifdef _X86_
              Sample = _byteswap_ulong(Sample);
#endif
              Sample /= 16777216;
              BufferOut[Index] = (Sample & 0xFF);
          }
          *Result = BufferOut;
          *ResultLength = Samples * sizeof(UCHAR);
    }
    else if (OldWidth == 32 && NewWidth == 16)
    {
         USHORT Sample;
         PULONG BufferIn = (PULONG)Buffer;
         PUSHORT BufferOut = ExAllocatePool(NonPagedPool, Samples * sizeof(USHORT));
         if (!BufferOut)
             return STATUS_INSUFFICIENT_RESOURCES;

          for(Index = 0; Index < Samples; Index++)
          {
              Sample = BufferIn[Index];
#ifdef _X86_
              Sample = _byteswap_ulong(Sample);
#endif
              Sample /= 65536;
              BufferOut[Index] = (Sample & 0xFFFF);
          }
          *Result = BufferOut;
          *ResultLength = Samples * sizeof(USHORT);
    }
    else
    {
        DPRINT1("Not implemented conversion OldWidth %u NewWidth %u\n", OldWidth, NewWidth);
        return STATUS_NOT_IMPLEMENTED;
    }

    return STATUS_SUCCESS;
}


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
    UNIMPLEMENTED

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
    UNIMPLEMENTED

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
    UNIMPLEMENTED

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
    UNIMPLEMENTED

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
    UNIMPLEMENTED

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

    UNIMPLEMENTED

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
    UNIMPLEMENTED


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
    UNIMPLEMENTED
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
    PKSSTREAM_HEADER StreamHeader;
    PVOID BufferOut;
    ULONG BufferLength;
    NTSTATUS Status = STATUS_SUCCESS;
    PKSDATAFORMAT_WAVEFORMATEX Formats;
    PKSDATAFORMAT_WAVEFORMATEX InputFormat, OutputFormat;

    DPRINT("Pin_fnFastWrite called DeviceObject %p Irp %p\n", DeviceObject);

    Formats = (PKSDATAFORMAT_WAVEFORMATEX)FileObject->FsContext2;

    InputFormat = Formats;
    OutputFormat = (Formats + 1);
    StreamHeader = (PKSSTREAM_HEADER)Buffer;


    DPRINT("Num Channels %u Old Channels %u\n SampleRate %u Old SampleRate %u\n BitsPerSample %u Old BitsPerSample %u\n",
               InputFormat->WaveFormatEx.nChannels, OutputFormat->WaveFormatEx.nChannels,
               InputFormat->WaveFormatEx.nSamplesPerSec, OutputFormat->WaveFormatEx.nSamplesPerSec,
               InputFormat->WaveFormatEx.wBitsPerSample, OutputFormat->WaveFormatEx.wBitsPerSample);


    if (InputFormat->WaveFormatEx.wBitsPerSample != OutputFormat->WaveFormatEx.wBitsPerSample)
    {
        Status = PerformQualityConversion(StreamHeader->Data,
                                          StreamHeader->DataUsed,
                                          InputFormat->WaveFormatEx.wBitsPerSample,
                                          OutputFormat->WaveFormatEx.wBitsPerSample,
                                          &BufferOut,
                                          &BufferLength);
        if (NT_SUCCESS(Status))
        {
            //DPRINT1("Old BufferSize %u NewBufferSize %u\n", StreamHeader->DataUsed, BufferLength);
            ExFreePool(StreamHeader->Data);
            StreamHeader->Data = BufferOut;
            StreamHeader->DataUsed = BufferLength;
        }
    }

    if (InputFormat->WaveFormatEx.nSamplesPerSec != OutputFormat->WaveFormatEx.nSamplesPerSec)
    {
        /* sample format conversion must be done in a deferred routine */
        DPRINT1("SampleRate conversion not available yet %u %u\n", InputFormat->WaveFormatEx.nSamplesPerSec, OutputFormat->WaveFormatEx.nSamplesPerSec);
        return FALSE;
    }

    if (NT_SUCCESS(Status))
        return TRUE;
    else
        return TRUE;
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
    PVOID Block = ExAllocatePool(NonPagedPool, Elements * ElementSize);
    if (Block)
        RtlZeroMemory(Block, Elements * ElementSize);

    return Block;
}

void free(PVOID Block)
{
    ExFreePool(Block);
}

void *memset(
   void* dest,
   int c,
   size_t count)
{
    RtlFillMemory(dest, count, c);
    return dest;
}

void * memcpy(
   void* dest,
   const void* src,
   size_t count)
{
    RtlCopyMemory(dest, src, count);
    return dest;
}

void *memmove(
   void* dest,
   const void* src,
   size_t count)
{
    RtlMoveMemory(dest, src, count);
    return dest;
}



