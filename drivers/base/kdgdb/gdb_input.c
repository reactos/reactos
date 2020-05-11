/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kddll/gdb_input.c
 * PURPOSE:         Base functions for the kernel debugger.
 */

#include "kdgdb.h"

/* LOCALS *********************************************************************/
static ULONG_PTR gdb_run_tid;
static struct
{
    ULONG_PTR Address;
    ULONG Handle;
} BreakPointHandles[32];


/* GLOBALS ********************************************************************/
UINT_PTR gdb_dbg_pid;
UINT_PTR gdb_dbg_tid;

static inline
KDSTATUS
LOOP_IF_SUCCESS(int x)
{
    return (x == KdPacketReceived) ? (KDSTATUS)-1 : x;
}

/* PRIVATE FUNCTIONS **********************************************************/
static
UINT_PTR
hex_to_tid(char* buffer)
{
    ULONG_PTR ret = 0;
    char hex;
    while (*buffer)
    {
        hex = hex_value(*buffer++);
        if (hex < 0)
            return ret;
        ret <<= 4;
        ret += hex;
    }
    return ret;
}
#define hex_to_pid hex_to_tid

static
ULONG64
hex_to_address(char* buffer)
{
    ULONG64 ret = 0;
    char hex;
    while (*buffer)
    {
        hex = hex_value(*buffer++);
        if (hex < 0)
            return ret;
        ret <<= 4;
        ret += hex;
    }
    return ret;
}

/* H* packets */
static
KDSTATUS
handle_gdb_set_thread(void)
{
    KDSTATUS Status;

    switch (gdb_input[1])
    {
    case 'c':
        if (strcmp(&gdb_input[2], "-1") == 0)
            gdb_run_tid = (ULONG_PTR)-1;
        else
            gdb_run_tid = hex_to_tid(&gdb_input[2]);
        Status = send_gdb_packet("OK");
        break;
    case 'g':
        KDDBGPRINT("Setting debug thread: %s.\n", gdb_input);
#if MONOPROCESS
        gdb_dbg_pid = 0;
        if (strncmp(&gdb_input[2], "-1", 2) == 0)
        {
            gdb_dbg_tid = (UINT_PTR)-1;
        }
        else
        {
            gdb_dbg_tid = hex_to_tid(&gdb_input[2]);
        }
#else
        if (strncmp(&gdb_input[2], "p-1", 3) == 0)
        {
            gdb_dbg_pid = (UINT_PTR)-1;
            gdb_dbg_tid = (UINT_PTR)-1;
        }
        else
        {
            char* ptr = strstr(gdb_input, ".") + 1;
            gdb_dbg_pid = hex_to_pid(&gdb_input[3]);
            if (strncmp(ptr, "-1", 2) == 0)
                gdb_dbg_tid = (UINT_PTR)-1;
            else
                gdb_dbg_tid = hex_to_tid(ptr);
        }
#endif
        Status = send_gdb_packet("OK");
        break;
    default:
        KDDBGPRINT("KDGBD: Unknown 'H' command: %s\n", gdb_input);
        Status = send_gdb_packet("");
    }

    return Status;
}

static
KDSTATUS
handle_gdb_thread_alive(void)
{
    ULONG_PTR Pid, Tid;
    PETHREAD Thread;
    KDSTATUS Status;

#if MONOPROCESS
    Pid = 0;
    Tid = hex_to_tid(&gdb_input[1]);

    KDDBGPRINT("Checking if %p is alive.\n", Tid);

#else
    Pid = hex_to_pid(&gdb_input[2]);
    Tid = hex_to_tid(strstr(gdb_input, ".") + 1);

    /* We cannot use PsLookupProcessThreadByCid as we could be running at any IRQL.
     * So loop. */
    KDDBGPRINT("Checking if p%p.%p is alive.\n", Pid, Tid);
#endif

    Thread = find_thread(Pid, Tid);

    if (Thread != NULL)
        Status = send_gdb_packet("OK");
    else
        Status = send_gdb_packet("E03");

    return Status;
}

/* q* packets */
static
KDSTATUS
handle_gdb_query(void)
{
    if (strncmp(gdb_input, "qSupported:", 11) == 0)
    {
#if MONOPROCESS
        return send_gdb_packet("PacketSize=1000;qXfer:libraries:read+;");
#else
        return send_gdb_packet("PacketSize=1000;multiprocess+;qXfer:libraries:read+;");
#endif
    }

    if (strncmp(gdb_input, "qAttached", 9) == 0)
    {
#if MONOPROCESS
        return send_gdb_packet("1");
#else
        UINT_PTR queried_pid = hex_to_pid(&gdb_input[10]);
        /* Let's say we created system process */
        if (gdb_pid_to_handle(queried_pid) == NULL)
            return send_gdb_packet("0");
        else
            return send_gdb_packet("1");
#endif
    }

    if (strncmp(gdb_input, "qRcmd,", 6) == 0)
    {
        return send_gdb_packet("OK");
    }

    if (strcmp(gdb_input, "qC") == 0)
    {
        char gdb_out[64];
#if MONOPROCESS
        sprintf(gdb_out, "QC:%"PRIxPTR";",
            handle_to_gdb_tid(PsGetThreadId((PETHREAD)(ULONG_PTR)CurrentStateChange.Thread)));
#else
        sprintf(gdb_out, "QC:p%"PRIxPTR".%"PRIxPTR";",
            handle_to_gdb_pid(PsGetThreadProcessId((PETHREAD)(ULONG_PTR)CurrentStateChange.Thread)),
            handle_to_gdb_tid(PsGetThreadId((PETHREAD)(ULONG_PTR)CurrentStateChange.Thread)));
#endif
        return send_gdb_packet(gdb_out);
    }

    if (strncmp(gdb_input, "qfThreadInfo", 12) == 0)
    {
        PEPROCESS Process;
        char gdb_out[40];
        LIST_ENTRY* CurrentProcessEntry;

        CurrentProcessEntry = ProcessListHead->Flink;
        if (CurrentProcessEntry == NULL) /* Ps is not initialized */
        {
#if MONOPROCESS
            return send_gdb_packet("m1");
#else
            return send_gdb_packet("mp1.1");
#endif
        }

        /* We will push threads as we find them */
        start_gdb_packet();

        /* Start with the system thread */
#if MONOPROCESS
        send_gdb_partial_packet("m1");
#else
        send_gdb_partial_packet("mp1.1");
#endif

        /* List all the processes */
        for ( ;
            CurrentProcessEntry != ProcessListHead;
            CurrentProcessEntry = CurrentProcessEntry->Flink)
        {
            LIST_ENTRY* CurrentThreadEntry;

            Process = CONTAINING_RECORD(CurrentProcessEntry, EPROCESS, ActiveProcessLinks);

            /* List threads from this process */
            for ( CurrentThreadEntry = Process->ThreadListHead.Flink;
                 CurrentThreadEntry != &Process->ThreadListHead;
                 CurrentThreadEntry = CurrentThreadEntry->Flink)
            {
                PETHREAD Thread = CONTAINING_RECORD(CurrentThreadEntry, ETHREAD, ThreadListEntry);

#if MONOPROCESS
                _snprintf(gdb_out, 40, ",%p", handle_to_gdb_tid(Thread->Cid.UniqueThread));
#else
                _snprintf(gdb_out, 40, ",p%p.%p",
                    handle_to_gdb_pid(Process->UniqueProcessId),
                    handle_to_gdb_tid(Thread->Cid.UniqueThread));
#endif
                send_gdb_partial_packet(gdb_out);
            }
        }

        return finish_gdb_packet();
    }

    if (strncmp(gdb_input, "qsThreadInfo", 12) == 0)
    {
        /* We sent the whole thread list on first qfThreadInfo call */
        return send_gdb_packet("l");
    }

    if (strncmp(gdb_input, "qThreadExtraInfo,", 17) == 0)
    {
        ULONG_PTR Pid, Tid;
        PETHREAD Thread;
        PEPROCESS Process;
        char out_string[64];
        STRING String = {0, 64, out_string};

        KDDBGPRINT("Giving extra info for");

#if MONOPROCESS
        Pid = 0;
        Tid = hex_to_tid(&gdb_input[17]);

        KDDBGPRINT(" %p.\n", Tid);

        Thread = find_thread(Pid, Tid);
        Process = CONTAINING_RECORD(Thread->Tcb.Process, EPROCESS, Pcb);
#else
        Pid = hex_to_pid(&gdb_input[18]);
        Tid = hex_to_tid(strstr(&gdb_input[18], ".") + 1);

        /* We cannot use PsLookupProcessThreadByCid as we could be running at any IRQL.
         * So loop. */
        KDDBGPRINT(" p%p.%p.\n", Pid, Tid);

        Process = find_process(Pid);
        Thread = find_thread(Pid, Tid);
#endif

        if (PsGetThreadProcessId(Thread) == 0)
        {
            String.Length = sprintf(out_string, "SYSTEM");
        }
        else
        {
            String.Length = sprintf(out_string, "%.*s", 16, Process->ImageFileName);
        }

        return gdb_send_debug_io(&String, FALSE);
    }

    if (strncmp(gdb_input, "qOffsets", 8) == 0)
    {
        /* We load ntoskrnl at 0x80800000 while compiling it at 0x00800000 base address */
        return send_gdb_packet("TextSeg=80000000");
    }

    if (strcmp(gdb_input, "qTStatus") == 0)
    {
        /* No tracepoint support */
        return send_gdb_packet("T0");
    }

    if (strcmp(gdb_input, "qSymbol::") == 0)
    {
        /* No need */
        return send_gdb_packet("OK");
    }

    if (strncmp(gdb_input, "qXfer:libraries:read::", 22) == 0)
    {
        static LIST_ENTRY* CurrentEntry = NULL;
        char str_helper[256];
        char name_helper[64];        
        ULONG_PTR Offset = hex_to_address(&gdb_input[22]);
        ULONG_PTR ToSend = hex_to_address(strstr(&gdb_input[22], ",") + 1);
        ULONG Sent = 0;
        static BOOLEAN allDone = FALSE;

        KDDBGPRINT("KDGDB: qXfer:libraries:read !\n");

        /* Start the packet */
        start_gdb_packet();

        if (allDone)
        {
            send_gdb_partial_packet("l");
            allDone = FALSE;
            return finish_gdb_packet();
        }

        send_gdb_partial_packet("m");
        Sent++;

        /* Are we starting ? */
        if (Offset == 0)
        {
            Sent += send_gdb_partial_binary("<?xml version=\"1.0\"?>", 21);
            Sent += send_gdb_partial_binary("<library-list>", 14);

            CurrentEntry = ModuleListHead->Flink;

            if (!CurrentEntry)
            {
                /* Ps is not initialized. Send end of XML data or mark that we are finished. */
                Sent += send_gdb_partial_binary("</library-list>", 15);
                allDone = TRUE;
                return finish_gdb_packet();
            }
        }

        for ( ;
            CurrentEntry != ModuleListHead;
            CurrentEntry = CurrentEntry->Flink)
        {
            PLDR_DATA_TABLE_ENTRY TableEntry = CONTAINING_RECORD(CurrentEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
            PVOID DllBase = (PVOID)((ULONG_PTR)TableEntry->DllBase + 0x1000);
            LONG mem_length;
            USHORT i;

            /* Convert names to lower case. Yes this _is_ ugly */
            for (i = 0; i < (TableEntry->BaseDllName.Length / sizeof(WCHAR)); i++)
            {
                name_helper[i] = (char)TableEntry->BaseDllName.Buffer[i];
                if (name_helper[i] >= 'A' && name_helper[i] <= 'Z')
                    name_helper[i] += 'a' - 'A';
            }
            name_helper[i] = 0;

            /* GDB doesn't load the file if you don't prefix it with a drive letter... */
            mem_length = _snprintf(str_helper, 256, "<library name=\"C:\\%s\"><segment address=\"0x%p\"/></library>", &name_helper, DllBase);
            
            /* DLL name must be too long. */
            if (mem_length < 0)
            {
                KDDBGPRINT("Failed to report %wZ\n", &TableEntry->BaseDllName);
                continue;
            }

            if ((Sent + mem_length) > ToSend)
            {
                /* We're done for this pass */
                return finish_gdb_packet();
            }

            Sent += send_gdb_partial_binary(str_helper, mem_length);
        }

        if ((ToSend - Sent) > 15)
        {
            Sent += send_gdb_partial_binary("</library-list>", 15);
            allDone = TRUE;
        }

        return finish_gdb_packet();
    }

    KDDBGPRINT("KDGDB: Unknown query: %s\n", gdb_input);
    return send_gdb_packet("");
}

#if 0
static
KDSTATUS
handle_gdb_registers(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength)
{
    /*
    if (gdb_dbg_thread)
        KDDBGPRINT("Should get registers from other thread!\n");
    */

    State->ApiNumber = DbgKdGetContextApi;
    State->ReturnStatus = STATUS_SUCCESS; /* ? */
    State->Processor = CurrentStateChange.Processor;
    State->ProcessorLevel = CurrentStateChange.ProcessorLevel;
    if (MessageData)
        MessageData->Length = 0;
    *MessageLength = 0;
    return KdPacketReceived;
}
#endif

static
void
ReadMemorySendHandler(
    _In_ ULONG PacketType,
    _In_ PSTRING MessageHeader,
    _In_ PSTRING MessageData)
{
    DBGKD_MANIPULATE_STATE64* State = (DBGKD_MANIPULATE_STATE64*)MessageHeader->Buffer;

    if (PacketType != PACKET_TYPE_KD_STATE_MANIPULATE)
    {
        // KdAssert
        KDDBGPRINT("Wrong packet type (%lu) received after DbgKdReadVirtualMemoryApi request.\n", PacketType);
        while (1);
    }

    if (State->ApiNumber != DbgKdReadVirtualMemoryApi)
    {
        KDDBGPRINT("Wrong API number (%lu) after DbgKdReadVirtualMemoryApi request.\n", State->ApiNumber);
    }

    /* Check status. Allow to send partial data. */
    if (!MessageData->Length && !NT_SUCCESS(State->ReturnStatus))
        send_gdb_ntstatus(State->ReturnStatus);
    else
        send_gdb_memory(MessageData->Buffer, MessageData->Length);
    KdpSendPacketHandler = NULL;
    KdpManipulateStateHandler = NULL;

#if MONOPROCESS
    if (gdb_dbg_tid != 0)
    /* Reset the TLB */
#else
    if ((gdb_dbg_pid != 0) && gdb_pid_to_handle(gdb_dbg_pid) != PsGetCurrentProcessId())
#endif
    {
        /* Only do this if Ps is initialized */
        if (ProcessListHead->Flink)
            __writecr3(PsGetCurrentProcess()->Pcb.DirectoryTableBase[0]);
    }
}

static
KDSTATUS
handle_gdb_read_mem(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext)
{
    State->ApiNumber = DbgKdReadVirtualMemoryApi;
    State->ReturnStatus = STATUS_SUCCESS; /* ? */
    State->Processor = CurrentStateChange.Processor;
    State->ProcessorLevel = CurrentStateChange.ProcessorLevel;
    if (MessageData)
        MessageData->Length = 0;
    *MessageLength = 0;

    /* Set the TLB according to the process being read. Pid 0 means any process. */
#if MONOPROCESS
    if ((gdb_dbg_tid != 0) && gdb_tid_to_handle(gdb_dbg_tid) != PsGetCurrentThreadId())
    {
        PETHREAD AttachedThread = find_thread(0, gdb_dbg_tid);
        PKPROCESS AttachedProcess;
        if (AttachedThread == NULL)
        {
            KDDBGPRINT("The current GDB debug thread is invalid!");
            return LOOP_IF_SUCCESS(send_gdb_packet("E03"));
        }

        AttachedProcess = AttachedThread->Tcb.Process;
        if (AttachedProcess == NULL)
        {
            KDDBGPRINT("The current GDB debug thread is invalid!");
            return LOOP_IF_SUCCESS(send_gdb_packet("E03"));
        }
        __writecr3(AttachedProcess->DirectoryTableBase[0]);
    }
#else
    if ((gdb_dbg_pid != 0) && gdb_pid_to_handle(gdb_dbg_pid) != PsGetCurrentProcessId())
    {
        PEPROCESS AttachedProcess = find_process(gdb_dbg_pid);
        if (AttachedProcess == NULL)
        {
            KDDBGPRINT("The current GDB debug thread is invalid!");
            return LOOP_IF_SUCCESS(send_gdb_packet("E03"));
        }
        /* Only do this if Ps is initialized */
        if (ProcessListHead->Flink)
            __writecr3(AttachedProcess->Pcb.DirectoryTableBase[0]);
    }
#endif

    State->u.ReadMemory.TargetBaseAddress = hex_to_address(&gdb_input[1]);
    State->u.ReadMemory.TransferCount = hex_to_address(strstr(&gdb_input[1], ",") + 1);

    /* KD will reply with KdSendPacket. Catch it */
    KdpSendPacketHandler = ReadMemorySendHandler;
    return KdPacketReceived;
}

static
void
WriteMemorySendHandler(
    _In_ ULONG PacketType,
    _In_ PSTRING MessageHeader,
    _In_ PSTRING MessageData)
{
    DBGKD_MANIPULATE_STATE64* State = (DBGKD_MANIPULATE_STATE64*)MessageHeader->Buffer;

    if (PacketType != PACKET_TYPE_KD_STATE_MANIPULATE)
    {
        // KdAssert
        KDDBGPRINT("Wrong packet type (%lu) received after DbgKdWriteVirtualMemoryApi request.\n", PacketType);
        while (1);
    }

    if (State->ApiNumber != DbgKdWriteVirtualMemoryApi)
    {
        KDDBGPRINT("Wrong API number (%lu) after DbgKdWriteVirtualMemoryApi request.\n", State->ApiNumber);
    }

    /* Check status */
    if (!NT_SUCCESS(State->ReturnStatus))
        send_gdb_ntstatus(State->ReturnStatus);
    else
        send_gdb_packet("OK");
    KdpSendPacketHandler = NULL;
    KdpManipulateStateHandler = NULL;

#if MONOPROCESS
    if (gdb_dbg_tid != 0)
    /* Reset the TLB */
#else
    if ((gdb_dbg_pid != 0) && gdb_pid_to_handle(gdb_dbg_pid) != PsGetCurrentProcessId())
#endif
    {
        /* Only do this if Ps is initialized */
        if (ProcessListHead->Flink)
            __writecr3(PsGetCurrentProcess()->Pcb.DirectoryTableBase[0]);
    }
}

static
KDSTATUS
handle_gdb_write_mem(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext)
{
    /* Maximal input buffer is 0x1000. Each byte is encoded on two bytes by GDB */
    static UCHAR OutBuffer[0x800];
    ULONG BufferLength;
    char* blob_ptr;
    UCHAR* OutPtr;

    State->ApiNumber = DbgKdWriteVirtualMemoryApi;
    State->ReturnStatus = STATUS_SUCCESS; /* ? */
    State->Processor = CurrentStateChange.Processor;
    State->ProcessorLevel = CurrentStateChange.ProcessorLevel;

    /* Set the TLB according to the process being read. Pid 0 means any process. */
#if MONOPROCESS
    if ((gdb_dbg_tid != 0) && gdb_tid_to_handle(gdb_dbg_tid) != PsGetCurrentThreadId())
    {
        PETHREAD AttachedThread = find_thread(0, gdb_dbg_tid);
        PKPROCESS AttachedProcess;
        if (AttachedThread == NULL)
        {
            KDDBGPRINT("The current GDB debug thread is invalid!");
            return LOOP_IF_SUCCESS(send_gdb_packet("E03"));
        }

        AttachedProcess = AttachedThread->Tcb.Process;
        if (AttachedProcess == NULL)
        {
            KDDBGPRINT("The current GDB debug thread is invalid!");
            return LOOP_IF_SUCCESS(send_gdb_packet("E03"));
        }
        __writecr3(AttachedProcess->DirectoryTableBase[0]);
    }
#else
    if ((gdb_dbg_pid != 0) && gdb_pid_to_handle(gdb_dbg_pid) != PsGetCurrentProcessId())
    {
        PEPROCESS AttachedProcess = find_process(gdb_dbg_pid);
        if (AttachedProcess == NULL)
        {
            KDDBGPRINT("The current GDB debug thread is invalid!");
            return LOOP_IF_SUCCESS(send_gdb_packet("E03"));
        }
        /* Only do this if Ps is initialized */
        if (ProcessListHead->Flink)
            __writecr3(AttachedProcess->Pcb.DirectoryTableBase[0]);
    }
#endif

    State->u.WriteMemory.TargetBaseAddress = hex_to_address(&gdb_input[1]);
    BufferLength = hex_to_address(strstr(&gdb_input[1], ",") + 1);
    if (BufferLength == 0)
    {
        /* Nothing to do */
        return LOOP_IF_SUCCESS(send_gdb_packet("OK"));
    }
    
    State->u.WriteMemory.TransferCount = BufferLength;
    MessageData->Length = BufferLength;
    MessageData->Buffer = (CHAR*)OutBuffer;

    OutPtr = OutBuffer;
    blob_ptr = strstr(strstr(&gdb_input[1], ",") + 1, ":") + 1;
    while (BufferLength)
    {
        if (BufferLength >= 4)
        {
            *((ULONG*)OutPtr) = *((ULONG*)blob_ptr);
            OutPtr += 4;
            blob_ptr += 4;
            BufferLength -= 4;
        }
        else if (BufferLength >= 2)
        {
            *((USHORT*)OutPtr) = *((USHORT*)blob_ptr);
            OutPtr += 2;
            blob_ptr += 2;
            BufferLength -= 2;
        }
        else
        {
            *OutPtr++ = *blob_ptr++;
            BufferLength--;
        }
    }

    /* KD will reply with KdSendPacket. Catch it */
    KdpSendPacketHandler = WriteMemorySendHandler;
    return KdPacketReceived;
}

static
void
WriteBreakPointSendHandler(
    _In_ ULONG PacketType,
    _In_ PSTRING MessageHeader,
    _In_ PSTRING MessageData)
{
    DBGKD_MANIPULATE_STATE64* State = (DBGKD_MANIPULATE_STATE64*)MessageHeader->Buffer;

    if (PacketType != PACKET_TYPE_KD_STATE_MANIPULATE)
    {
        // KdAssert
        KDDBGPRINT("Wrong packet type (%lu) received after DbgKdWriteBreakPointApi request.\n", PacketType);
        while (1);
    }

    if (State->ApiNumber != DbgKdWriteBreakPointApi)
    {
        KDDBGPRINT("Wrong API number (%lu) after DbgKdWriteBreakPointApi request.\n", State->ApiNumber);
    }

    /* Check status */
    if (!NT_SUCCESS(State->ReturnStatus))
    {
        KDDBGPRINT("Inserting breakpoint failed!\n");
        send_gdb_ntstatus(State->ReturnStatus);
    }
    else
    {
        /* Keep track of the address+handle couple */
        ULONG i;
        for (i = 0; i < (sizeof(BreakPointHandles) / sizeof(BreakPointHandles[0])); i++)
        {
            if (BreakPointHandles[i].Address == 0)
            {
                BreakPointHandles[i].Address = (ULONG_PTR)State->u.WriteBreakPoint.BreakPointAddress;
                BreakPointHandles[i].Handle = State->u.WriteBreakPoint.BreakPointHandle;
                break;
            }
        }
        send_gdb_packet("OK");
    }
    KdpSendPacketHandler = NULL;
    KdpManipulateStateHandler = NULL;
}

static
KDSTATUS
handle_gdb_insert_breakpoint(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext)
{
    State->ReturnStatus = STATUS_SUCCESS; /* ? */
    State->Processor = CurrentStateChange.Processor;
    State->ProcessorLevel = CurrentStateChange.ProcessorLevel;
    if (MessageData)
        MessageData->Length = 0;
    *MessageLength = 0;

    switch (gdb_input[1])
    {
        case '0':
        {
            ULONG_PTR Address = hex_to_address(&gdb_input[3]);
            ULONG i;
            BOOLEAN HasFreeSlot = FALSE;

            KDDBGPRINT("Inserting breakpoint at %p.\n", (void*)Address);

            for (i = 0; i < (sizeof(BreakPointHandles) / sizeof(BreakPointHandles[0])); i++)
            {
                if (BreakPointHandles[i].Address == 0)
                    HasFreeSlot = TRUE;
            }

            if (!HasFreeSlot)
            {
                /* We don't have a way to keep track of this break point. Fail. */
                KDDBGPRINT("No breakpoint slot available!\n");
                return LOOP_IF_SUCCESS(send_gdb_packet("E01"));
            }

            State->ApiNumber = DbgKdWriteBreakPointApi;
            State->u.WriteBreakPoint.BreakPointAddress = Address;
            /* FIXME : ignoring all other Z0 arguments */

            /* KD will reply with KdSendPacket. Catch it */
            KdpSendPacketHandler = WriteBreakPointSendHandler;
            return KdPacketReceived;
        }
    }

    KDDBGPRINT("Unhandled 'Z' packet: %s\n", gdb_input);
    return LOOP_IF_SUCCESS(send_gdb_packet("E01"));
}

static
void
RestoreBreakPointSendHandler(
    _In_ ULONG PacketType,
    _In_ PSTRING MessageHeader,
    _In_ PSTRING MessageData)
{
    DBGKD_MANIPULATE_STATE64* State = (DBGKD_MANIPULATE_STATE64*)MessageHeader->Buffer;
    ULONG i;

    if (PacketType != PACKET_TYPE_KD_STATE_MANIPULATE)
    {
        // KdAssert
        KDDBGPRINT("Wrong packet type (%lu) received after DbgKdRestoreBreakPointApi request.\n", PacketType);
        while (1);
    }

    if (State->ApiNumber != DbgKdRestoreBreakPointApi)
    {
        KDDBGPRINT("Wrong API number (%lu) after DbgKdRestoreBreakPointApi request.\n", State->ApiNumber);
    }

    /* We ignore failure here. If DbgKdRestoreBreakPointApi fails, 
     * this means that the breakpoint was already invalid for KD. So clean it up on our side. */
    for (i = 0; i < (sizeof(BreakPointHandles) / sizeof(BreakPointHandles[0])); i++)
    {
        if (BreakPointHandles[i].Handle == State->u.RestoreBreakPoint.BreakPointHandle)
        {
            BreakPointHandles[i].Address = 0;
            BreakPointHandles[i].Handle = 0;
            break;
        }
    }

    send_gdb_packet("OK");

    KdpSendPacketHandler = NULL;
    KdpManipulateStateHandler = NULL;
}

static
KDSTATUS
handle_gdb_remove_breakpoint(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext)
{
    State->ReturnStatus = STATUS_SUCCESS; /* ? */
    State->Processor = CurrentStateChange.Processor;
    State->ProcessorLevel = CurrentStateChange.ProcessorLevel;
    if (MessageData)
        MessageData->Length = 0;
    *MessageLength = 0;

    switch (gdb_input[1])
    {
        case '0':
        {
            ULONG_PTR Address = hex_to_address(&gdb_input[3]);
            ULONG i, Handle = 0;

            KDDBGPRINT("Removing breakpoint on %p.\n", (void*)Address);

            for (i = 0; i < (sizeof(BreakPointHandles) / sizeof(BreakPointHandles[0])); i++)
            {
                if (BreakPointHandles[i].Address == Address)
                {
                    Handle = BreakPointHandles[i].Handle;
                    break;
                }
            }

            if (Handle == 0)
            {
                KDDBGPRINT("Received %s, but breakpoint was never inserted ?!\n", gdb_input);
                return LOOP_IF_SUCCESS(send_gdb_packet("E01"));
            }

            State->ApiNumber = DbgKdRestoreBreakPointApi;
            State->u.RestoreBreakPoint.BreakPointHandle = Handle;
            /* FIXME : ignoring all other z0 arguments */

            /* KD will reply with KdSendPacket. Catch it */
            KdpSendPacketHandler = RestoreBreakPointSendHandler;
            return KdPacketReceived;
        }
    }

    KDDBGPRINT("Unhandled 'Z' packet: %s\n", gdb_input);
    return LOOP_IF_SUCCESS(send_gdb_packet("E01"));
}

static
KDSTATUS
handle_gdb_c(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext)
{
    KDSTATUS Status;

    /* Tell GDB everything is fine, we will handle it */
    Status = send_gdb_packet("OK");
    if (Status != KdPacketReceived)
        return Status;
        

    if (CurrentStateChange.NewState == DbgKdExceptionStateChange)
    {
        DBGKM_EXCEPTION64* Exception = &CurrentStateChange.u.Exception;
        ULONG_PTR ProgramCounter = KdpGetContextPc(&CurrentContext);

        /* See if we should update the program counter */
        if (Exception && (Exception->ExceptionRecord.ExceptionCode == STATUS_BREAKPOINT)
                && ((*(KD_BREAKPOINT_TYPE*)ProgramCounter) == KD_BREAKPOINT_VALUE))
        {
            /* We must get past the breakpoint instruction */
            KdpSetContextPc(&CurrentContext, ProgramCounter + KD_BREAKPOINT_SIZE);

            SetContextManipulateHandler(State, MessageData, MessageLength, KdContext);
            KdpManipulateStateHandler = ContinueManipulateStateHandler;
            return KdPacketReceived;
        }
    }

    return ContinueManipulateStateHandler(State, MessageData, MessageLength, KdContext);
}

static
KDSTATUS
handle_gdb_C(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext)
{
    KDSTATUS Status;

    /* Tell GDB everything is fine, we will handle it */
    Status = send_gdb_packet("OK");
    if (Status != KdPacketReceived)
        return Status;

    if (CurrentStateChange.NewState == DbgKdExceptionStateChange)
    {
        /* Debugger didn't handle the exception, report it back to the kernel */
        State->u.Continue2.ContinueStatus = CurrentStateChange.u.Exception.ExceptionRecord.ExceptionCode;
        State->ApiNumber = DbgKdContinueApi2;
        return KdPacketReceived;
    }
    /* We should never reach this ? */
    return ContinueManipulateStateHandler(State, MessageData, MessageLength, KdContext);
}

static
KDSTATUS
handle_gdb_s(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext)
{
    KDDBGPRINT("Single stepping.\n");
    /* Set CPU single step mode and continue */
    KdpSetSingleStep(&CurrentContext);
    SetContextManipulateHandler(State, MessageData, MessageLength, KdContext);
    KdpManipulateStateHandler = ContinueManipulateStateHandler;
    return KdPacketReceived;
}

static
KDSTATUS
handle_gdb_v(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext)
{
    if (strncmp(gdb_input, "vCont", 5) == 0)
    {
        if (gdb_input[5] == '?')
        {
            /* Report what we support */
            return LOOP_IF_SUCCESS(send_gdb_packet("vCont;c;s"));
        }

        if (strncmp(gdb_input, "vCont;c", 7) == 0)
        {
            return handle_gdb_c(State, MessageData, MessageLength, KdContext);
        }

        if (strncmp(gdb_input, "vCont;s", 7) == 0)
        {
            
            return handle_gdb_s(State, MessageData, MessageLength, KdContext);
        }
    }

    KDDBGPRINT("Unhandled 'v' packet: %s\n", gdb_input);
    return LOOP_IF_SUCCESS(send_gdb_packet(""));
}

KDSTATUS
gdb_receive_and_interpret_packet(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext)
{
    KDSTATUS Status;

    do
    {
        KDDBGPRINT("KDGBD: Receiving packet.\n");
        Status = gdb_receive_packet(KdContext);
        KDDBGPRINT("KDGBD: Packet \"%s\" received with status %u\n", gdb_input, Status);

        if (Status != KdPacketReceived)
            return Status;

        Status = (KDSTATUS)-1;

        switch (gdb_input[0])
        {
        case '?':
            /* Send the Status */
            Status = LOOP_IF_SUCCESS(gdb_send_exception());
            break;
        case '!':
            Status = LOOP_IF_SUCCESS(send_gdb_packet("OK"));
            break;
        case 'c':
            Status = handle_gdb_c(State, MessageData, MessageLength, KdContext);
            break;
        case 'C':
            Status = handle_gdb_C(State, MessageData, MessageLength, KdContext);
            break;
        case 'g':
            Status = LOOP_IF_SUCCESS(gdb_send_registers());
            break;
        case 'H':
            Status = LOOP_IF_SUCCESS(handle_gdb_set_thread());
            break;
        case 'm':
            Status = handle_gdb_read_mem(State, MessageData, MessageLength, KdContext);
            break;
        case 'p':
            Status = LOOP_IF_SUCCESS(gdb_send_register());
            break;
        case 'q':
            Status = LOOP_IF_SUCCESS(handle_gdb_query());
            break;
        case 's':
            Status = handle_gdb_s(State, MessageData, MessageLength, KdContext);
            break;
        case 'T':
            Status = LOOP_IF_SUCCESS(handle_gdb_thread_alive());
            break;
        case 'v':
            Status = handle_gdb_v(State, MessageData, MessageLength, KdContext);
            break;
        case 'X':
            Status = handle_gdb_write_mem(State, MessageData, MessageLength, KdContext);
            break;
        case 'z':
            Status = handle_gdb_remove_breakpoint(State, MessageData, MessageLength, KdContext);
            break;
        case 'Z':
            Status = handle_gdb_insert_breakpoint(State, MessageData, MessageLength, KdContext);
            break;
        default:
            /* We don't know how to handle this request. */
            KDDBGPRINT("Unsupported GDB command: %s.\n", gdb_input);
            Status = LOOP_IF_SUCCESS(send_gdb_packet(""));
        }
    } while (Status == (KDSTATUS)-1);

    return Status;
}

