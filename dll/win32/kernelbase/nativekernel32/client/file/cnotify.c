/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/file/cnotify.c
 * PURPOSE:         Find functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES *******************************************************************/

#include <k32.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

CHAR staticchangebuff[sizeof(FILE_NOTIFY_INFORMATION) + 16];
IO_STATUS_BLOCK staticIoStatusBlock;

/* PRIVATE FUNCTIONS **********************************************************/

VOID
WINAPI
BasepIoCompletion(IN PVOID ApcContext,
                  IN PIO_STATUS_BLOCK IoStatusBlock,
                  IN DWORD Reserved)
{
    PBASEP_ACTCTX_BLOCK ActivationBlock = ApcContext;
    LPOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine;
    DWORD BytesTransfered, Result;
    RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME ActCtx;
    PVOID ActivationContext = NULL;

    /* Setup the activation frame */
    RtlZeroMemory(&ActCtx, sizeof(ActCtx));
    ActCtx.Size = sizeof(ActCtx);
    ActCtx.Format = RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_FORMAT_WHISTLER;

    /* Check if the routine returned an error */
    if (NT_ERROR(IoStatusBlock->Status))
    {
        /* Convert the error code and don't copy anything */
        Result = RtlNtStatusToDosError(IoStatusBlock->Status);
        BytesTransfered = 0;
    }
    else
    {
        /* Set success code and copy the bytes transferred */
        Result = ERROR_SUCCESS;
        BytesTransfered = IoStatusBlock->Information;
    }

    /* Read context and routine out from the activation block */
    ActivationContext = ActivationBlock->ActivationContext;
    CompletionRoutine = ActivationBlock->CompletionRoutine;

    /* Check if the block should be freed */
    if (!(ActivationBlock->Flags & 1))
    {
        /* Free it */
        BasepFreeActivationContextActivationBlock(ActivationBlock);
    }

    /* Activate the context, call the routine, and then deactivate the context */
    RtlActivateActivationContextUnsafeFast(&ActCtx, ActivationContext);
    CompletionRoutine(Result, BytesTransfered, (LPOVERLAPPED)IoStatusBlock);
    RtlDeactivateActivationContextUnsafeFast(&ActCtx);
}

VOID
WINAPI
BasepIoCompletionSimple(IN PVOID ApcContext,
                        IN PIO_STATUS_BLOCK IoStatusBlock,
                        IN DWORD Reserved)
{
    LPOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine = ApcContext;
    DWORD Result, BytesTransfered;

    /* Check if the routine returned an error */
    if (NT_ERROR(IoStatusBlock->Status))
    {
        /* Convert the error code and don't copy anything */
        Result = RtlNtStatusToDosError(IoStatusBlock->Status);
        BytesTransfered = 0;
    }
    else
    {
        /* Set success code and copy the bytes transferred */
        Result = ERROR_SUCCESS;
        BytesTransfered = IoStatusBlock->Information;
    }

    /* Call the callback routine */
    CompletionRoutine(Result, BytesTransfered, (LPOVERLAPPED)IoStatusBlock);
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
FindCloseChangeNotification(IN HANDLE hChangeHandle)
{
    /* Just close the handle */
    return CloseHandle(hChangeHandle);
}

/*
 * @implemented
 */
HANDLE
WINAPI
FindFirstChangeNotificationA(IN LPCSTR lpPathName,
                             IN BOOL bWatchSubtree,
                             IN DWORD dwNotifyFilter)
{
    /* Call the W(ide) function */
    ConvertWin32AnsiChangeApiToUnicodeApi(FindFirstChangeNotification,
                                          lpPathName,
                                          bWatchSubtree,
                                          dwNotifyFilter);
}

/*
 * @implemented
 */
HANDLE
WINAPI
FindFirstChangeNotificationW(IN LPCWSTR lpPathName,
                             IN BOOL bWatchSubtree,
                             IN DWORD dwNotifyFilter)
{
    NTSTATUS Status;
    UNICODE_STRING NtPathU;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hDir;
    RTL_RELATIVE_NAME_U RelativeName;
    PWCHAR PathBuffer;
    IO_STATUS_BLOCK IoStatusBlock;

    /* Convert to NT path and get the relative name too */
    if (!RtlDosPathNameToNtPathName_U(lpPathName,
                                      &NtPathU,
                                      NULL,
                                      &RelativeName))
    {
        /* Bail out if the path name makes no sense */
        SetLastError(ERROR_PATH_NOT_FOUND);
        return INVALID_HANDLE_VALUE;
    }

    /* Save the path buffer in case we free it later */
    PathBuffer = NtPathU.Buffer;

    /* If we have a relative name... */
    if (RelativeName.RelativeName.Length)
    {
        /* Do a relative open with only the relative path set */
        NtPathU = RelativeName.RelativeName;
    }
    else
    {
        /* Do a full path open with no containing directory */
        RelativeName.ContainingDirectory = NULL;
    }

    /* Now open the directory name that was passed in */
    InitializeObjectAttributes(&ObjectAttributes,
                               &NtPathU,
                               OBJ_CASE_INSENSITIVE,
                               RelativeName.ContainingDirectory,
                               NULL);
    Status = NtOpenFile(&hDir,
                        SYNCHRONIZE | FILE_LIST_DIRECTORY,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT);

    /* Release our buffer and relative name structure */
    RtlReleaseRelativeName(&RelativeName);
    RtlFreeHeap(RtlGetProcessHeap(), 0, PathBuffer);

    /* Check if the open failed */
    if (!NT_SUCCESS(Status))
    {
        /* Bail out in that case */
        BaseSetLastNTError(Status);
        return INVALID_HANDLE_VALUE;
    }

    /* Now setup the notification on the directory as requested */
    Status = NtNotifyChangeDirectoryFile(hDir,
                                         NULL,
                                         NULL,
                                         NULL,
                                         &staticIoStatusBlock,
                                         staticchangebuff,
                                         sizeof(staticchangebuff),
                                         dwNotifyFilter,
                                         (BOOLEAN)bWatchSubtree);
    if (!NT_SUCCESS(Status))
    {
        /* We failed, close the handle and convert the error */
        NtClose(hDir);
        BaseSetLastNTError(Status);
        hDir = INVALID_HANDLE_VALUE;
    }

    /* Return the directory handle on success, or invalid handle otherwise */
    return hDir;
}

/*
 * @implemented
 */
BOOL
WINAPI
FindNextChangeNotification(IN HANDLE hChangeHandle)
{
    NTSTATUS Status;

    /* Just call the native API directly, dealing with the non-optional parameters */
    Status = NtNotifyChangeDirectoryFile(hChangeHandle,
                                         NULL,
                                         NULL,
                                         NULL,
                                         &staticIoStatusBlock,
                                         staticchangebuff,
                                         sizeof(staticchangebuff),
                                         FILE_NOTIFY_CHANGE_SECURITY,
                                         TRUE);
    if (!NT_SUCCESS(Status))
    {
        /* Convert the error code and fail */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* All went well */
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
ReadDirectoryChangesW(IN HANDLE hDirectory,
                      IN LPVOID lpBuffer OPTIONAL,
                      IN DWORD nBufferLength,
                      IN BOOL bWatchSubtree,
                      IN DWORD dwNotifyFilter,
                      OUT LPDWORD lpBytesReturned,
                      IN LPOVERLAPPED lpOverlapped OPTIONAL,
                      IN LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{

    NTSTATUS Status;
    HANDLE EventHandle;
    PVOID ApcContext;
    PIO_APC_ROUTINE ApcRoutine;
    PBASEP_ACTCTX_BLOCK ActivationContext = NULL;
    BOOL Result = TRUE;
    IO_STATUS_BLOCK IoStatusBlock;

    /* Is the caller doing this synchronously? */
    if (!lpOverlapped)
    {
        /* Great, just pass in the parameters */
        Status = NtNotifyChangeDirectoryFile(hDirectory,
                                             NULL,
                                             NULL,
                                             NULL,
                                             &IoStatusBlock,
                                             lpBuffer,
                                             nBufferLength,
                                             dwNotifyFilter,
                                             bWatchSubtree);
        if (Status == STATUS_PENDING)
        {
            /* Wait for completion since we are synchronous */
            Status = NtWaitForSingleObject(hDirectory, FALSE, NULL);
            if (!NT_SUCCESS(Status))
            {
                /* The wait failed, bail out */
                BaseSetLastNTError(Status);
                return FALSE;
            }

            /* Retrieve the final status code */
            Status = IoStatusBlock.Status;
        }

        /* Did the operation succeed? */
        if (NT_SUCCESS(Status))
        {
            /* Return the bytes transferd and success */
            *lpBytesReturned = IoStatusBlock.Information;
            return Result;
        }

        /* Convert error code and return failure */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Does the caller want an APC callback? */
    if (lpCompletionRoutine)
    {
        /* Don't use an event in this case */
        EventHandle = NULL;

        /* Allocate a Fusion/SxS activation context for the callback routine */
        Status = BasepAllocateActivationContextActivationBlock(1 | 2,
                                                               lpCompletionRoutine,
                                                               lpOverlapped,
                                                               &ActivationContext);
        if (!NT_SUCCESS(Status))
        {
            /* This failed, so abandon the call */
            BaseSetLastNTError(Status);
            return FALSE;
        }

        /* Use the SxS context as the APC context */
        ApcContext = ActivationContext;
        if (ActivationContext)
        {
            /* And use a special stub routine that deals with activation */
            ApcRoutine = BasepIoCompletion;
        }
        else
        {
            /* If there was no context, however, use the simple stub routine */
            ApcContext = lpCompletionRoutine;
            ApcRoutine = BasepIoCompletionSimple;
        }
    }
    else
    {
        /* Use the even with no APC routine */
        EventHandle = lpOverlapped->hEvent;
        ApcRoutine = 0;

        /* LPOVERLAPPED should be ignored if event is ORed with 1 */
        ApcContext = (ULONG_PTR)lpOverlapped->hEvent & 1 ? NULL : lpOverlapped;
    }

    /* Set the initial status to pending and call the native API */
    lpOverlapped->Internal = STATUS_PENDING;
    Status = NtNotifyChangeDirectoryFile(hDirectory,
                                         EventHandle,
                                         ApcRoutine,
                                         ApcContext,
                                         (PIO_STATUS_BLOCK)lpOverlapped,
                                         lpBuffer,
                                         nBufferLength,
                                         dwNotifyFilter,
                                         (BOOLEAN)bWatchSubtree);
    if (NT_ERROR(Status))
    {
        /* Normally we cleanup the context in the completion routine, but we failed */
        if (ActivationContext) BasepFreeActivationContextActivationBlock(ActivationContext);

        /* Convert the error and fail */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Return success */
    return Result;
}

/* EOF */
