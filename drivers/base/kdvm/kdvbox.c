/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kdvm/kdvbox.c
 * PURPOSE:         VBOX data exchange function for kdvbox
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include "kdvm.h"

typedef struct
{
    ULONG SendSize;
    ULONG BufferSize;
} KDVBOX_SEND_HEADER, *PKDVBOX_SEND_HEADER;

typedef struct
{
    ULONG ReceivedDataSize;
} KDVBOX_RECEIVE_HEADER, *PKDVBOX_RECEIVE_HEADER;

VOID
NTAPI
KdVmPrepareBuffer(VOID)
{
    KdVmBufferPos = sizeof(KDVBOX_SEND_HEADER);
}

VOID
NTAPI
KdVmKdVmExchangeData(
    _Out_ PVOID* ReceiveData,
    _Out_ PULONG ReceiveDataSize)
{
    PKDVBOX_SEND_HEADER SendHeader;
    PKDVBOX_RECEIVE_HEADER ReceiveHeader;

    /* Setup the send-header */
    SendHeader = (PKDVBOX_SEND_HEADER)KdVmDataBuffer;
    SendHeader->SendSize = KdVmBufferPos - sizeof(KDVBOX_SEND_HEADER);
    SendHeader->BufferSize = KDVM_BUFFER_SIZE;

    //KdpDbgPrint("Sending buffer:\n");
    //KdVmDbgDumpBuffer(KdVmDataBuffer, KdVmBufferPos);

    /* Do the data exchange */
    KdVmExchange((ULONG_PTR)KdVmBufferPhysicalAddress.QuadPart, 0);

    /* Reset the buffer position */
    KdVmBufferPos = sizeof(KDVBOX_SEND_HEADER);

    /* Get the receive-header and return information about the received data */
    ReceiveHeader = (PKDVBOX_RECEIVE_HEADER)KdVmDataBuffer;
    *ReceiveData = KdVmDataBuffer + sizeof(KDVBOX_RECEIVE_HEADER);
    *ReceiveDataSize = ReceiveHeader->ReceivedDataSize;

    //KdpDbgPrint("got data:\n");
    //KdVmDbgDumpBuffer(KdVmDataBuffer, *ReceiveDataSize + sizeof(*ReceiveHeader));

}
