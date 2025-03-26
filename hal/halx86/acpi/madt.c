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

// See HalpParseApicTables(). Only enable this to local-debug it.
// That needs, for example, to test-call the function later or to use the "FrLdrDbgPrint" hack.
#if DBG && 0
    #define DPRINT01        DPRINT1
    #define DPRINT00        DPRINT
#else
#if defined(_MSC_VER)
    #define DPRINT01        __noop
    #define DPRINT00        __noop
#else
    #define DPRINT01(...)   do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
    #define DPRINT00(...)   do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#endif // _MSC_VER
#endif // DBG && 0

/* GLOBALS ********************************************************************/

HALP_APIC_INFO_TABLE HalpApicInfoTable;

// ACPI_MADT_LOCAL_APIC.LapicFlags masks
#define LAPIC_FLAG_ENABLED          0x00000001
#define LAPIC_FLAG_ONLINE_CAPABLE   0x00000002
// Bits 2-31 are reserved.

static PROCESSOR_IDENTITY HalpStaticProcessorIdentity[MAXIMUM_PROCESSORS];
const PPROCESSOR_IDENTITY HalpProcessorIdentity = HalpStaticProcessorIdentity;

#if 0
extern ULONG HalpPicVectorRedirect[16];
#endif

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
        DPRINT01("MADT table not found\n");
        return;
    }

    if (MadtTable->Header.Length < sizeof(*MadtTable))
    {
        DPRINT01("Length is too short: %p, %u\n", MadtTable, MadtTable->Header.Length);
        return;
    }

    DPRINT00("MADT table: Address %08X, Flags %08X\n", MadtTable->Address, MadtTable->Flags);

#if 1

    // TODO: We support only legacy APIC for now
    HalpApicInfoTable.ApicMode = HALP_APIC_MODE_LEGACY;
    // TODO: What about 'MadtTable->Flags & ACPI_MADT_PCAT_COMPAT'?

#else // TODO: Is that correct?

    if ((MadtTable->Flags & ACPI_MADT_PCAT_COMPAT) == ACPI_MADT_DUAL_PIC)
    {
        HalpApicInfoTable.ApicMode = HALP_APIC_MODE_LEGACY;
    }
    else // if ((MadtTable->Flags & ACPI_MADT_PCAT_COMPAT) == ACPI_MADT_MULTIPLE_APIC)
    {
#if 1
        DPRINT01("ACPI_MADT_MULTIPLE_APIC support is UNIMPLEMENTED\n");
        return;
#else
        HalpApicInfoTable.ApicMode = HALP_APIC_MODE_xyz;
#endif
    }

#endif

    HalpApicInfoTable.LocalApicPA = MadtTable->Address;

    AcpiHeader = (ACPI_SUBTABLE_HEADER *)((ULONG_PTR)MadtTable + sizeof(*MadtTable));
    TableEnd = (ULONG_PTR)MadtTable + MadtTable->Header.Length;
    DPRINT00(" MadtTable %p, subtables %p - %p\n", MadtTable, AcpiHeader, (PVOID)TableEnd);

    while ((ULONG_PTR)(AcpiHeader + 1) <= TableEnd)
    {
        if (AcpiHeader->Length < sizeof(*AcpiHeader))
        {
            DPRINT01("Length is too short: %p, %u\n", AcpiHeader, AcpiHeader->Length);
            return;
        }

        if ((ULONG_PTR)AcpiHeader + AcpiHeader->Length > TableEnd)
        {
            DPRINT01("Length mismatch: %p, %u, %p\n",
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
                    DPRINT01("Type/Length mismatch: %p, %u\n", AcpiHeader, AcpiHeader->Length);
                    return;
                }

                DPRINT00(" Local Apic, Processor %lu: ProcessorId %u, Id %u, LapicFlags %08X\n",
                         HalpApicInfoTable.ProcessorCount,
                         LocalApic->ProcessorId, LocalApic->Id, LocalApic->LapicFlags);

                if (!(LocalApic->LapicFlags & (LAPIC_FLAG_ONLINE_CAPABLE | LAPIC_FLAG_ENABLED)))
                {
                    DPRINT00("  Ignored: unusable\n");
                    break;
                }

                if (HalpApicInfoTable.ProcessorCount == _countof(HalpStaticProcessorIdentity))
                {
                    DPRINT00("  Skipped: array is full\n");
                    // We assume ignoring this processor is acceptable, until proven otherwise.
                    break;
                }

                // Note: ProcessorId and Id are not validated in any way (yet).
                HalpProcessorIdentity[HalpApicInfoTable.ProcessorCount].ProcessorId =
                    LocalApic->ProcessorId;
                HalpProcessorIdentity[HalpApicInfoTable.ProcessorCount].LapicId = LocalApic->Id;

                HalpApicInfoTable.ProcessorCount++;

                break;
            }
            case ACPI_MADT_TYPE_IO_APIC:
            {
                ACPI_MADT_IO_APIC *IoApic = (ACPI_MADT_IO_APIC *)AcpiHeader;

                if (AcpiHeader->Length != sizeof(*IoApic))
                {
                    DPRINT01("Type/Length mismatch: %p, %u\n", AcpiHeader, AcpiHeader->Length);
                    return;
                }

                DPRINT00(" Io Apic: Id %u, Address %08X, GlobalIrqBase %08X\n",
                         IoApic->Id, IoApic->Address, IoApic->GlobalIrqBase);

                // Ensure HalpApicInfoTable.IOAPICCount consistency.
                if (HalpApicInfoTable.IoApicPA[IoApic->Id] != 0)
                {
                    DPRINT01("Id duplication: %p, %u\n", IoApic, IoApic->Id);
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
                    DPRINT01("Type/Length mismatch: %p, %u\n", AcpiHeader, AcpiHeader->Length);
                    return;
                }

                DPRINT00(" Interrupt Override: Bus %u, SourceIrq %u, GlobalIrq %08X, IntiFlags %04X / UNIMPLEMENTED\n",
                         InterruptOverride->Bus, InterruptOverride->SourceIrq,
                         InterruptOverride->GlobalIrq, InterruptOverride->IntiFlags);

                if (InterruptOverride->Bus != 0) // 0 = ISA
                {
                    DPRINT01("Invalid Bus: %p, %u\n", InterruptOverride, InterruptOverride->Bus);
                    return;
                }

#if 1
                // TODO: Implement it.
#else // TODO: Is that correct?
                if (InterruptOverride->SourceIrq > _countof(HalpPicVectorRedirect))
                {
                    DPRINT01("Invalid SourceIrq: %p, %u\n",
                             InterruptOverride, InterruptOverride->SourceIrq);
                    return;
                }

                // Note: GlobalIrq is not validated in any way (yet).
                HalpPicVectorRedirect[InterruptOverride->SourceIrq] = InterruptOverride->GlobalIrq;
                // TODO: What about 'InterruptOverride->IntiFlags'?
#endif

                break;
            }
            default:
            {
                DPRINT01(" UNIMPLEMENTED: Type %u, Length %u\n",
                         AcpiHeader->Type, AcpiHeader->Length);
                return;
            }
        }

        AcpiHeader = (ACPI_SUBTABLE_HEADER *)((ULONG_PTR)AcpiHeader + AcpiHeader->Length);
    }

    if ((ULONG_PTR)AcpiHeader != TableEnd)
    {
        DPRINT01("Length mismatch: %p, %p, %p\n", MadtTable, AcpiHeader, (PVOID)TableEnd);
        return;
    }
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
