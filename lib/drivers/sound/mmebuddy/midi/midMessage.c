/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/midi/midMessage.c
 *
 * PURPOSE:     Provides the midMessage exported function, as required by
 *              the MME API, for MIDI input device support.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>

#include <mmebuddy.h>

APIENTRY DWORD
midMessage(
    DWORD device_id,
    DWORD message,
    DWORD private_handle,
    DWORD parameter1,
    DWORD parameter2)
{
    TRACE_("midMessageStub called\n");

    switch ( message )
    {
        case MIDM_GETNUMDEVS :
            return 0;

        case MIDM_GETDEVCAPS :
        case MIDM_OPEN :
            return MMSYSERR_BADDEVICEID;

        case MIDM_CLOSE :
        case MIDM_ADDBUFFER :
        case MIDM_START :
        case MIDM_STOP :
        case MIDM_RESET :
            return MMSYSERR_INVALHANDLE;

        default :
            return MMSYSERR_NOTSUPPORTED;
    }
}
