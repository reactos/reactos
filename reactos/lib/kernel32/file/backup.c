/* $Id: backup.c,v 1.4 2003/01/15 21:24:33 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/backup.c
 * PURPOSE:         Backup functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include <k32.h>

/* FUNCTIONS ****************************************************************/

WINBOOL
STDCALL
BackupRead (
	HANDLE	hFile,
	LPBYTE	lpBuffer,
	DWORD	nNumberOfBytesToRead,
	LPDWORD	lpNumberOfBytesRead,
	WINBOOL	bAbort,
	WINBOOL	bProcessSecurity,
	LPVOID	* lpContext
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
BackupSeek (
	HANDLE	hFile,
	DWORD	dwLowBytesToSeek,
	DWORD	dwHighBytesToSeek,
	LPDWORD	lpdwLowByteSeeked,
	LPDWORD	lpdwHighByteSeeked,
	LPVOID	* lpContext
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
BackupWrite (
	HANDLE	hFile,
	LPBYTE	lpBuffer,
	DWORD	nNumberOfBytesToWrite,
	LPDWORD	lpNumberOfBytesWritten,
	WINBOOL	bAbort,
	WINBOOL	bProcessSecurity,
	LPVOID	* lpContext
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/* EOF */
