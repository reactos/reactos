/******************************************************************************
 *
 * Module Name: tbinstal - ACPI table installation and removal
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2015, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights. You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code. No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision. In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change. Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee. Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution. In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE. ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT, ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES. INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS. INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES. THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government. In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/

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
        ACPI_MEMCMP (TableDesc->Pointer, Table, TableLength)) ?
        FALSE : TRUE);

    /* Release the acquired table */

    AcpiTbReleaseTable (Table, TableLength, TableFlags);
    return (IsIdentical);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbInstallTableWithOverride
 *
 * PARAMETERS:  TableIndex              - Index into root table array
 *              NewTableDesc            - New table descriptor to install
 *              Override                - Whether override should be performed
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
    UINT32                  TableIndex,
    ACPI_TABLE_DESC         *NewTableDesc,
    BOOLEAN                 Override)
{

    if (TableIndex >= AcpiGbl_RootTableList.CurrentTableCount)
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

    AcpiTbInitTableDescriptor (&AcpiGbl_RootTableList.Tables[TableIndex],
        NewTableDesc->Address, NewTableDesc->Flags, NewTableDesc->Pointer);

    AcpiTbPrintTableHeader (NewTableDesc->Address, NewTableDesc->Pointer);

    /* Set the global integer width (based upon revision of the DSDT) */

    if (TableIndex == ACPI_TABLE_INDEX_DSDT)
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
 *              TableIndex              - Index into root table array
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
    UINT32                  TableIndex)
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

    AcpiTbInstallTableWithOverride (TableIndex, &NewTableDesc, TRUE);

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
        ACPI_ERROR ((AE_INFO, "Could not acquire table length at %8.8X%8.8X",
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
        ACPI_INFO ((AE_INFO, "Ignoring installation of %4.4s at %8.8X%8.8X",
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
           (ACPI_STRNCMP (NewTableDesc.Signature.Ascii, "OEM", 3)))
        {
            ACPI_BIOS_ERROR ((AE_INFO,
                "Table has invalid signature [%4.4s] (0x%8.8X), "
                "must be SSDT or OEMx",
                AcpiUtValidAcpiName (NewTableDesc.Signature.Ascii) ?
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
            if (AcpiGbl_RootTableList.Tables[i].Flags & ACPI_TABLE_IS_LOADED)
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

    Status = AcpiTbGetNextTableDescriptor (&i, NULL);
    if (ACPI_FAILURE (Status))
    {
        goto ReleaseAndExit;
    }

    *TableIndex = i;
    AcpiTbInstallTableWithOverride (i, &NewTableDesc, Override);

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

    ACPI_INFO ((AE_INFO, "%4.4s 0x%8.8X%8.8X"
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
