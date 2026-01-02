/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/file/volumeansi.c
 * PURPOSE:         Volume functions (ANSI)
 */

/* INCLUDES *****************************************************************/

#include <k32.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
GetVolumePathNameA(IN LPCSTR lpszFileName,
                   IN LPSTR lpszVolumePathName,
                   IN DWORD cchBufferLength)
{
    BOOL Ret;
    PUNICODE_STRING FileNameU;
    ANSI_STRING VolumePathName;
    UNICODE_STRING VolumePathNameU;

    /* Convert file name to unicode */
    FileNameU = Basep8BitStringToStaticUnicodeString(lpszFileName);
    if (FileNameU == NULL)
    {
        return FALSE;
    }

    /* Initialize all the strings we'll need */
    VolumePathName.Buffer = lpszVolumePathName;
    VolumePathName.Length = 0;
    VolumePathName.MaximumLength = cchBufferLength - 1;

    VolumePathNameU.Length = 0;
    VolumePathNameU.MaximumLength = (cchBufferLength - 1) * sizeof(WCHAR) + sizeof(UNICODE_NULL);
    /* Allocate a buffer for calling the -W */
    VolumePathNameU.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, VolumePathNameU.MaximumLength);
    if (VolumePathNameU.Buffer == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Call the -W implementation */
    Ret = GetVolumePathNameW(FileNameU->Buffer, VolumePathNameU.Buffer, cchBufferLength);
    /* If it succeed */
    if (Ret)
    {
        NTSTATUS Status;

        /* Convert back to ANSI */
        RtlInitUnicodeString(&VolumePathNameU, VolumePathNameU.Buffer);
        Status = RtlUnicodeStringToAnsiString(&VolumePathName, &VolumePathNameU, FALSE);
        /* If conversion failed, just set error code and fail the rest */
        if (!NT_SUCCESS(Status))
        {
            BaseSetLastNTError(Status);
            Ret = FALSE;
        }
        /* Otherwise, null terminate the string (it's OK, we computed -1) */
        else
        {
            VolumePathName.Buffer[VolumePathName.Length] = ANSI_NULL;
        }
    }

    /* Free the buffer allocated for -W call */
    RtlFreeHeap(RtlGetProcessHeap(), 0, VolumePathNameU.Buffer);
    return Ret;
}

/* EOF */
