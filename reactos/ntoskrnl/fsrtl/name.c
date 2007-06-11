/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/name.c
 * PURPOSE:         Provides name parsing and other support routines for FSDs
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Filip Navara (navaraf@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name FsRtlAreNamesEqual
 * @implemented
 *
 * FILLME
 *
 * @param Name1
 *        FILLME
 *
 * @param Name2
 *        FILLME
 *
 * @param IgnoreCase
 *        FILLME
 *
 * @param UpcaseTable
 *        FILLME
 *
 * @return None
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
    ULONG i;
    NTSTATUS Status;

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
    KEBUGCHECK(0);
}

/*++
 * @name FsRtlDoesNameContainWildCards
 * @implemented
 *
 * FILLME
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
 * FILLME
 *
 * @param DeviceObject
 *        FILLME
 *
 * @param Irp
 *        FILLME
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

