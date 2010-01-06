/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/generic/misc.c
 * PURPOSE:         Miscellanous Routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Eric Kohl (ekohl@abo.rhein-zeitung.de)
 *                  Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

LARGE_INTEGER HalpPerformanceFrequency;

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
HalpInitIdtEntry(PKIDTENTRY64 Idt, PVOID Address)
{
    Idt->OffsetLow = (ULONG_PTR)Address & 0xffff;
    Idt->OffsetMiddle = ((ULONG_PTR)Address >> 16) & 0xffff;
    Idt->OffsetHigh = (ULONG_PTR)Address >> 32;
    Idt->Selector = KGDT_64_R0_CODE;
    Idt->IstIndex = 0;
    Idt->Type = 0x0e;
    Idt->Dpl = 0;
    Idt->Present = 1;
    Idt->Reserved0 = 0;
    Idt->Reserved1 = 0;
}

VOID
NTAPI
HalpSetInterruptGate(ULONG Index, PVOID Address)
{
    ULONG_PTR Flags;

    /* Disable interupts */
    Flags = __readeflags();
    _disable();

    /* Initialize the entry */
    HalpInitIdtEntry(&KeGetPcr()->IdtBase[Index], Address);

    /* Enable interrupts if they were enabled previously */
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
    return FALSE;
}

VOID
NTAPI
HalBugCheckSystem (PVOID ErrorRecord)
{
  UNIMPLEMENTED;
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

LARGE_INTEGER
NTAPI
KeQueryPerformanceCounter(
    OUT PLARGE_INTEGER PerformanceFrequency OPTIONAL)
{
    LARGE_INTEGER Result;

//    ASSERT(HalpPerformanceFrequency.QuadPart != 0);

    /* Does the caller want the frequency? */
    if (PerformanceFrequency)
    {
        /* Return value */
        *PerformanceFrequency = HalpPerformanceFrequency;
    }

    Result.QuadPart = __rdtsc();
    return Result;
}

