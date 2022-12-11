/******************************************************************************
 *
 * Module Name: utcksum - Support generating table checksums
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
#include "acdisasm.h"
#include "acutils.h"


/* This module used for application-level code only */

#define _COMPONENT          ACPI_CA_DISASSEMBLER
        ACPI_MODULE_NAME    ("utcksum")


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtVerifyChecksum
 *
 * PARAMETERS:  Table               - ACPI table to verify
 *              Length              - Length of entire table
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Verifies that the table checksums to zero. Optionally returns
 *              exception on bad checksum.
 *              Note: We don't have to check for a CDAT here, since CDAT is 
 *              not in the RSDT/XSDT, and the CDAT table is never installed
 *              via ACPICA.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtVerifyChecksum (
    ACPI_TABLE_HEADER       *Table,
    UINT32                  Length)
{
    UINT8                   Checksum;


    /*
     * FACS/S3PT:
     * They are the odd tables, have no standard ACPI header and no checksum
     */
    if (ACPI_COMPARE_NAMESEG (Table->Signature, ACPI_SIG_S3PT) ||
        ACPI_COMPARE_NAMESEG (Table->Signature, ACPI_SIG_FACS))
    {
        return (AE_OK);
    }

    /* Compute the checksum on the table */

    Length = Table->Length;
    Checksum = AcpiUtGenerateChecksum (ACPI_CAST_PTR (UINT8, Table), Length, Table->Checksum);

    /* Computed checksum matches table? */

    if (Checksum != Table->Checksum)
    {
        ACPI_BIOS_WARNING ((AE_INFO,
            "Incorrect checksum in table [%4.4s] - 0x%2.2X, "
            "should be 0x%2.2X",
            Table->Signature, Table->Checksum,
            Table->Checksum - Checksum));

#if (ACPI_CHECKSUM_ABORT)
        return (AE_BAD_CHECKSUM);
#endif
    }

    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtVerifyCdatChecksum
 *
 * PARAMETERS:  Table               - CDAT ACPI table to verify
 *              Length              - Length of entire table
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Verifies that the CDAT table checksums to zero. Optionally
 *              returns an exception on bad checksum.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtVerifyCdatChecksum (
    ACPI_TABLE_CDAT         *CdatTable,
    UINT32                  Length)
{
    UINT8                   Checksum;


    /* Compute the checksum on the table */

    Checksum = AcpiUtGenerateChecksum (ACPI_CAST_PTR (UINT8, CdatTable),
                    CdatTable->Length, CdatTable->Checksum);

    /* Computed checksum matches table? */

    if (Checksum != CdatTable->Checksum)
    {
        ACPI_BIOS_WARNING ((AE_INFO,
            "Incorrect checksum in table [%4.4s] - 0x%2.2X, "
            "should be 0x%2.2X",
            AcpiGbl_CDAT, CdatTable->Checksum, Checksum));

#if (ACPI_CHECKSUM_ABORT)
        return (AE_BAD_CHECKSUM);
#endif
    }

    CdatTable->Checksum = Checksum;
    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtGenerateChecksum
 *
 * PARAMETERS:  Table               - Pointer to table to be checksummed
 *              Length              - Length of the table
 *              OriginalChecksum    - Value of the checksum field
 *
 * RETURN:      8 bit checksum of buffer
 *
 * DESCRIPTION: Computes an 8 bit checksum of the table.
 *
 ******************************************************************************/

UINT8
AcpiUtGenerateChecksum (
    void                    *Table,
    UINT32                  Length,
    UINT8                   OriginalChecksum)
{
    UINT8                   Checksum;


    /* Sum the entire table as-is */

    Checksum = AcpiUtChecksum ((UINT8 *) Table, Length);

    /* Subtract off the existing checksum value in the table */

    Checksum = (UINT8) (Checksum - OriginalChecksum);

    /* Compute and return the final checksum */

    Checksum = (UINT8) (0 - Checksum);
    return (Checksum);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtChecksum
 *
 * PARAMETERS:  Buffer          - Pointer to memory region to be checked
 *              Length          - Length of this memory region
 *
 * RETURN:      Checksum (UINT8)
 *
 * DESCRIPTION: Calculates circular checksum of memory region.
 *
 ******************************************************************************/

UINT8
AcpiUtChecksum (
    UINT8                   *Buffer,
    UINT32                  Length)
{
    UINT8                   Sum = 0;
    UINT8                   *End = Buffer + Length;


    while (Buffer < End)
    {
        Sum = (UINT8) (Sum + *(Buffer++));
    }

    return (Sum);
}
