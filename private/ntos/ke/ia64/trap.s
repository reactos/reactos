//++
//
// Module Name:
//       trap.s
//
// Abstract:
//       Low level interruption handlers
//
// Author:
//       Bernard Lint      12-Jun-1995
//
// Environment:
//       Kernel mode only
//
// Revision History:
//
//
// Open Design Issues:
//
//       1. Optimizations for more than 2-way parallelism when 
//          regsiters available.
//--

#define INTERRUPTION_LOGGING 1
#include "ksia64.h"

         .file "trap.s"
         .explicit

//
// Globals imported:
//

        PublicFunction(KeBugCheckEx)
        PublicFunction(KiApcInterrupt)
        PublicFunction(KiCheckForSoftwareInterrupt)
        PublicFunction(KiDispatchException)
        PublicFunction(KiExternalInterruptHandler)
        PublicFunction(KiFloatTrap)
        PublicFunction(KiFloatFault)
        PublicFunction(KiGeneralExceptions)
        PublicFunction(KiNatExceptions)
        PublicFunction(KiMemoryFault)
        PublicFunction(KiOtherBreakException)
        PublicFunction(KiPanicHandler)
        PublicFunction(KiSingleStep)
        PublicFunction(KiUnalignedFault)
        PublicFunction(PsConvertToGuiThread)
        PublicFunction(ExpInterlockedPopEntrySListFault)
        PublicFunction(NscLastBundle)
        PublicFunction(KiEmulateSpeculationFault)

        PublicFunction(KiIA32ExceptionVectorHandler)
        PublicFunction(KiIA32InterruptionVectorHandler)
        PublicFunction(KiIA32InterceptionVectorHandler)


        .global     KiSystemServiceHandler
        .global     KeServiceDescriptorTable
        .global     KeGdiFlushUserBatch
        .global     __imp_HalEOITable

//
// Following are for A0 2173 workaround
//
#ifdef A0_2173
        .global    __imp_WRITE_PORT_ULONG_SPECIAL
        .global    __imp_READ_PORT_ULONG_SPECIAL

        .global     __imp_HalPxbTcap
        .global     __imp_HalIpiLock
#endif


//
// Register aliases used throughout the entire module
//

//
// Banked general registers
// 
// h16-h23 can only be used when psr.ic=1.
//
// h24-h31 can only be used when psr.ic=0 (these are reserved for tlb
// and pal/machine check handlers when psr.ic=1).
//

//
// Shown below are aliases of bank 0 registers used in the low level handlers
// by macros ALLOCATE_TRAP_FRAME, SAVE_INTERRUPTION_RESOURCES, and 
// NORMAL_KERNEL_EXIT.  When the code in the macros are changes, these
// register aliases must be reviewed.
//

        rHIPSR      = h16
        rHpT2       = h16

        rHIIPA      = h17
        rHRSC       = h17
        rHDfhPFS    = h17  // used to preserve pfs in KiDisabledFpRegisterVector

        rHIIP       = h18
        rHFPSR      = h18

        rHOldPreds  = h19
        rHBrp       = h19
        rHDCR       = h19

        rHIFS       = h20
        rHPFS       = h20
        rHBSP       = h20

        rHISR       = h21
        rHUNAT      = h21
        rHpT3       = h21
        
        rHSp        = h22
        rHDfhBrp    = h22  // used to preserve brp in KiDisabledFpRegisterVector
        rHpT4       = h22

        rHpT1       = h23
        
        rHIFA       = h24
        rTH3        = h24

        rHHandler   = h25
        rTH1        = h26
        rTH2        = h27
        rHIIM       = h28
        rHEPCVa     = h29

        rHEPCVa2    = h30
        rPanicCode  = h30

//
// General registers used through out module
//

        pApc      = ps0                         // User Apc Pending
        pUser     = ps1                         // mode on entry was user
        pKrnl     = ps2                         // mode on entry was kernel
        pUstk     = ps3
        pKstk     = ps4
        pEM       = ps5                         // EM ISA on kernel entry
        pIA       = ps6                         // X86 ISA on kernel entry

//
// Kernel registers used through out module
//
        rkHandler = k6                          // specific exception handler




//
// Macro definitions for this module only
//

//
// Define vector/exception entry/exit macros.
// N.B. All HANDLER_ENTRY functions go into .nsc section with
//      KiNormalSystemCall being the first.
//

#define HANDLER_ENTRY(Name)                     \
        .##global Name;                         \
        .##proc   Name;                         \
Name::

#define HANDLER_ENTRY_EX(Name, Handler)         \
        .##global Name;                         \
        .##proc   Name;                         \
        .##type   Handler, @function;           \
        .##personality Handler;                 \
Name::

#if defined(INTERRUPTION_LOGGING)
#define  VECTOR_ENTRY(Offset, Name, Extra0)     \
        .##org Offset;                          \
        .##global Name;                         \
        .##proc   Name;                         \
Name::                                          \
        mov    h30 = (Offset >> 8)             ;\
        mov    h31 = Extra0                    ;\
        br     LogInterruptionEvent            ;\
        ;;                                     ;\
        mov    b0 = h30  /* restore b0       */

#else
#define  VECTOR_ENTRY(Offset, Name, Extra0)     \
        .##org Offset;                          \
        .##global Name;                         \
        .##proc   Name;                         \
Name::
#endif

#define VECTOR_EXIT(Name)                       \
        .##endp Name

#define HANDLER_EXIT(Name)                      \
        .##endp Name


//++
// Routine:
//
//       IO_END_OF_INTERRUPT(rVector,rT1,rT2,pEOI)
//
// Routine Description:
//
//       HalEOITable Entry corresponding to the vectorNo is tested.
//       If the entry is nonzero, then vectorNo is stored to the location
//       specified in the entry. If the entry is zero, return.
//
// Arguements:
//
//
// Notes:
//
//       MS preprocessor requires /*   */ style comments
//
//--

#define IO_END_OF_INTERRUPT(rVector,rT1,rT2,pEOI)                             ;\
        add         rT1 = @gprel(__imp_HalEOITable),gp                        ;\
        ;;                                                                    ;\
        ld8         rT1 = [rT1]                                               ;\
        ;;                                                                    ;\
        shladd      rT2 = rVector,3,rT1                                       ;\
        ;;                                                                    ;\
        ld8         rT1 = [rT2]                                               ;\
        ;;                                                                    ;\
        cmp.ne      pEOI = zero, rT1                                          ;\
        ;;                                                                    ;\
(pEOI)  st8.rel     [rT1] = rVector


//++
// Routine:
//
//       VECTOR_CALL_HANDLER(Handler, SpecificHandler)
//
// Routine Description:
//
//       Common code for transfer to heavyweight handlers from
//       interruption vectors. Put RSE in store intensive mode,
//       cover current frame and call handler.
//
// Arguments:
//
//       Handler: First level handler for this vector
//       SpecificHandler: Specific handler to be called by the generic
//                        exception handler.
//
// Return Value:
//
//       None
//
// Notes: 
//      Uses just the kernel banked registers (h16-h31)
//
//      MS preprocessor requires /* */ style comments
//--


#define VECTOR_CALL_HANDLER(Handler,SpecificHandler)                          ;\
        mov         rHIFA = cr##.##ifa                                        ;\
        movl        rTH1 = KiPcr+PcSavedIFA                                   ;\
        ;;                                                                    ;\
                                                                              ;\
        st8         [rTH1] = rHIFA                                            ;\
        movl        rHHandler = SpecificHandler                               ;\
        br##.##sptk Handler                                                   ;\
        ;;

//++
// Routine:
//
//       ALLOCATE_TRAP_FRAME
//
// Routine Description:
//
//       Common code for allocating trap frame on kernel entry for heavyweight
//       handler.
//
// On entry:
//
// On exit: sp -> trap frame; any instruction that depends on sp must be
//          placed in the new instruction group.  Interruption resources
//          ipsr, iipa, iip, predicates, isr, sp, ifs are captured in
//          seven of the banked registers h16-23.  The last one is used
//          by SAVE_INTERRUPTION_STATE as a pointer to save these resources
//          in the trap frame.
//
// Return Value:
//
//       None
//
// Notes: 
//      Uses just the kernel banked registers (h16-h31)
//
//      MS preprocessor requires /* */ style comments below
//--

#define ALLOCATE_TRAP_FRAME                                                   ;\
                                                                              ;\
        pOverflow   = pt2                                                     ;\
                                                                              ;\
        mov         rHIPSR = cr##.##ipsr                                      ;\
        mov         rHIIP = cr##.##iip                                        ;\
        cover                                   /* cover and save IFS       */;\
        ;;                                                                    ;\
                                                                              ;\
        mov         rHIIPA = cr##.##iipa                                      ;\
        movl        rTH2 = MM_EPC_VA                                          ;\
                                                                              ;\
        mov         rTH3 = ar##.##bsp                                         ;\
        mov         rHOldPreds = pr                                           ;\
        mov         rHEPCVa2 = @secrel(NscLastBundle)                         ;\
        ;;                                                                    ;\
                                                                              ;\
        mov         rHIFS = cr##.##ifs                                        ;\
        add         rHEPCVa = 0x30, rTH2                                      ;\
        add         rHEPCVa2 = rHEPCVa2, rTH2                                 ;\
                                                                              ;\
        mov         rHISR = cr##.##isr                                        ;\
        movl        rTH2 = KiPcr+PcInitialStack                               ;\
                                                                              ;\
        tbit##.##z  pEM, pIA = rHIPSR, PSR_IS           /* set instr set    */;\
        extr##.##u  rTH1 = rHIPSR, PSR_CPL, PSR_CPL_LEN /* get mode         */;\
        mov         rHSp = sp                                                 ;\
        ;;                                                                    ;\
                                                                              ;\
        ssm         (1 << PSR_IC) | (1 << PSR_DFH) | (1 << PSR_AC)            ;\
        cmp4##.##eq pKrnl, pUser = PL_KERNEL, rTH1   /* set mode pred       */;\
        cmp4##.##eq pKstk, pUstk = PL_KERNEL, rTH1   /* set stack pred      */;\
        ;;                                                                    ;\
                                                                              ;\
(pKrnl) cmp##.##eq##.##or##.##andcm  pUstk, pKstk = rHIIP, rHEPCVa            ;\
(pKrnl) cmp##.##eq##.##or##.##andcm  pUstk, pKstk = rHIIP, rHEPCVa2           ;\
        ;;                                                                    ;\
                                                                              ;\
.pred.rel "mutex",pUstk,pKstk                                                 ;\
(pUstk) ld8         sp = [rTH2], PcStackLimit-PcInitialStack                  ;\
(pKstk) add         sp = -TrapFrameLength, sp           /* allocate TF      */;\
(pKstk) add         rTH2 = PcStackLimit-PcInitialStack, rTH2                  ;\
        ;;                                                                    ;\
                                                                              ;\
(pKstk) ld8         rTH1 = [rTH2], PcBStoreLimit-PcStackLimit                 ;\
(pUstk) add         sp = -ThreadStateSaveAreaLength-TrapFrameLength, sp       ;\
        mov         rPanicCode = PANIC_STACK_SWITCH                           ;\
        ;;                                                                    ;\
                                                                              ;\
(pKstk) ld8         rTH2 = [rTH2]                                             ;\
(pKstk) cmp##.##leu##.##unc pOverflow = sp, rTH1   /* kernel stack overflow */;\
(pOverflow) br.spnt.few KiPanicHandler                                        ;\
        ;;                                                                    ;\
                                                                              ;\
(pKstk) cmp##.##geu##.##unc pOverflow = rTH3, rTH2 /* kernel bstore overflow*/;\
        add         rHpT1 = TrStIPSR, sp          /* -> IPSR                */;\
(pOverflow) br.spnt.few KiPanicHandler



//++
// Routine:
//
//       SAVE_INTERRUPTION_STATE(Label)
//
// Routine Description:
//
//       Common code for saving interruption state on entry to a heavyweight
//       handler.
//
// Arguments:
//
//       Label: label for branching around BSP switch
//
// On entry:
//
//       sp -> trap frame
//
// On exit:
//
//       Static registers gp, teb, sp, fpsr spilled into the trap frame.
//       Registers gp, teb, fpsr are set up for kernel mode execution.
//
// Return Value:
//
//       None
//
// Notes: 
//
//      Interruption resources already captured in bank 0 registers h16-h23.
//      It's safe to take data TLB fault when saving them into the trap
//      frame because kernel stack is always resident in memory.  This macro
//      is carefully constructed to save the bank registers' contents in
//      the trap frame and reuse them to capture other register states as
//      soon as they are available.  Until we have a virtual register
//      allocation scheme in place, the bank 0 register aliases defined at
//      the beginning of the file must be updated when this macro is modified.
//      
//      MS preprocessor requires /* */ style comments below
//--


#define SAVE_INTERRUPTION_STATE(Label)                                        ;\
                                                                              ;\
/* Save interruption resources in trap frame */                               ;\
                                                                              ;\
                                                                              ;\
        srlz##.##i                            /* I serialize required       */;\
        ;;                                                                    ;\
        st8         [rHpT1] = rHIPSR, TrStISR-TrStIPSR /* save IPSR         */;\
        add         rHpT2 = TrPreds, sp               /* -> Preds           */;\
        ;;                                                                    ;\
                                                                              ;\
        st8         [rHpT1] = rHISR, TrStIIP-TrStISR  /* save ISR           */;\
        st8         [rHpT2] = rHOldPreds, TrBrRp-TrPreds                      ;\
        ;;                                                                    ;\
                                                                              ;\
        mov         rHUNAT = ar##.##unat                                      ;\
        st8         [rHpT1] = rHIIP, TrStIFS-TrStIIP  /* save IIP           */;\
        mov         rHBrp = brp                                               ;\
        ;;                                                                    ;\
                                                                              ;\
        mov         rHFPSR = ar##.##fpsr                                      ;\
        st8         [rHpT1] = rHIFS, TrStIIPA-TrStIFS /* save IFS           */;\
        mov         rHPFS = ar##.##pfs                                        ;\
        ;;                                                                    ;\
                                                                              ;\
        st8         [rHpT1] = rHIIPA, TrStFPSR-TrStIIPA /* save IIPA        */;\
        st8         [rHpT2] = rHBrp, TrRsPFS-TrBrRp                           ;\
        ;;                                                                    ;\
                                                                              ;\
        mov         rHRSC = ar##.##rsc                                        ;\
        st8         [rHpT2] = rHPFS                   /* save PFS           */;\
        add         rHpT2 = TrApUNAT, sp                                      ;\
                                                                              ;\
        mov         rHBSP = ar##.##bsp                                        ;\
        mov         rHDCR = cr##.##dcr                                        ;\
        ;;                                                                    ;\
                                                                              ;\
        st8         [rHpT1] = rHFPSR, TrRsRSC-TrStFPSR /* save FPSR         */;\
        st8         [rHpT2] = rHUNAT, TrIntGp-TrApUNAT /* save UNAT         */;\
        ;;                                                                    ;\
                                                                              ;\
        st8         [rHpT1] = rHRSC, TrRsBSP-TrRsRSC  /* save RSC           */;\
        st8##.##spill [rHpT2] = gp, TrIntTeb-TrIntGp  /* spill GP           */;\
        ;;                                                                    ;\
                                                                              ;\
        st8##.##spill [rHpT2] = teb, TrIntSp-TrIntTeb /* spill TEB (r13)    */;\
        mov         teb = kteb                        /* sanitize teb       */;\
        ;;                                                                    ;\
                                                                              ;\
        st8         [rHpT1] = rHBSP                   /* save BSP           */;\
        movl        rHpT1 = KiPcr + PcKernelGP                                ;\
                                                                              ;\
(pUstk) mov         ar##.##rsc = r0                 /* put RSE in lazy mode */;\
        st8##.##spill [rHpT2] = rHSp, TrApDCR-TrIntSp /* spill SP           */;\
        ;;                                                                    ;\
                                                                              ;\
        ld8         gp = [rHpT1], PcInitialBStore-PcKernelGP /* load GP     */;\
        st8         [rHpT2] = rHDCR                   /* save DCR           */;\
(pKstk) br##.##dpnt Label                       /* br if on kernel stack    */;\
                                                                              ;\
                                                                              ;\
/*                                                                          */;\
/* Local register aliases for back store switch                             */;\
/* N.B. These must be below h24 since PSR.ic = 1 at this point              */;\
/*      h16-h23 are available                                               */;\
/*                                                                          */;\
                                                                              ;\
        rpRNAT    = h16                                                       ;\
        rpBSPStore= h17                                                       ;\
        rBSPStore = h18                                                       ;\
        rKBSPStore= h19                                                       ;\
        rRNAT     = h20                                                       ;\
        rKrnlFPSR = h21                                                       ;\
        rEFLAG    = h22                                                       ;\
                                                                              ;\
/*                                                                          */;\
/* If previous mode is user, switch to kernel backing store                 */;\
/* -- uses the "loadrs" approach. Note that we do not save the              */;\
/* BSP/BSPSTORE in the trap frame if prvious mode was kernel                */;\
/*                                                                          */;\
                                                                              ;\
                                                                              ;\
        mov       rBSPStore = ar##.##bspstore   /* get user bsp store point */;\
        mov       rRNAT = ar##.##rnat           /* get RNAT                 */;\
        add       rpRNAT = TrRsRNAT, sp         /* -> RNAT                  */;\
        ;;                                                                    ;\
                                                                              ;\
        ld8       rKBSPStore = [rHpT1]          /* load kernel bstore       */;\
        movl      rKrnlFPSR = FPSR_FOR_KERNEL   /* initial fpsr value       */;\
        ;;                                                                    ;\
                                                                              ;\
        mov       ar##.##fpsr = rKrnlFPSR       /* set fpsr                 */;\
        add       rpBSPStore = TrRsBSPSTORE, sp /* -> User BSPStore         */;\
        ;;                                                                    ;\
                                                                              ;\
        st8       [rpRNAT] = rRNAT              /* save user RNAT           */;\
        st8       [rpBSPStore] = rBSPStore      /* save user BSP Store      */;\
        ;;                                                                    ;\
        dep       rKBSPStore = rBSPStore, rKBSPStore, 0, 9                    ;\
                                                /* adjust kernel BSPSTORE   */;\
                                                /* for NAT collection       */;\
                                                                              ;\
        ;;                                                                    ;\
                                                                              ;\
/*                                                                          */;\
/* Now running on kernel backing store                                      */;\
/*                                                                          */;\
                                                                              ;\
Label:                                                                        ;\
(pUstk) mov       ar##.##bspstore = rKBSPStore  /* switch to kernel BSP     */;\
(pUstk) mov       ar##.##rsc = RSC_KERNEL       /* turn rse on, kernel mode */;\
        bsw##.##1                               /* switch back to user bank */;\
        ;;                                      /* stop bit required        */;\
        nop.m 0;;                               /* Merced A0 workaround     */;\
        nop.m 0;;                               /* Merced A0 workaround     */;\
        nop.m 0;;                               /* Merced A0 workaround     */;



//++
// Routine:
//
//       RETURN_FROM_INTERRUPTION
//
// Routine Description:
//
//       Common handler code to restore trap frame and resume execution
//       at the interruption address.
//
// Arguments:
//
//       Label
//
// Return Value:
//
//       None
//
// Note: 
//
//       On entry: interrrupts disabled, sp -> trap frame
//       On exit:
//       MS preprocessor requires /* */ style comments below
//--

#define RETURN_FROM_INTERRUPTION(Label)                                       ;\
                                                                              ;\
        .##regstk 0,3,2,0       /* must match the alloc instruction below */  ;\
                                                                              ;\
        rBSP      = loc0                                                      ;\
        rBSPStore = loc1                                                      ;\
        rRnat     = loc2                                                      ;\
                                                                              ;\
        rpT1      = t1                                                        ;\
        rpT2      = t2                                                        ;\
        rpT3      = t3                                                        ;\
        rpT4      = t4                                                        ;\
        rThread   = t6                                                        ;\
        rApcFlag  = t7                                                        ;\
        rT1       = t8                                                        ;\
        rT2       = t9                                                        ;\
                                                                              ;\
        alloc       rT1 = 0,4,2,0                                             ;\
        movl        rpT1 = KiPcr + PcCurrentThread     /* ->PcCurrentThread */;\
        ;;                                                                    ;\
                                                                              ;\
(pUstk) ld8         rThread = [rpT1]                   /* load thread ptr   */;\
        add         rBSP = TrRsBSP, sp                 /* -> user BSP       */;\
(pKstk) br##.##call##.##spnt brp = KiRestoreTrapFrame                         ;\
        ;;                                                                    ;\
                                                                              ;\
        add         rBSPStore = TrRsBSPSTORE, sp       /* -> user BSP Store */;\
        add         rRnat = TrRsRNAT, sp               /* -> user RNAT      */;\
(pKstk) br##.##spnt Label##ReturnToKernel                                     ;\
        ;;                                                                    ;\
                                                                              ;\
        add         rpT1 = ThApcState+AsUserApcPending, rThread               ;\
        ;;                                                                    ;\
        ld1         rApcFlag = [rpT1], ThAlerted-ThApcState-AsUserApcPending  ;\
        ;;                                                                    ;\
        st1.nta     [rpT1] = zero                                             ;\
        cmp##.##ne  pApc = zero, rApcFlag                                     ;\
        ;;                                                                    ;\
                                                                              ;\
        PSET_IRQL   (pApc, APC_LEVEL)                                         ;\
 (pApc) FAST_ENABLE_INTERRUPTS                                                ;\
 (pApc) mov         out1 = sp                                                 ;\
 (pApc) br##.##call##.##sptk brp = KiApcInterrupt                             ;\
        ;;                                                                    ;\
                                                                              ;\
 (pApc) FAST_DISABLE_INTERRUPTS                                               ;\
        PSET_IRQL   (pApc, zero)                                              ;\
                                                                              ;\
        ld8         rBSP = [rBSP]                      /* user BSP          */;\
        ld8         rBSPStore = [rBSPStore]            /* user BSP Store    */;\
        ld8         rRnat = [rRnat]                    /* user RNAT         */;\
        br##.##call##.##sptk brp = KiRestoreDebugRegisters                    ;\
        ;;                                                                    ;\
                                                                              ;\
        invala                                                                ;\
        br##.##call##.##sptk brp = KiRestoreTrapFrame                         ;\
        ;;                                                                    ;\
                                                                              ;\
                                                                              ;\
Label##CriticalExitCode:                                                      ;\
                                                                              ;\
        rHRscE = h17                                                          ;\
        rHRnat = h18                                                          ;\
        rHBSPStore = h19                                                      ;\
        rHRscD = h20                                                          ;\
        rHRscDelta = h24                                                      ;\
                                                                              ;\
        bsw##.##0                                                             ;\
        ;;                                                                    ;\
                                                                              ;\
        mov         rHRscE = ar##.##rsc                /* save user RSC     */;\
        mov         rHBSPStore = rBSPStore                                    ;\
        mov         rHRscD = RSC_KERNEL_DISABLED                              ;\
                                                                              ;\
        sub         rHRscDelta = rBSP, rBSPStore /* delta = BSP - BSP Store */;\
        ;;                                                                    ;\
        mov         rHRnat  = rRnat                                           ;\
        dep         rHRscD = rHRscDelta, rHRscD, 16, 14  /* set RSC.loadrs  */;\
        ;;                                                                    ;\
                                                                              ;\
        alloc       rTH1 = 0,0,0,0                                            ;\
        mov         ar##.##rsc = rHRscD                /* RSE off       */    ;\
        ;;                                                                    ;\
        loadrs                                         /* pull in user regs */;\
                                                       /* up to tear point */ ;\
        ;;                                                                    ;\
                                                                              ;\
        mov         ar##.##bspstore = rHBSPStore       /* restore user BSP */ ;\
        ;;                                                                    ;\
        mov         ar##.##rnat = rHRnat               /* restore user RNAT */;\
                                                                              ;\
Label##ReturnToKernel:                                                        ;\
                                                                              ;\
(pUstk) mov         ar.rsc = rHRscE                 /* restore user RSC     */;\
        bsw##.##0                                                             ;\
        ;;                                                                    ;\
                                                                              ;\
        add         rHpT2 = TrApUNAT, sp            /* -> previous UNAT     */;\
        add         rHpT1 = TrStFPSR, sp            /* -> previous Preds    */;\
        ;;                                                                    ;\
                                                                              ;\
        ld8         rHUNAT = [rHpT2],TrPreds-TrApUNAT                         ;\
        ld8         rHFPSR = [rHpT1],TrRsPFS-TrStFPSR                         ;\
        ;;                                                                    ;\
                                                                              ;\
        ld8         rHOldPreds = [rHpT2], TrIntSp-TrPreds                     ;\
        ld8         rHPFS = [rHpT1],TrStIIPA-TrRsPFS                          ;\
        ;;                                                                    ;\
                                                                              ;\
        ld8##.##fill rHSp = [rHpT2], TrBrRp-TrIntSp                           ;\
        ld8         rHIIPA = [rHpT1], TrStIIP-TrStIIPA                        ;\
        ;;                                                                    ;\
                                                                              ;\
        mov         ar##.##fpsr = rHFPSR            /* restore FPSR         */;\
        ld8         rHIIP = [rHpT1], TrStIPSR-TrStIIP  /* load IIP          */;\
        mov         pr = rHOldPreds, -1             /* restore preds        */;\
                                                                              ;\
        mov         ar##.##unat = rHUNAT            /* restore UNAT         */;\
        ld8         rHBrp = [rHpT2], TrStIFS-TrBrRp                           ;\
        mov         ar##.##pfs = rHPFS              /* restore PFS          */;\
        ;;                                                                    ;\
                                                                              ;\
        ld8         rHIFS = [rHpT2]                 /* load IFS             */;\
        ld8         rHIPSR = [rHpT1]                /* load IPSR            */;\
                                                                              ;\
        rsm         1 << PSR_IC                     /* reset ic bit         */;\
        ;;                                                                    ;\
        srlz##.##d                                  /* must serialize       */;\
        ;;                                                                    ;\
        mov         brp = rHBrp                     /* restore brp          */;\
                                                                              ;\
/*                                                                          */;\
/* Restore status registers                                                 */;\
/*                                                                          */;\
                                                                              ;\
        mov         cr##.##ipsr = rHIPSR        /* restore previous IPSR    */;\
        mov         cr##.##iip = rHIIP          /* restore previous IIP     */;\
                                                                              ;\
        mov         cr##.##ifs = rHIFS          /* restore previous IFS     */;\
        mov         cr##.##iipa = rHIIPA        /* restore previous IIPA    */;\
        ;;                                                                    ;\
                                                                              ;\
/*                                                                          */;\
/* Resume at point of interruption (rfi must be at end of instruction group)*/;\
/*                                                                          */;\
        mov         sp = rHSp                   /* restore sp               */;\
        mov         h17 = r0                    /* clear TB loop count      */;\
        rfi                                                                   ;\
        ;;


//++
// Routine:
//
//       USER_APC_CHECK
//
// Routine Description:
//
//       Common handler code for requesting
//       pending APC if returning to user mode.
//
// Arguments:
//
// Return Value:
//
//       None
//
// Note: 
//
//       On entry: interrrupts disabled, sp -> trap frame
//       On exit:
//       MS preprocessor requires /* */ style comments below
//--


#define USER_APC_CHECK                                                        ;\
                                                                              ;\
/*                                                                          */;\
/* Check for pending APC's                                                  */;\
/*                                                                          */;\
                                                                              ;\
        movl        t22=KiPcr + PcCurrentThread        /* ->PcCurrentThread */;\
        ;;                                                                    ;\
                                                                              ;\
        LDPTR       (t22,t22)                                                 ;\
        ;;                                                                    ;\
        add         t22=ThApcState+AsUserApcPending, t22 /* -> pending flag */;\
        ;;                                                                    ;\
                                                                              ;\
        ld1         t8 = [t22], ThAlerted-ThApcState-AsUserApcPending         ;\
        ;;                                                                    ;\
        st1         [t22] = zero                                              ;\
        cmp##.##ne  pApc = zero, t8                  /* pApc = 1 if pending */;\
        ;;                                                                    ;\
                                                                              ;\
        PSET_IRQL   (pApc, APC_LEVEL)                                         ;\
(pApc)  FAST_ENABLE_INTERRUPTS                                                ;\
(pApc)  mov         out1 = sp                                                 ;\
(pApc)  br##.##call##.##sptk brp = KiApcInterrupt                             ;\
        ;;                                                                    ;\
(pApc)  FAST_DISABLE_INTERRUPTS                                               ;\
        PSET_IRQL   (pApc, zero)


//++
// Routine:
//
//       BSTORE_SWITCH
//
// Routine Description:
//
//       Common handler code for switching to user backing store, if
//       returning to user mode.
//
// On entry:
//
//      sp: pointer to trap frame
//
// On exit:
//
//      on user backing store, can't afford another alloc of any frame size
//      other than zero.  otherwise, the kernel may panic.
//
// Return Value:
//
//       None
//
// Note: 
//
//       MS preprocessor requires /* */ style comments below
//--


#define BSTORE_SWITCH                                                         ;\
/*                                                                          */;\
/* Set sp to trap frame and switch to kernel banked registers               */;\
/*                                                                          */;\
        rRscD     = t11                                                       ;\
        rpT2      = t12                                                       ;\
        rRNAT     = t13                                                       ;\
        rBSPStore = t14                                                       ;\
        rRscDelta = t15                                                       ;\
        rpT1      = t16                                                       ;\
                                                                              ;\
                                                                              ;\
        add       rpT1 = TrRsRNAT, sp                  /* -> user RNAT      */;\
        add       rpT2 = TrRsBSPSTORE, sp              /* -> user BSP Store */;\
        mov       rRscD = RSC_KERNEL_DISABLED                                 ;\
        ;;                                                                    ;\
                                                                              ;\
/*                                                                          */;\
/* Switch to user BSP -- put in load intensive mode to overlap RS restore   */;\
/* with volatile state restore.                                             */;\
/*                                                                          */;\
                                                                              ;\
        ld8       rRNAT = [rpT1], TrRsBSP-TrRsRNAT     /* user RNAT     */    ;\
        ld8       rBSPStore = [rpT2]                   /* user BSP Store*/    ;\
        ;;                                                                    ;\
                                                                              ;\
        alloc     t22 = 0,0,0,0                                               ;\
        ld8       rRscDelta = [rpT1]                   /* user BSP      */    ;\
        ;;                                                                    ;\
        sub       rRscDelta = rRscDelta, rBSPStore     /* delta = BSP - BSP Store */;\
        ;;                                                                    ;\
                                                                              ;\
        invala                                                                ;\
        dep       rRscD = rRscDelta, rRscD, 16, 14     /* set RSC.loadrs    */;\
        ;;                                                                    ;\
                                                                              ;\
        mov       ar##.##rsc = rRscD                   /* RSE off       */    ;\
        ;;                                                                    ;\
        loadrs                                         /* pull in user regs */;\
                                                       /* up to tear point */ ;\
        ;;                                                                    ;\
                                                                              ;\
        mov       ar##.##bspstore = rBSPStore          /* restore user BSP */ ;\
        ;;                                                                    ;\
        mov       ar##.##rnat = rRNAT                  /* restore user RNAT */


//++
// Routine:
//
//       NORMAL_KERNEL_EXIT
//
// Routine Description:
//
//       Common handler code for restoring previous state and rfi.
//
// On entry:
//
//      sp: pointer to trap frame
//      ar.unat: contains Nat for previous sp (restored by ld8.fill)
//
// Return Value:
//
//       None
//
// Note: 
//
//      Uses just the kernel banked registers (h16-h31)
//
//       MS preprocessor requires /* */ style comments below
//--

#define NORMAL_KERNEL_EXIT                                                    ;\
                                                                              ;\
        add         rHpT2 = TrApUNAT, sp            /* -> previous UNAT     */;\
        add         rHpT1 = TrStFPSR, sp            /* -> previous Preds    */;\
        ;;                                                                    ;\
                                                                              ;\
        ld8         rHUNAT = [rHpT2],TrPreds-TrApUNAT                         ;\
        ld8         rHFPSR = [rHpT1],TrRsPFS-TrStFPSR                         ;\
        ;;                                                                    ;\
                                                                              ;\
        ld8         rHOldPreds = [rHpT2], TrIntSp-TrPreds                     ;\
        ld8         rHPFS = [rHpT1],TrStIIPA-TrRsPFS                          ;\
        ;;                                                                    ;\
                                                                              ;\
        ld8##.##fill rHSp = [rHpT2], TrBrRp-TrIntSp                           ;\
        ld8         rHIIPA = [rHpT1], TrStIIP-TrStIIPA                        ;\
        ;;                                                                    ;\
                                                                              ;\
        mov         ar##.##fpsr = rHFPSR            /* restore FPSR         */;\
        ld8         rHIIP = [rHpT1], TrStIPSR-TrStIIP  /* load IIP          */;\
        mov         pr = rHOldPreds, -1             /* restore preds        */;\
                                                                              ;\
        mov         ar##.##unat = rHUNAT            /* restore UNAT         */;\
        ld8         rHBrp = [rHpT2], TrStIFS-TrBrRp                           ;\
        mov         ar##.##pfs = rHPFS              /* restore PFS          */;\
        ;;                                                                    ;\
                                                                              ;\
        ld8         rHIFS = [rHpT2]                 /* load IFS             */;\
        ld8         rHIPSR = [rHpT1]                /* load IPSR            */;\
                                                                              ;\
        rsm         1 << PSR_IC                     /* reset ic bit         */;\
        ;;                                                                    ;\
        srlz##.##d                                  /* must serialize       */;\
        ;;                                                                    ;\
        mov         brp = rHBrp                     /* restore brp          */;\
                                                                              ;\
/*                                                                          */;\
/* Restore status registers                                                 */;\
/*                                                                          */;\
                                                                              ;\
        mov         cr##.##ipsr = rHIPSR        /* restore previous IPSR    */;\
        mov         cr##.##iip = rHIIP          /* restore previous IIP     */;\
                                                                              ;\
        mov         cr##.##ifs = rHIFS          /* restore previous IFS     */;\
        mov         cr##.##iipa = rHIIPA        /* restore previous IIPA    */;\
        ;;                                                                    ;\
                                                                              ;\
/*                                                                          */;\
/* Resume at point of interruption (rfi must be at end of instruction group)*/;\
/*                                                                          */;\
        mov         sp = rHSp                   /* restore sp               */;\
        mov         h17 = r0                    /* clear TB loop count      */;\
        rfi                                                                   ;\
        ;;

//++
// Routine:
//
//       GET_INTERRUPT_VECTOR(pGet, rVector)
//
// Routine Description:
//
//       Hook to get the vector for an interrupt. Currently just
//       reads the Interrupt Vector Control Register.
//
// Agruments:
//
//       pGet:    Predicate: if true then get, else skip.
//       rVector: Register for the vector number.
//
// Return Value:
//
//       The vector number of the highest priority pending interrupt.
//       Vectors number is an 8-bit value. All other bits 0.
//
//--

#define GET_INTERRUPT_VECTOR(pGet,rVector)                         \
        srlz##.##d                                                ;\
        ;;                                                         \
(pGet)  mov         rVector = cr##.##ivr


//
// Interruption Vector Table. First level interruption vectors.
// This section must be 32K aligned. The special section ".drectve"
// is used to pass the align command to the linker.
//
        .section .drectve, "MI", "progbits"
        string "-section:.ivt,,align=0x8000"
        
        .section .ivt = "ax", "progbits"
KiIvtBase::                                     // symbol for start of IVT

//++
//
// KiVhptTransVector 
//
// Cause:       The hardware VHPT walker encountered a TLB miss while attempting to
//              reference the virtuall addressed VHPT linear table.
//
// Parameters:  cr.iip  - address of bundle for which the hardware VHPT walker was
//              trying to resolve the TLB miss
//
//              cr.ipsr - copy of PSR at the time of the fault
//
//              cr.idtr - default translation inforamtion for the address that caused
//              a VHPT translation fault
//
//              cr.ifa  - original faulting address
//
//              cr.isr  - original faulting status information
//              
// Handle:      Extracts the PDE index from cr.iha (PTE address in VHPT) and 
//              generates a PDE address by adding to VHPT_DIRBASE. When accesses 
//              a page directory entry (PDE), there might be a TLB miss on the 
//              page directory table and returns a NaT on ld8.s. If so, branches 
//              to KiPageDirectoryTableFault. If the page-not-present bit of the 
//              PDE is not set, branches to KiPageNotPresentFault. Otherwise, 
//              inserts the PDE entry into the data TC (translation cache).
//
//--
                                               
        VECTOR_ENTRY(0x0000, KiVhptTransVector, cr.ifa)

#if 1
        rva     = h24
        riha    = h25
        rpr     = h26 
        rPte    = h27
        rPte2   = h28
        rps     = h29
        risr    = h30
        rCache  = h28

        mov             riha = cr.iha           // M
        mov             rva = cr.ifa            // M
        mov             rpr = pr                // I

        mov             risr = cr.isr           // M
        ;;

        ld8.s           rPte = [riha]           // M
        tbit.z          pt3, pt4 = risr, ISR_X  // I        

        ;;
        tnat.nz         pt0 = rPte              // I
        tbit.z          pt1 = rPte, 0           // I

(pt0)   br.cond.spnt    KiPageTableFault        // B
(pt1)   br.cond.spnt    KiPteNotPresentFault    // B
        extr.u          rCache = rPte, 2, 3     // I
        ;;

        cmp.eq          pt5 = 1, rCache         // A
(pt5)   br.cond.spnt    KiPageTableFault        // B
        ;;

.pred.rel "mutex",pt3,pt4
(pt4)   itc.i           rPte                    // M
(pt3)   itc.d           rPte                    // M             
        ;;

#if !defined(NT_UP)
                
        ld8.s           rPte2 = [riha]          // M
        mov             rps = PAGE_SHIFT << PS_SHIFT // I
        cmp.ne          pt0 = zero, zero             // I   
        ;;

        cmp.ne.or       pt0 = rPte2, rPte       // M
        tnat.nz.or      pt0 = rPte2             // I

        ;;
(pt0)   ptc.l           rva, rps                // M
#endif
        mov             pr = rpr, -1            // I
        rfi;;                                   // B

#else
        rva     =       h24
        riha    =       h25
        rpr     =       h26
        rpPde   =       h27
        rPde    =       h28
        rPde2   =       h29
        rps     =       h30

        mov             riha = cr.iha           // M
        mov             rva = cr.ifa            // M
        mov             rpr = pr                // I
        ;;
        
        thash           rpPde = riha            // M
        ;;      

        ld8.s           rPde = [rpPde]          // M, load PDE
        ;;

        tnat.nz         pt0, p0 = rPde          // I
        tbit.z          pt1, p0 = rPde, 0       // I, if non-present page fault 

(pt0)   br.cond.spnt    KiPageDirectoryFault    // B
(pt1)   br.cond.spnt    KiPdeNotPresentFault    // B

        mov             cr.ifa = riha           // M
        ;;
        itc.d           rPde                    // M
        ;;

#if !defined(NT_UP)
        ld8.s           rPde2 = [rpPde]         // M
        mov             rps = PAGE_SHIFT << PS_SHIFT // I
        cmp.ne          pt0 = zero, zero             // I   
        ;;
            
        cmp.ne.or       pt0 = rPde2, rPde     // M, if PTEs are different 
        tnat.nz.or      pt0 = rPde2           // I          

        ;;
(pt0)   ptc.l           riha, rps              // M, purge it
#endif                        
        mov             pr = rpr, -1            // I
        rfi                                     // B
        ;;
#endif

        VECTOR_EXIT(KiVhptTransVector)
                                               

//++
//
// KiInstTlbVector
//
// Cause:       The VHPT walker aborted the search for the instruction translation.
//              
// Parameters:  cr.iip  - address of bundle for which the hardware VHPT walker was
//              trying to resolve the TLB miss
//
//              cr.ipsr - copy of PSR at the time of the fault
//
//              cr.iha  - PTE address in the VHPT which the VHPT walker was attempting to 
//              reference
//
//              cr.iitr - default translation inforamtion for the address that caused
//              a instruction TLB miss        
//
//              cr.isr  - faulting status information
//              
// Handle:      As the VHPT has aborted the search or the implemenation does not 
//              support the h/w page table walk, the handler needs to emulates the 
//              function. Since the offending PTE address is already available 
//              with cr.iha, the handler can access the PTE without performing THASH.
//              Accessing a PTE with ld8.s may return a NaT. If so, it branches to 
//              KiPageTableFault. If the page-not-present bit of the PTE is not set,
//              it branches to KiPageFault.
//        
// Comments:    Merced never casues this fault since it never abort the search on the 
//              VHPT.
//
//--

        VECTOR_ENTRY(0x0400, KiInstTlbVector, cr.iipa)
        
        rva     = h24
        riha    = h25
        rpr     = h26 
        rPte    = h27
        rPte2   = h28
        rps     = h29
        rCache  = h28
        

        mov             riha = cr.iha           // M
        mov             rva = cr.ifa            // M
        mov             rpr = pr                // I
        ;;

        ld8.s           rPte = [riha]           // M
        ;;
        
        tnat.nz         pt0, p0 = rPte          // I
        tbit.z          pt1, p0 = rPte, 0       // I

(pt0)   br.cond.spnt    KiPageTableFault        // B
(pt1)   br.cond.spnt    KiPteNotPresentFault    // B

        extr.u          rCache = rPte, 2, 3     // I
        ;;

        cmp.eq          pt3 = 1, rCache         // A
(pt3)   br.cond.spnt    Ki4KInstTlbFault        // B
 
        itc.i           rPte                    // M
        ;;

#if !defined(NT_UP)
                
        ld8.s           rPte2 = [riha]          // M
        mov             rps = PAGE_SHIFT << PS_SHIFT // I
        cmp.ne          pt0 = zero, zero             // I   
        ;;

        cmp.ne.or       pt0 = rPte2, rPte       // M
        tnat.nz.or      pt0 = rPte2             // I

        ;;
(pt0)   ptc.l           rva, rps                // M
#endif
        mov             pr = rpr, -1            // I
        rfi;;                                   // B

        VECTOR_EXIT(KiInstTlbVector)

                
//++
//
// KiDataTlbVector
//
// Cause:       The VHPT walker aborted the search for the data translation.
//              
// Parameters:  cr.iip  - address of bundle for which the hardware VHPT walker was
//              trying to resolve the TLB miss
//
//              cr.ipsr - copy of PSR at the time of the fault
//
//              cr.iha  - PTE address in the VHPT which the VHPT walker was attempting to 
//              reference
//
//              cr.idtr - default translation inforamtion for the address that caused
//              a data TLB miss
//
//              cr.ifa  - address that caused a data TLB miss
//
//              cr.isr  - faulting status information
//              
// Handle:      As the VHPT has aborted the search or the implemenation does not 
//              support the h/w page table walk, the handler needs to emulates the 
//              function. Since the offending PTE address is already available 
//              with cr.iha, the handler can access the PTE without performing THASH.
//              Accessing a PTE with ld8.s may return a NaT. If so, it branches to 
//              KiPageTableFault. If the page-not-present bit of the PTE is not set,
//              it branches to KiPageFault.
//        
// Comments:    Merced never casues instruction TLB faults since the VHPT search always
//              sucesses.
//              
//--

        VECTOR_ENTRY(0x0800, KiDataTlbVector, cr.ifa)

        rva     = h24
        riha    = h25
        rpr     = h26
        rPte    = h27
        rPte2   = h28
        rps     = h29
        rCache  = h28

        mov             riha = cr.iha           // M
        mov             rva = cr.ifa            // M
        mov             rpr = pr                // I
        ;;

        ld8.s           rPte = [riha]           // M
        ;;
        
        tnat.nz         pt0, p0 = rPte          // I
        tbit.z          pt1, p0 = rPte, 0       // I

(pt0)   br.cond.spnt    KiPageTableFault        // B
(pt1)   br.cond.spnt    KiPteNotPresentFault    // B

        extr.u          rCache = rPte, 2, 3     // I
        ;;

        cmp.eq          pt3 = 1, rCache         // A
(pt3)   br.cond.spnt    Ki4KDataTlbFault        // B
 
        itc.d           rPte                    // M
        ;;

#if !defined(NT_UP)
        ld8.s           rPte2 = [riha]          // M
        mov             rps = PAGE_SHIFT << PS_SHIFT // I
        cmp.ne          pt0 = zero, zero             // I   
        ;;

        cmp.ne.or       pt0 = rPte2, rPte       // M
        tnat.nz.or      pt0 = rPte2             // I
        ;;

(pt0)   ptc.l           rva, rps                // M
#endif
        mov             pr = rpr, -1            // I
        rfi;;                                   // B

        VECTOR_EXIT(KiDataTlbVector)

        
//++
//
// KiAltTlbVector
//
// Cause:       There was a TLB miss for instruction execution and the VHPT 
//              walker was not enabled for the referenced region.
//              
// Parameters:  cr.iip  - address of bundle that caused a TLB miss
//
//              cr.ipsr - copy of PSR at the time of the fault
//
//              cr.idtr - default translation inforamtion for the address that caused
//              the fault.
//
//              cr.isr  - faulting status information
//              
// Handle:      Currently, NT does not have any use of this vector.
//              
//--
        
        VECTOR_ENTRY(0x0c00, KiAltInstTlbVector, cr.iipa)

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiMemoryFault)

        VECTOR_EXIT(KiAltInstTlbVector)



//++
//
// KiAltDataTlbVector
//
// Cause:       There was a data TLB miss and the VHPT walker was not enabled for 
//              the referenced region.
//              
// Parameters:  cr.iip  - address of bundle that caused a TLB miss
//
//              cr.ipsr - copy of PSR at the time of the fault
//
//              cr.idtr - default translation inforamtion for the address that caused
//              the fault.
//
//              cr.isr  - faulting status information
//              
// Handle:      Currently, NT does not have any use of this vector.
//              
//--
        
        VECTOR_ENTRY(0x1000, KiAltDataTlbVector, cr.ifa)

        rva     =       h24     
        riha    =       h25
        rpr     =       h26     
        rPte    =       h27
        rIPSR   =       h28      
        rISR    =       h29    
        rVrn    =       h31

        mov             rva = cr.ifa                    // M
        mov             rpr = pr                        // I
        ;;

        shr.u           rVrn = rva, VRN_SHIFT           // I, get VPN
        ;;      
        cmp.ne          pt2 = KSEG3_VRN, rVrn           // M/I
(pt2)   br.cond.spnt    no_kseg3                        // B

        mov             rISR = cr.isr                   // M
        movl            rPte = VALID_KERNEL_PTE         // L

        mov             rIPSR = cr.ipsr                 // M
        shr.u           rva = rva, PAGE_SHIFT           // I
        ;;
        tbit.z          pt2, pt3 = rISR, ISR_SP         // I
        dep.z           rva = rva, PAGE_SHIFT, 32       // I
        ;;
        or              rPte = rPte, rva                // I
        dep             rIPSR = 1, rIPSR, PSR_ED, 1     // I
        ;;

(pt2)   itc.d           rPte                            // M
        ;;
(pt3)   ptc.l           rva, rps                        // M
(pt3)   mov             cr.ipsr = rIPSR                 // M        
        ;;

        mov             pr = rpr, -1                    // I
        rfi                                             // B
        ;;

no_kseg3:        
        mov             pr = rpr, -1

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiMemoryFault)
        
        VECTOR_EXIT(KiAltDataTlbVector)

        
                
//++
//
// KiNestedTlbVector 
//
// Cause:       Instruction/Data TLB miss handler encountered a TLB miss while 
//              attempting to reference the PTE in the virtuall addressed 
//              VHPT linear table.
//
// Parameters:  cr.iip  - address of bundle for which the hardware VHPT walker was
//              trying to resolve the TLB miss
//
//              cr.ipsr - copy of PSR at the time of VHPT translation fault
//
//              cr.iha  - address in VHPT which the VHPT walker was attempting to 
//              reference
//
//              cr.idtr - default translation inforamtion for the virtual address
//              contained in cr.iha  
//
//              cr.ifa  - original faulting address
//
//              cr.isr  - faulting status information
//              
//              h16(riha) - PTE address in the VHPT which caused the Nested miss
//              
// Handle:      Currently, there is no use for Nested TLB vector. This should be
//              a bug fault. Call KiPanicHandler.
//
//--
                                               
        VECTOR_ENTRY(0x1400, KiNestedTlbVector, cr.ifa)
        
        ALLOCATE_TRAP_FRAME
        br.sptk     KiPanicHandler

        VECTOR_EXIT(KiNestedTlbVector)

//++
//
// KiInstKeyMissVector
//
// Cause:       There was a instruction key miss in the translation. Since the
//              architecture allows an implementation to choose a unified TC 
//              structure, the hyper space translation brought by the data 
//              access-bit may cause a instruction key miss fault.  Only erroneous
//              user code tries to execute the NT page table and hyper space.
//
// Parameters:  cr.iip  - address of bundle which caused a instruction key miss fault 
//
//              cr.ipsr - copy of PSR at the time of the data key miss fault 
//
//              cr.idtr - default translation inforamtion of the address that caused 
//              the fault.
//
//              cr.isr  - faulting status information
//              
// Handle:      Save the whole register state and call MmAccessFault().
//              
//--

        VECTOR_ENTRY(0x1800, KiInstKeyMissVector, cr.iipa)

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiMemoryFault)
                 
        VECTOR_EXIT(KiInstKeyMissVector)

  
              
//++
//
// KiDataKeyMissVector
//
// Cause:       The referenced translation had the different key ID from the one
//              specified the key permission register. This is an indication of 
//              TLB miss on the NT page table and hyperspace.
//
// Parameters:  cr.iip  - address of bundle which caused the fault 
//
//              cr.ipsr - copy of PSR at the time of the data key miss fault 
//
//              cr.idtr - default translation inforamtion of the address that caused 
//              the fault.
//
//              cr.ifa  - address that caused a data key miss
//
//              cr.isr  - faulting status information
//              
// Handle:      The handler needs to purge the faulting translation and install
//              a new PTE by loading it from the VHPT.  The key ID of the IDTR 
//              for the installing translation should be modified to be the same 
//              ID as the local region ID.  This effectively creates a local 
//              space within the global kernel space.
//
//--

        VECTOR_ENTRY(0x1c00, KiDataKeyMissVector, cr.ifa)

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiMemoryFault)

        VECTOR_EXIT(KiDataKeyMissVector)

//++
//
// KiDirtyBitVector
//
// Cause:       The refereced data translation did not have the dirty-bit set and 
//              a write operation was made to this page.
//
// Parameters:  cr.iip  - address of bundle which caused a dirty bit fault
//
//              cr.ipsr - copy of PSR at the time of a data access fault
//
//              cr.idtr - default translation inforamtion for the address that 
//              caused the fault
//
//              cr.ifa  - referenced data address that caused the dirty-bit fault
//
//              cr.isr  - faulting status information
//              
// Handle:      Save the whole register state and call MmAccessFault().
//              
// Comments:    There is always a TLB coherency problem on a multiprocessor 
//              system. Rather than implementing an atomic operation of setting 
//              dirty-bit within this handler, the handler instead calls the high 
//              level C routine, MmAccessFault(), to perform locking the page table 
//              and setting the dirty-bit of the PTE. 
//
//              It is too much effort to implement the atomic operation of setting 
//              the dirty-bit using cmpxchg; a potential nested TLB miss on load/store 
//              and restoring ar.ccv complicate the design of the handler.
//
//--
        
        VECTOR_ENTRY(0x2000, KiDirtyBitVector, cr.ifa)
        
        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiMemoryFault)
        
        VECTOR_EXIT(KiDirtyBitVector)

//++
//
// KiInstAccessBitVector
//
// Cause:       There is a access-bit fault on the instruction translation. This only
//              happens if the erroneous user mistakenly accesses the NT page table and 
//              hyper space. 
//
// Parameters:  cr.iip  - address of bundle which caused a instruction access bit fault
//
//              cr.ipsr - copy of PSR at the time of a data access fault
//
//              cr.idtr - default translation inforamtion for the address that 
//              caused the fault
//
//              cr.ifa  - referenced data address that caused the data access-bit fault
//
//              cr.isr  - faulting status information
//              
// Handle:      Save the whole register state and call MmAccessFault().
//              
//--

        VECTOR_ENTRY(0x2400, KiInstAccessBitVector, cr.iipa)
    
        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiMemoryFault)

        VECTOR_EXIT(KiInstAccessBitVector)

//++
//
// KiDataBitAccessVector
//
// Cause:       The reference-bit in the the referenced translation was zero, 
//              indicating there was a TLB miss on the NT page table or hyperspace.
//
// Parameters:  cr.iip  - address of bundle which caused a data access bit fault
//
//              cr.ipsr - copy of PSR at the time of a data access fault
//
//              cr.idtr - default translation inforamtion for the address that 
//              caused the fault
//
//              cr.ifa  - referenced data address that caused the data access-bit fault
//
//              cr.isr  - faulting status information
//              
// Handle:      The reference-bit is used to fault on PTEs for the NT page table and  
//              hyperspace. On a data access-bit fault, the handler needs to change the
//              the default key ID of the IDTR to be the local key ID. This effectively
//              creates the local space within the global kernel space.
//
//--

        VECTOR_ENTRY(0x2800, KiDataAccessBitVector, cr.ifa)

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiMemoryFault)
            
        VECTOR_EXIT(KiDataBitAccessVector)
       
//--------------------------------------------------------------------
// Routine:
//
//       KiBreakVector
//
// Description:
//
//       Interruption vector for break instruction.
//
// On entry:
//
//       IIM contains break immediate value:
//                 -- BREAK_SYSCALL -> standard system call
//       interrupts disabled
//       r16-r31 switched to kernel bank
//       r16-r31 all available since no TLB faults at this point
//
// Return value:
//
//       if system call, sys call return value in v0.
//
// Process:
//--------------------------------------------------------------------

        VECTOR_ENTRY(0x2C00, KiBreakVector, cr.iim)

        mov       rHIIM = cr.iim               // get break value
        movl      rTH1 = KiPcr+PcSavedIIM
        ;;
        st8       [rTH1] = rHIIM

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiOtherBreakException)

//
// Do not return from handler
//

        VECTOR_EXIT(KiBreakVector)

//--------------------------------------------------------------------
// Routine:
//
//       KiExternalInterruptVector
//
// Routine Description:
//
//       Interruption vector for External Interrrupt
//
// On entry:
//
//       interrupts disabled
//       r16-r31 switched to kernel bank
//
// Return value:
//
//       none
//
// Process:
//--------------------------------------------------------------------

        VECTOR_ENTRY(0x3000, KiExternalInterruptVector, r0)

        ALLOCATE_TRAP_FRAME
        ;;
        SAVE_INTERRUPTION_STATE(Keih_SaveTrapFrame)
        br.many     KiExternalInterruptHandler
        ;;

//
// Do not return (rfi from handler)
//

        VECTOR_EXIT(KiExternalInterruptVector)

//++
//
// KiPageNotPresentVector
//
// Cause:       The translation for the referenced page did not have a present-bit
//              set.         
//              
// Parameters:  cr.iip  - address of bundle which caused a page not present fault
//
//              cr.ipsr - copy of PSR at the time of a page not present ault
//
//              cr.idtr - default translation inforamtion for the address that 
//              caused the fault
//
//              cr.ifa  - referenced data address if the fault occurred on the data 
//              reference
//
//              cr.isr  - faulting status information
//              
// Handle:      This is the page fault. The handler saves the register context and 
//              calls MmAccessFault().
//
//--

        VECTOR_ENTRY(0x5000, KiPageNotPresentVector, cr.ifa)


        rva     =       h24
        riha    =       h25
        rpr     =       h26
        rps     =       h27

        rPfn    =       h28
        rpAte   =       h28
        rAte    =       h29
        rAteEnd =       h30
        rAteBase =      h31
        rAteMask =      h22
        
        rK0Base =       h30
        rK2Base =       h31

        pIndr = pt1
        
        mov             rva = cr.ifa            // M        
        mov             rpr = pr                // I

        mov             rps  = PS_4K << PS_SHIFT // M  
        movl            rK0Base = KSEG0_BASE     // L
        ;;

        cmp.geu         pt3, p0 = rva, rK0Base  // M
        movl            rK2Base = KSEG2_BASE    // L
        ;;

(pt3)   cmp.ltu         pt3, p0 = rva, rK2Base    // M
        movl            rAteBase = ALT4KB_BASE    // L

        thash           riha = rva                // M      
(pt3)   br.cond.spnt    KiKseg0Fault              // B

        mov             rAteMask = ATE_MASK0      // M
        shr.u           rPfn = rva, PAGE4K_SHIFT  // I
        ;;

        shladd  rpAte = rPfn, PTE_SHIFT, rAteBase // M
        movl    rAteEnd = ALT4KB_END              // L
        ;;

        ld8.s           rAte = [rpAte]            // M
        andcm           rAteMask = -1, rAteMask   // I
        cmp.ltu         pIndr = rpAte, rAteEnd    // I
        ;;

        or              rAteMask = rAte, rAteMask // M
        tnat.z.and      pIndr = rAte              // I
        tbit.nz.and     pIndr = rAte, PTE_VALID   // I

        tbit.nz.and     pIndr = rAte, PTE_ACCESS  // M
        tbit.nz.and     pIndr = rAte, ATE_INDIRECT // I
(pIndr) br.cond.spnt    KiPteIndirectFault        
        ;;

        ptc.l           rva, rps                // M
        movl            h22 = KiPcr+PcSavedIFA

        movl            rHHandler = KiMemoryFault
        ;;

        st8             [h22] = rva
        mov             pr = rpr, -1            // I
        br.sptk         KiGenericExceptionHandler
        ;;
        
        VECTOR_EXIT(KiPageNotPresentVector)


        
//++
//
// KiKeyPermVector
//
// Cause:       Read, write or execution key permissions were violated. 
//              
// Parameters:  cr.iip  - address of bundle which caused a key permission fault
//
//              cr.ipsr - copy of PSR at the time of a key permission fault
//
//              cr.idtr - default translation inforamtion for the address that 
//              caused the fault
//
//              cr.ifa  - referenced data address if the key permission occurred on 
//              the data reference
//
//              cr.isr  - faulting status information
//              
// Handle:      This should not happen.  The EM/NT does not utilize the key permissions.
//              The handler saves the register state and calls the bug check.
//
//--

        VECTOR_ENTRY(0x5100, KiKeyPermVector, cr.ifa)

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiMemoryFault)
        
        VECTOR_EXIT(KiKeyPermVector)
        

                
//++
//
// KiInstAccessRightsVector
//
// Cause:       The referenced page had a access rights violation.   
//
// Parameters:  cr.iip  - address of bundle which caused a data access bit fault
//
//              cr.ipsr - copy of PSR at the time of a data access fault
//
//              cr.idtr - default translation inforamtion for the address that 
//              caused the fault
//
//              cr.ifa  - referenced data address that caused the data ccess-bit fault
//
//              cr.isr  - faulting status information
//              
// Handle:      The handler saves the register context and calls MmAccessFault().
//
//--

        VECTOR_ENTRY(0x5200, KiInstAccessRightsVector, cr.iipa)

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiMemoryFault)
        
        VECTOR_EXIT(KiInstAccessRightsVector)


        
//++
//
// KiDataAccessRightsVector
//
// Cause:       The referenced page had a data access rights violation.
//
// Parameters:  cr.iip  - address of bundle which caused a data access rights fault
//
//              cr.ipsr - copy of PSR at the time of a data access rights fault
//
//              cr.idtr - default translation inforamtion for the address that 
//              caused the fault
//
//              cr.ifa  - referenced data address that caused the data access rights 
//              fault
//
//              cr.isr  - faulting status information
//              
// Handle:      The handler saves the register context and calls MmAccessFault().
//        
//--

        VECTOR_ENTRY(0x5300, KiDataAccessRightsVector, cr.ifa)

#if 0
        rva     =       h24
        rpPte   =       h25
        rpr     =       h26
        rUserHigh =     h29
        rps     =       h27

        pBr     =       pt0
        pOr     =       pt4
        pShd    =       pt5


        mov     rva = cr.ifa        
        movl    rUserHigh = 0x80000000
        mov     rpr = pr
        mov     rps = PS_4K << PS_SHIFT
        ;;
        mov     cr.itir = rps
        thash   rpPte = rva 
//
// Check to see if it is a user access to a kernel page
//

        rPfn    =       h28
        rpAte   =       h28
        rAte    =       h29
        rPte    =       h30
        rAltBase =      h31
        rPte0   =       h31

        shr.u   rPfn = rva, PAGE4K_SHIFT
        movl    rAltBase = ALT4KB_BASE
        cmp.leu         pBr = rva, rUserHigh
        ;;

        shladd  rpAte  = rPfn, PTE_SHIFT, rAltBase
        ;;

        ld8.s   rPte = [rpPte]
        ld8.s   rAte = [rpAte]
        ;;

        tnat.z.and      pBr = rPte
        tnat.z.and      pBr = rAte

        tbit.nz.and     pBr = rPte, PTE_VALID
        tbit.nz.and     pBr = rAte, PTE_VALID
        tbit.nz.and     pBr = rAte, PTE_ACCESS 
        tbit.nz.and     pBr = rAte, PTE_WRITE   // check to see if ATE has WRITE
        tbit.nz         pShd, pOr = rAte, ATE_SHARED  // check to see if ATE has SHARED 
        dep             rPte0 = 1, rPte, PTE_WRITE, 1       // perform logical OR

(pBr)   br.cond.spnt   KiInstall_4kpte
        ;;

        mov             pr = rpr, -1
#endif
        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiMemoryFault)

        VECTOR_EXIT(KiDataAccessRightsVector)

//--------------------------------------------------------------------
// Routine:
//
//       KiGeneralExceptionsVector
//
// Description:
//
//       Interruption vector for General Exceptions
//
// On entry:
//       interrupts disabled
//       r16-r31 switched to kernel bank
//
// Return value:
//
//       none
//
// Process:
//--------------------------------------------------------------------

        VECTOR_ENTRY(0x5400, KiGeneralExceptionsVector, cr.isr)

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiGeneralExceptions)

//
// Do not return (rfi from handler)
//

        VECTOR_EXIT(KiGeneralExceptionsVector)

//--------------------------------------------------------------------
// Routine:
//
//       KiDisabledFpRegisterVector
//
// Description:
//
//       Interruption vector for Disabled FP-register vector
//
// On entry:
//       interrupts disabled
//       r16-r31 switched to kernel bank
//
// Return value:
//
//       none
//
// Process:
//--------------------------------------------------------------------

        VECTOR_ENTRY(0x5500, KiDisabledFpRegisterVector, cr.isr)

        mov      rHIPSR = cr.ipsr
        mov      rHIIP = cr.iip
        cover
        ;;

        mov      rHIFS = cr.ifs
        extr.u   rTH1 = rHIPSR, PSR_CPL, PSR_CPL_LEN
        mov      rHOldPreds = pr
        ;;

        cmp4.eq  pKrnl, pUser = PL_KERNEL, rTH1
        ;;
(pUser) tbit.z.unc pt0, pt1 = rHIPSR, PSR_DFH    // if dfh not set, 
                                                 //     dfl must be set
        ;;

 (pt1)  ssm      1 << PSR_IC                     // set ic bit
 (pt1)  mov      rHDfhPFS = ar.pfs
 (pt1)  mov      rHDfhBrp = brp
        ;;

 (pt1)  br.call.sptk.many brp = KiRestoreHigherFPVolatile
 (pt0)  br.spnt.few Kdfrv10
        ;;

        rsm      1 << PSR_IC                     // reset ic bit
        mov      brp = rHDfhBrp
        mov      ar.pfs = rHDfhPFS
        ;;

        srlz.d
        ;;
        mov      cr.ifs = rHIFS
        dep      rHIPSR = 0, rHIPSR, PSR_DFH, 1  // reset dfh bit
        ;;

        mov      cr.ipsr = rHIPSR
        mov      cr.iip = rHIIP
        mov      pr = rHOldPreds, -1
        ;;

        rfi
        ;;

Kdfrv10:
        mov      pr = rHOldPreds, -1
        movl     rHHandler = KiGeneralExceptions

        br.sptk     KiGenericExceptionHandler
        ;;

        VECTOR_EXIT(KiDisabledFpRegisterVector)

//--------------------------------------------------------------------
// Routine:
//
//       KiNatConsumptionVector 
//
// Description:
//
//       Interruption vector for Nat Consumption Vector
//
// On entry:
//       interrupts disabled
//       r16-r31 switched to kernel bank
//
// Return value:
//
//       none
//
// Process:
//--------------------------------------------------------------------

        VECTOR_ENTRY(0x5600, KiNatConsumptionVector, cr.isr)

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiNatExceptions)

//
// Do not return (rfi from handler)
//

        VECTOR_EXIT(KiNatConsumptionVector)

//++
//
// KiSpeculationVector
//
// Cause:       CHK.S, CHK.A, FCHK detected an exception condition. 
//
// Parameters:  cr.iip  - address of bundle which caused a speculation fault
//
//              cr.ipsr - copy of PSR at the time of a speculation fault
//
//              cr.iipa - address of bundle containing the last
//                      successfully executed instruction
//
//              cr.iim  - contains the immediate value in either 
//                         CHK.S, CHK.A, or FCHK opecode
//
//              cr.isr  - faulting status information
//              
// Handle:      The handler implements a branch operation to the
//              recovery code specified by the IIM IP-offset.
//
// Note:        This code will not be exercised until the compiler
//              generates the speculation code.
//
// TBD:         Need to check for taken branch trap.
//
//--
        
        VECTOR_ENTRY(0x5700, KiSpeculationVector, cr.iim)
        
        mov             h16 = cr.iim            // get imm offset
        mov             h17 = cr.iip            // get IIP
        ;;
        extr            h16 = h16, 0, 21        // get sign-extended
        mov             h18 = cr.ipsr
        ;;
        shladd          h16 = h16, 4, h17       // get addr for recovery handler
        dep             h18 = 0, h18, PSR_RI, 2 // zero target slot number
        ;;
        mov             cr.ipsr = h18
        mov             cr.iip = h16
        ;;
        rfi
        ;;

        VECTOR_EXIT(KiSpeculationVector)

//++
//
// KiUnalignedFaultVector
//
// Cause:       A unaligned data access fault has occured
//
// Parameters:  cr.iip  - address of bundle causing the fault.
//
//              cr.ipsr - copy of PSR at the time of interruption.
//
//              cr.iipa - address of bundle containing the last
//                        successfully executed instruction
//
//              cr.isr  - faulting status information. ISR.ei bits are
//                        set to indicate which instruction caused the
//                        exception. 
//                        The ISR.code contains information about the
//                        FP exception fault. See trapc.c and the EAS
//              
//--
        
        VECTOR_ENTRY(0x5a00, KiUnalignedFaultVector, cr.ifa)
        
        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiUnalignedFault)

//
// Do not return (rfi from handler)
//

        VECTOR_EXIT(KiUnalignedFaultVector)


//++
//
// KiFloatFaultVector
//
// Cause:       A floating point fault has occured
//
// Parameters:  cr.iip  - address of bundle causing the fault.
//
//              cr.ipsr - copy of PSR at the time of interruption.
//
//              cr.iipa - address of bundle containing the last
//                        successfully executed instruction
//
//              cr.isr  - faulting status information. ISR.ei bits are
//                        set to indicate which instruction caused the
//                        exception. 
//                        The ISR.code contains information about the
//                        FP exception fault. See trapc.c and the EAS
//              
//--
        
        VECTOR_ENTRY(0x5c00, KiFloatFaultVector, cr.isr)
         
        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiFloatFault)

//
// Do not return (rfi from handler)
//

        VECTOR_EXIT(KiFloatFaultVector)

//++
//
// KiFloatTrapVector
//
// Cause:       A floating point trap has occured
//
// Parameters:  cr.iip  - address of bundle with the instruction to be
//                        executed next.
//
//              cr.ipsr - copy of PSR at the time of interruption.
//
//              cr.iipa - address of bundle containing the last
//                        successfully executed instruction
//
//              cr.isr  - faulting status information. ISR.ei bits are
//                        set to indicate which instruction caused the
//                        exception. 
//                        The ISR.code contains information about the
//                        FP trap. See trapc.c and the EAS
//              
//--
        
        VECTOR_ENTRY(0x5d00, KiFloatTrapVector, cr.isr)
        
        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiFloatTrap)

//
// Do not return (rfi from handler)
//

        VECTOR_EXIT(KiFloatTrapVector)

//++
//
// KiLowerPrivilegeVector
//
// Cause:       A branch lowers the privilege level and PSR.lp is 1.
//              Or an attempt made to execute an instruction
//              in the unimplemented address space.  
//              This trap is higher priority than taken branch 
//              or single step traps.
//
// Parameters:  cr.iip  - address of bundle containing the instruction to
//                        be executed next.
//
//              cr.ipsr - copy of PSR at the time of interruption.
//
//              cr.iipa - address of bundle containing the last
//                        successfully executed instruction
//
//              cr.isr  - faulting status information. The ISR.code
//                        contains a bit vector for all traps which
//                        occurred in the trapping bundle. 
//              
//--
        
        VECTOR_ENTRY(0x5e00, KiLowerPrivilegeVector, cr.iipa)
        
        mov       rHIIPA = cr.iipa
        movl      rHEPCVa = MM_EPC_VA

        mov       rHIPSR = cr.ipsr
        mov       rTH2 = @secrel(NscLastBundle)
        mov       rHOldPreds = pr
        ;;

        ssm       1 << PSR_IC
        movl      rHpT1 = KiPcr+PcInitialStack

        add       rHEPCVa = rHEPCVa, rTH2
        movl      rHpT3 = 1 << PSR_DB | 1 << PSR_TB | 1 << PSR_SS
        ;;

        cmp.ne    pt0 = rHEPCVa, rHIIPA
        mov       rPanicCode = UNEXPECTED_KERNEL_MODE_TRAP
(pt0)   br.spnt.few KiPanicHandler
        
        ld8       rHpT1 = [rHpT1]
        ;;
        srlz.d
        add       rHpT1=-ThreadStateSaveAreaLength-TrapFrameLength+TrStIPSR,rHpT1
        ;;

        ld8       rHpT4 = [rHpT1]
        ;;
        rsm       1 << PSR_IC
        dep       rHIPSR = 0, rHIPSR, PSR_LP, 1     // reset psr.lp bit
        ;;

        st8       [rHpT1] = rHIPSR           // clear psr.db, tb, ss, lp
        and       rHpT4 = rHpT3, rHpT4       // capture psr.db, psr.tb, psr.ss
        ;;
        or        rHIPSR = rHIPSR, rHpT4     // propagate psr bit settings

        srlz.d
        ;;
        mov       cr.ipsr = rHIPSR
        mov       pr = rHOldPreds, -2
        ;;

        rfi
        ;;

        VECTOR_EXIT(KiLowerPrivilegeVector)

//++
//
// KiTakenBranchVector
//
// Cause:       A taken branch was successfully execcuted and the PSR.tb
//              bit is 1. This trap is higher priority than single step trap.
//
// Parameters:  cr.iip  - address of bundle containing the instruction to
//                        be executed next.
//
//              cr.ipsr - copy of PSR at the time of interruption.
//
//              cr.iipa - address of bundle containing the last
//                        successfully executed instruction
//
//              cr.isr  - faulting status information. The ISR.code
//                        contains a bit vector for all traps which
//                        occurred in the trapping bundle. 
//              
//--
        
        VECTOR_ENTRY(0x5f00, KiTakenBranchVector, cr.iipa)
        
        mov       rHIIP = cr.iip
        movl      rHEPCVa = MM_EPC_VA+0x20     // user system call entry point

        mov       rHIPSR = cr.ipsr
        movl      rHpT1 = KiPcr+PcInitialStack
        ;;

        ld8       rHpT1 = [rHpT1]
        mov       rHOldPreds = pr
        mov       rPanicCode = UNEXPECTED_KERNEL_MODE_TRAP
        ;;

        cmp.eq    pt0 = rHEPCVa, rHIIP
        extr.u    rTH1 = rHIPSR, PSR_CPL, PSR_CPL_LEN
        ;;

        cmp4.eq   pKrnl, pUser = PL_KERNEL, rTH1
(pKrnl) br.spnt.few KiPanicHandler
        ;;

 (pt0)  ssm       1 << PSR_IC
 (pt0)  movl      rTH1 = 1 << PSR_LP
        ;;

 (pt0)  or        rHpT3 = rHIPSR, rTH1
        movl      rHHandler = KiSingleStep

 (pt0)  srlz.d
        add       rHpT1=-ThreadStateSaveAreaLength-TrapFrameLength+TrStIPSR,rHpT1
 (pt0)  br.spnt.few Ktbv10

        mov       pr = rHOldPreds, -2
        br.sptk   KiGenericExceptionHandler
        ;;


Ktbv10:

        st8       [rHpT1] = rHpT3
        movl      rTH1 = 1 << PSR_SS | 1 << PSR_TB | 1 << PSR_DB
        ;;

        rsm       1 << PSR_IC
        mov       pr = rHOldPreds, -2
        andcm     rHIPSR = rHIPSR, rTH1   // clear ss, tb, db bits
        ;;

        srlz.d
        ;;
        mov       cr.ipsr = rHIPSR
        ;;
        rfi
        ;;

        VECTOR_EXIT(KiTakenBranchVector)

//++
//
// KiSingleStepVector
//
// Cause:       An instruction was successfully execcuted and the PSR.ss
//              bit is 1. 
//
// Parameters:  cr.iip  - address of bundle containing the instruction to
//                        be executed next.
//
//              cr.ipsr - copy of PSR at the time of interruption.
//
//              cr.iipa - address of bundle containing the last
//                        successfully executed instruction
//
//              cr.isr  - faulting status information. The ISR.code
//                        contains a bit vector for all traps which
//                        occurred in the trapping bundle. 
//              
//--
        
        VECTOR_ENTRY(0x6000, KiSingleStepVector, cr.iipa)
        
        mov       rHIIP = cr.iip
        movl      rHEPCVa = MM_EPC_VA+0x20     // user system call entry point

        mov       rHIPSR = cr.ipsr
        movl      rHpT1 = KiPcr+PcInitialStack
        ;;

        ld8       rHpT1 = [rHpT1]
        mov       rHOldPreds = pr
        mov       rPanicCode = UNEXPECTED_KERNEL_MODE_TRAP
        ;;

        cmp.eq    pt0 = rHEPCVa, rHIIP
        extr.u    rTH1 = rHIPSR, PSR_CPL, PSR_CPL_LEN
        ;;

        cmp4.eq   pKrnl, pUser = PL_KERNEL, rTH1
(pKrnl) br.spnt.few KiPanicHandler
        ;;

 (pt0)  ssm       1 << PSR_IC
 (pt0)  movl      rTH1 = 1 << PSR_LP
        ;;

 (pt0)  or        rHpT3 = rHIPSR, rTH1
        movl      rHHandler = KiSingleStep

 (pt0)  srlz.d
        add       rHpT1=-ThreadStateSaveAreaLength-TrapFrameLength+TrStIPSR,rHpT1
 (pt0)  br.spnt.few Kssv10

        mov       pr = rHOldPreds, -2
        br.sptk   KiGenericExceptionHandler
        ;;


Kssv10:

        st8       [rHpT1] = rHpT3
        movl      rTH1 = 1 << PSR_SS | 1 << PSR_DB
        ;;

        rsm       1 << PSR_IC
        mov       pr = rHOldPreds, -2
        andcm     rHIPSR = rHIPSR, rTH1   // clear ss, db bits
        ;;

        srlz.d
        ;;
        mov       cr.ipsr = rHIPSR
        ;;
        rfi
        ;;

        VECTOR_EXIT(KiSingleStepVector)

//++
//
// KiIA32ExceptionVector
//
// Cause:       A fault or trap was generated while executing from the
//              iA-32 instruction set.
//
// Parameters:  cr.iip  - address of the iA-32 instruction causing interruption
//
//              cr.ipsr - copy of PSR at the time of the instruction
//
//              cr.iipa - Address of the last successfully executed
//                        iA-32 or EM instruction
//
//              cr.isr  - The ISR.ei exception indicator is cleared.
//                        ISR.iA_vector contains the iA-32 interruption vector
//                        number.  ISR.code contains the iA-32 16-bit error cod
//
// Handle:      Save the whole register state and
//                        call KiIA32ExceptionVectorHandler()().
//
//--

        VECTOR_ENTRY(0x6900, KiIA32ExceptionVector, r0)

        mov       rHIIM = cr.iim               // save info from IIM
        movl      rTH1 = KiPcr+PcSavedIIM
        ;;
        st8       [rTH1] = rHIIM

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler,
                               KiIA32ExceptionVectorHandler)

        VECTOR_EXIT(KiIA32ExceptionVector)

//++
//
// KiIA32InterceptionVector
//
// Cause:       A interception fault or trap was generated while executing
//              from the iA-32 instruction set.
//
// Parameters:  cr.iip  - address of the iA-32 instruction causing interruption
//
//              cr.ipsr - copy of PSR at the time of the instruction
//
//              cr.iipa - Address of the last successfully executed
//                        iA-32 or EM instruction
//
//              cr.isr  - The ISR.ei exception indicator is cleared.
//                        ISR.iA_vector contains the iA-32 interruption vector
//                        number.  ISR.code contains the iA-32specific
//                        interception information
//
// Handle:      Save the whole register state and
//                        call KiIA32InterceptionVectorHandler()().
//
//--

        VECTOR_ENTRY(0x6a00, KiIA32InterceptionVector, r0)


        mov       rHIIM = cr.iim               // save info from IIM
        movl      rTH1 = KiPcr+PcSavedIIM
        ;;
        st8       [rTH1] = rHIIM

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler,
                                KiIA32InterceptionVectorHandler)

         VECTOR_EXIT(KiIA32InterceptionVector)

//++
//
// KiIA32InterruptionVector
//
// Cause:       An iA software interrupt was executed
//
// Parameters:  cr.iip  - address of the iA-32 instruction causing interruption
//
//              cr.ipsr - copy of PSR at the time of the instruction
//
//              cr.iipa - Address of the last successfully executed
//                        iA-32 or EM instruction
//
//              cr.isr  - ISR.iA_vector contains the iA-32 defined vector
//                        number.  ISR.code contains 0
//                        ISR.ei excepting instruction indicator is cleared.
//                        ISR.iA_vector contains the iA-32 instruction vector.
//                        ISR.code contains iA-32 specific information.
//
// Handle:      Save the whole register state and
//                        call KiIA32InterruptionVectorHandler()().
//
//--

        VECTOR_ENTRY(0x6b00, KiIA32InterruptionVector, r0)

        // This one doesn't need IIM, so we won't bother to save it

        VECTOR_CALL_HANDLER(KiGenericExceptionHandler,
                                KiIA32InterruptionVectorHandler)


        VECTOR_EXIT(KiIA32InterruptionVector)

//
// All non-VECTOR_ENTRY functions must follow KiNormalSystemCall.
//
// N.B. KiNormalSystemCall must be the first function body in the .nsc
//      section.
//


//--------------------------------------------------------------------
// Routine:
//
//       KiNormalSystemCall
//
// Description:
//
//       Handler for normal (not fast) system calls
//
// On entry:
//
//       ic off
//       interrupts disabled
//       v0: contains sys call #
//       cover done by call
//       r32-r39: sys call arguments
//       CFM: sof = # args, ins = 0, outs = # args
//
// Return value:
//
//       v0: system call return value
//
// Process:
//
//--------------------------------------------------------------------

        .section .drectve, "MI", "progbits"
        string " -section:.nsc,,align=0x2000"

        .section .nsc = "ax", "progbits"

        HANDLER_ENTRY_EX(KiNormalSystemCall, KiSystemServiceHandler)

        .prologue
        .unwabi     @nt,  SYSCALL_FRAME

        rThread     = t1                  // current thread
        rIFS        = t1
        rIIP        = t2
        rPreds      = t3
        rIPSR       = t4
        rUNAT       = t5

        rSp         = t6

        rpT1        = t7
        rpT2        = t8
        rpT3        = t9
        rpT4        = t10
        rT0         = t11
        rT1         = t12
        rT2         = t13
        rT3         = t14
        rT4         = t15

        rIntNats    = t17

        rpSd        = t16                  /* -> service descriptor entry   */
        rSdOffset   = t17                  /* service descriptor offset     */
        rArgTable   = t18                  /* pointer to argument table     */
        rArgNum     = t20                  /* number of arguments     */
        rArgBytes   = t21

        rpBSPStore  = t16
        rRscD       = t16
        rRNAT       = t17
        rRscE       = t18
        rKBSPStore  = t18
        rBSPStore   = t19
        rpBSP       = t20
        rRscDelta   = t20

        rBSP        = t21
        rPreviousMode = t22

        pInvl       = pt0                  /* pInvl = not GUI service       */
        pVal        = pt1
        pGui        = pt2                  /* true if GUI call              */
        pNoGui      = pt3                  /* true if no GUI call           */
        pNatedArg   = pt4                  /* true if any input argument    */
                                           /* register is Nat'ed            */
        pNoCopy     = pt5                  /* no in-memory arguments to copy */
        pCopy       = pt6


        mov       rUNAT = ar.unat
        tnat.nz   pt0 = sp
        mov       rPreviousMode = KernelMode

        mov       rIPSR = psr
        rsm       1 << PSR_I | 1 << PSR_MFH
        br.sptk   Knsc_Allocate
        ;;

//
// N.B. KiUserSystemCall is at an offset of 0x20 from KiNormalSystemCall.
// Whenever this offset is changed, the definition of kernel system call
// stub in services.stb must be updated to reflect the new value.
//

        ALTERNATE_ENTRY(KiUserSystemCall)

        mov       rUNAT = ar.unat
        mov       rPreviousMode = UserMode
        epc
        ;;

        mov       rIPSR = psr
        rsm       1 << PSR_I
        tnat.nz   pt0 = sp
        ;;                        // stop bit needed to ensure interrupt is off

Knsc_Allocate::

//
// if sp is Nat'ed return to caller with an error status
// N.B. sp is not spilled and a value of zero is saved in the IntNats field
//

        mov       rT1 = cr.dcr
 (pt0)  movl      v0 = STATUS_IA64_INVALID_STACK

        mov       rPreds = pr
        cmp.eq    pUser, pKrnl = UserMode, rPreviousMode
 (pt0)  br.spnt   NscLastBundle

        mov       rT2 = ar.rsc
        movl      rpT4 = KiPcr+PcInitialStack

        mov       rT0 = ar.fpsr
        mov       rIFS = ar.pfs
        mov       rIIP = brp
        ;;

(pUser) rum       1 << PSR_BE | 1 << PSR_MFH     // ensure little endian
        mov       rSp = sp
        ;;

(pUser) ld8       sp = [rpT4], PcInitialBStore-PcInitialStack   // set new sp
(pKrnl) add       rpT4 = PcCurrentThread-PcInitialStack, rpT4
(pKrnl) add       sp = -TrapFrameLength, sp      // allocate TF
        ;;

(pUser) ld8       rKBSPStore = [rpT4], PcCurrentThread-PcInitialBStore
(pUser) add       sp = -ThreadStateSaveAreaLength-TrapFrameLength, sp
        ;;

        add       rpT1 = TrStIPSR, sp            // -> IPSR
        add       rpT2 = TrIntSp, sp             // -> IntSp
        ;;

(pUser) ld8       rT4 = [rpT1]
(pUser) movl      rT3 = 1 << PSR_SS | 1 << PSR_DB | 1 << PSR_TB | 1 << PSR_LP

        mov       rBSP = ar.bsp
        st8       [rpT2] = rSp, TrApUNAT-TrIntSp // sp is not Nat'ed
        add       rpT3 = TrStIFS, sp             // -> IFS
        ;;

        st8       [rpT2] = rUNAT, TrApDCR-TrApUNAT
        st8       [rpT3] = rIFS, TrRsPFS-TrStIFS // save IFS
        ;;

        st8       [rpT2] = rT1, TrPreds-TrApDCR
        st8.nta   [rpT3] = rIFS, TrRsRSC-TrRsPFS // save PFS
(pUser) and       rT4 = rT3, rT4                 // capture psr.db, tb, ss
        ;;

        st8       [rpT2] = rPreds, TrIntNats-TrPreds
        st8       [rpT3] = rT2, TrStFPSR-TrRsRSC // save RSC
(pUser) mov       rT3 = rIPSR
        ;;

(pUser) mov       ar.rsc = r0                    // put RSE in lazy mode
        st8       [rpT3] = rT0, TrStIIP-TrStFPSR // save FPSR
(pUser) or        rIPSR = rIPSR, rT4
        ;;

        st8       [rpT1] = rIPSR                 // save IPSR
        st8       [rpT2] = zero                  // all integer Nats are 0
(pUser) dep       rT3 = 0, rT3, PSR_DB, 1        // clear psr.db
        ;;

        st8       [rpT3] = rIIP, TrBrRp-TrStIIP  // save IIP
        add       rpBSP = TrRsBSP, sp            // -> BSP
(pUser) dep       rT3 = 0, rT3, PSR_I, 1         // clear interrupt enable bit
        ;;

        st8.nta   [rpT3] = rIIP                  // save BRP
        st8       [rpBSP] = rBSP
(pUser) dep       rT3 = 1, rT3, PSR_AC, 1        // enable alignment check
        ;;

(pUser) mov       psr.l = rT3
(pKrnl) br.dpnt   Knsc10                         // br if on kernel stack
        ;;

        mov       rBSPStore = ar.bspstore        // get user bsp store point
        mov       rRNAT = ar.rnat                // get RNAT
        add       rpBSPStore = TrRsBSPSTORE, sp  // -> User BSPStore

        add       rpT2 = TrRsRNAT, sp            // -> RNAT
        movl      gp = _gp                       // set up kernel gp

        mov       teb = kteb                     // get Teb pointer
        movl      rT1 = FPSR_FOR_KERNEL          // initial fpsr value
        ;;

        mov       ar.fpsr = rT1                  // set fpsr
        movl      rT2 = KiSetupDebugRegisters

        st8       [rpBSPStore] = rBSPStore       // save user BSP Store
        st8       [rpT2] = rRNAT                 // save RNAT in trap frame
                                                 // for user backing store
        dep       rKBSPStore = rBSPStore, rKBSPStore, 0, 9
        ;;

        mov       ar.bspstore = rKBSPStore       // switch to kernel BSP
        mov       bt0 = rT2
        ;;

        mov       ar.rsc = RSC_KERNEL            // turn rse on, in kernel mode
(pUser) br.call.spnt brp = bt0
        ;;

Knsc10:

//
// Now running with user banked registers and on kernel backing store
//
// Can now take TLB faults
//
// Preserve the output args (as in's) in the local frame until it's ready to
// call the specified system service because other calls may have to be made
// before that. Also allocate locals and one out.
//


//
// Register aliases for rest of procedure
//

        .regstk    8, 8, 0, 0

        rScallGp    = loc0
        rpThObj     = loc1                      // pointer to thread object
        rSavedV0    = loc2                      // saved v0 for GUI thread
        rpEntry     = loc3                      // syscall routine entry point
        rSnumber    = loc4                      // service number
        rArgEntry   = loc5
        rCount      = loc6      /* index of the first Nat'ed input register  */
        rUserSp     = loc7

        //
        // the following code uses the predicates to determine the first of
        // the 8 input argument register whose Nat bit is set.  The result
        // is saved in register rCount and used to determine whether to fail
        // the system call in Knsc_CheckArguments.
        //

        alloc     t0 = 8, 8, 0, 0
        FAST_ENABLE_INTERRUPTS
        mov       pr = zero, -1
        ;;

        LDPTR     (rpThObj, rpT4)               // rpT4 -> PcCurrentThread
        add       rpT2 = TrEOFMarker, sp
        mov       rUserSp = rSp
        ;;

        cmp.eq    p1 = r32, r32
        cmp.eq    p9 = r33, r33
        cmp.eq    p17 = r34, r34
        cmp.eq    p25 = r35, r35
        cmp.eq    p33 = r36, r36
        cmp.eq    p41 = r37, r37
        cmp.eq    p49 = r38, r38
        cmp.eq    p57 = r39, r39
        ;;

        mov       rT1 = pr
        movl      rT3 = KTRAP_FRAME_EOF | SYSCALL_FRAME
        ;;

        st8       [rpT2] = rT3
        mov       rSavedV0 = v0                 // save syscall # across call
        dep       rT1 = 0, rT1, 0, 1            // clear bit 0
        ;;

//
// Save and update thread previous mode and trap frame
//

        add       rpT3 = ThTrapFrame, rpThObj   // rpT3 -> thread previous mode
        add       rpT1 = TrTrapFrame, sp        // rpT2 -> TrTrapFrame
        czx1.r    rCount = rT1                  // determine which arg is Nat'ed
        ;;

        PROLOGUE_END


        LDPTRINC  (rT4, rpT3, ThPreviousMode-ThTrapFrame) // rT4 = ThTrapFrame
        ;;
        ld1       rT0 = [rpT3]                  // rT0 = thread's previous mode

        STPTRINC  (rpT1, rT4, TrPreviousMode-TrTrapFrame)
        st1       [rpT3] = rPreviousMode        // set new thread previous mode
        cmp.eq    pUser,pKrnl=UserMode,rPreviousMode  // restore pUser & pKrnl
        ;;

        // *** TBD 1 byte save thread previous mode in trap frame
        st4       [rpT1] = rT0

//
// If the specified system service number is not within range, then
// attempt to convert the thread to a GUI thread and retry the service
// dispatch.
//
// N.B. The system call arguments, the system service entry point (rpEntry), 
//      the service number (Snumber) 
//      are implicitly preserved in the register stack while attempting to 
//      convert the thread to a GUI thread. v0 and the gp must be preserved
//      explicitly.
//
// Validate sys call number
//

        ALTERNATE_ENTRY(KiSystemServiceRepeat)

        add       rpT1 = ThTrapFrame, rpThObj   // rpT1 -> ThTrapFrame
        add       rpT2 = ThServiceTable,rpThObj // rpT2 -> ThServiceTable
        shr.u     rT2 = rSavedV0, SERVICE_TABLE_SHIFT // isolate service descriptor offset
        ;;

        STPTR     (rpT1, sp)                    // set trap frame address
        LDPTR     (rT3, rpT2)                   // -> service descriptor table
        and       rSdOffset = SERVICE_TABLE_MASK, rT2
        ;;

        add       rT2 = rSdOffset, rT3          // rT2 -> service descriptor
        mov       rSnumber = SERVICE_NUMBER_MASK
        ;;
        add       rpSd = SdLimit, rT2           // rpSd -> table limit
        ;;

        ld4       rT1 = [rpSd], SdTableBaseGpOffset-SdLimit // rT1 = table limit
        ;;
        ld4       rScallGp = [rpSd], SdBase-SdTableBaseGpOffset
        and       rSnumber = rSnumber, rSavedV0 // rSnumber = service number
        ;;

        LDPTRINC  (rT4, rpSd, SdNumber-SdBase)  // rT4 = table base      
        sxt4      rScallGp = rScallGp           // sign-extend offset
        ;;
        add       rScallGp = rT4, rScallGp      // compute syscall gp

        LDPTR     (rArgTable, rpSd)             // -> arg table
        cmp4.leu  pt0, pt1 = rT1, rSnumber      // pt0 = limit <= number
        shladd    rpT3 = rSnumber, 3, rT4       // -> entry point address
        ;;

        PLDPTR    (pt1, rpEntry, rpT3)          // -> sys call routine plabel 
        add       rArgEntry = rSnumber, rArgTable    // -> # arg bytes
(pt1)   br.sptk   Knsc_NotGUI                   // br if service # out of bounds
        ;;

//
// If rSdOffset == SERVICE_TABLE_TEST then service number is GUI service
// 

        cmp.ne    pInvl, pVal = SERVICE_TABLE_TEST, rSdOffset 
        movl      v0 = PsConvertToGuiThread
        ;;

(pInvl) mov       v0 = 1
(pVal)  mov       bt0 = v0
(pVal)  br.call.sptk brp = bt0
        ;;

        cmp4.eq   pVal, pInvl = 0, v0           // pVal = 1, if successful
        movl      v0 = STATUS_INVALID_SYSTEM_SERVICE      // invalid, if not successful
        ;;

(pVal)  br.sptk   KiSystemServiceRepeat         // br if successful
(pInvl) br.sptk   KiSystemServiceExit           // br to KiSystemServiceExit


Knsc_NotGUI:


#if DBG // checked build code

        add       rpT1 = SdCount-SdNumber, rpSd // rpT1 -> count table address
        ;;
        LDPTR     (rpT2, rpT1)                  // rpT2 = service count table address
        ;;

        cmp.ne    pt0 = rpT2, zero              // if zero, no table defined
        shladd    rpT3 = rSnumber, 2, rpT2      // compute service count address
        ;;

(pt0)   ld4       rT1 = [rpT3]                  // increment count
        ;;
(pt0)   add       rT1 = 1, rT1
        ;;
(pt0)   st4       [rpT3] = rT1                  // store result

#endif // DBG

//
// If the system service is a GUI service and the GDI user batch queue is
// not empty, then call the appropriate service to flush the user batch.
//

        cmp4.ne   pNoGui = SERVICE_TABLE_TEST, rSdOffset // check if GUI system service
        add       rpT1 = TeGdiBatchCount, teb   // get number of batched calls
(pNoGui)br.dpnt   Knsc_CheckArguments              
        ;;

        ld4       rT1 = [rpT1]
        add       rpT3 = @gprel(KeGdiFlushUserBatch), gp  // -> KeGdiFlushUserBatch
        ;;
        cmp4.ne   pGui = rT1, zero              // skip if no calls
        ;;

        PLDPTR    (pGui, rpT1, rpT3)            // get KeGdiFlushUserBatch routine's plabel address
        ;;

(pGui)  ld8       rT1 = [rpT1], PlGlobalPointer-PlEntryPoint // get entry point
        ;;
(pGui)  ld8       gp = [rpT1]                   // set global pointer 
(pGui)  mov       bt0 = rT1     
(pGui)  br.call.sptk brp = bt0                  // call to KeGdiFlushUserBatch
        ;;

//
// Check for Nat'ed input argument register and
// Copy in-memory arguments from caller stack to kernel stack
//

Knsc_CheckArguments:

        tbit.z    pt0, pt1 = rpEntry, 0
        extr.u    rArgNum = rpEntry, 1, 3       // extract # of arguments
        dep       rpEntry = 0, rpEntry, 0, 4    // clear least significant 4 bit
        ;;

 (pt1)  ld1.nt1   rArgBytes = [rArgEntry]       // get # arg bytes
        movl      v0 = STATUS_INVALID_PARAMETER_1

        mov       gp = rScallGp
        mov       bt0 = rpEntry
        mov       rSp = rUserSp
        ;;

        add       v0 = rCount, v0               // set return status
        cmp.ne    pNatedArg = zero, zero        // assume no Nat'ed argument
 (pt1)  shr       rArgNum = rArgBytes, 2
        ;;

 (pt1)  add       rArgNum = 7, rArgNum          // number of in arguments
        ;;
        cmp.geu   pNoCopy, pCopy = 8, rArgNum   // any in-memory arguments ?
 (pt1)  shl       rArgBytes = rArgNum, 3        // x2 since args are 8 bytes each, not 4
        ;;

(pNoCopy) cmp.gt  pNatedArg = rArgNum, rCount   // any Nat'ed argument ?
  (pCopy) cmp.gt  pNatedArg = 8, rCount
  (pCopy) add     rArgBytes = -64, rArgBytes    // size of in-memory arguments
        ;;

        alloc     rT1 = 0,0,8,0                 // output regs are ready
(pNatedArg) br.spnt KiSystemServiceExit         // exit if Nat'ed arg found
(pNoCopy) br.dpnt Knsc_NoCopy                   // skip copy if no memory args
        ;;

//
// Get the caller's sp. If caller was user mode, validate the stack pointer
// ar.unat contains Nat for previous sp (in TrIntSp)
//

(pUser) tnat.nz.unc pt1, pt2 = rSp              // test user sp for Nat
(pUser) movl      rpT2 = MM_USER_PROBE_ADDRESS  // User sp limit
        ;;

(pt2)   cmp.geu   pt1 = rSp, rpT2               // user sp >= PROBE ADDRESS ?
        mov       rpT1 = rSp                    // previous sp
        ;;

(pt1)   add       rpT1 = -STACK_SCRATCH_AREA,rpT2 // set out of range (includes Nat case)
        ;;
        add       rpT1 = STACK_SCRATCH_AREA, rpT1 // adjust for scratch area
        add       rpT2 = STACK_SCRATCH_AREA, sp // adjust for scratch area
        ;;
        
        add       rpT3 = 8, rpT1                // second source pointer
        add       rpT4 = 8, rpT2                // second destination pointer
        ;;

//
// At this point rpT1, rpT3 -> source and rpT2, rpT4 -> destination
// Copy rArgBytes from source to destination, 16 bytes per iteration
// Exceptions handled by KiSystemServiceHandler
//

        ALTERNATE_ENTRY(KiSystemServiceStartAddress)

Knsc_CopyLoop:

        ld8       rT1 = [rpT1], 16              // get caller arg
        ld8       rT2 = [rpT3], 16              // get caller arg
        add       rArgBytes = -16, rArgBytes    // decrement count
        ;;
        st8       [rpT2] = rT1, 16              // store in kernel stack
        st8       [rpT4] = rT2, 16              // store in kernel stack
        cmp4.gt   pt1 = rArgBytes, zero         // loop if # bytes > 0
(pt1)   br.dpnt   Knsc_CopyLoop
        ;;

        ALTERNATE_ENTRY(KiSystemServiceEndAddress)

Knsc_NoCopy:

        //
        // N.B. t0 is reserved to pass trap frame address to NtContinue()
        //

        mov       t0 = sp                       // for NtContinue()
        br.call.sptk   brp = bt0                // call routine(args)
        ;;

        ALTERNATE_ENTRY(KiSystemServiceExit)

//
// At this point:
//      ar.unat contains Nat for previous sp (ar.unat is preserved register)
//      sp -> trap frame
//
// Returning from "call": no need to restore volatile state
// *** TBD *** : zero volatile state for security? PPC does zero, mips does not.
//


        FAST_DISABLE_INTERRUPTS
        movl      rpT1 = KiPcr + PcPrcb         // rpT1 -> Prcb
        ;;

//
// Update PbSystemCalls
//

        LDPTRINC  (rpT3, rpT1, PcCurrentThread-PcPrcb)
        ;;
        LDPTR     (rThread, rpT1)               // rpT1 -> current thread
        add       rpT3 = PbSystemCalls, rpT3    // pointer to sys call counter
        ;;

        ld4       rT1 = [rpT3]                  // rT1.4 = counter value
        add       rpT2 = TrTrapFrame, sp        // -> saved trap frame address
        add       rpT4 = ThTrapFrame, rThread   // -> thread trap frame
        ;;

        add       rT1 = 1, rT1                  // increment
        ;;
        st4       [rpT3] = rT1                  // store

//
// Restore thread previous mode and trap frame address from those saved in trap frame
//

        LDPTRINC  (rT1, rpT2, TrPreviousMode-TrTrapFrame) // rT1 = saved trap frame address from trap frame
        ;;
        ld4       rT3 = [rpT2], TrRsRSC-TrPreviousMode

        STPTRINC  (rpT4, rT1,ThPreviousMode-ThTrapFrame)  // restore TrapFrame in thread object
        ;;
        st1       [rpT4] = rT3                   // restore prevmode in thread
        add       rpT1 = ThApcState+AsUserApcPending, rThread
        ;;

(pKrnl) ld8       rRscE = [rpT2]                 // load user RSC
(pUser) ld1       rT2 = [rpT1], ThAlerted-ThApcState-AsUserApcPending

        mov       t0 = sp                        // set t0 to trap frame
(pKrnl) br.sptk Knsc_CommonExit                  // br if returning to kernel
        ;;

        st1       [rpT1] = zero
        cmp4.eq   pt0 = zero, rT2
(pt0)   br.sptk   KiUserServiceExit
        ;;

        alloc     rT1 = ar.pfs, 0, 1, 2, 0
        add       loc0 = TrIntV0, sp
        add       rpT1 = TrIntTeb, sp

//
// v0 is saved in the trap frame so the return status can be restored
// by NtContinue after the user APC has been dispatched.
//

        ssm       1 << PSR_I             // enable interrupts
        SET_IRQL  (APC_LEVEL)
        movl      rT3 = KiApcInterrupt
        ;;

        st8.nta   [loc0] = v0            // save return status in trap frame
        st8.nta   [rpT1] = teb
        mov       bt0 = rT3

        mov       out1 = sp
        br.call.sptk brp = bt0
        ;;

        rsm       1 << PSR_I             // disable interrupt
        ld8.nta   v0 = [loc0]            // restore system call return status
        SET_IRQL (zero)

        HANDLER_EXIT(KiNormalSystemCall)

//
// KiServiceExit is carefully constructed as a continuation of 
// KiNormalSystemCall.  From now on, t0 must be preserved because it is 
// used to hold the trap frame address. v0 must be preserved because it
// is holding the return status.
//

        HANDLER_ENTRY_EX(KiServiceExit, KiSystemServiceHandler)

        .prologue
        .unwabi   @nt,  SYSCALL_FRAME

        .vframe   t0
        mov       t0 = sp
        ;;

        ALTERNATE_ENTRY(KiUserServiceExit)

        invala
        movl      rpT3 = KiRestoreDebugRegisters
        ;;

        mov       bt0 = rpT3
        br.call.sptk brp = bt0
        ;;

        add       rpT1 = TrRsRSC, t0                   // -> user RSC
        add       rpT2 = TrRsBSPSTORE, t0              // -> user BSP Store
        ;;

        PROLOGUE_END

        ld8       rRscE = [rpT1], TrRsRNAT-TrRsRSC     // load user RSC
        ld8       rBSPStore = [rpT2], TrRsBSP-TrRsBSPSTORE // user BSP Store
        mov       rRscD = RSC_KERNEL_DISABLED
        ;;

//
// Switch to user BSP -- put in load intensive mode to overlap RS restore
// with volatile state restore.
//

        ld8       rRNAT = [rpT1]                       // user RNAT
        ld8       rRscDelta = [rpT2]                   // user BSP
        ;;
        sub       rRscDelta = rRscDelta, rBSPStore     // delta = BSP-BSPSTORE
        ;;

        alloc     rT2 = 0,0,0,0
        dep       rRscD = rRscDelta, rRscD, 16, 14     // set RSC.loadrs
        ;;

        mov       ar.rsc = rRscD                       // turn off RSE
        ;;
        loadrs                                         // pull in user regs
        ;;

        mov       ar.bspstore = rBSPStore              // restore user BSP
        ;;
        mov       ar.rnat = rRNAT                      // restore user RNAT


Knsc_CommonExit:

        add       rpT3 = TrIntNats, t0
        add       rpT4 = TrStIPSR, t0
        mov       rT0 = 1 << PSR_I
        ;;

        ld8       rIntNats = [rpT3]
        ld8       rIPSR = [rpT4]
        ;;

        mov       ar.unat = rIntNats
        mov       ar.rsc = rRscE              // restore RSC
(pUser) dep       rIPSR = 0, rIPSR, PSR_TB, 1 // ensure psr.tb is clear
        ;;

        add       rpT1 = TrIntSp, t0
        add       rpT3 = TrStIFS, t0
        andcm     rIPSR = rIPSR, rT0          // mask off interrupt enable bit
        ;;

        ld8.fill  rSp = [rpT1], TrPreds-TrIntSp // fill sp
        ld8       rIFS = [rpT3], TrStIIP-TrStIFS  // get previous PFS
(pKrnl) mov       rPreviousMode = KernelMode
        ;;

        ld8       rPreds = [rpT1], TrApUNAT-TrPreds
        ld8       rIIP = [rpT3], TrStFPSR-TrStIIP
(pUser) mov       rPreviousMode = UserMode
        ;;

        ld8       rUNAT = [rpT1], TrApDCR-TrApUNAT
        ld8       rT0 = [rpT3], TrStIPSR-TrStFPSR   // load fpsr
        mov       ar.pfs = rIFS               // restore PFS
        ;;

        mov       psr.l = rIPSR               // restore psr settings
        ld8       rT4 = [rpT1]                // load dcr
        mov       pr = rPreds                 // restore preds
        ;;

        mov       ar.unat = rUNAT             // restore UNAT
        mov       ar.fpsr = rT0               // restore FPSR
        cmp4.eq pt1, pt0 = KernelMode, rPreviousMode
        ;;

 (pt1)  ssm       1 << PSR_I                  // enable interrupt
        mov       cr.dcr = rT4                // restore DCR
        mov       brp = rIIP                  // restore brp
        ;;

        srlz.d                                // must serialize
        ;;
        mov       sp = rSp
 (pt1)  br.ret.spnt brp
        ;;

//
// Resume at point of call (ret will restore psr.cpl)
//

NscLastBundle:
        ssm       1 << PSR_I                  // psr.i = 1
        // ptc.e     r0
        br.ret.sptk brp
        ;;

        HANDLER_EXIT(KiServiceExit)

        .sdata
KiSystemServiceExitOffset::
        data4  @secrel(KiSystemServiceExit)
KiSystemServiceStartOffset::
        data4  @secrel(KiSystemServiceStartAddress)
KiSystemServiceEndOffset::
        data4  @secrel(KiSystemServiceEndAddress)


        .text
//++
//--------------------------------------------------------------------
// Routine:
//
//       KiGenericExceptionHandler
//
// Description:
//
//       First level handler for heavyweight exceptions.
//
// On entry:
//
//       ic off
//       interrupts disabled
//       current frame covered
//
// Process:
//
// Notes:
//
//       PCR page mapped with TR
//--------------------------------------------------------------------

        HANDLER_ENTRY(KiGenericExceptionHandler)

        .prologue
        .unwabi     @nt,  EXCEPTION_FRAME

        ALLOCATE_TRAP_FRAME

//
// sp points to trap frame
//
// Save exception handler routine in kernel register
//

        mov       rkHandler = rHHandler
        ;;

//
// Save interruption state in trap frame and switch to user bank registers
// and switch to kernel backing store.
//

        SAVE_INTERRUPTION_STATE(Kgeh_SaveTrapFrame)

//
// Now running with user banked registers and on kernel stack.
//
// Can now take TLB faults
//
// sp -> trap frame
//

        br.call.sptk brp = KiSaveTrapFrame
        ;;

//
// setup debug registers if previous mode is user
//

(pUser) br.call.spnt brp = KiSetupDebugRegisters

//
// Register aliases
//

        rpT1        = t0
        rpT2        = t1
        rpT3        = t2
        rT1         = t3
        rT2         = t4
        rT3         = t5
        rPreviousMode = t6                      // previous mode
        rT4         = t7


        mov       rT1 = rkHandler               // restore address of interruption routine
        movl      rpT1 = KiPcr+PcSavedIIM
        ;;

        ld8       rT2 = [rpT1], PcSavedIFA-PcSavedIIM  // load saved IIM
        add       rpT2 = TrEOFMarker, sp
        add       rpT3 = TrStIIM, sp
        ;;

        ld8       rT4 = [rpT1]                  // load saved IFA
        movl      rT3 = KTRAP_FRAME_EOF | EXCEPTION_FRAME
        ;;

        st8       [rpT2] = rT3, TrHandler-TrEOFMarker
        st8       [rpT3] = rT2, TrStIFA-TrStIIM // save IIM in trap frame
        mov       bt0 = rT1                     // set destination address
        ;;

        st8       [rpT3] = rT4                  // save IFA in trap frame
#if DBG
        st8       [rpT2] = rT1                  // save debug info in TrFrame
#endif // DBG
        ;;

        PROLOGUE_END

        .regstk     0, 1, 2, 0                  // must be in sync with KiExceptionExit
        alloc       out1 = 0,1,2,0              // alloc 0 in, 1 locals, 2 outs
        FAST_ENABLE_INTERRUPTS                  // enable interrupt

//
// Dispatch the exception via call to address in rkHandler
//
.pred.rel "mutex",pUser,pKrnl
        add       rpT1 = TrPreviousMode, sp     // -> previous mode
(pUser) mov       rPreviousMode = UserMode      // set previous mode
(pKrnl) mov       rPreviousMode = KernelMode
        ;;

        st4       [rpT1] = rPreviousMode        // **** TBD 1 byte -- save in trap frame
        mov       out0 = sp                     // trap frame pointer
        br.call.sptk brp = bt0                  // call handler(tf) (C code)
        ;;

.pred.rel "mutex",pUser,pKrnl
        cmp.ne    pt0, pt1 = v0, zero
(pUser) mov       out1 = UserMode
(pKrnl) mov       out1 = KernelMode

        //
        // does not return
        //

        mov       out0 = sp
(pt1)   br.cond.sptk KiAlternateExit
(pt0)   br.call.spnt brp = KiExceptionDispatch

        nop.m     0
        nop.m     0
        nop.i     0
        ;;

        HANDLER_EXIT(KiGenericExceptionHandler)


//--------------------------------------------------------------------
// Routine:
//
//       KiExternalInterruptHandler
//
// Description:
//
//       First level external interrupt handler. Dispatch highest priority 
//       pending interrupt.
//
// On entry:
//
//       ic off
//       interrupts disabled
//       current frame covered
//
// Process:
//--------------------------------------------------------------------

        HANDLER_ENTRY(KiExternalInterruptHandler)

//
// Now running with user banked registers and on kernel backing store.
// N.B. sp -> trap frame
//
// Can now take TLB faults
//

        .prologue
        .unwabi     @nt,  INTERRUPT_FRAME
#if 0
        alloc       loc1 = 0, 4, 2, 0
#endif
        alloc       loc1 = 0, 7, 2, 0           // for A0 2173 fix
        ;;
//
// Register aliases
//

        rVector     = loc0
        rSaveGP     = loc1
        rpSaveIrql  = loc2                      // -> old irql in trap frame
        rOldIrql    = loc3

// For A0 2173 fix

        rpWP        = loc2
        rpRP        = loc3
        rHalGp      = loc4
        rPxbTcap    = loc5
        rpIpiLock   = loc6

        rpT1        = t0
        rpT2        = t1
        rpT3        = t2
        rT1         = t3
        rT2         = t4
        rT3         = t5
        rPreviousMode = t6                      // previous mode
        rNewIrql    = t7

        pEOI        = pt1

//
// Save kernel gp
//

        mov         rSaveGP = gp
//
// Get the vector number
//
#ifndef A0_2173
        mov         rVector = cr.ivr // for A0 2173 workaround
#endif
        ;;
        br.call.sptk brp = KiSaveTrapFrame
        ;;

#ifdef A0_2173

//
// A0 2173 bug fix: must protect reads of ivr
//

//
// First get IPI spin lock
//
        add         rpT1 = @gprel(__imp_HalIpiLock), gp
        ;;
        ld8         rpIpiLock = [rpT1]
        ;;
        ACQUIRE_SPINLOCK(rpIpiLock, rpIpiLock, Keih_Lock)
        ;;

//
// Need to mask PCI interrupts at PXB
//
        add         rpT1 = @gprel(__imp_WRITE_PORT_ULONG_SPECIAL),gp 
        add         rpT2 = @gprel(__imp_READ_PORT_ULONG_SPECIAL),gp 
        ;; 
        ld8         rpT1 = [rpT1]          // get function pointer
        ld8         rpT2 = [rpT2]
        ;;
        ld8         rpWP = [rpT1],PlGlobalPointer-PlEntryPoint
        ld8         rpRP = [rpT2]
        ;;
        ld8         rHalGp = [rpT1]
        ;;
        mov         bt0 = rpWP
        ;;

//
//      Write 0x80ff81c0 to i/o port 0xcf8
//
        mov         out0 = 0xcf8
        movl        out1 = 0x80ff81c0
        mov         gp = rHalGp
        ;;
        br.call.sptk brp = bt0
        ;;
        mov         gp = rSaveGP
        ;;
//
//      Disable pci writes (write pxb_tcap value to 0xcfc)
//
        mov         out0 = 0xcfc
        add         rpT1 = @gprel(__imp_HalPxbTcap), gp
        ;;
        ld8         rpT1 = [rpT1]
        ;;
        ld4         rPxbTcap = [rpT1]
        ;;
        mov         out1 = rPxbTcap
        movl        rT1 = 0xff03ffff 
        ;;
        and         out1 = out1, rT1        // clear bits 23:18
        mov         bt0 = rpWP
        mov         gp = rHalGp
        ;;
        br.call.sptk brp = bt0
        ;;
//
// Read eoi
//
        mov         out0 = 0xcfc
        mov         bt0 = rpRP
        mov         gp = rHalGp
        ;;
        br.call.sptk brp = bt0               // call READ_PORT_LONG
        ;;
        mov         rVector = cr.ivr
        ;;
//
// Enable PCI
//
        mov         out0 = 0xcfc
        mov         out1 = rPxbTcap
        mov         bt0 = rpWP
        mov         gp = rHalGp
        ;;
        br.call.spnt brp = bt0
        ;;
        mov         gp = rSaveGP


//
// Enable IPIs
// 
        ;;
        RELEASE_SPINLOCK(rpIpiLock)
        ;;

#endif // A0_2173

#if defined(INTERRUPTION_LOGGING)
        movl        t0 = KiPcr+PcInterruptionCount
        ;;
        ld4.nt1     t0 = [t0]
        mov         t1 = MAX_NUMBER_OF_IHISTORY_RECORDS - 1
        ;;
        add         t0 = -1, t0
        movl        t2 = KiPcr+0x1000
        ;;
        and         t1 = t0, t1
        ;;
        shl         t1 = t1, 5
        ;;
        add         t0 = t2, t1
        ;;
        add         t0 = 24, t0
        ;;
        st8.nta     [t0] = rVector     // save ivr in the Extra0 field
#endif // defined(INTERRUPTION_LOGGING)

//
// setup debug registers if previous mode is user
//

(pUser) br.call.spnt brp = KiSetupDebugRegisters

//
// Exit if spurious interrupt vector
//

        cmp.eq    pt0, pt1 = 0xF, rVector
(pt0)   br.spnt   Keih_Exit        
        ;;

//
// sp -> trap frame
//

        add       rpT2 = TrEOFMarker, sp
        movl      rT3 = KTRAP_FRAME_EOF | INTERRUPT_FRAME
        ;;

        st8       [rpT2] = rT3, TrPreviousMode - TrEOFMarker
.pred.rel "mutex",pUser,pKrnl
(pUser) mov       rPreviousMode = UserMode             // set previous mode
(pKrnl) mov       rPreviousMode = KernelMode

        GET_IRQL  (rOldIrql)
        add       rpSaveIrql = TrOldIrql, sp    // -> old irql
        ;;

        st4       [rpT2] = rPreviousMode, TrHandler-TrPreviousMode
        st4       [rpSaveIrql] = rOldIrql       // save irql in trap frame
        mov       rSaveGP = gp                  // save kernel gp

        PROLOGUE_END

Keih_InterruptLoop:
//
// Dispatch the interrupt: first raise the IRQL to the level of the new
// interrupt and enable interrupts.
//

        GET_IRQL_FOR_VECTOR(p0, rNewIrql, rVector)
        movl      rpT3 = KiPcr+PcInterruptRoutine // -> interrupt routine table
        ;;

        shladd    rpT3 = rVector, INT_ROUTINES_SHIFT, rpT3    // base + offset
        SET_IRQL  (rNewIrql)                    // raise to new level
        movl      rpT1 = KiPcr + PcPrcb         // pointer to prcb
        ;;

        LDPTR     (out0, rpT3)                  // out0 -> interrupt dispatcher
        LDPTR     (rpT2, rpT1)
        ;;

        FAST_ENABLE_INTERRUPTS
        add       rpT2 = PbInterruptCount, rpT2 // -> interrupt counter
        ;;

        ld4       rT1 = [rpT2]                  // counter value
        ;;
        add       rT1 = 1, rT1                  // increment
        ;;

//
// Call the interrupt dispatch routine via a function pointer
//

        st4       [rpT2] = rT1                  // store, ignore overflow
        ld8       rT2 = [out0], PlGlobalPointer-PlEntryPoint  // get entry point
        mov       out1 = sp                     // out1 -> trap frame
        ;;

        ld8       gp = [out0], PlEntryPoint-PlGlobalPointer
        mov       bt0 = rT2
        br.call.sptk brp = bt0                  // call interrupt dispatch routine(fptr(routine),tf)
        ;;

        END_OF_INTERRUPT                        // end of interrupt processing
        mov       gp = rSaveGP
        ;;
        IO_END_OF_INTERRUPT(rVector,rT1,rT2,pEOI)
        ;;
        srlz.d
        ;;

//
// Disable interrupts and restore IRQL level
//

        FAST_DISABLE_INTERRUPTS

        ld4       rOldIrql = [rpSaveIrql]
        ;;
        LOWER_IRQL(rOldIrql)
  
//
// Get the next vector number
//
#ifdef A0_2173
        mov         rVector = 0xF    // force spurious
#else 
        mov         rVector = cr.ivr // for A0 2173 workaround
#endif 
        ;;
//
// Loop if more interrupts pending (spurious vector == 0xF)
//

        cmp.ne      pt0 = 0xF, rVector
(pt0)   br.spnt     Keih_InterruptLoop
        ;;

Keih_Exit:

        RETURN_FROM_INTERRUPTION(Keih)

        HANDLER_EXIT(KiExternalInterruptHandler)

//--------------------------------------------------------------------
// Routine:
//
//       KiPanicHandler
//
// Description:
//
//       Handler for panic. Call the bug check routine. A place
//       holder for now.
//
// On entry:
//
//       running on kernel memory stack and kernel backing store
//       sp: top of stack -- points to trap frame
//       interrupts enabled
//
//       IIP: address of bundle causing fault
//
//       IPSR: copy of PSR at time of interruption
//
// Output:
//
//       sp: top of stack -- points to trap frame
//
// Return value:
//
//       none
//
// Notes:
//
//       If ISR code out of bounds, this code will inovke the panic handler
//
//--------------------------------------------------------------------

        HANDLER_ENTRY(KiPanicHandler)

        .prologue
        .unwabi     @nt,  EXCEPTION_FRAME

        mov       rHpT1 = KERNEL_STACK_SIZE
        movl      rTH1 = KiPcr+PcPanicStack
        ;;

        ld8       sp = [rTH1], PcInitialStack-PcPanicStack
        movl      rTH2 = KiPcr+PcSystemReserved
        ;;

        st4       [rTH2] = rPanicCode
        st8       [rTH1] = sp, PcStackLimit-PcInitialStack
        sub       rTH2 = sp, rHpT1
        ;;

        st8       [rTH1] = rTH2, PcInitialBStore-PcStackLimit
        mov       rHpT1 = KERNEL_BSTORE_SIZE
        ;;

        st8       [rTH1] = sp, PcBStoreLimit-PcInitialBStore
        add       rTH2 = rHpT1, sp
        add       sp = -TrapFrameLength, sp
        ;;

        st8       [rTH1] = rTH2
        add       rHpT1 = TrStIPSR, sp
        ;;

        SAVE_INTERRUPTION_STATE(Kph_SaveTrapFrame)

        rpRNAT    = t16
        rpBSPStore= t17
        rBSPStore = t18
        rKBSPStore= t19
        rRNAT     = t20
        rKrnlFPSR = t21

        mov       ar.rsc = r0                  // put RSE in lazy mode
        movl      rKBSPStore = KiPcr+PcInitialBStore
        ;;

        mov       rBSPStore = ar.bspstore
        mov       rRNAT = ar.rnat
        ;;

        ld8       rKBSPStore = [rKBSPStore]
        add       rpRNAT = TrRsRNAT, sp
        add       rpBSPStore = TrRsBSPSTORE, sp
        ;;

        st8       [rpRNAT] = rRNAT
        st8       [rpBSPStore] = rBSPStore
        dep       rKBSPStore = rBSPStore, rKBSPStore, 0, 9
        ;;

        mov       ar.bspstore = rKBSPStore
        mov       ar.rsc = RSC_KERNEL
        ;;

        alloc     t22 = ar.pfs, 0, 0, 5, 0
        ;;

        PROLOGUE_END

        br.call.sptk brp = KiSaveTrapFrame
        ;;

        movl      out0 = KiPcr+PcSystemReserved
        ;;

        ld4       out0 = [out0]                // 1st argument: panic code
        mov       out1 = sp                    // 2nd argument: trap frame
        br.call.sptk.many brp = KeBugCheckEx
        ;;

        nop.m     0
        nop.m     0
        nop.i     0
        ;;

        HANDLER_EXIT(KiPanicHandler)

//++
//--------------------------------------------------------------------
// Routine:
//
//      VOID
//      KiSaveTrapFrame(PKTRAP_FRAME)
//
// Description:
//
//       Save volatile application state in trap frame.
//       Note: sp, brp, UNAT, RSC, predicates, BSP, BSP Store,
//       PFS, DCR, and FPSR saved elsewhere.
//
// Input:
//
//       sp: points to trap frame
//       ar.unat: contains the Nats of sp, gp, teb, which have already
//                been spilled into the trap frame.
//
// Output:
//
//       None
//
// Return value:
//
//       none
//
//--------------------------------------------------------------------

        LEAF_ENTRY(KiSaveTrapFrame)

        .regstk    0, 3, 0, 0

//
// Local register aliases
//

        rpTF1     = loc0
        rpTF2     = loc1
        rL1       = t0
        rL2       = t1
        rL3       = t2
        rL4       = t3
        rL5       = t4


//
// (ar.unat unchanged from point of save)
// Spill temporary (volatile) integer registers
//
         
        alloc       loc2 = 0,3,0,0              // don't destroy static register
        add         rpTF1 = TrIntT0, sp         // -> t0 save area
        add         rpTF2 = TrIntT1, sp         // -> t1 save area
        ;;
        .mem.offset 0,0
        st8.spill [rpTF1] = t0, TrIntT2-TrIntT0 // spill t0 - t22
        .mem.offset 8,0
        st8.spill [rpTF2] = t1, TrIntT3-TrIntT1
        ;;
        .mem.offset 0,0
        st8.spill [rpTF1] = t2, TrIntT4-TrIntT2
        .mem.offset 8,0
        st8.spill [rpTF2] = t3, TrIntT5-TrIntT3
        ;;
        .mem.offset 0,0
        st8.spill [rpTF1] = t4, TrIntT6-TrIntT4
        .mem.offset 8,0
        st8.spill [rpTF2] = t5, TrIntT7-TrIntT5
        ;;
        .mem.offset 0,0
        st8.spill [rpTF1] = t6, TrIntT8-TrIntT6
        .mem.offset 8,0
        st8.spill [rpTF2] = t7, TrIntT9-TrIntT7
        ;;
        .mem.offset 0,0
        st8.spill [rpTF1] = t8, TrIntT10-TrIntT8
        .mem.offset 8,0
        st8.spill [rpTF2] = t9, TrIntT11-TrIntT9
        ;;
        .mem.offset 0,0
        st8.spill [rpTF1] = t10, TrIntT12-TrIntT10
        .mem.offset 8,0
        st8.spill [rpTF2] = t11, TrIntT13-TrIntT11
        ;;
        .mem.offset 0,0
        st8.spill [rpTF1] = t12, TrIntT14-TrIntT12
        .mem.offset 8,0
        st8.spill [rpTF2] = t13, TrIntT15-TrIntT13
        ;;
        .mem.offset 0,0
        st8.spill [rpTF1] = t14, TrIntT16-TrIntT14
        .mem.offset 8,0
        st8.spill [rpTF2] = t15, TrIntT17-TrIntT15
        ;;
        .mem.offset 0,0
        st8.spill [rpTF1] = t16, TrIntT18-TrIntT16
        .mem.offset 8,0
        st8.spill [rpTF2] = t17, TrIntT19-TrIntT17
        ;;
        .mem.offset 0,0
        st8.spill [rpTF1] = t18, TrIntT20-TrIntT18
        .mem.offset 8,0
        st8.spill [rpTF2] = t19, TrIntT21-TrIntT19
        ;;
        .mem.offset 0,0
        st8.spill [rpTF1] = t20, TrIntT22-TrIntT20
        .mem.offset 8,0
        st8.spill [rpTF2] = t21, TrIntV0-TrIntT21
        ;;
        .mem.offset 0,0
        st8.spill [rpTF1] = t22, TrBrT0-TrIntT22
        .mem.offset 8,0
        st8.spill [rpTF2] = v0, TrIntNats-TrIntV0       // spill old V0
        ;;

//
// Now save the Nats interger regsisters saved so far (includes Nat for sp)
//

        mov       rL1 = ar.unat
        mov       rL2 = bt0
        mov       rL3 = bt1
        ;;

        st8       [rpTF2] = rL1, TrBrT1-TrIntNats       // save Nats of volatile regs
        mov       rL4 = ar.ccv
        ;;

//
// Save temporary (volatile) branch registers
//

        st8       [rpTF1] = rL2, TrApCCV-TrBrT0         // save old bt0 - bt1
        st8       [rpTF2] = rL3
        ;;

        st8       [rpTF1] = rL4                         // save ar.ccv
        add       rpTF1 = TrFltT0, sp                   // point to FltT0
        add       rpTF2 = TrFltT1, sp                   // point to FltT1
        ;;

//
// Spill temporary (volatile) floating point registers
//

        stf.spill [rpTF1] = ft0, TrFltT2-TrFltT0        // spill float tmp 0 - 9
        stf.spill [rpTF2] = ft1, TrFltT3-TrFltT1
        ;;
        stf.spill [rpTF1] = ft2, TrFltT4-TrFltT2
        stf.spill [rpTF2] = ft3, TrFltT5-TrFltT3
        ;;
        stf.spill [rpTF1] = ft4, TrFltT6-TrFltT4
        stf.spill [rpTF2] = ft5, TrFltT7-TrFltT5
        ;;
        stf.spill [rpTF1] = ft6, TrFltT8-TrFltT6
        stf.spill [rpTF2] = ft7, TrFltT9-TrFltT7
        ;;
        stf.spill [rpTF1] = ft8
        stf.spill [rpTF2] = ft9
        ;;

        rum       1 << PSR_MFL                          // clear mfl bit

//
// TBD **** Debug/performance regs ** ?
// **** Performance regs not needed (either user or system wide)
// No performance regs switched on kernel entry
// **** Debug regs saved if in use
//

        LEAF_RETURN
        ;;
        LEAF_EXIT(KiSaveTrapFrame)

//++
//--------------------------------------------------------------------
// Routine:
//
//      VOID
//      KiRestoreTrapFrame(PKTRAP_FRAME)
//
// Description:
//
//       Restore volatile application state from trap frame. Restore DCR
//       Note: sp, brp, RSC, UNAT, predicates, BSP, BSP Store, PFS,
//       DCR and FPSR not restored here.
//
// Input:
//
//      sp: points to trap frame
//      RSE frame size is zero
//
// Output:
//
//      None
//
// Return value:
//
//       none
//
//--------------------------------------------------------------------

        LEAF_ENTRY(KiRestoreTrapFrame)

        LEAF_SETUP(0,2,0,0)

        rpTF1     = loc0
        rpTF2     = loc1

//
// **** TBD **** Restore debug/performance registers??
// **** Performance regs not needed (either user or system wide)
// No performance regs switched on kernel entry
// **** Debug regs saved if in use
//

//
// Restore RSC, CCV, DCR, and volatile branch, floating point, integer register
//

        mov       t21 = psr
        add       rpTF1 = TrRsRSC, sp
        add       rpTF2 = TrApCCV, sp
        ;;

        ld8       t5 = [rpTF1], TrIntNats-TrRsRSC
        ld8       t1 = [rpTF2], TrApDCR-TrApCCV
        ;;

        ld8       t0 = [rpTF1], TrBrT0-TrIntNats
        ld8       t3 = [rpTF2], TrBrT1-TrApDCR
        ;;

        ld8       t2 = [rpTF1]
        ld8       t4 = [rpTF2]

        mov       ar.rsc = t5
        mov       ar.ccv = t1
        add       rpTF1 = TrIntGp, sp

        mov       ar.unat = t0
        mov       cr.dcr = t3
        add       rpTF2 = TrIntT0, sp
        ;;

        ld8.fill  gp = [rpTF1], TrIntT1-TrIntGp
        ld8.fill  t0 = [rpTF2], TrIntT2-TrIntT0 
        mov       bt0 = t2
        ;;

        ld8.fill  t1 = [rpTF1], TrIntT3-TrIntT1
        ld8.fill  t2 = [rpTF2], TrIntT4-TrIntT2
        mov       bt1 = t4
        ;;

        ld8.fill  t3 = [rpTF1], TrIntT5-TrIntT3
        ld8.fill  t4 = [rpTF2], TrIntT6-TrIntT4
        tbit.z    pt1 = t21, PSR_MFL
        ;;

        ld8.fill  t5 = [rpTF1], TrIntT7-TrIntT5
        ld8.fill  t6 = [rpTF2], TrIntT8-TrIntT6
        ;;

        ld8.fill  t7 = [rpTF1], TrIntT9-TrIntT7
        ld8.fill  t8 = [rpTF2], TrIntT10-TrIntT8
        ;;

        ld8.fill  t9 = [rpTF1], TrIntT11-TrIntT9
        ld8.fill  t10 = [rpTF2], TrIntT12-TrIntT10
        ;;

        ld8.fill  t11 = [rpTF1], TrIntT13-TrIntT11
        ld8.fill  t12 = [rpTF2], TrIntT14-TrIntT12
        ;;

        ld8.fill  t13 = [rpTF1], TrIntT15-TrIntT13
        ld8.fill  t14 = [rpTF2], TrIntT16-TrIntT14
        ;;

        ld8.fill  t15 = [rpTF1], TrIntT17-TrIntT15
        ld8.fill  t16 = [rpTF2], TrIntT18-TrIntT16
        ;;

        ld8.fill  t17 = [rpTF1], TrIntT19-TrIntT17
        ld8.fill  t18 = [rpTF2], TrIntT20-TrIntT18
        ;;

        ld8.fill  t19 = [rpTF1], TrIntT21-TrIntT19
        ld8.fill  t20 = [rpTF2], TrIntT22-TrIntT20
        ;;

        ld8.fill  t21 = [rpTF1], TrIntTeb-TrIntT21
        ld8.fill  t22 = [rpTF2], TrIntV0-TrIntT22
        ;;

        ld8.fill  teb = [rpTF1], TrFltT1-TrIntTeb
        ld8.fill  v0 = [rpTF2], TrFltT0-TrIntV0
        ;;

        ldf.fill  ft0 = [rpTF2], TrFltT2-TrFltT0
        ldf.fill  ft1 = [rpTF1], TrFltT3-TrFltT1
        ;;
        
        ldf.fill  ft2 = [rpTF2], TrFltT4-TrFltT2
        ldf.fill  ft3 = [rpTF1], TrFltT5-TrFltT3
        ;;
        
        ldf.fill  ft4 = [rpTF2], TrFltT6-TrFltT4
        ldf.fill  ft5 = [rpTF1], TrFltT7-TrFltT5
        ;;
        
        ldf.fill  ft6 = [rpTF2], TrFltT8-TrFltT6
        ldf.fill  ft7 = [rpTF1], TrFltT9-TrFltT7
        ;;
        
        ldf.fill  ft8 = [rpTF2]
        ldf.fill  ft9 = [rpTF1]
        br.ret.sptk.many brp
        ;;
        
        LEAF_EXIT(KiRestoreTrapFrame)

//++
//--------------------------------------------------------------------
// Routine:
//
//      VOID
//      KiSetupDebugRegisters
//
// Description:
//
//      We maintain two debug register flags:
//         1. Thread DebugActive: Debug registers active for current thread
//         2. PCR KernelDebugActive: Debug registers active in kernel mode
//            (setup by kernel debugger)
//
//      On user -> kernel transitions there are four possibilities:
//
//               Thread        Kernel 
//               DebugActive   DebugActive   Action
//
//      1.       0             0             None
//
//      2.       1             0             None (kernel PSR.db = 0 by default)
//
//      3.       0             1             Set PSR.db = 1 for kernel
//
//      4.       1             1             Set PSR.db = 1 for kernel and
//                                           load kernel debug registers
//
//      Note we never save the user debug registers: 
//      the user cannot change the DRs so the values in the DR save area are 
//      always up-to-date (set by SetContext).
//
// Input:
//
//       None (Previous mode is USER)
//
// Output:
//
//       None
//
// Return value:
//
//       none
//
//--------------------------------------------------------------------

        LEAF_ENTRY(KiSetupDebugRegisters)

//
// *** TBD -- no support for kernel debug registers (KernelDebugActive = 0)
// All the calls to this function are removed and have to be reinstated
// when hardware debug support is implemented in the kernel debugger.
//

        LEAF_RETURN
        LEAF_EXIT(KiSetupDebugRegisters)

//++
//--------------------------------------------------------------------
// Routine:
//
//      VOID
//      KiRestoreDebugRegisters
//
// Description:
//
//      If debug active, restore user debug registers from DR save area in
//      kernel stack.
//
// Input:
//
//       None
//
// Output:
//
//       None
//
// Return value:
//
//       none
//
// Note:
//      We find the DR save are from the the StackBase not PCR->InitialStack,
//      which can be changed in KiCallUserMode().
//
//--------------------------------------------------------------------

        LEAF_ENTRY(KiRestoreDebugRegisters)

//
// Local register aliases
//

        rpSA0       = t0
        rpSA1       = t1
        rDebugActive = t2
        rpT1        = t3
        rPrcb       = t4
        rDr0        = t5
        rDr1        = t6
        rDrIndex0   = t7
        rDrIndex1   = t8
        rSaveLC     = t9
        rpCurrentThread = t10
        rStackBase  = t11

        pNoRestore  = pt0

//
// Restore debug registers, if debug active
//

//
//  Do not touch debug regs for power on
//
        LEAF_RETURN
        ;;
 
        movl        rpT1 = KiPcr+PcCurrentThread
        ;;
        mov         rSaveLC = ar.lc             // save
        ld8         rpCurrentThread = [rpT1]    // get Current thread pointer
        ;;
        add         rpT1 = ThDebugActive, rpCurrentThread
        add         rStackBase = ThStackBase, rpCurrentThread
        ;;
        ld1         rDebugActive = [rpT1]       // get thread debug active flag
        ;;
        cmp.eq      pNoRestore = zero, rDebugActive
(pNoRestore) br.sptk Krdr_Exit                   // skip if not active
        ;;
        mov         rDrIndex0 = 0               
        mov         rDrIndex1 = 1
        ;;
        add         rpSA0 = -ThreadStateSaveAreaLength+TsDebugRegisters+DrDbI0,rStackBase
        add         rpSA1 = -ThreadStateSaveAreaLength+TsDebugRegisters+DrDbI1,rStackBase
        mov         ar.lc = 3                   // 4 pair of ibr
        ;;
Krdr_ILoop:
        ld8         rDr0 = [rpSA0], 16          // get ibr pair
        ld8         rDr1 = [rpSA1], 16          // step by 16 = 1 pair of DRs
        ;;
        .auto
        mov         ibr[rDrIndex0] = rDr0       // restore ibr pair
        mov         ibr[rDrIndex1] = rDr1
        ;;
        add         rDrIndex0 = 1, rDrIndex0    // next pair
        add         rDrIndex1 = 1, rDrIndex1
        br.cloop.sptk Krdr_ILoop
        ;;
        mov         ar.lc = 3                   // 4 pair of dbr
        mov         rDrIndex0 = 0
        mov         rDrIndex1 = 1
        ;;
Krdr_DLoop:
        ld8         rDr0 = [rpSA0], 16          // get dbr pair
        ld8         rDr1 = [rpSA1], 16          // step by 16 = 1 pair of DRs
        ;;
        mov         dbr[rDrIndex0] = rDr0       // restore dbr pair
        mov         dbr[rDrIndex1] = rDr1
        ;;
        .default
        add         rDrIndex0 = 1, rDrIndex0    // next pair
        add         rDrIndex1 = 1, rDrIndex1
        br.cloop.sptk Krdr_DLoop
        ;;
        mov         ar.lc = rSaveLC             // restore
Krdr_Exit:
        LEAF_RETURN
        LEAF_EXIT(KiRestoreDebugRegisters)

//++
//--------------------------------------------------------------------
// Routine:
//
//      VOID
//      KiSaveExceptionFrame(PKEXCEPTION_FRAME)
//
// Description:
//
//      Save preserved context in exception frame.
//
// Input:
//
//      a0: points to exception frame
//
// Output:
//
//      None
//
// Return value:
//
//      none
//
// Note: t0 may contain the trap frame address; don't touch it.
//
//--------------------------------------------------------------------

        LEAF_ENTRY(KiSaveExceptionFrame)

//
// Local register aliases
//

        rpEF1     = t10
        rpEF2     = t11

        add       rpEF1 = ExIntS0, a0            // -> ExIntS0
        add       rpEF2 = ExIntS1, a0            // -> ExIntS1
        mov       t3 = ar.ec
        ;;

        .mem.offset 0,0
        st8.spill [rpEF1] = s0, ExIntS2-ExIntS0
        .mem.offset 8,0
        st8.spill [rpEF2] = s1, ExIntS3-ExIntS1
        mov       t4 = ar.lc
        ;;

        .mem.offset 0,0
        st8.spill [rpEF1] = s2, ExApEC-ExIntS2
        .mem.offset 8,0
        st8.spill [rpEF2] = s3, ExApLC-ExIntS3
        mov       t5 = bs0
        ;;

        st8       [rpEF1] = t3, ExBrS0-ExApEC
        st8       [rpEF2] = t4, ExBrS1-ExApLC
        mov       t6 = bs1
        ;;

        mov       t2 = ar.unat                   // save user nat register for
        mov       t7 = bs2
        mov       t8 = bs3

        st8       [rpEF1] = t5, ExBrS2-ExBrS0
        st8       [rpEF2] = t6, ExBrS3-ExBrS1
        mov       t9 = bs4
        ;;

        st8       [rpEF1] = t7, ExBrS4-ExBrS2
        st8       [rpEF2] = t8, ExIntNats-ExBrS3
        ;;

        st8       [rpEF1] = t9, ExFltS0-ExBrS4
        st8       [rpEF2] = t2, ExFltS1-ExIntNats
        ;;

        stf.spill [rpEF1] = fs0, ExFltS2-ExFltS0
        stf.spill [rpEF2] = fs1, ExFltS3-ExFltS1
        ;;

        stf.spill [rpEF1] = fs2, ExFltS4-ExFltS2
        stf.spill [rpEF2] = fs3, ExFltS5-ExFltS3
        ;;

        stf.spill [rpEF1] = fs4, ExFltS6-ExFltS4
        stf.spill [rpEF2] = fs5, ExFltS7-ExFltS5
        ;;

        stf.spill [rpEF1] = fs6, ExFltS8-ExFltS6
        stf.spill [rpEF2] = fs7, ExFltS9-ExFltS7
        ;;

        stf.spill [rpEF1] = fs8, ExFltS10-ExFltS8
        stf.spill [rpEF2] = fs9, ExFltS11-ExFltS9
        ;;

        stf.spill [rpEF1] = fs10, ExFltS12-ExFltS10
        stf.spill [rpEF2] = fs11, ExFltS13-ExFltS11
        ;;

        stf.spill [rpEF1] = fs12, ExFltS14-ExFltS12
        stf.spill [rpEF2] = fs13, ExFltS15-ExFltS13
        ;;

        stf.spill [rpEF1] = fs14, ExFltS16-ExFltS14
        stf.spill [rpEF2] = fs15, ExFltS17-ExFltS15
        ;;

        stf.spill [rpEF1] = fs16, ExFltS18-ExFltS16
        stf.spill [rpEF2] = fs17, ExFltS19-ExFltS17
        ;;

        stf.spill [rpEF1] = fs18
        stf.spill [rpEF2] = fs19
        LEAF_RETURN
        ;;

        LEAF_EXIT(KiSaveExceptionFrame)

//--------------------------------------------------------------------
// Routine:
//
//      VOID
//      KiRestoreExceptionFrame(PKEXCEPTION_FRAME)
//
// Description:
//
//       Restores preserved context from the exception frame. Also
//       restore volatile part of floating point context not restored with
//       rest of volatile context.
//
// Input:
//
//      a0: points to exception frame
//
// Output:
//
//      None
//
// Return value:
//
//      none
//
//--------------------------------------------------------------------

        LEAF_ENTRY(KiRestoreExceptionFrame)

//
// Local register aliases
//

        rpEF1     = t10
        rpEF2     = t11

        add       rpEF1 = ExIntNats, a0
        add       rpEF2 = ExApEC, a0
        ;;

        ld8       t2 = [rpEF1], ExBrS0-ExIntNats
        ld8       t3 = [rpEF2], ExApLC-ExApEC
        ;;

        ld8       t5 = [rpEF1], ExBrS1-ExBrS0
        ld8       t4 = [rpEF2], ExBrS2-ExApLC
        ;;

        mov       ar.unat = t2
        mov       ar.ec = t3
        ;;

        ld8       t6 = [rpEF1], ExBrS3-ExBrS1
        ld8       t7 = [rpEF2], ExBrS4-ExBrS2
        mov       ar.lc = t4
        ;;

        ld8       t8 = [rpEF1], ExIntS0-ExBrS3
        ld8       t9 = [rpEF2], ExIntS1-ExBrS4
        mov       bs0 = t5
        ;;

        ld8.fill  s0 = [rpEF1], ExIntS2-ExIntS0
        ld8.fill  s1 = [rpEF2], ExIntS3-ExIntS1
        mov       bs1 = t6
        ;;

        ld8.fill  s2 = [rpEF1], ExFltS0-ExIntS2
        ld8.fill  s3 = [rpEF2], ExFltS1-ExIntS3
        mov       bs2 = t7
        ;;

        ldf.fill  fs0 = [rpEF1], ExFltS2-ExFltS0
        ldf.fill  fs1 = [rpEF2], ExFltS3-ExFltS1
        mov       bs3 = t8
        ;;

        ldf.fill  fs2 = [rpEF1], ExFltS4-ExFltS2
        ldf.fill  fs3 = [rpEF2], ExFltS5-ExFltS3
        mov       bs4 = t9
        ;;

        ldf.fill  fs4 = [rpEF1], ExFltS6-ExFltS4
        ldf.fill  fs5 = [rpEF2], ExFltS7-ExFltS5
        ;;

        ldf.fill  fs6 = [rpEF1], ExFltS8-ExFltS6
        ldf.fill  fs7 = [rpEF2], ExFltS9-ExFltS7
        ;;

        ldf.fill  fs8 = [rpEF1], ExFltS10-ExFltS8
        ldf.fill  fs9 = [rpEF2], ExFltS11-ExFltS9
        ;;

        ldf.fill  fs10 = [rpEF1], ExFltS12-ExFltS10
        ldf.fill  fs11 = [rpEF2], ExFltS13-ExFltS11
        ;;

        ldf.fill  fs12 = [rpEF1], ExFltS14-ExFltS12
        ldf.fill  fs13 = [rpEF2], ExFltS15-ExFltS13
        ;;

        ldf.fill  fs14 = [rpEF1], ExFltS16-ExFltS14
        ldf.fill  fs15 = [rpEF2], ExFltS17-ExFltS15
        ;;

        ldf.fill  fs16 = [rpEF1], ExFltS18-ExFltS16
        ldf.fill  fs17 = [rpEF2], ExFltS19-ExFltS17
        ;;

        ldf.fill  fs18 = [rpEF1]
        ldf.fill  fs19 = [rpEF2]
        LEAF_RETURN
        ;;
        LEAF_EXIT(KiRestoreExceptionFrame)

//++
//--------------------------------------------------------------------
// Routine:
//
//      KiSaveHigherFPVolatile(PKHIGHER_FP_SAVEAREA)
//
// Description:
//
//       Save higher FP volatile context in higher FP save area
//
// Input:
//
//       a0: pointer to higher FP save area
//       brp: return address
//
// Output:
//
//       None
//
// Return value:
//
//       None
//
//--------------------------------------------------------------------

        LEAF_ENTRY(KiSaveHigherFPVolatile)

//
// Local register aliases
//

        rpSA1     = t0
        rpSA2     = t1

//
// Spill higher floating point volatile registers f32-f127.
// Must add length of preserved area within FP save area to
// point to volatile save area.
//

//
// Clear DFH bit so the high floating point set may be saved by the kernel
// Disable interrupts so that save is atomic
//

        rsm       (1 << PSR_DFH) | (1 << PSR_I)
        add       rpSA1 = HiFltF32, a0      // -> HiFltF32
        add       rpSA2 = HiFltF33, a0      // -> HiFltF33
        ;;

        srlz.d
        ;;

        stf.spill [rpSA1] = f32, HiFltF34-HiFltF32
        stf.spill [rpSA2] = f33, HiFltF35-HiFltF33
        ;;
        stf.spill [rpSA1] = f34, HiFltF36-HiFltF34
        stf.spill [rpSA2] = f35, HiFltF37-HiFltF35
        ;;
        stf.spill [rpSA1] = f36, HiFltF38-HiFltF36
        stf.spill [rpSA2] = f37, HiFltF39-HiFltF37
        ;;
        stf.spill [rpSA1] = f38, HiFltF40-HiFltF38
        stf.spill [rpSA2] = f39, HiFltF41-HiFltF39
        ;;

        stf.spill [rpSA1] = f40, HiFltF42-HiFltF40
        stf.spill [rpSA2] = f41, HiFltF43-HiFltF41
        ;;
        stf.spill [rpSA1] = f42, HiFltF44-HiFltF42
        stf.spill [rpSA2] = f43, HiFltF45-HiFltF43
        ;;
        stf.spill [rpSA1] = f44, HiFltF46-HiFltF44
        stf.spill [rpSA2] = f45, HiFltF47-HiFltF45
        ;;
        stf.spill [rpSA1] = f46, HiFltF48-HiFltF46
        stf.spill [rpSA2] = f47, HiFltF49-HiFltF47
        ;;
        stf.spill [rpSA1] = f48, HiFltF50-HiFltF48
        stf.spill [rpSA2] = f49, HiFltF51-HiFltF49
        ;;

        stf.spill [rpSA1] = f50, HiFltF52-HiFltF50
        stf.spill [rpSA2] = f51, HiFltF53-HiFltF51
        ;;
        stf.spill [rpSA1] = f52, HiFltF54-HiFltF52
        stf.spill [rpSA2] = f53, HiFltF55-HiFltF53
        ;;
        stf.spill [rpSA1] = f54, HiFltF56-HiFltF54
        stf.spill [rpSA2] = f55, HiFltF57-HiFltF55
        ;;
        stf.spill [rpSA1] = f56, HiFltF58-HiFltF56
        stf.spill [rpSA2] = f57, HiFltF59-HiFltF57
        ;;
        stf.spill [rpSA1] = f58, HiFltF60-HiFltF58
        stf.spill [rpSA2] = f59, HiFltF61-HiFltF59
        ;;

        stf.spill [rpSA1] = f60, HiFltF62-HiFltF60
        stf.spill [rpSA2] = f61, HiFltF63-HiFltF61
        ;;
        stf.spill [rpSA1] = f62, HiFltF64-HiFltF62
        stf.spill [rpSA2] = f63, HiFltF65-HiFltF63
        ;;
        stf.spill [rpSA1] = f64, HiFltF66-HiFltF64
        stf.spill [rpSA2] = f65, HiFltF67-HiFltF65
        ;;
        stf.spill [rpSA1] = f66, HiFltF68-HiFltF66
        stf.spill [rpSA2] = f67, HiFltF69-HiFltF67
        ;;
        stf.spill [rpSA1] = f68, HiFltF70-HiFltF68
        stf.spill [rpSA2] = f69, HiFltF71-HiFltF69
        ;;

        stf.spill [rpSA1] = f70, HiFltF72-HiFltF70
        stf.spill [rpSA2] = f71, HiFltF73-HiFltF71
        ;;
        stf.spill [rpSA1] = f72, HiFltF74-HiFltF72
        stf.spill [rpSA2] = f73, HiFltF75-HiFltF73
        ;;
        stf.spill [rpSA1] = f74, HiFltF76-HiFltF74
        stf.spill [rpSA2] = f75, HiFltF77-HiFltF75
        ;;
        stf.spill [rpSA1] = f76, HiFltF78-HiFltF76
        stf.spill [rpSA2] = f77, HiFltF79-HiFltF77
        ;;
        stf.spill [rpSA1] = f78, HiFltF80-HiFltF78
        stf.spill [rpSA2] = f79, HiFltF81-HiFltF79
        ;;

        stf.spill [rpSA1] = f80, HiFltF82-HiFltF80
        stf.spill [rpSA2] = f81, HiFltF83-HiFltF81
        ;;
        stf.spill [rpSA1] = f82, HiFltF84-HiFltF82
        stf.spill [rpSA2] = f83, HiFltF85-HiFltF83
        ;;
        stf.spill [rpSA1] = f84, HiFltF86-HiFltF84
        stf.spill [rpSA2] = f85, HiFltF87-HiFltF85
        ;;
        stf.spill [rpSA1] = f86, HiFltF88-HiFltF86
        stf.spill [rpSA2] = f87, HiFltF89-HiFltF87
        ;;
        stf.spill [rpSA1] = f88, HiFltF90-HiFltF88
        stf.spill [rpSA2] = f89, HiFltF91-HiFltF89
        ;;

        stf.spill [rpSA1] = f90, HiFltF92-HiFltF90
        stf.spill [rpSA2] = f91, HiFltF93-HiFltF91
        ;;
        stf.spill [rpSA1] = f92, HiFltF94-HiFltF92
        stf.spill [rpSA2] = f93, HiFltF95-HiFltF93
        ;;
        stf.spill [rpSA1] = f94, HiFltF96-HiFltF94
        stf.spill [rpSA2] = f95, HiFltF97-HiFltF95
        ;;
        stf.spill [rpSA1] = f96, HiFltF98-HiFltF96
        stf.spill [rpSA2] = f97, HiFltF99-HiFltF97
        ;;
        stf.spill [rpSA1] = f98, HiFltF100-HiFltF98
        stf.spill [rpSA2] = f99, HiFltF101-HiFltF99
        ;;

        stf.spill [rpSA1] = f100, HiFltF102-HiFltF100
        stf.spill [rpSA2] = f101, HiFltF103-HiFltF101
        ;;
        stf.spill [rpSA1] = f102, HiFltF104-HiFltF102
        stf.spill [rpSA2] = f103, HiFltF105-HiFltF103
        ;;
        stf.spill [rpSA1] = f104, HiFltF106-HiFltF104
        stf.spill [rpSA2] = f105, HiFltF107-HiFltF105
        ;;
        stf.spill [rpSA1] = f106, HiFltF108-HiFltF106
        stf.spill [rpSA2] = f107, HiFltF109-HiFltF107
        ;;
        stf.spill [rpSA1] = f108, HiFltF110-HiFltF108
        stf.spill [rpSA2] = f109, HiFltF111-HiFltF109
        ;;

        stf.spill [rpSA1] = f110, HiFltF112-HiFltF110
        stf.spill [rpSA2] = f111, HiFltF113-HiFltF111
        ;;
        stf.spill [rpSA1] = f112, HiFltF114-HiFltF112
        stf.spill [rpSA2] = f113, HiFltF115-HiFltF113
        ;;
        stf.spill [rpSA1] = f114, HiFltF116-HiFltF114
        stf.spill [rpSA2] = f115, HiFltF117-HiFltF115
        ;;
        stf.spill [rpSA1] = f116, HiFltF118-HiFltF116
        stf.spill [rpSA2] = f117, HiFltF119-HiFltF117
        ;;
        stf.spill [rpSA1] = f118, HiFltF120-HiFltF118
        stf.spill [rpSA2] = f119, HiFltF121-HiFltF119
        ;;

        stf.spill [rpSA1] = f120, HiFltF122-HiFltF120
        stf.spill [rpSA2] = f121, HiFltF123-HiFltF121
        ;;
        stf.spill [rpSA1] = f122, HiFltF124-HiFltF122
        stf.spill [rpSA2] = f123, HiFltF125-HiFltF123
        ;;
        stf.spill [rpSA1] = f124, HiFltF126-HiFltF124
        stf.spill [rpSA2] = f125, HiFltF127-HiFltF125
        ;;
        stf.spill [rpSA1] = f126
        stf.spill [rpSA2] = f127

//
// Set DFH bit so the high floating point set may not be used by the kernel
// Must clear mfh after fp registers saved
//

        rsm       1 << PSR_MFH
        ssm       (1 << PSR_DFH) | (1 << PSR_I)
        ;;
        srlz.d
        ;;
        LEAF_RETURN

        LEAF_EXIT(KiSaveHigherFPVolatile)

//++
//--------------------------------------------------------------------
// Routine:
//
//      KiRestoreHigherFPVolatile()
//
// Description:
//
//       Restore higher FP volatile context from higher FP save area
//
//       N.B. This function is carefully constructed to use only scratch 
//            registers rHpT1, rHpT3, and rTH2.  This function may be
//            called by C code and the disabled fp vector when user
//            and kernel bank is used respectively.
//       N.B. Caller must ensure higher fp enabled (psr.dfh=0)
//       N.B. Caller must ensure no interrupt during restore
//
// Input:
//
//       None.
//
// Output:
//
//       None
//
// Return value:
//
//       None
//
//--------------------------------------------------------------------

        LEAF_ENTRY(KiRestoreHigherFPVolatile)

//
// rHpT1 & rHpT3 are 2 registers that are available as 
// scratch registers in this function.
//

        srlz.d
        movl      rHpT1 = KiPcr+PcInitialStack
        ;;

        ld8       rTH2 = [rHpT1]
        ;;
        add       rHpT1 = -ThreadStateSaveAreaLength+TsHigherFPVolatile+HiFltF32, rTH2
        add       rHpT3 = -ThreadStateSaveAreaLength+TsHigherFPVolatile+HiFltF33, rTH2
        ;;

        ldf.fill  f32 = [rHpT1], HiFltF34-HiFltF32
        ldf.fill  f33 = [rHpT3], HiFltF35-HiFltF33
        ;;

        ldf.fill  f34 = [rHpT1], HiFltF36-HiFltF34
        ldf.fill  f35 = [rHpT3], HiFltF37-HiFltF35
        ;;
        ldf.fill  f36 = [rHpT1], HiFltF38-HiFltF36
        ldf.fill  f37 = [rHpT3], HiFltF39-HiFltF37
        ;;
        ldf.fill  f38 = [rHpT1], HiFltF40-HiFltF38
        ldf.fill  f39 = [rHpT3], HiFltF41-HiFltF39
        ;;

        ldf.fill  f40 = [rHpT1], HiFltF42-HiFltF40
        ldf.fill  f41 = [rHpT3], HiFltF43-HiFltF41
        ;;
        ldf.fill  f42 = [rHpT1], HiFltF44-HiFltF42
        ldf.fill  f43 = [rHpT3], HiFltF45-HiFltF43
        ;;
        ldf.fill  f44 = [rHpT1], HiFltF46-HiFltF44
        ldf.fill  f45 = [rHpT3], HiFltF47-HiFltF45
        ;;
        ldf.fill  f46 = [rHpT1], HiFltF48-HiFltF46
        ldf.fill  f47 = [rHpT3], HiFltF49-HiFltF47
        ;;
        ldf.fill  f48 = [rHpT1], HiFltF50-HiFltF48
        ldf.fill  f49 = [rHpT3], HiFltF51-HiFltF49
        ;;

        ldf.fill  f50 = [rHpT1], HiFltF52-HiFltF50
        ldf.fill  f51 = [rHpT3], HiFltF53-HiFltF51
        ;;
        ldf.fill  f52 = [rHpT1], HiFltF54-HiFltF52
        ldf.fill  f53 = [rHpT3], HiFltF55-HiFltF53
        ;;
        ldf.fill  f54 = [rHpT1], HiFltF56-HiFltF54
        ldf.fill  f55 = [rHpT3], HiFltF57-HiFltF55
        ;;
        ldf.fill  f56 = [rHpT1], HiFltF58-HiFltF56
        ldf.fill  f57 = [rHpT3], HiFltF59-HiFltF57
        ;;
        ldf.fill  f58 = [rHpT1], HiFltF60-HiFltF58
        ldf.fill  f59 = [rHpT3], HiFltF61-HiFltF59
        ;;

        ldf.fill  f60 = [rHpT1], HiFltF62-HiFltF60
        ldf.fill  f61 = [rHpT3], HiFltF63-HiFltF61
        ;;
        ldf.fill  f62 = [rHpT1], HiFltF64-HiFltF62
        ldf.fill  f63 = [rHpT3], HiFltF65-HiFltF63
        ;;
        ldf.fill  f64 = [rHpT1], HiFltF66-HiFltF64
        ldf.fill  f65 = [rHpT3], HiFltF67-HiFltF65
        ;;
        ldf.fill  f66 = [rHpT1], HiFltF68-HiFltF66
        ldf.fill  f67 = [rHpT3], HiFltF69-HiFltF67
        ;;
        ldf.fill  f68 = [rHpT1], HiFltF70-HiFltF68
        ldf.fill  f69 = [rHpT3], HiFltF71-HiFltF69
        ;;

        ldf.fill  f70 = [rHpT1], HiFltF72-HiFltF70
        ldf.fill  f71 = [rHpT3], HiFltF73-HiFltF71
        ;;
        ldf.fill  f72 = [rHpT1], HiFltF74-HiFltF72
        ldf.fill  f73 = [rHpT3], HiFltF75-HiFltF73
        ;;
        ldf.fill  f74 = [rHpT1], HiFltF76-HiFltF74
        ldf.fill  f75 = [rHpT3], HiFltF77-HiFltF75
        ;;
        ldf.fill  f76 = [rHpT1], HiFltF78-HiFltF76
        ldf.fill  f77 = [rHpT3], HiFltF79-HiFltF77
        ;;
        ldf.fill  f78 = [rHpT1], HiFltF80-HiFltF78
        ldf.fill  f79 = [rHpT3], HiFltF81-HiFltF79
        ;;

        ldf.fill  f80 = [rHpT1], HiFltF82-HiFltF80
        ldf.fill  f81 = [rHpT3], HiFltF83-HiFltF81
        ;;
        ldf.fill  f82 = [rHpT1], HiFltF84-HiFltF82
        ldf.fill  f83 = [rHpT3], HiFltF85-HiFltF83
        ;;
        ldf.fill  f84 = [rHpT1], HiFltF86-HiFltF84
        ldf.fill  f85 = [rHpT3], HiFltF87-HiFltF85
        ;;
        ldf.fill  f86 = [rHpT1], HiFltF88-HiFltF86
        ldf.fill  f87 = [rHpT3], HiFltF89-HiFltF87
        ;;
        ldf.fill  f88 = [rHpT1], HiFltF90-HiFltF88
        ldf.fill  f89 = [rHpT3], HiFltF91-HiFltF89
        ;;

        ldf.fill  f90 = [rHpT1], HiFltF92-HiFltF90
        ldf.fill  f91 = [rHpT3], HiFltF93-HiFltF91
        ;;
        ldf.fill  f92 = [rHpT1], HiFltF94-HiFltF92
        ldf.fill  f93 = [rHpT3], HiFltF95-HiFltF93
        ;;
        ldf.fill  f94 = [rHpT1], HiFltF96-HiFltF94
        ldf.fill  f95 = [rHpT3], HiFltF97-HiFltF95
        ;;
        ldf.fill  f96 = [rHpT1], HiFltF98-HiFltF96
        ldf.fill  f97 = [rHpT3], HiFltF99-HiFltF97
        ;;
        ldf.fill  f98 = [rHpT1], HiFltF100-HiFltF98
        ldf.fill  f99 = [rHpT3], HiFltF101-HiFltF99
        ;;

        ldf.fill  f100 = [rHpT1], HiFltF102-HiFltF100
        ldf.fill  f101 = [rHpT3], HiFltF103-HiFltF101
        ;;
        ldf.fill  f102 = [rHpT1], HiFltF104-HiFltF102
        ldf.fill  f103 = [rHpT3], HiFltF105-HiFltF103
        ;;
        ldf.fill  f104 = [rHpT1], HiFltF106-HiFltF104
        ldf.fill  f105 = [rHpT3], HiFltF107-HiFltF105
        ;;
        ldf.fill  f106 = [rHpT1], HiFltF108-HiFltF106
        ldf.fill  f107 = [rHpT3], HiFltF109-HiFltF107
        ;;
        ldf.fill  f108 = [rHpT1], HiFltF110-HiFltF108
        ldf.fill  f109 = [rHpT3], HiFltF111-HiFltF109
        ;;

        ldf.fill  f110 = [rHpT1], HiFltF112-HiFltF110
        ldf.fill  f111 = [rHpT3], HiFltF113-HiFltF111
        ;;
        ldf.fill  f112 = [rHpT1], HiFltF114-HiFltF112
        ldf.fill  f113 = [rHpT3], HiFltF115-HiFltF113
        ;;
        ldf.fill  f114 = [rHpT1], HiFltF116-HiFltF114
        ldf.fill  f115 = [rHpT3], HiFltF117-HiFltF115
        ;;
        ldf.fill  f116 = [rHpT1], HiFltF118-HiFltF116
        ldf.fill  f117 = [rHpT3], HiFltF119-HiFltF117
        ;;
        ldf.fill  f118 = [rHpT1], HiFltF120-HiFltF118
        ldf.fill  f119 = [rHpT3], HiFltF121-HiFltF119
        ;;

        ldf.fill  f120 = [rHpT1], HiFltF122-HiFltF120
        ldf.fill  f121 = [rHpT3], HiFltF123-HiFltF121
        ;;
        ldf.fill  f122 = [rHpT1], HiFltF124-HiFltF122
        ldf.fill  f123 = [rHpT3], HiFltF125-HiFltF123
        ;;
        ldf.fill  f124 = [rHpT1], HiFltF126-HiFltF124
        ldf.fill  f125 = [rHpT3], HiFltF127-HiFltF125
        ;;
        ldf.fill  f126 = [rHpT1]
        ldf.fill  f127 = [rHpT3]
        ;;

        rsm       1 << PSR_MFH                 // clear psr.mfh bit
        br.ret.sptk brp
        ;;

        LEAF_EXIT(KiRestoreHigherFPVolatile)

//
// ++
//
// Routine:
//
//       KiPageTableFault
//
// Description:
//
//       Branched from Inst/DataTlbVector
//       Inserts a missing PDE translation for VHPT mapping
//       If PageNotPresent-bit of PDE is not set, 
//                      branchs out to KiPdeNotPresentFault
//
// On entry:
//
//       rva  (h24) : offending virtual address
//       riha (h25) : a offending PTE address
//       rpr: (h26) : saved predicate        
//
// Handle:
//
//       Extracts the PDE index from riha (PTE address in VHPT) and 
//       generates a PDE address by adding to VHPT_DIRBASE. When accesses 
//       a page directory entry (PDE), there might be a TLB miss on the 
//       page directory table and returns a NaT on ld8.s. If so, branches 
//       to KiPageDirectoryTableFault. If the page-not-present bit of the 
//       PDE is not set, branches to KiPageNotPresentFault. Otherwise, 
//       inserts the PDE entry into the data TC (translation cache).
//
// Notes:
//
//       
// --
        
        HANDLER_ENTRY(KiPageTableFault)

        rva             = h24
        riha            = h25
        rpr             = h26
        rpPde           = h27
        rPde            = h28
        rPde2           = h29
        rps             = h30

        thash           rpPde = riha            // M
        mov             rps = PAGE_SHIFT << PS_SHIFT    // I
        ;;

        mov             cr.itir = rps           // M
        ld8.s           rPde = [rpPde]          // M, load PDE
        ;;
        
        tnat.nz         pt0, p0 = rPde          // I
        tbit.z          pt1, p0 = rPde, 0       // I, if non-present page fault         

(pt0)   br.cond.spnt    KiPageDirectoryFault    // B, tb miss on PDE access
(pt1)   br.cond.spnt    KiPdeNotPresentFault    // B, page fault 
        ;;

        mov             cr.ifa = riha           // M
        ;;
        itc.d           rPde                    // M
        ;;

#if !defined(NT_UP)
        ld8.s           rPde2 = [rpPde]         // M
        cmp.ne          pt0 = zero, zero        // I        
        ;;
            
        cmp.ne.or       pt0, p0 = rPde2, rPde   // I, if PTEs are different 
        tnat.nz.or      pt0, p0 = rPde2         // I

        ;;
(pt0)   ptc.l           riha, rps               // M, purge it
        
#endif                        
        mov             pr = rpr, -1            // I
        rfi;;                                   // B

        HANDLER_EXIT(KiPageTableFault)


                
//++
//
// KiPageDirectoryFault
//
// Cause:       
//              
// Parameters:  
//              rpPde (h28) : pointer to PDE entry
//              rpr   (h26) : saved predicate    
//
//
// Handle:     
//              
//--
        HANDLER_ENTRY(KiPageDirectoryFault)

        rva             = h24
        rpPpe           = h25
        rpr             = h26
        rpPde           = h27
        rPpe            = h28
        rPpe2           = h29
        rps             = h30
        
        thash           rpPpe = rpPde           // M
        ;;

        ld8.s           rPpe = [rpPpe]          // M        
        ;;

        tnat.nz         pt0, p0 = rPde          // I
        tbit.z          pt1, p0 = rPde, 0       // I, if non-present page fault 

(pt0)   br.cond.spnt    KiPageFault             // B
(pt1)   br.cond.spnt    KiPdeNotPresentFault    // B
        ;;

        mov             cr.ifa = rpPde          // M, set tva for vhpt translation
        ;;
        itc.d           rPde                    // M
        ;;

#if !defined(NT_UP)
        ld8.s           rPpe2 = [rpPpe]         // M
        mov             rps = PAGE_SHIFT << PS_SHIFT // I
        cmp.ne          pt0 = zero, zero        // I        
        ;;

        cmp.ne.or       pt0, p0 = rPpe2, rPpe   // I, if PTEs are different 
        tnat.nz.or      pt0, p0 = rPpe2         // I

        ;;
(pt0)   ptc.l           rpPde, rps              // M, purge it
        
#endif                        
        mov             pr = rpr, -1            // I
        rfi;;                                   // B


        HANDLER_EXIT(KiPageDirectoryFault)


//
// ++
//
// Routine:
//
//       KiPteNotPresentFault
//
// Description:
//
//       Branched from KiVhptTransVector and KiPageTableFault.
//       Inserts a missing PDE translation for VHPT mapping
//       If no PDE for it, branchs out to KiPageFault
//
// On entry:
//
//       rva  (h24)     : offending virtual address        
//       rpr  (h26)     : saved predicate
//       rPde (h28)     : PDE entry
//
// Handle:
//       
//       Check to see if PDE is marked as LARGE_PAGE. If so,
//       make it valid and install the large page size PTE.
//       If not, branch to KiPageFault.       
//
//      
// Notes:
//
//       PCR page mapped with TR
// --

        HANDLER_ENTRY(KiPteNotPresentFault)

        rva     =       h24    // passed                 
        riha    =       h25    // passed
        rpr     =       h26    // passed 
        rps     =       h27

        rPfn    =       h28
        rpAte   =       h28
        rAte    =       h29
        rAteEnd =       h30
        rAteBase =      h31
        rAteMask =      h22
        
        rK0Base =       h30
        rK2Base =       h31

        pIndr = pt1
        
        mov             rps  = PS_4K << PS_SHIFT // M  
        movl            rK0Base = KSEG0_BASE     // L
        ;;

        cmp.geu         pt3, p0 = rva, rK0Base  // M
        movl            rK2Base = KSEG2_BASE    // L
        ;;

(pt3)   cmp.ltu         pt3, p0 = rva, rK2Base    // M
        movl            rAteBase = ALT4KB_BASE    // L
        ;;

        shr.u           rPfn = rva, PAGE4K_SHIFT  // I
(pt3)   br.cond.spnt    KiKseg0Fault              // B
        ;;

        mov             rAteMask = ATE_MASK0      // I 
        
        shladd  rpAte = rPfn, PTE_SHIFT, rAteBase // M      
        movl    rAteEnd = ALT4KB_END              // L
        ;;

        ld8.s           rAte = [rpAte]            // M
        andcm           rAteMask = -1, rAteMask   // I
        cmp.ltu         pIndr = rpAte, rAteEnd    // I
        ;;
        tnat.z.and      pIndr = rAte              // I      
        tbit.nz.and     pIndr = rAte, PTE_VALID   // I

        or              rAteMask = rAte, rAteMask // M
        tbit.nz.and     pIndr = rAte, PTE_ACCESS  // I 
        tbit.nz.and     pIndr = rAte, ATE_INDIRECT // I


(pIndr) br.cond.spnt    KiPteIndirectFault        
        ;;
        ptc.l           rva, rps                // M

        br.spnt         KiPageFault             // B              

        HANDLER_EXIT(KiPteNotPresentFault)

//
// ++
//
// Routine:
//
//       KiPdeNotPresentFault
//
// Description:
//
//       Branched from KiVhptTransVector and KiPageTableFault.
//       Inserts a missing PDE translation for VHPT mapping
//       If no PDE for it, branchs out to KiPageFault
//
// On entry:
//
//       rva  (h24)     : offending virtual address        
//       rpr  (h26)     : saved predicate
//       rPde (h28)     : PDE entry
//
// Handle:
//       
//       Check to see if PDE is marked as LARGE_PAGE. If so,
//       make it valid and install the large page size PTE.
//       If not, branch to KiPageFault.       
//
//      
// Notes:
//
//       PCR page mapped with TR
// --

        HANDLER_ENTRY(KiPdeNotPresentFault)

        br.spnt         KiPageFault

#if 0
        rva             = h24
        rK0Base         = h25
        rpr             = h26
        rK2Base         = h27
        rPde            = h28
        ridtr           = h29
        rps             = h30
        rPte            = h25

        movl            rK0Base = KSEG0_BASE
        ;;

        cmp.ltu         pt3, pt4 = rva, rK0Base
        movl            rK2Base = KSEG2_BASE
        ;;
            
(pt4)   cmp.geu         pt3, p0 = rva, rK2Base
(pt3)   br.cond.spnt    KiPageFault
        
        mov             ridtr = cr.itir
        shr.u           rPde = rva, PAGE_SHIFT
        mov             rps = 24
        ;;

        movl            rPte = VALID_KERNEL_PTE
        dep.z           rPde = rPde, PAGE_SHIFT, 28 - PAGE_SHIFT
        dep             ridtr = rps, ridtr, PS_SHIFT, PS_LEN
        ;;

        mov             cr.itir = ridtr       
        or              rPde = rPte, rPde
        ;;

        itc.d           rPde
        ;;
        mov             pr = rpr, -1
        rfi;;
#endif

        HANDLER_EXIT(KiPdeNotPresentFault)


//
// ++
//
// Routine:
//
//       KiKseg0Fault
//
// Description:
//
//       TLB miss on KSEG0 space
//       
//
// On entry:
//
//       rva  (h24)     : faulting virtual address        
//       riha (h25)     : IHA address
//       rpr  (h26)     : saved predicate
//
// Process:
//       
//      
// Notes:
//
//       PCR page mapped with TR
// --

        HANDLER_ENTRY(KiKseg0Fault)

        rIPSR   =       h25      
        rISR    =       h30    
        rva     =       h24     // passed
        riha    =       h25     // passed
        rpr     =       h26     // passed
        rps     =       h27     
        rPte    =       h29
        rITIR   =       h28
        rPs     =       h30
        rPte0   =       h31

        mov     rISR = cr.isr                   // M
        shr.u   rva = rva, PAGE_SHIFT           // I
        cmp.ne  pt1 = r0, r0                    // I, pt1 = 0

        ;;
        ld8.s   rPte = [riha]                   // M
        dep.z   rva = rva, PAGE_SHIFT, 32       // I        
        tbit.z  pt2, pt3 = rISR, ISR_SP         // I

        ;;
        or      rPte0 = PTE_VALID_MASK, rPte    // M
        tbit.z.or   pt1 = rPte, PTE_LARGE_PAGE  // I
        tbit.nz.or  pt1 = rPte, PTE_VALID       // I

        mov     rIPSR = cr.ipsr                 // M
        tnat.nz pt4 = rPte                      // I
(pt1)   br.cond.spnt      KiPageFault           // B, invalid access

        extr    rPs = rPte, PTE_PS, PS_LEN      // I
(pt4)   br.cond.spnt      KiPageTableFault      // B, tb miss on PTE access
        ;;
        dep.z   rITIR = rPs, PS_SHIFT, PS_LEN   // I
        ;;
(pt2)   mov     cr.itir = rITIR                 // M
        dep     rIPSR = 1, rIPSR, PSR_ED, 1     // I
        ;;
(pt2)   itc.d   rPte0                           // M
        mov     rps = PAGE_SHIFT << PS_SHIFT    // I
        ;;
(pt3)   ptc.l   rva, rps                        // M
(pt3)   mov     cr.ipsr = rIPSR                 // M        
        ;;
        mov     pr = rpr, -1                    // I
        rfi                                     // B
        ;;

        HANDLER_EXIT(KiKseg0Fault)

//
// ++
//
// Routine:
//
//       KiKseg3Fault
//
// Description:
//
//       TLB miss on KSEG3 space
//       
//
// On entry:
//
//       rva  (h24)     : faulting virtual address        
//       riha (h25)     : IHA address
//       rpr  (h26)     : saved predicate
//
// Process:
//       
//      
// Notes:
//
//       PCR page mapped with TR
// --

        HANDLER_ENTRY(KiKseg3Fault)

        rIPSR   =       h22      
        rISR    =       h23    
        rva     =       h24     // passed
        riha    =       h25
        rpr     =       h26     // passed
        rPte    =       h27

        mov     rISR = cr.isr                   // M
        movl    rPte = VALID_KERNEL_PTE         // L

        mov     rIPSR = cr.ipsr                 // M
        shr.u   rva = rva, PAGE_SHIFT           // I
        ;;
        tbit.z  pt2, pt3 = rISR, ISR_SP         // I
        dep.z   rva = rva, PAGE_SHIFT, 32       // I
        ;;
        or      rPte = rPte, rva                // I
        dep     rIPSR = 1, rIPSR, PSR_ED, 1     // I
        ;;

(pt2)   itc.d   rPte                            // M
        ;;
(pt3)   ptc.l   rva, rps                        // M
(pt3)   mov     cr.ipsr = rIPSR                 // M        
        ;;

        mov     pr = rpr, -1                    // I
        rfi                                     // B
        ;;

        HANDLER_EXIT(KiKseg3Fault)


//
// ++
//
// Routine:
//
//       KiPageFault
//
// Description:
//
//       This must be a genuine page fault. Call KiMemoryFault().
//       
//
// On entry:
//
//       rva (h24)     : offending virtual address
//       rpr (h26)     : PDE contents 
//
// Process:
//       
//       Restores the save predicate (pr), and branches to 
//       KiGenericExceptionHandler with the argument KiMemoryFault with
//       macro VECTOR_CALL_HANDLER().       
//      
// Notes:
//
//       PCR page mapped with TR
// --

        ALTERNATE_ENTRY(KiPageFault2)
        
        rRR0 = h21

        //
        // enable the VHPT again
        //

        mov            rr[r0] = rRR0            // M, enable VHPT again     

        HANDLER_ENTRY(KiPageFault)

        rva     = h24
        rpr     = h26
        rIPSR   = h27
        rISR    = h31

        //
        // check to see if non-present fault occurred on a speculative load.
        // if so, set IPSR.ed bit. This forces to generate a NaT on ld.s after
        // rfi
        //

        mov             rISR = cr.isr           // M
        mov             rIPSR = cr.ipsr         // M
        ;;

        tbit.z          pt0, p0 = rISR, ISR_SP  // I
        dep             rIPSR = 1, rIPSR, PSR_ED, 1 // I

(pt0)   br.cond.spnt    KiCallMemoryFault          // B     
        ;;

        mov             cr.ipsr = rIPSR         // M
        ;;
        mov             pr = rpr, -1            // I
        rfi                                     // B
        ;;

KiCallMemoryFault:

        mov             pr = rpr, -1            // I
        
        VECTOR_CALL_HANDLER(KiGenericExceptionHandler, KiMemoryFault)

        HANDLER_EXIT(KiPageFault)

//
// ++
//
// Routine:
//
//       KiPteIndirectFault
//
// Description:
//
//       The PTE itself indicates a PteIndirect fault. The target PTE address  
//       should be generated by extracting PteOffset from PTE and adding it to 
//       PTE_UBASE.  The owner field of the target PTE must be 1. Otherwise, 
//       Call MmX86Fault().        
//      
// On entry:
//
//       rva (h24)     : offending virtual address
//       rpr (h26)     : PDE contents 
//
// Process:
//       
//       Restores the save predicate (pr), and branches to 
//       KiGenericExceptionHandler with the argument KiMemoryFault with
//       macro VECTOR_CALL_HANDLER().       
//      
// Notes:
//
//       PCR page mapped with TR
// --

        HANDLER_ENTRY(KiPteIndirectFault)

        rpr     =       h26  // passed
        rps     =       h27  // passed
        rpAte   =       h28  // passed
        rAte    =       h29  // passed
        rPte    =       h30
        rPte0   =       h31
        rAteMask =      h22  // passed
        rVa12   =       h23

        rPteOffset = h24
        rpNewPte = h21

        rOldIIP =       h17  // preserved
        rIA32IIP =      h18 
        rpVa    =       h19  // preserved
        rIndex  =       h20  // preserved
        rpBuffer=       h16
        rpPte   =       h21

        pBr = pt0
        pPrg = pt3
        pLoop = pt4
        pClear = pt5

        mov             cr.itir = rps                           // M
        movl            rpNewPte = PTE_BASE                     // L

        mov             rIA32IIP = cr.iip                       // M 
        extr.u          rPteOffset = rAte, PAGE4K_SHIFT, 32     // I

        ;;
        add             rpNewPte = rPteOffset, rpNewPte         // M/I
        cmp.eq          pLoop, pClear = rIA32IIP, rOldIIP       // I                        

        ;;
        ld8.s           rPte = [rpNewPte]                       // M
        shr             rVa12 = rpAte, PTE_SHIFT                // I
        ;;
        tnat.nz.or      pBr = rPte                              // I
        tbit.z.or       pBr = rPte, PTE_VALID                   // I

        ;;
(pClear)mov           rIndex = 0                               // M        
        and              rPte0 = rPte, rAteMask                // I
        ;;

        //
        // deposit extra PFN bits for 4k page
        //
        dep     rPte0 = rVa12, rPte0, PAGE4K_SHIFT, PAGE_SHIFT-PAGE4K_SHIFT // I
        ;;
(pClear)itc.d   rPte0                                           // M
        movl         rpBuffer = KiPcr + PcForwardProgressBuffer   // L    
        ;;

        shladd       rpVa = rIndex, 4, rpBuffer  // M
        add          rIndex = 1, rIndex          // I
        ;;

        st8          [rpVa] = rva                // M     
        add          rpPte = 8, rpVa             // I
        and          rIndex = 7, rIndex          // I
        ;;

        st8          [rpPte] = rPte0             // M
        mf                                       // M

        mov            rOldIIP = rIA32IIP        // I

        
#if !defined(NT_UP)
        rAte2   =       h28
        rPte2   =       h31

        ld8.s   rPte2 = [rpNewPte]              // M
        ld8.s   rAte2 = [rpAte]                 // M
        cmp.ne  pPrg = zero, zero               // I
        ;;

        cmp.ne.or       pPrg = rPte, rPte2      // I
        tnat.nz.or      pPrg = rPte2            // I

        cmp.ne.or       pPrg = rAte, rAte2      // I
        tnat.nz.or      pPrg = rAte2            // I
        ;;

(pPrg)  ptc.l           rva, rps                // M
(pPrg)  st8             [rpPte] = r0            // M

#endif          
(pLoop) br.cond.spnt    KiFillForwardProgressTb // B
        mov             pr = rpr, -1            // I
        rfi;;                                   // B
            
        HANDLER_EXIT(KiPteIndirectFault)

//
// ++
//
// Routine:
//
//       Ki4KDataTlbFault
//
// Description:
//
//       Branched from KiDataTlbVector if PTE.Cache indicates the reserved 
//       encoding. Reads the corresponding ATE and creates a 4kb TB on the 
//       fly inserts it to the TLB. If a looping condition at IIP is 
//       detected, it branches to KiFillForwardProgressTb and insert the TBs 
//       from the forward progress TB queue.
//      
// On entry:
//
//       rva (h24)     : offending virtual address
//       riha(h25)     : IHA address
//       rpr (h26)     : PDE contents 
//
// Notes:
//
// --

        HANDLER_ENTRY(Ki4KDataTlbFault)

        rva     =       h24  // passed
        riha    =       h25  // passed
        rpr     =       h26  // passed
        rps     =       h27  
        rPfn    =       h28
        rpAte   =       h28
        rAte    =       h29
        rPte    =       h30
        rAltBase =      h31
        rPte0   =       h31
        rAteMask =      h22
        rVa12   =       h23

        rOldIIP =       h17 // preserved
        rIA32IIP =      h18
        rpVa    =       h19
        rIndex  =       h20 // preserved

        rpBuffer=       h16
        rpPte   =       h21

        pBr = pt0
        pIndr = pt1
        pMiss = pt2
        pPrg = pt3
        pLoop = pt4
        pClear = pt5
        pMiss2 = pt6

        mov     rIA32IIP = cr.iip               // M 
        mov     rps = PS_4K << PS_SHIFT         // I
        shr.u   rPfn = rva, PAGE4K_SHIFT        // I

        cmp.ne  pBr = zero, zero                // M/I, initialize to 0
        movl    rAltBase = ALT4KB_BASE          // L
        ;;

        ld8.s   rPte = [riha]                   // M
        shladd  rpAte = rPfn, PTE_SHIFT, rAltBase  // I
        ;;

        ld8.s   rAte = [rpAte]                  // M
        movl     rAteMask = ATE_MASK            // L        
        ;;

        cmp.eq         pLoop, pClear = rIA32IIP, rOldIIP// M  
        tnat.nz        pMiss = rPte             // I
(pMiss) br.cond.spnt   KiPageTableFault         // B

        tnat.nz        pMiss2 = rAte            // I
(pMiss2)br.cond.spnt   KiAltTableFault          // B

        tbit.z.or      pBr = rPte, PTE_VALID    // I
        tbit.z.or      pBr = rAte, PTE_VALID    // I
        tbit.z.or      pBr = rAte, PTE_ACCESS   // I

        or             rAteMask = rAte, rAteMask // M  
        tbit.nz        pIndr, p0 = rAte, ATE_INDIRECT // I
(pBr)   br.cond.spnt   KiPageFault              // B

        add            rPte0 = -4, rPte         // M, make it WB
        shr            rVa12 = rpAte, PTE_SHIFT // I   
(pIndr) br.cond.spnt   KiPteIndirectFault       // B  
        ;;
(pClear)mov           rIndex = 0                // M        
        mov           cr.itir = rps             // M
        and           rPte0 = rPte0, rAteMask   // I      
        ;;
        //
        // deposit extra PFN bits for 4k page
        //

        dep           rPte0 = rVa12, rPte0, PAGE4K_SHIFT, PAGE_SHIFT-PAGE4K_SHIFT // I
        ;;

(pClear)itc.d         rPte0                     // M, install PTE 

        movl         rpBuffer = KiPcr + PcForwardProgressBuffer   // L    
        ;;

        shladd       rpVa = rIndex, 4, rpBuffer  // M
        add          rIndex = 1, rIndex          // I
        ;;

        st8          [rpVa] = rva                // M     
        add          rpPte = 8, rpVa             // I
        and          rIndex = 7, rIndex          // I
        ;;

        st8          [rpPte] = rPte0             // M
        mf                                       // M

        mov            rOldIIP = rIA32IIP        // I

#if !defined(NT_UP)
        rps     =       h27
        rAte2   =       h28
        rPte2   =       h31

        ld8.s   rPte2 = [riha]                  // M
        ld8.s   rAte2 = [rpAte]                 // M
        cmp.ne  pPrg = zero, zero               // I
        ;;

        cmp.ne.or       pPrg = rPte, rPte2      // M
        tnat.nz.or      pPrg = rPte2            // I

        cmp.ne.or       pPrg = rAte, rAte2      // M
        tnat.nz.or      pPrg = rAte2            // I        
        ;;

(pPrg)  ptc.l           rva, rps                // M
(pPrg)  st8             [rpPte] = r0            // M
#endif          
(pLoop) br.cond.spnt    KiFillForwardProgressTb // B
  
        mov             pr = rpr, -1            // I
        rfi;;                                   // B
            
        HANDLER_EXIT(Ki4KDataTlbFault)
        
//
// ++
//
// Routine:
//
//       Ki4KInstTlbFault
//
// Description:
//
//       Branched from KiInstTlbVector if PTE.Cache indicates the reserved 
//       encoding. Reads the corresponding ATE and creates a 4kb TB on the 
//       fly inserts it to the TLB. 
//      
// On entry:
//
//       rva (h24)     : offending virtual address
//       riha(h25)     : IHA address
//       rpr (h26)     : PDE contents 
//
// Notes:
//
// --

        HANDLER_ENTRY(Ki4KInstTlbFault)

        rva     =       h24  // passed
        riha    =       h25  // passed
        rpr     =       h26  // passed
        rps     =       h27  
        rPfn    =       h28
        rpAte   =       h28
        rAte    =       h29
        rPte    =       h30
        rAltBase =      h31
        rPte0   =       h31
        rAteMask =      h22
        rVa12   =       h23

        pBr = pt0
        pIndr = pt1
        pMiss = pt2
        pPrg = pt3
        pMiss2 = pt6


        mov     rps = PS_4K << PS_SHIFT         // M
        movl    rAltBase = ALT4KB_BASE          // L
        shr.u   rPfn = rva, PAGE4K_SHIFT        // I
        ;;

        ld8.s   rPte = [riha]                   // M
        cmp.ne  pBr = zero, zero                // M/I, initialize to 0
        shladd  rpAte = rPfn, PTE_SHIFT, rAltBase  // I
        ;;

        ld8.s   rAte = [rpAte]                  // M
        movl     rAteMask = ATE_MASK            // L        
        ;;

        tnat.nz        pMiss = rPte             // I
(pMiss) br.cond.spnt   KiPageTableFault         // B

        tnat.nz        pMiss2 = rAte            // I
(pMiss2)br.cond.spnt   KiAltTableFault          // B

        tbit.z.or      pBr = rPte, PTE_VALID    // I
        tbit.z.or      pBr = rAte, PTE_VALID    // I
        tbit.z.or      pBr = rAte, PTE_ACCESS   // I

        or             rAteMask = rAte, rAteMask // M  
        tbit.nz        pIndr, p0 = rAte, ATE_INDIRECT // I
(pBr)   br.cond.spnt   KiPageFault              // B

        add            rPte0 = -4, rPte         // M, make it WB
        shr            rVa12 = rpAte, PTE_SHIFT // I   
(pIndr) br.cond.spnt   KiPteIndirectFault       // B  
        ;;
        mov     cr.itir = rps                   // M
        and            rPte0 = rPte0, rAteMask  // I      
        ;;
        //
        // deposit extra PFN bits for 4k page
        //

        dep          rPte0 = rVa12, rPte0, PAGE4K_SHIFT, PAGE_SHIFT-PAGE4K_SHIFT // I
        ;;

        itc.i        rPte0                     // M, install PTE 
        ;;

#if !defined(NT_UP)
        rps     =       h27
        rAte2   =       h28
        rPte2   =       h31

        ld8.s   rPte2 = [riha]                  // M
        ld8.s   rAte2 = [rpAte]                 // M
        cmp.ne  pPrg = zero, zero               // I
        ;;

        cmp.ne.or       pPrg = rPte, rPte2      // M
        tnat.nz.or      pPrg = rPte2            // I

        cmp.ne.or       pPrg = rAte, rAte2      // M
        tnat.nz.or      pPrg = rAte2            // I        
        ;;

(pPrg)  ptc.l           rva, rps                // M

#endif          
  
        mov             pr = rpr, -1            // I
        rfi;;                                   // B
            
        HANDLER_EXIT(Ki4KInstTlbFault)
        
//
// ++
//
// Routine:
//
//       KiAltTableFault
//
// Description:
//
//       Branched from Inst/DataAccessBitVector
//       Inserts a missing PTE translation for the alt table. 
//
// On entry:
//
//       rva  (h24) : offending virtual address
//       riha (h25) : a offending PTE address
//       rpr: (h26) : saved predicate        
//
// Handle:
//
// --
        
        HANDLER_ENTRY(KiAltTableFault)

        rpAte   = h28   // passed

        rva     = h24 
        riha    = h25
        rpr     = h26   // passed
        rPte    = h27
        rPte2   = h28
        rps     = h29

        thash           riha = rpAte            // M
        mov             rva  = rpAte            // I
        ;;

        ld8.s           rPte = [riha]           // M
        ;;
        
        tnat.nz         pt0, p0 = rPte          // I
        tbit.z          pt1, p0 = rPte, 0       // I        

(pt0)   br.cond.spnt    KiPageTableFault        // B
(pt1)   br.cond.spnt    KiPteNotPresentFault    // B
        ;;

        mov             cr.ifa = rva
        ;;
        itc.d           rPte                    // M
        ;;

#if !defined(NT_UP)
        ld8.s           rPte2 = [riha]          // M
        mov             rps = PAGE_SHIFT << PS_SHIFT // I
        cmp.ne          pt0 = zero, zero             // I   
        ;;

        cmp.ne.or       pt0 = rPte2, rPte       // M
        tnat.nz.or      pt0 = rPte2             // I
        ;;

(pt0)   ptc.l           rva, rps                // M
#endif
        mov             pr = rpr, -1            // I
        rfi;;                                   // B

        HANDLER_EXIT(KiAltTableFault)


//
// ++
//
// Routine:
//
//       KiFillForwardProgressTb
//
// Description:
//
//       Fill TB from TLB forward progress buffer.
//
// On entry:
//
//       rpBuffer (h16) : forward progress buffer address
//       rpr: (h26) : saved predicate        
//
// Handle:
//
// --
        
        HANDLER_ENTRY(KiFillForwardProgressTb)

        rLc     =       h29
        rT0     =       h28
        rps     =       h27
        rpr     =       h26
        rVa     =       h22
        rPte    =       h21
        rpVa    =       h19
        rpPte   =       h17
        rpBuffer=       h16

        mov     rpVa = rpBuffer                         // A
        mov.i   rLc = ar.lc                             // I 
        mov     rT0 = NUMBER_OF_FWP_ENTRIES - 1         // 
        ;;
        add     rpPte = 8, rpBuffer                     // A
        mov.i   ar.lc = rT0                             // I
        ;;

fpb_loop:        

        //
        // use ALAT to see if somebody modify the PTE entry
        //

        ld8     rVa = [rpVa], 16                        // M 
        ld8.a   rPte = [rpPte]                          // M
        ;;
        
        mov     cr.ifa = rVa                            // M
        cmp.ne  pt0, pt1 = rPte, r0                     // I
        ;;

(pt0)   itc.d   rPte                                    // M
        ;;
(pt0)   ld8.c.clr rPte = [rpPte]                        // M
        add     rpPte = 16, rpPte                       // I 
        ;;
(pt1)   invala.e rPte                                   // M, invalidate ALAT entry
(pt0)   cmp.eq.and pt0 = rPte, r0                       // I
        ;;
(pt0)   ptc.l   rVa, rps                                // M
        br.cloop.dptk.many fpb_loop;;                   // B

        mov.i  ar.lc = rLc 

        mov     pr = rpr, -1                            // I
        rfi                                             // B
        ;;

        HANDLER_EXIT(KiFillForwardProgressTb)
                

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
//      a0 - pointer to trap frame
//      a1 - previous mode
//
// Return Value:
//
//      None.
//
//--

        NESTED_ENTRY(KiExceptionDispatch)

//
// Build exception frame
//

        .regstk   2, 3, 5, 0
        .prologue 0xA, loc0
        alloc     t16 = ar.pfs, 2, 3, 5, 0
        mov       loc0 = sp
        cmp4.eq   pt0 = UserMode, a1                  // previous mode is user?

        mov       loc1 = brp 
        add       sp = -ExceptionFrameLength, sp
        ;;

        .save     ar.unat, loc2
        mov       loc2 = ar.unat
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
 (pt0)  add       out0 = TrapFrameLength+TsHigherFPVolatile, a0
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
 (pt0)  br.call.sptk brp = KiSaveHigherFPVolatile
        ;;


        PROLOGUE_END

        add       out0 = TrExceptionRecord, a0        // -> exception record
        add       out1 = STACK_SCRATCH_AREA, sp       // -> exception frame
        mov       out2 = a0                           // -> trap frame

        mov       out3 = a1                           // previous mode
        mov       out4 = 1                            // first chance
        br.call.sptk.many brp = KiDispatchException

        add       t1 = ExApEC+STACK_SCRATCH_AREA, sp
        movl      t0 = KiExceptionExit
        ;;

        ld8       t1 = [t1]
        mov       brp = t0
        ;;

        mov       ar.unat = loc2
        mov       ar.pfs = t1

        add       s1 = STACK_SCRATCH_AREA, sp         // s1 -> exception frame
        mov       s0 = a0                             // s0 -> trap frame
        br.ret.sptk brp
        ;;

        ALTERNATE_ENTRY(KiExceptionExit)

//++
//
// Routine Description:
//
//     This routine is called to exit from an exception.
//
//     N.B. This transfer of control occurs from:
//
//         1. fall-through from above
//         2. exit from continue system service
//         3. exit from raise exception system service
//         4. exit into user mode from thread startup
//
// Arguments:
//
//     loc0 - pointer to trap frame
//     sp - pointer to high preserved float save area + STACK_SCRATCH_AREA
//
// Return Value:
//
//      Does not return.
//
//--

//
// upon entry of this block, s0 and s1 must be set to the address of
// the trap and the exception frames respectively.
//
// preserved state is restored here because they may have been modified
// by SetContext
//

     
        LEAF_SETUP(0, 1, 2, 0)                        // must be in sync with
                                                  // KiGenericExceptionHandler
        mov       loc0 = s0                       // -> trap frame
        mov       out0 = s1                       // -> exception frame
        ;;

        br.call.sptk brp = KiRestoreExceptionFrame
        ;;

        mov       sp = loc0                       // deallocate exception
                                                  // frame by restoring sp

        ALTERNATE_ENTRY(KiAlternateExit)

//
// sp -> trap frame addres
//
// Interrupts disabled from here to rfi
//

        FAST_DISABLE_INTERRUPTS
        ;;

        RETURN_FROM_INTERRUPTION(Ked)

        NESTED_EXIT(KiExceptionDispatch)


//++
//
// BOOLEAN
// KeInvalidAccessAllowed (
//    IN PVOID TrapInformation
//    )
//
// Routine Description:
//
//    Mm will pass a pointer to a trap frame prior to issuing a bug check on
//    a pagefault.  This routine lets Mm know if it is ok to bugcheck.  The
//    specific case we must protect are the interlocked pop sequences which can
//    blindly access memory that may have been freed and/or reused prior to the
//    access.  We don't want to bugcheck the system in these cases, so we check
//    the instruction pointer here.
//
// Arguments:
//
//    TrapFrame (a0) - Supplies a  trap frame pointer.  NULL means return False.
//
// Return Value:
//
//    True if the invalid access should be ignored.
//    False which will usually trigger a bugcheck.
//
//--

        LEAF_ENTRY(KeInvalidAccessAllowed)

        .regstk    1, 0, 0, 0

        cmp.eq     pt0 = 0, a0
        movl       t1 = ExpInterlockedPopEntrySListFault

        add        t0 = TrStIIP, a0
        mov        v0 = zero              // assume access not allowed
 (pt0)  br.ret.spnt brp
        ;;

        ld8        t0 = [t0]
        ;;
        cmp.eq     pt2 = t0, t1
        ;;

        nop.m      0
 (pt2)  mov        v0 = 1
        br.ret.sptk brp

        LEAF_EXIT(KeInvalidAccessAllowed)

        LEAF_ENTRY(LogInterruptionEvent)

        mov       h28 = cr.iip
        movl      h25 = KiPcr+PcInterruptionCount
        ;;

        mov       h29 = cr.ipsr
        ld4.nt1   h26 = [h25]
        mov       h24 = MAX_NUMBER_OF_IHISTORY_RECORDS - 1
        ;;

        add       h27 = 1, h26
        and       h26 = h24, h26
        add       h24 = 0x1000-PcInterruptionCount, h25
        ;;

        st4.nta   [h25] = h27
        shl       h26 = h26, 5
        ;;
        add       h27 = h26, h24
        ;;

        st8       [h27] = h30, 8
        ;;
        st8       [h27] = h28, 8
        shl       h30 = h30, 8
        ;;

        st8       [h27] = h29, 8
        movl      h28 = KiIvtBase+0x10
        ;;

        st8       [h27] = h31;
        add       h29 = h28, h30
        mov       h30 = b0
        ;;

        mov       b0 = h29
        br        b0
        ;;

        LEAF_EXIT(LogInterruptionEvent)



