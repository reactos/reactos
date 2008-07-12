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

VOID
NotifySoundClient(
    DWORD Message,
    DWORD Parameter)
{
    /* TODO... DriverCallback */
}
