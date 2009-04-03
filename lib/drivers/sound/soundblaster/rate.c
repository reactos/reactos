/*
    ReactOS Sound System
    Sound Blaster DSP support
    Sample rate routines

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

BOOLEAN
SbDspIsValidInputRate(
    IN  UCHAR MajorVersion,
    IN  UCHAR MinorVersion,
    IN  USHORT Rate,
    IN  BOOLEAN Stereo)
{
    if ( MajorVersion == 1 )
    {
        if ( Stereo )
            return FALSE;

        return ( ( Rate >= 4000 ) && ( Rate <= 13000 ) );
    }
    else if ( MajorVersion == 2 )
    {
        if ( Stereo )
            return FALSE;

        if ( MinorVersion == 0 )
            return ( ( Rate >= 4000 ) && ( Rate <= 15000 ) );
        else
            return ( ( Rate >= 4000 ) && ( Rate <= 44100 ) );
    }
    else if ( MajorVersion == 3 )
    {
        if ( Stereo )
            return FALSE;

        return ( ( Rate >= 4000 ) && ( Rate <= 13000 ) );
    }
    else /* 4.00 and above */
    {
        return ( ( Rate >= 5000 ) && ( Rate <= 44100 ) );
    }
}

BOOLEAN
SbDspIsValidOutputRate(
    IN  UCHAR MajorVersion,
    IN  UCHAR MinorVersion,
    IN  USHORT Rate,
    IN  BOOLEAN Stereo)
{
    if ( MajorVersion == 1 )
    {
        if ( Stereo )
            return FALSE;

        return ( ( Rate >= 4000 ) && ( Rate <= 23000 ) );
    }
    else if ( MajorVersion == 2 )
    {
        if ( Stereo )
            return FALSE;

        if ( MinorVersion == 0 )
            return ( ( Rate >= 4000 ) && ( Rate <= 23000 ) );
        else
            return ( ( Rate >= 4000 ) && ( Rate <= 44100 ) );
    }
    else if ( MajorVersion == 3 )
    {
        if ( ! Stereo )
            return ( ( Rate >= 4000 ) && ( Rate <= 44100 ) );
        else
            return ( ( Rate >= 11025 ) && ( Rate <= 22050 ) );
    }
    else /* 4.00 and above */
    {
        return ( ( Rate >= 5000 ) && ( Rate <= 44100 ) );
    }
}

/* Internal routine - call only after submitting one of the rate commands */
NTSTATUS
SbDsp4WriteRate(
    IN  PUCHAR BasePort,
    IN  USHORT Rate,
    IN  ULONG Timeout)
{
    NTSTATUS Status;

    /* NOTE - No check for validity of rate! */

    /* Write high byte */
    Status = SbDspWrite(BasePort, (Rate & 0xff00) >> 8, Timeout);
    if ( Status != STATUS_SUCCESS )
        return Status;

    /* Write low byte */
    Status = SbDspWrite(BasePort, Rate & 0xff, Timeout);
    if ( Status != STATUS_SUCCESS )
        return Status;

    return Status;
}

NTSTATUS
SbDsp4SetOutputRate(
    IN  PUCHAR BasePort,
    IN  USHORT Rate,
    IN  ULONG Timeout)
{
    NTSTATUS Status;

    /* NOTE - No check for validity of rate! */

    /* Prepare to write the output rate */
    Status = SbDspWrite(BasePort, SB_DSP_OUTPUT_RATE, (Rate & 0xff00) >> 8);
    if ( Status != STATUS_SUCCESS )
        return Status;

    return SbDsp4WriteRate(BasePort, Rate, Timeout);
}

NTSTATUS
SbDsp4SetInputRate(
    IN  PUCHAR BasePort,
    IN  USHORT Rate,
    IN  ULONG Timeout)
{
    NTSTATUS Status;

    /* NOTE - No check for validity of rate! */

    /* Prepare to write the input rate */
    Status = SbDspWrite(BasePort, SB_DSP_OUTPUT_RATE, (Rate & 0xff00) >> 8);
    if ( Status != STATUS_SUCCESS )
        return Status;

    return SbDsp4WriteRate(BasePort, Rate, Timeout);
}
 
