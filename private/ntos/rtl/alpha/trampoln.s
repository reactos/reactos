//      TITLE("Trampoline Code For User Mode APC and Exception Dispatching")
//++
//
// Copyright (c) 1990  Microsoft Corporation
// Copyright (c) 1992  Digital Equipment Corporation
//
// Module Name:
//
//    trampoln.s
//
// Abstract:
//
//    This module implements the trampoline code necessary to dispatch user
//    mode APCs and exceptions.
//
// Author:
//
//    David N. Cutler (davec) 3-Apr-1990
//
// Environment:
//
//    User mode only.
//
// Revision History:
//
//    Thomas Van Baak (tvb) 11-May-1992
//
//        Adapted for Alpha AXP.
//
//--

#include "ksalpha.h"

//
// Define length of exception dispatcher stack frame.
//

#define ExceptionDispatcherFrameLength (ExceptionRecordLength + ContextFrameLength)

        SBTTL("User APC Dispatcher")
//++
//
// The following code is never executed. Its purpose is to support unwinding
// through the call to the APC dispatcher.
//
//--

//
// N.B. This function specifies its own private exception handler.
//
        EXCEPTION_HANDLER(KiUserApcHandler)

        NESTED_ENTRY(KiUserApcDispatch, ContextFrameLength, zero);

        .set    noreorder
        .set    noat
        stq     sp, CxIntSp(sp)         // save stack pointer
        stq     ra, CxIntRa(sp)         // save return address
        stq     ra, CxFir(sp)           // set continuation address
        stq     fp, CxIntFp(sp)         // save integer register fp
        stq     gp, CxIntGp(sp)         // save integer register gp

        stq     s0, CxIntS0(sp)         // save integer registers s0 - s5
        stq     s1, CxIntS1(sp)         //
        stq     s2, CxIntS2(sp)         //
        stq     s3, CxIntS3(sp)         //
        stq     s4, CxIntS4(sp)         //
        stq     s5, CxIntS5(sp)         //

        stt     f2, CxFltF2(sp)         // save floating registers f2 - f9
        stt     f3, CxFltF3(sp)         //
        stt     f4, CxFltF4(sp)         //
        stt     f5, CxFltF5(sp)         //
        stt     f6, CxFltF6(sp)         //
        stt     f7, CxFltF7(sp)         //
        stt     f8, CxFltF8(sp)         //
        stt     f9, CxFltF9(sp)         //

        mov     sp, fp                  // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

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
//    in user mode. The context frame for this routine was built when the
//    APC interrupt was processed and contains the entire machine state of
//    the current thread. The specified APC routine is called and then the
//    machine state is restored and execution is continued.
//
// Arguments:
//
//    a0 - Supplies the normal context parameter that was specified when the
//       APC was initialized.
//
//    a1 - Supplies the first argument that was provided by the executive when
//       the APC was queued.
//
//    a2 - Supplies the second argument that was provided by the executive
//       when the APC was queued.
//
//    a3 - Supplies the address of the function that is to be called.
//
//    N.B. Register sp supplies a pointer to a context frame.
//
//    N.B. Register fp supplies the same value as sp and is used as a frame
//       pointer.
//
// Return Value:
//
//    None.
//
//--

//
// N.B. This function is not called in the typical way. Instead of a normal
// subroutine call to the nested entry point above, the alternate entry point
// address below is stuffed into the Fir address of the trap frame. Thus when
// the kernel returns from the trap, the following code is executed directly.
//
        ALTERNATE_ENTRY(KiUserApcDispatcher)

        jsr     ra, (a3)                // call specified APC routine

        mov     fp, a0                  // set address of context frame
        ldil    a1, TRUE                // set test alert argument true
        bsr     ra, ZwContinue          // execute system service to continue
        mov     v0, s0                  // save status value

//
// Unsuccessful completion after attempting to continue execution. Use the
// return status as the exception code, set noncontinuable exception and
// attempt to raise another exception. Note there is no return from raise
// status.
//

10:     mov     s0, a0                  // set status value
        bsr     ra, RtlRaiseStatus      // raise exception
        br      zero, 10b               // loop on return

        .end    KiUserApcDispatch

        SBTTL("User APC Exception Handler")
//++
//
// EXCEPTION_DISPOSITION
// KiUserApcHandler (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN UINT_PTR EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PDISPATCHER_CONTEXT DispatcherContext
//
// Routine Description:
//
//    This function is called when an exception occurs in an APC routine
//    or one of its dynamic descendents, or when an unwind through the
//    APC dispatcher is in progress. If an unwind is in progress, then test
//    alert is called to ensure that all currently queued APCs are executed.
//
// Arguments:
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (a1) - Supplies the frame pointer of the establisher
//       of this exception handler.
//
//    ContextRecord (a2) - Supplies a pointer to a context record.
//
//    DispatcherContext (a3) - Supplies a pointer to the dispatcher context
//       record.
//
// Return Value:
//
//    ExceptionContinueSearch is returned as the function value.
//
//--

        .struct 0
HdRa:   .space  8                       // saved return address
        .space  1 * 8                   // required for 16-byte stack alignment
HandlerFrameLength:                     // length of handler frame

        NESTED_ENTRY(KiUserApcHandler, HandlerFrameLength, zero)

        lda     sp, -HandlerFrameLength(sp)     // allocate stack frame
        stq     ra, HdRa(sp)                    // save return address

        PROLOGUE_END

//
// The following code is equivalent to:
//
//      EXCEPTION_DISPOSITION
//      KiUserApcHandler(IN PEXCEPTION_RECORD ExceptionRecord)
//      {
//          if (IS_UNWINDING(ExceptionRecord->ExceptionFlags)) {
//              NtTestAlert();
//          }
//          return ExceptionContinueSearch
//      }
//

        ldl     t0, ErExceptionFlags(a0)        // get exception flags
        and     t0, EXCEPTION_UNWIND, t0        // check if unwind in progress
        beq     t0, 10f                         // if eq, no unwind in progress

        bsr     ra, ZwTestAlert                 // test for alert pending

10:     ldil    v0, ExceptionContinueSearch     // set disposition value
        ldq     ra, HdRa(sp)                    // restore return address
        lda     sp, HandlerFrameLength(sp)      // deallocate stack frame
        ret     zero, (ra)                      // return

        .end    KiUserApcHandler


        SBTTL("User Callback Dispatcher")
//++
//
// The following code is never executed. Its purpose is to support unwinding
// through the call to the exception dispatcher.
//
//--

        NESTED_ENTRY(KiUserCallbackDispatch, ContextFrameLength, zero);
.set noreorder
        stq     sp, CkSp(sp)
        stq     ra, CkRa(sp)
.set reorder
        PROLOGUE_END
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
//    (sp + 16) - Supplies a value of zero for alignment.
//
//    (sp + 24) - Supplies the API number of the callback function that is
//        executed.
//
//    (sp + 32) - Supplies a pointer to the input buffer.
//
//    (sp + 40) - Supplies the input buffer length.
//
// Return Value:
//
//    This function returns to kernel mode.
//
//--

        ALTERNATE_ENTRY(KiUserCallbackDispatcher)

        LDP     a0, CkBuffer(sp)        // get input buffer address
        ldl     a1, CkLength(sp)        // get input buffer length
        ldl     t0, CkApiNumber(sp)     // get API number
        GET_THREAD_ENVIRONMENT_BLOCK    // get TEB in v0
        LDP     t5, TePeb(v0)           // get PEB in t5
        LDP     t2, PeKernelCallbackTable(t5)  // get address of callback table
        SPADDP  t0, t2, t3              // get address of callback
        LDP     t4, 0(t3)               // get callback pointer
        jsr     ra, (t4)                // call specified function

//
// If a return from the callback function occurs, then the output buffer
// address and length are returned as NULL.
//

        bis     zero,zero,a0            // set zero buffer address
        bis     zero,zero,a1            // set zero buffer length
        bis     v0, zero, a2            // set completion status
        bsr     ra, ZwCallbackReturn    // return to kernel mode

//
// Unsuccessful completion after attempting to return to kernel mode. Use
// the return status as the exception code, set noncontinuable exception and
// attempt to raise another exception. Note there is no return from raise
// status.
//

        bis     v0, zero, s0            // save status value
10:     bis     s0, zero, a0            // set status value
        bsr     ra, RtlRaiseStatus      // raise exception
        br      zero, 10b               // loop on return

        .end    KiUserCallbackDispatch

        SBTTL("User Exception Dispatcher")
//++
//
// The following code is never executed. Its purpose is to support unwinding
// through the call to the exception dispatcher.
//
// When reverse executed, this prologue will restore all integer registers,
// rather than just the non-volatile registers. This is necessary for proper
// unwinding through the call to the exception dispatcher when non-standard
// calls have been used in frames at or above the exception frame.  Non-leaf
// functions using a non-standard call are allowed to save the return address
// register in another integer register instead of on the stack.
//
//--

        NESTED_ENTRY(KiUserExceptionDispatch, ExceptionDispatcherFrameLength, zero);

        .set    noreorder
        .set    noat
        stq     sp, CxIntSp(sp)         // save stack pointer
        stq     ra, CxIntRa(sp)         // save return address
        stq     ra, CxFir(sp)           // set continuation address

        stq     v0, CxIntV0(sp)         // save integer register v0
        stq     t0, CxIntT0(sp)         // save integer registers t0 - t6
        stq     t1, CxIntT1(sp)         //
        stq     t2, CxIntT2(sp)         //
        stq     t3, CxIntT3(sp)         //
        stq     t4, CxIntT4(sp)         //
        stq     t5, CxIntT5(sp)         //
        stq     t6, CxIntT6(sp)         //
        stq     t7, CxIntT7(sp)         //

        stq     s0, CxIntS0(sp)         // save integer registers s0 - s5
        stq     s1, CxIntS1(sp)         //
        stq     s2, CxIntS2(sp)         //
        stq     s3, CxIntS3(sp)         //
        stq     s4, CxIntS4(sp)         //
        stq     s5, CxIntS5(sp)         //
        stq     fp, CxIntFp(sp)         // save integer register fp

        stq     a0, CxIntA0(sp)         // save integer registers a0 - a5
        stq     a1, CxIntA1(sp)         //
        stq     a2, CxIntA2(sp)         //
        stq     a3, CxIntA3(sp)         //
        stq     a4, CxIntA4(sp)         //
        stq     a5, CxIntA5(sp)         //

        stq     t8, CxIntT8(sp)         // save integer registers t8 - t11
        stq     t9, CxIntT9(sp)         //
        stq     t10, CxIntT10(sp)       //
        stq     t11, CxIntT11(sp)       //

        stq     t12, CxIntT12(sp)       // save integer register t12
        stq     AT, CxIntAt(sp)         // save integer register AT
        stq     gp, CxIntGp(sp)         // save integer register gp

        stt     f2, CxFltF2(sp)         // save floating registers f2 - f9
        stt     f3, CxFltF3(sp)         //
        stt     f4, CxFltF4(sp)         //
        stt     f5, CxFltF5(sp)         //
        stt     f6, CxFltF6(sp)         //
        stt     f7, CxFltF7(sp)         //
        stt     f8, CxFltF8(sp)         //
        stt     f9, CxFltF9(sp)         //

        mov     sp, fp                  // set frame pointer
        .set    at
        .set    reorder

        PROLOGUE_END

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
//    the execution is continued. Otherwise last chance processing is performed.
//
// Arguments:
//
//    s0 - Supplies a pointer to an exception record.
//
//    s1 - Supplies a pointer to a context frame.
//
//    fp - Supplies the same value as sp and is used as a frame pointer.
//
// Return Value:
//
//    None.
//
//--

//
// N.B. This function is not called in the typical way. Instead of a normal
// subroutine call to the nested entry point above, the alternate entry point
// address below is stuffed into the Fir address of the trap frame. Thus when
// the kernel returns from the trap, the following code is executed directly.
//

        ALTERNATE_ENTRY(KiUserExceptionDispatcher)
#if defined(_AXP64_)
        //
        // Give WOW64 a chance to clean up before the exception is dispatched.
        // This must be done from assembly code as WOW64 may switch stacks.
        //
        ldq     t0, Wow64PrepareForException
        beq     t0, 5f
        jsr     ra, (t0)
        // returns with a0 and a1 preserved
5:
#endif

        mov     s0, a0                  // set address of exception record
        mov     s1, a1                  // set address of context frame
        bsr     ra, RtlDispatchException // attempt to dispatch the exception

//
// If the return status is TRUE, then the exception was handled and execution
// should be continued with the NtContinue service in case the context was
// changed. If the return status is FALSE, then the exception was not handled
// and NtRaiseException is called to perform last chance exception processing.
//

        beq     v0, 10f                 // if eq [false], perform last chance processing

//
// Continue execution.
//

        mov     s1, a0                  // set address of context frame
        ldil    a1, FALSE               // set test alert argument false
        bsr     ra, ZwContinue          // execute system service to continue
        br      zero, 20f               // join common code

//
// Last chance processing.
//

10:     mov     s0, a0                  // set address of exception record
        mov     s1, a1                  // set address of context frame
        ldil    a2, FALSE               // set first chance argument false
        bsr     ra, ZwRaiseException    // perform last chance processing

//
// Common code for unsuccessful completion of the continue or last chance
// service. Use the return status (which is now in v0) as the exception code,
// set noncontinuable exception and attempt to raise another exception. Note
// the stack grows and eventually this loop will end.
//

20:     lda     sp, -ExceptionRecordLength(sp)  // allocate exception record
        mov     sp, a0                          // get address of actual record
        stl     v0, ErExceptionCode(a0)         // set exception code
        ldil    t0, EXCEPTION_NONCONTINUABLE    // set noncontinuable flag
        stl     t0, ErExceptionFlags(a0)        // store exception flags
        STP     s0, ErExceptionRecord(a0)       // set associated exception record
        stl     zero, ErNumberParameters(a0)    // set number of parameters
        bsr     ra, RtlRaiseException           // raise exception
        br      zero, 20b                       // loop on error

        .end    KiUserExceptionDispatch

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
//    v0 - Supplies the status code to be raised.
//
// Return Value:
//
//    ExceptionCode
//
//--

//
// N.B. This function is not called in the typical way. Instead of a normal
// subroutine call to the nested entry point above, the alternate entry point
// address below is stuffed into the Fir address of the trap frame. Thus when
// the kernel returns from the trap, the following code is executed directly.
//

        .struct 0
RaiseRa: .space  8                       // saved return address
RaiseV0: .space  8                       // saved S0
RaiseExr: .space ExceptionRecordLength  // exception record for RtlRaiseException
RaiseFrameLength:                        // length of handler frame

        NESTED_ENTRY(KiRaiseUserExceptionDispatcher, RaiseFrameLength, zero)
        lda     sp, -RaiseFrameLength(sp)   // allocate stack frame
        stq     ra, RaiseRa(sp)             // save return address
        PROLOGUE_END

        stq     v0, RaiseV0(sp)                     // save function return status
        stl     v0, ErExceptionCode+RaiseExr(sp)     // set exception code
        stl     zero, ErExceptionFlags+RaiseExr(sp)  // set exception flags
        STP     zero, ErExceptionRecord+RaiseExr(sp) // set exception record
        STP     ra, ErExceptionAddress+RaiseExr(sp)  // set exception address
        stl     zero, ErNumberParameters+RaiseExr(sp)

        lda     a0, RaiseExr(sp)    // set argument to RtlRaiseException
        bsr     ra, RtlRaiseException          // attempt to raise the exception

        ldq     v0, RaiseV0(sp)                // return status

        ldq     ra, RaiseRa(sp)             // restore ra
        lda     sp, RaiseFrameLength(sp)    // deallocate stack frame
        ret     zero, (ra)                  // return

        .end    KiRaiseUserExceptionDispatch
