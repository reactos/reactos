/*++

Copyright (c) 1991  Microsoft Corporation
Copyright (c) 1992  Digital Equipment Corporation

Module Name:

    getsetrg.c

Abstract:

    This module implement the code necessary to get and set register values.
    These routines are used during the emulation of unaligned data references
    and floating point exceptions.

Author:

    David N. Cutler (davec) 17-Jun-1991

Environment:

    Kernel mode only.

Revision History:

    Thomas Van Baak (tvb) 14-Jul-1992

        Adapted for NT/Alpha

--*/

#include "ki.h"

ULONGLONG
KiGetRegisterValue (
    IN ULONG Register,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called to get the value of a register from the specified
    exception or trap frame.

Arguments:

    Register - Supplies the number of the register whose value is to be
        returned. Integer registers are specified as 0 - 31 and floating
        registers are specified as 32 - 63.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    The value of the specified register is returned as the function value.

--*/

{

    //
    // Dispatch on the register number.
    //

    switch (Register) {

        //
        // Integer register V0.
        //

    case 0:
        return TrapFrame->IntV0;

        //
        // Integer register T0.
        //

    case 1:
        return TrapFrame->IntT0;

        //
        // Integer register T1.
        //

    case 2:
        return TrapFrame->IntT1;

        //
        // Integer register T2.
        //

    case 3:
        return TrapFrame->IntT2;

        //
        // Integer register T3.
        //

    case 4:
        return TrapFrame->IntT3;

        //
        // Integer register T4.
        //

    case 5:
        return TrapFrame->IntT4;

        //
        // Integer register T5.
        //

    case 6:
        return TrapFrame->IntT5;

        //
        // Integer register T6.
        //

    case 7:
        return TrapFrame->IntT6;

        //
        // Integer register T7.
        //

    case 8:
        return TrapFrame->IntT7;

        //
        // Integer register S0.
        //

    case 9:
        return ExceptionFrame->IntS0;

        //
        // Integer register S1.
        //

    case 10:
        return ExceptionFrame->IntS1;

        //
        // Integer register S2.
        //

    case 11:
        return ExceptionFrame->IntS2;

        //
        // Integer register S3.
        //

    case 12:
        return ExceptionFrame->IntS3;

        //
        // Integer register S4.
        //

    case 13:
        return ExceptionFrame->IntS4;

        //
        // Integer register S5.
        //

    case 14:
        return ExceptionFrame->IntS5;

        //
        // Integer register S6/Fp.
        //
        // N.B. Unlike the other S registers, S6 is obtained from the trap
        // frame instead of the exception frame since it is used by the kernel
        // as a trap frame pointer.
        //

    case 15:
        return TrapFrame->IntFp;

        //
        // Integer register A0.
        //

    case 16:
        return TrapFrame->IntA0;

        //
        // Integer register A1.
        //

    case 17:
        return TrapFrame->IntA1;

        //
        // Integer register A2
        //

    case 18:
        return TrapFrame->IntA2;

        //
        // Integer register A3.
        //

    case 19:
        return TrapFrame->IntA3;

        //
        // Integer register A4.
        //

    case 20:
        return TrapFrame->IntA4;

        //
        // Integer register A5.
        //

    case 21:
        return TrapFrame->IntA5;

        //
        // Integer register T8.
        //

    case 22:
        return TrapFrame->IntT8;

        //
        // Integer register T9.
        //

    case 23:
        return TrapFrame->IntT9;

        //
        // Integer register T10.
        //

    case 24:
        return TrapFrame->IntT10;

        //
        // Integer register T11.
        //

    case 25:
        return TrapFrame->IntT11;

        //
        // Integer register Ra.
        //

    case 26:
        return TrapFrame->IntRa;

        //
        // Integer register T12.
        //

    case 27:
        return TrapFrame->IntT12;

        //
        // Integer register At.
        //

    case 28:
        return TrapFrame->IntAt;

        //
        // Integer register Gp.
        //

    case 29:
        return TrapFrame->IntGp;

        //
        // Integer register Sp.
        //

    case 30:
        return TrapFrame->IntSp;

        //
        // Integer register Zero.
        //

    case 31:
        return 0;

        //
        // Floating register F0.
        //

    case 32:
        return TrapFrame->FltF0;

        //
        // Floating register F1.
        //

    case 33:
        return TrapFrame->FltF1;

        //
        // Floating register F2.
        //

    case 34:
        return ExceptionFrame->FltF2;

        //
        // Floating register F3.
        //

    case 35:
        return ExceptionFrame->FltF3;

        //
        // Floating register F4.
        //

    case 36:
        return ExceptionFrame->FltF4;

        //
        // Floating register F5.
        //

    case 37:
        return ExceptionFrame->FltF5;

        //
        // Floating register F6.
        //

    case 38:
        return ExceptionFrame->FltF6;

        //
        // Floating register F7.
        //

    case 39:
        return ExceptionFrame->FltF7;

        //
        // Floating register F8.
        //

    case 40:
        return ExceptionFrame->FltF8;

        //
        // Floating register F9.
        //

    case 41:
        return ExceptionFrame->FltF9;

        //
        // Floating register F10.
        //

    case 42:
        return TrapFrame->FltF10;

        //
        // Floating register F11.
        //

    case 43:
        return TrapFrame->FltF11;

        //
        // Floating register F12.
        //

    case 44:
        return TrapFrame->FltF12;

        //
        // Floating register F13.
        //

    case 45:
        return TrapFrame->FltF13;

        //
        // Floating register F14.
        //

    case 46:
        return TrapFrame->FltF14;

        //
        // Floating register F15.
        //

    case 47:
        return TrapFrame->FltF15;

        //
        // Floating register F16.
        //

    case 48:
        return TrapFrame->FltF16;

        //
        // Floating register F17.
        //

    case 49:
        return TrapFrame->FltF17;

        //
        // Floating register F18.
        //

    case 50:
        return TrapFrame->FltF18;

        //
        // Floating register F19.
        //

    case 51:
        return TrapFrame->FltF19;

        //
        // Floating register F20.
        //

    case 52:
        return TrapFrame->FltF20;

        //
        // Floating register F21.
        //

    case 53:
        return TrapFrame->FltF21;

        //
        // Floating register F22.
        //

    case 54:
        return TrapFrame->FltF22;

        //
        // Floating register F23.
        //

    case 55:
        return TrapFrame->FltF23;

        //
        // Floating register F24.
        //

    case 56:
        return TrapFrame->FltF24;

        //
        // Floating register F25.
        //

    case 57:
        return TrapFrame->FltF25;

        //
        // Floating register F26.
        //

    case 58:
        return TrapFrame->FltF26;

        //
        // Floating register F27.
        //

    case 59:
        return TrapFrame->FltF27;

        //
        // Floating register F28.
        //

    case 60:
        return TrapFrame->FltF28;

        //
        // Floating register F29.
        //

    case 61:
        return TrapFrame->FltF29;

        //
        // Floating register F30.
        //

    case 62:
        return TrapFrame->FltF30;

        //
        // Floating register F31 (Zero).
        //

    case 63:
        return 0;
    }
}

VOID
KiSetRegisterValue (
    IN ULONG Register,
    IN ULONGLONG Value,
    OUT PKEXCEPTION_FRAME ExceptionFrame,
    OUT PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called to set the value of a register in the specified
    exception or trap frame.

Arguments:

    Register - Supplies the number of the register whose value is to be
        stored. Integer registers are specified as 0 - 31 and floating
        registers are specified as 32 - 63.

    Value - Supplies the value to be stored in the specified register.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    None.

--*/

{

    //
    // Dispatch on the register number.
    //

    switch (Register) {

        //
        // Integer register V0.
        //

    case 0:
        TrapFrame->IntV0 = Value;
        return;

        //
        // Integer register T0.
        //

    case 1:
        TrapFrame->IntT0 = Value;
        return;

        //
        // Integer register T1.
        //

    case 2:
        TrapFrame->IntT1 = Value;
        return;

        //
        // Integer register T2.
        //

    case 3:
        TrapFrame->IntT2 = Value;
        return;

        //
        // Integer register T3.
        //

    case 4:
        TrapFrame->IntT3 = Value;
        return;

        //
        // Integer register T4.
        //

    case 5:
        TrapFrame->IntT4 = Value;
        return;

        //
        // Integer register T5.
        //

    case 6:
        TrapFrame->IntT5 = Value;
        return;

        //
        // Integer register T6.
        //

    case 7:
        TrapFrame->IntT6 = Value;
        return;

        //
        // Integer register T7.
        //

    case 8:
        TrapFrame->IntT7 = Value;
        return;

        //
        // Integer register S0.
        //

    case 9:
        ExceptionFrame->IntS0 = Value;
        return;

        //
        // Integer register S1.
        //

    case 10:
        ExceptionFrame->IntS1 = Value;
        return;

        //
        // Integer register S2.
        //

    case 11:
        ExceptionFrame->IntS2 = Value;
        return;

        //
        // Integer register S3.
        //

    case 12:
        ExceptionFrame->IntS3 = Value;
        return;

        //
        // Integer register S4.
        //

    case 13:
        ExceptionFrame->IntS4 = Value;
        return;

        //
        // Integer register S5.
        //

    case 14:
        ExceptionFrame->IntS5 = Value;
        return;

        //
        // Integer register S6/Fp.
        //
        // N.B. Unlike the other S registers, S6 is stored back in the trap
        // frame instead of the exception frame since it is used by the kernel
        // as a trap frame pointer.
        //

    case 15:
        TrapFrame->IntFp = Value;
        return;

        //
        // Integer register A0.
        //

    case 16:
        TrapFrame->IntA0 = Value;
        return;

        //
        // Integer register A1.
        //

    case 17:
        TrapFrame->IntA1 = Value;
        return;

        //
        // Integer register A2.
        //

    case 18:
        TrapFrame->IntA2 = Value;
        return;

        //
        // Integer register A3.
        //

    case 19:
        TrapFrame->IntA3 = Value;
        return;

        //
        // Integer register A4.
        //

    case 20:
        TrapFrame->IntA4 = Value;
        return;

        //
        // Integer register A5.
        //

    case 21:
        TrapFrame->IntA5 = Value;
        return;

        //
        // Integer register T8.
        //

    case 22:
        TrapFrame->IntT8 = Value;
        return;

        //
        // Integer register T9.
        //

    case 23:
        TrapFrame->IntT9 = Value;
        return;

        //
        // Integer register T10.
        //

    case 24:
        TrapFrame->IntT10 = Value;
        return;

        //
        // Integer register T11.
        //

    case 25:
        TrapFrame->IntT11 = Value;
        return;

        //
        // Integer register Ra.
        //

    case 26:
        TrapFrame->IntRa = Value;
        return;

        //
        // Integer register T12.
        //

    case 27:
        TrapFrame->IntT12 = Value;
        return;

        //
        // Integer register At.
        //

    case 28:
        TrapFrame->IntAt = Value;
        return;

        //
        // Integer register Gp.
        //

    case 29:
        TrapFrame->IntGp = Value;
        return;

        //
        // Integer register Sp.
        //

    case 30:
        TrapFrame->IntSp = Value;
        return;

        //
        // Integer register Zero.
        //

    case 31:
        return;

        //
        // Floating register F0.
        //

    case 32:
        TrapFrame->FltF0 = Value;
        return;

        //
        // Floating register F1.
        //

    case 33:
        TrapFrame->FltF1 = Value;
        return;

        //
        // Floating register F2.
        //

    case 34:
        ExceptionFrame->FltF2 = Value;
        return;

        //
        // Floating register F3.
        //

    case 35:
        ExceptionFrame->FltF3 = Value;
        return;

        //
        // Floating register F4.
        //

    case 36:
        ExceptionFrame->FltF4 = Value;
        return;

        //
        // Floating register F5.
        //

    case 37:
        ExceptionFrame->FltF5 = Value;
        return;

        //
        // Floating register F6.
        //

    case 38:
        ExceptionFrame->FltF6 = Value;
        return;

        //
        // Floating register F7.
        //

    case 39:
        ExceptionFrame->FltF7 = Value;
        return;

        //
        // Floating register F8.
        //

    case 40:
        ExceptionFrame->FltF8 = Value;
        return;

        //
        // Floating register F9.
        //

    case 41:
        ExceptionFrame->FltF9 = Value;
        return;

        //
        // Floating register F10.
        //

    case 42:
        TrapFrame->FltF10 = Value;
        return;

        //
        // Floating register F11.
        //

    case 43:
        TrapFrame->FltF11 = Value;
        return;

        //
        // Floating register F12.
        //

    case 44:
        TrapFrame->FltF12 = Value;
        return;

        //
        // Floating register F13.
        //

    case 45:
        TrapFrame->FltF13 = Value;
        return;

        //
        // Floating register F14.
        //

    case 46:
        TrapFrame->FltF14 = Value;
        return;

        //
        // Floating register F15.
        //

    case 47:
        TrapFrame->FltF15 = Value;
        return;

        //
        // Floating register F16.
        //

    case 48:
        TrapFrame->FltF16 = Value;
        return;

        //
        // Floating register F17.
        //

    case 49:
        TrapFrame->FltF17 = Value;
        return;

        //
        // Floating register F18.
        //

    case 50:
        TrapFrame->FltF18 = Value;
        return;

        //
        // Floating register F19.
        //

    case 51:
        TrapFrame->FltF19 = Value;
        return;

        //
        // Floating register F20.
        //

    case 52:
        TrapFrame->FltF20 = Value;
        return;

        //
        // Floating register F21.
        //

    case 53:
        TrapFrame->FltF21 = Value;
        return;

        //
        // Floating register F22.
        //

    case 54:
        TrapFrame->FltF22 = Value;
        return;

        //
        // Floating register F23.
        //

    case 55:
        TrapFrame->FltF23 = Value;
        return;

        //
        // Floating register F24.
        //

    case 56:
        TrapFrame->FltF24 = Value;
        return;

        //
        // Floating register F25.
        //

    case 57:
        TrapFrame->FltF25 = Value;
        return;

        //
        // Floating register F26.
        //

    case 58:
        TrapFrame->FltF26 = Value;
        return;

        //
        // Floating register F27.
        //

    case 59:
        TrapFrame->FltF27 = Value;
        return;

        //
        // Floating register F28.
        //

    case 60:
        TrapFrame->FltF28 = Value;
        return;

        //
        // Floating register F29.
        //

    case 61:
        TrapFrame->FltF29 = Value;
        return;

        //
        // Floating register F30.
        //

    case 62:
        TrapFrame->FltF30 = Value;
        return;

        //
        // Floating register F31 (Zero).
        //

    case 63:
        return;
    }
}
