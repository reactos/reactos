/*
 *
 * COPYRIGHT:           See COPYING in the top level directory
 * PROJECT:             ReactOS Multimedia
 * FILE:                lib/wdmaud/wavehdr.c
 * PURPOSE:             WDM Audio Support - Device Control (Play/Stop etc.)
 * PROGRAMMER:          Andrew Greenwood
 * UPDATE HISTORY:
 *                      Nov 23, 2005: Created
 */

#include <windows.h>
#include "wdmaud.h"

/*
    StartDevice

    Creates a completion thread for a device, sets the "is_running" member
    of the device state to "true", and tells the kernel device to start
    processing audio/MIDI data.

    Wave devices always start paused.
*/

MMRESULT StartDevice(PWDMAUD_DEVICE_INFO device)
{
    MMRESULT result;
    DWORD ioctl_code;

    result = ValidateDeviceInfoAndState(device);

    if ( result != MMSYSERR_NOERROR )
    {
        DPRINT1("Device info/state not valid\n");
        return result;
    }

    ioctl_code =
        IsWaveInDeviceType(device->type)  ? IOCTL_WDMAUD_WAVE_IN_START :
        IsWaveOutDeviceType(device->type) ? IOCTL_WDMAUD_WAVE_OUT_START :
        IsMidiInDeviceType(device->type)  ? IOCTL_WDMAUD_MIDI_IN_START :
        0x0000;

    ASSERT( ioctl_code );

    result = CreateCompletionThread(device);

    if ( MM_FAILURE( result ) )
    {
        DPRINT1("Failed to create completion thread\n");
        return result;
    }

    device->state->is_running = TRUE;

    result = CallKernelDevice(device, ioctl_code, 0, 0);

    if ( MM_FAILURE( result ) )
    {
        DPRINT1("Audio could not be started\n");
        return result;
    }

    if ( ! IsWaveDeviceType(device->type) )
        device->state->is_paused = FALSE;

    return result;
}

MMRESULT StopDevice(PWDMAUD_DEVICE_INFO device)
{
    MMRESULT result;
    DWORD ioctl_code;

    result = ValidateDeviceInfoAndState(device);

    if ( result != MMSYSERR_NOERROR )
    {
        DPRINT1("Device info/state not valid\n");
        return result;
    }

    ioctl_code =
        IsWaveInDeviceType(device->type)  ? IOCTL_WDMAUD_WAVE_IN_STOP :
        IsWaveOutDeviceType(device->type) ? IOCTL_WDMAUD_WAVE_OUT_STOP :
        IsMidiInDeviceType(device->type)  ? IOCTL_WDMAUD_MIDI_IN_STOP :
        0x0000;

    ASSERT( ioctl_code );

    if ( IsMidiInDeviceType(device->type) )
    {
        EnterCriticalSection(device->state->device_queue_guard);

        if ( ! device->state->is_running )
        {
            /* TODO: Free the MIDI data queue */
        }
        else
        {
            device->state->is_running = FALSE;
        }

        LeaveCriticalSection(device->state->device_queue_guard);
    }
    else /* wave device */
    {
        device->state->is_paused = TRUE;
    }

    result = CallKernelDevice(device, ioctl_code, 0, 0);

    if ( MM_FAILURE( result ) )
    {
        DPRINT1("Audio could not be stopped\n");
        return result;
    }

    if ( IsWaveDeviceType(device-type) )
    {
        device->state->is_paused = TRUE;
    }
    else    /* MIDI Device */
    {
        /* TODO: Destroy completion thread etc. */
    }

    return result;
}

MMRESULT ResetDevice(PWDMAUD_DEVICE_INFO device)
{
    MMRESULT result;
    DWORD ioctl_code;

    result = ValidateDeviceInfoAndState(device);

    if ( result != MMSYSERR_NOERROR )
    {
        DPRINT1("Device info/state not valid\n");
        return result;
    }

    ioctl_code =
        IsWaveInDeviceType(device->type)  ? IOCTL_WDMAUD_WAVE_IN_RESET :
        IsWaveOutDeviceType(device->type) ? IOCTL_WDMAUD_WAVE_OUT_RESET :
        IsMidiInDeviceType(device->type)  ? IOCTL_WDMAUD_MIDI_IN_RESET :
        0x0000;

    ASSERT( ioctl_code );

    if ( IsMidiInDeviceType(device->type) )
    {
        EnterCriticalSection(device->state->device_queue_guard);

        if ( ! device->state->is_running )
        {
            /* TODO: Free the MIDI data queue */
        }
        else
        {
            device->state->is_running = FALSE;
        }

        LeaveCriticalSection(device->state->device_queue_guard);
    }

    result = CallKernelDevice(device, ioctl_code, 0, 0);

    if ( MM_FAILURE( result ) )
    {
        DPRINT1("Audio could not be reset\n");
        return result;
    }

    if ( IsWaveDeviceType(device->type) )
    {
        if ( IsWaveInDeviceType(device->type) )
            device->state->is_paused = TRUE;
        else if ( IsWaveOutDeviceType(device->type) )
            device->state->is_paused = FALSE;

        /* TODO: Destroy completion thread + check ret val */
    }
    else /* MIDI input device */
    {
        /* TODO: Destroy completion thread + check ret val */
        /* TODO - more stuff */
    }

    return result;
}

MMRESULT StopDeviceLooping(PWDMAUD_DEVICE_INFO device)
{
}
