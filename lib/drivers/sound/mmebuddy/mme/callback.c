/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/mme/callback.c
 *
 * PURPOSE:     Calls an MME API client application, usually to return
 *              buffers which have been processed etc.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>

#include <mmebuddy.h>

VOID
NotifySoundClient(
    PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    DWORD Message,
    DWORD Parameter)
{
    ASSERT( SoundDeviceInstance );

    TRACE_("MME client callback - message %d, parameter %d\n",
           (int) Message,
           (int) Parameter);

    if ( SoundDeviceInstance->WinMM.ClientCallback )
    {
        DriverCallback(SoundDeviceInstance->WinMM.ClientCallback,
                       HIWORD(SoundDeviceInstance->WinMM.Flags),
                       SoundDeviceInstance->WinMM.Handle,
                       Message,
                       SoundDeviceInstance->WinMM.ClientCallbackInstanceData,
                       Parameter,
                       0);
    }
}
