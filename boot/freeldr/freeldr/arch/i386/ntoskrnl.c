/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/freeldr/arch/i386/ntoskrnl.c
 * PURPOSE:         NTOS glue routines for the MINIHAL library
 * PROGRAMMERS:     Hervé Poussineau  <hpoussin@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>

/* For KeStallExecutionProcessor */
#if defined(_M_IX86) || defined(_M_AMD64)
#include <arch/pc/pcbios.h>
#endif

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KeInitializeEvent(
    IN PRKEVENT Event,
    IN EVENT_TYPE Type,
    IN BOOLEAN State)
{
    memset(Event, 0, sizeof(*Event));
}

VOID
FASTCALL
KiAcquireSpinLock(
    IN PKSPIN_LOCK SpinLock)
{
}

VOID
FASTCALL
KiReleaseSpinLock(
    IN PKSPIN_LOCK SpinLock)
{
}

VOID
NTAPI
KeSetTimeIncrement(
    IN ULONG MaxIncrement,
    IN ULONG MinIncrement)
{
}

VOID
FASTCALL
IoAssignDriveLetters(
    IN struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
    IN PSTRING NtDeviceName,
    OUT PUCHAR NtSystemPath,
    OUT PSTRING NtSystemPathString)
{
}

NTSTATUS
FASTCALL
IoSetPartitionInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG PartitionNumber,
    IN ULONG PartitionType)
{
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * NTSTATUS
 * FASTCALL
 * IoReadPartitionTable(
 *     IN PDEVICE_OBJECT DeviceObject,
 *     IN ULONG SectorSize,
 *     IN BOOLEAN ReturnRecognizedPartitions,
 *     OUT PDRIVE_LAYOUT_INFORMATION *PartitionBuffer);
 *
 * See boot/freeldr/freeldr/disk/partition.c
 */

NTSTATUS
FASTCALL
IoWritePartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG SectorsPerTrack,
    IN ULONG NumberOfHeads,
    IN PDRIVE_LAYOUT_INFORMATION PartitionBuffer)
{
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
KeStallExecutionProcessor(
    IN ULONG MicroSeconds)
{
#if defined(_M_IX86) || defined(_M_AMD64)
    REGS Regs;
    ULONG usec_this;

    // Int 15h AH=86h
    // BIOS - WAIT (AT,PS)
    //
    // AH = 86h
    // CX:DX = interval in microseconds
    // Return:
    // CF clear if successful (wait interval elapsed)
    // CF set on error or AH=83h wait already in progress
    // AH = status (see #00496)

    // Note: The resolution of the wait period is 977 microseconds on
    // many systems because many BIOSes use the 1/1024 second fast
    // interrupt from the AT real-time clock chip which is available on INT 70;
    // because newer BIOSes may have much more precise timers available, it is
    // not possible to use this function accurately for very short delays unless
    // the precise behavior of the BIOS is known (or found through testing)

    while (MicroSeconds)
    {
        usec_this = MicroSeconds;

        if (usec_this > 4000000)
        {
            usec_this = 4000000;
        }

        Regs.b.ah = 0x86;
        Regs.w.cx = usec_this >> 16;
        Regs.w.dx = usec_this & 0xffff;
        Int386(0x15, &Regs, &Regs);

        MicroSeconds -= usec_this;
    }
#else
    #error unimplemented
#endif
}
