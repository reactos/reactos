/*
 *  FreeLoader
 *
 *  Copyright (C) 2004  Eric Kohl
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(HWDETECT);

static
PPCI_IRQ_ROUTING_TABLE
GetPciIrqRoutingTable(VOID)
{
    PPCI_IRQ_ROUTING_TABLE Table;
    PUCHAR Ptr;
    ULONG Sum;
    ULONG i;

    Table = (PPCI_IRQ_ROUTING_TABLE)0xF0000;
    while ((ULONG_PTR)Table < 0x100000)
    {
        if (Table->Signature == 'RIP$')
        {
            TRACE("Found signature\n");

            if (Table->TableSize < FIELD_OFFSET(PCI_IRQ_ROUTING_TABLE, Slot) ||
                Table->TableSize % 16 != 0)
            {
                ERR("Invalid routing table size (%u) at 0x%p. Continue searching...\n", Table->TableSize, Table);
                Table = (PPCI_IRQ_ROUTING_TABLE)((ULONG_PTR)Table + 0x10);
                continue;
            }

            Ptr = (PUCHAR)Table;
            Sum = 0;
            for (i = 0; i < Table->TableSize; i++)
            {
                Sum += Ptr[i];
            }

            if ((Sum & 0xFF) != 0)
            {
                ERR("Invalid routing table checksum (%#lx) at 0x%p. Continue searching...\n", Sum & 0xFF, Table);
            }
            else
            {
                TRACE("Valid checksum (%#lx): found routing table at 0x%p\n", Sum & 0xFF, Table);
                return Table;
            }
        }

        Table = (PPCI_IRQ_ROUTING_TABLE)((ULONG_PTR)Table + 0x10);
    }

    ERR("No valid routing table found!\n");

    return NULL;
}

ULONG
PciReadConfigDword(
    _In_ UCHAR Bus,
    _In_ UCHAR Device,
    _In_ UCHAR Function,
    _In_ UCHAR Register)
{
    PCI_TYPE1_CFG_BITS Cfg;

    Cfg.u.AsULONG = 0;
    Cfg.u.bits.Enable = 1;
    Cfg.u.bits.BusNumber = Bus;
    Cfg.u.bits.DeviceNumber = Device;
    Cfg.u.bits.FunctionNumber = Function;
    Cfg.u.bits.RegisterNumber = Register & ~3;

    WRITE_PORT_ULONG(PCI_TYPE1_ADDRESS_PORT, Cfg.u.AsULONG);
    return READ_PORT_ULONG((PULONG)PCI_TYPE1_DATA_PORT);
}

BOOLEAN
PciScanFunction(
    _In_ UCHAR Bus,
    _In_ UCHAR Device,
    _In_ UCHAR Function,
    _Inout_updates_(PCI_MAX_BUSES) BOOLEAN *ScannedBuses,
    _Inout_updates_(PCI_MAX_BUSES) UCHAR *PendingBuses,
    _Inout_ ULONG *PendingCount)
{
    ULONG VendorDevice, HeaderTypeDword, BridgeBusNumbers;
    UCHAR HeaderType, SecondaryBus, SubordinateBus, b;

    VendorDevice = PciReadConfigDword(Bus, Device, Function, 0x00);
    if ((VendorDevice & 0xFFFF) == 0xFFFF)
        return FALSE;

    HeaderTypeDword = PciReadConfigDword(Bus, Device, Function, 0x0C);
    HeaderType = (UCHAR)((HeaderTypeDword >> 16) & 0xFF);

    if ((HeaderType & PCI_HEADER_TYPE_MASK) == PCI_HEADER_TYPE_BRIDGE)
    {
        BridgeBusNumbers = PciReadConfigDword(Bus, Device, Function, 0x18);
        SecondaryBus = (UCHAR)((BridgeBusNumbers >> 8) & 0xFF);
        SubordinateBus = (UCHAR)((BridgeBusNumbers >> 16) & 0xFF);

        if (SecondaryBus != 0 && SecondaryBus != Bus && SubordinateBus >= SecondaryBus)
        {
            for (b = SecondaryBus; b <= SubordinateBus; b++)
            {
                if (!ScannedBuses[b] && (*PendingCount < PCI_MAX_BUSES))
                {
                    ScannedBuses[b] = TRUE;
                    PendingBuses[(*PendingCount)++] = b;
                }

                if (b == 255)
                    break; /* avoid UCHAR wraparound */
            }
        }
    }

    return TRUE;
}

#ifndef UEFIBOOT
BOOLEAN
PcFindPciBios(PPCI_REGISTRY_INFO BusData)
{
    REGS  RegsIn;
    REGS  RegsOut;

    RegsIn.b.ah = 0xB1; /* Subfunction B1h */
    RegsIn.b.al = 0x01; /* PCI BIOS present */

    Int386(0x1A, &RegsIn, &RegsOut);

    if (INT386_SUCCESS(RegsOut) &&
        (RegsOut.d.edx == ' ICP') &&
        (RegsOut.b.ah == 0))
    {
        TRACE("Found PCI bios\n");

        TRACE("AL: %x\n", RegsOut.b.al);
        TRACE("BH: %x\n", RegsOut.b.bh);
        TRACE("BL: %x\n", RegsOut.b.bl);
        TRACE("CL: %x\n", RegsOut.b.cl);

        BusData->NoBuses = RegsOut.b.cl + 1;
        BusData->MajorRevision = RegsOut.b.bh;
        BusData->MinorRevision = RegsOut.b.bl;
        BusData->HardwareMechanism = RegsOut.b.al;

        return TRUE;
    }

    TRACE("No PCI bios found\n");

    return FALSE;
}
#endif

static
VOID
DetectPciIrqRoutingTable(PCONFIGURATION_COMPONENT_DATA BusKey)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PPCI_IRQ_ROUTING_TABLE Table;
    PCONFIGURATION_COMPONENT_DATA TableKey;
    ULONG Size;

    Table = GetPciIrqRoutingTable();
    if (Table != NULL)
    {
        TRACE("Table size: %u\n", Table->TableSize);

        /* Set 'Configuration Data' value */
        Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors[2]) + Table->TableSize;
        PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
        if (PartialResourceList == NULL)
        {
            ERR("Failed to allocate resource descriptor\n");
            return;
        }

        /* Initialize resource descriptor */
        RtlZeroMemory(PartialResourceList, Size);
        PartialResourceList->Version = 1;
        PartialResourceList->Revision = 1;
        PartialResourceList->Count = 2;

        PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
        PartialDescriptor->Type = CmResourceTypeBusNumber;
        PartialDescriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        PartialDescriptor->u.BusNumber.Start = 0;
        PartialDescriptor->u.BusNumber.Length = 1;

        PartialDescriptor = &PartialResourceList->PartialDescriptors[1];
        PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
        PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
        PartialDescriptor->u.DeviceSpecificData.DataSize = Table->TableSize;

        RtlCopyMemory(&PartialResourceList->PartialDescriptors[2],
                      Table, Table->TableSize);

        FldrCreateComponentKey(BusKey,
                               PeripheralClass,
                               RealModeIrqRoutingTable,
                               0,
                               0,
                               0xFFFFFFFF,
                               "PCI Real-mode IRQ Routing Table",
                               PartialResourceList,
                               Size,
                               &TableKey);
    }
}

VOID
DetectPciBios(
    _In_ PCONFIGURATION_COMPONENT_DATA SystemKey,
    _Inout_ PULONG BusNumber,
    _In_ FIND_PCI_BIOS MachFindPciBios)
{
    PCI_REGISTRY_INFO BusData;
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PCONFIGURATION_COMPONENT_DATA BiosKey;
    PCONFIGURATION_COMPONENT_DATA BusKey;
    ULONG Size;
    ULONG i;

    /* Report the PCI BIOS */
    if (!MachFindPciBios(&BusData))
        return;

    /* Set 'Configuration Data' value */
    Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors);
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate resource descriptor\n");
        return;
    }

    /* Initialize resource descriptor */
    RtlZeroMemory(PartialResourceList, Size);

    /* Create new bus key */
    FldrCreateComponentKey(SystemKey,
                           AdapterClass,
                           MultiFunctionAdapter,
                           0,
                           0,
                           0xFFFFFFFF,
                           "PCI BIOS",
                           PartialResourceList,
                           Size,
                           &BiosKey);

    /* Increment bus number */
    (*BusNumber)++;

    DetectPciIrqRoutingTable(BiosKey);

    /* Report PCI buses */
    for (i = 0; i < (ULONG)BusData.NoBuses; i++)
    {
        /* Check if this is the first bus */
        if (i == 0)
        {
            /* Set 'Configuration Data' value */
            Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors[1]) +
                   sizeof(BusData);
            PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
            if (!PartialResourceList)
            {
                ERR("Failed to allocate resource descriptor! Ignoring remaining PCI buses. (i = %lu, NoBuses = %lu)\n",
                    i, (ULONG)BusData.NoBuses);
                return;
            }

            /* Initialize resource descriptor */
            RtlZeroMemory(PartialResourceList, Size);
            PartialResourceList->Version = 1;
            PartialResourceList->Revision = 1;
            PartialResourceList->Count = 1;

            PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
            PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
            PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
            PartialDescriptor->u.DeviceSpecificData.DataSize = sizeof(BusData);

            RtlCopyMemory(&PartialResourceList->PartialDescriptors[1],
                          &BusData, sizeof(BusData));
        }
        else
        {
            /* Set 'Configuration Data' value */
            Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors);
            PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
            if (!PartialResourceList)
            {
                ERR("Failed to allocate resource descriptor! Ignoring remaining PCI buses. (i = %lu, NoBuses = %lu)\n",
                    i, (ULONG)BusData.NoBuses);
                return;
            }

            /* Initialize resource descriptor */
            RtlZeroMemory(PartialResourceList, Size);
        }

        /* Create the bus key */
        FldrCreateComponentKey(SystemKey,
                               AdapterClass,
                               MultiFunctionAdapter,
                               0,
                               0,
                               0xFFFFFFFF,
                               "PCI",
                               PartialResourceList,
                               Size,
                               &BusKey);

        /* Increment bus number */
        (*BusNumber)++;
    }
}

/* EOF */
