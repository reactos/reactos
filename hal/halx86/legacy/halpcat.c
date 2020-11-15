/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * PURPOSE:         HAL Legacy Support Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <hal.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* This determines the HAL type */
BOOLEAN HalDisableFirmwareMapper = FALSE;
#if defined(SARCH_XBOX)
PWCHAR HalHardwareIdString = L"xbox";
PWCHAR HalName = L"Xbox HAL";
#elif defined(SARCH_PC98)
PWCHAR HalHardwareIdString = L"pc98_up";
PWCHAR HalName = L"NEC PC-98 Compatible NESA/C-Bus HAL";
#else
PWCHAR HalHardwareIdString = L"e_isa_up";
PWCHAR HalName = L"PC Compatible Eisa/Isa HAL";
#endif

/* PRIVATE FUNCTIONS **********************************************************/

CODE_SEG("INIT")
NTSTATUS
NTAPI
HalpSetupAcpiPhase0(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* There is no ACPI on these HALs */
    return STATUS_NO_SUCH_DEVICE;
}

CODE_SEG("INIT")
VOID
NTAPI
HalpBuildAddressMap(VOID)
{
    /* FIXME: Inherit ROM blocks from the registry */
    //HalpInheritROMBlocks();

    /* FIXME: Add the ROM blocks to our ranges */
    //HalpAddROMRanges();
}

CODE_SEG("INIT")
BOOLEAN
NTAPI
HalpGetDebugPortTable(VOID)
{
    /* No ACPI */
    return FALSE;
}

CODE_SEG("INIT")
ULONG
NTAPI
HalpIs16BitPortDecodeSupported(VOID)
{
    /* Only EISA systems support this */
    return (HalpBusType == MACHINE_TYPE_EISA) ? CM_RESOURCE_PORT_16_BIT_DECODE : 0;
}

#if 0
CODE_SEG("INIT")
NTSTATUS
NTAPI
HaliInitPnpDriver(VOID)
{
    /* On PC-AT, this will interface with the PCI driver */
    //HalpDebugPciBus();
    return STATUS_SUCCESS;
}
#endif

/*
 * @implemented
 */
CODE_SEG("INIT")
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
    RtlInitUnicodeString(&HalString, HalName);
    HalpReportResourceUsage(&HalString, InterfaceType);

    /* Setup PCI debugging and Hibernation */
    HalpRegisterPciDebuggingDeviceInfo();
}

/* EOF */
