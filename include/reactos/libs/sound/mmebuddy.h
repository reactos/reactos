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

#include <stdio.h>
#define SOUND_TRACE printf

#define SOUND_DEBUG(x) \
    MessageBox(0, x, L"Debug", MB_OK | MB_TASKMODAL);

#define SOUND_DEBUG_HEX(x) \
    { \
        WCHAR dbgmsg[1024], dbgtitle[1024]; \
        wsprintf(dbgtitle, L"%hS[%d]", __FILE__, __LINE__); \
        wsprintf(dbgmsg, L"%hS == %x", #x, x); \
        MessageBox(0, dbgmsg, dbgtitle, MB_OK | MB_TASKMODAL); \
    }

#define SOUND_ASSERT(x) \
    { \
        if ( ! ( x ) ) \
        { \
            WCHAR dbgmsg[1024], dbgtitle[1024]; \
            wsprintf(dbgtitle, L"%hS[%d]", __FILE__, __LINE__); \
            wsprintf(dbgmsg, L"ASSERT FAILED:\n%hS", #x); \
            MessageBox(0, dbgmsg, dbgtitle, MB_OK | MB_TASKMODAL); \
            exit(1); \
        } \
    }


/*
    Some memory allocation helper macros
*/

/*
#define AllocateMemory(size) \
    HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size)

#define FreeMemory(ptr) \
    HeapFree(GetProcessHeap(), 0, ptr)
*/

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


#define MinimumOf(value_a, value_b) \
    ( value_a < value_b ? value_a : value_b )

#define MaximumOf(value_a, value_b) \
    ( value_a > value_b ? value_a : value_b )


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

typedef struct _MMFUNCTION_TABLE
{
    MMCREATEINSTANCE_FUNC   Constructor;
    MMDESTROYINSTANCE_FUNC  Destructor;
    MMGETCAPS_FUNC          GetCapabilities;

    MMWAVEQUERYFORMAT_FUNC  QueryWaveFormat;
    MMWAVESETFORMAT_FUNC    SetWaveFormat;
    MMWAVEQUEUEBUFFER_FUNC  QueueWaveBuffer;
} MMFUNCTION_TABLE, *PMMFUNCTION_TABLE;


/*
    Represents an audio device
*/

typedef struct _SOUND_DEVICE
{
    struct _SOUND_DEVICE* Next;
    struct _SOUND_DEVICE_INSTANCE* FirstInstance;
    UCHAR DeviceType;
    LPWSTR DevicePath;
    HANDLE Handle;
    MMFUNCTION_TABLE Functions;
} SOUND_DEVICE, *PSOUND_DEVICE;


/*
    Represents an individual instance of an audio device
*/

typedef struct _WAVE_STREAM_INFO
{
    /* Buffer queue head and tail */
    PWAVEHDR BufferQueueHead;
    PWAVEHDR BufferQueueTail;
    /* The buffer currently being processed */
    PWAVEHDR CurrentBuffer;
    /* How far into the current buffer we've gone */
    //DWORD BufferOffset;
    /* How many I/O operations have been submitted */
    DWORD BuffersOutstanding;
} WAVE_STREAM_INFO, *PWAVE_STREAM_INFO;

typedef struct _SOUND_DEVICE_INSTANCE
{
    struct _SOUND_DEVICE_INSTANCE* Next;
    PSOUND_DEVICE Device;
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

MMRESULT
GetSoundDeviceType(
    IN  PSOUND_DEVICE Device,
    OUT PUCHAR DeviceType);


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
    IN  PSOUND_DEVICE SoundDevice,
    IN  DWORD AccessRights);

MMRESULT
CloseKernelSoundDevice(
    PSOUND_DEVICE SoundDevice);

MMRESULT
PerformSoundDeviceIo(
    IN  PSOUND_DEVICE SoundDevice,
    IN  DWORD IoControlCode,
    IN  LPVOID InBuffer,
    IN  DWORD InBufferSize,
    OUT LPVOID OutBuffer,
    IN  DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    IN  LPOVERLAPPED Overlapped);

MMRESULT
ReadSoundDevice(
    IN  PSOUND_DEVICE SoundDevice,
    IN  DWORD IoControlCode,
    OUT LPVOID OutBuffer,
    IN  DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    IN  LPOVERLAPPED Overlapped);

MMRESULT
WriteSoundDevice(
    IN  PSOUND_DEVICE SoundDevice,
    IN  DWORD IoControlCode,
    IN  LPVOID InBuffer,
    IN  DWORD InBufferSize,
    OUT LPDWORD BytesReturned,
    IN  LPOVERLAPPED Overlapped);

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
AllocateMemory(
    IN  DWORD Size);

VOID
FreeMemory(
    IN  PVOID Pointer);

DWORD
GetMemoryAllocations();

ULONG
GetDigitCount(
    IN  ULONG Number);

MMRESULT
Win32ErrorToMmResult(IN UINT error_code);


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


/*
    ...
*/

MMRESULT
GetSoundDeviceCapabilities(
    IN  PSOUND_DEVICE Device,
    OUT PUNIVERSAL_CAPS Capabilities);

MMRESULT
DefaultGetSoundDeviceCapabilities(
    IN  PSOUND_DEVICE Device,
    OUT PUNIVERSAL_CAPS Capabilities);

MMRESULT
QueryWaveDeviceFormatSupport(
    IN  PSOUND_DEVICE Device,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize);

MMRESULT
DefaultQueryWaveDeviceFormatSupport(
    IN  PSOUND_DEVICE Device,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize);

MMRESULT
SetWaveDeviceFormat(
    IN  PSOUND_DEVICE_INSTANCE Instance,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize);

MMRESULT
DefaultSetWaveDeviceFormat(
    IN  PSOUND_DEVICE_INSTANCE Instance,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize);

MMRESULT
DefaultInstanceConstructor(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance);

VOID
DefaultInstanceDestructor(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance);


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



MMRESULT
QueueWaveDeviceBuffer(
    IN  PSOUND_DEVICE_INSTANCE Instance,
    IN  PWAVEHDR BufferHeader);



/*
    mme/wodMessage.c
*/

APIENTRY DWORD
wodMessage(
    DWORD device_id,
    DWORD message,
    DWORD private_handle,
    DWORD parameter1,
    DWORD parameter2);


#endif
