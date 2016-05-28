/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kddll/kddll.c
 * PURPOSE:         Base functions for the kernel debugger.
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include "kddll.h"

#define _FULL_ 1
#define _WORKS_ 0
#define _WORKS2_ 0

/* GLOBALS ********************************************************************/

ULONG CurrentPacketId = INITIAL_PACKET_ID | SYNC_PACKET_ID;
ULONG RemotePacketId  = INITIAL_PACKET_ID;

#if _FULL_
ULONG KdCompNumberRetries = 5;
ULONG KdCompRetryCount = 5;
#endif

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
    return STATUS_SUCCESS;
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
    if (PacketType == PACKET_TYPE_KD_POLL_BREAKIN)
    {
        return KdpPollBreakIn();
    }

    for (;;)
    {
#if _FULL_ || _WORKS2_
        if (KdContext)
        {
            KdContext->KdpControlCPending = FALSE;
        }
#endif
        /* Step 1 - Read PacketLeader */
        KdStatus = KdpReceivePacketLeader(&Packet.PacketLeader);
#if _FULL_
        if (KdStatus != KDP_PACKET_TIMEOUT)
        {
//            KdCompNumberRetries = KdCompRetryCount;
        }
#endif
        if (KdStatus != KDP_PACKET_RECEIVED)
        {
            /* Check if we got a breakin  */
            if (KdStatus == KDP_PACKET_RESEND)
            {
#if _FULL_ || _WORKS2_ || _PATCH_
                if (KdContext)
#endif
                    KdContext->KdpControlCPending = TRUE;
            }
            return KdStatus;
        }

        /* Step 2 - Read PacketType */
        KdStatus = KdpReceiveBuffer(&Packet.PacketType, sizeof(USHORT));
        if (KdStatus != KDP_PACKET_RECEIVED)
        {
            /* Didn't receive a PacketType. */
            KDDBGPRINT("KdReceivePacket - Didn't receive a PacketType.\n");
#if _WORKS_
            continue;
#else
            return KdStatus;
#endif
        }

        /* Check if we got a resend packet */
        if (Packet.PacketLeader == CONTROL_PACKET_LEADER &&
            Packet.PacketType == PACKET_TYPE_KD_RESEND)
        {
            KDDBGPRINT("KdReceivePacket - PACKET_TYPE_KD_RESEND.\n");
            return KDP_PACKET_RESEND;
        }

        /* Step 3 - Read ByteCount */
        KdStatus = KdpReceiveBuffer(&Packet.ByteCount, sizeof(USHORT));
#if _WORKS_
        if (KdStatus != KDP_PACKET_RECEIVED || Packet.ByteCount > PACKET_MAX_SIZE)
#else
        if (KdStatus != KDP_PACKET_RECEIVED)
#endif
        {
            /* Didn't receive ByteCount _WORKS_: or it's too big. Start over. */
            KDDBGPRINT("KdReceivePacket - Didn't receive ByteCount.\n");
#if _WORKS_
            continue;
#else
            return KdStatus;
#endif
        }

        /* Step 4 - Read PacketId */
        KdStatus = KdpReceiveBuffer(&Packet.PacketId, sizeof(ULONG));
        if (KdStatus != KDP_PACKET_RECEIVED)
        {
            /* Didn't receive PacketId. */
            KDDBGPRINT("KdReceivePacket - Didn't receive PacketId.\n");
#if _WORKS_
            continue;
#else
            return KdStatus;
#endif
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
            /* Didn't receive Checksum. */
            KDDBGPRINT("KdReceivePacket - Didn't receive Checksum.\n");
#if _WORKS_
            continue;
#else
            return KdStatus;
#endif
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
                    /* That's not what we were waiting for, start over */
                    continue;

                case PACKET_TYPE_KD_RESET:
                    KDDBGPRINT("KdReceivePacket - got PACKET_TYPE_KD_RESET\n");
                    CurrentPacketId = INITIAL_PACKET_ID;
                    RemotePacketId  = INITIAL_PACKET_ID;
                    KdpSendControlPacket(PACKET_TYPE_KD_RESET, 0);
                    /* Fall through */

                case PACKET_TYPE_KD_RESEND:
                    KDDBGPRINT("KdReceivePacket - got PACKET_TYPE_KD_RESEND\n");
                    /* Remote wants us to resend the last packet */
                    return KDP_PACKET_RESEND;

                default:
                    KDDBGPRINT("KdReceivePacket - got unknown control packet\n");
#if _FULL_
                    continue;
#else
                    return KDP_PACKET_RESEND;
#endif
            }
        }

        /* Did we wait for an ack packet? */
        if (PacketType == PACKET_TYPE_KD_ACKNOWLEDGE)
        {
            /* We received something different */
#if _FULL_ || _WORKS2_
            if (Packet.PacketId != RemotePacketId)
            {
                KdpSendControlPacket(PACKET_TYPE_KD_ACKNOWLEDGE,
                                     Packet.PacketId);
                continue;
            }
#elif _PATCH_
            DBGKD_MANIPULATE_STATE64 State;
            KdStatus = KdpReceiveBuffer(&State, sizeof(State));
            KDDBGPRINT("KdReceivePacket - unxpected Packet.PacketType=0x%x, 0x%x, 0x%x\n",
                       Packet.PacketType, Packet.Checksum, State.ApiNumber);
#endif
            KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0);
            CurrentPacketId ^= 1;
            return KDP_PACKET_RECEIVED;
        }

        /* Get size of the message header */
#if _WORKS_
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
#else
        MessageHeader->Length = MessageHeader->MaximumLength;

        /* Packet smaller than expected or too big? */
        if (Packet.ByteCount < MessageHeader->Length ||
            Packet.ByteCount > PACKET_MAX_SIZE)
#endif
        {
            KDDBGPRINT("KdReceivePacket - too few data (%d) for type %d\n",
                          Packet.ByteCount, MessageHeader->Length);
            MessageHeader->Length = Packet.ByteCount;
            KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0);
            continue;
        }

        //KDDBGPRINT("KdReceivePacket - got normal PacketType, Buffer = %p\n", MessageHeader->Buffer);

        /* Receive the message header data */
#if _WORKS2_
        MessageHeader->Length = MessageHeader->MaximumLength;
#endif
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
            MessageData->Length = (USHORT)*DataLength;

            /* Do we have data? */
            if (MessageData->Length)
            {
                KDDBGPRINT("KdReceivePacket - 0x%lx bytes data\n", *DataLength);

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

        /* We must receive a PACKET_TRAILING_BYTE now */
        KdStatus = KdpReceiveBuffer(&Byte, sizeof(UCHAR));
        if (KdStatus != KDP_PACKET_RECEIVED || Byte != PACKET_TRAILING_BYTE)
        {
            KDDBGPRINT("KdReceivePacket - wrong trailing byte (0x%x), status 0x%x\n", Byte, KdStatus);
            KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0);
            continue;
        }

        /* Compare checksum */
        if (Packet.Checksum != Checksum)
        {
            KDDBGPRINT("KdReceivePacket - wrong cheksum, got %x, calculated %x\n",
                       Packet.Checksum, Checksum);
            KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0);
            continue;
        }

#if _FULL_ || _WORKS2_
        if (Packet.PacketId != INITIAL_PACKET_ID &&
            Packet.PacketId != (INITIAL_PACKET_ID ^ 1))
        {
            KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0);
            continue;
        }
#endif

        /* Acknowledge the received packet */
        KdpSendControlPacket(PACKET_TYPE_KD_ACKNOWLEDGE, Packet.PacketId);

        /* Check if the received PacketId is ok */
        if (Packet.PacketId != RemotePacketId)
        {
            /* Continue with next packet */
            KDDBGPRINT("KdReceivePacket - Wrong PacketId.\n");
            continue;
        }

        /* Did we get the right packet type? */
        if (PacketType == Packet.PacketType)
        {
            /* Yes, return success */
            //KDDBGPRINT("KdReceivePacket - all ok\n");
            RemotePacketId ^= 1;
            return KDP_PACKET_RECEIVED;
        }

        /* We received something different, ignore it. */
        KDDBGPRINT("KdReceivePacket - wrong PacketType\n");
    }

    return KDP_PACKET_RECEIVED;
}

static
BOOLEAN
IsQuickTimeoutAllowed(
    IN ULONG PacketType,
    IN PSTRING MessageHeader)
{
    PDBGKD_DEBUG_IO DebugIo = (PDBGKD_DEBUG_IO)MessageHeader->Buffer;
    ULONG ApiNumber = DebugIo->ApiNumber;

    if ( ((PacketType == PACKET_TYPE_KD_DEBUG_IO) &&
          ((ApiNumber == DbgKdPrintStringApi))) ||
         ((PacketType == PACKET_TYPE_KD_FILE_IO) &&
          ((ApiNumber == DbgKdCreateFileApi))) ||
         ((PacketType == PACKET_TYPE_KD_STATE_CHANGE64) &&
          ((ApiNumber == DbgKdLoadSymbolsStateChange))) )
    {
        return TRUE;
    }

    return FALSE;
}

VOID
NTAPI
KdSendPacket(
    IN ULONG PacketType,
    IN PSTRING MessageHeader,
    IN PSTRING MessageData,
    IN OUT PKD_CONTEXT KdContext)
{
    KD_PACKET Packet;
    KDP_STATUS KdStatus;
    ULONG Retries;

    /* Initialize a KD_PACKET */
    Packet.PacketLeader = PACKET_LEADER;
    Packet.PacketType = (USHORT)PacketType;
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

#if _FULL_
    Retries = KdCompNumberRetries = KdCompRetryCount;
#else
    Retries = KdContext->KdpDefaultRetries;
#endif

    for (;;)
    {
        /* Set the packet id */
        Packet.PacketId = CurrentPacketId;

        /* Send the packet header to the KD port */
        KdpSendBuffer(&Packet, sizeof(KD_PACKET));

        /* Send the message header */
        KdpSendBuffer(MessageHeader->Buffer, MessageHeader->Length);

        /* If we have message data, also send it */
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
                                   NULL,
                                   KdContext);

        /* Did we succeed? */
        if (KdStatus == KDP_PACKET_RECEIVED)
        {
            /* Packet received, we can quit the loop */
            CurrentPacketId &= ~SYNC_PACKET_ID;
            break;
        }

        if (KdStatus == KDP_PACKET_TIMEOUT)
        {
            /* Timeout, decrement the retry count */
            Retries--;

            /*
             * If the retry count reaches zero, bail out
             * for packet types allowed to timeout.
             */
            if ((Retries == 0) &&
                IsQuickTimeoutAllowed(PacketType, MessageHeader))
            {

                /* Reset debugger state */
                KD_DEBUGGER_NOT_PRESENT = TRUE;
                SharedUserData->KdDebuggerEnabled &= ~0x00000002;
                CurrentPacketId = INITIAL_PACKET_ID | SYNC_PACKET_ID;
                RemotePacketId = INITIAL_PACKET_ID;
                return;
            }
        }

        /* Packet timed out, send it again */
        KDDBGPRINT("KdSendPacket got KdStatus 0x%x\n", KdStatus);
    }

#if _FULL_
    KdCompNumberRetries = Retries;
#endif // _FULL_
}

