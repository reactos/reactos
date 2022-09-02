#pragma once

///
/// WDMAUD Interface Definition
///
/// History: 12/02/2008 Created

#include <sndtypes.h>

/* Fixing alignment is needed to make the struct size compatible with MS */
#pragma pack(1)

/* Contains device state management related stuff */
typedef struct
{
    DWORD Samples; // Verified to match Windows XP/2003, number of samples passed. Required for TIME_SAMPLES time format.
    HANDLE hThread; // Verified to match Windows XP/2003, the sound thread handle.
    DWORD unk2;

    LPWAVEHDR WaveQueue; // Verified to match Windows XP/2003, wave queue header.
    LPMIDIHDR MidiQueue; // Verified to match Windows XP/2003, midi queue header.

    DWORD unk3;
    PVOID QueueCriticalSection; // Verified to match Windows XP/2003, queue critical section.

    HANDLE hNotifyEvent; // Verified to match Windows XP/2003, queue notification event.
    HANDLE hStopEvent;   // Verified to match Windows XP/2003, queue stop event.

    BOOL unk5;
    BOOL bReset; // Verified to match Windows XP/2003, indicates whether device is starting to be reset.
    BOOL bStart; // Verified to match Windows XP/2003, indicates whether device is to be started.
    BOOL bStartInThread; // Verified to match Windows XP/2003, indicates whether the sound is started in thread.

    char unk7;
    char unk8;

}WDMAUD_DEVICE_STATE, *PWDMAUD_DEVICE_STATE;

/* Contains the main information about a sound device */
typedef struct
{
    PVOID unk1;
    DWORD DeviceIndex; // Verified to match Windows XP/2003, indicates the ordering number of device.
    SOUND_DEVICE_TYPE DeviceType; // Verified to match Windows XP/2003, indicates the type of the sound device, from 0 to 5.
    HANDLE hDevice; // Verified to match Windows XP/2003, a handle to an opened audio device.

    DWORD_PTR dwInstance; // Verified to match Windows XP/2003, WinMM client callback's instance from the caller's WAVEOPENDESC structure.
    DWORD_PTR dwCallback; // Verified to match Windows XP/2003, WinMM client callback from the caller's WAVEOPENDESC structure.

    DWORD unk2;

    DWORD Flags;  // Verified to match Windows XP/2003, wave open flags passed in by the caller.

    PVOID Buffer; // Verified to match Windows XP/2003, optional buffer containing an additional data passed by the caller. It's different for different device management actions.
    DWORD BufferSize; // Verified to match Windows XP/2003, the size, in bytes, of optional buffer passed by the caller.

    DWORD unk3;
    DWORD unk4;

    HANDLE hMixer;
    DWORD NotificationType;
    DWORD Value;

    DWORD unk5;
    DWORD unk6;
    DWORD unk7;
    INT unk8;

    PWDMAUD_DEVICE_STATE DeviceState; // Verified to match Windows XP/2003, WDMAUD_DEVICE_STATE structure for this device.

    LPCWSTR DeviceInterfaceString; // Verified to match Windows XP/2003, device interface string for this device.

}WDMAUD_DEVICE_INFO, *PWDMAUD_DEVICE_INFO;

#pragma pack()

/// IOCTL_INIT_WDMAUD
///
/// Description: This IOCTL does the initialization of wdmaud.sys
///
/// Arguments:   Buffer is NULL,
///              BufferSize is zero
/// Return Code: STATUS_SUCCESS indicates success, otherwise appropriate error code
/// Prerequisites:  none

#define IOCTL_INIT_WDMAUD \
    CTL_CODE(FILE_DEVICE_SOUND, \
             0x0000, \
             METHOD_BUFFERED, \
             FILE_WRITE_ACCESS)

/// IOCTL_ADD_DEVNODE
///
/// Description: This IOCTL adds a new device node
///
/// Arguments:   Buffer is NULL,
///              BufferSize is zero
/// Note:        DeviceType identifies the device type, DeviceIndex the index, WaveFormat the device details
/// Return Code: STATUS_SUCCESS indicates success, otherwise appropriate error code
/// Prerequisites:  none

#define IOCTL_ADD_DEVNODE \
    CTL_CODE(FILE_DEVICE_SOUND, \
             0x0001, \
             METHOD_BUFFERED, \
             FILE_WRITE_ACCESS)

/// IOCTL_REMOVE_DEVNODE
///
/// Description: This IOCTL removes an existing device node
///
/// Arguments:   Buffer is NULL,
///              BufferSize is zero
/// Note:        DeviceType identifies the device type, DeviceIndex the index, WaveFormat the device details
/// Return Code: STATUS_SUCCESS indicates success, otherwise appropriate error code
/// Prerequisites:  none

#define IOCTL_REMOVE_DEVNODE \
    CTL_CODE(FILE_DEVICE_SOUND, \
             0x0002, \
             METHOD_BUFFERED, \
             FILE_WRITE_ACCESS)

/// IOCTL_GETCAPABILITIES
///
/// Description: This IOCTL retrieves the capabilities of an specific wave out device
///
/// Arguments:  Buffer is a pointer to a capabilities data,
///             BufferSize is size of capabilities data
/// Note:       The DeviceType and DeviceIndex must be set
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prerequisites: none

#define IOCTL_GETCAPABILITIES \
    CTL_CODE(FILE_DEVICE_SOUND, \
             0x0003, \
             METHOD_BUFFERED, \
             FILE_WRITE_ACCESS)

/// IOCTL_GETNUMDEVS_TYPE
///
/// Description: This IOCTL queries the number of devices currently present of a specific type.
///
/// Arguments:  Buffer is NULL,
///             BufferSize is zero
/// Note:       The DeviceType contains the requested device type.
/// Result:     The result is returned in DeviceCount
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prerequisites: none

#define IOCTL_GETNUMDEVS_TYPE \
    CTL_CODE(FILE_DEVICE_SOUND, \
             0x0004, \
             METHOD_BUFFERED, \
             FILE_WRITE_ACCESS)


/// IOCTL_OPEN_WDMAUD
///
/// Description: This IOCTL informs wdmaud that an application whats to use wdmsys for a waveOut / waveIn / aux operation
///
/// Arguments:   Buffer is a pointer to the WAVEFORMATEX structure,
///              BufferSize is size of the WAVEFORMATEX structure
/// Note:        DeviceType identifies the device type, DeviceIndex the index, WaveFormat the device details
/// Result:      is returned in hDevice
/// Return Code: STATUS_SUCCESS indicates success, otherwise appropriate error code
/// Prerequisites:  none

#define IOCTL_OPEN_WDMAUD \
    CTL_CODE(FILE_DEVICE_SOUND, \
             0x0005, \
             METHOD_BUFFERED, \
             FILE_WRITE_ACCESS)

/// IOCTL_CLOSE_WDMAUD
///
/// Description: This IOCTL informs that an application has finished with wdmsys and closes the connection
///
/// Arguments:   Buffer is NULL,
///              BufferSize is zero
/// Note:        DeviceType, DeviceIndex and hDevice must be set
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prerequisites: Openend device

#define IOCTL_CLOSE_WDMAUD \
    CTL_CODE(FILE_DEVICE_SOUND, \
             0x006, \
             METHOD_BUFFERED, \
             FILE_WRITE_ACCESS)

/// IOCTL_GETVOLUME
///
/// Description: This IOCTL returns the volume to a device
///
/// Arguments:  InputBuffer is a pointer to a WDMAUD_DEVICE_INFO structure,
///             InputBufferSize is size of WDMAUD_DEVICE_INFO structure
/// Note:       The DeviceType and hDevice must be set
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prerequisites: Opened device

#define IOCTL_GETVOLUME \
    CTL_CODE(FILE_DEVICE_SOUND, \
             0x0007, \
             METHOD_BUFFERED, \
             FILE_WRITE_ACCESS)


/// IOCTL_SETVOLUME
///
/// Description: This IOCTL sets the volume for a device
///
/// Arguments:  InputBuffer is a pointer to a WDMAUD_DEVICE_INFO structure,
///             InputBufferSize is size of WDMAUD_DEVICE_INFO structure
/// Note:       The DeviceType, hDevice and Volume must be set
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prerequisites: Opened device

#define IOCTL_SETVOLUME \
    CTL_CODE(FILE_DEVICE_SOUND, \
             0x0008, \
             METHOD_BUFFERED, \
             FILE_WRITE_ACCESS)


/// IOCTL_SETVOLUME
///
/// Description: This IOCTL does unloading of wdmaud.sys
///
/// Arguments:  InputBuffer is NULL,
///             InputBufferSize is zero
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prerequisites: Initialized device

#define IOCTL_EXIT_WDMAUD \
    CTL_CODE(FILE_DEVICE_SOUND, \
             0x0009, \
             METHOD_BUFFERED, \
             FILE_WRITE_ACCESS)

/// IOCTL_PAUSE_PLAYBACK
///
/// Description: This IOCTL pauses playback of an opened waveOut device
///
/// Arguments:  Buffer is NULL,
///             BufferSize is zero
/// Note:       The DeviceType and hDevice members must be set. State determines the new state
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prerequisites: Opened device

#define IOCTL_PAUSE_PLAYBACK \
    CTL_CODE(FILE_DEVICE_SOUND, \
             0x0040, \
             METHOD_BUFFERED, \
             FILE_WRITE_ACCESS)

/// IOCTL_START_PLAYBACK
///
/// Description: This IOCTL starts playback for an opened waveOut device
///
/// Arguments:  Buffer is NULL,
///             BufferSize is zero
/// Note:       The DeviceType and hDevice members must be set. State determines the new state
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prerequisites: Opened device

#define IOCTL_START_PLAYBACK \
    CTL_CODE(FILE_DEVICE_SOUND, \
             0x0041, \
             METHOD_BUFFERED, \
             FILE_WRITE_ACCESS)

/// IOCTL_RESET_PLAYBACK
///
/// Description: This IOCTL resets the stream for an opened waveOut device to default state
///
/// Arguments:  Buffer is NULL,
///             BufferSize is zero
/// Note:       The DeviceType and hDevice members must be set. State determines the new state
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prerequisites: Opened device

#define IOCTL_RESET_PLAYBACK \
    CTL_CODE(FILE_DEVICE_SOUND, \
             0x0042, \
             METHOD_BUFFERED, \
             FILE_WRITE_ACCESS)

/// IOCTL_GETOUTPOS
///
/// Description: This IOCTL retrieves the current playback / write position
///
/// Arguments:  Buffer is a pointer to a variable that receives playback / write position,
///             BufferSize is set to size of DWORD
/// Note:       The DeviceType and hDevice must be set
/// Result:     The result is returned in Position
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prerequisites: Opened device

#define IOCTL_GETOUTPOS \
    CTL_CODE(FILE_DEVICE_SOUND, \
             0x0044, \
             METHOD_BUFFERED, \
             FILE_WRITE_ACCESS)

/// IOCTL_WRITEDATA
///
/// Description: This IOCTL writes data to the specified WaveOut device
///
/// Arguments:  Buffer is a pointer to a WAVEHDR structure,
///             BufferSize is size of WAVEHDR structure
/// Note:       The DeviceType and hDevice must be set
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prerequisites: Opened device

#define IOCTL_WRITEDATA \
    CTL_CODE(FILE_DEVICE_SOUND, \
             0x0047, \
             METHOD_BUFFERED, \
             FILE_WRITE_ACCESS)

/// IOCTL_PAUSE_CAPTURE
///
/// Description: This IOCTL pauses capture of an opened waveIn device
///
/// Arguments:  Buffer is NULL,
///             BufferSize is zero
/// Note:       The DeviceType and hDevice members must be set. State determines the new state
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prerequisites: Opened device

#define IOCTL_PAUSE_CAPTURE \
    CTL_CODE(FILE_DEVICE_SOUND, \
             0x0050, \
             METHOD_BUFFERED, \
             FILE_WRITE_ACCESS)

/// IOCTL_START_CAPTURE
///
/// Description: This IOCTL starts capture for an opened waveIn device
///
/// Arguments:  Buffer is NULL,
///             BufferSize is zero
/// Note:       The DeviceType and hDevice members must be set. State determines the new state
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prerequisites: Opened device

#define IOCTL_START_CAPTURE \
    CTL_CODE(FILE_DEVICE_SOUND, \
             0x0051, \
             METHOD_BUFFERED, \
             FILE_WRITE_ACCESS)

/// IOCTL_RESET_CAPTURE
///
/// Description: This IOCTL resets the stream for an opened waveIn device to default state
///
/// Arguments:  Buffer is NULL,
///             BufferSize is zero
/// Note:       The DeviceType and hDevice members must be set. State determines the new state
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prerequisites: Opened device

#define IOCTL_RESET_CAPTURE \
    CTL_CODE(FILE_DEVICE_SOUND, \
             0x0052, \
             METHOD_BUFFERED, \
             FILE_WRITE_ACCESS)

/// IOCTL_GETINPOS
///
/// Description: This IOCTL retrieves the current capture / read position
///
/// Arguments:  Buffer is a pointer to a variable that receives playback / write position,
///             BufferSize is set to size of DWORD
/// Note:       The DeviceType and hDevice must be set
/// Result:     The result is returned in Position
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prerequisites: Opened device

#define IOCTL_GETINPOS \
    CTL_CODE(FILE_DEVICE_SOUND, \
             0x0053, \
             METHOD_BUFFERED, \
             FILE_WRITE_ACCESS)

/// IOCTL_READDATA
///
/// Description: This IOCTL reads data from the specified WaveIn device
///
/// Arguments:  Buffer is a pointer to a WAVEHDR structure,
///             BufferSize is size of WAVEHDR structure
/// Note:       The DeviceType and hDevice must be set
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prerequisites: Opened device

#define IOCTL_READDATA \
    CTL_CODE(FILE_DEVICE_SOUND, \
             0x0054, \
             METHOD_BUFFERED, \
             FILE_WRITE_ACCESS)

/// IOCTL_OPEN_MIXER
///
/// Description: This IOCTL opens mixer device
///
/// Arguments:  InputBuffer is a pointer to a WDMAUD_DEVICE_INFO structure,
///             InputBufferSize is size of WDMAUD_DEVICE_INFO structure
/// Note:       The hDevice member must be set
/// Result:     The result is returned in MixLine
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prerequisites: Opened device

#define IOCTL_OPEN_MIXER \
    CTL_CODE(FILE_DEVICE_SOUND, \
             0x00C0, \
             METHOD_BUFFERED, \
             FILE_WRITE_ACCESS)

/// IOCTL_GET_MIXER_EVENT
///
/// Description: This IOCTL queries for wdmaud driver if there any new kernel streaming events available
///
/// Arguments:  InputBuffer is a pointer to a WDMAUD_DEVICE_INFO structure,
///             InputBufferSize is size of WDMAUD_DEVICE_INFO structure
/// Note:       The hDevice member must be set
/// Result:     The result is returned in the struct MixerInfo
/// ReturnCode:  STATUS_SUCCESS indicates success

#define IOCTL_GET_MIXER_EVENT \
    CTL_CODE(FILE_DEVICE_SOUND, \
             0x00C6, \
             METHOD_BUFFERED, \
             FILE_WRITE_ACCESS)

/// IOCTL_GETLINEINFO
///
/// Description: This IOCTL retrieves information on a mixerline
///
/// Arguments:  InputBuffer is a pointer to a WDMAUD_DEVICE_INFO structure,
///             InputBufferSize is size of WDMAUD_DEVICE_INFO structure
/// Note:       The hDevice member must be set
/// Result:     The result is returned in MixLine
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prerequisites: Opened device

#define IOCTL_GETLINEINFO \
    CTL_CODE(FILE_DEVICE_SOUND, \
             11, \
             METHOD_BUFFERED, \
             FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS)


/// IOCTL_GETLINECONTROLS
///
/// Description: This IOCTL retrieves controls of a mixerline
///
/// Arguments:  InputBuffer is a pointer to a WDMAUD_DEVICE_INFO structure,
///             InputBufferSize is size of WDMAUD_DEVICE_INFO structure
/// Note:       The hDevice member must be set
/// Result:     The result is returned in MixControls
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prerequisites: Opened device

#define IOCTL_GETLINECONTROLS \
    CTL_CODE(FILE_DEVICE_SOUND, \
             12, \
             METHOD_BUFFERED, \
             FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS)


/// IOCTL_SETCONTROLDETAILS
///
/// Description: This IOCTL sets details of a control of a mixerline
///
/// Arguments:  InputBuffer is a pointer to a WDMAUD_DEVICE_INFO structure,
///             InputBufferSize is size of WDMAUD_DEVICE_INFO structure
/// Note:       The hDevice member must be set
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prerequisites: Opened device

#define IOCTL_SETCONTROLDETAILS \
    CTL_CODE(FILE_DEVICE_SOUND, \
             13, \
             METHOD_BUFFERED, \
             FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS)


/// IOCTL_GETCONTROLDETAILS
///
/// Description: This IOCTL gets details of a control of a mixerline
///
/// Arguments:  InputBuffer is a pointer to a WDMAUD_DEVICE_INFO structure,
///             InputBufferSize is size of WDMAUD_DEVICE_INFO structure
/// Note:       The hDevice member must be set
/// Result:     The result is returned in MixDetails
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prerequisites: Opened device

#define IOCTL_GETCONTROLDETAILS \
    CTL_CODE(FILE_DEVICE_SOUND, \
             14, \
             METHOD_BUFFERED, \
             FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS)

