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
void
handle_gdb_set_thread(void)
{
    switch (gdb_input[1])
    {
    case 'c':
        if (strcmp(&gdb_input[2], "-1") == 0)
            gdb_run_tid = (ULONG_PTR)-1;
        else
            gdb_run_tid = hex_to_tid(&gdb_input[2]);
        send_gdb_packet("OK");
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
        send_gdb_packet("OK");
        break;
    default:
        KDDBGPRINT("KDGBD: Unknown 'H' command: %s\n", gdb_input);
        send_gdb_packet("");
    }
}

static
void
handle_gdb_thread_alive(void)
{
    ULONG_PTR Pid, Tid;
    PETHREAD Thread;

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
        send_gdb_packet("OK");
    else
        send_gdb_packet("E03");
}

/* q* packets */
static
void
handle_gdb_query(void)
{
    if (strncmp(gdb_input, "qSupported:", 11) == 0)
    {
#if MONOPROCESS
        send_gdb_packet("PacketSize=1000;");
#else
        send_gdb_packet("PacketSize=1000;multiprocess+;");
#endif
        return;
    }

    if (strncmp(gdb_input, "qAttached", 9) == 0)
    {
#if MONOPROCESS
        send_gdb_packet("1");
#else
        UINT_PTR queried_pid = hex_to_pid(&gdb_input[10]);
        /* Let's say we created system process */
        if (gdb_pid_to_handle(queried_pid) == NULL)
            send_gdb_packet("0");
        else
            send_gdb_packet("1");
#endif
        return;
    }

    if (strncmp(gdb_input, "qRcmd,", 6) == 0)
    {
        send_gdb_packet("OK");
        return;
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
        send_gdb_packet(gdb_out);
        return;
    }

    if ((strncmp(gdb_input, "qfThreadInfo", 12) == 0)
            || (strncmp(gdb_input, "qsThreadInfo", 12) == 0))
    {
        BOOLEAN FirstThread = TRUE;
        PEPROCESS Process;
        PETHREAD Thread;
        char gdb_out[1024];
        char* ptr = gdb_out;
        BOOLEAN Resuming = strncmp(gdb_input, "qsThreadInfo", 12) == 0;
        /* Keep track of where we are. */
        static LIST_ENTRY* CurrentProcessEntry;
        static LIST_ENTRY* CurrentThreadEntry;

        ptr = gdb_out;

        *ptr++ = 'm';
        /* NULL terminate in case we got nothing more to iterate */
        *ptr  = '\0';

        if (!Resuming)
        {
        /* Report the idle thread */
#if MONOPROCESS
            ptr += sprintf(ptr, "1");
#else
            ptr += sprintf(gdb, "p1.1");
#endif
            /* Initialize the entries */
            CurrentProcessEntry = ProcessListHead->Flink;
            CurrentThreadEntry = NULL;
            FirstThread = FALSE;
        }

        if (CurrentProcessEntry == NULL) /* Ps is not initialized */
        {
            send_gdb_packet(Resuming ? "l" : gdb_out);
            return;
        }

        /* List all the processes */
        for ( ;
            CurrentProcessEntry != ProcessListHead;
            CurrentProcessEntry = CurrentProcessEntry->Flink)
        {

            Process = CONTAINING_RECORD(CurrentProcessEntry, EPROCESS, ActiveProcessLinks);

            if (CurrentThreadEntry != NULL)
                CurrentThreadEntry = CurrentThreadEntry->Flink;
            else
                CurrentThreadEntry = Process->ThreadListHead.Flink;

            /* List threads from this process */
            for ( ;
                 CurrentThreadEntry != &Process->ThreadListHead;
                 CurrentThreadEntry = CurrentThreadEntry->Flink)
            {
                Thread = CONTAINING_RECORD(CurrentThreadEntry, ETHREAD, ThreadListEntry);

                /* See if we should add a comma */
                if (FirstThread)
                {
                    FirstThread = FALSE;
                }
                else
                {
                    *ptr++ = ',';
                }

#if MONOPROCESS
                ptr += _snprintf(ptr, 1024 - (ptr - gdb_out),
                    "%p",
                    handle_to_gdb_tid(Thread->Cid.UniqueThread));
#else
                ptr += _snprintf(ptr, 1024 - (ptr - gdb_out),
                    "p%p.%p",
                    handle_to_gdb_pid(Process->UniqueProcessId),
                    handle_to_gdb_tid(Thread->Cid.UniqueThread));
#endif
                if (ptr > (gdb_out + 1024))
                {
                    /* send what we got */
                    send_gdb_packet(gdb_out);
                    /* GDB can ask anything at this point, it isn't necessarily a qsThreadInfo packet */
                    return;
                }
            }
            /* We're done for this process */
            CurrentThreadEntry = NULL;
        }

        if (gdb_out[1] == '\0')
        {
            /* We didn't iterate over anything, meaning we were already done */
            send_gdb_packet("l");
        }
        else
        {
            send_gdb_packet(gdb_out);
        }
        /* GDB can ask anything at this point, it isn't necessarily a qsThreadInfo packet */
        return;
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

        gdb_send_debug_io(&String, FALSE);
        return;
    }

    if (strncmp(gdb_input, "qOffsets", 8) == 0)
    {
        /* We load ntoskrnl at 0x80800000 while compiling it at 0x00800000 base adress */
        send_gdb_packet("TextSeg=80000000");
        return;
    }

    if (strcmp(gdb_input, "qTStatus") == 0)
    {
        /* No tracepoint support */
        send_gdb_packet("T0");
        return;
    }

    KDDBGPRINT("KDGDB: Unknown query: %s\n", gdb_input);
    send_gdb_packet("");
    return;
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
            send_gdb_packet("E03");
            return (KDSTATUS)-1;
        }

        AttachedProcess = AttachedThread->Tcb.Process;
        if (AttachedProcess == NULL)
        {
            KDDBGPRINT("The current GDB debug thread is invalid!");
            send_gdb_packet("E03");
            return (KDSTATUS)-1;
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
            send_gdb_packet("E03");
            return (KDSTATUS)-1;
        }
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
            send_gdb_packet("E03");
            return (KDSTATUS)-1;
        }

        AttachedProcess = AttachedThread->Tcb.Process;
        if (AttachedProcess == NULL)
        {
            KDDBGPRINT("The current GDB debug thread is invalid!");
            send_gdb_packet("E03");
            return (KDSTATUS)-1;
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
            send_gdb_packet("E03");
            return (KDSTATUS)-1;
        }
        __writecr3(AttachedProcess->Pcb.DirectoryTableBase[0]);
    }
#endif

    State->u.WriteMemory.TargetBaseAddress = hex_to_address(&gdb_input[1]);
    BufferLength = hex_to_address(strstr(&gdb_input[1], ",") + 1);
    if (BufferLength == 0)
    {
        /* Nothing to do */
        send_gdb_packet("OK");
        return (KDSTATUS)-1;
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
                send_gdb_packet("E01");
                return (KDSTATUS)-1;
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
    send_gdb_packet("E01");
    return (KDSTATUS)-1;
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
                send_gdb_packet("E01");
                return (KDSTATUS)-1;
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
    send_gdb_packet("E01");
    return (KDSTATUS)-1;
}

static
KDSTATUS
handle_gdb_c(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext)
{
    /* Tell GDB everything is fine, we will handle it */
    send_gdb_packet("OK");

    if (CurrentStateChange.NewState == DbgKdExceptionStateChange)
    {
        DBGKM_EXCEPTION64* Exception = &CurrentStateChange.u.Exception;
        ULONG_PTR ProgramCounter = KdpGetContextPc(&CurrentContext);

        /* See if we should update the program counter */
        if (Exception && (Exception->ExceptionRecord.ExceptionCode == STATUS_BREAKPOINT)
                && ProgramCounter == KdDebuggerDataBlock->BreakpointWithStatus.Pointer)
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
            send_gdb_packet("vCont;c;s");
            return (KDSTATUS)-1;
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
    return KdPacketReceived;
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
        Status = gdb_receive_packet(KdContext);
        KDDBGPRINT("KDGBD: Packet received with status %u\n", Status);

        if (Status != KdPacketReceived)
            return Status;

        Status = (KDSTATUS)-1;

        switch (gdb_input[0])
        {
        case '?':
            /* Send the Status */
            gdb_send_exception();
            break;
        case '!':
            send_gdb_packet("OK");
            break;
        case 'c':
            Status = handle_gdb_c(State, MessageData, MessageLength, KdContext);
            break;
        case 'g':
            gdb_send_registers();
            break;
        case 'H':
            handle_gdb_set_thread();
            break;
        case 'm':
            Status = handle_gdb_read_mem(State, MessageData, MessageLength, KdContext);
            break;
        case 'p':
            gdb_send_register();
            break;
        case 'q':
            handle_gdb_query();
            break;
        case 's':
            Status = handle_gdb_s(State, MessageData, MessageLength, KdContext);
            break;
        case 'T':
            handle_gdb_thread_alive();
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
            send_gdb_packet("");
        }
    } while (Status == (KDSTATUS)-1);

    return Status;
}

