#include "virtiosnd.h"

#define VIRTIO_SND_VOLUME_MINIMUM (-9600)
#define VIRTIO_SND_VOLUME_MAXIMUM 0
#define VIRTIO_SND_VOLUME_STEP 100
#define VIRTIO_SND_GAIN_ONE_DB_Q16 58410

static volatile LONG VirtioSndMasterVolume;
static volatile LONG VirtioSndMasterMute;

LONG
VirtioSndGetMasterVolume(VOID)
{
    return InterlockedCompareExchange(&VirtioSndMasterVolume, 0, 0);
}

NTSTATUS
VirtioSndSetMasterVolume(
    _In_ LONG VolumeLevel)
{
    if (VolumeLevel < VIRTIO_SND_VOLUME_MINIMUM ||
        VolumeLevel > VIRTIO_SND_VOLUME_MAXIMUM ||
        VolumeLevel % VIRTIO_SND_VOLUME_STEP != 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    InterlockedExchange(&VirtioSndMasterVolume, VolumeLevel);
    return STATUS_SUCCESS;
}

BOOLEAN
VirtioSndGetMasterMute(VOID)
{
    return InterlockedCompareExchange(&VirtioSndMasterMute, 0, 0) != 0;
}

VOID
VirtioSndSetMasterMute(
    _In_ BOOLEAN Muted)
{
    InterlockedExchange(&VirtioSndMasterMute, Muted ? TRUE : FALSE);
}

VOID
VirtioSndApplyMasterControls(
    _Inout_updates_bytes_(Length) PVOID Audio,
    _In_ ULONG Length,
    _In_ ULONG BitsPerSample)
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
            Samples[SampleIndex] =
                (SHORT)(((LONG)Samples[SampleIndex] * Gain) / (1 << 15));
        }
    }
    else if (BitsPerSample == 32)
    {
        PLONG Samples = (PLONG)Audio;

        SampleCount = Length / sizeof(*Samples);
        for (SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
        {
            Samples[SampleIndex] = (LONG)(
                ((LONGLONG)Samples[SampleIndex] * Gain) / (1 << 15));
        }
    }
}
