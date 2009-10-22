/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kddll/kddll.c
 * PURPOSE:         Base functions for the kernel debugger.
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@ewactos.org)
 */

//#define KDDEBUG /* uncomment to enable debugging this dll */
#include "kddll.h"

/* GLOBALS ********************************************************************/

PFNDBGPRNT KdpDbgPrint = NULL;
ULONG CurrentPacketId = INITIAL_PACKET_ID | SYNC_PACKET_ID;


/* PRIVATE FUNCTIONS **********************************************************/

/******************************************************************************
 * \name KdpCalculateChecksum
 * \brief Calculates the checksum for the packet data.
 * \param Buffer Pointer to the packet data.
 * \param Length Length of data in bytes.
 * \return The calculated checksum.
 * \sa http://www.vista-xp.co.uk/forums/technical-reference-library/2540-basics-debugging.html
 */
ULONG
NTAPI
KdpCalculateChecksum(
    IN PVOID Buffer,
    IN ULONG Length)
{
    ULONG i, Checksum = 0;

    for (i = 0; i < Length; i++)
    {
        Checksum += ((PUCHAR)Buffer)[i];
    }

    return Checksum;
}

VOID
NTAPI
KdpSendControlPacket(
    IN USHORT PacketType,
    IN ULONG PacketId OPTIONAL)
{
    KD_PACKET Packet;

    Packet.PacketLeader = CONTROL_PACKET_LEADER;
    Packet.PacketId = PacketId;
    Packet.ByteCount = 0;
    Packet.Checksum = 0;
    Packet.PacketType = PacketType;

    KdpSendBuffer(&Packet, sizeof(KD_PACKET));
}


/* PUBLIC FUNCTIONS ***********************************************************/

NTSTATUS
NTAPI
KdD0Transition(VOID)
{
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KdD3Transition(VOID)
{
    return STATUS_SUCCESS;
}


/******************************************************************************
 * \name KdDebuggerInitialize1
 * \brief Phase 1 initialization.
 * \param [opt] LoaderBlock Pointer to the Loader parameter block. Can be NULL.
 * \return Status
 */
NTSTATUS
NTAPI
KdDebuggerInitialize1(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock OPTIONAL)
{
    // HACK: misuse this function to get a pointer to FrLdrDbgPrint
    KdpDbgPrint = (PVOID)LoaderBlock;
    KDDBGPRINT("KdDebuggerInitialize1\n");

    return STATUS_NOT_IMPLEMENTED;
}


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
KDP_STATUS
NTAPI
KdReceivePacket(
    IN ULONG PacketType,
    OUT PSTRING MessageHeader,
    OUT PSTRING MessageData,
    OUT PULONG DataLength,
    IN OUT PKD_CONTEXT KdContext)
{
    UCHAR Byte = 0;
    KDP_STATUS KdStatus;
    KD_PACKET Packet;
    ULONG Checksum;

    /* Special handling for breakin packet */
    if(PacketType == PACKET_TYPE_KD_POLL_BREAKIN)
    {
        return KdpPollBreakIn();
    }

    for (;;)
    {
        /* Step 1 - Read PacketLeader */
        KdStatus = KdpReceivePacketLeader(&Packet.PacketLeader);
        if (KdStatus != KDP_PACKET_RECEIVED)
        {
            /* Check if we got a breakin  */
            if (KdStatus == KDP_PACKET_RESEND)
            {
                KdContext->BreakInRequested = TRUE;
            }
            return KdStatus;
        }

        /* Step 2 - Read PacketType */
        KdStatus = KdpReceiveBuffer(&Packet.PacketType, sizeof(USHORT));
        if (KdStatus != KDP_PACKET_RECEIVED)
        {
            /* Didn't receive a PacketType or PacketType is bad. Start over. */
            continue;
        }

        /* Step 3 - Read ByteCount */
        KdStatus = KdpReceiveBuffer(&Packet.ByteCount, sizeof(USHORT));
        if (KdStatus != KDP_PACKET_RECEIVED || Packet.ByteCount > PACKET_MAX_SIZE)
        {
            /* Didn't receive ByteCount or it's too big. Start over. */
            continue;
        }

        /* Step 4 - Read PacketId */
        KdStatus = KdpReceiveBuffer(&Packet.PacketId, sizeof(ULONG));
        if (KdStatus != KDP_PACKET_RECEIVED)
        {
            /* Didn't receive PacketId. Start over. */
            continue;
        }

/*
        if (Packet.PacketId != ExpectedPacketId)
        {
            // Ask for a resend!
            continue;
        }
*/

        /* Step 5 - Read Checksum */
        KdStatus = KdpReceiveBuffer(&Packet.Checksum, sizeof(ULONG));
        if (KdStatus != KDP_PACKET_RECEIVED)
        {
            /* Didn't receive Checksum. Start over. */
            continue;
        }

        /* Step 6 - Handle control packets */
        if (Packet.PacketLeader == CONTROL_PACKET_LEADER)
        {
            switch (Packet.PacketType)
            {
                case PACKET_TYPE_KD_ACKNOWLEDGE:
                    /* Are we waiting for an ACK packet? */                    
                    if (PacketType == PACKET_TYPE_KD_ACKNOWLEDGE &&
                        Packet.PacketId == (CurrentPacketId & ~SYNC_PACKET_ID))
                    {
                        /* Remote acknowledges the last packet */
                        CurrentPacketId ^= 1;
                        return KDP_PACKET_RECEIVED;
                    }
                    /* That's not what we were waiting for, start over. */
                    continue;

                case PACKET_TYPE_KD_RESET:
                    KDDBGPRINT("KdReceivePacket - got a reset packet\n");
                    KdpSendControlPacket(PACKET_TYPE_KD_RESET, 0);
                    CurrentPacketId = INITIAL_PACKET_ID;
                    /* Fall through */

                case PACKET_TYPE_KD_RESEND:
                    KDDBGPRINT("KdReceivePacket - got PACKET_TYPE_KD_RESEND\n");
                    /* Remote wants us to resend the last packet */
                    return KDP_PACKET_RESEND;

                default:
                    KDDBGPRINT("KdReceivePacket - got unknown control packet\n");
                    return KDP_PACKET_RESEND;
            }
        }

        /* Did we wait for an ack packet? */
        if (PacketType == PACKET_TYPE_KD_ACKNOWLEDGE)
        {
            /* We received something different */
            KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0);
            CurrentPacketId ^= 1;
            return KDP_PACKET_RECEIVED;
        }

        /* Did we get the right packet type? */
        if (PacketType != Packet.PacketType)
        {
            /* We received something different, start over */
            KDDBGPRINT("KdReceivePacket - wrong PacketType\n");
            KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0);
            continue;
        }

        /* Get size of the message header */
        switch (Packet.PacketType)
        {
            case PACKET_TYPE_KD_STATE_CHANGE64:
                MessageHeader->Length = sizeof(DBGKD_WAIT_STATE_CHANGE64);
                break;

            case PACKET_TYPE_KD_STATE_MANIPULATE:
                MessageHeader->Length = sizeof(DBGKD_MANIPULATE_STATE64);
                break;

            case PACKET_TYPE_KD_DEBUG_IO:
                MessageHeader->Length = sizeof(DBGKD_DEBUG_IO);
                break;

            default:
                KDDBGPRINT("KdReceivePacket - unknown PacketType\n");
                return KDP_PACKET_RESEND;
        }

        //KDDBGPRINT("KdReceivePacket - got normal PacketType\n");

        /* Packet smaller than expected? */
        if (MessageHeader->Length > Packet.ByteCount)
        {
            KDDBGPRINT("KdReceivePacket - too few data (%d) for type %d\n",
                          Packet.ByteCount, MessageHeader->Length);
            MessageHeader->Length = Packet.ByteCount;
        }

        //KDDBGPRINT("KdReceivePacket - got normal PacketType, Buffer = %p\n", MessageHeader->Buffer);

        /* Receive the message header data */
        KdStatus = KdpReceiveBuffer(MessageHeader->Buffer,
                                   MessageHeader->Length);
        if (KdStatus != KDP_PACKET_RECEIVED)
        {
            /* Didn't receive data. Packet needs to be resent. */
            KDDBGPRINT("KdReceivePacket - Didn't receive message header data.\n");
            KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0);
            continue;
        }

        //KDDBGPRINT("KdReceivePacket - got normal PacketType 3\n");

        /* Calculate checksum for the header data */
        Checksum = KdpCalculateChecksum(MessageHeader->Buffer,
                                        MessageHeader->Length);

        /* Calculate the length of the message data */
        *DataLength = Packet.ByteCount - MessageHeader->Length;

        /* Shall we receive messsage data? */
        if (MessageData)
        {
            /* Set the length of the message data */
            MessageData->Length = *DataLength;

            /* Do we have data? */
            if (MessageData->Length)
            {
                KDDBGPRINT("KdReceivePacket - got data\n");

                /* Receive the message data */
                KdStatus = KdpReceiveBuffer(MessageData->Buffer,
                                           MessageData->Length);
                if (KdStatus != KDP_PACKET_RECEIVED)
                {
                    /* Didn't receive data. Start over. */
                    KDDBGPRINT("KdReceivePacket - Didn't receive message data.\n");
                    KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0);
                    continue;
                }

                /* Add cheksum for message data */
                Checksum += KdpCalculateChecksum(MessageData->Buffer,
                                                 MessageData->Length);
            }
        }

        /* Compare checksum */
        if (Packet.Checksum != Checksum)
        {
            KDDBGPRINT("KdReceivePacket - wrong cheksum, got %x, calculated %x\n",
                          Packet.Checksum, Checksum);
            KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0);
            continue;
        }

        /* We must receive a PACKET_TRAILING_BYTE now */
        KdStatus = KdpReceiveBuffer(&Byte, sizeof(UCHAR));
        if (KdStatus != KDP_PACKET_RECEIVED || Byte != PACKET_TRAILING_BYTE)
        {
            KDDBGPRINT("KdReceivePacket - wrong trailing byte (0x%x), status 0x%x\n", Byte, KdStatus);
            KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0);
            continue;
        }

        /* Acknowledge the received packet */
        KdpSendControlPacket(PACKET_TYPE_KD_ACKNOWLEDGE, Packet.PacketId);

        //KDDBGPRINT("KdReceivePacket - all ok\n");

        return KDP_PACKET_RECEIVED;
    }

    return KDP_PACKET_RECEIVED;
}


VOID
NTAPI
KdSendPacket(
    IN ULONG PacketType,
    IN PSTRING MessageHeader,
    IN PSTRING MessageData,
    IN OUT PKD_CONTEXT Context)
{
    KD_PACKET Packet;
    KDP_STATUS KdStatus;

    /* Initialize a KD_PACKET */
    Packet.PacketLeader = PACKET_LEADER;
    Packet.PacketType = PacketType;
    Packet.ByteCount = MessageHeader->Length;
    Packet.Checksum = KdpCalculateChecksum(MessageHeader->Buffer,
                                           MessageHeader->Length);

    /* If we have message data, add it to the packet */
    if (MessageData)
    {
        Packet.ByteCount += MessageData->Length;
        Packet.Checksum += KdpCalculateChecksum(MessageData->Buffer,
                                                MessageData->Length);
    }

    for (;;)
    {
        /* Set the packet id */
        Packet.PacketId = CurrentPacketId;

        /* Send the packet header to the KD port */
        KdpSendBuffer(&Packet, sizeof(KD_PACKET));

        /* Send the message header */
        KdpSendBuffer(MessageHeader->Buffer, MessageHeader->Length);

        /* If we have meesage data, also send it */
        if (MessageData)
        {
            KdpSendBuffer(MessageData->Buffer, MessageData->Length);
        }

        /* Finalize with a trailing byte */
        KdpSendByte(PACKET_TRAILING_BYTE);

        /* Wait for acknowledge */
        KdStatus = KdReceivePacket(PACKET_TYPE_KD_ACKNOWLEDGE,
                                  NULL,
                                  NULL,
                                  0,
                                  NULL);

        /* Did we succeed? */
        if (KdStatus == KDP_PACKET_RECEIVED)
        {
            CurrentPacketId &= ~SYNC_PACKET_ID;
            break;
        }

        /* PACKET_TYPE_KD_DEBUG_IO is allowed to instantly timeout */
        if (PacketType == PACKET_TYPE_KD_DEBUG_IO)
        {
            /* No response, silently fail. */
            return;
        }

        /* Packet timed out, send it again */
    }

    return;
}

