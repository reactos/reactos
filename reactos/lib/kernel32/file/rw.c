/* $Id: rw.c,v 1.24 2004/01/23 16:37:11 ekohl Exp $
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
#include <kernel32/kernel32.h>


/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
BOOL STDCALL
WriteFile(HANDLE hFile,
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
   
  if (lpOverLapped == NULL && lpNumberOfBytesWritten == NULL)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
  }  
   
  if (lpNumberOfBytesWritten)
  {
    *lpNumberOfBytesWritten = 0;
  }
   
   if (IsConsoleHandle(hFile))
     {
	return(WriteConsoleA(hFile,
			     lpBuffer,
			     nNumberOfBytesToWrite,
			     lpNumberOfBytesWritten,
			     NULL));
     }
      
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

   errCode = NtWriteFile(hFile,
			 hEvent,
			 NULL,
			 NULL,
			 IoStatusBlock,
			 (PVOID)lpBuffer, 
			 nNumberOfBytesToWrite,
			 ptrOffset,
			 NULL);
       
  if (!NT_SUCCESS(errCode) || errCode == STATUS_PENDING)
  {
    SetLastErrorByStatus (errCode);
    return FALSE;
  }
     
  if (lpNumberOfBytesWritten != NULL )
  {
    *lpNumberOfBytesWritten = IoStatusBlock->Information;
  }
     
  DPRINT("WriteFile() succeeded\n");
  return(TRUE);
}


/*
 * @implemented
 */
BOOL STDCALL
ReadFile(
  HANDLE hFile,
  LPVOID lpBuffer,
  DWORD nNumberOfBytesToRead,
  LPDWORD lpNumberOfBytesRead,
  LPOVERLAPPED lpOverLapped
  )
{
   HANDLE hEvent = NULL;
   LARGE_INTEGER Offset;
   NTSTATUS errCode;
   IO_STATUS_BLOCK IIosb;
   PIO_STATUS_BLOCK IoStatusBlock;
   PLARGE_INTEGER ptrOffset;

  if (lpOverLapped == NULL && lpNumberOfBytesRead == NULL)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
  }  

  if (lpNumberOfBytesRead)
  {
    *lpNumberOfBytesRead = 0;
  }
   
  if (IsConsoleHandle(hFile))
  {
    return(ReadConsoleA(hFile,
			    lpBuffer,
			    nNumberOfBytesToRead,
			    lpNumberOfBytesRead,
			    NULL));
  }
   
  if (lpOverLapped) 
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

  /* for non-overlapped io, a end-of-file error is translated to
  a successful operation with zero bytes read */
  if (!lpOverLapped && errCode == STATUS_END_OF_FILE)
  {
    /* IoStatusBlock->Information should contain zero in case of an error, but to be safe,
    set lpNumberOfBytesRead to zero manually instead of using IoStatusBlock->Information */
    *lpNumberOfBytesRead = 0;
    return TRUE;
  }
   
  if (!NT_SUCCESS(errCode) || errCode == STATUS_PENDING)
  {
    SetLastErrorByStatus (errCode);
    return FALSE;
  }

  if (lpNumberOfBytesRead != NULL)
  {
    *lpNumberOfBytesRead = IoStatusBlock->Information;
  }
  
  return(TRUE);
}

VOID STDCALL
ApcRoutine(PVOID ApcContext, 
		struct _IO_STATUS_BLOCK* IoStatusBlock, 
		ULONG Reserved)
{
   DWORD dwErrorCode;
   LPOVERLAPPED_COMPLETION_ROUTINE  lpCompletionRoutine = 
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
WriteFileEx (HANDLE				hFile,
	     LPCVOID				lpBuffer,
	     DWORD				nNumberOfBytesToWrite,
	     LPOVERLAPPED			lpOverLapped,
	     LPOVERLAPPED_COMPLETION_ROUTINE	lpCompletionRoutine)
{

   LARGE_INTEGER Offset;
   NTSTATUS errCode;
   PIO_STATUS_BLOCK IoStatusBlock;
   PLARGE_INTEGER ptrOffset;
   
   DPRINT("WriteFileEx(hFile %x)\n",hFile);
   
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
       
   if (NT_ERROR(errCode))
     {
	SetLastErrorByStatus (errCode);
	DPRINT("WriteFileEx() failed\n");
	return FALSE;
     }
     
  if (NT_WARNING(errCode))
  {
    SetLastErrorByStatus(errCode);
  }
  else
  {
    SetLastError(0);
  }
  
   DPRINT("WriteFileEx() succeeded\n");
   return(TRUE);
}


/*
 * @implemented
 */
BOOL STDCALL
ReadFileEx(HANDLE hFile,
			   LPVOID lpBuffer,
			   DWORD nNumberOfBytesToRead,
			   LPOVERLAPPED lpOverLapped,
			   LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
   LARGE_INTEGER Offset;
   NTSTATUS errCode;
   PIO_STATUS_BLOCK IoStatusBlock;
   PLARGE_INTEGER ptrOffset;
   
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

   if (NT_ERROR(errCode))  
     {
	SetLastErrorByStatus (errCode);
	return(FALSE);
     }
     
  if (NT_WARNING(errCode))
  {
    SetLastErrorByStatus(errCode);
  }
  else
  {
    SetLastError(0);
  }
     
   return(TRUE);
}

/* EOF */
