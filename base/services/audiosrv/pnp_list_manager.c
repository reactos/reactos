/*
 * PROJECT:          ReactOS
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             base/services/audiosrv/pnp_list_manager.c
 * PURPOSE:          Audio Service List Manager
 * COPYRIGHT:        Copyright 2007 Andrew Greenwood
 */

#include "audiosrv.h"

/*
    Device descriptor
*/

VOID*
CreateDeviceDescriptor(WCHAR* path, BOOL is_enabled)
{
    PnP_AudioDevice* device;

    int path_length = WideStringSize(path);
    int size = sizeof(PnP_AudioDevice) + path_length;

/*    printf("path_length %d, total %d\n", path_length, size);*/

    device = malloc(size);
    if (! device)
    {
        logmsg("Failed to create a device descriptor (malloc fail)\n");
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

/*
    printf("list size is %d\n", audio_device_list->size);
    printf("device info size is %d bytes\n", device_info_size);
*/

    /* We DON'T want to overshoot the end of the buffer! */
    if (audio_device_list->size + device_info_size > audio_device_list->max_size)
    {
        /*printf("max_size would be exceeded! Failing...\n");*/

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

    logmsg("Device added to list\n");

    return TRUE;
}

BOOL
CreateAudioDeviceList(DWORD max_size)
{
/*    printf("Initializing memory device list lock\n");*/

    if (!InitializeAudioDeviceListLock())
    {
        /*printf("Failed!\n");*/
        return FALSE;
    }

    /* Preliminary locking - the list memory will likely be a big
       buffer of gibberish at this point so we don't want anyone
       turning up before we're ready... */
    LockAudioDeviceList();

    logmsg("Creating file mapping\n");
    /* Expose our device list to the world */
    device_list_file = CreateFileMappingW(INVALID_HANDLE_VALUE,
                                          NULL,
                                          PAGE_READWRITE,
                                          0,
                                          max_size,
                                          AUDIO_LIST_NAME);
    if (!device_list_file)
    {
        logmsg("Creation of audio device list failed (err %d)\n", GetLastError());

        UnlockAudioDeviceList();
        KillAudioDeviceListLock();

        return FALSE;
    }

    logmsg("Mapping view of file\n");
    /* Of course, we'll need to access the list ourselves */
    audio_device_list = MapViewOfFile(device_list_file,
                                      FILE_MAP_WRITE,
                                      0,
                                      0,
                                      max_size);
    if (!audio_device_list)
    {
        logmsg("MapViewOfFile FAILED (err %d)\n", GetLastError());

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

    logmsg("Device list created\n");

    return TRUE;
}

VOID
DestroyAudioDeviceList(VOID)
{
    logmsg("Destroying device list\n");

    LockAudioDeviceList();

    /*printf("Unmapping view\n");*/
    UnmapViewOfFile(audio_device_list);
    audio_device_list = NULL;

    /*printf("Closing memory mapped file\n");*/
    CloseHandle(device_list_file);
    device_list_file = NULL;

    UnlockAudioDeviceList();

    /*printf("Killing devlist lock\n");*/
    KillAudioDeviceListLock();
}
