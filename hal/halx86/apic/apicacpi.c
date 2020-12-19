/*
 * PROJECT:         ReactOS HAL
 * COPYRIGHT:       GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * FILE:            hal/halx86/apic/apicacpi.c
 * PURPOSE:         ACPI part APIC HALs code
 * PROGRAMMERS:     Copyright 2020 Vadim Galyant <vgal@rambler.ru>
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

#include "apic.h"
#include "apicacpi.h"

/* GLOBALS ********************************************************************/

HALP_MP_INFO_TABLE HalpMpInfoTable;

extern UCHAR HalpIRQLtoTPR[32];    // table, which sets the correspondence between IRQL levels and TPR (Task Priority Register) values.
extern KIRQL HalpVectorToIRQL[16];

/* FUNCTIONS ******************************************************************/

VOID
NTAPI 
HalpInitMpInfo(_In_ PACPI_TABLE_MADT ApicTable,
               _In_ ULONG Phase,
               _In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    // FIXME UNIMPLIMENTED;
    ASSERT(FALSE);
}

BOOLEAN
NTAPI 
HalpVerifyIOUnit(
    _In_ PIO_APIC_REGISTERS IOUnitRegs)
{
    // FIXME UNIMPLIMENTED;
    ASSERT(FALSE);
    return FALSE;
}

BOOLEAN
NTAPI 
DetectMP(_In_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PACPI_TABLE_MADT HalpApicTable;
    PHYSICAL_ADDRESS PhAddress;
    ULONG_PTR LocalApicBaseVa;
    ULONG ix;

    LoaderBlock->Extension->HalpIRQLToTPR = HalpIRQLtoTPR;
    LoaderBlock->Extension->HalpVectorToIRQL = HalpVectorToIRQL;

    RtlZeroMemory(&HalpMpInfoTable, sizeof(HalpMpInfoTable));

    HalpApicTable = HalAcpiGetTable(LoaderBlock, 'CIPA');
    if (!HalpApicTable)
    {
        return FALSE;
    }

    HalpInitMpInfo(HalpApicTable, 0, LoaderBlock);
    if (!HalpMpInfoTable.IoApicCount)
    {
        return FALSE;
    }

    HalpMpInfoTable.LocalApicPA = HalpApicTable->Address;
    PhAddress.QuadPart = HalpMpInfoTable.LocalApicPA;

    LocalApicBaseVa = (ULONG_PTR)HalpMapPhysicalMemoryWriteThrough64(PhAddress, 1);

    HalpRemapVirtualAddress64((PVOID)LOCAL_APIC_BASE, PhAddress, TRUE);

    if (*(volatile PUCHAR)(LocalApicBaseVa + APIC_VER) > LOCAL_APIC_VERSION_MAX)
    {
        return FALSE;
    }

    for (ix = 0; ix < HalpMpInfoTable.IoApicCount; ix++)
    {
        if (!HalpVerifyIOUnit((PIO_APIC_REGISTERS)HalpMpInfoTable.IoApicVA[ix]))
        {
            return FALSE;
        }
    }

    return TRUE;
}

/* EOF */
