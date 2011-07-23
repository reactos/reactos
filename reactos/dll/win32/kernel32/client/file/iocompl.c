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
BOOL WINAPI
CancelIo(HANDLE hFile)
{
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS Status;

  Status = NtCancelIoFile(hFile,
			  &IoStatusBlock);
  if (!NT_SUCCESS(Status))
    {
      BaseSetLastNTError(Status);
      return(FALSE);
    }

  return(TRUE);
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
