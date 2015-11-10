/******************************************************************************
 *
 * Module Name: utprint - Formatted printing routines
 *
 *****************************************************************************/

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

#define _COMPONENT          ACPI_UTILITIES
        ACPI_MODULE_NAME    ("utprint")


#define ACPI_FORMAT_SIGN            0x01
#define ACPI_FORMAT_SIGN_PLUS       0x02
#define ACPI_FORMAT_SIGN_PLUS_SPACE 0x04
#define ACPI_FORMAT_ZERO            0x08
#define ACPI_FORMAT_LEFT            0x10
#define ACPI_FORMAT_UPPER           0x20
#define ACPI_FORMAT_PREFIX          0x40


/* Local prototypes */

static ACPI_SIZE
AcpiUtBoundStringLength (
    const char              *String,
    ACPI_SIZE               Count);

static char *
AcpiUtBoundStringOutput (
    char                    *String,
    const char              *End,
    char                    c);

static char *
AcpiUtFormatNumber (
    char                    *String,
    char                    *End,
    UINT64                  Number,
    UINT8                   Base,
    INT32                   Width,
    INT32                   Precision,
    UINT8                   Type);

static char *
AcpiUtPutNumber (
    char                    *String,
    UINT64                  Number,
    UINT8                   Base,
    BOOLEAN                 Upper);


/* Module globals */

static const char           AcpiGbl_LowerHexDigits[] = "0123456789abcdef";
static const char           AcpiGbl_UpperHexDigits[] = "0123456789ABCDEF";


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtBoundStringLength
 *
 * PARAMETERS:  String              - String with boundary
 *              Count               - Boundary of the string
 *
 * RETURN:      Length of the string. Less than or equal to Count.
 *
 * DESCRIPTION: Calculate the length of a string with boundary.
 *
 ******************************************************************************/

static ACPI_SIZE
AcpiUtBoundStringLength (
    const char              *String,
    ACPI_SIZE               Count)
{
    UINT32                  Length = 0;


    while (*String && Count)
    {
        Length++;
        String++;
        Count--;
    }

    return (Length);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtBoundStringOutput
 *
 * PARAMETERS:  String              - String with boundary
 *              End                 - Boundary of the string
 *              c                   - Character to be output to the string
 *
 * RETURN:      Updated position for next valid character
 *
 * DESCRIPTION: Output a character into a string with boundary check.
 *
 ******************************************************************************/

static char *
AcpiUtBoundStringOutput (
    char                    *String,
    const char              *End,
    char                    c)
{

    if (String < End)
    {
        *String = c;
    }

    ++String;
    return (String);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtPutNumber
 *
 * PARAMETERS:  String              - Buffer to hold reverse-ordered string
 *              Number              - Integer to be converted
 *              Base                - Base of the integer
 *              Upper               - Whether or not using upper cased digits
 *
 * RETURN:      Updated position for next valid character
 *
 * DESCRIPTION: Convert an integer into a string, note that, the string holds a
 *              reversed ordered number without the trailing zero.
 *
 ******************************************************************************/

static char *
AcpiUtPutNumber (
    char                    *String,
    UINT64                  Number,
    UINT8                   Base,
    BOOLEAN                 Upper)
{
    const char              *Digits;
    UINT64                  DigitIndex;
    char                    *Pos;


    Pos = String;
    Digits = Upper ? AcpiGbl_UpperHexDigits : AcpiGbl_LowerHexDigits;

    if (Number == 0)
    {
        *(Pos++) = '0';
    }
    else
    {
        while (Number)
        {
            (void) AcpiUtDivide (Number, Base, &Number, &DigitIndex);
            *(Pos++) = Digits[DigitIndex];
        }
    }

    /* *(Pos++) = '0'; */
    return (Pos);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtScanNumber
 *
 * PARAMETERS:  String              - String buffer
 *              NumberPtr           - Where the number is returned
 *
 * RETURN:      Updated position for next valid character
 *
 * DESCRIPTION: Scan a string for a decimal integer.
 *
 ******************************************************************************/

const char *
AcpiUtScanNumber (
    const char              *String,
    UINT64                  *NumberPtr)
{
    UINT64                  Number = 0;


    while (ACPI_IS_DIGIT (*String))
    {
        Number *= 10;
        Number += *(String++) - '0';
    }

    *NumberPtr = Number;
    return (String);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtPrintNumber
 *
 * PARAMETERS:  String              - String buffer
 *              Number              - The number to be converted
 *
 * RETURN:      Updated position for next valid character
 *
 * DESCRIPTION: Print a decimal integer into a string.
 *
 ******************************************************************************/

const char *
AcpiUtPrintNumber (
    char                    *String,
    UINT64                  Number)
{
    char                    AsciiString[20];
    const char              *Pos1;
    char                    *Pos2;


    Pos1 = AcpiUtPutNumber (AsciiString, Number, 10, FALSE);
    Pos2 = String;

    while (Pos1 != AsciiString)
    {
        *(Pos2++) = *(--Pos1);
    }

    *Pos2 = 0;
    return (String);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtFormatNumber
 *
 * PARAMETERS:  String              - String buffer with boundary
 *              End                 - Boundary of the string
 *              Number              - The number to be converted
 *              Base                - Base of the integer
 *              Width               - Field width
 *              Precision           - Precision of the integer
 *              Type                - Special printing flags
 *
 * RETURN:      Updated position for next valid character
 *
 * DESCRIPTION: Print an integer into a string with any base and any precision.
 *
 ******************************************************************************/

static char *
AcpiUtFormatNumber (
    char                    *String,
    char                    *End,
    UINT64                  Number,
    UINT8                   Base,
    INT32                   Width,
    INT32                   Precision,
    UINT8                   Type)
{
    char                    *Pos;
    char                    Sign;
    char                    Zero;
    BOOLEAN                 NeedPrefix;
    BOOLEAN                 Upper;
    INT32                   i;
    char                    ReversedString[66];


    /* Parameter validation */

    if (Base < 2 || Base > 16)
    {
        return (NULL);
    }

    if (Type & ACPI_FORMAT_LEFT)
    {
        Type &= ~ACPI_FORMAT_ZERO;
    }

    NeedPrefix = ((Type & ACPI_FORMAT_PREFIX) && Base != 10) ? TRUE : FALSE;
    Upper = (Type & ACPI_FORMAT_UPPER) ? TRUE : FALSE;
    Zero = (Type & ACPI_FORMAT_ZERO) ? '0' : ' ';

    /* Calculate size according to sign and prefix */

    Sign = '\0';
    if (Type & ACPI_FORMAT_SIGN)
    {
        if ((INT64) Number < 0)
        {
            Sign = '-';
            Number = - (INT64) Number;
            Width--;
        }
        else if (Type & ACPI_FORMAT_SIGN_PLUS)
        {
            Sign = '+';
            Width--;
        }
        else if (Type & ACPI_FORMAT_SIGN_PLUS_SPACE)
        {
            Sign = ' ';
            Width--;
        }
    }
    if (NeedPrefix)
    {
        Width--;
        if (Base == 16)
        {
            Width--;
        }
    }

    /* Generate full string in reverse order */

    Pos = AcpiUtPutNumber (ReversedString, Number, Base, Upper);
    i = ACPI_PTR_DIFF (Pos, ReversedString);

    /* Printing 100 using %2d gives "100", not "00" */

    if (i > Precision)
    {
        Precision = i;
    }

    Width -= Precision;

    /* Output the string */

    if (!(Type & (ACPI_FORMAT_ZERO | ACPI_FORMAT_LEFT)))
    {
        while (--Width >= 0)
        {
            String = AcpiUtBoundStringOutput (String, End, ' ');
        }
    }
    if (Sign)
    {
        String = AcpiUtBoundStringOutput (String, End, Sign);
    }
    if (NeedPrefix)
    {
        String = AcpiUtBoundStringOutput (String, End, '0');
        if (Base == 16)
        {
            String = AcpiUtBoundStringOutput (String, End,
                        Upper ? 'X' : 'x');
        }
    }
    if (!(Type & ACPI_FORMAT_LEFT))
    {
        while (--Width >= 0)
        {
            String = AcpiUtBoundStringOutput (String, End, Zero);
        }
    }

    while (i <= --Precision)
    {
        String = AcpiUtBoundStringOutput (String, End, '0');
    }
    while (--i >= 0)
    {
        String = AcpiUtBoundStringOutput (String, End,
                    ReversedString[i]);
    }
    while (--Width >= 0)
    {
        String = AcpiUtBoundStringOutput (String, End, ' ');
    }

    return (String);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtVsnprintf
 *
 * PARAMETERS:  String              - String with boundary
 *              Size                - Boundary of the string
 *              Format              - Standard printf format
 *              Args                - Argument list
 *
 * RETURN:      Number of bytes actually written.
 *
 * DESCRIPTION: Formatted output to a string using argument list pointer.
 *
 ******************************************************************************/

int
AcpiUtVsnprintf (
    char                    *String,
    ACPI_SIZE               Size,
    const char              *Format,
    va_list                 Args)
{
    UINT8                   Base;
    UINT8                   Type;
    INT32                   Width;
    INT32                   Precision;
    char                    Qualifier;
    UINT64                  Number;
    char                    *Pos;
    char                    *End;
    char                    c;
    const char              *s;
    const void              *p;
    INT32                   Length;
    int                     i;


    Pos = String;
    End = String + Size;

    for (; *Format; ++Format)
    {
        if (*Format != '%')
        {
            Pos = AcpiUtBoundStringOutput (Pos, End, *Format);
            continue;
        }

        Type = 0;
        Base = 10;

        /* Process sign */

        do
        {
            ++Format;
            if (*Format == '#')
            {
                Type |= ACPI_FORMAT_PREFIX;
            }
            else if (*Format == '0')
            {
                Type |= ACPI_FORMAT_ZERO;
            }
            else if (*Format == '+')
            {
                Type |= ACPI_FORMAT_SIGN_PLUS;
            }
            else if (*Format == ' ')
            {
                Type |= ACPI_FORMAT_SIGN_PLUS_SPACE;
            }
            else if (*Format == '-')
            {
                Type |= ACPI_FORMAT_LEFT;
            }
            else
            {
                break;
            }
        } while (1);

        /* Process width */

        Width = -1;
        if (ACPI_IS_DIGIT (*Format))
        {
            Format = AcpiUtScanNumber (Format, &Number);
            Width = (INT32) Number;
        }
        else if (*Format == '*')
        {
            ++Format;
            Width = va_arg (Args, int);
            if (Width < 0)
            {
                Width = -Width;
                Type |= ACPI_FORMAT_LEFT;
            }
        }

        /* Process precision */

        Precision = -1;
        if (*Format == '.')
        {
            ++Format;
            if (ACPI_IS_DIGIT(*Format))
            {
                Format = AcpiUtScanNumber (Format, &Number);
                Precision = (INT32) Number;
            }
            else if (*Format == '*')
            {
                ++Format;
                Precision = va_arg (Args, int);
            }
            if (Precision < 0)
            {
                Precision = 0;
            }
        }

        /* Process qualifier */

        Qualifier = -1;
        if (*Format == 'h' || *Format == 'l' || *Format == 'L')
        {
            Qualifier = *Format;
            ++Format;

            if (Qualifier == 'l' && *Format == 'l')
            {
                Qualifier = 'L';
                ++Format;
            }
        }

        switch (*Format)
        {
        case '%':

            Pos = AcpiUtBoundStringOutput (Pos, End, '%');
            continue;

        case 'c':

            if (!(Type & ACPI_FORMAT_LEFT))
            {
                while (--Width > 0)
                {
                    Pos = AcpiUtBoundStringOutput (Pos, End, ' ');
                }
            }

            c = (char) va_arg (Args, int);
            Pos = AcpiUtBoundStringOutput (Pos, End, c);

            while (--Width > 0)
            {
                Pos = AcpiUtBoundStringOutput (Pos, End, ' ');
            }
            continue;

        case 's':

            s = va_arg (Args, char *);
            if (!s)
            {
                s = "<NULL>";
            }
            Length = AcpiUtBoundStringLength (s, Precision);
            if (!(Type & ACPI_FORMAT_LEFT))
            {
                while (Length < Width--)
                {
                    Pos = AcpiUtBoundStringOutput (Pos, End, ' ');
                }
            }
            for (i = 0; i < Length; ++i)
            {
                Pos = AcpiUtBoundStringOutput (Pos, End, *s);
                ++s;
            }
            while (Length < Width--)
            {
                Pos = AcpiUtBoundStringOutput (Pos, End, ' ');
            }
            continue;

        case 'o':

            Base = 8;
            break;

        case 'X':

            Type |= ACPI_FORMAT_UPPER;

        case 'x':

            Base = 16;
            break;

        case 'd':
        case 'i':

            Type |= ACPI_FORMAT_SIGN;

        case 'u':

            break;

        case 'p':

            if (Width == -1)
            {
                Width = 2 * sizeof (void *);
                Type |= ACPI_FORMAT_ZERO;
            }

            p = va_arg (Args, void *);
            Pos = AcpiUtFormatNumber (Pos, End,
                    ACPI_TO_INTEGER (p), 16, Width, Precision, Type);
            continue;

        default:

            Pos = AcpiUtBoundStringOutput (Pos, End, '%');
            if (*Format)
            {
                Pos = AcpiUtBoundStringOutput (Pos, End, *Format);
            }
            else
            {
                --Format;
            }
            continue;
        }

        if (Qualifier == 'L')
        {
            Number = va_arg (Args, UINT64);
            if (Type & ACPI_FORMAT_SIGN)
            {
                Number = (INT64) Number;
            }
        }
        else if (Qualifier == 'l')
        {
            Number = va_arg (Args, unsigned long);
            if (Type & ACPI_FORMAT_SIGN)
            {
                Number = (INT32) Number;
            }
        }
        else if (Qualifier == 'h')
        {
            Number = (UINT16) va_arg (Args, int);
            if (Type & ACPI_FORMAT_SIGN)
            {
                Number = (INT16) Number;
            }
        }
        else
        {
            Number = va_arg (Args, unsigned int);
            if (Type & ACPI_FORMAT_SIGN)
            {
                Number = (signed int) Number;
            }
        }

        Pos = AcpiUtFormatNumber (Pos, End, Number, Base,
                Width, Precision, Type);
    }

    if (Size > 0)
    {
        if (Pos < End)
        {
            *Pos = '\0';
        }
        else
        {
            End[-1] = '\0';
        }
    }

    return (ACPI_PTR_DIFF (Pos, String));
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtSnprintf
 *
 * PARAMETERS:  String              - String with boundary
 *              Size                - Boundary of the string
 *              Format, ...         - Standard printf format
 *
 * RETURN:      Number of bytes actually written.
 *
 * DESCRIPTION: Formatted output to a string.
 *
 ******************************************************************************/

int
AcpiUtSnprintf (
    char                    *String,
    ACPI_SIZE               Size,
    const char              *Format,
    ...)
{
    va_list                 Args;
    int                     Length;


    va_start (Args, Format);
    Length = AcpiUtVsnprintf (String, Size, Format, Args);
    va_end (Args);

    return (Length);
}


#ifdef ACPI_APPLICATION
/*******************************************************************************
 *
 * FUNCTION:    AcpiUtFileVprintf
 *
 * PARAMETERS:  File                - File descriptor
 *              Format              - Standard printf format
 *              Args                - Argument list
 *
 * RETURN:      Number of bytes actually written.
 *
 * DESCRIPTION: Formatted output to a file using argument list pointer.
 *
 ******************************************************************************/

int
AcpiUtFileVprintf (
    ACPI_FILE               File,
    const char              *Format,
    va_list                 Args)
{
    ACPI_CPU_FLAGS          Flags;
    int                     Length;


    Flags = AcpiOsAcquireLock (AcpiGbl_PrintLock);
    Length = AcpiUtVsnprintf (AcpiGbl_PrintBuffer,
                sizeof (AcpiGbl_PrintBuffer), Format, Args);

    (void) AcpiOsWriteFile (File, AcpiGbl_PrintBuffer, Length, 1);
    AcpiOsReleaseLock (AcpiGbl_PrintLock, Flags);

    return (Length);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiUtFilePrintf
 *
 * PARAMETERS:  File                - File descriptor
 *              Format, ...         - Standard printf format
 *
 * RETURN:      Number of bytes actually written.
 *
 * DESCRIPTION: Formatted output to a file.
 *
 ******************************************************************************/

int
AcpiUtFilePrintf (
    ACPI_FILE               File,
    const char              *Format,
    ...)
{
    va_list                 Args;
    int                     Length;


    va_start (Args, Format);
    Length = AcpiUtFileVprintf (File, Format, Args);
    va_end (Args);

    return (Length);
}
#endif
