/*
    ReactOS Sound System
    Sound Blaster DSP support

    Author:
        Andrew Greenwood (andrew.greenwood@silverblade.co.uk)

    History:
        26 May 2008 - Created

    Notes:
        ...
*/

#include <ntddk.h>

#include <time.h>
#include <sbdsp.h>

BOOLEAN
ResetSoundBlasterDSP(
    IN  PUCHAR BasePort,
    IN  ULONG Timeout)
{
    ULONG Expiry;
    KTIMER Timer;
    KIRQL CurrentIrqLevel = KeGetCurrentIrql();

    /* Should be called from DriverEntry with this IRQL */
    ASSERT(CurrentIrqLevel == PASSIVE_LEVEL);

    KeInitializeTimer(&Timer);

    WRITE_SB_DSP_RESET(BasePort, 0x01);
    SleepMs(50);   /* Should be enough */
    WRITE_SB_DSP_RESET(BasePort, 0x00);

    Expiry = QuerySystemTimeMs() + Timeout;

    while ( (QuerySystemTimeMs() < Expiry) || ( Timeout == 0) )
    {
        if ( READ_SB_DSP_DATA(BasePort) == SB_DSP_READY )
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN
WaitForSoundBlasterDSPReady(
    IN  PUCHAR BasePort,
    IN  ULONG Timeout)
{
    ULONG Expiry = QuerySystemTimeMs() + Timeout;

    while ( (QuerySystemTimeMs() < Expiry) || (Timeout == 0) )
    {
        // ...
    }

    return FALSE;
}

NTSTATUS
GetSoundBlasterDSPVersion(
    IN  PUCHAR BasePort,
    OUT PUCHAR MajorVersion,
    OUT PUCHAR MinorVersion,
    IN  ULONG Timeout)
{
    /* TODO */
    return STATUS_NOT_SUPPORTED;
}
