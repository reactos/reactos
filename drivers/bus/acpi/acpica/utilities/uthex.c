/******************************************************************************
 *
 * Module Name: uthex -- Hex/ASCII support functions
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

#define _COMPONENT          ACPI_COMPILER
        ACPI_MODULE_NAME    ("uthex")


/* Hex to ASCII conversion table */

static const char           AcpiGbl_HexToAscii[] =
{
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'
};


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtHexToAsciiChar
 *
 * PARAMETERS:  Integer             - Contains the hex digit
 *              Position            - bit position of the digit within the
 *                                    integer (multiple of 4)
 *
 * RETURN:      The converted Ascii character
 *
 * DESCRIPTION: Convert a hex digit to an Ascii character
 *
 ******************************************************************************/

char
AcpiUtHexToAsciiChar (
    UINT64                  Integer,
    UINT32                  Position)
{
    UINT64                  Index;

    AcpiUtShortShiftRight (Integer, Position, &Index);
    return (AcpiGbl_HexToAscii[Index & 0xF]);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtAsciiToHexByte
 *
 * PARAMETERS:  TwoAsciiChars               - Pointer to two ASCII characters
 *              ReturnByte                  - Where converted byte is returned
 *
 * RETURN:      Status and converted hex byte
 *
 * DESCRIPTION: Perform ascii-to-hex translation, exactly two ASCII characters
 *              to a single converted byte value.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtAsciiToHexByte (
    char                    *TwoAsciiChars,
    UINT8                   *ReturnByte)
{

    /* Both ASCII characters must be valid hex digits */

    if (!isxdigit ((int) TwoAsciiChars[0]) ||
        !isxdigit ((int) TwoAsciiChars[1]))
    {
        return (AE_BAD_HEX_CONSTANT);
    }

    *ReturnByte =
        AcpiUtAsciiCharToHex (TwoAsciiChars[1]) |
        (AcpiUtAsciiCharToHex (TwoAsciiChars[0]) << 4);

    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtAsciiCharToHex
 *
 * PARAMETERS:  HexChar                 - Hex character in Ascii. Must be:
 *                                        0-9 or A-F or a-f
 *
 * RETURN:      The binary value of the ascii/hex character
 *
 * DESCRIPTION: Perform ascii-to-hex translation
 *
 ******************************************************************************/

UINT8
AcpiUtAsciiCharToHex (
    int                     HexChar)
{

    /* Values 0-9 */

    if (HexChar <= '9')
    {
        return ((UINT8) (HexChar - '0'));
    }

    /* Upper case A-F */

    if (HexChar <= 'F')
    {
        return ((UINT8) (HexChar - 0x37));
    }

    /* Lower case a-f */

    return ((UINT8) (HexChar - 0x57));
}
