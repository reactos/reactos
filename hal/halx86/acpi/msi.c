/*
 * PROJECT:         ReactOS Hardware Abstraction Layer
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/acpi/msi.c
 * PURPOSE:         PCI MSI/MSI-X Capability Programming
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* DEFINES ********************************************************************/

/* MSI Capability Register Offsets (within capability) */
#define MSI_CONTROL_REG         0x02
#define MSI_ADDRESS_LO_REG      0x04
#define MSI_ADDRESS_HI_REG      0x08
#define MSI_DATA_32_REG         0x08
#define MSI_DATA_64_REG         0x0C

/* MSI Control Register Bits */
#define MSI_ENABLE              0x01
#define MSI_MULTIPLE_MSG_CAP    0x0E  /* Bits 3:1 */
#define MSI_MULTIPLE_MSG_ENABLE 0x70  /* Bits 6:4 */
#define MSI_64BIT_ADDR_CAP      0x80

/* MSI Message Address Format (x86 LAPIC) */
#define MSI_ADDR_BASE           0xFEE00000UL
#define MSI_ADDR_DEST_ID_SHIFT  12
#define MSI_ADDR_RH_BIT         0x08
#define MSI_ADDR_DM_BIT         0x04

/* MSI Message Data Format */
#define MSI_DATA_VECTOR_MASK    0xFF
#define MSI_DATA_DELIVERY_SHIFT 8
#define MSI_DATA_LEVEL_BIT      0x4000
#define MSI_DATA_ASSERT_BIT     0x4000

/* MSI-X Capability Register Offsets (within capability) */
#define MSIX_CONTROL_REG        0x02
#define MSIX_TABLE_OFFSET_REG   0x04
#define MSIX_PBA_OFFSET_REG     0x08

/* MSI-X Control Register Bits */
#define MSIX_ENABLE             0x8000
#define MSIX_FUNCTION_MASKED    0x4000
#define MSIX_TABLE_SIZE_MASK    0x07FF

/* MSI-X Table Entry Size */
#define MSIX_TABLE_ENTRY_SIZE   16
#define MSIX_ENTRY_ADDR_LO      0x00
#define MSIX_ENTRY_ADDR_HI      0x04
#define MSIX_ENTRY_DATA         0x08
#define MSIX_ENTRY_CONTROL      0x0C
#define MSIX_ENTRY_VECTOR_CTRL  0x01

/* FUNCTIONS ******************************************************************/

/**
 * @brief Walk the PCI capability list to find MSI capability
 * @param BusHandler Bus handler for PCI config access
 * @param Slot PCI slot number
 * @return Offset to MSI capability (0 if not found)
 */
UCHAR
NTAPI
HalpPciDetectMsiCapability(
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot)
{
    PCI_COMMON_CONFIG Config;
    UCHAR CapPtr;
    UCHAR CapId;
    USHORT Status;

    /* Read PCI common header */
    HalpReadPCIConfig(BusHandler, Slot, &Config, 0, sizeof(PCI_COMMON_CONFIG));

    /* Check if device has capability list */
    Status = Config.Status;
    if (!(Status & PCI_STATUS_CAPABILITIES_LIST))
    {
        DPRINT("Device has no capability list\n");
        return 0;
    }

    /* Start at capabilities pointer */
    CapPtr = Config.u.type0.CapabilitiesPtr;

    /* Walk the capability list */
    while (CapPtr != 0)
    {
        /* Read capability header (ID and Next) */
        HalpReadPCIConfig(BusHandler, Slot, &CapId, CapPtr, sizeof(UCHAR));

        /* Check if this is MSI capability (ID 0x05) */
        if (CapId == PCI_CAPABILITY_ID_MSI)
        {
            DPRINT("Found MSI capability at offset 0x%02X\n", CapPtr);
            return CapPtr;
        }

        /* Move to next capability (stored at offset+1) */
        HalpReadPCIConfig(BusHandler, Slot, &CapPtr, CapPtr + 1, sizeof(UCHAR));

        /* Sanity check: offset should be 4-byte aligned and within config space */
        if ((CapPtr & 0x03) || CapPtr < 0x40)
        {
            DPRINT("Invalid capability pointer: 0x%02X\n", CapPtr);
            return 0;
        }
    }

    DPRINT("MSI capability not found\n");
    return 0;
}

/**
 * @brief Walk the PCI capability list to find MSI-X capability
 * @param BusHandler Bus handler for PCI config access
 * @param Slot PCI slot number
 * @return Offset to MSI-X capability (0 if not found)
 */
UCHAR
NTAPI
HalpPciDetectMsiXCapability(
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot)
{
    PCI_COMMON_CONFIG Config;
    UCHAR CapPtr;
    UCHAR CapId;
    USHORT Status;

    /* Read PCI common header */
    HalpReadPCIConfig(BusHandler, Slot, &Config, 0, sizeof(PCI_COMMON_CONFIG));

    /* Check if device has capability list */
    Status = Config.Status;
    if (!(Status & PCI_STATUS_CAPABILITIES_LIST))
    {
        DPRINT("Device has no capability list\n");
        return 0;
    }

    /* Start at capabilities pointer */
    CapPtr = Config.u.type0.CapabilitiesPtr;

    /* Walk the capability list */
    while (CapPtr != 0)
    {
        /* Read capability header (ID and Next) */
        HalpReadPCIConfig(BusHandler, Slot, &CapId, CapPtr, sizeof(UCHAR));

        /* Check if this is MSI-X capability (ID 0x11) */
        if (CapId == PCI_CAPABILITY_ID_MSIX)
        {
            DPRINT("Found MSI-X capability at offset 0x%02X\n", CapPtr);
            return CapPtr;
        }

        /* Move to next capability (stored at offset+1) */
        HalpReadPCIConfig(BusHandler, Slot, &CapPtr, CapPtr + 1, sizeof(UCHAR));

        /* Sanity check: offset should be 4-byte aligned and within config space */
        if ((CapPtr & 0x03) || CapPtr < 0x40)
        {
            DPRINT("Invalid capability pointer: 0x%02X\n", CapPtr);
            return 0;
        }
    }

    DPRINT("MSI-X capability not found\n");
    return 0;
}

/**
 * @brief Enable MSI for a device with a single message vector
 * @param BusHandler Bus handler for PCI config access
 * @param Slot PCI slot number
 * @param Vector CPU vector number (32-255)
 * @param ProcessorNumber Target processor for interrupt delivery
 * @return TRUE if successful
 */
BOOLEAN
NTAPI
HalpPciEnableMsi(
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN ULONG Vector,
    IN UCHAR ProcessorNumber)
{
    UCHAR MsiOffset;
    USHORT Control;
    ULONG MessageAddr;
    USHORT MessageData;
    UCHAR ApicId;

    /* Find MSI capability */
    MsiOffset = HalpPciDetectMsiCapability(BusHandler, Slot);
    if (MsiOffset == 0)
    {
        DPRINT1("MSI capability not found\n");
        return FALSE;
    }

    DPRINT("Enabling MSI on device at offset 0x%02X\n", MsiOffset);

    /* Read current control register */
    HalpReadPCIConfig(BusHandler, Slot, &Control, MsiOffset + MSI_CONTROL_REG, sizeof(USHORT));

    /* Check for 64-bit address support */
    BOOLEAN Use64Bit = (Control & MSI_64BIT_ADDR_CAP) != 0;

    /* Read current Message Address (lower 32 bits) */
    HalpReadPCIConfig(BusHandler, Slot, &MessageAddr, MsiOffset + MSI_ADDRESS_LO_REG, sizeof(ULONG));

    /* Get APIC ID for target processor (for now, use destination physical addressing) */
    ApicId = (UCHAR)ProcessorNumber;

    /* Build Message Address: 0xFEE00000 | (ApicId << 12) */
    MessageAddr = MSI_ADDR_BASE | (ApicId << MSI_ADDR_DEST_ID_SHIFT);

    /* Write Message Address (lower 32 bits) */
    HalpWritePCIConfig(BusHandler, Slot, &MessageAddr, MsiOffset + MSI_ADDRESS_LO_REG, sizeof(ULONG));

    /* If 64-bit, write upper 32 bits as zero */
    if (Use64Bit)
    {
        ULONG MessageAddrHi = 0;
        HalpWritePCIConfig(BusHandler, Slot, &MessageAddrHi, MsiOffset + MSI_ADDRESS_HI_REG, sizeof(ULONG));

        /* Message Data is at offset +12 for 64-bit */
        MessageData = Vector;
        HalpWritePCIConfig(BusHandler, Slot, &MessageData, MsiOffset + MSI_DATA_64_REG, sizeof(USHORT));
    }
    else
    {
        /* Message Data is at offset +8 for 32-bit */
        MessageData = Vector;
        HalpWritePCIConfig(BusHandler, Slot, &MessageData, MsiOffset + MSI_DATA_32_REG, sizeof(USHORT));
    }

    /* Set Multiple Message Enable to 1 message (bits 6:4 = 000) */
    Control &= ~MSI_MULTIPLE_MSG_ENABLE;

    /* Enable MSI (bit 0) */
    Control |= MSI_ENABLE;

    /* Write control register back */
    HalpWritePCIConfig(BusHandler, Slot, &Control, MsiOffset + MSI_CONTROL_REG, sizeof(USHORT));

    DPRINT("MSI enabled: Address=0x%08lX, Data=0x%04X\n", MessageAddr, MessageData);
    return TRUE;
}

/**
 * @brief Enable MSI-X for a device
 * @param BusHandler Bus handler for PCI config access
 * @param Slot PCI slot number
 * @param TableEntry Which entry in the MSI-X table (0-based)
 * @param Vector CPU vector number (32-255)
 * @param ProcessorNumber Target processor for interrupt delivery
 * @return TRUE if successful
 */
BOOLEAN
NTAPI
HalpPciEnableMsiX(
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot,
    IN ULONG TableEntry,
    IN ULONG Vector,
    IN UCHAR ProcessorNumber)
{
    UCHAR MsixOffset;
    USHORT Control;
    USHORT TableOffsetReg;
    ULONG TableBarIndex;
    ULONG TableOffset;
    UCHAR ApicId;
    ULONG MessageAddr;
    ULONG MessageData;
    ULONG VectorControl;

    /* Find MSI-X capability */
    MsixOffset = HalpPciDetectMsiXCapability(BusHandler, Slot);
    if (MsixOffset == 0)
    {
        DPRINT1("MSI-X capability not found\n");
        return FALSE;
    }

    DPRINT("Enabling MSI-X on device at offset 0x%02X, entry %lu\n", MsixOffset, TableEntry);

    /* Read control register */
    HalpReadPCIConfig(BusHandler, Slot, &Control, MsixOffset + MSIX_CONTROL_REG, sizeof(USHORT));

    /* Get table size from bits 10:0 */
    ULONG TableSize = (Control & MSIX_TABLE_SIZE_MASK) + 1;

    /* Validate table entry */
    if (TableEntry >= TableSize)
    {
        DPRINT1("Invalid MSI-X table entry %lu (size=%lu)\n", TableEntry, TableSize);
        return FALSE;
    }

    /* Read Table Offset and BAR Index register */
    HalpReadPCIConfig(BusHandler, Slot, &TableOffsetReg, MsixOffset + MSIX_TABLE_OFFSET_REG, sizeof(ULONG));

    TableBarIndex = TableOffsetReg & 0x07;
    TableOffset = TableOffsetReg & ~0x07;

    DPRINT("MSI-X table: BAR %lu, offset 0x%lX, entry size %d\n", TableBarIndex, TableOffset, MSIX_TABLE_ENTRY_SIZE);

    /* Note: In a full implementation, we would:
     * 1. Read the BAR to get physical address
     * 2. Map the table into kernel VA
     * 3. Write table entries at: VA + TableOffset + (TableEntry * MSIX_TABLE_ENTRY_SIZE)
     *
     * For now, we'll program via config space, though this is typically slower.
     * MSI-X table is at config offset 0x100+ for extended config space.
     */

    /* Get APIC ID for target processor */
    ApicId = (UCHAR)ProcessorNumber;

    /* Build Message Address: 0xFEE00000 | (ApicId << 12) */
    MessageAddr = MSI_ADDR_BASE | (ApicId << MSI_ADDR_DEST_ID_SHIFT);

    /* Message Data contains vector */
    MessageData = Vector;

    /* Vector Control: bit 0 is mask bit (0 = enabled, 1 = masked) */
    VectorControl = 0;  /* Enable this entry */

    DPRINT("Writing MSI-X entry: Addr=0x%08lX, Data=0x%08lX, Ctrl=0x%08lX\n",
           MessageAddr, MessageData, VectorControl);

    /* Write Message Address (Low 32 bits) via config space */
    /* MSI-X table starts at config offset 0x100 or is accessed via BAR mapping */
    /* For extended capability space (offset >= 0x100): */
    /* Entry offset = TableOffset + (TableEntry * 16) */

    ULONG EntryOffset = TableOffset + (TableEntry * MSIX_TABLE_ENTRY_SIZE);

    /* Write Address Low */
    HalpWritePCIConfig(BusHandler, Slot, &MessageAddr, EntryOffset + MSIX_ENTRY_ADDR_LO, sizeof(ULONG));

    /* Write Address High (always 0 for now) */
    ULONG AddrHi = 0;
    HalpWritePCIConfig(BusHandler, Slot, &AddrHi, EntryOffset + MSIX_ENTRY_ADDR_HI, sizeof(ULONG));

    /* Write Data */
    HalpWritePCIConfig(BusHandler, Slot, &MessageData, EntryOffset + MSIX_ENTRY_DATA, sizeof(ULONG));

    /* Write Vector Control */
    HalpWritePCIConfig(BusHandler, Slot, &VectorControl, EntryOffset + MSIX_ENTRY_CONTROL, sizeof(ULONG));

    /* Enable MSI-X (bit 15) */
    Control |= MSIX_ENABLE;

    /* Write control register back */
    HalpWritePCIConfig(BusHandler, Slot, &Control, MsixOffset + MSIX_CONTROL_REG, sizeof(USHORT));

    DPRINT("MSI-X enabled for entry %lu\n", TableEntry);
    return TRUE;
}

/**
 * @brief Disable MSI on a device
 * @param BusHandler Bus handler for PCI config access
 * @param Slot PCI slot number
 * @return TRUE if successful
 */
BOOLEAN
NTAPI
HalpPciDisableMsi(
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot)
{
    UCHAR MsiOffset;
    USHORT Control;

    /* Find MSI capability */
    MsiOffset = HalpPciDetectMsiCapability(BusHandler, Slot);
    if (MsiOffset == 0)
    {
        DPRINT("MSI capability not found\n");
        return FALSE;
    }

    DPRINT("Disabling MSI on device\n");

    /* Read control register */
    HalpReadPCIConfig(BusHandler, Slot, &Control, MsiOffset + MSI_CONTROL_REG, sizeof(USHORT));

    /* Clear MSI Enable bit */
    Control &= ~MSI_ENABLE;

    /* Write control register back */
    HalpWritePCIConfig(BusHandler, Slot, &Control, MsiOffset + MSI_CONTROL_REG, sizeof(USHORT));

    DPRINT("MSI disabled\n");
    return TRUE;
}

/**
 * @brief Disable MSI-X on a device
 * @param BusHandler Bus handler for PCI config access
 * @param Slot PCI slot number
 * @return TRUE if successful
 */
BOOLEAN
NTAPI
HalpPciDisableMsiX(
    IN PBUS_HANDLER BusHandler,
    IN PCI_SLOT_NUMBER Slot)
{
    UCHAR MsixOffset;
    USHORT Control;

    /* Find MSI-X capability */
    MsixOffset = HalpPciDetectMsiXCapability(BusHandler, Slot);
    if (MsixOffset == 0)
    {
        DPRINT("MSI-X capability not found\n");
        return FALSE;
    }

    DPRINT("Disabling MSI-X on device\n");

    /* Read control register */
    HalpReadPCIConfig(BusHandler, Slot, &Control, MsixOffset + MSIX_CONTROL_REG, sizeof(USHORT));

    /* Clear MSI-X Enable bit (bit 15) */
    Control &= ~MSIX_ENABLE;

    /* Write control register back */
    HalpWritePCIConfig(BusHandler, Slot, &Control, MsixOffset + MSIX_CONTROL_REG, sizeof(USHORT));

    DPRINT("MSI-X disabled\n");
    return TRUE;
}

/* EOF */
