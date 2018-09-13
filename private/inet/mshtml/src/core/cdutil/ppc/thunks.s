//***
//  ppcmac.asm
//
//  Copyright (c) 1995, Microsoft Corporation. All rights reserved.
//
//**********************************************************************
//
// Support routines and data for the dynamic vtables and tearoff interfaces
//
//**********************************************************************

#define offsetof_pvObject    12      // Keep in sync w/ TOFF.CXX
#define offsetof_apfn        16
#define offsetof_Mask        28

// #define _DEBUG_TRAP

#define TRAP1
#define TRAP2

#ifdef  _DEBUG_TRAP
#define TRAP trap
#else
#define TRAP
#endif


//****************
//*
//* DESCRIPTION
//*
//* This function provides the meat of the Dynamic Vtable functionality.  It
//* is called from the code that is in the g_pbThunks[] array.  Its behavior
//* is similar to the Windows code.
//*
//* The method # is passed in in R3 (set by the g_pbThunks code).  The same
//* mechanism as Windows uses is employed to call the thunk function.
//*
//* The thunk table code relies on the fact that the TOC will be the same for
//* DynVtable as well as the code in the thunk table.
//*
//* An extra step (beyond the Windows code) is needed to set up the thunk code:
//* the 4 words in DynLinkFunc need to be copied into g_abThunk.  This appears
//* be the easiest way to set this up.
//*
//* The fucntion EnsureDynamicVtable() will then insert the method #, and the
//* pointer to the function inside the table.  This allows the code to maintain
//* very close similarity to Windows.
//*
//* Currently the thunk table is not really dynamic.  It is preset to an arbitrarily
//* large size.  Nor has it been optimized to PowerPC yet.
//*
//* ON ENTRY
//*
//*      r0  - Method #
//*      r3  - this
//*      The stack frame is of the original caller.
//*
//* REGISTER USAGE
//*
//*      r0  - temp
//*      r11 - temp
//*
//* ON EXIT
//*
//*      r3 - this
//*      r11 - Method #
//*      r12 - The transition vector address (thunk function being called)
//*
//****************

.pdata

.text
.align 2

#define THUNK_C_1(n)    .long  ..TearoffThunk##n, TearoffThunk##n##End, 0, 0, ..TearoffThunk##n ;
#define THUNK_C_2(n)    .extern TearoffThunk##n, ..TearoffThunk##n
#define THUNK_C_3(n)    TearoffThunk##n:
#define THUNK_C_4(n)        .long ..TearoffThunk##n
#define THUNK_C_5(n)        .long .toc
#define THUNK_C_6(n)        .text
#define THUNK_C_7(n)    ..TearoffThunk##n:
#define THUNK_C_8(n)        TRAP1
#define THUNK_C_9(n)        li      r12, 1                  /* load 1 into r12, and... */
#define THUNK_C_10(n)       slwi    r12, r12, n             /* ...shift left by the index */
#define THUNK_C_11(n)       lwz     r11, offsetof_Mask(r3)  /* load mask dword into r11 */
#define THUNK_C_12(n)       and     r11, r11, r12
#define THUNK_C_13(n)       cmpli   1, 0, r11, 0            /* compare */
#define THUNK_C_14(n)       bc      12, 6, Obj##n
#define THUNK_C_15(n)       addi    r3, r3, 8               /* add for object 1 */
#define THUNK_C_16(n) Obj##n:addi    r3, r3, 12             /* add offset */

                        // r3 is now the pvObject ptr, i.e. the intermediate 'this' ptr
#define THUNK_C_17(n)       li      r12, n                  /* Put the method # in r12 */
#define THUNK_C_18(n)       rlwinm  r12, r12, 2, 0, 31-2    /* Multiply by 4 to get byte offset */
#define THUNK_C_19(n)       lwz     r11, 0x4(r3)            /* Copy this->apfn to r11 */
#define THUNK_C_20(n)       lwzx    r11, r12, r11           /* access apfnObj[n] */
#define THUNK_C_21(n)       lwz     r3, 0(r3)               /* Copy pObj pointer to r3 */
                        // CONSIDER: do we need this following block? (sumitc)
#define THUNK_C_22(n)       lwz     r2, 4(r11)              /* load new TOC (may be redundant?) */
#define THUNK_C_23(n)       lwz     r11, 0(r11)             /* load function pointer */
#define THUNK_C_24(n)       mtctr   r11                     /* move fn addr to counter register */
#define THUNK_C_25(n)       bctr                            /* branch to counter */
#define THUNK_C_26(n)   TearoffThunk##n##End:
#define THUNK_C_27(n)
#define THUNK_C_28(n)
#define THUNK_C_29(n)
#define THUNK_C_30(n)

//
//      Define the thunks from 3 to 15 (these are compare thunks)
//

#include "..\thunks_c.h"


#define THUNK_S_1(n)    .long  ..TearoffThunk##n, TearoffThunk##n##End, 0, 0, ..TearoffThunk##n
#define THUNK_S_2(n)    .extern TearoffThunk##n, ..TearoffThunk##n
#define THUNK_S_3(n)    TearoffThunk##n:
#define THUNK_S_4(n)        .long ..TearoffThunk##n
#define THUNK_S_5(n)        .long .toc
#define THUNK_S_6(n)        .text
#define THUNK_S_7(n)    ..TearoffThunk##n:
#define THUNK_S_8(n)        TRAP2
#define THUNK_S_9(n)        li      r12, n                      /* Put the method # in r12 */
#define THUNK_S_10(n)       rlwinm  r12, r12, 2, 0, 31-2        /* Multiply by 4 to get byte offset */
#define THUNK_S_11(n)       lwz     r11, offsetof_apfn(r3)      /* Copy this->apfn to r11 */
#define THUNK_S_12(n)       lwzx    r11, r12, r11               /* access apfnObj[n] */
#define THUNK_S_13(n)       lwz     r3, offsetof_pvObject(r3)   /* get new 'this' pointer into r3 */
#define THUNK_S_14(n)       lwz     r2, 4(r11)                  /* load new TOC (may be redundant?) */
#define THUNK_S_15(n)       lwz     r11, 0(r11)                 /* load function pointer */
#define THUNK_S_16(n)       mtctr   r11                         /* move fn addr to counter register */
#define THUNK_S_17(n)       bctr                                /* branch to counter */
#define THUNK_S_18(n)   TearoffThunk##n##End:
#define THUNK_S_19(n)
#define THUNK_S_20(n)
//
//      Define the thunks from 16 onwards (these are simple thunks)
//

#include "..\thunks_s.h"

