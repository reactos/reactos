//      TITLE("Get Set Context Special Apc Wrapper")
//++
//
// Copyright (c) 1993  Digital Equipment Corporation
//
// Module Name:
//
//    pswrap.s
//
// Abstract:
//
//    This module implements the PspGetSetContextSpecialApc function as an
//    assembly wrapper around the call to the real function.
//
// Author:
//
//    Thomas Van Baak (tvb) 26-Jul-1993
//
// Environment:
//
//    Kernel mode.
//
// Revision History:
//
//--

#include "ksalpha.h"

        SBTTL("Set Context Special Apc Wrapper")

//++
//
// VOID
// PspGetSetContextSpecialApc (
//    IN PKAPC Apc,
//    IN PKNORMAL_ROUTINE *NormalRoutine,
//    IN PVOID *NormalContext,
//    IN PVOID *SystemArgument1,
//    IN PVOID *SystemArgument2
//    )
//
// Routine Description:
//
//    This is a wrapper around the call to PspGetSetContextSpecialApcMain.
//    Its purpose is to save all preserved registers on the stack so the
//    virtual unwind loop in PspGetSetContextSpecialApc is guaranteed to find
//    that all preserved registers have been saved on the stack. The algorithm
//    used to query or modify thread context assumes all registers are stored
//    somewhere on the stack: either in the trap frame itself, or in register
//    save areas of any intervening stack frames.
//
// Arguments:
//
//    Apc - Supplies a pointer to the APC control object that caused entry
//          into this routine.
//
//    NormalRoutine - Supplies a pointer to the normal routine function that
//        was specified when the APC was initialized. This parameter is not
//        used.
//
//    NormalContext - Supplies a pointer to an arbitrary data structure that
//        was specified when the APC was initialized. This parameter is not
//        used.
//
//    SystemArgument1, SystemArgument2 - Supplies a set of two pointer to two
//        arguments that contain untyped data. These parameters are not used.
//
// Return Value:
//
//    None.
//
//--

        .struct 0
StRa:   .space  8                       // return address
StS0:   .space  8                       // 7 non-volatile integer registers
StS1:   .space  8                       //
StS2:   .space  8                       //
StS3:   .space  8                       //
StS4:   .space  8                       //
StS5:   .space  8                       //
StFp:   .space  8                       //
StF2:   .space  8                       // 8 non-volatile floating registers
StF3:   .space  8                       //
StF4:   .space  8                       //
StF5:   .space  8                       //
StF6:   .space  8                       //
StF7:   .space  8                       //
StF8:   .space  8                       //
StF9:   .space  8                       //
        .space  0 * 8                   // padding for 16-byte stack alignment
StFrameSize:                            //

        NESTED_ENTRY(PspGetSetContextSpecialApc, StFrameSize, zero)

        lda     sp, -StFrameSize(sp)    // allocate stack frame
        stq     ra, StRa(sp)            // save return address

        stq     s0, StS0(sp)            // save non-volatile integer registers
        stq     s1, StS1(sp)            //
        stq     s2, StS2(sp)            //
        stq     s3, StS3(sp)            //
        stq     s4, StS4(sp)            //
        stq     s5, StS5(sp)            //
//        stq     fp, StFp(sp)            //

        stt     f2, StF2(sp)            // save non-volatile floating registers
        stt     f3, StF3(sp)            //
        stt     f4, StF4(sp)            //
        stt     f5, StF5(sp)            //
        stt     f6, StF6(sp)            //
        stt     f7, StF7(sp)            //
        stt     f8, StF8(sp)            //
        stt     f9, StF9(sp)            //

        PROLOGUE_END

//
// Pass the same five arguments to PspGetSetContextSpecialApcMain.
//

        bsr     ra, PspGetSetContextSpecialApcMain
        ldq     ra, StRa(sp)            // restore return address

        ldq     s0, StS0(sp)            // restore saved integer registers
        ldq     s1, StS1(sp)            //
        ldq     s2, StS2(sp)            //
        ldq     s3, StS3(sp)            //
        ldq     s4, StS4(sp)            //
        ldq     s5, StS5(sp)            //
//        ldq     fp, StFp(sp)            //

        ldt     f2, StF2(sp)            // restore saved floating registers
        ldt     f3, StF3(sp)            //
        ldt     f4, StF4(sp)            //
        ldt     f5, StF5(sp)            //
        ldt     f6, StF6(sp)            //
        ldt     f7, StF7(sp)            //
        ldt     f8, StF8(sp)            //
        ldt     f9, StF9(sp)            //

        lda     sp, StFrameSize(sp)     // deallocate stack frame
        ret     zero, (ra)              // return

        .end    PspGetSetContextSpecialApc
