/*
 * PROJECT:         ReactOS Windows-Compatible Session Manager
 * LICENSE:         BSD 2-Clause License
 * FILE:            base/system/smss/smss.c
 * PURPOSE:         Main SMSS Code
 * PROGRAMMERS:     Alex Ionescu
 */

/* INCLUDES *******************************************************************/

#include "smss.h"
#define NDEBUG
#include "debug.h"

/* GLOBALS ********************************************************************/

typedef struct _INIT_BUFFER
{
    WCHAR DebugBuffer[256];
    RTL_USER_PROCESS_INFORMATION ProcessInfo;
} INIT_BUFFER, *PINIT_BUFFER;

/* NT Initial User Application */
WCHAR NtInitialUserProcessBuffer[128] = L"\\SystemRoot\\System32\\smss.exe";
ULONG NtInitialUserProcessBufferLength = sizeof(NtInitialUserProcessBuffer) -
                                         sizeof(WCHAR);
ULONG NtInitialUserProcessBufferType = REG_SZ;

UNICODE_STRING SmpSystemRoot;
ULONG AttachedSessionId = -1;
BOOLEAN SmpDebug, SmpEnableDots;

/* FUNCTIONS ******************************************************************/

/* GCC's incompetence strikes again */
VOID
sprintf_nt(IN PCHAR Buffer,
           IN PCHAR Format,
           IN ...)
{
    va_list ap;
    va_start(ap, Format);
    sprintf(Buffer, Format, ap);
    va_end(ap);
}

NTSTATUS
NTAPI
SmpExecuteImage(IN PUNICODE_STRING FileName,
                IN PUNICODE_STRING Directory,
                IN PUNICODE_STRING CommandLine,
                IN ULONG MuSessionId,
                IN ULONG Flags,
                IN PRTL_USER_PROCESS_INFORMATION ProcessInformation)
{
    PRTL_USER_PROCESS_INFORMATION ProcessInfo;
    NTSTATUS Status;
    RTL_USER_PROCESS_INFORMATION LocalProcessInfo;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;

    /* Use the input process information if we have it, otherwise use local */
    ProcessInfo = ProcessInformation;
    if (!ProcessInfo) ProcessInfo = &LocalProcessInfo;

    /* Create parameters for the target process */
    Status = RtlCreateProcessParameters(&ProcessParameters,
                                        FileName,
                                        SmpDefaultLibPath.Length ?
                                        &SmpDefaultLibPath : NULL,
                                        Directory,
                                        CommandLine,
                                        SmpDefaultEnvironment,
                                        NULL,
                                        NULL,
                                        NULL,
                                        0);
    if (!NT_SUCCESS(Status))
    {
        /* This is a pretty bad failure. ASSERT on checked builds and exit */
        ASSERTMSG(NT_SUCCESS(Status), "RtlCreateProcessParameters");
        DPRINT1("SMSS: RtlCreateProcessParameters failed for %wZ - Status == %lx\n",
                FileName, Status);
        return Status;
    }

    /* Set the size field as required */
    ProcessInfo->Size = sizeof(RTL_USER_PROCESS_INFORMATION);

    /* Check if the debug flag was requested */
    if (Flags & SMP_DEBUG_FLAG)
    {
        /* Write it in the process parameters */
        ProcessParameters->DebugFlags = 1;
    }
    else
    {
        /* Otherwise inherit the flag that was passed to SMSS itself */
        ProcessParameters->DebugFlags = SmpDebug;
    }

    /* Subsystems get the first 1MB of memory reserved for DOS/IVT purposes */
    if (Flags & SMP_SUBSYSTEM_FLAG)
    {
        ProcessParameters->Flags |= RTL_USER_PROCESS_PARAMETERS_RESERVE_1MB;
    }

    /* And always force NX for anything that SMSS launches */
    ProcessParameters->Flags |= RTL_USER_PROCESS_PARAMETERS_NX;

    /* Now create the process */
    Status = RtlCreateUserProcess(FileName,
                                  OBJ_CASE_INSENSITIVE,
                                  ProcessParameters,
                                  NULL,
                                  NULL,
                                  NULL,
                                  FALSE,
                                  NULL,
                                  NULL,
                                  ProcessInfo);
    RtlDestroyProcessParameters(ProcessParameters);
    if (!NT_SUCCESS(Status))
    {
        /* If we couldn't create it, fail back to the caller */
        DPRINT1("SMSS: Failed load of %wZ - Status  == %lx\n",
                FileName, Status);
        return Status;
    }

    /* Associate a session with this process */
    Status = SmpSetProcessMuSessionId(ProcessInfo->ProcessHandle, MuSessionId);

    /* If the application is deferred (suspended), there's nothing to do */
    if (Flags & SMP_DEFERRED_FLAG) return Status;

    /* Otherwise, get ready to start it, but make sure it's a native app */
    if (ProcessInfo->ImageInformation.SubSystemType == IMAGE_SUBSYSTEM_NATIVE)
    {
        /* Resume it */
        NtResumeThread(ProcessInfo->ThreadHandle, NULL);
        if (!(Flags & SMP_ASYNC_FLAG))
        {
            /* Block on it unless Async was requested */
            NtWaitForSingleObject(ProcessInfo->ThreadHandle, FALSE, NULL);
        }

        /* It's up and running now, close our handles */
        NtClose(ProcessInfo->ThreadHandle);
        NtClose(ProcessInfo->ProcessHandle);
    }
    else
    {
        /* This image is invalid, so kill it, close our handles, and fail */
        Status = STATUS_INVALID_IMAGE_FORMAT;
        NtTerminateProcess(ProcessInfo->ProcessHandle, Status);
        NtWaitForSingleObject(ProcessInfo->ThreadHandle, 0, 0);
        NtClose(ProcessInfo->ThreadHandle);
        NtClose(ProcessInfo->ProcessHandle);
        DPRINT1("SMSS: Not an NT image - %wZ\n", FileName);
    }

    /* Return the outcome of the process create */
    return Status;
}

NTSTATUS
NTAPI
SmpInvokeAutoChk(IN PUNICODE_STRING FileName,
                 IN PUNICODE_STRING Directory,
                 IN PUNICODE_STRING Arguments,
                 IN ULONG Flags)
{
    ANSI_STRING DestinationString;
    CHAR SourceString[256];
    UNICODE_STRING Destination;
    WCHAR Buffer[1024];
    BOOLEAN BootState, BootOkay, ShutdownOkay;

    /* Check if autochk should show dots (if the user booted with /SOS) */
    if (SmpQueryRegistrySosOption()) SmpEnableDots = FALSE;

    /* Make sure autochk was actually found */
    if (Flags & SMP_INVALID_PATH)
    {
        /* It wasn't, so create an error message to print on the screen */
        sprintf_nt(SourceString,
                   "%wZ program not found - skipping AUTOCHECK\n",
                   FileName);
        RtlInitAnsiString(&DestinationString, SourceString);
        if (RtlAnsiStringToUnicodeString(&Destination,
                                         &DestinationString,
                                         TRUE))
        {
            /* And show it */
            NtDisplayString(&Destination);
            RtlFreeUnicodeString(&Destination);
        }
    }
    else
    {
        /* Autochk is there, so record the BSD state */
        BootState = SmpSaveAndClearBootStatusData(&BootOkay, &ShutdownOkay);

        /* Build the path to autochk and place its arguments */
        RtlInitEmptyUnicodeString(&Destination, Buffer, sizeof(Buffer));
        RtlAppendUnicodeStringToString(&Destination, FileName);
        RtlAppendUnicodeToString(&Destination, L" ");
        RtlAppendUnicodeStringToString(&Destination, Arguments);

        /* Execute it */
        SmpExecuteImage(FileName,
                        Directory,
                        &Destination,
                        0,
                        Flags & ~SMP_AUTOCHK_FLAG,
                        NULL);

        /* Restore the BSD state */
        if (BootState) SmpRestoreBootStatusData(BootOkay, ShutdownOkay);
    }

    /* We're all done! */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
SmpExecuteCommand(IN PUNICODE_STRING CommandLine,
                  IN ULONG MuSessionId,
                  OUT PHANDLE ProcessId,
                  IN ULONG Flags)
{
    NTSTATUS Status;
    UNICODE_STRING Arguments, Directory, FileName;

    /* There's no longer a debugging subsystem */
    if (Flags & SMP_DEBUG_FLAG) return STATUS_SUCCESS;

    /* Parse the command line to see what execution flags are requested */
    Status = SmpParseCommandLine(CommandLine,
                                 &Flags,
                                 &FileName,
                                 &Directory,
                                 &Arguments);
    if (!NT_SUCCESS(Status))
    {
        /* Fail if we couldn't do that */
        DPRINT1("SMSS: SmpParseCommand( %wZ ) failed - Status == %lx\n",
                CommandLine, Status);
        return Status;
    }

    /* Check if autochk is requested */
    if (Flags & SMP_AUTOCHK_FLAG)
    {
        /* Run it */
        Status = SmpInvokeAutoChk(&FileName, &Directory, &Arguments, Flags);
    }
    else if (Flags & SMP_SUBSYSTEM_FLAG)
    {
        Status = SmpLoadSubSystem(&FileName,
                                  &Directory,
                                  CommandLine,
                                  MuSessionId,
                                  ProcessId);
    }
    else if (Flags & SMP_INVALID_PATH)
    {
        /* An invalid image was specified, fail */
        DPRINT1("SMSS: Image file (%wZ) not found\n", &FileName);
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
    }
    else
    {
        /* An actual image name was present -- execute it */
        Status = SmpExecuteImage(&FileName,
                                 &Directory,
                                 CommandLine,
                                 MuSessionId,
                                 Flags,
                                 NULL);
    }

    /* Free all the token parameters */
    if (FileName.Buffer) RtlFreeHeap(RtlGetProcessHeap(), 0, FileName.Buffer);
    if (Directory.Buffer) RtlFreeHeap(RtlGetProcessHeap(), 0, Directory.Buffer);
    if (Arguments.Buffer) RtlFreeHeap(RtlGetProcessHeap(), 0, Arguments.Buffer);

    /* Return to the caller */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SMSS: Command '%wZ' failed - Status == %x\n",
                CommandLine, Status);
    }
    return Status;
}

NTSTATUS
NTAPI
ExpLoadInitialProcess(IN PINIT_BUFFER InitBuffer,
                      OUT PRTL_USER_PROCESS_PARAMETERS *ProcessParameters,
                      OUT PCHAR *ProcessEnvironment)
{
    NTSTATUS Status;
    SIZE_T Size;
    PWSTR p;
    UNICODE_STRING NullString = RTL_CONSTANT_STRING(L"");
    UNICODE_STRING SmssName, DebugString;
    PVOID EnvironmentPtr = NULL;
    PRTL_USER_PROCESS_INFORMATION ProcessInformation;
    PRTL_USER_PROCESS_PARAMETERS ProcessParams = NULL;

    NullString.Length = sizeof(WCHAR);

    /* Use the initial buffer, after the strings */
    ProcessInformation = &InitBuffer->ProcessInfo;

    /* Allocate memory for the process parameters */
    Size = sizeof(*ProcessParams) + ((MAX_PATH * 6) * sizeof(WCHAR));
    Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                     (PVOID*)&ProcessParams,
                                     0,
                                     &Size,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        /* Failed, display error */
        p = InitBuffer->DebugBuffer;
        _snwprintf(p,
                   256 * sizeof(WCHAR),
                   L"INIT: Unable to allocate Process Parameters. 0x%lx",
                   Status);
        RtlInitUnicodeString(&DebugString, p);
        ZwDisplayString(&DebugString);

        /* Bugcheck the system */
        return Status;
    }

    /* Setup the basic header, and give the process the low 1MB to itself */
    ProcessParams->Length = (ULONG)Size;
    ProcessParams->MaximumLength = (ULONG)Size;
    ProcessParams->Flags = RTL_USER_PROCESS_PARAMETERS_NORMALIZED |
                           RTL_USER_PROCESS_PARAMETERS_RESERVE_1MB;

    /* Allocate a page for the environment */
    Size = PAGE_SIZE;
    Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                     &EnvironmentPtr,
                                     0,
                                     &Size,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        /* Failed, display error */
        p = InitBuffer->DebugBuffer;
        _snwprintf(p,
                   256 * sizeof(WCHAR),
                   L"INIT: Unable to allocate Process Environment. 0x%lx",
                   Status);
        RtlInitUnicodeString(&DebugString, p);
        ZwDisplayString(&DebugString);

        /* Bugcheck the system */
        return Status;
    }

    /* Write the pointer */
    ProcessParams->Environment = EnvironmentPtr;

    /* Make a buffer for the DOS path */
    p = (PWSTR)(ProcessParams + 1);
    ProcessParams->CurrentDirectory.DosPath.Buffer = p;
    ProcessParams->CurrentDirectory.DosPath.MaximumLength = MAX_PATH *
                                                            sizeof(WCHAR);

    /* Copy the DOS path */
    RtlCopyUnicodeString(&ProcessParams->CurrentDirectory.DosPath,
                         &SmpSystemRoot);

    /* Make a buffer for the DLL Path */
    p = (PWSTR)((PCHAR)ProcessParams->CurrentDirectory.DosPath.Buffer +
                ProcessParams->CurrentDirectory.DosPath.MaximumLength);
    ProcessParams->DllPath.Buffer = p;
    ProcessParams->DllPath.MaximumLength = MAX_PATH * sizeof(WCHAR);

    /* Copy the DLL path and append the system32 directory */
    RtlCopyUnicodeString(&ProcessParams->DllPath,
                         &ProcessParams->CurrentDirectory.DosPath);
    RtlAppendUnicodeToString(&ProcessParams->DllPath, L"\\System32");

    /* Make a buffer for the image name */
    p = (PWSTR)((PCHAR)ProcessParams->DllPath.Buffer +
                ProcessParams->DllPath.MaximumLength);
    ProcessParams->ImagePathName.Buffer = p;
    ProcessParams->ImagePathName.MaximumLength = MAX_PATH * sizeof(WCHAR);

    /* Make sure the buffer is a valid string which within the given length */
    if ((NtInitialUserProcessBufferType != REG_SZ) ||
        ((NtInitialUserProcessBufferLength != MAXULONG) &&
         ((NtInitialUserProcessBufferLength < sizeof(WCHAR)) ||
          (NtInitialUserProcessBufferLength >
           sizeof(NtInitialUserProcessBuffer) - sizeof(WCHAR)))))
    {
        /* Invalid initial process string, bugcheck */
        return STATUS_INVALID_PARAMETER;
    }

    /* Cut out anything after a space */
    p = NtInitialUserProcessBuffer;
    while ((*p) && (*p != L' ')) p++;

    /* Set the image path length */
    ProcessParams->ImagePathName.Length =
        (USHORT)((PCHAR)p - (PCHAR)NtInitialUserProcessBuffer);

    /* Copy the actual buffer */
    RtlCopyMemory(ProcessParams->ImagePathName.Buffer,
                  NtInitialUserProcessBuffer,
                  ProcessParams->ImagePathName.Length);

    /* Null-terminate it */
    ProcessParams->ImagePathName.Buffer[ProcessParams->ImagePathName.Length /
                                        sizeof(WCHAR)] = UNICODE_NULL;

    /* Make a buffer for the command line */
    p = (PWSTR)((PCHAR)ProcessParams->ImagePathName.Buffer +
                ProcessParams->ImagePathName.MaximumLength);
    ProcessParams->CommandLine.Buffer = p;
    ProcessParams->CommandLine.MaximumLength = MAX_PATH * sizeof(WCHAR);

    /* Add the image name to the command line */
    RtlAppendUnicodeToString(&ProcessParams->CommandLine,
                             NtInitialUserProcessBuffer);

    /* Create the environment string */
    ProcessParams->Environment = SmpDefaultEnvironment;

    /* Create SMSS process */
    SmssName = ProcessParams->ImagePathName;
    Status = RtlCreateUserProcess(&SmssName,
                                  OBJ_CASE_INSENSITIVE,
                                  RtlDeNormalizeProcessParams(ProcessParams),
                                  NULL,
                                  NULL,
                                  NULL,
                                  FALSE,
                                  NULL,
                                  NULL,
                                  ProcessInformation);
    if (!NT_SUCCESS(Status))
    {
        /* Failed, display error */
        p = InitBuffer->DebugBuffer;
        _snwprintf(p,
                   256 * sizeof(WCHAR),
                   L"INIT: Unable to create Session Manager. 0x%lx",
                   Status);
        RtlInitUnicodeString(&DebugString, p);
        ZwDisplayString(&DebugString);

        /* Bugcheck the system */
        return Status;
    }

    /* Resume the thread */
    Status = ZwResumeThread(ProcessInformation->ThreadHandle, NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Failed, display error */
        p = InitBuffer->DebugBuffer;
        _snwprintf(p,
                   256 * sizeof(WCHAR),
                   L"INIT: Unable to resume Session Manager. 0x%lx",
                   Status);
        RtlInitUnicodeString(&DebugString, p);
        ZwDisplayString(&DebugString);

        /* Bugcheck the system */
        return Status;
    }

    /* Return success */
    *ProcessParameters = ProcessParams;
    *ProcessEnvironment = EnvironmentPtr;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
LaunchOldSmss(OUT PHANDLE Handles)
{
    PINIT_BUFFER InitBuffer;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters = NULL;
    PRTL_USER_PROCESS_INFORMATION ProcessInfo;
    NTSTATUS Status;
    PCHAR Environment;

    /* No handles at first */
    Handles[0] = Handles[1] = NULL;

    /* Initialize the system root */
    RtlInitUnicodeString(&SmpSystemRoot, SharedUserData->NtSystemRoot);

    /* Allocate the initialization buffer */
    InitBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(INIT_BUFFER));
    if (!InitBuffer)
    {
        /* Bugcheck */
        return STATUS_NO_MEMORY;
    }

    /* Launch initial process */
    ProcessInfo = &InitBuffer->ProcessInfo;
    Status = ExpLoadInitialProcess(InitBuffer, &ProcessParameters, &Environment);
    if (!NT_SUCCESS(Status))
    {
        /* Failed, display error */
        DPRINT1("INIT: Session Manager failed to load.\n");
        return Status;
    }

    /* Return the handle and status */
    Handles[0] = ProcessInfo->ProcessHandle;
    Handles[1] = ProcessInfo->ProcessHandle;
    return Status;
}

NTSTATUS
NTAPI
SmpTerminate(IN PULONG_PTR Parameters,
             IN ULONG ParameterMask,
             IN ULONG ParameterCount)
{
    NTSTATUS Status;
    BOOLEAN Old;
    ULONG Response;

    /* Give the shutdown privilege to the thread */
    if (RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE, TRUE, &Old) ==
        STATUS_NO_TOKEN)
    {
        /* Thread doesn't have a token, give it to the entire process */
        RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE, TRUE, FALSE, &Old);
    }

    /* Take down the process/machine with a hard error */
    Status = NtRaiseHardError(STATUS_SYSTEM_PROCESS_TERMINATED,
                              ParameterCount,
                              ParameterMask,
                              Parameters,
                              OptionShutdownSystem,
                              &Response);

    /* Terminate the process if the hard error didn't already */
    return NtTerminateProcess(NtCurrentProcess(), Status);
}

LONG
SmpUnhandledExceptionFilter(IN PEXCEPTION_POINTERS ExceptionInfo)
{
    ULONG_PTR Parameters[4];
    UNICODE_STRING DestinationString;

    /* Print and breakpoint into the debugger */
    DbgPrint("SMSS: Unhandled exception - Status == %x  IP == %x\n",
             ExceptionInfo->ExceptionRecord->ExceptionCode,
             ExceptionInfo->ExceptionRecord->ExceptionAddress);
    DbgPrint("      Memory Address: %x  Read/Write: %x\n",
             ExceptionInfo->ExceptionRecord->ExceptionInformation[0],
             ExceptionInfo->ExceptionRecord->ExceptionInformation[1]);
    DbgBreakPoint();

    /* Build the hard error and terminate */
    RtlInitUnicodeString(&DestinationString, L"Unhandled Exception in Session Manager");
    Parameters[0] = (ULONG_PTR)&DestinationString;
    Parameters[1] = ExceptionInfo->ExceptionRecord->ExceptionCode;
    Parameters[2] = (ULONG_PTR)ExceptionInfo->ExceptionRecord->ExceptionAddress;
    Parameters[3] = (ULONG_PTR)ExceptionInfo->ContextRecord;
    SmpTerminate(Parameters, 1, RTL_NUMBER_OF(Parameters));

    /* We hould never get here */
    ASSERT(FALSE);
    return EXCEPTION_EXECUTE_HANDLER;
}

NTSTATUS
_main(IN INT argc,
      IN PCHAR argv[],
      IN PCHAR envp[],
      IN ULONG DebugFlag)
{
    NTSTATUS Status;
    KPRIORITY SetBasePriority;
    ULONG_PTR Parameters[4];
    HANDLE Handles[2];
    PVOID State;
    ULONG Flags;
    PROCESS_BASIC_INFORMATION ProcessInfo;
    UNICODE_STRING DbgString, InitialCommand;

    /* Make us critical */
    RtlSetProcessIsCritical(TRUE, NULL, FALSE);
    RtlSetThreadIsCritical(TRUE, NULL, FALSE);

    /* Raise our priority */
    SetBasePriority = 11;
    Status = NtSetInformationProcess(NtCurrentProcess(),
                                     ProcessBasePriority,
                                     (PVOID)&SetBasePriority,
                                     sizeof(SetBasePriority));
    ASSERT(NT_SUCCESS(Status));

    /* Save the debug flag if it was passed */
    if (DebugFlag) SmpDebug = DebugFlag;

    /* Build the hard error parameters */
    Parameters[0] = (ULONG_PTR)&DbgString;
    Parameters[1] = Parameters[2] = Parameters[3] = 0;

    /* Enter SEH so we can terminate correctly if anything goes wrong */
    _SEH2_TRY
    {
        /* Initialize SMSS */
        Status = SmpInit(&InitialCommand, Handles);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("SMSS: SmpInit return failure - Status == %x\n", Status);
            RtlInitUnicodeString(&DbgString, L"Session Manager Initialization");
            Parameters[1] = Status;
            //_SEH2_LEAVE; Hack so that setup can work. will go away later
        }

        /* Get the global flags */
        Status = NtQuerySystemInformation(SystemFlagsInformation,
                                          &Flags,
                                          sizeof(Flags),
                                          NULL);
        ASSERT(NT_SUCCESS(Status));

        /* Before executing the initial command check if the debug flag is on */
        if (Flags & (FLG_DEBUG_INITIAL_COMMAND | FLG_DEBUG_INITIAL_COMMAND_EX))
        {
            /* SMSS should launch ntsd with a few parameters at this point */
            DPRINT1("Global Flags Set to SMSS Debugging: Not yet supported\n");
        }

#if 0
        /* Execute the initial command (Winlogon.exe) */
        Status = SmpExecuteInitialCommand(0, &InitialCommand, &Handles[1], NULL);
#else
        /* Launch the original SMSS */
        DPRINT1("SMSS-2 Loaded... Launching original SMSS\n");
        Status = LaunchOldSmss(Handles);
#endif
        if (!NT_SUCCESS(Status))
        {
            /* Fail and raise a hard error */
            DPRINT1("SMSS: Execute Initial Command failed\n");
            RtlInitUnicodeString(&DbgString,
                                 L"Session Manager ExecuteInitialCommand");
            Parameters[1] = Status;
            _SEH2_LEAVE;
        }

        /*  Check if we're already attached to a session */
        Status = SmpAcquirePrivilege(SE_LOAD_DRIVER_PRIVILEGE, &State);
        if (AttachedSessionId != -1)
        {
            /* Detach from it, we should be in no session right now */
            Status = NtSetSystemInformation(SystemSessionDetach,
                                            &AttachedSessionId,
                                            sizeof(AttachedSessionId));
            ASSERT(NT_SUCCESS(Status));
            AttachedSessionId = -1;
        }
        SmpReleasePrivilege(State);

        /* Wait on either CSRSS or Winlogon to die */
        Status = NtWaitForMultipleObjects(RTL_NUMBER_OF(Handles),
                                          Handles,
                                          WaitAny,
                                          FALSE,
                                          NULL);
        if (Status == STATUS_WAIT_0)
        {
            /* CSRSS is dead, get exit code and prepare for the hard error */
            RtlInitUnicodeString(&DbgString, L"Windows SubSystem");
            Status = NtQueryInformationProcess(Handles[0],
                                               ProcessBasicInformation,
                                               &ProcessInfo,
                                               sizeof(ProcessInfo),
                                               NULL);
            DPRINT1("SMSS: Windows subsystem terminated when it wasn't supposed to.\n");
        }
        else
        {
            /* The initial command is dead or we have another failure */
            RtlInitUnicodeString(&DbgString, L"Windows Logon Process");
            if (Status == STATUS_WAIT_1)
            {
                /* Winlogon.exe got terminated, get its exit code */
                Status = NtQueryInformationProcess(Handles[1],
                                                   ProcessBasicInformation,
                                                   &ProcessInfo,
                                                   sizeof(ProcessInfo),
                                                   NULL);
            }
            else
            {
                /* Something else satisfied our wait, so set the wait status */
                ProcessInfo.ExitStatus = Status;
                Status = STATUS_SUCCESS;
            }
            DPRINT1("SMSS: Initial command '%wZ' terminated when it wasn't supposed to.\n",
                    &InitialCommand);
        }

        /* Check if NtQueryInformationProcess was successful */
        if (NT_SUCCESS(Status))
        {
            /* Then we must have a valid exit status in the structure, use it */
            Parameters[1] = ProcessInfo.ExitStatus;
        }
        else
        {
            /* We really don't know what happened, so set a generic error */
            Parameters[1] = STATUS_UNSUCCESSFUL;
        }
    }
    _SEH2_EXCEPT(SmpUnhandledExceptionFilter(_SEH2_GetExceptionInformation()))
    {
        /* The filter should never return here */
        ASSERT(FALSE);
    }
    _SEH2_END;

    /* Something in the init loop failed, terminate SMSS */
    return SmpTerminate(Parameters, 1, RTL_NUMBER_OF(Parameters));
}

/* EOF */
