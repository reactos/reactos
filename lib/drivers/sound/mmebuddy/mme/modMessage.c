/*
    ReactOS Sound System
    MME Interface

    Purpose:
        MIDI Output device message handler

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        4 July 2008 - Created
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
