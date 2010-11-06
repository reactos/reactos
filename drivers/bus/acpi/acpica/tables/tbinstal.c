/******************************************************************************
 *
 * Module Name: tbinstal - ACPI table installation and removal
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2009, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights.  You may have additional license terms from the party that provided
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
 * to or modifications of the Original Intel Code.  No other license or right
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
 * and the following Disclaimer and Export Compliance provision.  In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change.  Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee.  Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution.  In
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
 * HERE.  ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT,  ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.  THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government.  In the
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


#define __TBINSTAL_C__

#include "acpi.h"
#include "accommon.h"
#include "acnamesp.h"
#include "actables.h"


#define _COMPONENT          ACPI_TABLES
        ACPI_MODULE_NAME    ("tbinstal")


/******************************************************************************
 *
 * FUNCTION:    AcpiTbVerifyTable
 *
 * PARAMETERS:  TableDesc           - table
 *
 * RETURN:      Status
 *
 * DESCRIPTION: this function is called to verify and map table
 *
 *****************************************************************************/

ACPI_STATUS
AcpiTbVerifyTable (
    ACPI_TABLE_DESC         *TableDesc)
{
    ACPI_STATUS             Status = AE_OK;


    ACPI_FUNCTION_TRACE (TbVerifyTable);


    /* Map the table if necessary */

    if (!TableDesc->Pointer)
    {
        if ((TableDesc->Flags & ACPI_TABLE_ORIGIN_MASK) ==
            ACPI_TABLE_ORIGIN_MAPPED)
        {
            TableDesc->Pointer = AcpiOsMapMemory (
                TableDesc->Address, TableDesc->Length);
        }

        if (!TableDesc->Pointer)
        {
            return_ACPI_STATUS (AE_NO_MEMORY);
        }
    }

    /* FACS is the odd table, has no standard ACPI header and no checksum */

    if (!ACPI_COMPARE_NAME (&TableDesc->Signature, ACPI_SIG_FACS))
    {
        /* Always calculate checksum, ignore bad checksum if requested */

        Status = AcpiTbVerifyChecksum (TableDesc->Pointer, TableDesc->Length);
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbAddTable
 *
 * PARAMETERS:  TableDesc           - Table descriptor
 *              TableIndex          - Where the table index is returned
 *
 * RETURN:      Status
 *
 * DESCRIPTION: This function is called to add an ACPI table. It is used to
 *              dynamically load tables via the Load and LoadTable AML
 *              operators.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiTbAddTable (
    ACPI_TABLE_DESC         *TableDesc,
    UINT32                  *TableIndex)
{
    UINT32                  i;
    ACPI_STATUS             Status = AE_OK;
    ACPI_TABLE_HEADER       *OverrideTable = NULL;


    ACPI_FUNCTION_TRACE (TbAddTable);


    if (!TableDesc->Pointer)
    {
        Status = AcpiTbVerifyTable (TableDesc);
        if (ACPI_FAILURE (Status) || !TableDesc->Pointer)
        {
            return_ACPI_STATUS (Status);
        }
    }

    /*
     * Originally, we checked the table signature for "SSDT" or "PSDT" here.
     * Next, we added support for OEMx tables, signature "OEM".
     * Valid tables were encountered with a null signature, so we've just
     * given up on validating the signature, since it seems to be a waste
     * of code. The original code was removed (05/2008).
     */

    (void) AcpiUtAcquireMutex (ACPI_MTX_TABLES);

    /* Check if table is already registered */

    for (i = 0; i < AcpiGbl_RootTableList.Count; ++i)
    {
        if (!AcpiGbl_RootTableList.Tables[i].Pointer)
        {
            Status = AcpiTbVerifyTable (&AcpiGbl_RootTableList.Tables[i]);
            if (ACPI_FAILURE (Status) ||
                !AcpiGbl_RootTableList.Tables[i].Pointer)
            {
                continue;
            }
        }

        /*
         * Check for a table match on the entire table length,
         * not just the header.
         */
        if (TableDesc->Length != AcpiGbl_RootTableList.Tables[i].Length)
        {
            continue;
        }

        if (ACPI_MEMCMP (TableDesc->Pointer,
                AcpiGbl_RootTableList.Tables[i].Pointer,
                AcpiGbl_RootTableList.Tables[i].Length))
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

        /*
         * Table is already registered.
         * We can delete the table that was passed as a parameter.
         */
        AcpiTbDeleteTable (TableDesc);
        *TableIndex = i;

        if (AcpiGbl_RootTableList.Tables[i].Flags & ACPI_TABLE_IS_LOADED)
        {
            /* Table is still loaded, this is an error */

            Status = AE_ALREADY_EXISTS;
            goto Release;
        }
        else
        {
            /* Table was unloaded, allow it to be reloaded */

            TableDesc->Pointer = AcpiGbl_RootTableList.Tables[i].Pointer;
            TableDesc->Address = AcpiGbl_RootTableList.Tables[i].Address;
            Status = AE_OK;
            goto PrintHeader;
        }
    }

    /*
     * ACPI Table Override:
     * Allow the host to override dynamically loaded tables.
     */
    Status = AcpiOsTableOverride (TableDesc->Pointer, &OverrideTable);
    if (ACPI_SUCCESS (Status) && OverrideTable)
    {
        ACPI_INFO ((AE_INFO,
            "%4.4s @ 0x%p Table override, replaced with:",
            TableDesc->Pointer->Signature,
            ACPI_CAST_PTR (void, TableDesc->Address)));

        /* We can delete the table that was passed as a parameter */

        AcpiTbDeleteTable (TableDesc);

        /* Setup descriptor for the new table */

        TableDesc->Address = ACPI_PTR_TO_PHYSADDR (OverrideTable);
        TableDesc->Pointer = OverrideTable;
        TableDesc->Length = OverrideTable->Length;
        TableDesc->Flags = ACPI_TABLE_ORIGIN_OVERRIDE;
    }

    /* Add the table to the global root table list */

    Status = AcpiTbStoreTable (TableDesc->Address, TableDesc->Pointer,
                TableDesc->Length, TableDesc->Flags, TableIndex);
    if (ACPI_FAILURE (Status))
    {
        goto Release;
    }

PrintHeader:
    AcpiTbPrintTableHeader (TableDesc->Address, TableDesc->Pointer);

Release:
    (void) AcpiUtReleaseMutex (ACPI_MTX_TABLES);
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


    ACPI_FUNCTION_TRACE (TbResizeRootTableList);


    /* AllowResize flag is a parameter to AcpiInitializeTables */

    if (!(AcpiGbl_RootTableList.Flags & ACPI_ROOT_ALLOW_RESIZE))
    {
        ACPI_ERROR ((AE_INFO, "Resize of Root Table Array is not allowed"));
        return_ACPI_STATUS (AE_SUPPORT);
    }

    /* Increase the Table Array size */

    Tables = ACPI_ALLOCATE_ZEROED (
        ((ACPI_SIZE) AcpiGbl_RootTableList.Size +
            ACPI_ROOT_TABLE_SIZE_INCREMENT) *
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
            (ACPI_SIZE) AcpiGbl_RootTableList.Size * sizeof (ACPI_TABLE_DESC));

        if (AcpiGbl_RootTableList.Flags & ACPI_ROOT_ORIGIN_ALLOCATED)
        {
            ACPI_FREE (AcpiGbl_RootTableList.Tables);
        }
    }

    AcpiGbl_RootTableList.Tables = Tables;
    AcpiGbl_RootTableList.Size += ACPI_ROOT_TABLE_SIZE_INCREMENT;
    AcpiGbl_RootTableList.Flags |= (UINT8) ACPI_ROOT_ORIGIN_ALLOCATED;

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbStoreTable
 *
 * PARAMETERS:  Address             - Table address
 *              Table               - Table header
 *              Length              - Table length
 *              Flags               - flags
 *
 * RETURN:      Status and table index.
 *
 * DESCRIPTION: Add an ACPI table to the global table list
 *
 ******************************************************************************/

ACPI_STATUS
AcpiTbStoreTable (
    ACPI_PHYSICAL_ADDRESS   Address,
    ACPI_TABLE_HEADER       *Table,
    UINT32                  Length,
    UINT8                   Flags,
    UINT32                  *TableIndex)
{
    ACPI_STATUS             Status = AE_OK;


    /* Ensure that there is room for the table in the Root Table List */

    if (AcpiGbl_RootTableList.Count >= AcpiGbl_RootTableList.Size)
    {
        Status = AcpiTbResizeRootTableList();
        if (ACPI_FAILURE (Status))
        {
            return (Status);
        }
    }

    /* Initialize added table */

    AcpiGbl_RootTableList.Tables[AcpiGbl_RootTableList.Count].Address = Address;
    AcpiGbl_RootTableList.Tables[AcpiGbl_RootTableList.Count].Pointer = Table;
    AcpiGbl_RootTableList.Tables[AcpiGbl_RootTableList.Count].Length = Length;
    AcpiGbl_RootTableList.Tables[AcpiGbl_RootTableList.Count].OwnerId = 0;
    AcpiGbl_RootTableList.Tables[AcpiGbl_RootTableList.Count].Flags = Flags;

    ACPI_MOVE_32_TO_32 (
        &(AcpiGbl_RootTableList.Tables[AcpiGbl_RootTableList.Count].Signature),
        Table->Signature);

    *TableIndex = AcpiGbl_RootTableList.Count;
    AcpiGbl_RootTableList.Count++;
    return (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbDeleteTable
 *
 * PARAMETERS:  TableIndex          - Table index
 *
 * RETURN:      None
 *
 * DESCRIPTION: Delete one internal ACPI table
 *
 ******************************************************************************/

void
AcpiTbDeleteTable (
    ACPI_TABLE_DESC         *TableDesc)
{

    /* Table must be mapped or allocated */

    if (!TableDesc->Pointer)
    {
        return;
    }

    switch (TableDesc->Flags & ACPI_TABLE_ORIGIN_MASK)
    {
    case ACPI_TABLE_ORIGIN_MAPPED:
        AcpiOsUnmapMemory (TableDesc->Pointer, TableDesc->Length);
        break;

    case ACPI_TABLE_ORIGIN_ALLOCATED:
        ACPI_FREE (TableDesc->Pointer);
        break;

    default:
        break;
    }

    TableDesc->Pointer = NULL;
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

    for (i = 0; i < AcpiGbl_RootTableList.Count; i++)
    {
        AcpiTbDeleteTable (&AcpiGbl_RootTableList.Tables[i]);
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
    AcpiGbl_RootTableList.Count = 0;

    ACPI_DEBUG_PRINT ((ACPI_DB_INFO, "ACPI Tables freed\n"));
    (void) AcpiUtReleaseMutex (ACPI_MTX_TABLES);
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

    if (TableIndex >= AcpiGbl_RootTableList.Count)
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
    if (TableIndex < AcpiGbl_RootTableList.Count)
    {
        Status = AcpiUtAllocateOwnerId
                    (&(AcpiGbl_RootTableList.Tables[TableIndex].OwnerId));
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
    if (TableIndex < AcpiGbl_RootTableList.Count)
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
    if (TableIndex < AcpiGbl_RootTableList.Count)
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
 * PARAMETERS:  TableIndex          - Table index
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
    if (TableIndex < AcpiGbl_RootTableList.Count)
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
    if (TableIndex < AcpiGbl_RootTableList.Count)
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

