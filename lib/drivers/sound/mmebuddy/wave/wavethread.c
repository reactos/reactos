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
ProcessWaveThreadRequest(
    IN  PSOUND_DEVICE_INSTANCE Instance,
    IN  DWORD RequestId,
    IN  PVOID Data)
{
    /* Just some temporary testing code for now */
    WCHAR msg[128];
    wsprintf(msg, L"Request %d received", RequestId);

    MessageBox(0, msg, L"Request", MB_OK | MB_TASKMODAL);

    return MMSYSERR_NOTSUPPORTED;
}

MMRESULT
StartWaveThread(
    IN  PSOUND_DEVICE_INSTANCE Instance)
{
    MMRESULT Result = MMSYSERR_NOERROR;

    if ( ! Instance )
        return MMSYSERR_INVALPARAM;

    /* Kick off the thread */
    Result = StartSoundThread(Instance, ProcessWaveThreadRequest);
    if ( Result != MMSYSERR_NOERROR )
    {
        return Result;
    }

    /* AddSoundThreadOperation(Instance, 69, SayHello); */
    return MMSYSERR_NOERROR;
}

MMRESULT
StopWaveThread(
    IN  PSOUND_DEVICE_INSTANCE Instance)
{
    if ( ! Instance )
        return MMSYSERR_INVALPARAM;

    return StopSoundThread(Instance);
}
