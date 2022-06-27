/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Base functions for the kernel network debugger
 * COPYRIGHT:   Copyright 2009-2016 Timo Kreuzer <timo.kreuzer@reactos.org>
 *              Copyright 2022 Hervé Poussineau <hpoussin@reactos.org>
 */

#define NOEXTAPI
#include <ntifs.h>
#include <windbgkd.h>
#include <arc/arc.h>
#include <kddll.h>
#include <kdnetextensibility.h>

/* GLOBALS ********************************************************************/

ULONG CurrentPacketId = INITIAL_PACKET_ID | SYNC_PACKET_ID;
ULONG RemotePacketId  = INITIAL_PACKET_ID;
PKDNET_EXTENSIBILITY_EXPORTS KdNetExtensibilityExports = NULL;

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
    PUCHAR ByteBuffer = Buffer;
    ULONG Checksum = 0;

    while (Length-- > 0)
    {
        Checksum += (ULONG)*ByteBuffer++;
    }
    return Checksum;
}

VOID
NTAPI
KdpSendControlPacket(
    IN USHORT PacketType,
    IN ULONG PacketId OPTIONAL)
{
    PKD_PACKET Packet;
    NTSTATUS Status;
    ULONG TxHandle;

    Status = KdGetTxPacket(NULL, &TxHandle);
    if (!NT_SUCCESS(Status))
        return;
    Packet = KdGetPacketAddress(NULL, TxHandle);
    if (!Packet)
        return;
    ASSERT(KdGetPacketLength(NULL, TxHandle) >= sizeof(KD_PACKET));

    Packet->PacketLeader = CONTROL_PACKET_LEADER;
    Packet->PacketId = PacketId;
    Packet->ByteCount = 0;
    Packet->Checksum = 0;
    Packet->PacketType = PacketType;

    KdSendTxPacket(NULL, TxHandle, sizeof(KD_PACKET));
}

/******************************************************************************
 * \name KdpReceivePacketLeader
 * \brief Receives a packet leader from the KD port.
 * \param PacketLeader Pointer to an ULONG that receives the packet leader.
 * \return KdPacketReceived if successful.
 *         KdPacketTimedOut if the receive timed out.
 *         KdPacketNeedsResend if a breakin byte was detected.
 */
KDSTATUS
NTAPI
KdpReceivePacketLeader(
    OUT PULONG PacketLeader,
    IN PUCHAR *pRxPacket,
    IN PULONG pRxLength)
{
    UCHAR Index = 0, Byte, Buffer[4];

    /* Set first character to 0 */
    Buffer[0] = 0;

    do
    {
        if (*pRxLength == 0)
        {
            /* End of RX packet. Check if we already got a breakin byte */
            if (Buffer[0] == BREAKIN_PACKET_BYTE)
                return KdPacketNeedsResend;

            /* Report timeout */
            return KdPacketTimedOut;
        }

        /* Receive a single byte */
        Byte = **pRxPacket;
        ++(*pRxPacket);
        --(*pRxLength);

        /* Check if this is a valid packet leader byte */
        if (Byte == PACKET_LEADER_BYTE ||
            Byte == CONTROL_PACKET_LEADER_BYTE)
        {
            /* Check if we match the first byte */
            if (Byte != Buffer[0])
            {
                /* No, this is the new byte 0! */
                Index = 0;
            }

            /* Store the byte in the buffer */
            Buffer[Index] = Byte;

            /* Continue with next byte */
            Index++;
            continue;
        }

        /* Check for breakin byte */
        if (Byte == BREAKIN_PACKET_BYTE)
        {
            Index = 0;
            Buffer[0] = Byte;
            continue;
        }

        /* Restart */
        Index = 0;
        Buffer[0] = 0;
    }
    while (Index < 4);

    /* Return the received packet leader */
    *PacketLeader = *(PULONG)Buffer;

    return KdPacketReceived;
}

/******************************************************************************
 * \name KdpReceiveBuffer
 * \brief Receives data from the KD port and fills a buffer.
 * \param Buffer Pointer to a buffer that receives the data.
 * \param Size Size of data to receive in bytes.
 * \return KdPacketReceived if successful.
 *         KdPacketTimedOut if the receive timed out.
 */
KDSTATUS
NTAPI
KdpReceiveBuffer(
    OUT PVOID Buffer,
    IN  ULONG Size,
    IN  PUCHAR *pRxPacket,
    IN  PULONG pRxLength)
{
    if (*pRxLength < Size)
    {
        *pRxLength = 0;
        return KdPacketTimedOut;
    }

    RtlCopyMemory(Buffer, *pRxPacket, Size);
    (*pRxPacket) += Size;
    (*pRxLength) -= Size;

    return KdPacketReceived;
}

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

NTSTATUS
NTAPI
KdDebuggerInitialize0(IN PLOADER_PARAMETER_BLOCK LoaderBlock OPTIONAL)
{
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KdDebuggerInitialize1(IN PLOADER_PARAMETER_BLOCK LoaderBlock OPTIONAL)
{
    KDNET_EXTENSIBILITY_IMPORTS ImportTable = {0};
    NTSTATUS Status;

    Status = KdInitializeLibrary(&ImportTable, LoaderBlock ? LoaderBlock->LoadOptions : NULL, NULL);
    if (NT_SUCCESS(Status))
        KdNetExtensibilityExports = ImportTable.Exports;
    return Status;
}


NTSTATUS
NTAPI
KdRestore(IN BOOLEAN SleepTransition)
{
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KdSave(IN BOOLEAN SleepTransition)
{
    return STATUS_SUCCESS;
}

VOID
NTAPI
KdSetHiberRange(VOID)
{
}

KDSTATUS
NTAPI
KdReceivePacket(
    _In_ ULONG PacketType,
    _Out_ PSTRING MessageHeader,
    _Out_ PSTRING MessageData,
    _Out_ PULONG DataLength,
    _Inout_ PKD_CONTEXT Context)
{
    UCHAR Byte = 0;
    KDSTATUS KdStatus;
    KD_PACKET Packet;
    ULONG Checksum, MessageDataLength;
    ULONG RxHandle, RxLength;
    PVOID RxPacket;
    NTSTATUS Status;

    /* Special handling for breakin packet */
    if (PacketType == PACKET_TYPE_KD_POLL_BREAKIN)
    {
        return FALSE;
    }

    for (;;)
    {
        Status = KdGetRxPacket(NULL, &RxHandle, &RxPacket, &RxLength);
        if (!NT_SUCCESS(Status))
        {
            /* Report timeout */
            return KdPacketTimedOut;
        }

        /* Step 1 - Read PacketLeader */
        KdStatus = KdpReceivePacketLeader(&Packet.PacketLeader, (PUCHAR*)&RxPacket, &RxLength);
        if (KdStatus != KdPacketReceived)
        {
            return KdStatus;
        }

        /* Step 2 - Read PacketType */
        KdStatus = KdpReceiveBuffer(&Packet.PacketType, sizeof(USHORT), (PUCHAR*)&RxPacket, &RxLength);
        if (KdStatus != KdPacketReceived)
        {
            /* Didn't receive a PacketType. */
            return KdStatus;
        }

        /* Check if we got a resend packet */
        if (Packet.PacketLeader == CONTROL_PACKET_LEADER &&
            Packet.PacketType == PACKET_TYPE_KD_RESEND)
        {
            return KdPacketNeedsResend;
        }

        /* Step 3 - Read ByteCount */
        KdStatus = KdpReceiveBuffer(&Packet.ByteCount, sizeof(USHORT), (PUCHAR*)&RxPacket, &RxLength);
        if (KdStatus != KdPacketReceived)
        {
            /* Didn't receive ByteCount. */
            return KdStatus;
        }

        /* Step 4 - Read PacketId */
        KdStatus = KdpReceiveBuffer(&Packet.PacketId, sizeof(ULONG), (PUCHAR*)&RxPacket, &RxLength);
        if (KdStatus != KdPacketReceived)
        {
            /* Didn't receive PacketId. */
            return KdStatus;
        }

        /* Step 5 - Read Checksum */
        KdStatus = KdpReceiveBuffer(&Packet.Checksum, sizeof(ULONG), (PUCHAR*)&RxPacket, &RxLength);
        if (KdStatus != KdPacketReceived)
        {
            /* Didn't receive Checksum. */
            return KdStatus;
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
                        return KdPacketReceived;
                    }
                    /* That's not what we were waiting for, start over */
                    continue;

                case PACKET_TYPE_KD_RESET:
                    CurrentPacketId = INITIAL_PACKET_ID;
                    RemotePacketId  = INITIAL_PACKET_ID;
                    KdpSendControlPacket(PACKET_TYPE_KD_RESET, 0);
                    /* Fall through */

                case PACKET_TYPE_KD_RESEND:
                    /* Remote wants us to resend the last packet */
                    return KdPacketNeedsResend;

                default:
                    /* We got an invalid packet, ignore it and start over */
                    continue;
            }
        }

        /* Did we wait for an ack packet? */
        if (PacketType == PACKET_TYPE_KD_ACKNOWLEDGE)
        {
            /* We received something different */
            KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0);
            CurrentPacketId ^= 1;
            return KdPacketReceived;
        }

        /* Get size of the message header */
        MessageHeader->Length = MessageHeader->MaximumLength;

        /* Packet smaller than expected or too big? */
        if (Packet.ByteCount < MessageHeader->Length ||
            Packet.ByteCount > PACKET_MAX_SIZE)
        {
            MessageHeader->Length = Packet.ByteCount;
            KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0);
            continue;
        }

        /* Receive the message header data */
        KdStatus = KdpReceiveBuffer(MessageHeader->Buffer,
                                    MessageHeader->Length,
                                    (PUCHAR*)&RxPacket, &RxLength);
        if (KdStatus != KdPacketReceived)
        {
            /* Didn't receive data. Packet needs to be resent. */
            KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0);
            continue;
        }

        /* Calculate checksum for the header data */
        Checksum = KdpCalculateChecksum(MessageHeader->Buffer,
                                        MessageHeader->Length);

        /* Calculate the length of the message data */
        MessageDataLength = Packet.ByteCount - MessageHeader->Length;

        /* Shall we receive message data? */
        if (MessageData)
        {
            /* Set the length of the message data */
            MessageData->Length = (USHORT)MessageDataLength;

            /* Do we have data? */
            if (MessageData->Length)
            {
                /* Receive the message data */
                KdStatus = KdpReceiveBuffer(MessageData->Buffer,
                                            MessageData->Length,
                                            (PUCHAR*)&RxPacket, &RxLength);
                if (KdStatus != KdPacketReceived)
                {
                    /* Didn't receive data. Start over. */
                    KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0);
                    continue;
                }

                /* Add cheksum for message data */
                Checksum += KdpCalculateChecksum(MessageData->Buffer,
                                                 MessageData->Length);
            }
        }

        /* We must receive a PACKET_TRAILING_BYTE now */
        KdStatus = KdpReceiveBuffer(&Byte, sizeof(UCHAR), (PUCHAR*)&RxPacket, &RxLength);
        if (KdStatus != KdPacketReceived || Byte != PACKET_TRAILING_BYTE)
        {
            KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0);
            continue;
        }

        /* Compare checksum */
        if (Packet.Checksum != Checksum)
        {
            KdpSendControlPacket(PACKET_TYPE_KD_RESEND, 0);
            continue;
        }

        /* Acknowledge the received packet */
        KdpSendControlPacket(PACKET_TYPE_KD_ACKNOWLEDGE, Packet.PacketId);

        /* Check if the received PacketId is ok */
        if (Packet.PacketId != RemotePacketId)
        {
            /* Continue with next packet */
            continue;
        }

        /* Did we get the right packet type? */
        if (PacketType == Packet.PacketType)
        {
            /* Yes, return success */
            RemotePacketId ^= 1;
            return KdPacketReceived;
        }

        /* We received something different, ignore it. */
    }

    return KdPacketReceived;
}

VOID
NTAPI
KdSendPacket(
    _In_ ULONG PacketType,
    _In_ PSTRING MessageHeader,
    _In_ PSTRING MessageData,
    _Inout_ PKD_CONTEXT Context)
{
    PKD_PACKET Packet;
    KDSTATUS KdStatus;
    ULONG Retries;
    ULONG TxHandle, DataLength;
    NTSTATUS Status;

    Status = KdGetTxPacket(NULL, &TxHandle);
    if (!NT_SUCCESS(Status))
        return;
    Packet = KdGetPacketAddress(NULL, TxHandle);
    ASSERT(Packet);
    ASSERT(KdGetPacketLength(NULL, TxHandle) >= sizeof(KD_PACKET));

    /* Initialize a KD_PACKET */
    Packet->PacketLeader = PACKET_LEADER;
    Packet->PacketType = (USHORT)PacketType;
    Packet->ByteCount = MessageHeader->Length;
    Packet->Checksum = KdpCalculateChecksum(MessageHeader->Buffer,
                                            MessageHeader->Length);

    /* If we have message data, add it to the packet */
    if (MessageData)
    {
        Packet->ByteCount += MessageData->Length;
        Packet->Checksum += KdpCalculateChecksum(MessageData->Buffer,
                                                 MessageData->Length);
    }

    if (KdGetPacketLength(NULL, TxHandle) < sizeof(KD_PACKET) + Packet->ByteCount + 1)
    {
        /* Release TX packet */
        KdSendTxPacket(NULL, TxHandle, 0);
        return;
    }


    /* Copy message header and message data to packet */
    RtlCopyMemory(Packet + 1, MessageHeader->Buffer, MessageHeader->Length);
    if (MessageData)
        RtlCopyMemory((PUCHAR)(Packet + 1) + MessageHeader->Length, MessageData->Buffer, MessageData->Length);

    /* Finalize with a trailing byte */
    ((PUCHAR)(Packet + 1))[Packet->ByteCount] = PACKET_TRAILING_BYTE;

    Retries = 3;

    for (;;)
    {
        /* Set the packet id */
        Packet->PacketId = CurrentPacketId;

        /* Send the packet to the KD port */
        KdSendTxPacket(NULL, TxHandle, sizeof(KD_PACKET) + Packet->ByteCount + 1);

        /* Wait for acknowledge */
        KdStatus = KdReceivePacket(PACKET_TYPE_KD_ACKNOWLEDGE,
                                   NULL,
                                   NULL,
                                   &DataLength,
                                   NULL);

        /* Did we succeed? */
        if (KdStatus == KdPacketReceived)
        {
            /* Packet received, we can quit the loop */
            CurrentPacketId &= ~SYNC_PACKET_ID;
            Retries = 3;
            break;
        }
        else if (KdStatus == KdPacketTimedOut)
        {
            /* Timeout, decrement the retry count */
            if (Retries > 0)
                Retries--;

            /*
             * If the retry count reaches zero, bail out
             * for packet types allowed to timeout.
             */
            if (Retries == 0)
            {
                ULONG MessageId = *(PULONG)MessageHeader->Buffer;
                switch (PacketType)
                {
                    case PACKET_TYPE_KD_DEBUG_IO:
                    {
                        if (MessageId != DbgKdPrintStringApi) continue;
                        break;
                    }

                    case PACKET_TYPE_KD_STATE_CHANGE32:
                    case PACKET_TYPE_KD_STATE_CHANGE64:
                    {
                        if (MessageId != DbgKdLoadSymbolsStateChange) continue;
                        break;
                    }

                    case PACKET_TYPE_KD_FILE_IO:
                    {
                        if (MessageId != DbgKdCreateFileApi) continue;
                        break;
                    }
                }

                /* Reset debugger state */
                CurrentPacketId = INITIAL_PACKET_ID | SYNC_PACKET_ID;
                RemotePacketId  = INITIAL_PACKET_ID;

                return;
            }
        }

        /* Packet timed out, send it again */
    }
}
