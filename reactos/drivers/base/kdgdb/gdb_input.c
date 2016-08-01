/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kddll/gdb_input.c
 * PURPOSE:         Base functions for the kernel debugger.
 */

#include "kdgdb.h"

/* LOCALS *********************************************************************/
static ULONG_PTR gdb_run_tid;


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
            /* Initialize the entries */
            CurrentProcessEntry = ProcessListHead->Flink;
            CurrentThreadEntry = NULL;

            /* Start with idle thread */
#if MONOPROCESS
            ptr = gdb_out + sprintf(gdb_out, "m1");
#else
            ptr = gdb_out + sprintf(gdb_out, "mp1.1");
#endif
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
        Pid = hex_to_pid(&gdb_input[2]);
        Tid = hex_to_tid(strstr(gdb_input, ".") + 1);

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

    /* Check status */
    if (!NT_SUCCESS(State->ReturnStatus))
        send_gdb_ntstatus(State->ReturnStatus);
    else
        send_gdb_memory(MessageData->Buffer, MessageData->Length);
    KdpSendPacketHandler = NULL;
    KdpManipulateStateHandler = NULL;

#if !MONOPROCESS
    /* Reset the TLB */
    if ((gdb_dbg_pid != 0) && gdb_pid_to_handle(gdb_dbg_pid) != PsGetCurrentProcessId())
    {
        __writecr3(PsGetCurrentProcess()->Pcb.DirectoryTableBase[0]);
    }
#endif
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

#if !MONOPROCESS
    /* Set the TLB according to the process being read. Pid 0 means any process. */
    if ((gdb_dbg_pid != 0) && gdb_pid_to_handle(gdb_dbg_pid) != PsGetCurrentProcessId())
    {
        PEPROCESS AttachedProcess = find_process(gdb_dbg_pid);
        if (AttachedProcess == NULL)
        {
            KDDBGPRINT("The current GDB debug thread is invalid!");
            send_gdb_packet("E03");
            return gdb_receive_and_interpret_packet(State, MessageData, MessageLength, KdContext);
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
            send_gdb_packet("vCont;c;C;s;S");
            return (KDSTATUS)-1;
        }

        if (strncmp(gdb_input, "vCont;c", 7) == 0)
        {
            DBGKM_EXCEPTION64* Exception = NULL;

            /* Tell GDB everything is fine, we will handle it */
            send_gdb_packet("OK");

            if (CurrentStateChange.NewState == DbgKdExceptionStateChange)
                Exception = &CurrentStateChange.u.Exception;

            /* See if we should update the program counter (unlike windbg, gdb doesn't do it for us) */
            if (Exception && (Exception->ExceptionRecord.ExceptionCode == STATUS_BREAKPOINT)
                    && (Exception->ExceptionRecord.ExceptionInformation[0] == 0))
            {
                ULONG_PTR ProgramCounter;

                /* So we must get past the breakpoint instruction */
                ProgramCounter = KdpGetContextPc(&CurrentContext);
                KdpSetContextPc(&CurrentContext, ProgramCounter + KD_BREAKPOINT_SIZE);

                SetContextManipulateHandler(State, MessageData, MessageLength, KdContext);
                KdpManipulateStateHandler = ContinueManipulateStateHandler;
                return KdPacketReceived;
            }

            return ContinueManipulateStateHandler(State, MessageData, MessageLength, KdContext);
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
            gdb_send_exception(TRUE);
            break;
        case '!':
            send_gdb_packet("OK");
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
        case 'T':
            handle_gdb_thread_alive();
            break;
        case 'v':
            Status = handle_gdb_v(State, MessageData, MessageLength, KdContext);
            break;
        default:
            /* We don't know how to handle this request. Maybe this is something for KD */
            State->ReturnStatus = STATUS_NOT_SUPPORTED;
            KDDBGPRINT("Unsupported GDB command: %s.\n", gdb_input);
            return KdPacketReceived;
        }
    } while (Status == (KDSTATUS)-1);

    return Status;
}

