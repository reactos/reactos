/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/lpc/create.c
 * PURPOSE:         Local Procedure Call: Port/Queue/Message Creation
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include "lpc.h"
#define NDEBUG
#include <internal/debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

NTSTATUS
NTAPI
LpcpCreatePort(OUT PHANDLE PortHandle,
               IN POBJECT_ATTRIBUTES ObjectAttributes,
               IN ULONG MaxConnectionInfoLength,
               IN ULONG MaxMessageLength,
               IN ULONG MaxPoolUsage,
               IN BOOLEAN Waitable)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtCreatePort(OUT PHANDLE PortHandle,
             IN POBJECT_ATTRIBUTES ObjectAttributes,
             IN ULONG MaxConnectInfoLength,
             IN ULONG MaxDataLength,
             IN ULONG MaxPoolUsage)
{
    PAGED_CODE();

    /* Call the internal API */
    return LpcpCreatePort(PortHandle,
                          ObjectAttributes,
                          MaxConnectInfoLength,
                          MaxDataLength,
                          MaxPoolUsage,
                          FALSE);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtCreateWaitablePort(OUT PHANDLE PortHandle,
                     IN POBJECT_ATTRIBUTES ObjectAttributes,
                     IN ULONG MaxConnectInfoLength,
                     IN ULONG MaxDataLength,
                     IN ULONG MaxPoolUsage)
{
    PAGED_CODE();

    /* Call the internal API */
    return LpcpCreatePort(PortHandle,
                          ObjectAttributes,
                          MaxConnectInfoLength,
                          MaxDataLength,
                          MaxPoolUsage,
                          TRUE);
}

/* EOF */
