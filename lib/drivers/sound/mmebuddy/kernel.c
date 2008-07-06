/*
    ReactOS Sound System
    MME Driver Helper

    Purpose:
        Kernel device I/O

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        4 July 2008 - Created
*/

#include <windows.h>
#include <mmsystem.h>
#include <ntddsnd.h>
#include <debug.h>

#include <mmebuddy.h>


MMRESULT
OpenKernelSoundDeviceByName(
    PWSTR DeviceName,
    DWORD AccessRights,
    PHANDLE Handle)
{
    DWORD OpenFlags = 0;

    if ( ! Handle )
    {
        MessageBox(0, L"Failed handle", L"kernel.c", MB_OK | MB_TASKMODAL);
        return MMSYSERR_INVALPARAM;
    }

    if ( ! DeviceName )
    {
        MessageBox(0, L"Failed devname", L"kernel.c", MB_OK | MB_TASKMODAL);
        return MMSYSERR_INVALPARAM;
    }

    if ( AccessRights != GENERIC_READ )
    {
        OpenFlags = FILE_FLAG_OVERLAPPED;
    }

    DPRINT("Attempting to open '%ws'\n", DeviceName);
    MessageBox(0, DeviceName, L"Attempting to open", MB_OK | MB_TASKMODAL);

    *Handle = CreateFile(DeviceName,
                         AccessRights,
                         FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         OpenFlags,
                         NULL);

    if ( *Handle == INVALID_HANDLE_VALUE )
    {
        return Win32ErrorToMmResult(GetLastError());
    }

    return MMSYSERR_NOERROR;
}

MMRESULT
OpenKernelSoundDevice(
    PSOUND_DEVICE SoundDevice,
    DWORD AccessRights)
{
    MMRESULT Result;

    if ( ! SoundDevice )
    {
        SOUND_DEBUG(L"No sound device specified");
        return MMSYSERR_INVALPARAM;
    }

    if ( SoundDevice->Handle != INVALID_HANDLE_VALUE )
    {
        SOUND_DEBUG(L"Already open?");
        return MMSYSERR_ERROR; /*MMSYSERR_ALLOC;*/
    }

    Result = OpenKernelSoundDeviceByName(SoundDevice->DevicePath,
                                         AccessRights,
                                         &SoundDevice->Handle);

    return Result;
}

MMRESULT
CloseKernelSoundDevice(
    PSOUND_DEVICE SoundDevice)
{
    if ( ! SoundDevice )
        return MMSYSERR_INVALPARAM;

    if ( SoundDevice->Handle == INVALID_HANDLE_VALUE )
        return MMSYSERR_ERROR;  /* ok? */

    CloseHandle(SoundDevice->Handle);
    SoundDevice->Handle = INVALID_HANDLE_VALUE;

    return MMSYSERR_NOERROR;
}

MMRESULT
PerformSoundDeviceIo(
    PSOUND_DEVICE SoundDevice,
    DWORD IoControlCode,
    LPVOID InBuffer,
    DWORD InBufferSize,
    LPVOID OutBuffer,
    DWORD OutBufferSize,
    LPDWORD BytesReturned,
    LPOVERLAPPED Overlapped)
{
    BOOLEAN TemporaryOpen = FALSE;
    BOOLEAN IoResult = FALSE;
    DWORD AccessRights = GENERIC_READ;
    MMRESULT Result;

    if ( ! SoundDevice )
        return MMSYSERR_INVALPARAM;

    /* Determine if we actually need to write stuff */
    if ( InBuffer != NULL )
        AccessRights |= GENERIC_WRITE;

    /* Open the device temporarily,if it's not open */
    TemporaryOpen = (SoundDevice->Handle == INVALID_HANDLE_VALUE);

    if ( TemporaryOpen )
    {
        MessageBox(0, L"Opening sound device", L"Info", MB_OK | MB_TASKMODAL);

        Result = OpenKernelSoundDevice(SoundDevice, AccessRights);

        if ( Result != MMSYSERR_NOERROR )
            return Result;
    }

    MessageBox(0, L"Doing IO", L"Info", MB_OK | MB_TASKMODAL);
    IoResult = DeviceIoControl(
        SoundDevice->Handle,
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

    /* If we opened the device, we must close it here */
    if ( TemporaryOpen )
    {
        MessageBox(0, L"Closing sound device", L"Info", MB_OK | MB_TASKMODAL);
        CloseHandle(SoundDevice->Handle);
        SoundDevice->Handle = INVALID_HANDLE_VALUE;
    }

    return MMSYSERR_NOERROR;
}

MMRESULT
ReadSoundDevice(
    PSOUND_DEVICE SoundDevice,
    DWORD IoControlCode,
    LPVOID OutBuffer,
    DWORD OutBufferSize,
    LPDWORD BytesReturned,
    LPOVERLAPPED Overlapped)
{
    return PerformSoundDeviceIo(SoundDevice,
                                IoControlCode,
                                NULL,
                                0,
                                OutBuffer,
                                OutBufferSize,
                                BytesReturned,
                                Overlapped);
}

MMRESULT
WriteSoundDevice(
    PSOUND_DEVICE SoundDevice,
    DWORD IoControlCode,
    LPVOID InBuffer,
    DWORD InBufferSize,
    LPDWORD BytesReturned,
    LPOVERLAPPED Overlapped)
{
    return PerformSoundDeviceIo(SoundDevice,
                                IoControlCode,
                                InBuffer,
                                InBufferSize,
                                NULL,
                                0,
                                BytesReturned,
                                Overlapped);
}

MMRESULT
WriteSoundDeviceBuffer(
    PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    LPVOID Buffer,
    DWORD BufferSize,
    LPOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine)
{
    WCHAR msg[128];
    if ( ( ! SoundDeviceInstance ) || ( ! Buffer ) || ( BufferSize == 0 ) )
        return MMSYSERR_INVALPARAM;

    ZeroMemory(&SoundDeviceInstance->Overlapped, sizeof(OVERLAPPED));

    wsprintf(msg, L"Writing to handle %x", SoundDeviceInstance->Device->Handle);
    SOUND_DEBUG(msg);

    if ( ! WriteFileEx(SoundDeviceInstance->Device->Handle,
                       Buffer,
                       BufferSize,
                       &SoundDeviceInstance->Overlapped,
                       CompletionRoutine) )
    {
        wsprintf(msg, L"Win32 Error %d", GetLastError());
        SOUND_DEBUG(msg);
        return Win32ErrorToMmResult(GetLastError());
    }

    return MMSYSERR_NOERROR;
}
