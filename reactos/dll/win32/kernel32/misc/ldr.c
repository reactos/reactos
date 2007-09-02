/* $Id$
 *
 * COPYRIGHT: See COPYING in the top level directory
 * PROJECT  : ReactOS user mode libraries
 * MODULE   : kernel32.dll
 * FILE     : reactos/lib/kernel32/misc/ldr.c
 * AUTHOR   : Ariadne
 */

#include <k32.h>

#define NDEBUG
#include <debug.h>

typedef struct tagLOADPARMS32 {
  LPSTR lpEnvAddress;
  LPSTR lpCmdLine;
  LPSTR lpCmdShow;
  DWORD dwReserved;
} LOADPARMS32;

extern BOOLEAN InWindows;

/* FUNCTIONS ****************************************************************/

/**
 * @name GetDllLoadPath
 *
 * Internal function to compute the load path to use for a given dll.
 *
 * @remarks Returned pointer must be freed by caller.
 */

LPWSTR STDCALL
GetDllLoadPath(LPCWSTR lpModule)
{
        ULONG Pos = 0, Length = 0;
        PWCHAR EnvironmentBufferW = NULL;
        LPCWSTR lpModuleEnd = NULL;
        UNICODE_STRING ModuleName;

	if (lpModule != NULL)
	{
		lpModuleEnd = lpModule + wcslen(lpModule);
	}
	else
	{
	        ModuleName = NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters->ImagePathName;
	        lpModule = ModuleName.Buffer;
	        lpModuleEnd = lpModule + (ModuleName.Length / sizeof(WCHAR));
	}

	if (lpModule != NULL)
	{
	        while (lpModuleEnd > lpModule && *lpModuleEnd != L'/' &&
	               *lpModuleEnd != L'\\' && *lpModuleEnd != L':')
			--lpModuleEnd;
		Length = (lpModuleEnd - lpModule) + 1;
	}

	Length += GetCurrentDirectoryW(0, NULL);
	Length += GetSystemDirectoryW(NULL, 0);
	Length += GetWindowsDirectoryW(NULL, 0);
	Length += GetEnvironmentVariableW(L"PATH", NULL, 0);

	EnvironmentBufferW = RtlAllocateHeap(RtlGetProcessHeap(), 0,
                                             Length * sizeof(WCHAR));
	if (EnvironmentBufferW == NULL)
		return NULL;

	if (lpModule)
	{
		RtlCopyMemory(EnvironmentBufferW, lpModule,
		              (lpModuleEnd - lpModule) * sizeof(WCHAR));
		Pos += lpModuleEnd - lpModule;
		EnvironmentBufferW[Pos++] = L';';
	}
	Pos += GetCurrentDirectoryW(Length, EnvironmentBufferW + Pos);
	EnvironmentBufferW[Pos++] = L';';
	Pos += GetSystemDirectoryW(EnvironmentBufferW + Pos, Length - Pos);
	EnvironmentBufferW[Pos++] = L';';
	Pos += GetWindowsDirectoryW(EnvironmentBufferW + Pos, Length - Pos);
	EnvironmentBufferW[Pos++] = L';';
	Pos += GetEnvironmentVariableW(L"PATH", EnvironmentBufferW + Pos, Length - Pos);
	EnvironmentBufferW[Pos] = 0;

	return EnvironmentBufferW;
}

/*
 * @implemented
 */
BOOL
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


/*
 * @implemented
 */
HINSTANCE
STDCALL
LoadLibraryA (
	LPCSTR	lpLibFileName
	)
{
	return LoadLibraryExA (lpLibFileName, 0, 0);
}


/*
 * @implemented
 */
HINSTANCE
STDCALL
LoadLibraryExA (
	LPCSTR	lpLibFileName,
	HANDLE	hFile,
	DWORD	dwFlags
	)
{
   PWCHAR FileNameW;

   if (!(FileNameW = FilenameA2W(lpLibFileName, FALSE)))
      return FALSE;

   return LoadLibraryExW(FileNameW, hFile, dwFlags);
}


/*
 * @implemented
 */
HINSTANCE
STDCALL
LoadLibraryW (
	LPCWSTR	lpLibFileName
	)
{
	return LoadLibraryExW (lpLibFileName, 0, 0);
}


/*
 * @implemented
 */
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
	PWSTR SearchPath;
    ULONG DllCharacteristics;

        (void)hFile;

	if ( lpLibFileName == NULL )
		return NULL;

    /* Check for any flags LdrLoadDll might be interested in */
    if (dwFlags & DONT_RESOLVE_DLL_REFERENCES)
    {
        /* Tell LDR to treat it as an EXE */
        DllCharacteristics = IMAGE_FILE_EXECUTABLE_IMAGE;
    }

	dwFlags &=
	  DONT_RESOLVE_DLL_REFERENCES |
	  LOAD_LIBRARY_AS_DATAFILE |
	  LOAD_WITH_ALTERED_SEARCH_PATH;

	SearchPath = GetDllLoadPath(
	  dwFlags & LOAD_WITH_ALTERED_SEARCH_PATH ? lpLibFileName : NULL);

	RtlInitUnicodeString(&DllName, (LPWSTR)lpLibFileName);
    if (InWindows)
    {
        /* Call the API Properly */
        Status = LdrLoadDll(SearchPath,
                            &DllCharacteristics,
                            &DllName,
                            (PVOID*)&hInst);
    }
    else
    {
        /* Call the ROS API. NOTE: Don't fix this, I have a patch to merge later. */
        Status = LdrLoadDll(SearchPath, &dwFlags, &DllName, (PVOID*)&hInst);
    }
	RtlFreeHeap(RtlGetProcessHeap(), 0, SearchPath);
	if ( !NT_SUCCESS(Status))
	{
		SetLastErrorByStatus (Status);
		return NULL;
	}

	return hInst;
}


/*
 * @implemented
 */
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


/*
 * @implemented
 */
BOOL
STDCALL
FreeLibrary( HMODULE hLibModule )
{
	LdrUnloadDll(hLibModule);
	return TRUE;
}


/*
 * @implemented
 */
VOID
STDCALL
FreeLibraryAndExitThread (
	HMODULE	hLibModule,
	DWORD	dwExitCode
	)
{
	if ( FreeLibrary(hLibModule) )
		ExitThread(dwExitCode);
	for (;;)
		;
}


/*
 * @implemented
 */
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
	PLDR_DATA_TABLE_ENTRY Module;
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
		Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
		if (Module->DllBase == (PVOID)hModule)
		{
			if (nSize * sizeof(WCHAR) < Module->FullDllName.Length)
			{
				SetLastErrorByStatus (STATUS_BUFFER_TOO_SMALL);
			}
			else
			{
				FileName.Length = 0;
				FileName.MaximumLength = (USHORT)nSize * sizeof(WCHAR);
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


/*
 * @implemented
 */
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
	PLDR_DATA_TABLE_ENTRY Module;
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
		Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

		if (Module->DllBase == (PVOID)hModule)
		{
			if (nSize * sizeof(WCHAR) < Module->FullDllName.Length)
			{
				SetLastErrorByStatus (STATUS_BUFFER_TOO_SMALL);
			}
			else
			{
				FileName.Length = 0;
				FileName.MaximumLength =(USHORT)nSize * sizeof(WCHAR);
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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
BOOL
STDCALL
GetModuleHandleExW(IN DWORD dwFlags,
                   IN LPCWSTR lpModuleName  OPTIONAL,
                   OUT HMODULE* phModule)
{
    HMODULE hModule;
    NTSTATUS Status;
    BOOL Ret = FALSE;

    if (phModule == NULL ||
        ((dwFlags & (GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT)) ==
         (GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (lpModuleName == NULL)
    {
        hModule = NtCurrentPeb()->ImageBaseAddress;
    }
    else
    {
        if (dwFlags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS)
        {
            hModule = (HMODULE)RtlPcToFileHeader((PVOID)lpModuleName,
                                                 (PVOID*)&hModule);
            if (hModule == NULL)
            {
                SetLastErrorByStatus(STATUS_DLL_NOT_FOUND);
            }
        }
        else
        {
            hModule = GetModuleHandleW(lpModuleName);
        }
    }

    if (hModule != NULL)
    {
        if (!(dwFlags & GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT))
        {
            Status = LdrAddRefDll((dwFlags & GET_MODULE_HANDLE_EX_FLAG_PIN) ? LDR_PIN_MODULE : 0,
                                  hModule);

            if (NT_SUCCESS(Status))
            {
                Ret = TRUE;
            }
            else
            {
                SetLastErrorByStatus(Status);
                hModule = NULL;
            }
        }
        else
            Ret = TRUE;
    }

    *phModule = hModule;
    return Ret;
}

/*
 * @implemented
 */
BOOL
STDCALL
GetModuleHandleExA(IN DWORD dwFlags,
                   IN LPCSTR lpModuleName  OPTIONAL,
                   OUT HMODULE* phModule)
{
    UNICODE_STRING UnicodeName;
    ANSI_STRING ModuleName;
    LPCWSTR lpModuleNameW;
    NTSTATUS Status;
    BOOL Ret;

    if (dwFlags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS)
    {
        lpModuleNameW = (LPCWSTR)lpModuleName;
    }
    else
    {
        RtlInitAnsiString(&ModuleName,
                          (LPSTR)lpModuleName);

        /* convert ansi (or oem) string to unicode */
        if (bIsFileApiAnsi)
            Status = RtlAnsiStringToUnicodeString(&UnicodeName,
                                                  &ModuleName,
                                                  TRUE);
        else
            Status = RtlOemStringToUnicodeString(&UnicodeName,
                                                 &ModuleName,
                                                 TRUE);

        if (!NT_SUCCESS(Status))
        {
            SetLastErrorByStatus(Status);
            return FALSE;
        }

        lpModuleNameW = UnicodeName.Buffer;
    }

    Ret = GetModuleHandleExW(dwFlags,
                             lpModuleNameW,
                             phModule);

    if (!(dwFlags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS))
    {
        RtlFreeUnicodeString(&UnicodeName);
    }

    return Ret;
}


/*
 * @implemented
 */
DWORD
STDCALL
LoadModule (
    LPCSTR  lpModuleName,
    LPVOID  lpParameterBlock
    )
{
  STARTUPINFOA StartupInfo;
  PROCESS_INFORMATION ProcessInformation;
  LOADPARMS32 *LoadParams;
  char FileName[MAX_PATH];
  char *CommandLine, *t;
  BYTE Length;

  LoadParams = (LOADPARMS32*)lpParameterBlock;
  if(!lpModuleName || !LoadParams || (((WORD*)LoadParams->lpCmdShow)[0] != 2))
  {
    /* windows doesn't check parameters, we do */
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
  }

  if(!SearchPathA(NULL, lpModuleName, ".exe", MAX_PATH, FileName, NULL) &&
     !SearchPathA(NULL, lpModuleName, NULL, MAX_PATH, FileName, NULL))
  {
    return ERROR_FILE_NOT_FOUND;
  }

  Length = (BYTE)LoadParams->lpCmdLine[0];
  if(!(CommandLine = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                               strlen(lpModuleName) + Length + 2)))
  {
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return 0;
  }

  /* Create command line string */
  strcpy(CommandLine, lpModuleName);
  t = CommandLine + strlen(CommandLine);
  *(t++) = ' ';
  memcpy(t, LoadParams->lpCmdLine + 1, Length);

  /* Build StartupInfo */
  RtlZeroMemory(&StartupInfo, sizeof(STARTUPINFOA));
  StartupInfo.cb = sizeof(STARTUPINFOA);
  StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
  StartupInfo.wShowWindow = ((WORD*)LoadParams->lpCmdShow)[1];

  if(!CreateProcessA(FileName, CommandLine, NULL, NULL, FALSE, 0, LoadParams->lpEnvAddress,
                     NULL, &StartupInfo, &ProcessInformation))
  {
    DWORD Error;

    HeapFree(GetProcessHeap(), 0, CommandLine);
    /* return the right value */
    Error = GetLastError();
    switch(Error)
    {
      case ERROR_BAD_EXE_FORMAT:
      {
        return ERROR_BAD_FORMAT;
      }
      case ERROR_FILE_NOT_FOUND:
      case ERROR_PATH_NOT_FOUND:
      {
        return Error;
      }
    }
    return 0;
  }

  HeapFree(GetProcessHeap(), 0, CommandLine);

  /* Wait up to 15 seconds for the process to become idle */
  /* FIXME: This is user32! Windows soft-loads this only if required. */
  //WaitForInputIdle(ProcessInformation.hProcess, 15000);

  CloseHandle(ProcessInformation.hThread);
  CloseHandle(ProcessInformation.hProcess);

  return 33;
}

/* EOF */
