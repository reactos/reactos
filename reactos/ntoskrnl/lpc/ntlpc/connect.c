/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/lpc/connect.c
 * PURPOSE:         Local Procedure Call: Connection Management
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
NtSecureConnectPort(OUT PHANDLE PortHandle,
                    IN PUNICODE_STRING PortName,
                    IN PSECURITY_QUALITY_OF_SERVICE Qos,
                    IN OUT PPORT_VIEW ClientView OPTIONAL,
                    IN PSID ServerSid OPTIONAL,
                    IN OUT PREMOTE_PORT_VIEW ServerView OPTIONAL,
                    OUT PULONG MaxMessageLength OPTIONAL,
                    IN OUT PVOID ConnectionInformation OPTIONAL,
                    IN OUT PULONG ConnectionInformationLength OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtConnectPort(OUT PHANDLE PortHandle,
              IN PUNICODE_STRING PortName,
              IN PSECURITY_QUALITY_OF_SERVICE Qos,
              IN PPORT_VIEW ClientView,
              IN PREMOTE_PORT_VIEW ServerView,
              OUT PULONG MaxMessageLength,
              IN PVOID ConnectionInformation,
              OUT PULONG ConnectionInformationLength)
{
    /* Call the newer API */
    return NtSecureConnectPort(PortHandle,
                               PortName,
                               Qos,
                               ClientView,
                               NULL,
                               ServerView,
                               MaxMessageLength,
                               ConnectionInformation,
                               ConnectionInformationLength);
}

/* EOF */
