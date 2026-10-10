/*
 * PROJECT:     ReactOS PCI Bus Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     PCI Express Capability Support
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

static
BOOLEAN
NTAPI
PciIsExtendedCapabilityOffsetValid(
    _In_ ULONG Offset)
{
    if ((Offset & 3) || (Offset < PCI_LEGACY_CONFIG_LENGTH))
        return FALSE;

    return (Offset + sizeof(PCI_EXPRESS_ENHANCED_CAPABILITY_HEADER)) <=
           PCI_EXTENDED_CONFIG_LENGTH;
}

/**
 * @brief
 * Finds an extended capability and reads it into the caller's buffer.
 *
 * @return
 * The offset of the capability, or 0 if it was not found.
 */
ULONG
NTAPI
PciReadDeviceExtendedCapability(
    _In_ PPCI_PDO_EXTENSION DeviceExtension,
    _In_ ULONG CapabilityId,
    _Out_writes_bytes_(Length) PPCI_EXPRESS_ENHANCED_CAPABILITY_HEADER Buffer,
    _In_ ULONG Length)
{
    PCI_EXPRESS_ENHANCED_CAPABILITY_HEADER Header;
    ULONG Offset, EntriesLeft;

    ASSERT(DeviceExtension->ExtensionType == PciPdoExtensionType);
    ASSERT(Length >= sizeof(PCI_EXPRESS_ENHANCED_CAPABILITY_HEADER));

    if (!DeviceExtension->IsExtendedConfigReachable)
        return 0;

    /* More entries than aligned offsets means the list loops */
    EntriesLeft = (PCI_EXTENDED_CONFIG_LENGTH - PCI_LEGACY_CONFIG_LENGTH) / sizeof(ULONG);

    for (Offset = PCI_LEGACY_CONFIG_LENGTH; Offset != 0; Offset = Header.Next)
    {
        if (!PciIsExtendedCapabilityOffsetValid(Offset))
            return 0;

        if (EntriesLeft == 0)
        {
            DPRINT1("PCI device %p extended capabilities list is broken\n",
                    DeviceExtension);
            return 0;
        }
        EntriesLeft--;

        PciReadDeviceConfig(DeviceExtension, &Header, Offset, sizeof(Header));

        /* A function that does not decode extended space reads back all ones */
        if (Header.CapabilityID == 0xFFFF)
            return 0;

        if (Header.CapabilityID != CapabilityId)
            continue;

        if (Length > (PCI_EXTENDED_CONFIG_LENGTH - Offset))
            return 0;

        PciReadDeviceConfig(DeviceExtension, Buffer, Offset, Length);
        return Offset;
    }

    return 0;
}

/**
 * @brief
 * Records the Express capability of a new function and whether its
 * extended configuration space can be reached.
 *
 * @param[in,out] PdoExtension
 * The PDO extension of the function being enumerated.
 */
VOID
NTAPI
PciGetExpressCapabilities(
    _Inout_ PPCI_PDO_EXTENSION PdoExtension)
{
    PCI_EXPRESS_CAPABILITY Express;
    PPCI_FDO_EXTENSION ParentFdo;
    PPCI_PDO_EXTENSION ParentBridge;
    UCHAR Offset;
    PAGED_CODE();

    PdoExtension->ExpressCapabilityPtr = 0;
    PdoExtension->ExpressDeviceType = 0;
    PdoExtension->IsExtendedConfigReachable = FALSE;

    if (!PdoExtension->CapabilitiesPtr)
        return;

    if (PdoExtension->HackFlags & PCI_HACK_NO_EXPRESS_CAP)
        return;

    /* Only the capabilities register is needed */
    Offset = PciReadDeviceCapability(PdoExtension,
                                     PdoExtension->CapabilitiesPtr,
                                     PCI_CAPABILITY_ID_PCI_EXPRESS,
                                     &Express.Header,
                                     RTL_SIZEOF_THROUGH_FIELD(PCI_EXPRESS_CAPABILITY,
                                                              ExpressCapabilities));
    if (!Offset)
        return;

    PdoExtension->ExpressCapabilityPtr = Offset;
    PdoExtension->ExpressDeviceType = (UCHAR)Express.ExpressCapabilities.DeviceType;

    /* Every bridge between the root and this function has to pass extended config cycles */
    ParentFdo = PdoExtension->ParentFdoExtension;
    if (PCI_IS_ROOT_FDO(ParentFdo))
    {
        PdoExtension->IsExtendedConfigReachable = PciEcamVerified;
    }
    else
    {
        ParentBridge = (PPCI_PDO_EXTENSION)ParentFdo->PhysicalDeviceObject->DeviceExtension;
        ASSERT_PDO(ParentBridge);
        PdoExtension->IsExtendedConfigReachable = ParentBridge->IsExtendedConfigReachable;
    }

    DPRINT1("PCI - Express capability at 0x%x, port type %u, version %u\n",
            PdoExtension->ExpressCapabilityPtr,
            PdoExtension->ExpressDeviceType,
            Express.ExpressCapabilities.CapabilityVersion);
}

/* EOF */
