/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/iocompl.c
 * PURPOSE:         Io Completion functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <k32.h>
#define NDEBUG
#include <debug.h>

#define NANOS_TO_100NS(nanos) (((LONGLONG)(nanos)) / 100)
#define MICROS_TO_100NS(micros) (((LONGLONG)(micros)) * NANOS_TO_100NS(1000))
#define MILLIS_TO_100NS(milli) (((LONGLONG)(milli)) * MICROS_TO_100NS(1000))

/*
 * @implemented
 */
HANDLE
WINAPI
CreateIoCompletionPort(
    HANDLE FileHandle,
    HANDLE ExistingCompletionPort,
    ULONG_PTR CompletionKey,
    DWORD NumberOfConcurrentThreads
    )
{
   HANDLE CompletionPort = NULL;
   NTSTATUS errCode;
   FILE_COMPLETION_INFORMATION CompletionInformation;
   IO_STATUS_BLOCK IoStatusBlock;

   if ( FileHandle == INVALID_HANDLE_VALUE && ExistingCompletionPort != NULL )
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   if ( ExistingCompletionPort != NULL )
   {
      CompletionPort = ExistingCompletionPort;
   }
   else
   {

      errCode = NtCreateIoCompletion(&CompletionPort,
                                     IO_COMPLETION_ALL_ACCESS,
                                     NULL,//ObjectAttributes
                                     NumberOfConcurrentThreads);

      if (!NT_SUCCESS(errCode) )
      {
         BaseSetLastNTError (errCode);
         return FALSE;
      }

   }

   if ( FileHandle != INVALID_HANDLE_VALUE )
   {
      CompletionInformation.Port = CompletionPort;
      CompletionInformation.Key  = (PVOID)CompletionKey;

      errCode = NtSetInformationFile(FileHandle,
                                     &IoStatusBlock,
                                     &CompletionInformation,
                                     sizeof(FILE_COMPLETION_INFORMATION),
                                     FileCompletionInformation);

      if ( !NT_SUCCESS(errCode) )
      {
         if ( ExistingCompletionPort == NULL )
         {
            NtClose(CompletionPort);
         }

         BaseSetLastNTError (errCode);
         return FALSE;
      }
   }

   return CompletionPort;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetQueuedCompletionStatus(
   HANDLE CompletionHandle,
   LPDWORD lpNumberOfBytesTransferred,
   PULONG_PTR lpCompletionKey,
   LPOVERLAPPED *lpOverlapped,
   DWORD dwMilliseconds
   )
{
   NTSTATUS errCode;
   IO_STATUS_BLOCK IoStatus;
   ULONG_PTR CompletionKey;
   LARGE_INTEGER Time;
   PLARGE_INTEGER TimePtr;

   if (!lpNumberOfBytesTransferred || !lpCompletionKey || !lpOverlapped)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }
   
   /* Convert the timeout */
   TimePtr = BaseFormatTimeOut(&Time, dwMilliseconds);

   errCode = NtRemoveIoCompletion(CompletionHandle,
                                  (PVOID*)&CompletionKey,
                                  (PVOID*)lpOverlapped,
                                  &IoStatus,
                                  TimePtr);

   if (!NT_SUCCESS(errCode) || errCode == STATUS_TIMEOUT) {
      *lpOverlapped = NULL;

      if(errCode == STATUS_TIMEOUT)
         SetLastError(WAIT_TIMEOUT);
      else
         BaseSetLastNTError(errCode);

      return FALSE;
   }

   *lpCompletionKey = CompletionKey;
   *lpNumberOfBytesTransferred = IoStatus.Information;

   if (!NT_SUCCESS(IoStatus.Status)){
      //failed io operation
      BaseSetLastNTError(IoStatus.Status);
      return FALSE;
   }

   return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
PostQueuedCompletionStatus(
   HANDLE CompletionHandle,
   DWORD dwNumberOfBytesTransferred,
   ULONG_PTR dwCompletionKey,
   LPOVERLAPPED lpOverlapped
   )
{
   NTSTATUS errCode;

   errCode = NtSetIoCompletion(CompletionHandle,
                               (PVOID)dwCompletionKey,      // KeyContext
                               (PVOID)lpOverlapped,         // ApcContext
                               STATUS_SUCCESS,              // IoStatusBlock->Status
                               dwNumberOfBytesTransferred); // IoStatusBlock->Information

   if ( !NT_SUCCESS(errCode) )
   {
      BaseSetLastNTError (errCode);
      return FALSE;
   }
   return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetOverlappedResult(IN HANDLE hFile,
                    IN LPOVERLAPPED lpOverlapped,
                    OUT LPDWORD lpNumberOfBytesTransferred,
                    IN BOOL bWait)
{
    DWORD WaitStatus;
    HANDLE hObject;

    /* Check for pending operation */
    if (lpOverlapped->Internal == STATUS_PENDING)
    {
        /* Check if the caller is okay with waiting */
        if (!bWait)
        {
            /* Set timeout */
            WaitStatus = WAIT_TIMEOUT;
        }
        else
        {
            /* Wait for the result */
            hObject = lpOverlapped->hEvent ? lpOverlapped->hEvent : hFile;
            WaitStatus = WaitForSingleObject(hObject, INFINITE);
        }

        /* Check for timeout */
        if (WaitStatus == WAIT_TIMEOUT)
        {
            /* We have to override the last error with INCOMPLETE instead */
            SetLastError(ERROR_IO_INCOMPLETE);
            return FALSE;
        }

        /* Fail if we had an error -- the last error is already set */
        if (WaitStatus != 0) return FALSE;
    }

    /* Return bytes transferred */
    *lpNumberOfBytesTransferred = lpOverlapped->InternalHigh;

    /* Check for failure during I/O */
    if (!NT_SUCCESS(lpOverlapped->Internal))
    {
        /* Set the error and fail */
        BaseSetLastNTError(lpOverlapped->Internal);
        return FALSE;
    }

    /* All done */
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
BindIoCompletionCallback(HANDLE FileHandle,
                         LPOVERLAPPED_COMPLETION_ROUTINE Function,
                         ULONG Flags)
{
    NTSTATUS Status = 0;

    DPRINT("(%p, %p, %d)\n", FileHandle, Function, Flags);

    Status = RtlSetIoCompletionCallback(FileHandle,
                                        (PIO_APC_ROUTINE)Function,
                                        Flags);

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/* EOF */
