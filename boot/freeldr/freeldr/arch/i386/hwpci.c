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

FIND_PCI_BIOS FindPciBios = NULL;

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
        Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors) +
               2 * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) + Table->TableSize;
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

        memcpy(&PartialResourceList->PartialDescriptors[2],
               Table,
               Table->TableSize);

        FldrCreateComponentKey(BusKey,
                               PeripheralClass,
                               RealModeIrqRoutingTable,
                               0x0,
                               0x0,
                               0xFFFFFFFF,
                               "PCI Real-mode IRQ Routing Table",
                               PartialResourceList,
                               Size,
                               &TableKey);
    }
}


VOID
DetectPciBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PCI_REGISTRY_INFO BusData;
    PCONFIGURATION_COMPONENT_DATA BiosKey;
    ULONG Size;
    PCONFIGURATION_COMPONENT_DATA BusKey;
    ULONG i;

    /* Report the PCI BIOS */
    if (FindPciBios(&BusData))
    {
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
                               0x0,
                               0x0,
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
                Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST,
                                    PartialDescriptors) +
                       sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) +
                       sizeof(PCI_REGISTRY_INFO);
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
                PartialDescriptor->u.DeviceSpecificData.DataSize = sizeof(PCI_REGISTRY_INFO);
                memcpy(&PartialResourceList->PartialDescriptors[1],
                       &BusData,
                       sizeof(PCI_REGISTRY_INFO));
            }
            else
            {
                /* Set 'Configuration Data' value */
                Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST,
                                    PartialDescriptors);
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
                                   0x0,
                                   0x0,
                                   0xFFFFFFFF,
                                   "PCI",
                                   PartialResourceList,
                                   Size,
                                   &BusKey);

            /* Increment bus number */
            (*BusNumber)++;
        }
    }
}

/* EOF */
