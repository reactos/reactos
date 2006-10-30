/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/lpc/complete.c
* PURPOSE:         Local Procedure Call: Connection Completion
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
 * @implemented
 */
NTSTATUS
NTAPI
NtAcceptConnectPort(OUT PHANDLE PortHandle,
                    IN PVOID PortContext OPTIONAL,
                    IN PPORT_MESSAGE ReplyMessage,
                    IN BOOLEAN AcceptConnection,
                    IN PPORT_VIEW ClientView,
                    IN PREMOTE_PORT_VIEW ServerView)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtCompleteConnectPort(IN HANDLE PortHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
