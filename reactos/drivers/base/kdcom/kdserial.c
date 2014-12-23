/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kddll/kdserial.c
 * PURPOSE:         Serial communication functions for the kernel debugger.
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include "kddll.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

/******************************************************************************
 * \name KdpSendBuffer
 * \brief Sends a buffer of data to the serial KD port.
 * \param Buffer Pointer to the data.
 * \param Size Size of data in bytes.
 */
VOID
NTAPI
KdpSendBuffer(
    IN PVOID Buffer,
    IN ULONG Size)
{
    PUCHAR ByteBuffer = Buffer;

    while (Size-- > 0)
    {
        KdpSendByte(*ByteBuffer++);
    }
}

/******************************************************************************
 * \name KdpReceiveBuffer
 * \brief Receives data from the KD port and fills a buffer.
 * \param Buffer Pointer to a buffer that receives the data.
 * \param Size Size of data to receive in bytes.
 * \return KDP_PACKET_RECEIVED if successful.
 *         KDP_PACKET_TIMEOUT if the receice timed out.
 */
KDP_STATUS
NTAPI
KdpReceiveBuffer(
    OUT PVOID Buffer,
    IN  ULONG Size)
{
    PUCHAR ByteBuffer = Buffer;
    UCHAR Byte;
    KDP_STATUS Status;

    while (Size-- > 0)
    {
        /* Try to get a byte from the port */
        Status = KdpReceiveByte(&Byte);
        if (Status != KDP_PACKET_RECEIVED)
            return Status;

        *ByteBuffer++ = Byte;
    }

    return KDP_PACKET_RECEIVED;
}


/******************************************************************************
 * \name KdpReceivePacketLeader
 * \brief Receives a packet leadr from the KD port.
 * \param PacketLeader Pointer to an ULONG that receives the packet leader.
 * \return KDP_PACKET_RECEIVED if successful.
 *         KDP_PACKET_TIMEOUT if the receive timed out.
 *         KDP_PACKET_RESEND if a breakin byte was detected.
 */
KDP_STATUS
NTAPI
KdpReceivePacketLeader(
    OUT PULONG PacketLeader)
{
    UCHAR Index = 0, Byte, Buffer[4];
    KDP_STATUS KdStatus;

    /* Set first character to 0 */
    Buffer[0] = 0;

    do
    {
        /* Receive a single byte */
        KdStatus = KdpReceiveByte(&Byte);

        /* Check for timeout */
        if (KdStatus == KDP_PACKET_TIMEOUT)
        {
            /* Check if we already got a breakin byte */
            if (Buffer[0] == BREAKIN_PACKET_BYTE)
            {
                return KDP_PACKET_RESEND;
            }

            /* Report timeout */
            return KDP_PACKET_TIMEOUT;
        }

        /* Check if we received a byte */
        if (KdStatus == KDP_PACKET_RECEIVED)
        {
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
                KDDBGPRINT("BREAKIN_PACKET_BYTE\n");
                Index = 0;
                Buffer[0] = Byte;
                continue;
            }
        }

        /* Restart */
        Index = 0;
        Buffer[0] = 0;
    }
    while (Index < 4);

    /* Enable the debugger */
    KD_DEBUGGER_NOT_PRESENT = FALSE;
    SharedUserData->KdDebuggerEnabled |= 0x00000002;

    /* Return the received packet leader */
    *PacketLeader = *(PULONG)Buffer;

    return KDP_PACKET_RECEIVED;
}

/* EOF */
