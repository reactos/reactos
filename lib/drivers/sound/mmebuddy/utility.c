/*
    ReactOS Sound System
    MME Driver Helper

    Purpose:
        Utility functions

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        4 July 2008 - Created
*/

#include <windows.h>
#include <mmsystem.h>

ULONG
GetDigitCount(
    ULONG Number)
{
    ULONG Value = Number;
    ULONG Digits = 1;

    while ( Value > 9 )
    {
        Value /= 10;
        ++ Digits;
    }

    return Digits;
}

MMRESULT
Win32ErrorToMmResult(UINT error_code)
{
    switch ( error_code )
    {
        case NO_ERROR :
        case ERROR_IO_PENDING :
            return MMSYSERR_NOERROR;

        case ERROR_BUSY :
            return MMSYSERR_ALLOCATED;

        case ERROR_NOT_SUPPORTED :
        case ERROR_INVALID_FUNCTION :
            return MMSYSERR_NOTSUPPORTED;

        case ERROR_NOT_ENOUGH_MEMORY :
            return MMSYSERR_NOMEM;

        case ERROR_ACCESS_DENIED :
            return MMSYSERR_BADDEVICEID;

        case ERROR_INSUFFICIENT_BUFFER :
            return MMSYSERR_INVALPARAM;
    };

    /* If all else fails, it's just a plain old error */

    return MMSYSERR_ERROR;
}
