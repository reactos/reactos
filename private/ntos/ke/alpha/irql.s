//      TITLE("Manipulate Interrupt Request Level")
//++
//
// Copyright (c) 1990  Microsoft Corporation
// Copyright (c) 1992  Microsoft Corporation
//
// Module Name:
//
//    irql.s
//
// Abstract:
//
//    This module implements the code necessary to lower and raise the current
//    Interrupt Request Level (IRQL).
//
//
// Author:
//
//    David N. Cutler (davec) 12-Aug-1990
//    Joe Notarangelo 06-Apr-1992
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksalpha.h"

//++
//
// VOID
// KeLowerIrql (
//    KIRQL NewIrql
//    )
//
// Routine Description:
//
//    This function lowers the current IRQL to the specified value.
//
// Arguments:
//
//    NewIrql (a0) - Supplies the new IRQL value.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KeLowerIrql)

	SWAP_IRQL			// a0 = new, on return v0 = old irql

	ret	zero, (ra)		// return

        .end    KeLowerIrql

//++
//
// VOID
// KeRaiseIrql (
//    KIRQL NewIrql,
//    PKIRQL OldIrql
//    )
//
// Routine Description:
//
//    This function raises the current IRQL to the specified value and returns
//    the old IRQL value.
//
// Arguments:
//
//    NewIrql (a0) - Supplies the new IRQL value.
//
//    OldIrql (a1) - Supplies a pointer to a variable that recieves the old
//       IRQL value.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KeRaiseIrql)

	bis	a1, zero, t0		// save pointer to old irql

	SWAP_IRQL			// a0 = new, on return v0 = old irql

	ldq_u	t1, 0(t0)		// get quadword around old irql
	bic	t0, 0x3, t3		// get containing longword address
	insbl	v0, t0, t2		// put destination byte into position
	mskbl	t1, t0, t1		// clear destination byte
	bis	t1, t2, t1		// merge destination byte
	extll	t1, t3, t1		// get appropriate longword
	stl	t1, 0(t3)		// store byte
	ret	zero, (ra)		// return

        .end    KeRaiseIrql
