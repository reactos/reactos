/*
 * PROJECT:          ReactOS
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             include/reactos/libs/audiosrv/audiosrv.h
 * PURPOSE:          Audio Service Plug and Play list
 * COPYRIGHT:        Copyright 2007 Andrew Greenwood
 */

#include <windows.h>

#ifndef AUDIOSRV_H
#define AUDIOSRV_H

/* This is currently set to avoid conflicting service names in Windows! */
#define SERVICE_NAME                L"RosAudioSrv"

/* A named mutex is used for synchronizing access to the device list.
   If this mutex doesn't exist, it means the audio service isn't running. */
#define AUDIO_LIST_LOCK_NAME        L"Global\\AudioDeviceListLock"

/* ...and this is where the device list will be available */
#define AUDIO_LIST_NAME             L"Global\\AudioDeviceList"

/* Amount of shared memory to allocate */
#define AUDIO_LIST_MAX_SIZE         65536

typedef struct
{
    DWORD enabled;
    WCHAR path[];       /* The device object path (excluded from sizeof) */
} PnP_AudioDevice;

typedef struct
{
    DWORD size;         /* Size of the shared mem */
    DWORD max_size;     /* Amount of mem available */
    DWORD device_count; /* Number of devices */
    PnP_AudioDevice first_device[];
} PnP_AudioHeader;


/* Calculate amount of memory consumed by a wide string - this includes the
   terminating NULL. */

#define WideStringSize(str) \
    ( (lstrlenW(str) + 1) * sizeof(WCHAR) )

BOOL
InitializeAudioDeviceListLock();

VOID
KillAudioDeviceListLock();

VOID
LockAudioDeviceList();

VOID
UnlockAudioDeviceList();

#endif
