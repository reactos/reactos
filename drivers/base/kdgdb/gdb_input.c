/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kddll/gdb_input.c
 * PURPOSE:         Base functions for the kernel debugger.
 */

#include "kdgdb.h"

/* LOCALS *********************************************************************/
static UINT_PTR gdb_run_tid;
static struct
{
    ULONG_PTR Address;
    ULONG Handle;
} BreakPointHandles[32];
static GDB_HARDWARE_BREAKPOINT HardwareBreakpoints[4];
static LIST_ENTRY* ThreadInfoProcessEntry;
static LIST_ENTRY* ThreadInfoThreadEntry;
static UINT_PTR ThreadInfoInitialTid;
static BOOLEAN ThreadInfoCurrentPending;
static BOOLEAN ThreadInfoIdlePending;


/* GLOBALS ********************************************************************/
UINT_PTR gdb_dbg_pid;
UINT_PTR gdb_dbg_tid;

const GDB_HARDWARE_BREAKPOINT*
gdb_hardware_breakpoints(VOID)
{
    return HardwareBreakpoints;
}

static BOOLEAN
apply_hardware_breakpoints(VOID)
{
    PKPRCB* ProcessorBlock;
    ULONG_PTR ProcessorBlockAddress;
    ULONG Processor;

    if (KdDebuggerDataBlock == NULL)
        return FALSE;

#if defined(_M_IX86) && (defined(__GNUC__) || defined(__clang__))
    ProcessorBlockAddress = KdDebuggerDataBlock->KiProcessorBlock.ptr;
#else
    ProcessorBlockAddress = (ULONG_PTR)KdDebuggerDataBlock->KiProcessorBlock;
#endif
    if (ProcessorBlockAddress == 0)
        return FALSE;

    ProcessorBlock = (PKPRCB*)ProcessorBlockAddress;
    for (Processor = 0; Processor < CurrentStateChange.NumberProcessors; Processor++)
    {
        if (ProcessorBlock[Processor] == NULL)
            continue;

        gdb_arch_program_hw_breakpoints(
            &ProcessorBlock[Processor]->ProcessorState.SpecialRegisters,
            HardwareBreakpoints);
    }

    gdb_arch_report_hw_breakpoints(HardwareBreakpoints);
    return TRUE;
}

BOOLEAN
gdb_get_watchpoint_stop(_Out_ const CHAR** Reason, _Out_ PULONG64 Address)
{
    ULONG Slot;

    if (CurrentStateChange.NewState != DbgKdExceptionStateChange ||
        CurrentStateChange.u.Exception.ExceptionRecord.ExceptionCode != STATUS_SINGLE_STEP)
    {
        return FALSE;
    }

    for (Slot = 0; Slot < gdb_hw_breakpoint_count; Slot++)
    {
        if (!HardwareBreakpoints[Slot].Active ||
            HardwareBreakpoints[Slot].Type == GDB_HW_EXECUTE ||
            !gdb_arch_hw_breakpoint_hit(HardwareBreakpoints, Slot))
        {
            continue;
        }

        *Reason = HardwareBreakpoints[Slot].Type == GDB_HW_WRITE ? "watch" :
                  HardwareBreakpoints[Slot].Type == GDB_HW_READ ? "rwatch" : "awatch";
        *Address = HardwareBreakpoints[Slot].Address;
        return TRUE;
    }

    return FALSE;
}

static inline
KDSTATUS
LOOP_IF_SUCCESS(int x)
{
    return (x == KdPacketReceived) ? (KDSTATUS)-1 : x;
}

/* PRIVATE FUNCTIONS **********************************************************/
static
VOID
reset_thread_info(VOID)
{
    PETHREAD CurrentThread = (PETHREAD)(ULONG_PTR)CurrentStateChange.Thread;

    ThreadInfoInitialTid = handle_to_gdb_tid(PsGetThreadId(CurrentThread));
    ThreadInfoCurrentPending = TRUE;
    ThreadInfoIdlePending = (ThreadInfoInitialTid != 1);
    ThreadInfoThreadEntry = NULL;

    if (ps_initialized())
        ThreadInfoProcessEntry = ProcessListHead->Flink;
    else
        ThreadInfoProcessEntry = NULL;
}

/*
 * Emit the next thread of the enumeration, returning its length.
 * Zero means the enumeration is over: all the cursors above are then either
 * NULL or parked on the list head, so a further call keeps returning zero.
 */
static
LONG
get_next_thread_info(
    _Out_writes_(BufferSize) char* Buffer,
    _In_ SIZE_T BufferSize)
{
    if (ThreadInfoCurrentPending)
    {
        PETHREAD Thread = (PETHREAD)(ULONG_PTR)CurrentStateChange.Thread;

        ThreadInfoCurrentPending = FALSE;
        return format_gdb_tid(Buffer, BufferSize,
                              PsGetThreadProcessId(Thread),
                              ThreadInfoInitialTid);
    }

    if (ThreadInfoIdlePending)
    {
        ThreadInfoIdlePending = FALSE;
        return format_gdb_tid(Buffer, BufferSize, gdb_pid_to_handle(1), 1);
    }

    while (ThreadInfoProcessEntry &&
           ThreadInfoProcessEntry != ProcessListHead)
    {
        PEPROCESS Process;

        Process = CONTAINING_RECORD(ThreadInfoProcessEntry,
                                    EPROCESS,
                                    ActiveProcessLinks);
        if (ThreadInfoThreadEntry == NULL)
            ThreadInfoThreadEntry = Process->ThreadListHead.Flink;

        while (ThreadInfoThreadEntry != &Process->ThreadListHead)
        {
            PETHREAD Thread;
            UINT_PTR Tid;

            Thread = CONTAINING_RECORD(ThreadInfoThreadEntry,
                                       ETHREAD,
                                       ThreadListEntry);
            ThreadInfoThreadEntry = ThreadInfoThreadEntry->Flink;
            Tid = handle_to_gdb_tid(Thread->Cid.UniqueThread);

            if ((Tid == ThreadInfoInitialTid) || (Tid == 1))
                continue;

            return format_gdb_tid(Buffer, BufferSize,
                                  Process->UniqueProcessId, Tid);
        }

        ThreadInfoProcessEntry = ThreadInfoProcessEntry->Flink;
        ThreadInfoThreadEntry = NULL;
    }

    return 0;
}

static
KDSTATUS
send_thread_info(
    _In_ BOOLEAN Reset)
{
    char gdb_out[40];
    SIZE_T PacketLength;
    LONG Length;

    if (Reset)
        reset_thread_info();

    Length = get_next_thread_info(gdb_out, sizeof(gdb_out));
    if (Length <= 0)
        return send_gdb_packet("l");

    start_gdb_packet();
    send_gdb_partial_packet("m");
    send_gdb_partial_packet(gdb_out);
    PacketLength = 1 + Length;

    while ((GDB_PACKET_MAX_SIZE - PacketLength) >= sizeof(gdb_out))
    {
        Length = get_next_thread_info(gdb_out, sizeof(gdb_out));
        if (Length <= 0)
            break;

        send_gdb_partial_packet(",");
        send_gdb_partial_packet(gdb_out);
        PacketLength += 1 + Length;
    }

    return finish_gdb_packet();
}

/*
 * Rendering the library list twice - once to measure it, once to emit the
 * requested window - avoids buffering the whole XML document, which can be
 * several kilobytes. The measuring pass is simply a stream with a zero length
 * window, which never matches a chunk and so emits nothing.
 */
typedef struct _GDB_XFER_STREAM
{
    ULONG64 Offset;
    ULONG64 Length;
    ULONG64 Position;
} GDB_XFER_STREAM;

static
VOID
stream_xfer_data(
    _Inout_ GDB_XFER_STREAM* Stream,
    _In_reads_bytes_(Length) const VOID* Buffer,
    _In_ SIZE_T Length)
{
    ULONG64 ChunkStart = Stream->Position;
    ULONG64 ChunkEnd = ChunkStart + Length;
    ULONG64 RangeStart;
    ULONG64 RangeEnd;

    Stream->Position = ChunkEnd;
    if (ChunkEnd <= Stream->Offset || ChunkStart >= Stream->Offset + Stream->Length)
        return;

    RangeStart = max(ChunkStart, Stream->Offset);
    RangeEnd = min(ChunkEnd, Stream->Offset + Stream->Length);
    send_gdb_partial_binary((const UCHAR*)Buffer + (SIZE_T)(RangeStart - ChunkStart),
                            (SIZE_T)(RangeEnd - RangeStart));
}

#define STREAM_XFER_LITERAL(Stream, Literal) \
    stream_xfer_data((Stream), (Literal), sizeof(Literal) - 1)

static
VOID
stream_library_name(
    _Inout_ GDB_XFER_STREAM* Stream,
    _In_ const UNICODE_STRING* Name)
{
    USHORT i;

    if (Name->Buffer == NULL)
        return;

    for (i = 0; i < Name->Length / sizeof(WCHAR); i++)
    {
        WCHAR WideCharacter = Name->Buffer[i];
        CHAR Character;

        if (WideCharacter >= L'A' && WideCharacter <= L'Z')
            WideCharacter += L'a' - L'A';

        if (WideCharacter > 0x7f)
        {
            Character = '?';
            stream_xfer_data(Stream, &Character, sizeof(Character));
            continue;
        }

        Character = (CHAR)WideCharacter;
        switch (Character)
        {
            case '&':
                STREAM_XFER_LITERAL(Stream, "&amp;");
                break;
            case '<':
                STREAM_XFER_LITERAL(Stream, "&lt;");
                break;
            case '>':
                STREAM_XFER_LITERAL(Stream, "&gt;");
                break;
            case '"':
                STREAM_XFER_LITERAL(Stream, "&quot;");
                break;
            case '\'':
                STREAM_XFER_LITERAL(Stream, "&apos;");
                break;
            default:
                stream_xfer_data(Stream, &Character, sizeof(Character));
                break;
        }
    }
}

static
VOID
stream_libraries_xml(
    _Inout_ GDB_XFER_STREAM* Stream)
{
    LIST_ENTRY* CurrentEntry;

    STREAM_XFER_LITERAL(Stream, "<?xml version=\"1.0\"?>");
    STREAM_XFER_LITERAL(Stream, "<library-list>");

    if (modules_initialized())
    {
        for (CurrentEntry = ModuleListHead->Flink;
             CurrentEntry != ModuleListHead;
             CurrentEntry = CurrentEntry->Flink)
        {
            PLDR_DATA_TABLE_ENTRY TableEntry;
            CHAR Address[2 + sizeof(ULONG_PTR) * 2 + 1];
            PVOID DllBase;
            LONG AddressLength;

            TableEntry = CONTAINING_RECORD(CurrentEntry,
                                           LDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);
            DllBase = (PVOID)((ULONG_PTR)TableEntry->DllBase + 0x1000);

            STREAM_XFER_LITERAL(Stream, "<library name=\"C:\\");
            stream_library_name(Stream, &TableEntry->BaseDllName);
            STREAM_XFER_LITERAL(Stream, "\"><segment address=\"0x");
            AddressLength = _snprintf(Address, sizeof(Address), "%p", DllBase);
            if (AddressLength > 0)
                stream_xfer_data(Stream, Address, AddressLength);
            STREAM_XFER_LITERAL(Stream, "\"/></library>");
        }
    }

    STREAM_XFER_LITERAL(Stream, "</library-list>");
}

static
VOID
stream_thread_xml(
    _Inout_ GDB_XFER_STREAM* Stream,
    _In_ PETHREAD Thread,
    _In_ BOOLEAN Current)
{
    CHAR ThreadId[40];
    LONG Length;

    STREAM_XFER_LITERAL(Stream, "<thread id=\"");
    Length = format_gdb_tid(ThreadId,
                           sizeof(ThreadId),
                           PsGetThreadProcessId(Thread),
                           handle_to_gdb_tid(PsGetThreadId(Thread)));
    if (Length > 0)
        stream_xfer_data(Stream, ThreadId, Length);
    if (Current)
    {
        CHAR Core[2 + sizeof(ULONG) * 2 + 1];

        STREAM_XFER_LITERAL(Stream, "\" core=\"");
        Length = _snprintf(Core, sizeof(Core), "%x", CurrentStateChange.Processor);
        if (Length > 0)
            stream_xfer_data(Stream, Core, Length);
    }
    STREAM_XFER_LITERAL(Stream, "\"/>");
}

static
VOID
stream_threads_xml(
    _Inout_ GDB_XFER_STREAM* Stream)
{
    PETHREAD CurrentThread = (PETHREAD)(ULONG_PTR)CurrentStateChange.Thread;
    LIST_ENTRY* ProcessEntry;

    STREAM_XFER_LITERAL(Stream, "<?xml version=\"1.0\"?>");
    STREAM_XFER_LITERAL(Stream, "<threads>");
    stream_thread_xml(Stream, CurrentThread, TRUE);

    if (TheIdleThread && TheIdleThread != CurrentThread)
        stream_thread_xml(Stream, TheIdleThread, FALSE);

    if (ps_initialized())
    {
        for (ProcessEntry = ProcessListHead->Flink;
             ProcessEntry != ProcessListHead;
             ProcessEntry = ProcessEntry->Flink)
        {
            PEPROCESS Process;
            LIST_ENTRY* ThreadEntry;

            Process = CONTAINING_RECORD(ProcessEntry,
                                         EPROCESS,
                                         ActiveProcessLinks);
            for (ThreadEntry = Process->ThreadListHead.Flink;
                 ThreadEntry != &Process->ThreadListHead;
                 ThreadEntry = ThreadEntry->Flink)
            {
                PETHREAD Thread;

                Thread = CONTAINING_RECORD(ThreadEntry,
                                           ETHREAD,
                                           ThreadListEntry);
                if (Thread != CurrentThread && Thread != TheIdleThread)
                    stream_thread_xml(Stream, Thread, FALSE);
            }
        }
    }

    STREAM_XFER_LITERAL(Stream, "</threads>");
}

typedef VOID (*PGDB_XFER_STREAMER)(_Inout_ GDB_XFER_STREAM* Stream);

static
KDSTATUS
send_xfer_stream(
    _In_ PGDB_XFER_STREAMER Streamer,
    _In_ ULONG64 RequestedOffset,
    _In_ ULONG64 RequestedLength)
{
    GDB_XFER_STREAM Stream = {0};
    ULONG64 Offset;
    ULONG64 Length;
    ULONG64 TotalLength;

    /* Measuring pass: a zero length window emits nothing */
    Streamer(&Stream);
    TotalLength = Stream.Position;

    Offset = min(RequestedOffset, TotalLength);
    Length = min(RequestedLength, (ULONG64)GDB_XFER_MAX_SIZE);
    Length = min(Length, TotalLength - Offset);

    start_gdb_packet();
    send_gdb_partial_packet((Offset + Length < TotalLength) ? "m" : "l");
    Stream.Offset = Offset;
    Stream.Length = Length;
    Stream.Position = 0;
    Streamer(&Stream);
    return finish_gdb_packet();
}

static
KDSTATUS
send_xfer_buffer(
    _In_reads_bytes_(BufferLength) const VOID* Buffer,
    _In_ SIZE_T BufferLength,
    _In_ ULONG64 Offset,
    _In_ ULONG64 Length)
{
    if (Offset > BufferLength)
        Offset = BufferLength;
    if (Length > GDB_XFER_MAX_SIZE)
        Length = GDB_XFER_MAX_SIZE;
    if (Length > BufferLength - Offset)
        Length = BufferLength - Offset;

    start_gdb_packet();
    send_gdb_partial_packet((Offset + Length < BufferLength) ? "m" : "l");
    send_gdb_partial_binary((const UCHAR*)Buffer + (SIZE_T)Offset, (SIZE_T)Length);
    return finish_gdb_packet();
}

/*
 * Monitor output is streamed into as few 'O' packets as possible: every
 * finished packet costs a full round-trip waiting for GDB to acknowledge it,
 * and a listing can run to hundreds of lines.
 */
static ULONG MonitorPacketLength;

static
KDSTATUS
flush_monitor_output(VOID)
{
    if (MonitorPacketLength == 0)
        return KdPacketReceived;

    MonitorPacketLength = 0;
    return finish_gdb_packet();
}

static
KDSTATUS
send_monitor_output(
    _In_z_ _Printf_format_string_ const CHAR* Format,
    ...)
{
    CHAR Buffer[256];
    LONG Length;
    va_list Arguments;
    KDSTATUS Status;

    va_start(Arguments, Format);
    Length = _vsnprintf(Buffer, sizeof(Buffer), Format, Arguments);
    va_end(Arguments);
    if (Length < 0)
        Length = sizeof(Buffer) - 1;

    /* Start a packet, or close the current one if this line no longer fits */
    if (MonitorPacketLength != 0 &&
        (MonitorPacketLength + Length * 2) > GDB_MEMORY_MAX_SIZE)
    {
        Status = flush_monitor_output();
        if (Status != KdPacketReceived)
            return Status;
    }

    if (MonitorPacketLength == 0)
    {
        start_gdb_packet();
        send_gdb_partial_packet("O");
        MonitorPacketLength = 1;
    }

    send_gdb_partial_memory(Buffer, Length);
    MonitorPacketLength += Length * 2;
    return KdPacketReceived;
}

#define MONITOR_PRINT(...) \
    do { \
        if (Status == KdPacketReceived) \
            Status = send_monitor_output(__VA_ARGS__); \
    } while (0)

static
VOID
copy_module_name(
    _In_ const UNICODE_STRING* Name,
    _Out_writes_(BufferSize) CHAR* Buffer,
    _In_ SIZE_T BufferSize)
{
    SIZE_T Length;
    SIZE_T i;

    if (BufferSize == 0)
        return;
    if (Name->Buffer == NULL)
    {
        Buffer[0] = '\0';
        return;
    }

    Length = min(Name->Length / sizeof(WCHAR), BufferSize - 1);
    for (i = 0; i < Length; i++)
    {
        WCHAR WideCharacter = Name->Buffer[i];

        if (WideCharacter >= L'A' && WideCharacter <= L'Z')
            WideCharacter += L'a' - L'A';
        Buffer[i] = (WideCharacter <= 0x7f) ? (CHAR)WideCharacter : '?';
    }
    Buffer[Length] = '\0';
}

static
KDSTATUS
handle_gdb_monitor_command(VOID)
{
    CHAR CommandBuffer[128];
    CHAR* Command;
    SIZE_T CommandLength;
    KDSTATUS Status = KdPacketReceived;

    if (gdb_input_length < 6 ||
        ((gdb_input_length - 6) & 1) != 0 ||
        (gdb_input_length - 6) / 2 >= sizeof(CommandBuffer) ||
        !gdb_decode_hex(&gdb_input[6],
                        gdb_input_length - 6,
                        CommandBuffer,
                        (gdb_input_length - 6) / 2))
    {
        return send_gdb_packet("E01");
    }

    CommandLength = (gdb_input_length - 6) / 2;
    CommandBuffer[CommandLength] = '\0';
    Command = CommandBuffer;
    while (*Command == ' ' || *Command == '\t')
        Command++;
    while (CommandLength != 0 &&
           (CommandBuffer[CommandLength - 1] == ' ' ||
            CommandBuffer[CommandLength - 1] == '\t'))
    {
        CommandBuffer[--CommandLength] = '\0';
    }

    if (strcmp(Command, "help") == 0 || *Command == '\0')
    {
        MONITOR_PRINT("KDGDB monitor commands:\n");
        MONITOR_PRINT("  version    kernel and KD protocol information\n");
        MONITOR_PRINT("  processes  active process list\n");
        MONITOR_PRINT("  threads    active thread list\n");
        MONITOR_PRINT("  modules    loaded kernel module list\n");
    }
    else if (strcmp(Command, "version") == 0)
    {
        MONITOR_PRINT("KD %u.%u protocol %u machine 0x%x\n",
                      KdVersion.MajorVersion,
                      KdVersion.MinorVersion,
                      KdVersion.ProtocolVersion,
                      KdVersion.MachineType);
        MONITOR_PRINT("kernel base 0x%I64x processor %u\n",
                      KdVersion.KernBase,
                      CurrentStateChange.Processor);
    }
    else if (strcmp(Command, "processes") == 0)
    {
        LIST_ENTRY* ProcessEntry;

        MONITOR_PRINT("pid              process\n");
        if (TheIdleProcess)
            MONITOR_PRINT("%-16" PRIxPTR " Idle\n", handle_to_gdb_pid(NULL));

        if (ps_initialized())
        {
            for (ProcessEntry = ProcessListHead->Flink;
                 ProcessEntry != ProcessListHead && Status == KdPacketReceived;
                 ProcessEntry = ProcessEntry->Flink)
            {
                PEPROCESS Process;

                Process = CONTAINING_RECORD(ProcessEntry,
                                             EPROCESS,
                                             ActiveProcessLinks);
                MONITOR_PRINT("%-16" PRIxPTR " %.15s\n",
                              handle_to_gdb_pid(Process->UniqueProcessId),
                              Process->ImageFileName);
            }
        }
    }
    else if (strcmp(Command, "threads") == 0)
    {
        LIST_ENTRY* ProcessEntry;

        MONITOR_PRINT("tid              pid              state priority process\n");
        if (ps_initialized())
        {
            for (ProcessEntry = ProcessListHead->Flink;
                 ProcessEntry != ProcessListHead && Status == KdPacketReceived;
                 ProcessEntry = ProcessEntry->Flink)
            {
                PEPROCESS Process;
                LIST_ENTRY* ThreadEntry;

                Process = CONTAINING_RECORD(ProcessEntry,
                                             EPROCESS,
                                             ActiveProcessLinks);
                for (ThreadEntry = Process->ThreadListHead.Flink;
                     ThreadEntry != &Process->ThreadListHead && Status == KdPacketReceived;
                     ThreadEntry = ThreadEntry->Flink)
                {
                    PETHREAD Thread;

                    Thread = CONTAINING_RECORD(ThreadEntry,
                                               ETHREAD,
                                               ThreadListEntry);
                    MONITOR_PRINT("%-16" PRIxPTR " %-16" PRIxPTR " %-5u %-8d %.15s\n",
                                  handle_to_gdb_tid(Thread->Cid.UniqueThread),
                                  handle_to_gdb_pid(Process->UniqueProcessId),
                                  Thread->Tcb.State,
                                  Thread->Tcb.Priority,
                                  Process->ImageFileName);
                }
            }
        }
    }
    else if (strcmp(Command, "modules") == 0)
    {
        LIST_ENTRY* ModuleEntry;

        MONITOR_PRINT("base              size       module\n");
        if (modules_initialized())
        {
            for (ModuleEntry = ModuleListHead->Flink;
                 ModuleEntry != ModuleListHead && Status == KdPacketReceived;
                 ModuleEntry = ModuleEntry->Flink)
            {
                PLDR_DATA_TABLE_ENTRY Module;
                CHAR Name[96];

                Module = CONTAINING_RECORD(ModuleEntry,
                                           LDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);
                copy_module_name(&Module->BaseDllName, Name, sizeof(Name));
                MONITOR_PRINT("%p %08lx %s\n",
                              Module->DllBase,
                              Module->SizeOfImage,
                              Name);
            }
        }
    }
    else
    {
        MONITOR_PRINT("Unknown KDGDB monitor command: %s\n", Command);
    }

    if (Status == KdPacketReceived)
        Status = flush_monitor_output();
    if (Status != KdPacketReceived)
        return Status;
    return send_gdb_packet("OK");
}

/* H* packets */
static
KDSTATUS
handle_gdb_set_thread(void)
{
    const char* End = &gdb_input[gdb_input_length];
    UINT_PTR Pid;
    UINT_PTR Tid;
    KDSTATUS Status;

    if (gdb_input_length < 3 ||
        (gdb_input[1] != 'c' && gdb_input[1] != 'g') ||
        !parse_gdb_thread_id(&gdb_input[2], End, &Pid, &Tid))
    {
        return send_gdb_packet("E01");
    }

    if (Tid != 0 && Tid != (UINT_PTR)-1 && find_thread(Pid, Tid) == NULL)
        return send_gdb_packet("E03");

    switch (gdb_input[1])
    {
    case 'c':
        gdb_run_tid = Tid;
        Status = send_gdb_packet("OK");
        break;
    case 'g':
        KDDBGPRINT("Setting debug thread: %s.\n", gdb_input);
        gdb_dbg_pid = Pid;
        gdb_dbg_tid = Tid;
        Status = send_gdb_packet("OK");
        break;
    default:
        KDDBGPRINT("KDGDB: Unknown 'H' command: %s\n", gdb_input);
        Status = send_gdb_packet("");
    }

    return Status;
}

static
KDSTATUS
handle_gdb_thread_alive(void)
{
    const char* End = &gdb_input[gdb_input_length];
    UINT_PTR Pid, Tid;
    PETHREAD Thread;
    KDSTATUS Status;

    if (!parse_gdb_thread_id(&gdb_input[1], End, &Pid, &Tid) ||
        Tid == 0 || Tid == (UINT_PTR)-1)
    {
        return send_gdb_packet("E01");
    }

    KDDBGPRINT("Checking if p%p.%p is alive.\n", Pid, Tid);

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
    if ((strcmp(gdb_input, "qSupported") == 0) ||
        (strncmp(gdb_input, "qSupported:", 11) == 0))
    {
#if MONOPROCESS
        return send_gdb_packet("PacketSize=1000;QStartNoAckMode+;binary-upload+;qXfer:exec-file:read+;qXfer:features:read+;qXfer:libraries:read+;qXfer:memory-map:read+;qXfer:threads:read+;vContSupported+;");
#else
        return send_gdb_packet("PacketSize=1000;QStartNoAckMode+;binary-upload+;multiprocess+;qXfer:exec-file:read+;qXfer:features:read+;qXfer:libraries:read+;qXfer:memory-map:read+;qXfer:threads:read+;vContSupported+;");
#endif
    }

    if (strcmp(gdb_input, "qAttached") == 0)
    {
        return send_gdb_packet("1");
    }

    if (strncmp(gdb_input, "qRcmd,", 6) == 0)
    {
        return handle_gdb_monitor_command();
    }

    if (strcmp(gdb_input, "qC") == 0)
    {
        PETHREAD Thread = (PETHREAD)(ULONG_PTR)CurrentStateChange.Thread;
        char gdb_out[64] = "QC";

        format_gdb_tid(&gdb_out[2], sizeof(gdb_out) - 2,
                       PsGetThreadProcessId(Thread),
                       handle_to_gdb_tid(PsGetThreadId(Thread)));
        return send_gdb_packet(gdb_out);
    }

    if (strcmp(gdb_input, "qfThreadInfo") == 0)
        return send_thread_info(TRUE);

    if (strcmp(gdb_input, "qsThreadInfo") == 0)
        return send_thread_info(FALSE);

    if (strncmp(gdb_input, "qGetTIBAddr:", 12) == 0)
    {
        const char* End = &gdb_input[gdb_input_length];
        UINT_PTR Pid, Tid;
        PETHREAD Thread;

        if (!parse_gdb_thread_id(&gdb_input[12], End, &Pid, &Tid) ||
            Tid == 0 || Tid == (UINT_PTR)-1)
        {
            return send_gdb_packet("E01");
        }

        Thread = find_thread(Pid, Tid);
        if (Thread == NULL)
            return send_gdb_packet("E03");

        return send_gdb_memory(&Thread->Tcb.Teb, sizeof(Thread->Tcb.Teb));
    }

    if (strncmp(gdb_input, "qThreadExtraInfo,", 17) == 0)
    {
        const char* End = &gdb_input[gdb_input_length];
        UINT_PTR Pid, Tid;
        PETHREAD Thread;
        PEPROCESS Process;
        char out_string[64];
        STRING String = {0, 64, out_string};

        KDDBGPRINT("Giving extra info for");

        if (!parse_gdb_thread_id(&gdb_input[17], End, &Pid, &Tid) ||
            Tid == 0 || Tid == (UINT_PTR)-1)
        {
            return send_gdb_packet("E01");
        }

        Thread = find_thread(Pid, Tid);

        if (Thread == NULL)
            return send_gdb_packet("E03");

#if MONOPROCESS
        Process = CONTAINING_RECORD(Thread->Tcb.Process, EPROCESS, Pcb);
#else
        Process = find_process(Pid);
        if (Process == NULL)
            return send_gdb_packet("E03");
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

    if (strncmp(gdb_input, "qXfer:features:read:target.xml:", 31) == 0)
    {
        const char* End = &gdb_input[gdb_input_length];
        const char* Rest;
        ULONG64 Window[2];

        if (!parse_hex_fields(&gdb_input[31], End, ",", Window, RTL_NUMBER_OF(Window), &Rest) ||
            Rest != End)
        {
            return send_gdb_packet("E01");
        }

        return send_xfer_buffer(gdb_target_xml,
                                gdb_target_xml_length,
                                Window[0],
                                Window[1]);
    }

    if (strncmp(gdb_input, "qXfer:exec-file:read::", 22) == 0)
    {
        static const CHAR KernelName[] = "C:\\ntoskrnl.exe";
        const char* End = &gdb_input[gdb_input_length];
        const char* Rest;
        ULONG64 Window[2];

        if (!parse_hex_fields(&gdb_input[22], End, ",", Window, RTL_NUMBER_OF(Window), &Rest) ||
            Rest != End)
        {
            return send_gdb_packet("E01");
        }

        return send_xfer_buffer(KernelName,
                                sizeof(KernelName) - 1,
                                Window[0],
                                Window[1]);
    }

    if (strncmp(gdb_input, "qXfer:libraries:read::", 22) == 0)
    {
        const char* End = &gdb_input[gdb_input_length];
        const char* Rest;
        ULONG64 Window[2];

        KDDBGPRINT("KDGDB: qXfer:libraries:read\n");

        if (!parse_hex_fields(&gdb_input[22], End, ",", Window, RTL_NUMBER_OF(Window), &Rest) ||
            Rest != End)
        {
            return send_gdb_packet("E01");
        }

        return send_xfer_stream(stream_libraries_xml, Window[0], Window[1]);
    }

    if (strncmp(gdb_input, "qXfer:memory-map:read::", 23) == 0)
    {
        static const CHAR MemoryMap[] = "<?xml version=\"1.0\"?><memory-map></memory-map>";
        const char* End = &gdb_input[gdb_input_length];
        const char* Rest;
        ULONG64 Window[2];

        if (!parse_hex_fields(&gdb_input[23], End, ",", Window, RTL_NUMBER_OF(Window), &Rest) || Rest != End)
            return send_gdb_packet("E01");

        return send_xfer_buffer(MemoryMap, sizeof(MemoryMap) - 1, Window[0], Window[1]);
    }

    if (strncmp(gdb_input, "qXfer:threads:read::", 20) == 0)
    {
        const char* End = &gdb_input[gdb_input_length];
        const char* Rest;
        ULONG64 Window[2];

        if (!parse_hex_fields(&gdb_input[20], End, ",", Window, RTL_NUMBER_OF(Window), &Rest) ||
            Rest != End)
        {
            return send_gdb_packet("E01");
        }

        return send_xfer_stream(stream_threads_xml, Window[0], Window[1]);
    }

    KDDBGPRINT("KDGDB: Unknown query: %s\n", gdb_input);
    return send_gdb_packet("");
}

static UCHAR SearchPattern[PACKET_MAX_SIZE];

static
BOOLEAN
SearchMemorySendHandler(_In_ ULONG PacketType, _In_ PSTRING MessageHeader, _In_ PSTRING MessageData)
{
    DBGKD_MANIPULATE_STATE64* State = (DBGKD_MANIPULATE_STATE64*)MessageHeader->Buffer;

    UNREFERENCED_PARAMETER(MessageData);

    if (PacketType != PACKET_TYPE_KD_STATE_MANIPULATE || State->ApiNumber != DbgKdSearchMemoryApi)
    {
        KDDBGPRINT("Wrong packet received after DbgKdSearchMemoryApi request.\n");
        return FALSE;
    }

    if (NT_SUCCESS(State->ReturnStatus))
    {
        CHAR Reply[2 + sizeof(ULONG64) * 2 + 1];

        _snprintf(Reply, sizeof(Reply), "1,%" PRIx64, State->u.SearchMemory.FoundAddress);
        send_gdb_packet(Reply);
    }
    else if (State->ReturnStatus == STATUS_NOT_FOUND)
    {
        send_gdb_packet("0");
    }
    else
    {
        send_gdb_ntstatus(State->ReturnStatus);
    }

    KdpSendPacketHandler = NULL;
    KdpManipulateStateHandler = NULL;

#if MONOPROCESS
    if (gdb_dbg_tid != 0)
#else
    if ((gdb_dbg_pid != 0) && gdb_pid_to_handle(gdb_dbg_pid) != PsGetCurrentProcessId())
#endif
    {
        if (ps_initialized())
            __writecr3(KdpGetDirectoryTableBase(&PsGetCurrentProcess()->Pcb));
    }

    return TRUE;
}

static
KDSTATUS
handle_gdb_search_memory(_Out_ DBGKD_MANIPULATE_STATE64* State, _Out_ PSTRING MessageData, _Out_ PULONG MessageLength, _Inout_ PKD_CONTEXT KdContext)
{
    const char* Current = &gdb_input[15];
    const char* End = &gdb_input[gdb_input_length];
    ULONG64 Address;
    ULONG64 Length;
    ULONG PatternLength;

    UNREFERENCED_PARAMETER(KdContext);

    if (!parse_hex_value(Current, End, &Address, &Current) || Current == End || *Current++ != ';' ||
        !parse_hex_value(Current, End, &Length, &Current) || Current == End || *Current++ != ';')
    {
        return LOOP_IF_SUCCESS(send_gdb_packet("E01"));
    }

    PatternLength = (ULONG)(End - Current);
    if (PatternLength == 0 || PatternLength > sizeof(SearchPattern) || Address > MAXULONG_PTR || Length > ~(ULONG64)0 - Address)
        return LOOP_IF_SUCCESS(send_gdb_packet("E01"));
    if (PatternLength > Length)
        return LOOP_IF_SUCCESS(send_gdb_packet("0"));

    RtlCopyMemory(SearchPattern, Current, PatternLength);
    State->ApiNumber = DbgKdSearchMemoryApi;
    State->ReturnStatus = STATUS_SUCCESS;
    State->Processor = CurrentStateChange.Processor;
    State->ProcessorLevel = CurrentStateChange.ProcessorLevel;
    State->u.SearchMemory.SearchAddress = Address;
    State->u.SearchMemory.SearchLength = Length;
    State->u.SearchMemory.PatternLength = PatternLength;
    MessageData->Buffer = (CHAR*)SearchPattern;
    MessageData->Length = (USHORT)PatternLength;
    *MessageLength = PatternLength;

#if MONOPROCESS
    if ((gdb_dbg_tid != 0) && gdb_tid_to_handle(gdb_dbg_tid) != PsGetCurrentThreadId())
    {
        PETHREAD AttachedThread = find_thread(0, gdb_dbg_tid);
        PKPROCESS AttachedProcess;

        if (AttachedThread == NULL || AttachedThread->Tcb.Process == NULL)
            return LOOP_IF_SUCCESS(send_gdb_packet("E03"));
        AttachedProcess = AttachedThread->Tcb.Process;
        __writecr3(KdpGetDirectoryTableBase(AttachedProcess));
    }
#else
    if ((gdb_dbg_pid != 0) && gdb_pid_to_handle(gdb_dbg_pid) != PsGetCurrentProcessId())
    {
        PEPROCESS AttachedProcess = find_process(gdb_dbg_pid);

        if (AttachedProcess == NULL)
            return LOOP_IF_SUCCESS(send_gdb_packet("E03"));
        if (ps_initialized())
            __writecr3(KdpGetDirectoryTableBase(&AttachedProcess->Pcb));
    }
#endif

    KdpSendPacketHandler = SearchMemorySendHandler;
    return KdPacketReceived;
}

static BOOLEAN ReadMemoryBinary;

static
BOOLEAN
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
        return FALSE;
    }

    if (State->ApiNumber != DbgKdReadVirtualMemoryApi)
    {
        KDDBGPRINT("Wrong API number (%lu) after DbgKdReadVirtualMemoryApi request.\n", State->ApiNumber);
        return FALSE;
    }

    /* Check status. Allow to send partial data. */
    if (!MessageData->Length && !NT_SUCCESS(State->ReturnStatus))
        send_gdb_ntstatus(State->ReturnStatus);
    else if (ReadMemoryBinary)
    {
        start_gdb_packet();
        send_gdb_partial_binary(MessageData->Buffer, MessageData->Length);
        finish_gdb_packet();
    }
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
        if (ps_initialized())
            __writecr3(KdpGetDirectoryTableBase(&PsGetCurrentProcess()->Pcb));
    }

    return TRUE;
}

static
KDSTATUS
handle_gdb_read_mem(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext)
{
    const char* End = &gdb_input[gdb_input_length];
    const char* Rest;
    ULONG64 Request[2];
    ULONG64 Address;
    ULONG64 Length;

    if (!parse_hex_fields(&gdb_input[1], End, ",", Request, RTL_NUMBER_OF(Request), &Rest) ||
        Rest != End || Request[1] > GDB_MEMORY_MAX_SIZE)
    {
        return LOOP_IF_SUCCESS(send_gdb_packet("E01"));
    }

    Address = Request[0];
    Length = Request[1];
    if (Length == 0)
        return LOOP_IF_SUCCESS(send_gdb_packet(""));

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
        __writecr3(KdpGetDirectoryTableBase(AttachedProcess));
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
        if (ps_initialized())
            __writecr3(KdpGetDirectoryTableBase(&AttachedProcess->Pcb));
    }
#endif

    State->u.ReadMemory.TargetBaseAddress = Address;
    State->u.ReadMemory.TransferCount = (ULONG)Length;
    ReadMemoryBinary = (gdb_input[0] == 'x');

    /* KD will reply with KdSendPacket. Catch it */
    KdpSendPacketHandler = ReadMemorySendHandler;
    return KdPacketReceived;
}

static
BOOLEAN
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
        return FALSE;
    }

    if (State->ApiNumber != DbgKdWriteVirtualMemoryApi)
    {
        KDDBGPRINT("Wrong API number (%lu) after DbgKdWriteVirtualMemoryApi request.\n", State->ApiNumber);
        return FALSE;
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
        if (ps_initialized())
            __writecr3(KdpGetDirectoryTableBase(&PsGetCurrentProcess()->Pcb));
    }
    return TRUE;
}

static
KDSTATUS
handle_gdb_write_mem(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext)
{
    static UCHAR OutBuffer[GDB_PACKET_MAX_SIZE];
    const char* End = &gdb_input[gdb_input_length];
    const char* Blob;
    /* 'M' carries a hex-encoded payload, 'X' a binary one */
    BOOLEAN IsHexEncoded = (gdb_input[0] == 'M');
    ULONG64 Request[2];
    ULONG64 Address;
    ULONG64 Length;
    ULONG BufferLength;

    if (!parse_hex_fields(&gdb_input[1], End, ",:", Request, RTL_NUMBER_OF(Request), &Blob))
        return LOOP_IF_SUCCESS(send_gdb_packet("E01"));

    Address = Request[0];
    Length = Request[1];
    if (Length > (IsHexEncoded ? GDB_MEMORY_MAX_SIZE : sizeof(OutBuffer)))
        return LOOP_IF_SUCCESS(send_gdb_packet("E01"));

    BufferLength = (ULONG)Length;
    if (IsHexEncoded)
    {
        /* gdb_decode_hex enforces that the blob is exactly twice as long */
        if (!gdb_decode_hex(Blob, (ULONG)(End - Blob), OutBuffer, BufferLength))
            return LOOP_IF_SUCCESS(send_gdb_packet("E01"));
    }
    else
    {
        if ((ULONG64)(End - Blob) != Length)
            return LOOP_IF_SUCCESS(send_gdb_packet("E01"));
        RtlCopyMemory(OutBuffer, Blob, BufferLength);
    }

    if (BufferLength == 0)
    {
        /* Nothing to do */
        return LOOP_IF_SUCCESS(send_gdb_packet("OK"));
    }

    State->ApiNumber = DbgKdWriteVirtualMemoryApi;
    State->ReturnStatus = STATUS_SUCCESS; /* ? */
    State->Processor = CurrentStateChange.Processor;
    State->ProcessorLevel = CurrentStateChange.ProcessorLevel;
    State->u.WriteMemory.TargetBaseAddress = Address;
    State->u.WriteMemory.TransferCount = BufferLength;
    MessageData->Length = BufferLength;
    MessageData->Buffer = (CHAR*)OutBuffer;

    /* Set the TLB according to the process being written. Pid 0 means any process. */
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
        __writecr3(KdpGetDirectoryTableBase(AttachedProcess));
    }
#else
    if ((gdb_dbg_pid != 0) && gdb_pid_to_handle(gdb_dbg_pid) != PsGetCurrentProcessId())
    {
        PEPROCESS AttachedProcess = find_process(gdb_dbg_pid);
        if (AttachedProcess == NULL)
        {
            KDDBGPRINT("The current GDB debug process is invalid!");
            return LOOP_IF_SUCCESS(send_gdb_packet("E03"));
        }
        /* Only do this if Ps is initialized */
        if (ps_initialized())
            __writecr3(KdpGetDirectoryTableBase(&AttachedProcess->Pcb));
    }
#endif

    /* KD will reply with KdSendPacket. Catch it */
    KdpSendPacketHandler = WriteMemorySendHandler;
    return KdPacketReceived;
}

static
KDSTATUS
handle_gdb_write_registers(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext)
{
    if (!gdb_write_registers(&gdb_input[1], gdb_input_length - 1))
        return LOOP_IF_SUCCESS(send_gdb_packet("E01"));

    return SetContextManipulateHandlerWithReply(State,
                                                MessageData,
                                                MessageLength,
                                                KdContext);
}

static
KDSTATUS
handle_gdb_write_register(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext)
{
    const char* End = &gdb_input[gdb_input_length];
    const char* Value;
    ULONG64 Register;

    if (!parse_hex_fields(&gdb_input[1], End, "=", &Register, 1, &Value) ||
        Register > MAXULONG ||
        !gdb_write_register((ULONG)Register, Value, (ULONG)(End - Value)))
    {
        return LOOP_IF_SUCCESS(send_gdb_packet("E01"));
    }

    return SetContextManipulateHandlerWithReply(State,
                                                MessageData,
                                                MessageLength,
                                                KdContext);
}

static
BOOLEAN
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
        return FALSE;
    }

    if (State->ApiNumber != DbgKdWriteBreakPointApi)
    {
        KDDBGPRINT("Wrong API number (%lu) after DbgKdWriteBreakPointApi request.\n", State->ApiNumber);
        return FALSE;
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
        for (i = 0; i < RTL_NUMBER_OF(BreakPointHandles); i++)
        {
            if (BreakPointHandles[i].Handle == 0)
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
    return TRUE;
}

typedef enum _GDB_BREAKPOINT_PACKET
{
    GdbBreakpointInvalid,
    GdbBreakpointUnsupported,
    GdbBreakpointSupported
} GDB_BREAKPOINT_PACKET;

static
GDB_BREAKPOINT_PACKET
parse_breakpoint_packet(_Out_ PULONG Type, _Out_ PULONG64 Address, _Out_ PULONG Kind)
{
    const char* End = &gdb_input[gdb_input_length];
    const char* Rest;
    ULONG64 Fields[3];

    /* Type,Address,Kind - the kind may be followed by unsupported conditions */
    if (!parse_hex_fields(&gdb_input[1], End, ",,", Fields, RTL_NUMBER_OF(Fields), &Rest))
        return GdbBreakpointInvalid;

    if (Fields[0] > MAXULONG || Fields[1] > MAXULONG_PTR || Fields[2] > MAXULONG)
        return GdbBreakpointInvalid;

    *Type = (ULONG)Fields[0];
    *Address = Fields[1];
    *Kind = (ULONG)Fields[2];
    if (*Type > 4 || Rest != End)
        return GdbBreakpointUnsupported;
    if (*Kind == 0)
        return GdbBreakpointInvalid;
    return GdbBreakpointSupported;
}

static KDSTATUS
handle_gdb_insert_hardware_breakpoint(_In_ ULONG Type, _In_ ULONG64 Address, _In_ ULONG Kind)
{
    GDB_HARDWARE_BREAKPOINT Breakpoint = {Address, Kind, (UCHAR)Type, TRUE};
    ULONG FreeSlot = gdb_hw_breakpoint_count;
    ULONG Slot;

    if (!gdb_arch_hw_breakpoint_valid(&Breakpoint))
    {
        return LOOP_IF_SUCCESS(send_gdb_packet("E01"));
    }
#if defined(_M_IX86)
    if (Kind == 8)
        return LOOP_IF_SUCCESS(send_gdb_packet("E01"));
#endif

    for (Slot = 0; Slot < gdb_hw_breakpoint_count; Slot++)
    {
        if (HardwareBreakpoints[Slot].Active &&
            HardwareBreakpoints[Slot].Type == Type &&
            HardwareBreakpoints[Slot].Address == Address &&
            HardwareBreakpoints[Slot].Kind == Kind)
        {
            return LOOP_IF_SUCCESS(send_gdb_packet("OK"));
        }
        if (!HardwareBreakpoints[Slot].Active && FreeSlot == gdb_hw_breakpoint_count)
            FreeSlot = Slot;
    }

    if (FreeSlot == gdb_hw_breakpoint_count)
        return LOOP_IF_SUCCESS(send_gdb_packet("E01"));

    HardwareBreakpoints[FreeSlot] = Breakpoint;
    if (!apply_hardware_breakpoints())
    {
        RtlZeroMemory(&HardwareBreakpoints[FreeSlot], sizeof(HardwareBreakpoints[FreeSlot]));
        return LOOP_IF_SUCCESS(send_gdb_packet("E01"));
    }

    return LOOP_IF_SUCCESS(send_gdb_packet("OK"));
}

static KDSTATUS
handle_gdb_remove_hardware_breakpoint(_In_ ULONG Type, _In_ ULONG64 Address, _In_ ULONG Kind)
{
    ULONG Slot;

    for (Slot = 0; Slot < gdb_hw_breakpoint_count; Slot++)
    {
        GDB_HARDWARE_BREAKPOINT Previous;

        if (!HardwareBreakpoints[Slot].Active ||
            HardwareBreakpoints[Slot].Type != Type ||
            HardwareBreakpoints[Slot].Address != Address ||
            HardwareBreakpoints[Slot].Kind != Kind)
        {
            continue;
        }

        Previous = HardwareBreakpoints[Slot];
        RtlZeroMemory(&HardwareBreakpoints[Slot], sizeof(HardwareBreakpoints[Slot]));
        if (!apply_hardware_breakpoints())
        {
            HardwareBreakpoints[Slot] = Previous;
            return LOOP_IF_SUCCESS(send_gdb_packet("E01"));
        }
        break;
    }

    return LOOP_IF_SUCCESS(send_gdb_packet("OK"));
}

static
KDSTATUS
handle_gdb_insert_breakpoint(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext)
{
    ULONG Type;
    ULONG64 Address;
    ULONG Kind;
    GDB_BREAKPOINT_PACKET Packet;
    ULONG i;
    BOOLEAN HasFreeSlot = FALSE;

    State->ReturnStatus = STATUS_SUCCESS; /* ? */
    State->Processor = CurrentStateChange.Processor;
    State->ProcessorLevel = CurrentStateChange.ProcessorLevel;
    if (MessageData)
        MessageData->Length = 0;
    *MessageLength = 0;

    Packet = parse_breakpoint_packet(&Type, &Address, &Kind);
    if (Packet == GdbBreakpointInvalid)
        return LOOP_IF_SUCCESS(send_gdb_packet("E01"));
    if (Packet == GdbBreakpointUnsupported)
        return LOOP_IF_SUCCESS(send_gdb_packet(""));
    if (Type != 0)
        return handle_gdb_insert_hardware_breakpoint(Type, Address, Kind);

    KDDBGPRINT("Inserting breakpoint at %p.\n", (void*)(ULONG_PTR)Address);

    for (i = 0; i < RTL_NUMBER_OF(BreakPointHandles); i++)
    {
        if (BreakPointHandles[i].Handle != 0 &&
            BreakPointHandles[i].Address == (ULONG_PTR)Address)
        {
            return LOOP_IF_SUCCESS(send_gdb_packet("OK"));
        }

        if (BreakPointHandles[i].Handle == 0)
            HasFreeSlot = TRUE;
    }

    if (!HasFreeSlot)
    {
        KDDBGPRINT("No breakpoint slot available!\n");
        return LOOP_IF_SUCCESS(send_gdb_packet("E01"));
    }

    State->ApiNumber = DbgKdWriteBreakPointApi;
    State->u.WriteBreakPoint.BreakPointAddress = Address;

    /* KD will reply with KdSendPacket. Catch it */
    KdpSendPacketHandler = WriteBreakPointSendHandler;
    return KdPacketReceived;
}

static
BOOLEAN
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
        return FALSE;
    }

    if (State->ApiNumber != DbgKdRestoreBreakPointApi)
    {
        KDDBGPRINT("Wrong API number (%lu) after DbgKdRestoreBreakPointApi request.\n", State->ApiNumber);
        return FALSE;
    }

    /* We ignore failure here. If DbgKdRestoreBreakPointApi fails,
     * this means that the breakpoint was already invalid for KD. So clean it up on our side. */
    for (i = 0; i < RTL_NUMBER_OF(BreakPointHandles); i++)
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
    return TRUE;
}

static
KDSTATUS
handle_gdb_remove_breakpoint(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext)
{
    ULONG Type;
    ULONG64 Address;
    ULONG Kind;
    GDB_BREAKPOINT_PACKET Packet;
    ULONG i, Handle = 0;

    State->ReturnStatus = STATUS_SUCCESS; /* ? */
    State->Processor = CurrentStateChange.Processor;
    State->ProcessorLevel = CurrentStateChange.ProcessorLevel;
    if (MessageData)
        MessageData->Length = 0;
    *MessageLength = 0;

    Packet = parse_breakpoint_packet(&Type, &Address, &Kind);
    if (Packet == GdbBreakpointInvalid)
        return LOOP_IF_SUCCESS(send_gdb_packet("E01"));
    if (Packet == GdbBreakpointUnsupported)
        return LOOP_IF_SUCCESS(send_gdb_packet(""));
    if (Type != 0)
        return handle_gdb_remove_hardware_breakpoint(Type, Address, Kind);

    KDDBGPRINT("Removing breakpoint on %p.\n", (void*)(ULONG_PTR)Address);

    for (i = 0; i < RTL_NUMBER_OF(BreakPointHandles); i++)
    {
        if (BreakPointHandles[i].Handle != 0 &&
            BreakPointHandles[i].Address == (ULONG_PTR)Address)
        {
            Handle = BreakPointHandles[i].Handle;
            break;
        }
    }

    if (Handle == 0)
    {
        KDDBGPRINT("Received %s, but breakpoint was never inserted?!\n", gdb_input);
        return LOOP_IF_SUCCESS(send_gdb_packet("E01"));
    }

    State->ApiNumber = DbgKdRestoreBreakPointApi;
    State->u.RestoreBreakPoint.BreakPointHandle = Handle;

    /* KD will reply with KdSendPacket. Catch it */
    KdpSendPacketHandler = RestoreBreakPointSendHandler;
    return KdPacketReceived;
}

static
KDSTATUS
handle_gdb_detach(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext)
{
    KDSTATUS Status = send_gdb_packet("OK");

    if (Status != KdPacketReceived)
        return Status;

    /* A later debugger connection starts in acknowledgement mode again. */
    gdb_no_ack_mode = FALSE;
    return ContinueManipulateStateHandler(State, MessageData, MessageLength, KdContext);
}

static
KDSTATUS
handle_gdb_set(VOID)
{
    KDSTATUS Status;

    if (strcmp(gdb_input, "QStartNoAckMode") != 0)
        return LOOP_IF_SUCCESS(send_gdb_packet(""));

    /*
     * The OK packet itself still uses acknowledgement mode. Switch only after
     * it has been acknowledged, as required by the remote protocol.
     */
    Status = send_gdb_packet("OK");
    if (Status == KdPacketReceived)
        gdb_no_ack_mode = TRUE;
    return LOOP_IF_SUCCESS(Status);
}

static
BOOLEAN
parse_resume_address(
    _Out_ PBOOLEAN HasAddress,
    _Out_ PULONG64 Address)
{
    const char* Current = &gdb_input[1];
    const char* End = &gdb_input[gdb_input_length];

    *HasAddress = FALSE;

    /* vCont carries no resume address */
    if (gdb_input[0] == 'v')
        return TRUE;

    if (gdb_input[0] == 'C' || gdb_input[0] == 'S')
    {
        ULONG64 Signal;

        if (!parse_hex_value(Current, End, &Signal, &Current) ||
            Signal > 0xff)
        {
            return FALSE;
        }

        if (Current == End)
            return TRUE;
        if (*Current++ != ';')
            return FALSE;
    }

    if (Current == End)
        return TRUE;
    if (!parse_hex_value(Current, End, Address, &Current) || Current != End)
        return FALSE;

    *HasAddress = TRUE;
    return TRUE;
}

/* Push the modified context back to KD, then let the target run */
static
KDSTATUS
set_context_then_continue(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext)
{
    SetContextManipulateHandler(State, MessageData, MessageLength, KdContext);
    KdpManipulateStateHandler = ContinueManipulateStateHandler;
    return KdPacketReceived;
}

/*
 * We stopped on one of our own breakpoint instructions, so the program counter
 * must be moved past it before resuming. Returns whether it was moved.
 */
static
BOOLEAN
step_over_breakpoint(VOID)
{
    DBGKM_EXCEPTION64* Exception = &CurrentStateChange.u.Exception;
    ULONG_PTR ProgramCounter = KdpGetContextPc(&CurrentContext);

    if (CurrentStateChange.NewState != DbgKdExceptionStateChange)
        return FALSE;

    if ((Exception->ExceptionRecord.ExceptionCode == STATUS_BREAKPOINT)
        && ((*(KD_BREAKPOINT_TYPE*)ProgramCounter) == KD_BREAKPOINT_VALUE))
    {
        KdpSetContextPc(&CurrentContext, ProgramCounter + KD_BREAKPOINT_SIZE);
        return TRUE;
    }

    return FALSE;
}

static
KDSTATUS
handle_gdb_c(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext)
{
    BOOLEAN HasAddress;
    BOOLEAN WasSingleStepping;
    ULONG64 Address;

    if (!parse_resume_address(&HasAddress, &Address))
        return LOOP_IF_SUCCESS(send_gdb_packet("E01"));

    WasSingleStepping = (CurrentContext.EFlags & EFLAGS_TF) != 0;
    KdpClearSingleStep(&CurrentContext);
    if (HasAddress)
    {
        KdpSetContextPc(&CurrentContext, (ULONG_PTR)Address);
        return set_context_then_continue(State, MessageData, MessageLength, KdContext);
    }

    if (step_over_breakpoint())
        return set_context_then_continue(State, MessageData, MessageLength, KdContext);

#if defined(_M_IX86) || defined(_M_AMD64)
    /* Unlike single stepping, continuing also steps over a runtime check failure */
    if (CurrentStateChange.NewState == DbgKdExceptionStateChange)
    {
        DBGKM_EXCEPTION64* Exception = &CurrentStateChange.u.Exception;
        ULONG_PTR ProgramCounter = KdpGetContextPc(&CurrentContext);

        if ((Exception->ExceptionRecord.ExceptionCode == STATUS_ASSERTION_FAILURE)
            && ((*(KD_BREAKPOINT_TYPE*)ProgramCounter) == 0xCD)
            && (*((KD_BREAKPOINT_TYPE*)ProgramCounter + 1) == 0x2C))
        {
            /* INT 2C (a.k.a. runtime check failure) */
            KdpSetContextPc(&CurrentContext, ProgramCounter + 2);
            return set_context_then_continue(State, MessageData, MessageLength, KdContext);
        }
    }
#endif

    if (WasSingleStepping)
        return set_context_then_continue(State, MessageData, MessageLength, KdContext);

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
    BOOLEAN HasAddress;
    ULONG64 Address;

    if (!parse_resume_address(&HasAddress, &Address))
        return LOOP_IF_SUCCESS(send_gdb_packet("E01"));

    KDDBGPRINT("Single stepping.\n");
    if (HasAddress)
        KdpSetContextPc(&CurrentContext, (ULONG_PTR)Address);
    else
        step_over_breakpoint();

    /* Set CPU single step mode and continue */
    KdpSetSingleStep(&CurrentContext);
    return set_context_then_continue(State, MessageData, MessageLength, KdContext);
}

static
KDSTATUS
handle_gdb_v(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext)
{
    const char* Current;
    const char* End = &gdb_input[gdb_input_length];
    CHAR ResumeAction = '\0';

    if (strcmp(gdb_input, "vCont?") == 0)
        return LOOP_IF_SUCCESS(send_gdb_packet("vCont;c;s"));

    if (strcmp(gdb_input, "vMustReplyEmpty") == 0)
        return LOOP_IF_SUCCESS(send_gdb_packet(""));

    if (strcmp(gdb_input, "vCtrlC") == 0)
        return LOOP_IF_SUCCESS(send_gdb_packet("OK"));

    if (gdb_input_length < 7 || strncmp(gdb_input, "vCont;", 6) != 0)
        return LOOP_IF_SUCCESS(send_gdb_packet(""));

    Current = &gdb_input[6];
    while (Current != End)
    {
        CHAR Action = *Current++;

        if ((Action != 'c' && Action != 's') ||
            (ResumeAction != '\0' && ResumeAction != Action))
        {
            return LOOP_IF_SUCCESS(send_gdb_packet("E01"));
        }
        ResumeAction = Action;

        if (Current != End && *Current == ':')
        {
            const char* ThreadEnd;
            UINT_PTR Pid;
            UINT_PTR Tid;

            Current++;
            ThreadEnd = Current;
            while (ThreadEnd != End && *ThreadEnd != ';')
                ThreadEnd++;
            if (!parse_gdb_thread_id(Current, ThreadEnd, &Pid, &Tid) ||
                (Tid != 0 && Tid != (UINT_PTR)-1 && find_thread(Pid, Tid) == NULL))
            {
                return LOOP_IF_SUCCESS(send_gdb_packet("E01"));
            }
            gdb_run_tid = Tid;
            Current = ThreadEnd;
        }

        if (Current != End)
        {
            if (*Current++ != ';' || Current == End)
                return LOOP_IF_SUCCESS(send_gdb_packet("E01"));
        }
    }

    if (ResumeAction == 'c')
        return handle_gdb_c(State, MessageData, MessageLength, KdContext);
    if (ResumeAction == 's')
        return handle_gdb_s(State, MessageData, MessageLength, KdContext);
    return LOOP_IF_SUCCESS(send_gdb_packet("E01"));
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
        KDDBGPRINT("KDGDB: Receiving packet.\n");
        Status = gdb_receive_packet(KdContext);
        KDDBGPRINT("KDGDB: Packet \"%s\" received with status %u\n", gdb_input, Status);

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
        case 'C':
            Status = handle_gdb_c(State, MessageData, MessageLength, KdContext);
            break;
        case 'g':
            Status = LOOP_IF_SUCCESS(gdb_send_registers());
            break;
        case 'D':
            Status = handle_gdb_detach(State, MessageData, MessageLength, KdContext);
            break;
        case 'G':
            Status = handle_gdb_write_registers(State, MessageData, MessageLength, KdContext);
            break;
        case 'H':
            Status = LOOP_IF_SUCCESS(handle_gdb_set_thread());
            break;
        case 'm':
            Status = handle_gdb_read_mem(State, MessageData, MessageLength, KdContext);
            break;
        case 'M':
            Status = handle_gdb_write_mem(State, MessageData, MessageLength, KdContext);
            break;
        case 'p':
            Status = LOOP_IF_SUCCESS(gdb_send_register());
            break;
        case 'P':
            Status = handle_gdb_write_register(State, MessageData, MessageLength, KdContext);
            break;
        case 'Q':
            Status = handle_gdb_set();
            break;
        case 'q':
            if (gdb_input_length >= 15 && strncmp(gdb_input, "qSearch:memory:", 15) == 0)
                Status = handle_gdb_search_memory(State, MessageData, MessageLength, KdContext);
            else
                Status = LOOP_IF_SUCCESS(handle_gdb_query());
            break;
        case 's':
        case 'S':
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
        case 'x':
            Status = handle_gdb_read_mem(State, MessageData, MessageLength, KdContext);
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
