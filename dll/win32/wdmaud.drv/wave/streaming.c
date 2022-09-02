/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/sound/mmebuddy/wave/streaming.c
 *
 * PURPOSE:     Wave streaming
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include "wdmaud.h"

#define YDEBUG
#include <debug.h>


/*
    DoWaveStreaming
        Check if there is streaming to be done, and if so, do it.
*/

MMRESULT
DoWaveStreaming(
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWAVEHDR Header)
{
    MMRESULT Result;

    /* Is there any work to do? */
    if (!Header)
    {
        DPRINT("DoWaveStreaming: No work to do - doing nothing\n");
        return MMSYSERR_NOMEM;
    }

    /* Sanity checks */
    ASSERT(Header->dwFlags & WHDR_PREPARED);
    ASSERT(Header->dwFlags & WHDR_INQUEUE);

    /* Submit wave header */
    Result = FUNC_NAME(WdmAudSubmitWaveHeader)(DeviceInfo, Header);
    if (!MMSUCCESS(Result))
    {
        DPRINT1("WdmAudSubmitWaveHeader failed with error %d\n", GetLastError());
        Header->dwFlags &= ~WHDR_INQUEUE;
        return TranslateInternalMmResult(Result);
    }

    /* Done */
    return MMSYSERR_NOERROR;
}

/*
    Stream control functions
    (External/internal thread pairs)

    TODO - Move elsewhere as these shouldn't be wave specific!
*/

MMRESULT
StopStreamingInSoundThread(
    IN  PWDMAUD_DEVICE_INFO DeviceInfo)
{
    MMRESULT Result;

    /* Reset the stream */
    Result = FUNC_NAME(WdmAudResetStream)(DeviceInfo, FALSE);

    if ( ! MMSUCCESS(Result) )
    {
        /* Failed */
        return TranslateInternalMmResult(Result);
    }

    return MMSYSERR_NOERROR;
}

MMRESULT
StopStreaming(
    IN  PWDMAUD_DEVICE_INFO DeviceInfo)
{
    if ( ! DeviceInfo )
        return MMSYSERR_INVALHANDLE;

    if ( DeviceInfo->DeviceType != WAVE_OUT_DEVICE_TYPE && DeviceInfo->DeviceType != WAVE_IN_DEVICE_TYPE )
        return MMSYSERR_NOTSUPPORTED;

    return StopStreamingInSoundThread(DeviceInfo);
}
