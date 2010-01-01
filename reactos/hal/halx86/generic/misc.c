/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/generic/misc.c
 * PURPOSE:         Miscellanous Routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Eric Kohl (ekohl@abo.rhein-zeitung.de)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
HalpCheckPowerButton(VOID)
{
    /* Nothing to do on non-ACPI */
    return;
}

PVOID
NTAPI
HalpMapPhysicalMemory64(IN PHYSICAL_ADDRESS PhysicalAddress,
                        IN ULONG NumberPage)
{
    /* Use kernel memory manager I/O map facilities */
    return MmMapIoSpace(PhysicalAddress,
                        NumberPage << PAGE_SHIFT,
                        MmNonCached);
}

VOID
NTAPI
HalpUnmapVirtualAddress(IN PVOID VirtualAddress,
                        IN ULONG NumberPages)
{
    /* Use kernel memory manager I/O map facilities */
    MmUnmapIoSpace(VirtualAddress, NumberPages << PAGE_SHIFT);
}

VOID
NTAPI
HalpFlushTLB(VOID)
{
    ULONG Flags, Cr4;
    INT CpuInfo[4];
    ULONG_PTR PageDirectory;

    //
    // Disable interrupts
    //
    Flags = __readeflags();
    _disable();

    //
    // Get page table directory base
    //
    PageDirectory = __readcr3();

    //
    // Check for CPUID support
    //
    if (KeGetCurrentPrcb()->CpuID)
    {
        //
        // Check for global bit in CPU features
        //
        __cpuid(CpuInfo, 1);
        if (CpuInfo[3] & 0x2000)
        {
            //
            // Get current CR4 value
            //
            Cr4 = __readcr4();

            //
            // Disable global pit
            //
            __writecr4(Cr4 & ~CR4_PGE);

            //
            // Flush TLB and re-enable global bit
            //
            __writecr3(PageDirectory);
            __writecr4(Cr4);

            //
            // Restore interrupts
            //
            __writeeflags(Flags);
        }
    }

    //
    // Legacy: just flush TLB
    //
    __writecr3(PageDirectory);
    __writeeflags(Flags);
}

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID
NTAPI
HalHandleNMI(IN PVOID NmiInfo)
{
    UCHAR ucStatus;

    /* Get the NMI Flag */
    ucStatus = READ_PORT_UCHAR((PUCHAR)0x61);

    /* Display NMI failure string */
    HalDisplayString ("\n*** Hardware Malfunction\n\n");
    HalDisplayString ("Call your hardware vendor for support\n\n");

    /* Check for parity error */
    if (ucStatus & 0x80)
    {
        /* Display message */
        HalDisplayString ("NMI: Parity Check / Memory Parity Error\n");
    }

    /* Check for I/O failure */
    if (ucStatus & 0x40)
    {
        /* Display message */
        HalDisplayString ("NMI: Channel Check / IOCHK\n");
    }

    /* Halt the system */
    HalDisplayString("\n*** The system has halted ***\n");
    //KeEnterKernelDebugger();
}

/*
 * @implemented
 */
UCHAR
FASTCALL
HalSystemVectorDispatchEntry(IN ULONG Vector,
                             OUT PKINTERRUPT_ROUTINE **FlatDispatch,
                             OUT PKINTERRUPT_ROUTINE *NoConnection)
{
    /* Not implemented on x86 */
    return 0;
}

/*
 * @implemented
 */
VOID
NTAPI
KeFlushWriteBuffer(VOID)
{
    /* Not implemented on x86 */
    return;
}
