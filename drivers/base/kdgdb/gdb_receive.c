/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kddll/gdb_receive.c
 * PURPOSE:         Base functions for the kernel debugger.
 */

#include "kdgdb.h"

/* GLOBALS ********************************************************************/
CHAR gdb_input[GDB_PACKET_MAX_SIZE + 1];
ULONG gdb_input_length;

/* GLOBAL FUNCTIONS ***********************************************************/
char
hex_value(char ch)
{
    if ((ch >= '0') && (ch <= '9'))
        return (ch - '0');

    if ((ch >= 'a') && (ch <= 'f'))
        return (ch - 'a' + 10);

    if ((ch >= 'A') && (ch <= 'F'))
        return (ch - 'A' + 10);

    return -1;
}

KDSTATUS
NTAPI
gdb_receive_packet(_Inout_ PKD_CONTEXT KdContext)
{
    UCHAR* ByteBuffer;
    UCHAR Byte;
    KDSTATUS Status;
    CHAR CheckSum, ReceivedCheckSum;
    BOOLEAN PacketTooLarge;
    char HighNibble, LowNibble;

wait_for_packet:
    do
    {
        Status = KdpReceiveByte(&Byte);
        if (Status != KdPacketReceived)
            return Status;
    } while (Byte != '$');

get_packet:
    CheckSum = 0;
    ByteBuffer = (UCHAR*)gdb_input;
    PacketTooLarge = FALSE;

    while (TRUE)
    {
        /* Try to get a byte from the port */
        Status = KdpReceiveByte(&Byte);
        if (Status != KdPacketReceived)
            return Status;

        if (Byte == '#')
        {
            *ByteBuffer = '\0';
            break;
        }
        CheckSum += (CHAR)Byte;

        /* See if we should escape */
        if (Byte == 0x7d)
        {
            Status = KdpReceiveByte(&Byte);
            if (Status != KdPacketReceived)
                return Status;
            CheckSum += (CHAR)Byte;
            Byte ^= 0x20;
        }

        if (ByteBuffer < (UCHAR*)&gdb_input[GDB_PACKET_MAX_SIZE])
            *ByteBuffer++ = Byte;
        else
            PacketTooLarge = TRUE;
    }

    /* Get Check sum (two bytes) */
    Status = KdpReceiveByte(&Byte);
    if (Status != KdPacketReceived)
        return Status;
    HighNibble = hex_value(Byte);

    Status = KdpReceiveByte(&Byte);
    if (Status != KdPacketReceived)
        return Status;
    LowNibble = hex_value(Byte);

    if ((HighNibble < 0) || (LowNibble < 0))
    {
        if (gdb_no_ack_mode)
            goto wait_for_packet;
        KdpSendByte('-');
        return KdPacketNeedsResend;
    }

    ReceivedCheckSum = (HighNibble << 4) | LowNibble;
    if (PacketTooLarge || (ReceivedCheckSum != CheckSum))
    {
        KDDBGPRINT(PacketTooLarge ? "Packet exceeds advertised size!" : "Checksums don't match!");
        if (gdb_no_ack_mode)
            goto wait_for_packet;
        KdpSendByte('-');
        return KdPacketNeedsResend;
    }

    gdb_input_length = (ULONG)(ByteBuffer - (UCHAR*)gdb_input);
    *ByteBuffer = '\0';

    /* Ensure there is nothing left in the pipe */
    while (KdpPollByte(&Byte) == KdPacketReceived)
    {
        switch (Byte)
        {
            case '$':
                KDDBGPRINT("Received new packet just after %s.\n", gdb_input);
                goto get_packet;
            case 0x03:
                KdContext->KdpControlCPending = TRUE;
                break;
        }
    }

    if (!gdb_no_ack_mode)
        KdpSendByte('+');

    return KdPacketReceived;
}
