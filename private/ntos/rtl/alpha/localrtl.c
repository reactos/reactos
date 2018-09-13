/*++

Copyright (c) 1993  Digital Equipment Corporation

Module Name:

    localrtl.c

Abstract:

    This module contains alternate implementations of each of the Rtl Memory
    functions and other common functions required by the Rtl Memory test
    programs.

Author:

    Thomas Van Baak (tvb) 11-Jan-1993

Revision History:

--*/

#include <nt.h>
#include "localrtl.h"

//
// Simple pattern generator.
//

#define PATTERN "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define PATTERN_SIZE (sizeof(PATTERN) - 1)
UCHAR Pattern[] = PATTERN;

VOID
FillPattern(PUCHAR To, ULONG Length)
{
    ULONG Index;
    ULONG Rotor = 0;

    for (Index = 0; Index < Length; Index += 1) {
        To[Index] = Pattern[Rotor];
        Rotor += 1;
        if (Rotor == PATTERN_SIZE) {
            Rotor = 0;
        }
    }
}

//
// The following functions are simple, non-optimized, (and thus maybe even
// bug-proof) implementations of each of the Rtl Memory functions.
//

ULONG
LocalCompareMemory (
    PVOID Source1,
    PVOID Source2,
    ULONG Length
    )
{
    ULONG Index;
    PUCHAR Left = Source1;
    ULONG Match;
    PUCHAR Right = Source2;

    Match = 0;
    for (Index = 0; Index < Length; Index += 1) {
        if (Left[Index] != Right[Index]) {
            break;
        }
        Match += 1;
    }
    return Match;
}

ULONG
LocalCompareMemoryUlong (
    PVOID Source,
    ULONG Length,
    ULONG Pattern
    )
{
    PULONG From = Source;
    ULONG Index;
    ULONG Match;

    Match = 0;
    for (Index = 0; Index < Length / sizeof(ULONG); Index += 1) {
        if (From[Index] != Pattern) {
            break;
        }
        Match += sizeof(ULONG);
    }
    return Match;
}

VOID
LocalMoveMemory (
   PVOID Destination,
   PVOID Source,
   ULONG Length
   )
{
    PUCHAR From = Source;
    ULONG Index;
    PUCHAR To = Destination;

    for (Index = 0; Index < Length; Index += 1) {
        if (To <= From) {
            To[Index] = From[Index];

        } else {
            To[Length - 1 - Index] = From[Length - 1 - Index];
        }
    }
}

VOID
LocalFillMemory (
   PVOID Destination,
   ULONG Length,
   UCHAR Fill
   )
{
    ULONG Index;
    PUCHAR To = Destination;

    for (Index = 0; Index < Length; Index += 1) {
        To[Index] = Fill;
    }
}

VOID
LocalFillMemoryUlong (
   PVOID Destination,
   ULONG Length,
   ULONG Pattern
   )
{
    ULONG Index;
    PULONG To = Destination;

    for (Index = 0; Index < Length / sizeof(ULONG); Index += 1) {
        To[Index] = Pattern;
    }
}

VOID
LocalZeroMemory (
   PVOID Destination,
   ULONG Length
   )
{
    ULONG Index;
    PUCHAR To = Destination;

    for (Index = 0; Index < Length; Index += 1) {
        To[Index] = 0;
    }
}
