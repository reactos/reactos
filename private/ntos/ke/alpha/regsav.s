//      TITLE("Register Save and Restore")
//++
//
// Copyright (c) 1992 Digital Equipment Corporation
//
// Module Name:
//
//      regsav.s
//
// Abstract:
//
//      Implements save/restore general purpose processor
//      registers during exception handling
//
// Author:
//
//      Joe Notarangelo 06-May-1992
//
// Environment:
//
//      Kernel mode only.
//
// Revision History:
//
//--

#include "ksalpha.h"

        SBTTL("Generate Trap Frame")
//++
//
// Routine Description:
//
//     Save volatile register state (integer/float) in
//     a trap frame.
//
//     Note: control registers, ra, sp, fp, gp have already
//     been saved, argument registers a0-a3 have also been saved.
//
// Arguments:
//
//     fp - Supplies a pointer to the trap frame.
//
// Return Value:
//
//     None.
//
//--

        LEAF_ENTRY(KiGenerateTrapFrame)

        stq     v0, TrIntV0(fp)         // save integer register v0
        stq     t0, TrIntT0(fp)         // save integer registers t0 - t7
        stq     t1, TrIntT1(fp)         //
        stq     t2, TrIntT2(fp)         //
        stq     t3, TrIntT3(fp)         //
        stq     t4, TrIntT4(fp)         //
        stq     t5, TrIntT5(fp)         //
        stq     t6, TrIntT6(fp)         //
        stq     t7, TrIntT7(fp)         //
        stq     a4, TrIntA4(fp)         // save integer registers a4 - a5
        stq     a5, TrIntA5(fp)         //
        stq     t8, TrIntT8(fp)         // save integer registers t8 - t12
        stq     t9, TrIntT9(fp)         //
        stq     t10, TrIntT10(fp)       //
        stq     t11, TrIntT11(fp)       //
        stq     t12, TrIntT12(fp)       //

        .set    noat
        stq     AT, TrIntAt(fp)         // save integer register AT
        .set    at

        br      zero, KiSaveVolatileFloatState // save volatile float state

        .end    KiGenerateTrapFrame

        SBTTL("Restore Trap Frame")
//++
//
// Routine Description:
//
//     Restore volatile register state (integer/float) from
//     a trap frame
//
//     Note: control registers, ra, sp, fp, gp will be
//     restored by the PALcode, as will argument registers a0-a3.
//
// Arguments:
//
//     fp - Supplies a pointer to trap frame.
//
// Return Value:
//
//     None.
//
//--

        LEAF_ENTRY(KiRestoreTrapFrame)

        ldq     v0, TrIntV0(fp)         // restore integer register v0
        ldq     t0, TrIntT0(fp)         // restore integer registers t0 - t7
        ldq     t1, TrIntT1(fp)         //
        ldq     t2, TrIntT2(fp)         //
        ldq     t3, TrIntT3(fp)         //
        ldq     t4, TrIntT4(fp)         //
        ldq     t5, TrIntT5(fp)         //
        ldq     t6, TrIntT6(fp)         //
        ldq     t7, TrIntT7(fp)         //
        ldq     a4, TrIntA4(fp)         // restore integer registers a4 - a5
        ldq     a5, TrIntA5(fp)         //
        ldq     t8, TrIntT8(fp)         // restore integer registers t8 - t12
        ldq     t9, TrIntT9(fp)         //
        ldq     t10, TrIntT10(fp)       //
        ldq     t11, TrIntT11(fp)       //
        ldq     t12, TrIntT12(fp)       //

        .set    noat
        ldq     AT, TrIntAt(fp)         // restore integer register AT
        .set    at

//
// Restore the volatile floating register state
//

        br      zero, KiRestoreVolatileFloatState //

        .end    KiRestoreTrapFrame

        SBTTL("Save Volatile Floating Registers")
//++
//
// Routine Description:
//
//     Save volatile floating registers in a trap frame.
//
// Arguments:
//
//     fp - Supplies a pointer to the trap frame.
//
// Return Value:
//
//     None.
//
//--

        LEAF_ENTRY(KiSaveVolatileFloatState)

        //
        // asaxp is broken, it does not know that mf_fpcr f0
        // destroys f0.
        //

        .set noreorder
        stt     f0, TrFltF0(fp)         // save floating register f0
        mf_fpcr f0                      // save fp control register
        .set reorder

        stt     f0, TrFpcr(fp)          //
        stt     f1, TrFltF1(fp)         // save floating register f1
        stt     f10, TrFltF10(fp)       // save floating registers f10 - f30
        stt     f11, TrFltF11(fp)       //
        stt     f12, TrFltF12(fp)       //
        stt     f13, TrFltF13(fp)       //
        stt     f14, TrFltF14(fp)       //
        stt     f15, TrFltF15(fp)       //
        stt     f16, TrFltF16(fp)       //
        stt     f17, TrFltF17(fp)       //
        stt     f18, TrFltF18(fp)       //
        stt     f19, TrFltF19(fp)       //
        stt     f20, TrFltF20(fp)       //
        stt     f21, TrFltF21(fp)       //
        stt     f22, TrFltF22(fp)       //
        stt     f23, TrFltF23(fp)       //
        stt     f24, TrFltF24(fp)       //
        stt     f25, TrFltF25(fp)       //
        stt     f26, TrFltF26(fp)       //
        stt     f27, TrFltF27(fp)       //
        stt     f28, TrFltF28(fp)       //
        stt     f29, TrFltF29(fp)       //
        stt     f30, TrFltF30(fp)       //

        ret     zero, (ra)              // return

        .end    KiSaveVolatileFloatState

        SBTTL("Restore Volatile Floating State")
//++
//
// Routine Description:
//
//     Restore volatile floating registers from a trap frame.
//
//
// Arguments:
//
//     fp - pointer to trap frame
//
// Return Value:
//
//     None.
//
//--

        LEAF_ENTRY(KiRestoreVolatileFloatState)

        ldt     f0, TrFpcr(fp)          // restore fp control register
        mt_fpcr f0                      //
        ldt     f0, TrFltF0(fp)         // restore floating registers f0 - f1
        ldt     f1, TrFltF1(fp)         //
        ldt     f10, TrFltF10(fp)       // restore floating registers f10 - f30
        ldt     f11, TrFltF11(fp)       //
        ldt     f12, TrFltF12(fp)       //
        ldt     f13, TrFltF13(fp)       //
        ldt     f14, TrFltF14(fp)       //
        ldt     f15, TrFltF15(fp)       //
        ldt     f16, TrFltF16(fp)       //
        ldt     f17, TrFltF17(fp)       //
        ldt     f18, TrFltF18(fp)       //
        ldt     f19, TrFltF19(fp)       //
        ldt     f20, TrFltF20(fp)       //
        ldt     f21, TrFltF21(fp)       //
        ldt     f22, TrFltF22(fp)       //
        ldt     f23, TrFltF23(fp)       //
        ldt     f24, TrFltF24(fp)       //
        ldt     f25, TrFltF25(fp)       //
        ldt     f26, TrFltF26(fp)       //
        ldt     f27, TrFltF27(fp)       //
        ldt     f28, TrFltF28(fp)       //
        ldt     f29, TrFltF29(fp)       //
        ldt     f30, TrFltF30(fp)       //

        ret     zero, (ra)              // return

        .end    KiRestoreVolatileFloatState

        SBTTL("Save Non-Volatile Floating State")
//++
//
// Routine Description:
//
//      Save nonvolatile floating registers in
//      an exception frame
//
//
// Arguments:
//
//      sp - pointer to exception frame
//
// Return Value:
//
//      None.
//
//--

        LEAF_ENTRY(KiSaveNonVolatileFloatState)

        stt     f2, ExFltF2(sp)         // save floating registers f2 - f9
        stt     f3, ExFltF3(sp)         //
        stt     f4, ExFltF4(sp)         //
        stt     f5, ExFltF5(sp)         //
        stt     f6, ExFltF6(sp)         //
        stt     f7, ExFltF7(sp)         //
        stt     f8, ExFltF8(sp)         //
        stt     f9, ExFltF9(sp)         //

        ret     zero, (ra)              // return

        .end    KiSaveNonVolatileFloatState

        SBTTL("Restore Non-Volatile Floating State")
//++
//
// Routine Description:
//
//     Restore nonvolatile floating registers from an exception frame.
//
//
// Arguments:
//
//     sp - Supplies a pointer to an exception frame.
//
// Return Value:
//
//      None.
//
//--


        LEAF_ENTRY(KiRestoreNonVolatileFloatState)

        ldt     f2, ExFltF2(sp)         // restore floating registers f2 - f9
        ldt     f3, ExFltF3(sp)         //
        ldt     f4, ExFltF4(sp)         //
        ldt     f5, ExFltF5(sp)         //
        ldt     f6, ExFltF6(sp)         //
        ldt     f7, ExFltF7(sp)         //
        ldt     f8, ExFltF8(sp)         //
        ldt     f9, ExFltF9(sp)         //

        ret     zero, (ra)              // return

        .end    KiRestoreNonVolatileFloatState

        SBTTL("Save Volatile Integer State")
//++
//
// Routine Description:
//
//     Save volatile integer register state in a trap frame.
//
//     Note: control registers, ra, sp, fp, gp have already been saved
//     as have argument registers a0-a3.
//
// Arguments:
//
//      fp - Supplies a pointer to the trap frame.
//
// Return Value:
//
//      None.
//
//--

        LEAF_ENTRY( KiSaveVolatileIntegerState)

        stq     v0, TrIntV0(fp)         // save integer register v0
        stq     t0, TrIntT0(fp)         // save integer registers t0 - t7
        stq     t1, TrIntT1(fp)         //
        stq     t2, TrIntT2(fp)         //
        stq     t3, TrIntT3(fp)         //
        stq     t4, TrIntT4(fp)         //
        stq     t5, TrIntT5(fp)         //
        stq     t6, TrIntT6(fp)         //
        stq     t7, TrIntT7(fp)         //
        stq     a4, TrIntA4(fp)         // save integer registers a4 - a5
        stq     a5, TrIntA5(fp)         //
        stq     t8, TrIntT8(fp)         // save integer registers t8 - t12
        stq     t9, TrIntT9(fp)         //
        stq     t10, TrIntT10(fp)       //
        stq     t11, TrIntT11(fp)       //
        stq     t12, TrIntT12(fp)       //

        .set    noat
        stq     AT, TrIntAt(fp)         // save integer register AT
        .set    at

        ret     zero, (ra)              // return

        .end    KiSaveVolatileIntegerState

        SBTTL("Restore Volatile Integer State")
//++
//
// Routine Description:
//
//     Restore volatile integer register state from a trap frame.
//
//     Note: control registers, ra, sp, fp, gp and argument registers
//     a0 - a3 will be restored by the PALcode.
//
// Arguments:
//
//     fp - Supplies a pointer to the trap frame.
//
// Return Value:
//
//     None.
//
//--

        LEAF_ENTRY(KiRestoreVolatileIntegerState)

        ldq     v0, TrIntV0(fp)         // restore integer register v0
        ldq     t0, TrIntT0(fp)         // restore integer registers t0 - t7
        ldq     t1, TrIntT1(fp)         //
        ldq     t2, TrIntT2(fp)         //
        ldq     t3, TrIntT3(fp)         //
        ldq     t4, TrIntT4(fp)         //
        ldq     t5, TrIntT5(fp)         //
        ldq     t6, TrIntT6(fp)         //
        ldq     t7, TrIntT7(fp)         //
        ldq     a4, TrIntA4(fp)         // restore integer registers a4 - a5
        ldq     a5, TrIntA5(fp)         //
        ldq     t8, TrIntT8(fp)         // restore integer registers t8 - t12
        ldq     t9, TrIntT9(fp)         //
        ldq     t10, TrIntT10(fp)       //
        ldq     t11, TrIntT11(fp)       //
        ldq     t12, TrIntT12(fp)       //

        .set    noat
        ldq     AT, TrIntAt(fp)         // restore integer register AT
        .set    at

        ret     zero, (ra)              // return

        .end    KiRestoreVolatileIntegerState

        SBTTL("Save Floating Point State")
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
//     N.B. Currently this saves only the hardware FPCR. Software
//          emulation is not supported. Floating point from within
//          a DPC is not supported.
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

        //
        // Generate default FPCR value
        //

        ldiq    t0, 0x0800000000000000
        stq     t0, KfsReserved1(a0)
        ldt     f1, KfsReserved1(a0)

        //
        // asaxp is broken, it does not know that mf_fpcr f0
        // destroys f0.
        //

        .set noreorder
        mf_fpcr f0                      // save fp control register
        .set reorder

        stt     f0, KfsFpcr(a0)         //

        //
        // Set default mode - ROUND_TO_NEAREST
        //

        mt_fpcr f1                      //
        bis     zero, zero, v0          // always return success
        ret     zero, (ra)              // return

        .end    KeSaveFloatingPointState

        SBTTL("Restore Floating Point State")
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
//     N.B. Currently this restores only the hardware FPCR. Software
//          emulation is not supported. Floating point from within
//          a DPC is not supported.
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

        ldt     f0, KfsFpcr(a0)         // restore fp control register
        mt_fpcr f0                      //
        bis     zero, zero, v0          // always return success
        ret     zero, (ra)              // return

        .end    KeRestoreFloatingPointState


        SBTTL("Save State For Hibernate")
//++
//
//  VOID
//  KeSaveStateForHibernate(
//      IN PKPROCESSOR_STATE ProcessorState
//      )
//  /*++
//
//  Routine Description:
//
//      Saves all processor-specific state that must be preserved
//      across an S4 state (hibernation).
//
//  Arguments:
//
//      ProcessorState - Supplies the KPROCESSOR_STATE where the
//          current CPU's state is to be saved.
//
//  Return Value:
//
//      None.
//
//--
        .struct 0
KsRa:   .space  8
KsA0:   .space  8
SaveStateLength:
        NESTED_ENTRY(KeSaveStateForHibernate, SaveStateLength,zero)
        lda     sp, -SaveStateLength(sp)    // allocate stack frame
        stq     ra, KsRa(sp)                // save return address
        PROLOGUE_END

        stq     a0, KsA0(sp)
        bsr     ra, RtlCaptureContext
        ldq     t1, KsA0(sp)                // get copy of context pointer
        lda     a1, CxIntA1(t1)             // SleepData pointer will be restored to a1

//    
// The processor context when calling cp_sleep is the one that will be
// restored by the PAL code when doing the restore. A0 contains the 
// address at which execution will resume. We resume in this function so
// that SP gets readjusted to the callers value. However, we must be
// careful not to reference anything in our stack frame after a resume
// as that data was not saved to disk.
//

        lda     a0, do_return               // address at which restore continues execution
        ldq     ra, KsRa(sp)                // address to return to after continuing
        ldil    v0, 0                       // return value on wakeup
        call_pal    cp_sleep                // save PAL state

//
// v0 is now the PALmode (physical) address of the PALcode restore routine, i.e. where
// to jump to in PALmode.  
//
        stq     v0, CxIntA2(t1)             // it will be in a2        

// Set the address to start up at when first entered (the swppal to enter PALmode).

        lda     t2, reentry             // address of startup code below
        stq     t2, CxFir(t1)           // store startup address in CONTEXT        

//
// Set the address for swppal to transfer control to in PALmode.  This must be in a0
// when swppal is executed.
//
        lda     t3, gorestore           // address of transfer to restore below
        sll     t3, 33, t3              // clear high-order 33 bits to convert
        srl     t3, 33, t3              //  to PALmode physical address
        stq     t3, CxIntA0(t1)

//
// Return.
//
        ldq     ra, KsRa(sp)                // restore return address
do_return:
        lda     sp, SaveStateLength(sp)     // deallocate stack frame
        ret     zero, (ra)                  // return

//
// This is where the OS Loader transfers control back to NT.  The CONTEXT was
// set up to direct execution here with registers set as follows:
//
//     a0 - PALmode address of code to be executed in PALmode (gorestore)
//     a1 - pointer to the SleepData structure (returned by sleep)
//     a2 - PALmode address of the restore routine (returned by sleep)
//
// All other registers are presently "don't care", but the code in OS Loader
// that transfers control should restore the entire CONTEXT so that it remains
// compatible even if this convention changes.

reentry:

#if defined(DBG_LEDS)

// LED display: F1

        ldiq    t0, 0xfffffc0000000000+0x87A0000180
        ldil    t1, 0xF1
        stq     t1, (t0)
        mb

        ldil    t0, 166666666
1:
        subl    t0, 1, t0
        bne     t0, 1b

#endif

        call_pal swppal                 // simply enter PALmode, transfer to
                                        // following code

//
// Control is transferred here in PALmode by swppal. a1 contains the pointer
// to the SleepData structure, and a2 contains the PALmode address of the
// restore routine.
//

gorestore:

#if defined(DBG_LEDS)

// LED display: F2

        ldiq    t0, 0xfffffc0000000000+0x87A0000180
        ldil    t1, 0xF2
        stq     t1, (t0)
        mb

        ldil    t0, 166666666
1:
        subl    t0, 1, t0
        bne     t0, 1b

#endif

        bis     a1, 0, a0               // restore needs SleepData pointer in a0
        jmp     (a2)                     // jump to restore routine

        .end KeSaveStateForHibernate

