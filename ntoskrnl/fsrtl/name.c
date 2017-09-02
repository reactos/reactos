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
    USHORT Offset, Position, BackTrackingPosition, OldBackTrackingPosition;
    USHORT BackTrackingBuffer[16], OldBackTrackingBuffer[16] = {0};
    PUSHORT BackTrackingSwap, BackTracking = BackTrackingBuffer, OldBackTracking = OldBackTrackingBuffer;
    UNICODE_STRING IntExpression;
    USHORT ExpressionPosition, NamePosition = 0, MatchingChars = 1;
    BOOLEAN EndOfName = FALSE;
    BOOLEAN Result;
    BOOLEAN DontSkipDot;
    WCHAR CompareChar;
    PAGED_CODE();

    /* Check if we were given strings at all */
    if (!Name->Length || !Expression->Length)
    {
        /* Return TRUE if both strings are empty, otherwise FALSE */
        if (!Name->Length && !Expression->Length)
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

    /* Name parsing loop */
    for (; !EndOfName; MatchingChars = BackTrackingPosition, NamePosition++)
    {
        /* Reset positions */
        OldBackTrackingPosition = BackTrackingPosition = 0;

        if (NamePosition >= Name->Length / sizeof(WCHAR))
        {
            EndOfName = TRUE;
            if (MatchingChars && (OldBackTracking[MatchingChars - 1] == Expression->Length * 2))
                break;
        }

        while (MatchingChars > OldBackTrackingPosition)
        {
            ExpressionPosition = (OldBackTracking[OldBackTrackingPosition++] + 1) / 2;

            /* Expression parsing loop */
            for (Offset = 0; ExpressionPosition < Expression->Length; Offset = sizeof(WCHAR))
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
                                                         (Expression->Length + sizeof(WCHAR)) * sizeof(USHORT),
                                                         'nrSF');
                    /* Copy old buffer content */
                    RtlCopyMemory(BackTracking,
                                  BackTrackingBuffer,
                                  RTL_NUMBER_OF(BackTrackingBuffer) * sizeof(USHORT));

                    /* Allocate memory for OldBackTracking */
                    OldBackTracking = ExAllocatePoolWithTag(PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,
                                                            (Expression->Length + sizeof(WCHAR)) * sizeof(USHORT),
                                                            'nrSF');
                    /* Copy old buffer content */
                    RtlCopyMemory(OldBackTracking,
                                  OldBackTrackingBuffer,
                                  RTL_NUMBER_OF(OldBackTrackingBuffer) * sizeof(USHORT));
                }

                /* Basic check to test if chars are equal */
                CompareChar = (NamePosition >= Name->Length / sizeof(WCHAR)) ? UNICODE_NULL : (IgnoreCase ? UpcaseTable[Name->Buffer[NamePosition]] :
                                           Name->Buffer[NamePosition]);
                if (Expression->Buffer[ExpressionPosition / sizeof(WCHAR)] == CompareChar && !EndOfName)
                {
                    BackTracking[BackTrackingPosition++] = (ExpressionPosition + sizeof(WCHAR)) * 2;
                }
                /* Check cases that eat one char */
                else if (Expression->Buffer[ExpressionPosition / sizeof(WCHAR)] == L'?' && !EndOfName)
                {
                    BackTracking[BackTrackingPosition++] = (ExpressionPosition + sizeof(WCHAR)) * 2;
                }
                /* Test star */
                else if (Expression->Buffer[ExpressionPosition / sizeof(WCHAR)] == L'*')
                {
                    BackTracking[BackTrackingPosition++] = ExpressionPosition * 2;
                    BackTracking[BackTrackingPosition++] = (ExpressionPosition * 2) + 3;
                    continue;
                }
                /* Check DOS_STAR */
                else if (Expression->Buffer[ExpressionPosition / sizeof(WCHAR)] == DOS_STAR)
                {
                    /* Look for last dot */
                    DontSkipDot = TRUE;
                    if (!EndOfName && Name->Buffer[NamePosition] == '.')
                    {
                        for (Position = NamePosition - 1; Position < Name->Length; Position++)
                        {
                            if (Name->Buffer[Position] == L'.')
                            {
                                DontSkipDot = FALSE;
                                break;
                            }
                        }
                    }

                    if (EndOfName || Name->Buffer[NamePosition] != L'.' || !DontSkipDot)
                        BackTracking[BackTrackingPosition++] = ExpressionPosition * 2;

                    BackTracking[BackTrackingPosition++] = (ExpressionPosition * 2) + 3;
                    continue;
                }
                /* Check DOS_DOT */
                else if (Expression->Buffer[ExpressionPosition / sizeof(WCHAR)] == DOS_DOT)
                {
                    if (EndOfName) continue;

                    if (Name->Buffer[NamePosition] == L'.')
                        BackTracking[BackTrackingPosition++] = (ExpressionPosition + sizeof(WCHAR)) * 2;
                }
                /* Check DOS_QM */
                else if (Expression->Buffer[ExpressionPosition / sizeof(WCHAR)] == DOS_QM)
                {
                    if (EndOfName || Name->Buffer[NamePosition] == L'.') continue;

                    BackTracking[BackTrackingPosition++] = (ExpressionPosition + sizeof(WCHAR)) * 2;
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
    Result = MatchingChars > 0 && (OldBackTracking[MatchingChars - 1] == (Expression->Length * 2));

    /* Frees the memory if necessary */
    if (BackTracking != BackTrackingBuffer && BackTracking != OldBackTrackingBuffer)
        ExFreePoolWithTag(BackTracking, 'nrSF');
    if (OldBackTracking != BackTrackingBuffer && OldBackTracking != OldBackTrackingBuffer)
        ExFreePoolWithTag(OldBackTracking, 'nrSF');

    return Result;
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
        Status = RtlUpcaseUnicodeString(&UpcaseName2, Name2, TRUE);
        if (!NT_SUCCESS(Status))
        {
            RtlFreeUnicodeString(&UpcaseName1);
            RtlRaiseStatus(Status);
        }

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
