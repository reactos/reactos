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
#include "../include/debug.h"


/*
 * @implemented
 */
HANDLE
STDCALL
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

   if ( ExistingCompletionPort == NULL && FileHandle == INVALID_HANDLE_VALUE )
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
         SetLastErrorByStatus (errCode);
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

         SetLastErrorByStatus (errCode);
         return FALSE;
      }
   }

   return CompletionPort;
}


/*
 * @implemented
 */
BOOL
STDCALL
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
   LARGE_INTEGER Interval;

   if (!lpNumberOfBytesTransferred||!lpCompletionKey||!lpOverlapped)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   if (dwMilliseconds != INFINITE)
   {
      Interval.QuadPart = RELATIVE_TIME(MILLIS_TO_100NS(dwMilliseconds));
   }

   errCode = NtRemoveIoCompletion(CompletionHandle,
                                  (PVOID*)lpCompletionKey,
                                  (PVOID*)lpNumberOfBytesTransferred,
                                  &IoStatus,
                                  dwMilliseconds == INFINITE ? NULL : &Interval);

   if (!NT_SUCCESS(errCode)) {
      *lpOverlapped = NULL;
      SetLastErrorByStatus(errCode);
      return FALSE;
   }

   *lpOverlapped = (LPOVERLAPPED)IoStatus.Information;

   if (!NT_SUCCESS(IoStatus.Status)){
      //failed io operation
      SetLastErrorByStatus(IoStatus.Status);
      return FALSE;
   }

   return TRUE;

}


/*
 * @implemented
 */
BOOL
STDCALL
PostQueuedCompletionStatus(
   HANDLE CompletionHandle,
   DWORD dwNumberOfBytesTransferred,
   ULONG_PTR dwCompletionKey,
   LPOVERLAPPED lpOverlapped
   )
{
   NTSTATUS errCode;

   errCode = NtSetIoCompletion(CompletionHandle,
                               (PVOID)dwCompletionKey,
                               (PVOID)lpOverlapped,//CompletionValue
                               STATUS_SUCCESS,                         //IoStatusBlock->Status
                               dwNumberOfBytesTransferred);     //IoStatusBlock->Information

   if ( !NT_SUCCESS(errCode) )
   {
      SetLastErrorByStatus (errCode);
      return FALSE;
   }
   return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
CancelIo(HANDLE hFile)
{
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS Status;

  Status = NtCancelIoFile(hFile,
			  &IoStatusBlock);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }

  return(TRUE);
}


/*
 * @unimplemented
 */
BOOL
WINAPI
CancelIoEx(IN HANDLE hFile,
           IN LPOVERLAPPED lpOverlapped)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
CancelSynchronousIo(IN HANDLE hThread)
{
    UNIMPLEMENTED;
    return FALSE;
}

/* EOF */
