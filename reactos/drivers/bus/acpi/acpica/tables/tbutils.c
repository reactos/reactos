/******************************************************************************
 *
 * Module Name: tbutils - ACPI Table utilities
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2017, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#include "acpi.h"
#include "accommon.h"
#include "actables.h"

#define _COMPONENT          ACPI_TABLES
        ACPI_MODULE_NAME    ("tbutils")


/* Local prototypes */

static ACPI_PHYSICAL_ADDRESS
AcpiTbGetRootTableEntry (
    UINT8                   *TableEntry,
    UINT32                  TableEntrySize);


#if (!ACPI_REDUCED_HARDWARE)
/*******************************************************************************
 *
 * FUNCTION:    AcpiTbInitializeFacs
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Create a permanent mapping for the FADT and save it in a global
 *              for accessing the Global Lock and Firmware Waking Vector
 *
 ******************************************************************************/

ACPI_STATUS
AcpiTbInitializeFacs (
    void)
{
    ACPI_TABLE_FACS         *Facs;


    /* If Hardware Reduced flag is set, there is no FACS */

    if (AcpiGbl_ReducedHardware)
    {
        AcpiGbl_FACS = NULL;
        return (AE_OK);
    }
    else if (AcpiGbl_FADT.XFacs &&
         (!AcpiGbl_FADT.Facs || !AcpiGbl_Use32BitFacsAddresses))
    {
        (void) AcpiGetTableByIndex (AcpiGbl_XFacsIndex,
            ACPI_CAST_INDIRECT_PTR (ACPI_TABLE_HEADER, &Facs));
        AcpiGbl_FACS = Facs;
    }
    else if (AcpiGbl_FADT.Facs)
    {
        (void) AcpiGetTableByIndex (AcpiGbl_FacsIndex,
            ACPI_CAST_INDIRECT_PTR (ACPI_TABLE_HEADER, &Facs));
        AcpiGbl_FACS = Facs;
    }

    /* If there is no FACS, just continue. There was already an error msg */

    return (AE_OK);
}
#endif /* !ACPI_REDUCED_HARDWARE */


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbCheckDsdtHeader
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Quick compare to check validity of the DSDT. This will detect
 *              if the DSDT has been replaced from outside the OS and/or if
 *              the DSDT header has been corrupted.
 *
 ******************************************************************************/

void
AcpiTbCheckDsdtHeader (
    void)
{

    /* Compare original length and checksum to current values */

    if (AcpiGbl_OriginalDsdtHeader.Length != AcpiGbl_DSDT->Length ||
        AcpiGbl_OriginalDsdtHeader.Checksum != AcpiGbl_DSDT->Checksum)
    {
        ACPI_BIOS_ERROR ((AE_INFO,
            "The DSDT has been corrupted or replaced - "
            "old, new headers below"));

        AcpiTbPrintTableHeader (0, &AcpiGbl_OriginalDsdtHeader);
        AcpiTbPrintTableHeader (0, AcpiGbl_DSDT);

        /* Disable further error messages */

        AcpiGbl_OriginalDsdtHeader.Length = AcpiGbl_DSDT->Length;
        AcpiGbl_OriginalDsdtHeader.Checksum = AcpiGbl_DSDT->Checksum;
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbCopyDsdt
 *
 * PARAMETERS:  TableIndex          - Index of installed table to copy
 *
 * RETURN:      The copied DSDT
 *
 * DESCRIPTION: Implements a subsystem option to copy the DSDT to local memory.
 *              Some very bad BIOSs are known to either corrupt the DSDT or
 *              install a new, bad DSDT. This copy works around the problem.
 *
 ******************************************************************************/

ACPI_TABLE_HEADER *
AcpiTbCopyDsdt (
    UINT32                  TableIndex)
{
    ACPI_TABLE_HEADER       *NewTable;
    ACPI_TABLE_DESC         *TableDesc;


    TableDesc = &AcpiGbl_RootTableList.Tables[TableIndex];

    NewTable = ACPI_ALLOCATE (TableDesc->Length);
    if (!NewTable)
    {
        ACPI_ERROR ((AE_INFO, "Could not copy DSDT of length 0x%X",
            TableDesc->Length));
        return (NULL);
    }

    memcpy (NewTable, TableDesc->Pointer, TableDesc->Length);
    AcpiTbUninstallTable (TableDesc);

    AcpiTbInitTableDescriptor (
        &AcpiGbl_RootTableList.Tables[AcpiGbl_DsdtIndex],
        ACPI_PTR_TO_PHYSADDR (NewTable),
        ACPI_TABLE_ORIGIN_INTERNAL_VIRTUAL, NewTable);

    ACPI_INFO ((
        "Forced DSDT copy: length 0x%05X copied locally, original unmapped",
        NewTable->Length));

    return (NewTable);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbGetRootTableEntry
 *
 * PARAMETERS:  TableEntry          - Pointer to the RSDT/XSDT table entry
 *              TableEntrySize      - sizeof 32 or 64 (RSDT or XSDT)
 *
 * RETURN:      Physical address extracted from the root table
 *
 * DESCRIPTION: Get one root table entry. Handles 32-bit and 64-bit cases on
 *              both 32-bit and 64-bit platforms
 *
 * NOTE:        ACPI_PHYSICAL_ADDRESS is 32-bit on 32-bit platforms, 64-bit on
 *              64-bit platforms.
 *
 ******************************************************************************/

static ACPI_PHYSICAL_ADDRESS
AcpiTbGetRootTableEntry (
    UINT8                   *TableEntry,
    UINT32                  TableEntrySize)
{
    UINT64                  Address64;


    /*
     * Get the table physical address (32-bit for RSDT, 64-bit for XSDT):
     * Note: Addresses are 32-bit aligned (not 64) in both RSDT and XSDT
     */
    if (TableEntrySize == ACPI_RSDT_ENTRY_SIZE)
    {
        /*
         * 32-bit platform, RSDT: Return 32-bit table entry
         * 64-bit platform, RSDT: Expand 32-bit to 64-bit and return
         */
        return ((ACPI_PHYSICAL_ADDRESS) (*ACPI_CAST_PTR (
            UINT32, TableEntry)));
    }
    else
    {
        /*
         * 32-bit platform, XSDT: Truncate 64-bit to 32-bit and return
         * 64-bit platform, XSDT: Move (unaligned) 64-bit to local,
         *  return 64-bit
         */
        ACPI_MOVE_64_TO_64 (&Address64, TableEntry);

#if ACPI_MACHINE_WIDTH == 32
        if (Address64 > ACPI_UINT32_MAX)
        {
            /* Will truncate 64-bit address to 32 bits, issue warning */

            ACPI_BIOS_WARNING ((AE_INFO,
                "64-bit Physical Address in XSDT is too large (0x%8.8X%8.8X),"
                " truncating",
                ACPI_FORMAT_UINT64 (Address64)));
        }
#endif
        return ((ACPI_PHYSICAL_ADDRESS) (Address64));
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbParseRootTable
 *
 * PARAMETERS:  RsdpAddress         - Pointer to the RSDP
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This function is called to parse the Root System Description
 *              Table (RSDT or XSDT)
 *
 * NOTE:        Tables are mapped (not copied) for efficiency. The FACS must
 *              be mapped and cannot be copied because it contains the actual
 *              memory location of the ACPI Global Lock.
 *
 ******************************************************************************/

ACPI_STATUS ACPI_INIT_FUNCTION
AcpiTbParseRootTable (
    ACPI_PHYSICAL_ADDRESS   RsdpAddress)
{
    ACPI_TABLE_RSDP         *Rsdp;
    UINT32                  TableEntrySize;
    UINT32                  i;
    UINT32                  TableCount;
    ACPI_TABLE_HEADER       *Table;
    ACPI_PHYSICAL_ADDRESS   Address;
    UINT32                  Length;
    UINT8                   *TableEntry;
    ACPI_STATUS             Status;
    UINT32                  TableIndex;


    ACPI_FUNCTION_TRACE (TbParseRootTable);


    /* Map the entire RSDP and extract the address of the RSDT or XSDT */

    Rsdp = AcpiOsMapMemory (RsdpAddress, sizeof (ACPI_TABLE_RSDP));
    if (!Rsdp)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    AcpiTbPrintTableHeader (RsdpAddress,
        ACPI_CAST_PTR (ACPI_TABLE_HEADER, Rsdp));

    /* Use XSDT if present and not overridden. Otherwise, use RSDT */

    if ((Rsdp->Revision > 1) &&
        Rsdp->XsdtPhysicalAddress &&
        !AcpiGbl_DoNotUseXsdt)
    {
        /*
         * RSDP contains an XSDT (64-bit physical addresses). We must use
         * the XSDT if the revision is > 1 and the XSDT pointer is present,
         * as per the ACPI specification.
         */
        Address = (ACPI_PHYSICAL_ADDRESS) Rsdp->XsdtPhysicalAddress;
        TableEntrySize = ACPI_XSDT_ENTRY_SIZE;
    }
    else
    {
        /* Root table is an RSDT (32-bit physical addresses) */

        Address = (ACPI_PHYSICAL_ADDRESS) Rsdp->RsdtPhysicalAddress;
        TableEntrySize = ACPI_RSDT_ENTRY_SIZE;
    }

    /*
     * It is not possible to map more than one entry in some environments,
     * so unmap the RSDP here before mapping other tables
     */
    AcpiOsUnmapMemory (Rsdp, sizeof (ACPI_TABLE_RSDP));

    /* Map the RSDT/XSDT table header to get the full table length */

    Table = AcpiOsMapMemory (Address, sizeof (ACPI_TABLE_HEADER));
    if (!Table)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    AcpiTbPrintTableHeader (Address, Table);

    /*
     * Validate length of the table, and map entire table.
     * Minimum length table must contain at least one entry.
     */
    Length = Table->Length;
    AcpiOsUnmapMemory (Table, sizeof (ACPI_TABLE_HEADER));

    if (Length < (sizeof (ACPI_TABLE_HEADER) + TableEntrySize))
    {
        ACPI_BIOS_ERROR ((AE_INFO,
            "Invalid table length 0x%X in RSDT/XSDT", Length));
        return_ACPI_STATUS (AE_INVALID_TABLE_LENGTH);
    }

    Table = AcpiOsMapMemory (Address, Length);
    if (!Table)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    /* Validate the root table checksum */

    Status = AcpiTbVerifyChecksum (Table, Length);
    if (ACPI_FAILURE (Status))
    {
        AcpiOsUnmapMemory (Table, Length);
        return_ACPI_STATUS (Status);
    }

    /* Get the number of entries and pointer to first entry */

    TableCount = (UINT32) ((Table->Length - sizeof (ACPI_TABLE_HEADER)) /
        TableEntrySize);
    TableEntry = ACPI_ADD_PTR (UINT8, Table, sizeof (ACPI_TABLE_HEADER));

    /* Initialize the root table array from the RSDT/XSDT */

    for (i = 0; i < TableCount; i++)
    {
        /* Get the table physical address (32-bit for RSDT, 64-bit for XSDT) */

        Address = AcpiTbGetRootTableEntry (TableEntry, TableEntrySize);

        /* Skip NULL entries in RSDT/XSDT */

        if (!Address)
        {
            goto NextTable;
        }

        Status = AcpiTbInstallStandardTable (Address,
            ACPI_TABLE_ORIGIN_INTERNAL_PHYSICAL, FALSE, TRUE, &TableIndex);

        if (ACPI_SUCCESS (Status) &&
            ACPI_COMPARE_NAME (
                &AcpiGbl_RootTableList.Tables[TableIndex].Signature,
                ACPI_SIG_FADT))
        {
            AcpiGbl_FadtIndex = TableIndex;
            AcpiTbParseFadt ();
        }

NextTable:

        TableEntry += TableEntrySize;
    }

    AcpiOsUnmapMemory (Table, Length);
    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbGetTable
 *
 * PARAMETERS:  TableDesc           - Table descriptor
 *              OutTable            - Where the pointer to the table is returned
 *
 * RETURN:      Status and pointer to the requested table
 *
 * DESCRIPTION: Increase a reference to a table descriptor and return the
 *              validated table pointer.
 *              If the table descriptor is an entry of the root table list,
 *              this API must be invoked with ACPI_MTX_TABLES acquired.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiTbGetTable (
    ACPI_TABLE_DESC        *TableDesc,
    ACPI_TABLE_HEADER      **OutTable)
{
    ACPI_STATUS            Status;


    ACPI_FUNCTION_TRACE (AcpiTbGetTable);


    if (TableDesc->ValidationCount == 0)
    {
        /* Table need to be "VALIDATED" */

        Status = AcpiTbValidateTable (TableDesc);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }
    }

    if (TableDesc->ValidationCount < ACPI_MAX_TABLE_VALIDATIONS)
    {
        TableDesc->ValidationCount++;

        /*
         * Detect ValidationCount overflows to ensure that the warning
         * message will only be printed once.
         */
        if (TableDesc->ValidationCount >= ACPI_MAX_TABLE_VALIDATIONS)
        {
            ACPI_WARNING((AE_INFO,
                "Table %p, Validation count overflows\n", TableDesc));
        }
    }

    *OutTable = TableDesc->Pointer;
    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbPutTable
 *
 * PARAMETERS:  TableDesc           - Table descriptor
 *
 * RETURN:      None
 *
 * DESCRIPTION: Decrease a reference to a table descriptor and release the
 *              validated table pointer if no references.
 *              If the table descriptor is an entry of the root table list,
 *              this API must be invoked with ACPI_MTX_TABLES acquired.
 *
 ******************************************************************************/

void
AcpiTbPutTable (
    ACPI_TABLE_DESC        *TableDesc)
{

    ACPI_FUNCTION_TRACE (AcpiTbPutTable);


    if (TableDesc->ValidationCount < ACPI_MAX_TABLE_VALIDATIONS)
    {
        TableDesc->ValidationCount--;

        /*
         * Detect ValidationCount underflows to ensure that the warning
         * message will only be printed once.
         */
        if (TableDesc->ValidationCount >= ACPI_MAX_TABLE_VALIDATIONS)
        {
            ACPI_WARNING ((AE_INFO,
                "Table %p, Validation count underflows\n", TableDesc));
            return_VOID;
        }
    }

    if (TableDesc->ValidationCount == 0)
    {
        /* Table need to be "INVALIDATED" */

        AcpiTbInvalidateTable (TableDesc);
    }

    return_VOID;
}
