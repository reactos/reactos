//++
//
// Module Name:
//
//    miscs.s
//
// Abstract:
//
//    This module implements machine dependent miscellaneous kernel functions.
//    Functions are provided to request a software interrupt, continue thread
//    execution, flush TLBs and write buffers, and perform last chance 
//    exception processing.
//
// Author:
//
//    William K. Cheung (wcheung) 3-Nov-1995
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//    7-Jul-1997   bl    Updated to EAS2.3
//
//    27-Feb-1996  wc    Updated to EAS2.1
//
//    11-Jan-1996  wc    Set up register sp to point to the exception frame
//                       on the stack before calling KiSaveExceptionFrame and
//                       branching directly to KiExceptionExit.
//
//--

#include "ksia64.h"

//
// Global symbols
//

        PublicFunction(KiContinue)
        PublicFunction(KiSaveExceptionFrame)
        PublicFunction(KeTestAlertThread)
        PublicFunction(KiExceptionExit)
        PublicFunction(KiRaiseException)
        PublicFunction(KiNormalSystemCall)


//
//++
//
// NTSTATUS
// NtContinue (
//    IN PCONTEXT ContextRecord,
//    IN BOOLEAN TestAlert
//    )
//
// Routine Description:
//
//    This routine is called as a system service to continue execution after
//    an exception has occurred. Its functions is to transfer information from
//    the specified context record into the trap frame that was built when the
//    system service was executed, and then exit the system as if an exception
//    had occurred.
//
// Arguments:
//
//    ContextRecord (a0) - Supplies a pointer to a context record.
//
//    TestAlert (a1) - Supplies a boolean value that specifies whether alert
//       should be tested for the previous processor mode.
//
//    N.B. Register t0 is assumed to contain the address of a trap frame.
//
// Return Value:
//
//    Normally there is no return from this routine. However, if the specified
//    context record is misaligned or is not accessible, then the appropriate
//    status code is returned.
//
//--

        NESTED_ENTRY(NtContinue)

        NESTED_SETUP(2,4,3,0)
        .fframe   ExceptionFrameLength
        add       sp = -ExceptionFrameLength, sp

        PROLOGUE_END

//
// Transfer information from the context record to the exception and trap
// frames.
//

        add       loc3 = TrStIPSR, t0
        ;;

        ld8       loc3 = [loc3]
        mov       loc2 = t0
        mov       out0 = a0                     // context frame address

        add       out1 = STACK_SCRATCH_AREA, sp // -> exception frame
        mov       out2 = t0                     // trap frame address
        br.call.sptk.many brp = KiContinue

//
// If KiContinue() returns success, then exit via the exception exit code.
// Otherwise, return to the system service dispatcher.
//
// Check to determine if alert should be tested for the previous processor
// mode and restore the previous mode in the thread object.
//
// Application that invokes the NtContinue() system service must have
// flushed all stacked registers to the backing store.  Sanitize the
// bspstore to be equal to bsp; otherwise, some stacked GRs will not be
// restored from the backing store.
//

        cmp.ne    pt0 = zero, v0                // if ne, transfer failed.
        movl      t7 = 1 << PSR_TB | 1 << PSR_DB | 1 << PSR_SS
        ;;

        add       t0 = TrRsBSP, loc2
        add       t1 = TrRsBSPSTORE, loc2
 (pt0)  dep       t7 = 1, t7, PSR_LP, 1         // capture psr.lp if failed
        ;;

        ld8       t4 = [t0], TrStIPSR-TrRsBSP
        cmp4.ne   pt1 = zero, a1                // if ne, test for alert
        and       loc3 = t7, loc3               // capture psr.tb, db, ss bits
        ;;

        ld8       t7 = [t0]
        st8       [t1] = t4
        add       t2 = TrTrapFrame, loc2
        ;;

        add       t3 = TrPreviousMode, loc2
        or        t7 = loc3, t7
 (pt0)  br.cond.spnt Nc10                       // jump to Nc10 if pt0 is TRUE
        ;;

//
// Restore the nonvolatile machine state from the exception frame
// and exit the system via the exception exit code.
//

        ld8       t5 = [t2]                     // get old trap frame address
        movl      t1 = KiPcr + PcCurrentThread  // -> current thread
        ;;

        st8       [t0] = t7
        movl      t9 = KTRAP_FRAME_EOF | EXCEPTION_FRAME

        LDPTR     (t4,t1)                       // get current thread address
        ld4       t6 = [t3], TrEOFMarker-TrPreviousMode // get old previous mode
        ;;

        add       t7 = ThPreviousMode, t4
        add       t8 = ThTrapFrame, t4
        ;;

 (pt1)  ld1       out0 = [t7]                   // get current previous mode
        STPTR     (t8, t5)                      // restore old trap frame addr
        mov       a0 = loc2                     // pointer to trap frame

        st1       [t7] = t6                     // restore old previous mode
        st8.nta   [t3] = t9                     // change the trap frame marker
 (pt1)  br.call.spnt.many brp = KeTestAlertThread

//
// sp -> stack scratch area/FP save area/exception frame/trap frame
//
// Set up for branch to KiExceptionExit
//
//      s0 = trap frame
//      s1 = exception frame
//
// N.B. predicate register alias pUstk & pKstk must be the same as trap.s
//      and they must be set up correctly upon entry into KiExceptionExit.
//
// N.B. The exception exit code will restore the exception frame & trap frame
//      and then rfi to user code. pUstk is set to 1 while pKstk is set to 0.
//

        pUstk     = ps3
        pKstk     = ps4

        cmp.eq      pUstk, pKstk = zero, zero
        add         s1 = STACK_SCRATCH_AREA, sp
        mov         s0 = loc2
        br          KiExceptionExit

Nc10:

        .restore
        add         sp = ExceptionFrameLength, sp
        NESTED_RETURN

        NESTED_EXIT(NtContinue)

//++
//
// NTSTATUS
// NtRaiseException (
//    IN PEXCEPTION_RECORD ExceptionRecord,
//    IN PCONTEXT ContextRecord,
//    IN BOOLEAN FirstChance
//    )
//
// Routine Description:
//
//    This routine is called as a system service to raise an exception.
//    The exception can be raised as a first or second chance exception.
//
// Arguments:
//
//    ExceptionRecord (a0) - Supplies a pointer to an exception record.
//
//    ContextRecord (a1) - Supplies a pointer to a context record.
//
//    FirstChance (a2) - Supplies a boolean value that determines whether
//       this is the first (TRUE) or second (FALSE) chance for dispatching
//       the exception.
//
//    N.B. Register t0 is assumed to contain the address of a trap frame.
//
// Return Value:
//
//    Normally there is no return from this routine. However, if the specified
//    context record or exception record is misaligned or is not accessible,
//    then the appropriate status code is returned.
//
//--

        NESTED_ENTRY(NtRaiseException)

        NESTED_SETUP(3,3,5,0)
        .fframe   ExceptionFrameLength
        add       sp = -ExceptionFrameLength, sp
        ;;

        PROLOGUE_END

//
// Save nonvolatile states.
//

        add       out0 = STACK_SCRATCH_AREA, sp
        mov       loc2 = t0                   // save pointer to trap frame
        br.call.sptk brp = KiSaveExceptionFrame

//
// Call the raise exception kernel routine wich will marshall the argments
// and then call the exception dispatcher.
//

        add       out2 = STACK_SCRATCH_AREA, sp // -> exception frame
        mov       out1 = a1
        mov       out0 = a0

        add       out4 = zero, a2
        mov       out3 = t0
        br.call.sptk.many brp = KiRaiseException

//
// If the raise exception routine returns success, then exit via the exception
// exit code.  Otherwise, return to the system service dispatcher.
//
// N.B. The exception exit code will restore the exception frame & trap frame
//      and then rfi to user code.
//
// Set up for branch to KiExceptionExit
//
//      s0 = trap frame
//      s1 = exception frame
//

        pUstk     = ps3
        pKstk     = ps4


        cmp4.ne   p0, pt1 = zero, v0            // if ne, dispatch failed.
        ;;
(pt1)   mov       s0 = loc2                     // copy trap frame pointer
(pt1)   add       s1 = STACK_SCRATCH_AREA, sp

(pt1)   cmp.eq    pUstk, pKstk = zero, zero
(pt1)   br.cond.sptk.many KiExceptionExit

        .restore
        add         sp = ExceptionFrameLength, sp
        NESTED_RETURN

        NESTED_EXIT(NtRaiseException)


//++
//
// VOID
// KiSetProcessRid (
//    ULONG NewProcessRid
// )
//
// Routine Description:
//    This function load the new global and local RID into the region registers.
//      
// Arguments:
//
//    NewProcessRid (a0) - Supplies a local RID value to be loaded into region 0, and 1.
//
// Return Value:
//
//    None.
//
//-- 

        LEAF_ENTRY(KiSetProcessRid)

        rRRu = t0
        rRN0 = t1

        FAST_DISABLE_INTERRUPTS

        shl       rRRu = a0, RR_RID
        dep.z     rRN0 = UREGION_INDEX, RR_INDEX, RR_INDEX_LEN // index for rr 
        ;;
        or        rRRu = RR_PS_VE, rRRu        // set page size, enable VHPT
        ;;

        mov       rr[rRN0] = rRRu              // switch user space 0-1G
        ;;
        srlz.i
        ;;

        FAST_ENABLE_INTERRUPTS
        LEAF_RETURN

        LEAF_EXIT(KeSetProcessRid)


//++
//
// ULONG
// KeGetRid (
//    VOID
//    )
//
// Routine Description:
//
//    This function gets the region ID
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
         LEAF_ENTRY(KeGetRid)

         mov       t0 = UREGION_INDEX
         ;;
         mov       v0 = rr[t0]

         LEAF_RETURN
         LEAF_EXIT(KeGetRid)


//++
//
// VOID
// KeFillLargeEntryTb (
//    IN HARDWARE_PTE Pte[],
//    IN PVOID Virtual,
//    IN ULONG PageSize
//    )
//
// Routine Description:
//
//    This function fills a large translation buffer entry.
//
//    N.B. It is assumed that the large entry is not in the TB and therefore
//      the TB is not probed.
//
// Arguments:
//
//    Pte (a0) - Supplies a pointer to the page table entries that are to be
//       written into the TB.
//
//    Virtual (a1) - Supplies the virtual address of the entry that is to
//       be filled in the translation buffer.
//
//    PageSize (a2) - Supplies the size of the large page table entry.
//
// Return Value:
//
//    None.
//
//--

        LEAF_ENTRY(KeFillLargeEntryTb)

        
        rPte    = t0
        rScnd   = t1
        ridtr   = t2
        rid     = t3
        rDtr    = t4

        rTb             = t6
        rTbPFN          = t7
        rpAttr          = t8
        rAttrOffset     = t9


        ARGPTR  (a0)
        ARGPTR  (a1)

        shr.u   rScnd = a2, 6           // mask off page size fields
        ;;
        shl     rScnd = rScnd, 6        //         
        ;;
        and     rScnd = a1, rScnd       // examine the virtual address bit
        ;;
        cmp.eq  pt0, pt1 = r0, rScnd    
        shl     ridtr = a2, PS_SHIFT
        mov     rDtr = DTR_VIDEO_INDEX
        ;;

        rsm     1 << PSR_I              // turn off interrupts
(pt0)   add     a0 = (1 << PTE_SHIFT), a0
        ;;

        ld8     rPte = [a0]             // load PTE
        rsm     1 << PSR_IC             // interrupt is off, now reset PSR.ic
        ;;

        srlz.d                          // serialize
        ;;

        mov     cr.itir = ridtr         // idtr for insertion
        mov     cr.ifa = a1             // ifa for insertion
        ;;

        itr.d   dtr[rDtr] = rPte
        ssm     1 << PSR_IC             // set PSR.ic bit again
        ;;

        srlz.i                          // I serialize
        ;;
        ssm     1 << PSR_I

        LEAF_RETURN

        LEAF_EXIT(KeFillLargeEntryTb)         // return


//++
//
// VOID
// KeFillFixedEntryTb (
//    IN HARDWARE_PTE Pte[],
//    IN PVOID Virtual,
//    IN ULONG Index
//    )
//
// Routine Description:
//
//    This function fills a fixed translation buffer entry.
//
// Arguments:
//
//    Pte (a0) - Supplies a pointer to the page table entries that are to be
//       written into the TB.
//
//    Virtual (a1) - Supplies the virtual address of the entry that is to
//       be filled in the translation buffer.
//
//    Index (a2) - Supplies the index where the TB entry is to be written.
//
// Return Value:
//
//    None.
//
// Comments:
//
//    
//
//--
        LEAF_ENTRY(KeFillFixedEntryTb)

        
        rPte    = t0
        rScnd   = t1
        ridtr   = t2
        rid     = t3
        rDtr    = t4

        rTb             = t6
        rTbPFN          = t7
        rpAttr          = t8
        rAttrOffset     = t9


        ARGPTR  (a0)
        ARGPTR  (a1)

        rsm     1 << PSR_I              // reset PSR.i
        ld8     rPte = [a0]             // load PTE
        mov     ridtr = PAGE_SHIFT << PS_SHIFT
        ;;

        rsm     1 << PSR_IC             // interrupt is off, now reset PSR.ic
        ;;
        srlz.d                          // serialize
        zxt4    rDtr = a2
        ;;
        
        mov     cr.itir = ridtr         // idtr for insertion
        mov     cr.ifa = a1             // ifa for insertion
        ;;

        itr.d   dtr[rDtr] = rPte
        ssm     1 << PSR_IC             // set PSR.ic bit again
        ;;

        srlz.i                          // I serialize
        ;;
        ssm     1 << PSR_I

        LEAF_RETURN

        LEAF_EXIT(KeFillFixedEntryTb)


//++
//
// VOID
// KeFillFixedLargeEntryTb (
//    IN HARDWARE_PTE Pte[],
//    IN PVOID Virtual,
//    IN ULONG PageSize,
//    IN ULONG Index
//    )
//
// Routine Description:
//
//    This function fills a fixed translation buffer entry with a large page 
//    size.
//
// Arguments:
//
//    Pte (a0) - Supplies a pointer to the page table entries that are to be
//       written into the TB.
//
//    Virtual (a1) - Supplies the virtual address of the entry that is to
//       be filled in the translation buffer.
//
//    PageSize (a2) - Supplies the size of the large page table entry.
//
//    Index (a3) - Supplies the index where the TB entry is to be written.
//
// Return Value:
//
//    None.
//
// Comments:
//
//    Yet to be implemented.
//
//--
        LEAF_ENTRY(KeFillFixedLargeEntryTb)

        
        rPte    = t0
        rScnd   = t1
        ridtr   = t2
        rid     = t3
        rDtr    = t4

        rTb             = t6
        rTbPFN          = t7
        rpAttr          = t8
        rAttrOffset     = t9


        ARGPTR  (a0)
        ARGPTR  (a1)

        rsm     1 << PSR_I              // reset PSR.i
        ld8     rPte = [a0]             // load PTE
        shl     ridtr = a2, PS_SHIFT
        ;;

        rsm     1 << PSR_IC             // interrupt is off, now reset PSR.ic
        mov     rDtr = a3               // set the DTR index
        ;;

        srlz.d                          // serialize
        ;;
        mov     cr.itir = ridtr         // idtr for insertion
        mov     cr.ifa = a1             // ifa for insertion
        ;;

        itr.d   dtr[rDtr] = rPte
        ssm     1 << PSR_IC             // set PSR.ic bit again
        ;;

        srlz.i                          // I serialize
        ;;
        ssm     1 << PSR_I

        LEAF_RETURN

        LEAF_EXIT(KeFillFixedLargeEntryTb)         // return

//++
//
// VOID
// KeFillInstEntryTb (
//    IN HARDWARE_PTE Pte[],
//    IN PVOID Virtual,
//    )
//
// Routine Description:
//
//    This function fills a large translation buffer entry.
//
//    N.B. It is assumed that the large entry is not in the TB and therefore
//      the TB is not probed.
//
// Arguments:
//
//    Pte (a0) - Supplies a page table entry that is to be
//       written into the Inst TB.
//
//    Virtual (a1) - Supplies the virtual address of the entry that is to
//       be filled in the translation buffer.
//
// Return Value:
//
//    None.
//
//--
        LEAF_ENTRY(KeFillInstEntryTb)

        riitr   = t2
        rid     = t3

        ARGPTR  (a1)


        rsm     1 << PSR_I              // reset PSR.i
        ;;
        rsm     1 << PSR_IC             // interrupt is off, now reset PSR.ic
        mov     riitr = PAGE_SIZE << PS_LEN
        ;;

        srlz.d                          // serialize
        ;;
        mov     cr.ifa = a1             // set va to install
        mov     cr.itir = riitr         // iitr for insertion
        ;;

        itc.i   a0
        ssm     1 << PSR_IC             // set PSR.ic bit again
        ;;

        srlz.i                          // I serialize
        ;;
        ssm     1 << PSR_I

        LEAF_RETURN

        LEAF_EXIT(KeFillInstEntryTb)         // return


//++
//
// VOID
// KeBreakinBreakpoint
//    VOID
//    )
//
// Routine Description:
//
//    This function causes a BREAKIN breakpoint.
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
         LEAF_ENTRY(KeBreakinBreakpoint)

         //
         // Flush the RSE or the kernel debugger is unable to do a stack unwind
         //

         flushrs
         ;;
         break.i   BREAKIN_BREAKPOINT
         LEAF_RETURN

         LEAF_EXIT(KeBreakinBreakpoint)


//++
//
// VOID
// KeInitializeVhpt(
//    IN PVOID Virtual,
//    IN ULONG Size
// )
//
// Routine Description:
//
//    This function initializes the VHPT and enables the hardware walker.
// 
// Arguments:
// 
//    Virtual (a0) - Supplies the virtual address for the VHPT base address
//
//    Size    (a1) - Supplies the size for the VHPT table.
//
// Return Value:
//   
//    None
//
//--

        LEAF_ENTRY(KeInitializeVhpt)

        ARGPTR  (a0)
        shl     a1 = a1, 2
        mov     t0 = PTA_INITIAL 
        ;;
        or      a1 = t0, a1
        ;;
        or      a0 = a0, a1
        ;;
        mov     cr.pta = a0

        LEAF_RETURN
        LEAF_EXIT(KeInitializeVhpt)


#ifdef WX86


//++
//
// VOID
// KiIA32RegistersInit
//    VOID
//    )
//
// Routine Description:
//
//    This function to Initialize per processor IA32 related registers
//    These registers do not saved/restored on context switch time
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
        LEAF_ENTRY(KiIA32RegistersInit)

        mov         t0 = TeGdtDescriptor
        mov         iA32iobase = 0
        ;;
        mov         iA32index = t0

        LEAF_RETURN
        LEAF_EXIT(KiIA32RegistersInit)
#endif // WX86


//++
//
// PKTHREAD
// KeGetCurrentThread (VOID)
//
// Routine Description:
//
// Arguments:
//
//    None.
//
// Return Value:
//
//    Returns a pointer to the executing thread object.
//
//--

        LEAF_ENTRY(KeGetCurrentThread)

        movl    v0 = KiPcr + PcCurrentThread  // -> current thread
        ;;
  
        ld8     v0 = [v0]
        br.ret.sptk brp

        LEAF_EXIT(KeGetCurrentThread)

//++
//
// Routine Description:
//
//     This routine saves the thread's current non-volatile NPX state,
//     and sets a new initial floating point state for the caller.
//
//     This is intended for use by kernel-mode code that needs to use
//     the floating point registers. Must be paired with
//     KeRestoreFloatingPointState
//
// Arguments:
//
//     a0 - Supplies pointer to KFLOATING_SAVE structure
//
// Return Value:
//
//     None.
//
//--

        LEAF_ENTRY(KeSaveFloatingPointState)

        mov  v0 = zero
        LEAF_RETURN

        LEAF_EXIT(KeSaveFloatingPointState)

//++
//
// Routine Description:
//
//     This routine restores the thread's current non-volatile NPX state,
//     to the passed in state.
//
//     This is intended for use by kernel-mode code that needs to use
//     the floating point registers. Must be paired with
//     KeSaveFloatingPointState
//
// Arguments:
//
//     a0 - Supplies pointer to KFLOATING_SAVE structure
//
// Return Value:
//
//     None.
//
//--

        LEAF_ENTRY(KeRestoreFloatingPointState)

        mov  v0 = zero
        LEAF_RETURN

        LEAF_EXIT(KeRestoreFloatingPointState)


//++
//
// Routine Description:
//
//     This routine flush all the dirty registers to the backing store
//     and invalidate them.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     None.
//
//--
        LEAF_ENTRY(KiFlushRse)

        flushrs
        mov       t1 = ar.rsc
        mov       t0 = RSC_KERNEL_DISABLED
        ;;

        mov       ar.rsc = t0
        ;;
        loadrs
        ;;
        mov       ar.rsc = t1
        ;;
        br.ret.sptk brp
       
        LEAF_EXIT(KiFlushRse)

#if 0

//++
//
// Routine Description:
//
//     This routine invalidate all the physical stacked registers.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     None.
//
//--
        LEAF_ENTRY(KiInvalidateStackedRegisters)

        mov       t1 = ar.rsc
        mov       t0 = RSC_KERNEL_DISABLED
        ;;
        mov       ar.rsc = t0
        ;;
        loadrs
        ;;
        mov       ar.rsc = t1
        ;;
        br.ret.sptk brp

        LEAF_EXIT(KiInvalidateStackedRegisters)
#endif // 0


//++ 
//
// PVOID
// KiGetPhysicalAddress(
//     PVOID Virtual
//     )        
//
// Routine Description:
//
//      This routine translates to physical address uing TPA instruction.
//
// Arguments:
//
//      a0 - virtual address to be translated to physical address.
//
// Return Value:
//
//      physical address
//
//--

        LEAF_ENTRY(KiGetPhysicalAddress)

        tpa     r8 = a0
        LEAF_RETURN

        LEAF_EXIT(KiGetPhysicalAddress)


//++ 
//
// VOID
// KiSetRegionRegister(
//     PVOID Region,
//     ULONGLONG Contents 
//     )
//
// Routine Description:
//
//      This routine sets the value of a region register.
//
// Arguments:
//
//      a0 - Supplies the region register #
//     
//      a1 - Supplies the value to be stored in the specified region register
//
// Return Value:
//
//      None.
//
//--


        LEAF_ENTRY(KiSetRegionRegister)

        mov       rr[a0] = a1

        ;;
        srlz.i
        ;;
        LEAF_RETURN

        LEAF_EXIT(KiSetRegionId)



        LEAF_ENTRY(KiSaveProcessorControlState)

        LEAF_RETURN

        LEAF_EXIT(KiSaveProcessorControlState)


        LEAF_ENTRY(KiRestoreProcessorControlState)

        LEAF_RETURN

        LEAF_EXIT(KiRestoreProcessorControlState)


        PublicFunction(KiSaveExceptionFrame)
        PublicFunction(KiRestoreExceptionFrame)
        PublicFunction(KiIpiServiceRoutine)


        NESTED_ENTRY(KeIpiInterrupt)
        NESTED_SETUP(1, 2, 2, 0)
        .fframe ExceptionFrameLength
        add     sp = -ExceptionFrameLength, sp
        ;;
 
        PROLOGUE_END

        add     out0 = STACK_SCRATCH_AREA, sp       // -> exception frame
        br.call.sptk brp = KiSaveExceptionFrame
        ;;

        add     out1 = STACK_SCRATCH_AREA, sp       // -> exception frame
        mov     out0 = a0                           // -> trap frame
        br.call.sptk brp = KiIpiServiceRoutine
        ;;

        add     out0 = STACK_SCRATCH_AREA, sp       // -> exception frame
        br.call.sptk brp = KiRestoreExceptionFrame
        ;;

        add     sp = ExceptionFrameLength, sp
        NESTED_RETURN

        NESTED_EXIT(KeIpiInterrupt)
