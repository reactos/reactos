/******************************************************************************
 *
 * Module Name: tbxfload - Table load/unload external interfaces
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

#define EXPORT_ACPI_INTERFACES

#include "acpi.h"
#include "accommon.h"
#include "acnamesp.h"
#include "actables.h"
#include "acevents.h"

#define _COMPONENT          ACPI_TABLES
        ACPI_MODULE_NAME    ("tbxfload")


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

ACPI_STATUS ACPI_INIT_FUNCTION
AcpiLoadTables (
    void)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiLoadTables);


    /*
     * Install the default operation region handlers. These are the
     * handlers that are defined by the ACPI specification to be
     * "always accessible" -- namely, SystemMemory, SystemIO, and
     * PCI_Config. This also means that no _REG methods need to be
     * run for these address spaces. We need to have these handlers
     * installed before any AML code can be executed, especially any
     * module-level code (11/2015).
     * Note that we allow OSPMs to install their own region handlers
     * between AcpiInitializeSubsystem() and AcpiLoadTables() to use
     * their customized default region handlers.
     */
    Status = AcpiEvInstallRegionHandlers ();
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "During Region initialization"));
        return_ACPI_STATUS (Status);
    }

    /* Load the namespace from the tables */

    Status = AcpiTbLoadNamespace ();

    /* Don't let single failures abort the load */

    if (Status == AE_CTRL_TERMINATE)
    {
        Status = AE_OK;
    }

    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status,
            "While loading namespace from ACPI tables"));
    }

    /*
     * Initialize the objects in the namespace that remain uninitialized.
     * This runs the executable AML that may be part of the declaration of
     * these name objects:
     *     OperationRegions, BufferFields, Buffers, and Packages.
     *
     */
    Status = AcpiNsInitializeObjects ();
    if (ACPI_SUCCESS (Status))
    {
        AcpiGbl_NamespaceInitialized = TRUE;
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

ACPI_STATUS
AcpiTbLoadNamespace (
    void)
{
    ACPI_STATUS             Status;
    UINT32                  i;
    ACPI_TABLE_HEADER       *NewDsdt;
    ACPI_TABLE_DESC         *Table;
    UINT32                  TablesLoaded = 0;
    UINT32                  TablesFailed = 0;


    ACPI_FUNCTION_TRACE (TbLoadNamespace);


    (void) AcpiUtAcquireMutex (ACPI_MTX_TABLES);

    /*
     * Load the namespace. The DSDT is required, but any SSDT and
     * PSDT tables are optional. Verify the DSDT.
     */
    Table = &AcpiGbl_RootTableList.Tables[AcpiGbl_DsdtIndex];

    if (!AcpiGbl_RootTableList.CurrentTableCount ||
        !ACPI_COMPARE_NAMESEG (Table->Signature.Ascii, ACPI_SIG_DSDT) ||
         ACPI_FAILURE (AcpiTbValidateTable (Table)))
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
    AcpiGbl_DSDT = Table->Pointer;

    /*
     * Optionally copy the entire DSDT to local memory (instead of simply
     * mapping it.) There are some BIOSs that corrupt or replace the original
     * DSDT, creating the need for this option. Default is FALSE, do not copy
     * the DSDT.
     */
    if (AcpiGbl_CopyDsdtLocally)
    {
        NewDsdt = AcpiTbCopyDsdt (AcpiGbl_DsdtIndex);
        if (NewDsdt)
        {
            AcpiGbl_DSDT = NewDsdt;
        }
    }

    /*
     * Save the original DSDT header for detection of table corruption
     * and/or replacement of the DSDT from outside the OS.
     */
    memcpy (&AcpiGbl_OriginalDsdtHeader, AcpiGbl_DSDT,
        sizeof (ACPI_TABLE_HEADER));

    /* Load and parse tables */

    (void) AcpiUtReleaseMutex (ACPI_MTX_TABLES);
    Status = AcpiNsLoadTable (AcpiGbl_DsdtIndex, AcpiGbl_RootNode);
    (void) AcpiUtAcquireMutex (ACPI_MTX_TABLES);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "[DSDT] table load failed"));
        TablesFailed++;
    }
    else
    {
        TablesLoaded++;
    }

    /* Load any SSDT or PSDT tables. Note: Loop leaves tables locked */

    for (i = 0; i < AcpiGbl_RootTableList.CurrentTableCount; ++i)
    {
        Table = &AcpiGbl_RootTableList.Tables[i];

        if (!Table->Address ||
            (!ACPI_COMPARE_NAMESEG (Table->Signature.Ascii, ACPI_SIG_SSDT) &&
             !ACPI_COMPARE_NAMESEG (Table->Signature.Ascii, ACPI_SIG_PSDT) &&
             !ACPI_COMPARE_NAMESEG (Table->Signature.Ascii, ACPI_SIG_OSDT)) ||
            ACPI_FAILURE (AcpiTbValidateTable (Table)))
        {
            continue;
        }

        /* Ignore errors while loading tables, get as many as possible */

        (void) AcpiUtReleaseMutex (ACPI_MTX_TABLES);
        Status =  AcpiNsLoadTable (i, AcpiGbl_RootNode);
        (void) AcpiUtAcquireMutex (ACPI_MTX_TABLES);
        if (ACPI_FAILURE (Status))
        {
            ACPI_EXCEPTION ((AE_INFO, Status, "(%4.4s:%8.8s) while loading table",
                Table->Signature.Ascii, Table->Pointer->OemTableId));

            TablesFailed++;

            ACPI_DEBUG_PRINT_RAW ((ACPI_DB_INIT,
                "Table [%4.4s:%8.8s] (id FF) - Table namespace load failed\n\n",
                Table->Signature.Ascii, Table->Pointer->OemTableId));
        }
        else
        {
            TablesLoaded++;
        }
    }

    if (!TablesFailed)
    {
        ACPI_INFO ((
            "%u ACPI AML tables successfully acquired and loaded",
            TablesLoaded));
    }
    else
    {
        ACPI_ERROR ((AE_INFO,
            "%u table load failures, %u successful",
            TablesFailed, TablesLoaded));

        /* Indicate at least one failure */

        Status = AE_CTRL_TERMINATE;
    }

#ifdef ACPI_APPLICATION
    ACPI_DEBUG_PRINT_RAW ((ACPI_DB_INIT, "\n"));
#endif


UnlockAndExit:
    (void) AcpiUtReleaseMutex (ACPI_MTX_TABLES);
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiInstallTable
 *
 * PARAMETERS:  Table               - Pointer to the ACPI table to be installed.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Dynamically install an ACPI table.
 *              Note: This function should only be invoked after
 *                    AcpiInitializeTables() and before AcpiLoadTables().
 *
 ******************************************************************************/

ACPI_STATUS ACPI_INIT_FUNCTION
AcpiInstallTable (
    ACPI_TABLE_HEADER       *Table)
{
    ACPI_STATUS             Status;
    UINT32                  TableIndex;


    ACPI_FUNCTION_TRACE (AcpiInstallTable);


    Status = AcpiTbInstallStandardTable (ACPI_PTR_TO_PHYSADDR (Table),
        ACPI_TABLE_ORIGIN_EXTERNAL_VIRTUAL, Table, FALSE, FALSE, &TableIndex);

    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL_INIT (AcpiInstallTable)


/*******************************************************************************
 *
 * FUNCTION:    AcpiInstallPhysicalTable
 *
 * PARAMETERS:  Address             - Address of the ACPI table to be installed.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Dynamically install an ACPI table.
 *              Note: This function should only be invoked after
 *                    AcpiInitializeTables() and before AcpiLoadTables().
 *
 ******************************************************************************/

ACPI_STATUS ACPI_INIT_FUNCTION
AcpiInstallPhysicalTable (
    ACPI_PHYSICAL_ADDRESS   Address)
{
    ACPI_STATUS             Status;
    UINT32                  TableIndex;


    ACPI_FUNCTION_TRACE (AcpiInstallPhysicalTable);


    Status = AcpiTbInstallStandardTable (Address,
        ACPI_TABLE_ORIGIN_INTERNAL_PHYSICAL, NULL, FALSE, FALSE, &TableIndex);

    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL_INIT (AcpiInstallPhysicalTable)


/*******************************************************************************
 *
 * FUNCTION:    AcpiLoadTable
 *
 * PARAMETERS:  Table               - Pointer to a buffer containing the ACPI
 *                                    table to be loaded.
 *              TableIdx            - Pointer to a UINT32 for storing the table
 *                                    index, might be NULL
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
    ACPI_TABLE_HEADER       *Table,
    UINT32                  *TableIdx)
{
    ACPI_STATUS             Status;
    UINT32                  TableIndex;


    ACPI_FUNCTION_TRACE (AcpiLoadTable);


    /* Parameter validation */

    if (!Table)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    /* Install the table and load it into the namespace */

    ACPI_INFO (("Host-directed Dynamic ACPI Table Load:"));
    Status = AcpiTbInstallAndLoadTable (ACPI_PTR_TO_PHYSADDR (Table),
        ACPI_TABLE_ORIGIN_EXTERNAL_VIRTUAL, Table, FALSE, &TableIndex);
    if (TableIdx)
    {
        *TableIdx = TableIndex;
    }

    if (ACPI_SUCCESS (Status))
    {
        /* Complete the initialization/resolution of new objects */

        AcpiNsInitializeObjects ();
    }

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

    /* Must acquire the table lock during this operation */

    Status = AcpiUtAcquireMutex (ACPI_MTX_TABLES);
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
        if (ACPI_COMPARE_NAMESEG (
                AcpiGbl_RootTableList.Tables[i].Signature.Ascii,
                ACPI_SIG_DSDT))
        {
            Status = AE_TYPE;
            break;
        }

        (void) AcpiUtReleaseMutex (ACPI_MTX_TABLES);
        Status = AcpiTbUnloadTable (i);
        (void) AcpiUtAcquireMutex (ACPI_MTX_TABLES);
        break;
    }

    (void) AcpiUtReleaseMutex (ACPI_MTX_TABLES);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiUnloadParentTable)


/*******************************************************************************
 *
 * FUNCTION:    AcpiUnloadTable
 *
 * PARAMETERS:  TableIndex          - Index as returned by AcpiLoadTable
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Via the TableIndex representing an SSDT or OEMx table, unloads
 *              the table and deletes all namespace objects associated with
 *              that table. Unloading of the DSDT is not allowed.
 *              Note: Mainly intended to support hotplug removal of SSDTs.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUnloadTable (
    UINT32                  TableIndex)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiUnloadTable);


    if (TableIndex == 1)
    {
        /* TableIndex==1 means DSDT is the owner. DSDT cannot be unloaded */

        return_ACPI_STATUS (AE_TYPE);
    }

    Status = AcpiTbUnloadTable (TableIndex);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiUnloadTable)
