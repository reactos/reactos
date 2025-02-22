/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kddll/utils.c
 * PURPOSE:         Misc helper functions.
 */

#include "kdgdb.h"

/*
 * We cannot use PsLookupProcessThreadByCid or alike as we could be running at any IRQL.
 * So we have to loop over the process list.
 */

PEPROCESS
find_process(
    _In_ UINT_PTR Pid)
{
    HANDLE ProcessId = gdb_pid_to_handle(Pid);
    LIST_ENTRY* ProcessEntry;
    PEPROCESS Process;

    /* Special case for idle process */
    if (ProcessId == NULL)
        return TheIdleProcess;

    for (ProcessEntry = ProcessListHead->Flink;
            ProcessEntry != ProcessListHead;
            ProcessEntry = ProcessEntry->Flink)
    {
        Process = CONTAINING_RECORD(ProcessEntry, EPROCESS, ActiveProcessLinks);

        if (Process->UniqueProcessId == ProcessId)
            return Process;
    }

    return NULL;
}

PETHREAD
find_thread(
    _In_ UINT_PTR Pid,
    _In_ UINT_PTR Tid)
{
    HANDLE ThreadId = gdb_tid_to_handle(Tid);
    PETHREAD Thread;
    PEPROCESS Process;
    LIST_ENTRY* ThreadEntry;
#if MONOPROCESS
    LIST_ENTRY* ProcessEntry;
#endif

    if (
#if !MONOPROCESS
    (Pid == 0) &&
#endif
    (Tid == 0))
    {
        /* Zero means any, so use the current one */
        return (PETHREAD)(ULONG_PTR)CurrentStateChange.Thread;
    }

#if MONOPROCESS

    /* Special case for the idle thread */
    if (Tid == 1)
        return TheIdleThread;

    for (ProcessEntry = ProcessListHead->Flink;
        ProcessEntry != ProcessListHead;
        ProcessEntry = ProcessEntry->Flink)
    {
        Process = CONTAINING_RECORD(ProcessEntry, EPROCESS, ActiveProcessLinks);
#else

    Process = find_process(Pid);

    /* Special case for the idle thread */
    if ((Process == TheIdleProcess) && (Tid == 1))
        return TheIdleThread;

    if (!Process)
        return NULL;

#endif

    for (ThreadEntry = Process->ThreadListHead.Flink;
            ThreadEntry != &Process->ThreadListHead;
            ThreadEntry = ThreadEntry->Flink)
    {
        Thread = CONTAINING_RECORD(ThreadEntry, ETHREAD, ThreadListEntry);
        /* For GDB, Tid == 0 means any thread */
        if ((Thread->Cid.UniqueThread == ThreadId) || (Tid == 0))
        {
            return Thread;
        }
    }

#if MONOPROCESS
    }
#endif

    return NULL;
}
