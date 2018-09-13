/*++

Copyright (c) 1993 IBM Corporation and Microsoft Corporation

Module Name:

    getsetrg.c

Abstract:

    This module implement the code necessary to get and set register values.
    These routines are used during the emulation of unaligned data references
    and floating point exceptions.

Author:

    Rick Simpson  6-Aug-1993

    Based on MIPS version by David N. Cutler (davec) 17-Jun-1991

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

    This function is called to get the value of a register from the specified
    exception or trap frame.

Arguments:

    Register - Supplies the number of the register whose value is to be
        returned.  Only GPRs (integer regs) are supported, numbered 0..31.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    The value of the specified register is returned as the function value.

--*/

{

    //
    // Dispatch on the GP register number.
    //

    switch (Register) {
      case 0:
        return TrapFrame->Gpr0;
      case 1:
	    return TrapFrame->Gpr1;
      case 2:
	    return TrapFrame->Gpr2;
      case 3:
        return TrapFrame->Gpr3;
      case 4:
        return TrapFrame->Gpr4;
      case 5:
        return TrapFrame->Gpr5;
      case 6:
        return TrapFrame->Gpr6;
      case 7:
        return TrapFrame->Gpr7;
      case 8:
        return TrapFrame->Gpr8;
      case 9:
        return TrapFrame->Gpr9;
      case 10:
        return TrapFrame->Gpr10;
      case 11:
        return TrapFrame->Gpr11;
      case 12:
        return TrapFrame->Gpr12;
      case 13:
	    return ExceptionFrame->Gpr13;
      case 14:
	    return ExceptionFrame->Gpr14;
      case 15:
	    return ExceptionFrame->Gpr15;
      case 16:
	    return ExceptionFrame->Gpr16;
      case 17:
	    return ExceptionFrame->Gpr17;
      case 18:
	    return ExceptionFrame->Gpr18;
      case 19:
	    return ExceptionFrame->Gpr19;
      case 20:
	    return ExceptionFrame->Gpr20;
      case 21:
	    return ExceptionFrame->Gpr21;
      case 22:
	    return ExceptionFrame->Gpr22;
      case 23:
	    return ExceptionFrame->Gpr23;
      case 24:
	    return ExceptionFrame->Gpr24;
      case 25:
	    return ExceptionFrame->Gpr25;
      case 26:
	    return ExceptionFrame->Gpr26;
      case 27:
	    return ExceptionFrame->Gpr27;
      case 28:
	    return ExceptionFrame->Gpr28;
      case 29:
	    return ExceptionFrame->Gpr29;
      case 30:
	    return ExceptionFrame->Gpr30;
      case 31:
	    return ExceptionFrame->Gpr31;
    }
    return(0);  // to eliminate a compiler warning
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

    This function is called to set the value of a register in the specified
    exception or trap frame.

Arguments:

    Register - Supplies the number of the register whose value is to be
        stored.  This routine handles only GPRs (integer regs), numbered 0..31.

    Value - Supplies the value to be stored in the specified register.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    None.

--*/

{

    //
    // Dispatch on the GP register number.
    //

    switch (Register) {

      case 0:
        TrapFrame->Gpr0 = Value;
	return;
      case 1:
	TrapFrame->Gpr1 = Value;
	return;
      case 2:
	TrapFrame->Gpr2 = Value;
	return;
      case 3:
        TrapFrame->Gpr3 = Value;
	return;
      case 4:
        TrapFrame->Gpr4 = Value;
	return;
      case 5:
        TrapFrame->Gpr5 = Value;
	return;
      case 6:
        TrapFrame->Gpr6 = Value;
	return;
      case 7:
        TrapFrame->Gpr7 = Value;
	return;
      case 8:
        TrapFrame->Gpr8 = Value;
	return;
      case 9:
        TrapFrame->Gpr9 = Value;
	return;
      case 10:
        TrapFrame->Gpr10 = Value;
	return;
      case 11:
        TrapFrame->Gpr11 = Value;
	return;
      case 12:
        TrapFrame->Gpr12 = Value;
	return;
      case 13:
	ExceptionFrame->Gpr13 = Value;
	return;
      case 14:
	ExceptionFrame->Gpr14 = Value;
	return;
      case 15:
	ExceptionFrame->Gpr15 = Value;
	return;
      case 16:
	ExceptionFrame->Gpr16 = Value;
	return;
      case 17:
	ExceptionFrame->Gpr17 = Value;
	return;
      case 18:
	ExceptionFrame->Gpr18 = Value;
	return;
      case 19:
	ExceptionFrame->Gpr19 = Value;
	return;
      case 20:
	ExceptionFrame->Gpr20 = Value;
	return;
      case 21:
	ExceptionFrame->Gpr21 = Value;
	return;
      case 22:
	ExceptionFrame->Gpr22 = Value;
	return;
      case 23:
	ExceptionFrame->Gpr23 = Value;
	return;
      case 24:
	ExceptionFrame->Gpr24 = Value;
	return;
      case 25:
	ExceptionFrame->Gpr25 = Value;
	return;
      case 26:
	ExceptionFrame->Gpr26 = Value;
	return;
      case 27:
	ExceptionFrame->Gpr27 = Value;
	return;
      case 28:
	ExceptionFrame->Gpr28 = Value;
	return;
      case 29:
	ExceptionFrame->Gpr29 = Value;
	return;
      case 30:
	ExceptionFrame->Gpr30 = Value;
	return;
      case 31:
	ExceptionFrame->Gpr31 = Value;
	return;

    }
}

DOUBLE
KiGetFloatRegisterValue (
    IN ULONG Register,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called to get the value of a floating point register 
    from the specified exception or trap frame.

Arguments:

    Register - Supplies the number of the register whose value is to be
        returned.  Only FPRs (float regs) are supported, numbered 0..31.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    The value of the specified register is returned as the function value.

--*/

{

    //
    // Dispatch on the FP register number.
    //

    switch (Register) {
      case 0:
        return TrapFrame->Fpr0;
      case 1:
	    return TrapFrame->Fpr1;
      case 2:
	    return TrapFrame->Fpr2;
      case 3:
        return TrapFrame->Fpr3;
      case 4:
        return TrapFrame->Fpr4;
      case 5:
        return TrapFrame->Fpr5;
      case 6:
        return TrapFrame->Fpr6;
      case 7:
        return TrapFrame->Fpr7;
      case 8:
        return TrapFrame->Fpr8;
      case 9:
        return TrapFrame->Fpr9;
      case 10:
        return TrapFrame->Fpr10;
      case 11:
        return TrapFrame->Fpr11;
      case 12:
        return TrapFrame->Fpr12;
      case 13:
	    return TrapFrame->Fpr13;
      case 14:
	    return ExceptionFrame->Fpr14;
      case 15:
	    return ExceptionFrame->Fpr15;
      case 16:
	    return ExceptionFrame->Fpr16;
      case 17:
	    return ExceptionFrame->Fpr17;
      case 18:
	    return ExceptionFrame->Fpr18;
      case 19:
	    return ExceptionFrame->Fpr19;
      case 20:
	    return ExceptionFrame->Fpr20;
      case 21:
	    return ExceptionFrame->Fpr21;
      case 22:
	    return ExceptionFrame->Fpr22;
      case 23:
	    return ExceptionFrame->Fpr23;
      case 24:
	    return ExceptionFrame->Fpr24;
      case 25:
	    return ExceptionFrame->Fpr25;
      case 26:
	    return ExceptionFrame->Fpr26;
      case 27:
	    return ExceptionFrame->Fpr27;
      case 28:
	    return ExceptionFrame->Fpr28;
      case 29:
	    return ExceptionFrame->Fpr29;
      case 30:
	    return ExceptionFrame->Fpr30;
      case 31:
	    return ExceptionFrame->Fpr31;
    }
}

VOID
KiSetFloatRegisterValue (
    IN ULONG Register,
    IN DOUBLE Value,
    OUT PKEXCEPTION_FRAME ExceptionFrame,
    OUT PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called to set the value of a floating point register 
    in the specified exception or trap frame.

Arguments:

    Register - Supplies the number of the register whose value is to be
        stored.  This routine handles only Fprs (float regs), numbered 0..31.

    Value - Supplies the value to be stored in the specified register.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    None.

--*/

{

    //
    // Dispatch on the FP register number.
    //

    switch (Register) {

      case 0:
        TrapFrame->Fpr0 = Value;
	return;
      case 1:
	TrapFrame->Fpr1 = Value;
	return;
      case 2:
	TrapFrame->Fpr2 = Value;
	return;
      case 3:
        TrapFrame->Fpr3 = Value;
	return;
      case 4:
        TrapFrame->Fpr4 = Value;
	return;
      case 5:
        TrapFrame->Fpr5 = Value;
	return;
      case 6:
        TrapFrame->Fpr6 = Value;
	return;
      case 7:
        TrapFrame->Fpr7 = Value;
	return;
      case 8:
        TrapFrame->Fpr8 = Value;
	return;
      case 9:
        TrapFrame->Fpr9 = Value;
	return;
      case 10:
        TrapFrame->Fpr10 = Value;
	return;
      case 11:
        TrapFrame->Fpr11 = Value;
	return;
      case 12:
        TrapFrame->Fpr12 = Value;
	return;
      case 13:
	TrapFrame->Fpr13 = Value;
	return;
      case 14:
	ExceptionFrame->Fpr14 = Value;
	return;
      case 15:
	ExceptionFrame->Fpr15 = Value;
	return;
      case 16:
	ExceptionFrame->Fpr16 = Value;
	return;
      case 17:
	ExceptionFrame->Fpr17 = Value;
	return;
      case 18:
	ExceptionFrame->Fpr18 = Value;
	return;
      case 19:
	ExceptionFrame->Fpr19 = Value;
	return;
      case 20:
	ExceptionFrame->Fpr20 = Value;
	return;
      case 21:
	ExceptionFrame->Fpr21 = Value;
	return;
      case 22:
	ExceptionFrame->Fpr22 = Value;
	return;
      case 23:
	ExceptionFrame->Fpr23 = Value;
	return;
      case 24:
	ExceptionFrame->Fpr24 = Value;
	return;
      case 25:
	ExceptionFrame->Fpr25 = Value;
	return;
      case 26:
	ExceptionFrame->Fpr26 = Value;
	return;
      case 27:
	ExceptionFrame->Fpr27 = Value;
	return;
      case 28:
	ExceptionFrame->Fpr28 = Value;
	return;
      case 29:
	ExceptionFrame->Fpr29 = Value;
	return;
      case 30:
	ExceptionFrame->Fpr30 = Value;
	return;
      case 31:
	ExceptionFrame->Fpr31 = Value;
	return;

    }
}
