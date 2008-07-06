/*
    ReactOS Sound System
    MME Support Helper

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        4 July 2008 - Created

    Notes:
        MME Buddy was the best name I could come up with...
*/

#ifndef ROS_AUDIO_MMEBUDDY_H
#define ROS_AUDIO_MMEBUDDY_H

/*
    Hacky debug macro
*/

#define SOUND_DEBUG(x) \
    MessageBox(0, x, L"Debug", MB_OK | MB_TASKMODAL);


/*
    Some memory allocation helper macros
*/

#define AllocateMemory(size) \
    HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size)

#define FreeMemory(ptr) \
    HeapFree(GetProcessHeap(), 0, ptr)

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


/*
    Used internally to shuttle data to/from the sound processing thread.
*/
typedef struct _THREAD_REQUEST
{
    struct _SOUND_DEVICE_INSTANCE* DeviceInstance;
    DWORD RequestId;
    PVOID Data;
    MMRESULT Result;
} THREAD_REQUEST, *PTHREAD_REQUEST;


/*
    Thread helper operations
*/
typedef MMRESULT (*SOUND_THREAD_REQUEST_HANDLER)(
    IN  struct _SOUND_DEVICE_INSTANCE* Instance,
    IN  DWORD RequestId,
    IN  PVOID Data);

typedef struct _SOUND_THREAD
{
    HANDLE Handle;
    BOOLEAN Running;
    SOUND_THREAD_REQUEST_HANDLER RequestHandler;
    HANDLE ReadyEvent;      /* Thread waiting for a request */
    HANDLE RequestEvent;    /* Caller sending a request */
    HANDLE DoneEvent;       /* Thread completed a request */
    THREAD_REQUEST Request;
} SOUND_THREAD, *PSOUND_THREAD;


/*
    Audio device function table
*/

typedef MMRESULT (*MMOPEN_FUNC)(
    IN  UCHAR DeviceType,
    IN  LPWSTR DevicePath,
    OUT PHANDLE Handle);

typedef MMRESULT (*MMCLOSE_FUNC)(
    IN  HANDLE Handle);

typedef MMRESULT (*MMGETCAPS_FUNC)(
    IN  struct _SOUND_DEVICE* SoundDevice,
    OUT PUNIVERSAL_CAPS Capabilities);

typedef MMRESULT (*MMWAVEFORMAT_FUNC)(
    IN  struct _SOUND_DEVICE* SoundDevice,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize);

typedef struct _MMFUNCTION_TABLE
{
    MMOPEN_FUNC             Open;
    MMCLOSE_FUNC            Close;
    MMGETCAPS_FUNC          GetCapabilities;

    MMWAVEFORMAT_FUNC       QueryWaveFormat;
    MMWAVEFORMAT_FUNC       SetWaveFormat;
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

typedef struct _SOUND_DEVICE_INSTANCE
{
    struct _SOUND_DEVICE_INSTANCE* Next;
    PSOUND_DEVICE Device;
    PSOUND_THREAD Thread;
} SOUND_DEVICE_INSTANCE, *PSOUND_DEVICE_INSTANCE;


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
    UCHAR DeviceType);

MMRESULT
GetSoundDevice(
    IN  UCHAR DeviceType,
    IN  ULONG DeviceIndex,
    OUT PSOUND_DEVICE* Device);

MMRESULT
GetSoundDevicePath(
    IN  PSOUND_DEVICE SoundDevice,
    OUT LPWSTR* DevicePath);

VOID
DestroyAllSoundDevices();

BOOLEAN
DestroySoundDevices(
    UCHAR DeviceType);

BOOLEAN
AddSoundDevice(
    UCHAR DeviceType,
    PWSTR DevicePath);

BOOLEAN
RemoveSoundDevice(
    UCHAR DeviceType,
    ULONG Index);


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
    LPWSTR ServiceName,
    UCHAR DeviceType,
    SOUND_DEVICE_DETECTED_PROC SoundDeviceDetectedProc);

MMRESULT
DetectNt4SoundDevices(
    UCHAR DeviceType,
    PWSTR BaseDevicePath,
    SOUND_DEVICE_DETECTED_PROC SoundDeviceDetectedProc);


/*
    kernel.c
*/

MMRESULT
OpenKernelSoundDeviceByName(
    PWSTR DeviceName,
    DWORD AccessRights,
    PHANDLE Handle);

MMRESULT
OpenKernelSoundDevice(
    PSOUND_DEVICE SoundDevice,
    DWORD AccessRights);

MMRESULT
PerformSoundDeviceIo(
    PSOUND_DEVICE SoundDevice,
    DWORD IoControlCode,
    LPVOID InBuffer,
    DWORD InBufferSize,
    LPVOID OutBuffer,
    DWORD OutBufferSize,
    LPDWORD BytesReturned,
    LPOVERLAPPED Overlapped);

MMRESULT
ReadSoundDevice(
    PSOUND_DEVICE SoundDevice,
    DWORD IoControlCode,
    LPVOID OutBuffer,
    DWORD OutBufferSize,
    LPDWORD BytesReturned,
    LPOVERLAPPED Overlapped);

MMRESULT
WriteSoundDevice(
    PSOUND_DEVICE SoundDevice,
    DWORD IoControlCode,
    LPVOID InBuffer,
    DWORD InBufferSize,
    LPDWORD BytesReturned,
    LPOVERLAPPED Overlapped);


/*
    utility.c
*/

ULONG
GetDigitCount(
    ULONG Number);

MMRESULT
Win32ErrorToMmResult(UINT error_code);


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
    IN  PSOUND_DEVICE Device,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize);

MMRESULT
DefaultSetWaveDeviceFormat(
    IN  PSOUND_DEVICE Device,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize);


/*
    thread.c
*/

MMRESULT
StartSoundThread(
    IN  PSOUND_DEVICE_INSTANCE Instance,
    IN  SOUND_THREAD_REQUEST_HANDLER RequestHandler);

MMRESULT
StopSoundThread(
    IN  PSOUND_DEVICE_INSTANCE Instance);

MMRESULT
CallSoundThread(
    IN  PSOUND_DEVICE_INSTANCE Instance,
    IN  DWORD RequestId,
    IN  PVOID RequestData);



MMRESULT
StartWaveThread(
    IN  PSOUND_DEVICE_INSTANCE Instance);

MMRESULT
StopWaveThread(
    IN  PSOUND_DEVICE_INSTANCE Instance);


#endif
