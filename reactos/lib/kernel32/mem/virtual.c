/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/kernel32/mem/virtual.c
 * PURPOSE:              Passing the Virtualxxx functions onto the kernel
 * PROGRAMMER:           David Welch (welch@mcmail.com)
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <windows.h>

/* FUNCTIONS *****************************************************************/

LPVOID STDCALL VirtualAllocEx(HANDLE hProcess,
			      LPVOID lpAddress,
			      DWORD dwSize,
			      DWORD flAllocationType,
			      DWORD flProtect)
{
   NTSTATUS Status;
   
   Status = ZwAllocateVirtualMemory(hProcess,
				    (PVOID *)&lpAddress,
				    0,
				    (PULONG)&dwSize,
				    flAllocationType,
				    flProtect);
   if (!NT_SUCCESS(Status))
     {
        SetLastError(RtlNtStatusToDosError(Status));
	return(NULL);
     }
   return(lpAddress);
}

LPVOID STDCALL VirtualAlloc(LPVOID lpAddress,
			    DWORD dwSize,
			    DWORD flAllocationType,
			    DWORD flProtect)
{
   return(VirtualAllocEx(GetCurrentProcess(),lpAddress,dwSize,flAllocationType,
			 flProtect));
}

WINBOOL STDCALL VirtualFreeEx(HANDLE hProcess,
			      LPVOID lpAddress,
			      DWORD dwSize,
			      DWORD dwFreeType)
{
   NTSTATUS Status;
   
   Status = ZwFreeVirtualMemory(hProcess,
				(PVOID *)&lpAddress,
				(PULONG)&dwSize,
				dwFreeType);
   if (!NT_SUCCESS(Status))
     {
	SetLastError(RtlNtStatusToDosError(Status));
	return(FALSE);
     }
   return(TRUE);
}

WINBOOL STDCALL VirtualFree(LPVOID lpAddress, DWORD dwSize, DWORD dwFreeType)
{
   return(VirtualFreeEx(GetCurrentProcess(),lpAddress,dwSize,dwFreeType));
}

WINBOOL STDCALL VirtualProtect(LPVOID lpAddress,
			       DWORD dwSize,
			       DWORD flNewProtect,
			       PDWORD lpflOldProtect)
{
   return(VirtualProtectEx(GetCurrentProcess(),
			   lpAddress, 
			   dwSize, 
			   flNewProtect, 
			   lpflOldProtect));
}


WINBOOL STDCALL VirtualProtectEx(HANDLE hProcess,
				 LPVOID lpAddress,
				 DWORD dwSize,
				 DWORD flNewProtect,
				 PDWORD lpflOldProtect)
{
   NTSTATUS Status;
   
   Status = ZwProtectVirtualMemory(hProcess,
				   (PVOID)lpAddress,
				   dwSize,
				   flNewProtect,
				   (PULONG)lpflOldProtect);
   if (Status != STATUS_SUCCESS)
     {
	SetLastError(RtlNtStatusToDosError(Status));
	return(FALSE);
     }
   return(TRUE);
}


WINBOOL
STDCALL
VirtualLock (
	LPVOID	lpAddress,
	DWORD	dwSize
	)
{
	ULONG BytesLocked;
	NTSTATUS Status;
	Status = NtLockVirtualMemory(NtCurrentProcess(),lpAddress,dwSize, &BytesLocked);
	if (!NT_SUCCESS(Status))
        {
		SetLastError(RtlNtStatusToDosError(Status));
		return FALSE;
     	}
	return TRUE;
}


DWORD
STDCALL
VirtualQuery (
	LPCVOID				lpAddress,
	PMEMORY_BASIC_INFORMATION	lpBuffer,
	DWORD				dwLength
	)
{
	return VirtualQueryEx (NtCurrentProcess(),lpAddress, lpBuffer, dwLength );
}

#define MemoryBasicInformation 0
DWORD
STDCALL
VirtualQueryEx (
	HANDLE				hProcess,
	LPCVOID				lpAddress,
	PMEMORY_BASIC_INFORMATION	lpBuffer,
	DWORD				dwLength
	)
{
	NTSTATUS Status;
	ULONG ResultLength;
	
	Status = NtQueryVirtualMemory(
		hProcess,(LPVOID)lpAddress,
		MemoryBasicInformation, lpBuffer,
		sizeof(MEMORY_BASIC_INFORMATION),
		&ResultLength );
	
	if (!NT_SUCCESS(Status))
        {
		SetLastError(RtlNtStatusToDosError(Status));
		return ResultLength;
     	}
	return ResultLength;
}


WINBOOL
STDCALL
VirtualUnlock (
	LPVOID	lpAddress,
	DWORD	dwSize
	)
{
	ULONG BytesLocked;
	NTSTATUS Status;
	Status = NtUnlockVirtualMemory(NtCurrentProcess(),lpAddress,dwSize, &BytesLocked);
	if (!NT_SUCCESS(Status))
        {
		SetLastError(RtlNtStatusToDosError(Status));
		return FALSE;
     	}
	return TRUE;
}
