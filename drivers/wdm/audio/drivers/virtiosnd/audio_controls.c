/*
 * PROJECT:     ReactOS VirtIO Sound Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Master volume and mute controls
 * COPYRIGHT:   Copyright 2026 Ralf Lang <ralf.lang@ralf-lang.de>
 */

/**
 * @file
 * @brief Master volume and mute controls for VirtIO Sound Driver
 *
 * This file implements the master volume and mute functionality for the VirtIO sound driver.
 *
 * @copyright Copyright 2026 Ralf Lang <ralf.lang@ralf-lang.de>
 * @license GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 */

#include "virtiosnd.h"

/**
 * @brief Minimum volume level in hundredths of dB
 */
#define VIRTIO_SND_VOLUME_MINIMUM (-9600)

/**
 * @brief Maximum volume level in hundredths of dB
 */
#define VIRTIO_SND_VOLUME_MAXIMUM 0

/**
 * @brief Volume step size in hundredths of dB
 */
#define VIRTIO_SND_VOLUME_STEP 100

/**
 * @brief Gain value for 1 dB in Q16 format
 */
#define VIRTIO_SND_GAIN_ONE_DB_Q16 58410

/**
 * @brief Global variable storing the master volume level
 */
static volatile LONG VirtioSndMasterVolume;

/**
 * @brief Global variable storing the master mute state
 */
static volatile LONG VirtioSndMasterMute;

/**
 * @brief Gets the current master volume level
 *
 * Retrieves the current master volume level in hundredths of dB.
 *
 * @return LONG Current master volume level in hundredths of dB
 */
LONG
VirtioSndGetMasterVolume(VOID)
{
    return InterlockedCompareExchange(&VirtioSndMasterVolume, 0, 0);
}

/**
 * @brief Sets the master volume level
 *
 * Sets the master volume level in hundredths of dB.
 *
 * @param VolumeLevel Volume level to set in hundredths of dB
 * @return NTSTATUS Status of the operation
 */
NTSTATUS
VirtioSndSetMasterVolume(_In_ LONG VolumeLevel)
{
    if (VolumeLevel < VIRTIO_SND_VOLUME_MINIMUM || VolumeLevel > VIRTIO_SND_VOLUME_MAXIMUM ||
        VolumeLevel % VIRTIO_SND_VOLUME_STEP != 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    InterlockedExchange(&VirtioSndMasterVolume, VolumeLevel);
    return STATUS_SUCCESS;
}

/**
 * @brief Gets the current master mute state
 *
 * Retrieves the current master mute state.
 *
 * @return BOOLEAN TRUE if muted or FALSE if not muted
 */
BOOLEAN
VirtioSndGetMasterMute(VOID)
{
    return InterlockedCompareExchange(&VirtioSndMasterMute, 0, 0) != 0;
}

/**
 * @brief Sets the master mute state
 *
 * Sets the master mute state.
 *
 * @param Muted TRUE to mute or FALSE to unmute
 */
VOID
VirtioSndSetMasterMute(_In_ BOOLEAN Muted)
{
    InterlockedExchange(&VirtioSndMasterMute, Muted ? TRUE : FALSE);
}

/**
 * @brief Applies master volume and mute controls to audio data
 *
 * This function applies the current master volume level and mute state to the audio data.
 * If muted, the audio data gets zeroed. If volume is less than maximum, it applies attenuation.
 *
 * @param Audio Pointer to the audio data buffer
 * @param Length Length of the audio data in bytes
 * @param BitsPerSample Number of bits per sample (16 or 32)
 */
VOID
VirtioSndApplyMasterControls(_Inout_updates_bytes_(Length) PVOID Audio, _In_ ULONG Length, _In_ ULONG BitsPerSample)
{
    LONG VolumeLevel;
    LONG Gain;
    LONG Step;
    ULONG SampleCount;
    ULONG SampleIndex;

    if (Audio == NULL || Length == 0)
        return;

    if (VirtioSndGetMasterMute())
    {
        RtlZeroMemory(Audio, Length);
        return;
    }

    VolumeLevel = VirtioSndGetMasterVolume();
    if (VolumeLevel == VIRTIO_SND_VOLUME_MAXIMUM)
        return;

    Gain = 1 << 15;
    for (Step = 0; Step < -VolumeLevel / VIRTIO_SND_VOLUME_STEP; ++Step)
    {
        Gain = (Gain * VIRTIO_SND_GAIN_ONE_DB_Q16 + (1 << 15)) >> 16;
    }

    if (BitsPerSample == 16)
    {
        PSHORT Samples = (PSHORT)Audio;

        SampleCount = Length / sizeof(*Samples);
        for (SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
        {
            Samples[SampleIndex] = (SHORT)(((LONG)Samples[SampleIndex] * Gain) / (1 << 15));
        }
    }
    else if (BitsPerSample == 32)
    {
        PLONG Samples = (PLONG)Audio;

        SampleCount = Length / sizeof(*Samples);
        for (SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
        {
            Samples[SampleIndex] = (LONG)(((LONGLONG)Samples[SampleIndex] * Gain) / (1 << 15));
        }
    }
}
