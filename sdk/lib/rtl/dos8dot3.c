/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/dos8dot3.c
 * PURPOSE:         Short name (8.3 name) functions
 * PROGRAMMER:      Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

extern PUSHORT NlsUnicodeToMbOemTable;

/* CONSTANTS *****************************************************************/

const ULONG RtlpShortIllegals[] = { 0xFFFFFFFF, 0xFC009C04, 0x38000000, 0x10000000 };

/* FUNCTIONS *****************************************************************/

BOOLEAN
NTAPI
RtlIsValidOemCharacter(IN PWCHAR Char);

static BOOLEAN
RtlpIsShortIllegal(const WCHAR Char)
{
    return (Char < 128 && (RtlpShortIllegals[Char / 32] & (1 << (Char % 32))));
}

static USHORT
RtlpGetCheckSum(PUNICODE_STRING Name)
{
    PWCHAR CurrentChar;
    USHORT Hash;
    USHORT Saved;
    USHORT Length;

    if (!Name->Length)
        return 0;

    if (Name->Length == sizeof(WCHAR))
        return Name->Buffer[0];

    CurrentChar = Name->Buffer;
    Hash = (*CurrentChar << 8) + *(CurrentChar + 1);

    if (Name->Length == 2 * sizeof(WCHAR))
        return Hash;

    Saved = Hash;
    Length = 2;

    do
    {
        CurrentChar += 2;
        Hash = (Hash << 7) + *CurrentChar;
        Hash = (Saved >> 1) + (Hash << 8);

        if (Length + 1 < Name->Length / sizeof(WCHAR))
        {
            Hash += *(CurrentChar + 1);
        }

        Saved = Hash;
        Length += 2;
    }
    while (Length < Name->Length / sizeof(WCHAR));

    return Hash;
}

/*
 * @implemented
 */
VOID
NTAPI
RtlGenerate8dot3Name(IN PUNICODE_STRING Name,
                     IN BOOLEAN AllowExtendedCharacters,
                     IN OUT PGENERATE_NAME_CONTEXT Context,
                     OUT PUNICODE_STRING Name8dot3)
{
    ULONG Length = Name->Length / sizeof(WCHAR);
    ULONG IndexLength;
    ULONG Index;
    ULONG DotPos;
    WCHAR IndexBuffer[8];
    WCHAR Char;
    USHORT Checksum;

    if (!Context->NameLength)
    {
        DotPos = Length;

        /* Find last dot in Name */
        for (Index = 0; Index < Length; Index++)
        {
            if (Name->Buffer[Index] == L'.')
                DotPos = Index;
        }

        /* Copy name. OEM string length can't exceed 6. */
        UCHAR OemSizeLeft = 6;
        for (Index = 0; (Index < DotPos) && OemSizeLeft; Index++)
        {
            Char = Name->Buffer[Index];

            if ((Char > L' ') && (Char != L'.') &&
                ((Char < 127) || (AllowExtendedCharacters && RtlIsValidOemCharacter(&Char))))
            {
                if (RtlpIsShortIllegal(Char))
                    Char = L'_';
                else if (Char >= L'a' && Char <= L'z')
                    Char = RtlpUpcaseUnicodeChar(Char);

                /* Beware of MB OEM codepage */
                if (NlsMbOemCodePageTag && HIBYTE(NlsUnicodeToMbOemTable[Char]))
                {
                    if (OemSizeLeft < 2)
                        break;
                    OemSizeLeft--;
                }

                Context->NameBuffer[Context->NameLength] = Char;
                Context->NameLength++;
                OemSizeLeft--;
            }
        }

        /* Copy extension (4 valid characters max) */
        Context->ExtensionLength = 0;
        if (DotPos < Length)
        {
            Context->ExtensionBuffer[0] = L'.';
            Context->ExtensionLength = 1;

            while (DotPos < Length && Context->ExtensionLength < 4)
            {
                Char = Name->Buffer[DotPos];

                if ((Char > L' ') && (Char != L'.') &&
                    ((Char < 127) || (AllowExtendedCharacters && RtlIsValidOemCharacter(&Char))))
                {
                    if (RtlpIsShortIllegal(Char))
                        Char = L'_';
                    else if (Char >= L'a' && Char <= L'z')
                        Char = RtlpUpcaseUnicodeChar(Char);

                    Context->ExtensionBuffer[Context->ExtensionLength++] = Char;
                }

                Char = UNICODE_NULL;
                ++DotPos;
            }

            if (Char != UNICODE_NULL)
                Context->ExtensionBuffer[Context->ExtensionLength - 1] = L'~';
        }

        if (Context->NameLength <= 2)
        {
            Checksum = Context->Checksum = RtlpGetCheckSum(Name);

            for (Index = 0; Index < 4; Index++)
            {
                Context->NameBuffer[Context->NameLength + Index] =
                    (Checksum & 0xF) > 9 ? (Checksum & 0xF) + L'A' - 10 : (Checksum & 0xF) + L'0';
                Checksum >>= 4;
            }

            Context->CheckSumInserted = TRUE;
            Context->NameLength += 4;
        }
    }

    ++Context->LastIndexValue;

    if (Context->LastIndexValue > 4 && !Context->CheckSumInserted)
    {
        Checksum = Context->Checksum = RtlpGetCheckSum(Name);

        for (Index = 2; Index < 6; Index++)
        {
            Context->NameBuffer[Index] =
                (Checksum & 0xF) > 9 ? (Checksum & 0xF) + L'A' - 10 : (Checksum & 0xF) + L'0';
            Checksum >>= 4;
        }

        Context->CheckSumInserted = TRUE;
        Context->NameLength = 6;
        Context->LastIndexValue = 1;
    }

    /* Calculate index length and index buffer */
    Index = Context->LastIndexValue;
    for (IndexLength = 1; IndexLength <= 7 && Index > 0; IndexLength++)
    {
        IndexBuffer[8 - IndexLength] = L'0' + (Index % 10);
        Index /= 10;
    }

    IndexBuffer[8 - IndexLength] = L'~';

    /* Reset name length */
    Name8dot3->Length = 0;

    /* If name present */
    if (Context->NameLength)
    {
        /* Copy name buffer */
        Length = Context->NameLength * sizeof(WCHAR);
        RtlCopyMemory(Name8dot3->Buffer, Context->NameBuffer, Length);
        Name8dot3->Length = Length;
    }

    /* Copy index buffer */
    Length = IndexLength * sizeof(WCHAR);
    RtlCopyMemory(Name8dot3->Buffer + (Name8dot3->Length / sizeof(WCHAR)),
                  IndexBuffer + (8 - IndexLength),
                  Length);
    Name8dot3->Length += Length;

    /* If extension present */
    if (Context->ExtensionLength)
    {
        /* Copy extension buffer */
        Length = Context->ExtensionLength * sizeof(WCHAR);
        RtlCopyMemory(Name8dot3->Buffer + (Name8dot3->Length / sizeof(WCHAR)),
                      Context->ExtensionBuffer,
                      Length);
        Name8dot3->Length += Length;
    }
}


/*
 * @implemented
 * Note: the function does not conform to the annotations.
 * SpacesFound is not always set!
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlIsNameLegalDOS8Dot3(IN PCUNICODE_STRING Name,
                       IN OUT POEM_STRING OemName,
                       OUT PBOOLEAN NameContainsSpaces OPTIONAL)
{
    static const char Illegal[] = "*?<>|\"+=,;[]:/\\\345";
    int Dot = -1;
    int i;
    char Buffer[12];
    OEM_STRING OemString;
    BOOLEAN GotSpace = FALSE;
    NTSTATUS Status;

    if (!OemName)
    {
        OemString.Length = sizeof(Buffer);
        OemString.MaximumLength = sizeof(Buffer);
        OemString.Buffer = Buffer;
        OemName = &OemString;
    }

    Status = RtlUpcaseUnicodeStringToCountedOemString(OemName, Name, FALSE);
    if (!NT_SUCCESS(Status))
        return FALSE;

    if ((OemName->Length > 12) || (OemName->Buffer == NULL)) return FALSE;

    /* a starting . is invalid, except for . and .. */
    if (OemName->Buffer[0] == '.')
    {
        if (OemName->Length != 1 && (OemName->Length != 2 || OemName->Buffer[1] != '.')) return FALSE;
        if (NameContainsSpaces) *NameContainsSpaces = FALSE;

        return TRUE;
    }

    for (i = 0; i < OemName->Length; i++)
    {
        switch (OemName->Buffer[i])
        {
            case ' ':
                /* leading/trailing spaces not allowed */
                if (!i || i == OemName->Length-1 || OemName->Buffer[i+1] == '.') return FALSE;
                GotSpace = TRUE;
                break;

            case '.':
                if (Dot != -1) return FALSE;
                Dot = i;
                break;

            default:
                if (strchr(Illegal, OemName->Buffer[i])) return FALSE;
                break;
        }
    }
    /* check file part is shorter than 8, extension shorter than 3
     * dot cannot be last in string
     */
    if (Dot == -1)
    {
        if (OemName->Length > 8) return FALSE;
    }
    else
    {
        if (Dot > 8 || (OemName->Length - Dot > 4) || Dot == OemName->Length - 1) return FALSE;
    }

    if (NameContainsSpaces) *NameContainsSpaces = GotSpace;

    return TRUE;
}

/* EOF */
