/*
 * PROJECT:          ReactOS
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             base/services/audiosrv/audiosrv.h
 * PURPOSE:          Audio Service (private header)
 * COPYRIGHT:        Copyright 2007 Andrew Greenwood
 */

#include <dbt.h>

/* This is currently set to avoid conflicting service names in Windows! */
#define SERVICE_NAME                L"RosAudioSrv"

/* A named mutex is used for synchronizing access to the device list.
   If this mutex doesn't exist, it means the audio service isn't running. */
#define AUDIO_LIST_LOCK_NAME        L"Global\\AudioDeviceListLock"

/* ...and this is where the device list will be available */
#define AUDIO_LIST_NAME             L"Global\\AudioDeviceList"

/* Amount of shared memory to allocate */
#define AUDIO_LIST_MAX_SIZE         65536

#ifndef AUDIOSRV_PRIVATE_H
#define AUDIOSRV_PRIVATE_H

extern SERVICE_STATUS_HANDLE service_status_handle;


/* List management (pnp_list_manager.c) */

VOID*
CreateDeviceDescriptor(WCHAR* path, BOOL is_enabled);

#define DestroyDeviceDescriptor(descriptor) free(descriptor)

/*BOOL
AppendAudioDeviceToList(PnP_AudioDevice* device);*/

BOOL
CreateAudioDeviceList(DWORD max_size);

VOID
DestroyAudioDeviceList(VOID);


/* Plug and Play (pnp.c) */

BOOL
ProcessExistingDevices(VOID);

DWORD
ProcessDeviceArrival(DEV_BROADCAST_DEVICEINTERFACE* device);

BOOL
RegisterForDeviceNotifications(VOID);

VOID
UnregisterDeviceNotifications(VOID);

DWORD
HandleDeviceEvent(
    DWORD dwEventType,
    LPVOID lpEventData);

BOOL
StartSystemAudioServices(VOID);

/* Debugging */

void logmsg(char* string, ...);

#endif
