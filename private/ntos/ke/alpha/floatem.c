/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1993  Digital Equipment Corporation

Module Name:

    floatem.c

Abstract:

    This module implements a software emulation of the IEEE single and
    double floating operations. It is required on Alpha processors since
    the hardware does not fully support all of the operations required
    by the IEEE standard. In particular, infinities and NaNs are not
    handled by the hardware, but rather cause an exception. On receipt
    of the exception, a software emulation of the floating operation
    is performed to determine the real result of the operation and if
    an exception will actually be raised.

    This code is also used to perform all floating operations on EV4
    processors when plus or minus infinity rounding is used.

    Since floating exceptions are rather rare events, this routine is
    written in C. Should a higher performance implementation be required,
    then the algorithms contained herein, can be used to guide a higher
    performance assembly language implementation.

    N.B. This routine does not emulate floating loads, floating stores,
         control to/from floating, or move to/from floating instructions.
         These instructions never require emulation.

    Floating point operations are carried out by unpacking the operands,
    normalizing denormalized numbers, checking for NaNs, interpreting
    infinities, and computing results.

    Floating operands are converted to a format that has a value with the
    appropriate number of leading zeros, an overflow bit, the mantissa, a
    guard bit, a round bit, and a set of sticky bits. The unpacked mantissa
    includes the hidden bit.

    The overflow bit is needed for addition and is also used for multiply.
    The mantissa is 24-bits for single operations and 53-bits for double
    operations. The guard bit and round bit are used to hold precise values
    for normalization and rounding.

    If the result of an operation is normalized, then the guard bit becomes
    the round bit and the round bit is accumulated with the sticky bits. If
    the result of an operation needs to be shifted left one bit for purposes
    of normalization, then the guard bit becomes part of the mantissa and the
    round bit is used for rounding.

    The round bit plus the sticky bits are used to determine how rounding is
    performed.

Author:

    David N. Cutler (davec) 16-Jun-1991

Environment:

    Kernel mode only.

Revision History:

    Thomas Van Baak (tvb) 12-Sep-1992

        Adapted for Alpha AXP.

    Nigel Haslock (haslock) 20-Apr-1995

        Adjustments for additional EV4.5 and EV5 functionality

    Kim Peterson (peterson) 4-Feb-1998

        Corrections for denormal and double precision denormal processing.
        Rounds denormal after shifting it into denormal format.
        Preserves first operand NAN if second operand is not a NAN
        (single precision was already correct).

--*/

#include "ki.h"
#pragma hdrstop
#include "alphaops.h"

#if DBG

extern ULONG RtlDebugFlags;
#define DBGPRINT  ((RtlDebugFlags & 0x4) != 0) && DbgPrint
#define DBGPRINT2 ((RtlDebugFlags & 0x8) != 0) && DbgPrint

#else

#define DBGPRINT  0 && DbgPrint
#define DBGPRINT2 0 && DbgPrint

#endif

#define LOW_PART(Quad) ((ULONG)(Quad))
#define HIGH_PART(Quad) ((ULONG)(Quad >> 32))
#define MAKE_QUAD(Low, High) (((ULONGLONG)(High)) << 32 | ((ULONGLONG)(Low)))

//
// The hardware recognizes the new CVTST instruction by the kludged
// opcode function 16.2ac instead of the proper 16.00e (per ECO #46).
//

#define CVTST_FUNC_PROPER 0x00E

//
// Define unpacked format NaN mask values and boolean macros.
//
// N.B. The NaN bit is set for a quiet NaN and reset for a signaling NaN.
//      This is the same as Intel, Sun, IBM and opposite of Mips, HP.
//

#define DOUBLE_NAN_BIT_HIGH (1 << (53 - 32))
#define SINGLE_NAN_BIT (1 << 24)

#define DoubleSignalNan(DoubleOperand) \
    (((DoubleOperand)->Nan != FALSE) && \
     (((DoubleOperand)->MantissaHigh & DOUBLE_NAN_BIT_HIGH) == 0))

#define DoubleQuietNan(DoubleOperand) \
    (((DoubleOperand)->Nan != FALSE) && \
     (((DoubleOperand)->MantissaHigh & DOUBLE_NAN_BIT_HIGH) != 0))

#define SingleSignalNan(SingleOperand) \
    (((SingleOperand)->Nan != FALSE) && \
     (((SingleOperand)->Mantissa & SINGLE_NAN_BIT) == 0))

#define SingleQuietNan(SingleOperand) \
    (((SingleOperand)->Nan != FALSE) && \
     (((SingleOperand)->Mantissa & SINGLE_NAN_BIT) != 0))

//
// Define context block structure.
//

typedef struct _FP_CONTEXT_BLOCK {
    ULONG Fc;
    PEXCEPTION_RECORD ExceptionRecord;
    PKEXCEPTION_FRAME ExceptionFrame;
    PKTRAP_FRAME TrapFrame;
    PSW_FPCR SoftwareFpcr;
    ULONG Round;
    BOOLEAN IeeeMode;
    BOOLEAN UnderflowEnable;
} FP_CONTEXT_BLOCK, *PFP_CONTEXT_BLOCK;

//
// Define single and double operand value structures.
//

typedef struct _FP_DOUBLE_OPERAND {
    LONG MantissaHigh;
    ULONG MantissaLow;
    LONGLONG Mantissa;                  // ## Not fully used yet
    LONG Exponent;
    LONG Sign;
    BOOLEAN Infinity;
    BOOLEAN Nan;
    BOOLEAN Normal;
} FP_DOUBLE_OPERAND, *PFP_DOUBLE_OPERAND;

typedef struct _FP_SINGLE_OPERAND {
    LONG Mantissa;
    LONG Exponent;
    LONG Sign;
    BOOLEAN Infinity;
    BOOLEAN Nan;
    BOOLEAN Normal;
} FP_SINGLE_OPERAND, *PFP_SINGLE_OPERAND;

//
// Define single and double IEEE floating point memory formats.
//

typedef struct _DOUBLE_FORMAT {
    ULONGLONG Mantissa : 52;
    ULONGLONG Exponent : 11;
    ULONGLONG Sign : 1;
} DOUBLE_FORMAT, *PDOUBLE_FORMAT;

typedef struct _SINGLE_FORMAT {
    ULONG Mantissa : 23;
    ULONG Exponent : 8;
    ULONG Sign : 1;
} SINGLE_FORMAT, *PSINGLE_FORMAT;

//
// Define forward referenced function prototypes.
//

ULONGLONG
KiConvertSingleOperandToRegister (
    IN ULONG SingleValue
    );

ULONG
KiConvertRegisterToSingleOperand (
    IN ULONGLONG DoubleValue
    );

BOOLEAN
KiConvertQuadwordToLongword (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN LONGLONG Quadword
    );

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

PFP_IEEE_VALUE
KiInitializeIeeeValue (
    IN PEXCEPTION_RECORD ExceptionRecord
    );

BOOLEAN
KiInvalidCompareDouble (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN BOOLEAN CheckForSignalNan,
    IN PFP_DOUBLE_OPERAND DoubleOperand1,
    IN PFP_DOUBLE_OPERAND DoubleOperand2
    );

BOOLEAN
KiInvalidOperationDouble (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN BOOLEAN CheckForSignalNan,
    IN PFP_DOUBLE_OPERAND DoubleOperand1,
    IN PFP_DOUBLE_OPERAND DoubleOperand2
    );

BOOLEAN
KiInvalidOperationQuadword (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN ULONGLONG ResultValue
    );

BOOLEAN
KiInvalidOperationSingle (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN BOOLEAN CheckForSignalNan,
    IN PFP_SINGLE_OPERAND SingleOperand1,
    IN PFP_SINGLE_OPERAND SingleOperand2
    );

BOOLEAN
KiNormalizeDouble (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN PFP_DOUBLE_OPERAND ResultOperand,
    IN ULONGLONG StickyBits
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
    IN OUT PKTRAP_FRAME TrapFrame,
    IN OUT PSW_FPCR SoftwareFpcr
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

    SoftwareFpcr - Supplies a pointer to a variable that contains a copy of
        the software FPCR.

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
    BOOLEAN CompareLess;
    BOOLEAN CompareResult;
    FP_CONTEXT_BLOCK ContextBlock;
    LARGE_INTEGER DoubleDividend;
    LARGE_INTEGER DoubleDivisor;
    ULONG DoubleMantissaLow;
    LONG DoubleMantissaHigh;
    FP_DOUBLE_OPERAND DoubleOperand1;
    FP_DOUBLE_OPERAND DoubleOperand2;
    FP_DOUBLE_OPERAND DoubleOperand3;
    LARGE_INTEGER DoubleQuotient;
    PVOID ExceptionAddress;
    ULONG ExponentDifference;
    ULONG Fa;
    ULONG Fb;
    ULONG Function;
    ULONG Index;
    ALPHA_INSTRUCTION Instruction;
    ULARGE_INTEGER LargeResult;
    LONG Negation;
    LONGLONG Quadword;
    LONG SingleMantissa;
    FP_SINGLE_OPERAND SingleOperand1;
    FP_SINGLE_OPERAND SingleOperand2;
    FP_SINGLE_OPERAND SingleOperand3;
    ULONG StickyBits;
    BOOLEAN ValidOperation;

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
        // Fetch the faulting or trapping instruction. Check the opcode and
        // function code (including the trap enable bits) for IEEE floating
        // point operations that are expected to be emulated.
        //
        // N.B. Only a subset of the 2048 possible combinations of 11 bits
        //      in the function field are valid. A total of 88 functions
        //      are affected by missing plus and minus infinity rounding
        //      mode support in the EV4 chip.
        //

        Instruction = *((PALPHA_INSTRUCTION)ExceptionRecord->ExceptionAddress);
        DBGPRINT("KiEmulateFloating: Instruction = %.8lx, Fpcr = %.16Lx\n",
                 Instruction.Long, TrapFrame->Fpcr);
        Function = Instruction.FpOp.Function;

        ValidOperation = FALSE;
        if (Instruction.FpOp.Opcode == IEEEFP_OP) {

            //
            // Adjust the function code if the instruction is CVTST.
            //

            if (Function == CVTST_FUNC) {
                Function = CVTST_FUNC_PROPER;

            } else if (Function == CVTST_S_FUNC) {
                Function = CVTST_FUNC_PROPER | FP_TRAP_ENABLE_S;
            }

            switch (Function & FP_FUNCTION_MASK) {
            case ADDS_FUNC :
            case SUBS_FUNC :
            case MULS_FUNC :
            case DIVS_FUNC :
            case ADDT_FUNC :
            case SUBT_FUNC :
            case MULT_FUNC :
            case DIVT_FUNC :
            case CVTTQ_FUNC :
            case CVTTS_FUNC :

                switch (Function & FP_TRAP_ENABLE_MASK) {
                case FP_TRAP_ENABLE_NONE :
                case FP_TRAP_ENABLE_U :
                case FP_TRAP_ENABLE_SU :
                case FP_TRAP_ENABLE_SUI :

                    ValidOperation = TRUE;
                    break;
                }
                break;

            case CVTQS_FUNC :
            case CVTQT_FUNC :

                switch (Function & FP_TRAP_ENABLE_MASK) {
                case FP_TRAP_ENABLE_NONE :
                case FP_TRAP_ENABLE_SUI :

                    ValidOperation = TRUE;
                    break;
                }
                break;

            case CVTST_FUNC_PROPER :

                switch (Function & FP_TRAP_ENABLE_MASK) {
                case FP_TRAP_ENABLE_NONE :
                case FP_TRAP_ENABLE_S :

                    ValidOperation = TRUE;
                    break;
                }
                break;

            case CMPTEQ_FUNC :
            case CMPTLE_FUNC :
            case CMPTLT_FUNC :
            case CMPTUN_FUNC :

                ValidOperation = TRUE;
                break;
            }

        } else if (Instruction.FpOp.Opcode == FPOP_OP) {
            switch (Function) {
            case CVTLQ_FUNC :
            case CVTQL_FUNC :
            case CVTQLV_FUNC :
            case CVTQLSV_FUNC :

                ValidOperation = TRUE;
                break;
            }
        }

        if (ValidOperation == FALSE) {

            //
            // An illegal instruction, function code, format value, or trap
            // enable value was encountered. Generate an illegal instruction
            // exception.
            //

            ExceptionRecord->ExceptionCode = STATUS_ILLEGAL_INSTRUCTION;
            DBGPRINT("KiEmulateFloating: Invalid Function or Format\n");
            return FALSE;
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
        // N.B. The SoftwareFpcr and IEEE exception records are only used
        //      with IEEE mode instructions.
        //

        ContextBlock.ExceptionRecord = ExceptionRecord;
        ContextBlock.ExceptionFrame = ExceptionFrame;
        ContextBlock.TrapFrame = TrapFrame;
        ContextBlock.SoftwareFpcr = SoftwareFpcr;

        //
        // Check if the /S bit is set in the instruction. This bit is always
        // set in the case of a trigger instruction of an asynchronous trap
        // (assuming valid trap shadow) but not necessarily always set in the
        // case of an unimplemented floating instruction fault.
        //

        if ((Function & FP_TRAP_ENABLE_S) != 0) {
            ContextBlock.IeeeMode = TRUE;

        } else {
            ContextBlock.IeeeMode = FALSE;
        }

        if ((Function & FP_TRAP_ENABLE_U) != 0) {
            ContextBlock.UnderflowEnable = TRUE;

        } else {
            ContextBlock.UnderflowEnable = FALSE;
        }

        //
        // Set the current rounding mode from the rounding mode specified in
        // the instruction, or if dynamic rounding is specified, from the
        // rounding mode specified in the FPCR.
        // Set the emulation flag and emulate the floating point operation.
        // The return value is dependent on the results of the emulation.
        //

        ContextBlock.Fc = Instruction.FpOp.Fc;
        Fa = Instruction.FpOp.Fa;
        Fb = Instruction.FpOp.Fb;

        if ((Function & FP_ROUND_MASK) == FP_ROUND_D) {
            ContextBlock.Round = ((PFPCR)&TrapFrame->Fpcr)->DynamicRoundingMode;

        } else {
            ContextBlock.Round = (Function & FP_ROUND_MASK) >> FP_ROUND_SHIFT;
        }

        SoftwareFpcr->EmulationOccurred = 1;

        //
        // Unpack operands and dispense with NaNs.
        //

        switch (Function & FP_FUNCTION_MASK) {
        case ADDS_FUNC :
        case SUBS_FUNC :
        case MULS_FUNC :
        case DIVS_FUNC :

            //
            // The function has two single operand values.
            //

            KiUnpackSingle(Fa, &ContextBlock, &SingleOperand1);
            KiUnpackSingle(Fb, &ContextBlock, &SingleOperand2);

            //
            // Non-IEEE mode operate instructions trap on NaN, infinity, or
            // denormal operands.
            //

            if ((ContextBlock.IeeeMode == FALSE) &&
                ((SingleOperand1.Normal == FALSE) ||
                 (SingleOperand2.Normal == FALSE))) {
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
                return FALSE;
            }

            if ((SingleOperand1.Nan != FALSE) || (SingleOperand2.Nan != FALSE)) {

                //
                // Store a quiet NaN if the invalid operation trap
                // is disabled, or raise an exception if the invalid
                // operation trap is enabled and either of the NaNs
                // is a signaling NaN.
                //

                return KiInvalidOperationSingle(&ContextBlock,
                                                TRUE,
                                                &SingleOperand1,
                                                &SingleOperand2);
            }
            break;

        case ADDT_FUNC :
        case SUBT_FUNC :
        case MULT_FUNC :
        case DIVT_FUNC :

            //
            // The function has two double operand values.
            //

            KiUnpackDouble(Fa, &ContextBlock, &DoubleOperand1);
            KiUnpackDouble(Fb, &ContextBlock, &DoubleOperand2);

            //
            // Non-IEEE mode operate instructions trap on NaN, infinity, or
            // denormal operands.
            //

            if ((ContextBlock.IeeeMode == FALSE) &&
                ((DoubleOperand1.Normal == FALSE) ||
                 (DoubleOperand2.Normal == FALSE))) {
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
                return FALSE;
            }
            if ((DoubleOperand1.Nan != FALSE) || (DoubleOperand2.Nan != FALSE)) {

                //
                // Store a quiet NaN if the invalid operation trap
                // is disabled, or raise an exception if the invalid
                // operation trap is enabled and either of the NaNs
                // is a signaling NaN.
                //

                return KiInvalidOperationDouble(&ContextBlock,
                                                TRUE,
                                                &DoubleOperand1,
                                                &DoubleOperand2);
            }
            break;

        case CMPTEQ_FUNC :
        case CMPTLE_FUNC :
        case CMPTLT_FUNC :
        case CMPTUN_FUNC :

            //
            // The function has two double operand values.
            //

            KiUnpackDouble(Fa, &ContextBlock, &DoubleOperand1);
            KiUnpackDouble(Fb, &ContextBlock, &DoubleOperand2);

            //
            // Non-IEEE mode compare instructions trap on NaN or denormal
            // operands.
            //

            if ((ContextBlock.IeeeMode == FALSE) &&
                (((DoubleOperand1.Normal == FALSE) &&
                  (DoubleOperand1.Infinity == FALSE)) ||
                 ((DoubleOperand2.Normal == FALSE) &&
                  (DoubleOperand2.Infinity == FALSE)))) {
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
                return FALSE;
            }

            //
            // Compare operation.
            //
            // If either operand is a NaN, then check the type of compare
            // operation to determine the result value and if an exception
            // should be raised. Otherwise, if the operation is a compare
            // unordered operation, store a false result.
            //

            if ((DoubleOperand1.Nan != FALSE) || (DoubleOperand2.Nan != FALSE)) {

                //
                // If the compare is an unordered compare, then store a true
                // result (a NaN compares unordered with everything, including
                // itself). Raise an exception if the invalid operation trap
                // is enabled and either of the NaNs is a signaling NaN.
                //
                // Otherwise, if the operation is compare equal, then store a
                // false result. Raise an exception if the invalid operation
                // trap is enabled and either of the NaNs is a signaling NaN.
                //
                // Otherwise store a false result and raise an exception if
                // the invalid operation trap is enabled.
                //

                if ((Function & FP_FUNCTION_MASK) == CMPTUN_FUNC) {
                    KiSetRegisterValue(ContextBlock.Fc + 32,
                                       FP_COMPARE_TRUE,
                                       ExceptionFrame,
                                       TrapFrame);

                    return KiInvalidCompareDouble(&ContextBlock,
                                                  TRUE,
                                                  &DoubleOperand1,
                                                  &DoubleOperand2);

                } else if ((Function & FP_FUNCTION_MASK) == CMPTEQ_FUNC) {
                    KiSetRegisterValue(ContextBlock.Fc + 32,
                                       FP_COMPARE_FALSE,
                                       ExceptionFrame,
                                       TrapFrame);

                    return KiInvalidCompareDouble(&ContextBlock,
                                                  TRUE,
                                                  &DoubleOperand1,
                                                  &DoubleOperand2);

                } else {
                    KiSetRegisterValue(ContextBlock.Fc + 32,
                                       FP_COMPARE_FALSE,
                                       ExceptionFrame,
                                       TrapFrame);

                    return KiInvalidCompareDouble(&ContextBlock,
                                                  FALSE,
                                                  &DoubleOperand1,
                                                  &DoubleOperand2);
                }

            } else {
                if ((Function & FP_FUNCTION_MASK) == CMPTUN_FUNC) {
                    KiSetRegisterValue(ContextBlock.Fc + 32,
                                       FP_COMPARE_FALSE,
                                       ExceptionFrame,
                                       TrapFrame);

                    return TRUE;
                }
            }
            break;

        case CVTST_FUNC_PROPER :

            //
            // The function has one single operand value which is found in
            // the second operand.
            //

            KiUnpackSingle(Fb, &ContextBlock, &SingleOperand1);

            //
            // Non-IEEE mode convert instructions trap on NaN, infinity, or
            // denormal operands.
            //

            if ((ContextBlock.IeeeMode == FALSE) &&
                (SingleOperand1.Normal == FALSE)) {
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
                return FALSE;
            }
            break;

        case CVTTQ_FUNC :
        case CVTTS_FUNC :

            //
            // The function has one double operand value which is found in
            // the second operand.
            //

            KiUnpackDouble(Fb, &ContextBlock, &DoubleOperand1);

            //
            // Non-IEEE mode convert instructions trap on NaN, infinity, or
            // denormal operands.
            //

            if ((ContextBlock.IeeeMode == FALSE) &&
                (DoubleOperand1.Normal == FALSE)) {
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
                return FALSE;
            }
            break;

        case CVTLQ_FUNC :
        case CVTQL_FUNC :
        case CVTQS_FUNC :
        case CVTQT_FUNC :

            //
            // The function has one quadword operand value which is found in
            // the second operand.
            //

            Quadword = KiGetRegisterValue(Fb + 32,
                                          ContextBlock.ExceptionFrame,
                                          ContextBlock.TrapFrame);
            break;
        }

        //
        // Case to the proper function routine to emulate the operation.
        //

        Negation = 0;
        switch (Function & FP_FUNCTION_MASK) {

        //
        // Floating subtract operation.
        //
        // Floating subtract is accomplished by complementing the sign
        // of the second operand and then performing an add operation.
        //

        case SUBS_FUNC :
            DBGPRINT2("subs\n");
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
        // are equal), it is possible that the result of the subtract
        // could be negative (if the exponents are equal). If this
        // occurs, then the result sign and mantissa are complemented
        // to obtain the correct result.
        //

        case ADDS_FUNC :
            DBGPRINT2("adds\n");

            //
            // Complement the sign of the second operand if the operation
            // is subtraction.
            //

            SingleOperand2.Sign ^= Negation;

            //
            // Reorder the operands according to their exponent value
            // so that Operand1 exponent will be >= Operand2 exponent.
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
            // operation by adding the values together. Otherwise, if the
            // operands are not infinity, perform the operation by
            // subtracting the second operand from the first operand.
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

                    //
                    // If the result is exactly zero and the signs of the
                    // operands differ, then the result is plus zero except
                    // when the rounding mode is minus infinity.
                    //

                    if ((SingleOperand1.Mantissa == 0) && (StickyBits == 0)) {
                        if (ContextBlock.Round == ROUND_TO_MINUS_INFINITY) {
                            SingleOperand1.Sign = 0x1;

                        } else {
                            SingleOperand1.Sign = 0x0;
                        }
                    }
                }
            }

            //
            // Normalize and store the result value.
            //

            return KiNormalizeSingle(&ContextBlock,
                                     &SingleOperand1,
                                     StickyBits);

        case SUBT_FUNC :
            DBGPRINT2("subt\n");
            Negation = 0x1;

        case ADDT_FUNC :
            DBGPRINT2("addt\n");

            //
            // Complement the sign of the second operand if the operation
            // is subtraction.
            //

            DoubleOperand2.Sign ^= Negation;

            //
            // Reorder the operands according to their exponent value
            // so that Operand1 exponent will be >= Operand2 exponent.
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
            // operation by adding the values together. Otherwise, if the
            // operands are not infinity, perform the operation by
            // subtracting the second operand from the first operand.
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
                        DoubleOperand1.MantissaLow = -(LONG)DoubleOperand1.MantissaLow;
                        DoubleOperand1.MantissaHigh = -DoubleOperand1.MantissaHigh;
                        if (DoubleOperand1.MantissaLow != 0) {
                            DoubleOperand1.MantissaHigh -= 1;
                        }
                        DoubleOperand1.Sign ^= 0x1;
                    }

                    //
                    // If the result is exactly zero and the signs of the
                    // operands differ, then the result is plus zero except
                    // when the rounding mode is minus infinity.
                    //

                    if ((DoubleOperand1.MantissaHigh == 0) &&
                        (DoubleOperand1.MantissaLow == 0) &&
                        (StickyBits == 0)) {
                        if (ContextBlock.Round == ROUND_TO_MINUS_INFINITY) {
                            DoubleOperand1.Sign = 0x1;

                        } else {
                            DoubleOperand1.Sign = 0x0;
                        }
                    }
                }
            }

            //
            // Normalize and store the result value.
            //

            return KiNormalizeDouble(&ContextBlock,
                                     &DoubleOperand1,
                                     StickyBits);

        //
        // Floating multiply operation.
        //
        // Floating multiply is accomplished using unsigned multiplies
        // of the mantissa values, and adding the partial results together
        // to form the total product.
        //
        // The two mantissa values are preshifted such that the final
        // result is properly aligned.
        //

        case MULS_FUNC :
            DBGPRINT2("muls\n");

            //
            // Reorder the operands according to their exponent value
            // so that Operand1 exponent will be >= Operand2 exponent.
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
            // The size of the product will be (1+23+2)+(1+23+2) = 52 bits
            // of which the high (1+1+23+2) = 27 bits are result and the
            // remaining 25 bits are sticky. By preshifting the operands
            // left 7 bits, the number of sticky bits is 32. This alignment
            // is convenient.
            //
            // The 7 bit preshift amount must be applied in part to both
            // operands because 26 of 32 bits of the mantissa are used and
            // so neither operand can be safely shifted left by more than 6
            // bits. Thus one operand is shifted the maximum of 6 bits and
            // the other the remaining 1 bit.
            //

            LargeResult.QuadPart = ((ULONGLONG)((ULONG)(SingleOperand1.Mantissa << (32 - 26)))) *
                                   ((ULONGLONG)((ULONG)(SingleOperand2.Mantissa << 1)));

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

        case MULT_FUNC :
            DBGPRINT2("mult\n");

            //
            // Reorder the operands according to their exponent value
            // so that Operand1 exponent will be >= Operand2 exponent.
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
            // The size of the product will be (1+52+2)+(1+52+2) = 110 bits
            // of which the high (1+1+52+2) = 56 bits are result and the
            // remaining 54 bits are sticky. By preshifting the operands
            // left 10 bits, the number of sticky bits is 64. This alignment
            // is convenient.
            //
            // The 10 bit preshift amount must be applied in part to both
            // operands because 55 of 64 bits of the mantissa are used and
            // so neither operand can be safely shifted left by more than 9
            // bits. Thus one operand is shifted the maximum of 9 bits and
            // the other the remaining 1 bit.
            //

            DoubleOperand1.MantissaHigh =
                    (DoubleOperand1.MantissaHigh << 1) |
                            (DoubleOperand1.MantissaLow >> 31);

            DoubleOperand1.MantissaLow <<= 1;
            DoubleOperand2.MantissaHigh =
                    (DoubleOperand2.MantissaHigh << (64 - 55)) |
                            (DoubleOperand2.MantissaLow >> (32 - (64 - 55)));

            DoubleOperand2.MantissaLow <<= (64 - 55);

            //
            // The 128-bit product is formed by multiplying and adding
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

            AhighBhigh.QuadPart = (ULONGLONG)(ULONG)DoubleOperand1.MantissaHigh *
                                  (ULONGLONG)(ULONG)DoubleOperand2.MantissaHigh;

            AhighBlow.QuadPart = (ULONGLONG)(ULONG)DoubleOperand1.MantissaHigh *
                                 (ULONGLONG)DoubleOperand2.MantissaLow;

            AlowBhigh.QuadPart = (ULONGLONG)DoubleOperand1.MantissaLow *
                                 (ULONGLONG)(ULONG)DoubleOperand2.MantissaHigh;

            AlowBlow.QuadPart = (ULONGLONG)DoubleOperand1.MantissaLow *
                                (ULONGLONG)DoubleOperand2.MantissaLow;

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

        case DIVS_FUNC :
            DBGPRINT2("divs\n");

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
                SingleOperand3.Mantissa <<= 1;
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

        case DIVT_FUNC :
            DBGPRINT2("divt\n");

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
                    DoubleDividend.QuadPart = DoubleDividend.QuadPart - DoubleDivisor.QuadPart;
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
            // N.B. The sign of zero is ignored.
            //

        case CMPTEQ_FUNC :
        case CMPTLE_FUNC :
        case CMPTLT_FUNC :

            //
            // If either operand is zero, then set the sign of the operand
            // positive and the exponent to a value less than the minimum
            // denormal number.
            //

            if ((DoubleOperand1.Infinity == FALSE) &&
                (DoubleOperand1.MantissaHigh == 0)) {
                DoubleOperand1.Sign = 0;
                DoubleOperand1.Exponent = -52;
            }

            if ((DoubleOperand2.Infinity == FALSE) &&
                (DoubleOperand2.MantissaHigh == 0)) {
                DoubleOperand2.Sign = 0;
                DoubleOperand2.Exponent = -52;
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
            // Form the condition code result value using the comparison
            // information and the compare function codes.
            //

            switch (Function & FP_FUNCTION_MASK) {
            case CMPTEQ_FUNC :
                CompareResult = CompareEqual;
                DBGPRINT2("cmpteq\n");
                break;

            case CMPTLE_FUNC :
                CompareResult = (CompareLess | CompareEqual);
                DBGPRINT2("cmptle\n");
                break;

            case CMPTLT_FUNC :
                CompareResult = CompareLess;
                DBGPRINT2("cmptlt\n");
                break;
            }

            //
            // Set the result operand to 2.0 if the comparison is true,
            // otherwise store 0.0.
            //

            if (CompareResult != FALSE) {
                KiSetRegisterValue(ContextBlock.Fc + 32,
                                   FP_COMPARE_TRUE,
                                   ExceptionFrame,
                                   TrapFrame);

            } else {
                KiSetRegisterValue(ContextBlock.Fc + 32,
                                   FP_COMPARE_FALSE,
                                   ExceptionFrame,
                                   TrapFrame);
            }
            return TRUE;

        //
        // Floating convert single to double.
        //
        // Floating conversion to double is accomplished by forming a
        // double floating operand and then normalizing and storing
        // the result value.
        //

        case CVTST_FUNC_PROPER :
            DBGPRINT2("cvtst\n");

            //
            // If the operand is a NaN, then store a quiet NaN if the
            // invalid operation trap is disabled, or raise an exception
            // if the invalid operation trap is enabled and the operand
            // is a signaling NaN.
            //

            if (SingleOperand1.Nan != FALSE) {
                DoubleOperand1.MantissaHigh =
                        SingleOperand1.Mantissa >> (26 - (55 - 32));
                DoubleOperand1.MantissaLow =
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

        //
        // Floating convert double to single.
        //
        // Floating conversion to single is accomplished by forming a
        // single floating operand and then normalizing and storing the
        // result value.
        //

        case CVTTS_FUNC :
            DBGPRINT2("cvtts\n");

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

        //
        // Floating convert longword to quadword.
        //
        // Floating conversion from longword to quadword is accomplished by
        // a repositioning of 32 bits of the operand, with sign extension.
        //

        case CVTLQ_FUNC :
            DBGPRINT2("cvtlq\n");

            //
            // Pack floating register longword format into upper 32-bits
            // by keeping bits 63..62 and 58..29, eliminating unused bits
            // 61..59. Then right justify and sign extend the 32 bits into
            // 64 bits.
            //

            Quadword = ((Quadword >> 62) << 62) | ((ULONGLONG)(Quadword << 5) >> 2);
            KiSetRegisterValue(ContextBlock.Fc + 32,
                               Quadword >> 32,
                               ExceptionFrame,
                               TrapFrame);

            return TRUE;

        //
        // Floating convert quadword to longword.
        //
        // Floating conversion from quadword to longword is accomplished by
        // truncating the high order 32 bits of the quadword after checking
        // for overflow.
        //

        case CVTQL_FUNC :
            DBGPRINT2("cvtql\n");

            return KiConvertQuadwordToLongword(&ContextBlock, Quadword);

        //
        // Floating convert quadword to single.
        //
        // Floating conversion to single is accomplished by forming a
        // single floating operand and then normalizing and storing the
        // result value.
        //

        case CVTQS_FUNC :
            DBGPRINT2("cvtqs\n");

            //
            // Compute the sign of the result.
            //

            if (Quadword < 0) {
                SingleOperand1.Sign = 0x1;
                Quadword = -Quadword;

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

            if (Quadword != 0) {
                SingleOperand1.Exponent = SINGLE_EXPONENT_BIAS + 63;
                while (Quadword > 0) {
                    Quadword <<= 1;
                    SingleOperand1.Exponent -= 1;
                }

                SingleOperand1.Mantissa = (LONG)((ULONGLONG)Quadword >> (64 - 26));
                if (Quadword & (((ULONGLONG)1 << (64 - 26)) - 1)) {
                    StickyBits = 1;

                } else {
                    StickyBits = 0;
                }

            } else {
                SingleOperand1.Exponent = 0;
                SingleOperand1.Mantissa = 0;
                StickyBits = 0;
            }

            //
            // Normalize and store the result value.
            //

            return KiNormalizeSingle(&ContextBlock,
                                     &SingleOperand1,
                                     StickyBits);

        //
        // Floating convert quadword to double.
        //
        // Floating conversion to double is accomplished by forming a
        // double floating operand and then normalizing and storing the
        // result value.
        //

        case CVTQT_FUNC :
            DBGPRINT2("cvtqt\n");

            //
            // Compute the sign of the result.
            //

            if (Quadword < 0) {
                DoubleOperand1.Sign = 0x1;
                Quadword = -Quadword;

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

            if (Quadword != 0) {
                DoubleOperand1.Exponent = DOUBLE_EXPONENT_BIAS + 63;
                while (Quadword > 0) {
                    Quadword <<= 1;
                    DoubleOperand1.Exponent -= 1;
                }

                DoubleOperand1.MantissaHigh = (LONG)((ULONGLONG)Quadword >> ((64 - 55) + 32));
                DoubleOperand1.MantissaLow = (LONG)((ULONGLONG)Quadword >> (64 - 55));
                if (Quadword & (((ULONGLONG)1 << (64 - 55)) - 1)) {
                    StickyBits = 1;

                } else {
                    StickyBits = 0;
                }

            } else {
                DoubleOperand1.MantissaHigh = 0;
                DoubleOperand1.MantissaLow = 0;
                DoubleOperand1.Exponent = 0;
                StickyBits = 0;
            }

            //
            // Normalize and store the result value.
            //

            return KiNormalizeDouble(&ContextBlock,
                                     &DoubleOperand1,
                                     StickyBits);

        //
        // Floating convert double to quadword.
        //
        // Floating conversion to quadword is accomplished by forming
        // a quadword value from a double floating value.
        //

        case CVTTQ_FUNC :
            DBGPRINT2("cvttq\n");

            //
            // If the operand is infinite or is a NaN, then store a
            // quiet NaN or an appropriate infinity if the invalid
            // operation trap is disabled, or raise an exception if
            // the invalid trap is enabled.
            //

            if ((DoubleOperand1.Infinity != FALSE) ||
                (DoubleOperand1.Nan != FALSE)) {
                return KiInvalidOperationQuadword(&ContextBlock, 0);
            }

            //
            // Convert double to quadword and store the result value.
            //

            return KiNormalizeQuadword(&ContextBlock, &DoubleOperand1);
        }

    //
    // If an exception occurs, then copy the new exception information to the
    // original exception record and handle the exception.
    //

    } except (KiCopyInformation(ExceptionRecord,
                                (GetExceptionInformation())->ExceptionRecord)) {

        //
        // Preserve the original exception address.
        //

        ExceptionRecord->ExceptionAddress = ExceptionAddress;
        DBGPRINT("KiEmulateFloating: Exception\n");
        return FALSE;
    }

    DBGPRINT("KiEmulateFloating: Invalid Instruction\n");
    return FALSE;
}

ULONGLONG
KiConvertSingleOperandToRegister (
    IN ULONG SingleValue
    )

/*++

Routine Description:

    This function converts a 32-bit single format floating point value to
    the 64-bit, double format used within floating point registers. Alpha
    floating point registers are 64-bits wide and single format values are
    transformed to 64-bits when stored or loaded from memory.

Arguments:

    SingleValue - Supplies the 32-bit single operand value as an integer.

Return Value:

    The 64-bit register format operand value is returned as the function
    value.

--*/

{
    PDOUBLE_FORMAT DoubleFormat;
    ULONGLONG Result;
    PSINGLE_FORMAT SingleFormat;

    SingleFormat = (PSINGLE_FORMAT)&SingleValue;
    DoubleFormat = (PDOUBLE_FORMAT)&Result;

    DoubleFormat->Sign = SingleFormat->Sign;
    DoubleFormat->Mantissa = ((ULONGLONG)SingleFormat->Mantissa) << (52 - 23);
    if (SingleFormat->Exponent == SINGLE_MAXIMUM_EXPONENT) {
        DoubleFormat->Exponent = DOUBLE_MAXIMUM_EXPONENT;

    } else if (SingleFormat->Exponent == SINGLE_MINIMUM_EXPONENT) {
        DoubleFormat->Exponent = DOUBLE_MINIMUM_EXPONENT;

    } else {
        DoubleFormat->Exponent = SingleFormat->Exponent - SINGLE_EXPONENT_BIAS +
                                 DOUBLE_EXPONENT_BIAS;
    }
    return Result;
}

ULONG
KiConvertRegisterToSingleOperand (
    IN ULONGLONG DoubleValue
    )

/*++

Routine Description:

    This function converts the 64-bit, double format floating point value
    used within the floating point registers to a 32-bit, single format
    floating point value.

Arguments:

    DoubleValue - Supplies the 64-bit double operand value as an integer.

Return Value:

    The 32-bit register format operand value is returned as the function
    value.

--*/

{
    PDOUBLE_FORMAT DoubleFormat;
    ULONG Result;
    PSINGLE_FORMAT SingleFormat;

    SingleFormat = (PSINGLE_FORMAT)&Result;
    DoubleFormat = (PDOUBLE_FORMAT)&DoubleValue;

    SingleFormat->Sign = (ULONG)DoubleFormat->Sign;
    SingleFormat->Mantissa = (ULONG)(DoubleFormat->Mantissa >> (52 - 23));
    if (DoubleFormat->Exponent == DOUBLE_MAXIMUM_EXPONENT) {
        SingleFormat->Exponent = SINGLE_MAXIMUM_EXPONENT;

    } else if (DoubleFormat->Exponent == DOUBLE_MINIMUM_EXPONENT) {
        SingleFormat->Exponent = SINGLE_MINIMUM_EXPONENT;

    } else {
        SingleFormat->Exponent = (ULONG)(DoubleFormat->Exponent - DOUBLE_EXPONENT_BIAS +
                                         SINGLE_EXPONENT_BIAS);
    }
    return Result;
}

BOOLEAN
KiConvertQuadwordToLongword (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN LONGLONG Quadword
    )

/*++

Routine Description:

    This function is called to convert a quadword operand to a longword
    result.

Arguments:

    ContextBlock - Supplies a pointer to the emulation context block.

    Operand - Supplies the quadword operand value.

Return Value:

    If the quadword value would overflow the longword result and the invalid
    trap is enabled then a value of FALSE is returned. Otherwise, the quadword
    is truncated to a longword and a value of TRUE is returned.

--*/

{
    PEXCEPTION_RECORD ExceptionRecord;
    PFPCR Fpcr;
    PFP_IEEE_VALUE IeeeValue;
    ULONGLONG ResultValue;
    PSW_FPCR SoftwareFpcr;

    //
    // Truncate the quadword to a longword and convert the longword integer
    // to floating register longword integer format.
    //

    ResultValue = ((Quadword & (ULONGLONG)0xc0000000) << 32) |
                  ((Quadword & (ULONGLONG)0x3fffffff) << 29);

    //
    // Check to determine if an exception should be delivered or the result
    // should be written to the destination register.
    //

    if ((Quadword < (LONG)0x80000000) || (Quadword > (LONG)0x7fffffff)) {
        Fpcr = (PFPCR)&ContextBlock->TrapFrame->Fpcr;
        Fpcr->InvalidOperation = 1;
        Fpcr->SummaryBit = 1;
        if (ContextBlock->IeeeMode == FALSE) {
            ExceptionRecord = ContextBlock->ExceptionRecord;
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
            return FALSE;
        }
        SoftwareFpcr = ContextBlock->SoftwareFpcr;
        SoftwareFpcr->StatusInvalid = 1;
        if (SoftwareFpcr->EnableInvalid != 0) {
            ExceptionRecord = ContextBlock->ExceptionRecord;
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
            IeeeValue = KiInitializeIeeeValue(ExceptionRecord);
            IeeeValue->Value.U64Value.LowPart = LOW_PART(ResultValue);
            IeeeValue->Value.U64Value.HighPart = HIGH_PART(ResultValue);
            return FALSE;
        }

        Fpcr->DisableInvalid = 1;
    }

    //
    // Set the destination register value and return a value of TRUE.
    //

    KiSetRegisterValue(ContextBlock->Fc + 32,
                       ResultValue,
                       ContextBlock->ExceptionFrame,
                       ContextBlock->TrapFrame);
    return TRUE;
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
    then a value of FALSE is returned. Otherwise, a quiet NaN or a properly
    signed infinity is stored as the destination result and a value of TRUE
    is returned.

--*/

{

    PEXCEPTION_RECORD ExceptionRecord;
    PFPCR Fpcr;
    PFP_IEEE_VALUE IeeeValue;
    ULONG ResultSign;
    ULONG ResultValueHigh;
    ULONG ResultValueLow;
    PSW_FPCR SoftwareFpcr;

    //
    // The result value is a properly signed infinity.
    //

    ResultSign = DoubleOperand1->Sign ^ DoubleOperand2->Sign;
    ResultValueHigh = DOUBLE_INFINITY_VALUE_HIGH | (ResultSign << 31);
    ResultValueLow = DOUBLE_INFINITY_VALUE_LOW;

    //
    // If the first operand is not infinite and the divide by zero trap is
    // enabled, then store the proper exception code and exception flags
    // and return a value of FALSE. Otherwise, store the appropriately signed
    // infinity and return a value of TRUE.
    //

    if (DoubleOperand1->Infinity == FALSE) {

        Fpcr = (PFPCR)&ContextBlock->TrapFrame->Fpcr;
        Fpcr->DivisionByZero = 1;
        Fpcr->SummaryBit = 1;
        if (ContextBlock->IeeeMode == FALSE) {
            ExceptionRecord = ContextBlock->ExceptionRecord;
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_DIVIDE_BY_ZERO;
            return FALSE;
        }
        SoftwareFpcr = ContextBlock->SoftwareFpcr;
        SoftwareFpcr->StatusDivisionByZero = 1;
        if (SoftwareFpcr->EnableDivisionByZero != 0) {
            ExceptionRecord = ContextBlock->ExceptionRecord;
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_DIVIDE_BY_ZERO;
            IeeeValue = KiInitializeIeeeValue(ExceptionRecord);
            IeeeValue->Value.Fp64Value.W[0] = ResultValueLow;
            IeeeValue->Value.Fp64Value.W[1] = ResultValueHigh;
            return FALSE;
        }

        Fpcr->DisableDivisionByZero = 1;
    }

    KiSetRegisterValue(ContextBlock->Fc + 32,
                       MAKE_QUAD(ResultValueLow, ResultValueHigh),
                       ContextBlock->ExceptionFrame,
                       ContextBlock->TrapFrame);

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
    then a value of FALSE is returned. Otherwise, a quiet NaN or a properly
    signed infinity is stored as the destination result and a value of TRUE
    is returned.

--*/

{

    PEXCEPTION_RECORD ExceptionRecord;
    PFPCR Fpcr;
    PFP_IEEE_VALUE IeeeValue;
    ULONG ResultSign;
    ULONG ResultValue;
    PSW_FPCR SoftwareFpcr;

    //
    // The result value is a properly signed infinity.
    //

    ResultSign = SingleOperand1->Sign ^ SingleOperand2->Sign;
    ResultValue = SINGLE_INFINITY_VALUE | (ResultSign << 31);

    //
    // If the first operand is not infinite and the divide by zero trap is
    // enabled, then store the proper exception code and exception flags
    // and return a value of FALSE. Otherwise, store the appropriately signed
    // infinity and return a value of TRUE.
    //

    if (SingleOperand1->Infinity == FALSE) {

        Fpcr = (PFPCR)&ContextBlock->TrapFrame->Fpcr;
        Fpcr->DivisionByZero = 1;
        Fpcr->SummaryBit = 1;
        if (ContextBlock->IeeeMode == FALSE) {
            ExceptionRecord = ContextBlock->ExceptionRecord;
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_DIVIDE_BY_ZERO;
            return FALSE;
        }
        SoftwareFpcr = ContextBlock->SoftwareFpcr;
        SoftwareFpcr->StatusDivisionByZero = 1;
        if (SoftwareFpcr->EnableDivisionByZero != 0) {
            ExceptionRecord = ContextBlock->ExceptionRecord;
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_DIVIDE_BY_ZERO;
            IeeeValue = KiInitializeIeeeValue(ExceptionRecord);
            IeeeValue->Value.Fp32Value.W[0] = ResultValue;
            return FALSE;
        }

        Fpcr->DisableDivisionByZero = 1;
    }

    KiSetRegisterValue(ContextBlock->Fc + 32,
                       KiConvertSingleOperandToRegister(ResultValue),
                       ContextBlock->ExceptionFrame,
                       ContextBlock->TrapFrame);

    return TRUE;
}

PFP_IEEE_VALUE
KiInitializeIeeeValue (
    IN PEXCEPTION_RECORD ExceptionRecord
    )

/*++

Routine Description:

    This function is called to initialize an IEEE exception record.

    N.B. The original hardware exception record should be overwritten with an
         IEEE exception record only when it is known for certain that an IEEE
         exception must be generated.

Arguments:

    ExceptionRecord - Supplies a pointer to the exception record.

Return Value:

    The address of the IEEE value portion of the exception record is returned
    as the function value.

--*/

{

    //
    // Initialize the number of exception information parameters, zero
    // the first parameter to indicate a hardware initiated exception,
    // set the continuation address, and clear the IEEE exception value.
    //

    ExceptionRecord->NumberParameters = 6;
    ExceptionRecord->ExceptionInformation[0] = 0;
    ExceptionRecord->ExceptionInformation[1] =
        ((ULONG_PTR)(ExceptionRecord)->ExceptionAddress) + 4;
    ExceptionRecord->ExceptionInformation[2] = 0;
    ExceptionRecord->ExceptionInformation[3] = 0;
    ExceptionRecord->ExceptionInformation[4] = 0;
    ExceptionRecord->ExceptionInformation[5] = 0;

    //
    // Return address of IEEE exception value.
    //

    return (PFP_IEEE_VALUE)&ExceptionRecord->ExceptionInformation[2];
}

BOOLEAN
KiInvalidCompareDouble (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN BOOLEAN CheckForSignalNan,
    IN PFP_DOUBLE_OPERAND DoubleOperand1,
    IN PFP_DOUBLE_OPERAND DoubleOperand2
    )

/*++

Routine Description:

    This function is called to determine whether an invalid operation
    exception should be raised for a double compare operation.

Arguments:

    ContextBlock - Supplies a pointer to the emulation context block.

    CheckForSignalNan - Supplies a boolean value that determines whether the
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
    PFPCR Fpcr;
    PFP_IEEE_VALUE IeeeValue;
    PSW_FPCR SoftwareFpcr;

    //
    // If an invalid operation is specified or one of the operands is a
    // signaling NaN and the invalid operation trap is enabled, then
    // store the proper exception code and exception flags and return
    // a value of FALSE. Otherwise, perform no operation and return a
    // value of TRUE.
    //

    if ((CheckForSignalNan == FALSE) ||
        (DoubleSignalNan(DoubleOperand1) != FALSE) ||
        (DoubleSignalNan(DoubleOperand2) != FALSE)) {

        Fpcr = (PFPCR)&ContextBlock->TrapFrame->Fpcr;
        Fpcr->InvalidOperation = 1;
        Fpcr->SummaryBit = 1;
        if (ContextBlock->IeeeMode == FALSE) {
            ExceptionRecord = ContextBlock->ExceptionRecord;
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
            return FALSE;
        }
        SoftwareFpcr = ContextBlock->SoftwareFpcr;
        SoftwareFpcr->StatusInvalid = 1;
        if (SoftwareFpcr->EnableInvalid != 0) {
            ExceptionRecord = ContextBlock->ExceptionRecord;
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
            IeeeValue = KiInitializeIeeeValue(ExceptionRecord);
            IeeeValue->Value.CompareValue = FpCompareUnordered;
            return FALSE;
        }

        Fpcr->DisableInvalid = 1;
    }

    return TRUE;
}

BOOLEAN
KiInvalidOperationDouble (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN BOOLEAN CheckForSignalNan,
    IN PFP_DOUBLE_OPERAND DoubleOperand1,
    IN PFP_DOUBLE_OPERAND DoubleOperand2
    )

/*++

Routine Description:

    This function is called to either raise an exception or store a
    quiet NaN for an invalid double floating operation.

Arguments:

    ContextBlock - Supplies a pointer to the emulation context block.

    CheckForSignalNan - Supplies a boolean value that determines whether the
        operand values should be checked for a signaling NaN.

    DoubleOperand1 - Supplies a pointer to the first operand value.

    DoubleOperand2 - Supplies a pointer ot the second operand value.

Return Value:

    If the invalid operation trap is enabled and either the operation is
    invalid or one of the operands in a signaling NaN, then a value of
    FALSE is returned. Otherwise, a quiet NaN is stored as the destination
    result and a value of TRUE is returned.

--*/

{

    PEXCEPTION_RECORD ExceptionRecord;
    PFPCR Fpcr;
    PFP_IEEE_VALUE IeeeValue;
    ULONG ResultValueHigh;
    ULONG ResultValueLow;
    PSW_FPCR SoftwareFpcr;

    //
    // If the second operand is a NaN, then compute a quiet NaN from its
    // value. Otherwise, if the first operand is a NaN, then compute a
    // quiet NaN from its value. Otherwise, the result value is a quiet
    // (real indefinite) NaN.
    //

    DBGPRINT("Operand1: Inf=%d NaN=%d Sign=%d Exponent=%d Mantissa=%.8x%.8x\n",
             DoubleOperand1->Infinity, DoubleOperand1->Nan,
             DoubleOperand1->Sign,
             DoubleOperand1->Exponent,
             DoubleOperand1->MantissaHigh, DoubleOperand1->MantissaLow);
    DBGPRINT("Operand2: Inf=%d NaN=%d Sign=%d Exponent=%d Mantissa=%.8x%.8x\n",
             DoubleOperand2->Infinity, DoubleOperand2->Nan,
             DoubleOperand2->Sign,
             DoubleOperand2->Exponent,
             DoubleOperand2->MantissaHigh, DoubleOperand2->MantissaLow);

    if (DoubleOperand2->Nan != FALSE) {
        ResultValueLow = DoubleOperand2->MantissaLow >> 2;
        ResultValueLow |= DoubleOperand2->MantissaHigh << 30;
        ResultValueHigh = DoubleOperand2->MantissaHigh >> 2;
        ResultValueHigh |= DOUBLE_QUIET_NAN_PREFIX_HIGH;
        ResultValueHigh |= DoubleOperand2->Sign << 31;

    } else if (DoubleOperand1->Nan != FALSE) {
        ResultValueLow = DoubleOperand1->MantissaLow >> 2;
        ResultValueLow |= DoubleOperand1->MantissaHigh << 30;
        ResultValueHigh = DoubleOperand1->MantissaHigh >> 2;
        ResultValueHigh |= DOUBLE_QUIET_NAN_PREFIX_HIGH;
        ResultValueHigh |= DoubleOperand1->Sign << 31;

    } else {
        ResultValueLow = DOUBLE_QUIET_NAN_VALUE_LOW;
        ResultValueHigh = DOUBLE_QUIET_NAN_VALUE_HIGH;
    }

    //
    // If an invalid operation is specified or one of the operands is a
    // signaling NaN and the invalid operation trap is enabled, then
    // store the proper exception code and exception flags and return
    // a value of FALSE. Otherwise, store a quiet NaN as the destination
    // result and return a value of TRUE.
    //

    if ((CheckForSignalNan == FALSE) ||
        (DoubleSignalNan(DoubleOperand1) != FALSE) ||
        (DoubleSignalNan(DoubleOperand2) != FALSE)) {

        Fpcr = (PFPCR)&ContextBlock->TrapFrame->Fpcr;
        Fpcr->InvalidOperation = 1;
        Fpcr->SummaryBit = 1;
        if (ContextBlock->IeeeMode == FALSE) {
            ExceptionRecord = ContextBlock->ExceptionRecord;
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
            return FALSE;
        }
        SoftwareFpcr = ContextBlock->SoftwareFpcr;
        SoftwareFpcr->StatusInvalid = 1;
        if (SoftwareFpcr->EnableInvalid != 0) {
            ExceptionRecord = ContextBlock->ExceptionRecord;
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
            IeeeValue = KiInitializeIeeeValue(ExceptionRecord);
            IeeeValue->Value.Fp64Value.W[0] = ResultValueLow;
            IeeeValue->Value.Fp64Value.W[1] = ResultValueHigh;
            return FALSE;
        }

        Fpcr->DisableInvalid = 1;
    }

    KiSetRegisterValue(ContextBlock->Fc + 32,
                       MAKE_QUAD(ResultValueLow, ResultValueHigh),
                       ContextBlock->ExceptionFrame,
                       ContextBlock->TrapFrame);

    return TRUE;
}

BOOLEAN
KiInvalidOperationQuadword (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN ULONGLONG ResultValue
    )

/*++

Routine Description:

    This function is called to either raise an exception or store a
    quiet NaN for an invalid conversion to quadword.

Arguments:

    ContextBlock - Supplies a pointer to the emulation context block.

    ResultValue - Suplies a quadword result value to be stored.

Return Value:

    If the invalid operation trap is enabled, then a value of FALSE is
    returned. Otherwise, an appropriate quadword value is stored as the
    destination result and a value of TRUE is returned.

--*/

{

    PEXCEPTION_RECORD ExceptionRecord;
    PFPCR Fpcr;
    PFP_IEEE_VALUE IeeeValue;
    PSW_FPCR SoftwareFpcr;

    //
    // If the invalid operation trap is enabled then store the proper
    // exception code and exception flags and return a value of FALSE.
    // Otherwise, store a quiet NaN as the destination result and return
    // a value of TRUE.
    //

    Fpcr = (PFPCR)&ContextBlock->TrapFrame->Fpcr;
    Fpcr->InvalidOperation = 1;
    Fpcr->SummaryBit = 1;
    if (ContextBlock->IeeeMode == FALSE) {
        ExceptionRecord = ContextBlock->ExceptionRecord;
        ExceptionRecord->ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
        return FALSE;
    }
    SoftwareFpcr = ContextBlock->SoftwareFpcr;
    SoftwareFpcr->StatusInvalid = 1;
    if (SoftwareFpcr->EnableInvalid != 0) {
        ExceptionRecord = ContextBlock->ExceptionRecord;
        ExceptionRecord->ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
        IeeeValue = KiInitializeIeeeValue(ExceptionRecord);
        IeeeValue->Value.U64Value.LowPart = LOW_PART(ResultValue);
        IeeeValue->Value.U64Value.HighPart = HIGH_PART(ResultValue);
        return FALSE;
    }

    Fpcr->DisableInvalid = 1;

    KiSetRegisterValue(ContextBlock->Fc + 32,
                       ResultValue,
                       ContextBlock->ExceptionFrame,
                       ContextBlock->TrapFrame);

    return TRUE;
}

BOOLEAN
KiInvalidOperationSingle (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN BOOLEAN CheckForSignalNan,
    IN PFP_SINGLE_OPERAND SingleOperand1,
    IN PFP_SINGLE_OPERAND SingleOperand2
    )

/*++

Routine Description:

    This function is called to either raise an exception or store a
    quiet NaN for an invalid single floating operation.

Arguments:

    ContextBlock - Supplies a pointer to the emulation context block.

    CheckForSignalNan - Supplies a boolean value that determines whether the
        operand values should be checked for a signaling NaN.

    SingleOperand1 - Supplies a pointer to the first operand value.

    SingleOperand2 - Supplies a pointer ot the second operand value.

Return Value:

    If the invalid operation trap is enabled and either the operation is
    invalid or one of the operands in a signaling NaN, then a value of
    FALSE is returned. Otherwise, a quiet NaN is stored as the destination
    result and a value of TRUE is returned.

--*/

{

    PEXCEPTION_RECORD ExceptionRecord;
    PFPCR Fpcr;
    PFP_IEEE_VALUE IeeeValue;
    ULONG ResultValue;
    PSW_FPCR SoftwareFpcr;

    //
    // If the second operand is a NaN, then compute a quiet NaN from its
    // value. Otherwise, if the first operand is a NaN, then compute a
    // quiet NaN from its value. Otherwise, the result value is a quiet
    // (real indefinite) NaN.
    //

    if (SingleOperand2->Nan != FALSE) {
        ResultValue = SingleOperand2->Mantissa >> 2;
        ResultValue |= SINGLE_QUIET_NAN_PREFIX;
        ResultValue |= SingleOperand2->Sign << 31;

    } else if (SingleOperand1->Nan != FALSE) {
        ResultValue = SingleOperand1->Mantissa >> 2;
        ResultValue |= SINGLE_QUIET_NAN_PREFIX;
        ResultValue |= SingleOperand1->Sign << 31;

    } else {
        ResultValue = SINGLE_QUIET_NAN_VALUE;
    }

    //
    // If an invalid operation is specified or one of the operands is a
    // signaling NaN and the invalid operation trap is enabled, then
    // store the proper exception code and exception flags and return
    // a value of FALSE. Otherwise, store a quiet NaN as the destination
    // result and return a value of TRUE.
    //

    if ((CheckForSignalNan == FALSE) ||
        (SingleSignalNan(SingleOperand1) != FALSE) ||
        (SingleSignalNan(SingleOperand2) != FALSE)) {

        Fpcr = (PFPCR)&ContextBlock->TrapFrame->Fpcr;
        Fpcr->InvalidOperation = 1;
        Fpcr->SummaryBit = 1;
        if (ContextBlock->IeeeMode == FALSE) {
            ExceptionRecord = ContextBlock->ExceptionRecord;
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
            return FALSE;
        }
        SoftwareFpcr = ContextBlock->SoftwareFpcr;
        SoftwareFpcr->StatusInvalid = 1;
        if (SoftwareFpcr->EnableInvalid != 0) {
            ExceptionRecord = ContextBlock->ExceptionRecord;
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_INVALID_OPERATION;
            IeeeValue = KiInitializeIeeeValue(ExceptionRecord);
            IeeeValue->Value.Fp32Value.W[0] = ResultValue;
            return FALSE;
        }

        Fpcr->DisableInvalid = 1;
    }

    KiSetRegisterValue(ContextBlock->Fc + 32,
                       KiConvertSingleOperandToRegister(ResultValue),
                       ContextBlock->ExceptionFrame,
                       ContextBlock->TrapFrame);

    return TRUE;
}

BOOLEAN
KiNormalizeDouble (
    IN PFP_CONTEXT_BLOCK ContextBlock,
    IN PFP_DOUBLE_OPERAND ResultOperand,
    IN ULONGLONG StickyBits
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

        The sticky bits specify bits that were lost during the computation.

Arguments:

    ContextBlock - Supplies a pointer to the emulation context block.

    ResultOperand - Supplies a pointer to the result operand value.

    StickyBits - Supplies the value of the sticky bits.

Return Value:

    If there is not an exception, or the exception is handled, then a proper
    result is stored in the destination result, the continuation address is
    set, and a value of TRUE is returned. Otherwise, a proper value is stored and
    a value of FALSE is returned.

--*/

{

    ULONGLONG DenormalizeShift;
    PEXCEPTION_RECORD ExceptionRecord;
    ULONGLONG ExceptionResult;
    PFPCR Fpcr;
    PFP_IEEE_VALUE IeeeValue;
    BOOLEAN Inexact;
    ULONGLONG Mantissa;
    BOOLEAN Overflow;
    ULONGLONG ResultValue;
    ULONG RoundBit;
    PSW_FPCR SoftwareFpcr;
    BOOLEAN Underflow;
    ULONGLONG ResultStickyBits;
    ULONGLONG ResultMantissa;
    ULONG ResultRoundBit;
    LONG ResultExponent;
    BOOLEAN ReturnValue = TRUE;

    //
    // If the result is infinite, then store a properly signed infinity
    // in the destination register and return a value of TRUE. Otherwise,
    // round and normalize the result and check for overflow and underflow.
    //

    DBGPRINT("KiNormalizeDouble: Inf=%d NaN=%d Sign=%d Exponent=%d Mantissa=%.8x%.8x\n",
             ResultOperand->Infinity, ResultOperand->Nan, ResultOperand->Sign,
             ResultOperand->Exponent,
             ResultOperand->MantissaHigh, ResultOperand->MantissaLow);
    DBGPRINT("KiNormalizeDouble: StickyBits=%.16Lx\n", StickyBits);

    if (ResultOperand->Infinity != FALSE) {
        KiSetRegisterValue(ContextBlock->Fc + 32,
                           MAKE_QUAD(DOUBLE_INFINITY_VALUE_LOW,
                                     DOUBLE_INFINITY_VALUE_HIGH |
                                         (ResultOperand->Sign << 31)),
                           ContextBlock->ExceptionFrame,
                           ContextBlock->TrapFrame);

        return TRUE;
    }

    Mantissa = MAKE_QUAD(ResultOperand->MantissaLow,
                         ResultOperand->MantissaHigh);
    Fpcr = (PFPCR)&ContextBlock->TrapFrame->Fpcr;
    SoftwareFpcr = ContextBlock->SoftwareFpcr;

    //
    // If the overflow bit is set, then right shift the mantissa one bit,
    // accumulate the lost bit with the sticky bits, and adjust the exponent
    // value.
    //

    if ((Mantissa & ((ULONGLONG)1 << 55)) != 0) {
        StickyBits |= (Mantissa & 0x1);
        Mantissa >>= 1;
        ResultOperand->Exponent += 1;
    }

    //
    // If the mantissa is nonzero, then normalize the mantissa by left
    // shifting one bit at a time until there is a one bit in bit 54.
    //

    if (Mantissa != 0) {
        while ((Mantissa & ((ULONGLONG)1 << 54)) == 0) {
            Mantissa <<= 1;
            ResultOperand->Exponent -= 1;
        }
    }

    //
    // Right shift the mantissa two bits, set the round bit, and accumulate
    // the other lost bit with the sticky bits.
    //

    StickyBits |= (Mantissa & 0x1);
    RoundBit = (ULONG)(Mantissa & 0x2);
    Mantissa >>= 2;

    //
    // Convert to denormal format before rounding to allow underflow to
    // be detected on rounded result.  Save context to calculate IEEE
    // exception record, if needed.
    //

    if (ResultOperand->Exponent <= DOUBLE_MINIMUM_EXPONENT && Mantissa != 0) {

        //
        // Save everything needed for calculating IEEE exception record value
        //

        ResultMantissa = Mantissa;
        ResultExponent = ResultOperand->Exponent;
        ResultStickyBits = StickyBits;
        ResultRoundBit = RoundBit;

        //
        // Right shift the mantissa to set the minimum exponent plus an extra
        // bit for the denormal format
        //

        DenormalizeShift = 1 - ResultOperand->Exponent;

        //
        // The maximum denormal shift is 52 bits for the mantissa plus 1 bit for the round
        // A denormal shift of 54 guarantees 0 mantissa and 0 round bit and preserves all sticky bits
        //

        if (DenormalizeShift > 54) {
            DenormalizeShift = 54;
        }

        //
        // The denormalized result will be rounded after it is
        // shifted.  Preserve existing Round and Sticky Bits.
        //

        StickyBits |= RoundBit; 
        StickyBits |= (Mantissa << 1) << (64 - DenormalizeShift); 
        RoundBit = (ULONG)(Mantissa >> (DenormalizeShift - 1)) & 1;
        Mantissa = Mantissa >> DenormalizeShift;
        ResultOperand->Exponent = DOUBLE_MINIMUM_EXPONENT;
    }

    //
    // Round the result value using the mantissa, the round bit, and the sticky bits.
    //

    switch (ContextBlock->Round) {

        //
        // Round to nearest representable number.
        //

    case ROUND_TO_NEAREST:
        if (RoundBit != 0) {
            if ((StickyBits != 0) || ((Mantissa & 0x1) != 0)) {
                Mantissa += 1;
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
            Mantissa += 1;
        }
        break;

        //
        // Round toward minus infinity.
        //

    case ROUND_TO_MINUS_INFINITY:
        if ((ResultOperand->Sign != 0) &&
            ((StickyBits != 0) || (RoundBit != 0))) {
            Mantissa += 1;
        }
        break;
    }

    //
    // If rounding resulted in a carry into bit 53, then right shift the
    // mantissa one bit and adjust the exponent.
    //

    if ((Mantissa & ((ULONGLONG)1 << 53)) != 0) {
        Mantissa >>= 1;
        ResultOperand->Exponent += 1;
    }

    //
    // If rounding resulted in a carry into bit 52 in denormal format, then
    // adjust the exponent.
    //

    if ((ResultOperand->Exponent == DOUBLE_MINIMUM_EXPONENT) && 
        (Mantissa & ((ULONGLONG)1 << 52)) != 0) {
        ResultOperand->Exponent += 1;
    }

    //
    // If the exponent value is greater than or equal to the maximum
    // exponent value, then overflow has occurred. This results in both
    // the inexact and overflow sticky bits being set in the FPCR.
    //
    // If the exponent value is less than or equal to the minimum exponent
    // value, the mantissa is nonzero, and the denormalized result is inexact,
    // then underflow has occurred.  This results in both the inexact and
    // underflow sticky bits being set in the FPCR. 
    //
    // Or if underflow exceptions are enabled, underflow occurs for all denormal
    // numbers.  This results in the underflow sticky bit always being set in the
    // FPCR and the inexact sticky bit is set when the denormalized result is
    // also inexact.
    //
    // Otherwise, a normal result can be delivered, but it may be inexact.
    // If the result is inexact, then the inexact sticky bit is set in the
    // FPCR.
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
            ResultValue = MAKE_QUAD(DOUBLE_INFINITY_VALUE_LOW,
                                    DOUBLE_INFINITY_VALUE_HIGH |
                                        (ResultOperand->Sign << 31));
            break;

            //
            // Round toward zero.
            //
            // The result is the maximum number with the sign of the result.
            //

        case ROUND_TO_ZERO:
            ResultValue = MAKE_QUAD(DOUBLE_MAXIMUM_VALUE_LOW,
                                    DOUBLE_MAXIMUM_VALUE_HIGH |
                                        (ResultOperand->Sign << 31));
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
                ResultValue = MAKE_QUAD(DOUBLE_INFINITY_VALUE_LOW,
                                        DOUBLE_INFINITY_VALUE_HIGH);

            } else {
                ResultValue = MAKE_QUAD(DOUBLE_MAXIMUM_VALUE_LOW,
                                        DOUBLE_MAXIMUM_VALUE_HIGH |
                                            (1 << 31));
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
                ResultValue = MAKE_QUAD(DOUBLE_INFINITY_VALUE_LOW,
                                        DOUBLE_INFINITY_VALUE_HIGH |
                                            (1 << 31));

            } else {
                ResultValue = MAKE_QUAD(DOUBLE_MAXIMUM_VALUE_LOW,
                                        DOUBLE_MAXIMUM_VALUE_HIGH);
            }
            break;
        }

        //
        // Compute the overflow exception result value by subtracting 1536
        // from the exponent.
        //

        ExceptionResult = Mantissa & (((ULONGLONG)1 << 52) - 1);
        ExceptionResult |= (((ULONGLONG)ResultOperand->Exponent - 1536) << 52);
        ExceptionResult |= ((ULONGLONG)ResultOperand->Sign << 63);

    } else {

        //
        // After rounding if the exponent value is equal to
        // the minimum exponent value and the result was nonzero, then
        // underflow has occurred.
        //

        if ((ResultOperand->Exponent == DOUBLE_MINIMUM_EXPONENT) &&
            (Mantissa != 0 || RoundBit != 00 || StickyBits != 0)) {

            //
            // If the FPCR underflow to zero (denormal enable) control bit
            // is set, then flush the denormalized result to zero and do
            // not set an underflow status or generate an exception.
            //

            if ((ContextBlock->IeeeMode == FALSE) ||
                (SoftwareFpcr->DenormalResultEnable == 0)) {
                DBGPRINT("SoftwareFpcr->DenormalResultEnable == 0\n");
                ResultValue = 0;
                Inexact = FALSE;
                Overflow = FALSE;
                Underflow = FALSE;

            } else {

                ResultValue = Mantissa;
                ResultValue |= (ULONGLONG)ResultOperand->Sign << 63;

                //
                //
                // Compute the underflow exception result value by recalculating the
                // full precision answer and adding 1536 to the exponent.
                //

                //
                // Round the result value using the mantissa, the round bit, and the sticky bits.
                //

                switch (ContextBlock->Round) {

                    //
                    // Round to nearest representable number.
                    //

                case ROUND_TO_NEAREST:
                    if (ResultRoundBit != 0) {
                        if ((ResultStickyBits != 0) || ((ResultMantissa & 0x1) != 0)) {
                            ResultMantissa += 1;
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
                        ((ResultStickyBits != 0) || (ResultRoundBit != 0))) {
                        ResultMantissa += 1;
                    }
                    break;

                    //
                    // Round toward minus infinity.
                    //

                case ROUND_TO_MINUS_INFINITY:
                    if ((ResultOperand->Sign != 0) &&
                        ((ResultStickyBits != 0) || (ResultRoundBit != 0))) {
                        ResultMantissa += 1;
                    }
                    break;
                }

                //
                // If rounding resulted in a carry into bit 53, then right shift the
                // mantissa one bit and adjust the exponent.
                //

                if ((ResultMantissa & ((ULONGLONG)1 << 53)) != 0) {
                    ResultMantissa >>= 1;
                    ResultExponent += 1;
                }

                // Compute the underflow exception result value by adding
                // 1536 to the exponent.
                //

                ExceptionResult = ResultMantissa & (((ULONGLONG)1 << 52) - 1);
                ExceptionResult |= (((ULONGLONG)ResultExponent + 1536) << 52);
                ExceptionResult |= ((ULONGLONG)ResultOperand->Sign << 63);

                //
                // If the denormalized result is inexact, then set underflow.
                // Otherwise, for exact denormals do not set the underflow
                // sticky bit unless underflow exception is enabled.
                //

                Overflow = FALSE;
                Underflow = TRUE;
                if ((StickyBits != 0) || (RoundBit != 0)) {
                    Inexact = TRUE;

                } else {
                    Inexact = FALSE;
                }
            }

        } else {

            //
            // If the result is zero, then set the proper sign for zero.
            //

            if (Mantissa == 0) {
                ResultOperand->Exponent = 0;
            }

            ResultValue = Mantissa & (((ULONGLONG)1 << 52) - 1);
            ResultValue |= (ULONGLONG)ResultOperand->Exponent << 52;
            ResultValue |= (ULONGLONG)ResultOperand->Sign << 63;
            if ((StickyBits != 0) || (RoundBit != 0)) {
                Inexact = TRUE;

            } else {
                Inexact = FALSE;
            }
            Overflow = FALSE;
            Underflow = FALSE;
        }
    }

    //
    // Check to determine if an exception should be delivered.
    //

    ExceptionRecord = ContextBlock->ExceptionRecord;

    if (Overflow != FALSE) {
        Fpcr->Overflow = 1;
        Fpcr->InexactResult = 1;
        Fpcr->SummaryBit = 1;
        if (ContextBlock->IeeeMode == FALSE) {
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_OVERFLOW;
            return FALSE;
        }
        IeeeValue = KiInitializeIeeeValue(ExceptionRecord);
        SoftwareFpcr->StatusOverflow = 1;
        SoftwareFpcr->StatusInexact = 1;
        if (SoftwareFpcr->EnableOverflow != 0) {
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_OVERFLOW;
            IeeeValue->Value.Fp64Value.W[0] = LOW_PART(ExceptionResult);
            IeeeValue->Value.Fp64Value.W[1] = HIGH_PART(ExceptionResult);
            ReturnValue = FALSE;
        } else if (SoftwareFpcr->EnableInexact != 0) {
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
            IeeeValue->Value.Fp64Value.W[0] = LOW_PART(ExceptionResult);
            IeeeValue->Value.Fp64Value.W[1] = HIGH_PART(ExceptionResult);
            ReturnValue = FALSE;
        } else {
            Fpcr->DisableOverflow = 1;
            Fpcr->DisableInexact = 1;
        }

    } else if (Underflow != FALSE) {

        //
        // Non-IEEE instruction always forces underflow to zero
        //

        if (ContextBlock->IeeeMode == FALSE) {
            Fpcr->Underflow = 1;
            Fpcr->SummaryBit = 1;
            Fpcr->InexactResult = 1;
            if (ContextBlock->UnderflowEnable != FALSE) {
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_UNDERFLOW;
                return FALSE;
            }

        //
        // IEEE instructions don't report underflow unless the results are
        // inexact or underflow exceptions are enabled
        //

        } else {
            IeeeValue = KiInitializeIeeeValue(ExceptionRecord);
            if (Inexact != FALSE) {
                Fpcr->Underflow = 1;
                Fpcr->SummaryBit = 1;
                Fpcr->InexactResult = 1;
                SoftwareFpcr->StatusUnderflow = 1;
                SoftwareFpcr->StatusInexact = 1;
            } 
            if (SoftwareFpcr->EnableUnderflow != 0) {
                Fpcr->Underflow = 1;
                Fpcr->SummaryBit = 1;
                SoftwareFpcr->StatusUnderflow = 1;
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_UNDERFLOW;
                IeeeValue->Value.Fp64Value.W[0] = LOW_PART(ExceptionResult);
                IeeeValue->Value.Fp64Value.W[1] = HIGH_PART(ExceptionResult);
                ReturnValue = FALSE;
            } else if (Inexact != FALSE && SoftwareFpcr->EnableInexact != 0) {
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
                IeeeValue->Value.Fp64Value.W[0] = LOW_PART(ExceptionResult);
                IeeeValue->Value.Fp64Value.W[1] = HIGH_PART(ExceptionResult);
                ReturnValue = FALSE;
            } else if (Inexact != FALSE) {
                Fpcr->DisableUnderflow = 1;
                Fpcr->DisableInexact = 1;
            }
        }

    } else if (Inexact != FALSE) {
        Fpcr->InexactResult = 1;
        Fpcr->SummaryBit = 1;
        if (ContextBlock->IeeeMode != FALSE) {
            IeeeValue = KiInitializeIeeeValue(ExceptionRecord);
            SoftwareFpcr->StatusInexact = 1;
            if (SoftwareFpcr->EnableInexact != 0) {
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
                IeeeValue->Value.Fp64Value.W[0] = LOW_PART(ResultValue);
                IeeeValue->Value.Fp64Value.W[1] = HIGH_PART(ResultValue);
                ReturnValue = FALSE;
            } else {
                Fpcr->DisableInexact = 1;
            }
        }
    }

    //
    // Always write the destination register.  If an exception is delivered, and
    // then dismissed, the correct value must be in the register.
    //

    KiSetRegisterValue(ContextBlock->Fc + 32,
                       ResultValue,
                       ContextBlock->ExceptionFrame,
                       ContextBlock->TrapFrame);

    //
    // Return a value of TRUE.unless an exception should be generated
    //

    return ReturnValue;
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
        the hidden bit (if appropriate), and an overflow bit of zero.
        As called above, the guard bit and the round bit are also zero.
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
    LONGLONG ExponentShift;
    PFPCR Fpcr;
    PFP_IEEE_VALUE IeeeValue;
    ULONGLONG Mantissa;
    BOOLEAN Overflow;
    ULONGLONG ResultValue;
    ULONG RoundBit;
    ULONGLONG StickyBits;
    PSW_FPCR SoftwareFpcr;

    //
    // Subtract out the exponent bias and divide the cases into right
    // and left shifts.
    //

    ExponentShift = ResultOperand->Exponent - DOUBLE_EXPONENT_BIAS;
    DBGPRINT("KiNormalizeQuadword: Inf=%d NaN=%d Sign=%d Exponent=%d Mantissa=%.8x%.8x\n",
             ResultOperand->Infinity, ResultOperand->Nan, ResultOperand->Sign,
             ResultOperand->Exponent,
             ResultOperand->MantissaHigh, ResultOperand->MantissaLow);
    DBGPRINT(".. ExponentShift = %d\n", ExponentShift);
    Mantissa = MAKE_QUAD(ResultOperand->MantissaLow,
                         ResultOperand->MantissaHigh);

    if (ExponentShift < 54) {

        //
        // The integer result value is less than 2**54 and so a right shift
        // must be performed.
        //

        ExponentShift = 54 - ExponentShift;
        if (ExponentShift < 64) {
            StickyBits = Mantissa << (64 - ExponentShift);
            ResultValue = Mantissa >> ExponentShift;

        } else {
            StickyBits = Mantissa;
            ResultValue = 0;
        }
        Overflow = FALSE;

    } else if (ExponentShift > 54) {
        ExponentShift -= 54;

        //
        // The integer result value is 2**54 or greater and so a left shift
        // must be performed. If the unsigned integer result value is 2**64
        // or greater, then overflow has occurred and store the low order 64
        // bits of the true result.
        //

        if (ExponentShift < (64 - 54)) {
            StickyBits = Mantissa >> (64 - ExponentShift);
            ResultValue = Mantissa << ExponentShift;
            Overflow = FALSE;

        } else {
            StickyBits = 0;
            if (ExponentShift < 64) {
                ResultValue = Mantissa << ExponentShift;

            } else {
                ResultValue = 0;
            }
            Overflow = TRUE;
        }

    } else {
        StickyBits = 0;
        ResultValue = Mantissa;
        Overflow = FALSE;
    }
    DBGPRINT(".. ResultValue = %.16Lx, StickyBits = %.16Lx\n",
             ResultValue, StickyBits);

    //
    // Round the result value using the mantissa, the round bit, and the
    // sticky bits.
    //

    RoundBit = (ULONG)(StickyBits >> 63);
    StickyBits <<= 1;
    DBGPRINT(".. ResultValue = %.16Lx, StickyBits = %.16Lx, RoundBit = %lx\n",
             ResultValue, StickyBits, RoundBit);
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
        if ((ResultOperand->Sign == 0) &&
            ((StickyBits != 0) || (RoundBit != 0))) {
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
        if ((ResultOperand->Sign != 0) &&
            ((StickyBits != 0) || (RoundBit != 0))) {
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
        if ((LONGLONG)ResultValue < 0) {
            Overflow = TRUE;
        }

    } else {
        ResultValue = -(LONGLONG)ResultValue;
        if ((LONGLONG)ResultValue > 0) {
            Overflow = TRUE;
        }
    }
    DBGPRINT(".. ResultValue = %.16Lx, StickyBits = %.16Lx\n",
             ResultValue, StickyBits);

    //
    // Check to determine if an exception should be delivered or the result
    // should be written to the destination register.
    //

    if (Overflow != FALSE) {
        return KiInvalidOperationQuadword(ContextBlock, ResultValue);

    } else if ((StickyBits | RoundBit) != 0) {
        Fpcr = (PFPCR)&ContextBlock->TrapFrame->Fpcr;
        Fpcr->InexactResult = 1;
        Fpcr->SummaryBit = 1;
        if (ContextBlock->IeeeMode != FALSE) {
            SoftwareFpcr = ContextBlock->SoftwareFpcr;
            SoftwareFpcr->StatusInexact = 1;
            if (SoftwareFpcr->EnableInexact != 0) {
                ExceptionRecord = ContextBlock->ExceptionRecord;
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
                IeeeValue = KiInitializeIeeeValue(ExceptionRecord);
                IeeeValue->Value.U64Value.LowPart = LOW_PART(ResultValue);
                IeeeValue->Value.U64Value.HighPart = HIGH_PART(ResultValue);
                return FALSE;
            }

            Fpcr->DisableInexact = 1;
        }
    }

    //
    // Set the destination register value and return a value of TRUE.
    //

    KiSetRegisterValue(ContextBlock->Fc + 32,
                       ResultValue,
                       ContextBlock->ExceptionFrame,
                       ContextBlock->TrapFrame);

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

        The sticky bits specify bits that were lost during the computation.

Arguments:

    ContextBlock - Supplies a pointer to the emulation context block.

    ResultOperand - Supplies a pointer to the result operand value.

    StickyBits - Supplies the value of the sticky bits.

Return Value:

    If there is not an exception, or the exception is handled, then a proper
    result is stored in the destination result, the continuation address is
    set, and a value of TRUE is returned. Otherwise, a proper value is stored and
    a value of FALSE is returned.

--*/

{

    ULONG DenormalizeShift;
    PEXCEPTION_RECORD ExceptionRecord;
    ULONG ExceptionResult;
    PFPCR Fpcr;
    PFP_IEEE_VALUE IeeeValue;
    BOOLEAN Inexact;
    ULONG Mantissa;
    BOOLEAN Overflow;
    ULONG ResultValue;
    ULONG RoundBit;
    PSW_FPCR SoftwareFpcr;
    BOOLEAN Underflow;
    ULONG ResultStickyBits;
    ULONG ResultMantissa;
    ULONG ResultRoundBit;
    LONG ResultExponent;
    BOOLEAN ReturnValue = TRUE;

    //
    // If the result is infinite, then store a properly signed infinity
    // in the destination register and return a value of TRUE. Otherwise,
    // round and normalize the result and check for overflow and underflow.
    //

    DBGPRINT("KiNormalizeSingle: Inf=%d NaN=%d Sign=%d Exponent=%d Mantissa=%.8x\n",
             ResultOperand->Infinity, ResultOperand->Nan, ResultOperand->Sign,
             ResultOperand->Exponent, ResultOperand->Mantissa);
    DBGPRINT("KiNormalizeSingle: StickyBits=%.8lx\n", StickyBits);

    if (ResultOperand->Infinity != FALSE) {
        ResultValue = SINGLE_INFINITY_VALUE | (ResultOperand->Sign << 31);
        KiSetRegisterValue(ContextBlock->Fc + 32,
                           KiConvertSingleOperandToRegister(ResultValue),
                           ContextBlock->ExceptionFrame,
                           ContextBlock->TrapFrame);

        return TRUE;
    }

    Mantissa = ResultOperand->Mantissa;
    Fpcr = (PFPCR)&ContextBlock->TrapFrame->Fpcr;
    SoftwareFpcr = ContextBlock->SoftwareFpcr;

    //
    // If the overflow bit is set, then right shift the mantissa one bit,
    // accumulate the lost bit with the sticky bits, and adjust the exponent
    // value.
    //

    if ((Mantissa & (1 << 26)) != 0) {
        StickyBits |= (Mantissa & 0x1);
        Mantissa >>= 1;
        ResultOperand->Exponent += 1;
    }

    //
    // If the mantissa is nonzero, then normalize the mantissa by left
    // shifting one bit at a time until there is a one bit in bit 25.
    //

    if (Mantissa != 0) {
        while ((Mantissa & (1 << 25)) == 0) {
            Mantissa <<= 1;
            ResultOperand->Exponent -= 1;
        }
    }

    //
    // Right shift the mantissa two bits, set the round bit, and accumulate
    // the other lost bit with the sticky bits.
    //

    StickyBits |= (Mantissa & 0x1);
    RoundBit = (Mantissa & 0x2);
    Mantissa >>= 2;

    //
    // Convert to denormal format before rounding to allow underflow to
    // be detected on rounded result.  Save context to calculate IEEE
    // exception record, if needed.
    //

    if (ResultOperand->Exponent <= SINGLE_MINIMUM_EXPONENT && Mantissa != 0) {

        //
        // Save everything needed for calculating IEEE exception record value
        //

        ResultMantissa = Mantissa;
        ResultExponent = ResultOperand->Exponent;
        ResultStickyBits = StickyBits;
        ResultRoundBit = RoundBit;

        //
        // Right shift the mantissa to set the minimum exponent plus an extra
        // bit for the denormal format
        //

        DenormalizeShift = 1 - ResultOperand->Exponent;

        //
        // The maximum denormal shift is 23 bits for the mantissa plus 1 bit for the round
        // A denormal shift of 25 guarantees 0 mantissa and 0 round bit and preserves all sticky bits
        //

        if (DenormalizeShift > 25) {
            DenormalizeShift = 25;
        }

        //
        // The denormalized result will be rounded after it is
        // shifted.  Preserve existing Round and Sticky Bits.
        //

        StickyBits |= RoundBit; 
        StickyBits |= (Mantissa << 1) << (32 - DenormalizeShift); 
        RoundBit = (Mantissa >> (DenormalizeShift - 1)) & 1;
        Mantissa = Mantissa >> DenormalizeShift;
        ResultOperand->Exponent = SINGLE_MINIMUM_EXPONENT;
    }

    //
    // Round the result value using the mantissa, the round bit, and the sticky bits.
    //

    switch (ContextBlock->Round) {

        //
        // Round to nearest representable number.
        //

    case ROUND_TO_NEAREST:
        if (RoundBit != 0) {
            if ((StickyBits != 0) || ((Mantissa & 0x1) != 0)) {
                Mantissa += 1;
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
            Mantissa += 1;
        }
        break;

        //
        // Round toward minus infinity.
        //

    case ROUND_TO_MINUS_INFINITY:
        if ((ResultOperand->Sign != 0) &&
            ((StickyBits != 0) || (RoundBit != 0))) {
            Mantissa += 1;
        }
        break;
    }

    //
    // If rounding resulted in a carry into bit 24, then right shift the
    // mantissa one bit and adjust the exponent.
    //

    if ((Mantissa & (1 << 24)) != 0) {
        Mantissa >>= 1;
        ResultOperand->Exponent += 1;
    }

    //
    // If rounding resulted in a carry into bit 23 in denormal format, then
    // adjust the exponent.
    //

    if ((ResultOperand->Exponent == SINGLE_MINIMUM_EXPONENT) && 
        (Mantissa & (1 << 23)) != 0) {
        ResultOperand->Exponent += 1;
    }

    //
    // If the exponent value is greater than or equal to the maximum
    // exponent value, then overflow has occurred. This results in both
    // the inexact and overflow sticky bits being set in the FPCR.
    //
    // If the exponent value is less than or equal to the minimum exponent
    // value, the mantissa is nonzero, and the denormalized result is inexact,
    // then underflow has occurred.  This results in both the inexact and
    // underflow sticky bits being set in the FPCR. 
    //
    // Or if underflow exceptions are enabled, underflow occurs for all denormal
    // numbers.  This results in the underflow sticky bit always being set in the
    // FPCR and the inexact sticky bit is set when the denormalized result is
    // also inexact.
    //
    // Otherwise, a normal result can be delivered, but it may be inexact.
    // If the result is inexact, then the inexact sticky bit is set in the
    // FPCR.
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

        ExceptionResult = Mantissa & ((1 << 23) - 1);
        ExceptionResult |= ((ResultOperand->Exponent - 192) << 23);
        ExceptionResult |= (ResultOperand->Sign << 31);

    } else {

        //
        // After rounding if the exponent value is equal to
        // the minimum exponent value and the result was nonzero, then
        // underflow has occurred.
        //

        if ((ResultOperand->Exponent == SINGLE_MINIMUM_EXPONENT) &&
            (Mantissa != 0 || RoundBit != 00 || StickyBits != 0)) {

            //
            // If the FPCR underflow to zero (denormal enable) control bit
            // is set, then flush the denormalized result to zero and do
            // not set an underflow status or generate an exception.
            //

            if ((ContextBlock->IeeeMode == FALSE) ||
                (SoftwareFpcr->DenormalResultEnable == 0)) {
                DBGPRINT("SoftwareFpcr->DenormalResultEnable == 0\n");
                ResultValue = 0;
                Inexact = FALSE;
                Overflow = FALSE;
                Underflow = FALSE;

            } else {

                ResultValue = Mantissa;
                ResultValue |= ResultOperand->Sign << 31;

                //
                //
                // Compute the underflow exception result value by first recalculating the
                // full precision answer.
                //

                //
                // Round the result value using the mantissa, the round bit, and the sticky bits.
                //

                switch (ContextBlock->Round) {

                    //
                    // Round to nearest representable number.
                    //

                case ROUND_TO_NEAREST:
                    if (ResultRoundBit != 0) {
                        if ((ResultStickyBits != 0) || ((ResultMantissa & 0x1) != 0)) {
                            ResultMantissa += 1;
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
                        ((ResultStickyBits != 0) || (ResultRoundBit != 0))) {
                        ResultMantissa += 1;
                    }
                    break;

                    //
                    // Round toward minus infinity.
                    //

                case ROUND_TO_MINUS_INFINITY:
                    if ((ResultOperand->Sign != 0) &&
                        ((ResultStickyBits != 0) || (ResultRoundBit != 0))) {
                        ResultMantissa += 1;
                    }
                    break;
                }

                //
                // If rounding resulted in a carry into bit 24, then right shift the
                // mantissa one bit and adjust the exponent.
                //

                if ((ResultMantissa & (1 << 24)) != 0) {
                    ResultMantissa >>= 1;
                    ResultExponent += 1;
                }

                //
                // Compute the underflow exception result value by adding
                // 192 to the exponent.
                //

                ExceptionResult = ResultMantissa & ((1 << 23) - 1);
                ExceptionResult |= ((ResultExponent + 192) << 23);
                ExceptionResult |= (ResultOperand->Sign << 31);

                //
                // If the denormalized result is inexact, then set underflow.
                // Otherwise, for exact denormals do not set the underflow
                // sticky bit unless underflow exception is enabled.
                //

                Overflow = FALSE;
                Underflow = TRUE;
                if ((StickyBits != 0) || (RoundBit != 0)) {
                    Inexact = TRUE;

                } else {
                    Inexact = FALSE;
                }
            }

        } else {

            //
            // If the result is zero, then set the proper sign for zero.
            //

            if (Mantissa == 0) {
                ResultOperand->Exponent = 0;
            }

            ResultValue = Mantissa & ((1 << 23) - 1);
            ResultValue |= (ResultOperand->Exponent << 23);
            ResultValue |= (ResultOperand->Sign << 31);
            if ((StickyBits != 0) || (RoundBit != 0)) {
                Inexact = TRUE;

            } else {
                Inexact = FALSE;
            }
            Overflow = FALSE;
            Underflow = FALSE;
        }
    }

    //
    // Check to determine if an exception should be delivered.
    //

    ExceptionRecord = ContextBlock->ExceptionRecord;

    if (Overflow != FALSE) {
        Fpcr->Overflow = 1;
        Fpcr->InexactResult = 1;
        Fpcr->SummaryBit = 1;
        if (ContextBlock->IeeeMode == FALSE) {
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_OVERFLOW;
            return FALSE;
        }
        IeeeValue = KiInitializeIeeeValue(ExceptionRecord);
        SoftwareFpcr->StatusOverflow = 1;
        SoftwareFpcr->StatusInexact = 1;
        if (SoftwareFpcr->EnableOverflow != 0) {
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_OVERFLOW;
            IeeeValue->Value.Fp32Value.W[0] = ExceptionResult;
            ReturnValue = FALSE;
        } else if (SoftwareFpcr->EnableInexact != 0) {
            ExceptionRecord->ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
            IeeeValue->Value.Fp32Value.W[0] = ExceptionResult;
            ReturnValue = FALSE;
        } else {
            Fpcr->DisableOverflow = 1;
            Fpcr->DisableInexact = 1;
        }

    } else if (Underflow != FALSE) {

        //
        // Non-IEEE instruction always forces underflow to zero
        //

        if (ContextBlock->IeeeMode == FALSE) {
            Fpcr->Underflow = 1;
            Fpcr->SummaryBit = 1;
            Fpcr->InexactResult = 1;
            if (ContextBlock->UnderflowEnable != FALSE) {
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_UNDERFLOW;
                return FALSE;
            }

        //
        // IEEE instructions don't report underflow unless the results are
        // inexact or underflow exceptions are enabled
        //

        } else {
            IeeeValue = KiInitializeIeeeValue(ExceptionRecord);
            if (Inexact != FALSE) {
                Fpcr->Underflow = 1;
                Fpcr->InexactResult = 1;
                Fpcr->SummaryBit = 1;
                SoftwareFpcr->StatusUnderflow = 1;
                SoftwareFpcr->StatusInexact = 1;
            }
            if (SoftwareFpcr->EnableUnderflow != 0) {
                Fpcr->Underflow = 1;
                Fpcr->SummaryBit = 1;
                SoftwareFpcr->StatusUnderflow = 1;
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_UNDERFLOW;
                IeeeValue->Value.Fp32Value.W[0] = ExceptionResult;
                ReturnValue = FALSE;
            } else if (Inexact != FALSE && SoftwareFpcr->EnableInexact != 0) {
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
                IeeeValue->Value.Fp32Value.W[0] = ExceptionResult;
                ReturnValue = FALSE;
            } else if (Inexact != FALSE) {
                Fpcr->DisableUnderflow = 1;
                Fpcr->DisableInexact = 1;
            }
        }

    } else if (Inexact != FALSE) {
        Fpcr->InexactResult = 1;
        Fpcr->SummaryBit = 1;
        if (ContextBlock->IeeeMode != FALSE) {
            IeeeValue = KiInitializeIeeeValue(ExceptionRecord);
            SoftwareFpcr->StatusInexact = 1;
            if (SoftwareFpcr->EnableInexact != 0) {
                ExceptionRecord->ExceptionCode = STATUS_FLOAT_INEXACT_RESULT;
                IeeeValue->Value.Fp32Value.W[0] = ResultValue;
                ReturnValue = FALSE;
            } else {
                Fpcr->DisableInexact = 1;
            }
        }
    }

    //
    // Always write the destination register.  If an exception is delivered, and
    // then dismissed, the correct value must be in the register.
    //

    KiSetRegisterValue(ContextBlock->Fc + 32,
                       KiConvertSingleOperandToRegister(ResultValue),
                       ContextBlock->ExceptionFrame,
                       ContextBlock->TrapFrame);

    //
    // Return a value of TRUE.unless an exception should be generated
    //

    return ReturnValue;
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

    ULONGLONG Value;
    ULONG Value1;
    ULONG Value2;

    //
    // Get the source register value and unpack the sign, exponent, and
    // mantissa value.
    //

    Value = KiGetRegisterValue(Source + 32,
                               ContextBlock->ExceptionFrame,
                               ContextBlock->TrapFrame);
    Value1 = (ULONG)Value;
    Value2 = (ULONG)(Value >> 32);

    DoubleOperand->Sign = Value2 >> 31;
    DoubleOperand->Exponent = (Value2 >> (52 - 32)) & 0x7ff;
    DoubleOperand->MantissaHigh = Value2 & 0xfffff;
    DoubleOperand->MantissaLow = Value1;

    //
    // If the exponent is the largest possible value, then the number is
    // either a NaN or an infinity. Otherwise if the exponent is the smallest
    // possible value and the mantissa is nonzero, then the number is
    // denormalized. Otherwise the number is finite and normal.
    //

    if (DoubleOperand->Exponent == DOUBLE_MAXIMUM_EXPONENT) {
        DoubleOperand->Normal = FALSE;
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
        DoubleOperand->Normal = TRUE;
        if (DoubleOperand->Exponent == DOUBLE_MINIMUM_EXPONENT) {
            if ((DoubleOperand->MantissaHigh | DoubleOperand->MantissaLow) != 0) {
                if (ContextBlock->SoftwareFpcr->DenormalOperandsEnable == 0) {
                    DBGPRINT("SoftwareFpcr->DenormalOperandsEnable == 0\n");
                    DoubleOperand->MantissaHigh = 0;
                    DoubleOperand->MantissaLow = 0;
                    DoubleOperand->Exponent = 0;
                } else {
                    DoubleOperand->Normal = FALSE;
                    DoubleOperand->Exponent += 1;
                    while ((DoubleOperand->MantissaHigh & (1 << 20)) == 0) {
                        DoubleOperand->MantissaHigh =
                                (DoubleOperand->MantissaHigh << 1) |
                                                (DoubleOperand->MantissaLow >> 31);
                        DoubleOperand->MantissaLow <<= 1;
                        DoubleOperand->Exponent -= 1;
                    }
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
    DBGPRINT("KiUnpackDouble: Inf=%d NaN=%d Sign=%d Exponent=%d Mantissa=%.8x%.8x\n",
             DoubleOperand->Infinity, DoubleOperand->Nan, DoubleOperand->Sign,
             DoubleOperand->Exponent,
             DoubleOperand->MantissaHigh, DoubleOperand->MantissaLow);

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

    Value = KiConvertRegisterToSingleOperand(
                KiGetRegisterValue(Source + 32,
                                  ContextBlock->ExceptionFrame,
                                  ContextBlock->TrapFrame));

    SingleOperand->Sign = Value >> 31;
    SingleOperand->Exponent = (Value >> 23) & 0xff;
    SingleOperand->Mantissa = Value & 0x7fffff;

    //
    // If the exponent is the largest possible value, then the number is
    // either a NaN or an infinity. Otherwise if the exponent is the smallest
    // possible value and the mantissa is nonzero, then the number is
    // denormalized. Otherwise the number is finite and normal.
    //

    if (SingleOperand->Exponent == SINGLE_MAXIMUM_EXPONENT) {
        SingleOperand->Normal = FALSE;
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
        SingleOperand->Normal = TRUE;
        if (SingleOperand->Exponent == SINGLE_MINIMUM_EXPONENT) {
            if (SingleOperand->Mantissa != 0) {
                if (ContextBlock->SoftwareFpcr->DenormalOperandsEnable == 0) {
                    DBGPRINT("SoftwareFpcr->DenormalOperandsEnable == 0\n");
                    SingleOperand->Mantissa = 0;
                    SingleOperand->Exponent = 0;
                } else {
                    SingleOperand->Normal = FALSE;
                    SingleOperand->Exponent += 1;
                    while ((SingleOperand->Mantissa & (1 << 23)) == 0) {
                        SingleOperand->Mantissa <<= 1;
                        SingleOperand->Exponent -= 1;
                    }
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
    DBGPRINT("KiUnpackSingle: Inf=%d NaN=%d Sign=%d Exponent=%d Mantissa=%.8x\n",
             SingleOperand->Infinity, SingleOperand->Nan, SingleOperand->Sign,
             SingleOperand->Exponent, SingleOperand->Mantissa);
    return;
}
