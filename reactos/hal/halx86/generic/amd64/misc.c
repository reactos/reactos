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
KAFFINITY HalpActiveProcessors;

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
HalpInitIdtEntry(PKIDTENTRY64 Idt, PVOID Address)
{
    Idt->OffsetLow = (ULONG_PTR)Address & 0xffff;
    Idt->OffsetMiddle = ((ULONG_PTR)Address >> 16) & 0xffff;
    Idt->OffsetHigh = (ULONG_PTR)Address >> 32;
    Idt->Selector = KGDT64_R0_CODE;
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

VOID
NTAPI
HalBugCheckSystem(
  IN PWHEA_ERROR_RECORD ErrorRecord)
{
    UNIMPLEMENTED;
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

VOID
NTAPI
HalDisableSystemInterrupt(
    UCHAR Vector,
    KIRQL Irql)
{
    UNIMPLEMENTED;
}

BOOLEAN
NTAPI
HalEnableSystemInterrupt(
    UCHAR Vector,
    KIRQL Irql,
    KINTERRUPT_MODE InterruptMode)
{
    UNIMPLEMENTED;
    return FALSE;
}

VOID
NTAPI
HalpCalibrateStallExecution(VOID)
{
    UNIMPLEMENTED;
}

