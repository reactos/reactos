/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Source File for MADT Table parsing
 * COPYRIGHT:   Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 *              Copyright 2023 Serge Gautherie <reactos-git_serge_171003@gautherie.fr>
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#include <acpi.h>
/* ACPI_BIOS_ERROR defined in acoutput.h and bugcodes.h */
#undef ACPI_BIOS_ERROR
#include <smp.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

HALP_APIC_INFO_TABLE HalpApicInfoTable;

// ACPI_MADT_LOCAL_APIC.LapicFlags masks
#define LAPIC_FLAG_ENABLED          0x00000001
#define LAPIC_FLAG_ONLINE_CAPABLE   0x00000002
// Bits 2-31 are reserved.

PROCESSOR_IDENTITY HalpProcessorIdentity[MAXIMUM_PROCESSORS];

extern ULONG HalpPicVectorRedirect[16];

ULONG
HalpGetCurrentProcessorHwID(VOID);

/* FUNCTIONS ******************************************************************/

// Note: HalpParseApicTables() is called early, so its DPRINT*() do nothing.
VOID
HalpParseApicTables(
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ACPI_TABLE_MADT *MadtTable;
    ACPI_SUBTABLE_HEADER *AcpiHeader;
    ULONG_PTR TableEnd;

    MadtTable = HalAcpiGetTable(LoaderBlock, APIC_SIGNATURE);
    if (!MadtTable)
    {
        DPRINT1("MADT table not found\n");
        return;
    }

    if (MadtTable->Header.Length < sizeof(*MadtTable))
    {
        DPRINT1("Length is too short: %p, %u\n", MadtTable, MadtTable->Header.Length);
        return;
    }

    DPRINT("MADT table: Address %08X, Flags %08X\n", MadtTable->Address, MadtTable->Flags);


    HalpApicInfoTable.LocalApicPA = MadtTable->Address;
    AcpiHeader = (ACPI_SUBTABLE_HEADER *)((ULONG_PTR)MadtTable + sizeof(*MadtTable));
    TableEnd = (ULONG_PTR)MadtTable + MadtTable->Header.Length;
    DPRINT(" MadtTable %p, subtables %p - %p\n", MadtTable, AcpiHeader, (PVOID)TableEnd);

    while ((ULONG_PTR)(AcpiHeader + 1) <= TableEnd)
    {
        if (AcpiHeader->Length < sizeof(*AcpiHeader))
        {
            DPRINT1("Length is too short: %p, %u\n", AcpiHeader, AcpiHeader->Length);
            return;
        }

        if ((ULONG_PTR)AcpiHeader + AcpiHeader->Length > TableEnd)
        {
            DPRINT1("Length mismatch: %p, %u, %p\n",
                     AcpiHeader, AcpiHeader->Length, (PVOID)TableEnd);
            return;
        }

        switch (AcpiHeader->Type)
        {
            case ACPI_MADT_TYPE_LOCAL_APIC:
            {
                ACPI_MADT_LOCAL_APIC *LocalApic = (ACPI_MADT_LOCAL_APIC *)AcpiHeader;

                if (AcpiHeader->Length != sizeof(*LocalApic))
                {
                    DPRINT1("Type/Length mismatch: %p, %u\n", AcpiHeader, AcpiHeader->Length);
                    return;
                }

                DPRINT(" Local Apic, Processor %lu: ProcessorId %u, Id %u, LapicFlags %08X\n",
                         HalpApicInfoTable.ProcessorCount,
                         LocalApic->ProcessorId, LocalApic->Id, LocalApic->LapicFlags);

                if (!(LocalApic->LapicFlags & (LAPIC_FLAG_ONLINE_CAPABLE | LAPIC_FLAG_ENABLED)))
                {
                    DPRINT("  Ignored: unusable\n");
                    break;
                }

                if (HalpApicInfoTable.ProcessorCount == MAXIMUM_PROCESSORS)
                {
                    DPRINT1("  Skipped: array is full\n");
                    // We assume ignoring this processor is acceptable, until proven otherwise.
                    break;
                }

                // Note: ProcessorId and Id are not validated in any way (yet).
                HalpProcessorIdentity[HalpApicInfoTable.ProcessorCount].ProcessorId =
                    LocalApic->ProcessorId;
                HalpProcessorIdentity[HalpApicInfoTable.ProcessorCount].LapicId = LocalApic->Id;

                /* Check against the processor running this code, which is only the BSP. */
                if (LocalApic->Id == HalpGetCurrentProcessorHwID())
                {
                    HalpProcessorIdentity[HalpApicInfoTable.ProcessorCount].BSPCheck = TRUE;
                    HalpProcessorIdentity[HalpApicInfoTable.ProcessorCount].ProcessorStarted = TRUE;
                }

                HalpApicInfoTable.ProcessorCount++;

                break;
            }
            case ACPI_MADT_TYPE_IO_APIC:
            {
                ACPI_MADT_IO_APIC *IoApic = (ACPI_MADT_IO_APIC *)AcpiHeader;

                if (AcpiHeader->Length != sizeof(*IoApic))
                {
                    DPRINT1("Type/Length mismatch: %p, %u\n", AcpiHeader, AcpiHeader->Length);
                    return;
                }

                DPRINT(" IoApic: Id %u, Address %08X, GlobalIrqBase %08X\n",
                         IoApic->Id, IoApic->Address, IoApic->GlobalIrqBase);

                // Ensure HalpApicInfoTable.IOAPICCount consistency.
                if (HalpApicInfoTable.IoApicPA[IoApic->Id] != 0)
                {
                    DPRINT1("Id duplication: %p, %u\n", IoApic, IoApic->Id);
                    return;
                }

                // Note: Address and GlobalIrqBase are not validated in any way (yet).
                HalpApicInfoTable.IoApicPA[IoApic->Id] = IoApic->Address;
                HalpApicInfoTable.IoApicIrqBase[IoApic->Id] = IoApic->GlobalIrqBase;

                HalpApicInfoTable.IOAPICCount++;

                break;
            }
            case ACPI_MADT_TYPE_INTERRUPT_OVERRIDE:
            {
                ACPI_MADT_INTERRUPT_OVERRIDE *InterruptOverride =
                    (ACPI_MADT_INTERRUPT_OVERRIDE *)AcpiHeader;

                if (AcpiHeader->Length != sizeof(*InterruptOverride))
                {
                    DPRINT1("Type/Length mismatch: %p, %u\n", AcpiHeader, AcpiHeader->Length);
                    return;
                }

                /*
                 * We use this information to make sure IOAPIC knows the difference between the
                 * legacy PIC IRQs and the associated GSI with them.
                 */
                DPRINT1(" Interrupt Override: Bus %u, SourceIrq %u, GlobalIrq %08X, IntiFlags %04X / UNIMPLEMENTED\n",
                         InterruptOverride->Bus, InterruptOverride->SourceIrq,
                         InterruptOverride->GlobalIrq, InterruptOverride->IntiFlags);

                if (InterruptOverride->Bus != 0) // 0 = ISA
                {
                    DPRINT1("Invalid Bus: %p, %u\n", InterruptOverride, InterruptOverride->Bus);
                    return;
                }

                break;
            }
            default:
            {
                DPRINT1(" UNIMPLEMENTED: Type %u, Length %u\n",
                       AcpiHeader->Type, AcpiHeader->Length);
                return;
            }
        }

        AcpiHeader = (ACPI_SUBTABLE_HEADER *)((ULONG_PTR)AcpiHeader + AcpiHeader->Length);
    }

    if ((ULONG_PTR)AcpiHeader != TableEnd)
    {
        DPRINT("Length mismatch: %p, %p, %p\n", MadtTable, AcpiHeader, (PVOID)TableEnd);
        return;
    }

    HalpPrintApicTables();
}

VOID
HalpPrintApicTables(VOID)
{
#if DBG
    ULONG i;

    DPRINT1("Physical processor count: %lu\n", HalpApicInfoTable.ProcessorCount);
    for (i = 0; i < HalpApicInfoTable.ProcessorCount; i++)
    {
        DPRINT1(" Processor %lu: ProcessorId %u, LapicId %u, ProcessorStarted %u, BSPCheck %u, ProcessorPrcb %p\n",
                i,
                HalpProcessorIdentity[i].ProcessorId,
                HalpProcessorIdentity[i].LapicId,
                HalpProcessorIdentity[i].ProcessorStarted,
                HalpProcessorIdentity[i].BSPCheck,
                HalpProcessorIdentity[i].ProcessorPrcb);
    }
#endif
}
