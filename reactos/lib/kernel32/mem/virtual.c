/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/kernel32/mem/virtual.c
 * PURPOSE:              Passing the Virtualxxx functions onto the kernel
 * PROGRAMMER:           David Welch (welch@mcmail.com)
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>

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
