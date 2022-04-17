/*
 * COPYRIGHT:   See COPYING in the top level directory
 * LICENSE:     See LGPL.txt in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        reactos/lib/psapi/misc/win32.c
 * PURPOSE:     Win32 interfaces for PSAPI
 * PROGRAMMER:  KJK::Hyperion <noog@libero.it>
 *              Thomas Weidenmueller <w3seek@reactos.com>
 *              Pierre Schweitzer <pierre@reactos.org>
 * UPDATE HISTORY:
 *              10/06/2002: Created
 */

#include <stdarg.h>

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#define NTOS_MODE_USER
#include <ndk/exfuncs.h>
#include <ndk/mmfuncs.h>
#include <ndk/psfuncs.h>
#include <ndk/rtlfuncs.h>

#include <psapi.h>

#include <pseh/pseh2.h>

#define NDEBUG
#include <debug.h>

#define MAX_MODULES 0x2710      // Matches 10.000 modules
#define INIT_MEMORY_SIZE 0x1000 // Matches 4kB

/* INTERNAL *******************************************************************/

/*
 * @implemented
 */
static BOOL NTAPI
FindDeviceDriver(IN PVOID ImageBase,
                 OUT PRTL_PROCESS_MODULE_INFORMATION MatchingModule)
{
    NTSTATUS Status;
    DWORD i, RequiredSize;
    PRTL_PROCESS_MODULES Information;
    RTL_PROCESS_MODULE_INFORMATION Module;
    /* By default, to prevent too many reallocations, we already make room for 4 modules */
    DWORD Size = sizeof(RTL_PROCESS_MODULES) + 3 * sizeof(RTL_PROCESS_MODULE_INFORMATION);

    while (TRUE)
    {
        /* Allocate a buffer to hold modules information */
        Information = LocalAlloc(LMEM_FIXED, Size);
        if (!Information)
        {
            SetLastError(ERROR_NO_SYSTEM_RESOURCES);
            return FALSE;
        }

        /* Query information */
        Status = NtQuerySystemInformation(SystemModuleInformation, Information, Size, &RequiredSize);
        if (!NT_SUCCESS(Status))
        {
            /* Free the current buffer */
            LocalFree(Information);

            /* If it was not a length mismatch (ie, buffer too small), just leave */
            if (Status != STATUS_INFO_LENGTH_MISMATCH)
            {
                SetLastError(RtlNtStatusToDosError(Status));
                return FALSE;
            }

            /* Try again with the required size */
            Size = RequiredSize;
            continue;
        }

        /* No modules returned? Leave */
        if (Information->NumberOfModules == 0)
        {
            break;
        }

        /* Try to find which module matches the base address given */
        for (i = 0; i < Information->NumberOfModules; ++i)
        {
            Module = Information->Modules[i];
            if (Module.ImageBase == ImageBase)
            {
                /* Copy the matching module and leave */
                memcpy(MatchingModule, &Module, sizeof(Module));
                LocalFree(Information);
                return TRUE;
            }
        }

        /* If we arrive here, it means we were not able to find matching base address */
        break;
    }

    /* Release and leave */
    LocalFree(Information);
    SetLastError(ERROR_INVALID_HANDLE);

    return FALSE;
}

/*
 * @implemented
 */
static BOOL NTAPI
FindModule(IN HANDLE hProcess,
           IN HMODULE hModule OPTIONAL,
           OUT PLDR_DATA_TABLE_ENTRY Module)
{
    DWORD Count;
    NTSTATUS Status;
    PPEB_LDR_DATA LoaderData;
    PLIST_ENTRY ListHead, ListEntry;
    PROCESS_BASIC_INFORMATION ProcInfo;

    /* Query the process information to get its PEB address */
    Status = NtQueryInformationProcess(hProcess, ProcessBasicInformation, &ProcInfo, sizeof(ProcInfo), NULL);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    /* If no module was provided, get base as module */
    if (hModule == NULL)
    {
        if (!ReadProcessMemory(hProcess, &ProcInfo.PebBaseAddress->ImageBaseAddress, &hModule, sizeof(hModule), NULL))
        {
            return FALSE;
        }
    }

    /* Read loader data address from PEB */
    if (!ReadProcessMemory(hProcess, &ProcInfo.PebBaseAddress->Ldr, &LoaderData, sizeof(LoaderData), NULL))
    {
        return FALSE;
    }

    if (LoaderData == NULL)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    /* Store list head address */
    ListHead = &(LoaderData->InMemoryOrderModuleList);

    /* Read first element in the modules list */
    if (!ReadProcessMemory(hProcess,
                           &(LoaderData->InMemoryOrderModuleList.Flink),
                           &ListEntry,
                           sizeof(ListEntry),
                           NULL))
    {
        return FALSE;
    }

    Count = 0;

    /* Loop on the modules */
    while (ListEntry != ListHead)
    {
        /* Load module data */
        if (!ReadProcessMemory(hProcess,
                               CONTAINING_RECORD(ListEntry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks),
                               Module,
                               sizeof(*Module),
                               NULL))
        {
            return FALSE;
        }

        /* Does that match the module we're looking for? */
        if (Module->DllBase == hModule)
        {
            return TRUE;
        }

        ++Count;
        if (Count > MAX_MODULES)
        {
            break;
        }

        /* Get to next listed module */
        ListEntry = Module->InMemoryOrderLinks.Flink;
    }

    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
}

typedef struct _INTERNAL_ENUM_PAGE_FILES_CONTEXT
{
    LPVOID lpContext;
    PENUM_PAGE_FILE_CALLBACKA pCallbackRoutine;
    DWORD dwErrCode;
} INTERNAL_ENUM_PAGE_FILES_CONTEXT, *PINTERNAL_ENUM_PAGE_FILES_CONTEXT;

/*
 * @implemented
 */
static BOOL CALLBACK
CallBackConvertToAscii(LPVOID pContext,
                       PENUM_PAGE_FILE_INFORMATION pPageFileInfo,
                       LPCWSTR lpFilename)
{
    BOOL Ret;
    SIZE_T Len;
    LPSTR AnsiFileName;
    PINTERNAL_ENUM_PAGE_FILES_CONTEXT Context = (PINTERNAL_ENUM_PAGE_FILES_CONTEXT)pContext;

    Len = wcslen(lpFilename);

    /* Alloc space for the ANSI string */
    AnsiFileName = LocalAlloc(LMEM_FIXED, (Len * sizeof(CHAR)) + sizeof(ANSI_NULL));
    if (AnsiFileName == NULL)
    {
        Context->dwErrCode = RtlNtStatusToDosError(STATUS_INSUFFICIENT_RESOURCES);
        return FALSE;
    }

    /* Convert string to ANSI */
    if (WideCharToMultiByte(CP_ACP, 0, lpFilename, -1, AnsiFileName, (Len * sizeof(CHAR)) + sizeof(ANSI_NULL), NULL, NULL) == 0)
    {
        Context->dwErrCode = GetLastError();
        LocalFree(AnsiFileName);
        return FALSE;
    }

    /* And finally call "real" callback */
    Ret = Context->pCallbackRoutine(Context->lpContext, pPageFileInfo, AnsiFileName);
    LocalFree(AnsiFileName);

    return Ret;
}

/*
 * @unimplemented
 */
static VOID NTAPI
PsParseCommandLine(VOID)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
static VOID NTAPI
PsInitializeAndStartProfile(VOID)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
static VOID NTAPI
PsStopAndAnalyzeProfile(VOID)
{
    UNIMPLEMENTED;
}

/* PUBLIC *********************************************************************/

/*
 * @implemented
 */
BOOLEAN
WINAPI
DllMain(HINSTANCE hDllHandle,
        DWORD nReason,
        LPVOID Reserved)
{
    switch(nReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hDllHandle);
            if (NtCurrentPeb()->ProcessParameters->Flags & RTL_USER_PROCESS_PARAMETERS_PROFILE_USER)
            {
                PsParseCommandLine();
                PsInitializeAndStartProfile();
            }
            break;

        case DLL_PROCESS_DETACH:
            if (NtCurrentPeb()->ProcessParameters->Flags & RTL_USER_PROCESS_PARAMETERS_PROFILE_USER)
            {
                PsStopAndAnalyzeProfile();
            }
            break;
  }

  return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
EmptyWorkingSet(HANDLE hProcess)
{
    SYSTEM_INFO SystemInfo;
    QUOTA_LIMITS QuotaLimits;
    NTSTATUS Status;

    GetSystemInfo(&SystemInfo);

    /* Query the working set */
    Status = NtQueryInformationProcess(hProcess,
                                       ProcessQuotaLimits,
                                       &QuotaLimits,
                                       sizeof(QuotaLimits),
                                       NULL);

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    /* Empty the working set */
    QuotaLimits.MinimumWorkingSetSize = -1;
    QuotaLimits.MaximumWorkingSetSize = -1;

    /* Set the working set */
    Status = NtSetInformationProcess(hProcess,
                                     ProcessQuotaLimits,
                                     &QuotaLimits,
                                     sizeof(QuotaLimits));
    if (!NT_SUCCESS(Status) && Status != STATUS_PRIVILEGE_NOT_HELD)
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
EnumDeviceDrivers(LPVOID *lpImageBase,
                  DWORD cb,
                  LPDWORD lpcbNeeded)
{
    NTSTATUS Status;
    DWORD NewSize, Count;
    PRTL_PROCESS_MODULES Information;
    /* By default, to prevent too many reallocations, we already make room for 4 modules */
    DWORD Size = sizeof(RTL_PROCESS_MODULES) + 3 * sizeof(RTL_PROCESS_MODULE_INFORMATION);

    do
    {
        /* Allocate a buffer to hold modules information */
        Information = LocalAlloc(LMEM_FIXED, Size);
        if (!Information)
        {
            SetLastError(ERROR_NO_SYSTEM_RESOURCES);
            return FALSE;
        }

        /* Query information */
        Status = NtQuerySystemInformation(SystemModuleInformation, Information, Size, &Count);
        /* In case of an error */
        if (!NT_SUCCESS(Status))
        {
            /* Save the amount of output modules */
            NewSize = Information->NumberOfModules;
            /* And free buffer */
            LocalFree(Information);

            /* If it was not a length mismatch (ie, buffer too small), just leave */
            if (Status != STATUS_INFO_LENGTH_MISMATCH)
            {
                SetLastError(RtlNtStatusToDosError(Status));
                return FALSE;
            }

            /* Compute new size length */
            ASSERT(Size >= sizeof(RTL_PROCESS_MODULES));
            NewSize *= sizeof(RTL_PROCESS_MODULE_INFORMATION);
            NewSize += sizeof(ULONG);
            ASSERT(NewSize >= sizeof(RTL_PROCESS_MODULES));
            /* Check whether it is really bigger - otherwise, leave */
            if (NewSize < Size)
            {
                ASSERT(NewSize > Size);
                SetLastError(RtlNtStatusToDosError(STATUS_INFO_LENGTH_MISMATCH));
                return FALSE;
            }

            /* Loop again with that new buffer */
            Size = NewSize;
            continue;
        }

        /* End of allocation loop */
        break;
    } while (TRUE);

    _SEH2_TRY
    {
        for (Count = 0; Count < Information->NumberOfModules && Count < cb / sizeof(LPVOID); ++Count)
        {
            lpImageBase[Count] = Information->Modules[Count].ImageBase;
        }

        *lpcbNeeded = Information->NumberOfModules * sizeof(LPVOID);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(RtlNtStatusToDosError(_SEH2_GetExceptionCode()));
        _SEH2_YIELD(return FALSE);
    }
    _SEH2_END;

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
EnumProcesses(DWORD *lpidProcess,
              DWORD cb,
              LPDWORD lpcbNeeded)
{
    NTSTATUS Status;
    DWORD Size = MAXSHORT, Count;
    PSYSTEM_PROCESS_INFORMATION ProcInfo;
    PSYSTEM_PROCESS_INFORMATION ProcInfoArray;

    /* First of all, query all the processes */
    do
    {
        ProcInfoArray = LocalAlloc(LMEM_FIXED, Size);
        if (ProcInfoArray == NULL)
        {
            return FALSE;
        }

        Status = NtQuerySystemInformation(SystemProcessInformation, ProcInfoArray, Size, NULL);
        if (Status == STATUS_INFO_LENGTH_MISMATCH)
        {
            LocalFree(ProcInfoArray);
            Size += MAXSHORT;
            continue;
        }

        break;
    }
    while (TRUE);

    if (!NT_SUCCESS(Status))
    {
        LocalFree(ProcInfoArray);
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    /* Then, loop to output data */
    Count = 0;
    ProcInfo = ProcInfoArray;

    _SEH2_TRY
    {
        do
        {
            /* It may sound weird, but actually MS only updated Count on
             * successful write. So, it cannot measure the amount of space needed!
             * This is really tricky.
             */
            if (Count < cb / sizeof(DWORD))
            {
                lpidProcess[Count] = HandleToUlong(ProcInfo->UniqueProcessId);
                Count++;
            }

            if (ProcInfo->NextEntryOffset == 0)
            {
                break;
            }

            ProcInfo = (PSYSTEM_PROCESS_INFORMATION)((ULONG_PTR)ProcInfo + ProcInfo->NextEntryOffset);
        }
        while (TRUE);

        *lpcbNeeded = Count * sizeof(DWORD);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(RtlNtStatusToDosError(_SEH2_GetExceptionCode()));
        LocalFree(ProcInfoArray);
        _SEH2_YIELD(return FALSE);
    }
    _SEH2_END;

    LocalFree(ProcInfoArray);
    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
EnumProcessModules(HANDLE hProcess,
                   HMODULE *lphModule,
                   DWORD cb,
                   LPDWORD lpcbNeeded)
{
    NTSTATUS Status;
    DWORD NbOfModules, Count;
    PPEB_LDR_DATA LoaderData;
    PLIST_ENTRY ListHead, ListEntry;
    PROCESS_BASIC_INFORMATION ProcInfo;
    LDR_DATA_TABLE_ENTRY CurrentModule;

    /* Query the process information to get its PEB address */
    Status = NtQueryInformationProcess(hProcess, ProcessBasicInformation, &ProcInfo, sizeof(ProcInfo), NULL);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    if (ProcInfo.PebBaseAddress == NULL)
    {
        SetLastError(RtlNtStatusToDosError(STATUS_PARTIAL_COPY));
        return FALSE;
    }

    /* Read loader data address from PEB */
    if (!ReadProcessMemory(hProcess, &ProcInfo.PebBaseAddress->Ldr, &LoaderData, sizeof(LoaderData), NULL))
    {
        return FALSE;
    }

    /* Store list head address */
    ListHead = &LoaderData->InLoadOrderModuleList;

    /* Read first element in the modules list */
    if (!ReadProcessMemory(hProcess, &LoaderData->InLoadOrderModuleList.Flink, &ListEntry, sizeof(ListEntry), NULL))
    {
        return FALSE;
    }

    NbOfModules = cb / sizeof(HMODULE);
    Count = 0;

    /* Loop on the modules */
    while (ListEntry != ListHead)
    {
        /* Load module data */
        if (!ReadProcessMemory(hProcess,
                               CONTAINING_RECORD(ListEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks),
                               &CurrentModule,
                               sizeof(CurrentModule),
                               NULL))
        {
            return FALSE;
        }

        /* Check if we can output module, do it if so */
        if (Count < NbOfModules)
        {
            _SEH2_TRY
            {
                lphModule[Count] = CurrentModule.DllBase;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                SetLastError(RtlNtStatusToDosError(_SEH2_GetExceptionCode()));
                _SEH2_YIELD(return FALSE);
            }
            _SEH2_END;
        }

        ++Count;
        if (Count > MAX_MODULES)
        {
            SetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }

        /* Get to next listed module */
        ListEntry = CurrentModule.InLoadOrderLinks.Flink;
    }

    _SEH2_TRY
    {
        *lpcbNeeded = Count * sizeof(HMODULE);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(RtlNtStatusToDosError(_SEH2_GetExceptionCode()));
        _SEH2_YIELD(return FALSE);
    }
    _SEH2_END;

    return TRUE;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetDeviceDriverBaseNameA(LPVOID ImageBase,
                         LPSTR lpBaseName,
                         DWORD nSize)
{
    SIZE_T Len, LenWithNull;
    RTL_PROCESS_MODULE_INFORMATION Module;

    /* Get the associated device driver to the base address */
    if (!FindDeviceDriver(ImageBase, &Module))
    {
        return 0;
    }

    /* And copy as much as possible to output buffer.
     * Try to add 1 to the len, to copy the null char as well.
     */
    Len =
    LenWithNull = strlen(&Module.FullPathName[Module.OffsetToFileName]) + 1;
    if (Len > nSize)
    {
        Len = nSize;
    }

    memcpy(lpBaseName, &Module.FullPathName[Module.OffsetToFileName], Len);
    /* In case we copied null char, remove it from final len */
    if (Len == LenWithNull)
    {
        --Len;
    }

    return Len;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetDeviceDriverFileNameA(LPVOID ImageBase,
                         LPSTR lpFilename,
                         DWORD nSize)
{
    SIZE_T Len, LenWithNull;
    RTL_PROCESS_MODULE_INFORMATION Module;

    /* Get the associated device driver to the base address */
    if (!FindDeviceDriver(ImageBase, &Module))
    {
        return 0;
    }

    /* And copy as much as possible to output buffer.
     * Try to add 1 to the len, to copy the null char as well.
     */
    Len =
    LenWithNull = strlen(Module.FullPathName) + 1;
    if (Len > nSize)
    {
        Len = nSize;
    }

    memcpy(lpFilename, Module.FullPathName, Len);
    /* In case we copied null char, remove it from final len */
    if (Len == LenWithNull)
    {
        --Len;
    }

    return Len;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetDeviceDriverBaseNameW(LPVOID ImageBase,
                         LPWSTR lpBaseName,
                         DWORD nSize)
{
    DWORD Len;
    LPSTR BaseName;

    /* Allocate internal buffer for conversion */
    BaseName = LocalAlloc(LMEM_FIXED, nSize);
    if (BaseName == 0)
    {
        return 0;
    }

    /* Call A API */
    Len = GetDeviceDriverBaseNameA(ImageBase, BaseName, nSize);
    if (Len == 0)
    {
        LocalFree(BaseName);
        return 0;
    }

    /* And convert output */
    if (MultiByteToWideChar(CP_ACP, 0, BaseName, (Len < nSize) ? Len + 1 : Len, lpBaseName, nSize) == 0)
    {
        LocalFree(BaseName);
        return 0;
    }

    LocalFree(BaseName);
    return Len;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetDeviceDriverFileNameW(LPVOID ImageBase,
                         LPWSTR lpFilename,
                         DWORD nSize)
{
    DWORD Len;
    LPSTR FileName;

    /* Allocate internal buffer for conversion */
    FileName = LocalAlloc(LMEM_FIXED, nSize);
    if (FileName == 0)
    {
        return 0;
    }

    /* Call A API */
    Len = GetDeviceDriverFileNameA(ImageBase, FileName, nSize);
    if (Len == 0)
    {
        LocalFree(FileName);
        return 0;
    }

    /* And convert output */
    if (MultiByteToWideChar(CP_ACP, 0, FileName, (Len < nSize) ? Len + 1 : Len, lpFilename, nSize) == 0)
    {
        LocalFree(FileName);
        return 0;
    }

    LocalFree(FileName);
    return Len;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetMappedFileNameA(HANDLE hProcess,
                   LPVOID lpv,
                   LPSTR lpFilename,
                   DWORD nSize)
{
    DWORD Len;
    LPWSTR FileName;

    DPRINT("GetMappedFileNameA(%p, %p, %p, %lu)\n", hProcess, lpv, lpFilename, nSize);

    /* Allocate internal buffer for conversion */
    FileName = LocalAlloc(LMEM_FIXED, nSize * sizeof(WCHAR));
    if (FileName == NULL)
    {
        return 0;
    }

    /* Call W API */
    Len = GetMappedFileNameW(hProcess, lpv, FileName, nSize);

    /* And convert output */
    if (WideCharToMultiByte(CP_ACP, 0, FileName, (Len < nSize) ? Len + 1 : Len, lpFilename, nSize, NULL, NULL) == 0)
    {
        Len = 0;
    }

    LocalFree(FileName);
    return Len;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetMappedFileNameW(HANDLE hProcess,
                   LPVOID lpv,
                   LPWSTR lpFilename,
                   DWORD nSize)
{
    DWORD Len;
    SIZE_T OutSize;
    NTSTATUS Status;
    struct
    {
        MEMORY_SECTION_NAME;
        WCHAR CharBuffer[MAX_PATH];
    } SectionName;

    DPRINT("GetMappedFileNameW(%p, %p, %p, %lu)\n", hProcess, lpv, lpFilename, nSize);

    /* If no buffer, no need to keep going on */
    if (nSize == 0)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
    }

    /* Query section name */
    Status = NtQueryVirtualMemory(hProcess, lpv, MemorySectionName,
                                  &SectionName, sizeof(SectionName), &OutSize);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return 0;
    }

    /* Prepare to copy file name */
    Len =
    OutSize = SectionName.SectionFileName.Length / sizeof(WCHAR);
    if (OutSize + 1 > nSize)
    {
        Len = nSize - 1;
        OutSize = nSize;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }
    else
    {
        SetLastError(ERROR_SUCCESS);
    }

    /* Copy, zero and return */
    memcpy(lpFilename, SectionName.SectionFileName.Buffer, Len * sizeof(WCHAR));
    lpFilename[Len] = 0;

    return OutSize;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetModuleBaseNameA(HANDLE hProcess,
                   HMODULE hModule,
                   LPSTR lpBaseName,
                   DWORD nSize)
{
    DWORD Len;
    PWSTR BaseName;

    /* Allocate internal buffer for conversion */
    BaseName = LocalAlloc(LMEM_FIXED, nSize * sizeof(WCHAR));
    if (BaseName == NULL)
    {
        return 0;
    }

    /* Call W API */
    Len = GetModuleBaseNameW(hProcess, hModule, BaseName, nSize);
    /* And convert output */
    if (WideCharToMultiByte(CP_ACP, 0, BaseName, (Len < nSize) ? Len + 1 : Len, lpBaseName, nSize, NULL, NULL) == 0)
    {
        Len = 0;
    }

    LocalFree(BaseName);

    return Len;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetModuleBaseNameW(HANDLE hProcess,
                   HMODULE hModule,
                   LPWSTR lpBaseName,
                   DWORD nSize)
{
    DWORD Len;
    LDR_DATA_TABLE_ENTRY Module;

    /* Get the matching module */
    if (!FindModule(hProcess, hModule, &Module))
    {
        return 0;
    }

    /* Get the maximum len we have/can write in given size */
    Len = Module.BaseDllName.Length + sizeof(UNICODE_NULL);
    if (nSize * sizeof(WCHAR) < Len)
    {
        Len = nSize * sizeof(WCHAR);
    }

    /* Read string */
    if (!ReadProcessMemory(hProcess, (&Module.BaseDllName)->Buffer, lpBaseName, Len, NULL))
    {
        return 0;
    }

    /* If we are at the end of the string, prepare to override to nullify string */
    if (Len == Module.BaseDllName.Length + sizeof(UNICODE_NULL))
    {
        Len -= sizeof(UNICODE_NULL);
    }

    /* Nullify at the end if needed */
    if (Len >= nSize * sizeof(WCHAR))
    {
        if (nSize)
        {
            ASSERT(nSize >= sizeof(UNICODE_NULL));
            lpBaseName[nSize - 1] = UNICODE_NULL;
        }
    }
    /* Otherwise, nullify at last written char */
    else
    {
        ASSERT(Len + sizeof(UNICODE_NULL) <= nSize * sizeof(WCHAR));
        lpBaseName[Len / sizeof(WCHAR)] = UNICODE_NULL;
    }

    return Len / sizeof(WCHAR);
}


/*
 * @implemented
 */
DWORD
WINAPI
GetModuleFileNameExA(HANDLE hProcess,
                     HMODULE hModule,
                     LPSTR lpFilename,
                     DWORD nSize)
{
    DWORD Len;
    PWSTR Filename;

    /* Allocate internal buffer for conversion */
    Filename = LocalAlloc(LMEM_FIXED, nSize * sizeof(WCHAR));
    if (Filename == NULL)
    {
        return 0;
    }

    /* Call W API */
    Len = GetModuleFileNameExW(hProcess, hModule, Filename, nSize);
    /* And convert output */
    if (WideCharToMultiByte(CP_ACP, 0, Filename, (Len < nSize) ? Len + 1 : Len, lpFilename, nSize, NULL, NULL) == 0)
    {
        Len = 0;
    }

    LocalFree(Filename);

    return Len;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetModuleFileNameExW(HANDLE hProcess,
                     HMODULE hModule,
                     LPWSTR lpFilename,
                     DWORD nSize)
{
    DWORD Len;
    LDR_DATA_TABLE_ENTRY Module;

    /* Get the matching module */
    if (!FindModule(hProcess, hModule, &Module))
    {
        return 0;
    }

    /* Get the maximum len we have/can write in given size */
    Len = Module.FullDllName.Length + sizeof(UNICODE_NULL);
    if (nSize * sizeof(WCHAR) < Len)
    {
        Len = nSize * sizeof(WCHAR);
    }

    /* Read string */
    if (!ReadProcessMemory(hProcess, (&Module.FullDllName)->Buffer, lpFilename, Len, NULL))
    {
        return 0;
    }

    /* If we are at the end of the string, prepare to override to nullify string */
    if (Len == Module.FullDllName.Length + sizeof(UNICODE_NULL))
    {
        Len -= sizeof(UNICODE_NULL);
    }

    /* Nullify at the end if needed */
    if (Len >= nSize * sizeof(WCHAR))
    {
        if (nSize)
        {
            ASSERT(nSize >= sizeof(UNICODE_NULL));
            lpFilename[nSize - 1] = UNICODE_NULL;
        }
    }
    /* Otherwise, nullify at last written char */
    else
    {
        ASSERT(Len + sizeof(UNICODE_NULL) <= nSize * sizeof(WCHAR));
        lpFilename[Len / sizeof(WCHAR)] = UNICODE_NULL;
    }

    return Len / sizeof(WCHAR);
}


/*
 * @implemented
 */
BOOL
WINAPI
GetModuleInformation(HANDLE hProcess,
                     HMODULE hModule,
                     LPMODULEINFO lpmodinfo,
                     DWORD cb)
{
    MODULEINFO LocalInfo;
    LDR_DATA_TABLE_ENTRY Module;

    /* Check output size */
    if (cb < sizeof(MODULEINFO))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    /* Get the matching module */
    if (!FindModule(hProcess, hModule, &Module))
    {
        return FALSE;
    }

    /* Get a local copy first, to check for valid pointer once */
    LocalInfo.lpBaseOfDll = hModule;
    LocalInfo.SizeOfImage = Module.SizeOfImage;
    LocalInfo.EntryPoint = Module.EntryPoint;

    /* Attempt to copy to output */
    _SEH2_TRY
    {
        memcpy(lpmodinfo, &LocalInfo, sizeof(LocalInfo));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(RtlNtStatusToDosError(_SEH2_GetExceptionCode()));
        _SEH2_YIELD(return FALSE);
    }
    _SEH2_END;

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
InitializeProcessForWsWatch(HANDLE hProcess)
{
    NTSTATUS Status;

    /* Simply forward the call */
    Status = NtSetInformationProcess(hProcess,
                                     ProcessWorkingSetWatch,
                                     NULL,
                                     0);
    /* In case the function returns this, MS considers the call as a success */
    if (NT_SUCCESS(Status) || Status == STATUS_PORT_ALREADY_SET || Status == STATUS_ACCESS_DENIED)
    {
        return TRUE;
    }

    SetLastError(RtlNtStatusToDosError(Status));
    return FALSE;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetWsChanges(HANDLE hProcess,
             PPSAPI_WS_WATCH_INFORMATION lpWatchInfo,
             DWORD cb)
{
    NTSTATUS Status;

    /* Simply forward the call */
    Status = NtQueryInformationProcess(hProcess,
                                       ProcessWorkingSetWatch,
                                       lpWatchInfo,
                                       cb,
                                       NULL);
    if(!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetProcessImageFileNameW(HANDLE hProcess,
                         LPWSTR lpImageFileName,
                         DWORD nSize)
{
    PUNICODE_STRING ImageFileName;
    SIZE_T BufferSize;
    NTSTATUS Status;
    DWORD Len;

    /* Allocate string big enough to hold name */
    BufferSize = sizeof(UNICODE_STRING) + (nSize * sizeof(WCHAR));
    ImageFileName = LocalAlloc(LMEM_FIXED, BufferSize);
    if (ImageFileName == NULL)
    {
        return 0;
    }

    /* Query name */
    Status = NtQueryInformationProcess(hProcess,
                                       ProcessImageFileName,
                                       ImageFileName,
                                       BufferSize,
                                       NULL);
    /* Len mismatch => buffer too small */
    if (Status == STATUS_INFO_LENGTH_MISMATCH)
    {
        Status = STATUS_BUFFER_TOO_SMALL;
    }
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        LocalFree(ImageFileName);
        return 0;
    }

    /* Copy name and null-terminate if possible */
    memcpy(lpImageFileName, ImageFileName->Buffer, ImageFileName->Length);
    Len = ImageFileName->Length / sizeof(WCHAR);
    if (Len < nSize)
    {
        lpImageFileName[Len] = UNICODE_NULL;
    }

    LocalFree(ImageFileName);
    return Len;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetProcessImageFileNameA(HANDLE hProcess,
                         LPSTR lpImageFileName,
                         DWORD nSize)
{
    PUNICODE_STRING ImageFileName;
    SIZE_T BufferSize;
    NTSTATUS Status;
    DWORD Len;

    /* Allocate string big enough to hold name */
    BufferSize = sizeof(UNICODE_STRING) + (nSize * sizeof(WCHAR));
    ImageFileName = LocalAlloc(LMEM_FIXED, BufferSize);
    if (ImageFileName == NULL)
    {
        return 0;
    }

    /* Query name */
    Status = NtQueryInformationProcess(hProcess,
                                       ProcessImageFileName,
                                       ImageFileName,
                                       BufferSize,
                                       NULL);
    /* Len mismatch => buffer too small */
    if (Status == STATUS_INFO_LENGTH_MISMATCH)
    {
        Status = STATUS_BUFFER_TOO_SMALL;
    }
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        LocalFree(ImageFileName);
        return 0;
    }

    /* Copy name */
    Len = WideCharToMultiByte(CP_ACP, 0, ImageFileName->Buffer,
                              ImageFileName->Length, lpImageFileName, nSize, NULL, NULL);
    /* If conversion was successful, don't return len with added \0 */
    if (Len != 0)
    {
        Len -= sizeof(ANSI_NULL);
    }

    LocalFree(ImageFileName);
    return Len;
}


/*
 * @implemented
 */
BOOL
WINAPI
EnumPageFilesA(PENUM_PAGE_FILE_CALLBACKA pCallbackRoutine,
               LPVOID lpContext)
{
    BOOL Ret;
    INTERNAL_ENUM_PAGE_FILES_CONTEXT Context;

    Context.dwErrCode = ERROR_SUCCESS;
    Context.lpContext = lpContext;
    Context.pCallbackRoutine = pCallbackRoutine;

    /* Call W with our own callback for W -> A conversions */
    Ret = EnumPageFilesW(CallBackConvertToAscii, &Context);
    /* If we succeed but we have error code, fail and set error */
    if (Ret && Context.dwErrCode != ERROR_SUCCESS)
    {
        Ret = FALSE;
        SetLastError(Context.dwErrCode);
    }

    return Ret;
}


/*
 * @implemented
 */
BOOL
WINAPI
EnumPageFilesW(PENUM_PAGE_FILE_CALLBACKW pCallbackRoutine,
               LPVOID lpContext)
{
    PWSTR Colon;
    NTSTATUS Status;
    DWORD Size = INIT_MEMORY_SIZE, Needed;
    ENUM_PAGE_FILE_INFORMATION Information;
    PSYSTEM_PAGEFILE_INFORMATION PageFileInfoArray, PageFileInfo;

    /* First loop till we have all the information about page files */
    do
    {
        PageFileInfoArray = LocalAlloc(LMEM_FIXED, Size);
        if (PageFileInfoArray == NULL)
        {
            SetLastError(RtlNtStatusToDosError(STATUS_INSUFFICIENT_RESOURCES));
            return FALSE;
        }

        Status = NtQuerySystemInformation(SystemPageFileInformation, PageFileInfoArray, Size, &Needed);
        if (NT_SUCCESS(Status))
        {
            break;
        }

        LocalFree(PageFileInfoArray);

        /* In case we have unexpected status, quit */
        if (Status != STATUS_INFO_LENGTH_MISMATCH)
        {
            SetLastError(RtlNtStatusToDosError(Status));
            return FALSE;
        }

        /* If needed size is smaller than actual size, guess it's something to add to our current size */
        if (Needed <= Size)
        {
            Size += Needed;
        }
        /* Otherwise, take it as size to allocate */
        else
        {
            Size = Needed;
        }
    }
    while (TRUE);

    /* Start browsing all our entries */
    PageFileInfo = PageFileInfoArray;
    do
    {
        /* Ensure we really have an entry */
        if (Needed < sizeof(SYSTEM_PAGEFILE_INFORMATION))
        {
            break;
        }

        /* Prepare structure to hand to the user */
        Information.Reserved = 0;
        Information.cb = sizeof(Information);
        Information.TotalSize = PageFileInfo->TotalSize;
        Information.TotalInUse = PageFileInfo->TotalInUse;
        Information.PeakUsage = PageFileInfo->PeakUsage;

        /* Search for colon */
        Colon = wcschr(PageFileInfo->PageFileName.Buffer, L':');
        /* If it's found and not at the begin of the string */
        if (Colon != 0 && Colon != PageFileInfo->PageFileName.Buffer)
        {
            /* We can call the user callback routine with the colon */
            --Colon;
            pCallbackRoutine(lpContext, &Information, Colon);
        }

        /* If no next entry, then, it's over */
        if (PageFileInfo->NextEntryOffset == 0 || PageFileInfo->NextEntryOffset > Needed)
        {
            break;
        }

        /* Jump to next entry while keeping accurate bytes left count */
        Needed -= PageFileInfo->NextEntryOffset;
        PageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION)((ULONG_PTR)PageFileInfo + PageFileInfo->NextEntryOffset);
    }
    while (TRUE);

    LocalFree(PageFileInfoArray);
    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetPerformanceInfo(PPERFORMANCE_INFORMATION pPerformanceInformation,
                   DWORD cb)
{
    NTSTATUS Status;
    SYSTEM_BASIC_INFORMATION SystemBasicInfo;
    SYSTEM_PERFORMANCE_INFORMATION SystemPerfInfo;
    SYSTEM_FILECACHE_INFORMATION SystemFileCacheInfo;
    PSYSTEM_PROCESS_INFORMATION ProcInfoArray, SystemProcInfo;
    DWORD Size = INIT_MEMORY_SIZE, Needed, ProcCount, ThreadsCount, HandleCount;

    /* Validate output buffer */
    if (cb < sizeof(PERFORMANCE_INFORMATION))
    {
        SetLastError(RtlNtStatusToDosError(STATUS_INFO_LENGTH_MISMATCH));
        return FALSE;
    }

    /* First, gather as many information about the system as possible */
    Status = NtQuerySystemInformation(SystemBasicInformation,
                                      &SystemBasicInfo,
                                      sizeof(SystemBasicInfo),
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    Status = NtQuerySystemInformation(SystemPerformanceInformation,
                                      &SystemPerfInfo,
                                      sizeof(SystemPerfInfo),
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    Status = NtQuerySystemInformation(SystemFileCacheInformation,
                                      &SystemFileCacheInfo,
                                      sizeof(SystemFileCacheInfo),
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    /* Then loop till we have all the information about processes */
    do
    {
        ProcInfoArray = LocalAlloc(LMEM_FIXED, Size);
        if (ProcInfoArray == NULL)
        {
            SetLastError(RtlNtStatusToDosError(STATUS_INSUFFICIENT_RESOURCES));
            return FALSE;
        }

        Status = NtQuerySystemInformation(SystemProcessInformation,
                                          ProcInfoArray,
                                          Size,
                                          &Needed);
        if (NT_SUCCESS(Status))
        {
            break;
        }

        LocalFree(ProcInfoArray);

        /* In case we have unexpected status, quit */
        if (Status != STATUS_INFO_LENGTH_MISMATCH)
        {
            SetLastError(RtlNtStatusToDosError(Status));
            return FALSE;
        }

        /* If needed size is smaller than actual size, guess it's something to add to our current size */
        if (Needed <= Size)
        {
            Size += Needed;
        }
        /* Otherwise, take it as size to allocate */
        else
        {
            Size = Needed;
        }
    } while (TRUE);

    /* Start browsing all our entries */
    ProcCount = 0;
    HandleCount = 0;
    ThreadsCount = 0;
    SystemProcInfo = ProcInfoArray;
    do
    {
        /* Ensure we really have an entry */
        if (Needed < sizeof(SYSTEM_PROCESS_INFORMATION))
        {
            break;
        }

        /* Sum procs, threads and handles */
        ++ProcCount;
        ThreadsCount += SystemProcInfo->NumberOfThreads;
        HandleCount += SystemProcInfo->HandleCount;

        /* If no next entry, then, it's over */
        if (SystemProcInfo->NextEntryOffset == 0 || SystemProcInfo->NextEntryOffset > Needed)
        {
            break;
        }

        /* Jump to next entry while keeping accurate bytes left count */
        Needed -= SystemProcInfo->NextEntryOffset;
        SystemProcInfo = (PSYSTEM_PROCESS_INFORMATION)((ULONG_PTR)SystemProcInfo + SystemProcInfo->NextEntryOffset);
    }
    while (TRUE);

    LocalFree(ProcInfoArray);

    /* Output data */
    pPerformanceInformation->CommitTotal = SystemPerfInfo.CommittedPages;
    pPerformanceInformation->CommitLimit = SystemPerfInfo.CommitLimit;
    pPerformanceInformation->CommitPeak = SystemPerfInfo.PeakCommitment;
    pPerformanceInformation->PhysicalTotal = SystemBasicInfo.NumberOfPhysicalPages;
    pPerformanceInformation->PhysicalAvailable = SystemPerfInfo.AvailablePages;
    pPerformanceInformation->SystemCache = SystemFileCacheInfo.CurrentSizeIncludingTransitionInPages;
    pPerformanceInformation->KernelNonpaged = SystemPerfInfo.NonPagedPoolPages;
    pPerformanceInformation->PageSize = SystemBasicInfo.PageSize;
    pPerformanceInformation->cb = sizeof(PERFORMANCE_INFORMATION);
    pPerformanceInformation->KernelTotal = SystemPerfInfo.PagedPoolPages + SystemPerfInfo.NonPagedPoolPages;
    pPerformanceInformation->KernelPaged = SystemPerfInfo.PagedPoolPages;
    pPerformanceInformation->HandleCount = HandleCount;
    pPerformanceInformation->ProcessCount = ProcCount;
    pPerformanceInformation->ThreadCount = ThreadsCount;

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetProcessMemoryInfo(HANDLE Process,
                     PPROCESS_MEMORY_COUNTERS ppsmemCounters,
                     DWORD cb)
{
    NTSTATUS Status;
    VM_COUNTERS_EX Counters;

    /* Validate output size
     * It can be either PROCESS_MEMORY_COUNTERS or PROCESS_MEMORY_COUNTERS_EX
     */
    if (cb < sizeof(PROCESS_MEMORY_COUNTERS))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    _SEH2_TRY
    {
        ppsmemCounters->PeakPagefileUsage = 0;

        /* Query counters */
        Status = NtQueryInformationProcess(Process,
                                           ProcessVmCounters,
                                           &Counters,
                                           sizeof(Counters),
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            SetLastError(RtlNtStatusToDosError(Status));
            _SEH2_YIELD(return FALSE);
        }

        /* Properly set cb, according to what we received */
        if (cb >= sizeof(PROCESS_MEMORY_COUNTERS_EX))
        {
            ppsmemCounters->cb = sizeof(PROCESS_MEMORY_COUNTERS_EX);
        }
        else
        {
            ppsmemCounters->cb = sizeof(PROCESS_MEMORY_COUNTERS);
        }

        /* Output data */
        ppsmemCounters->PageFaultCount = Counters.PageFaultCount;
        ppsmemCounters->PeakWorkingSetSize = Counters.PeakWorkingSetSize;
        ppsmemCounters->WorkingSetSize = Counters.WorkingSetSize;
        ppsmemCounters->QuotaPeakPagedPoolUsage = Counters.QuotaPeakPagedPoolUsage;
        ppsmemCounters->QuotaPagedPoolUsage = Counters.QuotaPagedPoolUsage;
        ppsmemCounters->QuotaPeakNonPagedPoolUsage = Counters.QuotaPeakNonPagedPoolUsage;
        ppsmemCounters->QuotaNonPagedPoolUsage = Counters.QuotaNonPagedPoolUsage;
        ppsmemCounters->PagefileUsage = Counters.PagefileUsage;
        ppsmemCounters->PeakPagefileUsage = Counters.PeakPagefileUsage;
        /* And if needed, additional field for _EX version */
        if (cb >= sizeof(PROCESS_MEMORY_COUNTERS_EX))
        {
            ((PPROCESS_MEMORY_COUNTERS_EX)ppsmemCounters)->PrivateUsage = Counters.PrivateUsage;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(RtlNtStatusToDosError(_SEH2_GetExceptionCode()));
        _SEH2_YIELD(return FALSE);
    }
    _SEH2_END;

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
QueryWorkingSet(HANDLE hProcess,
                PVOID pv,
                DWORD cb)
{
    NTSTATUS Status;

    /* Simply forward the call */
    Status = NtQueryVirtualMemory(hProcess,
                                  NULL,
                                  MemoryWorkingSetList,
                                  pv,
                                  cb,
                                  NULL);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
QueryWorkingSetEx(IN HANDLE hProcess,
                  IN OUT PVOID pv,
                  IN DWORD cb)
{
    NTSTATUS Status;

    /* Simply forward the call */
    Status = NtQueryVirtualMemory(hProcess,
                                  NULL,
                                  MemoryWorkingSetExList,
                                  pv,
                                  cb,
                                  NULL);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/* EOF */
