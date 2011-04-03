/*
 * COPYRIGHT: See COPYING in the top level directory
 * PROJECT  : ReactOS user mode libraries
 * MODULE   : kernel32.dll
 * FILE     : reactos/dll/win32/kernel32/misc/ldr.c
 * AUTHOR   : Aleksey Bragin <aleksey@reactos.org>
 *            Ariadne
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
extern WaitForInputIdleType lpfnGlobalRegisterWaitForInputIdle;

#define BASEP_GET_MODULE_HANDLE_EX_PARAMETER_VALIDATION_FAIL     1
#define BASEP_GET_MODULE_HANDLE_EX_PARAMETER_VALIDATION_SUCCESS  2
#define BASEP_GET_MODULE_HANDLE_EX_PARAMETER_VALIDATION_CONTINUE 3

/* FUNCTIONS ****************************************************************/

DWORD
WINAPI
BasepGetModuleHandleExParameterValidation(DWORD dwFlags,
                                          LPCWSTR lpwModuleName,
                                          HMODULE *phModule)
{
    /* Set phModule to 0 if it's not a NULL pointer */
    if (phModule) *phModule = 0;

    /* Check for invalid flags combination */
    if (dwFlags & ~(GET_MODULE_HANDLE_EX_FLAG_PIN |
                    GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT |
                    GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS) ||
        (dwFlags & GET_MODULE_HANDLE_EX_FLAG_PIN &&
         dwFlags & GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT) ||
         (!lpwModuleName && (dwFlags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS))
        )
    {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER_1);
        return BASEP_GET_MODULE_HANDLE_EX_PARAMETER_VALIDATION_FAIL;
    }

    /* Check 2nd parameter */
    if (!phModule)
    {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER_2);
        return BASEP_GET_MODULE_HANDLE_EX_PARAMETER_VALIDATION_FAIL;
    }

    /* Return what we have according to the module name */
    if (lpwModuleName)
    {
        return BASEP_GET_MODULE_HANDLE_EX_PARAMETER_VALIDATION_CONTINUE;
    }

    /* No name given, so put ImageBaseAddress there */
    *phModule = (HMODULE)NtCurrentPeb()->ImageBaseAddress;

    return BASEP_GET_MODULE_HANDLE_EX_PARAMETER_VALIDATION_SUCCESS;
}

/**
 * @name GetDllLoadPath
 *
 * Internal function to compute the load path to use for a given dll.
 *
 * @remarks Returned pointer must be freed by caller.
 */

LPWSTR
GetDllLoadPath(LPCWSTR lpModule)
{
	ULONG Pos = 0, Length = 0;
	PWCHAR EnvironmentBufferW = NULL;
	LPCWSTR lpModuleEnd = NULL;
	UNICODE_STRING ModuleName;
	DWORD LastError = GetLastError(); /* GetEnvironmentVariable changes LastError */

	if ((lpModule != NULL) && (wcslen(lpModule) > 2) && (lpModule[1] == ':'))
	{
		lpModuleEnd = lpModule + wcslen(lpModule);
	}
	else
	{
		ModuleName = NtCurrentPeb()->ProcessParameters->ImagePathName;
		lpModule = ModuleName.Buffer;
		lpModuleEnd = lpModule + (ModuleName.Length / sizeof(WCHAR));
	}

	if (lpModule != NULL)
	{
		while (lpModuleEnd > lpModule && *lpModuleEnd != L'/' &&
		       *lpModuleEnd != L'\\' && *lpModuleEnd != L':')
		{
			--lpModuleEnd;
		}
		Length = (lpModuleEnd - lpModule) + 1;
	}

	Length += GetCurrentDirectoryW(0, NULL);
	Length += GetDllDirectoryW(0, NULL);
	Length += GetSystemDirectoryW(NULL, 0);
	Length += GetWindowsDirectoryW(NULL, 0);
	Length += GetEnvironmentVariableW(L"PATH", NULL, 0);

	EnvironmentBufferW = RtlAllocateHeap(RtlGetProcessHeap(), 0,
	                                     Length * sizeof(WCHAR));
	if (EnvironmentBufferW == NULL)
	{
		return NULL;
	}

	if (lpModule)
	{
		RtlCopyMemory(EnvironmentBufferW, lpModule,
		              (lpModuleEnd - lpModule) * sizeof(WCHAR));
		Pos += lpModuleEnd - lpModule;
		EnvironmentBufferW[Pos++] = L';';
	}

	Pos += GetCurrentDirectoryW(Length, EnvironmentBufferW + Pos);
	EnvironmentBufferW[Pos++] = L';';
	Pos += GetDllDirectoryW(Length - Pos, EnvironmentBufferW + Pos);
	EnvironmentBufferW[Pos++] = L';';
	Pos += GetSystemDirectoryW(EnvironmentBufferW + Pos, Length - Pos);
	EnvironmentBufferW[Pos++] = L';';
	Pos += GetWindowsDirectoryW(EnvironmentBufferW + Pos, Length - Pos);
	EnvironmentBufferW[Pos++] = L';';
	Pos += GetEnvironmentVariableW(L"PATH", EnvironmentBufferW + Pos, Length - Pos);

	SetLastError(LastError);
	return EnvironmentBufferW;
}

/*
 * @implemented
 */
BOOL
WINAPI
DisableThreadLibraryCalls(
    IN HMODULE hLibModule)
{
    NTSTATUS Status;

    Status = LdrDisableThreadCalloutsForDll((PVOID)hLibModule);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }
    return TRUE;
}


/*
 * @implemented
 */
HINSTANCE
WINAPI
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
WINAPI
LoadLibraryExA(
    LPCSTR lpLibFileName,
    HANDLE hFile,
    DWORD dwFlags)
{
   PUNICODE_STRING FileNameW;

    if (!(FileNameW = Basep8BitStringToStaticUnicodeString(lpLibFileName)))
        return NULL;

    return LoadLibraryExW(FileNameW->Buffer, hFile, dwFlags);
}


/*
 * @implemented
 */
HINSTANCE
WINAPI
LoadLibraryW (
	LPCWSTR	lpLibFileName
	)
{
	return LoadLibraryExW (lpLibFileName, 0, 0);
}


static
NTSTATUS
LoadLibraryAsDatafile(PWSTR path, LPCWSTR name, HMODULE* hmod)
{
    static const WCHAR dotDLL[] = {'.','d','l','l',0};

    WCHAR filenameW[MAX_PATH];
    HANDLE hFile = INVALID_HANDLE_VALUE;
    HANDLE mapping;
    HMODULE module;

    *hmod = 0;

    if (!SearchPathW( path, name, dotDLL, sizeof(filenameW) / sizeof(filenameW[0]),
                     filenameW, NULL ))
    {
        return NtCurrentTeb()->LastStatusValue;
    }

    hFile = CreateFileW( filenameW, GENERIC_READ, FILE_SHARE_READ,
                         NULL, OPEN_EXISTING, 0, 0 );

    if (hFile == INVALID_HANDLE_VALUE) return NtCurrentTeb()->LastStatusValue;

    mapping = CreateFileMappingW( hFile, NULL, PAGE_READONLY, 0, 0, NULL );
    CloseHandle( hFile );
    if (!mapping) return NtCurrentTeb()->LastStatusValue;

    module = MapViewOfFile( mapping, FILE_MAP_READ, 0, 0, 0 );
    CloseHandle( mapping );
    if (!module) return NtCurrentTeb()->LastStatusValue;

    /* make sure it's a valid PE file */
    if (!RtlImageNtHeader(module))
    {
        UnmapViewOfFile( module );
        return STATUS_INVALID_IMAGE_FORMAT;
    }
    *hmod = (HMODULE)((char *)module + 1);  /* set low bit of handle to indicate datafile module */
    return STATUS_SUCCESS;
}


/*
 * @implemented
 */
HINSTANCE
WINAPI
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
    ULONG DllCharacteristics = 0;
	BOOL FreeString = FALSE;

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

	if (DllName.Buffer[DllName.Length/sizeof(WCHAR) - 1] == L' ')
	{
		RtlCreateUnicodeString(&DllName, (LPWSTR)lpLibFileName);
		while (DllName.Length > sizeof(WCHAR) &&
				DllName.Buffer[DllName.Length/sizeof(WCHAR) - 1] == L' ')
		{
			DllName.Length -= sizeof(WCHAR);
		}
		DllName.Buffer[DllName.Length/sizeof(WCHAR)] = UNICODE_NULL;
		FreeString = TRUE;
	}

    if (dwFlags & LOAD_LIBRARY_AS_DATAFILE)
    {
        Status = LdrGetDllHandle(SearchPath, NULL, &DllName, (PVOID*)&hInst);
        if (!NT_SUCCESS(Status))
        {
            /* The method in load_library_as_datafile allows searching for the
             * 'native' libraries only
             */
            Status = LoadLibraryAsDatafile(SearchPath, DllName.Buffer, &hInst);
            goto done;
        }
    }

    /* HACK!!! FIXME */
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

done:
	RtlFreeHeap(RtlGetProcessHeap(), 0, SearchPath);
	if (FreeString)
		RtlFreeUnicodeString(&DllName);
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
WINAPI
GetProcAddress( HMODULE hModule, LPCSTR lpProcName )
{
	ANSI_STRING ProcedureName;
	FARPROC fnExp = NULL;
	NTSTATUS Status;

	if (!hModule)
	{
		SetLastError(ERROR_PROC_NOT_FOUND);
		return NULL;
	}

	if (HIWORD(lpProcName) != 0)
	{
		RtlInitAnsiString (&ProcedureName,
		                   (LPSTR)lpProcName);
		Status = LdrGetProcedureAddress ((PVOID)hModule,
		                        &ProcedureName,
		                        0,
		                        (PVOID*)&fnExp);
	}
	else
	{
		Status = LdrGetProcedureAddress ((PVOID)hModule,
		                        NULL,
		                        (ULONG)lpProcName,
		                        (PVOID*)&fnExp);
	}

	if (!NT_SUCCESS(Status))
	{
		SetLastErrorByStatus(Status);
		fnExp = NULL;
	}

	return fnExp;
}


/*
 * @implemented
 */
BOOL WINAPI FreeLibrary(HINSTANCE hLibModule)
{
    NTSTATUS Status;

    if ((ULONG_PTR)hLibModule & 1)
    {
        /* This is a LOAD_LIBRARY_AS_DATAFILE module */
        if (RtlImageNtHeader((PVOID)((ULONG_PTR)hLibModule & ~1)))
        {
            /* Unmap view */
            Status = NtUnmapViewOfSection(NtCurrentProcess(), (PVOID)((ULONG_PTR)hLibModule & ~1));

            /* Unload alternate resource module */
            LdrUnloadAlternateResourceModule(hLibModule);
        }
        else
            Status = STATUS_INVALID_IMAGE_FORMAT;
    }
    else
    {
        /* Just unload it */
        Status = LdrUnloadDll((PVOID)hLibModule);
    }

    /* Check what kind of status we got */
    if (!NT_SUCCESS(Status))
    {
        /* Set last error */
        BaseSetLastNTError(Status);

        /* Return failure */
        return FALSE;
    }

    /* Return success */
    return TRUE;
}


/*
 * @implemented
 */
VOID
WINAPI
FreeLibraryAndExitThread(HMODULE hLibModule,
                         DWORD dwExitCode)
{
    NTSTATUS Status;

    if ((ULONG_PTR)hLibModule & 1)
    {
        /* This is a LOAD_LIBRARY_AS_DATAFILE module */
        if (RtlImageNtHeader((PVOID)((ULONG_PTR)hLibModule & ~1)))
        {
            /* Unmap view */
            Status = NtUnmapViewOfSection(NtCurrentProcess(), (PVOID)((ULONG_PTR)hLibModule & ~1));

            /* Unload alternate resource module */
            LdrUnloadAlternateResourceModule(hLibModule);
        }
    }
    else
    {
        /* Just unload it */
        Status = LdrUnloadDll((PVOID)hLibModule);
    }

    /* Exit thread */
    ExitThread(dwExitCode);
}


/*
 * @implemented
 */
DWORD
WINAPI
GetModuleFileNameA(HINSTANCE hModule,
                   LPSTR lpFilename,
                   DWORD nSize)
{
    UNICODE_STRING filenameW;
    ANSI_STRING FilenameA;
    NTSTATUS Status;
    DWORD Length = 0;

    /* Allocate a unicode buffer */
    filenameW.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, nSize * sizeof(WCHAR));
    if (!filenameW.Buffer)
    {
        BaseSetLastNTError(STATUS_NO_MEMORY);
        return 0;
    }

    /* Call unicode API */
    filenameW.Length = GetModuleFileNameW(hModule, filenameW.Buffer, nSize) * sizeof(WCHAR);
    filenameW.MaximumLength = filenameW.Length + sizeof(WCHAR);

    if (filenameW.Length)
    {
        /* Convert to ansi string */
        Status = BasepUnicodeStringTo8BitString(&FilenameA, &filenameW, TRUE);
        if (!NT_SUCCESS(Status))
        {
            /* Set last error, free string and retun failure */
            BaseSetLastNTError(Status);
            RtlFreeUnicodeString(&filenameW);
            return 0;
        }

        /* Calculate size to copy */
        Length = min(nSize, FilenameA.Length);

        /* Remove terminating zero */
        if (Length == FilenameA.Length)
            Length--;

        /* Now copy back to the caller amount he asked */
        RtlMoveMemory(lpFilename, FilenameA.Buffer, Length);

        /* Free ansi filename */
        RtlFreeAnsiString(&FilenameA);
    }

    /* Free unicode filename */
    RtlFreeHeap(RtlGetProcessHeap(), 0, filenameW.Buffer);

    /* Return length copied */
    return Length;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetModuleFileNameW(HINSTANCE hModule,
                   LPWSTR lpFilename,
                   DWORD nSize)
{
    PLIST_ENTRY ModuleListHead, Entry;
    PLDR_DATA_TABLE_ENTRY Module;
    ULONG Length = 0;
    ULONG Cookie;
    PPEB Peb;

    /* Upscale nSize from chars to bytes */
    nSize *= sizeof(WCHAR);

    _SEH2_TRY
    {
        /* We don't use per-thread cur dir now */
        //PRTL_PERTHREAD_CURDIR PerThreadCurdir = (PRTL_PERTHREAD_CURDIR)teb->NtTib.SubSystemTib;

        Peb = NtCurrentPeb ();

        /* Acquire a loader lock */
        LdrLockLoaderLock(LDR_LOCK_LOADER_LOCK_FLAG_RAISE_STATUS, NULL, &Cookie);

        /* Traverse the module list */
        ModuleListHead = &Peb->Ldr->InLoadOrderModuleList;
        Entry = ModuleListHead->Flink;
        while (Entry != ModuleListHead)
        {
            Module = CONTAINING_RECORD(Entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

            /* Check if this is the requested module */
            if (Module->DllBase == (PVOID)hModule)
            {
                /* Calculate size to copy */
                Length = min(nSize, Module->FullDllName.MaximumLength);

                /* Copy contents */
                RtlMoveMemory(lpFilename, Module->FullDllName.Buffer, Length);

                /* Subtract a terminating zero */
                if (Length == Module->FullDllName.MaximumLength)
                    Length -= sizeof(WCHAR);

                /* Break out of the loop */
                break;
            }

            /* Advance to the next entry */
            Entry = Entry->Flink;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        BaseSetLastNTError(_SEH2_GetExceptionCode());
        Length = 0;
    } _SEH2_END

    /* Release the loader lock */
    LdrUnlockLoaderLock(LDR_LOCK_LOADER_LOCK_FLAG_RAISE_STATUS, Cookie);

    return Length;
}

HMODULE
WINAPI
GetModuleHandleForUnicodeString(PUNICODE_STRING ModuleName)
{
    NTSTATUS Status;
    PVOID Module;
    LPWSTR DllPath;

    /* Try to get a handle with a magic value of 1 for DllPath */
    Status = LdrGetDllHandle((LPWSTR)1, NULL, ModuleName, &Module);

    /* If that succeeded - we're done */
    if (NT_SUCCESS(Status)) return Module;

    /* If not, then the path should be computed */
    DllPath = BasepGetDllPath(NULL, 0);

    /* Call LdrGetHandle() again providing the computed DllPath
       and wrapped into SEH */
    _SEH2_TRY
    {
        Status = LdrGetDllHandle(DllPath, NULL, ModuleName, &Module);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Fail with the SEH error */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* Free the DllPath */
    RtlFreeHeap(RtlGetProcessHeap(), 0, DllPath);

    /* In case of error set last win32 error and return NULL */
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failure acquiring DLL module '%wZ' handle, Status 0x%08X\n", ModuleName, Status);
        SetLastErrorByStatus(Status);
        Module = 0;
    }

    /* Return module */
    return (HMODULE)Module;
}

BOOLEAN
WINAPI
BasepGetModuleHandleExW(BOOLEAN NoLock, DWORD dwPublicFlags, LPCWSTR lpwModuleName, HMODULE *phModule)
{
    DWORD Cookie;
    NTSTATUS Status = STATUS_SUCCESS, Status2;
    HANDLE hModule = 0;
    UNICODE_STRING ModuleNameU;
    DWORD dwValid;
    BOOLEAN Redirected = FALSE; // FIXME

    /* Validate parameters */
    dwValid = BasepGetModuleHandleExParameterValidation(dwPublicFlags, lpwModuleName, phModule);
    ASSERT(dwValid == BASEP_GET_MODULE_HANDLE_EX_PARAMETER_VALIDATION_CONTINUE);

    /* Acquire lock if necessary */
    if (!NoLock)
    {
        Status = LdrLockLoaderLock(0, NULL, &Cookie);
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            SetLastErrorByStatus(Status);
            if (phModule) *phModule = 0;
            return Status;
        }
    }

    if (!(dwPublicFlags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS))
    {
        /* Create a unicode string out of module name */
        RtlInitUnicodeString(&ModuleNameU, lpwModuleName);

        // FIXME: Do some redirected DLL stuff?
        if (Redirected)
        {
            UNIMPLEMENTED;
        }

        if (!hModule)
        {
            hModule = GetModuleHandleForUnicodeString(&ModuleNameU);
            if (!hModule)
            {
                // FIXME: Status?!
                goto quickie;
            }
        }
    }
    else
    {
        /* Perform Pc to file header to get module instance */
        hModule = (HMODULE)RtlPcToFileHeader((PVOID)lpwModuleName,
                                             (PVOID*)&hModule);

        /* Check if it succeeded */
        if (!hModule)
        {
            /* Set "dll not found" status and quit */
            Status = STATUS_DLL_NOT_FOUND;
            goto quickie;
        }
    }

    /* Check if changing reference is not forbidden */
    if (!(dwPublicFlags & GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT))
    {
        /* Add reference to this DLL */
        Status = LdrAddRefDll((dwPublicFlags & GET_MODULE_HANDLE_EX_FLAG_PIN) ? LDR_PIN_MODULE : 0,
                              hModule);
    }

quickie:
    /* Unlock loader lock if it was acquired */
    if (!NoLock)
    {
        Status2 = LdrUnlockLoaderLock(0, Cookie);
        ASSERT(NT_SUCCESS(Status2));
    }

    /* Set last error in case of failure */
    if (!NT_SUCCESS(Status))
        SetLastErrorByStatus(Status);

    /* Set the module handle to the caller */
    if (phModule) *phModule = hModule;

    /* Return TRUE on success and FALSE otherwise */
    return NT_SUCCESS(Status);
}

/*
 * @implemented
 */
HMODULE
WINAPI
GetModuleHandleA(LPCSTR lpModuleName)
{
    PUNICODE_STRING ModuleNameW;
    PTEB pTeb = NtCurrentTeb();

    /* Check if we have no name to convert */
    if (!lpModuleName)
        return ((HMODULE)pTeb->ProcessEnvironmentBlock->ImageBaseAddress);

    /* Convert module name to unicode */
    ModuleNameW = Basep8BitStringToStaticUnicodeString(lpModuleName);

    /* Call W version if conversion was successful */
    if (ModuleNameW)
        return GetModuleHandleW(ModuleNameW->Buffer);

    /* Return failure */
    return 0;
}


/*
 * @implemented
 */
HMODULE
WINAPI
GetModuleHandleW(LPCWSTR lpModuleName)
{
    HMODULE hModule;
    NTSTATUS Status;

    /* If current module is requested - return it right away */
    if (!lpModuleName)
        return ((HMODULE)NtCurrentPeb()->ImageBaseAddress);

    /* Use common helper routine */
    Status = BasepGetModuleHandleExW(TRUE,
                                     GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                     lpModuleName,
                                     &hModule);

    /* If it wasn't successful - return 0 */
    if (!NT_SUCCESS(Status)) hModule = 0;

    /* Return the handle */
    return hModule;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetModuleHandleExW(IN DWORD dwFlags,
                   IN LPCWSTR lpwModuleName  OPTIONAL,
                   OUT HMODULE* phModule)
{
    NTSTATUS Status;
    DWORD dwValid;
    BOOL Ret = FALSE;

    /* Validate parameters */
    dwValid = BasepGetModuleHandleExParameterValidation(dwFlags, lpwModuleName, phModule);

    /* If result is invalid parameter - return failure */
    if (dwValid == BASEP_GET_MODULE_HANDLE_EX_PARAMETER_VALIDATION_FAIL) return FALSE;

    /* If result is 2, there is no need to do anything - return success. */
    if (dwValid == BASEP_GET_MODULE_HANDLE_EX_PARAMETER_VALIDATION_SUCCESS) return TRUE;

    /* Use common helper routine */
    Status = BasepGetModuleHandleExW(FALSE,
                                     dwFlags,
                                     lpwModuleName,
                                     phModule);

    /* Return TRUE in case of success */
    if (NT_SUCCESS(Status)) Ret = TRUE;

    return Ret;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetModuleHandleExA(IN DWORD dwFlags,
                   IN LPCSTR lpModuleName OPTIONAL,
                   OUT HMODULE* phModule)
{
    PUNICODE_STRING lpModuleNameW;
    DWORD dwValid;
    BOOL Ret;

    /* Validate parameters */
    dwValid = BasepGetModuleHandleExParameterValidation(dwFlags, (LPCWSTR)lpModuleName, phModule);

    /* If result is invalid parameter - return failure */
    if (dwValid == BASEP_GET_MODULE_HANDLE_EX_PARAMETER_VALIDATION_FAIL) return FALSE;

    /* If result is 2, there is no need to do anything - return success. */
    if (dwValid == BASEP_GET_MODULE_HANDLE_EX_PARAMETER_VALIDATION_SUCCESS) return TRUE;

    /* Check if we don't need to convert the name */
    if (dwFlags & GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS)
    {
        /* Call the W version of the API without conversion */
        Ret = GetModuleHandleExW(dwFlags,
                                 (LPCWSTR)lpModuleName,
                                 phModule);
    }
    else
    {
        /* Convert module name to unicode */
        lpModuleNameW = Basep8BitStringToStaticUnicodeString(lpModuleName);

        /* Return FALSE if conversion failed */
        if (!lpModuleNameW) return FALSE;

        /* Call the W version of the API */
        Ret = GetModuleHandleExW(dwFlags,
                                 lpModuleNameW->Buffer,
                                 phModule);
    }

    /* Return result */
    return Ret;
}


/*
 * @implemented
 */
DWORD
WINAPI
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
  if(!(CommandLine = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY,
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

    RtlFreeHeap(RtlGetProcessHeap(), 0, CommandLine);
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

  RtlFreeHeap(RtlGetProcessHeap(), 0, CommandLine);

  /* Wait up to 15 seconds for the process to become idle */
  if (NULL != lpfnGlobalRegisterWaitForInputIdle)
  {
    lpfnGlobalRegisterWaitForInputIdle(ProcessInformation.hProcess, 15000);
  }

  NtClose(ProcessInformation.hThread);
  NtClose(ProcessInformation.hProcess);

  return 33;
}

/* EOF */
