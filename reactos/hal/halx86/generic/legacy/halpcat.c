/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            hal/halx86/generic/acpi/halpcat.c
 * PURPOSE:         HAL Legacy Support Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* PRIVATE FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
HalpSetupAcpiPhase0(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* There is no ACPI on these HALs */
    return STATUS_NO_SUCH_DEVICE;
}

VOID
NTAPI
HalpInitializePciBus(VOID)
{
    /* FIXME: Should do legacy PCI bus detection */
    
    /* FIXME: Should detect chipset hacks */
    
    /* FIXME: Should detect broken PCI hardware and apply hacks */
    
    /* FIXME: Should build resource ranges */
}

/*
 * @implemented
 */
VOID
NTAPI
HalReportResourceUsage(VOID)
{
    INTERFACE_TYPE InterfaceType;
    UNICODE_STRING HalString;

    /* FIXME: Initialize MCA bus */

    /* Initialize PCI bus. */
    HalpInitializePciBus();

    /* Initialize the stubs */
    HalpInitializePciStubs();

    /* What kind of bus is this? */
    switch (HalpBusType)
    {
        /* ISA Machine */
        case MACHINE_TYPE_ISA:
            InterfaceType = Isa;
            break;

        /* EISA Machine */
        case MACHINE_TYPE_EISA:
            InterfaceType = Eisa;
            break;

        /* MCA Machine */
        case MACHINE_TYPE_MCA:
            InterfaceType = MicroChannel;
            break;

        /* Unknown */
        default:
            InterfaceType = Internal;
            break;
    }

    /* Build HAL usage */
    RtlInitUnicodeString(&HalString, L"PC Compatible Eisa/Isa HAL");
    HalpReportResourceUsage(&HalString, InterfaceType);

    /* Setup PCI debugging and Hibernation */
    HalpRegisterPciDebuggingDeviceInfo();
}

/* EOF */
