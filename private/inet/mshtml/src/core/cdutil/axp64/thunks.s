 //----------------------------------------------------------------------------
 //
 // File:     thunks.s
 //
 // Contains: Assembly code for the Alpha. Implements the dynamic vtable stuff
 //           and the tearoff code.
 //
 // NOTE!!: This file (after preprocessing) is what would normally be passed
 //         to ASAXP.EXE, the Alpha assembler.  Unfortunately that causes the
 //         assembler to GPF.  Instead the preprocessed version is hand-edited
 //         to replace all semicolons with newlines, remove all unnecessary
 //         spaces and replace all macros.
 //
 //----------------------------------------------------------------------------

// offsetof_pvObject   = 24  // Must be kept in sync with the source code
// offsetof_apfn       = 32  // Ditto
// offsetof_mask       = 56

// rThis       = $a0

// rpObj       = $t1
// rpFnTable   = $t2
// rOffset     = $t3
// rMask       = $t4
// rJump       = $t5
// rTemp       = $t6
// rTmp2       = $t7

.align 3

 // Ensure that this function stays exactly how we expect
.set noreorder
.set nomacro

 //----------------------------------------------------------------------------
 //
 //  Function:  TearOffCompareThunk
 //
 //  Synopsis:  The "handler" function that handles calls to torn-off interfaces
 //
 //  Notes:     Delegates to methods in the function pointer array held by
 //             the CTearOffThunk class
 //
 //----------------------------------------------------------------------------

 // here's the layout of the 'this' object in $a0 (i.e. $a0) is
 //      0      void *      pafnVtblThis
 //      8      DWORD       ulRef
 //     12      DWORD       (padding on 64-bit systems)
 //     16      IID **      apIID
 //     24      void *      pvObject1's this
 //     32      void *      pvObject1's function table
 //     40      void *      pvObject2's this
 //     48      void *      pvObject2's function table
 //     56      DWORD       mask to decide whether to use Object 1 or 2
 //     60      DWORD       index of method into vtbl
 //     64      void *      apVtblPropDesc
 //     72      sizeof(TEAROFF_THUNK)


#define THUNK_C_1(n)    .globl TearoffThunk##n;
#define THUNK_C_2(n)    .ent TearoffThunk##n;
#define THUNK_C_3(n)    TearoffThunk##n:
#define THUNK_C_4(n)        clr     $t3;                /* set rOffset to zero */
#define THUNK_C_5(n)        ldq     $t4, 56($a0);       /* rThis+56 is the Mask */
#define THUNK_C_6(n)        mov     0x1, $t6;           /* move 1 into rTemp */
#define THUNK_C_7(n)        sll     $t6, n, $t6;        /* shift to the left by n */
#define THUNK_C_8(n)        and     $t4, $t6, $t4;      /* compare rMask and (1 << n) */
#define THUNK_C_9(n)        beq     $t4, Obj##n;
#define THUNK_C_10(n)       addq    $t3, 16, $t3;        /* object 2 offset is 20 ((2+3)*4) */
#define THUNK_C_11(n) Obj##n:addq    $t3, 24, $t3;       /* object 1 offset is 12 ((3)*4) */
                                                        /* get correct pvObject */
#define THUNK_C_12(n)       addq    $t3, $a0, $t1;      /* rpObj = rThis + rOffset */
#define THUNK_C_13(n)       mov     $a0, $t7;           /* store the tearoff ptr in t7 */
#define THUNK_C_14(n)       mov     n, $t8;             /* n must be in a register */
#define THUNK_C_15(n)       stl     $t8, 60($a0);       /* store index of called method in thunk */
#define THUNK_C_16(n)       ldq     $a0, 0($t1);        /* place obj ptr in 'this' register */
#define THUNK_C_17(n)       ldq     $t2, 8($t1);        /* and get function-ptr array */
#define THUNK_C_18(n)       ldq     $t5, (8 * n)($t2);  /* compute function pointer */
#define THUNK_C_19(n)       jmp     ($t5);              /* ... and call function */
#define THUNK_C_20(n)   .end TearoffThunk##n;
#define THUNK_C_21(n)
#define THUNK_C_22(n)
#define THUNK_C_23(n)
#define THUNK_C_24(n)
#define THUNK_C_25(n)
#define THUNK_C_26(n)
#define THUNK_C_27(n)
#define THUNK_C_28(n)
#define THUNK_C_29(n)
#define THUNK_C_30(n)

 //----------------------------------------------------------------------------
 //
 //  Function:  GetTearoff
 //
 //  Synopsis:  This function returns the tearoff thunk pointer stored in
 //             the temp register t7. This should be called first thing from
 //             the C++ functions that handles calls to torn-off interfaces
 //
 //----------------------------------------------------------------------------

#define THUNK_GT_1    .globl _GetTearoff;
#define THUNK_GT_2    .ent _GetTearoff;
#define THUNK_GT_3  _GetTearoff:
#define THUNK_GT_4    .frame $sp, 0, $ra;         /* No frame */
#define THUNK_GT_5    .prologue 0;                /*  and no prologue */
#define THUNK_GT_6        mov     $t7, $v0;       /* place tearoff ptr in return reg */
#define THUNK_GT_7        mov     $ra, $t8;       /* place tearoff ptr in return reg */
#define THUNK_GT_8        jmp     ($t8);          /* jump back to caller */
#define THUNK_GT_9   .end _GetTearoff;

//
//      Define the thunks from 3 to 15 (these are compare thunks)
//

#include "..\thunks_c.h"

 //----------------------------------------------------------------------------
 //
 //  Function:  CallTearOffSimpleThunk
 //
 //  Synopsis:  The "handler" function that handles calls to torn-off interfaces
 //
 //  Notes:     Delegates to methods in the function pointer array held by
 //             the CTearOffThunk class
 //
 //----------------------------------------------------------------------------


#define THUNK_S_1(n)    .globl TearoffThunk##n;
#define THUNK_S_2(n)    .ent TearoffThunk##n;
#define THUNK_S_3(n)  TearoffThunk##n:
#define THUNK_S_4(n)    .frame $sp, 0, $ra;         /* No frame */
#define THUNK_S_5(n)    .prologue 0;                /*  and no prologue */
#define THUNK_S_6(n)        mov     $a0, $t7;       /* store the tearoff ptr in t7 */
#define THUNK_S_7(n)        ldq     $t1, 24($a0);   /* Get object pointer */
#define THUNK_S_8(n)        ldq     $t2, 32($a0);   /* get function-ptr array */
#define THUNK_S_9(n)        mov     $t1, $a0;       /* place obj ptr in 'this' */
#define THUNK_S_10(n)       mov     n, $t6;         /* n must be in a register */
#define THUNK_S_11(n)       s8addq  $t6, $t2, $t2;  /* compute function pointer */
#define THUNK_S_12(n)       stl     $t6, 60($t7);   /* store index of called method in thunk */
#define THUNK_S_13(n)       ldq     $t5, 0x0($t2);  /* Get the fn pointer */
#define THUNK_S_14(n)       jmp     ($t5);          /* ... and call function */
#define THUNK_S_15(n)   .end TearoffThunk##n;
#define THUNK_S_16(n)
#define THUNK_S_17(n)
#define THUNK_S_18(n)
#define THUNK_S_19(n)
#define THUNK_S_20(n)

//
//      Define the thunks from 16 onwards (these are simple thunks)
//

#include "..\thunks_s.h"

.set macro
.set reorder
