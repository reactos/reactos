/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kddll/kdpacket.c
 * PURPOSE:         Base functions for the kernel debugger.
 */

#include "kdgdb.h"

/* GLOBALS ********************************************************************/

DBGKD_ANY_WAIT_STATE_CHANGE CurrentStateChange;
DBGKD_GET_VERSION64 KdVersion;
KDDEBUGGER_DATA64* KdDebuggerDataBlock;

/* LOCALS *********************************************************************/
static BOOLEAN FakeNextManipulatePacket = FALSE;
static DBGKD_MANIPULATE_STATE64 FakeManipulateState = {0};

/* PRIVATE FUNCTIONS **********************************************************/
static
void
send_kd_state_change(DBGKD_ANY_WAIT_STATE_CHANGE* StateChange)
{
    static BOOLEAN first = TRUE;

    /* Save current state for later GDB queries */
    CurrentStateChange = *StateChange;

    if (first)
    {
        /*
         * This is the first packet we receive.
         * We take this as an opportunity to connect with GDB and to
         * get the KD version block
         */
        FakeNextManipulatePacket = TRUE;
        FakeManipulateState.ApiNumber = DbgKdGetVersionApi;
        FakeManipulateState.Processor = StateChange->Processor;
        FakeManipulateState.ProcessorLevel = StateChange->ProcessorLevel;
        FakeManipulateState.ReturnStatus = STATUS_SUCCESS;

        first = FALSE;
        return;
    }

    switch (StateChange->NewState)
    {
    case DbgKdLoadSymbolsStateChange:
    {
        /* We don't care about symbols loading */
        FakeNextManipulatePacket = TRUE;
        FakeManipulateState.ApiNumber = DbgKdContinueApi;
        FakeManipulateState.Processor = StateChange->Processor;
        FakeManipulateState.ProcessorLevel = StateChange->ProcessorLevel;
        FakeManipulateState.ReturnStatus = STATUS_SUCCESS;
        FakeManipulateState.u.Continue.ContinueStatus = STATUS_SUCCESS;
        break;
    }
    case DbgKdExceptionStateChange:
        gdb_send_exception();
        break;
    default:
        /* FIXME */
        while (1);
    }
}

static
void
send_kd_debug_io(
    _In_ DBGKD_DEBUG_IO* DebugIO,
    _In_ PSTRING String)
{
    switch (DebugIO->ApiNumber)
    {
    case DbgKdPrintStringApi:
        gdb_send_debug_io(String);
        break;
    default:
        /* FIXME */
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
    case DbgKdReadVirtualMemoryApi:
        /* Answer to 'm' GDB request */
        send_gdb_memory(MessageData->Buffer, State->u.ReadMemory.ActualBytesRead);
        break;
    case DbgKdGetVersionApi:
    {
        LIST_ENTRY* DebuggerDataList;
        /* Simply get a copy */
        RtlCopyMemory(&KdVersion, &State->u.GetVersion64, sizeof(KdVersion));
        DebuggerDataList = (LIST_ENTRY*)(ULONG_PTR)KdVersion.DebuggerDataList;
        KdDebuggerDataBlock = CONTAINING_RECORD(DebuggerDataList->Flink, KDDEBUGGER_DATA64, Header.List);
        return;
    }
    default:
        /* FIXME */
        while (1);
    }
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
    KDSTATUS Status;
    DBGKD_MANIPULATE_STATE64* State;

    /* Special handling for breakin packet */
    if (PacketType == PACKET_TYPE_KD_POLL_BREAKIN)
    {
        return KdpPollBreakIn();
    }

    if (PacketType != PACKET_TYPE_KD_STATE_MANIPULATE)
    {
        /* What should we do ? */
        while (1);
    }

    State = (DBGKD_MANIPULATE_STATE64*)MessageHeader->Buffer;

    if (FakeNextManipulatePacket)
    {
        FakeNextManipulatePacket = FALSE;
        *State = FakeManipulateState;
        return KdPacketReceived;
    }

    /* Receive data from GDB */
    Status = gdb_receive_packet(KdContext);
    if (Status != KdPacketReceived)
        return Status;

    /* Interpret it */
    return gdb_interpret_input(State, MessageData, DataLength, KdContext);
}

VOID
NTAPI
KdSendPacket(
    IN ULONG PacketType,
    IN PSTRING MessageHeader,
    IN PSTRING MessageData,
    IN OUT PKD_CONTEXT KdContext)
{
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
        /* FIXME */
        while (1);
    }
}

/* EOF */
