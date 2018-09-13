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
//    Chuck Lenzmeier (chuckl) 11-Nov-1994
//      modified from MIPS version by David N. Cutler (davec)
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

//list(off)
#include "ksppc.h"
//list(on)

//
// Define external variables that can be addressed using GP.
//

        .extern KeUserCallbackDispatcher

        .extern ..MmGrowKernelStack
        .extern ..KiServiceExit
        .extern ..RtlCopyMemory
        .extern .._savegpr_14
        .extern .._restgpr_14
        .extern .._savefpr_14
        .extern .._restfpr_14


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
//    OutputBuffer (r.3) - Supplies a pointer to the variable that receivies
//        the address of the output buffer.
//
//    OutputLength (r.4) - Supplies a pointer to a variable that receives
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

        NESTED_ENTRY_S(KiCallUserMode, CuFrameLength, 18, 18, _TEXT$01)

        PROLOGUE_END(KiCallUserMode)

//
// Save argument registers.  Check if sufficient room is available on
// the kernel stack for another system call.
//

        lwz     r.6,KiPcr+PcCurrentThread(r.0)  // get current thread address
        stw     r.4,CuR4(r.sp)                  // save output length address
        stw     r.3,CuR3(r.sp)                  // save output buffer address

        lwz     r.9,ThStackLimit(r.6)           // get current stack limit
        subi    r.7,r.sp,KERNEL_LARGE_STACK_COMMIT // compute bottom address
        cmplw   r.7,r.9                         // check if limit exceeded
        bge     Cu.10                           // if ge, limit not exceeded

        ori     r.3,r.sp,0                      // set current kernel stack address
        bl      ..MmGrowKernelStack             // attempt to grow the kernel stack
        cmpwi   r.3,0                           // did it work?
        lwz     r.6,KiPcr+PcCurrentThread(r.0)  // get current thread address
        bne     Cu.20                           // jump if it failed
        lwz     r.9,ThStackLimit(r.6)           // get new stack limit
        stw     r.9,KiPcr+PcStackLimit(r.0)     // set new stack limit

Cu.10:

//
// Get the user-mode continuation address.
//
// Get the address of the current thread and save the previous trap frame
// and callback stack addresses in the current frame. Also save the new
// callback stack address in the thread object.
//

        lwz     r.9,[toc]KeUserCallbackDispatcher(r.toc) // get address of KeUserCallbackDispatcher
        lwz     r.5,KiPcr+PcInitialStack(r.0)   // get initial stack address
        lwz     r.12,ThTrapFrame(r.6)           // get trap frame address
        lwz     r.7,ThCallbackStack(r.6)        // get callback stack address
        lwz     r.9,0(r.9)                      // get address of descriptor
        stw     r.5,CuInStk(r.sp)               // save initial stack address
        stw     r.12,CuTrFr(r.sp)               // save trap frame address
        stw     r.7,CuCbStk(r.sp)               // save callback stack address
        stw     r.sp,ThCallbackStack(r.6)       // set callback stack address
        lwz     r.10,0(r.9)                     // get continuation IAR
        lwz     r.11,4(r.9)                     // get continuation TOC

//
// Restore state and callback to user mode.
//

        DISABLE_INTERRUPTS(r.7, r.8)            // disable interrupts

        lwz     r.7,TrIar(r.12)                 // get trap IAR
        lwz     r.8,TrGpr2(r.12)                // get trap TOC
        stw     r.sp,ThInitialStack(r.6)        // reset initial stack address
        stw     r.sp,KiPcr+PcInitialStack(r.0)  //
        stw     r.10,TrIar(r.12)                // set trap IAR
        stw     r.11,TrGpr2(r.12)               // set trap TOC
        stw     r.7,CuTrIar(r.sp)               // save trap IAR
        stw     r.8,CuTrToc(r.sp)               // save trap TOC

        lwz     r.10,TrMsr(r.12)                // get caller's MSR value
        lbz     r.8,KiPcr+PcCurrentIrql(r.0)    // get current IRQL

        b       ..KiServiceExit

//
// An attempt to grow the kernel stack failed.
//
// Note:  We don't need to restore the nonvolatile registers here,
//        since we didn't modify them.  So we don't use NESTED_EXIT,
//        and pop the stack manually instead.

Cu.20:
        lwz     r.0,CuLr(r.sp)
        addi    r.sp,r.sp,CuFrameLength
        mtlr    r.0
        SPECIAL_EXIT(KiCallUserMode)

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
//    StackBase (r.3) - Supplies a pointer to the base of the new kernel
//        stack.
//
//    StackLimit (r.4) - supplies a pointer to the limit of the new kernel
//        stack.
//
// Return Value:
//
//    The old kernel stack is returned as the function value.
//
//--

                .struct 0
SsFrame:        .space  StackFrameHeaderLength
SsSp:           .space  4               // saved new stack pointer
SsR3:           .space  4               // saved R3 (StackBase)
SsR4:           .space  4               // saved R4 (StackLimit)
SsLr:           .space  4               // saved LR
SsGpr:          .space  4 * 2           // saved GPRs
                .align  3
SsFrameLength:                          // length of stack frame

        NESTED_ENTRY_S(KeSwitchKernelStack, SsFrameLength, 2, 0, _TEXT$01)

        PROLOGUE_END(KeSwitchKernelStack)

//
// Save the address of the new stack and copy the old stack to the new
// stack.
//

        stw     r.3,SsR3(r.sp)                  // save new kernel stack base address
        lwz     r.31,KiPcr+PcCurrentThread(r.0) // get current thread address
        stw     r.4,SsR4(r.sp)                  // save new kernel stack limit address
        lwz     r.6,ThTrapFrame(r.31)           // get current trap frame address
        lwz     r.5,ThStackBase(r.31)           // get current stack base address
        sub     r.30,r.3,r.5                    // calculate offset from old stack to new
        ori     r.4,r.sp,0                      // set source address of copy
        sub     r.5,r.5,r.sp                    // compute length of copy
        add     r.6,r.6,r.30                    // relocate current trap frame address
        sub     r.3,r.3,r.5                     // set destination address of copy
        stw     r.6,ThTrapFrame(r.31)           // store relocated trap frame address
        stw     r.3,SsSp(r.sp)                  // save new stack pointer address
        bl      ..RtlCopyMemory                 // copy old stack to new stack

//
// Switch to new kernel stack and return the address of the old kernel
// stack.
//

        DISABLE_INTERRUPTS(r.4, r.5)            // disable interrupts

        lwz     r.3,ThStackBase(r.31)           // get old kernel stack base address
        lwz     r.5,SsR3(r.sp)                  // get new kernel stack base address
        lwz     r.6,SsR4(r.sp)                  // get new kernel stack limit address
        li      r.7,TRUE
        lwz     r.sp,SsSp(r.sp)                 // switch to new kernel stack
        stw     r.5,KiPcr+PcInitialStack(r.0)   // set new initial stack adddress
        stw     r.5,ThInitialStack(r.31)        // set new initial stack address
        lwz     r.8,SsFrameLength(r.sp)         // get caller's stack frame link
        stw     r.5,ThStackBase(r.31)           // set new stack base address
        stw     r.6,KiPcr+PcStackLimit(r.0)     // set new stack limit adddress
        add     r.8,r.8,r.30                    // relocate caller's stack frame link
        stw     r.6,ThStackLimit(r.31)          // set new stack limit address
        stb     r.7,ThLargeStack(r.31)          // set large kernel stack TRUE
        stw     r.8,SsFrameLength(r.sp)         // set caller's new stack frame link

        ENABLE_INTERRUPTS(r.4)                  // enable interrupts

        NESTED_EXIT(KeSwitchKernelStack, SsFrameLength, 2, 0)

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
//    OutputBuffer (r.3) - Supplies an optional pointer to an output buffer.
//
//    OutputLength (r.4) - Supplies the length of the output buffer.
//
//    Status (r.5) - Supplies the status value returned to the caller of the
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

        LEAF_ENTRY_S(NtCallbackReturn, _TEXT$01)

        lwz     r.7,KiPcr+PcCurrentThread(r.0)  // get current thread address
        lwz     r.8,ThCallbackStack(r.7)        // get callback stack address
        cmpwi   r.8,0                           // if eq, no callback stack present
        beq-    cr.10

//
// Restore the trap frame and callback stacks addresses, store the output
// buffer address and length, restore the floating status, and set the
// service status.
//

        lwz     r.9,CuR3(r.8)                   // get address to store output address
        lwz     r.10,CuR4(r.8)                  // get address to store output length
        lwz     r.11,CuTrFr(r.8)                // get previous trap frame address
        lwz     r.12,CuCbStk(r.8)               // get previous callback stack address
        stw     r.3,0(r.9)                      // store output buffer address
        lwz     r.3,CuTrIar(r.8)                // get saved trap IAR
        stw     r.4,0(r.10)                     // store output buffer length
        lwz     r.4,CuTrToc(r.8)                // get saved trap TOC
        stw     r.11,ThTrapFrame(r.7)           // restore trap frame address
        stw     r.12,ThCallbackStack(r.7)       // restore callback stack address
        stw     r.3,TrIar(r.11)                 // restore trap IAR
        stw     r.4,TrGpr2(r.11)                // restore trap TOC

        ori     r.3,r.5,0                       // set callback service status

//
// **** this is the place where the current stack would be trimmed back.
//

//
// Restore initial stack pointer, trim stackback to callback frame,
// deallocate callback stack frame, and return to callback caller.
//

        lwz     r.4,CuInStk(r.8)                // get previous initial stack

        DISABLE_INTERRUPTS(r.5, r.9)            // disable interrupts

        stw     r.4,ThInitialStack(r.7)         // restore initial stack address
        stw     r.4,KiPcr+PcInitialStack(r.0)   //
        ori     r.sp,r.8,0                      // trim stack back to callback frame

        ENABLE_INTERRUPTS(r.5)                  // enable interrupts

        NESTED_EXIT(NtCallbackReturn, CuFrameLength, 18, 18)
//
// No callback is currently active.
//

cr.10:
        LWI     (r.3,STATUS_NO_CALLBACK_ACTIVE) // set service status
        ALTERNATE_EXIT(NtCallbackReturn)

