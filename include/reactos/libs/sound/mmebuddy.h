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
    Thread helper operations
*/

typedef MMRESULT (*SOUND_THREAD_OPERATION)(
    IN  struct _SOUND_DEVICE_INSTANCE* Instance,
    IN  PVOID Data);

typedef struct _THREAD_OPERATIONS
{
    struct _THREAD_OPERATIONS* Next;
    DWORD Id;
    SOUND_THREAD_OPERATION Operation;
} THREAD_OPERATIONS, *PTHREAD_OPERATIONS;

typedef struct _SOUND_THREAD
{
    HANDLE Handle;
    BOOLEAN Running;
    PTHREAD_OPERATIONS FirstOperation;
    HANDLE KillEvent;
    HANDLE RequestEvent;
    HANDLE RequestCompletionEvent;
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

typedef struct _MMFUNCTION_TABLE
{
    MMOPEN_FUNC     Open;
    MMCLOSE_FUNC    Close;
    MMGETCAPS_FUNC  GetCapabilities;
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
CreateSoundDevice(
    UCHAR DeviceType,
    PWSTR DevicePath);

BOOLEAN
DestroySoundDevice(
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


/*
    thread.c
*/

MMRESULT
StartSoundThread(
    IN  PSOUND_DEVICE_INSTANCE Instance);

MMRESULT
StopSoundThread(
    IN  PSOUND_DEVICE_INSTANCE Instance);


#endif
