/* $Id: rw.c,v 1.8 2000/01/11 17:30:16 ekohl Exp $
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

#include <ddk/ntddk.h>
#include <windows.h>
#include <wchar.h>
#include <string.h>

#define NDEBUG
#include <kernel32/kernel32.h>


/* FUNCTIONS ****************************************************************/

WINBOOL STDCALL WriteFile(HANDLE hFile,
			  LPCVOID lpBuffer,	
			  DWORD nNumberOfBytesToWrite,
			  LPDWORD lpNumberOfBytesWritten,	
			  LPOVERLAPPED lpOverLapped)
{
   HANDLE hEvent = NULL;
   LARGE_INTEGER Offset;
   NTSTATUS errCode;
   IO_STATUS_BLOCK IIosb;
   PIO_STATUS_BLOCK IoStatusBlock;
   PLARGE_INTEGER ptrOffset;
   
   DPRINT("WriteFile(hFile %x)\n",hFile);
   
   if (lpOverLapped != NULL) 
     {
        Offset.u.LowPart = lpOverLapped->Offset;
        Offset.u.HighPart = lpOverLapped->OffsetHigh;
	lpOverLapped->Internal = STATUS_PENDING;
	hEvent = lpOverLapped->hEvent;
	IoStatusBlock = (PIO_STATUS_BLOCK)lpOverLapped;
	ptrOffset = &Offset;
     }
   else 
     {
	ptrOffset = NULL;
	IoStatusBlock = &IIosb;
        Offset.QuadPart = 0;
     }

   errCode = NtWriteFile(hFile,
			 hEvent,
			 NULL,
			 NULL,
			 IoStatusBlock,
			 (PVOID)lpBuffer, 
			 nNumberOfBytesToWrite,
			 ptrOffset,
			 NULL);
   if (!NT_SUCCESS(errCode))
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	DPRINT("WriteFile() failed\n");
	return FALSE;
     }
   if (lpNumberOfBytesWritten != NULL )
     {
	*lpNumberOfBytesWritten = IoStatusBlock->Information;
     }
   DPRINT("WriteFile() succeeded\n");
   return(TRUE);
}



WINBOOL STDCALL ReadFile(HANDLE hFile,
			 LPVOID lpBuffer,
			 DWORD nNumberOfBytesToRead,
			 LPDWORD lpNumberOfBytesRead,
			 LPOVERLAPPED lpOverLapped)
{
   HANDLE hEvent = NULL;
   LARGE_INTEGER Offset;
   NTSTATUS errCode;
   IO_STATUS_BLOCK IIosb;
   PIO_STATUS_BLOCK IoStatusBlock;
   PLARGE_INTEGER ptrOffset;
   
   if (lpOverLapped != NULL) 
     {
        Offset.u.LowPart = lpOverLapped->Offset;
        Offset.u.HighPart = lpOverLapped->OffsetHigh;
	lpOverLapped->Internal = STATUS_PENDING;
	hEvent = lpOverLapped->hEvent;
	IoStatusBlock = (PIO_STATUS_BLOCK)lpOverLapped;
	ptrOffset = &Offset;
     }
   else 
     {
	ptrOffset = NULL;
	IoStatusBlock = &IIosb;
     }
   
   errCode = NtReadFile(hFile,
			hEvent,
			NULL,
			NULL,
			IoStatusBlock,
			lpBuffer,
			nNumberOfBytesToRead,
			ptrOffset,
			NULL);
   
   if (errCode != STATUS_PENDING && lpNumberOfBytesRead != NULL)
     {
	*lpNumberOfBytesRead = IoStatusBlock->Information;
     }
   
   if (!NT_SUCCESS(errCode))  
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	return(FALSE);
     }
   return(TRUE);
}

VOID ApcRoutine(PVOID ApcContext, struct _IO_STATUS_BLOCK* IoStatusBlock, ULONG NumberOfBytesTransfered)
{
	DWORD dwErrorCode;
	LPOVERLAPPED_COMPLETION_ROUTINE  lpCompletionRoutine = (LPOVERLAPPED_COMPLETION_ROUTINE)ApcContext;

	dwErrorCode = RtlNtStatusToDosError( IoStatusBlock->Status);
	lpCompletionRoutine(  dwErrorCode, NumberOfBytesTransfered, (LPOVERLAPPED)IoStatusBlock );
}


WINBOOL
STDCALL
WriteFileEx (
	HANDLE				hFile,
	LPCVOID				lpBuffer,
	DWORD				nNumberOfBytesToWrite,
	LPOVERLAPPED			lpOverLapped,
	LPOVERLAPPED_COMPLETION_ROUTINE	lpCompletionRoutine
	)
{

   LARGE_INTEGER Offset;
   NTSTATUS errCode;
   PIO_STATUS_BLOCK IoStatusBlock;
   PLARGE_INTEGER ptrOffset;
   
   DPRINT("WriteFileEx(hFile %x)\n",hFile);
   
   if (lpOverLapped == NULL) 
	return FALSE;

   Offset.u.LowPart = lpOverLapped->Offset;
   Offset.u.HighPart = lpOverLapped->OffsetHigh;
   lpOverLapped->Internal = STATUS_PENDING;
   IoStatusBlock = (PIO_STATUS_BLOCK)lpOverLapped;
   ptrOffset = &Offset;

   errCode = NtWriteFile(hFile,
			 NULL,
			 ApcRoutine,
			 lpCompletionRoutine,
			 IoStatusBlock,
			 (PVOID)lpBuffer, 
			 nNumberOfBytesToWrite,
			 ptrOffset,
			 NULL);
   if (!NT_SUCCESS(errCode))
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	DPRINT("WriteFileEx() failed\n");
	return FALSE;
     }
  
   DPRINT("WriteFileEx() succeeded\n");
   return(TRUE);
}

WINBOOL STDCALL ReadFileEx(HANDLE hFile,
			   LPVOID lpBuffer,
			   DWORD nNumberOfBytesToRead,
			   LPOVERLAPPED lpOverLapped,
			   LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
   LARGE_INTEGER Offset;
   NTSTATUS errCode;
   PIO_STATUS_BLOCK IoStatusBlock;
   PLARGE_INTEGER ptrOffset;
   
   if (lpOverLapped == NULL) 
	return FALSE;

   Offset.u.LowPart = lpOverLapped->Offset;
   Offset.u.HighPart = lpOverLapped->OffsetHigh;
   lpOverLapped->Internal = STATUS_PENDING;
   IoStatusBlock = (PIO_STATUS_BLOCK)lpOverLapped;
   ptrOffset = &Offset;

   errCode = NtReadFile(hFile,
			NULL,
			ApcRoutine,
			lpCompletionRoutine,
			IoStatusBlock,
			lpBuffer,
			nNumberOfBytesToRead,
			ptrOffset,
			NULL);

   if (!NT_SUCCESS(errCode))  
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	return(FALSE);
     }
   return(TRUE);
}

/* EOF */
