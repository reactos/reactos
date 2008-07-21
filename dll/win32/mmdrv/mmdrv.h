/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS Multimedia
 * FILE:                 dll/win32/mmdrv/mmdrv.h
 * PURPOSE:              Multimedia User Mode Driver (header)
 * PROGRAMMER:           Andrew Greenwood
 *                       Aleksey Bragin
 * UPDATE HISTORY:
 *                       Jan 30, 2004: Imported into ReactOS tree
 *                       Jan 10, 2007: Rewritten and tidied up
 */

#ifndef MMDRV_H
#define MMDRV_H

#include <mmioctl.h>
#include <mmddk.h>

#include <stdio.h>
#include <debug.h>

/* Need to check these */
#define MAX_DEVICES             256
#define MAX_DEVICE_NAME_LENGTH  256
#define MAX_BUFFER_SIZE         1048576
#define MAX_WAVE_BYTES          1048576

/* Custom flag set when overlapped I/O is done */
#define WHDR_COMPLETE 0x80000000


/*
    The kinds of devices which MMSYSTEM/WINMM may request from us.
*/

typedef enum
{
    WaveOutDevice,
    WaveInDevice,
    MidiOutDevice,
    MidiInDevice,
    AuxDevice
} DeviceType;

#define IsWaveDevice(devicetype) \
    ( ( devicetype == WaveOutDevice ) || ( devicetype == WaveInDevice ) )

#define IsMidiDevice(devicetype) \
    ( ( devicetype == MidiOutDevice ) || ( devicetype == MidiInDevice ) )

#define IsAuxDevice(devicetype) \
    ( devicetype == AuxDevice )


/*
    We use these structures to store information regarding open devices. Since
    the main structure gets destroyed when a device is closed, I call this a
    "session".
*/

typedef struct
{
    OVERLAPPED overlap;
    LPWAVEHDR header;
} WaveOverlapInfo;

/*
typedef enum
{
    WaveAddBuffer,
    WaveClose,
    WaveReset,
    WaveRestart,
    SessionThreadTerminate,
    InvalidFunction
} ThreadFunction;
*/

/* Our own values, used with the session threads */
typedef DWORD ThreadFunction;
#define DRVM_TERMINATE   0xFFFFFFFE
#define DRVM_INVALID     0xFFFFFFFF

typedef enum
{
    WavePlaying,
    WaveStopped,
    WaveReset,
    WaveRestart
} WaveState;

typedef union
{
    PWAVEHDR wave_header;
    PMIDIHDR midi_header;
} MediaHeader;

/*
typedef union
{
    MediaHeader header;
} ThreadParameter;
*/

typedef struct _ThreadInfo
{
    HANDLE handle;
    HANDLE ready_event;
    HANDLE go_event;

    /*ThreadFunction function;*/
    DWORD function;
    PVOID parameter;

    MMRESULT result;
} ThreadInfo;

typedef struct _LoopInfo
{
    PWAVEHDR head;
    DWORD iterations;
} LoopInfo;

typedef struct _SessionInfo
{
    struct _SessionInfo* next;

    DeviceType device_type;
    DWORD device_id;

    HANDLE kernel_device_handle;

    /* These are all the same */
    union
    {
        HDRVR mme_handle;
        HWAVE mme_wave_handle;
        HMIDI mme_midi_handle;
    };

    /* If playback is paused or not */
    BOOL is_paused;

    /* Stuff passed to us from winmm */
    DWORD app_user_data;
    DWORD callback;

    DWORD flags;

    /* Can only be one or the other */
    union
    {
        PWAVEHDR wave_queue;
        PMIDIHDR midi_queue;
    };

    /* Current playback point */
    //PWAVEHDR next_buffer;

    /* Where in the current buffer we are */
    DWORD buffer_position;

//    DWORD remaining_bytes;

    LoopInfo loop;

    ThreadInfo thread;
} SessionInfo;

#undef ASSERT
#define ASSERT(condition) \
    if ( ! (condition) ) \
        DPRINT("ASSERT FAILED: %s\n", #condition);

/*
    MME interface
*/

BOOL
NotifyClient(
    SessionInfo* session_info,
    DWORD message,
    DWORD parameter1,
    DWORD parameter2);


/*
    Helpers
*/

MMRESULT
ErrorToMmResult(UINT error_code);


/* Kernel interface */

MMRESULT
CobbleDeviceName(
    DeviceType device_type,
    DWORD device_id,
    PWCHAR out_device_name);

MMRESULT
OpenKernelDevice(
    DeviceType device_type,
    DWORD device_id,
    DWORD access,
    HANDLE* handle);

VOID
CloseKernelDevice(HANDLE device_handle);

MMRESULT
SetDeviceData(
    HANDLE device_handle,
    DWORD ioctl,
    PBYTE input_buffer,
    DWORD buffer_size);

MMRESULT
GetDeviceData(
    HANDLE device_handle,
    DWORD ioctl,
    PBYTE output_buffer,
    DWORD buffer_size);


/* Session management */

MMRESULT
CreateSession(
    DeviceType device_type,
    DWORD device_id,
    SessionInfo** session_info);

VOID
DestroySession(SessionInfo* session);

SessionInfo*
GetSession(
    DeviceType device_type,
    DWORD device_id);

MMRESULT
StartSessionThread(SessionInfo* session_info);

MMRESULT
CallSessionThread(
    SessionInfo* session_info,
    ThreadFunction function,
    PVOID thread_parameter);

DWORD
HandleBySessionThread(
    DWORD private_handle,
    DWORD message,
    DWORD parameter);


/* General */

DWORD
GetDeviceCount(DeviceType device_type);

DWORD
GetDeviceCapabilities(
    DeviceType device_type,
    DWORD device_id,
    PVOID capabilities,
    DWORD capabilities_size);

DWORD
OpenDevice(
    DeviceType device_type,
    DWORD device_id,
    PVOID open_descriptor,
    DWORD flags,
    DWORD private_handle);

DWORD
CloseDevice(
    DWORD private_handle);

DWORD
PauseDevice(
    DWORD private_handle);

DWORD
RestartDevice(
    DWORD private_handle);

DWORD
ResetDevice(
    DWORD private_handle);

DWORD
GetPosition(
    DWORD private_handle,
    PMMTIME time,
    DWORD time_size);

DWORD
BreakLoop(DWORD private_handle);

DWORD
QueryWaveFormat(
    DeviceType device_type,
    PVOID lpFormat);

DWORD
WriteWaveBuffer(
    DWORD private_handle,
    PWAVEHDR wave_header,
    DWORD wave_header_size);





/* wave thread */

DWORD
WaveThread(LPVOID parameter);


/* Wave I/O */

VOID
PerformWaveIO(SessionInfo* session_info);


CRITICAL_SECTION critical_section;



#endif
