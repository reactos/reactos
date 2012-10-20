/*
 * subsystems/win32/csrss/csrsrv/api/process.c
 *
 * "\windows\ApiPort" port process management functions
 *
 * ReactOS Operating System
 */

/* INCLUDES ******************************************************************/

#include <srv.h>

#define NDEBUG
#include <debug.h>
    
extern NTSTATUS CallProcessCreated(PCSR_PROCESS, PCSR_PROCESS);

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

/**********************************************************************
 *	CSRSS API
 *********************************************************************/

CSR_API(CsrSrvCreateProcess)
{
     NTSTATUS Status;
     HANDLE ProcessHandle, ThreadHandle;
     PCSR_THREAD CsrThread;
     PCSR_PROCESS Process, NewProcess;
     ULONG Flags, VdmPower = 0, DebugFlags = 0;

     /* Get the current client thread */
     CsrThread = CsrGetClientThread();
     ASSERT(CsrThread != NULL);

     Process = CsrThread->Process;

     /* Extract the flags out of the process handle */
     Flags = (ULONG_PTR)ApiMessage->Data.CreateProcessRequest.ProcessHandle & 3;
     ApiMessage->Data.CreateProcessRequest.ProcessHandle = (HANDLE)((ULONG_PTR)ApiMessage->Data.CreateProcessRequest.ProcessHandle & ~3);

     /* Duplicate the process handle */
     Status = NtDuplicateObject(Process->ProcessHandle,
                                ApiMessage->Data.CreateProcessRequest.ProcessHandle,
                                NtCurrentProcess(),
                                &ProcessHandle,
                                0,
                                0,
                                DUPLICATE_SAME_ACCESS);
     if (!NT_SUCCESS(Status))
     {
         DPRINT1("Failed to duplicate process handle\n");
         return Status;
     }

     /* Duplicate the thread handle */
     Status = NtDuplicateObject(Process->ProcessHandle,
                                ApiMessage->Data.CreateProcessRequest.ThreadHandle,
                                NtCurrentProcess(),
                                &ThreadHandle,
                                0,
                                0,
                                DUPLICATE_SAME_ACCESS);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to duplicate process handle\n");
        NtClose(ProcessHandle);
        return Status;
    }

    /* See if this is a VDM process */
    if (VdmPower)
    {
        /* Request VDM powers */
        Status = NtSetInformationProcess(ProcessHandle,
                                         ProcessWx86Information,
                                         &VdmPower,
                                         sizeof(VdmPower));
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to get VDM powers\n");
            NtClose(ProcessHandle);
            NtClose(ThreadHandle);
            return Status;
        }
    }
    
    /* Convert some flags. FIXME: More need conversion */
    if (ApiMessage->Data.CreateProcessRequest.CreationFlags & CREATE_NEW_PROCESS_GROUP)
    {
        DebugFlags |= CsrProcessCreateNewGroup;
    }

    /* FIXME: SxS Stuff */

    /* Call CSRSRV to create the CSR_PROCESS structure and the first CSR_THREAD */
    Status = CsrCreateProcess(ProcessHandle,
                              ThreadHandle,
                              &ApiMessage->Data.CreateProcessRequest.ClientId,
                              Process->NtSession,
                              DebugFlags,
                              NULL);
    if (Status == STATUS_THREAD_IS_TERMINATING)
    {
        DPRINT1("Thread already dead\n");
        return Status;
    }

    /* Check for other failures */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create process/thread structures: %lx\n", Status);
        return Status;
    }

    /* FIXME: Should notify user32 */

    /* FIXME: VDM vodoo */
    
    /* ReactOS Compatibility */
    Status = CsrLockProcessByClientId(ApiMessage->Data.CreateProcessRequest.ClientId.UniqueProcess, &NewProcess);
    ASSERT(Status == STATUS_SUCCESS);
    if (!(ApiMessage->Data.CreateProcessRequest.CreationFlags & (CREATE_NEW_CONSOLE | DETACHED_PROCESS)))
    {
        NewProcess->ParentConsole = Process->Console;
        NewProcess->bInheritHandles = ApiMessage->Data.CreateProcessRequest.bInheritHandles;
    }
    RtlInitializeCriticalSection(&NewProcess->HandleTableLock);
    CallProcessCreated(Process, NewProcess);
    CsrUnlockProcess(NewProcess);

    /* Return the result of this operation */
    return Status;
}

CSR_API(CsrSrvCreateThread)
{
    PCSR_THREAD CurrentThread;
    HANDLE ThreadHandle;
    NTSTATUS Status;
    PCSR_PROCESS CsrProcess;
    
    /* Get the current CSR thread */
    CurrentThread = CsrGetClientThread();
    if (!CurrentThread)
    {
        DPRINT1("Server Thread TID: [%lx.%lx]\n",
                ApiMessage->Data.CreateThreadRequest.ClientId.UniqueProcess,
                ApiMessage->Data.CreateThreadRequest.ClientId.UniqueThread);
        return STATUS_SUCCESS; // server-to-server
    }

    /* Get the CSR Process for this request */
    CsrProcess = CurrentThread->Process;
    if (CsrProcess->ClientId.UniqueProcess !=
        ApiMessage->Data.CreateThreadRequest.ClientId.UniqueProcess)
    {
        /* This is a remote thread request -- is it within the server itself? */
        if (ApiMessage->Data.CreateThreadRequest.ClientId.UniqueProcess == NtCurrentTeb()->ClientId.UniqueProcess)
        {
            /* Accept this without any further work */
            return STATUS_SUCCESS;
        }

        /* Get the real CSR Process for the remote thread's process */
        Status = CsrLockProcessByClientId(ApiMessage->Data.CreateThreadRequest.ClientId.UniqueProcess,
                                          &CsrProcess);
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Duplicate the thread handle so we can own it */
    Status = NtDuplicateObject(CurrentThread->Process->ProcessHandle,
                               ApiMessage->Data.CreateThreadRequest.ThreadHandle,
                               NtCurrentProcess(),
                               &ThreadHandle,
                               0,
                               0,
                               DUPLICATE_SAME_ACCESS);
    if (NT_SUCCESS(Status))
    {
        /* Call CSRSRV to tell it about the new thread */
        Status = CsrCreateThread(CsrProcess,
                                 ThreadHandle,
                                 &ApiMessage->Data.CreateThreadRequest.ClientId);
    }

    /* Unlock the process and return */
    if (CsrProcess != CurrentThread->Process) CsrUnlockProcess(CsrProcess);
    return Status;
}

CSR_API(CsrTerminateProcess)
{
    PCSR_THREAD CsrThread = CsrGetClientThread();
    ASSERT(CsrThread != NULL);

    /* Set magic flag so we don't reply this message back */
    ApiMessage->ApiNumber = 0xBABE;

    /* Remove the CSR_THREADs and CSR_PROCESS */
    return CsrDestroyProcess(&CsrThread->ClientId,
                             (NTSTATUS)ApiMessage->Data.TerminateProcessRequest.uExitCode);
}

CSR_API(CsrConnectProcess)
{
    return STATUS_SUCCESS;
}

CSR_API(CsrGetShutdownParameters)
{
    PCSR_THREAD CsrThread = CsrGetClientThread();
    ASSERT(CsrThread);

    ApiMessage->Data.GetShutdownParametersRequest.Level = CsrThread->Process->ShutdownLevel;
    ApiMessage->Data.GetShutdownParametersRequest.Flags = CsrThread->Process->ShutdownFlags;

    return STATUS_SUCCESS;
}

CSR_API(CsrSetShutdownParameters)
{
    PCSR_THREAD CsrThread = CsrGetClientThread();
    ASSERT(CsrThread);

    CsrThread->Process->ShutdownLevel = ApiMessage->Data.SetShutdownParametersRequest.Level;
    CsrThread->Process->ShutdownFlags = ApiMessage->Data.SetShutdownParametersRequest.Flags;

    return STATUS_SUCCESS;
}

/* EOF */
