/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    flpt2.c

Abstract:

    This module implements user mode IEEE floating point tests.

Author:

    David N. Cutler (davec) 1-Jul-1991

Environment:

    User mode only.

Revision History:

--*/

#include "flpt.h"

VOID
Test20 (
    VOID
    )

{

    ULONG Count;
    FLOATING_STATUS Fsr;
    ULARGE_INTEGER DoubleOperand;
    ULARGE_INTEGER DoubleResult;
    ULONG Subtest;

    //
    // Test 20 - Absolute, move, and negate double test.
    //

    Subtest = 0;
    printf("    Test 20 - absolute, move, and negate double ...");
    Subtest += 1;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = 0x80000;
    Fsr.Data = AbsoluteDouble(ROUND_TO_NEAREST,
                              &DoubleOperand,
                              &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x80000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = SIGN | 0x80000;
    Fsr.Data = AbsoluteDouble(ROUND_TO_NEAREST,
                              &DoubleOperand,
                              &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x80000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand.HighPart = DOUBLE_QUIET_NAN;
    Fsr.Data = AbsoluteDouble(ROUND_TO_NEAREST,
                              &DoubleOperand,
                              &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand.HighPart = DOUBLE_SIGNAL_NAN;
    Fsr.Data = AbsoluteDouble(ROUND_TO_NEAREST,
                              &DoubleOperand,
                              &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    DoubleOperand.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand.HighPart = DOUBLE_SIGNAL_NAN;
    try {
        Fsr.Data = AbsoluteDouble(EV | ROUND_TO_NEAREST,
                                  &DoubleOperand,
                                  &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = 0x80000;
    Fsr.Data = MoveDouble(ROUND_TO_NEAREST,
                          &DoubleOperand,
                          &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x80000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = SIGN | 0x80000;
    Fsr.Data = MoveDouble(ROUND_TO_NEAREST,
                          &DoubleOperand,
                          &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != (SIGN | 0x80000))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand.HighPart = DOUBLE_QUIET_NAN;
    Fsr.Data = MoveDouble(ROUND_TO_NEAREST,
                          &DoubleOperand,
                          &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand.HighPart = DOUBLE_SIGNAL_NAN;
    Fsr.Data = MoveDouble(ROUND_TO_NEAREST,
                          &DoubleOperand,
                          &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_SIGNAL_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand.HighPart = DOUBLE_SIGNAL_NAN;
    Fsr.Data = MoveDouble(EV | ROUND_TO_NEAREST,
                          &DoubleOperand,
                          &DoubleResult);

    if ((Fsr.Data != (EV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_SIGNAL_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = 0x80000;
    Fsr.Data = NegateDouble(ROUND_TO_NEAREST,
                            &DoubleOperand,
                            &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != (SIGN | 0x80000))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = SIGN | 0x80000;
    Fsr.Data = NegateDouble(ROUND_TO_NEAREST,
                            &DoubleOperand,
                            &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x80000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand.HighPart = DOUBLE_QUIET_NAN;
    Fsr.Data = NegateDouble(ROUND_TO_NEAREST,
                            &DoubleOperand,
                            &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand.HighPart = DOUBLE_SIGNAL_NAN;
    Fsr.Data = NegateDouble(ROUND_TO_NEAREST,
                            &DoubleOperand,
                            &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    DoubleOperand.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand.HighPart = DOUBLE_SIGNAL_NAN;
    try {
        Fsr.Data = NegateDouble(EV | ROUND_TO_NEAREST,
                                &DoubleOperand,
                                &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    //
    // End of test 20.
    //

    printf("succeeded\n");
    return;

    //
    // Test 20 failed.
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
Test21 (
    VOID
    )

{

    ULONG Count;
    FLOATING_STATUS Fsr;
    ULARGE_INTEGER DoubleOperand;
    ULONG SingleResult;
    ULONG Subtest;

    //
    // Test 21 - Convert to single.
    //

    Subtest = 0;
    printf("    Test 21 - convert to single ...");
    Subtest += 1;
    DoubleOperand.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand.HighPart = DOUBLE_QUIET_NAN;
    Fsr.Data = ConvertToSingleFromDouble(ROUND_TO_NEAREST,
                                         &DoubleOperand,
                                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand.HighPart = DOUBLE_SIGNAL_NAN;
    Fsr.Data = ConvertToSingleFromDouble(ROUND_TO_NEAREST,
                                         &DoubleOperand,
                                         &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = ConvertToSingleFromDouble(ROUND_TO_NEAREST,
                                         &DoubleOperand,
                                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    Fsr.Data = ConvertToSingleFromDouble(ROUND_TO_NEAREST,
                                         &DoubleOperand,
                                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != MINUS_SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    DoubleOperand.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand.HighPart = DOUBLE_SIGNAL_NAN;
    try {
        Fsr.Data = ConvertToSingleFromDouble(EV | ROUND_TO_NEAREST,
                                             &DoubleOperand,
                                             &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart =
        ((DOUBLE_EXPONENT_BIAS - SINGLE_EXPONENT_BIAS) << (52 - 32)) | 0xfffff;
    Fsr.Data = ConvertToSingleFromDouble(ROUND_TO_ZERO,
                                         &DoubleOperand,
                                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_ZERO) ||
        (SingleResult != 0x7ffffc)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = SIGN |
        ((DOUBLE_EXPONENT_BIAS - SINGLE_EXPONENT_BIAS) << (52 - 32)) | 0xfffff;
    Fsr.Data = ConvertToSingleFromDouble(ROUND_TO_ZERO,
                                         &DoubleOperand,
                                         &SingleResult);

    if ((Fsr.Data != ROUND_TO_ZERO) ||
        (SingleResult != (SIGN | 0x7ffffc))) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0xf0000000;
    DoubleOperand.HighPart =
        ((DOUBLE_EXPONENT_BIAS - SINGLE_EXPONENT_BIAS) << (52 - 32)) | 0xfffff;
    Fsr.Data = ConvertToSingleFromDouble(ROUND_TO_ZERO,
                                         &DoubleOperand,
                                         &SingleResult);

    if ((Fsr.Data != (SI | SU | ROUND_TO_ZERO)) ||
        (SingleResult != 0x7fffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0xf0000000;
    DoubleOperand.HighPart = SIGN |
        ((DOUBLE_EXPONENT_BIAS - SINGLE_EXPONENT_BIAS) << (52 - 32)) | 0xfffff;
    Fsr.Data = ConvertToSingleFromDouble(ROUND_TO_ZERO,
                                         &DoubleOperand,
                                         &SingleResult);

    if ((Fsr.Data != (SI | SU | ROUND_TO_ZERO)) ||
        (SingleResult != (SIGN | 0x7fffff))) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    DoubleOperand.LowPart = 0xf0000000;
    DoubleOperand.HighPart = SIGN |
        ((DOUBLE_EXPONENT_BIAS - SINGLE_EXPONENT_BIAS) << (52 - 32)) | 0xfffff;
    try {
        Fsr.Data = ConvertToSingleFromDouble(EU | ROUND_TO_ZERO,
                                             &DoubleOperand,
                                             &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_UNDERFLOW) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    DoubleOperand.LowPart = 0xf0000000;
    DoubleOperand.HighPart = SIGN |
        ((DOUBLE_EXPONENT_BIAS - SINGLE_EXPONENT_BIAS) << (52 - 32)) | 0xfffff;
    try {
        Fsr.Data = ConvertToSingleFromDouble(EI | ROUND_TO_ZERO,
                                             &DoubleOperand,
                                             &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = 0xfffff;
    Fsr.Data = ConvertToSingleFromDouble(ROUND_TO_ZERO,
                                         &DoubleOperand,
                                         &SingleResult);

    if ((Fsr.Data != (SI | SU | ROUND_TO_ZERO)) ||
        (SingleResult != 0x0)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = SIGN | 0xfffff;
    Fsr.Data = ConvertToSingleFromDouble(ROUND_TO_ZERO,
                                         &DoubleOperand,
                                         &SingleResult);

    if ((Fsr.Data != (SI | SU | ROUND_TO_ZERO)) ||
        (SingleResult != (SIGN | 0x0))) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = 0xfffff;
    try {
        Fsr.Data = ConvertToSingleFromDouble(EU | ROUND_TO_ZERO,
                                             &DoubleOperand,
                                             &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_UNDERFLOW) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = 0xfffff;
    try {
        Fsr.Data = ConvertToSingleFromDouble(EI | ROUND_TO_ZERO,
                                             &DoubleOperand,
                                             &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = ((DOUBLE_EXPONENT_BIAS +
                            SINGLE_EXPONENT_BIAS + 1) << (52 - 32)) | 0xfffff;
    Fsr.Data = ConvertToSingleFromDouble(ROUND_TO_ZERO,
                                         &DoubleOperand,
                                         &SingleResult);

    if ((Fsr.Data != (SI | SO | XI | XO | ROUND_TO_ZERO)) ||
        (SingleResult != SINGLE_MAXIMUM_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = SIGN | ((DOUBLE_EXPONENT_BIAS +
                            SINGLE_EXPONENT_BIAS + 1) << (52 - 32)) | 0xfffff;
    Fsr.Data = ConvertToSingleFromDouble(ROUND_TO_ZERO,
                                         &DoubleOperand,
                                         &SingleResult);

    if ((Fsr.Data != (SI | SO | XI | XO | ROUND_TO_ZERO)) ||
        (SingleResult != (SIGN | SINGLE_MAXIMUM_VALUE))) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = ((DOUBLE_EXPONENT_BIAS +
                            SINGLE_EXPONENT_BIAS + 1) << (52 - 32)) | 0xfffff;
    try {
        Fsr.Data = ConvertToSingleFromDouble(EO | ROUND_TO_ZERO,
                                             &DoubleOperand,
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
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = ((DOUBLE_EXPONENT_BIAS +
                            SINGLE_EXPONENT_BIAS + 1) << (52 - 32)) | 0xfffff;
    try {
        Fsr.Data = ConvertToSingleFromDouble(EI | ROUND_TO_ZERO,
                                             &DoubleOperand,
                                             &SingleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    //
    // End of test 21.
    //

    printf("succeeded\n");
    return;

    //
    // Test 21 failed.
    //

TestFailed:
    printf(" subtest %d failed, fsr = %lx, result = %lx\n",
           Subtest,
           Fsr.Data,
           SingleResult);

    return;
}

VOID
Test22 (
    VOID
    )

{

    ULONG Count;
    FLOATING_STATUS Fsr;
    ULARGE_INTEGER DoubleResult;
    ULONG SingleOperand;
    ULONG Subtest;

    //
    // Test 22 - Convert to double.
    //

    Subtest = 0;
    printf("    Test 22 - convert to double ...");
    Subtest += 1;
    SingleOperand = SINGLE_QUIET_NAN;
    Fsr.Data = ConvertToDoubleFromSingle(ROUND_TO_NEAREST,
                                         SingleOperand,
                                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    SingleOperand = SINGLE_SIGNAL_NAN;
    Fsr.Data = ConvertToDoubleFromSingle(ROUND_TO_NEAREST,
                                         SingleOperand,
                                         &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    SingleOperand = SINGLE_INFINITY_VALUE;
    Fsr.Data = ConvertToDoubleFromSingle(ROUND_TO_NEAREST,
                                         SingleOperand,
                                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != DOUBLE_INFINITY_VALUE_HIGH)) {
        goto TestFailed;
    }

    Subtest += 1;
    SingleOperand = MINUS_SINGLE_INFINITY_VALUE;
    Fsr.Data = ConvertToDoubleFromSingle(ROUND_TO_NEAREST,
                                         SingleOperand,
                                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != MINUS_DOUBLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    SingleOperand = SINGLE_SIGNAL_NAN;
    try {
        Fsr.Data = ConvertToDoubleFromSingle(EV | ROUND_TO_NEAREST,
                                             SingleOperand,
                                             &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    SingleOperand = 0x400000;
    Fsr.Data = ConvertToDoubleFromSingle(ROUND_TO_ZERO,
                                         SingleOperand,
                                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_ZERO) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x38000000)) {
        goto TestFailed;
    }

    Subtest += 1;
    SingleOperand = SIGN | 0x400000;
    Fsr.Data = ConvertToDoubleFromSingle(ROUND_TO_ZERO,
                                         SingleOperand,
                                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_ZERO) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != (SIGN | 0x38000000))) {
        goto TestFailed;
    }

    Subtest += 1;
    SingleOperand = 0x440000;
    Fsr.Data = ConvertToDoubleFromSingle(ROUND_TO_ZERO,
                                         SingleOperand,
                                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_ZERO) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x38010000)) {
        goto TestFailed;
    }

    Subtest += 1;
    SingleOperand = SIGN | 0x440000;
    Fsr.Data = ConvertToDoubleFromSingle(ROUND_TO_ZERO,
                                         SingleOperand,
                                         &DoubleResult);

    if ((Fsr.Data != ROUND_TO_ZERO) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != (SIGN | 0x38010000))) {
        goto TestFailed;
    }

    //
    // End of test 22.
    //

    printf("succeeded\n");
    return;

    //
    // Test 22 failed.
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
Test23 (
    VOID
    )

{

    ULONG Count;
    FLOATING_STATUS Fsr;
    ULONG LongwordResult;
    ULONG SingleOperand;
    ULONG Subtest;

    //
    // Test 23 - Convert to longword from single.
    //

    Subtest = 0;
    printf("    Test 23 - convert to longword from single ...");
    Subtest += 1;
    SingleOperand = SINGLE_QUIET_NAN;
    Fsr.Data = ConvertToLongwordFromSingle(ROUND_TO_NEAREST,
                                           SingleOperand,
                                           &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != INTEGER_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    SingleOperand = SINGLE_SIGNAL_NAN;
    Fsr.Data = ConvertToLongwordFromSingle(ROUND_TO_NEAREST,
                                           SingleOperand,
                                           &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != INTEGER_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    SingleOperand = SINGLE_INFINITY_VALUE;
    Fsr.Data = ConvertToLongwordFromSingle(ROUND_TO_NEAREST,
                                           SingleOperand,
                                           &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != 0x7fffffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    SingleOperand = MINUS_SINGLE_INFINITY_VALUE;
    Fsr.Data = ConvertToLongwordFromSingle(ROUND_TO_NEAREST,
                                           SingleOperand,
                                           &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != 0x80000000)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    SingleOperand = SINGLE_QUIET_NAN;
    try {
        Fsr.Data = ConvertToLongwordFromSingle(EV | ROUND_TO_NEAREST,
                                               SingleOperand,
                                               &LongwordResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    SingleOperand = SINGLE_SIGNAL_NAN;
    try {
        Fsr.Data = ConvertToLongwordFromSingle(EV | ROUND_TO_NEAREST,
                                               SingleOperand,
                                               &LongwordResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    SingleOperand = SINGLE_INFINITY_VALUE;
    try {
        Fsr.Data = ConvertToLongwordFromSingle(EV | ROUND_TO_NEAREST,
                                               SingleOperand,
                                               &LongwordResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    SingleOperand = MINUS_SINGLE_INFINITY_VALUE;
    try {
        Fsr.Data = ConvertToLongwordFromSingle(EV | ROUND_TO_NEAREST,
                                               SingleOperand,
                                               &LongwordResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    SingleOperand = 0x400000;
    Fsr.Data = ConvertToLongwordFromSingle(ROUND_TO_NEAREST,
                                           SingleOperand,
                                           &LongwordResult);

    if ((Fsr.Data != (SI | ROUND_TO_NEAREST)) ||
        (LongwordResult != 0x0)) {
        goto TestFailed;
    }

    Subtest += 1;
    SingleOperand = 0x1;
    Fsr.Data = ConvertToLongwordFromSingle(ROUND_TO_NEAREST,
                                           SingleOperand,
                                           &LongwordResult);

    if ((Fsr.Data != (SI | ROUND_TO_NEAREST)) ||
        (LongwordResult != 0x0)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    SingleOperand = 0x400000;
    try {
        Fsr.Data = ConvertToLongwordFromSingle(EI | ROUND_TO_NEAREST,
                                               SingleOperand,
                                               &LongwordResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    SingleOperand = ((SINGLE_EXPONENT_BIAS + 32) << 23);
    Fsr.Data = ConvertToLongwordFromSingle(ROUND_TO_NEAREST,
                                           SingleOperand,
                                           &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != INTEGER_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    SingleOperand = ((SINGLE_EXPONENT_BIAS + 31) << 23);
    Fsr.Data = ConvertToLongwordFromSingle(ROUND_TO_NEAREST,
                                           SingleOperand,
                                           &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != INTEGER_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    SingleOperand = SIGN | ((SINGLE_EXPONENT_BIAS + 31) << 23);
    Fsr.Data = ConvertToLongwordFromSingle(ROUND_TO_NEAREST,
                                           SingleOperand,
                                           &LongwordResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (LongwordResult != 0x80000000)) {
        goto TestFailed;
    }

    Subtest += 1;
    SingleOperand = SIGN | ((SINGLE_EXPONENT_BIAS + 31) << 23) | 0x1;
    Fsr.Data = ConvertToLongwordFromSingle(ROUND_TO_NEAREST,
                                           SingleOperand,
                                           &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != INTEGER_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    SingleOperand = ((SINGLE_EXPONENT_BIAS + 31) << 23);
    try {
        Fsr.Data = ConvertToLongwordFromSingle(EV | ROUND_TO_NEAREST,
                                               SingleOperand,
                                               &LongwordResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    SingleOperand = SINGLE_QUIET_NAN;
    Fsr.Data = RoundToLongwordFromSingle(ROUND_TO_NEAREST,
                                         SingleOperand,
                                         &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != INTEGER_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    SingleOperand = SINGLE_SIGNAL_NAN;
    Fsr.Data = RoundToLongwordFromSingle(ROUND_TO_NEAREST,
                                         SingleOperand,
                                         &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != INTEGER_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    SingleOperand = SINGLE_INFINITY_VALUE;
    Fsr.Data = RoundToLongwordFromSingle(ROUND_TO_NEAREST,
                                         SingleOperand,
                                         &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != 0x7fffffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    SingleOperand = MINUS_SINGLE_INFINITY_VALUE;
    Fsr.Data = RoundToLongwordFromSingle(ROUND_TO_NEAREST,
                                         SingleOperand,
                                         &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != 0x80000000)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    SingleOperand = SINGLE_QUIET_NAN;
    try {
        Fsr.Data = RoundToLongwordFromSingle(EV | ROUND_TO_NEAREST,
                                             SingleOperand,
                                             &LongwordResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    SingleOperand = SINGLE_SIGNAL_NAN;
    try {
        Fsr.Data = RoundToLongwordFromSingle(EV | ROUND_TO_NEAREST,
                                             SingleOperand,
                                             &LongwordResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    SingleOperand = SINGLE_INFINITY_VALUE;
    try {
        Fsr.Data = RoundToLongwordFromSingle(EV | ROUND_TO_NEAREST,
                                             SingleOperand,
                                             &LongwordResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    SingleOperand = MINUS_SINGLE_INFINITY_VALUE;
    try {
        Fsr.Data = RoundToLongwordFromSingle(EV | ROUND_TO_NEAREST,
                                             SingleOperand,
                                             &LongwordResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    SingleOperand = 0x400000;
    Fsr.Data = RoundToLongwordFromSingle(ROUND_TO_NEAREST,
                                         SingleOperand,
                                         &LongwordResult);

    if ((Fsr.Data != (SI | ROUND_TO_NEAREST)) ||
        (LongwordResult != 0x0)) {
        goto TestFailed;
    }

    Subtest += 1;
    SingleOperand = 0x1;
    Fsr.Data = RoundToLongwordFromSingle(ROUND_TO_NEAREST,
                                         SingleOperand,
                                         &LongwordResult);

    if ((Fsr.Data != (SI | ROUND_TO_NEAREST)) ||
        (LongwordResult != 0x0)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    SingleOperand = 0x400000;
    try {
        Fsr.Data = RoundToLongwordFromSingle(EI | ROUND_TO_NEAREST,
                                             SingleOperand,
                                             &LongwordResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    SingleOperand = ((SINGLE_EXPONENT_BIAS + 32) << 23);
    Fsr.Data = RoundToLongwordFromSingle(ROUND_TO_NEAREST,
                                         SingleOperand,
                                         &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != INTEGER_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    SingleOperand = ((SINGLE_EXPONENT_BIAS + 31) << 23);
    Fsr.Data = RoundToLongwordFromSingle(ROUND_TO_NEAREST,
                                         SingleOperand,
                                         &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != INTEGER_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    SingleOperand = SIGN | ((SINGLE_EXPONENT_BIAS + 31) << 23);
    Fsr.Data = RoundToLongwordFromSingle(ROUND_TO_NEAREST,
                                         SingleOperand,
                                         &LongwordResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (LongwordResult != 0x80000000)) {
        goto TestFailed;
    }

    Subtest += 1;
    SingleOperand = SIGN | ((SINGLE_EXPONENT_BIAS + 31) << 23) | 0x1;
    Fsr.Data = RoundToLongwordFromSingle(ROUND_TO_NEAREST,
                                         SingleOperand,
                                         &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != INTEGER_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    SingleOperand = ((SINGLE_EXPONENT_BIAS + 31) << 23);
    try {
        Fsr.Data = RoundToLongwordFromSingle(EV | ROUND_TO_NEAREST,
                                             SingleOperand,
                                             &LongwordResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    //
    // End of test 23.
    //

    printf("succeeded\n");
    return;

    //
    // Test 23 failed.
    //

TestFailed:
    printf(" subtest %d failed, fsr = %lx, result = %lx\n",
           Subtest,
           Fsr.Data,
           LongwordResult);

    return;
}

VOID
Test24 (
    VOID
    )

{

    ULONG Count;
    FLOATING_STATUS Fsr;
    ULARGE_INTEGER DoubleOperand;
    ULONG LongwordResult;
    ULONG Subtest;

    //
    // Test 24 - Convert to longword from double.
    //

    Subtest = 0;
    printf("    Test 24 - convert to longword from double ...");
    Subtest += 1;
    DoubleOperand.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand.HighPart = DOUBLE_QUIET_NAN;
    Fsr.Data = ConvertToLongwordFromDouble(ROUND_TO_NEAREST,
                                           &DoubleOperand,
                                           &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != INTEGER_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand.HighPart = DOUBLE_SIGNAL_NAN;
    Fsr.Data = ConvertToLongwordFromDouble(ROUND_TO_NEAREST,
                                           &DoubleOperand,
                                           &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != INTEGER_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = ConvertToLongwordFromDouble(ROUND_TO_NEAREST,
                                           &DoubleOperand,
                                           &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != 0x7fffffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    Fsr.Data = ConvertToLongwordFromDouble(ROUND_TO_NEAREST,
                                           &DoubleOperand,
                                           &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != 0x80000000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand.HighPart = DOUBLE_QUIET_NAN;
    Fsr.Data = RoundToLongwordFromDouble(ROUND_TO_NEAREST,
                                         &DoubleOperand,
                                         &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != INTEGER_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand.HighPart = DOUBLE_SIGNAL_NAN;
    Fsr.Data = RoundToLongwordFromDouble(ROUND_TO_NEAREST,
                                         &DoubleOperand,
                                         &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != INTEGER_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = RoundToLongwordFromDouble(ROUND_TO_NEAREST,
                                         &DoubleOperand,
                                         &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != 0x7fffffff)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    Fsr.Data = RoundToLongwordFromDouble(ROUND_TO_NEAREST,
                                         &DoubleOperand,
                                         &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != 0x80000000)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    DoubleOperand.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand.HighPart = DOUBLE_QUIET_NAN;
    try {
        Fsr.Data = ConvertToLongwordFromDouble(EV | ROUND_TO_ZERO,
                                               &DoubleOperand,
                                               &LongwordResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    DoubleOperand.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand.HighPart = DOUBLE_SIGNAL_NAN;
    try {
        Fsr.Data = ConvertToLongwordFromDouble(EV | ROUND_TO_ZERO,
                                               &DoubleOperand,
                                               &LongwordResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    DoubleOperand.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    try {
        Fsr.Data = ConvertToLongwordFromDouble(EV | ROUND_TO_ZERO,
                                               &DoubleOperand,
                                               &LongwordResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    DoubleOperand.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    try {
        Fsr.Data = ConvertToLongwordFromDouble(EV | ROUND_TO_ZERO,
                                               &DoubleOperand,
                                               &LongwordResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    DoubleOperand.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand.HighPart = DOUBLE_QUIET_NAN;
    try {
        Fsr.Data = TruncateToLongwordFromDouble(EV | ROUND_TO_ZERO,
                                                &DoubleOperand,
                                                &LongwordResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    DoubleOperand.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand.HighPart = DOUBLE_SIGNAL_NAN;
    try {
        Fsr.Data = TruncateToLongwordFromDouble(EV | ROUND_TO_ZERO,
                                                &DoubleOperand,
                                                &LongwordResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    DoubleOperand.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    try {
        Fsr.Data = TruncateToLongwordFromDouble(EV | ROUND_TO_ZERO,
                                                &DoubleOperand,
                                                &LongwordResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    DoubleOperand.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    try {
        Fsr.Data = TruncateToLongwordFromDouble(EV | ROUND_TO_ZERO,
                                                &DoubleOperand,
                                                &LongwordResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = 0x80000;
    Fsr.Data = ConvertToLongwordFromDouble(ROUND_TO_NEAREST,
                                           &DoubleOperand,
                                           &LongwordResult);

    if ((Fsr.Data != (SI | ROUND_TO_NEAREST)) ||
        (LongwordResult != 0x0)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0x1;
    DoubleOperand.HighPart = 0x0;
    Fsr.Data = ConvertToLongwordFromDouble(ROUND_TO_NEAREST,
                                           &DoubleOperand,
                                           &LongwordResult);

    if ((Fsr.Data != (SI | ROUND_TO_NEAREST)) ||
        (LongwordResult != 0x0)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = 0x80000;
    Fsr.Data = RoundToLongwordFromDouble(ROUND_TO_NEAREST,
                                         &DoubleOperand,
                                         &LongwordResult);

    if ((Fsr.Data != (SI | ROUND_TO_NEAREST)) ||
        (LongwordResult != 0x0)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0x1;
    DoubleOperand.HighPart = 0x0;
    Fsr.Data = RoundToLongwordFromDouble(ROUND_TO_NEAREST,
                                         &DoubleOperand,
                                         &LongwordResult);

    if ((Fsr.Data != (SI | ROUND_TO_NEAREST)) ||
        (LongwordResult != 0x0)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = 0x80000;
    try {
        Fsr.Data = ConvertToLongwordFromDouble(EI | ROUND_TO_ZERO,
                                               &DoubleOperand,
                                               &LongwordResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = 0x80000;
    try {
        Fsr.Data = TruncateToLongwordFromDouble(EI | ROUND_TO_ZERO,
                                                &DoubleOperand,
                                                &LongwordResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INEXACT_RESULT) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = ((DOUBLE_EXPONENT_BIAS + 32) << 20);
    Fsr.Data = ConvertToLongwordFromDouble(ROUND_TO_NEAREST,
                                           &DoubleOperand,
                                           &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != INTEGER_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = ((DOUBLE_EXPONENT_BIAS + 31) << 20);
    Fsr.Data = ConvertToLongwordFromDouble(ROUND_TO_NEAREST,
                                           &DoubleOperand,
                                           &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != INTEGER_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = SIGN | ((DOUBLE_EXPONENT_BIAS + 31) << 20);
    Fsr.Data = ConvertToLongwordFromDouble(ROUND_TO_NEAREST,
                                           &DoubleOperand,
                                           &LongwordResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (LongwordResult != 0x80000000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = SIGN | ((DOUBLE_EXPONENT_BIAS + 31) << 20) | 0x1;
    Fsr.Data = ConvertToLongwordFromDouble(ROUND_TO_NEAREST,
                                           &DoubleOperand,
                                           &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != INTEGER_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = ((DOUBLE_EXPONENT_BIAS + 32) << 20);
    Fsr.Data = RoundToLongwordFromDouble(ROUND_TO_NEAREST,
                                         &DoubleOperand,
                                         &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != INTEGER_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = ((DOUBLE_EXPONENT_BIAS + 31) << 20);
    Fsr.Data = RoundToLongwordFromDouble(ROUND_TO_NEAREST,
                                         &DoubleOperand,
                                         &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != INTEGER_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = SIGN | ((DOUBLE_EXPONENT_BIAS + 31) << 20);
    Fsr.Data = RoundToLongwordFromDouble(ROUND_TO_NEAREST,
                                         &DoubleOperand,
                                         &LongwordResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (LongwordResult != 0x80000000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = SIGN | ((DOUBLE_EXPONENT_BIAS + 31) << 20) | 0x1;
    Fsr.Data = RoundToLongwordFromDouble(ROUND_TO_NEAREST,
                                         &DoubleOperand,
                                         &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != INTEGER_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = SIGN | ((DOUBLE_EXPONENT_BIAS + 31) << 20) | 0x1;
    try {
        Fsr.Data = ConvertToLongwordFromDouble(EV | ROUND_TO_ZERO,
                                               &DoubleOperand,
                                               &LongwordResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = SIGN | ((DOUBLE_EXPONENT_BIAS + 31) << 20) | 0x1;
    try {
        Fsr.Data = TruncateToLongwordFromDouble(EV | ROUND_TO_ZERO,
                                                &DoubleOperand,
                                                &LongwordResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0xfff00000;
    DoubleOperand.HighPart = ((DOUBLE_EXPONENT_BIAS + 30) << 20) | 0xfffff;
    Fsr.Data = ConvertToLongwordFromDouble(ROUND_TO_NEAREST,
                                           &DoubleOperand,
                                           &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != INTEGER_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    DoubleOperand.LowPart = 0xfff00000;
    DoubleOperand.HighPart = ((DOUBLE_EXPONENT_BIAS + 30) << 20) | 0xfffff;
    try {
        Fsr.Data = ConvertToLongwordFromDouble(EV | ROUND_TO_NEAREST,
                                               &DoubleOperand,
                                               &LongwordResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0xfff00000;
    DoubleOperand.HighPart = ((DOUBLE_EXPONENT_BIAS + 30) << 20) | 0xfffff;
    Fsr.Data = RoundToLongwordFromDouble(ROUND_TO_NEAREST,
                                         &DoubleOperand,
                                         &LongwordResult);

    if ((Fsr.Data != (SV | ROUND_TO_NEAREST)) ||
        (LongwordResult != INTEGER_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    DoubleOperand.LowPart = 0xfff00000;
    DoubleOperand.HighPart = ((DOUBLE_EXPONENT_BIAS + 30) << 20) | 0xfffff;
    try {
        Fsr.Data = RoundToLongwordFromDouble(EV | ROUND_TO_NEAREST,
                                             &DoubleOperand,
                                             &LongwordResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    //
    // End of test 24.
    //

    printf("succeeded\n");
    return;

    //
    // Test 24 failed.
    //

TestFailed:
    printf(" subtest %d failed, fsr = %lx, result = %lx\n",
           Subtest,
           Fsr.Data,
           LongwordResult);

    return;
}

VOID
Test25 (
    VOID
    )

{

    ULONG Count;
    FLOATING_STATUS Fsr;
    ULARGE_INTEGER DoubleOperand;
    ULARGE_INTEGER DoubleResult;
    ULONG Subtest;

    //
    // Test 25 - Square root double test.
    //

    Subtest = 0;
    printf("    Test 25 - square root double ...");
    Subtest += 1;
    DoubleOperand.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand.HighPart = DOUBLE_QUIET_NAN;
    Fsr.Data = SquareRootDouble(ROUND_TO_NEAREST,
                                &DoubleOperand,
                                &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand.HighPart = DOUBLE_SIGNAL_NAN;
    Fsr.Data = SquareRootDouble(ROUND_TO_NEAREST,
                                &DoubleOperand,
                                &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    DoubleOperand.LowPart = DOUBLE_NAN_LOW;
    DoubleOperand.HighPart = DOUBLE_SIGNAL_NAN;
    try {
        Fsr.Data = SquareRootDouble(EV | ROUND_TO_NEAREST,
                                    &DoubleOperand,
                                    &DoubleResult);

    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        Count += 1;
    }

    if (Count == 0) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand.HighPart = DOUBLE_INFINITY_VALUE_HIGH;
    Fsr.Data = SquareRootDouble(ROUND_TO_NEAREST,
                                &DoubleOperand,
                                &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != DOUBLE_INFINITY_VALUE_LOW) ||
        (DoubleResult.HighPart != DOUBLE_INFINITY_VALUE_HIGH)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = DOUBLE_INFINITY_VALUE_LOW;
    DoubleOperand.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
    Fsr.Data = SquareRootDouble(ROUND_TO_NEAREST,
                                &DoubleOperand,
                                &DoubleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != DOUBLE_NAN_LOW) ||
        (DoubleResult.HighPart != DOUBLE_QUIET_NAN)) {
        goto TestFailed;
    }

//    Subtest += 1;
//    Count = 0;
//    DoubleOperand.LowPart = DOUBLE_INFINITY_VALUE_LOW;
//    DoubleOperand.HighPart = MINUS_DOUBLE_INFINITY_VALUE;
//    try {
//        Fsr.Data = SquareRootDouble(EV | ROUND_TO_NEAREST,
//                                    &DoubleOperand,
//                                    &DoubleResult);
//
//    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
//              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
//        Count += 1;
//    }
//
//    if (Count == 0) {
//        goto TestFailed;
//    }

    Subtest += 1;
    DoubleOperand.LowPart = 0;
    DoubleOperand.HighPart = 0;
    Fsr.Data = SquareRootDouble(ROUND_TO_NEAREST,
                                &DoubleOperand,
                                &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0) ||
        (DoubleResult.HighPart != 0)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0;
    DoubleOperand.HighPart = 0 | SIGN;
    Fsr.Data = SquareRootDouble(ROUND_TO_NEAREST,
                                &DoubleOperand,
                                &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0) ||
        (DoubleResult.HighPart != (0 | SIGN))) {
        goto TestFailed;
    }

// ****** //

    Subtest += 1;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = 0x40000;
    Fsr.Data = SquareRootDouble(ROUND_TO_NEAREST,
                                &DoubleOperand,
                                &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x80000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = 0x10000;
    Fsr.Data = SquareRootDouble(ROUND_TO_NEAREST,
                                &DoubleOperand,
                                &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x40000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = 0x4000;
    Fsr.Data = SquareRootDouble(ROUND_TO_NEAREST,
                                &DoubleOperand,
                                &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x20000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = 0x1000;
    Fsr.Data = SquareRootDouble(ROUND_TO_NEAREST,
                                &DoubleOperand,
                                &DoubleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (DoubleResult.LowPart != 0x0) ||
        (DoubleResult.HighPart != 0x10000)) {
        goto TestFailed;
    }

    Subtest += 1;
    DoubleOperand.LowPart = 0x0;
    DoubleOperand.HighPart = 0x80000;
    Fsr.Data = SquareRootDouble(ROUND_TO_NEAREST,
                                &DoubleOperand,
                                &DoubleResult);

    if ((Fsr.Data != (SU | SI | ROUND_TO_NEAREST)) ||
        (DoubleResult.LowPart != 0x333f9de6) ||
        (DoubleResult.HighPart != 0xb504f)) {
        goto TestFailed;
    }

// ****** //

    //
    // End of test 25.
    //

    printf("succeeded\n");
    return;

    //
    // Test 25 failed.
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
Test26 (
    VOID
    )

{

    ULONG Count;
    FLOATING_STATUS Fsr;
    ULONG Subtest;
    ULONG SingleResult;

    //
    // Test 26 - Square root single test.
    //

    Subtest = 0;
    printf("    Test 26 - square root single ...");
    Subtest += 1;
    Fsr.Data = SquareRootSingle(ROUND_TO_NEAREST,
                               SINGLE_QUIET_NAN,
                               &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = SquareRootSingle(ROUND_TO_NEAREST,
                               SINGLE_SIGNAL_NAN,
                               &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

    Subtest += 1;
    Count = 0;
    try {
        Fsr.Data = SquareRootSingle(EV | ROUND_TO_NEAREST,
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
    Fsr.Data = SquareRootSingle(ROUND_TO_NEAREST,
                                SINGLE_INFINITY_VALUE,
                                &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != SINGLE_INFINITY_VALUE)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = SquareRootSingle(ROUND_TO_NEAREST,
                                MINUS_SINGLE_INFINITY_VALUE,
                                &SingleResult);

    if ((Fsr.Data != (SV | XV | ROUND_TO_NEAREST)) ||
        (SingleResult != SINGLE_QUIET_NAN)) {
        goto TestFailed;
    }

//    Subtest += 1;
//    Count = 0;
//    try {
//        Fsr.Data = SquareRootSingle(EV | ROUND_TO_NEAREST,
//                                   MINUS_SINGLE_INFINITY_VALUE,
//                                   &SingleResult);
//
//    } except ((GetExceptionCode() == STATUS_FLOAT_INVALID_OPERATION) ?
//              EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
//        Count += 1;
//    }
//
//    if (Count == 0) {
//        goto TestFailed;
//    }

    Subtest += 1;
    Fsr.Data = SquareRootSingle(ROUND_TO_NEAREST,
                                0,
                                &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = SquareRootSingle(ROUND_TO_NEAREST,
                                0 | SIGN,
                                &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != (0 | SIGN))) {
        goto TestFailed;
    }

// ****** //

    Subtest += 1;
    Fsr.Data = SquareRootSingle(ROUND_TO_NEAREST,
                                0x200000,
                                &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x400000)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = SquareRootSingle(ROUND_TO_NEAREST,
                                0x80000,
                                &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x200000)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = SquareRootSingle(ROUND_TO_NEAREST,
                                0x20000,
                                &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x100000)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = SquareRootSingle(ROUND_TO_NEAREST,
                                0x8000,
                                &SingleResult);

    if ((Fsr.Data != ROUND_TO_NEAREST) ||
        (SingleResult != 0x80000)) {
        goto TestFailed;
    }

    Subtest += 1;
    Fsr.Data = SquareRootSingle(ROUND_TO_NEAREST,
                                0x400000,
                                &SingleResult);

    if ((Fsr.Data != (SU | SI | ROUND_TO_NEAREST)) ||
        (SingleResult != 0x5a8279)) {
        goto TestFailed;
    }

// ****** //

    //
    // End of test 26.
    //

    printf("succeeded\n");
    return;

    //
    // Test 26 failed.
    //

TestFailed:
    printf(" subtest %d failed, fsr = %lx, result = %lx\n",
           Subtest,
           Fsr.Data,
           SingleResult);

    return;
}
