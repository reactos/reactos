/******************************************************************************
 *
 * Module Name: tbinstal - ACPI table installation and removal
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2016, Intel Corp.
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
        ACPI_MODULE_NAME    ("tbinstal")

/* Local prototypes */

static BOOLEAN
AcpiTbCompareTables (
    ACPI_TABLE_DESC         *TableDesc,
    UINT32                  TableIndex);


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbCompareTables
 *
 * PARAMETERS:  TableDesc           - Table 1 descriptor to be compared
 *              TableIndex          - Index of table 2 to be compared
 *
 * RETURN:      TRUE if both tables are identical.
 *
 * DESCRIPTION: This function compares a table with another table that has
 *              already been installed in the root table list.
 *
 ******************************************************************************/

static BOOLEAN
AcpiTbCompareTables (
    ACPI_TABLE_DESC         *TableDesc,
    UINT32                  TableIndex)
{
    ACPI_STATUS             Status = AE_OK;
    BOOLEAN                 IsIdentical;
    ACPI_TABLE_HEADER       *Table;
    UINT32                  TableLength;
    UINT8                   TableFlags;


    Status = AcpiTbAcquireTable (&AcpiGbl_RootTableList.Tables[TableIndex],
        &Table, &TableLength, &TableFlags);
    if (ACPI_FAILURE (Status))
    {
        return (FALSE);
    }

    /*
     * Check for a table match on the entire table length,
     * not just the header.
     */
    IsIdentical = (BOOLEAN)((TableDesc->Length != TableLength ||
        memcmp (TableDesc->Pointer, Table, TableLength)) ?
        FALSE : TRUE);

    /* Release the acquired table */

    AcpiTbReleaseTable (Table, TableLength, TableFlags);
    return (IsIdentical);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbInstallTableWithOverride
 *
 * PARAMETERS:  NewTableDesc            - New table descriptor to install
 *              Override                - Whether override should be performed
 *              TableIndex              - Where the table index is returned
 *
 * RETURN:      None
 *
 * DESCRIPTION: Install an ACPI table into the global data structure. The
 *              table override mechanism is called to allow the host
 *              OS to replace any table before it is installed in the root
 *              table array.
 *
 ******************************************************************************/

void
AcpiTbInstallTableWithOverride (
    ACPI_TABLE_DESC         *NewTableDesc,
    BOOLEAN                 Override,
    UINT32                  *TableIndex)
{
    UINT32                  i;
    ACPI_STATUS             Status;


    Status = AcpiTbGetNextTableDescriptor (&i, NULL);
    if (ACPI_FAILURE (Status))
    {
        return;
    }

    /*
     * ACPI Table Override:
     *
     * Before we install the table, let the host OS override it with a new
     * one if desired. Any table within the RSDT/XSDT can be replaced,
     * including the DSDT which is pointed to by the FADT.
     */
    if (Override)
    {
        AcpiTbOverrideTable (NewTableDesc);
    }

    AcpiTbInitTableDescriptor (&AcpiGbl_RootTableList.Tables[i],
        NewTableDesc->Address, NewTableDesc->Flags, NewTableDesc->Pointer);

    AcpiTbPrintTableHeader (NewTableDesc->Address, NewTableDesc->Pointer);

    /* This synchronizes AcpiGbl_DsdtIndex */

    *TableIndex = i;

    /* Set the global integer width (based upon revision of the DSDT) */

    if (i == AcpiGbl_DsdtIndex)
    {
        AcpiUtSetIntegerWidth (NewTableDesc->Pointer->Revision);
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbInstallFixedTable
 *
 * PARAMETERS:  Address                 - Physical address of DSDT or FACS
 *              Signature               - Table signature, NULL if no need to
 *                                        match
 *              TableIndex              - Where the table index is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Install a fixed ACPI table (DSDT/FACS) into the global data
 *              structure.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiTbInstallFixedTable (
    ACPI_PHYSICAL_ADDRESS   Address,
    char                    *Signature,
    UINT32                  *TableIndex)
{
    ACPI_TABLE_DESC         NewTableDesc;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (TbInstallFixedTable);


    if (!Address)
    {
        ACPI_ERROR ((AE_INFO, "Null physical address for ACPI table [%s]",
            Signature));
        return (AE_NO_MEMORY);
    }

    /* Fill a table descriptor for validation */

    Status = AcpiTbAcquireTempTable (&NewTableDesc, Address,
        ACPI_TABLE_ORIGIN_INTERNAL_PHYSICAL);
    if (ACPI_FAILURE (Status))
    {
        ACPI_ERROR ((AE_INFO, "Could not acquire table length at %8.8X%8.8X",
            ACPI_FORMAT_UINT64 (Address)));
        return_ACPI_STATUS (Status);
    }

    /* Validate and verify a table before installation */

    Status = AcpiTbVerifyTempTable (&NewTableDesc, Signature);
    if (ACPI_FAILURE (Status))
    {
        goto ReleaseAndExit;
    }

    /* Add the table to the global root table list */

    AcpiTbInstallTableWithOverride (&NewTableDesc, TRUE, TableIndex);

ReleaseAndExit:

    /* Release the temporary table descriptor */

    AcpiTbReleaseTempTable (&NewTableDesc);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbInstallStandardTable
 *
 * PARAMETERS:  Address             - Address of the table (might be a virtual
 *                                    address depending on the TableFlags)
 *              Flags               - Flags for the table
 *              Reload              - Whether reload should be performed
 *              Override            - Whether override should be performed
 *              TableIndex          - Where the table index is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This function is called to install an ACPI table that is
 *              neither DSDT nor FACS (a "standard" table.)
 *              When this function is called by "Load" or "LoadTable" opcodes,
 *              or by AcpiLoadTable() API, the "Reload" parameter is set.
 *              After sucessfully returning from this function, table is
 *              "INSTALLED" but not "VALIDATED".
 *
 ******************************************************************************/

ACPI_STATUS
AcpiTbInstallStandardTable (
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT8                   Flags,
    BOOLEAN                 Reload,
    BOOLEAN                 Override,
    UINT32                  *TableIndex)
{
    UINT32                  i;
    ACPI_STATUS             Status = AE_OK;
    ACPI_TABLE_DESC         NewTableDesc;


    ACPI_FUNCTION_TRACE (TbInstallStandardTable);


    /* Acquire a temporary table descriptor for validation */

    Status = AcpiTbAcquireTempTable (&NewTableDesc, Address, Flags);
    if (ACPI_FAILURE (Status))
    {
        ACPI_ERROR ((AE_INFO,
            "Could not acquire table length at %8.8X%8.8X",
            ACPI_FORMAT_UINT64 (Address)));
        return_ACPI_STATUS (Status);
    }

    /*
     * Optionally do not load any SSDTs from the RSDT/XSDT. This can
     * be useful for debugging ACPI problems on some machines.
     */
    if (!Reload &&
        AcpiGbl_DisableSsdtTableInstall &&
        ACPI_COMPARE_NAME (&NewTableDesc.Signature, ACPI_SIG_SSDT))
    {
        ACPI_INFO ((
            "Ignoring installation of %4.4s at %8.8X%8.8X",
            NewTableDesc.Signature.Ascii, ACPI_FORMAT_UINT64 (Address)));
        goto ReleaseAndExit;
    }

    /* Validate and verify a table before installation */

    Status = AcpiTbVerifyTempTable (&NewTableDesc, NULL);
    if (ACPI_FAILURE (Status))
    {
        goto ReleaseAndExit;
    }

    if (Reload)
    {
        /*
         * Validate the incoming table signature.
         *
         * 1) Originally, we checked the table signature for "SSDT" or "PSDT".
         * 2) We added support for OEMx tables, signature "OEM".
         * 3) Valid tables were encountered with a null signature, so we just
         *    gave up on validating the signature, (05/2008).
         * 4) We encountered non-AML tables such as the MADT, which caused
         *    interpreter errors and kernel faults. So now, we once again allow
         *    only "SSDT", "OEMx", and now, also a null signature. (05/2011).
         */
        if ((NewTableDesc.Signature.Ascii[0] != 0x00) &&
           (!ACPI_COMPARE_NAME (&NewTableDesc.Signature, ACPI_SIG_SSDT)) &&
           (strncmp (NewTableDesc.Signature.Ascii, "OEM", 3)))
        {
            ACPI_BIOS_ERROR ((AE_INFO,
                "Table has invalid signature [%4.4s] (0x%8.8X), "
                "must be SSDT or OEMx",
                AcpiUtValidNameseg (NewTableDesc.Signature.Ascii) ?
                    NewTableDesc.Signature.Ascii : "????",
                NewTableDesc.Signature.Integer));

            Status = AE_BAD_SIGNATURE;
            goto ReleaseAndExit;
        }

        /* Check if table is already registered */

        for (i = 0; i < AcpiGbl_RootTableList.CurrentTableCount; ++i)
        {
            /*
             * Check for a table match on the entire table length,
             * not just the header.
             */
            if (!AcpiTbCompareTables (&NewTableDesc, i))
            {
                continue;
            }

            /*
             * Note: the current mechanism does not unregister a table if it is
             * dynamically unloaded. The related namespace entries are deleted,
             * but the table remains in the root table list.
             *
             * The assumption here is that the number of different tables that
             * will be loaded is actually small, and there is minimal overhead
             * in just keeping the table in case it is needed again.
             *
             * If this assumption changes in the future (perhaps on large
             * machines with many table load/unload operations), tables will
             * need to be unregistered when they are unloaded, and slots in the
             * root table list should be reused when empty.
             */
            if (AcpiGbl_RootTableList.Tables[i].Flags &
                ACPI_TABLE_IS_LOADED)
            {
                /* Table is still loaded, this is an error */

                Status = AE_ALREADY_EXISTS;
                goto ReleaseAndExit;
            }
            else
            {
                /*
                 * Table was unloaded, allow it to be reloaded.
                 * As we are going to return AE_OK to the caller, we should
                 * take the responsibility of freeing the input descriptor.
                 * Refill the input descriptor to ensure
                 * AcpiTbInstallTableWithOverride() can be called again to
                 * indicate the re-installation.
                 */
                AcpiTbUninstallTable (&NewTableDesc);
                *TableIndex = i;
                return_ACPI_STATUS (AE_OK);
            }
        }
    }

    /* Add the table to the global root table list */

    AcpiTbInstallTableWithOverride (&NewTableDesc, Override, TableIndex);

ReleaseAndExit:

    /* Release the temporary table descriptor */

    AcpiTbReleaseTempTable (&NewTableDesc);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbOverrideTable
 *
 * PARAMETERS:  OldTableDesc        - Validated table descriptor to be
 *                                    overridden
 *
 * RETURN:      None
 *
 * DESCRIPTION: Attempt table override by calling the OSL override functions.
 *              Note: If the table is overridden, then the entire new table
 *              is acquired and returned by this function.
 *              Before/after invocation, the table descriptor is in a state
 *              that is "VALIDATED".
 *
 ******************************************************************************/

void
AcpiTbOverrideTable (
    ACPI_TABLE_DESC         *OldTableDesc)
{
    ACPI_STATUS             Status;
    char                    *OverrideType;
    ACPI_TABLE_DESC         NewTableDesc;
    ACPI_TABLE_HEADER       *Table;
    ACPI_PHYSICAL_ADDRESS   Address;
    UINT32                  Length;


    /* (1) Attempt logical override (returns a logical address) */

    Status = AcpiOsTableOverride (OldTableDesc->Pointer, &Table);
    if (ACPI_SUCCESS (Status) && Table)
    {
        AcpiTbAcquireTempTable (&NewTableDesc, ACPI_PTR_TO_PHYSADDR (Table),
            ACPI_TABLE_ORIGIN_EXTERNAL_VIRTUAL);
        OverrideType = "Logical";
        goto FinishOverride;
    }

    /* (2) Attempt physical override (returns a physical address) */

    Status = AcpiOsPhysicalTableOverride (OldTableDesc->Pointer,
        &Address, &Length);
    if (ACPI_SUCCESS (Status) && Address && Length)
    {
        AcpiTbAcquireTempTable (&NewTableDesc, Address,
            ACPI_TABLE_ORIGIN_INTERNAL_PHYSICAL);
        OverrideType = "Physical";
        goto FinishOverride;
    }

    return; /* There was no override */


FinishOverride:

    /* Validate and verify a table before overriding */

    Status = AcpiTbVerifyTempTable (&NewTableDesc, NULL);
    if (ACPI_FAILURE (Status))
    {
        return;
    }

    ACPI_INFO (("%4.4s 0x%8.8X%8.8X"
        " %s table override, new table: 0x%8.8X%8.8X",
        OldTableDesc->Signature.Ascii,
        ACPI_FORMAT_UINT64 (OldTableDesc->Address),
        OverrideType, ACPI_FORMAT_UINT64 (NewTableDesc.Address)));

    /* We can now uninstall the original table */

    AcpiTbUninstallTable (OldTableDesc);

    /*
     * Replace the original table descriptor and keep its state as
     * "VALIDATED".
     */
    AcpiTbInitTableDescriptor (OldTableDesc, NewTableDesc.Address,
        NewTableDesc.Flags, NewTableDesc.Pointer);
    AcpiTbValidateTempTable (OldTableDesc);

    /* Release the temporary table descriptor */

    AcpiTbReleaseTempTable (&NewTableDesc);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbUninstallTable
 *
 * PARAMETERS:  TableDesc           - Table descriptor
 *
 * RETURN:      None
 *
 * DESCRIPTION: Delete one internal ACPI table
 *
 ******************************************************************************/

void
AcpiTbUninstallTable (
    ACPI_TABLE_DESC         *TableDesc)
{

    ACPI_FUNCTION_TRACE (TbUninstallTable);


    /* Table must be installed */

    if (!TableDesc->Address)
    {
        return_VOID;
    }

    AcpiTbInvalidateTable (TableDesc);

    if ((TableDesc->Flags & ACPI_TABLE_ORIGIN_MASK) ==
        ACPI_TABLE_ORIGIN_INTERNAL_VIRTUAL)
    {
        ACPI_FREE (ACPI_PHYSADDR_TO_PTR (TableDesc->Address));
    }

    TableDesc->Address = ACPI_PTR_TO_PHYSADDR (NULL);
    return_VOID;
}
