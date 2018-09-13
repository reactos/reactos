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
//--

#include "ksmips.h"

//
// Define call frame for calling exception handlers.
//

        .struct 0
CfArg:  .space  4 * 4                   // argument register save area
        .space  3 * 4                   // fill for alignment
CfRa:   .space  4                       // saved return address
CfFrameLength:                          // length of stack frame
CfA0:   .space  4                       // caller argument save area
CfA1:   .space  4                       //
CfA2:   .space  4                       //
CfA3:   .space  4                       //
CfExr:  .space  4                       // address of exception routine

        SBTTL("Execute Handler for Exception")
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
//    ExceptionRoutine (4 * 4(sp)) - supplies a pointer to the exception handler
//       that is to be called.
//
// Return Value:
//
//    The disposition value returned by the specified exception handler is
//    returned as the function value.
//
//--

        EXCEPTION_HANDLER(RtlpExceptionHandler)

        NESTED_ENTRY(RtlpExecuteHandlerForException, CfFrameLength, zero)

        subu    sp,sp,CfFrameLength     // allocate stack frame
        sw      ra,CfRa(sp)             // save return address

        PROLOGUE_END

        lw      t0,CfExr(sp)            // get address of exception routine
        sw      a3,CfA3(sp)             // save address of dispatcher context
        jal     t0                      // call exception exception handler
        lw      ra,CfRa(sp)             // restore return address
        addu    sp,sp,CfFrameLength     // deallocate stack frame
        j       ra                      // return

        .end    RtlpExecuteHandlerForException

        SBTTL("Local Exception Handler")
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

        lw      t0,ErExceptionFlags(a0) // get exception flags
        and     t0,t0,EXCEPTION_UNWIND  // check if unwind in progress
        bne     zero,t0,10f             // if neq, unwind in progress

//
// Unwind is not in progress - return nested exception disposition.
//

        lw      t0,CfA3 - CfA0(a1)      // get dispatcher context address
        li      v0,ExceptionNestedException // set disposition value
        lw      t1,DcEstablisherFrame(t0) // copy the establisher frame pointer
        sw      t1,DcEstablisherFrame(a3) // to current dispatcher context
        j       ra                      // return

//
// Unwind is in progress - return continue search disposition.
//

10:     li      v0,ExceptionContinueSearch // set disposition value
        j       ra                      // return

        .end    RtlpExceptionHandler)

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
//    handler of of this function is called and the establisher frame pointer
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
//    ExceptionRoutine (4 * 4(sp)) - supplies a pointer to the exception handler
//       that is to be called.
//
// Return Value:
//
//    The disposition value returned by the specified exception handler is
//    returned as the function value.
//
//--

        EXCEPTION_HANDLER(RtlpUnwindHandler)

        NESTED_ENTRY(RtlpExecuteHandlerForUnwind, CfFrameLength, zero)

        subu    sp,sp,CfFrameLength     // allocate stack frame
        sw      ra,CfRa(sp)             // save return address

        PROLOGUE_END

        lw      t0,CfExr(sp)            // get address of exception routine
        sw      a3,CfA3(sp)             // save address of dispatcher context
        jal     t0                      // call exception unwind handler
        lw      ra,CfRa(sp)             // restore return address
        addu    sp,sp,CfFrameLength     // deallocate stack frame
        j       ra                      // return

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

        lw      t0,ErExceptionFlags(a0) // get exception flags
        and     t0,t0,EXCEPTION_UNWIND  // check if unwind in progress
        beq     zero,t0,10f             // if eq, unwind not in progress

//
// Unwind is in progress - return collided unwind disposition.
//

        lw      t0,CfA3 - CfA0(a1)      // get dispatcher context address
        li      v0,ExceptionCollidedUnwind // set disposition value
        lw      t1,DcControlPc(t0)      // Copy the establisher frames'
        lw      t2,DcFunctionEntry(t0)  // dispatcher context to the current
        lw      t3,DcEstablisherFrame(t0) // dispatcher context
        lw      t4,DcContextRecord(t0)  //
        sw      t1,DcControlPc(a3)      //
        sw      t2,DcFunctionEntry(a3)  //
        sw      t3,DcEstablisherFrame(a3) //
        sw      t4,DcContextRecord(a3)  //
        j       ra                      // return

//
// Unwind is not in progress - return continue search disposition.
//

10:     li      v0,ExceptionContinueSearch // set disposition value
        j       ra                      // return
        .end    RtlpUnwindHandler

        SBTTL("Get Stack Limits")
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

        li      t0,UsPcr                // get address of user PCR
        bgez    sp,10f                  // if gez, current mode is user

//
// Current mode is kernel - compute stack limits.
//

        lw      t1,KiPcr + PcInitialStack(zero) // get high limit kernel stack
        lw      t2,KiPcr + PcStackLimit(zero) // get low limit kernel stack
        b       20f                     // finish in commom code

//
// Current mode is user - get stack limits from the TEB.
//

10:     lw      t0,PcTeb(t0)            // get address of TEB
        lw      t2,TeStackLimit(t0)     // get low limit of user stack
        lw      t1,TeStackBase(t0)      // get high limit of user stack
20:     sw      t2,0(a0)                // store low stack limit
        sw      t1,0(a1)                // store high stack limit
        j       ra                      // return

        .end    RtlpGetStackLimits
