/*++

Copyright (c) 1993  Digital Equipment Corporation

Module Name:

    localrtl.h

Abstract:

    This module contains the local Rtl header file.

Author:

    Thomas Van Baak (tvb) 11-Jan-1993

Revision History:

--*/

//
// Define function prototypes of local Rtl Memory functions.
//

ULONG
LocalCompareMemory (
    PVOID Source1,
    PVOID Source2,
    ULONG Length
    );

ULONG
LocalCompareMemoryUlong (
    PVOID Source,
    ULONG Length,
    ULONG Pattern
    );

VOID
LocalMoveMemory (
   PVOID Destination,
   CONST VOID *Source,
   ULONG Length
   );

VOID
LocalFillMemory (
   PVOID Destination,
   ULONG Length,
   UCHAR Fill
   );

VOID
LocalFillMemoryUlong (
   PVOID Destination,
   ULONG Length,
   ULONG Pattern
   );

VOID
LocalZeroMemory (
   PVOID Destination,
   ULONG Length
   );

//
// Define function prototypes of other common functions.
//

VOID
FillPattern(
    PUCHAR To,
    ULONG Length
    );

//
// Define maximum values for the string tests.
//

#define MAX_MARGIN 8
#define MAX_OFFSET 32
#define MAX_LENGTH 72
