/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/proc.c
 * PURPOSE:         Process functions
 * PROGRAMMERS:     Ariadne (ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ****************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

WaitForInputIdleType UserWaitForInputIdleRoutine;
UNICODE_STRING BaseUnicodeCommandLine;
ANSI_STRING BaseAnsiCommandLine;
UNICODE_STRING BasePathVariableName = RTL_CONSTANT_STRING(L"PATH");
LPSTARTUPINFOA BaseAnsiStartupInfo = NULL;
PLDR_DATA_TABLE_ENTRY BasepExeLdrEntry;
BOOLEAN g_AppCertInitialized;
BOOLEAN g_HaveAppCerts;
LIST_ENTRY BasepAppCertDllsList;
RTL_CRITICAL_SECTION gcsAppCert;
PBASEP_APPCERT_EMBEDDED_FUNC fEmbeddedCertFunc;
NTSTATUS g_AppCertStatus;
RTL_QUERY_REGISTRY_TABLE BasepAppCertTable[2] =
{
    {
        BasepConfigureAppCertDlls,
        1,
        L"AppCertDlls",
        &BasepAppCertDllsList,
        0,
        NULL,
        0
    }
};

PSAFER_REPLACE_PROCESS_THREAD_TOKENS g_SaferReplaceProcessThreadTokens;
HMODULE gSaferHandle = (HMODULE)-1;

VOID WINAPI
RegisterWaitForInputIdle(WaitForInputIdleType lpfnRegisterWaitForInputIdle);

#define CMD_STRING L"cmd /c "

/* FUNCTIONS ****************************************************************/

VOID
WINAPI
StuffStdHandle(IN HANDLE ProcessHandle,
               IN HANDLE StandardHandle,
               IN PHANDLE Address)
{
    NTSTATUS Status;
    HANDLE DuplicatedHandle;
    SIZE_T NumberOfBytesWritten;

    /* If there is no handle to duplicate, return immediately */
    if (!StandardHandle) return;

    /* Duplicate the handle */
    Status = NtDuplicateObject(NtCurrentProcess(),
                               StandardHandle,
                               ProcessHandle,
                               &DuplicatedHandle,
                               0,
                               0,
                               DUPLICATE_SAME_ACCESS |
                               DUPLICATE_SAME_ATTRIBUTES);
    if (!NT_SUCCESS(Status)) return;

    /* Write it */
    NtWriteVirtualMemory(ProcessHandle,
                         Address,
                         &DuplicatedHandle,
                         sizeof(HANDLE),
                         &NumberOfBytesWritten);
}

BOOLEAN
WINAPI
BuildSubSysCommandLine(IN LPCWSTR SubsystemName,
                       IN LPCWSTR ApplicationName,
                       IN LPCWSTR CommandLine,
                       OUT PUNICODE_STRING SubsysCommandLine)
{
    UNICODE_STRING CommandLineString, ApplicationNameString;
    PWCHAR Buffer;
    ULONG Length;

    /* Convert to unicode strings */
    RtlInitUnicodeString(&CommandLineString, ApplicationName);
    RtlInitUnicodeString(&ApplicationNameString, CommandLine);

    /* Allocate buffer for the output string */
    Length = CommandLineString.MaximumLength + ApplicationNameString.MaximumLength + 32;
    Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length);
    RtlInitEmptyUnicodeString(SubsysCommandLine, Buffer, (USHORT)Length);
    if (!Buffer)
    {
        /* Fail, no memory */
        BaseSetLastNTError(STATUS_NO_MEMORY);
        return FALSE;
    }

    /* Build the final subsystem command line */
    RtlAppendUnicodeToString(SubsysCommandLine, SubsystemName);
    RtlAppendUnicodeStringToString(SubsysCommandLine, &CommandLineString);
    RtlAppendUnicodeToString(SubsysCommandLine, L" /C ");
    RtlAppendUnicodeStringToString(SubsysCommandLine, &ApplicationNameString);
    return TRUE;
}

BOOLEAN
WINAPI
BasepIsImageVersionOk(IN ULONG ImageMajorVersion,
                      IN ULONG ImageMinorVersion)
{
    /* Accept images for NT 3.1 or higher */
    if (ImageMajorVersion > 3 ||
        (ImageMajorVersion == 3 && ImageMinorVersion >= 10))
    {
        /* ReactOS-specific: Accept images even if they are newer than our internal NT version. */
        if (ImageMajorVersion > SharedUserData->NtMajorVersion ||
            (ImageMajorVersion == SharedUserData->NtMajorVersion && ImageMinorVersion > SharedUserData->NtMinorVersion))
        {
            DPRINT1("Accepting image version %lu.%lu, although ReactOS is an NT %hu.%hu OS!\n",
                ImageMajorVersion,
                ImageMinorVersion,
                SharedUserData->NtMajorVersion,
                SharedUserData->NtMinorVersion);
        }

        return TRUE;
    }

    return FALSE;
}

NTSTATUS
WINAPI
BasepCheckWebBladeHashes(IN HANDLE FileHandle)
{
    NTSTATUS Status;
    CHAR Hash[16];

    /* Get all the MD5 hashes */
    Status = RtlComputeImportTableHash(FileHandle, Hash, 1);
    if (!NT_SUCCESS(Status)) return Status;

    /* Depending on which suite this is, run a bsearch and block the appropriate ones */
    if (SharedUserData->SuiteMask & VER_SUITE_COMPUTE_SERVER)
    {
        DPRINT1("Egad! This is a ReactOS Compute Server and we should prevent you from using certain APIs...but we won't.");
    }
    else if (SharedUserData->SuiteMask & VER_SUITE_STORAGE_SERVER)
    {
        DPRINT1("Gasp! This is a ReactOS Storage Server and we should prevent you from using certain APIs...but we won't.");
    }
    else if (SharedUserData->SuiteMask & VER_SUITE_BLADE)
    {
        DPRINT1("Golly! This is a ReactOS Web Blade Server and we should prevent you from using certain APIs...but we won't.");
    }

    /* Actually, fuck it, don't block anything, we're open source */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
BasepSaveAppCertRegistryValue(IN PLIST_ENTRY List,
                              IN PWCHAR ComponentName,
                              IN PWCHAR DllName)
{
    /* Pretty much the only thing this key is used for, is malware */
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
BasepConfigureAppCertDlls(IN PWSTR ValueName,
                          IN ULONG ValueType,
                          IN PVOID ValueData,
                          IN ULONG ValueLength,
                          IN PVOID Context,
                          IN PVOID EntryContext)
{
    /* Add this to the certification list */
    return BasepSaveAppCertRegistryValue(Context, ValueName, ValueData);
}

NTSTATUS
WINAPI
BasepIsProcessAllowed(IN LPWSTR ApplicationName)
{
    NTSTATUS Status, Status1;
    PWCHAR Buffer;
    UINT Length;
    HMODULE TrustLibrary;
    PBASEP_APPCERT_ENTRY Entry;
    ULONG CertFlag;
    PLIST_ENTRY NextEntry;
    HANDLE KeyHandle;
    UNICODE_STRING CertKey = RTL_CONSTANT_STRING(L"\\Registry\\MACHINE\\System\\CurrentControlSet\\Control\\Session Manager\\AppCertDlls");
    OBJECT_ATTRIBUTES KeyAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(&CertKey, OBJ_CASE_INSENSITIVE);

    /* Try to initialize the certification subsystem */
    while (!g_AppCertInitialized)
    {
        /* Defaults */
        Status = STATUS_SUCCESS;
        Buffer = NULL;

        /* Acquire the lock while initializing and see if we lost a race */
        RtlEnterCriticalSection(&gcsAppCert);
        if (g_AppCertInitialized) break;

        /* On embedded, there is a special DLL */
        if (SharedUserData->SuiteMask & VER_SUITE_EMBEDDEDNT)
        {
            /* Allocate a buffer for the name */
            Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                     0,
                                     MAX_PATH * sizeof(WCHAR) +
                                     sizeof(UNICODE_NULL));
            if (!Buffer)
            {
                /* Fail if no memory */
                Status = STATUS_NO_MEMORY;
            }
            else
            {
                /* Now get the system32 directory in our buffer, make sure it fits */
                Length = GetSystemDirectoryW(Buffer, MAX_PATH - sizeof("EmbdTrst.DLL"));
                if ((Length) && (Length <= MAX_PATH - sizeof("EmbdTrst.DLL")))
                {
                    /* Add a slash if needed, and add the embedded cert DLL name */
                    if (Buffer[Length - 1] != '\\') Buffer[Length++] = '\\';
                    RtlCopyMemory(&Buffer[Length],
                                  L"EmbdTrst.DLL",
                                  sizeof(L"EmbdTrst.DLL"));

                    /* Try to load it */
                    TrustLibrary = LoadLibraryW(Buffer);
                    if (TrustLibrary)
                    {
                        /* And extract the special function out of it */
                        fEmbeddedCertFunc = (PVOID)GetProcAddress(TrustLibrary,
                                                                  "ImageOkToRunOnEmbeddedNT");
                    }
                }

                /* If we didn't get this far, set a failure code */
                if (!fEmbeddedCertFunc) Status = STATUS_UNSUCCESSFUL;
            }
        }
        else
        {
            /* Other systems have a registry entry for this */
            Status1 = NtOpenKey(&KeyHandle, KEY_READ, &KeyAttributes);
            if (NT_SUCCESS(Status1))
            {
                /* Close it, we'll query it through Rtl */
                NtClose(KeyHandle);

                /* Do the query, which will call a special callback */
                Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
                                                L"Session Manager",
                                                BasepAppCertTable,
                                                NULL,
                                                NULL);
                if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
                {
                    Status = STATUS_SUCCESS;
                }
            }
        }

        /* Free any buffer if we had one */
        if (Buffer) RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);

        /* Check for errors, or a missing embedded/custom certification DLL */
        if (!NT_SUCCESS(Status) ||
            (!(fEmbeddedCertFunc) && (IsListEmpty(&BasepAppCertDllsList))))
        {
            /* The subsystem is not active on this machine, so give up */
            g_HaveAppCerts = FALSE;
            g_AppCertStatus = Status;
        }
        else
        {
            /* We have certification DLLs active, remember this */
            g_HaveAppCerts = TRUE;
        }

        /* We are done the initialization phase, release the lock */
        g_AppCertInitialized = TRUE;
        RtlLeaveCriticalSection(&gcsAppCert);
    }

    /* If there's no certification DLLs present, return the failure code */
    if (!g_HaveAppCerts) return g_AppCertStatus;

    /* Otherwise, assume success and make sure we have *something* */
    ASSERT(fEmbeddedCertFunc || !IsListEmpty(&BasepAppCertDllsList));
    Status = STATUS_SUCCESS;

    /* If the something is an embedded certification DLL, call it and return */
    if (fEmbeddedCertFunc) return fEmbeddedCertFunc(ApplicationName);

    /* Otherwise we have custom certification DLLs, parse them */
    NextEntry = BasepAppCertDllsList.Flink;
    CertFlag = 2;
    while (NextEntry != &BasepAppCertDllsList)
    {
        /* Make sure the entry has a callback */
        Entry = CONTAINING_RECORD(NextEntry, BASEP_APPCERT_ENTRY, Entry);
        ASSERT(Entry->fPluginCertFunc != NULL);

        /* Call it and check if it failed */
        Status = Entry->fPluginCertFunc(ApplicationName, 1);
        if (!NT_SUCCESS(Status)) CertFlag = 3;

        /* Move on */
        NextEntry = NextEntry->Flink;
    }

    /* Now loop them again */
    NextEntry = BasepAppCertDllsList.Flink;
    while (NextEntry != &BasepAppCertDllsList)
    {
        /* Make sure the entry has a callback */
        Entry = CONTAINING_RECORD(NextEntry, BASEP_APPCERT_ENTRY, Entry);
        ASSERT(Entry->fPluginCertFunc != NULL);

        /* Call it, this time with the flag from the loop above */
        Status = Entry->fPluginCertFunc(ApplicationName, CertFlag);
    }

    /* All done, return the status */
    return Status;
}

NTSTATUS
WINAPI
BasepReplaceProcessThreadTokens(IN HANDLE TokenHandle,
                                IN HANDLE ProcessHandle,
                                IN HANDLE ThreadHandle)
{
    NTSTATUS Status;
    ANSI_STRING SaferiReplaceProcessThreadTokens = RTL_CONSTANT_STRING("SaferiReplaceProcessThreadTokens");

    /* Enter the application certification lock */
    RtlEnterCriticalSection(&gcsAppCert);

    /* Check if we already know the function */
    if (g_SaferReplaceProcessThreadTokens)
    {
        /* Call it */
        Status = g_SaferReplaceProcessThreadTokens(TokenHandle,
                                                   ProcessHandle,
                                                   ThreadHandle) ?
                                                   STATUS_SUCCESS :
                                                   STATUS_UNSUCCESSFUL;
    }
    else
    {
        /* Check if the app certification DLL isn't loaded */
        if (!(gSaferHandle) ||
            (gSaferHandle == (HMODULE)-1) ||
            (gSaferHandle == (HMODULE)-2))
        {
            /* Then we can't call the function */
            Status = STATUS_ENTRYPOINT_NOT_FOUND;
        }
        else
        {
            /* We have the DLL, find the address of the Safer function */
            Status = LdrGetProcedureAddress(gSaferHandle,
                                            &SaferiReplaceProcessThreadTokens,
                                            0,
                                            (PVOID*)&g_SaferReplaceProcessThreadTokens);
            if (NT_SUCCESS(Status))
            {
                /* Found it, now call it */
                Status = g_SaferReplaceProcessThreadTokens(TokenHandle,
                                                           ProcessHandle,
                                                           ThreadHandle) ?
                                                           STATUS_SUCCESS :
                                                           STATUS_UNSUCCESSFUL;
            }
            else
            {
                /* We couldn't find it, so this must be an unsupported DLL */
                LdrUnloadDll(gSaferHandle);
                gSaferHandle = NULL;
                Status = STATUS_ENTRYPOINT_NOT_FOUND;
            }
        }
    }

    /* Release the lock and return the result */
    RtlLeaveCriticalSection(&gcsAppCert);
    return Status;
}

VOID
WINAPI
BasepSxsCloseHandles(IN PBASE_MSG_SXS_HANDLES Handles)
{
    NTSTATUS Status;

    /* Sanity checks */
    ASSERT(Handles != NULL);
    ASSERT(Handles->Process == NULL || Handles->Process == NtCurrentProcess());

    /* Close the file handle */
    if (Handles->File)
    {
        Status = NtClose(Handles->File);
        ASSERT(NT_SUCCESS(Status));
    }

    /* Close the section handle */
    if (Handles->Section)
    {
        Status = NtClose(Handles->Section);
        ASSERT(NT_SUCCESS(Status));
    }

    /* Unmap the section view */
    if (Handles->ViewBase.QuadPart)
    {
        Status = NtUnmapViewOfSection(NtCurrentProcess(),
                                      (PVOID)(ULONG_PTR)Handles->ViewBase.QuadPart);
        ASSERT(NT_SUCCESS(Status));
    }
}

DECLSPEC_NORETURN
VOID
WINAPI
BaseProcessStartup(
    _In_ PPROCESS_START_ROUTINE lpStartAddress)
{
    _SEH2_TRY
    {
        /* Set our Start Address */
        NtSetInformationThread(NtCurrentThread(),
                               ThreadQuerySetWin32StartAddress,
                               &lpStartAddress,
                               sizeof(PPROCESS_START_ROUTINE));

        /* Call the Start Routine */
        ExitThread(lpStartAddress());
    }
    _SEH2_EXCEPT(UnhandledExceptionFilter(_SEH2_GetExceptionInformation()))
    {
        /* Get the Exit code from the SEH Handler */
        if (!BaseRunningInServerProcess)
        {
            /* Kill the whole process, usually */
            ExitProcess(_SEH2_GetExceptionCode());
        }
        else
        {
            /* If running inside CSRSS, kill just this thread */
            ExitThread(_SEH2_GetExceptionCode());
        }
    }
    _SEH2_END;
}

BOOLEAN
WINAPI
BasePushProcessParameters(IN ULONG ParameterFlags,
                          IN HANDLE ProcessHandle,
                          IN PPEB RemotePeb,
                          IN LPCWSTR ApplicationPathName,
                          IN LPWSTR lpCurrentDirectory,
                          IN LPWSTR lpCommandLine,
                          IN LPVOID lpEnvironment,
                          IN LPSTARTUPINFOW StartupInfo,
                          IN DWORD CreationFlags,
                          IN BOOL InheritHandles,
                          IN ULONG ImageSubsystem,
                          IN PVOID AppCompatData,
                          IN ULONG AppCompatDataSize)
{
    WCHAR FullPath[MAX_PATH + 5];
    PWCHAR Remaining, DllPathString, ScanChar;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters, RemoteParameters;
    PVOID RemoteAppCompatData;
    UNICODE_STRING DllPath, ImageName, CommandLine, CurrentDirectory;
    UNICODE_STRING Desktop, Shell, Runtime, Title;
    NTSTATUS Status;
    ULONG EnviroSize;
    SIZE_T Size;
    BOOLEAN HavePebLock = FALSE, Result;
    PPEB Peb = NtCurrentPeb();

    /* Get the full path name */
    Size = GetFullPathNameW(ApplicationPathName,
                            MAX_PATH + 4,
                            FullPath,
                            &Remaining);
    if ((Size) && (Size <= (MAX_PATH + 4)))
    {
        /* Get the DLL Path */
        DllPathString = BaseComputeProcessDllPath(FullPath, lpEnvironment);
        if (!DllPathString)
        {
            /* Fail */
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        /* Initialize Strings */
        RtlInitUnicodeString(&DllPath, DllPathString);
        RtlInitUnicodeString(&ImageName, FullPath);
    }
    else
    {
        /* Couldn't get the path name. Just take the original path */
        DllPathString = BaseComputeProcessDllPath((LPWSTR)ApplicationPathName,
                                                  lpEnvironment);
        if (!DllPathString)
        {
            /* Fail */
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        /* Initialize Strings */
        RtlInitUnicodeString(&DllPath, DllPathString);
        RtlInitUnicodeString(&ImageName, ApplicationPathName);
    }

    /* Initialize Strings */
    RtlInitUnicodeString(&CommandLine, lpCommandLine);
    RtlInitUnicodeString(&CurrentDirectory, lpCurrentDirectory);

    /* Initialize more Strings from the Startup Info */
    if (StartupInfo->lpDesktop)
    {
        RtlInitUnicodeString(&Desktop, StartupInfo->lpDesktop);
    }
    else
    {
        RtlInitUnicodeString(&Desktop, L"");
    }
    if (StartupInfo->lpReserved)
    {
        RtlInitUnicodeString(&Shell, StartupInfo->lpReserved);
    }
    else
    {
        RtlInitUnicodeString(&Shell, L"");
    }
    if (StartupInfo->lpTitle)
    {
        RtlInitUnicodeString(&Title, StartupInfo->lpTitle);
    }
    else
    {
        RtlInitUnicodeString(&Title, ApplicationPathName);
    }

    /* This one is special because the length can differ */
    Runtime.Buffer = (LPWSTR)StartupInfo->lpReserved2;
    Runtime.MaximumLength = Runtime.Length = StartupInfo->cbReserved2;

    /* Enforce no app compat data if the pointer was NULL */
    if (!AppCompatData) AppCompatDataSize = 0;

    /* Create the Parameter Block */
    ProcessParameters = NULL;
    DPRINT("ImageName: '%wZ'\n", &ImageName);
    DPRINT("DllPath  : '%wZ'\n", &DllPath);
    DPRINT("CurDir   : '%wZ'\n", &CurrentDirectory);
    DPRINT("CmdLine  : '%wZ'\n", &CommandLine);
    DPRINT("Title    : '%wZ'\n", &Title);
    DPRINT("Desktop  : '%wZ'\n", &Desktop);
    DPRINT("Shell    : '%wZ'\n", &Shell);
    DPRINT("Runtime  : '%wZ'\n", &Runtime);
    Status = RtlCreateProcessParameters(&ProcessParameters,
                                        &ImageName,
                                        &DllPath,
                                        lpCurrentDirectory ?
                                        &CurrentDirectory : NULL,
                                        &CommandLine,
                                        lpEnvironment,
                                        &Title,
                                        &Desktop,
                                        &Shell,
                                        &Runtime);
    if (!NT_SUCCESS(Status)) goto FailPath;

    /* Clear the current directory handle if not inheriting */
    if (!InheritHandles) ProcessParameters->CurrentDirectory.Handle = NULL;

    /* Check if the user passed in an environment */
    if (lpEnvironment)
    {
        /* We should've made it part of the parameters block, enforce this */
        ASSERT(ProcessParameters->Environment == lpEnvironment);
        lpEnvironment = ProcessParameters->Environment;
    }
    else
    {
        /* The user did not, so use the one from the current PEB */
        HavePebLock = TRUE;
        RtlAcquirePebLock();
        lpEnvironment = Peb->ProcessParameters->Environment;
    }

    /* Save pointer and start lookup */
    ScanChar = lpEnvironment;
    if (lpEnvironment)
    {
        /* Find the environment size */
        while (*ScanChar++) while (*ScanChar++);
        EnviroSize = (ULONG)((ULONG_PTR)ScanChar - (ULONG_PTR)lpEnvironment);

        /* Allocate and Initialize new Environment Block */
        Size = EnviroSize;
        ProcessParameters->Environment = NULL;
        Status = NtAllocateVirtualMemory(ProcessHandle,
                                         (PVOID*)&ProcessParameters->Environment,
                                         0,
                                         &Size,
                                         MEM_COMMIT,
                                         PAGE_READWRITE);
        if (!NT_SUCCESS(Status)) goto FailPath;

        /* Write the Environment Block */
        Status = NtWriteVirtualMemory(ProcessHandle,
                                      ProcessParameters->Environment,
                                      lpEnvironment,
                                      EnviroSize,
                                      NULL);

        /* No longer need the PEB lock anymore */
        if (HavePebLock)
        {
            /* Release it */
            RtlReleasePebLock();
            HavePebLock = FALSE;
        }

        /* Check if the write failed */
        if (!NT_SUCCESS(Status)) goto FailPath;
    }

    /* Write new parameters */
    ProcessParameters->StartingX = StartupInfo->dwX;
    ProcessParameters->StartingY = StartupInfo->dwY;
    ProcessParameters->CountX = StartupInfo->dwXSize;
    ProcessParameters->CountY = StartupInfo->dwYSize;
    ProcessParameters->CountCharsX = StartupInfo->dwXCountChars;
    ProcessParameters->CountCharsY = StartupInfo->dwYCountChars;
    ProcessParameters->FillAttribute = StartupInfo->dwFillAttribute;
    ProcessParameters->WindowFlags = StartupInfo->dwFlags;
    ProcessParameters->ShowWindowFlags = StartupInfo->wShowWindow;

    /* Write the handles only if we have to */
    if (StartupInfo->dwFlags &
        (STARTF_USESTDHANDLES | STARTF_USEHOTKEY | STARTF_SHELLPRIVATE))
    {
        ProcessParameters->StandardInput = StartupInfo->hStdInput;
        ProcessParameters->StandardOutput = StartupInfo->hStdOutput;
        ProcessParameters->StandardError = StartupInfo->hStdError;
    }

    /* Use Special Flags for ConDllInitialize in Kernel32 */
    if (CreationFlags & DETACHED_PROCESS)
    {
        ProcessParameters->ConsoleHandle = HANDLE_DETACHED_PROCESS;
    }
    else if (CreationFlags & CREATE_NEW_CONSOLE)
    {
        ProcessParameters->ConsoleHandle = HANDLE_CREATE_NEW_CONSOLE;
    }
    else if (CreationFlags & CREATE_NO_WINDOW)
    {
        ProcessParameters->ConsoleHandle = HANDLE_CREATE_NO_WINDOW;
    }
    else
    {
        /* Inherit our Console Handle */
        ProcessParameters->ConsoleHandle = Peb->ProcessParameters->ConsoleHandle;

        /* Make sure that the shell isn't trampling on our handles first */
        if (!(StartupInfo->dwFlags &
             (STARTF_USESTDHANDLES | STARTF_USEHOTKEY | STARTF_SHELLPRIVATE)))
        {
            /* Copy the handle if we are inheriting or if it's a console handle */
            if ((InheritHandles) ||
                (IsConsoleHandle(Peb->ProcessParameters->StandardInput)))
            {
                ProcessParameters->StandardInput = Peb->ProcessParameters->StandardInput;
            }
            if ((InheritHandles) ||
                (IsConsoleHandle(Peb->ProcessParameters->StandardOutput)))
            {
                ProcessParameters->StandardOutput = Peb->ProcessParameters->StandardOutput;
            }
            if ((InheritHandles) ||
                (IsConsoleHandle(Peb->ProcessParameters->StandardError)))
            {
                ProcessParameters->StandardError = Peb->ProcessParameters->StandardError;
            }
        }
    }

    /* Also set the Console Flag */
    if ((CreationFlags & CREATE_NEW_PROCESS_GROUP) &&
        (!(CreationFlags & CREATE_NEW_CONSOLE)))
    {
        ProcessParameters->ConsoleFlags = 1;
    }

    /* Check if there's a .local file present */
    if (ParameterFlags & 1)
    {
        ProcessParameters->Flags |= RTL_USER_PROCESS_PARAMETERS_LOCAL_DLL_PATH;
    }

    /* Check if we failed to open the IFEO key */
    if (ParameterFlags & 2)
    {
        ProcessParameters->Flags |= RTL_USER_PROCESS_PARAMETERS_IMAGE_KEY_MISSING;
    }

    /* Allocate memory for the parameter block */
    Size = ProcessParameters->Length;
    RemoteParameters = NULL;
    Status = NtAllocateVirtualMemory(ProcessHandle,
                                     (PVOID*)&RemoteParameters,
                                     0,
                                     &Size,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status)) goto FailPath;

    /* Set the allocated size */
    ProcessParameters->MaximumLength = Size;

    /* Handle some Parameter Flags */
    ProcessParameters->Flags |= (CreationFlags & PROFILE_USER) ?
                                 RTL_USER_PROCESS_PARAMETERS_PROFILE_USER : 0;
    ProcessParameters->Flags |= (CreationFlags & PROFILE_KERNEL) ?
                                 RTL_USER_PROCESS_PARAMETERS_PROFILE_KERNEL : 0;
    ProcessParameters->Flags |= (CreationFlags & PROFILE_SERVER) ?
                                 RTL_USER_PROCESS_PARAMETERS_PROFILE_SERVER : 0;
    ProcessParameters->Flags |= (Peb->ProcessParameters->Flags &
                                 RTL_USER_PROCESS_PARAMETERS_DISABLE_HEAP_CHECKS);

    /* Write the Parameter Block */
    Status = NtWriteVirtualMemory(ProcessHandle,
                                  RemoteParameters,
                                  ProcessParameters,
                                  ProcessParameters->Length,
                                  NULL);
    if (!NT_SUCCESS(Status)) goto FailPath;

    /* Write the PEB Pointer */
    Status = NtWriteVirtualMemory(ProcessHandle,
                                  &RemotePeb->ProcessParameters,
                                  &RemoteParameters,
                                  sizeof(PVOID),
                                  NULL);
    if (!NT_SUCCESS(Status)) goto FailPath;

    /* Check if there's any app compat data to write */
    RemoteAppCompatData = NULL;
    if (AppCompatData)
    {
        /* Allocate some space for the application compatibility data */
        Size = AppCompatDataSize;
        Status = NtAllocateVirtualMemory(ProcessHandle,
                                         &RemoteAppCompatData,
                                         0,
                                         &Size,
                                         MEM_COMMIT,
                                         PAGE_READWRITE);
        if (!NT_SUCCESS(Status)) goto FailPath;

        /* Write the application compatibility data */
        Status = NtWriteVirtualMemory(ProcessHandle,
                                      RemoteAppCompatData,
                                      AppCompatData,
                                      AppCompatDataSize,
                                      NULL);
        if (!NT_SUCCESS(Status)) goto FailPath;
    }

    /* Write the PEB Pointer to the app compat data (might be NULL) */
    Status = NtWriteVirtualMemory(ProcessHandle,
                                  &RemotePeb->pShimData,
                                  &RemoteAppCompatData,
                                  sizeof(PVOID),
                                  NULL);
    if (!NT_SUCCESS(Status)) goto FailPath;

    /* Now write Peb->ImageSubSystem */
    if (ImageSubsystem)
    {
        NtWriteVirtualMemory(ProcessHandle,
                             &RemotePeb->ImageSubsystem,
                             &ImageSubsystem,
                             sizeof(ImageSubsystem),
                             NULL);
    }

    /* Success path */
    Result = TRUE;

Quickie:
    /* Cleanup */
    if (HavePebLock) RtlReleasePebLock();
    RtlFreeHeap(RtlGetProcessHeap(), 0, DllPath.Buffer);
    if (ProcessParameters) RtlDestroyProcessParameters(ProcessParameters);
    return Result;
FailPath:
    DPRINT1("Failure to create process parameters: %lx\n", Status);
    BaseSetLastNTError(Status);
    Result = FALSE;
    goto Quickie;
}

VOID
WINAPI
InitCommandLines(VOID)
{
    NTSTATUS Status;

    /* Read the UNICODE_STRING from the PEB */
    BaseUnicodeCommandLine = NtCurrentPeb()->ProcessParameters->CommandLine;

    /* Convert to ANSI_STRING for the *A callers */
    Status = RtlUnicodeStringToAnsiString(&BaseAnsiCommandLine,
                                          &BaseUnicodeCommandLine,
                                          TRUE);
    if (!NT_SUCCESS(Status)) RtlInitEmptyAnsiString(&BaseAnsiCommandLine, 0, 0);
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
GetProcessAffinityMask(IN HANDLE hProcess,
                       OUT PDWORD_PTR lpProcessAffinityMask,
                       OUT PDWORD_PTR lpSystemAffinityMask)
{
    PROCESS_BASIC_INFORMATION ProcessInfo;
    NTSTATUS Status;

    /* Query information on the process from the kernel */
    Status = NtQueryInformationProcess(hProcess,
                                       ProcessBasicInformation,
                                       &ProcessInfo,
                                       sizeof(ProcessInfo),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Copy the affinity mask, and get the system one from our shared data */
    *lpProcessAffinityMask = (DWORD)ProcessInfo.AffinityMask;
    *lpSystemAffinityMask = (DWORD)BaseStaticServerData->SysInfo.ActiveProcessorsAffinityMask;
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetProcessAffinityMask(IN HANDLE hProcess,
                       IN DWORD_PTR dwProcessAffinityMask)
{
    NTSTATUS Status;

    /* Directly set the affinity mask */
    Status = NtSetInformationProcess(hProcess,
                                     ProcessAffinityMask,
                                     (PVOID)&dwProcessAffinityMask,
                                     sizeof(DWORD));
    if (!NT_SUCCESS(Status))
    {
        /* Handle failure */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Everything was ok */
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetProcessShutdownParameters(OUT LPDWORD lpdwLevel,
                             OUT LPDWORD lpdwFlags)
{
    BASE_API_MESSAGE ApiMessage;
    PBASE_GETSET_PROCESS_SHUTDOWN_PARAMS ShutdownParametersRequest = &ApiMessage.Data.ShutdownParametersRequest;

    /* Ask CSRSS for shutdown information */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(BASESRV_SERVERDLL_INDEX, BasepGetProcessShutdownParam),
                        sizeof(*ShutdownParametersRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        /* Return the failure from CSRSS */
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    /* Get the data back */
    *lpdwLevel = ShutdownParametersRequest->ShutdownLevel;
    *lpdwFlags = ShutdownParametersRequest->ShutdownFlags;
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetProcessShutdownParameters(IN DWORD dwLevel,
                             IN DWORD dwFlags)
{
    BASE_API_MESSAGE ApiMessage;
    PBASE_GETSET_PROCESS_SHUTDOWN_PARAMS ShutdownParametersRequest = &ApiMessage.Data.ShutdownParametersRequest;

    /* Write the data into the CSRSS request and send it */
    ShutdownParametersRequest->ShutdownLevel = dwLevel;
    ShutdownParametersRequest->ShutdownFlags = dwFlags;
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(BASESRV_SERVERDLL_INDEX, BasepSetProcessShutdownParam),
                        sizeof(*ShutdownParametersRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        /* Return the failure from CSRSS */
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    /* All went well */
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetProcessWorkingSetSizeEx(IN HANDLE hProcess,
                           OUT PSIZE_T lpMinimumWorkingSetSize,
                           OUT PSIZE_T lpMaximumWorkingSetSize,
                           OUT PDWORD Flags)
{
    QUOTA_LIMITS_EX QuotaLimits;
    NTSTATUS Status;

    /* Query the kernel about this */
    Status = NtQueryInformationProcess(hProcess,
                                       ProcessQuotaLimits,
                                       &QuotaLimits,
                                       sizeof(QuotaLimits),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Return error */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Copy the quota information out */
    *lpMinimumWorkingSetSize = QuotaLimits.MinimumWorkingSetSize;
    *lpMaximumWorkingSetSize = QuotaLimits.MaximumWorkingSetSize;
    *Flags = QuotaLimits.Flags;
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetProcessWorkingSetSize(IN HANDLE hProcess,
                         OUT PSIZE_T lpMinimumWorkingSetSize,
                         OUT PSIZE_T lpMaximumWorkingSetSize)
{
    DWORD Dummy;
    return GetProcessWorkingSetSizeEx(hProcess,
                                      lpMinimumWorkingSetSize,
                                      lpMaximumWorkingSetSize,
                                      &Dummy);
}

/*
 * @implemented
 */
BOOL
WINAPI
SetProcessWorkingSetSizeEx(IN HANDLE hProcess,
                           IN SIZE_T dwMinimumWorkingSetSize,
                           IN SIZE_T dwMaximumWorkingSetSize,
                           IN DWORD Flags)
{
    QUOTA_LIMITS_EX QuotaLimits;
    NTSTATUS Status, ReturnStatus;
    BOOL Result;
    PVOID State;
    ULONG Privilege = SE_INC_BASE_PRIORITY_PRIVILEGE;

    /* Zero out the input structure */
    RtlZeroMemory(&QuotaLimits, sizeof(QuotaLimits));

    /* Check if the caller sent any limits */
    if ((dwMinimumWorkingSetSize) && (dwMaximumWorkingSetSize))
    {
        /* Write the quota information */
        QuotaLimits.MinimumWorkingSetSize = dwMinimumWorkingSetSize;
        QuotaLimits.MaximumWorkingSetSize = dwMaximumWorkingSetSize;
        QuotaLimits.Flags = Flags;

        /* Acquire the required privilege */
        Status = RtlAcquirePrivilege(&Privilege, 1, 0, &State);

        /* Request the new quotas */
        ReturnStatus = NtSetInformationProcess(hProcess,
                                               ProcessQuotaLimits,
                                               &QuotaLimits,
                                               sizeof(QuotaLimits));
        Result = NT_SUCCESS(ReturnStatus);
        if (NT_SUCCESS(Status))
        {
            /* Release the privilege and set succes code */
            ASSERT(State != NULL);
            RtlReleasePrivilege(State);
            State = NULL;
        }
    }
    else
    {
        /* No limits, fail the call */
        ReturnStatus = STATUS_INVALID_PARAMETER;
        Result = FALSE;
    }

    /* Return result code, set error code if this was a failure */
    if (!Result) BaseSetLastNTError(ReturnStatus);
    return Result;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetProcessWorkingSetSize(IN HANDLE hProcess,
                         IN SIZE_T dwMinimumWorkingSetSize,
                         IN SIZE_T dwMaximumWorkingSetSize)
{
    /* Call the newer API */
    return SetProcessWorkingSetSizeEx(hProcess,
                                      dwMinimumWorkingSetSize,
                                      dwMaximumWorkingSetSize,
                                      0);
}

/*
 * @implemented
 */
BOOL
WINAPI
GetProcessTimes(IN HANDLE hProcess,
                IN LPFILETIME lpCreationTime,
                IN LPFILETIME lpExitTime,
                IN LPFILETIME lpKernelTime,
                IN LPFILETIME lpUserTime)
{
    KERNEL_USER_TIMES Kut;
    NTSTATUS Status;

    /* Query the times */
    Status = NtQueryInformationProcess(hProcess,
                                       ProcessTimes,
                                       &Kut,
                                       sizeof(Kut),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Handle failure */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Copy all the times and return success */
    lpCreationTime->dwLowDateTime = Kut.CreateTime.u.LowPart;
    lpCreationTime->dwHighDateTime = Kut.CreateTime.u.HighPart;
    lpExitTime->dwLowDateTime = Kut.ExitTime.u.LowPart;
    lpExitTime->dwHighDateTime = Kut.ExitTime.u.HighPart;
    lpKernelTime->dwLowDateTime = Kut.KernelTime.u.LowPart;
    lpKernelTime->dwHighDateTime = Kut.KernelTime.u.HighPart;
    lpUserTime->dwLowDateTime = Kut.UserTime.u.LowPart;
    lpUserTime->dwHighDateTime = Kut.UserTime.u.HighPart;
    return TRUE;
}

/*
 * @implemented
 */
HANDLE
WINAPI
GetCurrentProcess(VOID)
{
    return (HANDLE)NtCurrentProcess();
}

/*
 * @implemented
 */
HANDLE
WINAPI
GetCurrentThread(VOID)
{
    return (HANDLE)NtCurrentThread();
}

/*
 * @implemented
 */
DWORD
WINAPI
GetCurrentProcessId(VOID)
{
    return HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess);
}

/*
 * @implemented
 */
BOOL
WINAPI
GetExitCodeProcess(IN HANDLE hProcess,
                   IN LPDWORD lpExitCode)
{
    PROCESS_BASIC_INFORMATION ProcessBasic;
    NTSTATUS Status;

    /* Ask the kernel */
    Status = NtQueryInformationProcess(hProcess,
                                       ProcessBasicInformation,
                                       &ProcessBasic,
                                       sizeof(ProcessBasic),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        /* We failed, was this because this is a VDM process? */
        if (BaseCheckForVDM(hProcess, lpExitCode) != FALSE) return TRUE;

        /* Not a VDM process, fail the call */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Succes case, return the exit code */
    *lpExitCode = (DWORD)ProcessBasic.ExitStatus;
    return TRUE;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetProcessId(IN HANDLE Process)
{
    PROCESS_BASIC_INFORMATION ProcessBasic;
    NTSTATUS Status;

    /* Query the kernel */
    Status = NtQueryInformationProcess(Process,
                                       ProcessBasicInformation,
                                       &ProcessBasic,
                                       sizeof(ProcessBasic),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Handle failure */
        BaseSetLastNTError(Status);
        return 0;
    }

    /* Return the PID */
    return (DWORD)ProcessBasic.UniqueProcessId;
}

/*
 * @implemented
 */
HANDLE
WINAPI
OpenProcess(IN DWORD dwDesiredAccess,
            IN BOOL bInheritHandle,
            IN DWORD dwProcessId)
{
    NTSTATUS Status;
    HANDLE ProcessHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    CLIENT_ID ClientId;

    /* Setup the input client ID structure */
    ClientId.UniqueProcess = UlongToHandle(dwProcessId);
    ClientId.UniqueThread = 0;

    /* This is needed just to define the inheritance flags */
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               (bInheritHandle ? OBJ_INHERIT : 0),
                               NULL,
                               NULL);

    /* Now try to open the process */
    Status = NtOpenProcess(&ProcessHandle,
                           dwDesiredAccess,
                           &ObjectAttributes,
                           &ClientId);
    if (!NT_SUCCESS(Status))
    {
        /* Handle failure */
        BaseSetLastNTError(Status);
        return NULL;
    }

    /* Otherwise return a handle to the process */
    return ProcessHandle;
}

/*
 * @implemented
 */
VOID
WINAPI
RegisterWaitForInputIdle(IN WaitForInputIdleType lpfnRegisterWaitForInputIdle)
{
    /* Write the global function pointer */
    UserWaitForInputIdleRoutine = lpfnRegisterWaitForInputIdle;
}

/*
 * @implemented
 */
VOID
WINAPI
GetStartupInfoW(IN LPSTARTUPINFOW lpStartupInfo)
{
    PRTL_USER_PROCESS_PARAMETERS Params;

    /* Get the process parameters */
    Params = NtCurrentPeb()->ProcessParameters;

    /* Copy the data out of there */
    lpStartupInfo->cb = sizeof(STARTUPINFOW);
    lpStartupInfo->lpReserved = Params->ShellInfo.Buffer;
    lpStartupInfo->lpDesktop = Params->DesktopInfo.Buffer;
    lpStartupInfo->lpTitle = Params->WindowTitle.Buffer;
    lpStartupInfo->dwX = Params->StartingX;
    lpStartupInfo->dwY = Params->StartingY;
    lpStartupInfo->dwXSize = Params->CountX;
    lpStartupInfo->dwYSize = Params->CountY;
    lpStartupInfo->dwXCountChars = Params->CountCharsX;
    lpStartupInfo->dwYCountChars = Params->CountCharsY;
    lpStartupInfo->dwFillAttribute = Params->FillAttribute;
    lpStartupInfo->dwFlags = Params->WindowFlags;
    lpStartupInfo->wShowWindow = (WORD)Params->ShowWindowFlags;
    lpStartupInfo->cbReserved2 = Params->RuntimeData.Length;
    lpStartupInfo->lpReserved2 = (LPBYTE)Params->RuntimeData.Buffer;

    /* Check if the standard handles are being used for other features */
    if (lpStartupInfo->dwFlags & (STARTF_USESTDHANDLES |
                                  STARTF_USEHOTKEY |
                                  STARTF_SHELLPRIVATE))
    {
        /* These are, so copy the standard handles too */
        lpStartupInfo->hStdInput = Params->StandardInput;
        lpStartupInfo->hStdOutput = Params->StandardOutput;
        lpStartupInfo->hStdError = Params->StandardError;
    }
}

/*
 * @implemented
 */
VOID
WINAPI
GetStartupInfoA(IN LPSTARTUPINFOA lpStartupInfo)
{
    PRTL_USER_PROCESS_PARAMETERS Params;
    ANSI_STRING TitleString, ShellString, DesktopString;
    LPSTARTUPINFOA StartupInfo;
    NTSTATUS Status;

    /* Get the cached information as well as the PEB parameters */
    StartupInfo = BaseAnsiStartupInfo;
    Params = NtCurrentPeb()->ProcessParameters;

    /* Check if this is the first time we have to get the cached version */
    while (!StartupInfo)
    {
        /* Create new ANSI startup info */
        StartupInfo = RtlAllocateHeap(RtlGetProcessHeap(),
                                      0,
                                      sizeof(*StartupInfo));
        if (StartupInfo)
        {
            /* Zero out string pointers in case we fail to create them */
            StartupInfo->lpReserved = NULL;
            StartupInfo->lpDesktop = NULL;
            StartupInfo->lpTitle = NULL;

            /* Set the size */
            StartupInfo->cb = sizeof(*StartupInfo);

            /* Copy what's already stored in the PEB */
            StartupInfo->dwX = Params->StartingX;
            StartupInfo->dwY = Params->StartingY;
            StartupInfo->dwXSize = Params->CountX;
            StartupInfo->dwYSize = Params->CountY;
            StartupInfo->dwXCountChars = Params->CountCharsX;
            StartupInfo->dwYCountChars = Params->CountCharsY;
            StartupInfo->dwFillAttribute = Params->FillAttribute;
            StartupInfo->dwFlags = Params->WindowFlags;
            StartupInfo->wShowWindow = (WORD)Params->ShowWindowFlags;
            StartupInfo->cbReserved2 = Params->RuntimeData.Length;
            StartupInfo->lpReserved2 = (LPBYTE)Params->RuntimeData.Buffer;
            StartupInfo->hStdInput = Params->StandardInput;
            StartupInfo->hStdOutput = Params->StandardOutput;
            StartupInfo->hStdError = Params->StandardError;

            /* Copy shell info string */
            Status = RtlUnicodeStringToAnsiString(&ShellString,
                                                  &Params->ShellInfo,
                                                  TRUE);
            if (NT_SUCCESS(Status))
            {
                /* Save it */
                StartupInfo->lpReserved = ShellString.Buffer;

                /* Copy desktop info string */
                Status = RtlUnicodeStringToAnsiString(&DesktopString,
                                                      &Params->DesktopInfo,
                                                      TRUE);
                if (NT_SUCCESS(Status))
                {
                    /* Save it */
                    StartupInfo->lpDesktop = DesktopString.Buffer;

                    /* Copy window title string */
                    Status = RtlUnicodeStringToAnsiString(&TitleString,
                                                          &Params->WindowTitle,
                                                          TRUE);
                    if (NT_SUCCESS(Status))
                    {
                        /* Save it */
                        StartupInfo->lpTitle = TitleString.Buffer;

                        /* We finished with the ANSI version, try to cache it */
                        if (!InterlockedCompareExchangePointer((PVOID*)&BaseAnsiStartupInfo,
                                                               StartupInfo,
                                                               NULL))
                        {
                            /* We were the first thread through, use the data */
                            break;
                        }

                        /* Someone beat us to it, use their data instead */
                        StartupInfo = BaseAnsiStartupInfo;
                        Status = STATUS_SUCCESS;

                        /* We're going to free our own stuff, but not raise */
                        RtlFreeAnsiString(&TitleString);
                    }
                    RtlFreeAnsiString(&DesktopString);
                }
                RtlFreeAnsiString(&ShellString);
            }
            RtlFreeHeap(RtlGetProcessHeap(), 0, StartupInfo);
        }
        else
        {
            /* No memory, fail */
            Status = STATUS_NO_MEMORY;
        }

        /* Raise an error unless we got here due to the race condition */
        if (!NT_SUCCESS(Status)) RtlRaiseStatus(Status);
    }

    /* Now copy from the cached ANSI version */
    lpStartupInfo->cb = StartupInfo->cb;
    lpStartupInfo->lpReserved = StartupInfo->lpReserved;
    lpStartupInfo->lpDesktop = StartupInfo->lpDesktop;
    lpStartupInfo->lpTitle = StartupInfo->lpTitle;
    lpStartupInfo->dwX = StartupInfo->dwX;
    lpStartupInfo->dwY = StartupInfo->dwY;
    lpStartupInfo->dwXSize = StartupInfo->dwXSize;
    lpStartupInfo->dwYSize = StartupInfo->dwYSize;
    lpStartupInfo->dwXCountChars = StartupInfo->dwXCountChars;
    lpStartupInfo->dwYCountChars = StartupInfo->dwYCountChars;
    lpStartupInfo->dwFillAttribute = StartupInfo->dwFillAttribute;
    lpStartupInfo->dwFlags = StartupInfo->dwFlags;
    lpStartupInfo->wShowWindow = StartupInfo->wShowWindow;
    lpStartupInfo->cbReserved2 = StartupInfo->cbReserved2;
    lpStartupInfo->lpReserved2 = StartupInfo->lpReserved2;

    /* Check if the shell is hijacking the handles for other features */
    if (lpStartupInfo->dwFlags &
        (STARTF_USESTDHANDLES | STARTF_USEHOTKEY | STARTF_SHELLPRIVATE))
    {
        /* It isn't, so we can return the raw values */
        lpStartupInfo->hStdInput = StartupInfo->hStdInput;
        lpStartupInfo->hStdOutput = StartupInfo->hStdOutput;
        lpStartupInfo->hStdError = StartupInfo->hStdError;
    }
    else
    {
        /* It is, so make sure nobody uses these as console handles */
        lpStartupInfo->hStdInput = INVALID_HANDLE_VALUE;
        lpStartupInfo->hStdOutput = INVALID_HANDLE_VALUE;
        lpStartupInfo->hStdError = INVALID_HANDLE_VALUE;
    }
}

/*
 * @implemented
 */
BOOL
WINAPI
FlushInstructionCache(IN HANDLE hProcess,
                      IN LPCVOID lpBaseAddress,
                      IN SIZE_T nSize)
{
    NTSTATUS Status;

    /* Call the native function */
    Status = NtFlushInstructionCache(hProcess, (PVOID)lpBaseAddress, nSize);
    if (!NT_SUCCESS(Status))
    {
        /* Handle failure case */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* All good */
    return TRUE;
}

/*
 * @implemented
 */
VOID
WINAPI
ExitProcess(IN UINT uExitCode)
{
    BASE_API_MESSAGE ApiMessage;
    PBASE_EXIT_PROCESS ExitProcessRequest = &ApiMessage.Data.ExitProcessRequest;

    ASSERT(!BaseRunningInServerProcess);

    _SEH2_TRY
    {
        /* Acquire the PEB lock */
        RtlAcquirePebLock();

        /* Kill all the threads */
        NtTerminateProcess(NULL, uExitCode);

        /* Unload all DLLs */
        LdrShutdownProcess();

        /* Notify Base Server of process termination */
        ExitProcessRequest->uExitCode = uExitCode;
        CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                            NULL,
                            CSR_CREATE_API_NUMBER(BASESRV_SERVERDLL_INDEX, BasepExitProcess),
                            sizeof(*ExitProcessRequest));

        /* Now do it again */
        NtTerminateProcess(NtCurrentProcess(), uExitCode);
    }
    _SEH2_FINALLY
    {
        /* Release the PEB lock */
        RtlReleasePebLock();
    }
    _SEH2_END;

    /* should never get here */
    ASSERT(0);
    while(1);
}

/*
 * @implemented
 */
BOOL
WINAPI
TerminateProcess(IN HANDLE hProcess,
                 IN UINT uExitCode)
{
    NTSTATUS Status;

    /* Check if no handle was passed in */
    if (!hProcess)
    {
        /* Set error code */
        SetLastError(ERROR_INVALID_HANDLE);
    }
    else
    {
        /* Otherwise, try to terminate the process */
        Status = NtTerminateProcess(hProcess, uExitCode);
        if (NT_SUCCESS(Status)) return TRUE;

        /* It failed, convert error code */
        BaseSetLastNTError(Status);
    }

    /* This is the failure path */
    return FALSE;
}

/*
 * @implemented
 */
VOID
WINAPI
FatalAppExitA(UINT uAction,
              LPCSTR lpMessageText)
{
    PUNICODE_STRING MessageTextU;
    ANSI_STRING MessageText;
    NTSTATUS Status;

    /* Initialize the string using the static TEB pointer */
    MessageTextU = &NtCurrentTeb()->StaticUnicodeString;
    RtlInitAnsiString(&MessageText, (LPSTR)lpMessageText);

    /* Convert to unicode, or just exit normally if this failed */
    Status = RtlAnsiStringToUnicodeString(MessageTextU, &MessageText, FALSE);
    if (!NT_SUCCESS(Status)) ExitProcess(0);

    /* Call the Wide function */
    FatalAppExitW(uAction, MessageTextU->Buffer);
}

/*
 * @implemented
 */
VOID
WINAPI
FatalAppExitW(IN UINT uAction,
              IN LPCWSTR lpMessageText)
{
    UNICODE_STRING UnicodeString;
    ULONG Response;
    NTSTATUS Status;

    /* Setup the string to print out */
    RtlInitUnicodeString(&UnicodeString, lpMessageText);

    /* Display the hard error no matter what */
    Status = NtRaiseHardError(STATUS_FATAL_APP_EXIT | HARDERROR_OVERRIDE_ERRORMODE,
                              1,
                              1,
                              (PULONG_PTR)&UnicodeString,
#if DBG
    /* On Checked builds, Windows allows the user to cancel the operation */
                              OptionOkCancel,
#else
                              OptionOk,
#endif
                              &Response);

    /* Give the user a chance to abort */
    if ((NT_SUCCESS(Status)) && (Response == ResponseCancel))
    {
        return;
    }

    /* Otherwise kill the process */
    ExitProcess(0);
}

/*
 * @implemented
 */
VOID
WINAPI
FatalExit(IN int ExitCode)
{
#if DBG
    /* On Checked builds, Windows gives the user a nice little debugger UI */
    CHAR ch[2];
    DbgPrint("FatalExit...\n");
    DbgPrint("\n");

    while (TRUE)
    {
        DbgPrompt( "A (Abort), B (Break), I (Ignore)? ", ch, sizeof(ch));
        switch (ch[0])
        {
            case 'B': case 'b':
                 DbgBreakPoint();
                 break;

            case 'A': case 'a':
                ExitProcess(ExitCode);

            case 'I': case 'i':
                return;
        }
    }
#endif
    /* On other builds, just kill the process */
    ExitProcess(ExitCode);
}

/*
 * @implemented
 */
DWORD
WINAPI
GetPriorityClass(IN HANDLE hProcess)
{
    NTSTATUS Status;
    PROCESS_PRIORITY_CLASS DECLSPEC_ALIGN(4) PriorityClass;

    /* Query the kernel */
    Status = NtQueryInformationProcess(hProcess,
                                       ProcessPriorityClass,
                                       &PriorityClass,
                                       sizeof(PriorityClass),
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Handle the conversion from NT to Win32 classes */
        switch (PriorityClass.PriorityClass)
        {
            case PROCESS_PRIORITY_CLASS_IDLE: return IDLE_PRIORITY_CLASS;
            case PROCESS_PRIORITY_CLASS_BELOW_NORMAL: return BELOW_NORMAL_PRIORITY_CLASS;
            case PROCESS_PRIORITY_CLASS_ABOVE_NORMAL: return ABOVE_NORMAL_PRIORITY_CLASS;
            case PROCESS_PRIORITY_CLASS_HIGH: return HIGH_PRIORITY_CLASS;
            case PROCESS_PRIORITY_CLASS_REALTIME: return REALTIME_PRIORITY_CLASS;
            case PROCESS_PRIORITY_CLASS_NORMAL: default: return NORMAL_PRIORITY_CLASS;
        }
    }

    /* Failure path */
    BaseSetLastNTError(Status);
    return 0;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetPriorityClass(IN HANDLE hProcess,
                 IN DWORD dwPriorityClass)
{
    NTSTATUS Status;
    PVOID State = NULL;
    PROCESS_PRIORITY_CLASS PriorityClass;

    /* Handle conversion from Win32 to NT priority classes */
    switch (dwPriorityClass)
    {
        case IDLE_PRIORITY_CLASS:
            PriorityClass.PriorityClass = PROCESS_PRIORITY_CLASS_IDLE;
            break;

        case BELOW_NORMAL_PRIORITY_CLASS:
            PriorityClass.PriorityClass = PROCESS_PRIORITY_CLASS_BELOW_NORMAL;
            break;

        case NORMAL_PRIORITY_CLASS:
            PriorityClass.PriorityClass = PROCESS_PRIORITY_CLASS_NORMAL;
            break;

        case ABOVE_NORMAL_PRIORITY_CLASS:
            PriorityClass.PriorityClass = PROCESS_PRIORITY_CLASS_ABOVE_NORMAL;
            break;

        case HIGH_PRIORITY_CLASS:
            PriorityClass.PriorityClass = PROCESS_PRIORITY_CLASS_HIGH;
            break;

        case REALTIME_PRIORITY_CLASS:
            /* Try to acquire the privilege. If it fails, just use HIGH */
            State = BasepIsRealtimeAllowed(TRUE);
            PriorityClass.PriorityClass = PROCESS_PRIORITY_CLASS_HIGH;
            PriorityClass.PriorityClass += (State != NULL);
            break;

        default:
            /* Unrecognized priority classes don't make it to the kernel */
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
    }

    /* Send the request to the kernel, and don't touch the foreground flag */
    PriorityClass.Foreground = FALSE;
    Status = NtSetInformationProcess(hProcess,
                                     ProcessPriorityClass,
                                     &PriorityClass,
                                     sizeof(PROCESS_PRIORITY_CLASS));

    /* Release the privilege if we had it */
    if (State) RtlReleasePrivilege(State);
    if (!NT_SUCCESS(Status))
    {
        /* Handle error path */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* All done */
    return TRUE;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetProcessVersion(IN DWORD ProcessId)
{
    DWORD Version = 0;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_DOS_HEADER DosHeader;
    PPEB Peb;
    PROCESS_BASIC_INFORMATION ProcessBasicInfo;
    PVOID BaseAddress;
    ULONG e_lfanew;
    HANDLE ProcessHandle = NULL;
    NTSTATUS Status;
    USHORT VersionData[2];
    BOOLEAN Result;

    /* We'll be accessing stuff that can fault, so protect everything with SEH */
    _SEH2_TRY
    {
        /* It this an in-process or out-of-process request? */
        if (!(ProcessId) || (GetCurrentProcessId() == ProcessId))
        {
            /* It's in-process, so just read our own header */
            NtHeader = RtlImageNtHeader(NtCurrentPeb()->ImageBaseAddress);
            if (!NtHeader)
            {
                /* Unable to read the NT header, something is wrong here... */
                Status = STATUS_INVALID_IMAGE_FORMAT;
                goto Error;
            }

            /* Get the version straight out of the NT header */
            Version = MAKELONG(NtHeader->OptionalHeader.MinorSubsystemVersion,
                               NtHeader->OptionalHeader.MajorSubsystemVersion);
        }
        else
        {
            /* Out-of-process, so open it */
            ProcessHandle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
                                        FALSE,
                                        ProcessId);
            if (!ProcessHandle) _SEH2_YIELD(return 0);

            /* Try to find out where its PEB lives */
            Status = NtQueryInformationProcess(ProcessHandle,
                                               ProcessBasicInformation,
                                               &ProcessBasicInfo,
                                               sizeof(ProcessBasicInfo),
                                               NULL);

            if (!NT_SUCCESS(Status)) goto Error;
            Peb = ProcessBasicInfo.PebBaseAddress;

            /* Now that we have the PEB, read the image base address out of it */
            Result = ReadProcessMemory(ProcessHandle,
                                       &Peb->ImageBaseAddress,
                                       &BaseAddress,
                                       sizeof(BaseAddress),
                                       NULL);
            if (!Result) goto Error;

            /* Now read the e_lfanew (offset to NT header) from the base */
            DosHeader = BaseAddress;
            Result = ReadProcessMemory(ProcessHandle,
                                       &DosHeader->e_lfanew,
                                       &e_lfanew,
                                       sizeof(e_lfanew),
                                       NULL);
            if (!Result) goto Error;

            /* And finally, read the NT header itself by adding the offset */
            NtHeader = (PVOID)((ULONG_PTR)BaseAddress + e_lfanew);
            Result = ReadProcessMemory(ProcessHandle,
                                       &NtHeader->OptionalHeader.MajorSubsystemVersion,
                                       &VersionData,
                                       sizeof(VersionData),
                                       NULL);
            if (!Result) goto Error;

            /* Get the version straight out of the NT header */
            Version = MAKELONG(VersionData[0], VersionData[1]);

Error:
            /* If there was an error anywhere, set the last error */
            if (!NT_SUCCESS(Status)) BaseSetLastNTError(Status);
        }
    }
    _SEH2_FINALLY
    {
        /* Close the process handle */
        if (ProcessHandle) CloseHandle(ProcessHandle);
    }
    _SEH2_END;

    /* And return the version data */
    return Version;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetProcessIoCounters(IN HANDLE hProcess,
                     OUT PIO_COUNTERS lpIoCounters)
{
    NTSTATUS Status;

    /* Query the kernel. Structures are identical, so let it do the copy too. */
    Status = NtQueryInformationProcess(hProcess,
                                       ProcessIoCounters,
                                       lpIoCounters,
                                       sizeof(IO_COUNTERS),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Handle error path */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* All done */
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetProcessPriorityBoost(IN HANDLE hProcess,
                        OUT PBOOL pDisablePriorityBoost)
{
    NTSTATUS Status;
    ULONG PriorityBoost;

    /* Query the kernel */
    Status = NtQueryInformationProcess(hProcess,
                                       ProcessPriorityBoost,
                                       &PriorityBoost,
                                       sizeof(PriorityBoost),
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Convert from ULONG to a BOOL */
        *pDisablePriorityBoost = PriorityBoost ? TRUE : FALSE;
        return TRUE;
    }

    /* Handle error path */
    BaseSetLastNTError(Status);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetProcessPriorityBoost(IN HANDLE hProcess,
                        IN BOOL bDisablePriorityBoost)
{
    NTSTATUS Status;
    ULONG PriorityBoost;

    /* Enforce that this is a BOOL, and send it to the kernel as a ULONG */
    PriorityBoost = (bDisablePriorityBoost ? TRUE : FALSE);
    Status = NtSetInformationProcess(hProcess,
                                     ProcessPriorityBoost,
                                     &PriorityBoost,
                                     sizeof(ULONG));
    if (!NT_SUCCESS(Status))
    {
        /* Handle error path */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* All done */
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetProcessHandleCount(IN HANDLE hProcess,
                      OUT PDWORD pdwHandleCount)
{
    ULONG phc;
    NTSTATUS Status;

    /* Query the kernel */
    Status = NtQueryInformationProcess(hProcess,
                                       ProcessHandleCount,
                                       &phc,
                                       sizeof(phc),
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Copy the count and return success */
        *pdwHandleCount = phc;
        return TRUE;
    }

    /* Handle error path */
    BaseSetLastNTError(Status);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
IsWow64Process(IN HANDLE hProcess,
               OUT PBOOL Wow64Process)
{
    ULONG_PTR pbi;
    NTSTATUS Status;

    /* Query the kernel */
    Status = NtQueryInformationProcess(hProcess,
                                       ProcessWow64Information,
                                       &pbi,
                                       sizeof(pbi),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Handle error path */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Enforce this is a BOOL, and return success */
    *Wow64Process = (pbi != 0);
    return TRUE;
}

/*
 * @implemented
 */
LPSTR
WINAPI
GetCommandLineA(VOID)
{
    return BaseAnsiCommandLine.Buffer;
}

/*
 * @implemented
 */
LPWSTR
WINAPI
GetCommandLineW(VOID)
{
    return BaseUnicodeCommandLine.Buffer;
}

/*
 * @implemented
 */
BOOL
NTAPI
ReadProcessMemory(IN HANDLE hProcess,
                  IN LPCVOID lpBaseAddress,
                  IN LPVOID lpBuffer,
                  IN SIZE_T nSize,
                  OUT SIZE_T* lpNumberOfBytesRead)
{
    NTSTATUS Status;

    /* Do the read */
    Status = NtReadVirtualMemory(hProcess,
                                 (PVOID)lpBaseAddress,
                                 lpBuffer,
                                 nSize,
                                 &nSize);

    /* In user-mode, this parameter is optional */
    if (lpNumberOfBytesRead) *lpNumberOfBytesRead = nSize;
    if (!NT_SUCCESS(Status))
    {
        /* We failed */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Return success */
    return TRUE;
}

/*
 * @implemented
 */
BOOL
NTAPI
WriteProcessMemory(IN HANDLE hProcess,
                   IN LPVOID lpBaseAddress,
                   IN LPCVOID lpBuffer,
                   IN SIZE_T nSize,
                   OUT SIZE_T *lpNumberOfBytesWritten)
{
    NTSTATUS Status;
    ULONG OldValue;
    SIZE_T RegionSize;
    PVOID Base;
    BOOLEAN UnProtect;

    /* Set parameters for protect call */
    RegionSize = nSize;
    Base = lpBaseAddress;

    /* Check the current status */
    Status = NtProtectVirtualMemory(hProcess,
                                    &Base,
                                    &RegionSize,
                                    PAGE_EXECUTE_READWRITE,
                                    &OldValue);
    if (NT_SUCCESS(Status))
    {
        /* Check if we are unprotecting */
        UnProtect = OldValue & (PAGE_READWRITE |
                                PAGE_WRITECOPY |
                                PAGE_EXECUTE_READWRITE |
                                PAGE_EXECUTE_WRITECOPY) ? FALSE : TRUE;
        if (!UnProtect)
        {
            /* Set the new protection */
            Status = NtProtectVirtualMemory(hProcess,
                                            &Base,
                                            &RegionSize,
                                            OldValue,
                                            &OldValue);

            /* Write the memory */
            Status = NtWriteVirtualMemory(hProcess,
                                          lpBaseAddress,
                                          (LPVOID)lpBuffer,
                                          nSize,
                                          &nSize);

            /* In Win32, the parameter is optional, so handle this case */
            if (lpNumberOfBytesWritten) *lpNumberOfBytesWritten = nSize;

            if (!NT_SUCCESS(Status))
            {
                /* We failed */
                BaseSetLastNTError(Status);
                return FALSE;
            }

            /* Flush the ITLB */
            NtFlushInstructionCache(hProcess, lpBaseAddress, nSize);
            return TRUE;
        }
        else
        {
            /* Check if we were read only */
            if (OldValue & (PAGE_NOACCESS | PAGE_READONLY))
            {
                /* Restore protection and fail */
                NtProtectVirtualMemory(hProcess,
                                       &Base,
                                       &RegionSize,
                                       OldValue,
                                       &OldValue);
                BaseSetLastNTError(STATUS_ACCESS_VIOLATION);

                /* Note: This is what Windows returns and code depends on it */
                return STATUS_ACCESS_VIOLATION;
            }

            /* Otherwise, do the write */
            Status = NtWriteVirtualMemory(hProcess,
                                          lpBaseAddress,
                                          (LPVOID)lpBuffer,
                                          nSize,
                                          &nSize);

            /* In Win32, the parameter is optional, so handle this case */
            if (lpNumberOfBytesWritten) *lpNumberOfBytesWritten = nSize;

            /* And restore the protection */
            NtProtectVirtualMemory(hProcess,
                                   &Base,
                                   &RegionSize,
                                   OldValue,
                                   &OldValue);
            if (!NT_SUCCESS(Status))
            {
                /* We failed */
                BaseSetLastNTError(STATUS_ACCESS_VIOLATION);

                /* Note: This is what Windows returns and code depends on it */
                return STATUS_ACCESS_VIOLATION;
            }

            /* Flush the ITLB */
            NtFlushInstructionCache(hProcess, lpBaseAddress, nSize);
            return TRUE;
        }
    }
    else
    {
        /* We failed */
        BaseSetLastNTError(Status);
        return FALSE;
    }
}

/*
 * @implemented
 */
BOOL
WINAPI
ProcessIdToSessionId(IN DWORD dwProcessId,
                     OUT PDWORD pSessionId)
{
    PROCESS_SESSION_INFORMATION SessionInformation;
    OBJECT_ATTRIBUTES ObjectAttributes;
    CLIENT_ID ClientId;
    HANDLE ProcessHandle;
    NTSTATUS Status;

    /* Do a quick check if the pointer is not writable */
    if (IsBadWritePtr(pSessionId, sizeof(DWORD)))
    {
        /* Fail fast */
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Open the process passed in by ID */
    ClientId.UniqueProcess = UlongToHandle(dwProcessId);
    ClientId.UniqueThread = 0;
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
    Status = NtOpenProcess(&ProcessHandle,
                           PROCESS_QUERY_INFORMATION,
                           &ObjectAttributes,
                           &ClientId);
    if (NT_SUCCESS(Status))
    {
        /* Query the session ID from the kernel */
        Status = NtQueryInformationProcess(ProcessHandle,
                                           ProcessSessionInformation,
                                           &SessionInformation,
                                           sizeof(SessionInformation),
                                           NULL);

        /* Close the handle and check if we succeeded */
        NtClose(ProcessHandle);
        if (NT_SUCCESS(Status))
        {
            /* Return the session ID */
            *pSessionId = SessionInformation.SessionId;
            return TRUE;
        }
    }

    /* Set error code and fail */
    BaseSetLastNTError(Status);
    return FALSE;
}


#define AddToHandle(x,y)  (x) = (HANDLE)((ULONG_PTR)(x) | (y));
#define RemoveFromHandle(x,y)  (x) = (HANDLE)((ULONG_PTR)(x) & ~(y));
C_ASSERT(PROCESS_PRIORITY_CLASS_REALTIME == (PROCESS_PRIORITY_CLASS_HIGH + 1));

/*
 * @implemented
 */
BOOL
WINAPI
CreateProcessInternalW(IN HANDLE hUserToken,
                       IN LPCWSTR lpApplicationName,
                       IN LPWSTR lpCommandLine,
                       IN LPSECURITY_ATTRIBUTES lpProcessAttributes,
                       IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
                       IN BOOL bInheritHandles,
                       IN DWORD dwCreationFlags,
                       IN LPVOID lpEnvironment,
                       IN LPCWSTR lpCurrentDirectory,
                       IN LPSTARTUPINFOW lpStartupInfo,
                       IN LPPROCESS_INFORMATION lpProcessInformation,
                       OUT PHANDLE hNewToken)
{
    //
    // Core variables used for creating the initial process and thread
    //
    SECURITY_ATTRIBUTES LocalThreadAttributes, LocalProcessAttributes;
    OBJECT_ATTRIBUTES LocalObjectAttributes;
    POBJECT_ATTRIBUTES ObjectAttributes;
    SECTION_IMAGE_INFORMATION ImageInformation;
    IO_STATUS_BLOCK IoStatusBlock;
    CLIENT_ID ClientId;
    ULONG NoWindow, StackSize, ErrorCode, Flags;
    SIZE_T RegionSize;
    USHORT ImageMachine;
    ULONG ParameterFlags, PrivilegeValue, HardErrorMode, ErrorResponse;
    ULONG_PTR ErrorParameters[2];
    BOOLEAN InJob, SaferNeeded, UseLargePages, HavePrivilege;
    BOOLEAN QuerySection, SkipSaferAndAppCompat;
    CONTEXT Context;
    BASE_API_MESSAGE CsrMsg[2];
    PBASE_CREATE_PROCESS CreateProcessMsg;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    PVOID BaseAddress, PrivilegeState, RealTimePrivilegeState;
    HANDLE DebugHandle, TokenHandle, JobHandle, KeyHandle, ThreadHandle;
    HANDLE FileHandle, SectionHandle, ProcessHandle;
    ULONG ResumeCount;
    PROCESS_PRIORITY_CLASS PriorityClass;
    NTSTATUS Status, AppCompatStatus, SaferStatus, IFEOStatus, ImageDbgStatus;
    PPEB Peb, RemotePeb;
    PTEB Teb;
    INITIAL_TEB InitialTeb;
    PVOID TibValue;
    PIMAGE_NT_HEADERS NtHeaders;
    STARTUPINFOW StartupInfo;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    UNICODE_STRING DebuggerString;
    BOOL Result;
    //
    // Variables used for command-line and argument parsing
    //
    PCHAR pcScan;
    SIZE_T n;
    WCHAR SaveChar;
    ULONG Length, FileAttribs, CmdQuoteLength;
    ULONG ResultSize;
    SIZE_T EnvironmentLength, CmdLineLength;
    PWCHAR QuotedCmdLine, AnsiCmdCommand, ExtBuffer, CurrentDirectory;
    PWCHAR NullBuffer, ScanString, NameBuffer, SearchPath, DebuggerCmdLine;
    ANSI_STRING AnsiEnv;
    UNICODE_STRING UnicodeEnv, PathName;
    BOOLEAN SearchRetry, QuotesNeeded, CmdLineIsAppName, HasQuotes;

    //
    // Variables used for Fusion/SxS (Side-by-Side Assemblies)
    //
    RTL_PATH_TYPE SxsPathType, PathType;
#if _SXS_SUPPORT_ENABLED_
    PRTL_BUFFER ByteBuffer;
    PRTL_UNICODE_STRING_BUFFER ThisBuffer, Buffer, SxsStaticBuffers[5];
    PRTL_UNICODE_STRING_BUFFER* BufferHead, SxsStringBuffer;
    RTL_UNICODE_STRING_BUFFER SxsWin32ManifestPath, SxsNtManifestPath;
    RTL_UNICODE_STRING_BUFFER SxsWin32PolicyPath, SxsNtPolicyPath;
    RTL_UNICODE_STRING_BUFFER SxsWin32AssemblyDirectory;
    BASE_MSG_SXS_HANDLES MappedHandles, Handles, FileHandles;
    PVOID CapturedStrings[3];
    SXS_WIN32_NT_PATH_PAIR ExePathPair, ManifestPathPair, PolicyPathPair;
    SXS_OVERRIDE_MANIFEST OverrideManifest;
    UNICODE_STRING FreeString, SxsNtExePath;
    PWCHAR SxsConglomeratedBuffer, StaticBuffer;
    ULONG ConglomeratedBufferSizeBytes, StaticBufferSize, i;
#endif
    ULONG FusionFlags;

    //
    // Variables used for path conversion (and partially Fusion/SxS)
    //
    PWCHAR FilePart, PathBuffer, FreeBuffer;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME_U SxsWin32RelativePath;
    UNICODE_STRING PathBufferString, SxsWin32ExePath;

    //
    // Variables used by Application Compatibility (and partially Fusion/SxS)
    //
    PVOID AppCompatSxsData, AppCompatData;
    ULONG AppCompatSxsDataSize, AppCompatDataSize;
    //
    // Variables used by VDM (Virtual Dos Machine) and WOW32 (16-bit Support)
    //
    ULONG BinarySubType, VdmBinaryType, VdmTask, VdmReserve;
    ULONG VdmUndoLevel;
    BOOLEAN UseVdmReserve;
    HANDLE VdmWaitObject;
    ANSI_STRING VdmAnsiEnv;
    UNICODE_STRING VdmString, VdmUnicodeEnv;
    BOOLEAN IsWowApp;
    PBASE_CHECK_VDM CheckVdmMsg;

    /* Zero out the initial core variables and handles */
    QuerySection = FALSE;
    InJob = FALSE;
    SkipSaferAndAppCompat = FALSE;
    ParameterFlags = 0;
    Flags = 0;
    DebugHandle = NULL;
    JobHandle = NULL;
    TokenHandle = NULL;
    FileHandle = NULL;
    SectionHandle = NULL;
    ProcessHandle = NULL;
    ThreadHandle = NULL;
    BaseAddress = (PVOID)1;

    /* Zero out initial SxS and Application Compatibility state */
    AppCompatData = NULL;
    AppCompatDataSize = 0;
    AppCompatSxsData = NULL;
    AppCompatSxsDataSize = 0;
    CaptureBuffer = NULL;
#if _SXS_SUPPORT_ENABLED_
    SxsConglomeratedBuffer = NULL;
#endif
    FusionFlags = 0;

    /* Zero out initial parsing variables -- others are initialized later */
    DebuggerCmdLine = NULL;
    PathBuffer = NULL;
    SearchPath = NULL;
    NullBuffer = NULL;
    FreeBuffer = NULL;
    NameBuffer = NULL;
    CurrentDirectory = NULL;
    FilePart = NULL;
    DebuggerString.Buffer = NULL;
    HasQuotes = FALSE;
    QuotedCmdLine = NULL;

    /* Zero out initial VDM state */
    VdmAnsiEnv.Buffer = NULL;
    VdmUnicodeEnv.Buffer = NULL;
    VdmString.Buffer = NULL;
    VdmTask = 0;
    VdmUndoLevel = 0;
    VdmBinaryType = 0;
    VdmReserve = 0;
    VdmWaitObject = NULL;
    UseVdmReserve = FALSE;
    IsWowApp = FALSE;

    /* Set message structures */
    CreateProcessMsg = &CsrMsg[0].Data.CreateProcessRequest;
    CheckVdmMsg = &CsrMsg[1].Data.CheckVDMRequest;

    /* Clear the more complex structures by zeroing out their entire memory */
    RtlZeroMemory(&Context, sizeof(Context));
#if _SXS_SUPPORT_ENABLED_
    RtlZeroMemory(&FileHandles, sizeof(FileHandles));
    RtlZeroMemory(&MappedHandles, sizeof(MappedHandles));
    RtlZeroMemory(&Handles, sizeof(Handles));
#endif
    RtlZeroMemory(&CreateProcessMsg->Sxs, sizeof(CreateProcessMsg->Sxs));
    RtlZeroMemory(&LocalProcessAttributes, sizeof(LocalProcessAttributes));
    RtlZeroMemory(&LocalThreadAttributes, sizeof(LocalThreadAttributes));

    /* Zero out output arguments as well */
    RtlZeroMemory(lpProcessInformation, sizeof(*lpProcessInformation));
    if (hNewToken) *hNewToken = NULL;

    /* Capture the special window flag */
    NoWindow = dwCreationFlags & CREATE_NO_WINDOW;
    dwCreationFlags &= ~CREATE_NO_WINDOW;

#if _SXS_SUPPORT_ENABLED_
    /* Setup the SxS static string arrays and buffers */
    SxsStaticBuffers[0] = &SxsWin32ManifestPath;
    SxsStaticBuffers[1] = &SxsWin32PolicyPath;
    SxsStaticBuffers[2] = &SxsWin32AssemblyDirectory;
    SxsStaticBuffers[3] = &SxsNtManifestPath;
    SxsStaticBuffers[4] = &SxsNtPolicyPath;
    ExePathPair.Win32 = &SxsWin32ExePath;
    ExePathPair.Nt = &SxsNtExePath;
    ManifestPathPair.Win32 = &SxsWin32ManifestPath.String;
    ManifestPathPair.Nt = &SxsNtManifestPath.String;
    PolicyPathPair.Win32 = &SxsWin32PolicyPath.String;
    PolicyPathPair.Nt = &SxsNtPolicyPath.String;
#endif

    DPRINT("CreateProcessInternalW: '%S' '%S' %lx\n", lpApplicationName, lpCommandLine, dwCreationFlags);

    /* Finally, set our TEB and PEB */
    Teb = NtCurrentTeb();
    Peb = NtCurrentPeb();

    /* This combination is illegal (see MSDN) */
    if ((dwCreationFlags & (DETACHED_PROCESS | CREATE_NEW_CONSOLE)) ==
        (DETACHED_PROCESS | CREATE_NEW_CONSOLE))
    {
        DPRINT1("Invalid flag combo used\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Convert the priority class */
    if (dwCreationFlags & IDLE_PRIORITY_CLASS)
    {
        PriorityClass.PriorityClass = PROCESS_PRIORITY_CLASS_IDLE;
    }
    else if (dwCreationFlags & BELOW_NORMAL_PRIORITY_CLASS)
    {
        PriorityClass.PriorityClass = PROCESS_PRIORITY_CLASS_BELOW_NORMAL;
    }
    else if (dwCreationFlags & NORMAL_PRIORITY_CLASS)
    {
        PriorityClass.PriorityClass = PROCESS_PRIORITY_CLASS_NORMAL;
    }
    else if (dwCreationFlags & ABOVE_NORMAL_PRIORITY_CLASS)
    {
        PriorityClass.PriorityClass = PROCESS_PRIORITY_CLASS_ABOVE_NORMAL;
    }
    else if (dwCreationFlags & HIGH_PRIORITY_CLASS)
    {
        PriorityClass.PriorityClass = PROCESS_PRIORITY_CLASS_HIGH;
    }
    else if (dwCreationFlags & REALTIME_PRIORITY_CLASS)
    {
        PriorityClass.PriorityClass = PROCESS_PRIORITY_CLASS_HIGH;
        PriorityClass.PriorityClass += (BasepIsRealtimeAllowed(FALSE) != NULL);
    }
    else
    {
        PriorityClass.PriorityClass = PROCESS_PRIORITY_CLASS_INVALID;
    }

    /* Done with the priority masks, so get rid of them */
    PriorityClass.Foreground = FALSE;
    dwCreationFlags &= ~(NORMAL_PRIORITY_CLASS |
                         IDLE_PRIORITY_CLASS |
                         HIGH_PRIORITY_CLASS |
                         REALTIME_PRIORITY_CLASS |
                         BELOW_NORMAL_PRIORITY_CLASS |
                         ABOVE_NORMAL_PRIORITY_CLASS);

    /* You cannot request both a shared and a separate WoW VDM */
    if ((dwCreationFlags & CREATE_SEPARATE_WOW_VDM) &&
        (dwCreationFlags & CREATE_SHARED_WOW_VDM))
    {
        /* Fail such nonsensical attempts */
        DPRINT1("Invalid WOW flags\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    else if (!(dwCreationFlags & CREATE_SHARED_WOW_VDM) &&
             (BaseStaticServerData->DefaultSeparateVDM))
    {
        /* A shared WoW VDM was not requested but system enforces separation */
        dwCreationFlags |= CREATE_SEPARATE_WOW_VDM;
    }

    /* If a shared WoW VDM is used, make sure the process isn't in a job */
    if (!(dwCreationFlags & CREATE_SEPARATE_WOW_VDM) &&
        (NtIsProcessInJob(NtCurrentProcess(), NULL)))
    {
        /* Remove the shared flag and add the separate flag */
        dwCreationFlags = (dwCreationFlags &~ CREATE_SHARED_WOW_VDM) |
                                              CREATE_SEPARATE_WOW_VDM;
    }

    /* Convert the environment */
    if ((lpEnvironment) && !(dwCreationFlags & CREATE_UNICODE_ENVIRONMENT))
    {
        /* Scan the environment to calculate its Unicode size */
        AnsiEnv.Buffer = pcScan = (PCHAR)lpEnvironment;
        while ((*pcScan) || (*(pcScan + 1))) ++pcScan;

        /* Make sure the environment is not too large */
        EnvironmentLength = (pcScan + sizeof(ANSI_NULL) - (PCHAR)lpEnvironment);
        if (EnvironmentLength > MAXUSHORT)
        {
            /* Fail */
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        /* Create our ANSI String */
        AnsiEnv.Length = (USHORT)EnvironmentLength;
        AnsiEnv.MaximumLength = AnsiEnv.Length + sizeof(ANSI_NULL);

        /* Allocate memory for the Unicode Environment */
        UnicodeEnv.Buffer = NULL;
        RegionSize = AnsiEnv.MaximumLength * sizeof(WCHAR);
        Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                         (PVOID)&UnicodeEnv.Buffer,
                                         0,
                                         &RegionSize,
                                         MEM_COMMIT,
                                         PAGE_READWRITE);
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            BaseSetLastNTError(Status);
            return FALSE;
        }

        /* Use the allocated size and convert */
        UnicodeEnv.MaximumLength = (USHORT)RegionSize;
        Status = RtlAnsiStringToUnicodeString(&UnicodeEnv, &AnsiEnv, FALSE);
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            NtFreeVirtualMemory(NtCurrentProcess(),
                                (PVOID)&UnicodeEnv.Buffer,
                                &RegionSize,
                                MEM_RELEASE);
            BaseSetLastNTError(Status);
            return FALSE;
        }

        /* Now set the Unicode environment as the environment string pointer */
        lpEnvironment = UnicodeEnv.Buffer;
    }

    /* Make a copy of the caller's startup info since we'll modify it */
    StartupInfo = *lpStartupInfo;

    /* Check if private data is being sent on the same channel as std handles */
    if ((StartupInfo.dwFlags & STARTF_USESTDHANDLES) &&
        (StartupInfo.dwFlags & (STARTF_USEHOTKEY | STARTF_SHELLPRIVATE)))
    {
        /* Cannot use the std handles since we have monitor/hotkey values */
        StartupInfo.dwFlags &= ~STARTF_USESTDHANDLES;
    }

    /* If there's a debugger, or we have to launch cmd.exe, we go back here */
AppNameRetry:
    /* New iteration -- free any existing name buffer */
    if (NameBuffer)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, NameBuffer);
        NameBuffer = NULL;
    }

    /* New iteration -- free any existing free buffer */
    if (FreeBuffer)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, FreeBuffer);
        FreeBuffer = NULL;
    }

    /* New iteration -- close any existing file handle */
    if (FileHandle)
    {
        NtClose(FileHandle);
        FileHandle = NULL;
    }

    /* Set the initial parsing state. This code can loop -- don't move this! */
    ErrorCode = 0;
    SearchRetry = TRUE;
    QuotesNeeded = FALSE;
    CmdLineIsAppName = FALSE;

    /* First check if we don't have an application name */
    if (!lpApplicationName)
    {
        /* This should be the first time we attempt creating one */
        ASSERT(NameBuffer == NULL);

        /* Allocate a buffer to hold it */
        NameBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                     0,
                                     MAX_PATH * sizeof(WCHAR));
        if (!NameBuffer)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            Result = FALSE;
            goto Quickie;
        }

        /* Initialize the application name and our parsing parameters */
        lpApplicationName = NullBuffer = ScanString = lpCommandLine;

        /* Check for an initial quote*/
        if (*lpCommandLine == L'\"')
        {
            /* We found a quote, keep searching for another one */
            SearchRetry = FALSE;
            ScanString++;
            lpApplicationName = ScanString;
            while (*ScanString)
            {
                /* Have we found the terminating quote? */
                if (*ScanString == L'\"')
                {
                    /* We're done, get out of here */
                    NullBuffer = ScanString;
                    HasQuotes = TRUE;
                    break;
                }

                /* Keep searching for the quote */
                ScanString++;
                NullBuffer = ScanString;
            }
        }
        else
        {
StartScan:
            /* We simply make the application name be the command line*/
            lpApplicationName = lpCommandLine;
            while (*ScanString)
            {
                /* Check if it starts with a space or tab */
                if ((*ScanString == L' ') || (*ScanString == L'\t'))
                {
                    /* Break out of the search loop */
                    NullBuffer = ScanString;
                    break;
                }

                /* Keep searching for a space or tab */
                ScanString++;
                NullBuffer = ScanString;
            }
        }

        /* We have found the end of the application name, terminate it */
        SaveChar = *NullBuffer;
        *NullBuffer = UNICODE_NULL;

        /* New iteration -- free any existing saved path */
        if (SearchPath)
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, SearchPath);
            SearchPath = NULL;
        }

        /* Now compute the final EXE path based on the name */
        SearchPath = BaseComputeProcessExePath((LPWSTR)lpApplicationName);
        DPRINT("Search Path: %S\n", SearchPath);
        if (!SearchPath)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            Result = FALSE;
            goto Quickie;
        }

        /* And search for the executable in the search path */
        Length = SearchPathW(SearchPath,
                             lpApplicationName,
                             L".exe",
                             MAX_PATH,
                             NameBuffer,
                             NULL);

        /* Did we find it? */
        if ((Length) && (Length < MAX_PATH))
        {
            /* Get file attributes */
            FileAttribs = GetFileAttributesW(NameBuffer);
            if ((FileAttribs != INVALID_FILE_ATTRIBUTES) &&
                (FileAttribs & FILE_ATTRIBUTE_DIRECTORY))
            {
                /* This was a directory, fail later on */
                Length = 0;
            }
            else
            {
                /* It's a file! */
                Length++;
            }
        }

        DPRINT("Length: %lu Buffer: %S\n", Length, NameBuffer);

        /* Check if there was a failure in SearchPathW */
        if ((Length) && (Length < MAX_PATH))
        {
            /* Everything looks good, restore the name */
            *NullBuffer = SaveChar;
            lpApplicationName = NameBuffer;
        }
        else
        {
            /* Check if this was a relative path, which would explain it */
            PathType = RtlDetermineDosPathNameType_U(lpApplicationName);
            if (PathType != RtlPathTypeRelative)
            {
                /* This should fail, and give us a detailed LastError */
                FileHandle = CreateFileW(lpApplicationName,
                                         GENERIC_READ,
                                         FILE_SHARE_READ |
                                         FILE_SHARE_WRITE,
                                         NULL,
                                         OPEN_EXISTING,
                                         FILE_ATTRIBUTE_NORMAL,
                                         NULL);
                if (FileHandle != INVALID_HANDLE_VALUE)
                {
                    /* It worked? Return a generic error */
                    CloseHandle(FileHandle);
                    FileHandle = NULL;
                    BaseSetLastNTError(STATUS_OBJECT_NAME_NOT_FOUND);
                }
            }
            else
            {
                /* Path was absolute, which means it doesn't exist */
                BaseSetLastNTError(STATUS_OBJECT_NAME_NOT_FOUND);
            }

            /* Did we already fail once? */
            if (ErrorCode)
            {
                /* Set the error code */
                SetLastError(ErrorCode);
            }
            else
            {
                /* Not yet, cache it */
                ErrorCode = GetLastError();
            }

            /* Put back the command line */
            *NullBuffer = SaveChar;
            lpApplicationName = NameBuffer;

            /* It's possible there's whitespace in the directory name */
            if (!(*ScanString) || !(SearchRetry))
            {
                /* Not the case, give up completely */
                Result = FALSE;
                goto Quickie;
            }

            /* There are spaces, so keep trying the next possibility */
            ScanString++;
            NullBuffer = ScanString;

            /* We will have to add a quote, since there is a space */
            QuotesNeeded = TRUE;
            HasQuotes = TRUE;
            goto StartScan;
        }
    }
    else if (!(lpCommandLine) || !(*lpCommandLine))
    {
        /* We don't have a command line, so just use the application name */
        CmdLineIsAppName = TRUE;
        lpCommandLine = (LPWSTR)lpApplicationName;
    }

    /* Convert the application name to its NT path */
    TranslationStatus = RtlDosPathNameToRelativeNtPathName_U(lpApplicationName,
                                                             &PathName,
                                                             NULL,
                                                             &SxsWin32RelativePath);
    if (!TranslationStatus)
    {
        /* Path must be invalid somehow, bail out */
        DPRINT1("Path translation for SxS failed\n");
        SetLastError(ERROR_PATH_NOT_FOUND);
        Result = FALSE;
        goto Quickie;
    }

    /* Setup the buffer that needs to be freed at the end */
    ASSERT(FreeBuffer == NULL);
    FreeBuffer = PathName.Buffer;

    /* Check what kind of path the application is, for SxS (Fusion) purposes */
    RtlInitUnicodeString(&SxsWin32ExePath, lpApplicationName);
    SxsPathType = RtlDetermineDosPathNameType_U(lpApplicationName);
    if ((SxsPathType != RtlPathTypeDriveAbsolute) &&
        (SxsPathType != RtlPathTypeLocalDevice) &&
        (SxsPathType != RtlPathTypeRootLocalDevice) &&
        (SxsPathType != RtlPathTypeUncAbsolute))
    {
        /* Relative-type path, get the full path */
        RtlInitEmptyUnicodeString(&PathBufferString, NULL, 0);
        Status = RtlGetFullPathName_UstrEx(&SxsWin32ExePath,
                                           NULL,
                                           &PathBufferString,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &SxsPathType,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            /* Fail the rest of the create */
            RtlReleaseRelativeName(&SxsWin32RelativePath);
            BaseSetLastNTError(Status);
            Result = FALSE;
            goto Quickie;
        }

        /* Use this full path as the SxS path */
        SxsWin32ExePath = PathBufferString;
        PathBuffer = PathBufferString.Buffer;
        PathBufferString.Buffer = NULL;
        DPRINT("SxS Path: %S\n", PathBuffer);
    }

    /* Also set the .EXE path based on the path name */
#if _SXS_SUPPORT_ENABLED_
    SxsNtExePath = PathName;
#endif
    if (SxsWin32RelativePath.RelativeName.Length)
    {
        /* If it's relative, capture the relative name */
        PathName = SxsWin32RelativePath.RelativeName;
    }
    else
    {
        /* Otherwise, it's absolute, make sure no relative dir is used */
        SxsWin32RelativePath.ContainingDirectory = NULL;
    }

    /* Now use the path name, and the root path, to try opening the app */
    DPRINT("Path: %wZ. Dir: %p\n", &PathName, SxsWin32RelativePath.ContainingDirectory);
    InitializeObjectAttributes(&LocalObjectAttributes,
                               &PathName,
                               OBJ_CASE_INSENSITIVE,
                               SxsWin32RelativePath.ContainingDirectory,
                               NULL);
    Status = NtOpenFile(&FileHandle,
                        SYNCHRONIZE |
                        FILE_READ_ATTRIBUTES |
                        FILE_READ_DATA |
                        FILE_EXECUTE,
                        &LocalObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_DELETE,
                        FILE_SYNCHRONOUS_IO_NONALERT |
                        FILE_NON_DIRECTORY_FILE);
    if (!NT_SUCCESS(Status))
    {
        /* Try to open the app just for execute purposes instead */
        Status = NtOpenFile(&FileHandle,
                            SYNCHRONIZE | FILE_EXECUTE,
                            &LocalObjectAttributes,
                            &IoStatusBlock,
                            FILE_SHARE_READ | FILE_SHARE_DELETE,
                            FILE_SYNCHRONOUS_IO_NONALERT |
                            FILE_NON_DIRECTORY_FILE);
    }

    /* Failure path, display which file failed to open */
    if (!NT_SUCCESS(Status))
        DPRINT1("Open file failed: %lx (%wZ)\n", Status, &PathName);

    /* Cleanup in preparation for failure or success */
    RtlReleaseRelativeName(&SxsWin32RelativePath);

    if (!NT_SUCCESS(Status))
    {
        /* Failure path, try to understand why */
        if (RtlIsDosDeviceName_U(lpApplicationName))
        {
            /* If a device is being executed, return this special error code */
            SetLastError(ERROR_BAD_DEVICE);
            Result = FALSE;
            goto Quickie;
        }
        else
        {
            /* Otherwise return the converted NT error code */
            BaseSetLastNTError(Status);
            Result = FALSE;
            goto Quickie;
        }
    }

    /* Did the caller specify a desktop? */
    if (!StartupInfo.lpDesktop)
    {
        /* Use the one from the current process */
        StartupInfo.lpDesktop = Peb->ProcessParameters->DesktopInfo.Buffer;
    }

    /* Create a section for this file */
    Status = NtCreateSection(&SectionHandle,
                             SECTION_ALL_ACCESS,
                             NULL,
                             NULL,
                             PAGE_EXECUTE,
                             SEC_IMAGE,
                             FileHandle);
    DPRINT("Section status: %lx\n", Status);
    if (NT_SUCCESS(Status))
    {
        /* Are we running on Windows Embedded, Datacenter, Blade or Starter? */
        if (SharedUserData->SuiteMask & (VER_SUITE_EMBEDDEDNT |
                                         VER_SUITE_DATACENTER |
                                         VER_SUITE_PERSONAL |
                                         VER_SUITE_BLADE))
        {
            /* These SKUs do not allow running certain applications */
            Status = BasepCheckWebBladeHashes(FileHandle);
            if (Status == STATUS_ACCESS_DENIED)
            {
                /* And this is one of them! */
                DPRINT1("Invalid Blade hashes!\n");
                SetLastError(ERROR_ACCESS_DISABLED_WEBBLADE);
                Result = FALSE;
                goto Quickie;
            }

            /* Did we get some other failure? */
            if (!NT_SUCCESS(Status))
            {
                /* If we couldn't check the hashes, assume nefariousness */
                DPRINT1("Tampered Blade hashes!\n");
                SetLastError(ERROR_ACCESS_DISABLED_WEBBLADE_TAMPER);
                Result = FALSE;
                goto Quickie;
            }
        }

        /* Now do Winsafer, etc, checks */
        Status = BasepIsProcessAllowed((LPWSTR)lpApplicationName);
        if (!NT_SUCCESS(Status))
        {
            /* Fail if we're not allowed to launch the process */
            DPRINT1("Process not allowed to launch: %lx\n", Status);
            BaseSetLastNTError(Status);
            if (SectionHandle)
            {
                NtClose(SectionHandle);
                SectionHandle = NULL;
            }
            Result = FALSE;
            goto Quickie;
        }

        /* Is a DOS VDM being forced, but we already have a WOW32 instance ready? */
        if ((dwCreationFlags & CREATE_FORCEDOS) &&
            (BaseStaticServerData->IsWowTaskReady))
        {
            /* This request can't be satisfied, instead, a separate VDM is needed */
            dwCreationFlags &= ~(CREATE_FORCEDOS | CREATE_SHARED_WOW_VDM);
            dwCreationFlags |= CREATE_SEPARATE_WOW_VDM;

            /* Set a failure code, ask for VDM reservation */
            Status = STATUS_INVALID_IMAGE_WIN_16;
            UseVdmReserve = TRUE;

            /* Close the current handle */
            NtClose(SectionHandle);
            SectionHandle = NULL;

            /* Don't query the section later */
            QuerySection = FALSE;
        }
    }

    /* Did we already do these checks? */
    if (!SkipSaferAndAppCompat)
    {
        /* Is everything OK so far, OR do we have an non-MZ, non-DOS app? */
        if ((NT_SUCCESS(Status)) ||
            ((Status == STATUS_INVALID_IMAGE_NOT_MZ) &&
            !(BaseIsDosApplication(&PathName, Status))))
        {
            /* Clear the machine type in case of failure */
            ImageMachine = 0;

            /* Clean any app compat data that may have accumulated */
            BasepFreeAppCompatData(AppCompatData, AppCompatSxsData);
            AppCompatData = NULL;
            AppCompatSxsData = NULL;

            /* Do we have a section? */
            if (SectionHandle)
            {
                /* Have we already queried it? */
                if (QuerySection)
                {
                    /* Nothing to do */
                    AppCompatStatus = STATUS_SUCCESS;
                }
                else
                {
                    /* Get some information about the executable */
                    AppCompatStatus = NtQuerySection(SectionHandle,
                                            SectionImageInformation,
                                            &ImageInformation,
                                            sizeof(ImageInformation),
                                            NULL);
                }

                /* Do we have section information now? */
                if (NT_SUCCESS(AppCompatStatus))
                {
                    /* Don't ask for it again, save the machine type */
                    QuerySection = TRUE;
                    ImageMachine = ImageInformation.Machine;
                }
            }

            /* Is there a reason/Shim we shouldn't run this application? */
            AppCompatStatus = BasepCheckBadapp(FileHandle,
                                      FreeBuffer,
                                      lpEnvironment,
                                      ImageMachine,
                                      &AppCompatData,
                                      &AppCompatDataSize,
                                      &AppCompatSxsData,
                                      &AppCompatSxsDataSize,
                                      &FusionFlags);
            if (!NT_SUCCESS(AppCompatStatus))
            {
                /* This is usually the status we get back */
                DPRINT1("App compat launch failure: %lx\n", AppCompatStatus);
                if (AppCompatStatus == STATUS_ACCESS_DENIED)
                {
                    /* Convert it to something more Win32-specific */
                    SetLastError(ERROR_CANCELLED);
                }
                else
                {
                    /* Some other error */
                    BaseSetLastNTError(AppCompatStatus);
                }

                /* Did we have a section? */
                if (SectionHandle)
                {
                    /* Clean it up */
                    NtClose(SectionHandle);
                    SectionHandle = NULL;
                }

                /* Fail the call */
                Result = FALSE;
                goto Quickie;
            }
        }
    }

    //ASSERT((dwFusionFlags & ~SXS_APPCOMPACT_FLAG_APP_RUNNING_SAFEMODE) == 0);

    /* Have we already done, and do we need to do, SRP (WinSafer) checks? */
    if (!(SkipSaferAndAppCompat) &&
        ~(dwCreationFlags & CREATE_PRESERVE_CODE_AUTHZ_LEVEL))
    {
        /* Assume yes */
        SaferNeeded = TRUE;
        switch (Status)
        {
            case STATUS_INVALID_IMAGE_NE_FORMAT:
            case STATUS_INVALID_IMAGE_PROTECT:
            case STATUS_INVALID_IMAGE_WIN_16:
            case STATUS_FILE_IS_OFFLINE:
                /* For all DOS, 16-bit, OS/2 images, we do*/
                break;

            case STATUS_INVALID_IMAGE_NOT_MZ:
                /* For invalid files, we don't, unless it's a .BAT file */
                if (BaseIsDosApplication(&PathName, Status)) break;

            default:
                /* Any other error codes we also don't */
                if (!NT_SUCCESS(Status))
                {
                    SaferNeeded = FALSE;
                }

                /* But for success, we do */
                break;
        }

        /* Okay, so what did the checks above result in? */
        if (SaferNeeded)
        {
            /* We have to call into the WinSafer library and actually check */
            SaferStatus = BasepCheckWinSaferRestrictions(hUserToken,
                                                    (LPWSTR)lpApplicationName,
                                                    FileHandle,
                                                    &InJob,
                                                    &TokenHandle,
                                                    &JobHandle);
            if (SaferStatus == 0xFFFFFFFF)
            {
                /* Back in 2003, they didn't have an NTSTATUS for this... */
                DPRINT1("WinSafer blocking process launch\n");
                SetLastError(ERROR_ACCESS_DISABLED_BY_POLICY);
                Result = FALSE;
                goto Quickie;
            }

            /* Other status codes are not-Safer related, just convert them */
            if (!NT_SUCCESS(SaferStatus))
            {
                DPRINT1("Error checking WinSafer: %lx\n", SaferStatus);
                BaseSetLastNTError(SaferStatus);
                Result = FALSE;
                goto Quickie;
            }
        }
    }

    /* The last step is to figure out why the section object was not created */
    switch (Status)
    {
        case STATUS_INVALID_IMAGE_WIN_16:
        {
            /* 16-bit binary. Should we use WOW or does the caller force VDM? */
            if (!(dwCreationFlags & CREATE_FORCEDOS))
            {
                /* Remember that we're launching WOW */
                IsWowApp = TRUE;

                /* Create the VDM environment, it's valid for WOW too */
                Result = BaseCreateVDMEnvironment(lpEnvironment,
                                                  &VdmAnsiEnv,
                                                  &VdmUnicodeEnv);
                if (!Result)
                {
                    DPRINT1("VDM environment for WOW app failed\n");
                    goto Quickie;
                }

                /* We're going to try this twice, so do a loop */
                while (TRUE)
                {
                    /* Pick which kind of WOW mode we want to run in */
                    VdmBinaryType = (dwCreationFlags &
                                     CREATE_SEPARATE_WOW_VDM) ?
                                     BINARY_TYPE_SEPARATE_WOW : BINARY_TYPE_WOW;

                    /* Get all the VDM settings and current status */
                    Status = BaseCheckVDM(VdmBinaryType,
                                          lpApplicationName,
                                          lpCommandLine,
                                          lpCurrentDirectory,
                                          &VdmAnsiEnv,
                                          &CsrMsg[1],
                                          &VdmTask,
                                          dwCreationFlags,
                                          &StartupInfo,
                                          hUserToken);

                    /* If it worked, no need to try again */
                    if (NT_SUCCESS(Status)) break;

                    /* Check if it's disallowed or if it's our second time */
                    BaseSetLastNTError(Status);
                    if ((Status == STATUS_VDM_DISALLOWED) ||
                        (VdmBinaryType == BINARY_TYPE_SEPARATE_WOW) ||
                        (GetLastError() == ERROR_ACCESS_DENIED))
                    {
                        /* Fail the call -- we won't try again */
                        DPRINT1("VDM message failure for WOW: %lx\n", Status);
                        Result = FALSE;
                        goto Quickie;
                    }

                    /* Try one more time, but with a separate WOW instance */
                    dwCreationFlags |= CREATE_SEPARATE_WOW_VDM;
                }

                /* Check which VDM state we're currently in */
                switch (CheckVdmMsg->VDMState & (VDM_NOT_LOADED |
                                                 VDM_NOT_READY |
                                                 VDM_READY))
                {
                    case VDM_NOT_LOADED:
                        /* VDM is not fully loaded, so not that much to undo */
                        VdmUndoLevel = VDM_UNDO_PARTIAL;

                        /* Reset VDM reserve if needed */
                        if (UseVdmReserve) VdmReserve = 1;

                        /* Get the required parameters and names for launch */
                        Result = BaseGetVdmConfigInfo(lpCommandLine,
                                                      VdmTask,
                                                      VdmBinaryType,
                                                      &VdmString,
                                                      &VdmReserve);
                        if (!Result)
                        {
                            DPRINT1("VDM Configuration failed for WOW\n");
                            BaseSetLastNTError(Status);
                            goto Quickie;
                        }

                        /* Update the command-line with the VDM one instead */
                        lpCommandLine = VdmString.Buffer;
                        lpApplicationName = NULL;

                        /* We don't want a console, detachment, nor a window */
                        dwCreationFlags |= CREATE_NO_WINDOW;
                        dwCreationFlags &= ~(CREATE_NEW_CONSOLE | DETACHED_PROCESS);

                        /* Force feedback on */
                        StartupInfo.dwFlags |= STARTF_FORCEONFEEDBACK;
                        break;


                    case VDM_READY:
                        /* VDM is ready, so we have to undo everything */
                        VdmUndoLevel = VDM_UNDO_REUSE;

                        /* Check if CSRSS wants us to wait on VDM */
                        VdmWaitObject = CheckVdmMsg->WaitObjectForParent;
                        break;

                    case VDM_NOT_READY:
                        /* Something is wrong with VDM, we'll fail the call */
                        DPRINT1("VDM is not ready for WOW\n");
                        SetLastError(ERROR_NOT_READY);
                        Result = FALSE;
                        goto Quickie;

                    default:
                        break;
                }

                /* Since to get NULL, we allocate from 0x1, account for this */
                VdmReserve--;

                /* This implies VDM is ready, so skip everything else */
                if (VdmWaitObject) goto VdmShortCircuit;

                /* Don't inherit handles since we're doing VDM now */
                bInheritHandles = FALSE;

                /* Had the user passed in environment? If so, destroy it */
                if ((lpEnvironment) &&
                    !(dwCreationFlags & CREATE_UNICODE_ENVIRONMENT))
                {
                    RtlDestroyEnvironment(lpEnvironment);
                }

                /* We've already done all these checks, don't do them again */
                SkipSaferAndAppCompat = TRUE;
                goto AppNameRetry;
            }

            // There is no break here on purpose, so FORCEDOS drops down!
        }

        case STATUS_INVALID_IMAGE_PROTECT:
        case STATUS_INVALID_IMAGE_NOT_MZ:
        case STATUS_INVALID_IMAGE_NE_FORMAT:
        {
            /* We're launching an executable application */
            BinarySubType = BINARY_TYPE_EXE;

            /* We can drop here from other "cases" above too, so check */
            if ((Status == STATUS_INVALID_IMAGE_PROTECT) ||
                (Status == STATUS_INVALID_IMAGE_NE_FORMAT) ||
                (BinarySubType = BaseIsDosApplication(&PathName, Status)))
            {
                /* We're launching a DOS application */
                VdmBinaryType = BINARY_TYPE_DOS;

                /* Based on the caller environment, create a VDM one */
                Result = BaseCreateVDMEnvironment(lpEnvironment,
                                                  &VdmAnsiEnv,
                                                  &VdmUnicodeEnv);
                if (!Result)
                {
                    DPRINT1("VDM environment for DOS failed\n");
                    goto Quickie;
                }

                /* Check the current state of the VDM subsystem */
                Status = BaseCheckVDM(VdmBinaryType | BinarySubType,
                                      lpApplicationName,
                                      lpCommandLine,
                                      lpCurrentDirectory,
                                      &VdmAnsiEnv,
                                      &CsrMsg[1],
                                      &VdmTask,
                                      dwCreationFlags,
                                      &StartupInfo,
                                      NULL);
                if (!NT_SUCCESS(Status))
                {
                    /* Failed to inquire about VDM, fail the call */
                    DPRINT1("VDM message failure for DOS: %lx\n", Status);
                    BaseSetLastNTError(Status);
                    Result = FALSE;
                    goto Quickie;
                };

                /* Handle possible VDM states */
                switch (CheckVdmMsg->VDMState & (VDM_NOT_LOADED |
                                                 VDM_NOT_READY |
                                                 VDM_READY))
                {
                    case VDM_NOT_LOADED:
                        /* If VDM is not loaded, we'll do a partial undo */
                        VdmUndoLevel = VDM_UNDO_PARTIAL;

                        /* A VDM process can't also be detached, so fail */
                        if (dwCreationFlags & DETACHED_PROCESS)
                        {
                            DPRINT1("Detached process but no VDM, not allowed\n");
                            SetLastError(ERROR_ACCESS_DENIED);
                            return FALSE;
                        }

                        /* Get the required parameters and names for launch */
                        Result = BaseGetVdmConfigInfo(lpCommandLine,
                                                      VdmTask,
                                                      VdmBinaryType,
                                                      &VdmString,
                                                      &VdmReserve);
                        if (!Result)
                        {
                            DPRINT1("VDM Configuration failed for DOS\n");
                            BaseSetLastNTError(Status);
                            goto Quickie;
                        }

                        /* Update the command-line to launch VDM instead */
                        lpCommandLine = VdmString.Buffer;
                        lpApplicationName = NULL;
                        break;

                    case VDM_READY:
                        /* VDM is ready, so we have to undo everything */
                        VdmUndoLevel = VDM_UNDO_REUSE;

                        /* Check if CSRSS wants us to wait on VDM */
                        VdmWaitObject = CheckVdmMsg->WaitObjectForParent;
                        break;

                    case VDM_NOT_READY:
                        /* Something is wrong with VDM, we'll fail the call */
                        DPRINT1("VDM is not ready for DOS\n");
                        SetLastError(ERROR_NOT_READY);
                        Result = FALSE;
                        goto Quickie;

                    default:
                        break;
                }

                /* Since to get NULL, we allocate from 0x1, account for this */
                VdmReserve--;

                /* This implies VDM is ready, so skip everything else */
                if (VdmWaitObject) goto VdmShortCircuit;

                /* Don't inherit handles since we're doing VDM now */
                bInheritHandles = FALSE;

                /* Had the user passed in environment? If so, destroy it */
                if ((lpEnvironment) &&
                    !(dwCreationFlags & CREATE_UNICODE_ENVIRONMENT))
                {
                    RtlDestroyEnvironment(lpEnvironment);
                }

                /* Use our VDM Unicode environment instead */
                lpEnvironment = VdmUnicodeEnv.Buffer;
            }
            else
            {
                /* It's a batch file, get the extension */
                ExtBuffer = &PathName.Buffer[PathName.Length / sizeof(WCHAR) - 4];

                /* Make sure the extensions are correct */
                if ((PathName.Length < (4 * sizeof(WCHAR))) ||
                    ((_wcsnicmp(ExtBuffer, L".bat", 4)) &&
                     (_wcsnicmp(ExtBuffer, L".cmd", 4))))
                {
                    DPRINT1("'%wZ': Invalid EXE, and not a batch or script file\n", &PathName);
                    SetLastError(ERROR_BAD_EXE_FORMAT);
                    Result = FALSE;
                    goto Quickie;
                }

                /* Check if we need to account for quotes around the path */
                CmdQuoteLength = CmdLineIsAppName || HasQuotes;
                if (!CmdLineIsAppName)
                {
                    if (HasQuotes) CmdQuoteLength++;
                }
                else
                {
                    CmdQuoteLength++;
                }

                /* Calculate the length of the command line */
                CmdLineLength = wcslen(lpCommandLine);
                CmdLineLength += wcslen(CMD_STRING);
                CmdLineLength += CmdQuoteLength + sizeof(ANSI_NULL);
                CmdLineLength *= sizeof(WCHAR);

                /* Allocate space for the new command line */
                AnsiCmdCommand = RtlAllocateHeap(RtlGetProcessHeap(),
                                                 0,
                                                 CmdLineLength);
                if (!AnsiCmdCommand)
                {
                    BaseSetLastNTError(STATUS_NO_MEMORY);
                    Result = FALSE;
                    goto Quickie;
                }

                /* Build it */
                wcscpy(AnsiCmdCommand, CMD_STRING);
                if ((CmdLineIsAppName) || (HasQuotes))
                {
                    wcscat(AnsiCmdCommand, L"\"");
                }
                wcscat(AnsiCmdCommand, lpCommandLine);
                if ((CmdLineIsAppName) || (HasQuotes))
                {
                    wcscat(AnsiCmdCommand, L"\"");
                }

                /* Create it as a Unicode String */
                RtlInitUnicodeString(&DebuggerString, AnsiCmdCommand);

                /* Set the command line to this */
                lpCommandLine = DebuggerString.Buffer;
                lpApplicationName = NULL;
                DPRINT1("Retrying with: %S\n", lpCommandLine);
            }

            /* We've already done all these checks, don't do them again */
            SkipSaferAndAppCompat = TRUE;
            goto AppNameRetry;
        }

        case STATUS_INVALID_IMAGE_WIN_64:
        {
            /* 64-bit binaries are not allowed to run on 32-bit ReactOS */
            DPRINT1("64-bit binary, failing\n");
            SetLastError(ERROR_EXE_MACHINE_TYPE_MISMATCH);
            Result = FALSE;
            goto Quickie;
        }

        case STATUS_FILE_IS_OFFLINE:
        {
            /* Set the correct last error for this */
            DPRINT1("File is offline, failing\n");
            SetLastError(ERROR_FILE_OFFLINE);
            break;
        }

        default:
        {
            /* Any other error, convert it to a generic Win32 error */
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Failed to create section: %lx\n", Status);
                SetLastError(ERROR_BAD_EXE_FORMAT);
                Result = FALSE;
                goto Quickie;
            }

            /* Otherwise, this must be success */
            ASSERT(Status == STATUS_SUCCESS);
            break;
        }
    }

    /* Is this not a WOW application, but a WOW32 VDM was requested for it? */
    if (!(IsWowApp) && (dwCreationFlags & CREATE_SEPARATE_WOW_VDM))
    {
        /* Ignore the nonsensical request */
        dwCreationFlags &= ~CREATE_SEPARATE_WOW_VDM;
    }

    /* Did we already check information for the section? */
    if (!QuerySection)
    {
        /* Get some information about the executable */
        Status = NtQuerySection(SectionHandle,
                                SectionImageInformation,
                                &ImageInformation,
                                sizeof(ImageInformation),
                                NULL);
        if (!NT_SUCCESS(Status))
        {
            /* We failed, bail out */
            DPRINT1("Section query failed\n");
            BaseSetLastNTError(Status);
            Result = FALSE;
            goto Quickie;
        }

        /* Don't check this later */
        QuerySection = TRUE;
    }

    /* Check if this was linked as a DLL */
    if (ImageInformation.ImageCharacteristics & IMAGE_FILE_DLL)
    {
        /* These aren't valid images to try to execute! */
        DPRINT1("Trying to launch a DLL, failing\n");
        SetLastError(ERROR_BAD_EXE_FORMAT);
        Result = FALSE;
        goto Quickie;
    }

    /* Don't let callers pass in this flag -- we'll only get it from IFEO */
    Flags &= ~PROCESS_CREATE_FLAGS_LARGE_PAGES;

    /* Clear the IFEO-missing flag, before we know for sure... */
    ParameterFlags &= ~2;

    /* If the process is being debugged, only read IFEO if the PEB says so */
    if (!(dwCreationFlags & (DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS)) ||
        (NtCurrentPeb()->ReadImageFileExecOptions))
    {
        /* Let's do this! Attempt to open IFEO */
        IFEOStatus = LdrOpenImageFileOptionsKey(&PathName, 0, &KeyHandle);
        if (!NT_SUCCESS(IFEOStatus))
        {
            /* We failed, set the flag so we store this in the parameters */
            if (IFEOStatus == STATUS_OBJECT_NAME_NOT_FOUND) ParameterFlags |= 2;
        }
        else
        {
            /* Was this our first time going through this path? */
            if (!DebuggerCmdLine)
            {
                /* Allocate a buffer for the debugger path */
                DebuggerCmdLine = RtlAllocateHeap(RtlGetProcessHeap(),
                                                  0,
                                                  MAX_PATH * sizeof(WCHAR));
                if (!DebuggerCmdLine)
                {
                    /* Close IFEO on failure */
                    IFEOStatus = NtClose(KeyHandle);
                    ASSERT(NT_SUCCESS(IFEOStatus));

                    /* Fail the call */
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    Result = FALSE;
                    goto Quickie;
                }
            }

            /* Now query for the debugger */
            IFEOStatus = LdrQueryImageFileKeyOption(KeyHandle,
                                                 L"Debugger",
                                                 REG_SZ,
                                                 DebuggerCmdLine,
                                                 MAX_PATH * sizeof(WCHAR),
                                                 &ResultSize);
            if (!(NT_SUCCESS(IFEOStatus)) ||
                (ResultSize < sizeof(WCHAR)) ||
                (DebuggerCmdLine[0] == UNICODE_NULL))
            {
                /* If it's not there, or too small, or invalid, ignore it */
                RtlFreeHeap(RtlGetProcessHeap(), 0, DebuggerCmdLine);
                DebuggerCmdLine = NULL;
            }

            /* Also query if we should map with large pages */
            IFEOStatus = LdrQueryImageFileKeyOption(KeyHandle,
                                                 L"UseLargePages",
                                                 REG_DWORD,
                                                 &UseLargePages,
                                                 sizeof(UseLargePages),
                                                 NULL);
            if ((NT_SUCCESS(IFEOStatus)) && (UseLargePages))
            {
                /* Do it! This is the only way this flag can be set */
                Flags |= PROCESS_CREATE_FLAGS_LARGE_PAGES;
            }

            /* We're done with IFEO, can close it now */
            IFEOStatus = NtClose(KeyHandle);
            ASSERT(NT_SUCCESS(IFEOStatus));
        }
    }

    /* Make sure the image was compiled for this processor */
    if ((ImageInformation.Machine < SharedUserData->ImageNumberLow) ||
        (ImageInformation.Machine > SharedUserData->ImageNumberHigh))
    {
        /* It was not -- raise a hard error */
        ErrorResponse = ResponseOk;
        ErrorParameters[0] = (ULONG_PTR)&PathName;
        NtRaiseHardError(STATUS_IMAGE_MACHINE_TYPE_MISMATCH_EXE,
                         1,
                         1,
                         ErrorParameters,
                         OptionOk,
                         &ErrorResponse);
        if (Peb->ImageSubsystemMajorVersion <= 3)
        {
            /* If it's really old, return this error */
            SetLastError(ERROR_BAD_EXE_FORMAT);
        }
        else
        {
            /* Otherwise, return a more modern error */
            SetLastError(ERROR_EXE_MACHINE_TYPE_MISMATCH);
        }

        /* Go to the failure path */
        DPRINT1("Invalid image architecture: %lx\n", ImageInformation.Machine);
        Result = FALSE;
        goto Quickie;
    }

    /* Check if this isn't a Windows image */
    if ((ImageInformation.SubSystemType != IMAGE_SUBSYSTEM_WINDOWS_GUI) &&
        (ImageInformation.SubSystemType != IMAGE_SUBSYSTEM_WINDOWS_CUI))
    {
        /* Get rid of section-related information since we'll retry */
        NtClose(SectionHandle);
        SectionHandle = NULL;
        QuerySection = FALSE;

        /* The only other non-Windows image type we support here is POSIX */
        if (ImageInformation.SubSystemType != IMAGE_SUBSYSTEM_POSIX_CUI)
        {
            /* Bail out if it's something else */
            SetLastError(ERROR_CHILD_NOT_COMPLETE);
            Result = FALSE;
            goto Quickie;
        }

        /* Now build the command-line to have posix launch this image */
        Result = BuildSubSysCommandLine(L"POSIX /P ",
                                        lpApplicationName,
                                        lpCommandLine,
                                        &DebuggerString);
        if (!Result)
        {
            /* Bail out if that failed */
            DPRINT1("Subsystem command line failed\n");
            goto Quickie;
        }

        /* And re-try launching the process, with the new command-line now */
        lpCommandLine = DebuggerString.Buffer;
        lpApplicationName = NULL;

        /* We've already done all these checks, don't do them again */
        SkipSaferAndAppCompat = TRUE;
        DPRINT1("Retrying with: %S\n", lpCommandLine);
        goto AppNameRetry;
    }

    /* Was this image built for a version of Windows whose images we can run? */
    Result = BasepIsImageVersionOk(ImageInformation.SubSystemMajorVersion,
                                   ImageInformation.SubSystemMinorVersion);
    if (!Result)
    {
        /* It was not, bail out */
        DPRINT1("Invalid subsystem version: %hu.%hu\n",
                ImageInformation.SubSystemMajorVersion,
                ImageInformation.SubSystemMinorVersion);
        SetLastError(ERROR_BAD_EXE_FORMAT);
        goto Quickie;
    }

    /* Check if there is a debugger associated with the application */
    if (DebuggerCmdLine)
    {
        /* Get the length of the command line */
        n = wcslen(lpCommandLine);
        if (!n)
        {
            /* There's no command line, use the application name instead */
            lpCommandLine = (LPWSTR)lpApplicationName;
            n = wcslen(lpCommandLine);
        }

        /* Protect against overflow */
        if (n > UNICODE_STRING_MAX_CHARS)
        {
            BaseSetLastNTError(STATUS_NAME_TOO_LONG);
            Result = FALSE;
            goto Quickie;
        }

        /* Now add the length of the debugger command-line */
        n += wcslen(DebuggerCmdLine);

        /* Again make sure we don't overflow */
        if (n > UNICODE_STRING_MAX_CHARS)
        {
            BaseSetLastNTError(STATUS_NAME_TOO_LONG);
            Result = FALSE;
            goto Quickie;
        }

        /* Account for the quotes and space between the two */
        n += sizeof("\" \"") - sizeof(ANSI_NULL);

        /* Convert to bytes, and make sure we don't overflow */
        n *= sizeof(WCHAR);
        if (n > UNICODE_STRING_MAX_BYTES)
        {
            BaseSetLastNTError(STATUS_NAME_TOO_LONG);
            Result = FALSE;
            goto Quickie;
        }

        /* Allocate space for the string */
        DebuggerString.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, n);
        if (!DebuggerString.Buffer)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            Result = FALSE;
            goto Quickie;
        }

        /* Set the length */
        RtlInitEmptyUnicodeString(&DebuggerString,
                                  DebuggerString.Buffer,
                                  (USHORT)n);

        /* Now perform the command line creation */
        ImageDbgStatus = RtlAppendUnicodeToString(&DebuggerString,
                                                  DebuggerCmdLine);
        ASSERT(NT_SUCCESS(ImageDbgStatus));
        ImageDbgStatus = RtlAppendUnicodeToString(&DebuggerString, L" ");
        ASSERT(NT_SUCCESS(ImageDbgStatus));
        ImageDbgStatus = RtlAppendUnicodeToString(&DebuggerString, lpCommandLine);
        ASSERT(NT_SUCCESS(ImageDbgStatus));

        /* Make sure it all looks nice */
        DbgPrint("BASE: Calling debugger with '%wZ'\n", &DebuggerString);

        /* Update the command line and application name */
        lpCommandLine = DebuggerString.Buffer;
        lpApplicationName = NULL;

        /* Close all temporary state */
        NtClose(SectionHandle);
        SectionHandle = NULL;
        QuerySection = FALSE;

        /* Free all temporary memory */
        RtlFreeHeap(RtlGetProcessHeap(), 0, NameBuffer);
        NameBuffer = NULL;
        RtlFreeHeap(RtlGetProcessHeap(), 0, FreeBuffer);
        FreeBuffer = NULL;
        RtlFreeHeap(RtlGetProcessHeap(), 0, DebuggerCmdLine);
        DebuggerCmdLine = NULL;
        DPRINT1("Retrying with: %S\n", lpCommandLine);
        goto AppNameRetry;
    }

    /* Initialize the process object attributes */
    ObjectAttributes = BaseFormatObjectAttributes(&LocalObjectAttributes,
                                                  lpProcessAttributes,
                                                  NULL);
    if ((hUserToken) && (lpProcessAttributes))
    {
        /* Augment them with information from the user */

        LocalProcessAttributes = *lpProcessAttributes;
        LocalProcessAttributes.lpSecurityDescriptor = NULL;
        ObjectAttributes = BaseFormatObjectAttributes(&LocalObjectAttributes,
                                                      &LocalProcessAttributes,
                                                      NULL);
    }

    /* Check if we're going to be debugged */
    if (dwCreationFlags & DEBUG_PROCESS)
    {
        /* Set process flag */
        Flags |= PROCESS_CREATE_FLAGS_BREAKAWAY;
    }

    /* Check if we're going to be debugged */
    if (dwCreationFlags & (DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS))
    {
        /* Connect to DbgUi */
        Status = DbgUiConnectToDbg();
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to connect to DbgUI!\n");
            BaseSetLastNTError(Status);
            Result = FALSE;
            goto Quickie;
        }

        /* Get the debug object */
        DebugHandle = DbgUiGetThreadDebugObject();

        /* Check if only this process will be debugged */
        if (dwCreationFlags & DEBUG_ONLY_THIS_PROCESS)
        {
            /* Set process flag */
            Flags |= PROCESS_CREATE_FLAGS_NO_DEBUG_INHERIT;
        }
    }

    /* Set inherit flag */
    if (bInheritHandles) Flags |= PROCESS_CREATE_FLAGS_INHERIT_HANDLES;

    /* Check if the process should be created with large pages */
    HavePrivilege = FALSE;
    PrivilegeState = NULL;
    if (Flags & PROCESS_CREATE_FLAGS_LARGE_PAGES)
    {
        /* Acquire the required privilege so that the kernel won't fail the call */
        PrivilegeValue = SE_LOCK_MEMORY_PRIVILEGE;
        Status = RtlAcquirePrivilege(&PrivilegeValue, 1, 0, &PrivilegeState);
        if (NT_SUCCESS(Status))
        {
            /* Remember to release it later */
            HavePrivilege = TRUE;
        }
    }

    /* Save the current TIB value since kernel overwrites it to store PEB */
    TibValue = Teb->NtTib.ArbitraryUserPointer;

    /* Tell the kernel to create the process */
    Status = NtCreateProcessEx(&ProcessHandle,
                               PROCESS_ALL_ACCESS,
                               ObjectAttributes,
                               NtCurrentProcess(),
                               Flags,
                               SectionHandle,
                               DebugHandle,
                               NULL,
                               InJob);

    /* Load the PEB address from the hacky location where the kernel stores it */
    RemotePeb = Teb->NtTib.ArbitraryUserPointer;

    /* And restore the old TIB value */
    Teb->NtTib.ArbitraryUserPointer = TibValue;

    /* Release the large page privilege if we had acquired it */
    if (HavePrivilege) RtlReleasePrivilege(PrivilegeState);

    /* And now check if the kernel failed to create the process */
    if (!NT_SUCCESS(Status))
    {
        /* Go to failure path */
        DPRINT1("Failed to create process: %lx\n", Status);
        BaseSetLastNTError(Status);
        Result = FALSE;
        goto Quickie;
    }

    /* Check if there is a priority class to set */
    if (PriorityClass.PriorityClass)
    {
        /* Reset current privilege state */
        RealTimePrivilegeState = NULL;

        /* Is realtime priority being requested? */
        if (PriorityClass.PriorityClass == PROCESS_PRIORITY_CLASS_REALTIME)
        {
            /* Check if the caller has real-time access, and enable it if so */
            RealTimePrivilegeState = BasepIsRealtimeAllowed(TRUE);
        }

        /* Set the new priority class and release the privilege */
        Status = NtSetInformationProcess(ProcessHandle,
                                         ProcessPriorityClass,
                                         &PriorityClass,
                                         sizeof(PROCESS_PRIORITY_CLASS));
        if (RealTimePrivilegeState) RtlReleasePrivilege(RealTimePrivilegeState);

        /* Check if we failed to set the priority class */
        if (!NT_SUCCESS(Status))
        {
            /* Bail out on failure */
            DPRINT1("Failed to set priority class: %lx\n", Status);
            BaseSetLastNTError(Status);
            Result = FALSE;
            goto Quickie;
        }
    }

    /* Check if the caller wants the default error mode */
    if (dwCreationFlags & CREATE_DEFAULT_ERROR_MODE)
    {
        /* Set Error Mode to only fail on critical errors */
        HardErrorMode = SEM_FAILCRITICALERRORS;
        NtSetInformationProcess(ProcessHandle,
                                ProcessDefaultHardErrorMode,
                                &HardErrorMode,
                                sizeof(ULONG));
    }

    /* Check if this was a VDM binary */
    if (VdmBinaryType)
    {
        /* Update VDM by telling it the process has now been created */
        VdmWaitObject = ProcessHandle;
        Result = BaseUpdateVDMEntry(VdmEntryUpdateProcess,
                                    &VdmWaitObject,
                                    VdmTask,
                                    VdmBinaryType);

        if (!Result)
        {
            /* Bail out on failure */
            DPRINT1("Failed to update VDM with wait object\n");
            VdmWaitObject = NULL;
            goto Quickie;
        }

        /* At this point, a failure means VDM has to undo all the state */
        VdmUndoLevel |= VDM_UNDO_FULL;
    }

    /* Check if VDM needed reserved low-memory */
    if (VdmReserve)
    {
        /* Reserve the requested allocation */
        RegionSize = VdmReserve;
        Status = NtAllocateVirtualMemory(ProcessHandle,
                                         &BaseAddress,
                                         0,
                                         &RegionSize,
                                         MEM_RESERVE,
                                         PAGE_EXECUTE_READWRITE);
        if (!NT_SUCCESS(Status))
        {
            /* Bail out on failure */
            DPRINT1("Failed to reserve memory for VDM: %lx\n", Status);
            BaseSetLastNTError(Status);
            Result = FALSE;
            goto Quickie;
        }

        VdmReserve = (ULONG)RegionSize;
    }

    /* Check if we've already queried information on the section */
    if (!QuerySection)
    {
        /* We haven't, so get some information about the executable */
        Status = NtQuerySection(SectionHandle,
                                SectionImageInformation,
                                &ImageInformation,
                                sizeof(ImageInformation),
                                NULL);
        if (!NT_SUCCESS(Status))
        {
            /* Bail out on failure */
            DPRINT1("Failed to query section: %lx\n", Status);
            BaseSetLastNTError(Status);
            Result = FALSE;
            goto Quickie;
        }

        /* If we encounter a restart, don't re-query this information again */
        QuerySection = TRUE;
    }

    /* Do we need to apply SxS to this image? */
    if (!(ImageInformation.DllCharacteristics & IMAGE_DLLCHARACTERISTICS_NO_ISOLATION))
    {
        /* Too bad, we don't support this yet */
        DPRINT1("Image should receive SxS Fusion Isolation\n");
    }

    /* There's some SxS flag that we need to set if fusion flags have 1 set */
    if (FusionFlags & 1) CreateProcessMsg->Sxs.Flags |= 0x10;

    /* Check if we have a current directory */
    if (lpCurrentDirectory)
    {
        /* Allocate a buffer so we can keep a Unicode copy */
        DPRINT("Current directory: %S\n", lpCurrentDirectory);
        CurrentDirectory = RtlAllocateHeap(RtlGetProcessHeap(),
                                           0,
                                           (MAX_PATH * sizeof(WCHAR)) +
                                           sizeof(UNICODE_NULL));
        if (!CurrentDirectory)
        {
            /* Bail out if this failed */
            BaseSetLastNTError(STATUS_NO_MEMORY);
            Result = FALSE;
            goto Quickie;
        }

        /* Get the length in Unicode */
        Length = GetFullPathNameW(lpCurrentDirectory,
                                  MAX_PATH,
                                  CurrentDirectory,
                                  &FilePart);
        if (Length > MAX_PATH)
        {
            /* The directory is too long, so bail out */
            SetLastError(ERROR_DIRECTORY);
            Result = FALSE;
            goto Quickie;
        }

        /* Make sure the directory is actually valid */
        FileAttribs = GetFileAttributesW(CurrentDirectory);
        if ((FileAttribs == INVALID_FILE_ATTRIBUTES) ||
           !(FileAttribs & FILE_ATTRIBUTE_DIRECTORY))
        {
            /* It isn't, so bail out */
            DPRINT1("Current directory is invalid\n");
            SetLastError(ERROR_DIRECTORY);
            Result = FALSE;
            goto Quickie;
        }
    }

    /* Insert quotes if needed */
    if ((QuotesNeeded) || (CmdLineIsAppName))
    {
        /* Allocate our buffer, plus enough space for quotes and a NULL */
        QuotedCmdLine = RtlAllocateHeap(RtlGetProcessHeap(),
                                        0,
                                        (wcslen(lpCommandLine) * sizeof(WCHAR)) +
                                        (2 * sizeof(L'\"') + sizeof(UNICODE_NULL)));
        if (QuotedCmdLine)
        {
            /* Copy the first quote */
            wcscpy(QuotedCmdLine, L"\"");

            /* Save the current null-character */
            if (QuotesNeeded)
            {
                SaveChar = *NullBuffer;
                *NullBuffer = UNICODE_NULL;
            }

            /* Copy the command line and the final quote */
            wcscat(QuotedCmdLine, lpCommandLine);
            wcscat(QuotedCmdLine, L"\"");

            /* Copy the null-char back */
            if (QuotesNeeded)
            {
                *NullBuffer = SaveChar;
                wcscat(QuotedCmdLine, NullBuffer);
            }
        }
        else
        {
            /* We can't put quotes around the thing, so try it anyway */
            if (QuotesNeeded) QuotesNeeded = FALSE;
            if (CmdLineIsAppName) CmdLineIsAppName = FALSE;
        }
    }

    /* Use isolation if needed */
    if (CreateProcessMsg->Sxs.Flags & 1) ParameterFlags |= 1;

    /* Set the new command-line if needed */
    if ((QuotesNeeded) || (CmdLineIsAppName)) lpCommandLine = QuotedCmdLine;

    /* Call the helper function in charge of RTL_USER_PROCESS_PARAMETERS */
    Result = BasePushProcessParameters(ParameterFlags,
                                       ProcessHandle,
                                       RemotePeb,
                                       lpApplicationName,
                                       CurrentDirectory,
                                       lpCommandLine,
                                       lpEnvironment,
                                       &StartupInfo,
                                       dwCreationFlags | NoWindow,
                                       bInheritHandles,
                                       IsWowApp ? IMAGE_SUBSYSTEM_WINDOWS_GUI: 0,
                                       AppCompatData,
                                       AppCompatDataSize);
    if (!Result)
    {
        /* The remote process would have an undefined state, so fail the call */
        DPRINT1("BasePushProcessParameters failed\n");
        goto Quickie;
    }

    /* Free the VDM command line string as it's no longer needed */
    RtlFreeUnicodeString(&VdmString);
    VdmString.Buffer = NULL;

    /* Non-VDM console applications usually inherit handles unless specified */
    if (!(VdmBinaryType) &&
        !(bInheritHandles) &&
        !(StartupInfo.dwFlags & STARTF_USESTDHANDLES) &&
        !(dwCreationFlags & (CREATE_NO_WINDOW |
                             CREATE_NEW_CONSOLE |
                             DETACHED_PROCESS)) &&
        (ImageInformation.SubSystemType == IMAGE_SUBSYSTEM_WINDOWS_CUI))
    {
        /* Get the remote parameters */
        Status = NtReadVirtualMemory(ProcessHandle,
                                     &RemotePeb->ProcessParameters,
                                     &ProcessParameters,
                                     sizeof(PRTL_USER_PROCESS_PARAMETERS),
                                     NULL);
        if (NT_SUCCESS(Status))
        {
            /* Duplicate standard input unless it's a console handle */
            if (!IsConsoleHandle(Peb->ProcessParameters->StandardInput))
            {
                StuffStdHandle(ProcessHandle,
                               Peb->ProcessParameters->StandardInput,
                               &ProcessParameters->StandardInput);
            }

            /* Duplicate standard output unless it's a console handle */
            if (!IsConsoleHandle(Peb->ProcessParameters->StandardOutput))
            {
                StuffStdHandle(ProcessHandle,
                               Peb->ProcessParameters->StandardOutput,
                               &ProcessParameters->StandardOutput);
            }

            /* Duplicate standard error unless it's a console handle */
            if (!IsConsoleHandle(Peb->ProcessParameters->StandardError))
            {
                StuffStdHandle(ProcessHandle,
                               Peb->ProcessParameters->StandardError,
                               &ProcessParameters->StandardError);
            }
        }
    }

    /* Create the Thread's Stack */
    StackSize = max(256 * 1024, ImageInformation.MaximumStackSize);
    Status = BaseCreateStack(ProcessHandle,
                             ImageInformation.CommittedStackSize,
                             StackSize,
                             &InitialTeb);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Creating the thread stack failed: %lx\n", Status);
        BaseSetLastNTError(Status);
        Result = FALSE;
        goto Quickie;
    }

    /* Create the Thread's Context */
    BaseInitializeContext(&Context,
                          Peb,
                          ImageInformation.TransferAddress,
                          InitialTeb.StackBase,
                          0);

    /* Convert the thread attributes */
    ObjectAttributes = BaseFormatObjectAttributes(&LocalObjectAttributes,
                                                  lpThreadAttributes,
                                                  NULL);
    if ((hUserToken) && (lpThreadAttributes))
    {
        /* If the caller specified a user token, zero the security descriptor */
        LocalThreadAttributes = *lpThreadAttributes;
        LocalThreadAttributes.lpSecurityDescriptor = NULL;
        ObjectAttributes = BaseFormatObjectAttributes(&LocalObjectAttributes,
                                                      &LocalThreadAttributes,
                                                      NULL);
    }

    /* Create the Kernel Thread Object */
    Status = NtCreateThread(&ThreadHandle,
                            THREAD_ALL_ACCESS,
                            ObjectAttributes,
                            ProcessHandle,
                            &ClientId,
                            &Context,
                            &InitialTeb,
                            TRUE);
    if (!NT_SUCCESS(Status))
    {
        /* A process is not allowed to exist without a main thread, so fail */
        DPRINT1("Creating the main thread failed: %lx\n", Status);
        BaseSetLastNTError(Status);
        Result = FALSE;
        goto Quickie;
    }

    /* Begin filling out the CSRSS message, first with our IDs and handles */
    CreateProcessMsg->ProcessHandle = ProcessHandle;
    CreateProcessMsg->ThreadHandle = ThreadHandle;
    CreateProcessMsg->ClientId = ClientId;

    /* Write the remote PEB address and clear it locally, we no longer use it */
    CreateProcessMsg->PebAddressNative = RemotePeb;
#ifdef _WIN64
    DPRINT1("TODO: WOW64 is not supported yet\n");
    CreateProcessMsg->PebAddressWow64 = 0;
#else
    CreateProcessMsg->PebAddressWow64 = (ULONG)RemotePeb;
#endif
    RemotePeb = NULL;

    /* Now check what kind of architecture this image was made for */
    switch (ImageInformation.Machine)
    {
        /* IA32, IA64 and AMD64 are supported in Server 2003 */
        case IMAGE_FILE_MACHINE_I386:
            CreateProcessMsg->ProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
            break;
        case IMAGE_FILE_MACHINE_IA64:
            CreateProcessMsg->ProcessorArchitecture = PROCESSOR_ARCHITECTURE_IA64;
            break;
        case IMAGE_FILE_MACHINE_AMD64:
            CreateProcessMsg->ProcessorArchitecture = PROCESSOR_ARCHITECTURE_AMD64;
            break;

        /* Anything else results in image unknown -- but no failure */
        default:
            DbgPrint("kernel32: No mapping for ImageInformation.Machine == %04x\n",
                     ImageInformation.Machine);
            CreateProcessMsg->ProcessorArchitecture = PROCESSOR_ARCHITECTURE_UNKNOWN;
            break;
    }

    /* Write the input creation flags except any debugger-related flags */
    CreateProcessMsg->CreationFlags = dwCreationFlags &
                                      ~(DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS);

    /* CSRSS needs to know if this is a GUI app or not */
    if ((ImageInformation.SubSystemType == IMAGE_SUBSYSTEM_WINDOWS_GUI) ||
        (IsWowApp))
    {
        /*
         * For GUI apps we turn on the 2nd bit. This allow CSRSS server dlls
         * (basesrv in particular) to know whether or not this is a GUI or a
         * TUI application.
         */
        AddToHandle(CreateProcessMsg->ProcessHandle, 2);

        /* Also check if the parent is also a GUI process */
        NtHeaders = RtlImageNtHeader(GetModuleHandle(NULL));
        if ((NtHeaders) &&
            (NtHeaders->OptionalHeader.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI))
        {
            /* Let it know that it should display the hourglass mouse cursor */
            AddToHandle(CreateProcessMsg->ProcessHandle, 1);
        }
    }

    /* For all apps, if this flag is on, the hourglass mouse cursor is shown */
    if (StartupInfo.dwFlags & STARTF_FORCEONFEEDBACK)
    {
        AddToHandle(CreateProcessMsg->ProcessHandle, 1);
    }

    /* Likewise, the opposite holds as well */
    if (StartupInfo.dwFlags & STARTF_FORCEOFFFEEDBACK)
    {
        RemoveFromHandle(CreateProcessMsg->ProcessHandle, 1);
    }

    /* Also store which kind of VDM app (if any) this is */
    CreateProcessMsg->VdmBinaryType = VdmBinaryType;

    /* And if it really is a VDM app... */
    if (VdmBinaryType)
    {
        /* Store the task ID and VDM console handle */
        CreateProcessMsg->hVDM = VdmTask ? 0 : Peb->ProcessParameters->ConsoleHandle;
        CreateProcessMsg->VdmTask = VdmTask;
    }
    else if (VdmReserve)
    {
        /* Extended VDM, set a flag */
        CreateProcessMsg->VdmBinaryType |= BINARY_TYPE_WOW_EX;
    }

    /* Check if there's side-by-side assembly data associated with the process */
    if (CreateProcessMsg->Sxs.Flags)
    {
        /* This should not happen in ReactOS yet */
        DPRINT1("This is an SxS Message -- should not happen yet\n");
        BaseSetLastNTError(STATUS_NOT_IMPLEMENTED);
        NtTerminateProcess(ProcessHandle, STATUS_NOT_IMPLEMENTED);
        Result = FALSE;
        goto Quickie;
    }

    /* We are finally ready to call CSRSS to tell it about our new process! */
    CsrClientCallServer((PCSR_API_MESSAGE)&CsrMsg[0],
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(BASESRV_SERVERDLL_INDEX,
                                              BasepCreateProcess),
                        sizeof(*CreateProcessMsg));

    /* CSRSS has returned, free the capture buffer now if we had one */
    if (CaptureBuffer)
    {
        CsrFreeCaptureBuffer(CaptureBuffer);
        CaptureBuffer = NULL;
    }

    /* Check if CSRSS failed to accept ownership of the new Windows process */
    if (!NT_SUCCESS(CsrMsg[0].Status))
    {
        /* Terminate the process and enter failure path with the CSRSS status */
        DPRINT1("Failed to tell csrss about new process\n");
        BaseSetLastNTError(CsrMsg[0].Status);
        NtTerminateProcess(ProcessHandle, CsrMsg[0].Status);
        Result = FALSE;
        goto Quickie;
    }

    /* Check if we have a token due to Authz/Safer, not passed by the user */
    if ((TokenHandle) && !(hUserToken))
    {
        /* Replace the process and/or thread token with the one from Safer */
        Status = BasepReplaceProcessThreadTokens(TokenHandle,
                                                 ProcessHandle,
                                                 ThreadHandle);
        if (!NT_SUCCESS(Status))
        {
            /* If this failed, kill the process and enter the failure path */
            DPRINT1("Failed to update process token: %lx\n", Status);
            NtTerminateProcess(ProcessHandle, Status);
            BaseSetLastNTError(Status);
            Result = FALSE;
            goto Quickie;
        }
    }

    /* Check if a job was associated with this process */
    if (JobHandle)
    {
        /* Bind the process and job together now */
        Status = NtAssignProcessToJobObject(JobHandle, ProcessHandle);
        if (!NT_SUCCESS(Status))
        {
            /* Kill the process and enter the failure path if binding failed */
            DPRINT1("Failed to assign process to job: %lx\n", Status);
            NtTerminateProcess(ProcessHandle, STATUS_ACCESS_DENIED);
            BaseSetLastNTError(Status);
            Result = FALSE;
            goto Quickie;
        }
    }

    /* Finally, resume the thread to actually get the process started */
    if (!(dwCreationFlags & CREATE_SUSPENDED))
    {
        NtResumeThread(ThreadHandle, &ResumeCount);
    }

VdmShortCircuit:
    /* We made it this far, meaning we have a fully created process and thread */
    Result = TRUE;

    /* Anyone doing a VDM undo should now undo everything, since we are done */
    if (VdmUndoLevel) VdmUndoLevel |= VDM_UNDO_COMPLETED;

    /* Having a VDM wait object implies this must be a VDM process */
    if (VdmWaitObject)
    {
        /* Check if it's a 16-bit separate WOW process */
        if (VdmBinaryType == BINARY_TYPE_SEPARATE_WOW)
        {
            /* OR-in the special flag to indicate this, and return to caller */
            AddToHandle(VdmWaitObject, 2);
            lpProcessInformation->hProcess = VdmWaitObject;

            /* Check if this was a re-used VDM */
            if (VdmUndoLevel & VDM_UNDO_REUSE)
            {
                /* No Client ID should be returned in this case */
                ClientId.UniqueProcess = 0;
                ClientId.UniqueThread = 0;
            }
        }
        else
        {
            /* OR-in the special flag to indicate this is not a separate VDM */
            AddToHandle(VdmWaitObject, 1);

            /* Return handle to the caller */
            lpProcessInformation->hProcess = VdmWaitObject;
        }

        /* Close the original process handle, since it's not needed for VDM */
        if (ProcessHandle) NtClose(ProcessHandle);
    }
    else
    {
        /* This is a regular process, so return the real process handle */
        lpProcessInformation->hProcess = ProcessHandle;
    }

    /* Return the rest of the process information based on what we have so far */
    lpProcessInformation->hThread = ThreadHandle;
    lpProcessInformation->dwProcessId = HandleToUlong(ClientId.UniqueProcess);
    lpProcessInformation->dwThreadId = HandleToUlong(ClientId.UniqueThread);

    /* NULL these out here so we know to treat this as a success scenario */
    ProcessHandle = NULL;
    ThreadHandle = NULL;

Quickie:
    /* Free the debugger command line if one was allocated */
    if (DebuggerCmdLine) RtlFreeHeap(RtlGetProcessHeap(), 0, DebuggerCmdLine);

    /* Check if an SxS full path as queried */
    if (PathBuffer)
    {
        /* Reinitialize the executable path */
        RtlInitEmptyUnicodeString(&SxsWin32ExePath, NULL, 0);
        SxsWin32ExePath.Length = 0;

        /* Free the path buffer */
        RtlFreeHeap(RtlGetProcessHeap(), 0, PathBuffer);
    }

#if _SXS_SUPPORT_ENABLED_
    /* Check if this was a non-VDM process */
    if (!VdmBinaryType)
    {
        /* Then it must've had SxS data, so close the handles used for it */
        BasepSxsCloseHandles(&Handles);
        BasepSxsCloseHandles(&FileHandles);

        /* Check if we built SxS byte buffers for this create process request */
        if (SxsConglomeratedBuffer)
        {
            /* Loop all of them */
            for (i = 0; i < 5; i++)
            {
                /* Check if this one was allocated */
                ThisBuffer = SxsStaticBuffers[i];
                if (ThisBuffer)
                {
                    /* Get the underlying RTL_BUFFER structure */
                    ByteBuffer = &ThisBuffer->ByteBuffer;
                    if ((ThisBuffer != (PVOID)-8) && (ByteBuffer->Buffer))
                    {
                        /* Check if it was dynamic */
                        if (ByteBuffer->Buffer != ByteBuffer->StaticBuffer)
                        {
                            /* Free it from the heap */
                            FreeString.Buffer = (PWCHAR)ByteBuffer->Buffer;
                            RtlFreeUnicodeString(&FreeString);
                        }

                        /* Reset the buffer to its static data */
                        ByteBuffer->Buffer = ByteBuffer->StaticBuffer;
                        ByteBuffer->Size = ByteBuffer->StaticSize;
                    }

                    /* Reset the string to the static buffer */
                    RtlInitEmptyUnicodeString(&ThisBuffer->String,
                                              (PWCHAR)ByteBuffer->StaticBuffer,
                                              ByteBuffer->StaticSize);
                    if (ThisBuffer->String.Buffer)
                    {
                        /* Also NULL-terminate it */
                        *ThisBuffer->String.Buffer = UNICODE_NULL;
                    }
                }
            }
        }
    }
#endif
    /* Check if an environment was passed in */
    if ((lpEnvironment) && !(dwCreationFlags & CREATE_UNICODE_ENVIRONMENT))
    {
        /* Destroy it */
        RtlDestroyEnvironment(lpEnvironment);

        /* If this was the VDM environment too, clear that as well */
        if (VdmUnicodeEnv.Buffer == lpEnvironment) VdmUnicodeEnv.Buffer = NULL;
        lpEnvironment = NULL;
    }

    /* Unconditionally free all the name parsing buffers we always allocate */
    RtlFreeHeap(RtlGetProcessHeap(), 0, QuotedCmdLine);
    RtlFreeHeap(RtlGetProcessHeap(), 0, NameBuffer);
    RtlFreeHeap(RtlGetProcessHeap(), 0, CurrentDirectory);
    RtlFreeHeap(RtlGetProcessHeap(), 0, FreeBuffer);

    /* Close open file/section handles */
    if (FileHandle) NtClose(FileHandle);
    if (SectionHandle) NtClose(SectionHandle);

    /* If we have a thread handle, this was a failure path */
    if (ThreadHandle)
    {
        /* So kill the process and close the thread handle */
        NtTerminateProcess(ProcessHandle, 0);
        NtClose(ThreadHandle);
    }

    /* If we have a process handle, this was a failure path, so close it */
    if (ProcessHandle) NtClose(ProcessHandle);

    /* Thread/process handles, if any, are now processed. Now close this one. */
    if (JobHandle) NtClose(JobHandle);

    /* Check if we had created a token */
    if (TokenHandle)
    {
        /* And if the user asked for one */
        if (hUserToken)
        {
            /* Then return it */
            *hNewToken = TokenHandle;
        }
        else
        {
            /* User didn't want it, so we used it temporarily -- close it */
            NtClose(TokenHandle);
        }
    }

    /* Free any temporary app compatibility data, it's no longer needed */
    BasepFreeAppCompatData(AppCompatData, AppCompatSxsData);

    /* Free a few strings. The API takes care of these possibly being NULL */
    RtlFreeUnicodeString(&VdmString);
    RtlFreeUnicodeString(&DebuggerString);

    /* Check if we had built any sort of VDM environment */
    if ((VdmAnsiEnv.Buffer) || (VdmUnicodeEnv.Buffer))
    {
        /* Free it */
        BaseDestroyVDMEnvironment(&VdmAnsiEnv, &VdmUnicodeEnv);
    }

    /* Check if this was any kind of VDM application that we ended up creating */
    if ((VdmUndoLevel) && (!(VdmUndoLevel & VDM_UNDO_COMPLETED)))
    {
        /* Send an undo */
        BaseUpdateVDMEntry(VdmEntryUndo,
                           (PHANDLE)&VdmTask,
                           VdmUndoLevel,
                           VdmBinaryType);

        /* And close whatever VDM handle we were using for notifications */
        if (VdmWaitObject) NtClose(VdmWaitObject);
    }

    /* Check if we ended up here with an allocated search path, and free it */
    if (SearchPath) RtlFreeHeap(RtlGetProcessHeap(), 0, SearchPath);

    /* Finally, return the API's result */
    return Result;
}

/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
CreateProcessW(LPCWSTR lpApplicationName,
               LPWSTR lpCommandLine,
               LPSECURITY_ATTRIBUTES lpProcessAttributes,
               LPSECURITY_ATTRIBUTES lpThreadAttributes,
               BOOL bInheritHandles,
               DWORD dwCreationFlags,
               LPVOID lpEnvironment,
               LPCWSTR lpCurrentDirectory,
               LPSTARTUPINFOW lpStartupInfo,
               LPPROCESS_INFORMATION lpProcessInformation)
{
    /* Call the internal (but exported) version */
    return CreateProcessInternalW(NULL,
                                  lpApplicationName,
                                  lpCommandLine,
                                  lpProcessAttributes,
                                  lpThreadAttributes,
                                  bInheritHandles,
                                  dwCreationFlags,
                                  lpEnvironment,
                                  lpCurrentDirectory,
                                  lpStartupInfo,
                                  lpProcessInformation,
                                  NULL);
}

/*
 * @implemented
 */
BOOL
WINAPI
CreateProcessInternalA(HANDLE hToken,
                       LPCSTR lpApplicationName,
                       LPSTR lpCommandLine,
                       LPSECURITY_ATTRIBUTES lpProcessAttributes,
                       LPSECURITY_ATTRIBUTES lpThreadAttributes,
                       BOOL bInheritHandles,
                       DWORD dwCreationFlags,
                       LPVOID lpEnvironment,
                       LPCSTR lpCurrentDirectory,
                       LPSTARTUPINFOA lpStartupInfo,
                       LPPROCESS_INFORMATION lpProcessInformation,
                       PHANDLE hNewToken)
{
    UNICODE_STRING CommandLine;
    UNICODE_STRING ApplicationName;
    UNICODE_STRING CurrentDirectory;
    BOOL bRetVal;
    STARTUPINFOW StartupInfo;

    DPRINT("dwCreationFlags %x, lpEnvironment %p, lpCurrentDirectory %p, "
            "lpStartupInfo %p, lpProcessInformation %p\n",
            dwCreationFlags, lpEnvironment, lpCurrentDirectory,
            lpStartupInfo, lpProcessInformation);

    /* Copy Startup Info */
    RtlMoveMemory(&StartupInfo, lpStartupInfo, sizeof(*lpStartupInfo));

    /* Initialize all strings to nothing */
    CommandLine.Buffer = NULL;
    ApplicationName.Buffer = NULL;
    CurrentDirectory.Buffer = NULL;
    StartupInfo.lpDesktop = NULL;
    StartupInfo.lpReserved = NULL;
    StartupInfo.lpTitle = NULL;

    /* Convert the Command line */
    if (lpCommandLine)
    {
        Basep8BitStringToDynamicUnicodeString(&CommandLine,
                                              lpCommandLine);
    }

    /* Convert the Name and Directory */
    if (lpApplicationName)
    {
        Basep8BitStringToDynamicUnicodeString(&ApplicationName,
                                              lpApplicationName);
    }
    if (lpCurrentDirectory)
    {
        Basep8BitStringToDynamicUnicodeString(&CurrentDirectory,
                                              lpCurrentDirectory);
    }

    /* Now convert Startup Strings */
    if (lpStartupInfo->lpReserved)
    {
        BasepAnsiStringToHeapUnicodeString(lpStartupInfo->lpReserved,
                                           &StartupInfo.lpReserved);
    }
    if (lpStartupInfo->lpDesktop)
    {
        BasepAnsiStringToHeapUnicodeString(lpStartupInfo->lpDesktop,
                                           &StartupInfo.lpDesktop);
    }
    if (lpStartupInfo->lpTitle)
    {
        BasepAnsiStringToHeapUnicodeString(lpStartupInfo->lpTitle,
                                           &StartupInfo.lpTitle);
    }

    /* Call the Unicode function */
    bRetVal = CreateProcessInternalW(hToken,
                                     ApplicationName.Buffer,
                                     CommandLine.Buffer,
                                     lpProcessAttributes,
                                     lpThreadAttributes,
                                     bInheritHandles,
                                     dwCreationFlags,
                                     lpEnvironment,
                                     CurrentDirectory.Buffer,
                                     &StartupInfo,
                                     lpProcessInformation,
                                     hNewToken);

    /* Clean up */
    RtlFreeUnicodeString(&ApplicationName);
    RtlFreeUnicodeString(&CommandLine);
    RtlFreeUnicodeString(&CurrentDirectory);
    RtlFreeHeap(RtlGetProcessHeap(), 0, StartupInfo.lpDesktop);
    RtlFreeHeap(RtlGetProcessHeap(), 0, StartupInfo.lpReserved);
    RtlFreeHeap(RtlGetProcessHeap(), 0, StartupInfo.lpTitle);

    /* Return what Unicode did */
    return bRetVal;
}

/*
 * FUNCTION: The CreateProcess function creates a new process and its
 * primary thread. The new process executes the specified executable file
 * ARGUMENTS:
 *
 *     lpApplicationName = Pointer to name of executable module
 *     lpCommandLine = Pointer to command line string
 *     lpProcessAttributes = Process security attributes
 *     lpThreadAttributes = Thread security attributes
 *     bInheritHandles = Handle inheritance flag
 *     dwCreationFlags = Creation flags
 *     lpEnvironment = Pointer to new environment block
 *     lpCurrentDirectory = Pointer to current directory name
 *     lpStartupInfo = Pointer to startup info
 *     lpProcessInformation = Pointer to process information
 *
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
CreateProcessA(LPCSTR lpApplicationName,
               LPSTR lpCommandLine,
               LPSECURITY_ATTRIBUTES lpProcessAttributes,
               LPSECURITY_ATTRIBUTES lpThreadAttributes,
               BOOL bInheritHandles,
               DWORD dwCreationFlags,
               LPVOID lpEnvironment,
               LPCSTR lpCurrentDirectory,
               LPSTARTUPINFOA lpStartupInfo,
               LPPROCESS_INFORMATION lpProcessInformation)
{
    /* Call the internal (but exported) version */
    return CreateProcessInternalA(NULL,
                                  lpApplicationName,
                                  lpCommandLine,
                                  lpProcessAttributes,
                                  lpThreadAttributes,
                                  bInheritHandles,
                                  dwCreationFlags,
                                  lpEnvironment,
                                  lpCurrentDirectory,
                                  lpStartupInfo,
                                  lpProcessInformation,
                                  NULL);
}

/*
 * @implemented
 */
UINT
WINAPI
DECLSPEC_HOTPATCH
WinExec(LPCSTR lpCmdLine,
        UINT uCmdShow)
{
    STARTUPINFOA StartupInfo;
    PROCESS_INFORMATION  ProcessInformation;
    DWORD dosErr;

    RtlZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(STARTUPINFOA);
    StartupInfo.wShowWindow = (WORD)uCmdShow;
    StartupInfo.dwFlags = 0;

    if (!CreateProcessA(NULL,
                        (PVOID)lpCmdLine,
                        NULL,
                        NULL,
                        FALSE,
                        0,
                        NULL,
                        NULL,
                        &StartupInfo,
                        &ProcessInformation))
    {
        dosErr = GetLastError();
        return dosErr < 32 ? dosErr : ERROR_BAD_FORMAT;
    }

    if (NULL != UserWaitForInputIdleRoutine)
    {
        UserWaitForInputIdleRoutine(ProcessInformation.hProcess,
                                           10000);
    }

    NtClose(ProcessInformation.hProcess);
    NtClose(ProcessInformation.hThread);

    return 33; /* Something bigger than 31 means success. */
}

/* EOF */
