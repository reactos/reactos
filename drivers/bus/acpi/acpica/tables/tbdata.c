/******************************************************************************
 *
 * Module Name: tbdata - Table manager data structure functions
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2022, Intel Corp.
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
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
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
#include "acnamesp.h"
#include "actables.h"
#include "acevents.h"

#define _COMPONENT          ACPI_TABLES
        ACPI_MODULE_NAME    ("tbdata")

/* Local prototypes */

static ACPI_STATUS
AcpiTbCheckDuplication (
    ACPI_TABLE_DESC         *TableDesc,
    UINT32                  *TableIndex);

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
 * FUNCTION:    AcpiTbInitTableDescriptor
 *
 * PARAMETERS:  TableDesc               - Table descriptor
 *              Address                 - Physical address of the table
 *              Flags                   - Allocation flags of the table
 *              Table                   - Pointer to the table
 *
 * RETURN:      None
 *
 * DESCRIPTION: Initialize a new table descriptor
 *
 ******************************************************************************/

void
AcpiTbInitTableDescriptor (
    ACPI_TABLE_DESC         *TableDesc,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT8                   Flags,
    ACPI_TABLE_HEADER       *Table)
{

    /*
     * Initialize the table descriptor. Set the pointer to NULL for external
     * tables, since the table is not fully mapped at this time.
     */
    memset (TableDesc, 0, sizeof (ACPI_TABLE_DESC));
    TableDesc->Address = Address;
    TableDesc->Length = Table->Length;
    TableDesc->Flags = Flags;
    ACPI_MOVE_32_TO_32 (TableDesc->Signature.Ascii, Table->Signature);

    switch (TableDesc->Flags & ACPI_TABLE_ORIGIN_MASK)
    {
    case ACPI_TABLE_ORIGIN_INTERNAL_VIRTUAL:
    case ACPI_TABLE_ORIGIN_EXTERNAL_VIRTUAL:

        TableDesc->Pointer = Table;
        break;

    case ACPI_TABLE_ORIGIN_INTERNAL_PHYSICAL:
    default:

        break;
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbAcquireTable
 *
 * PARAMETERS:  TableDesc           - Table descriptor
 *              TablePtr            - Where table is returned
 *              TableLength         - Where table length is returned
 *              TableFlags          - Where table allocation flags are returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Acquire an ACPI table. It can be used for tables not
 *              maintained in the AcpiGbl_RootTableList.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiTbAcquireTable (
    ACPI_TABLE_DESC         *TableDesc,
    ACPI_TABLE_HEADER       **TablePtr,
    UINT32                  *TableLength,
    UINT8                   *TableFlags)
{
    ACPI_TABLE_HEADER       *Table = NULL;


    switch (TableDesc->Flags & ACPI_TABLE_ORIGIN_MASK)
    {
    case ACPI_TABLE_ORIGIN_INTERNAL_PHYSICAL:

        Table = AcpiOsMapMemory (TableDesc->Address, TableDesc->Length);
        break;

    case ACPI_TABLE_ORIGIN_INTERNAL_VIRTUAL:
    case ACPI_TABLE_ORIGIN_EXTERNAL_VIRTUAL:

        Table = TableDesc->Pointer;
        break;

    default:

        break;
    }

    /* Table is not valid yet */

    if (!Table)
    {
        return (AE_NO_MEMORY);
    }

    /* Fill the return values */

    *TablePtr = Table;
    *TableLength = TableDesc->Length;
    *TableFlags = TableDesc->Flags;
    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbReleaseTable
 *
 * PARAMETERS:  Table               - Pointer for the table
 *              TableLength         - Length for the table
 *              TableFlags          - Allocation flags for the table
 *
 * RETURN:      None
 *
 * DESCRIPTION: Release a table. The inverse of AcpiTbAcquireTable().
 *
 ******************************************************************************/

void
AcpiTbReleaseTable (
    ACPI_TABLE_HEADER       *Table,
    UINT32                  TableLength,
    UINT8                   TableFlags)
{

    switch (TableFlags & ACPI_TABLE_ORIGIN_MASK)
    {
    case ACPI_TABLE_ORIGIN_INTERNAL_PHYSICAL:

        AcpiOsUnmapMemory (Table, TableLength);
        break;

    case ACPI_TABLE_ORIGIN_INTERNAL_VIRTUAL:
    case ACPI_TABLE_ORIGIN_EXTERNAL_VIRTUAL:
    default:

        break;
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbAcquireTempTable
 *
 * PARAMETERS:  TableDesc           - Table descriptor to be acquired
 *              Address             - Address of the table
 *              Flags               - Allocation flags of the table
 *              Table               - Pointer to the table (required for virtual
 *                                    origins, optional for physical)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This function validates the table header to obtain the length
 *              of a table and fills the table descriptor to make its state as
 *              "INSTALLED". Such a table descriptor is only used for verified
 *              installation.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiTbAcquireTempTable (
    ACPI_TABLE_DESC         *TableDesc,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT8                   Flags,
    ACPI_TABLE_HEADER       *Table)
{
    BOOLEAN                 MappedTable = FALSE;


    switch (Flags & ACPI_TABLE_ORIGIN_MASK)
    {
    case ACPI_TABLE_ORIGIN_INTERNAL_PHYSICAL:

        /* Get the length of the full table from the header */

        if (!Table)
        {
            Table = AcpiOsMapMemory (Address, sizeof (ACPI_TABLE_HEADER));
            if (!Table)
            {
                return (AE_NO_MEMORY);
            }

            MappedTable = TRUE;
        }

        break;

    case ACPI_TABLE_ORIGIN_INTERNAL_VIRTUAL:
    case ACPI_TABLE_ORIGIN_EXTERNAL_VIRTUAL:

        if (!Table)
        {
            return (AE_BAD_PARAMETER);
        }

        break;

    default:

        /* Table is not valid yet */

        return (AE_NO_MEMORY);
    }

    AcpiTbInitTableDescriptor (TableDesc, Address, Flags, Table);
    if (MappedTable)
    {
        AcpiOsUnmapMemory (Table, sizeof (ACPI_TABLE_HEADER));
    }

    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbReleaseTempTable
 *
 * PARAMETERS:  TableDesc           - Table descriptor to be released
 *
 * RETURN:      Status
 *
 * DESCRIPTION: The inverse of AcpiTbAcquireTempTable().
 *
 *****************************************************************************/

void
AcpiTbReleaseTempTable (
    ACPI_TABLE_DESC         *TableDesc)
{

    /*
     * Note that the .Address is maintained by the callers of
     * AcpiTbAcquireTempTable(), thus do not invoke AcpiTbUninstallTable()
     * where .Address will be freed.
     */
    AcpiTbInvalidateTable (TableDesc);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiTbValidateTable
 *
 * PARAMETERS:  TableDesc           - Table descriptor
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This function is called to validate the table, the returned
 *              table descriptor is in "VALIDATED" state.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiTbValidateTable (
    ACPI_TABLE_DESC         *TableDesc)
{
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE (TbValidateTable);


    /* Validate the table if necessary */

    if (!TableDesc->Pointer)
    {
        Status = AcpiTbAcquireTable (TableDesc, &TableDesc->Pointer,
            &TableDesc->Length, &TableDesc->Flags);
        if (!TableDesc->Pointer)
        {
            Status = AE_NO_MEMORY;
        }
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbInvalidateTable
 *
 * PARAMETERS:  TableDesc           - Table descriptor
 *
 * RETURN:      None
 *
 * DESCRIPTION: Invalidate one internal ACPI table, this is the inverse of
 *              AcpiTbValidateTable().
 *
 ******************************************************************************/

void
AcpiTbInvalidateTable (
    ACPI_TABLE_DESC         *TableDesc)
{

    ACPI_FUNCTION_TRACE (TbInvalidateTable);


    /* Table must be validated */

    if (!TableDesc->Pointer)
    {
        return_VOID;
    }

    AcpiTbReleaseTable (TableDesc->Pointer, TableDesc->Length,
        TableDesc->Flags);

    switch (TableDesc->Flags & ACPI_TABLE_ORIGIN_MASK)
    {
    case ACPI_TABLE_ORIGIN_INTERNAL_PHYSICAL:

        TableDesc->Pointer = NULL;
        break;

    case ACPI_TABLE_ORIGIN_INTERNAL_VIRTUAL:
    case ACPI_TABLE_ORIGIN_EXTERNAL_VIRTUAL:
    default:

        break;
    }

    return_VOID;
}


/******************************************************************************
 *
 * FUNCTION:    AcpiTbValidateTempTable
 *
 * PARAMETERS:  TableDesc           - Table descriptor
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This function is called to validate the table, the returned
 *              table descriptor is in "VALIDATED" state.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiTbValidateTempTable (
    ACPI_TABLE_DESC         *TableDesc)
{

    if (!TableDesc->Pointer && !AcpiGbl_EnableTableValidation)
    {
        /*
         * Only validates the header of the table.
         * Note that Length contains the size of the mapping after invoking
         * this work around, this value is required by
         * AcpiTbReleaseTempTable().
         * We can do this because in AcpiInitTableDescriptor(), the Length
         * field of the installed descriptor is filled with the actual
         * table length obtaining from the table header.
         */
        TableDesc->Length = sizeof (ACPI_TABLE_HEADER);
    }

    return (AcpiTbValidateTable (TableDesc));
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbCheckDuplication
 *
 * PARAMETERS:  TableDesc           - Table descriptor
 *              TableIndex          - Where the table index is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Avoid installing duplicated tables. However table override and
 *              user aided dynamic table load is allowed, thus comparing the
 *              address of the table is not sufficient, and checking the entire
 *              table content is required.
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiTbCheckDuplication (
    ACPI_TABLE_DESC         *TableDesc,
    UINT32                  *TableIndex)
{
    UINT32                  i;


    ACPI_FUNCTION_TRACE (TbCheckDuplication);


    /* Check if table is already registered */

    for (i = 0; i < AcpiGbl_RootTableList.CurrentTableCount; ++i)
    {
        /* Do not compare with unverified tables */

        if (!(AcpiGbl_RootTableList.Tables[i].Flags & ACPI_TABLE_IS_VERIFIED))
        {
            continue;
        }

        /*
         * Check for a table match on the entire table length,
         * not just the header.
         */
        if (!AcpiTbCompareTables (TableDesc, i))
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

            return_ACPI_STATUS (AE_ALREADY_EXISTS);
        }
        else
        {
            *TableIndex = i;
            return_ACPI_STATUS (AE_CTRL_TERMINATE);
        }
    }

    /* Indicate no duplication to the caller */

    return_ACPI_STATUS (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AcpiTbVerifyTempTable
 *
 * PARAMETERS:  TableDesc           - Table descriptor
 *              Signature           - Table signature to verify
 *              TableIndex          - Where the table index is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This function is called to validate and verify the table, the
 *              returned table descriptor is in "VALIDATED" state.
 *              Note that 'TableIndex' is required to be set to !NULL to
 *              enable duplication check.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiTbVerifyTempTable (
    ACPI_TABLE_DESC         *TableDesc,
    char                    *Signature,
    UINT32                  *TableIndex)
{
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE (TbVerifyTempTable);


    /* Validate the table */

    Status = AcpiTbValidateTempTable (TableDesc);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    /* If a particular signature is expected (DSDT/FACS), it must match */

    if (Signature &&
        !ACPI_COMPARE_NAMESEG (&TableDesc->Signature, Signature))
    {
        ACPI_BIOS_ERROR ((AE_INFO,
            "Invalid signature 0x%X for ACPI table, expected [%s]",
            TableDesc->Signature.Integer, Signature));
        Status = AE_BAD_SIGNATURE;
        goto InvalidateAndExit;
    }

    if (AcpiGbl_EnableTableValidation)
    {
        /* Verify the checksum */

        Status = AcpiUtVerifyChecksum (TableDesc->Pointer, TableDesc->Length);
        if (ACPI_FAILURE (Status))
        {
            ACPI_EXCEPTION ((AE_INFO, AE_NO_MEMORY,
                "%4.4s 0x%8.8X%8.8X"
                " Attempted table install failed",
                AcpiUtValidNameseg (TableDesc->Signature.Ascii) ?
                    TableDesc->Signature.Ascii : "????",
                ACPI_FORMAT_UINT64 (TableDesc->Address)));

            goto InvalidateAndExit;
        }

        /* Avoid duplications */

        if (TableIndex)
        {
            Status = AcpiTbCheckDuplication (TableDesc, TableIndex);
            if (ACPI_FAILURE (Status))
            {
                if (Status != AE_CTRL_TERMINATE)
                {
                    ACPI_EXCEPTION ((AE_INFO, Status,
                        "%4.4s 0x%8.8X%8.8X"
                        " Table is already loaded",
                        AcpiUtValidNameseg (TableDesc->Signature.Ascii) ?
                            TableDesc->Signature.Ascii : "????",
                        ACPI_FORMAT_UINT64 (TableDesc->Address)));
                }

                goto InvalidateAndExit;
            }
        }

        TableDesc->Flags |= ACPI_TABLE_IS_VERIFIED;
    }

    return_ACPI_STATUS (Status);

InvalidateAndExit:
    AcpiTbInvalidateTable (TableDesc);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbResizeRootTableList
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Expand the size of global table array
 *
 ******************************************************************************/

ACPI_STATUS
AcpiTbResizeRootTableList (
    void)
{
    ACPI_TABLE_DESC         *Tables;
    UINT32                  TableCount;
    UINT32                  CurrentTableCount, MaxTableCount;
    UINT32                  i;


    ACPI_FUNCTION_TRACE (TbResizeRootTableList);


    /* AllowResize flag is a parameter to AcpiInitializeTables */

    if (!(AcpiGbl_RootTableList.Flags & ACPI_ROOT_ALLOW_RESIZE))
    {
        ACPI_ERROR ((AE_INFO, "Resize of Root Table Array is not allowed"));
        return_ACPI_STATUS (AE_SUPPORT);
    }

    /* Increase the Table Array size */

    if (AcpiGbl_RootTableList.Flags & ACPI_ROOT_ORIGIN_ALLOCATED)
    {
        TableCount = AcpiGbl_RootTableList.MaxTableCount;
    }
    else
    {
        TableCount = AcpiGbl_RootTableList.CurrentTableCount;
    }

    MaxTableCount = TableCount + ACPI_ROOT_TABLE_SIZE_INCREMENT;
    Tables = ACPI_ALLOCATE_ZEROED (
        ((ACPI_SIZE) MaxTableCount) * sizeof (ACPI_TABLE_DESC));
    if (!Tables)
    {
        ACPI_ERROR ((AE_INFO, "Could not allocate new root table array"));
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    /* Copy and free the previous table array */

    CurrentTableCount = 0;
    if (AcpiGbl_RootTableList.Tables)
    {
        for (i = 0; i < TableCount; i++)
        {
            if (AcpiGbl_RootTableList.Tables[i].Address)
            {
                memcpy (Tables + CurrentTableCount,
                    AcpiGbl_RootTableList.Tables + i,
                    sizeof (ACPI_TABLE_DESC));
                CurrentTableCount++;
            }
        }

        if (AcpiGbl_RootTableList.Flags & ACPI_ROOT_ORIGIN_ALLOCATED)
        {
            ACPI_FREE (AcpiGbl_RootTableList.Tables);
        }
    }

    AcpiGbl_RootTableList.Tables = Tables;
    AcpiGbl_RootTableList.MaxTableCount = MaxTableCount;
    AcpiGbl_RootTableList.CurrentTableCount = CurrentTableCount;
    AcpiGbl_RootTableList.Flags |= ACPI_ROOT_ORIGIN_ALLOCATED;

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbGetNextTableDescriptor
 *
 * PARAMETERS:  TableIndex          - Where table index is returned
 *              TableDesc           - Where table descriptor is returned
 *
 * RETURN:      Status and table index/descriptor.
 *
 * DESCRIPTION: Allocate a new ACPI table entry to the global table list
 *
 ******************************************************************************/

ACPI_STATUS
AcpiTbGetNextTableDescriptor (
    UINT32                  *TableIndex,
    ACPI_TABLE_DESC         **TableDesc)
{
    ACPI_STATUS             Status;
    UINT32                  i;


    /* Ensure that there is room for the table in the Root Table List */

    if (AcpiGbl_RootTableList.CurrentTableCount >=
        AcpiGbl_RootTableList.MaxTableCount)
    {
        Status = AcpiTbResizeRootTableList();
        if (ACPI_FAILURE (Status))
        {
            return (Status);
        }
    }

    i = AcpiGbl_RootTableList.CurrentTableCount;
    AcpiGbl_RootTableList.CurrentTableCount++;

    if (TableIndex)
    {
        *TableIndex = i;
    }
    if (TableDesc)
    {
        *TableDesc = &AcpiGbl_RootTableList.Tables[i];
    }

    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbTerminate
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Delete all internal ACPI tables
 *
 ******************************************************************************/

void
AcpiTbTerminate (
    void)
{
    UINT32                  i;


    ACPI_FUNCTION_TRACE (TbTerminate);


    (void) AcpiUtAcquireMutex (ACPI_MTX_TABLES);

    /* Delete the individual tables */

    for (i = 0; i < AcpiGbl_RootTableList.CurrentTableCount; i++)
    {
        AcpiTbUninstallTable (&AcpiGbl_RootTableList.Tables[i]);
    }

    /*
     * Delete the root table array if allocated locally. Array cannot be
     * mapped, so we don't need to check for that flag.
     */
    if (AcpiGbl_RootTableList.Flags & ACPI_ROOT_ORIGIN_ALLOCATED)
    {
        ACPI_FREE (AcpiGbl_RootTableList.Tables);
    }

    AcpiGbl_RootTableList.Tables = NULL;
    AcpiGbl_RootTableList.Flags = 0;
    AcpiGbl_RootTableList.CurrentTableCount = 0;

    ACPI_DEBUG_PRINT ((ACPI_DB_INFO, "ACPI Tables freed\n"));

    (void) AcpiUtReleaseMutex (ACPI_MTX_TABLES);
    return_VOID;
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbDeleteNamespaceByOwner
 *
 * PARAMETERS:  TableIndex          - Table index
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Delete all namespace objects created when this table was loaded.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiTbDeleteNamespaceByOwner (
    UINT32                  TableIndex)
{
    ACPI_OWNER_ID           OwnerId;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (TbDeleteNamespaceByOwner);


    Status = AcpiUtAcquireMutex (ACPI_MTX_TABLES);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    if (TableIndex >= AcpiGbl_RootTableList.CurrentTableCount)
    {
        /* The table index does not exist */

        (void) AcpiUtReleaseMutex (ACPI_MTX_TABLES);
        return_ACPI_STATUS (AE_NOT_EXIST);
    }

    /* Get the owner ID for this table, used to delete namespace nodes */

    OwnerId = AcpiGbl_RootTableList.Tables[TableIndex].OwnerId;
    (void) AcpiUtReleaseMutex (ACPI_MTX_TABLES);

    /*
     * Need to acquire the namespace writer lock to prevent interference
     * with any concurrent namespace walks. The interpreter must be
     * released during the deletion since the acquisition of the deletion
     * lock may block, and also since the execution of a namespace walk
     * must be allowed to use the interpreter.
     */
    Status = AcpiUtAcquireWriteLock (&AcpiGbl_NamespaceRwLock);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    AcpiNsDeleteNamespaceByOwner (OwnerId);
    AcpiUtReleaseWriteLock (&AcpiGbl_NamespaceRwLock);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbAllocateOwnerId
 *
 * PARAMETERS:  TableIndex          - Table index
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Allocates OwnerId in TableDesc
 *
 ******************************************************************************/

ACPI_STATUS
AcpiTbAllocateOwnerId (
    UINT32                  TableIndex)
{
    ACPI_STATUS             Status = AE_BAD_PARAMETER;


    ACPI_FUNCTION_TRACE (TbAllocateOwnerId);


    (void) AcpiUtAcquireMutex (ACPI_MTX_TABLES);
    if (TableIndex < AcpiGbl_RootTableList.CurrentTableCount)
    {
        Status = AcpiUtAllocateOwnerId (
            &(AcpiGbl_RootTableList.Tables[TableIndex].OwnerId));
    }

    (void) AcpiUtReleaseMutex (ACPI_MTX_TABLES);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbReleaseOwnerId
 *
 * PARAMETERS:  TableIndex          - Table index
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Releases OwnerId in TableDesc
 *
 ******************************************************************************/

ACPI_STATUS
AcpiTbReleaseOwnerId (
    UINT32                  TableIndex)
{
    ACPI_STATUS             Status = AE_BAD_PARAMETER;


    ACPI_FUNCTION_TRACE (TbReleaseOwnerId);


    (void) AcpiUtAcquireMutex (ACPI_MTX_TABLES);
    if (TableIndex < AcpiGbl_RootTableList.CurrentTableCount)
    {
        AcpiUtReleaseOwnerId (
            &(AcpiGbl_RootTableList.Tables[TableIndex].OwnerId));
        Status = AE_OK;
    }

    (void) AcpiUtReleaseMutex (ACPI_MTX_TABLES);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbGetOwnerId
 *
 * PARAMETERS:  TableIndex          - Table index
 *              OwnerId             - Where the table OwnerId is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: returns OwnerId for the ACPI table
 *
 ******************************************************************************/

ACPI_STATUS
AcpiTbGetOwnerId (
    UINT32                  TableIndex,
    ACPI_OWNER_ID           *OwnerId)
{
    ACPI_STATUS             Status = AE_BAD_PARAMETER;


    ACPI_FUNCTION_TRACE (TbGetOwnerId);


    (void) AcpiUtAcquireMutex (ACPI_MTX_TABLES);
    if (TableIndex < AcpiGbl_RootTableList.CurrentTableCount)
    {
        *OwnerId = AcpiGbl_RootTableList.Tables[TableIndex].OwnerId;
        Status = AE_OK;
    }

    (void) AcpiUtReleaseMutex (ACPI_MTX_TABLES);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbIsTableLoaded
 *
 * PARAMETERS:  TableIndex          - Index into the root table
 *
 * RETURN:      Table Loaded Flag
 *
 ******************************************************************************/

BOOLEAN
AcpiTbIsTableLoaded (
    UINT32                  TableIndex)
{
    BOOLEAN                 IsLoaded = FALSE;


    (void) AcpiUtAcquireMutex (ACPI_MTX_TABLES);
    if (TableIndex < AcpiGbl_RootTableList.CurrentTableCount)
    {
        IsLoaded = (BOOLEAN)
            (AcpiGbl_RootTableList.Tables[TableIndex].Flags &
            ACPI_TABLE_IS_LOADED);
    }

    (void) AcpiUtReleaseMutex (ACPI_MTX_TABLES);
    return (IsLoaded);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbSetTableLoadedFlag
 *
 * PARAMETERS:  TableIndex          - Table index
 *              IsLoaded            - TRUE if table is loaded, FALSE otherwise
 *
 * RETURN:      None
 *
 * DESCRIPTION: Sets the table loaded flag to either TRUE or FALSE.
 *
 ******************************************************************************/

void
AcpiTbSetTableLoadedFlag (
    UINT32                  TableIndex,
    BOOLEAN                 IsLoaded)
{

    (void) AcpiUtAcquireMutex (ACPI_MTX_TABLES);
    if (TableIndex < AcpiGbl_RootTableList.CurrentTableCount)
    {
        if (IsLoaded)
        {
            AcpiGbl_RootTableList.Tables[TableIndex].Flags |=
                ACPI_TABLE_IS_LOADED;
        }
        else
        {
            AcpiGbl_RootTableList.Tables[TableIndex].Flags &=
                ~ACPI_TABLE_IS_LOADED;
        }
    }

    (void) AcpiUtReleaseMutex (ACPI_MTX_TABLES);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbLoadTable
 *
 * PARAMETERS:  TableIndex              - Table index
 *              ParentNode              - Where table index is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Load an ACPI table
 *
 ******************************************************************************/

ACPI_STATUS
AcpiTbLoadTable (
    UINT32                  TableIndex,
    ACPI_NAMESPACE_NODE     *ParentNode)
{
    ACPI_TABLE_HEADER       *Table;
    ACPI_STATUS             Status;
    ACPI_OWNER_ID           OwnerId;


    ACPI_FUNCTION_TRACE (TbLoadTable);


    /*
     * Note: Now table is "INSTALLED", it must be validated before
     * using.
     */
    Status = AcpiGetTableByIndex (TableIndex, &Table);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    Status = AcpiNsLoadTable (TableIndex, ParentNode);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /*
     * Update GPEs for any new _Lxx/_Exx methods. Ignore errors. The host is
     * responsible for discovering any new wake GPEs by running _PRW methods
     * that may have been loaded by this table.
     */
    Status = AcpiTbGetOwnerId (TableIndex, &OwnerId);
    if (ACPI_SUCCESS (Status))
    {
        AcpiEvUpdateGpes (OwnerId);
    }

    /* Invoke table handler */

    AcpiTbNotifyTable (ACPI_TABLE_EVENT_LOAD, Table);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbInstallAndLoadTable
 *
 * PARAMETERS:  Address                 - Physical address of the table
 *              Flags                   - Allocation flags of the table
 *              Table                   - Pointer to the table (required for
 *                                        virtual origins, optional for
 *                                        physical)
 *              Override                - Whether override should be performed
 *              TableIndex              - Where table index is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Install and load an ACPI table
 *
 ******************************************************************************/

ACPI_STATUS
AcpiTbInstallAndLoadTable (
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT8                   Flags,
    ACPI_TABLE_HEADER       *Table,
    BOOLEAN                 Override,
    UINT32                  *TableIndex)
{
    ACPI_STATUS             Status;
    UINT32                  i;


    ACPI_FUNCTION_TRACE (TbInstallAndLoadTable);


    /* Install the table and load it into the namespace */

    Status = AcpiTbInstallStandardTable (Address, Flags, Table, TRUE,
        Override, &i);
    if (ACPI_FAILURE (Status))
    {
        goto Exit;
    }

    Status = AcpiTbLoadTable (i, AcpiGbl_RootNode);

Exit:
    *TableIndex = i;
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbUnloadTable
 *
 * PARAMETERS:  TableIndex              - Table index
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Unload an ACPI table
 *
 ******************************************************************************/

ACPI_STATUS
AcpiTbUnloadTable (
    UINT32                  TableIndex)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_TABLE_HEADER       *Table;


    ACPI_FUNCTION_TRACE (TbUnloadTable);


    /* Ensure the table is still loaded */

    if (!AcpiTbIsTableLoaded (TableIndex))
    {
        return_ACPI_STATUS (AE_NOT_EXIST);
    }

    /* Invoke table handler */

    Status = AcpiGetTableByIndex (TableIndex, &Table);
    if (ACPI_SUCCESS (Status))
    {
        AcpiTbNotifyTable (ACPI_TABLE_EVENT_UNLOAD, Table);
    }

    /* Delete the portion of the namespace owned by this table */

    Status = AcpiTbDeleteNamespaceByOwner (TableIndex);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    (void) AcpiTbReleaseOwnerId (TableIndex);
    AcpiTbSetTableLoadedFlag (TableIndex, FALSE);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbNotifyTable
 *
 * PARAMETERS:  Event               - Table event
 *              Table               - Validated table pointer
 *
 * RETURN:      None
 *
 * DESCRIPTION: Notify a table event to the users.
 *
 ******************************************************************************/

void
AcpiTbNotifyTable (
    UINT32                          Event,
    void                            *Table)
{
    /* Invoke table handler if present */

    if (AcpiGbl_TableHandler)
    {
        (void) AcpiGbl_TableHandler (Event, Table,
            AcpiGbl_TableHandlerContext);
    }
}
