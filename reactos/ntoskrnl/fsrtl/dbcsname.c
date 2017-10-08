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
    USHORT Offset, Position, BackTrackingPosition, OldBackTrackingPosition;
    USHORT BackTrackingBuffer[16], OldBackTrackingBuffer[16] = {0};
    PUSHORT BackTrackingSwap, BackTracking = BackTrackingBuffer, OldBackTracking = OldBackTrackingBuffer;
    USHORT ExpressionPosition, NamePosition = 0, MatchingChars = 1;
    USHORT NameChar = 0, ExpressionChar;
    BOOLEAN EndOfName = FALSE;
    BOOLEAN Result;
    BOOLEAN DontSkipDot;
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

    /* Name parsing loop */
    for (; !EndOfName; MatchingChars = BackTrackingPosition)
    {
        /* Reset positions */
        OldBackTrackingPosition = BackTrackingPosition = 0;

        if (NamePosition >= Name->Length)
        {
            EndOfName = TRUE;
            if (OldBackTracking[MatchingChars - 1] == Expression->Length * 2)
                break;
        }
        else
        {
            /* If lead byte present */
            if (FsRtlIsLeadDbcsCharacter(Name->Buffer[NamePosition]))
            {
                NameChar = Name->Buffer[NamePosition] +
                           (0x100 * Name->Buffer[NamePosition + 1]);
                NamePosition += sizeof(USHORT);
            }
            else
            {
                NameChar = Name->Buffer[NamePosition];
                NamePosition += sizeof(UCHAR);
            }
        }

        while (MatchingChars > OldBackTrackingPosition)
        {
            ExpressionPosition = (OldBackTracking[OldBackTrackingPosition++] + 1) / 2;

            /* Expression parsing loop */
            for (Offset = 0; ExpressionPosition < Expression->Length; )
            {
                ExpressionPosition += Offset;

                if (ExpressionPosition == Expression->Length)
                {
                    BackTracking[BackTrackingPosition++] = Expression->Length * 2;
                    break;
                }

                /* If buffer too small */
                if (BackTrackingPosition > RTL_NUMBER_OF(BackTrackingBuffer) - 1)
                {
                    /* Allocate memory for BackTracking */
                    BackTracking = ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,
                                                         (Expression->Length + 1) * sizeof(USHORT) * 2,
                                                         'nrSF');
                    /* Copy old buffer content */
                    RtlCopyMemory(BackTracking,
                                  BackTrackingBuffer,
                                  RTL_NUMBER_OF(BackTrackingBuffer) * sizeof(USHORT));

                    /* Allocate memory for OldBackTracking */
                    OldBackTracking = ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,
                                                            (Expression->Length + 1) * sizeof(USHORT) * 2,
                                                            'nrSF');
                    /* Copy old buffer content */
                    RtlCopyMemory(OldBackTracking,
                                  OldBackTrackingBuffer,
                                  RTL_NUMBER_OF(OldBackTrackingBuffer) * sizeof(USHORT));
                }

                /* If lead byte present */
                if (FsRtlIsLeadDbcsCharacter(Expression->Buffer[ExpressionPosition]))
                {
                    ExpressionChar = Expression->Buffer[ExpressionPosition] +
                                     (0x100 * Expression->Buffer[ExpressionPosition + 1]);
                    Offset = sizeof(USHORT);
                }
                else
                {
                    ExpressionChar = Expression->Buffer[ExpressionPosition];
                    Offset = sizeof(UCHAR);
                }

                /* Basic check to test if chars are equal */
                if (ExpressionChar == NameChar && !EndOfName)
                {
                    BackTracking[BackTrackingPosition++] = (ExpressionPosition + Offset) * 2;
                }
                /* Check cases that eat one char */
                else if (ExpressionChar == '?' && !EndOfName)
                {
                    BackTracking[BackTrackingPosition++] = (ExpressionPosition + Offset) * 2;
                }
                /* Test star */
                else if (ExpressionChar == '*')
                {
                    BackTracking[BackTrackingPosition++] = ExpressionPosition * 2;
                    BackTracking[BackTrackingPosition++] = (ExpressionPosition * 2) + 1;
                    continue;
                }
                /* Check DOS_STAR */
                else if (ExpressionChar == ANSI_DOS_STAR)
                {
                    /* Look for last dot */
                    DontSkipDot = TRUE;
                    if (!EndOfName && NameChar == '.')
                    {
                        for (Position = NamePosition; Position < Name->Length; )
                        {
                            /* If lead byte not present */
                            if (!FsRtlIsLeadDbcsCharacter(Name->Buffer[Position]))
                            {
                                if (Name->Buffer[Position] == '.')
                                {
                                    DontSkipDot = FALSE;
                                    break;
                                }

                                Position += sizeof(UCHAR);
                            }
                            else
                            {
                                Position += sizeof(USHORT);
                            }
                        }
                    }

                    if (EndOfName || NameChar != '.' || !DontSkipDot)
                        BackTracking[BackTrackingPosition++] = ExpressionPosition * 2;

                    BackTracking[BackTrackingPosition++] = (ExpressionPosition * 2) + 1;
                    continue;
                }
                /* Check DOS_DOT */
                else if (ExpressionChar == DOS_DOT)
                {
                    if (EndOfName) continue;

                    if (NameChar == '.')
                        BackTracking[BackTrackingPosition++] = (ExpressionPosition + Offset) * 2;
                }
                /* Check DOS_QM */
                else if (ExpressionChar == ANSI_DOS_QM)
                {
                    if (EndOfName || NameChar == '.') continue;

                    BackTracking[BackTrackingPosition++] = (ExpressionPosition + Offset) * 2;
                }

                /* Leave from loop */
                break;
            }

            for (Position = 0; MatchingChars > OldBackTrackingPosition && Position < BackTrackingPosition; Position++)
            {
                while (MatchingChars > OldBackTrackingPosition &&
                       BackTracking[Position] > OldBackTracking[OldBackTrackingPosition])
                {
                    ++OldBackTrackingPosition;
                }
            }
        }

        /* Swap pointers */
        BackTrackingSwap = BackTracking;
        BackTracking = OldBackTracking;
        OldBackTracking = BackTrackingSwap;
    }

    /* Store result value */
    Result = (OldBackTracking[MatchingChars - 1] == Expression->Length * 2);

    /* Frees the memory if necessary */
    if (BackTracking != BackTrackingBuffer && BackTracking != OldBackTrackingBuffer)
        ExFreePoolWithTag(BackTracking, 'nrSF');
    if (OldBackTracking != BackTrackingBuffer && OldBackTracking != OldBackTrackingBuffer)
        ExFreePoolWithTag(OldBackTracking, 'nrSF');

    return Result;
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
    ANSI_STRING FirstPart, RemainingPart;
    BOOLEAN LastDot;
    USHORT i;
    PAGED_CODE();

    /* Just quit if the string is empty */
    if (!DbcsName.Length)
        return FALSE;

    /* Accept special filename if wildcards are allowed */
    if (WildCardsPermissible && (DbcsName.Length == 1 || DbcsName.Length == 2) && DbcsName.Buffer[0] == '.')
    {
        if (DbcsName.Length == 2)
        {
            if (DbcsName.Buffer[1] == '.')
                return TRUE;
        }
        else
        {
            return TRUE;
        }
    }

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

    if (PathNamePermissible)
    {
        /* We copy the buffer for FsRtlDissectDbcs call */
        RemainingPart.Buffer = DbcsName.Buffer;
        RemainingPart.Length = DbcsName.Length;
        RemainingPart.MaximumLength = DbcsName.MaximumLength;

        while (RemainingPart.Length > 0)
        {
            if (RemainingPart.Buffer[0] == '\\')
                return FALSE;

            /* Call once again our dissect function */
            FsRtlDissectDbcs(RemainingPart, &FirstPart, &RemainingPart);

            if (!FsRtlIsFatDbcsLegal(FirstPart,
                                     WildCardsPermissible,
                                     FALSE,
                                     FALSE))
            {
                return FALSE;
            }
        }

        return TRUE;
    }

    if (WildCardsPermissible && FsRtlDoesDbcsContainWildCards(&DbcsName))
    {
        for (i = 0; i < DbcsName.Length; i++)
        {
            /* First make sure the character it's not the Lead DBCS */
            if (FsRtlIsLeadDbcsCharacter(DbcsName.Buffer[i]))
            {
                i++;
            }
            /* Then check for bad characters */
            else if (!FsRtlIsAnsiCharacterLegalFat(DbcsName.Buffer[i], TRUE))
            {
                return FALSE;
            }
        }

        return TRUE;
    }

    /* Filename must be 8.3 filename */
    if (DbcsName.Length > 12)
        return FALSE;

    /* Reset dots count */
    LastDot = FALSE;

    for (i = 0; i < DbcsName.Length; i++)
    {
        /* First make sure the character it's not the Lead DBCS */
        if (FsRtlIsLeadDbcsCharacter(DbcsName.Buffer[i]))
        {
            if (!LastDot && (i >= 7))
                return FALSE;

            if (i == (DbcsName.Length - 1))
                return FALSE;

            i++;
            continue;
        }
        /* Then check for bad characters */
        else if (!FsRtlIsAnsiCharacterLegalFat(DbcsName.Buffer[i], WildCardsPermissible))
        {
            return FALSE;
        }
        else if (DbcsName.Buffer[i] == '.')
        {
            /* Filename can only contain one dot */
            if (LastDot)
                return FALSE;

            LastDot = TRUE;

            /* We mustn't have spaces before dot or at the end of the filename
             * and no dot at the beginning of the filename */
            if (i == (DbcsName.Length - 1) || i == 0)
                return FALSE;

            /* Filename must be 8.3 filename and not 3.8 filename */
            if ((DbcsName.Length - 1) - i > 3)
                return FALSE;

            if ((i > 0) && DbcsName.Buffer[i - 1] == ' ')
                return FALSE;
        }
        /* Filename mustn't finish with a space */
        else if (DbcsName.Buffer[i] == ' ' && i == (DbcsName.Length - 1))
        {
            return FALSE;
        }

        if (!LastDot && (i >= 8))
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
    ANSI_STRING FirstPart, RemainingPart;
    USHORT i;
    PAGED_CODE();

    /* Just quit if the string is empty */
    if (!DbcsName.Length)
        return FALSE;

    /* Accept special filename if wildcards are allowed */
    if (WildCardsPermissible && (DbcsName.Length == 1 || DbcsName.Length == 2) && DbcsName.Buffer[0] == '.')
    {
        if (DbcsName.Length == 2)
        {
            if (DbcsName.Buffer[1] == '.')
                return TRUE;
        }
        else
        {
            return TRUE;
        }
    }

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

    if (PathNamePermissible)
    {
        /* We copy the buffer for FsRtlDissectDbcs call */
        RemainingPart.Buffer = DbcsName.Buffer;
        RemainingPart.Length = DbcsName.Length;
        RemainingPart.MaximumLength = DbcsName.MaximumLength;

        while (RemainingPart.Length > 0)
        {
            if (RemainingPart.Buffer[0] == '\\')
                return FALSE;

            /* Call once again our dissect function */
            FsRtlDissectDbcs(RemainingPart, &FirstPart, &RemainingPart);

            if (!FsRtlIsHpfsDbcsLegal(FirstPart,
                                      WildCardsPermissible,
                                      FALSE,
                                      FALSE))
            {
                return FALSE;
            }
        }

        return TRUE;
    }

    if (DbcsName.Length > 255)
        return FALSE;

    for (i = 0; i < DbcsName.Length; i++)
    {
        /* First make sure the character it's not the Lead DBCS */
        if (FsRtlIsLeadDbcsCharacter(DbcsName.Buffer[i]))
        {
            if (i == (DbcsName.Length - 1))
                return FALSE;
            i++;
        }
        /* Then check for bad characters */
        else if (!FsRtlIsAnsiCharacterLegalHpfs(DbcsName.Buffer[i], WildCardsPermissible))
        {
            return FALSE;
        }
        /* Filename mustn't finish with a space or a dot */
        else if ((DbcsName.Buffer[i] == ' ' || DbcsName.Buffer[i] == '.') && i == (DbcsName.Length - 1))
        {
            return FALSE;
        }
    }

    return TRUE;
}
