/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/client/file/deviceio.c
 * PURPOSE:         Device I/O Base Client Functionality
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#include <ntddbeep.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

VOID
WINAPI
NotifySoundSentry(VOID)
{
    BASE_API_MESSAGE ApiMessage;
    PBASE_SOUND_SENTRY SoundSentryRequest = &ApiMessage.Data.SoundSentryRequest;

    /* Get the video mode */
    if (!GetConsoleDisplayMode(&SoundSentryRequest->VideoMode))
    {
        SoundSentryRequest->VideoMode = 0;
    }

    /* Make sure it's not fullscreen, and send the message if not */
    if (SoundSentryRequest->VideoMode == 0)
    {
        CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                            NULL,
                            CSR_CREATE_API_NUMBER(BASESRV_SERVERDLL_INDEX, BasepSoundSentryNotification),
                            sizeof(*SoundSentryRequest));
    }
}

/*
 * @implemented
 */
BOOL
WINAPI
Beep(IN DWORD dwFreq,
     IN DWORD dwDuration)
{
    HANDLE hBeep;
    UNICODE_STRING BeepDevice;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    BEEP_SET_PARAMETERS BeepSetParameters;
    NTSTATUS Status;

    //
    // On TS systems, we need to Load Winsta.dll and call WinstationBeepOpen
    // after doing a GetProcAddress for it
    //

    /* Open the device */
    RtlInitUnicodeString(&BeepDevice, L"\\Device\\Beep");
    InitializeObjectAttributes(&ObjectAttributes, &BeepDevice, 0, NULL, NULL);
    Status = NtCreateFile(&hBeep,
                          FILE_READ_DATA | FILE_WRITE_DATA,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          0,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN_IF,
                          0,
                          NULL,
                          0);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* check the parameters */
    if ((dwFreq >= 0x25 && dwFreq <= 0x7FFF) ||
        (dwFreq == 0x0 && dwDuration == 0x0))
    {
        /* Set beep data */
        BeepSetParameters.Frequency = dwFreq;
        BeepSetParameters.Duration = dwDuration;

        /* Send the beep */
        Status = NtDeviceIoControlFile(hBeep,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &IoStatusBlock,
                                       IOCTL_BEEP_SET,
                                       &BeepSetParameters,
                                       sizeof(BeepSetParameters),
                                       NULL,
                                       0);
    }
    else
    {
        /* We'll fail the call, but still notify the sound sentry */
        Status = STATUS_INVALID_PARAMETER;
    }

    /* Notify the sound sentry */
    NotifySoundSentry();

    /* Bail out if the hardware beep failed */
    if (!NT_SUCCESS(Status))
    {
        NtClose(hBeep);
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* If an actual beep was emitted, wait for it */
    if (((dwFreq != 0x0) || (dwDuration != 0x0)) && (dwDuration != MAXDWORD))
    {
        SleepEx(dwDuration, TRUE);
    }

    /* Close the handle and return success */
    NtClose(hBeep);
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
DeviceIoControl(IN HANDLE hDevice,
                IN DWORD dwIoControlCode,
                IN LPVOID lpInBuffer OPTIONAL,
                IN DWORD nInBufferSize OPTIONAL,
                OUT LPVOID lpOutBuffer OPTIONAL,
                IN DWORD nOutBufferSize OPTIONAL,
                OUT LPDWORD lpBytesReturned OPTIONAL,
                IN LPOVERLAPPED lpOverlapped OPTIONAL)
{
    BOOL FsIoCtl;
    NTSTATUS Status;
    PVOID ApcContext;
    IO_STATUS_BLOCK Iosb;

    //
    // Note: on a TS Machine, we should call IsTSAppCompatEnabled and unless the
    // IOCTLs are IOCTL_STORAGE_EJECT_MEDIA, IOCTL_DISK_EJECT_MEDIA, FSCTL_DISMOUNT_VOLUME
    // we should call IsCallerAdminOrSystem and return STATUS_ACCESS_DENIED for
    // any other IOCTLs.
    //

    /* Check what kind of IOCTL to send */
    FsIoCtl = ((dwIoControlCode >> 16) == FILE_DEVICE_FILE_SYSTEM);

    /* CHeck for async */
    if (lpOverlapped != NULL)
    {
        /* Set pending status */
        lpOverlapped->Internal = STATUS_PENDING;

        /* Check if there's an APC context */
        ApcContext = (((ULONG_PTR)lpOverlapped->hEvent & 0x1) ? NULL : lpOverlapped);

        /* Send file system control? */
        if (FsIoCtl)
        {
            /* Send it */
            Status = NtFsControlFile(hDevice,
                                     lpOverlapped->hEvent,
                                     NULL,
                                     ApcContext,
                                     (PIO_STATUS_BLOCK)lpOverlapped,
                                     dwIoControlCode,
                                     lpInBuffer,
                                     nInBufferSize,
                                     lpOutBuffer,
                                     nOutBufferSize);
        }
        else
        {
            /* Otherwise send a device control */
            Status = NtDeviceIoControlFile(hDevice,
                                           lpOverlapped->hEvent,
                                           NULL,
                                           ApcContext,
                                           (PIO_STATUS_BLOCK)lpOverlapped,
                                           dwIoControlCode,
                                           lpInBuffer,
                                           nInBufferSize,
                                           lpOutBuffer,
                                           nOutBufferSize);
        }

        /* Check for or information instead of failure */
        if (!(NT_ERROR(Status)) && (lpBytesReturned))
        {
            /* Protect with SEH */
            _SEH2_TRY
            {
                /* Return the bytes */
                *lpBytesReturned = lpOverlapped->InternalHigh;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Return zero bytes */
                *lpBytesReturned = 0;
            }
            _SEH2_END;
        }

        /* Now check for any kind of failure except pending*/
        if (!(NT_SUCCESS(Status)) || (Status == STATUS_PENDING))
        {
            /* Fail */
            BaseSetLastNTError(Status);
            return FALSE;
        }
    }
    else
    {
        /* Sync case -- send file system code? */
        if (FsIoCtl)
        {
            /* Do it */
            Status = NtFsControlFile(hDevice,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &Iosb,
                                     dwIoControlCode,
                                     lpInBuffer,
                                     nInBufferSize,
                                     lpOutBuffer,
                                     nOutBufferSize);
        }
        else
        {
            /* Send device code instead */
            Status = NtDeviceIoControlFile(hDevice,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &Iosb,
                                           dwIoControlCode,
                                           lpInBuffer,
                                           nInBufferSize,
                                           lpOutBuffer,
                                           nOutBufferSize);
        }

        /* Now check if the operation isn't done yet */
        if (Status == STATUS_PENDING)
        {
            /* Wait for it and get the final status */
            Status = NtWaitForSingleObject(hDevice, FALSE, NULL);
            if (NT_SUCCESS(Status)) Status = Iosb.Status;
        }

        /* Check for success */
        if (NT_SUCCESS(Status))
        {
            /* Return the byte count */
            *lpBytesReturned = Iosb.Information;
        }
        else
        {
            /* Check for informational or warning failure */
            if (!NT_ERROR(Status)) *lpBytesReturned = Iosb.Information;

            /* Return a failure */
            BaseSetLastNTError(Status);
            return FALSE;
        }
    }

    /* Return success */
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
CancelIo(IN HANDLE hFile)
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    Status = NtCancelIoFile(hFile, &IoStatusBlock);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/* EOF */
