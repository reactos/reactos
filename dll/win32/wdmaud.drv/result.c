/*
 * PROJECT:     ReactOS Sound Subsystem
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WDM Audio Driver MMRESULT return code routines
 * COPYRIGHT:   Copyright 2009-2010 Johannes Anderwald
 *              Copyright 2022 Oleg Dubinskiy (oleg.dubinskij30@gmail.com)
 */

#include <wdmaud.h>

/*
    Translate a Win32 error code into an MMRESULT code.
*/
MMRESULT
Win32ErrorToMmResult(
    IN  UINT ErrorCode)
{
    switch ( ErrorCode )
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

        case ERROR_INVALID_PARAMETER :
            return MMSYSERR_INVALPARAM;


        default :
            return MMSYSERR_ERROR;
    }
}

/*
    If a function invokes another function, this aids in translating the
    result code so that it is applicable in the context of the original caller.
    For example, specifying that an invalid parameter was passed probably does
    not make much sense if the parameter wasn't passed by the original caller!

    This could potentially highlight internal logic problems.

    However, things like MMSYSERR_NOMEM make sense to return to the caller.
*/
MMRESULT
TranslateInternalMmResult(
    IN  MMRESULT Result)
{
    switch ( Result )
    {
        case MMSYSERR_INVALPARAM :
        case MMSYSERR_INVALFLAG :
        {
            return MMSYSERR_ERROR;
        }
    }

    return Result;
}

