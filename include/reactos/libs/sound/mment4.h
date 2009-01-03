/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library (NT4 Helpers)
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        include/reactos/libs/sound/mment4.h
 *
 * PURPOSE:     Header for the NT4 part of the "MME Buddy" helper library
 *              (located in lib/drivers/sound/mment4)
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
 *
 *              31 Dec 2008 - Created
 *
 * NOTES:       This is intended for use in building NT4 compatible audio device
 *              drivers. Include mmebuddy.h first.
*/

#ifndef ROS_AUDIO_MMENT4_H
#define ROS_AUDIO_MMENT4_H

/*
    detect.c
*/

typedef BOOLEAN (*SOUND_DEVICE_DETECTED_PROC)(
    UCHAR DeviceType,
    PWSTR DevicePath);

MMRESULT
EnumerateNt4ServiceSoundDevices(
    IN  LPWSTR ServiceName,
    IN  MMDEVICE_TYPE DeviceType,
    IN  SOUND_DEVICE_DETECTED_PROC SoundDeviceDetectedProc);

MMRESULT
DetectNt4SoundDevices(
    IN  MMDEVICE_TYPE DeviceType,
    IN  PWSTR BaseDeviceName,
    IN  SOUND_DEVICE_DETECTED_PROC SoundDeviceDetectedProc);


/*
    registry.c
*/

MMRESULT
OpenSoundDriverParametersRegKey(
    IN  LPWSTR ServiceName,
    OUT PHKEY KeyHandle);

MMRESULT
OpenSoundDeviceRegKey(
    IN  LPWSTR ServiceName,
    IN  DWORD DeviceIndex,
    OUT PHKEY KeyHandle);


/*
    general.c - UNSORTED
*/

MMRESULT
GetNt4SoundDeviceCapabilities(
    IN  PSOUND_DEVICE SoundDevice,
    OUT PVOID Capabilities,
    IN  DWORD CapabilitiesSize);

MMRESULT
QueryNt4WaveDeviceFormatSupport(
    IN  PSOUND_DEVICE SoundDevice,
    IN  LPWAVEFORMATEX Format,
    IN  DWORD FormatSize);

MMRESULT
SetNt4WaveDeviceFormat(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  LPWAVEFORMATEX Format,
    IN  DWORD FormatSize);

MMRESULT
OpenNt4SoundDevice(
    IN  PSOUND_DEVICE SoundDevice,
    OUT PVOID* Handle);

MMRESULT
CloseNt4SoundDevice(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID Handle);

#endif
