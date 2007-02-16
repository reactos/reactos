/*
 *
 * COPYRIGHT:           See COPYING in the top level directory
 * PROJECT:             ReactOS Multimedia
 * FILE:                lib/wdmaud/threads.c
 * PURPOSE:             WDM Audio Support - Completion Threads
 * PROGRAMMER:          Andrew Greenwood
 * UPDATE HISTORY:
 *                      Nov 18, 2005: Created
 */

#include "wdmaud.h"

DWORD WINAPI WaveCompletionThreadStart(LPVOID data)
{
    PWDMAUD_DEVICE_INFO device = (PWDMAUD_DEVICE_INFO) data;
    MMRESULT result = MMSYSERR_ERROR;
    PWDMAUD_WAVE_PREPARATION_DATA prep_data = NULL;
    HANDLE overlap_event = NULL;
    BOOL quit_loop = FALSE;

    DPRINT("WaveCompletionThread started\n");

    EnterCriticalSection(device->state->device_queue_guard);

    while ( ! quit_loop )
    {
    result = ValidateDeviceData(device, TRUE);

    if ( result != MMSYSERR_NOERROR )
    {
        DPRINT1("Invalid device data or state structure!\n");
        break;
    }

    /* TODO: REIMPLEMENT */
    /* result = ValidateDeviceStateEvents(device->state); */

    if ( result != MMSYSERR_NOERROR )
    {
        DPRINT1("Invalid device state events\n");
        break;
    }

    if ( device->state->current_wave_header )
    {
        DPRINT("No current header - running? %d\n", (int) device->state->is_running);

        if ( ! device->state->is_running )
        {
            LeaveCriticalSection(device->state->device_queue_guard);

            DPRINT("Waiting for queue_event\n");
            WaitForSingleObject(device->state->queue_event, INFINITE);

            DPRINT("field 24 == %d\n", (int) device->state->unknown_24);

            /* What is the importance of this field */

            if ( ! device->state->unknown_24 )
            {
                /* ?!?! Presumably this dequeues */
                continue;
            }

            DPRINT("We broke out the loop! Yay!\n");

            /* TODO! */

            return TRUE; /* bleh */
        }
        else
        {
            /* TODO: STOP */
            DPRINT("TODO: Stop the device\n");
        }
    }
    else
    {
        PWAVEHDR wave_header = device->state->current_wave_header;

        DPRINT("An open descriptor or wave header was found\n");

        result = ValidateWaveHeader(wave_header);

        if ( result == MMSYSERR_NOERROR )
        {
            prep_data = (PWDMAUD_WAVE_PREPARATION_DATA) wave_header->reserved;

            result = ValidateWavePreparationData(prep_data);
        }

        /* If both checks passed, the playback is complete */

        if ( result != MMSYSERR_NOERROR )
        {
            result = MMSYSERR_NOERROR;

            DPRINT("Activating the next header\n");

            /* Activate the next header */
            device->state->current_wave_header = wave_header->lpNext;

            /* Reset this just in case */
            prep_data = NULL;
            /* continue; */
        }
        else
        {
            /* Should have valid prep data... */
            overlap_event = prep_data->overlapped->hEvent;

            /* Setting this will cause the loop to exit now */
            quit_loop = TRUE;
        }
    }

    }

    /* We do this here in case there's an error - deadlock = bad! */
    LeaveCriticalSection(device->state->device_queue_guard);

    if ( result != MMSYSERR_NOERROR)
        goto cleanup;

    DPRINT("Waiting for object: %d\n", (int) overlap_event);
    WaitForSingleObject(overlap_event, INFINITE);

    cleanup :
    {
        DPRINT("Performing thread cleanup\n");

        /* Yeah, like what? */

        return result;
    }
}

DWORD WINAPI MidiCompletionThreadStart(LPVOID data)
{
    DPRINT("MidiCompletionThread started\n");
    return 0;
}

BOOL CreateCompletionThread(PWDMAUD_DEVICE_INFO device)
{
    LPTHREAD_START_ROUTINE thread_start = NULL;

    if ( IsWaveDeviceType(device->type) )
        thread_start = WaveCompletionThreadStart;
    else if ( IsMidiDeviceType(device->type) )
        thread_start = MidiCompletionThreadStart;
    else
        return FALSE;   /* What did you just give me?! */

    if ( device->state->unknown_30 != 0 )
    {
        DPRINT1("unknown_30 wasn't zero (it was %d)\n",
                (int) device->state->unknown_30);
    }

    if ( device->state->thread )
    {
        DPRINT("Thread isn't null\n");
    }
    else
    {
        DPRINT("Thread is null\n");

        if ( ( (DWORD) device->state->queue_event != 0 ) &&
             ( (DWORD) device->state->queue_event != MAGIC_42) &&
             ( (DWORD) device->state->queue_event != MAGIC_43) )
        {
            /* Not fatal... */
            DPRINT("Queue event is being overwritten!\n");
            /* return FALSE; */
        }

        device->state->queue_event = CreateEvent(NULL, FALSE, FALSE, NULL);

        if ( ! device->state->queue_event )
        {
            /* TODO - hmm original doesn't seem to care what happens */
        }

        if ( ( (DWORD) device->state->exit_thread_event != 0x00000000 ) &&
             ( (DWORD) device->state->exit_thread_event != 0x48484848 ) )
        {
            /* Not fatal... */
            DPRINT("Exit Thread event is being overwritten!\n");
            /* return FALSE; */
        }

        device->state->exit_thread_event = CreateEvent(NULL, FALSE, FALSE, NULL);

        if ( ! device->state->exit_thread_event )
        {
            /* TODO - hmm original doesn't seem to care what happens */
        }

        device->state->thread = NULL;

        /* Should this be unknown_04? aka THREAD? */

        device->state->thread = CreateThread(NULL, 0, thread_start, device, 0,
                                             &device->state->thread_id);

        if ( ! device->state->thread )
        {
            DPRINT1("Thread creation failed (error %d)\n",
                    (int) GetLastError());

            if ( device->state->queue_event )
            {
                CloseHandle(device->state->queue_event);
                device->state->queue_event = NULL;
            }

            if ( device->state->exit_thread_event )
            {
                CloseHandle(device->state->exit_thread_event);
                device->state->exit_thread_event = NULL;
            }

            return FALSE;
        }

        SetThreadPriority(device->state->thread, 0xf);

        DPRINT("Thread created! - %d\n", (int) device->state->thread);

        /* TODO: Set priority */
    }

    return TRUE; /* TODO / FIXME */
}
