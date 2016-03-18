#ifndef __WDMAUD_H__
#define __WDMAUD_H__

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <winreg.h>

#include <winuser.h>
#include <mmddk.h>
#include <mmebuddy.h>
#include <ksmedia.h>
#include <interface.h>
#include <devioctl.h>
#include <setupapi.h>

BOOL
WdmAudInitUserModeMixer(VOID);

ULONG
WdmAudGetWaveOutCount(VOID);

ULONG
WdmAudGetWaveInCount(VOID);

ULONG
WdmAudGetMixerCount(VOID);

MMRESULT
WdmAudGetNumWdmDevsByMMixer(
    IN  MMDEVICE_TYPE DeviceType,
    OUT DWORD* DeviceCount);

MMRESULT
WdmAudCommitWaveBufferByLegacy(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID OffsetPtr,
    IN  DWORD Length,
    IN  PSOUND_OVERLAPPED Overlap,
    IN  LPOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine);

MMRESULT
WriteFileEx_Remixer(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID OffsetPtr,
    IN  DWORD Length,
    IN  PSOUND_OVERLAPPED Overlap,
    IN  LPOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine);

MMRESULT
WdmAudGetCapabilitiesByMMixer(
    IN  PSOUND_DEVICE SoundDevice,
    IN  DWORD DeviceId,
    OUT PVOID Capabilities,
    IN  DWORD CapabilitiesSize);

MMRESULT
WdmAudOpenSoundDeviceByMMixer(
    IN  struct _SOUND_DEVICE* SoundDevice,
    OUT PVOID* Handle);

MMRESULT
WdmAudCloseSoundDeviceByMMixer(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN  PVOID Handle);

MMRESULT
WdmAudGetLineInfo(
    IN HANDLE hMixer,
    IN DWORD MixerId,
    IN LPMIXERLINEW MixLine,
    IN ULONG Flags);

MMRESULT
WdmAudGetLineControls(
    IN HANDLE hMixer,
    IN DWORD MixerId,
    IN LPMIXERLINECONTROLSW MixControls,
    IN ULONG Flags);

MMRESULT
WdmAudSetControlDetails(
    IN HANDLE hMixer,
    IN DWORD MixerId,
    IN LPMIXERCONTROLDETAILS MixDetails,
    IN ULONG Flags);

MMRESULT
WdmAudGetControlDetails(
    IN HANDLE hMixer,
    IN DWORD MixerId,
    IN LPMIXERCONTROLDETAILS MixDetails,
    IN ULONG Flags);

MMRESULT
WdmAudSetWaveDeviceFormatByMMixer(
    IN  PSOUND_DEVICE_INSTANCE Instance,
    IN  DWORD DeviceId,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize);

MMRESULT
WdmAudGetDeviceInterfaceStringByMMixer(
    IN  MMDEVICE_TYPE DeviceType,
    IN  DWORD DeviceId,
    IN  LPWSTR Interface,
    IN  DWORD  InterfaceLength,
    OUT  DWORD * InterfaceSize);

MMRESULT
WdmAudSetMixerDeviceFormatByMMixer(
    IN  PSOUND_DEVICE_INSTANCE Instance,
    IN  DWORD DeviceId,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize);

MMRESULT
WdmAudQueryMixerInfoByMMixer(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN DWORD DeviceId,
    IN UINT uMsg,
    IN LPVOID Parameter,
    IN DWORD Flags);

MMRESULT
WdmAudSetWaveStateByMMixer(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN BOOL bStart);

MMRESULT
WdmAudResetStreamByMMixer(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN  MMDEVICE_TYPE DeviceType,
    IN  BOOLEAN bStartReset);

MMRESULT
WdmAudGetWavePositionByMMixer(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN  MMTIME* Time);

MMRESULT
WdmAudCommitWaveBufferByMMixer(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID OffsetPtr,
    IN  DWORD Length,
    IN  PSOUND_OVERLAPPED Overlap,
    IN  LPOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine);

MMRESULT
WdmAudCleanupByMMixer(VOID);

/* legacy.c */

MMRESULT
WdmAudCleanupByLegacy(VOID);

MMRESULT
WdmAudGetCapabilitiesByLegacy(
    IN  PSOUND_DEVICE SoundDevice,
    IN  DWORD DeviceId,
    OUT PVOID Capabilities,
    IN  DWORD CapabilitiesSize);

MMRESULT
WdmAudOpenSoundDeviceByLegacy(
    IN PSOUND_DEVICE SoundDevice,
    OUT PVOID *Handle
);

MMRESULT
WdmAudCloseSoundDeviceByLegacy(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN  PVOID Handle);

MMRESULT
WdmAudGetDeviceInterfaceStringByLegacy(
    IN  MMDEVICE_TYPE DeviceType,
    IN  DWORD DeviceId,
    IN  LPWSTR Interface,
    IN  DWORD  InterfaceLength,
    OUT  DWORD * InterfaceSize);

MMRESULT
WdmAudSetMixerDeviceFormatByLegacy(
    IN  PSOUND_DEVICE_INSTANCE Instance,
    IN  DWORD DeviceId,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize);

MMRESULT
WdmAudQueryMixerInfoByLegacy(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN DWORD DeviceId,
    IN UINT uMsg,
    IN LPVOID Parameter,
    IN DWORD Flags);

MMRESULT
WdmAudSetWaveDeviceFormatByLegacy(
    IN  PSOUND_DEVICE_INSTANCE Instance,
    IN  DWORD DeviceId,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize);

MMRESULT
WdmAudSetWaveStateByLegacy(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN BOOL bStart);

MMRESULT
WdmAudResetStreamByLegacy(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN  MMDEVICE_TYPE DeviceType,
    IN  BOOLEAN bStartReset);

MMRESULT
WdmAudGetWavePositionByLegacy(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN  MMTIME* Time);

MMRESULT
WriteFileEx_Committer2(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID OffsetPtr,
    IN  DWORD Length,
    IN  PSOUND_OVERLAPPED Overlap,
    IN  LPOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine);

MMRESULT
WdmAudGetNumWdmDevsByLegacy(
    IN  MMDEVICE_TYPE DeviceType,
    OUT DWORD* DeviceCount);

DWORD
WINAPI
MixerEventThreadRoutine(
    LPVOID Parameter);

#endif /* __WDMAUD_H__ */
