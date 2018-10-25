/******************************************************************************
 *
 * Module Name: tbxfroot - Find the root ACPI table (RSDT)
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
        ACPI_MODULE_NAME    ("tbxfroot")


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbGetRsdpLength
 *
 * PARAMETERS:  Rsdp                - Pointer to RSDP
 *
 * RETURN:      Table length
 *
 * DESCRIPTION: Get the length of the RSDP
 *
 ******************************************************************************/

UINT32
AcpiTbGetRsdpLength (
    ACPI_TABLE_RSDP         *Rsdp)
{

    if (!ACPI_VALIDATE_RSDP_SIG (Rsdp->Signature))
    {
        /* BAD Signature */

        return (0);
    }

    /* "Length" field is available if table version >= 2 */

    if (Rsdp->Revision >= 2)
    {
        return (Rsdp->Length);
    }
    else
    {
        return (ACPI_RSDP_CHECKSUM_LENGTH);
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbValidateRsdp
 *
 * PARAMETERS:  Rsdp                - Pointer to unvalidated RSDP
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Validate the RSDP (ptr)
 *
 ******************************************************************************/

ACPI_STATUS
AcpiTbValidateRsdp (
    ACPI_TABLE_RSDP         *Rsdp)
{

    /*
     * The signature and checksum must both be correct
     *
     * Note: Sometimes there exists more than one RSDP in memory; the valid
     * RSDP has a valid checksum, all others have an invalid checksum.
     */
    if (!ACPI_VALIDATE_RSDP_SIG (Rsdp->Signature))
    {
        /* Nope, BAD Signature */

        return (AE_BAD_SIGNATURE);
    }

    /* Check the standard checksum */

    if (AcpiTbChecksum ((UINT8 *) Rsdp, ACPI_RSDP_CHECKSUM_LENGTH) != 0)
    {
        return (AE_BAD_CHECKSUM);
    }

    /* Check extended checksum if table version >= 2 */

    if ((Rsdp->Revision >= 2) &&
        (AcpiTbChecksum ((UINT8 *) Rsdp, ACPI_RSDP_XCHECKSUM_LENGTH) != 0))
    {
        return (AE_BAD_CHECKSUM);
    }

    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiFindRootPointer
 *
 * PARAMETERS:  TableAddress            - Where the table pointer is returned
 *
 * RETURN:      Status, RSDP physical address
 *
 * DESCRIPTION: Search lower 1Mbyte of memory for the root system descriptor
 *              pointer structure. If it is found, set *RSDP to point to it.
 *
 * NOTE1:       The RSDP must be either in the first 1K of the Extended
 *              BIOS Data Area or between E0000 and FFFFF (From ACPI Spec.)
 *              Only a 32-bit physical address is necessary.
 *
 * NOTE2:       This function is always available, regardless of the
 *              initialization state of the rest of ACPI.
 *
 ******************************************************************************/

ACPI_STATUS ACPI_INIT_FUNCTION
AcpiFindRootPointer (
    ACPI_PHYSICAL_ADDRESS   *TableAddress)
{
    UINT8                   *TablePtr;
    UINT8                   *MemRover;
    UINT32                  PhysicalAddress;


    ACPI_FUNCTION_TRACE (AcpiFindRootPointer);


    /* 1a) Get the location of the Extended BIOS Data Area (EBDA) */

    TablePtr = AcpiOsMapMemory (
        (ACPI_PHYSICAL_ADDRESS) ACPI_EBDA_PTR_LOCATION,
        ACPI_EBDA_PTR_LENGTH);
    if (!TablePtr)
    {
        ACPI_ERROR ((AE_INFO,
            "Could not map memory at 0x%8.8X for length %u",
            ACPI_EBDA_PTR_LOCATION, ACPI_EBDA_PTR_LENGTH));

        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    ACPI_MOVE_16_TO_32 (&PhysicalAddress, TablePtr);

    /* Convert segment part to physical address */

    PhysicalAddress <<= 4;
    AcpiOsUnmapMemory (TablePtr, ACPI_EBDA_PTR_LENGTH);

    /* EBDA present? */

    if (PhysicalAddress > 0x400)
    {
        /*
         * 1b) Search EBDA paragraphs (EBDA is required to be a
         *     minimum of 1K length)
         */
        TablePtr = AcpiOsMapMemory (
            (ACPI_PHYSICAL_ADDRESS) PhysicalAddress,
            ACPI_EBDA_WINDOW_SIZE);
        if (!TablePtr)
        {
            ACPI_ERROR ((AE_INFO,
                "Could not map memory at 0x%8.8X for length %u",
                PhysicalAddress, ACPI_EBDA_WINDOW_SIZE));

            return_ACPI_STATUS (AE_NO_MEMORY);
        }

        MemRover = AcpiTbScanMemoryForRsdp (
            TablePtr, ACPI_EBDA_WINDOW_SIZE);
        AcpiOsUnmapMemory (TablePtr, ACPI_EBDA_WINDOW_SIZE);

        if (MemRover)
        {
            /* Return the physical address */

            PhysicalAddress +=
                (UINT32) ACPI_PTR_DIFF (MemRover, TablePtr);

            *TableAddress = (ACPI_PHYSICAL_ADDRESS) PhysicalAddress;
            return_ACPI_STATUS (AE_OK);
        }
    }

    /*
     * 2) Search upper memory: 16-byte boundaries in E0000h-FFFFFh
     */
    TablePtr = AcpiOsMapMemory (
        (ACPI_PHYSICAL_ADDRESS) ACPI_HI_RSDP_WINDOW_BASE,
        ACPI_HI_RSDP_WINDOW_SIZE);

    if (!TablePtr)
    {
        ACPI_ERROR ((AE_INFO,
            "Could not map memory at 0x%8.8X for length %u",
            ACPI_HI_RSDP_WINDOW_BASE, ACPI_HI_RSDP_WINDOW_SIZE));

        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    MemRover = AcpiTbScanMemoryForRsdp (
        TablePtr, ACPI_HI_RSDP_WINDOW_SIZE);
    AcpiOsUnmapMemory (TablePtr, ACPI_HI_RSDP_WINDOW_SIZE);

    if (MemRover)
    {
        /* Return the physical address */

        PhysicalAddress = (UINT32)
            (ACPI_HI_RSDP_WINDOW_BASE + ACPI_PTR_DIFF (MemRover, TablePtr));

        *TableAddress = (ACPI_PHYSICAL_ADDRESS) PhysicalAddress;
        return_ACPI_STATUS (AE_OK);
    }

    /* A valid RSDP was not found */

    ACPI_BIOS_ERROR ((AE_INFO, "A valid RSDP was not found"));
    return_ACPI_STATUS (AE_NOT_FOUND);
}

ACPI_EXPORT_SYMBOL_INIT (AcpiFindRootPointer)


/*******************************************************************************
 *
 * FUNCTION:    AcpiTbScanMemoryForRsdp
 *
 * PARAMETERS:  StartAddress        - Starting pointer for search
 *              Length              - Maximum length to search
 *
 * RETURN:      Pointer to the RSDP if found, otherwise NULL.
 *
 * DESCRIPTION: Search a block of memory for the RSDP signature
 *
 ******************************************************************************/

UINT8 *
AcpiTbScanMemoryForRsdp (
    UINT8                   *StartAddress,
    UINT32                  Length)
{
    ACPI_STATUS             Status;
    UINT8                   *MemRover;
    UINT8                   *EndAddress;


    ACPI_FUNCTION_TRACE (TbScanMemoryForRsdp);


    EndAddress = StartAddress + Length;

    /* Search from given start address for the requested length */

    for (MemRover = StartAddress; MemRover < EndAddress;
         MemRover += ACPI_RSDP_SCAN_STEP)
    {
        /* The RSDP signature and checksum must both be correct */

        Status = AcpiTbValidateRsdp (
            ACPI_CAST_PTR (ACPI_TABLE_RSDP, MemRover));
        if (ACPI_SUCCESS (Status))
        {
            /* Sig and checksum valid, we have found a real RSDP */

            ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
                "RSDP located at physical address %p\n", MemRover));
            return_PTR (MemRover);
        }

        /* No sig match or bad checksum, keep searching */
    }

    /* Searched entire block, no RSDP was found */

    ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
        "Searched entire block from %p, valid RSDP was not found\n",
        StartAddress));
    return_PTR (NULL);
}
