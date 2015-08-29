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

// #ifdef _WINKD_
/*
 * WinDBG Debugger Worker Thread data
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
// #endif /* _WINKD_ */

/* FUNCTIONS *****************************************************************/

// #ifdef _WINKD_
/*
 * WinDBG Debugger Worker Thread
 *
 * A worker thread is queued whenever WinDBG wants to attach or kill a user-mode
 * process from within live kernel-mode session, and/or page-in an address region.
 *
 * WinDBG commands:
 *     .process /i <addr> (where <addr> is the address of the EPROCESS block for this process)
 *     .kill <addr>       (       "                "                "                "       )
 *     .pagein <addr>     (where <addr> is the address to page in)
 *
 * The implementation is very naive because the same data is reused, so that if
 * the worker thread has not started before WinDBG sends fresh new data again,
 * then only the latest data is taken into account.
 */
VOID
NTAPI
ExpDebuggerWorker(IN PVOID Context)
{
    PEPROCESS ProcessToAttach, ProcessToKill;
    ULONG_PTR PageInAddress;
    PEPROCESS Process;
    KAPC_STATE ApcState;

    UNREFERENCED_PARAMETER(Context);

    /* Be sure we were started in an initialized state */
    ASSERTMSG("ExpDebuggerWorker being entered with state != 2\n",
              ExpDebuggerWork == WinKdWorkerInitialized);
    if (ExpDebuggerWork != WinKdWorkerInitialized) return;

    /* Reset the worker flag to the disabled state */
    ExpDebuggerWork = WinKdWorkerDisabled;

    /* Get the processes to be attached or killed, and the address to page in */
    ProcessToAttach = ExpDebuggerProcessAttach;
    ProcessToKill   = ExpDebuggerProcessKill;
    PageInAddress   = ExpDebuggerPageIn;

    /* Reset to their default values */
    ExpDebuggerProcessAttach = NULL;
    ExpDebuggerProcessKill   = NULL;
    ExpDebuggerPageIn = (ULONG_PTR)NULL;

    /* Default to the current process if we don't find the process to be attached or killed */
    Process = NULL;

    /* Check if we need to attach or kill some process */
    if (ProcessToAttach != NULL || ProcessToKill != NULL)
    {
        /* Find the process in the list */
        Process = PsGetNextProcess(Process);
        while (Process)
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

            /* Get the next process */
            Process = PsGetNextProcess(Process);
        }

        /* We either have found a process, or we default to the current process */
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
            DPRINT1("Failed to page in address 0x%p, Status 0x%08lx\n", PageInAddress, _SEH2_GetExceptionCode());
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
        /* Dereference the process which was referenced for us by PsGetNextProcess */
        ObDereferenceObject(Process);
    }
}
// #endif /* _WINKD_ */

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
