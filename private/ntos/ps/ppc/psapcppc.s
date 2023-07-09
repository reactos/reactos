//      TITLE("PspGetSetContextSpecialAPC")
//++
//
//  Copyright (c) 1994  IBM Corporation
//
//  Module Name:
//
//     psapcppc.s
//
//  Abstract:
//
//     Since APCs are implemented in software on the PowerPC, it is possible
//     that all the non-volatile registers are not restored prior to returning
//     to user mode. This procedure saves and restores all non-volatiles to
//     ensure that the full context (set via the PspGetSetContextSpecialAPC)
//     is returned to the thread.
//
//  Author:
//
//    Chuck Bauman 26-Sep-1994
//
//  Environment:
//
//     Kernel mode.
//
//  Revision History:
//
//--

#include "ksppc.h"


//++
//
// VOID
// PspGetSetContextSpecialAPC(
//    IN PKAPC Apc,
//    IN PKNORMAL_ROUTINE *NormalRoutine,
//    IN PVOID *NormalContext,
//    IN PVOID *SystemArgument1,
//    IN PVOID *SystemArgument2
//    )
//
// Routine Description:
//
//    Ensure that all non-volatiles are stored on the kernel mode stack
//    so that the normal save/restore prologue/epilogue code restores
//    the entire context after the set context APC prior to returning.
//
//  Arguments:
//
//  Apc (r.3) - Supplies a pointer to the APC control object that caused entry
//        into this routine.
//
//  NormalRoutine (r.4) - Supplies a pointer to the normal routine function
//      that was specified when the APC was initialized. This parameter is not
//      used.
//
//  NormalContext (r.5) - Supplies a pointer to an arbitrary data structure
//      that was specified when the APC was initialized. This parameter is not
//      used.
//
//  SystemArgument1, SystemArgument2 (r.6,r.7) - Supplies a set of two
//      pointer arguments that contain untyped data. These parameters
//      are not used.
//
// Return Value:
//
//    None.
//
//--

        .extern ..PspGetSetContextApc

        .struct 0
        .space  STK_MIN_FRAME
Lr:     .space  4
        .align  3
ApcEx:  .space  ExceptionFrameLength
        .align  3
ApcFrameLen:

        SPECIAL_ENTRY(PspGetSetContextSpecialApc)
        mflr    r.0
        stwu    r.sp, -ApcFrameLen(r.sp)        // buy stack frame
        stw     r.14, ApcEx + ExGpr14 (r.sp)    // Store non-volatile GPRs
        stw     r.15, ApcEx + ExGpr15 (r.sp)
        stw     r.16, ApcEx + ExGpr16 (r.sp)
        stw     r.17, ApcEx + ExGpr17 (r.sp)
        stw     r.18, ApcEx + ExGpr18 (r.sp)
        stw     r.19, ApcEx + ExGpr19 (r.sp)
        stw     r.20, ApcEx + ExGpr20 (r.sp)
        stw     r.21, ApcEx + ExGpr21 (r.sp)
        stw     r.22, ApcEx + ExGpr22 (r.sp)
        stw     r.23, ApcEx + ExGpr23 (r.sp)
        stw     r.24, ApcEx + ExGpr24 (r.sp)
        stw     r.25, ApcEx + ExGpr25 (r.sp)
        stw     r.26, ApcEx + ExGpr26 (r.sp)
        stw     r.27, ApcEx + ExGpr27 (r.sp)
        stw     r.28, ApcEx + ExGpr28 (r.sp)
        stw     r.29, ApcEx + ExGpr29 (r.sp)
        stw     r.30, ApcEx + ExGpr30 (r.sp)
        stw     r.31, ApcEx + ExGpr31 (r.sp)
        stfd    f.14, ApcEx + ExFpr14 (r.sp)    // Store non-volatile FPRs
        stfd    f.15, ApcEx + ExFpr15 (r.sp)
        stfd    f.16, ApcEx + ExFpr16 (r.sp)
        stfd    f.17, ApcEx + ExFpr17 (r.sp)
        stfd    f.18, ApcEx + ExFpr18 (r.sp)
        stfd    f.19, ApcEx + ExFpr19 (r.sp)
        stfd    f.20, ApcEx + ExFpr20 (r.sp)
        stfd    f.21, ApcEx + ExFpr21 (r.sp)
        stfd    f.22, ApcEx + ExFpr22 (r.sp)
        stfd    f.23, ApcEx + ExFpr23 (r.sp)
        stfd    f.24, ApcEx + ExFpr24 (r.sp)
        stfd    f.25, ApcEx + ExFpr25 (r.sp)
        stfd    f.26, ApcEx + ExFpr26 (r.sp)
        stfd    f.27, ApcEx + ExFpr27 (r.sp)
        stfd    f.28, ApcEx + ExFpr28 (r.sp)
        stfd    f.29, ApcEx + ExFpr29 (r.sp)
        stfd    f.30, ApcEx + ExFpr30 (r.sp)
        stfd    f.31, ApcEx + ExFpr31 (r.sp)
        stw     r.0,  Lr (r.sp)                 // save return address
        PROLOGUE_END(PspGetSetContextSpecialApc)

        bl      ..PspGetSetContextApc

        lwz     r.0,  Lr (r.sp)                 // restore the return address
        lwz     r.14, ApcEx + ExGpr14 (r.sp)    // Store non-volatile GPRs
        lwz     r.15, ApcEx + ExGpr15 (r.sp)
        lwz     r.16, ApcEx + ExGpr16 (r.sp)
        lwz     r.17, ApcEx + ExGpr17 (r.sp)
        lwz     r.18, ApcEx + ExGpr18 (r.sp)
        lwz     r.19, ApcEx + ExGpr19 (r.sp)
        lwz     r.20, ApcEx + ExGpr20 (r.sp)
        lwz     r.21, ApcEx + ExGpr21 (r.sp)
        lwz     r.22, ApcEx + ExGpr22 (r.sp)
        lwz     r.23, ApcEx + ExGpr23 (r.sp)
        lwz     r.24, ApcEx + ExGpr24 (r.sp)
        lwz     r.25, ApcEx + ExGpr25 (r.sp)
        lwz     r.26, ApcEx + ExGpr26 (r.sp)
        lwz     r.27, ApcEx + ExGpr27 (r.sp)
        lwz     r.28, ApcEx + ExGpr28 (r.sp)
        lwz     r.29, ApcEx + ExGpr29 (r.sp)
        lwz     r.30, ApcEx + ExGpr30 (r.sp)
        lwz     r.31, ApcEx + ExGpr31 (r.sp)
        mtlr    r.0
        lfd     f.14, ApcEx + ExFpr14 (r.sp)    // Store non-volatile FPRs
        lfd     f.15, ApcEx + ExFpr15 (r.sp)
        lfd     f.16, ApcEx + ExFpr16 (r.sp)
        lfd     f.17, ApcEx + ExFpr17 (r.sp)
        lfd     f.18, ApcEx + ExFpr18 (r.sp)
        lfd     f.19, ApcEx + ExFpr19 (r.sp)
        lfd     f.20, ApcEx + ExFpr20 (r.sp)
        lfd     f.21, ApcEx + ExFpr21 (r.sp)
        lfd     f.22, ApcEx + ExFpr22 (r.sp)
        lfd     f.23, ApcEx + ExFpr23 (r.sp)
        lfd     f.24, ApcEx + ExFpr24 (r.sp)
        lfd     f.25, ApcEx + ExFpr25 (r.sp)
        lfd     f.26, ApcEx + ExFpr26 (r.sp)
        lfd     f.27, ApcEx + ExFpr27 (r.sp)
        lfd     f.28, ApcEx + ExFpr28 (r.sp)
        lfd     f.29, ApcEx + ExFpr29 (r.sp)
        lfd     f.30, ApcEx + ExFpr30 (r.sp)
        lfd     f.31, ApcEx + ExFpr31 (r.sp)
        addi    r.sp, r.sp, ApcFrameLen
        SPECIAL_EXIT(PspGetSetContextSpecialApc)

