/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/create.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include <k32.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(kernel32file);

/* GLOBALS ******************************************************************/

LONG ProcessPipeId = 0;

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
BOOL
STDCALL
CreatePipe(PHANDLE hReadPipe,
           PHANDLE hWritePipe,
           LPSECURITY_ATTRIBUTES lpPipeAttributes,
           DWORD nSize)
{
    WCHAR Buffer[64];
    UNICODE_STRING PipeName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK StatusBlock;
    LARGE_INTEGER DefaultTimeout;
    NTSTATUS Status;
    HANDLE ReadPipeHandle;
    HANDLE WritePipeHandle;
    LONG PipeId;
    ULONG Attributes;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;

    /* Set the timeout to 120 seconds */
    DefaultTimeout.QuadPart = -1200000000;

    /* Use default buffer size if desired */
    if (!nSize) nSize = 0x1000;

    /* Increase the Pipe ID */
    PipeId = InterlockedIncrement(&ProcessPipeId);

    /* Create the pipe name */
    swprintf(Buffer,
             L"\\Device\\NamedPipe\\Win32Pipes.%08x.%08x",
             NtCurrentTeb()->ClientId.UniqueProcess,
             PipeId);
    RtlInitUnicodeString(&PipeName, Buffer);

    /* Always use case insensitive */
    Attributes = OBJ_CASE_INSENSITIVE;

    /* Check if we got attributes */
    if (lpPipeAttributes)
    {
        /* Use the attributes' SD instead */
        SecurityDescriptor = lpPipeAttributes->lpSecurityDescriptor;

        /* Set OBJ_INHERIT if requested */
        if (lpPipeAttributes->bInheritHandle) Attributes |= OBJ_INHERIT;
    }

    /* Initialize the attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               &PipeName,
                               Attributes,
                               NULL,
                               SecurityDescriptor);

    /* Create the named pipe */
    Status = NtCreateNamedPipeFile(&ReadPipeHandle,
                                   GENERIC_READ |FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                                   &ObjectAttributes,
                                   &StatusBlock,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   FILE_CREATE,
                                   FILE_SYNCHRONOUS_IO_NONALERT,
                                   FILE_PIPE_BYTE_STREAM_TYPE,
                                   FILE_PIPE_BYTE_STREAM_MODE,
                                   FILE_PIPE_QUEUE_OPERATION,
                                   1,
                                   nSize,
                                   nSize,
                                   &DefaultTimeout);
    if (!NT_SUCCESS(Status))
    {
        /* Convert error and fail */
        WARN("Status: %lx\n", Status);
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    /* Now try opening it for write access */
    Status = NtOpenFile(&WritePipeHandle,
                        FILE_GENERIC_WRITE | SYNCHRONIZE,
                        &ObjectAttributes,
                        &StatusBlock,
                        FILE_SHARE_READ,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
    if (!NT_SUCCESS(Status))
    {
        /* Convert error and fail */
        WARN("Status: %lx\n", Status);
        NtClose(ReadPipeHandle);
        SetLastErrorByStatus(Status);
        return FALSE;
    }

    /* Return both handles */
    *hReadPipe = ReadPipeHandle;
    *hWritePipe = WritePipeHandle;
    return TRUE;
}

/* EOF */
