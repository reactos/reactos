/*
 * PROJECT:     ReactOS Sound System
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/wdmaud.drv/mixer.c
 *
 * PURPOSE:     WDM Audio Driver (User-mode part)
 * PROGRAMMERS: Johannes Anderwald
 */

#include "wdmaud.h"

#include <samplerate.h>
#include <float_cast.h>

#define NDEBUG
#include <debug.h>
#include <mmebuddy_debug.h>

extern HANDLE KernelHandle;

DWORD
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
    ULONG Index;
    SRC_STATE * State;
    SRC_DATA Data;
    PUCHAR ResultOut;
    int error;
    PFLOAT FloatIn, FloatOut;
    ULONG NumSamples;
    ULONG NewSamples;

    //SND_TRACE(L"PerformSampleRateConversion OldRate %u NewRate %u BytesPerSample %u NumChannels %u\n", OldRate, NewRate, BytesPerSample, NumChannels);

    ASSERT(BytesPerSample == 1 || BytesPerSample == 2 || BytesPerSample == 4);

    NumSamples = BufferLength / (BytesPerSample * NumChannels);

    FloatIn = HeapAlloc(GetProcessHeap(), 0, NumSamples * NumChannels * sizeof(FLOAT));
    if (!FloatIn)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    NewSamples = lrintf(((FLOAT)NumSamples * ((FLOAT)NewRate / (FLOAT)OldRate))) + 2;

    FloatOut = HeapAlloc(GetProcessHeap(), 0, NewSamples * NumChannels * sizeof(FLOAT));
    if (!FloatOut)
    {
        HeapFree(GetProcessHeap(), 0,FloatIn);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ResultOut = HeapAlloc(GetProcessHeap(), 0, NewSamples * NumChannels * BytesPerSample);
    if (!ResultOut)
    {
        HeapFree(GetProcessHeap(), 0,FloatIn);
        HeapFree(GetProcessHeap(), 0,FloatOut);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    State = src_new(SRC_SINC_FASTEST, NumChannels, &error);
    if (!State)
    {
        HeapFree(GetProcessHeap(), 0,FloatIn);
        HeapFree(GetProcessHeap(), 0,FloatOut);
        HeapFree(GetProcessHeap(), 0,ResultOut);
        return ERROR_NOT_ENOUGH_MEMORY;
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
        HeapFree(GetProcessHeap(), 0,FloatIn);
        HeapFree(GetProcessHeap(), 0,FloatOut);
        HeapFree(GetProcessHeap(), 0,ResultOut);
        return ERROR_INVALID_DATA;
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

        src_float_to_short_array(FloatOut, (short*)Res, Data.output_frames_gen * NumChannels);
    }
    else if (BytesPerSample == 4)
    {
        PULONG Res = (PULONG)ResultOut;

        src_float_to_int_array(FloatOut, (int*)Res, Data.output_frames_gen * NumChannels);
    }


    *Result = ResultOut;
    *ResultLength = Data.output_frames_gen * BytesPerSample * NumChannels;
    HeapFree(GetProcessHeap(), 0,FloatIn);
    HeapFree(GetProcessHeap(), 0,FloatOut);
    src_delete(State);
    return ERROR_SUCCESS;
}

DWORD
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

    SND_TRACE(L"PerformChannelConversion OldChannels %u NewChannels %u\n", OldChannels, NewChannels);

    if (NewChannels > OldChannels)
    {
        if (BitsPerSample == 8)
        {
            PUCHAR BufferOut = HeapAlloc(GetProcessHeap(), 0, Samples * NewChannels);
            if (!BufferOut)
                return ERROR_NOT_ENOUGH_MEMORY;

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
            PUSHORT BufferOut = HeapAlloc(GetProcessHeap(), 0, Samples * NewChannels);
            if (!BufferOut)
                return ERROR_NOT_ENOUGH_MEMORY;

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
            PUCHAR BufferOut = HeapAlloc(GetProcessHeap(), 0, Samples * NewChannels);
            if (!BufferOut)
                return ERROR_NOT_ENOUGH_MEMORY;

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
            PULONG BufferOut = HeapAlloc(GetProcessHeap(), 0, Samples * NewChannels);
            if (!BufferOut)
                return ERROR_NOT_ENOUGH_MEMORY;

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
        PUSHORT BufferOut = HeapAlloc(GetProcessHeap(), 0, Samples * NewChannels);
        if (!BufferOut)
            return ERROR_NOT_ENOUGH_MEMORY;

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
    return ERROR_SUCCESS;
}


DWORD
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

    //SND_TRACE(L"PerformQualityConversion OldWidth %u NewWidth %u\n", OldWidth, NewWidth);

    if (OldWidth == 8 && NewWidth == 16)
    {
         USHORT Sample;
         PUSHORT BufferOut = HeapAlloc(GetProcessHeap(), 0, Samples * sizeof(USHORT));
         if (!BufferOut)
             return ERROR_NOT_ENOUGH_MEMORY;

          for(Index = 0; Index < Samples; Index++)
          {
              Sample = Buffer[Index];// & 0xFF);
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
         PULONG BufferOut = HeapAlloc(GetProcessHeap(), 0, Samples * sizeof(ULONG));
         if (!BufferOut)
             return ERROR_NOT_ENOUGH_MEMORY;

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
         PULONG BufferOut = HeapAlloc(GetProcessHeap(), 0, Samples * sizeof(ULONG));
         if (!BufferOut)
             return ERROR_NOT_ENOUGH_MEMORY;

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
         PUCHAR BufferOut = HeapAlloc(GetProcessHeap(), 0, Samples * sizeof(UCHAR));
         if (!BufferOut)
             return ERROR_NOT_ENOUGH_MEMORY;

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
         PUCHAR BufferOut = HeapAlloc(GetProcessHeap(), 0, Samples * sizeof(UCHAR));
         if (!BufferOut)
             return ERROR_NOT_ENOUGH_MEMORY;

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
         PUSHORT BufferOut = HeapAlloc(GetProcessHeap(), 0, Samples * sizeof(USHORT));
         if (!BufferOut)
             return ERROR_NOT_ENOUGH_MEMORY;

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
        return ERROR_NOT_SUPPORTED;
    }

    return ERROR_SUCCESS;
}

VOID
CALLBACK
MixerCompletionRoutine(
    IN  DWORD dwErrorCode,
    IN  DWORD dwNumberOfBytesTransferred,
    IN  LPOVERLAPPED lpOverlapped)
{
    PSOUND_OVERLAPPED Overlap = (PSOUND_OVERLAPPED)lpOverlapped;

    /* Call mmebuddy overlap routine */
    Overlap->OriginalCompletionRoutine(dwErrorCode, PtrToUlong(Overlap->CompletionContext), lpOverlapped);
}

MMRESULT
WriteFileEx_Remixer(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID OffsetPtr,
    IN  DWORD Length,
    IN  PSOUND_OVERLAPPED Overlap,
    IN  LPOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine)
{
    HANDLE Handle;
    WDMAUD_DEVICE_INFO DeviceInfo;
    DWORD BufferLength, BufferLengthTemp;
    PVOID BufferOut, BufferOutTemp;
    DWORD Status;
    BOOL Result;

    VALIDATE_MMSYS_PARAMETER( SoundDeviceInstance );
    VALIDATE_MMSYS_PARAMETER( OffsetPtr );
    VALIDATE_MMSYS_PARAMETER( Overlap );
    VALIDATE_MMSYS_PARAMETER( CompletionRoutine );

    GetSoundDeviceInstanceHandle(SoundDeviceInstance, &Handle);

    SND_ASSERT(Handle);

    BufferOut = OffsetPtr;
    BufferLength = Length;

    if (SoundDeviceInstance->WaveFormatEx.wBitsPerSample != 16)
    {
        Status = PerformQualityConversion(OffsetPtr,
                                          Length,
                                          SoundDeviceInstance->WaveFormatEx.wBitsPerSample,
                                          16,
                                          &BufferOut,
                                          &BufferLength);
        if (Status)
        {
            SND_TRACE(L"PerformQualityConversion failed\n");
            return MMSYSERR_NOERROR;
        }
    }

    if (SoundDeviceInstance->WaveFormatEx.nChannels != 2)
    {
        Status = PerformChannelConversion(BufferOut,
                                          BufferLength,
                                          SoundDeviceInstance->WaveFormatEx.nChannels,
                                          2,
                                          16,
                                          &BufferOutTemp,
                                          &BufferLengthTemp);

        if (BufferOut != OffsetPtr)
        {
            HeapFree(GetProcessHeap(), 0, BufferOut);
        }

        if (Status)
        {
            SND_TRACE(L"PerformChannelConversion failed\n");
            return MMSYSERR_NOERROR;
        }

        BufferOut = BufferOutTemp;
        BufferLength = BufferLengthTemp;
    }

    if (SoundDeviceInstance->WaveFormatEx.nSamplesPerSec != 44100)
    {
        Status = PerformSampleRateConversion(BufferOut,
                                             BufferLength,
                                             SoundDeviceInstance->WaveFormatEx.nSamplesPerSec,
                                             44100,
                                             2,
                                             2,
                                             &BufferOutTemp,
                                             &BufferLengthTemp);

        if (BufferOut != OffsetPtr)
        {
            HeapFree(GetProcessHeap(), 0, BufferOut);
        }

        if (Status)
        {
            SND_TRACE(L"PerformSampleRateConversion failed\n");
            return MMSYSERR_NOERROR;
        }

        BufferOut = BufferOutTemp;
        BufferLength = BufferLengthTemp;
    }

    ZeroMemory(&DeviceInfo, sizeof(WDMAUD_DEVICE_INFO));
    DeviceInfo.hDevice = Handle;
    DeviceInfo.DeviceType = WAVE_OUT_DEVICE_TYPE; //FIXME
    DeviceInfo.Header.FrameExtent = BufferLength;
    DeviceInfo.Header.DataUsed = BufferLength;
    DeviceInfo.Header.Data = BufferOut;
    DeviceInfo.Header.Size = sizeof(KSSTREAM_HEADER);
    DeviceInfo.Header.PresentationTime.Numerator = 1;
    DeviceInfo.Header.PresentationTime.Denominator = 1;

    Overlap->CompletionContext = UlongToPtr(Length);
    Overlap->OriginalCompletionRoutine = CompletionRoutine;

    Overlap->Standard.hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);

    //SND_TRACE(L"OriginalLength %u NewLength %u\n", Length, BufferLength);

#if 0
    Result = WriteFileEx(KernelHandle, &DeviceInfo, sizeof(WDMAUD_DEVICE_INFO), (LPOVERLAPPED)Overlap, CompletionRoutine);
#else
    Result = WriteFileEx(KernelHandle, &DeviceInfo, sizeof(WDMAUD_DEVICE_INFO), (LPOVERLAPPED)Overlap, MixerCompletionRoutine);
#endif

    if ( ! Result )
    {
        SND_TRACE(L"WriteFileEx failed with %x\n", GetLastError());
        return MMSYSERR_NOERROR;
    }

    WaitForSingleObjectEx (KernelHandle, INFINITE, TRUE);

#ifdef USERMODE_MIXER
       // if (BufferOut != OffsetPtr)
       //     HeapFree(GetProcessHeap(), 0, BufferOut);
#endif


    return MMSYSERR_NOERROR;
}

