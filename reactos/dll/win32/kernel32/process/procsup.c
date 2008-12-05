/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/process/create.c
 * PURPOSE:         Process functions
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 *                  Ariadne ( ariadne@xs4all.nl)
 */

/* INCLUDES ****************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

#define CMD_STRING L"cmd /c "

extern __declspec(noreturn)
VOID
CALLBACK
ConsoleControlDispatcher(DWORD CodeAndFlag);

/* INTERNAL FUNCTIONS *******************************************************/

static
LONG BaseExceptionFilter(EXCEPTION_POINTERS *ExceptionInfo)
{
    LONG ExceptionDisposition = EXCEPTION_EXECUTE_HANDLER;

    if (GlobalTopLevelExceptionFilter != NULL)
    {
        _SEH2_TRY
        {
            ExceptionDisposition = GlobalTopLevelExceptionFilter(ExceptionInfo);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
        }
        _SEH2_END;
    }
    if ((ExceptionDisposition == EXCEPTION_CONTINUE_SEARCH || ExceptionDisposition == EXCEPTION_EXECUTE_HANDLER) &&
        GlobalTopLevelExceptionFilter != UnhandledExceptionFilter)
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

    /* REturn Success */
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
    BasepCreateStack(ProcessHandle,
                     SectionImageInfo->MaximumStackSize,
                     SectionImageInfo->CommittedStackSize,
                     &InitialTeb);

    /* Create the Thread's Context */
    BasepInitializeContext(&Context,
                           NtCurrentPeb(),
                           SectionImageInfo->TransferAddress,
                           InitialTeb.StackBase,
                           0);

    /* Convert the thread attributes */
    ObjectAttributes = BasepConvertObjectAttributes(&LocalObjectAttributes,
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
        if (BasepCheckRealTimePrivilege())
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

LPWSTR
WINAPI
BasepGetDllPath(LPWSTR FullPath,
                PVOID Environment)
{
    /* FIXME: Not yet implemented */
    return NULL;
}

VOID
WINAPI
BasepCopyHandles(IN PRTL_USER_PROCESS_PARAMETERS Params,
                 IN PRTL_USER_PROCESS_PARAMETERS PebParams,
                 IN BOOL InheritHandles)
{
    DPRINT("BasepCopyHandles %p %p, %d\n", Params, PebParams, InheritHandles);

    /* Copy the handle if we are inheriting or if it's a console handle */
    if (InheritHandles || IsConsoleHandle(PebParams->StandardInput))
    {
        Params->StandardInput = PebParams->StandardInput;
    }
    if (InheritHandles || IsConsoleHandle(PebParams->StandardOutput))
    {
        Params->StandardOutput = PebParams->StandardOutput;
    }
    if (InheritHandles || IsConsoleHandle(PebParams->StandardError))
    {
        Params->StandardError = PebParams->StandardError;
    }
}

NTSTATUS
WINAPI
BasepInitializeEnvironment(HANDLE ProcessHandle,
                           PPEB Peb,
                           LPWSTR ApplicationPathName,
                           LPWSTR lpCurrentDirectory,
                           LPWSTR lpCommandLine,
                           LPVOID lpEnvironment,
                           SIZE_T EnvSize,
                           LPSTARTUPINFOW StartupInfo,
                           DWORD CreationFlags,
                           BOOL InheritHandles)
{
    WCHAR FullPath[MAX_PATH];
    LPWSTR Remaining;
    LPWSTR DllPathString;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    PRTL_USER_PROCESS_PARAMETERS RemoteParameters = NULL;
    UNICODE_STRING DllPath, ImageName, CommandLine, CurrentDirectory;
    UINT RetVal;
    NTSTATUS Status;
    PWCHAR ScanChar;
    ULONG EnviroSize;
    SIZE_T Size;
    UNICODE_STRING Desktop, Shell, Runtime, Title;
    PPEB OurPeb = NtCurrentPeb();
    LPVOID Environment = lpEnvironment;

    DPRINT("BasepInitializeEnvironment\n");

    /* Get the full path name */
    RetVal = GetFullPathNameW(ApplicationPathName,
                              MAX_PATH,
                              FullPath,
                              &Remaining);
    DPRINT("ApplicationPathName: %S, FullPath: %S\n", ApplicationPathName,
            FullPath);

    /* Get the DLL Path */
    DllPathString = BasepGetDllPath(FullPath, Environment);

    /* Initialize Strings */
    RtlInitUnicodeString(&DllPath, DllPathString);
    RtlInitUnicodeString(&ImageName, FullPath);
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
        RtlInitUnicodeString(&Title, L"");
    }

    /* This one is special because the length can differ */
    Runtime.Buffer = (LPWSTR)StartupInfo->lpReserved2;
    Runtime.MaximumLength = Runtime.Length = StartupInfo->cbReserved2;

    /* Create the Parameter Block */
    DPRINT("Creating Process Parameters: %wZ %wZ %wZ %wZ %wZ %wZ %wZ\n",
            &ImageName, &DllPath, &CommandLine, &Desktop, &Title, &Shell,
            &Runtime);
    Status = RtlCreateProcessParameters(&ProcessParameters,
                                        &ImageName,
                                        &DllPath,
                                        lpCurrentDirectory ?
                                        &CurrentDirectory : NULL,
                                        &CommandLine,
                                        Environment,
                                        &Title,
                                        &Desktop,
                                        &Shell,
                                        &Runtime);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create process parameters!\n");
        return Status;
    }

    /* Check if we got an environment. If not, use ours */
    if (Environment)
    {
        /* Save pointer and start lookup */
        Environment = ScanChar = ProcessParameters->Environment;
    }
    else
    {
        /* Save pointer and start lookup */
        Environment = ScanChar = OurPeb->ProcessParameters->Environment;
    }

    /* Find the environment size */
    if (ScanChar)
    {
        if (EnvSize && Environment == lpEnvironment)
        {
            /* its a converted ansi environment, bypass the length calculation */
            EnviroSize = EnvSize;
        }
        else
        {
            while (*ScanChar)
            {
                ScanChar += wcslen(ScanChar) + 1;
            }

            /* Calculate the size of the block */
            if (ScanChar == Environment)
            {
                EnviroSize = 2 * sizeof(WCHAR);
            }
            else
            {
                EnviroSize = (ULONG)((ULONG_PTR)ScanChar - (ULONG_PTR)Environment + sizeof(WCHAR));
            }
        }
        DPRINT("EnvironmentSize %ld\n", EnviroSize);

        /* Allocate and Initialize new Environment Block */
        Size = EnviroSize;
        ProcessParameters->Environment = NULL;
        Status = ZwAllocateVirtualMemory(ProcessHandle,
                                         (PVOID*)&ProcessParameters->Environment,
                                         0,
                                         &Size,
                                         MEM_COMMIT,
                                         PAGE_READWRITE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to allocate Environment Block\n");
            return(Status);
        }

        /* Write the Environment Block */
        ZwWriteVirtualMemory(ProcessHandle,
                             ProcessParameters->Environment,
                             Environment,
                             EnviroSize,
                             NULL);
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
    if (StartupInfo->dwFlags & STARTF_USESTDHANDLES)
    {
        DPRINT("Using Standard Handles\n");
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
        ProcessParameters->ConsoleHandle = OurPeb->ProcessParameters->ConsoleHandle;

        /* Is the shell trampling on our Handles? */
        if (!(StartupInfo->dwFlags &
              (STARTF_USESTDHANDLES | STARTF_USEHOTKEY | STARTF_SHELLPRIVATE)))
        {
            /* Use handles from PEB, if inheriting or they are console */
            DPRINT("Copying handles from parent\n");
            BasepCopyHandles(ProcessParameters,
                             OurPeb->ProcessParameters,
                             InheritHandles);
        }
    }

    /* Also set the Console Flag */
    if (CreationFlags & CREATE_NEW_PROCESS_GROUP)
    {
        ProcessParameters->ConsoleFlags = 1;
    }

    /* Allocate memory for the parameter block */
    Size = ProcessParameters->Length;
    Status = NtAllocateVirtualMemory(ProcessHandle,
                                     (PVOID*)&RemoteParameters,
                                     0,
                                     &Size,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to allocate Parameters Block\n");
        return(Status);
    }

    /* Set the allocated size */
    ProcessParameters->MaximumLength = Size;

    /* Handle some Parameter Flags */
    ProcessParameters->ConsoleFlags = (CreationFlags & CREATE_NEW_PROCESS_GROUP);
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

    /* Write the PEB Pointer */
    Status = NtWriteVirtualMemory(ProcessHandle,
                                  &Peb->ProcessParameters,
                                  &RemoteParameters,
                                  sizeof(PVOID),
                                  NULL);

    /* Cleanup */
    RtlFreeHeap(RtlGetProcessHeap(), 0, DllPath.Buffer);
    RtlDestroyProcessParameters(ProcessParameters);

    DPRINT("Completed\n");
    return STATUS_SUCCESS;
}

/* FUNCTIONS ****************************************************************/

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
        /* Serach for escape sequences */
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
                    SetLastErrorByStatus(STATUS_OBJECT_NAME_NOT_FOUND);
                }
            }
            else
            {
                /* Immediately set the error */
                SetLastErrorByStatus(STATUS_OBJECT_NAME_NOT_FOUND);
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
        SetLastErrorByStatus(Status);
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
    ObjectAttributes = BasepConvertObjectAttributes(&LocalObjectAttributes,
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
            SetLastErrorByStatus(Status);
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
    if(!NT_SUCCESS(Status))
    {
        DPRINT1("Unable to create process, status 0x%x\n", Status);
        SetLastErrorByStatus(Status);
        goto Cleanup;
    }

    /* Set new class */
    Status = NtSetInformationProcess(hProcess,
                                     ProcessPriorityClass,
                                     &PriorityClass,
                                     sizeof(PROCESS_PRIORITY_CLASS));
    if(!NT_SUCCESS(Status))
    {
        DPRINT1("Unable to set new process priority, status 0x%x\n", Status);
        SetLastErrorByStatus(Status);
        goto Cleanup;
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
    Status = BasepInitializeEnvironment(hProcess,
                                        RemotePeb,
                                        (LPWSTR)lpApplicationName,
                                        CurrentDirectory,
                                        (QuotesNeeded || CmdLineIsAppName || Escape) ?
                                        QuotedCmdLine : lpCommandLine,
                                        lpEnvironment,
                                        EnvSize,
                                        &StartupInfo,
                                        dwCreationFlags,
                                        bInheritHandles);

    /* Cleanup Environment */
    if (lpEnvironment && !(dwCreationFlags & CREATE_UNICODE_ENVIRONMENT))
    {
        RtlDestroyEnvironment(lpEnvironment);
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Could not initialize Process Environment\n");
        SetLastErrorByStatus(Status);
        goto Cleanup;
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
                                     &ClientId);

    if (hThread == NULL)
    {
        DPRINT1("Could not create Initial Thread\n");
        /* FIXME - set last error code */
        goto Cleanup;
    }

    /* Notify CSRSS */
    Status = BasepNotifyCsrOfCreation(dwCreationFlags,
                                      (HANDLE)ProcessBasicInfo.UniqueProcessId,
                                      bInheritHandles);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CSR Notification Failed");
        SetLastErrorByStatus(Status);
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
            CommandLine = Basep8BitStringToCachedUnicodeString(lpCommandLine);
        }
        else
        {
            /* Use a dynamic version */
            Basep8BitStringToHeapUnicodeString(&LiveCommandLine,
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
        Basep8BitStringToHeapUnicodeString(&ApplicationName,
                                           lpApplicationName);
    }
    if (lpCurrentDirectory)
    {
        Basep8BitStringToHeapUnicodeString(&CurrentDirectory,
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
