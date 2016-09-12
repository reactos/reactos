/******************************************************************************
 *
 * Module Name: tbxface - ACPI table-oriented external interfaces
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

#define EXPORT_ACPI_INTERFACES

#include "acpi.h"
#include "accommon.h"
#include "actables.h"

#define _COMPONENT          ACPI_TABLES
        ACPI_MODULE_NAME    ("tbxface")


/*******************************************************************************
 *
 * FUNCTION:    AcpiAllocateRootTable
 *
 * PARAMETERS:  InitialTableCount   - Size of InitialTableArray, in number of
 *                                    ACPI_TABLE_DESC structures
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Allocate a root table array. Used by iASL compiler and
 *              AcpiInitializeTables.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiAllocateRootTable (
    UINT32                  InitialTableCount)
{

    AcpiGbl_RootTableList.MaxTableCount = InitialTableCount;
    AcpiGbl_RootTableList.Flags = ACPI_ROOT_ALLOW_RESIZE;

    return (AcpiTbResizeRootTableList ());
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiInitializeTables
 *
 * PARAMETERS:  InitialTableArray   - Pointer to an array of pre-allocated
 *                                    ACPI_TABLE_DESC structures. If NULL, the
 *                                    array is dynamically allocated.
 *              InitialTableCount   - Size of InitialTableArray, in number of
 *                                    ACPI_TABLE_DESC structures
 *              AllowResize         - Flag to tell Table Manager if resize of
 *                                    pre-allocated array is allowed. Ignored
 *                                    if InitialTableArray is NULL.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Initialize the table manager, get the RSDP and RSDT/XSDT.
 *
 * NOTE:        Allows static allocation of the initial table array in order
 *              to avoid the use of dynamic memory in confined environments
 *              such as the kernel boot sequence where it may not be available.
 *
 *              If the host OS memory managers are initialized, use NULL for
 *              InitialTableArray, and the table will be dynamically allocated.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiInitializeTables (
    ACPI_TABLE_DESC         *InitialTableArray,
    UINT32                  InitialTableCount,
    BOOLEAN                 AllowResize)
{
    ACPI_PHYSICAL_ADDRESS   RsdpAddress;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiInitializeTables);


    /*
     * Setup the Root Table Array and allocate the table array
     * if requested
     */
    if (!InitialTableArray)
    {
        Status = AcpiAllocateRootTable (InitialTableCount);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }
    }
    else
    {
        /* Root Table Array has been statically allocated by the host */

        memset (InitialTableArray, 0,
            (ACPI_SIZE) InitialTableCount * sizeof (ACPI_TABLE_DESC));

        AcpiGbl_RootTableList.Tables = InitialTableArray;
        AcpiGbl_RootTableList.MaxTableCount = InitialTableCount;
        AcpiGbl_RootTableList.Flags = ACPI_ROOT_ORIGIN_UNKNOWN;
        if (AllowResize)
        {
            AcpiGbl_RootTableList.Flags |= ACPI_ROOT_ALLOW_RESIZE;
        }
    }

    /* Get the address of the RSDP */

    RsdpAddress = AcpiOsGetRootPointer ();
    if (!RsdpAddress)
    {
        return_ACPI_STATUS (AE_NOT_FOUND);
    }

    /*
     * Get the root table (RSDT or XSDT) and extract all entries to the local
     * Root Table Array. This array contains the information of the RSDT/XSDT
     * in a common, more useable format.
     */
    Status = AcpiTbParseRootTable (RsdpAddress);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL_INIT (AcpiInitializeTables)


/*******************************************************************************
 *
 * FUNCTION:    AcpiReallocateRootTable
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Reallocate Root Table List into dynamic memory. Copies the
 *              root list from the previously provided scratch area. Should
 *              be called once dynamic memory allocation is available in the
 *              kernel.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiReallocateRootTable (
    void)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiReallocateRootTable);


    /*
     * Only reallocate the root table if the host provided a static buffer
     * for the table array in the call to AcpiInitializeTables.
     */
    if (AcpiGbl_RootTableList.Flags & ACPI_ROOT_ORIGIN_ALLOCATED)
    {
        return_ACPI_STATUS (AE_SUPPORT);
    }

    AcpiGbl_RootTableList.Flags |= ACPI_ROOT_ALLOW_RESIZE;

    Status = AcpiTbResizeRootTableList ();
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL_INIT (AcpiReallocateRootTable)


/*******************************************************************************
 *
 * FUNCTION:    AcpiGetTableHeader
 *
 * PARAMETERS:  Signature           - ACPI signature of needed table
 *              Instance            - Which instance (for SSDTs)
 *              OutTableHeader      - The pointer to the table header to fill
 *
 * RETURN:      Status and pointer to mapped table header
 *
 * DESCRIPTION: Finds an ACPI table header.
 *
 * NOTE:        Caller is responsible in unmapping the header with
 *              AcpiOsUnmapMemory
 *
 ******************************************************************************/

ACPI_STATUS
AcpiGetTableHeader (
    char                    *Signature,
    UINT32                  Instance,
    ACPI_TABLE_HEADER       *OutTableHeader)
{
    UINT32                  i;
    UINT32                  j;
    ACPI_TABLE_HEADER       *Header;


    /* Parameter validation */

    if (!Signature || !OutTableHeader)
    {
        return (AE_BAD_PARAMETER);
    }

    /* Walk the root table list */

    for (i = 0, j = 0; i < AcpiGbl_RootTableList.CurrentTableCount; i++)
    {
        if (!ACPI_COMPARE_NAME (
                &(AcpiGbl_RootTableList.Tables[i].Signature), Signature))
        {
            continue;
        }

        if (++j < Instance)
        {
            continue;
        }

        if (!AcpiGbl_RootTableList.Tables[i].Pointer)
        {
            if ((AcpiGbl_RootTableList.Tables[i].Flags &
                    ACPI_TABLE_ORIGIN_MASK) ==
                ACPI_TABLE_ORIGIN_INTERNAL_PHYSICAL)
            {
                Header = AcpiOsMapMemory (
                    AcpiGbl_RootTableList.Tables[i].Address,
                    sizeof (ACPI_TABLE_HEADER));
                if (!Header)
                {
                    return (AE_NO_MEMORY);
                }

                memcpy (OutTableHeader, Header, sizeof (ACPI_TABLE_HEADER));
                AcpiOsUnmapMemory (Header, sizeof (ACPI_TABLE_HEADER));
            }
            else
            {
                return (AE_NOT_FOUND);
            }
        }
        else
        {
            memcpy (OutTableHeader,
                AcpiGbl_RootTableList.Tables[i].Pointer,
                sizeof (ACPI_TABLE_HEADER));
        }

        return (AE_OK);
    }

    return (AE_NOT_FOUND);
}

ACPI_EXPORT_SYMBOL (AcpiGetTableHeader)


/*******************************************************************************
 *
 * FUNCTION:    AcpiGetTable
 *
 * PARAMETERS:  Signature           - ACPI signature of needed table
 *              Instance            - Which instance (for SSDTs)
 *              OutTable            - Where the pointer to the table is returned
 *
 * RETURN:      Status and pointer to the requested table
 *
 * DESCRIPTION: Finds and verifies an ACPI table. Table must be in the
 *              RSDT/XSDT.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiGetTable (
    char                    *Signature,
    UINT32                  Instance,
    ACPI_TABLE_HEADER       **OutTable)
{
    UINT32                  i;
    UINT32                  j;
    ACPI_STATUS             Status;


    /* Parameter validation */

    if (!Signature || !OutTable)
    {
        return (AE_BAD_PARAMETER);
    }

    /* Walk the root table list */

    for (i = 0, j = 0; i < AcpiGbl_RootTableList.CurrentTableCount; i++)
    {
        if (!ACPI_COMPARE_NAME (
                &(AcpiGbl_RootTableList.Tables[i].Signature), Signature))
        {
            continue;
        }

        if (++j < Instance)
        {
            continue;
        }

        Status = AcpiTbValidateTable (&AcpiGbl_RootTableList.Tables[i]);
        if (ACPI_SUCCESS (Status))
        {
            *OutTable = AcpiGbl_RootTableList.Tables[i].Pointer;
        }

        return (Status);
    }

    return (AE_NOT_FOUND);
}

ACPI_EXPORT_SYMBOL (AcpiGetTable)


/*******************************************************************************
 *
 * FUNCTION:    AcpiGetTableByIndex
 *
 * PARAMETERS:  TableIndex          - Table index
 *              Table               - Where the pointer to the table is returned
 *
 * RETURN:      Status and pointer to the requested table
 *
 * DESCRIPTION: Obtain a table by an index into the global table list. Used
 *              internally also.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiGetTableByIndex (
    UINT32                  TableIndex,
    ACPI_TABLE_HEADER       **Table)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiGetTableByIndex);


    /* Parameter validation */

    if (!Table)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    (void) AcpiUtAcquireMutex (ACPI_MTX_TABLES);

    /* Validate index */

    if (TableIndex >= AcpiGbl_RootTableList.CurrentTableCount)
    {
        (void) AcpiUtReleaseMutex (ACPI_MTX_TABLES);
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    if (!AcpiGbl_RootTableList.Tables[TableIndex].Pointer)
    {
        /* Table is not mapped, map it */

        Status = AcpiTbValidateTable (
            &AcpiGbl_RootTableList.Tables[TableIndex]);
        if (ACPI_FAILURE (Status))
        {
            (void) AcpiUtReleaseMutex (ACPI_MTX_TABLES);
            return_ACPI_STATUS (Status);
        }
    }

    *Table = AcpiGbl_RootTableList.Tables[TableIndex].Pointer;
    (void) AcpiUtReleaseMutex (ACPI_MTX_TABLES);
    return_ACPI_STATUS (AE_OK);
}

ACPI_EXPORT_SYMBOL (AcpiGetTableByIndex)


/*******************************************************************************
 *
 * FUNCTION:    AcpiInstallTableHandler
 *
 * PARAMETERS:  Handler         - Table event handler
 *              Context         - Value passed to the handler on each event
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Install a global table event handler.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiInstallTableHandler (
    ACPI_TABLE_HANDLER      Handler,
    void                    *Context)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiInstallTableHandler);


    if (!Handler)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    Status = AcpiUtAcquireMutex (ACPI_MTX_EVENTS);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Don't allow more than one handler */

    if (AcpiGbl_TableHandler)
    {
        Status = AE_ALREADY_EXISTS;
        goto Cleanup;
    }

    /* Install the handler */

    AcpiGbl_TableHandler = Handler;
    AcpiGbl_TableHandlerContext = Context;

Cleanup:
    (void) AcpiUtReleaseMutex (ACPI_MTX_EVENTS);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiInstallTableHandler)


/*******************************************************************************
 *
 * FUNCTION:    AcpiRemoveTableHandler
 *
 * PARAMETERS:  Handler         - Table event handler that was installed
 *                                previously.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Remove a table event handler
 *
 ******************************************************************************/

ACPI_STATUS
AcpiRemoveTableHandler (
    ACPI_TABLE_HANDLER      Handler)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiRemoveTableHandler);


    Status = AcpiUtAcquireMutex (ACPI_MTX_EVENTS);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Make sure that the installed handler is the same */

    if (!Handler ||
        Handler != AcpiGbl_TableHandler)
    {
        Status = AE_BAD_PARAMETER;
        goto Cleanup;
    }

    /* Remove the handler */

    AcpiGbl_TableHandler = NULL;

Cleanup:
    (void) AcpiUtReleaseMutex (ACPI_MTX_EVENTS);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiRemoveTableHandler)
