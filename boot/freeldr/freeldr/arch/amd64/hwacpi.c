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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <freeldr.h>
#include <debug.h>

BOOLEAN AcpiPresent = FALSE;

static PRSDP_DESCRIPTOR
FindAcpiBios(VOID)
{
    PUCHAR Ptr;

    /* Find the 'Root System Descriptor Table Pointer' */
    Ptr = (PUCHAR)0xE0000;
    while ((ULONG_PTR)Ptr < 0x100000)
    {
        if (!memcmp(Ptr, "RSD PTR ", 8))
        {
            DbgPrint((DPRINT_HWDETECT, "ACPI supported\n"));

            return (PRSDP_DESCRIPTOR)Ptr;
        }

        Ptr = (PUCHAR)((ULONG_PTR)Ptr + 0x10);
    }

    DbgPrint((DPRINT_HWDETECT, "ACPI not supported\n"));

    return NULL;
}


VOID
DetectAcpiBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber)
{
    PCONFIGURATION_COMPONENT_DATA BiosKey;
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PRSDP_DESCRIPTOR Rsdp;
    PACPI_BIOS_DATA AcpiBiosData;
    BIOS_MEMORY_MAP BiosMemoryMap[32];
    ULONG BiosMemoryMapEntryCount, TableSize;

    Rsdp = FindAcpiBios();

    if (Rsdp)
    {
        /* Set up the flag in the loader block */
        AcpiPresent = TRUE;
        LoaderBlock.Flags |= MB_FLAGS_ACPI_TABLE;

        /* Create new bus key */
        FldrCreateComponentKey(SystemKey,
                               L"MultifunctionAdapter",
                               *BusNumber,
                               AdapterClass,
                               MultiFunctionAdapter,
                               &BiosKey);

        /* Set 'Component Information' */
        FldrSetComponentInformation(BiosKey,
                                    0x0,
                                    0x0,
                                    0xFFFFFFFF);

        /* Get BIOS memory map */
        RtlZeroMemory(BiosMemoryMap, sizeof(BIOS_MEMORY_MAP) * 32);
        BiosMemoryMapEntryCount = MachGetMemoryMap(BiosMemoryMap,
            sizeof(BiosMemoryMap) / sizeof(BIOS_MEMORY_MAP));

        /* Calculate the table size */
        TableSize = BiosMemoryMapEntryCount * sizeof(BIOS_MEMORY_MAP) +
            sizeof(ACPI_BIOS_DATA) - sizeof(BIOS_MEMORY_MAP);

        /* Set 'Configuration Data' value */
        PartialResourceList =
            MmHeapAlloc(sizeof(CM_PARTIAL_RESOURCE_LIST) + TableSize);
        memset(PartialResourceList, 0, sizeof(CM_PARTIAL_RESOURCE_LIST) + TableSize);
        PartialResourceList->Version = 0;
        PartialResourceList->Revision = 0;
        PartialResourceList->Count = 1;

        PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
        PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
        PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
        PartialDescriptor->u.DeviceSpecificData.DataSize = TableSize;

        /* Fill the table */
        AcpiBiosData = (PACPI_BIOS_DATA)&PartialResourceList->PartialDescriptors[1];
        AcpiBiosData->RSDTAddress.LowPart = Rsdp->rsdt_physical_address;
        AcpiBiosData->Count = BiosMemoryMapEntryCount;
        memcpy(AcpiBiosData->MemoryMap, BiosMemoryMap,
            BiosMemoryMapEntryCount * sizeof(BIOS_MEMORY_MAP));

        DbgPrint((DPRINT_HWDETECT, "RSDT %p, data size %x\n", Rsdp->rsdt_physical_address,
            TableSize));

        FldrSetConfigurationData(BiosKey,
                                 PartialResourceList,
                                 sizeof(CM_PARTIAL_RESOURCE_LIST) + TableSize
                                 );

        /* Increment bus number */
        (*BusNumber)++;

        /* Set 'Identifier' value */
        FldrSetIdentifier(BiosKey, "ACPI BIOS");
        MmFreeMemory(PartialResourceList);
    }
}

/* EOF */
