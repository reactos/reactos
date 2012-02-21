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
     PCSR_PROCESS NewProcessData;
     ULONG Flags, VdmPower = 0, DebugFlags = 0;

     /* Get the current client thread */
     CsrThread = NtCurrentTeb()->CsrClientThread;
     ASSERT(CsrThread != NULL);

     /* Extract the flags out of the process handle */
     Flags = (ULONG_PTR)Request->Data.CreateProcessRequest.ProcessHandle & 3;
     Request->Data.CreateProcessRequest.ProcessHandle = (HANDLE)((ULONG_PTR)Request->Data.CreateProcessRequest.ProcessHandle & ~3);

     /* Duplicate the process handle */
     Status = NtDuplicateObject(CsrThread->Process->ProcessHandle,
                                Request->Data.CreateProcessRequest.ProcessHandle,
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
     Status = NtDuplicateObject(CsrThread->Process->ProcessHandle,
                                Request->Data.CreateProcessRequest.ThreadHandle,
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
    if (Request->Data.CreateProcessRequest.CreationFlags & CREATE_NEW_PROCESS_GROUP)
    {
        DebugFlags |= CsrProcessCreateNewGroup;
    }

    /* FIXME: SxS Stuff */

    /* Call CSRSRV to create the CSR_PROCESS structure and the first CSR_THREAD */
    Status = CsrCreateProcess(ProcessHandle,
                              ThreadHandle,
                              &Request->Data.CreateProcessRequest.ClientId,
                              CsrThread->Process->NtSession,
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
    Status = CsrLockProcessByClientId(Request->Data.CreateProcessRequest.ClientId.UniqueProcess, &NewProcessData);
    ASSERT(Status == STATUS_SUCCESS);
    if (!(Request->Data.CreateProcessRequest.CreationFlags & (CREATE_NEW_CONSOLE | DETACHED_PROCESS)))
    {
        NewProcessData->ParentConsole = ProcessData->Console;
        NewProcessData->bInheritHandles = Request->Data.CreateProcessRequest.bInheritHandles;
    }
    RtlInitializeCriticalSection(&NewProcessData->HandleTableLock);
    CallProcessCreated(ProcessData, NewProcessData);
    CsrUnlockProcess(NewProcessData);

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
    CurrentThread = NtCurrentTeb()->CsrClientThread;
    if (!CurrentThread)
    {
        DPRINT1("Server Thread TID: [%lx.%lx]\n",
                Request->Data.CreateThreadRequest.ClientId.UniqueProcess,
                Request->Data.CreateThreadRequest.ClientId.UniqueThread);
        return STATUS_SUCCESS; // server-to-server
    }

    /* Get the CSR Process for this request */
    CsrProcess = CurrentThread->Process;
    if (CsrProcess->ClientId.UniqueProcess !=
        Request->Data.CreateThreadRequest.ClientId.UniqueProcess)
    {
        /* This is a remote thread request -- is it within the server itself? */
        if (Request->Data.CreateThreadRequest.ClientId.UniqueProcess == NtCurrentTeb()->ClientId.UniqueProcess)
        {
            /* Accept this without any further work */
            return STATUS_SUCCESS;
        }

        /* Get the real CSR Process for the remote thread's process */
        Status = CsrLockProcessByClientId(Request->Data.CreateThreadRequest.ClientId.UniqueProcess,
                                          &CsrProcess);
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Duplicate the thread handle so we can own it */
    Status = NtDuplicateObject(CurrentThread->Process->ProcessHandle,
                               Request->Data.CreateThreadRequest.ThreadHandle,
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
                                 &Request->Data.CreateThreadRequest.ClientId);
    }

    /* Unlock the process and return */
    if (CsrProcess != CurrentThread->Process) CsrUnlockProcess(CsrProcess);
    return Status;
}

CSR_API(CsrTerminateProcess)
{
    PCSR_THREAD CsrThread = NtCurrentTeb()->CsrClientThread;
    ASSERT(CsrThread != NULL);

    /* Set magic flag so we don't reply this message back */
    Request->Type = 0xBABE;

    /* Remove the CSR_THREADs and CSR_PROCESS */
    return CsrDestroyProcess(&CsrThread->ClientId,
                             (NTSTATUS)Request->Data.TerminateProcessRequest.uExitCode);
}

CSR_API(CsrConnectProcess)
{

   return(STATUS_SUCCESS);
}

CSR_API(CsrGetShutdownParameters)
{

  Request->Data.GetShutdownParametersRequest.Level = ProcessData->ShutdownLevel;
  Request->Data.GetShutdownParametersRequest.Flags = ProcessData->ShutdownFlags;

  return(STATUS_SUCCESS);
}

CSR_API(CsrSetShutdownParameters)
{

  ProcessData->ShutdownLevel = Request->Data.SetShutdownParametersRequest.Level;
  ProcessData->ShutdownFlags = Request->Data.SetShutdownParametersRequest.Flags;

  return(STATUS_SUCCESS);
}

/* EOF */
