//      TITLE("Interrupt Object Support Routines")
//++
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
//    Bernard Lint 20-Nov-1995
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//    Based on MIPS version (David N. Cutler (davec) 2-Apr-1990)
//
//--

#include "ksia64.h"
//
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

        NESTED_ENTRY(KeSynchronizeExecution)
        NESTED_SETUP(3,4,1,0)

        PROLOGUE_END

//
// Register aliases for entire procedure
//

        rOldIrql  = loc2                        // saved IRQL value
        rpSpinLock= loc3                        // address of spin lock

//
// Raise IRQL to the synchronization level and acquire the associated
// spin lock.
//

        ARGPTR    (a0)
        ARGPTR    (a1)
        add       t2 = InSynchronizeIrql, a0    // -> sync IRQL
        ;;
        ld1       t3 = [t2], InActualLock-InSynchronizeIrql
        ;;

#if !defined(NT_UP)

        LDPTR     (rpSpinLock, t2)              // get address of spin lock

#endif // !defined(NT_UP)


        GET_IRQL(rOldIrql)                      // save old IRQL
        SET_IRQL(t3)                            // raise IRQL to synchronization IRQL

#if !defined(NT_UP)

        ACQUIRE_SPINLOCK(rpSpinLock, rpSpinLock, Kse10)

#endif // !defined(NT_UP)

//
// Call specified routine passing the specified context parameter.
//

        ld8       t1 = [a1], PlGlobalPointer-PlEntryPoint
        ;;
        ld8       gp = [a1]
        mov       bt0 = t1                      // setup br
        mov       out0 = a2                     // get synchronize context
        br.call.sptk.many brp = bt0             // call routine

//
// Release spin lock, lower IRQL to its previous level, and return the value
// returned by the specified routine.
//

#if !defined(NT_UP)

        RELEASE_SPINLOCK(rpSpinLock)

#endif // !defined(NT_UP)

        SET_IRQL(rOldIrql)                      // lower IRQL to previous level
        NESTED_RETURN
        NESTED_EXIT(KeSynchronizeExecution)
//
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
//       been saved. Also the volatile lower floating point registers saved.
//
//    N.B. gp will be destroyed by the interrupt service routine; if this code
//         uses the gp of this module after the call, then it must save and
//         restore gp.
//
// Arguments:
//
//    a0 - Supplies a function pointer to the ISR (in the interrupt object
//         dispatch code).
//
//    a1 - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

        NESTED_ENTRY(KiChainedDispatch)
        NESTED_SETUP(2,7,2,0)

        PROLOGUE_END

//
// Register aliases
//

        rpSpinLock = loc2                       // pointer to spinlock
        rMode     = loc3                        // interrupt mode (level sensitive)
        rpEntry   = loc4                        // current list entry
        rIrql     = loc5                        // source interrupt IRQL
        rSirql    = loc6                        // new interrupt IRQL
        rpI1      = t0                          // temp pointer
        rpI2      = t1                          // temp pointer
        rpFptr    = t2                          // pointer to ISR fptr
        rpCtxt    = t3                          // pointer to service context
        rFptr     = t4                          // ISR fptr

        pLoop1    = pt1                         // do another loop
        pLoop2    = pt2                         // do another loop
        pNEqual   = ps0                         // true if source IRQL != sync IRQL

//
// Initialize loop variables.
//
         
        add       out0 = -InDispatchCode, a0    // out0 -> interrupt object
        ;;
        add       rpEntry = InInterruptListEntry, out0   // set address of listhead
        add       rpI1 = InMode, out0           // -> mode of interrupt
        add       rpI2 = InIrql, out0           // -> interrupt source IRQL
        ;;
        ld1       rMode = [rpI1]                // get mode of interrupt
        ld1       rIrql = [rpI2]                // get interrupt source IRQL

//
// Walk the list of connected interrupt objects and call the respective
// interrupt service routines.
//
// Raise IRQL to synchronization level if synchronization level is not
// equal to the interrupt source level.
//

Kcd_Loop:
        add       rpI1 = InSynchronizeIrql, out0
        ;;
        ld1       rSirql = [rpI1], InActualLock-InSynchronizeIrql
        ;;

        cmp.ne    pNEqual = rIrql, rSirql       // if ne, IRQL levels are 
                                                // not the same
        ;;
        PSET_IRQL(pNEqual, rSirql)              // raise to synchronization IRQL

//
//
// Acquire the service routine spin lock and call the service routine.
//

#if !defined(NT_UP)

        LDPTR     (rpSpinLock, rpI1)            // get address of spin lock

        ACQUIRE_SPINLOCK(rpSpinLock, rpSpinLock, Kcd_Lock)

#endif // !defined(NT_UP)

        add       rpFptr = InServiceRoutine, out0        // pointer to fptr
        add       rpCtxt = InServiceContext, out0 // pointer to service context
        ;;
        LDPTR     (rFptr, rpFptr)               // get fptr
        LDPTR     (out1, rpCtxt)                // get service context
        ;;
        ld8       t5 = [rFptr], PlGlobalPointer-PlEntryPoint
        ;;
        ld8       gp = [rFptr]
        mov       bt0 = t5                      // set br address
        br.call.sptk brp = bt0                  // call ISR

//
// Release the service routine spin lock.
//

#if !defined(NT_UP)

        RELEASE_SPINLOCK(rpSpinLock)

#endif

//
// Lower IRQL to the interrupt source level if synchronization level is not
// the same as the interrupt source level.
//

        PSET_IRQL(pNEqual,rIrql)

//
// Get next list entry and check for end of loop.
//

        add       rpI1 = LsFlink, rpEntry        // -> next entry
        ;;
        LDPTR     (rpEntry, rpI1)                // -> next interrupt object
        ;;

//
// Loop if (1) interrrupt not handled and not end of list or
//      if (2) interrupt handled, and not level sensistive, and not end of list
//

        cmp4.eq   pLoop1 = zero, zero            // initialize pLoop1
        cmp4.eq   pLoop2 = zero, zero            // initialize pLoop2
        add       out0 = -InInterruptListEntry, rpEntry  // -> next interrupt object
        ;;

        cmp4.eq.and pLoop1 = zero, v0            // if eq, interrupt not handled
        cmp.ne.and  pLoop1, pLoop2 = zero, rpEntry   // if ne, not end of list
(pLoop1) br.dptk   Kcd_Loop                      // loop to handle next enrty

        cmp4.ne.and pLoop2 = zero, v0            // if ne, interrupt handled
        cmp4.ne.and pLoop2 = zero, rMode         // if ne, not level sensitive
(pLoop2) br.dptk   Kcd_Loop                      // loop to handle next enrty

//
// Either the interrupt is level sensitive and has been handled or the end of
// the interrupt object chain has been reached.
//

        NESTED_RETURN
        NESTED_EXIT(KiChainedDispatch)

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
//       been saved. Also volatile lower floating point registers saved.
//
//    N.B. This routine raises the interrupt level to the synchronization
//       level specified in the interrupt object.
//
//    N.B. gp will be destroyed by the interrupt service routine; if this code
//         uses the gp of this module after the call, then it must save and
//         restore gp.
//
// Arguments:
//
//    a0 - Supplies a function pointer to the ISR (in the interrupt object
//         dispatch code).
//
//    a1 - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

        NESTED_ENTRY(KiInterruptDispatchRaise)
        NESTED_SETUP(2,4,2,0)

        PROLOGUE_END

//
// Register aliases
//

        rpSpinLock = loc2
        rSirql    = loc3                        // sync IRQL
        rpI1      = t1                          // temp pointer
        rpFptr    = t2                          // pointer to ISR function pointer
        rpCtxt    = t3                          // pointer to service context
        rFptr     = t4                          // ISR function pointer (plabel pointer)
         
//
// Raise IRQL to synchronization level.
//

        add       out0 = -InDispatchCode, a0    // out0 -> interrupt object
        add       rpI1 = InSynchronizeIrql-InDispatchCode, a0
        ;;

        ld1       rSirql = [rpI1], InActualLock-InSynchronizeIrql
        ;;
        SET_IRQL(rSirql)                        // raise to synchronization IRQL

//
//
// Acquire the service routine spin lock and call the service routine.
//

#if !defined(NT_UP)

        LDPTR     (rpSpinLock, rpI1)            // get address of spin lock
        ;;
        ACQUIRE_SPINLOCK(rpSpinLock, rpSpinLock, Kidr_Lock)

#endif // !defined(NT_UP)

        add       rpFptr = InServiceRoutine, out0 // pointer to fptr
        add       rpCtxt = InServiceContext, out0 // pointer to service context
        ;;
        LDPTR     (rFptr, rpFptr)               // get fptr
        LDPTR     (out1, rpCtxt)                // get service context
        ;;
        ld8       t5 = [rFptr], PlGlobalPointer-PlEntryPoint
        ;;
        ld8       gp = [rFptr]
        mov       bt0 = t5                      // set br address
        br.call.sptk brp = bt0                  // call ISR

//
// Release the service routine spin lock.
//

#if !defined(NT_UP)

        RELEASE_SPINLOCK(rpSpinLock)

#endif // !defined(NT_UP)

//
// IRQL lowered to the previous level in the external handler.
//

        NESTED_RETURN
        NESTED_EXIT(KiInterruptDispatchRaise)
//
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
//       been saved. Also the volatile lower float point registers.
//
//    N.B. gp will be destroyed by the interrupt service routine; if this code
//         uses the gp of this module after the call, then it must save and
//         restore gp.
//
// Arguments:
//
//    a0 - Supplies a function pointer to the ISR (in the interrupt object
//         dispatch code).
//
//    a1 - Supplies a pointer to a trap frame..
//
// Return Value:
//
//    None.
//
//--

#if defined(NT_UP)

        LEAF_ENTRY(KiInterruptDispatchSame)
//
// Register aliases
//

        rpFptr    = t2                          // pointer to ISR function pointer
        rFptr     = t4                          // ISR function pointer (plabel pointer)

        add       a0 = -InDispatchCode, a0      // a0 points to interrupt object
        ;;
        add       rpFptr = InServiceRoutine, a0 // -> service routine fptr
        add       t1 = InServiceContext, a0     // -> service context
        ;;
        LDPTR     (rFptr, rpFptr)               // service routine fptr
        LDPTR     (a1, t1)                      // service context
        ;;
        ld8       t5 = [rFptr], PlGlobalPointer-PlEntryPoint
        ;;
        ld8       gp = [rFptr]
        mov       bt0 = t5
        br.sptk   bt0                           // jump to service routine

//
// N.B.: Return to trap handler from ISR.
//

        LEAF_EXIT(KiInterruptDispatchSame)
#else

        NESTED_ENTRY(KiInterruptDispatchSame)
        NESTED_SETUP(2,3,2,0)

        PROLOGUE_END

//
// Register aliases
//

        rpSpinLock = loc2
        rpFptr    = t2                          // -> ISR function pointer
        rpCtxt    = t3                          // -> service context
        rFptr     = t4                          // ISR function pointer (plabel pointer)
//
//
// Acquire the service routine spin lock and call the service routine.
//

        add       out0 = -InDispatchCode, a0    // -> interrupt object
        ;;
        add       t1 = InActualLock, out0       // pointer to address of lock
        add       rpCtxt = InServiceContext,out0
        ;;

        LDPTR     (rpSpinLock, t1)              // get address of spin lock
        add       rpFptr = InServiceRoutine, out0 // pointer to fptr
        ;;
        ACQUIRE_SPINLOCK(rpSpinLock, rpSpinLock, Kids_Lock)

        LDPTR     (rFptr, rpFptr)               // get fptr
        LDPTR     (out1, rpCtxt)                // get service context
        ;;
        ld8       t5 = [rFptr], PlGlobalPointer-PlEntryPoint
        ;;
        ld8       gp = [rFptr]
        mov       bt0 = t5                      // set br address
        br.call.sptk brp = bt0                  // call ISR

//
// Release the service routine spin lock.
//

        RELEASE_SPINLOCK(rpSpinLock)

        NESTED_RETURN

        NESTED_EXIT(KiInterruptDispatchSame)
#endif   // !defined(NT_UP)

        SBTTL("Disable Interrupts")
//++
//
// BOOLEAN
// KiDisableInterrupts (
//    VOID
//    )
//
// Routine Description:
//
//    This function disables interrupts and returns whether interrupts
//    were previously enabled.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    A boolean value that determines whether interrupts were previously
//    enabled (TRUE) or disabled(FALSE).
//
//--

        LEAF_ENTRY(KiDisableInterrupts)

        DISABLE_INTERRUPTS(t0)                  // t0 = previous state
        ;;
        tbit.nz   pt0, pt1 = t0, PSR_I          // pt0 = 1, if enabled; pt1 = 1 if disabled
        ;;

(pt0)   mov       v0 = TRUE                     // set return value -- TRUE if enabled
(pt1)   mov       v0 = FALSE                    // FALSE if disabled

        LEAF_RETURN
        LEAF_EXIT(KiDisableInterrupts)

//++
//
// VOID
// KiRestoreInterrupts (
//    IN BOOLEAN Enable
//    )
//
// Routine Description:
//
//    This function restores the interrupt enable that was returned by
//    the disable interrupts function.
//
// Arguments:
//
//    Enable (a0) - Supplies the interrupt enable value.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiRestoreInterrupts)

        dep.z     t0 = a0, PSR_I, 1             // enable or disable based on input arg
        ;;
        RESTORE_INTERRUPTS(t0)

        LEAF_RETURN
        LEAF_EXIT(KiRestoreInterrupts)

//++
//
// VOID
// KiPassiveRelease (
//    VOID
//    )
//
// Routine Description:
//
//    This function is called when an interrupt has been passively released.
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiPassiveRelease)

        LEAF_RETURN 
        LEAF_EXIT(KiPassiveRelease)


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
//       been saved. Also the volatile lower float point registers.
//
// Arguments:
//
//    a0 - Supplies a function pointer to the ISR (in the interrupt object
//         dispatch code).
//
//    a1 - Supplies a pointer to a trap frame.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KiUnexpectedInterrupt)

        LEAF_RETURN
        LEAF_EXIT(KiUnexpectedInterrupt)


        LEAF_ENTRY(KiFloatingDispatch)

        LEAF_RETURN
        LEAF_EXIT(KiFloatingDispatch)

#ifdef notyet
//
// Not needed since we always save low floating point regs in TF
//

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
#endif // notyet
