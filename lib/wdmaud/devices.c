/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS Multimedia
 * FILE:                 lib/wdmaud/devices.c
 * PURPOSE:              WDM Audio Support - Device Management
 * PROGRAMMER:           Andrew Greenwood
 * UPDATE HISTORY:
 *                       Nov 18, 2005: Created
 * 
 * WARNING! SOME OF THESE FUNCTIONS OUGHT TO COPY THE DEVICE INFO STRUCTURE
 * THAT HAS BEEN FED TO THEM!
*/

#include <windows.h>
#include "wdmaud.h"

const char WDMAUD_DEVICE_INFO_SIG[4] = "WADI";
const char WDMAUD_DEVICE_STATE_SIG[4] = "WADS";
const char WDMAUD_NULL_SIGNATURE[4] = {0,0,0,0};


BOOL IsValidDevicePath(WCHAR* path)
{
    if (IsBadReadPtr(path, 1))  /* TODO: Replace with flags */
    {
        DPRINT1("Bad interface\n");
        return FALSE;
    }

    /* Original driver seems to check for strlenW < 0x1000 */

    return TRUE;
}

MMRESULT ValidateDeviceInfo(PWDMAUD_DEVICE_INFO device_info)
{
    if ( IsBadWritePtr(device_info, sizeof(WDMAUD_DEVICE_INFO)) )
        return MMSYSERR_INVALPARAM;

    if ( *device_info->signature != *WDMAUD_DEVICE_INFO_SIG )
        return MMSYSERR_INVALPARAM;

    return MMSYSERR_NOERROR;
}

MMRESULT ValidateDeviceState(PWDMAUD_DEVICE_STATE state)
{
    if ( IsBadWritePtr(state, sizeof(WDMAUD_DEVICE_INFO)) )
        return MMSYSERR_INVALPARAM;

    if ( *state->signature != *WDMAUD_DEVICE_STATE_SIG )
        return MMSYSERR_INVALPARAM;

    return MMSYSERR_NOERROR;
}

/*
    ValidateDeviceStateEvents should be used in conjunction with the standard
    state validation routine.
*/

MMRESULT ValidateDeviceStateEvents(PWDMAUD_DEVICE_STATE state)
{
    if ( ( (DWORD) state->exit_thread_event == 0x00000000 ) &&
         ( (DWORD) state->exit_thread_event != 0x48484848 ) )
    {
        DPRINT1("Bad exit thread event\n");
        return MMSYSERR_INVALPARAM;
    }

    if ( ( (DWORD) state->queue_event == 0x00000000 ) &&
         ( (DWORD) state->queue_event != 0x42424242 ) &&
         ( (DWORD) state->queue_event != 0x43434343 ) )
    {
        DPRINT1("Bad queue event\n");
        return MMSYSERR_INVALPARAM;
    }

    return MMSYSERR_NOERROR;
}

MMRESULT ValidateDeviceInfoAndState(PWDMAUD_DEVICE_INFO device_info)
{
    MMRESULT result;

    result = ValidateDeviceInfo(device_info);

    if ( result != MMSYSERR_NOERROR )
        return result;

    result = ValidateDeviceState(device_info->state);

    if ( result != MMSYSERR_NOERROR )
        return result;

    return MMSYSERR_NOERROR;
}

PWDMAUD_DEVICE_INFO CreateDeviceData(CHAR device_type, WCHAR* device_path)
{
    HANDLE heap = 0;
    PWDMAUD_DEVICE_INFO device_data = 0;
    int path_size = 0;

    DPRINT("Creating device data for device type %d\n", (int) device_type);

    if ( ! IsValidDevicePath(device_path) )
    {
        DPRINT1("No valid device interface given!\n");
        goto cleanup;
    }

    /* Take into account this is a unicode string... */
    path_size = (lstrlen(device_path) + 1) * sizeof(WCHAR);
    /* DPRINT("Size of path is %d\n", (int) path_size); */

    heap = GetProcessHeap();

    if ( ! heap )
    {
        DPRINT1("Couldn't get the process heap (error %d)\n",
                (int) GetLastError());
        goto cleanup;
    }

    DPRINT("Allocating %d bytes\n",
           path_size + sizeof(WDMAUD_DEVICE_INFO));
/*
    device_data = (PWDMAUD_DEVICE_INFO) HeapAlloc(heap,
                                                  HEAP_ZERO_MEMORY,
                                                  path_size + sizeof(WDMAUD_DEVICE_INFO));
*/

    device_data = (PWDMAUD_DEVICE_INFO)
        AllocMem(path_size + sizeof(WDMAUD_DEVICE_INFO));

    if ( ! device_data )
    {
        DPRINT1("Unable to allocate memory for device data (error %d)\n",
                (int) GetLastError());
        goto cleanup;
    }

    DPRINT("Copying signature\n");
    memcpy(device_data->signature, WDMAUD_DEVICE_INFO_SIG, 4);

    DPRINT("Copying path (0x%x)\n", (int)device_path);
    lstrcpy(device_data->path, device_path);

    device_data->type = device_type;

    cleanup :
    {
        /* No cleanup needed (no failures possible after allocation.) */
        DPRINT("Performing cleanup\n");

        return device_data;
    }
}

/*
    CloneDeviceData

    This isn't all that great... Maybe some macros would be better:
    BEGIN_CLONING_STRUCT(source, target)
        CLONE_MEMBER(member)
    END_CLONING_STRUCT()

    The main problem is that sometimes we'll want to copy more than
    the data presented here. I guess we could blindly copy EVERYTHING
    but that'd be excessive.
*/

PWDMAUD_DEVICE_INFO CloneDeviceData(PWDMAUD_DEVICE_INFO original)
{
    PWDMAUD_DEVICE_INFO clone = NULL;

    if ( ValidateDeviceInfo(original) != MMSYSERR_NOERROR)
    {
        DPRINT1("Original device data was invalid\n");
        return NULL;
    }

    /* This will set the type and path, so we can forget about those */
    clone = CreateDeviceData(original->type, original->path);

    if ( ! clone )
    {
        DPRINT1("Clone creation failed\n");
        return NULL;
    }

    clone->id = original->id;
    clone->wave_handle = original->wave_handle; /* ok? */

    /* TODO: Maybe we should copy some more? */

    return clone;
}

void DeleteDeviceData(PWDMAUD_DEVICE_INFO device_data)
{
    HANDLE heap;

    ASSERT( device_data );

    /* Erase the signature to prevent any possible future mishaps */
    *device_data->signature = *WDMAUD_NULL_SIGNATURE;

    heap = GetProcessHeap();

    if ( ! heap )
    {
        DPRINT1("Couldn't get the process heap (error %d)\n",
                (int) GetLastError());
        goto exit;
    }

    FreeMem(device_data);
    /*
    {
        DPRINT1("Couldn't free device data memory (error %d)\n",
                (int) GetLastError());
    }
    */

    exit :
        /* We just return */
        return;
}

MMRESULT ModifyDevicePresence(
    CHAR device_type,
    WCHAR* device_path,
    BOOL adding)
{
    DWORD ioctl = 0;
    PWDMAUD_DEVICE_INFO device_data = 0;
    MMRESULT result = MMSYSERR_ERROR;
    MMRESULT kernel_result = MMSYSERR_ERROR;

    DPRINT("ModifyDevicePresence - %s a device\n",
           adding ? "adding" : "removing");

    /* DPRINT("Topology path %S\n", device_path); */
    DPRINT("Devtype %d\n", (int) device_type);

    ASSERT( IsValidDeviceType(device_type) );
    ASSERT( device_path );

    device_data = CreateDeviceData(device_type, device_path);

    if ( ! device_data )
    {
        DPRINT1("Couldn't allocate memory for device data\n");
        result = MMSYSERR_NOMEM;
        goto cleanup;
    }

    ioctl = adding ? IOCTL_WDMAUD_ADD_DEVICE : IOCTL_WDMAUD_REMOVE_DEVICE;

    kernel_result = CallKernelDevice(device_data,
                                     ioctl,
                                     0,
                                     0);

    if ( kernel_result != MMSYSERR_NOERROR )
    {
        DPRINT1("WdmAudioIoControl FAILED with error %d\n", (int) kernel_result);

        switch ( kernel_result )
        {
            /* TODO: Translate into a real error code */
            default :
                result = MMSYSERR_ERROR;
        }

        goto cleanup;
    }

    DPRINT("ModifyDevicePresence succeeded\n");

    result = MMSYSERR_NOERROR;

    cleanup :
    {
        if ( device_data )
            DeleteDeviceData(device_data);

        return result;
    }
}

DWORD GetDeviceCount(CHAR device_type, WCHAR* topology_path)
{
    PWDMAUD_DEVICE_INFO device_data;
    int device_count = 0;

    DPRINT("Topology path %S\n", topology_path);

    device_data = CreateDeviceData(device_type, topology_path);

    if (! device_data)
    {
        DPRINT1("Couldn't allocate device data\n");
        goto cleanup;
    }

    DPRINT("Getting num devs\n");

    device_data->with_critical_section = FALSE;

    if ( CallKernelDevice(device_data,
                          IOCTL_WDMAUD_GET_DEVICE_COUNT,
                          0,
                          0) != MMSYSERR_NOERROR )
    {
        DPRINT1("Failed\n");
        goto cleanup;
    }

    device_count = device_data->id;

    DPRINT("There are %d devs\n", device_count);

    cleanup :
    {
        if ( device_data )
            DeleteDeviceData(device_data);

        return device_count;
    }
}

/*
    GetDeviceCapabilities

    This uses a different structure to the traditional documentation, because
    we handle plug and play devices.

    Much sleep was lost over implementing this.
*/

MMRESULT GetDeviceCapabilities(
    CHAR device_type,
    DWORD device_id,
    WCHAR* device_path,
    LPMDEVICECAPSEX caps
)
{
    PWDMAUD_DEVICE_INFO device = NULL;
    MMRESULT result = MMSYSERR_ERROR;

    DPRINT("Device path %S\n", device_path);

    /* Is this right? */
    if (caps->cbSize == 0)
    {
        DPRINT1("We appear to have been given an invalid parameter\n");
        return MMSYSERR_INVALPARAM;
    }

    DPRINT("Going to have to query the kernel-mode part\n");

    device = CreateDeviceData(device_type, device_path);

    if ( ! device )
    {
        DPRINT("Unable to allocate device data memory\n");
        result = MMSYSERR_NOMEM;
        goto cleanup;
    }

    device->id = device_id;
    device->with_critical_section = FALSE;

    *(LPWORD)caps->pCaps = (WORD) 0x43;

    DPRINT("Calling kernel device\n");
    result = CallKernelDevice(device,
                              IOCTL_WDMAUD_GET_CAPABILITIES,
                              (DWORD)caps->cbSize,
                              (DWORD)caps->pCaps);

    if ( result != MMSYSERR_NOERROR )
    {
        DPRINT("IoControl failed\n");
        goto cleanup;
    }

    /* Return code will already be MMSYSERR_NOERROR by now */

    cleanup :
    {
        if ( device )
            DeleteDeviceData(device);

        return result;
    }
}

MMRESULT TryOpenDevice(
    PWDMAUD_DEVICE_INFO device,
    LPWAVEFORMATEX format
)
{
    if ( device->id > 0x64 )    /* FIXME */
    {
        DPRINT1("device->id > 0x64 ! ???\n");
        return MMSYSERR_BADDEVICEID; /* OK? */
    }

    /* We'll only have a format set for wave devices */
    if ( format )
    {
        if ( format->wFormatTag == 1 )
        {
            DWORD sample_size;

            DPRINT("Standard (PCM) format\n");
            sample_size = format->nChannels * format->wBitsPerSample;
            device->state->sample_size = sample_size;

            if ( CallKernelDevice(device,
                                  IOCTL_WDMAUD_OPEN_DEVICE,
                                  0x10,
                                  (DWORD)format)
                            != MMSYSERR_NOERROR )
            {
                DPRINT("Call failed\n");
                /* FIXME */
                return MMSYSERR_NOTSUPPORTED;   /* WAVERR_BADFORMAT? */
            }
        }
        else
        {
            /* FIXME */
            DPRINT("Non-PCM format\n");
            return MMSYSERR_NOTSUPPORTED;
        }
    }

    /* If we got this far without error, the format is supported! */
    return MMSYSERR_NOERROR;
}

MMRESULT OpenWaveDevice(
    CHAR device_type,
    DWORD device_id,
    LPWAVEOPENDESC open_details,
    DWORD flags,
    DWORD user_data
)
{
    HANDLE heap = 0;
    PWDMAUD_DEVICE_INFO device = NULL;
    WCHAR* device_path = NULL;
    MMRESULT result = MMSYSERR_ERROR;

    /* ASSERT(open_details); */

    heap = GetProcessHeap();

    if ( ! heap )
    {
        DPRINT1("Couldn't get the process heap (error %d)\n",
                (int) GetLastError());
        result = MMSYSERR_ERROR;
        goto cleanup;
    }

    DPRINT("OpenDevice called\n");

    device_path = (WCHAR*) open_details->dnDevNode;
    device = CreateDeviceData(device_type, device_path);

    if ( ! device )
    {
        DPRINT1("Couldn't create device data\n");
        result = MMSYSERR_NOMEM;
        goto cleanup;
    }

    DPRINT("Allocated device data, allocating device state\n");

    device->state = HeapAlloc(heap,
                              HEAP_ZERO_MEMORY,
                              sizeof(WDMAUD_DEVICE_STATE));

    if ( ! device->state )
    {
        DPRINT1("Couldn't allocate memory for device state (error %d)\n",
                (int) GetLastError());
        result = MMSYSERR_NOMEM;
        goto cleanup;
    }

    /* FIXME: ok here ? */
    device->type = device_type;
    device->id = device_id;
    device->flags = flags;

    if ( flags & WAVE_FORMAT_QUERY )
    {
        DPRINT("Do I support this format? Hmm...\n");

        result = TryOpenDevice(device, open_details->lpFormat);

        if ( result != MMSYSERR_NOERROR )
        {
            DPRINT("Format not supported\n");
            goto cleanup;
        }

        DPRINT("Yes, I do support this format!\n");
    }
    else
    {

        DPRINT("You actually want me to open the device, huh?\n");

        /* Allocate memory for the "queue" critical section */

        device->state->queue_critical_section =
            HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(CRITICAL_SECTION));

        if ( ! device->state->queue_critical_section )
        {
            DPRINT1("Couldn't allocate memory for queue critical section (error %d)\n",
                    (int) GetLastError());
            result = MMSYSERR_NOMEM;
            goto cleanup;
        }

        /* Initialize the critical section */
        InitializeCriticalSection(device->state->queue_critical_section);

        /* We need these so we can contact the client later */
        device->client_instance = open_details->dwInstance;
        device->client_callback = open_details->dwCallback;

        /* Reset state */
        device->state->open_descriptor = NULL;
        device->state->unknown_24 = 0;

        device->state->is_running = FALSE;
        device->state->is_paused =
            device->type == WDMAUD_WAVE_IN ? TRUE : FALSE;

        memcpy(device->state->signature, WDMAUD_DEVICE_STATE_SIG, 4);

        DPRINT("All systems are go...\n");

        result = TryOpenDevice(device, open_details->lpFormat);

        if ( result != MMSYSERR_NOERROR )
        {
            DPRINT1("Format not supported?\n");
            goto cleanup; /* no need to set result - already done */
        }

        /* Enter the critical section while updating the device list */
        EnterCriticalSection(device->state->queue_critical_section);
        /* ... */
        LeaveCriticalSection(device->state->queue_critical_section);

        /* The wave device handle is actually our structure. Neat, eh? */
        open_details->hWave = (HWAVE) device;

        /* We also need to set our "user data" for winmm */
        LPVOID* ud = (LPVOID*) user_data;   /* FIXME */
        *ud = device;

        if (device->client_callback)
        {
            DWORD message;

            message = (device->type == WDMAUD_WAVE_IN ? WIM_OPEN :
                                       WDMAUD_WAVE_OUT ? WOM_OPEN : -1);

            DPRINT("About to call the client callback\n");

            /* Call the callback */
            NotifyClient(device, message, 0, 0);

            DPRINT("...it is done!\n");
        }

        result = MMSYSERR_NOERROR;
    }

    /*
        This cleanup may need checking for memory leakage. It's not very pretty
        to look at, either...
    */

    cleanup :
    {
        if ( ( result != MMSYSERR_NOERROR ) && ( heap ) )
        {
            if ( device )
            {
                if ( device->state )
                {
                    if ( device->state->queue_critical_section )
                    {
                        DeleteCriticalSection(device->state->queue_critical_section);
                        HeapFree(heap, 0, device->state->queue_critical_section);
                    }

                    HeapFree(heap, 0, device->state);
                }

                DeleteDeviceData(device);
            }
        }

        DPRINT("Returning %d\n", (int) result);

        return result;
    }
}

MMRESULT CloseDevice(
    PWDMAUD_DEVICE_INFO device
)
{
    MMRESULT result = MMSYSERR_ERROR;

    DPRINT("CloseDevice()\n");

    DUMP_WDMAUD_DEVICE_INFO(device);

    if ( ValidateDeviceInfo(device) != MMSYSERR_NOERROR )
    {
        DPRINT1("Invalid device info passed to CloseDevice\n");
        result = MMSYSERR_INVALHANDLE;
        goto cleanup;
    }

    /* TODO: Check state! */

    if ( device->id > 0x64 ) /* FIXME ? */
    {
        DPRINT1("??\n");
        goto cleanup;
    }

    switch(device->type)
    {
        case WDMAUD_WAVE_OUT :
        {
            if ( device->state->open_descriptor )
            {
                DPRINT1("Device is still playing!\n");
                result = WAVERR_STILLPLAYING;
                goto cleanup;
            }

            /* TODO: Destroy completion thread */
            
            break;
        }
        
        default :
        {
            DPRINT1("Sorry, device type %d not supported yet!\n", (int) device->type);
            goto cleanup;
        }
    }

    result = CallKernelDevice(device, IOCTL_WDMAUD_CLOSE_DEVICE, 0, 0);

    if ( result != MMSYSERR_NOERROR )
    {
        DPRINT1("Couldn't close the device!\n");
        goto cleanup;
    }

    if (device->client_callback)
    {
        DWORD message;

        message = (device->type == WDMAUD_WAVE_IN ? WIM_CLOSE :
                                   WDMAUD_WAVE_OUT ? WOM_CLOSE :
                                   WDMAUD_MIDI_IN ? MIM_CLOSE :
                                   WDMAUD_MIDI_OUT ? MOM_CLOSE : -1);

        DPRINT("About to call the client callback\n");

        /* Call the callback */
        NotifyClient(device, message, 0, 0);

        DPRINT("...it is done!\n");
    }

    /* Result was set earlier by CallKernelDevice */

    cleanup :
    {
        return result;
    }
}
