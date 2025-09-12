/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/device.c
 * PURPOSE:         Device Management
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
Device_SaveCurrentSettings(IN PPCI_CONFIGURATOR_CONTEXT Context)
{
    PPCI_COMMON_HEADER PciData;
    PIO_RESOURCE_DESCRIPTOR IoDescriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor;
    PPCI_FUNCTION_RESOURCES Resources;
    PULONG BarArray;
    ULONG Bar, BarMask, i;

    /* Get variables from context */
    PciData = Context->Current;
    Resources = Context->PdoExtension->Resources;

    /* Loop all the PCI BARs */
    BarArray = PciData->u.type0.BaseAddresses;
    for (i = 0; i <= PCI_TYPE0_ADDRESSES; i++)
    {
        /* Get the resource descriptor and limit descriptor for this BAR */
        CmDescriptor = &Resources->Current[i];
        IoDescriptor = &Resources->Limit[i];

        /* Build the resource descriptor based on the limit descriptor */
        CmDescriptor->Type = IoDescriptor->Type;
        if (CmDescriptor->Type == CmResourceTypeNull) continue;
        CmDescriptor->Flags = IoDescriptor->Flags;
        CmDescriptor->ShareDisposition = IoDescriptor->ShareDisposition;
        CmDescriptor->u.Generic.Start.HighPart = 0;
        CmDescriptor->u.Generic.Length = IoDescriptor->u.Generic.Length;

        /* Check if we're handling PCI BARs, or the ROM BAR */
        if (i < PCI_TYPE0_ADDRESSES)
        {
            /* Read the actual BAR value */
            Bar = BarArray[i];

            /* Check if this is an I/O BAR */
            if (Bar & PCI_ADDRESS_IO_SPACE)
            {
                /* Use the right mask to get the I/O port base address */
                ASSERT(CmDescriptor->Type == CmResourceTypePort);
                BarMask = PCI_ADDRESS_IO_ADDRESS_MASK;
            }
            else
            {
                /* It's a RAM BAR, use the right mask to get the base address */
                ASSERT(CmDescriptor->Type == CmResourceTypeMemory);
                BarMask = PCI_ADDRESS_MEMORY_ADDRESS_MASK;

                /* Check if it's a 64-bit BAR */
                if ((Bar & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_64BIT)
                {
                    /* The next BAR value is actually the high 32-bits */
                    CmDescriptor->u.Memory.Start.HighPart = BarArray[i + 1];
                }
                else if ((Bar & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_20BIT)
                {
                    /* Legacy BAR, don't read more than 20 bits of the address */
                    BarMask = 0xFFFF0;
                }
            }
        }
        else
        {
            /* Actually a ROM BAR, so read the correct register */
            Bar = PciData->u.type0.ROMBaseAddress;

            /* Apply the correct mask for ROM BARs */
            BarMask = PCI_ADDRESS_ROM_ADDRESS_MASK;

            /* Make sure it's enabled */
            if (!(Bar & PCI_ROMADDRESS_ENABLED))
            {
                /* If it isn't, then a descriptor won't be built for it */
                CmDescriptor->Type = CmResourceTypeNull;
                continue;
            }
        }

        /* Now we have the right mask, read the actual address from the BAR */
        Bar &= BarMask;
        CmDescriptor->u.Memory.Start.LowPart = Bar;

        /* And check for invalid BAR addresses */
        if (!(CmDescriptor->u.Memory.Start.HighPart | Bar))
        {
            /* Skip these descriptors */
            CmDescriptor->Type = CmResourceTypeNull;
            DPRINT1("Invalid BAR\n");
        }
    }

    /* Also save the sub-IDs that came directly from the PCI header */
    Context->PdoExtension->SubsystemVendorId = PciData->u.type0.SubVendorID;
    Context->PdoExtension->SubsystemId = PciData->u.type0.SubSystemID;
}

VOID
NTAPI
Device_SaveLimits(IN PPCI_CONFIGURATOR_CONTEXT Context)
{
    PPCI_COMMON_HEADER Current, PciData;
    PPCI_PDO_EXTENSION PdoExtension;
    PULONG BarArray;
    PIO_RESOURCE_DESCRIPTOR Limit;
    ULONG i;

    /* Get pointers from the context */
    PdoExtension = Context->PdoExtension;
    Current = Context->Current;
    PciData = Context->PciData;

    /* And get the array of bARs */
    BarArray = PciData->u.type0.BaseAddresses;

    /* First, check for IDE controllers that are not in native mode */
    if ((PdoExtension->BaseClass == PCI_CLASS_MASS_STORAGE_CTLR) &&
        (PdoExtension->SubClass == PCI_SUBCLASS_MSC_IDE_CTLR) &&
        (PdoExtension->ProgIf & 5) != 5)
    {
        /* They should not be using any non-legacy resources */
        BarArray[0] = 0;
        BarArray[1] = 0;
        BarArray[2] = 0;
        BarArray[3] = 0;
    }
    else if ((PdoExtension->VendorId == 0x5333) &&
             ((PdoExtension->DeviceId == 0x88F0) ||
              (PdoExtension->DeviceId == 0x8880)))
    {
        /*
         * The problem is caused by the S3 Vision 968/868 video controller which
         * is used on the Diamond Stealth 64 Video 3000 series, Number Nine 9FX
         * motion 771, and other popular video cards, all containing a memory bug.
         * The 968/868 claims to require 32 MB of memory, but it actually decodes
         * 64 MB of memory.
         */
        for (i = 0; i < PCI_TYPE0_ADDRESSES; i++)
        {
            /* Find its 32MB RAM BAR */
            if (BarArray[i] == 0xFE000000)
            {
                /* Increase it to 64MB to make sure nobody touches the buffer */
                BarArray[i] = 0xFC000000;
                DPRINT1("PCI - Adjusted broken S3 requirement from 32MB to 64MB\n");
            }
        }
    }

    /* Check for Cirrus Logic GD5430/5440 cards */
    if ((PdoExtension->VendorId == 0x1013) && (PdoExtension->DeviceId == 0xA0))
    {
        /* Check for the I/O port requirement */
        if (BarArray[1] == 0xFC01)
        {
            /* Check for completely bogus BAR */
            if (Current->u.type0.BaseAddresses[1] == 1)
            {
                /* Ignore it */
                BarArray[1] = 0;
                DPRINT1("PCI - Ignored Cirrus GD54xx broken IO requirement (400 ports)\n");
            }
            else
            {
                /* Otherwise, this BAR seems okay */
                DPRINT1("PCI - Cirrus GD54xx 400 port IO requirement has a valid setting (%08x)\n",
                        Current->u.type0.BaseAddresses[1]);
            }
        }
        else if (BarArray[1])
        {
            /* Strange, the I/O BAR was not found as expected (or at all) */
            DPRINT1("PCI - Warning Cirrus Adapter 101300a0 has unexpected resource requirement (%08x)\n",
                    BarArray[1]);
        }
    }

    /* Finally, process all the limit descriptors */
    Limit = PdoExtension->Resources->Limit;
    for (i = 0; i < PCI_TYPE0_ADDRESSES; i++)
    {
        /* And build them based on the BARs */
        if (PciCreateIoDescriptorFromBarLimit(&Limit[i], &BarArray[i], FALSE))
        {
            /* This function returns TRUE if the BAR was 64-bit, handle this */
            ASSERT((i + 1) < PCI_TYPE0_ADDRESSES);
            i++;
            Limit[i].Type = CmResourceTypeNull;
        }
    }

    /* Create the last descriptor based on the ROM address */
    PciCreateIoDescriptorFromBarLimit(&Limit[i],
                                      &PciData->u.type0.ROMBaseAddress,
                                      TRUE);
}

VOID
NTAPI
Device_MassageHeaderForLimitsDetermination(IN PPCI_CONFIGURATOR_CONTEXT Context)
{
    PPCI_COMMON_HEADER PciData;
    PPCI_PDO_EXTENSION PdoExtension;
    PULONG BarArray;
    ULONG i = 0;

    /* Get pointers from context data */
    PdoExtension = Context->PdoExtension;
    PciData = Context->PciData;

    /* Get the array of BARs */
    BarArray = PciData->u.type0.BaseAddresses;

    /* Check for IDE controllers that are not in native mode */
    if ((PdoExtension->BaseClass == PCI_CLASS_MASS_STORAGE_CTLR) &&
        (PdoExtension->SubClass == PCI_SUBCLASS_MSC_IDE_CTLR) &&
        (PdoExtension->ProgIf & 5) != 5)
    {
        /* These controllers only use legacy resources */
        i = 4;
    }

    /* Set all the bits on, which will allow us to recover the limit data */
    for (i = 0; i < PCI_TYPE0_ADDRESSES; i++) BarArray[i] = 0xFFFFFFFF;

    /* Do the same for the PCI ROM BAR */
    PciData->u.type0.ROMBaseAddress = PCI_ADDRESS_ROM_ADDRESS_MASK;
}

VOID
NTAPI
Device_RestoreCurrent(IN PPCI_CONFIGURATOR_CONTEXT Context)
{
    UNREFERENCED_PARAMETER(Context);
    /* Nothing to do for devices */
    return;
}

VOID
NTAPI
Device_GetAdditionalResourceDescriptors(IN PPCI_CONFIGURATOR_CONTEXT Context,
                                        IN PPCI_COMMON_HEADER PciData,
                                        IN PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(PciData);
    UNREFERENCED_PARAMETER(IoDescriptor);
    /* Not yet implemented */
    UNIMPLEMENTED_DBGBREAK();
}

VOID
NTAPI
Device_ResetDevice(IN PPCI_PDO_EXTENSION PdoExtension,
                   IN PPCI_COMMON_HEADER PciData)
{
    UNREFERENCED_PARAMETER(PdoExtension);
    UNREFERENCED_PARAMETER(PciData);
    /* Not yet implemented */
    UNIMPLEMENTED_DBGBREAK();
}

VOID
NTAPI
Device_ChangeResourceSettings(IN PPCI_PDO_EXTENSION PdoExtension,
                              IN PPCI_COMMON_HEADER PciData)
{
    UNREFERENCED_PARAMETER(PdoExtension);
    UNREFERENCED_PARAMETER(PciData);
    /* Not yet implemented */
    UNIMPLEMENTED_DBGBREAK();
}

/* EOF */
