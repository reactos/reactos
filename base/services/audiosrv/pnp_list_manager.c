/*
 * PROJECT:     ReactOS
 * LICENSE:     GPL - See COPYING in the top level directory
 * PURPOSE:     Audio Service List Manager
 * COPYRIGHT:   Copyright 2007 Andrew Greenwood
 */

#include "audiosrv.h"

#define NDEBUG
#include <debug.h>

/*
    Device descriptor
*/

VOID*
CreateDeviceDescriptor(WCHAR* path, BOOL is_enabled)
{
    PnP_AudioDevice* device;

    int path_length = WideStringSize(path);
    int size = sizeof(PnP_AudioDevice) + path_length;

    device = malloc(size);
    if (! device)
    {
        DPRINT("Failed to malloc device descriptor\n");
        return NULL;
    }

    device->enabled = is_enabled;
    memcpy(device->path, path, path_length);

    return device;
}


/*
    Device list (manager-side)

    The device list is stored in some shared-memory, with a named, global
    mutex to provide a locking mechanism (to avoid it from being updated
    whilst being read).
*/

static HANDLE device_list_file = NULL;
static PnP_AudioHeader* audio_device_list = NULL;


/*
    TODO: Detect duplicate entries and ignore them! (In case we receive
    a PnP event for an existing device...)
*/

BOOL
AppendAudioDeviceToList(PnP_AudioDevice* device)
{
    int device_info_size;

    /* Figure out the actual structure size */
    device_info_size = sizeof(PnP_AudioDevice);
    device_info_size += WideStringSize(device->path);

    LockAudioDeviceList();

    /* We DON'T want to overshoot the end of the buffer! */
    if (audio_device_list->size + device_info_size > audio_device_list->max_size)
    {
        /*DPRINT("failed, max_size would be exceeded\n");*/

        UnlockAudioDeviceList();

        return FALSE;
    }

    /* Commit the device descriptor to the list */
    memcpy((char*)audio_device_list + audio_device_list->size,
           device,
           device_info_size);

    /* Update the header */
    audio_device_list->device_count ++;
    audio_device_list->size += device_info_size;

    UnlockAudioDeviceList();

    DPRINT("Device added to list\n");

    return TRUE;
}

BOOL
CreateAudioDeviceList(DWORD max_size)
{
    if (!InitializeAudioDeviceListLock())
    {
        /*DPRINT("Failed\n");*/
        return FALSE;
    }

    /* Preliminary locking - the list memory will likely be a big
       buffer of gibberish at this point so we don't want anyone
       turning up before we're ready... */
    LockAudioDeviceList();

    DPRINT("Creating file mapping\n");
    /* Expose our device list to the world */
    device_list_file = CreateFileMappingW(INVALID_HANDLE_VALUE,
                                          NULL,
                                          PAGE_READWRITE,
                                          0,
                                          max_size,
                                          AUDIO_LIST_NAME);
    if (!device_list_file)
    {
        DPRINT("Creation of audio device list failed (err %d)\n", GetLastError());

        UnlockAudioDeviceList();
        KillAudioDeviceListLock();

        return FALSE;
    }

    DPRINT("Mapping view of file\n");
    /* Of course, we'll need to access the list ourselves */
    audio_device_list = MapViewOfFile(device_list_file,
                                      FILE_MAP_WRITE,
                                      0,
                                      0,
                                      max_size);
    if (!audio_device_list)
    {
        DPRINT("MapViewOfFile FAILED (err %d)\n", GetLastError());

        CloseHandle(device_list_file);
        device_list_file = NULL;

        UnlockAudioDeviceList();
        KillAudioDeviceListLock();

        return FALSE;
    }

    /* Clear the mem to avoid any random stray data */
    memset(audio_device_list, 0, max_size);

    /* Don't want devices to overwrite the list! */
    audio_device_list->size = sizeof(PnP_AudioHeader);
    audio_device_list->max_size = max_size;
    audio_device_list->device_count = 0;

    UnlockAudioDeviceList();

    DPRINT("Device list created\n");

    return TRUE;
}

VOID
DestroyAudioDeviceList(VOID)
{
    DPRINT("Destroying device list\n");

    LockAudioDeviceList();

    /*DPRINT("Unmapping view\n");*/
    UnmapViewOfFile(audio_device_list);
    audio_device_list = NULL;

    /*DPRINT("Closing memory mapped file\n");*/
    CloseHandle(device_list_file);
    device_list_file = NULL;

    UnlockAudioDeviceList();

    /*DPRINT("Killing devlist lock\n");*/
    KillAudioDeviceListLock();
}
