//      TITLE("Interrupt Support")
//++
//
// Copyright (c) 1995  Microsoft Corporation and IBM Corporation
//
// Module Name:
//
//    xxintsup.s
//
// Abstract:
//
//    This module implements the PowerPC machine dependent code for
//    interrupt handling.
//
// Author:
//
//    Chuck Lenzmeier (chuckl) 18-Feb-1995
//    Adapter from C code by Peter L. Johnston (plj@vnet.ibm.com) August 1993
//    Adapted from code by David N. Cutler (davec) 1-Apr-1991
//
// Environment:
//
//    Kernel mode only, IRQL DISPATCH_LEVEL.
//
// Revision History:
//
//--

#include "ksppc.h"

        .extern __imp_KeLowerIrql
        .extern __imp_KeRaiseIrql

#if !defined(NT_UP) && SPINDBG1
        .extern ..KiAcquireSpinLockDbg
#endif

        SBTTL("Synchronize Execution")
//++
//
// BOOLEAN
// KeSynchronizeExecution (
//    IN PKINTERRUPT Interrupt,
//    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
//    IN PVOID SynchronizeContext
//    )
//
// Routine Description:
//
//     This function synchronizes the execution of the specified routine
//     with the execution of the service routine associated with the
//     specified interrupt object.
//
// Arguments:
//
//    Interrupt (r3) - Supplies a pointer to a control object of type interrupt.
//
//    SynchronizeRoutine (r4) - Supplies a pointer to a function whose execution
//       is to be synchronized with the execution of the service routine associated
//       with the specified interrupt object.
//
//    SynchronizeContext (r5) - Supplies a pointer to an arbitrary data structure
//       which is to be passed to the function specified by the SynchronizeRoutine
//       parameter.
//
// Return Value:
//
//     The value returned by the SynchronizeRoutine function is returned as
//     the function value.
//
//--

                .struct 0
                .space  StackFrameHeaderLength
kseLR:          .space  4
kseR31:         .space  4
kseR4:          .space  4
kseR5:          .space  4
kseIrql:        .space  4
kseToc:         .space  4
                .align  3               // ensure 8 byte alignment
kseFrameLength:

//
// N.B.  We are a bit footloose with the TOC pointer in this routine.
//       We do not restore it immediately after the call to KeRaiseIrql,
//       and only restore it on return from the synchronize routine. (And
//       we only restore it then because the call to KeLowerIrql needs it.)
//

        SPECIAL_ENTRY_S(KeSynchronizeExecution,_TEXT$00)

        mflr    r0                              // get return address
        stwu    sp, -kseFrameLength(sp)         // allocate stack frame
        stw     r31, kseR31(sp)
        stw     r0,  kseLR(sp)                  // save return address

        PROLOGUE_END(KeSynchronizeExecution)

        stw     r4, kseR4(sp)                   // save synchronization routine address
        lwz     r6, [toc]__imp_KeRaiseIrql(rtoc) // &&function descriptor
        stw     r5, kseR5(sp)                   // save synchronization routine context
        lwz     r6, 0(r6)                       // &function descriptor
        stw     rtoc, kseToc(sp)                // save our TOC
        lwz     r5, 0(r6)                       // &KeRaiseIrql

//
// Raise IRQL to the synchronization level and acquire the associated
// spin lock.
//

#if !defined(NT_UP)
        lwz     r31, InActualLock(r3)           // get address of spin lock
#endif

        lwz     rtoc, 4(r6)                     // HAL's TOC
        lbz     r3, InSynchronizeIrql(r3)       // get synchronization IRQL
        mtctr   r5
        addi    r4, sp, kseIrql                 // compute address to save IRQL
        bctrl
                                                // N.B.  skip restoring the TOC

        lwz     r4, kseR4(sp)                   // get synchronize routine descriptor

#if !defined(NT_UP)
        ACQUIRE_SPIN_LOCK(r31, r31, r5, kse_lock, kse_lock_spin)
#endif

//
// Call specified routine passing the specified context parameter.
//
        lwz     r5, 0(r4)                       // get synchronize routine address
        lwz     rtoc, 4(r4)                     // get synchronize routine TOC
        mtctr   r5                              // put routine address in CTR
        lwz     r3, kseR5(sp)                   // get synchronize routine context
        bctrl                                   // call specified routine

//
// Release spin lock, lower IRQL to its previous level, and return the value
// returned by the specified routine.
//

        lwz     rtoc, kseToc(sp)                // restore our TOC
#if !defined(NT_UP)
        li      r5, 0                           // get a 0 for spin lock
#endif
        lwz     r6, [toc]__imp_KeLowerIrql(rtoc) // &&function descriptor
#if !defined(NT_UP)
        RELEASE_SPIN_LOCK(r31, r5)
#endif
        lwz     r6, 0(r6)                       // &function descriptor
        ori     r31, r3, 0                      // save return value
        lwz     r5, 0(r6)                       // &KeLowerIrql

        lbz     r3, kseIrql(sp)                 // get saved IRQL
        mtctr   r5
        lwz     rtoc, 4(r6)                     // HAL's TOC
        bctrl
        lwz     rtoc, kseToc(sp)                // restore our TOC

        ori     r3, r31, 0                      // set return value

        lwz     r0,  kseLR(sp)                  // get return address
        lwz     r31, kseR31(sp)                 // restore r31
        mtlr    r0                              // set return address
        addi    sp, sp, kseFrameLength          // return stack frame

        blr

#if !defined(NT_UP)
        SPIN_ON_SPIN_LOCK(r31, r5, kse_lock, kse_lock_spin)
#endif

        DUMMY_EXIT(KeSynchronizeExecution)

        SBTTL("Chained Dispatch")
//++
//
// VOID
// KiChainedDispatch (
//    IN PKINTERRUPT Interrupt,
//    IN PVOID ServiceContext,
//    IN PVOID TrapFrame
//    )
//
// Routine Description:
//
//     This routine is entered as a result of an interrupt being generated
//     via a vector that is connected to more than one interrupt object.  Its
//     function is to walk the list of connected interrupt objects and call
//     each interrupt service routine.  If the mode of the interrupt is latched,
//     then a complete traversal of the chain must be performed.
//
// Arguments:
//
//    Interrupt (r3) - Supplies a pointer to the Interrupt Object.
//
//    ServiceContext (r4) - Supplies a pointer to the Service Context associated
//       with this Interrupt Object.
//
//    TrapFrame (r5) - Supplies the address of the Trap Frame created as a
//       result of this interrupt.
//
// Return Value:
//
//     None.
//
//--

                .struct 0
                .space  StackFrameHeaderLength
kcdLR:          .space  4
#if !defined(NT_UP)
kcdR24:         .space  4
#endif
kcdR25:         .space  4
kcdR26:         .space  4
kcdR27:         .space  4
kcdR28:         .space  4
kcdR29:         .space  4
kcdR30:         .space  4
kcdR31:         .space  4
kcdIrql:        .space  4
kcdToc:         .space  4
                .align  3               // ensure 8 byte alignment
kcdFrameLength:

//
// N.B.  We are a bit footloose with the TOC pointer in this routine.
//       We only ensure the TOC is correct when we explicitly need it
//       to be correct.  We do not restore it on exit.  This is safe
//       because we know that our caller called us via a function
//       descriptor and will therefore restore its own TOC when we return.
//

        SPECIAL_ENTRY(KiChainedDispatch)

        mflr    r0                              // get return address
        stwu    sp, -kcdFrameLength(sp)         // allocate stack frame
#if !defined(NT_UP)
        stw     r24, kcdR24(sp)
#endif
        stw     r25, kcdR25(sp)
        stw     r26, kcdR26(sp)
        stw     r27, kcdR27(sp)
        stw     r28, kcdR28(sp)
        stw     r29, kcdR29(sp)
        stw     r30, kcdR30(sp)
        stw     r31, kcdR31(sp)
        stw     r0,  kcdLR(sp)                  // save return address

        PROLOGUE_END(KiChainedDispatch)

        ori     r25, r5, 0                      // save trap frame address
        stw     rtoc, kcdToc(sp)                // save our TOC

//
// Initialize loop variables.
//

        addi    r31, r3, InInterruptListEntry   // set address of listhead
        ori     r30, r31, 0                     // set address of first entry
        li      r29, 0                          // clear floating state saved flag
        lbz     r28, InMode(r3)                 // get mode of interrupt
        lbz     r27, InIrql(r3)                 // get interrupt source IRQL

//
// Walk the list of connected interrupt objects and call the respective
// interrupt service routines.
//

kcd10:

        lbz     r8, InFloatingSave(r3)          // get floating save flag
        cmpwi   r29, 0                          // floating state already saved?
        cmpwi   cr7, r8, 0                      // interrupt uses floating state?
        bne     kcd20                           // if ne, floating state already saved
        beq     cr7, kcd20                      // if eq, don't save floating state

//
// Save volatile floating registers in trap frame.
//

        li      r29, 1                          // set floating state saved flag
        SAVE_VOLATILE_FLOAT_STATE(r25)          // save volatile floating state

kcd20:

//
// Raise IRQL to synchronization level if synchronization level is not
// equal to the interrupt source level.
//

        lbz     r26, InSynchronizeIrql(r3)      // get synchronization IRQL
        cmpw    r27, r26                        // IRQL levels the same?
        beq     kcd25                           // if eq, IRQL levels are the same

        lwz     r6, [toc]__imp_KeRaiseIrql(rtoc) // &&function descriptor
        ori     r3, r26, 0                      // set synchronization IRQL
        lwz     r6, 0(r6)                       // &function descriptor
        addi    r4, sp, kcdIrql                 // compute address to save IRQL
        lwz     r5, 0(r6)                       // &KeRaiseIrql
        lwz     rtoc, 4(r6)                     // HAL's TOC
        mtctr   r5
        bctrl
                                                // N.B.  skip restoring the TOC

        subi    r3, r30, InInterruptListEntry   // recompute interrupt object address

kcd25:

        lwz     r5, InServiceRoutine(r3)        // get service routine descriptor

//
// Acquire the service routine spin lock and call the service routine.
//

#if !defined(NT_UP)

        lwz     r24, InActualLock(r3)           // get address of spin lock
        ACQUIRE_SPIN_LOCK(r24, r24, r7, kcd_lock, kcd_lock_spin)
#endif

        lwz     r6, 0(r5)                       // get address of service routine
        lwz     rtoc, 4(r5)                     // get service routine TOC
        mtctr   r6                              // put routine address in CTR
        lwz     r4, InServiceContext(r3)        // get service context
        ori     r5, r25, 0                      // pass &TrapFrame
        bctrl                                   // call service routine

//
// Release the service routine spin lock.  Lower IRQL to the interrupt source
// level if synchronization level is not the same as the interrupt source level.
//

        lwz     rtoc, kcdToc(sp)                // restore our TOC
#if !defined(NT_UP)
        li      r4, 0                           // get a 0 for spin lock
#endif
        cmpw    r27, r26                        // IRQL levels the same?
#if !defined(NT_UP)
        RELEASE_SPIN_LOCK(r24, r4)
#endif
        ori     r26, r3, 0                      // save service routine status
        beq     kcd35                           // if eq, IRQL levels are the same

        lwz     r6, [toc]__imp_KeLowerIrql(rtoc) // &&function descriptor
        ori     r3, r27, 0                      // set interrupt source IRQL
        lwz     r6, 0(r6)                       // &function descriptor
        lwz     r5, 0(r6)                       // &KeLowerIrql
        lwz     rtoc, 4(r6)                     // HAL's TOC
        mtctr   r5
        bctrl
        lwz     rtoc, kcdToc(sp)                // restore our TOC (for next loop)

kcd35:

//
// Get next list entry and check for end of loop.
//

        lwz     r30, LsFlink(r30)               // get next interrupt object address
        cmpwi   r26, 0                          // interrupt handled?
        cmpwi   cr7, r28, 0                     // level sensitive interrupt?
        cmpw    cr6, r30, r31                   // end of list?
        beq     kcd40                           // if eq, interrupt not handled
        beq     cr7,kcd50                       // if eq, level sensitive interrupt
kcd40:
        subi    r3, r30, InInterruptListEntry   // compute interrupt object address
        bne     cr6,kcd10                       // if ne, not end of list
kcd50:

//
// Either the interrupt is level sensitive and has been handled or the end of
// the interrupt object chain has been reached. Check to determine if floating
// machine state needs to be restored.
//

        cmpwi   r29, 0                          // floating state saved?
        beq     kcd60                           // if eq, floating state not saved

//
// Restore volatile floating registers from trap frame.
//

        RESTORE_VOLATILE_FLOAT_STATE(r25)       // restore volatile floating state

kcd60:

//
// Restore nonvolatile registers, retrieve return address, deallocate
// stack frame, and return.
//

        lwz     r0,  kcdLR(sp)                  // get return address
        lwz     r31, kcdR31(sp)                 // restore r31
        lwz     r30, kcdR30(sp)                 // restore r30
        lwz     r29, kcdR29(sp)                 // restore r29
        lwz     r28, kcdR28(sp)                 // restore r28
        lwz     r27, kcdR27(sp)                 // restore r27
        lwz     r26, kcdR26(sp)                 // restore r26
        lwz     r25, kcdR25(sp)                 // restore r25
#if !defined(NT_UP)
        lwz     r24, kcdR24(sp)                 // restore r24
#endif
        mtlr    r0                              // set return address
        addi    sp, sp, kcdFrameLength          // return stack frame

        blr

#if !defined(NT_UP)
        SPIN_ON_SPIN_LOCK(r24, r7, kcd_lock, kcd_lock_spin)
#endif

        DUMMY_EXIT(KiChainedDispatch)

        SBTTL("Floating Dispatch")
//++
//
// VOID
// KiFloatingDispatch (
//    IN PKINTERRUPT Interrupt,
//    IN PVOID ServiceContext,
//    IN PVOID TrapFrame
//    )
//
// Routine Description:
//
//    This routine is entered as the result of an interrupt being generated
//    via a vector that is connected to an interrupt object. Its function is
//    to save the volatile floating machine state and then call the specified
//    interrupt service routine.
//
//    N.B. On entry to this routine only the volatile integer registers have
//       been saved.
//
// Arguments:
//
//    Interrupt (r3) - Supplies a pointer to the Interrupt Object.
//
//    ServiceContext (r4) - Supplies a pointer to the Service Context associated
//       with this Interrupt Object.
//
//    TrapFrame (r5) - Supplies the address of the Trap Frame created as a
//       result of this interrupt.
//
// Return Value:
//
//     None.
//
//--

                .struct 0
                .space  StackFrameHeaderLength
kfdLR:          .space  4
#if !defined(NT_UP)
kfdR29:         .space  4
#endif
kfdR30:         .space  4
kfdR31:         .space  4
kfdIrql:        .space  4
kfdToc:         .space  4
                .align  3               // ensure 8 byte alignment
kfdFrameLength:

//
// N.B.  We are a bit footloose with the TOC pointer in this routine.
//       We only ensure the TOC is correct when we explicitly need it
//       to be correct.  We do not restore it on exit.  This is safe
//       because we know that our caller called us via a function
//       descriptor and will therefore restore its own TOC when we return.
//

        SPECIAL_ENTRY(KiFloatingDispatch)

        mflr    r0                              // get return address
        stwu    sp, -kfdFrameLength(sp)         // allocate stack frame
#if !defined(NT_UP)
        stw     r29, kfdR29(sp)
#endif
        stw     r30, kfdR30(sp)
        stw     r31, kfdR31(sp)
        stw     r0,  kfdLR(sp)                  // save return address

        PROLOGUE_END(KiFloatingDispatch)

        ori     r30, r5, 0                      // save trap frame address
        stw     rtoc, kfdToc(sp)                // save our TOC

//
// Save volatile floating registers in trap frame.
//

        SAVE_VOLATILE_FLOAT_STATE(r5)           // save volatile floating state

//
// Raise IRQL to synchronization level if synchronization level is not
// equal to the interrupt source level.
//

        ori     r31, r3, 0                      // save address of interrupt object
        lbz     r4, InIrql(r3)                  // get interrupt source IRQL
        lbz     r8, InSynchronizeIrql(r3)       // get synchronization IRQL
        cmpw    r8, r4                          // IRQL levels the same?
        beq     kfd10                           // if eq, IRQL levels are the same

        lwz     r6, [toc]__imp_KeRaiseIrql(rtoc) // &&function descriptor
        ori     r3, r8, 0                       // set synchronization IRQL
        lwz     r6, 0(r6)                       // &function descriptor
        addi    r4, sp, kfdIrql                 // compute address to save IRQL
        lwz     r8, 0(r6)                       // &KeLowerIrql
        lwz     rtoc, 4(r6)                     // HAL's TOC
        mtctr   r8
        bctrl
                                                // N.B.  skip restoring the TOC

        ori     r3, r31, 0                      // restore address of interrupt object

kfd10:

        lwz     r8, InServiceRoutine(r31)       // get service routine descriptor

//
// Acquire the service routine spin lock and call the service routine.
//

#if !defined(NT_UP)
        lwz     r29, InActualLock(r31)          // get address of spin lock
        ACQUIRE_SPIN_LOCK(r29, r29, r7, kfd_lock, kfd_lock_spin)
#endif

        lwz     r6, 0(r8)                       // get address of service routine
        lwz     rtoc, 4(r8)                     // get service routine TOC
        mtctr   r6                              // put routine address in CTR
        lwz     r4, InServiceContext(r31)       // get service context
        bctrl                                   // call service routine

//
// Release the service routine spin lock.  Lower IRQL to the interrupt source
// level if synchronization level is not the same as the interrupt source level.
//

        lwz     rtoc, kfdToc(sp)                // restore our TOC
#if !defined(NT_UP)
        li      r6, 0                           // get a 0 for spin lock
#endif
        lbz     r3, InIrql(r31)                 // get interrupt source IRQL
        lbz     r4, InSynchronizeIrql(r31)      // get synchronization IRQL
#if !defined(NT_UP)
        RELEASE_SPIN_LOCK(r29, r6)
#endif
        cmpw    r3, r4                          // IRQL levels the same?
        beq     kfd30                           // if eq, IRQL levels are the same

        lwz     r6, [toc]__imp_KeLowerIrql(rtoc) // &&function descriptor
        lwz     r6, 0(r6)                        // &function descriptor
        lwz     r5, 0(r6)                        // &KeLowerIrql
        lwz     rtoc, 4(r6)                      // HAL's TOC
        mtctr   r5
        bctrl

kfd30:

//
// Restore volatile floating registers from trap frame.
//

        RESTORE_VOLATILE_FLOAT_STATE(r30)       // restore volatile floating state

//
// Restore nonvolatile registers, retrieve return address, deallocate
// stack frame, and return.
//

        lwz     r0,  kfdLR(sp)                  // get return address
#if !defined(NT_UP)
        lwz     r29, kfdR29(sp)                 // restore r29
#endif
        lwz     r30, kfdR30(sp)                 // restore r30
        lwz     r31, kfdR31(sp)                 // restore r31
        mtlr    r0                              // set return address
        addi    sp, sp, kfdFrameLength          // return stack frame

        blr

#if !defined(NT_UP)
        SPIN_ON_SPIN_LOCK(r29, r7, kfd_lock, kfd_lock_spin)
#endif

        DUMMY_EXIT(KiFloatingDispatch)

        SBTTL("Interrupt Dispatch - Raise IRQL")
//++
//
// VOID
// KiInterruptDispatchRaise (
//    IN PKINTERRUPT Interrupt,
//    IN PVOID ServiceContext,
//    IN PVOID TrapFrame
//    )
//
// Routine Description:
//
//    This routine is entered as the result of an interrupt being generated
//    via a vector that is connected to an interrupt object. Its function is
//    to directly call the specified interrupt service routine.
//
//    N.B. This routine raises the interrupt level to the synchronization
//       level specified in the interrupt object.
//
// Arguments:
//
//    Interrupt (r3) - Supplies a pointer to the Interrupt Object.
//
//    ServiceContext (r4) - Supplies a pointer to the Service Context associated
//       with this Interrupt Object.
//
//    TrapFrame (r5) - Supplies the address of the Trap Frame created as a
//       result of this interrupt.
//
// Return Value:
//
//     None.
//
//--

                .struct 0
                .space  StackFrameHeaderLength
kidrLR:         .space  4
kidrR31:        .space  4
kidrIrql:       .space  4
kidrToc:        .space  4
                .align  3               // ensure 8 byte alignment
kidrFrameLength:

//
// N.B.  We are a bit footloose with the TOC pointer in this routine.
//       We only ensure the TOC is correct when we explicitly need it
//       to be correct.  We do not restore it on exit.  This is safe
//       because we know that our caller called us via a function
//       descriptor and will therefore restore its own TOC when we return.
//

        SPECIAL_ENTRY(KiInterruptDispatchRaise)

        mflr    r0                              // get return address
        stwu    sp, -kidrFrameLength(sp)        // allocate stack frame
        stw     r31, kidrR31(sp)
        lwz     r6, [toc]__imp_KeRaiseIrql(rtoc) // &&function descriptor
        stw     r0,  kidrLR(sp)                 // save return address

        PROLOGUE_END(KiInterruptDispatchRaise)

        lwz     r6, 0(r6)                       // &function descriptor
        stw     rtoc, kidrToc(sp)               // save our TOC

//
// Raise IRQL to synchronization level.
//

        lwz     r8, 0(r6)                       // &KeRaiseIrql
        ori     r31, r3, 0                      // save address of interrupt object
        lwz     rtoc, 4(r6)                     // HAL's TOC
        lbz     r3, InSynchronizeIrql(r3)       // get synchronization IRQL
        mtctr   r8
        addi    r4, sp, kidrIrql                // compute address to save IRQL
        bctrl
                                                // N.B.  skip restoring the TOC

        lwz     r8, InServiceRoutine(r31)       // get service routine descriptor
        ori     r3, r31, 0                      // restore address of interrupt object
        lwz     r4, InServiceContext(r31)       // get service context

//
// Acquire the service routine spin lock and call the service routine.
//

        lwz     r6, 0(r8)                       // get address of service routine

#if !defined(NT_UP)
        lwz     r31, InActualLock(r31)          // get address of spin lock
        ACQUIRE_SPIN_LOCK(r31, r31, r7, kidr_lock, kidr_lock_spin)
#endif

        mtctr   r6                              // put routine address in CTR
        lwz     rtoc, 4(r8)                     // get service routine TOC
        bctrl                                   // call service routine

//
// Release the service routine spin lock.  Lower IRQL to the previous level.
//

        lwz     rtoc, kidrToc(sp)               // restore our TOC
#if !defined(NT_UP)
        li      r4, 0                           // get a 0 for spin lock
#endif
        lwz     r6, [toc]__imp_KeLowerIrql(rtoc) // &&function descriptor
#if !defined(NT_UP)
        RELEASE_SPIN_LOCK(r31, r4)
#endif
        lwz     r6, 0(r6)                       // &function descriptor
        lwz     r8, 0(r6)                       // &KeLowerIrql
        lwz     rtoc, 4(r6)                     // HAL's TOC
        mtctr   r8
        lbz     r3,kidrIrql(sp)                 // get previous IRQL
        bctrl
                                                // N.B.  skip restoring the TOC

//
// Restore nonvolatile registers, retrieve return address, deallocate
// stack frame, and return.
//

        lwz     r0,  kidrLR(sp)                 // get return address
        lwz     r31, kidrR31(sp)                // restore r31
        mtlr    r0                              // set return address
        addi    sp, sp, kidrFrameLength         // return stack frame

        blr

#if !defined(NT_UP)
        SPIN_ON_SPIN_LOCK(r31, r7, kidr_lock, kidr_lock_spin)
#endif

        DUMMY_EXIT(KiInterruptDispatchRaise)

        SBTTL("Interrupt Dispatch - Same IRQL")
//++
//
// VOID
// KiInterruptDispatchSame (
//    IN PKINTERRUPT Interrupt,
//    IN PVOID ServiceContext,
//    IN PVOID TrapFrame
//    )
//
// Routine Description:
//
//    This routine is entered as the result of an interrupt being generated
//    via a vector that is connected to an interrupt object. Its function is
//    to directly call the specified interrupt service routine.
//
//    Note: On PowerPC, if uniprocessor, this routine is bypassed completely.
//    The Interrupt Object is initialized such that KiInterruptException will
//    dispatch directly to the service routine.
//
// Arguments:
//
//    Interrupt (r3) - Supplies a pointer to the Interrupt Object.
//
//    ServiceContext (r4) - Supplies a pointer to the Service Context associated
//       with this Interrupt Object.
//
//    TrapFrame (r5) - Supplies the address of the Trap Frame created as a
//       result of this interrupt.
//
// Return Value:
//
//     None.
//
//--

                .struct 0
                .space  StackFrameHeaderLength
kidsLR:         .space  4
kidsR31:        .space  4
                .align  3               // ensure 8 byte alignment
kidsFrameLength:

//
// N.B.  We do not save/restore the TOC pointer in this routine.
//       This is safe because our caller called us via a function
//       descriptor and will therefore restore the TOC when we return.
//

#if defined(NT_UP)

        LEAF_ENTRY(KiInterruptDispatchSame)

        lwz     r8, InServiceRoutine(r3)        // get service routine descriptor
        lwz     r6, 0(r8)                       // get address of service routine
        lwz     rtoc, 4(r8)                     // get service routine TOC
        mtctr   r6                              // put routine address in CTR
        lwz     r4, InServiceContext(r3)        // get service context
        bctr                                    // jump to service routine

        DUMMY_EXIT(KiInterruptDispatchSame)

#else

        SPECIAL_ENTRY(KiInterruptDispatchSame)

        mflr    r0                              // get return address
        stwu    sp, -kidsFrameLength(sp)        // allocate stack frame
        stw     r31, kidsR31(sp)
        stw     r0,  kidsLR(sp)                 // save return address

        PROLOGUE_END(KiInterruptDispatchSame)

        lwz     r8, InServiceRoutine(r3)        // get service routine descriptor

//
// Acquire the service routine spin lock and call the service routine.
//

        lwz     r31, InActualLock(r3)           // get address of spin lock
        lwz     r6, 0(r8)                       // get address of service routine
        ACQUIRE_SPIN_LOCK(r31, r31, r7, kids_lock, kids_lock_spin)

        lwz     rtoc, 4(r8)                     // get service routine TOC
        mtctr   r6                              // put routine address in CTR
        lwz     r4, InServiceContext(r3)        // get service context
        bctrl                                   // call service routine

//
// Release the service routine spin lock.  Restore nonvolatile registers,
// retrieve return address, deallocate stack frame, and return.
//

        li      r4, 0                           // get a 0 for spin lock
        lwz     r0,  kidsLR(sp)                 // get return address
        RELEASE_SPIN_LOCK(r31, r4)
        lwz     r31, kidsR31(sp)                // restore r31
        mtlr    r0                              // set return address
        addi    sp, sp, kidsFrameLength         // return stack frame

        blr

        SPIN_ON_SPIN_LOCK(r31, r7, kids_lock, kids_lock_spin)

        DUMMY_EXIT(KiInterruptDispatchSame)

#endif // else defined(NT_UP)

        SBTTL("Unexpected Interrupt")
//++
//
// Routine Description:
//
//    This routine is entered as the result of an interrupt being generated
//    via a vector that is not connected to an interrupt object. Its function
//    is to report the error and dismiss the interrupt.
//
// Arguments:
//
//    Interrupt (r3) - Supplies a pointer to the Interrupt Object.
//
//    ServiceContext (r4) - Supplies a pointer to the Service Context associated
//       with this Interrupt Object.
//
//    TrapFrame (r5) - Supplies the address of the Trap Frame created as a
//       result of this interrupt.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiUnexpectedInterrupt)

        LEAF_EXIT(KiUnexpectedInterrupt)        // ****** temp ******
