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
