/*
 * PROJECT:     ReactOS ARM64 HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Platform discovery through the ACPI tables
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

HALP_PLATFORM HalpPlatform;
ULONG64 HalpProcessorMpidr[MAXIMUM_PROCESSORS];

/* FUNCTIONS *****************************************************************/

static
PHYSICAL_ADDRESS
HalpAcpiFindRootTable(
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PCONFIGURATION_COMPONENT_DATA Entry;
    PCONFIGURATION_COMPONENT_DATA Next = NULL;
    PCM_PARTIAL_RESOURCE_LIST ResourceList;
    PACPI_BIOS_MULTI_NODE Node;
    PHYSICAL_ADDRESS Root;

    Root.QuadPart = 0;

    for (Entry = KeFindConfigurationNextEntry(LoaderBlock->ConfigurationRoot,
                                              AdapterClass,
                                              MultiFunctionAdapter,
                                              NULL,
                                              &Next);
         Entry != NULL;
         Entry = KeFindConfigurationNextEntry(LoaderBlock->ConfigurationRoot,
                                              AdapterClass,
                                              MultiFunctionAdapter,
                                              NULL,
                                              &Next))
    {
        Next = Entry;
        if (_stricmp(Entry->ComponentEntry.Identifier, "ACPI BIOS"))
            continue;

        /* The node data follows the resource list header */
        ResourceList = Entry->ConfigurationData;
        Node = (PACPI_BIOS_MULTI_NODE)(ResourceList + 1);
        Root = Node->RsdtAddress;
        break;
    }

    return Root;
}

/**
 * @brief Maps a whole ACPI table, the caller unmaps it with HalpUnmapTable.
 */
static
PDESCRIPTION_HEADER
HalpMapTable(
    _In_ PHYSICAL_ADDRESS PhysicalAddress)
{
    PDESCRIPTION_HEADER Header;
    ULONG Length;

    Header = HalpMapPhysicalMemory(PhysicalAddress, 1, FALSE);
    if (!Header)
        return NULL;

    Length = Header->Length;
    HalpUnmapVirtualAddress(Header, 1);

    if (Length < sizeof(*Header))
        return NULL;

    return HalpMapPhysicalMemory(PhysicalAddress, BYTES_TO_PAGES(Length), FALSE);
}

static
VOID
HalpUnmapTable(
    _In_ PDESCRIPTION_HEADER Header)
{
    HalpUnmapVirtualAddress(Header, BYTES_TO_PAGES(Header->Length));
}

static
VOID
HalpParseMadt(
    _In_ PHALP_MADT Madt)
{
    PHALP_MADT_ENTRY Entry;
    PUCHAR Current, End;
    PHALP_MADT_GICC Gicc;
    ULONG Cpu = 0;

    Current = Madt->Entries;
    End = (PUCHAR)Madt + Madt->Header.Length;

    while (Current + sizeof(*Entry) <= End)
    {
        Entry = (PHALP_MADT_ENTRY)Current;
        if (Entry->Length < sizeof(*Entry))
            break;

        switch (Entry->Type)
        {
            case MADT_TYPE_GICC:
                Gicc = (PHALP_MADT_GICC)Entry;

                /* Bit 0 is the enabled flag */
                if (!(Gicc->Flags & 1) || (Cpu >= MAXIMUM_PROCESSORS))
                    break;

                HalpProcessorMpidr[Cpu++] = Gicc->Mpidr;

                /* Without a GICR entry every GICC carries its own redistributor */
                if (!HalpPlatform.GicrLength && Gicc->GicrBaseAddress)
                    HalpPlatform.GicrBase.QuadPart = Gicc->GicrBaseAddress;
                break;

            case MADT_TYPE_GICD:
                HalpPlatform.GicdBase.QuadPart = ((PHALP_MADT_GICD)Entry)->PhysicalBaseAddress;
                break;

            case MADT_TYPE_GICR:
                HalpPlatform.GicrBase.QuadPart = ((PHALP_MADT_GICR)Entry)->DiscoveryRangeBaseAddress;
                HalpPlatform.GicrLength = ((PHALP_MADT_GICR)Entry)->DiscoveryRangeLength;
                break;
        }

        Current += Entry->Length;
    }
}

static
VOID
HalpParseTable(
    _In_ PDESCRIPTION_HEADER Header)
{
    PHALP_GTDT Gtdt;
    PFADT Fadt;

    switch (Header->Signature)
    {
        case MADT_SIGNATURE:
            HalpParseMadt((PHALP_MADT)Header);
            break;

        case GTDT_SIGNATURE:
            Gtdt = (PHALP_GTDT)Header;
            if (Gtdt->VirtualEl1TimerGsiv)
                HalpPlatform.TimerIntId = Gtdt->VirtualEl1TimerGsiv;
            break;

        case FADT_SIGNATURE:
            Fadt = (PFADT)Header;
            if (Header->Length >= FIELD_OFFSET(FADT, minor_revision))
                HalpPlatform.ArmBootFlags = Fadt->arm_boot_arch;
            break;
    }
}

/**
 * @brief Collects the GIC, timer and PSCI details from the firmware tables.
 */
VOID
NTAPI
HalpAcpiInitialize(
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PDESCRIPTION_HEADER Root, Header;
    PHYSICAL_ADDRESS RootAddress, TableAddress;
    ULONG Count, EntrySize, i;
    PUCHAR Entries;

    HalpPlatform.TimerIntId = HALP_VIRTUAL_TIMER_INTID;

    RootAddress = HalpAcpiFindRootTable(LoaderBlock);
    if (!RootAddress.QuadPart)
    {
        DPRINT1("No ACPI tables from the loader\n");
        return;
    }

    Root = HalpMapTable(RootAddress);
    if (!Root)
        return;

    /* The loader hands over the XSDT when there is one, the RSDT otherwise */
    EntrySize = (Root->Signature == XSDT_SIGNATURE) ? sizeof(ULONG64) : sizeof(ULONG);
    Count = (Root->Length - sizeof(*Root)) / EntrySize;
    Entries = (PUCHAR)(Root + 1);

    for (i = 0; i < Count; i++)
    {
        if (EntrySize == sizeof(ULONG64))
            TableAddress.QuadPart = *(ULONG64 UNALIGNED *)(Entries + i * EntrySize);
        else
            TableAddress.QuadPart = *(ULONG UNALIGNED *)(Entries + i * EntrySize);

        Header = HalpMapTable(TableAddress);
        if (!Header)
            continue;

        HalpParseTable(Header);
        HalpUnmapTable(Header);
    }

    HalpUnmapTable(Root);
}
