/* $Id: iocompl.c,v 1.10 2003/07/10 18:50:51 chorns Exp $
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


#include <kernel32/error.h>

/*
 * @implemented
 */
HANDLE
STDCALL
CreateIoCompletionPort(
    HANDLE FileHandle,
    HANDLE ExistingCompletionPort,
    DWORD CompletionKey,
    DWORD NumberOfConcurrentThreads
    )
{
   HANDLE CompletionPort = NULL;
   NTSTATUS errCode;
   FILE_COMPLETION_INFORMATION CompletionInformation;
   IO_STATUS_BLOCK IoStatusBlock;

   if ( ExistingCompletionPort == NULL && FileHandle == INVALID_HANDLE_VALUE ) 
   {
      SetLastErrorByStatus (STATUS_INVALID_PARAMETER);
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
      CompletionInformation.IoCompletionHandle = CompletionPort;
      CompletionInformation.CompletionKey  = CompletionKey;

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
WINBOOL
STDCALL
GetQueuedCompletionStatus(
   HANDLE CompletionHandle,
   LPDWORD lpNumberOfBytesTransferred,
   LPDWORD lpCompletionKey,
   LPOVERLAPPED *lpOverlapped,
   DWORD dwMilliseconds
   )
{
   NTSTATUS errCode;
   IO_STATUS_BLOCK IoStatus;
   LARGE_INTEGER Interval;

   if (!lpNumberOfBytesTransferred||!lpCompletionKey||!lpOverlapped)
   {
      return ERROR_INVALID_PARAMETER;
   }

   if (dwMilliseconds != INFINITE)
   {
      /*
       * System time units are 100 nanoseconds (a nanosecond is a billionth of
       * a second).
       */
      Interval.QuadPart = dwMilliseconds;
      Interval.QuadPart = -(Interval.QuadPart * 10000);
   }  
   else
   {
      /* Approximately 292000 years hence */
      Interval.QuadPart = -0x7FFFFFFFFFFFFFFF;
   }

   errCode = NtRemoveIoCompletion(CompletionHandle,
                                  lpCompletionKey,
                                  lpNumberOfBytesTransferred,
                                  &IoStatus,
                                  &Interval);

   if (!NT_SUCCESS(errCode) ) {
      *lpOverlapped = NULL;
      SetLastErrorByStatus(errCode);
      return FALSE;
   }

   *lpOverlapped = (LPOVERLAPPED)IoStatus.Information;

   if (!NT_SUCCESS(IoStatus.Status)){
      //failed io operation
      SetLastErrorByStatus (IoStatus.Status);
      return FALSE;
   }

   return TRUE;

}


/*
 * @implemented
 */
WINBOOL
STDCALL
PostQueuedCompletionStatus(
   HANDLE CompletionHandle,
   DWORD dwNumberOfBytesTransferred,
   DWORD dwCompletionKey,
   LPOVERLAPPED lpOverlapped
   )
{
   NTSTATUS errCode;

   errCode = NtSetIoCompletion(CompletionHandle,  
                               dwCompletionKey, 
                               dwNumberOfBytesTransferred,//CompletionValue 
                               0,                         //IoStatusBlock->Status
                               (ULONG)lpOverlapped );     //IoStatusBlock->Information

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

/* EOF */
