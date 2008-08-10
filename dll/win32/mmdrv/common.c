/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS Multimedia
 * FILE:                 dll/win32/mmdrv/common.c
 * PURPOSE:              Multimedia User Mode Driver (Common functions)
 * PROGRAMMER:           Andrew Greenwood
 * UPDATE HISTORY:
 *                       Jan 14, 2007: Created
 */

#include <mmdrv.h>

/*
    Translates errors to MMRESULT codes.
*/

MMRESULT
ErrorToMmResult(UINT error_code)
{
    switch ( error_code )
    {
        case NO_ERROR :
        case ERROR_IO_PENDING :
            return MMSYSERR_NOERROR;

        case ERROR_BUSY :
            return MMSYSERR_ALLOCATED;

        case ERROR_NOT_SUPPORTED :
        case ERROR_INVALID_FUNCTION :
            return MMSYSERR_NOTSUPPORTED;

        case ERROR_NOT_ENOUGH_MEMORY :
            return MMSYSERR_NOMEM;

        case ERROR_ACCESS_DENIED :
            return MMSYSERR_BADDEVICEID;

        case ERROR_INSUFFICIENT_BUFFER :
            return MMSYSERR_INVALPARAM;
    };

    /* If all else fails, it's just a plain old error */

    return MMSYSERR_ERROR;
}


/*
    Obtains a device count for a specific kind of device.
*/

DWORD
GetDeviceCount(DeviceType device_type)
{
    UINT index = 0;
    HANDLE handle;

    /* Cycle through devices until an error occurs */

    while ( OpenKernelDevice(device_type, index, GENERIC_READ, &handle) == MMSYSERR_NOERROR )
    {
        CloseHandle(handle);
        index ++;
    }

    DPRINT("Found %d devices of type %d\n", index, device_type);

    return index;
}


/*
    Obtains device capabilities. This could either be done as individual
    functions for wave, MIDI and aux, or like this. I chose this method as
    it centralizes everything.
*/

DWORD
GetDeviceCapabilities(
    DeviceType device_type,
    DWORD device_id,
    PVOID capabilities,
    DWORD capabilities_size)
{
    MMRESULT result;
    DWORD ioctl;
    HANDLE handle;
    DWORD bytes_returned;
    BOOL device_io_result;

    ASSERT(capabilities);

    /* Choose the right IOCTL for the job */

    if ( IsWaveDevice(device_type) )
        ioctl = IOCTL_WAVE_GET_CAPABILITIES;
    else if ( IsMidiDevice(device_type) )
        ioctl = IOCTL_MIDI_GET_CAPABILITIES;
    else if ( IsAuxDevice(device_type) )
        return MMSYSERR_NOTSUPPORTED; /* TODO */
    else
        return MMSYSERR_NOTSUPPORTED;

    result = OpenKernelDevice(device_type,
                              device_id,
                              GENERIC_READ,
                              &handle);

    if ( result != MMSYSERR_NOERROR )
    {
        DPRINT("Failed to open kernel device\n");
        return result;
    }

    device_io_result = DeviceIoControl(handle,
                                       ioctl,
                                       NULL,
                                       0,
                                       (LPVOID) capabilities,
                                       capabilities_size,
                                       &bytes_returned,
                                       NULL);

    /* Translate result */

    if ( device_io_result )
        result = MMSYSERR_NOERROR;
    else
        result = ErrorToMmResult(GetLastError());

    /* Clean up and return */

    CloseKernelDevice(handle);

    return result;
}


/*
    A wrapper around OpenKernelDevice that creates a session,
    opens the kernel device, initializes session data and notifies
    the client (application) that the device has been opened. Again,
    this supports any device type and the only real difference is
    the open descriptor.
*/

DWORD
OpenDevice(
    DeviceType device_type,
    DWORD device_id,
    PVOID open_descriptor,
    DWORD flags,
    DWORD private_handle)
{
    SessionInfo* session_info;
    MMRESULT result;
    DWORD message;

    /* This will automatically check for duplicate sessions */
    result = CreateSession(device_type, device_id, &session_info);

    if ( result != MMSYSERR_NOERROR )
    {
        DPRINT("Couldn't allocate session info\n");
        return result;
    }

    result = OpenKernelDevice(device_type,
                              device_id,
                              GENERIC_READ,
                              &session_info->kernel_device_handle);

    if ( result != MMSYSERR_NOERROR )
    {
        DPRINT("Failed to open kernel device\n");
        DestroySession(session_info);
        return result;
    }

    /* Set common session data */

    session_info->flags = flags;

    /* Set wave/MIDI specific data */

    if ( IsWaveDevice(device_type) )
    {
        LPWAVEOPENDESC wave_open_desc = (LPWAVEOPENDESC) open_descriptor;
        session_info->callback = wave_open_desc->dwCallback;
        session_info->mme_wave_handle = wave_open_desc->hWave;
        session_info->app_user_data = wave_open_desc->dwInstance;
    }
    else
    {
        DPRINT("Only wave devices are supported at present!\n");
        DestroySession(session_info);
        return MMSYSERR_NOTSUPPORTED;
    }

    /* Start the processing thread */

    result = StartSessionThread(session_info);

    if ( result != MMSYSERR_NOERROR )
    {
        DestroySession(session_info);
        return result;
    }

    /* Store the session info */

    *((SessionInfo**)private_handle) = session_info;

    /* Send the right message */

    message = (device_type == WaveOutDevice) ? WOM_OPEN :
              (device_type == WaveInDevice) ? WIM_OPEN :
              (device_type == MidiOutDevice) ? MOM_OPEN :
              (device_type == MidiInDevice) ? MIM_OPEN : 0xFFFFFFFF;

    NotifyClient(session_info, message, 0, 0);

    return MMSYSERR_NOERROR;
}


/*
    Attempts to close a device. This can fail if playback/recording has
    not been stopped. We need to make sure it's safe to destroy the
    session as well (mainly by killing the session thread.)
*/

DWORD
CloseDevice(
    DWORD private_handle)
{
    MMRESULT result;
    SessionInfo* session_info = (SessionInfo*) private_handle;
    /* TODO: Maybe this is best off inside the playback thread? */

    ASSERT(session_info);

    result = CallSessionThread(session_info, WODM_CLOSE, 0);

    if ( result == MMSYSERR_NOERROR )
    {
        /* TODO: Wait for it to be safe to terminate */

        CloseKernelDevice(session_info->kernel_device_handle);

        DestroySession(session_info);
    }

    return result;
}

