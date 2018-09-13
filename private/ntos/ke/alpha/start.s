//      TITLE( "Start System" )
//++
//
// Copyright (c) 1992  Digital Equipment Corporation
//
// Module:
//
//    start.s
//
// Abstract:
//
//     This module implements the code necessary to iniitially start NT
//     on an alpha - it includes the routine that first receives control
//     when the loader executes the kernel.
//
// Author:
//
//     Joe Notarangelo  02-Apr-1992
//
// Environment:
//
//     Kernel Mode only.
//
// Revision History:
//
//--

#include "ksalpha.h"

//
// Define the total frame size.
//

#define TotalFrameLength (KERNEL_STACK_SIZE - (TrapFrameLength + \
                            ExceptionFrameLength) )

//
// Define variables to hold the address of the PCR and current thread
// on uniprocessor systems.
//

        .data

#ifdef NT_UP

        .globl  KiPcrBaseAddress
KiPcrBaseAddress:
        .quad   0 : 1

        .globl  KiCurrentThread
KiCurrentThread:
        .quad   0 : 1

#endif //NT_UP

        SBTTL( "System Startup" )
//++
//
// Routine Description:
//
//     This routine represents the final stage of the loader and is also
//     executed as each additional processor is brought online is a multi-
//     processor system. It is responsible for installing the loaded PALcode
//     image and transfering control kernel startup code.
//
//     N.B. This code assumes that the I-cache is coherent.
//
//     N.B. This routine does not execute in the context of the operating
//          system but instead executes in the context of the firmware
//          PAL environment.  This routine can only use those services
//          guaranteed to exist in the firmware.  The only PAL services
//          that can be counted on are: swppal, imb, and halt.
//
// Arguments:
//
//     a0 - Supplies pointer to loader parameter block.
//
// Return Value:
//
//     None.
//
//--

        .struct 0
SsRa:   .space  8                       // Save ra
        .space  8                       // for stack alignment
SsFrameLength:                          //

        NESTED_ENTRY(KiSystemStartup, SsFrameLength, ra)

        ALTERNATE_ENTRY(KiStartProcessor)

        lda     sp, -SsFrameLength(sp)  // allocate stack frame
        stq     ra, SsRa(sp)            // save return address

        PROLOGUE_END

//
// Save the loader block address in a register that is preserved during
// the pal swap.
//

        ldl     s0, LpbPcrPage(a0)      // get PCR page frame number
        LDIP    s1, KSEG0_BASE          // get kseg0 base address
        bis     a0, zero, s2            //
        ldl     s3, LpbPdrPage(s2)      // get pdr page number
        sll     s3, PAGE_SHIFT, s3      // compute virtual address of PDR
        bis     s3, s1, s3              //

//
// Swap PAL code and enter the kernel initialization routine.
//
//      a0 - Physical base address of PAL.
//      a1 - Page frame number of PCR.
//      a2 - The virtual address of the PDR (AXP64 systems only).
//      ra - Return address from PAL call.
//

        LDP     a0, LpbPalBaseAddress(s2) // get PAL base address
        sll     a0, 32 + 3, a0          // clear upper address bits
        srl     a0, 32 + 3, a0          //
        bis     s0, zero, a1            // set PCR page frame number
        bis     s3, zero, a2            // set level 1 page directory address
        lda     ra, KiStartupContinue   // set return address

        SWPPAL                          // swap PAL images

//
// Control should never get here!
//

        ldq     ra, SsRa(sp)            // Restore ra
        lda     sp, SsFrameLength(sp)   // Restore stack pointer
        ret     zero, (ra)              // shouldn't get here

        .end    KiSystemStartup

        SBTTL( "Startup Continue" )
//++
//
// Routine Description:
//
//     This routine is called when NT begins execution after loading the
//     Kernel environment from the PAL. It registers exception routines
//     and system values with the pal code, calls kernel initialization
//     and falls into the idle thread code.
//
// Arguments:
//
//     s0 - Supplies the PCR page frame number.
//
//     s1 - Supplies the base address of KSEG0.
//
//     s2 - Supplies a pointer to the loader parameter block.
//
//     s3 - Supplies the virtual address of the PDR.
//
// Return Value:
//
//     None.
//
//--

        LEAF_ENTRY(KiStartupContinue)

//
// Set kernel stack pointer and kernel global pointer from loader
// parameter block.
//

        LDP     sp, LpbKernelStack(s2)  // set kernel stack pointer
        LDP     gp, LpbGpBase(s2)       // set kernel global pointer

//
//  Initialize PAL values, sp, gp, pcr, pdr, initial thread.
//
// sp - Kernel stack pointer.
// gp - System global pointer.
// a0 - Page directory (PDR) address.
// a1 - Idle thread address.
// a2 - Idle thread Teb address.
// a3 - Panic stack address.
// a4 - Maximum kernel stack allocation size.
//

        bis     s3, zero, a0            // set level 1 page directory address
        LDP     a1, LpbThread(s2)       // get idle thread address
        bis     zero, zero, a2          // zero Teb for initial thread
        LDP     a3, LpbPanicStack(s2)   // get panic stack base
        ldil    a4, TotalFrameLength    // set maximum kernel stack size

        INITIALIZE_PAL

//
// Save copies of the per processor values in global variables for
// uniprocessor systems.
//

        sll     s0, PAGE_SHIFT, s0      // compute physical address of pcr
        bis     s0, s1, s0              // compute address of pcr

#ifdef NT_UP

        lda     t0, KiPcrBaseAddress    // save PCR address
        STP     s0, 0(t0)               //
        LDP     t1, LpbThread(s2)       // get idle thread address
        lda     t0, KiCurrentThread     // save idle thread address
        STP     t1, 0(t0)               //

#endif //NT_UP

#if 0

        lda     a0, BdSystemName        // set system name address
        bis     zero, zero, a1          // set system base address
        lda     a2, BdDebugOptions      // set debug options address
        bsr     ra, BdInitDebugger      // initialize special debugger
        bsr     ra, DbgBreakPoint       // break into debugger

#endif

//
// Register kernel exception entry points with the PALcode.
//

        lda     a0, KiPanicException    // bugcheck entry point
        ldil    a1, entryBugCheck       //

        WRITE_KERNEL_ENTRY_POINT        //

        lda     a0, KiGeneralException  // general exception entry point
        ldil    a1, entryGeneral        //

        WRITE_KERNEL_ENTRY_POINT        //

        lda     a0, KiMemoryManagementException // memory mgmt exception entry
        ldil    a1, entryMM             //

        WRITE_KERNEL_ENTRY_POINT        //

        lda     a0, KiInterruptException // interrupt exception entry point
        ldil    a1, entryInterrupt      //

        WRITE_KERNEL_ENTRY_POINT        //

        lda     a0, KiSystemServiceException // syscall entry point
        ldil    a1, entrySyscall        //

        WRITE_KERNEL_ENTRY_POINT        //

//
// Initialize fields in the pcr.
//

        ldil    t1, PCR_MINOR_VERSION   // get minor version
        ldil    t2, PCR_MAJOR_VERSION   // get major version
        stl     t1, PcMinorVersion(s0)  // store minor version number
        stl     t2, PcMajorVersion(s0)  // store major version number

        LDP     t0, LpbThread(s2)       // save idle thread in pcr
        STP     t0, PcIdleThread(s0)    //

        LDP     t0, LpbPanicStack(s2)   // save panic stack in pcr
        STP     t0, PcPanicStack(s0)    //

        ldl     t0, LpbProcessorType(s2) // save processor type in pcr
        stl     t0, PcProcessorType(s0) //

        ldl     t0, LpbProcessorRevision(s2) // save processor revision
        stl     t0, PcProcessorRevision(s0)  //

        ldl     t0, LpbPhysicalAddressBits(s2) // save physical address bits
        stl     t0, PcPhysicalAddressBits(s0)  //

        ldl     t0, LpbMaximumAddressSpaceNumber(s2) // save max asn
        stl     t0, PcMaximumAddressSpaceNumber(s0)  //

        ldl     t0, LpbFirstLevelDcacheSize(s2) // save first level dcache size
        stl     t0, PcFirstLevelDcacheSize(s0)  //

        ldl     t0, LpbFirstLevelDcacheFillSize(s2) // save dcache fill size
        stl     t0, PcFirstLevelDcacheFillSize(s0)  //

        ldl     t0, LpbFirstLevelIcacheSize(s2) // save first level icache size
        stl     t0, PcFirstLevelIcacheSize(s0)  //

        ldl     t0, LpbFirstLevelIcacheFillSize(s2) // save icache fill size
        stl     t0, PcFirstLevelIcacheFillSize(s0)  //

        ldl     t0, LpbSystemType(s2)   // save system type
        stl     t0, PcSystemType(s0)    //
        ldl     t0, LpbSystemType+4(s2) //
        stl     t0, PcSystemType+4(s0)  //

        ldl     t0, LpbSystemVariant(s2) // save system variant
        stl     t0, PcSystemVariant(s0) //

        ldl     t0, LpbSystemRevision(s2) // save system revision
        stl     t0, PcSystemRevision(s0) //

        ldl     t0, LpbSystemSerialNumber(s2) // save system serial number
        stl     t0, PcSystemSerialNumber(s0) //
        ldl     t0, LpbSystemSerialNumber+4(s2) //
        stl     t0, PcSystemSerialNumber+4(s0) //
        ldl     t0, LpbSystemSerialNumber+8(s2) //
        stl     t0, PcSystemSerialNumber+8(s0) //
        ldl     t0, LpbSystemSerialNumber+12(s2) //
        stl     t0, PcSystemSerialNumber+12(s0) //

        ldl     t0, LpbCycleClockPeriod(s2) // save cycle counter period
        stl     t0, PcCycleClockPeriod(s0)  //

        LDP     t0, LpbRestartBlock(s2) // save Restart Block address
        STP     t0, PcRestartBlock(s0)  //

        ldq     t0, LpbFirmwareRestartAddress(s2) // save firmware restart
        stq     t0, PcFirmwareRestartAddress(s0) //

        ldl     t0, LpbFirmwareRevisionId(s2) // save firmware revision
        stl     t0, PcFirmwareRevisionId(s0) //

        LDP     t0, LpbDpcStack(s2)     // save Dpc Stack address
        STP     t0, PcDpcStack(s0)      //

        LDP     t0, LpbPrcb(s2)         // save Prcb address
        STP     t0, PcPrcb(s0)          //

        stl     zero, PbDpcRoutineActive(t0) // clear DPC Active flag
        stl     zero, PcMachineCheckError(s0) // indicate no HAL mchk handler

//
// Set system service dispatch address limits used by get and set context.
//

        lda     t0, KiSystemServiceDispatchStart // set start address of range
        STP     t0, PcSystemServiceDispatchStart(s0) //
        lda     t0, KiSystemServiceDispatchEnd // set end address of range
        STP     t0, PcSystemServiceDispatchEnd(s0) //

//
// Initialize the system.
//

        LDP     a0, LpbProcess(s2)      // get idle process address
        LDP     a1, LpbThread(s2)       // get idle thread address
        bis     a1, zero, s1            // save idle thread address
        LDP     a2, LpbKernelStack(s2)  // get idle thread stack
        LDP     a3, LpbPrcb(s2)         // get processor block address
        bis     a3, zero, s0            // save processor block address
        LoadByte(a4, PbNumber(a3))      // get processor number
        bis     s2, zero, a5            // get loader parameter block
        bsr     ra, KiInitializeKernel  // initialize system data

//
// Set the wait IRQL of the idle thread, and lower IRQL to DISPATCH_LEVEL.
//

        ldil    a0, DISPATCH_LEVEL      // get dispatch level IRQL
        StoreByte(a0, ThWaitIrql(s1))   // set wait IRQL of idle thread
        bsr     ra, KeLowerIrql         // lower IRQL

        ENABLE_INTERRUPTS               // enable interrupts

        bis     zero, zero, ra          // set bogus RA to stop debugger
        br      zero, KiIdleLoop        // continue in idle loop

        .end    KiStartupContinue

//
// The following code represents the idle loop for all processors. The idle
// loop executes at DISPATCH_LEVEL and continually polls for work.
//

        NESTED_ENTRY(KiIdleLoop, ExceptionFrameLength, zero)

        lda     sp, -ExceptionFrameLength(sp) // allocate context frame
        stq     ra, ExIntRa(sp)         // set bogus RA to stop debugger

        PROLOGUE_END

        lda     t0, KiIdleReturn        // set swap context return address
        stq     t0, ExSwapReturn(sp)    //

//
// Lower IRQL back to DISPATCH_LEVEL and restore global register values.
//
// N.B. The address of the current processor block (s0) is preserved across
//      the switch from idle call.
//

KiIdleReturn:                           //
        ldil    a0, DISPATCH_LEVEL      // set IRQL to dispatch level

        SWAP_IRQL                       //

#if DBG

        bis     zero, zero, s2          // reset breakin loop counter

#endif

        lda     s3, PbDpcListHead(s0)   // get DPC listhead address

#if !defined(NT_UP)

        ldil    s4, LockQueueDispatcherLock * 2 // compute per processor
        SPADDP  s4, s0, s4              // lock queue entry address
        lda     s4, PbLockQueue(s4)     //
        lda     s5, KiDispatcherLock    // get address of dispatcher lock

#endif

//
// Continually scan for debugger break in, a nonempty DPC list, or a new
// thread ready for execution.
//

IdleLoop:                               //

#if DBG

        subl    s2, 1, s2               // decrement breakin loop counter
        bge     s2, 5f                  // if ge, not time for breakin check
        ldil    s2, 200 * 1000          // set breakin loop counter
        bsr     ra, KdPollBreakIn       // check if breakin is requested
        beq     v0, 5f                  // if eq, then no breakin requested
        lda     a0, DBG_STATUS_CONTROL_C //
        bsr     ra, DbgBreakPointWithStatus //
5:                                      //

#endif //DBG

//
// Disable interrupts and check if there is any work in the DPC list
// of the current processor or a target processor.
//

CheckDpcList:                           //

        ENABLE_INTERRUPTS               // give interrupts a chance

        DISABLE_INTERRUPTS              // to interrupt spinning

//
// Process the deferred procedure call list for the current processor.
//

        ldl     t0, PbDpcQueueDepth(s0) // get current queue depth
        beq     t0, CheckNextThread     // if eq, DPC list is empty

//
// Clear dispatch interrupt.
//

        ldil    a0, DISPATCH_LEVEL      //clear any pending software interrupts
        ldl     t0, PbSoftwareInterrupts(s0) //
        bic     t0, a0, t1
        stl     t1, PbSoftwareInterrupts(s0) //

        DEASSERT_SOFTWARE_INTERRUPT     // clear any PAL-requested interrupts.

        bsr     ra, KiRetireDpcList     // process the DPC list

#if DBG

        bis     zero, zero, s2          // clear breakin loop counter

#endif

//
// Check if a thread has been selected to run on this processor.
//

CheckNextThread:                        //
        LDP     a0, PbNextThread(s0)    // get address of next thread object
        beq     a0, IdleProcessor       // if eq, no thread selected

//
// A thread has been selected for execution on this processor. Acquire
// dispatcher database lock, get the thread address again (it may have
// changed), clear the address of the next thread in the processor block,
// and call swap context to start execution of the selected thread.
//
// N.B. If the dispatcher database lock cannot be obtained immediately,
//      then attempt to process another DPC rather than spinning on the
//      dispatcher database lock.
//
// N.B. This is a very special acquire of the dispatcher lock in that it
//      will not be acquired unless it is free. Therefore, it is known
//      that there cannot be any queued lock requests.
//

#if !defined(NT_UP)

130:    LDP_L   t0, 0(s5)               // get current lock value
        bis     s4, zero, t1            // set lock ownership value
        bne     t0, CheckDpcList        // if ne, spin lock owned
        STP_C   t1, 0(s5)               // set spin lock owned
        beq     t1, 135f                // if eq, store conditional failed
        mb                              // synchronize reads after acquire
        bis     s5, LOCK_QUEUE_OWNER, t0 // set lock owner bit in lock entry
        STP     t0, LqLock(s4)          //

#endif

//
// Raise IRQL to sync level and re-enable interrupts
//

        ldl     a0, KiSynchIrql         //

        SWAP_IRQL                       //

        ENABLE_INTERRUPTS               //

        LDP     s2, PbNextThread(s0)    // get address of next thread
        LDP     s1, PbIdleThread(s0)    // get address of current thread
        STP     zero, PbNextThread(s0)  // clear next thread address
        STP     s2, PbCurrentThread(s0) // set address of current thread

//
// Set new thread's state to running. Note this must be done under the
// dispatcher lock so that KiSetPriorityThread sees the correct state.
//

        ldil    t0, Running             // set thread state to running
        StoreByte(t0, ThState(s2))      //

//
// Acquire the context swap lock so the address space of the old thread
// cannot be deleted and then release the dispatcher database lock. In
// this case the old thread is the idle thread, but the context swap code
// releases the context swap lock so it must be acquired.
//
// N.B. This lock is used to protect the address space until the context
//    switch has sufficiently progressed to the point where the address
//    space is no longer needed. This lock is also acquired by the reaper
//    thread before it finishes thread termination.
//

#if !defined(NT_UP)

        ldil    a0, LockQueueContextSwapLock * 2 // compute per processor
        SPADDP  a0, s0, a0              // lock queue entry address
        lda     a0, PbLockQueue(a0)     //
        bsr     ra, KiAcquireQueuedSpinLock // acquire context swap lock
        bis     s4, zero, a0                // set lock queue endtry address
        bsr     ra, KiReleaseQueuedSpinLock // release dispatcher lock

#endif

//
// Swap context to the new thread from the idle thread.
//
//  N.B. Control returns directly from this call to the top of the idle
//       loop.
//

        bsr     ra, SwapFromIdle        // swap context to new thread
        br      zero, KiIdleReturn      // control should not reach here

//
// There are no entries in the DPC list and a thread has not been selected
// for execution on this processor. Call the HAL so power management can
// be performed.
//
//  N.B. The HAL is called with interrupts disabled. The HAL will return
//       with interrupts enabled.
//

IdleProcessor:                          //
        lda     ra, IdleLoop            // set return address
        
        lda     a0, PbPowerState(s0)    // Get the Pointer to the current
                                        // C State Handler and Jump to it.
                                        // The Handler that gets called expects
                                        // A0 to contain the PState pointer.

        LDP     t0, PpIdleFunction(a0)
        jmp     zero, (t0)              //

//
// Conditional store of dispatcher lock failed. Retry. Do not spin in cache
// here. If the lock is owned, we want to check the DPC list again.
//

#if !defined(NT_UP)

135:    ENABLE_INTERRUPTS               // enable interrupts

        DISABLE_INTERRUPTS              // disable interrupts

        br      zero, 130b              // try again

#endif

        .end    KiIdleLoop

        SBTTL("Retire Deferred Procedure Call List")
//++
//
// Routine Description:
//
//    This routine is called to retire the specified deferred procedure
//    call list.
//
//    N.B. Interrupts must be disabled entry to this routine. Control is
//         returned to the caller with the same conditions true.
//
// Arguments:
//
//    s0 - Address of the processor control block.
//
// Return value:
//
//    None.
//
//--

        .struct 0
DpRa:   .space  8                       // return address
        .space  8                       // fill
DpcFrameLength:                         // frame length

        NESTED_ENTRY(KiRetireDpcList, DpcFrameLength, zero)

        lda     sp, -DpcFrameLength(sp) // allocate stack frame
        stq     ra, DpRa(sp)            // save return address

        PROLOGUE_END

//
// Process the DPC list.
//

10:     ldl     t0, PbDpcQueueDepth(s0) // get current DPC queue depth
        beq     t0, 60f                 // if eq, list is empty
15:     stl     t0, PbDpcRoutineActive(s0) // set DPC routine active
        lda     t2, PbDpcListHead(s0)   // compute DPC list head address

#if !defined(NT_UP)

20:     LDP_L   t1, PbDpcLock(s0)       // get current lock value
        bis     s0, zero, t3            // set lock ownership value
        bne     t1, 25f                 // if ne, spin lock owned
        STP_C   t3, PbDpcLock(s0)       // set spin lock owned
        beq     t3, 25f                 // if eq, store conditional failed
        mb                              // synchronize memory access
        ldl     t0, PbDpcQueueDepth(s0) // get current DPC queue depth
        beq     t0, 50f                 // if eq, DPC list is empty

#endif

        LDP     a0, LsFlink(t2)         // get address of next entry
        LDP     t1, LsFlink(a0)         // get address of next entry
        lda     a0, -DpDpcListEntry(a0) // compute address of DPC object
        STP     t1, LsFlink(t2)         // set address of next in header
        STP     t2, LsBlink(t1)         // set address of previous in next
        LDP     a1, DpDeferredContext(a0) // get deferred context argument
        LDP     a2, DpSystemArgument1(a0) // get first system argument
        LDP     a3, DpSystemArgument2(a0) // get second system argument
        LDP     t1, DpDeferredRoutine(a0) // get deferred routine address
        STP     zero, DpLock(a0)        // clear DPC inserted state
        subl    t0, 1, t0               // decrement DPC queue depth
        stl     t0, PbDpcQueueDepth(s0) // update DPC queue depth

#if DBG

        stl     zero, PbDebugDpcTime(s0) // clear the time spent in dpc

#endif

#if !defined(NT_UP)

        mb                              // synchronize previous writes
        STP     zero, PbDpcLock(s0)     // set spinlock not owned

#endif
        ENABLE_INTERRUPTS               // enable interrupts

        jsr     ra, (t1)                // call DPC routine

        DISABLE_INTERRUPTS              // disable interrupts

        br      zero, 10b               //

//
// Unlock DPC list and clear DPC active.
//

50:                                     //

#if !defined(NT_UP)

        mb                              // synchronize previous writes
        STP     zero, PbDpcLock(s0)     // set spin lock not owned

#endif

60:     stl     zero, PbDpcRoutineActive(s0) // clear DPC routine active
        stl     zero, PbDpcInterruptRequested(s0) // clear DPC interrupt requested

//
// Check one last time that the DPC list is empty. This is required to
// close a race condition with the DPC queuing code where it appears that
// a DPC routine is active (and thus an interrupt is not requested), but
// this code has decided the DPC list is empty and is clearing the DPC
// active flag.
//

#if !defined(NT_UP)

        mb                              //

#endif

        ldl     t0, PbDpcQueueDepth(s0) // get current DPC queue depth
        bne     t0, 65f                 // if ne, DPC list not empty
        ldq     ra, DpRa(sp)            // restore return address
        lda     sp, DpcFrameLength(sp)  // deallocate stack frame
        ret     zero, (ra)              // return

65:     br      zero, 15b               //

#if !defined(NT_UP)

25:     LDP     t1, PbDpcLock(s0)       // spin in cache until lock free
        beq     t1, 20b                 // retry spinlock
        br      zero, 25b               //

#endif

        .end    KiRetireDpcList

#if 0


        SBTTL("Initialize Traps")
//++
//
// Routine Description:
//
//    This function connects the PAL code to the boot debugger trap
//    routines.
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

        NESTED_ENTRY(BdInitializeTraps, 8, ra)

        lda     sp, -8(sp)              // allocate stack frame
        stq     ra, 0(sp)               // save return address

        lda     a0, BdGeneralException  // general exception entry point
        ldil    a1, entryGeneral        //

        WRITE_KERNEL_ENTRY_POINT        //

        lda     a0, BdMemoryManagementException // memory mgmt exception entry
        ldil    a1, entryMM             //

        WRITE_KERNEL_ENTRY_POINT        //

        ldq     ra, 0(sp)               // restore return address
        lda     sp, 8(sp)               // deallocate stack frame
        ret     zero, (ra)              // return

        .end    BdInitializeTraps

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

        NESTED_ENTRY(BdGeneralExceptionDispatch, TrapFrameLength, zero)

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

        ALTERNATE_ENTRY(BdGeneralException)

        bsr     ra, KiGenerateTrapFrame // store volatile state
        br      ra, BdExceptionDispatch // handle the exception

        .end    BdGeneralExceptionDispatch

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

        NESTED_ENTRY(BdExceptionDispatch, ExceptionFrameLength, zero )

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
        bis     zero, zero, a3          // set previous mode
        bis     fp, zero, a2            // set pointer to trap frame
        bis     sp, zero, a1            // set pointer to exception frame
        lda     a0, TrExceptionRecord(fp) // set address of exception record
        LDP     t0, BdDebugRoutine      // get address of debug routine
        jsr     ra, (t0)                // call kernel debugger
        ldq     s0, ExIntS0(sp)         // restore integer registers s0 - s5
        ldq     s1, ExIntS1(sp)         //
        ldq     s2, ExIntS2(sp)         //
        ldq     s3, ExIntS3(sp)         //
        ldq     s4, ExIntS4(sp)         //
        ldq     s5, ExIntS5(sp)         //
        ldl     a0, TrPsr(fp)           // get previous psr
        bsr     ra, KiRestoreNonVolatileFloatState // restore nv float state
        bsr     ra, KiRestoreTrapFrame  // restore volatile state
        bis     zero, zero, a1          // assume softwareinterrupt requested

//
// a0 = previous psr
// a1 = sfw interrupt requests
//

        RETURN_FROM_TRAP_OR_INTERRUPT   // return from exception

        .end    BdExceptionDispatch

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

        NESTED_ENTRY(BdMemoryManagementDispatch, TrapFrameLength, zero)

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

        ALTERNATE_ENTRY(BdMemoryManagementException)

        bsr     ra, KiGenerateTrapFrame // store volatile state
        STP     a0, TrExceptionRecord + ErExceptionInformation(fp) // set load/store

#if defined(_AXP64_)

        stq     a1, TrExceptionRecord + ErExceptionInformation + 8(fp) // set bad va

#else

        stl     a1, TrExceptionRecord + ErExceptionInformation + 4(fp) // set bad va

#endif

        lda     a0, TrExceptionRecord(fp) // get exception record address
        ldil    v0, STATUS_ACCESS_VIOLATION // get access violation code
        stl     v0, ErExceptionCode(a0) // save exception code
        stl     zero, ErExceptionFlags(a0) // set exception flags
        STP     zero, ErExceptionRecord(a0) // set associated record
        bis     zero, 2, t0             // set number of parameters
        stl     t0, ErNumberParameters(a0) // set number of parameters
        br      ra, BdExceptionDispatch // dispatch exception

        .end     BdMemoryManagementDispatch

#endif
