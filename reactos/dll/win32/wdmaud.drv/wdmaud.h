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
WdmAudGetMixerCount();

MMRESULT
WdmAudGetMixerCapabilties(
    IN ULONG DeviceId, 
    LPMIXERCAPSW Capabilities);

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
