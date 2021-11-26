/******************************************************************************
 *
 * Module Name: utprint - Formatted printing routines
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


    while (isdigit ((int) *String))
    {
        AcpiUtShortMultiply (Number, 10, &Number);
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
    i = (INT32) ACPI_PTR_DIFF (Pos, ReversedString);

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
            String = AcpiUtBoundStringOutput (
                String, End, Upper ? 'X' : 'x');
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
 * FUNCTION:    vsnprintf
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
vsnprintf (
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


    if (Size != ACPI_UINT32_MAX) {
        End = String + Size;
    } else {
        End = ACPI_CAST_PTR(char, ACPI_UINT32_MAX);
    }

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
        if (isdigit ((int) *Format))
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
            if (isdigit ((int) *Format))
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
            Length = (INT32) AcpiUtBoundStringLength (s, Precision);
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
            ACPI_FALLTHROUGH;

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
            Pos = AcpiUtFormatNumber (
                Pos, End, ACPI_TO_INTEGER (p), 16, Width, Precision, Type);
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

    return ((int) ACPI_PTR_DIFF (Pos, String));
}


/*******************************************************************************
 *
 * FUNCTION:    snprintf
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
snprintf (
    char                    *String,
    ACPI_SIZE               Size,
    const char              *Format,
    ...)
{
    va_list                 Args;
    int                     Length;


    va_start (Args, Format);
    Length = vsnprintf (String, Size, Format, Args);
    va_end (Args);

    return (Length);
}


/*******************************************************************************
 *
 * FUNCTION:    sprintf
 *
 * PARAMETERS:  String              - String with boundary
 *              Format, ...         - Standard printf format
 *
 * RETURN:      Number of bytes actually written.
 *
 * DESCRIPTION: Formatted output to a string.
 *
 ******************************************************************************/

int
sprintf (
    char                    *String,
    const char              *Format,
    ...)
{
    va_list                 Args;
    int                     Length;


    va_start (Args, Format);
    Length = vsnprintf (String, ACPI_UINT32_MAX, Format, Args);
    va_end (Args);

    return (Length);
}


#ifdef ACPI_APPLICATION
/*******************************************************************************
 *
 * FUNCTION:    vprintf
 *
 * PARAMETERS:  Format              - Standard printf format
 *              Args                - Argument list
 *
 * RETURN:      Number of bytes actually written.
 *
 * DESCRIPTION: Formatted output to stdout using argument list pointer.
 *
 ******************************************************************************/

int
vprintf (
    const char              *Format,
    va_list                 Args)
{
    ACPI_CPU_FLAGS          Flags;
    int                     Length;


    Flags = AcpiOsAcquireLock (AcpiGbl_PrintLock);
    Length = vsnprintf (AcpiGbl_PrintBuffer,
                sizeof (AcpiGbl_PrintBuffer), Format, Args);

    (void) fwrite (AcpiGbl_PrintBuffer, Length, 1, ACPI_FILE_OUT);
    AcpiOsReleaseLock (AcpiGbl_PrintLock, Flags);

    return (Length);
}


/*******************************************************************************
 *
 * FUNCTION:    printf
 *
 * PARAMETERS:  Format, ...         - Standard printf format
 *
 * RETURN:      Number of bytes actually written.
 *
 * DESCRIPTION: Formatted output to stdout.
 *
 ******************************************************************************/

int
printf (
    const char              *Format,
    ...)
{
    va_list                 Args;
    int                     Length;


    va_start (Args, Format);
    Length = vprintf (Format, Args);
    va_end (Args);

    return (Length);
}


/*******************************************************************************
 *
 * FUNCTION:    vfprintf
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
vfprintf (
    FILE                    *File,
    const char              *Format,
    va_list                 Args)
{
    ACPI_CPU_FLAGS          Flags;
    int                     Length;


    Flags = AcpiOsAcquireLock (AcpiGbl_PrintLock);
    Length = vsnprintf (AcpiGbl_PrintBuffer,
        sizeof (AcpiGbl_PrintBuffer), Format, Args);

    (void) fwrite (AcpiGbl_PrintBuffer, Length, 1, File);
    AcpiOsReleaseLock (AcpiGbl_PrintLock, Flags);

    return (Length);
}


/*******************************************************************************
 *
 * FUNCTION:    fprintf
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
fprintf (
    FILE                    *File,
    const char              *Format,
    ...)
{
    va_list                 Args;
    int                     Length;


    va_start (Args, Format);
    Length = vfprintf (File, Format, Args);
    va_end (Args);

    return (Length);
}
#endif
