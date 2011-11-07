/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            kernel32/file/deviceio.c
 * PURPOSE:         Device I/O Base Client Functionality
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <k32.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
Beep (DWORD dwFreq, DWORD dwDuration)
{
    HANDLE hBeep;
    UNICODE_STRING BeepDevice;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    BEEP_SET_PARAMETERS BeepSetParameters;
    NTSTATUS Status;

    /* check the parameters */
    if ((dwFreq >= 0x25 && dwFreq <= 0x7FFF) ||
        (dwFreq == 0x0 && dwDuration == 0x0))
    {
        /* open the device */
        RtlInitUnicodeString(&BeepDevice,
                             L"\\Device\\Beep");

        InitializeObjectAttributes(&ObjectAttributes,
                                   &BeepDevice,
                                   0,
                                   NULL,
                                   NULL);

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
        if (NT_SUCCESS(Status))
        {
            /* Set beep data */
            BeepSetParameters.Frequency = dwFreq;
            BeepSetParameters.Duration = dwDuration;

            Status = NtDeviceIoControlFile(hBeep,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &IoStatusBlock,
                                           IOCTL_BEEP_SET,
                                           &BeepSetParameters,
                                           sizeof(BEEP_SET_PARAMETERS),
                                           NULL,
                                           0);

            /* do an alertable wait if necessary */
            if (NT_SUCCESS(Status) &&
                (dwFreq != 0x0 || dwDuration != 0x0) && dwDuration != MAXDWORD)
            {
                SleepEx(dwDuration,
                        TRUE);
            }

            NtClose(hBeep);
        }
    }
    else
        Status = STATUS_INVALID_PARAMETER;

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError (Status);
        return FALSE;
    }

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
