/*
 * PROJECT:     ReactOS Sound System "MME Buddy" Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/sound/mmebuddy/kernel.c
 *
 * PURPOSE:     Routines assisting with device I/O between user-mode and
 *              kernel-mode.
 *
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
*/

#include <windows.h>
#include <mmsystem.h>
#include <ntddsnd.h>
#include <mmebuddy.h>

MMRESULT
OpenKernelSoundDeviceByName(
    IN  PWSTR DeviceName,
    IN  BOOLEAN ReadOnly,
    OUT PHANDLE Handle)
{
    DWORD AccessRights;

    VALIDATE_MMSYS_PARAMETER( DeviceName );
    VALIDATE_MMSYS_PARAMETER( Handle );

    AccessRights = ReadOnly ? GENERIC_READ : GENERIC_WRITE;

    *Handle = CreateFile(DeviceName,
                         AccessRights,
                         FILE_SHARE_WRITE,  /* FIXME? Should be read also? */
                         NULL,
                         OPEN_EXISTING,
                         ReadOnly ? 0 : FILE_FLAG_OVERLAPPED,
                         NULL);

    if ( *Handle == INVALID_HANDLE_VALUE )
    {
        return Win32ErrorToMmResult(GetLastError());
    }

    return MMSYSERR_NOERROR;
}

MMRESULT
CloseKernelSoundDevice(
    IN  HANDLE Handle)
{
    VALIDATE_MMSYS_PARAMETER( Handle );

    CloseHandle(Handle);

    return MMSYSERR_NOERROR;
}

MMRESULT
SoundDeviceIoControl(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
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

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceInstance(SoundDeviceInstance) );

    /* Overlapped I/O is done here - this is used for waiting for completion */
    ZeroMemory(&Overlapped, sizeof(OVERLAPPED));
    Overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if ( ! Overlapped.hEvent )
        return Win32ErrorToMmResult(GetLastError());

    /* Talk to the device */
    IoResult = DeviceIoControl(SoundDeviceInstance->KernelDeviceHandle,
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
    IoResult = GetOverlappedResult(SoundDeviceInstance->KernelDeviceHandle,
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
