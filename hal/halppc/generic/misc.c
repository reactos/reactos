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
BOOLEAN
FASTCALL
HalSystemVectorDispatchEntry(IN ULONG Vector,
                             OUT PKINTERRUPT_ROUTINE **FlatDispatch,
                             OUT PKINTERRUPT_ROUTINE *NoConnection)
{
    /* Not implemented on x86 */
    return FALSE;
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
