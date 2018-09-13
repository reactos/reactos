/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

//      TITLE("Context Conversion")
//++
//                                                        
// Module Name:
//
//    convctx.s
//
// Abstract:
//
//    This module implements the code necessary to convert or assign 
//    the user to kernel transition context to or from iA mode context.
//
// Author:
//
//    Edward G. Chron (echron) 01-Apr-1996
//
// Environment:
//
//    Kernel mode only.
//
// Revision History:
//
//
//--

#include "ksia64.h"

        .file       "convctx.s"
         
//++
//
// VOID
// RtlEmFpToIaFpContext (
//      IN     PCONTEXT  ContextEM,
//      IN OUT UCHAR    *iAFpArea   
//    )
//
// Routine Description:
//
//      This function takes the values stored for the EM FP registers which
//      map the iA mode FP registers and copies them (in iA mode 10 byte
//      format) to the targeted 80 byte iA mode FP register context area.
//
// Arguments:
//
//      ContextEM (a0) - Pointer to the EM mode Context which contain the FP registers.
//      iAFpArea(a1)   - Pointer to area to store 8 * 10 byte iA mode FP regs. 
//
// Return Value:
//
//    None.
//
// N.B. The format iA mode FP registers is 80 bits and will not change.
//
//--

        LEAF_ENTRY(RtlEmFpToIaFpContext)


        ARGPTR(a1)                          // iA Fp Save Area Pointer + 8
        ARGPTR(a0)                          // Addr of first EM Context Fp 
                                            // Reg mapping first iA mode FP Reg
         
        add         t5 = -8, r0             // Need to convert 8 EM mode FP Regs
        ;;
         
cvtEmFp:         

        ldfe        ft0 = [a0], 16          // Get EM FP reg mapping iA FP reg
        add         t4 = -4, r0             // Need to store 2 bytes 4 times
        ;;
         
        getf.sig    t2 = ft0                // floating-point reg significand
        getf.exp    t3 = ft0                // floating-point sign & exponent
        ;;
         
stIaFpReg:

        st2         [a1] = t2, 2            // save 2 bytes of significand 
        extr.u      t2 = t2, 16, 48         // shift to the next two bytes
        adds        t4 = 0x1, t4            // bump loop count
        ;;

        cmp.lt      p6, p0 = t4, r0         // done?
(p6)    br.cond.dpnt.few stIaFpReg          // no, store the next two bytes
         
        adds        t5 = 0x1, t5            // bump loop count
        add         t4 = 0, t3              // copy of exponent and sign
        ;;
        extr.u      t4 = t4, 2, 62          // get the sign in bit 15
        ;;

        cmp.lt      p6, p7 = t5, r0         // done?
        dep         t3 = t3, t4, 0, 15      // just first 16 bits of exponent
        ;;

        st2         [a1] = t3, 2            // save sign and iA exponent
(p6)    br.cond.dpnt.few cvtEmFp            // no, process the next register
(p7)    br.ret.sptk brp
        ;;

        LEAF_EXIT(RtlEmFpToIaFpContext)    

//++
//
// VOID
// RtlIaFpToEmFpContext (
//      IN  UCHAR        *iAFpArea,   
//      IN  OUT PCONTEXT  ContextEM,
//      IN  OUT FLOAT128 *FpWorkArea  
//    )
//
// Routine Description:
//
//      This function takes the values stored for the EM FP registers which
//      map the iA mode FP registers and copies them (in iA mode 10 byte
//      format) to the targeted 80 byte iA mode FP register context area.
//
// Arguments:
//
//      ContextEM (a0) - Pointer to the EM mode Context registers that map iA mode FP regs.
//      iAFpArea(a1)   - Pointer to area to store 8 * 10 byte iA mode FP regs. 
//      alignedFp(a2)  - FLOAT128 work area (aligned on 16 byte boundry).
//
// Return Value:
//
//    None.
//
// N.B. The format iA mode FP registers is 80 bits and will not change.
//
//--

        LEAF_ENTRY(RtlIaFpToEmFpContext)
         
        ARGPTR(a0)
        ARGPTR(a1)                          // Addr of first EM Context Fp Reg 
        ARGPTR(a2)                          // Addr of first EM Context Fp Reg 
        add         t0 = 8, a0              // iA Fp Save Area Pointer
        add         t5 = -8, r0             // Need to convert 8 iA mode FP Regs
        ;;
         
cvtIaFp:         
        add         t8 = -4, r0             // Need to load 2 bytes 4 times
        ld2         t3 = [t0], -2           // load exponent and sign
        ;;
        add         t4 = 0, t3              // copy of exponent and sign
        ;;

        add         t2 = 0, r0
        extr.u      t4 = t4, 15, 49         // get the sign in bit 17
        ;;
        dep         t3 = t4, t3, 17, 1      // just first 16 bits of exponent
         

ldIaFpReg:
        ld2         t6 = [t0], -2           // load 2 bytes of significand 
        adds        t8 = 0x1, t8            // bump loop count
        ;;
        or          t2 = t2, t6             // retain until complete

        cmp.lt      p6, p0 = t8, r0         // done?
        ;;
(p6)    dep.z       t2 = t2, 16, 48         //   no, shift to the next two bytes
(p6)    br.cond.dpnt.few ldIaFpReg          //   no, get next two bytes
        ;;
         
        add         t8 = -4, r0             // Need to store 2 bytes 4 times
        mov         t7 = a2                 // get address of aligned save area
        ;;

alignIaFp:
        st2         [t7] = t2, 2            // save 2 bytes of significand 
        extr.u      t2 = t2, 16, 48         // shift to the next two bytes
        adds        t8 = 0x1, t8            // bump loop count
        ;;

        cmp.lt      p6, p0 = t8, r0         // done?
        add         t6 = 0, t3              // copy of exponent and sign
(p6)    br.cond.dpnt.few alignIaFp          //   maybe
        ;;

        nop.m       0
        extr.u      t6 = t6, 2, 62          // get the sign in bit 15
        ;;
        dep         t3 = t3, t6, 0, 15      // just first 15 bits of exponent
        ;;

        st2         [t7] = t3, 2            // save sign and iA exponent
        ldfe        ft0 = [a2]
        adds        t5 = 0x1, t5            // bump loop count
        ;;

        stfe        [a1] = ft0, CxFltT3 - CxFltT2   // Store value into EM FP Context
        cmp.lt      p6, p7 = t5, r0         // done?
        ;;
(p6)    add         t2 = 0, r0              //   no, reset significand for next

(p6)    add         t0 = 20, t0             //   no, starting at the high end
(p6)    br.cond.dpnt.few cvtIaFp            //   no, get next context record
(p7)    br.ret.sptk brp
        ;;

        LEAF_EXIT(RtlIaFpToEmFpContext)    

