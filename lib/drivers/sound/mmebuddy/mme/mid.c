/*
    ReactOS Sound System
    MME Interface

    Purpose:
        MIDI Input device message handler

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        4 July 2008 - Created
*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <debug.h>

APIENTRY DWORD
midMessage(
    DWORD device_id,
    DWORD message,
    DWORD private_handle,
    DWORD parameter1,
    DWORD parameter2)
{
    DPRINT("midMessageStub called\n");

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
