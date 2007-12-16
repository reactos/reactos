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

static BOOLEAN
FindAcpiBios(VOID)
{
    PUCHAR Ptr;

    /* Find the 'Root System Descriptor Table Pointer' */
    Ptr = (PUCHAR)0xE0000;
    while ((ULONG)Ptr < 0x100000)
    {
        if (!memcmp(Ptr, "RSD PTR ", 8))
        {
            DbgPrint((DPRINT_HWDETECT, "ACPI supported\n"));

            return TRUE;
        }

        Ptr = (PUCHAR)((ULONG)Ptr + 0x10);
    }

    DbgPrint((DPRINT_HWDETECT, "ACPI not supported\n"));

    return FALSE;
}


VOID
DetectAcpiBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber)
{
    PCONFIGURATION_COMPONENT_DATA BiosKey;
    CM_PARTIAL_RESOURCE_LIST PartialResourceList;

    if (FindAcpiBios())
    {
        AcpiPresent = TRUE;

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
        
        /* Set 'Configuration Data' value */
        memset(&PartialResourceList, 0, sizeof(CM_PARTIAL_RESOURCE_LIST));
        PartialResourceList.Version = 0;
        PartialResourceList.Revision = 0;
        PartialResourceList.Count = 0;
        FldrSetConfigurationData(BiosKey,
                                 &PartialResourceList,
                                 sizeof(CM_PARTIAL_RESOURCE_LIST) -
                                 sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

        /* Increment bus number */
        (*BusNumber)++;
        
        /* Set 'Identifier' value */
        FldrSetIdentifier(BiosKey, L"ACPI BIOS");
    }
    
    /* FIXME: Add congiguration data */
}

/* EOF */
