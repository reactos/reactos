//      TITLE("Miscellaneous Exception Handling")
//++
//
// Copyright (c) 1990  Microsoft Corporation
//
// Module Name:
//
//    xcptmisc.s
//
// Abstract:
//
//    This module implements miscellaneous routines that are required
//    to support exception handling. Functions are provided to capture
//    and restore the caller's context, call an exception handler for
//    an exception, call an exception handler for unwinding, call an
//    exception filter, call a termination handler, and get the
//    caller's stack limits.
//
// Author:
//
//    Rick Simpson  10-Sep-1993
//
//    Based on the MIPS routines xxcaptur.s and xcptmisc.s, by
//        David N. Cutler (davec) 12-Sep-1990
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//    Tom Wood (twood) 1-Nov-1993
//      Rewrite RtlpExecuteHandlerForException, RtlpExceptionHandler,
//      RtlpExecuteHandlerForUnwind, and RtlpUnwindHandler to be more
//      like the MIPS versions.
//--
//list(off)
#include "ksppc.h"
//list(on)
                .extern NtContinue

        .set    PCR_SAVE4, PcGprSave + 8
        .set    PCR_SAVE5, PcGprSave + 12
        .set    PCR_SAVE6, PcGprSave + 16
        .set    sprg.1, 1

//++
//
// VOID
// RtlCaptureContext (
//    OUT PCONTEXT ContextRecord
//    )
//
// Routine Description:
//
//    This function captures the context of the caller in the specified
//    context record.
//
//    N.B.  The context IS guaranteed to be doubleword (8-byte) aligned,
//          as are all structs in PowerPC containing "double"s.
//
// Arguments:
//
//    ContextRecord (r.3) - Supplies the address of a context record.
//
// Return Value:
//
//    None.
//
//--

// Presumed user-mode MSR value -- ILE | EE | PR | FP | ME | FE0 | IR |
//                                 FE1 | DR | LE

        .set    UserMSR, 0x0001F931

        LEAF_ENTRY(RtlCaptureContext)

//
// Save the floating-point context
//

        stfd    f.0, CxFpr0 (r.3)
        mffs    f.0                             // fetch FPSCR into f.0
        stfd    f.1, CxFpr1 (r.3)
        stfd    f.2, CxFpr2 (r.3)
        stfd    f.3, CxFpr3 (r.3)
        stfd    f.4, CxFpr4 (r.3)
        stfd    f.5, CxFpr5 (r.3)
        stfd    f.6, CxFpr6 (r.3)
        stfd    f.7, CxFpr7 (r.3)
        stfd    f.8, CxFpr8 (r.3)
        stfd    f.9, CxFpr9 (r.3)
        stfd    f.10, CxFpr10 (r.3)
        stfd    f.11, CxFpr11 (r.3)
        stfd    f.12, CxFpr12 (r.3)
        stfd    f.13, CxFpr13 (r.3)
        stfd    f.14, CxFpr14 (r.3)
        stfd    f.15, CxFpr15 (r.3)
        stfd    f.16, CxFpr16 (r.3)
        stfd    f.17, CxFpr17 (r.3)
        stfd    f.18, CxFpr18 (r.3)
        stfd    f.19, CxFpr19 (r.3)
        stfd    f.20, CxFpr20 (r.3)
        stfd    f.21, CxFpr21 (r.3)
        stfd    f.22, CxFpr22 (r.3)
        stfd    f.23, CxFpr23 (r.3)
        stfd    f.24, CxFpr24 (r.3)
        stfd    f.25, CxFpr25 (r.3)
        stfd    f.26, CxFpr26 (r.3)
        stfd    f.27, CxFpr27 (r.3)
        stfd    f.28, CxFpr28 (r.3)
        stfd    f.29, CxFpr29 (r.3)
        stfd    f.30, CxFpr30 (r.3)
        stfd    f.31, CxFpr31 (r.3)
        stfd    f.0, CxFpscr (r.3)              // store FPSCR in context record

//
// Save the integer context
//

        stw     r.0, CxGpr0 (r.3)
        mfcr    r.0
        stw     r.1, CxGpr1 (r.3)
        stw     r.2, CxGpr2 (r.3)
        stw     r.3, CxGpr3 (r.3)
        stw     r.4, CxGpr4 (r.3)
        mfxer   r.4
        stw     r.5, CxGpr5 (r.3)
        mflr    r.5
        stw     r.6, CxGpr6 (r.3)
        mfctr   r.6
        stw     r.7, CxGpr7 (r.3)
        stw     r.8, CxGpr8 (r.3)
        stw     r.9, CxGpr9 (r.3)
        stw     r.10, CxGpr10 (r.3)
        stw     r.11, CxGpr11 (r.3)
        stw     r.12, CxGpr12 (r.3)
        stw     r.13, CxGpr13 (r.3)
        stw     r.14, CxGpr14 (r.3)
        stw     r.15, CxGpr15 (r.3)
        stw     r.16, CxGpr16 (r.3)
        stw     r.17, CxGpr17 (r.3)
        stw     r.18, CxGpr18 (r.3)
        stw     r.19, CxGpr19 (r.3)
        stw     r.20, CxGpr20 (r.3)
        stw     r.21, CxGpr21 (r.3)
        stw     r.22, CxGpr22 (r.3)
        stw     r.23, CxGpr23 (r.3)
        stw     r.24, CxGpr24 (r.3)
        stw     r.25, CxGpr25 (r.3)
        stw     r.26, CxGpr26 (r.3)
        stw     r.27, CxGpr27 (r.3)

//
// Test high-order bit of stack pointer value
//      0 => caller in user mode   (address <= 0x7FFFFFFF)
//      1 => caller in kernel mode (address >= 0x80000000)

        cmpwi   r.1, 0

//
// Save special registers (some integer, some control)
//

        stw     r.0, CxCr (r.3)                 // condition register
        stw     r.4, CxXer (r.3)                // fixed point exception register
        stw     r.5, CxIar (r.3)                // instruction address (current PC)
        stw     r.5, CxLr (r.3)                 // link register (return address)
        stw     r.6, CxCtr (r.3)                // count register

//
// Save actual MSR value if caller is kernel, otherwise save
// canonical user-mode MSR value
//

        lis     r.0, UserMSR > 16
        ori     r.0, r.0, UserMSR & 0xFFFF
        bnl     Cap1                            // branch around if user mode
        mfmsr   r.0
Cap1:
        stw     r.28, CxGpr28 (r.3)
        stw     r.29, CxGpr29 (r.3)
        stw     r.30, CxGpr30 (r.3)
        stw     r.31, CxGpr31 (r.3)

        stw     r.0, CxMsr (r.3)                // save MSR value

//
// Set context record flags, and exit
//

        lis     r.0, CONTEXT_FULL > 16
        ori     r.0, r.0, CONTEXT_FULL & 0xFFFF
        stw     r.0, CxContextFlags (r.3)

        LEAF_EXIT (RtlCaptureContext)

//++
//
// VOID
// RtlpRestoreContext (
//    IN PCONTEXT ContextRecord,
//    IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL
//    )
//
// Routine Description:
//
//    This function restores the context of the caller from the specified
//    context record.
//
//    N.B.  The context IS guaranteed to be doubleword (8-byte) aligned,
//          as are all structs in PowerPC containing "double"s.
//
//    N.B.  This is a special routine that is used by RtlUnwind to restore
//          context in the current mode.
//
// Arguments:
//
//    ContextRecord (r.3) - Supplies the address of a context record.
//
//    ExceptionRecord (r.4) - Supplies an optional pointer to an exception
//       record.
//
// Return Value:
//
//    None.
//
//    N.B.  There is no return from this routine.
//
//--

        LEAF_ENTRY(RtlpRestoreContext)

//
// If an exception record is specified and the exception status is
// STATUS_LONGJUMP, then restore the nonvolatile registers to their
// state at the call to setjmp before restoring the context record.
//

        cmplwi  cr.1, r.4, 0                    // ExceptionRecord supplied?
        cmpwi   cr.0, r.1, 0                    // test kernel/user mode
        beq     cr.1, NoExcpRec                 // if eq, no exception record
        lwz     r.6, ErExceptionCode(r.4)       // get exception code
        LWI(r.8,STATUS_LONGJUMP)                // get long jump status code
        cmplw   cr.1, r.6, r.8                  // is long jump status?
        lwz     r.5, ErExceptionInformation(r.4)// get address of jump buffer
        bne     cr.1, NoExcpRec                 // if ne, not a long jump

// Get non-volatile control context

        lwz     r.7, JbIar(r.5)
        lwz     r.8, JbCr(r.5)
        lwz     r.9, JbGpr1(r.5)
        lwz     r.10,JbGpr2(r.5)

// Get non-volatile Floating Point context from jump buffer

        lfd     f.14, JbFpr14(r.5)
        lfd     f.15, JbFpr15(r.5)
        lfd     f.16, JbFpr16(r.5)
        lfd     f.17, JbFpr17(r.5)
        lfd     f.18, JbFpr18(r.5)
        lfd     f.19, JbFpr19(r.5)
        lfd     f.20, JbFpr20(r.5)
        lfd     f.21, JbFpr21(r.5)
        lfd     f.22, JbFpr22(r.5)
        lfd     f.23, JbFpr23(r.5)
        lfd     f.24, JbFpr24(r.5)
        lfd     f.25, JbFpr25(r.5)
        lfd     f.26, JbFpr26(r.5)
        lfd     f.27, JbFpr27(r.5)
        lfd     f.28, JbFpr28(r.5)
        lfd     f.29, JbFpr29(r.5)
        lfd     f.30, JbFpr30(r.5)
        lfd     f.31, JbFpr31(r.5)

// Get non-volatile Integer context from jump buffer

        lwz     r.14, JbGpr14(r.5)
        lwz     r.15, JbGpr15(r.5)
        lwz     r.16, JbGpr16(r.5)
        lwz     r.17, JbGpr17(r.5)
        lwz     r.18, JbGpr18(r.5)
        lwz     r.19, JbGpr19(r.5)
        lwz     r.20, JbGpr20(r.5)
        lwz     r.21, JbGpr21(r.5)
        lwz     r.22, JbGpr22(r.5)
        lwz     r.23, JbGpr23(r.5)
        lwz     r.24, JbGpr24(r.5)
        lwz     r.25, JbGpr25(r.5)
        lwz     r.26, JbGpr26(r.5)
        lwz     r.27, JbGpr27(r.5)
        lwz     r.28, JbGpr28(r.5)
        lwz     r.29, JbGpr29(r.5)
        lwz     r.30, JbGpr30(r.3)
        lwz     r.31, JbGpr31(r.3)

//
// Save non-volatile control context in context record
//

        stw     r.7, CxIar(r.3)
        stw     r.8, CxCr(r.3)
        stw     r.9, CxGpr1(r.3)
        stw     r.10,CxGpr2(r.3)

//
// Save non-volatile Floating Point and Integer registers in context record
// plj note: do we really need to do this if we are in kernel mode?
//

        stfd    f.14, CxFpr14(r.3)
        stfd    f.15, CxFpr15(r.3)
        stfd    f.16, CxFpr16(r.3)
        stfd    f.17, CxFpr17(r.3)
        stfd    f.18, CxFpr18(r.3)
        stfd    f.19, CxFpr19(r.3)
        stfd    f.20, CxFpr20(r.3)
        stfd    f.21, CxFpr21(r.3)
        stfd    f.22, CxFpr22(r.3)
        stfd    f.23, CxFpr23(r.3)
        stfd    f.24, CxFpr24(r.3)
        stfd    f.25, CxFpr25(r.3)
        stfd    f.26, CxFpr26(r.3)
        stfd    f.27, CxFpr27(r.3)
        stfd    f.28, CxFpr28(r.3)
        stfd    f.29, CxFpr29(r.3)
        stfd    f.30, CxFpr30(r.3)
        stfd    f.31, CxFpr31(r.3)
        stw     r.14, CxGpr14(r.3)
        stw     r.15, CxGpr15(r.3)
        stw     r.16, CxGpr16(r.3)
        stw     r.17, CxGpr17(r.3)
        stw     r.18, CxGpr18(r.3)
        stw     r.19, CxGpr19(r.3)
        stw     r.20, CxGpr20(r.3)
        stw     r.21, CxGpr21(r.3)
        stw     r.22, CxGpr22(r.3)
        stw     r.23, CxGpr23(r.3)
        stw     r.24, CxGpr24(r.3)
        stw     r.25, CxGpr25(r.3)
        stw     r.26, CxGpr26(r.3)
        stw     r.27, CxGpr27(r.3)
        stw     r.28, CxGpr28(r.3)
        stw     r.29, CxGpr29(r.3)
        stw     r.30, CxGpr30(r.3)
        stw     r.31, CxGpr31(r.3)

//
// If from kernel mode, continue restoration with volatile registers.
//

        blt     cr.0, ResKrnlVol

//
// If called in user mode (stack pointer in r.1 is < 0x80000000), branch
// directly to the "continue" system service to continue execution.
//
// If we fell thru from above, the blt below will fall thru also, and it's
// free on PowerPC.
//

NoExcpRec:

        blt     cr.0, ResKrnlAll                // branch if kernel mode

        lwz     r.5, [toc] NtContinue (r.toc)   // get function desc. addr
        lwz     r.0, 0(r.5)                     // get entry point address
        li      r.4, 0                          // set "test alert" arg FALSE
        mtctr   r.0                             //   into Ctr
        lwz     r.2, 4(r.5)                     // get TOC address
        bctr                                    // branch to system service

ResKrnlAll:

//
// Restore the non-volatile Floating Point context
//

        lfd     f.14, CxFpr14 (r.3)
        lfd     f.15, CxFpr15 (r.3)
        lfd     f.16, CxFpr16 (r.3)
        lfd     f.17, CxFpr17 (r.3)
        lfd     f.18, CxFpr18 (r.3)
        lfd     f.19, CxFpr19 (r.3)
        lfd     f.20, CxFpr20 (r.3)
        lfd     f.21, CxFpr21 (r.3)
        lfd     f.22, CxFpr22 (r.3)
        lfd     f.23, CxFpr23 (r.3)
        lfd     f.24, CxFpr24 (r.3)
        lfd     f.25, CxFpr25 (r.3)
        lfd     f.26, CxFpr26 (r.3)
        lfd     f.27, CxFpr27 (r.3)
        lfd     f.28, CxFpr28 (r.3)
        lfd     f.29, CxFpr29 (r.3)
        lfd     f.30, CxFpr30 (r.3)
        lfd     f.31, CxFpr31 (r.3)

//
// Restore the non-volatile Integer context
//

        lwz     r.14, CxGpr14 (r.3)
        lwz     r.15, CxGpr15 (r.3)
        lwz     r.16, CxGpr16 (r.3)
        lwz     r.17, CxGpr17 (r.3)
        lwz     r.18, CxGpr18 (r.3)
        lwz     r.19, CxGpr19 (r.3)
        lwz     r.20, CxGpr20 (r.3)
        lwz     r.21, CxGpr21 (r.3)
        lwz     r.22, CxGpr22 (r.3)
        lwz     r.23, CxGpr23 (r.3)
        lwz     r.24, CxGpr24 (r.3)
        lwz     r.25, CxGpr25 (r.3)
        lwz     r.26, CxGpr26 (r.3)
        lwz     r.27, CxGpr27 (r.3)
        lwz     r.28, CxGpr28 (r.3)
        lwz     r.29, CxGpr29 (r.3)
        lwz     r.30, CxGpr30 (r.3)
        lwz     r.31, CxGpr31 (r.3)

//
// Restore the volatile floating-point context
//

ResKrnlVol:

        lfd     f.13, CxFpscr (r.3)             // fetch FPSCR into f.13
        lfd     f.0, CxFpr0 (r.3)
        lfd     f.1, CxFpr1 (r.3)
        lfd     f.2, CxFpr2 (r.3)
        lfd     f.3, CxFpr3 (r.3)
        lfd     f.4, CxFpr4 (r.3)
        lfd     f.5, CxFpr5 (r.3)
        lfd     f.6, CxFpr6 (r.3)
        lfd     f.7, CxFpr7 (r.3)
        lfd     f.8, CxFpr8 (r.3)
        lfd     f.9, CxFpr9 (r.3)
        lfd     f.10, CxFpr10 (r.3)
        lfd     f.11, CxFpr11 (r.3)
        lfd     f.12, CxFpr12 (r.3)
        mtfsf   0xff, f.13                      // set the FPSCR value
        lfd     f.13, CxFpr13 (r.3)

//
// Restore the volatile integer and control context
//

        lwz     r.0, CxCr (r.3)                 // condition register
        lwz     r.4, CxXer (r.3)                // fixed point exception register
        lwz     r.5, CxLr (r.3)                 // link register (return address)
        lwz     r.6, CxCtr (r.3)                // count register

        mtcrf   0xff, r.0                       // restore CR
        mtxer   r.4                             // restore XER
        mtlr    r.5                             // restore LR
        mtctr   r.6                             // restore CTR

        lwz     r.0, CxGpr0 (r.3)               // restore GPRs
        lwz     r.2, CxGpr2 (r.3)
        lwz     r.4, CxGpr4 (r.3)
        lwz     r.5, CxGpr5 (r.3)
        lwz     r.6, CxGpr6 (r.3)
        lwz     r.9, CxGpr9 (r.3)
        lwz     r.10, CxGpr10 (r.3)
        lwz     r.11, CxGpr11 (r.3)
        lwz     r.12, CxGpr12 (r.3)

        mfmsr   r.8                             // fetch current MSR value
        rlwinm  r.8, r.8, 0, ~MASK_SPR(MSR_EE,1) // turn off EE bit
        mtmsr   r.8                             // disable interrupts
	cror	0,0,0				// N.B. 603e/ev Errata 15

        lwz     r.1, CxGpr3 (r.3)
        lwz     r.7, CxGpr7 (r.3)
        stw     r.1, KiPcr+PCR_SAVE4 (r.0)
        lwz     r.1, CxGpr8 (r.3)
        stw     r.7, KiPcr+PCR_SAVE5 (r.0)
        stw     r.1, KiPcr+PCR_SAVE6 (r.0)

        lwz     r.1, CxGpr1 (r.3)               // This MUST BE AFTER
                                                //         interrupts disabled
        lwz     r.7, CxIar (r.3)                // instruction address (current PC)
        lwz     r.3, CxMsr (r.3)                // machine state register

        mfsprg  r.8, sprg.1

        DUMMY_ENTRY(RtlpRestoreContextRfiJump)
        b       $                               // This is changed to be a branch
                                                //  to KSEG0 code at init time
RtlpRestoreContext.end:

//
// Define call frame for calling exception handlers.
//

                .struct 0
CfBackChain:    .space  4                       // chain to previous call frame
                .space  5*4                     // remaining part of frame header
                .space  8*4                     // 8 words space for call args
CfDispContext:  .space  4               // space to save the incoming
                                        // Dispatcher Context ptr
CfSavedRtoc:    .space  4               // space to save rtoc
                .space  4                       //   space to save LR
                .align  3                       // force frame length to multiple of
CfEnd:                                          //   eight bytes

//++
//
// EXCEPTION_DISPOSITION
// RtlpExecuteHandlerForException (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN ULONG EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PDISPATCHER_CONTEXT DispatcherContext,
//    IN PEXCEPTION_ROUTINE ExceptionRoutine
//    )
//
// Routine Description:
//
//    This function allocates a call frame, stores the establisher frame
//    pointer in the frame, establishes an exception handler, and then calls
//    the specified exception handler as an exception handler. If a nested
//    exception occurs, then the exception handler of this function is called
//    and the establisher frame pointer is returned to the exception dispatcher
//    via the dispatcher context parameter. If control is returned to this
//    routine, then the frame is deallocated and the disposition status is
//    returned to the exception dispatcher.
//
// Arguments:
//
//    ExceptionRecord (r.3) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (r.4) - Supplies the frame pointer of the establisher
//       of the exception handler that is to be called.
//
//    ContextRecord (r.5) - Supplies a pointer to a context record.
//
//    DispatcherContext (r.6) - Supplies a pointer to the dispatcher context
//       record.
//
//    ExceptionRoutine (r.7) - supplies a pointer to the function descriptor
//       for the exception handler that is to be called.
//
// Return Value:
//
//    The disposition value returned by the specified exception handler is
//    returned as the function value.
//
//--

        NESTED_ENTRY_EX (RtlpExecuteHandlerForException,CfEnd,0,0,RtlpExceptionHandler,0)

        stw     r.toc,CfSavedRtoc(r.sp) // save rtoc

        PROLOGUE_END (RtlpExecuteHandlerForException)

        lwz     r.0, 0 (r.7)                    // get entry point addr from descriptor
        mtlr    r.0                             // LR <- entry point addr
        lwz     r.toc,4(r.7)            // rtoc <- TOC addr from descriptor
        stw     r.6, CfDispContext (r.sp)       // save Dispatcher Context where
                                                //   RtlpExectionHandler can find it

        blrl                                    // call the exception handler

        lwz     r.toc,CfSavedRtoc(r.sp) // restore rtoc
        NESTED_EXIT (RtlpExecuteHandlerForException,CfEnd,0,0)

//++
//
// EXCEPTION_DISPOSITION
// RtlpExceptionHandler (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN ULONG EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PDISPATCHER_CONTEXT DispatcherContext
//    )
//
// Routine Description:
//
//    This function is called when a nested exception occurs. Its function
//    is to retrieve the establisher frame pointer from its establisher's
//    call frame, store this information in the dispatcher context record,
//    and return a disposition value of nested exception.
//
// Arguments:
//
//    ExceptionRecord (r.3) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (r.4) - Supplies the frame pointer of the establisher
//       of this exception handler.
//
//    ContextRecord (r.5) - Supplies a pointer to a context record.
//
//    DispatcherContext (r.6) - Supplies a pointer to the dispatcher context
//       record.
//
// Return Value:
//
//    A disposition value ExceptionNestedException is returned if an unwind
//    is not in progress. Otherwise a value of ExceptionContinueSearch is
//    returned.
//
//--

        LEAF_ENTRY (RtlpExceptionHandler)
        lwz     r.0, ErExceptionFlags (r.3)     // get exception flags
        li      r.3, ExceptionContinueSearch    // set usual disposition value
        andi.   r.0, r.0, EXCEPTION_UNWIND      // check if unwind in progress
        bnelr                           // if neq, unwind in progress
                                        // return continue search disposition

//
// Unwind is not in progress - return nested exception disposition.
//

        lwz     r.7,CfDispContext-CfEnd(r.4)    // get dispatcher context address
        li      r.3, ExceptionNestedException   // set disposition value
        lwz     r.0, DcEstablisherFrame(r.7)    // copy the establisher environment
        stw     r.0, DcEstablisherFrame (r.6)   //   to current dispatcher context
        LEAF_EXIT (RtlpExceptionHandler)

//++
//
// EXCEPTION_DISPOSITION
// RtlpExecuteHandlerForUnwind (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN PVOID EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PVOID DispatcherContext,
//    IN PEXCEPTION_ROUTINE ExceptionRoutine
//    )
//
// Routine Description:
//
//    This function allocates a call frame, stores the establisher frame
//    pointer and the context record address in the frame, establishes an
//    exception handler, and then calls the specified exception handler as
//    an unwind handler. If a collided unwind occurs, then the exception
//    handler of of this function is called and the establisher frame pointer
//    and context record address are returned to the unwind dispatcher via
//    the dispatcher context parameter. If control is returned to this routine,
//    then the frame is deallocated and the disposition status is returned to
//    the unwind dispatcher.
//
// Arguments:
//
//    ExceptionRecord (r.3) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (r.4) - Supplies the frame pointer of the establisher
//       of the exception handler that is to be called.
//
//    ContextRecord (r.5) - Supplies a pointer to a context record.
//
//    DispatcherContext (r.6) - Supplies a pointer to the dispatcher context
//       record.
//
//    ExceptionRoutine (r.7) - supplies a pointer to the exception handler
//       that is to be called.
//
// Return Value:
//
//    The disposition value returned by the specified exception handler is
//    returned as the function value.
//
//--

        NESTED_ENTRY_EX (RtlpExecuteHandlerForUnwind,CfEnd,0,0,RtlpUnwindHandler,0)

        stw     r.toc, CfSavedRtoc(r.sp)        // save rtoc

        PROLOGUE_END (RtlpExecuteHandlerForUnwind)

        lwz     r.0, 0 (r.7)                    // get entry point addr from descriptor
        mtlr    r.0                             // LR <- entry point addr
        lwz     r.toc, 4 (r.7)                  // rtoc <- TOC addr from descriptor
        stw     r.6, CfDispContext (r.sp)       // save Dispatcher Context where
                                                //   RtlpExectionHandler can find it

        blrl                                    // call the exception handler

        lwz     r.toc, CfSavedRtoc(r.sp)        // restore rtoc

        NESTED_EXIT (RtlpExecuteHandlerForUnwind,CfEnd,0,0)

//++
//
// EXCEPTION_DISPOSITION
// RtlpUnwindHandler (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN PVOID EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PVOID DispatcherContext
//    )
//
// Routine Description:
//
//    This function is called when a collided unwind occurs. Its function
//    is to retrieve the establisher dispatcher context, copy it to the
//    current dispatcher context, and return a disposition value of nested
//    unwind.
//
// Arguments:
//
//    ExceptionRecord (r.3) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (r.4) - Supplies the frame pointer of the establisher
//       of this exception handler.
//
//    ContextRecord (r.5) - Supplies a pointer to a context record.
//
//    DispatcherContext (r.6) - Supplies a pointer to the dispatcher context
//       record.
//
// Return Value:
//
//    A disposition value ExceptionCollidedUnwind is returned if an unwind is
//    in progress. Otherwise, a value of ExceptionContinueSearch is returned.
//
//--

        LEAF_ENTRY (RtlpUnwindHandler)

        lwz     r.0, ErExceptionFlags (r.3)     // get exception flags
        li      r.3, ExceptionContinueSearch    // set usual disposition value
        andi.   r.0, r.0, EXCEPTION_UNWIND      // check if unwind in progress
        beqlr                                   // if eq, unwind not in progress
                                                // return continue search disposition

//
// Unwind is in progress - return collided exception disposition.
//

        lwz     r.7, CfDispContext-CfEnd (r.4)  // get dispatcher context address
        lwz     r.0, DcControlPc (r.7)          // copy the establisher frame's
        lwz     r.3, DcFunctionEntry (r.7)      //   dispatcher context to the
        lwz     r.4, DcEstablisherFrame (r.7)   //   current dispatcher context
        lwz     r.5, DcContextRecord (r.7)
        stw     r.0, DcControlPc (r.6)
        stw     r.3, DcFunctionEntry (r.6)
        stw     r.4, DcEstablisherFrame (r.6)
        stw     r.5, DcContextRecord (r.6)
        li      r.3, ExceptionCollidedUnwind    // return collided unwind disposition
        LEAF_EXIT (RtlpUnwindHandler)

//++
//
// VOID
// RtlpGetStackLimits (
//    OUT PULONG LowLimit,
//    OUT PULONG HighLimit
//    )
//
// Routine Description:
//
//    This function returns the current stack limits based on the current
//    processor mode.
//
// Arguments:
//
//    LowLimit (r.3) - Supplies a pointer to a variable that is to receive
//       the low limit of the stack.
//
//    HighLimit (r.4) - Supplies a pointer to a variable that is to receive
//       the high limit of the stack.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY (RtlpGetStackLimits)

#ifdef ROS_DEBUG
        li      r.0, 0                          // force return low=0, high=2GB for debug
        stw     r.0, 0(r.3)
        lis     r.0, 0x7FFF
        ori     r.0, r.0, 0xFFFF
        stw     r.0, 0(r.4)
        blr
#endif

        cmpwi   r.sp, 0                         // if stack ptr < 0x80000000, user mode
        bnl     Sl10                            // branch if user mode

//
// Current mode is kernel - compute stack limits.
//

        lwz     r.6, KiPcr+PcInitialStack(r.0)  // get high limit of kernel stack
        lwz     r.7, KiPcr+PcStackLimit(r.0)    // get low limit of kernel stack
        stw     r.6, 0 (r.4)                    // store high limit
        stw     r.7, 0 (r.3)                    // store low limit
        ALTERNATE_EXIT (RtlpGetStackLimits)

//
// Current mode is user - get stack limits from the TEB.
//

Sl10:

// Fast-path system service returns TEB address in r.3, having killed only CR

        lwz     r.0, TeStackLimit (r.13)        // get low limit of user stack
        lwz     r.6, TeStackBase (r.13)         // get high limit of user stack
        stw     r.0, 0 (r.3)                    // store low stack limit
        stw     r.6, 0 (r.4)                    // store high stack limit
        LEAF_EXIT (RtlpGetStackLimits)
