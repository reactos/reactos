/*******************************************************************************
 *
 * Module Name: utstrsuppt - Support functions for string-to-integer conversion
 *
 ******************************************************************************/

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

#define _COMPONENT          ACPI_UTILITIES
        ACPI_MODULE_NAME    ("utstrsuppt")


/* Local prototypes */

static ACPI_STATUS
AcpiUtInsertDigit (
    UINT64                  *AccumulatedValue,
    UINT32                  Base,
    int                     AsciiDigit);

static ACPI_STATUS
AcpiUtStrtoulMultiply64 (
    UINT64                  Multiplicand,
    UINT32                  Base,
    UINT64                  *OutProduct);

static ACPI_STATUS
AcpiUtStrtoulAdd64 (
    UINT64                  Addend1,
    UINT32                  Digit,
    UINT64                  *OutSum);


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtConvertOctalString
 *
 * PARAMETERS:  String                  - Null terminated input string
 *              ReturnValuePtr          - Where the converted value is returned
 *
 * RETURN:      Status and 64-bit converted integer
 *
 * DESCRIPTION: Performs a base 8 conversion of the input string to an
 *              integer value, either 32 or 64 bits.
 *
 * NOTE:        Maximum 64-bit unsigned octal value is 01777777777777777777777
 *              Maximum 32-bit unsigned octal value is 037777777777
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtConvertOctalString (
    char                    *String,
    UINT64                  *ReturnValuePtr)
{
    UINT64                  AccumulatedValue = 0;
    ACPI_STATUS             Status = AE_OK;


    /* Convert each ASCII byte in the input string */

    while (*String)
    {
        /* Character must be ASCII 0-7, otherwise terminate with no error */

        if (!(ACPI_IS_OCTAL_DIGIT (*String)))
        {
            break;
        }

        /* Convert and insert this octal digit into the accumulator */

        Status = AcpiUtInsertDigit (&AccumulatedValue, 8, *String);
        if (ACPI_FAILURE (Status))
        {
            Status = AE_OCTAL_OVERFLOW;
            break;
        }

        String++;
    }

    /* Always return the value that has been accumulated */

    *ReturnValuePtr = AccumulatedValue;
    return (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtConvertDecimalString
 *
 * PARAMETERS:  String                  - Null terminated input string
 *              ReturnValuePtr          - Where the converted value is returned
 *
 * RETURN:      Status and 64-bit converted integer
 *
 * DESCRIPTION: Performs a base 10 conversion of the input string to an
 *              integer value, either 32 or 64 bits.
 *
 * NOTE:        Maximum 64-bit unsigned decimal value is 18446744073709551615
 *              Maximum 32-bit unsigned decimal value is 4294967295
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtConvertDecimalString (
    char                    *String,
    UINT64                  *ReturnValuePtr)
{
    UINT64                  AccumulatedValue = 0;
    ACPI_STATUS             Status = AE_OK;


    /* Convert each ASCII byte in the input string */

    while (*String)
    {
        /* Character must be ASCII 0-9, otherwise terminate with no error */

        if (!isdigit (*String))
        {
           break;
        }

        /* Convert and insert this decimal digit into the accumulator */

        Status = AcpiUtInsertDigit (&AccumulatedValue, 10, *String);
        if (ACPI_FAILURE (Status))
        {
            Status = AE_DECIMAL_OVERFLOW;
            break;
        }

        String++;
    }

    /* Always return the value that has been accumulated */

    *ReturnValuePtr = AccumulatedValue;
    return (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtConvertHexString
 *
 * PARAMETERS:  String                  - Null terminated input string
 *              ReturnValuePtr          - Where the converted value is returned
 *
 * RETURN:      Status and 64-bit converted integer
 *
 * DESCRIPTION: Performs a base 16 conversion of the input string to an
 *              integer value, either 32 or 64 bits.
 *
 * NOTE:        Maximum 64-bit unsigned hex value is 0xFFFFFFFFFFFFFFFF
 *              Maximum 32-bit unsigned hex value is 0xFFFFFFFF
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtConvertHexString (
    char                    *String,
    UINT64                  *ReturnValuePtr)
{
    UINT64                  AccumulatedValue = 0;
    ACPI_STATUS             Status = AE_OK;


    /* Convert each ASCII byte in the input string */

    while (*String)
    {
        /* Must be ASCII A-F, a-f, or 0-9, otherwise terminate with no error */

        if (!isxdigit (*String))
        {
            break;
        }

        /* Convert and insert this hex digit into the accumulator */

        Status = AcpiUtInsertDigit (&AccumulatedValue, 16, *String);
        if (ACPI_FAILURE (Status))
        {
            Status = AE_HEX_OVERFLOW;
            break;
        }

        String++;
    }

    /* Always return the value that has been accumulated */

    *ReturnValuePtr = AccumulatedValue;
    return (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtRemoveLeadingZeros
 *
 * PARAMETERS:  String                  - Pointer to input ASCII string
 *
 * RETURN:      Next character after any leading zeros. This character may be
 *              used by the caller to detect end-of-string.
 *
 * DESCRIPTION: Remove any leading zeros in the input string. Return the
 *              next character after the final ASCII zero to enable the caller
 *              to check for the end of the string (NULL terminator).
 *
 ******************************************************************************/

char
AcpiUtRemoveLeadingZeros (
    char                    **String)
{

    while (**String == ACPI_ASCII_ZERO)
    {
        *String += 1;
    }

    return (**String);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtRemoveWhitespace
 *
 * PARAMETERS:  String                  - Pointer to input ASCII string
 *
 * RETURN:      Next character after any whitespace. This character may be
 *              used by the caller to detect end-of-string.
 *
 * DESCRIPTION: Remove any leading whitespace in the input string. Return the
 *              next character after the final ASCII zero to enable the caller
 *              to check for the end of the string (NULL terminator).
 *
 ******************************************************************************/

char
AcpiUtRemoveWhitespace (
    char                    **String)
{

    while (isspace ((UINT8) **String))
    {
        *String += 1;
    }

    return (**String);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtDetectHexPrefix
 *
 * PARAMETERS:  String                  - Pointer to input ASCII string
 *
 * RETURN:      TRUE if a "0x" prefix was found at the start of the string
 *
 * DESCRIPTION: Detect and remove a hex "0x" prefix
 *
 ******************************************************************************/

BOOLEAN
AcpiUtDetectHexPrefix (
    char                    **String)
{

    if ((**String == ACPI_ASCII_ZERO) &&
        (tolower ((int) *(*String + 1)) == 'x'))
    {
        *String += 2;        /* Go past the leading 0x */
        return (TRUE);
    }

    return (FALSE);     /* Not a hex string */
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtDetectOctalPrefix
 *
 * PARAMETERS:  String                  - Pointer to input ASCII string
 *
 * RETURN:      True if an octal "0" prefix was found at the start of the
 *              string
 *
 * DESCRIPTION: Detect and remove an octal prefix (zero)
 *
 ******************************************************************************/

BOOLEAN
AcpiUtDetectOctalPrefix (
    char                    **String)
{

    if (**String == ACPI_ASCII_ZERO)
    {
        *String += 1;       /* Go past the leading 0 */
        return (TRUE);
    }

    return (FALSE);     /* Not an octal string */
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtInsertDigit
 *
 * PARAMETERS:  AccumulatedValue        - Current value of the integer value
 *                                        accumulator. The new value is
 *                                        returned here.
 *              Base                    - Radix, either 8/10/16
 *              AsciiDigit              - ASCII single digit to be inserted
 *
 * RETURN:      Status and result of the convert/insert operation. The only
 *              possible returned exception code is numeric overflow of
 *              either the multiply or add conversion operations.
 *
 * DESCRIPTION: Generic conversion and insertion function for all bases:
 *
 *              1) Multiply the current accumulated/converted value by the
 *              base in order to make room for the new character.
 *
 *              2) Convert the new character to binary and add it to the
 *              current accumulated value.
 *
 *              Note: The only possible exception indicates an integer
 *              overflow (AE_NUMERIC_OVERFLOW)
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiUtInsertDigit (
    UINT64                  *AccumulatedValue,
    UINT32                  Base,
    int                     AsciiDigit)
{
    ACPI_STATUS             Status;
    UINT64                  Product;


     /* Make room in the accumulated value for the incoming digit */

    Status = AcpiUtStrtoulMultiply64 (*AccumulatedValue, Base, &Product);
    if (ACPI_FAILURE (Status))
    {
        return (Status);
    }

    /* Add in the new digit, and store the sum to the accumulated value */

    Status = AcpiUtStrtoulAdd64 (Product, AcpiUtAsciiCharToHex (AsciiDigit),
        AccumulatedValue);

    return (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtStrtoulMultiply64
 *
 * PARAMETERS:  Multiplicand            - Current accumulated converted integer
 *              Base                    - Base/Radix
 *              OutProduct              - Where the product is returned
 *
 * RETURN:      Status and 64-bit product
 *
 * DESCRIPTION: Multiply two 64-bit values, with checking for 64-bit overflow as
 *              well as 32-bit overflow if necessary (if the current global
 *              integer width is 32).
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiUtStrtoulMultiply64 (
    UINT64                  Multiplicand,
    UINT32                  Base,
    UINT64                  *OutProduct)
{
    UINT64                  Product;
    UINT64                  Quotient;


    /* Exit if either operand is zero */

    *OutProduct = 0;
    if (!Multiplicand || !Base)
    {
        return (AE_OK);
    }

    /*
     * Check for 64-bit overflow before the actual multiplication.
     *
     * Notes: 64-bit division is often not supported on 32-bit platforms
     * (it requires a library function), Therefore ACPICA has a local
     * 64-bit divide function. Also, Multiplier is currently only used
     * as the radix (8/10/16), to the 64/32 divide will always work.
     */
    AcpiUtShortDivide (ACPI_UINT64_MAX, Base, &Quotient, NULL);
    if (Multiplicand > Quotient)
    {
        return (AE_NUMERIC_OVERFLOW);
    }

    Product = Multiplicand * Base;

    /* Check for 32-bit overflow if necessary */

    if ((AcpiGbl_IntegerBitWidth == 32) && (Product > ACPI_UINT32_MAX))
    {
        return (AE_NUMERIC_OVERFLOW);
    }

    *OutProduct = Product;
    return (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtStrtoulAdd64
 *
 * PARAMETERS:  Addend1                 - Current accumulated converted integer
 *              Digit                   - New hex value/char
 *              OutSum                  - Where sum is returned (Accumulator)
 *
 * RETURN:      Status and 64-bit sum
 *
 * DESCRIPTION: Add two 64-bit values, with checking for 64-bit overflow as
 *              well as 32-bit overflow if necessary (if the current global
 *              integer width is 32).
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiUtStrtoulAdd64 (
    UINT64                  Addend1,
    UINT32                  Digit,
    UINT64                  *OutSum)
{
    UINT64                  Sum;


    /* Check for 64-bit overflow before the actual addition */

    if ((Addend1 > 0) && (Digit > (ACPI_UINT64_MAX - Addend1)))
    {
        return (AE_NUMERIC_OVERFLOW);
    }

    Sum = Addend1 + Digit;

    /* Check for 32-bit overflow if necessary */

    if ((AcpiGbl_IntegerBitWidth == 32) && (Sum > ACPI_UINT32_MAX))
    {
        return (AE_NUMERIC_OVERFLOW);
    }

    *OutSum = Sum;
    return (AE_OK);
}
