/* $Id: backup.c,v 1.2 2002/09/07 15:12:26 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/backup.c
 * PURPOSE:         Backup functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include <windows.h>
#define NTOS_USER_MODE
#include <ntos.h>
#include <wchar.h>
#include <string.h>

#include <kernel32/kernel32.h>
#include <kernel32/error.h>

/* FUNCTIONS ****************************************************************/

BOOL
STDCALL
BackupRead (
	HANDLE	hFile,
	LPBYTE	lpBuffer,
	DWORD	nNumberOfBytesToRead,
	LPDWORD	lpNumberOfBytesRead,
	BOOL	bAbort,
	BOOL	bProcessSecurity,
	LPVOID	* lpContext
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
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


BOOL
STDCALL
BackupWrite (
	HANDLE	hFile,
	LPBYTE	lpBuffer,
	DWORD	nNumberOfBytesToWrite,
	LPDWORD	lpNumberOfBytesWritten,
	BOOL	bAbort,
	BOOL	bProcessSecurity,
	LPVOID	* lpContext
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/* EOF */
