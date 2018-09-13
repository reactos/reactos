//      TITLE("Kernel Trap Handler")
//++
// Copyright (c) 1990 Microsoft Corporation
// Copyright (c) 1992 Digital Equipment Corporation
//
// Module Name:
//
//     trap.s
//
//
// Abstract:
//
//     Implements trap routines for ALPHA, these are the
//     entry points that the palcode calls for exception
//     processing.
//
//
// Author:
//
//      David N. Cutler (davec) 4-Apr-1990
//      Joe Notarangelo 06-Feb-1992
//
//
// Environment:
//
//      Kernel mode only.
//
//
// Revision History:
//
//      Nigel Haslock 05-May-1995       preserve fpcr across system calls
//
//--

#include "ksalpha.h"

//
// Define exception handler frame
//

        .struct 0
HdRa:   .space  8                       // return address
        .space  3*8                     // round to cache block
HandlerFrameLength:                     // frame length

        SBTTL("General Exception Dispatch")
//++
//
// Routine Description:
//
//     The following code is never executed.  Its purpose is to allow the
//     kernel debugger to walk call frames backwards through an exception
//     to support unwinding through exceptions for system services, and to
//     support get/set user context.
//
//    N.B. The volatile registers must be saved in this prologue because
//         the compiler will occasionally generate code that uses volatile
//         registers to save the contents of nonvolatile registers when
//         a function only calls another function with a known register
//         signature (such as _OtsDivide).
//
//--

        NESTED_ENTRY(KiGeneralExceptionDispatch, TrapFrameLength, zero)

        .set    noreorder
        stq     sp, TrIntSp(sp)         // save stack pointer
        stq     ra, TrIntRa(sp)         // save return address
        stq     ra, TrFir(sp)           // save return address
        stq     fp, TrIntFp(sp)         // save frame pointer
        stq     gp, TrIntGp(sp)         // save global pointer
        bis     sp, sp, fp              // set frame pointer
        .set    reorder

        stq     v0, TrIntV0(sp)         // save integer register v0
        stq     t0, TrIntT0(sp)         // save integer registers t0 - t7
        stq     t1, TrIntT1(sp)         //
        stq     t2, TrIntT2(sp)         //
        stq     t3, TrIntT3(sp)         //
        stq     t4, TrIntT4(sp)         //
        stq     t5, TrIntT5(sp)         //
        stq     t6, TrIntT6(sp)         //
        stq     t7, TrIntT7(sp)         //
        stq     a4, TrIntA4(sp)         // save integer registers a4 - a5
        stq     a5, TrIntA5(sp)         //
        stq     t8, TrIntT8(sp)         // save integer registers t8 - t12
        stq     t9, TrIntT9(sp)         //
        stq     t10, TrIntT10(sp)       //
        stq     t11, TrIntT11(sp)       //
        stq     t12, TrIntT12(sp)       //

        .set    noat
        stq     AT, TrIntAt(sp)         // save integer register AT
        .set    at

        PROLOGUE_END

//++
//
// Routine Description:
//
//     PALcode dispatches to this kernel entry point when a "general"
//     exception occurs.  These general exceptions are any exception
//     other than an interrupt, system service call or memory management
//     fault.  The types of exceptions that will dispatch through this
//     routine will be: breakpoints, unaligned accesses, machine check
//     errors, illegal instruction exceptions, and arithmetic exceptions.
//     The purpose of this routine is to save the volatile state and
//     enter the common exception dispatch code.
//
// Arguments:
//
//     fp - Supplies a pointer to the trap frame.
//     gp - Supplies a pointer to the system short data area.
//     sp - Supplies a pointer to the trap frame.
//     a0 = Supplies a pointer to the exception record.
//     a3 = Supplies the previous psr.
//
//     Note: Control registers, ra, sp, fp, gp have already been saved
//           argument registers a0-a3 have been saved as well
//
//--

        ALTERNATE_ENTRY(KiGeneralException)

        bsr     ra, KiGenerateTrapFrame // store volatile state
        br      ra, KiExceptionDispatch // handle the exception

        .end    KiGeneralExceptionDispatch

        SBTTL("Exception Dispatch")
//++
//
// Routine Description:
//
//     This routine begins the common code for raising an exception.
//     The routine saves the non-volatile state and dispatches to the
//     next level exception dispatcher.
//
// Arguments:
//
//     fp - Supplies a pointer to the trap frame.
//     sp - Supplies a pointer to the trap frame.
//     a0 = Supplies a pointer to the exception record.
//     a3 = Supplies the previous psr.
//
//     gp, ra - saved in trap frame
//     a0-a3 - saved in trap frame
//
// Return Value:
//
//      None.
//
//--

        NESTED_ENTRY(KiExceptionDispatch, ExceptionFrameLength, zero )

//
// Build exception frame
//

        lda     sp, -ExceptionFrameLength(sp) // allocate exception frame
        stq     ra, ExIntRa(sp)         // save ra
        stq     s0, ExIntS0(sp)         // save integer registers s0 - s5
        stq     s1, ExIntS1(sp)         //
        stq     s2, ExIntS2(sp)         //
        stq     s3, ExIntS3(sp)         //
        stq     s4, ExIntS4(sp)         //
        stq     s5, ExIntS5(sp)         //
        stt     f2, ExFltF2(sp)         // save floating registers f2 - f9
        stt     f3, ExFltF3(sp)         //
        stt     f4, ExFltF4(sp)         //
        stt     f5, ExFltF5(sp)         //
        stt     f6, ExFltF6(sp)         //
        stt     f7, ExFltF7(sp)         //
        stt     f8, ExFltF8(sp)         //
        stt     f9, ExFltF9(sp)         //

        PROLOGUE_END

        ldil    a4, TRUE                // first chance to true
        and     a3, PSR_MODE_MASK, a3   // set previous mode
        bis     fp, zero, a2            // set pointer to trap frame
        bis     sp, zero, a1            // set pointer to exception frame
        bsr     ra, KiDispatchException // dispatch exception

        SBTTL("Exception Exit")
//++
//
// Routine Description:
//
//     This routine is called to exit from an exception.
//
//     N.B. This transfer of control occurs from:
//
//         1. fall-through from above
//         2. exit from continue system service
//         3. exit from raise exception system service
//         4. exit into user mode from thread startup
//
// Arguments:
//
//     fp - Supplies a pointer to the trap frame.
//     sp - Supplies a pointer to the exception frame.
//
// Return Value:
//
//     Does not return.
//
//--

        ALTERNATE_ENTRY(KiExceptionExit)

        ldq     s0, ExIntS0(sp)         // restore integer registers s0 - s5
        ldq     s1, ExIntS1(sp)         //
        ldq     s2, ExIntS2(sp)         //
        ldq     s3, ExIntS3(sp)         //
        ldq     s4, ExIntS4(sp)         //
        ldq     s5, ExIntS5(sp)         //
        ldl     a0, TrPsr(fp)           // get previous psr
        bsr     ra, KiRestoreNonVolatileFloatState // restore nv float state

        ALTERNATE_ENTRY(KiAlternateExit)

//
// on entry:
//
//      a0 = Supplies the previous psr.
//
// rfe will do the following for us:
//
//      set sfw interrupt requests as per a1
//      restore previous irql and mode from previous psr
//      restore registers, a0-a3, fp, sp, ra, gp
//      return to saved exception address in the trap frame
//
//      here, we need to restore the trap frame and determine
//      if we must request an APC interrupt
//

        bis     zero, zero, a1          // assume softwareinterrupt requested
        blbc    a0, 30f                 // if lbc, previous mode kernel

//
// Check to determine if an apc interrupt should be generated.
//

        GET_CURRENT_THREAD              // get current thread address

        ldq_u   t1, ThApcState+AsUserApcPending(v0) // get user APC pending
        extbl   t1, (ThApcState+AsUserApcPending) % 8, t0 //
        ZeroByte(ThAlerted(v0))         // clear kernel mode alerted
        cmovne  t0, APC_INTERRUPT, a1   // if pending set APC interrupt
30:     bsr     ra, KiRestoreTrapFrame  // restore volatile state

//
// a0 = previous psr
// a1 = sfw interrupt requests
//

        RETURN_FROM_TRAP_OR_INTERRUPT   // return from exception

        .end    KiExceptionDispatch

        SBTTL("Memory Management Exception Dispatch")
//++
//
// Routine Description:
//
//     The following code is never executed.  Its purpose is to allow the
//     kernel debugger to walk call frames backwards through an exception
//     to support unwinding through exceptions for system services, and to
//     support get/set user context.
//
//    N.B. The volatile registers must be saved in this prologue because
//         the compiler will occasionally generate code that uses volatile
//         registers to save the contents of nonvolatile registers when
//         a function only calls another function with a known register
//         signature (such as _OtsMove).
//
//--

        NESTED_ENTRY(KiMemoryManagementDispatch, TrapFrameLength, zero)

        .set    noreorder
        stq     sp, TrIntSp(sp)         // save stack pointer
        stq     ra, TrIntRa(sp)         // save return address
        stq     ra, TrFir(sp)           // save return address
        stq     fp, TrIntFp(sp)         // save frame pointer
        stq     gp, TrIntGp(sp)         // save global pointer
        bis     sp, sp, fp              // set frame pointer
        .set    reorder

        stq     v0, TrIntV0(sp)         // save integer register v0
        stq     t0, TrIntT0(sp)         // save integer registers t0 - t7
        stq     t1, TrIntT1(sp)         //
        stq     t2, TrIntT2(sp)         //
        stq     t3, TrIntT3(sp)         //
        stq     t4, TrIntT4(sp)         //
        stq     t5, TrIntT5(sp)         //
        stq     t6, TrIntT6(sp)         //
        stq     t7, TrIntT7(sp)         //
        stq     a4, TrIntA4(sp)         // save integer registers a4 - a5
        stq     a5, TrIntA5(sp)         //
        stq     t8, TrIntT8(sp)         // save integer registers t8 - t12
        stq     t9, TrIntT9(sp)         //
        stq     t10, TrIntT10(sp)       //
        stq     t11, TrIntT11(sp)       //
        stq     t12, TrIntT12(sp)       //

        .set    noat
        stq     AT, TrIntAt(sp)         // save integer register AT
        .set    at

        PROLOGUE_END

//++
//
// Routine Description:
//
//     This routine is called from the PALcode when a translation not valid
//     fault or an access violation is encountered.  This routine will
//     call MmAccessFault to attempt to resolve the fault.  If the fault
//     cannot be resolved then the routine will dispatch to the exception
//     dispatcher so the exception can be raised.
//
// Arguments:
//
//      fp - Supplies a pointer to the trap frame.
//      gp - Supplies a pointer to the system short data area.
//      sp - Supplies a pointer to the trap frame.
//      a0 = Supplies the load/store indicator, 1 = store, 0 = load.
//      a1 = Supplies the bad virtual address.
//      a2 = Supplies the previous mode.
//      a3 = Supplies the previous psr.
//
//      gp, ra - saved in trap frame
//      a0-a3 - saved in trap frame
//
// Return Value:
//
//      None.
//
//--

        ALTERNATE_ENTRY(KiMemoryManagementException)

        bsr     ra, KiGenerateTrapFrame // store volatile state

//
// Save parameters in exception record and save previous psr.
//

        STP     a0, TrExceptionRecord + ErExceptionInformation(fp) // set load/store

#if defined(_AXP64_)

        stq     a1, TrExceptionRecord + ErExceptionInformation + 8(fp) // set bad va

#else

        stl     a1, TrExceptionRecord + ErExceptionInformation + 4(fp) // set bad va

#endif

        stl     a3, TrExceptionRecord + ErExceptionCode(fp) // save previous psr

//
// Call memory management to handle the access fault.
//

        bis     fp, zero, a3            // set address of trap frame
        bsr     ra, MmAccessFault       // resolve memory management fault
        ldl     a3, TrExceptionRecord + ErExceptionCode(fp) // get previous psr

//
// Check if working set watch is enabled.
//

        ldl     t0, PsWatchEnabled      // get working set watch enable flag
        bis     v0, zero, a0            // save status of fault resolution
        blt     v0, 40f                 // if ltz, resolution not successful
        beq     t0, 35f                 // if eq. zero, watch not enabled
        LDP     a1, TrExceptionRecord + ErExceptionAddress(fp) // set exception address

#if defined(_AXP64_)

        ldq     a2, TrExceptionRecord + ErExceptionInformation + 8(fp) // set bad address

#else

        ldl     a2, TrExceptionRecord + ErExceptionInformation + 4(fp) // set bad address

#endif

        bsr     ra, PsWatchWorkingSet   // record working set information

//
// Check if debugger has any breakpoints that should be inserted.
//

35:     ldl     t0, KdpOweBreakpoint    // get owned breakpoint flag
        zap     t0, 0xfe, t1            // mask off high bytes
        beq     t1, 37f                 // if eq, no break points owed
        bsr     ra, KdSetOwedBreakpoints // set owed break points

//
// Exit exception.
//

37:     ldl     a0, TrPsr(fp)           // get previous psr
        br      zero, KiAlternateExit   // exception handled

//
// Check to determine if the fault occured in the interlocked pop
// entry slist code. There is a case where a fault may occur in this
// code when the right set of circumstances occurs. The fault can be
// ignored by simply skipping the faulting instruction.
//

40:     ldq     t0, TrFir(fp)           // get faulting instruction address
        lda     t1, ExpInterlockedPopEntrySListFault // get address of pop code
        cmpeq   t0, t1, t2              // check if address matches
        bne     t2, 70f                 // if ne, fault address match

//
// Memory management failed to resolve fault.
//
// STATUS_IN_PAGE_ERROR | 0x10000000 is a special status that indicates a
//      page fault at Irql greater than APC_LEVEL.
//
// The following statuses can be raised:
//
//      STATUS_ACCESS_VIOLATION
//      STATUS_GUARD_PAGE_VIOLATION
//      STATUS_STACK_OVERFLOW
//
// All other status will be set to:
//
//      STATUS_IN_PAGE_ERROR
//
// dispatch exception via common code in KiDispatchException
// Following must be done:
//      allocate exception frame via sp
//      complete data in ExceptionRecord
//      a0 points to ExceptionRecord
//      a1 points to ExceptionFrame
//      a2 points to TrapFrame
//      a3 = previous psr
//
// Exception record information has the following values
//      offset  value
//      0       read vs write indicator (set on entry)
//      4       bad virtual address (set on entry)
//      8       real status (only if status was not "recognized")
//
//
// Check for special status that indicates a page fault at
// Irql above APC_LEVEL.
//

        ldil    t1, STATUS_IN_PAGE_ERROR | 0x10000000 // get special status
        cmpeq   v0, t1, t2              // check if status is special case
        bne     t2, 60f                 // if ne, status is special case

//
// Check for expected status values.
//

        lda     a0, TrExceptionRecord(fp) // get exception record address
        bis     zero, 2, t0             // set number of parameters
        ldil    t1, STATUS_ACCESS_VIOLATION // get access violation code
        cmpeq   v0, t1, t2              // check if access violation
        bne     t2, 50f                 // if ne, access violation
        ldil    t1, STATUS_GUARD_PAGE_VIOLATION // get guard page violation code
        cmpeq   v0, t1, t2              // check if guard page violation
        bne     t2, 50f                 // if ne, guard page violation
        ldil    t1, STATUS_STACK_OVERFLOW // get stack overflow code
        cmpeq   v0, t1, t2              // check if stack overflow
        bne     t2, 50f                 // if ne, stack overflow

//
// Status is not recognized, save real status, bump the number
// of exception parameters, and set status to STATUS_IN_PAGE_ERROR
//

#if defined(_AXP64_)

        stq     v0, ErExceptionInformation + 16(a0) // save real status code

#else

        stl     v0, ErExceptionInformation + 8(a0) // save real status code

#endif

        bis     zero, 3, t0             // set number of params
        ldil    v0, STATUS_IN_PAGE_ERROR // set status to in page error

//
// Fill in the remaining exception record parameters and attempt to
// resolve the exception.
//

50:     ldl     a3, ErExceptionCode(a0) // get previous psr
        stl     v0, ErExceptionCode(a0) // save exception code
        stl     zero, ErExceptionFlags(a0) // set exception flags
        STP     zero, ErExceptionRecord(a0) // set associated record
        stl     t0, ErNumberParameters(a0) // set number of parameters
        br      ra, KiExceptionDispatch // dispatch exception

//
// Generate a bugcheck - A page fault has occured at an IRQL that is greater
// than APC_LEVEL.
//

60:     ldil    a0, IRQL_NOT_LESS_OR_EQUAL // set bugcheck code

#if defined(_AXP64_)

        ldq     a1, TrExceptionRecord + ErExceptionInformation + 8(fp) // set bad va

#else

        ldl     a1, TrExceptionRecord + ErExceptionInformation + 4(fp) // set bad va

#endif

        ldl     a2, TrExceptionRecord + ErExceptionCode(fp) // set previous IRQL
        srl     a2, PSR_IRQL, a2        //
        LDP     a3, TrExceptionRecord + ErExceptionInformation(fp) // set load/store indicator
        ldq     a4, TrFir(fp)           // set exception pc
        br      ra, KeBugCheckEx        // handle bugcheck

//
// The fault occured in the interlocked pop slist function and the faulting
// instruction should be skipped.
//

70:     lda     t0, ExpInterlockedPopEntrySListResume // get resumption address
        stq     t0, TrFir(fp)           // set continuation address
        ldl     a0, TrPsr(fp)           // get previous psr
        br      zero, KiAlternateExit   //

        .end     KiMemoryManagementDispatch

        SBTTL("Invalid Access Allowed")
//++
//
// BOOLEAN
// KeInvalidAccessAllowed (
//    IN PVOID TrapFrame
//    )
//
// Routine Description:
//
//    Mm will pass a pointer to a trap frame prior to issuing a bug check on
//    a pagefault. This routine lets Mm know if it is ok to bugcheck.  The
//    specific case we must protect are the interlocked pop sequences which can
//    blindly access memory that may have been freed and/or reused prior to the
//    access.  We don't want to bugcheck the system in these cases, so we check
//    the instruction pointer here.
//
// Arguments:
//
//    TrapFrame (a0) - Supplies a  trap frame pointer.  NULL means return False.
//
// Return Value:
//
//    True if the invalid access should be ignored.
//    False which will usually trigger a bugcheck.
//
//--

        LEAF_ENTRY(KeInvalidAccessAllowed)

        bis     zero, 0, v0             // assume access not allowed
        beq     a0, 10f                 // if eq, no trap frame specified
        ldq     t0, TrFir(a0)           // get faulting instruction address
        lda     t1, ExpInterlockedPopEntrySListFault // get address of pop code
        cmpeq   t0, t1, v0              // check if fault at pop code address
10:     ret     zero, (ra)              // return

        .end    KeInvalidAccessAllowed

        SBTTL("Primary Interrupt Dispatch")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//    N.B. The volatile registers must be saved in this prologue because
//         the compiler will occasionally generate code that uses volatile
//         registers to save the contents of nonvolatile registers when
//         a function only calls another function with a known register
//         signature (such as _OtsMove)
//
//--

        EXCEPTION_HANDLER(KiInterruptHandler)

        NESTED_ENTRY(KiInterruptDistribution, TrapFrameLength, zero);

        .set    noreorder
        stq     sp,TrIntSp(sp)          // save stack pointer
        stq     ra,TrIntRa(sp)          // save return address
        stq     ra,TrFir(sp)            // save return address
        stq     fp,TrIntFp(sp)          // save frame pointer
        stq     gp,TrIntGp(sp)          // save general pointer
        bis     sp, sp, fp              // set frame pointer
        .set    reorder

        stq     v0, TrIntV0(sp)         // save integer register v0
        stq     t0, TrIntT0(sp)         // save integer registers t0 - t7
        stq     t1, TrIntT1(sp)         //
        stq     t2, TrIntT2(sp)         //
        stq     t3, TrIntT3(sp)         //
        stq     t4, TrIntT4(sp)         //
        stq     t5, TrIntT5(sp)         //
        stq     t6, TrIntT6(sp)         //
        stq     t7, TrIntT7(sp)         //
        stq     a4, TrIntA4(sp)         // save integer registers a4 - a5
        stq     a5, TrIntA5(sp)         //
        stq     t8, TrIntT8(sp)         // save integer registers t8 - t12
        stq     t9, TrIntT9(sp)         //
        stq     t10, TrIntT10(sp)       //
        stq     t11, TrIntT11(sp)       //
        stq     t12, TrIntT12(sp)       //

        .set    noat
        stq     AT, TrIntAt(sp)         // save integer register AT
        .set    at

        PROLOGUE_END

//++
//
// Routine Description:
//
//     The PALcode dispatches to this routine when an enabled interrupt
//     is asserted.
//
//     When this routine is entered, interrupts are disabled.
//
//     The function of this routine is to determine the highest priority
//     pending interrupt, raise the IRQL to the level of the highest interrupt,
//     and then dispatch the interrupt to the proper service routine.
//
//
// Arguments:
//
//     a0 - Supplies the interrupt vector number.
//     a1 - Supplies the address of the pcr.
//     a3 - Supplies the previous psr.
//     fp - Supplies a pointer to the trap frame.
//     gp - Supplies a pointer to the system short data area.
//
// Return Value:
//
//     None.
//
//--

        ALTERNATE_ENTRY(KiInterruptException)

        bsr     ra, KiSaveVolatileIntegerState // save integer registers

//
// Count the number of interrupts.
//

        GET_PROCESSOR_CONTROL_BLOCK_BASE // get current prcb address

        ldl     t0, PbInterruptCount(v0) // get current count of interrupts
        addl    t0, 1, t1               // increment count
        stl     t1, PbInterruptCount(v0) // save new interrupt count

//
// If interrupt vector > DISPATCH_LEVEL, indicate interrupt active in PRCB
//

        cmpule  a0, DISPATCH_LEVEL, t4  // compare vector to DISPATCH_LEVEL
        LDP     t2, PbInterruptTrapFrame(v0)// get old interrupt trap frame
        cmovne  t4, zero, t2            // zero trap frame if <= DISPATCH_LEVEL
        STP     t2, TrTrapFrame(fp)     // save old interrupt trap in new
        bne     t4, 10f                 // vector <= DISPATCH_LEVEL, no interrupt trap
        STP     fp, PbInterruptTrapFrame(v0)// set new interrupt trap frame
10:     SPADDP  a0, a1, a0              // convert index to offset + PCR base
        LDP     a0, PcInterruptRoutine(a0) // get service routine address
        jsr     ra, (a0)                // call interrupt service routine

//
// Restore state and exit interrupt.
//

        ldl     a0, TrPsr(fp)           // get previous psr

#ifndef NT_UP

        DISABLE_INTERRUPTS              // disable interrupts

#endif

        GET_PROCESSOR_CONTROL_BLOCK_BASE // get current prcb address

        LDP     t0, TrTrapFrame(fp)     // get old interrupt trap frame
        STP     t0, PbInterruptTrapFrame(v0)// restore old interrupt trap frame

#ifndef NT_UP

        ENABLE_INTERRUPTS               // enable interrupts

#endif

        bne     t0, 50f                 // if ne, interrupt still active,

//
// If a dispatch interrupt is pending, lower IRQL to DISPATCH_LEVEL, and
// directly call the dispatch interrupt handler.
//

        ldl     t2, PbSoftwareInterrupts(v0) // get pending SW interrupts
        beq     t2, 50f                 // if eq, no pending SW interrupts
        stl     zero, PbSoftwareInterrupts(v0) // clear pending SW interrupts
        and     a0, PSR_IRQL_MASK, a1   // extract IRQL from PSR
        cmpult  a1, DISPATCH_LEVEL << PSR_IRQL, t3 // check return IRQL
        beq     t3, 70f                 // if not lt DISPATCH_LEVEL, can't bypass

//
// Update count of bypassed dispatch interrupts.
//

        ldl     t4, PbDpcBypassCount(v0) // get old bypass count
        addl    t4, 1, t5                // increment
        stl     t5, PbDpcBypassCount(v0) // store new bypass count
        ldil    a0, DISPATCH_LEVEL      // set new IRQL level

        SWAP_IRQL                       // lower IRQL to DISPATCH_LEVEL

        bsr     ra, KiDispatchInterrupt // process dispatch interrupt
45:     ldl     a0, TrPsr(fp)           // restore previous psr

//
// Check if an APC interrupt should be generated.
//

50:     bis     zero, zero, a1          // clear sfw interrupt request
        blbc    a0, 60f                 // if kernel no apc

        GET_CURRENT_THREAD              // get current thread address

        ldq_u   t1, ThApcState+AsUserApcPending(v0) // get user APC pending
        extbl   t1, (ThApcState+AsUserApcPending) % 8, t0 //
        ZeroByte(ThAlerted(v0))         // clear kernel mode alerted
        cmovne  t0, APC_INTERRUPT, a1   // if pending set APC interrupt
60:     bsr     ra, KiRestoreVolatileIntegerState // restore volatile state

//
// a0 = previous mode
// a1 = sfw interrupt requests
//

        RETURN_FROM_TRAP_OR_INTERRUPT   // return from trap/interrupt

//
// Previous IRQL is >= DISPATCH_LEVEL, so a pending software interrupt cannot
// be short-circuited. Request a software interrupt from the PAL.
//

70:     ldil    a0, DISPATCH_LEVEL      // set interrupt request level

        REQUEST_SOFTWARE_INTERRUPT      // request interrupt from PAL

        br      zero, 45b               // rejoin common code

        .end    KiInterruptDistribution

//++
//
// EXCEPTION_DISPOSITION
// KiInterruptHandler (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN ULONG EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PDISPATCHER_CONTEXT DispatcherContext
//
// Routine Description:
//
//    Control reaches here when an exception is not handled by an interrupt
//    service routine or an unwind is initiated in an interrupt service
//    routine that would result in an unwind through the interrupt dispatcher.
//    This is considered to be a fatal system error and bug check is called.
//
// Arguments:
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (a1) - Supplies the frame pointer of the establisher
//       of this exception handler.
//
//       N.B. This is not actually the frame pointer of the establisher of
//            this handler. It is actually the stack pointer of the caller
//            of the system service. Therefore, the establisher frame pointer
//            is not used and the address of the trap frame is determined by
//            examining the saved fp register in the context record.
//
//    ContextRecord (a2) - Supplies a pointer to a context record.
//
//    DispatcherContext (a3) - Supplies a pointer to  the dispatcher context
//       record.
//
// Return Value:
//
//    There is no return from this routine.
//
//--

        NESTED_ENTRY(KiInterruptHandler, HandlerFrameLength, zero)

        lda     sp, -HandlerFrameLength(sp) // allocate stack frame
        stq     ra, HdRa(sp)            // save return address

        PROLOGUE_END

        ldl     t0, ErExceptionFlags(a0) // get exception flags
        ldil    a0, INTERRUPT_UNWIND_ATTEMPTED // assume unwind in progress
        and     t0, EXCEPTION_UNWIND, t1 // check if unwind in progress
        bne     t1, 10f                  // if ne, unwind in progress
        ldil    a0, INTERRUPT_EXCEPTION_NOT_HANDLED // set bug check code
10:     bsr     ra, KeBugCheck           // call bug check routine

        .end    KiInterruptHandler

        SBTTL("System Service Dispatch")
//++
//
// Routine Description:
//
//    The following code is never executed. Its purpose is to allow the
//    kernel debugger to walk call frames backwards through an exception,
//    to support unwinding through exceptions for system services, and to
//    support get/set user context.
//
//--
        .struct 0
ScCurrentThread:                        // current thread address
        .space  8                       //
ScServiceRoutine:                       // service routine address
        .space  8                       //
ScServiceDescriptor:                    // service descriptor address
        .space  8                       //
ScServiceNumber:                        // service number
        .space  4                       //
        .space  4                       // fill
SyscallFrameLength:                     // frame length

        EXCEPTION_HANDLER(KiSystemServiceHandler)

        NESTED_ENTRY(KiSystemServiceDispatch, TrapFrameLength, zero);

        .set    noreorder
        stq     sp, TrIntSp - TrapFrameLength(sp) // save stack pointer
        lda     sp, -TrapFrameLength(sp) // allocate stack frame
        stq     ra,TrIntRa(sp)          // save return address
        stq     ra,TrFir(sp)            // save return address
        stq     fp,TrIntFp(sp)          // save frame pointer
        stq     gp,TrIntGp(sp)          // save general pointer
        bis     sp, sp, fp              // set frame pointer
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//    Control reaches here when we have a system call call pal executed.
//    When this routine is entered, interrupts are disabled.
//
//    The function of this routine is to call the specified system service.
//
//
// Arguments:
//
//    v0 - Supplies the system service code.
//    t0 - Previous processor mode
//    t1 - Current thread address
//    gp - Supplies a pointer to the system short data area.
//    fp - Supplies a pointer to the trap frame.
//
// Return Value:
//
//    None.
//
//--

        ALTERNATE_ENTRY(KiSystemServiceException)

        START_REGION(KiSystemServiceDispatchStart)

        mf_fpcr f0                      // save floating control register
        stt     f0, TrFpcr(fp)          //
        lda     sp, -SyscallFrameLength(sp) // allocate stack frame
        STP     t1, ScCurrentThread(sp) // save current thread address
        ldq_u   t4, ThPreviousMode(t1)  // get old previous thread mode
        LDP     t5, ThTrapFrame(t1)     // get current trap frame address
        extbl   t4, ThPreviousMode % 8, t3 // extract previous mode
        stl     t3, TrPreviousMode(fp)  // save old previous mode of thread
        StoreByte(t0, ThPreviousMode(t1)) // set new previous mode in thread
        STP     t5, TrTrapFrame(fp)     // save current trap frame address

//
// If the specified system service number is not within range, then
// attempt to convert the thread to a GUI thread and retry the service
// dispatch.
//
// N.B. The argument registers a0-a3, the system service number in v0,
//      and the thread address in t1 must be preserved while attempting
//      to convert the thread to a GUI thread.
//

        ALTERNATE_ENTRY(KiSystemServiceRepeat)

        STP     fp, ThTrapFrame(t1)     // save address of trap frame
        LDP     t10, ThServiceTable(t1) // get service descriptor table address
        srl     v0, SERVICE_TABLE_SHIFT, t2 // isolate service descriptor offset
        and     t2, SERVICE_TABLE_MASK, t2 //
        ADDP    t2, t10, t10            // compute service descriptor address
        ldl     t3, SdLimit(t10)        // get service number limit
        and     v0, SERVICE_NUMBER_MASK, t7 // isolate service table offset
        cmpult  t7, t3, t4              // check if valid service number
        beq     t4, 80f                 // if eq, not valid service number
        LDP     t4, SdBase(t10)         // get service table address
        SPADDP  t7, t4, t3              // compute address in service table
        LDP     t5, 0(t3)               // get address of service routine

#if DBG

        LDP     t6, SdCount(t10)        // get service count table address
        beq     t6, 5f                  // if eq, table not defined
        S4ADDP  t7, t6, t6              // compute system service offset value
        ldl     t11, 0(t6)              // increment system service count
        addl    t11, 1, t11             //
        stl     t11, 0(t6)              // store result

#endif

//
// If the system service is a GUI service and the GDI user batch queue is
// not empty, then call the appropriate service to flush the user batch.
//

5:      cmpeq   t2, SERVICE_TABLE_TEST, t2 // check if GUI system service
        beq     t2, 15f                 // if eq, not GUI system service
        LDP     t3, ThTeb(t1)           // get current thread TEB address
        ldl     t4, TeGdiBatchCount(t3) // get number of batched GDI calls
        beq     t4, 15f                 // if eq, no batched calls
        STP     t5, ScServiceRoutine(sp) // save service routine address
        STP     t10, ScServiceDescriptor(sp)// save service descriptor address
        stl     t7, ScServiceNumber(sp)  // save service table offset
        LDP     t5, KeGdiFlushUserBatch // get address of flush routine
        stq     a0, TrIntA0(fp)         // save possible arguments
        stq     a1, TrIntA1(fp)         //
        stq     a2, TrIntA2(fp)         //
        stq     a3, TrIntA3(fp)         //
        stq     a4, TrIntA4(fp)         //
        stq     a5, TrIntA5(fp)         //
        jsr     ra, (t5)                // flush GDI user batch
        ldq     a0, TrIntA0(fp)         // restore possible arguments
        ldq     a1, TrIntA1(fp)         //
        ldq     a2, TrIntA2(fp)         //
        ldq     a3, TrIntA3(fp)         //
        ldq     a4, TrIntA4(fp)         //
        ldq     a5, TrIntA5(fp)         //
        LDP     t5, ScServiceRoutine(sp) // restore service routine address
        LDP     t10, ScServiceDescriptor(sp) // restore service descriptor address
        ldl     t7, ScServiceNumber(sp) // restore service table offset

//
// Check if system service has any in memory arguments.
//

15:     blbc    t5, 30f                 // if clear, no in memory arguments
        LDP     t10, SdNumber(t10)      // get argument table address
        ADDP    t7, t10, t11            // compute address in argument table

//
// The following code captures arguments that were passed in memory on the
// callers stack. This is necessary to ensure that the caller does not modify
// the arguments after they have been probed and is also necessary in kernel
// mode because a trap frame has been allocated on the stack.
//
// If the previous mode is user, then the user stack is probed for readability.
//

        LDP     t10, TrIntSp(fp)        // get previous stack pointer
        beq     t0, 10f                 // if eq, previous mode was kernel
        LDIP    t2, MM_USER_PROBE_ADDRESS // get user probe address
        cmpult  t10, t2, t4             // check if stack in user region
        cmoveq  t4, t2, t10             // if eq, set invalid user stack address
10:     ldq_u   t4, 0(t11)              // get number of memory arguments * 8
        extbl   t4, t11, t9             //
        addl    t9, 0x1f, t3            // round up to hexaword (32 bytes)
        bic     t3, 0x1f, t3            // ensure hexaword alignment
        SUBP    sp, t3, sp              // allocate space on kernel stack
        bis     sp, zero, t2            // set destination copy address
        ADDP    t2, t3, t4              // compute destination end address

        START_REGION(KiSystemServiceStartAddress)

//
// This code is set up to load the cache block in the first
// instruction and then perform computations that do not require
// the cache while waiting for the data.  In addition, the stores
// are setup so they will be in order.
//

20:     ldq     t6, 24(t10)             // get argument from previous stack
        ADDP    t10, 32, t10            // next hexaword on previous stack
        ADDP    t2, 32, t2              // next hexaword on kernel stack
        cmpeq   t2, t4, t11             // at end address?
        stq     t6, -8(t2)              // store argument on kernel stack
        ldq     t7, -16(t10)            // argument from previous stack
        ldq     t8, -24(t10)            // argument from previous stack
        ldq     t9, -32(t10)            // argument from previous stack
        stq     t7, -16(t2)             // save argument on kernel stack
        stq     t8, -24(t2)             // save argument on kernel stack
        stq     t9, -32(t2)             // save argument on kernel stack
        beq     t11, 20b                // if eq, get next block

        END_REGION(KiSystemServiceEndAddress)

        bic     t5, 3, t5               // clear lower bits of service addr

//
// Call system service.
//

30:     jsr     ra, (t5)

//
// Restore old trap frame address from the current trap frame and update
// the number of system calls.
//

        ALTERNATE_ENTRY(KiSystemServiceExit)

        bis     v0, zero, t1            // save return status

        GET_PROCESSOR_CONTROL_BLOCK_BASE // get processor block address

        LDP     t2, -SyscallFrameLength + ScCurrentThread(fp) // get current thread address
        LDP     t3, TrTrapFrame(fp)     // get old trap frame address
        ldl     t10, PbSystemCalls(v0)  // increment number of calls
        addl    t10, 1, t10             //
        stl     t10, PbSystemCalls(v0)  // store result
        STP     t3, ThTrapFrame(t2)     // restore old trap frame address
        bis     t1, zero, v0            // restore return status
        ldt     f0, TrFpcr(fp)          // restore floating control register
        mt_fpcr f0                      //
        ldl     a0, TrPsr(fp)           // get previous processor status
        ldl     t5, TrPreviousMode(fp)  // get old previous mode
        StoreByte(t5, ThPreviousMode(t2)) // store previous mode in thread

//
// Check if an APC interrupt should be generated.
//

        bis     zero, zero, a1          // clear siftware interrupt request
        blbc    a0, 70f                 // if kernel mode skip apc check
        ldq_u   t1, ThApcState+AsUserApcPending(t2) // get user APC pending
        extbl   t1, (ThApcState+AsUserApcPending) % 8, t0 //
        ZeroByte(ThAlerted(t2))         // clear kernel mode alerted
        cmovne  t0, APC_INTERRUPT, a1   // if pending set APC interrupt

//
// a0 = previous psr
// a1 = sfw interrupt requests
//

70:     RETURN_FROM_SYSTEM_CALL         // return to caller

//
// The specified system service number is not within range. Attempt to
// convert the thread to a GUI thread if specified system service is a
// GUI service.
//
// N.B. The argument register a0-a5 and the system service number in v0
//      must be preserved if an attempt is made to convert the thread to
//      a GUI thread.
//

80:     cmpeq   t2, SERVICE_TABLE_TEST, t2 // check if GUI system service
        beq     t2, 55f                 // if eq, not GUI system service
        stl     v0, ScServiceNumber(sp) // save system service number
        stq     a0, TrIntA0(fp)         // save argument registers a0-a5
        stq     a1, TrIntA1(fp)         //
        stq     a2, TrIntA2(fp)         //
        stq     a3, TrIntA3(fp)         //
        stq     a4, TrIntA4(fp)         //
        stq     a5, TrIntA5(fp)         //
        bsr     ra, PsConvertToGuiThread // attempt to convert to GUI thread
        bis     v0, zero, t0            // save completion status
        lda     fp, SyscallFrameLength(sp) // restore trap frame address

        GET_CURRENT_THREAD              // restore current thread address

        bis     v0, zero, t1            // set current thread address
        ldl     v0, ScServiceNumber(sp) // restore system service number
        ldq     a0, TrIntA0(fp)         // restore argument registers a0-a5
        ldq     a1, TrIntA1(fp)         //
        ldq     a2, TrIntA2(fp)         //
        ldq     a3, TrIntA3(fp)         //
        ldq     a4, TrIntA4(fp)         //
        ldq     a5, TrIntA5(fp)         //
        beq     t0, KiSystemServiceRepeat // if eq, successful conversion

//
// The conversion to a Gui thread failed. The correct return value is encoded
// in a byte table indexed by the service number that is at the end of the
// service address table. The encoding is as follows:
//
//     0 - return 0.
//    -1 - return -1.
//     1 - return status code.
//

        lda     t2, KeServiceDescriptorTableShadow // get descriptor base address
        ldl     t3, SERVICE_TABLE_TEST + SdLimit(t2) // get service number limit
        LDP     t4, SERVICE_TABLE_TEST + SdBase(t2) // get service table address
        SPADDP  t3, t4, t2              // compute ending service table address
        and     v0, SERVICE_NUMBER_MASK, t3 // isolate service number
        ADDP    t2, t3, t3              // compute return value address
        ldq_u   t2, 0(t3)               // get packed status bytes
        extbl   t2, t3, t2              // extract encoded status byte
        sll     t2, 7 * 8, t2           // sign extend status byte value
        sra     t2, 7 * 8, v0           //
        ble     v0, KiSystemServiceExit // if le, return value set

//
// Return invalid system service status for invalid service code.
//

55:     ldil    v0, STATUS_INVALID_SYSTEM_SERVICE // set completion status
        br      zero, KiSystemServiceExit //

        END_REGION(KiSystemServiceDispatchEnd)

        .end    KiSystemServiceDispatch

//++
//
// EXCEPTION_DISPOSITION
// KiSystemServiceHandler (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN ULONG EstablisherFrame,
//    IN OUT PCONTEXT ContextRecord,
//    IN OUT PDISPATCHER_CONTEXT DispatcherContext
//    )
//
// Routine Description:
//
//    Control reaches here when a exception is raised in a system service
//    or the system service dispatcher, and for an unwind during a kernel
//    exception.
//
//    If an unwind is being performed and the system service dispatcher is
//    the target of the unwind, then an exception occured while attempting
//    to copy the user's in-memory argument list. Control is transfered to
//    the system service exit by return a continue execution disposition
//    value.
//
//    If an unwind is being performed and the previous mode is user, then
//    bug check is called to crash the system. It is not valid to unwind
//    out of a system service into user mode.
//
//    If an unwind is being performed, the previous mode is kernel, the
//    system service dispatcher is not the target of the unwind, and the
//    thread does not own any mutexes, then the previous mode field from
//    the trap frame is restored to the thread object. Otherwise, bug
//    check is called to crash the system. It is invalid to unwind out of
//    a system service while owning a mutex.
//
//    If an exception is being raised and the exception PC is within the
//    range of the system service dispatcher in-memory argument copy code,
//    then an unwind to the system service exit code is initiated.
//
//    If an exception is being raised and the exception PC is not within
//    the range of the system service dispatcher, and the previous mode is
//    not user, then a continue searh disposition value is returned. Otherwise,
//    a system service has failed to handle an exception and bug check is
//    called. It is invalid for a system service not to handle all exceptions
//    that can be raised in the service.
//
// Arguments:
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    EstablisherFrame (a1) - Supplies the frame pointer of the establisher
//       of this exception handler.
//
//       N.B. This is not actually the frame pointer of the establisher of
//            this handler. It is actually the stack pointer of the caller
//            of the system service. Therefore, the establisher frame pointer
//            is not used and the address of the trap frame is determined by
//            examining the saved fp register in the context record.
//
//    ContextRecord (a2) - Supplies a pointer to a context record.
//
//    DispatcherContext (a3) - Supplies a pointer to  the dispatcher context
//       record.
//
// Return Value:
//
//    If bug check is called, there is no return from this routine and the
//    system is crashed. If an exception occured while attempting to copy
//    the user in-memory argument list, then there is no return from this
//    routine, and unwind is called. Otherwise, ExceptionContinueSearch is
//    returned as the function value.
//
//--

        LEAF_ENTRY(KiSystemServiceHandler)

        lda     sp, -HandlerFrameLength(sp) // allocate stack frame
        stq     ra, HdRa(sp)            // save return address

        PROLOGUE_END

        ldl     t0, ErExceptionFlags(a0) // get exception flags
        and     t0, EXCEPTION_UNWIND, t1 // check if unwind in progress
        bne     t1, 40f                  // if ne, unwind in progress

//
// An exception is in progress.
//
// If the exception PC is within the in-memory argument copy code of the
// system service dispatcher, then call unwind to transfer control to the
// system service exit code. Otherwise, check if the previous mode is user
// or kernel mode.
//
//

        LDP     t0, ErExceptionAddress(a0) // get address of exception
        lda     t1, KiSystemServiceStartAddress // address of system service
        cmpult  t0, t1, t3                 // check if before start range
        lda     t2, KiSystemServiceEndAddress // end address
        bne     t3, 10f                 // if ne, before start of range
        cmpult  t0, t2, t3              // check if before end of range
        bne     t3, 30f                 // if ne, before end of range

//
// If the previous mode was kernel mode, then a continue search disposition
// value is returned. Otherwise, the exception was raised in a system service
// and was not handled by that service. Call bug check to crash the system.
//

10:     GET_CURRENT_THREAD              // get current thread address

        ldq_u   t4, ThPreviousMode(v0)  // get previous mode from thread
        extbl   t4, ThPreviousMode % 8, t1 //
        bne     t1, 20f                 // if ne, previous mode was user

//
// Previous mode is kernel mode.
//

        ldil    v0, ExceptionContinueSearch // set disposition code
        lda     sp, HandlerFrameLength(sp) // deallocate stack frame
        jmp     zero, (ra)              // return

//
// Previous mode is user mode. Call bug check to crash the system.
//

20:     bis     a3, zero, a4            // set dispatcher context address
        bis     a2, zero, a3            // set context record address
        bis     a1, zero, a2            // set system service frame address
        bis     a0, zero, a1            // set exception record address
        ldil    a0, SYSTEM_SERVICE_EXCEPTION // set bug check code
        bsr     ra, KeBugCheckEx        // call bug check routine

//
// The exception was raised in the system service dispatcher. Unwind to the
// the system service exit code.
//

30:     ldl     a3, ErExceptionCode(a0) // set return value
        bis     zero, zero, a2          // set exception record address
        bis     a1, zero, a0            // set target frame address
        lda     a1, KiSystemServiceExit // set target PC address
        bsr     ra, RtlUnwind           // unwind to system service exit

//
// An unwind is in progress.
//
// If a target unwind is being performed, then continue execution is returned
// to transfer control to the system service exit code. Otherwise, restore the
// previous mode if the previous mode is not user and there are no mutexes owned
// by the current thread.
//

40:     and     t0, EXCEPTION_TARGET_UNWIND, t1 // check if target unwnd in progres
        bne     t1, 60f                 // if ne, target unwind in progress

//
// An unwind is being performed through the system service dispatcher. If the
// previous mode is not kernel or the current thread owns one or more mutexes,
// then call bug check and crash the system. Otherwise, restore the previous
// mode in the current thread object.
//

        GET_CURRENT_THREAD              // get current thread address

        ldl     t1, CxIntFp(a2)         // get address of trap frame
        ldq_u   t4, ThPreviousMode(v0)  // get previous mode from thread
        extbl   t4, ThPreviousMode % 8, t3 //
        ldl     t4,TrPreviousMode(t1)   // get previous mode from trap frame
        bne     t3, 50f                 // if ne, previous mode was user

//
// Restore previous from trap frame to thread object and continue the unwind
// operation.
//

        StoreByte(t4, ThPreviousMode(v0)) // restore previous mode from trap frame
        ldil    v0, ExceptionContinueSearch // set disposition value
        lda     sp, HandlerFrameLength(sp) // deallocate stack frame
        jmp     zero, (ra)              // return

//
// An attempt is being made to unwind into user mode. Call bug check to crash
// the system.
//

50:     ldil    a0, SYSTEM_UNWIND_PREVIOUS_USER // set bug check code
        bsr     ra, KeBugCheck          // call bug check

//
// A target unwind is being performed. Return a continue search disposition
// value.
//

60:     ldil    v0, ExceptionContinueSearch // set disposition value
        lda     sp, HandlerFrameLength(sp) // deallocate stack frame
        jmp      zero, (ra)             // return

        .end    KiSystemServiceHandler

//++
//
// Routine Description:
//
//     The following code is never executed.  Its purpose is to allow the
//     kernel debugger to walk call frames backwards through an exception
//     to support unwinding through exceptions for system services, and to
//     support get/set user context.
//--

        NESTED_ENTRY(KiPanicDispatch, TrapFrameLength, zero)

        .set    noreorder
        stq     sp, TrIntSp(sp)         // save stack pointer
        stq     ra, TrIntRa(sp)         // save return address
        stq     ra, TrFir(sp)           // save return address
        stq     fp, TrIntFp(sp)         // save frame pointer
        stq     gp, TrIntGp(sp)         // save global pointer
        bis     sp, sp, fp              // set frame pointer
        .set    reorder

        PROLOGUE_END

//++
//
// Routine Description:
//
//     PALcode dispatches to this entry point when a panic situation
//     is detected while in PAL mode.  The panic situation may be that
//     the kernel stack is about to overflow/underflow or there may be
//     a condition that was not expected to occur while in PAL mode
//     (eg. arithmetic exception while in PAL).  This entry point is
//     here to help us debug the condition.
//
// Arguments:
//
//      fp - points to trap frame
//      sp - points to exception frame
//      a0 = Bug check code
//      a1 = Exception address
//      a2 = Bugcheck parameter
//      a3 = Bugcheck parameter
//
//      gp, ra - saved in trap frame
//      a0-a3 - saved in trap frame
//
// Return Value:
//
//      None.
//
//--

        ALTERNATE_ENTRY(KiPanicException)

        stq     ra, TrIntRa(fp)         // PAL is supposed to do this, but it doesn't!

//
// Save state, volatile float and integer state via KiGenerateTrapFrame
//

        bsr     ra, KiGenerateTrapFrame // save volatile state

//
// Dispatch to KeBugCheckEx, does not return
//

        br      ra, KeBugCheckEx        // do the bugcheck

        .end    KiPanicDispatch

//++
//
// VOID
// KiBreakinBreakpoint(
//     VOID
//     );
//
// Routine Description:
//
//     This routine issues a breakin breakpoint.
//
// Arguments:
//
//      None.
//
// Return Value:
//
//      None.
//
//--

        LEAF_ENTRY( KiBreakinBreakpoint )

        BREAK_BREAKIN                   // execute breakin breakpoint

        ret     zero, (ra)              // return to caller

        .end    KiBreakinBreakpoint
