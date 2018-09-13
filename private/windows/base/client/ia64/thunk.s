//++
//
// Module Name:
//
//    thunk.s
//
// Abstract:
//
//   This module implements all Win32 thunks. This includes the
///   first level thread starter...
//
// Author:
//
//   12-Oct-1995
//
// Revision History:
//
//--

#include "ksia64.h"
        .file    "thunk.s"


//++
//
// VOID
// BaseThreadStartThunk(
//    IN PTHREAD_START_ROUTINE StartRoutine,
//    IN PVOID ThreadParameter
//    )
//
// Routine Description:
//
//    This function calls to the portable thread starter after moving
//    its arguments from registers to the argument registers.
//
// Arguments:
//
//    s1 - StartRoutine
//    s2 - ThreadParameter
//
// Return Value:
//
//    Never Returns
//
//--

        PublicFunction(BaseThreadStart)

        LEAF_ENTRY(BaseThreadStartThunk)
        LEAF_SETUP(0,0,2,0)

        mov        out0=s1
        mov        out1=s2
        br.many    BaseThreadStart
        ;;

        //
        // never come back here
        //

        LEAF_EXIT(BaseThreadStartThunk)


//++
//
// VOID
// BaseProcessStartThunk(
//    IN PTHREAD_START_ROUTINE StartRoutine,
//    IN PVOID ThreadParameter
//    )
//
// Routine Description:
//
//    This function calls to the portable thread starter after moving
//    its arguments from registers to the argument registers.
//
// Arguments:
//
//    s1 - StartRoutine
//    s2 - ThreadParameter
//
// Return Value:
//
//    Never Returns
//
//--

        LEAF_ENTRY(BaseProcessStartThunk)

        ld8     t3=[s1], 8                   // load ep from there
        ;;
        alloc   t22 = ar.pfs, 0, 0, 1, 0
        mov     bt0 = t3

        ld8     gp=[s1]                      // load gp too
        mov     out0=s2
        br      bt0                          // jump to entry point
        ;;

        LEAF_EXIT(BaseProcessStartThunk)


//++
//
// VOID
// BaseSwitchStackThenTerminate(
//     IN PVOID StackLimit,
//     IN PVOID NewStack,
//     IN DWORD ExitCode
//     )
//
//
// Routine Description:
//
//     This API is called during thread termination to delete a thread's
//     stack, switch to a stack in the thread's TEB, and then terminate.
//
// Arguments:
//
//     StackLimit (a0) - Supplies the address of the stack to be freed.
//
//     NewStack (a1) - Supplies an address within the terminating threads TEB
//         that is to be used as its temporary stack while exiting.
//         This is also used as the new RseStackBase
//
//     ExitCode (a2) - Supplies the termination status that the thread
//         is to exit with.
//
// Return Value:
//
//     None.
//
//--
        PublicFunction(BaseFreeStackAndTerminate)

        RseSIZE   = 320         // reserve 320 bytes RseStack, RseStack should
                                //  be larger than memory stack
        // Input arguments
        StackBase = a0
        NewStack  = a1
        ExitCode  = a2

        OldExitCode = t0
        NewStackBase = t1
        OldStackBase = t2
        SavedRSC = t3
        temp = t4
        Junk=t22

        LEAF_ENTRY(BaseSwitchStackThenTerminate)

        add     NewStackBase = RseSIZE, NewStack
        mov     t0 = 15                      // align base to 16 byte boundry
        ;;
        andcm   NewStackBase = NewStackBase, t0

        mov     SavedRSC = ar.rsc
        mov     OldStackBase = StackBase     // save the arguments to pass down
        mov     OldExitCode = ExitCode
        ;;

        alloc   t22 = ar.pfs, 0, 0, 0, 0     // throw Stacked registers away
        mov     temp = SavedRSC
        ;;
        dep     temp = 0, temp, RSC_MODE, 2
        ;;
        
        mov     ar.rsc = temp
        ;;
        loadrs
        ;;

        mov     ar.bspstore = NewStackBase
        mov     ar.rsc = SavedRSC
        add     sp = -STACK_SCRATCH_AREA, NewStackBase  // setup new stack
        ;;

        alloc   Junk = ar.pfs, 0, 0, 2, 0    // allocate 2 output registers
        mov     brp = zero
        mov     ar.pfs = zero

        mov     out0 = OldStackBase          // re arrange args
        mov     out1 = OldExitCode
        br.many BaseFreeStackAndTerminate
        ;;

        // branch to BaseFreeStackAndTerminate(StackLimit, ExitCode)
        // never comes back here

        LEAF_EXIT(BaseSwitchStackThenTerminate)
//++
//
// VOID
// BaseAttachCompleteThunk(
//        PCONTEXT CONTEXT
//     )
//
//
// Routine Description:
//
//     This function is called after a successful debug attach. Its
//     purpose is to call portable code that does a breakpoint, followed
//     by an NtContinue.
//     When control comes to this routine, the Context is pre-setup in the 
//     memory stack, and a0 is point to the Context.  What we try to do here,
//     is only a transition point as thunk.
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
        PublicFunction(BaseAttachComplete)

        LEAF_ENTRY(BaseAttachCompleteThunk)

        br.many         BaseAttachComplete
        ;;

        LEAF_EXIT(BaseAttachCompleteThunk)
//++
//
// ULONG
// BaseGetTickCount (
//    IN LARGE_INTEGER CurrentTime,
//    IN LARGE_INTEGER BootTime
//    )
//
// Routine Description:
//
//    This function computes the number of milliseconds that have transpired
//    since the system was booted.
//
// Arguments:
//
//    CurrentTime  a0 - Supplies the current time in 100ns units.
//
//    BootTime a1 - Supplies the boot time in 100ns units.
//
//    BasepTickCountMultiplier    dd  0d1b71759H
//
// Return Value:
//
//    The number of milliseconds since system boot is returned as the
//    function value.
//
//--
        .radix  H
        LEAF_ENTRY(BaseGetTickCount)
        LEAF_SETUP(2,0,0,0)

        sub     t2 = a0, a1                 // compute time difference
        movl    t0 = 0d1b71759H
        ;;

        setf.sig fs3 = t0                   // convert into float form
        setf.sig fs2 = t2                   // convert into float form
        ;;
        xma.lu   fs1 = fs2, fs3, f0         // multiply
        ;;
        getf.sig v0 = fs1                   // get reult into integer form
        ;;
        extr.u  v0 = v0, 24, 32             // shift off 24-bit fraction part 
                                            // and the 32-bit canonical ULONG 
        LEAF_RETURN 
        LEAF_EXIT(BaseGetTickCount)
        .radix  C

//++
//
// VOID
// SwitchToFiber(
//    PFIBER NewFiber
//    )
//
// Routine Description:
//
//    This function saves the state of the current fiber and switches
//    to the new fiber.
//
// Arguments:
//
//    NewFiber (a0) - Supplies the address of the new fiber.
//
// Return Value:
//
//    None
//
//--

        LEAF_ENTRY(SwitchToFiber)


        // local register aliases

        rOldFb = t21
        rOldCx = t20
        rNewFb = t19
        rNewCx = t18
        rA0    = t17

        dest1  = t10
        dest2  = t11
        dest4  = t12
        dest5  = t13

        //
        // set up pointers to old and new fiber and context records
        //

        add          rNewFb = zero, a0
        add          rNewCx = FbFiberContext, a0
        add          t0 = TeFiberData, teb
        ;;

        LDPTR        (t1, t0)
        mov          rA0 = a0
        ;;

        add          rOldFb = 0, t1
        add          rOldCx = FbFiberContext, t1
        ;;

        //
        // step 1
        // save current state in to old fiber's fiber and context rec
        //

        //
        // save fiber exception list and stack info
        //
        add          dest1 = TeExceptionList, teb      // TEB
        add          dest2 = FbExceptionList, rOldFb    // Old fiber
        ;;

        LDPTRINC     (t0, dest1, TeStackLimit - TeExceptionList)  
        ;;
        STPTRINC     (dest2, t0, FbStackLimit - FbExceptionList)
        ;;

        LDPTR        (t0, dest1)
        ;;
        STPTR        (dest2, t0)

        //
        // also save RSE stack info
        //
        add          dest4 = TeBStoreLimit, teb
        add          dest5 = FbBStoreLimit, rOldFb
        ;;

        LDPTR        (t1, dest4)
        ;;
        STPTR        (dest5, t1)
        ;;

        //
        // spill low non-volatile fp registers 0-3, 5-19
        //
        add          dest1 = CxFltS0, rOldCx
        add          dest2 = CxFltS1, rOldCx
        ;;

        stf.spill    [dest1] = fs0, CxFltS2 - CxFltS0
        stf.spill    [dest2] = fs1, CxFltS3 - CxFltS1
        ;;

        stf.spill    [dest1] = fs2, CxFltS5 - CxFltS2
        stf.spill    [dest2] = fs3, CxFltS6 - CxFltS3
        ;;

        stf.spill    [dest1] = fs5, CxFltS7 - CxFltS5
        stf.spill    [dest2] = fs6, CxFltS8 - CxFltS6
        ;;

        stf.spill    [dest1] = fs7, CxFltS9 - CxFltS7
        stf.spill    [dest2] = fs8, CxFltS10 - CxFltS8
        ;;

        stf.spill    [dest1] = fs9, CxFltS11 - CxFltS9
        stf.spill    [dest2] = fs10, CxFltS12 - CxFltS10
        ;;

        stf.spill    [dest1] = fs11, CxFltS13 - CxFltS11
        stf.spill    [dest2] = fs12, CxFltS14 - CxFltS12
        ;;

        stf.spill    [dest1] = fs13, CxFltS15 - CxFltS13
        stf.spill    [dest2] = fs14, CxFltS16 - CxFltS14
        ;;

        stf.spill    [dest1] = fs15, CxFltS17 - CxFltS15
        stf.spill    [dest2] = fs16, CxFltS18 - CxFltS16
        ;;
        
        stf.spill    [dest1] = fs17, CxFltS19 - CxFltS17
        stf.spill    [dest2] = fs18 
        ;;

        stf.spill    [dest1] = fs19

        //
        // fp status registers
        //

        mov          t2 = ar.fpsr                         //FPSR
        add          dest1 = CxStFPSR,  rOldCx
        add          dest2 = CxStFSR,  rOldCx
        ;;
        
        mov          t3 = ar28                            //FSR
        st8          [dest1] = t2
        ;;

        mov          t4 = ar29                            //FIR
        st8          [dest2] = t3, CxStFDR - CxStFSR
        add          dest1 = CxStFIR, rOldCx
        ;;

        mov          t5 = ar30                            //FDR
        st8          [dest1] = t4
        ;;

        st8          [dest2] = t5

        // 
        // save old unat before starting the spills
        //

        mov          t6 = ar.unat          
        add          dest4 = CxApUNAT, rOldCx
        ;;

        st8          [dest4] = t6
        mov          ar.unat = zero
        ;;

        // ordering ? should not start spilling before unat is saved 

        // save sp and preserved int registers

        add          dest4 = CxIntS0, rOldCx
        add          dest5 = CxIntSp, rOldCx
        ;;

        .mem.offset 0,0
        st8.spill    [dest5] = sp, CxIntS1 - CxIntSp
        .mem.offset 8,0
        st8.spill    [dest4] = s0, CxIntS2 - CxIntS0
        ;;

        .mem.offset 0,0
        st8.spill    [dest5] = s1, CxIntS3 - CxIntS1
        .mem.offset 8,0
        st8.spill    [dest4] = s2
        ;;

        st8.spill    [dest5] = s3

        // save predicates

        add          dest4 = CxPreds, rOldCx
        add          dest5 = CxBrRp, rOldCx

        mov          t7 = pr
        ;;
        st8          [dest4] = t7, CxBrS0 - CxPreds

        // save preserved branch registers

        mov          t8 = brp
        ;;

        st8          [dest5] = t8, CxBrS1 - CxBrRp
        mov          t9 = bs0

        ;;
        st8          [dest4] = t9, CxBrS2 - CxBrS0

        mov          t1 = bs1
        ;;
        st8          [dest5] = t1, CxBrS3 - CxBrS1

        mov          t2 = bs2
        ;;
        st8          [dest4] = t2, CxBrS4 - CxBrS2

        mov          t3 = bs3
        ;;
        st8          [dest5] = t3

        mov          t4 = bs4
        ;;
        st8          [dest4] = t4

        // save other applicatin registers
        //

        mov          t6 = ar.lc
        add          dest4 = CxApLC, rOldCx
        add          dest5 = CxApEC, rOldCx
        ;;

        st8          [dest4] = t6, CxRsPFS - CxApLC
        mov          t7 = ar.ec
        ;;

        st8          [dest5] = t7, CxRsRSC - CxApEC

        //
        // save RSE stuff
        //
        mov        t8 = ar.pfs
        ;;

        st8        [dest4] = t8
        mov        t9 = ar.rsc
        ;;

        st8        [dest5] = t9
        dep        t9 = 0, t9, RSC_MODE, 2                  // put in lazy mode
        ;;
        mov        ar.rsc = t9

        //
        // since we do not use locals, we don't need cover..
        // cover
        // ;;

        ;;
        flushrs
        dep        t9 = 0, t9, RSC_LOADRS, RSC_LOADRS_LEN  // invalidate all 
        ;;
        mov        ar.rsc = t9
        ;;
        loadrs

        add        dest1 = CxRsRNAT, rOldCx
        add        dest2 = CxRsBSPSTORE, rOldCx
        ;;

        mov        t1 = ar.bspstore
        ;;
        st8        [dest2] = t1

        mov        t2 = ar.rnat
        ;;
        st8        [dest1] = t2


        // save all spilled NaT bits in in IntNats

        add        dest1 = CxIntNats, rOldCx
        mov        t3 = ar.unat
        ;;
        st8        [dest1] = t3


        //
        // step 2
        // setup the state for new fiber from new context/fiber record
        //

        // restore exception list and stack info fist
        //
        add          dest1 = TeExceptionList, teb
        add          dest2 = FbExceptionList, rNewFb
        ;;

        LDPTRINC     (t0, dest2, FbStackBase - FbExceptionList)  
        ;;
        STPTRINC     (dest1, t0, TeStackBase - TeExceptionList)
        ;;

        LDPTRINC     (t0, dest2, FbStackLimit - FbStackBase)
        ;;
        STPTRINC     (dest1, t0, TeStackLimit - TeStackBase)
        ;;

        LDPTR        (t0, dest2)
        ;;
        STPTR        (dest1, t0)

        add          dest4 = TeDeallocationStack, teb
        add          dest5 = FbDeallocationStack, rNewFb
        ;;

        LDPTR        (t1, dest5)
        ;;
        STPTR        (dest4, t1)

        // also restore RSE stack info
        //
        add          dest4 = TeBStoreLimit, teb
        add          dest5 = FbBStoreLimit, rNewFb
        ;;

        LDPTRINC     (t2, dest5, FbDeallocationBStore - FbBStoreLimit)
        ;;
        STPTRINC     (dest4, t2, TeDeallocationBStore - TeBStoreLimit)
        ;;

        LDPTR        (t3, dest5)
        ;;
        STPTR        (dest4, t3)

        // set the fiber pointer in teb to point to new fiber
        //
        add          dest1 = TeFiberData, teb
        ;;
        STPTR        (dest1, rA0)


        // set up new RSE first

        add          dest1 = CxRsRSC, rNewCx
        add          dest2 = CxRsBSPSTORE, rNewCx
        ;;

        ld8          t2 = [dest2], CxRsRNAT - CxRsBSPSTORE
        ;;
        mov          ar.bspstore = t2
        invala

        ld8          t3 = [dest2]
        ;;
        mov          ar.rnat = t3

        ld8          t4 = [dest1]
        ;;
        mov          ar.rsc = t4

        add          dest4 = CxRsPFS, rNewCx
        ;;
        ld8          t5 = [dest4]
        ;;
        mov          ar.pfs = t5


        // restore floating point registers

        add          dest1 = CxFltS0, rNewCx
        add          dest2 = CxFltS1, rNewCx
        ;;

        ldf.fill     fs0 = [dest1], CxFltS2 - CxFltS0
        ldf.fill     fs1 = [dest2] , CxFltS3 - CxFltS1
        ;;

        ldf.fill     fs2 = [dest1], CxFltS5 - CxFltS2
        ldf.fill     fs3 = [dest2], CxFltS6 - CxFltS3
        ;;

        ldf.fill     fs5 = [dest1], CxFltS7 - CxFltS5
        ldf.fill     fs6 = [dest2], CxFltS8 - CxFltS6
        ;;

        ldf.fill     fs7 = [dest1], CxFltS9 - CxFltS7
        ldf.fill     fs8 = [dest2], CxFltS10 - CxFltS8
        ;;
        
        ldf.fill     fs9 = [dest1], CxFltS11 - CxFltS9
        ldf.fill     fs10 = [dest2], CxFltS12 - CxFltS10
        ;;
        
        ldf.fill     fs11 = [dest1], CxFltS13 - CxFltS11
        ldf.fill     fs12 = [dest2], CxFltS14 - CxFltS12
        ;;
        
        ldf.fill     fs13 = [dest1], CxFltS15 - CxFltS13
        ldf.fill     fs14 = [dest2], CxFltS16 - CxFltS14
        ;;
        
        ldf.fill     fs15 = [dest1], CxFltS17 - CxFltS15
        ldf.fill     fs16 = [dest2], CxFltS18 - CxFltS16
        ;;
        
        ldf.fill     fs17 = [dest1], CxFltS19 - CxFltS17
        ldf.fill     fs18 = [dest2]
        ;;
        
        ldf.fill     fs19 = [dest1]

        add          dest1 = CxStFPSR, rNewCx
        add          dest2 = CxStFSR, rNewCx
        ;;

        ld8          t2 = [dest1]                         //FPSR
        ;;
        mov          ar.fpsr = t2

        ld8          t3 = [dest2], CxStFDR - CxStFSR
        add          dest1 = CxStFIR, rNewCx
        ;;
        mov          ar28 = t3                            //FSR

        ld8          t4 = [dest1]
        ;;
        mov          ar29 = t4                            //FIR

        ld8          t5 = [dest2]
        ;;
        mov          ar30 = t5                            //FDR

        //
        // restore ar.unat first, so that fills will restore the
        // nat bits correctly
        //
        add          dest4 = CxIntNats, rNewCx
        ;;
        ld8          t6 = [dest4]
        ;;
        mov          ar.unat = t6

        // now start filling the preserved integer registers
        //
        add          dest4 = CxIntS0, rNewCx
        add          dest5 = CxIntSp, rNewCx
        ;;


        ld8.fill     sp = [dest5], CxIntS1 - CxIntSp

        // save preserved integer registers

        ld8.fill     s0 = [dest4], CxIntS2 - CxIntS0
        ;;
        ld8.fill     s1 = [dest5], CxIntS3 - CxIntS1

        ld8.fill     s2 = [dest4]
        ;;
        ld8.fill     s3 = [dest5]

        // restore predicates and branch registers

        add          dest4 = CxPreds, rNewCx
        add          dest5 = CxBrRp, rNewCx
        ;;

        ld8          t7 = [dest4], CxBrS0 - CxPreds
        ;;
        mov          pr = t7

        ld8          t8 = [dest5], CxBrS1 - CxBrRp
        ;;
        mov          brp = t8

        ld8          t9 = [dest4], CxBrS2 - CxBrS0
        ;;
        mov          bs0 = t9

        ld8          t1 = [dest5], CxBrS3 - CxBrS1
        ;;
        mov          bs1 = t1

        ld8          t2 = [dest4], CxBrS4 - CxBrS2
        ;;
        mov          bs2 = t2

        ld8          t3 = [dest5]
        ;;
        mov          bs3 = t3

        ld8          t4 = [dest4]
        ;;
        mov          bs4 = t4


        // restore other applicatin registers
        //
        add          dest4 = CxApLC, rNewCx
        add          dest5 = CxApEC, rNewCx
        ;;

        ld8          t6 = [dest4]
        ;;
        mov          ar.lc = t6

        ld8          t7 = [dest5]
        ;;
        mov          ar.ec = t7


        // finally restore the unat register
        //
        add          dest4 = CxApUNAT, rNewCx
        ;;
        ld8          t5 = [dest4]
        ;;
        mov          ar.unat = t5

        br.ret.sptk brp

        // 
        // this will execute BaseFiberStart if we are switching to
        // the new fiber for the first time. otherwise, it will
        // return back to new fiber. 
        //
        
        LEAF_EXIT(SwitchToFiber)


#if 0
        LEAF_ENTRY(GenericIACall)
        LEAF_SETUP(1,95,0,0)

//
// Load iA state for iVE. Since working with flat 32 in NT,
// much of the state is a constant (per Freds document)
//  
        mov rBase   = teb               // Get TEB pointer

// load up selector register constants, we dont care about GS
        mov rES = _DataSelector 
        mov rSS = _DataSelector
        mov rDS = _DataSelector
        mov rGS = _DataSelector
        mov rCS = _CodeSelector
        mov rFS = _FsSelector
        mov rLDT    = _LdtSelector  
//
//  Setup pointer to iA32 Resources relative to TEB
//
        mov r23 = rIA32Rsrc
        add rIA32Ptr = rBase, r23

        ld8    rGDTD   = [rIA32Ptr], 8      //  load LDT Descriptor registers
        ld8    rLDTD   = [rIA32Ptr], 8      //  GDT Descriptor is 8 bytes after
        ld8    rFSD    = [rIA32Ptr]         //  FSDescriptor is 8 bytes after
//
//  Eflag should not be touched by stub routines...
//

//
// Since CSD and SSD are in AR registers and since they are saved
// on context switches, dont need to reload them...
//
//
//  DSD and ESD are the same as SSD, and we dont care about GSD
//
        mov rESD    =   rSSD    
        mov rDSD    =   rSSD
        mov rGSD    =   rSSD
    
//
// push the return address on the memory stack
//
//
// As we never return, just push NULL...
//
//
// Stack always points to a valid value, so decrement before putting on
// return address
//
        adds    sp = -4, sp
        st4     [sp] = r0

        ARGPTR (in0)
        sxt4   r23 = in0
        mov b7 = r23

        br.ia.sptk   b7

//
// Return addresses and stuff would go here, but we never return
//

        LEAF_EXIT(GenericIACall)
#endif
