/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/rw.c
 * PURPOSE:         Read/write functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ****************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
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

   LARGE_INTEGER Offset;
   HANDLE hEvent = NULL;
   NTSTATUS errCode;
   PIO_STATUS_BLOCK IoStatusBlock;
   IO_STATUS_BLOCK IIosb;
   
   DPRINT("WriteFile(hFile %x)\n",hFile);
   
   if (lpOverLapped != NULL ) 
     {
        Offset.u.LowPart = lpOverLapped->Offset;
        Offset.u.HighPart = lpOverLapped->OffsetHigh;
	lpOverLapped->Internal = STATUS_PENDING;
	hEvent= lpOverLapped->hEvent;
   	IoStatusBlock = (PIO_STATUS_BLOCK)lpOverLapped;
     }
   else
     {
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
			 &Offset,
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

WINBOOL STDCALL KERNEL32_ReadFile(HANDLE hFile,
				  LPVOID lpBuffer,
				  DWORD nNumberOfBytesToRead,
				  LPDWORD lpNumberOfBytesRead,
				  LPOVERLAPPED lpOverLapped,
				  LPOVERLAPPED_COMPLETION_ROUTINE 
				   lpCompletionRoutine)
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
			(PIO_APC_ROUTINE)lpCompletionRoutine,
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

WINBOOL STDCALL ReadFile(HANDLE hFile,
			 LPVOID lpBuffer,
			 DWORD nNumberOfBytesToRead,
			 LPDWORD lpNumberOfBytesRead,
			 LPOVERLAPPED lpOverLapped)
{
   return(KERNEL32_ReadFile(hFile,
			    lpBuffer,
			    nNumberOfBytesToRead,
			    lpNumberOfBytesRead,
			    lpOverLapped,
			    NULL));
}

WINBOOL STDCALL ReadFileEx(HANDLE hFile,
			   LPVOID lpBuffer,
			   DWORD nNumberOfBytesToRead,
			   LPOVERLAPPED lpOverLapped,
			   LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
   return(KERNEL32_ReadFile(hFile,
			    lpBuffer,
			    nNumberOfBytesToRead,
			    NULL,
			    lpOverLapped,
			    lpCompletionRoutine));
}
