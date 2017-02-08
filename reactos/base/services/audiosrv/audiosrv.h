/*
 * PROJECT:          ReactOS
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             base/services/audiosrv/audiosrv.h
 * PURPOSE:          Audio Service (private header)
 * COPYRIGHT:        Copyright 2007 Andrew Greenwood
 */

#ifndef _AUDIOSRV_PCH_
#define _AUDIOSRV_PCH_

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <malloc.h>
#include <dbt.h>
#include <audiosrv/audiosrv.h>

#ifndef AUDIOSRV_PRIVATE_H
#define AUDIOSRV_PRIVATE_H

extern SERVICE_STATUS_HANDLE service_status_handle;


/* List management (pnp_list_manager.c) */

VOID*
CreateDeviceDescriptor(WCHAR* path, BOOL is_enabled);

#define DestroyDeviceDescriptor(descriptor) free(descriptor)

BOOL
AppendAudioDeviceToList(PnP_AudioDevice* device);

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

#endif /* _AUDIOSRV_PCH_ */
