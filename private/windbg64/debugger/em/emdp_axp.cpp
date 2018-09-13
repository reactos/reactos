/**** EMDPDEV.C - Debugger end Execution Model (ALPHA dependent code)     **
 *                                                                         *
 *  Copyright <C> 1993, Digital Equipment Corporation                      *
 *  Copyright <C> 1990, Microsoft Corp                                     *
 *                                                                         *
 *  Created: January 1, 1993, Miche Baker-Harvey (mbh)
 *                                                                         *
 *  Revision History:                                                      *
 *      MBH - this file is a copy of the mips version, with DoSetReg,      *
 *            DoGetReg replaced with the ALPHA equivalents, and the        *
 *            #ifdef 0 code has been removed.                              *
 *            Taken from MIPS version dated 08-DEC-92.                     *
 *            ReTaken from MIPS version in nt353,                          *
 *                   which contained no #ifdef 0 code                      *
 *                                                                         *
 *  Purpose:                                                               *
 *                                                                         *
 ***************************************************************************/


#define TARGET_ALPHA
#include "emdp_plt.h"
#include "str_axp.h"

#include "ehdata.h"

RD AxpRgrd[] = {
#include "regs_axp.h"
};
unsigned CAxpRgrd = (sizeof(AxpRgrd)/sizeof(AxpRgrd[0]));

RGFD AxpRgfd[] = {
#include "flag_axp.h"
};
unsigned CAxpRgfd = (sizeof(AxpRgfd)/sizeof(AxpRgfd[0]));


#define WINDBG_POINTERS_MACROS_ONLY
#include "sundown.h"
#undef WINDBG_POINTERS_MACROS_ONLY


#define CEXM_MDL_native 0x20

LPVOID
AxpSwFunctionTableAccess(
    LPVOID lpvhpid,
    DWORD64 AddrBase
    );

LPVOID
AxpSwFunctionTableAccessEx(
    LPVOID lpvhpid,
    DWORD64 AddrBase,
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY lpRfe64
    );

XOSD
AxpGetAddr (
    HPID   hpid,
    HTID   htid,
    ADR    adr,
    LPADDR lpaddr
    );

XOSD
AxpSetAddr (
    HPID   hpid,
    HTID   htid,
    ADR    adr,
    LPADDR lpaddr
    );

LPVOID
AxpDoGetReg(
    LPVOID lpregs1,
    DWORD ireg,
    LPVOID lpvRegValue
    );

XOSD
AxpGetRegValue (
    HPID hpid,
    HTID htid,
    DWORD ireg,
    LPVOID lpvRegValue
    );

LPVOID
AxpDoSetReg(
    LPVOID   lpregs1,
    DWORD    ireg,
    LPVOID   lpvRegValue
    );

XOSD
AxpSetRegValue (
    HPID hpid,
    HTID htid,
    DWORD ireg,
    LPVOID lpvRegValue
    );

XOSD
AxpDoGetFrame(
    HPID hpid,
    HTID uhtid,
    DWORD_PTR wValue,
    DWORD_PTR lValue
    );

XOSD
AxpDoGetFrameEH(
    HPID hpid,
    HTID htid,
    LPEXHDLR *lpexhdlr,
    LPDWORD cAddrsAllocated
);

XOSD
AxpUpdateChild (
    HPID hpid,
    HTID htid,
    DMF dmfCommand
    );

void
AxpCopyFrameRegs(
    LPTHD lpthd,
    LPBPR lpbpr
    );

void
AxpAdjustForProlog(
    HPID hpid,
    HTID htid,
    PADDR origAddr,
    CANSTEP *CanStep
    );

XOSD
AxpDoGetFunctionInfo(
    HPID hpid,
    LPGFI lpgfi
    );

CPU_POINTERS AxpPointers = {
    sizeof(CONTEXT),        //  size_t SizeOfContext;
    0,                      //  size_t SizeOfStackRegisters;
    AxpRgfd,                //  RGFD * Rgfd;
    AxpRgrd,                //  RD   * Rgrd;
    CAxpRgfd,               //  int    CRgfd;
    CAxpRgrd,               //  int    CRgrd;

    AxpGetAddr,             //  PFNGETADDR          pfnGetAddr;
    AxpSetAddr,             //  PFNSETADDR          pfnSetAddr;
    AxpDoGetReg,            //  PFNDOGETREG         pfnDoGetReg;
    AxpGetRegValue,         //  PFNGETREGVALUE      pfnGetRegValue;
    AxpDoSetReg,            //  PFNSETREG           pfnDoSetReg;
    AxpSetRegValue,         //  PFNSETREGVALUE      pfnSetRegValue;
    XXGetFlagValue,         //  PFNGETFLAG          pfnGetFlag;
    XXSetFlagValue,         //  PFNSETFLAG          pfnSetFlag;
    AxpDoGetFrame,          //  PFNGETFRAME         pfnGetFrame;
    AxpDoGetFrameEH,        //  PFNGETFRAMEEH       pfnGetFrameEH;
    AxpUpdateChild,         //  PFNUPDATECHILD      pfnUpdateChild;
    AxpCopyFrameRegs,       //  PFNADJUSTFORPROLOG  pfnAdjustForProlog;
    AxpAdjustForProlog,     //  PFNCOPYFRAMEREGS    pfnCopyFrameRegs;
    AxpDoGetFunctionInfo,   //  PFNGETFUNCTIONINFO  pfnGetFunctionInfo;
};




XOSD
AxpGetAddr (
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

    fVhtid = (HandleToLong(htid) & 1);
    if (fVhtid) {
        htid = (HTID)((DWORD_PTR)htid & ~1);
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
            AddrInit(lpaddr, 
                     0, 
                     0,
                     sizeof(DWORD64) == sizeof( ((PCONTEXT) (lpthd->regs))->Fir ) 
                        ? ((PCONTEXT) (lpthd->regs))->Fir
                        : SE32To64( ((PCONTEXT) (lpthd->regs))->Fir ),
                     lpthd->fFlat,
                     lpthd->fOff32, 
                     FALSE, 
                     lpthd->fReal
                     );
        } else {
            AddrInit(lpaddr, 
                     0, 
                     0,
                     lpthd->StackFrame.AddrPC.Offset, 
                     lpthd->fFlat,
                     lpthd->fOff32, 
                     FALSE, 
                     lpthd->fReal
                     );
        }
        SetEmi ( hpid, lpaddr );
        break;

    case adrBase:
        if (!fVhtid) {
            AddrInit(lpaddr, 
                     0, 
                     0,
                     sizeof(DWORD64) == sizeof( ((PCONTEXT) (lpthd->regs))->IntSp )
                        ? ( ((PCONTEXT) (lpthd->regs))->IntSp ) 
                        : SEPtrTo64( ((PCONTEXT) (lpthd->regs))->IntSp ),
                     lpthd->fFlat,
                     lpthd->fOff32, 
                     FALSE, 
                     lpthd->fReal
                     );
        } else {
            AddrInit(lpaddr, 
                     0, 
                     0,
                     lpthd->StackFrame.AddrFrame.Offset, 
                     lpthd->fFlat,
                     lpthd->fOff32, 
                     FALSE, 
                     lpthd->fReal
                     );
        }
        SetEmi ( hpid, lpaddr );
        break;

    case adrData:
        {
            UOFFSET offset = 0;
            ADDR addr = {0};

            AxpGetAddr(hpid, htid, adrPC, &addr);
            hmdi = LLFind( LlmdiFromHprc( hprc ), 0, &emiAddr( addr ), emdiEMI);
            if (hmdi != 0) {
                lpmdi = (LPMDI) LLLock( hmdi);
                offset = lpmdi->lpBaseOfData;
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

            assert( Is64PtrSE(lpmdi->lpBaseOfDll) );

            SendRequestX( dmfQueryTlsBase,
                          hpid, 
                          htid, 
                          sizeof(lpmdi->lpBaseOfDll),
                          &lpmdi->lpBaseOfDll
                          );

            assert( Is64PtrSE(lpmdi->lpBaseOfDll) );


            lpthd->addrTls = *((LPADDR) LpDmMsg->rgb);
            emiAddr(lpthd->addrTls) = hemi;
            LLUnlock( hmdi );

        }

        *lpaddr = lpthd->addrTls;
        emiAddr(*lpaddr) = 0;
        break;

    case adrStack:
        if (!fVhtid) {
            AddrInit(lpaddr, 
                     0, 
                     0,
                     sizeof(DWORD64) == sizeof(((PCONTEXT) (lpthd->regs))->IntSp)
                        ? (((PCONTEXT) (lpthd->regs))->IntSp)
                        : SE32To64( ((PCONTEXT) (lpthd->regs))->IntSp ), 
                     lpthd->fFlat,
                     lpthd->fOff32, 
                     FALSE, 
                     lpthd->fReal
                     );
        } else {
            AddrInit(lpaddr, 
                     0, 
                     0,
                     (UOFFSET) lpthd->StackFrame.AddrStack.Offset, 
                     lpthd->fFlat,
                     lpthd->fOff32, 
                     FALSE, 
                     lpthd->fReal
                     );
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
AxpSetAddr (
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
        ((PCONTEXT) (lpthd->regs))->Fir  = GetAddrOff ( *lpaddr );
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
AxpSetAddrFromCSIP (
    HTHD hthd
    )
{

    ADDR addr = {0};
    LPTHD lpthd;

    assert ( hthd != 0 && hthd != hthdInvalid );

    lpthd = (LPTHD) LLLock ( hthd );

    GetAddrSeg ( addr ) = 0;
    GetAddrOff ( addr ) = (UOFFSET) ((PCONTEXT) (lpthd->regs))->Fir;
    emiAddr ( addr ) =  0;
    ADDR_IS_FLAT ( addr ) = TRUE;

    LLUnlock ( hthd );

    return xosdNone;
}


//
// For DoGetReg, we push the full 64-bit value.
// The correct value is still picked up in the longword
// by the caller.
// This is written to be executable on ALPHA, MIPS and i386
//


LPVOID
AxpDoGetReg(
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

    return-value - lpvRegValue + size of register on success and NULL on
                failure
--*/

{
    PCONTEXT lpregs = (PCONTEXT)lpregs1;
    PDWORDLONG RegArray;
    PDWORDLONG pdwl = (PDWORDLONG)lpvRegValue;
    PDWORD pdw = (PDWORD)lpvRegValue;

    switch ( ireg ) {

    case CV_ALPHA_IntV0 :
    case CV_ALPHA_IntT0 :
    case CV_ALPHA_IntT1 :
    case CV_ALPHA_IntT2 :
    case CV_ALPHA_IntT3 :
    case CV_ALPHA_IntT4 :
    case CV_ALPHA_IntT5 :
    case CV_ALPHA_IntT6 :
    case CV_ALPHA_IntT7 :
    case CV_ALPHA_IntS0 :
    case CV_ALPHA_IntS1 :
    case CV_ALPHA_IntS2 :
    case CV_ALPHA_IntS3 :
    case CV_ALPHA_IntS4 :
    case CV_ALPHA_IntS5 :
    case CV_ALPHA_IntFP :
    case CV_ALPHA_IntA0 :
    case CV_ALPHA_IntA1 :
    case CV_ALPHA_IntA2 :
    case CV_ALPHA_IntA3 :
    case CV_ALPHA_IntA4 :
    case CV_ALPHA_IntA5 :
    case CV_ALPHA_IntT8 :
    case CV_ALPHA_IntT9 :
    case CV_ALPHA_IntT10 :
    case CV_ALPHA_IntT11 :
    case CV_ALPHA_IntRA :
    case CV_ALPHA_IntT12 :
    case CV_ALPHA_IntAT :
    case CV_ALPHA_IntGP :
    case CV_ALPHA_IntSP :
    case CV_ALPHA_IntZERO :

        RegArray  = &lpregs->IntV0;
        *pdwl = RegArray[ireg - CV_ALPHA_IntV0];
        lpvRegValue = (LPVOID)(pdwl + 1);
        break;

    case CV_ALPHA_Fir:

        *pdwl = lpregs->Fir;
        lpvRegValue = (LPVOID)(pdwl + 1);
        break;

    case CV_ALPHA_Psr:

        *pdw = lpregs->Psr;
        lpvRegValue = (LPVOID)(pdw + 1);
        break;

    case CV_ALPHA_Fpcr:
        *pdwl = lpregs->Fpcr;
        lpvRegValue = (LPVOID)(pdwl + 1);
        break;

    case CV_ALPHA_SoftFpcr:
        *pdwl = lpregs->SoftFpcr;
        lpvRegValue = (LPVOID)(pdwl + 1);
        break;

    case CV_ALPHA_FltF0 :
    case CV_ALPHA_FltF1 :
    case CV_ALPHA_FltF2 :
    case CV_ALPHA_FltF3 :
    case CV_ALPHA_FltF4 :
    case CV_ALPHA_FltF5 :
    case CV_ALPHA_FltF6 :
    case CV_ALPHA_FltF7 :
    case CV_ALPHA_FltF8 :
    case CV_ALPHA_FltF9 :
    case CV_ALPHA_FltF10 :
    case CV_ALPHA_FltF11 :
    case CV_ALPHA_FltF12 :
    case CV_ALPHA_FltF13 :
    case CV_ALPHA_FltF14 :
    case CV_ALPHA_FltF15 :
    case CV_ALPHA_FltF16 :
    case CV_ALPHA_FltF17 :
    case CV_ALPHA_FltF18 :
    case CV_ALPHA_FltF19 :
    case CV_ALPHA_FltF20 :
    case CV_ALPHA_FltF21 :
    case CV_ALPHA_FltF22 :
    case CV_ALPHA_FltF23 :
    case CV_ALPHA_FltF24 :
    case CV_ALPHA_FltF25 :
    case CV_ALPHA_FltF26 :
    case CV_ALPHA_FltF27 :
    case CV_ALPHA_FltF28 :
    case CV_ALPHA_FltF29 :
    case CV_ALPHA_FltF30 :
    case CV_ALPHA_FltF31 :

        RegArray  = &lpregs->FltF0;
        *pdwl = RegArray [ireg - CV_ALPHA_FltF0];
        lpvRegValue = (LPVOID)(pdwl + 1);
        break;

    default:
        assert(FALSE);
        return 0;
    }

    return lpvRegValue;

}


LPVOID
AxpDoSetReg(
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
    PCONTEXT lpregs = (PCONTEXT)lpregs1;
    PDWORDLONG RegArray;
    PDWORDLONG pdwl = (PDWORDLONG)lpvRegValue;
    PDWORD pdw = (PDWORD)lpvRegValue;

    switch ( ireg ) {
    case CV_ALPHA_IntZERO :
        return NULL;

    case CV_ALPHA_IntV0 :
    case CV_ALPHA_IntT0 :
    case CV_ALPHA_IntT1 :
    case CV_ALPHA_IntT2 :
    case CV_ALPHA_IntT3 :
    case CV_ALPHA_IntT4 :
    case CV_ALPHA_IntT5 :
    case CV_ALPHA_IntT6 :
    case CV_ALPHA_IntT7 :
    case CV_ALPHA_IntS0 :
    case CV_ALPHA_IntS1 :
    case CV_ALPHA_IntS2 :
    case CV_ALPHA_IntS3 :
    case CV_ALPHA_IntS4 :
    case CV_ALPHA_IntS5 :
    case CV_ALPHA_IntFP :
    case CV_ALPHA_IntA0 :
    case CV_ALPHA_IntA1 :
    case CV_ALPHA_IntA2 :
    case CV_ALPHA_IntA3 :
    case CV_ALPHA_IntA4 :
    case CV_ALPHA_IntA5 :
    case CV_ALPHA_IntT8 :
    case CV_ALPHA_IntT9 :
    case CV_ALPHA_IntT10 :
    case CV_ALPHA_IntT11 :
    case CV_ALPHA_IntRA :
    case CV_ALPHA_IntT12 :
    case CV_ALPHA_IntAT :
    case CV_ALPHA_IntGP :
    case CV_ALPHA_IntSP :

        RegArray  = &lpregs->IntV0;
        RegArray[ireg - CV_ALPHA_IntV0] = *pdwl;
        lpvRegValue = (PVOID)(pdwl + 1);
        break;

    case CV_ALPHA_Fir:
        lpregs->Fir     = *pdwl;
        lpvRegValue = (PVOID)(pdwl + 1);
        break;

    case CV_ALPHA_Psr:
        lpregs->Psr = *pdw;
        lpvRegValue = (PVOID)(pdw + 1);
        break;

    case CV_ALPHA_Fpcr:
        lpregs->Fpcr     = *pdwl;
        lpvRegValue = (PVOID)(pdwl + 1);
        break;

    case CV_ALPHA_SoftFpcr:
        lpregs->SoftFpcr     = *pdwl;
        lpvRegValue = (PVOID)(pdwl + 1);
        break;

    case CV_ALPHA_FltF0 :
    case CV_ALPHA_FltF1 :
    case CV_ALPHA_FltF2 :
    case CV_ALPHA_FltF3 :
    case CV_ALPHA_FltF4 :
    case CV_ALPHA_FltF5 :
    case CV_ALPHA_FltF6 :
    case CV_ALPHA_FltF7 :
    case CV_ALPHA_FltF8 :
    case CV_ALPHA_FltF9 :
    case CV_ALPHA_FltF10 :
    case CV_ALPHA_FltF11 :
    case CV_ALPHA_FltF12 :
    case CV_ALPHA_FltF13 :
    case CV_ALPHA_FltF14 :
    case CV_ALPHA_FltF15 :
    case CV_ALPHA_FltF16 :
    case CV_ALPHA_FltF17 :
    case CV_ALPHA_FltF18 :
    case CV_ALPHA_FltF19 :
    case CV_ALPHA_FltF20 :
    case CV_ALPHA_FltF21 :
    case CV_ALPHA_FltF22 :
    case CV_ALPHA_FltF23 :
    case CV_ALPHA_FltF24 :
    case CV_ALPHA_FltF25 :
    case CV_ALPHA_FltF26 :
    case CV_ALPHA_FltF27 :
    case CV_ALPHA_FltF28 :
    case CV_ALPHA_FltF29 :
    case CV_ALPHA_FltF30 :
    case CV_ALPHA_FltF31 :


        //
        // Transfer the data from the caller.
        // We can do this to DOUBLES because we are doing
        // memory-memory copies.
        //

        RegArray  = &lpregs->FltF0;
        RegArray [ireg - CV_ALPHA_FltF0] = *pdwl;
        lpvRegValue = (PVOID)(pdwl + 1);
        break;

    default:
        assert(FALSE);
        return NULL;
    }

    return lpvRegValue;

}


LPVOID
AxpDoSetFrameReg(
    HPID hpid,
    HTID htid,
    LPTHD lpthd,
    PKNONVOLATILE_CONTEXT_POINTERS contextPtrs,
    DWORD ireg,
    LPVOID lpvRegValue
    )

/*++

Routine Description:

    Sets a register in an old frame; uses context pointers to do it,
    which is why we can't use DoSetReg:
    there's another layer of indirection

Arguments:

    lpregs      - Supplies pointer to pointers to the frame context

    ireg        - Supplies the index of the register to be modified

    lpvRegValue - Supplies the buffer containning the new data

Return Value:

    return-value - the pointer the the next location where a register
        value could be.

--*/

{

    ADDR address;
    LPADDR lpaddr = &address;
    XOSD xosdreturn;
    DWORD cb;

    switch ( ireg ) {

    case CV_ALPHA_IntV0 :
    case CV_ALPHA_IntT0 :
    case CV_ALPHA_IntT1 :
    case CV_ALPHA_IntT2 :
    case CV_ALPHA_IntT3 :
    case CV_ALPHA_IntT4 :
    case CV_ALPHA_IntT5 :
    case CV_ALPHA_IntT6 :
    case CV_ALPHA_IntT7 :
    case CV_ALPHA_IntS0 :
    case CV_ALPHA_IntS1 :
    case CV_ALPHA_IntS2 :
    case CV_ALPHA_IntS3 :
    case CV_ALPHA_IntS4 :
    case CV_ALPHA_IntS5 :
    case CV_ALPHA_IntFP :
    case CV_ALPHA_IntA0 :
    case CV_ALPHA_IntA1 :
    case CV_ALPHA_IntA2 :
    case CV_ALPHA_IntA3 :
    case CV_ALPHA_IntA4 :
    case CV_ALPHA_IntA5 :
    case CV_ALPHA_IntT8 :
    case CV_ALPHA_IntT9 :
    case CV_ALPHA_IntT10 :
    case CV_ALPHA_IntT11 :
    case CV_ALPHA_IntRA :
    case CV_ALPHA_IntT12 :
    case CV_ALPHA_IntAT :
    case CV_ALPHA_IntGP :
    case CV_ALPHA_IntSP :
    case CV_ALPHA_IntZERO :

        //
        // Setup the ADDR structure for where this register was saved
        // on the stack for this frame.
        //

        AddrInit(lpaddr, 
                 0, 
                 0,
#ifdef _WIN64
                 (DWORD64) contextPtrs->IntegerContext[ireg - CV_ALPHA_IntV0],
#else
                 SE32To64( contextPtrs->IntegerContext[ireg - CV_ALPHA_IntV0] ),
#endif
                 lpthd->fFlat,
                 lpthd->fOff32, 
                 FALSE, 
                 lpthd->fReal
                 );

        SetEmi ( hpid, lpaddr );

        xosdreturn = WriteBuffer(hpid, htid, &address, 8, (unsigned char *) lpvRegValue, &cb);

        break;


    case CV_ALPHA_FltF0 :
    case CV_ALPHA_FltF1 :
    case CV_ALPHA_FltF2 :
    case CV_ALPHA_FltF3 :
    case CV_ALPHA_FltF4 :
    case CV_ALPHA_FltF5 :
    case CV_ALPHA_FltF6 :
    case CV_ALPHA_FltF7 :
    case CV_ALPHA_FltF8 :
    case CV_ALPHA_FltF9 :
    case CV_ALPHA_FltF10 :
    case CV_ALPHA_FltF11 :
    case CV_ALPHA_FltF12 :
    case CV_ALPHA_FltF13 :
    case CV_ALPHA_FltF14 :
    case CV_ALPHA_FltF15 :
    case CV_ALPHA_FltF16 :
    case CV_ALPHA_FltF17 :
    case CV_ALPHA_FltF18 :
    case CV_ALPHA_FltF19 :
    case CV_ALPHA_FltF20 :
    case CV_ALPHA_FltF21 :
    case CV_ALPHA_FltF22 :
    case CV_ALPHA_FltF23 :
    case CV_ALPHA_FltF24 :
    case CV_ALPHA_FltF25 :
    case CV_ALPHA_FltF26 :
    case CV_ALPHA_FltF27 :
    case CV_ALPHA_FltF28 :
    case CV_ALPHA_FltF29 :
    case CV_ALPHA_FltF30 :
    case CV_ALPHA_FltF31 :

        //
        // Setup the ADDR structure for where this register was saved
        // on the stack for this frame.
        //


        AddrInit(lpaddr, 
                 0, 
                 0,
#ifdef _WIN64
                 (DWORD64) contextPtrs->FloatingContext[ireg - CV_ALPHA_FltF0],
#else
                 SE32To64( contextPtrs->FloatingContext[ireg - CV_ALPHA_FltF0] ),
#endif
                 lpthd->fFlat,
                 lpthd->fOff32, 
                 FALSE, 
                 lpthd->fReal
                 );

        SetEmi ( hpid, lpaddr );

        xosdreturn = WriteBuffer(hpid, htid, &address, 8, (unsigned char *) lpvRegValue, &cb);

        break;


    default:

        return NULL;
    }

    if ( xosdreturn == xosdNone) {
        return lpvRegValue;
    } else {
        return NULL;
    }
}


XOSD
AxpDoGetFrame(
    HPID hpid,
    HTID uhtid,
    DWORD_PTR wValue,
    DWORD_PTR lValue
    )
{
    HPRC hprc = ValidHprcFromHpid(hpid);
    HTHD hthd;
    LPTHD lpthd;
    HTID htid = (HTID)((UINT_PTR)uhtid & ~1);
    HTID vhtid = (HTID)((UINT_PTR)uhtid | 1);
    DWORD i;
    XOSD xosd;
    BOOL fGoodFrame;
    BOOL fReturnHtid = FALSE;
    DWORD64 validPC;

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
            SendRequestX(dmfWriteReg,
                         hpid,
                         htid,
                         sizeof(CONTEXT),
                         lpthd->regs
                         );
            lpthd->drt = (DRT) (lpthd->drt & ~(drtCntrlDirty|drtAllDirty) );
        }
        UpdateRegisters( hprc, hthd );

        ZeroMemory( &lpthd->StackFrame, sizeof(STACKFRAME64) );
        memcpy (lpthd->frameRegs, lpthd->regs, sizeof(CONTEXT) );
        lpthd->frameNumber = 0;
    }

    validPC = ((PCONTEXT)lpthd->regs)->Fir;
    fGoodFrame = FALSE;
    xosd = xosdNone;
    for (i = 0; xosd == xosdNone && ((wValue != 0)? (i < wValue) : 1); i++) {

        DWORD64 pc    = lpthd->StackFrame.AddrPC.Offset;
        DWORD64 stack = lpthd->StackFrame.AddrFrame.Offset;
        HMDI hmdi = SwGetMdi( hpid, validPC );
        LPMDI lpmdi = 0;
        USHORT machine = 0;

        if (!hmdi) {
            break;
        }

        lpmdi = (LPMDI) LLLock( hmdi );
        if (!lpmdi) {
            break;
        }
        machine = lpmdi->lpDebug->machine;
        LLUnlock( hmdi );

        if (StackWalk64( machine,
                         hpid,
                         htid,
                         &lpthd->StackFrame,
                         lpthd->frameRegs,
                         SwReadMemory,
                         AxpSwFunctionTableAccess,
                         SwGetModuleBase,
                         NULL
                         ))
        {

            //
            // Establish that AddrPC points to return address
            //
            if (lpthd->frameNumber != 0 && lpthd->StackFrame.AddrPC.Offset) {
                lpthd->StackFrame.AddrPC.Offset += 4;
            }

            lpthd->frameNumber++;
            fGoodFrame = TRUE;
        } else {
            xosd = xosdEndOfStack;
        }

        validPC = lpthd->StackFrame.AddrPC.Offset;
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


static
PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY
ConvertFunctionEntry (
    PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY InputFunctionEntry,
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY OutputFunctionEntry
    )

/*++

Routine Description:

    This function converts a 32 bit function entry into a 64 bit function entry
    when called from AxpLookupFunctionEntry

Arguments:

    InputFunctionEntry -

    OutputFunctionEntry -

Return Value:

    The input value cast as a 64 bit function entry.

--*/

{
    COPYSE(OutputFunctionEntry, InputFunctionEntry, BeginAddress);
    COPYSE(OutputFunctionEntry, InputFunctionEntry, EndAddress);
    COPYSE(OutputFunctionEntry, InputFunctionEntry, PrologEndAddress);
    COPYSE(OutputFunctionEntry, InputFunctionEntry, ExceptionHandler);
    COPYSE(OutputFunctionEntry, InputFunctionEntry, HandlerData);
    return (PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY)InputFunctionEntry;
}


PIMAGE_RUNTIME_FUNCTION_ENTRY
AxpLookupFunctionEntry (
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY FunctionTable,
    DWORD                           NumberOfFunctions,
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY OriginalFunctionTable,
    DWORD64                         ControlPc,
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY ConversionFunctionEntry
    )

/*++

Routine Description:

    This function searches the currently active function tables for an entry
    that corresponds to the specified PC value.

Arguments:

    FunctionTable -

    NumberOfFunctions -

    OriginalFunctionTable -

    ControlPc - Supplies the address of an instruction within the specified
        function.

    ConversionFunctionEntry - points to an empty 64 bit Function Entry buffer
        if the function tables are 32 bits, otherwise it is null

Return Value:

    If there is no entry in the function table for the specified PC, then
    NULL is returned. Otherwise, the address of the function table entry
    that corresponds to the specified PC is returned.  Note that pointers to
    both 32 bit and 64 bit function entries are returned, although the 
    return type is equivalent to the 64 bit function entry 
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY.

Side Effects:

    The ConversionFunctionEntry buffer is returned with the function table
    values in 64 bit format if it is specified and if there is a valid function
    table.  Otherwise its value is undefined.

--*/

{

    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY ReturnFunctionEntry;
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY FunctionEntry;
    PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY FunctionEntry32;
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

        if (ConversionFunctionEntry) {

            //
            // Convert the 32 bit function entry into the provided 64 bit entry
            // Return the true 32 bit bit entry in the table, but use
            // the converted function entry in the algorithm
            //

            FunctionEntry32 = (PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY)&FunctionTable[Middle];
            ReturnFunctionEntry = ConvertFunctionEntry( FunctionEntry32, ConversionFunctionEntry );
            FunctionEntry = ConversionFunctionEntry;

        } else {
             ReturnFunctionEntry = &FunctionTable[Middle];
             FunctionEntry = &FunctionTable[Middle];
        }


        if (ControlPc < FunctionEntry->BeginAddress) {
            High = Middle - 1;

        } else if (ControlPc >= FunctionEntry->EndAddress) {
            Low = Middle + 1;

        } else {
            //
            // The capability exists for more than one function entry
            // to map to the same function. This permits a function to
            // have (within reason) discontiguous code segment(s). If
            // PrologEndAddress is out of range, it is re-interpreted
            // as a pointer to the primary function table entry for
            // that function.
            //

            if ((FunctionEntry->PrologEndAddress < FunctionEntry->BeginAddress) ||
                (FunctionEntry->PrologEndAddress > FunctionEntry->EndAddress)) {

                //
                // We now have a pointer to the primary function entry - in the
                // target process.  We must adjust it to point to the copy of the
                // function table here in the debugger.
                //

                if (ConversionFunctionEntry) {

                    //
                    // 32 bit conversion
                    //

                    FunctionEntry32 = 
                        (PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY) ((ULONG64)FunctionEntry->PrologEndAddress & ~3) -
                            (PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY) OriginalFunctionTable +
                            (PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY) FunctionTable;
                    ReturnFunctionEntry = ConvertFunctionEntry( FunctionEntry32, ConversionFunctionEntry );

                } else {

                    //
                    // 64 bit conversion
                    //

                    ReturnFunctionEntry =
                        (PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY) ((ULONG64)FunctionEntry->PrologEndAddress & ~3) -
                            OriginalFunctionTable +
                            FunctionTable;
                }

            }
            return ReturnFunctionEntry;
        }
    }

    // A function table entry for the specified PC was not found.

    return NULL;
}


LPVOID
AxpSwFunctionTableAccess(
    LPVOID  lpvhpid,
    DWORD64 AddrBase
    )
{
    return AxpSwFunctionTableAccessEx( lpvhpid, AddrBase, NULL );
}


LPVOID
AxpSwFunctionTableAccessEx(
    LPVOID  lpvhpid,
    DWORD64 AddrBase,
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY pRfe64
    )

/*++

Routine Description:

    This function returns a pointer to either a 32 bit or 64 bit
    procedure descriptor that corresponds to a given address.
    The function will return the information in canonical 64 bit format
    if a buffer is specified.

Arguments:

    lpvhpid - 

    AddrBase - Address within a function

    pRfe64 - points to an empty 64 bit Function Entry buffer
        if the procedure descriptor info is needed in a canonical format

Return Value:

    If there is no entry in the function table for the specified PC, then
    NULL is returned. Otherwise, the address of the function table entry
    that corresponds to the specified PC is returned.

Side Effects:
    The pRfe64 buffer is filled if it is specified.

--*/

{
    HLLI                            hlli  = 0;
    HMDI                            hmdi  = 0;
    LPMDI                           lpmdi = 0;
    PIMAGE_RUNTIME_FUNCTION_ENTRY   rf;
    HPID                            hpid = (HPID)lpvhpid;
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY Flag64Bit = NULL;
    IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY  Buffer64Bit;


    hmdi = SwGetMdi( hpid, AddrBase );
    if (!hmdi) {
        return NULL;
    }

    lpmdi = (LPMDI) LLLock( hmdi );
    if (lpmdi) {
        VerifyDebugDataLoaded(hpid, NULL, lpmdi);       // M00BUG

        if (lpmdi->lpDebug->machine == IMAGE_FILE_MACHINE_ALPHA) {
            Flag64Bit = &Buffer64Bit;
        }

        rf = AxpLookupFunctionEntry( (PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY) lpmdi->lpDebug->lpRtf,
                                     lpmdi->lpDebug->cRtf,
                                     (PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY) lpmdi->lpDebug->lpOriginalRtf,
                                     AddrBase,
                                     Flag64Bit
                                   );

        LLUnlock( hmdi );
        if (rf && pRfe64) {
            if (Flag64Bit) {
                *pRfe64 = *Flag64Bit;
            } else {
                *pRfe64 = *rf;
            }
        }
        return (LPVOID)rf;
    }
    return NULL;
}


XOSD
AxpDoGetFunctionInfo(
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
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY   pfe;
    IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY   fe;


    pfe = (PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY) AxpSwFunctionTableAccessEx(
              (LPVOID)hpid, GetAddrOff( *(lpgfi->lpaddr) ), &fe );

    if ( pfe ) {

        AddrInit( &lpgfi->lpFunctionInformation->AddrStart,     0,0, fe.BeginAddress,
                  TRUE, TRUE, FALSE, FALSE );
        AddrInit( &lpgfi->lpFunctionInformation->AddrEnd,       0,0, fe.EndAddress,
                  TRUE, TRUE, FALSE, FALSE );
        AddrInit( &lpgfi->lpFunctionInformation->AddrPrologEnd, 0,0, fe.PrologEndAddress,
                  TRUE, TRUE, FALSE, FALSE );

    } else {

        xosd = xosdUnknown;
    }

    return xosd;
}

//
//  The following two variants come directly from ntalpha.h.
//  These are added here for so that EM can manage both 32 bit and 64 bit ALPHA
//  Another difference is that addresses are declared here as signed values,
//  so that address sign extension will work when converting between 32 bit and 64 bit
//

#pragma pack(push, 4)

typedef struct _SEH_BLOCK32 {
    LONG HandlerAddress;
    LONG JumpTarget;
    __int32 ParentSeb;
} SEH_BLOCK32, *PSEH_BLOCK32;

typedef struct _SEH_CONTEXT32 {
    __int32 CurrentSeb;
    ULONG ExceptionCode;
    LONG RealFramePointer;
} SEH_CONTEXT32, *PSEH_CONTEXT32;

#pragma pack(pop)
#pragma pack(push, 8)

typedef struct _SEH_BLOCK64 {
    LONGLONG HandlerAddress;
    LONGLONG JumpTarget;
    LONGLONG ParentSeb;
} SEH_BLOCK64, *PSEH_BLOCK64;

typedef struct _SEH_CONTEXT64 {
    LONGLONG CurrentSeb;
    ULONG ExceptionCode;
    LONGLONG RealFramePointer;
} SEH_CONTEXT64, *PSEH_CONTEXT64;
#pragma pack(pop)

void
AddHandlerEntry(
    LPEXHDLR *lpexhdlr,
    LPDWORD cAddrsAllocated,
    PADDR paddr
    )
{
    if ((*lpexhdlr)->count >= *cAddrsAllocated) {
        *cAddrsAllocated *= 2;
        *lpexhdlr = (LPEXHDLR) MHRealloc(*lpexhdlr, sizeof(EXHDLR)
            + *cAddrsAllocated * sizeof(ADDR));
        assert(*lpexhdlr);
    }
    (*lpexhdlr)->addr[(*lpexhdlr)->count++] = *paddr;
}

static
void
ConvertFuncInfo(
    FuncInfo32* lpInput,
    FuncInfo64* lpOutput
    )
{
    lpOutput->magicNumber = lpInput->magicNumber;
    lpOutput->maxState = lpInput->maxState;
    lpOutput->pUnwindMap = lpInput->pUnwindMap;
    lpOutput->nTryBlocks = lpInput->nTryBlocks;
    lpOutput->pTryBlockMap = lpInput->pTryBlockMap;
    lpOutput->EHContextDelta = lpInput->EHContextDelta;
    lpOutput->nIPMapEntries = lpInput->nIPMapEntries;
    lpOutput->pIPtoStateMap = lpInput->pIPtoStateMap;
}

static
void
ConvertSehCxt(
    SEH_CONTEXT32* lpInput,
    SEH_CONTEXT64* lpOutput
    )
{
    lpOutput->CurrentSeb = lpInput->CurrentSeb;
    lpOutput->ExceptionCode = lpInput->ExceptionCode;
    lpOutput->RealFramePointer = lpInput->RealFramePointer;
}

static
void
ConvertSehBlock(
    SEH_BLOCK32* lpInput,
    SEH_BLOCK64* lpOutput
    )
{
    lpOutput->HandlerAddress = lpInput->HandlerAddress;
    lpOutput->JumpTarget = lpInput->JumpTarget;
    lpOutput->ParentSeb =  lpInput->ParentSeb;
}

static
void
ConvertEHContext(
    EHContext32* lpInput,
    EHContext64* lpOutput
    )
{
    lpOutput->State = lpInput->State;
    lpOutput->Rfp = lpInput->Rfp;
}

static
void
ConvertTryBlockMap(
    TryBlockMapEntry32* lpInput,
    TryBlockMapEntry64* lpOutput,
    unsigned int nCount
    )
{
    unsigned int i;
    for (i = 0; i < nCount; i++ ) {
        lpOutput[i].tryLow = lpInput[i].tryLow;
        lpOutput[i].tryHigh = lpInput[i].tryHigh;
        lpOutput[i].nCatches = lpInput[i].nCatches;
        lpOutput[i].pHandlerArray = lpInput[i].pHandlerArray;
    }
}

static
void
ConvertHandlerType(
    HandlerType32* lpInput,
    HandlerType64* lpOutput,
    unsigned int nCount
    )
{
    unsigned int i;
    for (i = 0; i < nCount; i++ ) {
        lpOutput[i].isConst = lpInput[i].isConst;
        lpOutput[i].isVolatile = lpInput[i].isVolatile;
        lpOutput[i].isUnaligned = lpInput[i].isUnaligned;
        lpOutput[i].isReference = lpInput[i].isReference;
        lpOutput[i].isResumable = lpInput[i].isResumable;
        lpOutput[i].pType = lpInput[i].pType;
        lpOutput[i].dispCatchObj = lpInput[i].dispCatchObj;
        lpOutput[i].addressOfHandler = lpInput[i].addressOfHandler;
    }
}

#pragma message ("DoGetFrameEH needs to be updated for ALPHA")
XOSD
AxpDoGetFrameEH(
    HPID hpid,
    HTID htid,
    LPEXHDLR *lpexhdlr,
    LPDWORD cAddrsAllocated
    )

/*++

Routine Description:

    Fill lpexhdlr with the addresses of all exception handlers for this frame

Arguments

    hpid            - current hpid

    htid            - current htid (may be virtual)

    lpexhdlr        - where to store address

    cAddrsAllocated - how many addresses are currently allocated

Return Value:

    some appropriate xosd

--*/
{
    HPRC  hprc;
    HTHD  hthd;
    LPTHD lpthd = NULL;
    XOSD  xosd = xosdNone;

    BOOL  fVhtid;
    DWORD64 AddrBase;
    HMDI                            hmdi  = 0;
    LPMDI                           lpmdi = 0;
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY   rfe = NULL;
    ADDR addr = {0}, lsebAddr, lsehcxtAddr;
    SEH_CONTEXT64 lsehcxt;
    SEH_BLOCK64 lseb;
    DWORD cb;
    STACKFRAME64 newFrame;
    CONTEXT newContext;
    EHContext64 lEHContext;
    EHContext32 lEHContext32;
    PUCHAR pEHContextBuffer;
    DWORD lEHContextSize;
    TryBlockMapEntry64 *lpTryBlockMap;
    DWORD i;
    HandlerType64 * lpHandlerType;
    DWORD j;
    USHORT machine = IMAGE_FILE_MACHINE_ALPHA;
    PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY Flag64Bit = NULL;
    IMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY  Buffer64Bit;

    hprc = ValidHprcFromHpid(hpid);
    if (!hprc) {
        return xosdBadProcess;
    }
    fVhtid = (HandleToLong(htid) & 1);
    if (fVhtid) {
        htid = (HTID) ((UINT_PTR)htid & ~1);
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

        machine = lpmdi->lpDebug->machine;

        if (machine == IMAGE_FILE_MACHINE_ALPHA) {
            Flag64Bit = &Buffer64Bit;
        }

        rfe = AxpLookupFunctionEntry( (PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY) lpmdi->lpDebug->lpRtf,
                                   lpmdi->lpDebug->cRtf,
                                   (PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY) lpmdi->lpDebug->lpOriginalRtf,
                                   AddrBase,
                                   Flag64Bit
                                );

        if (rfe && Flag64Bit) {
            rfe = Flag64Bit;
        }

        LLUnlock( hmdi );
    }
    if (rfe == NULL) {
        return( xosd );
    }
    /* Look for handlers.
     */
    if (rfe->HandlerData != NULL) {

        FuncInfo64 lFuncInfo;


        ADDR_IS_FLAT(addr) = TRUE;
        ADDR_IS_OFF32(addr) = TRUE;

        addr.addr.off =  (UOFFSET)rfe->ExceptionHandler;

        // Get the virtual frame pointer
        newFrame = lpthd->StackFrame;
        memcpy ( (void *) &newContext, (void *) lpthd->frameRegs, sizeof(CONTEXT) );

        StackWalk64( machine,
                     hpid,
                     htid,
                     &newFrame,
                     &newContext,
                     SwReadMemory,
                     AxpSwFunctionTableAccess,
                     SwGetModuleBase,
                     NULL
                     );

        addr.addr.off = (UOFFSET)rfe->HandlerData;

        cb = 0;

        if (machine == IMAGE_FILE_MACHINE_ALPHA64) {
            xosd = ReadBuffer(hpid, htid, &addr, sizeof(FuncInfo64),
                 (PUCHAR)&lFuncInfo, &cb);
        } else {
            FuncInfo32 lFuncInfo32;
            SEH_CONTEXT32 lsehcxt32;
            xosd = ReadBuffer(hpid, htid, &addr, sizeof(FuncInfo32),
                 (PUCHAR)&lFuncInfo32, &cb);
            ConvertFuncInfo(&lFuncInfo32, &lFuncInfo);
            cb += sizeof(FuncInfo64) - sizeof(FuncInfo32);
        }


#if DBG
        assert (xosd != xosdNone || cb ==  sizeof(FuncInfo64));
#endif

        if (xosd != xosdNone) {

            // Assume SEH

            lsehcxtAddr = addr;
            lsehcxtAddr.addr.off = (UOFFSET)((ULONG64)rfe->HandlerData + newFrame.AddrFrame.Offset);

            // Determine correct machine size

            if (machine == IMAGE_FILE_MACHINE_ALPHA64) {
                xosd = ReadBuffer(hpid, htid, &lsehcxtAddr, sizeof(SEH_CONTEXT64),
                     (PUCHAR)&lsehcxt, &cb);
            } else {
                SEH_CONTEXT32 lsehcxt32;
                xosd = ReadBuffer(hpid, htid, &lsehcxtAddr, sizeof(SEH_CONTEXT32),
                     (PUCHAR)&lsehcxt32, &cb);
                ConvertSehCxt(&lsehcxt32, &lsehcxt);
            }
            cb = 0;
            if (xosd != xosdNone) {
#if DBG
                GetLastError();
#endif
                return xosd;
            }

            lsebAddr = addr;

            //
            // Do this for each active handler in the function
            //
            for ( lsebAddr.addr.off = (UOFFSET)lsehcxt.CurrentSeb;
                  lsebAddr.addr.off;
                  lsebAddr.addr.off = (UOFFSET) lseb.ParentSeb)
            {
                cb = 0;

                if (machine == IMAGE_FILE_MACHINE_ALPHA64) {
                    xosd = ReadBuffer(hpid, htid, &lsebAddr, sizeof(SEH_BLOCK64),
                         (PUCHAR)&lseb, &cb);
                } else {
                    SEH_BLOCK32 lseb32;
                    xosd = ReadBuffer(hpid, htid, &lsebAddr, sizeof(SEH_BLOCK32),
                         (PUCHAR)&lseb32, &cb);
                    ConvertSehBlock(&lseb32, &lseb);
                }
                if ((xosd ) != xosdNone) {
#if DBG
                    GetLastError();
#endif
                    return xosd;
                }

                if (lseb.JumpTarget) {

                    addr.addr.off = lseb.JumpTarget;
                    AddHandlerEntry(lpexhdlr, cAddrsAllocated, &addr);

                } else if (lseb.HandlerAddress) {
                    addr.addr.off = lseb.HandlerAddress;

                    hmdi = SwGetMdi( hpid, addr.addr.off );

                    if (!hmdi) {
                        return xosdBadThread;
                    }

                    lpmdi = (LPMDI) LLLock( hmdi );
                    if (lpmdi) {
                        VerifyDebugDataLoaded(hpid, htid, lpmdi);

                        machine = lpmdi->lpDebug->machine;

                        if (machine == IMAGE_FILE_MACHINE_ALPHA) {
                            Flag64Bit = &Buffer64Bit;
                        } else {
                            Flag64Bit = NULL;
                        }

                        rfe = AxpLookupFunctionEntry( (PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY) lpmdi->lpDebug->lpRtf,
                                                   lpmdi->lpDebug->cRtf,
                                                   (PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY) lpmdi->lpDebug->lpOriginalRtf,
                                                   addr.addr.off,
                                                   Flag64Bit
                                                );

                        if (rfe && Flag64Bit) {
                            rfe = Flag64Bit;
                        }

                        LLUnlock( hmdi );
                    }
                    if (rfe == NULL) {
                        return( xosd );
                    }

                    // Need to get rfe for the lpseh
                    if (rfe->PrologEndAddress) {
                        addr.addr.off = rfe->PrologEndAddress & ~0x3;
                    }

                    AddHandlerEntry(lpexhdlr, cAddrsAllocated, &addr);
                }
            }

        } else if (lFuncInfo.magicNumber == 0x19930520) {

            // Validate the C++ EH specification being handled.
            // If this fails, then support must be added for the new spec.
            // C++ Exception handlers present


            addr.addr.off = (ULONG_PTR)lFuncInfo.EHContextDelta + newFrame.AddrFrame.Offset;

            cb = 0;

            if (machine == IMAGE_FILE_MACHINE_ALPHA64) {
                xosd = ReadBuffer(hpid, htid, &addr, sizeof(EHContext64),
                     (unsigned char *)&lEHContext, &cb);
                lEHContextSize = sizeof(EHContext64);
            } else {
                xosd = ReadBuffer(hpid, htid, &addr, sizeof(EHContext32),
                     (unsigned char *)&lEHContext32, &cb);
                cb += sizeof(EHContext64) - sizeof(EHContext32);
                ConvertEHContext(&lEHContext32, &lEHContext);
            }

            if ((xosd ) != xosdNone) {
#if DBG
                GetLastError();
#endif
                return xosd;
            }

#if DBG
            assert (cb == sizeof(EHContext64));
#endif

            // We need to read the TryBlockMap, but in order to do so,
            //   we first need to get the first word which is the count of
            //   entries in the MAP.

            addr.addr.off = (UOFFSET)lFuncInfo.pTryBlockMap;

            // Make sure there are try blocks
            if (lFuncInfo.nTryBlocks == 0) {
                assert (lFuncInfo.pUnwindMap);
                return xosd;
            }

            lpTryBlockMap = (TryBlockMapEntry64 *) MHAlloc(lFuncInfo.nTryBlocks *
                                sizeof(TryBlockMapEntry64));

            cb = 0;

            if (machine == IMAGE_FILE_MACHINE_ALPHA64) {
                xosd = ReadBuffer(hpid, htid, &addr, lFuncInfo.nTryBlocks *
                     sizeof(TryBlockMapEntry64), (unsigned char *)lpTryBlockMap, &cb);
            } else {
                TryBlockMapEntry32 *lpTryBlockMap32;
                lpTryBlockMap32 = (TryBlockMapEntry32 *) MHAlloc(lFuncInfo.nTryBlocks *
                                sizeof(TryBlockMapEntry32));
                xosd = ReadBuffer(hpid, htid, &addr, lFuncInfo.nTryBlocks *
                     sizeof(TryBlockMapEntry32), (unsigned char *)lpTryBlockMap32, &cb);
                cb += lFuncInfo.nTryBlocks * (sizeof(TryBlockMapEntry64) - sizeof(TryBlockMapEntry32));
                ConvertTryBlockMap(lpTryBlockMap32, lpTryBlockMap, lFuncInfo.nTryBlocks);
                MHFree( (void *) lpTryBlockMap32);
            }

            if (xosd != xosdNone) {
#if DBG
                GetLastError();
#endif
                MHFree( (void *) lpTryBlockMap);
                return xosd;
            }

#if DBG
            assert (cb ==  (lFuncInfo.nTryBlocks *  sizeof(TryBlockMapEntry)));
#endif

            for (i=0; i < lFuncInfo.nTryBlocks; ++i) {

                if ((lEHContext.State >= (UOFFSET)lpTryBlockMap[i].tryLow) &&
                    (lEHContext.State <= (UOFFSET)lpTryBlockMap[i].tryHigh)) {

                    assert (lpTryBlockMap[i].nCatches);

                    lpHandlerType = (HandlerType64 *) MHAlloc(lpTryBlockMap[i].nCatches *
                                        sizeof(HandlerType64));

#if DBG
                    cb = 0;
#endif
                    addr.addr.off = (UOFFSET)lpTryBlockMap[i].pHandlerArray;

                    if (machine == IMAGE_FILE_MACHINE_ALPHA64) {
                        xosd = ReadBuffer(hpid, htid, &addr, lpTryBlockMap[i].nCatches *
                               sizeof(HandlerType64), (unsigned char *)lpHandlerType, &cb);
                    } else {
                        HandlerType32 * lpHandlerType32;
                        lpHandlerType32 = (HandlerType32 *) MHAlloc(lpTryBlockMap[i].nCatches *
                               sizeof(HandlerType32));
                        xosd = ReadBuffer(hpid, htid, &addr, lpTryBlockMap[i].nCatches *
                               sizeof(HandlerType32), (unsigned char *)lpHandlerType32, &cb);
                        cb +=  lpTryBlockMap[i].nCatches * (sizeof(HandlerType64) - sizeof(HandlerType32));
                        ConvertHandlerType(lpHandlerType32, lpHandlerType, lpTryBlockMap[i].nCatches);
                        MHFree( (void *) lpHandlerType32);
                    }

                    if ((xosd) != xosdNone) {
#if DBG
                        GetLastError();
#endif
                        MHFree( (void *) lpHandlerType);
                        MHFree( (void *) lpTryBlockMap);
                        return xosd;
                    }
#if DBG
                    assert (cb ==  (lpTryBlockMap[i].nCatches * sizeof(HandlerType64)));
#endif

                    // This is a good scope, so lets start setting breakpoints
                    for (j=0; j < (DWORD)lpTryBlockMap[i].nCatches; ++j) {

                        assert (lpHandlerType[j].addressOfHandler);

                        addr.addr.off = (UOFFSET) lpHandlerType[j].addressOfHandler;

                        hmdi = SwGetMdi( hpid, addr.addr.off );
                        if (!hmdi) {
                            MHFree( (void *) lpHandlerType);
                            MHFree( (void *) lpTryBlockMap);
                            return xosdBadThread;
                        }

                        lpmdi = (LPMDI) LLLock( hmdi );

                        if (lpmdi) {
                            VerifyDebugDataLoaded(hpid, htid, lpmdi);

                            machine = lpmdi->lpDebug->machine;

                            if (machine == IMAGE_FILE_MACHINE_ALPHA) {
                                Flag64Bit = &Buffer64Bit;
                            } else {
                                Flag64Bit = NULL;
                            }

                            rfe = AxpLookupFunctionEntry( (PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY) lpmdi->lpDebug->lpRtf,
                                                       lpmdi->lpDebug->cRtf,
                                                       (PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY) lpmdi->lpDebug->lpOriginalRtf,
                                                       addr.addr.off,
                                                       Flag64Bit
                                                    );

                            if (rfe && Flag64Bit) {
                                rfe = Flag64Bit;
                            }

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
                        AddHandlerEntry(lpexhdlr, cAddrsAllocated, &addr);
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
}


///////////////////////////////////////////////////////////
//  part 2.  Additional target dependent
///////////////////////////////////////////////////////////

XOSD
AxpUpdateChild (
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
AxpGetRegValue (
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

    if ( HandleToLong(htid) & 1 ) {
        return GetFrameRegValue(hpid, (HTID)((DWORD_PTR)htid & ~1), ireg, lpvRegValue);
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


            case CV_ALPHA_Fir:
            case CV_ALPHA_IntSP:
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

    lpvRegValue = AxpDoGetReg ( lpregs, ireg & 0xff, lpvRegValue );

    if ( lpvRegValue == NULL ) {
        LLUnlock ( hthd );
        return xosdInvalidParameter;
    }
    ireg = ireg >> 8;

    if ( ireg != CV_REG_NONE ) {
        lpvRegValue = AxpDoGetReg ( lpregs, ireg, lpvRegValue );
        if ( lpvRegValue == NULL ) {
            return xosdInvalidParameter;
        }
    }

    return xosdNone;

}                        /* GetRegValue */


XOSD
AxpSetRegValue (
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

    lpvRegValue = AxpDoSetReg ( (LPCONTEXT) lpregs, ireg & 0xff, lpvRegValue );

    if ( lpvRegValue == NULL ) {
        LLUnlock ( hthd );
        return xosdInvalidParameter;
    }

    ireg = ireg >> 8;
    if ( ireg != 0 ) {
        lpvRegValue = AxpDoSetReg ( (LPCONTEXT) lpregs, ireg, lpvRegValue );
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
AxpCopyFrameRegs(
    LPTHD lpthd,
    LPBPR lpbpr
    )
{
    ((PCONTEXT) (lpthd->regs))->Fir     = lpbpr->offEIP;
    ((PCONTEXT) (lpthd->regs))->IntSp   = lpbpr->offEBP;
}


void
AxpAdjustForProlog(
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
