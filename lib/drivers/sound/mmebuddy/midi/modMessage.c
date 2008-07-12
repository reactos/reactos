/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/midi/modMessage.c
 *
 * PURPOSE:     Provides the modMessage exported function, as required by
 *              the MME API, for MIDI output device support.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>

#include <mmebuddy.h>

APIENTRY DWORD
modMessage(
    DWORD device_id,
    DWORD message,
    DWORD private_handle,
    DWORD parameter1,
    DWORD parameter2)
{
    TRACE_("modMessageStub called\n");

    switch ( message )
    {
        case MODM_GETNUMDEVS :
            return 0;

        case MODM_GETDEVCAPS :
        case MODM_OPEN :
            return MMSYSERR_BADDEVICEID;

        case MODM_CLOSE :
        case MODM_DATA :
        case MODM_LONGDATA :
        case MODM_RESET :
            return MMSYSERR_INVALHANDLE;

        default :
            return MMSYSERR_NOTSUPPORTED;
    }
}
