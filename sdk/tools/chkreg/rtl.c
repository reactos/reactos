/*
 * PROJECT:     ReactOS Tools
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Check Registry Utility I/O Run-Time routines
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "chkreg.h"

/* FUNCTIONS ****************************************************************/

VOID
NTAPI
RtlInitializeBitMap(
    IN PRTL_BITMAP BitMapHeader,
    IN PULONG BitMapBuffer,
    IN ULONG SizeOfBitMap)
{
    BitMapHeader->SizeOfBitMap = SizeOfBitMap;
    BitMapHeader->Buffer = BitMapBuffer;
}

VOID
NTAPI
RtlSetAllBits(
    IN PRTL_BITMAP BitMapHeader)
{
    return;
}

VOID
NTAPI
RtlClearAllBits(
    IN PRTL_BITMAP BitMapHeader)
{
    return;
}

ULONG
NTAPI
RtlFindSetBits(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG NumberToFind,
    IN ULONG HintIndex)
{
    return 0;
}

VOID
NTAPI
RtlSetBits(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG StartingIndex,
    IN ULONG NumberToSet)
{
    return;
}

LONG
NTAPI
RtlCompareUnicodeString(
    IN PCUNICODE_STRING String1,
    IN PCUNICODE_STRING String2,
    IN BOOLEAN CaseInSensitive)
{
    USHORT i;
    WCHAR c1, c2;

    for (i = 0; i <= String1->Length / sizeof(WCHAR) && i <= String2->Length / sizeof(WCHAR); i++)
    {
        if (CaseInSensitive)
        {
            c1 = RtlUpcaseUnicodeChar(String1->Buffer[i]);
            c2 = RtlUpcaseUnicodeChar(String2->Buffer[i]);
        }
        else
        {
            c1 = String1->Buffer[i];
            c2 = String2->Buffer[i];
        }

        if (c1 < c2)
            return -1;
        else if (c1 > c2)
            return 1;
    }

    return 0;
}

WCHAR
NTAPI
RtlUpcaseUnicodeChar(
    IN WCHAR Source)
{
    USHORT Offset;

    if (Source < 'a')
        return Source;

    if (Source <= 'z')
        return (Source - ('a' - 'A'));

    Offset = 0;

    return Source + (SHORT)Offset;
}

VOID
NTAPI
KeQuerySystemTime(
    OUT PLARGE_INTEGER CurrentTime)
{
    CurrentTime->QuadPart = 0;
}

/* EOF */
