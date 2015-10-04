/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/dbcsname.c
 * PURPOSE:         Provides DBCS parsing and other support routines for FSDs
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Pierre Schweitzer (pierre.schweitzer@reactos.org)
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
    USHORT FirstPosition, i;
    USHORT SkipFirstSlash = 0;
    PAGED_CODE();

    /* Zero the strings before continuing */
    RtlZeroMemory(FirstPart, sizeof(ANSI_STRING));
    RtlZeroMemory(RemainingPart, sizeof(ANSI_STRING));

    /* Just quit if the string is empty */
    if (!Name.Length) return;

    /* Find first backslash */
    FirstPosition = Name.Length;
    for (i = 0; i < Name.Length; i++)
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
    FirstPart->Length = (FirstPosition - SkipFirstSlash);
    FirstPart->MaximumLength = FirstPart->Length;

    /* And second one, if necessary */
    if (FirstPosition < (Name.Length))
    {
        RemainingPart->Buffer = Name.Buffer + FirstPosition + 1;
        RemainingPart->Length = Name.Length - (FirstPosition + 1);
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
    USHORT i;
    PAGED_CODE();

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
 * @remarks
 *
 *--*/
BOOLEAN
NTAPI
FsRtlIsDbcsInExpression(IN PANSI_STRING Expression,
                        IN PANSI_STRING Name)
{
    SHORT StarFound = -1, DosStarFound = -1;
    PUSHORT BackTracking = NULL, DosBackTracking = NULL;
    USHORT ExpressionPosition = 0, NamePosition = 0, MatchingChars, LastDot;
    PAGED_CODE();

    ASSERT(Name->Length);
    ASSERT(Expression->Length);
    ASSERT(!FsRtlDoesDbcsContainWildCards(Name));

    /* Check if we were given strings at all */
    if (!Name->Length || !Expression->Length)
    {
        /* Return TRUE if both strings are empty, otherwise FALSE */
        if (Name->Length == 0 && Expression->Length == 0)
            return TRUE;
        else
            return FALSE;
    }

    /* Check for a shortcut: just one wildcard */
    if (Expression->Length == sizeof(CHAR))
    {
        if (Expression->Buffer[0] == '*')
            return TRUE;
    }

    //ASSERT(FsRtlDoesDbcsContainWildCards(Expression));

    /* Another shortcut, wildcard followed by some string */
    if (Expression->Buffer[0] == '*')
    {
        /* Copy Expression to our local variable */
        ANSI_STRING IntExpression = *Expression;

        /* Skip the first char */
        IntExpression.Buffer++;
        IntExpression.Length -= sizeof(CHAR);

        /* Continue only if the rest of the expression does NOT contain
           any more wildcards */
        if (!FsRtlDoesDbcsContainWildCards(&IntExpression))
        {
            /* Check for a degenerate case */
            if (Name->Length < (Expression->Length - sizeof(CHAR)))
                return FALSE;

            /* Calculate position */
            NamePosition = (Name->Length - IntExpression.Length) / sizeof(CHAR);

            /* Check whether we are breaking a two chars char (DBCS) */
            if (NlsMbOemCodePageTag)
            {
                MatchingChars = 0;

                while (MatchingChars < NamePosition)
                {
                    /* Check if current char is DBCS lead char, if so, jump by two chars */
                    MatchingChars += FsRtlIsLeadDbcsCharacter(Name->Buffer[MatchingChars]) ? 2 : 1;
                }

                /* If so, deny */
                if (MatchingChars > NamePosition)
                    return FALSE;
            }

            /* Compare */
            return RtlEqualMemory(IntExpression.Buffer,
                                  (Name->Buffer + NamePosition),
                                  IntExpression.Length);
        }
    }

    while (NamePosition < Name->Length && ExpressionPosition < Expression->Length)
    {
        /* Basic check to test if chars are equal */
        if ((Expression->Buffer[ExpressionPosition] == Name->Buffer[NamePosition]))
        {
            NamePosition++;
            ExpressionPosition++;
        }
        /* Check cases that eat one char */
        else if (Expression->Buffer[ExpressionPosition] == '?')
        {
            NamePosition++;
            ExpressionPosition++;
        }
        /* Test star */
        else if (Expression->Buffer[ExpressionPosition] == '*')
        {
            /* Skip contigous stars */
            while (ExpressionPosition + 1 < Expression->Length && Expression->Buffer[ExpressionPosition + 1] == '*')
            {
                ExpressionPosition++;
            }

            /* Save star position */
            if (!BackTracking)
            {
                BackTracking = ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,
                                                     Expression->Length * sizeof(USHORT), 'nrSF');
            }
            BackTracking[++StarFound] = ExpressionPosition++;

            /* If star is at the end, then eat all rest and leave */
            if (ExpressionPosition == Expression->Length)
            {
                NamePosition = Name->Length;
                break;
            }
            /* Allow null matching */
            else if (Expression->Buffer[ExpressionPosition] != '?' &&
                     Expression->Buffer[ExpressionPosition] != Name->Buffer[NamePosition])
            {
                NamePosition++;
            }
        }
        /* Check DOS_STAR */
        else if (Expression->Buffer[ExpressionPosition] == ANSI_DOS_STAR)
        {
            /* Skip contigous stars */
            while (ExpressionPosition + 1 < Expression->Length && Expression->Buffer[ExpressionPosition + 1] == ANSI_DOS_STAR)
            {
                ExpressionPosition++;
            }

            /* Look for last dot */
            MatchingChars = 0;
            LastDot = (USHORT)-1;
            while (MatchingChars < Name->Length)
            {
                if (Name->Buffer[MatchingChars] == '.')
                {
                    LastDot = MatchingChars;
                    if (LastDot > NamePosition)
                        break;
                }

                MatchingChars++;
            }

            /* If we don't have dots or we didn't find last yet
             * start eating everything
             */
            if (MatchingChars != Name->Length || LastDot == (USHORT)-1)
            {
                if (!DosBackTracking) DosBackTracking = ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,
                                                                              Expression->Length * sizeof(USHORT), 'nrSF');
                DosBackTracking[++DosStarFound] = ExpressionPosition++;

                /* Not the same char, start exploring */
                if (Expression->Buffer[ExpressionPosition] != Name->Buffer[NamePosition])
                    NamePosition++;
            }
            else
            {
                /* Else, if we are at last dot, eat it - otherwise, null match */
                if (Name->Buffer[NamePosition] == '.')
                    NamePosition++;

                 ExpressionPosition++;
            }
        }
        /* Check DOS_DOT */
        else if (Expression->Buffer[ExpressionPosition] == ANSI_DOS_DOT)
        {
            /* We only match dots */
            if (Name->Buffer[NamePosition] == '.')
            {
                NamePosition++;
            }
            /* Try to explore later on for null matching */
            else if (ExpressionPosition + 1 < Expression->Length &&
                     Name->Buffer[NamePosition] == Expression->Buffer[ExpressionPosition + 1])
            {
                NamePosition++;
            }
            ExpressionPosition++;
        }
        /* Check DOS_QM */
        else if (Expression->Buffer[ExpressionPosition] == ANSI_DOS_QM)
        {
            /* We match everything except dots */
            if (Name->Buffer[NamePosition] != '.')
            {
                NamePosition++;
            }
            ExpressionPosition++;
        }
        /* If nothing match, try to backtrack */
        else if (StarFound >= 0)
        {
            ExpressionPosition = BackTracking[StarFound--];
        }
        else if (DosStarFound >= 0)
        {
            ExpressionPosition = DosBackTracking[DosStarFound--];
        }
        /* Otherwise, fail */
        else
        {
            break;
        }

        /* Under certain circumstances, expression is over, but name isn't
         * and we can backtrack, then, backtrack */
        if (ExpressionPosition == Expression->Length &&
            NamePosition != Name->Length && StarFound >= 0)
        {
            ExpressionPosition = BackTracking[StarFound--];
        }
    }
    /* If we have nullable matching wc at the end of the string, eat them */
    if (ExpressionPosition != Expression->Length && NamePosition == Name->Length)
    {
        while (ExpressionPosition < Expression->Length)
        {
            if (Expression->Buffer[ExpressionPosition] != ANSI_DOS_DOT &&
                Expression->Buffer[ExpressionPosition] != '*' &&
                Expression->Buffer[ExpressionPosition] != ANSI_DOS_STAR)
            {
                break;
            }
            ExpressionPosition++;
        }
    }

    if (BackTracking)
    {
        ExFreePoolWithTag(BackTracking, 'nrSF');
    }
    if (DosBackTracking)
    {
        ExFreePoolWithTag(DosBackTracking, 'nrSF');
    }

    return (ExpressionPosition == Expression->Length && NamePosition == Name->Length);
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
    USHORT i;
    PAGED_CODE();

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
        if (FirstPart.Length < 3 || FirstPart.Length > 12)
            return FALSE;

        /* Now, we will parse the filename to find everything bad in */
        for (i = 0; i < FirstPart.Length; i++)
        {
            /* First make sure the character it's not the Lead DBCS */
            if (FsRtlIsLeadDbcsCharacter(FirstPart.Buffer[i]))
            {
                if (i == (FirstPart.Length) - 1)
                    return FALSE;
                i++;
            }
            /* Then check for bad characters */
            else if (!FsRtlIsAnsiCharacterLegalFat(FirstPart.Buffer[i], WildCardsPermissible))
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
                if ((i == (FirstPart.Length) - 1) || i == 0)
                    return FALSE;

                if (i > 0)
                    if (FirstPart.Buffer[i - 1] == ' ')
                        return FALSE;

                /* Filename must be 8.3 filename and not 3.8 filename */
                if ((FirstPart.Length - 1) - i > 3)
                    return FALSE;
            }
        }

        /* Filename mustn't finish with a space */
        if (FirstPart.Buffer[FirstPart.Length - 1] == ' ')
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
    USHORT i;
    PAGED_CODE();

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

        /* Now, we will parse the filename to find everything bad in */
        for (i = 0; i < FirstPart.Length; i++)
        {
            /* First make sure the character it's not the Lead DBCS */
            if (FsRtlIsLeadDbcsCharacter(FirstPart.Buffer[i]))
            {
                if (i == (FirstPart.Length) - 1)
                    return FALSE;
                i++;
            }
            /* Then check for bad characters */
            else if (!FsRtlIsAnsiCharacterLegalHpfs(FirstPart.Buffer[i], WildCardsPermissible))
            {
                return FALSE;
            }
        }

        /* Filename mustn't finish with a space or a dot */
        if ((FirstPart.Buffer[FirstPart.Length - 1] == ' ') ||
            (FirstPart.Buffer[FirstPart.Length - 1] == '.'))
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
