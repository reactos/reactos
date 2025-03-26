/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kddll/kdpacket.c
 * PURPOSE:         Base functions for the kernel debugger.
 */

#include "kdgdb.h"

/* LOCALS *********************************************************************/
static
BOOLEAN
FirstSendHandler(
    _In_ ULONG PacketType,
    _In_ PSTRING MessageHeader,
    _In_ PSTRING MessageData);
static BOOLEAN InException = FALSE;

/* GLOBALS ********************************************************************/
DBGKD_GET_VERSION64 KdVersion;
KDDEBUGGER_DATA64* KdDebuggerDataBlock;
LIST_ENTRY* ProcessListHead;
LIST_ENTRY* ModuleListHead;
/* Callbacks used to communicate with KD aside from GDB */
KDP_SEND_HANDLER KdpSendPacketHandler = FirstSendHandler;
KDP_MANIPULATESTATE_HANDLER KdpManipulateStateHandler = NULL;
/* Data describing the current exception */
DBGKD_ANY_WAIT_STATE_CHANGE CurrentStateChange;
CONTEXT CurrentContext;
PEPROCESS TheIdleProcess;
PETHREAD TheIdleThread;

/* PRIVATE FUNCTIONS **********************************************************/

static
BOOLEAN
GetContextSendHandler(
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
        KDDBGPRINT("ERROR: Received wrong packet from KD.\n");
        return FALSE;
    }

    /* Just copy it */
    RtlCopyMemory(&CurrentContext, Context, sizeof(*Context));
    KdpSendPacketHandler = NULL;
    return TRUE;
}

static
KDSTATUS
GetContextManipulateHandler(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext
)
{
    State->ApiNumber = DbgKdGetContextApi;
    State->Processor = CurrentStateChange.Processor;
    State->ReturnStatus = STATUS_SUCCESS;
    State->ProcessorLevel = CurrentStateChange.ProcessorLevel;
    MessageData->Length = 0;

    /* Update the send <-> receive loop handler */
    KdpSendPacketHandler = GetContextSendHandler;
    KdpManipulateStateHandler = NULL;

    return KdPacketReceived;
}

static
BOOLEAN
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
        KDDBGPRINT("BAD BAD BAD not manipulating state for sending context.\n");
        return FALSE;
    }

    KdpSendPacketHandler = NULL;
    return TRUE;
}

KDSTATUS
SetContextManipulateHandler(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext
)
{
    State->ApiNumber = DbgKdSetContextApi;
    State->Processor = CurrentStateChange.Processor;
    State->ReturnStatus = STATUS_SUCCESS;
    State->ProcessorLevel = CurrentStateChange.ProcessorLevel;
    MessageData->Length = sizeof(CurrentContext);

    if (MessageData->MaximumLength < sizeof(CurrentContext))
    {
        KDDBGPRINT("Wrong message length %u.\n", MessageData->MaximumLength);
        while (1);
    }

    RtlCopyMemory(MessageData->Buffer, &CurrentContext, sizeof(CurrentContext));

    /* Update the send <-> receive loop handlers */
    KdpSendPacketHandler = SetContextSendHandler;
    KdpManipulateStateHandler = NULL;

    return KdPacketReceived;
}

static
void
send_kd_state_change(DBGKD_ANY_WAIT_STATE_CHANGE* StateChange)
{
    InException = TRUE;

    switch (StateChange->NewState)
    {
    case DbgKdLoadSymbolsStateChange:
    case DbgKdExceptionStateChange:
    {
        PETHREAD Thread = (PETHREAD)(ULONG_PTR)StateChange->Thread;
        /* Save current state for later GDB queries */
        CurrentStateChange = *StateChange;
        KDDBGPRINT("Exception 0x%08x in thread p%p.%p.\n",
            StateChange->u.Exception.ExceptionRecord.ExceptionCode,
            PsGetThreadProcessId(Thread),
            PsGetThreadId(Thread));
        /* Set the current debugged process/thread accordingly */
        gdb_dbg_tid = handle_to_gdb_tid(PsGetThreadId(Thread));
#if MONOPROCESS
        gdb_dbg_pid = 0;
#else
        gdb_dbg_pid = handle_to_gdb_pid(PsGetThreadProcessId(Thread));
#endif
        gdb_send_exception();
        /* Next receive call will ask for the context */
        KdpManipulateStateHandler = GetContextManipulateHandler;
        break;
    }
    default:
        KDDBGPRINT("Unknown StateChange %u.\n", StateChange->NewState);
        while (1);
    }
}

static
void
send_kd_debug_io(
    _In_ DBGKD_DEBUG_IO* DebugIO,
    _In_ PSTRING String)
{
    if (InException)
        return;

    switch (DebugIO->ApiNumber)
    {
    case DbgKdPrintStringApi:
    case DbgKdGetStringApi:
        gdb_send_debug_io(String, TRUE);
        break;
    default:
        KDDBGPRINT("Unknown ApiNumber %u.\n", DebugIO->ApiNumber);
        while (1);
    }
}

static
void
send_kd_state_manipulate(
    _In_ DBGKD_MANIPULATE_STATE64* State,
    _In_ PSTRING MessageData)
{
    switch (State->ApiNumber)
    {
#if 0
    case DbgKdGetContextApi:
        /* This is an answer to a 'g' GDB request */
        gdb_send_registers((CONTEXT*)MessageData->Buffer);
        return;
#endif
    default:
        KDDBGPRINT("Unknown ApiNumber %u.\n", State->ApiNumber);
        while (1);
    }
}

KDSTATUS
ContinueManipulateStateHandler(
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
    /* We're not handling an exception anymore */
    InException = FALSE;

    return KdPacketReceived;
}

static
BOOLEAN
GetVersionSendHandler(
    _In_ ULONG PacketType,
    _In_ PSTRING MessageHeader,
    _In_ PSTRING MessageData)
{
    DBGKD_MANIPULATE_STATE64* State = (DBGKD_MANIPULATE_STATE64*)MessageHeader->Buffer;
    PLIST_ENTRY DebuggerDataList;

    /* Confirm that all went well */
    if ((PacketType != PACKET_TYPE_KD_STATE_MANIPULATE)
            || (State->ApiNumber != DbgKdGetVersionApi)
            || !NT_SUCCESS(State->ReturnStatus))
    {
        KDDBGPRINT("Wrong packet received after asking for data.\n");
        return FALSE;
    }

    /* Copy the relevant data */
    RtlCopyMemory(&KdVersion, &State->u.GetVersion64, sizeof(KdVersion));
    DebuggerDataList = *(PLIST_ENTRY*)&KdVersion.DebuggerDataList;
    KdDebuggerDataBlock = CONTAINING_RECORD(DebuggerDataList->Flink, KDDEBUGGER_DATA64, Header.List);
    ProcessListHead = *(PLIST_ENTRY*)&KdDebuggerDataBlock->PsActiveProcessHead;
    ModuleListHead = *(PLIST_ENTRY*)&KdDebuggerDataBlock->PsLoadedModuleList;

    /* Now we can get the context for the current state */
    KdpSendPacketHandler = NULL;
    KdpManipulateStateHandler = GetContextManipulateHandler;
    return TRUE;
}

static
KDSTATUS
GetVersionManipulateStateHandler(
    _Out_ DBGKD_MANIPULATE_STATE64* State,
    _Out_ PSTRING MessageData,
    _Out_ PULONG MessageLength,
    _Inout_ PKD_CONTEXT KdContext)
{
    /* Ask for the version data */
    State->ApiNumber = DbgKdGetVersionApi;
    State->Processor = CurrentStateChange.Processor;
    State->ProcessorLevel = CurrentStateChange.ProcessorLevel;

    /* The next send call will serve this query */
    KdpSendPacketHandler = GetVersionSendHandler;
    KdpManipulateStateHandler = NULL;

    return KdPacketReceived;
}

static
BOOLEAN
FirstSendHandler(
    _In_ ULONG PacketType,
    _In_ PSTRING MessageHeader,
    _In_ PSTRING MessageData)
{
    DBGKD_ANY_WAIT_STATE_CHANGE* StateChange = (DBGKD_ANY_WAIT_STATE_CHANGE*)MessageHeader->Buffer;
    PETHREAD Thread;

    if (PacketType != PACKET_TYPE_KD_STATE_CHANGE64)
    {
        KDDBGPRINT("First KD packet is not a state change!\n");
        return FALSE;
    }

    KDDBGPRINT("KDGDB: START!\n");

    Thread = (PETHREAD)(ULONG_PTR)StateChange->Thread;

    /* Set up the current state */
    CurrentStateChange = *StateChange;
    gdb_dbg_tid = handle_to_gdb_tid(PsGetThreadId(Thread));
#if MONOPROCESS
    gdb_dbg_pid = 0;
#else
    gdb_dbg_pid = handle_to_gdb_pid(PsGetThreadProcessId(Thread));
#endif
    /* This is the idle process. Save it! */
    TheIdleThread = Thread;
    TheIdleProcess = (PEPROCESS)Thread->Tcb.ApcState.Process;

    KDDBGPRINT("Pid Tid of the first message: %" PRIxPTR", %" PRIxPTR ".\n", gdb_dbg_pid, gdb_dbg_tid);

    /* The next receive call will be asking for the version data */
    KdpSendPacketHandler = NULL;
    KdpManipulateStateHandler = GetVersionManipulateStateHandler;
    return TRUE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/******************************************************************************
 * \name KdReceivePacket
 * \brief Receive a packet from the KD port.
 * \param [in] PacketType Describes the type of the packet to receive.
 *        This can be one of the PACKET_TYPE_ constants.
 * \param [out] MessageHeader Pointer to a STRING structure for the header.
 * \param [out] MessageData Pointer to a STRING structure for the data.
 * \return KdPacketReceived if successful, KdPacketTimedOut if the receive
 *         timed out, KdPacketNeedsResend to signal that the last packet needs
 *         to be sent again.
 * \note If PacketType is PACKET_TYPE_KD_POLL_BREAKIN, the function doesn't
 *       wait for any data, but returns KdPacketTimedOut instantly if no breakin
 *       packet byte is received.
 * \sa http://www.nynaeve.net/?p=169
 */
KDSTATUS
NTAPI
KdReceivePacket(
    _In_ ULONG PacketType,
    _Out_ PSTRING MessageHeader,
    _Out_ PSTRING MessageData,
    _Out_ PULONG DataLength,
    _Inout_ PKD_CONTEXT KdContext)
{
    KDDBGPRINT("KdReceivePacket --> ");

    if (PacketType == PACKET_TYPE_KD_POLL_BREAKIN)
    {
        static BOOLEAN firstTime = TRUE;
        KDDBGPRINT("Polling break in.\n");
        if (firstTime)
        {
            /* Force debug break on init */
            firstTime = FALSE;
            return KdPacketReceived;
        }

        return KdpPollBreakIn();
    }

    if (PacketType == PACKET_TYPE_KD_DEBUG_IO)
    {
        static BOOLEAN ignore = 0;
        KDDBGPRINT("Debug prompt.\n");
        /* HACK ! Debug prompt asks for break or ignore. First break, then ignore. */
        MessageData->Length = 1;
        MessageData->Buffer[0] = ignore ? 'i' : 'b';
        ignore = !ignore;
        return KdPacketReceived;
    }

    if (PacketType == PACKET_TYPE_KD_STATE_MANIPULATE)
    {
        DBGKD_MANIPULATE_STATE64* State = (DBGKD_MANIPULATE_STATE64*)MessageHeader->Buffer;

        KDDBGPRINT("State manipulation: ");

        /* Maybe we are in a send<->receive loop that GDB doesn't need to know about */
        if (KdpManipulateStateHandler != NULL)
        {
            KDDBGPRINT("We have a manipulate state handler.\n");
            return KdpManipulateStateHandler(State, MessageData, DataLength, KdContext);
        }

        /* Receive data from GDB  and interpret it */
        KDDBGPRINT("Receiving data from GDB.\n");
        return gdb_receive_and_interpret_packet(State, MessageData, DataLength, KdContext);
    }

    /* What should we do ? */
    while (1);
    return KdPacketNeedsResend;
}

VOID
NTAPI
KdSendPacket(
    IN ULONG PacketType,
    IN PSTRING MessageHeader,
    IN PSTRING MessageData,
    IN OUT PKD_CONTEXT KdContext)
{
    /* Override if we have some debug print from KD. */
    if (PacketType == PACKET_TYPE_KD_DEBUG_IO)
    {
        send_kd_debug_io((DBGKD_DEBUG_IO*)MessageHeader->Buffer, MessageData);
        return;
    }

    /* Maybe we are in a send <-> receive loop that GDB doesn't need to know about */
    if (KdpSendPacketHandler
        && KdpSendPacketHandler(PacketType, MessageHeader, MessageData))
    {
        return;
    }

    switch (PacketType)
    {
    case PACKET_TYPE_KD_STATE_CHANGE64:
        send_kd_state_change((DBGKD_ANY_WAIT_STATE_CHANGE*)MessageHeader->Buffer);
        return;
    case PACKET_TYPE_KD_DEBUG_IO:
        send_kd_debug_io((DBGKD_DEBUG_IO*)MessageHeader->Buffer, MessageData);
        break;
    case PACKET_TYPE_KD_STATE_MANIPULATE:
        send_kd_state_manipulate((DBGKD_MANIPULATE_STATE64*)MessageHeader->Buffer, MessageData);
        break;
    default:
        KDDBGPRINT("Unknown packet type %u.\n", PacketType);
        while (1);
    }
}

/* EOF */
