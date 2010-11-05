#include <ntoskrnl.h>
#define NDEBUG
#include <arch.h>

VOID
NTAPI
KeInitializeEvent(
    IN PRKEVENT Event,
    IN EVENT_TYPE Type,
    IN BOOLEAN State)
{
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

NTKERNELAPI
VOID
FASTCALL
IoAssignDriveLetters(
    IN struct _LOADER_PARAMETER_BLOCK *LoaderBlock,
    IN PSTRING NtDeviceName,
    OUT PUCHAR NtSystemPath,
    OUT PSTRING NtSystemPathString)
{
}

NTKERNELAPI
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

NTKERNELAPI
NTSTATUS
FASTCALL
IoWritePartitionTable(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN ULONG SectorsPerTrack,
    IN ULONG NumberOfHeads,
    IN struct _DRIVE_LAYOUT_INFORMATION *PartitionBuffer)
{
    return STATUS_NOT_IMPLEMENTED;
}

NTHALAPI
VOID
NTAPI
KeStallExecutionProcessor(
    IN ULONG MicroSeconds)
{
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
}
