//      TITLE("Call Out to User Mode")
//++
//
// Copyright (c) 1994  Microsoft Corporation
//
// Module Name:
//
//     callout.s
//
// Abstract:
//
//    This module implements the code necessary to call out from kernel
//    mode to user mode.
//
// Author:
//
//     John Vert (jvert) 2-Nov-1994
//
// Environment:
//
//     Kernel mode only.
//
// Revision History:
//
//--

#include "ksalpha.h"

//
// Define external variables that can be addressed using GP.
//

        .extern KeUserCallbackDispatcher

        SBTTL("Call User Mode Function")
//++
//
// NTSTATUS
// KiCallUserMode (
//    IN PVOID *OutputBuffer,
//    IN PULONG OutputLength
//    )
//
// Routine Description:
//
//    This function calls a user mode function.
//
//    N.B. This function calls out to user mode and the NtCallbackReturn
//        function returns back to the caller of this function. Therefore,
//        the stack layout must be consistent between the two routines.
//
// Arguments:
//
//    OutputBuffer (a0) - Supplies a pointer to the variable that receivies
//        the address of the output buffer.
//
//    OutputLength (a1) - Supplies a pointer to a variable that receives
//        the length of the output buffer.
//
// Return Value:
//
//    The final status of the call out function is returned as the status
//    of the function.
//
//    N.B. This function does not return to its caller. A return to the
//        caller is executed when a NtCallbackReturn system service is
//        executed.
//
//    N.B. This function does return to its caller if a kernel stack
//         expansion is required and the attempted expansion fails.
//
//--

        NESTED_ENTRY(KiCallUserMode, CuFrameLength, zero)

        lda     sp, -CuFrameLength(sp)  // allocate stack frame
        stq     ra, CuRa(sp)            // save return address

//
// Save nonvolatile integer registers.
//

        stq     s0, CuS0(sp)            // save integer registers s0 - s5
        stq     s1, CuS1(sp)            //
        stq     s2, CuS2(sp)            //
        stq     s3, CuS3(sp)            //
        stq     s4, CuS4(sp)            //
        stq     s5, CuS5(sp)            //
        stq     fp, CuFP(sp)            // save FP

//
// Save nonvolatile floating registers.
//

        stt     f2, CuF2(sp)            // save floating registers f2 - f9
        stt     f3, CuF3(sp)            //
        stt     f4, CuF4(sp)            //
        stt     f5, CuF5(sp)            //
        stt     f6, CuF6(sp)            //
        stt     f7, CuF7(sp)            //
        stt     f8, CuF8(sp)            //
        stt     f9, CuF9(sp)            //

        PROLOGUE_END

//
// Save argument registers.
//

        stq     a0, CuA0(sp)            // save output buffer address
        stq     a1, CuA1(sp)            // save output length address

//
// Check if sufficient room is available on the kernel stack for another
// system call.
//

        GET_CURRENT_THREAD              // get current thread address

        bis     v0, zero, t0            // save current thread address
        LDP     t1, ThInitialStack(t0)  // get initial stack address
        LDP     t2, ThStackLimit(t0)    // get current stack limit
        SUBP    sp, KERNEL_LARGE_STACK_COMMIT, t3 // compute bottom address
        cmpult  t2, t3, t4              // check if limit exceeded
        bne     t4, 10f                 // if ne, limit not exceeded
        bis     sp, zero, a0            // set current kernel stack address
        bsr     ra, MmGrowKernelStack   // attempt to grow the kernel stack
        bne     v0, 20f                 // if ne, attempt to grow failed

        GET_CURRENT_THREAD              // get current thread address

        bis     v0, zero, t0            // save current thread address
        LDP     t1, ThInitialStack(t0)  // get initial stack address
10:     LDP     fp, ThTrapFrame(t0)     // get trap frame address
        LDP     t2, ThCallbackStack(t0) // get callback stack address
        STP     t1, CuInStk(sp)         // save initial stack address
        STP     fp, CuTrFr(sp)          // save trap frame address
        STP     t2, CuCbStk(sp)         // save callback stack address
        STP     sp, ThCallbackStack(t0) // set callback stack address

//
// Restore state and callback to user mode.
//
// N.B. Interrupts are disabled to prevent get/set context APCs from
//      occurring.
//

        DISABLE_INTERRUPTS              // disable interrupts

        STP     sp, ThInitialStack(t0)  // reset initial stack address
        ldq     t3, TrFir(fp)           // get old PC
        STP     t3, CuTrFir(sp)         // save old PC
        LDP     t4, KeUserCallbackDispatcher // get continuation address
        stq     t4, TrFir(fp)           // set continuation address

//
// If a user mode APC is pending, then request an APC interrupt.
//

        bis     zero, zero, a1          // assume no user APC pending
        ldq_u   t1, ThApcState+AsUserApcPending(t0) // get user APC pending
        extbl   t1, (ThApcState+AsUserApcPending) % 8, t1 //
        ZeroByte(ThAlerted(t0))         // clear kernel mode alerted
        cmovne  t1, APC_INTERRUPT, a1   // if pending set APC interrupt

//
// Set initial kernel stack for this thread.
//

        bis     sp, zero, a0            // set kernel stack address

        SET_INITIAL_KERNEL_STACK        // set kernel stack pointer

        ldl     a0, TrPsr(fp)           // get previous processor status

//
// a0 = previous psr
// a1 = sfw interrupt requests
//

        RETURN_FROM_SYSTEM_CALL         // return to user mode

        ret     zero, (ra)              //

//
// An attempt to grow the kernel stack failed.
//

20:     ldq     ra, CuRa(sp)            // restore return address
        lda     sp, CuFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)

        .end KiCallUserMode

        SBTTL("Switch Kernel Stack")
//++
//
// PVOID
// KeSwitchKernelStack (
//    IN PVOID StackBase,
//    IN PVOID StackLimit
//    )
//
// Routine Description:
//
//    This function switches to the specified large kernel stack.
//
//    N.B. This function can ONLY be called when there are no variables
//        in the stack that refer to other variables in the stack, i.e.,
//        there are no pointers into the stack.
//
// Arguments:
//
//    StackBase (a0) - Supplies a pointer to the base of the new kernel
//        stack.
//
//    StackLimit (a1) - supplies a pointer to the limit of the new kernel
//        stack.
//
// Return Value:
//
//    The old kernel stack is returned as the function value.
//
//--

        .struct 0
SsRa:   .space  8                       // saved return address
SsSp:   .space  8                       // saved new stack pointer
SsA0:   .space  8                       // saved argument registers a0-a1
SsA1:   .space  8                       //
SsFrameLength:                          // length of stack frame

        NESTED_ENTRY(KeSwitchKernelStack, SsFrameLength, zero)

        lda     sp, -SsFrameLength(sp)  // allocate stack frame
        stq     ra, SsRa(sp)            // save return address

        PROLOGUE_END

//
// Save the address of the new stack and copy the old stack to the new
// stack.
//

        GET_CURRENT_THREAD                  // get current thread address

        stq     a0, SsA0(sp)                // save new stack base address
        stq     a1, SsA1(sp)                // save new stack limit address
        LDP     a2, ThStackBase(v0)         // get current stack base address
        LDP     a3, ThTrapFrame(v0)         // get current trap frame address
        ADDP    a3, a0, a3                  // relocate trap frame address
        SUBP    a3, a2, a3                  //
        STP     a3, ThTrapFrame(v0)         // set current trap frame address
        bis     sp, zero, a1                // set source address of copy
        SUBP    a2, sp, a2                  // compute length of copy
        SUBP    a0, a2, a0                  // set destination address of copy
        stq     a0, SsSp(sp)                // save new stack pointer address
        bsr     ra, RtlMoveMemory           // copy old stack to new stack

//
// Switch to new kernel stack and return the address of the old kernel stack.
//

        GET_CURRENT_THREAD                  // get current thread address

        DISABLE_INTERRUPTS                  // disable interrupts

        LDP     t0, ThStackBase(v0)         // get old stack base address
        ldq     a0, SsA0(sp)                // get new stack base address
        ldq     a1, SsA1(sp)                // get new stack limit address
        STP     a0, ThInitialStack(v0)      // set new initial stack address
        STP     a0, ThStackBase(v0)         // set new stack base address
        STP     a1, ThStackLimit(v0)        // set new stack limit address
        ldil    t1, TRUE                    // set large kernel stack TRUE
        StoreByte(t1, ThLargeStack(v0))     //
        ldq     sp, SsSp(sp)                // set initial stack address

        SET_INITIAL_KERNEL_STACK            // set initial kernel stack address

        ENABLE_INTERRUPTS                   // enable interrupts

        ldq     ra, SsRa(sp)                // restore return address
        lda     sp, SsFrameLength(sp)       // deallocate stack frame
        ret     zero, (ra)                  // return

        .end    KeSwitchKernelStack

        SBTTL("Return from User Mode Callback")
//++
//
// NTSTATUS
// NtCallbackReturn (
//    IN PVOID OutputBuffer OPTIONAL,
//    IN ULONG OutputLength,
//    IN NTSTATUS Status
//    )
//
// Routine Description:
//
//    This function returns from a user mode callout to the kernel
//    mode caller of the user mode callback function.
//
//    N.B. This function returns to the function that called out to user
//        mode and the KiCallUserMode function calls out to user mode.
//        Therefore, the stack layout must be consistent between the
//        two routines.
//
// Arguments:
//
//    OutputBuffer - Supplies an optional pointer to an output buffer.
//
//    OutputLength - Supplies the length of the output buffer.
//
//    Status - Supplies the status value returned to the caller of the
//        callback function.
//
// Return Value:
//
//    If the callback return cannot be executed, then an error status is
//    returned. Otherwise, the specified callback status is returned to
//    the caller of the callback function.
//
//    N.B. This function returns to the function that called out to user
//         mode is a callout is currently active.
//
//--

        LEAF_ENTRY(NtCallbackReturn)

        GET_CURRENT_THREAD              // get current thread address

        LDP     t1, ThCallbackStack(v0) // get callback stack address
        beq     t1, 10f                 // if eq, no callback stack present

//
// Restore nonvolatile integer registers.
//

        ldq     s0, CuS0(t1)            // restore integer registers s0 - s5
        ldq     s1, CuS1(t1)            //
        ldq     s2, CuS2(t1)            //
        ldq     s3, CuS3(t1)            //
        ldq     s4, CuS4(t1)            //
        ldq     s5, CuS5(t1)            //
        ldq     fp, CuFP(t1)            // restore FP

//
// Restore nonvolatile floating registers.
//

        ldt     f2, CuF2(t1)            // restore floating registers f2 - f9
        ldt     f3, CuF3(t1)            //
        ldt     f4, CuF4(t1)            //
        ldt     f5, CuF5(t1)            //
        ldt     f6, CuF6(t1)            //
        ldt     f7, CuF7(t1)            //
        ldt     f8, CuF8(t1)            //
        ldt     f9, CuF9(t1)            //

//
// Restore the trap frame and callback stack addresses, and store the output
// buffer address and length.
//

        LDP     t2, CuTrFr(t1)          // get previous trap frame address
        LDP     t3, CuCbStk(t1)         // get previous callback stack address
        LDP     t4, CuA0(t1)            // get address to store output address
        LDP     t5, CuA1(t1)            // get address to store output length
        LDP     t6, CuTrFir(t1)         // get old trap frame PC
        STP     t2, ThTrapFrame(v0)     // restore trap frame address
        STP     t3, ThCallbackStack(v0) // restore callback stack address
        STP     a0, 0(t4)               // store output buffer address
        STP     a1, 0(t5)               // store output buffer length
        stq     t6, TrFir(t2)           // restore old trap frame PC

//
// **** this is the place where the current stack could be trimmed back.
//
// Restore initial stack pointer, trim stackback to callback frame,
// deallocate callback stack frame, and return to callback caller.
//

        LDP     a0, CuInStk(t1)         // get previous initial stack
        STP     a0, ThInitialStack(v0)  //

        SET_INITIAL_KERNEL_STACK        // set initial kernel stack address

        bis     t1, zero, sp            // trim stack callback frame
        bis     a2, zero, v0            // set callback service status
        ldq     ra, CuRa(sp)            // restore return address
        lda     sp, CuFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

//
// No callback is currently active.
//

10:     ldil    v0, STATUS_NO_CALLBACK_ACTIVE // set service status
        ret     zero, (ra)              // return

        .end NtCallbackReturn
