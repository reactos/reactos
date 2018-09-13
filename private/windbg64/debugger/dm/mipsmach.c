/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    mipsmach.c

Abstract:

    This file contains the MIPS specific code for dealing with
    machine dependent issues that invlove registers, instruction
    disassembly, function calling and other interesting things.

Author:

    Jim Schaad (jimsch)

Environment:

    Win32 - User

Notes:

--*/

#include "precomp.h"

extern LPDM_MSG LpDmMsg;

extern CRITICAL_SECTION csContinueQueue;

typedef enum _MIPS_InstrType
   {
   mips_itype_Imm,
   mips_itype_Jump,
   mips_itype_Reg,
   mips_itype_dont_care
   }
MIPS_InstrType;



BOOL
IsRet(
    HTHDX hthd,
    LPADDR addr
    )
{
    DWORD instr;
    DWORD cBytes;
    if (!AddrReadMemory( hthd->hprc, hthd, addr, &instr, 4, &cBytes )) {
        return FALSE;
    }
    return (instr == 0x03e00008); // JR r31
}


void
IsCall (
    HTHDX   hthd,
    LPADDR  lpaddr,
    LPINT   lpf,
    BOOL    fStepOver
    )

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

Return Value:

    None.

--*/

{
    ULONG   opcode;
    ADDR    firaddr = *lpaddr;
    DWORD   length;
    ULONGLONG   *regArray = &hthd->context.XIntZero;
    INSTR   disinstr;
    BOOL    r;



    if (hthd->fIsCallDone) {
        *lpaddr = hthd->addrIsCall;
        *lpf = hthd->iInstrIsCall;
        return;
    }

    /*
     *  Assume that this is not a call instruction
     */

    *lpf = FALSE;

    /*
     *  Read in the dword which contains the instruction under
     *  inspection.
     */

    r = AddrReadMemory(hthd->hprc,
                       hthd,
                       &firaddr,
                       &disinstr.instruction,
                       sizeof(DWORD),
                       &length );
    if (!r || length != sizeof(DWORD)) {
        goto done;
    }

    /*
     *  Assume that this is a jump instruction and get the opcode.
     *  This is the top 6 bits of the instruction dword.
     */

    opcode = disinstr.jump_instr.Opcode;

    /*
     *  The first thing to check for is the SPECIAL instruction.
     *
     *   BREAK and JALR
     */

    if (opcode == 0x00L) {
        /*
         *  There are one opcode in the SPECIAL range which need to
         *      be treaded specially.
         *
         *      BREAK:
         *         If the value is 0x16 then this was a "breakpoint" set
         *         by the debugger.  Other values represent different
         *         exceptions which were programmed in by the code writer.
         */

        if (disinstr.break_instr.Function == 0x0D) {
            if (disinstr.break_instr.Code == 0x16) {
                *lpf = INSTR_BREAKPOINT;
            } else {
                *lpf = INSTR_SOFT_INTERRUPT;
            }
        } else if (disinstr.special_instr.Funct == 0x09L) {
            *lpf = INSTR_IS_CALL;
        }
    }

    /*
     *  Next item is REGIMM
     *
     *          BLTZAL, BGEZAL, BLTZALL, BGEZALL
     */

    else if (opcode == 0x01L) {
        if (((disinstr.immed_instr.RT & ~0x3) == 0x10) &&

            ((((LONGLONG)regArray[disinstr.immed_instr.RS]) >= 0) ==
             (BOOL)(disinstr.immed_instr.RT & 0x01))) {

            *lpf = INSTR_IS_CALL;
        }
    }

    /*
     *  Next item is JAL
     */

    else if (opcode == 0x03) {
        *lpf = INSTR_IS_CALL;
    }

    DPRINT(1, ("(IsCall?) FIR=%08x Type=%s\n", GetAddrOff(firaddr),
               (*lpf==INSTR_IS_CALL?           "CALL":
                (*lpf==INSTR_BREAKPOINT?       "BREAKPOINT":
                 (*lpf==INSTR_SOFT_INTERRUPT?  "INTERRUPT":
                                               "NORMAL"))) ) );

done:
    if (*lpf==INSTR_IS_CALL) {
        lpaddr->addr.off += BP_SIZE + DELAYED_BRANCH_SLOT_SIZE;
        hthd->addrIsCall = *lpaddr;
    } else if ( *lpf==INSTR_SOFT_INTERRUPT ) {
        lpaddr->addr.off += BP_SIZE;
    }
    hthd->iInstrIsCall = *lpf;

    return;
}                               /* IsCall() */


#if 0

ULONG
GetNextOffset (
    HTHDX hthd,
    BOOL fStep
    )

/*++

Routine Description:

    From a limited disassembly of the instruction pointed
    by the FIR register, compute the offset of the next
    instruction for either a trace or step operation.

Arguments:

    hthd  - Supplies the handle to the thread to get the next offset for

    fStep - Supplies TRUE for STEP offset and FALSE for trace offset

Return Value:

    Offset to place breakpoint at for doing a STEP or TRACE

--*/

{
    ULONG   returnvalue;
    ULONG   opcode;
    ADDR    firaddr;
    DWORD   length;
    ULONGLONG   *regArray = &hthd->context.XIntZero;
    INSTR   disinstr;
    BOOL    r;

    AddrFromHthdx(&firaddr, hthd);
    r = AddrReadMemory(hthd->hprc,
                       hthd,
                       &firaddr,
                       &disinstr.instruction,
                       sizeof(DWORD),
                       &length);
    if (!r || length != sizeof(DWORD)) {
        DPRINT(1, ("GetNextOffset: AddrReadMemory failed %I64x\n", GetAddrOff(firaddr)) );
        assert(FALSE);
        return 4;
    }
    opcode = disinstr.jump_instr.Opcode;
    returnvalue = firaddr.addr.off + sizeof(ULONG) * 2; /* assume delay slot */

    if (disinstr.instruction == 0x0000000c) {
        // stepping over a syscall instruction must set the breakpoint
        // at the caller's return address, not the inst after the syscall
        returnvalue = (DWORD)hthd->context.XIntRa;
    }
    else
    if (opcode == 0x00L       /* SPECIAL */
        && (disinstr.special_instr.Funct & ~0x01L) == 0x08L) {
        /* jr/jalr only */
        if (disinstr.special_instr.Funct == 0x08L || !fStep) /* jr or trace */
          returnvalue = (DWORD)regArray[disinstr.special_instr.RS];
    }
    else if (opcode == 0x01L) {

        /*
         *  For BCOND opcode, RT values 0x00 - 0x03, 0x10 - 0x13
         *  are defined as conditional jumps.  A 16-bit relative
         *  offset is taken if:
         *
         *    (RT is even and (RS) < 0  (0x00 = BLTZ,   0x02 = BLTZL,
         *               0x10 = BLTZAL, 0x12 = BLTZALL)
         *     OR
         *     RT is odd and (RS) >= 0  (0x01 = BGEZ,   0x03 = BGEZL
         *               0x11 = BGEZAL, 0x13 = BGEZALL))
         *  AND
         *    (RT is 0x00 to 0x03       (BLTZ BGEZ BLTZL BGEZL non-linking)
         *     OR
         *     fStep is FALSE       (linking and not stepping over))
         */

        if (((disinstr.immed_instr.RT & ~0x13) == 0x00) &&
            (((LONGLONG)regArray[disinstr.immed_instr.RS] >= 0) ==
             (BOOL)(disinstr.immed_instr.RT & 0x01)) &&
            (((disinstr.immed_instr.RT & 0x10) == 0x00) || !fStep))
          returnvalue = ((LONG)(SHORT)disinstr.immed_instr.Value << 2)
            + firaddr.addr.off + sizeof(ULONG);
    }

    else if ((opcode & ~0x01L) == 0x02) {
        /*
         *  J and JAL opcodes (0x02 and 0x03).  Target is
         *  26-bit absolute offset using high four bits of the
         *  instruction location.  Return target if J opcode or
         *  not stepping over JAL.
         */

        if (opcode == 0x02 || !fStep)
          returnvalue = (disinstr.jump_instr.Target << 2)
            + (firaddr.addr.off & 0xf0000000);
    }

    else if ((opcode & ~0x11L) == 0x04) {
        /*  BEQ, BNE, BEQL, BNEL opcodes (0x04, 0x05, 0x14, 0x15).
         *  Target is 16-bit relative offset to next instruction.
         *  Return target if (BEQ or BEQL) and (RS) == (RT),
         *  or (BNE or BNEL) and (RS) != (RT).
         */

        if ((BOOL)(opcode & 0x01) ==
            (BOOL)(regArray[disinstr.immed_instr.RS] !=
                   regArray[disinstr.immed_instr.RT]))
          returnvalue = ((LONG)(SHORT)disinstr.immed_instr.Value << 2)
            + firaddr.addr.off + sizeof(ULONG);
    }
    else if ((opcode & ~0x11L) == 0x06) {
        /*  BLEZ, BGTZ, BLEZL, BGTZL opcodes (0x06, 0x07, 0x16, 0x17).
         *  Target is 16-bit relative offset to next instruction.
         *  Return target if (BLEZ or BLEZL) and (RS) <= 0,
         *  or (BGTZ or BGTZL) and (RS) > 0.
         */
        if ((BOOL)(opcode & 0x01) ==
            (BOOL)((LONGLONG)regArray[disinstr.immed_instr.RS] > 0))
          returnvalue = ((LONG)(SHORT)disinstr.immed_instr.Value << 2)
            + firaddr.addr.off + sizeof(ULONG);
    }
    else if (opcode == 0x11L
             && (disinstr.immed_instr.RS & ~0x04L) == 0x08L
             && (disinstr.immed_instr.RT & ~0x03L) == 0x00L) {

        /*  COP1 opcode (0x11) with (RS) == 0x08 or (RS) == 0x0c and
         *  (RT) == 0x00 to 0x03, producing BC1F, BC1T, BC1FL, BC1TL
         *  instructions.  Return target if (BC1F or BC1FL) and floating
         *  point condition is FALSE or if (BC1T or BC1TL) and condition TRUE.
         *
         *  NOTENOTE - JLS -- I don't know that this is correct. rs = 0x3
         *              will also use CP3
         */

//  if ((disinstr.immed_instr.RT & 0x01) == GetRegFlagValue(FLAGFPC))
        if ((disinstr.immed_instr.RT & 0x01) == ((hthd->context.XFsr>>23)&1)) {
            returnvalue = ((LONG)(SHORT)disinstr.immed_instr.Value << 2)
                        + firaddr.addr.off + sizeof(ULONG);
        }
    }
    else {
        returnvalue -= sizeof(ULONG); /* remove delay slot */
    }

    return returnvalue;
}                               /* GetNextOffset() */
#endif // 0


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
#ifdef OSDEBUG4
        return xosdBadThread;
#else
        return xosdInvalidThread;
#endif
    }

    /*
     * Now get the current stack offset.
     */

    lpeo->addrStack.addr.off = (DWORD)lpeo->hthd->context.XIntSp;

    /*
     * Now place the return address correctly
     */

    lpeo->hthd->context.XFir = (LONG)lpeo->addrStart.addr.off;
    lpeo->hthd->context.XIntRa = (LONG)lpeo->addrStart.addr.off;

    /*
     * Set the instruction pointer to the starting addresses
     *  and write the context back out
     */

    lpeo->hthd->context.XFir = (LONG)lpeo->addrStart.addr.off;

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

    if (lpeo->addrStack.addr.off <= (DWORD)lpeo->hthd->context.XIntSp) {
        return TRUE;
    }

    return FALSE;
}                               /* CompareStacks() */



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

PVOID
InfoExceptionDuringStep(
    HTHDX hthd
    )
{
    Unreferenced(hthd);

    // Information that needs to be propagated from the step location
    // to the action handler when an exception occurs is passed in the
    // void * returned by this function. In case of Mips no information
    // needs to be passed currently.
    return NULL;
}

DWORDLONG
GetFunctionResult(
    PCALLSTRUCT pcs
    )
{
    return pcs->context.XIntV0;
}


VOID
vCallFunctionHelper(
    HTHDX hthd,
    FARPROC lpFunction,
    int cArgs,
    va_list vargs
    )
{
    int i;
    
    assert(Is64PtrSE(lpFunction));

    for (i = 0; i < cArgs; i++) {
        (&hthd->context.XIntA0)[i] = va_arg(vargs, DWORD);
    }

    hthd->context.XIntRa = PC(hthd);
    Set_PC(hthd, lpFunction);
    hthd->fContextDirty = TRUE;
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
    *pmsg = (UINT)hthd->context.XIntA1;
    return TRUE;
}

#endif

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


#if 0


DWORD
BranchUnassemble(
    void   *Memory,
    ADDR   *Addr,
    BOOL   *IsBranch,
    BOOL   *TargetKnown,
    BOOL   *IsCall,
    BOOL   *IsTable,
    ADDR   *Target
    )
{
    ULONG   OpCode;
    INSTR  *Instr;
    UOFF32  Offset;
    UOFF32  TargetOffset;

    MIPS_InstrType  itype = mips_itype_dont_care;

    assert( Memory );
    assert( IsBranch );
    assert( TargetKnown );
    assert( IsCall );
    assert( Target );

    Offset       = GetAddrOff( *Addr );
    TargetOffset = 0;
    *IsBranch    = FALSE;
    *IsTable     = FALSE;

    Instr     = (INSTR *)Memory;
    OpCode    = Instr->jump_instr.Opcode;

    switch ( OpCode ) {

        case 0x00L:
            //
            //  Special
            //
            switch ( Instr->special_instr.Funct ) {

                case 0x09L:
                    //
                    //  JALR
                    //
                    *IsBranch    = TRUE;
                    *IsCall      = TRUE;
                    *TargetKnown = FALSE;
                    itype        = mips_itype_dont_care;
                    break;

                case 0x08L:
                    //
                    //  JR
                    //
                    *IsBranch    = TRUE;
                    *IsCall      = FALSE;
                    *TargetKnown = FALSE;
                    itype        = mips_itype_dont_care;
                    break;
            }
            break;

        case 0x03:
            //
            //  JAL
            //
            *IsBranch    = TRUE;
            *IsCall      = TRUE;
            *TargetKnown = TRUE;
            itype        = mips_itype_Jump;
            break;

        case 0x02:
            //
            //  J
            //
            *IsBranch    = TRUE;
            *IsCall      = FALSE;
            *TargetKnown = TRUE;
            itype        = mips_itype_Jump;
            break;

        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
            //
            //  BCz
            //
            *IsBranch    = TRUE;
            *IsCall      = FALSE;
            *TargetKnown = TRUE;
            itype        = mips_itype_Imm;
            break;

        case 0x04:  // BEQ
        case 0x14:  // BEQL
        case 0x05:  // BNE
        case 0x15:  // BNEL
            *IsBranch    = TRUE;
            *IsCall      = FALSE;
            *TargetKnown = TRUE;
            itype        = mips_itype_Imm;
            break;

        case 0x01:
            //
            //  REGIMM
            //
            itype = mips_itype_Imm;

            switch ( Instr->immed_instr.RT ) {

                case 0x00:  // BLTZ
                case 0x01:  // BGEZ
                case 0x02:  // BLTZL
                case 0x03:  // BGEZL
                    *IsBranch    = TRUE;
                    *IsCall      = FALSE;
                    *TargetKnown = TRUE;
                    break;

                case 0x10:  // BLTZAL
                case 0x11:  // BGEZAL
                case 0x12:  // BLTZALL
                case 0x13:  // BGEZALL
                    *IsBranch    = TRUE;
                    *IsCall      = TRUE;
                    *TargetKnown = TRUE;
                    break;

            }
            break;

        case 0x07: // BGTZ  ?
        case 0x17: // BGTZL ?
        case 0x06: // BLEZ  ?
        case 0x16: // BLEZL ?
            if ( Instr->immed_instr.RT == 0x00 ) {
                *IsBranch    = TRUE;
                *IsCall      = FALSE;
                *TargetKnown = TRUE;
                itype = mips_itype_Imm;
            }
            break;

        default:
            break;
    }

   if (*TargetKnown)
      {
      switch (itype)
         {
         default:
         case (mips_itype_Reg):
         case (mips_itype_dont_care):
            break;

         case (mips_itype_Imm):
            TargetOffset
               = (UOFF32)(LONG)((SHORT)Instr->immed_instr.Value << 2 )
                 +
                 (Offset + sizeof(DWORD));
            break;

         case (mips_itype_Jump):
            TargetOffset = (UOFF32)(Instr->jump_instr.Target << 2 )
                           |
                           ((Offset + sizeof(DWORD)) & 0xF0000000);
            break;
         }
    }

    AddrInit( Target, 0, 0, TargetOffset, TRUE, TRUE, FALSE, FALSE );

    return sizeof( DWORD );
}

#endif
/********************************************************

note: "z" refers to a co-processor, with value = 0, 1, 2, or 3

   instr               opcode      rs/rt/imm         (Imm)
   mnemonic  op  type  mnemonic    rs/rt/rd/sa/fn    (Reg)
   --------  --  ----  --------    --/--/--/--/---
   ADD       00  Reg   SPECIAL     xx/xx/xx/00/20
   ADDI      08  Imm   ADDI        xx/xx/xxxx
   ADDIU     09  Imm   ADDIU       xx/xx/xxxx
   ADDU      00  Reg   SPECIAL     xx/xx/xx/00/21
   AND       00  Reg   SPECIAL     xx/xx/xx/00/24
   ANDI      0a  Imm   ANDI        xx/xx/xxxx
 * BCzF      1z  Imm   COPz        08/00/xxxx
 * BCzFL     1z  Imm   COPz        08/02/xxxx
 * BCzT      1z  Imm   COPz        08/01/xxxx
 * BCzTL     1z  Imm   COPz        08/03/xxxx
 * BEQ       04  Imm   BEQ         xx/xx/xxxx
 * BEQL      14  Imm   BEQL        xx/xx/xxxx
 * BGEZ      01  Imm   REGIMM      xx/01/xxxx
 * BGEZAL    01  Imm   REGIMM      xx/11/xxxx
 * BGEZALL   01  Imm   REGIMM      xx/13/xxxx
 * BGEZL     01  Imm   REGIMM      xx/03/xxxx
 * BGTZ      07  Imm   BGTZ        xx/00/xxxx
 * BGTZL     17  Imm   BGTZL       xx/00/xxxx
 * BLEZ      06  Imm   BLEZ        xx/00/xxxx
 * BLEZL     16  Imm   BLEZL       xx/00/xxxx
 * BLTZ      01  Imm   REGIMM      xx/00/xxxx
 * BLTZAL    01  Imm   REGIMM      xx/10/xxxx
 * BLTZALL   01  Imm   REGIMM      xx/12/xxxx
 * BLTZL     01  Imm   REGIMM      xx/02/xxxx
 * BNE       05  Imm   BNE         xx/xx/xxxx
 * BNEL      15  Imm   BNEL        xx/xx/xxxx
   BREAK     00  ?     SPECIAL     xx/xx/xx/xx/0c
   CACHE     2f  Imm   CACHE       xx/xx/xxxx
   CFCz      1z  ?     COPz        02/xx/xx/xx/00
 ? COPz      1z  ?     COPz        1x/xx/xxxx
   CTCz      1z  Imm   COPz        06/xx/xx/??/00
   DADD      00  Reg   SPECIAL     xx/xx/xx/00/2c
   DADDI     18  Imm   DADDI       xx/xx/xxxx
   DADDIU    19  Imm   DADDIU      xx/xx/xxxx
   DADDU     00  Reg   SPECIAL     xx/xx/xx/00/2d
   DDIV      00  Reg   SPECIAL     xx/xx/00/00/1e
   DDIVU     00  Reg   SPECIAL     xx/xx/00/00/1f
   DIV       00  Reg   SPECIAL     xx/xx/00/00/1a
   DIVU      00  Imm   SPECIAL     xx/xx/00/00/1b
   DMFC0     10  Reg   COP0        01/xx/xx/00/00
   DMFC0     10  Reg   COP0        05/xx/xx/00/00
   DMULT     00  Reg   SPECIAL     xx/xx/00/00/1c
   DMULTU    00  Reg   SPECIAL     xx/xx/00/00/1d
   DSLL      00  Reg   SPECIAL     00/xx/xx/xx/38
   DSLLV     00  Reg   SPECIAL     00/xx/xx/xx/14
   DSLL32    00  Reg   SPECIAL     00/xx/xx/xx/3c
   DSRA      00  Reg   SPECIAL     00/xx/xx/xx/3b
   DSRAV     00  Reg   SPECIAL     00/xx/xx/xx/17
   DSRA32    00  Reg   SPECIAL     00/xx/xx/xx/3f
   DSRL      00  Reg   SPECIAL     00/xx/xx/xx/3a
   DSRLV     00  Reg   SPECIAL     00/xx/xx/xx/16
   DSRL32    00  Reg   SPECIAL     00/xx/xx/xx/3e
   DSUB      00  Reg   SPECIAL     xx/xx/xx/00/2e
   DSUBU     00  Reg   SPECIAL     xx/xx/xx/00/2f
   ERET      10  Reg   COP0        10/00/00/00/28
 * J         02  Jump  J           xxxxxxx
 * JAL       03  Jump  JAL         xxxxxxx
 * JALR      00  Reg   SPECIAL     xx/00/xx/00/09
 * JR        00  Reg   SPECIAL     xx/00/00/00/08
   LB        30  Imm   LB          xx/xx/xxxx
   LBU       34  Imm   LBU         xx/xx/xxxx
   LD        37  Imm   LD          xx/xx/xxxx
   LDCz      3?  Imm   LDCz        xx/xx/xxxx
   LDL       1a  Imm   LDL         xx/xx/xxxx
   LDR       1b  Imm   LDR         xx/xx/xxxx
   LH        21  Imm   LH          xx/xx/xxxx
   LHU       25  Imm   LHU         xx/xx/xxxx
   LL        30  Imm   LL          xx/xx/xxxx
   LLD       34  Imm   LLD         xx/xx/xxxx
   LUI       1f  Imm   LUI         00/xx/xxxx
   LW        23  Imm   LW          xx/xx/xxxx
   LWCz      3z  Imm   LWCz        xx/xx/xxxx
   LWL       22  Imm   LWL         xx/xx/xxxx
   LWR       25  Imm   LWR         xx/xx/xxxx
   LWU       2f  Imm   LWU         xx/xx/xxxx
   MFC0      10  Reg   COP0        00/xx/xx/00/00
   MFCz      1z  Reg   COPz        00/xx/xx/00/00
   MFHI      00  Reg   SPECIAL     00/00/xx/00/10
   MFLO      00  Reg   SPECIAL     00/00/xx/00/12
   MTC0      10  Reg   COP0        04/xx/xx/00/00
   MTCz      1z  Reg   COPz        04/xx/xx/00/00
   MTHI      00  Reg   SPECIAL     xx/00/00/00/11
   MTLO      00  Reg   SPECIAL     xx/00/00/00/13
   MULT      00  Reg   SPECIAL     xx/xx/00/00/18
   MULTU     00  Reg   SPECIAL     xx/xx/00/00/19
   NOR       00  Reg   SPECIAL     xx/xx/xx/00/37
   OR        00  Reg   SPECIAL     xx/xx/xx/00/35
   ORI       0d  Imm   ORI         xx/xx/xxxx
   SB        28  Imm   SB          xx/xx/xxxx
   SC        38  Imm   SC          xx/xx/xxxx
   SCD       3c  Imm   SCD         xx/xx/xxxx
   SCDz      3?  Imm   SCDz        xx/xx/xxxx
   SDL       2c  Imm   SDL         xx/xx/xxxx
   SDR       2d  Imm   SDR         xx/xx/xxxx
   SH        29  Imm   SH          xx/xx/xxxx
   SLL       00  Reg   SPECIAL     xx/xx/xx/xx/00
   SLLV      00  Reg   SPECIAL     xx/xx/xx/xx/04
   SLT       00  Reg   SPECIAL     xx/xx/xx/xx/2a
   SLTI      0a  Imm   SLTI        xx/xx/xxxx
   SLTIU     0b  Imm   SLTIU       xx/xx/xxxx
   SLTU      00  Reg   SPECIAL     xx/xx/xx/xx/2b
   SRA       00  Reg   SPECIAL     00/xx/xx/xx/03
   SRAV      00  Reg   SPECIAL     xx/xx/xx/00/07
   SRL       00  Reg   SPECIAL     00/xx/xx/xx/02
   SRLV      00  Reg   SPECIAL     xx/xx/xx/00/06
   SUB       00  Reg   SPECIAL     xx/xx/xx/00/22
   SUBU      00  Reg   SPECIAL     xx/xx/xx/00/23
   SW        2b  Imm   SW          xx/xx/xxxx
   SWCz      3?  Imm   SWCz        xx/xx/xxxx
   SWL       2a  Reg   SWL         xx/xx/xxxx
   SWR       2d  Reg   SWR         xx/xx/xxxx
   SYNC      00  Reg   SPECIAL     00/00/00/00/0f
   SYSCALL   00  Reg   SPECIAL     xx/xx/xx/xx/0c
   TEQ       00  Reg   SPECIAL     xx/xx/xx/xx/36
   TEQI      01  Imm   REGIMM      xx/0c/xxxx
   TGE       00  Reg   SPECIAL     xx/xx/xx/xx/30
   TGEI      01  Reg   REGIMM      xx/08/xxxx
   TGEIU     01  Imm   REGIMM      xx/09/xxxx
   TGEU      00  Reg   SPECIAL     xx/xx/xx/xx/31
   TLBP      10  Reg   COP0        10/00/00/00/08
   TLBR      10  Reg   COP0        10/00/00/00/01
   TLBWI     10  Reg   COP0        10/00/00/00/02
   TLBWR     10  Reg   COP0        10/00/00/00/06
   TLT       00  Reg   SPECIAL     xx/xx/xx/xx/32
   TLTI      01  Imm   REGIMM      xx/0a/xxxx
   TLTIU     01  Imm   REGIMM      xx/0b/xxxx
   TLTU      00  Reg   SPECIAL     xx/xx/xx/xx/33
   TNE       00  Reg   SPECIAL     xx/xx/xx/xx/3c
   TNEI      01  Imm   REGIMM      xx/0e/xxxx
   XOR       00  Reg   SPECIAL     xx/xx/xx/00/26
   XORI      0c  Imm   XORI        xx/xx/xxxx



********************************************************/

BOOL
DecodeSingleStepEvent(
    HTHDX           hthd,
    DEBUG_EVENT64    *de,
    PDWORD          eventCode,
    PDWORD          subClass
    )
/*++

Routine Description:


Arguments:

    hthd    - Supplies thread that has a single step exception pending

    de      - Supplies the DEBUG_EVENT64 structure for the exception

    eventCode - Returns the remapped debug event id

    subClass - Returns the remapped subClass id


Return Value:

    TRUE if event was a real single step or was successfully mapped
    to a breakpoint.  FALSE if a register breakpoint occurred which was
    not expected.

--*/
{
    return FALSE;
}

BOOL
CoerceContext64To32(
    PCONTEXT pContext
    )
{
    PULONGLONG Src;
    PULONG Dst;
    ULONG Index;

    //
    // If the target system is running a kernel with 64-bit addressing
    // enabled in user mode, then coerce the 64-bit context to 32-bits.
    //

    DPRINT(1, ("ConvertContext64To32: ContextFlags == %08x\n", pContext->ContextFlags));
    if ((pContext->ContextFlags & CONTEXT_EXTENDED_INTEGER) == CONTEXT_EXTENDED_INTEGER) {
        DPRINT(1, ("ConvertContext64To32: Ra == %016Lx\n", pContext->XIntRa));
        Src = &pContext->XIntZero;
        Dst = &pContext->IntZero;
        for (Index = 0; Index < 32; Index += 1) {
            *Dst++ = (ULONG)*Src++;
        }
        pContext->ContextFlags &= ~CONTEXT_EXTENDED_INTEGER;
        pContext->ContextFlags |= CONTEXT_INTEGER;
        return TRUE;
    }
    return FALSE;
}

BOOL
CoerceContext32To64 (
    PCONTEXT pContext
    )
{
    PULONG Src;
    PULONGLONG Dst;
    ULONG Index;

    //
    // If the target system is running a kernel with 64-bit addressing
    // enabled in user mode, then coerce the 32-bit context to 64-bits.
    //

    DPRINT(1, ("ConvertContext32To64: ContextFlags == %08x\n", pContext->ContextFlags));
    if ((pContext->ContextFlags & CONTEXT_EXTENDED_INTEGER) != CONTEXT_EXTENDED_INTEGER) {
        DPRINT(1, ("ConvertContext32To64: Ra == %08x\n", pContext->IntRa));
        Dst = &pContext->XIntZero;
        Src = &pContext->IntZero;
        for (Index = 0; Index < 32; Index += 1) {
            *Dst++ = (LONG)*Src++;
        }
        pContext->ContextFlags |= CONTEXT_EXTENDED_INTEGER;
        return TRUE;
    }
    return FALSE;
}

ULONGLONG
GetRegValue(
    PCONTEXT regs,
    int cvindex
    )
{
    if (cvindex < (CV_M4_IntZERO+32)) {
        return ((&regs->XIntZero)[cvindex - CV_M4_IntZERO]);
    }
    switch (cvindex) {
        case CV_M4_IntLO:
            return regs->XIntLo;

        case CV_M4_IntHI:
            return regs->XIntHi;

        case CV_M4_Fir:
            return regs->XFir;

        case CV_M4_Psr:
            return regs->XPsr;

        default:
            assert(!"GetRegValue called with unrecognized index");
            return (ULONGLONG)0 - 1;
    }
}
