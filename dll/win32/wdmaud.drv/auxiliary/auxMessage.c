/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/auxiliary/auxMessage.c
 *
 * PURPOSE:     Provides the auxMessage exported function, as required by
 *              the MME API, for auxiliary device support.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include "wdmaud.h"

#define YDEBUG
#include <debug.h>

/*
    Standard MME driver entry-point for messages relating to auxiliary devices.
*/
DWORD
APIENTRY
auxMessage(
    UINT DeviceId,
    UINT Message,
    DWORD_PTR PrivateHandle,
    DWORD_PTR Parameter1,
    DWORD_PTR Parameter2)
{
    MMRESULT Result = MMSYSERR_NOTSUPPORTED;

    AcquireEntrypointMutex(AUX_DEVICE_TYPE);

    DPRINT("auxMessage - Message type %d\n", Message);

    switch ( Message )
    {
#ifndef USE_MMIXER_LIB
        case AUXM_INIT:
        {
            Result = WdmAudAddRemoveDeviceNode(AUX_DEVICE_TYPE, TRUE);
            break;
        }

        case DRVM_EXIT:
        {
            Result = WdmAudAddRemoveDeviceNode(AUX_DEVICE_TYPE, FALSE);
            break;
        }
#endif

        case AUXDM_GETNUMDEVS :
        {
            Result = MmeGetNumDevs(AUX_DEVICE_TYPE);
            break;
        }

        case AUXDM_GETDEVCAPS :
        {
            Result = MmeGetSoundDeviceCapabilities(AUX_DEVICE_TYPE,
                                                   DeviceId,
                                                   (PVOID) Parameter1,
                                                   Parameter2);
            break;
        }
    }

    DPRINT("auxMessage returning MMRESULT %d\n", Result);

    ReleaseEntrypointMutex(AUX_DEVICE_TYPE);

    return Result;
}
