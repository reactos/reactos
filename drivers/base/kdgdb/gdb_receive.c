/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kddll/gdb_receive.c
 * PURPOSE:         Base functions for the kernel debugger.
 */

#include "kdgdb.h"

/* GLOBALS ********************************************************************/
CHAR gdb_input[0x1000];

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

    do
    {
        Status = KdpReceiveByte(&Byte);
        if (Status != KdPacketReceived)
            return Status;
    } while (Byte != '$');

get_packet:
    CheckSum = 0;
    ByteBuffer = (UCHAR*)gdb_input;

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
        *ByteBuffer++ = Byte;
    }

    /* Get Check sum (two bytes) */
    Status = KdpReceiveByte(&Byte);
    if (Status != KdPacketReceived)
        goto end;
    ReceivedCheckSum = hex_value(Byte) << 4;

    Status = KdpReceiveByte(&Byte);
    if (Status != KdPacketReceived)
        goto end;
    ReceivedCheckSum += hex_value(Byte);

end:
    if (ReceivedCheckSum != CheckSum)
    {
        /* Do not acknowledge to GDB */
        KDDBGPRINT("Check sums don't match!");
        KdpSendByte('-');
        return KdPacketNeedsResend;
    }

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

    /* Acknowledge */
    KdpSendByte('+');

    return KdPacketReceived;
}
