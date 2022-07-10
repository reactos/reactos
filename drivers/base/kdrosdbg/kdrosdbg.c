/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kdcom/kdbg.c
 * PURPOSE:         Serial i/o functions for the kernel debugger.
 * PROGRAMMER:      Alex Ionescu
 *                  Hervé Poussineau
 */

/* INCLUDES *****************************************************************/

#include "kdrosdbg.h"
#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
KdDebuggerInitialize0(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock OPTIONAL)
{
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
KdDebuggerInitialize1(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock OPTIONAL)
{
    return STATUS_SUCCESS;
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

/*
 * @implemented
 */
NTSTATUS
NTAPI
KdSave(
    IN BOOLEAN SleepTransition)
{
    /* Nothing to do on COM ports */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
KdRestore(
    IN BOOLEAN SleepTransition)
{
    /* Nothing to do on COM ports */
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
VOID
NTAPI
KdSendPacket(
    IN ULONG PacketType,
    IN PSTRING MessageHeader,
    IN PSTRING MessageData,
    IN OUT PKD_CONTEXT Context)
{
}

/*
 * @unimplemented
 */
KDSTATUS
NTAPI
KdReceivePacket(
    IN ULONG PacketType,
    OUT PSTRING MessageHeader,
    OUT PSTRING MessageData,
    OUT PULONG DataLength,
    IN OUT PKD_CONTEXT Context)
{
#ifndef KDBG
    if (PacketType == PACKET_TYPE_KD_STATE_MANIPULATE)
    {
        /* Let's say that everything went fine every time. */
        PDBGKD_MANIPULATE_STATE64 ManipulateState = (PDBGKD_MANIPULATE_STATE64)MessageHeader->Buffer;
        RtlZeroMemory(MessageHeader->Buffer, MessageHeader->MaximumLength);
        ManipulateState->ApiNumber = DbgKdContinueApi;
        ManipulateState->u.Continue.ContinueStatus = STATUS_SUCCESS;
        MessageHeader->Length = sizeof(*ManipulateState);
        return KdPacketReceived;
    }
#endif

    return KdPacketTimedOut;
}

/* EOF */
