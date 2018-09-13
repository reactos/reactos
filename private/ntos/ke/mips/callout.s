//      TITLE("Call Out to User Mode")
//++
//
// Copyright (c) 1994  Microsoft Corporation
//
// Module Name:
//
//    callout.s
//
// Abstract:
//
//    This module implements the code necessary to call out from kernel
//    mode to user mode.
//
// Author:
//
//    David N. Cutler (davec) 29-Oct-1994
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksmips.h"

//
// Define external variables that can be addressed using GP.
//

        .extern KeUserCallbackDispatcher 4

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

        subu    sp,sp,CuFrameLength     // allocate stack frame
        sw      ra,CuRa(sp)             // save return address

//
// Save nonvolatile integer registers.
//

        sw      s0,CuS0(sp)             // save integer registers s0-s8
        sw      s1,CuS1(sp)             //
        sw      s2,CuS2(sp)             //
        sw      s3,CuS3(sp)             //
        sw      s4,CuS4(sp)             //
        sw      s5,CuS5(sp)             //
        sw      s6,CuS6(sp)             //
        sw      s7,CuS7(sp)             //
        sw      s8,CuS8(sp)             //

//
// Save nonvolatile floating registers.
//

        sdc1    f20,CuF20(sp)           // save floating registers f20-f31
        sdc1    f22,CuF22(sp)           //
        sdc1    f24,CuF24(sp)           //
        sdc1    f26,CuF26(sp)           //
        sdc1    f28,CuF28(sp)           //
        sdc1    f30,CuF30(sp)           //

        PROLOGUE_END

//
// Save argument registers.
//

        sw      a0,CuA0(sp)             // save output buffer address
        sw      a1,CuA1(sp)             // save output length address

//
// Check if sufficient room is available on the kernel stack for another
// system call.
//

        lw      t0,KiPcr + PcCurrentThread(zero) // get current thread address
        lw      t1,KiPcr + PcInitialStack(zero) // get initial stack address
        lw      t2,ThStackLimit(t0)     // get current stack limit
        subu    t3,sp,KERNEL_LARGE_STACK_COMMIT // compute bottom address
        sltu    t4,t3,t2                // check if limit exceeded
        beq     zero,t4,10f             // if eq, limit not exceeded
        move    a0,sp                   // set current kernel stack address
        jal     MmGrowKernelStack       // attempt to grow the kernel stack
        lw      t0,KiPcr + PcCurrentThread(zero) // get current thread address
        lw      t1,KiPcr + PcInitialStack(zero) // get initial stack address
        lw      t2,ThStackLimit(t0)     // get expanded stack limit
        bne     zero,v0,20f             // if ne, attempt to grow failed
        sw      t2,KiPcr + PcStackLimit(zero) // set expanded stack limit

//
// Get the address of the current thread and save the previous trap frame
// and callback stack addresses in the current frame. Also save the new
// callback stack address in the thread object.
//

10:     lw      s8,ThTrapFrame(t0)      // get trap frame address
        lw      t2,ThCallbackStack(t0)  // get callback stack address
        sw      t1,CuInStk(sp)          // save initial stack address
        sw      s8,CuTrFr(sp)           // save trap frame address
        sw      t2,CuCbStk(sp)          // save callback stack address
        sw      sp,ThCallbackStack(t0)  // set callback stack address

//
// Restore state and callback to user mode.
//

        lw      t2,TrFsr(s8)            // get previous floating status
        li      t3,CU1_ENABLE           // set coprocessor 1 enable bits

        .set    noreorder
        .set    noat
        cfc1    t4,fsr                  // get current floating status
        mtc0    t3,psr                  // disable interrupts - 3 cycle hazzard
        ctc1    t2,fsr                  // restore previous floating status
        lw      t3,TrPsr(s8)            // get previous processor status
        sw      sp,ThInitialStack(t0)   // reset initial stack address
        sw      sp,KiPcr + PcInitialStack(zero) //
        sw      t4,CuFsr(sp)            // save current floating status
        lw      t4,KeUserCallbackDispatcher // set continuation address

//
// If a user mode APC is pending, then request an APC interrupt.
//

        lbu     t1,ThApcState + AsUserApcPending(t0) // get user APC pending
        sb      zero,ThAlerted(t0)      // clear kernel mode alerted
        mfc0    t2,cause                // get exception cause register
        sll     t1,t1,(APC_LEVEL + CAUSE_INTPEND - 1) // shift APC pending
        or      t2,t2,t1                // merge possilbe APC interrupt request
        mtc0    t2,cause                // set exception cause register

//
// Save the new processor status and continuation PC in the PCR so a TB
// miss is not possible, then restore the volatile register state.
//

        sw      t3,KiPcr + PcSavedT7(zero) // save processor status
        j       KiServiceExit           // join common code
        sw      t4,KiPcr + PcSavedEpc(zero) // save continuation address
        .set    at
        .set    reorder

//
// An attempt to grow the kernel stack failed.
//

20:     lw      ra,CuRa(sp)             // restore return address
        addu    sp,sp,CuFrameLength     // deallocate stack frame
        j       ra                      // return

        .end    KiCalluserMode

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
        .space  4 * 4                   // argument register save area
SsRa:   .space  4                       // saved return address
SsSp:   .space  4                       // saved new stack pointer
        .space  2 * 4                   // fill
SsFrameLength:                          // length of stack frame
SsA0:   .space  4                       // saved argument registers a0-a1
SsA1:   .space  4                       //

        NESTED_ENTRY(KeSwitchKernelStack, SsFrameLength, zero)

        subu    sp,sp,SsFrameLength     // allocate stack frame
        sw      ra,SsRa(sp)             // save return address

        PROLOGUE_END

//
// Save the address of the new stack and copy the old stack to the new
// stack.
//

        lw      t0,KiPcr + PcCurrentThread(zero) // get current thread address
        sw      a0,SsA0(sp)             // save new kernel stack base address
        sw      a1,SsA1(sp)             // save new kernel stack limit address
        lw      a2,ThStackBase(t0)      // get current stack base address
        lw      a3,ThTrapFrame(t0)      // get current trap frame address
        addu    a3,a3,a0                // relocate current trap frame address
        subu    a3,a3,a2                //
        sw      a3,ThTrapFrame(t0)      //
        move    a1,sp                   // set source address of copy
        subu    a2,a2,sp                // compute length of copy
        subu    a0,a0,a2                // set destination address of copy
        sw      a0,SsSp(sp)             // save new stack pointer address
        jal     RtlMoveMemory           // copy old stack to new stack

//
// Switch to new kernel stack and return the address of the old kernel
// stack.
//

        lw      t0,KiPcr + PcCurrentThread(zero) // get current thread address

        DISABLE_INTERRUPTS(t1)          // disable interrupts

        lw      v0,ThStackBase(t0)      // get old kernel stack base address
        lw      a0,SsA0(sp)             // get new kernel stack base address
        lw      a1,SsA1(sp)             // get new kernel stack limit address
        sw      a0,ThInitialStack(t0)   // set new initial stack address
        sw      a0,ThStackBase(t0)      // set new stack base address
        sw      a1,ThStackLimit(t0)     // set new stack limit address
        li      v1,TRUE                 // set large kernel stack TRUE
        sb      v1,ThLargeStack(t0)     //
        sw      a0,KiPcr + PcInitialStack(zero) // set initial stack adddress
        sw      a1,KiPcr + PcStackLimit(zero) // set stack limit
        lw      sp,SsSp(sp)             // switch to new kernel stack

        ENABLE_INTERRUPTS(t1)           // enable interrupts

        lw      ra,SsRa(sp)             // restore return address
        addu    sp,sp,SsFrameLength     // deallocate stack frame
        j       ra                      // return

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

        lw      t0,KiPcr + PcCurrentThread(zero) // get current thread address
        lw      t1,ThCallbackStack(t0)  // get callback stack address
        beq     zero,t1,10f             // if eq, no callback stack present

//
// Restore nonvolatile integer registers.
//

        lw      s0,CuS0(t1)             // restore integer registers s0-s8
        lw      s1,CuS1(t1)             //
        lw      s2,CuS2(t1)             //
        lw      s3,CuS3(t1)             //
        lw      s4,CuS4(t1)             //
        lw      s5,CuS5(t1)             //
        lw      s6,CuS6(t1)             //
        lw      s7,CuS7(t1)             //
        lw      s8,CuS8(t1)             //

//
// Save nonvolatile floating registers.
//

        ldc1    f20,CuF20(t1)           // restore floating registers f20-f31
        ldc1    f22,CuF22(t1)           //
        ldc1    f24,CuF24(t1)           //
        ldc1    f26,CuF26(t1)           //
        ldc1    f28,CuF28(t1)           //
        ldc1    f30,CuF30(t1)           //

//
// Restore the trap frame and callback stacks addresses, store the output
// buffer address and length, restore the floating status, and set the
// service status.
//

        lw      t2,CuTrFr(t1)           // get previous trap frame address
        lw      t3,CuCbStk(t1)          // get previous callback stack address
        lw      t4,CuA0(t1)             // get address to store output address
        lw      t5,CuA1(t1)             // get address to store output length
        lw      t6,CuFsr(t1)            // get previous floating status
        sw      t2,ThTrapFrame(t0)      // restore trap frame address
        sw      t3,ThCallbackStack(t0)  // restore callback stack address
        sw      a0,0(t4)                // store output buffer address
        sw      a1,0(t5)                // store output buffer length

        .set    noreorder
        .set    noat
        ctc1    t6,fsr                  // restore previous floating status
        .set    at
        .set    reorder

        move    v0,a2                   // set callback service status

//
// Restore initial stack pointer, trim stackback to callback frame,
// deallocate callback stack frame, and return to callback caller.
//

        lw      t2,CuInStk(t1)          // get previous initial stack

        DISABLE_INTERRUPTS(t3)          // disable interrupts

        sw      t2,ThInitialStack(t0)   // restore initial stack address
        sw      t2,KiPcr + PcInitialStack(zero) //
        move    sp,t1                   // trim stack back callback frame

        ENABLE_INTERRUPTS(t3)           // enable interrupts

        lw      ra,CuRa(sp)             // restore return address
        addu    sp,sp,CuFrameLength     // deallocate stack frame
        j       ra                      // return

//
// No callback is currently active.
//

10:     li      v0,STATUS_NO_CALLBACK_ACTIVE // set service status
        j       ra                      // return

        .end    NtCallbackReturn
