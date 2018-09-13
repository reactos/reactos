/**** EMDPDEV.C - Debugger end Execution Model (PPC dependent code)     **
 *                                                                         *
 *  Copyright <C> 1990, Microsoft Corp                                     *
 *                                                                         *
 *  Created: October 15, 1990 by David W. Gray                             *
 *  Much modified by Farooq Butt (fmbutt@engage.sps.mot.com), Motorola     *
 *                                                                         *
 *  Revision History:                                                      *
 *                                                                         *
 *  Purpose:                                                               *
 *                                                                         *
 ***************************************************************************/


#define TARGET_PPC
#include "emdp_plt.h"
#include "str_ppc.h"

#include "ehdata.h"

RD PpcRgrd[] = {
#include "regs_ppc.h"
};
unsigned CPpcRgrd = (sizeof(PpcRgrd)/sizeof(PpcRgrd[0]));

RGFD PpcRgfd[] = {
#include "flag_Ppc.h"
};
unsigned CPpcRgfd = (sizeof(PpcRgfd)/sizeof(PpcRgfd[0]));


#define CEXM_MDL_native 0x20

#define SIZEOF_STACK_OFFSET sizeof(LONG)

LPVOID
PpcSwFunctionTableAccess(
    LPVOID lpvhpid,
    DWORD   AddrBase
    );

XOSD
PpcGetAddr (
    HPID   hpid,
    HTID   htid,
    ADR    adr,
    LPADDR lpaddr
    );

XOSD
PpcSetAddr (
    HPID   hpid,
    HTID   htid,
    ADR    adr,
    LPADDR lpaddr
    );

LPVOID
PpcDoGetReg(
    LPVOID lpregs1,
    DWORD ireg,
    LPVOID lpvRegValue
    );

XOSD
PpcGetRegValue (
    HPID hpid,
    HTID htid,
    DWORD ireg,
    LPVOID lpvRegValue
    );

LPVOID
PpcDoSetReg(
    LPVOID   lpregs1,
    DWORD    ireg,
    LPVOID   lpvRegValue
    );

XOSD
PpcSetRegValue (
    HPID hpid,
    HTID htid,
    DWORD ireg,
    LPVOID lpvRegValue
    );

XOSD
PpcDoGetFrame(
    HPID hpid,
    HTID uhtid,
    DWORD wValue,
    DWORD lValue
    );

XOSD
PpcDoGetFrameEH(
    HPID hpid,
    HTID htid,
    LPEXHDLR *lpexhdlr,
    LPDWORD cAddrsAllocated
);

XOSD
PpcUpdateChild (
    HPID hpid,
    HTID htid,
    DMF dmfCommand
    );

void
PpcCopyFrameRegs(
    LPTHD lpthd,
    LPBPR lpbpr
    );

void
PpcAdjustForProlog(
    HPID hpid,
    HTID htid,
    PADDR origAddr,
    CANSTEP *CanStep
    );

XOSD
PpcDoGetFunctionInfo(
    HPID hpid,
    LPGFI lpgfi
    );

CPU_POINTERS PpcPointers = {
    sizeof(CONTEXT),        //  size_t SizeOfContext;
    PpcRgfd,                //  RGFD * Rgfd;
    PpcRgrd,                //  RD   * Rgrd;
    CPpcRgfd,               //  int    CRgfd;
    CPpcRgrd,               //  int    CRgrd;

    PpcGetAddr,             //  PFNGETADDR          pfnGetAddr;
    PpcSetAddr,             //  PFNSETADDR          pfnSetAddr;
    PpcDoGetReg,            //  PFNDOGETREG         pfnDoGetReg;
    PpcGetRegValue,         //  PFNGETREGVALUE      pfnGetRegValue;
    PpcDoSetReg,            //  PFNSETREG           pfnDoSetReg;
    PpcSetRegValue,         //  PFNSETREGVALUE      pfnSetRegValue;
    XXGetFlagValue,         //  PFNGETFLAG          pfnGetFlag;
    XXSetFlagValue,         //  PFNSETFLAG          pfnSetFlag;
    PpcDoGetFrame,          //  PFNGETFRAME         pfnGetFrame;
    PpcDoGetFrameEH,        //  PFNGETFRAMEEH       pfnGetFrameEH;
    PpcUpdateChild,         //  PFNUPDATECHILD      pfnUpdateChild;
    PpcCopyFrameRegs,       //  PFNADJUSTFORPROLOG  pfnAdjustForProlog;
    PpcAdjustForProlog,     //  PFNCOPYFRAMEREGS    pfnCopyFrameRegs;
    PpcDoGetFunctionInfo,   //  PFNGETFUNCTIONINFO  pfnGetFunctionInfo;
};




XOSD
PpcGetAddr (
    HPID   hpid,
    HTID   htid,
    ADR    adr,
    LPADDR lpaddr
    )

/*++

Routine Description:

    This function will get return a specific type of address.

Arguments:

    hpid   - Supplies the handle to the process to retrive the address from

    htid   - Supplies the handle to the thread to retrieve the address from

    adr    - Supplies the type of address to be retrieved

    lpaddr - Returns the requested address

Return Value:

    XOSD error code

--*/

{
    HPRC  hprc;
    HTHD  hthd;
    LPTHD lpthd = NULL;
    XOSD  xosd = xosdNone;
    HEMI  hemi = emiAddr(*lpaddr);
    HMDI        hmdi;
    LPMDI       lpmdi;

    BOOL  fVhtid;

    assert ( lpaddr != NULL );
    assert ( hpid != 0 );

    hprc = ValidHprcFromHpid(hpid);
    if (!hprc) {
        return xosdBadProcess;
    }

    fVhtid = ((DWORD)htid & 1);
    if (fVhtid) {
        htid = (HTID)((DWORD)htid & ~1);
    }

    hthd = HthdFromHtid(hprc, htid);

    if ( hthd != 0 ) {
        lpthd = (LPTHD) LLLock ( hthd );
    }

    _fmemset ( lpaddr, 0, sizeof ( ADDR ) );

    if (!fVhtid) {
        switch ( adr ) {

        case adrPC:
            if ( lpthd && !(lpthd->drt & drtCntrlPresent) ) {
                UpdateRegisters ( hprc, hthd );
            }
            break;

        case adrData:
            if ( lpthd && !(lpthd->drt & drtAllPresent) ) {
                UpdateRegisters ( hprc, hthd );
            }
            break;
        }
    }

    switch ( adr ) {

    case adrPC:
        if (!fVhtid) {
            AddrInit(lpaddr, 0, 0,
                     (UOFFSET) ((PCONTEXT) (lpthd->regs))->Iar, lpthd->fFlat,
                     lpthd->fOff32, FALSE, lpthd->fReal);
        } else {
            AddrInit(lpaddr, 0, 0,
                     (UOFFSET) lpthd->StackFrame.AddrPC.Offset, lpthd->fFlat,
                     lpthd->fOff32, FALSE, lpthd->fReal);
        }
        SetEmi ( hpid, lpaddr );
        break;

    case adrBase:
        if (!fVhtid) {
            AddrInit(lpaddr, 0, 0,
                     (UOFFSET) ((PCONTEXT) (lpthd->regs))->Gpr1, lpthd->fFlat,
                     lpthd->fOff32, FALSE, lpthd->fReal);
        } else {
            AddrInit(lpaddr, 0, 0,
                    (UOFFSET) lpthd->StackFrame.AddrFrame.Offset, lpthd->fFlat,
                    lpthd->fOff32, FALSE, lpthd->fReal);
        }
        SetEmi ( hpid, lpaddr );
        break;

    case adrData:
        {
            UOFFSET offset = 0;
            ADDR addr = {0};

            PpcGetAddr(hpid, htid, adrPC, &addr);
            hmdi = LLFind( LlmdiFromHprc( hprc ), 0, &emiAddr( addr ), emdiEMI);
            if (hmdi != 0) {
                lpmdi = (LPMDI) LLLock( hmdi);
                offset = (UOFFSET) lpmdi->lpBaseOfData;
                LLUnlock( hmdi );
            }
            AddrInit(lpaddr, 0, 0, offset,
                     lpthd->fFlat, lpthd->fOff32, FALSE, lpthd->fReal);
            SetEmi ( hpid, lpaddr );
        }
        break;

    case adrTlsBase:
        /*
         * If -1 then we have not gotten a value from the DM yet.
         */

        assert(hemi != 0);

        if (hemi == 0) {
            return xosdBadAddress;
        }

        if (hemi != emiAddr(lpthd->addrTls)) {
            hmdi = LLFind( LlmdiFromHprc( hprc ), 0, (LPBYTE) &hemi, emdiEMI);
            assert(hmdi != 0);

            if (hmdi == 0) {
                return xosdBadAddress;
            }

            lpmdi = (LPMDI) LLLock( hmdi );

            SendRequestX( dmfQueryTlsBase, 
                          hpid, 
                          htid, 
                          sizeof(lpmdi->lpBaseOfDll),
                          &lpmdi->lpBaseOfDll
                          );

            lpthd->addrTls = *((LPADDR) LpDmMsg->rgb);
            emiAddr(lpthd->addrTls) = hemi;
            LLUnlock( hmdi );

        }

        *lpaddr = lpthd->addrTls;
        emiAddr(*lpaddr) = 0;
        break;

    case adrStack:
        if (!fVhtid) {
            AddrInit(lpaddr, 0, 0,
                     (UOFFSET) ((PCONTEXT) (lpthd->regs))->Gpr1, lpthd->fFlat,
                     lpthd->fOff32, FALSE, lpthd->fReal);
        } else {
            AddrInit(lpaddr, 0, 0,
                     (UOFFSET) lpthd->StackFrame.AddrStack.Offset, lpthd->fFlat,
                     lpthd->fOff32, FALSE, lpthd->fReal);
        }
        SetEmi ( hpid, lpaddr );
        break;

    default:

        assert ( FALSE );
        break;
    }

    if ( hthd != 0 ) {
        LLUnlock ( hthd );
    }

    return xosd;
}                               /* GetAddr() */


XOSD
PpcSetAddr (
    HPID   hpid,
    HTID   htid,
    ADR    adr,
    LPADDR lpaddr
    )
{
    HPRC  hprc;
    HTHD  hthd;
    LPTHD lpthd = NULL;

    assert ( lpaddr != NULL );
    assert ( hpid != 0 );

    hprc = ValidHprcFromHpid(hpid);
    if (!hprc) {
        return xosdBadProcess;
    }

    hthd = HthdFromHtid(hprc, htid);

    if ( hthd != 0 ) {
        lpthd = (LPTHD) LLLock ( hthd );
    }

    switch ( adr ) {
    case adrPC:
        if ( lpthd && !(lpthd->drt & drtCntrlPresent) ) {
            UpdateRegisters ( hprc, hthd );
        }
        break;

    case adrData:
        if ( lpthd && !(lpthd->drt & drtAllPresent) ) {
            UpdateRegisters ( hprc, hthd );
        }
        break;
    }

    switch ( adr ) {

    case adrPC:
        ((PCONTEXT) (lpthd->regs))->Iar  = GetAddrOff ( *lpaddr );
        lpthd->drt = (DRT) (lpthd->drt | drtCntrlDirty);
        break;

    case adrData:
    case adrTlsBase:
    default:
        assert ( FALSE );
        break;
    }

    if ( hthd != 0 ) {
        LLUnlock ( hthd );
    }

    return xosdNone;
}                               /* SetAddr() */


XOSD
PpcSetAddrFromCSIP (
    HTHD hthd
    )
{

    ADDR addr = {0};
    LPTHD lpthd;

    assert ( hthd != 0 && hthd != hthdInvalid );

    lpthd = (LPTHD) LLLock ( hthd );

    GetAddrSeg ( addr ) = 0;
    GetAddrOff ( addr ) = (UOFFSET) ((PCONTEXT) (lpthd->regs))->Iar;
    emiAddr ( addr ) =  0;
    ADDR_IS_FLAT ( addr ) = TRUE;

    LLUnlock ( hthd );

    return xosdNone;
}



LPVOID
PpcDoGetReg(
    LPVOID lpregs1,
    DWORD ireg,
    LPVOID lpvRegValue
    )

/*++

Routine Description:

    This routine is used to extract the value of a single register from
    the debuggee.

Arguments:

    lpregs      - Supplies pointer to the register set for the debuggee

    ireg        - Supplies the index of the register to be read

    lpvRegValue - Supplies the buffer to place the register value in

Return Value:

    return-value - lpvRegValue + size of register on sucess and NULL on
                failure
--*/

{
    LPCONTEXT lpregs = (LPCONTEXT)lpregs1;
    switch ( ireg ) {

    case CV_PPC_GPR0:
    case CV_PPC_GPR1:
    case CV_PPC_GPR2:
    case CV_PPC_GPR3:
    case CV_PPC_GPR4:
    case CV_PPC_GPR5:
    case CV_PPC_GPR6:
    case CV_PPC_GPR7:
    case CV_PPC_GPR8:
    case CV_PPC_GPR9:
    case CV_PPC_GPR10:
    case CV_PPC_GPR11:
    case CV_PPC_GPR12:
    case CV_PPC_GPR13:
    case CV_PPC_GPR14:
    case CV_PPC_GPR15:
    case CV_PPC_GPR16:
    case CV_PPC_GPR17:
    case CV_PPC_GPR18:
    case CV_PPC_GPR19:
    case CV_PPC_GPR20:
    case CV_PPC_GPR21:
    case CV_PPC_GPR22:
    case CV_PPC_GPR23:
    case CV_PPC_GPR24:
    case CV_PPC_GPR25:
    case CV_PPC_GPR26:
    case CV_PPC_GPR27:
    case CV_PPC_GPR28:
    case CV_PPC_GPR29:
    case CV_PPC_GPR30:
    case CV_PPC_GPR31:
        *((LPL) lpvRegValue) = ((LONG *)(&lpregs->Gpr0))[ireg - CV_PPC_GPR0];
        break;

    case CV_PPC_FPSCR:
        *((double *) lpvRegValue) = lpregs->Fpscr;
        break;

    case CV_PPC_PC:
        *((LPL) lpvRegValue) = lpregs->Iar;
        break;

    case CV_PPC_MSR:
        *((LPL) lpvRegValue) = lpregs->Msr;
        break;

    case CV_PPC_XER:
        *((LPL) lpvRegValue) = lpregs->Xer;
        break;

    case CV_PPC_LR:
        *((LPL) lpvRegValue) = lpregs->Lr;
        break;

    case CV_PPC_CTR:
        *((LPL) lpvRegValue) = lpregs->Ctr;
        break;

    case CV_PPC_CR:
        *((LPL) lpvRegValue) = lpregs->Cr;
        break;


    case CV_PPC_FPR0:
    case CV_PPC_FPR1:
    case CV_PPC_FPR2:
    case CV_PPC_FPR3:
    case CV_PPC_FPR4:
    case CV_PPC_FPR5:
    case CV_PPC_FPR6:
    case CV_PPC_FPR7:
    case CV_PPC_FPR8:
    case CV_PPC_FPR9:
    case CV_PPC_FPR10:
    case CV_PPC_FPR11:
    case CV_PPC_FPR12:
    case CV_PPC_FPR13:
    case CV_PPC_FPR14:
    case CV_PPC_FPR15:
    case CV_PPC_FPR16:
    case CV_PPC_FPR17:
    case CV_PPC_FPR18:
    case CV_PPC_FPR19:
    case CV_PPC_FPR20:
    case CV_PPC_FPR21:
    case CV_PPC_FPR22:
    case CV_PPC_FPR23:
    case CV_PPC_FPR24:
    case CV_PPC_FPR25:
    case CV_PPC_FPR26:
    case CV_PPC_FPR27:
    case CV_PPC_FPR28:
    case CV_PPC_FPR29:
    case CV_PPC_FPR30:
    case CV_PPC_FPR31:
        *((double *) lpvRegValue) = ((double *)(&lpregs->Fpr0))[ireg - CV_PPC_FPR0];
        break;

    default:
        if (ireg != 0) {
            char string[1000];
            sprintf(string, "PpcDoGetReg: ireg == %d (0x%x)\n", ireg, ireg);
            OutputDebugString(string);
            assert(FALSE);
        }
    }

    return (LPVOID) ((LPBYTE)lpvRegValue + sizeof(LONG));
}


LPVOID
PpcDoSetReg(
    LPVOID lpregs1,
    DWORD ireg,
    LPVOID lpvRegValue
    )

/*++

Routine Description:

    This routine is used to set a specific register in a threads
    context

Arguments:

    lpregs      - Supplies pointer to register context for thread

    ireg        - Supplies the index of the register to be modified

    lpvRegValue - Supplies the buffer containning the new data

Return Value:

    return-value - the pointer the the next location where a register
        value could be.

--*/

{
    LPCONTEXT lpregs = (LPCONTEXT)lpregs1;
    switch ( ireg ) {

    case CV_PPC_GPR0:
    case CV_PPC_GPR1:
    case CV_PPC_GPR2:
    case CV_PPC_GPR3:
    case CV_PPC_GPR4:
    case CV_PPC_GPR5:
    case CV_PPC_GPR6:
    case CV_PPC_GPR7:
    case CV_PPC_GPR8:
    case CV_PPC_GPR9:
    case CV_PPC_GPR10:
    case CV_PPC_GPR11:
    case CV_PPC_GPR12:
    case CV_PPC_GPR13:
    case CV_PPC_GPR14:
    case CV_PPC_GPR15:
    case CV_PPC_GPR16:
    case CV_PPC_GPR17:
    case CV_PPC_GPR18:
    case CV_PPC_GPR19:
    case CV_PPC_GPR20:
    case CV_PPC_GPR21:
    case CV_PPC_GPR22:
    case CV_PPC_GPR23:
    case CV_PPC_GPR24:
    case CV_PPC_GPR25:
    case CV_PPC_GPR26:
    case CV_PPC_GPR27:
    case CV_PPC_GPR28:
    case CV_PPC_GPR29:
    case CV_PPC_GPR30:
    case CV_PPC_GPR31:
        ((LONG *)(&lpregs->Gpr0))[ireg - CV_PPC_GPR0] = *((LPL) lpvRegValue);
        break;

    case CV_PPC_FPSCR:
        lpregs->Fpscr = *((double *) lpvRegValue);
        break;

    case CV_PPC_LR:
        lpregs->Lr = *((LPL) lpvRegValue);
        break;

    case CV_PPC_PC:
        lpregs->Iar = *((LPL) lpvRegValue);
        break;

    case CV_PPC_XER:
        lpregs->Xer = *((LPL) lpvRegValue);
        break;

    case CV_PPC_MSR:
        lpregs->Msr = *((LPL) lpvRegValue);
        break;

    case CV_PPC_CTR:
        lpregs->Ctr = *((LPL) lpvRegValue);
        break;

    case CV_PPC_CR:
        lpregs->Cr = *((LPL) lpvRegValue);
        break;

    case CV_PPC_FPR0:
    case CV_PPC_FPR1:
    case CV_PPC_FPR2:
    case CV_PPC_FPR3:
    case CV_PPC_FPR4:
    case CV_PPC_FPR5:
    case CV_PPC_FPR6:
    case CV_PPC_FPR7:
    case CV_PPC_FPR8:
    case CV_PPC_FPR9:
    case CV_PPC_FPR10:
    case CV_PPC_FPR11:
    case CV_PPC_FPR12:
    case CV_PPC_FPR13:
    case CV_PPC_FPR14:
    case CV_PPC_FPR15:
    case CV_PPC_FPR16:
    case CV_PPC_FPR17:
    case CV_PPC_FPR18:
    case CV_PPC_FPR19:
    case CV_PPC_FPR20:
    case CV_PPC_FPR21:
    case CV_PPC_FPR22:
    case CV_PPC_FPR23:
    case CV_PPC_FPR24:
    case CV_PPC_FPR25:
    case CV_PPC_FPR26:
    case CV_PPC_FPR27:
    case CV_PPC_FPR28:
    case CV_PPC_FPR29:
    case CV_PPC_FPR30:
    case CV_PPC_FPR31:
        ((double *)(&lpregs->Fpr0))[ireg - CV_PPC_FPR0] = *((double *) lpvRegValue);
        break;


    default:
        assert(FALSE);
        return NULL;
    }

    return (LPVOID)((LPBYTE)lpvRegValue + sizeof(LONG));
}


LPVOID
PpcDoSetFrameReg(
    HPID hpid,
    HTID htid,
    LPTHD lpthd,
    PKNONVOLATILE_CONTEXT_POINTERS contextPtrs,
    DWORD ireg,
    LPVOID lpvRegValue
    )
{
    return PpcDoSetReg(&lpthd->regs, ireg, lpvRegValue);
}


XOSD
PpcDoGetFrame(
    HPID hpid,
    HTID uhtid,
    DWORD wValue,
    DWORD lValue
    )
{
    HPRC hprc = ValidHprcFromHpid(hpid);
    HTHD hthd;
    LPTHD lpthd;
    HTID htid = (HTID)((DWORD)uhtid & ~1);
    HTID vhtid = (HTID)((DWORD)uhtid | 1);
    DWORD i;
    XOSD xosd;
    BOOL fGoodFrame;
    BOOL fReturnHtid = FALSE;

    if (!hprc) {
        return xosdBadProcess;
    }

    if (wValue == (DWORD)-1) {
        wValue = 1;
        fReturnHtid = TRUE;
    }

    hthd = HthdFromHtid(hprc, htid);
    if (hthd == NULL) {
        return xosdBadThread;
    }

    lpthd = (LPTHD) LLLock( hthd );
    assert(lpthd != NULL);



    if ( uhtid == htid ) {

        //
        // first frame -
        // Get regs and clear STACKFRAME struct
        //

        if (lpthd->drt & (drtCntrlDirty|drtAllDirty)) {
            SendRequestX(dmfWriteReg,hpid,htid,sizeof(CONTEXT),lpthd->regs);
            lpthd->drt = (DRT) (lpthd->drt & ~(drtCntrlDirty|drtAllDirty) );
        }
        UpdateRegisters( hprc, hthd );

        ZeroMemory( &lpthd->StackFrame, sizeof(STACKFRAME64) );
        memcpy (lpthd->frameRegs, lpthd->regs, sizeof(CONTEXT) );
        lpthd->frameNumber = 0;
    }

    fGoodFrame = FALSE;
    xosd = xosdNone;
    for (i = 0; xosd == xosdNone && ((wValue != 0)? (i < wValue) : 1); i++) {

        DWORD pc    = lpthd->StackFrame.AddrPC.Offset;
        DWORD stack = lpthd->StackFrame.AddrFrame.Offset;
        if (StackWalk64( IMAGE_FILE_MACHINE_POWERPC,
                         hpid,
                         htid,
                         &lpthd->StackFrame,
                         lpthd->frameRegs,
                         SwReadMemory,
                         PpcSwFunctionTableAccess,
                         NULL,
                         NULL
                         ))
        {



            // Establish that AddrPC points to return address
            if (lpthd->frameNumber != 0 && lpthd->StackFrame.AddrPC.Offset) {
                lpthd->StackFrame.AddrPC.Offset += 4;
            }

            lpthd->frameNumber++;
            fGoodFrame = TRUE;
        } else {
            xosd = xosdEndOfStack;
        }
        if ((pc == lpthd->StackFrame.AddrPC.Offset) &&
            (stack == lpthd->StackFrame.AddrFrame.Offset)) {
            xosd = xosdEndOfStack;
        }
    }

    if (fGoodFrame) {
        *(LPHTID)lValue = fReturnHtid? htid : vhtid;
    }

    LLUnlock( hthd );

    return xosd;
}


PIMAGE_RUNTIME_FUNCTION_ENTRY
PpcLookupFunctionEntry (
    PIMAGE_RUNTIME_FUNCTION_ENTRY FunctionTable,
    DWORD                         NumberOfFunctions,
    DWORD                         ControlPc
    )

/*++

Routine Description:

    This function searches the currently active function tables for an entry
    that corresponds to the specified PC value.

Arguments:

    ControlPc - Supplies the address of an instruction within the specified
        function.

Return Value:

    If there is no entry in the function table for the specified PC, then
    NULL is returned. Otherwise, the address of the function table entry
    that corresponds to the specified PC is returned.

--*/

{

    PIMAGE_RUNTIME_FUNCTION_ENTRY FunctionEntry;
    LONG High;
    LONG Low;
    LONG Middle;

    //
    // Initialize search indicies.
    //

    Low = 0;
    High = NumberOfFunctions - 1;

    //
    // Perform binary search on the function table for a function table
    // entry that subsumes the specified PC.
    //

    while (High >= Low) {

        //
        // Compute next probe index and test entry. If the specified PC
        // is greater than of equal to the beginning address and less
        // than the ending address of the function table entry, then
        // return the address of the function table entry. Otherwise,
        // continue the search.
        //

        Middle = (Low + High) >> 1;
        FunctionEntry = &FunctionTable[Middle];
        if (ControlPc < FunctionEntry->BeginAddress) {
            High = Middle - 1;

        } else if (ControlPc >= FunctionEntry->EndAddress) {
            Low = Middle + 1;

        } else {

#if 0 // This requires some work...
            //
            // The capability exists for more than one function entry
            // to map to the same function. This permits a function to
            // have (within reason) discontiguous code segment(s). If
            // EndOfPrologue is out of range, it is re-interpreted
            // as a pointer to the primary function table entry for
            // that function.  The out of range test takes into account
            // the redundant encoding of millicode and glue code.
            //

            if (((FunctionEntry->EndOfPrologue < FunctionEntry->StartingAddress) ||
                 (FunctionEntry->EndOfPrologue > FunctionEntry->EndingAddress)) &&
                (FunctionEntry->EndOfPrologue & 3) == 0) {
                FunctionEntry = (PRUNTIME_FUNCTION)FunctionEntry->EndOfPrologue;
            }
#endif

            return FunctionEntry;
        }
    }

    //
    // A function table entry for the specified PC was not found.
    //

    return NULL;
}


LPVOID
PpcSwFunctionTableAccess(
    LPVOID  lpvhpid,
    DWORD   AddrBase
    )
{
    HLLI                            hlli  = 0;
    HMDI                            hmdi  = 0;
    LPMDI                           lpmdi = 0;
    PIMAGE_RUNTIME_FUNCTION_ENTRY   rf;
    HPID                            hpid = (HPID)lpvhpid;


    hmdi = SwGetMdi( hpid, AddrBase );
    if (!hmdi) {
        return NULL;
    }

    lpmdi = (LPMDI)LLLock( hmdi );
    if (lpmdi) {
        VerifyDebugDataLoaded(hpid, NULL, lpmdi);        //M00BUG

        rf = PpcLookupFunctionEntry( (PIMAGE_RUNTIME_FUNCTION_ENTRY)lpmdi->lpDebug->lpRtf,
                                     lpmdi->lpDebug->cRtf,
                                     AddrBase
                                   );

        LLUnlock( hmdi );
        return (LPVOID)rf;
    }
    return NULL;
}


XOSD
PpcDoGetFunctionInfo(
    HPID   hpid,
    LPGFI  lpgfi
    )
/*++

Routine Description:

    Gets function information for a particular address.

Arguments:

    hpid - Supplies process

    lpgfi - Supplies a GetFunctionInfo packet

Return Value:

    xosd error code

--*/
{
    XOSD xosd   =   xosdNone;
    PIMAGE_RUNTIME_FUNCTION_ENTRY   pfe;


    pfe = (PIMAGE_RUNTIME_FUNCTION_ENTRY)
        PpcSwFunctionTableAccess( (LPVOID)hpid, GetAddrOff( *(lpgfi->lpaddr) ) );

    if ( pfe ) {

        AddrInit( &lpgfi->lpFunctionInformation->AddrStart,     0,0, pfe->BeginAddress,
                  TRUE, TRUE, FALSE, FALSE );
        AddrInit( &lpgfi->lpFunctionInformation->AddrEnd,       0,0, pfe->EndAddress,
                  TRUE, TRUE, FALSE, FALSE );
        AddrInit( &lpgfi->lpFunctionInformation->AddrPrologEnd, 0,0, pfe->PrologEndAddress,
                  TRUE, TRUE, FALSE, FALSE );

    } else {

        xosd = xosdUnknown;
    }

    return xosd;
}


#if 0

// this is the alpha code, left here as a helpful template
// in case somebody decides to do this for PPC.

//  This comes directly from ntalpha.h.  I could not figure out how to make
//  it available here because of header file problems.

#pragma message ("Determine if duplicating the SEH_BLOCK def is wrong.")
typedef struct _SEH_BLOCK {
    ULONG HandlerAddress;
    ULONG JumpTarget;
    struct _SEH_BLOCK *ParentSeb;
} SEH_BLOCK, *PSEH_BLOCK;

typedef struct _SEH_CONTEXT {
    PSEH_BLOCK CurrentSeb;
    ULONG ExceptionCode;
    ULONG RealFramePointer;
} SEH_CONTEXT, *PSEH_CONTEXT;

#endif

// BUGBUG DoGetFrameEH needs to be updated for PPC
XOSD
PpcDoGetFrameEH(
    HPID hpid,
    HTID htid,
    LPEXHDLR *lpexhdlr,
    LPDWORD cAddrsAllocated
)
/*++

Routine Description:

    Fill lpexhdlr with the addresses of all exception handlers for this frame

Arguments:

    hpid            - current hpid

    htid            - current htid (may be virtual)

    lpexhdlr        - where to store address

    cAddrsAllocated - how many addresses are currently allocated

Return Value:



--*/
{
    return xosdNone;

#if 0
    HPRC  hprc;
    HTHD  hthd;
    LPTHD lpthd = NULL;
    XOSD  xosd = xosdNone;

    BOOL  fVhtid;
    DWORD AddrBase;
    HMDI                            hmdi  = 0;
    LPMDI                           lpmdi = 0;
    PIMAGE_RUNTIME_FUNCTION_ENTRY   rfe;
    ADDR addr = {0}, lsebAddr, lsehcxtAddr;
    SEH_CONTEXT lsehcxt;
    SEH_BLOCK lseb;
    DWORD cb;
    STACKFRAME64 newFrame;
    CONTEXT newContext;
    EHContext lEHContext;
    TryBlockMapEntry *lpTryBlockMap;
    DWORD i;
    HandlerType * lpHandlerType;
    DWORD j;

    hprc = ValidHprcFromHpid(hpid);
    if (!hprc) {
        return xosdBadProcess;
    }
    fVhtid = ((DWORD)htid & 1);
    if (fVhtid) {
        htid = (HTID) ((DWORD)htid & ~1);
    }

    hthd = HthdFromHtid(hprc, htid);
    if ( hthd != NULL ) {
        lpthd = (LPTHD) LLLock ( hthd );
    }
    if (!fVhtid) {
        if (lpthd->drt & (drtCntrlDirty|drtAllDirty)) {
            SendRequestX(dmfWriteReg,hpid,htid,sizeof(CONTEXT),lpthd->regs);
            lpthd->drt = (DRT) (lpthd->drt & ~(drtCntrlDirty|drtAllDirty) );
        }
        UpdateRegisters( hprc, hthd );
    }
    AddrBase = fVhtid ? lpthd->StackFrame.AddrPC.Offset : ((PCONTEXT) (lpthd->regs))->Fir;
    if ( hthd != NULL ) {
        LLUnlock ( hthd );
    }
    hmdi = SwGetMdi( hpid, AddrBase );
    if (!hmdi) {
        return xosdBadThread;
    }

    lpmdi = (LPMDI) LLLock( hmdi );
    if (lpmdi) {
        VerifyDebugDataLoaded(hpid, htid, lpmdi);

        rfe = PpcLookupFunctionEntry( (PIMAGE_RUNTIME_FUNCTION_ENTRY) lpmdi->lpDebug->lpRtf,
                                   lpmdi->lpDebug->cRtf,
                                   AddrBase
                                );

        LLUnlock( hmdi );
    }
    if (rfe == NULL) {
        return( xosd );
    }
    /* Look for handlers.
     */
    if (rfe->HandlerData != NULL) {

        FuncInfo lFuncInfo;

        ADDR_IS_FLAT(addr) = TRUE;
        ADDR_IS_OFF32(addr) = TRUE;

        addr.addr.off =  (UOFF32)rfe->ExceptionHandler;

        // Get the virtual frame pointer
        newFrame = lpthd->StackFrame;
        memcpy ( (void *) &newContext, (void *) lpthd->regs, sizeof(CONTEXT) );

        StackWalk( IMAGE_FILE_MACHINE_ALPHA,
                   hpid,
                   htid,
                   &newFrame,
                   &newContext,
                   SwReadMemory,
                   PpcSwFunctionTableAccess,
                   NULL,
                   NULL
                   );

        addr.addr.off = (UOFF32)rfe->HandlerData;

        cb = 0;

        xosd = ReadBuffer(hpid, htid, &addr, sizeof(FuncInfo),
                 (PUCHAR)&lFuncInfo, &cb);

#if DBG
        assert (xosd != xosdNone || cb ==  sizeof(lFuncInfo));
#endif

        if (xosd != xosdNone) {

            // Assume SEH

            lsehcxtAddr = addr;
            lsehcxtAddr.addr.off = (UOFF32)(((DWORD)rfe->HandlerData) +
                    ((DWORD)newFrame.AddrFrame.Offset));
            cb = 0;
            if ((xosd = ReadBuffer(hpid, htid, &lsehcxtAddr, sizeof(SEH_CONTEXT),
                     (PUCHAR)&lsehcxt, &cb)) != xosdNone) {
#if DBG
                GetLastError();
#endif
                return xosd;
            }

            lsebAddr = addr;
            lsebAddr.addr.off = (UOFF32)lsehcxt.CurrentSeb;

            // Do this for each handler in the function
            do {

                cb = 0;

                if ((xosd = ReadBuffer(hpid, htid, &lsebAddr, sizeof(SEH_BLOCK),
                         (PUCHAR)&lseb, &cb)) != xosdNone) {
#if DBG
                    GetLastError();
#endif
                    return xosd;
                }

                if (lseb.JumpTarget) {
                    addr.addr.off = lseb.JumpTarget;

                    (*lpexhdlr)->addr[(*lpexhdlr)->count++] = addr;
                    if ((*lpexhdlr)->count == *cAddrsAllocated) {
                        *cAddrsAllocated *= 2;
                        *lpexhdlr = (LPEXHDLR) MHRealloc(*lpexhdlr, sizeof(EXHDLR)
                            + *cAddrsAllocated * sizeof(ADDR));
                        assert(*lpexhdlr);
                    }
                } else if (lseb.HandlerAddress) {
                    addr.addr.off = lseb.HandlerAddress;

                    hmdi = SwGetMdi( hpid, addr.addr.off );

                    if (!hmdi) {
                        return xosdBadThread;
                    }

                    lpmdi = (LPMDI) LLLock( hmdi );
                    if (lpmdi) {
                        VerifyDebugDataLoaded(hpid, htid, lpmdi);

                        rfe = PpcLookupFunctionEntry( (PIMAGE_RUNTIME_FUNCTION_ENTRY) lpmdi->lpDebug->lpRtf,
                                                   lpmdi->lpDebug->cRtf,
                                                   addr.addr.off
                                                );

                        LLUnlock( hmdi );
                    }
                    if (rfe == NULL) {
                        return( xosd );
                    }

                    // Need to get rfe for the lpseh
                    if (rfe->PrologEndAddress) {
                        addr.addr.off = rfe->PrologEndAddress & ~0x3;
                    }

                    (*lpexhdlr)->addr[(*lpexhdlr)->count++] = addr;
                    if ((*lpexhdlr)->count == *cAddrsAllocated) {
                        *cAddrsAllocated *= 2;
                        *lpexhdlr = (LPEXHDLR) MHRealloc(*lpexhdlr, sizeof(EXHDLR)
                            + *cAddrsAllocated * sizeof(ADDR));
                        assert(*lpexhdlr);
                    }
                }

                // Go to parent now
                lsebAddr.addr.off = (UOFF32) lseb.ParentSeb;

            } while (lseb.ParentSeb);

        } else if (lFuncInfo.magicNumber == 0x19930520) {

            // Validate the C++ EH specification being handled.
            // If this fails, then support must be added for the new spec.
            // C++ Exception handlers present


            addr.addr.off = (UOFF32)(((DWORD)lFuncInfo.EHContextDelta) +
                    ((DWORD)newFrame.AddrFrame.Offset));

            cb = 0;

            if ((xosd = ReadBuffer(hpid, htid, &addr, sizeof(EHContext),
                     (unsigned char *)&lEHContext, &cb)) != xosdNone) {
#if DBG
                GetLastError();
#endif
                return xosd;
            }

#if DBG
            assert (cb ==  sizeof(EHContext));
#endif

            // We need to read the TryBlockMap, but in order to do so,
            //   we first need to get the first word which is the count of
            //   entries in the MAP.

            addr.addr.off = (UOFF32)lFuncInfo.pTryBlockMap;

            // Make sure there are try blocks
            if (lFuncInfo.nTryBlocks == 0) {
                assert (lFuncInfo.pUnwindMap);
                return xosd;
            }

            lpTryBlockMap = (TryBlockMapEntry *) MHAlloc(lFuncInfo.nTryBlocks *
                                sizeof(TryBlockMapEntry));

            cb = 0;

            if ((xosd = ReadBuffer(hpid, htid, &addr, lFuncInfo.nTryBlocks *
                     sizeof(TryBlockMapEntry), (unsigned char *)lpTryBlockMap, &cb)) != xosdNone) {
#if DBG
                GetLastError();
#endif
                return xosd;
            }

#if DBG
            assert (cb ==  (lFuncInfo.nTryBlocks *  sizeof(TryBlockMapEntry)));
#endif

            for (i=0; i < lFuncInfo.nTryBlocks; ++i) {

                if ((lEHContext.State >= (ULONG)lpTryBlockMap[i].tryLow) &&
                    (lEHContext.State <= (ULONG)lpTryBlockMap[i].tryHigh)) {

                    assert (lpTryBlockMap[i].nCatches);

                    lpHandlerType = (HandlerType *) MHAlloc(lpTryBlockMap[i].nCatches *
                                        sizeof(HandlerType));

#if DBG
                    cb = 0;
#endif
                    addr.addr.off = (UOFF32)lpTryBlockMap[i].pHandlerArray;

                    if ((xosd = ReadBuffer(hpid, htid, &addr, lpTryBlockMap[i].nCatches *
                         sizeof(HandlerType), (unsigned char *)lpHandlerType, &cb)) != xosdNone) {
#if DBG
                        GetLastError();
#endif
                        MHFree( (void *) lpHandlerType);
                        MHFree( (void *) lpTryBlockMap);
                        return xosd;
                    }
#if DBG
                    assert (cb ==  (lpTryBlockMap[i].nCatches * sizeof(HandlerType)));
#endif

                    // This is a good scope, so lets start setting breakpoints
                    for (j=0; j < (DWORD)lpTryBlockMap[i].nCatches; ++j) {

                        assert (lpHandlerType[j].addressOfHandler);

                        addr.addr.off = (UOFF32) lpHandlerType[j].addressOfHandler;

                        hmdi = SwGetMdi( hpid, addr.addr.off );
                        if (!hmdi) {
                            MHFree( (void *) lpHandlerType);
                            MHFree( (void *) lpTryBlockMap);
                            return xosdBadThread;
                        }

                        lpmdi = (LPMDI) LLLock( hmdi );
                        if (lpmdi) {
                            VerifyDebugDataLoaded(hpid, htid, lpmdi);

                            rfe = PpcLookupFunctionEntry( (PIMAGE_RUNTIME_FUNCTION_ENTRY) lpmdi->lpDebug->lpRtf,
                                                       lpmdi->lpDebug->cRtf,
                                                       addr.addr.off
                                                    );

                            LLUnlock( hmdi );
                        }
                        if (rfe == NULL) {
                            MHFree( (void *) lpHandlerType);
                            MHFree( (void *) lpTryBlockMap);
                            return( xosd );
                        }

                        // Need to get rfe for the lpseh
                        if (rfe->PrologEndAddress) {
                            addr.addr.off = rfe->PrologEndAddress & ~0x3;
                        }
                        (*lpexhdlr)->addr[(*lpexhdlr)->count++] = addr;
                        if ((*lpexhdlr)->count == *cAddrsAllocated) {
                            *cAddrsAllocated *= 2;
                            *lpexhdlr = (LPEXHDLR) MHRealloc(*lpexhdlr, sizeof(EXHDLR)
                                + *cAddrsAllocated * sizeof(ADDR));
                            assert(*lpexhdlr);
                        }
                    } // for (j

                    MHFree( (void *) lpHandlerType);

                } // if (lEHContext

            } // end for (i

            MHFree( (void *) lpTryBlockMap);

        } else {

            // No known language-specific handler
            // Bail on this right now

            return xosd;

        }
    }  // end if (rfe
    return(xosd);

#endif // if 0
}


///////////////////////////////////////////////////////////
//  part 2.  Additional target dependent
///////////////////////////////////////////////////////////

XOSD
PpcUpdateChild (
    HPID hpid,
    HTID htid,
    DMF dmfCommand
    )
/*++

Routine Description:

    This function is used to cause registers to be written back to
    the child as necessary before the child is executed.

Arguments:

    hprc        - Supplies a process handle

    hthdExec    - Supplies the handle of the thread to update

    dmfCommand  - Supplies the command about to be executed.

Return Value:

    XOSD error code

--*/

{
    HPRC  hprc;
    HTHD  hthd;
    HTHD  hthdExec;
    XOSD  xosd  = xosdNone;
    HLLI  llthd;
    LPPRC lpprc;
    PST pst;
    HLLI    hllmdi;
    HMDI    hmdi;

    hprc = HprcFromHpid(hpid);
    hthdExec = HthdFromHtid(hprc, htid);

    llthd = LlthdFromHprc ( hprc );

    xosd = ProcessStatus(hpid, &pst);

    if (xosd != xosdNone) {
        return xosd;
    }

    if (pst.dwProcessState == pstDead || pst.dwProcessState == pstExited) {
        return xosdBadProcess;
    }

    else if (pst.dwProcessState != pstRunning) {
        for ( hthd = LLNext ( llthd, hthdNull );
              hthd != hthdNull;
              hthd = LLNext ( llthd, hthd ) ) {

            LPTHD lpthd = (LPTHD) LLLock ( hthd );

            if ( lpthd->drt & (drtCntrlDirty | drtAllDirty) ) {
                assert(lpthd->drt & drtAllPresent);
                SendRequestX (dmfWriteReg,
                              hpid,
                              lpthd->htid,
                              sizeof ( CONTEXT ),
                              lpthd->regs
                             );

                lpthd->drt = (DRT) (lpthd->drt & ~(drtCntrlDirty | drtAllDirty) ) ;
            }
            if ( lpthd->drt & drtSpecialDirty ) {
                assert(lpthd->drt & drtSpecialPresent);
                if (lpthd->dwcbSpecial) {
                    SendRequestX(dmfWriteRegEx,
                                 hpid,
                                 lpthd->htid,
                                 lpthd->dwcbSpecial,
                                 lpthd->pvSpecial
                                );
                }
                lpthd->drt = (DRT) (lpthd->drt & ~drtSpecialDirty);
            }

            lpthd->fRunning = TRUE;

            LLUnlock ( hthd );

            if ( xosd != xosdNone ) {
                break;
            }
        }
        lpprc = (LPPRC) LLLock(hprc);
        lpprc->fRunning = TRUE;
        hllmdi = lpprc->llmdi;

        while ( hmdi = LLFind ( hllmdi, NULL, NULL, emdiNLG ) ) {
            LPMDI   lpmdi = (LPMDI) LLLock ( hmdi );
            NLG     nlg = lpmdi->nlg;

            FixupAddr ( hpid, htid, &nlg.addrNLGDispatch );
            FixupAddr ( hpid, htid, &nlg.addrNLGDestination );
            FixupAddr ( hpid, htid, &nlg.addrNLGReturn );

//          SwapNlg ( &nlg );

            xosd = SendRequestX (
                                dmfNonLocalGoto,
                                HpidFromHprc ( hprc ),
                                NULL,
                                sizeof ( nlg ),
                                &nlg
                                );

            lpmdi->fSendNLG = FALSE;
            LLUnlock ( hmdi );
        }

        LLUnlock(hprc);
    }

    return xosd;
}


XOSD
PpcGetRegValue (
    HPID hpid,
    HTID htid,
    DWORD ireg,
    LPVOID lpvRegValue
    )
{
    HPRC        hprc;
    HTHD        hthd;
    LPTHD       lpthd;
    LPPRC       lpprc;
    LPCONTEXT   lpregs;

    hprc = ValidHprcFromHpid(hpid);
    if (!hprc) {
        return xosdBadProcess;
    }

    if ( (DWORD)htid & 1 ) {
        return GetFrameRegValue(hpid, (HTID)((DWORD)htid & ~1), ireg, lpvRegValue);
    }

    hthd = HthdFromHtid(hprc, htid);
    assert ( hthd != hthdNull );

    lpthd = (LPTHD) LLLock ( hthd );
    lpprc = (LPPRC) LLLock( hprc );

    lpregs = (LPCONTEXT) (lpthd->regs);

    if (lpthd->fRunning) {
        UpdateRegisters( lpthd->hprc, hthd );

        lpthd->fFlat = lpthd->fOff32 = TRUE;
        lpthd->fReal = FALSE;
    } else {

        if ( !(lpthd->drt & drtAllPresent) ) {

            switch ( ireg ) {


            case CV_PPC_PC:
            case CV_PPC_GPR1:
                if (!(lpthd->drt & drtCntrlPresent)) {
                    UpdateRegisters( lpthd->hprc, hthd );
                }
                break;

            default:

                UpdateRegisters ( lpthd->hprc, hthd );
                break;
            }
        }
    }


    LLUnlock( hprc );
    LLUnlock( hthd );

    lpvRegValue = PpcDoGetReg ( lpregs, ireg & 0xff, lpvRegValue );

    if ( lpvRegValue == NULL ) {
        LLUnlock ( hthd );
        return xosdInvalidParameter;
    }
    ireg = ireg >> 8;

    if ( ireg != CV_REG_NONE ) {
        lpvRegValue = PpcDoGetReg ( lpregs, ireg, lpvRegValue );
        if ( lpvRegValue == NULL ) {
            return xosdInvalidParameter;
        }
    }

    return xosdNone;

}                        /* GetRegValue */


XOSD
PpcSetRegValue (
    HPID hpid,
    HTID htid,
    DWORD ireg,
    LPVOID lpvRegValue
    )
{
    XOSD        xosd = xosdNone;
    HPRC        hprc;
    HTHD        hthd;
    LPTHD       lpthd;
    LPVOID      lpregs = NULL;

    hprc = ValidHprcFromHpid(hpid);
    if (!hprc) {
        return xosdBadProcess;
    }
    hthd = HthdFromHtid(hprc, htid);
    assert ( hthd != hthdNull );

    lpthd = (LPTHD) LLLock ( hthd );

    lpregs = lpthd->regs;

    if ( !(lpthd->drt & drtAllPresent) ) {
        UpdateRegisters ( lpthd->hprc, hthd );
    }

    lpvRegValue = PpcDoSetReg ( (LPCONTEXT) lpregs, ireg & 0xff, lpvRegValue );

    if ( lpvRegValue == NULL ) {
        LLUnlock ( hthd );
        return xosdInvalidParameter;
    }

    ireg = ireg >> 8;
    if ( ireg != 0 ) {
        lpvRegValue = PpcDoSetReg ( (LPCONTEXT) lpregs, ireg, lpvRegValue );
    }
    if ( lpvRegValue == NULL ) {
        LLUnlock ( hthd );
        return xosdInvalidParameter;
    }

    lpthd->drt = (DRT) (lpthd->drt | drtAllDirty);

    LLUnlock ( hthd );

    return xosd;

}


void
PpcCopyFrameRegs(
    LPTHD lpthd,
    LPBPR lpbpr
    )
{
    ((PCONTEXT) (lpthd->regs))->Iar    = lpbpr->offEIP;
    ((PCONTEXT) (lpthd->regs))->Gpr1   = lpbpr->offEBP;
}


void
PpcAdjustForProlog(
    HPID hpid,
    HTID htid,
    PADDR origAddr,
    CANSTEP *CanStep
    )
{
    Unreferenced(hpid);
    Unreferenced(htid);
    Unreferenced(origAddr);
    Unreferenced(CanStep);
}

