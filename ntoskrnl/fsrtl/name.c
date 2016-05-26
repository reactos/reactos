/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/name.c
 * PURPOSE:         Provides name parsing and other support routines for FSDs
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Filip Navara (navaraf@reactos.org)
 *                  Pierre Schweitzer (pierre.schweitzer@reactos.org)
 *                  Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/
BOOLEAN
NTAPI
FsRtlIsNameInExpressionPrivate(IN PUNICODE_STRING Expression,
                               IN PUNICODE_STRING Name,
                               IN BOOLEAN IgnoreCase,
                               IN PWCHAR UpcaseTable OPTIONAL)
{
    SHORT StarFound = -1, DosStarFound = -1;
    USHORT BackTrackingBuffer[5], DosBackTrackingBuffer[5];
    PUSHORT BackTracking = BackTrackingBuffer, DosBackTracking = DosBackTrackingBuffer;
    SHORT BackTrackingSize = RTL_NUMBER_OF(BackTrackingBuffer);
    SHORT DosBackTrackingSize = RTL_NUMBER_OF(DosBackTrackingBuffer);
    UNICODE_STRING IntExpression;
    USHORT ExpressionPosition = 0, NamePosition = 0, MatchingChars, LastDot;
    WCHAR CompareChar;
    PAGED_CODE();

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
    if (Expression->Length == sizeof(WCHAR))
    {
        if (Expression->Buffer[0] == L'*')
            return TRUE;
    }

    ASSERT(!IgnoreCase || UpcaseTable);

    /* Another shortcut, wildcard followed by some string */
    if (Expression->Buffer[0] == L'*')
    {
        /* Copy Expression to our local variable */
        IntExpression = *Expression;

        /* Skip the first char */
        IntExpression.Buffer++;
        IntExpression.Length -= sizeof(WCHAR);

        /* Continue only if the rest of the expression does NOT contain
           any more wildcards */
        if (!FsRtlDoesNameContainWildCards(&IntExpression))
        {
            /* Check for a degenerate case */
            if (Name->Length < (Expression->Length - sizeof(WCHAR)))
                return FALSE;

            /* Calculate position */
            NamePosition = (Name->Length - IntExpression.Length) / sizeof(WCHAR);

            /* Compare */
            if (!IgnoreCase)
            {
                /* We can just do a byte compare */
                return RtlEqualMemory(IntExpression.Buffer,
                                      Name->Buffer + NamePosition,
                                      IntExpression.Length);
            }
            else
            {
                /* Not so easy, need to upcase and check char by char */
                for (ExpressionPosition = 0; ExpressionPosition < (IntExpression.Length / sizeof(WCHAR)); ExpressionPosition++)
                {
                    /* Assert that expression is already upcased! */
                    ASSERT(IntExpression.Buffer[ExpressionPosition] == UpcaseTable[IntExpression.Buffer[ExpressionPosition]]);

                    /* Now compare upcased name char with expression */
                    if (UpcaseTable[Name->Buffer[NamePosition + ExpressionPosition]] !=
                        IntExpression.Buffer[ExpressionPosition])
                    {
                        return FALSE;
                    }
                }

                /* It matches */
                return TRUE;
            }
        }
    }

    while ((NamePosition < Name->Length / sizeof(WCHAR)) &&
           (ExpressionPosition < Expression->Length / sizeof(WCHAR)))
    {
        /* Basic check to test if chars are equal */
        CompareChar = IgnoreCase ? UpcaseTable[Name->Buffer[NamePosition]] :
                                   Name->Buffer[NamePosition];
        if (Expression->Buffer[ExpressionPosition] == CompareChar)
        {
            NamePosition++;
            ExpressionPosition++;
        }
        /* Check cases that eat one char */
        else if (Expression->Buffer[ExpressionPosition] == L'?')
        {
            NamePosition++;
            ExpressionPosition++;
        }
        /* Test star */
        else if (Expression->Buffer[ExpressionPosition] == L'*')
        {
            /* Skip contigous stars */
            while ((ExpressionPosition + 1 < (USHORT)(Expression->Length / sizeof(WCHAR))) &&
                   (Expression->Buffer[ExpressionPosition + 1] == L'*'))
            {
                ExpressionPosition++;
            }

            /* Save star position */
            StarFound++;
            if (StarFound >= BackTrackingSize)
            {
                BackTrackingSize = Expression->Length / sizeof(WCHAR);
                BackTracking = ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,
                                                     BackTrackingSize * sizeof(USHORT),
                                                     'nrSF');
                RtlCopyMemory(BackTracking, BackTrackingBuffer, sizeof(BackTrackingBuffer));

            }
            BackTracking[StarFound] = ExpressionPosition++;

            /* If star is at the end, then eat all rest and leave */
            if (ExpressionPosition == Expression->Length / sizeof(WCHAR))
            {
                NamePosition = Name->Length / sizeof(WCHAR);
                break;
            }

            /* Allow null matching */
            if (Expression->Buffer[ExpressionPosition] != L'?' &&
                     Expression->Buffer[ExpressionPosition] != Name->Buffer[NamePosition])
            {
                NamePosition++;
            }
        }
        /* Check DOS_STAR */
        else if (Expression->Buffer[ExpressionPosition] == DOS_STAR)
        {
            /* Skip contigous stars */
            while ((ExpressionPosition + 1 < (USHORT)(Expression->Length / sizeof(WCHAR))) &&
                   (Expression->Buffer[ExpressionPosition + 1] == DOS_STAR))
            {
                ExpressionPosition++;
            }

            /* Look for last dot */
            MatchingChars = 0;
            LastDot = (USHORT)-1;
            while (MatchingChars < Name->Length / sizeof(WCHAR))
            {
                if (Name->Buffer[MatchingChars] == L'.')
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
                DosStarFound++;
                if (DosStarFound >= DosBackTrackingSize)
                {
                    DosBackTrackingSize = Expression->Length / sizeof(WCHAR);
                    DosBackTracking = ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,
                                                            DosBackTrackingSize * sizeof(USHORT),
                                                            'nrSF');
                    RtlCopyMemory(DosBackTracking, DosBackTrackingBuffer, sizeof(DosBackTrackingBuffer));
                }
                DosBackTracking[DosStarFound] = ExpressionPosition++;

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
        else if (Expression->Buffer[ExpressionPosition] == DOS_DOT)
        {
            /* We only match dots */
            if (Name->Buffer[NamePosition] == L'.')
            {
                NamePosition++;
            }
            /* Try to explore later on for null matching */
            else if ((ExpressionPosition + 1 < (USHORT)(Expression->Length / sizeof(WCHAR))) &&
                     (Name->Buffer[NamePosition] == Expression->Buffer[ExpressionPosition + 1]))
            {
                NamePosition++;
            }
            ExpressionPosition++;
        }
        /* Check DOS_QM */
        else if (Expression->Buffer[ExpressionPosition] == DOS_QM)
        {
            /* We match everything except dots */
            if (Name->Buffer[NamePosition] != L'.')
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
        if (ExpressionPosition == Expression->Length / sizeof(WCHAR) &&
            NamePosition != Name->Length / sizeof(WCHAR) &&
            StarFound >= 0)
        {
            ExpressionPosition = BackTracking[StarFound--];
        }
    }
    /* If we have nullable matching wc at the end of the string, eat them */
    if (ExpressionPosition != Expression->Length / sizeof(WCHAR) && NamePosition == Name->Length / sizeof(WCHAR))
    {
        while (ExpressionPosition < Expression->Length / sizeof(WCHAR))
        {
            if (Expression->Buffer[ExpressionPosition] != DOS_DOT &&
                Expression->Buffer[ExpressionPosition] != L'*' &&
                Expression->Buffer[ExpressionPosition] != DOS_STAR)
            {
                break;
            }
            ExpressionPosition++;
        }
    }

    if (BackTracking != BackTrackingBuffer)
    {
        ExFreePoolWithTag(BackTracking, 'nrSF');
    }
    if (DosBackTracking != DosBackTrackingBuffer)
    {
        ExFreePoolWithTag(DosBackTracking, 'nrSF');
    }

    return (ExpressionPosition == Expression->Length / sizeof(WCHAR) && NamePosition == Name->Length / sizeof(WCHAR));
}

/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name FsRtlAreNamesEqual
 * @implemented
 *
 * Compare two strings to check if they match
 *
 * @param Name1
 *        First unicode string to compare
 *
 * @param Name2
 *        Second unicode string to compare
 *
 * @param IgnoreCase
 *        If TRUE, Case will be ignored when comparing strings
 *
 * @param UpcaseTable
 *        Table for upcase letters. If NULL is given, system one will be used
 *
 * @return TRUE if the strings are equal
 *
 * @remarks From Bo Branten's ntifs.h v25.
 *
 *--*/
BOOLEAN
NTAPI
FsRtlAreNamesEqual(IN PCUNICODE_STRING Name1,
                   IN PCUNICODE_STRING Name2,
                   IN BOOLEAN IgnoreCase,
                   IN PCWCH UpcaseTable OPTIONAL)
{
    UNICODE_STRING UpcaseName1;
    UNICODE_STRING UpcaseName2;
    BOOLEAN StringsAreEqual, MemoryAllocated = FALSE;
    USHORT i;
    NTSTATUS Status;
    PAGED_CODE();

    /* Well, first check their size */
    if (Name1->Length != Name2->Length) return FALSE;

    /* Check if the caller didn't give an upcase table */
    if ((IgnoreCase) && !(UpcaseTable))
    {
        /* Upcase the string ourselves */
        Status = RtlUpcaseUnicodeString(&UpcaseName1, Name1, TRUE);
        if (!NT_SUCCESS(Status)) RtlRaiseStatus(Status);

        /* Upcase the second string too */
        RtlUpcaseUnicodeString(&UpcaseName2, Name2, TRUE);
        Name1 = &UpcaseName1;
        Name2 = &UpcaseName2;

        /* Make sure we go through the path below, but free the strings */
        IgnoreCase = FALSE;
        MemoryAllocated = TRUE;
    }

    /* Do a case-sensitive search */
    if (!IgnoreCase)
    {
        /* Use a raw memory compare */
        StringsAreEqual = RtlEqualMemory(Name1->Buffer,
                                         Name2->Buffer,
                                         Name1->Length);

        /* Check if we allocated strings */
        if (MemoryAllocated)
        {
            /* Free them */
            RtlFreeUnicodeString(&UpcaseName1);
            RtlFreeUnicodeString(&UpcaseName2);
        }

        /* Return the equality */
        return StringsAreEqual;
    }
    else
    {
        /* Case in-sensitive search */
        for (i = 0; i < Name1->Length / sizeof(WCHAR); i++)
        {
            /* Check if the character matches */
            if (UpcaseTable[Name1->Buffer[i]] != UpcaseTable[Name2->Buffer[i]])
            {
                /* Non-match found! */
                return FALSE;
            }
        }

        /* We finished the loop so we are equal */
        return TRUE;
    }
}

/*++
 * @name FsRtlDissectName
 * @implemented
 *
 * Dissects a given path name into first and remaining part.
 *
 * @param Name
 *        Unicode string to dissect.
 *
 * @param FirstPart
 *        Pointer to user supplied UNICODE_STRING, that will later point
 *        to the first part of the original name.
 *
 * @param RemainingPart
 *        Pointer to user supplied UNICODE_STRING, that will later point
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
FsRtlDissectName(IN UNICODE_STRING Name,
                 OUT PUNICODE_STRING FirstPart,
                 OUT PUNICODE_STRING RemainingPart)
{
    USHORT FirstPosition, i;
    USHORT SkipFirstSlash = 0;
    PAGED_CODE();

    /* Zero the strings before continuing */
    RtlZeroMemory(FirstPart, sizeof(UNICODE_STRING));
    RtlZeroMemory(RemainingPart, sizeof(UNICODE_STRING));

    /* Just quit if the string is empty */
    if (!Name.Length) return;

    /* Find first backslash */
    FirstPosition = Name.Length / sizeof(WCHAR) ;
    for (i = 0; i < Name.Length / sizeof(WCHAR); i++)
    {
        /* If we found one... */
        if (Name.Buffer[i] == L'\\')
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
    FirstPart->Length = (FirstPosition - SkipFirstSlash) * sizeof(WCHAR);
    FirstPart->MaximumLength = FirstPart->Length;

    /* And second one, if necessary */
    if (FirstPosition < (Name.Length / sizeof(WCHAR)))
    {
        RemainingPart->Buffer = Name.Buffer + FirstPosition + 1;
        RemainingPart->Length = Name.Length - (FirstPosition + 1) * sizeof(WCHAR);
        RemainingPart->MaximumLength = RemainingPart->Length;
    }
}

/*++
 * @name FsRtlDoesNameContainWildCards
 * @implemented
 *
 * Checks if the given string contains WildCards
 *
 * @param Name
 *        Pointer to a UNICODE_STRING containing Name to examine
 *
 * @return TRUE if Name contains wildcards, FALSE otherwise
 *
 * @remarks From Bo Branten's ntifs.h v12.
 *
 *--*/
BOOLEAN
NTAPI
FsRtlDoesNameContainWildCards(IN PUNICODE_STRING Name)
{
    PWCHAR Ptr;
    PAGED_CODE();

    /* Loop through every character */
    if (Name->Length)
    {
        Ptr = Name->Buffer + (Name->Length / sizeof(WCHAR)) - 1;
        while ((Ptr >= Name->Buffer) && (*Ptr != L'\\'))
        {
            /* Check for Wildcard */
            if (FsRtlIsUnicodeCharacterWild(*Ptr)) return TRUE;
            Ptr--;
        }
    }

    /* Nothing Found */
    return FALSE;
}

/*++
 * @name FsRtlIsNameInExpression
 * @implemented
 *
 * Check if the Name string is in the Expression string.
 *
 * @param Expression
 *        The string in which we've to find Name. It can contain wildcards.
 *        If IgnoreCase is set to TRUE, this string MUST BE uppercase.
 *
 * @param Name
 *        The string to find. It cannot contain wildcards
 *
 * @param IgnoreCase
 *        If set to TRUE, case will be ignore with upcasing both strings
 *
 * @param UpcaseTable
 *        If not NULL, and if IgnoreCase is set to TRUE, it will be used to
 *        upcase the both strings
 *
 * @return TRUE if Name is in Expression, FALSE otherwise
 *
 * @remarks From Bo Branten's ntifs.h v12. This function should be
 *          rewritten to avoid recursion and better wildcard handling
 *          should be implemented (see FsRtlDoesNameContainWildCards).
 *
 *--*/
BOOLEAN
NTAPI
FsRtlIsNameInExpression(IN PUNICODE_STRING Expression,
                        IN PUNICODE_STRING Name,
                        IN BOOLEAN IgnoreCase,
                        IN PWCHAR UpcaseTable OPTIONAL)
{
    BOOLEAN Result;
    NTSTATUS Status;
    UNICODE_STRING IntName;

    if (IgnoreCase && !UpcaseTable)
    {
        Status = RtlUpcaseUnicodeString(&IntName, Name, TRUE);
        if (!NT_SUCCESS(Status))
        {
            ExRaiseStatus(Status);
        }
        Name = &IntName;
        IgnoreCase = FALSE;
    }
    else
    {
        IntName.Buffer = NULL;
    }

    Result = FsRtlIsNameInExpressionPrivate(Expression, Name, IgnoreCase, UpcaseTable);

    if (IntName.Buffer != NULL)
    {
        RtlFreeUnicodeString(&IntName);
    }

    return Result;
}
