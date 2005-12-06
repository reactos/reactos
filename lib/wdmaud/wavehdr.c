/*
 *
 * COPYRIGHT:           See COPYING in the top level directory
 * PROJECT:             ReactOS Multimedia
 * FILE:                lib/wdmaud/wavehdr.c
 * PURPOSE:             WDM Audio Support - Wave Header Manipulation
 * PROGRAMMER:          Andrew Greenwood
 * UPDATE HISTORY:
 *                      Nov 18, 2005: Created
 */

#include <windows.h>
#include "wdmaud.h"

const char WAVE_PREPARE_DATA_SIG[4] = "WPPD";

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
    return MMSYSERR_NOERROR;
}

/*
    ValidateWaveHeader Overview :

    Check that the pointer is valid to write to. If not, signal invalid
    parameter.

    Perform a bitwise AND on the flags & 0xFFFFFFE0. If there are no bits
    set, that's good. Otherwise give error and leave.

    Check that the "reserved" member contains a valid WAVEPREPAREDATA
    structure.

    Return MMSYSERR_NOERROR if all's well!
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
    PrepareWaveHeader Overview :

    Validate the parameters.

    Allocate and lock 12 bytes of global memory (fixed, zeroed) for the
    WAVEPREPAREDATA structure.

    If that fails, return immediately.

    Otherwise, allocate 20 bytes of memory on the process heap (with no
    special flags.) This is for the overlapped member of the WAVEPREPAREDATA
    structure.

    If the HeapAlloc failed, free the global memory and return.

    Create an event with all parameters false, NULL or zero. This is to be
    stored as the hEvent member of the OVERLAPPED structure.

    If the event creation failed, free all memory used and return.

    If it succeeded, set the WAVEPREPAREDATA signature to "WPPD", and set
    the reserved member of "header" to the pointer of the WAVEPREPAREDATA
    instance.

    Return MMSYSERR_NOTSUPPORTED so that winmm does further processing.
*/

MMRESULT PrepareWaveHeader(
    PWDMAUD_DEVICE_INFO device,
    PWAVEHDR header
)
{
    MMRESULT result = MMSYSERR_ERROR;
    PWDMAUD_WAVE_PREPARATION_DATA prep_data = NULL;
    HANDLE heap = NULL;

    DPRINT("PrepareWaveHeader called\n");

    result = ValidateDeviceInfoAndState(device);

    if ( result != MMSYSERR_NOERROR )
    {
        DPRINT1("Bad device info or device state\n");
        return result;
    }

    /* Make sure we were actually given a header to process */
    DPRINT("Checking that a header was supplied\n");

    if ( ! header )
    {
        DPRINT1("Bad header\n");
        return MMSYSERR_INVALPARAM;
    }

    DPRINT("Checking flags\n");

    /* I don't think Winmm would let this happen, but ya never know */
    /*
    ASSERT( ! header->dwFlags & WHDR_PREPARED );
    ASSERT( header->dwFlags & WHDR_INQUEUE );
    */

    header->lpNext = NULL;
    header->reserved = 0;

    heap = GetProcessHeap();

    if ( ! heap )
    {
        DPRINT1("Couldn't obtain the process heap (error %d)\n",
                (int)GetLastError());
        result = MMSYSERR_NOMEM;
        goto fail;
    }

    DPRINT("Allocating preparation data\n");

    prep_data =
        (PWDMAUD_WAVE_PREPARATION_DATA)
            HeapAlloc(heap,
                      HEAP_ZERO_MEMORY,
                      sizeof(WDMAUD_WAVE_PREPARATION_DATA));

    if ( ! prep_data )
    {
        DPRINT1("Couldn't lock global memory for preparation data (error %d)\n",
                (int)GetLastError());
        result = MMSYSERR_NOMEM;
        goto fail;
    }

    DPRINT("Allocating overlapped data\n");

    prep_data->overlapped = HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(OVERLAPPED));

    if ( ! prep_data->overlapped )
    {
        DPRINT1("Couldn't allocate heap memory for overlapped structure (error %d)\n",
                (int)GetLastError());
        result = MMSYSERR_NOMEM;
        goto fail;
    }

    DPRINT("Creating overlapped event\n");

    prep_data->overlapped->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if ( ! prep_data->overlapped->hEvent )
    {
        DPRINT1("Creation of overlapped event failed (error %d)\n",
                (int)GetLastError());
        result = MMSYSERR_NOMEM;
        goto fail;
    }

    /* Copy the signature over and tie the prepare structure to the wave header */
    DPRINT("Copying signature\n");
    *prep_data->signature = *WAVE_PREPARE_DATA_SIG;
    header->reserved = (DWORD) prep_data;

    result = MMSYSERR_NOTSUPPORTED;
    return result;

    fail :
    {
        if ( heap )
        {
            if ( prep_data )
            {
                if ( prep_data->overlapped )
                {
                    if ( prep_data->overlapped->hEvent )
                        CloseHandle(prep_data->overlapped->hEvent); /* ok? */
    
                    HeapFree(heap, 0, prep_data->overlapped);
                }

                HeapFree(heap, 0, prep_data);
            }
        }

        return result;
    }
}

/*
    UnprepareWaveHeader

    Winmm is intelligent enough to not call this function with a header that
    is currently queued for playing!
*/

MMRESULT UnprepareWaveHeader(PWAVEHDR header)
{
    HANDLE heap = NULL;
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

    /*
        Overview :

        Set "reserved" to NULL.

        Close the event handle (overlapped->hEvent) and set that
        structure member to NULL (not strictly necessary.)

        Obtain the heap pointer and free the heap-allocated and global
        memory allocated in PrepareWaveHeader.
    */

    /* We're about to free the preparation structure, so this needs to go */
    header->reserved = 0;

    CloseHandle(prep_data->overlapped->hEvent);

    heap = GetProcessHeap();

    if ( ! heap )
    {
        /* Not quite sure how we handle this */
        DPRINT1("Couldn't obtain the process heap (error %d)\n",
                (int)GetLastError());
        result = MMSYSERR_ERROR;
        goto cleanup;
    }

    if ( ! HeapFree(heap, 0, prep_data->overlapped) )
    {
        DPRINT1("Unable to free the OVERLAPPED memory (error %d)\n",
                (int)GetLastError());
        /* This shouldn't happen! */
        ASSERT(FALSE);
    }

    /* Overwrite the signature (structure will be invalid from now on) */
    ZeroMemory(prep_data->signature, 4);

    if ( ! HeapFree(heap, 0, prep_data) )
    {
        DPRINT1("Unable to free the preparation structure memory (error %d)\n",
                (int)GetLastError());
        /* This shouldn't happen! */
        ASSERT(FALSE);
    }

    /* Always return like this so winmm thinks we didn't do anything */

    DPRINT("Header now unprepared.\n");
    result = MMSYSERR_NOTSUPPORTED;

    cleanup :
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

    result = ValidateDeviceInfoAndState(device);

    if ( result != MMSYSERR_NOERROR )
    {
        DPRINT1("Bad device info or device state supplied\n");
        return result;
    }

    return result;
}

MMRESULT WriteWaveData(PWDMAUD_DEVICE_INFO device, PWAVEHDR header)
{
    MMRESULT result = MMSYSERR_ERROR;
    DWORD io_result = 0;
    PWDMAUD_WAVE_PREPARATION_DATA prep_data = NULL;
    PWDMAUD_DEVICE_INFO clone;

    /* For the DeviceIoControl later */
    DWORD ioctl_code;
    DWORD bytes_returned;

    DPRINT("SubmitWaveHeader called\n");

    result = ValidateWriteWaveDataParams(device, header);

    if ( result != MMSYSERR_NOERROR )
        return result;

    /*
        TODO: Check to see if we actually get called with bad flags!
    */

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

    EnterCriticalSection(device->state->queue_critical_section);

    if ( ! device->state->wave_header )
    {
        DPRINT("Device state wave_header is NULL\n");

        device->state->wave_header = header;

        /* My, what pretty symmetry you have... */

        DPRINT("Queue event == %d\n", (int) device->state->queue_event);

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

    LeaveCriticalSection(device->state->queue_critical_section);

    /* CODE FROM SUBMITWAVEHEADER */

    DPRINT("Now we actually perform header submission\n");

    clone = CloneDeviceData(device);

    if ( ! clone )
    {
        /* Safe to do this? */
        DPRINT1("Clone creation failed\n");
        return MMSYSERR_NOMEM;
    }

    if ( ! IsHeaderPrepared(header) )
    {
        DPRINT1("Unprepared header!\n");
        DeleteDeviceData(clone);
        return MMSYSERR_INVALPARAM;
    }

    /* Not sure what this is for */
    prep_data->offspring = clone;

    /* The modern version of WODM_WRITE, I guess ;) */
    clone->ioctl_param1 = sizeof(WAVEHDR);
    clone->ioctl_param2 = (DWORD) header;

    ioctl_code = clone->type == WDMAUD_WAVE_IN
                                ? 0x1d8150   /* FIXME */
                                : IOCTL_WDMAUD_SUBMIT_WAVE_HDR;

    DPRINT("Overlapped == 0x%x\n", (int) prep_data->overlapped);

    /*
        FIXME:
        For wave input to work, we may need to pass different parameters.
    */

    /* We now send the header to the driver */

    io_result =
        DeviceIoControl(GetKernelInterface(),
                        ioctl_code,
                        clone,
                        sizeof(WDMAUD_DEVICE_INFO) + (lstrlen(clone->path) * 2),
                        clone,
                        sizeof(WDMAUD_DEVICE_INFO),
                        &bytes_returned,    /* ... */
                        prep_data->overlapped);

    DPRINT("Wave header submission result : %d\n", (int) io_result);

    if ( io_result != STATUS_SUCCESS )
    {
        DPRINT1("Wave header submission FAILED! (error %d)\n", (int) io_result);

        CLEAR_WAVEHDR_FLAG(header, WHDR_INQUEUE);
        device->state->queue_critical_section = NULL;
        device->state->wave_header = NULL;

        return TranslateWinError(io_result);
    }

    /* CallKernelDevice(clone, ioctl_code, 0x20, (DWORD) header); */

    DPRINT("Creating completion thread\n");
    if ( ! CreateCompletionThread(device) )
    {
        DPRINT1("Couldn't create completion thread\n");

        CLEAR_WAVEHDR_FLAG(header, WHDR_INQUEUE);
        device->state->queue_critical_section = NULL;
        device->state->wave_header = NULL;

        return MMSYSERR_ERROR; /* Care to be more specific? */
    }


    /* TODO */

    DPRINT("applying hacks\n");

    DPRINT("Running %d paused %d\n", (int)device->state->is_running, (int)device->state->is_paused);
#if 1
    /* HACK */
    DPRINT("%d\n", (int)
        DeviceIoControl(GetKernelInterface(),
                        0x1d8104,
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

    return result;
}
