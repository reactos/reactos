/**** EMDP_IA64.CPP - Debugger end Execution Model (IA64 dependent code)  **
 *                                                                         *
 *  Copyright <C> 1997, Intel Corporation                                  *
 *  Copyright <C> 1990, Microsoft Corp                                     *
 *                                                                         *
 *  Created: September 15, 1997, Vadim Paretsky (VVP)
 *                                                                         *
 *  Revision History:                                                      *
 *      VVP - original revision                                            *
 *                                                                         *
 *  Purpose:                                                               *
 *                                                                         *
 ***************************************************************************/


#define TARGET_IA64
#include "emdp_plt.h"
#include "str_ia64.h"

#include "ehdata.h"


#define WINDBG_POINTERS_MACROS_ONLY
#include "sundown.h"
#undef WINDBG_POINTERS_MACROS_ONLY


RD IA64Rgrd[] = {
#include "regs_ia64.h"
};
unsigned CIA64Rgrd = (sizeof(IA64Rgrd)/sizeof(IA64Rgrd[0]));

RGFD IA64Rgfd[] = {
#include "flag_ia64.h"
};
unsigned CIA64Rgfd = (sizeof(IA64Rgfd)/sizeof(IA64Rgfd[0]));


#define CEXM_MDL_native 0x20

#define SIZEOF_STACK_OFFSET sizeof(LONG)

LPVOID IA64SwFunctionTableAccess(LPVOID lpvhpid, UOFFSET AddrBase);

XOSD
IA64GetAddr (
    HPID   hpid,
    HTID   htid,
    ADR    adr,
    LPADDR lpaddr
    );

XOSD
IA64SetAddr (
    HPID   hpid,
    HTID   htid,
    ADR    adr,
    LPADDR lpaddr
    );

LPVOID
IA64DoGetReg(
    LPVOID lpregs1,
    DWORD ireg,
    LPVOID lpvRegValue
    );

XOSD
IA64GetRegValue (
    HPID hpid,
    HTID htid,
    DWORD ireg,
    LPVOID lpvRegValue
    );

LPVOID
IA64DoSetReg(
    LPVOID   lpthd,
    DWORD    ireg,
    LPVOID   lpvRegValue
    );

XOSD
IA64SetRegValue (
    HPID hpid,
    HTID htid,
    DWORD ireg,
    LPVOID lpvRegValue
    );

XOSD
IA64DoGetFrame(
    HPID hpid,
    HTID uhtid,
    DWORD_PTR wValue,
    DWORD_PTR lValue
    );

XOSD
IA64DoGetFrameEH(
    HPID hpid,
    HTID htid,
    LPEXHDLR *lpexhdlr,
    LPDWORD cAddrsAllocated
);

XOSD
IA64UpdateChild (
    HPID hpid,
    HTID htid,
    DMF dmfCommand
    );

void
IA64CopyFrameRegs(
    LPTHD lpthd,
    LPBPR lpbpr
    );

void
IA64AdjustForProlog(
    HPID hpid,
    HTID htid,
    PADDR origAddr,
    CANSTEP *CanStep
    );

XOSD
IA64DoGetFunctionInfo(
    HPID hpid,
    LPGFI lpgfi
    );

CPU_POINTERS IA64Pointers = {
    sizeof(CONTEXT),         //  size_t SizeOfContext;
    sizeof(STACK_REGISTERS), //  size_t SizeOfStackRegisters;
    IA64Rgfd,                //  RGFD * Rgfd;
    IA64Rgrd,                //  RD   * Rgrd;
    CIA64Rgfd,               //  int    CRgfd;
    CIA64Rgrd,               //  int    CRgrd;

    IA64GetAddr,             //  PFNGETADDR          pfnGetAddr;
    IA64SetAddr,             //  PFNSETADDR          pfnSetAddr;
    IA64DoGetReg,            //  PFNDOGETREG         pfnDoGetReg;
    IA64GetRegValue,         //  PFNGETREGVALUE      pfnGetRegValue;
    IA64DoSetReg,            //  PFNSETREG           pfnDoSetReg;
    IA64SetRegValue,         //  PFNSETREGVALUE      pfnSetRegValue;
    XXGetFlagValue,         //  PFNGETFLAG          pfnGetFlag;
    XXSetFlagValue,         //  PFNSETFLAG          pfnSetFlag;
    IA64DoGetFrame,          //  PFNGETFRAME         pfnGetFrame;
    IA64DoGetFrameEH,        //  PFNGETFRAMEEH       pfnGetFrameEH;
    IA64UpdateChild,         //  PFNUPDATECHILD      pfnUpdateChild;
    IA64CopyFrameRegs,       //  PFNADJUSTFORPROLOG  pfnAdjustForProlog;
    IA64AdjustForProlog,     //  PFNCOPYFRAMEREGS    pfnCopyFrameRegs;
    IA64DoGetFunctionInfo,   //  PFNGETFUNCTIONINFO  pfnGetFunctionInfo;
};




XOSD
IA64GetAddr (
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
    BOOL        fVhtid;

    assert ( lpaddr != NULL );
    assert ( hpid != NULL );

    hprc = ValidHprcFromHpid(hpid);
    if (!hprc) {
        return xosdBadProcess;
    }

    fVhtid = (BOOL)((DWORD_PTR)htid & 1);
    if (fVhtid) {
        htid = (HTID) ((DWORD_PTR)htid & ~1);
    }

   hthd = HthdFromHtid(hprc, htid);

    if ( hthd != NULL ) {
        lpthd = (LPTHD) LLLock ( hthd );
    }

    _fmemset ( lpaddr, 0, sizeof ( ADDR ) );

    switch ( adr ) {

    case adrPC:
        if ( lpthd && !(lpthd->drt & drtCntrlPresent) ) {
            UpdateRegisters ( hprc, hthd );
        }
        break;

    case adrBase:
    case adrData:
        if ( lpthd && !(lpthd->drt & drtAllPresent )) {
            UpdateRegisters ( hprc, hthd );
        }
        break;
    }

    switch ( adr ) {

    case adrPC:
        AddrInit(lpaddr, 0, 0,
			(UOFFSET) ((PCONTEXT) (lpthd->regs))->StIIP |
                            (((PCONTEXT) (lpthd->regs))->StIPSR >> (PSR_RI - 2)) & 0xF,
                 lpthd->fFlat, lpthd->fOff32, FALSE, lpthd->fReal);
        SetEmi ( hpid, lpaddr );
        break;
    case adrData:
        AddrInit(lpaddr, 0, 0, 0,
                 lpthd->fFlat, lpthd->fOff32, FALSE, lpthd->fReal);
        SetEmi ( hpid, lpaddr );
        break;

    case adrBase:
        if (!fVhtid) {
/*            HTID vhtid; //the "frame pointer" - the stack pointer of the previous frame
            IA64DoGetFrame(hpid,htid,2,(UINT_PTR)&vhtid); //get the previous frame
            AddrInit(lpaddr, 0, 0,
                    (UOFFSET) lpthd->StackFrame.AddrFrame.Offset, lpthd->fFlat,
                    lpthd->fOff32, FALSE, lpthd->fReal);*/
            AddrInit(lpaddr, 0, 0,
                     (UOFFSET) ((PCONTEXT) (lpthd->regs))->IntSp, lpthd->fFlat,
                     lpthd->fOff32, FALSE, lpthd->fReal);
        } else {
            AddrInit(lpaddr, 0, 0,
                    (UOFFSET) lpthd->StackFrame.AddrFrame.Offset, lpthd->fFlat,
                    lpthd->fOff32, FALSE, lpthd->fReal);
        }
        SetEmi ( hpid, lpaddr );
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
            AddrInit(lpaddr, 0, 0,
                     (UOFFSET) ((PCONTEXT) (lpthd->regs))->IntSp, lpthd->fFlat,
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

    if ( hthd != NULL ) {
        LLUnlock ( hthd );
    }

    return xosd;
}                               /* GetAddr() */

XOSD
IA64SetAddr (
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

    switch ( adr ) {
    case adrPC:
        if ( !( lpthd->drt & drtCntrlPresent) ) {
            UpdateRegisters ( hprc, hthd );
        }
        break;

    case adrData:
        if ( !(lpthd->drt & drtAllPresent) ) {
            UpdateRegisters ( hprc, hthd );
        }
        break;
    }

    switch ( adr ) {

    case adrPC:
	{
	    // EM address contains two parts: bundle(bit:4-63) and slot(bit:2,3)
        ((PCONTEXT) (lpthd->regs))->StIIP = GetAddrOff (*lpaddr) & ~0xF;
        ((PCONTEXT) (lpthd->regs))->StIPSR &= ~((DWORDLONG)0x3 << PSR_RI);
        ((PCONTEXT) (lpthd->regs))->StIPSR |= ((GetAddrOff (*lpaddr) & 0xF) >> 2) << PSR_RI;

        lpthd->drt = (DRT) (lpthd->drt | drtCntrlDirty);
        break;
	}

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
IA64SetAddrFromCSIP (
    HTHD hthd
    )
{

    ADDR addr = {0};
    LPTHD lpthd;

    assert ( hthd != NULL && hthd != hthdInvalid );

    lpthd = (LPTHD) LLLock ( hthd );
    PCONTEXT p = (PCONTEXT)(lpthd->regs);

    // EM address contains two parts: bundle(bit:4-63) and slot(bit:2,3)
    AddrInit(&addr, 0, 0,
             (UOFFSET)(p->StIIP | (((p->StIPSR >> PSR_RI) & 0x3) << 2)),
             lpthd->fFlat,lpthd->fOff32, FALSE, lpthd->fReal);
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
IA64DoGetReg(
    LPVOID lpv,
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
    LPCONTEXT lpregs = (LPCONTEXT) (((LPTHD)lpv)->regs);
    PSTACK_REGISTERS lpstackregs = (PSTACK_REGISTERS)(((LPTHD)lpv)->pvStackRegs);
	PDWORDLONG pli = (PDWORDLONG)lpvRegValue;
	PFLOAT128 pdv = (PFLOAT128)lpvRegValue;

    switch ( ireg ) {
	case CV_IA64_DbI0:
    case CV_IA64_DbI1:
    case CV_IA64_DbI2:
    case CV_IA64_DbI3:
    case CV_IA64_DbI4:
    case CV_IA64_DbI5:
    case CV_IA64_DbI6:
    case CV_IA64_DbI7:

    case CV_IA64_DbD0:
    case CV_IA64_DbD1:
    case CV_IA64_DbD2:
    case CV_IA64_DbD3:
    case CV_IA64_DbD4:
    case CV_IA64_DbD5:
    case CV_IA64_DbD6:
    case CV_IA64_DbD7:
        *pli = ((DWORDLONG *)(&lpregs->DbI0))[ireg - CV_IA64_DbI0];
        break;

    case CV_IA64_BrRp:
    case CV_IA64_BrS0:
    case CV_IA64_BrS1:
    case CV_IA64_BrS2:
    case CV_IA64_BrS3:
    case CV_IA64_BrS4:
    case CV_IA64_BrT0:
    case CV_IA64_BrT1:
        *pli = ((DWORDLONG *)(&lpregs->BrRp))[ireg - CV_IA64_BrRp];
        break;

    case CV_IA64_Preds:
        *pli = lpregs->Preds;
        break;

    case CV_IA64_IntNats:
        *pli = lpregs->IntNats;
        break;

    case CV_IA64_IntZero:
        assert(FALSE);
        break;

    case CV_IA64_IntGp:
    case CV_IA64_IntT0:
    case CV_IA64_IntT1:
    case CV_IA64_IntS0:
    case CV_IA64_IntS1:
    case CV_IA64_IntS2:
    case CV_IA64_IntS3:
    case CV_IA64_IntV0:
    case CV_IA64_IntT2:
    case CV_IA64_IntT3:
    case CV_IA64_IntT4:
    case CV_IA64_IntSp:
    case CV_IA64_IntTeb:
    case CV_IA64_IntT5:
    case CV_IA64_IntT6:
    case CV_IA64_IntT7:
    case CV_IA64_IntT8:
    case CV_IA64_IntT9:
    case CV_IA64_IntT10:
    case CV_IA64_IntT11:
    case CV_IA64_IntT12:
    case CV_IA64_IntT13:
    case CV_IA64_IntT14:
    case CV_IA64_IntT15:
    case CV_IA64_IntT16:
    case CV_IA64_IntT17:
    case CV_IA64_IntT18:
    case CV_IA64_IntT19:
    case CV_IA64_IntT20:
    case CV_IA64_IntT21:
    case CV_IA64_IntT22:
        *pli = ((DWORDLONG *)(&lpregs->IntGp))[ireg - CV_IA64_IntGp];
        break;

    // all IA64 stack registers and NaTs
    case CV_IA64_IntR32:
    case CV_IA64_IntR33:
    case CV_IA64_IntR34:
    case CV_IA64_IntR35:
    case CV_IA64_IntR36:
    case CV_IA64_IntR37:
    case CV_IA64_IntR38:
    case CV_IA64_IntR39:
    case CV_IA64_IntR40:
    case CV_IA64_IntR41:
    case CV_IA64_IntR42:
    case CV_IA64_IntR43:
    case CV_IA64_IntR44:
    case CV_IA64_IntR45:
    case CV_IA64_IntR46:
    case CV_IA64_IntR47:
    case CV_IA64_IntR48:
    case CV_IA64_IntR49:
    case CV_IA64_IntR50:
    case CV_IA64_IntR51:
    case CV_IA64_IntR52:
    case CV_IA64_IntR53:
    case CV_IA64_IntR54:
    case CV_IA64_IntR55:
    case CV_IA64_IntR56:
    case CV_IA64_IntR57:
    case CV_IA64_IntR58:
    case CV_IA64_IntR59:
    case CV_IA64_IntR60:
    case CV_IA64_IntR61:
    case CV_IA64_IntR62:
    case CV_IA64_IntR63:
    case CV_IA64_IntR64:
    case CV_IA64_IntR65:
    case CV_IA64_IntR66:
    case CV_IA64_IntR67:
    case CV_IA64_IntR68:
    case CV_IA64_IntR69:
    case CV_IA64_IntR70:
    case CV_IA64_IntR71:
    case CV_IA64_IntR72:
    case CV_IA64_IntR73:
    case CV_IA64_IntR74:
    case CV_IA64_IntR75:
    case CV_IA64_IntR76:
    case CV_IA64_IntR77:
    case CV_IA64_IntR78:
    case CV_IA64_IntR79:
    case CV_IA64_IntR80:
    case CV_IA64_IntR81:
    case CV_IA64_IntR82:
    case CV_IA64_IntR83:
    case CV_IA64_IntR84:
    case CV_IA64_IntR85:
    case CV_IA64_IntR86:
    case CV_IA64_IntR87:
    case CV_IA64_IntR88:
    case CV_IA64_IntR89:
    case CV_IA64_IntR90:
    case CV_IA64_IntR91:
    case CV_IA64_IntR92:
    case CV_IA64_IntR93:
    case CV_IA64_IntR94:
    case CV_IA64_IntR95:
    case CV_IA64_IntR96:
    case CV_IA64_IntR97:
    case CV_IA64_IntR98:
    case CV_IA64_IntR99:

    case CV_IA64_IntR100:
    case CV_IA64_IntR101:
    case CV_IA64_IntR102:
    case CV_IA64_IntR103:
    case CV_IA64_IntR104:
    case CV_IA64_IntR105:
    case CV_IA64_IntR106:
    case CV_IA64_IntR107:
    case CV_IA64_IntR108:
    case CV_IA64_IntR109:
    case CV_IA64_IntR110:
    case CV_IA64_IntR111:
    case CV_IA64_IntR112:
    case CV_IA64_IntR113:
    case CV_IA64_IntR114:
    case CV_IA64_IntR115:
    case CV_IA64_IntR116:
    case CV_IA64_IntR117:
    case CV_IA64_IntR118:
    case CV_IA64_IntR119:
    case CV_IA64_IntR120:
    case CV_IA64_IntR121:
    case CV_IA64_IntR122:
    case CV_IA64_IntR123:
    case CV_IA64_IntR124:
    case CV_IA64_IntR125:
    case CV_IA64_IntR126:
    case CV_IA64_IntR127:
		if((ireg - CV_IA64_IntR32) < (lpregs->StIFS & PFS_SIZE_MASK)) { //v-vadimp - is this stack register allocated
			*pli = ((DWORDLONG *)(&lpstackregs->IntR32))[ireg - CV_IA64_IntR32];
		} else {
			*pli = 0xBADBADBADBADBAD;
		}
        break;

    case CV_IA64_IntNats2:
	    *pli = lpstackregs->IntNats2;
        break;

    case CV_IA64_IntNats3:
	    *pli = lpstackregs->IntNats3;
        break;

        break;
    case CV_IA64_ApUNAT:
        *pli = lpregs->ApUNAT;
        break;

    case CV_IA64_ApLC:
        *pli = lpregs->ApLC;
        break;

    case CV_IA64_ApEC:
        *pli = lpregs->ApEC;
        break;

    case CV_IA64_ApCCV:
        *pli = lpregs->ApCCV;
        break;

    case CV_IA64_ApDCR:
        *pli = lpregs->ApDCR;
        break;

    case CV_IA64_RsPFS:
        *pli = lpregs->RsPFS;
        break;

    case CV_IA64_RsBSP:
        *pli = lpregs->RsBSP;
        break;

    case CV_IA64_RsBSPSTORE:
        *pli = lpregs->RsBSPSTORE;
        break;

    case CV_IA64_RsRSC:
        *pli = lpregs->RsRSC;
        break;

    case CV_IA64_RsRNAT:
        *pli = lpregs->RsRNAT;
        break;

    case CV_IA64_StIPSR:
        *pli = lpregs->StIPSR;
        break;

    case CV_IA64_StIIP:
        *pli = lpregs->StIIP;
        break;

    case CV_IA64_StIFS:
        *pli = lpregs->StIFS;
        break;

    case CV_IA64_FltZero:
    case CV_IA64_FltOne:
        assert(FALSE);
        break;

    case CV_IA64_FltS0:
    case CV_IA64_FltS1:
    case CV_IA64_FltS2:
    case CV_IA64_FltS3:
    case CV_IA64_FltT0:
    case CV_IA64_FltT1:
    case CV_IA64_FltT2:
    case CV_IA64_FltT3:
    case CV_IA64_FltT4:
    case CV_IA64_FltT5:
    case CV_IA64_FltT6:
    case CV_IA64_FltT7:
    case CV_IA64_FltT8:
    case CV_IA64_FltT9:
    case CV_IA64_FltS4:
    case CV_IA64_FltS5:
    case CV_IA64_FltS6:
    case CV_IA64_FltS7:
    case CV_IA64_FltS8:
    case CV_IA64_FltS9:
    case CV_IA64_FltS10:
    case CV_IA64_FltS11:
    case CV_IA64_FltS12:
    case CV_IA64_FltS13:
    case CV_IA64_FltS14:
    case CV_IA64_FltS15:
    case CV_IA64_FltS16:
    case CV_IA64_FltS17:
    case CV_IA64_FltS18:
    case CV_IA64_FltS19:

    case CV_IA64_FltF32:
    case CV_IA64_FltF33:
    case CV_IA64_FltF34:
    case CV_IA64_FltF35:
    case CV_IA64_FltF36:
    case CV_IA64_FltF37:
    case CV_IA64_FltF38:
    case CV_IA64_FltF39:
    case CV_IA64_FltF40:
    case CV_IA64_FltF41:
    case CV_IA64_FltF42:
    case CV_IA64_FltF43:
    case CV_IA64_FltF44:
    case CV_IA64_FltF45:
    case CV_IA64_FltF46:
    case CV_IA64_FltF47:
    case CV_IA64_FltF48:
    case CV_IA64_FltF49:
    case CV_IA64_FltF50:
    case CV_IA64_FltF51:
    case CV_IA64_FltF52:
    case CV_IA64_FltF53:
    case CV_IA64_FltF54:
    case CV_IA64_FltF55:
    case CV_IA64_FltF56:
    case CV_IA64_FltF57:
    case CV_IA64_FltF58:
    case CV_IA64_FltF59:
    case CV_IA64_FltF60:
    case CV_IA64_FltF61:
    case CV_IA64_FltF62:
    case CV_IA64_FltF63:
    case CV_IA64_FltF64:
    case CV_IA64_FltF65:
    case CV_IA64_FltF66:
    case CV_IA64_FltF67:
    case CV_IA64_FltF68:
    case CV_IA64_FltF69:
    case CV_IA64_FltF70:
    case CV_IA64_FltF71:
    case CV_IA64_FltF72:
    case CV_IA64_FltF73:
    case CV_IA64_FltF74:
    case CV_IA64_FltF75:
    case CV_IA64_FltF76:
    case CV_IA64_FltF77:
    case CV_IA64_FltF78:
    case CV_IA64_FltF79:
    case CV_IA64_FltF80:
    case CV_IA64_FltF81:
    case CV_IA64_FltF82:
    case CV_IA64_FltF83:
    case CV_IA64_FltF84:
    case CV_IA64_FltF85:
    case CV_IA64_FltF86:
    case CV_IA64_FltF87:
    case CV_IA64_FltF88:
    case CV_IA64_FltF89:
    case CV_IA64_FltF90:
    case CV_IA64_FltF91:
    case CV_IA64_FltF92:
    case CV_IA64_FltF93:
    case CV_IA64_FltF94:
    case CV_IA64_FltF95:
    case CV_IA64_FltF96:
    case CV_IA64_FltF97:
    case CV_IA64_FltF98:
    case CV_IA64_FltF99:

    case CV_IA64_FltF100:
    case CV_IA64_FltF101:
    case CV_IA64_FltF102:
    case CV_IA64_FltF103:
    case CV_IA64_FltF104:
    case CV_IA64_FltF105:
    case CV_IA64_FltF106:
    case CV_IA64_FltF107:
    case CV_IA64_FltF108:
    case CV_IA64_FltF109:
    case CV_IA64_FltF110:
    case CV_IA64_FltF111:
    case CV_IA64_FltF112:
    case CV_IA64_FltF113:
    case CV_IA64_FltF114:
    case CV_IA64_FltF115:
    case CV_IA64_FltF116:
    case CV_IA64_FltF117:
    case CV_IA64_FltF118:
    case CV_IA64_FltF119:
    case CV_IA64_FltF120:
    case CV_IA64_FltF121:
    case CV_IA64_FltF122:
    case CV_IA64_FltF123:
    case CV_IA64_FltF124:
    case CV_IA64_FltF125:
    case CV_IA64_FltF126:
    case CV_IA64_FltF127:

        pdv->LowPart = ((PFLOAT128)(&lpregs->FltS0))[ireg - CV_IA64_FltS0].LowPart;
        pdv->HighPart =((PFLOAT128)(&lpregs->FltS0))[ireg - CV_IA64_FltS0].HighPart;
        break;

    case CV_IA64_StFPSR:
        *pli = lpregs->StFPSR;
        break;

    default:
        assert(!"Unknown register");
		*pli=0xBADBADBADBADBAD;
		break;
    }

    if (*pli == 0xBADBADBADBADBAD) {
		return NULL;
	} else {
		return lpvRegValue;
	}
}


LPVOID
IA64DoSetReg(
    LPVOID lpv,
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
    LPCONTEXT lpregs = (LPCONTEXT)(((LPTHD)lpv)->regs);
    PSTACK_REGISTERS lpstackregs = (PSTACK_REGISTERS)(((LPTHD)lpv)->pvStackRegs);
	PDWORDLONG pli = (PDWORDLONG)lpvRegValue;
	PFLOAT128 pdv = (PFLOAT128)lpvRegValue;

    switch ( ireg ) {
	case CV_IA64_DbI0:
    case CV_IA64_DbI1:
    case CV_IA64_DbI2:
    case CV_IA64_DbI3:
    case CV_IA64_DbI4:
    case CV_IA64_DbI5:
    case CV_IA64_DbI6:
    case CV_IA64_DbI7:

    case CV_IA64_DbD0:
    case CV_IA64_DbD1:
    case CV_IA64_DbD2:
    case CV_IA64_DbD3:
    case CV_IA64_DbD4:
    case CV_IA64_DbD5:
    case CV_IA64_DbD6:
    case CV_IA64_DbD7:
        ((DWORDLONG *)(&lpregs->DbI0))[ireg - CV_IA64_DbI0] = *pli;
        break;

    case CV_IA64_BrRp:
    case CV_IA64_BrS0:
    case CV_IA64_BrS1:
    case CV_IA64_BrS2:
    case CV_IA64_BrS3:
    case CV_IA64_BrT0:
    case CV_IA64_BrT1:
        ((DWORDLONG *)(&lpregs->BrRp))[ireg - CV_IA64_BrRp] = *pli;
        break;

    case CV_IA64_Preds:
        lpregs->Preds = *pli;
        break;

    case CV_IA64_IntNats:
        lpregs->IntNats = *pli;
        break;

    case CV_IA64_IntZero:
        return NULL;

    case CV_IA64_IntGp:
    case CV_IA64_IntT0:
    case CV_IA64_IntT1:
    case CV_IA64_IntS0:
    case CV_IA64_IntS1:
    case CV_IA64_IntS2:
    case CV_IA64_IntS3:
    case CV_IA64_IntV0:
    case CV_IA64_IntT2:
    case CV_IA64_IntT3:
    case CV_IA64_IntT4:
    case CV_IA64_IntSp:
    case CV_IA64_IntTeb:
    case CV_IA64_IntT5:
    case CV_IA64_IntT6:
    case CV_IA64_IntT7:
    case CV_IA64_IntT8:
    case CV_IA64_IntT9:
    case CV_IA64_IntT10:
    case CV_IA64_IntT11:
    case CV_IA64_IntT12:
    case CV_IA64_IntT13:
    case CV_IA64_IntT14:
    case CV_IA64_IntT15:
    case CV_IA64_IntT16:
    case CV_IA64_IntT17:
    case CV_IA64_IntT18:
    case CV_IA64_IntT19:
    case CV_IA64_IntT20:
    case CV_IA64_IntT21:
    case CV_IA64_IntT22:
        ((DWORDLONG *)(&lpregs->IntGp))[ireg - CV_IA64_IntGp] = *pli;
        break;

    // all IA64 stack registers and NaTs
    case CV_IA64_IntR32:
    case CV_IA64_IntR33:
    case CV_IA64_IntR34:
    case CV_IA64_IntR35:
    case CV_IA64_IntR36:
    case CV_IA64_IntR37:
    case CV_IA64_IntR38:
    case CV_IA64_IntR39:
    case CV_IA64_IntR40:
    case CV_IA64_IntR41:
    case CV_IA64_IntR42:
    case CV_IA64_IntR43:
    case CV_IA64_IntR44:
    case CV_IA64_IntR45:
    case CV_IA64_IntR46:
    case CV_IA64_IntR47:
    case CV_IA64_IntR48:
    case CV_IA64_IntR49:
    case CV_IA64_IntR50:
    case CV_IA64_IntR51:
    case CV_IA64_IntR52:
    case CV_IA64_IntR53:
    case CV_IA64_IntR54:
    case CV_IA64_IntR55:
    case CV_IA64_IntR56:
    case CV_IA64_IntR57:
    case CV_IA64_IntR58:
    case CV_IA64_IntR59:
    case CV_IA64_IntR60:
    case CV_IA64_IntR61:
    case CV_IA64_IntR62:
    case CV_IA64_IntR63:
    case CV_IA64_IntR64:
    case CV_IA64_IntR65:
    case CV_IA64_IntR66:
    case CV_IA64_IntR67:
    case CV_IA64_IntR68:
    case CV_IA64_IntR69:
    case CV_IA64_IntR70:
    case CV_IA64_IntR71:
    case CV_IA64_IntR72:
    case CV_IA64_IntR73:
    case CV_IA64_IntR74:
    case CV_IA64_IntR75:
    case CV_IA64_IntR76:
    case CV_IA64_IntR77:
    case CV_IA64_IntR78:
    case CV_IA64_IntR79:
    case CV_IA64_IntR80:
    case CV_IA64_IntR81:
    case CV_IA64_IntR82:
    case CV_IA64_IntR83:
    case CV_IA64_IntR84:
    case CV_IA64_IntR85:
    case CV_IA64_IntR86:
    case CV_IA64_IntR87:
    case CV_IA64_IntR88:
    case CV_IA64_IntR89:
    case CV_IA64_IntR90:
    case CV_IA64_IntR91:
    case CV_IA64_IntR92:
    case CV_IA64_IntR93:
    case CV_IA64_IntR94:
    case CV_IA64_IntR95:
    case CV_IA64_IntR96:
    case CV_IA64_IntR97:
    case CV_IA64_IntR98:
    case CV_IA64_IntR99:

    case CV_IA64_IntR100:
    case CV_IA64_IntR101:
    case CV_IA64_IntR102:
    case CV_IA64_IntR103:
    case CV_IA64_IntR104:
    case CV_IA64_IntR105:
    case CV_IA64_IntR106:
    case CV_IA64_IntR107:
    case CV_IA64_IntR108:
    case CV_IA64_IntR109:
    case CV_IA64_IntR110:
    case CV_IA64_IntR111:
    case CV_IA64_IntR112:
    case CV_IA64_IntR113:
    case CV_IA64_IntR114:
    case CV_IA64_IntR115:
    case CV_IA64_IntR116:
    case CV_IA64_IntR117:
    case CV_IA64_IntR118:
    case CV_IA64_IntR119:
    case CV_IA64_IntR120:
    case CV_IA64_IntR121:
    case CV_IA64_IntR122:
    case CV_IA64_IntR123:
    case CV_IA64_IntR124:
    case CV_IA64_IntR125:
    case CV_IA64_IntR126:
    case CV_IA64_IntR127:
        ((DWORDLONG *)(&lpstackregs->IntR32))[ireg - CV_IA64_IntR32] = *pli;
        break;

    case CV_IA64_IntNats2:
	    lpstackregs->IntNats2 = *pli;
        break;
	
    case CV_IA64_IntNats3:
	    lpstackregs->IntNats3 = *pli;
        break;

    case CV_IA64_ApUNAT:
        lpregs->ApUNAT = *pli;
        break;

    case CV_IA64_ApLC:
        lpregs->ApLC = *pli;
        break;

    case CV_IA64_ApEC:
        lpregs->ApEC = *pli;
        break;

    case CV_IA64_ApCCV:
        lpregs->ApCCV = *pli;
        break;

    case CV_IA64_ApDCR:
        lpregs->ApDCR = *pli;
        break;

    case CV_IA64_RsPFS:
        lpregs->RsPFS = *pli;
        break;

    case CV_IA64_RsBSP:
        lpregs->RsBSP = *pli;
        break;

    case CV_IA64_RsBSPSTORE:
        lpregs->RsBSPSTORE = *pli;
        break;

    case CV_IA64_RsRSC:
        lpregs->RsRSC = *pli;
        break;

    case CV_IA64_RsRNAT:
        lpregs->RsRNAT = *pli;
        break;

    case CV_IA64_StIPSR:
        lpregs->StIPSR = *pli;
        break;

    case CV_IA64_StIIP:
        lpregs->StIIP = *pli;
        break;

    case CV_IA64_StIFS:
        lpregs->StIFS = *pli;
        break;


    case CV_IA64_FltZero:
    case CV_IA64_FltOne:
        return NULL;

    case CV_IA64_FltS0:
    case CV_IA64_FltS1:
    case CV_IA64_FltS2:
    case CV_IA64_FltS3:
    case CV_IA64_FltT0:
    case CV_IA64_FltT1:
    case CV_IA64_FltT2:
    case CV_IA64_FltT3:
    case CV_IA64_FltT4:
    case CV_IA64_FltT5:
    case CV_IA64_FltT6:
    case CV_IA64_FltT7:
    case CV_IA64_FltT8:
    case CV_IA64_FltT9:
    case CV_IA64_FltS4:
    case CV_IA64_FltS5:
    case CV_IA64_FltS6:
    case CV_IA64_FltS7:
    case CV_IA64_FltS8:
    case CV_IA64_FltS9:
    case CV_IA64_FltS10:
    case CV_IA64_FltS11:
    case CV_IA64_FltS12:
    case CV_IA64_FltS13:
    case CV_IA64_FltS14:
    case CV_IA64_FltS15:
    case CV_IA64_FltS16:
    case CV_IA64_FltS17:
    case CV_IA64_FltS18:
    case CV_IA64_FltS19:

    case CV_IA64_FltF32:
    case CV_IA64_FltF33:
    case CV_IA64_FltF34:
    case CV_IA64_FltF35:
    case CV_IA64_FltF36:
    case CV_IA64_FltF37:
    case CV_IA64_FltF38:
    case CV_IA64_FltF39:
    case CV_IA64_FltF40:
    case CV_IA64_FltF41:
    case CV_IA64_FltF42:
    case CV_IA64_FltF43:
    case CV_IA64_FltF44:
    case CV_IA64_FltF45:
    case CV_IA64_FltF46:
    case CV_IA64_FltF47:
    case CV_IA64_FltF48:
    case CV_IA64_FltF49:
    case CV_IA64_FltF50:
    case CV_IA64_FltF51:
    case CV_IA64_FltF52:
    case CV_IA64_FltF53:
    case CV_IA64_FltF54:
    case CV_IA64_FltF55:
    case CV_IA64_FltF56:
    case CV_IA64_FltF57:
    case CV_IA64_FltF58:
    case CV_IA64_FltF59:
    case CV_IA64_FltF60:
    case CV_IA64_FltF61:
    case CV_IA64_FltF62:
    case CV_IA64_FltF63:
    case CV_IA64_FltF64:
    case CV_IA64_FltF65:
    case CV_IA64_FltF66:
    case CV_IA64_FltF67:
    case CV_IA64_FltF68:
    case CV_IA64_FltF69:
    case CV_IA64_FltF70:
    case CV_IA64_FltF71:
    case CV_IA64_FltF72:
    case CV_IA64_FltF73:
    case CV_IA64_FltF74:
    case CV_IA64_FltF75:
    case CV_IA64_FltF76:
    case CV_IA64_FltF77:
    case CV_IA64_FltF78:
    case CV_IA64_FltF79:
    case CV_IA64_FltF80:
    case CV_IA64_FltF81:
    case CV_IA64_FltF82:
    case CV_IA64_FltF83:
    case CV_IA64_FltF84:
    case CV_IA64_FltF85:
    case CV_IA64_FltF86:
    case CV_IA64_FltF87:
    case CV_IA64_FltF88:
    case CV_IA64_FltF89:
    case CV_IA64_FltF90:
    case CV_IA64_FltF91:
    case CV_IA64_FltF92:
    case CV_IA64_FltF93:
    case CV_IA64_FltF94:
    case CV_IA64_FltF95:
    case CV_IA64_FltF96:
    case CV_IA64_FltF97:
    case CV_IA64_FltF98:
    case CV_IA64_FltF99:

    case CV_IA64_FltF100:
    case CV_IA64_FltF101:
    case CV_IA64_FltF102:
    case CV_IA64_FltF103:
    case CV_IA64_FltF104:
    case CV_IA64_FltF105:
    case CV_IA64_FltF106:
    case CV_IA64_FltF107:
    case CV_IA64_FltF108:
    case CV_IA64_FltF109:
    case CV_IA64_FltF110:
    case CV_IA64_FltF111:
    case CV_IA64_FltF112:
    case CV_IA64_FltF113:
    case CV_IA64_FltF114:
    case CV_IA64_FltF115:
    case CV_IA64_FltF116:
    case CV_IA64_FltF117:
    case CV_IA64_FltF118:
    case CV_IA64_FltF119:
    case CV_IA64_FltF120:
    case CV_IA64_FltF121:
    case CV_IA64_FltF122:
    case CV_IA64_FltF123:
    case CV_IA64_FltF124:
    case CV_IA64_FltF125:
    case CV_IA64_FltF126:
    case CV_IA64_FltF127:

        ((PFLOAT128)(&lpregs->FltS0))[ireg - CV_IA64_FltS0].LowPart = pdv->LowPart;
        ((PFLOAT128)(&lpregs->FltS0))[ireg - CV_IA64_FltS0].HighPart = pdv->HighPart;
        break;

    case CV_IA64_StFPSR:
        lpregs->StFPSR = *pli;
        break;

    default:
        assert(FALSE);
        return NULL;
    }

    lpvRegValue = ((PDWORDLONG) lpvRegValue)+1;
//    (PDWORDLONG) lpvRegValue += 1;
    return lpvRegValue;
}

LPVOID
IA64DoSetFrameReg(
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

    return IA64DoSetReg(lpthd, ireg, lpvRegValue);
}

/*UOFFSET
IA64SwGetSectionRVA(
    LPVOID  lpvhpid,
    UOFFSET ReturnAddress
    )
{
    HLLI        hlli  = 0;
    HMDI        hmdi  = 0;
    LPMDI       lpmdi = NULL;
    HPID        hpid = (HPID)lpvhpid;


    hmdi = SwGetMdi( hpid, ReturnAddress );
    if (!hmdi) {
        return 0;
    }

    lpmdi = (LPMDI) LLLock( hmdi );
    if (lpmdi) {

        ADDR addr;
        AddrInit( &addr,0,0, ReturnAddress,TRUE, TRUE, FALSE, FALSE );
        UnFixupAddr(hpid,NULL,&addr);

        return (ReturnAddress - GetAddrOff(addr));
    }

    return 0;
}
*/
XOSD
IA64DoGetFrame(
    HPID hpid,
    HTID uhtid,
    DWORD_PTR wValue,
    DWORD_PTR lValue
    )
{
    HPRC hprc = ValidHprcFromHpid(hpid);
    HTHD hthd;
    LPTHD lpthd;
    HTID htid = (HTID)((DWORD_PTR)uhtid & ~1);
    HTID vhtid = (HTID)((DWORD_PTR)uhtid | 1);
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

        ZeroMemory( &lpthd->StackFrame, sizeof(STACKFRAME) );
        memcpy (lpthd->frameRegs, lpthd->regs, sizeof(CONTEXT) );
        lpthd->frameNumber = 0;
     }

    fGoodFrame = FALSE;
    xosd = xosdNone;
    for (i = 0; xosd == xosdNone && ((wValue != 0)? (i < wValue) : 1); i++) {

        UOFFSET pc    = lpthd->StackFrame.AddrPC.Offset;
        UOFFSET stack = lpthd->StackFrame.AddrFrame.Offset;
		
        if (StackWalk64( IMAGE_FILE_MACHINE_IA64,
                       hpid,
                       htid,
                       &lpthd->StackFrame,
                       lpthd->frameRegs,
                       SwReadMemory,
                       IA64SwFunctionTableAccess,
                       SwGetModuleBase,
                       NULL
                       ))
        {
            if ((pc == lpthd->StackFrame.AddrPC.Offset) &&
                (stack == lpthd->StackFrame.AddrFrame.Offset))  {
                xosd = xosdEndOfStack;
            } else  {
                lpthd->frameNumber++;
                fGoodFrame = TRUE;
            }
        } else  {
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
IA64LookupFunctionEntry (
    PIMAGE_RUNTIME_FUNCTION_ENTRY FunctionTable,
    DWORD                         NumberOfFunctions,
    PIMAGE_RUNTIME_FUNCTION_ENTRY OriginalFunctionTable,
    UOFFSET                       ControlPc
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
            //
            // The capability exists for more than one function entry
            // to map to the same function. This permits a function to
            // have (within reason) discontiguous code segment(s). If
            // PrologEndAddress is out of range, it is re-interpreted
            // as a pointer to the primary function table entry for
            // that function.
            //
#if 0 // v-vadimp we do get something in PrologEndAddress and it's greater than BeginAddress, but what is it?
            if ((FunctionEntry->PrologEndAddress < FunctionEntry->BeginAddress) ||
                (FunctionEntry->PrologEndAddress > FunctionEntry->EndAddress)) {

                //
                // We now have a pointer to the primary function entry - in the
                // target process.  We must adjust it to point to the copy of the
                // function table here in the debugger.
                //
                FunctionEntry =
                    (PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY) ((ULONG)FunctionEntry->PrologEndAddress & ~3) -
                            OriginalFunctionTable +
                            FunctionTable;

            }
#endif
// v-vadimp - this is a temp fix - we are getting 32-bit base relative addresses here and no PrologEndAddress here
// while too many places obviously depend on getting absolute 64-bit addresses and/or PrologEnd, so just fail for now -
// there is usually a workaround in the caller
			/*return NULL;*/ return FunctionEntry;
        }
    }

    //
    // A function table entry for the specified PC was not found.
    //

    return NULL;
}


LPVOID
IA64SwFunctionTableAccess(
    LPVOID  lpvhpid,
    UOFFSET   AddrBase
    )
{
    HLLI                            hlli  = 0;
    HMDI                            hmdi  = 0;
    LPMDI                           lpmdi = 0;
    PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY   rf;
	HPID                            hpid = (HPID)lpvhpid;

    assert( Is64PtrSE(AddrBase) );

    hmdi = SwGetMdi( hpid, AddrBase );
    if (!hmdi) {
        return NULL;
    }

    lpmdi = (LPMDI) LLLock( hmdi );
    if (lpmdi) {

        VerifyDebugDataLoaded(hpid, NULL, lpmdi);       // M00BUG

        assert( Is64PtrSE(lpmdi->lpBaseOfDll) );


        rf = IA64LookupFunctionEntry( (PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY) lpmdi->lpDebug->lpRtf,
                                     lpmdi->lpDebug->cRtf,
                                     (PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY) lpmdi->lpDebug->lpOriginalRtf,
                                     AddrBase - lpmdi->lpBaseOfDll
                                   );


        LLUnlock( hmdi );
        return (LPVOID)rf;
    }
    return NULL;
}


XOSD
IA64DoGetFunctionInfo(
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
    HMDI                            hmdi  = 0;
    LPMDI                           lpmdi = 0;
    PIMAGE_RUNTIME_FUNCTION_ENTRY   pfe;

    hmdi = SwGetMdi( hpid, GetAddrOff( *(lpgfi->lpaddr)));
    if (!hmdi) {
        return NULL;
    }

    lpmdi = (LPMDI) LLLock( hmdi );
    pfe = (PIMAGE_RUNTIME_FUNCTION_ENTRY)IA64SwFunctionTableAccess( (LPVOID)hpid, GetAddrOff( *(lpgfi->lpaddr) ) );

    if (pfe && lpmdi) {

        assert( Is64PtrSE(lpmdi->lpBaseOfDll) );

        VerifyDebugDataLoaded(hpid, NULL, lpmdi);       // M00BUG
        AddrInit( &lpgfi->lpFunctionInformation->AddrStart,     lpmdi->hemi,1, pfe->BeginAddress + lpmdi->lpBaseOfDll,
                  TRUE, TRUE, FALSE, FALSE );
        AddrInit( &lpgfi->lpFunctionInformation->AddrEnd,       lpmdi->hemi,1, pfe->EndAddress + lpmdi->lpBaseOfDll,
                  TRUE, TRUE, FALSE, FALSE );
#if 0  //v-vadimp currently no PrologEndAddress in IMAGE_RUNTIME_FUNCTION_ENTRY
		AddrInit( &lpgfi->lpFunctionInformation->AddrPrologEnd, lpmdi->hemi,1, pfe->PrologEndAddress + lpmdi->lpBaseOfDll,
				TRUE, TRUE, FALSE, FALSE );
#else
		AddrInit( &lpgfi->lpFunctionInformation->AddrPrologEnd, 0,0, 0,
				TRUE, TRUE, FALSE, FALSE );
#endif

    } else {

        xosd = xosdUnknown;
    }


    return xosd;
}

//  This comes directly from ntia64.h.  I could not figure out how to make
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

XOSD
IA64DoGetFrameEH(
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
    UOFFSET AddrBase;
    HMDI                            hmdi  = 0;
    LPMDI                           lpmdi = 0;
    PIMAGE_RUNTIME_FUNCTION_ENTRY   rfe;
    ADDR addr = {0}, lsebAddr, lsehcxtAddr;
    SEH_CONTEXT lsehcxt;
    SEH_BLOCK lseb;
    DWORD cb;
    STACKFRAME newFrame;
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
    fVhtid = (BOOL)((DWORD_PTR)htid & 1);
    if (fVhtid) {
        htid = (HTID) ((DWORD_PTR)htid & ~1);
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
    AddrBase = fVhtid ? lpthd->StackFrame.AddrPC.Offset : ((PCONTEXT) (lpthd->regs))->StIIP;
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

        rfe = IA64LookupFunctionEntry( (PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY) lpmdi->lpDebug->lpRtf,
                                   lpmdi->lpDebug->cRtf,
                                   (PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY) lpmdi->lpDebug->lpOriginalRtf,
                                   AddrBase
                                );

        LLUnlock( hmdi );
    }
    if (rfe == NULL) {
        return( xosd );
    }
    /* Look for handlers.
     */
#if 0 //v-vadimp - no handlers
    if (rfe->HandlerData != NULL) {

        FuncInfo lFuncInfo;

        ADDR_IS_FLAT(addr) = TRUE;
        ADDR_IS_OFF32(addr) = TRUE;

        GetAddrOff(addr) =  (UOFFSET)rfe->ExceptionHandler;

        // Get the virtual frame pointer
        newFrame = lpthd->StackFrame;
        memcpy ( (void *) &newContext, (void *) lpthd->frameRegs, sizeof(CONTEXT) );

        StackWalk( IMAGE_FILE_MACHINE_IA64,
                   hpid,
                   htid,
                   &newFrame,
                   &newContext,
                   SwReadMemory,
                   IA64SwFunctionTableAccess,
                   SwGetModuleBase,
                   NULL
                   );

        GetAddrOff(addr) = (UOFFSET)rfe->HandlerData;

        cb = 0;

        xosd = ReadBuffer(hpid, htid, &addr, sizeof(FuncInfo),
                 (PUCHAR)&lFuncInfo, &cb);

#if DBG
        assert (xosd != xosdNone || cb ==  sizeof(lFuncInfo));
#endif

        if (xosd != xosdNone) {

            // Assume SEH

            lsehcxtAddr = addr;
            GetAddrOff(lsehcxtAddr) = (UOFFSET)(rfe->HandlerData +newFrame.AddrFrame.Offset);
            cb = 0;
            if ((xosd = ReadBuffer(hpid, htid, &lsehcxtAddr, sizeof(SEH_CONTEXT),
                     (PUCHAR)&lsehcxt, &cb)) != xosdNone) {
#if DBG
                GetLastError();
#endif
                return xosd;
            }


            lsebAddr = addr;

            //
            // Do this for each active handler in the function
            //
            for ( GetAddrOff(lsebAddr) = (UOFFSET)lsehcxt.CurrentSeb;
                  GetAddrOff(lsebAddr) ;
                  GetAddrOff(lsebAddr)  = (UOFFSET) lseb.ParentSeb)
            {
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

                        rfe = IA64LookupFunctionEntry( (PIMAGE_RUNTIME_FUNCTION_ENTRY) lpmdi->lpDebug->lpRtf,
                                                   lpmdi->lpDebug->cRtf,
                                                   lpmdi->lpDebug->lpOriginalRtf,
                                                   GetAddrOff(addr)
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

                    AddHandlerEntry(lpexhdlr, cAddrsAllocated, &addr);
                }
            }

        } else if (lFuncInfo.magicNumber == 0x19930520) {

            // Validate the C++ EH specification being handled.
            // If this fails, then support must be added for the new spec.
            // C++ Exception handlers present


            GetAddrOff(addr) = (UOFFSET)(lFuncInfo.EHContextDelta) +newFrame.AddrFrame.Offset);

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

            GetAddrOff(addr) = (UOFFSET)lFuncInfo.pTryBlockMap;

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
                    GetAddrOff(addr) = (UOFFSET)lpTryBlockMap[i].pHandlerArray;

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

                        GetAddrOff(addr) = (UOFFSET) lpHandlerType[j].addressOfHandler;

                        hmdi = SwGetMdi( hpid, addr.addr.off );
                        if (!hmdi) {
                            MHFree( (void *) lpHandlerType);
                            MHFree( (void *) lpTryBlockMap);
                            return xosdBadThread;
                        }

                        lpmdi = (LPMDI) LLLock( hmdi );
                        if (lpmdi) {
                            VerifyDebugDataLoaded(hpid, htid, lpmdi);

                            rfe = IA64LookupFunctionEntry( (PIMAGE_RUNTIME_FUNCTION_ENTRY) lpmdi->lpDebug->lpRtf,
                                                       lpmdi->lpDebug->cRtf,
                                                       lpmdi->lpDebug->lpOriginalRtf,
                                                       GetAddrOff(addr)
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
#endif //0
    return(xosd);
}


///////////////////////////////////////////////////////////
//  part 2.  Additional target dependent
///////////////////////////////////////////////////////////

XOSD
IA64UpdateChild (
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
					assert(!"v-vadimp? are we updating stack registers here?");
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
IA64GetRegValue (
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

    hprc = ValidHprcFromHpid(hpid);
    if (!hprc) {
        return xosdBadProcess;
    }

    if ( (DWORD_PTR)htid & 1 ) {
        return GetFrameRegValue(hpid, (HTID)((DWORD_PTR)htid & ~1), ireg, lpvRegValue);
    }

    hthd = HthdFromHtid(hprc, htid);
    assert ( hthd != hthdNull );

    lpthd = (LPTHD) LLLock ( hthd );
    lpprc = (LPPRC) LLLock( hprc );

    if (lpthd->fRunning) {
        UpdateRegisters( lpthd->hprc, hthd );

        lpthd->fFlat = lpthd->fOff32 = TRUE;
        lpthd->fReal = FALSE;
    } else {

        if ( !(lpthd->drt & drtAllPresent) ) {

            switch ( ireg ) {


            case CV_IA64_StIIP:
            case CV_IA64_StIPSR:
            case CV_IA64_IntSp:
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

    lpvRegValue = IA64DoGetReg ( (LPVOID)lpthd, ireg , lpvRegValue );

    if ( lpvRegValue == NULL ) {
        LLUnlock ( hthd );
        return xosdInvalidParameter;
    }

	return xosdNone;

}                        /* GetRegValue */


XOSD
IA64SetRegValue (
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
    //LPVOID      lpregs = NULL;

    hprc = ValidHprcFromHpid(hpid);
    if (!hprc) {
        return xosdBadProcess;
    }
    hthd = HthdFromHtid(hprc, htid);
    assert ( hthd != hthdNull );

    lpthd = (LPTHD) LLLock ( hthd );

    //lpregs = lpthd->regs; // v-vadimp note, that we are passing lpthd to IA64DoSetReg, unlike other architectures,
							// since we have to pick up 2 register pointers here: regular and stacked

    if ( !(lpthd->drt & drtAllPresent) ) {
        UpdateRegisters ( lpthd->hprc, hthd );
    }

    lpvRegValue = IA64DoSetReg (lpthd, ireg /*& 0xff*/, lpvRegValue ); //0xff - is for x86 clean-up they have 256 regs

    if ( lpvRegValue == NULL ) {
        LLUnlock ( hthd );
        return xosdInvalidParameter;
    }

    lpthd->drt = (DRT) (lpthd->drt | drtAllDirty);

    LLUnlock ( hthd );

    return xosd;

}


void
IA64CopyFrameRegs(
    LPTHD lpthd,
    LPBPR lpbpr
    )
{
    ((PCONTEXT) (lpthd->regs))->StIIP     = lpbpr->offEIP;
    ((PCONTEXT) (lpthd->regs))->IntSp   = lpbpr->offEBP;
}


void
IA64AdjustForProlog(
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


BOOL
UpdateStackRegisters (
    HPRC hprc,
    HTHD hthd
    )
{
    ADDR   addr;
    DWORD  cb, i, j, cbNats;
    DWORDLONG    tmpIntNats2, tmpIntNats3;
    static DWORDLONG TmpStackRegs[sizeof(STACK_REGISTERS)/sizeof(ULONGLONG)];

    LPTHD lpthd = (LPTHD) LLLock ( hthd );
    PSTACK_REGISTERS lpStackRegs = (PSTACK_REGISTERS)lpthd->pvStackRegs;
	DWORD sof = (DWORD)(((PCONTEXT)(lpthd->regs))->StIFS & PFS_SIZE_MASK);  //the low 7 bits is the size of stack frame
    DWORD bspShift = (DWORD)(((PCONTEXT)(lpthd->regs))->RsBSP >> 3) & 0x3f; //this has to do with the RNAT spilling every 63 regs

    if (sof ==0)
	{
        return FALSE;                  // quit if size of no frame
    }

    lpthd->dwcbStackRegs =  (sof + (sof + bspShift) / NAT_BITS_PER_RNAT_REG) * 8;

    // read backing store data to temp area
    AddrInit(&addr, 0, 0, (OFFSET)((PCONTEXT)(lpthd->regs))->RsBSP, lpthd->fFlat, lpthd->fOff32, 0, lpthd->fReal);

    if (ReadBuffer(HpidFromHprc(hprc),HtidFromHthd(hthd),&addr,lpthd->dwcbStackRegs,(LPBYTE)TmpStackRegs,&cb)!= xosdNone)
	{
        lpthd->dwcbStackRegs = 0;
        return FALSE;
    }

    // convert raw rse store data to STACK_REGISTERS struct

    for (i = 0, j = 0, cbNats = 0; i < sof; ) {
        // save NaT collections in temp if store_address{8:3} is equal to 0x3f
        if ((( i + bspShift ) & 0x3f) == 0x3f) {
            if (cbNats == 0) {
                 assert(i<sizeof(STACK_REGISTERS)/sizeof(ULONGLONG));
                 tmpIntNats2 = TmpStackRegs[i++];
            }
            else {
                 assert(i<sizeof(STACK_REGISTERS)/sizeof(ULONGLONG));
                 tmpIntNats3 = TmpStackRegs[i++];
            }
            cbNats++;
        }
        assert(i<sizeof(STACK_REGISTERS)/sizeof(ULONGLONG));
        ((PDWORDLONG)(lpthd->pvStackRegs))[j++] = TmpStackRegs[i++];
    }

    // reconstruct IntNats2 and IntNats3


    switch(cbNats) {
    case 2:
        lpStackRegs->IntNats2 = tmpIntNats2 >> bspShift;
        lpStackRegs->IntNats2 |= tmpIntNats3 << (63 - bspShift);

        lpStackRegs->IntNats3 = tmpIntNats3 >> (bspShift + 1);
        lpStackRegs->IntNats3 |= ((PCONTEXT)lpthd->regs)->RsRNAT << (62 - bspShift);
        break;

    case 1:
        lpStackRegs->IntNats2 = tmpIntNats2 >> bspShift;
        lpStackRegs->IntNats2 |= ((PCONTEXT)lpthd->regs)->RsRNAT << (63 - bspShift);
        lpStackRegs->IntNats3 = 0;
        break;

    case 0:
        lpStackRegs->IntNats2 = ((PCONTEXT)lpthd->regs)->RsRNAT >> bspShift;
        lpStackRegs->IntNats3 = 0;
        break;

    default:
        lpStackRegs->IntNats2 = (DWORDLONG)-1;
        lpStackRegs->IntNats3 = (DWORDLONG)-1;
        break;
    }

    return TRUE;
}
