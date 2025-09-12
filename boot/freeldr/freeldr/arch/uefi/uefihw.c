/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware detection routines
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES ******************************************************************/

#include <uefildr.h>

#include <debug.h>
// AGENT-MODIFIED: Include ACPI header for table structures
#include <drivers/acpi/acpi.h>

DBG_DEFAULT_CHANNEL(WARNING);

// AGENT-MODIFIED: Added BGRT table signature for Boot Graphics Resource Table support
#define BGRT_SIGNATURE 0x54524742  // "BGRT"

/* GLOBALS *******************************************************************/

extern EFI_SYSTEM_TABLE * GlobalSystemTable;
extern EFI_HANDLE GlobalImageHandle;
extern UCHAR PcBiosDiskCount;
extern EFI_MEMORY_DESCRIPTOR* EfiMemoryMap;
extern UINT32 FreeldrDescCount;

BOOLEAN AcpiPresent = FALSE;

// AGENT-MODIFIED: Global pointer to store BGRT table information
static PBGRT_TABLE BgrtTable = NULL;

/* FUNCTIONS *****************************************************************/

BOOLEAN IsAcpiPresent(VOID)
{
    return AcpiPresent;
}

static
PRSDP_DESCRIPTOR
FindAcpiBios(VOID)
{
    UINTN i;
    RSDP_DESCRIPTOR* rsdp = NULL;
    EFI_GUID acpi2_guid = EFI_ACPI_20_TABLE_GUID;

    for (i = 0; i < GlobalSystemTable->NumberOfTableEntries; i++)
    {
        if (!memcmp(&GlobalSystemTable->ConfigurationTable[i].VendorGuid,
                    &acpi2_guid, sizeof(acpi2_guid)))
        {
            rsdp = (RSDP_DESCRIPTOR*)GlobalSystemTable->ConfigurationTable[i].VendorTable;
            break;
        }
    }

    return rsdp;
}

// AGENT-MODIFIED: Function to find and parse BGRT table from ACPI tables
static
PBGRT_TABLE
FindBgrtTable(
    _In_ PRSDP_DESCRIPTOR Rsdp)
{
    PDESCRIPTION_HEADER Header;
    PBGRT_TABLE Bgrt = NULL;
    ULONG *Tables;
    ULONGLONG *Tables64;
    ULONG TableCount;
    ULONG i;
    
    if (!Rsdp)
        return NULL;
    
    TRACE("[AGENT] Looking for BGRT table in ACPI tables\n");
    
    // Use XSDT for ACPI 2.0+, RSDT for ACPI 1.0
    if (Rsdp->revision > 0 && Rsdp->xsdt_physical_address)
    {
        // ACPI 2.0+ - Use XSDT
        PXSDT Xsdt = (PXSDT)(ULONG_PTR)Rsdp->xsdt_physical_address;
        if (!Xsdt)
        {
            TRACE("[AGENT] XSDT is NULL\n");
            return NULL;
        }
        
        // Calculate number of tables
        TableCount = (Xsdt->Header.Length - sizeof(DESCRIPTION_HEADER)) / sizeof(ULONGLONG);
        Tables64 = (ULONGLONG *)((ULONG_PTR)Xsdt + sizeof(DESCRIPTION_HEADER));
        
        TRACE("[AGENT] XSDT has %lu tables\n", TableCount);
        
        // Search for BGRT table
        for (i = 0; i < TableCount; i++)
        {
            Header = (PDESCRIPTION_HEADER)(ULONG_PTR)Tables64[i];
            if (Header && Header->Signature == BGRT_SIGNATURE)
            {
                Bgrt = (PBGRT_TABLE)Header;
                TRACE("[AGENT] Found BGRT table at %p (from XSDT)\n", Bgrt);
                break;
            }
        }
    }
    else if (Rsdp->rsdt_physical_address)
    {
        // ACPI 1.0 - Use RSDT
        PRSDT Rsdt = (PRSDT)(ULONG_PTR)Rsdp->rsdt_physical_address;
        if (!Rsdt)
        {
            TRACE("[AGENT] RSDT is NULL\n");
            return NULL;
        }
        
        // Calculate number of tables
        TableCount = (Rsdt->Header.Length - sizeof(DESCRIPTION_HEADER)) / sizeof(ULONG);
        Tables = (ULONG *)((ULONG_PTR)Rsdt + sizeof(DESCRIPTION_HEADER));
        
        TRACE("[AGENT] RSDT has %lu tables\n", TableCount);
        
        // Search for BGRT table
        for (i = 0; i < TableCount; i++)
        {
            Header = (PDESCRIPTION_HEADER)(ULONG_PTR)Tables[i];
            if (Header && Header->Signature == BGRT_SIGNATURE)
            {
                Bgrt = (PBGRT_TABLE)Header;
                TRACE("[AGENT] Found BGRT table at %p (from RSDT)\n", Bgrt);
                break;
            }
        }
    }
    
    // Validate and log BGRT information if found
    if (Bgrt)
    {
        TRACE("[AGENT] BGRT Version: %u\n", Bgrt->Version);
        TRACE("[AGENT] BGRT Status: 0x%02X\n", Bgrt->Status);
        TRACE("[AGENT] BGRT Image Type: %u\n", Bgrt->ImageType);
        TRACE("[AGENT] BGRT Logo Address: 0x%llX\n", Bgrt->LogoAddress);
        TRACE("[AGENT] BGRT Logo Position: (%lu, %lu)\n", Bgrt->OffsetX, Bgrt->OffsetY);
        
        // Validate that the image is valid
        if (!(Bgrt->Status & 0x01))
        {
            TRACE("[AGENT] BGRT image is not valid (Status bit 0 not set)\n");
            return NULL;
        }
    }
    else
    {
        TRACE("[AGENT] BGRT table not found\n");
    }
    
    return Bgrt;
}

VOID
DetectAcpiBios(PCONFIGURATION_COMPONENT_DATA SystemKey, ULONG *BusNumber)
{
    PCONFIGURATION_COMPONENT_DATA BiosKey;
    PCM_PARTIAL_RESOURCE_LIST PartialResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR PartialDescriptor;
    PRSDP_DESCRIPTOR Rsdp;
    PACPI_BIOS_DATA AcpiBiosData;
    ULONG TableSize;

    Rsdp = FindAcpiBios();

    if (Rsdp)
    {
        /* Set up the flag in the loader block */
        AcpiPresent = TRUE;

        /* Calculate the table size */
        TableSize = FreeldrDescCount * sizeof(BIOS_MEMORY_MAP) +
            sizeof(ACPI_BIOS_DATA) - sizeof(BIOS_MEMORY_MAP);

        /* Set 'Configuration Data' value */
        PartialResourceList = FrLdrHeapAlloc(sizeof(CM_PARTIAL_RESOURCE_LIST) +
                                             TableSize, TAG_HW_RESOURCE_LIST);
        if (PartialResourceList == NULL)
        {
            ERR("Failed to allocate resource descriptor\n");
            return;
        }

        RtlZeroMemory(PartialResourceList, sizeof(CM_PARTIAL_RESOURCE_LIST) + TableSize);
        PartialResourceList->Version = 0;
        PartialResourceList->Revision = 0;
        PartialResourceList->Count = 1;

        PartialDescriptor = &PartialResourceList->PartialDescriptors[0];
        PartialDescriptor->Type = CmResourceTypeDeviceSpecific;
        PartialDescriptor->ShareDisposition = CmResourceShareUndetermined;
        PartialDescriptor->u.DeviceSpecificData.DataSize = TableSize;

        /* Fill the table */
        AcpiBiosData = (PACPI_BIOS_DATA)&PartialResourceList->PartialDescriptors[1];

        if (Rsdp->revision > 0)
        {
            TRACE("ACPI >1.0, using XSDT address\n");
            AcpiBiosData->RSDTAddress.QuadPart = Rsdp->xsdt_physical_address;
        }
        else
        {
            TRACE("ACPI 1.0, using RSDT address\n");
            AcpiBiosData->RSDTAddress.LowPart = Rsdp->rsdt_physical_address;
        }

        AcpiBiosData->Count = FreeldrDescCount;
        memcpy(AcpiBiosData->MemoryMap, EfiMemoryMap,
            FreeldrDescCount * sizeof(BIOS_MEMORY_MAP));

        TRACE("RSDT %p, data size %x\n", Rsdp->rsdt_physical_address, TableSize);

        /* Create new bus key */
        FldrCreateComponentKey(SystemKey,
                               AdapterClass,
                               MultiFunctionAdapter,
                               0x0,
                               0x0,
                               0xFFFFFFFF,
                               "ACPI BIOS",
                               PartialResourceList,
                               sizeof(CM_PARTIAL_RESOURCE_LIST) + TableSize,
                               &BiosKey);

        /* Increment bus number */
        (*BusNumber)++;
    }
}

// AGENT-MODIFIED: Function to get the stored BGRT table
PBGRT_TABLE
GetBgrtTable(VOID)
{
    return BgrtTable;
}

PCONFIGURATION_COMPONENT_DATA
UefiHwDetect(
    _In_opt_ PCSTR Options)
{
    PCONFIGURATION_COMPONENT_DATA SystemKey;
    ULONG BusNumber = 0;

    TRACE("DetectHardware()\n");

    /* Create the 'System' key */
#if defined(_M_IX86) || defined(_M_AMD64)
    FldrCreateSystemKey(&SystemKey, "AT/AT COMPATIBLE");
#elif defined(_M_IA64)
    FldrCreateSystemKey(&SystemKey, "Intel Itanium processor family");
#elif defined(_M_ARM) || defined(_M_ARM64)
    FldrCreateSystemKey(&SystemKey, "ARM processor family");
#else
    #error Please define a system key for your architecture
#endif

    /* Detect ACPI */
    DetectAcpiBios(SystemKey, &BusNumber);
    
    // AGENT-MODIFIED: Find and store BGRT table for boot logo support
    if (AcpiPresent)
    {
        PRSDP_DESCRIPTOR Rsdp = FindAcpiBios();
        if (Rsdp)
        {
            BgrtTable = FindBgrtTable(Rsdp);
            if (BgrtTable)
            {
                TRACE("[AGENT] BGRT table found and stored for later use\n");
            }
        }
    }

    TRACE("DetectHardware() Done\n");
    return SystemKey;
}
