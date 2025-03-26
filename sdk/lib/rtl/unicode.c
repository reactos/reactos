/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Unicode Conversion Routines
 * FILE:              lib/rtl/unicode.c
 * PROGRAMMER:        Alex Ionescu (alex@relsoft.net)
 *                    Emanuele Aliberti
 *                    Gunnar Dalsnes
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

#include <wine/unicode.h>

/* GLOBALS *******************************************************************/

extern BOOLEAN NlsMbCodePageTag;
extern BOOLEAN NlsMbOemCodePageTag;
extern PUSHORT NlsLeadByteInfo;
extern USHORT NlsOemDefaultChar;
extern USHORT NlsUnicodeDefaultChar;
extern PUSHORT NlsOemLeadByteInfo;
extern PWCHAR NlsOemToUnicodeTable;
extern PCHAR NlsUnicodeToOemTable;
extern PUSHORT NlsUnicodeToMbOemTable;


/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
RtlMultiAppendUnicodeStringBuffer(OUT PRTL_UNICODE_STRING_BUFFER StringBuffer,
                                  IN ULONG NumberOfAddends,
                                  IN PCUNICODE_STRING Addends)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
* @implemented
*/
WCHAR
NTAPI
RtlAnsiCharToUnicodeChar(IN OUT PUCHAR *AnsiChar)
{
    ULONG Size;
    NTSTATUS Status;
    WCHAR UnicodeChar = L' ';
    PAGED_CODE_RTL();

    if (NlsLeadByteInfo)
    {
        Size = (NlsLeadByteInfo[**AnsiChar] == 0) ? 1 : 2;
    }
    else
    {
        DPRINT("HACK::Shouldn't have happened! Consider fixing Usetup and registry entries it creates on install\n");
        Size = 1;
    }

    Status = RtlMultiByteToUnicodeN(&UnicodeChar,
                                    sizeof(WCHAR),
                                    NULL,
                                    (PCHAR)*AnsiChar,
                                    Size);

    if (!NT_SUCCESS(Status))
    {
        UnicodeChar = L' ';
    }

    *AnsiChar += Size;
    return UnicodeChar;
}

/*
 * @implemented
 *
 * NOTES
 *  This function always writes a terminating '\0'.
 *  If the dest buffer is too small a partial copy is NOT performed!
 */
NTSTATUS
NTAPI
RtlAnsiStringToUnicodeString(
    IN OUT PUNICODE_STRING UniDest,
    IN PANSI_STRING AnsiSource,
    IN BOOLEAN AllocateDestinationString)
{
    NTSTATUS Status;
    ULONG Length;
    ULONG Index;

    PAGED_CODE_RTL();

    if (NlsMbCodePageTag == FALSE)
    {
        Length = (AnsiSource->Length + 1) * sizeof(WCHAR);
    }
    else
    {
        Length = RtlxAnsiStringToUnicodeSize(AnsiSource);
    }
    if (Length > MAXUSHORT) return STATUS_INVALID_PARAMETER_2;
    UniDest->Length = (USHORT)Length - sizeof(WCHAR);

    if (AllocateDestinationString)
    {
        UniDest->Buffer = RtlpAllocateStringMemory(Length, TAG_USTR);
        UniDest->MaximumLength = (USHORT)Length;
        if (!UniDest->Buffer) return STATUS_NO_MEMORY;
    }
    else if (UniDest->Length >= UniDest->MaximumLength)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    /* UniDest->MaximumLength must be even due to sizeof(WCHAR) being 2 */
    ASSERT(!(UniDest->MaximumLength & 1) && UniDest->Length <= UniDest->MaximumLength);

    Status = RtlMultiByteToUnicodeN(UniDest->Buffer,
                                    UniDest->Length,
                                    &Index,
                                    AnsiSource->Buffer,
                                    AnsiSource->Length);

    if (!NT_SUCCESS(Status))
    {
        if (AllocateDestinationString)
        {
            RtlpFreeStringMemory(UniDest->Buffer, TAG_USTR);
            UniDest->Buffer = NULL;
        }

        return Status;
    }

    UniDest->Buffer[Index / sizeof(WCHAR)] = UNICODE_NULL;
    return Status;
}

/*
 * @implemented
 *
 * RETURNS
 *  The calculated size in bytes including nullterm.
 */
ULONG
NTAPI
RtlxAnsiStringToUnicodeSize(IN PCANSI_STRING AnsiString)
{
    ULONG Size;
    PAGED_CODE_RTL();

    /* Convert from Mb String to Unicode Size */
    RtlMultiByteToUnicodeSize(&Size,
                              AnsiString->Buffer,
                              AnsiString->Length);

    /* Return the size plus the null-char */
    return(Size + sizeof(WCHAR));
}

/*
 * @implemented
 *
 * NOTES
 *  If src->length is zero dest is unchanged.
 *  Dest is never nullterminated.
 */
NTSTATUS
NTAPI
RtlAppendStringToString(IN PSTRING Destination,
                        IN const STRING *Source)
{
    USHORT SourceLength = Source->Length;

    if (SourceLength)
    {
        if (Destination->Length + SourceLength > Destination->MaximumLength)
        {
            return STATUS_BUFFER_TOO_SMALL;
        }

        RtlMoveMemory(&Destination->Buffer[Destination->Length],
                      Source->Buffer,
                      SourceLength);

        Destination->Length += SourceLength;
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 *
 * NOTES
 *  If src->length is zero dest is unchanged.
 *  Dest is nullterminated when the MaximumLength allowes it.
 *  When dest fits exactly in MaximumLength characters the nullterm is ommitted.
 */
NTSTATUS
NTAPI
RtlAppendUnicodeStringToString(
    IN OUT PUNICODE_STRING Destination,
    IN PCUNICODE_STRING Source)
{
    USHORT SourceLength = Source->Length;
    PWCHAR Buffer = &Destination->Buffer[Destination->Length / sizeof(WCHAR)];

    if (SourceLength)
    {
        if ((SourceLength + Destination->Length) > Destination->MaximumLength)
        {
            return STATUS_BUFFER_TOO_SMALL;
        }

        RtlMoveMemory(Buffer, Source->Buffer, SourceLength);
        Destination->Length += SourceLength;

        /* append terminating '\0' if enough space */
        if (Destination->MaximumLength > Destination->Length)
        {
            Buffer[SourceLength / sizeof(WCHAR)] = UNICODE_NULL;
        }
    }

    return STATUS_SUCCESS;
}

/**************************************************************************
 *      RtlCharToInteger   (NTDLL.@)
 * @implemented
 * Converts a character string into its integer equivalent.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. value contains the converted number
 *  Failure: STATUS_INVALID_PARAMETER, if base is not 0, 2, 8, 10 or 16.
 *           STATUS_ACCESS_VIOLATION, if value is NULL.
 *
 * NOTES
 *  For base 0 it uses 10 as base and the string should be in the format
 *      "{whitespace} [+|-] [0[x|o|b]] {digits}".
 *  For other bases the string should be in the format
 *      "{whitespace} [+|-] {digits}".
 *  No check is made for value overflow, only the lower 32 bits are assigned.
 *  If str is NULL it crashes, as the native function does.
 *
 * DIFFERENCES
 *  This function does not read garbage behind '\0' as the native version does.
 */
NTSTATUS
NTAPI
RtlCharToInteger(
    PCSZ str,      /* [I] '\0' terminated single-byte string containing a number */
    ULONG base,    /* [I] Number base for conversion (allowed 0, 2, 8, 10 or 16) */
    PULONG value)  /* [O] Destination for the converted value */
{
    CHAR chCurrent;
    int digit;
    ULONG RunningTotal = 0;
    char bMinus = 0;

    /* skip leading whitespaces */
    while (*str != '\0' && *str <= ' ') str++;

    /* Check for +/- */
    if (*str == '+')
    {
        str++;
    }
    else if (*str == '-')
    {
        bMinus = 1;
        str++;
    }

    /* base = 0 means autobase */
    if (base == 0)
    {
        base = 10;

        if (str[0] == '0')
        {
            if (str[1] == 'b')
            {
                str += 2;
                base = 2;
            }
            else if (str[1] == 'o')
            {
                str += 2;
                base = 8;
            }
            else if (str[1] == 'x')
            {
                str += 2;
                base = 16;
            }
        }
    }
    else if (base != 2 && base != 8 && base != 10 && base != 16)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (value == NULL) return STATUS_ACCESS_VIOLATION;

    while (*str != '\0')
    {
        chCurrent = *str;

        if (chCurrent >= '0' && chCurrent <= '9')
        {
            digit = chCurrent - '0';
        }
        else if (chCurrent >= 'A' && chCurrent <= 'Z')
        {
            digit = chCurrent - 'A' + 10;
        }
        else if (chCurrent >= 'a' && chCurrent <= 'z')
        {
            digit = chCurrent - 'a' + 10;
        }
        else
        {
            digit = -1;
        }

        if (digit < 0 || digit >= (int)base) break;

        RunningTotal = RunningTotal * base + digit;
        str++;
    }

    *value = bMinus ? (0 - RunningTotal) : RunningTotal;
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
LONG
NTAPI
RtlCompareString(
    IN const STRING *s1,
    IN const STRING *s2,
    IN BOOLEAN CaseInsensitive)
{
    unsigned int len;
    LONG ret = 0;
    LPCSTR p1, p2;

    len = min(s1->Length, s2->Length);
    p1 = s1->Buffer;
    p2 = s2->Buffer;

    if (CaseInsensitive)
    {
        while (!ret && len--)
            ret = RtlUpperChar(*p1++) - RtlUpperChar(*p2++);
    }
    else
    {
        while (!ret && len--) ret = *p1++ - *p2++;
    }

    if (!ret) ret = s1->Length - s2->Length;

    return ret;
}

/*
 * @implemented
 *
 * RETURNS
 *  TRUE if strings are equal.
 */
BOOLEAN
NTAPI
RtlEqualString(
    IN const STRING *s1,
    IN const STRING *s2,
    IN BOOLEAN CaseInsensitive)
{
    if (s1->Length != s2->Length) return FALSE;
    return !RtlCompareString(s1, s2, CaseInsensitive);
}

/*
 * @implemented
 *
 * RETURNS
 *  TRUE if strings are equal.
 */
BOOLEAN
NTAPI
RtlEqualUnicodeString(
    IN CONST UNICODE_STRING *s1,
    IN CONST UNICODE_STRING *s2,
    IN BOOLEAN  CaseInsensitive)
{
    if (s1->Length != s2->Length) return FALSE;
    return !RtlCompareUnicodeString(s1, s2, CaseInsensitive );
}

/*
 * @implemented
 */
VOID
NTAPI
RtlFreeAnsiString(IN PANSI_STRING AnsiString)
{
    PAGED_CODE_RTL();

    if (AnsiString->Buffer)
    {
        RtlpFreeStringMemory(AnsiString->Buffer, TAG_ASTR);
        RtlZeroMemory(AnsiString, sizeof(ANSI_STRING));
    }
}

/*
 * @implemented
 */
VOID
NTAPI
RtlFreeOemString(IN POEM_STRING OemString)
{
    PAGED_CODE_RTL();

    if (OemString->Buffer) RtlpFreeStringMemory(OemString->Buffer, TAG_OSTR);
}

/*
 * @implemented
 */
VOID
NTAPI
RtlFreeUnicodeString(IN PUNICODE_STRING UnicodeString)
{
    PAGED_CODE_RTL();

    if (UnicodeString->Buffer)
    {
        RtlpFreeStringMemory(UnicodeString->Buffer, TAG_USTR);
        RtlZeroMemory(UnicodeString, sizeof(UNICODE_STRING));
    }
}


/*
 * @implemented
 *
 * NOTES
 *  Check the OEM string to match the Unicode string.
 *
 *  Functions which convert Unicode strings to OEM strings will set a
 *  DefaultChar from the OEM codepage when the characters are unknown.
 *  So check it against the Unicode string and return false when the
 *  Unicode string does not contain a TransDefaultChar.
 */
BOOLEAN
NTAPI
RtlpDidUnicodeToOemWork(IN PCUNICODE_STRING UnicodeString,
                        IN POEM_STRING OemString)
{
    ULONG i = 0;

    if (NlsMbOemCodePageTag == FALSE)
    {
        /* single-byte code page */
        /* Go through all characters of a string */
        while (i < OemString->Length)
        {
            /* Check if it got translated into a default char,
             * but source char wasn't a default char equivalent
             */
            if ((OemString->Buffer[i] == NlsOemDefaultChar) &&
                (UnicodeString->Buffer[i] != NlsUnicodeDefaultChar))
            {
                /* Yes, it means unmappable characters were found */
                return FALSE;
            }

            /* Move to the next char */
            i++;
        }

        /* All chars were translated successfuly */
        return TRUE;
    }
    else
    {
        /* multibyte code page */

        /* FIXME */
        return TRUE;
    }
}

/*
* @implemented
*/
BOOLEAN
NTAPI
RtlIsValidOemCharacter(IN PWCHAR Char)
{
    WCHAR UnicodeChar;
    WCHAR OemChar;

    /* If multi-byte code page present */
    if (NlsMbOemCodePageTag)
    {
        USHORT Offset;

        OemChar = NlsUnicodeToMbOemTable[*Char];

        /* If character has Lead Byte */
        Offset = NlsOemLeadByteInfo[HIBYTE(OemChar)];
        if (Offset)
        {
            /* Use DBCS table */
            UnicodeChar = NlsOemLeadByteInfo[Offset + LOBYTE(OemChar)];
        }
        else
        {
            UnicodeChar = NlsOemToUnicodeTable[OemChar];
        }

        /* Upcase */
        UnicodeChar = RtlpUpcaseUnicodeChar(UnicodeChar);

        /* Receive OEM character from the table */
        OemChar = NlsUnicodeToMbOemTable[UnicodeChar];
    }
    else
    {
        /* Receive Unicode character from the table */
        UnicodeChar = RtlpUpcaseUnicodeChar(NlsOemToUnicodeTable[(UCHAR)NlsUnicodeToOemTable[*Char]]);

        /* Receive OEM character from the table */
        OemChar = NlsUnicodeToOemTable[UnicodeChar];
    }

    /* Not valid character, failed */
    if (OemChar == NlsOemDefaultChar)
    {
        DPRINT1("\\u%04x is not valid for OEM\n", *Char);
        return FALSE;
    }

    *Char = UnicodeChar;

    return TRUE;
}

/*
 * @implemented
 *
 * NOTES
 *  If source is NULL the length of source is assumed to be 0.
 */
VOID
NTAPI
RtlInitAnsiString(IN OUT PANSI_STRING DestinationString,
                  IN PCSZ SourceString)
{
    SIZE_T Size;

    if (SourceString)
    {
        Size = strlen(SourceString);
        if (Size > (MAXUSHORT - sizeof(CHAR))) Size = MAXUSHORT - sizeof(CHAR);
        DestinationString->Length = (USHORT)Size;
        DestinationString->MaximumLength = (USHORT)Size + sizeof(CHAR);
    }
    else
    {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
    }

    DestinationString->Buffer = (PCHAR)SourceString;
}

NTSTATUS
NTAPI
RtlInitAnsiStringEx(IN OUT PANSI_STRING DestinationString,
                    IN PCSZ SourceString)
{
    SIZE_T Size;

    if (SourceString)
    {
        Size = strlen(SourceString);
        if (Size > (MAXUSHORT - sizeof(CHAR))) return STATUS_NAME_TOO_LONG;
        DestinationString->Length = (USHORT)Size;
        DestinationString->MaximumLength = (USHORT)Size + sizeof(CHAR);
    }
    else
    {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
    }

    DestinationString->Buffer = (PCHAR)SourceString;
    return STATUS_SUCCESS;

}
/*
 * @implemented
 *
 * NOTES
 *  If source is NULL the length of source is assumed to be 0.
 */
VOID
NTAPI
RtlInitString(
    IN OUT PSTRING DestinationString,
    IN PCSZ SourceString)
{
    RtlInitAnsiString(DestinationString, SourceString);
}

/*
 * @implemented
 *
 * NOTES
 *  If source is NULL the length of source is assumed to be 0.
 */
VOID
NTAPI
RtlInitUnicodeString(
    IN OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString)
{
    SIZE_T Size;
    CONST SIZE_T MaxSize = (MAXUSHORT & ~1) - sizeof(UNICODE_NULL); // an even number

    if (SourceString)
    {
        Size = wcslen(SourceString) * sizeof(WCHAR);
        __analysis_assume(Size <= MaxSize);

        if (Size > MaxSize)
            Size = MaxSize;
        DestinationString->Length = (USHORT)Size;
        DestinationString->MaximumLength = (USHORT)Size + sizeof(UNICODE_NULL);
    }
    else
    {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
    }

    DestinationString->Buffer = (PWCHAR)SourceString;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlInitUnicodeStringEx(
    OUT PUNICODE_STRING DestinationString,
    IN PCWSTR SourceString)
{
    SIZE_T Size;
    CONST SIZE_T MaxSize = (MAXUSHORT & ~1) - sizeof(WCHAR); // an even number

    if (SourceString)
    {
        Size = wcslen(SourceString) * sizeof(WCHAR);
        if (Size > MaxSize) return STATUS_NAME_TOO_LONG;
        DestinationString->Length = (USHORT)Size;
        DestinationString->MaximumLength = (USHORT)Size + sizeof(WCHAR);
    }
    else
    {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
    }

    DestinationString->Buffer = (PWCHAR)SourceString;
    return STATUS_SUCCESS;
}

/*
 * @implemented
 *
 * NOTES
 *  Writes at most length characters to the string str.
 *  Str is nullterminated when length allowes it.
 *  When str fits exactly in length characters the nullterm is ommitted.
 */
NTSTATUS NTAPI RtlIntegerToChar(
    ULONG value,   /* [I] Value to be converted */
    ULONG base,    /* [I] Number base for conversion (allowed 0, 2, 8, 10 or 16) */
    ULONG length,  /* [I] Length of the str buffer in bytes */
    PCHAR str)     /* [O] Destination for the converted value */
{
    CHAR buffer[33];
    PCHAR pos;
    CHAR digit;
    SIZE_T len;

    if (base == 0)
    {
        base = 10;
    }
    else if (base != 2 && base != 8 && base != 10 && base != 16)
    {
        return STATUS_INVALID_PARAMETER;
    }

    pos = &buffer[32];
    *pos = '\0';

    do
    {
        pos--;
        digit = (CHAR)(value % base);
        value = value / base;

        if (digit < 10)
        {
            *pos = '0' + digit;
        }
        else
        {
            *pos = 'A' + digit - 10;
        }
    }
    while (value != 0L);

    len = &buffer[32] - pos;

    if (len > length)
    {
        return STATUS_BUFFER_OVERFLOW;
    }
    else if (str == NULL)
    {
        return STATUS_ACCESS_VIOLATION;
    }
    else if (len == length)
    {
        RtlCopyMemory(str, pos, len);
    }
    else
    {
        RtlCopyMemory(str, pos, len + 1);
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlIntegerToUnicode(
    IN ULONG Value,
    IN ULONG Base  OPTIONAL,
    IN ULONG Length OPTIONAL,
    IN OUT LPWSTR String)
{
    ULONG Radix;
    WCHAR  temp[33];
    ULONG v = Value;
    ULONG i;
    PWCHAR tp;
    PWCHAR sp;

    Radix = Base;

    if (Radix == 0) Radix = 10;

    if ((Radix != 2) && (Radix != 8) &&
        (Radix != 10) && (Radix != 16))
    {
        return STATUS_INVALID_PARAMETER;
    }

    tp = temp;

    while (v || tp == temp)
    {
        i = v % Radix;
        v = v / Radix;

        if (i < 10) *tp = (WCHAR)(i + L'0');
        else *tp = (WCHAR)(i + L'a' - 10);

        tp++;
    }

    if ((ULONG)((ULONG_PTR)tp - (ULONG_PTR)temp) >= Length)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    sp = String;

    while (tp > temp) *sp++ = *--tp;

    *sp = 0;

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlIntegerToUnicodeString(
    IN ULONG Value,
    IN ULONG Base OPTIONAL,
    IN OUT PUNICODE_STRING String)
{
    ANSI_STRING AnsiString;
    CHAR Buffer[33];
    NTSTATUS Status;

    Status = RtlIntegerToChar(Value, Base, sizeof(Buffer), Buffer);
    if (NT_SUCCESS(Status))
    {
        AnsiString.Buffer = Buffer;
        AnsiString.Length = (USHORT)strlen(Buffer);
        AnsiString.MaximumLength = sizeof(Buffer);

        Status = RtlAnsiStringToUnicodeString(String, &AnsiString, FALSE);
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlInt64ToUnicodeString (
    IN ULONGLONG Value,
    IN ULONG Base OPTIONAL,
    IN OUT PUNICODE_STRING String)
{
    LARGE_INTEGER LargeInt;
    ANSI_STRING AnsiString;
    CHAR Buffer[65];
    NTSTATUS Status;

    LargeInt.QuadPart = Value;

    Status = RtlLargeIntegerToChar(&LargeInt, Base, sizeof(Buffer), Buffer);
    if (NT_SUCCESS(Status))
    {
        AnsiString.Buffer = Buffer;
        AnsiString.Length = (USHORT)strlen(Buffer);
        AnsiString.MaximumLength = sizeof(Buffer);

        Status = RtlAnsiStringToUnicodeString(String, &AnsiString, FALSE);
    }

    return Status;
}

/*
 * @implemented
 *
 * RETURNS
 *  TRUE if String2 contains String1 as a prefix.
 */
BOOLEAN
NTAPI
RtlPrefixString(
    const STRING *String1,
    const STRING *String2,
    BOOLEAN CaseInsensitive)
{
    PCHAR pc1;
    PCHAR pc2;
    ULONG NumChars;

    if (String2->Length < String1->Length)
        return FALSE;

    NumChars = String1->Length;
    pc1 = String1->Buffer;
    pc2 = String2->Buffer;

    if (pc1 && pc2)
    {
        if (CaseInsensitive)
        {
            while (NumChars--)
            {
                if (RtlUpperChar(*pc1++) != RtlUpperChar(*pc2++))
                    return FALSE;
            }
        }
        else
        {
            while (NumChars--)
            {
                if (*pc1++ != *pc2++)
                    return FALSE;
            }
        }

        return TRUE;
    }

    return FALSE;
}

/*
 * @implemented
 *
 * RETURNS
 *  TRUE if String2 contains String1 as a prefix.
 */
BOOLEAN
NTAPI
RtlPrefixUnicodeString(
    PCUNICODE_STRING String1,
    PCUNICODE_STRING String2,
    BOOLEAN CaseInsensitive)
{
    PWCHAR pc1;
    PWCHAR pc2;
    ULONG  NumChars;

    if (String2->Length < String1->Length)
        return FALSE;

    NumChars = String1->Length / sizeof(WCHAR);
    pc1 = String1->Buffer;
    pc2 = String2->Buffer;

    if (pc1 && pc2)
    {
        if (CaseInsensitive)
        {
            while (NumChars--)
            {
                if (RtlpUpcaseUnicodeChar(*pc1++) !=
                    RtlpUpcaseUnicodeChar(*pc2++))
                    return FALSE;
            }
        }
        else
        {
            while (NumChars--)
            {
                if (*pc1++ != *pc2++)
                    return FALSE;
            }
        }

        return TRUE;
    }

    return FALSE;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlUnicodeStringToInteger(
    const UNICODE_STRING *str, /* [I] Unicode string to be converted */
    ULONG base,                /* [I] Number base for conversion (allowed 0, 2, 8, 10 or 16) */
    ULONG *value)              /* [O] Destination for the converted value */
{
    LPWSTR lpwstr = str->Buffer;
    USHORT CharsRemaining = str->Length / sizeof(WCHAR);
    WCHAR wchCurrent;
    int digit;
    ULONG RunningTotal = 0;
    char bMinus = 0;

    while (CharsRemaining >= 1 && *lpwstr <= ' ')
    {
        lpwstr++;
        CharsRemaining--;
    }

    if (CharsRemaining >= 1)
    {
        if (*lpwstr == '+')
        {
            lpwstr++;
            CharsRemaining--;
        }
        else if (*lpwstr == '-')
        {
            bMinus = 1;
            lpwstr++;
            CharsRemaining--;
        }
    }

    if (base == 0)
    {
        base = 10;

        if (CharsRemaining >= 2 && lpwstr[0] == '0')
        {
            if (lpwstr[1] == 'b')
            {
                lpwstr += 2;
                CharsRemaining -= 2;
                base = 2;
            }
            else if (lpwstr[1] == 'o')
            {
                lpwstr += 2;
                CharsRemaining -= 2;
                base = 8;
            }
            else if (lpwstr[1] == 'x')
            {
                lpwstr += 2;
                CharsRemaining -= 2;
                base = 16;
            }
        }
    }
    else if (base != 2 && base != 8 && base != 10 && base != 16)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (value == NULL)
    {
        return STATUS_ACCESS_VIOLATION;
    }

    while (CharsRemaining >= 1)
    {
        wchCurrent = *lpwstr;

        if (wchCurrent >= '0' && wchCurrent <= '9')
        {
            digit = wchCurrent - '0';
        }
        else if (wchCurrent >= 'A' && wchCurrent <= 'Z')
        {
            digit = wchCurrent - 'A' + 10;
        }
        else if (wchCurrent >= 'a' && wchCurrent <= 'z')
        {
            digit = wchCurrent - 'a' + 10;
        }
        else
        {
            digit = -1;
        }

        if (digit < 0 || (ULONG)digit >= base) break;

        RunningTotal = RunningTotal * base + digit;
        lpwstr++;
        CharsRemaining--;
    }

    *value = bMinus ? (0 - RunningTotal) : RunningTotal;
    return STATUS_SUCCESS;
}

/*
 * @implemented
 *
 * RETURNS
 *  Bytes necessary for the conversion including nullterm.
 */
ULONG
NTAPI
RtlxUnicodeStringToOemSize(IN PCUNICODE_STRING UnicodeString)
{
    ULONG Size;

    /* Convert the Unicode String to Mb Size */
    RtlUnicodeToMultiByteSize(&Size,
                              UnicodeString->Buffer,
                              UnicodeString->Length);

    /* Return the size + the null char */
    return (Size + sizeof(CHAR));
}

/*
 * @implemented
 *
 * NOTES
 *  This function always writes a terminating '\0'.
 *  It performs a partial copy if ansi is too small.
 */
NTSTATUS
NTAPI
RtlUnicodeStringToAnsiString(
    IN OUT PANSI_STRING AnsiDest,
    IN PCUNICODE_STRING UniSource,
    IN BOOLEAN AllocateDestinationString)
{
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS RealStatus;
    ULONG Length;
    ULONG Index;

    PAGED_CODE_RTL();

    ASSERT(!(UniSource->Length & 1));

    if (NlsMbCodePageTag == FALSE)
    {
        Length = (UniSource->Length + sizeof(WCHAR)) / sizeof(WCHAR);
    }
    else
    {
        Length = RtlxUnicodeStringToAnsiSize(UniSource);
    }

    if (Length > MAXUSHORT) return STATUS_INVALID_PARAMETER_2;

    AnsiDest->Length = (USHORT)Length - sizeof(CHAR);

    if (AllocateDestinationString)
    {
        AnsiDest->Buffer = RtlpAllocateStringMemory(Length, TAG_ASTR);
        AnsiDest->MaximumLength = (USHORT)Length;

        if (!AnsiDest->Buffer) return STATUS_NO_MEMORY;
    }
    else if (AnsiDest->Length >= AnsiDest->MaximumLength)
    {
        if (!AnsiDest->MaximumLength) return STATUS_BUFFER_OVERFLOW;

        Status = STATUS_BUFFER_OVERFLOW;
        AnsiDest->Length = AnsiDest->MaximumLength - 1;
    }

    RealStatus = RtlUnicodeToMultiByteN(AnsiDest->Buffer,
                                        AnsiDest->Length,
                                        &Index,
                                        UniSource->Buffer,
                                        UniSource->Length);

    if (!NT_SUCCESS(RealStatus) && AllocateDestinationString)
    {
        RtlpFreeStringMemory(AnsiDest->Buffer, TAG_ASTR);
        return RealStatus;
    }

    AnsiDest->Buffer[Index] = ANSI_NULL;
    return Status;
}

/*
 * @implemented
 *
 * NOTES
 *  This function always writes a terminating '\0'.
 *  Does NOT perform a partial copy if unicode is too small!
 */
NTSTATUS
NTAPI
RtlOemStringToUnicodeString(
    IN OUT PUNICODE_STRING UniDest,
    IN PCOEM_STRING OemSource,
    IN BOOLEAN AllocateDestinationString)
{
    NTSTATUS Status;
    ULONG Length;
    ULONG Index;

    PAGED_CODE_RTL();

    Length = RtlOemStringToUnicodeSize(OemSource);

    if (Length > MAXUSHORT) return STATUS_INVALID_PARAMETER_2;

    UniDest->Length = (USHORT)Length - sizeof(WCHAR);

    if (AllocateDestinationString)
    {
        UniDest->Buffer = RtlpAllocateStringMemory(Length, TAG_USTR);
        UniDest->MaximumLength = (USHORT)Length;

        if (!UniDest->Buffer) return STATUS_NO_MEMORY;
    }
    else if (UniDest->Length >= UniDest->MaximumLength)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    Status = RtlOemToUnicodeN(UniDest->Buffer,
                              UniDest->Length,
                              &Index,
                              OemSource->Buffer,
                              OemSource->Length);

    if (!NT_SUCCESS(Status) && AllocateDestinationString)
    {
        RtlpFreeStringMemory(UniDest->Buffer, TAG_USTR);
        UniDest->Buffer = NULL;
        return Status;
    }

    UniDest->Buffer[Index / sizeof(WCHAR)] = UNICODE_NULL;
    return Status;
}

/*
 * @implemented
 *
 * NOTES
 *   This function always '\0' terminates the string returned.
 */
NTSTATUS
NTAPI
RtlUnicodeStringToOemString(
    IN OUT POEM_STRING OemDest,
    IN PCUNICODE_STRING UniSource,
    IN BOOLEAN  AllocateDestinationString)
{
    NTSTATUS Status;
    ULONG Length;
    ULONG Index;

    PAGED_CODE_RTL();

    Length = RtlUnicodeStringToOemSize(UniSource);

    if (Length > MAXUSHORT) return STATUS_INVALID_PARAMETER_2;

    OemDest->Length = (USHORT)Length - sizeof(CHAR);

    if (AllocateDestinationString)
    {
        OemDest->Buffer = RtlpAllocateStringMemory(Length, TAG_OSTR);
        OemDest->MaximumLength = (USHORT)Length;

        if (!OemDest->Buffer) return STATUS_NO_MEMORY;
    }
    else if (OemDest->Length >= OemDest->MaximumLength)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    Status = RtlUnicodeToOemN(OemDest->Buffer,
                              OemDest->Length,
                              &Index,
                              UniSource->Buffer,
                              UniSource->Length);

    if (!NT_SUCCESS(Status) && AllocateDestinationString)
    {
        RtlpFreeStringMemory(OemDest->Buffer, TAG_OSTR);
        OemDest->Buffer = NULL;
        return Status;
    }

    OemDest->Buffer[Index] = ANSI_NULL;
    return Status;
}

#define ITU_IMPLEMENTED_TESTS (IS_TEXT_UNICODE_ODD_LENGTH|IS_TEXT_UNICODE_SIGNATURE)

/*
 * @implemented
 *
 * RETURNS
 *  The length of the string if all tests were passed, 0 otherwise.
 */
BOOLEAN
NTAPI
RtlIsTextUnicode(CONST VOID* buf, INT len, INT* pf)
{
    static const WCHAR std_control_chars[] = {'\r', '\n', '\t', ' ', 0x3000, 0};
    static const WCHAR byterev_control_chars[] = {0x0d00, 0x0a00, 0x0900, 0x2000, 0};
    const WCHAR *s = buf;
    int i;
    unsigned int flags = MAXULONG, out_flags = 0;
    UCHAR last_lo_byte = 0;
    UCHAR last_hi_byte = 0;
    ULONG hi_byte_diff = 0;
    ULONG lo_byte_diff = 0;
    ULONG weight = 3;
    ULONG lead_byte = 0;

    if (len < sizeof(WCHAR))
    {
        /* FIXME: MSDN documents IS_TEXT_UNICODE_BUFFER_TOO_SMALL but there is no such thing... */
        if (pf) *pf = 0;

        return FALSE;
    }

    if (pf)
        flags = *pf;

    /*
     * Apply various tests to the text string. According to the
     * docs, each test "passed" sets the corresponding flag in
     * the output flags. But some of the tests are mutually
     * exclusive, so I don't see how you could pass all tests ...
     */

    /* Check for an odd length ... pass if even. */
    if (len & 1) out_flags |= IS_TEXT_UNICODE_ODD_LENGTH;

    if (((char *)buf)[len - 1] == 0)
        len--;  /* Windows seems to do something like that to avoid e.g. false IS_TEXT_UNICODE_NULL_BYTES  */

    len /= sizeof(WCHAR);

    /* Windows only checks the first 256 characters */
    if (len > 256) len = 256;

    /* Check for the special byte order unicode marks. */
    if (*s == 0xFEFF) out_flags |= IS_TEXT_UNICODE_SIGNATURE;
    if (*s == 0xFFFE) out_flags |= IS_TEXT_UNICODE_REVERSE_SIGNATURE;

    for (i = 0; i < len; i++)
    {
        UCHAR lo_byte = LOBYTE(s[i]);
        UCHAR hi_byte = HIBYTE(s[i]);

        lo_byte_diff += max(lo_byte, last_lo_byte) - min(lo_byte, last_lo_byte);
        hi_byte_diff += max(hi_byte, last_hi_byte) - min(hi_byte, last_hi_byte);

        last_lo_byte = lo_byte;
        last_hi_byte = hi_byte;

        switch (s[i])
        {
            case 0xFFFE: /* Reverse BOM */
            case UNICODE_NULL:
            case 0x0A0D: /* ASCII CRLF (packed into one word) */
            case 0xFFFF: /* Unicode 0xFFFF */
                out_flags |= IS_TEXT_UNICODE_ILLEGAL_CHARS;
                break;
        }
    }

    if (NlsMbCodePageTag && pf && (*pf & IS_TEXT_UNICODE_DBCS_LEADBYTE))
    {
        for (i = 0; i < len; i++)
        {
            if (NlsLeadByteInfo[s[i]])
            {
                ++lead_byte;
                ++i;
            }
        }

        if (lead_byte)
        {
            weight = (len / 2) - 1;

            if (lead_byte < (weight / 3))
                weight = 3;
            else if (lead_byte < ((weight * 2) / 3))
                weight = 2;
            else
                weight = 1;

            out_flags |= IS_TEXT_UNICODE_DBCS_LEADBYTE;
        }
    }

    if (lo_byte_diff < 127 && !hi_byte_diff)
    {
        out_flags |= IS_TEXT_UNICODE_ASCII16;
    }

    if (hi_byte_diff && !lo_byte_diff)
    {
        out_flags |= IS_TEXT_UNICODE_REVERSE_ASCII16;
    }

    if ((weight * lo_byte_diff) < hi_byte_diff)
    {
        out_flags |= IS_TEXT_UNICODE_REVERSE_STATISTICS;
    }

    /* apply some statistical analysis */
    if ((flags & IS_TEXT_UNICODE_STATISTICS) &&
        ((weight * hi_byte_diff) < lo_byte_diff))
    {
        out_flags |= IS_TEXT_UNICODE_STATISTICS;
    }

    /* Check for unicode NULL chars */
    if (flags & IS_TEXT_UNICODE_NULL_BYTES)
    {
        for (i = 0; i < len; i++)
        {
            if (!(s[i] & 0xff) || !(s[i] >> 8))
            {
                out_flags |= IS_TEXT_UNICODE_NULL_BYTES;
                break;
            }
        }
    }

    if (flags & IS_TEXT_UNICODE_CONTROLS)
    {
        for (i = 0; i < len; i++)
        {
            if (strchrW(std_control_chars, s[i]))
            {
                out_flags |= IS_TEXT_UNICODE_CONTROLS;
                break;
            }
        }
    }

    if (flags & IS_TEXT_UNICODE_REVERSE_CONTROLS)
    {
        for (i = 0; i < len; i++)
        {
            if (strchrW(byterev_control_chars, s[i]))
            {
                out_flags |= IS_TEXT_UNICODE_REVERSE_CONTROLS;
                break;
            }
        }
    }

    if (pf)
    {
        out_flags &= *pf;
        *pf = out_flags;
    }

    /* check for flags that indicate it's definitely not valid Unicode */
    if (out_flags & (IS_TEXT_UNICODE_REVERSE_MASK | IS_TEXT_UNICODE_NOT_UNICODE_MASK)) return FALSE;

    /* now check for invalid ASCII, and assume Unicode if so */
    if (out_flags & IS_TEXT_UNICODE_NOT_ASCII_MASK) return TRUE;

    /* now check for Unicode flags */
    if (out_flags & IS_TEXT_UNICODE_UNICODE_MASK) return TRUE;

    /* no flags set */
    return FALSE;
}


/*
 * @implemented
 *
 * NOTES
 *  Same as RtlOemStringToUnicodeString but doesn't write terminating null
 *  A partial copy is NOT performed if the dest buffer is too small!
 */
NTSTATUS
NTAPI
RtlOemStringToCountedUnicodeString(
    IN OUT PUNICODE_STRING UniDest,
    IN PCOEM_STRING OemSource,
    IN BOOLEAN AllocateDestinationString)
{
    NTSTATUS Status;
    ULONG Length;
    ULONG Index;

    PAGED_CODE_RTL();

    /* Calculate size of the string */
    Length = RtlOemStringToCountedUnicodeSize(OemSource);

    /* If it's 0 then zero out dest string and return */
    if (!Length)
    {
        RtlZeroMemory(UniDest, sizeof(UNICODE_STRING));
        return STATUS_SUCCESS;
    }

    /* Check if length is a sane value */
    if (Length > MAXUSHORT) return STATUS_INVALID_PARAMETER_2;

    /* Store it in dest string */
    UniDest->Length = (USHORT)Length;

    /* If we're asked to alloc the string - do so */
    if (AllocateDestinationString)
    {
        UniDest->Buffer = RtlpAllocateStringMemory(Length, TAG_USTR);
        UniDest->MaximumLength = (USHORT)Length;

        if (!UniDest->Buffer) return STATUS_NO_MEMORY;
    }
    else if (UniDest->Length > UniDest->MaximumLength)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    /* Do the conversion */
    Status = RtlOemToUnicodeN(UniDest->Buffer,
                              UniDest->Length,
                              &Index,
                              OemSource->Buffer,
                              OemSource->Length);

    if (!NT_SUCCESS(Status) && AllocateDestinationString)
    {
        /* Conversion failed, free dest string and return status code */
        RtlpFreeStringMemory(UniDest->Buffer, TAG_USTR);
        UniDest->Buffer = NULL;
        return Status;
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 *
 * RETURNS
 *  TRUE if the names are equal, FALSE if not
 *
 * NOTES
 *  The comparison is case insensitive.
 */
BOOLEAN
NTAPI
RtlEqualComputerName(
    IN PUNICODE_STRING ComputerName1,
    IN PUNICODE_STRING ComputerName2)
{
    OEM_STRING OemString1;
    OEM_STRING OemString2;
    BOOLEAN Result = FALSE;

    if (NT_SUCCESS(RtlUpcaseUnicodeStringToOemString(&OemString1,
                                                     ComputerName1,
                                                     TRUE)))
    {
        if (NT_SUCCESS(RtlUpcaseUnicodeStringToOemString(&OemString2,
                                                         ComputerName2,
                                                         TRUE)))
        {
            Result = RtlEqualString(&OemString1, &OemString2, FALSE);
            RtlFreeOemString(&OemString2);
        }

        RtlFreeOemString(&OemString1);
    }

    return Result;
}

/*
 * @implemented
 *
 * RETURNS
 *  TRUE if the names are equal, FALSE if not
 *
 * NOTES
 *  The comparison is case insensitive.
 */
BOOLEAN
NTAPI
RtlEqualDomainName (
    IN PUNICODE_STRING DomainName1,
    IN PUNICODE_STRING DomainName2)
{
    return RtlEqualComputerName(DomainName1, DomainName2);
}

/*
 * @implemented
 *
 * RIPPED FROM WINE's ntdll\rtlstr.c rev 1.45
 *
 * Convert a string representation of a GUID into a GUID.
 *
 * PARAMS
 *  str  [I] String representation in the format "{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}"
 *  guid [O] Destination for the converted GUID
 *
 * RETURNS
 *  Success: STATUS_SUCCESS. guid contains the converted value.
 *  Failure: STATUS_INVALID_PARAMETER, if str is not in the expected format.
 *
 * SEE ALSO
 *  See RtlStringFromGUID.
 */
NTSTATUS
NTAPI
RtlGUIDFromString(
    IN UNICODE_STRING *str,
    OUT GUID* guid)
{
    int i = 0;
    const WCHAR *lpszCLSID = str->Buffer;
    BYTE* lpOut = (BYTE*)guid;

    //TRACE("(%s,%p)\n", debugstr_us(str), guid);

    /* Convert string: {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}
     * to memory:       DWORD... WORD WORD BYTES............
     */
    while (i <= 37)
    {
        switch (i)
        {
            case 0:
                if (*lpszCLSID != '{')
                    return STATUS_INVALID_PARAMETER;
                break;

            case 9:
            case 14:
            case 19:
            case 24:
                if (*lpszCLSID != '-')
                    return STATUS_INVALID_PARAMETER;
                break;

            case 37:
                if (*lpszCLSID != '}')
                    return STATUS_INVALID_PARAMETER;

                break;

            default:
            {
                WCHAR ch = *lpszCLSID, ch2 = lpszCLSID[1];
                unsigned char byte;

                /* Read two hex digits as a byte value */
                if      (ch >= '0' && ch <= '9')
                    ch = ch - '0';
                else if (ch >= 'a' && ch <= 'f')
                    ch = ch - 'a' + 10;
                else if (ch >= 'A' && ch <= 'F')
                    ch = ch - 'A' + 10;
                else
                    return STATUS_INVALID_PARAMETER;

                if      (ch2 >= '0' && ch2 <= '9')
                    ch2 = ch2 - '0';
                else if (ch2 >= 'a' && ch2 <= 'f')
                    ch2 = ch2 - 'a' + 10;
                else if (ch2 >= 'A' && ch2 <= 'F')
                    ch2 = ch2 - 'A' + 10;
                else
                    return STATUS_INVALID_PARAMETER;

                byte = ch << 4 | ch2;

                switch (i)
                {
#ifndef WORDS_BIGENDIAN
                        /* For Big Endian machines, we store the data such that the
                         * dword/word members can be read as DWORDS and WORDS correctly. */
                        /* Dword */
                    case 1:
                        lpOut[3] = byte;
                        break;
                    case 3:
                        lpOut[2] = byte;
                        break;
                    case 5:
                        lpOut[1] = byte;
                        break;
                    case 7:
                        lpOut[0] = byte;
                        lpOut += 4;
                        break;
                        /* Word */
                    case 10:
                    case 15:
                        lpOut[1] = byte;
                        break;
                    case 12:
                    case 17:
                        lpOut[0] = byte;
                        lpOut += 2;
                        break;
#endif
                        /* Byte */
                    default:
                        lpOut[0] = byte;
                        lpOut++;
                        break;
                }

                lpszCLSID++; /* Skip 2nd character of byte */
                i++;
            }
        }

        lpszCLSID++;
        i++;
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
NTAPI
RtlEraseUnicodeString(
    IN PUNICODE_STRING String)
{
    if (String->Buffer && String->MaximumLength)
    {
        RtlZeroMemory(String->Buffer, String->MaximumLength);
        String->Length = 0;
    }
}

/*
* @implemented
*/
NTSTATUS
NTAPI
RtlHashUnicodeString(
    IN CONST UNICODE_STRING *String,
    IN BOOLEAN CaseInSensitive,
    IN ULONG HashAlgorithm,
    OUT PULONG HashValue)
{
    if (String != NULL && HashValue != NULL)
    {
        switch (HashAlgorithm)
        {
            case HASH_STRING_ALGORITHM_DEFAULT:
            case HASH_STRING_ALGORITHM_X65599:
            {
                WCHAR *c, *end;

                *HashValue = 0;
                end = String->Buffer + (String->Length / sizeof(WCHAR));

                if (CaseInSensitive)
                {
                    for (c = String->Buffer; c != end; c++)
                    {
                        /* only uppercase characters if they are 'a' ... 'z'! */
                        *HashValue = ((65599 * (*HashValue)) +
                                      (ULONG)(((*c) >= L'a' && (*c) <= L'z') ?
                                              (*c) - L'a' + L'A' : (*c)));
                    }
                }
                else
                {
                    for (c = String->Buffer; c != end; c++)
                    {
                        *HashValue = ((65599 * (*HashValue)) + (ULONG)(*c));
                    }
                }

                return STATUS_SUCCESS;
            }
        }
    }

    return STATUS_INVALID_PARAMETER;
}

/*
 * @implemented
 *
 * NOTES
 *  Same as RtlUnicodeStringToOemString but doesn't write terminating null
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToCountedOemString(
    _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
    _When_(!AllocateDestinationString, _Inout_)
        POEM_STRING OemDest,
    _In_ PCUNICODE_STRING UniSource,
    _In_ BOOLEAN AllocateDestinationString)
{
    NTSTATUS Status;
    ULONG Length;
    ULONG Index;

    PAGED_CODE_RTL();

    /* Calculate size of the string */
    Length = RtlUnicodeStringToCountedOemSize(UniSource);

    /* If it's 0 then zero out dest string and return */
    if (!Length)
    {
        RtlZeroMemory(OemDest, sizeof(OEM_STRING));
        return STATUS_SUCCESS;
    }

    /* Check if length is a sane value */
    if (Length > MAXUSHORT) return STATUS_INVALID_PARAMETER_2;

    /* Store it in dest string */
    OemDest->Length = (USHORT)Length;

    /* If we're asked to alloc the string - do so */
    if (AllocateDestinationString)
    {
        OemDest->Buffer = RtlpAllocateStringMemory(Length, TAG_OSTR);
        OemDest->MaximumLength = (USHORT)Length;
        if (!OemDest->Buffer) return STATUS_NO_MEMORY;
    }
    else if (OemDest->Length > OemDest->MaximumLength)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    /* Do the conversion */
    Status = RtlUnicodeToOemN(OemDest->Buffer,
                              OemDest->Length,
                              &Index,
                              UniSource->Buffer,
                              UniSource->Length);

    /* Check for unmapped character */
    if (NT_SUCCESS(Status) && !RtlpDidUnicodeToOemWork(UniSource, OemDest))
        Status = STATUS_UNMAPPABLE_CHARACTER;

    if (!NT_SUCCESS(Status) && AllocateDestinationString)
    {
        /* Conversion failed, free dest string and return status code */
        RtlpFreeStringMemory(OemDest->Buffer, TAG_OSTR);
        OemDest->Buffer = NULL;
        return Status;
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlLargeIntegerToChar(
    IN PLARGE_INTEGER Value,
    IN ULONG  Base,
    IN ULONG  Length,
    IN OUT PCHAR  String)
{
    ULONGLONG Val = Value->QuadPart;
    CHAR Buffer[65];
    CHAR Digit;
    SIZE_T Len;
    PCHAR Pos;

    if (Base == 0) Base = 10;

    if ((Base != 2) && (Base != 8) && (Base != 10) && (Base != 16))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Pos = &Buffer[64];
    *Pos = '\0';

    do
    {
        Pos--;
        Digit = (CHAR)(Val % Base);
        Val = Val / Base;

        if (Digit < 10)
            *Pos = '0' + Digit;
        else
            *Pos = 'A' + Digit - 10;
    }
    while (Val != 0L);

    Len = &Buffer[64] - Pos;

    if (Len > Length)
        return STATUS_BUFFER_OVERFLOW;

    /* If possible, add the 0 termination */
    if (Len < Length)
        Len += 1;

    /* Copy the string to the target using SEH */
    return RtlpSafeCopyMemory(String, Pos, Len);
}

/*
 * @implemented
 *
 * NOTES
 *  dest is never '\0' terminated because it may be equal to src, and src
 *  might not be '\0' terminated. dest->Length is only set upon success.
 */
NTSTATUS
NTAPI
RtlUpcaseUnicodeString(
    IN OUT PUNICODE_STRING UniDest,
    IN PCUNICODE_STRING UniSource,
    IN BOOLEAN  AllocateDestinationString)
{
    ULONG i, j;

    PAGED_CODE_RTL();

    if (AllocateDestinationString)
    {
        UniDest->MaximumLength = UniSource->Length;
        UniDest->Buffer = RtlpAllocateStringMemory(UniDest->MaximumLength, TAG_USTR);
        if (UniDest->Buffer == NULL) return STATUS_NO_MEMORY;
    }
    else if (UniSource->Length > UniDest->MaximumLength)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    j = UniSource->Length / sizeof(WCHAR);

    for (i = 0; i < j; i++)
    {
        UniDest->Buffer[i] = RtlpUpcaseUnicodeChar(UniSource->Buffer[i]);
    }

    UniDest->Length = UniSource->Length;
    return STATUS_SUCCESS;
}

/*
 * @implemented
 *
 * NOTES
 *  This function always writes a terminating '\0'.
 *  It performs a partial copy if ansi is too small.
 */
NTSTATUS
NTAPI
RtlUpcaseUnicodeStringToAnsiString(
    IN OUT PANSI_STRING AnsiDest,
    IN PCUNICODE_STRING UniSource,
    IN BOOLEAN  AllocateDestinationString)
{
    NTSTATUS Status;
    ULONG Length;
    ULONG Index;
    PAGED_CODE_RTL();

    Length = RtlUnicodeStringToAnsiSize(UniSource);
    if (Length > MAXUSHORT) return STATUS_INVALID_PARAMETER_2;

    AnsiDest->Length = (USHORT)Length - sizeof(CHAR);

    if (AllocateDestinationString)
    {
        AnsiDest->Buffer = RtlpAllocateStringMemory(Length, TAG_ASTR);
        AnsiDest->MaximumLength = (USHORT)Length;
        if (!AnsiDest->Buffer) return STATUS_NO_MEMORY;
    }
    else if (AnsiDest->Length >= AnsiDest->MaximumLength)
    {
        if (!AnsiDest->MaximumLength) return STATUS_BUFFER_OVERFLOW;
    }

    Status = RtlUpcaseUnicodeToMultiByteN(AnsiDest->Buffer,
                                          AnsiDest->Length,
                                          &Index,
                                          UniSource->Buffer,
                                          UniSource->Length);

    if (!NT_SUCCESS(Status) && AllocateDestinationString)
    {
        RtlpFreeStringMemory(AnsiDest->Buffer, TAG_ASTR);
        AnsiDest->Buffer = NULL;
        return Status;
    }

    AnsiDest->Buffer[Index] = ANSI_NULL;
    return Status;
}

/*
 * @implemented
 *
 * NOTES
 *  This function always writes a terminating '\0'.
 *  It performs a partial copy if ansi is too small.
 */
NTSTATUS
NTAPI
RtlUpcaseUnicodeStringToCountedOemString(
    IN OUT POEM_STRING OemDest,
    IN PCUNICODE_STRING UniSource,
    IN BOOLEAN AllocateDestinationString)
{
    NTSTATUS Status;
    ULONG Length;
    ULONG Index;
    PAGED_CODE_RTL();

    Length = RtlUnicodeStringToCountedOemSize(UniSource);

    if (!Length)
    {
        RtlZeroMemory(OemDest, sizeof(OEM_STRING));
    }

    if (Length > MAXUSHORT) return STATUS_INVALID_PARAMETER_2;

    OemDest->Length = (USHORT)Length;

    if (AllocateDestinationString)
    {
        OemDest->Buffer = RtlpAllocateStringMemory(Length, TAG_OSTR);
        OemDest->MaximumLength = (USHORT)Length;
        if (!OemDest->Buffer) return STATUS_NO_MEMORY;
    }
    else if (OemDest->Length > OemDest->MaximumLength)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    Status = RtlUpcaseUnicodeToOemN(OemDest->Buffer,
                                    OemDest->Length,
                                    &Index,
                                    UniSource->Buffer,
                                    UniSource->Length);

    /* Check for unmapped characters */
    if (NT_SUCCESS(Status) && !RtlpDidUnicodeToOemWork(UniSource, OemDest))
        Status = STATUS_UNMAPPABLE_CHARACTER;

    if (!NT_SUCCESS(Status) && AllocateDestinationString)
    {
        RtlpFreeStringMemory(OemDest->Buffer, TAG_OSTR);
        OemDest->Buffer = NULL;
        return Status;
    }

    return Status;
}

/*
 * @implemented
 * NOTES
 *  OEM string is always nullterminated
 *  It performs a partial copy if oem is too small.
 */
NTSTATUS
NTAPI
RtlUpcaseUnicodeStringToOemString (
    IN OUT POEM_STRING OemDest,
    IN PCUNICODE_STRING UniSource,
    IN BOOLEAN  AllocateDestinationString)
{
    NTSTATUS Status;
    ULONG Length;
    ULONG Index;
    PAGED_CODE_RTL();

    Length = RtlUnicodeStringToOemSize(UniSource);
    if (Length > MAXUSHORT) return STATUS_INVALID_PARAMETER_2;

    OemDest->Length = (USHORT)Length - sizeof(CHAR);

    if (AllocateDestinationString)
    {
        OemDest->Buffer = RtlpAllocateStringMemory(Length, TAG_OSTR);
        OemDest->MaximumLength = (USHORT)Length;
        if (!OemDest->Buffer) return STATUS_NO_MEMORY;
    }
    else if (OemDest->Length >= OemDest->MaximumLength)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    Status = RtlUpcaseUnicodeToOemN(OemDest->Buffer,
                                    OemDest->Length,
                                    &Index,
                                    UniSource->Buffer,
                                    UniSource->Length);

    /* Check for unmapped characters */
    if (NT_SUCCESS(Status) && !RtlpDidUnicodeToOemWork(UniSource, OemDest))
        Status = STATUS_UNMAPPABLE_CHARACTER;

    if (!NT_SUCCESS(Status) && AllocateDestinationString)
    {
        RtlpFreeStringMemory(OemDest->Buffer, TAG_OSTR);
        OemDest->Buffer = NULL;
        return Status;
    }

    OemDest->Buffer[Index] = ANSI_NULL;
    return Status;
}

/*
 * @implemented
 *
 * RETURNS
 *  Bytes calculated including nullterm
 */
ULONG
NTAPI
RtlxOemStringToUnicodeSize(IN PCOEM_STRING OemString)
{
    ULONG Size;

    /* Convert the Mb String to Unicode Size */
    RtlMultiByteToUnicodeSize(&Size,
                              OemString->Buffer,
                              OemString->Length);

    /* Return the size + null-char */
    return (Size + sizeof(WCHAR));
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlStringFromGUID (IN REFGUID Guid,
                   OUT PUNICODE_STRING GuidString)
{
    /* Setup the string */
    GuidString->Length = 38 * sizeof(WCHAR);
    GuidString->MaximumLength = GuidString->Length + sizeof(UNICODE_NULL);
    GuidString->Buffer = RtlpAllocateStringMemory(GuidString->MaximumLength,
                                                  TAG_USTR);
    if (!GuidString->Buffer) return STATUS_NO_MEMORY;

    /* Now format the GUID */
    swprintf(GuidString->Buffer,
             L"{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
             Guid->Data1,
             Guid->Data2,
             Guid->Data3,
             Guid->Data4[0],
             Guid->Data4[1],
             Guid->Data4[2],
             Guid->Data4[3],
             Guid->Data4[4],
             Guid->Data4[5],
             Guid->Data4[6],
             Guid->Data4[7]);
    return STATUS_SUCCESS;
}

/*
 * @implemented
 *
 * RETURNS
 *  Bytes calculated including nullterm
 */
ULONG
NTAPI
RtlxUnicodeStringToAnsiSize(IN PCUNICODE_STRING UnicodeString)
{
    ULONG Size;
    PAGED_CODE_RTL();

    ASSERT(!(UnicodeString->Length & 1));

    /* Convert the Unicode String to Mb Size */
    RtlUnicodeToMultiByteSize(&Size,
                              UnicodeString->Buffer,
                              UnicodeString->Length);

    /* Return the size + null-char */
    return (Size + sizeof(CHAR));
}

/*
 * @implemented
 */
LONG
NTAPI
RtlCompareUnicodeString(
    IN PCUNICODE_STRING s1,
    IN PCUNICODE_STRING s2,
    IN BOOLEAN  CaseInsensitive)
{
    unsigned int len;
    LONG ret = 0;
    LPCWSTR p1, p2;

    len = min(s1->Length, s2->Length) / sizeof(WCHAR);
    p1 = s1->Buffer;
    p2 = s2->Buffer;

    if (CaseInsensitive)
    {
        while (!ret && len--) ret = RtlpUpcaseUnicodeChar(*p1++) - RtlpUpcaseUnicodeChar(*p2++);
    }
    else
    {
        while (!ret && len--) ret = *p1++ - *p2++;
    }

    if (!ret) ret = s1->Length - s2->Length;

    return ret;
}

/*
 * @implemented
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
LONG
NTAPI
RtlCompareUnicodeStrings(
    _In_reads_(String1Length) PCWCH String1,
    _In_ SIZE_T String1Length,
    _In_reads_(String2Length) PCWCH String2,
    _In_ SIZE_T String2Length,
    _In_ BOOLEAN CaseInSensitive)
{
    LONG Result = 0;
    SIZE_T MinStringLength = min(String1Length, String2Length);
    SIZE_T Index;

    if (CaseInSensitive)
    {
        for (Index = 0; Index < MinStringLength; Index++)
        {
            WCHAR Char1 = RtlpUpcaseUnicodeChar(String1[Index]);
            WCHAR Char2 = RtlpUpcaseUnicodeChar(String2[Index]);
            Result = Char1 - Char2;
            if (Result != 0)
            {
                return Result;
            }
        }
    }
    else
    {
        for (Index = 0; Index < MinStringLength; Index++)
        {
            Result = String1[Index] - String2[Index];
            if (Result != 0)
            {
                return Result;
            }
        }
    }

    return String1Length - String2Length;
}

/*
 * @implemented
 */
VOID
NTAPI
RtlCopyString(
    IN OUT PSTRING DestinationString,
    IN const STRING *SourceString OPTIONAL)
{
    ULONG SourceLength;
    PCHAR p1, p2;

    /* Check if there was no source given */
    if (!SourceString)
    {
        /* Simply return an empty string */
        DestinationString->Length = 0;
    }
    else
    {
        /* Choose the smallest length */
        SourceLength = min(DestinationString->MaximumLength,
                           SourceString->Length);

        /* Set it */
        DestinationString->Length = (USHORT)SourceLength;

        /* Save the pointers to each buffer */
        p1 = DestinationString->Buffer;
        p2 = SourceString->Buffer;

        /* Loop the buffer */
        while (SourceLength)
        {
            /* Copy the character and move on */
            *p1++ = * p2++;
            SourceLength--;
        }
    }
}

/*
 * @implemented
 */
VOID
NTAPI
RtlCopyUnicodeString(
    IN OUT PUNICODE_STRING DestinationString,
    IN PCUNICODE_STRING SourceString)
{
    ULONG SourceLength;

    if (!SourceString)
    {
        DestinationString->Length = 0;
    }
    else
    {
        SourceLength = min(DestinationString->MaximumLength,
                           SourceString->Length);
        DestinationString->Length = (USHORT)SourceLength;

        RtlCopyMemory(DestinationString->Buffer,
                      SourceString->Buffer,
                      SourceLength);

        if (DestinationString->Length < DestinationString->MaximumLength)
        {
            DestinationString->Buffer[SourceLength / sizeof(WCHAR)] = UNICODE_NULL;
        }
    }
}

/*
 * @implemented
 *
 * NOTES
 * Creates a nullterminated UNICODE_STRING
 */
BOOLEAN
NTAPI
RtlCreateUnicodeString(
    IN OUT PUNICODE_STRING UniDest,
    IN PCWSTR  Source)
{
    SIZE_T Size;
    PAGED_CODE_RTL();

    Size = (wcslen(Source) + 1) * sizeof(WCHAR);
    if (Size > MAXUSHORT) return FALSE;

    UniDest->Buffer = RtlpAllocateStringMemory((ULONG)Size, TAG_USTR);

    if (UniDest->Buffer == NULL) return FALSE;

    RtlCopyMemory(UniDest->Buffer, Source, Size);
    UniDest->MaximumLength = (USHORT)Size;
    UniDest->Length = (USHORT)Size - sizeof (WCHAR);

    return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlCreateUnicodeStringFromAsciiz(
    OUT PUNICODE_STRING Destination,
    IN PCSZ Source)
{
    ANSI_STRING AnsiString;
    NTSTATUS Status;

    RtlInitAnsiString(&AnsiString, Source);

    Status = RtlAnsiStringToUnicodeString(Destination,
                                          &AnsiString,
                                          TRUE);

    return NT_SUCCESS(Status);
}

/*
 * @implemented
 *
 * NOTES
 *  Dest is never '\0' terminated because it may be equal to src, and src
 *  might not be '\0' terminated.
 *  Dest->Length is only set upon success.
 */
NTSTATUS
NTAPI
RtlDowncaseUnicodeString(
    IN OUT PUNICODE_STRING UniDest,
    IN PCUNICODE_STRING UniSource,
    IN BOOLEAN AllocateDestinationString)
{
    ULONG i;
    ULONG StopGap;
    PAGED_CODE_RTL();

    if (AllocateDestinationString)
    {
        UniDest->MaximumLength = UniSource->Length;
        UniDest->Buffer = RtlpAllocateStringMemory(UniSource->Length, TAG_USTR);
        if (UniDest->Buffer == NULL) return STATUS_NO_MEMORY;
    }
    else if (UniSource->Length > UniDest->MaximumLength)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    UniDest->Length = UniSource->Length;
    StopGap = UniSource->Length / sizeof(WCHAR);

    for (i = 0 ; i < StopGap; i++)
    {
        if (UniSource->Buffer[i] < L'A')
        {
            UniDest->Buffer[i] = UniSource->Buffer[i];
        }
        else if (UniSource->Buffer[i] <= L'Z')
        {
            UniDest->Buffer[i] = (UniSource->Buffer[i] + (L'a' - L'A'));
        }
        else
        {
            UniDest->Buffer[i] = RtlpDowncaseUnicodeChar(UniSource->Buffer[i]);
        }
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 *
 * NOTES
 *  if src is NULL dest is unchanged.
 *  dest is '\0' terminated when the MaximumLength allowes it.
 *  When dest fits exactly in MaximumLength characters the '\0' is ommitted.
 */
NTSTATUS
NTAPI
RtlAppendUnicodeToString(IN OUT PUNICODE_STRING Destination,
                         IN PCWSTR Source)
{
    USHORT Length;
    PWCHAR DestBuffer;

    if (Source)
    {
        UNICODE_STRING UnicodeSource;

        RtlInitUnicodeString(&UnicodeSource, Source);
        Length = UnicodeSource.Length;

        if (Destination->Length + Length > Destination->MaximumLength)
        {
            return STATUS_BUFFER_TOO_SMALL;
        }

        DestBuffer = &Destination->Buffer[Destination->Length / sizeof(WCHAR)];
        RtlMoveMemory(DestBuffer, Source, Length);
        Destination->Length += Length;

        /* append terminating '\0' if enough space */
        if (Destination->MaximumLength > Destination->Length)
        {
            DestBuffer[Length / sizeof(WCHAR)] = UNICODE_NULL;
        }
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 *
 * NOTES
 *  if src is NULL dest is unchanged.
 *  dest is never '\0' terminated.
 */
NTSTATUS
NTAPI
RtlAppendAsciizToString(
    IN OUT   PSTRING  Destination,
    IN PCSZ  Source)
{
    SIZE_T Size;

    if (Source)
    {
        Size = strlen(Source);

        if (Destination->Length + Size > Destination->MaximumLength)
        {
            return STATUS_BUFFER_TOO_SMALL;
        }

        RtlMoveMemory(&Destination->Buffer[Destination->Length], Source, Size);
        Destination->Length += (USHORT)Size;
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
NTAPI
RtlUpperString(PSTRING DestinationString,
               const STRING *SourceString)
{
    USHORT Length;
    PCHAR Src, Dest;

    Length = min(SourceString->Length,
                 DestinationString->MaximumLength);

    Src = SourceString->Buffer;
    Dest = DestinationString->Buffer;
    DestinationString->Length = Length;

    while (Length)
    {
        *Dest++ = RtlUpperChar(*Src++);
        Length--;
    }
}

/*
 * @implemented
 *
 * NOTES
 *  See RtlpDuplicateUnicodeString
 */
NTSTATUS
NTAPI
RtlDuplicateUnicodeString(
    IN ULONG Flags,
    IN PCUNICODE_STRING SourceString,
    OUT PUNICODE_STRING DestinationString)
{
    PAGED_CODE_RTL();

    if (SourceString == NULL || DestinationString == NULL ||
        SourceString->Length > SourceString->MaximumLength ||
        (SourceString->Length == 0 && SourceString->MaximumLength > 0 && SourceString->Buffer == NULL) ||
        Flags == RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING || Flags >= 4)
    {
        return STATUS_INVALID_PARAMETER;
    }


    if ((SourceString->Length == 0) &&
        (Flags != (RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE |
                   RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING)))
    {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
        DestinationString->Buffer = NULL;
    }
    else
    {
        UINT DestMaxLength = SourceString->Length;

        if (Flags & RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE)
            DestMaxLength += sizeof(UNICODE_NULL);

        DestinationString->Buffer = RtlpAllocateStringMemory(DestMaxLength, TAG_USTR);

        if (DestinationString->Buffer == NULL)
            return STATUS_NO_MEMORY;

        RtlCopyMemory(DestinationString->Buffer, SourceString->Buffer, SourceString->Length);
        DestinationString->Length = SourceString->Length;
        DestinationString->MaximumLength = DestMaxLength;

        if (Flags & RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE)
            DestinationString->Buffer[DestinationString->Length / sizeof(WCHAR)] = 0;
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlValidateUnicodeString(
    _In_ ULONG Flags,
    _In_ PCUNICODE_STRING String)
{
    /* In Windows <= 2003 no flags are supported yet! */
    if (Flags != 0)
        return STATUS_INVALID_PARAMETER;

    /* NOTE: a NULL Unicode string pointer is considered to be a valid one! */
    if (String == NULL)
    {
        return STATUS_SUCCESS;
    }
    else if (!((String->Buffer == NULL) && (String->Length != 0 || String->MaximumLength != 0)) &&
              (String->Length % sizeof(WCHAR) == 0) &&
              (String->MaximumLength % sizeof(WCHAR) == 0) &&
              (String->Length <= String->MaximumLength))
    {
        return STATUS_SUCCESS;
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlpEnsureBufferSize(
    IN ULONG Flags,
    IN OUT PRTL_BUFFER Buffer,
    IN SIZE_T RequiredSize)
{
    PUCHAR NewBuffer;

    /* Parameter checks */
    if (Flags & ~RTL_SKIP_BUFFER_COPY)
        return STATUS_INVALID_PARAMETER;
    if (Buffer == NULL)
        return STATUS_INVALID_PARAMETER;

    /*
     * We don't need to grow the buffer if its size
     * is already larger than the required size.
     */
    if (Buffer->Size >= RequiredSize)
        return STATUS_SUCCESS;

    /*
     * When we are using the static buffer as our buffer, we don't need
     * to grow it if its size is already larger than the required size.
     * In this case, just keep it but update the current buffer size to
     * the one requested.
     * (But NEVER EVER modify the size of the static buffer!!)
     * Otherwise, we'll need to create a new buffer and use this one instead.
     */
    if ( (Buffer->Buffer == Buffer->StaticBuffer) &&
         (Buffer->StaticSize >= RequiredSize) )
    {
        Buffer->Size = RequiredSize;
        return STATUS_SUCCESS;
    }

    /* The buffer we are using is not large enough, try to create a bigger one */
    NewBuffer = RtlpAllocateStringMemory(RequiredSize, TAG_USTR);
    if (NewBuffer == NULL)
        return STATUS_NO_MEMORY;

    /* Copy the original content if needed */
    if (!(Flags & RTL_SKIP_BUFFER_COPY))
    {
        RtlMoveMemory(NewBuffer, Buffer->Buffer, Buffer->Size);
    }

    /* Free the original buffer only if it's not the static buffer */
    if (Buffer->Buffer != Buffer->StaticBuffer)
    {
        RtlpFreeStringMemory(Buffer->Buffer, TAG_USTR);
    }

    /* Update the members */
    Buffer->Buffer = NewBuffer;
    Buffer->Size   = RequiredSize;

    /* Done */
    return STATUS_SUCCESS;
}

static
BOOLEAN
RtlpIsCharInUnicodeString(
    IN WCHAR Char,
    IN PCUNICODE_STRING MatchString,
    IN BOOLEAN CaseInSensitive)
{
    USHORT i;

    if (CaseInSensitive)
        Char = RtlpUpcaseUnicodeChar(Char);

    for (i = 0; i < MatchString->Length / sizeof(WCHAR); i++)
    {
        WCHAR OtherChar = MatchString->Buffer[i];
        if (CaseInSensitive)
            OtherChar = RtlpUpcaseUnicodeChar(OtherChar);

        if (Char == OtherChar)
            return TRUE;
    }

    return FALSE;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlFindCharInUnicodeString(
    IN ULONG Flags,
    IN PCUNICODE_STRING SearchString,
    IN PCUNICODE_STRING MatchString,
    OUT PUSHORT Position)
{
    BOOLEAN Found;
    const BOOLEAN WantToFind = (Flags & RTL_FIND_CHAR_IN_UNICODE_STRING_COMPLEMENT_CHAR_SET) == 0;
    const BOOLEAN CaseInSensitive = (Flags & RTL_FIND_CHAR_IN_UNICODE_STRING_CASE_INSENSITIVE) != 0;
    USHORT i, Length;

    DPRINT("RtlFindCharInUnicodeString(%u, '%wZ', '%wZ', %p)\n",
           Flags, SearchString, MatchString, Position);

    /* Parameter checks */
    if (Position == NULL)
        return STATUS_INVALID_PARAMETER;

    *Position = 0;

    if (Flags & ~(RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END |
                  RTL_FIND_CHAR_IN_UNICODE_STRING_COMPLEMENT_CHAR_SET |
                  RTL_FIND_CHAR_IN_UNICODE_STRING_CASE_INSENSITIVE))
        return STATUS_INVALID_PARAMETER;

    /* Search */
    Length = SearchString->Length / sizeof(WCHAR);
    if (Flags & RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END)
    {
        for (i = Length - 1; (SHORT)i >= 0; i--)
        {
            Found = RtlpIsCharInUnicodeString(SearchString->Buffer[i], MatchString, CaseInSensitive);
            if (Found == WantToFind)
            {
                *Position = i * sizeof(WCHAR);
                return STATUS_SUCCESS;
            }
        }
    }
    else
    {
        for (i = 0; i < Length; i++)
        {
            Found = RtlpIsCharInUnicodeString(SearchString->Buffer[i], MatchString, CaseInSensitive);
            if (Found == WantToFind)
            {
                *Position = (i + 1) * sizeof(WCHAR);
                return STATUS_SUCCESS;
            }
        }
    }

    return STATUS_NOT_FOUND;
}

/*
 * @implemented
 *
 * NOTES
 *  Get the maximum of MAX_COMPUTERNAME_LENGTH characters from the dns.host name until the dot is found.
 *  Convert is to an uppercase oem string and check for unmapped characters.
 *  Then convert the oem string back to an unicode string.
 */
NTSTATUS
NTAPI
RtlDnsHostNameToComputerName(PUNICODE_STRING ComputerName, PUNICODE_STRING DnsHostName, BOOLEAN AllocateComputerNameString)
{
    NTSTATUS Status;
    ULONG Length;
    ULONG ComputerNameLength;
    ULONG ComputerNameOemNLength;
    OEM_STRING ComputerNameOem;
    CHAR ComputerNameOemN[MAX_COMPUTERNAME_LENGTH + 1];

    Status = STATUS_INVALID_COMPUTER_NAME;
    ComputerNameLength = DnsHostName->Length;

    /* find the first dot in the dns host name */
    for (Length = 0; Length < DnsHostName->Length / sizeof(WCHAR); Length++)
    {
        if (DnsHostName->Buffer[Length] == L'.')
        {
            /* dot found, so set the length for the oem translation */
            ComputerNameLength = Length * sizeof(WCHAR);
            break;
        }
    }

    /* the computername must have one character */
    if (ComputerNameLength > 0)
    {
        ComputerNameOemNLength = 0;
        /* convert to oem string and use uppercase letters */
        Status = RtlUpcaseUnicodeToOemN(ComputerNameOemN,
                                        MAX_COMPUTERNAME_LENGTH,
                                        &ComputerNameOemNLength,
                                        DnsHostName->Buffer,
                                        ComputerNameLength);

        /* status STATUS_BUFFER_OVERFLOW is not a problem since the computername shoud only
           have MAX_COMPUTERNAME_LENGTH characters */
        if ((Status == STATUS_SUCCESS) ||
                (Status == STATUS_BUFFER_OVERFLOW))
        {
            /* set the termination for the oem string */
            ComputerNameOemN[MAX_COMPUTERNAME_LENGTH] = 0;
            /* set status for the case the next function failed */
            Status = STATUS_INVALID_COMPUTER_NAME;
            /* fillup the oem string structure with the converted computername
               and check it for unmapped characters */
            ComputerNameOem.Buffer = ComputerNameOemN;
            ComputerNameOem.Length = (USHORT)ComputerNameOemNLength;
            ComputerNameOem.MaximumLength = (USHORT)(MAX_COMPUTERNAME_LENGTH + 1);

            if (RtlpDidUnicodeToOemWork(DnsHostName, &ComputerNameOem))
            {
                /* no unmapped character so convert it back to an unicode string */
                Status = RtlOemStringToUnicodeString(ComputerName,
                                                     &ComputerNameOem,
                                                     AllocateComputerNameString);
            }
        }
    }

    return Status;
}

