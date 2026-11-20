/*
 * PROJECT:     ReactOS Sound Subsystem
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WDM Audio Driver resampling support
 * COPYRIGHT:   Copyright 2009-2010 Johannes Anderwald
 *              Copyright 2026 Oleg Dubinskiy <oleg.dubinskiy@reactos.org>
 */

#include "precomp.h"
#include <main.c>

#include <debug.h>

#define SAMPLE_RATE_CONVERSION(type, func)                                      \
do {                                                                            \
    const type* BufferIn = (const type*)Buffer;                                 \
    ULONG NewSampleCount = func(BufferIn, NULL, OldRate, NewRate,               \
                                OldSampleCount, NumChannels);                   \
    type* BufferOut = AllocateMemory(NewSampleCount * sizeof(type));            \
    if (!BufferOut)                                                             \
        return ERROR_NOT_ENOUGH_MEMORY;                                         \
    func(BufferIn, BufferOut, OldRate, NewRate, OldSampleCount, NumChannels);   \
    *Result = BufferOut;                                                        \
    *ResultLength = NewSampleCount * sizeof(type);                              \
} while (FALSE)

#define CHANNEL_CONVERSION(type, type_sum)                                      \
do {                                                                            \
    const type* BufferIn = (const type*)Buffer;                                 \
    type* BufferOut = AllocateMemory(SampleCount * NewChannels * sizeof(type)); \
    if (!BufferOut)                                                             \
        return ERROR_NOT_ENOUGH_MEMORY;                                         \
    for (Index = 0; Index < SampleCount; Index++)                               \
    {                                                                           \
        if (NewChannels > OldChannels)                                          \
        {                                                                       \
            for (Channel = 0; Channel < NewChannels; Channel++)                 \
            {                                                                   \
                BufferOut[Index * NewChannels + Channel] =                      \
                    BufferIn[Index * OldChannels + (Channel % OldChannels)];    \
            }                                                                   \
        }                                                                       \
        else                                                                    \
        {                                                                       \
            type_sum Sum = 0;                                                   \
            type Sample;                                                        \
            for (Channel = 0; Channel < OldChannels; Channel++)                 \
            {                                                                   \
                Sum += BufferIn[Index * OldChannels + Channel];                 \
            }                                                                   \
            Sample = (type)(Sum / (type_sum)OldChannels);                       \
            for (Channel = 0; Channel < NewChannels; Channel++)                 \
            {                                                                   \
                BufferOut[Index * NewChannels + Channel] = Sample;              \
            }                                                                   \
        }                                                                       \
    }                                                                           \
    *Result = BufferOut;                                                        \
    *ResultLength = SampleCount * NewChannels * sizeof(type);                   \
} while (FALSE)

#define QUALITY_CONVERSION(type_in, type_out, func)                             \
do {                                                                            \
    const type_in* BufferIn = (const type_in*)Buffer;                           \
    type_out* BufferOut = AllocateMemory(SampleCount * sizeof(type_out));       \
    if (!BufferOut)                                                             \
        return ERROR_NOT_ENOUGH_MEMORY;                                         \
    func(BufferOut, BufferIn, SampleCount);                                     \
    *Result = BufferOut;                                                        \
    *ResultLength = SampleCount * sizeof(type_out);                             \
} while (FALSE)

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

    switch (BitsPerSample)
    {
        case 8:
            SAMPLE_RATE_CONVERSION(uint8_t, Resample_u8);
            return ERROR_SUCCESS;
        case 16:
            SAMPLE_RATE_CONVERSION(int16_t, Resample_s16);
            return ERROR_SUCCESS;
        case 32:
            SAMPLE_RATE_CONVERSION(float, Resample_f32);
            return ERROR_SUCCESS;
        default:
            DPRINT1("Not implemented conversion BitsPerSample %u OldRate %u NewRate %u\n", BitsPerSample, OldRate, NewRate);
    }
    return ERROR_NOT_SUPPORTED;
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

    switch (BitsPerSample)
    {
        case 8:
            CHANNEL_CONVERSION(uint8_t, ULONG);
            return ERROR_SUCCESS;
        case 16:
            CHANNEL_CONVERSION(int16_t, LONG);
            return ERROR_SUCCESS;
        case 32:
            CHANNEL_CONVERSION(int32_t, LONGLONG);
            return ERROR_SUCCESS;
        default:
            DPRINT1("Not implemented conversion BitsPerSample %u OldChannels %u NewChannels %u\n", BitsPerSample, OldChannels, NewChannels);
    }
    return ERROR_NOT_SUPPORTED;
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

    switch (NewWidth)
    {
        case 8:
            /*
             * Since dr_wav.h doesn't provide the routines for conversion to
             * 8-bit unsigned PCM (u8 as destination), we're handling these
             * manually here for now.
             */
            if (OldWidth == 16)
            {
                PSHORT BufferIn = (PSHORT)Buffer;
                PUCHAR BufferOut = AllocateMemory(SampleCount * sizeof(UCHAR));
                if (!BufferOut)
                    return ERROR_NOT_ENOUGH_MEMORY;

                for (Index = 0; Index < SampleCount; Index++)
                {
                    /* Convert signed 16-bit (-32768..32767) to unsigned 8-bit (0..255) */
                    BufferOut[Index] = (UCHAR)((BufferIn[Index] >> 8) + 128);
                }
                *Result = BufferOut;
                *ResultLength = SampleCount * sizeof(UCHAR);
            }
            else if (OldWidth == 32)
            {
                if (FormatTag == WAVE_FORMAT_IEEE_FLOAT)
                {
                    float *BufferIn = (float *)Buffer;
                    PUCHAR BufferOut = AllocateMemory(SampleCount * sizeof(UCHAR));
                    if (!BufferOut)
                        return ERROR_NOT_ENOUGH_MEMORY;

                    for (Index = 0; Index < SampleCount; Index++)
                    {
                        float Sample = BufferIn[Index];
                        if (Sample > 1.0f) Sample = 1.0f;
                        if (Sample < -1.0f) Sample = -1.0f;
                        BufferOut[Index] = (UCHAR)((Sample * 127.0f) + 128.0f);
                    }
                    *Result = BufferOut;
                    *ResultLength = SampleCount * sizeof(UCHAR);
                }
                else
                {
                    PLONG BufferIn = (PLONG)Buffer;
                    PUCHAR BufferOut = AllocateMemory(SampleCount * sizeof(UCHAR));
                    if (!BufferOut)
                        return ERROR_NOT_ENOUGH_MEMORY;

                    for (Index = 0; Index < SampleCount; Index++)
                    {
                        /* Convert signed 32-bit to unsigned 8-bit (0..255) */
                        BufferOut[Index] = (UCHAR)((BufferIn[Index] >> 24) + 128);
                    }
                    *Result = BufferOut;
                    *ResultLength = SampleCount * sizeof(UCHAR);
                }
            }
            return ERROR_SUCCESS;
        case 16:
            if (OldWidth == 8)
                QUALITY_CONVERSION(drwav_uint8, drwav_int16, drwav_u8_to_s16);
            else if (OldWidth == 32)
            {
                if (FormatTag == WAVE_FORMAT_IEEE_FLOAT)
                    QUALITY_CONVERSION(float, drwav_int16, drwav_f32_to_s16);
                else
                    QUALITY_CONVERSION(drwav_int32, drwav_int16, drwav_s32_to_s16);
            }
            return ERROR_SUCCESS;
        case 32:
            if (OldWidth == 8)
            {
                if (FormatTag == WAVE_FORMAT_IEEE_FLOAT)
                    QUALITY_CONVERSION(drwav_uint8, float, drwav_u8_to_f32);
                else
                    QUALITY_CONVERSION(drwav_uint8, drwav_int32, drwav_u8_to_s32);
            }
            else if (OldWidth == 16)
            {
                if (FormatTag == WAVE_FORMAT_IEEE_FLOAT)
                    QUALITY_CONVERSION(drwav_int16, float, drwav_s16_to_f32);
                else
                    QUALITY_CONVERSION(drwav_int16, drwav_int32, drwav_s16_to_s32);
            }
            return ERROR_SUCCESS;
        default:
            DPRINT1("Not implemented conversion OldWidth %u NewWidth %u\n", OldWidth, NewWidth);
    }
    return ERROR_NOT_SUPPORTED;
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
    DWORD CurrentBitsPerSample = WaveFormatEx->wBitsPerSample;
    DWORD CurrentChannels = WaveFormatEx->nChannels;

    /* Get input data */
    BufferOut = InputBuffer;
    BufferLength = InputLength;

    /* Do the resampling */
    if (WaveFormatEx->wBitsPerSample != DataRange->MaximumBitsPerSample)
    {
        Status = PerformQualityConversion(InputBuffer,
                                          InputLength,
                                          CurrentBitsPerSample,
                                          DataRange->MaximumBitsPerSample,
                                          WaveFormatEx->wFormatTag,
                                          &BufferOutTemp,
                                          &BufferLengthTemp);
        if (Status)
        {
            DPRINT("PerformQualityConversion failed\n");
            return MMSYSERR_ERROR;
        }

        BufferOut = BufferOutTemp;
        BufferLength = BufferLengthTemp;
        CurrentBitsPerSample = DataRange->MaximumBitsPerSample;
    }

    if (WaveFormatEx->nChannels != DataRange->MaximumChannels)
    {
        Status = PerformChannelConversion(BufferOut,
                                          BufferLength,
                                          CurrentChannels,
                                          DataRange->MaximumChannels,
                                          CurrentBitsPerSample,
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
        CurrentChannels = DataRange->MaximumChannels;
    }

    if (WaveFormatEx->nSamplesPerSec > DataRange->MaximumSampleFrequency)
    {
        /* Calculate bytes per sample */
        BytesPerSample = (WaveFormatEx->wBitsPerSample / 8) * WaveFormatEx->nChannels;

        Status = PerformSampleRateConversion(BufferOut,
                                             BufferLength,
                                             WaveFormatEx->nSamplesPerSec,
                                             DataRange->MaximumSampleFrequency,
                                             CurrentChannels,
                                             CurrentBitsPerSample,
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
