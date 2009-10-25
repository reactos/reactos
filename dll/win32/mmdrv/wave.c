/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS Multimedia
 * FILE:                 dll/win32/mmdrv/wave.c
 * PURPOSE:              Multimedia User Mode Driver (Wave Audio)
 * PROGRAMMER:           Andrew Greenwood
 * UPDATE HISTORY:
 *                       Jan 30, 2004: Imported into ReactOS tree
 *                       Jan 14, 2007: Rewritten and tidied up
 */

#include <mmdrv.h>


#define MAX_WAVE_BUFFER_SIZE    65536


MMRESULT
QueueWaveBuffer(
    SessionInfo* session_info,
    LPWAVEHDR wave_header)
{
    PWAVEHDR queue_node, previous_node;
    DPRINT("Queueing wave buffer\n");

    if ( ! wave_header )
    {
        return MMSYSERR_INVALPARAM;
    }

    if ( ! wave_header->lpData )
    {
        return MMSYSERR_INVALPARAM;
    }

    /* Headers must be prepared first */
    if ( ! ( wave_header->dwFlags & WHDR_PREPARED ) )
    {
        DPRINT("I was given a header which hasn't been prepared yet!\n");
        return WAVERR_UNPREPARED;
    }

    /* ...and they must not already be in the playing queue! */
    if ( wave_header->dwFlags & WHDR_INQUEUE )
    {
        DPRINT("I was given a header for a buffer which is already playing\n");
        return WAVERR_STILLPLAYING;
    }

    /* Initialize */
    wave_header->dwBytesRecorded = 0;

    /* Clear the DONE bit, and mark the buffer as queued */
    wave_header->dwFlags &= ~WHDR_DONE;
    wave_header->dwFlags |= WHDR_INQUEUE;

    /* Save our handle in the header */
    wave_header->reserved = (DWORD) session_info;

    /* Locate the end of the queue */
    previous_node = NULL;
    queue_node = session_info->wave_queue;

    while ( queue_node )
    {
        previous_node = queue_node;
        queue_node = queue_node->lpNext;
    }

    /* Go back a step to obtain the previous node (non-NULL) */
    queue_node = previous_node;

    /* Append our buffer here, and terminate the queue */
    queue_node->lpNext = wave_header;
    wave_header->lpNext = NULL;

    /* When no buffers are playing there's no play queue so we start one */
#if 0
    if ( ! session_info->next_buffer )
    {
        session_info->buffer_position = 0;
        session_info->next_buffer = wave_header;
    }
#endif

    /* Pass to the driver - happens automatically during playback */
//    return PerformWaveIO(session_info);
    return MMSYSERR_NOERROR;
}

VOID
ReturnCompletedBuffers(SessionInfo* session_info)
{
    PWAVEHDR header = NULL;

    /* Set the current header and test to ensure it's not NULL */
    while ( ( header = session_info->wave_queue ) )
    {
        if ( header->dwFlags & WHDR_DONE )
        {
            DWORD message;

            /* Mark as done, and unqueued */
            header->dwFlags &= ~WHDR_INQUEUE;
            header->dwFlags |= WHDR_DONE;

            /* Trim it from the start of the queue */
            session_info->wave_queue = header->lpNext;

            /* Choose appropriate notification */
            message = (session_info->device_type == WaveOutDevice) ? WOM_DONE :
                                                                     WIM_DATA;

            DPRINT("Notifying client that buffer 0x%x is done\n", (int) header);

            /* Notify the client */
            NotifyClient(session_info, message, (DWORD) header, 0);
        }
    }

    /* TODO: Perform I/O as a new buffer may have arrived */
}


/*
    Each thread function/request is packed into the SessionInfo structure
    using a function ID and a parameter (in some cases.) When the function
    completes, the function code is set to an "invalid" value. This is,
    effectively, a hub for operations where sound driver I/O is concerned.
    It handles MME message codes so is a form of deferred wodMessage().
*/

DWORD
ProcessSessionThreadRequest(SessionInfo* session_info)
{
    MMRESULT result = MMSYSERR_NOERROR;

    switch ( session_info->thread.function )
    {
        case WODM_WRITE :
        {
            result = QueueWaveBuffer(session_info,
                                     (LPWAVEHDR) session_info->thread.parameter);
            break;
        }

        case WODM_RESET :
        {
            /* TODO */
            break;
        }

        case WODM_PAUSE :
        {
            /* TODO */
            break;
        }

        case WODM_RESTART :
        {
            /* TODO */
            break;
        }

        case WODM_GETPOS :
        {
            /* TODO */
            break;
        }

        case WODM_SETPITCH :
        {
            result = SetDeviceData(session_info->kernel_device_handle,
                                   IOCTL_WAVE_SET_PITCH,
                                   (PBYTE) session_info->thread.parameter,
                                   sizeof(DWORD));
            break;
        }

        case WODM_GETPITCH :
        {
            result = GetDeviceData(session_info->kernel_device_handle,
                                   IOCTL_WAVE_GET_PITCH,
                                   (PBYTE) session_info->thread.parameter,
                                   sizeof(DWORD));
            break;
        }

        case WODM_SETVOLUME :
        {
            break;
        }

        case WODM_GETVOLUME :
        {
#if 0
            result = GetDeviceData(session_info->kernel_device_handle,
                                   IOCTL_WAVE_GET_VOLUME,
                                   (PBYTE) session_info->thread.parameter,);
#endif
            break;
        }

        case WODM_SETPLAYBACKRATE :
        {
            result = SetDeviceData(session_info->kernel_device_handle,
                                   IOCTL_WAVE_SET_PLAYBACK_RATE,
                                   (PBYTE) session_info->thread.parameter,
                                   sizeof(DWORD));
            break;
        }

        case WODM_GETPLAYBACKRATE :
        {
            result = GetDeviceData(session_info->kernel_device_handle,
                                   IOCTL_WAVE_GET_PLAYBACK_RATE,
                                   (PBYTE) session_info->thread.parameter,
                                   sizeof(DWORD));
            break;
        }

        case WODM_CLOSE :
        {
            DPRINT("Thread was asked if OK to close device\n");

            if ( session_info->wave_queue != NULL )
                result = WAVERR_STILLPLAYING;
            else
                result = MMSYSERR_NOERROR;

            break;
        }

        case DRVM_TERMINATE :
        {
            DPRINT("Terminating thread...\n");
            result = MMSYSERR_NOERROR;
            break;
        }

        default :
        {
            DPRINT("INVALID FUNCTION\n");
            result = MMSYSERR_ERROR;
            break;
        }
    }

    /* We're done with the function now */

    return result;
}


/*
    The wave "session". This starts, sets itself as high priority, then waits
    for the "go" event. When this occurs, it processes the requested function,
    tidies up any buffers that have finished playing, sends new buffers to the
    sound driver, then continues handing finished buffers back to the calling
    application until it's asked to do something else.
*/

DWORD
WaveThread(LPVOID parameter)
{
    MMRESULT result = MMSYSERR_ERROR;
    SessionInfo* session_info = (SessionInfo*) parameter;
    BOOL terminate = FALSE;

    /* All your CPU time are belong to us */
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    DPRINT("Wave processing thread setting ready state\n");

    SetEvent(session_info->thread.ready_event);

    while ( ! terminate )
    {
        /* Wait for GO event, or IO completion notification */
        while ( WaitForSingleObjectEx(session_info->thread.go_event,
                                      INFINITE,
                                      TRUE) == WAIT_IO_COMPLETION )
        {
            /* A buffer has been finished with - pass back to the client */
            ReturnCompletedBuffers(session_info);
        }

        DPRINT("Wave processing thread woken up\n");

        /* Set the terminate flag if that's what the caller wants */
        terminate = (session_info->thread.function == DRVM_TERMINATE);

        /* Process the request */
        DPRINT("Processing thread request\n");
        result = ProcessSessionThreadRequest(session_info);

        /* Store the result code */
        session_info->thread.result = result;

        /* Submit new buffers and continue existing ones */
        DPRINT("Performing wave I/O\n");
        PerformWaveIO(session_info);

        /* Now we're ready for more action */
        DPRINT("Wave processing thread sleeping\n");
        SetEvent(session_info->thread.ready_event);
    }

    return 0;
}


/*
    Convenience function for calculating the size of the WAVEFORMATEX struct.
*/

DWORD
GetWaveFormatExSize(PWAVEFORMATEX format)
{
    if ( format->wFormatTag == WAVE_FORMAT_PCM )
        return sizeof(PCMWAVEFORMAT);
    else
        return sizeof(WAVEFORMATEX) + format->cbSize;
}


/*
    Query if the driver/device is capable of handling a format. This is called
    if the device is a wave device, and the QUERYFORMAT flag is set.
*/

DWORD
QueryWaveFormat(
    DeviceType device_type,
    PVOID lpFormat)
{
    /* TODO */
    return WAVERR_BADFORMAT;
}


/*
    Set the format to be used.
*/

BOOL
SetWaveFormat(
    HANDLE device_handle,
    PWAVEFORMATEX format)
{
    DWORD bytes_returned;
    DWORD size;

    size = GetWaveFormatExSize(format);

    DPRINT("SetWaveFormat\n");

    return DeviceIoControl(device_handle,
                           IOCTL_WAVE_SET_FORMAT,
                           (PVOID) format,
                           size,
                           NULL,
                           0,
                           &bytes_returned,
                           NULL);
}


DWORD
WriteWaveBuffer(
    DWORD private_handle,
    PWAVEHDR wave_header,
    DWORD wave_header_size)
{
    SessionInfo* session_info = (SessionInfo*) private_handle;
    ASSERT(session_info);

    /* Let the processing thread know that it has work to do */
    return CallSessionThread(session_info, WODM_WRITE, wave_header);
}
