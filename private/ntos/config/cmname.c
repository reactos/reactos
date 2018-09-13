/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    cmname.c

Abstract:

    Provides routines for handling name comparisons and converting to/from the registry
    compressed name format.

Author:

    John Vert (jvert) 28-Oct-1993

Revision History:


--*/
#include "cmp.h"
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,CmpNameSize)
#pragma alloc_text(PAGE,CmpCopyName)
#pragma alloc_text(PAGE,CmpCompressedNameSize)
#pragma alloc_text(PAGE,CmpCopyCompressedName)
#pragma alloc_text(PAGE,CmpCompareCompressedName)
#endif


USHORT
CmpNameSize(
    IN PHHIVE Hive,
    IN PUNICODE_STRING Name
    )

/*++

Routine Description:

    Determines the space needed to store a given string in the registry.  May apply
    any relevant compression to compute the length, but the compression used is
    guaranteed to be the same as CmpCopyName.

Arguments:

    Hive - supplies the hive control structure (for version checking)

    Name - Supplies the unicode string to be copied into the registry.

Return Value:

    The number of bytes of storage required to store this name.

--*/

{
    ULONG i;

    if (Hive->Version == 1) {
        return(Name->Length);
    }
    for (i=0;i<Name->Length/sizeof(WCHAR);i++) {
        if ((USHORT)Name->Buffer[i] > (UCHAR)-1) {
            return(Name->Length);
        }
    }
    return(Name->Length / sizeof(WCHAR));

}

USHORT
CmpCopyName(
    IN PHHIVE Hive,
    IN PWCHAR Destination,
    IN PUNICODE_STRING Source
    )

/*++

Routine Description:

    Copies the given unicode name into the registry, applying any relevant compression
    at the same time.

Arguments:

    Hive - supplies the hive control structure (For version checking)

    Destination - Supplies the destination of the given string.

    Source - Supplies the unicode string to copy into the registry.

Return Value:

    Number of bytes of storage copied

--*/

{
    ULONG i;

    if (Hive->Version==1) {
        RtlCopyMemory(Destination,Source->Buffer, Source->Length);
        return(Source->Length);
    }

    for (i=0;i<Source->Length/sizeof(WCHAR);i++) {
        if ((USHORT)Source->Buffer[i] > (UCHAR)-1) {
            RtlCopyMemory(Destination,Source->Buffer, Source->Length);
            return(Source->Length);
        }
        ((PUCHAR)Destination)[i] = (UCHAR)(Source->Buffer[i]);
    }
    return(Source->Length / sizeof(WCHAR));
}


USHORT
CmpCompressedNameSize(
    IN PWCHAR Name,
    IN ULONG Length
    )

/*++

Routine Description:

    Computes the length of the unicode string that the given compressed name
    expands into.

Arguments:

    Name - Supplies the compressed name.

    Length - Supplies the length in bytes of the compressed name

Return Value:

    The number of bytes of storage required to hold the Unicode expanded name.

--*/

{
    return((USHORT)Length*sizeof(WCHAR));
}


VOID
CmpCopyCompressedName(
    IN PWCHAR Destination,
    IN ULONG DestinationLength,
    IN PWCHAR Source,
    IN ULONG SourceLength
    )

/*++

Routine Description:

    Copies a compressed name from the registry and expands it to Unicode.

Arguments:

    Destination - Supplies the destination Unicode buffer

    DestinationLength - Supplies the max length of the destination buffer in bytes

    Source - Supplies the compressed string.

    SourceLength - Supplies the length of the compressed string in bytes

Return Value:

    None.

--*/

{
    ULONG i;
    ULONG Chars;

    Chars = (DestinationLength/sizeof(WCHAR) < SourceLength)
             ? DestinationLength/sizeof(WCHAR)
             : SourceLength;

    for (i=0;i<Chars;i++) {
        Destination[i] = (WCHAR)(((PUCHAR)Source)[i]);
    }
}


LONG
CmpCompareCompressedName(
    IN PUNICODE_STRING SearchName,
    IN PWCHAR CompressedName,
    IN ULONG NameLength
    )

/*++

Routine Description:

    Compares a compressed registry string to a Unicode string.  Does a case-insensitive
    comparison.

Arguments:

    SearchName - Supplies the Unicode string to be compared

    CompressedName - Supplies the compressed string to be compared

    NameLength - Supplies the length of the compressed string

Return Value:

    0 = SearchName == CompressedName (of Cell)

    < 0 = SearchName < CompressedName

    > 0 = SearchName > CompressedName

--*/

{
    WCHAR *s1;
    UCHAR *s2;
    USHORT n1, n2;
    WCHAR c1;
    WCHAR c2;
    LONG cDiff;

    s1 = SearchName->Buffer;
    s2 = (UCHAR *)CompressedName;
    n1 = (USHORT )(SearchName->Length / sizeof(WCHAR));
    n2 = (USHORT )(NameLength);
    while (n1 && n2) {
        c1 = *s1++;
        c2 = (WCHAR)(*s2++);

        c1 = RtlUpcaseUnicodeChar(c1);
        c2 = RtlUpcaseUnicodeChar(c2);

        if ((cDiff = ((LONG)c1 - (LONG)c2)) != 0) {
            return( cDiff );
        }

        n1--;
        n2--;
    }

    return( n1 - n2 );
}

