/*
 * PROJECT:         ReactOS PCI Bus Driver
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/bus/pci/pci/ppbridge.c
 * PURPOSE:         PCI-to-PCI Bridge Support
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

ULONG
NTAPI
PciBridgeIoBase(IN PPCI_COMMON_HEADER PciData)
{
    BOOLEAN Is32Bit;
    ULONG Base, IoBase;
    ASSERT(PCI_CONFIGURATION_TYPE(PciData) == PCI_BRIDGE_TYPE);

    /* Get the base */
    Base = PciData->u.type1.IOLimit;

    /* Low bit specifies 32-bit address, top bits specify the base */
    Is32Bit = (Base & 0xF) == 1;
    IoBase = (Base & 0xF0) << 8;

    /* Is it 32-bit? */
    if (Is32Bit)
    {
        /* Read the upper 16-bits from the other register */
        IoBase |= PciData->u.type1.IOBaseUpper16 << 16;
        ASSERT(PciData->u.type1.IOLimit & 0x1);
    }

    /* Return the base address */
    return IoBase;
}

ULONG
NTAPI
PciBridgeIoLimit(IN PPCI_COMMON_HEADER PciData)
{
    BOOLEAN Is32Bit;
    ULONG Limit, IoLimit;
    ASSERT(PCI_CONFIGURATION_TYPE(PciData) == PCI_BRIDGE_TYPE);

    /* Get the limit */
    Limit = PciData->u.type1.IOLimit;

    /* Low bit specifies 32-bit address, top bits specify the limit */
    Is32Bit = (Limit & 0xF) == 1;
    IoLimit = (Limit & 0xF0) << 8;

    /* Is it 32-bit? */
    if (Is32Bit)
    {
        /* Read the upper 16-bits from the other register */
        IoLimit |= PciData->u.type1.IOLimitUpper16 << 16;
        ASSERT(PciData->u.type1.IOBase & 0x1);
    }

    /* Return the I/O limit */
    return IoLimit | 0xFFF;
}

ULONG
NTAPI
PciBridgeMemoryBase(IN PPCI_COMMON_HEADER PciData)
{
    ASSERT(PCI_CONFIGURATION_TYPE(PciData) == PCI_BRIDGE_TYPE);

    /* Return the memory base */
    return (PciData->u.type1.MemoryBase << 16);
}

ULONG
NTAPI
PciBridgeMemoryLimit(IN PPCI_COMMON_HEADER PciData)
{
    ASSERT(PCI_CONFIGURATION_TYPE(PciData) == PCI_BRIDGE_TYPE);

    /* Return the memory limit */
    return (PciData->u.type1.MemoryLimit << 16) | 0xFFFFF;
}

PHYSICAL_ADDRESS
NTAPI
PciBridgePrefetchMemoryBase(IN PPCI_COMMON_HEADER PciData)
{
    BOOLEAN Is64Bit;
    LARGE_INTEGER Base;
    USHORT PrefetchBase;
    ASSERT(PCI_CONFIGURATION_TYPE(PciData) == PCI_BRIDGE_TYPE);

    /* Get the base */
    PrefetchBase = PciData->u.type1.PrefetchBase;

    /* Low bit specifies 64-bit address, top bits specify the base */
    Is64Bit = (PrefetchBase & 0xF) == 1;
    Base.LowPart = ((PrefetchBase & 0xFFF0) << 16);

    /* Is it 64-bit? */
    if (Is64Bit)
    {
        /* Read the upper 32-bits from the other register */
        Base.HighPart = PciData->u.type1.PrefetchBaseUpper32;
    }

    /* Return the base */
    return Base;
}

PHYSICAL_ADDRESS
NTAPI
PciBridgePrefetchMemoryLimit(IN PPCI_COMMON_HEADER PciData)
{
    BOOLEAN Is64Bit;
    LARGE_INTEGER Limit;
    USHORT PrefetchLimit;
    ASSERT(PCI_CONFIGURATION_TYPE(PciData) == PCI_BRIDGE_TYPE);

    /* Get the base */
    PrefetchLimit = PciData->u.type1.PrefetchLimit;

    /* Low bit specifies 64-bit address, top bits specify the limit */
    Is64Bit = (PrefetchLimit & 0xF) == 1;
    Limit.LowPart = (PrefetchLimit << 16) | 0xFFFFF;

    /* Is it 64-bit? */
    if (Is64Bit)
    {
        /* Read the upper 32-bits from the other register */
        Limit.HighPart = PciData->u.type1.PrefetchLimitUpper32;
    }

    /* Return the limit */
    return Limit;
}

ULONG
NTAPI
PciBridgeMemoryWorstCaseAlignment(IN ULONG Length)
{
    ULONG Alignment;
    ASSERT(Length != 0);

    /* Start with highest alignment (2^31) */
    Alignment = 0x80000000;

    /* Keep dividing until we reach the correct power of two */
    while (!(Length & Alignment)) Alignment >>= 1;

    /* Return the alignment */
    return Alignment;
}

BOOLEAN
NTAPI
PciBridgeIsPositiveDecode(IN PPCI_PDO_EXTENSION PdoExtension)
{
    /* Undocumented ACPI Method PDEC to get positive decode settings */
    return PciIsSlotPresentInParentMethod(PdoExtension, 'CEDP');
}

BOOLEAN
NTAPI
PciBridgeIsSubtractiveDecode(IN PPCI_CONFIGURATOR_CONTEXT Context)
{
    PPCI_COMMON_HEADER Current, PciData;
    PPCI_PDO_EXTENSION PdoExtension;

    /* Get pointers from context */
    Current = Context->Current;
    PciData = Context->PciData;
    PdoExtension = Context->PdoExtension;

    /* Only valid for PCI-to-PCI bridges */
    ASSERT((Current->BaseClass == PCI_CLASS_BRIDGE_DEV) &&
           (Current->SubClass == PCI_SUBCLASS_BR_PCI_TO_PCI));

    /* Check for hacks first, then check the ProgIf of the bridge */
    if (!(PdoExtension->HackFlags & PCI_HACK_SUBTRACTIVE_DECODE) &&
         (Current->ProgIf != 1) &&
         ((PciData->u.type1.IOLimit & 0xF0) == 0xF0))
    {
        /* A subtractive decode bridge would have a ProgIf 1, and no I/O limit */
        DPRINT("Subtractive decode does not seem to be enabled\n");
        return FALSE;
    }

    /*
     * Check for Intel ICH PCI-to-PCI (i82801) bridges (used on the i810,
     * i820, i840, i845 Chipsets) that have subtractive decode broken.
     */
    if (((PdoExtension->VendorId == 0x8086) &&
         ((PdoExtension->DeviceId == 0x2418) ||
          (PdoExtension->DeviceId == 0x2428) ||
          (PdoExtension->DeviceId == 0x244E) ||
          (PdoExtension->DeviceId == 0x2448))) ||
        (PdoExtension->HackFlags & PCI_HACK_BROKEN_SUBTRACTIVE_DECODE))
    {
        /* Check if the ACPI BIOS says positive decode should be enabled */
        if (PciBridgeIsPositiveDecode(PdoExtension))
        {
            /* Obey ACPI */
            DPRINT1("Putting bridge in positive decode because of PDEC\n");
            return FALSE;
        }
    }

    /* If we found subtractive decode, we'll need a resource update later */
    DPRINT1("PCI : Subtractive decode on 0x%x\n", Current->u.type1.SecondaryBus);
    PdoExtension->UpdateHardware = TRUE;
    return TRUE;
}

VOID
NTAPI
PPBridge_SaveCurrentSettings(IN PPCI_CONFIGURATOR_CONTEXT Context)
{
    NTSTATUS Status;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR CmDescriptor;
    PIO_RESOURCE_DESCRIPTOR IoDescriptor;
    PPCI_FUNCTION_RESOURCES Resources;
    PCI_COMMON_HEADER BiosData;
    PPCI_COMMON_HEADER Current;
    PPCI_COMMON_CONFIG SavedConfig;
    ULONG i, Bar, BarMask;
    PULONG BarArray;
    PHYSICAL_ADDRESS Limit, Base, Length;
    BOOLEAN HaveIoLimit, CheckAlignment;
    PPCI_PDO_EXTENSION PdoExtension;

    /* Get the pointers from the extension */
    PdoExtension = Context->PdoExtension;
    Resources = PdoExtension->Resources;
    Current = Context->Current;

    /* Check if decodes are disabled */
    if (!(Context->Command & (PCI_ENABLE_IO_SPACE | PCI_ENABLE_MEMORY_SPACE)))
    {
        /* Well, we're going to need them from somewhere, use the registry data */
        Status = PciGetBiosConfig(PdoExtension, &BiosData);
        if (NT_SUCCESS(Status)) Current = &BiosData;
    }

    /* Scan all current and limit descriptors for each BAR needed */
    BarArray = Current->u.type1.BaseAddresses;
    for (i = 0; i < 6; i++)
    {
        /* Get the current resource descriptor, and the limit requirement */
        CmDescriptor = &Resources->Current[i];
        IoDescriptor = &Resources->Limit[i];

        /* Copy descriptor data, skipping null descriptors */
        CmDescriptor->Type = IoDescriptor->Type;
        if (CmDescriptor->Type == CmResourceTypeNull) continue;
        CmDescriptor->Flags = IoDescriptor->Flags;
        CmDescriptor->ShareDisposition = IoDescriptor->ShareDisposition;

        /* Initialize the high-parts to zero, since most stuff is 32-bit only */
        Base.QuadPart = Limit.QuadPart = Length.QuadPart = 0;

        /* Check if we're handling PCI BARs, or the ROM BAR */
        if ((i < PCI_TYPE1_ADDRESSES) || (i == 5))
        {
            /* Is this the ROM BAR? */
            if (i == 5)
            {
                /* Read the correct bar, with the appropriate mask */
                Bar = Current->u.type1.ROMBaseAddress;
                BarMask = PCI_ADDRESS_ROM_ADDRESS_MASK;

                /* Decode the base address, and write down the length */
                Base.LowPart = Bar & BarMask;
                DPRINT1("ROM BAR Base: %lx\n", Base.LowPart);
                CmDescriptor->u.Memory.Length = IoDescriptor->u.Memory.Length;
            }
            else
            {
                /* Otherwise, get the BAR from the array */
                Bar = BarArray[i];

                /* Is this an I/O BAR? */
                if (Bar & PCI_ADDRESS_IO_SPACE)
                {
                    /* Set the correct mask */
                    ASSERT(CmDescriptor->Type == CmResourceTypePort);
                    BarMask = PCI_ADDRESS_IO_ADDRESS_MASK;
                }
                else
                {
                    /* This is a memory BAR, set the correct base */
                    ASSERT(CmDescriptor->Type == CmResourceTypeMemory);
                    BarMask = PCI_ADDRESS_MEMORY_ADDRESS_MASK;

                    /* IS this a 64-bit BAR? */
                    if ((Bar & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_64BIT)
                    {
                        /* Read the next 32-bits as well, ie, the next BAR */
                        Base.HighPart = BarArray[i + 1];
                    }
                }

                /* Decode the base address, and write down the length */
                Base.LowPart = Bar & BarMask;
                DPRINT1("BAR Base: %lx\n", Base.LowPart);
                CmDescriptor->u.Generic.Length = IoDescriptor->u.Generic.Length;
            }
        }
        else
        {
            /* Reset loop conditions */
            HaveIoLimit = FALSE;
            CheckAlignment = FALSE;

            /* Check which descriptor is being parsed */
            if (i == 2)
            {
                /* I/O Port Requirements */
                Base.LowPart = PciBridgeIoBase(Current);
                Limit.LowPart = PciBridgeIoLimit(Current);
                DPRINT1("Bridge I/O Base and Limit: %lx %lx\n",
                         Base.LowPart, Limit.LowPart);

                /* Do we have any I/O Port data? */
                if (!(Base.LowPart) && (Current->u.type1.IOLimit))
                {
                    /* There's a limit */
                    HaveIoLimit = TRUE;
                }
            }
            else if (i == 3)
            {
                /* Memory requirements */
                Base.LowPart = PciBridgeMemoryBase(Current);
                Limit.LowPart = PciBridgeMemoryLimit(Current);

                /* These should always be there, so check their alignment */
                DPRINT1("Bridge MEM Base and Limit: %lx %lx\n",
                         Base.LowPart, Limit.LowPart);
                CheckAlignment = TRUE;
            }
            else if (i == 4)
            {
                /* This should only be present for prefetch memory */
                ASSERT(CmDescriptor->Flags & CM_RESOURCE_MEMORY_PREFETCHABLE);
                Base = PciBridgePrefetchMemoryBase(Current);
                Limit = PciBridgePrefetchMemoryLimit(Current);

                /* If it's there, check the alignment */
                DPRINT1("Bridge Prefetch MEM Base and Limit: %I64x %I64x\n", Base, Limit);
                CheckAlignment = TRUE;
            }

            /* Check for invalid base address */
            if (Base.QuadPart >= Limit.QuadPart)
            {
                /* Assume the descriptor is bogus */
                CmDescriptor->Type = CmResourceTypeNull;
                IoDescriptor->Type = CmResourceTypeNull;
                continue;
            }

            /* Check if there's no memory, and no I/O port either */
            if (!(Base.LowPart) && !(HaveIoLimit))
            {
                /* This seems like a bogus requirement, ignore it */
                CmDescriptor->Type = CmResourceTypeNull;
                continue;
            }

            /* Set the length to be the limit - the base; should always be 32-bit */
            Length.QuadPart = Limit.LowPart - Base.LowPart + 1;
            ASSERT(Length.HighPart == 0);
            CmDescriptor->u.Generic.Length = Length.LowPart;

            /* Check if alignment should be set */
            if (CheckAlignment)
            {
                /* Compute the required alignment for this length */
                ASSERT(CmDescriptor->u.Memory.Length > 0);
                IoDescriptor->u.Memory.Alignment =
                    PciBridgeMemoryWorstCaseAlignment(CmDescriptor->u.Memory.Length);
            }
        }

        /* Now set the base address */
        CmDescriptor->u.Generic.Start.LowPart = Base.LowPart;
    }

    /* Save PCI settings into the PDO extension for easy access later */
    PdoExtension->Dependent.type1.PrimaryBus = Current->u.type1.PrimaryBus;
    PdoExtension->Dependent.type1.SecondaryBus = Current->u.type1.SecondaryBus;
    PdoExtension->Dependent.type1.SubordinateBus = Current->u.type1.SubordinateBus;

    /* Check for subtractive decode bridges */
    if (PdoExtension->Dependent.type1.SubtractiveDecode)
    {
        /* Check if legacy VGA decodes are enabled */
        DPRINT1("Subtractive decode bridge\n");
        if (Current->u.type1.BridgeControl & PCI_ENABLE_BRIDGE_VGA)
        {
            /* Save this setting for later */
            DPRINT1("VGA Bridge\n");
            PdoExtension->Dependent.type1.VgaBitSet = TRUE;
        }

        /* Legacy ISA decoding is not compatible with subtractive decode */
        ASSERT(PdoExtension->Dependent.type1.IsaBitSet == FALSE);
    }
    else
    {
        /* Check if legacy VGA decodes are enabled */
        if (Current->u.type1.BridgeControl & PCI_ENABLE_BRIDGE_VGA)
        {
            /* Save this setting for later */
            DPRINT1("VGA Bridge\n");
            PdoExtension->Dependent.type1.VgaBitSet = TRUE;

            /* And on positive decode, we'll also need extra resources locked */
            PdoExtension->AdditionalResourceCount = 4;
        }

        /* Check if legacy ISA decoding is enabled */
        if (Current->u.type1.BridgeControl & PCI_ENABLE_BRIDGE_ISA)
        {
            /* Save this setting for later */
            DPRINT1("ISA Bridge\n");
            PdoExtension->Dependent.type1.IsaBitSet = TRUE;
        }
    }

    /*
     * Check for Intel ICH PCI-to-PCI (i82801) bridges (used on the i810,
     * i820, i840, i845 Chipsets) that have subtractive decode broken.
     */
    if (((PdoExtension->VendorId == 0x8086) &&
         ((PdoExtension->DeviceId == 0x2418) ||
          (PdoExtension->DeviceId == 0x2428) ||
          (PdoExtension->DeviceId == 0x244E) ||
          (PdoExtension->DeviceId == 0x2448))) ||
        (PdoExtension->HackFlags & PCI_HACK_BROKEN_SUBTRACTIVE_DECODE))
    {
        /* Check if subtractive decode is actually enabled */
        if (PdoExtension->Dependent.type1.SubtractiveDecode)
        {
            /* We're going to need a copy of the configuration for later use */
            DPRINT1("apply config save hack to ICH subtractive decode\n");
            SavedConfig = ExAllocatePoolWithTag(0, PCI_COMMON_HDR_LENGTH, 'PciP');
            PdoExtension->ParentFdoExtension->PreservedConfig = SavedConfig;
            if (SavedConfig) RtlCopyMemory(SavedConfig, Current, PCI_COMMON_HDR_LENGTH);
        }
    }
}

VOID
NTAPI
PPBridge_SaveLimits(IN PPCI_CONFIGURATOR_CONTEXT Context)
{
    PIO_RESOURCE_DESCRIPTOR Limit;
    PULONG BarArray;
    PHYSICAL_ADDRESS MemoryLimit;
    ULONG i;
    PPCI_COMMON_HEADER Working;
    PPCI_PDO_EXTENSION PdoExtension;

    /* Get the pointers from the context */
    Working = Context->PciData;
    PdoExtension = Context->PdoExtension;

    /* Scan the BARs into the limit descriptors */
    BarArray = Working->u.type1.BaseAddresses;
    Limit = PdoExtension->Resources->Limit;

    /* First of all, loop all the BARs */
    for (i = 0; i < PCI_TYPE1_ADDRESSES; i++)
    {
        /* Create a descriptor for their limits */
        if (PciCreateIoDescriptorFromBarLimit(&Limit[i], &BarArray[i], FALSE))
        {
            /* This was a 64-bit descriptor, make sure there's space */
            ASSERT((i + 1) < PCI_TYPE1_ADDRESSES);

            /* Skip the next descriptor since this one is double sized */
            i++;
            Limit[i].Type = CmResourceTypeNull;
        }
    }

    /* Check if this is a subtractive decode bridge */
    if (PciBridgeIsSubtractiveDecode(Context))
    {
        /* This bridge is subtractive */
        PdoExtension->Dependent.type1.SubtractiveDecode = TRUE;

        /* Subtractive bridges cannot use legacy ISA or VGA functionality */
        PdoExtension->Dependent.type1.IsaBitSet = FALSE;
        PdoExtension->Dependent.type1.VgaBitSet = FALSE;
    }

    /* For normal decode bridges, we'll need to find the bridge limits too */
    if (!PdoExtension->Dependent.type1.SubtractiveDecode)
    {
        /* Loop the descriptors that are left, to store the bridge limits */
        for (i = PCI_TYPE1_ADDRESSES; i < 5; i++)
        {
            /* No 64-bit memory addresses, and set the address to 0 to begin */
            MemoryLimit.HighPart = 0;
            (&Limit[i])->u.Port.MinimumAddress.QuadPart = 0;

            /* Are we getting the I/O limit? */
            if (i == 2)
            {
                /* There should be one, get it */
                ASSERT(Working->u.type1.IOLimit != 0);
                ASSERT((Working->u.type1.IOLimit & 0x0E) == 0);
                MemoryLimit.LowPart = PciBridgeIoLimit(Working);

                /* Build a descriptor for this limit */
                (&Limit[i])->Type = CmResourceTypePort;
                (&Limit[i])->Flags = CM_RESOURCE_PORT_WINDOW_DECODE |
                                     CM_RESOURCE_PORT_POSITIVE_DECODE;
                (&Limit[i])->u.Port.Alignment = 0x1000;
                (&Limit[i])->u.Port.MinimumAddress.QuadPart = 0;
                (&Limit[i])->u.Port.MaximumAddress = MemoryLimit;
                (&Limit[i])->u.Port.Length = 0;
            }
            else if (i == 3)
            {
                /* There should be a valid memory limit, get it */
                ASSERT((Working->u.type1.MemoryLimit & 0xF) == 0);
                MemoryLimit.LowPart = PciBridgeMemoryLimit(Working);

                /* Build the descriptor for it */
                (&Limit[i])->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
                (&Limit[i])->Type = CmResourceTypeMemory;
                (&Limit[i])->u.Memory.Alignment = 0x100000;
                (&Limit[i])->u.Memory.MinimumAddress.QuadPart = 0;
                (&Limit[i])->u.Memory.MaximumAddress = MemoryLimit;
                (&Limit[i])->u.Memory.Length = 0;
            }
            else if (Working->u.type1.PrefetchLimit)
            {
                /* Get the prefetch memory limit, if there is one */
                MemoryLimit = PciBridgePrefetchMemoryLimit(Working);

                /* Write out the descriptor for it */
                (&Limit[i])->Flags = CM_RESOURCE_MEMORY_PREFETCHABLE;
                (&Limit[i])->Type = CmResourceTypeMemory;
                (&Limit[i])->u.Memory.Alignment = 0x100000;
                (&Limit[i])->u.Memory.MinimumAddress.QuadPart = 0;
                (&Limit[i])->u.Memory.MaximumAddress = MemoryLimit;
                (&Limit[i])->u.Memory.Length = 0;
            }
            else
            {
                /* Blank descriptor */
                (&Limit[i])->Type = CmResourceTypeNull;
            }
        }
    }

    /* Does the ROM have its own BAR? */
    if (Working->u.type1.ROMBaseAddress & PCI_ROMADDRESS_ENABLED)
    {
        /* Build a limit for it as well */
        PciCreateIoDescriptorFromBarLimit(&Limit[i],
                                          &Working->u.type1.ROMBaseAddress,
                                          TRUE);
    }
}

VOID
NTAPI
PPBridge_MassageHeaderForLimitsDetermination(IN PPCI_CONFIGURATOR_CONTEXT Context)
{
    PPCI_COMMON_HEADER PciData, Current;

    /* Get pointers from context */
    PciData = Context->PciData;
    Current = Context->Current;

    /*
     * Write FFh everywhere so that the PCI bridge ignores what it can't handle.
     * Based on the bits that were ignored (still 0), this is how we can tell
     * what the limit is.
     */
    RtlFillMemory(PciData->u.type1.BaseAddresses,
                  FIELD_OFFSET(PCI_COMMON_HEADER, u.type1.CapabilitiesPtr) -
                  FIELD_OFFSET(PCI_COMMON_HEADER, u.type1.BaseAddresses),
                  0xFF);

    /* Copy the saved settings from the current context into the PCI header */
    PciData->u.type1.PrimaryBus = Current->u.type1.PrimaryBus;
    PciData->u.type1.SecondaryBus = Current->u.type1.SecondaryBus;
    PciData->u.type1.SubordinateBus = Current->u.type1.SubordinateBus;
    PciData->u.type1.SecondaryLatency = Current->u.type1.SecondaryLatency;

    /* No I/O limit or base. The bottom base bit specifies that FIXME */
    PciData->u.type1.IOBaseUpper16 = 0xFFFE;
    PciData->u.type1.IOLimitUpper16 = 0xFFFF;

    /* Save secondary status before it gets cleared */
    Context->SecondaryStatus = Current->u.type1.SecondaryStatus;

    /* Clear secondary status */
    Current->u.type1.SecondaryStatus = 0;
    PciData->u.type1.SecondaryStatus = 0;
}

VOID
NTAPI
PPBridge_RestoreCurrent(IN PPCI_CONFIGURATOR_CONTEXT Context)
{
    /* Copy back the secondary status register */
    Context->Current->u.type1.SecondaryStatus = Context->SecondaryStatus;
}

VOID
NTAPI
PPBridge_GetAdditionalResourceDescriptors(IN PPCI_CONFIGURATOR_CONTEXT Context,
                                          IN PPCI_COMMON_HEADER PciData,
                                          IN PIO_RESOURCE_DESCRIPTOR IoDescriptor)
{

    UNREFERENCED_PARAMETER(Context);

    /* Does this bridge have VGA decodes on it? */
    if (PciData->u.type1.BridgeControl & PCI_ENABLE_BRIDGE_VGA)
    {
        /* Build a private descriptor with 3 entries */
        IoDescriptor->Type = CmResourceTypeDevicePrivate;
        IoDescriptor->u.DevicePrivate.Data[0] = 3;
        IoDescriptor->u.DevicePrivate.Data[1] = 3;

        /* First, the VGA range at 0xA0000 */
        IoDescriptor[1].Type = CmResourceTypeMemory;
        IoDescriptor[1].Flags = CM_RESOURCE_MEMORY_READ_WRITE;
        IoDescriptor[1].u.Port.Length = 0x20000;
        IoDescriptor[1].u.Port.Alignment = 1;
        IoDescriptor[1].u.Port.MinimumAddress.QuadPart = 0xA0000;
        IoDescriptor[1].u.Port.MaximumAddress.QuadPart = 0xBFFFF;

        /* Then, the VGA registers at 0x3B0 */
        IoDescriptor[2].Type = CmResourceTypePort;
        IoDescriptor[2].Flags = CM_RESOURCE_PORT_POSITIVE_DECODE |
                                CM_RESOURCE_PORT_10_BIT_DECODE;
        IoDescriptor[2].u.Port.Length = 12;
        IoDescriptor[2].u.Port.Alignment = 1;
        IoDescriptor[2].u.Port.MinimumAddress.QuadPart = 0x3B0;
        IoDescriptor[2].u.Port.MaximumAddress.QuadPart = 0x3BB;

        /* And finally the VGA registers at 0x3C0 */
        IoDescriptor[3].Type = CmResourceTypePort;
        IoDescriptor[3].Flags = CM_RESOURCE_PORT_POSITIVE_DECODE |
                                CM_RESOURCE_PORT_10_BIT_DECODE;
        IoDescriptor[3].u.Port.Length = 32;
        IoDescriptor[3].u.Port.Alignment = 1;
        IoDescriptor[3].u.Port.MinimumAddress.QuadPart = 0x3C0;
        IoDescriptor[3].u.Port.MaximumAddress.QuadPart = 0x3DF;
    }
}

VOID
NTAPI
PPBridge_ResetDevice(IN PPCI_PDO_EXTENSION PdoExtension,
                     IN PPCI_COMMON_HEADER PciData)
{
    UNREFERENCED_PARAMETER(PdoExtension);
    UNREFERENCED_PARAMETER(PciData);
    UNIMPLEMENTED_DBGBREAK();
}

VOID
NTAPI
PPBridge_ChangeResourceSettings(IN PPCI_PDO_EXTENSION PdoExtension,
                                IN PPCI_COMMON_HEADER PciData)
{
    //BOOLEAN IoActive;
    PPCI_FDO_EXTENSION FdoExtension;
    PPCI_FUNCTION_RESOURCES PciResources;
    ULONG i;

    /* Check if I/O Decodes are enabled */
    //IoActive = (PciData->u.type1.IOBase & 0xF) == 1;

    /*
     * Check for Intel ICH PCI-to-PCI (i82801) bridges (used on the i810,
     * i820, i840, i845 Chipsets) that don't have subtractive decode broken.
     * If they do have broken subtractive support, or if they are not ICH bridges,
     * then check if the bridge supports subtractive decode at all.
     */
    if ((((PdoExtension->VendorId == 0x8086) &&
         ((PdoExtension->DeviceId == 0x2418) ||
          (PdoExtension->DeviceId == 0x2428) ||
          (PdoExtension->DeviceId == 0x244E) ||
          (PdoExtension->DeviceId == 0x2448))) &&
         (!(PdoExtension->HackFlags & PCI_HACK_BROKEN_SUBTRACTIVE_DECODE) ||
         (PdoExtension->Dependent.type1.SubtractiveDecode == FALSE))) ||
        (PdoExtension->Dependent.type1.SubtractiveDecode == FALSE))
    {
        /* No resources are needed on a subtractive decode bridge */
        PciData->u.type1.MemoryBase = 0xFFFF;
        PciData->u.type1.PrefetchBase = 0xFFFF;
        PciData->u.type1.IOBase = 0xFF;
        PciData->u.type1.IOLimit = 0;
        PciData->u.type1.MemoryLimit = 0;
        PciData->u.type1.PrefetchLimit = 0;
        PciData->u.type1.PrefetchBaseUpper32 = 0;
        PciData->u.type1.PrefetchLimitUpper32 = 0;
        PciData->u.type1.IOBaseUpper16 = 0;
        PciData->u.type1.IOLimitUpper16 = 0;
    }
    else
    {
        /*
         * Otherwise, get the FDO to read the old PCI configuration header that
         * had been saved by the hack in PPBridge_SaveCurrentSettings.
         */
        FdoExtension = PdoExtension->ParentFdoExtension;
        ASSERT(PdoExtension->Resources == NULL);

        /* Read the PCI header data and use that here */
        PciData->u.type1.IOBase = FdoExtension->PreservedConfig->u.type1.IOBase;
        PciData->u.type1.IOLimit = FdoExtension->PreservedConfig->u.type1.IOLimit;
        PciData->u.type1.MemoryBase = FdoExtension->PreservedConfig->u.type1.MemoryBase;
        PciData->u.type1.MemoryLimit = FdoExtension->PreservedConfig->u.type1.MemoryLimit;
        PciData->u.type1.PrefetchBase = FdoExtension->PreservedConfig->u.type1.PrefetchBase;
        PciData->u.type1.PrefetchLimit = FdoExtension->PreservedConfig->u.type1.PrefetchLimit;
        PciData->u.type1.PrefetchBaseUpper32 = FdoExtension->PreservedConfig->u.type1.PrefetchBaseUpper32;
        PciData->u.type1.PrefetchLimitUpper32 = FdoExtension->PreservedConfig->u.type1.PrefetchLimitUpper32;
        PciData->u.type1.IOBaseUpper16 = FdoExtension->PreservedConfig->u.type1.IOBaseUpper16;
        PciData->u.type1.IOLimitUpper16 = FdoExtension->PreservedConfig->u.type1.IOLimitUpper16;
    }

    /* Loop bus resources */
    PciResources = PdoExtension->Resources;
    if (PciResources)
    {
        /* Loop each resource type (the BARs, ROM BAR and Prefetch) */
        for (i = 0; i < 6; i++)
        {
            UNIMPLEMENTED;
        }
    }

    /* Copy the bus number data */
    PciData->u.type1.PrimaryBus = PdoExtension->Dependent.type1.PrimaryBus;
    PciData->u.type1.SecondaryBus = PdoExtension->Dependent.type1.SecondaryBus;
    PciData->u.type1.SubordinateBus = PdoExtension->Dependent.type1.SubordinateBus;

    /* Copy the decode flags */
    if (PdoExtension->Dependent.type1.IsaBitSet)
    {
        PciData->u.type1.BridgeControl |= PCI_ENABLE_BRIDGE_ISA;
    }

    if (PdoExtension->Dependent.type1.VgaBitSet)
    {
        PciData->u.type1.BridgeControl |= PCI_ENABLE_BRIDGE_VGA;
    }
}

/* EOF */
