/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/intrface/tr_irq.c
 * PURPOSE:         IRQ Translator Interface
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PCI_INTERFACE TranslatorInterfaceInterrupt =
{
    &GUID_TRANSLATOR_INTERFACE_STANDARD,
    sizeof(TRANSLATOR_INTERFACE),
    0,
    0,
    PCI_INTERFACE_FDO,
    0,
    PciTrans_Interrupt,
    tranirq_Constructor,
    tranirq_Initializer
};

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
tranirq_Initializer(IN PVOID Instance)
{
    UNREFERENCED_PARAMETER(Instance);
    /* PnP Interfaces don't get Initialized */
    ASSERTMSG("PCI tranirq_Initializer, unexpected call.\n", FALSE);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
tranirq_Constructor(IN PVOID DeviceExtension,
                    IN PVOID Instance,
                    IN PVOID InterfaceData,
                    IN USHORT Version,
                    IN USHORT Size,
                    IN PINTERFACE Interface)
{
    PPCI_FDO_EXTENSION FdoExtension = (PPCI_FDO_EXTENSION)DeviceExtension;
    ULONG BaseBus, ParentBus;
    INTERFACE_TYPE ParentInterface;
    ASSERT_FDO(FdoExtension);

    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Version);
    UNREFERENCED_PARAMETER(Size);

    /* Make sure it's the right resource type */
    if ((ULONG_PTR)InterfaceData != CmResourceTypeInterrupt)
    {
        /* Fail this invalid request */
        DPRINT1("PCI - IRQ trans constructor doesn't like %p in InterfaceSpecificData\n",
                InterfaceData);
        return STATUS_INVALID_PARAMETER_3;
    }

    /* Get the bus, and use this as the interface-specific data */
    BaseBus = FdoExtension->BaseBus;
    InterfaceData = UlongToPtr(BaseBus);

    /* Check if this is the root bus */
    if (PCI_IS_ROOT_FDO(FdoExtension))
    {
        /* It is, so there is no parent, and it's connected on the system bus */
        ParentBus = 0;
        ParentInterface = Internal;
        DPRINT1("      Is root FDO\n");
    }
    else
    {
        /* It's not, so we have to get the root bus' bus number instead */
        #if 0 // when have PDO commit
        ParentBus = FdoExtension->PhysicalDeviceObject->DeviceExtension->ParentFdoExtension->BaseBus;
        ParentInterface = PCIBus;
        DPRINT1("      Is bridge FDO, parent bus %x, secondary bus %x\n",
                ParentBus, BaseBus);
        #endif
    }

    /* Now call the legacy HAL interface to get the correct translator */
    return HalGetInterruptTranslator(ParentInterface,
                                     ParentBus,
                                     PCIBus,
                                     sizeof(TRANSLATOR_INTERFACE),
                                     0,
                                     (PTRANSLATOR_INTERFACE)Interface,
                                     (PULONG)&InterfaceData);
}

/* EOF */
