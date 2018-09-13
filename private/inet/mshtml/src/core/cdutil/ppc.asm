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

#include "tearoff.hxx"

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

#define THUNK_DECLARE(n) .long  ..TearoffThunk##n, TearoffThunk##n##End, 0, 0, ..TearoffThunk##n ;

THUNK_ARRAY_3_TO_15(DECLARE)
THUNK_ARRAY_16_AND_UP(DECLARE)

.text
.align 2

#define THUNK_DEFINE_COMPARE(n)\
\
/* ------------------------ COMPARE THUNK NUMBER n ------------------------- */ \
.extern TearoffThunk##n, ..TearoffThunk##n                                     ;\
TearoffThunk##n:                                                               ;\
    .long ..TearoffThunk##n                                                    ;\
    .long .toc                                                                 ;\
    .text                                                                      ;\
..TearoffThunk##n:                                                             ;\
        TRAP1                                                                  ;\
        li      r12, 1                  /* load 1 into r12, and... */          ;\
        slwi    r12, r12, n             /* ...shift left by the index */       ;\
        lwz     r11, offsetof_Mask(r3)  /* load mask dword into r11 */         ;\
        and     r11, r11, r12                                                  ;\
        cmpli   1, 0, r11, 0            /* compare */                          ;\
        bc      12, 6, Obj##n                                                  ;\
        addi    r3, r3, 8               /* add for object 1 */                 ;\
Obj##n:                                                                        ;\
        addi    r3, r3, 12              /* add offset */                       ;\
            /* r3 is now the pvObject ptr, i.e. the intermediate 'this' ptr */ ;\
            /* BUGBUG: replace 2 lines with    li r12, n * 4 */                 \
    li      r12, n                      /* Put the method # in r12 */          ;\
    rlwinm  r12, r12, 2, 0, 31-2        /* Multiply by 4 to get byte offset */ ;\
    lwz     r11, 0x4(r3)                /* Copy this->apfn to r11 */           ;\
    lwzx    r11, r12, r11               /* access apfnObj[n] */                ;\
    lwz     r3, 0(r3)                   /* Copy pObj pointer to r3 */          ;\
                                        /* DO WE NEED TO DO THIS BLOCK? */      \
    lwz     r2, 4(r11)                  /* load new TOC (may be redundant?) */ ;\
    lwz     r11, 0(r11)                 /* load function pointer */            ;\
    mtctr   r11                         /* move fn addr to counter register */ ;\
    bctr                                /* branch to counter */                ;\
TearoffThunk##n##End:

//
//      Define the thunks from 3 to 15 (these are compare thunks)
//

THUNK_ARRAY_3_TO_15(DEFINE_COMPARE)


#define THUNK_DEFINE_SIMPLE(n)\
\
/*------------------------ SIMPLE THUNK NUMBER n ----------------------------*/;\
.extern TearoffThunk##n, ..TearoffThunk##n                                     ;\
TearoffThunk##n:                                                               ;\
    .long ..TearoffThunk##n                                                    ;\
    .long .toc                                                                 ;\
    .text                                                                      ;\
..TearoffThunk##n:                                                             ;\
        TRAP2                                                                  ;\
    li      r12, n                      /* Put the method # in r12 */          ;\
    rlwinm  r12, r12, 2, 0, 31-2        /* Multiply by 4 to get byte offset */ ;\
    lwz     r11, offsetof_apfn(r3)      /* Copy this->apfn to r11 */           ;\
    lwzx    r11, r12, r11               /* access apfnObj[n] */                ;\
    lwz     r3, offsetof_pvObject(r3)   /* get new 'this' pointer into r3 */   ;\
    lwz     r2, 4(r11)                  /* load new TOC (may be redundant?) */ ;\
    lwz     r11, 0(r11)                 /* load function pointer */            ;\
    mtctr   r11                         /* move fn addr to counter register */ ;\
    bctr                                /* branch to counter */                ;\
TearoffThunk##n##End:

//
//      Define the thunks from 16 onwards (these are simple thunks)
//

THUNK_ARRAY_16_AND_UP(DEFINE_SIMPLE)
