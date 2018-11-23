/******************************************************************************
 *
 * Module Name: tbfind   - find table
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2018, Intel Corp.
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
        ACPI_MODULE_NAME    ("tbfind")


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbFindTable
 *
 * PARAMETERS:  Signature           - String with ACPI table signature
 *              OemId               - String with the table OEM ID
 *              OemTableId          - String with the OEM Table ID
 *              TableIndex          - Where the table index is returned
 *
 * RETURN:      Status and table index
 *
 * DESCRIPTION: Find an ACPI table (in the RSDT/XSDT) that matches the
 *              Signature, OEM ID and OEM Table ID. Returns an index that can
 *              be used to get the table header or entire table.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiTbFindTable (
    char                    *Signature,
    char                    *OemId,
    char                    *OemTableId,
    UINT32                  *TableIndex)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_TABLE_HEADER       Header;
    UINT32                  i;


    ACPI_FUNCTION_TRACE (TbFindTable);


    /* Validate the input table signature */

    if (!AcpiUtValidNameseg (Signature))
    {
        return_ACPI_STATUS (AE_BAD_SIGNATURE);
    }

    /* Don't allow the OEM strings to be too long */

    if ((strlen (OemId) > ACPI_OEM_ID_SIZE) ||
        (strlen (OemTableId) > ACPI_OEM_TABLE_ID_SIZE))
    {
        return_ACPI_STATUS (AE_AML_STRING_LIMIT);
    }

    /* Normalize the input strings */

    memset (&Header, 0, sizeof (ACPI_TABLE_HEADER));
    ACPI_MOVE_NAME (Header.Signature, Signature);
    strncpy (Header.OemId, OemId, ACPI_OEM_ID_SIZE);
    strncpy (Header.OemTableId, OemTableId, ACPI_OEM_TABLE_ID_SIZE);

    /* Search for the table */

    (void) AcpiUtAcquireMutex (ACPI_MTX_TABLES);
    for (i = 0; i < AcpiGbl_RootTableList.CurrentTableCount; ++i)
    {
        if (memcmp (&(AcpiGbl_RootTableList.Tables[i].Signature),
            Header.Signature, ACPI_NAME_SIZE))
        {
            /* Not the requested table */

            continue;
        }

        /* Table with matching signature has been found */

        if (!AcpiGbl_RootTableList.Tables[i].Pointer)
        {
            /* Table is not currently mapped, map it */

            Status = AcpiTbValidateTable (&AcpiGbl_RootTableList.Tables[i]);
            if (ACPI_FAILURE (Status))
            {
                goto UnlockAndExit;
            }

            if (!AcpiGbl_RootTableList.Tables[i].Pointer)
            {
                continue;
            }
        }

        /* Check for table match on all IDs */

        if (!memcmp (AcpiGbl_RootTableList.Tables[i].Pointer->Signature,
                Header.Signature, ACPI_NAME_SIZE) &&
            (!OemId[0] ||
             !memcmp (AcpiGbl_RootTableList.Tables[i].Pointer->OemId,
                 Header.OemId, ACPI_OEM_ID_SIZE)) &&
            (!OemTableId[0] ||
             !memcmp (AcpiGbl_RootTableList.Tables[i].Pointer->OemTableId,
                 Header.OemTableId, ACPI_OEM_TABLE_ID_SIZE)))
        {
            *TableIndex = i;

            ACPI_DEBUG_PRINT ((ACPI_DB_TABLES, "Found table [%4.4s]\n",
                Header.Signature));
            goto UnlockAndExit;
        }
    }
    Status = AE_NOT_FOUND;

UnlockAndExit:
    (void) AcpiUtReleaseMutex (ACPI_MTX_TABLES);
    return_ACPI_STATUS (Status);
}
