/*
    ReactOS Sound System
    MME Driver Helper

    Purpose:
        Wave thread operations

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        4 July 2008 - Created
*/

#include <windows.h>
#include <mmsystem.h>

#include <mmebuddy.h>

MMRESULT
StartWaveThread(
    IN  PSOUND_DEVICE_INSTANCE Instance)
{
    MMRESULT Result;

    if ( ! Instance )
        return MMSYSERR_INVALPARAM;

    /* Kick off the thread */
    Result = StartSoundThread(Instance);
    if ( Result != MMSYSERR_NOERROR )
    {
        return Result;
    }

    /* AddSoundThreadOperation(Instance, 69, SayHello); */
    return MMSYSERR_NOTSUPPORTED;
}
