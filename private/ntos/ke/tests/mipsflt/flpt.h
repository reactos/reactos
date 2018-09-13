/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    flpt.h

Abstract:

    This module is the header file for the user mode IEEE floating point
    tests.

Author:

    David N. Cutler (davec) 1-Jul-1991

Environment:

    User mode only.

Revision History:

--*/

#include "stdio.h"
#include "string.h"
#include "ntos.h"

//
// Floating status register bits.
//

#define SI (1 << 2)
#define SU (1 << 3)
#define SO (1 << 4)
#define SZ (1 << 5)
#define SV (1 << 6)

#define EI (1 << 7)
#define EU (1 << 8)
#define EO (1 << 9)
#define EZ (1 << 10)
#define EV (1 << 11)

#define XI (1 << 12)
#define XU (1 << 13)
#define XO (1 << 14)
#define XZ (1 << 15)
#define XV (1 << 16)

#define CC (1 << 23)

#define FS (1 << 24)

//
// Define negative infinity.
//

#define MINUS_DOUBLE_INFINITY_VALUE (DOUBLE_INFINITY_VALUE_HIGH | (1 << 31))
#define MINUS_SINGLE_INFINITY_VALUE (SINGLE_INFINITY_VALUE | (1 << 31))

//
// Define signaling NaN prefix values.
//

#define DOUBLE_SIGNAL_NAN_PREFIX 0x7ff80000
#define SINGLE_SIGNAL_NAN_PREFIX 0x7fc00000

//
// Define sign bit.
//

#define SIGN (1 << 31)

//
// Define floating status union.
//

typedef union _FLOATING_STATUS {
    FSR Status;
    ULONG Data;
} FLOATING_STATUS, *PFLOATING_STATUS;

//
// Define procedure prootypes.
//

ULONG
AddDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Addend1,
    IN PULARGE_INTEGER Addend2,
    OUT PULARGE_INTEGER Result
    );

ULONG
DivideDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Dividend,
    IN PULARGE_INTEGER Divisor,
    OUT PULARGE_INTEGER Result
    );

ULONG
MultiplyDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Multiplicand,
    IN PULARGE_INTEGER Multiplier,
    OUT PULARGE_INTEGER Result
    );

ULONG
SubtractDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Subtrahend,
    IN PULARGE_INTEGER Minuend,
    OUT PULARGE_INTEGER Result
    );

ULONG
AddSingle (
    IN ULONG RoundingMode,
    IN ULONG Addend1,
    IN ULONG Addend2,
    OUT PULONG Result
    );

ULONG
DivideSingle (
    IN ULONG RoundingMode,
    IN ULONG Dividend,
    IN ULONG Divisor,
    OUT PULONG Result
    );

ULONG
MultiplySingle (
    IN ULONG RoundingMode,
    IN ULONG Multiplicand,
    IN ULONG Multiplier,
    OUT PULONG Result
    );

ULONG
SubtractSingle (
    IN ULONG RoundingMode,
    IN ULONG Subtrahend,
    IN ULONG Minuend,
    OUT PULONG Result
    );

ULONG
AbsoluteDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Operand,
    OUT PULARGE_INTEGER Result
    );

ULONG
CeilToLongwordFromDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Source,
    OUT PULONG Result
    );

ULONG
CeilToLongwordFromSingle (
    IN ULONG RoundingMode,
    IN ULONG Source,
    OUT PULONG Result
    );

ULONG
ConvertToDoubleFromSingle (
    IN ULONG RoundingMode,
    IN ULONG Source,
    OUT PULARGE_INTEGER Result
    );

ULONG
ConvertToLongwordFromDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Source,
    OUT PULONG Result
    );

ULONG
ConvertToLongwordFromSingle (
    IN ULONG RoundingMode,
    IN ULONG Source,
    OUT PULONG Result
    );

ULONG
ConvertToSingleFromDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Source,
    OUT PULONG Result
    );

ULONG
CompareFDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Comparand1,
    IN PULARGE_INTEGER Comparand2
    );

ULONG
CompareUnDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Comparand1,
    IN PULARGE_INTEGER Comparand2
    );

ULONG
CompareEqDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Comparand1,
    IN PULARGE_INTEGER Comparand2
    );

ULONG
CompareUeqDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Comparand1,
    IN PULARGE_INTEGER Comparand2
    );

ULONG
CompareOltDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Comparand1,
    IN PULARGE_INTEGER Comparand2
    );

ULONG
CompareUltDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Comparand1,
    IN PULARGE_INTEGER Comparand2
    );

ULONG
CompareOleDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Comparand1,
    IN PULARGE_INTEGER Comparand2
    );

ULONG
CompareUleDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Comparand1,
    IN PULARGE_INTEGER Comparand2
    );

ULONG
CompareSfDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Comparand1,
    IN PULARGE_INTEGER Comparand2
    );

ULONG
CompareNgleDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Comparand1,
    IN PULARGE_INTEGER Comparand2
    );

ULONG
CompareSeqDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Comparand1,
    IN PULARGE_INTEGER Comparand2
    );

ULONG
CompareNglDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Comparand1,
    IN PULARGE_INTEGER Comparand2
    );

ULONG
CompareLtDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Comparand1,
    IN PULARGE_INTEGER Comparand2
    );

ULONG
CompareNgeDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Comparand1,
    IN PULARGE_INTEGER Comparand2
    );

ULONG
CompareLeDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Comparand1,
    IN PULARGE_INTEGER Comparand2
    );

ULONG
CompareNgtDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Comparand1,
    IN PULARGE_INTEGER Comparand2
    );

ULONG
CompareFSingle (
    IN ULONG RoundingMode,
    IN ULONG Comparand1,
    IN ULONG Comparand2
    );

ULONG
CompareUnSingle (
    IN ULONG RoundingMode,
    IN ULONG Comparand1,
    IN ULONG Comparand2
    );

ULONG
CompareEqSingle (
    IN ULONG RoundingMode,
    IN ULONG Comparand1,
    IN ULONG Comparand2
    );

ULONG
CompareUeqSingle (
    IN ULONG RoundingMode,
    IN ULONG Comparand1,
    IN ULONG Comparand2
    );

ULONG
CompareOltSingle (
    IN ULONG RoundingMode,
    IN ULONG Comparand1,
    IN ULONG Comparand2
    );

ULONG
CompareUltSingle (
    IN ULONG RoundingMode,
    IN ULONG Comparand1,
    IN ULONG Comparand2
    );

ULONG
CompareOleSingle (
    IN ULONG RoundingMode,
    IN ULONG Comparand1,
    IN ULONG Comparand2
    );

ULONG
CompareUleSingle (
    IN ULONG RoundingMode,
    IN ULONG Comparand1,
    IN ULONG Comparand2
    );

ULONG
CompareSfSingle (
    IN ULONG RoundingMode,
    IN ULONG Comparand1,
    IN ULONG Comparand2
    );

ULONG
CompareNgleSingle (
    IN ULONG RoundingMode,
    IN ULONG Comparand1,
    IN ULONG Comparand2
    );

ULONG
CompareSeqSingle (
    IN ULONG RoundingMode,
    IN ULONG Comparand1,
    IN ULONG Comparand2
    );

ULONG
CompareNglSingle (
    IN ULONG RoundingMode,
    IN ULONG Comparand1,
    IN ULONG Comparand2
    );

ULONG
CompareLtSingle (
    IN ULONG RoundingMode,
    IN ULONG Comparand1,
    IN ULONG Comparand2
    );

ULONG
CompareNgeSingle (
    IN ULONG RoundingMode,
    IN ULONG Comparand1,
    IN ULONG Comparand2
    );

ULONG
CompareLeSingle (
    IN ULONG RoundingMode,
    IN ULONG Comparand1,
    IN ULONG Comparand2
    );

ULONG
CompareNgtSingle (
    IN ULONG RoundingMode,
    IN ULONG Comparand1,
    IN ULONG Comparand2
    );

ULONG
FloorToLongwordFromDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Source,
    OUT PULONG Result
    );

ULONG
FloorToLongwordFromSingle (
    IN ULONG RoundingMode,
    IN ULONG Source,
    OUT PULONG Result
    );

ULONG
MoveDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Operand,
    OUT PULARGE_INTEGER Result
    );

ULONG
NegateDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Operand,
    OUT PULARGE_INTEGER Result
    );

ULONG
AbsoluteSingle (
    IN ULONG RoundingMode,
    IN ULONG Operand,
    OUT PULONG Result
    );

ULONG
MoveSingle (
    IN ULONG RoundingMode,
    IN ULONG Operand,
    OUT PULONG Result
    );

ULONG
NegateSingle (
    IN ULONG RoundingMode,
    IN ULONG Operand,
    OUT PULONG Result
    );

ULONG
RoundToLongwordFromDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Source,
    OUT PULONG Result
    );

ULONG
RoundToLongwordFromSingle (
    IN ULONG RoundingMode,
    IN ULONG Source,
    OUT PULONG Result
    );

ULONG
TruncateToLongwordFromDouble (
    IN ULONG RoundingMode,
    IN PULARGE_INTEGER Source,
    OUT PULONG Result
    );

ULONG
TruncateToLongwordFromSingle (
    IN ULONG RoundingMode,
    IN ULONG Source,
    OUT PULONG Result
    );

VOID
Test1 (
    VOID
    );

VOID
Test2 (
    VOID
    );

VOID
Test3 (
    VOID
    );

VOID
Test4 (
    VOID
    );

VOID
Test5 (
    VOID
    );

VOID
Test6 (
    VOID
    );

VOID
Test7 (
    VOID
    );

VOID
Test8 (
    VOID
    );

VOID
Test9 (
    VOID
    );

VOID
Test10 (
    VOID
    );

VOID
Test11 (
    VOID
    );

VOID
Test12 (
    VOID
    );

VOID
Test13 (
    VOID
    );

VOID
Test14 (
    VOID
    );

VOID
Test15 (
    VOID
    );

VOID
Test16 (
    VOID
    );

VOID
Test17 (
    VOID
    );

VOID
Test18 (
    VOID
    );

VOID
Test19 (
    VOID
    );

VOID
Test20 (
    VOID
    );

VOID
Test21 (
    VOID
    );

VOID
Test22 (
    VOID
    );

VOID
Test23 (
    VOID
    );

VOID
Test24 (
    VOID
    );

VOID
Test25 (
    VOID
    );

VOID
Test26 (
    VOID
    );
