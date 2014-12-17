/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Base API Server DLL
 * FILE:            subsystems/win/basesrv/proc.c
 * PURPOSE:         Process and Thread Management
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include "basesrv.h"
#include "vdm.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* User notification procedure to be called when a process is created */
static BASE_PROCESS_CREATE_NOTIFY_ROUTINE UserNotifyProcessCreate = NULL;

/* PUBLIC SERVER APIS *********************************************************/

CSR_API(BaseSrvDebugProcess)
{
    /* Deprecated */
    return STATUS_UNSUCCESSFUL;
}

CSR_API(BaseSrvRegisterThread)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(BaseSrvSxsCreateActivationContext)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(BaseSrvSetTermsrvAppInstallMode)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(BaseSrvSetTermsrvClientTimeZone)
{
    DPRINT1("%s not yet implemented\n", __FUNCTION__);
    return STATUS_NOT_IMPLEMENTED;
}

CSR_API(BaseSrvGetTempFile)
{
    static UINT BaseGetTempFileUnique = 0;
    PBASE_GET_TEMP_FILE GetTempFile = &((PBASE_API_MESSAGE)ApiMessage)->Data.GetTempFileRequest;

    /* Return 16-bits ID */
    GetTempFile->UniqueID = (++BaseGetTempFileUnique & 0xFFFF);

    DPRINT("Returning: %u\n", GetTempFile->UniqueID);

    return STATUS_SUCCESS;
}

CSR_API(BaseSrvCreateProcess)
{
    NTSTATUS Status;
    PBASE_CREATE_PROCESS CreateProcessRequest = &((PBASE_API_MESSAGE)ApiMessage)->Data.CreateProcessRequest;
    HANDLE ProcessHandle, ThreadHandle;
    PCSR_THREAD CsrThread;
    PCSR_PROCESS Process;
    ULONG Flags = 0, DebugFlags = 0, VdmPower = 0;

    /* Get the current client thread */
    CsrThread = CsrGetClientThread();
    ASSERT(CsrThread != NULL);

    Process = CsrThread->Process;

    /* Extract the flags out of the process handle */
    Flags = (ULONG_PTR)CreateProcessRequest->ProcessHandle & 3;
    CreateProcessRequest->ProcessHandle = (HANDLE)((ULONG_PTR)CreateProcessRequest->ProcessHandle & ~3);

    /* Some things should be done if this is a VDM process */
    if (CreateProcessRequest->VdmBinaryType)
    {
        /* We need to set the VDM power later on */
        VdmPower = 1;
    }

    /* Duplicate the process handle */
    Status = NtDuplicateObject(Process->ProcessHandle,
                               CreateProcessRequest->ProcessHandle,
                               NtCurrentProcess(),
                               &ProcessHandle,
                               0,
                               0,
                               DUPLICATE_SAME_ACCESS);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to duplicate process handle: %lx\n", Status);
        return Status;
    }

    /* Duplicate the thread handle */
    Status = NtDuplicateObject(Process->ProcessHandle,
                               CreateProcessRequest->ThreadHandle,
                               NtCurrentProcess(),
                               &ThreadHandle,
                               0,
                               0,
                               DUPLICATE_SAME_ACCESS);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to duplicate thread handle: %lx\n", Status);
        NtClose(ProcessHandle);
        return Status;
    }

    /* If this is a VDM process, request VDM power */
    if (VdmPower)
    {
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

    /* Flags conversion. FIXME: More need conversion */
    if (CreateProcessRequest->CreationFlags & CREATE_NEW_PROCESS_GROUP)
    {
        DebugFlags |= CsrProcessCreateNewGroup;
    }
    if ((Flags & 2) == 0)
    {
        /* We are launching a console process */
        DebugFlags |= CsrProcessIsConsoleApp;
    }

    /* FIXME: SxS Stuff */

    /* Call CSRSRV to create the CSR_PROCESS structure and the first CSR_THREAD */
    Status = CsrCreateProcess(ProcessHandle,
                              ThreadHandle,
                              &CreateProcessRequest->ClientId,
                              Process->NtSession,
                              DebugFlags,
                              NULL);
    if (Status == STATUS_THREAD_IS_TERMINATING)
    {
        DPRINT1("Thread already dead\n");

        /* Set the special reply value so we don't reply this message back */
        *ReplyCode = CsrReplyDeadClient;

        return Status;
    }

    /* Check for other failures */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create process/thread structures: %lx\n", Status);
        return Status;
    }

    /* Call the user notification procedure */
    if (UserNotifyProcessCreate)
    {
        UserNotifyProcessCreate(CreateProcessRequest->ClientId.UniqueProcess,
                                Process->ClientId.UniqueThread,
                                0,
                                Flags);
    }

    /* Check if this is a VDM process */
    if (CreateProcessRequest->VdmBinaryType)
    {
        PVDM_CONSOLE_RECORD ConsoleRecord;

        if (CreateProcessRequest->VdmTask != 0)
        {
            /* Get the console record using the task ID */
            Status = GetConsoleRecordBySessionId(CreateProcessRequest->VdmTask,
                                                 &ConsoleRecord);
        }
        else
        {
            /* Get the console record using the console handle */
            Status = BaseSrvGetConsoleRecord(CreateProcessRequest->hVDM,
                                             &ConsoleRecord);
        }

        /* Check if it failed */
        if (!NT_SUCCESS(Status)) return Status;

        /* Store the process ID of the VDM in the console record */
        ConsoleRecord->ProcessId = HandleToUlong(CreateProcessRequest->ClientId.UniqueProcess);
    }

    /* Return the result of this operation */
    return Status;
}

CSR_API(BaseSrvCreateThread)
{
    NTSTATUS Status;
    PBASE_CREATE_THREAD CreateThreadRequest = &((PBASE_API_MESSAGE)ApiMessage)->Data.CreateThreadRequest;
    PCSR_THREAD CurrentThread;
    HANDLE ThreadHandle;
    PCSR_PROCESS CsrProcess;

    /* Get the current CSR thread */
    CurrentThread = CsrGetClientThread();
    if (!CurrentThread)
    {
        DPRINT1("Server Thread TID: [%lx.%lx]\n",
                CreateThreadRequest->ClientId.UniqueProcess,
                CreateThreadRequest->ClientId.UniqueThread);
        return STATUS_SUCCESS; // server-to-server
    }

    /* Get the CSR Process for this request */
    CsrProcess = CurrentThread->Process;
    if (CsrProcess->ClientId.UniqueProcess !=
        CreateThreadRequest->ClientId.UniqueProcess)
    {
        /* This is a remote thread request -- is it within the server itself? */
        if (CreateThreadRequest->ClientId.UniqueProcess == NtCurrentTeb()->ClientId.UniqueProcess)
        {
            /* Accept this without any further work */
            return STATUS_SUCCESS;
        }

        /* Get the real CSR Process for the remote thread's process */
        Status = CsrLockProcessByClientId(CreateThreadRequest->ClientId.UniqueProcess,
                                          &CsrProcess);
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Duplicate the thread handle so we can own it */
    Status = NtDuplicateObject(CurrentThread->Process->ProcessHandle,
                               CreateThreadRequest->ThreadHandle,
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
                                 &CreateThreadRequest->ClientId,
                                 TRUE);
    }

    /* Unlock the process and return */
    if (CsrProcess != CurrentThread->Process) CsrUnlockProcess(CsrProcess);
    return Status;
}

CSR_API(BaseSrvExitProcess)
{
    PCSR_THREAD CsrThread = CsrGetClientThread();
    ASSERT(CsrThread != NULL);

    /* Set the special reply value so we don't reply this message back */
    *ReplyCode = CsrReplyDeadClient;

    /* Remove the CSR_THREADs and CSR_PROCESS */
    return CsrDestroyProcess(&CsrThread->ClientId,
                             (NTSTATUS)((PBASE_API_MESSAGE)ApiMessage)->Data.ExitProcessRequest.uExitCode);
}

CSR_API(BaseSrvGetProcessShutdownParam)
{
    PBASE_GETSET_PROCESS_SHUTDOWN_PARAMS ShutdownParametersRequest = &((PBASE_API_MESSAGE)ApiMessage)->Data.ShutdownParametersRequest;
    PCSR_THREAD CsrThread = CsrGetClientThread();
    ASSERT(CsrThread);

    ShutdownParametersRequest->ShutdownLevel = CsrThread->Process->ShutdownLevel;
    ShutdownParametersRequest->ShutdownFlags = CsrThread->Process->ShutdownFlags;

    return STATUS_SUCCESS;
}

CSR_API(BaseSrvSetProcessShutdownParam)
{
    PBASE_GETSET_PROCESS_SHUTDOWN_PARAMS ShutdownParametersRequest = &((PBASE_API_MESSAGE)ApiMessage)->Data.ShutdownParametersRequest;
    PCSR_THREAD CsrThread = CsrGetClientThread();
    ASSERT(CsrThread);

    CsrThread->Process->ShutdownLevel = ShutdownParametersRequest->ShutdownLevel;
    CsrThread->Process->ShutdownFlags = ShutdownParametersRequest->ShutdownFlags;

    return STATUS_SUCCESS;
}

/* PUBLIC API *****************************************************************/

VOID
NTAPI
BaseSetProcessCreateNotify(IN BASE_PROCESS_CREATE_NOTIFY_ROUTINE ProcessCreateNotifyProc)
{
    /* Set the user notification procedure to be called when a process is created */
    UserNotifyProcessCreate = ProcessCreateNotifyProc;
}

/* EOF */
