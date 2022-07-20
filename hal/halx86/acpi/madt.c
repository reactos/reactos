/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Source File for MADT Table parsing
 * COPYRIGHT:   Copyright 2021 Justin Miller <justinmiller100@gmail.com>
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

PROCESSOR_IDENTITY HalpStaticProcessorIdentity[MAXIMUM_PROCESSORS] = {{0}};
PPROCESSOR_IDENTITY HalpProcessorIdentity = NULL;
HALP_APIC_INFO_TABLE HalpApicInfoTable;
ACPI_TABLE_MADT *MadtTable;
ACPI_SUBTABLE_HEADER *AcpiHeader;
ACPI_MADT_LOCAL_APIC *LocalApic;

/* FUNCTIONS ******************************************************************/

VOID
HalpParseApicTables(
    _In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ULONG_PTR TableEnd;
    ULONG ValidProcessorCount;

    /* We only support legacy APIC for now, this will be updated in the future */
    HalpApicInfoTable.ApicMode = 0x10;
    MadtTable = HalAcpiGetTable(LoaderBlock, 'CIPA');

    AcpiHeader = (ACPI_SUBTABLE_HEADER*)MadtTable;
    AcpiHeader->Length = sizeof(ACPI_TABLE_MADT);
    TableEnd = (ULONG_PTR)MadtTable + MadtTable->Header.Length;

    HalpApicInfoTable.ProcessorCount = 0;
    HalpProcessorIdentity = HalpStaticProcessorIdentity;

    AcpiHeader = (ACPI_SUBTABLE_HEADER*)MadtTable;
    AcpiHeader->Length = sizeof(ACPI_TABLE_MADT);
    TableEnd = (ULONG_PTR)MadtTable + MadtTable->Header.Length;

    while ((ULONG_PTR)AcpiHeader <= TableEnd)
    {
        LocalApic = (ACPI_MADT_LOCAL_APIC*)AcpiHeader;

        if (LocalApic->Header.Type == ACPI_MADT_TYPE_LOCAL_APIC &&
            LocalApic->Header.Length == sizeof(ACPI_MADT_LOCAL_APIC))
        {
            ValidProcessorCount = HalpApicInfoTable.ProcessorCount;

            HalpProcessorIdentity[ValidProcessorCount].LapicId = LocalApic->Id;
            HalpProcessorIdentity[ValidProcessorCount].ProcessorId = LocalApic->ProcessorId;

            HalpApicInfoTable.ProcessorCount++;

            AcpiHeader = (ACPI_SUBTABLE_HEADER*)((ULONG_PTR)AcpiHeader + AcpiHeader->Length);
        }
        else
        {
            /* End the parsing early if we don't use the currently selected table */
            AcpiHeader = (ACPI_SUBTABLE_HEADER*)((ULONG_PTR)AcpiHeader + 1);
        }
    }
}

VOID
HalpPrintApicTables(VOID)
{ 
    UINT32 i;

    DPRINT1("HAL has detected a physical processor count of: %d\n", HalpApicInfoTable.ProcessorCount);
    for (i = 0; i < HalpApicInfoTable.ProcessorCount; i++)
    {
        DPRINT1("Information about the following processor is for processors number: %d\n"
                "   The BSPCheck is set to: %X\n"
                "   The LapicID is set to: %X\n",
                i, HalpProcessorIdentity[i].BSPCheck, HalpProcessorIdentity[i].LapicId);
    }
}
