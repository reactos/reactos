/*
    ReactOS Sound System
    MME Interface

    Purpose:
        Auxiliary device message handler

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        4 July 2008 - Created
*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <debug.h>

APIENTRY DWORD
auxMessage(
    DWORD device_id,
    DWORD message,
    DWORD private_handle,
    DWORD parameter1,
    DWORD parameter2)
{
    DPRINT("auxMessageStub called\n");
    /* TODO */
    return MMSYSERR_NOTSUPPORTED;
}
