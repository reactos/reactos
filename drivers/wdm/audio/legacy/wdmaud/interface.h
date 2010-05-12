#pragma once

///
/// WDMAUD Interface Definition
///
/// History: 12/02/2008 Created

// These are now in sndtypes.h
/*
typedef enum
{
    DEVICE_TYPE_NONE = 0,
    DEVICE_TYPE_WAVE_OUT,
    DEVICE_TYPE_WAVE_IN,
    DEVICE_TYPE_MIDI_IN,
    DEVICE_TYPE_MIDI_OUT,
    DEVICE_TYPE_AUX_IN,
    DEVICE_TYPE_AUX_OUT

}AUDIO_DEVICE_TYPE;
*/

#include <sndtypes.h>

typedef struct
{
    KSSTREAM_HEADER Header;
    SOUND_DEVICE_TYPE DeviceType;
    ULONG_PTR DeviceIndex;

    HANDLE hDevice;
    ULONG DeviceCount;
    ULONG Flags;

    union
    {
        MIXERCAPSW    MixCaps;
        MIXERCONTROLDETAILS MixDetails;
        MIXERLINECONTROLSW MixControls;
        MIXERLINEW MixLine;
        WAVEFORMATEX WaveFormatEx;
        WAVEOUTCAPSW WaveOutCaps;
        AUXCAPSW     AuxCaps;
        WAVEINCAPSW  WaveInCaps;
        ULONGLONG    Position;
        struct
        {
            LPWSTR DeviceInterfaceString;
            ULONG DeviceInterfaceStringSize;
        }Interface;

        struct
        {
            HANDLE hMixer;
            ULONG NotificationType;
            ULONG Value;
        }MixerEvent;
        KSSTATE State;
        KSRESET ResetStream;
        ULONG Volume;
        ULONG FrameSize;
        HANDLE hNotifyEvent;
    }u;

}WDMAUD_DEVICE_INFO, *PWDMAUD_DEVICE_INFO;



/// IOCTL_OPEN_WDMAUD
///
/// Description: This IOCTL informs wdmaud that an application whats to use wdmsys for a waveOut / waveIn / aux operation
///
/// Arguments:   InputBuffer is a pointer to a WDMAUD_DEVICE_INFO structure,
///              InputBufferSize is size of WDMAUD_DEVICE_INFO structure
/// Note:        DeviceType identifies the device type, DeviceIndex the index, WaveFormatEx the device details
/// Result:      is returned in hDevice
/// Return Code: STATUS_SUCCESS indicates success, otherwise appropiate error code
/// Prequsites:  none

#define IOCTL_OPEN_WDMAUD \
    CTL_CODE(FILE_DEVICE_SOUND, \
             0, \
             METHOD_BUFFERED, \
             FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS)


/// IOCTL_CLOSE_WDMAUD
///
/// Description: This IOCTL informs that an application has finished with wdmsys and closes the connection
///
/// Arguments:   InputBuffer is a pointer to a WDMAUD_DEVICE_INFO structure,
///              InputBufferSize is size of WDMAUD_DEVICE_INFO structure
/// Note:        DeviceType, DeviceIndex and hDevice must be set
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prequsites: openend device

#define IOCTL_CLOSE_WDMAUD \
    CTL_CODE(FILE_DEVICE_SOUND, \
             1, \
             METHOD_BUFFERED, \
             FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS) \


/// IOCTL_GETNUMDEVS_TYPE
///
/// Description: This IOCTL queries the number of devices currently present of a specific type. The caller passes a WDMAUD_DEVICE_INFO structure.
///
/// Arguments:  InputBuffer is a pointer to a WDMAUD_DEVICE_INFO structure,
///             InputBufferSize is size of WDMAUD_DEVICE_INFO structure
/// Note:       The DeviceType contains the requested device type.
/// Result:     The result is returned in DeviceCount
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prequsites: none

#define IOCTL_GETNUMDEVS_TYPE \
    CTL_CODE(FILE_DEVICE_SOUND, \
             2, \
             METHOD_BUFFERED, \
             FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS)


/// IOCTL_SETDEVICE_STATE
///
/// Description: This IOCTL sets an opened waveOut / waveIn / midiIn / midiOut / aux device to specific state
///
/// Arguments:  InputBuffer is a pointer to a WDMAUD_DEVICE_INFO structure,
///             InputBufferSize is size of WDMAUD_DEVICE_INFO structure
/// Note:       The DeviceType, DeviceIndex, hDevice and State member must be set. State determines the new state
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prequsites: opened device

#define IOCTL_SETDEVICE_STATE \
    CTL_CODE(FILE_DEVICE_SOUND, \
             3, \
             METHOD_BUFFERED, \
             FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS)


/// IOCTL_GETDEVID
///
/// Description: This IOCTL returns the device index by its provided handle
///
/// Arguments:  InputBuffer is a pointer to a WDMAUD_DEVICE_INFO structure,
///             InputBufferSize is size of WDMAUD_DEVICE_INFO structure
/// Note:       The DeviceType and hDevice must be set
/// Result:     The result is returned in DeviceIndex
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prequsites: opened device

#define IOCTL_GETDEVID \
    CTL_CODE(FILE_DEVICE_SOUND, \
             4, \
             METHOD_BUFFERED, \
             FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS)


/// IOCTL_GETVOLUME
///
/// Description: This IOCTL returns the volume a device
///
/// Arguments:  InputBuffer is a pointer to a WDMAUD_DEVICE_INFO structure,
///             InputBufferSize is size of WDMAUD_DEVICE_INFO structure
/// Note:       The DeviceType and hDevice must be set
/// Result:     The result is returned in Volume
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prequsites: opened device

#define IOCTL_GETVOLUME \
    CTL_CODE(FILE_DEVICE_SOUND, \
             5, \
             METHOD_BUFFERED, \
             FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS)


/// IOCTL_SETVOLUME
///
/// Description: This IOCTL sets the volume a device
///
/// Arguments:  InputBuffer is a pointer to a WDMAUD_DEVICE_INFO structure,
///             InputBufferSize is size of WDMAUD_DEVICE_INFO structure
/// Note:       The DeviceType, hDevice and Volume must be set
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prequsites: opened device

#define IOCTL_SETVOLUME \
    CTL_CODE(FILE_DEVICE_SOUND, \
             6, \
             METHOD_BUFFERED, \
             FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS)


/// IOCTL_GETCAPABILTIES
///
/// Description: This IOCTL retrieves the capabilties of an specific device
///
/// Arguments:  InputBuffer is a pointer to a WDMAUD_DEVICE_INFO structure,
///             InputBufferSize is size of WDMAUD_DEVICE_INFO structure
/// Note:       The DeviceType and DeviceIndex must be set
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prequsites: none

#define IOCTL_GETCAPABILITIES \
    CTL_CODE(FILE_DEVICE_SOUND, \
             7, \
             METHOD_BUFFERED, \
             FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS)


/// IOCTL_WRITEDATA
///
/// Description: This IOCTL writes data to specified device
///
/// Arguments:  InputBuffer is a pointer to a WDMAUD_DEVICE_INFO structure,
///             InputBufferSize is size of WDMAUD_DEVICE_INFO structure
/// Note:       The DeviceType, DeviceIndex, hDevice, BufferSize and Buffer must be set
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prequsites: opened device

#define IOCTL_WRITEDATA \
    CTL_CODE(FILE_DEVICE_SOUND, \
             8, \
             METHOD_BUFFERED, \
             FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS)

/// IOCTL_GETPOS
///
/// Description: This IOCTL retrieves the current playback / write position
///
/// Arguments:  InputBuffer is a pointer to a WDMAUD_DEVICE_INFO structure,
///             InputBufferSize is size of WDMAUD_DEVICE_INFO structure
/// Note:       The DeviceType and hDevice must be set
/// Result:     The result is returned in Position
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prequsites: opened device

#define IOCTL_GETPOS \
    CTL_CODE(FILE_DEVICE_SOUND, \
             9, \
             METHOD_BUFFERED, \
             FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS)

/// IOCTL_GETFRAMESIZE
///
/// Description: This IOCTL retrieves the frame size requirements for an audio pin
///
/// Arguments:  InputBuffer is a pointer to a WDMAUD_DEVICE_INFO structure,
///             InputBufferSize is size of WDMAUD_DEVICE_INFO structure
/// Note:       The DeviceType and hDevice must be set
/// Result:     The result is returned in FrameSize
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prequsites: opened device

#define IOCTL_GETFRAMESIZE \
    CTL_CODE(FILE_DEVICE_SOUND, \
             10, \
             METHOD_BUFFERED, \
             FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS)

/// IOCTL_GETLINEINFO
///
/// Description: This IOCTL retrieves information on a mixerline
///
/// Arguments:  InputBuffer is a pointer to a WDMAUD_DEVICE_INFO structure,
///             InputBufferSize is size of WDMAUD_DEVICE_INFO structure
/// Note:       The hDevice member must be set
/// Result:     The result is returned in MixLine
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prequsites: opened device

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
/// Prequsites: opened device

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
/// Prequsites: opened device

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
/// Prequsites: opened device

#define IOCTL_GETCONTROLDETAILS \
    CTL_CODE(FILE_DEVICE_SOUND, \
             14, \
             METHOD_BUFFERED, \
             FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS)


/// IOCTL_QUERYDEVICEINTERFACESTRING
///
/// Description: This IOCTL queries the mixer / playback / recording device for its device interface string
///
/// Arguments:  InputBuffer is a pointer to a WDMAUD_DEVICE_INFO structure,
///             InputBufferSize is size of WDMAUD_DEVICE_INFO structure
/// Note:       The DeviceType, DeviceIndex must be set
/// Result:     The size is returned in Interface.DeviceInterfaceStringSize and if a buffer is supplied in Interface.DeviceInterfaceString
///             the device interface string is stored
/// ReturnCode:  STATUS_SUCCESS indicates success

#define IOCTL_QUERYDEVICEINTERFACESTRING \
    CTL_CODE(FILE_DEVICE_SOUND, \
             15, \
             METHOD_BUFFERED, \
             FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS)

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
             16, \
             METHOD_BUFFERED, \
             FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS)

/// IOCTL_RESET_STREAM
///
/// Description: This IOCTL instructs wdmaud to reset a stream
///
/// Arguments:  InputBuffer is a pointer to a WDMAUD_DEVICE_INFO structure,
///             InputBufferSize is size of WDMAUD_DEVICE_INFO structure
/// Note:       The hDevice member must be set and DeviceType
/// ReturnCode:  STATUS_SUCCESS indicates success

#define IOCTL_RESET_STREAM \
    CTL_CODE(FILE_DEVICE_SOUND, \
             17, \
             METHOD_BUFFERED, \
             FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS)
