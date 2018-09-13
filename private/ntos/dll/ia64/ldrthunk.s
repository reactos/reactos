//++
//
// Copyright (c) 1996  Intel Corporation
// Copyright (c) 1989  Microsoft Corporation
//
// Module Name:
//
//    ldrthunk.s
//
// Abstract:
//
//    This module implements the thunk for the LdrpInitialize APC routine.
//
// Author:
//
//    William K. Cheung (wcheung) 19-Sep-95
//
// Revision History:
//
//    08-Feb-96    Updated to EAS2.1
//
//--

#include "ksia64.h"

        .file    "ldrthunk.s"

        PublicFunction(LdrpInitialize)

//++
//
// VOID
// LdrInitializeThunk(
//    IN PVOID NormalContext,
//    IN PVOID SystemArgument1,
//    IN PVOID SystemArgument2
//    )
//
// Routine Description:
//
//    This function computes a pointer to the context record on the stack
//    and jumps to the LdrpInitialize function with that pointer as its
//    parameter.
//
// Arguments:
//
//    NormalContext (a0) - User Mode APC context parameter
//
//    SystemArgument1 (a1) - User Mode APC system argument 1
//
//    SystemArgument2 (a2) - User Mode APC system argument 2
//
// Return Value:
//
//    None.
//
//--

        NESTED_ENTRY(LdrInitializeThunk)

        NESTED_SETUP(3,2,3,0)
        mov         out2 = a2
        ;;

        PROLOGUE_END

//
// SP points at stack scratch area followed by context record.
//

        add         out0 = STACK_SCRATCH_AREA, sp       // pointer to context
        mov         out1 = a1                           // copy args
        br.call.sptk.many brp = LdrpInitialize

//
// S0 in the context record contains the PLabel.  Fix IIP/GP in the context
// record with the entry point and gp values pointed to by S0.
//

        add         t7 = CxIntGp+STACK_SCRATCH_AREA, sp     // -> global pointer
        add         t2 = CxIntS0+STACK_SCRATCH_AREA, sp     // -> PLabel pointer
        add         t11 = CxStIIP+STACK_SCRATCH_AREA, sp    // -> func pointer
        ;;

        ld8         t8 = [t2]                   // get the function pointer
        mov         brp = savedbrp              // restore brp
        mov         ar.pfs = savedpfs           // restore pfs
        ;;

        ld8         t5 = [t8], PlGlobalPointer-PlEntryPoint  // get entry point
        ;;
        ld8         t6 = [t8]                   // get gp
        nop.i       0
        ;;

        st8         [t11] = t5                  // set iip to entry point
        st8         [t7] = t6                   // set gp
        br.ret.sptk.clr brp

        NESTED_EXIT(LdrInitializeThunk)

