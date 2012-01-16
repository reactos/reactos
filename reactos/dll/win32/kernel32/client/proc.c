/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/proc/proc.c
 * PURPOSE:         Process functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ****************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

typedef INT (WINAPI *MessageBoxW_Proc) (HWND, LPCWSTR, LPCWSTR, UINT);

/* GLOBALS *******************************************************************/

static UNICODE_STRING CommandLineStringW;
static ANSI_STRING CommandLineStringA;
UNICODE_STRING BasePathVariableName = RTL_CONSTANT_STRING(L"PATH");

static BOOL bCommandLineInitialized = FALSE;

WaitForInputIdleType  lpfnGlobalRegisterWaitForInputIdle;

LPSTARTUPINFOA lpLocalStartupInfo = NULL;

VOID WINAPI
RegisterWaitForInputIdle(WaitForInputIdleType lpfnRegisterWaitForInputIdle);

PLDR_DATA_TABLE_ENTRY BasepExeLdrEntry;

#define CMD_STRING L"cmd /c "

extern __declspec(noreturn)
VOID
CALLBACK
ConsoleControlDispatcher(DWORD CodeAndFlag);

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
    },
    {}
};

PSAFER_REPLACE_PROCESS_THREAD_TOKENS g_SaferReplaceProcessThreadTokens;
HMODULE gSaferHandle = (HMODULE)-1;

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
    UINT uExitCode = 0;

    DPRINT("BaseProcessStartup(..) - setting up exception frame.\n");

    _SEH2_TRY
    {
        /* Set our Start Address */
        NtSetInformationThread(NtCurrentThread(),
                               ThreadQuerySetWin32StartAddress,
                               &lpStartAddress,
                               sizeof(PPROCESS_START_ROUTINE));

        /* Call the Start Routine */
        uExitCode = (lpStartAddress)();
    }
    _SEH2_EXCEPT(BaseExceptionFilter(_SEH2_GetExceptionInformation()))
    {
        /* Get the SEH Error */
        uExitCode = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* Exit the Process with our error */
    ExitProcess(uExitCode);
}

/*
 * Tells CSR that a new process was created
 */
NTSTATUS
WINAPI
BasepNotifyCsrOfCreation(ULONG dwCreationFlags,
                         IN HANDLE ProcessId,
                         IN BOOL InheritHandles)
{
    ULONG Request = CREATE_PROCESS;
    CSR_API_MESSAGE CsrRequest;
    NTSTATUS Status;

    DPRINT("BasepNotifyCsrOfCreation: Process: %lx, Flags %lx\n",
            ProcessId, dwCreationFlags);

    /* Fill out the request */
    CsrRequest.Data.CreateProcessRequest.NewProcessId = ProcessId;
    CsrRequest.Data.CreateProcessRequest.Flags = dwCreationFlags;
    CsrRequest.Data.CreateProcessRequest.bInheritHandles = InheritHandles;

    /* Call CSR */
    Status = CsrClientCallServer(&CsrRequest,
                                 NULL,
                                 MAKE_CSR_API(Request, CSR_NATIVE),
                                 sizeof(CSR_API_MESSAGE));
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(CsrRequest.Status))
    {
        DPRINT1("Failed to tell csrss about new process\n");
        return CsrRequest.Status;
    }

    /* Return Success */
    return STATUS_SUCCESS;
}

NTSTATUS
WINAPI
BasepNotifyCsrOfThread(IN HANDLE ThreadHandle,
                       IN PCLIENT_ID ClientId)
{
    ULONG Request = CREATE_THREAD;
    CSR_API_MESSAGE CsrRequest;
    NTSTATUS Status;

    DPRINT("BasepNotifyCsrOfThread: Thread: %lx, Handle %lx\n",
            ClientId->UniqueThread, ThreadHandle);

    /* Fill out the request */
    CsrRequest.Data.CreateThreadRequest.ClientId = *ClientId;
    CsrRequest.Data.CreateThreadRequest.ThreadHandle = ThreadHandle;

    /* Call CSR */
    Status = CsrClientCallServer(&CsrRequest,
                                 NULL,
                                 MAKE_CSR_API(Request, CSR_NATIVE),
                                 sizeof(CSR_API_MESSAGE));
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(CsrRequest.Status))
    {
        DPRINT1("Failed to tell csrss about new thread\n");
        return CsrRequest.Status;
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
                       PCLIENT_ID ClientId)
{
    OBJECT_ATTRIBUTES LocalObjectAttributes;
    POBJECT_ATTRIBUTES ObjectAttributes;
    CONTEXT Context;
    INITIAL_TEB InitialTeb;
    NTSTATUS Status;
    HANDLE hThread;

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

    Status = BasepNotifyCsrOfThread(hThread, ClientId);
    if (!NT_SUCCESS(Status))
    {
        ASSERT(FALSE);
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
    DPRINT("BasePushProcessParameters\n");

    /* Get the full path name */
    Size = GetFullPathNameW(ApplicationPathName,
                            MAX_PATH + 4,
                            FullPath,
                            &Remaining);
    if ((Size) && (Size <= (MAX_PATH + 4)))
    {
        /* Get the DLL Path */
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
    else
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
        Status = ZwAllocateVirtualMemory(ProcessHandle,
                                         (PVOID*)&ProcessParameters->Environment,
                                         0,
                                         &Size,
                                         MEM_COMMIT,
                                         PAGE_READWRITE);
        if (!NT_SUCCESS(Status)) goto FailPath;

        /* Write the Environment Block */
        Status = ZwWriteVirtualMemory(ProcessHandle,
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
    else if (CreationFlags & CREATE_NO_WINDOW)
    {
        ProcessParameters->ConsoleHandle = HANDLE_CREATE_NO_WINDOW;
    }
    else if (CreationFlags & CREATE_NEW_CONSOLE)
    {
        ProcessParameters->ConsoleHandle = HANDLE_CREATE_NEW_CONSOLE;
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
    ProcessParameters->Flags |= (NtCurrentPeb()->ProcessParameters->Flags &
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
    PRTL_USER_PROCESS_PARAMETERS Params;

    /* get command line */
    Params = NtCurrentPeb()->ProcessParameters;
    RtlNormalizeProcessParams (Params);

    /* initialize command line buffers */
    CommandLineStringW.Length = Params->CommandLine.Length;
    CommandLineStringW.MaximumLength = CommandLineStringW.Length + sizeof(WCHAR);
    CommandLineStringW.Buffer = RtlAllocateHeap(GetProcessHeap(),
                                                HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY,
                                                CommandLineStringW.MaximumLength);
    if (CommandLineStringW.Buffer == NULL)
    {
        return;
    }

    RtlInitAnsiString(&CommandLineStringA, NULL);

    /* Copy command line */
    RtlCopyUnicodeString(&CommandLineStringW,
                         &(Params->CommandLine));
    CommandLineStringW.Buffer[CommandLineStringW.Length / sizeof(WCHAR)] = 0;

    /* convert unicode string to ansi (or oem) */
    if (bIsFileApiAnsi)
        RtlUnicodeStringToAnsiString(&CommandLineStringA,
                                     &CommandLineStringW,
                                     TRUE);
    else
        RtlUnicodeStringToOemString(&CommandLineStringA,
                                    &CommandLineStringW,
                                    TRUE);

    CommandLineStringA.Buffer[CommandLineStringA.Length] = 0;

    bCommandLineInitialized = TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetProcessAffinityMask(HANDLE hProcess,
                       PDWORD_PTR lpProcessAffinityMask,
                       PDWORD_PTR lpSystemAffinityMask)
{
    PROCESS_BASIC_INFORMATION ProcessInfo;
    SYSTEM_BASIC_INFORMATION SystemInfo;
    NTSTATUS Status;

    Status = NtQuerySystemInformation(SystemBasicInformation,
                                      &SystemInfo,
                                      sizeof(SystemInfo),
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    Status = NtQueryInformationProcess(hProcess,
                                       ProcessBasicInformation,
                                       (PVOID)&ProcessInfo,
                                       sizeof(PROCESS_BASIC_INFORMATION),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    *lpProcessAffinityMask = (DWORD)ProcessInfo.AffinityMask;
    *lpSystemAffinityMask = (DWORD)SystemInfo.ActiveProcessorsAffinityMask;

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetProcessAffinityMask(HANDLE hProcess,
                       DWORD_PTR dwProcessAffinityMask)
{
    NTSTATUS Status;

    Status = NtSetInformationProcess(hProcess,
                                     ProcessAffinityMask,
                                     (PVOID)&dwProcessAffinityMask,
                                     sizeof(DWORD));
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
BOOL
WINAPI
GetProcessShutdownParameters(LPDWORD lpdwLevel,
                             LPDWORD lpdwFlags)
{
    CSR_API_MESSAGE CsrRequest;
    ULONG Request;
    NTSTATUS Status;

    Request = GET_SHUTDOWN_PARAMETERS;
    Status = CsrClientCallServer(&CsrRequest,
                                 NULL,
                                 MAKE_CSR_API(Request, CSR_NATIVE),
                                 sizeof(CSR_API_MESSAGE));
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(CsrRequest.Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    *lpdwLevel = CsrRequest.Data.GetShutdownParametersRequest.Level;
    *lpdwFlags = CsrRequest.Data.GetShutdownParametersRequest.Flags;

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetProcessShutdownParameters(DWORD dwLevel,
                             DWORD dwFlags)
{
    CSR_API_MESSAGE CsrRequest;
    ULONG Request;
    NTSTATUS Status;

    CsrRequest.Data.SetShutdownParametersRequest.Level = dwLevel;
    CsrRequest.Data.SetShutdownParametersRequest.Flags = dwFlags;

    Request = SET_SHUTDOWN_PARAMETERS;
    Status = CsrClientCallServer(&CsrRequest,
                                 NULL,
                                 MAKE_CSR_API(Request, CSR_NATIVE),
                                 sizeof(CSR_API_MESSAGE));
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(CsrRequest.Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetProcessWorkingSetSize(HANDLE hProcess,
                         PSIZE_T lpMinimumWorkingSetSize,
                         PSIZE_T lpMaximumWorkingSetSize)
{
    QUOTA_LIMITS QuotaLimits;
    NTSTATUS Status;

    Status = NtQueryInformationProcess(hProcess,
                                       ProcessQuotaLimits,
                                       &QuotaLimits,
                                       sizeof(QUOTA_LIMITS),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    *lpMinimumWorkingSetSize = QuotaLimits.MinimumWorkingSetSize;
    *lpMaximumWorkingSetSize = QuotaLimits.MaximumWorkingSetSize;

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetProcessWorkingSetSize(HANDLE hProcess,
                         SIZE_T dwMinimumWorkingSetSize,
                         SIZE_T dwMaximumWorkingSetSize)
{
    QUOTA_LIMITS QuotaLimits;
    NTSTATUS Status;

    QuotaLimits.MinimumWorkingSetSize = dwMinimumWorkingSetSize;
    QuotaLimits.MaximumWorkingSetSize = dwMaximumWorkingSetSize;

    Status = NtSetInformationProcess(hProcess,
                                     ProcessQuotaLimits,
                                     &QuotaLimits,
                                     sizeof(QUOTA_LIMITS));
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
BOOL
WINAPI
GetProcessTimes(HANDLE hProcess,
                LPFILETIME lpCreationTime,
                LPFILETIME lpExitTime,
                LPFILETIME lpKernelTime,
                LPFILETIME lpUserTime)
{
    KERNEL_USER_TIMES Kut;
    NTSTATUS Status;

    Status = NtQueryInformationProcess(hProcess,
                                       ProcessTimes,
                                       &Kut,
                                       sizeof(Kut),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

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
    return HandleToUlong(GetTeb()->ClientId.UniqueProcess);
}


/*
 * @implemented
 */
BOOL
WINAPI
GetExitCodeProcess(HANDLE hProcess,
                   LPDWORD lpExitCode)
{
    PROCESS_BASIC_INFORMATION ProcessBasic;
    NTSTATUS Status;

    Status = NtQueryInformationProcess(hProcess,
                                       ProcessBasicInformation,
                                       &ProcessBasic,
                                       sizeof(PROCESS_BASIC_INFORMATION),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    *lpExitCode = (DWORD)ProcessBasic.ExitStatus;

    return TRUE;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetProcessId(HANDLE Process)
{
    PROCESS_BASIC_INFORMATION ProcessBasic;
    NTSTATUS Status;

    Status = NtQueryInformationProcess(Process,
                                       ProcessBasicInformation,
                                       &ProcessBasic,
                                       sizeof(PROCESS_BASIC_INFORMATION),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return 0;
    }

    return (DWORD)ProcessBasic.UniqueProcessId;
}


/*
 * @implemented
 */
HANDLE
WINAPI
OpenProcess(DWORD dwDesiredAccess,
            BOOL bInheritHandle,
            DWORD dwProcessId)
{
    NTSTATUS errCode;
    HANDLE ProcessHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    CLIENT_ID ClientId;

    ClientId.UniqueProcess = UlongToHandle(dwProcessId);
    ClientId.UniqueThread = 0;

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               (bInheritHandle ? OBJ_INHERIT : 0),
                               NULL,
                               NULL);

    errCode = NtOpenProcess(&ProcessHandle,
                            dwDesiredAccess,
                            &ObjectAttributes,
                            &ClientId);
    if (!NT_SUCCESS(errCode))
    {
        BaseSetLastNTError(errCode);
        return NULL;
    }

    return ProcessHandle;
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

    if (NULL != lpfnGlobalRegisterWaitForInputIdle)
    {
        lpfnGlobalRegisterWaitForInputIdle(ProcessInformation.hProcess,
                                           10000);
    }

    NtClose(ProcessInformation.hProcess);
    NtClose(ProcessInformation.hThread);

    return 33; /* Something bigger than 31 means success. */
}


/*
 * @implemented
 */
VOID
WINAPI
RegisterWaitForInputIdle(WaitForInputIdleType lpfnRegisterWaitForInputIdle)
{
    lpfnGlobalRegisterWaitForInputIdle = lpfnRegisterWaitForInputIdle;
    return;
}

/*
 * @implemented
 */
VOID
WINAPI
GetStartupInfoW(LPSTARTUPINFOW lpStartupInfo)
{
    PRTL_USER_PROCESS_PARAMETERS Params;

    if (lpStartupInfo == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return;
    }

    Params = NtCurrentPeb()->ProcessParameters;

    lpStartupInfo->cb = sizeof(STARTUPINFOW);
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

    lpStartupInfo->hStdInput = Params->StandardInput;
    lpStartupInfo->hStdOutput = Params->StandardOutput;
    lpStartupInfo->hStdError = Params->StandardError;
}


/*
 * @implemented
 */
VOID
WINAPI
GetStartupInfoA(LPSTARTUPINFOA lpStartupInfo)
{
    PRTL_USER_PROCESS_PARAMETERS Params;
    ANSI_STRING AnsiString;

    if (lpStartupInfo == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return;
    }

    Params = NtCurrentPeb ()->ProcessParameters;

    RtlAcquirePebLock ();

    /* FIXME - not thread-safe */
    if (lpLocalStartupInfo == NULL)
    {
        /* create new local startup info (ansi) */
        lpLocalStartupInfo = RtlAllocateHeap(RtlGetProcessHeap(),
                                             0,
                                             sizeof(STARTUPINFOA));
        if (lpLocalStartupInfo == NULL)
        {
            RtlReleasePebLock();
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return;
        }

        lpLocalStartupInfo->cb = sizeof(STARTUPINFOA);

        /* copy window title string */
        RtlUnicodeStringToAnsiString(&AnsiString,
                                     &Params->WindowTitle,
                                     TRUE);
        lpLocalStartupInfo->lpTitle = AnsiString.Buffer;

        /* copy desktop info string */
        RtlUnicodeStringToAnsiString(&AnsiString,
                                     &Params->DesktopInfo,
                                     TRUE);
        lpLocalStartupInfo->lpDesktop = AnsiString.Buffer;

        /* copy shell info string */
        RtlUnicodeStringToAnsiString(&AnsiString,
                                     &Params->ShellInfo,
                                     TRUE);
        lpLocalStartupInfo->lpReserved = AnsiString.Buffer;

        lpLocalStartupInfo->dwX = Params->StartingX;
        lpLocalStartupInfo->dwY = Params->StartingY;
        lpLocalStartupInfo->dwXSize = Params->CountX;
        lpLocalStartupInfo->dwYSize = Params->CountY;
        lpLocalStartupInfo->dwXCountChars = Params->CountCharsX;
        lpLocalStartupInfo->dwYCountChars = Params->CountCharsY;
        lpLocalStartupInfo->dwFillAttribute = Params->FillAttribute;
        lpLocalStartupInfo->dwFlags = Params->WindowFlags;
        lpLocalStartupInfo->wShowWindow = (WORD)Params->ShowWindowFlags;
        lpLocalStartupInfo->cbReserved2 = Params->RuntimeData.Length;
        lpLocalStartupInfo->lpReserved2 = (LPBYTE)Params->RuntimeData.Buffer;

        lpLocalStartupInfo->hStdInput = Params->StandardInput;
        lpLocalStartupInfo->hStdOutput = Params->StandardOutput;
        lpLocalStartupInfo->hStdError = Params->StandardError;
    }

    RtlReleasePebLock();

    /* copy local startup info data to external startup info */
    memcpy(lpStartupInfo,
           lpLocalStartupInfo,
           sizeof(STARTUPINFOA));
}


/*
 * @implemented
 */
BOOL
WINAPI
FlushInstructionCache(HANDLE hProcess,
                      LPCVOID lpBaseAddress,
                      SIZE_T dwSize)
{
    NTSTATUS Status;

    Status = NtFlushInstructionCache(hProcess,
                                     (PVOID)lpBaseAddress,
                                     dwSize);
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
VOID
WINAPI
ExitProcess(UINT uExitCode)
{
    CSR_API_MESSAGE CsrRequest;
    ULONG Request;
    NTSTATUS Status;

    /* kill sibling threads ... we want to be alone at this point */
    NtTerminateProcess(NULL, 0);

    /* unload all dll's */
    LdrShutdownProcess();

    /* notify csrss of process termination */
    Request = TERMINATE_PROCESS;
    Status = CsrClientCallServer(&CsrRequest,
                                 NULL,
                                 MAKE_CSR_API(Request, CSR_NATIVE),
                                 sizeof(CSR_API_MESSAGE));
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(CsrRequest.Status))
    {
        DPRINT("Failed to tell csrss about terminating process\n");
    }

    NtTerminateProcess(NtCurrentProcess (),
                       uExitCode);

    /* should never get here */
    ASSERT(0);
    while(1);
}


/*
 * @implemented
 */
BOOL
WINAPI
TerminateProcess(HANDLE hProcess,
                 UINT uExitCode)
{
    NTSTATUS Status;

    if (hProcess == NULL)
    {
      return FALSE;
    }

    Status = NtTerminateProcess(hProcess, uExitCode);
    if (NT_SUCCESS(Status))
    {
        return TRUE;
    }

    BaseSetLastNTError(Status);
    return FALSE;
}


/*
 * @unimplemented
 */
VOID
WINAPI
FatalAppExitA(UINT uAction,
              LPCSTR lpMessageText)
{
    UNICODE_STRING MessageTextU;
    ANSI_STRING MessageText;

    RtlInitAnsiString(&MessageText, (LPSTR)lpMessageText);

    RtlAnsiStringToUnicodeString(&MessageTextU,
                                 &MessageText,
                                 TRUE);

    FatalAppExitW(uAction, MessageTextU.Buffer);

    RtlFreeUnicodeString(&MessageTextU);
}


/*
 * @unimplemented
 */
VOID
WINAPI
FatalAppExitW(UINT uAction,
              LPCWSTR lpMessageText)
{
    static const WCHAR szUser32[] = L"user32.dll\0";

    HMODULE hModule = GetModuleHandleW(szUser32);
    MessageBoxW_Proc pMessageBoxW = NULL;

    DPRINT1("AppExit\n");

    if (hModule)
        pMessageBoxW = (MessageBoxW_Proc)GetProcAddress(hModule, "MessageBoxW");

    if (pMessageBoxW)
        pMessageBoxW(0, lpMessageText, NULL, MB_SYSTEMMODAL | MB_OK);
    else
        DPRINT1("%s\n", lpMessageText);

    ExitProcess(0);
}


/*
 * @implemented
 */
VOID
WINAPI
FatalExit(int ExitCode)
{
    ExitProcess(ExitCode);
}


/*
 * @implemented
 */
DWORD
WINAPI
GetPriorityClass(HANDLE hProcess)
{
  NTSTATUS Status;
  PROCESS_PRIORITY_CLASS PriorityClass;

  Status = NtQueryInformationProcess(hProcess,
                                     ProcessPriorityClass,
                                     &PriorityClass,
                                     sizeof(PROCESS_PRIORITY_CLASS),
                                     NULL);
  if(NT_SUCCESS(Status))
  {
    switch(PriorityClass.PriorityClass)
    {
      case PROCESS_PRIORITY_CLASS_IDLE:
        return IDLE_PRIORITY_CLASS;

      case PROCESS_PRIORITY_CLASS_BELOW_NORMAL:
        return BELOW_NORMAL_PRIORITY_CLASS;

      case PROCESS_PRIORITY_CLASS_NORMAL:
        return NORMAL_PRIORITY_CLASS;

      case PROCESS_PRIORITY_CLASS_ABOVE_NORMAL:
        return ABOVE_NORMAL_PRIORITY_CLASS;

      case PROCESS_PRIORITY_CLASS_HIGH:
        return HIGH_PRIORITY_CLASS;

      case PROCESS_PRIORITY_CLASS_REALTIME:
        return REALTIME_PRIORITY_CLASS;

      default:
        return NORMAL_PRIORITY_CLASS;
    }
  }

  BaseSetLastNTError(Status);
  return FALSE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetPriorityClass(HANDLE hProcess,
                 DWORD dwPriorityClass)
{
    NTSTATUS Status;
    PROCESS_PRIORITY_CLASS PriorityClass;

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
            PriorityClass.PriorityClass = PROCESS_PRIORITY_CLASS_REALTIME;
            break;

        default:
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
    }

    PriorityClass.Foreground = FALSE;

    Status = NtSetInformationProcess(hProcess,
                                     ProcessPriorityClass,
                                     &PriorityClass,
                                     sizeof(PROCESS_PRIORITY_CLASS));
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
DWORD
WINAPI
GetProcessVersion(DWORD ProcessId)
{
    DWORD Version = 0;
    PIMAGE_NT_HEADERS NtHeader = NULL;
    IMAGE_NT_HEADERS NtHeaders;
    IMAGE_DOS_HEADER DosHeader;
    PROCESS_BASIC_INFORMATION ProcessBasicInfo;
    PVOID BaseAddress = NULL;
    HANDLE ProcessHandle = NULL;
    NTSTATUS Status;
    SIZE_T Count;
    PEB Peb;

    _SEH2_TRY
    {
        if (0 == ProcessId || GetCurrentProcessId() == ProcessId)
        {
            /* Caller's */
            BaseAddress = (PVOID) NtCurrentPeb()->ImageBaseAddress;
            NtHeader = RtlImageNtHeader(BaseAddress);

            Version = (NtHeader->OptionalHeader.MajorOperatingSystemVersion << 16) |
                      (NtHeader->OptionalHeader.MinorOperatingSystemVersion);
        }
        else
        {
            /* Other process */
            ProcessHandle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
                                        FALSE,
                                        ProcessId);

            if (!ProcessHandle) return 0;

            Status = NtQueryInformationProcess(ProcessHandle,
                                               ProcessBasicInformation,
                                               &ProcessBasicInfo,
                                               sizeof(ProcessBasicInfo),
                                               NULL);

            if (!NT_SUCCESS(Status)) goto Error;

            Status = NtReadVirtualMemory(ProcessHandle,
                                         ProcessBasicInfo.PebBaseAddress,
                                         &Peb,
                                         sizeof(Peb),
                                         &Count);

            if (!NT_SUCCESS(Status) || Count != sizeof(Peb)) goto Error;

            memset(&DosHeader, 0, sizeof(DosHeader));
            Status = NtReadVirtualMemory(ProcessHandle,
                                         Peb.ImageBaseAddress,
                                         &DosHeader,
                                         sizeof(DosHeader),
                                         &Count);

            if (!NT_SUCCESS(Status) || Count != sizeof(DosHeader)) goto Error;
            if (DosHeader.e_magic != IMAGE_DOS_SIGNATURE) goto Error;

            memset(&NtHeaders, 0, sizeof(NtHeaders));
            Status = NtReadVirtualMemory(ProcessHandle,
                                         (char *)Peb.ImageBaseAddress + DosHeader.e_lfanew,
                                         &NtHeaders,
                                         sizeof(NtHeaders),
                                         &Count);

            if (!NT_SUCCESS(Status) || Count != sizeof(NtHeaders)) goto Error;
            if (NtHeaders.Signature != IMAGE_NT_SIGNATURE) goto Error;

            Version = MAKELONG(NtHeaders.OptionalHeader.MinorSubsystemVersion,
                               NtHeaders.OptionalHeader.MajorSubsystemVersion);

Error:
            if (!NT_SUCCESS(Status))
            {
                BaseSetLastNTError(Status);
            }
        }
    }
    _SEH2_FINALLY
    {
        if (ProcessHandle) CloseHandle(ProcessHandle);
    }
    _SEH2_END;

    return Version;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetProcessIoCounters(HANDLE hProcess,
                     PIO_COUNTERS lpIoCounters)
{
    NTSTATUS Status;

    Status = NtQueryInformationProcess(hProcess,
                                       ProcessIoCounters,
                                       lpIoCounters,
                                       sizeof(IO_COUNTERS),
                                       NULL);
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
BOOL
WINAPI
GetProcessPriorityBoost(HANDLE hProcess,
                        PBOOL pDisablePriorityBoost)
{
    NTSTATUS Status;
    ULONG PriorityBoost;

    Status = NtQueryInformationProcess(hProcess,
                                       ProcessPriorityBoost,
                                       &PriorityBoost,
                                       sizeof(ULONG),
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        *pDisablePriorityBoost = PriorityBoost;
        return TRUE;
    }

    BaseSetLastNTError(Status);
    return FALSE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetProcessPriorityBoost(HANDLE hProcess,
                        BOOL bDisablePriorityBoost)
{
    NTSTATUS Status;
    ULONG PriorityBoost = (bDisablePriorityBoost ? TRUE : FALSE); /* prevent setting values other than 1 and 0 */

    Status = NtSetInformationProcess(hProcess,
                                     ProcessPriorityBoost,
                                     &PriorityBoost,
                                     sizeof(ULONG));
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
BOOL
WINAPI
GetProcessHandleCount(HANDLE hProcess,
                      PDWORD pdwHandleCount)
{
    ULONG phc;
    NTSTATUS Status;

    Status = NtQueryInformationProcess(hProcess,
                                       ProcessHandleCount,
                                       &phc,
                                       sizeof(ULONG),
                                       NULL);
    if(NT_SUCCESS(Status))
    {
      *pdwHandleCount = phc;
      return TRUE;
    }

    BaseSetLastNTError(Status);
    return FALSE;
}


/*
 * @implemented
 */
BOOL
WINAPI
IsWow64Process(HANDLE hProcess,
               PBOOL Wow64Process)
{
    ULONG_PTR pbi;
    NTSTATUS Status;

    Status = NtQueryInformationProcess(hProcess,
                                       ProcessWow64Information,
                                       &pbi,
                                       sizeof(pbi),
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

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
    DPRINT("CommandLine \'%s\'\n", CommandLineStringA.Buffer);
    return CommandLineStringA.Buffer;
}


/*
 * @implemented
 */
LPWSTR
WINAPI
GetCommandLineW(VOID)
{
    DPRINT("CommandLine \'%S\'\n", CommandLineStringW.Buffer);
    return CommandLineStringW.Buffer;
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
                                 lpNumberOfBytesRead);
    if (!NT_SUCCESS(Status))
    {
        /* We failed */
        BaseSetLastNTError (Status);
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
                                          lpNumberOfBytesWritten);
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
            if ((OldValue & PAGE_NOACCESS) || (OldValue & PAGE_READONLY))
            {
                /* Restore protection and fail */
                NtProtectVirtualMemory(hProcess,
                                       &Base,
                                       &RegionSize,
                                       OldValue,
                                       &OldValue);
                BaseSetLastNTError(STATUS_ACCESS_VIOLATION);
                return FALSE;
            }

            /* Otherwise, do the write */
            Status = NtWriteVirtualMemory(hProcess,
                                          lpBaseAddress,
                                          (LPVOID)lpBuffer,
                                          nSize,
                                          lpNumberOfBytesWritten);

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
                return FALSE;
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
                     OUT DWORD *pSessionId)
{
    PROCESS_SESSION_INFORMATION SessionInformation;
    OBJECT_ATTRIBUTES ObjectAttributes;
    CLIENT_ID ClientId;
    HANDLE ProcessHandle;
    NTSTATUS Status;

    if (IsBadWritePtr(pSessionId, sizeof(DWORD)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ClientId.UniqueProcess = UlongToHandle(dwProcessId);
    ClientId.UniqueThread = 0;

    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);

    Status = NtOpenProcess(&ProcessHandle,
                           PROCESS_QUERY_INFORMATION,
                           &ObjectAttributes,
                           &ClientId);
    if (NT_SUCCESS(Status))
    {
        Status = NtQueryInformationProcess(ProcessHandle,
                                           ProcessSessionInformation,
                                           &SessionInformation,
                                           sizeof(SessionInformation),
                                           NULL);
        NtClose(ProcessHandle);

        if (NT_SUCCESS(Status))
        {
            *pSessionId = SessionInformation.SessionId;
            return TRUE;
        }
    }

    BaseSetLastNTError(Status);
    return FALSE;
}

BOOL
WINAPI
SetProcessWorkingSetSizeEx(IN HANDLE hProcess,
                           IN SIZE_T dwMinimumWorkingSetSize,
                           IN SIZE_T dwMaximumWorkingSetSize,
                           IN DWORD Flags)
{
    STUB;
    return FALSE;
}


BOOL
WINAPI
GetProcessWorkingSetSizeEx(IN HANDLE hProcess,
                           OUT PSIZE_T lpMinimumWorkingSetSize,
                           OUT PSIZE_T lpMaximumWorkingSetSize,
                           OUT PDWORD Flags)
{
    STUB;
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
            if (*ScanString == L'\"' || *ScanString == L'^' || *ScanString == L'\"')
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
    Status = ZwQuerySection(hSection,
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
        SetLastError(ERROR_BAD_EXE_FORMAT);
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
            /* FIXME: Set process flag */
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

    /* Notify CSRSS */
    Status = BasepNotifyCsrOfCreation(dwCreationFlags,
                                      (HANDLE)ProcessBasicInfo.UniqueProcessId,
                                      bInheritHandles);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSR Notification Failed\n");
        BaseSetLastNTError(Status);
        goto Cleanup;
    }

    /* Create the first thread */
    DPRINT("Creating thread for process (EntryPoint = 0x%p)\n",
            SectionImageInfo.TransferAddress);
    hThread = BasepCreateFirstThread(hProcess,
                                     lpThreadAttributes,
                                     &SectionImageInfo,
                                     &ClientId);

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
        /* We don't know any more details then this */
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

/* EOF */
