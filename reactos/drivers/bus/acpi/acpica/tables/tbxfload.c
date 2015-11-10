/******************************************************************************
 *
 * Module Name: tbxfload - Table load/unload external interfaces
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

#define EXPORT_ACPI_INTERFACES

#include "acpi.h"
#include "accommon.h"
#include "acnamesp.h"
#include "actables.h"

#define _COMPONENT          ACPI_TABLES
        ACPI_MODULE_NAME    ("tbxfload")

/* Local prototypes */

static ACPI_STATUS
AcpiTbLoadNamespace (
    void);


/*******************************************************************************
 *
 * FUNCTION:    AcpiLoadTables
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Load the ACPI tables from the RSDT/XSDT
 *
 ******************************************************************************/

ACPI_STATUS
AcpiLoadTables (
    void)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiLoadTables);


    /* Load the namespace from the tables */

    Status = AcpiTbLoadNamespace ();
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status,
            "While loading namespace from ACPI tables"));
    }

    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL_INIT (AcpiLoadTables)


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbLoadNamespace
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Load the namespace from the DSDT and all SSDTs/PSDTs found in
 *              the RSDT/XSDT.
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiTbLoadNamespace (
    void)
{
    ACPI_STATUS             Status;
    UINT32                  i;
    ACPI_TABLE_HEADER       *NewDsdt;


    ACPI_FUNCTION_TRACE (TbLoadNamespace);


    (void) AcpiUtAcquireMutex (ACPI_MTX_TABLES);

    /*
     * Load the namespace. The DSDT is required, but any SSDT and
     * PSDT tables are optional. Verify the DSDT.
     */
    if (!AcpiGbl_RootTableList.CurrentTableCount ||
        !ACPI_COMPARE_NAME (
            &(AcpiGbl_RootTableList.Tables[ACPI_TABLE_INDEX_DSDT].Signature),
            ACPI_SIG_DSDT) ||
         ACPI_FAILURE (AcpiTbValidateTable (
            &AcpiGbl_RootTableList.Tables[ACPI_TABLE_INDEX_DSDT])))
    {
        Status = AE_NO_ACPI_TABLES;
        goto UnlockAndExit;
    }

    /*
     * Save the DSDT pointer for simple access. This is the mapped memory
     * address. We must take care here because the address of the .Tables
     * array can change dynamically as tables are loaded at run-time. Note:
     * .Pointer field is not validated until after call to AcpiTbValidateTable.
     */
    AcpiGbl_DSDT = AcpiGbl_RootTableList.Tables[ACPI_TABLE_INDEX_DSDT].Pointer;

    /*
     * Optionally copy the entire DSDT to local memory (instead of simply
     * mapping it.) There are some BIOSs that corrupt or replace the original
     * DSDT, creating the need for this option. Default is FALSE, do not copy
     * the DSDT.
     */
    if (AcpiGbl_CopyDsdtLocally)
    {
        NewDsdt = AcpiTbCopyDsdt (ACPI_TABLE_INDEX_DSDT);
        if (NewDsdt)
        {
            AcpiGbl_DSDT = NewDsdt;
        }
    }

    /*
     * Save the original DSDT header for detection of table corruption
     * and/or replacement of the DSDT from outside the OS.
     */
    ACPI_MEMCPY (&AcpiGbl_OriginalDsdtHeader, AcpiGbl_DSDT,
        sizeof (ACPI_TABLE_HEADER));

    (void) AcpiUtReleaseMutex (ACPI_MTX_TABLES);

    /* Load and parse tables */

    Status = AcpiNsLoadTable (ACPI_TABLE_INDEX_DSDT, AcpiGbl_RootNode);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Load any SSDT or PSDT tables. Note: Loop leaves tables locked */

    (void) AcpiUtAcquireMutex (ACPI_MTX_TABLES);
    for (i = 0; i < AcpiGbl_RootTableList.CurrentTableCount; ++i)
    {
        if ((!ACPI_COMPARE_NAME (&(AcpiGbl_RootTableList.Tables[i].Signature),
                    ACPI_SIG_SSDT) &&
             !ACPI_COMPARE_NAME (&(AcpiGbl_RootTableList.Tables[i].Signature),
                    ACPI_SIG_PSDT)) ||
             ACPI_FAILURE (AcpiTbValidateTable (
                &AcpiGbl_RootTableList.Tables[i])))
        {
            continue;
        }

        /* Ignore errors while loading tables, get as many as possible */

        (void) AcpiUtReleaseMutex (ACPI_MTX_TABLES);
        (void) AcpiNsLoadTable (i, AcpiGbl_RootNode);
        (void) AcpiUtAcquireMutex (ACPI_MTX_TABLES);
    }

    ACPI_INFO ((AE_INFO, "All ACPI Tables successfully acquired"));

UnlockAndExit:
    (void) AcpiUtReleaseMutex (ACPI_MTX_TABLES);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiInstallTable
 *
 * PARAMETERS:  Address             - Address of the ACPI table to be installed.
 *              Physical            - Whether the address is a physical table
 *                                    address or not
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Dynamically install an ACPI table.
 *              Note: This function should only be invoked after
 *                    AcpiInitializeTables() and before AcpiLoadTables().
 *
 ******************************************************************************/

ACPI_STATUS
AcpiInstallTable (
    ACPI_PHYSICAL_ADDRESS   Address,
    BOOLEAN                 Physical)
{
    ACPI_STATUS             Status;
    UINT8                   Flags;
    UINT32                  TableIndex;


    ACPI_FUNCTION_TRACE (AcpiInstallTable);


    if (Physical)
    {
        Flags = ACPI_TABLE_ORIGIN_EXTERNAL_VIRTUAL;
    }
    else
    {
        Flags = ACPI_TABLE_ORIGIN_INTERNAL_PHYSICAL;
    }

    Status = AcpiTbInstallStandardTable (Address, Flags,
        FALSE, FALSE, &TableIndex);

    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL_INIT (AcpiInstallTable)


/*******************************************************************************
 *
 * FUNCTION:    AcpiLoadTable
 *
 * PARAMETERS:  Table               - Pointer to a buffer containing the ACPI
 *                                    table to be loaded.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Dynamically load an ACPI table from the caller's buffer. Must
 *              be a valid ACPI table with a valid ACPI table header.
 *              Note1: Mainly intended to support hotplug addition of SSDTs.
 *              Note2: Does not copy the incoming table. User is responsible
 *              to ensure that the table is not deleted or unmapped.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiLoadTable (
    ACPI_TABLE_HEADER       *Table)
{
    ACPI_STATUS             Status;
    UINT32                  TableIndex;


    ACPI_FUNCTION_TRACE (AcpiLoadTable);


    /* Parameter validation */

    if (!Table)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* Must acquire the interpreter lock during this operation */

    Status = AcpiUtAcquireMutex (ACPI_MTX_INTERPRETER);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Install the table and load it into the namespace */

    ACPI_INFO ((AE_INFO, "Host-directed Dynamic ACPI Table Load:"));
    (void) AcpiUtAcquireMutex (ACPI_MTX_TABLES);

    Status = AcpiTbInstallStandardTable (ACPI_PTR_TO_PHYSADDR (Table),
                ACPI_TABLE_ORIGIN_EXTERNAL_VIRTUAL, TRUE, FALSE,
                &TableIndex);

    (void) AcpiUtReleaseMutex (ACPI_MTX_TABLES);
    if (ACPI_FAILURE (Status))
    {
        goto UnlockAndExit;
    }

    /*
     * Note: Now table is "INSTALLED", it must be validated before
     * using.
     */
    Status = AcpiTbValidateTable (&AcpiGbl_RootTableList.Tables[TableIndex]);
    if (ACPI_FAILURE (Status))
    {
        goto UnlockAndExit;
    }

    Status = AcpiNsLoadTable (TableIndex, AcpiGbl_RootNode);

    /* Invoke table handler if present */

    if (AcpiGbl_TableHandler)
    {
        (void) AcpiGbl_TableHandler (ACPI_TABLE_EVENT_LOAD, Table,
                    AcpiGbl_TableHandlerContext);
    }

UnlockAndExit:
    (void) AcpiUtReleaseMutex (ACPI_MTX_INTERPRETER);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiLoadTable)


/*******************************************************************************
 *
 * FUNCTION:    AcpiUnloadParentTable
 *
 * PARAMETERS:  Object              - Handle to any namespace object owned by
 *                                    the table to be unloaded
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Via any namespace object within an SSDT or OEMx table, unloads
 *              the table and deletes all namespace objects associated with
 *              that table. Unloading of the DSDT is not allowed.
 *              Note: Mainly intended to support hotplug removal of SSDTs.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUnloadParentTable (
    ACPI_HANDLE             Object)
{
    ACPI_NAMESPACE_NODE     *Node = ACPI_CAST_PTR (ACPI_NAMESPACE_NODE, Object);
    ACPI_STATUS             Status = AE_NOT_EXIST;
    ACPI_OWNER_ID           OwnerId;
    UINT32                  i;


    ACPI_FUNCTION_TRACE (AcpiUnloadParentTable);


    /* Parameter validation */

    if (!Object)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /*
     * The node OwnerId is currently the same as the parent table ID.
     * However, this could change in the future.
     */
    OwnerId = Node->OwnerId;
    if (!OwnerId)
    {
        /* OwnerId==0 means DSDT is the owner. DSDT cannot be unloaded */

        return_ACPI_STATUS (AE_TYPE);
    }

    /* Must acquire the interpreter lock during this operation */

    Status = AcpiUtAcquireMutex (ACPI_MTX_INTERPRETER);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Find the table in the global table list */

    for (i = 0; i < AcpiGbl_RootTableList.CurrentTableCount; i++)
    {
        if (OwnerId != AcpiGbl_RootTableList.Tables[i].OwnerId)
        {
            continue;
        }

        /*
         * Allow unload of SSDT and OEMx tables only. Do not allow unload
         * of the DSDT. No other types of tables should get here, since
         * only these types can contain AML and thus are the only types
         * that can create namespace objects.
         */
        if (ACPI_COMPARE_NAME (
            AcpiGbl_RootTableList.Tables[i].Signature.Ascii,
            ACPI_SIG_DSDT))
        {
            Status = AE_TYPE;
            break;
        }

        /* Ensure the table is actually loaded */

        if (!AcpiTbIsTableLoaded (i))
        {
            Status = AE_NOT_EXIST;
            break;
        }

        /* Invoke table handler if present */

        if (AcpiGbl_TableHandler)
        {
            (void) AcpiGbl_TableHandler (ACPI_TABLE_EVENT_UNLOAD,
                        AcpiGbl_RootTableList.Tables[i].Pointer,
                        AcpiGbl_TableHandlerContext);
        }

        /*
         * Delete all namespace objects owned by this table. Note that
         * these objects can appear anywhere in the namespace by virtue
         * of the AML "Scope" operator. Thus, we need to track ownership
         * by an ID, not simply a position within the hierarchy.
         */
        Status = AcpiTbDeleteNamespaceByOwner (i);
        if (ACPI_FAILURE (Status))
        {
            break;
        }

        Status = AcpiTbReleaseOwnerId (i);
        AcpiTbSetTableLoadedFlag (i, FALSE);
        break;
    }

    (void) AcpiUtReleaseMutex (ACPI_MTX_INTERPRETER);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiUnloadParentTable)
