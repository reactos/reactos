/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS Multimedia
 * FILE:                 lib/wdmaud/devices.c
 * PURPOSE:              WDM Audio Support - Device Management
 * PROGRAMMER:           Andrew Greenwood
 * UPDATE HISTORY:
 *                       Nov 18, 2005: Created
 * 
 * WARNING! SOME OF THESE FUNCTIONS OUGHT TO COPY THE DEVICE INFO STRUCTURE
 * THAT HAS BEEN FED TO THEM!
*/

#include <windows.h>
#include "wdmaud.h"

const char WDMAUD_DEVICE_INFO_SIG[4] = "WADI";
const char WDMAUD_DEVICE_STATE_SIG[4] = "WADS";


/*
    IsValidDevicePath

    Just checks to see if the string containing the path to the device path
    (object) is a valid, readable string.
*/

BOOL IsValidDevicePath(WCHAR* path)
{
    if (IsBadReadPtr(path, 1))  /* TODO: Replace with flags */
    {
        DPRINT1("Bad interface\n");
        return FALSE;
    }

    /* Original driver seems to check for strlenW < 0x1000 */

    return TRUE;
}

/*
    ValidateDeviceData

    Checks that the memory pointed at by the device data pointer is writable,
    and that it has a valid signature.

    If the "state" member isn't NULL, the state structure is also validated
    in the same way. If the "require_state" parameter is TRUE and the "state"
    member is NULL, an error code is returned. Otherwise the "state" member
    isn't validated and no error occurs.
*/

MMRESULT ValidateDeviceData(
    PWDMAUD_DEVICE_INFO device,
    BOOL require_state
)
{
    if ( IsBadWritePtr(device, sizeof(WDMAUD_DEVICE_INFO)) )
    {
        DPRINT1("Device data structure not writable\n");
        return MMSYSERR_INVALPARAM;
    }

    if ( strncmp(device->signature, WDMAUD_DEVICE_INFO_SIG, 4) != 0 )
    {
        DPRINT1("Device signature is invalid\n");
        return MMSYSERR_INVALPARAM;
    }

    if ( ! IsValidDeviceType(device->type) )
    {
        DPRINT1("Invalid device type\n");
        return MMSYSERR_INVALPARAM;
    }

    if ( device->id > 100 )
    {
        DPRINT1("Device ID is out of range\n");
        return MMSYSERR_INVALPARAM;
    }

    /* Now we validate the device state (if present) */

    if ( device->state )
    {
        if ( IsBadWritePtr(device->state, sizeof(WDMAUD_DEVICE_INFO)) )
        {
            DPRINT1("Device state structure not writable\n");
            return MMSYSERR_INVALPARAM;
        }

        if ( strncmp(device->state->signature,
                     WDMAUD_DEVICE_STATE_SIG,
                     4) != 0 )
        {
            DPRINT1("Device state signature is invalid\n");
            return MMSYSERR_INVALPARAM;
        }

        /* TODO: Validate state events */
    }
    else if ( require_state )
    {
        return MMSYSERR_INVALPARAM;
    }

    return MMSYSERR_NOERROR;
}


/*
    ValidateDeviceStateEvents should be used in conjunction with the standard
    state validation routine (NOT on its own!)

    FIXME: The tests are wrong
*/
/*
MMRESULT ValidateDeviceStateEvents(PWDMAUD_DEVICE_STATE state)
{
    if ( ( (DWORD) state->exit_thread_event != 0x00000000 ) &&
         ( (DWORD) state->exit_thread_event != 0x48484848 ) )
    {
        DPRINT1("Bad exit thread event\n");
        return MMSYSERR_INVALPARAM;
    }

    if ( ( (DWORD) state->queue_event != 0x00000000 ) &&
         ( (DWORD) state->queue_event != 0x42424242 ) &&
         ( (DWORD) state->queue_event != 0x43434343 ) )
    {
        DPRINT1("Bad queue event\n");
        return MMSYSERR_INVALPARAM;
    }

    return MMSYSERR_NOERROR;
}
*/

/*
    CreateDeviceData

    This is a glorified memory allocation routine, which acts as a primitive
    constructor for a device data structure.

    It validates the device path given, allocates memory for both the device
    data and the device state data, copies the signatures over and sets the
    device type accordingly.

    In some cases, a state structure isn't required, so the creation of one can
    be avoided by passing FALSE for the "with_state" parameter.
*/

PWDMAUD_DEVICE_INFO
CreateDeviceData(
    CHAR device_type,
    DWORD device_id,
    WCHAR* device_path,
    BOOL with_state
)
{
    BOOL success = FALSE;
    PWDMAUD_DEVICE_INFO device = 0;
    int path_size = 0;

    DPRINT("Creating device data for device type %d\n", (int) device_type);

    if ( ! IsValidDevicePath(device_path) )
    {
        DPRINT1("No valid device interface given!\n");
        goto cleanup;
    }

    /* Take into account this is a unicode string... */
    path_size = (lstrlen(device_path) + 1) * sizeof(WCHAR);
    /* DPRINT("Size of path is %d\n", (int) path_size); */

    DPRINT("Allocating %d bytes for device data\n",
           path_size + sizeof(WDMAUD_DEVICE_INFO));

    device = (PWDMAUD_DEVICE_INFO)
        AllocMem(path_size + sizeof(WDMAUD_DEVICE_INFO));

    if ( ! device )
    {
        DPRINT1("Unable to allocate memory for device data (error %d)\n",
                (int) GetLastError());
        goto cleanup;
    }

    /* Copy the signature and device path */
    memcpy(device->signature, WDMAUD_DEVICE_INFO_SIG, 4);
    lstrcpy(device->path, device_path);

    /* Initialize these common members */
    device->id = device_id;
    device->type = device_type;

    if ( with_state )
    {
        /* Allocate device state structure */
        device->state = AllocMem(sizeof(WDMAUD_DEVICE_STATE));

        if ( ! device->state )
        {
            DPRINT1("Couldn't allocate memory for device state (error %d)\n",
                    (int) GetLastError());
            goto cleanup;
        }

        /* Copy the signature */
        memcpy(device->state->signature, WDMAUD_DEVICE_STATE_SIG, 4);
    }

    success = TRUE;

    cleanup :
    {
        if ( ! success )
        {
            if ( device )
            {
                if ( device->state )
                {
                    ZeroMemory(device->state->signature, 4);
                    FreeMem(device->state);
                }

                ZeroMemory(device->signature, 4);
                FreeMem(device);
            }
        }

        return (success ? device : NULL);
    }
}


/*
    DeleteDeviceData

    Blanks out the device and device state structures, and frees the memory
    associated with the structures.

    TODO: Free critical sections / events if set?
*/

void DeleteDeviceData(PWDMAUD_DEVICE_INFO device_data)
{
    DPRINT("Deleting device data\n");

    ASSERT( device_data );

    /* We don't really care if the structure is valid or not */
    if ( ! device_data )
        return;

    if ( device_data->state )
    {
        /* We DON'T want these to be set - should we clean up? */
        ASSERT ( ! device_data->state->device_queue_guard );
        ASSERT ( ! device_data->state->queue_event );
        ASSERT ( ! device_data->state->exit_thread_event );

        /* Insert a cow (not sure if this is right or not) */
        device_data->state->sample_size = 0xDEADBEEF;

        /* Overwrite the structure with zeroes and free it */
        ZeroMemory(device_data->state, sizeof(WDMAUD_DEVICE_STATE));
        FreeMem(device_data->state);
    }

    /* Overwrite the structure with zeroes and free it */
    ZeroMemory(device_data, sizeof(WDMAUD_DEVICE_INFO));
    FreeMem(device_data);
}

/*
    ModifyDevicePresence

    Use this to add or remove devices in the kernel-mode driver. If the
    "adding" parameter is TRUE, the device is added, otherwise it is removed.

    "device_type" is WDMAUD_WAVE_IN, WDMAUD_WAVE_OUT, etc...

    "device_path" specifies the NT object path of the device.

    (I'm not sure what happens to devices that are added but never removed.)
*/

MMRESULT ModifyDevicePresence(
    CHAR device_type,
    WCHAR* device_path,
    BOOL adding)
{
    DWORD ioctl = 0;
    PWDMAUD_DEVICE_INFO device_data = 0;
    MMRESULT result = MMSYSERR_ERROR;
    MMRESULT kernel_result = MMSYSERR_ERROR;

    DPRINT("ModifyDevicePresence - %s a device\n",
           adding ? "adding" : "removing");

    /* DPRINT("Topology path %S\n", device_path); */
    DPRINT("Devtype %d\n", (int) device_type);

    ASSERT( IsValidDeviceType(device_type) );
    ASSERT( device_path );

    device_data = CreateDeviceData(device_type, 0, device_path, FALSE);

    if ( ! device_data )
    {
        DPRINT1("Couldn't allocate memory for device data\n");
        result = MMSYSERR_NOMEM;
        goto cleanup;
    }

    ioctl = adding ? IOCTL_WDMAUD_ADD_DEVICE : IOCTL_WDMAUD_REMOVE_DEVICE;

    kernel_result = CallKernelDevice(device_data,
                                     ioctl,
                                     0,
                                     0);

    if ( kernel_result != MMSYSERR_NOERROR )
    {
        DPRINT1("WdmAudioIoControl FAILED with error %d\n", (int) kernel_result);

        switch ( kernel_result )
        {
            /* TODO: Translate into a real error code */
            default :
                result = MMSYSERR_ERROR;
        }

        goto cleanup;
    }

    DPRINT("ModifyDevicePresence succeeded\n");

    result = MMSYSERR_NOERROR;

    cleanup :
    {
        if ( device_data )
            DeleteDeviceData(device_data);

        return result;
    }
}

/*
    GetDeviceCount

    Pretty straightforward - pass the device type (WDMAUD_WAVE_IN, ...) and
    a topology device (NT object path) to obtain the number of devices
    present in that topology of that particular type.

    The topology path is supplied to us by winmm.
*/

DWORD GetDeviceCount(CHAR device_type, WCHAR* topology_path)
{
    PWDMAUD_DEVICE_INFO device_data;
    int device_count = 0;

    DPRINT("Topology path %S\n", topology_path);

    device_data = CreateDeviceData(device_type, 0, topology_path, FALSE);

    if (! device_data)
    {
        DPRINT1("Couldn't allocate device data\n");
        goto cleanup;
    }

    DPRINT("Getting num devs\n");

    device_data->with_critical_section = FALSE;

    if ( CallKernelDevice(device_data,
                          IOCTL_WDMAUD_GET_DEVICE_COUNT,
                          0,
                          0) != MMSYSERR_NOERROR )
    {
        DPRINT1("Failed\n");
        goto cleanup;
    }

    device_count = device_data->id;

    DPRINT("There are %d devs\n", device_count);

    cleanup :
    {
        if ( device_data )
            DeleteDeviceData(device_data);

        return device_count;
    }
}

/*
    GetDeviceCapabilities

    This uses a different structure to the traditional documentation, because
    we handle plug and play devices.

    Much sleep was lost over implementing this. I got the ID and type
    parameters the wrong way round!
*/

MMRESULT GetDeviceCapabilities(
    CHAR device_type,
    DWORD device_id,
    WCHAR* device_path,
    LPMDEVICECAPSEX caps
)
{
    PWDMAUD_DEVICE_INFO device = NULL;
    MMRESULT result = MMSYSERR_ERROR;

    DPRINT("Device path %S\n", device_path);

    /* Is this right? */
    if (caps->cbSize == 0)
    {
        DPRINT1("We appear to have been given an invalid parameter\n");
        return MMSYSERR_INVALPARAM;
    }

    DPRINT("Going to have to query the kernel-mode part\n");

    device = CreateDeviceData(device_type, device_id, device_path, FALSE);

    if ( ! device )
    {
        DPRINT("Unable to allocate device data memory\n");
        result = MMSYSERR_NOMEM;
        goto cleanup;
    }

    /* These are not needed as they're already initialized */
    ASSERT( device_id == device->id );
    ASSERT( ! device->with_critical_section );

    *(LPWORD)caps->pCaps = (WORD) 0x43;

    DPRINT("Calling kernel device\n");
    result = CallKernelDevice(device,
                              IOCTL_WDMAUD_GET_CAPABILITIES,
                              (DWORD)caps->cbSize,
                              (DWORD)caps->pCaps);

    if ( result != MMSYSERR_NOERROR )
    {
        DPRINT("IoControl failed\n");
        goto cleanup;
    }

    /* Return code will already be MMSYSERR_NOERROR by now */

    cleanup :
    {
        if ( device )
            DeleteDeviceData(device);

        return result;
    }
}


/*
    OpenDeviceViaKernel

    Internal function to rub the kernel mode part of wdmaud the right way
    so it opens a device on our behalf.
*/

MMRESULT
OpenDeviceViaKernel(
    PWDMAUD_DEVICE_INFO device,
    LPWAVEFORMATEX format
)
{
    DWORD format_struct_len = 0;

    DPRINT("Opening device via kernel\n");

    if ( format->wFormatTag == 1 )  /* FIXME */
    {
        /* Standard PCM format */
        DWORD sample_size;

        DPRINT("Standard (PCM) format\n");

        sample_size = format->nChannels * format->wBitsPerSample;

        device->state->sample_size = sample_size;

        format_struct_len = 16; /* FIXME */
    }
    else
    {
        /* Non-standard format */
        return MMSYSERR_NOTSUPPORTED; /* TODO */
    }

    return CallKernelDevice(device,
                            IOCTL_WDMAUD_OPEN_DEVICE,
                            format_struct_len,
                            (DWORD)format);
}


/* MOVEME */
LPCRITICAL_SECTION CreateCriticalSection()
{
    LPCRITICAL_SECTION cs;

    cs = AllocMem(sizeof(CRITICAL_SECTION));

    if ( ! cs )
        return NULL;

    InitializeCriticalSection(cs);

    return cs;
}


/*
    OpenDevice

    A generic "open device" function, which makes use of the above function
    once parameters have been checked. This is capable of handling both
    MIDI and wave devices, which is an improvement over the previous
    implementation (which had a lot of duplicate functionality.)
*/

MMRESULT
OpenDevice(
    CHAR device_type,
    DWORD device_id,
    LPVOID open_descriptor,
    DWORD flags,
    PWDMAUD_DEVICE_INFO* user_data
)
{
    MMRESULT result = MMSYSERR_ERROR;
    WCHAR* device_path;
    PWDMAUD_DEVICE_INFO device;
    LPWAVEFORMATEX format;

    /* As we support both types */
    LPWAVEOPENDESC wave_opendesc = (LPWAVEOPENDESC) open_descriptor;
    LPMIDIOPENDESC midi_opendesc = (LPMIDIOPENDESC) open_descriptor;

    /* FIXME: Does this just apply to wave, or MIDI also? */
    if ( device_id > 100 )
        return MMSYSERR_BADDEVICEID;

    /* Copy the appropriate dnDevNode value */
    if ( IsWaveDeviceType(device_type) )
        device_path = (WCHAR*) wave_opendesc->dnDevNode;
    else if ( IsMidiDeviceType(device_type) )
        device_path = (WCHAR*) midi_opendesc->dnDevNode;
    else
        return MMSYSERR_INVALPARAM;

    device = CreateDeviceData(device_type, device_id, device_path, TRUE);

    if ( ! device )
    {
        DPRINT1("Couldn't allocate memory for device data\n");
        result = MMSYSERR_NOMEM;
        goto cleanup;
    }

    device->flags = flags;

    if ( ( IsWaveDeviceType(device->type) ) &&
         ( device->flags & WAVE_FORMAT_QUERY ) )
    {
        result = OpenDeviceViaKernel(device, wave_opendesc->lpFormat);

        if ( result != MMSYSERR_NOERROR )
        {
            DPRINT1("Format not supported (mmsys error %d)\n", (int) result);
            result = WAVERR_BADFORMAT;
        }
        else
        {
            DPRINT("Format supported\n");
            result = MMSYSERR_NOERROR;
        }

        goto cleanup;
    }

    device->state->device_queue_guard = CreateCriticalSection();

    if ( ! device->state->device_queue_guard )
    {
        DPRINT1("Couldn't create queue cs\n");
        result = MMSYSERR_NOMEM;
        goto cleanup;
    }

    /* Set up the callbacks */
    device->client_instance = IsWaveDeviceType(device->type)
                              ? wave_opendesc->dwInstance
                              : midi_opendesc->dwInstance;

    device->client_callback = IsWaveDeviceType(device->type)
                              ? wave_opendesc->dwCallback
                              : midi_opendesc->dwCallback;

    /*
        The device state will be stopped and unpaused already, but in some
        cases this isn't the desired behaviour.
    */

    /* FIXME: What do our friends MIDI in and out need? */
    device->state->is_paused = IsWaveOutDeviceType(device->type) ? TRUE : FALSE;

    if ( IsMidiOutDeviceType(device->type) )
    {
        device->state->midi_buffer = AllocMem(2048);

        if ( ! device->state->midi_buffer )
        {
            DPRINT1("Couldn't allocate MIDI buffer\n");
            result = MMSYSERR_NOMEM;
            goto cleanup;
        }
    }

    /* Format is only for wave devices */
    format = IsWaveDeviceType(device->type) ? wave_opendesc->lpFormat : NULL;

    result = OpenDeviceViaKernel(device, format);

    if ( MM_FAILURE(result) )
    {
        DPRINT1("FAILED to open device - mm error %d\n", (int) result);
        goto cleanup;
    }

    EnterCriticalSection(device->state->device_queue_guard);
    /* TODO */
    LeaveCriticalSection(device->state->device_queue_guard);

    if ( IsWaveDeviceType(device->type) )
        wave_opendesc->hWave = (HWAVE) device;
    else
        midi_opendesc->hMidi = (HMIDI) device;

    /* Our "user data" is actually the device information */
    *user_data = device;

    if ( device->client_callback )
    {
        DWORD message = IsWaveInDeviceType(device->type)    ? WIM_OPEN :
                        IsWaveOutDeviceType(device->type)   ? WOM_OPEN :
                        IsMidiInDeviceType(device->type)    ? MIM_OPEN :
                                                              MOM_OPEN;

        DPRINT("Calling client with message %d\n", (int) message);
        NotifyClient(device, message, 0, 0);
    }

    result = MMSYSERR_NOERROR;

    cleanup :
        return result;
}















