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
    PWSTR DeviceName,
    DWORD AccessRights,
    PHANDLE Handle)
{
    DWORD OpenFlags = 0;

    VALIDATE_MMSYS_PARAMETER( Handle );
    VALIDATE_MMSYS_PARAMETER( DeviceName );

    if ( AccessRights != GENERIC_READ )
    {
        OpenFlags = FILE_FLAG_OVERLAPPED;
    }

    /*TRACE_("Attempting to open '%ws'\n", DeviceName);*/

    *Handle = CreateFile(DeviceName,
                         AccessRights,
                         FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         OpenFlags,
                         NULL);

    if ( *Handle == INVALID_HANDLE_VALUE )
    {
        ERR_("Failed to open\n");
        return Win32ErrorToMmResult(GetLastError());
    }

    return MMSYSERR_NOERROR;
}

MMRESULT
OpenKernelSoundDevice(
    PSOUND_DEVICE SoundDevice,
    DWORD AccessRights,
    PHANDLE Handle)
{
    MMRESULT Result;

    VALIDATE_MMSYS_PARAMETER( IsValidSoundDevice(SoundDevice) );
    VALIDATE_MMSYS_PARAMETER( Handle );

    Result = OpenKernelSoundDeviceByName(SoundDevice->DevicePath,
                                         AccessRights,
                                         Handle);

    Result = TranslateInternalMmResult(Result);
    return Result;
}

MMRESULT
CloseKernelSoundDevice(
    HANDLE Handle)
{
    if ( Handle == INVALID_HANDLE_VALUE )
        return MMSYSERR_INVALPARAM;

    CloseHandle(Handle);

    return MMSYSERR_NOERROR;
}

MMRESULT
PerformDeviceIo(
    IN  HANDLE Handle,
    IN  DWORD IoControlCode,
    IN  LPVOID InBuffer,
    IN  DWORD InBufferSize,
    OUT LPVOID OutBuffer,
    IN  DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    IN  LPOVERLAPPED Overlapped)
{
    BOOLEAN IoResult = FALSE;
    DWORD AccessRights = GENERIC_READ;

    VALIDATE_MMSYS_PARAMETER( Handle != INVALID_HANDLE_VALUE );

    /* Determine if we actually need to write stuff */
    if ( InBuffer != NULL )
        AccessRights |= GENERIC_WRITE;

    MessageBox(0, L"Doing IO", L"Info", MB_OK | MB_TASKMODAL);
    IoResult = DeviceIoControl(
        Handle,
        IoControlCode,
        InBuffer,
        InBufferSize,
        OutBuffer,
        OutBufferSize,
        BytesReturned,
        Overlapped);

    if ( ! IoResult )
    {
        return Win32ErrorToMmResult(GetLastError());
    }

    return MMSYSERR_NOERROR;
}

MMRESULT
ReadFromDeviceHandle(
    IN  HANDLE Handle,
    IN  DWORD IoControlCode,
    OUT LPVOID OutBuffer,
    IN  DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    IN  LPOVERLAPPED Overlapped)
{
    return PerformDeviceIo(Handle,
                           IoControlCode,
                           NULL,
                           0,
                           OutBuffer,
                           OutBufferSize,
                           BytesReturned,
                           Overlapped);
}

MMRESULT
WriteToDeviceHandle(
    IN  HANDLE Handle,
    IN  DWORD IoControlCode,
    IN  LPVOID InBuffer,
    IN  DWORD InBufferSize,
    OUT LPDWORD BytesReturned,
    IN  LPOVERLAPPED Overlapped)
{
    return PerformDeviceIo(Handle,
                           IoControlCode,
                           InBuffer,
                           InBufferSize,
                           NULL,
                           0,
                           BytesReturned,
                           Overlapped);
}

MMRESULT
PerformSoundDeviceIo(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  DWORD IoControlCode,
    IN  LPVOID InBuffer,
    IN  DWORD InBufferSize,
    OUT LPVOID OutBuffer,
    IN  DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    IN  LPOVERLAPPED Overlapped)
{
    if ( ! IsValidSoundDeviceInstance(SoundDeviceInstance) )
        return MMSYSERR_INVALPARAM;

    if ( SoundDeviceInstance->Handle == INVALID_HANDLE_VALUE )
        return MMSYSERR_ERROR;

    return PerformDeviceIo(SoundDeviceInstance->Handle,
                           IoControlCode,
                           InBuffer,
                           InBufferSize,
                           OutBuffer,
                           OutBufferSize,
                           BytesReturned,
                           Overlapped);
}

MMRESULT
ReadFromSoundDevice(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  DWORD IoControlCode,
    OUT LPVOID OutBuffer,
    IN  DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    IN  LPOVERLAPPED Overlapped)
{
    return PerformSoundDeviceIo(SoundDeviceInstance,
                                IoControlCode,
                                NULL,
                                0,
                                OutBuffer,
                                OutBufferSize,
                                BytesReturned,
                                Overlapped);
}

MMRESULT
WriteToSoundDevice(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  DWORD IoControlCode,
    IN  LPVOID InBuffer,
    IN  DWORD InBufferSize,
    OUT LPDWORD BytesReturned,
    IN  LPOVERLAPPED Overlapped)
{
    return PerformSoundDeviceIo(SoundDeviceInstance,
                                IoControlCode,
                                InBuffer,
                                InBufferSize,
                                NULL,
                                0,
                                BytesReturned,
                                Overlapped);
}


/* TODO: move somewhere else */
MMRESULT
WriteSoundDeviceBuffer(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  LPVOID Buffer,
    IN  DWORD BufferSize,
    IN  LPOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine,
    IN  LPOVERLAPPED Overlapped)
{
    VALIDATE_MMSYS_PARAMETER( IsValidSoundDeviceInstance(SoundDeviceInstance) );
    VALIDATE_MMSYS_PARAMETER( Buffer );
    VALIDATE_MMSYS_PARAMETER( BufferSize > 0 );
    
    /*wsprintf(msg, L"Writing to handle %x", SoundDeviceInstance->Device->Handle);*/
    /*SOUND_DEBUG(msg);*/

    TRACE_("WriteFileEx(%p, %p, %d, %p, %p)\n", 
           SoundDeviceInstance->Handle,
           Buffer,
           (int) BufferSize,
           Overlapped,
           CompletionRoutine);

    if ( ! WriteFileEx(SoundDeviceInstance->Handle,
                       Buffer,
                       BufferSize,
                       Overlapped,
                       CompletionRoutine) )
    {
        ERR_("WriteFileEx -- Win32 Error %d", (int) GetLastError());
        return Win32ErrorToMmResult(GetLastError());
    }

    return MMSYSERR_NOERROR;
}
