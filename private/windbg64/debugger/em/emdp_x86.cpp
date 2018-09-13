/**** EMDPDEV.C - Debugger end Execution Model (x86 dependent code)       **
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

#define TARGET_i386
#include "emdp_plt.h"
#include "str_x86.h"
#include "x86regs.h"

#include <wdbgexts.h>

RD X86Rgrd[] = {
#include "regs_x86.h"
};
const unsigned CX86Rgrd = (sizeof(X86Rgrd)/sizeof(X86Rgrd[0]));

RGFD X86Rgfd[] = {
#include "flag_x86.h"
};
const unsigned CX86Rgfd = (sizeof(X86Rgfd)/sizeof(X86Rgfd[0]));


#define WINDBG_POINTERS_MACROS_ONLY
#include "sundown.h"
#undef WINDBG_POINTERS_MACROS_ONLY

typedef struct _L_DOUBLE {
    BYTE b[10];
} L_DOUBLE, FAR *LPL_DOUBLE;


#define CEXM_MDL_native 0x20


#ifdef TARGET32
#define SIZEOF_STACK_OFFSET sizeof(LONG)
#else // TARGET32
#define SIZEOF_STACK_OFFSET sizeof(WORD)
#endif // TARGET32

LPVOID
X86SwFunctionTableAccess(
    LPVOID lpvhpid,
    DWORD64 AddrBase
    );

void
X86UpdateSpecialRegisters (
    HPRC hprc,
    HTHD hthd
    );

XOSD
X86GetAddr (
    HPID   hpid,
    HTID   htid,
    ADR    adr,
    LPADDR lpaddr
    );

XOSD
X86SetAddr (
    HPID   hpid,
    HTID   htid,
    ADR    adr,
    LPADDR lpaddr
    );

LPVOID
X86DoGetReg(
    LPVOID lpregs1,
    DWORD ireg,
    LPVOID lpvRegValue
    );

XOSD
X86GetRegValue (
    HPID hpid,
    HTID htid,
    DWORD ireg,
    LPVOID lpvRegValue
    );

LPVOID
X86DoSetReg(
    LPVOID   lpregs1,
    DWORD    ireg,
    LPVOID   lpvRegValue
    );

XOSD
X86SetRegValue (
    HPID hpid,
    HTID htid,
    DWORD ireg,
    LPVOID lpvRegValue
    );

XOSD
X86DoGetFrame(
    HPID hpid,
    HTID uhtid,
    DWORD_PTR wValue,
    DWORD_PTR lValue
    );

XOSD
X86DoGetFrameEH(
    HPID hpid,
    HTID htid,
    LPEXHDLR *lpexhdlr,
    LPDWORD cAddrsAllocated
);

XOSD
X86UpdateChild (
    HPID hpid,
    HTID htid,
    DMF dmfCommand
    );

void
X86CopyFrameRegs(
    LPTHD lpthd,
    LPBPR lpbpr
    );

void
X86AdjustForProlog(
    HPID hpid,
    HTID htid,
    PADDR origAddr,
    CANSTEP *CanStep
    );

XOSD
X86DoGetFunctionInfo(
    HPID hpid,
    LPGFI lpgfi
    );

CPU_POINTERS X86Pointers = {
    sizeof(CONTEXT),        //  size_t SizeOfContext;
    0,                      //  size_t SizeOfStackRegisters;
    X86Rgfd,                //  RGFD * Rgfd;
    X86Rgrd,                //  RD   * Rgrd;
    CX86Rgfd,               //  int    CRgfd;
    CX86Rgrd,               //  int    CRgrd;

    X86GetAddr,             //  PFNGETADDR          pfnGetAddr;
    X86SetAddr,             //  PFNSETADDR          pfnSetAddr;
    X86DoGetReg,            //  PFNDOGETREG         pfnDoGetReg;
    X86GetRegValue,         //  PFNGETREGVALUE      pfnGetRegValue;
    X86DoSetReg,            //  PFNSETREG           pfnDoSetReg;
    X86SetRegValue,         //  PFNSETREGVALUE      pfnSetRegValue;
    XXGetFlagValue,         //  PFNGETFLAG          pfnGetFlagValue;
    XXSetFlagValue,         //  PFNSETFLAG          pfnSetFlagValue;
    X86DoGetFrame,          //  PFNGETFRAME         pfnGetFrame;
    X86DoGetFrameEH,        //  PFNGETFRAMEEH       pfnGetFrameEH;
    X86UpdateChild,         //  PFNUPDATECHILD      pfnUpdateChild;
    X86CopyFrameRegs,       //  PFNCOPYFRAMEREGS    pfnCopyFrameRegs;
    X86AdjustForProlog,     //  PFNADJUSTFORPROLOG  pfnAdjustForProlog;
    X86DoGetFunctionInfo,   //  PFNGETFUNCTIONINFO  pfnGetFunctionInfo;
};



XOSD
X86GetAddr (
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
    HPRC        hprc;
    HTHD        hthd;
    LPTHD       lpthd = NULL;
    XOSD        xosd = xosdNone;
    HEMI        hemi = emiAddr(*lpaddr);
    HMDI        hmdi;
    LPMDI       lpmdi;
    BOOL        fVhtid;

    assert ( lpaddr != NULL );
    assert ( hpid != NULL );

    hprc = ValidHprcFromHpid(hpid);
    if (!hprc) {
        return xosdBadProcess;
    }

    fVhtid = (HandleToLong(htid) & 1);
    if (fVhtid) {
        htid = (HTID) ((DWORD_PTR)htid & ~1);
    }

    hthd = HthdFromHtid(hprc, htid);

    if ( hthd != hthdNull ) {
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

        case adrBase:
        case adrStack:
        case adrData:
            if ( lpthd && !(lpthd->drt & drtAllPresent )) {
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
                         (SEGMENT) ((PCONTEXT) (lpthd->regs))->SegCs,
                         SE32To64( ((PCONTEXT) (lpthd->regs))->Eip ), 
                         lpthd->fFlat,
                         lpthd->fOff32, 
                         FALSE, 
                         lpthd->fReal
                         );
            } else {
                AddrInit(lpaddr, 
                         0, 
                         (SEGMENT) ((PCONTEXT) (lpthd->regs))->SegCs,
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

            //
            // Danger: ugly code alert to do with virtual frames [apennell] VS98:6296
            // It turns out that DbgHelp sometimes gives us a zero as the result,
            // mostly gives us the correct value, but on no-EBP functions it gives
            // us a value that is 4 too small. This code handles all of these cases.
            //

            if (!fVhtid || lpthd->StackFrame.AddrFrame.Offset == 0 ) {
                AddrInit(lpaddr, 
                         0, 
                         (SEGMENT) 0,
                         SE32To64( ((PCONTEXT) (lpthd->regs))->Ebp ),
                         lpthd->fFlat, 
                         lpthd->fOff32, 
                         FALSE, 
                         lpthd->fReal
                         );
            } else {
                UOFFSET uBase = lpthd->StackFrame.AddrFrame.Offset;
                PFPO_DATA pFpo = (PFPO_DATA)lpthd->StackFrame.FuncTableEntry;
                if (pFpo && pFpo->cbFrame == FRAME_FPO) {
                    uBase += 4;
                }
                AddrInit(lpaddr, 
                         0, 
                         (SEGMENT) 0,
                         uBase,
                         lpthd->fFlat, 
                         lpthd->fOff32, 
                         FALSE, 
                         lpthd->fReal
                         );
            }
            SetEmi ( hpid, lpaddr );
            break;


break;

        case adrData:
            AddrInit(lpaddr, 0, (SEGMENT) ((PCONTEXT) (lpthd->regs))->SegDs, 0,
                     lpthd->fFlat, lpthd->fOff32, FALSE, lpthd->fReal);
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
                              &lpmdi->lpBaseOfDll);

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
                         (SEGMENT) ((PCONTEXT) (lpthd->regs))->SegSs,
                         SE32To64( ((PCONTEXT) (lpthd->regs))->Esp ), 
                         lpthd->fFlat,
                         lpthd->fOff32, 
                         FALSE, 
                         lpthd->fReal
                         );
            } else {
                AddrInit(lpaddr, 
                         0, 
                         (SEGMENT) ((PCONTEXT) (lpthd->regs))->SegSs,
                         lpthd->StackFrame.AddrStack.Offset,
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

    if ( hthd != hthdNull ) {
        LLUnlock ( hthd );
    }

    return xosd;
}                               /* GetAddr() */


XOSD
X86SetAddr (
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

    assert ( (HandleToLong(htid) & 1) == 0 );

    hprc = ValidHprcFromHpid(hpid);
    if (!hprc) {
        return xosdBadProcess;
    }

    hthd = HthdFromHtid(hprc, htid);


    if ( hthd != hthdNull ) {
        lpthd = (LPTHD) LLLock ( hthd );
    }

    switch ( adr ) {
        case adrPC:
            if ( !( lpthd->drt & drtCntrlPresent) ) {
                UpdateRegisters ( hprc, hthd );
            }
            break;


        case adrBase:
        case adrStack:
        case adrData:
            if ( !(lpthd->drt & drtAllPresent) ) {
                UpdateRegisters ( hprc, hthd );
            }
            break;

    }
    switch ( adr ) {
        case adrPC:
            ((PCONTEXT) (lpthd->regs))->SegCs = GetAddrSeg ( *lpaddr );
            ((PCONTEXT) (lpthd->regs))->Eip = (DWORD)GetAddrOff ( *lpaddr );
            lpthd->drt = (DRT) (lpthd->drt | drtCntrlDirty);
            break;

        case adrBase:
            ((PCONTEXT) (lpthd->regs))->Ebp = (DWORD)GetAddrOff ( *lpaddr );
            lpthd->drt = (DRT) (lpthd->drt | drtAllDirty);
            break;

        case adrStack:
            ((PCONTEXT) (lpthd->regs))->SegSs = GetAddrSeg ( *lpaddr );
            ((PCONTEXT) (lpthd->regs))->Esp = (DWORD)GetAddrOff ( *lpaddr );
            lpthd->drt = (DRT) (lpthd->drt | drtAllDirty);
            break;

        case adrData:
        case adrTlsBase:
        default:
            assert ( FALSE );
            break;
    }

    if ( hthd != hthdNull ) {
        LLUnlock ( hthd );
    }

    return xosdNone;
}                               /* SetAddr() */


XOSD
X86SetAddrFromCSIP (
    HTHD hthd
    )
{

    ADDR addr = {0};
    LPTHD lpthd;

    assert ( hthd != hthdNull && hthd != hthdInvalid );

    lpthd = (LPTHD) LLLock ( hthd );

    GetAddrSeg ( addr ) = (SEGMENT) ((PCONTEXT) (lpthd->regs))->SegCs;
    GetAddrOff ( addr ) = (UOFFSET) ((PCONTEXT) (lpthd->regs))->Eip;
    emiAddr ( addr ) =  0;
    ADDR_IS_FLAT ( addr ) = TRUE;

    LLUnlock ( hthd );

    return xosdNone;
}


LPVOID
X86DoGetReg(
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
    int         i;
    LPCONTEXT  lpregs = (LPCONTEXT) lpregs1;

    if (!lpregs) {
        return NULL;
    }

    switch ( ireg ) {

    case iregAL:
        *( (LPBYTE) lpvRegValue ) = (BYTE) lpregs->Eax;
        break;

    case iregCL:
        *( (LPBYTE) lpvRegValue ) = (BYTE) lpregs->Ecx;
        break;

    case iregDL:
        *( (LPBYTE) lpvRegValue ) = (BYTE) lpregs->Edx;
        break;

    case iregBL:
        *( (LPBYTE) lpvRegValue ) = (BYTE) lpregs->Ebx;
        break;

    case iregAH:
        *( (LPBYTE) lpvRegValue ) = (BYTE) (lpregs->Eax >> 8);
        break;

    case iregCH:
        *( (LPBYTE) lpvRegValue ) = (BYTE) (lpregs->Ecx >> 8);
        break;

    case iregDH:
        *( (LPBYTE) lpvRegValue ) = (BYTE) (lpregs->Edx >> 8);
        break;

    case iregBH:
        *( (LPBYTE) lpvRegValue ) = (BYTE) (lpregs->Ebx >> 8);
        break;

    case iregAX:
        *( (LPWORD) lpvRegValue ) = (WORD) lpregs->Eax;
        break;

    case iregCX:
        *( (LPWORD) lpvRegValue ) = (WORD) lpregs->Ecx;
        break;

    case iregDX:
        *( (LPWORD) lpvRegValue ) = (WORD) lpregs->Edx;
        break;

    case iregBX:
        *( (LPWORD) lpvRegValue ) = (WORD) lpregs->Ebx;
        break;

    case iregSP:
        *( (LPWORD) lpvRegValue ) = (WORD) lpregs->Esp;
        break;

    case iregBP:
        *( (LPWORD) lpvRegValue ) = (WORD) lpregs->Ebp;
        break;

    case iregSI:
        *( (LPWORD) lpvRegValue ) = (WORD) lpregs->Esi;
        break;

    case iregDI:
        *( (LPWORD) lpvRegValue ) = (WORD) lpregs->Edi;
        break;

    case iregIP:
        *( (LPWORD) lpvRegValue ) = (WORD) lpregs->Eip;
        break;

    case iregFLAGS:
        *( (LPWORD) lpvRegValue ) = (WORD) lpregs->EFlags;
        break;

    case iregES:
        *( (LPWORD) lpvRegValue ) = (SEG16) lpregs->SegEs;
        break;

    case iregCS:
        *( (LPWORD) lpvRegValue ) = (SEG16) lpregs->SegCs;
        break;

    case iregSS:
        *( (LPWORD) lpvRegValue ) = (SEG16) lpregs->SegSs;
        break;

    case iregDS:
        *( (LPWORD) lpvRegValue ) = (SEG16) lpregs->SegDs;
        break;

    case iregFS:
        *( (LPWORD) lpvRegValue ) = (SEG16) lpregs->SegFs;
        break;

    case iregGS:
        *( (LPWORD) lpvRegValue ) = (SEG16) lpregs->SegGs;
        break;

    case iregEAX:
        *( (LPLONG) lpvRegValue ) = lpregs->Eax;
        break;

    case iregECX:
        *( (LPLONG) lpvRegValue ) = lpregs->Ecx;
        break;

    case iregEDX:
        *( (LPLONG) lpvRegValue ) = lpregs->Edx;
        break;

    case iregEBX:
        *( (LPLONG) lpvRegValue ) = lpregs->Ebx;
        break;

    case iregESP:
        *( (LPLONG) lpvRegValue ) = lpregs->Esp;
        break;

    case iregEBP:
        *( (LPLONG) lpvRegValue ) = lpregs->Ebp;
        break;

    case iregESI:
        *( (LPLONG) lpvRegValue ) = lpregs->Esi;
        break;

    case iregEDI:
        *( (LPLONG) lpvRegValue ) = lpregs->Edi;
        break;

    case iregEIP:
        *( (LPLONG) lpvRegValue ) = lpregs->Eip;
        break;

    case iregEFLAGS:
        *( (LPLONG) lpvRegValue ) = lpregs->EFlags;
        break;

    case iregST0:
    case iregST1:
    case iregST2:
    case iregST3:
    case iregST4:
    case iregST5:
    case iregST6:
    case iregST7:

//        i = (lpregs->FloatSave.StatusWord >> 11) & 0x7;
//        i = (i + ireg - iregST0) % 8;

          i = ireg - iregST0;

        *( (LPL_DOUBLE) lpvRegValue ) =
          ((LPL_DOUBLE)(lpregs->FloatSave.RegisterArea))[ i ];
        break;

    case iregCTRL:
        *( (LPLONG) lpvRegValue ) =  lpregs->FloatSave.ControlWord;
        break;

    case iregSTAT:
        *( (LPLONG) lpvRegValue ) =  lpregs->FloatSave.StatusWord;
        break;

    case iregTAG:
        *( (LPLONG) lpvRegValue ) =  lpregs->FloatSave.TagWord;
        break;

    case iregFPIP:
        *( (LPWORD) lpvRegValue ) =  (OFF16) lpregs->FloatSave.ErrorOffset;
        break;

    case CV_REG_FPEIP:
        *( (LPLONG) lpvRegValue ) =  lpregs->FloatSave.ErrorOffset;
        break;

    case iregFPCS:
        *( (LPWORD) lpvRegValue ) =  (SEG16) lpregs->FloatSave.ErrorSelector;
        break;

    case iregFPDO:
        *( (LPLONG) lpvRegValue ) =  (OFF16) lpregs->FloatSave.DataOffset;
        break;

    case CV_REG_FPEDO:
        *( (LPLONG) lpvRegValue ) =  lpregs->FloatSave.DataOffset;
        break;

    case iregFPDS:
        *( (LPWORD) lpvRegValue ) =  (SEG16) lpregs->FloatSave.DataSelector;
        break;

#define lpsr ((PKSPECIAL_REGISTERS)lpregs)
    case CV_REG_GDTR:
        *( (LPDWORD) lpvRegValue ) = lpsr->Gdtr.Base;
        break;

    case CV_REG_GDTL:
        *( (LPWORD) lpvRegValue ) = lpsr->Gdtr.Limit;
        break;

    case CV_REG_IDTR:
        *( (LPDWORD) lpvRegValue ) = lpsr->Idtr.Base;
        break;

    case CV_REG_IDTL:
        *( (LPWORD) lpvRegValue ) = lpsr->Idtr.Limit;
        break;

    case CV_REG_LDTR:
        *( (LPWORD) lpvRegValue ) = lpsr->Ldtr;
        break;

    case CV_REG_TR:
        *( (LPWORD) lpvRegValue ) = lpsr->Tr;
        break;

    case CV_REG_CR0:
        *( (LPDWORD) lpvRegValue ) = lpsr->Cr0;
        break;

    case CV_REG_CR2:
        *( (LPDWORD) lpvRegValue ) = lpsr->Cr2;
        break;

    case CV_REG_CR3:
        *( (LPDWORD) lpvRegValue ) = lpsr->Cr3;
        break;

    case CV_REG_CR4:
        *( (LPDWORD) lpvRegValue ) = lpsr->Cr4;
        break;
#undef lpsr

    case CV_REG_DR0:
        *( (PULONG) lpvRegValue ) = lpregs->Dr0;
        break;

    case CV_REG_DR1:
        *( (PULONG) lpvRegValue ) = lpregs->Dr1;
        break;

    case CV_REG_DR2:
        *( (PULONG) lpvRegValue ) = lpregs->Dr2;
        break;

    case CV_REG_DR3:
        *( (PULONG) lpvRegValue ) = lpregs->Dr3;
        break;

    case CV_REG_DR6:
        *( (PULONG) lpvRegValue ) = lpregs->Dr6;
        break;

    case CV_REG_DR7:
        *( (PULONG) lpvRegValue ) = lpregs->Dr7;
        break;

    }

    switch ( ireg ) {

    case iregAL:
    case iregCL:
    case iregDL:
    case iregBL:
    case iregAH:
    case iregCH:
    case iregDH:
    case iregBH:

        lpvRegValue = (LPBYTE) (lpvRegValue) + sizeof ( BYTE ) ;
        break;

    case iregAX:
    case iregCX:
    case iregDX:
    case iregBX:
    case iregSP:
    case iregBP:
    case iregSI:
    case iregDI:
    case iregIP:
    case iregFLAGS:
    case iregES:
    case iregCS:
    case iregSS:
    case iregDS:
    case iregFS:
    case iregGS:
    case iregFPCS:
    case iregFPDS:
    case iregCTRL:
    case iregSTAT:
    case iregTAG:
    case iregFPIP:
    case iregFPDO:

    case CV_REG_GDTL:
    case CV_REG_IDTL:
    case CV_REG_LDTR:
    case CV_REG_TR:

        lpvRegValue = (LPBYTE) (lpvRegValue) + sizeof ( WORD ) ;

        break;

    case iregEAX:
    case iregECX:
    case iregEDX:
    case iregEBX:
    case iregESP:
    case iregEBP:
    case iregESI:
    case iregEDI:
    case iregEIP:
    case iregEFLAGS:
    case CV_REG_FPEIP:
    case CV_REG_FPEDO:

    case CV_REG_CR0:
    case CV_REG_CR1:
    case CV_REG_CR2:
    case CV_REG_CR3:
    case CV_REG_CR4:

    case CV_REG_DR0:
    case CV_REG_DR1:
    case CV_REG_DR2:
    case CV_REG_DR3:
    case CV_REG_DR4:
    case CV_REG_DR5:
    case CV_REG_DR6:
    case CV_REG_DR7:

    case CV_REG_GDTR:
    case CV_REG_IDTR:

        lpvRegValue = (LPBYTE) (lpvRegValue) + sizeof ( LONG ) ;
        break;

    case iregST0:
    case iregST1:
    case iregST2:
    case iregST3:
    case iregST4:
    case iregST5:
    case iregST6:
    case iregST7:

        lpvRegValue = (LPBYTE) (lpvRegValue) + sizeof ( L_DOUBLE ) ;
        break;

    default:
        lpvRegValue = NULL;
        break;
    }

    return lpvRegValue;
}                               /* DoGetReg() */


LPVOID
X86DoSetReg(
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
    int         i;
    LPCONTEXT   lpregs = (LPCONTEXT) lpregs1;

    switch ( ireg ) {

    case iregAL:
        lpregs->Eax = (lpregs->Eax & 0xFFFFFF00) | *( (LPBYTE) lpvRegValue );
        break;

    case iregCL:
        lpregs->Ecx = (lpregs->Ecx & 0xFFFFFF00) | *( (LPBYTE) lpvRegValue );
        break;

    case iregDL:
        lpregs->Edx = (lpregs->Edx & 0xFFFFFF00) | *( (LPBYTE) lpvRegValue );
        break;

    case iregBL:
        lpregs->Ebx = (lpregs->Ebx & 0xFFFFFF00) | *( (LPBYTE) lpvRegValue );
        break;

    case iregAH:
        lpregs->Eax = (lpregs->Eax & 0xFFFF00FF) |
          (((WORD) *( (LPBYTE) lpvRegValue )) << 8);
        break;

    case iregCH:
        lpregs->Ecx = (lpregs->Ecx & 0xFFFF00FF) |
          (((WORD) *( (LPBYTE) lpvRegValue )) << 8);
        break;

    case iregDH:
        lpregs->Edx = (lpregs->Edx & 0xFFFF00FF) |
          (((WORD) *( (LPBYTE) lpvRegValue )) << 8);
        break;

    case iregBH:
        lpregs->Ebx = (lpregs->Ebx & 0xFFFF00FF) |
          (((WORD) *( (LPBYTE) lpvRegValue )) << 8);
        break;

    case iregAX:
        lpregs->Eax = (lpregs->Eax & 0xFFFF0000) | *( (LPWORD) lpvRegValue );
        break;

    case iregCX:
        lpregs->Ecx = (lpregs->Ecx & 0xFFFF0000) | *( (LPWORD) lpvRegValue );
        break;

    case iregDX:
        lpregs->Edx = (lpregs->Edx & 0xFFFF0000) | *( (LPWORD) lpvRegValue );
        break;

    case iregBX:
        lpregs->Ebx = (lpregs->Ebx & 0xFFFF0000) | *( (LPWORD) lpvRegValue );
        break;

    case iregSP:
        lpregs->Esp = (lpregs->Esp & 0xFFFF0000) | *( (LPWORD) lpvRegValue );
        break;

    case iregBP:
        lpregs->Ebp = (lpregs->Ebp & 0xFFFF0000) | *( (LPWORD) lpvRegValue );
        break;

    case iregSI:
        lpregs->Esi = (lpregs->Esi & 0xFFFF0000) | *( (LPWORD) lpvRegValue );
        break;

    case iregDI:
        lpregs->Edi = (lpregs->Edi & 0xFFFF0000) | *( (LPWORD) lpvRegValue );
        break;

    case iregIP:
        lpregs->Eip = (lpregs->Eip & 0xFFFF0000) | *( (LPWORD) lpvRegValue );
        break;

    case iregFLAGS:
        lpregs->EFlags = (lpregs->EFlags & 0xFFFF0000 ) | *( (LPWORD) lpvRegValue );
        break;

    case iregES:
        lpregs->SegEs = *( (LPWORD) lpvRegValue );
        break;

    case iregCS:
        lpregs->SegCs = *( (LPWORD) lpvRegValue );
        break;

    case iregSS:
        lpregs->SegSs = *( (LPWORD) lpvRegValue );
        break;

    case iregDS:
        lpregs->SegDs = *( (LPWORD) lpvRegValue );
        break;

    case iregFS:
        lpregs->SegFs = *( (LPWORD) lpvRegValue );
        break;

    case iregGS:
        lpregs->SegGs = *( (LPWORD) lpvRegValue );
        break;

    case iregEAX:
        lpregs->Eax = *( (LPLONG) lpvRegValue );
        break;

    case iregECX:
        lpregs->Ecx = *( (LPLONG) lpvRegValue );
        break;

    case iregEDX:
        lpregs->Edx = *( (LPLONG) lpvRegValue );
        break;

    case iregEBX:
        lpregs->Ebx = *( (LPLONG) lpvRegValue );
        break;

    case iregESP:
        lpregs->Esp = *( (LPLONG) lpvRegValue );
        break;

    case iregEBP:
        lpregs->Ebp = *( (LPLONG) lpvRegValue );
        break;

    case iregESI:
        lpregs->Esi = *( (LPLONG) lpvRegValue );
        break;

    case iregEDI:
        lpregs->Edi = *( (LPLONG) lpvRegValue );
        break;

    case iregEIP:
        lpregs->Eip = *( (LPLONG) lpvRegValue );
        break;

    case iregEFLAGS:
        lpregs->EFlags = *( (LPLONG) lpvRegValue );
        break;

    case iregST0:
    case iregST1:
    case iregST2:
    case iregST3:
    case iregST4:
    case iregST5:
    case iregST6:
    case iregST7:
//        i = (lpregs->FloatSave.StatusWord >> 11) & 0x7;
//        i = (i + ireg - iregST0) % 8;
        i = ireg - iregST0;
        memcpy(&lpregs->FloatSave.RegisterArea[10*(i)], lpvRegValue, 10);
        break;

    case iregCTRL:
        lpregs->FloatSave.ControlWord = *( (LPWORD) lpvRegValue );
        break;

    case iregSTAT:
        lpregs->FloatSave.StatusWord = *( (LPWORD) lpvRegValue );
        break;

    case iregTAG:
        lpregs->FloatSave.TagWord = *( (LPWORD) lpvRegValue );
        break;

    case iregFPIP:
        lpregs->FloatSave.ErrorOffset = *( (LPWORD) lpvRegValue );
        break;

    case CV_REG_FPEIP:
        lpregs->FloatSave.ErrorOffset = *( (LPLONG) lpvRegValue );
        break;

    case iregFPCS:
        lpregs->FloatSave.ErrorSelector = *( (LPWORD) lpvRegValue );
        break;

    case iregFPDO:
        lpregs->FloatSave.DataOffset = *( (LPWORD) lpvRegValue );
        break;

    case CV_REG_FPEDO:
        lpregs->FloatSave.DataOffset = *( (LPWORD) lpvRegValue );
        break;

    case iregFPDS:
        lpregs->FloatSave.DataSelector = *( (LPWORD) lpvRegValue );
        break;

#define lpsr ((PKSPECIAL_REGISTERS)lpregs)
    case CV_REG_GDTR:
        lpsr->Gdtr.Base = *( (LPDWORD) lpvRegValue );
        break;

    case CV_REG_GDTL:
        lpsr->Gdtr.Limit = *( (LPWORD) lpvRegValue );
        break;

    case CV_REG_IDTR:
        lpsr->Idtr.Base = *( (LPDWORD) lpvRegValue );
        break;

    case CV_REG_IDTL:
        lpsr->Idtr.Limit = *( (LPWORD) lpvRegValue );
        break;

    case CV_REG_LDTR:
        lpsr->Ldtr = *( (LPWORD) lpvRegValue );
        break;

    case CV_REG_TR:
        lpsr->Tr = *( (LPWORD) lpvRegValue );
        break;

    case CV_REG_CR0:
        lpsr->Cr0 = *( (LPDWORD) lpvRegValue );
        break;

    case CV_REG_CR2:
        lpsr->Cr2 = *( (LPDWORD) lpvRegValue );
        break;

    case CV_REG_CR3:
        lpsr->Cr3 = *( (LPDWORD) lpvRegValue );
        break;

    case CV_REG_CR4:
        lpsr->Cr4 = *( (LPDWORD) lpvRegValue );
        break;
#undef lpsr

    case CV_REG_DR0:
        lpregs->Dr0 = *( (PULONG) lpvRegValue );
        break;

    case CV_REG_DR1:
        lpregs->Dr1 = *( (PULONG) lpvRegValue );
        break;

    case CV_REG_DR2:
        lpregs->Dr2 = *( (PULONG) lpvRegValue );
        break;

    case CV_REG_DR3:
        lpregs->Dr3 = *( (PULONG) lpvRegValue );
        break;

    case CV_REG_DR6:
        lpregs->Dr6 = *( (PULONG) lpvRegValue );
        break;

    case CV_REG_DR7:
        lpregs->Dr7 = *( (PULONG) lpvRegValue );

    }


    switch ( ireg ) {

    case iregAL:
    case iregCL:
    case iregDL:
    case iregBL:
    case iregAH:
    case iregCH:
    case iregDH:
    case iregBH:

        lpvRegValue = (LPBYTE) lpvRegValue + sizeof ( BYTE );
        break;

    case iregAX:
    case iregCX:
    case iregDX:
    case iregBX:
    case iregSP:
    case iregBP:
    case iregSI:
    case iregDI:
    case iregIP:
    case iregFLAGS:
    case iregES:
    case iregCS:
    case iregSS:
    case iregDS:
    case iregFS:
    case iregGS:
    case iregCTRL:
    case iregSTAT:
    case iregTAG:
    case iregFPIP:
    case iregFPCS:
    case iregFPDO:
    case iregFPDS:
    case CV_REG_GDTL:
    case CV_REG_IDTL:
    case CV_REG_LDTR:
    case CV_REG_TR:

        lpvRegValue = (LPBYTE) lpvRegValue + sizeof ( WORD );
        break;

    case iregEAX:
    case iregECX:
    case iregEDX:
    case iregEBX:
    case iregESP:
    case iregEBP:
    case iregESI:
    case iregEDI:
    case iregEIP:
    case iregEFLAGS:
    case CV_REG_FPEIP:
    case CV_REG_FPEDO:
    case CV_REG_CR0:
    case CV_REG_CR1:
    case CV_REG_CR2:
    case CV_REG_CR3:
    case CV_REG_CR4:
    case CV_REG_DR0:
    case CV_REG_DR1:
    case CV_REG_DR2:
    case CV_REG_DR3:
    case CV_REG_DR4:
    case CV_REG_DR5:
    case CV_REG_DR6:
    case CV_REG_DR7:
    case CV_REG_GDTR:
    case CV_REG_IDTR:

        lpvRegValue = (LPBYTE) lpvRegValue + sizeof ( LONG );
        break;

    case iregST0:
    case iregST1:
    case iregST2:
    case iregST3:
    case iregST4:
    case iregST5:
    case iregST6:
    case iregST7:

        lpvRegValue = (LPBYTE) lpvRegValue + sizeof ( L_DOUBLE );
        break;

    default:

        lpvRegValue = NULL;
        break;
    }

    return lpvRegValue;
}                               /* DoSetReg() */



LPVOID
X86DoSetFrameReg(
    HPID hpid,
    HTID htid,
    LPTHD lpthd,
    PKNONVOLATILE_CONTEXT_POINTERS contextPtrs,
    DWORD ireg,
    LPVOID lpvRegValue
    )
{
    return X86DoSetReg( (LPCONTEXT) lpthd->regs, ireg, lpvRegValue);
}


XOSD
X86DoGetFrame(
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
    ADDRESS_MODE  mode;
    LPPRC lpprc;
    BYTE  buf[sizeof(SSS) + sizeof(IOCTLGENERIC) + sizeof(KDHELP64)];
    LPSSS lpsss;
    PIOCTLGENERIC pig;
    PKDHELP64 pkd;

    if (!hprc) {
        return xosdBadProcess;
    }

    lpprc = (LPPRC)LLLock(hprc);

    if (wValue == (DWORD_PTR)-1) {
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
            lpthd->drt = (DRT) (lpthd->drt & ( ~(drtCntrlDirty | drtAllDirty) ) );
        }
        UpdateRegisters( hprc, hthd );

        ZeroMemory( &lpthd->StackFrame, sizeof(STACKFRAME64) );
        memcpy (lpthd->frameRegs, lpthd->regs, sizeof(CONTEXT) );
        lpthd->frameNumber = 0;

        //
        // set the addressing mode
        //
        if (lpthd->fFlat || lpprc->dmi.fAlwaysFlat ) {
            mode = AddrModeFlat;
        } else
        if (lpthd->fReal) {
            mode = AddrModeReal;
        } else
        if (lpthd->fOff32) {
            mode = AddrMode1632;
        } else {
            mode = AddrMode1616;
        }

        //
        // setup the program counter
        //
        lpthd->StackFrame.AddrPC.Offset       = (ULONG64)(LONG64)(LONG)((PCONTEXT) (lpthd->regs))->Eip;
        lpthd->StackFrame.AddrPC.Segment      = (WORD)((PCONTEXT) (lpthd->regs))->SegCs;
        lpthd->StackFrame.AddrPC.Mode         = mode;

        //
        // setup the frame pointer
        //
        lpthd->StackFrame.AddrFrame.Offset    = (ULONG64)(LONG64)(LONG)((PCONTEXT) (lpthd->regs))->Ebp;
        lpthd->StackFrame.AddrFrame.Segment   = (WORD)((PCONTEXT) (lpthd->regs))->SegSs;
        lpthd->StackFrame.AddrFrame.Mode         = mode;

        //
        // setup the stack pointer
        //
        lpthd->StackFrame.AddrStack.Offset    = (ULONG64)(LONG64)(LONG)((PCONTEXT) (lpthd->regs))->Esp;
        lpthd->StackFrame.AddrStack.Segment   = (WORD)((PCONTEXT) (lpthd->regs))->SegSs;
        lpthd->StackFrame.AddrStack.Mode         = mode;

        //
        // Get KDHELP if available
        //

        lpsss = (LPSSS) buf;
        pig = (PIOCTLGENERIC)lpsss->rgbData;
        pkd = (PKDHELP64)pig->data;

        i = sizeof(buf);

        lpsss->ssvc = (SSVC)ssvcGeneric;
        lpsss->cbSend = i;
        lpsss->cbReturned = 0;

        pig->ioctlSubType = IG_KSTACK_HELP;
        pig->length = sizeof(KDHELP64);

        xosd = SystemService(hpid,
                             htid,
                             i,
                             lpsss
                             );

        if (xosd == xosdNone) {
            memcpy(&lpthd->StackFrame.KdHelp, pkd, sizeof(KDHELP64));
        }
    }

    fGoodFrame = FALSE;
    xosd = xosdNone;
    for (i = 0; xosd == xosdNone && ((wValue != 0)? (i < wValue) : 1); i++) {

        if (StackWalk64( IMAGE_FILE_MACHINE_I386,
                         hpid,
                         htid,
                         &lpthd->StackFrame,
                         lpthd->frameRegs,
                         SwReadMemory,
                         X86SwFunctionTableAccess,
                         SwGetModuleBase,
                         SwTranslateAddress
                         ))
        {
            xosd = xosdNone;
            lpthd->frameNumber++;
            fGoodFrame = TRUE;
        } else {
            xosd = xosdEndOfStack;
        }
    }

    if (fGoodFrame) {
        *(LPHTID)lValue = fReturnHtid? htid : vhtid;
    }

    LLUnlock( hthd );
    LLUnlock( hprc );

    return xosd;
}



PFPO_DATA
SwSearchFpoData(
    DWORD     key,
    PFPO_DATA base,
    DWORD     num
    )
{
    PFPO_DATA  lo = base;
    PFPO_DATA  hi = base + (num - 1);
    PFPO_DATA  mid;
    DWORD      half;

    while (lo <= hi) {
        if (half = num / 2) {
            mid = lo + (num & 1 ? half : (half - 1));
            if ((key >= mid->ulOffStart)&&
                       (key < (mid->ulOffStart+mid->cbProcSize))) {
                return mid;
            }
            if (key < mid->ulOffStart) {
                hi = mid - 1;
                num = num & 1 ? half : half-1;
            }
            else {
                lo = mid + 1;
                num = half;
            }
        }
        else if (num) {
            if ((key >= lo->ulOffStart)&&
                                     (key < (lo->ulOffStart+lo->cbProcSize))) {
                return lo;
            }
            else {
                break;
            }
        }
        else {
            break;
        }
    }
    return(NULL);
}

LPVOID
X86SwFunctionTableAccess(
    LPVOID  lpvhpid,
    DWORD64 AddrBase
    )
{
    HLLI        hlli  = 0;
    HMDI        hmdi  = 0;
    LPMDI       lpmdi = 0;
    DWORD64     off;
    HPID        hpid = (HPID)lpvhpid;
    PFPO_DATA   pFpo = NULL;

    assert( Is64PtrSE(AddrBase) );

    
    hmdi = SwGetMdi( hpid, AddrBase );

    if (hmdi) {

        lpmdi = (LPMDI) LLLock( hmdi );

        if (lpmdi) {
            
            assert( Is64PtrSE(lpmdi->lpBaseOfDll) );

            VerifyDebugDataLoaded(hpid, NULL, lpmdi);    // M00BUG
            
            assert( Is64PtrSE(lpmdi->lpBaseOfDll) );

            if (lpmdi->lpDebug) {

                off = SEPtrTo64( ConvertOmapToSrc( lpmdi, AddrBase ) );

                if (off) {
                    AddrBase = off;
                }

                pFpo = SwSearchFpoData( (DWORD)(AddrBase-lpmdi->lpBaseOfDll),
                                        (PFPO_DATA) lpmdi->lpDebug->lpRtf,
                                        lpmdi->lpDebug->cRtf
                                      );
            }
            LLUnlock( hmdi );
        }
    }

    return (LPVOID)pFpo;
}

XOSD
X86DoGetFunctionInfo(
    HPID hpid,
    LPGFI lpgfi
    )
{
    PFPO_DATA pFpo;

    assert(!ADDR_IS_LI(*lpgfi->lpaddr));
    assert(ADDR_IS_FLAT(*lpgfi->lpaddr));

    pFpo = (PFPO_DATA)X86SwFunctionTableAccess((LPVOID)hpid, GetAddrOff(*(lpgfi->lpaddr)));

    if (pFpo) {
        AddrInit(&lpgfi->lpFunctionInformation->AddrStart,
                 0, 
                 (SEGMENT) 0, 
                 SE32To64( pFpo->ulOffStart ),
                 TRUE, 
                 TRUE, 
                 FALSE, 
                 FALSE
                 );
        AddrInit(&lpgfi->lpFunctionInformation->AddrEnd,
                 0, 
                 (SEGMENT) 0, 
                 SE32To64( pFpo->ulOffStart + pFpo->cbProcSize ),
                 TRUE, 
                 TRUE, 
                 FALSE, 
                 FALSE
                 );
        AddrInit(&lpgfi->lpFunctionInformation->AddrPrologEnd,
                 0, 
                 (SEGMENT) 0, 
                 SE32To64( pFpo->ulOffStart + pFpo->cbProlog ),
                 TRUE, 
                 TRUE, 
                 FALSE, 
                 FALSE
                 );
        return xosdNone;
    } else {
        return xosdUnknown;
    }
}


DWORD64
SwTranslateAddress(
    LPVOID      lpvhpid,
    LPVOID      lpvhtid,
    LPADDRESS64 lpaddress
    )
{
    XOSD               xosd;

    const DWORD        dwPigLen = sizeof(ADDR) + 4;
    const DWORD        dwTotalLen = sizeof(SSS) + sizeof(IOCTLGENERIC) + dwPigLen;

    ADDR               addr;
    BYTE               buf[dwTotalLen];
    HPID               hpid = (HPID)lpvhpid;
    HTID               htid = (HTID)lpvhtid;
    LPSSS              lpsss = (LPSSS)buf;
    PIOCTLGENERIC      pig   = (PIOCTLGENERIC)lpsss->rgbData;


    ZeroMemory( &addr, sizeof(addr) );
    addr.addr.off     = lpaddress->Offset;
    addr.addr.seg     = lpaddress->Segment;
    addr.mode.fFlat   = lpaddress->Mode == AddrModeFlat;
    addr.mode.fOff32  = lpaddress->Mode == AddrMode1632;
    addr.mode.fReal   = lpaddress->Mode == AddrModeReal;

    memcpy( pig->data, &addr, sizeof(addr) );

    lpsss->ssvc           = (SSVC)ssvcGeneric;
    lpsss->cbSend         = sizeof(IOCTLGENERIC) + dwPigLen;
    pig->length           = dwPigLen;
    pig->ioctlSubType     = IG_TRANSLATE_ADDRESS;
    xosd = SystemService(hpid, htid, dwTotalLen, lpsss);

    if (xosd == xosdNone) {
        addr = *((LPADDR)pig->data);

        lpaddress->Offset   = addr.addr.off;
        lpaddress->Segment  = (WORD)addr.addr.seg;

        if (addr.mode.fFlat) {
            lpaddress->Mode = AddrModeFlat;
        } else
        if (addr.mode.fOff32) {
            lpaddress->Mode = AddrMode1632;
        } else
        if (addr.mode.fReal) {
            lpaddress->Mode = AddrModeReal;
        }

        return TRUE;
    }

    return FALSE;
}


XOSD
X86DoGetFrameEH(
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

    xosd status

--*/
{
    return(xosdUnsupported);
}




//////////////////////////////////////////////////////////////////
//  part 2.  Additional target dependent
//////////////////////////////////////////////////////////////////

XOSD
X86UpdateChild (
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

    else if (pst.dwProcessState != pstRunning)

    {
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

                    ((PKSPECIAL_REGISTERS)(lpthd->pvSpecial))->KernelDr0 =
                                                 ((PCONTEXT) (lpthd->regs))->Dr0;
                    ((PKSPECIAL_REGISTERS)(lpthd->pvSpecial))->KernelDr1 =
                                                 ((PCONTEXT)  (lpthd->regs))->Dr1;
                    ((PKSPECIAL_REGISTERS)(lpthd->pvSpecial))->KernelDr2 =
                                                 ((PCONTEXT) (lpthd->regs))->Dr2;
                    ((PKSPECIAL_REGISTERS)(lpthd->pvSpecial))->KernelDr3 =
                                                 ((PCONTEXT) (lpthd->regs))->Dr3;
                    ((PKSPECIAL_REGISTERS)(lpthd->pvSpecial))->KernelDr6 =
                                                 ((PCONTEXT) (lpthd->regs))->Dr6;
                    ((PKSPECIAL_REGISTERS)(lpthd->pvSpecial))->KernelDr7 =
                                                 ((PCONTEXT) (lpthd->regs))->Dr7;

                    SendRequestX(dmfWriteRegEx,
                                 hpid,
                                 lpthd->htid,
                                 lpthd->dwcbSpecial,
                                 lpthd->pvSpecial
                                );
                } else {

                    DWORD DR[6];
                    DR[0] = ((PCONTEXT) (lpthd->regs) )->Dr0;
                    DR[1] = ((PCONTEXT) (lpthd->regs) )->Dr1;
                    DR[2] = ((PCONTEXT) (lpthd->regs) )->Dr2;
                    DR[3] = ((PCONTEXT) (lpthd->regs) )->Dr3;
                    DR[4] = ((PCONTEXT) (lpthd->regs) )->Dr6;
                    DR[5] = ((PCONTEXT) (lpthd->regs) )->Dr7;
                    SendRequestX (dmfWriteRegEx,
                                  hpid,
                                  lpthd->htid,
                                  sizeof ( DR ),
                                  DR
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

            xosd = SendRequestX(dmfNonLocalGoto,
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
X86GetRegValue (
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
        X86UpdateSpecialRegisters( lpthd->hprc, hthd );

        if (lpprc->dmi.fAlwaysFlat || ( ((LPCONTEXT) (lpthd->regs))->SegCs == lpprc->selFlatCs) ) {
            lpthd->fFlat = lpthd->fOff32 = TRUE;
            lpthd->fReal = FALSE;
        } else {
            /*
             *  BUGBUG -- jimsch -- some one might eventually catch on
             *  that this is incorrect.  We are not checking to see if the
             *  current address is really a 16-bit WOW address but assuming
             *  that it is.  This will be a problem for people who are doing
             *  real 16:32 programming (on WOW) and people who are doing
             *  real mode program -- but so what
             */
            lpthd->fFlat = lpthd->fOff32 = lpthd->fReal = FALSE;
        }
    } else {

        if ( !(lpthd->drt & drtAllPresent) ) {

            switch ( ireg ) {


            case CV_REG_CS:
            case CV_REG_IP:
            case CV_REG_SS:
            case CV_REG_BP:

            case CV_REG_EIP:
            case CV_REG_EBP:
                if (!(lpthd->drt & drtCntrlPresent)) {
                    UpdateRegisters( lpthd->hprc, hthd );
                }
                break;

            default:

                UpdateRegisters ( lpthd->hprc, hthd );
                break;
            }
        }


        if ( !(lpthd->drt & drtSpecialPresent) ) {

            switch ( ireg ) {

            case CV_REG_GDTR:
            case CV_REG_GDTL:
            case CV_REG_IDTR:
            case CV_REG_IDTL:
            case CV_REG_LDTR:
            case CV_REG_TR:

            case CV_REG_CR0:
            case CV_REG_CR1:
            case CV_REG_CR2:
            case CV_REG_CR3:
            case CV_REG_CR4:

            case CV_REG_DR0:
            case CV_REG_DR1:
            case CV_REG_DR2:
            case CV_REG_DR3:
            case CV_REG_DR4:
            case CV_REG_DR5:
            case CV_REG_DR6:
            case CV_REG_DR7:

                X86UpdateSpecialRegisters( lpthd->hprc, hthd );
                break;

            default:
                break;
            }
        }
    }

    switch ( ireg ) {

        case CV_REG_GDTR:
        case CV_REG_GDTL:
        case CV_REG_IDTR:
        case CV_REG_IDTL:
        case CV_REG_LDTR:
        case CV_REG_TR:

        case CV_REG_CR0:
        case CV_REG_CR1:
        case CV_REG_CR2:
        case CV_REG_CR3:
        case CV_REG_CR4:

            lpregs = (LPCONTEXT) lpthd->pvSpecial;
            break;

        default:
            break;
    }


    LLUnlock( hprc );
    LLUnlock( hthd );

    lpvRegValue = X86DoGetReg ( lpregs, ireg & 0xff, lpvRegValue );

    if ( lpvRegValue == NULL ) {
        LLUnlock ( hthd );
        return xosdInvalidParameter;
    }
    ireg = ireg >> 8;

    if ( ireg != CV_REG_NONE ) {
        lpvRegValue = X86DoGetReg ( lpregs, ireg, lpvRegValue );
        if ( lpvRegValue == NULL ) {
            return xosdInvalidParameter;
        }
    }

    return xosdNone;

}                        /* GetRegValue */





XOSD
X86SetRegValue (
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

    switch ( ireg ) {
        case CV_REG_GDTR:
        case CV_REG_GDTL:
        case CV_REG_IDTR:
        case CV_REG_IDTL:
        case CV_REG_LDTR:
        case CV_REG_TR:

        case CV_REG_CR0:
        case CV_REG_CR1:
        case CV_REG_CR2:
        case CV_REG_CR3:
        case CV_REG_CR4:

            lpregs = lpthd->pvSpecial;
            // fall thru

        case CV_REG_DR0:
        case CV_REG_DR1:
        case CV_REG_DR2:
        case CV_REG_DR3:
        case CV_REG_DR4:
        case CV_REG_DR5:
        case CV_REG_DR6:
        case CV_REG_DR7:

            if (!(lpthd->drt & drtSpecialPresent)) {
                X86UpdateSpecialRegisters( lpthd->hprc, hthd );
            }
            break;

        default:

            lpregs = lpthd->regs;

            if ( !(lpthd->drt & drtAllPresent) ) {
                UpdateRegisters ( lpthd->hprc, hthd );
            }
            break;
    }

    lpvRegValue = X86DoSetReg ( (LPCONTEXT) lpregs, ireg & 0xff, lpvRegValue );

    if ( lpvRegValue == NULL ) {
        LLUnlock ( hthd );
        return xosdInvalidParameter;
    }

    ireg = ireg >> 8;
    if ( ireg != 0 ) {
        lpvRegValue = X86DoSetReg ( (LPCONTEXT) lpregs, ireg, lpvRegValue );
    }
    if ( lpvRegValue == NULL ) {
        LLUnlock ( hthd );
        return xosdInvalidParameter;
    }


    switch ( ireg ) {
        case CV_REG_GDTR:
        case CV_REG_GDTL:
        case CV_REG_IDTR:
        case CV_REG_IDTL:
        case CV_REG_LDTR:
        case CV_REG_TR:

        case CV_REG_CR0:
        case CV_REG_CR1:
        case CV_REG_CR2:
        case CV_REG_CR3:
        case CV_REG_CR4:

        case CV_REG_DR0:
        case CV_REG_DR1:
        case CV_REG_DR2:
        case CV_REG_DR3:
        case CV_REG_DR4:
        case CV_REG_DR5:
        case CV_REG_DR6:
        case CV_REG_DR7:

            lpthd->drt = (DRT) (lpthd->drt | drtAllDirty);
            break;

        default:

            lpthd->drt = (DRT) (lpthd->drt | drtAllDirty);
            break;
    }



    LLUnlock ( hthd );

    return xosd;

}


void
X86UpdateSpecialRegisters (
    HPRC hprc,
    HTHD hthd
    )
{

    LPTHD lpthd = (LPTHD) LLLock ( hthd );

    SendRequest ( dmfReadRegEx, HpidFromHprc ( hprc ), HtidFromHthd ( hthd ) );

    if (lpthd->dwcbSpecial) {
        //
        // in kernel mode...
        //
        _fmemcpy ( lpthd->pvSpecial, LpDmMsg->rgb, lpthd->dwcbSpecial );
        ((PCONTEXT) (lpthd->regs))->Dr0 = ((PKSPECIAL_REGISTERS)(LpDmMsg->rgb))->KernelDr0;
        ((PCONTEXT) (lpthd->regs))->Dr1 = ((PKSPECIAL_REGISTERS)(LpDmMsg->rgb))->KernelDr1;
        ((PCONTEXT) (lpthd->regs))->Dr2 = ((PKSPECIAL_REGISTERS)(LpDmMsg->rgb))->KernelDr2;
        ((PCONTEXT) (lpthd->regs))->Dr3 = ((PKSPECIAL_REGISTERS)(LpDmMsg->rgb))->KernelDr3;
        ((PCONTEXT) (lpthd->regs))->Dr6 = ((PKSPECIAL_REGISTERS)(LpDmMsg->rgb))->KernelDr6;
        ((PCONTEXT) (lpthd->regs))->Dr7 = ((PKSPECIAL_REGISTERS)(LpDmMsg->rgb))->KernelDr7;
    } else {
        //
        // User mode
        //
        ((PCONTEXT) (lpthd->regs))->Dr0 = ((LPDWORD)(LpDmMsg->rgb))[0];
        ((PCONTEXT) (lpthd->regs))->Dr1 = ((LPDWORD)(LpDmMsg->rgb))[1];
        ((PCONTEXT) (lpthd->regs))->Dr2 = ((LPDWORD)(LpDmMsg->rgb))[2];
        ((PCONTEXT) (lpthd->regs))->Dr3 = ((LPDWORD)(LpDmMsg->rgb))[3];
        ((PCONTEXT) (lpthd->regs))->Dr6 = ((LPDWORD)(LpDmMsg->rgb))[4];
        ((PCONTEXT) (lpthd->regs))->Dr7 = ((LPDWORD)(LpDmMsg->rgb))[5];
    }

    lpthd->drt = (DRT) (lpthd->drt & (~drtSpecialDirty) );
    lpthd->drt = (DRT) (lpthd->drt | drtSpecialPresent);

    LLUnlock ( hthd );

}




void
X86CopyFrameRegs(
    LPTHD lpthd,
    LPBPR lpbpr
    )
{
    ((PCONTEXT) (lpthd->regs))->SegCs   = lpbpr->segCS;
    ((PCONTEXT) (lpthd->regs))->SegSs   = lpbpr->segSS;
    ((PCONTEXT) (lpthd->regs))->Eip     = (DWORD)lpbpr->offEIP;
    ((PCONTEXT) (lpthd->regs))->Ebp     = (DWORD)lpbpr->offEBP;
    ZeroMemory(&lpthd->StackFrame, sizeof(lpthd->StackFrame));

}


void
X86AdjustForProlog(
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
