/* $Id: lock.c,v 1.6 2002/09/07 15:12:26 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/file.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *		    GetTempFileName is modified from WINE [ Alexandre Juiliard ]
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* FIXME: the large integer manipulations in this file dont handle overflow  */

/* INCLUDES ****************************************************************/

#include <windows.h>
#define NTOS_USER_MODE
#include <ntos.h>
#include <wchar.h>
#include <string.h>

#define NDEBUG
#include <kernel32/kernel32.h>
#include <kernel32/error.h>

/* FUNCTIONS ****************************************************************/

WINBOOL
STDCALL
LockFile(
	 HANDLE hFile,
	 DWORD dwFileOffsetLow,
	 DWORD dwFileOffsetHigh,
	 DWORD nNumberOfBytesToLockLow,
	 DWORD nNumberOfBytesToLockHigh
	 )
{	
	DWORD dwReserved;
	OVERLAPPED Overlapped;
   
	Overlapped.Offset = dwFileOffsetLow;
	Overlapped.OffsetHigh = dwFileOffsetHigh;
	dwReserved = 0;

  	return LockFileEx(hFile, LOCKFILE_FAIL_IMMEDIATELY|LOCKFILE_EXCLUSIVE_LOCK,dwReserved,nNumberOfBytesToLockLow, nNumberOfBytesToLockHigh, &Overlapped ) ;
 
}

WINBOOL
STDCALL
LockFileEx(
	   HANDLE hFile,
	   DWORD dwFlags,
	   DWORD dwReserved,
	   DWORD nNumberOfBytesToLockLow,
	   DWORD nNumberOfBytesToLockHigh,
	   LPOVERLAPPED lpOverlapped
	   )
{
   LARGE_INTEGER BytesToLock;	
   BOOL LockImmediate;
   BOOL LockExclusive;
   NTSTATUS errCode;
   LARGE_INTEGER Offset;
   
   if(dwReserved != 0) 
     {      
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
     }
   
   lpOverlapped->Internal = STATUS_PENDING;  
   
   Offset.u.LowPart = lpOverlapped->Offset;
   Offset.u.HighPart = lpOverlapped->OffsetHigh;
   
   if ( (dwFlags & LOCKFILE_FAIL_IMMEDIATELY) == LOCKFILE_FAIL_IMMEDIATELY )
     LockImmediate = TRUE;
   else
     LockImmediate = FALSE;
   
   if ( (dwFlags & LOCKFILE_EXCLUSIVE_LOCK) == LOCKFILE_EXCLUSIVE_LOCK )
     LockExclusive = TRUE;
   else
     LockExclusive = FALSE;
   
   BytesToLock.u.LowPart = nNumberOfBytesToLockLow;
   BytesToLock.u.HighPart = nNumberOfBytesToLockHigh;
   
   errCode = NtLockFile(hFile,
			NULL,
			NULL,
			NULL,
			(PIO_STATUS_BLOCK)lpOverlapped,
			&Offset,
			&BytesToLock,
			NULL,
			LockImmediate,
			LockExclusive);
   if ( !NT_SUCCESS(errCode) ) 
     {
      SetLastErrorByStatus (errCode);
      return FALSE;
     }
   
   return TRUE;
  	         
}

WINBOOL
STDCALL
UnlockFile(
	   HANDLE hFile,
	   DWORD dwFileOffsetLow,
	   DWORD dwFileOffsetHigh,
	   DWORD nNumberOfBytesToUnlockLow,
	   DWORD nNumberOfBytesToUnlockHigh
	   )
{
	DWORD dwReserved;
	OVERLAPPED Overlapped;
	Overlapped.Offset = dwFileOffsetLow;
	Overlapped.OffsetHigh = dwFileOffsetHigh;
	dwReserved = 0;
	return UnlockFileEx(hFile, dwReserved, nNumberOfBytesToUnlockLow, nNumberOfBytesToUnlockHigh, &Overlapped);

}



WINBOOL 
STDCALL 
UnlockFileEx(
	HANDLE hFile,
	DWORD dwReserved,
	DWORD nNumberOfBytesToUnLockLow,
	DWORD nNumberOfBytesToUnLockHigh,
	LPOVERLAPPED lpOverlapped
	)
{
   LARGE_INTEGER BytesToUnLock;
   LARGE_INTEGER StartAddress;
   NTSTATUS errCode;
   
   if(dwReserved != 0) 
     {
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
     }
   if ( lpOverlapped == NULL ) 
     {
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
     }
   
   BytesToUnLock.u.LowPart = nNumberOfBytesToUnLockLow;
   BytesToUnLock.u.HighPart = nNumberOfBytesToUnLockHigh;
   
   StartAddress.u.LowPart = lpOverlapped->Offset;
   StartAddress.u.HighPart = lpOverlapped->OffsetHigh;
   
   errCode = NtUnlockFile(hFile,
			  (PIO_STATUS_BLOCK)lpOverlapped,
			  &StartAddress,
			  &BytesToUnLock,
			  NULL);
   if ( !NT_SUCCESS(errCode) ) {
      SetLastErrorByStatus (errCode);
      return FALSE;
   }
   
   return TRUE;
}

/* EOF */
