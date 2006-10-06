/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/cm/cmname.c
 * PURPOSE:         Routines for dealing with registry key and value names.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

USHORT
NTAPI
CmpCopyName(IN PHHIVE Hive,
            IN PWCHAR Destination,
            IN PUNICODE_STRING Source)
{
    ULONG i;

    /* Check for old hives */
    if (Hive->Version == 1)
    {
        /* Just copy the source directly */
        RtlCopyMemory(Destination, Source->Buffer, Source->Length);
        return Source->Length;
    }

    /* For new versions, check for compressed name */
    for (i = 0; i < (Source->Length / sizeof(WCHAR)); i++)
    {
        /* Check if the name is non compressed */
        if ((Source->Buffer[i]) > -1)
        {
            /* Do the copy */
            RtlCopyMemory(Destination, Source->Buffer, Source->Length);
            return Source->Length;
        }

        /* Copy this character */
        Destination[i] = Source->Buffer[i];
    }

    /* Compressed name, return length */
    return Source->Length / sizeof(WCHAR);
}

USHORT
NTAPI
CmpNameSize(IN PHHIVE Hive,
            IN PUNICODE_STRING Name)
{
    ULONG i;

    /* For old hives, just retun the length */
    if (Hive->Version == 1) return Name->Length;

    /* For new versions, check for compressed name */
    for (i = 0; i < (Name->Length / sizeof(WCHAR)); i++)
    {
        /* Check if the name is non compressed */
        if ((Name->Buffer[i]) > -1) return Name->Length;
    }

    /* Compressed name, return length */
    return Name->Length / sizeof(WCHAR);
}

LONG
NTAPI
CmpCompareCompressedName(IN PUNICODE_STRING SearchName,
                         IN PWCHAR CompressedName,
                         IN ULONG NameLength)
{
    WCHAR *p, *pp;
    WCHAR p1, p2;
    USHORT SearchLength;
    LONG Result;

    /* Set the pointers and length and then loop */
    p = SearchName->Buffer;
    pp = CompressedName;
    SearchLength = (SearchName->Length / sizeof(WCHAR));
    while (SearchLength && NameLength)
    {
        /* Get the characters */
        p1 = *p++;
        p2 = *pp++;

        /* See if they match and return result if they don't */
        Result = (LONG)RtlUpcaseUnicodeChar(p1) -
                 (LONG)RtlUpcaseUnicodeChar(p2);
        if (Result) return Result;

        /* Next chars */
        SearchLength--;
        NameLength--;
    }

    /* Return the difference directly */
    return SearchLength - NameLength;
}
