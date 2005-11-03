/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/kernel32/mem/procmem.c
 * PURPOSE:
 * PROGRAMMER:           Boudewijn Dekker
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOL
STDCALL
ReadProcessMemory (
	HANDLE	hProcess,
	LPCVOID	lpBaseAddress,
	LPVOID	lpBuffer,
	DWORD	nSize,
	LPDWORD	lpNumberOfBytesRead
	)
{

	NTSTATUS Status;

	Status = NtReadVirtualMemory( hProcess, (PVOID)lpBaseAddress,lpBuffer, nSize,
		(PULONG)lpNumberOfBytesRead
		);

	if (!NT_SUCCESS(Status))
     	{
		SetLastErrorByStatus (Status);
		return FALSE;
     	}
	return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
WriteProcessMemory (
	HANDLE hProcess,
	LPVOID lpBaseAddress,
	LPCVOID lpBuffer,
	SIZE_T nSize,
	SIZE_T *lpNumberOfBytesWritten
	)
{
	NTSTATUS Status;

	Status = NtWriteVirtualMemory( hProcess, lpBaseAddress, (LPVOID)lpBuffer, nSize,
		(PULONG)lpNumberOfBytesWritten
		);

	if (!NT_SUCCESS(Status))
     	{
		SetLastErrorByStatus (Status);
		return FALSE;
     	}
	return TRUE;
}

/* EOF */
