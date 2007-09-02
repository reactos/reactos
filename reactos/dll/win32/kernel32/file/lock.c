/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/file.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* FIXME: the large integer manipulations in this file dont handle overflow  */

/* INCLUDES ****************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
BOOL
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
   Overlapped.hEvent = NULL;
	dwReserved = 0;

  	return LockFileEx(hFile, LOCKFILE_FAIL_IMMEDIATELY|LOCKFILE_EXCLUSIVE_LOCK,dwReserved,nNumberOfBytesToLockLow, nNumberOfBytesToLockHigh, &Overlapped ) ;

}


/*
 * @implemented
 */
BOOL
STDCALL
LockFileEx(
	   HANDLE hFile,
	   DWORD dwFlags,
	   DWORD dwReserved,
	   DWORD nNumberOfBytesToLockLow,
	   DWORD nNumberOfBytesToLockHigh,
	   LPOVERLAPPED lpOverlapped /* required! */
	   )
{
   LARGE_INTEGER BytesToLock;
   BOOL LockImmediate;
   BOOL LockExclusive;
   NTSTATUS errCode;
   LARGE_INTEGER Offset;

   if(dwReserved != 0 || lpOverlapped==NULL)
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
         lpOverlapped->hEvent,
			NULL,
			NULL,
			(PIO_STATUS_BLOCK)lpOverlapped,
			&Offset,
			&BytesToLock,
			0,
			(BOOLEAN)LockImmediate,
			(BOOLEAN)LockExclusive);
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
BOOL
STDCALL
UnlockFile(
	   HANDLE hFile,
	   DWORD dwFileOffsetLow,
	   DWORD dwFileOffsetHigh,
	   DWORD nNumberOfBytesToUnlockLow,
	   DWORD nNumberOfBytesToUnlockHigh
	   )
{
	OVERLAPPED Overlapped;
   DWORD dwReserved;
	Overlapped.Offset = dwFileOffsetLow;
	Overlapped.OffsetHigh = dwFileOffsetHigh;
   dwReserved = 0;

   return UnlockFileEx(hFile, dwReserved, nNumberOfBytesToUnlockLow, nNumberOfBytesToUnlockHigh, &Overlapped);
}


/*
 * @implemented
 */
BOOL
STDCALL
UnlockFileEx(
	HANDLE hFile,
	DWORD dwReserved,
	DWORD nNumberOfBytesToUnLockLow,
	DWORD nNumberOfBytesToUnLockHigh,
	LPOVERLAPPED lpOverlapped /* required! */
	)
{
   LARGE_INTEGER BytesToUnLock;
   LARGE_INTEGER StartAddress;
   NTSTATUS errCode;

   if(dwReserved != 0 || lpOverlapped == NULL)
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
			  0);
   if ( !NT_SUCCESS(errCode) ) {
      SetLastErrorByStatus (errCode);
      return FALSE;
   }

   return TRUE;
}

/* EOF */
