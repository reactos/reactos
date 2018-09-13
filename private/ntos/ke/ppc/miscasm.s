//++
//
// Copyright (c) 1993  IBM Corporation
//
// Module Name:
//
//    miscasm.s
//
// Abstract:
//
//    This module implements machine dependent miscellaneous kernel functions.
//
// Author:
//
//    Rick Simpson   July 26, 1993
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//    plj   September 15, 1993    Added routines KiDisableInterrupts and
//                                KiRestoreInterrupts.
//    Mark Mergen  09/93-10/93  Ke/KiFlush/FillTb KiSwapProcess subroutines.
//    Pat Carr     11/93        Mods for 603: Ke/KiFlush/FillTb routines.
//    Ying Chan    02/94        Mods for 604: Ke/KiFlush/FillTb routines.
//    plj          09/94        MP support + use PIDs for VSIDs
//    plj          02/95        KiSwapProcess moved to ctxswap.s
//    patcarr      02/95        Added support for 603+, 604+
//
//--

//list(off)
#include "ksppc.h"
//list(on)

//      Globals referenced within this file:

        .globl  ..KiContinue
        .globl  ..KeTestAlertThread
        .globl  ..KiExceptionExit
        .globl  ..KiRaiseException


//++
//
// KPCR *
// KiGetPcr ()
//
// Routine Description:
//
//    This function returns the effective address of the Processor
//    Control Region (KPCR *).  This address is constant, and
//    this routine merely copies that constant into GPR 3 and returns.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    Effective address of this processor's PCR.
//
//--

        LEAF_ENTRY (KiGetPcr)

        KIPCR(r.3)

        LEAF_EXIT (KiGetPcr)

//++
//
// void
// KiSetDbat
//
// Routine Description:
//
//    Writes a set of values to DBAT n
//
//    No validation of parameters is done.  Protection is set for kernel
//    mode access only.
//
// Arguments:
//
//    r.3       Number of DBAT
//    r.4       Physical address
//    r.5       Virtual Address
//    r.6       Length (in bytes)
//    r.7       Coherence Requirements (WIM)
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY (KiSetDbat)

        mfpvr   r.9                     // different format for
                                        // 601 vs other 6xx processors
        cmpwi   cr.5, r.3, 1
        cmpwi   cr.6, r.3, 2
        cmpwi   cr.7, r.3, 3

        rlwinm. r.10, r.9, 0, 0xfffe0000// Check for 601

        // calculate mask (ie BSM)  If we knew the number passed in was
        // always a power of two we could just subtract 1 and shift right
        // 17 bits.  But to be sure we will use a slightly more complex
        // algorithm than will always generate a correct mask.
        //
        // the mask is given by
        //
        //    ( 1 << ( 32 - 17 - cntlzw(Length - 1) ) ) - 1
        // == ( 1 << ( 15 - cntlzw(Length - 1) ) ) - 1

        addi    r.6, r.6, -1
        oris    r.6, r.6, 1             // ensure min length 128KB
        ori     r.6, r.6, 0xffff
        cntlzw  r.6, r.6
        subfic  r.6, r.6, 15
        li      r.10, 1
        slw     r.6, r.10, r.6
        addi    r.6, r.6, -1

        beq     cr.0, KiSetDbat601

        // processor is not a 601.

        rlwinm  r.7, r.7, 3, 0x38       // position WIM  (G = 0)
        rlwinm  r.6, r.6, 2, 0x1ffc     // restrict BAT maximum (non 601)
                                        // after left shifting by 2.

        // if caching is Inhibited, set the Guard bit as well.

        rlwimi  r.7, r.7, 30, 0x8       // copy G bit from I bit.
        ori     r.6, r.6, 0x2           // Valid (bit) in supervisor state only
        ori     r.7, r.7, 2             // PP = 0x2
        or      r.5, r.5, r.6           // = Virt addr | BL | Vs | Vp
        or      r.4, r.4, r.7           // = Phys addr | WIMG | 0 | PP

        beq     cr.5, KiSetDbat1
        beq     cr.6, KiSetDbat2
        beq     cr.7, KiSetDbat3

KiSetDbat0:
        mtdbatl 0, r.4
        mtdbatu 0, r.5
        b       KiSetDbatExit

KiSetDbat1:
        mtdbatl 1, r.4
        mtdbatu 1, r.5
        b       KiSetDbatExit

KiSetDbat2:
        mtdbatl 2, r.4
        mtdbatu 2, r.5
        b       KiSetDbatExit

KiSetDbat3:
        mtdbatl 3, r.4
        mtdbatu 3, r.5
        b       KiSetDbatExit

        // 601 has different format BAT registers and actually only has
        // one set unlike other PowerPC processors which have seperate
        // Instruction and Data BATs.   The 601 BAT registers are set
        // with the mtibat[u|l] instructions.

KiSetDbat601:

        rlwinm  r.7, r.7, 3, 0x70       // position WIMG (601 has no G bit)
        rlwinm  r.6, r.6, 0, 0x3f       // restrict BAT maximum (601 = 8MB)
        ori     r.6, r.6, 0x40          // Valid bit
        ori     r.7, r.7, 4             // Ks = 0 | Ku = 1 | PP = 0b00
        or      r.4, r.4, r.6           // = Phys addr | Valid | BL
        or      r.5, r.5, r.7           // = Virt addr | WIM | Ks | Ku | PP

        beq     cr.5, KiSet601Bat1
        beq     cr.6, KiSet601Bat2
        beq     cr.7, KiSet601Bat3

KiSet601Bat0:
        mtibatl 0, r.4
        mtibatu 0, r.5
        b       KiSetDbatExit

KiSet601Bat1:
        mtibatl 1, r.4
        mtibatu 1, r.5
        b       KiSetDbatExit

KiSet601Bat2:
        mtibatl 2, r.4
        mtibatu 2, r.5
        b       KiSetDbatExit

KiSet601Bat3:
        mtibatl 3, r.4
        mtibatu 3, r.5

KiSetDbatExit:
        isync
        LEAF_EXIT(KiSetDbat)

//++
//
// void
// KiSetDbatInvalid(BAT)
//
// Routine Description:
//
//    Clears the valid bit(s) in DBAT n
//
// Arguments:
//
//    r.3       Number of DBAT
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiSetDbatInvalid)

        mfpvr   r.9                     // different format for
                                        // 601 vs other 6xx processors
        cmpwi   cr.5, r.3, 1
        cmpwi   cr.6, r.3, 2
        cmpwi   cr.7, r.3, 3

        rlwinm. r.10, r.9, 0, 0xfffe0000// Check for 601

        li      r.0, 0                  // no valid bit

        beq     cr.0, KiInvalidateBat601

        // processor is not a 601.

        beq     cr.5, KiInvalidateDbat1
        beq     cr.6, KiInvalidateDbat2
        beq     cr.7, KiInvalidateDbat3

KiInvalidateDbat0:
        mtdbatu 0, r.0
        b       KiSetDbatInvalidExit

KiInvalidateDbat1:
        mtdbatu 1, r.0
        b       KiSetDbatInvalidExit

KiInvalidateDbat2:
        mtdbatu 2, r.0
        b       KiSetDbatInvalidExit

KiInvalidateDbat3:
        mtdbatu 3, r.0
        b       KiSetDbatInvalidExit

        // 601 has different format BAT registers and actually only has
        // one set unlike other PowerPC processors which have seperate
        // Instruction and Data BATs.   The 601 BAT registers are set
        // with the mtibat[u|l] instructions.

KiInvalidateBat601:

        beq     cr.5, KiInvalidate601Bat1
        beq     cr.6, KiInvalidate601Bat2
        beq     cr.7, KiInvalidate601Bat3

KiInvalidate601Bat0:
        mtibatl 0, r.0
        b       KiSetDbatInvalidExit

KiInvalidate601Bat1:
        mtibatl 1, r.0
        b       KiSetDbatInvalidExit

KiInvalidate601Bat2:
        mtibatl 2, r.0
        b       KiSetDbatInvalidExit

KiInvalidate601Bat3:
        mtibatl 3, r.0

KiSetDbatInvalidExit:
        isync
        LEAF_EXIT(KiSetDbatInvalid)

//++
//
// ULONG
// KiGetPvr ()
//
// Routine Description:
//
//    Returns the contents of PVR, the Processor Version Register.
//    This is a read-only register, so there is no corresponding
//    function to write the PVR.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    Contents of PVR
//
//--

        LEAF_ENTRY (KiGetPvr)

        mfpvr   r.3                               // read PVR

        LEAF_EXIT (KiGetPvr)

//++
//
// BOOLEAN
// KiDisableInterrupts (VOID)
//
// Routine Description:
//
//    This function disables interrupts and returns whether interrupts
//    were previously enabled.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    A boolean value that determines whether interrupts were previously
//    enabled (TRUE) or disabled (FALSE).
//
//--

        LEAF_ENTRY(KiDisableInterrupts)

        DISABLE_INTERRUPTS(r.3, r.4)            // turn off interrupts, old MSR
                                                // in r.3
        extrwi  r.3, r.3, 1, MSR_EE             // isolate enable bit
        LEAF_EXIT(KiDisableInterrupts)


//++
//
// VOID
// KiRestoreInterrupts (IN BOOLEAN Enable)
//
// Routine Description:
//
//    This function restores the interrupt enable that was returned by
//    the disable interrupts function.
//
// Arguments:
//
//    Enable (r.3) - Supplies the interrupt enable value.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiRestoreInterrupts)

        mfmsr   r.4                             // get current processor state
        insrwi  r.4, r.3, 1, MSR_EE             // insert external interrupt
                                                // enable/disable
        mtmsr   r.4                             // set new state
	cror	0,0,0				// N.B. 603e/ev Errata 15

        LEAF_EXIT(KiRestoreInterrupts)

//++
//
// NTSTATUS
// NtContinue (
//    IN PCONTEXT ContextRecord,
//    IN BOOLEAN TestAlert
//    )
//
// Routine Description:
//
//    This routine is called as a system service to continue execution after
//    an exception has occurred. Its functions is to transfer information from
//    the specified context record into the trap frame that was built when the
//    system service was executed, and then exit the system as if an exception
//    had occurred.
//
// Arguments:
//
//    ContextRecord (r.3) - Supplies a pointer to a context record.
//
//    TestAlert (r.4) - Supplies a boolean value that specifies whether alert
//       should be tested for the previous processor mode.
//
// Return Value:
//
//    Normally there is no return from this routine. However, if the specified
//    context record is misaligned or is not accessible, then the appropriate
//    status code is returned.
//
//--

                .struct 0
con_cr_hdr:     .space  StackFrameHeaderLength
con_ex_frame:   .space  ExceptionFrameLength
con_tr_frame:   .space  4
con_test_alert: .space  4
con_saved_lr:   .space  4
                .align  3
con_cr_length:

        NESTED_ENTRY (NtContinue,con_cr_length,0,0)
        PROLOGUE_END (NtContinue)

        stw     r.4, con_test_alert (r.sp)      // save test alert argument
        stw     r.12, con_tr_frame (r.sp)       // save the trap frame address

//
// Transfer information from the context frame to the exception and trap
// frames.
//
                                                // r.3 points to Context (1st parm)
        la      r.4, con_ex_frame (r.sp)        // r.4 = addr of Exception Frame (2nd parm)
        ori     r.5, r.12, 0                    // Pass the real trap frame
        bl      ..KiContinue                   // transfer context to kernel frames

//
// If the kernel continuation routine returns success, then exit via the
// exception exit code. Otherwise return to the system service dispatcher.
//

        cmpwi   r.3, 0                          // test return value
        bne     con_20                          // branch if non-zero (failed)

//
// Check to determine if alert should be tested for the previous processor
// mode and restore the previous mode in the thread object.
//

        lwz     r.4, KiPcr+PcCurrentThread(r.0) // get current thread address
        lwz     r.5, con_test_alert (r.sp)      // get test alert argument
        lwz     r.12, con_tr_frame (r.sp)       // get trap frame address
        cmpwi   r.5, 0                          // test test alert flag
        lwz     r.6, TrTrapFrame (r.12)         // get old trap frame address
        lbz     r.7, TrPreviousMode (r.12)      // get old previous mode
        lbz     r.3, ThPreviousMode (r.4)       // get current previous mode
        stw     r.6, ThTrapFrame (r.4)          // restore old trap frame address
        stb     r.7, ThPreviousMode (r.4)       // restore old previous mode
        beq     con_10                          // if flag zero, don't test for alert
        bl      ..KeTestAlertThread             // test alert for current thread

//
// Exit the system via exception exit which will restore the nonvolatile
// machine state.
//

con_10:
        la      r.3, con_ex_frame (r.sp)        // parm 1 = Exception Frame addr
        lwz     r.4, con_tr_frame (r.sp)        // set the original trap frame addr
        b       ..KiExceptionExit              // finish in exception exit

//
// Context record is misaligned or not accessible.
//

con_20:
        NESTED_EXIT (NtContinue,con_cr_length,0,0)

//++
//
// NTSTATUS
// NtRaiseException (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN PCONTEXT ContextRecord,
//    IN BOOLEAN FirstChance
//    )
//
// Routine Description:
//
//    This routine is called as a system service to raise an exception.
//    The exception can be raised as a first or second chance exception.
//
// Arguments:
//
//    ExceptionRecord (r.3) - Supplies a pointer to an exception record.
//
//    ContextRecord (r.4) - Supplies a pointer to a context record.
//
//    FirstChance (r.5) - Supplies a boolean value that determines whether
//       this is the first (TRUE) or second (FALSE) chance for dispatching
//       the exception.
//
//    N.B. Register r.12 is assumed to contain the address of a trap frame.
//         (HACK!) See above description of NtContinue().
//
// Return Value:
//
//    Normally there is no return from this routine. However, if the specified
//    context record or exception record is misaligned or is not accessible,
//    then the appropriate status code is returned.
//
//--

                .struct 0
rai_cr_hdr:     .space  StackFrameHeaderLength
rai_ex_frame:   .space  ExceptionFrameLength
rai_tr_frame:   .space  4
rai_saved_lr:   .space  4
                .align  3
rai_cr_length:

        NESTED_ENTRY (NtRaiseException,rai_cr_length,0,0)
        PROLOGUE_END (NtRaiseException)

        stw     r.12, rai_tr_frame (r.sp)       // save incoming Trap Frame pointer

        ori     r.7, r.5, 0                     // move "first chance" arg to 5th position

//
// Save the nonvolatile machine state so that it can be restored by exception
// exit if it is not overwritten by the specified context record.
//

        la      r.5, rai_ex_frame (r.sp)        // point r.5 to the Exception Frame

        stw     r.13, ExGpr13 (r.5)             // save non-volatile GPRs
        stw     r.14, ExGpr14 (r.5)
        stw     r.15, ExGpr15 (r.5)
        stw     r.16, ExGpr16 (r.5)
        stw     r.17, ExGpr17 (r.5)
        stw     r.18, ExGpr18 (r.5)
        stw     r.19, ExGpr19 (r.5)
        stw     r.20, ExGpr20 (r.5)
        stw     r.21, ExGpr21 (r.5)
        stw     r.22, ExGpr22 (r.5)
        stw     r.23, ExGpr23 (r.5)
        stw     r.24, ExGpr24 (r.5)
        stw     r.25, ExGpr25 (r.5)
        stw     r.26, ExGpr26 (r.5)
        stw     r.27, ExGpr27 (r.5)
        stw     r.28, ExGpr28 (r.5)
        stw     r.29, ExGpr29 (r.5)
        stw     r.30, ExGpr30 (r.5)
        stw     r.31, ExGpr31 (r.5)

        stfd    f.14, ExFpr14 (r.5)             // save non-volatile FPRs
        stfd    f.15, ExFpr15 (r.5)
        stfd    f.16, ExFpr16 (r.5)
        stfd    f.17, ExFpr17 (r.5)
        stfd    f.18, ExFpr18 (r.5)
        stfd    f.19, ExFpr19 (r.5)
        stfd    f.20, ExFpr20 (r.5)
        stfd    f.21, ExFpr21 (r.5)
        stfd    f.22, ExFpr22 (r.5)
        stfd    f.23, ExFpr23 (r.5)
        stfd    f.24, ExFpr24 (r.5)
        stfd    f.25, ExFpr25 (r.5)
        stfd    f.26, ExFpr26 (r.5)
        stfd    f.27, ExFpr27 (r.5)
        stfd    f.28, ExFpr28 (r.5)
        stfd    f.29, ExFpr29 (r.5)
        stfd    f.30, ExFpr30 (r.5)
        stfd    f.31, ExFpr31 (r.5)

//
// Call the raise exception kernel routine which will marshall the arguments
// and then call the exception dispatcher.
//
//      r.3     addr of Exception Record
//      r.4     addr of Context Record
//      r.5     addr of Exception Frame
//      r.6     addr of Trap Frame
//      r.7     "first chance" boolean
//

        ori     r.6, r.12, 0                    // move Trap Frame pointer into call arg
        bl      ..KiRaiseException              // call Raise Exception routine

//
// If the raise exception routine returns success, then exit via the exception
// exit code. Otherwise return to the system service dispatcher.
//

        lwz     r.5, KiPcr+PcCurrentThread(r.0) // get current thread address
        lwz     r.4, rai_tr_frame (r.sp)        // parm 2 = Trap Frame addr
        lwz     r.6, TrTrapFrame (r.4)          // get old trap frame address
        cmpwi   r.3, 0                          // test return value
        stw     r.6, ThTrapFrame (r.5)          // restore old trap frame address
        bne     rai10                           // branch if dispatch not successful

//
// Exit the system via exception exit which will restore the nonvolatile
// machine state.
//

        la      r.3, rai_ex_frame (r.sp)        // parm 1 = Exception Frame addr
        b       ..KiExceptionExit               // finish in Exception Exit

//
// The context or exception record is misaligned or not accessible, or the
// exception was not handled.
//

rai10:
        NESTED_EXIT (NtRaiseException,rai_cr_length,0,0)

//++
//
// PKTHREAD
// KeGetCurrentThread (VOID)
//
// Routine Description:
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    Returns a pointer to the executing thread object.
//
//--

        LEAF_ENTRY(KeGetCurrentThread)
        lwz     r.3, KiPcr+PcCurrentThread(r.0)
        LEAF_EXIT(KeGetCurrentThread)        // return

//++
//
// KIRQL
// KeGetCurrentIrql (VOID)
//
// Routine Description:
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    Returns a pointer to the executing thread object.
//
//--

        LEAF_ENTRY(KeGetCurrentIrql)
        lbz     r.3, KiPcr+PcCurrentIrql(r.0)
        LEAF_EXIT(KeGetCurrentIrql)        // return

