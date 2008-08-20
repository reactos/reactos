/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/name.c
 * PURPOSE:         Provides DBCS parsing and other support routines for FSDs
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Pierre Schweitzer (heis_spiter@hotmail.com) 
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name FsRtlDissectDbcs
 * @implemented
 *
 * Dissects a given path name into first and remaining part.
 *
 * @param Name
 *        ANSI string to dissect.
 *
 * @param FirstPart
 *        Pointer to user supplied ANSI_STRING, that will later point
 *        to the first part of the original name.
 *
 * @param RemainingPart
 *        Pointer to user supplied ANSI_STRING, that will later point
 *        to the remaining part of the original name.
 *
 * @return None
 *
 * @remarks Example:
 *          Name:           \test1\test2\test3
 *          FirstPart:      test1
 *          RemainingPart:  test2\test3
 *
 *--*/
VOID
NTAPI
FsRtlDissectDbcs(IN ANSI_STRING Name,
                 OUT PANSI_STRING FirstPart,
                 OUT PANSI_STRING RemainingPart)
{
    ULONG FirstPosition, i;
    ULONG SkipFirstSlash = 0;

    /* Zero the strings before continuing */
    RtlZeroMemory(FirstPart, sizeof(*FirstPart));
    RtlZeroMemory(RemainingPart, sizeof(*RemainingPart));

    /* Just quit if the string is empty */
    if (!Name.Length) return;

    /* Find first backslash */
    FirstPosition = Name.Length / sizeof(CHAR);
    for (i = 0; i < Name.Length / sizeof(CHAR); i++)
    {
        /* First make sure the character it's not the Lead DBCS */
        if (FsRtlIsLeadDbcsCharacter(Name.Buffer[i]))
        {
            i++;
        }
        /* If we found one... */
        else if (Name.Buffer[i] == '\\')
        {
            /* If it begins string, just notice it and continue */
            if (i == 0)
            {
                SkipFirstSlash = 1;
            }
            else
            {
                /* Else, save its position and break out of the loop */
                FirstPosition = i;
                break;
            }
        }
    }

    /* Set up the first result string */
    FirstPart->Buffer = Name.Buffer + SkipFirstSlash;
    FirstPart->Length = (FirstPosition - SkipFirstSlash) * sizeof(CHAR);
    FirstPart->MaximumLength = FirstPart->Length;

    /* And second one, if necessary */
    if (FirstPosition < (Name.Length / sizeof(CHAR)))
    {
        RemainingPart->Buffer = Name.Buffer + FirstPosition + 1;
        RemainingPart->Length = Name.Length - (FirstPosition + 1) * sizeof(CHAR);
        RemainingPart->MaximumLength = RemainingPart->Length;
    }
}

/*++
 * @name FsRtlDoesDbcsContainWildCards
 * @implemented
 *
 * Returns TRUE if the given DbcsName contains wildcards such as *, ?, 
 * ANSI_DOS_STAR, ANSI_DOS_DOT, and ANSI_DOS_QM
 *
 * @param Name
 *        The Name to check
 *
 * @return TRUE if there are wildcards, FALSE otherwise
 *
 * @remarks None
 *
 *--*/
BOOLEAN
NTAPI
FsRtlDoesDbcsContainWildCards(IN PANSI_STRING Name)
{
    ULONG i;

    /* Check every character */
    for (i = 0; i < Name->Length; i++)
    {
        /* First make sure it's not the Lead DBCS */
        if (FsRtlIsLeadDbcsCharacter(Name->Buffer[i]))
        {
            i++;
        }
        else if (FsRtlIsAnsiCharacterWild(Name->Buffer[i]))
        {
            /* Now return if it has a wildcard */
            return TRUE;
        }
    }

    /* We didn't return above...so none found */
    return FALSE;
}

/*++
 * @name FsRtlIsDbcsInExpression
 * @implemented
 *
 * Check if the Name string is in the Expression string.
 *
 * @param Expression
 *        The string in which we've to find Name. It can contains wildcards
 *
 * @param Name
 *        The string to find. It cannot contain wildcards.
 *
 * @return TRUE if Name is found in Expression, FALSE otherwise
 *
 * @remarks None
 *
 *--*/
BOOLEAN
NTAPI
FsRtlIsDbcsInExpression(IN PANSI_STRING Expression,
                        IN PANSI_STRING Name)
{
    ULONG ExpressionPosition, NamePosition, MatchingChars = 0;

    ASSERT(!FsRtlDoesDbcsContainWildCards(Name));

    /* One can't be null, both can be */
    if (!Expression->Length || !Name->Length)
    {
        return !(Expression->Length ^ Name->Length);
    }

    for (ExpressionPosition = 0; ExpressionPosition < Expression->Length / sizeof(CHAR); ExpressionPosition++)
    {
        if ((Expression->Buffer[ExpressionPosition] == Name->Buffer[MatchingChars]) ||
            (Expression->Buffer[ExpressionPosition] == '?') ||
            (Expression->Buffer[ExpressionPosition] == ANSI_DOS_QM) ||
            (Expression->Buffer[ExpressionPosition] == ANSI_DOS_DOT &&
            (Name->Buffer[MatchingChars] == '.' || Name->Buffer[MatchingChars] == '0')))
        {
            MatchingChars++;
        }
        else if (Expression->Buffer[ExpressionPosition] == '*')
        {
            MatchingChars = Name->Length / sizeof(CHAR);
        }
        else if (Expression->Buffer[ExpressionPosition] == ANSI_DOS_STAR)
        {
            for (NamePosition = MatchingChars; NamePosition < Name->Length / sizeof(CHAR); NamePosition++)
            {
                if (Name->Buffer[NamePosition] == '.')
                {
                    MatchingChars = NamePosition;
                }
            }
        }
        else
        {
            MatchingChars = 0;
        }
        if (MatchingChars == Name->Length / sizeof(CHAR))
        {
            return TRUE;
        }
    }

    return FALSE;
}

/*++
 * @name FsRtlIsFatDbcsLegal
 * @implemented
 *
 * Returns TRUE if the given DbcsName is a valid FAT filename (in 8.3)
 *
 * @param DbcsName
 *        The filename to check. It can also contains pathname.
 *
 * @param WildCardsPermissible
 *        If this is set to FALSE and if filename contains wildcard, the function 
 *        will fail
 *
 * @param PathNamePermissible
 *        If this is set to FALSE and if the filename comes with a pathname, the
 *        function will fail
 *
 * @param LeadingBackslashPermissible
 *        If this is set to FALSE and if the filename starts with a backslash, the
 *        function will fail
 *
 * @return TRUE if the DbcsName is legal, FALSE otherwise
 *
 * @remarks None
 *
 *--*/
BOOLEAN
NTAPI
FsRtlIsFatDbcsLegal(IN ANSI_STRING DbcsName,
                    IN BOOLEAN WildCardsPermissible,
                    IN BOOLEAN PathNamePermissible,
                    IN BOOLEAN LeadingBackslashPermissible)
{
    ANSI_STRING FirstPart, RemainingPart, Name;
    BOOLEAN LastDot;
    ULONG i;

    /* Just quit if the string is empty */
    if (!DbcsName.Length)
        return FALSE;

    /* DbcsName wasn't supposed to be started with \ */
    if (!LeadingBackslashPermissible && DbcsName.Buffer[0] == '\\')
        return FALSE;
    /* DbcsName was allowed to be started with \, but now, remove it */
    else if (LeadingBackslashPermissible && DbcsName.Buffer[0] == '\\')
    {
        DbcsName.Buffer = DbcsName.Buffer + 1;
        DbcsName.Length = DbcsName.Length - 1;
        DbcsName.MaximumLength = DbcsName.MaximumLength - 1;
    }

    /* Extract first part of the DbcsName to work on */
    FsRtlDissectDbcs(DbcsName, &FirstPart, &RemainingPart);
    while (FirstPart.Length > 0)
    {
        /* Reset dots count */
        LastDot = FALSE;

        /* Accept special filename if wildcards are allowed */
        if (WildCardsPermissible && (FirstPart.Length == 1 || FirstPart.Length == 2) && FirstPart.Buffer[0] == '.')
        {
            if (FirstPart.Length == 2)
            {
                if (FirstPart.Buffer[1] == '.')
                {
                    goto EndLoop;
                }
            }
            else
            {
                goto EndLoop;
            }
        }

        /* Filename must be 8.3 filename */
        if (FirstPart.Length / sizeof(CHAR) < 3 || FirstPart.Length / sizeof(CHAR) > 12)
            return FALSE;

        if (!WildCardsPermissible && FsRtlDoesDbcsContainWildCards(&FirstPart))
            return FALSE;

        /* Now, we will parse the filename to find everything bad in
         * It mustn't contain:
         *   0x00-0x1F, 0x22, 0x2B, 0x2C, 0x2F, 0x3A, 0x3B, 0x3D, 0x5B, 0x5D, 0x7C */
        for (i = 0; i < FirstPart.Length / sizeof(CHAR); i++)
        {
            /* First make sure the character it's not the Lead DBCS */
            if (FsRtlIsLeadDbcsCharacter(FirstPart.Buffer[i]))
            {
                if (i == (FirstPart.Length / sizeof(CHAR)) - 1)
                    return FALSE;
                i++;
            }
            else if ((FirstPart.Buffer[i] < 0x1F) || (FirstPart.Buffer[i] == 0x22) ||
                     (FirstPart.Buffer[i] == 0x2B) || (FirstPart.Buffer[i] == 0x2C) ||
                     (FirstPart.Buffer[i] == 0x2F) || (FirstPart.Buffer[i] == 0x3A) ||
                     (FirstPart.Buffer[i] == 0x3B) || (FirstPart.Buffer[i] == 0x3D) ||
                     (FirstPart.Buffer[i] == 0x5B) || (FirstPart.Buffer[i] == 0x5D) ||
                     (FirstPart.Buffer[i] == 0x7C))
            {
                return FALSE;
            }
            else if (FirstPart.Buffer[i] == '.')
            {
                /* Filename can only contain one dot */
                if (LastDot)
                    return FALSE;

                LastDot = TRUE;

                /* We mustn't have spaces before dot or at the end of the filename
                 * and no dot at the beginning of the filename */
                if ((i == (FirstPart.Length / sizeof(CHAR)) - 1) || i == 0)
                    return FALSE;

                if (i > 0)
                    if (FirstPart.Buffer[i - 1] == ' ')
                        return FALSE;

                /* Filename must be 8.3 filename and not 3.8 filename */
                if ((FirstPart.Length / sizeof(CHAR) - 1) - i > 3)
                    return FALSE;
            }
        }

        /* Filename mustn't finish with a space */
        if (FirstPart.Buffer[FirstPart.Length / sizeof(CHAR) - 1] == ' ')
            return FALSE;

        EndLoop:
        /* Preparing next loop */
        Name.Buffer = RemainingPart.Buffer;
        Name.Length = RemainingPart.Length;
        Name.MaximumLength = RemainingPart.MaximumLength;

        /* Call once again our dissect function */
        FsRtlDissectDbcs(Name, &FirstPart, &RemainingPart);

        /* We found a pathname, it wasn't allowed */
        if (FirstPart.Length > 0 && !PathNamePermissible)
            return FALSE;
    }
    return TRUE;
}

/*++
 * @name FsRtlIsHpfsDbcsLegal
 * @implemented
 *
 * Returns TRUE if the given DbcsName is a valid HPFS filename
 *
 * @param DbcsName
 *        The filename to check. It can also contains pathname.
 *
 * @param WildCardsPermissible
 *        If this is set to FALSE and if filename contains wildcard, the function 
 *        will fail
 *
 * @param PathNamePermissible
 *        If this is set to FALSE and if the filename comes with a pathname, the
 *        function will fail
 *
 * @param LeadingBackslashPermissible
 *        If this is set to FALSE and if the filename starts with a backslash, the
 *        function will fail
 *
 * @return TRUE if the DbcsName is legal, FALSE otherwise
 *
 * @remarks None
 *
 *--*/
BOOLEAN
NTAPI
FsRtlIsHpfsDbcsLegal(IN ANSI_STRING DbcsName,
                     IN BOOLEAN WildCardsPermissible,
                     IN BOOLEAN PathNamePermissible,
                     IN BOOLEAN LeadingBackslashPermissible)
{
    ANSI_STRING FirstPart, RemainingPart, Name;
    ULONG i;

    /* Just quit if the string is empty */
    if (!DbcsName.Length)
        return FALSE;

    /* DbcsName wasn't supposed to be started with \ */
    if (!LeadingBackslashPermissible && DbcsName.Buffer[0] == '\\')
        return FALSE;
    /* DbcsName was allowed to be started with \, but now, remove it */
    else if (LeadingBackslashPermissible && DbcsName.Buffer[0] == '\\')
    {
        DbcsName.Buffer = DbcsName.Buffer + 1;
        DbcsName.Length = DbcsName.Length - 1;
        DbcsName.MaximumLength = DbcsName.MaximumLength - 1;
    }

    /* Extract first part of the DbcsName to work on */
    FsRtlDissectDbcs(DbcsName, &FirstPart, &RemainingPart);
    while (FirstPart.Length > 0)
    {
        /* Accept special filename if wildcards are allowed */
        if (WildCardsPermissible && (FirstPart.Length == 1 || FirstPart.Length == 2) && FirstPart.Buffer[0] == '.')
        {
            if (FirstPart.Length == 2)
            {
                if (FirstPart.Buffer[1] == '.')
                {
                    goto EndLoop;
                }
            }
            else
            {
                goto EndLoop;
            }
        }

        /* Filename must be 255 bytes maximum */
        if (FirstPart.Length > 255)
            return FALSE;

        if (!WildCardsPermissible && FsRtlDoesDbcsContainWildCards(&FirstPart))
            return FALSE;

        /* Now, we will parse the filename to find everything bad in
         * It mustn't contain:
         *   0x00-0x1F, 0x22, 0x2A, 0x2F, 0x3A, 0x3C, 0x3E, 0x3F, 0x7C */
        for (i = 0; i < FirstPart.Length / sizeof(CHAR); i++)
        {
            /* First make sure the character it's not the Lead DBCS */
            if (FsRtlIsLeadDbcsCharacter(FirstPart.Buffer[i]))
            {
                if (i == (FirstPart.Length / sizeof(CHAR)) - 1)
                    return FALSE;
                i++;
            }
            else if ((FirstPart.Buffer[i] < 0x1F) || (FirstPart.Buffer[i] == 0x22) ||
                     (FirstPart.Buffer[i] == 0x2A) || (FirstPart.Buffer[i] == 0x2F) ||
                     (FirstPart.Buffer[i] == 0x3A) || (FirstPart.Buffer[i] == 0x3C) ||
                     (FirstPart.Buffer[i] == 0x3E) || (FirstPart.Buffer[i] == 0x3F) ||
                     (FirstPart.Buffer[i] == 0x7C))
            {
                return FALSE;
            }
        }

        /* Filename mustn't finish with a space or a dot */
        if ((FirstPart.Buffer[FirstPart.Length / sizeof(CHAR) - 1] == ' ') ||
            (FirstPart.Buffer[FirstPart.Length / sizeof(CHAR) - 1] == '.'))
            return FALSE;

        EndLoop:
        /* Preparing next loop */
        Name.Buffer = RemainingPart.Buffer;
        Name.Length = RemainingPart.Length;
        Name.MaximumLength = RemainingPart.MaximumLength;

        /* Call once again our dissect function */
        FsRtlDissectDbcs(Name, &FirstPart, &RemainingPart);

        /* We found a pathname, it wasn't allowed */
        if (FirstPart.Length > 0 && !PathNamePermissible)
            return FALSE;
    }
    return TRUE;
}
