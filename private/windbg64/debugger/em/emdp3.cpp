/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    emdp3.c

Abstract:

    This file contains the some of the machine independent portions of the
    execution model.  The machine dependent portions are in other files.

Author:

    Kent Forschmiedt (kentf) 11-8-93

Environment:

    Win32 -- User

Notes:

    The orginal source for this came from the CodeView group.

--*/

#include "emdp.h"

#include "fbrdbg.h"


#define WINDBG_POINTERS_MACROS_ONLY
#include "sundown.h"
#undef WINDBG_POINTERS_MACROS_ONLY


//
// This list is only used when there is no DM present.  Whenever an
// hpid is created, the real list is obtained from the DM and stored
// in a list bound to the hprc.
//
//  M00BUG -- this list does not match the one in the DM. // JLS
//
static EXCEPTION_DESCRIPTION DefaultExceptionList[] = {
    {EXCEPTION_ACCESS_VIOLATION,        efdStop,  exfSpecified, _T("Access Violation")},
    {EXCEPTION_ARRAY_BOUNDS_EXCEEDED,   efdStop,  exfSpecified, _T("Array Bounds Exceeded")},
    {EXCEPTION_FLT_DENORMAL_OPERAND,    efdStop,  exfSpecified, _T("FP Denormal Operand")},
    {EXCEPTION_FLT_DIVIDE_BY_ZERO,      efdStop,  exfSpecified, _T("FP Divide by Zero")},
    {EXCEPTION_FLT_INEXACT_RESULT,      efdStop,  exfSpecified, _T("FP Inexact Result")},
    {EXCEPTION_FLT_INVALID_OPERATION,   efdStop,  exfSpecified, _T("FP Invalid Operation")},
    {EXCEPTION_FLT_OVERFLOW,            efdStop,  exfSpecified, _T("FP Overflow")},
    {EXCEPTION_FLT_STACK_CHECK,         efdStop,  exfSpecified, _T("FP Stack Check")},
    {EXCEPTION_FLT_UNDERFLOW,           efdStop,  exfSpecified, _T("FP Underflow")},
    {EXCEPTION_INT_DIVIDE_BY_ZERO,      efdStop,  exfSpecified, _T("Int Divide by zero")},
    {EXCEPTION_INT_OVERFLOW,            efdStop,  exfSpecified, _T("Int Overflow")},
    {EXCEPTION_PRIV_INSTRUCTION,        efdStop,  exfSpecified, _T("Insufficient Privilege")},
    {EXCEPTION_IN_PAGE_ERROR,           efdStop,  exfSpecified, _T("I/O Error in Paging")},
    {EXCEPTION_ILLEGAL_INSTRUCTION,     efdStop,  exfSpecified, _T("Illegal Instruction")},
    {EXCEPTION_NONCONTINUABLE_EXCEPTION,efdStop,  exfSpecified, _T("Noncontinuable Exception")},
    {EXCEPTION_STACK_OVERFLOW,          efdStop,  exfSpecified, _T("Stack Overflow")},
    {EXCEPTION_INVALID_DISPOSITION,     efdStop,  exfSpecified, _T("Invalid Disposition")},
    {DBG_CONTROL_C,                     efdStop,  exfSpecified, _T("Control-C break")},
    {0,                                 efdStop,  exfSpecified, _T("") },
};

EXCEPTION_FILTER_DEFAULT EfdDefault = efdNotify;


#define DECL_MASK(n,v,s) n = v,
#define DECL_MSG(n,s,m)

enum {
#include "win32msg.h"
};

#undef DECL_MASK
#define DECL_MASK(n,v,s) { n, _T(s) },

MASKINFO MaskInfo[] = {
#include "win32msg.h"
};

#define MASKMAPSIZE (sizeof(MaskInfo)/sizeof(MASKINFO))
MASKMAP MaskMap = {MASKMAPSIZE, MaskInfo};

#undef DECL_MASK
#undef DECL_MSG


#define DECL_MASK(n,v,s)
#define DECL_MSG(n,s,m) { n, _T(s), m },

MESSAGEINFO MessageInfo[] = {
#include "win32msg.h"
};

#define MESSAGEMAPSIZE (sizeof(MessageInfo)/sizeof(MESSAGEINFO))
MESSAGEMAP MessageMap = {MESSAGEMAPSIZE,MessageInfo};

#undef DECL_MASK
#undef DECL_MSG




XOSD
HandleBreakpoints(
    HPID hpid,
    DWORD wValue,
    UINT_PTR lValue
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    LPBPS lpbps = (LPBPS) lValue;
    LPDBB lpdbb = (LPDBB) MHAlloc(FIELD_OFFSET(DBB, rgbVar) + wValue);

    ZeroMemory(lpdbb, FIELD_OFFSET(DBB, rgbVar) + wValue);

    // let the DM handle everything?
    lpdbb->hpid = hpid;
    lpdbb->htid = NULL;
    lpdbb->dmf  = dmfBreakpoint;
    memcpy(lpdbb->rgbVar, lpbps, wValue);
    CallTL ( tlfRequest, hpid, FIELD_OFFSET ( DBB, rgbVar ) + wValue, (DWORD64)lpdbb );
    if (LpDmMsg->xosdRet == xosdNone) {
        memcpy(lpbps, LpDmMsg->rgb, SizeofBPS(lpbps));
    }
    MHFree(lpdbb);
    return LpDmMsg->xosdRet;
}


XOSD
Go (
    HPID hpid,
    HTID htid,
    LPEXOP lpexop
    )
{
    UpdateChild( hpid, htid, dmfGo );
    return SendRequestX(dmfGo, hpid, htid, sizeof(EXOP), lpexop);
}


XOSD
ReturnStep (
    HPID hpid,
    HTID htid,
    LPEXOP lpexop
    )
{
    RTRNSTP rtrnstp;
    XOSD xosd = xosdNone;
    HTID vhtid = htid;

    rtrnstp.exop = *lpexop;
    if (((HandleToLong(htid)) & 1) == 0) {
        xosd = GetFrame( hpid, vhtid, 1, (DWORD_PTR)&vhtid );
    }
    if ( xosd == xosdNone ) {
       xosd = GetFrame( hpid, vhtid, 1, (DWORD_PTR)&vhtid );
       if ( xosd == xosdNone ) {
          xosd = GetAddr( hpid, vhtid, adrPC, &(rtrnstp.addrRA) );
          if ( xosd == xosdNone ) {
              xosd = GetAddr( hpid, htid, adrStack, &(rtrnstp.addrStack) );
          }
       }
    }
    if ( xosd != xosdNone ) {
       return( xosd );
    }
    return SendRequestX ( dmfReturnStep, hpid, htid, sizeof(rtrnstp), &rtrnstp);

}

XOSD
ThreadStatus (
    HPID hpid,
    HTID htid,
    LPTST lptst
    )
{
    XOSD xosd = SendRequest ( dmfThreadStatus, hpid, htid );
    if (xosd == xosdNone) {
        xosd = LpDmMsg->xosdRet;
    }
    if (xosd == xosdNone) {
        memcpy(lptst, LpDmMsg->rgb, sizeof(TST));
    } else {
        HPRC hprc = ValidHprcFromHpid(hpid);
        if (hprc) {
            HTHD hthd = HthdFromHtid(hprc, htid);
            if (hthd) {
                LPTHD lpthd = (LPTHD) LLLock(hthd);
                lptst->dwThreadID = lpthd->tid;
                LLUnlock(hthd);
            }
        }
    }
    return xosd;
}


XOSD
ProcessStatus(
    HPID hpid,
    LPPST lppst
    )
{
    XOSD xosd;
    xosd = SendRequest(dmfProcessStatus, hpid, NULL );
    if (xosd == xosdNone) {
        xosd = LpDmMsg->xosdRet;
    }
    if (xosd == xosdNone) {
        memcpy(lppst, LpDmMsg->rgb, sizeof(PST));
    }
    return xosd;
}


XOSD
GetTimeStamp(
    HPID    hpid,
    HTID    htid,
    LPTCS   lptcs
    )
{
    XOSD    xosd;
    ULONG   len;
    LPTCSR  lptcsr = (LPTCSR) LpDmMsg->rgb;

    len = _tcslen (lptcs->ImageName) + 1;

    xosd = SendRequestX (dmfGetTimeStamp, hpid, htid, len, lptcs->ImageName);

    if (xosd == xosdNone) {
        xosd = LpDmMsg->xosdRet;
    }

    if (xosd == xosdNone) {
        lptcs->TimeStamp = lptcsr->TimeStamp;
        lptcs->CheckSum = lptcsr->CheckSum;
    }

    return xosd;
}



XOSD
Freeze (
    HPID hpid,
    HTID htid
    )
{
    HTHD hthd;
    HPRC hprc = ValidHprcFromHpid(hpid);
    if (!hprc) {
        return xosdBadProcess;
    }
    if ( (hthd = HthdFromHtid(hprc, htid)) == hthdInvalid || hthd == NULL ) {
        return xosdBadThread;
    }

    SendRequest ( dmfFreeze, hpid, htid);

    return LpDmMsg->xosdRet;
}


XOSD
Thaw (
    HPID hpid,
    HTID htid
    )
{
    HTHD hthd;
    HPRC hprc = ValidHprcFromHpid(hpid);
    if (!hprc) {
        return xosdBadProcess;
    }
    if ( (hthd = HthdFromHtid(hprc, htid)) == hthdInvalid || hthd == NULL ) {
        return xosdBadThread;
    }

    SendRequest ( dmfResume, hpid, htid);

    return LpDmMsg->xosdRet;
}


XOSD
DebugMetric (
    HPID hpid,
    HTID htid,
    MTRC mtrc,
    LPLONG lpl
    )
/*++

Routine Description:

    The debugger queries this function to find out the size of OS and machine
    dependent values, e.g. the size of a process ID.

Arguments:

    hpid

    htid

    mtrc   - metric identifier

    lpl    - answer buffer

Return Value:

    xosdNone if the request succeeded, xosd error code otherwise.

--*/
{
    HPRC hprc;
    HTHD hthd;
    LPPRC lpprc = NULL;
    XOSD xosd = xosdNone;


    hprc = HprcFromHpid(hpid);

    if (hprc) {

        lpprc = (LPPRC) LLLock( hprc );
        assert( lpprc );

        switch ( mtrc ) {

          default:
            break;

          case mtrcProcessorType:
          case mtrcProcessorLevel:
          case mtrcEndian:
          case mtrcThreads:
          case mtrcAsync:
          case mtrcAsyncStop:
          case mtrcBreakPoints:
          case mtrcReturnStep:
          case mtrcRemote:
          case mtrcOSVersion:
            if (!lpprc->fDmiCache) {
                xosd = SendRequest ( dmfGetDmInfo, hpid, htid );
                if (xosd == xosdNone) {
                    memcpy(&lpprc->dmi, LpDmMsg->rgb, sizeof(DMINFO));
                    lpprc->fDmiCache = lpprc->dmi.fDMInfoCacheable;
                }
            }
            break;

        }

        if (xosd != xosdNone) {
            LLUnlock( hprc );
            return xosd;
        }


    }

    switch ( mtrc ) {

      default:
        assert(FALSE);
        xosd = xosdInvalidParameter;
        break;

      case mtrcProcessorType:

        assert(lpprc);
        *lpl = lpprc->dmi.Processor.Type;
        break;

      case mtrcProcessorLevel:

        assert(lpprc);
        *lpl = lpprc->dmi.Processor.Level;
        break;

      case mtrcEndian:

        assert(lpprc);
        *lpl = lpprc->dmi.Processor.Endian;
        break;

      case mtrcThreads:

        assert(lpprc);
        *lpl = lpprc->dmi.fHasThreads;
        break;

      case mtrcCRegs:

        *lpl = CRgrd(hpid);
        break;

      case mtrcCFlags:

        *lpl = CRgfd(hpid);
        break;

      case mtrcExtRegs:

        assert(0 && "do something with this");
        break;

      case mtrcExtFP:

        assert(0 && "do something with this");
        break;

      case mtrcExtMMU:

        *( (LPDWORD) lpl) = lpprc->dmi.fKernelMode;
        break;

      case mtrcExceptionHandling:

        *( (LPDWORD) lpl) = TRUE;
        break;

      case mtrcAssembler:
#if 0
        switch(MPTFromHprc(hprc)) {
            case mptix86:
            case mptdaxp:
            default:
                *( (LPDWORD) lpl) = FALSE;
                break;
        }
#else
        *( (LPDWORD) lpl) = FALSE;
#endif
        break;

      case mtrcAsync:

        assert(lpprc);
#if defined(DOLPHIN) // HACK!!! Need to get mtrc bits into od.h
        *(LPWORD)lpl = (WORD)lpprc->dmi.mAsync;
#else
        *lpl = !!lpprc->dmi.mAsync;
#endif
        break;

      case mtrcAsyncStop:

        assert(lpprc);
        *lpl = !!(lpprc->dmi.mAsync & asyncStop);
        break;

      case mtrcBreakPoints:

        assert(lpprc);
        //
        // Message BPs are implemented in the EM
        // on top of the exec BP implemented by the DM.
        //
        *lpl = lpprc->dmi.Breakpoints |
                bptsMessage |
                bptsMClass;
        break;

      case mtrcReturnStep:

        assert(lpprc);
        *lpl = lpprc->dmi.fReturnStep;
        break;

      case mtrcShowDebuggee:

        *lpl = FALSE;
        break;

      case mtrcHardSoftMode:

        *lpl = FALSE;
        break;

      case mtrcRemote:

        assert(lpprc);
        *lpl = lpprc->dmi.fRemote;
        break;

      case mtrcOleRpc:

        *lpl = TRUE;
        break;

      case mtrcNativeDebugger:

        *lpl = FALSE;
        break;

      case mtrcOSVersion:

        *lpl = (lpprc->dmi.MajorVersion << 16) | lpprc->dmi.MinorVersion;
        break;

      case mtrcMultInstances:

        *(BOOL*) lpl = TRUE;
        break;

      case mtrcTidValue:
        HTHD hthd = HthdFromHtid(hprc, htid);
        if (hthd) {
            LPTHD lpthd = (LPTHD) LLLock(hthd);
            *lpl = lpthd->tid;
            LLUnlock(hthd);
        } else {
            *lpl = 0;
        }
        break;
    }

    LLUnlock( hprc );

    return xosdNone;
}


XOSD
FakeGetExceptionState(
    LPEXCEPTION_DESCRIPTION lpexd
    )
/*++

Routine Description:

    Handle the GetExceptionState call when there is no DM connected.

Arguments:

    lpexd - Returns EXCEPTION_DESCRIPTION record

Return Value:

    xosdNone except when exc is exfNext and lpexd->dwExceptionCode
    was not in the list.

--*/
{
    DWORD dwT;
    int i;

    if (lpexd->exc == exfFirst) {
        *lpexd = DefaultExceptionList[0];
        return xosdNone;
    }

    if (lpexd->exc == exfDefault) {
        lpexd->dwExceptionCode = 0;
        lpexd->efd = EfdDefault;
        lpexd->rgchDescription[0] = 0;
        return xosdNone;
    }

    for (i = 0; DefaultExceptionList[i].dwExceptionCode != 0; i++) {
        if (DefaultExceptionList[i].dwExceptionCode == lpexd->dwExceptionCode) {
            break;
        }
    }

    if (lpexd->exc == exfSpecified) {
        dwT = lpexd->dwExceptionCode;
        *lpexd = DefaultExceptionList[i];
        lpexd->dwExceptionCode = dwT;
        return xosdNone;
    }

    if (DefaultExceptionList[++i].dwExceptionCode != 0) {
        *lpexd = DefaultExceptionList[i];
        return xosdNone;
    }

    return xosdInvalidParameter;
}


XOSD
GetExceptionState(
    HPID hpid,
    HTID htid,
    LPEXCEPTION_DESCRIPTION lpexd
    )
{
    HPRC hprc;
    LPPRC lpprc;
    XOSD xosd = xosdNone;
    HEXD hexd;

    if (!hpid) {
        return FakeGetExceptionState(lpexd);
    }

    hprc = HprcFromHpid( hpid );
    assert(hprc);
    lpprc = (LPPRC) LLLock(hprc);

    switch (lpexd->exc) {

      default:
        assert( 0 && "Invalid arg to em!GetExceptionState" );
        xosd = xosdInvalidParameter;
        break;

      case exfDefault:
        lpexd->dwExceptionCode = 0;
        lpexd->efd = EfdDefault;
        lpexd->rgchDescription[0] = 0;
        break;

      case exfFirst:

        hexd = LLNext( lpprc->llexc, NULL );
        if (!hexd) {
            memset(lpexd, 0, sizeof(EXCEPTION_DESCRIPTION));
            xosd = xosdInvalidParameter;
        } else {
            *lpexd = *(LPEXCEPTION_DESCRIPTION)LLLock(hexd);
            LLUnlock(hexd);
        }
        break;


      case exfSpecified:

        hexd = LLFind( lpprc->llexc, NULL, &lpexd->dwExceptionCode, 0 );
        if (!hexd) {
            memset(lpexd, 0, sizeof(EXCEPTION_DESCRIPTION));
            xosd = xosdInvalidParameter;
        } else {
           *lpexd = *(LPEXCEPTION_DESCRIPTION)LLLock(hexd);
           LLUnlock(hexd);
        }
        break;


      case exfNext:

        hexd = LLFind( lpprc->llexc, NULL, &lpexd->dwExceptionCode, 0 );
        if (!hexd) {
            //
            // origin must exist
            //
            xosd = xosdInvalidParameter;
        } else {
            //
            // but the next one need not
            //
            hexd = LLNext( lpprc->llexc, hexd );
            if (!hexd) {
                memset(lpexd, 0, sizeof(EXCEPTION_DESCRIPTION));
                xosd = xosdEndOfStack;
            } else {
                *lpexd = *(LPEXCEPTION_DESCRIPTION)LLLock(hexd);
                LLUnlock(hexd);
            }
        }
        break;

    }

    LLUnlock(hprc);
    return xosd;
}


XOSD
SetExceptionState(
    HPID hpid,
    HTID htid,
    LPEXCEPTION_DESCRIPTION lpexd
    )
{
    HPRC hprc = HprcFromHpid( hpid );
    HLLI llexc;
    HEXD hexd;
    LPPRC lpprc;
    XOSD xosd = xosdNone;

    assert(lpexd->efd == efdIgnore ||
           lpexd->efd == efdNotify ||
           lpexd->efd == efdCommand ||
           lpexd->efd == efdStop);

    //
    // exfFirst and exfNext are not valid here
    //

    assert(lpexd->exc == exfSpecified ||
           lpexd->exc == exfDefault);

    if (!hprc) {
        return xosdBadProcess;
    }

    lpprc = (LPPRC)LLLock(hprc);

    if (lpexd->exc == exfDefault) {

        //
        // default action is not stored in the exception action list
        //

        EfdDefault = lpexd->efd;

        // However we need to notify the DM of the change in default behavior.
        xosd = SendRequestX(dmfSetExceptionState,
                            hpid,
                            htid,
                            sizeof(EXCEPTION_DESCRIPTION),
                            lpexd
                            );

    } else {

        llexc = lpprc->llexc;
        hexd = LLFind( llexc, NULL, &lpexd->dwExceptionCode, 0 );

        if (!hexd) {
            hexd = LLCreate( llexc );
            if (!hexd) {
                xosd = xosdOutOfMemory;
            } else {
                LLAdd( llexc, hexd );
            }
        }

        //
        // Don't call the DM if we can't allocate memory - it would
        // cause the em and dm to be out of sync, which we don't need.
        //
        if (hexd) {
            *(LPEXCEPTION_DESCRIPTION)LLLock(hexd) = *lpexd;
            LLUnlock(hexd);
            xosd = SendRequestX(dmfSetExceptionState,
                                hpid,
                                htid,
                                sizeof(EXCEPTION_DESCRIPTION),
                                lpexd
                                );
        }
    }

    LLUnlock(hprc);
    return xosd;
}


XOSD
GetMemoryInfo(
    HPID hpid,
    HTID htid,
    LPMEMINFO lpmi
    )
{
    ADDR addr;
    XOSD xosd = xosdNone;

    Unreferenced(htid);

    addr = lpmi->addr;

    if (ADDR_IS_LI(addr)) {
        xosd = FixupAddr(hpid, htid, &addr);
    }

    if (xosd == xosdNone) {
        xosd = SendRequestX(dmfVirtualQuery, 
                            hpid, 
                            0, 
                            sizeof(addr),
                            (LPVOID)&addr 
                            );
    }

    if (xosd == xosdNone) {
        LPMEMINFO lpmiFromDM = (LPMEMINFO) LpDmMsg->rgb;
        lpmi->addrAllocBase = addr;
        lpmi->addrAllocBase.addr.off = lpmiFromDM->addrAllocBase.addr.off;
        lpmi->uRegionSize = lpmiFromDM->uRegionSize;
        lpmi->dwProtect = lpmiFromDM->dwProtect;
        lpmi->dwState = lpmiFromDM->dwState;
        lpmi->dwType = lpmiFromDM->dwType;
        lpmi->dwAllocationProtect = lpmiFromDM->dwAllocationProtect;
    }

    return xosd;
}


XOSD
FreezeThread(
    HPID hpid,
    HTID htid,
    BOOL fFreeze
    )
{
    HTHD hthd;
    HPRC hprc = ValidHprcFromHpid(hpid);
    if (!hprc) {
        return xosdBadProcess;
    }
    if ( (hthd = HthdFromHtid(hprc, htid)) == hthdInvalid || hthd == NULL ) {
        return xosdBadThread;
    }

    if (fFreeze) {
        SendRequest ( dmfFreeze, hpid, htid);
    } else {
        SendRequest ( dmfResume, hpid, htid);
    }

    return LpDmMsg->xosdRet;
}



#define FreeModuleList(m)                       MHFree(m)
#define ModuleListCount(m)                      ((m)->Count)
#define FirstModuleEntry(m)                     ((LPMODULE_ENTRY)((m)+1))
#define NextModuleEntry(e)                      ((e)+1)
#define NthModuleEntry(m,n)                     (FirstModuleEntry(m)+(n))

#define ModuleEntryFlat(e)                      ((e)->Flat)
#define ModuleEntryReal(e)                      ((e)->Real)
#define ModuleEntrySegment(e)                   ((e)->Segment)
#define ModuleEntrySelector(e)                  ((e)->Selector)
#define ModuleEntryBase(e)                      ((e)->Base)
#define ModuleEntryLimit(e)                     ((e)->Limit)
#define ModuleEntryType(e)                      ((e)->Type)
#define ModuleEntrySectionCount(e)              ((e)->SectionCount)
#define ModuleEntryName(e)                      ((e)->Name)


XOSD
GetModuleNameFromAddress(
    IN  HPID                hpid,
    IN  DWORD64             Address,
    OUT PTSTR               pszModuleName
    )
/*++

Routine Description:

    Get a module name via the PID and address. This is done by using the EMs module
    list, and bypasses SAPI.

Arguments:

    hpid - Supplies process

    Address - Address used to find the module.

    pszModuleName - If successful will contain the module name. MUST be at 
                    least MAX_PATH in length.

Return Value:

    xosdNone if successful.  Other xosd codes indicate the
    cause of failure.

--*/
{

    HMDI hmdi = SwGetMdi(hpid, Address);
    LPMDI       lpmdi = NULL;
    XOSD        xosd = xosdGeneral;

    if (hmdi) {
        lpmdi = (LPMDI)LLLock( hmdi );
        if (lpmdi) {
            //
            // The module name is embedded in the string received from
            // the DM and has the following format: |g:\path\filename.ext|...|...
            //  
            assert('|' == *lpmdi->lszName);

            TCHAR szModPath[MAX_PATH];
            PTSTR pszEnd = strchr(lpmdi->lszName+1, '|');

            if (pszEnd) {
                //
                // First get the path, then get the file name.
                //

                size_t Len = min( MAX_PATH-1, pszEnd - (lpmdi->lszName +1));

                _tcsncpy(szModPath, lpmdi->lszName+1, Len);
                szModPath[Len] = 0;
                
                _tsplitpath(szModPath, 0, 0, pszModuleName, 0);

                xosd = xosdNone;
            }           
            LLUnlock( hmdi );
        }
    }
    
    return xosd;
}


XOSD
GetModuleList(
    HPID                    hpid,
    HTID                    htid,
    LPTSTR                  lpModuleName,
    LPMODULE_LIST FAR *     lplpModuleList
    )
{
    XOSD            xosd = xosdNone;
    HLLI            llmdi;
    HMDI            hmdi;
    LPMDI           lpmdi;
    DWORD           Count;
    LPMODULE_LIST   ModList;
    LPMODULE_LIST   TmpList;
    LPMODULE_ENTRY  Entry;
    LDT_ENTRY       Ldt;
    DWORD           MaxSize;
    DWORD           Delta;
    DWORD           i;
    SEGMENT         Selector;
    DWORD           Base;
    DWORD           Limit;
    OBJD           *ObjD;
    LPTSTR          p;
    TCHAR           WantedName[ MAX_PATH ];
    TCHAR           WantedExt[ MAX_PATH ];
    TCHAR           ModName[ MAX_PATH ];
    TCHAR           ModExt[ MAX_PATH ];
    TCHAR           Name[ MAX_PATH ];

    *WantedName = '\0';
    *WantedExt  = '\0';

    if ( !lplpModuleList ) {
        xosd = xosdInvalidParameter;
        goto Done;
    }

    *lplpModuleList = NULL;

    llmdi = LlmdiFromHprc( HprcFromHpid ( hpid ));

    if ( !llmdi ) {
        xosd = xosdBadProcess;
        goto Done;
    }


    //
    //  Estimate the list size, to minimize the calls to realloc.
    //
    if ( lpModuleName ) {

        Count = 20;
        _tsplitpath( lpModuleName, NULL, NULL, WantedName, WantedExt );

    } else {

        hmdi  = hmdiNull;
        Count = 0;

        while ( (hmdi = LLNext( llmdi, hmdi )) != hmdiNull ) {
            lpmdi = (LPMDI) LLLock( hmdi );
            Count += lpmdi->fFlatMode ? 1 : lpmdi->cobj;
            LLUnlock( hmdi );
        }
    }

    //
    //  Allocate the list
    //
    MaxSize = sizeof(MODULE_LIST) + Count * sizeof(MODULE_ENTRY);

    ModList = (LPMODULE_LIST) MHAlloc( MaxSize );

    if ( !ModList ) {
        xosd = xosdOutOfMemory;
        goto Done;
    }

    //
    //  Build the list
    //
    Count = 0;

    for ( hmdi = NULL; (hmdi = LLNext( llmdi, hmdi )); LLUnlock( hmdi ) ) {

        lpmdi = (LPMDI) LLLock( hmdi );

        if (lpmdi && lpmdi->cobj == -1) {
            if (GetSectionObjectsFromDM( hpid, lpmdi ) != xosdNone) {
                continue;
            }
        }

        //
        //  Get the module name
        //
        p = (*(lpmdi->lszName) == _T('|')) ? lpmdi->lszName+1 : lpmdi->lszName;
        _ftcscpy( Name, p );
        p = _ftcschr( Name, _T('|') );
        if ( p ) {
            *p = _T('\0');
        }

        if ( lpModuleName ) {

            //
            //  Add if base name matches
            //
            _tsplitpath( Name, NULL, NULL, ModName, ModExt );

            if (_ftcsicmp(WantedName, ModName) || _ftcsicmp(WantedExt, ModExt) ) {
                continue;
            }
        }

        Delta = lpmdi->fFlatMode ? 1 : lpmdi->cobj;

        //
        //  Reallocate buffer if necessary
        //
        if ( (Count + Delta) * sizeof(MODULE_ENTRY) > MaxSize ) {

            MaxSize += Delta * sizeof(MODULE_ENTRY);
            TmpList = (LPMODULE_LIST) MHRealloc( ModList, MaxSize );
            if ( !TmpList ) {
                FreeModuleList(ModList);
                xosd = xosdOutOfMemory;
                break;
            }

            ModList = TmpList;
        }

        //
        //  have buffer, fill it up
        //
        if ( lpmdi->fFlatMode ) {

            Entry = NthModuleEntry(ModList,Count);

            assert( Is64PtrSE(lpmdi->lpBaseOfDll) );

            ModuleEntryFlat(Entry)          = TRUE;
            ModuleEntryReal(Entry)          = FALSE;
            ModuleEntrySegment(Entry)       = 0;
            ModuleEntrySelector(Entry)      = 0;
            ModuleEntryBase(Entry)          = lpmdi->lpBaseOfDll;
            ModuleEntryLimit(Entry)         = lpmdi->dwSizeOfDll;
            ModuleEntryType(Entry)          = 0;
            ModuleEntrySectionCount(Entry)  = lpmdi->cobj;
            ModuleEntryEmi(Entry)           = lpmdi->hemi;
            _ftcscpy(ModuleEntryName(Entry), Name);

            Count++;

        } else {

            for ( i=0, ObjD = lpmdi->rgobjd; i < Delta; i++, ObjD++ ) {

                if ( ObjD->wSel ) {

                    Selector = ObjD->wSel;

                    Entry    = NthModuleEntry(ModList,Count);

                    ModuleEntrySegment(Entry)       = i+1;
                    ModuleEntrySelector(Entry)      = Selector;
                    ModuleEntryType(Entry)          = 0;
                    ModuleEntrySectionCount(Entry)  = 0;
                    ModuleEntryEmi(Entry)           = lpmdi->hemi;

                    _ftcscpy(ModuleEntryName(Entry), Name);

                    if ( lpmdi->fRealMode ) {

                        xosd = xosdNone;

                        ModuleEntryFlat(Entry)          = FALSE;
                        ModuleEntryReal(Entry)          = TRUE;
                        ModuleEntryBase(Entry)          = 0xBAD00BAD;
                        ModuleEntryLimit(Entry)         = 0xBAD00BAD;

                        Count++;

                    } else {

                        xosd = SendRequestX( dmfQuerySelector,
                                             hpid,
                                             NULL,
                                             sizeof(Selector),
                                             &Selector 
                                             );

                        if (xosd == xosdNone) {


                            memcpy( &Ldt, LpDmMsg->rgb, sizeof(Ldt));

                            Base = (Ldt.HighWord.Bits.BaseHi  << 0x18) |
                                   (Ldt.HighWord.Bits.BaseMid << 0x10) |
                                   Ldt.BaseLow;

                            Limit = (Ldt.HighWord.Bits.LimitHi << 0x10) |
                                                    Ldt.LimitLow;

                            ModuleEntryFlat(Entry)          = FALSE;
                            ModuleEntryReal(Entry)          = FALSE;
                            ModuleEntryBase(Entry)          = Base;
                            ModuleEntryLimit(Entry)         = Limit;

                            Count++;

                        } else {

                            xosd = xosdNone;

                            ModuleEntryFlat(Entry)          = FALSE;
                            ModuleEntryReal(Entry)          = FALSE;
                            ModuleEntryBase(Entry)          = 0xBAD00BAD;
                            ModuleEntryLimit(Entry)         = 0xBAD00BAD;
                            Count++;
                        }
                    }
                }
            }
        }
    }

    if (hmdi) {
        LLUnlock(hmdi);
    }

    ModuleListCount(ModList) = Count;
    *lplpModuleList = ModList;

Done:
    return xosd;
}


XOSD
DoCustomCommand(
    HPID   hpid,
    HTID   htid,
    LPARAM wValue,
    LPSSS  lpsss
    )
{
    LPTSTR  lpsz = (LPTSTR)lpsss->rgbData;
    LPTSTR  p;
    XOSD   xosd;
    TCHAR  cmd[256];

    //
    // parse the command from the command line
    //
    p = cmd;
    while (*lpsz && !_istspace(*lpsz)) {
                _tccpy(p,lpsz);
                p = _tcsinc(p);
                lpsz = _tcsinc(lpsz);
    }
    *p = _T('\0');

    //
    // this is where you would stricmp() for your custom em command
    // otherwise it is passed to the dm
    //

    return SendRequestX( dmfSystemService, hpid, htid, (DWORD)wValue, (LPVOID) lpsss );

    //
    // this is what would be executed if you have a custom em command
    // instead of the above sendrequest()
    //

#if 0
    _tcscpy( lpiol->rgbVar, lpsz );
    xosd = IoctlCmd(hpid, htid, wValue, lpiol);
#endif

    return xosd;
}                                    /* DoCustomCommand */




XOSD
SystemService(
    HPID   hpid,
    HTID   htid,
    UINT_PTR  wValue,
    LPSSS  lpsss
    )

/*++

Routine Description:

    This function examines SystemService requests (escapes) and deals
    with those which the EM knows about.  All others are passed on to
    the DM for later processing.

Arguments:

    argument-name - Supplies | Returns description of argument.
    .
    .

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{
    XOSD        xosd;
    DWORD       dw;
    HPRC        hprc;
    HTHD        hthd;
    LPTHD       lpthd;


    switch( lpsss-> ssvc ) {

      case ssvcGetStackFrame:
        hprc = HprcFromHpid( hpid );
        hthd = HthdFromHtid ( hprc, htid );
        assert(hthd);
        lpthd = (LPTHD) LLLock(hthd);
        memcpy(lpsss->rgbData, &lpthd->StackFrame, sizeof(STACKFRAME64));
        lpsss->cbReturned = sizeof(STACKFRAME64);
        xosd = xosdNone;
        break;

      case ssvcGetThreadContext:

        xosd = SendRequest ( dmfReadReg, hpid, htid );
        if (xosd == xosdNone) {
            xosd = LpDmMsg->xosdRet;
            dw = min(lpsss->cbSend, (unsigned) SizeOfContext(hpid));
            memcpy (lpsss->rgbData, LpDmMsg->rgb, dw);
            lpsss->cbReturned = dw;
        }
        break;

      case ssvcSetThreadContext:
        xosd = SendRequestX( dmfWriteReg, hpid, htid, lpsss->cbSend,
                                                              lpsss->rgbData );
        break;

      case ssvcGetProcessHandle:
      case ssvcGetThreadHandle:
        xosd = SendRequestX(dmfSystemService,hpid, htid, (DWORD)wValue, (LPVOID)lpsss);
        if (xosd == xosdNone) {
            xosd = LpDmMsg->xosdRet;
            dw = min(lpsss->cbSend, sizeof(HANDLE));
            memcpy (lpsss->rgbData, LpDmMsg->rgb, dw);
            lpsss->cbReturned = dw;
        }
        break;


      case ssvcCustomCommand:
        xosd = DoCustomCommand(hpid, htid, wValue, lpsss);
        break;

      case ssvcGetPrompt:
        xosd = SendRequestX(dmfSystemService, hpid, htid, (DWORD)wValue, (LPVOID)lpsss);
        if (xosd == xosdNone) {
            xosd = LpDmMsg->xosdRet;
            lpsss->cbReturned = ((LPPROMPTMSG)((LPSSS)LpDmMsg->rgb))->len + sizeof(PROMPTMSG);
            if (lpsss->cbReturned) {
                memcpy((LPVOID)lpsss->rgbData,
                         LpDmMsg->rgb,
                         lpsss->cbReturned);
            }
        }
        break;
      case ssvcFiberDebug:
        {
            OFBRS ofbrs = *((OFBRS *) lpsss->rgbData);
            xosd = SendRequestX(dmfSystemService,hpid, htid, (DWORD)wValue, (LPVOID)lpsss);
            if ((xosd == xosdNone) &&
                    ((ofbrs.op == OFBR_GET_LIST) ||
                    (ofbrs.op == OFBR_QUERY_LIST_SIZE))) {
                xosd = LpDmMsg->xosdRet;
                dw = min(lpsss->cbSend, *((DWORD *) LpDmMsg->rgb)-4);
                memcpy (lpsss->rgbData, ((DWORD *)LpDmMsg->rgb)+1, dw);
                lpsss->cbReturned = dw;
            }
        }
        break;


      case ssvcGeneric:
        xosd = SendRequestX(dmfSystemService,hpid,htid,(DWORD)wValue,(LPVOID)lpsss);
        if (xosd == xosdNone) {
            PIOCTLGENERIC pig = (PIOCTLGENERIC)(LpDmMsg->rgb);
            xosd = LpDmMsg->xosdRet;
            lpsss->cbReturned = pig->length + sizeof(IOCTLGENERIC);
            if (lpsss->cbReturned) {
                memcpy ((LPVOID)lpsss->rgbData,
                        LpDmMsg->rgb,
                        lpsss->cbReturned);
            }
        }
        break;

      default:
        xosd = SendRequestX(dmfSystemService,hpid,htid,(DWORD)wValue,(LPVOID)lpsss);
        break;
    }

    return xosd;

}

XOSD
RangeStep (
    HPID   hpid,
    HTID   htid,
    LPRSS  lprss
    )

/*++

Routine Description:

    This function is called to implement range steps in the EM.  A range
    step is defined as step all instructions as long as the program counter
    remains within the starting and ending addresses.

Arguments:

    hpid      - Supplies the handle of the process to be stepped

    htid      - Supplies the handle of thread to be stepped

    lprss -

Return Value:

    XOSD error code

--*/

{
    XOSD  xosd = xosdNone;
    RST rst = {0};


    UpdateChild( hpid, htid, dmfRangeStep );

    rst.exop = *lprss->lpExop;
    rst.offStart = lprss->lpaddrMin->addr.off;
    rst.offEnd = lprss->lpaddrMax->addr.off;

    return SendRequestX (
        dmfRangeStep,
        hpid,
        htid,
        sizeof ( rst ),
        &rst
    );

}                           /* RangeStep() */

XOSD
SingleStep (
    HPID   hpid,
    HTID   htid,
    LPEXOP lpexop
    )
{
    assert ( hpid != NULL );
    assert ( htid != NULL );

    UpdateChild( hpid, htid, dmfSingleStep );

    return SendRequestX (
        dmfSingleStep,
        hpid,
        htid,
        sizeof(EXOP),
        lpexop
    );
}


int
__cdecl
CompMsg(
    void const *lpdwKeyMsg,
    void const *lpMessageInfo
    )
{
    if (*((LPDWORD)lpdwKeyMsg) < ((LPMESSAGEINFO)lpMessageInfo)->dwMsg) {
        return -1;
    } else if (*((LPDWORD)lpdwKeyMsg) > ((LPMESSAGEINFO)lpMessageInfo)->dwMsg) {
        return 1;
    } else {
        return 0;
    }
}

DWORD
GetMessageMask(
    DWORD dwMessage
)
{
    LPMESSAGEINFO lpMessageInfo;

    lpMessageInfo = (LPMESSAGEINFO)bsearch((const void*)&dwMessage,
                                           (const void*)MessageInfo,
                                           sizeof(MessageInfo)/sizeof(MESSAGEINFO),
                                           sizeof(MESSAGEINFO),
                                           CompMsg);

    if (lpMessageInfo) {
        return lpMessageInfo->dwMsgMask;
    } else {
        return 0;
    }
}

int
__cdecl
CompMESSAGEINFO(
    void const *lpMessageInfo1,
    void const *lpMessageInfo2
    )
{
    // a bit of code reuse
    return CompMsg(&((LPMESSAGEINFO)lpMessageInfo1)->dwMsg, lpMessageInfo2);
}

void
SortMessages (
    void
    )
{
    qsort(MessageInfo,
          sizeof(MessageInfo) / sizeof(MESSAGEINFO),
          sizeof(MESSAGEINFO),
          CompMESSAGEINFO);
}




XOSD
SetPath(
    HPID   hpid,
    HTID   htid,
    BOOL   Set,
    LPTSTR Path
    )
/*++

Routine Description:

    Sets the search path in the DM

Arguments:

    hpid    -   process
    htid    -   thread
    Set     -   set flag
    Path    -   Path to search, PATH if null


Return Value:

    xosd error code

--*/

{
    TCHAR    Buffer[ MAX_PATH ];
    SETPTH  *SetPth = (SETPTH *)&Buffer;

    if ( Set ) {

        SetPth->Set = TRUE;
        if ( Path ) {
            _ftcscpy(SetPth->Path, Path );
        } else {
            SetPth->Path[0] = _T('\0');
        }
    } else {
        SetPth->Set     = FALSE;
        SetPth->Path[0] = _T('\0');
    }

    return SendRequestX( dmfSetPath, 
                         hpid, 
                         htid,
                         sizeof(SETPTH) + _ftcslen(SetPth->Path)*sizeof(TCHAR),
                         SetPth 
                         );
}


XOSD
NewSymbolsLoaded(
    VOID
    )
/*++

Routine Description:

    Notifies the DM that new symbols have been
    reloaded, and that the DM should update any
    information it needs to.

Arguments:

Return Value:

    xosd error code

--*/

{
    return SendRequest( dmfNewSymbolsLoaded,
            NULL, // Grab the first TL the debugger can find
            NULL
            );
}
