/*** EMDP_MIP.C - Debugger end Execution Model (MIPS dependent code)      **
 *                                                                         *
 *                                                                         *
 *  Copyright <C> 1990, Microsoft Corp                                     *
 *                                                                         *
 *  Created: October 15, 1990 by David W. Gray                             *
 *                                                                         *
 *  Revision History:                                                      *
 *                                                                         *
 *  Purpose:                                                               *
 *                                                                         *
 *                                                                         *
 ***************************************************************************/

#define TARGET_MIPS
#include "emdp_plt.h"
#include "str_mip.h"


RD MipsRgrd[] = {
#include "regs_mip.h"
};
const unsigned CMipsRgrd = (sizeof(MipsRgrd)/sizeof(MipsRgrd[0]));

RGFD MipsRgfd[] = {
#include "flag_mip.h"
};
const unsigned CMipsRgfd = (sizeof(MipsRgfd)/sizeof(MipsRgfd[0]));


#define CEXM_MDL_native 0x20

#define SIZEOF_STACK_OFFSET sizeof(LONG)

LPVOID MipsSwFunctionTableAccess(LPVOID lpvhpid, DWORD   AddrBase);

// in emdisasm.cpp

LPSTR
_SHGetSymbol(
        LPADDR  addr1,
        SOP             sop,
        LPADDR  addr2,
        LPSTR   szName,
        LPDWORD Delta
        );


XOSD
MipsGetAddr (
    HPID   hpid,
    HTID   htid,
    ADR    adr,
    LPADDR lpaddr
    );

XOSD
MipsSetAddr (
    HPID   hpid,
    HTID   htid,
    ADR    adr,
    LPADDR lpaddr
    );

LPVOID
MipsDoGetReg(
    LPVOID lpregs1,
    DWORD ireg,
    LPVOID lpvRegValue
    );

XOSD
MipsGetRegValue (
    HPID hpid,
    HTID htid,
    DWORD ireg,
    LPVOID lpvRegValue
    );

LPVOID
MipsDoSetReg(
    LPVOID   lpregs1,
    DWORD    ireg,
    LPVOID   lpvRegValue
    );

XOSD
MipsSetRegValue (
    HPID hpid,
    HTID htid,
    DWORD ireg,
    LPVOID lpvRegValue
    );

XOSD
MipsDoGetFrame(
    HPID hpid,
    HTID uhtid,
    DWORD wValue,
    DWORD lValue
    );

XOSD
MipsDoGetFrameEH(
    HPID hpid,
    HTID htid,
    LPEXHDLR *lpexhdlr,
    LPDWORD cAddrsAllocated
);

XOSD
MipsUpdateChild (
    HPID hpid,
    HTID htid,
    DMF dmfCommand
    );

void
MipsCopyFrameRegs(
    LPTHD lpthd,
    LPBPR lpbpr
    );

void
MipsAdjustForProlog(
    HPID hpid,
    HTID htid,
    PADDR origAddr,
    CANSTEP *CanStep
    );

XOSD
MipsDoGetFunctionInfo(
    HPID hpid,
    LPGFI lpgfi
    );

CPU_POINTERS MipsPointers = {
    sizeof(CONTEXT),        //  size_t SizeOfContext;
    MipsRgfd,               //  RGFD * Rgfd;
    MipsRgrd,               //  RD   * Rgrd;
    CMipsRgfd,              //  int    CRgfd;
    CMipsRgrd,              //  int    CRgrd;

    MipsGetAddr,            //  PFNGETADDR          pfnGetAddr;
    MipsSetAddr,            //  PFNSETADDR          pfnSetAddr;
    MipsDoGetReg,           //  PFNDOGETREG         pfnDoGetReg;
    MipsGetRegValue,        //  PFNGETREGVALUE      pfnGetRegValue;
    MipsDoSetReg,           //  PFNSETREG           pfnDoSetReg;
    MipsSetRegValue,        //  PFNSETREGVALUE      pfnSetRegValue;
    XXGetFlagValue,         //  PFNGETFLAG          pfnGetFlagValue;
    XXSetFlagValue,         //  PFNSETFLAG          pfnSetFlagValue;
    MipsDoGetFrame,         //  PFNGETFRAME         pfnGetFrame;
    MipsDoGetFrameEH,       //  PFNGETFRAMEEH       pfnGetFrameEH;
    MipsUpdateChild,        //  PFNUPDATECHILD      pfnUpdateChild;
    MipsCopyFrameRegs,      //  PFNADJUSTFORPROLOG  pfnAdjustForProlog;
    MipsAdjustForProlog,    //  PFNCOPYFRAMEREGS    pfnCopyFrameRegs;
    MipsDoGetFunctionInfo,  //  PFNGETFUNCTIONINFO  pfnGetFunctionInfo;
};




XOSD
MipsGetAddr (
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
    assert ( hpid != NULL );

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

    _fmemset ( lpaddr, 0, sizeof ( ADDR ) );

    if (lpthd && lpthd->fRunning) {
        lpthd->drt = (DRT) (lpthd->drt & ~(drtCntrlPresent | drtAllPresent) );
    }
    if (!fVhtid) {
        switch ( adr ) {

        case adrPC:
            if ( lpthd && !(lpthd->drt & drtCntrlPresent) ) {
                UpdateRegisters ( hprc, hthd );
            }
            break;

        case adrStack:
        case adrBase:
            if ( lpthd && !(lpthd->drt & drtAllPresent )) {
                UpdateRegisters ( hprc, hthd );
            }
            break;
        }

    }
    switch ( adr ) {

    case adrPC:
        if (!fVhtid) {
            AddrInit(lpaddr, 0, 0,
                     (UOFFSET) ((PCONTEXT) (lpthd->regs))->Fir, lpthd->fFlat,
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
                     (UOFFSET) ((PCONTEXT) (lpthd->regs))->IntSp, lpthd->fFlat,
                     lpthd->fOff32, FALSE, lpthd->fReal);
        } else {
            AddrInit(lpaddr, 0, 0,
                    (UOFFSET) lpthd->StackFrame.AddrFrame.Offset, lpthd->fFlat,
                    lpthd->fOff32, FALSE, lpthd->fReal);
        }
        emiAddr (*lpaddr) = (HEMI) hpid;
        break;

    case adrData: {
        UOFFSET offset = 0;
        ADDR addr = {0};

        MipsGetAddr(hpid, htid, adrPC, &addr);
        hmdi = LLFind( LlmdiFromHprc( hprc ), 0, &emiAddr( addr ), emdiEMI);
        if (hmdi != 0) {
            lpmdi = (LPMDI) LLLock( hmdi );
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
                     (UOFFSET) ((PCONTEXT) (lpthd->regs))->IntSp, lpthd->fFlat,
                     lpthd->fOff32, FALSE, lpthd->fReal);
        } else {
            AddrInit(lpaddr, 0, 0,
                     (UOFFSET) lpthd->StackFrame.AddrStack.Offset, lpthd->fFlat,
                     lpthd->fOff32, FALSE, lpthd->fReal);
        }
        emiAddr (*lpaddr) = (HEMI) hpid;
        break;

    default:

        assert ( FALSE );
        break;
    }

    if ( hthd != NULL ) {
        LLUnlock ( hthd );
    }

    return xosd;
}                               /* GetAddr() */


XOSD
MipsSetAddr (
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
    assert ( hpid != NULL );

    hprc = ValidHprcFromHpid(hpid);
    if (!hprc) {
        return xosdBadProcess;
    }

    hthd = HthdFromHtid(hprc, htid);

    if ( hthd != NULL ) {
        lpthd = (LPTHD) LLLock ( hthd );
    }

    if (lpthd && lpthd->fRunning) {
        lpthd->drt = (DRT) (lpthd->drt & ~(drtCntrlPresent | drtAllPresent) );
    }
    switch ( adr ) {
    case adrPC:
        if ( !( lpthd->drt & drtCntrlPresent) ) {
            UpdateRegisters ( hprc, hthd );
        }
        break;

    }

    switch ( adr ) {

    case adrPC:
        ((PCONTEXT) (lpthd->regs))->Fir = GetAddrOff ( *lpaddr );
        lpthd->drt = (DRT) (lpthd->drt | drtCntrlDirty);
        break;

    case adrData:
    case adrTlsBase:
    default:
        assert ( FALSE );
        break;
    }

    if ( hthd != NULL ) {
        LLUnlock ( hthd );
    }

    return xosdNone;
}                               /* SetAddr() */


XOSD
MipsSetAddrFromCSIP (
    HTHD hthd
    )
{

    ADDR addr = {0};
    LPTHD lpthd;

    assert ( hthd != NULL && hthd != hthdInvalid );

    lpthd = (LPTHD) LLLock ( hthd );

    GetAddrSeg ( addr ) = 0;
    GetAddrOff ( addr ) = (UOFFSET) ((PCONTEXT) (lpthd->regs))->Fir;
    emiAddr ( addr ) =  0;
    ADDR_IS_FLAT ( addr ) = TRUE;

    LLUnlock ( hthd );

    return xosdNone;
}




LPVOID
MipsDoGetReg(
    LPVOID lpregs1,
    DWORD  ireg,
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
    LPCONTEXT lpregs = (LPCONTEXT) lpregs1;

    switch ( ireg ) {
    case CV_M4_IntZERO:
    case CV_M4_IntAT:
    case CV_M4_IntV0:
    case CV_M4_IntV1:
    case CV_M4_IntA0:
    case CV_M4_IntA1:
    case CV_M4_IntA2:
    case CV_M4_IntA3:
    case CV_M4_IntT0:
    case CV_M4_IntT1:
    case CV_M4_IntT2:
    case CV_M4_IntT3:
    case CV_M4_IntT4:
    case CV_M4_IntT5:
    case CV_M4_IntT6:
    case CV_M4_IntT7:
    case CV_M4_IntS0:
    case CV_M4_IntS1:
    case CV_M4_IntS2:
    case CV_M4_IntS3:
    case CV_M4_IntS4:
    case CV_M4_IntS5:
    case CV_M4_IntS6:
    case CV_M4_IntS7:
    case CV_M4_IntT8:
    case CV_M4_IntT9:
    case CV_M4_IntKT0:
    case CV_M4_IntKT1:
    case CV_M4_IntGP:
    case CV_M4_IntSP:
    case CV_M4_IntS8:
    case CV_M4_IntRA:
    case CV_M4_IntLO:
    case CV_M4_IntHI:
        *((LPLONG) lpvRegValue) = ((LONG *)(&lpregs->IntZero))[ireg - CV_M4_IntZERO];
        break;

    case CV_M4_Fir:
        *((LPLONG) lpvRegValue) = lpregs->Fir;
        break;

    case CV_M4_Psr:
        *((LPLONG) lpvRegValue) = lpregs->Psr;
        break;

    case CV_M4_FltF0:
    case CV_M4_FltF1:
    case CV_M4_FltF2:
    case CV_M4_FltF3:
    case CV_M4_FltF4:
    case CV_M4_FltF5:
    case CV_M4_FltF6:
    case CV_M4_FltF7:
    case CV_M4_FltF8:
    case CV_M4_FltF9:
    case CV_M4_FltF10:
    case CV_M4_FltF11:
    case CV_M4_FltF12:
    case CV_M4_FltF13:
    case CV_M4_FltF14:
    case CV_M4_FltF15:
    case CV_M4_FltF16:
    case CV_M4_FltF17:
    case CV_M4_FltF18:
    case CV_M4_FltF19:
    case CV_M4_FltF20:
    case CV_M4_FltF21:
    case CV_M4_FltF22:
    case CV_M4_FltF23:
    case CV_M4_FltF24:
    case CV_M4_FltF25:
    case CV_M4_FltF26:
    case CV_M4_FltF27:
    case CV_M4_FltF28:
    case CV_M4_FltF29:
    case CV_M4_FltF30:
    case CV_M4_FltF31:
        *((LPLONG) lpvRegValue) = ((LONG *)(&lpregs->FltF0))[ireg - CV_M4_FltF0];
        break;

    case CV_M4_FltFsr:
        *((LPLONG) lpvRegValue) = lpregs->Fsr;
        break;

    default:
        assert(FALSE);
    }

    lpvRegValue = ( (LPLONG) (lpvRegValue) ) + 1;
    return lpvRegValue;
}


LPVOID
MipsDoSetReg(
    LPVOID   lpregs1,
    DWORD    ireg,
    LPVOID   lpvRegValue
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

    LPCONTEXT lpregs = (LPCONTEXT) lpregs1;

    switch ( ireg ) {
    case CV_M4_IntZERO:
        return NULL;

    case CV_M4_IntAT:
    case CV_M4_IntV0:
    case CV_M4_IntV1:
    case CV_M4_IntA0:
    case CV_M4_IntA1:
    case CV_M4_IntA2:
    case CV_M4_IntA3:
    case CV_M4_IntT0:
    case CV_M4_IntT1:
    case CV_M4_IntT2:
    case CV_M4_IntT3:
    case CV_M4_IntT4:
    case CV_M4_IntT5:
    case CV_M4_IntT6:
    case CV_M4_IntT7:
    case CV_M4_IntS0:
    case CV_M4_IntS1:
    case CV_M4_IntS2:
    case CV_M4_IntS3:
    case CV_M4_IntS4:
    case CV_M4_IntS5:
    case CV_M4_IntS6:
    case CV_M4_IntS7:
    case CV_M4_IntT8:
    case CV_M4_IntT9:
    case CV_M4_IntKT0:
    case CV_M4_IntKT1:
    case CV_M4_IntGP:
    case CV_M4_IntSP:
    case CV_M4_IntS8:
    case CV_M4_IntRA:
    case CV_M4_IntLO:
    case CV_M4_IntHI:

        ((LONG *)(&lpregs->IntZero))[ireg - CV_M4_IntZERO] = *((LPLONG) lpvRegValue);
        break;

    case CV_M4_Fir:
        lpregs->Fir = *((LPLONG) lpvRegValue);
        break;

    case CV_M4_Psr:
        lpregs->Psr = *((LPLONG) lpvRegValue);
        break;

    case CV_M4_FltF0:
    case CV_M4_FltF1:
    case CV_M4_FltF2:
    case CV_M4_FltF3:
    case CV_M4_FltF4:
    case CV_M4_FltF5:
    case CV_M4_FltF6:
    case CV_M4_FltF7:
    case CV_M4_FltF8:
    case CV_M4_FltF9:
    case CV_M4_FltF10:
    case CV_M4_FltF11:
    case CV_M4_FltF12:
    case CV_M4_FltF13:
    case CV_M4_FltF14:
    case CV_M4_FltF15:
    case CV_M4_FltF16:
    case CV_M4_FltF17:
    case CV_M4_FltF18:
    case CV_M4_FltF19:
    case CV_M4_FltF20:
    case CV_M4_FltF21:
    case CV_M4_FltF22:
    case CV_M4_FltF23:
    case CV_M4_FltF24:
    case CV_M4_FltF25:
    case CV_M4_FltF26:
    case CV_M4_FltF27:
    case CV_M4_FltF28:
    case CV_M4_FltF29:
    case CV_M4_FltF30:
    case CV_M4_FltF31:
        ((LONG *)(&lpregs->FltF0))[ireg - CV_M4_FltF0] = *((LPLONG) lpvRegValue);
        break;

    case CV_M4_FltFsr:
        lpregs->Fsr = *((LPLONG) lpvRegValue);
        *((LPLONG) lpvRegValue) = lpregs->Fsr;
        break;

    default:
        assert(FALSE);
        return NULL;
    }

    *((LPLONG)lpvRegValue) = *((LPLONG)lpvRegValue) + 1;

    return lpvRegValue;
}

LPVOID
MipsDoSetFrameReg(
    HPID hpid,
    HTID htid,
    LPTHD lpthd,
    PKNONVOLATILE_CONTEXT_POINTERS contextPtrs,
    DWORD ireg,
    LPVOID lpvRegValue
    )
{

    return MipsDoSetReg( (LPCONTEXT) lpthd->regs, ireg, lpvRegValue);
}


XOSD
MipsDoGetFrame(
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
            lpthd->drt = (DRT) (lpthd->drt & ~(drtCntrlDirty | drtAllDirty) );
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
        if (StackWalk64( IMAGE_FILE_MACHINE_R4000,
                         hpid,
                         htid,
                         &lpthd->StackFrame,
                         lpthd->frameRegs,
                         SwReadMemory,
                         MipsSwFunctionTableAccess,
                         NULL,
                         NULL
                         ))
        {
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
        *((LPHTID)lValue) = fReturnHtid? htid : vhtid;
    }

    LLUnlock( hthd );

    return xosd;
}



PIMAGE_RUNTIME_FUNCTION_ENTRY
LookupFunctionEntry (
    PIMAGE_RUNTIME_FUNCTION_ENTRY FunctionTable,
    DWORD                         NumberOfFunctions,
    DWORD                         ControlPc
    )

/*++

Routine Description:

    This function searches the currently active function tables for an entry
    that corresponds to the specified PC value.

Arguments:

    FunctionTable

    NumberOfFunctions

    ControlPc

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
            return FunctionEntry;
        }
    }

    //
    // A function table entry for the specified PC was not found.
    //

    return NULL;
}

extern HMDI SwGetMdi(HPID,DWORD);

LPVOID
MipsSwFunctionTableAccess(
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

    lpmdi = (LPMDI) LLLock( hmdi );
    if (lpmdi) {
        assert(lpmdi->lpDebug != NULL);
        VerifyDebugDataLoaded(hpid, NULL, lpmdi);       // M00BUG

        rf = LookupFunctionEntry( (PIMAGE_RUNTIME_FUNCTION_ENTRY) lpmdi->lpDebug->lpRtf,
                                  lpmdi->lpDebug->cRtf,
                                  AddrBase
                                );

        LLUnlock( hmdi );
        return (LPVOID)rf;
    }
    return NULL;
}


XOSD
MipsDoGetFunctionInfo(
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
        MipsSwFunctionTableAccess( (LPVOID)hpid, GetAddrOff( *(lpgfi->lpaddr) ) );

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



XOSD
MipsDoGetFrameEH(
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
    HPRC  hprc;
    HTHD  hthd;
    LPTHD lpthd = NULL;
    XOSD  xosd = xosdNone;

    BOOL  fVhtid;
    DWORD AddrBase;
    HMDI                            hmdi  = 0;
    LPMDI                           lpmdi = 0;
    PIMAGE_RUNTIME_FUNCTION_ENTRY   rfe;

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
            lpthd->drt = (DRT) (lpthd->drt & ~(drtCntrlDirty | drtAllDirty) );
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

        rfe = LookupFunctionEntry( (PIMAGE_RUNTIME_FUNCTION_ENTRY) lpmdi->lpDebug->lpRtf,
                                   lpmdi->lpDebug->cRtf,
                                   AddrBase
                                );

        LLUnlock( hmdi );
    }
    if (rfe == NULL) {
        return( xosd );
    }
    /* Look for all functions after the current one that either store
     * their static link or have a prefix of __[F|T|C]
     * Stop looking after both predicates fail
     */
    for (++rfe; rfe < ((PIMAGE_RUNTIME_FUNCTION_ENTRY)lpmdi->lpDebug->lpRtf
                  + lpmdi->lpDebug->cRtf); rfe++) {
        ADDR addr = {0};
        DWORD cb;
        DWORD data;

        ADDR_IS_FLAT(addr) = TRUE;
        ADDR_IS_OFF32(addr) = TRUE;
        if (rfe->PrologEndAddress) {
            addr.addr.off = rfe->PrologEndAddress;
        } else {
            addr.addr.off = rfe->BeginAddress;
        }
        xosd = ReadBuffer( hpid, htid, &addr, sizeof(data), (LPBYTE)&data, &cb);
        if (xosd != xosdNone || cb != sizeof(data)) {
            assert(FALSE);
            break;
        }
        // Does this proc store the static link?
        if ((data&0xffff0000) == 0xafa20000) { // sw v0, ??(sp)
            addr.addr.off += sizeof(DWORD); // skip over sw
            (*lpexhdlr)->addr[(*lpexhdlr)->count++] = addr;
            if ((*lpexhdlr)->count == *cAddrsAllocated) {
                *cAddrsAllocated *= 2;
                *lpexhdlr = (LPEXHDLR) MHRealloc(*lpexhdlr, sizeof(EXHDLR)
                    + *cAddrsAllocated * sizeof(ADDR));
                assert(*lpexhdlr);
            }
        } else { // Try a name check __T __F __C
            ADDR addrT;
            UCHAR fname[512];
            LPCH lpchSymbol;
            DWORD displacement;

            addr.addr.off = rfe->BeginAddress;
            addrT = addr;
            lpchSymbol = _SHGetSymbol( &addrT, (DWORD) sopNone, &addr, (LPSTR) fname, &displacement );
            if (displacement == -1) {
                lpchSymbol = NULL;
            }
            if (lpchSymbol && fname[0] == '_' && fname[1] == '_' && (fname[2] == 'F' ||
                                                                fname[2] == 'T' ||
                                                                ((fname[2] == 'C') && (fname[3] != '_')))) {
                if (rfe->PrologEndAddress) {
                    addr.addr.off = rfe->PrologEndAddress;
                }
                (*lpexhdlr)->addr[(*lpexhdlr)->count++] = addr;
                if ((*lpexhdlr)->count == *cAddrsAllocated) {
                    *cAddrsAllocated *= 2;
                    *lpexhdlr = (LPEXHDLR) MHRealloc(*lpexhdlr, sizeof(EXHDLR)
                        + *cAddrsAllocated * sizeof(ADDR));
                    assert(*lpexhdlr);
                }
            } else {
                break;
            }
        }
    }
    return(xosd);
}


///////////////////////////////////////////////////////////////////
//  part 2.  Additional target dependent
///////////////////////////////////////////////////////////////////

XOSD
MipsUpdateChild (
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
    } else if (pst.dwProcessState != pstRunning) {
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
MipsGetRegValue (
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

            case CV_M4_Fir:
            case CV_M4_IntSP:
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

    lpvRegValue = MipsDoGetReg ( lpregs, ireg & 0xff, lpvRegValue );

    if ( lpvRegValue == NULL ) {
        LLUnlock ( hthd );
        return xosdInvalidParameter;
    }
    ireg = ireg >> 8;

    if ( ireg != CV_REG_NONE ) {
        lpvRegValue = MipsDoGetReg ( lpregs, ireg, lpvRegValue );
        if ( lpvRegValue == NULL ) {
            return xosdInvalidParameter;
        }
    }

    return xosdNone;

}                        /* GetRegValue */



XOSD
MipsSetRegValue (
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

    lpvRegValue = MipsDoSetReg ( (LPCONTEXT) lpregs, ireg & 0xff, lpvRegValue );

    if ( lpvRegValue == NULL ) {
        LLUnlock ( hthd );
        return xosdInvalidParameter;
    }

    ireg = ireg >> 8;
    if ( ireg != 0 ) {
        lpvRegValue = MipsDoSetReg ( (LPCONTEXT) lpregs, ireg, lpvRegValue );
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
MipsCopyFrameRegs(
    LPTHD lpthd,
    LPBPR lpbpr
    )
{
    ((PCONTEXT) (lpthd->regs))->Fir     = lpbpr->offEIP;
    ((PCONTEXT) (lpthd->regs))->IntSp   = lpbpr->offEBP;
}


void
MipsAdjustForProlog(
    HPID hpid,
    HTID htid,
    PADDR origAddr,
    CANSTEP *CanStep
    )
{
    PIMAGE_RUNTIME_FUNCTION_ENTRY rf;
    FixupAddr( hpid, htid, origAddr );
    GetAddrOff(*origAddr) += CanStep->PrologOffset;
    rf = (PIMAGE_RUNTIME_FUNCTION_ENTRY) MipsSwFunctionTableAccess( (LPVOID)hpid, GetAddrOff(*origAddr));
    if (rf && (rf->BeginAddress <= GetAddrOff(*origAddr)) && (rf->PrologEndAddress > GetAddrOff(*origAddr))) {
        DWORD data;
        DWORD cb;
        CanStep->PrologOffset += rf->PrologEndAddress - GetAddrOff(*origAddr);
        GetAddrOff(*origAddr) += rf->PrologEndAddress - GetAddrOff(*origAddr);
        ReadBuffer( hpid, htid, origAddr, sizeof(data), (LPBYTE)&data, &cb);
        assert(cb == sizeof(data));
        if ((data&0xffff0000) == 0xafa20000) { // sw v0, ??(sp)
            CanStep->PrologOffset += sizeof(DWORD);
        }
    }
}


