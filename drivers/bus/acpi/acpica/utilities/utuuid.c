/******************************************************************************
 *
 * Module Name: utuuid -- UUID support functions
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2021, Intel Corp.
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

#define _COMPONENT          ACPI_COMPILER
        ACPI_MODULE_NAME    ("utuuid")


#if (defined ACPI_ASL_COMPILER || defined ACPI_EXEC_APP || defined ACPI_HELP_APP)
/*
 * UUID support functions.
 *
 * This table is used to convert an input UUID ascii string to a 16 byte
 * buffer and the reverse. The table maps a UUID buffer index 0-15 to
 * the index within the 36-byte UUID string where the associated 2-byte
 * hex value can be found.
 *
 * 36-byte UUID strings are of the form:
 *     aabbccdd-eeff-gghh-iijj-kkllmmnnoopp
 * Where aa-pp are one byte hex numbers, made up of two hex digits
 *
 * Note: This table is basically the inverse of the string-to-offset table
 * found in the ACPI spec in the description of the ToUUID macro.
 */
const UINT8    AcpiGbl_MapToUuidOffset[UUID_BUFFER_LENGTH] =
{
    6,4,2,0,11,9,16,14,19,21,24,26,28,30,32,34
};


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtConvertStringToUuid
 *
 * PARAMETERS:  InString            - 36-byte formatted UUID string
 *              UuidBuffer          - Where the 16-byte UUID buffer is returned
 *
 * RETURN:      None. Output data is returned in the UuidBuffer
 *
 * DESCRIPTION: Convert a 36-byte formatted UUID string to 16-byte UUID buffer
 *
 ******************************************************************************/

void
AcpiUtConvertStringToUuid (
    char                    *InString,
    UINT8                   *UuidBuffer)
{
    UINT32                  i;


    for (i = 0; i < UUID_BUFFER_LENGTH; i++)
    {
        UuidBuffer[i] = (AcpiUtAsciiCharToHex (
            InString[AcpiGbl_MapToUuidOffset[i]]) << 4);

        UuidBuffer[i] |= AcpiUtAsciiCharToHex (
            InString[AcpiGbl_MapToUuidOffset[i] + 1]);
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtConvertUuidToString
 *
 * PARAMETERS:  UuidBuffer          - 16-byte UUID buffer
 *              OutString           - 36-byte formatted UUID string
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Convert 16-byte UUID buffer to 36-byte formatted UUID string
 *              OutString must be 37 bytes to include null terminator.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtConvertUuidToString (
    char                    *UuidBuffer,
    char                    *OutString)
{
    UINT32                  i;


    if (!UuidBuffer || !OutString)
    {
        return (AE_BAD_PARAMETER);
    }

    for (i = 0; i < UUID_BUFFER_LENGTH; i++)
    {
        OutString[AcpiGbl_MapToUuidOffset[i]] =
            AcpiUtHexToAsciiChar (UuidBuffer[i], 4);

        OutString[AcpiGbl_MapToUuidOffset[i] + 1] =
            AcpiUtHexToAsciiChar (UuidBuffer[i], 0);
    }

    /* Insert required hyphens (dashes) */

    OutString[UUID_HYPHEN1_OFFSET] =
    OutString[UUID_HYPHEN2_OFFSET] =
    OutString[UUID_HYPHEN3_OFFSET] =
    OutString[UUID_HYPHEN4_OFFSET] = '-';

    OutString[UUID_STRING_LENGTH] = 0; /* Null terminate */
    return (AE_OK);
}
#endif
