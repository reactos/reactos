/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/generic/usage.c
 * PURPOSE:         HAL Resource Report Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan.ginsberg@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PUCHAR KdComPortInUse;
PADDRESS_USAGE HalpAddressUsageList;
IDTUsageFlags HalpIDTUsageFlags[MAXIMUM_IDTVECTOR];
IDTUsage HalpIDTUsage[MAXIMUM_IDTVECTOR];

ADDRESS_USAGE HalpDefaultIoSpace =
{
    NULL, CmResourceTypePort, IDT_INTERNAL,
    {
        {0x2000,  0xC000}, /* PIC?? */
        {0xC000,  0x1000}, /* DMA 2 */
        {0x8000,  0x1000}, /* DMA 1 */
        {0x2000,  0x200},  /* PIC 1 */
        {0xA000,  0x200},  /* PIC 2 */
        {0x4000,  0x400},  /* PIT 1 */
        {0x4800,  0x400},  /* PIT 2 */
        {0x9200,  0x100},  /* ????? */
        {0x7000,  0x200},  /* CMOS  */
        {0xF000,  0x1000}, /* ????? */
        {0xCF800, 0x800},  /* PCI 0 */
        {0,0},
    }
};

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
HalpReportResourceUsage(IN PUNICODE_STRING HalName,
                        IN INTERFACE_TYPE InterfaceType)
{
    DbgPrint("%wZ has been initialized\n", HalName);
}

VOID
NTAPI
HalpRegisterVector(IN UCHAR Flags,
                   IN ULONG BusVector,
                   IN ULONG SystemVector,
                   IN KIRQL Irql)
{
    /* Save the vector flags */
    HalpIDTUsageFlags[SystemVector].Flags = Flags;

    /* Save the vector data */
    HalpIDTUsage[SystemVector].Irql  = Irql;
    HalpIDTUsage[SystemVector].BusReleativeVector = BusVector;
}

#ifndef _MINIHAL_
VOID
NTAPI
HalpEnableInterruptHandler(IN UCHAR Flags,
                           IN ULONG BusVector,
                           IN ULONG SystemVector,
                           IN KIRQL Irql,
                           IN PVOID Handler,
                           IN KINTERRUPT_MODE Mode)
{
    UCHAR Entry;

    /* Convert the vector into the IDT entry */
    Entry = HalVectorToIDTEntry(SystemVector);

    /* Register the vector */
    HalpRegisterVector(Flags, BusVector, SystemVector, Irql);

    /* Connect the interrupt */
    ((PKIPCR)KeGetPcr())->IDT[Entry].ExtendedOffset = (USHORT)(((ULONG_PTR)Handler >> 16) & 0xFFFF);
    ((PKIPCR)KeGetPcr())->IDT[Entry].Offset = (USHORT)((ULONG_PTR)Handler);

    /* Enable the interrupt */
    HalEnableSystemInterrupt(SystemVector, Irql, Mode);
}
#endif

/*
 * @unimplemented
 */
VOID
NTAPI
HalReportResourceUsage(VOID)
{
    INTERFACE_TYPE InterfaceType;
    UNICODE_STRING HalString;

    /* FIXME: Initialize DMA 64-bit support */

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

    /* FIXME: Setup PCI debugging and Hibernation */
}
