//      TITLE("Miscellaneous Exception Handling")
//++
//
// Copyright (c) 1990  Microsoft Corporation
// Copyright (c) 1992  Digital Equipment Corporation
//
// Module Name:
//
//    xcptmisc.s
//
// Abstract:
//
//    This module implements miscellaneous routines that are required to
//    support exception handling. Functions are provided to call an exception
//    handler for an exception, call an exception handler for unwinding, call
//    an exception filter, call a termination handler, and get the caller's
//    stack limits.
//
// Author:
//
//    David N. Cutler (davec) 12-Sep-1990
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//    Thomas Van Baak (tvb) 7-May-1992
//
//        Adapted for Alpha AXP.
//
//--

#include "ksalpha.h"

//
// Define call frame for calling exception handlers.
//

        .struct 0
CfRa:   .space  8                       // saved return addwress
CfA3:   .space  8                       // save area for argument a3
        .space  0 * 8                   // 16-byte stack alignment
CfFrameLength:                          // length of stack frame

        SBTTL("Execute Handler for Exception")
//++
//
// EXCEPTION_DISPOSITION
// RtlpExecuteHandlerForException (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN UINT_PTR EstablisherFrame,
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
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (a1) - Supplies the frame pointer of the establisher
//       of the exception handler that is to be called.
//
//    ContextRecord (a2) - Supplies a pointer to a context record.
//
//    DispatcherContext (a3) - Supplies a pointer to the dispatcher context
//       record.
//
//    ExceptionRoutine (a4) - Supplies a pointer to the exception handler that
//       is to be called.
//
// Return Value:
//
//    The disposition value returned by the specified exception handler is
//    returned as the function value.
//
//--

//
// N.B. This function specifies its own private exception handler.
//

        EXCEPTION_HANDLER(RtlpExceptionHandler)

        NESTED_ENTRY(RtlpExecuteHandlerForException, CfFrameLength, zero)

        lda     sp, -CfFrameLength(sp)  // allocate stack frame
        stq     ra, CfRa(sp)            // save return address

        PROLOGUE_END

//
// Save the address of the dispatcher context record in our stack frame so
// that our own exception handler (not the one we're calling) can retrieve it.
//

        stq     a3, CfA3(sp)            // save address of dispatcher context

//
// Now call the exception handler and return its return value as ours.
//

        bic     a4, 3, a4               // clear low-order bits (IEEE mode)
        jsr     ra, (a4)                // call exception handler

        ldq     ra, CfRa(sp)            // restore return address
        lda     sp, CfFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

        .end    RtlpExecuteHandlerForException

        SBTTL("Local Exception Handler")
//++
//
// EXCEPTION_DISPOSITION
// RtlpExceptionHandler (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN UINT_PTR EstablisherFrame,
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
//    A disposition value ExceptionNestedException is returned if an unwind
//    is not in progress. Otherwise a value of ExceptionContinueSearch is
//    returned.
//
//--

        LEAF_ENTRY(RtlpExceptionHandler)

        ldl     t0, ErExceptionFlags(a0)        // get exception flags
        and     t0, EXCEPTION_UNWIND, t0        // check if unwind in progress
        bne     t0, 10f                         // if neq, unwind in progress

//
// Unwind is not in progress - return nested exception disposition.
//

//
// Convert the given establisher virtual frame pointer (a1) to a real frame
// pointer (the value of a1 minus CfFrameLength) and retrieve the pointer to
// the dispatcher context that earlier was stored in the stack frame.
//

        ldq     t0, -CfFrameLength + CfA3(a1)   // get dispatcher context address

        LDP     t1, DcEstablisherFrame(t0)      // copy the establisher frame pointer
        STP     t1, DcEstablisherFrame(a3)      // to current dispatcher context

        ldil    v0, ExceptionNestedException    // set disposition value
        ret     zero, (ra)                      // return

//
// Unwind is in progress - return continue search disposition.
//

10:     ldil    v0, ExceptionContinueSearch     // set disposition value
        ret     zero, (ra)                      // return

        .end    RtlpExceptionHandler

        SBTTL("Execute Handler for Unwind")
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
//    handler of this function is called and the establisher frame pointer
//    and context record address are returned to the unwind dispatcher via
//    the dispatcher context parameter. If control is returned to this routine,
//    then the frame is deallocated and the disposition status is returned to
//    the unwind dispatcher.
//
// Arguments:
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (a1) - Supplies the frame pointer of the establisher
//       of the exception handler that is to be called.
//
//    ContextRecord (a2) - Supplies a pointer to a context record.
//
//    DispatcherContext (a3) - Supplies a pointer to the dispatcher context
//       record.
//
//    ExceptionRoutine (a4) - Supplies a pointer to the exception handler that
//       is to be called.
//
// Return Value:
//
//    The disposition value returned by the specified exception handler is
//    returned as the function value.
//
//--

//
// N.B. This function specifies its own private exception handler.
//

        EXCEPTION_HANDLER(RtlpUnwindHandler)

        NESTED_ENTRY(RtlpExecuteHandlerForUnwind, CfFrameLength, zero)

        lda     sp, -CfFrameLength(sp)  // allocate stack frame
        stq     ra, CfRa(sp)            // save return address

        PROLOGUE_END

//
// Save the address of the dispatcher context record in our stack frame so
// that our own exception handler (not the one we're calling) can retrieve it.
//

        stq     a3, CfA3(sp)            // save address of dispatcher context

//
// Now call the exception handler and return its return value as our return
// value.
//

        bic     a4, 3, a4               // clear low-order bits (IEEE mode)
        jsr     ra, (a4)                // call exception handler

        ldq     ra, CfRa(sp)            // restore return address
        lda     sp, CfFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

        .end    RtlpExecuteHandlerForUnwind

        SBTTL("Local Unwind Handler")
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
//    A disposition value ExceptionCollidedUnwind is returned if an unwind is
//    in progress. Otherwise, a value of ExceptionContinueSearch is returned.
//
//--

        LEAF_ENTRY(RtlpUnwindHandler)

        ldl     t0, ErExceptionFlags(a0)        // get exception flags
        and     t0, EXCEPTION_UNWIND, t0        // check if unwind in progress
        beq     t0, 10f                         // if eq, unwind not in progress

//
// Unwind is in progress - return collided unwind disposition.
//

//
// Convert the given establisher virtual frame pointer (a1) to a real frame
// pointer (the value of a1 minus CfFrameLength) and retrieve the pointer to
// the dispatcher context that earlier was stored in the stack frame.
//

        ldq     t0, -CfFrameLength + CfA3(a1)   // get dispatcher context address

        LDP     t1, DcControlPc(t0)             // copy the entire dispatcher
        LDP     t2, DcFunctionEntry(t0)         //   context of the establisher
        LDP     t3, DcEstablisherFrame(t0)      //     frame...
        LDP     t4, DcContextRecord(t0)         //

        STP     t1, DcControlPc(a3)             // to the current dispatcher
        STP     t2, DcFunctionEntry(a3)         //   context (it's four words
        STP     t3, DcEstablisherFrame(a3)      //     long).
        STP     t4, DcContextRecord(a3)         //

        ldil    v0, ExceptionCollidedUnwind     // set disposition value
        ret     zero, (ra)                      // return

//
// Unwind is not in progress - return continue search disposition.
//

10:     ldil    v0, ExceptionContinueSearch     // set disposition value
        ret     zero, (ra)                      // return

        .end    RtlpUnwindHandler

        SBTTL("Execute Exception Filter")
//++
//
// ULONG
// RtlpExecuteExceptionFilter (
//    PEXCEPTION_POINTERS ExceptionPointers,
//    EXCEPTION_FILTER ExceptionFilter,
//    UINT_PTR EstablisherFrame
//    )
//
// Routine Description:
//
//    This function sets the static link and transfers control to the specified
//    exception filter routine.
//
// Arguments:
//
//    ExceptionPointers (a0) - Supplies a pointer to the exception pointers
//       structure.
//
//    ExceptionFilter (a1) - Supplies the address of the exception filter
//       routine.
//
//    EstablisherFrame (a2) - Supplies the establisher frame pointer.
//
// Return Value:
//
//    The value returned by the exception filter routine.
//
//--

        LEAF_ENTRY(RtlpExecuteExceptionFilter)

//
// The protocol for calling exception filters used by the acc C-compiler is
// that the uplevel frame pointer is passed in register v0 and the pointer
// to the exception pointers structure is passed in register a0. The Gem
// compiler expects the static link in t0. Here we do both.
//

        mov     a2, v0                  // set static link
        mov     a2, t0                  // set alternate static link
        jmp     zero, (a1)              // transfer control to exception filter

        .end    RtlpExecuteExceptionFilter

        SBTTL("Execute Termination Handler")
//++
//
// VOID
// RtlpExecuteTerminationHandler (
//    BOOLEAN AbnormalTermination,
//    TERMINATION_HANDLER TerminationHandler,
//    UINT_PTR EstablisherFrame
//    )
//
// Routine Description:
//
//    This function sets the static link and transfers control to the specified
//    termination handler routine.
//
// Arguments:
//
//    AbnormalTermination (a0) - Supplies a boolean value that determines
//       whether the termination is abnormal.
//
//    TerminationHandler (a1) - Supplies the address of the termination handler
//       routine.
//
//    EstablisherFrame (a2) - Supplies the establisher frame pointer.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(RtlpExecuteTerminationHandler)

//
// The protocol for calling termination handlers used by the acc C-compiler
// is that the uplevel frame pointer is passed in register v0 and the boolean
// abnormal termination value is passed in register a0. The Gem compiler
// expects the static link in t0. Here we do both.
//

        mov     a2, v0                  // set static link
        mov     a2, t0                  // set alternate static link
        jmp     zero, (a1)              // transfer control to termination handler

        .end    RtlpExecuteTerminationHandler

        SBTTL("Get Stack Limits")
//++
//
// VOID
// RtlpGetStackLimits (
//    OUT PUINT_PTR LowLimit,
//    OUT PUINT_PTR HighLimit
//    )
//
// Routine Description:
//
//    This function returns the current stack limits based on the current
//    processor mode.
//
// Arguments:
//
//    LowLimit (a0) - Supplies a pointer to a variable that is to receive
//       the low limit of the stack.
//
//    HighLimit (a1) - Supplies a pointer to a variable that is to receive
//       the high limit of the stack.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(RtlpGetStackLimits)
#if defined(NTOS_KERNEL_RUNTIME)

//
// Current mode is kernel - compute stack limits.
//

        GET_INITIAL_KERNEL_STACK        // get initial kernel stack in v0

        mov     v0, t1                  // copy high limit of kernel stack
        GET_CURRENT_THREAD              // get current thread in v0
        LDP     t2, ThStackLimit(v0)    // get low limit of kernel stack
#else

//
// Current mode is user - get stack limits from the TEB.
//

        GET_THREAD_ENVIRONMENT_BLOCK    // get address of TEB in v0

        LDP     t1, TeStackBase(v0)     // get high limit of user stack
        LDP     t2, TeStackLimit(v0)    // get low limit of user stack
#endif

        STP     t2, 0(a0)               // store low stack limit
        STP     t1, 0(a1)               // store high stack limit
        ret     zero, (ra)              // return

        .end    RtlpGetStackLimits
