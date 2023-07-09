//      TITLE("Interrupt Object Support Routines")
//++
//
// Copyright (c) 1990  Microsoft Corporation
// Copyright (c) 1992  Digital Equipment Corporation
//
// Module Name:
//
//    intsup.s
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
//    Joe Notarangelo 07-Apr-1992  (based on xxintsup.s by Dave Cutler)
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//--

#include "ksalpha.h"

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
SyS0:   .space  8                       // saved integer register s0
SyIrql: .space  4                       // saved IRQL value
        .space  4                       // fill for alignment
SyRa:   .space  8                       // saved return address
SyA0:   .space  8                       // saved argument registers a0 - a2
SyA1:   .space  8                       //
SyA2:   .space  8                       //
SyFrameLength:                          // length of stack frame

        NESTED_ENTRY(KeSynchronizeExecution, SyFrameLength, zero)

        lda     sp, -SyFrameLength(sp)  // allocate stack frame
        stq     ra, SyRa(sp)            // save return address
        stq     s0, SyS0(sp)            // save integer register s0

        PROLOGUE_END

        stq     a1, SyA1(sp)            // save synchronization routine address
        stq     a2, SyA2(sp)            // save synchronization routine context

//
// Raise IRQL to the synchronization level and acquire the associated
// spin lock.
//

#if !defined(NT_UP)

        LDP     s0, InActualLock(a0)    // get address of spin lock

#endif

        ldq_u   t1, InSynchronizeIrql(a0) // get synchronization IRQL
	lda	t2, InSynchronizeIrql(a0) //
        extbl   t1, t2, a0              //

        SWAP_IRQL                       // raise IRQL to synchronization level

        stl     v0, SyIrql(sp)          // save old irql

#if !defined(NT_UP)

10:     LDP_L   t0, 0(s0)               // get current lock value
        bis     s0, zero, t1            // set lock ownership value
        bne     t0, 15f                 // if ne, spin lock owned
        STP_C   t1, 0(s0)               // set spin lock owned
        beq     t1, 15f                 // if eq, store conditional failed
        mb                              // synchronize memory access

#endif

//
// Call specified routine passing the specified context parameter.
//

        LDP     t5, SyA1(sp)            // get synchronize routine address
        LDP     a0, SyA2(sp)            // get synchronzie routine context
        jsr     ra, (t5)                // call routine

//
// Release spin lock, lower IRQL to its previous level, and return the value
// returned by the specified routine.
//

#if !defined(NT_UP)

        mb                              // synchronize memory access
        STP     zero, 0(s0)             // set spin lock not owned

#endif

        ldl     a0, SyIrql(sp)          // get saved IRQL
        extbl   a0, 0, a0               // this is a uchar
        bis     v0, zero, s0            // save return value

        SWAP_IRQL                       // lower IRQL to previous level

        bis     s0, zero, v0            // restore return value
        ldq     s0, SyS0(sp)            // restore s0
        ldq     ra, SyRa(sp)            // restore ra
        lda     sp, SyFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

//
// Attempt to acquire lock failed.
//

#if !defined(NT_UP)

15:     LDP     t0, 0(s0)               // read current lock value
        beq     t0, 10b                 // if lock available, retry spinlock
        br      zero, 15b               // spin in cache until lock available

#endif

        .end    KeSynchronizeExecution

        SBTTL("Dispatch Chained Interrupt")
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
//    s6/fp - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

        .struct 0
ChS0:   .space  8                       // saved integer registers s0 - s5
ChS1:   .space  8                       //
ChS2:   .space  8                       //
ChS3:   .space  8                       //
ChS4:   .space  8                       //
ChS5:   .space  8                       //
ChRa:   .space  8                       // saved return address
ChIrql: .space  4                       // saved IRQL value
ChSpinL: .space 4                       // address of spin lock
ChFrameLength:                          // length of stack frame

        NESTED_ENTRY(KiChainedDispatch, ChFrameLength, zero)

        lda     sp, -ChFrameLength(sp)  // allocate stack frame
        stq     ra, ChRa(sp)            // save return address
        stq     s0, ChS0(sp)            // save integer registers s0 - s6
        stq     s1, ChS1(sp)            //
        stq     s2, ChS2(sp)            //
        stq     s3, ChS3(sp)            //
        stq     s4, ChS4(sp)            //
        stq     s5, ChS5(sp)            //

        PROLOGUE_END

//
// Register usage:
//
//      s0 = address of listhead
//      s1 = address of current item in list
//      s2 = floating status saved flag
//      s3 = mode of interrupt
//      s4 = irql of interrupt source
//      s5 = synchronization level requested for current list item
//
// Initialize loop variables.
//

        ADDP    a0, InInterruptListEntry, s0 // set address of listhead
        bis     s0, zero, s1            // set address of first entry
        bis     zero, zero, s2          // clear floating state saved flag
        ldq_u   t0, InMode(a0)          // get mode of interrupt
	lda	t2, InMode(a0)          //
        extbl   t0, t2, s3              //
        ldq_u   t1, InIrql(a0)          // get interrupt source IRQL
	lda	t3, InIrql(a0)          //
        extbl   t1, t3, s4              //

//
// Walk the list of connected interrupt objects and call the respective
// interrupt service routines.
//

10:     SUBP    s1, InInterruptListEntry, a0 // compute interrupt object address
        ldq_u   t2, InFloatingSave(a0)  // get floating save flag
	lda	t1, InFloatingSave(a0)  //
        extbl   t2, t1, t0              //
        bne     s2, 20f                 // if ne, floating state already saved
        beq     t0, 20f                 // if eq, don't save floating state

//
// Save volatile floating registers in trap frame.
//

        bsr     ra, KiSaveVolatileFloatState // save floating state
        ldil    s2, 1                   // set floating state saved flag

//
// Raise IRQL to synchronization level if synchronization level is not
// equal to the interrupt source level.
//

20:     ldq_u   t1, InSynchronizeIrql(a0) // get synchronization IRQL
	lda	t2, InSynchronizeIrql(a0) //
        extbl   t1, t2, s5              //
        cmpeq   s4, s5, t0              // Check if synchronization equals source level?
        bne     t0, 25f                 // if ne[true], IRQL levels are same
        bis     s5, zero, a0            // set synchronization IRQL

        SWAP_IRQL                       // raise IRQL to synchronization level

        stl     v0, ChIrql(sp)          // save old IRQL
        SUBP    s1, InInterruptListEntry, a0 // recompute interrupt object address

//
//
// Acquire the service routine spin lock and call the service routine.
//

25:                                     //

#if !defined(NT_UP)

        LDP     t5, InActualLock(a0)    // get address of spin lock
30:     LDP_L   t1, 0(t5)               // get current lock value
        bis     t5, zero, t2            // set ownership value
        bne     t1, 35f                 // if ne, spin lock owned
        STP_C   t2, 0(t5)               // set spin lock owned
        beq     t2, 35f                 // if eq, store conditional failed
        mb                              // synchronize memory access
        STP     t5, ChSpinL(sp)         // save spin lock address

#endif

        LDP     t5, InServiceRoutine(a0) // get address of service routine
        LDP     a1, InServiceContext(a0) // get service context
        jsr     ra, (t5)                // call interrupt service routine

//
// Release the service routine spin lock.
//

#if !defined(NT_UP)

        LDP     t5, ChSpinL(sp)         // get address of spin lock
        mb                              // synchronize memory access
        STP     zero, 0(t5)             // set spin lock not owned

#endif

//
// Lower IRQL to the interrupt source level if synchronization level is not
// the same as the interrupt source level.
//

        cmpeq   s4, s5, t0              // check if synchronization equals source level
        bne     t0, 37f                 // if ne[true], IRQL levels are same
        bis     s4, zero, a0            // set interrupt source IRQL

        SWAP_IRQL                       // lower to interrupt source IRQL

//
// Get next list entry and check for end of loop.
//

37:     LDP     s1, LsFlink(s1)         // get next interrupt object address
        beq     v0, 40f                 // if eq, interrupt not handled
        beq     s3, 50f                 // if eq, level sensitive interrupt
40:     cmpeq   s0, s1, t0              // check if end of list
        beq     t0, 10b                 // if eq[false], not end of list

//
// Either the interrupt is level sensitive and has been handled or the end of
// the interrupt object chain has been reached. Check to determine if floating
// machine state needs to be restored.
//

50:     beq     s2, 60f                 // if eq, floating state not saved

//
// Restore volatile floating registers from trap frame.
//

        bsr     ra, KiRestoreVolatileFloatState // restore floating state

//
// Restore integer registers s0 - s5, retrieve return address, deallocate
// stack frame, and return.
//

60:     ldq     s0, ChS0(sp)            // restore integer registers s0 - s6
        ldq     s1, ChS1(sp)            //
        ldq     s2, ChS2(sp)            //
        ldq     s3, ChS3(sp)            //
        ldq     s4, ChS4(sp)            //
        ldq     s5, ChS5(sp)            //
        ldq     ra, ChRa(sp)            // restore return address
        lda     sp, ChFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

//
// Attempt to acquire spinlock failed.
//

#if !defined(NT_UP)

35:     LDP     t1, 0(t5)               // read current lock value
        beq     t1, 30b                 // if eq, lock available
        br      zero, 35b               // spin in cache until lock available

#endif

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
//    s6/fp - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

        .struct 0
FlS0:   .space  8                       // saved integer registers s0 - s1
FlS1:   .space  8                       //
FlIrql: .space  4                       // saved IRQL value
        .space  4                       // for alignment
FlRa:   .space  8                       // saved return address
FlFrameLength:                          // length of stack frame

        NESTED_ENTRY(KiFloatingDispatch, FlFrameLength, zero)

        lda     sp, -FlFrameLength(sp)  // allocate stack frame
        stq     ra, FlRa(sp)            // save return address
        stq     s0, FlS0(sp)            // save integer registers s0 - s1

#if !defined(NT_UP)

        stq     s1, FlS1(sp)            //

#endif

        PROLOGUE_END

//
// Save volatile floating registers f0 - f19 in trap frame.
//

        bsr     ra, KiSaveVolatileFloatState // save floating state

//
// Raise IRQL to synchronization level if synchronization level is not
// equal to the interrupt source level.
//

        bis     a0, zero, s0            // save address of interrupt object
        ldq_u   t2, InSynchronizeIrql(s0) // get synchronization IRQL
	lda	t1, InSynchronizeIrql(s0) //
        extbl   t2, t1, a0              //
        ldq_u   t3, InIrql(s0)          // get interrupt source IRQL
	lda	t4, InIrql(s0)          //
        extbl   t3, t4, t0              //
        cmpeq   a0, t0, t1              // check if synchronize equals source IRQL ?
        bne     t1, 10f                 // if ne[true], IRQL levels same

        SWAP_IRQL                       // raise IRQL to synchronization level

        stl     v0, FlIrql(sp)          // save old irql
10:     bis     s0, zero, a0            // restore address of interrupt object

//
//
// Acquire the service routine spin lock and call the service routine.
//

#if !defined(NT_UP)

        LDP     s1, InActualLock(a0)    // get address of spin lock
20:     LDP_L   t1, 0(s1)               // get current lock value
        bis     s1, s1, t2              // set ownership value
        bne     t1, 25f                 // if ne, spin lock owned
        STP_C   t2, 0(s1)               // set spin lock owned
        beq     t2, 25f                 // if eq, store conditional failed
        mb                              // synchronize memory access

#endif

        LDP     t5, InServiceRoutine(a0) // get address of service routine
        LDP     a1, InServiceContext(a0) // get service context
        jsr     ra, (t5)                // call service routine

//
// Release the service routine spin lock.
//

#if !defined(NT_UP)

        mb                              // synchronize memory access
        STP     zero, 0(s1)             // set spin lock not owned

#endif

//
// Lower IRQL to the interrupt source level if synchronization level is not
// the same as the interrupt source level.
//

        ldq_u   t3, InIrql(s0)         // get interrupt source IRQL
	lda	t1, InIrql(s0)         //
        extbl   t3, t1, a0             //
        ldq_u   t4, InSynchronizeIrql(s0) // get synchronization IRQL
	lda	t2, InSynchronizeIrql(s0) //
        extbl   t4, t2, t0              //
        cmpeq   a0, t0, t1              // check if synchronize equal source IRQL?
        bne     t1, 30f                 // if eq, IRQL levels are the same

        SWAP_IRQL                       // lower IRQL to interrupt source

//
// Restore volatile floating registers f0 - f19 from trap frame.
//

30:     bsr     ra, KiRestoreVolatileFloatState // restore floating state

//
// Restore integer registers s0 - s1, retrieve return address, deallocate
// stack frame, and return.
//

        ldq     s0, FlS0(sp)            // restore integer registers s0 - s1

#if !defined(NT_UP)

        ldq     s1, FlS1(sp)            //

#endif

        ldq     ra, FlRa(sp)            // restore return address
        lda     sp, FlFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

//
// Attempt to acquire spinlock failed.
//

#if !defined(NT_UP)

25:     LDP     t1, 0(s1)               // read current lock value
        beq     t1, 20b                 // if lock available, retry spinlock
        br      zero, 25b               // spin in cache until lock available

#endif

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
//    s6/fp - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

        .struct 0
        .space  8                       // insure octaword alignment
RdS0:   .space  8                       // saved integer registers s0
RdIrql: .space  4                       // saved IRQL value
        .space  4                       // for alignment
RdRa:   .space  8                       // saved return address
RdFrameLength:                          // length of stack frame

        NESTED_ENTRY(KiInterruptDispatchRaise, RdFrameLength, zero)

        lda     sp, -RdFrameLength(sp)  // allocate stack frame
        stq     ra, RdRa(sp)            // save return address
        stq     s0, RdS0(sp)            // save integer registers s0

        PROLOGUE_END

//
// Raise IRQL to synchronization level
//

        bis     a0, zero, s0            // save address of interrupt object
        ldq_u   t3, InSynchronizeIrql(s0) // get synchronization IRQL
	lda	t1, InSynchronizeIrql(s0) //
        extbl   t3, t1, a0              //

        SWAP_IRQL                       // raise IRQL to synchronization level

        stl     v0, RdIrql(sp)          // save old irql
        bis     s0, zero, a0            // restore address of interrupt object

//
//
// Acquire the service routine spin lock and call the service routine.
//

#if !defined(NT_UP)

        LDP     t3, InActualLock(a0)    // get address of actual lock
20:     LDP_L   t1, 0(t3)               // get current lock value
        bis     t3, t3, t2              // set lock ownership value
        bne     t1, 25f                 // if ne, spin lock owned
        STP_C   t2, 0(t3)               // set spin lock owned
        beq     t2, 25f                 // if eq, store conditional failed
        mb                              // synchronize memory access

#endif

        LDP     t5, InServiceRoutine(a0) // get address of service routine
        LDP     a1, InServiceContext(a0) // get service context
        jsr     ra, (t5)                // call service routine

//
// Release the service routine spin lock.
//

#if !defined(NT_UP)

        LDP     t2, InActualLock(s0)    // get address of actual lock
        mb                              // synchronize memory access
        STP     zero, 0(t2)             // set spin lock not owned

#endif

//
// Lower IRQL to the previous level.
//

        ldl     a0, RdIrql(sp)          // get previous IRQL

        SWAP_IRQL                       // lower to interrupt source IRQL

//
// Restore integer register s0, retrieve return address, deallocate
// stack frame, and return.
//

30:     ldq     s0, RdS0(sp)            // restore integer register s0
        ldq     ra, RdRa(sp)            // restore return address
        lda     sp, RdFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

//
// Attempt to acquire spinlock failed.

#if !defined(NT_UP)

25:     LDP     t1, 0(t3)               // read current lock value
        beq     t1, 20b                 // if lock available, retry spinlock
        br      zero, 25b               // spin in cache until lock available

#endif

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
//    s6/fp - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

#if defined(NT_UP)

        LEAF_ENTRY(KiInterruptDispatchSame)

        LDP     t5, InServiceRoutine(a0) // get address of service routine
        LDP     a1, InServiceContext(a0) // get service context
        jsr     zero, (t5)              // jump to service routine

#else

        .struct 0
        .space  8                       // insure octaword alignment
SdS0:   .space  8                       // saved integer registers s0
SdIrql: .space  4                       // saved IRQL value
        .space  4                       // for alignment
SdRa:   .space  8                       // saved return address
SdFrameLength:                          // length of stack frame

        NESTED_ENTRY(KiInterruptDispatchSame, SdFrameLength, zero)

        lda     sp, -SdFrameLength(sp)  // allocate stack frame
        stq     ra, SdRa(sp)            // save return address
        stq     s0, SdS0(sp)            // save integer registers s0

        PROLOGUE_END

//
//
// Acquire the service routine spin lock and call the service routine.
//

        LDP     t3, InActualLock(a0)    // get actual lock address
        bis     a0, zero, s0            // save interrupt object address
20:     LDP_L   t1, 0(t3)               // get current lock value
        bis     t3, t3, t2              // set lock ownership value
        bne     t1, 25f                 // if ne, spin lock owned
        STP_C   t2, 0(t3)               // set spin lock owned
        beq     t2, 25f                 // if eq, store conditional failed
        mb                              // synchronize memory access
        LDP     t5, InServiceRoutine(a0) // get address of service routine
        LDP     a1, InServiceContext(a0) // get service context
        jsr     ra, (t5)                // call service routine

//
// Release the service routine spin lock.
//

        LDP     t2, InActualLock(s0)    // get actual lock address
        mb                              // synchronize memory access
        STP     zero, 0(t2)             // set spin lock not owned

//
// Restore integer registers s0, retrieve return address, deallocate
// stack frame, and return.
//

30:     ldq     s0, SdS0(sp)            // restore integer register s0
        ldq     ra, SdRa(sp)            // restore return address
        lda     sp, SdFrameLength(sp)   // deallocate stack frame
        ret     zero, (ra)              // return

//
// Attempt to acquire spinlock failed.
//

25:     LDP     t1, 0(t3)               // read current lock value
        beq     t1, 20b                 // if lock available, retry spinlock
        br      zero, 25b               // spin in cache until lock available

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
//    s6/fp - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiInterruptTemplate)

        .set    noreorder
        .set    noat
        LDP     t5, InDispatchAddress - InDispatchCode(a0) // get dispatcher address
        SUBP    a0, InDispatchCode, a0  // compute address of interrupt object
        jmp     zero, (t5)              // transfer to dispatch routine
        bis     zero, zero, zero        // nop for alignment
        .set    at
        .set    reorder

        .end    KiInterruptTemplate

        SBTTL("Disable Interrupts")
//++
//
// Routine Description:
//
//     This routine disables interrupts on the current processor and
//     returns the previous state of the interrrupt enable bit.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     A boolean value, if true interrupts were previously turned on,
//     false indicates interrupts were previously off.
//
//--

        LEAF_ENTRY(KiDisableInterrupts)

        GET_CURRENT_PROCESSOR_STATUS_REGISTER // get current prcb address

        DISABLE_INTERRUPTS              // disable interrupts

        and     v0, PSR_IE_MASK, v0     // isolate interrupt enable
        srl     v0, PSR_IE, v0          // shift to bit 0
        ret     zero, (ra)              // return

        .end    KiDisableInterrupts

        SBTTL("Restore Interrupts")
//++
//
// Routine Description:
//
//     This routine enables interrupts according to the the previous
//     interrupt enable passed as input.
//
// Arguments:
//
//    a0 - Supplies previous interrupt enable state (returned by
//         KiDisableInterrupts)
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiRestoreInterrupts)

        beq     a0, 10f                 // if eq, then interrupts disabled

        ENABLE_INTERRUPTS               // enable interrupts

        ret     zero, (ra)              // return

10:     DISABLE_INTERRUPTS              // disable interrupts

        ret     zero, (ra)              // return

        .end    KiRestoreInterrupts

        SBTTL("Unexpected Interrupts")
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
//    N.B. - This routine relies upon a private convention with the
//           interrupt exception dispatcher that register t12 contains the
//           interrupt vector of the unexpected interrupt.  This convention
//           will only work if the first level dispatch causes the
//           unexpected interrupt.
//
// Arguments:
//
//    a0 - Supplies a pointer to the interrupt object.
//    t12 - Supplies the interrupt vector.
//
//    s6/fp - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

        .struct 0
        .space  8                       // filler for 16 byte alignment
UiRa:   .space  8                       // return address
UiFrameLength:                          // frame length

        NESTED_ENTRY(KiUnexpectedInterrupt, UiFrameLength, zero)

        lda     sp, -UiFrameLength(sp)  // allocate stack frame
        stq     ra, UiRa(sp)            // save return address

        PROLOGUE_END                    //

        ldil    a0, 0xfacefeed          // ****** temp ******
        bis     t12, zero, a1           // pass interrupt vector
        bis     zero, zero, a2          // zero remaining parameters
        bis     zero, zero, a3          //
        bis     zero, zero, a4          //
        bis     zero, zero, a5          //
        bsr     ra, KeBugCheckEx        // perform system crash

        .end    KiUnexpectedInterrupt

        SBTTL(KiPassiveRelease)
//++
//
// RoutineDescription:
//
//      KiPassiveRelease passively releases an interrupt that cannot/will not
//      be serviced at this time.  Or there is no reason to service it and
//      maybe this routine will never be called in a million years.
//
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

        LEAF_ENTRY( KiPassiveRelease )

        ret     zero, (ra)              // return

        .end    KiPassiveRelease
