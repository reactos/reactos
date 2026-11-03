/*
 * PROJECT:     ReactOS ARM64 HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PCI configuration space through the MCFG window
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

#define HALP_ECAM_FUNCTION_SIZE     0x1000
#define HALP_ECAM_BUS_SIZE          (PCI_MAX_DEVICES * PCI_MAX_FUNCTION * HALP_ECAM_FUNCTION_SIZE)

/* Buses are mapped the first time they are touched and stay mapped */
static PUCHAR HalpEcamBus[256];

/* FUNCTIONS *****************************************************************/

static
PUCHAR
HalpPciFunctionConfig(
    _In_ ULONG Bus,
    _In_ PCI_SLOT_NUMBER Slot)
{
    PHYSICAL_ADDRESS Address;
    PUCHAR View;

    if (!HalpPlatform.EcamBase.QuadPart ||
        (Bus < HalpPlatform.EcamStartBus) ||
        (Bus > HalpPlatform.EcamEndBus))
    {
        return NULL;
    }

    View = HalpEcamBus[Bus];
    if (!View)
    {
        if (KeGetCurrentIrql() > DISPATCH_LEVEL)
            return NULL;

        Address.QuadPart = HalpPlatform.EcamBase.QuadPart +
                           ((ULONG64)(Bus - HalpPlatform.EcamStartBus) * HALP_ECAM_BUS_SIZE);
        View = MmMapIoSpace(Address, HALP_ECAM_BUS_SIZE, MmNonCached);
        if (!View)
            return NULL;

        /* Another processor may have mapped it meanwhile */
        if (InterlockedCompareExchangePointer((PVOID *)&HalpEcamBus[Bus], View, NULL))
        {
            MmUnmapIoSpace(View, HALP_ECAM_BUS_SIZE);
            View = HalpEcamBus[Bus];
        }
    }

    return View + (((Slot.u.bits.DeviceNumber * PCI_MAX_FUNCTION) +
                    Slot.u.bits.FunctionNumber) * HALP_ECAM_FUNCTION_SIZE);
}

/* Uses the widest access the alignment allows, devices may not decode wider ones */
static
VOID
HalpPciTransfer(
    _In_ PUCHAR Config,
    _Inout_updates_bytes_(Length) PUCHAR Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length,
    _In_ BOOLEAN Write)
{
    ULONG Width;

    for (; Length; Offset += Width, Buffer += Width, Length -= Width)
    {
        if (!(Offset & 3) && (Length >= sizeof(ULONG)))
        {
            Width = sizeof(ULONG);
            if (Write)
                WRITE_REGISTER_ULONG((PULONG)(Config + Offset), *(ULONG UNALIGNED *)Buffer);
            else
                *(ULONG UNALIGNED *)Buffer = READ_REGISTER_ULONG((PULONG)(Config + Offset));
        }
        else if (!(Offset & 1) && (Length >= sizeof(USHORT)))
        {
            Width = sizeof(USHORT);
            if (Write)
                WRITE_REGISTER_USHORT((PUSHORT)(Config + Offset), *(USHORT UNALIGNED *)Buffer);
            else
                *(USHORT UNALIGNED *)Buffer = READ_REGISTER_USHORT((PUSHORT)(Config + Offset));
        }
        else
        {
            Width = sizeof(UCHAR);
            if (Write)
                WRITE_REGISTER_UCHAR(Config + Offset, *Buffer);
            else
                *Buffer = READ_REGISTER_UCHAR(Config + Offset);
        }
    }
}

/**
 * @brief Accesses configuration space without looking at the function first.
 *
 * Reads of a missing function return all ones, the way a bus master abort
 * does. This backs the bus interface the ACPI driver hands to the PCI driver.
 */
ULONG
NTAPI
HalpPciRawConfig(
    _In_ ULONG Bus,
    _In_ PCI_SLOT_NUMBER Slot,
    _Inout_updates_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length,
    _In_ BOOLEAN Write)
{
    PUCHAR Config;

    if ((Offset >= HALP_ECAM_FUNCTION_SIZE) || (Length > (HALP_ECAM_FUNCTION_SIZE - Offset)))
        return 0;

    Config = HalpPciFunctionConfig(Bus, Slot);
    if (!Config)
    {
        if (!Write)
            RtlFillMemory(Buffer, Length, 0xFF);
        return Length;
    }

    HalpPciTransfer(Config, Buffer, Offset, Length, Write);
    return Length;
}

/**
 * @brief Reads the common configuration space of a function.
 *
 * A missing function reads back as the invalid vendor ID only, the same as
 * the legacy mechanism reports it.
 */
ULONG
NTAPI
HalpPciReadConfig(
    _In_ ULONG Bus,
    _In_ PCI_SLOT_NUMBER Slot,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length)
{
    PUCHAR Config;
    ULONG Missing;

    if (Offset >= sizeof(PCI_COMMON_CONFIG))
        return 0;
    Length = min(Length, sizeof(PCI_COMMON_CONFIG) - Offset);

    Config = HalpPciFunctionConfig(Bus, Slot);
    if (!Config || (READ_REGISTER_USHORT((PUSHORT)Config) == PCI_INVALID_VENDORID))
    {
        if (Offset >= sizeof(USHORT))
            return 0;

        Missing = min(Length, sizeof(USHORT) - Offset);
        RtlFillMemory(Buffer, Missing, 0xFF);
        return Missing;
    }

    HalpPciTransfer(Config, Buffer, Offset, Length, FALSE);
    return Length;
}

ULONG
NTAPI
HalpPciWriteConfig(
    _In_ ULONG Bus,
    _In_ PCI_SLOT_NUMBER Slot,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length)
{
    PUCHAR Config;

    if (Offset >= sizeof(PCI_COMMON_CONFIG))
        return 0;
    Length = min(Length, sizeof(PCI_COMMON_CONFIG) - Offset);

    Config = HalpPciFunctionConfig(Bus, Slot);
    if (!Config || (READ_REGISTER_USHORT((PUSHORT)Config) == PCI_INVALID_VENDORID))
        return 0;

    HalpPciTransfer(Config, Buffer, Offset, Length, TRUE);
    return Length;
}

ULONG
NTAPI
HalGetBusDataByOffset(
    _In_ BUS_DATA_TYPE BusDataType,
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length)
{
    PCI_SLOT_NUMBER Slot;

    if (BusDataType != PCIConfiguration)
        return 0;

    Slot.u.AsULONG = SlotNumber;
    return HalpPciReadConfig(BusNumber, Slot, Buffer, Offset, Length);
}

ULONG
NTAPI
HalGetBusData(
    _In_ BUS_DATA_TYPE BusDataType,
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length)
{
    return HalGetBusDataByOffset(BusDataType, BusNumber, SlotNumber, Buffer, 0, Length);
}

ULONG
NTAPI
HalSetBusDataByOffset(
    _In_ BUS_DATA_TYPE BusDataType,
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length)
{
    PCI_SLOT_NUMBER Slot;

    if (BusDataType != PCIConfiguration)
        return 0;

    Slot.u.AsULONG = SlotNumber;
    return HalpPciWriteConfig(BusNumber, Slot, Buffer, Offset, Length);
}

ULONG
NTAPI
HalSetBusData(
    _In_ BUS_DATA_TYPE BusDataType,
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length)
{
    return HalSetBusDataByOffset(BusDataType, BusNumber, SlotNumber, Buffer, 0, Length);
}

/* EOF */
