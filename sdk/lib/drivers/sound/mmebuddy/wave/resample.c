/*
 * PROJECT:     ReactOS Sound Subsystem
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WDM Audio Driver resampling support
 * COPYRIGHT:   Copyright 2009-2010 Johannes Anderwald
 *              Copyright 2026 Oleg Dubinskiy (oleg.dubinskiy@reactos.org)
 */

#include "precomp.h"
#include <main.c>

#include <debug.h>

DWORD
PerformSampleRateConversion(
    PVOID Buffer,
    ULONG BufferLength,
    ULONG OldRate,
    ULONG NewRate,
    ULONG NumChannels,
    ULONG BitsPerSample,
    ULONG BytesPerSample,
    PVOID * Result,
    PULONG ResultLength)
{
    ULONG OldSampleCount;

    DPRINT("PerformSampleRateConversion OldRate %u NewRate %u BytesPerSample %u NumChannels %u\n", OldRate, NewRate, BytesPerSample, NumChannels);

    ASSERT(OldRate != NewRate);
    ASSERT(BytesPerSample <= 8);

    OldSampleCount = BufferLength / BytesPerSample * NumChannels;

    if (BitsPerSample == 8)
    {
        const uint8_t* BufferIn = (const uint8_t*)Buffer;
        ULONG NewSampleCount = Resample_u8(BufferIn, NULL, OldRate, NewRate, OldSampleCount, NumChannels);
        uint8_t* BufferOut = AllocateMemory(NewSampleCount * sizeof(uint8_t));
        if (!BufferOut)
            return ERROR_NOT_ENOUGH_MEMORY;

        Resample_u8(BufferIn,
                    BufferOut,
                    OldRate,
                    NewRate,
                    OldSampleCount,
                    NumChannels);

        *Result = BufferOut;
        *ResultLength = NewSampleCount * sizeof(uint8_t);
    }
    else if (BitsPerSample == 16)
    {
        const int16_t* BufferIn = (const int16_t*)Buffer;
        ULONG NewSampleCount = Resample_s16(BufferIn, NULL, OldRate, NewRate, OldSampleCount, NumChannels);
        int16_t* BufferOut = AllocateMemory(NewSampleCount * sizeof(int16_t));
        if (!BufferOut)
            return ERROR_NOT_ENOUGH_MEMORY;

        Resample_s16(BufferIn,
                     BufferOut,
                     OldRate,
                     NewRate,
                     OldSampleCount,
                     NumChannels);

        *Result = BufferOut;
        *ResultLength = NewSampleCount * sizeof(int16_t);
    }
    else if (BitsPerSample == 32)
    {
        const float* BufferIn = (const float*)Buffer;
        ULONG NewSampleCount = Resample_f32(BufferIn, NULL, OldRate, NewRate, OldSampleCount, NumChannels);
        float* BufferOut = AllocateMemory(NewSampleCount * sizeof(float));
        if (!BufferOut)
            return ERROR_NOT_ENOUGH_MEMORY;

        Resample_f32(BufferIn,
                     BufferOut,
                     OldRate,
                     NewRate,
                     OldSampleCount,
                     NumChannels);

        *Result = BufferOut;
        *ResultLength = NewSampleCount * sizeof(float);
    }
    else
    {
        DPRINT1("Not implemented conversion BitsPerSample %u OldRate %u NewRate %u\n", BitsPerSample, OldRate, NewRate);
        return ERROR_NOT_SUPPORTED;
    }

    return ERROR_SUCCESS;
}

DWORD
PerformChannelConversion(
    PVOID Buffer,
    ULONG BufferLength,
    ULONG OldChannels,
    ULONG NewChannels,
    ULONG BitsPerSample,
    PVOID * Result,
    PULONG ResultLength)
{
    ULONG Channel, Index, SampleCount;

    DPRINT("PerformChannelConversion OldChannels %u NewChannels %u\n", OldChannels, NewChannels);

    ASSERT(OldChannels != NewChannels);

    SampleCount = BufferLength / (BitsPerSample / 8) / OldChannels;

    if (BitsPerSample == 8)
    {
        uint8_t* BufferIn = (uint8_t*)Buffer;
        uint8_t* BufferOut = AllocateMemory(SampleCount * NewChannels * sizeof(uint8_t));
        if (!BufferOut)
            return ERROR_NOT_ENOUGH_MEMORY;

        for (Index = 0; Index < SampleCount; Index++)
        {
            if (NewChannels > OldChannels)
            {
                for (Channel = 0; Channel < NewChannels; Channel++)
                {
                    BufferOut[Index * NewChannels + Channel] =
                        BufferIn[Index * OldChannels + (Channel % OldChannels)];
                }
            }
            else
            {
                ULONG Sum = 0;
                uint8_t Sample;

                for (Channel = 0; Channel < OldChannels; Channel++)
                {
                    Sum += BufferIn[Index * OldChannels + Channel];
                }

                Sample = (uint8_t)(Sum / OldChannels);

                for (Channel = 0; Channel < NewChannels; Channel++)
                {
                    BufferOut[Index * NewChannels + Channel] = Sample;
                }
            }
        }

        *Result = BufferOut;
        *ResultLength = SampleCount * NewChannels * sizeof(uint8_t);
    }
    else if (BitsPerSample == 16)
    {
        int16_t* BufferIn = (int16_t*)Buffer;
        int16_t* BufferOut = AllocateMemory(SampleCount * NewChannels * sizeof(int16_t));
        if (!BufferOut)
            return ERROR_NOT_ENOUGH_MEMORY;

        for (Index = 0; Index < SampleCount; Index++)
        {
            if (NewChannels > OldChannels)
            {
                for (Channel = 0; Channel < NewChannels; Channel++)
                {
                    BufferOut[Index * NewChannels + Channel] =
                        BufferIn[Index * OldChannels + (Channel % OldChannels)];
                }
            }
            else
            {
                LONG Sum = 0;
                int16_t Sample;

                for (Channel = 0; Channel < OldChannels; Channel++)
                {
                    Sum += BufferIn[Index * OldChannels + Channel];
                }

                Sample = (int16_t)(Sum / (LONG)OldChannels);

                for (Channel = 0; Channel < NewChannels; Channel++)
                {
                    BufferOut[Index * NewChannels + Channel] = Sample;
                }
            }
        }

        *Result = BufferOut;
        *ResultLength = SampleCount * NewChannels * sizeof(int16_t);
    }
    else if (BitsPerSample == 32)
    {
        int32_t* BufferIn = (int32_t*)Buffer;
        int32_t* BufferOut = AllocateMemory(SampleCount * NewChannels * sizeof(int32_t));
        if (!BufferOut)
            return ERROR_NOT_ENOUGH_MEMORY;

        for (Index = 0; Index < SampleCount; Index++)
        {
            if (NewChannels > OldChannels)
            {
                for (Channel = 0; Channel < NewChannels; Channel++)
                {
                    BufferOut[Index * NewChannels + Channel] =
                        BufferIn[Index * OldChannels + (Channel % OldChannels)];
                }
            }
            else
            {
                LONGLONG Sum = 0;
                int32_t Sample;

                for (Channel = 0; Channel < OldChannels; Channel++)
                {
                    Sum += BufferIn[Index * OldChannels + Channel];
                }

                Sample = (int32_t)(Sum / (LONGLONG)OldChannels);

                for (Channel = 0; Channel < NewChannels; Channel++)
                {
                    BufferOut[Index * NewChannels + Channel] = Sample;
                }
            }
        }

        *Result = BufferOut;
        *ResultLength = SampleCount * NewChannels * sizeof(int32_t);
    }
    else
    {
        DPRINT1("Not implemented conversion BitsPerSample %u OldChannels %u NewChannels %u\n", BitsPerSample, OldChannels, NewChannels);
        return ERROR_NOT_SUPPORTED;
    }
    return ERROR_SUCCESS;
}

DWORD
PerformQualityConversion(
    PVOID Buffer,
    ULONG BufferLength,
    ULONG OldWidth,
    ULONG NewWidth,
    USHORT FormatTag,
    PVOID * Result,
    PULONG ResultLength)
{
    ULONG Index, SampleCount;

    ASSERT(OldWidth != NewWidth);

    SampleCount = BufferLength / (OldWidth / 8);

    DPRINT("PerformQualityConversion OldWidth %u NewWidth %u\n", OldWidth, NewWidth);
    DPRINT("SampleCount %u BufferLength %u\n", SampleCount, BufferLength);

    if (OldWidth == 8 && NewWidth == 16)
    {
        const drwav_uint8* BufferIn = (const drwav_uint8*)Buffer;
        drwav_int16* BufferOut = AllocateMemory(SampleCount * sizeof(drwav_int16));
        if (!BufferOut)
            return ERROR_NOT_ENOUGH_MEMORY;
        drwav_u8_to_s16(BufferOut, BufferIn, SampleCount);
        *Result = BufferOut;
        *ResultLength = SampleCount * sizeof(drwav_int16);
    }
    else if (OldWidth == 8 && NewWidth == 32)
    {
        const drwav_uint8* BufferIn = (const drwav_uint8*)Buffer;
        drwav_int32* BufferOut = AllocateMemory(SampleCount * sizeof(drwav_int32));
        if (!BufferOut)
            return ERROR_NOT_ENOUGH_MEMORY;
        drwav_u8_to_s32(BufferOut, BufferIn, SampleCount);
        *Result = BufferOut;
        *ResultLength = SampleCount * sizeof(drwav_int32);
    }
    else if (OldWidth == 16 && NewWidth == 32)
    {
        const drwav_int16* BufferIn = (const drwav_int16*)Buffer;
        drwav_int32* BufferOut = AllocateMemory(SampleCount * sizeof(drwav_int32));
        if (!BufferOut)
            return ERROR_NOT_ENOUGH_MEMORY;
        if (FormatTag == WAVE_FORMAT_IEEE_FLOAT)
            drwav_s16_to_f32((float*)BufferOut, BufferIn, SampleCount);
        else
            drwav_s16_to_s32(BufferOut, BufferIn, SampleCount);
        *Result = BufferOut;
        *ResultLength = SampleCount * sizeof(drwav_int32);
    }
    else if (OldWidth == 16 && NewWidth == 8)
    {
        USHORT Sample;
        PUSHORT BufferIn = (PUSHORT)Buffer;
        PUCHAR BufferOut = AllocateMemory(SampleCount * sizeof(UCHAR));
        if (!BufferOut)
            return ERROR_NOT_ENOUGH_MEMORY;

        for (Index = 0; Index < SampleCount; Index++)
        {
            Sample = BufferIn[Index];
#ifdef _X86_
            Sample = _byteswap_ushort(Sample);
#endif
            Sample /= 256;
            BufferOut[Index] = (Sample & 0xFF);
        }
        *Result = BufferOut;
        *ResultLength = SampleCount * sizeof(UCHAR);
    }
    else if (OldWidth == 32 && NewWidth == 8)
    {
        ULONG Sample;
        PULONG BufferIn = (PULONG)Buffer;
        PUCHAR BufferOut = AllocateMemory(SampleCount * sizeof(UCHAR));
        if (!BufferOut)
            return ERROR_NOT_ENOUGH_MEMORY;

        for (Index = 0; Index < SampleCount; Index++)
        {
            Sample = BufferIn[Index];
#ifdef _X86_
            Sample = _byteswap_ulong(Sample);
#endif
            Sample /= 16777216;
            BufferOut[Index] = (Sample & 0xFF);
        }
        *Result = BufferOut;
        *ResultLength = SampleCount * sizeof(UCHAR);
    }
    else if (OldWidth == 32 && NewWidth == 16)
    {
        const drwav_int32* BufferIn = (const drwav_int32*)Buffer;
        drwav_int16* BufferOut = AllocateMemory(SampleCount * sizeof(drwav_int16));
        if (!BufferOut)
            return ERROR_NOT_ENOUGH_MEMORY;
        if (FormatTag == WAVE_FORMAT_IEEE_FLOAT)
            drwav_f32_to_s16(BufferOut, (const float*)BufferIn, SampleCount);
        else
            drwav_s32_to_s16(BufferOut, BufferIn, SampleCount);
        *Result = BufferOut;
        *ResultLength = SampleCount * sizeof(drwav_int16);
    }
    else
    {
        DPRINT1("Not implemented conversion OldWidth %u NewWidth %u\n", OldWidth, NewWidth);
        return ERROR_NOT_SUPPORTED;
    }

    return ERROR_SUCCESS;
}

MMRESULT
MmeResampleStream(
    _In_ PWAVEFORMATEX WaveFormatEx,
    _In_ PKSDATARANGE_AUDIO DataRange,
    _In_ DWORD InputLength,
    _In_ PVOID InputBuffer,
    _Out_ PDWORD OutputLength,
    _Out_ PVOID* OutputBuffer)
{
    DWORD BufferLength, BufferLengthTemp;
    PVOID BufferOut, BufferOutTemp;
    DWORD Status, BytesPerSample;

    /* Get input data */
    BufferOut = InputBuffer;
    BufferLength = InputLength;

    /* Do the resampling */
    if (WaveFormatEx->wBitsPerSample != DataRange->MaximumBitsPerSample)
    {
        Status = PerformQualityConversion(InputBuffer,
                                          InputLength,
                                          WaveFormatEx->wBitsPerSample,
                                          DataRange->MaximumBitsPerSample,
                                          WaveFormatEx->wFormatTag,
                                          &BufferOut,
                                          &BufferLength);
        if (Status)
        {
            DPRINT("PerformQualityConversion failed\n");
            return MMSYSERR_ERROR;
        }
    }

    if (WaveFormatEx->nChannels != DataRange->MaximumChannels)
    {
        Status = PerformChannelConversion(BufferOut,
                                          BufferLength,
                                          WaveFormatEx->nChannels,
                                          DataRange->MaximumChannels,
                                          WaveFormatEx->wBitsPerSample,
                                          &BufferOutTemp,
                                          &BufferLengthTemp);
        if (BufferLength != InputLength)
            FreeMemory(BufferOut);
        if (Status)
        {
            DPRINT("PerformChannelConversion failed\n");
            return MMSYSERR_ERROR;
        }

        BufferOut = BufferOutTemp;
        BufferLength = BufferLengthTemp;
    }

    if (WaveFormatEx->nSamplesPerSec > DataRange->MaximumSampleFrequency)
    {
        /* Calculate bytes per sample */
        BytesPerSample = (WaveFormatEx->wBitsPerSample / 8) * WaveFormatEx->nChannels;

        Status = PerformSampleRateConversion(BufferOut,
                                             BufferLength,
                                             WaveFormatEx->nSamplesPerSec,
                                             DataRange->MaximumSampleFrequency,
                                             WaveFormatEx->nChannels,
                                             WaveFormatEx->wBitsPerSample,
                                             BytesPerSample,
                                             &BufferOutTemp,
                                             &BufferLengthTemp);
        if (BufferLength != InputLength)
            FreeMemory(BufferOut);
        if (Status)
        {
            DPRINT("PerformSampleRateConversion failed\n");
            return MMSYSERR_ERROR;
        }

        BufferOut = BufferOutTemp;
        BufferLength = BufferLengthTemp;
    }

    DPRINT("OriginalLength %u NewLength %u\n", InputLength, BufferLength);

    /* Set output data */
    *OutputBuffer = BufferOut;
    *OutputLength = BufferLength;

    return MMSYSERR_NOERROR;
}
