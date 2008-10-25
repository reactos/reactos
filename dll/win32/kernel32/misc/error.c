/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/misc/error.c
 * PURPOSE:         Environment functions
 * PROGRAMMER:      Emanuele Aliberti
 *                  Thomas Weidenmueller
 * UPDATE HISTORY:
 *                  Created 05/10/98
 */


#include <k32.h>

#define NDEBUG
#include <debug.h>


/*
 * @implemented
 */
VOID
STDCALL
SetLastError (DWORD dwErrorCode)
{
    NtCurrentTeb ()->LastErrorValue = (ULONG) dwErrorCode;
}


/*
 * @implemented
 */
DWORD
STDCALL
GetLastError (VOID)
{
    return (DWORD) (NtCurrentTeb ()->LastErrorValue);
}


/*
 * @implemented
 */
BOOL
STDCALL
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
                (dwFreq != 0x0 || dwDuration != 0x0) && dwDuration != (DWORD)-1)
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
        SetLastErrorByStatus (Status);
        return FALSE;
    }

    return TRUE;
}

/* EOF */
