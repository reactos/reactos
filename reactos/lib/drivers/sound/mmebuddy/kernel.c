/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/drivers/sound/mmebuddy/kernel.c
 *
 * PURPOSE:     Routines assisting with device I/O between user-mode and
 *              kernel-mode.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <ntddsnd.h>
#include <mmebuddy.h>

/*
    Wraps around CreateFile in order to provide a simpler interface tailored
    towards sound driver support code. This simply takes a device path and
    opens the device in either read-only mode, or read/write mode (depending on
    the ReadOnly parameter).

    If the device is opened in read/write mode, it is opened for overlapped I/O.
*/
MMRESULT
OpenKernelSoundDeviceByName(
    IN  PWSTR DevicePath,
    IN  BOOLEAN ReadOnly,
    OUT PHANDLE Handle)
{
    DWORD AccessRights;

    VALIDATE_MMSYS_PARAMETER( DevicePath );
    VALIDATE_MMSYS_PARAMETER( Handle );

    AccessRights = ReadOnly ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE;

    SND_TRACE(L"OpenKernelSoundDeviceByName: %wS\n", DevicePath);
    *Handle = CreateFile(DevicePath,
                         AccessRights,
                         FILE_SHARE_WRITE,  /* FIXME? Should be read also? */
                         NULL,
                         OPEN_EXISTING,
                         ReadOnly ? 0 : FILE_FLAG_OVERLAPPED,
                         NULL);

    if ( *Handle == INVALID_HANDLE_VALUE )
    {
        SND_ERR(L"CreateFile filed - winerror %d\n", GetLastError());
        return Win32ErrorToMmResult(GetLastError());
    }

    return MMSYSERR_NOERROR;
}


/*
    Just a wrapped around CloseHandle.
*/
MMRESULT
CloseKernelSoundDevice(
    IN  HANDLE Handle)
{
    VALIDATE_MMSYS_PARAMETER( Handle );

    CloseHandle(Handle);

    return MMSYSERR_NOERROR;
}

/*
    This is a wrapper around DeviceIoControl which provides control over
    instantiated sound devices. It waits for I/O to complete (since an
    instantiated sound device is opened in overlapped mode, this is necessary).
*/
MMRESULT
SyncOverlappedDeviceIoControl(
    IN  HANDLE Handle,
    IN  DWORD IoControlCode,
    IN  LPVOID InBuffer,
    IN  DWORD InBufferSize,
    OUT LPVOID OutBuffer,
    IN  DWORD OutBufferSize,
    OUT LPDWORD BytesTransferred OPTIONAL)
{
    OVERLAPPED Overlapped;
    BOOLEAN IoResult;
    DWORD Transferred;

    /* Overlapped I/O is done here - this is used for waiting for completion */
    ZeroMemory(&Overlapped, sizeof(OVERLAPPED));
    Overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if ( ! Overlapped.hEvent )
        return Win32ErrorToMmResult(GetLastError());

    /* Talk to the device */
    IoResult = DeviceIoControl(Handle,
                               IoControlCode,
                               InBuffer,
                               InBufferSize,
                               OutBuffer,
                               OutBufferSize,
                               NULL,
                               &Overlapped);

    /* If failure occurs, make sure it's not just due to the overlapped I/O */
    if ( ! IoResult )
    {
        if ( GetLastError() != ERROR_IO_PENDING )
        {
            CloseHandle(Overlapped.hEvent);
            return Win32ErrorToMmResult(GetLastError());
        }
    }

    /* Wait for the I/O to complete */
    IoResult = GetOverlappedResult(Handle,
                                   &Overlapped,
                                   &Transferred,
                                   TRUE);

    /* Don't need this any more */
    CloseHandle(Overlapped.hEvent);

    if ( ! IoResult )
        return Win32ErrorToMmResult(GetLastError());

    if ( BytesTransferred )
        *BytesTransferred = Transferred;

    return MMSYSERR_NOERROR;
}
