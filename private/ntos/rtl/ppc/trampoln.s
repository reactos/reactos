//++
//
// Copyright (c) 1993  IBM Corporation and Microsoft Corporation
//
// Module Name:
//
//    trampoln.s
//
// Abstract:
//
//    This module implements the trampoline code necessary to dispatch user
//    mode APCs.
//
// Author:
//
//    Rick Simpson  25-Oct-1993
//
//    based on MIPS version by David N. Cutler (davec) 3-Apr-1990
//
// Environment:
//
//    User mode only.
//
// Revision History:
//
//--

//list(off)
#include "ksppc.h"
//list(on)
                .extern __C_specific_handler
                .extern ..RtlDispatchException
                .extern ..RtlRaiseException
                .extern ..RtlRaiseStatus
                .extern ZwCallbackReturn
                .extern ZwContinue
                .extern ZwRaiseException
                .extern ZwTestAlert

//
// Define layout and length of APC Dispatcher stack frame.
//    N.B.  This must exactly match the computations in KiInitializeUserApc()
//

                .struct 0
ADFrame:        .space  StackFrameHeaderLength
ADContext:      .space  ContextFrameLength
ADTrap:         .space  TrapFrameLength
ADToc:          .long   0
                .space  STK_SLACK_SPACE
                .align  3
ADFrameLength:

                .text

//++
//
// The following code is never executed. Its purpose is to support unwinding
// through the call to the APC dispatcher.
//
//--

        .ydata                                  // scope table -- exception handler
        .align  2
UserApcDispatcherScopeTable:
        .long   1                               // number of scope table entries
        .long   ..KiUserApcDispatcher          // start of scope
        .long   KiUserApcDispatcher.end         // end of scope
        .long   KiUserApcHandler                // filter to decide what to do
        .long   0                               // it always decides to "continue search"
        .text

        FN_TABLE (KiUserApcDispatch,__C_specific_handler,UserApcDispatcherScopeTable)

        DUMMY_ENTRY (KiUserApcDispatch)

        stwu    r.sp, -ADFrameLength (r.sp)
        mflr    r.0
        stw     r.0, ADContext + CxLr (r.sp)
        stw     r.0, ADContext + CxIar (r.sp)
        stw     r.toc, ADToc (r.sp)

        PROLOGUE_END (KiUserApcDispatch)

//++
//
// VOID
// KiUserApcDispatcher (
//    IN PVOID NormalContext,
//    IN PVOID SystemArgument1,
//    IN PVOID SystemArgument2,
//    IN PKNORMAL_ROUTINE NormalRoutine
//    )
//
// Routine Description:
//
//    This routine is entered on return from kernel mode to deliver an APC
//    in user mode. The stack frame for this routine was built when the
//    APC interrupt was processed and contains the entire machine state of
//    the current thread. The specified APC routine is called and then the
//    machine state is restored and execution is continued.
//
//    On entry here, a stack frame as shown above is already addressed
//    via r.1
//
// Arguments:
//
//    r.1 - Stack frame pointer, already set up
//
//    r.3 - Supplies the normal context parameter that was specified when the
//          APC was initialized.
//
//    r.4 - Supplies the first argument that was provied by the executive when
//          the APC was queued.
//
//    r.5 - Supplies the second argument that was provided by the executive
//          when the APC was queued.
//
//    r.6 - Supplies that address of the descriptor for the function that is
//          to be called.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY (KiUserApcDispatcher)

        lwz     r.0, 0 (r.6)                    // fetch address of APC entry point
        mtlr    r.0                             // move into Link Reg
        lwz     r.2, 4 (r.6)                    // fetch TOC address for APC
        blrl                                    // call specified APC routine

        lwz     r.2, ADToc (r.sp)               // reload our own TOC pointer
        la      r.3, ADContext (r.sp)           // 1st parm = addr of Context Frame
        lwz     r.5, [toc] ZwContinue (r.2)     // fetch ptr to function descriptor
        li      r.4, 1                          // 2nd parm = TRUE (test alert)
        lwz     r.0, 0 (r.5)                    // fetch addr of ZwContinue entry point
        mtlr    r.0                             // move into Link Reg
        lwz     r.2, 4 (r.5)                    // fetch TOC addr for ZwContinue

                                                // this next is done under protest --
                                                //   we are copying MIPS slavishly:
        la      r.12, ADTrap (r.sp)             // "secret" parm = addr of Trap Frame
        blrl                                    // execute system service to continue
        lwz     r.2, ADToc (r.sp)               // reload our own TOC pointer

//
// Unsuccessful completion after attempting to continue execution. Use the
// return status as the exception code, set noncontinuable exception and
// attempt to raise another exception.  Note there is no return from raise
// status.
//

        ori     r.31, r.3, 0                    // save status value
ADloop:
        ori     r.3, r.31, 0                    // set status value
        bl      ..RtlRaiseStatus               // raise exception
        b       ADloop                          // loop on return

        DUMMY_EXIT (KiUserApcDispatcher)
        DUMMY_EXIT (KiUserApcDispatch)

//      SBTTL("User APC Exception Handler")
//++
//
// EXCEPTION_DISPOSITION
// KiUserApcHandler (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN ULONG EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PDISPATCHER_CONTEXT DispatcherContext
//
// Routine Description:
//
//    This function is called when an exception occurs in an APC routine
//    or one of its dynamic descendents and when an unwind through the
//    APC dispatcher is in progress. If an unwind is in progress, then test
//    alert is called to ensure that all currently queued APCs are executed.
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
//    ExceptionContinueSearch is returned as the function value.
//--

        .struct 0
        .space  StackFrameHeaderLength          // canonical stack frame header
        .space  4                               // reserve space for return address
        .space  4                               // reserve space for r.31
        .align  3
HFrameLength:                                   // length of handler frame

        .text

        NESTED_ENTRY (KiUserApcHandler, HFrameLength, 1, 0)

        ori     r.31, r.toc, 0                  // save our TOC value in r.31

        PROLOGUE_END (KiUserApcHandler)

        lwz     r.0, ErExceptionFlags (r.3)     // get exception flags
        andi.   r.0, r.0, EXCEPTION_UNWIND      // check if unwind in progress
        beq     H10                             // if eq, no unwind in progress
        lwz     r.7, [toc] ZwTestAlert (r.toc)  // get addr of function descriptor
        lwz     r.0, 0 (r.7)                    // get entry point address
        mtlr    r.0                             //   into Link Reg
        lwz     r.toc, 4 (r.7)                  // get callee's TOC pointer
        blrl                                    // test for alert pending
        ori     r.toc, r.31, 0                  // reload our own TOC pointer

H10:    li      r.3, ExceptionContinueSearch    // set disposition value

        NESTED_EXIT (KiUserApcHandler, HFrameLength, 1, 0)

//      SBTTL("User Callback Dispatcher")
//++
//
// The following code is never executed. Its purpose is to support unwinding
// through the call to the callback dispatcher.
//
//--

#if 0
        .ydata                                  // scope table -- exception handler
        .align  2
UserCallbackDispatcherScopeTable:
        .long   1                               // number of scope table entries
        .long   ..KiUserCallbackDispatcher      // start of scope
        .long   KiUserCallbackDispatcher.end    // end of scope
        .long   KiUserCallbackHandler           // filter to decide what to do
        .long   0                               // it always decides to "continue search"
        .text

        FN_TABLE (KiUserCallbackDispatch,__C_specific_handler,UserCallbackDispatcherScopeTable)
#endif

        DUMMY_ENTRY (KiUserCallbackDispatch)

        stwu    r.sp, -CkFrameLength(r.sp)
        mflr    r.0
        stw     r.0, CkLr(r.sp)
        stw     r.toc, CkToc(r.sp)

        PROLOGUE_END (KiUserCallbackDispatch)

//++
//
// VOID
// KiUserCallbackDispatcher (
//    VOID
//    )
//
// Routine Description:
//
//    This routine is entered on a callout from kernel mode to execute a
//    user mode callback function. All arguments for this function have
//    been placed on the stack.
//
// Arguments:
//
//    (sp + ApiNumber) - Supplies the API number of the callback function that is
//        executed.
//
//    (sp + Buffer) - Supplies a pointer to the input buffer.
//
//    (sp + Length) - Supplies the input buffer length.
//
// Return Value:
//
//    This function returns to kernel mode.
//
//--

        ALTERNATE_ENTRY(KiUserCallbackDispatcher)

        lwz     r.6, TePeb(r.13)        // get address of PEB
        ori     r.31, r.toc, 0          // save our TOC value in r.31

        lwz     r.5, CkApiNumber(r.1)   // get API number
        lwz     r.6, PeKernelCallbackTable(r.6) // get address of callback table
        lwz     r.3, CkBuffer(r.1)      // get input buffer address
        slwi    r.5, r.5, 2             // compute offset to table entry
        lwz     r.4, CkLength(r.1)      // get input buffer length
        lwzx    r.5, r.5, r.6           // get descriptor for callback routine
        lwz     r.0, 0 (r.5)            // get entry point address
        mtlr    r.0                     //   into link register
        lwz     r.toc, 4 (r.5)          // get callee's TOC pointer
        blrl                            // call specified function

//
// If a return from the callback function occurs, then the output buffer
// address and length are returned as NULL.
//

        ori     r.toc, r.31, 0          // reload our own TOC pointer

        lwz     r.7, [toc] ZwCallbackReturn (r.toc)  // get addr of function descriptor
        ori     r.5, r.3, 0             // set completion status
        li      r.3, 0                  // set zero buffer address
        li      r.4, 0                  // set zero buffer lenfth
        lwz     r.0, 0 (r.7)            // get entry point address
        mtlr    r.0                     //   into Link Reg
        lwz     r.toc, 4 (r.7)          // get callee's TOC pointer
        blrl                            // return to kernel mode

//
// Unsuccessful completion after attempting to return to kernel mode. Use
// the return status as the exception code, set noncontinuable exception and
// attempt to raise another exception. Note there is no return from raise
// status.
//

        ori     r.toc, r.31, 0          // reload our own TOC pointer

        ori     r.31, r.3, 0            // save status value
UCDloop:
        bl      ..RtlRaiseStatus        // raise exception
        ori     r.3, r.31, 0            // set status value
        b       UCDloop                 // loop on return

        DUMMY_EXIT (KiUserCallbackDispatch)

//      SBTTL("User Callback Exception Handler")
//++
//
// EXCEPTION_DISPOSITION
// KiUserCallbackHandler (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN ULONG EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PDISPATCHER_CONTEXT DispatcherContext
//
// Routine Description:
//
//    This function is called when an exception occurs in a user callback
//    routine or one of its dynamic descendents.
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
//    ExceptionContinueSearch is returned as the function value.
//--

        NESTED_ENTRY (KiUserCallbackHandler, HFrameLength, 1, 0)

        ori     r.31, r.toc, 0                  // save our TOC value in r.31

        PROLOGUE_END (KiUserCallbackHandler)

        lwz     r.0, ErExceptionFlags (r.3)     // get exception flags
        andi.   r.0, r.0, EXCEPTION_UNWIND      // check if unwind in progress
        beq     UCH10                           // if eq, no unwind in progress

//
// There is an attempt to unwind through a callback frame. If this were
// allowed, then a kernel callback frame would be abandoned on the kernel
// stack. Force a callback return.
//

        lwz     r.5, ErExceptionCode(r.3)       // get exception code
        li      r.3, 0                          // set zero buffer address
        li      r.4, 0                          // set zero buffer lenfth
        lwz     r.7, [toc]ZwCallbackReturn(r.toc) // get addr of function descriptor
        lwz     r.0, 0 (r.7)                    // get entry point address
        mtlr    r.0                             //   into Link Reg
        lwz     r.toc, 4 (r.7)                  // get callee's TOC pointer
        blrl                                    // return to kernel mode

//
// Unsuccessful completion after attempting to return to kernel mode. Use
// the return status as the exception code, set noncontinuable exception and
// attempt to raise another exception. Note there is no return from raise
// status.
//

        ori     r.toc, r.31, 0          // reload our own TOC pointer

        ori     r.31, r.3, 0            // save status value
UCHloop:
        bl      ..RtlRaiseStatus        // raise exception
        ori     r.3, r.31, 0            // set status value
        b       UCHloop                 // loop on return

UCH10:
        li      r.3, ExceptionContinueSearch    // set disposition value

        NESTED_EXIT (KiUserCallbackHandler, HFrameLength, 1, 0)

//
// Define layout and length of User Exception Dispatcher stack frame.
//    N.B.  This must exactly match the computations in KiDispatchException
//

                .struct 0
EDFrame:        .space  StackFrameHeaderLength
EDExcept:       .space  ExceptionRecordLength
EDContext:      .space  ContextFrameLength
EDToc:          .long   0
                .space  STK_SLACK_SPACE
                .align  3
EDFrameLength:

                .text

//      SBTTL("User Exception Dispatcher")
//++
//
// The following code is never executed. Its purpose is to support unwinding
// through the call to the exception dispatcher.
//
//--

        FN_TABLE (KiUserExceptionDispatch, 0, 0)

        DUMMY_ENTRY (KiUserExceptionDispatch)

        stwu    r.sp, -EDFrameLength (r.sp)     // buy stack frame
        mflr    r.0                             // save linkage
        stw     r.0, EDContext + CxLr (r.sp)    //   regs
        mflr    r.0                             // needed by vunwind code
        stw     r.0, EDContext + CxIar (r.sp)
        stw     r.toc, EDToc (r.sp)

        stw     r.13, EDContext + CxGpr13 (r.sp)  // save non-volatile integer state
        stw     r.14, EDContext + CxGpr14 (r.sp)
        stw     r.15, EDContext + CxGpr15 (r.sp)
        stw     r.16, EDContext + CxGpr16 (r.sp)
        stw     r.17, EDContext + CxGpr17 (r.sp)
        stw     r.18, EDContext + CxGpr18 (r.sp)
        stw     r.19, EDContext + CxGpr19 (r.sp)
        stw     r.20, EDContext + CxGpr20 (r.sp)
        stw     r.21, EDContext + CxGpr21 (r.sp)
        stw     r.22, EDContext + CxGpr22 (r.sp)
        stw     r.23, EDContext + CxGpr23 (r.sp)
        stw     r.24, EDContext + CxGpr24 (r.sp)
        stw     r.25, EDContext + CxGpr25 (r.sp)
        stw     r.26, EDContext + CxGpr26 (r.sp)
        stw     r.27, EDContext + CxGpr27 (r.sp)
        stw     r.28, EDContext + CxGpr28 (r.sp)
        stw     r.29, EDContext + CxGpr29 (r.sp)
        stw     r.30, EDContext + CxGpr30 (r.sp)
        stw     r.31, EDContext + CxGpr31 (r.sp)
        stfd    f.14, EDContext + CxFpr14 (r.sp)  // save non-volatile floating point state
        stfd    f.15, EDContext + CxFpr15 (r.sp)
        stfd    f.16, EDContext + CxFpr16 (r.sp)
        stfd    f.17, EDContext + CxFpr17 (r.sp)
        stfd    f.18, EDContext + CxFpr18 (r.sp)
        stfd    f.19, EDContext + CxFpr19 (r.sp)
        stfd    f.20, EDContext + CxFpr20 (r.sp)
        stfd    f.21, EDContext + CxFpr21 (r.sp)
        stfd    f.22, EDContext + CxFpr22 (r.sp)
        stfd    f.23, EDContext + CxFpr23 (r.sp)
        stfd    f.24, EDContext + CxFpr24 (r.sp)
        stfd    f.25, EDContext + CxFpr25 (r.sp)
        stfd    f.26, EDContext + CxFpr26 (r.sp)
        stfd    f.27, EDContext + CxFpr27 (r.sp)
        stfd    f.28, EDContext + CxFpr28 (r.sp)
        stfd    f.29, EDContext + CxFpr29 (r.sp)
        stfd    f.30, EDContext + CxFpr30 (r.sp)
        stfd    f.31, EDContext + CxFpr31 (r.sp)

        PROLOGUE_END (KiUserExceptionDispatch)

//++
//
// VOID
// KiUserExceptionDispatcher (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN PCONTEXT ContextRecord
//    )
//
// Routine Description:
//
//    This routine is entered on return from kernel mode to dispatch a user
//    mode exception. If a frame based handler handles the exception, then
//    the execution is continued. Else last chance processing is performed.
//
// Arguments:
//
//    r.3 - Supplies a pointer to an exception record.
//
//    r.4 - Supplies a pointer to a context record.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiUserExceptionDispatcher)

        bl      ..RtlDispatchException         // attempt to dispatch the exception

//
// If the return status is TRUE, then the exception was handled and execution
// should be continued with the NtContinue service in case the context was
// changed. If the return status is FALSE, then the exception was not handled
// and NtRaiseException is called to perform last chance exception processing.
//

        cmpwi   r.3, 0                          // compare return value to FALSE
        beq     ED10                            // if eq, perform last chance processing

//
// Continue execution.
//

        lwz     r.5, [toc] ZwContinue (r.2)     // load pointer to function descriptor
        la      r.3, EDContext (r.sp)           // set addr of context frame
        lwz     r.0, 0 (r.5)                    // load entry point address
        mtlr    r.0                             // move into Link Reg
        li      r.4, 0                          // set test alert argument false
        lwz     r.2, 4 (r.5)                    // load ZwContinue's TOC pointer
        blrl                                    // execute system service to continue
        lwz     r.2, EDToc (r.sp)               // reload our own TOC address
        b       ED20                            // join common code

//
// Last chance processing.
//

ED10:
        lwz     r.6, [toc] ZwRaiseException (r.2) // load pointer to function descriptor
        la      r.3, EDExcept (r.sp)            // set address of exception record
        lwz     r.0, 0 (r.6)                    // load entry point address
        mtlr    r.0                             //   into link reg
        la      r.4, EDContext (r.sp)           // set address of context frame
        li      r.5, 0                          // set first chance FALSE
        lwz     r.toc, 4 (r.6)                  // load callee's TOC addr
        blrl                                    // perform last chance processing
        lwz     r.toc, EDToc (r.sp)             // reload our own TOC address

//
// Common code for nonsuccessful completion of the continue or last chance
// service. Use the return status as the exception code, set noncontinuable
// exception and attempt to raise another exception. Note the stack grows
// and eventually this loop will end.
//

ED20:                                           // status value is in r.3
        la      r.4, EDExcept (r.sp)            // point to our exception record
        bl      ..KipUserExceptionDispatcherLoop // call subroutine below

        DUMMY_EXIT (KiUserExceptionDispatch)

//++
//
// VOID
// KiUserExceptionDispatcherLoop (
//    IN ULONG ExceptionStatus
//    )
//
// Routine Description:
//
//    This routine builds an Exception Record and calls RtlRaiseException.
//    On an unsuccessful return, it calls itself recursively; eventually
//    this will terminate when the stack fills up.
//
// Arguments:
//
//    r.3 - Status value
//
// Return Value:
//
//    None.  (Does not return.)
//
//--

//
// Stack frame layout for KipUserExceptionDispatchLoop
//

                .struct 0
LFrame:         .space  StackFrameHeaderLength
LExcept:        .space  ExceptionRecordLength
LOldExcept:     .long   0
LSavedLR:       .long   0
                .align  3
LFrameLength:

                .text

        NESTED_ENTRY (KipUserExceptionDispatcherLoop, LFrameLength, 0, 0)

        PROLOGUE_END (KipUserExceptionDispatcherLoop)

        stw     r.4, LOldExcept (r.sp)          // save incoming exception rec addr
        la      r.5, LExcept (r.sp)             // point to Exception Record
        stw     r.3, ErExceptionCode (r.5)      // fill in exception code (incoming status)
        li      r.6, EXCEPTION_NONCONTINUABLE   // set non-continuable flag
        stw     r.6, ErExceptionFlags (r.5)
        stw     r.4, ErExceptionRecord (r.5)    // set addr of prev. exception record
        li      r.0, 0                          // set number of parameters
        stw     r.0, ErNumberParameters (r.5)   //   to 0
        ori     r.3, r.5, 0                     // load 1st parameter pointer
        bl      ..RtlRaiseException            // raise an exception

        lwz     r.4, LOldExcept (r.sp)          // should not return, but if so:
        bl      ..KipUserExceptionDispatcherLoop // keep doing this in a loop

        NESTED_EXIT (KipUserExceptionDispatcherLoop, LFrameLength, 0, 0)

//++
//
// NTSTATUS
// KiRaiseUserExceptionDispatcher (
//    IN NTSTATUS ExceptionCode
//    )
//
// Routine Description:
//
//    This routine is entered on return from kernel mode to raise a user
//    mode exception.
//
// Arguments:
//
//    r3 - Supplies the status code to be raised.
//
// Return Value:
//
//    ExceptionCode
//
//--

//
// N.B. This function is not called in the typical way. Instead of a normal
// subroutine call to the nested entry point above, the alternate entry point
// address below is stuffed into the Iar address of the trap frame. Thus when
// the kernel returns from the trap, the following code is executed directly.
//

                .struct 0
ruedFrame:      .space  StackFrameHeaderLength
ruedExr:        .space  ExceptionRecordLength
ruedR3:         .space  4
ruedLr:         .space  4
                .align  3
ruedFrameLength:

        SPECIAL_ENTRY(KiRaiseUserExceptionDispatcher)

        mflr    r4                              // get return address (also exception address)
        stwu    sp,-ruedFrameLength(sp)         // allocate stack frame
        li      r0,0                            // get a 0
        stw     r4,ruedLr(sp)                   // save return address

        PROLOGUE_END(KiRaiseUserExceptionDispatcher)

        stw     r3,ruedR3(sp)                   // save function return status
        stw     r3,ErExceptionCode+ruedExr(sp)  // set exception code
        la      r3,ruedExr(sp)                  // compute exception record address
        lwz     r4,ruedLr(sp)                   // get exception address
        stw     r0,ErExceptionFlags(r3)         // set exception flags
        stw     r0,ErExceptionRecord(r3)        // set exception record
        stw     r0,ErNumberParameters(r3)       // set number of parameters
        stw     r4,ErExceptionAddress(r3)       // set exception address

        bl      ..RtlRaiseException             // attempt to raise the exception

        lwz     r3,ruedR3(sp)                   // restore function status

        NESTED_EXIT (KiRaiseUserExceptionDispatcher, ruedFrameLength, 0, 0)

