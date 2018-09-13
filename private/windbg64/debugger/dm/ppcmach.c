/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ppcmach.c

Abstract:

    This file contains the PPC601 specific code for dealing with
    the process of stepping a single instruction.  This includes
    determination of the next offset to be stopped at and if the
    instruction is all call type instruction.

Author:

    Kent Forschmiedt (kentf)
    Farooq Butt (fmbutt@engage.sps.mot.com)

Environment:

    Win32 - User

Notes:

--*/

#include "precomp.h"
#pragma hdrstop

    //setup a couple of macros

    // The below macro is used to do subscripting operations
    // len_item is the length of the embedded word that we are
    // interested in subscripting

#define NTH_BIT(word,n,len_item) \
    ( ((word) >> ((len_item) - (n) - 1)) & 0x01)



//
// Stuff for debug registers
//
// The debug register architecture is represented to NT as
// nearly identical to the x86.
// As of this writing, there is one debug register, and it only
// supports a data length of 8.
//


typedef struct _DR7 *PDR7;
typedef struct _DR7 {
    DWORD       L0      : 1;
    DWORD       G0      : 1;
    DWORD       L1      : 1;
    DWORD       G1      : 1;
    DWORD       L2      : 1;
    DWORD       G2      : 1;
    DWORD       L3      : 1;
    DWORD       G3      : 1;
    DWORD       LE      : 1;
    DWORD       GE      : 1;
    DWORD       Pad1    : 3;
    DWORD       GD      : 1;
    DWORD       Pad2    : 1;
    DWORD       Pad3    : 1;
    DWORD       Rwe0    : 2;
    DWORD       Len0    : 2;
    DWORD       Rwe1    : 2;
    DWORD       Len1    : 2;
    DWORD       Rwe2    : 2;
    DWORD       Len2    : 2;
    DWORD       Rwe3    : 2;
    DWORD       Len3    : 2;
} DR7;


#define RWE_EXEC        0x00
#define RWE_WRITE       0x01
#define RWE_RESERVED    0x02
#define RWE_READWRITE   0x03


DWORD       LenMask[ MAX_DEBUG_REG_DATA_SIZE + 1 ] = DEBUG_REG_LENGTH_MASKS;


extern LPDM_MSG LpDmMsg;

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
    return (instr == 0x4e800020);   // bclr branch always
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

    fStepOver

Return Value:

    None.

--*/

{
    ULONG   opcode;
    ADDR    iaraddr = *lpaddr;
    DWORD   length;
    PPC_INSTRUCTION   disinstr;
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
                       &iaraddr,
                       &disinstr.Long,
                       sizeof(DWORD),
                       &length);
    if (!r || length != sizeof(DWORD)) {
        goto done;
    }


    opcode = disinstr.Primary_Op;

    /* Do we have a branch or is this a breakpoint ? If it is a
       breakpoint, is it set by the user or was it set by the
       debugger ? If all else fails return FALSE */

    switch (opcode)
    {
         default:
            DPRINT(5,("IsCall opcode == DEFAULT"));
            break; // leaving *lpf = FALSE


         case BC_OP:
            DPRINT(5,("IsCall opcode == BC_OP"));
            // branch conditional NEVER a call
            break; // leaving *lpf == FALSE

         case B_OP:
            DPRINT(5,("IsCall opcode == B_OP"));
            // unconditional branch, could be a call
            // THIS is the real call operation if LK == 1

            if ((disinstr.Long & 1) == 1)
            {
              // LK is on, we have a call
              *lpf = INSTR_IS_CALL;
            }

            break; // leaving *lpf = FALSE if not call...


         case X19_OP:
            DPRINT(5,("IsCall opcode == X19_OP"));
            // branch conditional on register (various extended opcodes)
            // This could be a function call if it is a
            // BCCTRL

            if ((disinstr.XLform_XO == BCCTR_OP) &&
                ((disinstr.Long & 1) == 1)) {
                *lpf = INSTR_IS_CALL;
            }

            //
            // don't call BLR a call instruction
            //
            if (disinstr.XLform_XO == BCLR_OP && disinstr.Long != 0x4e800020) {
                *lpf = INSTR_IS_CALL;
            }

            break; // leaving *lpf = FALSE if not BCCTRL

         case TWI_OP:
            DPRINT(5,("IsCall opcode == TWI_OP"));

            // Is this TWI instruction installed by the debugger or
            // was it a user installed one ?

            // First make sure this is a BREAK
            if (disinstr.Dform_TO == 0x1f) // All 1's in the TO field
            {
               switch(disinstr.Dform_SI)
                  {

                      case DEBUG_PRINT_BREAKPOINT:
                      case DEBUG_PROMPT_BREAKPOINT:
                      case DEBUG_STOP_BREAKPOINT:
                      case DEBUG_LOAD_SYMBOLS_BREAKPOINT:
                      case DEBUG_UNLOAD_SYMBOLS_BREAKPOINT:

                      *lpf = INSTR_BREAKPOINT;
                      DPRINT(5,("IsCall opcode was an INSTR_BREAKPOINT"));
                      break;

                      default:
                      *lpf = INSTR_SOFT_INTERRUPT;
                      DPRINT(5,("IsCall opcode was a INSTR_SOFT_INTERRUPT"));
                      break;
                  }
            }

    }


    DPRINT(1, ("(IsCall?) FIR=%08x Type=%s\n", iaraddr.addr.off,
               *lpf==INSTR_IS_CALL                  ?"CALL":
               (*lpf==INSTR_BREAKPOINT?"BREAKPOINT":
                (*lpf==INSTR_SOFT_INTERRUPT    ?"INTERRUPT":
                 "NORMAL"))));

done:
    if (*lpf==INSTR_IS_CALL) {
        lpaddr->addr.off += BP_SIZE;
        hthd->addrIsCall = *lpaddr;
    } else if ( *lpf==INSTR_SOFT_INTERRUPT ) {
        lpaddr->addr.off += BP_SIZE;
    }
    hthd->iInstrIsCall = *lpf;

    return;
}                               /* IsCall() */



#ifndef KERNEL
void
ProcessGetDRegsCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    LPDWORD   lpdw = (LPDWORD)LpDmMsg->rgb;
    CONTEXT   cxt;
    int       rs = 0;

    DEBUG_PRINT( "ProcessGetDRegsCmd :\n");


    if (hthd == 0) {
        rs = 0;
    } else {
        cxt.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        if (!GetThreadContext(hthd->rwHand, &cxt)) {
            LpDmMsg->xosdRet = xosdUnknown;
            rs = 0;
        } else {
            lpdw[0] = hthd->context.Dr0;
            lpdw[1] = hthd->context.Dr1;
            lpdw[2] = hthd->context.Dr2;
            lpdw[3] = hthd->context.Dr3;
            lpdw[4] = hthd->context.Dr6;
            lpdw[5] = hthd->context.Dr7;
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
    LPDWORD     lpdw = (LPDWORD)(lpdbb->rgbVar);
    XOSD        xosd = xosdNone;

    Unreferenced(hprc);

    DPRINT(5, ("ProcessSetDRegsCmd : "));

    hthd->context.ContextFlags = CONTEXT_DEBUG_REGISTERS;

    hthd->context.Dr0 = lpdw[0];
    hthd->context.Dr1 = lpdw[1];
    hthd->context.Dr2 = lpdw[2];
    hthd->context.Dr3 = lpdw[3];
    hthd->context.Dr6 = lpdw[4];
    hthd->context.Dr7 = lpdw[5];


    if (hthd->fWowEvent) {
        WOWSetThreadContext(hthd, &hthd->context);
    } else {
        SetThreadContext(hthd->rwHand, &hthd->context);
    }

    Reply(0, &xosd, lpdbb->hpid);

    return;
}                               /* ProcessSetDRegsCmd() */



DWORDLONG
GetFunctionResult(
    PCALLSTRUCT pcs
    )
{
    return pcs->context.Gpr3;
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
        (&hthd->context.Gpr3)[i] = va_arg(vargs, DWORD);
    }

    hthd->context.Lr = PC(hthd);
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
    *pmsg = (UINT)hthd->context.Gpr4;
    return TRUE;
}

#endif  // !KERNEL



ULONG
PpcGetNextOffset (
    HTHDX hthd,
    BOOL fStep
    )

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
    ULONG   returnvalue;
    ULONG   opcode;
    ADDR    iaraddr;
    DWORD   length;
    ULONG   *regArray = &hthd->context.Gpr0;
    PPC_INSTRUCTION   disinstr;
    ULONG    absolute;
    ULONG   cr,ctr,lr,cond_ok=0,ctr_ok=0;
    BOOL    r;

    AddrFromHthdx(&iaraddr, hthd);

    r = AddrReadMemory(hthd->hprc,
                       hthd,
                       &iaraddr,
                       &disinstr.Long,
                       sizeof(DWORD),
                       &length);

    opcode = disinstr.Primary_Op;

    DPRINT(5,("Entered GetNextOffset routine, the address we start with is "
              "0x%I64x\n\tThe instruction is 0x%x",
              iaraddr.addr.off,
              disinstr.Long));

    // setup default return value
    returnvalue = iaraddr.addr.off + sizeof(ULONG);


    // setup the absolute flag in case of a branch
    absolute = (int) ((disinstr.Long >> 1) & 1);

    // Before going into the switch, let us do some up front
    // calculations

   /* Let us use the algorithm described in pp 10-22 of
      the MPC/601 users manual */

     ctr = hthd->context.Ctr;
     cr  = hthd->context.Cr;

     /* First find out whether the CTR has to be decremented */

     if (NTH_BIT(disinstr.Bform_BO,2,5) == 0)
     // i.e if ~B0[2] then ctr = ctr - 1
       ctr = ctr - 1;

    // next we do the following operation:
    // ctr_ok = BO[2] OR ((ctr NEQ 0) XOR BO[3]))

    ctr_ok = (NTH_BIT(disinstr.Bform_BO,2,5) ||
             ((ctr != 0) ^ (NTH_BIT(disinstr.Bform_BO,3,5))));

    // now for
    // cond_ok= BO[0] OR ( (CR[BI] LEQIV BO[1]))

    cond_ok = ((NTH_BIT(disinstr.Bform_BO,0,5)) ||
               ((NTH_BIT(cr,(disinstr.Bform_BI),32)) ==
                (NTH_BIT(disinstr.Bform_BO,1,5))));


    switch (opcode)
    {

        case SC_OP:
            DPRINT(5,("We have an SC_OP"));
            // stepping over a syscall instruction must set the breakpoint
            // at the inst after the syscall (default)
            break;

        case B_OP:
            DPRINT(5,("We have an B_OP"));
            // unconditional branch found
            // no need to chase down branch targets unless you are
            // tracing (i.e. NOT stepping over functions).
            // Of course the whole test about stepping etc. only
            // makes sense if you are stepping over a FUNCTION CALL
            // i.e. a branch and link operation


            if (!fStep) {
                if (absolute) {
                    /* LI can only address words not bytes so << 2 */
                    returnvalue = disinstr.Iform_LI << 2;
                } else {
                    returnvalue = (disinstr.Iform_LI << 2) + iaraddr.addr.off;
                }
            }

            break;

        case BC_OP:
            DPRINT(5,("We have a BC_OP"));
            /* We got a branch conditional, if it evaluates to true,
               let us set return to the target. Otherwise
               let us leave the default returnvalue in place */


             /* <<  2 bits since we address words */

             if (ctr_ok && cond_ok) {
                if (absolute) {
                    returnvalue = disinstr.Bform_BD << 2;
                } else {
                    returnvalue = (disinstr.Bform_BD << 2)+iaraddr.addr.off;
                }
             }
             break;

        case X19_OP:
            DPRINT(5,("We have an X19_OP"));
            if (disinstr.XLform_XO == BCLR_OP) {
                lr = hthd->context.Lr;

                if (ctr_ok && cond_ok) {
                    returnvalue = lr & ~3;  // remember, we address words
                                            // not bytes thus ~3

                }
            } else if (disinstr.XLform_XO == BCCTR_OP) {
                if (cond_ok) {
                    returnvalue = ctr & ~3;
                }
            }

            break;

            default:
            DPRINT(5,("We have an unhandled DEFAULT op"));
            break;

    }

    return returnvalue;
}                           /* GetNextOffset() */



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

    lpeo->addrStack.addr.off = lpeo->hthd->context.Gpr1;

    /*
     * Now place the return address correctly
     */

    lpeo->hthd->context.Iar = lpeo->hthd->context.Lr =
      lpeo->addrStart.addr.off;

    /*
     * Set the instruction pointer to the starting addresses
     *  and write the context back out
     */

    lpeo->hthd->context.Iar = lpeo->addrStart.addr.off;

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

    if (lpeo->addrStack.addr.off <= lpeo->hthd->context.Gpr1) {
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

#if 0

DWORD
BranchUnassemble(
    void    *Memory,
    ADDR    *Addr,
    BOOL    *IsBranch,
    BOOL    *TargetKnown,
    BOOL    *IsCall,
    BOOL    *IsTable,
    ADDR    *Target
    )

{

    ULONG   opcode, absolute=FALSE, linkbit_on=FALSE;
    PPC_INSTRUCTION disinstr;
    UOFF32  Offset;
    UOFF32  TargetOffset;


    assert( Memory );
    assert( IsBranch );
    assert( TargetKnown );
    assert( IsCall );
    assert( Target );

    Offset = GetAddrOff(*Addr);
    TargetOffset = 0;
    *IsBranch = FALSE;
    *IsTable  = FALSE;
    disinstr.Long = * (PULONG )  Memory;

    // Is the absolute bit on ?
    absolute = (int) ((disinstr.Long >> 1) & 1);

    // Is the link bit on ?
    linkbit_on =  (int) ((disinstr.Long & 1));

    opcode = disinstr.Primary_Op;

    switch (opcode)
    {
         default:
            break;

         case BC_OP:
            *IsCall= linkbit_on;
            *IsBranch = TRUE;
            *TargetKnown = TRUE;
            if (absolute)
              TargetOffset = disinstr.Bform_BD << 2;
            else
              TargetOffset = (disinstr.Bform_BD << 2)+ Offset;
            break;

         case B_OP:
            *IsCall= linkbit_on;
            *IsBranch = TRUE;
            *TargetKnown = TRUE;
            if (absolute)
              TargetOffset = disinstr.Iform_LI << 2;
            else
              TargetOffset = (disinstr.Iform_LI << 2) + Offset;
            break;


         case X19_OP:
            // branch conditional on register (various extended opcodes)
            *IsCall = linkbit_on;
            *IsBranch = TRUE;
            *TargetKnown = FALSE;
            TargetOffset = 0;
            break;

    }

    AddrInit(Target, 0, 0, TargetOffset, TRUE, TRUE, FALSE, FALSE);

    return(sizeof(DWORD));

}

#endif

BOOL
SetupDebugRegister(
    HTHDX       hthd,
    int         Register,
    int         DataSize,
    DWORD       DataAddr,
    DWORD       BpType
    )
{
    DWORD               Len;
    DWORD               rwMask;

#ifdef KERNEL
    KSPECIAL_REGISTERS  ksr;
    PDWORD  Dr0 = &ksr.KernelDr0;
    PDWORD  Dr1 = &ksr.KernelDr1;
    PDWORD  Dr2 = &ksr.KernelDr2;
    PDWORD  Dr3 = &ksr.KernelDr3;
    PDR7    Dr7 = (PDR7)&(ksr.KernelDr7);
#else
    CONTEXT     Context;
    PDWORD  Dr0 = &Context.Dr0;
    PDWORD  Dr1 = &Context.Dr1;
    PDWORD  Dr2 = &Context.Dr2;
    PDWORD  Dr3 = &Context.Dr3;
    PDR7    Dr7 = (PDR7)&(Context.Dr7);
#endif

    // ppc currently only supports 1
    assert(Register == 1);

#ifdef KERNEL
    if (!GetExtendedContext(hthd, &ksr))
#else
    Context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    if (!GetThreadContext(hthd->rwHand, &Context))
#endif
    {
        return FALSE;
    }


    Len  = LenMask[ DataSize ];

    switch ( BpType ) {
        case bptpDataR:
            rwMask = RWE_READWRITE;
            break;

        case bptpDataW:
        case bptpDataC:
            rwMask = RWE_WRITE;
            break;

        case bptpDataExec:
            rwMask = RWE_EXEC;
            //
            // length must be 0 for exec bp
            //
            Len = 0;
            break;

        default:
            assert(!"Invalid BpType!!");
            break;
    }


    switch( Register ) {
      case 0:
        *Dr0          = DataAddr;
        Dr7->Len0     = Len;
        Dr7->Rwe0     = rwMask;
        Dr7->L0       = 0x01;
        break;
      case 1:
        *Dr1          = DataAddr;
        Dr7->Len1     = Len;
        Dr7->Rwe1     = rwMask;
        Dr7->L1       = 0x01;
        break;
      case 2:
        *Dr2          = DataAddr;
        Dr7->Len2     = Len;
        Dr7->Rwe2     = rwMask;
        Dr7->L2       = 0x01;
        break;
      case 3:
        *Dr3          = DataAddr;
        Dr7->Len3     = Len;
        Dr7->Rwe3     = rwMask;
        Dr7->L3       = 0x01;
        break;
    }

#ifdef KERNEL
    ksr.KernelDr6 = 0;
    return SetExtendedContext(hthd, &ksr);
#else
    return SetThreadContext(hthd->rwHand, &Context);
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
    PDWORD  Dr0 = &ksr.KernelDr0;
    PDWORD  Dr1 = &ksr.KernelDr1;
    PDWORD  Dr2 = &ksr.KernelDr2;
    PDWORD  Dr3 = &ksr.KernelDr3;
    PDR7    Dr7 = (PDR7)&(ksr.KernelDr7);
#else
    CONTEXT     Context;
    PDWORD  Dr0 = &Context.Dr0;
    PDWORD  Dr1 = &Context.Dr1;
    PDWORD  Dr2 = &Context.Dr2;
    PDWORD  Dr3 = &Context.Dr3;
    PDR7    Dr7 = (PDR7)&(Context.Dr7);
#endif

    // ppc currently only supports 1
    assert(Register == 1);


#ifdef KERNEL
    if (GetExtendedContext(hthd, &ksr))
#else
    Context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    if (GetThreadContext(hthd->rwHand, &Context))
#endif
    {

        switch( Register ) {
          case 0:
            *Dr0          = 0;
            Dr7->Len0     = 0;
            Dr7->Rwe0     = 0;
            Dr7->L0       = 0;
            break;
          case 1:
            *Dr1          = 0;
            Dr7->Len1     = 0;
            Dr7->Rwe1     = 0;
            Dr7->L1       = 0;
            break;
          case 2:
            *Dr2          = 0;
            Dr7->Len2     = 0;
            Dr7->Rwe2     = 0;
            Dr7->L2       = 0;
            break;
          case 3:
            *Dr3          = 0;
            Dr7->Len3     = 0;
            Dr7->Rwe3     = 0;
            Dr7->L3       = 0;
            break;
        }

#ifdef KERNEL
        ksr.KernelDr6 = 0;
        SetExtendedContext(hthd, &ksr);
#else
        SetThreadContext( hthd->rwHand, &Context );
#endif
    }
}


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

ULONGLONG
GetRegValue(
    PCONTEXT regs,
    int cvindex
    )
{
    if (cvindex < CV_PPC_GPR0+32) {
        return (&regs->Gpr0)[cvindex - CV_PPC_GPR0];
    }

    switch (cvindex) {
        case CV_PPC_PC:
            return regs->Iar;

        case CV_PPC_LR:
            return regs->Lr;

        default:
            assert(!"GetRegValue called with unrecognized index");
            return (ULONGLONG)0 - 1;
    }
}


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
    // void * returned by this function. In case of PPC no information
    // needs to be passed currently.
    return NULL;
}

#endif
