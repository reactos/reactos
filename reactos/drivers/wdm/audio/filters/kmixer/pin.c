/*
 * PROJECT:         ReactOS Kernel Streaming Mixer
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/wdm/audio/filters/kmixer/kmixer.c
 * PURPOSE:         Pin functions
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */

#include "kmixer.h"



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

    /* first acquire float save context */
    Status = KeSaveFloatingPointState(&FloatSave);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("KeSaveFloatingPointState failed with %x\n", Status);
        return Status;
    }

    NumSamples = BufferLength / BytesPerSample;

    FloatIn = ExAllocatePool(NonPagedPool, NumSamples * sizeof(FLOAT));
    if (!FloatIn)
    {
        KeRestoreFloatingPointState(&FloatSave);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NewSamples = lrintf(((FLOAT)NumSamples * ((FLOAT)OldRate / (FLOAT)NewRate))) + 1;

    FloatOut = ExAllocatePool(NonPagedPool, NewSamples * sizeof(FLOAT));
    if (!FloatOut)
    {
        ExFreePool(FloatIn);
        KeRestoreFloatingPointState(&FloatSave);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ResultOut = ExAllocatePool(NonPagedPool, NewRate * BytesPerSample * NumChannels);
    if (!FloatOut)
    {
        ExFreePool(FloatIn);
        ExFreePool(FloatOut);
        KeRestoreFloatingPointState(&FloatSave);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    State = src_new(SRC_LINEAR, NumChannels, &error);
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
        for(Index = 0; Index < NumSamples; Index++)
            FloatIn[Index] = (float)Buffer[Index];
    }
    else if (BytesPerSample == 2)
    {
        PUSHORT Res = (PUSHORT)ResultOut;
        for(Index = 0; Index < NumSamples; Index++)
            FloatIn[Index] = (float)Res[Index];
    }
    else
    {
        UNIMPLEMENTED
        KeRestoreFloatingPointState(&FloatSave);
        ExFreePool(FloatIn);
        ExFreePool(FloatOut);
        ExFreePool(ResultOut);
        return STATUS_UNSUCCESSFUL;
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
        for(Index = 0; Index < Data.output_frames_gen; Index++)
            ResultOut[Index] = lrintf(FloatOut[Index]);
    }
    else if (BytesPerSample == 2)
    {
        PUSHORT Res = (PUSHORT)ResultOut;
        for(Index = 0; Index < Data.output_frames_gen; Index++)
            Res[Index] = lrintf(FloatOut[Index]);
    }

    *Result = ResultOut;
    *ResultLength = NewRate * BytesPerSample * NumChannels;
    ExFreePool(FloatIn);
    ExFreePool(FloatOut);
    KeRestoreFloatingPointState(&FloatSave);
    return STATUS_SUCCESS;
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



