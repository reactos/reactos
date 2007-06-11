/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/lpc/listen.c
 * PURPOSE:         Local Procedure Call: Listening
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtListenPort(IN HANDLE PortHandle,
             OUT PPORT_MESSAGE ConnectMessage)
{
    NTSTATUS Status;
    PAGED_CODE();
    LPCTRACE(LPC_LISTEN_DEBUG, "Handle: %lx\n", PortHandle);

    /* Wait forever for a connection request. */
    for (;;)
    {
        /* Do the wait */
        Status = NtReplyWaitReceivePort(PortHandle,
                                        NULL,
                                        NULL,
                                        ConnectMessage);

        /* Accept only LPC_CONNECTION_REQUEST requests. */
        if ((Status != STATUS_SUCCESS) ||
            (LpcpGetMessageType(ConnectMessage) == LPC_CONNECTION_REQUEST))
        {
            /* Break out */
            break;
        }
    }

    /* Return status */
    return Status;
}


/* EOF */
