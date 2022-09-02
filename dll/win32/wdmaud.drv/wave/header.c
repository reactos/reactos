/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/sound/mmebuddy/wave/header.c
 *
 * PURPOSE:     Wave header preparation and submission routines
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include "wdmaud.h"

#define YDEBUG
#include <debug.h>


/*
    The following routines are basically handlers for:
    - WODM_PREPARE
    - WODM_UNPREPARE
    - WODM_WRITE

    All of these calls are ultimately dealt with in the context of the
    appropriate sound thread, so the implementation should expect itself to
    be running in this other thread when any of these operations take place.
*/

MMRESULT
PrepareWaveHeader(
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWAVEHDR Header)
{
    //PWAVEHDR_EXTENSION HeaderExtension;

    VALIDATE_MMSYS_PARAMETER( DeviceInfo );
    VALIDATE_MMSYS_PARAMETER( Header );

    DPRINT("Preparing wave header\n");
#if 0
    Header->lpNext = NULL;
    Header->reserved = 0;

    /* Allocate header extension */
    HeaderExtension = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WAVEHDR_EXTENSION));
    if (!HeaderExtension)
    {
        /* No memory */
        return MMSYSERR_NOMEM;
    }

    /* Allocate OVERLAPPED */
    HeaderExtension->Overlapped = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(OVERLAPPED));
    if (!HeaderExtension->Overlapped)
    {
        /* No memory */
        HeapFree(GetProcessHeap(), 0, HeaderExtension);
        return MMSYSERR_NOMEM;
    }

    /* Create stream event */
    HeaderExtension->Overlapped->hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (!HeaderExtension->Overlapped->hEvent)
    {
        /* No memory */
        HeapFree(GetProcessHeap(), 0, HeaderExtension->Overlapped);
        HeapFree(GetProcessHeap(), 0, HeaderExtension);
        return MMSYSERR_NOMEM;
    }

    Header->reserved = (DWORD_PTR)HeaderExtension;
#endif
    /* This is what Windows XP/2003 returns */
    return MMSYSERR_NOTSUPPORTED;
}

MMRESULT
UnprepareWaveHeader(
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWAVEHDR Header)
{
    //PWAVEHDR_EXTENSION HeaderExtension;

    VALIDATE_MMSYS_PARAMETER( DeviceInfo );
    VALIDATE_MMSYS_PARAMETER( Header );

    DPRINT("Un-preparing wave header\n");
#if 0
    HeaderExtension = (PWAVEHDR_EXTENSION)Header->reserved;

    Header->reserved = 0;

    /* Destroy stream event */
    CloseHandle(HeaderExtension->Overlapped->hEvent);
    HeaderExtension->Overlapped->hEvent = NULL;

    /* Free OVERALPPED */
    HeapFree(GetProcessHeap(), 0, HeaderExtension->Overlapped);
    HeaderExtension->Overlapped = NULL;

    /* Free header extension */
    HeapFree(GetProcessHeap(), 0, HeaderExtension);
#endif
    /* This is what Windows XP/2003 returns */
    return MMSYSERR_NOTSUPPORTED;
}

MMRESULT
WriteWaveHeader(
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWAVEHDR Header)
{
    VALIDATE_MMSYS_PARAMETER( DeviceInfo );
    VALIDATE_MMSYS_PARAMETER( Header );

    DPRINT("Submitting wave header\n");

    /*
        A few minor sanity checks - any custom checks should've been carried
        out during wave header preparation etc.
    */
    VALIDATE_MMSYS_PARAMETER( Header->lpData != NULL );
    VALIDATE_MMSYS_PARAMETER( Header->dwBufferLength > 0 );
    VALIDATE_MMSYS_PARAMETER( Header->dwFlags & WHDR_PREPARED );
    VALIDATE_MMSYS_PARAMETER( ! (Header->dwFlags & WHDR_INQUEUE) );

    return EnqueueWaveHeader(DeviceInfo, Header);
}


/*
    EnqueueWaveHeader
        Put the header in the record/playback queue. This is performed within
        the context of the sound thread, it must NEVER be called from another
        thread.

    CompleteWaveHeader
        Set the header information to indicate that it has finished playing,
        and return it to the client application. This again must be called
        within the context of the sound thread.
*/

MMRESULT
EnqueueWaveHeader(
    IN PWDMAUD_DEVICE_INFO DeviceInfo,
    IN PWAVEHDR Header)
{
    VALIDATE_MMSYS_PARAMETER( DeviceInfo );
    VALIDATE_MMSYS_PARAMETER( Header );

    DPRINT("Next header %p\n", Header->lpNext);

    /* Set the "in queue" flag */
    Header->dwFlags |= WHDR_INQUEUE;

    /* Clear the "done" flag for the buffer */
    Header->dwFlags &= ~WHDR_DONE;

    /* Store device info */
    //((PWAVEHDR_EXTENSION)Header->reserved)->DeviceInfo = DeviceInfo;

    EnterCriticalSection(DeviceInfo->DeviceState->QueueCriticalSection);
    if (!DeviceInfo->DeviceState->WaveQueue)
    {
        /* This is the first header in the queue */
        DPRINT("Enqueued first wave header\n");
        DeviceInfo->DeviceState->WaveQueue = Header;
    }
    else
    {
        DPRINT("Enqueued next wave header\n");

        DeviceInfo->DeviceState->WaveQueue = Header;
        DeviceInfo->DeviceState->WaveQueue->lpNext = Header;
    }
    LeaveCriticalSection(DeviceInfo->DeviceState->QueueCriticalSection);

    /* Do wave streaming */
    return DoWaveStreaming(DeviceInfo, Header);
}

VOID
CompleteWaveHeader(
    IN  PWDMAUD_DEVICE_INFO DeviceInfo)
{
    PWAVEHDR Header;
    //PWAVEHDR_EXTENSION HeaderExtension;

    DPRINT("Completing wave header\n");

    Header = DeviceInfo->DeviceState->WaveQueue;

    if ( Header )
    {
        /* Move to the next header */
        DeviceInfo->DeviceState->WaveQueue = DeviceInfo->DeviceState->WaveQueue->lpNext;

        DPRINT("Returning buffer to client...\n");
#if 0
        /* Free header's device info */
        HeaderExtension = (PWAVEHDR_EXTENSION)Header->reserved;
        if (HeaderExtension->DeviceInfo)
        {
            HeapFree(GetProcessHeap(), 0, HeaderExtension->DeviceInfo);
            HeaderExtension->DeviceInfo = NULL;
        }
#endif
        /* Update the header */
        Header->lpNext = NULL;
        Header->dwFlags &= ~WHDR_INQUEUE;
        Header->dwFlags |= WHDR_DONE;

        /* Safe to do this without thread protection, as we're done with the header */
        NotifyMmeClient(DeviceInfo,
                        DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE ? WOM_DONE : WIM_DATA,
                        (DWORD_PTR)Header);
    }
}
