/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/lpc/send.c
 * PURPOSE:         Local Procedure Call: Sending (Requests)
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include "lpc.h"
#define NDEBUG
#include <internal/debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LpcRequestPort(IN PVOID Port,
               IN PPORT_MESSAGE LpcMessage)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
LpcRequestWaitReplyPort(IN PVOID Port,
                        IN PPORT_MESSAGE LpcMessageRequest,
                        OUT PPORT_MESSAGE LpcMessageReply)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtRequestPort(IN HANDLE PortHandle,
              IN PPORT_MESSAGE LpcMessage)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtRequestWaitReplyPort(IN HANDLE PortHandle,
                       IN PPORT_MESSAGE LpcRequest,
                       IN OUT PPORT_MESSAGE LpcReply)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
