/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PCI BIOS detection routines
 * COPYRIGHT:   Copyright 2004 Eric Kohl <eric.kohl@reactos.org>
 */

#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(HWDETECT);

/*
 * Specification:
 * - https://pcisig.com/PCIConventional/Specs/Firmware/Bios_2.1
 * - http://www.o3one.org/hwdocs/bios_doc/pci_bios_21.pdf
 * - https://hackipedia.org/browse.cgi/Computer/Platform/PC,%20IBM%20compatible/Busses/PCI/BIOS
 */
static
BOOLEAN
PcFindPciBios(
    _Out_ PPCI_REGISTRY_INFO BusData)
{
    REGS RegsIn;
    REGS RegsOut;

    RegsIn.b.ah = 0xB1; /* Subfunction B1h */
    RegsIn.b.al = 0x01; /* PCI BIOS present */

    Int386(0x1A, &RegsIn, &RegsOut);

    if (INT386_SUCCESS(RegsOut) &&
        (RegsOut.d.edx == ' ICP') &&
        (RegsOut.b.ah == 0))
    {
        TRACE("Found PCI BIOS\n");

        TRACE("AL: %x\n", RegsOut.b.al);
        TRACE("BH: %x\n", RegsOut.b.bh);
        TRACE("BL: %x\n", RegsOut.b.bl);
        TRACE("CL: %x\n", RegsOut.b.cl);

        BusData->MajorRevision = RegsOut.b.bh;
        BusData->MinorRevision = RegsOut.b.bl;
        BusData->NoBuses = RegsOut.b.cl + 1;
        BusData->HardwareMechanism = RegsOut.b.al;
        return TRUE;
    }

    TRACE("No PCI BIOS found\n");
    return FALSE;
}

/*
 * NOTE: The PCI IRQ Routing Table is a legacy pre-ACPI mechanism that serves
 * to dynamically route PCI interrupts to interrupt requests (IRQs)[^1][^2][^3].
 * On the other hand, on ACPI-aware systems, ACPI-specific methods are used[^4][^5].
 *
 * References:
 * [^1]: https://web.archive.org/web/20101230191251/https://www.microsoft.com/whdc/archive/pciirq.mspx
 * [^2]: https://www.betaarchive.com/wiki/index.php/Microsoft_KB_Archive/182604
 * [^3]: https://lisans.cozum.info.tr/networking/Interrupt%20sharing%20on%20PCI-devices.htm
 * [^4]: https://web.archive.org/web/20111012153449/http://msdn.microsoft.com/en-us/windows/hardware/gg454523
 * [^5]: https://github.com/jaykhandkar/pci-interrupt-routing
 */
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

static
VOID
DetectPciIrqRoutingTable(
    _In_ PCONFIGURATION_COMPONENT_DATA BusKey)
{
    PPCI_IRQ_ROUTING_TABLE Table;
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PCONFIGURATION_COMPONENT_DATA TableKey;
    ULONG Size;

    Table = GetPciIrqRoutingTable();
    if (!Table)
        return;

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

BOOLEAN
PcDetectPciBus(
    _In_ PCONFIGURATION_COMPONENT_DATA SystemKey,
    _Inout_ PULONG BusNumber,
    _Out_ PPCI_REGISTRY_INFO BusData)
{
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCONFIGURATION_COMPONENT_DATA BiosKey;
    ULONG Size;

    /* Report the PCI BIOS */
    if (!PcFindPciBios(BusData))
        return FALSE;

    /* Set 'Configuration Data' value */
    Size = FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors);
    PartialResourceList = FrLdrHeapAlloc(Size, TAG_HW_RESOURCE_LIST);
    if (PartialResourceList == NULL)
    {
        ERR("Failed to allocate resource descriptor\n");
        return FALSE;
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
    return TRUE;
}

/* EOF */
