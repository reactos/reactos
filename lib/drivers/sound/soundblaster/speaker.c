/*
    ReactOS Sound System
    Sound Blaster DSP support
    Speaker commands

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
    VirtualBox doesn't seem to support this.
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
    if ( Status != STATUS_SUCCESS )
        return Status;

    /* Obtain the status */
    Status = SbDspRead(BasePort, &SpeakerStatus, Timeout);
    if ( Status != STATUS_SUCCESS )
        return Status;

    DbgPrint("SBDSP - SpeakerStatus is %02x\n", SpeakerStatus);
    *IsEnabled = (SpeakerStatus == 0xFF);

    return STATUS_SUCCESS;
}
