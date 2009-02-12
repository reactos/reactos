#ifndef INTERFACE_H__
#define INTERFACE_H__

///
/// WDMAUD Interface Definition
///
/// History: 12/02/2008 Created


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

typedef struct
{
    AUDIO_DEVICE_TYPE DeviceType;
    ULONG DeviceIndex;

    HANDLE hDevice;
    ULONG DeviceCount;
    KSSTATE State;
    ULONG Volume;

    ULONG BufferSize;
    PUCHAR Buffer;

    union
    {
        WAVEFORMATEX WaveFormatEx;
        WAVEOUTCAPS WaveOutCaps;
        AUXCAPS     AuxCaps;
        WAVEINCAPS WaveInCaps;
    }u;

}WDMAUD_DEVICE_INFO;



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

#define IOCTL_OPEN_WDMAUD              CTL_CODE(FILE_DEVICE_SOUND, FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS, 0, METHOD_BUFFERED);


/// IOCTL_CLOSE_WDMAUD
///
/// Description: This IOCTL informs that an application has finished with wdmsys and closes the connection
///
/// Arguments:   InputBuffer is a pointer to a WDMAUD_DEVICE_INFO structure,
///              InputBufferSize is size of WDMAUD_DEVICE_INFO structure
/// Note:        DeviceType, DeviceIndex and hDevice must be set
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prequsites: openend device

#define IOCTL_CLOSE_WDMAUD             CTL_CODE(FILE_DEVICE_SOUND, FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS, 1, METHOD_BUFFERED);


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

#define IOCTL_GETNUMDEVS_TYPE               CTL_CODE(FILE_DEVICE_SOUND, FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS, 2, METHOD_BUFFERED);



/// IOCTL_SETDEVICE_STATE
///
/// Description: This IOCTL sets an opened waveOut / waveIn / midiIn / midiOut / aux device to specific state
///
/// Arguments:  InputBuffer is a pointer to a WDMAUD_DEVICE_INFO structure,
///             InputBufferSize is size of WDMAUD_DEVICE_INFO structure
/// Note:       The DeviceType, DeviceIndex, hDevice and State member must be set. State determines the new state
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prequsites: opened device

#define IOCTL_SETDEVICE_STATE               CTL_CODE(FILE_DEVICE_SOUND, FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS, 3, METHOD_BUFFERED);


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


#define IOCTL_GETDEVID                      CTL_CODE(FILE_DEVICE_SOUND, FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS, 4, METHOD_BUFFERED);



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


#define IOCTL_GETVOLUME                      CTL_CODE(FILE_DEVICE_SOUND, FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS, 5, METHOD_BUFFERED);


/// IOCTL_SETVOLUME
///
/// Description: This IOCTL sets the volume a device
///
/// Arguments:  InputBuffer is a pointer to a WDMAUD_DEVICE_INFO structure,
///             InputBufferSize is size of WDMAUD_DEVICE_INFO structure
/// Note:       The DeviceType, hDevice and Volume must be set
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prequsites: opened device


#define IOCTL_SETVOLUME                      CTL_CODE(FILE_DEVICE_SOUND, FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS, 6, METHOD_BUFFERED);


/// IOCTL_GETCAPABILTIES
///
/// Description: This IOCTL retrieves the capabilties of an specific device
///
/// Arguments:  InputBuffer is a pointer to a WDMAUD_DEVICE_INFO structure,
///             InputBufferSize is size of WDMAUD_DEVICE_INFO structure
/// Note:       The DeviceType and DeviceIndex must be set
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prequsites: none


#define IOCTL_GETCAPABILTIES                 CTL_CODE(FILE_DEVICE_SOUND, FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS, 7, METHOD_BUFFERED);


/// IOCTL_WRITEDATA
///
/// Description: This IOCTL writes data to specified device
///
/// Arguments:  InputBuffer is a pointer to a WDMAUD_DEVICE_INFO structure,
///             InputBufferSize is size of WDMAUD_DEVICE_INFO structure
/// Note:       The DeviceType, DeviceIndex, hDevice, BufferSize and Buffer must be set
/// ReturnCode:  STATUS_SUCCESS indicates success
/// Prequsites: opened device


#define IOCTL_WRITEDATA                 CTL_CODE(FILE_DEVICE_SOUND, FILE_CREATE_TREE_CONNECTION | FILE_ANY_ACCESS, 8, METHOD_BUFFERED);





#endif
