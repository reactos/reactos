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

/**
 * @brief
 * Perform various queries to the kernel debugger.
 *
 * @param[in]   Command
 * A SYSDBG_COMMAND value describing the kernel debugger command to perform.
 *
 * @param[in]   InputBuffer
 * Pointer to a user-provided input command-specific buffer, whose length
 * is given by InputBufferLength.
 *
 * @param[in]   InputBufferLength
 * The size (in bytes) of the buffer pointed by InputBuffer.
 *
 * @param[out]  OutputBuffer
 * Pointer to a user-provided command-specific output buffer, whose length
 * is given by OutputBufferLength.
 *
 * @param[in]   OutputBufferLength
 * The size (in bytes) of the buffer pointed by OutputBuffer.
 *
 * @param[out]  ReturnLength
 * Optional pointer to a ULONG variable that receives the actual length of
 * data written written in the output buffer. It is always zero, except for
 * the live dump commands where an actual non-zero length is returned.
 *
 * @return
 * STATUS_SUCCESS in case of success, or a proper error code otherwise.
 *
 * @remarks
 *
 * - The caller must have SeDebugPrivilege, otherwise the function fails
 *   with STATUS_ACCESS_DENIED.
 *
 * - Only the live dump commands: SysDbgGetTriageDump, and SysDbgGetLiveKernelDump
 *   (Win8.1+) are available even if the debugger is disabled or absent.
 *
 * - The following system-critical commands are not accessible anymore
 *   for user-mode usage with this API on NT 5.2+ (Windows 2003 SP1 and later)
 *   systems:
 *
 *   SysDbgQueryVersion,
 *   SysDbgReadVirtual and SysDbgWriteVirtual,
 *   SysDbgReadPhysical and SysDbgWritePhysical,
 *   SysDbgReadControlSpace and SysDbgWriteControlSpace,
 *   SysDbgReadIoSpace and SysDbgWriteIoSpace,
 *   SysDbgReadMsr and SysDbgWriteMsr,
 *   SysDbgReadBusData and SysDbgWriteBusData,
 *   SysDbgCheckLowMemory.
 *
 *   For these, NtSystemDebugControl() will return STATUS_NOT_IMPLEMENTED.
 *   They are now available from kernel-mode only with KdSystemDebugControl().
 *
 * @note
 * See: https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2004-2339
 *
 * @see KdSystemDebugControl()
 **/
NTSTATUS
NTAPI
NtSystemDebugControl(
    _In_ SYSDBG_COMMAND Command,
    _In_reads_bytes_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_opt_ PULONG ReturnLength)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    ULONG Length = 0;
    NTSTATUS Status;

    /* Debugger controlling requires the debug privilege */
    if (!SeSinglePrivilegeCheck(SeDebugPrivilege, PreviousMode))
        return STATUS_ACCESS_DENIED;

    _SEH2_TRY
    {
        if (PreviousMode != KernelMode)
        {
            if (InputBufferLength)
                ProbeForRead(InputBuffer, InputBufferLength, sizeof(ULONG));
            if (OutputBufferLength)
                ProbeForWrite(OutputBuffer, OutputBufferLength, sizeof(ULONG));
            if (ReturnLength)
                ProbeForWriteUlong(ReturnLength);
        }

        switch (Command)
        {
            case SysDbgQueryModuleInformation:
                /* Removed in WinNT4 */
                Status = STATUS_INVALID_INFO_CLASS;
                break;

#ifdef _M_IX86
            case SysDbgQueryTraceInformation:
            case SysDbgSetTracepoint:
            case SysDbgSetSpecialCall:
            case SysDbgClearSpecialCalls:
            case SysDbgQuerySpecialCalls:
                UNIMPLEMENTED;
                Status = STATUS_NOT_IMPLEMENTED;
                break;
#endif

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
                /* Those are implemented in KdSystemDebugControl */
                if (InitIsWinPEMode)
                {
                    Status = KdSystemDebugControl(Command,
                                                  InputBuffer, InputBufferLength,
                                                  OutputBuffer, OutputBufferLength,
                                                  &Length, PreviousMode);
                }
                else
                {
                    Status = STATUS_NOT_IMPLEMENTED;
                }
                break;

            case SysDbgBreakPoint:
                if (KdDebuggerEnabled)
                {
                    DbgBreakPointWithStatus(DBG_STATUS_DEBUG_CONTROL);
                    Status = STATUS_SUCCESS;
                }
                else
                {
                    Status = STATUS_UNSUCCESSFUL;
                }
                break;

            case SysDbgEnableKernelDebugger:
                Status = KdEnableDebugger();
                break;

            case SysDbgDisableKernelDebugger:
                Status = KdDisableDebugger();
                break;

            case SysDbgGetAutoKdEnable:
                if (OutputBufferLength != sizeof(BOOLEAN))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                }
                else
                {
                    *(PBOOLEAN)OutputBuffer = KdAutoEnableOnEvent;
                    Status = STATUS_SUCCESS;
                }
                break;

            case SysDbgSetAutoKdEnable:
                if (InputBufferLength != sizeof(BOOLEAN))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                }
                else if (KdPitchDebugger)
                {
                    Status = STATUS_ACCESS_DENIED;
                }
                else
                {
                    KdAutoEnableOnEvent = *(PBOOLEAN)InputBuffer;
                    Status = STATUS_SUCCESS;
                }
                break;

            case SysDbgGetPrintBufferSize:
                if (OutputBufferLength != sizeof(ULONG))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                }
                else
                {
                    /* Return buffer size only if KD is enabled */
                    *(PULONG)OutputBuffer = KdPitchDebugger ? 0 : KdPrintBufferSize;
                    Status = STATUS_SUCCESS;
                }
                break;

            case SysDbgSetPrintBufferSize:
                UNIMPLEMENTED;
                Status = STATUS_NOT_IMPLEMENTED;
                break;

            case SysDbgGetKdUmExceptionEnable:
                if (OutputBufferLength != sizeof(BOOLEAN))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                }
                else
                {
                    /* Unfortunately, the internal flag says if UM exceptions are disabled */
                    *(PBOOLEAN)OutputBuffer = !KdIgnoreUmExceptions;
                    Status = STATUS_SUCCESS;
                }
                break;

            case SysDbgSetKdUmExceptionEnable:
                if (InputBufferLength != sizeof(BOOLEAN))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                }
                else if (KdPitchDebugger)
                {
                    Status = STATUS_ACCESS_DENIED;
                }
                else
                {
                    /* Unfortunately, the internal flag says if UM exceptions are disabled */
                    KdIgnoreUmExceptions = !*(PBOOLEAN)InputBuffer;
                    Status = STATUS_SUCCESS;
                }
                break;

            case SysDbgGetTriageDump:
                UNIMPLEMENTED;
                Status = STATUS_NOT_IMPLEMENTED;
                break;

            case SysDbgGetKdBlockEnable:
                if (OutputBufferLength != sizeof(BOOLEAN))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                }
                else
                {
                    *(PBOOLEAN)OutputBuffer = KdBlockEnable;
                    Status = STATUS_SUCCESS;
                }
                break;

            case SysDbgSetKdBlockEnable:
                Status = KdChangeOption(KD_OPTION_SET_BLOCK_ENABLE,
                                        InputBufferLength,
                                        InputBuffer,
                                        OutputBufferLength,
                                        OutputBuffer,
                                        &Length);
                break;

            default:
                Status = STATUS_INVALID_INFO_CLASS;
                break;
        }

        if (ReturnLength)
            *ReturnLength = Length;

        _SEH2_YIELD(return Status);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;
}
