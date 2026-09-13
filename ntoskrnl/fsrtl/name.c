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
