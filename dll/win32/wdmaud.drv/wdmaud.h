#ifndef WDMAUD_H__
#define WDMAUD_H__

#include <windows.h>
#include <ntddsnd.h>
#include <sndtypes.h>
#include <setupapi.h>
#include <mmddk.h>
#include <mmebuddy.h>

#include <ks.h>
#include <ksmedia.h>
#include "interface.h"
#include "mmixer.h"
#include <debug.h>

BOOL
WdmAudInitUserModeMixer();

ULONG
WdmAudGetWaveOutCount();

ULONG
WdmAudGetWaveInCount();

ULONG
WdmAudGetMixerCount();

MMRESULT
WdmAudGetMixerCapabilties(
    IN ULONG DeviceId, 
    LPMIXERCAPSW Capabilities);

MMRESULT
WdmAudGetWaveOutCapabilities(
    IN ULONG DeviceId, 
    LPWAVEOUTCAPSW Capabilities);

MMRESULT
WdmAudGetWaveInCapabilities(
    IN ULONG DeviceId, 
    LPWAVEINCAPSW Capabilities);

MMRESULT
WdmAudCloseMixer(
    IN HMIXER Handle,
    IN HANDLE hNotifyEvent);

MMRESULT
WdmAudOpenMixer(
    IN PHANDLE hMixer,
    IN ULONG DeviceId, 
    IN HANDLE hNotifyEvent);

MMRESULT
WdmAudOpenWave(
    OUT PHANDLE hPin,
    IN DWORD DeviceId,
    IN PWAVEFORMATEX WaveFormat,
    IN DWORD bWaveIn);


MMRESULT
WdmAudGetLineInfo(
    IN HANDLE hMixer,
    IN LPMIXERLINE MixLine,
    IN ULONG Flags);

MMRESULT
WdmAudGetLineControls(
    IN HANDLE hMixer,
    IN LPMIXERLINECONTROLSW MixControls,
    IN ULONG Flags);

MMRESULT
WdmAudSetControlDetails(
    IN HANDLE hMixer,
    IN LPMIXERCONTROLDETAILS MixDetails,
    IN ULONG Flags);

MMRESULT
WdmAudGetControlDetails(
    IN HANDLE hMixer,
    IN LPMIXERCONTROLDETAILS MixDetails,
    IN ULONG Flags);

#endif
