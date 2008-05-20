/*
 * PROJECT:          ReactOS
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             base/services/audiosrv/list_lock.c
 * PURPOSE:          Audio Service Plug and Play list locking mechanism
 * COPYRIGHT:        Copyright 2007 Andrew Greenwood
 */

#include <windows.h>
#include <assert.h>
#include <audiosrv/audiosrv.h>

static HANDLE audio_device_list_lock = NULL;

BOOL
InitializeAudioDeviceListLock()
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

    return ( audio_device_list_lock != NULL );
}

VOID
KillAudioDeviceListLock()
{
    CloseHandle(audio_device_list_lock);
    audio_device_list_lock = NULL;
}

VOID
LockAudioDeviceList()
{
    assert( audio_device_list_lock != NULL );
    WaitForSingleObject(audio_device_list_lock, INFINITE);
}

VOID
UnlockAudioDeviceList()
{
    assert( audio_device_list_lock != NULL );
    ReleaseMutex(audio_device_list_lock);
}

