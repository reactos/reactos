/* $Id: procmem.c,v 1.2 2000/07/01 17:07:00 ea Exp $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/kernel32/mem/procmem.c
 * PURPOSE:              
 * PROGRAMMER:           Boudewijn Dekker
 */

/* INCLUDES ******************************************************************/


#include <ddk/ntddk.h>
#include <kernel32/error.h>
#include <windows.h>

/* FUNCTIONS *****************************************************************/
WINBOOL
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


WINBOOL
STDCALL
WriteProcessMemory (
	HANDLE	hProcess,
	LPVOID	lpBaseAddress,
	LPVOID	lpBuffer,
	DWORD	nSize,
	LPDWORD	lpNumberOfBytesWritten
	)
{
	NTSTATUS Status;

	Status = NtWriteVirtualMemory( hProcess, lpBaseAddress,lpBuffer, nSize,
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
