/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ex/dbgctrl.c
 * PURPOSE:         System debug control
 * PROGRAMMERS:     Alex Ionescu
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* DATA **********************************************************************/

/*
 * WinDBG Debugger Worker State Machine data
 */
WORK_QUEUE_ITEM ExpDebuggerWorkItem;
/*
 * The following global variables must be visible through all the kernel
 * because WinDBG explicitely search for them inside our symbols.
 */
WINKD_WORKER_STATE ExpDebuggerWork;
PEPROCESS ExpDebuggerProcessAttach;
PEPROCESS ExpDebuggerProcessKill;
ULONG_PTR ExpDebuggerPageIn;

/* FUNCTIONS *****************************************************************/

/*
 * WinDBG Debugger Worker State Machine
 *
 * This functionality is used whenever WinDBG wants to attach or kill a user-mode
 * process from within live kernel-mode session, and/or page-in an address region.
 * It is implemented as a state machine: when it is in "Ready" state, WinDBG can
 * initialize the data for the state machine, then switch its state to "Start".
 * The worker thread balance manager detects this, switches the state to "Initialized"
 * and queues a worker thread. As long as the state is not "Ready" again, WinDBG
 * prevents from requeuing a new thread. When the thread is started, it captures
 * all the data, then resets the machine state to "Ready", thus allowing WinDBG
 * to requeue another worker thread.
 *
 * WinDBG commands:
 *     .process /i <addr> (where <addr> is the address of the EPROCESS block for this process)
 *     .kill <addr>       (       "                "                "                "       )
 *     .pagein <addr>     (where <addr> is the address to page in)
 */
VOID
NTAPI
ExpDebuggerWorker(
    _In_ PVOID Context)
{
    PEPROCESS ProcessToAttach, ProcessToKill;
    ULONG_PTR PageInAddress;
    PEPROCESS Process;
    KAPC_STATE ApcState;

    UNREFERENCED_PARAMETER(Context);

    /* Be sure we were started in an initialized state */
    ASSERTMSG("ExpDebuggerWorker being entered in non-initialized state!\n",
              ExpDebuggerWork == WinKdWorkerInitialized);
    if (ExpDebuggerWork != WinKdWorkerInitialized)
    {
        /* An error happened, so get a chance to restart proper */
        ExpDebuggerWork = WinKdWorkerReady;
        return;
    }

    /* Get the processes to be attached or killed, and the address to page in */
    ProcessToAttach = ExpDebuggerProcessAttach;
    ProcessToKill   = ExpDebuggerProcessKill;
    PageInAddress   = ExpDebuggerPageIn;

    /* Reset the state machine to its ready state */
    ExpDebuggerProcessAttach = NULL;
    ExpDebuggerProcessKill   = NULL;
    ExpDebuggerPageIn = (ULONG_PTR)NULL;
    ExpDebuggerWork = WinKdWorkerReady;

    /* Default to the current process if we don't find the process to be attached or killed */
    Process = NULL;

    /* Check if we need to attach or kill some process */
    if (ProcessToAttach || ProcessToKill)
    {
        /* Find the process in the list */
        while ((Process = PsGetNextProcess(Process)))
        {
            /* Is this the process we want to attach to? */
            if (Process == ProcessToAttach)
            {
                /* Yes, attach ourselves to it */
                KeStackAttachProcess(&Process->Pcb, &ApcState);
                break;
            }
            /* Or is this the process we want to kill? */
            else if (Process == ProcessToKill)
            {
                /* Yes, kill and dereference it, then return */
                PsTerminateProcess(Process, DBG_TERMINATE_PROCESS);
                ObDereferenceObject(Process);
                return;
            }
        }

        if (!Process)
        {
            DbgPrintEx(DPFLTR_SYSTEM_ID, DPFLTR_ERROR_LEVEL,
                       "EX debug work: Unable to find process %p\n",
                       ProcessToAttach ? ProcessToAttach : ProcessToKill);
        }

        /* We either have found a process, or we default to the current one */
    }

    /* If we have an address to page in... */
    if (PageInAddress)
    {
        /* ... try to do it by attempting to read at this address */
        _SEH2_TRY
        {
            ProbeForReadUchar(PageInAddress);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            DbgPrintEx(DPFLTR_SYSTEM_ID, DPFLTR_ERROR_LEVEL,
                       "EX page in: Failed to page-in address 0x%p, Status 0x%08lx\n",
                       PageInAddress, _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Break into the process (or the current one if Process == NULL) */
    DbgBreakPointWithStatus(DBG_STATUS_WORKER);

    /* If we are attached to a process, not the current one... */
    if (Process)
    {
        /* ... we can detach from the process */
        KeUnstackDetachProcess(&ApcState);
        /* Dereference the process referenced by PsGetNextProcess() */
        ObDereferenceObject(Process);
    }
}

/*++
 * @name NtSystemDebugControl
 * @implemented
 *
 * Perform various queries to debugger.
 * This API is subject to test-case creation to further evaluate its
 * abilities (if needed to at all)
 *
 * See: http://www.osronline.com/showthread.cfm?link=93915
 *      http://void.ru/files/Ntexapi.h
 *      http://www.codeguru.com/code/legacy/system/ntexapi.zip
 *      http://www.securityfocus.com/bid/9694
 *
 * @param ControlCode
 *        Description of the parameter. Wrapped to more lines on ~70th
 *        column.
 *
 * @param InputBuffer
 *        FILLME
 *
 * @param InputBufferLength
 *        FILLME
 *
 * @param OutputBuffer
 *        FILLME
 *
 * @param OutputBufferLength
 *        FILLME
 *
  * @param ReturnLength
 *        FILLME
 *
 * @return STATUS_SUCCESS in case of success, proper error code otherwise
 *
 * @remarks None
 *
 *--*/
NTSTATUS
NTAPI
NtSystemDebugControl(SYSDBG_COMMAND ControlCode,
                     PVOID InputBuffer,
                     ULONG InputBufferLength,
                     PVOID OutputBuffer,
                     ULONG OutputBufferLength,
                     PULONG ReturnLength)
{
    switch (ControlCode)
    {
        case SysDbgQueryModuleInformation:
        case SysDbgQueryTraceInformation:
        case SysDbgSetTracepoint:
        case SysDbgSetSpecialCall:
        case SysDbgClearSpecialCalls:
        case SysDbgQuerySpecialCalls:
        case SysDbgQueryVersion:
        case SysDbgReadVirtual:
        case SysDbgWriteVirtual:
        case SysDbgReadPhysical:
        case SysDbgWritePhysical:
        case SysDbgReadControlSpace:
        case SysDbgWriteControlSpace:
        case SysDbgReadIoSpace:
        case SysDbgWriteIoSpace:
        case SysDbgReadMsr:
        case SysDbgWriteMsr:
        case SysDbgReadBusData:
        case SysDbgWriteBusData:
        case SysDbgCheckLowMemory:
        case SysDbgGetTriageDump:
            return STATUS_NOT_IMPLEMENTED;
        case SysDbgBreakPoint:
        case SysDbgEnableKernelDebugger:
        case SysDbgDisableKernelDebugger:
        case SysDbgGetAutoKdEnable:
        case SysDbgSetAutoKdEnable:
        case SysDbgGetPrintBufferSize:
        case SysDbgSetPrintBufferSize:
        case SysDbgGetKdUmExceptionEnable:
        case SysDbgSetKdUmExceptionEnable:
        case SysDbgGetKdBlockEnable:
        case SysDbgSetKdBlockEnable:
            return KdSystemDebugControl(
                ControlCode,
                InputBuffer, InputBufferLength,
                OutputBuffer, OutputBufferLength,
                ReturnLength, KeGetPreviousMode());
        default:
            return STATUS_INVALID_INFO_CLASS;
    }
}
