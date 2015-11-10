/******************************************************************************
 *
 * Module Name: tbdata - Table manager data structure functions
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
#include "acnamesp.h"
#include "actables.h"

#define _COMPONENT          ACPI_TABLES
        ACPI_MODULE_NAME    ("tbdata")


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
     * Initialize the table descriptor. Set the pointer to NULL, since the
     * table is not fully mapped at this time.
     */
    ACPI_MEMSET (TableDesc, 0, sizeof (ACPI_TABLE_DESC));
    TableDesc->Address = Address;
    TableDesc->Length = Table->Length;
    TableDesc->Flags = Flags;
    ACPI_MOVE_32_TO_32 (TableDesc->Signature.Ascii, Table->Signature);
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

        Table = ACPI_CAST_PTR (ACPI_TABLE_HEADER,
                    ACPI_PHYSADDR_TO_PTR (TableDesc->Address));
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
    UINT8                   Flags)
{
    ACPI_TABLE_HEADER       *TableHeader;


    switch (Flags & ACPI_TABLE_ORIGIN_MASK)
    {
    case ACPI_TABLE_ORIGIN_INTERNAL_PHYSICAL:

        /* Get the length of the full table from the header */

        TableHeader = AcpiOsMapMemory (Address, sizeof (ACPI_TABLE_HEADER));
        if (!TableHeader)
        {
            return (AE_NO_MEMORY);
        }

        AcpiTbInitTableDescriptor (TableDesc, Address, Flags, TableHeader);
        AcpiOsUnmapMemory (TableHeader, sizeof (ACPI_TABLE_HEADER));
        return (AE_OK);

    case ACPI_TABLE_ORIGIN_INTERNAL_VIRTUAL:
    case ACPI_TABLE_ORIGIN_EXTERNAL_VIRTUAL:

        TableHeader = ACPI_CAST_PTR (ACPI_TABLE_HEADER,
                        ACPI_PHYSADDR_TO_PTR (Address));
        if (!TableHeader)
        {
            return (AE_NO_MEMORY);
        }

        AcpiTbInitTableDescriptor (TableDesc, Address, Flags, TableHeader);
        return (AE_OK);

    default:

        break;
    }

    /* Table is not valid yet */

    return (AE_NO_MEMORY);
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
    TableDesc->Pointer = NULL;

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

    if (!TableDesc->Pointer && !AcpiGbl_VerifyTableChecksum)
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


/******************************************************************************
 *
 * FUNCTION:    AcpiTbVerifyTempTable
 *
 * PARAMETERS:  TableDesc           - Table descriptor
 *              Signature           - Table signature to verify
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This function is called to validate and verify the table, the
 *              returned table descriptor is in "VALIDATED" state.
 *
 *****************************************************************************/

ACPI_STATUS
AcpiTbVerifyTempTable (
    ACPI_TABLE_DESC         *TableDesc,
    char                    *Signature)
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
        !ACPI_COMPARE_NAME (&TableDesc->Signature, Signature))
    {
        ACPI_BIOS_ERROR ((AE_INFO,
            "Invalid signature 0x%X for ACPI table, expected [%s]",
            TableDesc->Signature.Integer, Signature));
        Status = AE_BAD_SIGNATURE;
        goto InvalidateAndExit;
    }

    /* Verify the checksum */

    if (AcpiGbl_VerifyTableChecksum)
    {
        Status = AcpiTbVerifyChecksum (TableDesc->Pointer, TableDesc->Length);
        if (ACPI_FAILURE (Status))
        {
            ACPI_EXCEPTION ((AE_INFO, AE_NO_MEMORY,
                "%4.4s 0x%8.8X%8.8X"
                " Attempted table install failed",
                AcpiUtValidAcpiName (TableDesc->Signature.Ascii) ?
                    TableDesc->Signature.Ascii : "????",
                ACPI_FORMAT_UINT64 (TableDesc->Address)));
            goto InvalidateAndExit;
        }
    }

    return_ACPI_STATUS (AE_OK);

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

    Tables = ACPI_ALLOCATE_ZEROED (
        ((ACPI_SIZE) TableCount + ACPI_ROOT_TABLE_SIZE_INCREMENT) *
        sizeof (ACPI_TABLE_DESC));
    if (!Tables)
    {
        ACPI_ERROR ((AE_INFO, "Could not allocate new root table array"));
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    /* Copy and free the previous table array */

    if (AcpiGbl_RootTableList.Tables)
    {
        ACPI_MEMCPY (Tables, AcpiGbl_RootTableList.Tables,
            (ACPI_SIZE) TableCount * sizeof (ACPI_TABLE_DESC));

        if (AcpiGbl_RootTableList.Flags & ACPI_ROOT_ORIGIN_ALLOCATED)
        {
            ACPI_FREE (AcpiGbl_RootTableList.Tables);
        }
    }

    AcpiGbl_RootTableList.Tables = Tables;
    AcpiGbl_RootTableList.MaxTableCount =
        TableCount + ACPI_ROOT_TABLE_SIZE_INCREMENT;
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
    (void) AcpiUtReleaseMutex (ACPI_MTX_INTERPRETER);
    Status = AcpiUtAcquireWriteLock (&AcpiGbl_NamespaceRwLock);

    AcpiNsDeleteNamespaceByOwner (OwnerId);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    AcpiUtReleaseWriteLock (&AcpiGbl_NamespaceRwLock);

    Status = AcpiUtAcquireMutex (ACPI_MTX_INTERPRETER);
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
