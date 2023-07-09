//      TITLE("Thread Startup")
//++
//
// Module Name:
//
//    threadbg.s
//
// Abstract:
//
//    This module implements the MIPS machine dependent code necessary to
//    startup a thread in kernel mode.
//
// Author:
//
//    Bernard Lint 19-Mar-1996
//
// Environment:
//
//    Kernel mode only, IRQL APC_LEVEL.
//
// Revision History:
//
//    Based on MIPS version (David N. Cutler (davec) 28-Mar-1990)
//
//--

#include "ksia64.h"

//
// Globals used
//

         PublicFunction(KeBugCheck)
         PublicFunction(KiRestoreExceptionFrame)
         PublicFunction(KiExceptionExit)

        SBTTL("Thread Startup")

//++
//
// Routine Description:
//
//    This routine is called at thread startup. Its function is to call the
//    initial thread procedure. If control returns from the initial thread
//    procedure and a user mode context was established when the thread
//    was initialized, then the user mode context is restored and control
//    is transfered to user mode. Otherwise a bug check will occur.
//
//    When this thread was created, a switch frame for SwapContext was pushed
//    onto the stack and initialized such that SwapContext will return to
//    the first instruction of this routine when this thread is first switched to.
//
// Arguments:
//
//    s0 (saved) - Supplies a boolean value that specified whether a user
//       mode thread context was established when the thread was initialized.
//
//    s1 (saved) - Supplies the starting context parameter for the initial
//       thread procedure.
//
//    s2 (saved) - Supplies the starting address of the initial thread routine.
//
//       N.B. This is a function pointer.
//
//    s3 - Supplies the starting address of the initial system routine.
//
//       N.B. This is an entry point.
//
// On entry:
//
//      Since SwapConext deallocates the switch frame sp points to either:
//            for system thread sp -> bottom of kernel stack;
//            for user thread sp -> exception frame (or higher fp savearea 
//            if not FPLAZY)
//
// Return Value:
//
//    None.
//
//--

        NESTED_ENTRY(KiThreadDispatch)

//
// N.B. The following code is never executed.  Its purpose is to allow the
//      kernel debugger to walk call frames backwards through thread startup
//      and to support get/set user context.
//

        .regstk   0,2,2,0
        .prologue 0xC, loc0

        .fframe   ExceptionFrameLength
        add       sp = -ExceptionFrameLength, sp
        ;;
        add       t0 = ExFltS19+STACK_SCRATCH_AREA, sp
        add       t1 = ExFltS18+STACK_SCRATCH_AREA, sp
        ;;

        .save.gf  0x0, 0xC0000
        stf.spill [t0] = fs19, ExFltS17-ExFltS19
        stf.spill [t1] = fs18, ExFltS16-ExFltS18
        ;;

        .save.gf  0x0, 0x30000
        stf.spill [t0] = fs17, ExFltS15-ExFltS17
        stf.spill [t1] = fs16, ExFltS14-ExFltS16
        mov       t10 = bs4
        ;;

        .save.gf  0x0, 0xC000
        stf.spill [t0] = fs15, ExFltS13-ExFltS15
        stf.spill [t1] = fs14, ExFltS12-ExFltS14
        mov       t11 = bs3
        ;;

        .save.gf  0x0, 0x3000
        stf.spill [t0] = fs13, ExFltS11-ExFltS13
        stf.spill [t1] = fs12, ExFltS10-ExFltS12
        mov       t12 = bs2
        ;;

        .save.gf  0x0, 0xC00
        stf.spill [t0] = fs11, ExFltS9-ExFltS11
        stf.spill [t1] = fs10, ExFltS8-ExFltS10
        mov       t13 = bs1
        ;;

        .save.gf  0x0, 0x300
        stf.spill [t0] = fs9, ExFltS7-ExFltS9
        stf.spill [t1] = fs8, ExFltS6-ExFltS8
        mov       t14 = bs0
        ;;

        .save.gf  0x0, 0xC0
        stf.spill [t0] = fs7, ExFltS5-ExFltS7
        stf.spill [t1] = fs6, ExFltS4-ExFltS6
        mov       t15 = ar.lc
        ;;

        .save.gf  0x0, 0x30
        stf.spill [t0] = fs5, ExFltS3-ExFltS5
        stf.spill [t1] = fs4, ExFltS2-ExFltS4
        ;;

        .save.f   0xC
        stf.spill [t0] = fs3, ExFltS1-ExFltS3         // save fs3
        stf.spill [t1] = fs2, ExFltS0-ExFltS2         // save fs2
        ;;

        .save.f   0x3
        stf.spill [t0] = fs1, ExBrS4-ExFltS1          // save fs1
        stf.spill [t1] = fs0, ExBrS3-ExFltS0          // save fs0
        ;;

        .save.b   0x18
        st8       [t0] = t10, ExBrS2-ExBrS4           // save bs4
        st8       [t1] = t11, ExBrS1-ExBrS3           // save bs3
        ;;

        .save.b   0x6
        st8       [t0] = t12, ExBrS0-ExBrS2           // save bs2
        st8       [t1] = t13, ExIntS2-ExBrS1          // save bs1
        ;;

        .save.b   0x1
        st8       [t0] = t14, ExIntS3-ExBrS0          // save bs0
        ;;

        .save.gf  0xC, 0x0
        .mem.offset 0,0
        st8.spill [t0] = s3, ExIntS1-ExIntS3          // save s3
        .mem.offset 8,0
        st8.spill [t1] = s2, ExIntS0-ExIntS2          // save s2
        ;;

        .save.gf  0x3, 0x0
        .mem.offset 0,0
        st8.spill [t0] = s1, ExApLC-ExIntS1           // save s1
        .mem.offset 8,0
        st8.spill [t1] = s0, ExApEC-ExIntS0           // save s0
        ;;

        .savepsp  ar.pfs, ExceptionFrameLength-ExApEC-STACK_SCRATCH_AREA
        st8       [t1] = t16, ExIntNats-ExApEC
        mov       t4 = ar.unat                        // captured Nats of s0-s3
        ;;

        .savepsp  ar.lc, ExceptionFrameLength-ExApLC-STACK_SCRATCH_AREA
        st8       [t0] = t15
        .savepsp  @priunat, ExceptionFrameLength-ExIntNats-STACK_SCRATCH_AREA
        st8       [t1] = t4                           // save Nats of s0-s3
        ;;


        ALTERNATE_ENTRY(KiThreadStartup)
         
        alloc     t0 = 0,2,2,0                  // allocate call frame 
        mov       savedpfs = zero               // setup bogus brp and pfs
        mov       savedbrp = zero               // to stop stack unwind
                                                // by the debugger
        ;;

        PROLOGUE_END

//
// restore the preserved states from the switch frame and then deallocate it
//

        add       out0 = SwExFrame+STACK_SCRATCH_AREA,sp
        br.call.sptk brp = KiRestoreExceptionFrame
        ;;

//
// Lower IRQL to APC_LEVEL
//

        add       sp = SwitchFrameLength, sp
        mov       bt0 = s3                      // setup call to system routine
        mov       t0 = APC_LEVEL
        ;;

        SET_IRQL(t0)

        mov       out0 = s2                     // arg 1 = thread routine (a function pointer)
        mov       out1 = s1                     // arg 2 = thread context
        br.call.sptk brp = bt0                  // call system routine
        ;;

//
// Finish in common exception exit code which will restore the nonvolatile
// registers and exit to user mode.
//
// N.B. predicate register alias pUstk & pKstk must be the same as trap.s
//      and they must be set up correctly upon entry into KiExceptionExit.
//
//
// If pKstk is set, an attempt was made to enter user mode for a thread 
// that has no user mode context. Generate a bug check.
//

        pUstk     = ps3
        pKstk     = ps4

        cmp.eq      pKstk, pUstk = zero, s0      // if s0 is zero, no user context (system thread)
        mov         out0 = NO_USER_MODE_CONTEXT  // set bug check code
(pKstk) br.call.spnt brp = KeBugCheck

//
// Set up for branch to KiExceptionExit
//
//      s0 = trap frame
//      s1 = exception frame
//

        add         s1 = STACK_SCRATCH_AREA, sp
        add         s0 = ExceptionFrameLength, sp
        br          KiExceptionExit
        ;;

        NESTED_EXIT(KiThreadDispatch)
