/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kddll/gdb_input.c
 * PURPOSE:         Base functions for the kernel debugger.
 */

#include "kdgdb.h"

#include <pstypes.h>

/* LOCALS *********************************************************************/
static HANDLE gdb_run_thread;
static HANDLE gdb_dbg_process;
HANDLE gdb_dbg_thread;
CONTEXT CurrentContext;
/* Keep track of where we are for qfThreadInfo/qsThreadInfo */
static LIST_ENTRY* CurrentProcessEntry;
static LIST_ENTRY* CurrentThreadEntry;

/* PRIVATE FUNCTIONS **********************************************************/
static
HANDLE
hex_to_thread(char* buffer)
{
    ULONG_PTR ret = 0;
    char hex;
    while (*buffer)
    {
        hex = hex_value(*buffer++);
        if (hex < 0)
            return (HANDLE)ret;
        ret <<= 4;
        ret += hex;
    }
    return (HANDLE)ret;
}

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
            gdb_run_thread = (HANDLE)-1;
        else
            gdb_run_thread = hex_to_thread(&gdb_input[2]);
        send_gdb_packet("OK");
        break;
    case 'g':
        if (strncmp(&gdb_input[2], "p-1", 3) == 0)
        {
            gdb_dbg_process = (HANDLE)-1;
            gdb_dbg_thread = (HANDLE)-1;
        }
        else
        {
            char* ptr = strstr(gdb_input, ".") + 1;
            gdb_dbg_process = hex_to_thread(&gdb_input[3]);
            if (strncmp(ptr, "-1", 2) == 0)
                gdb_dbg_thread = (HANDLE)-1;
            else
                gdb_dbg_thread = hex_to_thread(ptr);
        }
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
    char* ptr = strstr(gdb_input, ".") + 1;
    CLIENT_ID ClientId;
    PETHREAD Thread;
    NTSTATUS Status;

    ClientId.UniqueProcess = hex_to_thread(&gdb_input[2]);
    ClientId.UniqueThread = hex_to_thread(ptr);

    Status = PsLookupProcessThreadByCid(&ClientId, NULL, &Thread);

    if (!NT_SUCCESS(Status))
    {
        /* Thread doesn't exist */
        send_gdb_packet("E03");
        return;
    }

    /* It's OK */
    ObDereferenceObject(Thread);
    send_gdb_packet("OK");
}

/* q* packets */
static
void
handle_gdb_query(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext)
{
    if (strncmp(gdb_input, "qSupported:", 11) == 0)
    {
        send_gdb_packet("PacketSize=4096;multiprocess+;");
        return;
    }

    if (strncmp(gdb_input, "qAttached", 9) == 0)
    {
        /* Say yes: the remote server didn't create the process, ReactOS did! */
        send_gdb_packet("0");
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
        sprintf(gdb_out, "QC:p%p.%p;",
            PsGetThreadProcessId((PETHREAD)(ULONG_PTR)CurrentStateChange.Thread),
            PsGetThreadId((PETHREAD)(ULONG_PTR)CurrentStateChange.Thread));
        send_gdb_packet(gdb_out);
        return;
    }

    if ((strncmp(gdb_input, "qfThreadInfo", 12) == 0)
            || (strncmp(gdb_input, "qsThreadInfo", 12) == 0))
    {
        LIST_ENTRY* ProcessListHead = (LIST_ENTRY*)KdDebuggerDataBlock->PsActiveProcessHead.Pointer;
        BOOLEAN FirstThread = TRUE;
        PEPROCESS Process;
        PETHREAD Thread;
        char gdb_out[1024];
        char* ptr;
        BOOLEAN Resuming = strncmp(gdb_input, "qsThreadInfo", 12) == 0;

        /* Maybe this was not initialized yet */
        if (!ProcessListHead->Flink)
        {
            char gdb_out[64];

            if (Resuming)
            {
                /* there is only one thread to tell about */
                send_gdb_packet("l");
                return;
            }
            /* Just tell GDB about the current thread */
            sprintf(gdb_out, "mp%p.%p", PsGetCurrentProcessId(), PsGetCurrentThreadId());
            send_gdb_packet(gdb_out);
            /* GDB can ask anything at this point, it isn't necessarily a qsThreadInfo packet */
            gdb_receive_packet(KdContext);
            gdb_interpret_input(State, MessageData, MessageLength, KdContext);
            return;
        }

        if (Resuming)
        {
            if (CurrentThreadEntry == NULL)
                CurrentProcessEntry = CurrentProcessEntry->Flink;
        }
        else
            CurrentProcessEntry = ProcessListHead->Flink;

        if (CurrentProcessEntry == ProcessListHead)
        {
            /* We're done */
            send_gdb_packet("l");
            return;
        }

        Process = CONTAINING_RECORD(CurrentProcessEntry, EPROCESS, ActiveProcessLinks);

        if (Resuming && CurrentThreadEntry != NULL)
            CurrentThreadEntry = CurrentThreadEntry->Flink;
        else
            CurrentThreadEntry = Process->ThreadListHead.Flink;

        ptr = gdb_out;

        *ptr++ = 'm';
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

            ptr += _snprintf(ptr, 1024 - (ptr - gdb_out),
                "p%p.%p", PsGetProcessId(Process), PsGetThreadId(Thread));
            if (ptr > (gdb_out + 1024))
            {
                /* send what we got */
                send_gdb_packet(gdb_out);
                /* GDB can ask anything at this point, it isn't necessarily a qsThreadInfo packet */
                gdb_receive_packet(KdContext);
                gdb_interpret_input(State, MessageData, MessageLength, KdContext);
                return;
            }
        }

        /* send the list for this process */
        send_gdb_packet(gdb_out);
        CurrentThreadEntry = NULL;
        /* GDB can ask anything at this point, it isn't necessarily a qsThreadInfo packet */
        gdb_receive_packet(KdContext);
        gdb_interpret_input(State, MessageData, MessageLength, KdContext);
        return;
    }

    KDDBGPRINT("KDGDB: Unknown query: %s\n", gdb_input);
    send_gdb_packet("");
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
}

static
KDSTATUS
handle_gdb_read_mem(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength)
{
    State->ApiNumber = DbgKdReadVirtualMemoryApi;
    State->ReturnStatus = STATUS_SUCCESS; /* ? */
    State->Processor = CurrentStateChange.Processor;
    State->ProcessorLevel = CurrentStateChange.ProcessorLevel;
    if (MessageData)
        MessageData->Length = 0;
    *MessageLength = 0;

    State->u.ReadMemory.TargetBaseAddress = hex_to_address(&gdb_input[1]);
    State->u.ReadMemory.TransferCount = hex_to_address(strstr(&gdb_input[1], ",") + 1);

    /* KD will reply with KdSendPacket. Catch it */
    KdpSendPacketHandler = ReadMemorySendHandler;

    return KdPacketReceived;
}

static
VOID
GetCurrentContextSendHandler(
    _In_ ULONG PacketType,
    _In_ PSTRING MessageHeader,
    _In_ PSTRING MessageData
)
{
    DBGKD_MANIPULATE_STATE64* State = (DBGKD_MANIPULATE_STATE64*)MessageHeader->Buffer;
    const CONTEXT* Context = (const CONTEXT*)MessageData->Buffer;

    if ((PacketType != PACKET_TYPE_KD_STATE_MANIPULATE)
            || (State->ApiNumber != DbgKdGetContextApi)
            || (MessageData->Length < sizeof(*Context)))
    {
        /* Should we bugcheck ? */
        while (1);
    }

    /* Just copy it */
    RtlCopyMemory(&CurrentContext, Context, sizeof(*Context));
    KdpSendPacketHandler = NULL;
}

static
VOID
GetCurrentContext(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext,
    _In_opt_ KDP_MANIPULATESTATE_HANDLER ManipulateStateHandler
)
{
    State->ApiNumber = DbgKdGetContextApi;
    State->Processor = CurrentStateChange.Processor;
    State->ReturnStatus = STATUS_SUCCESS;
    State->ProcessorLevel = CurrentStateChange.ProcessorLevel;
    MessageData->Length = 0;

    /* Update the send <-> receive loop handler */
    KdpSendPacketHandler = GetCurrentContextSendHandler;
    KdpManipulateStateHandler = ManipulateStateHandler;
}

static
VOID
SetContextSendHandler(
    _In_ ULONG PacketType,
    _In_ PSTRING MessageHeader,
    _In_ PSTRING MessageData
)
{
    DBGKD_MANIPULATE_STATE64* State = (DBGKD_MANIPULATE_STATE64*)MessageHeader->Buffer;

    /* We just confirm that all went well */
    if ((PacketType != PACKET_TYPE_KD_STATE_MANIPULATE)
            || (State->ApiNumber != DbgKdSetContextApi)
            || (State->ReturnStatus != STATUS_SUCCESS))
    {
        /* Should we bugcheck ? */
        while (1);
    }

    KdpSendPacketHandler = NULL;
}

static
KDSTATUS
SetContext(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext,
    _In_opt_ KDP_MANIPULATESTATE_HANDLER ManipulateStateHandler
)
{
    State->ApiNumber = DbgKdSetContextApi;
    State->Processor = CurrentStateChange.Processor;
    State->ReturnStatus = STATUS_SUCCESS;
    State->ProcessorLevel = CurrentStateChange.ProcessorLevel;
    MessageData->Length = sizeof(CurrentContext);

    if (MessageData->MaximumLength < sizeof(CurrentContext))
    {
        while (1);
    }

    RtlCopyMemory(MessageData->Buffer, &CurrentContext, sizeof(CurrentContext));

    /* Update the send <-> receive loop handlers */
    KdpSendPacketHandler = SetContextSendHandler;
    KdpManipulateStateHandler = ManipulateStateHandler;

    return KdPacketReceived;
}

static
KDSTATUS
SendContinue(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext
)
{
    /* Let's go on */
    State->ApiNumber = DbgKdContinueApi;
    State->ReturnStatus = STATUS_SUCCESS; /* ? */
    State->Processor = CurrentStateChange.Processor;
    State->ProcessorLevel = CurrentStateChange.ProcessorLevel;
    if (MessageData)
        MessageData->Length = 0;
    *MessageLength = 0;
    State->u.Continue.ContinueStatus = STATUS_SUCCESS;

    /* We definitely are at the end of the send <-> receive loop, if any */
    KdpSendPacketHandler = NULL;
    KdpManipulateStateHandler = NULL;

    /* Tell GDB we are fine */
    send_gdb_packet("OK");
    return KdPacketReceived;
}

static
KDSTATUS
UpdateProgramCounterSendContinue(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext)
{
    ULONG_PTR ProgramCounter;

    /* So we must get past the breakpoint instruction */
    ProgramCounter = KdpGetContextPc(&CurrentContext);
    KdpSetContextPc(&CurrentContext, ProgramCounter + KD_BREAKPOINT_SIZE);

    /* Set the context and continue */
    SetContext(State, MessageData, MessageLength, KdContext, SendContinue);
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
            KDSTATUS Status;
            /* Report what we support */
            send_gdb_packet("vCont;c;C;s;S");
            Status = gdb_receive_packet(KdContext);
            if (Status != KdPacketReceived)
                return Status;
            return gdb_interpret_input(State, MessageData, MessageLength, KdContext);
        }

        if (strcmp(gdb_input, "vCont;c") == 0)
        {
            DBGKM_EXCEPTION64* Exception = NULL;

            if (CurrentStateChange.NewState == DbgKdExceptionStateChange)
                Exception = &CurrentStateChange.u.Exception;

            /* See if we should update the program counter (unlike windbg, gdb doesn't do it for us) */
            if (Exception && (Exception->ExceptionRecord.ExceptionCode == STATUS_BREAKPOINT)
                    && (Exception->ExceptionRecord.ExceptionInformation[0] == 0))
            {
                /* So we get the context, update it and send it back */
                GetCurrentContext(State, MessageData, MessageLength, KdContext, UpdateProgramCounterSendContinue);
                return KdPacketReceived;
            }

            return SendContinue(State, MessageData, MessageLength, KdContext);
        }
    }

    return KdPacketReceived;
}

/* GLOBAL FUNCTIONS ***********************************************************/
KDSTATUS
gdb_interpret_input(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext)
{
    KDSTATUS Status;
    switch (gdb_input[0])
    {
    case '?':
        /* Send the Status */
        gdb_send_exception();
        break;
    case 'g':
        gdb_send_registers();
        break;
    case 'H':
        handle_gdb_set_thread();
        break;
    case 'm':
        return handle_gdb_read_mem(State, MessageData, MessageLength);
    case 'q':
        handle_gdb_query(State, MessageData, MessageLength, KdContext);
        break;
    case 'T':
        handle_gdb_thread_alive();
        break;
    case 'v':
        return handle_gdb_v(State, MessageData, MessageLength, KdContext);
    default:
        /* We don't know how to handle this request. Maybe this is something for KD */
        State->ReturnStatus = STATUS_NOT_SUPPORTED;
        return KdPacketReceived;
    }
    /* Get the answer from GDB */
    Status = gdb_receive_packet(KdContext);
    if (Status != KdPacketReceived)
        return Status;
    /* Try interpreting this new packet */
    return gdb_interpret_input(State, MessageData, MessageLength, KdContext);
}
