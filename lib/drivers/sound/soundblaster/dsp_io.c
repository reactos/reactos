/*
    ReactOS Sound System
    Sound Blaster DSP support
    General I/O

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        2 July 2008 - Created (split from sbdsp.c)

    Notes:
        Functions documented in sbdsp.h
*/

#include <ntddk.h>
#include <debug.h>

#include <time.h>
#include <sbdsp.h>

NTSTATUS
SbDspReset(
    IN  PUCHAR BasePort,
    IN  ULONG Timeout)
{
    ULONG Expiry;
    KIRQL CurrentIrqLevel = KeGetCurrentIrql();
    BOOLEAN DataAvailable = FALSE;

    /* Should be called from DriverEntry with this IRQL */
    ASSERT(CurrentIrqLevel == PASSIVE_LEVEL);

    WRITE_SB_DSP_RESET(BasePort, 0x01);
    SleepMs(50);   /* Should be enough */
    WRITE_SB_DSP_RESET(BasePort, 0x00);

    Expiry = QuerySystemTimeMs() + Timeout;

    /* Wait for data to be available */
    while ( (QuerySystemTimeMs() < Expiry) || ( Timeout == 0) )
    {
        if ( SB_DSP_DATA_AVAILABLE(BasePort) )
        {
            DataAvailable = TRUE;
            break;
        }
    }

    if ( ! DataAvailable )
    {
        return STATUS_TIMEOUT;
    }

    /* Data is available - wait for the "DSP ready" code */
    while ( (QuerySystemTimeMs() < Expiry) || ( Timeout == 0) )
    {
        if ( READ_SB_DSP_DATA(BasePort) == SB_DSP_READY )
        {
            return STATUS_SUCCESS;
        }
    }

    return STATUS_TIMEOUT;
}

NTSTATUS
SbDspWaitToWrite(
    IN  PUCHAR BasePort,
    IN  ULONG Timeout)
{
    ULONG Expiry = QuerySystemTimeMs() + Timeout;

    while ( (QuerySystemTimeMs() < Expiry) || (Timeout == 0) )
    {
        if ( SB_DSP_CLEAR_TO_SEND(BasePort) )
        {
            return STATUS_SUCCESS;
        }
    }

    return STATUS_TIMEOUT;
}

NTSTATUS
SbDspWaitToRead(
    IN  PUCHAR BasePort,
    IN  ULONG Timeout)
{
    ULONG Expiry = QuerySystemTimeMs() + Timeout;

    while ( (QuerySystemTimeMs() < Expiry) || (Timeout == 0) )
    {
        if ( SB_DSP_DATA_AVAILABLE(BasePort) )
        {
            return STATUS_SUCCESS;
        }
    }

    return STATUS_TIMEOUT;
}

NTSTATUS
SbDspWrite(
    IN  PUCHAR BasePort,
    IN  UCHAR DataByte,
    IN  ULONG Timeout)
{
    NTSTATUS Status;

    Status = SbDspWaitToWrite(BasePort, Timeout);

    if ( Status != STATUS_SUCCESS )
    {
        return Status;
    }

    DbgPrint("SBDSP - Writing %02x\n", DataByte);
    WRITE_SB_DSP_DATA(BasePort, DataByte);

    return STATUS_SUCCESS;
}

NTSTATUS
SbDspRead(
    IN  PUCHAR BasePort,
    OUT PUCHAR DataByte,
    IN  ULONG Timeout)
{
    NTSTATUS Status;

    if ( ! DataByte )
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    Status = SbDspWaitToRead(BasePort, Timeout);

    if ( Status != STATUS_SUCCESS )
    {
        return Status;
    }

    *DataByte = READ_SB_DSP_DATA(BasePort);
    DbgPrint("SBDSP - Read %02x\n", *DataByte);

    return STATUS_SUCCESS;
}
