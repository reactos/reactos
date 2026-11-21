#ifndef _VIRTIOSND_H_
#define _VIRTIOSND_H_

#include <ntddk.h>
#include <portcls.h>

#ifdef __cplusplus
extern "C"
{
#endif

    NTSTATUS
    NTAPI
    VirtioSndStartDevice(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp, _In_ PRESOURCELIST ResourceList);

    NTSTATUS
    VirtioSndStartTransport(_In_ PDEVICE_OBJECT DeviceObject, _In_ PRESOURCELIST ResourceList);

    typedef struct _VIRTIO_SND_PLAYBACK VIRTIO_SND_PLAYBACK, *PVIRTIO_SND_PLAYBACK;

    NTSTATUS
    VirtioSndOpenPlayback(
        _In_ ULONG Channels,
        _In_ ULONG BitsPerSample,
        _In_ ULONG SampleRate,
        _In_ PKEVENT WakeEvent,
        _Out_ PVIRTIO_SND_PLAYBACK *Playback);

    NTSTATUS
    VirtioSndStartPlayback(_Inout_ PVIRTIO_SND_PLAYBACK Playback);

    NTSTATUS
    VirtioSndStopPlayback(_Inout_ PVIRTIO_SND_PLAYBACK Playback);

    NTSTATUS
    VirtioSndQueuePlayback(
        _Inout_ PVIRTIO_SND_PLAYBACK Playback,
        _In_reads_bytes_(Length) PVOID Audio,
        _In_ ULONG Length);

    NTSTATUS
    VirtioSndGetPlaybackCompletion(
        _Inout_ PVIRTIO_SND_PLAYBACK Playback,
        _Out_ PULONG DeviceStatus,
        _Out_ PULONG LatencyBytes);

    NTSTATUS
    VirtioSndProcessPlaybackEvents(_Inout_ PVIRTIO_SND_PLAYBACK Playback, _Out_ PULONG XrunCount);

    NTSTATUS
    VirtioSndGetPlaybackChannelMap(_In_ ULONG Channels, _Out_writes_bytes_(Channels) PUCHAR Positions);

    NTSTATUS
    VirtioSndGetPlaybackRates(_In_ ULONG Channels, _In_ ULONG BitsPerSample, _Out_ ULONGLONG *Rates);

    VOID
    VirtioSndClosePlayback(_Inout_ PVIRTIO_SND_PLAYBACK Playback);

    NTSTATUS
    VirtioSndInstallRenderSubdevice(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp, _In_ PRESOURCELIST ResourceList);

    NTSTATUS
    VirtioSndInstallTopologySubdevice(
        _In_ PDEVICE_OBJECT DeviceObject,
        _In_ PIRP Irp,
        _In_ PRESOURCELIST ResourceList,
        _Out_ PUNKNOWN *TopologyUnknown);

    LONG VirtioSndGetMasterVolume(VOID);

    NTSTATUS
    VirtioSndSetMasterVolume(_In_ LONG VolumeLevel);

    BOOLEAN
    VirtioSndGetMasterMute(VOID);

    VOID
    VirtioSndSetMasterMute(_In_ BOOLEAN Muted);

    VOID
    VirtioSndApplyMasterControls(
        _Inout_updates_bytes_(Length) PVOID Audio,
        _In_ ULONG Length,
        _In_ ULONG BitsPerSample);

    VOID
    VirtioSndStopTransport(_In_opt_ PDEVICE_OBJECT DeviceObject);

    BOOLEAN
    VirtioSndTransportStopping(VOID);

    BOOLEAN
    VirtioSndArmPlaybackCompletion(_Inout_ PVIRTIO_SND_PLAYBACK Playback);

    BOOLEAN
    VirtioSndPlaybackIsActive(_In_ PVIRTIO_SND_PLAYBACK Playback);

    VOID VirtioSndPlaybackWorkerQuiesced(VOID);

#ifdef __cplusplus
}
#endif

#endif
