/*
    ReactOS Sound System
    MME Interface

    Purpose:
        Wave input device message handler

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        4 July 2008 - Created
*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <ntddsnd.h>

#include <mmebuddy.h>

APIENTRY DWORD
widMessage(
    DWORD device_id,
    DWORD message,
    DWORD private_handle,
    DWORD parameter1,
    DWORD parameter2)
{
    TRACE_("widMessageStub called\n");

    switch ( message )
    {
        case WIDM_GETNUMDEVS :
            return GetSoundDeviceCount(WAVE_IN_DEVICE_TYPE);

        case WIDM_GETDEVCAPS :
        case WIDM_OPEN :
            return MMSYSERR_BADDEVICEID;

        case WIDM_CLOSE :
        case WIDM_START :
        case WIDM_RESET :
            return MMSYSERR_INVALHANDLE;

        default :
            return MMSYSERR_NOTSUPPORTED;
    }
}
