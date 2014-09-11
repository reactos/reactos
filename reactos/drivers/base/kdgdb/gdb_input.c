/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kddll/gdb_input.c
 * PURPOSE:         Base functions for the kernel debugger.
 */

#include "kdgdb.h"

/* LOCALS *********************************************************************/
static HANDLE gdb_run_thread;
static HANDLE gdb_dbg_process;
HANDLE gdb_dbg_thread;

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
handle_gdb_query(void)
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

    if (strncmp(gdb_input, "qTStatus", 8) == 0)
    {
        /* We don't support tracepoints. */
        send_gdb_packet("T0");
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
            /* Let's go on */
            State->ApiNumber = DbgKdContinueApi;
            State->ReturnStatus = STATUS_SUCCESS; /* ? */
            State->Processor = CurrentStateChange.Processor;
            State->ProcessorLevel = CurrentStateChange.ProcessorLevel;
            if (MessageData)
                MessageData->Length = 0;
            *MessageLength = 0;
            State->u.Continue.ContinueStatus = STATUS_SUCCESS;
            /* Tell GDB we are fine */
            send_gdb_packet("OK");
            return KdPacketReceived;
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
        handle_gdb_query();
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
