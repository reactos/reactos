/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/proc/proc.c
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
    SIZE_T Dummy;

    /* Duplicate the handle */
    Status = NtDuplicateObject(NtCurrentProcess(),
                               StandardHandle,
                               ProcessHandle,
                               &DuplicatedHandle,
                               DUPLICATE_SAME_ACCESS | DUPLICATE_SAME_ATTRIBUTES,
                               0,
                               0);
    if (NT_SUCCESS(Status))
    {
        /* Write it */
        NtWriteVirtualMemory(ProcessHandle,
                             Address,
                             &DuplicatedHandle,
                             sizeof(HANDLE),
                             &Dummy);
    }
}

BOOLEAN
WINAPI
BuildSubSysCommandLine(IN LPWSTR SubsystemName,
                       IN LPWSTR ApplicationName,
                       IN LPWSTR CommandLine,
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
    RtlInitEmptyUnicodeString(SubsysCommandLine, Buffer, Length);
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
    /* Accept images for NT 3.1 or higher, as long as they're not newer than us */
    return ((ImageMajorVersion >= 3) &&
            ((ImageMajorVersion != 3) ||
             (ImageMinorVersion >= 10)) &&
            (ImageMajorVersion <= SharedUserData->NtMajorVersion) &&
            ((ImageMajorVersion != SharedUserData->NtMajorVersion) ||
             (ImageMinorVersion <= SharedUserData->NtMinorVersion)));
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
BasepIsProcessAllowed(IN PCHAR ApplicationName)
{
    NTSTATUS Status;
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
            Status = NtOpenKey(&KeyHandle, KEY_READ, &KeyAttributes);
            if (NT_SUCCESS(Status))
            {
                /* Close it, we'll query it through Rtl */
                NtClose(KeyHandle);

                /* Do the query, which will call a special callback */
                Status = RtlQueryRegistryValues(2,
                                                L"Session Manager",
                                                BasepAppCertTable,
                                                0,
                                                0);
                if (Status == 0xC0000034) Status = STATUS_SUCCESS;
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
                                      (PVOID)Handles->ViewBase.LowPart);
        ASSERT(NT_SUCCESS(Status));
    }
}

static
LONG BaseExceptionFilter(EXCEPTION_POINTERS *ExceptionInfo)
{
    LONG ExceptionDisposition = EXCEPTION_EXECUTE_HANDLER;
    LPTOP_LEVEL_EXCEPTION_FILTER RealFilter;
    RealFilter = RtlDecodePointer(GlobalTopLevelExceptionFilter);

    if (RealFilter != NULL)
    {
        _SEH2_TRY
        {
            ExceptionDisposition = RealFilter(ExceptionInfo);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
        }
        _SEH2_END;
    }
    if ((ExceptionDisposition == EXCEPTION_CONTINUE_SEARCH || ExceptionDisposition == EXCEPTION_EXECUTE_HANDLER) &&
        RealFilter != UnhandledExceptionFilter)
    {
       ExceptionDisposition = UnhandledExceptionFilter(ExceptionInfo);
    }

    return ExceptionDisposition;
}

VOID
WINAPI
BaseProcessStartup(PPROCESS_START_ROUTINE lpStartAddress)
{
    DPRINT("BaseProcessStartup(..) - setting up exception frame.\n");

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
    _SEH2_EXCEPT(BaseExceptionFilter(_SEH2_GetExceptionInformation()))
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

NTSTATUS
WINAPI
BasepNotifyCsrOfThread(IN HANDLE ThreadHandle,
                       IN PCLIENT_ID ClientId)
{
    NTSTATUS Status;
    BASE_API_MESSAGE ApiMessage;
    PBASE_CREATE_THREAD CreateThreadRequest = &ApiMessage.Data.CreateThreadRequest;

    DPRINT("BasepNotifyCsrOfThread: Thread: %lx, Handle %lx\n",
            ClientId->UniqueThread, ThreadHandle);

    /* Fill out the request */
    CreateThreadRequest->ClientId = *ClientId;
    CreateThreadRequest->ThreadHandle = ThreadHandle;

    /* Call CSR */
    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(BASESRV_SERVERDLL_INDEX, BasepCreateThread),
                                 sizeof(BASE_CREATE_THREAD));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to tell CSRSS about new thread: %lx\n", Status);
        return Status;
    }

    /* Return Success */
    return STATUS_SUCCESS;
}

/*
 * Creates the first Thread in a Proces
 */
HANDLE
WINAPI
BasepCreateFirstThread(HANDLE ProcessHandle,
                       LPSECURITY_ATTRIBUTES lpThreadAttributes,
                       PSECTION_IMAGE_INFORMATION SectionImageInfo,
                       PCLIENT_ID ClientId,
                       DWORD dwCreationFlags)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES LocalObjectAttributes;
    POBJECT_ATTRIBUTES ObjectAttributes;
    CONTEXT Context;
    INITIAL_TEB InitialTeb;
    HANDLE hThread;
    BASE_API_MESSAGE ApiMessage;
    PBASE_CREATE_PROCESS CreateProcessRequest = &ApiMessage.Data.CreateProcessRequest;

    DPRINT("BasepCreateFirstThread. hProcess: %lx\n", ProcessHandle);

    /* Create the Thread's Stack */
    BaseCreateStack(ProcessHandle,
                     SectionImageInfo->MaximumStackSize,
                     SectionImageInfo->CommittedStackSize,
                     &InitialTeb);

    /* Create the Thread's Context */
    BaseInitializeContext(&Context,
                           NtCurrentPeb(),
                           SectionImageInfo->TransferAddress,
                           InitialTeb.StackBase,
                           0);

    /* Convert the thread attributes */
    ObjectAttributes = BaseFormatObjectAttributes(&LocalObjectAttributes,
                                                    lpThreadAttributes,
                                                    NULL);

    /* Create the Kernel Thread Object */
    Status = NtCreateThread(&hThread,
                            THREAD_ALL_ACCESS,
                            ObjectAttributes,
                            ProcessHandle,
                            ClientId,
                            &Context,
                            &InitialTeb,
                            TRUE);
    if (!NT_SUCCESS(Status))
    {
        return NULL;
    }

    /* Fill out the request to notify CSRSS */
    CreateProcessRequest->ClientId = *ClientId;
    CreateProcessRequest->ProcessHandle = ProcessHandle;
    CreateProcessRequest->ThreadHandle = hThread;
    CreateProcessRequest->CreationFlags = dwCreationFlags;

    /*
     * For GUI applications we turn on the 2nd bit. This also allows
     * us to know whether or not this is a GUI or a TUI application.
     */
    if (IMAGE_SUBSYSTEM_WINDOWS_GUI == SectionImageInfo->SubSystemType)
    {
        CreateProcessRequest->ProcessHandle = (HANDLE)
            ((ULONG_PTR)CreateProcessRequest->ProcessHandle | 2);
    }

    /* Call CSR */
    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(BASESRV_SERVERDLL_INDEX, BasepCreateProcess),
                                 sizeof(BASE_CREATE_PROCESS));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to tell CSRSS about new process: %lx\n", Status);
        return NULL;
    }

    /* Success */
    return hThread;
}

/*
 * Converts ANSI to Unicode Environment
 */
PVOID
WINAPI
BasepConvertUnicodeEnvironment(OUT SIZE_T* EnvSize,
                               IN PVOID lpEnvironment)
{
    PCHAR pcScan;
    ANSI_STRING AnsiEnv;
    UNICODE_STRING UnicodeEnv;
    NTSTATUS Status;

    DPRINT("BasepConvertUnicodeEnvironment\n");

    /* Scan the environment to calculate its Unicode size */
    AnsiEnv.Buffer = pcScan = (PCHAR)lpEnvironment;
    while (*pcScan)
    {
        pcScan += strlen(pcScan) + 1;
    }

    /* Create our ANSI String */
    if (pcScan == (PCHAR)lpEnvironment)
    {
        AnsiEnv.Length = 2 * sizeof(CHAR);
    }
    else
    {

        AnsiEnv.Length = (USHORT)((ULONG_PTR)pcScan - (ULONG_PTR)lpEnvironment + sizeof(CHAR));
    }
    AnsiEnv.MaximumLength = AnsiEnv.Length + 1;

    /* Allocate memory for the Unicode Environment */
    UnicodeEnv.Buffer = NULL;
    *EnvSize = AnsiEnv.MaximumLength * sizeof(WCHAR);
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     (PVOID)&UnicodeEnv.Buffer,
                                     0,
                                     EnvSize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    /* Failure */
    if (!NT_SUCCESS(Status))
    {
        SetLastError(Status);
        *EnvSize = 0;
        return NULL;
    }

    /* Use the allocated size */
    UnicodeEnv.MaximumLength = (USHORT)*EnvSize;

    /* Convert */
    RtlAnsiStringToUnicodeString(&UnicodeEnv, &AnsiEnv, FALSE);
    return UnicodeEnv.Buffer;
}

/*
 * Converts a Win32 Priority Class to NT
 */
ULONG
WINAPI
BasepConvertPriorityClass(IN ULONG dwCreationFlags)
{
    ULONG ReturnClass;

    if(dwCreationFlags & IDLE_PRIORITY_CLASS)
    {
        ReturnClass = PROCESS_PRIORITY_CLASS_IDLE;
    }
    else if(dwCreationFlags & BELOW_NORMAL_PRIORITY_CLASS)
    {
        ReturnClass = PROCESS_PRIORITY_CLASS_BELOW_NORMAL;
    }
    else if(dwCreationFlags & NORMAL_PRIORITY_CLASS)
    {
        ReturnClass = PROCESS_PRIORITY_CLASS_NORMAL;
    }
    else if(dwCreationFlags & ABOVE_NORMAL_PRIORITY_CLASS)
    {
        ReturnClass = PROCESS_PRIORITY_CLASS_ABOVE_NORMAL;
    }
    else if(dwCreationFlags & HIGH_PRIORITY_CLASS)
    {
        ReturnClass = PROCESS_PRIORITY_CLASS_HIGH;
    }
    else if(dwCreationFlags & REALTIME_PRIORITY_CLASS)
    {
        /* Check for Privilege First */
        if (BasepIsRealtimeAllowed(TRUE))
        {
            ReturnClass = PROCESS_PRIORITY_CLASS_REALTIME;
        }
        else
        {
            ReturnClass = PROCESS_PRIORITY_CLASS_HIGH;
        }
    }
    else
    {
        ReturnClass = PROCESS_PRIORITY_CLASS_INVALID;
    }

    return ReturnClass;
}

/*
 * Duplicates a standard handle and writes it where requested.
 */
VOID
WINAPI
BasepDuplicateAndWriteHandle(IN HANDLE ProcessHandle,
                             IN HANDLE StandardHandle,
                             IN PHANDLE Address)
{
    NTSTATUS Status;
    HANDLE DuplicatedHandle;
    SIZE_T Dummy;

    DPRINT("BasepDuplicateAndWriteHandle. hProcess: %lx, Handle: %lx,"
           "Address: %p\n", ProcessHandle, StandardHandle, Address);

    /* Don't touch Console Handles */
    if (IsConsoleHandle(StandardHandle)) return;

    /* Duplicate the handle */
    Status = NtDuplicateObject(NtCurrentProcess(),
                               StandardHandle,
                               ProcessHandle,
                               &DuplicatedHandle,
                               DUPLICATE_SAME_ACCESS | DUPLICATE_SAME_ATTRIBUTES,
                               0,
                               0);
    if (NT_SUCCESS(Status))
    {
        /* Write it */
        NtWriteVirtualMemory(ProcessHandle,
                             Address,
                             &DuplicatedHandle,
                             sizeof(HANDLE),
                             &Dummy);
    }
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
        while ((ScanChar[0]) || (ScanChar[1])) ++ScanChar;
        ScanChar += (2 * sizeof(UNICODE_NULL));
        EnviroSize = (ULONG_PTR)ScanChar - (ULONG_PTR)lpEnvironment;

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

    /* Use Special Flags for BasepInitConsole in Kernel32 */
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

    /* See if the first 1MB should be reserved */
    if (ParameterFlags & 1)
    {
        ProcessParameters->Flags |= RTL_USER_PROCESS_PARAMETERS_RESERVE_1MB;
    }

    /* See if the first 16MB should be reserved */
    if (ParameterFlags & 2)
    {
        ProcessParameters->Flags |= RTL_USER_PROCESS_PARAMETERS_RESERVE_16MB;
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
    DPRINT1("Failure to create proecss parameters: %lx\n", Status);
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
                                       (PVOID)&ProcessInfo,
                                       sizeof(PROCESS_BASIC_INFORMATION),
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
    NTSTATUS Status;
    BASE_API_MESSAGE ApiMessage;
    PBASE_GET_PROCESS_SHUTDOWN_PARAMS GetShutdownParametersRequest = &ApiMessage.Data.GetShutdownParametersRequest;

    /* Ask CSRSS for shutdown information */
    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(BASESRV_SERVERDLL_INDEX, BasepGetProcessShutdownParam),
                                 sizeof(BASE_GET_PROCESS_SHUTDOWN_PARAMS));
    if (!NT_SUCCESS(Status))
    {
        /* Return the failure from CSRSS */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Get the data back */
    *lpdwLevel = GetShutdownParametersRequest->Level;
    *lpdwFlags = GetShutdownParametersRequest->Flags;
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
    NTSTATUS Status;
    BASE_API_MESSAGE ApiMessage;
    PBASE_SET_PROCESS_SHUTDOWN_PARAMS SetShutdownParametersRequest = &ApiMessage.Data.SetShutdownParametersRequest;

    /* Write the data into the CSRSS request and send it */
    SetShutdownParametersRequest->Level = dwLevel;
    SetShutdownParametersRequest->Flags = dwFlags;
    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(BASESRV_SERVERDLL_INDEX, BasepSetProcessShutdownParam),
                                 sizeof(BASE_SET_PROCESS_SHUTDOWN_PARAMS));
    if (!NT_SUCCESS(Status))
    {
        /* Return the failure from CSRSS */
        BaseSetLastNTError(Status);
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
                                       sizeof(QUOTA_LIMITS_EX),
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
                                       sizeof(PROCESS_BASIC_INFORMATION),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        /* We failed, was this because this is a VDM process? */
        if (BaseCheckForVDM(hProcess, lpExitCode) == TRUE) return TRUE;

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
                                       sizeof(PROCESS_BASIC_INFORMATION),
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
            StartupInfo->lpReserved = 0;
            StartupInfo->lpDesktop = 0;
            StartupInfo->lpTitle = 0;

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
                        if (!InterlockedCompareExchangePointer(&BaseAnsiStartupInfo,
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
                      IN SIZE_T dwSize)
{
    NTSTATUS Status;

    /* Call the native function */
    Status = NtFlushInstructionCache(hProcess, (PVOID)lpBaseAddress, dwSize);
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
        NtTerminateProcess(NULL, 0);

        /* Unload all DLLs */
        LdrShutdownProcess();

        /* Notify Base Server of process termination */
        ExitProcessRequest->uExitCode = uExitCode;
        CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                            NULL,
                            CSR_CREATE_API_NUMBER(BASESRV_SERVERDLL_INDEX, BasepExitProcess),
                            sizeof(BASE_EXIT_PROCESS));

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

    /* Convert to unicode and just exit normally if this failed */
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
                              OptionOkCancel,
                              &Response);

    /* Give the user a chance to abort */
    if ((NT_SUCCESS(Status)) && (Response == ResponseCancel)) return;

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
    /* On Checked builds, Windows gives you a nice little debugger UI */
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
    PROCESS_PRIORITY_CLASS PriorityClass;

    /* Query the kernel */
    Status = NtQueryInformationProcess(hProcess,
                                       ProcessPriorityClass,
                                       &PriorityClass,
                                       sizeof(PROCESS_PRIORITY_CLASS),
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
    return FALSE;
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
            if (!ProcessHandle) return 0;

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
                                       sizeof(ULONG),
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
                                       sizeof(ULONG),
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Copy the count and return sucecss */
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

        /* Close the handle and check if we suceeded */
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

/*
 * @implemented
 */
BOOL
WINAPI
CreateProcessInternalW(HANDLE hToken,
                       LPCWSTR lpApplicationName,
                       LPWSTR lpCommandLine,
                       LPSECURITY_ATTRIBUTES lpProcessAttributes,
                       LPSECURITY_ATTRIBUTES lpThreadAttributes,
                       BOOL bInheritHandles,
                       DWORD dwCreationFlags,
                       LPVOID lpEnvironment,
                       LPCWSTR lpCurrentDirectory,
                       LPSTARTUPINFOW lpStartupInfo,
                       LPPROCESS_INFORMATION lpProcessInformation,
                       PHANDLE hNewToken)
{
    NTSTATUS Status;
    PROCESS_PRIORITY_CLASS PriorityClass;
    BOOLEAN FoundQuotes = FALSE;
    BOOLEAN QuotesNeeded = FALSE;
    BOOLEAN CmdLineIsAppName = FALSE;
    UNICODE_STRING ApplicationName = { 0, 0, NULL };
    OBJECT_ATTRIBUTES LocalObjectAttributes;
    POBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE hSection = NULL, hProcess = NULL, hThread = NULL, hDebug = NULL;
    SECTION_IMAGE_INFORMATION SectionImageInfo;
    LPWSTR CurrentDirectory = NULL;
    LPWSTR CurrentDirectoryPart;
    PROCESS_BASIC_INFORMATION ProcessBasicInfo;
    STARTUPINFOW StartupInfo;
    ULONG Dummy;
    LPWSTR BatchCommandLine;
    ULONG CmdLineLength;
    UNICODE_STRING CommandLineString;
    PWCHAR Extension;
    LPWSTR QuotedCmdLine = NULL;
    LPWSTR ScanString;
    LPWSTR NullBuffer = NULL;
    LPWSTR NameBuffer = NULL;
    WCHAR SaveChar = 0;
    ULONG RetVal;
    UINT Error = 0;
    BOOLEAN SearchDone = FALSE;
    BOOLEAN Escape = FALSE;
    CLIENT_ID ClientId;
    PPEB OurPeb = NtCurrentPeb();
    PPEB RemotePeb;
    SIZE_T EnvSize = 0;
    BOOL Ret = FALSE;

    /* FIXME should process
     * HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options
     * key (see http://blogs.msdn.com/oldnewthing/archive/2005/12/19/505449.aspx)
     */

    DPRINT("CreateProcessW: lpApplicationName: %S lpCommandLine: %S"
           " lpEnvironment: %p lpCurrentDirectory: %S dwCreationFlags: %lx\n",
           lpApplicationName, lpCommandLine, lpEnvironment, lpCurrentDirectory,
           dwCreationFlags);

    /* Flags we don't handle yet */
    if (dwCreationFlags & CREATE_SEPARATE_WOW_VDM)
    {
        DPRINT1("CREATE_SEPARATE_WOW_VDM not handled\n");
    }
    if (dwCreationFlags & CREATE_SHARED_WOW_VDM)
    {
        DPRINT1("CREATE_SHARED_WOW_VDM not handled\n");
    }
    if (dwCreationFlags & CREATE_FORCEDOS)
    {
        DPRINT1("CREATE_FORCEDOS not handled\n");
    }

    /* Fail on this flag, it's only valid with the WithLogonW function */
    if (dwCreationFlags & CREATE_PRESERVE_CODE_AUTHZ_LEVEL)
    {
        DPRINT1("Invalid flag used\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* This combination is illegal (see MSDN) */
    if ((dwCreationFlags & (DETACHED_PROCESS | CREATE_NEW_CONSOLE)) ==
        (DETACHED_PROCESS | CREATE_NEW_CONSOLE))
    {
        DPRINT1("Invalid flag combo used\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Another illegal combo */
    if ((dwCreationFlags & (CREATE_SEPARATE_WOW_VDM | CREATE_SHARED_WOW_VDM)) ==
        (CREATE_SEPARATE_WOW_VDM | CREATE_SHARED_WOW_VDM))
    {
        DPRINT1("Invalid flag combo used\n");
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (lpCurrentDirectory)
    {
        if ((GetFileAttributesW(lpCurrentDirectory) == INVALID_FILE_ATTRIBUTES) ||
            !(GetFileAttributesW(lpCurrentDirectory) & FILE_ATTRIBUTE_DIRECTORY))
        {
            SetLastError(ERROR_DIRECTORY);
            return FALSE;
        }
    }

    /*
     * We're going to modify and mask out flags and stuff in lpStartupInfo,
     * so we'll use our own local copy for that.
     */
    StartupInfo = *lpStartupInfo;

    /* FIXME: Use default Separate/Shared VDM Flag */

    /* If we are inside a Job, use Separate VDM so it won't escape the Job */
    if (!(dwCreationFlags & CREATE_SEPARATE_WOW_VDM))
    {
        if (NtIsProcessInJob(NtCurrentProcess(), NULL))
        {
            /* Remove the shared flag and add the separate flag. */
            dwCreationFlags = (dwCreationFlags &~ CREATE_SHARED_WOW_VDM) |
                                                  CREATE_SEPARATE_WOW_VDM;
        }
    }

    /*
     * According to some sites, ShellExecuteEx uses an undocumented flag to
     * send private handle data (such as HMONITOR or HICON). See:
     * www.catch22.net/tuts/undoc01.asp. This implies that we can't use the
     * standard handles anymore since we'd be overwriting this private data
     */
    if ((StartupInfo.dwFlags & STARTF_USESTDHANDLES) &&
        (StartupInfo.dwFlags & (STARTF_USEHOTKEY | STARTF_SHELLPRIVATE)))
    {
        StartupInfo.dwFlags &= ~STARTF_USESTDHANDLES;
    }

    /* Start by zeroing out the fields */
    RtlZeroMemory(lpProcessInformation, sizeof(PROCESS_INFORMATION));

    /* Easy stuff first, convert the process priority class */
    PriorityClass.Foreground = FALSE;
    PriorityClass.PriorityClass = (UCHAR)BasepConvertPriorityClass(dwCreationFlags);

    if (lpCommandLine)
    {
        /* Search for escape sequences */
        ScanString = lpCommandLine;
        while (NULL != (ScanString = wcschr(ScanString, L'^')))
        {
            ScanString++;
            if (*ScanString == L'\"' || *ScanString == L'^' || *ScanString == L'\\')
            {
                Escape = TRUE;
                break;
            }
        }
    }

    /* Get the application name and do all the proper formating necessary */
GetAppName:
    /* See if we have an application name (oh please let us have one!) */
    if (!lpApplicationName)
    {
        /* The fun begins */
        NameBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                     0,
                                     MAX_PATH * sizeof(WCHAR));
        if (NameBuffer == NULL)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto Cleanup;
        }

        /* This is all we have to work with :( */
        lpApplicationName = lpCommandLine;

        /* Initialize our friends at the beginning */
        NullBuffer = (LPWSTR)lpApplicationName;
        ScanString = (LPWSTR)lpApplicationName;

        /* We will start by looking for a quote */
        if (*ScanString == L'\"')
        {
             /* That was quick */
             SearchDone = TRUE;

             /* Advance past quote */
             ScanString++;
             lpApplicationName = ScanString;

             /* Find the closing quote */
             while (*ScanString)
             {
                 if (*ScanString == L'\"' && *(ScanString - 1) != L'^')
                 {
                     /* Found it */
                     NullBuffer = ScanString;
                     FoundQuotes = TRUE;
                     break;
                 }

                 /* Keep looking */
                 ScanString++;
                 NullBuffer = ScanString;
             }
        }
        else
        {
            /* No quotes, so we'll be looking for white space */
        WhiteScan:
            /* Reset the pointer */
            lpApplicationName = lpCommandLine;

            /* Find whitespace of Tab */
            while (*ScanString)
            {
                if (*ScanString == ' ' || *ScanString == '\t')
                {
                    /* Found it */
                    NullBuffer = ScanString;
                    break;
                }

                /* Keep looking */
                ScanString++;
                NullBuffer = ScanString;
            }
        }

        /* Set the Null Buffer */
        SaveChar = *NullBuffer;
        *NullBuffer = UNICODE_NULL;

        /* Do a search for the file */
        DPRINT("Ready for SearchPathW: %S\n", lpApplicationName);
        RetVal = SearchPathW(NULL,
                             lpApplicationName,
                             L".exe",
                             MAX_PATH,
                             NameBuffer,
                             NULL) * sizeof(WCHAR);

        /* Did it find something? */
        if (RetVal)
        {
            /* Get file attributes */
            ULONG Attributes = GetFileAttributesW(NameBuffer);
            if (Attributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                /* Give it a length of 0 to fail, this was a directory. */
                RetVal = 0;
            }
            else
            {
                /* It's a file! */
                RetVal += sizeof(WCHAR);
            }
        }

        /* Now check if we have a file, and if the path size is OK */
        if (!RetVal || RetVal >= (MAX_PATH * sizeof(WCHAR)))
        {
            ULONG PathType;
            HANDLE hFile;

            /* We failed, try to get the Path Type */
            DPRINT("SearchPathW failed. Retval: %ld\n", RetVal);
            PathType = RtlDetermineDosPathNameType_U(lpApplicationName);

            /* If it's not relative, try to get the error */
            if (PathType != RtlPathTypeRelative)
            {
                /* This should fail, and give us a detailed LastError */
                hFile = CreateFileW(lpApplicationName,
                                    GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);

                /* Did it actually NOT fail? */
                if (hFile != INVALID_HANDLE_VALUE)
                {
                    /* Fake the error */
                    CloseHandle(hFile);
                    BaseSetLastNTError(STATUS_OBJECT_NAME_NOT_FOUND);
                }
            }
            else
            {
                /* Immediately set the error */
                BaseSetLastNTError(STATUS_OBJECT_NAME_NOT_FOUND);
            }

            /* Did we already fail once? */
            if (Error)
            {
                SetLastError(Error);
            }
            else
            {
                /* Not yet, cache it */
                Error = GetLastError();
            }

            /* Put back the command line */
            *NullBuffer = SaveChar;
            lpApplicationName = NameBuffer;

            /*
             * If the search isn't done and we still have cmdline
             * then start over. Ex: c:\ha ha ha\haha.exe
             */
            if (*ScanString && !SearchDone)
            {
                /* Move in the buffer */
                ScanString++;
                NullBuffer = ScanString;

                /* We will have to add a quote, since there is a space*/
                QuotesNeeded = TRUE;

                /* And we will also fake the fact we found one */
                FoundQuotes = TRUE;

                /* Start over */
                goto WhiteScan;
            }

            /* We totally failed */
            goto Cleanup;
        }

        /* Put back the command line */
        *NullBuffer = SaveChar;
        lpApplicationName = NameBuffer;
        DPRINT("SearchPathW suceeded (%ld): %S\n", RetVal, NameBuffer);
    }
    else if (!lpCommandLine || *lpCommandLine == UNICODE_NULL)
    {
        /* We have an app name (good!) but no command line */
        CmdLineIsAppName = TRUE;
        lpCommandLine = (LPWSTR)lpApplicationName;
    }

    /* At this point the name has been toyed with enough to be openable */
    Status = BasepMapFile(lpApplicationName, &hSection, &ApplicationName);

    /* Check for failure */
    if (!NT_SUCCESS(Status))
    {
        /* Could be a non-PE File */
        switch (Status)
        {
            /* Check if the Kernel tells us it's not even valid MZ */
            case STATUS_INVALID_IMAGE_NE_FORMAT:
            case STATUS_INVALID_IMAGE_PROTECT:
            case STATUS_INVALID_IMAGE_NOT_MZ:

#if 0
            /* If it's a DOS app, use VDM */
            if ((BasepCheckDosApp(&ApplicationName)))
            {
                DPRINT1("Launching VDM...\n");
                RtlFreeHeap(RtlGetProcessHeap(), 0, NameBuffer);
                RtlFreeHeap(RtlGetProcessHeap(), 0, ApplicationName.Buffer);
                return CreateProcessW(L"ntvdm.exe",
                                      (LPWSTR)((ULONG_PTR)lpApplicationName), /* FIXME: Buffer must be writable!!! */
                                      lpProcessAttributes,
                                      lpThreadAttributes,
                                      bInheritHandles,
                                      dwCreationFlags,
                                      lpEnvironment,
                                      lpCurrentDirectory,
                                      &StartupInfo,
                                      lpProcessInformation);
            }
#endif
            /* It's a batch file */
            Extension = &ApplicationName.Buffer[ApplicationName.Length /
                                                sizeof(WCHAR) - 4];

            /* Make sure the extensions are correct */
            if (_wcsnicmp(Extension, L".bat", 4) && _wcsnicmp(Extension, L".cmd", 4))
            {
                SetLastError(ERROR_BAD_EXE_FORMAT);
                return FALSE;
            }

            /* Calculate the length of the command line */
            CmdLineLength = wcslen(CMD_STRING) + wcslen(lpCommandLine) + 1;

            /* If we found quotes, then add them into the length size */
            if (CmdLineIsAppName || FoundQuotes) CmdLineLength += 2;
            CmdLineLength *= sizeof(WCHAR);

            /* Allocate space for the new command line */
            BatchCommandLine = RtlAllocateHeap(RtlGetProcessHeap(),
                                               0,
                                               CmdLineLength);
            if (BatchCommandLine == NULL)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto Cleanup;
            }

            /* Build it */
            wcscpy(BatchCommandLine, CMD_STRING);
            if (CmdLineIsAppName || FoundQuotes)
            {
                wcscat(BatchCommandLine, L"\"");
            }
            wcscat(BatchCommandLine, lpCommandLine);
            if (CmdLineIsAppName || FoundQuotes)
            {
                wcscat(BatchCommandLine, L"\"");
            }

            /* Create it as a Unicode String */
            RtlInitUnicodeString(&CommandLineString, BatchCommandLine);

            /* Set the command line to this */
            lpCommandLine = CommandLineString.Buffer;
            lpApplicationName = NULL;

            /* Free memory */
            RtlFreeHeap(RtlGetProcessHeap(), 0, ApplicationName.Buffer);
            ApplicationName.Buffer = NULL;
            goto GetAppName;
            break;

            case STATUS_INVALID_IMAGE_WIN_16:

                /* It's a Win16 Image, use VDM */
                DPRINT1("Launching VDM...\n");
                RtlFreeHeap(RtlGetProcessHeap(), 0, NameBuffer);
                RtlFreeHeap(RtlGetProcessHeap(), 0, ApplicationName.Buffer);
                return CreateProcessW(L"ntvdm.exe",
                                      (LPWSTR)((ULONG_PTR)lpApplicationName), /* FIXME: Buffer must be writable!!! */
                                      lpProcessAttributes,
                                      lpThreadAttributes,
                                      bInheritHandles,
                                      dwCreationFlags,
                                      lpEnvironment,
                                      lpCurrentDirectory,
                                      &StartupInfo,
                                      lpProcessInformation);

            case STATUS_OBJECT_NAME_NOT_FOUND:
            case STATUS_OBJECT_PATH_NOT_FOUND:
                BaseSetLastNTError(Status);
                goto Cleanup;

            default:
                /* Invalid Image Type */
                SetLastError(ERROR_BAD_EXE_FORMAT);
                goto Cleanup;
        }
    }

    /* Use our desktop if we didn't get any */
    if (!StartupInfo.lpDesktop)
    {
        StartupInfo.lpDesktop = OurPeb->ProcessParameters->DesktopInfo.Buffer;
    }

    /* FIXME: Check if Application is allowed to run */

    /* FIXME: Allow CREATE_SEPARATE only for WOW Apps, once we have that. */

    /* Get some information about the executable */
    Status = NtQuerySection(hSection,
                            SectionImageInformation,
                            &SectionImageInfo,
                            sizeof(SectionImageInfo),
                            NULL);
    if(!NT_SUCCESS(Status))
    {
        DPRINT1("Unable to get SectionImageInformation, status 0x%x\n", Status);
        BaseSetLastNTError(Status);
        goto Cleanup;
    }

    /* Don't execute DLLs */
    if (SectionImageInfo.ImageCharacteristics & IMAGE_FILE_DLL)
    {
        DPRINT1("Can't execute a DLL\n");
        SetLastError(ERROR_BAD_EXE_FORMAT);
        goto Cleanup;
    }

    /* FIXME: Check for Debugger */

    /* FIXME: Check if Machine Type and SubSys Version Match */

    /* We don't support POSIX or anything else for now */
    if (IMAGE_SUBSYSTEM_WINDOWS_GUI != SectionImageInfo.SubSystemType &&
        IMAGE_SUBSYSTEM_WINDOWS_CUI != SectionImageInfo.SubSystemType)
    {
        DPRINT1("Invalid subsystem %d\n", SectionImageInfo.SubSystemType);
        /*
         * Despite the name of the error code suggests, it corresponds to the
         * well-known "The %1 application cannot be run in Win32 mode" message.
         */
        SetLastError(ERROR_CHILD_NOT_COMPLETE);
        goto Cleanup;
    }

    if (IMAGE_SUBSYSTEM_WINDOWS_GUI == SectionImageInfo.SubSystemType)
    {
        /* Do not create a console for GUI applications */
        dwCreationFlags &= ~CREATE_NEW_CONSOLE;
        dwCreationFlags |= DETACHED_PROCESS;
    }

    /* Initialize the process object attributes */
    ObjectAttributes = BaseFormatObjectAttributes(&LocalObjectAttributes,
                                                    lpProcessAttributes,
                                                    NULL);

    /* Check if we're going to be debugged */
    if (dwCreationFlags & DEBUG_PROCESS)
    {
        /* FIXME: Set process flag */
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
            goto Cleanup;
        }

        /* Get the debug object */
        hDebug = DbgUiGetThreadDebugObject();

        /* Check if only this process will be debugged */
        if (dwCreationFlags & DEBUG_ONLY_THIS_PROCESS)
        {
            /* Set process flag */
            hDebug = (HANDLE)((ULONG_PTR)hDebug | 0x1);
        }
    }

    /* Create the Process */
    Status = NtCreateProcess(&hProcess,
                             PROCESS_ALL_ACCESS,
                             ObjectAttributes,
                             NtCurrentProcess(),
                             (BOOLEAN)bInheritHandles,
                             hSection,
                             hDebug,
                             NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Unable to create process, status 0x%x\n", Status);
        BaseSetLastNTError(Status);
        goto Cleanup;
    }

    if (PriorityClass.PriorityClass != PROCESS_PRIORITY_CLASS_INVALID)
    {
        /* Set new class */
        Status = NtSetInformationProcess(hProcess,
                                         ProcessPriorityClass,
                                         &PriorityClass,
                                         sizeof(PROCESS_PRIORITY_CLASS));
        if(!NT_SUCCESS(Status))
        {
            DPRINT1("Unable to set new process priority, status 0x%x\n", Status);
            BaseSetLastNTError(Status);
            goto Cleanup;
        }
    }

    /* Set Error Mode */
    if (dwCreationFlags & CREATE_DEFAULT_ERROR_MODE)
    {
        ULONG ErrorMode = SEM_FAILCRITICALERRORS;
        NtSetInformationProcess(hProcess,
                                ProcessDefaultHardErrorMode,
                                &ErrorMode,
                                sizeof(ULONG));
    }

    /* Convert the directory to a full path */
    if (lpCurrentDirectory)
    {
        /* Allocate a buffer */
        CurrentDirectory = RtlAllocateHeap(RtlGetProcessHeap(),
                                           0,
                                           (MAX_PATH + 1) * sizeof(WCHAR));
        if (CurrentDirectory == NULL)
        {
            DPRINT1("Cannot allocate memory for directory name\n");
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto Cleanup;
        }

        /* Get the length */
        if (GetFullPathNameW(lpCurrentDirectory,
                             MAX_PATH,
                             CurrentDirectory,
                             &CurrentDirectoryPart) > MAX_PATH)
        {
            DPRINT1("Directory name too long\n");
            SetLastError(ERROR_DIRECTORY);
            goto Cleanup;
        }
    }

    /* Insert quotes if needed */
    if (QuotesNeeded || CmdLineIsAppName)
    {
        /* Allocate a buffer */
        QuotedCmdLine = RtlAllocateHeap(RtlGetProcessHeap(),
                                        0,
                                        (wcslen(lpCommandLine) + 2 + 1) *
                                        sizeof(WCHAR));
        if (QuotedCmdLine == NULL)
        {
            DPRINT1("Cannot allocate memory for quoted command line\n");
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto Cleanup;
        }

        /* Copy the first quote */
        wcscpy(QuotedCmdLine, L"\"");

        /* Save a null char */
        if (QuotesNeeded)
        {
            SaveChar = *NullBuffer;
            *NullBuffer = UNICODE_NULL;
        }

        /* Add the command line and the finishing quote */
        wcscat(QuotedCmdLine, lpCommandLine);
        wcscat(QuotedCmdLine, L"\"");

        /* Add the null char */
        if (QuotesNeeded)
        {
            *NullBuffer = SaveChar;
            wcscat(QuotedCmdLine, NullBuffer);
        }

        DPRINT("Quoted CmdLine: %S\n", QuotedCmdLine);
    }

    if (Escape)
    {
        if (QuotedCmdLine == NULL)
        {
            QuotedCmdLine = RtlAllocateHeap(RtlGetProcessHeap(),
                                            0,
                                            (wcslen(lpCommandLine) + 1) * sizeof(WCHAR));
            if (QuotedCmdLine == NULL)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto Cleanup;
            }
            wcscpy(QuotedCmdLine, lpCommandLine);
        }

        ScanString = QuotedCmdLine;
        while (NULL != (ScanString = wcschr(ScanString, L'^')))
        {
            ScanString++;
            if (*ScanString == L'\"' || *ScanString == L'^' || *ScanString == L'\\')
            {
                memmove(ScanString-1, ScanString, wcslen(ScanString) * sizeof(WCHAR) + sizeof(WCHAR));
            }
        }
    }

    /* Get the Process Information */
    Status = NtQueryInformationProcess(hProcess,
                                       ProcessBasicInformation,
                                       &ProcessBasicInfo,
                                       sizeof(ProcessBasicInfo),
                                       NULL);

    /* Convert the environment */
    if(lpEnvironment && !(dwCreationFlags & CREATE_UNICODE_ENVIRONMENT))
    {
        lpEnvironment = BasepConvertUnicodeEnvironment(&EnvSize, lpEnvironment);
        if (!lpEnvironment) goto Cleanup;
    }

    /* Create Process Environment */
    RemotePeb = ProcessBasicInfo.PebBaseAddress;
    Ret = BasePushProcessParameters(0,
                                    hProcess,
                                    RemotePeb,
                                    (LPWSTR)lpApplicationName,
                                    CurrentDirectory,
                                    (QuotesNeeded || CmdLineIsAppName || Escape) ?
                                    QuotedCmdLine : lpCommandLine,
                                    lpEnvironment,
                                    &StartupInfo,
                                    dwCreationFlags,
                                    bInheritHandles,
                                    0,
                                    NULL,
                                    0);
    if (!Ret) goto Cleanup;

    /* Cleanup Environment */
    if (lpEnvironment && !(dwCreationFlags & CREATE_UNICODE_ENVIRONMENT))
    {
        RtlDestroyEnvironment(lpEnvironment);
    }

    /* Close the section */
    NtClose(hSection);
    hSection = NULL;

    /* Duplicate the handles if needed */
    if (!bInheritHandles && !(StartupInfo.dwFlags & STARTF_USESTDHANDLES) &&
        SectionImageInfo.SubSystemType == IMAGE_SUBSYSTEM_WINDOWS_CUI)
    {
        PRTL_USER_PROCESS_PARAMETERS RemoteParameters;

        /* Get the remote parameters */
        Status = NtReadVirtualMemory(hProcess,
                                     &RemotePeb->ProcessParameters,
                                     &RemoteParameters,
                                     sizeof(PVOID),
                                     NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to read memory\n");
            goto Cleanup;
        }

        /* Duplicate and write the handles */
        BasepDuplicateAndWriteHandle(hProcess,
                                     OurPeb->ProcessParameters->StandardInput,
                                     &RemoteParameters->StandardInput);
        BasepDuplicateAndWriteHandle(hProcess,
                                     OurPeb->ProcessParameters->StandardOutput,
                                     &RemoteParameters->StandardOutput);
        BasepDuplicateAndWriteHandle(hProcess,
                                     OurPeb->ProcessParameters->StandardError,
                                     &RemoteParameters->StandardError);
    }

    /* Create the first thread */
    DPRINT("Creating thread for process (EntryPoint = 0x%p)\n",
            SectionImageInfo.TransferAddress);
    hThread = BasepCreateFirstThread(hProcess,
                                     lpThreadAttributes,
                                     &SectionImageInfo,
                                     &ClientId,
                                     dwCreationFlags);

    if (hThread == NULL)
    {
        DPRINT1("Could not create Initial Thread\n");
        /* FIXME - set last error code */
        goto Cleanup;
    }

    if (!(dwCreationFlags & CREATE_SUSPENDED))
    {
        NtResumeThread(hThread, &Dummy);
    }

    /* Return Data */
    lpProcessInformation->dwProcessId = (DWORD)ClientId.UniqueProcess;
    lpProcessInformation->dwThreadId = (DWORD)ClientId.UniqueThread;
    lpProcessInformation->hProcess = hProcess;
    lpProcessInformation->hThread = hThread;
    DPRINT("hThread[%p]: %p inside hProcess[%p]: %p\n", hThread,
            ClientId.UniqueThread, ClientId.UniqueProcess, hProcess);
    hProcess = hThread = NULL;
    Ret = TRUE;

Cleanup:
    /* De-allocate heap strings */
    if (NameBuffer) RtlFreeHeap(RtlGetProcessHeap(), 0, NameBuffer);
    if (ApplicationName.Buffer)
        RtlFreeHeap(RtlGetProcessHeap(), 0, ApplicationName.Buffer);
    if (CurrentDirectory) RtlFreeHeap(RtlGetProcessHeap(), 0, CurrentDirectory);
    if (QuotedCmdLine) RtlFreeHeap(RtlGetProcessHeap(), 0, QuotedCmdLine);

    /* Kill any handles still alive */
    if (hSection) NtClose(hSection);
    if (hThread)
    {
        /* We don't know any more details than this */
        NtTerminateProcess(hProcess, STATUS_UNSUCCESSFUL);
        NtClose(hThread);
    }
    if (hProcess) NtClose(hProcess);

    /* Return Success */
    return Ret;
}

/*
 * @implemented
 */
BOOL
WINAPI
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
    return CreateProcessInternalW(0,
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
    PUNICODE_STRING CommandLine = NULL;
    UNICODE_STRING DummyString;
    UNICODE_STRING LiveCommandLine;
    UNICODE_STRING ApplicationName;
    UNICODE_STRING CurrentDirectory;
    BOOL bRetVal;
    STARTUPINFOW StartupInfo;

    DPRINT("dwCreationFlags %x, lpEnvironment %x, lpCurrentDirectory %x, "
            "lpStartupInfo %x, lpProcessInformation %x\n",
            dwCreationFlags, lpEnvironment, lpCurrentDirectory,
            lpStartupInfo, lpProcessInformation);

    /* Copy Startup Info */
    RtlMoveMemory(&StartupInfo, lpStartupInfo, sizeof(*lpStartupInfo));

    /* Initialize all strings to nothing */
    LiveCommandLine.Buffer = NULL;
    DummyString.Buffer = NULL;
    ApplicationName.Buffer = NULL;
    CurrentDirectory.Buffer = NULL;
    StartupInfo.lpDesktop = NULL;
    StartupInfo.lpReserved = NULL;
    StartupInfo.lpTitle = NULL;

    /* Convert the Command line */
    if (lpCommandLine)
    {
        /* If it's too long, then we'll have a problem */
        if ((strlen(lpCommandLine) + 1) * sizeof(WCHAR) <
            NtCurrentTeb()->StaticUnicodeString.MaximumLength)
        {
            /* Cache it in the TEB */
            CommandLine = Basep8BitStringToStaticUnicodeString(lpCommandLine);
        }
        else
        {
            /* Use a dynamic version */
            Basep8BitStringToDynamicUnicodeString(&LiveCommandLine,
                                                  lpCommandLine);
        }
    }
    else
    {
        /* The logic below will use CommandLine, so we must make it valid */
        CommandLine = &DummyString;
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
                                     LiveCommandLine.Buffer ?
                                     LiveCommandLine.Buffer : CommandLine->Buffer,
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
    RtlFreeUnicodeString(&LiveCommandLine);
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
    return CreateProcessInternalA(0,
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
