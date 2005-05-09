/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/fs/name.c
 * PURPOSE:         Name and DBCS Name Validation and Dissection Functions.
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Eric Kohl
 *                  Filip Navara
 */

 /* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

static UCHAR LegalAnsiCharacterArray[] =
{
  0,                                                        /* CTRL+@, 0x00 */
  0,                                                        /* CTRL+A, 0x01 */
  0,                                                        /* CTRL+B, 0x02 */
  0,                                                        /* CTRL+C, 0x03 */
  0,                                                        /* CTRL+D, 0x04 */
  0,                                                        /* CTRL+E, 0x05 */
  0,                                                        /* CTRL+F, 0x06 */
  0,                                                        /* CTRL+G, 0x07 */
  0,                                                        /* CTRL+H, 0x08 */
  0,                                                        /* CTRL+I, 0x09 */
  0,                                                        /* CTRL+J, 0x0a */
  0,                                                        /* CTRL+K, 0x0b */
  0,                                                        /* CTRL+L, 0x0c */
  0,                                                        /* CTRL+M, 0x0d */
  0,                                                        /* CTRL+N, 0x0e */
  0,                                                        /* CTRL+O, 0x0f */
  0,                                                        /* CTRL+P, 0x10 */
  0,                                                        /* CTRL+Q, 0x11 */
  0,                                                        /* CTRL+R, 0x12 */
  0,                                                        /* CTRL+S, 0x13 */
  0,                                                        /* CTRL+T, 0x14 */
  0,                                                        /* CTRL+U, 0x15 */
  0,                                                        /* CTRL+V, 0x16 */
  0,                                                        /* CTRL+W, 0x17 */
  0,                                                        /* CTRL+X, 0x18 */
  0,                                                        /* CTRL+Y, 0x19 */
  0,                                                        /* CTRL+Z, 0x1a */
  0,                                                        /* CTRL+[, 0x1b */
  0,                                                        /* CTRL+\, 0x1c */
  0,                                                        /* CTRL+], 0x1d */
  0,                                                        /* CTRL+^, 0x1e */
  0,                                                        /* CTRL+_, 0x1f */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* ` ',    0x20 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `!',    0x21 */
  FSRTL_WILD_CHARACTER,                                     /* `"',    0x22 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `#',    0x23 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `$',    0x24 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `%',    0x25 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `&',    0x26 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `'',    0x27 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `(',    0x28 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `)',    0x29 */
  FSRTL_WILD_CHARACTER,                                     /* `*',    0x2a */
  FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `+',    0x2b */
  FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `,',    0x2c */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `-',    0x2d */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `.',    0x2e */
  0,                                                        /* `/',    0x2f */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `0',    0x30 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `1',    0x31 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `2',    0x32 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `3',    0x33 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `4',    0x34 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `5',    0x35 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `6',    0x36 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `7',    0x37 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `8',    0x38 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `9',    0x39 */
  FSRTL_NTFS_LEGAL,                                         /* `:',    0x3a */
  FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `;',    0x3b */
  FSRTL_WILD_CHARACTER,                                     /* `<',    0x3c */
  FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `=',    0x3d */
  FSRTL_WILD_CHARACTER,                                     /* `>',    0x3e */
  FSRTL_WILD_CHARACTER,                                     /* `?',    0x3f */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `@',    0x40 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `A',    0x41 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `B',    0x42 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `C',    0x43 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `D',    0x44 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `E',    0x45 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `F',    0x46 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `G',    0x47 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `H',    0x48 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `I',    0x49 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `J',    0x4a */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `K',    0x4b */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `L',    0x4c */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `M',    0x4d */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `N',    0x4e */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `O',    0x4f */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `P',    0x50 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `Q',    0x51 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `R',    0x52 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `S',    0x53 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `T',    0x54 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `U',    0x55 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `V',    0x56 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `W',    0x57 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `X',    0x58 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `Y',    0x59 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `Z',    0x5a */
  FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `[',    0x5b */
  0,                                                        /* `\',    0x5c */
  FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,                      /* `]',    0x5d */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `^',    0x5e */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `_',    0x5f */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* ``',    0x60 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `a',    0x61 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `b',    0x62 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `c',    0x63 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `d',    0x64 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `e',    0x65 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `f',    0x66 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `g',    0x67 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `h',    0x68 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `i',    0x69 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `j',    0x6a */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `k',    0x6b */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `l',    0x6c */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `m',    0x6d */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `n',    0x6e */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `o',    0x6f */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `p',    0x70 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `q',    0x71 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `r',    0x72 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `s',    0x73 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `t',    0x74 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `u',    0x75 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `v',    0x76 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `w',    0x77 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `x',    0x78 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `y',    0x79 */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `z',    0x7a */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `{',    0x7b */
  0,                                                        /* `|',    0x7c */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `}',    0x7d */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL,    /* `~',    0x7e */
  FSRTL_FAT_LEGAL | FSRTL_HPFS_LEGAL | FSRTL_NTFS_LEGAL     /*         0x7f */
};

PUCHAR EXPORTED FsRtlLegalAnsiCharacterArray = LegalAnsiCharacterArray;

/* FUNCTIONS *****************************************************************/

/*
 * NAME    EXPORTED
 *  FsRtlAreNamesEqual@16
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * NOTE
 *  From Bo Branten's ntifs.h v25.
 *
 * @implemented
 */
BOOLEAN
STDCALL
FsRtlAreNamesEqual(IN PUNICODE_STRING Name1,
                   IN PUNICODE_STRING Name2,
                   IN BOOLEAN IgnoreCase,
                   IN PWCHAR UpcaseTable OPTIONAL)
{
    UNICODE_STRING UpcaseName1;
    UNICODE_STRING UpcaseName2;
    BOOLEAN StringsAreEqual;

    /* Well, first check their size */
    if (Name1->Length != Name2->Length) {
        /* Not equal! */
        return FALSE;
    }

    /* Turn them into Upcase if we don't have a table */
    if (IgnoreCase && !UpcaseTable) {
        RtlUpcaseUnicodeString(&UpcaseName1, Name1, TRUE);
        RtlUpcaseUnicodeString(&UpcaseName2, Name2, TRUE);
        Name1 = &UpcaseName1;
        Name2 = &UpcaseName2;

        goto ManualCase;
    }

    /* Do a case-sensitive search */
    if (!IgnoreCase) {

ManualCase:
        /* Use a raw memory compare */
        StringsAreEqual = RtlEqualMemory(Name1->Buffer,
                                         Name2->Buffer,
                                         Name1->Length);

        /* Clear the strings if we need to */
        if (IgnoreCase) {
            RtlFreeUnicodeString(&UpcaseName1);
            RtlFreeUnicodeString(&UpcaseName2);
        }

        /* Return the equality */
        return StringsAreEqual;

    } else {

        /* Case in-sensitive search */

        LONG i;

        for (i = Name1->Length / sizeof(WCHAR) - 1; i >= 0; i--) {

            if (UpcaseTable[Name1->Buffer[i]] != UpcaseTable[Name2->Buffer[i]]) {

                /* Non-match found! */
                return FALSE;
            }
        }

        /* We finished the loop so we are equal */
        return TRUE;
    }
}

/*
 * NAME    EXPORTED
 *  FsRtlDissectDbcs@16
 *
 * Dissects a given path name into first and remaining part.
 *
 * ARGUMENTS
 * Name
 *    ANSI string to dissect.
 *
 * FirstPart
 *    Pointer to user supplied ANSI_STRING, that will
 *    later point to the first part of the original name.
 *
 * RemainingPart
 *    Pointer to user supplied ANSI_STRING, that will
 *    later point to the remaining part of the original name.
 *
 * RETURN VALUE
 *  None
 *
 * EXAMPLE
 *  Name:           \test1\test2\test3
 *  FirstPart:      test1
 *  RemainingPart:  test2\test3
 *
 * @implemented
 */
VOID
STDCALL
FsRtlDissectDbcs(IN ANSI_STRING Name,
                 OUT PANSI_STRING FirstPart,
                 OUT PANSI_STRING RemainingPart)
{
    ULONG i;
    ULONG FirstLoop;

    /* Initialize the Outputs */
    RtlZeroMemory(&FirstPart, sizeof(ANSI_STRING));
    RtlZeroMemory(&RemainingPart, sizeof(ANSI_STRING));

    /* Bail out if empty */
    if (!Name.Length) return;

    /* Ignore backslash */
    if (Name.Buffer[0] == '\\') {
        i = 1;
    } else {
        i = 0;
    }

    /* Loop until we find a backslash */
    for (FirstLoop = i;i < Name.Length;i++) {
        if (Name.Buffer[i] != '\\') break;
        if (FsRtlIsLeadDbcsCharacter(Name.Buffer[i])) i++;
    }

    /* Now we have the First Part */
    FirstPart->Length = (i-FirstLoop);
    FirstPart->MaximumLength = FirstPart->Length; /* +2?? */
    FirstPart->Buffer = &Name.Buffer[FirstLoop];

    /* Make the second part if something is still left */
    if (i<Name.Length) {
        RemainingPart->Length = (Name.Length - (i+1));
        RemainingPart->MaximumLength = RemainingPart->Length; /* +2?? */
        RemainingPart->Buffer = &Name.Buffer[i+1];
    }

    return;
}

/*
 * NAME    EXPORTED
 *  FsRtlDissectName@16
 *
 * DESCRIPTION
 *  Dissects a given path name into first and remaining part.
 *
 * ARGUMENTS
 *  Name
 *    Unicode string to dissect.
 *
 *  FirstPart
 *    Pointer to user supplied UNICODE_STRING, that will
 *    later point to the first part of the original name.
 *
 *  RemainingPart
 *    Pointer to user supplied UNICODE_STRING, that will
 *    later point to the remaining part of the original name.
 *
 * RETURN VALUE
 *  None
 *
 * EXAMPLE
 *  Name:          \test1\test2\test3
 *  FirstPart:     test1
 *  RemainingPart: test2\test3
 *
 * @implemented
 */
VOID
STDCALL
FsRtlDissectName(IN UNICODE_STRING Name,
                 OUT PUNICODE_STRING FirstPart,
                 OUT PUNICODE_STRING RemainingPart)
{
  USHORT NameOffset = 0;
  USHORT NameLength = 0;
  USHORT Length;

  FirstPart->Length = 0;
  FirstPart->MaximumLength = 0;
  FirstPart->Buffer = NULL;

  RemainingPart->Length = 0;
  RemainingPart->MaximumLength = 0;
  RemainingPart->Buffer = NULL;

  if (Name.Length == 0)
    return;

  /* Skip leading backslash */
  if (Name.Buffer[0] == L'\\')
    NameOffset++;

  Length = Name.Length / sizeof(WCHAR);

  /* Search for next backslash or end-of-string */
  while ((NameOffset + NameLength < Length) &&
         (Name.Buffer[NameOffset + NameLength] != L'\\'))
  {
    NameLength++;
  }

  FirstPart->Length =
  FirstPart->MaximumLength = NameLength * sizeof(WCHAR);
  FirstPart->Buffer = &Name.Buffer[NameOffset];

  NameOffset += NameLength + 1;
  if (NameOffset < Length)
  {
    RemainingPart->Length = (Length - NameOffset) * sizeof(WCHAR);
    RemainingPart->MaximumLength = (Length - NameOffset) * sizeof(WCHAR);
    RemainingPart->Buffer = &Name.Buffer[NameOffset];
  }
}

/*
 * NAME    EXPORTED
 *  FsRtlDoesDbcsContainWildCards@4
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @implemented
 */
BOOLEAN
STDCALL
FsRtlDoesDbcsContainWildCards(IN PANSI_STRING Name)
{
    ULONG i;

    /* Check every character */
    for (i=0;i < Name->Length;i++) {

        /* First make sure it's not the Lead DBCS */
        if (FsRtlIsLeadDbcsCharacter(Name->Buffer[i])) {
            i++;
        } else if (FsRtlIsAnsiCharacterWild(Name->Buffer[i])) {
            /* Now return if it has a Wilcard */
            return TRUE;
        }
    }

    /* We didn't return above...so none found */
    return FALSE;
}

/*
 * NAME    EXPORTED
 *  FsRtlDoesNameContainWildCards@4
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * NOTE
 *  From Bo Branten's ntifs.h v12.
 *
 * @implemented
 */
BOOLEAN
STDCALL
FsRtlDoesNameContainWildCards(IN PUNICODE_STRING Name)
{
    PWCHAR Ptr;

    /* Loop through every character */
    if (Name->Length) {
        for (Ptr = Name->Buffer + (Name->Length / sizeof(WCHAR))-1;
             Ptr >= Name->Buffer && *Ptr != L'\\';Ptr--) {

            /* Check for Wildcard */
            if (FsRtlIsUnicodeCharacterWild(*Ptr)) {
                return TRUE;
            }
        }
    }

    /* Nothing Found */
    return FALSE;
}

/*
 * NAME    EXPORTED
 * FsRtlIsDbcsInExpression@8
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @unimplemented
 */
BOOLEAN
STDCALL
FsRtlIsDbcsInExpression(IN PANSI_STRING Expression,
                        IN PANSI_STRING Name)
{
  return FALSE;
}

/*
 * NAME    EXPORTED
 *  FsRtlIsFatDbcsLegal@20
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @unimplemented
 */
BOOLEAN
STDCALL
FsRtlIsFatDbcsLegal(IN ANSI_STRING DbcsName,
                    IN BOOLEAN WildCardsPermissible,
                    IN BOOLEAN PathNamePermissible,
                    IN BOOLEAN LeadingBackslashPermissible)
{
  return FALSE;
}

/*
 * NAME    EXPORTED
 *  FsRtlIsHpfsDbcsLegal@20
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @unimplemented
 */
BOOLEAN
STDCALL
FsRtlIsHpfsDbcsLegal(IN ANSI_STRING DbcsName,
                     IN BOOLEAN WildCardsPermissible,
                     IN BOOLEAN PathNamePermissible,
                     IN BOOLEAN LeadingBackslashPermissible)
{
  return FALSE;
}

/*
 * NAME    EXPORTED
 *  FsRtlIsNameInExpression@16
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * NOTE
 *  From Bo Branten's ntifs.h v12. This function should be rewritten
 *      to avoid recursion and better wildcard handling should be
 *      implemented (see FsRtlDoesNameContainWildCards).
 *
 * @implemented
 */
BOOLEAN
STDCALL
FsRtlIsNameInExpression(IN PUNICODE_STRING Expression,
                        IN PUNICODE_STRING Name,
                        IN BOOLEAN IgnoreCase,
                        IN PWCHAR UpcaseTable OPTIONAL)
{
  USHORT ExpressionPosition, NamePosition;
  UNICODE_STRING TempExpression, TempName;

  ExpressionPosition = 0;
  NamePosition = 0;
  while (ExpressionPosition < (Expression->Length / sizeof(WCHAR)) &&
         NamePosition < (Name->Length / sizeof(WCHAR)))
    {
      if (Expression->Buffer[ExpressionPosition] == L'*')
        {
          ExpressionPosition++;
          if (ExpressionPosition == (Expression->Length / sizeof(WCHAR)))
            {
              return TRUE;
            }
          while (NamePosition < (Name->Length / sizeof(WCHAR)))
            {
              TempExpression.Length =
              TempExpression.MaximumLength =
                Expression->Length - (ExpressionPosition * sizeof(WCHAR));
              TempExpression.Buffer = Expression->Buffer + ExpressionPosition;
              TempName.Length =
              TempName.MaximumLength =
                Name->Length - (NamePosition * sizeof(WCHAR));
              TempName.Buffer = Name->Buffer + NamePosition;
              /* FIXME: Rewrite to get rid of recursion */
              if (FsRtlIsNameInExpression(&TempExpression, &TempName,
                                          IgnoreCase, UpcaseTable))
                {
                  return TRUE;
                }
              NamePosition++;
            }
        }
      else
        {
          /* FIXME: Take UpcaseTable into account! */
          if (Expression->Buffer[ExpressionPosition] == L'?' ||
              (IgnoreCase &&
               RtlUpcaseUnicodeChar(Expression->Buffer[ExpressionPosition]) ==
               RtlUpcaseUnicodeChar(Name->Buffer[NamePosition])) ||
              (!IgnoreCase &&
               Expression->Buffer[ExpressionPosition] ==
               Name->Buffer[NamePosition]))
            {
              NamePosition++;
              ExpressionPosition++;
            }
          else
            {
              return FALSE;
            }
        }
    }

  /* Handle matching of "f0_*.*" expression to "f0_000" file name. */
  if (ExpressionPosition < (Expression->Length / sizeof(WCHAR)) &&
      Expression->Buffer[ExpressionPosition] == L'.')
    {
      while (ExpressionPosition < (Expression->Length / sizeof(WCHAR)) &&
             (Expression->Buffer[ExpressionPosition] == L'.' ||
              Expression->Buffer[ExpressionPosition] == L'*' ||
              Expression->Buffer[ExpressionPosition] == L'?'))
        {
          ExpressionPosition++;
        }
    }

  if (ExpressionPosition == (Expression->Length / sizeof(WCHAR)) &&
      NamePosition == (Name->Length / sizeof(WCHAR)))
    {
      return TRUE;
    }

  return FALSE;
}

/* EOF */
