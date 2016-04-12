/*******************************************************************************
 *
 * Module Name: utnonansi - Non-ansi C library functions
 *
 ******************************************************************************/

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

#include "acpi.h"
#include "accommon.h"


#define _COMPONENT          ACPI_UTILITIES
        ACPI_MODULE_NAME    ("utnonansi")


/*
 * Non-ANSI C library functions - strlwr, strupr, stricmp, and a 64-bit
 * version of strtoul.
 */

/*******************************************************************************
 *
 * FUNCTION:    AcpiUtStrlwr (strlwr)
 *
 * PARAMETERS:  SrcString       - The source string to convert
 *
 * RETURN:      None
 *
 * DESCRIPTION: Convert a string to lowercase
 *
 ******************************************************************************/

void
AcpiUtStrlwr (
    char                    *SrcString)
{
    char                    *String;


    ACPI_FUNCTION_ENTRY ();


    if (!SrcString)
    {
        return;
    }

    /* Walk entire string, lowercasing the letters */

    for (String = SrcString; *String; String++)
    {
        *String = (char) tolower ((int) *String);
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtStrupr (strupr)
 *
 * PARAMETERS:  SrcString       - The source string to convert
 *
 * RETURN:      None
 *
 * DESCRIPTION: Convert a string to uppercase
 *
 ******************************************************************************/

void
AcpiUtStrupr (
    char                    *SrcString)
{
    char                    *String;


    ACPI_FUNCTION_ENTRY ();


    if (!SrcString)
    {
        return;
    }

    /* Walk entire string, uppercasing the letters */

    for (String = SrcString; *String; String++)
    {
        *String = (char) toupper ((int) *String);
    }
}


/******************************************************************************
 *
 * FUNCTION:    AcpiUtStricmp (stricmp)
 *
 * PARAMETERS:  String1             - first string to compare
 *              String2             - second string to compare
 *
 * RETURN:      int that signifies string relationship. Zero means strings
 *              are equal.
 *
 * DESCRIPTION: Case-insensitive string compare. Implementation of the
 *              non-ANSI stricmp function.
 *
 ******************************************************************************/

int
AcpiUtStricmp (
    char                    *String1,
    char                    *String2)
{
    int                     c1;
    int                     c2;


    do
    {
        c1 = tolower ((int) *String1);
        c2 = tolower ((int) *String2);

        String1++;
        String2++;
    }
    while ((c1 == c2) && (c1));

    return (c1 - c2);
}


#if defined (ACPI_DEBUGGER) || defined (ACPI_APPLICATION)
/*******************************************************************************
 *
 * FUNCTION:    AcpiUtSafeStrcpy, AcpiUtSafeStrcat, AcpiUtSafeStrncat
 *
 * PARAMETERS:  Adds a "DestSize" parameter to each of the standard string
 *              functions. This is the size of the Destination buffer.
 *
 * RETURN:      TRUE if the operation would overflow the destination buffer.
 *
 * DESCRIPTION: Safe versions of standard Clib string functions. Ensure that
 *              the result of the operation will not overflow the output string
 *              buffer.
 *
 * NOTE:        These functions are typically only helpful for processing
 *              user input and command lines. For most ACPICA code, the
 *              required buffer length is precisely calculated before buffer
 *              allocation, so the use of these functions is unnecessary.
 *
 ******************************************************************************/

BOOLEAN
AcpiUtSafeStrcpy (
    char                    *Dest,
    ACPI_SIZE               DestSize,
    char                    *Source)
{

    if (strlen (Source) >= DestSize)
    {
        return (TRUE);
    }

    strcpy (Dest, Source);
    return (FALSE);
}

BOOLEAN
AcpiUtSafeStrcat (
    char                    *Dest,
    ACPI_SIZE               DestSize,
    char                    *Source)
{

    if ((strlen (Dest) + strlen (Source)) >= DestSize)
    {
        return (TRUE);
    }

    strcat (Dest, Source);
    return (FALSE);
}

BOOLEAN
AcpiUtSafeStrncat (
    char                    *Dest,
    ACPI_SIZE               DestSize,
    char                    *Source,
    ACPI_SIZE               MaxTransferLength)
{
    ACPI_SIZE               ActualTransferLength;


    ActualTransferLength = ACPI_MIN (MaxTransferLength, strlen (Source));

    if ((strlen (Dest) + ActualTransferLength) >= DestSize)
    {
        return (TRUE);
    }

    strncat (Dest, Source, MaxTransferLength);
    return (FALSE);
}
#endif


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtStrtoul64
 *
 * PARAMETERS:  String                  - Null terminated string
 *              Base                    - Radix of the string: 16 or 10 or
 *                                        ACPI_ANY_BASE
 *              MaxIntegerByteWidth     - Maximum allowable integer,in bytes:
 *                                        4 or 8 (32 or 64 bits)
 *              RetInteger              - Where the converted integer is
 *                                        returned
 *
 * RETURN:      Status and Converted value
 *
 * DESCRIPTION: Convert a string into an unsigned value. Performs either a
 *              32-bit or 64-bit conversion, depending on the input integer
 *              size (often the current mode of the interpreter).
 *
 * NOTES:       Negative numbers are not supported, as they are not supported
 *              by ACPI.
 *
 *              AcpiGbl_IntegerByteWidth should be set to the proper width.
 *              For the core ACPICA code, this width depends on the DSDT
 *              version. For iASL, the default byte width is always 8 for the
 *              parser, but error checking is performed later to flag cases
 *              where a 64-bit constant is defined in a 32-bit DSDT/SSDT.
 *
 *              Does not support Octal strings, not needed at this time.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtStrtoul64 (
    char                    *String,
    UINT32                  Base,
    UINT32                  MaxIntegerByteWidth,
    UINT64                  *RetInteger)
{
    UINT32                  ThisDigit = 0;
    UINT64                  ReturnValue = 0;
    UINT64                  Quotient;
    UINT64                  Dividend;
    UINT8                   ValidDigits = 0;
    UINT8                   SignOf0x = 0;
    UINT8                   Term = 0;


    ACPI_FUNCTION_TRACE_STR (UtStrtoul64, String);


    switch (Base)
    {
    case ACPI_ANY_BASE:
    case 10:
    case 16:

        break;

    default:

        /* Invalid Base */

        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    if (!String)
    {
        goto ErrorExit;
    }

    /* Skip over any white space in the buffer */

    while ((*String) && (isspace ((int) *String) || *String == '\t'))
    {
        String++;
    }

    if (Base == ACPI_ANY_BASE)
    {
        /*
         * Base equal to ACPI_ANY_BASE means 'Either decimal or hex'.
         * We need to determine if it is decimal or hexadecimal.
         */
        if ((*String == '0') && (tolower ((int) *(String + 1)) == 'x'))
        {
            SignOf0x = 1;
            Base = 16;

            /* Skip over the leading '0x' */
            String += 2;
        }
        else
        {
            Base = 10;
        }
    }

    /* Any string left? Check that '0x' is not followed by white space. */

    if (!(*String) || isspace ((int) *String) || *String == '\t')
    {
        if (Base == ACPI_ANY_BASE)
        {
            goto ErrorExit;
        }
        else
        {
            goto AllDone;
        }
    }

    /*
     * Perform a 32-bit or 64-bit conversion, depending upon the input
     * byte width
     */
    Dividend = (MaxIntegerByteWidth <= ACPI_MAX32_BYTE_WIDTH) ?
        ACPI_UINT32_MAX : ACPI_UINT64_MAX;

    /* Main loop: convert the string to a 32- or 64-bit integer */

    while (*String)
    {
        if (isdigit ((int) *String))
        {
            /* Convert ASCII 0-9 to Decimal value */

            ThisDigit = ((UINT8) *String) - '0';
        }
        else if (Base == 10)
        {
            /* Digit is out of range; possible in ToInteger case only */

            Term = 1;
        }
        else
        {
            ThisDigit = (UINT8) toupper ((int) *String);
            if (isxdigit ((int) ThisDigit))
            {
                /* Convert ASCII Hex char to value */

                ThisDigit = ThisDigit - 'A' + 10;
            }
            else
            {
                Term = 1;
            }
        }

        if (Term)
        {
            if (Base == ACPI_ANY_BASE)
            {
                goto ErrorExit;
            }
            else
            {
                break;
            }
        }
        else if ((ValidDigits == 0) && (ThisDigit == 0) && !SignOf0x)
        {
            /* Skip zeros */
            String++;
            continue;
        }

        ValidDigits++;

        if (SignOf0x && ((ValidDigits > 16) ||
            ((ValidDigits > 8) && (MaxIntegerByteWidth <= ACPI_MAX32_BYTE_WIDTH))))
        {
            /*
             * This is ToInteger operation case.
             * No restrictions for string-to-integer conversion,
             * see ACPI spec.
             */
            goto ErrorExit;
        }

        /* Divide the digit into the correct position */

        (void) AcpiUtShortDivide (
            (Dividend - (UINT64) ThisDigit), Base, &Quotient, NULL);

        if (ReturnValue > Quotient)
        {
            if (Base == ACPI_ANY_BASE)
            {
                goto ErrorExit;
            }
            else
            {
                break;
            }
        }

        ReturnValue *= Base;
        ReturnValue += ThisDigit;
        String++;
    }

    /* All done, normal exit */

AllDone:

    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "Converted value: %8.8X%8.8X\n",
        ACPI_FORMAT_UINT64 (ReturnValue)));

    *RetInteger = ReturnValue;
    return_ACPI_STATUS (AE_OK);


ErrorExit:

    /* Base was set/validated above (10 or 16) */

    if (Base == 10)
    {
        return_ACPI_STATUS (AE_BAD_DECIMAL_CONSTANT);
    }
    else
    {
        return_ACPI_STATUS (AE_BAD_HEX_CONSTANT);
    }
}


#ifdef _OBSOLETE_FUNCTIONS
/* Removed: 01/2016 */

/*******************************************************************************
 *
 * FUNCTION:    strtoul64
 *
 * PARAMETERS:  String              - Null terminated string
 *              Terminater          - Where a pointer to the terminating byte
 *                                    is returned
 *              Base                - Radix of the string
 *
 * RETURN:      Converted value
 *
 * DESCRIPTION: Convert a string into an unsigned value.
 *
 ******************************************************************************/

ACPI_STATUS
strtoul64 (
    char                    *String,
    UINT32                  Base,
    UINT64                  *RetInteger)
{
    UINT32                  Index;
    UINT32                  Sign;
    UINT64                  ReturnValue = 0;
    ACPI_STATUS             Status = AE_OK;


    *RetInteger = 0;

    switch (Base)
    {
    case 0:
    case 8:
    case 10:
    case 16:

        break;

    default:
        /*
         * The specified Base parameter is not in the domain of
         * this function:
         */
        return (AE_BAD_PARAMETER);
    }

    /* Skip over any white space in the buffer: */

    while (isspace ((int) *String) || *String == '\t')
    {
        ++String;
    }

    /*
     * The buffer may contain an optional plus or minus sign.
     * If it does, then skip over it but remember what is was:
     */
    if (*String == '-')
    {
        Sign = ACPI_SIGN_NEGATIVE;
        ++String;
    }
    else if (*String == '+')
    {
        ++String;
        Sign = ACPI_SIGN_POSITIVE;
    }
    else
    {
        Sign = ACPI_SIGN_POSITIVE;
    }

    /*
     * If the input parameter Base is zero, then we need to
     * determine if it is octal, decimal, or hexadecimal:
     */
    if (Base == 0)
    {
        if (*String == '0')
        {
            if (tolower ((int) *(++String)) == 'x')
            {
                Base = 16;
                ++String;
            }
            else
            {
                Base = 8;
            }
        }
        else
        {
            Base = 10;
        }
    }

    /*
     * For octal and hexadecimal bases, skip over the leading
     * 0 or 0x, if they are present.
     */
    if (Base == 8 && *String == '0')
    {
        String++;
    }

    if (Base == 16 &&
        *String == '0' &&
        tolower ((int) *(++String)) == 'x')
    {
        String++;
    }

    /* Main loop: convert the string to an unsigned long */

    while (*String)
    {
        if (isdigit ((int) *String))
        {
            Index = ((UINT8) *String) - '0';
        }
        else
        {
            Index = (UINT8) toupper ((int) *String);
            if (isupper ((int) Index))
            {
                Index = Index - 'A' + 10;
            }
            else
            {
                goto ErrorExit;
            }
        }

        if (Index >= Base)
        {
            goto ErrorExit;
        }

        /* Check to see if value is out of range: */

        if (ReturnValue > ((ACPI_UINT64_MAX - (UINT64) Index) /
            (UINT64) Base))
        {
            goto ErrorExit;
        }
        else
        {
            ReturnValue *= Base;
            ReturnValue += Index;
        }

        ++String;
    }


    /* If a minus sign was present, then "the conversion is negated": */

    if (Sign == ACPI_SIGN_NEGATIVE)
    {
        ReturnValue = (ACPI_UINT32_MAX - ReturnValue) + 1;
    }

    *RetInteger = ReturnValue;
    return (Status);


ErrorExit:
    switch (Base)
    {
    case 8:

        Status = AE_BAD_OCTAL_CONSTANT;
        break;

    case 10:

        Status = AE_BAD_DECIMAL_CONSTANT;
        break;

    case 16:

        Status = AE_BAD_HEX_CONSTANT;
        break;

    default:

        /* Base validated above */

        break;
    }

    return (Status);
}
#endif
