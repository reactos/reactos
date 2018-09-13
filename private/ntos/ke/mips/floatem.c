/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    floatem.c

Abstract:

    This module implements a software emulation of the IEEE single and
    double floating operations. It is required on MIPS processors since
    the hardware does not fully support all of the operations required
    by the IEEE standard. In particular, infinitives and Nans are not
    handled by the hardware, but rather cause an exception. On receipt
    of the exception, a software emulation of the floating operation
    is performed to determine the real result of the operation and if
    an exception will actually be raised.

    Since floating exceptions are rather rare events, this routine is
    written in C. Should a higher performance implementation be required,
    then the algorithms contained herein, can be used to guide a higher
    performance assembly language implementation.

    N.B. This routine does not emulate floating loads, floating stores,
        control to/from floating, or move to/from floating instructions.
        These instructions either do not fault or are emulated elsewhere.

    Floating point operations are carried out by unpacking the operands,
    normalizing denormalized numbers, checking for NaNs, interpreting
    infinities, and computing results.

    Floating operands are converted to a format that has a value with the
    appropriate number of leading zeros, an overflow bit, the mantissa, a
    guard bit, a round bit, and a set of sticky bits.

    The overflow bit is needed for addition and is also used for multiply.
    The mantissa is 24-bits for single operations and 53-bits for double
    operations. The guard bit and round bit are used to hold precise values
    for normalization and rounding.

    If the result of an operation is normalized, then the guard bit becomes
    the round bit and the round bit is accumulated with the sticky bits. If
    the result of an operation needs to be shifted left one bit for purposes
    of nomalization, then the guard bit becomes part of the mantissa and the
    round bit is used for rounding.

    The round bit plus the sticky bits are used to determine how rounding is
    performed.

Author:

    David N. Cutler (davec) 16-Jun-1991

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// Define signaling NaN mask values.
//

#define DOUBLE_SIGNAL_NAN_MASK (1 << (53 - 32))
#define SINGLE_SIGNAL_NAN_MASK (1 << 24)

//
// Define quite NaN mask values.
//

#define DOUBLE_QUIET_NAN_MASK (1 << (51 - 32))
#define SINGLE_QUIET_NAN_MASK (1 << 22)

//
// Define quiet NaN prefix values.
//

#define DOUBLE_QUIET_NAN_PREFIX 0x7ff00000
#define SINGLE_QUIET_NAN_PREFIX 0x7f800000

//
// Define compare function masks.
//

#define COMPARE_UNORDERED_MASK (1 << 0)
#define COMPARE_EQUAL_MASK (1 << 1)
#define COMPARE_LESS_MASK (1 << 2)
#define COMPARE_ORDERED_MASK (1 << 3)

//
// Define context block structure.
//

typedef struct _FP_CONTEXT_BLOCK {
    ULONG Fd;
    ULONG BranchAddress;
    PEXCEPTION_RECORD ExceptionRecord;
    PKEXCEPTION_FRAME ExceptionFrame;
    PKTRAP_FRAME TrapFrame;
    ULONG Round;
} FP_CONTEXT_BLOCK, *PFP_CONTEXT_BLOCK;

//
// Define single and double operand value structures.
//

typedef struct _FP_DOUBLE_OPERAND {
    union {
        struct {
            ULONG MantissaLow;
            LONG MantissaHigh;
        };

        LONGLONG Mantissa;
    };

    LONG Exponent;
    LONG Sign;
    BOOLEAN Infinity;
    BOOLEAN Nan;
} FP_DOUBLE_OPERAND, *PFP_DOUBLE_OPERAND;

typedef struct _FP_SINGLE_OPERAND {
    LONG Mantissa;
    LONG Exponent;
    LONG Sign;
    BOOLEAN Infinity;
    BOOLEAN Nan;
} FP_SINGLE_OPERAND, *PFP_SINGLE_OPERAND;

//
// Define forward referenced function protypes.
//

BOOLEAN
KiDivideByZeroDouble (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN PFP_DOUBLE_OPERAND DoubleOperand1,
    IN PFP_DOUBLE_OPERAND DoubleOperand2
    );

BOOLEAN
KiDivideByZeroSingle (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN PFP_SINGLE_OPERAND SingleOperand1,
    IN PFP_SINGLE_OPERAND SingleOperand2
    );

BOOLEAN
KiInvalidCompareDouble (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN BOOLEAN CheckForNan,
    IN PFP_DOUBLE_OPERAND DoubleOperand1,
    IN PFP_DOUBLE_OPERAND DoubleOperand2
    );

BOOLEAN
KiInvalidCompareSingle (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN BOOLEAN CheckForNan,
    IN PFP_SINGLE_OPERAND SingleOperand1,
    IN PFP_SINGLE_OPERAND SingleOperand2
    );

BOOLEAN
KiInvalidOperationDouble (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN BOOLEAN CheckForNan,
    IN PFP_DOUBLE_OPERAND DoubleOperand1,
    IN PFP_DOUBLE_OPERAND DoubleOperand2
    );

BOOLEAN
KiInvalidOperationLongword (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN BOOLEAN Infinity,
    IN LONG Sign
    );

BOOLEAN
KiInvalidOperationQuadword (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN BOOLEAN Infinity,
    IN LONG Sign
    );

BOOLEAN
KiInvalidOperationSingle (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN BOOLEAN CheckForNan,
    IN PFP_SINGLE_OPERAND SingleOperand1,
    IN PFP_SINGLE_OPERAND SingleOperand2
    );

BOOLEAN
KiNormalizeDouble (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN PFP_DOUBLE_OPERAND ResultOperand,
    IN ULONG StickyBits
    );

BOOLEAN
KiNormalizeLongword (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN PFP_DOUBLE_OPERAND ResultOperand
    );

BOOLEAN
KiNormalizeQuadword (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN PFP_DOUBLE_OPERAND ResultOperand
    );

BOOLEAN
KiNormalizeSingle (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN PFP_SINGLE_OPERAND ResultOperand,
    IN ULONG StickyBits
    );

ULONG
KiSquareRootDouble (
    IN PULARGE_INTEGER DoubleValue
    );

ULONG
KiSquareRootSingle (
    IN PULONG SingleValue
    );

VOID
KiUnpackDouble (
    IN ULONG Source,
    IN PFP_CONTEXT_BLOCK ContextBlock,
    OUT PFP_DOUBLE_OPERAND DoubleOperand
    );

VOID
KiUnpackSingle (
    IN ULONG Source,
    IN PFP_CONTEXT_BLOCK ContextBlock,
    OUT PFP_SINGLE_OPERAND SingleOperand
    );

BOOLEAN
KiEmulateFloating (
    IN OUT PEXCEPTION_RECORD ExceptionRecord,
    IN OUT PKEXCEPTION_FRAME ExceptionFrame,
    IN OUT PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called to emulate a floating operation and convert the
    exception status to the proper value. If the exception is an unimplemented
    operation, then the operation is emulated. Otherwise, the status code is
    just converted to its proper value.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    A value of TRUE is returned if the floating exception is successfully
    emulated. Otherwise, a value of FALSE is returned.

--*/

{

    ULARGE_INTEGER AhighBhigh;
    ULARGE_INTEGER AhighBlow;
    ULARGE_INTEGER AlowBhigh;
    ULARGE_INTEGER AlowBlow;
    ULONG Carry1;
    ULONG Carry2;
    BOOLEAN CompareEqual;
    ULONG CompareFunction;
    BOOLEAN CompareLess;
    FP_CONTEXT_BLOCK ContextBlock;
    LARGE_INTEGER DoubleDividend;
    LARGE_INTEGER DoubleDivisor;
    ULARGE_INTEGER DoubleValue;
    ULONG DoubleMantissaLow;
    LONG DoubleMantissaHigh;
    FP_DOUBLE_OPERAND DoubleOperand1;
    FP_DOUBLE_OPERAND DoubleOperand2;
    FP_DOUBLE_OPERAND DoubleOperand3;
    LARGE_INTEGER DoubleQuotient;
    PVOID ExceptionAddress;
    ULONG ExponentDifference;
    ULONG ExponentSum;
    ULONG Format;
    ULONG Fs;
    ULONG Ft;
    ULONG Function;
    ULONG Index;
    MIPS_INSTRUCTION Instruction;
    ULARGE_INTEGER LargeResult;
    LONG Longword;
    LONG Negation;
    union {
        LONGLONG Quadword;
        LARGE_INTEGER LargeValue;
    }u;

    LONG SingleMantissa;
    FP_SINGLE_OPERAND SingleOperand1;
    FP_SINGLE_OPERAND SingleOperand2;
    FP_SINGLE_OPERAND SingleOperand3;
    ULONG SingleValue;
    ULONG StickyBits;

    //
    // Save the original exception address in case another exception
    // occurs.
    //

    ExceptionAddress = ExceptionRecord->ExceptionAddress;

    //
    // Any exception that occurs during the attempted emulation of the
    // floating operation causes the emulation to be aborted. The new
    // exception code and information is copied to the original exception
    // record and a value of FALSE is returned.
    //

    try {

        //
        // If the exception PC is equal to the fault instruction address
        // plus four, then the floating exception occurred in the delay
        // slot of a branch instruction and the continuation address must
        // be computed by emulating the branch instruction. Note that it
        // is possible for an exception to occur when the branch instruction
        // is read from user memory.
        //

        if ((TrapFrame->Fir + 4) == (ULONG)ExceptionRecord->ExceptionAddress) {
            ContextBlock.BranchAddress = KiEmulateBranch(ExceptionFrame,
                                                         TrapFrame);

        } else {
            ContextBlock.BranchAddress = TrapFrame->Fir + 4;
        }

        //
        // Increment the floating emulation count.
        //

        KeGetCurrentPrcb()->KeFloatingEmulationCount += 1;

        //
        // Initialize the address of the exception record, exception frame,
        // and trap frame in the context block used during the emulation of
        // the floating point operation.
        //

        ContextBlock.ExceptionRecord = ExceptionRecord;
        ContextBlock.ExceptionFrame = ExceptionFrame;
        ContextBlock.TrapFrame = TrapFrame;
        ContextBlock.Round = ((PFSR)&TrapFrame->Fsr)->RM;

        //
        // Initialize the number of exception information parameters, set
        // the branch address, and clear the IEEE exception value.
        //

        ExceptionRecord->NumberParameters = 6;
        ExceptionRecord->ExceptionInformation[0] = 0;
        ExceptionRecord->ExceptionInformation[1] = ContextBlock.BranchAddress;
        ExceptionRecord->ExceptionInformation[2] = 0;
        ExceptionRecord->ExceptionInformation[3] = 0;
        ExceptionRecord->ExceptionInformation[4] = 0;
        ExceptionRecord->ExceptionInformation[5] = 0;

        //
        // Clear all exception flags and emulate the floating point operation
        // The return value is dependent on the results of the emulation.
        //

        TrapFrame->Fsr &= ~(0x3f << 12);
        Instruction = *((PMIPS_INSTRUCTION)ExceptionRecord->ExceptionAddress);
        Function = Instruction.c_format.Function;
        ContextBlock.Fd = Instruction.c_format.Fd;
        Fs = Instruction.c_format.Fs;
        Ft = Instruction.c_format.Ft;
        Format = Instruction.c_format.Format;
        Negation = 0;

        //
        // Check for illegal register specification or format code.
        //

        if (((ContextBlock.Fd & 0x1) != 0) || ((Fs & 0x1) != 0) || ((Ft & 0x1) != 0) ||
            ((Format != FORMAT_LONGWORD) && (Format != FORMAT_QUADWORD) && (Format > FORMAT_DOUBLE))) {
            Function = FLOAT_ILLEGAL;
        }

        //
        // Decode operand values and dispose with NaNs.
        //

        if ((Function <= FLOAT_DIVIDE) || (Function >= FLOAT_COMPARE)) {

            //
            // The function has two operand values.
            //

            if (Format == FORMAT_SINGLE) {
                KiUnpackSingle(Fs, &ContextBlock, &SingleOperand1);
                KiUnpackSingle(Ft, &ContextBlock, &SingleOperand2);

                //
                // If either operand is a NaN, then check to determine if a
                // compare instruction or other dyadic operation is being
                // performed.
                //

                if ((SingleOperand1.Nan != FALSE) || (SingleOperand2.Nan != FALSE)) {
                    if (Function < FLOAT_COMPARE) {

                        //
                        // Dyadic operation.
                        //
                        // Store a quite Nan if the invalid operation trap
                        // is disabled, or raise an exception if the invalid
                        // operation trap is enabled and either of the NaNs
                        // is a signally NaN.
                        //

                        return KiInvalidOperationSingle(&ContextBlock,
                                                        TRUE,
                                                        &SingleOperand1,
                                                        &SingleOperand2);

                    } else {

                        //
                        // Compare operation.
                        //
                        // Set the condition based on the predicate of
                        // the floating comparison.
                        //
                        // If the compare is a signaling compare, then
                        // raise an exception if the invalid operation
                        // trap is enabled. Otherwise, raise an exception
                        // if one of the operands is a signaling NaN.
                        //

                        if ((Function & COMPARE_UNORDERED_MASK) != 0) {
                            ((PFSR)&TrapFrame->Fsr)->CC = 1;

                        } else {
                            ((PFSR)&TrapFrame->Fsr)->CC = 0;
                        }

                        if ((Function & COMPARE_ORDERED_MASK) != 0) {
                            return KiInvalidCompareSingle(&ContextBlock,
                                                          FALSE,
                                                          &SingleOperand1,
                                                          &SingleOperand2);

                        } else {
                            return KiInvalidCompareSingle(&ContextBlock,
                                                          TRUE,
                                                          &SingleOperand1,
                                                          &SingleOperand2);

                        }
                    }

                } else if (Function >= FLOAT_COMPARE) {
                    CompareFunction = Function;
                    Function = FLOAT_COMPARE_SINGLE;
                }

            } else if (Format == FORMAT_DOUBLE) {
                KiUnpackDouble(Fs, &ContextBlock, &DoubleOperand1);
                KiUnpackDouble(Ft, &ContextBlock, &DoubleOperand2);

                //
                // If either operand is a NaN, then check to determine if a
                // compare instruction or other dyadic operation is being
                // performed.
                //

                if ((DoubleOperand1.Nan != FALSE) || (DoubleOperand2.Nan != FALSE)) {
                    if (Function < FLOAT_COMPARE) {

                        //
                        // Dyadic operation.
                        //
                        // Store a quite Nan if the invalid operation trap
                        // is disabled, or raise an exception if the invalid
                        // operation trap is enabled and either of the NaNs
                        // is a signally NaN.
                        //

                        return KiInvalidOperationDouble(&ContextBlock,
                                                        TRUE,
                                                        &DoubleOperand1,
                                                        &DoubleOperand2);

                    } else {

                        //
                        // Compare operation.
                        //
                        // Set the condition based on the predicate of
                        // the floating comparison.
                        //
                        // If the compare is a signaling compare, then
                        // raise an exception if the invalid operation
                        // trap is enabled. Othersie, raise an exception
                        // if one of the operands is a signaling NaN.
                        //

                        if ((Function & COMPARE_UNORDERED_MASK) != 0) {
                            ((PFSR)&TrapFrame->Fsr)->CC = 1;

                        } else {
                            ((PFSR)&TrapFrame->Fsr)->CC = 0;
                        }

                        if ((Function & COMPARE_ORDERED_MASK) != 0) {
                            return KiInvalidCompareDouble(&ContextBlock,
                                                          FALSE,
                                                          &DoubleOperand1,
                                                          &DoubleOperand2);

                        } else {
                            return KiInvalidCompareDouble(&ContextBlock,
                                                          TRUE,
                                                          &DoubleOperand1,
                                                          &DoubleOperand2);

                        }
                    }

                } else if (Function >= FLOAT_COMPARE) {
                    CompareFunction = Function;
                    Function = FLOAT_COMPARE_DOUBLE;
                }

            } else {
                Function = FLOAT_ILLEGAL;
            }

        } else {

            //
            // The function has one operand value.
            //

            if (Format == FORMAT_SINGLE) {
                KiUnpackSingle(Fs, &ContextBlock, &SingleOperand1);

                //
                // If the operand is a NaN and the function is not a convert
                // operation, then store a quiet NaN if the invalid operation
                // trap is disabled, or raise an exception if the invalid
                // operation trap is enabled and the operand is a signaling
                // NaN.
                //

                if ((SingleOperand1.Nan != FALSE) &&
                    (Function < FLOAT_ROUND_QUADWORD) ||
                    (Function > FLOAT_CONVERT_QUADWORD) ||
                    ((Function > FLOAT_FLOOR_LONGWORD) &&
                    (Function < FLOAT_CONVERT_SINGLE))) {
                    return KiInvalidOperationSingle(&ContextBlock,
                                                    TRUE,
                                                    &SingleOperand1,
                                                    &SingleOperand1);

                }

            } else if (Format == FORMAT_DOUBLE) {
                KiUnpackDouble(Fs, &ContextBlock, &DoubleOperand1);

                //
                // If the operand is a NaN and the function is not a convert
                // operation, then store a quiet NaN if the invalid operation
                // trap is disabled, or raise an exception if the invalid
                // operation trap is enabled and the operand is a signaling
                // NaN.
                //

                if ((DoubleOperand1.Nan != FALSE) &&
                    (Function < FLOAT_ROUND_QUADWORD) ||
                    (Function > FLOAT_CONVERT_QUADWORD) ||
                    ((Function > FLOAT_FLOOR_LONGWORD) &&
                    (Function < FLOAT_CONVERT_SINGLE))) {
                    return KiInvalidOperationDouble(&ContextBlock,
                                                    TRUE,
                                                    &DoubleOperand1,
                                                    &DoubleOperand1);
                }

            } else if ((Format == FORMAT_LONGWORD) &&
                       (Function >= FLOAT_CONVERT_SINGLE)) {
                Longword = KiGetRegisterValue(Fs + 32,
                                              ContextBlock.ExceptionFrame,
                                              ContextBlock.TrapFrame);

            } else if ((Format == FORMAT_QUADWORD) &&
                       (Function >= FLOAT_CONVERT_SINGLE)) {
                u.LargeValue.LowPart = KiGetRegisterValue(Fs + 32,
                                                          ContextBlock.ExceptionFrame,
                                                          ContextBlock.TrapFrame);

                u.LargeValue.HighPart = KiGetRegisterValue(Fs + 33,
                                                           ContextBlock.ExceptionFrame,
                                                           ContextBlock.TrapFrame);

            } else {
                Function = FLOAT_ILLEGAL;
            }
        }

        //
        // Case to the proper function routine to emulate the operation.
        //

        switch (Function) {

            //
            // Floating subtract operation.
            //
            // Floating subtract is accomplished by complementing the sign
            // of the second operand and then performing an add operation.
            //

        case FLOAT_SUBTRACT:
            Negation = 0x1;

            //
            // Floating add operation.
            //
            // Floating add is accomplished using signed magnitude addition.
            //
            // The exponent difference is calculated and the smaller number
            // is right shifted by the specified amount, but no more than
            // the width of the operand values (i.e., 26 for single and 55
            // for double). The shifted out value is saved for rounding.
            //
            // If the signs of the two operands are the same, then they
            // are added together after having performed the alignment
            // shift.
            //
            // If the signs of the two operands are different, then the
            // sign of the result is the sign of the larger operand and
            // the smaller operand is subtracted from the larger operand.
            // In order to avoid making a double level test (i.e., one on
            // the exponents, and one on the mantissas if the exponents
            // are equal), it is posible that the result of the subtract
            // could be negative (if the exponents are equal). If this
            // occurs, then the result sign and mantissa are complemented
            // to obtain the correct result.
            //

        case FLOAT_ADD:
            if (Format == FORMAT_SINGLE) {

                //
                // Complement the sign of the second operand if the operation
                // is subtraction.
                //

                SingleOperand2.Sign ^= Negation;

                //
                // Reorder then operands according to their exponent value.
                //

                if (SingleOperand2.Exponent > SingleOperand1.Exponent) {
                    SingleOperand3 = SingleOperand2;
                    SingleOperand2 = SingleOperand1;
                    SingleOperand1 = SingleOperand3;
                }

                //
                // Compute the exponent difference and shift the smaller
                // mantissa right by the difference value or 26 which ever
                // is smaller. The bits shifted out are termed the sticky
                // bits and are used later in the rounding operation.
                //

                ExponentDifference =
                            SingleOperand1.Exponent - SingleOperand2.Exponent;

                if (ExponentDifference > 26) {
                    ExponentDifference = 26;
                }

                StickyBits =
                        SingleOperand2.Mantissa & ((1 << ExponentDifference) - 1);
                SingleMantissa = SingleOperand2.Mantissa >> ExponentDifference;

                //
                // If the operands both have the same sign, then perform the
                // operation by adding the values together. Otherwise, perform
                // the operation by subtracting the second operand from the
                // first operand.
                //

                if ((SingleOperand1.Sign ^ SingleOperand2.Sign) == 0) {
                    SingleOperand1.Mantissa += SingleMantissa;

                } else {
                    if ((SingleOperand1.Infinity != FALSE) &&
                        (SingleOperand2.Infinity != FALSE)) {
                        return KiInvalidOperationSingle(&ContextBlock,
                                                        FALSE,
                                                        &SingleOperand1,
                                                        &SingleOperand2);

                    } else if (SingleOperand1.Infinity == FALSE) {
                        if (StickyBits != 0) {
                            SingleOperand1.Mantissa -= 1;
                        }

                        SingleOperand1.Mantissa -= SingleMantissa;
                        if (SingleOperand1.Mantissa < 0) {
                            SingleOperand1.Mantissa = -SingleOperand1.Mantissa;
                            SingleOperand1.Sign ^= 0x1;
                        }
                    }
                }

                //
                // Normalize and store the result value.
                //

                return KiNormalizeSingle(&ContextBlock,
                                         &SingleOperand1,
                                         StickyBits);

            } else if (Format == FORMAT_DOUBLE) {

                //
                // Complement the sign of the second operand if the operation
                // is subtraction.
                //

                DoubleOperand2.Sign ^= Negation;

                //
                // Reorder then operands according to their exponent value.
                //

                if (DoubleOperand2.Exponent > DoubleOperand1.Exponent) {
                    DoubleOperand3 = DoubleOperand2;
                    DoubleOperand2 = DoubleOperand1;
                    DoubleOperand1 = DoubleOperand3;
                }

                //
                // Compute the exponent difference and shift the smaller
                // mantissa right by the difference value or 55 which ever
                // is smaller. The bits shifted out are termed the sticky
                // bits and are used later in the rounding operation.
                //

                ExponentDifference =
                        DoubleOperand1.Exponent - DoubleOperand2.Exponent;

                if (ExponentDifference > 55) {
                    ExponentDifference = 55;
                }

                if (ExponentDifference >= 32) {
                    ExponentDifference -= 32;
                    StickyBits = (DoubleOperand2.MantissaLow) |
                        (DoubleOperand2.MantissaHigh & ((1 << ExponentDifference) - 1));

                    DoubleMantissaLow =
                        DoubleOperand2.MantissaHigh >> ExponentDifference;

                    DoubleMantissaHigh = 0;

                } else if (ExponentDifference > 0) {
                    StickyBits =
                        DoubleOperand2.MantissaLow & ((1 << ExponentDifference) - 1);

                    DoubleMantissaLow =
                        (DoubleOperand2.MantissaLow >> ExponentDifference) |
                        (DoubleOperand2.MantissaHigh << (32 - ExponentDifference));

                    DoubleMantissaHigh =
                        DoubleOperand2.MantissaHigh >> ExponentDifference;

                } else {
                    StickyBits = 0;
                    DoubleMantissaLow = DoubleOperand2.MantissaLow;
                    DoubleMantissaHigh = DoubleOperand2.MantissaHigh;
                }

                //
                // If the operands both have the same sign, then perform the
                // operation by adding the values together. Otherwise, perform
                // the operation by subtracting the second operand from the
                // first operand.
                //

                if ((DoubleOperand1.Sign ^ DoubleOperand2.Sign) == 0) {
                    DoubleOperand1.MantissaLow += DoubleMantissaLow;
                    DoubleOperand1.MantissaHigh += DoubleMantissaHigh;
                    if (DoubleOperand1.MantissaLow < DoubleMantissaLow) {
                        DoubleOperand1.MantissaHigh += 1;
                    }

                } else {
                    if ((DoubleOperand1.Infinity != FALSE) &&
                        (DoubleOperand2.Infinity != FALSE)) {
                        return KiInvalidOperationDouble(&ContextBlock,
                                                        FALSE,
                                                        &DoubleOperand1,
                                                        &DoubleOperand2);

                    } else if (DoubleOperand1.Infinity == FALSE) {
                        if (StickyBits != 0) {
                            if (DoubleOperand1.MantissaLow < 1) {
                                DoubleOperand1.MantissaHigh -= 1;
                            }

                            DoubleOperand1.MantissaLow -= 1;
                        }

                        if (DoubleOperand1.MantissaLow < DoubleMantissaLow) {
                            DoubleOperand1.MantissaHigh -= 1;
                        }

                        DoubleOperand1.MantissaLow -= DoubleMantissaLow;
                        DoubleOperand1.MantissaHigh -= DoubleMantissaHigh;
                        if (DoubleOperand1.MantissaHigh < 0) {
                            DoubleOperand1.MantissaLow = ~DoubleOperand1.MantissaLow + 1;
                            DoubleOperand1.MantissaHigh = -DoubleOperand1.MantissaHigh;
                            if (DoubleOperand1.MantissaLow != 0) {
                                DoubleOperand1.MantissaHigh -= 1;
                            }

                            DoubleOperand1.Sign ^= 0x1;
                        }
                    }
                }

                //
                // Normalize and store the result value.
                //

                return KiNormalizeDouble(&ContextBlock,
                                         &DoubleOperand1,
                                         StickyBits);

            } else {
                break;
            }

            //
            // Floating multiply operation.
            //
            // Floating multiply is accomplished using unsigned multiplies
            // of the mantissa values, and adding the parital results together
            // to form the total product.
            //
            // The two mantissa values are preshifted such that the final
            // result is properly aligned.
            //

        case FLOAT_MULTIPLY:
            if (Format == FORMAT_SINGLE) {

                //
                // Reorder the operands according to their exponent value.
                //

                if (SingleOperand2.Exponent > SingleOperand1.Exponent) {
                    SingleOperand3 = SingleOperand2;
                    SingleOperand2 = SingleOperand1;
                    SingleOperand1 = SingleOperand3;
                }

                //
                // If the first operand is infinite and the second operand is
                // zero, then an invalid operation is specified.
                //

                if ((SingleOperand1.Infinity != FALSE) &&
                    (SingleOperand2.Infinity == FALSE) &&
                    (SingleOperand2.Mantissa == 0)) {
                    return KiInvalidOperationSingle(&ContextBlock,
                                                    FALSE,
                                                    &SingleOperand1,
                                                    &SingleOperand2);

                }

                //
                // Preshift the operand mantissas so the result will be a
                // properly aligned 64-bit value and then unsigned multiply
                // the two mantissa values. The single result is the high part
                // of the 64-bit product and the sticky bits are the low part
                // of the 64-bit product.
                //

                LargeResult.QuadPart = UInt32x32To64(SingleOperand1.Mantissa << (32 - 26),
                                                     SingleOperand2.Mantissa << 1);

                SingleOperand1.Mantissa = LargeResult.HighPart;
                StickyBits = LargeResult.LowPart;

                //
                // Compute the sign and exponent of the result.
                //

                SingleOperand1.Sign ^= SingleOperand2.Sign;
                SingleOperand1.Exponent +=
                            SingleOperand2.Exponent - SINGLE_EXPONENT_BIAS;

                //
                // Normalize and store the result value.
                //

                return KiNormalizeSingle(&ContextBlock,
                                         &SingleOperand1,
                                         StickyBits);

            } else if (Format == FORMAT_DOUBLE) {

                //
                // Reorder the operands according to their exponent value.
                //

                if (DoubleOperand2.Exponent > DoubleOperand1.Exponent) {
                    DoubleOperand3 = DoubleOperand2;
                    DoubleOperand2 = DoubleOperand1;
                    DoubleOperand1 = DoubleOperand3;
                }

                //
                // If the first operand is infinite and the second operand is
                // zero, then an invalid operation is specified.
                //

                if ((DoubleOperand1.Infinity != FALSE) &&
                    (DoubleOperand2.Infinity == FALSE) &&
                    (DoubleOperand2.MantissaHigh == 0)) {
                    return KiInvalidOperationDouble(&ContextBlock,
                                                    FALSE,
                                                    &DoubleOperand1,
                                                    &DoubleOperand2);

                }

                //
                // Preshift the operand mantissas so the result will be a
                // properly aligned 128-bit value and then unsigned multiply
                // the two mantissa values. The double result is the high part
                // of the 128-bit product and the sticky bits are the low part
                // of the 128-bit product.
                //

                DoubleOperand1.MantissaHigh =
                        (DoubleOperand1.MantissaHigh << 1) |
                                (DoubleOperand1.MantissaLow >> 31);

                DoubleOperand1.MantissaLow <<= 1;
                DoubleOperand2.MantissaHigh =
                        (DoubleOperand2.MantissaHigh << (64 - 55)) |
                                (DoubleOperand2.MantissaLow >> (32 - (64 -55)));

                DoubleOperand2.MantissaLow <<= (64 - 55);

                //
                // The 128-bit product is formed by mutiplying and adding
                // all the cross product values.
                //
                // Consider the operands (A and B) as being composed of two
                // parts Ahigh, Alow, Bhigh, and Blow. The cross product sum
                // is then:
                //
                //       Ahigh * Bhigh * 2^64 +
                //              Ahigh * Blow * 2^32 +
                //              Alow * Bhigh * 2^32 +
                //                              Alow * Blow
                //

                AhighBhigh.QuadPart = UInt32x32To64(DoubleOperand1.MantissaHigh,
                                                    DoubleOperand2.MantissaHigh);

                AhighBlow.QuadPart = UInt32x32To64(DoubleOperand1.MantissaHigh,
                                                   DoubleOperand2.MantissaLow);

                AlowBhigh.QuadPart = UInt32x32To64(DoubleOperand1.MantissaLow,
                                                   DoubleOperand2.MantissaHigh);

                AlowBlow.QuadPart = UInt32x32To64(DoubleOperand1.MantissaLow,
                                                  DoubleOperand2.MantissaLow);

                AlowBlow.HighPart += AhighBlow.LowPart;
                if (AlowBlow.HighPart < AhighBlow.LowPart) {
                    Carry1 = 1;

                } else {
                    Carry1 = 0;
                }

                AlowBlow.HighPart += AlowBhigh.LowPart;
                if (AlowBlow.HighPart < AlowBhigh.LowPart) {
                    Carry1 += 1;
                }

                DoubleOperand1.MantissaLow = AhighBlow.HighPart + Carry1;
                if (DoubleOperand1.MantissaLow < Carry1) {
                    Carry2 = 1;

                } else {
                    Carry2 = 0;
                }

                DoubleOperand1.MantissaLow += AlowBhigh.HighPart;
                if (DoubleOperand1.MantissaLow < AlowBhigh.HighPart) {
                    Carry2 += 1;
                }

                DoubleOperand1.MantissaLow += AhighBhigh.LowPart;
                if (DoubleOperand1.MantissaLow < AhighBhigh.LowPart) {
                    Carry2 += 1;
                }

                DoubleOperand1.MantissaHigh = AhighBhigh.HighPart + Carry2;
                StickyBits = AlowBlow.HighPart | AlowBlow.LowPart;

                //
                // Compute the sign and exponent of the result.
                //

                DoubleOperand1.Sign ^= DoubleOperand2.Sign;
                DoubleOperand1.Exponent +=
                            DoubleOperand2.Exponent - DOUBLE_EXPONENT_BIAS;

                //
                // Normalize and store the result value.
                //

                return KiNormalizeDouble(&ContextBlock,
                                         &DoubleOperand1,
                                         StickyBits);

            } else {
                break;
            }

            //
            // Floating divide operation.
            //
            // Floating division is accomplished by repeated subtract using
            // a single one-bit-at-a-time algorithm. The number of division
            // steps performed is equal to the mantissa size plus one guard
            // bit.
            //
            // The sticky bits are the remainder after the specified number
            // of division steps.
            //

        case FLOAT_DIVIDE:
            if (Format == FORMAT_SINGLE) {

                //
                // If the first operand is infinite and the second operand
                // is infinite, or both operands are zero, then an invalid
                // operation is specified.
                //

                if (((SingleOperand1.Infinity != FALSE) &&
                    (SingleOperand2.Infinity != FALSE)) ||
                    ((SingleOperand1.Infinity == FALSE) &&
                    (SingleOperand1.Mantissa == 0) &&
                    (SingleOperand2.Infinity == FALSE) &&
                    (SingleOperand2.Mantissa == 0))) {
                    return KiInvalidOperationSingle(&ContextBlock,
                                                    FALSE,
                                                    &SingleOperand1,
                                                    &SingleOperand2);

                }

                //
                // If the second operand is zero, then a divide by zero
                // operation is specified.
                //

                if ((SingleOperand2.Infinity == FALSE) &&
                    (SingleOperand2.Mantissa == 0)) {
                    return KiDivideByZeroSingle(&ContextBlock,
                                                &SingleOperand1,
                                                &SingleOperand2);
                }

                //
                // If the first operand is infinite, then the result is
                // infinite. Otherwise, if the second operand is infinite,
                // then the result is zero (note that both operands cannot
                // be infinite).
                //

                if (SingleOperand1.Infinity != FALSE) {
                    SingleOperand1.Sign ^= SingleOperand2.Sign;
                    return KiNormalizeSingle(&ContextBlock,
                                             &SingleOperand1,
                                             0);

                } else if (SingleOperand2.Infinity != FALSE) {
                    SingleOperand1.Sign ^= SingleOperand2.Sign;
                    SingleOperand1.Exponent = 0;
                    SingleOperand1.Mantissa = 0;
                    return KiNormalizeSingle(&ContextBlock,
                                             &SingleOperand1,
                                             0);

                }

                //
                // Perform divide operation by repeating a single bit
                // divide step 26 iterations.
                //

                SingleOperand3.Mantissa = 0;
                for (Index = 0; Index < 26; Index += 1) {
                    SingleOperand3.Mantissa <<=1;
                    if (SingleOperand1.Mantissa >= SingleOperand2.Mantissa) {
                        SingleOperand1.Mantissa -= SingleOperand2.Mantissa;
                        SingleOperand3.Mantissa |= 1;
                    }

                    SingleOperand1.Mantissa <<= 1;
                }

                //
                // Compute the sign and exponent of the result.
                //

                SingleOperand3.Sign = SingleOperand1.Sign ^ SingleOperand2.Sign;
                SingleOperand3.Exponent = SingleOperand1.Exponent -
                                SingleOperand2.Exponent + SINGLE_EXPONENT_BIAS;

                //
                // Normalize and store the result value.
                //

                SingleOperand3.Infinity = FALSE;
                SingleOperand3.Nan = FALSE;
                return KiNormalizeSingle(&ContextBlock,
                                         &SingleOperand3,
                                         SingleOperand1.Mantissa);

            } else if (Format == FORMAT_DOUBLE) {

                //
                // If the first operand is infinite and the second operand
                // is infinite, or both operands are zero, then an invalid
                // operation is specified.
                //

                if (((DoubleOperand1.Infinity != FALSE) &&
                    (DoubleOperand2.Infinity != FALSE)) ||
                    ((DoubleOperand1.Infinity == FALSE) &&
                    (DoubleOperand1.MantissaHigh == 0) &&
                    (DoubleOperand2.Infinity == FALSE) &&
                    (DoubleOperand2.MantissaHigh == 0))) {
                    return KiInvalidOperationDouble(&ContextBlock,
                                                    FALSE,
                                                    &DoubleOperand1,
                                                    &DoubleOperand2);

                }

                //
                // If the second operand is zero, then a divide by zero
                // operation is specified.
                //

                if ((DoubleOperand2.Infinity == FALSE) &&
                    (DoubleOperand2.MantissaHigh == 0)) {
                    return KiDivideByZeroDouble(&ContextBlock,
                                                &DoubleOperand1,
                                                &DoubleOperand2);
                }

                //
                // If the first operand is infinite, then the result is
                // infinite. Otherwise, if the second operand is infinite,
                // then the result is zero (note that both operands cannot
                // be infinite).
                //

                if (DoubleOperand1.Infinity != FALSE) {
                    DoubleOperand1.Sign ^= DoubleOperand2.Sign;
                    return KiNormalizeDouble(&ContextBlock,
                                             &DoubleOperand1,
                                             0);

                } else if (DoubleOperand2.Infinity != FALSE) {
                    DoubleOperand1.Sign ^= DoubleOperand2.Sign;
                    DoubleOperand1.Exponent = 0;
                    DoubleOperand1.MantissaHigh = 0;
                    DoubleOperand1.MantissaLow = 0;
                    return KiNormalizeDouble(&ContextBlock,
                                             &DoubleOperand1,
                                             0);

                }

                //
                // Perform divide operation by repeating a single bit
                // divide step 55 iterations.
                //

                DoubleDividend.LowPart = DoubleOperand1.MantissaLow;
                DoubleDividend.HighPart = DoubleOperand1.MantissaHigh;
                DoubleDivisor.LowPart = DoubleOperand2.MantissaLow;
                DoubleDivisor.HighPart = DoubleOperand2.MantissaHigh;
                DoubleQuotient.LowPart = 0;
                DoubleQuotient.HighPart = 0;
                for (Index = 0; Index < 55; Index += 1) {
                    DoubleQuotient.HighPart =
                                (DoubleQuotient.HighPart << 1) |
                                                DoubleQuotient.LowPart >> 31;

                    DoubleQuotient.LowPart <<= 1;
                    if (DoubleDividend.QuadPart >= DoubleDivisor.QuadPart) {
                        DoubleDividend.QuadPart -= DoubleDivisor.QuadPart;
                        DoubleQuotient.LowPart |= 1;
                    }

                    DoubleDividend.HighPart =
                                (DoubleDividend.HighPart << 1) |
                                                DoubleDividend.LowPart >> 31;

                    DoubleDividend.LowPart <<= 1;
                }

                DoubleOperand3.MantissaLow = DoubleQuotient.LowPart;
                DoubleOperand3.MantissaHigh = DoubleQuotient.HighPart;

                //
                // Compute the sign and exponent of the result.
                //

                DoubleOperand3.Sign = DoubleOperand1.Sign ^ DoubleOperand2.Sign;
                DoubleOperand3.Exponent = DoubleOperand1.Exponent -
                                DoubleOperand2.Exponent + DOUBLE_EXPONENT_BIAS;

                //
                // Normalize and store the result value.
                //

                DoubleOperand3.Infinity = FALSE;
                DoubleOperand3.Nan = FALSE;
                return KiNormalizeDouble(&ContextBlock,
                                         &DoubleOperand3,
                                         DoubleDividend.LowPart | DoubleDividend.HighPart);

            } else {
                break;
            }

            //
            // Floating square root.
            //

        case FLOAT_SQUARE_ROOT:
            if (Format == FORMAT_SINGLE) {

                //
                // If the operand is plus infinity, then the result is
                // plus infinity, or if the operand is plus or minus
                // zero, then the result is plus or minus zero.
                //

                if (((SingleOperand1.Sign == 0) &&
                   (SingleOperand1.Infinity != FALSE)) ||
                   (SingleOperand1.Mantissa == 0)) {
                    return KiNormalizeSingle(&ContextBlock,
                                             &SingleOperand1,
                                             0);
                }

                //
                // If the operand is negative, then the operation is
                // invalid.
                //

                if (SingleOperand1.Sign != 0) {
                    return KiInvalidOperationSingle(&ContextBlock,
                                                    FALSE,
                                                    &SingleOperand1,
                                                    &SingleOperand1);
                }

                //
                // The only case remaining that could cause an exception
                // is a denomalized source value. The square root of a
                // denormalized value is computed by:
                //
                //   1. Converting the value to a normalized value with
                //      an exponent equal to the denormalization shift count
                //      plus the bias of the exponent plus one.
                //
                //   2. Computing the square root of the value and unpacking
                //      the result.
                //
                //   3. Converting the shift count back to a normalization
                //      shift count.
                //
                //   4. Rounding and packing the resultant value.
                //
                // N.B. The square root of all denormalized number is a
                //      normalized number.
                //

                SingleOperand1.Exponent = (SINGLE_EXPONENT_BIAS + 1 +
                                            SingleOperand1.Exponent) << 23;

                SingleValue = (SingleOperand1.Mantissa & ~(1 << 25)) >> 2;
                SingleValue |= SingleOperand1.Exponent;
                StickyBits = KiSquareRootSingle(&SingleValue);
                SingleOperand1.Exponent =  (SingleValue >> 23) -
                                            ((SINGLE_EXPONENT_BIAS + 1) / 2);

                SingleOperand1.Mantissa = ((SingleValue &
                                            0x7fffff) | 0x800000) << 2;

                return KiNormalizeSingle(&ContextBlock,
                                         &SingleOperand1,
                                         StickyBits);

            } else if (Format == FORMAT_DOUBLE) {

                //
                // If the operand is plus infinity, then the result is
                // plus infinity, or if the operand is plus or minus
                // zero, then the result is plus or minus zero.
                //

                if (((DoubleOperand1.Sign == 0) &&
                   (DoubleOperand1.Infinity != FALSE)) ||
                   (DoubleOperand1.MantissaHigh == 0)) {
                    return KiNormalizeDouble(&ContextBlock,
                                             &DoubleOperand1,
                                             0);
                }

                //
                // If the operand is negative, then the operation is
                // invalid.
                //

                if (DoubleOperand1.Sign != 0) {
                    return KiInvalidOperationDouble(&ContextBlock,
                                                    FALSE,
                                                    &DoubleOperand1,
                                                    &DoubleOperand1);
                }

                //
                // The only case remaining that could cause an exception
                // is a denomalized source value. The square root of a
                // denormalized value is computed by:
                //
                //   1. Converting the value to a normalized value with
                //      an exponent equal to the denormalization shift count
                //      plus the bias of the exponent plus one.
                //
                //   2. Computing the square root of the value and unpacking
                //      the result.
                //
                //   3. Converting the shift count back to a normalization
                //      shift count.
                //
                //   4. Rounding and packing the resultant value.
                //
                // N.B. The square root of all denormalized numbers is a
                //      normalized number.
                //

                DoubleOperand1.Exponent = (DOUBLE_EXPONENT_BIAS + 1 +
                                            DoubleOperand1.Exponent) << 20;

                DoubleValue.HighPart = (DoubleOperand1.MantissaHigh & ~(1 << 22)) >> 2;
                DoubleValue.LowPart = (DoubleOperand1.MantissaHigh << 30) |
                                            (DoubleOperand1.MantissaLow >> 2);

                DoubleValue.HighPart |= DoubleOperand1.Exponent;
                StickyBits = KiSquareRootDouble(&DoubleValue);
                DoubleOperand1.Exponent =  (DoubleValue.HighPart >> 20) -
                                            ((DOUBLE_EXPONENT_BIAS + 1) / 2);

                DoubleOperand1.MantissaLow = DoubleValue.LowPart << 2;
                DoubleOperand1.MantissaHigh = ((DoubleValue.HighPart &
                                            0xfffff) | 0x100000) << 2;

                DoubleOperand1.MantissaHigh |= (DoubleValue.LowPart >> 30);
                return KiNormalizeDouble(&ContextBlock,
                                         &DoubleOperand1,
                                         StickyBits);

            } else {
                break;
            }

            //
            // Floating absolute operation.
            //
            // Floating absolute is accomplished by clearing the sign
            // of the floating value.
            //

        case FLOAT_ABSOLUTE:
            if (Format == FORMAT_SINGLE) {

                //
                // Clear the sign, normalize the result, and store in the
                // destination register.
                //

                SingleOperand1.Sign = 0;
                return KiNormalizeSingle(&ContextBlock,
                                         &SingleOperand1,
                                         0);

            } else if (Format == FORMAT_DOUBLE) {

                //
                // Clear the sign, normalize the result, and store in the
                // destination register.
                //

                DoubleOperand1.Sign = 0;
                return KiNormalizeDouble(&ContextBlock,
                                         &DoubleOperand1,
                                         0);

            } else {
                break;
            }

            //
            // Floating move operation.
            //
            // Floating move is accomplished by moving the source operand
            // to the destination register.
            //

        case FLOAT_MOVE:
            if (Format == FORMAT_SINGLE) {

                //
                // Normalize the result and store in the destination
                // register.
                //

                return KiNormalizeSingle(&ContextBlock,
                                         &SingleOperand1,
                                         0);

            } else if (Format == FORMAT_DOUBLE) {

                //
                // Normalize the result and store in the destination
                // register.
                //

                return KiNormalizeDouble(&ContextBlock,
                                         &DoubleOperand1,
                                         0);

            } else {
                break;
            }

            //
            // Floating negate operation.
            //
            // Floating absolute is accomplished by complementing the sign
            // of the floating value.
            //

        case FLOAT_NEGATE:
            if (Format == FORMAT_SINGLE) {

                //
                // Complement the sign, normalize the result, and store in the
                // destination register.
                //

                SingleOperand1.Sign ^= 0x1;
                return KiNormalizeSingle(&ContextBlock,
                                         &SingleOperand1,
                                         0);

            } else if (Format == FORMAT_DOUBLE) {

                //
                // Complement the sign, normalize the result, and store in the
                // destination register.
                //

                DoubleOperand1.Sign ^= 0x1;
                return KiNormalizeDouble(&ContextBlock,
                                         &DoubleOperand1,
                                         0);

            } else {
                break;
            }

            //
            // Floating compare single.
            //
            // This operation is performed after having separated out NaNs,
            // and therefore the only comparison predicates left are equal
            // and less.
            //
            // Floating compare single is accomplished by comparing signs,
            // then exponents, and finally the mantissa if necessary.
            //
            // N.B. The sign of zero is ignorned.
            //

        case FLOAT_COMPARE_SINGLE:

            //
            // If either operand is zero, then set the sign of the operand
            // positive.
            //

            if ((SingleOperand1.Infinity == FALSE) &&
                (SingleOperand1.Mantissa == 0)) {
                SingleOperand1.Sign = 0;
                SingleOperand1.Exponent = - 23;
            }

            if ((SingleOperand2.Infinity == FALSE)  &&
                (SingleOperand2.Mantissa == 0)) {
                SingleOperand2.Sign = 0;
                SingleOperand2.Exponent = - 23;
            }

            //
            // Compare signs first.
            //

            if (SingleOperand1.Sign < SingleOperand2.Sign) {

                //
                // The first operand is greater than the second operand.
                //

                CompareEqual = FALSE;
                CompareLess = FALSE;

            } else if (SingleOperand1.Sign > SingleOperand2.Sign) {

                //
                // The first operand is less than the second operand.
                //

                CompareEqual = FALSE;
                CompareLess = TRUE;

            } else {

                //
                // The operand signs are equal.
                //
                // If the sign of the operand is negative, then the sense of
                // the comparison is reversed.
                //

                if (SingleOperand1.Sign == 0) {

                    //
                    // Compare positive operand with positive operand.
                    //

                    if (SingleOperand1.Exponent > SingleOperand2.Exponent) {
                        CompareEqual = FALSE;
                        CompareLess = FALSE;

                    } else if (SingleOperand1.Exponent < SingleOperand2.Exponent) {
                        CompareEqual = FALSE;
                        CompareLess = TRUE;

                    } else {
                        if (SingleOperand1.Mantissa > SingleOperand2.Mantissa) {
                            CompareEqual = FALSE;
                            CompareLess = FALSE;

                        } else if (SingleOperand1.Mantissa < SingleOperand2.Mantissa) {
                            CompareEqual = FALSE;
                            CompareLess = TRUE;

                        } else {
                            CompareEqual = TRUE;
                            CompareLess = FALSE;
                        }
                    }

                } else {

                    //
                    // Compare negative operand with negative operand.
                    //

                    if (SingleOperand2.Exponent > SingleOperand1.Exponent) {
                        CompareEqual = FALSE;
                        CompareLess = FALSE;

                    } else if (SingleOperand2.Exponent < SingleOperand1.Exponent) {
                        CompareEqual = FALSE;
                        CompareLess = TRUE;

                    } else {
                        if (SingleOperand2.Mantissa > SingleOperand1.Mantissa) {
                            CompareEqual = FALSE;
                            CompareLess = FALSE;

                        } else if (SingleOperand2.Mantissa < SingleOperand1.Mantissa) {
                            CompareEqual = FALSE;
                            CompareLess = TRUE;

                        } else {
                            CompareEqual = TRUE;
                            CompareLess = FALSE;
                        }
                    }
                }
            }

            //
            // Form the condition code using the comparison information
            // and the compare function predicate bits.
            //

            if (((CompareLess != FALSE) &&
                ((CompareFunction & COMPARE_LESS_MASK) != 0)) ||
                ((CompareEqual != FALSE) &&
                ((CompareFunction & COMPARE_EQUAL_MASK) != 0))) {
                ((PFSR)&TrapFrame->Fsr)->CC = 1;

            } else {
                ((PFSR)&TrapFrame->Fsr)->CC = 0;
            }

            TrapFrame->Fir = ContextBlock.BranchAddress;
            return TRUE;

            //
            // Floating compare double.
            //
            // This operation is performed after having separated out NaNs,
            // and therefore the only comparison predicates left are equal
            // and less.
            //
            // Floating compare double is accomplished by comparing signs,
            // then exponents, and finally the mantissa if necessary.
            //
            // N.B. The sign of zero is ignorned.
            //

        case FLOAT_COMPARE_DOUBLE:

            //
            // If either operand is zero, then set the sign of the operand
            // positive.
            //

            if ((DoubleOperand1.Infinity == FALSE) &&
                (DoubleOperand1.MantissaHigh == 0)) {
                DoubleOperand1.Sign = 0;
                DoubleOperand1.Exponent = - 52;
            }

            if ((DoubleOperand2.Infinity == FALSE) &&
                (DoubleOperand2.MantissaHigh == 0)) {
                DoubleOperand2.Sign = 0;
                DoubleOperand2.Exponent = - 52;
            }

            //
            // Compare signs first.
            //

            if (DoubleOperand1.Sign < DoubleOperand2.Sign) {

                //
                // The first operand is greater than the second operand.
                //

                CompareEqual = FALSE;
                CompareLess = FALSE;

            } else if (DoubleOperand1.Sign > DoubleOperand2.Sign) {

                //
                // The first operand is less than the second operand.
                //

                CompareEqual = FALSE;
                CompareLess = TRUE;

            } else {

                //
                // The operand signs are equal.
                //
                // If the sign of the operand is negative, then the sense of
                // the comparison is reversed.
                //

                if (DoubleOperand1.Sign == 0) {

                    //
                    // Compare positive operand with positive operand.
                    //

                    if (DoubleOperand1.Exponent > DoubleOperand2.Exponent) {
                        CompareEqual = FALSE;
                        CompareLess = FALSE;

                    } else if (DoubleOperand1.Exponent < DoubleOperand2.Exponent) {
                        CompareEqual = FALSE;
                        CompareLess = TRUE;

                    } else {
                        if (DoubleOperand1.MantissaHigh >
                            DoubleOperand2.MantissaHigh) {
                            CompareEqual = FALSE;
                            CompareLess = FALSE;

                        } else if (DoubleOperand1.MantissaHigh <
                                   DoubleOperand2.MantissaHigh) {
                            CompareEqual = FALSE;
                            CompareLess = TRUE;

                        } else {
                            if (DoubleOperand1.MantissaLow >
                                DoubleOperand2.MantissaLow) {
                                CompareEqual = FALSE;
                                CompareLess = FALSE;

                            } else if (DoubleOperand1.MantissaLow <
                                       DoubleOperand2.MantissaLow) {
                                CompareEqual = FALSE;
                                CompareLess = TRUE;

                            } else {
                                CompareEqual = TRUE;
                                CompareLess = FALSE;
                            }
                        }
                    }

                } else {

                    //
                    // Compare negative operand with negative operand.
                    //

                    if (DoubleOperand2.Exponent > DoubleOperand1.Exponent) {
                        CompareEqual = FALSE;
                        CompareLess = FALSE;

                    } else if (DoubleOperand2.Exponent < DoubleOperand1.Exponent) {
                        CompareEqual = FALSE;
                        CompareLess = TRUE;

                    } else {
                        if (DoubleOperand2.MantissaHigh >
                            DoubleOperand1.MantissaHigh) {
                            CompareEqual = FALSE;
                            CompareLess = FALSE;

                        } else if (DoubleOperand2.MantissaHigh <
                                   DoubleOperand1.MantissaHigh) {
                            CompareEqual = FALSE;
                            CompareLess = TRUE;

                        } else {
                            if (DoubleOperand2.MantissaLow >
                                DoubleOperand1.MantissaLow) {
                                CompareEqual = FALSE;
                                CompareLess = FALSE;

                            } else if (DoubleOperand2.MantissaLow <
                                       DoubleOperand1.MantissaLow) {
                                CompareEqual = FALSE;
                                CompareLess = TRUE;

                            } else {
                                CompareEqual = TRUE;
                                CompareLess = FALSE;
                            }
                        }
                    }
                }
            }

            //
            // Form the condition code using the comparison information
            // and the compare function predicate bits.
            //

            if (((CompareLess != FALSE) &&
                ((CompareFunction & COMPARE_LESS_MASK) != 0)) ||
                ((CompareEqual != FALSE) &&
                ((CompareFunction & COMPARE_EQUAL_MASK) != 0))) {
                ((PFSR)&TrapFrame->Fsr)->CC = 1;

            } else {
                ((PFSR)&TrapFrame->Fsr)->CC = 0;
            }

            TrapFrame->Fir = ContextBlock.BranchAddress;
            return TRUE;

            //
            // Floating convert to single.
            //
            // This operation is only legal for conversion from quadword,
            // longword, and double formats to single format. This operation
            // can not be used to convert from a single format to a single format.
            //
            // Floating conversion to single is accompished by forming a
            // single floating operand and then normalize and storing the
            // result value.
            //

        case FLOAT_CONVERT_SINGLE:
            if (Format == FORMAT_SINGLE) {
                break;

            } else if (Format == FORMAT_DOUBLE) {

                //
                // If the operand is a NaN, then store a quiet NaN if the
                // invalid operation trap is disabled, or raise an exception
                // if the invalid operation trap is enabled and the operand
                // is a signaling NaN.
                //

                if (DoubleOperand1.Nan != FALSE) {
                    SingleOperand1.Mantissa =
                        (DoubleOperand1.MantissaHigh << (26 - (55 - 32))) |
                        (DoubleOperand1.MantissaLow >> (32 - (26 - (55 - 32))));
                    SingleOperand1.Exponent = SINGLE_MAXIMUM_EXPONENT;
                    SingleOperand1.Sign = DoubleOperand1.Sign;
                    SingleOperand1.Infinity = FALSE;
                    SingleOperand1.Nan = TRUE;
                    return KiInvalidOperationSingle(&ContextBlock,
                                                    TRUE,
                                                    &SingleOperand1,
                                                    &SingleOperand1);

                }

                //
                // Transform the double operand to single format.
                //

                SingleOperand1.Mantissa =
                    (DoubleOperand1.MantissaHigh << (26 - (55 - 32))) |
                    (DoubleOperand1.MantissaLow >> (32 - (26 - (55 - 32))));
                StickyBits = DoubleOperand1.MantissaLow << (26 - (55 - 32));
                SingleOperand1.Exponent = DoubleOperand1.Exponent +
                                    SINGLE_EXPONENT_BIAS - DOUBLE_EXPONENT_BIAS;
                SingleOperand1.Sign = DoubleOperand1.Sign;
                SingleOperand1.Infinity = DoubleOperand1.Infinity;
                SingleOperand1.Nan = FALSE;

                //
                // Normalize and store the result value.
                //

                return KiNormalizeSingle(&ContextBlock,
                                         &SingleOperand1,
                                         StickyBits);

            } else if (Format == FORMAT_LONGWORD) {

                //
                // Compute the sign of the result.
                //

                if (Longword < 0) {
                    SingleOperand1.Sign = 0x1;
                    Longword = -Longword;

                } else {
                    SingleOperand1.Sign = 0;
                }

                //
                // Initialize the infinity and NaN values.
                //

                SingleOperand1.Infinity = FALSE;
                SingleOperand1.Nan = FALSE;

                //
                // Compute the exponent value and normalize the longword
                // value.
                //

                if (Longword != 0) {
                    SingleOperand1.Exponent = SINGLE_EXPONENT_BIAS + 31;
                    while (Longword > 0) {
                        Longword <<= 1;
                        SingleOperand1.Exponent -= 1;
                    }

                    SingleOperand1.Mantissa = (ULONG)Longword >> (32 - 26);
                    StickyBits = Longword << 26;

                } else {
                    SingleOperand1.Mantissa = 0;
                    StickyBits = 0;
                    SingleOperand1.Exponent = 0;
                }

                //
                // Normalize and store the result value.
                //

                return KiNormalizeSingle(&ContextBlock,
                                         &SingleOperand1,
                                         StickyBits);

            } else if (Format == FORMAT_QUADWORD) {

                //
                // Compute the sign of the result.
                //

                if (u.Quadword < 0) {
                    SingleOperand1.Sign = 0x1;
                    u.Quadword = -u.Quadword;

                } else {
                    SingleOperand1.Sign = 0;
                }

                //
                // Initialize the infinity and NaN values.
                //

                SingleOperand1.Infinity = FALSE;
                SingleOperand1.Nan = FALSE;

                //
                // Compute the exponent value and normalize the quadword
                // value.
                //

                if (u.Quadword != 0) {
                    SingleOperand1.Exponent = SINGLE_EXPONENT_BIAS + 63;
                    while (u.Quadword > 0) {
                        u.Quadword <<= 1;
                        SingleOperand1.Exponent -= 1;
                    }

                    SingleOperand1.Mantissa = (LONG)((ULONGLONG)u.Quadword >> (64 - 26));
                    StickyBits = (u.Quadword << 26) ? 1 : 0;

                } else {
                    SingleOperand1.Mantissa = 0;
                    StickyBits = 0;
                    SingleOperand1.Exponent = 0;
                }

                //
                // Normalize and store the result value.
                //

                return KiNormalizeSingle(&ContextBlock,
                                         &SingleOperand1,
                                         StickyBits);

            } else {
                break;
            }

            //
            // Floating convert to double.
            //
            // This operation is only legal for conversion from quadword,
            // longword, and single formats to double format. This operation
            // cannot be used to convert from a double format to a double
            // format.
            //
            // Floating conversion to double is accomplished by forming
            // double floating operand and then normalizing and storing
            // the result value.
            //

        case FLOAT_CONVERT_DOUBLE:
            if (Format == FORMAT_SINGLE) {

                //
                // If the operand is a NaN, then store a quiet NaN if the
                // invalid operation trap is disabled, or raise an exception
                // if the invalid operation trap is enabled and the operand
                // is a signaling NaN.
                //

                if (SingleOperand1.Nan != FALSE) {
                    DoubleOperand1.MantissaHigh =
                            SingleOperand1.Mantissa >> (26 - (55 - 32));
                    DoubleOperand1.MantissaLow = (0xffffffff >> (26 - 2 - (55 - 32))) |
                            SingleOperand1.Mantissa << (32 - (26 - (55 - 32)));
                    DoubleOperand1.Exponent = DOUBLE_MAXIMUM_EXPONENT;
                    DoubleOperand1.Sign = SingleOperand1.Sign;
                    DoubleOperand1.Infinity = FALSE;
                    DoubleOperand1.Nan = TRUE;
                    return KiInvalidOperationDouble(&ContextBlock,
                                                    TRUE,
                                                    &DoubleOperand1,
                                                    &DoubleOperand1);

                }

                //
                // Transform the single operand to double format.
                //

                DoubleOperand1.MantissaHigh =
                            SingleOperand1.Mantissa >> (26 - (55 - 32));
                DoubleOperand1.MantissaLow =
                            SingleOperand1.Mantissa << (32 - (26 - (55 - 32)));
                DoubleOperand1.Exponent = SingleOperand1.Exponent +
                                    DOUBLE_EXPONENT_BIAS - SINGLE_EXPONENT_BIAS;
                DoubleOperand1.Sign = SingleOperand1.Sign;
                DoubleOperand1.Infinity = SingleOperand1.Infinity;
                DoubleOperand1.Nan = FALSE;

                //
                // Normalize and store the result value.
                //

                return KiNormalizeDouble(&ContextBlock,
                                         &DoubleOperand1,
                                         0);

            } else if (Format == FORMAT_DOUBLE) {
                break;

            } else if (Format == FORMAT_LONGWORD) {

                //
                // Compute the sign of the result.
                //

                if (Longword < 0) {
                    DoubleOperand1.Sign = 0x1;
                    Longword = -Longword;

                } else {
                    DoubleOperand1.Sign = 0;
                }

                //
                // Initialize the infinity and NaN values.
                //

                DoubleOperand1.Infinity = FALSE;
                DoubleOperand1.Nan = FALSE;

                //
                // Compute the exponent value and normalize the longword
                // value.
                //

                if (Longword != 0) {
                    SingleOperand1.Exponent = DOUBLE_EXPONENT_BIAS + 31;
                    while (Longword > 0) {
                        Longword <<= 1;
                        DoubleOperand1.Exponent -= 1;
                    }

                    DoubleOperand1.Mantissa = (ULONGLONG)Longword >> (64 - 55);

                } else {
                    DoubleOperand1.Mantissa = 0;
                    DoubleOperand1.Exponent = 0;
                }

                //
                // Normalize and store the result value.
                //

                return KiNormalizeDouble(&ContextBlock,
                                         &DoubleOperand1,
                                         0);

            } else if (Format == FORMAT_QUADWORD) {

                //
                // Compute the sign of the result.
                //

                if (u.Quadword < 0) {
                    DoubleOperand1.Sign = 0x1;
                    u.Quadword = -u.Quadword;

                } else {
                    DoubleOperand1.Sign = 0;
                }

                //
                // Initialize the infinity and NaN values.
                //

                DoubleOperand1.Infinity = FALSE;
                DoubleOperand1.Nan = FALSE;

                //
                // Compute the exponent value and normalize the quadword
                // value.
                //

                if (u.Quadword != 0) {
                    DoubleOperand1.Exponent = DOUBLE_EXPONENT_BIAS + 63;
                    while (u.Quadword > 0) {
                        u.Quadword <<= 1;
                        DoubleOperand1.Exponent -= 1;
                    }

                    DoubleOperand1.Mantissa = (ULONGLONG)u.Quadword >> (64 - 55);
                    StickyBits = (u.Quadword << 55) ? 1 : 0;

                } else {
                    DoubleOperand1.Mantissa = 0;
                    StickyBits = 0;
                    DoubleOperand1.Exponent = 0;
                }

                //
                // Normalize and store the result value.
                //

                return KiNormalizeDouble(&ContextBlock,
                                         &DoubleOperand1,
                                         StickyBits);

            } else {
                break;
            }

            //
            // Floating convert to quadword.
            //
            // This operation is only legal for conversion from double
            // and single formats to quadword format. This operation
            // cannot be used to convert from a quadword format to a
            // longword or quadword format.
            //
            // Floating conversion to quadword is accomplished by forming
            // a quadword value from a single or double floating value.
            //
            // There is one general conversion operation and four directed
            // rounding operations.
            //

        case FLOAT_ROUND_QUADWORD:
            ContextBlock.Round = ROUND_TO_NEAREST;
            goto ConvertQuadword;

        case FLOAT_TRUNC_QUADWORD:
            ContextBlock.Round = ROUND_TO_ZERO;
            goto ConvertQuadword;

        case FLOAT_CEIL_QUADWORD:
            ContextBlock.Round = ROUND_TO_PLUS_INFINITY;
            goto ConvertQuadword;

        case FLOAT_FLOOR_QUADWORD:
            ContextBlock.Round = ROUND_TO_MINUS_INFINITY;
            goto ConvertQuadword;

        case FLOAT_CONVERT_QUADWORD:
        ConvertQuadword:
            if (Format == FORMAT_SINGLE) {

                //
                // If the operand is infinite or is a NaN, then store a
                // quiet NaN or an appropriate infinity if the invalid
                // operation trap is disabled, or raise an exception if
                // the invalid trap is enabled.
                //

                if ((SingleOperand1.Infinity != FALSE) ||
                    (SingleOperand1.Nan != FALSE)) {
                    return KiInvalidOperationQuadword(&ContextBlock,
                                                      SingleOperand1.Infinity,
                                                      SingleOperand1.Sign);
                }

                //
                // Transform the single operand to double format.
                //

                DoubleOperand1.Mantissa = (LONGLONG)SingleOperand1.Mantissa << (55 - 26);
                DoubleOperand1.Exponent = SingleOperand1.Exponent +
                                    DOUBLE_EXPONENT_BIAS - SINGLE_EXPONENT_BIAS;

                DoubleOperand1.Sign = SingleOperand1.Sign;
                DoubleOperand1.Infinity = FALSE;
                DoubleOperand1.Nan = FALSE;

                //
                // Convert double to quadword and store the result value.
                //

                return KiNormalizeQuadword(&ContextBlock, &DoubleOperand1);

            } else if (Format == FORMAT_DOUBLE) {

                //
                // If the operand is infinite or is a NaN, then store a
                // quiet NaN or an appropriate infinity if the invalid
                // operation trap is disabled, or raise an exception if
                // the invalid trap is enabled.
                //

                if ((DoubleOperand1.Infinity != FALSE) ||
                    (DoubleOperand1.Nan != FALSE)) {
                    return KiInvalidOperationQuadword(&ContextBlock,
                                                      DoubleOperand1.Infinity,
                                                      DoubleOperand1.Sign);
                }

                //
                // Convert double to quadword and store the result value.
                //

                return KiNormalizeQuadword(&ContextBlock, &DoubleOperand1);

            } else {
                break;
            }

            //
            // Floating convert to longword.
            //
            // This operation is only legal for conversion from double
            // and single formats to longword format. This operation
            // cannot be used to convert from a longword format to a
            // longword format.
            //
            // Floating conversion to longword is accomplished by forming
            // a longword value from a single or double floating value.
            //
            // There is one general conversion operation and four directed
            // rounding operations.
            //

        case FLOAT_ROUND_LONGWORD:
            ContextBlock.Round = ROUND_TO_NEAREST;
            goto ConvertLongword;

        case FLOAT_TRUNC_LONGWORD:
            ContextBlock.Round = ROUND_TO_ZERO;
            goto ConvertLongword;

        case FLOAT_CEIL_LONGWORD:
            ContextBlock.Round = ROUND_TO_PLUS_INFINITY;
            goto ConvertLongword;

        case FLOAT_FLOOR_LONGWORD:
            ContextBlock.Round = ROUND_TO_MINUS_INFINITY;
            goto ConvertLongword;

        case FLOAT_CONVERT_LONGWORD:
        ConvertLongword:
            if (Format == FORMAT_SINGLE) {

                //
                // If the operand is infinite or is a NaN, then store a
                // quiet NaN or an appropriate infinity if the invalid
                // operation trap is disabled, or raise an exception if
                // the invalid trap is enabled.
                //

                if ((SingleOperand1.Infinity != FALSE) ||
                    (SingleOperand1.Nan != FALSE)) {
                    return KiInvalidOperationLongword(&ContextBlock,
                                                      SingleOperand1.Infinity,
                                                      SingleOperand1.Sign);
                }

                //
                // Transform the single operand to double format.
                //

                DoubleOperand1.MantissaHigh =
                            SingleOperand1.Mantissa >> (26 - (55 - 32));
                DoubleOperand1.MantissaLow =
                            SingleOperand1.Mantissa << (32 - (26 - (55 - 32)));
                DoubleOperand1.Exponent = SingleOperand1.Exponent +
                                    DOUBLE_EXPONENT_BIAS - SINGLE_EXPONENT_BIAS;
                DoubleOperand1.Sign = SingleOperand1.Sign;
                DoubleOperand1.Infinity = FALSE;
                DoubleOperand1.Nan = FALSE;

                //
                // Convert double to longword and store the result value.
                //

                return KiNormalizeLongword(&ContextBlock, &DoubleOperand1);

            } else if (Format == FORMAT_DOUBLE) {

                //
                // If the operand is infinite or is a NaN, then store a
                // quiet NaN or an appropriate infinity if the invalid
                // operation trap is disabled, or raise an exception if
                // the invalid trap is enabled.
                //

                if ((DoubleOperand1.Infinity != FALSE) ||
                    (DoubleOperand1.Nan != FALSE)) {
                    return KiInvalidOperationLongword(&ContextBlock,
                                                      DoubleOperand1.Infinity,
                                                      DoubleOperand1.Sign);
                }

                //
                // Convert double to longword and store the result value.
                //

                return KiNormalizeLongword(&ContextBlock, &DoubleOperand1);

            } else {
                break;
            }

            //
            // An illegal function, format value, or field value.
            //

        default :
            break;
        }

        //
        // An illegal function, format value, or field value was encoutnered.
        // Generate and illegal instruction exception.
        //

        ExceptionRecord->ExceptionCode = STATUS_ILLEGAL_INSTRUCTION;
        return FALSE;

    //
    // If an exception occurs, then copy the new exception information to the
    // original exception record and handle the exception.
    //

    } except (KiCopyInformation(ExceptionRecord,
                               (GetExceptionInformation())->ExceptionRecord)) {

        //
        // Preserve the original exception address and branch destination.
        //

        ExceptionRecord->ExceptionAddress = ExceptionAddress;
        return FALSE;
    }
}

BOOLEAN
KiDivideByZeroDouble (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN PFP_DOUBLE_OPERAND DoubleOperand1,
    IN PFP_DOUBLE_OPERAND DoubleOperand2
    )

/*++

Routine Description:

    This function is called to either raise an exception or store a
    quiet NaN or properly signed infinity for a divide by zero double
    floating operation.

Arguments:

    ContextBlock - Supplies a pointer to the emulation context block.

    DoubleOperand1 - Supplies a pointer to the first operand value.

    DoubleOperand2 - Supplies a pointer ot the second operand value.

Return Value:

    If the divide by zero trap is enabled and the dividend is not infinite,
    then a value of FALSE is returned. Otherwise, a quite NaN or a properly
    signed infinity is stored as the destination result and a value of TRUE
    is returned.

--*/

{

    PEXCEPTION_RECORD ExceptionRecord;
    PFP_IEEE_VALUE IeeeValue;
    ULONG ResultSign;
    ULONG ResultValueHigh;
    ULONG ResultValueLow;
    PKTRAP_FRAME TrapFrame;

    //
    // The result value is a properly signed infinity.
    //

    ResultSign = DoubleOperand1->Sign ^ DoubleOperand2->Sign;
    ResultValueHigh = DOUBLE_INFINITY_VALUE_HIGH | (ResultSign << 31);
    ResultValueLow = DOUBLE_INFINITY_VALUE_LOW;

    //
    // If the first operand is not infinite and the divide by zero trap is
    // enabled, then store the proper exception code and exception flags
    // and return a value of FALSE. Otherwise, store the appropriatly signed
    // infinity and return a value of TRUE.
    //

    ExceptionRecord = ContextBlock->ExceptionRecord;
    TrapFrame = ContextBlock->TrapFrame;
    if (DoubleOperand1->Infinity == FALSE) {
        ((PFSR)&TrapFrame->Fsr)->SZ = 1;
        if (((PFSR)&TrapFrame->Fsr)->EZ != 0) {
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_DIVIDE_BY_ZERO;
            ((PFSR)&TrapFrame->Fsr)->XZ = 1;
            IeeeValue = (PFP_IEEE_VALUE)&ExceptionRecord->ExceptionInformation[2];
            IeeeValue->Value.Fp64Value.W[0] = ResultValueLow;
            IeeeValue->Value.Fp64Value.W[1] = ResultValueHigh;
            return FALSE;
        }
    }

    KiSetRegisterValue(ContextBlock->Fd + 32,
                       ResultValueLow,
                       ContextBlock->ExceptionFrame,
                       ContextBlock->TrapFrame);

    KiSetRegisterValue(ContextBlock->Fd + 32 + 1,
                       ResultValueHigh,
                       ContextBlock->ExceptionFrame,
                       ContextBlock->TrapFrame);

    TrapFrame->Fir = ContextBlock->BranchAddress;
    return TRUE;
}

BOOLEAN
KiDivideByZeroSingle (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN PFP_SINGLE_OPERAND SingleOperand1,
    IN PFP_SINGLE_OPERAND SingleOperand2
    )

/*++

Routine Description:

    This function is called to either raise an exception or store a
    quiet NaN or properly signed infinity for a divide by zero single
    floating operation.

Arguments:

    ContextBlock - Supplies a pointer to the emulation context block.

    SingleOperand1 - Supplies a pointer to the first operand value.

    SingleOperand2 - Supplies a pointer ot the second operand value.

Return Value:

    If the divide by zero trap is enabled and the dividend is not infinite,
    then a value of FALSE is returned. Otherwise, a quite NaN is or properly
    signed infinity is stored as the destination result and a value of TRUE
    is returned.

--*/

{

    PEXCEPTION_RECORD ExceptionRecord;
    PFP_IEEE_VALUE IeeeValue;
    ULONG ResultSign;
    ULONG ResultValue;
    PKTRAP_FRAME TrapFrame;

    //
    // The result value is a properly signed infinity.
    //

    ResultSign = SingleOperand1->Sign ^ SingleOperand2->Sign;
    ResultValue = SINGLE_INFINITY_VALUE | (ResultSign << 31);

    //
    // If the first operand is not infinite and the divide by zero trap is
    // enabled, then store the proper exception code and exception flags
    // and return a value of FALSE. Otherwise, store the appropriatly signed
    // infinity and return a value of TRUE.
    //
    //

    ExceptionRecord = ContextBlock->ExceptionRecord;
    TrapFrame = ContextBlock->TrapFrame;
    if (SingleOperand1->Infinity == FALSE) {
        ((PFSR)&TrapFrame->Fsr)->SZ = 1;
        if (((PFSR)&TrapFrame->Fsr)->EZ != 0) {
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_DIVIDE_BY_ZERO;
            ((PFSR)&TrapFrame->Fsr)->XZ = 1;
            IeeeValue = (PFP_IEEE_VALUE)&ExceptionRecord->ExceptionInformation[2];
            IeeeValue->Value.Fp32Value.W[0] = ResultValue;
            return FALSE;
        }
    }

    KiSetRegisterValue(ContextBlock->Fd + 32,
                       ResultValue,
                       ContextBlock->ExceptionFrame,
                       ContextBlock->TrapFrame);

    TrapFrame->Fir = ContextBlock->BranchAddress;
    return TRUE;
}

BOOLEAN
KiInvalidCompareDouble (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN BOOLEAN CheckForNan,
    IN PFP_DOUBLE_OPERAND DoubleOperand1,
    IN PFP_DOUBLE_OPERAND DoubleOperand2
    )

/*++

Routine Description:

    This function is called to determine whether an invalid operation
    exception should be raised for a double compare operation.

Arguments:

    ContextBlock - Supplies a pointer to the emulation context block.

    CheckForNan - Supplies a boolean value that detetermines whether the
        operand values should be checked for a signaling NaN.

    DoubleOperand1 - Supplies a pointer to the first operand value.

    DoubleOperand2 - Supplies a pointer ot the second operand value.

Return Value:

    If the invalid operation trap is enabled and either the operation is
    invalid or one of the operands in a signaling NaN, then a value of
    FALSE is returned. Otherwise, no operation is performed and a value
    of TRUE is returned.

--*/

{

    PEXCEPTION_RECORD ExceptionRecord;
    PFP_IEEE_VALUE IeeeValue;
    PKTRAP_FRAME TrapFrame;

    //
    // If an invalid operation is specified or one of the operands is a
    // signaling NaN and the invalid operation trap is enabled, then
    // store the proper exception code and exception flags and return
    // a value of FALSE. Otherwise, perform no operation and return a
    // value of TRUE.
    //

    ExceptionRecord = ContextBlock->ExceptionRecord;
    TrapFrame = ContextBlock->TrapFrame;
    if ((CheckForNan == FALSE) ||
        ((DoubleOperand1->Nan != FALSE) &&
        ((DoubleOperand1->MantissaHigh & DOUBLE_SIGNAL_NAN_MASK) != 0)) ||
        ((DoubleOperand2->Nan != FALSE) &&
        ((DoubleOperand2->MantissaHigh & DOUBLE_SIGNAL_NAN_MASK) != 0))) {
        ((PFSR)&TrapFrame->Fsr)->SV = 1;
        if (((PFSR)&TrapFrame->Fsr)->EV != 0) {
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
            ((PFSR)&TrapFrame->Fsr)->XV = 1;
            IeeeValue = (PFP_IEEE_VALUE)&ExceptionRecord->ExceptionInformation[2];
            IeeeValue->Value.CompareValue = FpCompareUnordered;
            return FALSE;
        }
    }

    TrapFrame->Fir = ContextBlock->BranchAddress;
    return TRUE;
}

BOOLEAN
KiInvalidCompareSingle (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN BOOLEAN CheckForNan,
    IN PFP_SINGLE_OPERAND SingleOperand1,
    IN PFP_SINGLE_OPERAND SingleOperand2
    )

/*++

Routine Description:

    This function is called to determine whether an invalid operation
    exception should be raised for a single compare operation.

Arguments:

    ContextBlock - Supplies a pointer to the emulation context block.

    CheckForNan - Supplies a boolean value that detetermines whether the
        operand values should be checked for a signaling NaN.

    SingleOperand1 - Supplies a pointer to the first operand value.

    SingleOperand2 - Supplies a pointer ot the second operand value.

Return Value:

    If the invalid operation trap is enabled and either the operation is
    invalid or one of the operands in a signaling NaN, then a value of
    FALSE is returned. Otherwise, no operation is performed and a value
    of TRUE is returned.

--*/

{

    PEXCEPTION_RECORD ExceptionRecord;
    PFP_IEEE_VALUE IeeeValue;
    PKTRAP_FRAME TrapFrame;

    //
    // If an invalid operation is specified or one of the operands is a
    // signaling NaN and the invalid operation trap is enabled, then
    // store the proper exception code and exception flags and return
    // a value of FALSE. Otherwise, perform no operation and return a
    // value of TRUE.
    //

    ExceptionRecord = ContextBlock->ExceptionRecord;
    TrapFrame = ContextBlock->TrapFrame;
    if ((CheckForNan == FALSE) ||
        ((SingleOperand1->Nan != FALSE) &&
        ((SingleOperand1->Mantissa & SINGLE_SIGNAL_NAN_MASK) != 0)) ||
        ((SingleOperand2->Nan != FALSE) &&
        ((SingleOperand2->Mantissa & SINGLE_SIGNAL_NAN_MASK) != 0))) {
        ((PFSR)&TrapFrame->Fsr)->SV = 1;
        if (((PFSR)&TrapFrame->Fsr)->EV != 0) {
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
            ((PFSR)&TrapFrame->Fsr)->XV = 1;
            IeeeValue = (PFP_IEEE_VALUE)&ExceptionRecord->ExceptionInformation[2];
            IeeeValue->Value.CompareValue = FpCompareUnordered;
            return FALSE;
        }
    }

    TrapFrame->Fir = ContextBlock->BranchAddress;
    return TRUE;
}

BOOLEAN
KiInvalidOperationDouble (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN BOOLEAN CheckForNan,
    IN PFP_DOUBLE_OPERAND DoubleOperand1,
    IN PFP_DOUBLE_OPERAND DoubleOperand2
    )

/*++

Routine Description:

    This function is called to either raise and exception or store a
    quiet NaN for an invalid double floating operation.

Arguments:

    ContextBlock - Supplies a pointer to the emulation context block.

    CheckForNan - Supplies a boolean value that detetermines whether the
        operand values should be checked for a signaling NaN.

    DoubleOperand1 - Supplies a pointer to the first operand value.

    DoubleOperand2 - Supplies a pointer ot the second operand value.

Return Value:

    If the invalid operation trap is enabled and either the operation is
    invalid or one of the operands in a signaling NaN, then a value of
    FALSE is returned. Otherwise, a quite NaN is stored as the destination
    result and a value of TRUE is returned.

--*/

{

    PEXCEPTION_RECORD ExceptionRecord;
    PFP_IEEE_VALUE IeeeValue;
    ULONG MantissaHigh;
    ULONG ResultValueHigh;
    ULONG ResultValueLow;
    PKTRAP_FRAME TrapFrame;

    //
    // If the first operand is a NaN, then compute a quite NaN from its
    // value. Otherwise, if the second operand is a NaN, then compute a
    // quiet NaN from its value. Otherwise, the result value is a quite
    // NaN.

    if (DoubleOperand1->Nan != FALSE) {
        MantissaHigh = DoubleOperand1->MantissaHigh & ~DOUBLE_SIGNAL_NAN_MASK;
        if ((DoubleOperand1->MantissaLow | MantissaHigh) != 0) {
            ResultValueLow = DoubleOperand1->MantissaLow >> 2;
            ResultValueLow |= DoubleOperand1->MantissaHigh << 30;
            ResultValueHigh = DoubleOperand1->MantissaHigh >> 2;
            ResultValueHigh |= DOUBLE_QUIET_NAN_PREFIX;
            ResultValueHigh &= ~DOUBLE_QUIET_NAN_MASK;

        } else {
            ResultValueLow = DOUBLE_NAN_LOW;
            ResultValueHigh = DOUBLE_QUIET_NAN;
        }

    } else if (DoubleOperand2->Nan != FALSE) {
        MantissaHigh = DoubleOperand2->MantissaHigh & ~DOUBLE_SIGNAL_NAN_MASK;
        if ((DoubleOperand2->MantissaLow | MantissaHigh) != 0) {
            ResultValueLow = DoubleOperand2->MantissaLow >> 2;
            ResultValueLow |= DoubleOperand2->MantissaHigh << 30;
            ResultValueHigh = DoubleOperand2->MantissaHigh >> 2;
            ResultValueHigh |= DOUBLE_QUIET_NAN_PREFIX;
            ResultValueHigh &= ~DOUBLE_QUIET_NAN_MASK;

        } else {
            ResultValueLow = DOUBLE_NAN_LOW;
            ResultValueHigh = DOUBLE_QUIET_NAN;
        }

    } else {
        ResultValueLow = DOUBLE_NAN_LOW;
        ResultValueHigh = DOUBLE_QUIET_NAN;
    }

    //
    // If an invalid operation is specified or one of the operands is a
    // signaling NaN and the invalid operation trap is enabled, then
    // store the proper exception code and exception flags and return
    // a value of FALSE. Otherwise, store a quiet NaN as the destination
    // result and return a value of TRUE.
    //

    ExceptionRecord = ContextBlock->ExceptionRecord;
    TrapFrame = ContextBlock->TrapFrame;
    if ((CheckForNan == FALSE) ||
        ((DoubleOperand1->Nan != FALSE) &&
        ((DoubleOperand1->MantissaHigh & DOUBLE_SIGNAL_NAN_MASK) != 0)) ||
        ((DoubleOperand2->Nan != FALSE) &&
        ((DoubleOperand2->MantissaHigh & DOUBLE_SIGNAL_NAN_MASK) != 0))) {
        ((PFSR)&TrapFrame->Fsr)->SV = 1;
        if (((PFSR)&TrapFrame->Fsr)->EV != 0) {
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
            ((PFSR)&TrapFrame->Fsr)->XV = 1;
            IeeeValue = (PFP_IEEE_VALUE)&ExceptionRecord->ExceptionInformation[2];
            IeeeValue->Value.Fp64Value.W[0] = ResultValueLow;
            IeeeValue->Value.Fp64Value.W[1] = ResultValueHigh;
            return FALSE;
        }
    }

    KiSetRegisterValue(ContextBlock->Fd + 32,
                       ResultValueLow,
                       ContextBlock->ExceptionFrame,
                       ContextBlock->TrapFrame);

    KiSetRegisterValue(ContextBlock->Fd + 32 + 1,
                       ResultValueHigh,
                       ContextBlock->ExceptionFrame,
                       ContextBlock->TrapFrame);

    TrapFrame->Fir = ContextBlock->BranchAddress;
    return TRUE;
}

BOOLEAN
KiInvalidOperationLongword (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN BOOLEAN Infinity,
    IN LONG Sign
    )

/*++

Routine Description:

    This function is called to either raise and exception or store a
    quiet NaN for an invalid conversion to longword.

Arguments:

    ContextBlock - Supplies a pointer to the emulation context block.

    Infinity - Suuplies a boolean variable that specifies whether the
        invalid operand is infinite.

    Sign - Supplies the infinity sign if the invalid operand is infinite.

Return Value:

    If the invalid operation trap is enabled, then a value of FALSE is
    returned. Otherwise, an appropriate longword value is stored as the
    destination result and a value of TRUE is returned.

--*/

{

    PEXCEPTION_RECORD ExceptionRecord;
    PFP_IEEE_VALUE IeeeValue;
    ULONG ResultValue;
    PKTRAP_FRAME TrapFrame;

    //
    // If the value is infinite, then the result is a properly signed value
    // whose magnitude is the largest that will fit in 32-bits. Otherwise,
    // the result is an integer NaN.
    //

    if (Infinity != FALSE) {
        if (Sign == 0) {
            ResultValue = 0x7fffffff;

        } else {
            ResultValue = 0x80000000;
        }

    } else {
        ResultValue = SINGLE_INTEGER_NAN;
    }

    //
    // If the invalid operation trap is enabled then store the proper
    // exception code and exception flags and return a value of FALSE.
    // Otherwise, store a quiet NaN as the destination result and return
    // a value of TRUE.
    //

    ExceptionRecord = ContextBlock->ExceptionRecord;
    TrapFrame = ContextBlock->TrapFrame;
    ((PFSR)&TrapFrame->Fsr)->SV = 1;
    if (((PFSR)&TrapFrame->Fsr)->EV != 0) {
        ExceptionRecord->ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
        ((PFSR)&TrapFrame->Fsr)->XV = 1;
        IeeeValue = (PFP_IEEE_VALUE)&ExceptionRecord->ExceptionInformation[2];
        IeeeValue->Value.U32Value = ResultValue;
        return FALSE;

    } else {

        KiSetRegisterValue(ContextBlock->Fd + 32,
                           ResultValue,
                           ContextBlock->ExceptionFrame,
                           ContextBlock->TrapFrame);

        TrapFrame->Fir = ContextBlock->BranchAddress;
        return TRUE;
    }
}

BOOLEAN
KiInvalidOperationQuadword (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN BOOLEAN Infinity,
    IN LONG Sign
    )

/*++

Routine Description:

    This function is called to either raise and exception or store a
    quiet NaN for an invalid conversion to quadword.

Arguments:

    ContextBlock - Supplies a pointer to the emulation context block.

    Infinity - Suuplies a boolean variable that specifies whether the
        invalid operand is infinite.

    Sign - Supplies the infinity sign if the invalid operand is infinite.

Return Value:

    If the invalid operation trap is enabled, then a value of FALSE is
    returned. Otherwise, an appropriate longword value is stored as the
    destination result and a value of TRUE is returned.

--*/

{

    PEXCEPTION_RECORD ExceptionRecord;
    PFP_IEEE_VALUE IeeeValue;
    union {
        ULONGLONG ResultValue;
        ULARGE_INTEGER LargeValue;
    }u;

    PKTRAP_FRAME TrapFrame;

    //
    // If the value is infinite, then the result is a properly signed value
    // whose magnitude is the largest that will fit in 64-bits. Otherwise,
    // the result is an integer NaN.
    //

    if (Infinity != FALSE) {
        if (Sign == 0) {
            u.ResultValue = 0x7fffffffffffffff;

        } else {
            u.ResultValue = 0x8000000000000000;
        }

    } else {
        u.ResultValue = DOUBLE_INTEGER_NAN;
    }

    //
    // If the invalid operation trap is enabled then store the proper
    // exception code and exception flags and return a value of FALSE.
    // Otherwise, store a quiet NaN as the destination result and return
    // a value of TRUE.
    //

    ExceptionRecord = ContextBlock->ExceptionRecord;
    TrapFrame = ContextBlock->TrapFrame;
    ((PFSR)&TrapFrame->Fsr)->SV = 1;
    if (((PFSR)&TrapFrame->Fsr)->EV != 0) {
        ExceptionRecord->ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
        ((PFSR)&TrapFrame->Fsr)->XV = 1;
        IeeeValue = (PFP_IEEE_VALUE)&ExceptionRecord->ExceptionInformation[2];
        IeeeValue->Value.U64Value.QuadPart = u.ResultValue;
        return FALSE;

    } else {

        KiSetRegisterValue(ContextBlock->Fd + 32,
                           u.LargeValue.LowPart,
                           ContextBlock->ExceptionFrame,
                           ContextBlock->TrapFrame);

        KiSetRegisterValue(ContextBlock->Fd + 33,
                           u.LargeValue.HighPart,
                           ContextBlock->ExceptionFrame,
                           ContextBlock->TrapFrame);

        TrapFrame->Fir = ContextBlock->BranchAddress;
        return TRUE;
    }
}

BOOLEAN
KiInvalidOperationSingle (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN BOOLEAN CheckForNan,
    IN PFP_SINGLE_OPERAND SingleOperand1,
    IN PFP_SINGLE_OPERAND SingleOperand2
    )

/*++

Routine Description:

    This function is called to either raise and exception or store a
    quiet NaN for an invalid single floating operation.

Arguments:

    ContextBlock - Supplies a pointer to the emulation context block.

    CheckForNan - Supplies a boolean value that detetermines whether the
        operand values should be checked for a signaling NaN.

    SingleOperand1 - Supplies a pointer to the first operand value.

    SingleOperand2 - Supplies a pointer ot the second operand value.

Return Value:

    If the invalid operation trap is enabled and either the operation is
    invalid or one of the operands in a signaling NaN, then a value of
    FALSE is returned. Otherwise, a quite NaN is stored as the destination
    result and a value of TRUE is returned.

--*/

{

    PEXCEPTION_RECORD ExceptionRecord;
    PFP_IEEE_VALUE IeeeValue;
    ULONG ResultValue;
    PKTRAP_FRAME TrapFrame;

    //
    // If the first operand is a NaN, then compute a quite NaN from its
    // value. Otherwise, if the second operand is a NaN, then compute a
    // quiet NaN from its value. Otherwise, the result value is a quite
    // NaN.

    if (SingleOperand1->Nan != FALSE) {
        if ((SingleOperand1->Mantissa  & ~SINGLE_SIGNAL_NAN_MASK) != 0) {
            ResultValue = SingleOperand1->Mantissa >> 2;
            ResultValue |= SINGLE_QUIET_NAN_PREFIX;
            ResultValue &= ~SINGLE_QUIET_NAN_MASK;

        } else {
            ResultValue = SINGLE_QUIET_NAN;
        }

    } else if (SingleOperand2->Nan != FALSE) {
        if ((SingleOperand2->Mantissa & ~SINGLE_SIGNAL_NAN_MASK) != 0) {
            ResultValue = SingleOperand2->Mantissa >> 2;
            ResultValue |= SINGLE_QUIET_NAN_PREFIX;
            ResultValue &= ~SINGLE_QUIET_NAN_MASK;

        } else {
            ResultValue = SINGLE_QUIET_NAN;
        }

    } else {
        ResultValue = SINGLE_QUIET_NAN;
    }

    //
    // If an invalid operation is specified or one of the operands is a
    // signaling NaN and the invalid operation trap is enabled, then
    // store the proper exception code and exception flags and return
    // a value of FALSE. Otherwise, store a quiet NaN as the destination
    // result and return a value of TRUE.
    //

    ExceptionRecord = ContextBlock->ExceptionRecord;
    TrapFrame = ContextBlock->TrapFrame;
    if ((CheckForNan == FALSE) ||
        ((SingleOperand1->Nan != FALSE) &&
        ((SingleOperand1->Mantissa & SINGLE_SIGNAL_NAN_MASK) != 0)) ||
        ((SingleOperand2->Nan != FALSE) &&
        ((SingleOperand2->Mantissa & SINGLE_SIGNAL_NAN_MASK) != 0))) {
        ((PFSR)&TrapFrame->Fsr)->SV = 1;
        if (((PFSR)&TrapFrame->Fsr)->EV != 0) {
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
            ((PFSR)&TrapFrame->Fsr)->XV = 1;
            IeeeValue = (PFP_IEEE_VALUE)&ExceptionRecord->ExceptionInformation[2];
            IeeeValue->Value.Fp32Value.W[0] = ResultValue;
            return FALSE;
        }
    }

    KiSetRegisterValue(ContextBlock->Fd + 32,
                       ResultValue,
                       ContextBlock->ExceptionFrame,
                       ContextBlock->TrapFrame);

    TrapFrame->Fir = ContextBlock->BranchAddress;
    return TRUE;
}

BOOLEAN
KiNormalizeDouble (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN PFP_DOUBLE_OPERAND ResultOperand,
    IN ULONG StickyBits
    )

/*++

Routine Description:

    This function is called to normalize a double floating result.

    N.B. The result value is specified with a guard bit on the right,
        the hidden bit (if appropriate), and a possible overflow bit.
        The result format is:

        <63:56> - zero
        <55> - overflow bit
        <54> - hidden bit
        <53:2> - mantissa
        <1> - guard bit
        <0> - round bit

        The sticky bits specify bits that were lost during the computable.

Arguments:

    ContextBlock - Supplies a pointer to the emulation context block.

    ResultOperand - Supplies a pointer to the result operand value.

    StickyBits - Supplies the value of the sticky bits.

Return Value:

    If there is not an exception, or the exception is handled, then a proper
    result is stored in the destination result, the continuation address is
    set, and a value of TRUE is returned. Otherwise, no value is stored and
    a value of FALSE is returned.

--*/

{

    ULONG DenormalizeShift;
    PEXCEPTION_RECORD ExceptionRecord;
    ULONG ExceptionResultHigh;
    ULONG ExceptionResultLow;
    PFP_IEEE_VALUE IeeeValue;
    BOOLEAN Inexact;
    BOOLEAN Overflow;
    ULONG ResultValueHigh;
    ULONG ResultValueLow;
    ULONG RoundBit;
    PKTRAP_FRAME TrapFrame;
    BOOLEAN Underflow;

    //
    // If the result is infinite, then store a properly signed infinity
    // in the destination register and return a value of TRUE. Otherwise,
    // round and normalize the result and check for overflow and underflow.
    //

    ExceptionRecord = ContextBlock->ExceptionRecord;
    TrapFrame = ContextBlock->TrapFrame;
    if (ResultOperand->Infinity != FALSE) {
        KiSetRegisterValue(ContextBlock->Fd + 32,
                           DOUBLE_INFINITY_VALUE_LOW,
                           ContextBlock->ExceptionFrame,
                           ContextBlock->TrapFrame);

        KiSetRegisterValue(ContextBlock->Fd + 32 + 1,
                           DOUBLE_INFINITY_VALUE_HIGH | (ResultOperand->Sign << 31),
                           ContextBlock->ExceptionFrame,
                           ContextBlock->TrapFrame);

        TrapFrame->Fir = ContextBlock->BranchAddress;
        return TRUE;
    }

    //
    // If the overflow bit is set, then right shift the mantissa one bit,
    // accumlate the lost bit with the sticky bits, and adjust the exponent
    // value.
    //

    if ((ResultOperand->MantissaHigh & (1 << (55 - 32))) != 0) {
        StickyBits |= (ResultOperand->MantissaLow & 0x1);
        ResultOperand->MantissaLow =
                        (ResultOperand->MantissaLow >> 1) |
                                            (ResultOperand->MantissaHigh << 31);

        ResultOperand->MantissaHigh >>= 1;
        ResultOperand->Exponent += 1;
    }

    //
    // If the mantissa is not zero, then normalize the mantissa by left
    // shifting one bit at a time until there is a one bit in bit 54.
    //

    if ((ResultOperand->MantissaLow != 0) || (ResultOperand->MantissaHigh != 0)) {
        while ((ResultOperand->MantissaHigh & (1 << (54 - 32))) == 0) {
            ResultOperand->MantissaHigh =
                        (ResultOperand->MantissaHigh << 1) |
                                            (ResultOperand->MantissaLow >> 31);

            ResultOperand->MantissaLow <<= 1;
            ResultOperand->Exponent -= 1;
        }
    }

    //
    // Right shift the mantissa one bit and accumlate the lost bit with the
    // sticky bits.
    //

    StickyBits |= (ResultOperand->MantissaLow & 0x1);
    ResultOperand->MantissaLow =
                        (ResultOperand->MantissaLow >> 1) |
                                            (ResultOperand->MantissaHigh << 31);

    ResultOperand->MantissaHigh >>= 1;

    //
    // Round the result value using the mantissa and the sticky bits,
    //

    RoundBit = ResultOperand->MantissaLow & 0x1;
    switch (ContextBlock->Round) {

        //
        // Round to nearest representable number.
        //

    case ROUND_TO_NEAREST:
        if (RoundBit != 0) {
            if ((StickyBits != 0) || ((ResultOperand->MantissaLow & 0x2) != 0)) {
                ResultOperand->MantissaLow += 2;
                if (ResultOperand->MantissaLow < 2) {
                    ResultOperand->MantissaHigh += 1;
                }
            }
        }

        break;

        //
        // Round toward zero.
        //

    case ROUND_TO_ZERO:
        break;

        //
        // Round toward plus infinity.
        //

    case ROUND_TO_PLUS_INFINITY:
        if ((ResultOperand->Sign == 0) &&
            ((StickyBits != 0) || (RoundBit != 0))) {
            ResultOperand->MantissaLow += 2;
            if (ResultOperand->MantissaLow < 2) {
                ResultOperand->MantissaHigh += 1;
            }
        }

        break;

        //
        // Round toward minus infinity.
        //

    case ROUND_TO_MINUS_INFINITY:
        if ((ResultOperand->Sign != 0) &&
            ((StickyBits != 0) || (RoundBit != 0))) {
            ResultOperand->MantissaLow += 2;
            if (ResultOperand->MantissaLow < 2) {
                ResultOperand->MantissaHigh += 1;
            }
        }

        break;
    }

    //
    // If rounding resulted in a carry into bit 54, then right shift the
    // mantissa one bit and adjust the exponent.
    //

    if ((ResultOperand->MantissaHigh & (1 << (54 - 32))) != 0) {
        ResultOperand->MantissaLow =
                        (ResultOperand->MantissaLow >> 1) |
                                            (ResultOperand->MantissaHigh << 31);

        ResultOperand->MantissaHigh >>= 1;
        ResultOperand->Exponent += 1;
    }

    //
    // Right shift the mantissa one bit to normalize the final result.
    //

    StickyBits |= ResultOperand->MantissaLow & 0x1;
    ResultOperand->MantissaLow =
                        (ResultOperand->MantissaLow >> 1) |
                                            (ResultOperand->MantissaHigh << 31);

    ResultOperand->MantissaHigh >>= 1;

    //
    // If the exponent value is greater than or equal to the maximum
    // exponent value, then overflow has occurred. This results in both
    // the inexact and overflow sticky bits being set in FSR.
    //
    // If the exponent value is less than or equal to the minimum exponent
    // value, the mantissa is nonzero, and the result is inexact or the
    // denormalized result causes loss of accuracy, then underflow has
    // occurred. If denormals are being flushed to zero, then a result of
    // zero is returned. Otherwise, both the inexact and underflow sticky
    // bits are set in FSR.
    //
    // Otherwise, a normal result can be delivered, but it may be inexact.
    // If the result is inexact, then the inexact sticky bit is set in FSR.
    //

    if (ResultOperand->Exponent >= DOUBLE_MAXIMUM_EXPONENT) {
        Inexact = TRUE;
        Overflow = TRUE;
        Underflow = FALSE;

        //
        // The overflow value is dependent on the rounding mode.
        //

        switch (ContextBlock->Round) {

            //
            // Round to nearest representable number.
            //
            // The result value is infinity with the sign of the result.
            //

        case ROUND_TO_NEAREST:
            ResultValueLow = DOUBLE_INFINITY_VALUE_LOW;
            ResultValueHigh =
                        DOUBLE_INFINITY_VALUE_HIGH | (ResultOperand->Sign << 31);

            break;

            //
            // Round toward zero.
            //
            // The result is the maximum number with the sign of the result.
            //

        case ROUND_TO_ZERO:
            ResultValueLow = DOUBLE_MAXIMUM_VALUE_LOW;
            ResultValueHigh =
                        DOUBLE_MAXIMUM_VALUE_HIGH | (ResultOperand->Sign << 31);
            break;

            //
            // Round toward plus infinity.
            //
            // If the sign of the result is positive, then the result is
            // plus infinity. Otherwise, the result is the maximum negative
            // number.
            //

        case ROUND_TO_PLUS_INFINITY:
            if (ResultOperand->Sign == 0) {
                ResultValueLow = DOUBLE_INFINITY_VALUE_LOW;
                ResultValueHigh = DOUBLE_INFINITY_VALUE_HIGH;

            } else {
                ResultValueLow = DOUBLE_MAXIMUM_VALUE_LOW;
                ResultValueHigh = (ULONG)(DOUBLE_MAXIMUM_VALUE_HIGH | (1 << 31));
            }

            break;

            //
            // Round toward minus infinity.
            //
            // If the sign of the result is negative, then the result is
            // negative infinity. Otherwise, the result is the maximum
            // positive number.
            //


        case ROUND_TO_MINUS_INFINITY:
            if (ResultOperand->Sign != 0) {
                ResultValueLow = DOUBLE_INFINITY_VALUE_LOW;
                ResultValueHigh = (ULONG)(DOUBLE_INFINITY_VALUE_HIGH | (1 << 31));

            } else {
                ResultValueLow = DOUBLE_MAXIMUM_VALUE_LOW;
                ResultValueHigh = DOUBLE_MAXIMUM_VALUE_HIGH;
            }

            break;
        }

        //
        // Compute the overflow exception result value by subtracting 1536
        // from the exponent.
        //

        ExceptionResultLow = ResultOperand->MantissaLow;
        ExceptionResultHigh = ResultOperand->MantissaHigh & ((1 << (52 - 32)) - 1);
        ExceptionResultHigh |= ((ResultOperand->Exponent - 1536) << (52 - 32));
        ExceptionResultHigh |= (ResultOperand->Sign << 31);

    } else {
        if ((ResultOperand->Exponent <= DOUBLE_MINIMUM_EXPONENT) &&
            (ResultOperand->MantissaHigh != 0)) {
            if (((PFSR)&TrapFrame->Fsr)->FS == 0) {
                DenormalizeShift = 1 - ResultOperand->Exponent;
                if (DenormalizeShift >= 53) {
                    DenormalizeShift = 53;
                }

                if (DenormalizeShift >= 32) {
                    DenormalizeShift -= 32;
                    StickyBits |= ResultOperand->MantissaLow |
                        (ResultOperand->MantissaHigh & ((1 << DenormalizeShift) - 1));

                    ResultValueLow = ResultOperand->MantissaHigh >> DenormalizeShift;
                    ResultValueHigh = 0;

                } else if (DenormalizeShift > 0) {
                    StickyBits |=
                        ResultOperand->MantissaLow & ((1 << DenormalizeShift) - 1);

                    ResultValueLow =
                        (ResultOperand->MantissaLow >> DenormalizeShift) |
                        (ResultOperand->MantissaHigh << (32 - DenormalizeShift));

                    ResultValueHigh =
                            (ResultOperand->MantissaHigh >> DenormalizeShift);

                } else {
                    ResultValueLow = ResultOperand->MantissaLow;
                    ResultValueHigh = ResultOperand->MantissaHigh;
                }

                ResultValueHigh |= (ResultOperand->Sign << 31);
                if (StickyBits != 0) {
                    Inexact = TRUE;
                    Overflow = FALSE;
                    Underflow = TRUE;

                    //
                    // Compute the underflow exception result value by adding
                    // 1536 to the exponent.
                    //

                    ExceptionResultLow = ResultOperand->MantissaLow;
                    ExceptionResultHigh = ResultOperand->MantissaHigh & ((1 << (52 - 32)) - 1);
                    ExceptionResultHigh |= ((ResultOperand->Exponent + 1536) << (52 - 32));
                    ExceptionResultHigh |= (ResultOperand->Sign << 31);

                } else {
                    Inexact = FALSE;
                    Overflow = FALSE;
                    Underflow = FALSE;
                }

            } else {
                ResultValueLow = 0;
                ResultValueHigh = 0;
                Inexact = FALSE;
                Overflow = FALSE;
                Underflow = FALSE;
            }

        } else {
            if (ResultOperand->MantissaHigh == 0) {
                ResultOperand->Exponent = 0;
            }

            ResultValueLow = ResultOperand->MantissaLow;
            ResultValueHigh = ResultOperand->MantissaHigh & ((1 << (52 - 32)) - 1);
            ResultValueHigh |= (ResultOperand->Exponent << (52 - 32));
            ResultValueHigh |= (ResultOperand->Sign << 31);
            Inexact = StickyBits ? TRUE : FALSE;
            Overflow = FALSE;
            Underflow = FALSE;
        }
    }

    //
    // Check to determine if an exception should be delivered or the result
    // should be written to the destination register.
    //

    if (Overflow != FALSE) {
        ((PFSR)&TrapFrame->Fsr)->SI = 1;
        ((PFSR)&TrapFrame->Fsr)->SO = 1;
        if ((((PFSR)&TrapFrame->Fsr)->EO != 0) ||
            (((PFSR)&TrapFrame->Fsr)->EI != 0)) {
            if (((PFSR)&TrapFrame->Fsr)->EO != 0) {
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_OVERFLOW;

            } else {
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
            }

            IeeeValue = (PFP_IEEE_VALUE)&ExceptionRecord->ExceptionInformation[2];
            IeeeValue->Value.Fp64Value.W[0] = ExceptionResultLow;
            IeeeValue->Value.Fp64Value.W[1] = ExceptionResultHigh;
            ((PFSR)&TrapFrame->Fsr)->XI = 1;
            ((PFSR)&TrapFrame->Fsr)->XO = 1;
            return FALSE;
        }

    } else if (Underflow != FALSE) {
        ((PFSR)&TrapFrame->Fsr)->SI = 1;
        ((PFSR)&TrapFrame->Fsr)->SU = 1;
        if ((((PFSR)&TrapFrame->Fsr)->EU != 0) ||
            (((PFSR)&TrapFrame->Fsr)->EI != 0)) {
            if (((PFSR)&TrapFrame->Fsr)->EU != 0) {
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_UNDERFLOW;

            } else {
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
            }

            IeeeValue = (PFP_IEEE_VALUE)&ExceptionRecord->ExceptionInformation[2];
            IeeeValue->Value.Fp64Value.W[0] = ExceptionResultLow;
            IeeeValue->Value.Fp64Value.W[1] = ExceptionResultHigh;
            ((PFSR)&TrapFrame->Fsr)->XI = 1;
            ((PFSR)&TrapFrame->Fsr)->XU = 1;
            return FALSE;
        }

    } else if (Inexact != FALSE) {
        ((PFSR)&TrapFrame->Fsr)->SI = 1;
        if (((PFSR)&TrapFrame->Fsr)->EI != 0) {
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
            ((PFSR)&TrapFrame->Fsr)->XI = 1;
            IeeeValue = (PFP_IEEE_VALUE)&ExceptionRecord->ExceptionInformation[2];
            IeeeValue->Value.Fp64Value.W[0] = ResultValueLow;
            IeeeValue->Value.Fp64Value.W[1] = ResultValueHigh;
            return FALSE;
        }
    }

    //
    // Set the destination register value, update the return address,
    // and return a value of TRUE.
    //

    KiSetRegisterValue(ContextBlock->Fd + 32,
                       ResultValueLow,
                       ContextBlock->ExceptionFrame,
                       ContextBlock->TrapFrame);

    KiSetRegisterValue(ContextBlock->Fd + 32 + 1,
                       ResultValueHigh,
                       ContextBlock->ExceptionFrame,
                       ContextBlock->TrapFrame);

    TrapFrame->Fir = ContextBlock->BranchAddress;
    return TRUE;
}

BOOLEAN
KiNormalizeLongword (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN PFP_DOUBLE_OPERAND ResultOperand
    )

/*++

Routine Description:

    This function is called to convert a result value to a longword result.

    N.B. The result value is specified with a guard bit on the right,
        the hidden bit (if appropriate), and an overlfow bit of zero.
        The result format is:

        <63:55> - zero
        <54 - hidden bit
        <53:2> - mantissa
        <1> - guard bit
        <0> - round bit

        There are no sticky bits.

Arguments:

    ContextBlock - Supplies a pointer to the emulation context block.

    ResultOperand - Supplies a pointer to the result operand value.

Return Value:

    If there is not an exception, or the exception is handled, then a proper
    result is stored in the destination result, the continuation address is
    set, and a value of TRUE is returned. Otherwise, no value is stored and
    a value of FALSE is returned.

--*/

{

    PEXCEPTION_RECORD ExceptionRecord;
    LONG ExponentShift;
    PFP_IEEE_VALUE IeeeValue;
    BOOLEAN Inexact;
    BOOLEAN Overflow;
    ULONG ResultValue;
    ULONG RoundBit;
    ULONG StickyBits;
    PKTRAP_FRAME TrapFrame;

    //
    // Subtract out the exponent bias and divide the cases into right
    // and left shifts.
    //

    ExceptionRecord = ContextBlock->ExceptionRecord;
    TrapFrame = ContextBlock->TrapFrame;
    ExponentShift = ResultOperand->Exponent - DOUBLE_EXPONENT_BIAS;
    if (ExponentShift < 23) {

        //
        // The integer value is less than 2**23 and a right shift must
        // be performed.
        //

        ExponentShift = 22 - ExponentShift;
        if (ExponentShift > 24) {
            ExponentShift = 24;
        }

        StickyBits =
                (ResultOperand->MantissaLow >> 2) |
                (ResultOperand->MantissaHigh << (32 - ExponentShift));

        ResultValue = ResultOperand->MantissaHigh >> ExponentShift;
        Overflow = FALSE;

    } else {

        //
        // The integer value is two or greater and a left shift must be
        // performed.
        //

        ExponentShift -= 22;
        if (ExponentShift <= (31 - 22)) {
            StickyBits = ResultOperand->MantissaLow << ExponentShift;
            ResultValue =
                (ResultOperand->MantissaHigh << ExponentShift) |
                (ResultOperand->MantissaLow >> (32 - ExponentShift));

            Overflow = FALSE;

        } else {
            Overflow = TRUE;
        }
    }

    //
    // Round the result value using the mantissa and the sticky bits,
    //

    RoundBit = StickyBits >> 31;
    StickyBits <<= 1;
    switch (ContextBlock->Round) {

        //
        // Round to nearest representable number.
        //

    case ROUND_TO_NEAREST:
        if (RoundBit != 0) {
            if ((StickyBits != 0) || ((ResultValue & 0x1) != 0)) {
                ResultValue += 1;
                if (ResultValue == 0) {
                    Overflow = TRUE;
                }
            }
        }

        break;

        //
        // Round toward zero.
        //

    case ROUND_TO_ZERO:
        break;

        //
        // Round toward plus infinity.
        //

    case ROUND_TO_PLUS_INFINITY:
        if ((ResultOperand->Sign == 0) && (StickyBits != 0)) {
            ResultValue += 1;
            if (ResultValue == 0) {
                Overflow = TRUE;
            }
        }

        break;

        //
        // Round toward minus infinity.
        //

    case ROUND_TO_MINUS_INFINITY:
        if ((ResultOperand->Sign != 0) && (StickyBits != 0)) {
            ResultValue += 1;
            if (ResultValue == 0) {
                Overflow = TRUE;
            }
        }

        break;
    }

    //
    // If the result value is positive and the result is negative, then
    // overflow has occurred. Otherwise, negate the result value and
    // check if the result is negative. If the result is positive, then
    // overflow has occurred.
    //

    if (ResultOperand->Sign == 0) {
        if ((ResultValue >> 31) != 0) {
            Overflow = TRUE;
        }

    } else {
        ResultValue = ~ResultValue + 1;
        if ((ResultValue >> 31) == 0) {
            Overflow = TRUE;
        }
    }

    //
    // Check to determine if an exception should be delivered or the result
    // should be written to the destination register.
    //

    if (Overflow != FALSE) {
        return KiInvalidOperationLongword(ContextBlock,
                                          FALSE,
                                          0);

    } else if ((StickyBits | RoundBit) != 0) {
        ((PFSR)&TrapFrame->Fsr)->SI = 1;
        if (((PFSR)&TrapFrame->Fsr)->EI != 0) {
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
            ((PFSR)&TrapFrame->Fsr)->XI = 1;
            IeeeValue = (PFP_IEEE_VALUE)&ExceptionRecord->ExceptionInformation[2];
            IeeeValue->Value.U32Value = ResultValue;
            return FALSE;
        }

    }

    //
    // Set the destination register value, update the return address,
    // and return a value of TRUE.
    //

    KiSetRegisterValue(ContextBlock->Fd + 32,
                       ResultValue,
                       ContextBlock->ExceptionFrame,
                       ContextBlock->TrapFrame);

    TrapFrame->Fir = ContextBlock->BranchAddress;
    return TRUE;
}

BOOLEAN
KiNormalizeQuadword (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN PFP_DOUBLE_OPERAND ResultOperand
    )

/*++

Routine Description:

    This function is called to convert a result value to a quadword result.

    N.B. The result value is specified with a guard bit on the right,
        the hidden bit (if appropriate), and an overlfow bit of zero.
        The result format is:

        <63:55> - zero
        <54 - hidden bit
        <53:2> - mantissa
        <1> - guard bit
        <0> - round bit

        There are no sticky bits.

Arguments:

    ContextBlock - Supplies a pointer to the emulation context block.

    ResultOperand - Supplies a pointer to the result operand value.

Return Value:

    If there is not an exception, or the exception is handled, then a proper
    result is stored in the destination result, the continuation address is
    set, and a value of TRUE is returned. Otherwise, no value is stored and
    a value of FALSE is returned.

--*/

{

    PEXCEPTION_RECORD ExceptionRecord;
    LONG ExponentShift;
    PFP_IEEE_VALUE IeeeValue;
    BOOLEAN Inexact;
    BOOLEAN Overflow;
    union {
        ULONGLONG ResultValue;
        ULARGE_INTEGER LargeValue;
    }u;

    ULONG RoundBit;
    ULONG StickyBits;
    PKTRAP_FRAME TrapFrame;

    //
    // Subtract out the exponent bias and divide the cases into right
    // and left shifts.
    //

    ExceptionRecord = ContextBlock->ExceptionRecord;
    TrapFrame = ContextBlock->TrapFrame;
    ExponentShift = ResultOperand->Exponent - DOUBLE_EXPONENT_BIAS;
    if (ExponentShift < 54) {

        //
        // The integer value is less than 2**52 and a right shift must
        // be performed.
        //

        ExponentShift = 54 - ExponentShift;
        if (ExponentShift > 54) {
            ExponentShift = 54;
        }

        StickyBits = (ULONG)(ResultOperand->Mantissa << (32 - ExponentShift));
        u.ResultValue = ResultOperand->Mantissa >> ExponentShift;
        Overflow = FALSE;

    } else {

        //
        // The integer value is two or greater and a left shift must be
        // performed.
        //

        ExponentShift -= 54;
        if (ExponentShift <= (63 - 54)) {
            StickyBits = 0;
            u.ResultValue = ResultOperand->Mantissa << ExponentShift;
            Overflow = FALSE;

        } else {
            Overflow = TRUE;
        }
    }

    //
    // Round the result value using the mantissa and the sticky bits,
    //

    RoundBit = StickyBits >> 31;
    StickyBits <<= 1;
    switch (ContextBlock->Round) {

        //
        // Round to nearest representable number.
        //

    case ROUND_TO_NEAREST:
        if (RoundBit != 0) {
            if ((StickyBits != 0) || ((u.ResultValue & 0x1) != 0)) {
                u.ResultValue += 1;
                if (u.ResultValue == 0) {
                    Overflow = TRUE;
                }
            }
        }

        break;

        //
        // Round toward zero.
        //

    case ROUND_TO_ZERO:
        break;

        //
        // Round toward plus infinity.
        //

    case ROUND_TO_PLUS_INFINITY:
        if ((ResultOperand->Sign == 0) && (StickyBits != 0)) {
            u.ResultValue += 1;
            if (u.ResultValue == 0) {
                Overflow = TRUE;
            }
        }

        break;

        //
        // Round toward minus infinity.
        //

    case ROUND_TO_MINUS_INFINITY:
        if ((ResultOperand->Sign != 0) && (StickyBits != 0)) {
            u.ResultValue += 1;
            if (u.ResultValue == 0) {
                Overflow = TRUE;
            }
        }

        break;
    }

    //
    // If the result value is positive and the result is negative, then
    // overflow has occurred. Otherwise, negate the result value and
    // check if the result is negative. If the result is positive, then
    // overflow has occurred.
    //

    if (ResultOperand->Sign == 0) {
        if ((u.ResultValue >> 63) != 0) {
            Overflow = TRUE;
        }

    } else {
        u.ResultValue = ~u.ResultValue + 1;
        if ((u.ResultValue >> 63) == 0) {
            Overflow = TRUE;
        }
    }

    //
    // Check to determine if an exception should be delivered or the result
    // should be written to the destination register.
    //

    if (Overflow != FALSE) {
        return KiInvalidOperationQuadword(ContextBlock,
                                          FALSE,
                                          0);

    } else if ((StickyBits | RoundBit) != 0) {
        ((PFSR)&TrapFrame->Fsr)->SI = 1;
        if (((PFSR)&TrapFrame->Fsr)->EI != 0) {
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
            ((PFSR)&TrapFrame->Fsr)->XI = 1;
            IeeeValue = (PFP_IEEE_VALUE)&ExceptionRecord->ExceptionInformation[2];
            IeeeValue->Value.U64Value.QuadPart = u.ResultValue;
            return FALSE;
        }

    }

    //
    // Set the destination register value, update the return address,
    // and return a value of TRUE.
    //

    KiSetRegisterValue(ContextBlock->Fd + 32,
                       u.LargeValue.LowPart,
                       ContextBlock->ExceptionFrame,
                       ContextBlock->TrapFrame);

    KiSetRegisterValue(ContextBlock->Fd + 33,
                       u.LargeValue.HighPart,
                       ContextBlock->ExceptionFrame,
                       ContextBlock->TrapFrame);

    TrapFrame->Fir = ContextBlock->BranchAddress;
    return TRUE;
}

BOOLEAN
KiNormalizeSingle (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN PFP_SINGLE_OPERAND ResultOperand,
    IN ULONG StickyBits
    )

/*++

Routine Description:

    This function is called to normalize a single floating result.

    N.B. The result value is specified with a guard bit on the right,
        the hidden bit (if appropriate), and a possible overflow bit.
        The result format is:

        <31:27> - zero
        <26> - overflow bit
        <25> - hidden bit
        <24:2> - mantissa
        <1> - guard bit
        <0> - round bit

        The sticky bits specify bits that were lost during the computable.

Arguments:

    ContextBlock - Supplies a pointer to the emulation context block.

    ResultOperand - Supplies a pointer to the result operand value.

    StickyBits - Supplies the value of the sticky bits.

Return Value:

    If there is not an exception, or the exception is handled, then a proper
    result is stored in the destination result, the continuation address is
    set, and a value of TRUE is returned. Otherwise, no value is stored and
    a value of FALSE is returned.

--*/

{

    ULONG DenormalizeShift;
    PEXCEPTION_RECORD ExceptionRecord;
    ULONG ExceptionResult;
    PFP_IEEE_VALUE IeeeValue;
    BOOLEAN Inexact;
    BOOLEAN Overflow;
    ULONG ResultValue;
    ULONG RoundBit;
    PKTRAP_FRAME TrapFrame;
    BOOLEAN Underflow;

    //
    // If the result is infinite, then store a properly signed infinity
    // in the destination register and return a value of TRUE. Otherwise,
    // round and normalize the result and check for overflow and underflow.
    //

    ExceptionRecord = ContextBlock->ExceptionRecord;
    TrapFrame = ContextBlock->TrapFrame;
    if (ResultOperand->Infinity != FALSE) {
        KiSetRegisterValue(ContextBlock->Fd + 32,
                           SINGLE_INFINITY_VALUE | (ResultOperand->Sign << 31),
                           ContextBlock->ExceptionFrame,
                           ContextBlock->TrapFrame);

        TrapFrame->Fir = ContextBlock->BranchAddress;
        return TRUE;
    }

    //
    // If the overflow bit is set, then right shift the mantissa one bit,
    // accumlate the lost bit with the sticky bits, and adjust the exponent
    // value.
    //

    if ((ResultOperand->Mantissa & (1 << 26)) != 0) {
        StickyBits |= (ResultOperand->Mantissa & 0x1);
        ResultOperand->Mantissa >>= 1;
        ResultOperand->Exponent += 1;
    }

    //
    // If the mantissa is not zero, then normalize the mantissa by left
    // shifting one bit at a time until there is a one bit in bit 25.
    //

    if (ResultOperand->Mantissa != 0) {
        while ((ResultOperand->Mantissa & (1 << 25)) == 0) {
            ResultOperand->Mantissa <<= 1;
            ResultOperand->Exponent -= 1;
        }
    }

    //
    // Right shift the mantissa one bit and accumlate the lost bit with the
    // sticky bits.
    //

    StickyBits |= (ResultOperand->Mantissa & 0x1);
    ResultOperand->Mantissa >>= 1;

    //
    // Round the result value using the mantissa, the round bit, and the
    // sticky bits,
    //

    RoundBit = ResultOperand->Mantissa & 0x1;
    switch (ContextBlock->Round) {

        //
        // Round to nearest representable number.
        //

    case ROUND_TO_NEAREST:
        if (RoundBit != 0) {
            if ((StickyBits != 0) || ((ResultOperand->Mantissa & 0x2) != 0)) {
                ResultOperand->Mantissa += 2;
            }
        }

        break;

        //
        // Round toward zero.
        //

    case ROUND_TO_ZERO:
        break;

        //
        // Round toward plus infinity.
        //

    case ROUND_TO_PLUS_INFINITY:
        if ((ResultOperand->Sign == 0) &&
            ((StickyBits != 0) || (RoundBit != 0))) {
            ResultOperand->Mantissa += 2;
        }

        break;

        //
        // Round toward minus infinity.
        //

    case ROUND_TO_MINUS_INFINITY:
        if ((ResultOperand->Sign != 0) &&
            ((StickyBits != 0) || (RoundBit != 0))) {
            ResultOperand->Mantissa += 2;
        }

        break;
    }

    //
    // If rounding resulted in a carry into bit 25, then right shift the
    // mantissa one bit and adjust the exponent.
    //

    if ((ResultOperand->Mantissa & (1 << 25)) != 0) {
        ResultOperand->Mantissa >>= 1;
        ResultOperand->Exponent += 1;
    }

    //
    // Right shift the mantissa one bit to normalize the final result.
    //

    StickyBits |= RoundBit;
    ResultOperand->Mantissa >>= 1;

    //
    // If the exponent value is greater than or equal to the maximum
    // exponent value, then overflow has occurred. This results in both
    // the inexact and overflow stickt bits being set in FSR.
    //
    // If the exponent value is less than or equal to the minimum exponent
    // value, the mantissa is nonzero, and the result is inexact or the
    // denormalized result causes loss of accuracy, then underflow has
    // occurred. If denormals are being flushed to zero, then a result of
    // zero is returned. Otherwise, both the inexact and underflow sticky
    // bits are set in FSR.
    //
    // Otherwise, a normal result can be delivered, but it may be inexact.
    // If the result is inexact, then the inexact sticky bit is set in FSR.
    //

    if (ResultOperand->Exponent >= SINGLE_MAXIMUM_EXPONENT) {
        Inexact = TRUE;
        Overflow = TRUE;
        Underflow = FALSE;

        //
        // The overflow value is dependent on the rounding mode.
        //

        switch (ContextBlock->Round) {

            //
            // Round to nearest representable number.
            //
            // The result value is infinity with the sign of the result.
            //

        case ROUND_TO_NEAREST:
            ResultValue = SINGLE_INFINITY_VALUE | (ResultOperand->Sign << 31);
            break;

            //
            // Round toward zero.
            //
            // The result is the maximum number with the sign of the result.
            //

        case ROUND_TO_ZERO:
            ResultValue = SINGLE_MAXIMUM_VALUE | (ResultOperand->Sign << 31);
            break;

            //
            // Round toward plus infinity.
            //
            // If the sign of the result is positive, then the result is
            // plus infinity. Otherwise, the result is the maximum negative
            // number.
            //

        case ROUND_TO_PLUS_INFINITY:
            if (ResultOperand->Sign == 0) {
                ResultValue = SINGLE_INFINITY_VALUE;

            } else {
                ResultValue = (ULONG)(SINGLE_MAXIMUM_VALUE | (1 << 31));
            }

            break;

            //
            // Round toward minus infinity.
            //
            // If the sign of the result is negative, then the result is
            // negative infinity. Otherwise, the result is the maximum
            // positive number.
            //


        case ROUND_TO_MINUS_INFINITY:
            if (ResultOperand->Sign != 0) {
                ResultValue = (ULONG)(SINGLE_INFINITY_VALUE | (1 << 31));

            } else {
                ResultValue = SINGLE_MAXIMUM_VALUE;
            }

            break;
        }

        //
        // Compute the overflow exception result value by subtracting 192
        // from the exponent.
        //

        ExceptionResult = ResultOperand->Mantissa & ((1 << 23) - 1);
        ExceptionResult |= ((ResultOperand->Exponent - 192) << 23);
        ExceptionResult |= (ResultOperand->Sign << 31);

    } else {
        if ((ResultOperand->Exponent <= SINGLE_MINIMUM_EXPONENT) &&
            (ResultOperand->Mantissa != 0)) {
            if (((PFSR)&TrapFrame->Fsr)->FS == 0) {
                DenormalizeShift = 1 - ResultOperand->Exponent;
                if (DenormalizeShift >= 24) {
                    DenormalizeShift = 24;
                }

                ResultValue = ResultOperand->Mantissa >> DenormalizeShift;
                ResultValue |= (ResultOperand->Sign << 31);
                if ((StickyBits != 0) ||
                    ((ResultOperand->Mantissa & ((1 << DenormalizeShift) - 1)) != 0)) {
                    Inexact = TRUE;
                    Overflow = FALSE;
                    Underflow = TRUE;

                    //
                    // Compute the underflow exception result value by adding
                    // 192 to the exponent.
                    //

                    ExceptionResult = ResultOperand->Mantissa & ((1 << 23) - 1);
                    ExceptionResult |= ((ResultOperand->Exponent + 192) << 23);
                    ExceptionResult |= (ResultOperand->Sign << 31);

                } else {
                    Inexact = FALSE;
                    Overflow = FALSE;
                    Underflow = FALSE;
                }

            } else {
                ResultValue = 0;
                Inexact = FALSE;
                Overflow = FALSE;
                Underflow = FALSE;
            }

        } else {
            if (ResultOperand->Mantissa == 0) {
                ResultOperand->Exponent = 0;
            }

            ResultValue = ResultOperand->Mantissa & ((1 << 23) - 1);
            ResultValue |= (ResultOperand->Exponent << 23);
            ResultValue |= (ResultOperand->Sign << 31);
            Inexact = StickyBits ? TRUE : FALSE;
            Overflow = FALSE;
            Underflow = FALSE;
        }
    }

    //
    // Check to determine if an exception should be delivered or the result
    // should be written to the destination register.
    //

    if (Overflow != FALSE) {
        ((PFSR)&TrapFrame->Fsr)->SI = 1;
        ((PFSR)&TrapFrame->Fsr)->SO = 1;
        if ((((PFSR)&TrapFrame->Fsr)->EO != 0) ||
            (((PFSR)&TrapFrame->Fsr)->EI != 0)) {
            if (((PFSR)&TrapFrame->Fsr)->EO != 0) {
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_OVERFLOW;

            } else {
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
            }

            IeeeValue = (PFP_IEEE_VALUE)&ExceptionRecord->ExceptionInformation[2];
            IeeeValue->Value.Fp32Value.W[0] = ExceptionResult;
            ((PFSR)&TrapFrame->Fsr)->XI = 1;
            ((PFSR)&TrapFrame->Fsr)->XO = 1;
            return FALSE;
        }

    } else if (Underflow != FALSE) {
        ((PFSR)&TrapFrame->Fsr)->SI = 1;
        ((PFSR)&TrapFrame->Fsr)->SU = 1;
        if ((((PFSR)&TrapFrame->Fsr)->EU != 0) ||
            (((PFSR)&TrapFrame->Fsr)->EI != 0)) {
            if (((PFSR)&TrapFrame->Fsr)->EU != 0) {
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_UNDERFLOW;

            } else {
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
            }

            IeeeValue = (PFP_IEEE_VALUE)&ExceptionRecord->ExceptionInformation[2];
            IeeeValue->Value.Fp32Value.W[0] = ExceptionResult;
            ((PFSR)&TrapFrame->Fsr)->XI = 1;
            ((PFSR)&TrapFrame->Fsr)->XU = 1;
            return FALSE;
        }

    } else if (Inexact != FALSE) {
        ((PFSR)&TrapFrame->Fsr)->SI = 1;
        if (((PFSR)&TrapFrame->Fsr)->EI != 0) {
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
            ((PFSR)&TrapFrame->Fsr)->XI = 1;
            IeeeValue = (PFP_IEEE_VALUE)&ExceptionRecord->ExceptionInformation[2];
            IeeeValue->Value.Fp32Value.W[0] = ResultValue;
            return FALSE;
        }

    }

    //
    // Set the destination register value, update the return address,
    // and return a value of TRUE.
    //

    KiSetRegisterValue(ContextBlock->Fd + 32,
                       ResultValue,
                       ContextBlock->ExceptionFrame,
                       ContextBlock->TrapFrame);

    TrapFrame->Fir = ContextBlock->BranchAddress;
    return TRUE;
}

VOID
KiUnpackDouble (
    IN ULONG Source,
    IN PFP_CONTEXT_BLOCK ContextBlock,
    OUT PFP_DOUBLE_OPERAND DoubleOperand
    )

/*++

Routine Description:

    This function is called to unpack a double floating value from the
    specified source register.

    N.B. The unpacked mantissa value is returned with a guard bit and a
        round bit on the right and the hidden bit inserted if appropriate.
        The format of the returned value is:

        <63:55> - zero
        <54> - hidden bit
        <53:2> - mantissa
        <1> - guard bit
        <0> - round bit

Arguments:

    Source - Supplies the number of the register that contains the operand.

    ContextBlock - Supplies a pointer to the emulation context block.

    DoubleOperand - Supplies a pointer to a structure that is to receive the
        operand value.

Return Value:

    None.

--*/

{

    ULONG Value1;
    ULONG Value2;

    //
    // Get the source register value and unpack the sign, exponent, and
    // mantissa value.
    //

    Value1 = KiGetRegisterValue(Source + 32,
                                ContextBlock->ExceptionFrame,
                                ContextBlock->TrapFrame);

    Value2 = KiGetRegisterValue(Source + 32 + 1,
                                ContextBlock->ExceptionFrame,
                                ContextBlock->TrapFrame);

    DoubleOperand->Sign = Value2 >> 31;
    DoubleOperand->Exponent = (Value2 >> (52 - 32)) & 0x7ff;
    DoubleOperand->MantissaHigh = Value2 & 0xfffff;
    DoubleOperand->MantissaLow = Value1;

    //
    // If the exponent is the largest possible value, then the number is
    // either a Nan or an infinity.
    //

    if (DoubleOperand->Exponent == DOUBLE_MAXIMUM_EXPONENT) {
        if ((DoubleOperand->MantissaLow | DoubleOperand->MantissaHigh) != 0) {
            DoubleOperand->Infinity = FALSE;
            DoubleOperand->Nan = TRUE;

        } else {
            DoubleOperand->Infinity = TRUE;
            DoubleOperand->Nan = FALSE;
        }

    } else {
        DoubleOperand->Infinity = FALSE;
        DoubleOperand->Nan = FALSE;
        if (DoubleOperand->Exponent == DOUBLE_MINIMUM_EXPONENT) {
            if ((DoubleOperand->MantissaHigh | DoubleOperand->MantissaLow) != 0) {
                DoubleOperand->Exponent += 1;
                while ((DoubleOperand->MantissaHigh & (1 << 20)) == 0) {
                    DoubleOperand->MantissaHigh =
                            (DoubleOperand->MantissaHigh << 1) |
                                            (DoubleOperand->MantissaLow >> 31);
                    DoubleOperand->MantissaLow <<= 1;
                    DoubleOperand->Exponent -= 1;
                }
            }

        } else {
            DoubleOperand->MantissaHigh |= (1 << 20);
        }
    }

    //
    // Left shift the mantissa 2-bits to provide for a guard bit and a round
    // bit.
    //

    DoubleOperand->MantissaHigh =
        (DoubleOperand->MantissaHigh << 2) | (DoubleOperand->MantissaLow >> 30);
    DoubleOperand->MantissaLow <<= 2;
    return;
}

VOID
KiUnpackSingle (
    IN ULONG Source,
    IN PFP_CONTEXT_BLOCK ContextBlock,
    OUT PFP_SINGLE_OPERAND SingleOperand
    )

/*++

Routine Description:

    This function is called to unpack a single floating value from the
    specified source register.

    N.B. The unpacked mantissa value is returned with a guard bit and a
        round bit on the right and the hidden bit inserted if appropriate.
        The format of the returned value is:

        <31:26> - zero
        <25> - hidden bit
        <24:2> - mantissa
        <1> - guard bit
        <0> - round bit

Arguments:

    Source - Supplies the number of the register that contains the operand.

    ContextBlock - Supplies a pointer to the emulation context block.

    SingleOperand - Supplies a pointer to a structure that is to receive the
        operand value.

Return Value:

    None.

--*/

{

    ULONG Value;

    //
    // Get the source register value and unpack the sign, exponent, and
    // mantissa value.
    //

    Value = KiGetRegisterValue(Source + 32,
                               ContextBlock->ExceptionFrame,
                               ContextBlock->TrapFrame);

    SingleOperand->Sign = Value >> 31;
    SingleOperand->Exponent = (Value >> 23) & 0xff;
    SingleOperand->Mantissa = Value & 0x7fffff;

    //
    // If the exponent is the largest possible value, then the number is
    // either a Nan or an infinity.
    //

    if (SingleOperand->Exponent == SINGLE_MAXIMUM_EXPONENT) {
        if (SingleOperand->Mantissa != 0) {
            SingleOperand->Infinity = FALSE;
            SingleOperand->Nan = TRUE;

        } else {
            SingleOperand->Infinity = TRUE;
            SingleOperand->Nan = FALSE;
        }

    } else {
        SingleOperand->Infinity = FALSE;
        SingleOperand->Nan = FALSE;
        if (SingleOperand->Exponent == SINGLE_MINIMUM_EXPONENT) {
            if (SingleOperand->Mantissa != 0) {
                SingleOperand->Exponent += 1;
                while ((SingleOperand->Mantissa & (1 << 23)) == 0) {
                    SingleOperand->Mantissa <<= 1;
                    SingleOperand->Exponent -= 1;
                }
            }

        } else {
            SingleOperand->Mantissa |= (1 << 23);
        }
    }

    //
    // Left shift the mantissa 2-bits to provide for a guard bit and a round
    // bit.
    //

    SingleOperand->Mantissa <<= 2;
    return;
}
