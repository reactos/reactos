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

                /* GICv2 has one CPU interface address, banked per processor */
                if (!HalpPlatform.GiccBase.QuadPart)
                    HalpPlatform.GiccBase.QuadPart = Gicc->PhysicalBaseAddress;

                /* Without a GICR entry every GICC carries its own redistributor */
                if (!HalpPlatform.GicrLength && Gicc->GicrBaseAddress)
                    HalpPlatform.GicrBase.QuadPart = Gicc->GicrBaseAddress;
                break;

            case MADT_TYPE_GICD:
                HalpPlatform.GicdBase.QuadPart = ((PHALP_MADT_GICD)Entry)->PhysicalBaseAddress;
                HalpPlatform.GicVersion = ((PHALP_MADT_GICD)Entry)->GicVersion;
                break;

            case MADT_TYPE_GICR:
                HalpPlatform.GicrBase.QuadPart = ((PHALP_MADT_GICR)Entry)->DiscoveryRangeBaseAddress;
                HalpPlatform.GicrLength = ((PHALP_MADT_GICR)Entry)->DiscoveryRangeLength;
                break;
        }

        Current += Entry->Length;
    }
}

/* The loader hands over the XSDT when there is one, the RSDT otherwise */
static
ULONG
HalpRootTableEntrySize(
    _In_ PDESCRIPTION_HEADER Root)
{
    return (Root->Signature == XSDT_SIGNATURE) ? sizeof(ULONG64) : sizeof(ULONG);
}

static
ULONG
HalpRootTableCount(
    _In_ PDESCRIPTION_HEADER Root)
{
    return (Root->Length - sizeof(*Root)) / HalpRootTableEntrySize(Root);
}

static
PHYSICAL_ADDRESS
HalpRootTableEntry(
    _In_ PDESCRIPTION_HEADER Root,
    _In_ ULONG Index)
{
    PUCHAR Entry = (PUCHAR)(Root + 1) + Index * HalpRootTableEntrySize(Root);
    PHYSICAL_ADDRESS Address;

    if (HalpRootTableEntrySize(Root) == sizeof(ULONG64))
        Address.QuadPart = *(ULONG64 UNALIGNED *)Entry;
    else
        Address.QuadPart = *(ULONG UNALIGNED *)Entry;

    return Address;
}

/* Only segment 0 is supported, the PCI driver sees a single root */
static
VOID
HalpParseMcfg(
    _In_ PMCFG_TABLE Mcfg)
{
    PMCFG_ALLOCATION Entry, End;

    End = (PMCFG_ALLOCATION)((PUCHAR)Mcfg + Mcfg->Header.Length);
    for (Entry = Mcfg->Allocation; Entry + 1 <= End; Entry++)
    {
        if ((Entry->PciSegmentGroup != 0) || (Entry->EndBusNumber < Entry->StartBusNumber))
            continue;

        HalpPlatform.EcamBase.QuadPart = Entry->BaseAddress;
        HalpPlatform.EcamStartBus = Entry->StartBusNumber;
        HalpPlatform.EcamEndBus = Entry->EndBusNumber;
        break;
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
        case MCFG_SIGNATURE:
            HalpParseMcfg((PMCFG_TABLE)Header);
            break;

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
    PHYSICAL_ADDRESS RootAddress;
    ULONG Count, i;

    HalpPlatform.TimerIntId = HALP_VIRTUAL_TIMER_INTID;

    RootAddress = HalpAcpiFindRootTable(LoaderBlock);
    if (!RootAddress.QuadPart)
    {
        DPRINT1("No ACPI tables from the loader\n");
        return;
    }

    HalpPlatform.RootTable = RootAddress;
    Root = HalpMapTable(RootAddress);
    if (!Root)
        return;

    Count = HalpRootTableCount(Root);
    for (i = 0; i < Count; i++)
    {
        Header = HalpMapTable(HalpRootTableEntry(Root, i));
        if (!Header)
            continue;

        HalpParseTable(Header);
        HalpUnmapTable(Header);
    }

    HalpUnmapTable(Root);
}

/**
 * @brief Finds a firmware table by its signature.
 *
 * The table stays mapped, callers keep it for the life of the system.
 */
PVOID
NTAPI
HalAcpiGetTable(
    _In_opt_ PLOADER_PARAMETER_BLOCK LoaderBlock,
    _In_ ULONG Signature)
{
    PDESCRIPTION_HEADER Root, Header;
    ULONG Count, i;

    UNREFERENCED_PARAMETER(LoaderBlock);

    if (!HalpPlatform.RootTable.QuadPart)
        return NULL;

    Root = HalpMapTable(HalpPlatform.RootTable);
    if (!Root)
        return NULL;

    Count = HalpRootTableCount(Root);
    for (i = 0; i < Count; i++)
    {
        Header = HalpMapTable(HalpRootTableEntry(Root, i));
        if (!Header)
            continue;

        if (Header->Signature == Signature)
        {
            HalpUnmapTable(Root);
            return Header;
        }

        HalpUnmapTable(Header);
    }

    HalpUnmapTable(Root);
    return NULL;
}
