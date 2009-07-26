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
PerformSampleRateConversion(
    PUCHAR Buffer,
    ULONG BufferLength,
    ULONG OldRate,
    ULONG NewRate,
    ULONG BytesPerSample,
    ULONG NumChannels,
    PVOID * Result,
    PULONG ResultLength)
{
    KFLOATING_SAVE FloatSave;
    NTSTATUS Status;
    ULONG Index;
    SRC_STATE * State;
    SRC_DATA Data;
    PUCHAR ResultOut;
    int error;
    PFLOAT FloatIn, FloatOut;
    ULONG NumSamples;
    ULONG NewSamples;

    DPRINT("PerformSampleRateConversion OldRate %u NewRate %u BytesPerSample %u NumChannels %u Irql %u\n", OldRate, NewRate, BytesPerSample, NumChannels, KeGetCurrentIrql());

    ASSERT(BytesPerSample == 1 || BytesPerSample == 2 || BytesPerSample == 4);

    /* first acquire float save context */
    Status = KeSaveFloatingPointState(&FloatSave);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("KeSaveFloatingPointState failed with %x\n", Status);
        return Status;
    }

    NumSamples = BufferLength / (BytesPerSample * NumChannels);

    FloatIn = ExAllocatePool(NonPagedPool, NumSamples * NumChannels * sizeof(FLOAT));
    if (!FloatIn)
    {
        KeRestoreFloatingPointState(&FloatSave);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NewSamples = lrintf(((FLOAT)NumSamples * ((FLOAT)NewRate / (FLOAT)OldRate))) + 2;

    FloatOut = ExAllocatePool(NonPagedPool, NewSamples * NumChannels * sizeof(FLOAT));
    if (!FloatOut)
    {
        ExFreePool(FloatIn);
        KeRestoreFloatingPointState(&FloatSave);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ResultOut = ExAllocatePool(NonPagedPool, NewSamples * NumChannels * BytesPerSample);
    if (!FloatOut)
    {
        ExFreePool(FloatIn);
        ExFreePool(FloatOut);
        KeRestoreFloatingPointState(&FloatSave);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    State = src_new(SRC_SINC_FASTEST, NumChannels, &error);
    if (!State)
    {
        DPRINT1("KeSaveFloatingPointState failed with %x\n", Status);
        KeRestoreFloatingPointState(&FloatSave);
        ExFreePool(FloatIn);
        ExFreePool(FloatOut);
        ExFreePool(ResultOut);
        return STATUS_UNSUCCESSFUL;
    }

    /* fixme use asm */
    if (BytesPerSample == 1)
    {
        for(Index = 0; Index < NumSamples * NumChannels; Index++)
            FloatIn[Index] = (float)(Buffer[Index] / (1.0 * 0x80));
    }
    else if (BytesPerSample == 2)
    {
        src_short_to_float_array((short*)Buffer, FloatIn, NumSamples * NumChannels);
    }
    else if (BytesPerSample == 4)
    {
        src_int_to_float_array((int*)Buffer, FloatIn, NumSamples * NumChannels);
    }

    Data.data_in = FloatIn;
    Data.data_out = FloatOut;
    Data.input_frames = NumSamples;
    Data.output_frames = NewSamples;
    Data.src_ratio = (double)NewRate / (double)OldRate;

    error = src_process(State, &Data);
    if (error)
    {
        DPRINT1("src_process failed with %x\n", error);
        KeRestoreFloatingPointState(&FloatSave);
        ExFreePool(FloatIn);
        ExFreePool(FloatOut);
        ExFreePool(ResultOut);
        return STATUS_UNSUCCESSFUL;
    }

    if (BytesPerSample == 1)
    {
        /* FIXME perform over/under clipping */

        for(Index = 0; Index < Data.output_frames_gen * NumChannels; Index++)
            ResultOut[Index] = (lrintf(FloatOut[Index]) >> 24);
    }
    else if (BytesPerSample == 2)
    {
        PUSHORT Res = (PUSHORT)ResultOut;

        src_float_to_short_array(FloatIn, (short*)Res, Data.output_frames_gen * NumChannels);
    }
    else if (BytesPerSample == 4)
    {
        PULONG Res = (PULONG)ResultOut;

        src_float_to_int_array(FloatIn, (int*)Res, Data.output_frames_gen * NumChannels);
    }


    *Result = ResultOut;
    *ResultLength = Data.output_frames_gen * BytesPerSample * NumChannels;
    ExFreePool(FloatIn);
    ExFreePool(FloatOut);
    src_delete(State);
    KeRestoreFloatingPointState(&FloatSave);
    return STATUS_SUCCESS;
}

NTSTATUS
PerformChannelConversion(
    PUCHAR Buffer,
    ULONG BufferLength,
    ULONG OldChannels,
    ULONG NewChannels,
    ULONG BitsPerSample,
    PVOID * Result,
    PULONG ResultLength)
{
    ULONG Samples;
    ULONG NewIndex, OldIndex;

    Samples = BufferLength / (BitsPerSample / 8) / OldChannels;

    if (NewChannels > OldChannels)
    {
        if (BitsPerSample == 8)
        {
            PUCHAR BufferOut = ExAllocatePool(NonPagedPool, Samples * NewChannels);
            if (!BufferOut)
                return STATUS_INSUFFICIENT_RESOURCES;

            for(NewIndex = 0, OldIndex = 0; OldIndex < Samples * OldChannels; NewIndex += NewChannels, OldIndex += OldChannels)
            {
                ULONG SubIndex = 0;

                RtlMoveMemory(&BufferOut[NewIndex], &Buffer[OldIndex], OldChannels * sizeof(UCHAR));

                do
                {
                    /* 2 channel stretched to 4 looks like LRLR */
                     BufferOut[NewIndex+OldChannels + SubIndex] = Buffer[OldIndex + (SubIndex % OldChannels)];
                }while(SubIndex++ < NewChannels - OldChannels);
            }
            *Result = BufferOut;
            *ResultLength = Samples * NewChannels;
        }
        else if (BitsPerSample == 16)
        {
            PUSHORT BufferOut = ExAllocatePool(NonPagedPool, Samples * NewChannels);
            if (!BufferOut)
                return STATUS_INSUFFICIENT_RESOURCES;

            for(NewIndex = 0, OldIndex = 0; OldIndex < Samples * OldChannels; NewIndex += NewChannels, OldIndex += OldChannels)
            {
                ULONG SubIndex = 0;

                RtlMoveMemory(&BufferOut[NewIndex], &Buffer[OldIndex], OldChannels * sizeof(USHORT));

                do
                {
                     BufferOut[NewIndex+OldChannels + SubIndex] = Buffer[OldIndex + (SubIndex % OldChannels)];
                }while(SubIndex++ < NewChannels - OldChannels);
            }
            *Result = BufferOut;
            *ResultLength = Samples * NewChannels;
        }
        else if (BitsPerSample == 24)
        {
            PUCHAR BufferOut = ExAllocatePool(NonPagedPool, Samples * NewChannels);
            if (!BufferOut)
                return STATUS_INSUFFICIENT_RESOURCES;

            for(NewIndex = 0, OldIndex = 0; OldIndex < Samples * OldChannels; NewIndex += NewChannels, OldIndex += OldChannels)
            {
                ULONG SubIndex = 0;

                RtlMoveMemory(&BufferOut[NewIndex], &Buffer[OldIndex], OldChannels * 3);

                do
                {
                     RtlMoveMemory(&BufferOut[(NewIndex+OldChannels + SubIndex) * 3], &Buffer[(OldIndex + (SubIndex % OldChannels)) * 3], 3);
                }while(SubIndex++ < NewChannels - OldChannels);
            }
            *Result = BufferOut;
            *ResultLength = Samples * NewChannels;
        }
        else if (BitsPerSample == 32)
        {
            PULONG BufferOut = ExAllocatePool(NonPagedPool, Samples * NewChannels);
            if (!BufferOut)
                return STATUS_INSUFFICIENT_RESOURCES;

            for(NewIndex = 0, OldIndex = 0; OldIndex < Samples * OldChannels; NewIndex += NewChannels, OldIndex += OldChannels)
            {
                ULONG SubIndex = 0;

                RtlMoveMemory(&BufferOut[NewIndex], &Buffer[OldIndex], OldChannels * sizeof(ULONG));

                do
                {
                     BufferOut[NewIndex+OldChannels + SubIndex] = Buffer[OldIndex + (SubIndex % OldChannels)];
                }while(SubIndex++ < NewChannels - OldChannels);
            }
            *Result = BufferOut;
            *ResultLength = Samples * NewChannels;
        }

    }
    else
    {
        PUSHORT BufferOut = ExAllocatePool(NonPagedPool, Samples * NewChannels);
        if (!BufferOut)
            return STATUS_INSUFFICIENT_RESOURCES;

        for(NewIndex = 0, OldIndex = 0; OldIndex < Samples * OldChannels; NewIndex += NewChannels, OldIndex += OldChannels)
        {
            /* TODO
             * mix stream instead of just dumping part of it ;)
             */
            RtlMoveMemory(&BufferOut[NewIndex], &Buffer[OldIndex], NewChannels * (BitsPerSample/8));
        }

        *Result = BufferOut;
        *ResultLength = Samples * NewChannels;
    }
    return STATUS_SUCCESS;
}


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
            ExFreePool(StreamHeader->Data);
            StreamHeader->Data = BufferOut;
            StreamHeader->DataUsed = BufferLength;
        }
    }

    if (InputFormat->WaveFormatEx.nChannels != OutputFormat->WaveFormatEx.nChannels)
    {
        Status = PerformChannelConversion(StreamHeader->Data,
                                          StreamHeader->DataUsed,
                                          InputFormat->WaveFormatEx.nChannels,
                                          OutputFormat->WaveFormatEx.nChannels,
                                          OutputFormat->WaveFormatEx.wBitsPerSample,
                                          &BufferOut,
                                          &BufferLength);

        if (NT_SUCCESS(Status))
        {
            ExFreePool(StreamHeader->Data);
            StreamHeader->Data = BufferOut;
            StreamHeader->DataUsed = BufferLength;
        }
    }

    if (InputFormat->WaveFormatEx.nSamplesPerSec != OutputFormat->WaveFormatEx.nSamplesPerSec)
    {
        Status = PerformSampleRateConversion(StreamHeader->Data,
                                             StreamHeader->DataUsed,
                                             InputFormat->WaveFormatEx.nSamplesPerSec,
                                             OutputFormat->WaveFormatEx.nSamplesPerSec,
                                             OutputFormat->WaveFormatEx.wBitsPerSample / 8,
                                             OutputFormat->WaveFormatEx.nChannels,
                                             &BufferOut,
                                             &BufferLength);
        if (NT_SUCCESS(Status))
        {
            ExFreePool(StreamHeader->Data);
            StreamHeader->Data = BufferOut;
            StreamHeader->DataUsed = BufferLength;
        }
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

void *memset(
   void* dest,
   int c,
   size_t count)
{
    ULONG Index;
    PUCHAR Block = (PUCHAR)dest;

    for(Index = 0; Index < count; Index++)
        Block[Index] = c;

    return dest;
}

void * memcpy(
   void* dest,
   const void* src,
   size_t count)
{
    ULONG Index;
    PUCHAR Src = (PUCHAR)src, Dest = (PUCHAR)dest;

    for(Index = 0; Index < count; Index++)
        Dest[Index] = Src[Index];
    return dest;
}
