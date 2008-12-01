/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/lfile.c
 * PURPOSE:         Find functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <k32.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(kernel32file);

/*
 * @implemented
 */
long
WINAPI
_hread(
	HFILE	hFile,
	LPVOID	lpBuffer,
	long	lBytes
	)
{
	DWORD	NumberOfBytesRead;

	if ( !ReadFile(
		(HANDLE) hFile,
		(LPVOID) lpBuffer,
		(DWORD) lBytes,
		& NumberOfBytesRead,
		NULL) )
	{
		return HFILE_ERROR;
	}
	return NumberOfBytesRead;
}


/*
 * @implemented
 */
long
WINAPI
_hwrite (
	HFILE	hFile,
	LPCSTR	lpBuffer,
	long	lBytes
	)
{
	DWORD	NumberOfBytesWritten;

	if (lBytes == 0)
	{
		if ( !SetEndOfFile((HANDLE) hFile ) )
		{
			return HFILE_ERROR;
		}
		return 0;
	}
	if ( !WriteFile(
		(HANDLE) hFile,
		(LPVOID) lpBuffer,
		(DWORD) lBytes,
		& NumberOfBytesWritten,
		NULL) )
	{
		return HFILE_ERROR;
	}
	return NumberOfBytesWritten;
}


/*
 * @implemented
 */
HFILE
WINAPI
_lopen (
	LPCSTR	lpPathName,
	int	iReadWrite
	)
{
	DWORD dwAccessMask = 0;
	DWORD dwShareMode = 0;

	if ( (iReadWrite & OF_READWRITE ) == OF_READWRITE )
		dwAccessMask = GENERIC_READ | GENERIC_WRITE;
	else if ( (iReadWrite & OF_READ ) == OF_READ )
		dwAccessMask = GENERIC_READ;
	else if ( (iReadWrite & OF_WRITE ) == OF_WRITE )
		dwAccessMask = GENERIC_WRITE;

	if ((iReadWrite & OF_SHARE_DENY_READ) == OF_SHARE_DENY_READ)
		dwShareMode = FILE_SHARE_WRITE;
	else if ((iReadWrite & OF_SHARE_DENY_WRITE) == OF_SHARE_DENY_WRITE )
		dwShareMode = FILE_SHARE_READ;
	else if ((iReadWrite & OF_SHARE_EXCLUSIVE) == OF_SHARE_EXCLUSIVE)
		dwShareMode = 0;
	else
		/* OF_SHARE_DENY_NONE, OF_SHARE_COMPAT and everything else */
                dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;

	return (HFILE) CreateFileA(
			lpPathName,
			dwAccessMask,
			dwShareMode,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
}


/*
 * @implemented
 */
HFILE
WINAPI
_lcreat (
	LPCSTR	lpPathName,
	int	iAttribute
	)
{
	iAttribute &= FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
	return (HFILE) CreateFileA(
			lpPathName,
			GENERIC_READ | GENERIC_WRITE,
			(FILE_SHARE_READ | FILE_SHARE_WRITE),
			NULL,
			CREATE_ALWAYS,
			iAttribute,
			NULL);
}


/*
 * @implemented
 */
int
WINAPI
_lclose (
	HFILE	hFile
	)
{
	if (CloseHandle ((HANDLE)hFile))
	{
		return 0;
	}
	return HFILE_ERROR;
}


/*
 * @implemented
 */
LONG
WINAPI
_llseek(
	HFILE	hFile,
	LONG	lOffset,
	int	iOrigin
	)
{
	return SetFilePointer (
			(HANDLE) hFile,
			lOffset,
			NULL,
			(DWORD) iOrigin);
}

/* EOF */
