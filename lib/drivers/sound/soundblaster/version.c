/*
    ReactOS Sound System
    Sound Blaster DSP support
    Version routine

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        2 July 2008 - Created (split from sbdsp.c)

    Notes:
        Functions documented in sbdsp.h
*/

#include <ntddk.h>
#include <debug.h>

#include <sbdsp.h>

NTSTATUS
SbDspGetVersion(
    IN  PUCHAR BasePort,
    OUT PUCHAR MajorVersion,
    OUT PUCHAR MinorVersion,
    IN  ULONG Timeout)
{
    NTSTATUS Status;

    /* Make sure our parameters are sane */
    if ( ! MajorVersion )
        return STATUS_INVALID_PARAMETER_2;

    if ( ! MinorVersion )
        return STATUS_INVALID_PARAMETER_3;

    /* Send version request */
    Status = SbDspWrite(BasePort, SB_DSP_VERSION, Timeout);
    if ( Status != STATUS_SUCCESS )
        return Status;

    /* Get the major version */
    Status = SbDspRead(BasePort, MajorVersion, Timeout);
    if ( Status != STATUS_SUCCESS )
        return FALSE;

    /* Get the minor version */
    Status = SbDspRead(BasePort, MinorVersion, Timeout);
    return Status;
}
