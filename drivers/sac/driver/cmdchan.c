/*
 * PROJECT:     ReactOS Drivers
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        drivers/sac/driver/cmdchan.c
 * PURPOSE:     Driver for the Server Administration Console (SAC) for EMS
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include "sacdrv.h"

/* FUNCTIONS ******************************************************************/

NTSTATUS
CmdChannelCreate(
    IN PSAC_CHANNEL Channel
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
CmdChannelDestroy(
    IN PSAC_CHANNEL Channel
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
CmdChannelOWrite(
    IN PSAC_CHANNEL Channel,
    IN PWCHAR String,
    IN ULONG Size
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
CmdChannelOFlush(
    IN PSAC_CHANNEL Channel
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
CmdChannelORead(
    IN PSAC_CHANNEL Channel,
    IN PCHAR Buffer,
    IN ULONG BufferSize,
    OUT PULONG ByteCount
    )
{
    return STATUS_NOT_IMPLEMENTED;
}
