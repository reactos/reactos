/*
 * PROJECT:          ReactOS
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             base/services/audiosrv/pnp_list_manager.h
 * PURPOSE:          Audio Service List Manager
 * COPYRIGHT:        Copyright 2007 Andrew Greenwood
 */

#include <assert.h>
#include <audiosrv/audiosrv.h>

#ifndef PNP_LIST_MANAGER_H
#define PNP_LIST_MANAGER_H

VOID*
CreateDeviceDescriptor(WCHAR* path, BOOL is_enabled);

#define DestroyDeviceDescriptor(descriptor) free(descriptor)

BOOL
AppendAudioDeviceToList(PnP_AudioDevice* device);

BOOL
CreateAudioDeviceList(DWORD max_size);

VOID
DestroyAudioDeviceList();

#endif
