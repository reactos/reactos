/*++

Copyright (c) 1991  Microsoft Corporation

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

--*/

#include "ki.h"

ULONG
KiGetRegisterValue (
    IN ULONG Register,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called to get the 32-bit value of a register from the
    specified exception or trap frame.

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

    if (Register == 0) {
        return 0;

    } else if (Register < 32) {
        return (ULONG)(&TrapFrame->XIntZero)[Register];

    } else {
        switch (Register) {

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
            return TrapFrame->FltF2;

            //
            // Floating register F3.
            //

        case 35:
            return TrapFrame->FltF3;

            //
            // Floating register F4.
            //

        case 36:
            return TrapFrame->FltF4;

            //
            // Floating register F5.
            //

        case 37:
            return TrapFrame->FltF5;

            //
            // Floating register F6.
            //

        case 38:
            return TrapFrame->FltF6;

            //
            // Floating register F7.
            //

        case 39:
            return TrapFrame->FltF7;

            //
            // Floating register F8.
            //

        case 40:
            return TrapFrame->FltF8;

            //
            // Floating register F9.
            //

        case 41:
            return TrapFrame->FltF9;

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
            return ExceptionFrame->FltF20;

            //
            // Floating register F21.
            //

        case 53:
            return ExceptionFrame->FltF21;

            //
            // Floating register F22.
            //

        case 54:
            return ExceptionFrame->FltF22;

            //
            // Floating register F23.
            //

        case 55:
            return ExceptionFrame->FltF23;

            //
            // Floating register F24.
            //

        case 56:
            return ExceptionFrame->FltF24;

            //
            // Floating register F25.
            //

        case 57:
            return ExceptionFrame->FltF25;

            //
            // Floating register F26.
            //

        case 58:
            return ExceptionFrame->FltF26;

            //
            // Floating register F27.
            //

        case 59:
            return ExceptionFrame->FltF27;

            //
            // Floating register F28.
            //

        case 60:
            return ExceptionFrame->FltF28;

            //
            // Floating register F29.
            //

        case 61:
            return ExceptionFrame->FltF29;

            //
            // Floating register F30.
            //

        case 62:
            return ExceptionFrame->FltF30;

            //
            // Floating register F31.
            //

        case 63:
            return ExceptionFrame->FltF31;
        }
    }
}

ULONGLONG
KiGetRegisterValue64 (
    IN ULONG Register,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called to get the 64-bit value of a register from the
    specified exception or trap frame.

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

    if (Register == 0) {
        return 0;

    } else if (Register < 32) {
        return (&TrapFrame->XIntZero)[Register];

    } else {
        switch (Register) {

            //
            // Floating register F0.
            //

        case 32:
            return TrapFrame->XFltF0;

            //
            // Floating register F1.
            //

        case 33:
            return TrapFrame->XFltF1;

            //
            // Floating register F2.
            //

        case 34:
            return TrapFrame->XFltF2;

            //
            // Floating register F3.
            //

        case 35:
            return TrapFrame->XFltF3;

            //
            // Floating register F4.
            //

        case 36:
            return TrapFrame->XFltF4;

            //
            // Floating register F5.
            //

        case 37:
            return TrapFrame->XFltF5;

            //
            // Floating register F6.
            //

        case 38:
            return TrapFrame->XFltF6;

            //
            // Floating register F7.
            //

        case 39:
            return TrapFrame->XFltF7;

            //
            // Floating register F8.
            //

        case 40:
            return TrapFrame->XFltF8;

            //
            // Floating register F9.
            //

        case 41:
            return TrapFrame->XFltF9;

            //
            // Floating register F10.
            //

        case 42:
            return TrapFrame->XFltF10;

            //
            // Floating register F11.
            //

        case 43:
            return TrapFrame->XFltF11;

            //
            // Floating register F12.
            //

        case 44:
            return TrapFrame->XFltF12;

            //
            // Floating register F13.
            //

        case 45:
            return TrapFrame->XFltF13;

            //
            // Floating register F14.
            //

        case 46:
            return TrapFrame->XFltF14;

            //
            // Floating register F15.
            //

        case 47:
            return TrapFrame->XFltF15;

            //
            // Floating register F16.
            //

        case 48:
            return TrapFrame->XFltF16;

            //
            // Floating register F17.
            //

        case 49:
            return TrapFrame->XFltF17;

            //
            // Floating register F18.
            //

        case 50:
            return TrapFrame->XFltF18;

            //
            // Floating register F19.
            //

        case 51:
            return TrapFrame->XFltF19;

            //
            // Floating register F20.
            //

        case 52:
            return ExceptionFrame->XFltF20;

            //
            // Floating register F21.
            //

        case 53:
            return TrapFrame->XFltF21;

            //
            // Floating register F22.
            //

        case 54:
            return ExceptionFrame->XFltF22;

            //
            // Floating register F23.
            //

        case 55:
            return TrapFrame->XFltF23;

            //
            // Floating register F24.
            //

        case 56:
            return ExceptionFrame->XFltF24;

            //
            // Floating register F25.
            //

        case 57:
            return TrapFrame->XFltF25;

            //
            // Floating register F26.
            //

        case 58:
            return ExceptionFrame->XFltF26;

            //
            // Floating register F27.
            //

        case 59:
            return TrapFrame->XFltF27;

            //
            // Floating register F28.
            //

        case 60:
            return ExceptionFrame->XFltF28;

            //
            // Floating register F29.
            //

        case 61:
            return TrapFrame->XFltF29;

            //
            // Floating register F30.
            //

        case 62:
            return ExceptionFrame->XFltF30;

            //
            // Floating register F31.
            //

        case 63:
            return TrapFrame->XFltF31;
        }
    }
}

VOID
KiSetRegisterValue (
    IN ULONG Register,
    IN ULONG Value,
    OUT PKEXCEPTION_FRAME ExceptionFrame,
    OUT PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called to set the 32-bit value of a register in the
    specified exception or trap frame.

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

    if (Register < 32) {
        (&TrapFrame->XIntZero)[Register] = (LONG)Value;

    } else {
        switch (Register) {

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
            TrapFrame->FltF2 = Value;
            return;

            //
            // Floating register F3.
            //

        case 35:
            TrapFrame->FltF3 = Value;
            return;

            //
            // Floating register F4.
            //

        case 36:
            TrapFrame->FltF4 = Value;
            return;

            //
            // Floating register F5.
            //

        case 37:
            TrapFrame->FltF5 = Value;
            return;

            //
            // Floating register F6.
            //

        case 38:
            TrapFrame->FltF6 = Value;
            return;

            //
            // Floating register F7.
            //

        case 39:
            TrapFrame->FltF7 = Value;
            return;

            //
            // Floating register F8.
            //

        case 40:
            TrapFrame->FltF8 = Value;
            return;

            //
            // Floating register F9.
            //

        case 41:
            TrapFrame->FltF9 = Value;
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
            ExceptionFrame->FltF20 = Value;
            return;

            //
            // Floating register F21.
            //

        case 53:
            ExceptionFrame->FltF21 = Value;
            return;

            //
            // Floating register F22.
            //

        case 54:
            ExceptionFrame->FltF22 = Value;
            return;

            //
            // Floating register F23.
            //

        case 55:
            ExceptionFrame->FltF23 = Value;
            return;

            //
            // Floating register F24.
            //

        case 56:
            ExceptionFrame->FltF24 = Value;
            return;

            //
            // Floating register F25.
            //

        case 57:
            ExceptionFrame->FltF25 = Value;
            return;

            //
            // Floating register F26.
            //

        case 58:
            ExceptionFrame->FltF26 = Value;
            return;

            //
            // Floating register F27.
            //

        case 59:
            ExceptionFrame->FltF27 = Value;
            return;

            //
            // Floating register F28.
            //

        case 60:
            ExceptionFrame->FltF28 = Value;
            return;

            //
            // Floating register F29.
            //

        case 61:
            ExceptionFrame->FltF29 = Value;
            return;

            //
            // Floating register F30.
            //

        case 62:
            ExceptionFrame->FltF30 = Value;
            return;

            //
            // Floating register F31.
            //

        case 63:
            ExceptionFrame->FltF31 = Value;
            return;
        }
    }
}

VOID
KiSetRegisterValue64 (
    IN ULONG Register,
    IN ULONGLONG Value,
    OUT PKEXCEPTION_FRAME ExceptionFrame,
    OUT PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called to set the 64-bit value of a register in the
    specified exception or trap frame.

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

    if (Register < 32) {
        (&TrapFrame->XIntZero)[Register] = Value;

    } else {
        switch (Register) {

            //
            // Floating register F0.
            //

        case 32:
            TrapFrame->XFltF0 = Value;
            return;

            //
            // Floating register F1.
            //

        case 33:
            TrapFrame->XFltF1 = Value;
            return;

            //
            // Floating register F2.
            //

        case 34:
            TrapFrame->XFltF2 = Value;
            return;

            //
            // Floating register F3.
            //

        case 35:
            TrapFrame->XFltF3 = Value;
            return;

            //
            // Floating register F4.
            //

        case 36:
            TrapFrame->XFltF4 = Value;
            return;

            //
            // Floating register F5.
            //

        case 37:
            TrapFrame->XFltF5 = Value;
            return;

            //
            // Floating register F6.
            //

        case 38:
            TrapFrame->XFltF6 = Value;
            return;

            //
            // Floating register F7.
            //

        case 39:
            TrapFrame->XFltF7 = Value;
            return;

            //
            // Floating register F8.
            //

        case 40:
            TrapFrame->XFltF8 = Value;
            return;

            //
            // Floating register F9.
            //

        case 41:
            TrapFrame->XFltF9 = Value;
            return;

            //
            // Floating register F10.
            //

        case 42:
            TrapFrame->XFltF10 = Value;
            return;

            //
            // Floating register F11.
            //

        case 43:
            TrapFrame->XFltF11 = Value;
            return;

            //
            // Floating register F12.
            //

        case 44:
            TrapFrame->XFltF12 = Value;
            return;

            //
            // Floating register F13.
            //

        case 45:
            TrapFrame->XFltF13 = Value;
            return;

            //
            // Floating register F14.
            //

        case 46:
            TrapFrame->XFltF14 = Value;
            return;

            //
            // Floating register F15.
            //

        case 47:
            TrapFrame->XFltF15 = Value;
            return;

            //
            // Floating register F16.
            //

        case 48:
            TrapFrame->XFltF16 = Value;
            return;

            //
            // Floating register F17.
            //

        case 49:
            TrapFrame->XFltF17 = Value;
            return;

            //
            // Floating register F18.
            //

        case 50:
            TrapFrame->XFltF18 = Value;
            return;

            //
            // Floating register F19.
            //

        case 51:
            TrapFrame->XFltF19 = Value;
            return;

            //
            // Floating register F20.
            //

        case 52:
            ExceptionFrame->XFltF20 = Value;
            return;

            //
            // Floating register F21.
            //

        case 53:
            TrapFrame->XFltF21 = Value;
            return;

            //
            // Floating register F22.
            //

        case 54:
            ExceptionFrame->XFltF22 = Value;
            return;

            //
            // Floating register F23.
            //

        case 55:
            TrapFrame->XFltF23 = Value;
            return;

            //
            // Floating register F24.
            //

        case 56:
            ExceptionFrame->XFltF24 = Value;
            return;

            //
            // Floating register F25.
            //

        case 57:
            TrapFrame->XFltF25 = Value;
            return;

            //
            // Floating register F26.
            //

        case 58:
            ExceptionFrame->XFltF26 = Value;
            return;

            //
            // Floating register F27.
            //

        case 59:
            TrapFrame->XFltF27 = Value;
            return;

            //
            // Floating register F28.
            //

        case 60:
            ExceptionFrame->XFltF28 = Value;
            return;

            //
            // Floating register F29.
            //

        case 61:
            TrapFrame->XFltF29 = Value;
            return;

            //
            // Floating register F30.
            //

        case 62:
            ExceptionFrame->XFltF30 = Value;
            return;

            //
            // Floating register F31.
            //

        case 63:
            TrapFrame->XFltF31 = Value;
            return;
        }
    }
}
