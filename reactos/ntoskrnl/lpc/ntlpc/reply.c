/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/lpc/reply.c
 * PURPOSE:         Local Procedure Call: Receive (Replies)
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include "lpc.h"
#define NDEBUG
#include <internal/debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtReplyPort(IN HANDLE PortHandle,
            IN PPORT_MESSAGE LpcReply)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtReplyWaitReceivePortEx(IN HANDLE PortHandle,
                         OUT PVOID *PortContext OPTIONAL,
                         IN PPORT_MESSAGE ReplyMessage OPTIONAL,
                         OUT PPORT_MESSAGE ReceiveMessage,
                         IN PLARGE_INTEGER Timeout OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtReplyWaitReceivePort(IN HANDLE PortHandle,
                       OUT PVOID *PortContext OPTIONAL,
                       IN PPORT_MESSAGE ReplyMessage OPTIONAL,
                       OUT PPORT_MESSAGE ReceiveMessage)
{
    /* Call the newer API */
    return NtReplyWaitReceivePortEx(PortHandle,
                                    PortContext,
                                    ReplyMessage,
                                    ReceiveMessage,
                                    NULL);
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtReplyWaitReplyPort(IN HANDLE PortHandle,
                     IN PPORT_MESSAGE ReplyMessage)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtReadRequestData(IN HANDLE PortHandle,
                  IN PPORT_MESSAGE Message,
                  IN ULONG Index,
                  IN PVOID Buffer,
                  IN ULONG BufferLength,
                  OUT PULONG Returnlength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtWriteRequestData(IN HANDLE PortHandle,
                   IN PPORT_MESSAGE Message,
                   IN ULONG Index,
                   IN PVOID Buffer,
                   IN ULONG BufferLength,
                   OUT PULONG ReturnLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
