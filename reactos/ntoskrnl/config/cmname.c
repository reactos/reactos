/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmname.c
 * PURPOSE:         Configuration Manager - Name Management
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#include "cm.h"
#define NDEBUG
#include "debug.h"

/* GLOBALS *******************************************************************/

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
        if (Source->Buffer[i] > (UCHAR)-1)
        {
            /* Do the copy */
            RtlCopyMemory(Destination, Source->Buffer, Source->Length);
            return Source->Length;
        }

        /* Copy this character */
        ((PCHAR)Destination)[i] = (CHAR)(Source->Buffer[i]);
    }

    /* Compressed name, return length */
    return Source->Length / sizeof(WCHAR);
}

VOID
NTAPI
CmpCopyCompressedName(IN PWCHAR Destination,
                      IN ULONG DestinationLength,
                      IN PWCHAR Source,
                      IN ULONG SourceLength)
{
    ULONG i, Length;

    /* Get the actual length to copy */
    Length = min(DestinationLength / sizeof(WCHAR), SourceLength);
    for (i = 0; i < Length; i++)
    {
        /* Copy each character */
        Destination[i] = (WCHAR)((PCHAR)Source)[i];
    }
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
        if (Name->Buffer[i] > (UCHAR)-1) return Name->Length;
    }

    /* Compressed name, return length */
    return Name->Length / sizeof(WCHAR);
}

USHORT
NTAPI
CmpCompressedNameSize(IN PWCHAR Name,
                      IN ULONG Length)
{
    /*
     * Don't remove this: compressed names are "opaque" and just because
     * the current implementation turns them into ansi-names doesn't mean
     * that it will remain that way forever, so -never- assume this code
     * below internally!
     */
    return Length * sizeof(WCHAR);
}

LONG
NTAPI
CmpCompareCompressedName(IN PUNICODE_STRING SearchName,
                         IN PWCHAR CompressedName,
                         IN ULONG NameLength)
{
    WCHAR *p;
    CHAR *pp;
    WCHAR p1, p2;
    USHORT SearchLength;
    LONG Result;

    /* Set the pointers and length and then loop */
    p = SearchName->Buffer;
    pp = (PCHAR)CompressedName;
    SearchLength = (SearchName->Length / sizeof(WCHAR));
    while ((SearchLength) && (NameLength))
    {
        /* Get the characters */
        p1 = *p++;
        p2 = (WCHAR)(*pp++);

        /* Check if we have a direct match */
        if (p1 != p2)
        {
            /* See if they match and return result if they don't */
            Result = (LONG)RtlUpcaseUnicodeChar(p1) -
                     (LONG)RtlUpcaseUnicodeChar(p2);
            if (Result) return Result;
        }

        /* Next chars */
        SearchLength--;
        NameLength--;
    }

    /* Return the difference directly */
    return SearchLength - NameLength;
}

BOOLEAN
NTAPI
CmpFindNameInList(IN PHHIVE Hive,
                  IN PCHILD_LIST ChildList,
                  IN PUNICODE_STRING Name,
                  IN PULONG ChildIndex,
                  IN PHCELL_INDEX CellIndex)
{
    PCELL_DATA CellData;
    HCELL_INDEX CellToRelease = HCELL_NIL;
    ULONG i;
    PCM_KEY_VALUE KeyValue;
    LONG Result;
    UNICODE_STRING SearchName;
    BOOLEAN Success;

    /* Make sure there's actually something on the list */
    if (ChildList->Count != 0)
    {
        /* Get the cell data */
        CellData = (PCELL_DATA)HvGetCell(Hive, ChildList->List);
        if (!CellData)
        {
            /* Couldn't get the cell... tell the caller */
            *CellIndex = HCELL_NIL;
            return FALSE;
        }

        /* Now loop every entry */
        for (i = 0; i < ChildList->Count; i++)
        {
            /* Check if we have a cell to release */
            if (CellToRelease != HCELL_NIL)
            {
                /* Release it */
                HvReleaseCell(Hive, CellToRelease);
                CellToRelease = HCELL_NIL;
            }

            /* Get this value */
            KeyValue = (PCM_KEY_VALUE)HvGetCell(Hive, CellData->u.KeyList[i]);
            if (!KeyValue)
            {
                /* Return with no data found */
                *CellIndex = HCELL_NIL;
                Success = FALSE;
                goto Return;
            }

            /* Save the cell to release */
            CellToRelease = CellData->u.KeyList[i];

            /* Check if it's a compressed value name */
            if (KeyValue->Flags & VALUE_COMP_NAME)
            {
                /* Use the compressed name check */
                Result = CmpCompareCompressedName(Name,
                                                  KeyValue->Name,
                                                  KeyValue->NameLength);
            }
            else
            {
                /* Setup the Unicode string */
                SearchName.Length = KeyValue->NameLength;
                SearchName.MaximumLength = SearchName.Length;
                SearchName.Buffer = KeyValue->Name;
                Result = RtlCompareUnicodeString(Name, &SearchName, TRUE);
            }

            /* Check if we found it */
            if (!Result)
            {
                /* We did...return info to caller */
                if (ChildIndex) *ChildIndex = i;
                *CellIndex = CellData->u.KeyList[i];

                /* Set success state */
                Success = TRUE;
                goto Return;
            }
        }

        /* Got to the end of the list */
        if (ChildIndex) *ChildIndex = i;
        *CellIndex = HCELL_NIL;

        /* Nothing found if we got here */
        Success = TRUE;
        goto Return;
    }

    /* Nothing found...check if the caller wanted more info */
    ASSERT(ChildList->Count == 0);
    if (ChildIndex) *ChildIndex = 0;
    *CellIndex = HCELL_NIL;

    /* Nothing found if we got here */
    return TRUE;

Return:
    /* Release the first cell we got */
    if (CellData) HvReleaseCell(Hive, ChildList->List);

    /* Release the secondary one, if we have one */
    if (CellToRelease) HvReleaseCell(Hive, CellToRelease);
    return Success;
}
