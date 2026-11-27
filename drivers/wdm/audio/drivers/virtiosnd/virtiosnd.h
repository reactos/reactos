/*
 * PROJECT:     ReactOS VirtIO Sound Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Driver definitions
 * COPYRIGHT:   Copyright 2026 Ralf Lang <ralf.lang@ralf-lang.de>
 */

/**
 * @file
 * @brief VirtIO Sound Driver Definitions
 *
 * This file contains the public interface definitions for the VirtIO Sound Driver.
 *
 * @copyright Copyright 2026 Ralf Lang <ralf.lang@ralf-lang.de>
 * @license GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 */

#ifndef _VIRTIOSND_H_
#define _VIRTIOSND_H_

#include <ntddk.h>
#include <portcls.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Starts the VirtIO sound device
     *
     * Initializes the VirtIO sound device and registers it with the PortCls driver.
     *
     * @param DeviceObject Pointer to the device object
     * @param Irp Pointer to the I/O request packet
     * @param ResourceList Pointer to the resource list
     * @return NTSTATUS Status of the operation
     */
    NTSTATUS
    NTAPI
    VirtioSndStartDevice(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp, _In_ PRESOURCELIST ResourceList);

    /**
     * @brief Starts the VirtIO transport layer
     *
     * Initializes the VirtIO transport layer for the sound device.
     *
     * @param DeviceObject Pointer to the device object
     * @param ResourceList Pointer to the resource list
     * @return NTSTATUS Status of the operation
     */
    NTSTATUS
    VirtioSndStartTransport(_In_ PDEVICE_OBJECT DeviceObject, _In_ PRESOURCELIST ResourceList);

    /**
     * @brief Playback context structure
     *
     * Opaque structure for the playback context of a VirtIO sound device.
     */
    typedef struct _VIRTIO_SND_PLAYBACK VIRTIO_SND_PLAYBACK, *PVIRTIO_SND_PLAYBACK;

    /**
     * @brief Opens a playback stream
     *
     * Opens a new playback stream with the specified parameters.
     *
     * @param Channels Number of audio channels (1 or 2)
     * @param BitsPerSample Number of bits per sample (16 or 32)
     * @param SampleRate Audio sample rate in Hz
     * @param WakeEvent Pointer to the wake event for playback completion
     * @param Playback Pointer to receive the playback context
     * @return NTSTATUS Status of the operation
     */
    NTSTATUS
    VirtioSndOpenPlayback(
        _In_ ULONG Channels,
        _In_ ULONG BitsPerSample,
        _In_ ULONG SampleRate,
        _In_ PKEVENT WakeEvent,
        _Out_ PVIRTIO_SND_PLAYBACK *Playback);

    /**
     * @brief Starts playback on a stream
     *
     * Starts playback on the specified playback stream.
     *
     * @param Playback Pointer to the playback context
     * @return NTSTATUS Status of the operation
     */
    NTSTATUS
    VirtioSndStartPlayback(_Inout_ PVIRTIO_SND_PLAYBACK Playback);

    /**
     * @brief Stops playback on a stream
     *
     * Stops playback on the specified playback stream.
     *
     * @param Playback Pointer to the playback context
     * @return NTSTATUS Status of the operation
     */
    NTSTATUS
    VirtioSndStopPlayback(_Inout_ PVIRTIO_SND_PLAYBACK Playback);

    /**
     * @brief Queues audio data for playback
     *
     * Queues audio data for playback on the specified stream.
     *
     * @param Playback Pointer to the playback context
     * @param Audio Pointer to the audio data to queue
     * @param Length Length of the audio data in bytes
     * @return NTSTATUS Status of the operation
     */
    NTSTATUS
    VirtioSndQueuePlayback(
        _Inout_ PVIRTIO_SND_PLAYBACK Playback,
        _In_reads_bytes_(Length) PVOID Audio,
        _In_ ULONG Length);

    /**
     * @brief Gets playback completion status
     *
     * Retrieves the completion status of the last playback operation.
     *
     * @param Playback Pointer to the playback context
     * @param DeviceStatus Pointer to receive the device status
     * @param LatencyBytes Pointer to receive the latency in bytes
     * @return NTSTATUS Status of the operation
     */
    NTSTATUS
    VirtioSndGetPlaybackCompletion(
        _Inout_ PVIRTIO_SND_PLAYBACK Playback,
        _Out_ PULONG DeviceStatus,
        _Out_ PULONG LatencyBytes);

    /**
     * @brief Processes playback events
     *
     * Processes any pending playback events such as XRUN notifications.
     *
     * @param Playback Pointer to the playback context
     * @param XrunCount Pointer to receive the XRUN count
     * @return NTSTATUS Status of the operation
     */
    NTSTATUS
    VirtioSndProcessPlaybackEvents(_Inout_ PVIRTIO_SND_PLAYBACK Playback, _Out_ PULONG XrunCount);

    /**
     * @brief Gets channel mapping for playback
     *
     * Retrieves the channel mapping for the specified number of channels.
     *
     * @param Channels Number of channels
     * @param Positions Pointer to buffer to receive channel positions
     * @return NTSTATUS Status of the operation
     */
    NTSTATUS
    VirtioSndGetPlaybackChannelMap(_In_ ULONG Channels, _Out_writes_bytes_(Channels) PUCHAR Positions);

    /**
     * @brief Gets supported playback rates
     *
     * Retrieves the list of supported playback rates for the specified parameters.
     *
     * @param Channels Number of channels
     * @param BitsPerSample Number of bits per sample
     * @param Rates Pointer to receive the supported rates
     * @return NTSTATUS Status of the operation
     */
    NTSTATUS
    VirtioSndGetPlaybackRates(_In_ ULONG Channels, _In_ ULONG BitsPerSample, _Out_ ULONGLONG *Rates);

    /**
     * @brief Closes a playback stream
     *
     * Closes and cleans up the specified playback stream.
     *
     * @param Playback Pointer to the playback context
     */
    VOID
    VirtioSndClosePlayback(_Inout_ PVIRTIO_SND_PLAYBACK Playback);

    /**
     * @brief Installs the render subdevice
     *
     * Installs the render subdevice for the VirtIO sound device.
     *
     * @param DeviceObject Pointer to the device object
     * @param Irp Pointer to the I/O request packet
     * @param ResourceList Pointer to the resource list
     * @return NTSTATUS Status of the operation
     */
    NTSTATUS
    VirtioSndInstallRenderSubdevice(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp, _In_ PRESOURCELIST ResourceList);

    /**
     * @brief Installs the topology subdevice
     *
     * Installs the topology subdevice for the VirtIO sound device.
     *
     * @param DeviceObject Pointer to the device object
     * @param Irp Pointer to the I/O request packet
     * @param ResourceList Pointer to the resource list
     * @param TopologyUnknown Pointer to receive the topology unknown object
     * @return NTSTATUS Status of the operation
     */
    NTSTATUS
    VirtioSndInstallTopologySubdevice(
        _In_ PDEVICE_OBJECT DeviceObject,
        _In_ PIRP Irp,
        _In_ PRESOURCELIST ResourceList,
        _Out_ PUNKNOWN *TopologyUnknown);

    /**
     * @brief Gets the master volume level
     *
     * Retrieves the current master volume level in hundredths of dB.
     *
     * @return LONG Current master volume level
     */
    LONG VirtioSndGetMasterVolume(VOID);

    /**
     * @brief Sets the master volume level
     *
     * Sets the master volume level in hundredths of dB.
     *
     * @param VolumeLevel Volume level to set
     * @return NTSTATUS Status of the operation
     */
    NTSTATUS
    VirtioSndSetMasterVolume(_In_ LONG VolumeLevel);

    /**
     * @brief Gets the master mute state
     *
     * Retrieves the current master mute state.
     *
     * @return BOOLEAN TRUE if muted or FALSE otherwise
     */
    BOOLEAN
    VirtioSndGetMasterMute(VOID);

    /**
     * @brief Sets the master mute state
     *
     * Sets the master mute state.
     *
     * @param Muted TRUE to mute, FALSE to unmute
     */
    VOID
    VirtioSndSetMasterMute(_In_ BOOLEAN Muted);

    /**
     * @brief Applies master controls to audio data
     *
     * Applies the current master volume and mute settings to audio data.
     *
     * @param Audio Pointer to the audio data buffer
     * @param Length Length of the audio data in bytes
     * @param BitsPerSample Number of bits per sample
     */
    VOID
    VirtioSndApplyMasterControls(
        _Inout_updates_bytes_(Length) PVOID Audio,
        _In_ ULONG Length,
        _In_ ULONG BitsPerSample);

    /**
     * @brief Stops the VirtIO transport layer
     *
     * Stops the VirtIO transport layer for the sound device.
     *
     * @param DeviceObject Pointer to the device object (optional)
     */
    VOID
    VirtioSndStopTransport(_In_opt_ PDEVICE_OBJECT DeviceObject);

    /**
     * @brief Checks if transport is stopping
     *
     * Determines if the transport layer is in the process of stopping.
     *
     * @return BOOLEAN TRUE if transport is stopping or FALSE otherwise
     */
    BOOLEAN
    VirtioSndTransportStopping(VOID);

    /**
     * @brief Arms playback completion
     *
     * Arms the playback completion mechanism for the specified stream.
     *
     * @param Playback Pointer to the playback context
     * @return BOOLEAN TRUE if successful or FALSE otherwise
     */
    BOOLEAN
    VirtioSndArmPlaybackCompletion(_Inout_ PVIRTIO_SND_PLAYBACK Playback);

    /**
     * @brief Checks if playback is active
     *
     * Determines if playback is currently active for the specified stream.
     *
     * @param Playback Pointer to the playback context
     * @return BOOLEAN TRUE if playback is active or FALSE when off
     */
    BOOLEAN
    VirtioSndPlaybackIsActive(_In_ PVIRTIO_SND_PLAYBACK Playback);

    /**
     * @brief Indicates playback worker quiesced
     *
     * Signals that the playback worker has completed its work.
     */
    VOID VirtioSndPlaybackWorkerQuiesced(VOID);

#ifdef __cplusplus
}
#endif

#endif
