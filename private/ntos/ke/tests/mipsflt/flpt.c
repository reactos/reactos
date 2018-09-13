/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    flpt.c

Abstract:

    This module implements user mode IEEE floating point tests.

Author:

    David N. Cutler (davec) 20-Jun-1991

Environment:

    User mode only.

Revision History:

--*/

#include "flpt.h"

VOID
main(
    int argc,
    char *argv[]
    )

{
    //
    // Anounce start of floting point tests.
    //

    printf("\nStart of floating point test\n");
    Test1();
    Test2();
    Test3();
    Test4();
    Test5();
    Test6();
    Test7();
    Test8();
    Test9();
    Test10();
    Test11();
    Test12();
    Test13();
    Test14();
    Test15();
    Test16();
    Test17();
    Test18();
    Test19();
    Test20();
    Test21();
    Test22();
    Test23();
    Test24();
    Test25();
    Test26();

    //
    // Announce end of floating point test.
    //

    printf("End of floating point test\n");
    return;
}

VOID
Test1 (
    VOID
    )

{

    ULONG Count;
    FLOATING_STATUS Fsr;
    ULONG SingleResult;
    ULONG Subtest;

    //
    // Test 1 - Add single denormalized test.
    //

    Subtest = 0;
    printf("    Test 1 - add/subtract single denormalized ...");
    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         0x3ff,
                         0x1,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x400)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         0x1,
                         0x7fff,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x8000)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         0x400000,
                         SIGN | 0x440000,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != (SIGN | 0x40000))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         SIGN | 0x400000,
                         0x440000,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x40000)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         0x400000,
                         SIGN | 0x400000,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x0)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         SIGN | 0x400000,
                         0x400000,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != (SIGN | 0x0))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         0x400000,
                         0x400000,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x800000)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         0x800000,
                         0x7fffff,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0xffffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = AddSingle(EI | ROUND_TO_NEAREST,
                             0x800000,
                             0x3f800000,
                             &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = AddSingle(EO | ROUND_TO_NEAREST,
                             0x7f000000,
                             0x7f000000,
                             &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_OVERFLOW) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = AddSingle(EI | ROUND_TO_NEAREST,
                             0x7f000000,
                             0x7f000000,
                             &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = AddSingle(EI | EO | ROUND_TO_NEAREST,
                             0x7f000000,
                             0x7f000000,
                             &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_OVERFLOW) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = SubtractSingle(ROUND_TO_NEAREST,
                              0x3ff,
                              SIGN | 0x1,
                              &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x400)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = SubtractSingle(ROUND_TO_NEAREST,
                              0x1,
                              SIGN | 0x7fff,
                              &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x8000)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = SubtractSingle(ROUND_TO_NEAREST,
                              0x400000,
                              0x440000,
                              &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != (SIGN | 0x40000))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = SubtractSingle(ROUND_TO_NEAREST,
                              SIGN | 0x400000,
                              SIGN | 0x440000,
                              &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x40000)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = SubtractSingle(ROUND_TO_NEAREST,
                              0x400000,
                              0x400000,
                              &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x0)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = SubtractSingle(ROUND_TO_NEAREST,
                              SIGN | 0x400000,
                              SIGN | 0x400000,
                              &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != (SIGN | 0x0))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = SubtractSingle(ROUND_TO_NEAREST,
                              0x400000,
                              SIGN | 0x400000,
                              &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x800000)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = SubtractSingle(ROUND_TO_NEAREST,
                              0x800000,
                              SIGN | 0x7fffff,
                              &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0xffffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = SubtractSingle(EI | ROUND_TO_NEAREST,
                                  0x800000,
                                  SIGN | 0x3f800000,
                                  &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = SubtractSingle(EO | ROUND_TO_NEAREST,
                                  0x7f000000,
                                  SIGN | 0x7f000000,
                                  &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_OVERFLOW) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = SubtractSingle(EI | ROUND_TO_NEAREST,
                                  0x7f000000,
                                  SIGN | 0x7f000000,
                                  &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = SubtractSingle(EI | EO | ROUND_TO_NEAREST,
                                  0x7f000000,
                                  SIGN | 0x7f000000,
                                  &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_OVERFLOW) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    //
    // End of test 1.
    //

    printf("succeeded\n");
    return;

    //
    // Test 1 failed.
    //

TestFailed:
    printf(" subtest %d failed, fsr = %lx, result = %lx\n",
           Subtest,
           Fsr.Data,
           SingleResult);

    return;
}

VOID
Test2 (
    VOID
    )

{

    ULONG Count;
    FLOATING_STATUS Fsr;
    ULONG SingleResult;
    ULONG Subtest;

    //
    // Test 2 - Add single round to nearest test.
    //

    Subtest = 0;
    printf("    Test 2 - add single round to nearest ...");
    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         0x1800000,
                         0x7ffff8,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x19ffffe)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         0x1800000,
                         0x7ffff9,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_NEAREST)) ||
        (SingleResult != 0x19ffffe)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         0x1800000,
                         0x7ffffa,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_NEAREST)) ||
        (SingleResult != 0x19ffffe)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         0x1800000,
                         0x7ffffb,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_NEAREST)) ||
        (SingleResult != 0x19fffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         0x1800000,
                         0x7ffffc,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x19fffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         0x1800000,
                         0x7ffffd,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_NEAREST)) ||
        (SingleResult != 0x19fffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         0x1800000,
                         0x7ffffe,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_NEAREST)) ||
        (SingleResult != 0x1a00000)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         0x1800000,
                         0x7fffff,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_NEAREST)) ||
        (SingleResult != 0x1a00000)) {
        goto TestFailed;
    }

    Count = 0;
    try {
        Subtest += 1;
        Fsr.Data = AddSingle(EI | ROUND_TO_NEAREST,
                             0x1800000,
                             0x7fffff,
                             &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    //
    // End of test 2.
    //

    printf("succeeded\n");
    return;

    //
    // Test 2 failed.
    //

TestFailed:
    printf(" subtest %d failed, fsr = %lx, result = %lx\n",
           Subtest,
           Fsr.Data,
           SingleResult);

    return;
}

VOID
Test3 (
    VOID
    )

{

    ULONG Count;
    FLOATING_STATUS Fsr;
    ULONG SingleResult;
    ULONG Subtest;

    //
    // Test 3 - Add single round to zero test.
    //

    Subtest = 0;
    printf("    Test 3 - add single round to zero ...");
    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_ZERO,
                         0x1800000,
                         0x7ffff8,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_ZERO) ||
        (SingleResult != 0x19ffffe)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_ZERO,
                         0x1800000,
                         0x7ffff9,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_ZERO)) ||
        (SingleResult != 0x19ffffe)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_ZERO,
                         0x1800000,
                         0x7ffffa,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_ZERO)) ||
        (SingleResult != 0x19ffffe)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_ZERO,
                         0x1800000,
                         0x7ffffb,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_ZERO)) ||
        (SingleResult != 0x19ffffe)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_ZERO,
                         0x1800000,
                         0x7ffffc,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_ZERO) ||
        (SingleResult != 0x19fffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_ZERO,
                         0x1800000,
                         0x7ffffd,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_ZERO)) ||
        (SingleResult != 0x19fffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_ZERO,
                         0x1800000,
                         0x7ffffe,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_ZERO)) ||
        (SingleResult != 0x19fffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_ZERO,
                         0x1800000,
                         0x7fffff,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_ZERO)) ||
        (SingleResult != 0x19fffff)) {
        goto TestFailed;
    }

    Count = 0;
    try {
        Subtest += 1;
        Fsr.Data = AddSingle(EI | ROUND_TO_ZERO,
                             0x1800000,
                             0x7fffff,
                             &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    //
    // End of test 3.
    //

    printf("succeeded\n");
    return;

    //
    // Test 3 failed.
    //

TestFailed:
    printf(" subtest %d failed, fsr = %lx, result = %lx\n",
           Subtest,
           Fsr.Data,
           SingleResult);

    return;
}

VOID
Test4 (
    VOID
    )

{

    ULONG Count;
    FLOATING_STATUS Fsr;
    ULONG SingleResult;
    ULONG Subtest;

    //
    // Test 4 - Add single round to positive infinity test.
    //

    Subtest = 0;
    printf("    Test 4 - add single round to positive infinity ...");
    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_PLUS_INFINITY,
                         0x1800000,
                         0x7ffff8,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_PLUS_INFINITY) ||
        (SingleResult != 0x19ffffe)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_PLUS_INFINITY,
                         0x1800000,
                         0x7ffff9,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_PLUS_INFINITY)) ||
        (SingleResult != 0x19fffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_PLUS_INFINITY,
                         0x1800000,
                         0x7ffffa,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_PLUS_INFINITY)) ||
        (SingleResult != 0x19fffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_PLUS_INFINITY,
                         0x1800000,
                         0x7ffffb,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_PLUS_INFINITY)) ||
        (SingleResult != 0x19fffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_PLUS_INFINITY,
                         0x1800000,
                         0x7ffffc,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_PLUS_INFINITY) ||
        (SingleResult != 0x19fffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_PLUS_INFINITY,
                         0x1800000,
                         0x7ffffd,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_PLUS_INFINITY)) ||
        (SingleResult != 0x1a00000)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_PLUS_INFINITY,
                         0x1800000,
                         0x7ffffe,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_PLUS_INFINITY)) ||
        (SingleResult != 0x1a00000)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_PLUS_INFINITY,
                         0x1800000,
                         0x7fffff,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_PLUS_INFINITY)) ||
        (SingleResult != 0x1a00000)) {
        goto TestFailed;
    }

    Count = 0;
    try {
        Subtest += 1;
        Fsr.Data = AddSingle(EI | ROUND_TO_PLUS_INFINITY,
                             0x1800000,
                             0x7fffff,
                             &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_PLUS_INFINITY,
                         SIGN | 0x1800000,
                         SIGN | 0x7ffff8,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_PLUS_INFINITY) ||
        (SingleResult != (SIGN | 0x19ffffe))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_PLUS_INFINITY,
                         SIGN | 0x1800000,
                         SIGN | 0x7ffff9,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_PLUS_INFINITY)) ||
        (SingleResult != (SIGN | 0x19ffffe))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_PLUS_INFINITY,
                         SIGN | 0x1800000,
                         SIGN | 0x7ffffa,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_PLUS_INFINITY)) ||
        (SingleResult != (SIGN | 0x19ffffe))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_PLUS_INFINITY,
                         SIGN | 0x1800000,
                         SIGN | 0x7ffffb,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_PLUS_INFINITY)) ||
        (SingleResult != (SIGN | 0x19ffffe))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_PLUS_INFINITY,
                         SIGN | 0x1800000,
                         SIGN | 0x7ffffc,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_PLUS_INFINITY) ||
        (SingleResult != (SIGN | 0x19fffff))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_PLUS_INFINITY,
                         SIGN | 0x1800000,
                         SIGN | 0x7ffffd,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_PLUS_INFINITY)) ||
        (SingleResult != (SIGN | 0x19fffff))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_PLUS_INFINITY,
                         SIGN | 0x1800000,
                         SIGN | 0x7ffffe,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_PLUS_INFINITY)) ||
        (SingleResult != (SIGN | 0x19fffff))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_PLUS_INFINITY,
                         SIGN | 0x1800000,
                         SIGN | 0x7fffff,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_PLUS_INFINITY)) ||
        (SingleResult != (SIGN | 0x19fffff))) {
        goto TestFailed;
    }

    //
    // End of test 4.
    //

    printf("succeeded\n");
    return;

    //
    // Test 4 failed.
    //

TestFailed:
    printf(" subtest %d failed, fsr = %lx, result = %lx\n",
           Subtest,
           Fsr.Data,
           SingleResult);

    return;
}

VOID
Test5 (
    VOID
    )

{

    ULONG Count;
    FLOATING_STATUS Fsr;
    ULONG SingleResult;
    ULONG Subtest;

    //
    // Test 5 - Add single round to negative infinity test.
    //

    Subtest = 0;
    printf("    Test 5 - add single round to negative infinity ...");
    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_MINUS_INFINITY,
                         SIGN | 0x1800000,
                         SIGN | 0x7ffff8,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_MINUS_INFINITY) ||
        (SingleResult != (SIGN | 0x19ffffe))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_MINUS_INFINITY,
                         SIGN | 0x1800000,
                         SIGN | 0x7ffff9,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_MINUS_INFINITY)) ||
        (SingleResult != (SIGN | 0x19fffff))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_MINUS_INFINITY,
                         SIGN | 0x1800000,
                         SIGN | 0x7ffffa,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_MINUS_INFINITY)) ||
        (SingleResult != (SIGN | 0x19fffff))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_MINUS_INFINITY,
                         SIGN | 0x1800000,
                         SIGN | 0x7ffffb,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_MINUS_INFINITY)) ||
        (SingleResult != (SIGN | 0x19fffff))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_MINUS_INFINITY,
                         SIGN | 0x1800000,
                         SIGN | 0x7ffffc,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_MINUS_INFINITY) ||
        (SingleResult != (SIGN | 0x19fffff))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_MINUS_INFINITY,
                         SIGN | 0x1800000,
                         SIGN | 0x7ffffd,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_MINUS_INFINITY)) ||
        (SingleResult != (SIGN | 0x1a00000))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_MINUS_INFINITY,
                         SIGN | 0x1800000,
                         SIGN | 0x7ffffe,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_MINUS_INFINITY)) ||
        (SingleResult != (SIGN | 0x1a00000))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_MINUS_INFINITY,
                         SIGN | 0x1800000,
                         SIGN | 0x7fffff,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_MINUS_INFINITY)) ||
        (SingleResult != (SIGN | 0x1a00000))) {
        goto TestFailed;
    }

    Count = 0;
    try {
        Subtest += 1;
        Fsr.Data = AddSingle(EI | ROUND_TO_MINUS_INFINITY,
                             0x1800000,
                             0x7fffff,
                             &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_MINUS_INFINITY,
                         0x1800000,
                         0x7ffff8,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_MINUS_INFINITY) ||
        (SingleResult != 0x19ffffe)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_MINUS_INFINITY,
                         0x1800000,
                         0x7ffff9,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_MINUS_INFINITY)) ||
        (SingleResult != 0x19ffffe)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_MINUS_INFINITY,
                         0x1800000,
                         0x7ffffa,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_MINUS_INFINITY)) ||
        (SingleResult != 0x19ffffe)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_MINUS_INFINITY,
                         0x1800000,
                         0x7ffffb,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_MINUS_INFINITY)) ||
        (SingleResult != 0x19ffffe)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_MINUS_INFINITY,
                         0x1800000,
                         0x7ffffc,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_MINUS_INFINITY) ||
        (SingleResult != 0x19fffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_MINUS_INFINITY,
                         0x1800000,
                         0x7ffffd,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_MINUS_INFINITY)) ||
        (SingleResult != 0x19fffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_MINUS_INFINITY,
                         0x1800000,
                         0x7ffffe,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_MINUS_INFINITY)) ||
        (SingleResult != 0x19fffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_MINUS_INFINITY,
                         0x1800000,
                         0x7fffff,
                         &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_MINUS_INFINITY)) ||
        (SingleResult != 0x19fffff)) {
        goto TestFailed;
    }

    //
    // End of test 5.
    //

    printf("succeeded\n");
    return;

    //
    // Test 5 failed.
    //

TestFailed:
    printf(" subtest %d failed, fsr = %lx, result = %lx\n",
           Subtest,
           Fsr.Data,
           SingleResult);

    return;
}

VOID
Test6 (
    VOID
    )

{

    ULONG Count;
    FLOATING_STATUS Fsr;
    ULONG SingleResult;
    ULONG Subtest;

    //
    // Test 6 - Add single infinity and NaN test.
    //

    Subtest = 0;
    printf("    Test 6 - add single infinity and NaN ...");

    Subtest += 1;
    Fsr.Data = AddSingle(FS | ROUND_TO_NEAREST,
                         0x200000,
                         0x200000,
                         &SingleResult);

    if ((Fsr.Data != (FS | ROUND_TO_NEAREST)) ||
        (SingleResult != 0x0)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         0x400000,
                         SINGLE_SIGNAL_NAN_PREFIX,
                         &SingleResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         SINGLE_SIGNAL_NAN_PREFIX,
                         0x400000,
                         &SingleResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         0x400000,
                         SINGLE_INFINITY_VALUE,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         SIGN | 0x400000,
                         SINGLE_INFINITY_VALUE,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         0x400000,
                         MINUS_SINGLE_INFINITY_VALUE,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != MINUS_SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         SIGN | 0x400000,
                         MINUS_SINGLE_INFINITY_VALUE,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != MINUS_SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         SINGLE_INFINITY_VALUE,
                         SINGLE_INFINITY_VALUE,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         SINGLE_INFINITY_VALUE,
                         0x3f800000,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         0x3f800000,
                         SINGLE_INFINITY_VALUE,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         MINUS_SINGLE_INFINITY_VALUE,
                         MINUS_SINGLE_INFINITY_VALUE,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != MINUS_SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         MINUS_SINGLE_INFINITY_VALUE,
                         0x3f800000,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != MINUS_SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         0x3f800000,
                         MINUS_SINGLE_INFINITY_VALUE,
                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != MINUS_SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         SINGLE_INFINITY_VALUE,
                         MINUS_SINGLE_INFINITY_VALUE,
                         &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         MINUS_SINGLE_INFINITY_VALUE,
                         SINGLE_INFINITY_VALUE,
                         &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         SINGLE_QUIET_NAN,
                         SINGLE_QUIET_NAN,
                         &SingleResult);

    if ((Fsr.Data != (ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         SINGLE_QUIET_NAN,
                         0x3f800000,
                         &SingleResult);

    if ((Fsr.Data != (ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         0x3f800000,
                         SINGLE_QUIET_NAN,
                         &SingleResult);

    if ((Fsr.Data != (ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         SINGLE_SIGNAL_NAN,
                         SINGLE_SIGNAL_NAN,
                         &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         SINGLE_SIGNAL_NAN,
                         0x3f800000,
                         &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AddSingle(ROUND_TO_NEAREST,
                         0x3f800000,
                         SINGLE_SIGNAL_NAN,
                         &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    try {
        Fsr.Data = AddSingle(EV | ROUND_TO_NEAREST,
                             SINGLE_QUIET_NAN,
                             SINGLE_QUIET_NAN,
                             &SingleResult);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        goto TestFailed;
    }

    if ((Fsr.Data != (EV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = AddSingle(EV | ROUND_TO_NEAREST,
                             SINGLE_QUIET_NAN,
                             SINGLE_SIGNAL_NAN,
                             &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = AddSingle(EV | ROUND_TO_NEAREST,
                             SINGLE_SIGNAL_NAN,
                             SINGLE_QUIET_NAN,
                             &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = AddSingle(EV | ROUND_TO_NEAREST,
                             SINGLE_INFINITY_VALUE,
                             MINUS_SINGLE_INFINITY_VALUE,
                             &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    //
    // End of test 6.
    //

    printf("succeeded\n");
    return;

    //
    // Test 6 failed.
    //

TestFailed:
    printf(" subtest %d failed, fsr = %lx, result = %lx\n",
           Subtest,
           Fsr.Data,
           SingleResult);

    return;
}

VOID
Test7 (
    VOID
    )

{

    ULONG Count;
    FLOATING_STATUS Fsr;
    ULONG SingleResult;
    ULONG Subtest;

    //
    // Test 7 - Multiply test.
    //

    Subtest = 0;
    printf("    Test 7 - multiply single ...");
    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_NEAREST,
                              SINGLE_INFINITY_VALUE,
                              SINGLE_INFINITY_VALUE,
                              &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_NEAREST,
                              SINGLE_INFINITY_VALUE,
                              MINUS_SINGLE_INFINITY_VALUE,
                              &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != MINUS_SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_NEAREST,
                              MINUS_SINGLE_INFINITY_VALUE,
                              MINUS_SINGLE_INFINITY_VALUE,
                              &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_NEAREST,
                              MINUS_SINGLE_INFINITY_VALUE,
                              SINGLE_INFINITY_VALUE,
                              &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != MINUS_SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_NEAREST,
                              SINGLE_INFINITY_VALUE,
                              0x0,
                              &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_NEAREST,
                              0x0,
                              SINGLE_INFINITY_VALUE,
                              &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_NEAREST,
                              MINUS_SINGLE_INFINITY_VALUE,
                              0x0,
                              &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_NEAREST,
                              0x0,
                              MINUS_SINGLE_INFINITY_VALUE,
                              &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = MultiplySingle(EV | ROUND_TO_NEAREST,
                                  SINGLE_INFINITY_VALUE,
                                  0x0,
                                  &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = MultiplySingle(EV | ROUND_TO_NEAREST,
                                  0x0,
                                  SINGLE_INFINITY_VALUE,
                                  &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = MultiplySingle(EV | ROUND_TO_NEAREST,
                                  MINUS_SINGLE_INFINITY_VALUE,
                                  0x0,
                                  &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = MultiplySingle(EV | ROUND_TO_NEAREST,
                                  0x0,
                                  MINUS_SINGLE_INFINITY_VALUE,
                                  &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_NEAREST,
                              SINGLE_QUIET_NAN,
                              SINGLE_QUIET_NAN,
                              &SingleResult);

    if ((Fsr.Data != (ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_NEAREST,
                              SINGLE_QUIET_NAN,
                              0x3f800000,
                              &SingleResult);

    if ((Fsr.Data != (ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_NEAREST,
                              0x3f800000,
                              SINGLE_QUIET_NAN,
                              &SingleResult);

    if ((Fsr.Data != (ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_NEAREST,
                              SINGLE_SIGNAL_NAN,
                              SINGLE_SIGNAL_NAN,
                              &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_NEAREST,
                              SINGLE_SIGNAL_NAN,
                              0x3f800000,
                              &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_NEAREST,
                              0x3f800000,
                              SINGLE_SIGNAL_NAN,
                              &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    try {
        Fsr.Data = MultiplySingle(EV | ROUND_TO_NEAREST,
                                  SINGLE_QUIET_NAN,
                                  SINGLE_QUIET_NAN,
                                  &SingleResult);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        goto TestFailed;
    }

    if ((Fsr.Data != (EV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = MultiplySingle(EV | ROUND_TO_NEAREST,
                                  SINGLE_QUIET_NAN,
                                  SINGLE_SIGNAL_NAN,
                                  &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = MultiplySingle(EV | ROUND_TO_NEAREST,
                                  SINGLE_SIGNAL_NAN,
                                  SINGLE_QUIET_NAN,
                                  &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_NEAREST,
                              0x7f000000,
                              0x400000,
                              &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x3f800000)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_NEAREST,
                              0x400000,
                              0x7f000000,
                              &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x3f800000)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_NEAREST,
                              0x7f000000,
                              SIGN | 0x400000,
                              &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != (SIGN | 0x3f800000))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_NEAREST,
                              0x400000,
                              SIGN | 0x7f000000,
                              &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != (SIGN | 0x3f800000))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_NEAREST,
                              0x400004,
                              0x7f000001,
                              &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_NEAREST)) ||
        (SingleResult != 0x3f800009)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_NEAREST,
                              0x400004,
                              SIGN | 0x7f000001,
                              &SingleResult);

    if ((Fsr.Data != (SI | ROUND_TO_NEAREST)) ||
        (SingleResult != (SIGN | 0x3f800009))) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = MultiplySingle(EI | ROUND_TO_NEAREST,
                                  0x400004,
                                  0x7f000001,
                                  &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = MultiplySingle(EI | ROUND_TO_NEAREST,
                                  0x400004,
                                  SIGN | 0x7f000001,
                                  &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_NEAREST,
                              0x400000,
                              0x400000,
                              &SingleResult);

    if ((Fsr.Data != (SU | SI | ROUND_TO_NEAREST)) ||
        (SingleResult != 0x0)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_NEAREST,
                              0x400000,
                              SIGN | 0x400000,
                              &SingleResult);

    if ((Fsr.Data != (SU | SI | ROUND_TO_NEAREST)) ||
        (SingleResult != (SIGN | 0x0))) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = MultiplySingle(EI | ROUND_TO_NEAREST,
                                  0x400000,
                                  0x400000,
                                  &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = MultiplySingle(EI | ROUND_TO_NEAREST,
                                  0x400000,
                                  SIGN | 0x400000,
                                  &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = MultiplySingle(EU | ROUND_TO_NEAREST,
                                  0x400000,
                                  0x400000,
                                  &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_UNDERFLOW) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = MultiplySingle(EU | ROUND_TO_NEAREST,
                                  0x400000,
                                  SIGN | 0x400000,
                                  &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_UNDERFLOW) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = MultiplySingle(EU | EI | ROUND_TO_NEAREST,
                                  0x400000,
                                  0x400000,
                                  &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_UNDERFLOW) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = MultiplySingle(EU | EI | ROUND_TO_NEAREST,
                                  0x400000,
                                  SIGN | 0x400000,
                                  &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_UNDERFLOW) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_NEAREST,
                              SINGLE_MAXIMUM_VALUE,
                              SINGLE_MAXIMUM_VALUE,
                              &SingleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_NEAREST,
                              SIGN | SINGLE_MAXIMUM_VALUE,
                              SINGLE_MAXIMUM_VALUE,
                              &SingleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_NEAREST)) ||
        (SingleResult != MINUS_SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_NEAREST,
                              SINGLE_MAXIMUM_VALUE,
                              SIGN | SINGLE_MAXIMUM_VALUE,
                              &SingleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_NEAREST)) ||
        (SingleResult != MINUS_SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_NEAREST,
                              SIGN | SINGLE_MAXIMUM_VALUE,
                              SIGN | SINGLE_MAXIMUM_VALUE,
                              &SingleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_ZERO,
                              SINGLE_MAXIMUM_VALUE,
                              SINGLE_MAXIMUM_VALUE,
                              &SingleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_ZERO)) ||
        (SingleResult != SINGLE_MAXIMUM_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_ZERO,
                              SIGN | SINGLE_MAXIMUM_VALUE,
                              SINGLE_MAXIMUM_VALUE,
                              &SingleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_ZERO)) ||
        (SingleResult != (SIGN | SINGLE_MAXIMUM_VALUE))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_ZERO,
                              SINGLE_MAXIMUM_VALUE,
                              SIGN | SINGLE_MAXIMUM_VALUE,
                              &SingleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_ZERO)) ||
        (SingleResult != (SIGN | SINGLE_MAXIMUM_VALUE))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_ZERO,
                              SIGN | SINGLE_MAXIMUM_VALUE,
                              SIGN | SINGLE_MAXIMUM_VALUE,
                              &SingleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_ZERO)) ||
        (SingleResult != SINGLE_MAXIMUM_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_PLUS_INFINITY,
                              SINGLE_MAXIMUM_VALUE,
                              SINGLE_MAXIMUM_VALUE,
                              &SingleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_PLUS_INFINITY)) ||
        (SingleResult != SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_PLUS_INFINITY,
                              SIGN | SINGLE_MAXIMUM_VALUE,
                              SINGLE_MAXIMUM_VALUE,
                              &SingleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_PLUS_INFINITY)) ||
        (SingleResult != (SIGN | SINGLE_MAXIMUM_VALUE))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_PLUS_INFINITY,
                              SINGLE_MAXIMUM_VALUE,
                              SIGN | SINGLE_MAXIMUM_VALUE,
                              &SingleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_PLUS_INFINITY)) ||
        (SingleResult != (SIGN | SINGLE_MAXIMUM_VALUE))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_PLUS_INFINITY,
                              SIGN | SINGLE_MAXIMUM_VALUE,
                              SIGN | SINGLE_MAXIMUM_VALUE,
                              &SingleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_PLUS_INFINITY)) ||
        (SingleResult != SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_MINUS_INFINITY,
                              SINGLE_MAXIMUM_VALUE,
                              SINGLE_MAXIMUM_VALUE,
                              &SingleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_MINUS_INFINITY)) ||
        (SingleResult != SINGLE_MAXIMUM_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_MINUS_INFINITY,
                              SIGN | SINGLE_MAXIMUM_VALUE,
                              SINGLE_MAXIMUM_VALUE,
                              &SingleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_MINUS_INFINITY)) ||
        (SingleResult != MINUS_SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_MINUS_INFINITY,
                              SINGLE_MAXIMUM_VALUE,
                              SIGN | SINGLE_MAXIMUM_VALUE,
                              &SingleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_MINUS_INFINITY)) ||
        (SingleResult != MINUS_SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MultiplySingle(ROUND_TO_MINUS_INFINITY,
                              SIGN | SINGLE_MAXIMUM_VALUE,
                              SIGN | SINGLE_MAXIMUM_VALUE,
                              &SingleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_MINUS_INFINITY)) ||
        (SingleResult != SINGLE_MAXIMUM_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = MultiplySingle(EI | ROUND_TO_NEAREST,
                                  SINGLE_MAXIMUM_VALUE,
                                  SINGLE_MAXIMUM_VALUE,
                                  &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = MultiplySingle(EI | ROUND_TO_NEAREST,
                                  SINGLE_MAXIMUM_VALUE,
                                  SIGN | SINGLE_MAXIMUM_VALUE,
                                  &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = MultiplySingle(EO | ROUND_TO_NEAREST,
                                  SINGLE_MAXIMUM_VALUE,
                                  SINGLE_MAXIMUM_VALUE,
                                  &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_OVERFLOW) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = MultiplySingle(EO | ROUND_TO_NEAREST,
                                  SINGLE_MAXIMUM_VALUE,
                                  SIGN | SINGLE_MAXIMUM_VALUE,
                                  &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_OVERFLOW) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = MultiplySingle(EO | EI | ROUND_TO_NEAREST,
                                  SINGLE_MAXIMUM_VALUE,
                                  SINGLE_MAXIMUM_VALUE,
                                  &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_OVERFLOW) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = MultiplySingle(EO | EI | ROUND_TO_NEAREST,
                                  SINGLE_MAXIMUM_VALUE,
                                  SIGN | SINGLE_MAXIMUM_VALUE,
                                  &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_OVERFLOW) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    //
    // End of test 7.
    //

    printf("succeeded\n");
    return;

    //
    // Test 7 failed.
    //

TestFailed:
    printf(" subtest %d failed, fsr = %lx, result = %lx\n",
           Subtest,
           Fsr.Data,
           SingleResult);

    return;
}

VOID
Test8 (
    VOID
    )

{

    ULONG Count;
    FLOATING_STATUS Fsr;
    ULONG SingleResult;
    ULONG Subtest;

    //
    // Test 8 - Divide test.
    //

    Subtest = 0;
    printf("    Test 8 - divide single ...");
    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            SINGLE_INFINITY_VALUE,
                            SINGLE_INFINITY_VALUE,
                            &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            SINGLE_INFINITY_VALUE,
                            MINUS_SINGLE_INFINITY_VALUE,
                            &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            MINUS_SINGLE_INFINITY_VALUE,
                            MINUS_SINGLE_INFINITY_VALUE,
                            &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            MINUS_SINGLE_INFINITY_VALUE,
                            SINGLE_INFINITY_VALUE,
                            &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            0x0,
                            0x0,
                            &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            0x0,
                            SIGN | 0x0,
                            &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            SIGN | 0x0,
                            SIGN | 0x0,
                            &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            SIGN | 0x0,
                            0x0,
                            &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            SINGLE_INFINITY_VALUE,
                            0x0,
                            &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            0x3f800000,
                            0x0,
                            &SingleResult);

    if ((Fsr.Data != (SZ | XZ | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }


    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            MINUS_SINGLE_INFINITY_VALUE,
                            0x0,
                            &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != MINUS_SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            SIGN | 0x3f80000,
                            0x0,
                            &SingleResult);

    if ((Fsr.Data != (SZ | XZ | ROUND_TO_NEAREST)) ||
        (SingleResult != MINUS_SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = DivideSingle(EV | ROUND_TO_NEAREST,
                                SINGLE_INFINITY_VALUE,
                                SINGLE_INFINITY_VALUE,
                                &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = DivideSingle(EV | ROUND_TO_NEAREST,
                                0x0,
                                0x0,
                                &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = DivideSingle(EZ | ROUND_TO_NEAREST,
                                0x3f800000,
                                0x0,
                                &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_DIVIDE_BY_ZERO) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            SINGLE_QUIET_NAN,
                            SINGLE_QUIET_NAN,
                            &SingleResult);

    if ((Fsr.Data != (ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            SINGLE_QUIET_NAN,
                            0x3f800000,
                            &SingleResult);

    if ((Fsr.Data != (ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            0x3f800000,
                            SINGLE_QUIET_NAN,
                            &SingleResult);

    if ((Fsr.Data != (ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            SINGLE_SIGNAL_NAN,
                            SINGLE_SIGNAL_NAN,
                            &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            SINGLE_SIGNAL_NAN,
                            0x3f800000,
                            &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            0x3f800000,
                            SINGLE_SIGNAL_NAN,
                            &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    try {
        Fsr.Data = DivideSingle(EV | ROUND_TO_NEAREST,
                                SINGLE_QUIET_NAN,
                                SINGLE_QUIET_NAN,
                                &SingleResult);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        goto TestFailed;
    }

    if ((Fsr.Data != (EV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = DivideSingle(EV | ROUND_TO_NEAREST,
                                SINGLE_QUIET_NAN,
                                SINGLE_SIGNAL_NAN,
                                &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = DivideSingle(EV | ROUND_TO_NEAREST,
                                SINGLE_SIGNAL_NAN,
                                SINGLE_QUIET_NAN,
                                &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            0x400000,
                            SINGLE_INFINITY_VALUE,
                            &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x0)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            SIGN | 0x400000,
                            SINGLE_INFINITY_VALUE,
                            &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != (SIGN | 0x0))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            0x400000,
                            MINUS_SINGLE_INFINITY_VALUE,
                            &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != (SIGN | 0x0))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            SIGN | 0x400000,
                            MINUS_SINGLE_INFINITY_VALUE,
                            &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x0)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            0x400000,
                            0x400000,
                            &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x3f800000)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            SIGN | 0x400000,
                            0x400000,
                            &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != (SIGN | 0x3f800000))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            0x400000,
                            SIGN | 0x400000,
                            &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != (SIGN | 0x3f800000))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            SIGN | 0x400000,
                            SIGN | 0x400000,
                            &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x3f800000)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            0x3f800000,
                            0x400000,
                            &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x7f000000)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            0x40000000,
                            0x400000,
                            &SingleResult);

    if ((Fsr.Data != (SO | SI | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = DivideSingle(ROUND_TO_NEAREST,
                            0x3fffffff,
                            0x400000,
                            &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x7f7fffff)) {
        goto TestFailed;
    }

    //
    // End of test 8.
    //

    printf("succeeded\n");
    return;

    //
    // Test 8 failed.
    //

TestFailed:
    printf(" subtest %d failed, fsr = %lx, result = %lx\n",
           Subtest,
           Fsr.Data,
           SingleResult);

    return;
}

VOID
Test9 (
    VOID
    )

{

    ULONG Count;
    FLOATING_STATUS Fsr;
    ULONG Subtest;

    //
    // Test 9 - Compare single test.
    //

    Subtest = 0;
    printf("    Test 9 - compare single ...");

// ****** //

    Subtest += 1;
    Fsr.Data = CompareLtSingle(ROUND_TO_NEAREST,
                               0x0,
                               0x400000);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareLtSingle(ROUND_TO_NEAREST,
                               SIGN | 0x0,
                               0x400000);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareLtSingle(ROUND_TO_NEAREST,
                               0x400000,
                               0x0);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareLtSingle(ROUND_TO_NEAREST,
                               0X400000,
                               SIGN | 0x0);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

// ****** //

    Subtest += 1;
    Fsr.Data = CompareEqSingle(ROUND_TO_NEAREST,
                               SINGLE_INFINITY_VALUE,
                               SINGLE_INFINITY_VALUE);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareEqSingle(ROUND_TO_NEAREST,
                               0x400000,
                               0x400000);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareEqSingle(ROUND_TO_NEAREST,
                               MINUS_SINGLE_INFINITY_VALUE,
                               SINGLE_INFINITY_VALUE);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareEqSingle(ROUND_TO_NEAREST,
                               0x400000,
                               SIGN | 0x400000);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareLeSingle(ROUND_TO_NEAREST,
                               SINGLE_INFINITY_VALUE,
                               SINGLE_INFINITY_VALUE);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareLeSingle(ROUND_TO_NEAREST,
                               0x400000,
                               0x400000);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareLeSingle(ROUND_TO_NEAREST,
                               MINUS_SINGLE_INFINITY_VALUE,
                               SINGLE_INFINITY_VALUE);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareLeSingle(ROUND_TO_NEAREST,
                               SIGN | 0x400000,
                               0x400000);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareLeSingle(ROUND_TO_NEAREST,
                               SINGLE_INFINITY_VALUE,
                               MINUS_SINGLE_INFINITY_VALUE);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareLeSingle(ROUND_TO_NEAREST,
                               0x400000,
                               SIGN | 0x400000);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareLtSingle(ROUND_TO_NEAREST,
                               SINGLE_INFINITY_VALUE,
                               SINGLE_INFINITY_VALUE);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareLtSingle(ROUND_TO_NEAREST,
                               0x400000,
                               0x400000);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareLtSingle(ROUND_TO_NEAREST,
                               MINUS_SINGLE_INFINITY_VALUE,
                               SINGLE_INFINITY_VALUE);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareLtSingle(ROUND_TO_NEAREST,
                               SIGN | 0x400000,
                               0x400000);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareLtSingle(ROUND_TO_NEAREST,
                               0x400000,
                               0x410000);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareLtSingle(ROUND_TO_NEAREST,
                               0x200000,
                               0x400000);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareLtSingle(ROUND_TO_NEAREST,
                               SIGN | 0x400000,
                               0x400000);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareLtSingle(ROUND_TO_NEAREST,
                               SIGN | 0x410000,
                               SIGN | 0x400000);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareLtSingle(ROUND_TO_NEAREST,
                               SIGN | 0x400000,
                               SIGN | 0x200000);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareLtSingle(ROUND_TO_NEAREST,
                               SIGN | 0x400000,
                               0x400000);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareFSingle(ROUND_TO_NEAREST,
                               0x400000,
                               0x400000);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareUnSingle(ROUND_TO_NEAREST,
                               0x400000,
                               0x400000);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareUnSingle(ROUND_TO_NEAREST,
                               SINGLE_QUIET_NAN,
                               0x400000);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareEqSingle(ROUND_TO_NEAREST,
                               SINGLE_QUIET_NAN,
                               SINGLE_INFINITY_VALUE);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareEqSingle(ROUND_TO_NEAREST,
                               SINGLE_INFINITY_VALUE,
                               SINGLE_INFINITY_VALUE);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareUeqSingle(ROUND_TO_NEAREST,
                                0x400000,
                                SINGLE_INFINITY_VALUE);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareUeqSingle(ROUND_TO_NEAREST,
                                SINGLE_QUIET_NAN,
                                SINGLE_INFINITY_VALUE);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareOltSingle(ROUND_TO_NEAREST,
                                0x400000,
                                SINGLE_QUIET_NAN);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareOltSingle(ROUND_TO_NEAREST,
                                0x400000,
                                SINGLE_INFINITY_VALUE);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareUltSingle(ROUND_TO_NEAREST,
                                0x400000,
                                0x400000);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareUltSingle(ROUND_TO_NEAREST,
                                SINGLE_QUIET_NAN,
                                SINGLE_INFINITY_VALUE);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareOleSingle(ROUND_TO_NEAREST,
                                0x410000,
                                0x400000);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareOleSingle(ROUND_TO_NEAREST,
                                SINGLE_INFINITY_VALUE,
                                SINGLE_INFINITY_VALUE);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareSfSingle(ROUND_TO_NEAREST,
                               0x410000,
                               0x400000);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareSfSingle(ROUND_TO_NEAREST,
                               SINGLE_INFINITY_VALUE,
                               SINGLE_QUIET_NAN);

    if (Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareNgleSingle(ROUND_TO_NEAREST,
                                 0x410000,
                                 0x400000);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareNgleSingle(ROUND_TO_NEAREST,
                                 SINGLE_QUIET_NAN,
                                 SINGLE_INFINITY_VALUE);

    if (Fsr.Data != (CC | SV | XV | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareSeqSingle(ROUND_TO_NEAREST,
                                0x410000,
                                0x400000);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareSeqSingle(ROUND_TO_NEAREST,
                                SINGLE_QUIET_NAN,
                                SINGLE_INFINITY_VALUE);

    if (Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareNglSingle(ROUND_TO_NEAREST,
                                0x410000,
                                0x400000);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareNglSingle(ROUND_TO_NEAREST,
                                SINGLE_QUIET_NAN,
                                SINGLE_INFINITY_VALUE);

    if (Fsr.Data != (CC | SV | XV | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareLtSingle(ROUND_TO_NEAREST,
                               0x410000,
                               0x400000);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareLtSingle(ROUND_TO_NEAREST,
                               SINGLE_QUIET_NAN,
                               SINGLE_INFINITY_VALUE);

    if (Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareNgeSingle(ROUND_TO_NEAREST,
                                0x410000,
                                0x400000);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareNgeSingle(ROUND_TO_NEAREST,
                                SINGLE_QUIET_NAN,
                                SINGLE_INFINITY_VALUE);

    if (Fsr.Data != (CC | SV | XV | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareLeSingle(ROUND_TO_NEAREST,
                               0x410000,
                               0x400000);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareLeSingle(ROUND_TO_NEAREST,
                               SINGLE_QUIET_NAN,
                               SINGLE_INFINITY_VALUE);

    if (Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareNgtSingle(ROUND_TO_NEAREST,
                                0x410000,
                                0x400000);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = CompareNgtSingle(ROUND_TO_NEAREST,
                                SINGLE_QUIET_NAN,
                                SINGLE_INFINITY_VALUE);

    if (Fsr.Data != (CC | SV | XV | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = CompareSfSingle(EV | ROUND_TO_NEAREST,
                                   SINGLE_INFINITY_VALUE,
                                   SINGLE_QUIET_NAN);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = CompareNgleSingle(EV | ROUND_TO_NEAREST,
                                     SINGLE_QUIET_NAN,
                                     SINGLE_INFINITY_VALUE);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = CompareSeqSingle(EV | ROUND_TO_NEAREST,
                                    SINGLE_QUIET_NAN,
                                    SINGLE_INFINITY_VALUE);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = CompareNglSingle(EV | ROUND_TO_NEAREST,
                                    SINGLE_QUIET_NAN,
                                    SINGLE_INFINITY_VALUE);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = CompareLtSingle(EV | ROUND_TO_NEAREST,
                                   SINGLE_QUIET_NAN,
                                   SINGLE_INFINITY_VALUE);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = CompareNgeSingle(EV | ROUND_TO_NEAREST,
                                    SINGLE_QUIET_NAN,
                                    SINGLE_INFINITY_VALUE);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = CompareLeSingle(EV | ROUND_TO_NEAREST,
                                   SINGLE_QUIET_NAN,
                                   SINGLE_INFINITY_VALUE);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = CompareNgtSingle(EV | ROUND_TO_NEAREST,
                                    SINGLE_QUIET_NAN,
                                    SINGLE_INFINITY_VALUE);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = CompareEqSingle(EV | ROUND_TO_NEAREST,
                                   SINGLE_SIGNAL_NAN,
                                   SINGLE_INFINITY_VALUE);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    //
    // End of test 9.
    //

    printf("succeeded\n");
    return;

    //
    // Test 9 failed.
    //

TestFailed:
    printf(" subtest %d failed, fsr = %lx\n", Subtest, Fsr.Data);
    return;
}

VOID
Test10 (
    VOID
    )

{

    ULONG Count;
    FLOATING_STATUS Fsr;
    ULONG Subtest;
    ULONG SingleResult;

    //
    // Test 10 - Absolute, move, and negate single test.
    //

    Subtest = 0;
    printf("    Test 10 - absolute, move, and negate single ...");
    Subtest += 1;
    Fsr.Data = AbsoluteSingle(ROUND_TO_NEAREST,
                              0x400000,
                              &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x400000)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AbsoluteSingle(ROUND_TO_NEAREST,
                              SIGN | 0x400000,
                              &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x400000)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AbsoluteSingle(ROUND_TO_NEAREST,
                              SINGLE_QUIET_NAN,
                              &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = AbsoluteSingle(ROUND_TO_NEAREST,
                              SINGLE_SIGNAL_NAN,
                              &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = AbsoluteSingle(EV | ROUND_TO_NEAREST,
                                  SINGLE_SIGNAL_NAN,
                                  &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MoveSingle(ROUND_TO_NEAREST,
                          0x400000,
                          &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x400000)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MoveSingle(ROUND_TO_NEAREST,
                          SIGN | 0x400000,
                          &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != (SIGN | 0x400000))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MoveSingle(ROUND_TO_NEAREST,
                          SINGLE_QUIET_NAN,
                          &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MoveSingle(ROUND_TO_NEAREST,
                          SINGLE_SIGNAL_NAN,
                          &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != SINGLE_SIGNAL_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = MoveSingle(EV | ROUND_TO_NEAREST,
                          SINGLE_SIGNAL_NAN,
                          &SingleResult);

    if ((Fsr.Data != (EV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_SIGNAL_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = NegateSingle(ROUND_TO_NEAREST,
                            0x400000,
                            &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != (SIGN | 0x400000))) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = NegateSingle(ROUND_TO_NEAREST,
                            SIGN | 0x400000,
                            &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x400000)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = NegateSingle(ROUND_TO_NEAREST,
                            SINGLE_QUIET_NAN,
                            &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = NegateSingle(ROUND_TO_NEAREST,
                            SINGLE_SIGNAL_NAN,
                            &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = NegateSingle(EV | ROUND_TO_NEAREST,
                                SINGLE_SIGNAL_NAN,
                                &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    //
    // End of test 10.
    //

    printf("succeeded\n");
    return;

    //
    // Test 10 failed.
    //

TestFailed:
    printf(" subtest %d failed, fsr = %lx, result = %lx\n",
           Subtest,
           Fsr.Data,
           SingleResult);

    return;
}

VOID
Test11 (
    VOID
    )

{

    ULONG Count;
    ULARGE_INTEGER DoubleOperand1;
    ULARGE_INTEGER DoubleOperand2;
    ULARGE_INTEGER DoubleResult;
    FLOATING_STATUS Fsr;
    ULONG Subtest;

    //
    // Test 11 - Add double denormalized test.
    //

    Subtest = 0;
    printf("    Test 11 - add/subtract double denormalized ...");
    Subtest += 1;
    DoubleOperand1.LowPart = 0x3ff;
    DoubleOperand1.HighPart = 0x0;
    DoubleOperand2.LowPart = 0x1;
    DoubleOperand2.HighPart = 0x0;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x400) ||
        (DoubleResult.HighPart != 0x0)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x1;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x7fff;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x8000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x84000;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != (SIGN | 0x4000))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x84000;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x4000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x80000;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x0)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != (SIGN | 0x0))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x100000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x100000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x1fffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x1600000;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x40000000) ||
        (DoubleResult.HighPart != 0x1600000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x2600000;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x4000) ||
        (DoubleResult.HighPart != 0x2600000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x3f000000;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x3f000000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x100000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x3ff00000;
    Count = 0;
    try {
        Fsr.Data = AddDouble(EI | ROUND_TO_NEAREST,
                             &DoubleOperand1,
                             &DoubleOperand2,
                             &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x7fe00000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x7fe00000;
    Count = 0;
    try {
        Fsr.Data = AddDouble(EO | ROUND_TO_NEAREST,
                             &DoubleOperand1,
                             &DoubleOperand2,
                             &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_OVERFLOW) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x7fe00000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x7fe00000;
    Count = 0;
    try {
        Fsr.Data = AddDouble(EI | ROUND_TO_NEAREST,
                             &DoubleOperand1,
                             &DoubleOperand2,
                             &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x7fe00000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x7fe00000;
    Count = 0;
    try {
        Fsr.Data = AddDouble(EI | EO | ROUND_TO_NEAREST,
                             &DoubleOperand1,
                             &DoubleOperand2,
                             &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_OVERFLOW) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x3ff;
    DoubleOperand1.HighPart = 0x0;
    DoubleOperand2.LowPart = 0x1;
    DoubleOperand2.HighPart = SIGN;
    Fsr.Data = SubtractDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x400) ||
        (DoubleResult.HighPart != 0x0)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x1;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x7fff;
    Fsr.Data = SubtractDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x8000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x84000;
    Fsr.Data = SubtractDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != (SIGN | 0x4000))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x84000;
    Fsr.Data = SubtractDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x4000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = SubtractDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x0)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x80000;
    Fsr.Data = SubtractDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != (SIGN | 0x0))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x80000;
    Fsr.Data = SubtractDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x100000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x100000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0xfffff;
    Fsr.Data = SubtractDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x1fffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x1600000;
    Fsr.Data = SubtractDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x40000000) ||
        (DoubleResult.HighPart != 0x1600000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x2600000;
    Fsr.Data = SubtractDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x4000) ||
        (DoubleResult.HighPart != 0x2600000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x3f000000;
    Fsr.Data = SubtractDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x3f000000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x3ff00000;
    Count = 0;
    try {
        Fsr.Data = SubtractDouble(EI | ROUND_TO_NEAREST,
                                  &DoubleOperand1,
                                  &DoubleOperand2,
                                  &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x7fe00000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x7fe00000;
    Count = 0;
    try {
        Fsr.Data = SubtractDouble(EO | ROUND_TO_NEAREST,
                                  &DoubleOperand1,
                                  &DoubleOperand2,
                                  &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_OVERFLOW) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x7fe00000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x7fe00000;
    Count = 0;
    try {
        Fsr.Data = SubtractDouble(EI | ROUND_TO_NEAREST,
                                  &DoubleOperand1,
                                  &DoubleOperand2,
                                  &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x7fe00000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x7fe00000;
    Count = 0;
    try {
        Fsr.Data = SubtractDouble(EI | EO | ROUND_TO_NEAREST,
                                  &DoubleOperand1,
                                  &DoubleOperand2,
                                  &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_OVERFLOW) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    //
    // End of test 11.
    //

    printf("succeeded\n");
    return;

    //
    // Test 11 failed.
    //

TestFailed:
    printf(" subtest %d failed, fsr = %lx, result = %lx, %lx\n",
           Subtest,
           Fsr.Data,
           DoubleResult.LowPart,
           DoubleResult.HighPart);

    return;
}

VOID
Test12 (
    VOID
    )

{

    ULONG Count;
    ULARGE_INTEGER DoubleOperand1;
    ULARGE_INTEGER DoubleOperand2;
    ULARGE_INTEGER DoubleResult;
    FLOATING_STATUS Fsr;
    ULONG Subtest;

    //
    // Test 12 - Add double round to nearest test.
    //

    Subtest = 0;
    printf("    Test 12 - add double round to nearest ...");
    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffff8;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0xfffffffe) ||
        (DoubleResult.HighPart != 0x33ffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffff9;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != 0xfffffffe) ||
        (DoubleResult.HighPart != 0x33ffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffffa;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != 0xfffffffe) ||
        (DoubleResult.HighPart != 0x33ffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffffb;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != 0xffffffff) ||
        (DoubleResult.HighPart != 0x33ffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffffc;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0xffffffff) ||
        (DoubleResult.HighPart != 0x33ffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffffd;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != 0xffffffff) ||
        (DoubleResult.HighPart != 0x33ffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffffe;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x340000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xffffffff;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x340000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xffffffff;
    DoubleOperand2.HighPart = 0xfffff;
    Count = 0;
    try {
        Fsr.Data = AddDouble(EI | ROUND_TO_NEAREST,
                             &DoubleOperand1,
                             &DoubleOperand2,
                             &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    //
    // End of test 12.
    //

    printf("succeeded\n");
    return;

    //
    // Test 12 failed.
    //

TestFailed:
    printf(" subtest %d failed, fsr = %lx, result = %lx, %lx\n",
           Subtest,
           Fsr.Data,
           DoubleResult.LowPart,
           DoubleResult.HighPart);

    return;
}

VOID
Test13 (
    VOID
    )

{

    ULONG Count;
    ULARGE_INTEGER DoubleOperand1;
    ULARGE_INTEGER DoubleOperand2;
    ULARGE_INTEGER DoubleResult;
    FLOATING_STATUS Fsr;
    ULONG Subtest;

    //
    // Test 13 - Add double round to zero test.
    //

    Subtest = 0;
    printf("    Test 13 - add double round to zero ...");
    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffff8;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_ZERO,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_ZERO) ||
        (DoubleResult.LowPart != 0xfffffffe) ||
        (DoubleResult.HighPart != 0x33ffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffff9;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_ZERO,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_ZERO)) ||
        (DoubleResult.LowPart != 0xfffffffe) ||
        (DoubleResult.HighPart != 0x33ffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffffa;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_ZERO,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_ZERO)) ||
        (DoubleResult.LowPart != 0xfffffffe) ||
        (DoubleResult.HighPart != 0x33ffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffffb;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_ZERO,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_ZERO)) ||
        (DoubleResult.LowPart != 0xfffffffe) ||
        (DoubleResult.HighPart != 0x33ffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffffc;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_ZERO,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_ZERO) ||
        (DoubleResult.LowPart != 0xffffffff) ||
        (DoubleResult.HighPart != 0x33ffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffffd;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_ZERO,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_ZERO)) ||
        (DoubleResult.LowPart != 0xffffffff) ||
        (DoubleResult.HighPart != 0x33ffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffffe;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_ZERO,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_ZERO)) ||
        (DoubleResult.LowPart != 0xffffffff) ||
        (DoubleResult.HighPart != 0x33ffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xffffffff;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_ZERO,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_ZERO)) ||
        (DoubleResult.LowPart != 0xffffffff) ||
        (DoubleResult.HighPart != 0x33ffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xffffffff;
    DoubleOperand2.HighPart = 0xfffff;
    Count = 0;
    try {
        Fsr.Data = AddDouble(EI | ROUND_TO_ZERO,
                             &DoubleOperand1,
                             &DoubleOperand2,
                             &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    //
    // End of test 13.
    //

    printf("succeeded\n");
    return;

    //
    // Test 13 failed.
    //

TestFailed:
    printf(" subtest %d failed, fsr = %lx, result = %lx, %lx\n",
           Subtest,
           Fsr.Data,
           DoubleResult.LowPart,
           DoubleResult.HighPart);

    return;
}

VOID
Test14 (
    VOID
    )

{

    ULONG Count;
    ULARGE_INTEGER DoubleOperand1;
    ULARGE_INTEGER DoubleOperand2;
    FLOATING_STATUS Fsr;
    ULARGE_INTEGER DoubleResult;
    ULONG Subtest;

    //
    // Test 14 - Add double round to positive infinity test.
    //

    Subtest = 0;
    printf("    Test 14 - add double round to positive infinity ...");
    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffff8;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_PLUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_PLUS_INFINITY) ||
        (DoubleResult.LowPart != 0xfffffffe) ||
        (DoubleResult.HighPart != 0x33ffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffff9;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_PLUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_PLUS_INFINITY)) ||
        (DoubleResult.LowPart != 0xffffffff) ||
        (DoubleResult.HighPart != 0x33ffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffffa;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_PLUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_PLUS_INFINITY)) ||
        (DoubleResult.LowPart != 0xffffffff) ||
        (DoubleResult.HighPart != 0x33ffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffffb;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_PLUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_PLUS_INFINITY)) ||
        (DoubleResult.LowPart != 0xffffffff) ||
        (DoubleResult.HighPart != 0x33ffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffffc;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_PLUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_PLUS_INFINITY) ||
        (DoubleResult.LowPart != 0xffffffff) ||
        (DoubleResult.HighPart != 0x33ffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffffd;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_PLUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_PLUS_INFINITY)) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x340000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffffe;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_PLUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_PLUS_INFINITY)) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x340000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xffffffff;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_PLUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_PLUS_INFINITY)) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x340000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xffffffff;
    DoubleOperand2.HighPart = 0xfffff;
    Count = 0;
    try {
        Fsr.Data = AddDouble(EI | ROUND_TO_PLUS_INFINITY,
                             &DoubleOperand1,
                             &DoubleOperand2,
                             &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x300000;
    DoubleOperand2.LowPart = 0xfffffff8;
    DoubleOperand2.HighPart = SIGN | 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_PLUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_PLUS_INFINITY) ||
        (DoubleResult.LowPart != 0xfffffffe) ||
        (DoubleResult.HighPart != (SIGN | 0x33ffff))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x300000;
    DoubleOperand2.LowPart = 0xfffffff9;
    DoubleOperand2.HighPart = SIGN | 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_PLUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_PLUS_INFINITY)) ||
        (DoubleResult.LowPart != 0xfffffffe) ||
        (DoubleResult.HighPart != (SIGN | 0x33ffff))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x300000;
    DoubleOperand2.LowPart = 0xfffffffa;
    DoubleOperand2.HighPart = SIGN | 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_PLUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_PLUS_INFINITY)) ||
        (DoubleResult.LowPart != 0xfffffffe) ||
        (DoubleResult.HighPart != (SIGN | 0x33ffff))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x300000;
    DoubleOperand2.LowPart = 0xfffffffb;
    DoubleOperand2.HighPart = SIGN | 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_PLUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_PLUS_INFINITY)) ||
        (DoubleResult.LowPart != 0xfffffffe) ||
        (DoubleResult.HighPart != (SIGN | 0x33ffff))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x300000;
    DoubleOperand2.LowPart = 0xfffffffc;
    DoubleOperand2.HighPart = SIGN | 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_PLUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_PLUS_INFINITY) ||
        (DoubleResult.LowPart != 0xffffffff) ||
        (DoubleResult.HighPart != (SIGN | 0x33ffff))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x300000;
    DoubleOperand2.LowPart = 0xfffffffd;
    DoubleOperand2.HighPart = SIGN | 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_PLUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_PLUS_INFINITY)) ||
        (DoubleResult.LowPart != 0xffffffff) ||
        (DoubleResult.HighPart != (SIGN | 0x33ffff))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x300000;
    DoubleOperand2.LowPart = 0xfffffffe;
    DoubleOperand2.HighPart = SIGN | 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_PLUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_PLUS_INFINITY)) ||
        (DoubleResult.LowPart != 0xffffffff) ||
        (DoubleResult.HighPart != (SIGN | 0x33ffff))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x300000;
    DoubleOperand2.LowPart = 0xffffffff;
    DoubleOperand2.HighPart = SIGN | 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_PLUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_PLUS_INFINITY)) ||
        (DoubleResult.LowPart != 0xffffffff) ||
        (DoubleResult.HighPart != (SIGN | 0x33ffff))) {
        goto TestFailed;
    }

    //
    // End of test 14.
    //

    printf("succeeded\n");
    return;

    //
    // Test 14 failed.
    //

TestFailed:
    printf(" subtest %d failed, fsr = %lx, result = %lx, %lx\n",
           Subtest,
           Fsr.Data,
           DoubleResult.LowPart,
           DoubleResult.HighPart);

    return;
}

VOID
Test15 (
    VOID
    )

{

    ULONG Count;
    ULARGE_INTEGER DoubleOperand1;
    ULARGE_INTEGER DoubleOperand2;
    ULARGE_INTEGER DoubleResult;
    FLOATING_STATUS Fsr;
    ULONG Subtest;

    //
    // Test 15 - Add double round to negative infinity test.
    //

    Subtest = 0;
    printf("    Test 15 - add double round to negative infinity ...");
    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x300000;
    DoubleOperand2.LowPart = 0xfffffff8;
    DoubleOperand2.HighPart = SIGN | 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_MINUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_MINUS_INFINITY) ||
        (DoubleResult.LowPart != 0xfffffffe) ||
        (DoubleResult.HighPart != (SIGN | 0x33ffff))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x300000;
    DoubleOperand2.LowPart = 0xfffffff9;
    DoubleOperand2.HighPart = SIGN | 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_MINUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_MINUS_INFINITY)) ||
        (DoubleResult.LowPart != 0xffffffff) ||
        (DoubleResult.HighPart != (SIGN | 0x33ffff))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x300000;
    DoubleOperand2.LowPart = 0xfffffffa;
    DoubleOperand2.HighPart = SIGN | 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_MINUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_MINUS_INFINITY)) ||
        (DoubleResult.LowPart != 0xffffffff) ||
        (DoubleResult.HighPart != (SIGN | 0x33ffff))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x300000;
    DoubleOperand2.LowPart = 0xfffffffb;
    DoubleOperand2.HighPart = SIGN | 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_MINUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_MINUS_INFINITY)) ||
        (DoubleResult.LowPart != 0xffffffff) ||
        (DoubleResult.HighPart != (SIGN | 0x33ffff))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x300000;
    DoubleOperand2.LowPart = 0xfffffffc;
    DoubleOperand2.HighPart = SIGN | 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_MINUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_MINUS_INFINITY) ||
        (DoubleResult.LowPart != 0xffffffff) ||
        (DoubleResult.HighPart != (SIGN | 0x33ffff))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x300000;
    DoubleOperand2.LowPart = 0xfffffffd;
    DoubleOperand2.HighPart = SIGN | 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_MINUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_MINUS_INFINITY)) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != (SIGN | 0x340000))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x300000;
    DoubleOperand2.LowPart = 0xfffffffe;
    DoubleOperand2.HighPart = SIGN | 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_MINUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_MINUS_INFINITY)) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != (SIGN | 0x340000))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x300000;
    DoubleOperand2.LowPart = 0xffffffff;
    DoubleOperand2.HighPart = SIGN | 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_MINUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_MINUS_INFINITY)) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != (SIGN | 0x340000))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x300000;
    DoubleOperand2.LowPart = 0xffffffff;
    DoubleOperand2.HighPart = SIGN | 0xfffff;
    Count = 0;
    try {
        Fsr.Data = AddDouble(EI | ROUND_TO_MINUS_INFINITY,
                             &DoubleOperand1,
                             &DoubleOperand2,
                             &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffff8;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_MINUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_MINUS_INFINITY) ||
        (DoubleResult.LowPart != 0xfffffffe) ||
        (DoubleResult.HighPart != 0x33ffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffff9;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_MINUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_MINUS_INFINITY)) ||
        (DoubleResult.LowPart != 0xfffffffe) ||
        (DoubleResult.HighPart != 0x33ffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffffa;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_MINUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_MINUS_INFINITY)) ||
        (DoubleResult.LowPart != 0xfffffffe) ||
        (DoubleResult.HighPart != 0x33ffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffffb;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_MINUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_MINUS_INFINITY)) ||
        (DoubleResult.LowPart != 0xfffffffe) ||
        (DoubleResult.HighPart != 0x33ffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffffc;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_MINUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_MINUS_INFINITY) ||
        (DoubleResult.LowPart != 0xffffffff) ||
        (DoubleResult.HighPart != 0x33ffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffffd;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_MINUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_MINUS_INFINITY)) ||
        (DoubleResult.LowPart != 0xffffffff) ||
        (DoubleResult.HighPart != 0x33ffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xfffffffe;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_MINUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_MINUS_INFINITY)) ||
        (DoubleResult.LowPart != 0xffffffff) ||
        (DoubleResult.HighPart != 0x33ffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x300000;
    DoubleOperand2.LowPart = 0xffffffff;
    DoubleOperand2.HighPart = 0xfffff;
    Fsr.Data = AddDouble(ROUND_TO_MINUS_INFINITY,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SI | ROUND_TO_MINUS_INFINITY)) ||
        (DoubleResult.LowPart != 0xffffffff) ||
        (DoubleResult.HighPart != 0x33ffff)) {
        goto TestFailed;
    }

    //
    // End of test 15.
    //

    printf("succeeded\n");
    return;

    //
    // Test 15 failed.
    //

TestFailed:
    printf(" subtest %d failed, fsr = %lx, result = %lx, %lx\n",
           Subtest,
           Fsr.Data,
           DoubleResult.LowPart,
           DoubleResult.HighPart);

    return;
}

VOID
Test16 (
    VOID
    )

{

    ULONG Count;
    ULARGE_INTEGER DoubleOperand1;
    ULARGE_INTEGER DoubleOperand2;
    ULARGE_INTEGER DoubleResult;
    FLOATING_STATUS Fsr;
    ULONG Subtest;

    //
    // Test 16 - Add double infinity and NaN test.
    //

    Subtest = 0;
    printf("    Test 16 - add double infinity and NaN ...");

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x40000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x40000;
    Fsr.Data = AddDouble(FS | ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (FS | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x0)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = DOUBLE_SIGNAL_NAN_PREFIX;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = DOUBLE_SIGNAL_NAN_PREFIX;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != DOUBLE_INFINITY_VALUE_HIGH)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x80000;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != DOUBLE_INFINITY_VALUE_HIGH)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != MINUS_DOUBLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x80000;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != MINUS_DOUBLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != DOUBLE_INFINITY_VALUE_HIGH)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x3ff00000;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != DOUBLE_INFINITY_VALUE_HIGH)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x3ff00000;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != DOUBLE_INFINITY_VALUE_HIGH)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != MINUS_DOUBLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x3ff00000;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != MINUS_DOUBLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x3ff00000;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != MINUS_DOUBLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand2.HighPart = DOUBLE_QUIET_NAN;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x3ff00000;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x3ff00000;
    DoubleOperand2.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand2.HighPart = DOUBLE_QUIET_NAN;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_SIGNAL_NAN;
    DoubleOperand2.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand2.HighPart = DOUBLE_SIGNAL_NAN;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_SIGNAL_NAN;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x3ff00000;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x3ff00000;
    DoubleOperand2.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand2.HighPart = DOUBLE_SIGNAL_NAN;
    Fsr.Data = AddDouble(ROUND_TO_NEAREST,
                         &DoubleOperand1,
                         &DoubleOperand2,
                         &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand2.HighPart = DOUBLE_QUIET_NAN;
    try {
        Fsr.Data = AddDouble(EV | ROUND_TO_NEAREST,
                             &DoubleOperand1,
                             &DoubleOperand2,
                             &DoubleResult);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        goto TestFailed;
    }

    if ((Fsr.Data != (EV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand2.HighPart = DOUBLE_SIGNAL_NAN;
    Count = 0;
    try {
        Fsr.Data = AddDouble(EV | ROUND_TO_NEAREST,
                             &DoubleOperand1,
                             &DoubleOperand2,
                             &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_SIGNAL_NAN;
    DoubleOperand2.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand2.HighPart = DOUBLE_QUIET_NAN;
    Count = 0;
    try {
        Fsr.Data = AddDouble(EV | ROUND_TO_NEAREST,
                             &DoubleOperand1,
                             &DoubleOperand2,
                             &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    Count = 0;
    try {
        Fsr.Data = AddDouble(EV | ROUND_TO_NEAREST,
                             &DoubleOperand1,
                             &DoubleOperand2,
                             &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    //
    // End of test 16.
    //

    printf("succeeded\n");
    return;

    //
    // Test 16 failed.
    //

TestFailed:
    printf(" subtest %d failed, fsr = %lx, result = %lx, %lx\n",
           Subtest,
           Fsr.Data,
           DoubleResult.LowPart,
           DoubleResult.HighPart);

    return;
}

VOID
Test17 (
    VOID
    )

{

    ULONG Count;
    ULARGE_INTEGER DoubleOperand1;
    ULARGE_INTEGER DoubleOperand2;
    ULARGE_INTEGER DoubleResult;
    FLOATING_STATUS Fsr;
    ULONG Subtest;

    //
    // Test 17 - Multiply double test.
    //

    Subtest = 0;
    printf("    Test 17 - multiply double ...");
    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = MultiplyDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != DOUBLE_INFINITY_VALUE_HIGH)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    Fsr.Data = MultiplyDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != MINUS_DOUBLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    Fsr.Data = MultiplyDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != DOUBLE_INFINITY_VALUE_HIGH)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = MultiplyDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != MINUS_DOUBLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x0;
    Fsr.Data = MultiplyDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x0;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = MultiplyDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x0;
    Fsr.Data = MultiplyDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x0;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    Fsr.Data = MultiplyDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x0;
    Count = 0;
    try {
        Fsr.Data = MultiplyDouble(EV | ROUND_TO_NEAREST,
                                  &DoubleOperand1,
                                  &DoubleOperand2,
                                  &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x0;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Count = 0;
    try {
        Fsr.Data = MultiplyDouble(EV | ROUND_TO_NEAREST,
                                  &DoubleOperand1,
                                  &DoubleOperand2,
                                  &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x0;
    Count = 0;
    try {
        Fsr.Data = MultiplyDouble(EV | ROUND_TO_NEAREST,
                                  &DoubleOperand1,
                                  &DoubleOperand2,
                                  &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x0;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    Count = 0;
    try {
        Fsr.Data = MultiplyDouble(EV | ROUND_TO_NEAREST,
                                  &DoubleOperand1,
                                  &DoubleOperand2,
                                  &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand2.HighPart = DOUBLE_QUIET_NAN;
    Fsr.Data = MultiplyDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x3ff00000;
    Fsr.Data = MultiplyDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x3ff00000;
    DoubleOperand2.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand2.HighPart = DOUBLE_QUIET_NAN;
    Fsr.Data = MultiplyDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_SIGNAL_NAN;
    DoubleOperand2.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand2.HighPart = DOUBLE_SIGNAL_NAN;
    Fsr.Data = MultiplyDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_SIGNAL_NAN;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x3ff00000;
    Fsr.Data = MultiplyDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x3ff00000;
    DoubleOperand2.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand2.HighPart = DOUBLE_SIGNAL_NAN;
    Fsr.Data = MultiplyDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand2.HighPart = DOUBLE_QUIET_NAN;
    try {
        Fsr.Data = MultiplyDouble(EV | ROUND_TO_NEAREST,
                                  &DoubleOperand1,
                                  &DoubleOperand2,
                                  &DoubleResult);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        goto TestFailed;
    }

    if ((Fsr.Data != (EV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand2.HighPart = DOUBLE_SIGNAL_NAN;
    Count = 0;
    try {
        Fsr.Data = MultiplyDouble(EV | ROUND_TO_NEAREST,
                                  &DoubleOperand1,
                                  &DoubleOperand2,
                                  &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_SIGNAL_NAN;
    DoubleOperand2.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand2.HighPart = DOUBLE_QUIET_NAN;
    Count = 0;
    try {
        Fsr.Data = MultiplyDouble(EV | ROUND_TO_NEAREST,
                                  &DoubleOperand1,
                                  &DoubleOperand2,
                                  &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x7fe00000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = MultiplyDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x3ff00000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x7fe00000;
    Fsr.Data = MultiplyDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x3ff00000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x7fe00000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x80000;
    Fsr.Data = MultiplyDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != (SIGN | 0x3ff00000))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x7fe00000;
    Fsr.Data = MultiplyDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != (SIGN | 0x3ff00000))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80008;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x7fe00001;
    Fsr.Data = MultiplyDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x10000) ||
        (DoubleResult.HighPart != 0x3ff00011)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80008;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x7fe00001;
    Fsr.Data = MultiplyDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x10000) ||
        (DoubleResult.HighPart != (SIGN | 0x3ff00011))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80008;
    DoubleOperand2.LowPart = 0x1;
    DoubleOperand2.HighPart = 0x7fe00001;
    Count = 0;
    try {
        Fsr.Data = MultiplyDouble(EI | ROUND_TO_NEAREST,
                                  &DoubleOperand1,
                                  &DoubleOperand2,
                                  &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80008;
    DoubleOperand2.LowPart = 0x1;
    DoubleOperand2.HighPart = SIGN | 0x7fe00001;
    Count = 0;
    try {
        Fsr.Data = MultiplyDouble(EI | ROUND_TO_NEAREST,
                                  &DoubleOperand1,
                                  &DoubleOperand2,
                                  &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = MultiplyDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (SU | SI | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x0)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x80000;
    Fsr.Data = MultiplyDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (SU | SI | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != (SIGN | 0x0))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Count = 0;
    try {
        Fsr.Data = MultiplyDouble(EI | ROUND_TO_NEAREST,
                                  &DoubleOperand1,
                                  &DoubleOperand2,
                                  &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x80000;
    Count = 0;
    try {
        Fsr.Data = MultiplyDouble(EI | ROUND_TO_NEAREST,
                                  &DoubleOperand1,
                                  &DoubleOperand2,
                                  &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Count = 0;
    try {
        Fsr.Data = MultiplyDouble(EU | ROUND_TO_NEAREST,
                                  &DoubleOperand1,
                                  &DoubleOperand2,
                                  &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_UNDERFLOW) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x80000;
    Count = 0;
    try {
        Fsr.Data = MultiplyDouble(EU | ROUND_TO_NEAREST,
                                  &DoubleOperand1,
                                  &DoubleOperand2,
                                  &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_UNDERFLOW) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Count = 0;
    try {
        Fsr.Data = MultiplyDouble(EU | EI | ROUND_TO_NEAREST,
                                  &DoubleOperand1,
                                  &DoubleOperand2,
                                  &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_UNDERFLOW) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x80000;
    Count = 0;
    try {
        Fsr.Data = MultiplyDouble(EU | EI | ROUND_TO_NEAREST,
                                  &DoubleOperand1,
                                  &DoubleOperand2,
                                  &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_UNDERFLOW) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_MAXIMUM_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_MAXIMUM_VALUE_HIGH;
    Fsr.Data = MultiplyDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != DOUBLE_INFINITY_VALUE_HIGH)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand1.HighPart = SIGN | DOUBLE_MAXIMUM_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_MAXIMUM_VALUE_HIGH;
    Fsr.Data = MultiplyDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != MINUS_DOUBLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_MAXIMUM_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand2.HighPart = SIGN | DOUBLE_MAXIMUM_VALUE_HIGH;
    Fsr.Data = MultiplyDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != MINUS_DOUBLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand1.HighPart = SIGN | DOUBLE_MAXIMUM_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand2.HighPart = SIGN | DOUBLE_MAXIMUM_VALUE_HIGH;
    Fsr.Data = MultiplyDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != DOUBLE_INFINITY_VALUE_HIGH)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_MAXIMUM_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_MAXIMUM_VALUE_HIGH;
    Fsr.Data = MultiplyDouble(ROUND_TO_ZERO,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_ZERO)) ||
        (DoubleResult.LowPart != DOUBLE_MAXIMUM_VALUE_LOW) ||
        (DoubleResult.HighPart != DOUBLE_MAXIMUM_VALUE_HIGH)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand1.HighPart = SIGN | DOUBLE_MAXIMUM_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_MAXIMUM_VALUE_HIGH;
    Fsr.Data = MultiplyDouble(ROUND_TO_ZERO,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_ZERO)) ||
        (DoubleResult.LowPart != DOUBLE_MAXIMUM_VALUE_LOW) ||
        (DoubleResult.HighPart != (SIGN | DOUBLE_MAXIMUM_VALUE_HIGH))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_MAXIMUM_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand2.HighPart = SIGN | DOUBLE_MAXIMUM_VALUE_HIGH;
    Fsr.Data = MultiplyDouble(ROUND_TO_ZERO,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_ZERO)) ||
        (DoubleResult.LowPart != DOUBLE_MAXIMUM_VALUE_LOW) ||
        (DoubleResult.HighPart != (SIGN | DOUBLE_MAXIMUM_VALUE_HIGH))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand1.HighPart = SIGN | DOUBLE_MAXIMUM_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand2.HighPart = SIGN | DOUBLE_MAXIMUM_VALUE_HIGH;
    Fsr.Data = MultiplyDouble(ROUND_TO_ZERO,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_ZERO)) ||
        (DoubleResult.LowPart != DOUBLE_MAXIMUM_VALUE_LOW) ||
        (DoubleResult.HighPart != DOUBLE_MAXIMUM_VALUE_HIGH)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_MAXIMUM_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_MAXIMUM_VALUE_HIGH;
    Fsr.Data = MultiplyDouble(ROUND_TO_PLUS_INFINITY,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_PLUS_INFINITY)) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != DOUBLE_INFINITY_VALUE_HIGH)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand1.HighPart = SIGN | DOUBLE_MAXIMUM_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_MAXIMUM_VALUE_HIGH;
    Fsr.Data = MultiplyDouble(ROUND_TO_PLUS_INFINITY,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_PLUS_INFINITY)) ||
        (DoubleResult.LowPart != DOUBLE_MAXIMUM_VALUE_LOW) ||
        (DoubleResult.HighPart != (SIGN | DOUBLE_MAXIMUM_VALUE_HIGH))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_MAXIMUM_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand2.HighPart = SIGN | DOUBLE_MAXIMUM_VALUE_HIGH;
    Fsr.Data = MultiplyDouble(ROUND_TO_PLUS_INFINITY,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_PLUS_INFINITY)) ||
        (DoubleResult.LowPart != DOUBLE_MAXIMUM_VALUE_LOW) ||
        (DoubleResult.HighPart != (SIGN | DOUBLE_MAXIMUM_VALUE_HIGH))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand1.HighPart = SIGN | DOUBLE_MAXIMUM_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand2.HighPart = SIGN | DOUBLE_MAXIMUM_VALUE_HIGH;
    Fsr.Data = MultiplyDouble(ROUND_TO_PLUS_INFINITY,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_PLUS_INFINITY)) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != DOUBLE_INFINITY_VALUE_HIGH)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_MAXIMUM_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_MAXIMUM_VALUE_HIGH;
    Fsr.Data = MultiplyDouble(ROUND_TO_MINUS_INFINITY,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_MINUS_INFINITY)) ||
        (DoubleResult.LowPart != DOUBLE_MAXIMUM_VALUE_LOW) ||
        (DoubleResult.HighPart != DOUBLE_MAXIMUM_VALUE_HIGH)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand1.HighPart = SIGN | DOUBLE_MAXIMUM_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_MAXIMUM_VALUE_HIGH;
    Fsr.Data = MultiplyDouble(ROUND_TO_MINUS_INFINITY,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_MINUS_INFINITY)) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != MINUS_DOUBLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_MAXIMUM_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand2.HighPart = SIGN | DOUBLE_MAXIMUM_VALUE_HIGH;
    Fsr.Data = MultiplyDouble(ROUND_TO_MINUS_INFINITY,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_MINUS_INFINITY)) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != MINUS_DOUBLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand1.HighPart = SIGN | DOUBLE_MAXIMUM_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand2.HighPart = SIGN | DOUBLE_MAXIMUM_VALUE_HIGH;
    Fsr.Data = MultiplyDouble(ROUND_TO_MINUS_INFINITY,
                              &DoubleOperand1,
                              &DoubleOperand2,
                              &DoubleResult);

    if ((Fsr.Data != (SO | SI | XO | XI | ROUND_TO_MINUS_INFINITY)) ||
        (DoubleResult.LowPart != DOUBLE_MAXIMUM_VALUE_LOW) ||
        (DoubleResult.HighPart != DOUBLE_MAXIMUM_VALUE_HIGH)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand1.HighPart = SIGN | DOUBLE_MAXIMUM_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand2.HighPart = SIGN | DOUBLE_MAXIMUM_VALUE_HIGH;
    Count = 0;
    try {
        Fsr.Data = MultiplyDouble(EI | ROUND_TO_NEAREST,
                                  &DoubleOperand1,
                                  &DoubleOperand2,
                                  &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_MAXIMUM_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand2.HighPart = SIGN | DOUBLE_MAXIMUM_VALUE_HIGH;
    Count = 0;
    try {
        Fsr.Data = MultiplyDouble(EI | ROUND_TO_NEAREST,
                                  &DoubleOperand1,
                                  &DoubleOperand2,
                                  &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_MAXIMUM_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_MAXIMUM_VALUE_HIGH;
    Count = 0;
    try {
        Fsr.Data = MultiplyDouble(EO | ROUND_TO_NEAREST,
                                  &DoubleOperand1,
                                  &DoubleOperand2,
                                  &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_OVERFLOW) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_MAXIMUM_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand2.HighPart = SIGN | DOUBLE_MAXIMUM_VALUE_HIGH;
    Count = 0;
    try {
        Fsr.Data = MultiplyDouble(EO | ROUND_TO_NEAREST,
                                  &DoubleOperand1,
                                  &DoubleOperand2,
                                  &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_OVERFLOW) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_MAXIMUM_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_MAXIMUM_VALUE_HIGH;
    Count = 0;
    try {
        Fsr.Data = MultiplyDouble(EO | EI | ROUND_TO_NEAREST,
                                  &DoubleOperand1,
                                  &DoubleOperand2,
                                  &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_OVERFLOW) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_MAXIMUM_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_MAXIMUM_VALUE_LOW;
    DoubleOperand2.HighPart = SIGN | DOUBLE_MAXIMUM_VALUE_HIGH;
    Count = 0;
    try {
        Fsr.Data = MultiplyDouble(EO | EI | ROUND_TO_NEAREST,
                                  &DoubleOperand1,
                                  &DoubleOperand2,
                                  &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_OVERFLOW) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    //
    // End of test 17.
    //

    printf("succeeded\n");
    return;

    //
    // Test 17 failed.
    //

TestFailed:
    printf(" subtest %d failed, fsr = %lx, result = %lx, %lx\n",
           Subtest,
           Fsr.Data,
           DoubleResult.LowPart,
           DoubleResult.HighPart);

    return;
}

VOID
Test18 (
    VOID
    )

{

    ULONG Count;
    ULARGE_INTEGER DoubleOperand1;
    ULARGE_INTEGER DoubleOperand2;
    ULARGE_INTEGER DoubleResult;
    FLOATING_STATUS Fsr;
    ULONG Subtest;

    //
    // Test 18 - Divide double test.
    //

    Subtest = 0;
    printf("    Test 18 - divide double ...");
    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x0;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x0;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x0;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x0;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x0;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x0;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x0;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x0;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x0;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != DOUBLE_INFINITY_VALUE_HIGH)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x3ff00000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x0;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != (SZ | XZ | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != DOUBLE_INFINITY_VALUE_HIGH)) {
        goto TestFailed;
    }


    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x0;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != MINUS_DOUBLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x3ff00000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x0;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != (SZ | XZ | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != MINUS_DOUBLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Count = 0;
    try {
        Fsr.Data = DivideDouble(EV | ROUND_TO_NEAREST,
                                &DoubleOperand1,
                                &DoubleOperand2,
                                &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x0;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x0;
    Count = 0;
    try {
        Fsr.Data = DivideDouble(EV | ROUND_TO_NEAREST,
                                &DoubleOperand1,
                                &DoubleOperand2,
                                &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x3ff00000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x0;
    Count = 0;
    try {
        Fsr.Data = DivideDouble(EZ | ROUND_TO_NEAREST,
                                &DoubleOperand1,
                                &DoubleOperand2,
                                &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_DIVIDE_BY_ZERO) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand2.HighPart = DOUBLE_QUIET_NAN;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != (ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x3ff00000;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != (ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x3ff00000;
    DoubleOperand2.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand2.HighPart = DOUBLE_QUIET_NAN;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != (ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_SIGNAL_NAN;
    DoubleOperand2.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand2.HighPart = DOUBLE_SIGNAL_NAN;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_SIGNAL_NAN;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x3ff00000;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x3ff00000;
    DoubleOperand2.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand2.HighPart = DOUBLE_SIGNAL_NAN;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand2.HighPart = DOUBLE_QUIET_NAN;
    try {
        Fsr.Data = DivideDouble(EV | ROUND_TO_NEAREST,
                                &DoubleOperand1,
                                &DoubleOperand2,
                                &DoubleResult);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        goto TestFailed;
    }

    if ((Fsr.Data != (EV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand2.HighPart = DOUBLE_SIGNAL_NAN;
    Count = 0;
    try {
        Fsr.Data = DivideDouble(EV | ROUND_TO_NEAREST,
                                &DoubleOperand1,
                                &DoubleOperand2,
                                &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_SIGNAL_NAN;
    DoubleOperand2.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand2.HighPart = DOUBLE_QUIET_NAN;
    Count = 0;
    try {
        Fsr.Data = DivideDouble(EV | ROUND_TO_NEAREST,
                                &DoubleOperand1,
                                &DoubleOperand2,
                                &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count != 1) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x0)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x80000;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != (SIGN | 0x0))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != (SIGN | 0x0))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x80000;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x0)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x3ff00000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != (SIGN | 0x3ff00000))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x80000;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != (SIGN | 0x3ff00000))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x80000;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x3ff00000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x3ff00000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x7fe00000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x40000000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != (SO | SI | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != DOUBLE_INFINITY_VALUE_HIGH)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0xffffffff;
    DoubleOperand1.HighPart = 0x3fffffff;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = DivideDouble(ROUND_TO_NEAREST,
                            &DoubleOperand1,
                            &DoubleOperand2,
                            &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0xffffffff) ||
        (DoubleResult.HighPart != 0x7fefffff)) {
        goto TestFailed;
    }

    //
    // End of test 18.
    //

    printf("succeeded\n");
    return;

    //
    // Test 18 failed.
    //

TestFailed:
    printf(" subtest %d failed, fsr = %lx, result = %lx, %lx\n",
           Subtest,
           Fsr.Data,
           DoubleResult.LowPart,
           DoubleResult.HighPart);

    return;
}

VOID
Test19 (
    VOID
    )

{

    ULONG Count;
    ULARGE_INTEGER DoubleOperand1;
    ULARGE_INTEGER DoubleOperand2;
    FLOATING_STATUS Fsr;
    ULONG Subtest;

    //
    // Test 19 - Compare double test.
    //

    Subtest = 0;
    printf("    Test 19 - compare double ...");

// ****** //

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x0;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = CompareLtDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x0;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = CompareLtDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x0;
    Fsr.Data = CompareLtDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x0;
    Fsr.Data = CompareLtDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

// ****** //

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = CompareEqDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = CompareEqDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = CompareEqDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x80000;
    Fsr.Data = CompareEqDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = CompareLeDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = CompareLeDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = CompareLeDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = CompareLeDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    Fsr.Data = CompareLeDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x80000;
    Fsr.Data = CompareLeDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = CompareLtDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = CompareLtDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = CompareLtDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = CompareLtDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x1000;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = CompareLtDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x81000;
    Fsr.Data = CompareLtDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x40000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = CompareLtDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = CompareLtDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x1000;
    DoubleOperand1.HighPart = SIGN | 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x80000;
    Fsr.Data = CompareLtDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x81000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x80000;
    Fsr.Data = CompareLtDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = SIGN | 0x40000;
    Fsr.Data = CompareLtDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = SIGN | 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = CompareLtDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = CompareFDouble(ROUND_TO_NEAREST,
                              &DoubleOperand1,
                              &DoubleOperand2);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = CompareUnDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = CompareUnDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = CompareEqDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = CompareEqDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = CompareUeqDouble(ROUND_TO_NEAREST,
                                &DoubleOperand1,
                                &DoubleOperand2);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = CompareUeqDouble(ROUND_TO_NEAREST,
                                &DoubleOperand1,
                                &DoubleOperand2);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand2.HighPart = DOUBLE_QUIET_NAN;
    Fsr.Data = CompareOltDouble(ROUND_TO_NEAREST,
                                &DoubleOperand1,
                                &DoubleOperand2);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = CompareOltDouble(ROUND_TO_NEAREST,
                                &DoubleOperand1,
                                &DoubleOperand2);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x80000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = CompareUltDouble(ROUND_TO_NEAREST,
                                &DoubleOperand1,
                                &DoubleOperand2);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = CompareUltDouble(ROUND_TO_NEAREST,
                                &DoubleOperand1,
                                &DoubleOperand2);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x81000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = CompareOleDouble(ROUND_TO_NEAREST,
                                &DoubleOperand1,
                                &DoubleOperand2);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = CompareOleDouble(ROUND_TO_NEAREST,
                                &DoubleOperand1,
                                &DoubleOperand2);

    if (Fsr.Data != (CC | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x81000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = CompareSfDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand2.HighPart = DOUBLE_QUIET_NAN;
    Fsr.Data = CompareSfDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x81000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = CompareNgleDouble(ROUND_TO_NEAREST,
                                 &DoubleOperand1,
                                 &DoubleOperand2);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = CompareNgleDouble(ROUND_TO_NEAREST,
                                 &DoubleOperand1,
                                 &DoubleOperand2);

    if (Fsr.Data != (CC | SV | XV | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x81000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = CompareSeqDouble(ROUND_TO_NEAREST,
                                &DoubleOperand1,
                                &DoubleOperand2);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = CompareSeqDouble(ROUND_TO_NEAREST,
                                &DoubleOperand1,
                                &DoubleOperand2);

    if (Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x81000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = CompareNglDouble(ROUND_TO_NEAREST,
                                &DoubleOperand1,
                                &DoubleOperand2);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = CompareNglDouble(ROUND_TO_NEAREST,
                                &DoubleOperand1,
                                &DoubleOperand2);

    if (Fsr.Data != (CC | SV | XV | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x81000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = CompareLtDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = CompareLtDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x81000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = CompareNgeDouble(ROUND_TO_NEAREST,
                                &DoubleOperand1,
                                &DoubleOperand2);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = CompareNgeDouble(ROUND_TO_NEAREST,
                                &DoubleOperand1,
                                &DoubleOperand2);

    if (Fsr.Data != (CC | SV | XV | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x81000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = CompareLeDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = CompareLeDouble(ROUND_TO_NEAREST,
                               &DoubleOperand1,
                               &DoubleOperand2);

    if (Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = 0x0;
    DoubleOperand1.HighPart = 0x81000;
    DoubleOperand2.LowPart = 0x0;
    DoubleOperand2.HighPart = 0x80000;
    Fsr.Data = CompareNgtDouble(ROUND_TO_NEAREST,
                                &DoubleOperand1,
                                &DoubleOperand2);

    if (Fsr.Data != ROUND_TO_NEAREST) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = CompareNgtDouble(ROUND_TO_NEAREST,
                                &DoubleOperand1,
                                &DoubleOperand2);

    if (Fsr.Data != (CC | SV | XV | ROUND_TO_NEAREST)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand1.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    DoubleOperand2.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand2.HighPart = DOUBLE_QUIET_NAN;
    Count = 0;
    try {
        Fsr.Data = CompareSfDouble(EV | ROUND_TO_NEAREST,
                                   &DoubleOperand1,
                                   &DoubleOperand2);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Count = 0;
    try {
        Fsr.Data = CompareNgleDouble(EV | ROUND_TO_NEAREST,
                                     &DoubleOperand1,
                                     &DoubleOperand2);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Count = 0;
    try {
        Fsr.Data = CompareSeqDouble(EV | ROUND_TO_NEAREST,
                                    &DoubleOperand1,
                                    &DoubleOperand2);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Count = 0;
    try {
        Fsr.Data = CompareNglDouble(EV | ROUND_TO_NEAREST,
                                    &DoubleOperand1,
                                    &DoubleOperand2);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Count = 0;
    try {
        Fsr.Data = CompareLtDouble(EV | ROUND_TO_NEAREST,
                                   &DoubleOperand1,
                                   &DoubleOperand2);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Count = 0;
    try {
        Fsr.Data = CompareNgeDouble(EV | ROUND_TO_NEAREST,
                                    &DoubleOperand1,
                                    &DoubleOperand2);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Count = 0;
    try {
        Fsr.Data = CompareLeDouble(EV | ROUND_TO_NEAREST,
                                   &DoubleOperand1,
                                   &DoubleOperand2);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_QUIET_NAN;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Count = 0;
    try {
        Fsr.Data = CompareNgtDouble(EV | ROUND_TO_NEAREST,
                                    &DoubleOperand1,
                                    &DoubleOperand2);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand1.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand1.HighPart = DOUBLE_SIGNAL_NAN;
    DoubleOperand2.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand2.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Count = 0;
    try {
        Fsr.Data = CompareEqDouble(EV | ROUND_TO_NEAREST,
                                   &DoubleOperand1,
                                   &DoubleOperand2);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    //
    // End of test 19.
    //

    printf("succeeded\n");
    return;

    //
    // Test 19 failed.
    //

TestFailed:
    printf(" subtest %d failed, fsr = %lx\n", Subtest, Fsr.Data);
    return;
}
