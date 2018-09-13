//      TITLE("Interrupt Object Support Routines")
//++
//
// Copyright (c) 1990  Microsoft Corporation
//
// Module Name:
//
//    xxintsup.s
//
// Abstract:
//
//    This module implements the code necessary to support interrupt objects.
//    It contains the interrupt dispatch code and the code template that gets
//    copied into an interrupt object.
//
// Author:
//
//    David N. Cutler (davec) 2-Apr-1990
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksmips.h"

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
//    This function synchronizes the execution of the specified routine with the
//    execution of the service routine associated with the specified interrupt
//    object.
//
// Arguments:
//
//    Interrupt (a0) - Supplies a pointer to a control object of type interrupt.
//
//    SynchronizeRoutine (a1) - Supplies a pointer to a function whose execution
//       is to be synchronized with the execution of the service routine associated
//       with the specified interrupt object.
//
//    SynchronizeContext (a2) - Supplies a pointer to an arbitrary data structure
//       which is to be passed to the function specified by the SynchronizeRoutine
//       parameter.
//
// Return Value:
//
//    The value returned by the SynchronizeRoutine function is returned as the
//    function value.
//
//--

        .struct 0
SyArg:  .space  4 * 4                   // argument register save area
SyS0:   .space  4                       // saved integer register s0
SyIrql: .space  4                       // saved IRQL value
        .space  4                       // fill for alignment
SyRa:   .space  4                       // saved return address
SyFrameLength:                          // length of stack frame
SyA0:   .space  4                       // saved argument registers a0 - a2
SyA1:   .space  4                       //
SyA2:   .space  4                       //

        NESTED_ENTRY(KeSynchronizeExecution, SyFrameLength, zero)

        subu    sp,sp,SyFrameLength     // allocate stack frame
        sw      ra,SyRa(sp)             // save return address
        sw      s0,SyS0(sp)             // save integer register s0

        PROLOGUE_END

        sw      a1,SyA1(sp)             // save synchronization routine address
        sw      a2,SyA2(sp)             // save synchronization routine context

//
// Raise IRQL to the synchronization level and acquire the associated
// spin lock.
//

#if defined(R4000) && !defined(NT_UP)

        lw      s0,InActualLock(a0)     // get address of spin lock

#endif

        lbu     a0,InSynchronizeIrql(a0) // get synchronization IRQL
        addu    a1,sp,SyIrql            // compute address to save IRQL
        jal     KeRaiseIrql             // raise IRQL to synchronization IRQL

#if defined(R4000) && !defined(NT_UP)

10:     ll      t0,0(s0)                // get current lock value
        move    t1,s0                   // set lock ownership value
        bne     zero,t0,10b             // if ne, spin lock owned
        sc      t1,0(s0)                // set spin lock owned
        beq     zero,t1,10b             // if eq, store conditional failed

#endif

//
// Call specified routine passing the specified context parameter.
//

        lw      t0,SyA1(sp)             // get synchronize routine address
        lw      a0,SyA2(sp)             // get synchronize routine context
        jal     t0                      // call specified routine

//
// Release spin lock, lower IRQL to its previous level, and return the value
// returned by the specified routine.
//

#if defined(R4000) && !defined(NT_UP)

        sw      zero,0(s0)              // set spin lock not owned

#endif

        lbu     a0,SyIrql(sp)           // get saved IRQL
        move    s0,v0                   // save return value
        jal     KeLowerIrql             // lower IRQL to previous level
        move    v0,s0                   // set return value
        lw      s0,SyS0(sp)             // restore integer register s0
        lw      ra,SyRa(sp)             // restore return address
        addu    sp,sp,SyFrameLength     // deallocate stack frame
        j       ra                      // return

        .end    KeSynchronizeExecution

        SBTTL("Chained Dispatch")
//++
//
// Routine Description:
//
//    This routine is entered as the result of an interrupt being generated
//    via a vector that is connected to more than one interrupt object. Its
//    function is to walk the list of connected interrupt objects and call
//    each interrupt service routine. If the mode of the interrupt is latched,
//    then a complete traversal of the chain must be performed. If any of the
//    routines require saving the volatile floating point machine state, then
//    it is only saved once.
//
//    N.B. On entry to this routine only the volatile integer registers have
//       been saved.
//
// Arguments:
//
//    a0 - Supplies a pointer to the interrupt object.
//
//    s8 - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

        .struct 0
ChArg:  .space  4 * 4                   // argument register save area
ChS0:   .space  4                       // saved integer registers s0 - s6
ChS1:   .space  4                       //
ChS2:   .space  4                       //
ChS3:   .space  4                       //
ChS4:   .space  4                       //
ChS5:   .space  4                       //
ChS6:   .space  4                       //
ChRa:   .space  4                       // saved return address
ChFrameLength:                          // length of stack frame
ChIrql: .space  4                       // saved IRQL value

        NESTED_ENTRY(KiChainedDispatch, ChFrameLength, zero)

        subu    sp,sp,ChFrameLength     // allocate stack frame
        sw      ra,ChRa(sp)             // save return address
        sw      s0,ChS0(sp)             // save integer registers s0 - s6
        sw      s1,ChS1(sp)             //
        sw      s2,ChS2(sp)             //
        sw      s3,ChS3(sp)             //
        sw      s4,ChS4(sp)             //
        sw      s5,ChS5(sp)             //

#if defined(R4000) && !defined(NT_UP)

        sw      s6,ChS6(sp)             //

#endif

        PROLOGUE_END

//
// Initialize loop variables.
//

        addu    s0,a0,InInterruptListEntry // set address of listhead
        move    s1,s0                   // set address of first entry
        move    s2,zero                 // clear floating state saved flag
        lbu     s3,InMode(a0)           // get mode of interrupt
        lbu     s4,InIrql(a0)           // get interrupt source IRQL

//
// Walk the list of connected interrupt objects and call the respective
// interrupt service routines.
//

10:     subu    a0,s1,InInterruptListEntry // compute interrupt object address
        lbu     t0,InFloatingSave(a0)   // get floating save flag
        bne     zero,s2,20f             // if ne, floating state already saved
        beq     zero,t0,20f             // if eq, don't save floating state

//
// Save volatile floating registers f0 - f19 in trap frame.
//

        SAVE_VOLATILE_FLOAT_STATE       // save volatile floating state

        li      s2,1                    // set floating state saved flag

//
// Raise IRQL to synchronization level if synchronization level is not
// equal to the interrupt source level.
//

20:     lbu     s5,InSynchronizeIrql(a0) // get synchronization IRQL
        beq     s4,s5,25f               // if eq, IRQL levels are the same
        move    a0,s5                   // set synchronization IRQL
        addu    a1,sp,ChIrql            // compute address to save IRQL
        jal     KeRaiseIrql             // raise to synchronization IRQL
        subu    a0,s1,InInterruptListEntry // recompute interrupt object address

//
//
// Acquire the service routine spin lock and call the service routine.
//

25:                                     //

#if defined(R4000) && !defined(NT_UP)

        lw      s6,InActualLock(a0)     // get address of spin lock
30:     ll      t1,0(s6)                // get current lock value
        move    t2,s6                   // set lock ownership value
        bne     zero,t1,30b             // if ne, spin lock owned
        sc      t2,0(s6)                // set spin lock owned
        beq     zero,t2,30b             // if eq, store conditional failed

#endif

        lw      t0,InServiceRoutine(a0) // get address of service routine
        lw      a1,InServiceContext(a0) // get service context
        jal     t0                      // call service routine

//
// Release the service routine spin lock.
//

#if defined(R4000) && !defined(NT_UP)

        sw      zero,0(s6)              // set spin lock not owned

#endif

//
// Lower IRQL to the interrupt source level if synchronization level is not
// the same as the interrupt source level.
//

        beq     s4,s5,35f               // if eq, IRQL levels are the same
        move    a0,s4                   // set interrupt source IRQL
        jal     KeLowerIrql             // lower to interrupt source IRQL

//
// Get next list entry and check for end of loop.
//

35:     lw      s1,LsFlink(s1)          // get next interrupt object address
        beq     zero,v0,40f             // if eq, interrupt not handled
        beq     zero,s3,50f             // if eq, level sensitive interrupt
40:     bne     s0,s1,10b               // if ne, not end of list

//
// Either the interrupt is level sensitive and has been handled or the end of
// the interrupt object chain has been reached. Check to determine if floating
// machine state needs to be restored.
//

50:     beq     zero,s2,60f             // if eq, floating state not saved

//
// Restore volatile floating registers f0 - f19 from trap frame.
//

        RESTORE_VOLATILE_FLOAT_STATE    // restore volatile floating state

//
// Restore integer registers s0 - s6, retrieve return address, deallocate
// stack frame, and return.
//

60:     lw      s0,ChS0(sp)             // restore integer registers s0 - s6
        lw      s1,ChS1(sp)             //
        lw      s2,ChS2(sp)             //
        lw      s3,ChS3(sp)             //
        lw      s4,ChS4(sp)             //
        lw      s5,ChS5(sp)             //

#if defined(R4000) && !defined(NT_UP)

        lw      s6,ChS6(sp)             //

#endif

        lw      ra,ChRa(sp)             // restore return address
        addu    sp,sp,ChFrameLength     // deallocate stack frame
        j       ra                      // return

        .end    KiChainedDispatch

        SBTTL("Floating Dispatch")
//++
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
//    a0 - Supplies a pointer to the interrupt object.
//
//    s8 - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

        .struct 0
FlArg:  .space  4 * 4                   // argument register save area
FlS0:   .space  4                       // saved integer registers s0 - s1
FlS1:   .space  4                       //
FlIrql: .space  4                       // saved IRQL value
FlRa:   .space  4                       // saved return address
FlFrameLength:                          // length of stack frame

        NESTED_ENTRY(KiFloatingDispatch, FlFrameLength, zero)

        subu    sp,sp,FlFrameLength     // allocate stack frame
        sw      ra,FlRa(sp)             // save return address
        sw      s0,FlS0(sp)             // save integer registers s0 - s1

#if defined(R4000) && !defined(NT_UP)

        sw      s1,FlS1(sp)             //

#endif

        PROLOGUE_END

//
// Save volatile floating registers f0 - f19 in trap frame.
//

        SAVE_VOLATILE_FLOAT_STATE       // save volatile floating state

//
// Raise IRQL to synchronization level if synchronization level is not
// equal to the interrupt source level.
//

        move    s0,a0                   // save address of interrupt object
        lbu     a0,InSynchronizeIrql(s0) // get synchronization IRQL
        lbu     t0,InIrql(s0)           // get interrupt source IRQL
        beq     a0,t0,10f               // if eq, IRQL levels are the same
        addu    a1,sp,FlIrql            // compute address to save IRQL
        jal     KeRaiseIrql             // raise to synchronization IRQL
10:     move    a0,s0                   // restore address of interrupt object

//
//
// Acquire the service routine spin lock and call the service routine.
//

#if defined(R4000) && !defined(NT_UP)

        lw      s1,InActualLock(a0)     // get address of spin lock
20:     ll      t1,0(s1)                // get current lock value
        move    t2,s1                   // set lock ownership value
        bne     zero,t1,20b             // if ne, spin lock owned
        sc      t2,0(s1)                // set spin lock owned
        beq     zero,t2,20b             // if eq, store conditional failed

#endif

        lw      t0,InServiceRoutine(a0) // get address of service routine
        lw      a1,InServiceContext(a0) // get service context
        jal     t0                      // call service routine

//
// Release the service routine spin lock.
//

#if defined(R4000) && !defined(NT_UP)

        sw      zero,0(s1)              // set spin lock not owned

#endif

//
// Lower IRQL to the interrupt source level if synchronization level is not
// the same as the interrupt source level.
//

        lbu     a0,InIrql(s0)           // get interrupt source IRQL
        lbu     t0,InSynchronizeIrql(s0) // get synchronization IRQL
        beq     a0,t0,30f               // if eq, IRQL levels are the same
        jal     KeLowerIrql             // lower to interrupt source IRQL

//
// Restore volatile floating registers f0 - f19 from trap frame.
//

30:     RESTORE_VOLATILE_FLOAT_STATE    // restore volatile floating state

//
// Restore integer registers s0 - s1, retrieve return address, deallocate
// stack frame, and return.
//

        lw      s0,FlS0(sp)             // restore integer registers s0 - s1

#if defined(R4000) && !defined(NT_UP)

        lw      s1,FlS1(sp)             //

#endif

        lw      ra,FlRa(sp)             // restore return address
        addu    sp,sp,FlFrameLength     // deallocate stack frame
        j       ra                      // return

        .end    KiFloatingDispatch

        SBTTL("Interrupt Dispatch - Raise IRQL")
//++
//
// Routine Description:
//
//    This routine is entered as the result of an interrupt being generated
//    via a vector that is connected to an interrupt object. Its function is
//    to directly call the specified interrupt service routine.
//
//    N.B. On entry to this routine only the volatile integer registers have
//       been saved.
//
//    N.B. This routine raises the interrupt level to the synchronization
//       level specified in the interrupt object.
//
// Arguments:
//
//    a0 - Supplies a pointer to the interrupt object.
//
//    s8 - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

        .struct 0
RdArg:  .space  4 * 4                   // argument register save area
RdS0:   .space  4                       // saved integer register s0
        .space  4                       // fill
RdIrql: .space  4                       // saved IRQL value
RdRa:   .space  4                       // saved return address
RdFrameLength:                          // length of stack frame

        NESTED_ENTRY(KiInterruptDispatchRaise, RdFrameLength, zero)

        subu    sp,sp,RdFrameLength     // allocate stack frame
        sw      ra,RdRa(sp)             // save return address
        sw      s0,RdS0(sp)             // save integer register s0

        PROLOGUE_END

//
// Raise IRQL to synchronization level.
//

        move    s0,a0                   // save address of interrupt object
        lbu     a0,InSynchronizeIrql(s0) // get synchronization IRQL
        addu    a1,sp,RdIrql            // compute address to save IRQL
        jal     KeRaiseIrql             // raise to synchronization IRQL
        move    a0,s0                   // restore address of interrupt object

//
//
// Acquire the service routine spin lock and call the service routine.
//

#if defined(R4000) && !defined(NT_UP)

        lw      s0,InActualLock(a0)     // get address of spin lock
10:     ll      t1,0(s0)                // get current lock value
        move    t2,s0                   // set lock ownership value
        bne     zero,t1,10b             // if ne, spin lock owned
        sc      t2,0(s0)                // set spin lock owned
        beq     zero,t2,10b             // if eq, store conditional failed

#endif

        lw      t0,InServiceRoutine(a0) // get address of service routine
        lw      a1,InServiceContext(a0) // get service context
        jal     t0                      // call service routine

//
// Release the service routine spin lock.
//

#if defined(R4000) && !defined(NT_UP)

        sw      zero,0(s0)              // set spin lock not owned

#endif

//
// Lower IRQL to the previous level.
//

        lbu     a0,RdIrql(sp)           // get previous IRQL
        jal     KeLowerIrql             // lower to interrupt source IRQL

//
// Restore integer register s0, retrieve return address, deallocate
// stack frame, and return.
//

        lw      s0,RdS0(sp)             // restore integer registers s0 - s1
        lw      ra,RdRa(sp)             // restore return address
        addu    sp,sp,RdFrameLength     // deallocate stack frame
        j       ra                      // return

        .end    KiInterruptDispatchRaise

        SBTTL("Interrupt Dispatch - Same IRQL")
//++
//
// Routine Description:
//
//    This routine is entered as the result of an interrupt being generated
//    via a vector that is connected to an interrupt object. Its function is
//    to directly call the specified interrupt service routine.
//
//    N.B. On entry to this routine only the volatile integer registers have
//       been saved.
//
// Arguments:
//
//    a0 - Supplies a pointer to the interrupt object.
//
//    s8 - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

#if defined(NT_UP)

        LEAF_ENTRY(KiInterruptDispatchSame)

        lw      t0,InServiceRoutine(a0) // get address of service routine
        lw      a1,InServiceContext(a0) // get service context
        j       t0                      // jump to service routine

#else

        .struct 0
SdArg:  .space  4 * 4                   // argument register save area
SdS0:   .space  4                       // saved integer register s0
        .space  4 * 2                   // fill
SdRa:   .space  4                       // saved return address
SdFrameLength:                          // length of stack frame

        NESTED_ENTRY(KiInterruptDispatchSame, SdFrameLength, zero)

        subu    sp,sp,SdFrameLength     // allocate stack frame
        sw      ra,SdRa(sp)             // save return address
        sw      s0,SdS0(sp)             // save integer register s0

        PROLOGUE_END

//
//
// Acquire the service routine spin lock and call the service routine.
//

        lw      s0,InActualLock(a0)     // get address of spin lock
10:     ll      t1,0(s0)                // get current lock value
        move    t2,s0                   // set lock ownership value
        bne     zero,t1,10b             // if ne, spin lock owned
        sc      t2,0(s0)                // set spin lock owned
        beq     zero,t2,10b             // if eq, store conditional failed
        lw      t0,InServiceRoutine(a0) // get address of service routine
        lw      a1,InServiceContext(a0) // get service context
        jal     t0                      // call service routine

//
// Release the service routine spin lock.
//

        sw      zero,0(s0)              // set spin lock not owned

//
// Restore integer register s0, retrieve return address, deallocate
// stack frame, and return.
//

        lw      s0,SdS0(sp)             // restore integer registers s0 - s1
        lw      ra,SdRa(sp)             // restore return address
        addu    sp,sp,SdFrameLength     // deallocate stack frame
        j       ra                      // return

#endif

        .end    KiInterruptDispatchSame

        SBTTL("Interrupt Template")
//++
//
// Routine Description:
//
//    This routine is a template that is copied into each interrupt object. Its
//    function is to determine the address of the respective interrupt object
//    and then transfer control to the appropriate interrupt dispatcher.
//
//    N.B. On entry to this routine only the volatile integer registers have
//       been saved.
//
// Arguments:
//
//    a0 - Supplies a pointer to the interrupt template within an interrupt
//       object.
//
//    s8 - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiInterruptTemplate)

        .set    noreorder
        .set    noat
        lw      t0,InDispatchAddress - InDispatchCode(a0) // get dispatcher address
        subu    a0,a0,InDispatchCode    // compute address of interrupt object
        j       t0                      // transfer control to dispatch routine
        nop                             //
        .set    at
        .set    reorder

        .end    KiInterruptTemplate

        SBTTL("Unexpected Interrupt")
//++
//
// Routine Description:
//
//    This routine is entered as the result of an interrupt being generated
//    via a vector that is not connected to an interrupt object. Its function
//    is to report the error and dismiss the interrupt.
//
//    N.B. On entry to this routine only the volatile integer registers have
//       been saved.
//
// Arguments:
//
//    a0 - Supplies a pointer to the interrupt object.
//
//    s8 - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiUnexpectedInterrupt)

        j       ra                      // ****** temp ******

        .end    KiUnexpectedInterrupt
