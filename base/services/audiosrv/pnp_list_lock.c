/*
 * PROJECT:     ReactOS
 * LICENSE:     GPL - See COPYING in the top level directory
 * PURPOSE:     Audio Service Plug and Play list locking mechanism
 * COPYRIGHT:   Copyright 2007 Andrew Greenwood
 */

#include "audiosrv.h"

#include <assert.h>

static HANDLE audio_device_list_lock = NULL;

BOOL
InitializeAudioDeviceListLock(VOID)
{
    /* The security stuff is to make sure the mutex can be grabbed by
       other processes - is this the best idea though ??? */

    SECURITY_DESCRIPTOR security_descriptor;
    SECURITY_ATTRIBUTES security;

    InitializeSecurityDescriptor(&security_descriptor, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&security_descriptor, TRUE, 0, FALSE);

    security.nLength = sizeof(SECURITY_ATTRIBUTES);
    security.lpSecurityDescriptor = &security_descriptor;
    security.bInheritHandle = FALSE;

    audio_device_list_lock = CreateMutex(&security,
                                         FALSE,
                                         AUDIO_LIST_LOCK_NAME);

    return (audio_device_list_lock != NULL);
}

VOID
KillAudioDeviceListLock(VOID)
{
    CloseHandle(audio_device_list_lock);
    audio_device_list_lock = NULL;
}

VOID
LockAudioDeviceList(VOID)
{
    assert(audio_device_list_lock != NULL);
    WaitForSingleObject(audio_device_list_lock, INFINITE);
}

VOID
UnlockAudioDeviceList(VOID)
{
    assert(audio_device_list_lock != NULL);
    ReleaseMutex(audio_device_list_lock);
}
