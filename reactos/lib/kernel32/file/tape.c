/* $Id: tape.c,v 1.4 2003/01/15 21:24:34 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/tape.c
 * PURPOSE:         Tape functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *		    GetTempFileName is modified from WINE [ Alexandre Juiliard ]
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES *****************************************************************/

#include <k32.h>

#define NDEBUG
#include <kernel32/kernel32.h>

/* FUNCTIONS ****************************************************************/

DWORD
STDCALL
CreateTapePartition (
	HANDLE	hDevice,
	DWORD	dwPartitionMethod,
	DWORD	dwCount,
	DWORD	dwSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

DWORD
STDCALL
EraseTape (
	HANDLE	hDevice,
	DWORD	dwEraseType,
	WINBOOL	bImmediate
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

DWORD
STDCALL
GetTapeParameters (
	HANDLE	hDevice,
	DWORD	dwOperation,
	LPDWORD	lpdwSize,
	LPVOID	lpTapeInformation
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetTapeStatus (
	HANDLE	hDevice
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

DWORD
STDCALL
PrepareTape (
	HANDLE	hDevice,
	DWORD	dwOperation,
	WINBOOL	bImmediate
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

DWORD
STDCALL
SetTapeParameters (
	HANDLE	hDevice,
	DWORD	dwOperation,
	LPVOID	lpTapeInformation
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
SetTapePosition (
	HANDLE	hDevice,
	DWORD	dwPositionMethod,
	DWORD	dwPartition,
	DWORD	dwOffsetLow,
	DWORD	dwOffsetHigh,
	WINBOOL	bImmediate
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

DWORD
STDCALL
WriteTapemark (
	HANDLE	hDevice,
	DWORD	dwTapemarkType,
	DWORD	dwTapemarkCount,
	WINBOOL	bImmediate
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

DWORD
STDCALL
GetTapePosition (
	HANDLE	hDevice,
	DWORD	dwPositionType,
	LPDWORD	lpdwPartition,
	LPDWORD	lpdwOffsetLow,
	LPDWORD	lpdwOffsetHigh
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/* EOF */
