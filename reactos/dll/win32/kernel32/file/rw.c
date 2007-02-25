/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/rw.c
 * PURPOSE:         Read/write functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ****************************************************************/

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"


/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
BOOL STDCALL
WriteFile(IN HANDLE hFile,
          IN LPCVOID lpBuffer,
          IN DWORD nNumberOfBytesToWrite  OPTIONAL,
          OUT LPDWORD lpNumberOfBytesWritten  OPTIONAL,
          IN LPOVERLAPPED lpOverlapped  OPTIONAL)
{
   NTSTATUS Status;

   DPRINT("WriteFile(hFile %x)\n", hFile);

   if (lpNumberOfBytesWritten != NULL)
     {
        *lpNumberOfBytesWritten = 0;
     }

   if (IsConsoleHandle(hFile))
     {
	return WriteConsoleA(hFile,
                             lpBuffer,
                             nNumberOfBytesToWrite,
                             lpNumberOfBytesWritten,
                             lpOverlapped);
     }

   if (lpOverlapped != NULL)
     {
        LARGE_INTEGER Offset;
        PVOID ApcContext;

        Offset.u.LowPart = lpOverlapped->Offset;
        Offset.u.HighPart = lpOverlapped->OffsetHigh;
	lpOverlapped->Internal = STATUS_PENDING;
	ApcContext = (((ULONG_PTR)lpOverlapped->hEvent & 0x1) ? NULL : lpOverlapped);

	Status = NtWriteFile(hFile,
                             lpOverlapped->hEvent,
                             NULL,
                             ApcContext,
                             (PIO_STATUS_BLOCK)lpOverlapped,
                             (PVOID)lpBuffer,
                             nNumberOfBytesToWrite,
                             &Offset,
                             NULL);

        /* return FALSE in case of failure and pending operations! */
        if (!NT_SUCCESS(Status) || Status == STATUS_PENDING)
          {
             SetLastErrorByStatus(Status);
             return FALSE;
          }

        if (lpNumberOfBytesWritten != NULL)
          {
             *lpNumberOfBytesWritten = lpOverlapped->InternalHigh;
          }
     }
   else
     {
        IO_STATUS_BLOCK Iosb;

        Status = NtWriteFile(hFile,
                             NULL,
                             NULL,
                             NULL,
                             &Iosb,
                             (PVOID)lpBuffer,
                             nNumberOfBytesToWrite,
                             NULL,
                             NULL);

        /* wait in case operation is pending */
        if (Status == STATUS_PENDING)
          {
             Status = NtWaitForSingleObject(hFile,
                                            FALSE,
                                            NULL);
             if (NT_SUCCESS(Status))
               {
                  Status = Iosb.Status;
               }
          }

        if (NT_SUCCESS(Status))
          {
             /* lpNumberOfBytesWritten must not be NULL here, in fact Win doesn't
                check that case either and crashes (only after the operation
                completed) */
             *lpNumberOfBytesWritten = Iosb.Information;
          }
        else
          {
             SetLastErrorByStatus(Status);
             return FALSE;
          }
     }

   DPRINT("WriteFile() succeeded\n");
   return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
ReadFile(IN HANDLE hFile,
         IN LPVOID lpBuffer,
         IN DWORD nNumberOfBytesToRead,
         OUT LPDWORD lpNumberOfBytesRead  OPTIONAL,
         IN LPOVERLAPPED lpOverlapped  OPTIONAL)
{
   NTSTATUS Status;

   DPRINT("ReadFile(hFile %x)\n", hFile);

   if (lpNumberOfBytesRead != NULL)
     {
        *lpNumberOfBytesRead = 0;
     }

   if (IsConsoleHandle(hFile))
     {
	return ReadConsoleA(hFile,
                            lpBuffer,
                            nNumberOfBytesToRead,
                            lpNumberOfBytesRead,
                            NULL);
     }

   if (lpOverlapped != NULL)
     {
        LARGE_INTEGER Offset;
        PVOID ApcContext;

        Offset.u.LowPart = lpOverlapped->Offset;
        Offset.u.HighPart = lpOverlapped->OffsetHigh;
	lpOverlapped->Internal = STATUS_PENDING;
	ApcContext = (((ULONG_PTR)lpOverlapped->hEvent & 0x1) ? NULL : lpOverlapped);

	Status = NtReadFile(hFile,
                            lpOverlapped->hEvent,
                            NULL,
                            ApcContext,
                            (PIO_STATUS_BLOCK)lpOverlapped,
                            lpBuffer,
                            nNumberOfBytesToRead,
                            &Offset,
                            NULL);

        /* return FALSE in case of failure and pending operations! */
        if (!NT_SUCCESS(Status) || Status == STATUS_PENDING)
          {
             if (Status == STATUS_END_OF_FILE &&
                 lpNumberOfBytesRead != NULL)
               {
                  *lpNumberOfBytesRead = 0;
               }

             SetLastErrorByStatus(Status);
             return FALSE;
          }

        if (lpNumberOfBytesRead != NULL)
          {
             *lpNumberOfBytesRead = lpOverlapped->InternalHigh;
          }
     }
   else
     {
        IO_STATUS_BLOCK Iosb;

        Status = NtReadFile(hFile,
                            NULL,
                            NULL,
                            NULL,
                            &Iosb,
                            lpBuffer,
                            nNumberOfBytesToRead,
                            NULL,
                            NULL);

        /* wait in case operation is pending */
        if (Status == STATUS_PENDING)
          {
             Status = NtWaitForSingleObject(hFile,
                                            FALSE,
                                            NULL);
             if (NT_SUCCESS(Status))
               {
                  Status = Iosb.Status;
               }
          }

        if (Status == STATUS_END_OF_FILE)
          {
             /* lpNumberOfBytesRead must not be NULL here, in fact Win doesn't
                check that case either and crashes (only after the operation
                completed) */
             *lpNumberOfBytesRead = 0;
             return TRUE;
          }

        if (NT_SUCCESS(Status))
          {
             /* lpNumberOfBytesRead must not be NULL here, in fact Win doesn't
                check that case either and crashes (only after the operation
                completed) */
             *lpNumberOfBytesRead = Iosb.Information;
          }
        else
          {
             SetLastErrorByStatus(Status);
             return FALSE;
          }
     }

   DPRINT("ReadFile() succeeded\n");
   return TRUE;
}

VOID STDCALL
ApcRoutine(PVOID ApcContext,
		struct _IO_STATUS_BLOCK* IoStatusBlock,
		ULONG Reserved)
{
   DWORD dwErrorCode;
   LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine =
     (LPOVERLAPPED_COMPLETION_ROUTINE)ApcContext;

   dwErrorCode = RtlNtStatusToDosError(IoStatusBlock->Status);
   lpCompletionRoutine(dwErrorCode,
                       IoStatusBlock->Information,
                       (LPOVERLAPPED)IoStatusBlock);
}


/*
 * @implemented
 */
BOOL STDCALL
WriteFileEx(IN HANDLE hFile,
            IN LPCVOID lpBuffer,
            IN DWORD nNumberOfBytesToWrite  OPTIONAL,
            IN LPOVERLAPPED lpOverlapped,
            IN LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
   LARGE_INTEGER Offset;
   NTSTATUS Status;

   Offset.u.LowPart = lpOverlapped->Offset;
   Offset.u.HighPart = lpOverlapped->OffsetHigh;
   lpOverlapped->Internal = STATUS_PENDING;

   Status = NtWriteFile(hFile,
                        NULL,
                        ApcRoutine,
                        lpCompletionRoutine,
                        (PIO_STATUS_BLOCK)lpOverlapped,
                        (PVOID)lpBuffer,
                        nNumberOfBytesToWrite,
                        &Offset,
                        NULL);

   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return FALSE;
     }

   return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
ReadFileEx(IN HANDLE hFile,
           IN LPVOID lpBuffer,
           IN DWORD nNumberOfBytesToRead  OPTIONAL,
           IN LPOVERLAPPED lpOverlapped,
           IN LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
   LARGE_INTEGER Offset;
   NTSTATUS Status;

   Offset.u.LowPart = lpOverlapped->Offset;
   Offset.u.HighPart = lpOverlapped->OffsetHigh;
   lpOverlapped->Internal = STATUS_PENDING;

   Status = NtReadFile(hFile,
                       NULL,
                       ApcRoutine,
                       lpCompletionRoutine,
                       (PIO_STATUS_BLOCK)lpOverlapped,
                       lpBuffer,
                       nNumberOfBytesToRead,
                       &Offset,
                       NULL);

   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return FALSE;
     }

   return TRUE;
}

/* EOF */
