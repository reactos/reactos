/* $Id: tape.c,v 1.6 2004/01/23 16:37:11 ekohl Exp $
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

/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
DWORD
STDCALL
EraseTape (
	HANDLE	hDevice,
	DWORD	dwEraseType,
	BOOL	bImmediate
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
DWORD
STDCALL
GetTapeStatus (
	HANDLE	hDevice
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
PrepareTape (
	HANDLE	hDevice,
	DWORD	dwOperation,
	BOOL	bImmediate
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
DWORD
STDCALL
SetTapePosition (
	HANDLE	hDevice,
	DWORD	dwPositionMethod,
	DWORD	dwPartition,
	DWORD	dwOffsetLow,
	DWORD	dwOffsetHigh,
	BOOL	bImmediate
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
WriteTapemark (
	HANDLE	hDevice,
	DWORD	dwTapemarkType,
	DWORD	dwTapemarkCount,
	BOOL	bImmediate
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/*
 * @unimplemented
 */
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
