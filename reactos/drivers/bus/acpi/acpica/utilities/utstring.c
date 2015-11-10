/*******************************************************************************
 *
 * Module Name: utstring - Common functions for strings and characters
 *
 ******************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2015, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights. You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code. No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision. In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change. Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee. Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution. In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE. ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT, ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES. INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS. INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES. THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government. In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/

#include "acpi.h"
#include "accommon.h"
#include "acnamesp.h"


#define _COMPONENT          ACPI_UTILITIES
        ACPI_MODULE_NAME    ("utstring")


/*
 * Non-ANSI C library functions - strlwr, strupr, stricmp, and a 64-bit
 * version of strtoul.
 */

#ifdef ACPI_ASL_COMPILER
/*******************************************************************************
 *
 * FUNCTION:    AcpiUtStrlwr (strlwr)
 *
 * PARAMETERS:  SrcString       - The source string to convert
 *
 * RETURN:      None
 *
 * DESCRIPTION: Convert string to lowercase
 *
 * NOTE: This is not a POSIX function, so it appears here, not in utclib.c
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
        *String = (char) ACPI_TOLOWER (*String);
    }

    return;
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
 * DESCRIPTION: Implementation of the non-ANSI stricmp function (compare
 *              strings with no case sensitivity)
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
#endif


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtStrupr (strupr)
 *
 * PARAMETERS:  SrcString       - The source string to convert
 *
 * RETURN:      None
 *
 * DESCRIPTION: Convert string to uppercase
 *
 * NOTE: This is not a POSIX function, so it appears here, not in utclib.c
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
        *String = (char) ACPI_TOUPPER (*String);
    }

    return;
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
 *              NOTE: Does not support Octal strings, not needed.
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

    while ((*String) && (ACPI_IS_SPACE (*String) || *String == '\t'))
    {
        String++;
    }

    if (ToIntegerOp)
    {
        /*
         * Base equal to ACPI_ANY_BASE means 'ToInteger operation case'.
         * We need to determine if it is decimal or hexadecimal.
         */
        if ((*String == '0') && (ACPI_TOLOWER (*(String + 1)) == 'x'))
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

    if (!(*String) || ACPI_IS_SPACE (*String) || *String == '\t')
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
        if (ACPI_IS_DIGIT (*String))
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
            ThisDigit = (UINT8) ACPI_TOUPPER (*String);
            if (ACPI_IS_XDIGIT ((char) ThisDigit))
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

        (void) AcpiUtShortDivide ((Dividend - (UINT64) ThisDigit),
                    Base, &Quotient, NULL);

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


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtPrintString
 *
 * PARAMETERS:  String          - Null terminated ASCII string
 *              MaxLength       - Maximum output length. Used to constrain the
 *                                length of strings during debug output only.
 *
 * RETURN:      None
 *
 * DESCRIPTION: Dump an ASCII string with support for ACPI-defined escape
 *              sequences.
 *
 ******************************************************************************/

void
AcpiUtPrintString (
    char                    *String,
    UINT16                  MaxLength)
{
    UINT32                  i;


    if (!String)
    {
        AcpiOsPrintf ("<\"NULL STRING PTR\">");
        return;
    }

    AcpiOsPrintf ("\"");
    for (i = 0; (i < MaxLength) && String[i]; i++)
    {
        /* Escape sequences */

        switch (String[i])
        {
        case 0x07:

            AcpiOsPrintf ("\\a");       /* BELL */
            break;

        case 0x08:

            AcpiOsPrintf ("\\b");       /* BACKSPACE */
            break;

        case 0x0C:

            AcpiOsPrintf ("\\f");       /* FORMFEED */
            break;

        case 0x0A:

            AcpiOsPrintf ("\\n");       /* LINEFEED */
            break;

        case 0x0D:

            AcpiOsPrintf ("\\r");       /* CARRIAGE RETURN*/
            break;

        case 0x09:

            AcpiOsPrintf ("\\t");       /* HORIZONTAL TAB */
            break;

        case 0x0B:

            AcpiOsPrintf ("\\v");       /* VERTICAL TAB */
            break;

        case '\'':                      /* Single Quote */
        case '\"':                      /* Double Quote */
        case '\\':                      /* Backslash */

            AcpiOsPrintf ("\\%c", (int) String[i]);
            break;

        default:

            /* Check for printable character or hex escape */

            if (ACPI_IS_PRINT (String[i]))
            {
                /* This is a normal character */

                AcpiOsPrintf ("%c", (int) String[i]);
            }
            else
            {
                /* All others will be Hex escapes */

                AcpiOsPrintf ("\\x%2.2X", (INT32) String[i]);
            }
            break;
        }
    }
    AcpiOsPrintf ("\"");

    if (i == MaxLength && String[i])
    {
        AcpiOsPrintf ("...");
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtValidAcpiChar
 *
 * PARAMETERS:  Char            - The character to be examined
 *              Position        - Byte position (0-3)
 *
 * RETURN:      TRUE if the character is valid, FALSE otherwise
 *
 * DESCRIPTION: Check for a valid ACPI character. Must be one of:
 *              1) Upper case alpha
 *              2) numeric
 *              3) underscore
 *
 *              We allow a '!' as the last character because of the ASF! table
 *
 ******************************************************************************/

BOOLEAN
AcpiUtValidAcpiChar (
    char                    Character,
    UINT32                  Position)
{

    if (!((Character >= 'A' && Character <= 'Z') ||
          (Character >= '0' && Character <= '9') ||
          (Character == '_')))
    {
        /* Allow a '!' in the last position */

        if (Character == '!' && Position == 3)
        {
            return (TRUE);
        }

        return (FALSE);
    }

    return (TRUE);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtValidAcpiName
 *
 * PARAMETERS:  Name            - The name to be examined. Does not have to
 *                                be NULL terminated string.
 *
 * RETURN:      TRUE if the name is valid, FALSE otherwise
 *
 * DESCRIPTION: Check for a valid ACPI name. Each character must be one of:
 *              1) Upper case alpha
 *              2) numeric
 *              3) underscore
 *
 ******************************************************************************/

BOOLEAN
AcpiUtValidAcpiName (
    char                    *Name)
{
    UINT32                  i;


    ACPI_FUNCTION_ENTRY ();


    for (i = 0; i < ACPI_NAME_SIZE; i++)
    {
        if (!AcpiUtValidAcpiChar (Name[i], i))
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtRepairName
 *
 * PARAMETERS:  Name            - The ACPI name to be repaired
 *
 * RETURN:      Repaired version of the name
 *
 * DESCRIPTION: Repair an ACPI name: Change invalid characters to '*' and
 *              return the new name. NOTE: the Name parameter must reside in
 *              read/write memory, cannot be a const.
 *
 * An ACPI Name must consist of valid ACPI characters. We will repair the name
 * if necessary because we don't want to abort because of this, but we want
 * all namespace names to be printable. A warning message is appropriate.
 *
 * This issue came up because there are in fact machines that exhibit
 * this problem, and we want to be able to enable ACPI support for them,
 * even though there are a few bad names.
 *
 ******************************************************************************/

void
AcpiUtRepairName (
    char                    *Name)
{
    UINT32                  i;
    BOOLEAN                 FoundBadChar = FALSE;
    UINT32                  OriginalName;


    ACPI_FUNCTION_NAME (UtRepairName);


    ACPI_MOVE_NAME (&OriginalName, Name);

    /* Check each character in the name */

    for (i = 0; i < ACPI_NAME_SIZE; i++)
    {
        if (AcpiUtValidAcpiChar (Name[i], i))
        {
            continue;
        }

        /*
         * Replace a bad character with something printable, yet technically
         * still invalid. This prevents any collisions with existing "good"
         * names in the namespace.
         */
        Name[i] = '*';
        FoundBadChar = TRUE;
    }

    if (FoundBadChar)
    {
        /* Report warning only if in strict mode or debug mode */

        if (!AcpiGbl_EnableInterpreterSlack)
        {
            ACPI_WARNING ((AE_INFO,
                "Invalid character(s) in name (0x%.8X), repaired: [%4.4s]",
                OriginalName, Name));
        }
        else
        {
            ACPI_DEBUG_PRINT ((ACPI_DB_INFO,
                "Invalid character(s) in name (0x%.8X), repaired: [%4.4s]",
                OriginalName, Name));
        }
    }
}


#if defined ACPI_ASL_COMPILER || defined ACPI_EXEC_APP
/*******************************************************************************
 *
 * FUNCTION:    UtConvertBackslashes
 *
 * PARAMETERS:  Pathname        - File pathname string to be converted
 *
 * RETURN:      Modifies the input Pathname
 *
 * DESCRIPTION: Convert all backslashes (0x5C) to forward slashes (0x2F) within
 *              the entire input file pathname string.
 *
 ******************************************************************************/

void
UtConvertBackslashes (
    char                    *Pathname)
{

    if (!Pathname)
    {
        return;
    }

    while (*Pathname)
    {
        if (*Pathname == '\\')
        {
            *Pathname = '/';
        }

        Pathname++;
    }
}
#endif

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

    if (ACPI_STRLEN (Source) >= DestSize)
    {
        return (TRUE);
    }

    ACPI_STRCPY (Dest, Source);
    return (FALSE);
}

BOOLEAN
AcpiUtSafeStrcat (
    char                    *Dest,
    ACPI_SIZE               DestSize,
    char                    *Source)
{

    if ((ACPI_STRLEN (Dest) + ACPI_STRLEN (Source)) >= DestSize)
    {
        return (TRUE);
    }

    ACPI_STRCAT (Dest, Source);
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


    ActualTransferLength = ACPI_MIN (MaxTransferLength, ACPI_STRLEN (Source));

    if ((ACPI_STRLEN (Dest) + ActualTransferLength) >= DestSize)
    {
        return (TRUE);
    }

    ACPI_STRNCAT (Dest, Source, MaxTransferLength);
    return (FALSE);
}
#endif
