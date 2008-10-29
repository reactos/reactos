/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/name.c
 * PURPOSE:         Provides name parsing and other support routines for FSDs
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Filip Navara (navaraf@reactos.org)
 *                  Pierre Schweitzer (heis_spiter@hotmail.com) 
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
    ULONG i, j, k = 0;
	
    ASSERT(!FsRtlDoesNameContainWildCards(Name));

    for (i = 0 ; i < Expression->Length / sizeof(WCHAR) ; i++)
    {
        if ((Expression->Buffer[i] == (IgnoreCase ? UpcaseTable[Name->Buffer[k]] : Name->Buffer[k])) ||
            (Expression->Buffer[i] == L'?') || (Expression->Buffer[i] == DOS_QM) ||
            (Expression->Buffer[i] == DOS_DOT && (Name->Buffer[k] == L'.' || Name->Buffer[k] == L'0')))
        {
            k++;
        }
        else if (Expression->Buffer[i] == L'*')
        {
            k = Name->Length / sizeof(WCHAR);
        }
        else if (Expression->Buffer[i] == DOS_STAR)
        {
            for (j = k ; j < Name->Length / sizeof(WCHAR) ; j++)
            {
                if (Name->Buffer[j] == L'.')
                {
                    k = j;
                    break;
                }
            }
        }
        else
        {
            k = 0;
        }
        if (k >= Expression->Length / sizeof(WCHAR))
        {
            return TRUE;
        }
    }

    return FALSE;
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
    ULONG FirstPosition, i;
    ULONG SkipFirstSlash = 0;

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
        if (Status != STATUS_SUCCESS)
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
