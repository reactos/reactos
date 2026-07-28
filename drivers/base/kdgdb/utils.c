/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kddll/utils.c
 * PURPOSE:         Misc helper functions.
 */

#include "kdgdb.h"

/* Decode a run of hex digit pairs into raw bytes */
BOOLEAN
gdb_decode_hex(
    _In_reads_(InputLength) const CHAR* Input,
    _In_ ULONG InputLength,
    _Out_writes_bytes_(OutputLength) VOID* Output,
    _In_ SIZE_T OutputLength)
{
    UCHAR* OutputBytes = Output;
    SIZE_T i;

    if (InputLength != OutputLength * 2)
        return FALSE;

    for (i = 0; i < OutputLength; i++)
    {
        CHAR HighNibble = hex_value(Input[i * 2]);
        CHAR LowNibble = hex_value(Input[i * 2 + 1]);

        if (HighNibble < 0 || LowNibble < 0)
            return FALSE;

        OutputBytes[i] = (UCHAR)((HighNibble << 4) | LowNibble);
    }

    return TRUE;
}

/*
 * Parse one hexadecimal number. Fails on empty input and on overflow.
 * Stops at the first non-hex character, reported through Next.
 */
BOOLEAN
parse_hex_value(
    _In_reads_(End - Buffer) const char* Buffer,
    _In_ const char* End,
    _Out_ PULONG64 Value,
    _Out_opt_ const char** Next)
{
    ULONG64 Result = 0;
    const char* Current = Buffer;

    if (Current == End || hex_value(*Current) < 0)
        return FALSE;

    while (Current != End)
    {
        char Digit = hex_value(*Current);

        if (Digit < 0)
            break;

        if (Result > ((~(ULONG64)0 - (ULONG64)Digit) >> 4))
            return FALSE;

        Result = (Result << 4) | (ULONG64)Digit;
        Current++;
    }

    *Value = Result;
    if (Next)
        *Next = Current;
    return TRUE;
}

/*
 * Parse a list of Count hexadecimal fields. Delimiters holds the character
 * required after each field, and may be shorter than Count: once it runs out
 * no separator is expected. Rest, when asked for, receives the position where
 * parsing stopped, so callers that require the fields to span the whole packet
 * can check it against End themselves.
 */
BOOLEAN
parse_hex_fields(
    _In_reads_(End - Buffer) const char* Buffer,
    _In_ const char* End,
    _In_z_ const char* Delimiters,
    _Out_writes_(Count) PULONG64 Values,
    _In_ ULONG Count,
    _Out_opt_ const char** Rest)
{
    const char* Current = Buffer;
    ULONG i;

    for (i = 0; i < Count; i++)
    {
        if (!parse_hex_value(Current, End, &Values[i], &Current))
            return FALSE;

        if (Delimiters[i] == '\0')
            break;

        if (Current == End || *Current != Delimiters[i])
            return FALSE;
        Current++;
    }

    if (Rest)
        *Rest = Current;
    return TRUE;
}

static
BOOLEAN
parse_gdb_id_component(
    _In_reads_(End - Buffer) const char* Buffer,
    _In_ const char* End,
    _Out_ PUINT_PTR Value,
    _Out_ const char** Next)
{
    ULONG64 ParsedValue;
    const char* Current;

    if ((End - Buffer) >= 2 && Buffer[0] == '-' && Buffer[1] == '1')
    {
        *Value = (UINT_PTR)-1;
        *Next = Buffer + 2;
        return TRUE;
    }

    if (!parse_hex_value(Buffer, End, &ParsedValue, &Current) ||
        ParsedValue > (ULONG64)(UINT_PTR)-1)
    {
        return FALSE;
    }

    *Value = (UINT_PTR)ParsedValue;
    *Next = Current;
    return TRUE;
}

BOOLEAN
parse_gdb_thread_id(
    _In_reads_(End - Buffer) const char* Buffer,
    _In_ const char* End,
    _Out_ PUINT_PTR Pid,
    _Out_ PUINT_PTR Tid)
{
    const char* Current;

#if MONOPROCESS
    *Pid = 0;
    return parse_gdb_id_component(Buffer, End, Tid, &Current) &&
           Current == End;
#else
    if (Buffer == End || *Buffer++ != 'p' ||
        !parse_gdb_id_component(Buffer, End, Pid, &Current))
    {
        return FALSE;
    }

    if (Current == End)
    {
        *Tid = (UINT_PTR)-1;
        return TRUE;
    }

    if (*Current++ != '.' ||
        !parse_gdb_id_component(Current, End, Tid, &Current) ||
        Current != End ||
        (*Pid == (UINT_PTR)-1 && *Tid != (UINT_PTR)-1))
    {
        return FALSE;
    }

    return TRUE;
#endif
}

/*
 * We cannot use PsLookupProcessThreadByCid or alike as we could be running at any IRQL.
 * So we have to loop over the process list.
 */

PEPROCESS
find_process(
    _In_ UINT_PTR Pid)
{
    PETHREAD CurrentThread;
    HANDLE ProcessId = gdb_pid_to_handle(Pid);
    LIST_ENTRY* ProcessEntry;
    PEPROCESS Process;

    if (Pid == 0)
    {
        CurrentThread = (PETHREAD)(ULONG_PTR)CurrentStateChange.Thread;
        return CONTAINING_RECORD(CurrentThread->Tcb.Process, EPROCESS, Pcb);
    }

    if (Pid == (UINT_PTR)-1)
        return NULL;

    /* Special case for idle process */
    if (ProcessId == NULL)
        return TheIdleProcess;

    if (!ps_initialized())
        return NULL;

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
    PETHREAD CurrentThread;
    UINT_PTR CurrentTid;
    HANDLE ThreadId = gdb_tid_to_handle(Tid);
    PETHREAD Thread;
    PEPROCESS Process;
    LIST_ENTRY* ThreadEntry;
#if MONOPROCESS
    LIST_ENTRY* ProcessEntry;
#endif

    CurrentThread = (PETHREAD)(ULONG_PTR)CurrentStateChange.Thread;
    CurrentTid = handle_to_gdb_tid(PsGetThreadId(CurrentThread));

    if (Tid == 0 || Tid == (UINT_PTR)-1 || Tid == CurrentTid)
    {
        /* Zero and -1 mean any, so use the current one */
        return CurrentThread;
    }

#if MONOPROCESS

    /* Special case for the idle thread */
    if (Tid == 1)
        return TheIdleThread;

    if (!ps_initialized())
        return NULL;

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
