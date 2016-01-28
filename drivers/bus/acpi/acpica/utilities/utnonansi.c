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


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtStrtoul64
 *
 * PARAMETERS:  String          - Null terminated string
 *              Base            - Radix of the string: 16 or ACPI_ANY_BASE;
 *                                ACPI_ANY_BASE means 'in behalf of ToInteger'
 *              RetInteger      - Where the converted integer is returned
 *
 * RETURN:      Status and Converted value
 *
 * DESCRIPTION: Convert a string into an unsigned value. Performs either a
 *              32-bit or 64-bit conversion, depending on the current mode
 *              of the interpreter.
 *
 * NOTE:        Does not support Octal strings, not needed.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiUtStrtoul64 (
    char                    *String,
    UINT32                  Base,
    UINT64                  *RetInteger)
{
    UINT32                  ThisDigit = 0;
    UINT64                  ReturnValue = 0;
    UINT64                  Quotient;
    UINT64                  Dividend;
    UINT32                  ToIntegerOp = (Base == ACPI_ANY_BASE);
    UINT32                  Mode32 = (AcpiGbl_IntegerByteWidth == 4);
    UINT8                   ValidDigits = 0;
    UINT8                   SignOf0x = 0;
    UINT8                   Term = 0;


    ACPI_FUNCTION_TRACE_STR (UtStroul64, String);


    switch (Base)
    {
    case ACPI_ANY_BASE:
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

    if (ToIntegerOp)
    {
        /*
         * Base equal to ACPI_ANY_BASE means 'ToInteger operation case'.
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
        if (ToIntegerOp)
        {
            goto ErrorExit;
        }
        else
        {
            goto AllDone;
        }
    }

    /*
     * Perform a 32-bit or 64-bit conversion, depending upon the current
     * execution mode of the interpreter
     */
    Dividend = (Mode32) ? ACPI_UINT32_MAX : ACPI_UINT64_MAX;

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
            if (ToIntegerOp)
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

        if (SignOf0x && ((ValidDigits > 16) || ((ValidDigits > 8) && Mode32)))
        {
            /*
             * This is ToInteger operation case.
             * No any restrictions for string-to-integer conversion,
             * see ACPI spec.
             */
            goto ErrorExit;
        }

        /* Divide the digit into the correct position */

        (void) AcpiUtShortDivide (
            (Dividend - (UINT64) ThisDigit), Base, &Quotient, NULL);

        if (ReturnValue > Quotient)
        {
            if (ToIntegerOp)
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
    /* Base was set/validated above */

    if (Base == 10)
    {
        return_ACPI_STATUS (AE_BAD_DECIMAL_CONSTANT);
    }
    else
    {
        return_ACPI_STATUS (AE_BAD_HEX_CONSTANT);
    }
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
