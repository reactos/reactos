/*
 * PROJECT:         ReactOS Windows-Compatible Session Manager
 * LICENSE:         BSD 2-Clause License
 * FILE:            base/system/smss/smsbapi.c
 * PURPOSE:         Main SMSS Code
 * PROGRAMMERS:     Alex Ionescu
 */

/* INCLUDES *******************************************************************/

#include "smss.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PCHAR SmpSubSystemNames[] =
{
    "Unknown",
    "Native",
    "Windows",
    "Posix",
    "OS/2"
};

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
SmpSbCreateSession(IN PVOID Reserved,
                   IN PSMP_SUBSYSTEM OtherSubsystem,
                   IN PRTL_USER_PROCESS_INFORMATION ProcessInformation,
                   IN ULONG MuSessionId,
                   IN PCLIENT_ID DbgClientId)
{
    NTSTATUS Status;
    PSMP_SUBSYSTEM KnownSubsys;
    SB_API_MSG SbApiMsg;
    ULONG SessionId;
    PSB_CREATE_SESSION_MSG CreateSessionMsg;

    /* Write out the create session message including its initial process */
    CreateSessionMsg = &SbApiMsg.CreateSession;
    CreateSessionMsg->ProcessInfo = *ProcessInformation;
    CreateSessionMsg->MuSessionId = MuSessionId;
    if (DbgClientId)
    {
        CreateSessionMsg->ClientId = *DbgClientId;
    }
    else
    {
        CreateSessionMsg->ClientId.UniqueThread = NULL;
        CreateSessionMsg->ClientId.UniqueProcess = NULL;
    }

    /* Find a subsystem responsible for this session */
    SmpGetProcessMuSessionId(ProcessInformation->ProcessHandle, &MuSessionId);
    if (!SmpCheckDuplicateMuSessionId(MuSessionId))
    {
        NtClose(ProcessInformation->ProcessHandle);
        NtClose(ProcessInformation->ThreadHandle);
        DPRINT1("SMSS: CreateSession status=%x\n", STATUS_OBJECT_NAME_NOT_FOUND);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    /* Find the subsystem we have for this initial process */
    KnownSubsys = SmpLocateKnownSubSysByType(MuSessionId,
                                             ProcessInformation->
                                             ImageInformation.SubSystemType);
    if (KnownSubsys)
    {
        /* Duplicate the process handle into the message */
        Status = NtDuplicateObject(NtCurrentProcess(),
                                   ProcessInformation->ProcessHandle,
                                   KnownSubsys->ProcessHandle,
                                   &CreateSessionMsg->ProcessInfo.ProcessHandle,
                                   PROCESS_ALL_ACCESS,
                                   0,
                                   0);
        if (NT_SUCCESS(Status))
        {
            /* Duplicate the thread handle into the message */
            Status = NtDuplicateObject(NtCurrentProcess(),
                                       ProcessInformation->ThreadHandle,
                                       KnownSubsys->ProcessHandle,
                                       &CreateSessionMsg->ProcessInfo.ThreadHandle,
                                       THREAD_ALL_ACCESS,
                                       0,
                                       0);
            if (!NT_SUCCESS(Status))
            {
                /* Close everything on failure */
                NtClose(ProcessInformation->ProcessHandle);
                NtClose(ProcessInformation->ThreadHandle);
                SmpDereferenceSubsystem(KnownSubsys);
                DbgPrint("SmpSbCreateSession: NtDuplicateObject (Thread) Failed %lx\n", Status);
                return Status;
            }

            /* Close the original handles as they are no longer needed */
            NtClose(ProcessInformation->ProcessHandle);
            NtClose(ProcessInformation->ThreadHandle);

            /* Finally, allocate a new SMSS session ID for this session */
            SessionId = SmpAllocateSessionId(KnownSubsys, OtherSubsystem);
            CreateSessionMsg->SessionId = SessionId;

            /* Fill out the LPC message header and send it to the client! */
            SbApiMsg.ApiNumber = SbpCreateSession;
            SbApiMsg.h.u2.ZeroInit = 0;
            SbApiMsg.h.u1.s1.DataLength = sizeof(SB_CREATE_SESSION_MSG) + 8;
            SbApiMsg.h.u1.s1.TotalLength = sizeof(SbApiMsg);
            Status = NtRequestWaitReplyPort(KnownSubsys->SbApiPort,
                                            &SbApiMsg.h,
                                            &SbApiMsg.h);
            if (!NT_SUCCESS(Status))
            {
                /* Bail out */
                DPRINT1("SmpSbCreateSession: NtRequestWaitReply Failed %lx\n", Status);
            }
            else
            {
                /* If the API succeeded, get the result value from the LPC */
                Status = SbApiMsg.ReturnValue;
            }

            /* Delete the session on any kind of failure */
            if (!NT_SUCCESS(Status)) SmpDeleteSession(SessionId);
        }
        else
        {
            /* Close the handles on failure */
            DPRINT1("SmpSbCreateSession: NtDuplicateObject (Process) Failed %lx\n", Status);
            NtClose(ProcessInformation->ProcessHandle);
            NtClose(ProcessInformation->ThreadHandle);
        }

        /* Dereference the subsystem and return the status of the LPC call */
        SmpDereferenceSubsystem(KnownSubsys);
        return Status;
    }

    /* If we don't yet have a subsystem, only native images can be launched */
    if (ProcessInformation->ImageInformation.SubSystemType != IMAGE_SUBSYSTEM_NATIVE)
    {
        /* Fail */
        DPRINT1("SMSS: %s SubSystem has not been started.\n",
                SmpSubSystemNames[ProcessInformation->ImageInformation.SubSystemType]);
        Status = STATUS_UNSUCCESSFUL;
        NtClose(ProcessInformation->ProcessHandle);
        NtClose(ProcessInformation->ThreadHandle);
        return Status;
    }

#if 0
    /* This code handles debug applications, but it seems vestigial... */
    if ((*(ULONGLONG)&CreateSessionMsg.ClientId) && (SmpDbgSsLoaded))
    {
        Process = RtlAllocateHeap(SmpHeap, SmBaseTag, sizeof(SMP_PROCESS));
        if (!Process)
        {
            DPRINT1("Unable to initialize debugging for Native App %lx.%lx -- out of memory\n",
                    ProcessInformation->ClientId.UniqueProcess,
                    ProcessInformation->ClientId.UniqueThread);
            NtClose(ProcessInformation->ProcessHandle);
            NtClose(ProcessInformation->ThreadHandle);
            return STATUS_NO_MEMORY;
        }

        Process->DbgClientId = CreateSessionMsg->ClientId;
        Process->ClientId = ProcessInformation->ClientId;
        InsertHeadList(&NativeProcessList, &Process->Entry);
        DPRINT1("Native Debug App %lx.%lx\n", Process->ClientId.UniqueProcess, Process->ClientId.UniqueThread);

        Status = NtSetInformationProcess(ProcessInformation->ProcessHandle, 7, &SmpDebugPort, 4);
        ASSERT(NT_SUCCESS(Status));
    }
#endif

    /* This is a native application being started as the initial command */
    DPRINT1("Subsystem active, starting thread\n");
    NtClose(ProcessInformation->ProcessHandle);
    NtResumeThread(ProcessInformation->ThreadHandle, NULL);
    NtClose(ProcessInformation->ThreadHandle);
    return STATUS_SUCCESS;
}
