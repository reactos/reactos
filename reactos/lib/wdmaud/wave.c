/*
 *
 * COPYRIGHT:           See COPYING in the top level directory
 * PROJECT:             ReactOS Multimedia
 * FILE:                lib/wdmaud/wave.c
 * PURPOSE:             WDM Audio Support - Wave Device / Header Manipulation
 * PROGRAMMER:          Andrew Greenwood
 * UPDATE HISTORY:
 *                      Nov 18, 2005: Created
 */

#include <windows.h>
#include "wdmaud.h"

const char WAVE_PREPARE_DATA_SIG[4] = "WPPD";

/*
    OBSOLETE CODE - FOR REFERENCE ONLY
*/
#if 0
MMRESULT OpenWaveDevice(
    CHAR device_type,
    DWORD device_id,
    LPWAVEOPENDESC open_details,
    DWORD flags,
    DWORD user_data
)
{
    MMRESULT result = MMSYSERR_ERROR;
    WCHAR* device_path;
    PWDMAUD_DEVICE_INFO device;

    ASSERT( open_details );
    ASSERT( open_details->lpFormat );

    if ( device_id > 100 )
        return MMSYSERR_BADDEVICEID; /* Not sure about this */

    device_path = (WCHAR*) open_details->dnDevNode;
    device = CreateDeviceData(device_type, device_id, device_path, TRUE);

    if ( ! device )
    {
        DPRINT1("Couldn't create device data\n");
        result = MMSYSERR_NOMEM;
        goto cleanup;
    }

    device->type = device_type;
    device->id = device_id;
    device->flags = flags;

    /* We don't deal with this here */
    if ( flags & WAVE_FORMAT_QUERY )
    {
        result = QueryWaveFormatSupport(device, open_details);
        DeleteDeviceData( device );
        return result;
    }


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
    device->state->current_wave_header = NULL;
    device->state->unknown_24 = 0;

    device->state->is_running = FALSE;
    device->state->is_paused =
        device->type == WDMAUD_WAVE_IN ? TRUE : FALSE;

    DPRINT("Opening the device\n");

    result = OpenDeviceViaKernel(device, open_details->lpFormat);

    if ( result != MMSYSERR_NOERROR )
    {
        DPRINT1("Couldn't open! mmsys error %d\n", (int) result);
        /* TODO: WAVERR_BADFORMAT translation ? */
        goto cleanup;
    }

    /* Enter the critical section while updating the device list */
    EnterCriticalSection(device->state->device_queue_guard);
    /* ... */
    LeaveCriticalSection(device->state->device_queue_guard);

    /* The wave device handle is actually our structure. Neat, eh? */
    open_details->hWave = (HWAVE) device;

    /* We also need to set our "user data" for winmm */
    LPVOID* ud = (LPVOID*) user_data;   /* FIXME */
    *ud = device;

    if ( device->client_callback )
    {
        DWORD message;

        message = (device->type == WDMAUD_WAVE_IN ? WIM_OPEN : WOM_OPEN);

        DPRINT("About to call the client callback\n");

        /* Call the callback */
        NotifyClient(device, message, 0, 0);

        DPRINT("...it is done!\n");
    }

    result = MMSYSERR_NOERROR;

    cleanup :
    {
        if ( result != MMSYSERR_NOERROR )
            if ( device )
                DeleteDeviceData(device);

        return result;
    }
}
#endif

MMRESULT CloseWaveDevice(
    PWDMAUD_DEVICE_INFO device
)
{
    MMRESULT result = MMSYSERR_ERROR;

    result = ValidateDeviceData(device, TRUE);

    if ( result != MMSYSERR_NOERROR )
    {
        DPRINT1("Device data invalid\n");
        return MMSYSERR_INVALPARAM;
    }

    if ( ! IsWaveDeviceType(device->type) )
    {
        DPRINT1("Invalid device type (expected a WAVE device)\n");
        return MMSYSERR_INVALPARAM;
    }

    /* TODO: Perform actual close */
    if ( device->state->current_wave_header )
    {
        DPRINT1("Can't close! Device is still playing\n");
        return WAVERR_STILLPLAYING;
    }

    /* TODO */
    /* DestroyCompletionThread(device); - check result */

    result = CallKernelDevice(device, IOCTL_WDMAUD_CLOSE_DEVICE, 0, 0);

    if ( result != MMSYSERR_NOERROR )
    {
        DPRINT1("Close failed! mmsyserr %d\n", (int) result);
        return result;  /* TODO: convert? */
    }

    if ( device->client_callback )
    {
        DWORD message;

        message = (device->type == WDMAUD_WAVE_IN ? WIM_CLOSE : WOM_CLOSE);

        DPRINT("About to call the client callback\n");

        /* Call the callback */
        NotifyClient(device, message, 0, 0);

        DPRINT("...it is done!\n");
    }

    /*
        TODO:
        Enter critical section
        Loop through device list until we reach the end or until we find a
        pointer matching "device".
        Leave critical section
        Delete critical section
        ...
    */

    DeleteDeviceData(device);

    return MMSYSERR_NOERROR;
}


/*
    ValidateWaveHeaderPreparation Overview :

    First, check to see if we can write to the buffer given to us. Fail
    if we can't (invalid parameter?)

    Make sure the signature matches "WPPD". Fail if not (invalid param?)

    Finally, validate the "overlapped" member, by checking to see if
    that buffer is writable, and ensuring hEvent is non-NULL.
*/

MMRESULT ValidateWavePreparationData(PWDMAUD_WAVE_PREPARATION_DATA prep_data)
{
    /* UNIMPLEMENTED */
    return MMSYSERR_NOERROR;
}

/*
    ValidateWaveHeader

    Checks that the header memory can be written to, that the flags are
    valid (using the mask 0xFFFFFFE0), and that the wave preparation data
    is valid.

    Returns MMSYSERR_NOERROR if all's well, or MMSYSERR_INVALPARAM if not.
*/

MMRESULT ValidateWaveHeader(PWAVEHDR header)
{
    DWORD flag_check;

    if ( IsBadWritePtr(header, sizeof(WAVEHDR)) )
    {
        DPRINT1("Bad write pointer\n");
        return MMSYSERR_INVALPARAM;
    }

    flag_check = header->dwFlags & 0xffffffe0;  /* FIXME: Use flag names */

    if ( flag_check )
    {
        DPRINT1("Unknown flags present\n");
        return MMSYSERR_INVALPARAM;
    }

    return ValidateWavePreparationData(
                            (PWDMAUD_WAVE_PREPARATION_DATA) header->reserved);
}


/*
    PrepareWaveHeader

    Checks the parameters are sane, allocates memory for a WAVEPREPAREDATA
    structure and also memory for an OVERLAPPED structure.

    After this, an un-named event is created (as hEvent of the OVERLAPPED)
    structure, and the WAVEPREPAREDATA structure has its signature set
    accordingly.

    Returns MMSYSERR_NOTSUPPORTED so that winmm does further processing.
*/

MMRESULT PrepareWaveHeader(
    PWDMAUD_DEVICE_INFO device,
    PWAVEHDR header
)
{
    MMRESULT result = MMSYSERR_ERROR;
    PWDMAUD_WAVE_PREPARATION_DATA prep_data = NULL;

    DPRINT("PrepareWaveHeader called\n");

    /* Check the device data is valid */
    result = ValidateDeviceData(device, TRUE);

    if ( result != MMSYSERR_NOERROR )
    {
        DPRINT1("Bad device info or device state\n");
        return result;
    }

    /* Make sure we were actually given a header to process */
    if ( ! header )
    {
        DPRINT1("Bad header\n");
        return MMSYSERR_INVALPARAM;
    }

    /* NOTE: At this point, what happens if not prepared or already queued? */

    header->lpNext = NULL;
    header->reserved = 0;

    /* Allocate memory for the wave preparation data */
    prep_data =
        (PWDMAUD_WAVE_PREPARATION_DATA)
            AllocMem(sizeof(WDMAUD_WAVE_PREPARATION_DATA));

    if ( ! prep_data )
    {
        DPRINT1("Couldn't lock global memory for preparation data (error %d)\n",
                (int)GetLastError());
        result = MMSYSERR_NOMEM;
        goto fail;
    }

    /* Create an event */

    prep_data->overlapped = AllocMem(sizeof(OVERLAPPED));

    if ( ! prep_data->overlapped )
    {
        DPRINT1("Couldn't allocate memory for overlapped structure (error %d)\n",
                (int)GetLastError());
        result = MMSYSERR_NOMEM;
        goto fail;
    }

    prep_data->overlapped->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if ( ! prep_data->overlapped->hEvent )
    {
        DPRINT1("Creation of overlapped event failed (error %d)\n",
                (int)GetLastError());
        result = MMSYSERR_NOMEM;
        goto fail;
    }

    /* Copy the signature over and tie the prepare structure to the wave header */
    memcpy(prep_data->signature, WAVE_PREPARE_DATA_SIG, 4);
    header->reserved = (DWORD) prep_data;

    /* We return this so WINMM can do further processing */
    result = MMSYSERR_NOTSUPPORTED;
    return result;

    fail :
    {
        if ( prep_data )
        {
            if ( prep_data->overlapped )
            {
                if ( prep_data->overlapped->hEvent )
                    CloseHandle(prep_data->overlapped->hEvent); /* ok? */

                FreeMem(prep_data->overlapped);
            }

            FreeMem(prep_data);
        }

        return result;
    }
}

/*
    UnprepareWaveHeader

    Cleans up after a header has been used, by killing the event we set up
    above, and freeing the preparation data.

    Winmm is intelligent enough to not call this function with a header that
    is currently queued for playing!
*/

MMRESULT UnprepareWaveHeader(PWAVEHDR header)
{
    MMRESULT result = MMSYSERR_ERROR;
    PWDMAUD_WAVE_PREPARATION_DATA prep_data = NULL;

    DPRINT("UnprepareHeader called\n");

    /* Make sure we were actually given a header to process */

    if ( ! header )
    {
        DPRINT1("Bad header supplied\n");
        return MMSYSERR_INVALPARAM;
    }

    prep_data = (PWDMAUD_WAVE_PREPARATION_DATA) header->reserved;
    result = ValidateWavePreparationData(prep_data);

    if ( result != MMSYSERR_NOERROR )
    {
        DPRINT1("Bad wave header preparation structure pointer\n");
        return result;
    }

    /* We're about to free the preparation structure, so this needs to go */
    header->reserved = 0;

    /* Kill the event */
    CloseHandle(prep_data->overlapped->hEvent);
    FreeMem(prep_data->overlapped);

    /* Overwrite the signature (structure will be invalid from now on) */
    ZeroMemory(prep_data->signature, 4);
    FreeMem(prep_data);

    /* Always return like this so winmm thinks we didn't do anything */

    DPRINT("Header now unprepared.\n");
    result = MMSYSERR_NOTSUPPORTED;

    return result;
}

/* Not sure about this */
MMRESULT CompleteWaveHeader(PWAVEHDR header)
{
    return MMSYSERR_NOTSUPPORTED;
}


/*
    SubmitWaveHeader Overview :

    This may span 2 functions (this one and and another "SubmitHeader")

    First, validate the device info, then the state.

    Validate the header, followed by the reserved member.

    Fail if INQUEUE flag is set in header, or if PREPARED is not set in
    header.

    AND the flags with PREPARED, BEGINLOOP, ENDLOOP and INQUEUE. OR the
    result with INQUEUE.

    Enter the csQueue critical section.

    Check if the device state's "open descriptor" member is NULL or not.
    If we're adding an extra buffer, it will already have been allocated.

    If the open descriptor is NULL:

        If it's NULL, set "opendesc" to point to the wave header (?!)

        If the state structure's "hevtQueue" member isn't NULL, compare it's
        value to 0x43434343h and 0x42424242h. If it's not NULL or one of
        those values, set the event.

    If the open descriptor is NOT NULL:

        Check the header's lpNext member. If it's not NULL, check that
        structure's lpNext member, and so on, until a NULL entry is
        found.

        Set the NULL entry to point to our header.

    Leave the csQueue critical section.

    ** SUBMIT THE HEADER ** TODO **

    If submission failed:

        AND the flags with 0xFFFFFFEFh. If csQueue is set in the target
        (the header who's lpNext was NULL), set it to NULL. Set the open
        descriptor of state to NULL, too. And fail, of course.

    If the device state is PAUSED or RUNNING, we must fail.

    Otherwise, reset the device and set it as RUNNING. This may be done by
    our caller (wodMessage, etc.)
    0x1d8104 is used for wave in
    0x1d8148 is used for wave out?

    SetDeviceState should now be called with the above IOCTL code and the
    device info structure.
*/

/*
    ValidateWriteWaveDataParams

    This is just a helper function that shrinks WriteWaveData a little
    bit.
*/

static MMRESULT ValidateWriteWaveDataParams(
    PWDMAUD_DEVICE_INFO device,
    PWAVEHDR header
)
{
    MMRESULT result;

    result = ValidateWaveHeader(header);

    if ( result != MMSYSERR_NOERROR )
    {
        DPRINT1("Bad wave header supplied\n");
        return result;
    }

    /*
        We don't want to queue something already queued, and we don't want
        to queue something that hasn't been prepared. Who knows what garbage
        might be sent to us?!
    */

    if ( header->dwFlags & WHDR_INQUEUE )
    {
        DPRINT1("This header is already queued!\n");
        return MMSYSERR_INVALFLAG;
    }

    if ( ! header->dwFlags & WHDR_PREPARED )
    {
        DPRINT1("This header isn't prepared!\n");
        return WAVERR_UNPREPARED;
    }

    result = ValidateDeviceData(device, TRUE);

    if ( result != MMSYSERR_NOERROR )
    {
        DPRINT1("Bad device info or device state supplied\n");
        return result;
    }

    return result;
}

/*
    WriteWaveData

    This is the exciting (?!) bit where playback actually begins. Various
    validation takes place, before the header is queued for playback. Playback
    can then begin. This entails telling the kernel-mode device about the
    header, then telling the device to start playback.

    It all seems pretty straightforward, but it's not all that easy...
*/

MMRESULT WriteWaveData(PWDMAUD_DEVICE_INFO device, PWAVEHDR header)
{
    MMRESULT result = MMSYSERR_ERROR;
    DWORD io_result = 0;
    PWDMAUD_WAVE_PREPARATION_DATA prep_data = NULL;
    /* PWDMAUD_DEVICE_INFO clone; */

    /* For the DeviceIoControl later */
    DWORD ioctl_code;
    DWORD bytes_returned;

    DPRINT("WriteWaveHeader called\n");

    result = ValidateWriteWaveDataParams(device, header);

    if ( result != MMSYSERR_NOERROR )
        return result;

    /* Check to see if we actually get called with bad flags! */
    if ( ! IS_WAVEHDR_FLAG_SET(header, WHDR_PREPARED) )
    {
        DPRINT1("Not prepared!\n");
        return WAVERR_UNPREPARED;
    }

    /* Retrieve our precious data from the reserved member */
    prep_data = (PWDMAUD_WAVE_PREPARATION_DATA) header->reserved;

    result = ValidateWavePreparationData(prep_data);

    if ( result != MMSYSERR_NOERROR )
    {
        DPRINT1("Bad wave preparation structure supplied\n");
        return result;
    }

    DPRINT("Flags == 0x%x\n", (int) header->dwFlags);

    /* Mask the "done" flag */
    CLEAR_WAVEHDR_FLAG(header, WHDR_DONE);
    /* header->dwFlags &= ~WHDR_DONE; */
    /* ...and set the queue flag! */
    SET_WAVEHDR_FLAG(header, WHDR_INQUEUE);
    /* header->dwFlags |= WHDR_INQUEUE; */

    DPRINT("Flags == 0x%x\n", (int) header->dwFlags);

    EnterCriticalSection(device->state->device_queue_guard);

    if ( ! device->state->current_wave_header )
    {
        DPRINT("Device state wave_header is NULL\n");

        device->state->current_wave_header = header;

        /* My, what pretty symmetry you have... */

        DPRINT("Queue event == 0x%x\n", (int) device->state->queue_event);

        if ( ( (DWORD) device->state->queue_event != 0 ) &&
             ( (DWORD) device->state->queue_event != MAGIC_42 ) &&
             ( (DWORD) device->state->queue_event != MAGIC_43 ) )
        {
            DPRINT("Setting queue event\n");
            SetEvent(device->state->queue_event);
        }
    }
    else
    {
        DPRINT("Device state open_descriptor is NOT NULL\n");
        /* TODO */
        ASSERT(FALSE);
    }

    LeaveCriticalSection(device->state->device_queue_guard);

    /* Now we send the header to the kernel device */

    if ( ! IsHeaderPrepared(header) )
    {
        DPRINT1("Unprepared header!\n");
        result = MMSYSERR_INVALPARAM;
        goto cleanup;
    }

    /*
        Not sure what this is for. I *think* it's used for tracking which
        device a preparation belongs to.
    */
    prep_data->offspring = device;

    /* The modern version of WODM_WRITE, I guess ;) */
    device->ioctl_param1 = sizeof(WAVEHDR);
    device->ioctl_param2 = (DWORD) header;

    ioctl_code = device->type == WDMAUD_WAVE_IN
                                ? IOCTL_WDMAUD_SUBMIT_WAVE_IN_HDR   /* FIXME */
                                : IOCTL_WDMAUD_SUBMIT_WAVE_OUT_HDR;

    /*
        FIXME:
        For wave input to work, we may need to pass different parameters.
    */

    /* We now send the header to the driver */

    io_result =
        DeviceIoControl(GetKernelInterface(),
                        ioctl_code,
                        device,
                        sizeof(WDMAUD_DEVICE_INFO) + (lstrlen(device->path) * 2),
                        device,
                        sizeof(WDMAUD_DEVICE_INFO),
                        &bytes_returned,    /* ... */
                        prep_data->overlapped);

    DPRINT("Wave header submission result : %d\n", (int) io_result);

    if ( io_result != STATUS_SUCCESS )
    {
        DPRINT1("Wave header submission FAILED! (error %d)\n", (int) io_result);

        CLEAR_WAVEHDR_FLAG(header, WHDR_INQUEUE);
        device->state->device_queue_guard = NULL;
        device->state->current_wave_header = NULL;

        return TranslateWinError(io_result);
    }

    /* CallKernelDevice(clone, ioctl_code, 0x20, (DWORD) header); */

    if ( ! CreateCompletionThread(device) )
    {
        DPRINT1("Couldn't create completion thread\n");

        CLEAR_WAVEHDR_FLAG(header, WHDR_INQUEUE);
        device->state->device_queue_guard = NULL;
        device->state->current_wave_header = NULL;

        return MMSYSERR_ERROR; /* Care to be more specific? */
    }


    /* ***** FIXME ****** THIS IS NASTY HACKERY ****** */

    DPRINT("applying hacks\n");

    DPRINT("Running %d paused %d\n", (int)device->state->is_running, (int)device->state->is_paused);
#if 1
    /* HACK */
    DPRINT("%d\n", (int)
        DeviceIoControl(GetKernelInterface(),
                        IOCTL_WDMAUD_WAVE_OUT_START,
                        device,
                        sizeof(WDMAUD_DEVICE_INFO) + (lstrlen(device->path) * 2),
                        device,
                        sizeof(WDMAUD_DEVICE_INFO),
                        &bytes_returned,    /* ... */
                        prep_data->overlapped) );

    DPRINT("Running %d paused %d\n", (int)device->state->is_running, (int)device->state->is_paused);

#if 0 /* on error */
    DPRINT("%d\n", (int)
        DeviceIoControl(GetKernelInterface(),
                        0x1d8148,
                        device,
                        sizeof(WDMAUD_DEVICE_INFO) + (lstrlen(device->path) * 2),
                        device,
                        sizeof(WDMAUD_DEVICE_INFO),
                        &bytes_returned,    /* ... */
                        prep_data->overlapped) );
#endif
#endif

    result = MMSYSERR_NOERROR;

    cleanup :
    {
        /* TODO */
        return result;
    }
}
