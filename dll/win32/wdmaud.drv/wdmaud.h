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
#include <ks.h>
#include <ksmedia.h>
#include <interface.h>
#include <devioctl.h>
#include <setupapi.h>

/* 
 * Enables user-mode stream resampling support.
 * Resampling is required by some legacy devices
 * those don't support modern audio formats.
 * They're only able to use standard 44100 KHz sample rate,
 * 2 (stereo) channels and 16 BPS quality.
 * For example, Intel AC97 driver,
 * which is used in VirtualBox by default.
 */
//#define RESAMPLING_ENABLED

#define USE_MMIXER_LIB
#ifndef USE_MMIXER_LIB
#define FUNC_NAME(x) x##ByLegacy
#else
#define FUNC_NAME(x) x##ByMMixer
#endif

/* This lives in WAVEHDR.reserved */
typedef struct
{
    PWDMAUD_DEVICE_INFO DeviceInfo;
    LPOVERLAPPED Overlapped;
} WAVEHDR_EXTENSION, *PWAVEHDR_EXTENSION;

/* mmixer.c */

BOOL
WdmAudInitUserModeMixer(VOID);

ULONG
WdmAudGetWaveOutCount(VOID);

ULONG
WdmAudGetWaveInCount(VOID);

ULONG
WdmAudGetMixerCount(VOID);

/* resample.c */

MMRESULT
WdmAudResampleStream(
    _In_  PWDMAUD_DEVICE_INFO DeviceInfo,
    _In_ _Out_ PWAVEHDR WaveHeader);

/* legacy.c */

MMRESULT
WdmAudIoControl(
    _In_  PWDMAUD_DEVICE_INFO DeviceInfo,
    _In_opt_ ULONG DataBufferSize,
    _In_opt_ PVOID Buffer,
    _In_  DWORD IoControlCode);

MMRESULT
WdmAudCreateCompletionThread(
    _In_  PWDMAUD_DEVICE_INFO DeviceInfo);

MMRESULT
WdmAudDestroyCompletionThread(
    _In_ PWDMAUD_DEVICE_INFO DeviceInfo);

MMRESULT
WdmAudAddRemoveDeviceNode(
    _In_ SOUND_DEVICE_TYPE DeviceType,
    _In_ BOOL bAdd);

/* Shared functions for legacy.c and mmixer.c */

MMRESULT
FUNC_NAME(WdmAudOpenKernelSoundDevice)(VOID);

MMRESULT
FUNC_NAME(WdmAudCleanup)(VOID);

MMRESULT
FUNC_NAME(WdmAudGetCapabilities)(
    _In_  PWDMAUD_DEVICE_INFO DeviceInfo,
    _Out_ PVOID Capabilities,
    _In_  DWORD CapabilitiesSize);

MMRESULT
FUNC_NAME(WdmAudOpenSoundDevice)(
    _In_  PWDMAUD_DEVICE_INFO DeviceInfo,
    _In_  PWAVEFORMATEX WaveFormat);

MMRESULT
FUNC_NAME(WdmAudCloseSoundDevice)(
    _In_  PWDMAUD_DEVICE_INFO DeviceInfo,
    _In_  PVOID Handle);

MMRESULT
FUNC_NAME(WdmAudQueryMixerInfo)(
    _In_ PWDMAUD_DEVICE_INFO DeviceInfo,
    _In_ DWORD DeviceId,
    _In_ UINT uMsg,
    _In_ LPVOID Parameter,
    _In_ DWORD Flags);

MMRESULT
FUNC_NAME(WdmAudSetWaveState)(
    _In_ PWDMAUD_DEVICE_INFO DeviceInfo,
    _In_ BOOL bStart);

MMRESULT
FUNC_NAME(WdmAudSubmitWaveHeader)(
    _In_  PWDMAUD_DEVICE_INFO DeviceInfo,
    _In_  PWAVEHDR WaveHeader);

MMRESULT
FUNC_NAME(WdmAudResetStream)(
    _In_  PWDMAUD_DEVICE_INFO DeviceInfo,
    _In_  BOOL bStartReset);

MMRESULT
FUNC_NAME(WdmAudGetWavePosition)(
    _In_  PWDMAUD_DEVICE_INFO DeviceInfo,
    _In_  MMTIME* Time);

MMRESULT
FUNC_NAME(WdmAudGetNumWdmDevs)(
    _In_  SOUND_DEVICE_TYPE DeviceType,
    _Out_ DWORD* DeviceCount);

/*
    reentrancy.c
*/

MMRESULT
InitEntrypointMutexes(VOID);

VOID
CleanupEntrypointMutexes(VOID);

VOID
AcquireEntrypointMutex(
    _In_  SOUND_DEVICE_TYPE DeviceType);

VOID
ReleaseEntrypointMutex(
    _In_  SOUND_DEVICE_TYPE DeviceType);

/*
    mmewrap.c
*/

VOID
NotifyMmeClient(
    _In_  PWDMAUD_DEVICE_INFO DeviceInfo,
    _In_  UINT Message,
    _In_  DWORD_PTR Parameter);

DWORD
MmeGetNumDevs(
    _In_ SOUND_DEVICE_TYPE DeviceType);

MMRESULT
MmeGetSoundDeviceCapabilities(
    _In_  SOUND_DEVICE_TYPE DeviceType,
    _In_  DWORD DeviceId,
    _In_  PVOID Capabilities,
    _In_  DWORD CapabilitiesSize);

MMRESULT
MmeOpenDevice(
    _In_  SOUND_DEVICE_TYPE DeviceType,
    _In_  UINT DeviceId,
    _In_  LPWAVEOPENDESC OpenParameters,
    _In_  DWORD Flags,
    _Out_ DWORD_PTR* PrivateHandle);

MMRESULT
MmeCloseDevice(
    _In_  DWORD_PTR PrivateHandle);

MMRESULT
MmeGetPosition(
    _In_  DWORD_PTR PrivateHandle,
    _In_  MMTIME* Time,
    _In_  DWORD Size);

MMRESULT
MmeSetState(
    _In_  DWORD_PTR PrivateHandle,
    _In_  BOOL bStart);


#define MmePrepareWaveHeader(private_handle, header) \
    PrepareWaveHeader((PWDMAUD_DEVICE_INFO)private_handle, (PWAVEHDR)header)

#define MmeUnprepareWaveHeader(private_handle, header) \
    UnprepareWaveHeader((PWDMAUD_DEVICE_INFO)private_handle, (PWAVEHDR)header)

#define MmeWriteWaveHeader(private_handle, header) \
    WriteWaveHeader((PWDMAUD_DEVICE_INFO)private_handle, (PWAVEHDR)header)

MMRESULT
MmeResetWavePlayback(
    _In_  DWORD_PTR PrivateHandle);

/*
    result.c
*/

MMRESULT
Win32ErrorToMmResult(
    _In_  UINT ErrorCode);

MMRESULT
TranslateInternalMmResult(
    _In_  MMRESULT Result);

/*
    header.c
*/

MMRESULT
EnqueueWaveHeader(
    _In_  PWDMAUD_DEVICE_INFO DeviceInfo,
    _In_  PWAVEHDR Header);

VOID
CompleteWaveHeader(
    _In_  PWDMAUD_DEVICE_INFO DeviceInfo);

MMRESULT
PrepareWaveHeader(
    _In_  PWDMAUD_DEVICE_INFO DeviceInfo,
    _In_  PWAVEHDR Header);

MMRESULT
UnprepareWaveHeader(
    _In_  PWDMAUD_DEVICE_INFO DeviceInfo,
    _In_  PWAVEHDR Header);

MMRESULT
WriteWaveHeader(
    _In_  PWDMAUD_DEVICE_INFO DeviceInfo,
    _In_  PWAVEHDR Header);

/*
    streaming.c
*/

MMRESULT
DoWaveStreaming(
    _In_ PWDMAUD_DEVICE_INFO DeviceInfo,
    _In_ PWAVEHDR Header);

MMRESULT
StopStreaming(
    _In_ PWDMAUD_DEVICE_INFO DeviceInfo);


/*
    Convert a device type into a zero-based array index
 */

#define SOUND_DEVICE_TYPE_TO_INDEX(x) \
    ( x - MIN_SOUND_DEVICE_TYPE )

#define INDEX_TO_SOUND_DEVICE_TYPE(x) \
    ( x + MIN_SOUND_DEVICE_TYPE )

/*
    Validation
*/

#define VALIDATE_MMSYS_PARAMETER(parameter_condition) \
    { \
        if ( ! (parameter_condition) ) \
        { \
            DPRINT1("FAILED parameter check: %hS at File %S Line %lu\n", #parameter_condition, __FILE__, __LINE__); \
            return MMSYSERR_INVALPARAM; \
        } \
    }

#define MMSUCCESS(result) \
    ( result == MMSYSERR_NOERROR )


#endif /* __WDMAUD_H__ */
