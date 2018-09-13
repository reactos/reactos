/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ia64mach.c

Abstract:

    This file contains the IA64 specific code for dealing with
    the process of stepping a single instruction.  This includes
    determination of the next offset to be stopped at and if the
    instruction is all call type instruction.

Author:

    Ho Chen (hc)
    Vadim Paretsky - major mods

Environment:

    Win32 - User

Notes:

--*/

#include "precomp.h"
#pragma hdrstop



extern CRITICAL_SECTION csContinueQueue;

/**********************************************************************/

extern LPDM_MSG LpDmMsg;

/**********************************************************************/
/*
    //setup a couple of macros

    // The below macro is used to do subscripting operations
    // len_item is the length of the embedded word that we are
    // interested in subscripting

#define NTH_BIT(word,n,len_item) \
    ( ((word) >> ((len_item) - (n) - 1)) & 0x01)


*/
//
// Stuff for EM debug registers

#define RWE_WRITE       (DWORDLONG)0x4000000000000000
#define RWE_READ        (DWORDLONG)0x8000000000000000
#define RWE_EXEC        (DWORDLONG)0x8000000000000000
#define RWE_MASK        (DWORDLONG)0xf000000000000000

#define PLM_0           (DWORDLONG)0x0100000000000000
#define PLM_1           (DWORDLONG)0x0200000000000000
#define PLM_2           (DWORDLONG)0x0400000000000000
#define PLM_3           (DWORDLONG)0x0800000000000000
#define PLM_MASK        (DWORDLONG)0x0f00000000000000


DWORDLONG       LenMask[ MAX_DEBUG_REG_DATA_SIZE + 1 ] = DEBUG_REG_LENGTH_MASKS;



#define WIN_CDECL    __cdecl
#define DECEM    1    /* GetNextOffset() based on Intel Falcon decoder DLL */

#include "EMTOOLS\decem.h"

BOOL RequestEMToDecode(HTHDX hthd, LPADDR lpAddr, EM_Decoder_Info* pInfo)
{
#if 0
    IA64_INSTRUCTION disInstr;
    ADDR BundleAddr = *lpAddr;
    DWORD InstLen;
    BYTE dwSlot = (BYTE)((GetAddrOff(*lpAddr) & 0xF) >> 2);
    static EM_Decoder_Info Info[3];
    static UOFFSET LastOffset = 0;
    
    GetAddrOff(BundleAddr) &= ~0xF;

    if (!AddrReadMemory( hthd->hprc, hthd, &BundleAddr, &disInstr, BUNDLE_SIZE, &InstLen)) {
        DPRINT(0,("RequestEMToDecode: AddrReadMemory @ 0x%I64x failed\n",GetAddrOff(BundleAddr)));
        return FALSE;
    }

    if (InstLen != BUNDLE_SIZE ) {
        DPRINT(0,("RequestEMToDecode: ReadMemory @ 0x%I64x failed\n", GetAddrOff(BundleAddr)));
        return FALSE;
    }
    // we check to see if the bundle out instruction belongs to has just been disaassembled by the EM 
    // (it disassembles by bundle, not by instruction; if so we just pick up the needed slot,if not
    // we ask it to disassemble the bundle first
    if (dwSlot == 0 && GetAddrOff(BundleAddr) != LastOffset) {
        if (DMSendRequestReply (dbceDecode, hthd->hprc->hpid, hthd->htid, BUNDLE_SIZE, &disInstr, sizeof(Info), &Info) != xosdNone) {
            DPRINT(0,("RequestEMToDecode: Decode @ 0x%I64x failed \n", GetAddrOff(BundleAddr)));
            return FALSE;
        }
        LastOffset = GetAddrOff(BundleAddr);
    }
    
    *pInfo = Info[dwSlot];
    return TRUE;
#else 
    ASSERT(!"EMDecode not supported");
    return FALSE;
}

typedef struct
        {
            ULONGLONG Slot[3];
            ULONGLONG Template;
        } IA64_BUNDLE;

enum op_type { BAD_OP, I_OP, M_OP, F_OP, B_OP };
enum op_type OpType[64] = { M_OP,   I_OP,   I_OP,   BAD_OP,
                            M_OP,   I_OP,   I_OP,   BAD_OP,
                            M_OP,   I_OP,   BAD_OP, BAD_OP,
                            BAD_OP, BAD_OP, BAD_OP, BAD_OP,
                            M_OP,   M_OP,   I_OP,   BAD_OP,
                            M_OP,   M_OP,   I_OP,   BAD_OP,
                            M_OP,   F_OP,   I_OP,   BAD_OP,
                            M_OP,   M_OP,   F_OP,   BAD_OP,
                            M_OP,   I_OP,   B_OP,   BAD_OP,
                            M_OP,   B_OP,   B_OP,   BAD_OP,
                            BAD_OP, BAD_OP, BAD_OP, BAD_OP,
                            B_OP,   B_OP,   B_OP,   BAD_OP,
                            M_OP,   M_OP,   B_OP,   BAD_OP,
                            BAD_OP, BAD_OP, BAD_OP, BAD_OP,
                            M_OP,   F_OP,   B_OP,   BAD_OP,
                            BAD_OP, BAD_OP, BAD_OP, BAD_OP};

#define OP_TYPE(Template,Slot) OpType[(Template << 2) | Slot]

#define BITS(BitField,iHigh,iLow) (ULONGLONG)(((ULONGLONG)BitField << (63 - iHigh)) >> (63-iHigh + iLow))

#define MAJOR_OPCODE(op) BITS(op,40,37)

BOOL GetInstAndTemplate(HTHDX hthd, LPADDR lpAddr, PULONGLONG pInst, PULONGLONG pTemplate)
{
    ADDR addr = *lpAddr;
    DWORD InstLen;
    BOOL bSlot1;
    
    if(!AddrReadMemory(hthd->hprc, hthd, &addr, pInst, sizeof(ULONGLONG), &InstLen)) {
        return FALSE;
    }
    
    bSlot1 = FALSE;
    switch(GetAddrOff(addr) & 0xF) {
        case 0: *pInst = ((*pInst) & INST_SLOT0_MASK) >> 5;
                break;
        case 4: *pInst = ((*pInst) & INST_SLOT1_MASK) >> 14;
                bSlot1 = TRUE;
                break;
        case 8: *pInst = ((*pInst) & INST_SLOT2_MASK) >> 23;
                break;
        default:
            assert(!"Bad Slot");
    }

    GetAddrOff(addr) &= ~0xF;
    if(!AddrReadMemory(hthd->hprc, hthd, &addr, pTemplate, sizeof(ULONGLONG), &InstLen)) {
        return FALSE;
    }
    
    *pTemplate = ((*pTemplate) & INST_TEMPL_MASK) >> 1;

    if(*pTemplate == 0x2 && bSlot1) { //check for the MLI template, for movl it we need to reread slot 2, since that's where all the interesting info about it (predicate, etc), really is
        addr = *lpAddr;
        GetAddrOff(addr) += 4;
        if(!AddrReadMemory(hthd->hprc, hthd, &addr, pInst, sizeof(ULONGLONG), &InstLen)) {
            return FALSE;
        }
        *pInst = ((*pInst) & INST_SLOT2_MASK) >> 23;
    }
    return TRUE;
}

BOOL IsInstBreak(ULONGLONG Inst, DWORD dwSlot, ULONGLONG Template)
{
    if (MAJOR_OPCODE(Inst) != 0)  { // the major opcode has to be 0
        return FALSE;
    }
    if (BITS(Inst,32,27) != 0) { //6 bits 32:37 has to be 0
        return FALSE;
    }
    if ((BITS(Inst,35,33) != 0) && (OP_TYPE(Template,dwSlot) != B_OP)) { //bits 35:33 has to be 0 everywhere but break.b
        return FALSE;
    }
    return TRUE;
}

BOOL IsInstBranch(ULONGLONG Inst, DWORD dwSlot, ULONGLONG Template, DWORD dwMajorOpcode)
{
    if (OP_TYPE(Template,dwSlot) != B_OP) { // not a B; period
        DPRINT(1,("Inst: %I64x; Template:%I64x; Slot:%x - not a B\n",Inst,Template,dwSlot));
        return FALSE;
    }
    if (MAJOR_OPCODE(Inst) == dwMajorOpcode) {
        DPRINT(1,("Inst: %I64x; Template:%I64x; Slot:%x - a B with opcode %i\n",Inst,Template,dwSlot,dwMajorOpcode));
            return TRUE;
    } else {
        DPRINT(1,("Inst: %I64x; Template:%I64x; Slot:%x - a B but not with opcode %i\n",Inst,Template,dwSlot,dwMajorOpcode));
        return FALSE;
    }
}

BOOL IsInstBranchReturn(ULONGLONG Inst, DWORD dwSlot, ULONGLONG Template)
{
    if (OP_TYPE(Template,dwSlot) != B_OP) { // not a B; period
        return FALSE;
    }
    if ((MAJOR_OPCODE(Inst) == 0)   // major opcode 0
        && (BITS(Inst,32,27) == 21) // opcode extension 21
        && (BITS(Inst,8,6) == 4))   { // branch type opcode extension 4
        return TRUE;
    }
    return FALSE;
}


void IsCall (HTHDX   hthd, LPADDR  lpaddr, LPINT   lpf, BOOL    fStepOver )

/*++

Routine Description:

    IsCall

Arguments:

    hthd        - Supplies the handle to the thread

    lpaddr      - Supplies the address to be check for a call instruction

    lpf         - Returns class of instruction:
                     CALL
                     BREAKPOINT_INSTRUCTION
                     SOFTWARE_INTERRUPT
                     FALSE

    fStepOver

Return Value:

    None.

--*/

{
    DWORD InstLen;
    BYTE  PredReg;
    BYTE  dwSlot = (BYTE)((GetAddrOff(*lpaddr) & 0xF) >> 2);
    ULONGLONG Inst, Template;
    
    DPRINT(0,("Entering the IsCall routine for %p\n", GetAddrOff(*lpaddr)));

    if (hthd->fIsCallDone) {
        *lpaddr = hthd->addrIsCall;
        *lpf = hthd->iInstrIsCall;
        DPRINT(0,("fIsCallDone was set, exiting\n"));
        return;
    }

    /*
     *  Read in a bundle which contains the instruction under inspection to get the template
     */
    if (!GetInstAndTemplate(hthd, lpaddr, &Inst, &Template)) {
        DPRINT(0,("IsCall: Failed to get inst and template @ %p",GetAddrOff(*lpaddr)));
        return;
    }

    PredReg = (BYTE)BITS(Inst,5,0);
    if (((hthd->context.Preds >> PredReg) & 0x1) == 0) { //unset or bad predicate, we dont' care what the instruction really is
        
        DPRINT(0,("IsCall: Predicate not set %I64x:%i:%I64i\n",
               hthd->context.Preds,
               (DWORD)PredReg,
               hthd->context.Preds >> PredReg
               ));

    } else if (IsInstBreak(Inst,dwSlot,Template)) {
    
        DPRINT(0,("IsCall was a break, predicate set: Inst:%I64x Pred:%i\n",
            Inst,
            (DWORD)PredReg
            ));

        switch (((BITS(Inst,36,36) << 21) | BITS(Inst,25,6)) & 0x1c0000) {
            case BREAK_SYSCALL_BASE:
            case BREAK_FASTSYS_BASE:
                *lpf = INSTR_IS_CALL;
                DPRINT(0,("IsCall opcode was a INSTR_IS_CALL\n"));
                break;
            case BREAK_DEBUG_BASE:
                *lpf = INSTR_BREAKPOINT;
                DPRINT(0,("IsCall opcode was an INSTR_BREAKPOINT\n"));
                break;
            default:
                *lpf = INSTR_SOFT_INTERRUPT;
                DPRINT(0,("IsCall opcode was a INSTR_SOFT_INTERRUPT\n"));
                break;
        }
    } else if (IsInstBranch(Inst,dwSlot,Template,5)) {
        DPRINT(0,("IsCall opcode == br.call b1=target25\n"));
        *lpf = INSTR_IS_CALL;
    } else if (IsInstBranch(Inst,dwSlot,Template,1)) {
        DPRINT(0,("IsCall opcode == br.call b1=b2\n"));
        *lpf = INSTR_IS_CALL;
    } else  {
        DPRINT(0,("IsCall opcode == DEFAULT (not a call)\n"));
        *lpf = FALSE;
    }

// v-vadimp - shouldn't GetNextOffset do the following?
// advance current address for StepOver() breakpoint setting
    if (*lpf==INSTR_IS_CALL  ||  *lpf==INSTR_SOFT_INTERRUPT ) {    
        DPRINT(0,("Putting address in addrIsCall\n"));
        lpaddr->addr.off += 0x10;
        lpaddr->addr.off &= ~0xf;
        if (*lpf==INSTR_IS_CALL) {
            hthd->addrIsCall = *lpaddr;
        }
    }
    hthd->iInstrIsCall = *lpf;

    return;
}                               /* IsCall() */

BOOL IsRet(HTHDX hthd,LPADDR lpaddr)
{
    DWORD InstLen;
    BYTE PredReg;
    BYTE dwSlot = (BYTE)((GetAddrOff(*lpaddr) & 0xF) >> 2);
    ULONGLONG Inst, Template;


    DPRINT(0,("Entering the IsRet routine for %p\n", GetAddrOff(*lpaddr)));

    /*
     *  Read in a bundle which contains the instruction under inspection to get the template
     */
    if (!GetInstAndTemplate(hthd, lpaddr, &Inst, &Template)) {
        return FALSE;
    }
    
    PredReg = (BYTE)BITS(Inst,5,0);
    if (((hthd->context.Preds >> PredReg) & 0x1) == 0) { //unset or bad predicate, we dont' care what the instruction really is
        
        DPRINT(0,("IsRet: Predicate not set %I64x:%i:%I64i\n",
               hthd->context.Preds, 
               (DWORD)PredReg,
               hthd->context.Preds >> PredReg
               ));

        return FALSE;
    }
    
    if (IsInstBranchReturn(Inst,dwSlot,Template))  {
        DPRINT(0,("IsRet opcode == br.ret b2\n"));
        return TRUE;
    }

    DPRINT(0,("IsRet opcode == DEFAULT (not a ret)\n"));
    return FALSE;
}                               /* IsRet() */




UOFFSET IA64GetNextOffset (HTHDX hthd,BOOL fStep)

/*++

Routine Description:

    From a limited disassembly of the instruction pointed
    by the IAR register, compute the offset of the next
    instruction for either a trace or step operation.

Arguments:

    hthd  - Supplies the handle to the thread to get the next offset for

    fStep - Supplies TRUE for STEP offset and FALSE for trace offset

Return Value:

    Offset to place breakpoint at for doing a STEP or TRACE

--*/

{
    UOFFSET returnvalue;
    ADDR    firaddr;
    DWORD InstLen;
    BYTE dwSlot, PredReg;
    ULONGLONG Inst, Template;


    AddrFromHthdx(&firaddr, hthd);
    
    DPRINT(0,("Entering the GetNextOffset routine for %p\n",GetAddrOff(firaddr)));
    
    dwSlot = (BYTE)((GetAddrOff(firaddr) & 0xF) >> 2);
    // increment current address as default return value

    switch(dwSlot) {
    case 0:
    case 1:
        returnvalue = GetAddrOff(firaddr) + 4;
        break;

    case 2:
        returnvalue = GetAddrOff(firaddr) + 8;
        break;

    default:
        DPRINT(0, ("(GetNextOffset?) illegal EM address FIR=%p", GetAddrOff(firaddr)));
    }

    if (!GetInstAndTemplate(hthd, &firaddr, &Inst, &Template))
    {
        DPRINT(0,("GetNextOffset: Failed to get inst and template @ %p",GetAddrOff(firaddr)));
        return returnvalue;
    }

    PredReg = (BYTE)BITS(Inst,5,0);
    if (IsInstBreak(Inst,dwSlot,Template)) {
    
        DPRINT(0,("we have - break, \n"));
        switch (((BITS(Inst,36,36) << 21) | BITS(Inst,25,6)) & 0x1c0000) {
            case BREAK_SYSCALL_BASE: //stepping over a syscall inst sets a break on the callers return address
            case BREAK_FASTSYS_BASE:
                DPRINT(0,("Sys instruction - Setting next offset to %p\n",hthd->context.BrRp));
                returnvalue = (UOFFSET)(hthd->context.BrRp);
                break;
        }
    }
    else if (IsInstBranch(Inst,dwSlot,Template,5)) { // IP relative call
      DPRINT(0,("We have - br.call b1=target25\n"));
      if (((hthd->context.Preds >> PredReg) & 0x1) && !fStep) { // predicate set and not StepOver
          //LONG offset = (((INT)(BITS(Inst,36,36) << 21) | (LONG)(BITS(Inst,32,13) << 4)) << 11) >> 11;
          //DPRINT(0,("target25 - offset:%li\n",offset));
          returnvalue = (GetAddrOff(firaddr) & ~0xF) + ((((INT)(BITS(Inst,36,36) << 21) | (LONG)(BITS(Inst,32,13) << 4)) << 11) >> 11); //target25 == (sext(s<<20|imm21)) << 4;
      }
    }
    else if (IsInstBranch(Inst,dwSlot,Template,1)) {// indirect call
      DPRINT(0,("We have - br.call b1=b2\n"));
      if (((hthd->context.Preds >> PredReg) & 0x1) && !fStep) { // predicate set and not StepOver
             returnvalue = (UOFFSET)((ULONGLONG *)(&hthd->context.BrRp)[BITS(Inst,15,13)]);
      }
    } else if (IsInstBranch(Inst,dwSlot,Template,0)) { // indirect branch
      DPRINT(0,("We have - br.<...> (minor opcode:%I64i) b1=b2\n",BITS(Inst,8,6)));
      if ((hthd->context.Preds >> PredReg) & 0x1) // predicate set
            returnvalue = (UOFFSET)((ULONGLONG *)(&hthd->context.BrRp)[BITS(Inst,15,13)]);
    } else if (IsInstBranch(Inst,dwSlot,Template,4)) { // IP relative branch
       UOFFSET NewReturnValue = (GetAddrOff(firaddr) & ~0xF) + ((((INT)(BITS(Inst,36,36) << 21) | (LONG)(BITS(Inst,32,13) << 4)) << 11) >> 11);  //target25 == (sext(s<<20|imm21)) << 4;
       switch (BITS(Inst,8,6))  {
        case 0: // regular conditional branch
            DPRINT(0,("We have - br.cond target25"));
            if ((hthd->context.Preds >> PredReg) & 0x1) // taken if predicate set
                    returnvalue = NewReturnValue;
            break;
        case 2: // wexit
            DPRINT(0,("We have - br.wexit b1=target25\n")); 
            if (!((hthd->context.Preds >> PredReg) & 0x1) && (hthd->context.ApEC <= 1)) // taken if predicate not set and EC <= 1
                    returnvalue = NewReturnValue;
            break;
        case 3: //wtop
            DPRINT(0,("We have - br.wtop b1=target25\n"));
            if (((hthd->context.Preds >> PredReg) & 0x1) || (hthd->context.ApEC > 1)) // taken if predicate set or EC > 1
                    returnvalue = NewReturnValue;
            break;
        case 5: // cloop
            DPRINT(0,("We have - br.cloop target25"));
                if (hthd->context.ApLC != 0)  // taken if LC !=0
                    returnvalue = NewReturnValue;
            break;
        case 6: // cexit
            DPRINT(0,("We have - br.cexit b1=target25\n"));
                if ((hthd->context.ApLC ==0) && (hthd->context.ApEC <= 1))   // taken if LC==0 and EC <=1
                    returnvalue = NewReturnValue;
            break;
        case 7: // ctop
            DPRINT(0,("We have - br.ctop b1=target25\n"));
                if ((hthd->context.ApLC !=0) || (hthd->context.ApEC > 1))   // taken if LC!=0 or EC >1
                    returnvalue = NewReturnValue;
            break;
        default:
            DPRINT(0,("We have - br.<unknown> b1=target25!!!\n"));
            break;
       } 
    } else if (Template == 0x2 && dwSlot == 1) {
        // that should signify movl 
        // increment return offset since movl instruction takes two instruction slots
        DPRINT(0,("We have - movl.r1 imm64; returnvalue:%p",returnvalue));
        switch(returnvalue & 0xf) {
                case 8:
                    returnvalue += returnvalue;
                    break;

                default:
                    DPRINT(0, ("(GetNextOffset?) illegal MOVL address: %p", GetAddrOff(firaddr)));
                    break;
        }
     } else  {
        DPRINT(0,("GetNextOffset: We have an unhandled DEFAULT op\n"));
     }
    
    DPRINT(0,("GetNextOffset: next offset is %p\n",returnvalue));
    return returnvalue;
}                           /* GetNextOffset() */

DWORD
BranchUnassemble(
    HTHDX   hthd,
    void   *Memory,
    DWORD   Size,
    ADDR   *Addr,
    BOOL   *IsBranch,
    BOOL   *TargetKnown,
    BOOL   *IsCall,
    BOOL   *IsTable,
    ADDR   *Target
    )


{
    UOFFSET TargetOffset;
    ADDR    firaddr = *Addr;
    EM_Decoder_Info    info;


    DPRINT(0,("Entering the BranchUnassemble routine"));

    assert( Memory );
    assert( IsBranch );
    assert( TargetKnown );
    assert( IsCall );
    assert( Target );

    TargetOffset = 0;
    *IsBranch = FALSE;
    *IsTable = FALSE;
    *TargetKnown = FALSE;


    GetAddrOff(firaddr) &= ~0xF; 

    if (RequestEMToDecode(hthd, &firaddr, &info)) {

        switch( info.inst ) {
        //
        // IP-Relative call B3           - br.call b1=target25
        //
            case EM_BR_CALL_SPNT_FEW_B1_TARGET25:
            case EM_BR_CALL_SPNT_MANY_B1_TARGET25:
            case EM_BR_CALL_SPTK_FEW_B1_TARGET25:
            case EM_BR_CALL_SPTK_MANY_B1_TARGET25:
            case EM_BR_CALL_DPNT_FEW_B1_TARGET25:
            case EM_BR_CALL_DPNT_MANY_B1_TARGET25:
            case EM_BR_CALL_DPTK_FEW_B1_TARGET25:
            case EM_BR_CALL_DPTK_MANY_B1_TARGET25:
            case EM_BR_CALL_SPNT_FEW_CLR_B1_TARGET25:
            case EM_BR_CALL_SPNT_MANY_CLR_B1_TARGET25:
            case EM_BR_CALL_SPTK_FEW_CLR_B1_TARGET25:
            case EM_BR_CALL_SPTK_MANY_CLR_B1_TARGET25:
            case EM_BR_CALL_DPNT_FEW_CLR_B1_TARGET25:
            case EM_BR_CALL_DPNT_MANY_CLR_B1_TARGET25:
            case EM_BR_CALL_DPTK_FEW_CLR_B1_TARGET25:
            case EM_BR_CALL_DPTK_MANY_CLR_B1_TARGET25:
                *IsCall = TRUE;
                *IsBranch = TRUE;
                if (info.src1.type == EM_DECODER_IMMEDIATE) {
                    if (info.src1.imm_info.imm_type == EM_DECODER_IMM_SIGNED) {
                           *TargetKnown = TRUE;
                        TargetOffset = (IEL_GETDW0(info.src1.imm_info.val64)) + firaddr.addr.off;
                    }            
                }
                break;
        //
        // IP-Relative branch B1         - br.cond target25
        //
            case EM_BR_COND_SPNT_FEW_TARGET25:
            case EM_BR_COND_SPNT_MANY_TARGET25:
            case EM_BR_COND_SPTK_FEW_TARGET25:
            case EM_BR_COND_SPTK_MANY_TARGET25:
            case EM_BR_COND_DPNT_FEW_TARGET25:
            case EM_BR_COND_DPNT_MANY_TARGET25:
            case EM_BR_COND_DPTK_FEW_TARGET25:
            case EM_BR_COND_DPTK_MANY_TARGET25:
            case EM_BR_COND_SPNT_FEW_CLR_TARGET25:
            case EM_BR_COND_SPNT_MANY_CLR_TARGET25:
            case EM_BR_COND_SPTK_FEW_CLR_TARGET25:
            case EM_BR_COND_SPTK_MANY_CLR_TARGET25:
            case EM_BR_COND_DPNT_FEW_CLR_TARGET25:
            case EM_BR_COND_DPNT_MANY_CLR_TARGET25:
            case EM_BR_COND_DPTK_FEW_CLR_TARGET25:
            case EM_BR_COND_DPTK_MANY_CLR_TARGET25:
                *IsCall = FALSE;
                *IsBranch = TRUE;
                if (info.src1.type == EM_DECODER_IMMEDIATE) {
                    if (info.src1.imm_info.imm_type == EM_DECODER_IMM_SIGNED) {
                           *TargetKnown = TRUE;
                        TargetOffset = (IEL_GETDW0(info.src1.imm_info.val64)) + firaddr.addr.off;
                    }            
                }
                break;
        //                               - br.wexit target25
            case EM_BR_WEXIT_SPNT_FEW_TARGET25:
            case EM_BR_WEXIT_SPNT_MANY_TARGET25:
            case EM_BR_WEXIT_SPTK_FEW_TARGET25:
            case EM_BR_WEXIT_SPTK_MANY_TARGET25:
            case EM_BR_WEXIT_DPNT_FEW_TARGET25:
            case EM_BR_WEXIT_DPNT_MANY_TARGET25:
            case EM_BR_WEXIT_DPTK_FEW_TARGET25:
            case EM_BR_WEXIT_DPTK_MANY_TARGET25:
            case EM_BR_WEXIT_SPNT_FEW_CLR_TARGET25:
            case EM_BR_WEXIT_SPNT_MANY_CLR_TARGET25:
            case EM_BR_WEXIT_SPTK_FEW_CLR_TARGET25:
            case EM_BR_WEXIT_SPTK_MANY_CLR_TARGET25:
            case EM_BR_WEXIT_DPNT_FEW_CLR_TARGET25:
            case EM_BR_WEXIT_DPNT_MANY_CLR_TARGET25:
            case EM_BR_WEXIT_DPTK_FEW_CLR_TARGET25:
            case EM_BR_WEXIT_DPTK_MANY_CLR_TARGET25:
                *IsCall = FALSE;
                *IsBranch = TRUE;
                if (info.src1.type == EM_DECODER_IMMEDIATE) {
                    if (info.src1.imm_info.imm_type == EM_DECODER_IMM_SIGNED) {
                           *TargetKnown = TRUE;
                        TargetOffset =(IEL_GETDW0(info.src1.imm_info.val64)) + firaddr.addr.off;
                    }
                }                                                    // if PR[qp] = 0, kernel; fall-thru
                break;
        //                               - br.wtop target25
            case EM_BR_WTOP_SPNT_FEW_TARGET25:
            case EM_BR_WTOP_SPNT_MANY_TARGET25:
            case EM_BR_WTOP_SPTK_FEW_TARGET25:
            case EM_BR_WTOP_SPTK_MANY_TARGET25:
            case EM_BR_WTOP_DPNT_FEW_TARGET25:
            case EM_BR_WTOP_DPNT_MANY_TARGET25:
            case EM_BR_WTOP_DPTK_FEW_TARGET25:
            case EM_BR_WTOP_DPTK_MANY_TARGET25:
            case EM_BR_WTOP_SPNT_FEW_CLR_TARGET25:
            case EM_BR_WTOP_SPNT_MANY_CLR_TARGET25:
            case EM_BR_WTOP_SPTK_FEW_CLR_TARGET25:
            case EM_BR_WTOP_SPTK_MANY_CLR_TARGET25:
            case EM_BR_WTOP_DPNT_FEW_CLR_TARGET25:
            case EM_BR_WTOP_DPNT_MANY_CLR_TARGET25:
            case EM_BR_WTOP_DPTK_FEW_CLR_TARGET25:
            case EM_BR_WTOP_DPTK_MANY_CLR_TARGET25:
                *IsCall = FALSE;
                *IsBranch = TRUE;
                if (info.src1.type == EM_DECODER_IMMEDIATE) {
                    if (info.src1.imm_info.imm_type == EM_DECODER_IMM_SIGNED) {
                           *TargetKnown = TRUE;
                        TargetOffset =(IEL_GETDW0(info.src1.imm_info.val64)) + firaddr.addr.off;
                    }
                } 
                else {                                                 // if PR[qp] = 0, kernel; branch
                    if (info.src1.type == EM_DECODER_IMMEDIATE) {
                        if (info.src1.imm_info.imm_type == EM_DECODER_IMM_SIGNED) {
                               *TargetKnown = TRUE;
                            TargetOffset =(IEL_GETDW0(info.src1.imm_info.val64)) + firaddr.addr.off;
                        }
                    }
                }
                break;
        //
        // IP-Relative counted branch B2 - br.cloop target25
        //
            case EM_BR_CLOOP_SPNT_FEW_TARGET25:
            case EM_BR_CLOOP_SPNT_MANY_TARGET25:
            case EM_BR_CLOOP_SPTK_FEW_TARGET25:
            case EM_BR_CLOOP_SPTK_MANY_TARGET25:
            case EM_BR_CLOOP_DPNT_FEW_TARGET25:
            case EM_BR_CLOOP_DPNT_MANY_TARGET25:
            case EM_BR_CLOOP_DPTK_FEW_TARGET25:
            case EM_BR_CLOOP_DPTK_MANY_TARGET25:
            case EM_BR_CLOOP_SPNT_FEW_CLR_TARGET25:
            case EM_BR_CLOOP_SPNT_MANY_CLR_TARGET25:
            case EM_BR_CLOOP_SPTK_FEW_CLR_TARGET25:
            case EM_BR_CLOOP_SPTK_MANY_CLR_TARGET25:
            case EM_BR_CLOOP_DPNT_FEW_CLR_TARGET25:
            case EM_BR_CLOOP_DPNT_MANY_CLR_TARGET25:
            case EM_BR_CLOOP_DPTK_FEW_CLR_TARGET25:
            case EM_BR_CLOOP_DPTK_MANY_CLR_TARGET25:
                *IsCall = FALSE;
                *IsBranch = TRUE;
                if (info.src1.type == EM_DECODER_IMMEDIATE) {
                    if (info.src1.imm_info.imm_type == EM_DECODER_IMM_SIGNED) {
                          *TargetKnown = TRUE;
                        TargetOffset = (IEL_GETDW0(info.src1.imm_info.val64)) + firaddr.addr.off;
                    }
                } 
                break;

        //                               - br.cexit target25
            case EM_BR_CEXIT_SPNT_FEW_TARGET25:
            case EM_BR_CEXIT_SPNT_MANY_TARGET25:
            case EM_BR_CEXIT_SPTK_FEW_TARGET25:
            case EM_BR_CEXIT_SPTK_MANY_TARGET25:
            case EM_BR_CEXIT_DPNT_FEW_TARGET25:
            case EM_BR_CEXIT_DPNT_MANY_TARGET25:
            case EM_BR_CEXIT_DPTK_FEW_TARGET25:
            case EM_BR_CEXIT_DPTK_MANY_TARGET25:
            case EM_BR_CEXIT_SPNT_FEW_CLR_TARGET25:
            case EM_BR_CEXIT_SPNT_MANY_CLR_TARGET25:
            case EM_BR_CEXIT_SPTK_FEW_CLR_TARGET25:
            case EM_BR_CEXIT_SPTK_MANY_CLR_TARGET25:
            case EM_BR_CEXIT_DPNT_FEW_CLR_TARGET25:
            case EM_BR_CEXIT_DPNT_MANY_CLR_TARGET25:
            case EM_BR_CEXIT_DPTK_FEW_CLR_TARGET25:
            case EM_BR_CEXIT_DPTK_MANY_CLR_TARGET25:
                *IsCall = FALSE;
                *IsBranch = TRUE;
                if (info.src1.type == EM_DECODER_IMMEDIATE) {
                    if (info.src1.imm_info.imm_type == EM_DECODER_IMM_SIGNED) {
                          *TargetKnown = TRUE;
                        TargetOffset = (IEL_GETDW0(info.src1.imm_info.val64)) + firaddr.addr.off;
                    }
                }                                            // if LC > 0, kernel; fall-thru
                break;
        //                               - br.ctop target25
            case EM_BR_CTOP_SPNT_FEW_TARGET25:
            case EM_BR_CTOP_SPNT_MANY_TARGET25:
            case EM_BR_CTOP_SPTK_FEW_TARGET25:
            case EM_BR_CTOP_SPTK_MANY_TARGET25:
            case EM_BR_CTOP_DPNT_FEW_TARGET25:
            case EM_BR_CTOP_DPNT_MANY_TARGET25:
            case EM_BR_CTOP_DPTK_FEW_TARGET25:
            case EM_BR_CTOP_DPTK_MANY_TARGET25:
            case EM_BR_CTOP_SPNT_FEW_CLR_TARGET25:
            case EM_BR_CTOP_SPNT_MANY_CLR_TARGET25:
            case EM_BR_CTOP_SPTK_FEW_CLR_TARGET25:
            case EM_BR_CTOP_SPTK_MANY_CLR_TARGET25:
            case EM_BR_CTOP_DPNT_FEW_CLR_TARGET25:
            case EM_BR_CTOP_DPNT_MANY_CLR_TARGET25:
            case EM_BR_CTOP_DPTK_FEW_CLR_TARGET25:
            case EM_BR_CTOP_DPTK_MANY_CLR_TARGET25:
                *IsCall = FALSE;
                *IsBranch = TRUE;
                if (info.src1.type == EM_DECODER_IMMEDIATE) {
                    if (info.src1.imm_info.imm_type == EM_DECODER_IMM_SIGNED) {
                          *TargetKnown = TRUE;
                        TargetOffset = (IEL_GETDW0(info.src1.imm_info.val64)) + firaddr.addr.off;
                    }
                } 
                else {                                                 // if LC > 0, kernel; branch
                    if (info.src1.type == EM_DECODER_IMMEDIATE)
                        if (info.src1.imm_info.imm_type == EM_DECODER_IMM_SIGNED) {
                              *TargetKnown = TRUE;
                            TargetOffset = (IEL_GETDW0(info.src1.imm_info.val64)) + firaddr.addr.off;
                        }
                }
                break;
        //
        // Indirect call B5            - br.call b1=b2
        //
            case EM_BR_CALL_SPNT_FEW_B1_B2:
            case EM_BR_CALL_SPNT_MANY_B1_B2:
            case EM_BR_CALL_SPTK_FEW_B1_B2:
            case EM_BR_CALL_SPTK_MANY_B1_B2:
            case EM_BR_CALL_DPNT_FEW_B1_B2:
            case EM_BR_CALL_DPNT_MANY_B1_B2:
            case EM_BR_CALL_DPTK_FEW_B1_B2:
            case EM_BR_CALL_DPTK_MANY_B1_B2:
            case EM_BR_CALL_SPNT_FEW_CLR_B1_B2:
            case EM_BR_CALL_SPNT_MANY_CLR_B1_B2:
            case EM_BR_CALL_SPTK_FEW_CLR_B1_B2:
            case EM_BR_CALL_SPTK_MANY_CLR_B1_B2:
            case EM_BR_CALL_DPNT_FEW_CLR_B1_B2:
            case EM_BR_CALL_DPNT_MANY_CLR_B1_B2:
            case EM_BR_CALL_DPTK_FEW_CLR_B1_B2:
            case EM_BR_CALL_DPTK_MANY_CLR_B1_B2:
                *IsCall = TRUE;
                *IsBranch = TRUE;
                *TargetKnown = FALSE;
                break;

        //
        // Indirect branch B4           - br.ia b2
        //
            case EM_BR_IA_SPNT_FEW_B2:
            case EM_BR_IA_SPNT_MANY_B2:
            case EM_BR_IA_SPTK_FEW_B2:
            case EM_BR_IA_SPTK_MANY_B2:
            case EM_BR_IA_DPNT_FEW_B2:
            case EM_BR_IA_DPNT_MANY_B2:
            case EM_BR_IA_DPTK_FEW_B2:
            case EM_BR_IA_DPTK_MANY_B2:
            case EM_BR_IA_SPNT_FEW_CLR_B2:
            case EM_BR_IA_SPNT_MANY_CLR_B2:
            case EM_BR_IA_SPTK_FEW_CLR_B2:
            case EM_BR_IA_SPTK_MANY_CLR_B2:
            case EM_BR_IA_DPNT_FEW_CLR_B2:
            case EM_BR_IA_DPNT_MANY_CLR_B2:
            case EM_BR_IA_DPTK_FEW_CLR_B2:
            case EM_BR_IA_DPTK_MANY_CLR_B2:
                *IsCall = FALSE;
                *IsBranch = TRUE;
                *TargetKnown = FALSE;
                break;
        //
        //                              - br.cond b2
            case EM_BR_COND_SPNT_FEW_B2:
            case EM_BR_COND_SPNT_MANY_B2:
            case EM_BR_COND_SPTK_FEW_B2:
            case EM_BR_COND_SPTK_MANY_B2:
            case EM_BR_COND_DPNT_FEW_B2:
            case EM_BR_COND_DPNT_MANY_B2:
            case EM_BR_COND_DPTK_FEW_B2:
            case EM_BR_COND_DPTK_MANY_B2:
            case EM_BR_COND_SPNT_FEW_CLR_B2:
            case EM_BR_COND_SPNT_MANY_CLR_B2:
            case EM_BR_COND_SPTK_FEW_CLR_B2:
            case EM_BR_COND_SPTK_MANY_CLR_B2:
            case EM_BR_COND_DPNT_FEW_CLR_B2:
            case EM_BR_COND_DPNT_MANY_CLR_B2:
            case EM_BR_COND_DPTK_FEW_CLR_B2:
            case EM_BR_COND_DPTK_MANY_CLR_B2:
                *IsCall = FALSE;
                *IsBranch = TRUE;
                *TargetKnown = FALSE;
                break;
        //
        //                              - br.ret b2
            case EM_BR_RET_SPNT_FEW_B2:
            case EM_BR_RET_SPNT_MANY_B2:
            case EM_BR_RET_SPTK_FEW_B2:
            case EM_BR_RET_SPTK_MANY_B2:
            case EM_BR_RET_DPNT_FEW_B2:
            case EM_BR_RET_DPNT_MANY_B2:
            case EM_BR_RET_DPTK_FEW_B2:
            case EM_BR_RET_DPTK_MANY_B2:
            case EM_BR_RET_SPNT_FEW_CLR_B2:
            case EM_BR_RET_SPNT_MANY_CLR_B2:
            case EM_BR_RET_SPTK_FEW_CLR_B2:
            case EM_BR_RET_SPTK_MANY_CLR_B2:
            case EM_BR_RET_DPNT_FEW_CLR_B2:
            case EM_BR_RET_DPNT_MANY_CLR_B2:
            case EM_BR_RET_DPTK_FEW_CLR_B2:
            case EM_BR_RET_DPTK_MANY_CLR_B2:
                *IsCall = FALSE;
                *IsBranch = TRUE;
                *TargetKnown = FALSE;
                break;

            default:
                *IsCall = FALSE;
                *IsBranch = FALSE;
                *TargetKnown = FALSE;
                break;
        }

    } else {
        DPRINT(0,("Error disassembling instruction @ %I64x",GetAddrOff(firaddr)));
        return 0;
    }

    AddrInit( Target, 0, 0, TargetOffset, TRUE, TRUE, FALSE, FALSE );

    return sizeof(IA64_INSTRUCTION);
}                           /* BranchUnassemble() */

#endif //"native code"

#ifndef KERNEL
void
ProcessGetDRegsCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    PULONGLONG   lpdw = (PULONGLONG)LpDmMsg->rgb;
    CONTEXT   cxt;
    int       rs = 0;

    DEBUG_PRINT( "ProcessGetDRegsCmd :\n");


    if (hthd == 0) {
        rs = 0;
    } else {
        cxt.ContextFlags = CONTEXT_DEBUG;
        if (!GetThreadContext(hthd->rwHand, &cxt)) {
            LpDmMsg->xosdRet = xosdUnknown;
            rs = 0;
        } else {
            lpdw[0] = hthd->context.DbI0;
            lpdw[1] = hthd->context.DbI1;
            lpdw[2] = hthd->context.DbI2;
            lpdw[3] = hthd->context.DbI3;
            lpdw[4] = hthd->context.DbI4;
            lpdw[5] = hthd->context.DbI5;
            lpdw[6] = hthd->context.DbI6;
            lpdw[7] = hthd->context.DbI7;

            lpdw[8]  = hthd->context.DbD0;
            lpdw[9]  = hthd->context.DbD1;
            lpdw[10] = hthd->context.DbD2;
            lpdw[11] = hthd->context.DbD3;
            lpdw[12] = hthd->context.DbD4;
            lpdw[13] = hthd->context.DbD5;
            lpdw[14] = hthd->context.DbD6;
            lpdw[15] = hthd->context.DbD7;
            LpDmMsg->xosdRet = xosdNone;
            rs = sizeof(CONTEXT);
        }
    }

    Reply( rs, LpDmMsg, lpdbb->hpid );
    return;
}                             /* ProcessGetDRegsCmd() */



void
ProcessSetDRegsCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    PULONGLONG     lpdw = (PULONGLONG)(lpdbb->rgbVar);
    XOSD            xosd = xosdNone;

    Unreferenced(hprc);

    DPRINT(0, ("ProcessSetDRegsCmd : "));

    hthd->context.ContextFlags = CONTEXT_DEBUG;

    hthd->context.DbI0 = lpdw[0];
    hthd->context.DbI1 = lpdw[1];
    hthd->context.DbI2 = lpdw[2];
    hthd->context.DbI3 = lpdw[3];
    hthd->context.DbI4 = lpdw[4];
    hthd->context.DbI5 = lpdw[5];
    hthd->context.DbI6 = lpdw[6];
    hthd->context.DbI7 = lpdw[7];

    hthd->context.DbD0 = lpdw[8];
    hthd->context.DbD1 = lpdw[9];
    hthd->context.DbD2 = lpdw[10];
    hthd->context.DbD3 = lpdw[11];
    hthd->context.DbD4 = lpdw[12];
    hthd->context.DbD5 = lpdw[13];
    hthd->context.DbD6 = lpdw[14];
    hthd->context.DbD7 = lpdw[15];

    if (hthd->fWowEvent) {
        WOWSetThreadContext(hthd, &hthd->context);
    } else {
        SetThreadContext(hthd->rwHand, &hthd->context);
    }

    Reply(0, &xosd, lpdbb->hpid);

    return;
}                               /* ProcessSetDRegsCmd() */

/*VOID
MakeThreadSuspendItselfHelper(
    HTHDX   hthd,
    FARPROC lpSuspendThread
    )
{
    ULONG NewBspStore;
    ULONGLONG ArgumentsCopy[2];
    ULONG Count = 0;
    ULONG ShiftCount;
    BOOLEAN RnatSaved = FALSE;
    DWORD i, ra;

    //
    // set up the args to SuspendThread. IA64 software convention defines the 
    // parameter passing via backing store(r32-r39). Prepare argument copy
    // with current thread handle and RNAT if its backing store address [10:3]
    // are all 1.
    //

    NewBspStore = (ULONG)hthd->context.RsBSPSTORE;

    if ((NewBspStore & 0x1F8) == 0x1F8) {
        ArgumentsCopy[Count++] = hthd->context.RsRNAT;
        NewBspStore += sizeof(ULONGLONG);
        RnatSaved = TRUE;
    }
    ShiftCount = (NewBspStore & 0x1F8) >> 3;
    hthd->context.RsRNAT &= ~(0x1 << ShiftCount);
    ArgumentsCopy[Count++] = (ULONGLONG)GetCurrentThread();
    NewBspStore += sizeof(ULONGLONG);     

    if ((RnatSaved == FALSE) && ((NewBspStore & 0x1F8) == 0x1F8)) {
        ArgumentsCopy[Count++] = hthd->context.RsRNAT;
        NewBspStore += sizeof(ULONGLONG);
    }

    // Copy the argument onto the backing store

    AddrWriteMemory(hthd->hprc, 
                      hthd,
                      (LPADDR)hthd->context.RsBSPSTORE,
                      ArgumentsCopy, 
                      Count * sizeof(ULONGLONG),
                      &i);
    //
    // zero out loadrs, adjust RseStack
    //

    hthd->context.RsRSC = (RSC_MODE_LY<<RSC_MODE)
                       | (RSC_BE_LITTLE<<RSC_BE) 
                       | (0x3<<RSC_PL);


    // 
    // Setup argument numbers
    //

    hthd->context.StIFS = (ULONGLONG)Count;    // # of arguments/size of frame(sof)
    hthd->context.RsBSP = hthd->context.RsBSPSTORE = P32ToP64((LPVOID)NewBspStore);

    // Set the address of the target code into IIP and IPSR.ri; save the current IIP in RP. 
    // No change to Sp and Ap.

    ra = PC(hthd);
    hthd->context.BrRp = P32ToP64((LPVOID)ra);
    hthd->context.StIIP = P32ToP64(*lpSuspendThread);
    hthd->context.StIPSR &= ~((ULONGLONG)0x3 << PSR_RI);
    hthd->context.StIPSR |= ((hthd->context.StIIP & 0xf) >> 2) << PSR_RI;
    hthd->context.StIIP &= 0xf;
    hthd->fContextDirty = TRUE;
}
*/
#endif  // !KERNEL


XOSD
SetupFunctionCall(
    LPEXECUTE_OBJECT_DM    lpeo,
    LPEXECUTE_STRUCT       lpes
    )
{
    /*
     *  Can only execute functions on the current stopped thread.  Therefore
     *  assert that the current thread is stopped.
     */

    assert(lpeo->hthd->tstate & ts_stopped);
    if (!(lpeo->hthd->tstate & ts_stopped)) {
        return xosdBadThread;
    }

    /*
     * Now get the current stack offset.
     */

    lpeo->addrStack.addr.off = (ULONG)lpeo->hthd->context.IntSp;

    /*
     * Now place the return address correctly
     */

    // No need to update StIPSR.ri. return address is a bundle address. 

    lpeo->hthd->context.StIIP = lpeo->hthd->context.BrRp =
    //P32ToP64((LPVOID)lpeo->addrStart.addr.off); v-vadimp???

    /*
     * Set the instruction pointer to the starting addresses
     *  and write the context back out
     */

    lpeo->hthd->context.StIIP = GetAddrOff(lpeo->addrStart);

    lpeo->hthd->fContextDirty = TRUE;

    return xosdNone;
}



BOOL
CompareStacks(
    LPEXECUTE_OBJECT_DM       lpeo
    )

/*++

Routine Description:

    This routine is used to determine if the stack pointers are currect
    for terminating function evaluation.

Arguments:

    lpeo        - Supplies the pointer to the DM Execute Object description

Return Value:

    TRUE if the evaluation is to be terminated and FALSE otherwise

--*/

{

    if (lpeo->addrStack.addr.off <= lpeo->hthd->context.IntSp) {
        return TRUE;
    }

    return FALSE;
}                               /* CompareStacks() */

BOOL
ProcessFrameStackWalkNextCmd(
    HPRCX hprc,
    HTHDX hthd,
    PCONTEXT context,
    LPVOID pctxPtrs
    )

{
    return FALSE;
}



BOOL
SetupDebugRegister(
    HTHDX       hthd,
    int         Register,
    int         DataSize,
    DWORDLONG   DataAddr,
    DWORD       BpType
    )
/*++

Routine Description:

    This routine is used to initialize the selected debug register.

Arguments:

    Register    - Selected debug register # (0-3).
    DataSize    - Data size to be match in byte. Used by LenMask[] to select debug mask.. 
    DataAddr    - Breakpoint address
    BpType      - Breakpoint types. Defined in od.h 


Return Value:

    TRUE if successful and FALSE otherwise

--*/
{
    DWORDLONG               Len;

#ifdef KERNEL
    KSPECIAL_REGISTERS  ksr;
    PULONGLONG  DbI0 = &ksr.KernelDbI0;
    PULONGLONG  DbI1 = &ksr.KernelDbI1;
    PULONGLONG  DbI2 = &ksr.KernelDbI2;
    PULONGLONG  DbI3 = &ksr.KernelDbI3;
    PULONGLONG  DbI4 = &ksr.KernelDbI4;
    PULONGLONG  DbI5 = &ksr.KernelDbI5;
    PULONGLONG  DbI6 = &ksr.KernelDbI6;
    PULONGLONG  DbI7 = &ksr.KernelDbI7;

    PULONGLONG  DbD0 = &ksr.KernelDbD0;
    PULONGLONG  DbD1 = &ksr.KernelDbD1;
    PULONGLONG  DbD2 = &ksr.KernelDbD2;
    PULONGLONG  DbD3 = &ksr.KernelDbD3;
    PULONGLONG  DbD4 = &ksr.KernelDbD4;
    PULONGLONG  DbD5 = &ksr.KernelDbD5;
    PULONGLONG  DbD6 = &ksr.KernelDbD6;
    PULONGLONG  DbD7 = &ksr.KernelDbD7;
#else
    BOOL        bDbgRet;
    CONTEXT     Context;
    PULONGLONG  DbI0 = &Context.DbI0;
    PULONGLONG  DbI1 = &Context.DbI1;
    PULONGLONG  DbI2 = &Context.DbI2;
    PULONGLONG  DbI3 = &Context.DbI3;
    PULONGLONG  DbI4 = &Context.DbI4;
    PULONGLONG  DbI5 = &Context.DbI5;
    PULONGLONG  DbI6 = &Context.DbI6;
    PULONGLONG  DbI7 = &Context.DbI7;

    PULONGLONG  DbD0 = &Context.DbD0;
    PULONGLONG  DbD1 = &Context.DbD1;
    PULONGLONG  DbD2 = &Context.DbD2;
    PULONGLONG  DbD3 = &Context.DbD3;
    PULONGLONG  DbD4 = &Context.DbD4;
    PULONGLONG  DbD5 = &Context.DbD5;
    PULONGLONG  DbD6 = &Context.DbD6;
    PULONGLONG  DbD7 = &Context.DbD7;
#endif

    // IA64 currently only supports 4 BR's
    assert(Register >= 4);

#ifdef KERNEL
    if (!GetExtendedContext(hthd, &ksr))
#else
    Context.ContextFlags = CONTEXT_DEBUG;
    if (!GetThreadContext(hthd->rwHand, &Context))
#endif
    {
        return FALSE;
    }

    assert(DataSize > 9);
    Len  = LenMask[ DataSize ];

    // enable breakpoint at all privilege levels
    switch( BpType ) {
        case bptpDataR:     
            *(DbD0 + Register) = DataAddr;
            *(DbD1 + Register) = RWE_READ | PLM_MASK | Len;
            break;

        case bptpDataW:
        case bptpDataC:
            *(DbD0 + Register) = DataAddr;
            *(DbD1 + Register) = RWE_WRITE | PLM_MASK | Len;
            break;

        case bptpDataExec:
            *(DbI0 + Register) = DataAddr;
            *(DbI1 + Register) = RWE_EXEC | PLM_MASK | Len;
            break;

        default:
            assert(!"Invalid BpType!!");
            break;
    }

#ifdef KERNEL
    return SetExtendedContext(hthd, &ksr);
#else
    
    bDbgRet = SetThreadContext(hthd->rwHand, &Context);

    DPRINT(0, (_T("Setting Thread Context in debug register for thread %x:%i\n"), 
           hthd->tid,
           bDbgRet
           ));
    return bDbgRet;
#endif

}

VOID
ClearDebugRegister(
    HTHDX   hthd,
    int     Register
    )
{

#ifdef KERNEL
    KSPECIAL_REGISTERS  ksr;
    PULONGLONG  DbI0 = &ksr.KernelDbI0;
    PULONGLONG  DbI1 = &ksr.KernelDbI1;
    PULONGLONG  DbI2 = &ksr.KernelDbI2;
    PULONGLONG  DbI3 = &ksr.KernelDbI3;
    PULONGLONG  DbI4 = &ksr.KernelDbI4;
    PULONGLONG  DbI5 = &ksr.KernelDbI5;
    PULONGLONG  DbI6 = &ksr.KernelDbI6;
    PULONGLONG  DbI7 = &ksr.KernelDbI7;

    PULONGLONG  DbD0 = &ksr.KernelDbD0;
    PULONGLONG  DbD1 = &ksr.KernelDbD1;
    PULONGLONG  DbD2 = &ksr.KernelDbD2;
    PULONGLONG  DbD3 = &ksr.KernelDbD3;
    PULONGLONG  DbD4 = &ksr.KernelDbD4;
    PULONGLONG  DbD5 = &ksr.KernelDbD5;
    PULONGLONG  DbD6 = &ksr.KernelDbD6;
    PULONGLONG  DbD7 = &ksr.KernelDbD7;
#else
    CONTEXT     Context;
    PULONGLONG  DbI0 = &Context.DbI0;
    PULONGLONG  DbI1 = &Context.DbI1;
    PULONGLONG  DbI2 = &Context.DbI2;
    PULONGLONG  DbI3 = &Context.DbI3;
    PULONGLONG  DbI4 = &Context.DbI4;
    PULONGLONG  DbI5 = &Context.DbI5;
    PULONGLONG  DbI6 = &Context.DbI6;
    PULONGLONG  DbI7 = &Context.DbI7;

    PULONGLONG  DbD0 = &Context.DbD0;
    PULONGLONG  DbD1 = &Context.DbD1;
    PULONGLONG  DbD2 = &Context.DbD2;
    PULONGLONG  DbD3 = &Context.DbD3;
    PULONGLONG  DbD4 = &Context.DbD4;
    PULONGLONG  DbD5 = &Context.DbD5;
    PULONGLONG  DbD6 = &Context.DbD6;
    PULONGLONG  DbD7 = &Context.DbD7;
#endif

    // IA64 currently only supports 4
    assert(Register == 4);


#ifdef KERNEL
    if (GetExtendedContext(hthd, &ksr))
#else
    Context.ContextFlags = CONTEXT_DEBUG;
    if (GetThreadContext(hthd->rwHand, &Context))
#endif
    {

        switch( Register ) {
          case 0:
            *DbI0         = 0;
            *DbI1         = 0;
            *DbD0         = 0;
            *DbD1         = 0;
            break;

          case 1:
            *DbI2         = 0;
            *DbI3         = 0;
            *DbD2         = 0;
            *DbD3         = 0;
            break;

          case 3:
            *DbI4         = 0;
            *DbI5         = 0;
            *DbD4         = 0;
            *DbD5         = 0;
            break;

          case 4:
            *DbI6         = 0;
            *DbI7         = 0;
            *DbD6         = 0;
            *DbD7         = 0;
            break;
        }

#ifdef KERNEL
        SetExtendedContext(hthd, &ksr);
#else
        DPRINT(0, (_T("Setting Thread Context in clear debug register for thread %x:%i\n"), 
               hthd->tid,
               SetThreadContext(hthd->rwHand, &Context)
               ));
#endif
    }
}


BOOL
DecodeSingleStepEvent(
    HTHDX           hthd,
    DEBUG_EVENT64   *de,
    PDWORD          eventCode,
    PDWORD_PTR       subClass
    )
/*++

Routine Description:


Arguments:

    hthd    - Supplies thread that has a single step exception pending

    de      - Supplies the DEBUG_EVENT structure for the exception

    eventCode - Returns the remapped debug event id

    subClass - Returns the remapped subClass id


Return Value:

    TRUE if event was a real single step or was successfully mapped
    to a breakpoint.  FALSE if a register breakpoint occurred which was
    not expected.

--*/
{
    return TRUE;
    // v-vadimp - below is the x86 code kept here for reference purpose, all it does is checking Dr6 to see if we had a single step exception,
    // or a data BP has fired. I am not sure if this is still necessary for x86, and for Merced presumably EXCEPTION_SINGLE_STEP subclass
    // uniquely identifies a sinlge step fault. We'll see
#if 0  
    // most register walk routines are implemented in non-portable x86 codes.
    // i.e., heavy dependence on dr6. DO NOT TURN ON unless all references to 
    // dr6 are removed.
    DWORD       dr6;
    PBREAKPOINT bp;

#ifdef KERNEL

    KSPECIAL_REGISTERS ksr;

    GetExtendedContext( hthd, &ksr);
    dr6 = ksr.KernelDr6;

#else

    CONTEXT     Context;

    Context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    DbgGetThreadContext( hthd, &Context);
    dr6 = Context.Dr6;

#endif

    //
    // if it was a single step, look no further:
    //

    if ((dr6 & 0x4000) != 0) {
        return TRUE;
    }

    //
    //  Search for a matching walk...
    //

    bp = GetWalkBPFromBits(hthd, (dr6 & 0xf));

    if (bp) {
        de->dwDebugEventCode = *eventCode = BREAKPOINT_DEBUG_EVENT;
        de->u.Exception.ExceptionRecord.ExceptionCode = *subClass = (DWORD)bp;
        return TRUE;
    } else {
        return FALSE;
    }
    return FALSE;
#endif
}

// v-vadimp for IA64

#ifndef KERNEL

UOFFSET
GetSPFromNLGDest(
    HTHDX hthd,
    LPNLG_DESTINATION pNlgDest
    )
{
    UOFFSET dwRet;

    switch (pNlgDest->dwCode) {
        case NLG_CATCH_ENTER: // Catch handler
        case NLG_EXCEPT_ENTER: // Exception handler
        case NLG_FILTER_ENTER: // Exception filter
        case NLG_CATCH_LEAVE: // Return from Catch
        case NLG_FINALLY_ENTER: // Termination handlers
        case NLG_DESTRUCTOR_ENTER: // -GX handler
        case NLG_LONGJMPEX: // Exception safe longjmp
            dwRet = pNlgDest->uoffFramePointer;
            break;

        case NLG_LONGJMP:
            dwRet = pNlgDest->uoffFramePointer+1;
            break;

        default:
            dwRet = STACK_POINTER(hthd)+1; // emulate Virtual FP?
            break;
    }

        return dwRet;
}

void* InfoExceptionDuringStep(
    HTHDX hthd
    )
{
    Unreferenced(hthd);

    // Information that needs to be propagated from the step location
    // to the action handler when an exception occurs is passed in the
    // void * returned by this function. In case of IA64 no information
    // needs to be passed currently.
    
    // v-vadimp: may need code here
    
    return NULL;
}

BOOL
GetWndProcMessage(
    HTHDX   hthd,
    UINT*   pmsg
    )
/*++

Routine Description:

    This function is used to get the current Windows message (WM_CREATE, etc)
    when execution has been stopped at a wndproc.

Return Value:

    False on failure; True otherwise.

--*/
{
    // v-vadimp - note: windbg uses a dufferent method of retrieveing a window message, so this is untested
    ADDR addr; 
    ULONGLONG ulStackedRegs[2];
    ULONG cb;
    // read the back storage
    AddrInit(&addr, 0, 0, hthd->context.RsBSP, hthd->fAddrIsFlat, hthd->fAddrOff32, 0, hthd->fAddrIsReal);
    if (AddrReadMemory(hthd->hprc,hthd,&addr,(LPVOID)&ulStackedRegs,2,&cb) == xosdNone) 
    {
        *pmsg = (UINT)ulStackedRegs[1];  // window messages come in R33 - second stacked register
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

DWORDLONG
GetFunctionResult(
    PCALLSTRUCT pcs
    )
{
    return pcs->context.IntV0;
}

VOID
vCallFunctionHelper(
    HTHDX hthd,
    DWORDLONG lpFunction,
    int cArgs,
    va_list vargs
    )
{
    ULONG NewBspStore;
    ULONGLONG ArgCopy[96];
    ULONG ShiftCount;
    BOOLEAN RnatSaved = FALSE;
    DWORD ArgCount = 0;
    int i;

    assert(!"v-vadimp:vCallFunctionHelper not tested");
    
    //
    // set up the args to IA64 software convention defines the 
    // parameter passing via backing store(r32-r39). Prepare argument copy
    // with current thread handle and RNAT if its backing store address [10:3]
    // are all 1.
    //

    NewBspStore = (ULONG)hthd->context.RsBSPSTORE;

    assert(cArgs < 96);

    for (i = 0; i < min(cArgs,96); i++) { //v-vadimp - spill the arguments

        ArgCopy[ArgCount++] = va_arg(vargs, DWORD); // dump the argument itself
        NewBspStore += sizeof(ULONGLONG);                 // increment the backstore pointer
        ShiftCount = (NewBspStore & 0x1F8) >> 3;          // calculate the position for the argument's NAT bit in the RNAT (0x1f8 COMES FROM 63*8 - 63 arguments, then the RNAT)
        hthd->context.RsRNAT &= ~(0x1 << ShiftCount);     // put 1 there (we have no exception)

        if ((NewBspStore & 0x1F8) == 0x1F8)  // if it's time to spill the RNATm do it
        {
            ArgCopy[ArgCount++] = hthd->context.RsRNAT;
            NewBspStore += sizeof(ULONGLONG);
        }
    }
        

    // Copy the argument onto the backing store

    AddrWriteMemory(hthd->hprc, 
                      hthd,
                      (LPADDR)hthd->context.RsBSPSTORE,
                      ArgCopy, 
                      ArgCount * sizeof(ULONGLONG),
                      &i);
    //
    // zero out loadrs, adjust RseStack
    //

    hthd->context.RsRSC = (RSC_MODE_LY<<RSC_MODE)
                       | (RSC_BE_LITTLE<<RSC_BE) 
                       | (0x3<<RSC_PL);


    // 
    // Setup argument numbers
    //

    hthd->context.StIFS = (ULONGLONG)ArgCount;    // # of arguments/size of frame(sof)
    hthd->context.RsBSP = hthd->context.RsBSPSTORE = NewBspStore;

    // Set the address of the target code into IIP and IPSR.ri; save the current IIP in RP. 
    // No change to Sp and Ap.

    hthd->context.BrRp = PC(hthd);
    hthd->context.StIIP = lpFunction;
    hthd->context.StIPSR &= ~((ULONGLONG)0x3 << PSR_RI);
    hthd->context.StIPSR |= ((hthd->context.StIIP & 0xf) >> 2) << PSR_RI;
    hthd->context.StIIP &= 0xf;
    hthd->fContextDirty = TRUE;
}

#endif

ULONGLONG
GetRegValue(
    PCONTEXT regs,
    int cvindex
    )
{
    switch (cvindex)
    {
    case CV_IA64_IntT0: 
            return regs->IntT0;                        
            break;
    case CV_IA64_IntT1:
            return regs->IntT1;
            break;
    case CV_IA64_IntS0:
            return regs->IntS0;                        
            break;
    case CV_IA64_IntS1:
            return regs->IntS1;
            break;
    case CV_IA64_IntS2:
            return regs->IntS2;
            break;
    case CV_IA64_IntS3:
            return regs->IntS3;
            break;
    case CV_IA64_IntV0:
            return regs->IntV0;
            break;
    case CV_IA64_IntT2:
            return regs->IntT2;
            break;
    case CV_IA64_IntT3:
            return regs->IntT3;
            break;
    case CV_IA64_IntSp:
            return regs->IntSp;
            break;
    case CV_IA64_IntTeb:
            return regs->IntTeb;
            break;
    case CV_IA64_IntT5:
            return regs->IntT5;
            break;
    case CV_IA64_IntT6:
            return regs->IntT6;
            break;
    case CV_IA64_IntT7:
            return regs->IntT7;
            break;
    case CV_IA64_IntT8:
            return regs->IntT8;
            break;
    case CV_IA64_IntT9:
            return regs->IntT9;
            break;
    case CV_IA64_IntT10:
            return regs->IntT10;
            break;
    case CV_IA64_IntT11:
            return regs->IntT11;
            break;
    case CV_IA64_IntT12:
            return regs->IntT12;
            break;
    case CV_IA64_IntT13:
            return regs->IntT13;
            break;
    case CV_IA64_IntT14:
            return regs->IntT14;
            break;
    case CV_IA64_IntT15:
            return regs->IntT15;
            break;
    case CV_IA64_IntT16:
            return regs->IntT16;
            break;
    case CV_IA64_IntT17:
            return regs->IntT17;
            break;
    case CV_IA64_IntT18:
            return regs->IntT18;
            break;
    case CV_IA64_IntT19:
            return regs->IntT19;
            break;
    case CV_IA64_IntT20:
            return regs->IntT20;
            break;
    case CV_IA64_IntT21:
            return regs->IntT21;
            break;
    case CV_IA64_IntT22:
            return regs->IntT22;
            break;
    case CV_IA64_IntNats:
            return regs->IntNats;                      
            break;
    case CV_IA64_Preds:
            return regs->Preds;                       
            break;
    default:
            assert(!"GetRegValue called with unrecognized index");
            return (ULONGLONG)0 - 1;
    }
}



