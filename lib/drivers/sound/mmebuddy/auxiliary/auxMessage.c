/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/auxiliary/auxMessage.c
 *
 * PURPOSE:     Provides the auxMessage exported function, as required by
 *              the MME API, for auxiliary functionality support.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>

#include <mmebuddy.h>

APIENTRY DWORD
auxMessage(
    DWORD device_id,
    DWORD message,
    DWORD private_handle,
    DWORD parameter1,
    DWORD parameter2)
{
    TRACE_("auxMessageStub called\n");
    /* TODO */
    return MMSYSERR_NOTSUPPORTED;
}
