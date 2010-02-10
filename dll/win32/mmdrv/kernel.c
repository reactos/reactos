/*
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS Multimedia
 * FILE:                 dll/win32/mmdrv/kernel.c
 * PURPOSE:              Multimedia User Mode Driver (kernel interface)
 * PROGRAMMER:           Andrew Greenwood
 * UPDATE HISTORY:
 *                       Jan 14, 2007: Created
 */

#include <mmdrv.h>

/*
    Devices that we provide access to follow a standard naming convention.
    The first wave output, for example, appears as \Device\WaveOut0

    I'm not entirely certain how drivers find a free name to use, or why
    we need to strip the leading \Device from it when opening, but hey...
*/

MMRESULT
CobbleDeviceName(
    DeviceType device_type,
    UINT device_id,
    PWCHAR out_device_name)
{
    WCHAR base_device_name[MAX_DEVICE_NAME_LENGTH];

    /* Work out the base name from the device type */

    switch ( device_type )
    {
        case WaveOutDevice :
            wsprintf(base_device_name, L"%ls", WAVE_OUT_DEVICE_NAME);
            break;

        case WaveInDevice :
            wsprintf(base_device_name, L"%ls", WAVE_IN_DEVICE_NAME);
            break;

        case MidiOutDevice :
            wsprintf(base_device_name, L"%ls", MIDI_OUT_DEVICE_NAME);
            break;

        case MidiInDevice :
            wsprintf(base_device_name, L"%ls", MIDI_IN_DEVICE_NAME);
            break;

        case AuxDevice :
            wsprintf(base_device_name, L"%ls", AUX_DEVICE_NAME);
            break;

        default :
            return MMSYSERR_BADDEVICEID;
    };

    /* Now append the device number, removing the leading \Device */

    wsprintf(out_device_name,
             L"\\\\.%ls%d",
             base_device_name + strlen("\\Device"),
             device_id);

    return MMSYSERR_NOERROR;
}


/*
    Takes a device type (eg: WaveOutDevice), a device ID, desired access and
    a pointer to a location that will store the handle of the opened "file" if
    the function succeeds.

    The device type and ID are converted into a device name using the above
    function.
*/

MMRESULT
OpenKernelDevice(
    DeviceType device_type,
    UINT device_id,
    DWORD access,
    HANDLE* handle)
{
    MMRESULT result;
    WCHAR device_name[MAX_DEVICE_NAME_LENGTH];
    DWORD open_flags = 0;

    ASSERT(handle);

    /* Glue the base device name and the ID together */

    result = CobbleDeviceName(device_type, device_id, device_name);

    DPRINT("Opening kernel device %ls\n", device_name);

    if ( result != MMSYSERR_NOERROR )
        return result;

    /* We want overlapped I/O when writing */

    if ( access != GENERIC_READ )
        open_flags = FILE_FLAG_OVERLAPPED;

    /* Now try opening... */

    *handle = CreateFile(device_name,
                         access,
                         FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         open_flags,
                         NULL);

    if ( *handle == INVALID_HANDLE_VALUE )
        return ErrorToMmResult(GetLastError());

    return MMSYSERR_NOERROR;
}


/*
    Just an alias for the benefit of having a pair of functions ;)
*/

void
CloseKernelDevice(HANDLE device_handle)
{
    CloseHandle(device_handle);
}


MMRESULT
SetDeviceData(
    HANDLE device_handle,
    DWORD ioctl,
    PBYTE input_buffer,
    DWORD buffer_size)
{
    DPRINT("SetDeviceData\n");
    /* TODO */
    return 0;
}


MMRESULT
GetDeviceData(
    HANDLE device_handle,
    DWORD ioctl,
    PBYTE output_buffer,
    DWORD buffer_size)
{
    OVERLAPPED overlap;
    DWORD bytes_returned;
    BOOL success;
    DWORD transfer;

    DPRINT("GetDeviceData\n");

    memset(&overlap, 0, sizeof(overlap));

    overlap.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if ( ! overlap.hEvent )
        return MMSYSERR_NOMEM;

    success = DeviceIoControl(device_handle,
                              ioctl,
                              NULL,
                              0,
                              output_buffer,
                              buffer_size,
                              &bytes_returned,
                              &overlap);

    if ( ! success )
    {
        if ( GetLastError() == ERROR_IO_PENDING )
        {
            if ( ! GetOverlappedResult(device_handle, &overlap, &transfer, TRUE) )
            {
                CloseHandle(overlap.hEvent);
                return ErrorToMmResult(GetLastError());
            }
        }
        else
        {
            CloseHandle(overlap.hEvent);
            return ErrorToMmResult(GetLastError());
        }
    }

    while ( TRUE )
    {
        SetEvent(overlap.hEvent);

        if ( WaitForSingleObjectEx(overlap.hEvent, 0, TRUE) != WAIT_IO_COMPLETION )
        {
            break;
        }
    }

    CloseHandle(overlap.hEvent);

    return MMSYSERR_NOERROR;
}
