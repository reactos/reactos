/*
 * PROJECT:         ReactOS Windows-Compatible Session Manager
 * LICENSE:         BSD 2-Clause License
 * FILE:            base/system/smss/smsubsys.c
 * PURPOSE:         Main SMSS Code
 * PROGRAMMERS:     Alex Ionescu
 */

/* INCLUDES *******************************************************************/

#include "smss.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

RTL_CRITICAL_SECTION SmpKnownSubSysLock;
LIST_ENTRY SmpKnownSubSysHead;
HANDLE SmpWindowsSubSysProcess;
HANDLE SmpWindowsSubSysProcessId;
BOOLEAN RegPosixSingleInstance;
WCHAR InitialCommandBuffer[256];

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
SmpCallCsrCreateProcess(IN PSB_API_MSG SbApiMsg,
                        IN USHORT MessageLength,
                        IN HANDLE PortHandle)
{
    NTSTATUS Status;

    /* Initialize the header and send the message to CSRSS */
    SbApiMsg->h.u2.ZeroInit = 0;
    SbApiMsg->h.u1.s1.DataLength = MessageLength + 8;
    SbApiMsg->h.u1.s1.TotalLength = sizeof(SB_API_MSG);
    SbApiMsg->ApiNumber = SbpCreateProcess;
    Status = NtRequestWaitReplyPort(PortHandle, &SbApiMsg->h, &SbApiMsg->h);
    if (NT_SUCCESS(Status)) Status = SbApiMsg->ReturnValue;
    return Status;
}

VOID
NTAPI
SmpDereferenceSubsystem(IN PSMP_SUBSYSTEM SubSystem)
{
    /* Acquire the database lock while we (potentially) destroy this subsystem */
    RtlEnterCriticalSection(&SmpKnownSubSysLock);

    /* Drop the reference and see if it's terminating */
    if (!(--SubSystem->ReferenceCount) && (SubSystem->Terminating))
    {
        /* Close all handles and free it */
        if (SubSystem->Event) NtClose(SubSystem->Event);
        if (SubSystem->ProcessHandle) NtClose(SubSystem->ProcessHandle);
        if (SubSystem->SbApiPort) NtClose(SubSystem->SbApiPort);
        RtlFreeHeap(SmpHeap, 0, SubSystem);
    }

    /* Release the database lock */
    RtlLeaveCriticalSection(&SmpKnownSubSysLock);
}

PSMP_SUBSYSTEM
NTAPI
SmpLocateKnownSubSysByCid(IN PCLIENT_ID ClientId)
{
    PSMP_SUBSYSTEM Subsystem = NULL;
    PLIST_ENTRY NextEntry;

    /* Lock the subsystem database */
    RtlEnterCriticalSection(&SmpKnownSubSysLock);

    /* Loop each subsystem in the database */
    NextEntry = SmpKnownSubSysHead.Flink;
    while (NextEntry != &SmpKnownSubSysHead)
    {
        /* Check if this one matches the client ID and is still valid */
        Subsystem = CONTAINING_RECORD(NextEntry, SMP_SUBSYSTEM, Entry);
        if ((*(PULONGLONG)&Subsystem->ClientId == *(PULONGLONG)ClientId) &&
            !(Subsystem->Terminating))
        {
            /* Add a reference and return it */
            Subsystem->ReferenceCount++;
            break;
        }

        /* Reset the current pointer and keep earching */
        Subsystem = NULL;
        NextEntry = NextEntry->Flink;
    }

    /* Release the lock and return the subsystem we found */
    RtlLeaveCriticalSection(&SmpKnownSubSysLock);
    return Subsystem;
}

PSMP_SUBSYSTEM
NTAPI
SmpLocateKnownSubSysByType(IN ULONG MuSessionId,
                           IN ULONG ImageType)
{
    PSMP_SUBSYSTEM Subsystem = NULL;
    PLIST_ENTRY NextEntry;

    /* Lock the subsystem database */
    RtlEnterCriticalSection(&SmpKnownSubSysLock);

    /* Loop each subsystem in the database */
    NextEntry = SmpKnownSubSysHead.Flink;
    while (NextEntry != &SmpKnownSubSysHead)
    {
        /* Check if this one matches the image and uID, and is still valid */
        Subsystem = CONTAINING_RECORD(NextEntry, SMP_SUBSYSTEM, Entry);
        if ((Subsystem->ImageType == ImageType) &&
            !(Subsystem->Terminating) &&
            (Subsystem->MuSessionId == MuSessionId))
        {
            /* Return it referenced for the caller */
            Subsystem->ReferenceCount++;
            break;
        }

        /* Reset the current pointer and keep earching */
        Subsystem = NULL;
        NextEntry = NextEntry->Flink;
    }

    /* Release the lock and return the subsystem we found */
    RtlLeaveCriticalSection(&SmpKnownSubSysLock);
    return Subsystem;
}

NTSTATUS
NTAPI
SmpLoadSubSystem(IN PUNICODE_STRING FileName,
                 IN PUNICODE_STRING Directory,
                 IN PUNICODE_STRING CommandLine,
                 IN ULONG MuSessionId,
                 OUT PHANDLE ProcessId,
                 IN ULONG Flags)
{
    PSMP_SUBSYSTEM Subsystem, NewSubsystem, KnownSubsystem = NULL;
    HANDLE SubSysProcessId;
    NTSTATUS Status = STATUS_SUCCESS;
    SB_API_MSG SbApiMsg, SbApiMsg2;
    RTL_USER_PROCESS_INFORMATION ProcessInformation;
    LARGE_INTEGER Timeout;
    PVOID State;
    PSB_CREATE_PROCESS_MSG CreateProcess = &SbApiMsg.CreateProcess;
    PSB_CREATE_SESSION_MSG CreateSession = &SbApiMsg.CreateSession;

    /* Make sure this is a found subsystem */
    if (Flags & SMP_INVALID_PATH)
    {
        DPRINT1("SMSS: Unable to find subsystem - %wZ\n", FileName);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    /* Don't use a session if the flag is set */
    if (Flags & 0x80) MuSessionId = 0;

    /* Lock the subsystems while we do a look up */
    RtlEnterCriticalSection(&SmpKnownSubSysLock);
    while (TRUE)
    {
        /* Check if we found a subsystem not yet fully iniitalized */
        Subsystem = SmpLocateKnownSubSysByType(MuSessionId, -1);
        if (!Subsystem) break;
        RtlLeaveCriticalSection(&SmpKnownSubSysLock);

        /* Wait on it to initialize */
        NtWaitForSingleObject(Subsystem->Event, FALSE, NULL);

        /* Dereference it and try the next one */
        RtlEnterCriticalSection(&SmpKnownSubSysLock);
        SmpDereferenceSubsystem(Subsystem);
    }

    /* Check if this is a POSIX subsystem */
    if (Flags & SMP_POSIX_FLAG)
    {
        /* Do we already have it? */
        Subsystem = SmpLocateKnownSubSysByType(MuSessionId, IMAGE_SUBSYSTEM_POSIX_CUI);
    }
    else if (Flags & SMP_OS2_FLAG)
    {
        /* This is an OS/2 subsystem, do we we already have it? */
        Subsystem = SmpLocateKnownSubSysByType(MuSessionId, IMAGE_SUBSYSTEM_OS2_CUI);
    }

    /* Check if we already have one of the optional subsystems for the session */
    if (Subsystem)
    {
        /* Dereference and return, no work to do */
        SmpDereferenceSubsystem(Subsystem);
        RtlLeaveCriticalSection(&SmpKnownSubSysLock);
        return STATUS_SUCCESS;
    }

    /* Allocate a new subsystem! */
    NewSubsystem = RtlAllocateHeap(SmpHeap, SmBaseTag, sizeof(SMP_SUBSYSTEM));
    if (!NewSubsystem)
    {
        RtlLeaveCriticalSection(&SmpKnownSubSysLock);
        return STATUS_NO_MEMORY;
    }

    /* Initialize its header and reference count */
    NewSubsystem->ReferenceCount = 1;
    NewSubsystem->MuSessionId = MuSessionId;
    NewSubsystem->ImageType = -1;

    /* Clear out all the other data for now */
    NewSubsystem->Terminating = FALSE;
    NewSubsystem->ProcessHandle = NULL;
    NewSubsystem->Event = NULL;
    NewSubsystem->PortHandle = NULL;
    NewSubsystem->SbApiPort = NULL;

    /* Create the event we'll be wating on for initialization */
    Status = NtCreateEvent(&NewSubsystem->Event,
                           EVENT_ALL_ACCESS,
                           NULL,
                           NotificationEvent,
                           FALSE);
    if (!NT_SUCCESS(Status))
    {
        /* This failed, bail out */
        RtlFreeHeap(SmpHeap, 0, NewSubsystem);
        RtlLeaveCriticalSection(&SmpKnownSubSysLock);
        return STATUS_NO_MEMORY;
    }

    /* Insert the subsystem and release the lock. It can now be found */
    InsertTailList(&SmpKnownSubSysHead, &NewSubsystem->Entry);
    RtlLeaveCriticalSection(&SmpKnownSubSysLock);

    /* The OS/2 and POSIX subsystems are actually Windows applications! */
    if (Flags & (SMP_POSIX_FLAG | SMP_OS2_FLAG))
    {
        /* Locate the Windows subsystem for this session */
        KnownSubsystem = SmpLocateKnownSubSysByType(MuSessionId,
                                                    IMAGE_SUBSYSTEM_WINDOWS_GUI);
        if (!KnownSubsystem)
        {
            DPRINT1("SMSS: SmpLoadSubSystem - SmpLocateKnownSubSysByType Failed\n");
            goto Quickie2;
        }

        /* Fill out all the process details and call CSRSS to launch it */
        CreateProcess->In.ImageName = FileName;
        CreateProcess->In.CurrentDirectory = Directory;
        CreateProcess->In.CommandLine = CommandLine;
        CreateProcess->In.DllPath = SmpDefaultLibPath.Length ?
                                    &SmpDefaultLibPath : NULL;
        CreateProcess->In.Flags = Flags | SMP_DEFERRED_FLAG;
        CreateProcess->In.DebugFlags = SmpDebug;
        Status = SmpCallCsrCreateProcess(&SbApiMsg,
                                         sizeof(*CreateProcess),
                                         KnownSubsystem->SbApiPort);
        if (!NT_SUCCESS(Status))
        {
            /* Handle failures */
            DPRINT1("SMSS: SmpLoadSubSystem - SmpCallCsrCreateProcess Failed with  Status %lx\n",
                    Status);
            goto Quickie2;
        }

        /* Save the process information we'll need for the create session */
        ProcessInformation.ProcessHandle = CreateProcess->Out.ProcessHandle;
        ProcessInformation.ThreadHandle = CreateProcess->Out.ThreadHandle;
        ProcessInformation.ClientId = CreateProcess->Out.ClientId;
        ProcessInformation.ImageInformation.SubSystemType = CreateProcess->Out.SubsystemType;
    }
    else
    {
        /* This must be CSRSS itself, so just launch it and that's it */
        Status = SmpExecuteImage(FileName,
                                 Directory,
                                 CommandLine,
                                 MuSessionId,
                                 Flags | SMP_DEFERRED_FLAG,
                                 &ProcessInformation);
        if (!NT_SUCCESS(Status))
        {
            /* Handle failures */
            DPRINT1("SMSS: SmpLoadSubSystem - SmpExecuteImage Failed with  Status %lx\n",
                    Status);
            goto Quickie2;
        }
    }

    /* Fill out the handle and client ID in the subsystem structure now */
    NewSubsystem->ProcessHandle = ProcessInformation.ProcessHandle;
    NewSubsystem->ClientId = ProcessInformation.ClientId;

    /* Check if we launched a native image or a subsystem-backed image */
    if (ProcessInformation.ImageInformation.SubSystemType == IMAGE_SUBSYSTEM_NATIVE)
    {
        /* This must be CSRSS itself, since it's a native subsystem image */
        SubSysProcessId = ProcessInformation.ClientId.UniqueProcess;
        if ((ProcessId) && !(*ProcessId)) *ProcessId = SubSysProcessId;

        /* Was this the initial CSRSS on Session 0? */
        if (!MuSessionId)
        {
            /* Then save it in the global variables */
            SmpWindowsSubSysProcessId = SubSysProcessId;
            SmpWindowsSubSysProcess = ProcessInformation.ProcessHandle;
        }
        ASSERT(NT_SUCCESS(Status));
    }
    else
    {
        /* This is the POSIX or OS/2 subsystem process, copy its information */
        RtlCopyMemory(&CreateSession->ProcessInfo,
                      &ProcessInformation,
                      sizeof(CreateSession->ProcessInfo));

        /* Not sure these field mean what I think they do -- but clear them */
        *(PULONGLONG)&CreateSession->ClientId = 0;
        CreateSession->MuSessionId = 0;

        /* This should find CSRSS because they are POSIX or OS/2 subsystems */
        Subsystem = SmpLocateKnownSubSysByType(MuSessionId,
                                               ProcessInformation.ImageInformation.SubSystemType);
        if (!Subsystem)
        {
            /* Odd failure -- but handle it anyway */
            Status = STATUS_NO_SUCH_PACKAGE;
            DPRINT1("SMSS: SmpLoadSubSystem - SmpLocateKnownSubSysByType Failed with  Status %lx for sessionid %lu\n",
                    Status,
                    MuSessionId);
            goto Quickie;
        }

        /* Duplicate the parent process handle for the subsystem to have */
        Status = NtDuplicateObject(NtCurrentProcess(),
                                   ProcessInformation.ProcessHandle,
                                   Subsystem->ProcessHandle,
                                   &CreateSession->ProcessInfo.ProcessHandle,
                                   PROCESS_ALL_ACCESS,
                                   0,
                                   0);
        if (!NT_SUCCESS(Status))
        {
            /* Fail since this is critical */
            DPRINT1("SMSS: SmpLoadSubSystem - NtDuplicateObject Failed with  Status %lx for sessionid %lu\n",
                    Status,
                    MuSessionId);
            goto Quickie;
        }

        /* Duplicate the initial thread handle for the subsystem to have */
        Status = NtDuplicateObject(NtCurrentProcess(),
                                   ProcessInformation.ThreadHandle,
                                   Subsystem->ProcessHandle,
                                   &CreateSession->ProcessInfo.ThreadHandle,
                                   THREAD_ALL_ACCESS,
                                   0,
                                   0);
        if (!NT_SUCCESS(Status))
        {
            /* Fail since this is critical */
            DPRINT1("SMSS: SmpLoadSubSystem - NtDuplicateObject Failed with  Status %lx for sessionid %lu\n",
                    Status,
                    MuSessionId);
            goto Quickie;
        }

        /* Allocate an internal Session ID for this subsystem */
        MuSessionId = SmpAllocateSessionId(Subsystem, 0);
        CreateSession->SessionId = MuSessionId;

        /* Send the create session message to the subsystem */
        SbApiMsg2.ReturnValue = STATUS_SUCCESS;
        SbApiMsg2.h.u2.ZeroInit = 0;
        SbApiMsg2.h.u1.s1.DataLength = sizeof(SB_CREATE_SESSION_MSG) + 8;
        SbApiMsg2.h.u1.s1.TotalLength = sizeof(SB_API_MSG);
        Status = NtRequestWaitReplyPort(Subsystem->SbApiPort,
                                        &SbApiMsg2.h,
                                        &SbApiMsg2.h);
        if (NT_SUCCESS(Status)) Status = SbApiMsg2.ReturnValue;
        if (!NT_SUCCESS(Status))
        {
            /* Delete the session and handle failure if the LPC call failed */
            SmpDeleteSession(CreateSession->SessionId);
            DPRINT1("SMSS: SmpLoadSubSystem - NtRequestWaitReplyPort Failed with  Status %lx for sessionid %lu\n",
                    Status,
                    CreateSession->SessionId);
            goto Quickie;
        }
    }

    /* Okay, everything looks good to go, initialize this subsystem now! */
    Status = NtResumeThread(ProcessInformation.ThreadHandle, NULL);
    if (!NT_SUCCESS(Status))
    {
        /* That didn't work -- back out of everything */
        DPRINT1("SMSS: SmpLoadSubSystem - NtResumeThread failed Status %lx\n", Status);
        goto Quickie;
    }

    /* Check if this was the subsystem for a different session */
    if (MuSessionId)
    {
        /* Wait up to 60 seconds for it to initialize */
        Timeout.QuadPart = -600000000;
        Status = NtWaitForSingleObject(NewSubsystem->Event, FALSE, &Timeout);

        /* Timeout is done -- does this session still exist? */
        if (!SmpCheckDuplicateMuSessionId(MuSessionId))
        {
            /* Nope, it died. Cleanup should've ocurred in a different path. */
            DPRINT1("SMSS: SmpLoadSubSystem - session deleted\n");
            return STATUS_DELETE_PENDING;
        }

        /* Check if we timed our or there was another error with the wait */
        if (Status != STATUS_WAIT_0)
        {
            /* Something is wrong with the subsystem, so back out of everything */
            DPRINT1("SMSS: SmpLoadSubSystem - Timeout waiting for subsystem connect with Status %lx for sessionid %lu\n",
                    Status,
                    MuSessionId);
            goto Quickie;
        }
    }
    else
    {
        /* This a session 0 subsystem, just wait for it to initialize */
        NtWaitForSingleObject(NewSubsystem->Event, FALSE, NULL);
    }

    /* Subsystem is created, resumed, and initialized. Close handles and exit */
    NtClose(ProcessInformation.ThreadHandle);
    Status = STATUS_SUCCESS;
    goto Quickie2;

Quickie:
    /* This is the failure path. First check if we need to detach from session */
    if ((AttachedSessionId == -1) || (Flags & (SMP_POSIX_FLAG | SMP_OS2_FLAG)))
    {
        /* We were not attached, or did not launch subsystems that required it */
        DPRINT1("SMSS: Did not detach from Session Space: SessionId=%x Flags=%x Status=%x\n",
                AttachedSessionId,
                Flags | SMP_DEFERRED_FLAG,
                Status);
    }
    else
    {
        /* Get the privilege we need for detachment */
        Status = SmpAcquirePrivilege(SE_LOAD_DRIVER_PRIVILEGE, &State);
        if (!NT_SUCCESS(Status))
        {
            /* We can't detach without it */
            DPRINT1("SMSS: Did not detach from Session Space: SessionId=%x Flags=%x Status=%x\n",
                    AttachedSessionId,
                    Flags | SMP_DEFERRED_FLAG,
                    Status);
        }
        else
        {
            /* Now detach from the session */
            Status = NtSetSystemInformation(SystemSessionDetach,
                                            &AttachedSessionId,
                                            sizeof(AttachedSessionId));
            if (!NT_SUCCESS(Status))
            {
                /* Failed to detach. Note the DPRINT1 has a typo in Windows */
                DPRINT1("SMSS: SmpStartCsr, Couldn't Detach from Session Space. Status=%x\n", Status);
                ASSERT(NT_SUCCESS(Status));
            }
            else
            {
                /* Detachment worked, reset our attached session ID */
                AttachedSessionId = -1;
            }

            /* And release the privilege we acquired */
            SmpReleasePrivilege(State);
        }
    }

    /* Since this is the failure path, terminate the subsystem process */
    NtTerminateProcess(ProcessInformation.ProcessHandle, Status);
    NtClose(ProcessInformation.ThreadHandle);

Quickie2:
    /* This is the cleanup path -- first dereference our subsystems */
    RtlEnterCriticalSection(&SmpKnownSubSysLock);
    if (Subsystem) SmpDereferenceSubsystem(Subsystem);
    if (KnownSubsystem) SmpDereferenceSubsystem(KnownSubsystem);

    /* In the failure case, destroy the new subsystem we just created */
    if (!NT_SUCCESS(Status))
    {
        RemoveEntryList(&NewSubsystem->Entry);
        NtSetEvent(NewSubsystem->Event, 0);
        if (NewSubsystem) SmpDereferenceSubsystem(NewSubsystem);
    }

    /* Finally, we're all done! */
    RtlLeaveCriticalSection(&SmpKnownSubSysLock);
    return Status;
}

NTSTATUS
NTAPI
SmpLoadSubSystemsForMuSession(IN PULONG MuSessionId,
                              OUT PHANDLE ProcessId,
                              IN PUNICODE_STRING InitialCommand)
{
    NTSTATUS Status = STATUS_SUCCESS, Status2;
    PSMP_REGISTRY_VALUE RegEntry;
    UNICODE_STRING DestinationString, NtPath;
    PLIST_ENTRY NextEntry;
    LARGE_INTEGER Timeout;
    PVOID State;

    /* Write a few last registry keys with the boot partition information */
    SmpTranslateSystemPartitionInformation();

    /* Process "SetupExecute" values */
    NextEntry = SmpSetupExecuteList.Flink;
    while (NextEntry != &SmpSetupExecuteList)
    {
        /* Execute each one and move on */
        RegEntry = CONTAINING_RECORD(NextEntry, SMP_REGISTRY_VALUE, Entry);
        SmpExecuteCommand(&RegEntry->Name, 0, NULL, 0);
        NextEntry = NextEntry->Flink;
    }

    /* Now process the subsystems */
    NextEntry = SmpSubSystemList.Flink;
    while (NextEntry != &SmpSubSystemList)
    {
        /* Get the entry and check if this is the special Win32k entry */
        RegEntry = CONTAINING_RECORD(NextEntry, SMP_REGISTRY_VALUE, Entry);
        if (_wcsicmp(RegEntry->Name.Buffer, L"Kmode") == 0)
        {
            /* Translate it */
            if (!RtlDosPathNameToNtPathName_U(RegEntry->Value.Buffer,
                                              &NtPath,
                                              NULL,
                                              NULL))
            {
                Status = STATUS_OBJECT_PATH_SYNTAX_BAD;
                DPRINT1("Failed: %lx\n", Status);
            }
            else
            {
                /* Get the driver privilege */
                Status = SmpAcquirePrivilege(SE_LOAD_DRIVER_PRIVILEGE, &State);
                if (NT_SUCCESS(Status))
                {
                    /* Create the new session */
                    ASSERT(AttachedSessionId == -1);
                    Status = NtSetSystemInformation(SystemSessionCreate,
                                                    MuSessionId,
                                                    sizeof(*MuSessionId));
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("SMSS: Session space creation failed\n");
                        SmpReleasePrivilege(State);
                        RtlFreeHeap(RtlGetProcessHeap(), 0, NtPath.Buffer);
                        return Status;
                    }
                    AttachedSessionId = *MuSessionId;

                    /*
                     * Start Win32k.sys on this session. Use a hardcoded value
                     * instead of the Kmode one...
                     */
                    RtlInitUnicodeString(&DestinationString,
                                         L"\\SystemRoot\\System32\\win32k.sys");
                    Status = NtSetSystemInformation(SystemExtendServiceTableInformation,
                                                    &DestinationString,
                                                    sizeof(DestinationString));
                    RtlFreeHeap(RtlGetProcessHeap(), 0, NtPath.Buffer);
                    SmpReleasePrivilege(State);
                    if (!NT_SUCCESS(Status))
                    {
                        DPRINT1("SMSS: Load of WIN32K failed.\n");
                        return Status;
                    }
                }
            }
        }

        /* Next entry */
        NextEntry = NextEntry->Flink;
    }

    /* Now parse the required subsystem list */
    NextEntry = SmpSubSystemsToLoad.Flink;
    while (NextEntry != &SmpSubSystemsToLoad)
    {
        /* Get each entry and check if it's the internal debug or not */
        RegEntry = CONTAINING_RECORD(NextEntry, SMP_REGISTRY_VALUE, Entry);
        if (_wcsicmp(RegEntry->Name.Buffer, L"Debug") == 0)
        {
            /* Load the internal debug system */
            Status = SmpExecuteCommand(&RegEntry->Value,
                                       *MuSessionId,
                                       ProcessId,
                                       SMP_DEBUG_FLAG | SMP_SUBSYSTEM_FLAG);
        }
        else
        {
            /* Load the required subsystem */
            Status = SmpExecuteCommand(&RegEntry->Value,
                                       *MuSessionId,
                                       ProcessId,
                                       SMP_SUBSYSTEM_FLAG);
        }
        if (!NT_SUCCESS(Status))
        {
            DbgPrint("SMSS: Subsystem execute failed (%wZ)\n", &RegEntry->Value);
            return Status;
        }

        /* Move to the next entry */
        NextEntry = NextEntry->Flink;
    }

    /* Process the "Execute" list now */
    NextEntry = SmpExecuteList.Blink;
    if (NextEntry != &SmpExecuteList)
    {
        /* Get the custom initial command */
        RegEntry = CONTAINING_RECORD(NextEntry, SMP_REGISTRY_VALUE, Entry);

        /* Write the initial command and wait for 5 seconds (why??!) */
        *InitialCommand = RegEntry->Name;
        Timeout.QuadPart = -50000000;
        NtDelayExecution(FALSE, &Timeout);
    }
    else
    {
        /* Use the default Winlogon initial command */
        RtlInitUnicodeString(InitialCommand, L"winlogon.exe");
        InitialCommandBuffer[0] = UNICODE_NULL;

        /* Check if there's a debugger for Winlogon */
        Status2 = LdrQueryImageFileExecutionOptions(InitialCommand,
                                                    L"Debugger",
                                                    REG_SZ,
                                                    InitialCommandBuffer,
                                                    sizeof(InitialCommandBuffer) -
                                                    InitialCommand->Length,
                                                    NULL);
        if ((NT_SUCCESS(Status2)) && (InitialCommandBuffer[0]))
        {
            /* Put the debugger string with the Winlogon string */
            wcscat(InitialCommandBuffer, L" ");
            wcscat(InitialCommandBuffer, InitialCommand->Buffer);
            RtlInitUnicodeString(InitialCommand, InitialCommandBuffer);
        }
    }

    /* Finally check if there was a custom initial command */
    NextEntry = SmpExecuteList.Flink;
    while (NextEntry != &SmpExecuteList)
    {
        /* Execute each one */
        RegEntry = CONTAINING_RECORD(NextEntry, SMP_REGISTRY_VALUE, Entry);
        SmpExecuteCommand(&RegEntry->Name, *MuSessionId, NULL, 0);
        NextEntry = NextEntry->Flink;
    }

    /* Return status */
    return Status;
}
