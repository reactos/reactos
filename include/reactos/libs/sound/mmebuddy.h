/*
    ReactOS Sound System
    MME Support Helper

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        4 July 2008 - Created

    Notes:
        MME Buddy was the best name I could come up with...

        The structures etc. here should be treated as internal to the
        library so should not be directly accessed elsewhere.
*/

#ifndef ROS_AUDIO_MMEBUDDY_H
#define ROS_AUDIO_MMEBUDDY_H

/*
    Hacky debug macro
*/

#define POPUP(msg) \
    MessageBoxA(0, msg, __FUNCTION__, MB_OK | MB_TASKMODAL)


#ifndef NDEBUG

#ifdef ASSERT
    #undef ASSERT
#endif

/* HACK for testing */
#include <stdio.h>

#define TRACE_(...) \
    { \
        printf("  %s: ", __FUNCTION__); \
        printf(__VA_ARGS__); \
    }

#define WARN_(...) \
    { \
        printf("o %s: ", __FUNCTION__); \
        printf(__VA_ARGS__); \
    }

#define ERR_(...) \
    { \
        printf("X %s: ", __FUNCTION__); \
        printf(__VA_ARGS__); \
    }

#define ASSERT(x) \
    { \
        if ( ! (x) ) \
        { \
            ERR_("ASSERT failed! %s\n", #x); \
            exit(1); \
        } \
    }

#define TRACE_ENTRY() \
    TRACE_("entered function\n")

#define TRACE_EXIT(retval) \
    TRACE_("returning %d (0x%x)\n", (int)retval, (int)retval)

#endif


/*
    Some memory allocation helper macros
*/

#define AllocateMemory(amount) \
    AllocateTaggedMemory(0x00000000, amount)

#define FreeMemory(ptr) \
    FreeTaggedMemory(0x00000000, ptr)

#define AllocateTaggedMemoryFor(tag, thing) \
    (thing*) AllocateTaggedMemory(tag, sizeof(thing))

#define AllocateMemoryFor(thing) \
    (thing*) AllocateMemory(sizeof(thing))

#define StringLengthToBytes(chartype, string_length) \
    ( ( string_length + 1 ) * sizeof(chartype) )

#define AllocateWideString(string_length) \
    (PWSTR) AllocateMemory(StringLengthToBytes(WCHAR, string_length))

#define ZeroWideString(string) \
    ZeroMemory(string, StringLengthToBytes(WCHAR, wcslen(string)))

#define CopyWideString(dest, source) \
    CopyMemory(dest, source, StringLengthToBytes(WCHAR, wcslen(source)))


/*
    Helps find the minimum/maximum of two values
*/

#define MinimumOf(value_a, value_b) \
    ( value_a < value_b ? value_a : value_b )

#define MaximumOf(value_a, value_b) \
    ( value_a > value_b ? value_a : value_b )


/*
    Validation
*/

#define VALIDATE_MMSYS_PARAMETER(parameter_condition) \
    { \
        if ( ! (parameter_condition) ) \
        { \
            ERR_("Parameter check: %s\n", #parameter_condition); \
            TRACE_EXIT(MMSYSERR_INVALPARAM); \
            return MMSYSERR_INVALPARAM; \
        } \
    }


struct _SOUND_DEVICE;
struct _SOUND_DEVICE_INSTANCE;


/*
    Rather than pass caps structures around as a PVOID, this can be
    used instead.
*/

typedef union _UNIVERSAL_CAPS
{
    WAVEOUTCAPS WaveOut;
    WAVEINCAPS WaveIn;
    MIDIOUTCAPS MidiOut;
    MIDIINCAPS MidiIn;
} UNIVERSAL_CAPS, *PUNIVERSAL_CAPS;



/* New sound thread code */

typedef MMRESULT (*SOUND_THREAD_REQUEST_HANDLER)(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN  OPTIONAL PVOID Parameter);

typedef struct _SOUND_THREAD_REQUEST
{
    /* The sound device instance this request relates to */
    struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance;
    /* What function to call */
    SOUND_THREAD_REQUEST_HANDLER RequestHandler;
    /* Caller-defined parameter */
    PVOID Parameter;
    /* This will contain the return code of the request function */
    MMRESULT ReturnValue;
} SOUND_THREAD_REQUEST, *PSOUND_THREAD_REQUEST;

typedef VOID (*SOUND_THREAD_IO_COMPLETION_HANDLER)(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN  PVOID Parameter OPTIONAL,
    IN  DWORD BytesWritten);

typedef struct _SOUND_THREAD_COMPLETED_IO
{
    struct _SOUND_THREAD_COMPLETED_IO* Previous;
    struct _SOUND_THREAD_COMPLETED_IO* Next;

    struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance;
    SOUND_THREAD_IO_COMPLETION_HANDLER CompletionHandler;
    PVOID Parameter;
    DWORD BytesTransferred;
} SOUND_THREAD_COMPLETED_IO, *PSOUND_THREAD_COMPLETED_IO;

typedef struct _SOUND_THREAD_OVERLAPPED
{
    OVERLAPPED General;

    /* Pointer to structure to fill with completion data */
    PSOUND_THREAD_COMPLETED_IO CompletionData;
} SOUND_THREAD_OVERLAPPED, *PSOUND_THREAD_OVERLAPPED;

/*
    Audio device function table
*/

typedef MMRESULT (*MMCREATEINSTANCE_FUNC)(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance);

typedef VOID (*MMDESTROYINSTANCE_FUNC)(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance);

typedef MMRESULT (*MMGETCAPS_FUNC)(
    IN  struct _SOUND_DEVICE* Device,
    OUT PUNIVERSAL_CAPS Capabilities);

typedef MMRESULT (*MMWAVEQUERYFORMAT_FUNC)(
    IN  struct _SOUND_DEVICE* Device,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize);

typedef MMRESULT (*MMWAVESETFORMAT_FUNC)(
    IN  struct _SOUND_DEVICE_INSTANCE* Instance,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize);

typedef MMRESULT (*MMWAVEQUEUEBUFFER_FUNC)(
    IN  struct _SOUND_DEVICE_INSTANCE* Instance,
    IN  PWAVEHDR WaveHeader);

typedef MMRESULT (*MMGETWAVESTATE_FUNC)(
    IN  struct _SOUND_DEVICE_INSTANCE* Instance,
    OUT PULONG State);

typedef MMRESULT (*MMSETWAVESTATE_FUNC)(
    IN  struct _SOUND_DEVICE_INSTANCE* Instance);

typedef struct _MMFUNCTION_TABLE
{
    MMCREATEINSTANCE_FUNC   Constructor;
    MMDESTROYINSTANCE_FUNC  Destructor;
    MMGETCAPS_FUNC          GetCapabilities;

    MMWAVEQUERYFORMAT_FUNC  QueryWaveFormat;
    MMWAVESETFORMAT_FUNC    SetWaveFormat;
    MMWAVEQUEUEBUFFER_FUNC  QueueWaveBuffer;

    MMGETWAVESTATE_FUNC     GetWaveDeviceState;
    MMSETWAVESTATE_FUNC     PauseWaveDevice;
    MMSETWAVESTATE_FUNC     RestartWaveDevice;
    MMSETWAVESTATE_FUNC     ResetWaveDevice;
} MMFUNCTION_TABLE, *PMMFUNCTION_TABLE;


/*
    Represents an audio device
*/

#define SOUND_DEVICE_TAG "SndD"

typedef struct _SOUND_DEVICE
{
    struct _SOUND_DEVICE* Next;
    struct _SOUND_DEVICE_INSTANCE* FirstInstance;
    UCHAR DeviceType;
    LPWSTR DevicePath;
    MMFUNCTION_TABLE Functions;
} SOUND_DEVICE, *PSOUND_DEVICE;


/*
    Represents an individual instance of an audio device
*/

#define WAVE_STREAM_INFO_TAG "WavS"

typedef struct _WAVE_STREAM_INFO
{
    /* Buffer queue head and tail */
    PWAVEHDR BufferQueueHead;
    PWAVEHDR BufferQueueTail;
    /* The buffer currently being processed */
    PWAVEHDR CurrentBuffer;
    /* How far into the current buffer we've gone */
    DWORD BufferOffset;
    /* How many I/O operations have been submitted */
    DWORD BuffersOutstanding;
    /* Looping */
    PWAVEHDR LoopHead;
    DWORD LoopsRemaining;
} WAVE_STREAM_INFO, *PWAVE_STREAM_INFO;


#define SOUND_DEVICE_INSTANCE_TAG "SndI"

typedef struct _SOUND_DEVICE_INSTANCE
{
    struct _SOUND_DEVICE_INSTANCE* Next;
    PSOUND_DEVICE Device;

    /* The currently opened handle to the device */
    HANDLE Handle;
/*    PSOUND_THREAD Thread;*/

    /* Stuff generously donated to us from WinMM */
    struct
    {
        DWORD ClientCallback;
    } WinMM;

    /* Device-specific parameters */
    union
    {
        WAVE_STREAM_INFO Wave;
    } Streaming;
} SOUND_DEVICE_INSTANCE, *PSOUND_DEVICE_INSTANCE;


/*
    Thread requests
*/

#define THREADREQUEST_EXIT              0
#define WAVEREQUEST_QUEUE_BUFFER        1


/*
    entry.c
*/

LONG
DefaultDriverProc(
    DWORD driver_id,
    HANDLE driver_handle,
    UINT message,
    LONG parameter1,
    LONG parameter2);


/*
    devices.c
*/

ULONG
GetSoundDeviceCount(
    IN  UCHAR DeviceType);

MMRESULT
GetSoundDevice(
    IN  UCHAR DeviceType,
    IN  ULONG DeviceIndex,
    OUT PSOUND_DEVICE* Device);

MMRESULT
GetSoundDevicePath(
    IN  PSOUND_DEVICE SoundDevice,
    OUT LPWSTR* DevicePath);

BOOLEAN
AddSoundDevice(
    IN  UCHAR DeviceType,
    IN  PWSTR DevicePath,
    IN  PMMFUNCTION_TABLE FunctionTable);

MMRESULT
RemoveSoundDevice(
    IN  PSOUND_DEVICE SoundDevice);

MMRESULT
RemoveSoundDevices(
    IN  UCHAR DeviceType);

VOID
RemoveAllSoundDevices();

BOOLEAN
IsValidSoundDevice(
    IN  PSOUND_DEVICE SoundDevice);

MMRESULT
GetSoundDeviceType(
    IN  PSOUND_DEVICE Device,
    OUT PUCHAR DeviceType);

MMRESULT
GetSoundDeviceFunctionTable(
    IN  PSOUND_DEVICE SoundDevice,
    OUT PMMFUNCTION_TABLE* FunctionTable);

MMRESULT
DefaultInstanceConstructor(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance);

VOID
DefaultInstanceDestructor(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance);


/*
    nt4.c
*/

typedef BOOLEAN (*SOUND_DEVICE_DETECTED_PROC)(
    UCHAR DeviceType,
    PWSTR DevicePath,
    HANDLE Handle);

MMRESULT
OpenSoundDriverParametersRegKey(
    IN  LPWSTR ServiceName,
    OUT PHKEY KeyHandle);

MMRESULT
OpenSoundDeviceRegKey(
    IN  LPWSTR ServiceName,
    IN  DWORD DeviceIndex,
    OUT PHKEY KeyHandle);

MMRESULT
EnumerateNt4ServiceSoundDevices(
    IN  LPWSTR ServiceName,
    IN  UCHAR DeviceType,
    IN  SOUND_DEVICE_DETECTED_PROC SoundDeviceDetectedProc);

MMRESULT
DetectNt4SoundDevices(
    IN  UCHAR DeviceType,
    IN  PWSTR BaseDevicePath,
    IN  SOUND_DEVICE_DETECTED_PROC SoundDeviceDetectedProc);


/*
    kernel.c
*/

MMRESULT
OpenKernelSoundDeviceByName(
    IN  PWSTR DeviceName,
    IN  DWORD AccessRights,
    IN  PHANDLE Handle);

MMRESULT
OpenKernelSoundDevice(
    PSOUND_DEVICE SoundDevice,
    DWORD AccessRights,
    PHANDLE Handle);

MMRESULT
CloseKernelSoundDevice(
    IN  HANDLE Handle);

MMRESULT
PerformDeviceIo(
    IN  HANDLE Handle,
    IN  DWORD IoControlCode,
    IN  LPVOID InBuffer,
    IN  DWORD InBufferSize,
    OUT LPVOID OutBuffer,
    IN  DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    IN  LPOVERLAPPED Overlapped);

MMRESULT
RetrieveFromDeviceHandle(
    IN  HANDLE Handle,
    IN  DWORD IoControlCode,
    OUT LPVOID OutBuffer,
    IN  DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    IN  LPOVERLAPPED Overlapped);

MMRESULT
SendToDeviceHandle(
    IN  HANDLE Handle,
    IN  DWORD IoControlCode,
    IN  LPVOID InBuffer,
    IN  DWORD InBufferSize,
    OUT LPDWORD BytesReturned,
    IN  LPOVERLAPPED Overlapped);

MMRESULT
PerformSoundDeviceIo(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  DWORD IoControlCode,
    IN  LPVOID InBuffer,
    IN  DWORD InBufferSize,
    OUT LPVOID OutBuffer,
    IN  DWORD OutBufferSize,
    OUT LPDWORD BytesReturned);

MMRESULT
RetrieveFromSoundDevice(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  DWORD IoControlCode,
    OUT LPVOID OutBuffer,
    IN  DWORD OutBufferSize,
    OUT LPDWORD BytesReturned);

MMRESULT
SendToSoundDevice(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  DWORD IoControlCode,
    IN  LPVOID InBuffer,
    IN  DWORD InBufferSize,
    OUT LPDWORD BytesReturned);

MMRESULT
WriteSoundDeviceBuffer(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  LPVOID Buffer,
    IN  DWORD BufferSize,
    IN  LPOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine,
    LPOVERLAPPED Overlapped);


/*
    utility.c
*/

PVOID
AllocateTaggedMemory(
    IN  DWORD Tag,
    IN  DWORD Size);

VOID
FreeTaggedMemory(
    IN  DWORD Tag,
    IN  PVOID Pointer);

DWORD
GetMemoryAllocations();

ULONG
GetDigitCount(
    IN  ULONG Number);

MMRESULT
Win32ErrorToMmResult(IN UINT error_code);

MMRESULT
TranslateInternalMmResult(MMRESULT Result);


/*
    instances.c
*/

MMRESULT
CreateSoundDeviceInstance(
    IN  PSOUND_DEVICE SoundDevice,
    OUT PSOUND_DEVICE_INSTANCE* Instance);

MMRESULT
DestroySoundDeviceInstance(
    IN  PSOUND_DEVICE_INSTANCE Instance);

MMRESULT
DestroyAllInstancesOfSoundDevice(
    IN  PSOUND_DEVICE SoundDevice);

MMRESULT
GetSoundDeviceFromInstance(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    OUT PSOUND_DEVICE* SoundDevice);

BOOLEAN
IsValidSoundDeviceInstance(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance);

MMRESULT
GetSoundDeviceTypeFromInstance(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    OUT PUCHAR DeviceType);


/*
    capabilities.c
*/

MMRESULT
GetSoundDeviceCapabilities(
    IN  PSOUND_DEVICE SoundDevice,
    OUT PUNIVERSAL_CAPS Capabilities);

MMRESULT
DefaultGetSoundDeviceCapabilities(
    IN  PSOUND_DEVICE Device,
    OUT PUNIVERSAL_CAPS Capabilities);


/*
    wave/format.c
*/

MMRESULT
QueryWaveDeviceFormatSupport(
    IN  PSOUND_DEVICE SoundDevice,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize);

MMRESULT
DefaultQueryWaveDeviceFormatSupport(
    IN  PSOUND_DEVICE SoundDevice,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize);

MMRESULT
SetWaveDeviceFormat(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize);

MMRESULT
DefaultSetWaveDeviceFormat(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize);


/*
    thread.c
*/

MMRESULT
OverlappedSoundDeviceIo(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID Buffer,
    IN  DWORD BufferSize,
    IN  SOUND_THREAD_IO_COMPLETION_HANDLER IoCompletionHandler,
    IN  PVOID CompletionParameter OPTIONAL);

MMRESULT
StartSoundThread();

MMRESULT
StopSoundThread();

MMRESULT
CallUsingSoundThread(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  SOUND_THREAD_REQUEST_HANDLER RequestHandler,
    IN  PVOID Parameter);


/*
    wave/streamcontrol.c
*/

MMRESULT
InitWaveStreamData(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance);

MMRESULT
QueueWaveDeviceBuffer(
    IN  PSOUND_DEVICE_INSTANCE Instance,
    IN  PWAVEHDR BufferHeader);

MMRESULT
GetWaveDeviceState(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    OUT PULONG State);

MMRESULT
DefaultGetWaveDeviceState(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    OUT PULONG State);

MMRESULT
PauseWaveDevice(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance);

MMRESULT
DefaultPauseWaveDevice(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance);

MMRESULT
RestartWaveDevice(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance);

MMRESULT
DefaultRestartWaveDevice(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance);

MMRESULT
ResetWaveDevice(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance);

MMRESULT
DefaultResetWaveDevice(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance);


/*
    wave/wodMessage.c
*/

APIENTRY DWORD
wodMessage(
    DWORD device_id,
    DWORD message,
    DWORD private_handle,
    DWORD parameter1,
    DWORD parameter2);


#endif
