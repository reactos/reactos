/*
    ReactOS Sound System
    MME Support Helper

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        4 July 2008 - Created
        31 Dec 2008 - Split off NT4-specific code into a separate library

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

#define POPUP(...) \
    { \
        WCHAR dbg_popup_msg[1024], dbg_popup_title[256]; \
        wsprintf(dbg_popup_title, L"%hS(%d)", __FILE__, __LINE__); \
        wsprintf(dbg_popup_msg, __VA_ARGS__); \
        MessageBox(0, dbg_popup_msg, dbg_popup_title, MB_OK | MB_TASKMODAL); \
    }

#ifdef DEBUG_NT4
    #define SND_ERR(...) \
        { \
            WCHAR dbg_popup_msg[1024]; \
            wsprintf(dbg_popup_msg, __VA_ARGS__); \
            OutputDebugString(dbg_popup_msg); \
        }
    #define SND_WARN(...) \
        { \
            WCHAR dbg_popup_msg[1024]; \
            wsprintf(dbg_popup_msg, __VA_ARGS__); \
            OutputDebugString(dbg_popup_msg); \
        }
    #define SND_TRACE(...) \
        { \
            WCHAR dbg_popup_msg[1024]; \
            wsprintf(dbg_popup_msg, __VA_ARGS__); \
            OutputDebugString(dbg_popup_msg); \
        }

    #define SND_ASSERT(condition) \
        { \
            if ( ! ( condition ) ) \
            { \
                SND_ERR(L"ASSERT FAILED: %hS\n", #condition); \
                POPUP(L"ASSERT FAILED: %hS\n", #condition); \
                ExitProcess(1); \
            } \
        }

#else
    #define SND_ERR(...) while ( 0 ) do {}
    #define SND_WARN(...) while ( 0 ) do {}
    #define SND_TRACE(...) while ( 0 ) do {}
    #define SND_ASSERT(condition) while ( 0 ) do {}
#endif

/*
    Some memory allocation helper macros
*/

#define AllocateStruct(thing) \
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
    Convert a device type into a zero-based array index
*/

#define SOUND_DEVICE_TYPE_TO_INDEX(x) \
    ( x - MIN_SOUND_DEVICE_TYPE )

#define INDEX_TO_SOUND_DEVICE_TYPE(x) \
    ( x + MIN_SOUND_DEVICE_TYPE )


/*
    Validation
*/

#define IsValidSoundDeviceType IS_VALID_SOUND_DEVICE_TYPE

#define VALIDATE_MMSYS_PARAMETER(parameter_condition) \
    { \
        if ( ! (parameter_condition) ) \
        { \
            SND_ERR(L"FAILED parameter check: %hS\n", #parameter_condition); \
            return MMSYSERR_INVALPARAM; \
        } \
    }

#define MMSUCCESS(result) \
    ( result == MMSYSERR_NOERROR )


/*
    Types and Structures
*/

typedef UCHAR MMDEVICE_TYPE, *PMMDEVICE_TYPE;
struct _SOUND_DEVICE;
struct _SOUND_DEVICE_INSTANCE;


#define DEFINE_GETCAPS_FUNCTYPE(func_typename, caps_type) \
    typedef MMRESULT (*func_typename)( \
        IN  struct _SOUND_DEVICE* SoundDevice, \
        OUT caps_type Capabilities, \
        IN  DWORD CapabilitiesSize);

/* This one is for those of us who don't care */
DEFINE_GETCAPS_FUNCTYPE(MMGETCAPS_FUNC, PVOID);

/* These are for those of us that do */
DEFINE_GETCAPS_FUNCTYPE(MMGETWAVEOUTCAPS_FUNC, LPWAVEOUTCAPS);
DEFINE_GETCAPS_FUNCTYPE(MMGETWAVEINCAPS_FUNC,  LPWAVEINCAPS );
DEFINE_GETCAPS_FUNCTYPE(MMGETMIDIOUTCAPS_FUNC, LPMIDIOUTCAPS);
DEFINE_GETCAPS_FUNCTYPE(MMGETMIDIINCAPS_FUNC,  LPMIDIINCAPS );

struct _SOUND_DEVICE;
struct _SOUND_DEVICE_INSTANCE;

typedef MMRESULT (*MMWAVEQUERYFORMATSUPPORT_FUNC)(
    IN  struct _SOUND_DEVICE* Device,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize);

typedef MMRESULT (*MMWAVESETFORMAT_FUNC)(
    IN  struct _SOUND_DEVICE_INSTANCE* Instance,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize);

typedef MMRESULT (*MMOPEN_FUNC)(
    IN  struct _SOUND_DEVICE* SoundDevice,
    OUT PVOID* Handle);

typedef MMRESULT (*MMCLOSE_FUNC)(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN  PVOID Handle);  /* not sure about this */

typedef MMRESULT (*MMWAVEHEADER_FUNC)(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN  PWAVEHDR WaveHeader);

typedef struct _MMFUNCTION_TABLE
{
    union
    {
        MMGETCAPS_FUNC              GetCapabilities;
        MMGETWAVEOUTCAPS_FUNC       GetWaveOutCapabilities;
        MMGETWAVEINCAPS_FUNC        GetWaveInCapabilities;
        MMGETMIDIOUTCAPS_FUNC       GetMidiOutCapabilities;
        MMGETMIDIINCAPS_FUNC        GetMidiInCapabilities;
    };

    MMOPEN_FUNC                     Open;
    MMCLOSE_FUNC                    Close;

    MMWAVEQUERYFORMATSUPPORT_FUNC   QueryWaveFormatSupport;
    MMWAVESETFORMAT_FUNC            SetWaveFormat;

    MMWAVEHEADER_FUNC               PrepareWaveHeader;
    MMWAVEHEADER_FUNC               UnprepareWaveHeader;
    MMWAVEHEADER_FUNC               SubmitWaveHeader;
} MMFUNCTION_TABLE, *PMMFUNCTION_TABLE;

typedef MMRESULT (*SOUND_THREAD_REQUEST_HANDLER)(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN  PVOID Parameter);

typedef struct _SOUND_THREAD
{
    HANDLE Handle;
    BOOL Running;

    struct
    {
        HANDLE Ready;
        HANDLE Request;
        HANDLE Done;
    } Events;

    struct
    {
        SOUND_THREAD_REQUEST_HANDLER Handler;
        struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance;
        PVOID Parameter;
        MMRESULT Result;
    } Request;
} SOUND_THREAD, *PSOUND_THREAD;

typedef struct _SOUND_DEVICE
{
    struct _SOUND_DEVICE* Next;
    struct _SOUND_DEVICE_INSTANCE* HeadInstance;
    struct _SOUND_DEVICE_INSTANCE* TailInstance;
    MMDEVICE_TYPE Type;
    PVOID Identifier;       /* Path for NT4 drivers */
    /*PWSTR Path;*/
    MMFUNCTION_TABLE FunctionTable;
} SOUND_DEVICE, *PSOUND_DEVICE;

typedef struct _SOUND_DEVICE_INSTANCE
{
    struct _SOUND_DEVICE_INSTANCE* Next;
    struct _SOUND_DEVICE* Device;
    PVOID Handle;
    struct _SOUND_THREAD* Thread;

    /* Stuff generously donated to us from WinMM */
    struct
    {
        HDRVR Handle;
        DWORD Flags;
        DWORD ClientCallback;
        DWORD ClientCallbackInstanceData;
    } WinMM;
} SOUND_DEVICE_INSTANCE, *PSOUND_DEVICE_INSTANCE;

/*
    reentrancy.c
*/

MMRESULT
InitEntrypointMutexes();

VOID
CleanupEntrypointMutexes();

VOID
AcquireEntrypointMutex(
    IN  MMDEVICE_TYPE DeviceType);

VOID
ReleaseEntrypointMutex(
    IN  MMDEVICE_TYPE DeviceType);


/*
    mme.c
*/

VOID
NotifyMmeClient(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  DWORD Message,
    IN  DWORD Parameter);

MMRESULT
MmeGetSoundDeviceCapabilities(
    IN  MMDEVICE_TYPE DeviceType,
    IN  DWORD DeviceId,
    IN  PVOID Capabilities,
    IN  DWORD CapabilitiesSize);

MMRESULT
MmeOpenWaveDevice(
    IN  MMDEVICE_TYPE DeviceType,
    IN  DWORD DeviceId,
    IN  LPWAVEOPENDESC OpenParameters,
    IN  DWORD Flags,
    OUT DWORD* PrivateHandle);

MMRESULT
MmeCloseDevice(
    IN  DWORD PrivateHandle);

#define MmePrepareWaveHeader(private_handle, header) \
    PrepareWaveHeader((PSOUND_DEVICE_INSTANCE)private_handle, (PWAVEHDR)header)

#define MmeUnprepareWaveHeader(private_handle, header) \
    UnprepareWaveHeader((PSOUND_DEVICE_INSTANCE)private_handle, (PWAVEHDR)header)

#define MmeSubmitWaveHeader(private_handle, header) \
    SubmitWaveHeader((PSOUND_DEVICE_INSTANCE)private_handle, (PWAVEHDR)header)


/*
    capabilities.c
*/

MMRESULT
GetSoundDeviceCapabilities(
    IN  PSOUND_DEVICE SoundDevice,
    OUT PVOID Capabilities,
    IN  DWORD CapabilitiesSize);


/*
    devicelist.c
*/

ULONG
GetSoundDeviceCount(
    IN  MMDEVICE_TYPE DeviceType);

BOOLEAN
IsValidSoundDevice(
    IN  PSOUND_DEVICE SoundDevice);

MMRESULT
ListSoundDevice(
    IN  MMDEVICE_TYPE DeviceType,
    IN  PVOID Identifier OPTIONAL,
    OUT PSOUND_DEVICE* SoundDevice OPTIONAL);

MMRESULT
UnlistSoundDevice(
    IN  MMDEVICE_TYPE DeviceType,
    IN  PSOUND_DEVICE SoundDevice);

MMRESULT
UnlistSoundDevices(
    IN  MMDEVICE_TYPE DeviceType);

VOID
UnlistAllSoundDevices();

MMRESULT
GetSoundDevice(
    IN  MMDEVICE_TYPE DeviceType,
    IN  DWORD DeviceIndex,
    OUT PSOUND_DEVICE* Device);

MMRESULT
GetSoundDeviceIdentifier(
    IN  PSOUND_DEVICE SoundDevice,
    OUT PVOID* Identifier);

MMRESULT
GetSoundDeviceType(
    IN  PSOUND_DEVICE SoundDevice,
    OUT PMMDEVICE_TYPE DeviceType);


/*
    functiontable.c
*/

MMRESULT
SetSoundDeviceFunctionTable(
    IN  PSOUND_DEVICE SoundDevice,
    IN  PMMFUNCTION_TABLE FunctionTable OPTIONAL);

MMRESULT
GetSoundDeviceFunctionTable(
    IN  PSOUND_DEVICE SoundDevice,
    OUT PMMFUNCTION_TABLE* FunctionTable);


/*
    deviceinstance.c
*/

BOOLEAN
IsValidSoundDeviceInstance(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance);

MMRESULT
CreateSoundDeviceInstance(
    IN  PSOUND_DEVICE SoundDevice,
    OUT PSOUND_DEVICE_INSTANCE* SoundDeviceInstance);

MMRESULT
DestroySoundDeviceInstance(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance);

MMRESULT
DestroyAllSoundDeviceInstances(
    IN  PSOUND_DEVICE SoundDevice);

MMRESULT
GetSoundDeviceFromInstance(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    OUT PSOUND_DEVICE* SoundDevice);

MMRESULT
GetSoundDeviceInstanceHandle(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    OUT PVOID* Handle);

MMRESULT
SetSoundDeviceInstanceMmeData(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  HDRVR MmeHandle,
    IN  DWORD ClientCallback,
    IN  DWORD ClientCallbackData,
    IN  DWORD Flags);


/*
    thread.c
*/

MMRESULT
CreateSoundThread(
    OUT PSOUND_THREAD* Thread);

MMRESULT
DestroySoundThread(
    IN  PSOUND_THREAD Thread);

MMRESULT
CallSoundThread(
    IN  PSOUND_THREAD Thread,
    IN  SOUND_THREAD_REQUEST_HANDLER RequestHandler,
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance OPTIONAL,
    IN  PVOID Parameter OPTIONAL);


/*
    utility.c
*/

PVOID
AllocateMemory(
    IN  UINT Size);

VOID
FreeMemory(
    IN  PVOID Pointer);

UINT
GetMemoryAllocationCount();

UINT
GetDigitCount(
    IN  UINT Number);

MMRESULT
Win32ErrorToMmResult(
    IN  UINT ErrorCode);

MMRESULT
TranslateInternalMmResult(
    IN  MMRESULT Result);


/*
    wave/format.c
*/

MMRESULT
QueryWaveDeviceFormatSupport(
    IN  PSOUND_DEVICE SoundDevice,
    IN  LPWAVEFORMATEX Format,
    IN  DWORD FormatSize);

MMRESULT
SetWaveDeviceFormat(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  LPWAVEFORMATEX Format,
    IN  DWORD FormatSize);


/*
    wave/header.c
*/

MMRESULT
PrepareWaveHeader(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PWAVEHDR Header);

MMRESULT
UnprepareWaveHeader(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PWAVEHDR Header);

MMRESULT
SubmitWaveHeader(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PWAVEHDR Header);


/*
    kernel.c
*/

#if 0
#define QueryDevice(h, ctl, o, o_size, xfer, ovl) \
    Win32ErrorToMmResult( \
        DeviceIoControl(h, ctl, NULL, 0, o, o_size, xfer, ovl) != 0 \
        ? ERROR_SUCCESS : GetLastError() \
    )

#define ControlDevice(h, ctl, i, i_size, xfer, ovl) \
    Win32ErrorToMmResult( \
        DeviceIoControl(h, ctl, i, i_size, NULL, 0, xfer, ovl) != 0 \
        ? ERROR_SUCCESS : GetLastError() \
    )

#define QuerySoundDevice(sd, ctl, o, o_size, xfer) \
    SoundDeviceIoControl(sd, ctl, NULL, 0, o, o_size, xfer)

#define ControlSoundDevice(sd, ctl, i, i_size, xfer) \
    SoundDeviceIoControl(sd, ctl, i, i_size, NULL, 0, xfer)
#endif

MMRESULT
OpenKernelSoundDeviceByName(
    IN  PWSTR DevicePath,
    IN  BOOLEAN ReadOnly,
    OUT PHANDLE Handle);

MMRESULT
OpenKernelSoundDevice(
    IN  PSOUND_DEVICE SoundDevice,
    IN  BOOLEAN ReadOnly,
    OUT PHANDLE Handle);

MMRESULT
CloseKernelSoundDevice(
    IN  HANDLE Handle);

MMRESULT
SyncOverlappedDeviceIoControl(
    IN  HANDLE SoundDeviceInstance,
    IN  DWORD IoControlCode,
    IN  LPVOID InBuffer,
    IN  DWORD InBufferSize,
    OUT LPVOID OutBuffer,
    IN  DWORD OutBufferSize,
    OUT LPDWORD BytesTransferred OPTIONAL);


#if 0

typedef UCHAR MMDEVICE_TYPE, *PMMDEVICE_TYPE;

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
    MMSETWAVESTATE_FUNC     BreakWaveDeviceLoop;
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


    /* Device-specific parameters */
    union
    {
        WAVE_STREAM_INFO Wave;
    } Streaming;
} SOUND_DEVICE_INSTANCE, *PSOUND_DEVICE_INSTANCE;

#endif

#endif
