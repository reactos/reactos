/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            hal/halx86/generic/legacy/halpcat.c
 * PURPOSE:         HAL Legacy Support Code
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

#if defined(ALLOC_PRAGMA) && !defined(_MINIHAL_)
//#pragma alloc_text(INIT, HaliInitPnpDriver)
#pragma alloc_text(INIT, HalpBuildAddressMap)
#pragma alloc_text(INIT, HalpGetDebugPortTable)
#pragma alloc_text(INIT, HalpIs16BitPortDecodeSupported)
#pragma alloc_text(INIT, HalpSetupAcpiPhase0)
#pragma alloc_text(INIT, HalReportResourceUsage)
#endif

/* GLOBALS ********************************************************************/

/* This determines the HAL type */
BOOLEAN HalDisableFirmwareMapper = FALSE;
PWCHAR HalHardwareIdString = L"e_isa_up";
PWCHAR HalName = L"PC Compatible Eisa/Isa HAL";

/* PRIVATE FUNCTIONS **********************************************************/

INIT_SECTION
NTSTATUS
NTAPI
HalpSetupAcpiPhase0(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* There is no ACPI on these HALs */
    return STATUS_NO_SUCH_DEVICE;
}

INIT_SECTION
VOID
NTAPI
HalpBuildAddressMap(VOID)
{
    /* FIXME: Inherit ROM blocks from the registry */
    //HalpInheritROMBlocks();
    
    /* FIXME: Add the ROM blocks to our ranges */
    //HalpAddROMRanges();
}

INIT_SECTION
BOOLEAN
NTAPI
HalpGetDebugPortTable(VOID)
{
    /* No ACPI */
    return FALSE;
}

INIT_SECTION
ULONG
NTAPI
HalpIs16BitPortDecodeSupported(VOID)
{
    /* Only EISA systems support this */
    return (HalpBusType == MACHINE_TYPE_EISA) ? CM_RESOURCE_PORT_16_BIT_DECODE : 0;
}

#if 0
INIT_SECTION
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
INIT_SECTION
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
