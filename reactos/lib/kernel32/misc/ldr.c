/* $Id: ldr.c,v 1.13 2002/09/08 10:22:44 chorns Exp $
 *
 * COPYRIGHT: See COPYING in the top level directory
 * PROJECT  : ReactOS user mode libraries
 * MODULE   : kernel32.dll
 * FILE     : reactos/lib/kernel32/misc/ldr.c
 * AUTHOR   : Boudewijn Dekker
 */

#include <ddk/ntddk.h>
#include <ntdll/ldr.h>
#include <windows.h>

#define NDEBUG
#include <kernel32/kernel32.h>
#include <kernel32/error.h>


/* FUNCTIONS ****************************************************************/

WINBOOL
STDCALL
DisableThreadLibraryCalls (
	HMODULE	hLibModule
	)
{
	NTSTATUS Status;

	Status = LdrDisableThreadCalloutsForDll ((PVOID)hLibModule);
	if (!NT_SUCCESS (Status))
	{
		SetLastErrorByStatus (Status);
		return FALSE;
	}
	return TRUE;
}


HINSTANCE
STDCALL
LoadLibraryA (
	LPCSTR	lpLibFileName
	)
{
	return LoadLibraryExA (lpLibFileName, 0, 0);
}


HINSTANCE
STDCALL
LoadLibraryExA (
	LPCSTR	lpLibFileName,
	HANDLE	hFile,
	DWORD	dwFlags
	)
{
	UNICODE_STRING LibFileNameU;
	ANSI_STRING LibFileName;
	HINSTANCE hInst;
	NTSTATUS Status;

	RtlInitAnsiString (&LibFileName,
	                   (LPSTR)lpLibFileName);

	/* convert ansi (or oem) string to unicode */
	if (bIsFileApiAnsi)
		RtlAnsiStringToUnicodeString (&LibFileNameU,
		                              &LibFileName,
		                              TRUE);
	else
		RtlOemStringToUnicodeString (&LibFileNameU,
		                             &LibFileName,
		                             TRUE);

	Status = LdrLoadDll(NULL,
			    dwFlags,
			    &LibFileNameU,
			    (PVOID*)&hInst);

	RtlFreeUnicodeString (&LibFileNameU);

	if ( !NT_SUCCESS(Status))
	{
		SetLastErrorByStatus (Status);
		return NULL;
	}

	return hInst;
}


HINSTANCE
STDCALL
LoadLibraryW (
	LPCWSTR	lpLibFileName
	)
{
	return LoadLibraryExW (lpLibFileName, 0, 0);
}


HINSTANCE
STDCALL
LoadLibraryExW (
	LPCWSTR	lpLibFileName,
	HANDLE	hFile,
	DWORD	dwFlags
	)
{
	UNICODE_STRING DllName;
	HINSTANCE hInst;
	NTSTATUS Status;

	if ( lpLibFileName == NULL )
		return NULL;

	RtlInitUnicodeString (&DllName, (LPWSTR)lpLibFileName);
	Status = LdrLoadDll(NULL, dwFlags, &DllName, (PVOID*)&hInst);
	if ( !NT_SUCCESS(Status))
	{
		SetLastErrorByStatus (Status);
		return NULL;
	}
	
	return hInst;
}


FARPROC
STDCALL
GetProcAddress( HMODULE hModule, LPCSTR lpProcName )
{
	ANSI_STRING ProcedureName;
	FARPROC fnExp = NULL;

	if (HIWORD(lpProcName) != 0)
	{
		RtlInitAnsiString (&ProcedureName,
		                   (LPSTR)lpProcName);
		LdrGetProcedureAddress ((PVOID)hModule,
		                        &ProcedureName,
		                        0,
		                        (PVOID*)&fnExp);
	}
	else
	{
		LdrGetProcedureAddress ((PVOID)hModule,
		                        NULL,
		                        (ULONG)lpProcName,
		                        (PVOID*)&fnExp);
	}

	return fnExp;
}


WINBOOL
STDCALL
FreeLibrary( HMODULE hLibModule )
{
	LdrUnloadDll(hLibModule);
	return TRUE;
}


VOID
STDCALL
FreeLibraryAndExitThread (
	HMODULE	hLibModule,
	DWORD	dwExitCode
	)
{
	if ( FreeLibrary(hLibModule) )
		ExitThread(dwExitCode);
	return;
}


DWORD
STDCALL
GetModuleFileNameA (
	HINSTANCE	hModule,
	LPSTR		lpFilename,
	DWORD		nSize
	)
{
	ANSI_STRING FileName;
	PLIST_ENTRY ModuleListHead;
	PLIST_ENTRY Entry;
	PLDR_MODULE Module;
	PPEB Peb;
	ULONG Length = 0;

	Peb = NtCurrentPeb ();
	RtlEnterCriticalSection (Peb->LoaderLock);

	if (hModule == NULL)
		hModule = Peb->ImageBaseAddress;

	ModuleListHead = &Peb->Ldr->InLoadOrderModuleList;
	Entry = ModuleListHead->Flink;

	while (Entry != ModuleListHead)
	{
		Module = CONTAINING_RECORD(Entry, LDR_MODULE, InLoadOrderModuleList);
		if (Module->BaseAddress == (PVOID)hModule)
		{
			if (nSize * sizeof(WCHAR) < Module->FullDllName.Length)
			{
				SetLastErrorByStatus (STATUS_BUFFER_TOO_SMALL);
			}
			else
			{
				FileName.Length = 0;
				FileName.MaximumLength = nSize * sizeof(WCHAR);
				FileName.Buffer = lpFilename;

				/* convert unicode string to ansi (or oem) */
				if (bIsFileApiAnsi)
					RtlUnicodeStringToAnsiString (&FileName,
					                              &Module->FullDllName,
					                              FALSE);
				else
					RtlUnicodeStringToOemString (&FileName,
					                             &Module->FullDllName,
					                             FALSE);
				Length = Module->FullDllName.Length / sizeof(WCHAR);
			}

			RtlLeaveCriticalSection (Peb->LoaderLock);
			return Length;
		}

		Entry = Entry->Flink;
	}

	SetLastErrorByStatus (STATUS_DLL_NOT_FOUND);
	RtlLeaveCriticalSection (Peb->LoaderLock);

	return 0;
}


DWORD
STDCALL
GetModuleFileNameW (
	HINSTANCE	hModule,
	LPWSTR		lpFilename,
	DWORD		nSize
	)
{
	UNICODE_STRING FileName;
	PLIST_ENTRY ModuleListHead;
	PLIST_ENTRY Entry;
	PLDR_MODULE Module;
	PPEB Peb;
	ULONG Length = 0;

	Peb = NtCurrentPeb ();
	RtlEnterCriticalSection (Peb->LoaderLock);

	if (hModule == NULL)
		hModule = Peb->ImageBaseAddress;

	ModuleListHead = &Peb->Ldr->InLoadOrderModuleList;
	Entry = ModuleListHead->Flink;
	while (Entry != ModuleListHead)
	{
		Module = CONTAINING_RECORD(Entry, LDR_MODULE, InLoadOrderModuleList);

		if (Module->BaseAddress == (PVOID)hModule)
		{
			if (nSize * sizeof(WCHAR) < Module->FullDllName.Length)
			{
				SetLastErrorByStatus (STATUS_BUFFER_TOO_SMALL);
			}
			else
			{
				FileName.Length = 0;
				FileName.MaximumLength = nSize * sizeof(WCHAR);
				FileName.Buffer = lpFilename;

				RtlCopyUnicodeString (&FileName,
				                      &Module->FullDllName);
				Length = Module->FullDllName.Length / sizeof(WCHAR);
			}

			RtlLeaveCriticalSection (Peb->LoaderLock);
			return Length;
		}

		Entry = Entry->Flink;
	}

	SetLastErrorByStatus (STATUS_DLL_NOT_FOUND);
	RtlLeaveCriticalSection (Peb->LoaderLock);

	return 0;
}


HMODULE
STDCALL
GetModuleHandleA ( LPCSTR lpModuleName )
{
	UNICODE_STRING UnicodeName;
	ANSI_STRING ModuleName;
	PVOID BaseAddress;
	NTSTATUS Status;

	if (lpModuleName == NULL)
		return ((HMODULE)NtCurrentPeb()->ImageBaseAddress);
	RtlInitAnsiString (&ModuleName,
	                   (LPSTR)lpModuleName);

	/* convert ansi (or oem) string to unicode */
	if (bIsFileApiAnsi)
		RtlAnsiStringToUnicodeString (&UnicodeName,
					      &ModuleName,
					      TRUE);
	else
		RtlOemStringToUnicodeString (&UnicodeName,
					     &ModuleName,
					     TRUE);

	Status = LdrGetDllHandle (0,
				  0,
				  &UnicodeName,
				  &BaseAddress);

	RtlFreeUnicodeString (&UnicodeName);

	if (!NT_SUCCESS(Status))
	{
		SetLastErrorByStatus (Status);
		return NULL;
	}

	return ((HMODULE)BaseAddress);
}


HMODULE
STDCALL
GetModuleHandleW (LPCWSTR lpModuleName)
{
	UNICODE_STRING ModuleName;
	PVOID BaseAddress;
	NTSTATUS Status;

	if (lpModuleName == NULL)
		return ((HMODULE)NtCurrentPeb()->ImageBaseAddress);

	RtlInitUnicodeString (&ModuleName,
			      (LPWSTR)lpModuleName);

	Status = LdrGetDllHandle (0,
				  0,
				  &ModuleName,
				  &BaseAddress);
	if (!NT_SUCCESS(Status))
	{
		SetLastErrorByStatus (Status);
		return NULL;
	}

	return ((HMODULE)BaseAddress);
}

/* EOF */
