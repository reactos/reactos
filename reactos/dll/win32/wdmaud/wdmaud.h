/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS Multimedia
 * FILE:                 lib/wdmaud/wdmaud.h
 * PURPOSE:              WDM Audio Support - Common header
 * PROGRAMMER:           Andrew Greenwood
 * UPDATE HISTORY:
 *                       Nov 12, 2005: Declarations for debugging + interface
 */

#ifndef __WDMAUD_PRIVATE_H__
#define __WDMAUD_PRIVATE_H__

/* Debugging */


/*
	Some of this stuff belongs in ksmedia.h or other such global includes.
*/

#include <stdio.h>

#include <debug.h>
#include <ntddk.h>

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>

/* HACK! */
#define DbgPrint printf


/*
    Handy macros
*/

#define REPORT_MM_RESULT(message, success) \
    DPRINT("%s %s\n", message, success == MMSYSERR_NOERROR ? "succeeded" : "failed")

#define MM_SUCCESS(value) \
    ( value == MMSYSERR_NOERROR )

#define MM_FAILURE(value) \
    ( value != MMSYSERR_NOERROR )


#define GOBAL_CALLBACKS_PATH L"Global\\WDMAUD_Callbacks"


/*
	Private IOCTLs shared between wdmaud.sys and wdmaud.drv
    
    TO ADD/MODIFY:
        IOCTL_WDMAUD_OPEN_PIN
        IOCTL_WDMAUD_WAVE_OUT_WRITE_PIN
        IOCTL_WDMAUD_WAVE_IN_READ_PIN
        IOCTL_WDMAUD_MIXER_CLOSE
        IOCTL_WDMAUD_MIXER_OPEN
        IOCTL_WDMAUD_MIDI_IN_READ_PIN
        IOCTL_WDMAUD_MIXER_GETLINEINFO
        IOCTL_WDMAUD_MIXER_GETHARDWAREEVENTDATA
        IOCTL_WDMAUD_MIXER_SETCONTROLDETAILS
        IOCTL_WDMAUD_MIXER_GETCONTROLDETAILS
        IOCTL_WDMAUD_MIXER_GETLINECONTROLS
*/

/* 0x1d8000 */
#define IOCTL_WDMAUD_HELLO \
	CTL_CODE(FILE_DEVICE_SOUND, 0x0000, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_WDMAUD_ADD_DEVICE         0x1d8004
#define IOCTL_WDMAUD_REMOVE_DEVICE      0x1d8008
#define IOCTL_WDMAUD_GET_CAPABILITIES   0x1d800c
#define IOCTL_WDMAUD_GET_DEVICE_COUNT   0x1d8010
#define IOCTL_WDMAUD_OPEN_DEVICE        0x1d8014
#define IOCTL_WDMAUD_CLOSE_DEVICE       0x1d8018
#define IOCTL_WDMAUD_AUX_GET_VOLUME     0x1d801c
#define IOCTL_WDMAUD_AUX_SET_VOLUME     0x1d8020

/* 0x1d8024 */
#define IOCTL_WDMAUD_GOODBYE \
	CTL_CODE(FILE_DEVICE_SOUND, 0x0009, METHOD_BUFFERED, FILE_WRITE_ACCESS)

#define IOCTL_WDMAUD_SET_PREFERRED      0x1d8028

#define IOCTL_WDMAUD_WAVE_OUT_STOP      0x1d8100
#define IOCTL_WDMAUD_WAVE_OUT_START     0x1d8104
#define IOCTL_WDMAUD_WAVE_OUT_RESET     0x1d8108
#define IOCTL_WDMAUD_BREAK_LOOP         0x1d810c

#define IOCTL_WDMAUD_GET_WAVE_OUT_POS   0x1d8110	/* Does something funky */
#define IOCTL_WDMAUD_SET_VOLUME         0x1d8114	/* Already been covered? */
#define IOCTL_WDMAUD_UNKNOWN1           0x1d8118	/* Not used by wdmaud.drv */
#define IOCTL_WDMAUD_SUBMIT_WAVE_OUT_HDR 0x1d811c

#define IOCTL_WDMAUD_WAVE_IN_STOP       0x1d8140
#define IOCTL_WDMAUD_WAVE_IN_START      0x1d8144
#define IOCTL_WDMAUD_WAVE_IN_RESET      0x1d8148

#define IOCTL_WDMAUD_SUBMIT_WAVE_IN_HDR 0x1d8150    /* FIXME: Unsure about this */

#define IOCTL_WDMAUD_MIDI_OUT_SHORT_MESSAGE \
                                        0x1d8204	/* Wrong description? */

#define IOCTL_WDMAUD_UNKNOWN2           0x1d8208

#define IOCTL_WDMAUD_MIDI_OUT_LONG_MESSAGE \
                                        0x1d820c

#define IOCTL_WDMAUD_SUBMIT_MIDI_HDR    0x1d8210

#define IOCTL_WDMAUD_MIDI_IN_STOP       0x1d8240
#define IOCTL_WDMAUD_MIDI_IN_START      0x1d8244
#define IOCTL_WDMAUD_MIDI_IN_RESET      0x1d8248

#define IOCTL_WDMAUD_READ_MIDI_DATA     0x1d824c
#define IOCTL_WDMAUD_MIDI_MESSAGE       0x1d8300	/* Wrong description? */

#define IOCTL_WDMAUD_MIXER_UNKNOWN1     0x1d8310
#define IOCTL_WDMAUD_MIXER_UNKNOWN2     0x1d8314
#define IOCTL_WDMAUD_MIXER_UNKNOWN3     0x1d8318


/*
	Device Types
*/

enum
{
    WDMAUD_WAVE_IN = 0,
	WDMAUD_WAVE_OUT,

	WDMAUD_MIDI_IN,
	WDMAUD_MIDI_OUT,

    WDMAUD_MIXER,

	WDMAUD_AUX,

    /* For range checking */
    WDMAUD_MIN_DEVICE_TYPE = WDMAUD_WAVE_IN,
    WDMAUD_MAX_DEVICE_TYPE = WDMAUD_AUX
};

/*
    Some macros for device type matching and checking
*/

#define IsWaveInDeviceType(device_type)     (device_type == WDMAUD_WAVE_IN)
#define IsWaveOutDeviceType(device_type)    (device_type == WDMAUD_WAVE_OUT)
#define IsMidiInDeviceType(device_type)     (device_type == WDMAUD_MIDI_IN)
#define IsMidiOutDeviceType(device_type)    (device_type == WDMAUD_MIDI_OUT)
#define IsMixerDeviceType(device_type)      (device_type == WDMAUD_MIXER)
#define IsAuxDeviceType(device_type)        (device_type == WDMAUD_AUX)

#define IsWaveDeviceType(device_type) \
    (IsWaveInDeviceType(device_type) || IsWaveOutDeviceType(device_type))
#define IsMidiDeviceType(device_type) \
    (IsMidiInDeviceType(device_type) || IsMidiOutDeviceType(device_type))
    
#define IsValidDeviceType(device_type) \
    (device_type >= WDMAUD_MIN_DEVICE_TYPE && \
     device_type <= WDMAUD_MAX_DEVICE_TYPE)

/*
    The various "caps" (capabilities) structures begin with the same members,
    so a generic structure is defined here which can be accessed independently
    of a device type.
*/

/*
    This is used as a general-purpose structure to retrieve capabilities
    from the driver.
*/
typedef struct
{
    DWORD cbSize;
    LPVOID pCaps;
} MDEVICECAPSEX, *LPMDEVICECAPSEX;

/* Abstraction */
typedef LPVOID PWDMAUD_HEADER;

/*
    There are also various "opendesc" structures, but these don't have any
    common members. Regardless, this typedef simply serves as a placeholder
    to indicate that to access the members, it should be cast accordingly.

    TODO: Maybe have a generic OPEN routine that uses this?
*/
typedef struct OPENDESC *LPOPENDESC;

typedef struct
{
    DWORD sample_size;
    HANDLE thread;
    DWORD thread_id;

    union
    {
        LPWAVEHDR current_wave_header;
        LPMIDIHDR current_midi_header;
    };

    DWORD unknown_10;   /* pointer to something */
    DWORD unknown_14;

    LPCRITICAL_SECTION device_queue_guard;
    HANDLE queue_event;
    HANDLE exit_thread_event;

    DWORD unknown_24;

    DWORD is_paused;
    DWORD is_running;

    DWORD unknown_30;
    LPVOID midi_buffer;  /* for output ? */
    DWORD running_status;

    char signature[4];
} WDMAUD_DEVICE_STATE, *PWDMAUD_DEVICE_STATE;

typedef struct
{
    DWORD next_device;

    DWORD id;
    DWORD type;

    HWAVE handle;

    DWORD client_instance;
    DWORD client_callback;

    DWORD unknown_18;

    DWORD flags;
    DWORD ioctl_param2;
    DWORD ioctl_param1;
    DWORD with_critical_section;
    DWORD string_2c;

    DWORD unknown_30;

    DWORD playing_notes;

    DWORD unknown_38;
    DWORD unknown_3c;
    DWORD unknown_40;
    DWORD unknown_44;
    DWORD unknown_48;
    DWORD unknown_4C;
    DWORD unknown_50;

    DWORD beef;
    PWDMAUD_DEVICE_STATE state;
    char signature[4];
    WCHAR path[1];
} WDMAUD_DEVICE_INFO, *PWDMAUD_DEVICE_INFO;

typedef struct
{
    PWDMAUD_DEVICE_INFO offspring;  /* not sure about this */
    LPOVERLAPPED overlapped;
    char signature[4];
} WDMAUD_WAVE_PREPARATION_DATA, *PWDMAUD_WAVE_PREPARATION_DATA;

/* Ugh... */

typedef struct
{
    DWORD cbSize;   /* Maybe? */
    
} WDMAUD_CAPS, *PWDMAUD_CAPS;

/*
    Not quite sure what these are/do yet
*/

#define MAGIC_42    0x42424242  /* Queue critical section */
#define MAGIC_43    0x43434343  /* Queue critical section */
#define MAGIC_48    0x48484848  /* Exit-thread event */

#define IsQueueMagic(test_value) \
    ( ( (DWORD)test_value == MAGIC_42 ) || ( (DWORD)test_value == MAGIC_43) )


/*
    This should eventually be removed, but is used so we can be nosey and see
    what the kernel-mode wdmaud.sys is doing with our structures!
*/

#ifdef DUMP_WDMAUD_STRUCTURES

#define DUMP_MEMBER(struct, member) \
    DPRINT("%s : %d [0x%x]\n", #member, (int) struct->member, (int) struct->member);

#define DUMP_WDMAUD_DEVICE_INFO(info) \
    { \
        DPRINT("-- %s --\n", #info); \
        DUMP_MEMBER(info, unknown_00); \
        DUMP_MEMBER(info, id); \
        DUMP_MEMBER(info, type); \
        DUMP_MEMBER(info, wave_handle); \
        DUMP_MEMBER(info, client_instance); \
        DUMP_MEMBER(info, client_callback); \
        DUMP_MEMBER(info, unknown_18); \
        DUMP_MEMBER(info, flags); \
        DUMP_MEMBER(info, ioctl_param2); \
        DUMP_MEMBER(info, ioctl_param1); \
        DUMP_MEMBER(info, with_critical_section); \
        DUMP_MEMBER(info, string_2c); \
        DUMP_MEMBER(info, unknown_30); \
        DUMP_MEMBER(info, playing_notes); \
        DUMP_MEMBER(info, unknown_38); \
        DUMP_MEMBER(info, unknown_3c); \
        DUMP_MEMBER(info, unknown_40); \
        DUMP_MEMBER(info, unknown_44); \
        DUMP_MEMBER(info, unknown_48); \
        DUMP_MEMBER(info, unknown_4C); \
        DUMP_MEMBER(info, unknown_50); \
        DUMP_MEMBER(info, beef); \
        DUMP_MEMBER(info, state); \
        DUMP_MEMBER(info, signature); \
    }

#else

#define DUMP_MEMBER(struct, member)
#define DUMP_WDMAUD_DEVICE_INFO(info)

#endif


/* Helper (helper.c) funcs */

MMRESULT TranslateWinError(DWORD error);
#define GetLastMmError()        TranslateWinError(GetLastError());


/* user.c */

void
NotifyClient(
    PWDMAUD_DEVICE_INFO device,
    DWORD message,
    DWORD p1,
    DWORD p2
);


/* kernel.c */

BOOL
EnableKernelInterface();

BOOL
DisableKernelInterface();

HANDLE
GetKernelInterface();

MMRESULT
CallKernelDevice(
    PWDMAUD_DEVICE_INFO device,
    DWORD ioctl_code,
    DWORD param1,
    DWORD param2);

/* devices.c */

BOOL
IsValidDevicePath(WCHAR* path);

MMRESULT
ValidateDeviceData(
    PWDMAUD_DEVICE_INFO device_data,
    BOOL require_state
);
/*
MMRESULT ValidateDeviceState(PWDMAUD_DEVICE_STATE state);
MMRESULT ValidateDeviceStateEvents(PWDMAUD_DEVICE_STATE state);
MMRESULT ValidateDeviceInfoAndState(PWDMAUD_DEVICE_INFO device_info);
*/

/* TODO: Add ID parameter */

PWDMAUD_DEVICE_INFO
CreateDeviceData(
    CHAR device_type,
    DWORD device_id,
    WCHAR* device_path,
    BOOL with_state
);

void
DeleteDeviceData(PWDMAUD_DEVICE_INFO device_data);

/* mixer ... */

MMRESULT ModifyDevicePresence(
    CHAR device_type,
    WCHAR* device_path,
    BOOL adding);

#define AddDevice(device_type, device_path) \
	ModifyDevicePresence(device_type, device_path, TRUE)

#define AddWaveInDevice(device_path) \
        AddDevice(WDMAUD_WAVE_IN, device_path)
#define AddWaveOutDevice(device_path) \
        AddDevice(WDMAUD_WAVE_OUT, device_path)
#define AddMidiInDevice(device_path) \
        AddDevice(WDMAUD_MIDI_IN,  device_path)
#define AddMidiOutDevice(device_path) \
        AddDevice(WDMAUD_MIDI_OUT, device_path)
#define AddMixerDevice(device_path) \
        AddDevice(WDMAUD_MIXER, device_path)
#define AddAuxDevice(device_path) \
        AddDevice(WDMAUD_AUX, device_path)

#define RemoveDevice(device_type, device_path) \
        ModifyDevicePresence(device_type, device_path, FALSE)

#define RemoveWaveInDevice(device_path) \
        RemoveDevice(WDMAUD_WAVE_IN, device_path)
#define RemoveWaveOutDevice(device_path) \
        RemoveDevice(WDMAUD_WAVE_OUT, device_path)
#define RemoveMidiInDevice(device_path) \
        RemoveDevice(WDMAUD_MIDI_IN, device_path)
#define RemoveMidiOutDevice(device_path) \
        RemoveDevice(WDMAUD_MIDI_OUT, device_path)
#define RemoveMixerDevice(device_path) \
        RemoveDevice(WDMAUD_MIXER, device_path)
#define RemoveAuxDevice(device_path) \
        RemoveDevice(WDMAUD_AUX, device_path)


DWORD
GetDeviceCount(CHAR device_type, WCHAR* device_path);

#define GetWaveInCount(device_path)  GetDeviceCount(WDMAUD_WAVE_IN,  device_path)
#define GetWaveOutCount(device_path) GetDeviceCount(WDMAUD_WAVE_OUT, device_path)
#define GetMidiInCount(device_path)  GetDeviceCount(WDMAUD_MIDI_IN,  device_path)
#define GetMidiOutCount(device_path) GetDeviceCount(WDMAUD_MIDI_OUT, device_path)
#define GetMixerCount(device_path)   GetDeviceCount(WDMAUD_MIXER,    device_path)
#define GetAuxCount(device_path)     GetDeviceCount(WDMAUD_AUX,      device_path)

MMRESULT
GetDeviceCapabilities(
    CHAR device_type,
    DWORD device_id,
    WCHAR* device_path,
    LPMDEVICECAPSEX caps
);

#define GetWaveInCapabilities(id, device_path, caps) \
        GetDeviceCapabilities(WDMAUD_WAVE_IN, id, device_path, caps);
#define GetWaveOutCapabilities(id, device_path, caps) \
        GetDeviceCapabilities(WDMAUD_WAVE_OUT, id, device_path, caps);
#define GetMidiInCapabilities(id, device_path, caps) \
        GetDeviceCapabilities(WDMAUD_MIDI_IN, id, device_path, caps);
#define GetMidiOutCapabilities(id, device_path, caps) \
        GetDeviceCapabilities(WDMAUD_MIDI_OUT, id, device_path, caps);
#define GetMixerCapabilities(id, device_path, caps) \
        GetDeviceCapabilities(WDMAUD_MIXER, id, device_path, caps);
#define GetAuxCapabilities(id, device_path, caps) \
        GetDeviceCapabilities(WDMAUD_AUX, id, device_path, caps);

MMRESULT
OpenDeviceViaKernel(
    PWDMAUD_DEVICE_INFO device,
    LPWAVEFORMATEX format
);

MMRESULT
OpenDevice(
    CHAR device_type,
    DWORD device_id,
    LPVOID open_descriptor,
    DWORD flags,
    PWDMAUD_DEVICE_INFO* user_data
);


/* wave.c */
#if 0
MMRESULT
OpenWaveDevice(
    CHAR device_type,
    DWORD device_id,
    LPWAVEOPENDESC open_details,
    DWORD flags,
    DWORD user_data
);
#endif

#define OpenWaveInDevice(id, open_details, flags, user_data) \
        OpenDevice(WDMAUD_WAVE_IN, id, open_details, flags, user_data);
#define OpenWaveOutDevice(id, open_details, flags, user_data) \
        OpenDevice(WDMAUD_WAVE_OUT, id, open_details, flags, user_data);
#define OpenMidiInDevice(id, open_details, flags, user_data) \
        OpenDevice(WDMAUD_MIDI_IN, id, open_details, flags, user_data);
#define OpenMidiOutDevice(id, open_details, flags, user_data) \
        OpenDevice(WDMAUD_MIDI_OUT, id, open_details, flags, user_data);


MMRESULT
CloseWaveDevice(
    PWDMAUD_DEVICE_INFO device
);

#define SET_WAVEHDR_FLAG(header, flag) \
    header->dwFlags |= flag

#define CLEAR_WAVEHDR_FLAG(header, flag) \
    header->dwFlags &= ~flag

#define IS_WAVEHDR_FLAG_SET(header, flag) \
    ( header->dwFlags & flag )

MMRESULT
ValidateWavePreparationData(PWDMAUD_WAVE_PREPARATION_DATA prep_data);

MMRESULT
ValidateWaveHeader(PWAVEHDR header);

MMRESULT
PrepareWaveHeader(
    PWDMAUD_DEVICE_INFO device,
    PWAVEHDR header
);

MMRESULT
UnprepareWaveHeader(PWAVEHDR header);

#define IsHeaderPrepared(header) \
    ( header->reserved != 0 )

MMRESULT
CompleteWaveHeader(PWAVEHDR header);

MMRESULT
WriteWaveData(PWDMAUD_DEVICE_INFO device, PWAVEHDR header);


/* midi.c */

#if 0
MMRESULT
OpenMidiDevice(
    CHAR device_type,
    DWORD device_id,
    LPMIDIOPENDESC open_details,
    DWORD flags,
    DWORD user_data
);
#endif

MMRESULT
CloseMidiDevice(
    PWDMAUD_DEVICE_INFO device
);

MMRESULT
WriteMidiShort(
    PWDMAUD_DEVICE_INFO device,
    DWORD message
);

/* FIXME: Bad params */
MMRESULT
WriteMidiBuffer(PWDMAUD_DEVICE_INFO device);

MMRESULT
ResetMidiDevice(PWDMAUD_DEVICE_INFO device);




/* threads.c */

BOOL CreateCompletionThread(PWDMAUD_DEVICE_INFO device);


/* MORE... */

/*
    DEBUGGING + RESOURCE TRACKING
*/

LPVOID AllocMem(DWORD size);
VOID FreeMem(LPVOID pointer);
VOID ReportMem();

#define AutoAlloc(type) \
    (type*) MemAlloc(sizeof(type));

#endif
