/*
    ReactOS Sound System
    Sound Blaster DSP support

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        26 May 2008 - Created

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

    if ( ! NT_SUCCESS(Status) )
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

    if ( ! NT_SUCCESS(Status) )
    {
        return Status;
    }

    *DataByte = READ_SB_DSP_DATA(BasePort);
    DbgPrint("SBDSP - Read %02x\n", *DataByte);

    return STATUS_SUCCESS;
}

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
    if ( ! NT_SUCCESS(Status) )
        return Status;

    /* Get the major version */
    Status = SbDspRead(BasePort, MajorVersion, Timeout);
    if ( ! NT_SUCCESS(Status) )
        return FALSE;

    /* Get the minor version */
    Status = SbDspRead(BasePort, MinorVersion, Timeout);
    return Status;
}

NTSTATUS
SbDspEnableSpeaker(
    IN  PUCHAR BasePort,
    IN  ULONG Timeout)
{
    return SbDspWrite(BasePort, SB_DSP_SPEAKER_ON, Timeout);
}

NTSTATUS
SbDspDisableSpeaker(
    IN  PUCHAR BasePort,
    IN  ULONG Timeout)
{
    return SbDspWrite(BasePort, SB_DSP_SPEAKER_OFF, Timeout);
}

/*
    BUG?
    It seems under VirtualBox this returns 0x05, irrespective
    of the speaker state. I'm not sure if this will also occur
    on real hardware.
*/
NTSTATUS
SbDspIsSpeakerEnabled(
    IN  PUCHAR BasePort,
    OUT PBOOLEAN IsEnabled,
    IN  ULONG Timeout)
{
    NTSTATUS Status;
    UCHAR SpeakerStatus = 0;

    if ( ! IsEnabled )
        return STATUS_INVALID_PARAMETER_2;

    /* Request the speaker status */
    Status = SbDspWrite(BasePort, SB_DSP_SPEAKER_STATUS, Timeout);
    if ( ! NT_SUCCESS(Status) )
        return Status;

    /* Obtain the status */
    Status = SbDspRead(BasePort, &SpeakerStatus, Timeout);
    if ( ! NT_SUCCESS(Status) )
        return Status;

    DbgPrint("SBDSP - SpeakerStatus is %02x\n", SpeakerStatus);
    *IsEnabled = (SpeakerStatus == 0xFF);

    return STATUS_SUCCESS;
}

BOOLEAN
SbDspIsValidRate(
    IN  USHORT Rate)
{
    /* Not sure if this range is 100% correct */
    return ( ( Rate >= 5000 ) && ( Rate <= 45000 ) );
}

/* Internal routine - call only after submitting one of the rate commands */
NTSTATUS
SbDspWriteRate(
    IN  PUCHAR BasePort,
    IN  USHORT Rate,
    IN  ULONG Timeout)
{
    NTSTATUS Status;

    if ( ! SbDspIsValidRate(Rate) )
        return STATUS_INVALID_PARAMETER_2;

    /* Write high byte */
    Status = SbDspWrite(BasePort, (Rate & 0xff00) >> 8, Timeout);
    if ( ! NT_SUCCESS(Status) )
        return Status;

    /* Write low byte */
    Status = SbDspWrite(BasePort, Rate & 0xff, Timeout);
    if ( ! NT_SUCCESS(Status) )
        return Status;

    return Status;
}

NTSTATUS
SbDspSetOutputRate(
    IN  PUCHAR BasePort,
    IN  USHORT Rate,
    IN  ULONG Timeout)
{
    NTSTATUS Status;

    if ( ! SbDspIsValidRate(Rate) )
        return STATUS_INVALID_PARAMETER_2;

    /* Prepare to write the output rate */
    Status = SbDspWrite(BasePort, SB_DSP_OUTPUT_RATE, (Rate & 0xff00) >> 8);
    if ( ! NT_SUCCESS(Status) )
        return Status;

    return SbDspWriteRate(BasePort, Rate, Timeout);
}

NTSTATUS
SbDspSetInputRate(
    IN  PUCHAR BasePort,
    IN  USHORT Rate,
    IN  ULONG Timeout)
{
    NTSTATUS Status;

    if ( ! SbDspIsValidRate(Rate) )
        return STATUS_INVALID_PARAMETER_2;

    /* Prepare to write the input rate */
    Status = SbDspWrite(BasePort, SB_DSP_OUTPUT_RATE, (Rate & 0xff00) >> 8);
    if ( ! NT_SUCCESS(Status) )
        return Status;

    return SbDspWriteRate(BasePort, Rate, Timeout);
}
