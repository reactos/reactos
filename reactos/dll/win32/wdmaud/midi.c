/*
 *
 * COPYRIGHT:           See COPYING in the top level directory
 * PROJECT:             ReactOS Multimedia
 * FILE:                lib/wdmaud/midi.c
 * PURPOSE:             WDM Audio Support - MIDI Device / Header Manipulation
 * PROGRAMMER:          Andrew Greenwood
 * UPDATE HISTORY:
 *                      Nov 29, 2005: Created
 */

#include <windows.h>
#include "wdmaud.h"

/*
    OpenMidiDevice

    OBSOLETE CODE - REFERENCE ONLY
*/

#if 0
MMRESULT
OpenMidiDevice(
    CHAR device_type,
    DWORD device_id,
    LPMIDIOPENDESC open_details,
    DWORD flags,
    DWORD user_data
)
{
    MMRESULT result = MMSYSERR_ERROR;
    WCHAR* device_path;
    PWDMAUD_DEVICE_INFO device;

    ASSERT( open_details );

    /* FIXME? Is this true for MIDI devs too then? */
    if ( device_id > 100 )
        return MMSYSERR_BADDEVICEID; /* Not sure about this */

    /* TODO: Case statement for wave/midi selection? */
    device_path = (WCHAR*) open_details->dnDevNode;
    device = CreateDeviceData(device_type, device_id, device_path, TRUE);

    if ( ! device )
    {
        DPRINT1("Couldn't create device data\n");
        result = MMSYSERR_NOMEM;
        goto cleanup;
    }

    device->type = device_type; /* not necessary */
    device->id = device_id;
    device->flags = flags;

    /* Wave devices look for format query flag here... */
    /* ... Validate Flags ? */

    device->state->device_queue_guard = AllocMem(sizeof(CRITICAL_SECTION));

    if ( ! device->state->device_queue_guard )
    {
        DPRINT1("Couldn't allocate memory for queue critical section (error %d)\n",
                (int) GetLastError());
        result = MMSYSERR_NOMEM;
        goto cleanup;
    }

    /* Initialize the critical section */
    InitializeCriticalSection(device->state->device_queue_guard);

    /* We need these so we can contact the client later */
    device->client_instance = open_details->dwInstance;
    device->client_callback = open_details->dwCallback;

    /* Reset state */
    device->state->current_midi_header = NULL;
    device->state->unknown_24 = 0;

    device->state->is_running = FALSE;
    device->state->is_paused = FALSE;

    /* MIDI ONLY */
    device->state->midi_buffer = 0;
    device->state->running_status = 0x00;

    /* For wave devices, we call the kernel NOW (but we're not handling wave) */
    /* MIDI devices are a little more complicated... Code follows... */

    /* MIDI OUT */
    if ( device->type == WDMAUD_MIDI_OUT )
    {
        device->state->midi_buffer = AllocMem(2048);

        if ( ! device->state->midi_buffer )
        {
            DPRINT1("Couldn't allocate memory for MIDI output buffer\n");
            result = MMSYSERR_NOMEM;
            goto cleanup;
        }
    }

    /* Fairly generic code */

    result = OpenDeviceViaKernel(device, NULL);

    if ( result != MMSYSERR_NOERROR )
    {
        DPRINT1("Couldn't open device (mmsys error %d)\n", (int) result);
        goto cleanup;
    }

    /* Enter the critical section while updating the device list */
    EnterCriticalSection(device->state->device_queue_guard);
    /* ... update MIDI list ... */
    LeaveCriticalSection(device->state->device_queue_guard);

    /* The MIDI device handle is actually our structure. Neat, eh? */
    open_details->hMidi = (HMIDI) device;

    /* We also need to set our "user data" for winmm */
    LPVOID* ud = (LPVOID*) user_data;   /* FIXME */
    *ud = device;

    /* MIDI specific code follows */
    if ( device->type == WDMAUD_MIDI_IN )
    {
        /* TODO: Read MIDI data until none left? */
    }

    if ( device->client_callback )
    {
        DWORD message;

        message = (device->type == WDMAUD_MIDI_IN ? MIM_OPEN : MOM_OPEN);

        DPRINT("About to call the client callback\n");

        /* Call the callback */
        NotifyClient(device, message, 0, 0);

        DPRINT("...it is done!\n");
    }

    result = MMSYSERR_NOERROR;

    cleanup :
    {
        /* TODO!!!! */
        return result;
    }
}
#endif

MMRESULT
CloseMidiDevice(PWDMAUD_DEVICE_INFO device)
{
    DPRINT("CloseMidiDevice\n");
    return MMSYSERR_NOTSUPPORTED;
}

MMRESULT
WriteMidiShort(
    PWDMAUD_DEVICE_INFO device,
    DWORD message
)
{
    DPRINT("WriteMidiShort\n");
    return MMSYSERR_NOTSUPPORTED;
}

MMRESULT
WriteMidiBuffer(PWDMAUD_DEVICE_INFO device)
{
    DPRINT("WriteMidiBuffer\n");
    /* TODO - fix params too! */
    return MMSYSERR_NOTSUPPORTED;
}

MMRESULT
ResetMidiDevice(PWDMAUD_DEVICE_INFO device)
{
    DPRINT("ResetMidiDevice\n");
    return MMSYSERR_NOTSUPPORTED;
}

/*
    TODO:
        SetVolume
        GetVolume
        SetPreferred
*/

