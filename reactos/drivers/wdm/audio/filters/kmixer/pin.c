/*
 * PROJECT:         ReactOS Kernel Streaming Mixer
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/wdm/audio/filters/kmixer/kmixer.c
 * PURPOSE:         Pin functions
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */

#include "kmixer.h"

const GUID KSPROPSETID_Connection              = {0x1D58C920L, 0xAC9B, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};

#ifdef _X86_
#define htons(w) \
     ((((w) & 0xFF00) >> 8) | \
      (((w) & 0x00FF) << 8))

#define htonl(n) (((((unsigned long)(n) & 0xFF)) << 24) | \
                  ((((unsigned long)(n) & 0xFF00)) << 8) | \
                  ((((unsigned long)(n) & 0xFF0000)) >> 8) | \
                  ((((unsigned long)(n) & 0xFF000000)) >> 24))

#define ntohs(n) (((((unsigned short)(n) & 0xFF)) << 8) | (((unsigned short)(n) & 0xFF00) >> 8))


#define ntohl(n) (((((unsigned long)(n) & 0xFF)) << 24) | \
                  ((((unsigned long)(n) & 0xFF00)) << 8) | \
                  ((((unsigned long)(n) & 0xFF0000)) >> 8) | \
                  ((((unsigned long)(n) & 0xFF000000)) >> 24))

#endif


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
              BufferOut[Index] = htons(Sample);
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
              BufferOut[Index] = htonl(Sample);
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
              BufferOut[Index] = htonl(Sample);
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
              Sample = ntohs(Sample);
              Sample /= 256;
              BufferOut[Index] = (Sample / 0xFF);
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
              Sample = ntohl(Sample);
              Sample /= 16777216;
              BufferOut[Index] = Sample & 0xFF;
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
              Sample = ntohl(Sample);
              Sample /= 65536;
              BufferOut[Index] = Sample & 0xFFFF;
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
    PKSPROPERTY Property;
    DPRINT1("Pin_fnDeviceIoControl called DeviceObject %p Irp %p\n", DeviceObject);

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.DeviceIoControl.InputBufferLength == sizeof(KSPROPERTY) && IoStack->Parameters.DeviceIoControl.OutputBufferLength == sizeof(KSDATAFORMAT_WAVEFORMATEX))
    {
        Property = (PKSPROPERTY)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;

        if (IsEqualGUIDAligned(&Property->Set, &KSPROPSETID_Connection))
        {
            if (Property->Id == KSPROPERTY_CONNECTION_DATAFORMAT && Property->Flags == KSPROPERTY_TYPE_SET)
            {
                PKSDATAFORMAT_WAVEFORMATEX WaveFormat2;
                PKSDATAFORMAT_WAVEFORMATEX WaveFormat = ExAllocatePool(NonPagedPool, sizeof(KSDATAFORMAT_WAVEFORMATEX));

                if (!WaveFormat)
                {
                    Irp->IoStatus.Information = 0;
                    Irp->IoStatus.Status = STATUS_NO_MEMORY;
                    IoCompleteRequest(Irp, IO_NO_INCREMENT);
                    return STATUS_NO_MEMORY;
                }

                if (IoStack->FileObject->FsContext2)
                {
                    ExFreePool(IoStack->FileObject->FsContext2);
                }

                WaveFormat2 = (PKSDATAFORMAT_WAVEFORMATEX)Irp->UserBuffer;
                WaveFormat->WaveFormatEx.nChannels = WaveFormat2->WaveFormatEx.nChannels;
                WaveFormat->WaveFormatEx.nSamplesPerSec = WaveFormat2->WaveFormatEx.nSamplesPerSec;
                WaveFormat->WaveFormatEx.wBitsPerSample = WaveFormat2->WaveFormatEx.wBitsPerSample;

                IoStack->FileObject->FsContext2 = (PVOID)WaveFormat;


                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_SUCCESS;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_SUCCESS;
            }
        }
    }

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
    DPRINT1("Pin_fnRead called DeviceObject %p Irp %p\n", DeviceObject);

    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
PinWriteCompletionRoutine(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  Context)
{
    PIRP CIrp = (PIRP)Context;

    CIrp->IoStatus.Status = STATUS_SUCCESS;
    CIrp->IoStatus.Information = 0;
    IoCompleteRequest(CIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
Pin_fnWrite(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    DPRINT1("Pin_fnWrite called DeviceObject %p Irp %p\n", DeviceObject);


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
    DPRINT1("Pin_fnFlush called DeviceObject %p Irp %p\n", DeviceObject);

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
    DPRINT1("Pin_fnClose called DeviceObject %p Irp %p\n", DeviceObject);

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
    DPRINT1("Pin_fnQuerySecurity called DeviceObject %p Irp %p\n", DeviceObject);

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

    DPRINT1("Pin_fnSetSecurity called DeviceObject %p Irp %p\n", DeviceObject);

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
    DPRINT1("Pin_fnFastDeviceIoControl called DeviceObject %p Irp %p\n", DeviceObject);


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
    DPRINT1("Pin_fnFastRead called DeviceObject %p Irp %p\n", DeviceObject);

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
    PKSPIN_CONNECT ConnectDetails;
    PKSSTREAM_HEADER StreamHeader;
    PVOID BufferOut;
    ULONG BufferLength;
    NTSTATUS Status = STATUS_SUCCESS;
    LPWSTR PinName = L"{146F1A80-4791-11D0-A5D6-28DB04C10000}\\";

    PKSDATAFORMAT_WAVEFORMATEX BaseFormat, TransformedFormat;

    DPRINT1("Pin_fnFastWrite called DeviceObject %p Irp %p\n", DeviceObject);


    BaseFormat = (PKSDATAFORMAT_WAVEFORMATEX)FileObject->FsContext2;
    if (!BaseFormat)
    {
        DPRINT1("Expected DataFormat\n");
        DbgBreakPoint();
        IoStatus->Status = STATUS_UNSUCCESSFUL;
        IoStatus->Information = 0;
        return FALSE;
    }

    if (FileObject->FileName.Length < wcslen(PinName) + sizeof(KSPIN_CONNECT) + sizeof(KSDATAFORMAT))
    {
        DPRINT1("Expected DataFormat\n");
        DbgBreakPoint();
        IoStatus->Status = STATUS_INVALID_PARAMETER;
        IoStatus->Information = 0;
        return FALSE;
    }

    ConnectDetails = (PKSPIN_CONNECT)(FileObject->FileName.Buffer + wcslen(PinName));
    TransformedFormat = (PKSDATAFORMAT_WAVEFORMATEX)(ConnectDetails + 1);
    StreamHeader = (PKSSTREAM_HEADER)Buffer;

    DPRINT1("Num Channels %u Old Channels %u\n SampleRate %u Old SampleRate %u\n BitsPerSample %u Old BitsPerSample %u\n",
               BaseFormat->WaveFormatEx.nChannels, TransformedFormat->WaveFormatEx.nChannels,
               BaseFormat->WaveFormatEx.nSamplesPerSec, TransformedFormat->WaveFormatEx.nSamplesPerSec,
               BaseFormat->WaveFormatEx.wBitsPerSample, TransformedFormat->WaveFormatEx.wBitsPerSample);

    if (BaseFormat->WaveFormatEx.wBitsPerSample != TransformedFormat->WaveFormatEx.wBitsPerSample)
    {
        Status = PerformQualityConversion(StreamHeader->Data,
                                          StreamHeader->DataUsed,
                                          BaseFormat->WaveFormatEx.wBitsPerSample,
                                          TransformedFormat->WaveFormatEx.wBitsPerSample,
                                          &BufferOut,
                                          &BufferLength);
        if (NT_SUCCESS(Status))
        {
            DPRINT1("Old BufferSize %u NewBufferSize %u\n", StreamHeader->DataUsed, BufferLength);
            ExFreePool(StreamHeader->Data);
            StreamHeader->Data = BufferOut;
            StreamHeader->DataUsed = BufferLength;
        }
    }

    if (BaseFormat->WaveFormatEx.nSamplesPerSec != TransformedFormat->WaveFormatEx.nSamplesPerSec)
    {
        /* sample format conversion must be done in a deferred routine */
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



