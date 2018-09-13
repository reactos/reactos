/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    emdp2.c

Abstract:

    This file contains the some of the machine independent portions of the
    execution model.  The machine dependent portions are in other files.

Author:

    Kent Forschmiedt (kentf) 10-23-92

Environment:

    Win32 -- User

Notes:

    The original source for this came from the CodeView group.

--*/
#include "emdp.h"
#include "resource.h"

#include <stdio.h>

extern CRITICAL_SECTION csCache;


#define DECL_XOSD(n,v,s) {IDS_##n, NULL, n},
static struct _EMERROR {
    UINT    uID;
    LPTSTR  lpsz;
    XOSD    xosd;
} EmErrors[] = {
#include "xosd.h"
};
const int nErrors = (sizeof(EmErrors)/sizeof(*EmErrors));
#undef DECL_XOSD


#define WINDBG_POINTERS_MACROS_ONLY
#include "sundown.h"
#undef WINDBG_POINTERS_MACROS_ONLY


void
UpdateProcess (
    HPRC hprc
    );

BOOL
UpdateStackRegisters(
    HPRC hprc,
    HTHD hthd
    );



LPCTSTR
EmError(
    XOSD    xosd
    )
{
    int i;

    for (i = 0; i < nErrors; i++) {
        if (EmErrors[i].xosd == xosd) {
            break;
        }
    }

    //
    // If it didn't work, use the first entry
    //
    if (i >= nErrors) {
        i = 0;
    }

    //
    // Just get the pointer to the resource
    //
    if (EmErrors[i].lpsz == NULL) {
        LoadString (GetModuleHandle(NULL), EmErrors[i].uID, (LPSTR)&EmErrors[i].lpsz, 0);
    }

    return EmErrors[i].lpsz;
}

VOID
FreeEmErrorStrings(
    VOID
    )
{
}



XOSD
LoadFixups (
    HPID  hpid,
    HTID  htid,
    MODULELOAD *lpmdl
    )
/*++

Routine Description:

    This function is called in response to a module load message.  It
    will cause information to be internally setup for doing fixups/
    unfixups ...

Arguments:

    hpid        - Supplies a handle for the process

    lpmdl       - Supplies a pointer to a module load message from the DM

Return Value:

    xosd Error code

--*/

{
    XOSD            xosd = xosdNone;
    HMDI            hmdi;
    MDI *           lpmdi;
    LPTSTR          lpName;
    HPRC            hprc = HprcFromHpid( hpid );
    HLLI            llmdi = LlmdiFromHprc ( hprc );
    LPPRC           lpprc;
    int             cobj;
    //DWORD           fIsRemote;
    //LPSTR           p1;


    hmdi = LLCreate ( llmdi );
    if ( hmdi == 0 ) {
        assert( "load dll cannot create llmdi" && FALSE );
        return xosdOutOfMemory;
    }

    lpmdi = (MDI *)LLLock ( hmdi );

    assert( Is64PtrSE(lpmdl->lpBaseOfDll) );

    lpmdi->mte  = lpmdl->mte;
    lpmdi->lpBaseOfDll = lpmdl->lpBaseOfDll;
    lpmdi->dwSizeOfDll = lpmdl->dwSizeOfDll;
    lpmdi->StartingSegment = lpmdl->StartingSegment;

    lpmdi->CSSel  = lpmdl->CSSel;
    lpmdi->DSSel  = lpmdl->DSSel;
    lpmdi->lpBaseOfData = lpmdl->uoffDataBase;
    lpmdi->fRealMode = lpmdl->fRealMode;
    lpmdi->fOffset32 = lpmdl->fOffset32;
    lpmdi->fFlatMode = lpmdl->fFlatMode;
    lpmdi->fSendNLG = FALSE;
    lpmdi->nlg.fEnable = FALSE;
    lpmdi->lpDebug = LpDebugToBeLoaded;
    if (lpmdi->fFlatMode) {
        lpprc = (LPPRC) LLLock( hprc );
        lpprc->selFlatCs = lpmdi->CSSel;
        lpprc->selFlatDs = lpmdi->DSSel;
        LLUnlock( hprc );
    }

    // Thread local storage info (iTls is 1 based)

    lpmdi->isecTLS = lpmdl->isecTLS;
    lpmdi->iTls = lpmdl->iTls;
    lpmdi->uoffiTls = lpmdl->uoffiTls;

    //
    // REVIEW:BUG how do we get the count of tls indexes?
    //

    if (lpmdi->uoffiTls || lpmdi->iTls) {
        lpprc->cmdlTLS++;
    }

    //
    // cobj might be -1 or 0.  If it is -1, the objdir load is
    // deferred until we need it.  If it is 0, later segloads
    // will add sections one at a time.
    //

    cobj = lpmdi->cobj = lpmdl->cobj;
    if (cobj == -1) {
        cobj = 0;
    }

    lpmdi->rgobjd = NULL;
    lpName = ( (LPTSTR) &( lpmdl->rgobjd[cobj] ) );

    lpmdi->lszName = MHStrdup ( lpName );
    if ( lpmdi->lszName == NULL )  {
        LLUnlock( hmdi );
        assert( "load dll cannot dup mod name" && FALSE );
        return xosdOutOfMemory;
    }

    if (cobj) {
        lpmdi->rgobjd = (LPOBJD) MHAlloc ( sizeof(OBJD) * lpmdi->cobj);
        if ( lpmdi->rgobjd == NULL ) {
            LLUnlock( hmdi );
            assert( "load cannot create rgobjd" && FALSE );
            return xosdOutOfMemory;
        }
        memcpy ( lpmdi->rgobjd,
                 lpmdl->rgobjd,
                 sizeof(OBJD) * lpmdi->cobj);
    }

    LLAdd ( llmdi, hmdi );

#if 0
    //
    // If there is an hfile here, regardless of whether the target is
    // remote the hfile must become an OSDEBUG hfile.
    //

    //
    // The string is either "name" or "|name|time|cksum|hfile|imagebase|"
    // If there is an hfile, translate it.
    //

    hfile = -1;

    p1 = _tcschr(p1+1, '|');           // name
    if (p1) {
        p1 = p1 && _tcschr(p1+1, '|'); // time
        p1 = p1 && _tcschr(p1+1, '|'); // cksum
        p1 = p1 && _tcschr(p1+1, '|'); // hfile
        p1 = p1 && (p1 + 1);          //   first digit

        assert(p1 && *p1);

        if (p1) {
            DWORD dw;
            assert(p1[8] == '|');  // what, me trust the DM?
            dw = _tcstoul(p1, &p2, 16);
            xosd = OSDMakeFileHandle(hpid, dw, &dw);
            // this can only fail because of xosdOutOfMemory or a bad hpid.
            assert(xosd == xosdNone);
            sprintf(p1, "%08X", dw);
            p1[8] = '|';
        }
    }

    DebugMetric ( hpid, NULL, mtrcRemote, &fIsRemote );

    hfile = SHLocateSymbolFile( lpmdi->lszName, fIsRemote );

    //
    // get the pointer to the debug data (fpo/pdata/omap)
    //

    lpmdi->lpDebug = GetDebugData( hfile );

    //
    // the hfile stays open.  the SH will finish with it later.
    //

#endif // later...

    LLUnlock(hprc);

    xosd = CallDB ( dbcModLoad,
                    hpid,
                    htid,
                    CEXM_MDL_native,
                    lpmdl->fThreadIsStopped,
                    (DWORD64)lpmdi->lszName
                    );

    LLUnlock ( hmdi );

    return xosd;
}



XOSD
UnLoadFixups (
    HPID hpid,
    HEMI hemi
    )
/*++

Routine Description:

    This function is called in response to a module unload message.

    It returns the emi of the module being unloaded

Arguments:

    hprc        - Supplies a handle for the process

    hemi        - Supplies hemi (if Unload)

Return Value:

    TRUE if deleted

--*/

{
    HLLI  hlli;
    HMDI  hmdi;
    LPPRC lpprc;
    LPMDI lpmdi;
    HPRC  hprc = HprcFromHpid(hpid);
    XOSD  xosd = xosdGeneral;

    lpprc = (LPPRC) LLLock(hprc);

    if (lpprc) {
        hlli = lpprc->llmdi;
        hmdi = LLFind( hlli, 0, (LPVOID)&hemi, (LONG) emdiEMI);
        if (hmdi) {
            UnRegisterEmi(lpprc, hmdi);
            xosd = xosdNone;
        }
    }

    LLUnlock(hprc);

    if (xosd == xosdNone) {
        LLDelete( hlli, hmdi );
    }

    return xosd;
}                               /* UnLoadFixups() */

void
UnRegisterEmi(
    LPPRC lpprc,
    HMDI hmdi
    )
{
    HLLI  hlli;
    LPMDI lpmdi;
    NLG   nlg;

    lpmdi = (LPMDI) LLLock(hmdi);

    if (lpprc->dmi.fNonLocalGoto) {

        //
        // disable non-local goto hooks
        //

        if ( lpmdi->nlg.fEnable ) {

            lpmdi->nlg.fEnable = FALSE;
            nlg = lpmdi->nlg;

//          SwapNlg ( &nlg );

            SendRequestX (
                dmfNonLocalGoto,
                lpprc->hpid,
                NULL,
                sizeof ( nlg ),
                &nlg
                );
        }
    }

    lpmdi->hemi = 0;

    LLUnlock ( hmdi );
}



XOSD
CreateThreadStruct (
    HPID hpid,
    TID tid,
    HTID *lphtid
    )
{
    HPRC  hprc  = HprcFromHpid ( hpid );
    LPPRC lpprc = (LPPRC) LLLock ( hprc );
    HTHD  hthd  = hthdNull;
    LPTHD lpthd = NULL;


    hthd = HthdFromTid ( hprc, tid );
    assert(hthd == NULL);
    if ( hthd == hthdNull ) {

        hthd  = LLCreate ( lpprc->llthd );
        lpthd = (LPTHD) LLLock ( hthd );

        CallDB ( dbcoCreateThread,
                 hpid,
                 NULL,
                 CEXM_MDL_native,
                 sizeof ( HTID ),
                 (DWORD64)lphtid
                 );

        lpthd->htid   = *lphtid;
        lpthd->hprc   = hprc;
        lpthd->tid    = tid;
        lpthd->drt    = drtNonePresent;
        lpthd->dwcbSpecial = lpprc->dmi.cbSpecialRegs;
        if (lpthd->dwcbSpecial) {
            lpthd->pvSpecial = MHAlloc(lpthd->dwcbSpecial);
        }

        lpthd->regs = MHAlloc(SizeOfContext(hpid));
        lpthd->frameRegs = MHAlloc(SizeOfContext(hpid));

        if (MPTFromHthd(hthd) == mptia64) {
            lpthd->pvStackRegs = MHAlloc(SizeOfStackRegisters(hpid));
            lpthd->dwcbStackRegs = 0;
        } else {
            lpthd->pvStackRegs = NULL;
            lpthd->dwcbStackRegs = 0;
        }

        LLAdd ( lpprc->llthd, hthd );
    }
    else {
        lpthd = (LPTHD) LLLock ( hthd );
        assert ( lpthd->fVirtual == TRUE );
        *lphtid = lpthd->htid;
        lpthd->fVirtual = FALSE;
        lpthd->drt    = drtNonePresent;
    }

    lpthd->fFlat = TRUE;  // Assume flat to start off.

    LLUnlock ( hthd );
    LLUnlock ( hprc );

    return xosdNone;
}                              /* CreateThreadStruct() */


VOID
SyncHprcWithDM(
    HPID hpid
    )
{
    // ideally this would be a struct of DBB plus an EXCMD, but padding gets
    // in the way
    struct {
        DBB dbb;
        BYTE pad[sizeof(EXCMD)];
    } rgb = {0};
    LPDBB    pdbb = &rgb.dbb;
    LPEXCMD lpcmd = (LPEXCMD)&rgb.dbb.rgbVar;
    LPEXCEPTION_CONTROL lpexc = &lpcmd->exc;
    LPEXCEPTION_DESCRIPTION lpexd = &lpcmd->exd;
    LPEXCEPTION_DESCRIPTION lpexdr;
    HEXD hexd;
    HPRC hprc;
    HLLI llexc;
    LONG lT;
    XOSD xosd;

    hprc = HprcFromHpid(hpid);
    if (!hprc) {
        return;
    }
    llexc = ((LPPRC)LLLock(hprc))->llexc;
    LLUnlock(hprc);

    //
    // force the DMINFO struct to get loaded
    //
    // Until this completes, we cannot do any cpu-specific operations.
    //

    DebugMetric ( hpid, NULL, mtrcProcessorType, &lT );


    //
    // Clear out old exception info...
    //
    while (hexd = LLNext(llexc, NULL)) {
        LLDelete(llexc, hexd);
    }

    //
    // and get current exception info
    //

    pdbb->dmf  = dmfGetExceptionState;
    pdbb->hpid = hpid;
    pdbb->htid = NULL;
    *lpexc = exfFirst;

    do {

        xosd = CallTL(tlfRequest, hpid, sizeof(rgb), (DWORD64)&rgb);
        if ((xosd != xosdNone) || (LpDmMsg->xosdRet != xosdNone)) {
            break;
        }

        //
        // add to local exception list
        //
        hexd = LLCreate( llexc );
        LLAdd( llexc, hexd );
        lpexdr = (LPEXCEPTION_DESCRIPTION) LLLock( hexd );
        *lpexdr = *((LPEXCEPTION_DESCRIPTION)(LpDmMsg->rgb));
        LLUnlock( hexd );

        //
        // ask for the next one
        //
        *lpexd = *((LPEXCEPTION_DESCRIPTION)(LpDmMsg->rgb));
        *lpexc = exfNext;

    } while (1); // lpexd->dwExceptionCode != 0); /* 0 is valid exception code */

    UpdateProcess(hprc);
}


XOSD
CreateHprc (
    HPID hpid
    )
{
    XOSD  xosd = xosdNone;
    HPRC  hprc;
    LPPRC lpprc;

    hprc = LLCreate ( llprc );

    if ( hprc == 0 ) {
        return xosdOutOfMemory;
    }

    LLAdd ( llprc, hprc );

    lpprc = (LPPRC) LLLock ( hprc );

    lpprc->stat = statDead;
    lpprc->hpid = hpid;
    lpprc->pid  = (PID) 0;
    lpprc->fDmiCache = 0;

    lpprc->llthd = LLInit (
        sizeof ( THD ),
        llfNull,
        TiDKill,
        TDComp
    );

    if ( lpprc->llthd == 0 ) {
        xosd = xosdOutOfMemory;
    }

    lpprc->llmdi = LLInit ( sizeof ( MDI ), llfNull, MDIKill, MDIComp );

    if ( lpprc->llmdi == 0 ) {
        xosd = xosdOutOfMemory;
    }

    lpprc->llexc = LLInit ( sizeof(EXCEPTION_DESCRIPTION),
                            llfNull,
                            NULL,
                            EXCComp );
    if ( lpprc->llexc == 0 ) {
        xosd = xosdOutOfMemory;
    }

    LLUnlock ( hprc );

    return xosd;
}

VOID
DestroyHprc (
    HPRC hprc
    )
{
    EnterCriticalSection(&csCache);

    LLDelete ( llprc, hprc );
    FlushPTCache();

    LeaveCriticalSection(&csCache);
}

VOID
DestroyHthd(
    HTHD hthd
    )
{
    LPTHD lpthd;
    HPRC  hprc;

    EnterCriticalSection(&csCache);

    lpthd = (LPTHD) LLLock ( hthd );
    hprc = lpthd->hprc;
    LLUnlock ( hthd );
    LLDelete ( LlthdFromHprc ( hprc ), hthd );
    FlushPTCache();

    LeaveCriticalSection(&csCache);
}

void EMENTRY
PiDKill (
    LPVOID lpv
    )
{
    LPPRC lpprc = (LPPRC) lpv;
    LLDestroy ( lpprc->llthd );
    LLDestroy ( lpprc->llmdi );
    LLDestroy ( lpprc->llexc );
}

void EMENTRY
TiDKill (
    LPVOID lpv
    )
{
    LPTHD lpthd = (LPTHD) lpv;

    if (lpthd->pvSpecial) {
        MHFree(lpthd->pvSpecial);
    }

    if (lpthd->regs) {
        MHFree(lpthd->regs);
    }

    if (lpthd->frameRegs) {
        MHFree(lpthd->frameRegs);
    }

    if (lpthd->pvStackRegs) {
        MHFree(lpthd->pvStackRegs);
    }
}

void EMENTRY
MDIKill(
    LPVOID lpv
    )
{
    LPMDI lpmdi = (LPMDI)lpv;
    if (lpmdi->lszName) {
        MHFree(lpmdi->lszName);
        lpmdi->lszName = NULL;
    }
    if (lpmdi->rgobjd) {
        MHFree(lpmdi->rgobjd);
        lpmdi->rgobjd = NULL;
    }
    if ((lpmdi->lpDebug != NULL) && (lpmdi->lpDebug != LpDebugToBeLoaded)) {
        lpmdi->lpDebug = NULL;
    }
}


int EMENTRY
PDComp (
    LPVOID lpv1,
    LPVOID lpv2,
    LONG lParam
    )
{

    Unreferenced(lParam);

    if ( ( (LPPRC) lpv1)->hpid == *( (LPHPID) lpv2 ) ) {
        return fCmpEQ;
    }
    else {
        return fCmpLT;
    }
}

int EMENTRY
TDComp (
    LPVOID lpv1,
    LPVOID lpv2,
    LONG lParam
    )
{

    Unreferenced(lParam);

    if ( ( (LPTHD) lpv1)->htid == *( (LPHTID) lpv2 ) ) {
        return fCmpEQ;
    }
    else {
        return fCmpLT;
    }
}


int EMENTRY
MDIComp (
    LPVOID lpv1,
    LPVOID lpv,
    LONG lParam
    )
{
    LPMDI lpmdi = (LPMDI) lpv1;

    switch ( lParam ) {

        case emdiName:
            if ( !_ftcschr( (const char *) lpv, _T('|') ) ) {
                TCHAR Buffer[MAX_PATH];
                LPTSTR p1,p2;
                p1 = lpmdi->lszName;
                if ( *p1 == _T('|') ) {
                    p1 = _tcsinc(p1);
                }
                p2 = _ftcschr(p1, _T('|'));
                if ( !p2 ) {
                    p2 = p1 + _ftcslen(p1);
                }
                memcpy(Buffer, p1, (size_t)(p2-p1)*sizeof(TCHAR));
                Buffer[p2-p1]=_T('\0');
                return _tcsicmp ( (const char *) lpv, Buffer );

            } else {
                return _tcsicmp ( (const char *) lpv, lpmdi->lszName );
            }

        case emdiEMI:
            return !(lpmdi->hemi == *(( HEMI * ) lpv ) );

        case emdiMTE:
            return !(lpmdi->mte == *((LPWORD) lpv ));

        case emdiBaseAddr:
            assert(Is64PtrSE(lpmdi->lpBaseOfDll));
            assert(Is64PtrSE( *((OFFSET *) lpv) ));
            return !( lpmdi->lpBaseOfDll == *((OFFSET *) lpv) );

        case emdiNLG:
            return !lpmdi->fSendNLG;

        default:
            return (0);
            break;
    }
}


int
EMENTRY EXCComp(
    LPVOID lpRec,
    LPVOID lpVal,
    LONG lParam
    )
{
    Unreferenced(lParam);
    if ( ((LPEXCEPTION_DESCRIPTION)lpRec)->dwExceptionCode == *((LPDWORD)lpVal)) {
        return fCmpEQ;
    } else {
        return fCmpLT;
    }
}


DWORD
RvaOmapLookup(
    DWORD rva,
    LPOMAP  rgomap,
    DWORD   comap
    )
{
    OMAP  *pomapLow;
    OMAP  *pomapHigh;

    pomapLow = rgomap;
    pomapHigh = rgomap + comap;

    while (pomapLow < pomapHigh) {
        unsigned    comapHalf;
        OMAP  *pomapMid;

        comapHalf = comap / 2;

        pomapMid = pomapLow + ((comap & 1) ? comapHalf : (comapHalf - 1));

        if (rva == pomapMid->rva) {
            return(pomapMid->rvaTo);
        }

        if (rva < pomapMid->rva) {
            pomapHigh = pomapMid;
            comap = (comap & 1) ? comapHalf : (comapHalf - 1);
        }

        else {
            pomapLow = pomapMid + 1;
            comap = comapHalf;
        }
    }

    assert(pomapLow == pomapHigh);

    // If no exact match, pomapLow points to the next higher address

    if (pomapLow == rgomap) {
        // This address was not found

        return(0);
    }

    if (pomapLow[-1].rvaTo == 0) {
        // This address is in a deleted/inserted range

        return(0);
    }

    // Return the new address plus the bias

    return(pomapLow[-1].rvaTo + (rva - pomapLow[-1].rva));
}

XOSD
GetObjLength(
    HPID hpid,
    LPGOL lpgol
    )
{
    SEGMENT segAddr;

    SetEmi ( hpid, lpgol->lpaddr );

    *(lpgol->lplBase) = 0L;
    segAddr  = GetAddrSeg ( *(lpgol->lpaddr) );

    // SwapEndian ( &wSegAddr, sizeof ( wSegAddr ) );

    SendRequestX ( dmfSelLim, hpid, NULL, sizeof (segAddr), &segAddr);

    // SwapEndian ( lpbBuffer, sizeof ( LONG ) );
    *(lpgol->lplLen) = * ( (LONG *) LpDmMsg->rgb );

    return(xosdNone);
}

//
// given a section number, find where it started pre-lego
// and return its physical segment
//
ULONG
FindPreLegoSection(
    LPMDI lpmdi,
    ULONG wSeg,
    WORD *pSeg
    )
{
    ULONG iSeg;
    LPGSI lpgsi = lpmdi->lpgsi;
    DWORD LastSectionStart = *(DWORD*)lpmdi->lpDebug->lpOmapTo;
    DWORD LastSectionSize = 0;
    DWORD ImageAlign = 0x1000;              // AMPHACK for rpcrt4 only

    for (iSeg=0; iSeg<lpgsi->csgMax; iSeg++) {
        if (iSeg==wSeg) {
            *pSeg = lpmdi->rgobjd[(lpgsi->rgsgi[iSeg].isgPhy - 1)].wSel;
            return LastSectionStart;
        }

        LastSectionSize = (lpgsi->rgsgi[iSeg].cbSeg + ImageAlign -1) & ~(ImageAlign-1);
        LastSectionStart += LastSectionSize;
    }

    return (ULONG)-1;
}

//
// given an address in the pre-lego file, find its segment and offset
// returns (UOFFSET)-1 if failed to find
//
UOFFSET
FindPreLegoSegment(
    LPMDI lpmdi,
    UOFFSET uoff,
    UOFFSET *pOffset
    )
{
    ULONG iSeg;
    LPGSI lpgsi = lpmdi->lpgsi;
    UOFFSET LastSectionStart = *(DWORD*)lpmdi->lpDebug->lpOmapTo;
    DWORD LastSectionSize = 0;
    DWORD ImageAlign = 0x1000;              // AMPHACK for rpcrt4 only

    assert( Is64PtrSE(lpmdi->lpBaseOfDll) );

    uoff -= lpmdi->lpBaseOfDll;

    if (uoff >= LastSectionStart) {
        for (iSeg=0; iSeg < lpgsi->csgMax; iSeg++) {

            LastSectionSize = (lpgsi->rgsgi[iSeg].cbSeg + ImageAlign -1) & ~(ImageAlign-1);

            if (uoff < (LastSectionStart+LastSectionSize) ) {
                *pOffset = uoff - LastSectionStart;
                return iSeg;
            }

            LastSectionStart += LastSectionSize;
        }
    }

    *pOffset = 0;
    return (UOFFSET)-1;
}


XOSD
FixupAddr (
    HPID   hpid,
    HTID   htid,
    LPADDR lpaddr
    )
/*++

Routine Description:

    This routine is used to convert addresses between linker index (section
    or segment relative) addresses and real addresses (segment:offset).



Arguments:

    hpid        - Supplies the handle to the process for context to convert
                        the address.
    lpaddr      - Pointer to address packet to be converted.

Return Value:

    XOSD error code.

--*/

{
    HMDI  hmdi;
    HPRC  hprc;
    HTHD  hthd;

    hprc = ValidHprcFromHpid(hpid);
    hthd = HthdFromHtid( hprc, htid );

    assert( Is64PtrSE(lpaddr->addr.off) );

    /*
     *  Check to see if the address is already a segment:offset pair and
     *  return if it is.
     */

    if ( !ADDR_IS_LI(*lpaddr) ) {
        return xosdNone;
    }

    /*
     *  Now based on the emi field of the address (which uniquely defines
     *  the executable module in the symbol handler), get the conversion
     *  information.
     */

    assert( emiAddr( *lpaddr ) != 0 );

    if ( (HPID)emiAddr ( *lpaddr ) == hpid ) {

        ADDR_IS_LI(*lpaddr) = FALSE;

        if (MPTFromHthd(hthd) != mptix86) {
            /*
             * The opposite of the code in UnFixupAddr -- Remove the 1
             *      which was stuck in to make sure we did not think it was
             *      an absolute
             */
            lpaddr->addr.seg = 0;
        }

        emiAddr( *lpaddr ) = 0;
        SetEmi( hpid, lpaddr );
    } else {

        /*
         * Based on the symbol handler handle find our internal data structure
         *      for the dll.
         */

        hmdi = LLFind ( LlmdiFromHprc ( HprcFromHpid(hpid) ), 0,
                       (LPVOID)&emiAddr ( *lpaddr ), (LONG) emdiEMI );

        if ( hmdi == 0 ) {

            return xosdUnknown ; // Do we need a special xosd for this.
                                 // A common case where this will happen is pre-loading symbols.

        } else {

            LPMDI lpmdi = (LPMDI) LLLock ( hmdi );
            WORD  wsel;
            LPSGI lpsgi;
            unsigned short seg;

            /*
             *  If we could not find an internal structure for the DLL
             *  then it must be some type of error.
             */

            if ( lpmdi == NULL ) {
                return xosdUnknown;
            }

            if (lpmdi->cobj == -1) {
                if (GetSectionObjectsFromDM( hpid, lpmdi ) != xosdNone) {
                    return xosdUnknown;
                }
            }

            //  Make sure symbols have been loaded

            VerifyDebugDataLoaded(hpid, htid, lpmdi);

            /*
             *  If the segment/selector is 0 then it must be an absolute
             *  symbol and we therefore don't need to do any conversion.
             *
             *  If we could get no information describing the symbol
             *  information then we can't do any conversion.
             */

            if ( (GetAddrSeg( *lpaddr ) > 0) && (lpmdi->lpgsi) ) {

                /*
                 * Get the linker index number for the segment number
                 *      and assure that it is valid.
                 */

                wsel = (WORD) (GetAddrSeg( *lpaddr ) - 1);
                if ( wsel >= lpmdi->lpgsi->csgMax ) {
                    /*
                     * Linker index is either not valid or not yet loaded
                     */

                    return xosdUnknown;
                }
                else {

                    /*
                     *  We know which section it comes from.  To compute
                     *  the real offset we need to add the following
                     *  items together.
                     *
                     *  original offset                GetAddrOff( *lpaddr )
                     *  offset of index in section     lpsgi->doffseg
                     *      (this is the group offset)
                     *  offset of section from base of rgobjd[physSeg-1].offset
                     *          image
                     *
                     *
                     *  The segment can just be loaded from the MAP.  Notice
                     *  that we will infact "lose" information in this
                     *  conversion sometimes.  Specifically a cs:data address
                     *  after unfixup and fixup will come out ds:data.  This
                     *  is "expected" behavior.
                     */
                    if (lpmdi->lpDebug && lpmdi->lpDebug->lpOmapFrom) {
                        //
                        // component has been legoed, so translate logical address
                        // using pre-lego section starts
                        //
                        WORD pSeg;
                        DWORD off = FindPreLegoSection( lpmdi, wsel, &pSeg );
                        if (off != (DWORD)-1) {
                            // Address is still LI, so only 32 bits
                            assert((GetAddrOff( *lpaddr ) >> 32) == 0);
                            off = RvaOmapLookup( off + (DWORD)GetAddrOff( *lpaddr ),
                                                lpmdi->lpDebug->lpOmapFrom,
                                                lpmdi->lpDebug->cOmapFrom
                                                );
                            if (off) {

                                GetAddrOff ( *lpaddr ) = off + lpmdi->lpBaseOfDll;
                                GetAddrSeg ( *lpaddr ) = pSeg;
                                ADDR_IS_REAL(*lpaddr) = (BYTE)lpmdi->fRealMode;
                                ADDR_IS_OFF32(*lpaddr) = (BYTE)lpmdi->fOffset32;
                                ADDR_IS_FLAT(*lpaddr) = (BYTE)lpmdi->fFlatMode;
                                ADDR_IS_LI(*lpaddr) = FALSE;

                                assert( Is64PtrSE(GetAddrOff ( *lpaddr )) );

                                return xosdNone;
                            }
                        }
                    }

                    lpsgi = &lpmdi->lpgsi->rgsgi[ wsel ];

                    // Let's not underflow the array
                    if (0 == lpsgi->isgPhy) {
                        return xosdUnknown;
                    }
                    if (lpmdi->rgobjd[(lpsgi->isgPhy-1)].wPad == 0) {
                        return xosdUnknown;
                    }

                    GetAddrOff ( *lpaddr ) += lpsgi->doffseg;

                    if ( hthd && lpsgi->isgPhy == lpmdi->isecTLS ) {
                        GetAddrOff ( *lpaddr ) += GetTlsBase ( hprc,
                                                               hthd,
                                                               lpmdi->uoffiTls,
                                                               &lpmdi->iTls
                                                               );
                        seg = 0;
                    }
                    else {

                        GetAddrOff( *lpaddr ) +=
                            (UOFFSET) (lpmdi->rgobjd[ (lpsgi->isgPhy - 1) ]. offset);

                        seg = lpmdi->rgobjd[(lpsgi->isgPhy - 1)].wSel;
                    }
                }

                GetAddrSeg ( *lpaddr ) = seg;
            }

            /*
             *  Set the bits describing the address
             */

            assert( Is64PtrSE(GetAddrOff(*lpaddr)) );

            ADDR_IS_REAL(*lpaddr) = lpmdi->fRealMode;
            ADDR_IS_OFF32(*lpaddr) = lpmdi->fOffset32;
            ADDR_IS_FLAT(*lpaddr) = lpmdi->fFlatMode;
            ADDR_IS_LI(*lpaddr) = FALSE;

            //
            // Now release the module description
            //

            LLUnlock ( hmdi );
        }
    }

    return xosdNone;
}                               /* FixupAddr() */



XOSD
UnFixupAddr(
    HPID   hpid,
    HTID   htid,
    LPADDR lpaddr
    )

/*++

Routine Description:

    This routine is called to convert addresses from Real Physical addresses
    to linker index addresses.  Linker index addresses have an advantage
    to the symbol handler in that we know which DLL the address is in.

    The result of calling UnFixupAddr should be one of the following:

    1.  A true Linker Index address.  In this case
        emi == the HEXE (assigned by SH) for the DLL containning the address
        seg == the Section number of the address
        off == the offset in the Section

    2.  Address not in a dll.  In this case
        emi == the HPID of the current process
        seg == the physical selector of the address
        off == the offset in the physical selector

    3.  An error

Arguments:

    hpid   - Supplies the handle to the process the address is in
    lpaddr - Supplies a pointer to the address to be converted.  The
             address is converted in place

Return Value:

    XOSD error code

--*/

{
    HPRC        hprc;
    HTHD        hthd;
    LPPRC       lpprc;
    LDT_ENTRY   ldt;
    XOSD        xosd;

    assert( Is64PtrSE(lpaddr->addr.off) );

    /*
     *  If the address already has the Linker Index bit set then there
     *  is no work for use to do.
     */


    if ( ADDR_IS_LI(*lpaddr) ) {
        return xosdNone;
    }

    /*
     *  If the EMI field in the address is not already filled in, then
     *  we will now fill it in.
     */

    if ( emiAddr ( *lpaddr ) == 0 ) {
        SetEmi ( hpid, lpaddr );
    }

    /*
     *  Get the internal Process Descriptor structure
     */

    hprc = HprcFromHpid(hpid);
    hthd = HthdFromHtid( hprc, htid );

    /*
     *  Is the EMI we got from the address equal to the process handle?
     *  if so then we cannot unfix the address and should just set the
     *  bits in the mode field.
     */

    if ( (HPID)emiAddr ( *lpaddr ) != hpid ) {
        LPMDI lpmdi;
        HMDI  hmdi = LLFind (LlmdiFromHprc ( hprc ), 0,
                             (LPVOID)&emiAddr ( *lpaddr ), (LONG) emdiEMI);
        WORD            igsn;
        LPSGI           lpsgi;
        UOFFSET         ulo;
        USHORT          seg;
        ULONG           iSeg;


        if (hmdi == 0) {
            /*
             * If we get here we are really messed up.  We have a valid (?)
             *  emi field set in the ADDR packeet, it is not the process
             *  handle, but it does not correspond to a known emi in the
             *  current process.  Therefore bail out as an error
             */

            return xosdUnknown;
        }

        lpmdi = (LPMDI) LLLock ( hmdi );
        if ( lpmdi == NULL ) {
            return xosdUnknown;
        }

        if (lpmdi->cobj == -1) {
            if (GetSectionObjectsFromDM( hpid, lpmdi ) != xosdNone) {
                return xosdUnknown;
            }
        }

        //
        //  Ensure that we have symbols loaded for this module, otherwise
        //      we may get a bad supprise later
        //

        VerifyDebugDataLoaded(hpid, htid, lpmdi);

        /*
         * Start out by using the "default" set of fields.  These
         *      are based on what our best guess is for the executable
         *      module.  This is based on what the DM told use when
         *      it loaded the exe.
         */

        ADDR_IS_REAL(*lpaddr) = lpmdi->fRealMode;
        ADDR_IS_OFF32(*lpaddr) = lpmdi->fOffset32;
        ADDR_IS_FLAT(*lpaddr) = lpmdi->fFlatMode;

        /*
         *  If there is not table describing the layout of segments in
         *      the exe, there is no debug information and there fore no
         *      need to continue this process.
         */

        if ( lpmdi->lpgsi == NULL ) {
            LLUnlock( hmdi );
            emiAddr( *lpaddr ) = (HEMI) hpid;
            goto itsBogus;
        }

        if (lpmdi->lpDebug && lpmdi->lpDebug->lpOmapTo) {
            assert( Is64PtrSE(lpaddr->addr.off) );
            assert( Is64PtrSE(lpmdi->lpBaseOfDll) );
            ulo = RvaOmapLookup( (DWORD)(lpaddr->addr.off - lpmdi->lpBaseOfDll),
                                 lpmdi->lpDebug->lpOmapTo,
                                 lpmdi->lpDebug->cOmapTo
                                 );

            if (!ulo) {

                //
                // Address cannot be mapped.  Removed by Lego.
                //

                ADDR_IS_LI (*lpaddr) = TRUE;
                lpaddr->emi = (HEMI) hpid;
                LLUnlock ( hmdi );
                return xosdUnknown;
            }

            ulo += lpmdi->lpBaseOfDll;

            {
                UOFFSET seg2;
                UOFFSET off2;
                seg2 = FindPreLegoSegment( lpmdi, ulo, &off2 );
                if (seg2 != (UOFFSET)-1) {
                    
                    assert( Is64PtrSE(off2) );

                    GetAddrSeg( *lpaddr ) = (USHORT) (seg2 + 1);
                    GetAddrOff( *lpaddr ) = off2;
                    ADDR_IS_LI( *lpaddr ) = TRUE;
#ifdef TEST_FIXUP
                    {
                        ADDR fixedaddr = *lpaddr;
                        FixupAddr( hpid, htid, &fixedaddr );
                        if (GetAddrSeg( testAddr ))
                            assert( GetAddrSeg( fixedaddr ) == GetAddrSeg( testAddr ) );
                        assert( GetAddrOff( fixedaddr ) == GetAddrOff( testAddr ) );
                    }
#endif
                    return xosdNone;
                }
            }

        } else {
            ulo = GetAddrOff( *lpaddr );
        }


        seg = (USHORT) GetAddrSeg( *lpaddr );
        lpsgi = lpmdi->lpgsi->rgsgi;


        /*
         *  First correct out the "segment" portion of the offset.
         *
         *  For flat addresses this means that we locate which section
         *      number the address fell in and adjust back to that section
         *
         *  For non-flat address this mains locate which segment number
         *      the selector matches
         */

        if (ADDR_IS_FLAT( *lpaddr )) {
            for ( iSeg=0; iSeg < lpmdi->cobj; iSeg++) {
                if ((lpmdi->rgobjd[ iSeg ].offset <= ulo) &&
                    (ulo < (UOFFSET) (lpmdi->rgobjd[ iSeg ].offset +
                                     lpmdi->rgobjd[ iSeg].cb))) {

                    ulo -= lpmdi->rgobjd[ iSeg ].offset;
                    break;
                }
            }
        } else {
            for (iSeg=0; iSeg < lpmdi->cobj; iSeg++) {
                if (lpmdi->rgobjd[iSeg].wSel == seg) {
                    break;
                }
            }
        }

        if (iSeg == lpmdi->cobj) {
            // This was not a normal section, so now check to see if it is a TLS section

#if DBG
            if ( (!lpmdi->iTls || !lpmdi->uoffiTls) && ((PVOID)lpmdi->iTls != (PVOID)lpmdi->uoffiTls)) {
                char buf[1000];

                assert( Is64PtrSE(lpmdi->lpBaseOfDll) );

                sprintf(buf, "WINDBG: UnFixupAddress: uoffTls == %I64x, iTls == %d, lszName == %s, lpBaseOfDll == %I64x\n",
                        lpmdi->uoffiTls,
                        lpmdi->iTls,
                        lpmdi->lszName,
                        lpmdi->lpBaseOfDll
                        );
                OutputDebugString(buf);
            }
#endif

            if ( lpmdi->iTls && lpmdi->uoffiTls ) {
                LPPRC lpprc = (LPPRC) LLLock ( hprc );

                __try {
                    if ( lpprc->cmdlTLS > 0 ) {
                        UOFFSET uoffT = GetTlsBase (
                            hprc,
                            hthd,
                            lpmdi->uoffiTls,
                            &lpmdi->iTls
                            );

                        if (
                            uoffT != 0 &&
                            GetAddrOff ( *lpaddr ) >= uoffT &&
                            GetAddrOff ( *lpaddr ) <  uoffT +
                                    lpmdi->rgobjd [ lpmdi->isecTLS ].cb
                        ) {
                            iSeg = lpmdi->isecTLS - 1;
                        }
                        else {
                            lpaddr->mode.fIsLI = TRUE;
                            emiAddr ( *lpaddr ) = (HEMI)hpid;
                            LLUnlock ( hprc );
                            goto itsBogus;
                        }
                    }
                } __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {

#if DBG
                    char buf[1000];

                    assert( Is64PtrSE(lpmdi->lpBaseOfDll) );

                    sprintf(buf, "WINDBG: Bad TLS: uoffTls == %I64x, iTls == %d, lszName == %s, lpBaseOfDll == %I64x\n",
                            lpmdi->uoffiTls,
                            lpmdi->iTls,
                            lpmdi->lszName,
                            lpmdi->lpBaseOfDll
                            );
                    OutputDebugString(buf);
#else
                    ;
#endif

                }

                LLUnlock ( hprc );
            }

            emiAddr( *lpaddr ) = (HEMI) hpid;
            goto itsBogus;
        }

        iSeg += 1;

        for( igsn=0; igsn < lpmdi->lpgsi->csgMax; igsn++, lpsgi++ ) {

            if ( (ULONG)lpsgi->isgPhy == iSeg &&
                lpsgi->doffseg <= ulo &&
                ulo < lpsgi->doffseg + lpsgi->cbSeg ) {

                GetAddrSeg( *lpaddr ) = (USHORT) (igsn + 1);
                GetAddrOff( *lpaddr ) = ulo - lpsgi->doffseg;
                assert( Is64PtrSE(GetAddrOff( *lpaddr )) );

                break;
            }
        }

        if (igsn == lpmdi->lpgsi->csgMax) {
            LLUnlock ( hmdi );
            emiAddr( *lpaddr ) = (HEMI) hpid;
            goto itsBogus;
        }
        LLUnlock ( hmdi );

        ADDR_IS_LI(*lpaddr) = TRUE;

    } else {
    itsBogus:
        if (ADDR_IS_REAL( *lpaddr )) {
            ADDR_IS_FLAT( *lpaddr ) = FALSE;
            ADDR_IS_OFF32( *lpaddr ) = FALSE;
        } else {
            /*
             * See if the segment matches the flat segment.  If it does not
             *      then we must be in a non-flat segment.
             */

            lpprc = (LPPRC) LLLock( hprc );

            if ((lpaddr->addr.seg == 0) ||
                (lpprc->dmi.fAlwaysFlat) ||
                (lpaddr->addr.seg == lpprc->selFlatCs) ||
                (lpaddr->addr.seg == lpprc->selFlatDs)) {

                ADDR_IS_FLAT(*lpaddr) = TRUE;
                ADDR_IS_OFF32(*lpaddr) = TRUE;
                ADDR_IS_REAL(*lpaddr) = FALSE;

            } else {

                xosd = SendRequestX(dmfQuerySelector, hpid, htid,
                                    sizeof(SEGMENT), &GetAddrSeg(*lpaddr)  );

                if (xosd != xosdNone) {
                    LLUnlock(hprc);
                    return xosd;
                }

                _fmemcpy( &ldt, LpDmMsg->rgb, sizeof(ldt));

                ADDR_IS_FLAT(*lpaddr) = FALSE;
                ADDR_IS_OFF32(*lpaddr) = (BYTE) ldt.HighWord.Bits.Default_Big;
                ADDR_IS_REAL(*lpaddr) = FALSE;
            }
            LLUnlock( hprc );
        }

        if ( MPTFromHprc(hprc) != mptix86) {

            /*
             *      This line is funny.  We assume that all addresses
             *      which have a segment of 0 to be absolute symbols.
             *      We therefore set the segment to 1 just to make sure
             *      that it is not zero.
             */

            if (emiAddr(*lpaddr) == (HEMI) hpid) {
               lpaddr->addr.seg = 1;
            }
        }
    }

    //ADDR_IS_LI(*lpaddr) = TRUE;
    return xosdNone;
}                               /* UnFixupAddr() */



void
UpdateRegisters (
    HPRC hprc,
    HTHD hthd
    )
{
    LPTHD lpthd = (LPTHD) LLLock ( hthd );
    HPID hpid = HpidFromHprc(hprc);

    SendRequest ( dmfReadReg, hpid, HtidFromHthd ( hthd ) );
    _fmemcpy ( lpthd->regs, LpDmMsg->rgb, SizeOfContext(hpid) );

    if (MPTFromHthd(hthd) == mptia64) {
        //
        // make stack regs update as part of register update
        //
        UpdateStackRegisters(hprc, hthd);
    }

    lpthd->drt = (DRT) (drtCntrlPresent | drtAllPresent);

    LLUnlock ( hthd );
}




void
RegisterEmi (
    HPID   hpid,
    HTID   htid,
    LPREMI lpremi
    )
{
    HLLI     llmdi;
    HMDI     hmdi;
    LPMDI    lpmdi;
    HPRC     hprc = HprcFromHpid ( hpid );

    llmdi = LlmdiFromHprc( hprc );
    assert( llmdi != 0 );

    hmdi = LLFind( llmdi, 0, lpremi->lsz, (LONG)emdiName );

    if (hmdi == 0) {
        hmdi = LLFind( llmdi, 0, &lpremi->hemi, (LONG)emdiEMI );
    }

    assert( hmdi != 0 );

    lpmdi = (LPMDI) LLLock ( hmdi );
    assert( lpmdi != NULL );

    if (lpmdi->hemi != 0) {
        UnRegisterEmi( (LPPRC)LLLock(hprc), hmdi );
        LLUnlock(hprc);
    }

    assert( lpremi->hemi != 0 );

    lpmdi->hemi = lpremi->hemi;

    lpmdi->lpDebug = LpDebugToBeLoaded;


    LLUnlock ( hmdi );

    // purge the emi cache (get rid of old, now invalid hpid/emi pairs)
    CleanCacheOfEmi();
}


void
UpdateProcess (
    HPRC hprc
    )
{
    assert ( hprc != NULL );
    EnterCriticalSection(&csCache);

    {
        LPPRC lpprc = (LPPRC) LLLock ( hprc );

        FlushPTCache();

        pointersCurr = PointersFromMPT(lpprc->dmi.Processor.Type);
        if (pointersCurr) {
            hprcCurr = hprc;
            hpidCurr = lpprc->hpid;
            pidCurr  = lpprc->pid;
            mptCurr = lpprc->dmi.Processor.Type;
        } else {
            hprcCurr = 0;
            hpidCurr = 0;
            pidCurr  = 0;
            mptCurr = -1;
        }

        LLUnlock ( hprc );
    }
    LeaveCriticalSection(&csCache);
}


void
UpdateThread (
    HTHD hthd
    )
{
    EnterCriticalSection(&csCache);

    if ( hthd == NULL ) {
        FlushPTCache();
    } else {
        LPTHD lpthd = (LPTHD) LLLock ( hthd );

        UpdateProcess ( lpthd->hprc );

        hthdCurr = hthd;
        htidCurr = lpthd->htid;
        tidCurr  = lpthd->tid;

        LLUnlock ( hthd );
    }
    LeaveCriticalSection(&csCache);
}



HEMI
HemiFromHmdi (
    HMDI hmdi
    )
{
    LPMDI lpmdi = (LPMDI) LLLock ( hmdi );
    HEMI  hemi = lpmdi->hemi;

    LLUnlock ( hmdi );
    return hemi;
}


void
FlushPTCache (
    void
    )
{
    EnterCriticalSection(&csCache);

    hprcCurr = NULL;
    hpidCurr = NULL;
    pidCurr  = 0;
    pointersCurr = NULL;
    mptCurr = (MPT)-1;

    hthdCurr = NULL;
    htidCurr = NULL;
    tidCurr =  0;

    LeaveCriticalSection(&csCache);
}


EMEXPORT HPRC
ValidHprcFromHpid(
    HPID hpid
    )
/*++

Routine Description:

    only return an hprc if there is a real process for it.
    the other version will return an hprc whose process has
    not been created or has been destroyed.

Arguments:

    hpid  - Supplies hpid to look for in HPRC list.

Return Value:

    An HPRC or NULL.

--*/
{
    HPRC hprcT;
    HPRC hprc = NULL;
    LPPRC lpprc;

    EnterCriticalSection(&csCache);

    if ( hpid == hpidCurr ) {

        hprc = hprcCurr;

    } else {

        if ( hpid != NULL ) {
            hprc = LLFind ( llprc, NULL, (LPVOID)&hpid, 0 );
        }

        if ( hprc != NULL ) {
            lpprc = (LPPRC) LLLock( hprcT = hprc );
            if (lpprc->stat == statDead) {
                hprc = NULL;
            }
            LLUnlock( hprcT );
        }
        if ( hprc != NULL ) {
            UpdateProcess ( hprc );
        }
    }

    LeaveCriticalSection(&csCache);

    return hprc;
}


HPRC
HprcFromHpid (
    HPID hpid
    )
{
    HPRC hprc = NULL;

    EnterCriticalSection(&csCache);

    if ( hpid == hpidCurr ) {

        hprc = hprcCurr;

    } else {

        if ( hpid != NULL ) {
            hprc = LLFind ( llprc, NULL, (LPVOID)&hpid, 0 );
        }

        if ( hprc != NULL ) {
            UpdateProcess ( hprc );
        }
    }

    LeaveCriticalSection(&csCache);

    return hprc;
}


EMEXPORT HPRC
HprcFromPid (
    PID pid
    )
{
    HPRC hprc;
    BOOL fFound = FALSE;

    EnterCriticalSection(&csCache);

    if ( pid == pidCurr ) {

        hprc = hprcCurr;

    } else {

        for ( hprc = LLNext ( llprc, 0 );
              !fFound && hprc != 0;
              hprc = LLNext ( llprc, hprc ) ) {

            LPPRC lpprc = (LPPRC) LLLock ( hprc );
            fFound = lpprc->pid == pid;
            LLUnlock ( hprc );
        }

        if ( !fFound ) {
            hprc = NULL;
        } else {
            UpdateProcess ( hprc );
        }
    }

    LeaveCriticalSection(&csCache);

    return hprc;
}


EMEXPORT HPID
HpidFromHprc (
    HPRC hprc
    )
{
    HPID hpid = NULL;

    EnterCriticalSection(&csCache);

    if ( hprc == hprcCurr ) {
        hpid = hpidCurr;
    } else if ( hprc != NULL ) {
        UpdateProcess ( hprc );
        hpid = hpidCurr;
    }

    LeaveCriticalSection(&csCache);

    return hpid;
}


PID
PidFromHprc (
    HPRC hprc
    )
{
    PID pid = 0;

    EnterCriticalSection(&csCache);

    if ( hprc == hprcCurr ) {
        pid = pidCurr;
    } else if ( hprc != NULL ) {
        UpdateProcess ( hprc );
        pid = pidCurr;
    }

    LeaveCriticalSection(&csCache);

    return pid;
}


EMEXPORT HTHD
HthdFromTid (
    HPRC hprc,
    TID tid
    )
{
    LPPRC lpprc;
    HTHD  hthd = NULL;
    BOOL  fFound = FALSE;

    EnterCriticalSection(&csCache);

    if ( hprc == hprcCurr && tid == tidCurr ) {
        hthd = hthdCurr;
    } else {
        lpprc = (LPPRC) LLLock ( hprc );

        for ( hthd = LLNext ( lpprc->llthd, 0 );
              !fFound && hthd != 0;
              hthd = LLNext ( lpprc->llthd, hthd ) ) {

            LPTHD lpthd = (LPTHD) LLLock ( hthd );
            fFound = lpthd->tid == tid;

            LLUnlock ( hthd );
        }

        LLUnlock ( hprc );

        if ( fFound ) {
            UpdateThread ( hthd );
        } else {
            hthd = NULL;
        }
    }

    LeaveCriticalSection(&csCache);

    return hthd;
}


EMEXPORT HTHD
HthdFromHtid (
    HPRC hprc,
    HTID htid
    )
{
    HTHD  hthd = NULL;

    EnterCriticalSection(&csCache);

    if (HandleToLong(htid) & 1) {// HACK around vhtid
       htid = (HTID) (((INT_PTR)htid) ^ 1);
    }
    if ( hprc == hprcCurr && htid == htidCurr ) {
        hthd = hthdCurr;
    } else if ( hprc != NULL ) {
        LPPRC lpprc = (LPPRC) LLLock ( hprc );
        hthd  = LLFind ( lpprc->llthd, NULL, (LPVOID)&htid, 0 );
        LLUnlock ( hprc );
    }
    UpdateThread ( hthd );

    LeaveCriticalSection(&csCache);

    return hthd;
}


EMEXPORT HTID
HtidFromHthd (
    HTHD hthd
    )
{
    HTID htid = NULL;

    EnterCriticalSection(&csCache);

    if ( hthd != hthdCurr ) {
        UpdateThread ( hthd );
    }
    htid = htidCurr;

    LeaveCriticalSection(&csCache);

    return htid;
}


TID
TidFromHthd (
    HTHD hthd
    )
{
    TID tid = 0;

    EnterCriticalSection(&csCache);

    if ( hthd != hthdCurr ) {
        UpdateThread ( hthd );
    }
    tid = tidCurr;

    LeaveCriticalSection(&csCache);

    return tid;
}

EMEXPORT HPID
HpidFromHthd(
    HTHD hthd
    )
{
    HPID hpid = NULL;

    EnterCriticalSection(&csCache);

    if (hthd != hthdCurr) {
        UpdateThread( hthd );
    }
    hpid = hpidCurr;

    LeaveCriticalSection(&csCache);

    return hpid;
}

HLLI
LlthdFromHprc (
    HPRC hprc
    )
{
    HLLI llthd = 0;

    if ( hprc != NULL ) {
        LPPRC lpprc = (LPPRC) LLLock ( hprc );
        llthd = lpprc->llthd;
        LLUnlock ( hprc );
    }

    return llthd;
}

HLLI
LlmdiFromHprc (
    HPRC hprc
    )
{
    HLLI llmdi = 0;

    if ( hprc != NULL ) {
        LPPRC lpprc = (LPPRC) LLLock ( hprc );
        llmdi = lpprc->llmdi;
        LLUnlock ( hprc );
    }

    return llmdi;
}

STAT
StatFromHprc (
    HPRC hprc
    )
{
    LPPRC lpprc = (LPPRC) LLLock ( hprc );
    STAT  stat  = lpprc->stat;
    LLUnlock ( hprc );
    return stat;
}

MPT
MPTFromHthd(
    HTHD hthd
    )
{
    MPT m = (MPT)-1;
    EnterCriticalSection(&csCache);
    if (hthd != hthdCurr) {
        UpdateThread(hthd);
    }
    m = mptCurr;
    LeaveCriticalSection(&csCache);
    return m;
}

MPT
MPTFromHprc(
    HPRC hprc
    )
{
    MPT m = (MPT)-1;
    EnterCriticalSection(&csCache);
    if (hprc != hprcCurr) {
        UpdateProcess(hprc);
    }
    m = mptCurr;
    LeaveCriticalSection(&csCache);
    return m;
}


PCPU_POINTERS
PointersFromMPT(
    MPT mpt
    )
{
extern CPU_POINTERS X86Pointers;
extern CPU_POINTERS AxpPointers;
extern CPU_POINTERS IA64Pointers;
    switch (mpt) {
        case mptix86:
            return &X86Pointers;

        case mptdaxp:
            return &AxpPointers;

        case mptia64:
            return &IA64Pointers;

        default:
            //
            // this will happen the first time it is
            // called, during the first call to SyncHPRCWithDM.
            //
            return NULL;
    }
}

PCPU_POINTERS
PointersFromHpid(
    HPID hpid
    )
{
    PCPU_POINTERS p = NULL;

    EnterCriticalSection(&csCache);

    if (hpid == hpidCurr || HprcFromHpid(hpid)) {
        p = pointersCurr;
    }

    LeaveCriticalSection(&csCache);

    return p;
}

//**************************************************************************
//
// global stack walking api support functions
//
// these are the callbacks used by dbghelp.dll
//
// there are custom callbacks in each of the emdpdev.c files
//
//**************************************************************************


BOOL
SwReadMemory(
    LPVOID  lpvhpid,
    DWORD64 lpBaseAddress,
    LPVOID  lpBuffer,
    DWORD   nSize,
    LPDWORD lpNumberOfBytesRead
    )
{
    ADDR   addr;
    DWORD  cb;
    XOSD   xosd;
    HPID   hpid = (HPID)lpvhpid;

    addr.addr.off     = lpBaseAddress;
    addr.addr.seg     = 0;
    addr.emi          = 0;
    addr.mode.fFlat   = TRUE;
    addr.mode.fOff32  = FALSE;
    addr.mode.fIsLI   = FALSE;
    addr.mode.fReal   = FALSE;

    xosd = ReadBuffer( hpid, NULL, &addr, nSize, (LPBYTE) lpBuffer, &cb );
    if (xosd != xosdNone) {
        return FALSE;
    }

    if (lpNumberOfBytesRead) {
        *lpNumberOfBytesRead = cb;
    }

    return TRUE;
}


HMDI
SwGetMdi(
    HPID    hpid,
    DWORD64 Address
    )
{
    HLLI        hlli  = 0;
    HMDI        hmdi  = 0;
    LPMDI       lpmdi = NULL;

    assert( Is64PtrSE(Address) );


    hlli = LlmdiFromHprc( HprcFromHpid ( hpid ));

    do {

        hmdi = LLNext( hlli, hmdi );
        if (hmdi) {
            lpmdi = (LPMDI)LLLock( hmdi );
            if (lpmdi) {
                //
                // we have a pointer to a module so lets see if its the one...
                //

                assert( Is64PtrSE(lpmdi->lpBaseOfDll) );

                if (Address >= lpmdi->lpBaseOfDll &&
                    Address <  lpmdi->lpBaseOfDll+lpmdi->dwSizeOfDll ) {

                    LLUnlock( hmdi );
                    return hmdi;

                }
                LLUnlock( hmdi );
            }
        }

    } while (hmdi);

    return 0;
}


DWORD64
SwGetModuleBase(
    LPVOID  lpvhpid,
    DWORD64 ReturnAddress
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
        UOFFSET BaseOfDll = lpmdi->lpBaseOfDll;
        LLUnlock( hmdi );
        return BaseOfDll;
    }

    return 0;
}

// TLS
XOSD
InitTLS (
    HPRC hprc,
    HTHD hthd
    )
{
    LPPRC   lpprc = (LPPRC) LLLock ( hprc );
    LPTHD   lpthd = (LPTHD) LLLock ( hthd );
    ADDR    addr  = {0};
    DWORD   cb    = 0;
    XOSD    xosd  = xosdNone;

    if ( lpthd->uoffTEB == 0 ) {
        xosd = xosdBadThread;
    }
    else {

        UINT     cbRead   = 0;

        ADDR_IS_OFF32( addr ) = TRUE;
        ADDR_IS_FLAT ( addr ) = TRUE;

        if (lpprc->dmi.fPtr64) {
            ULONG64 d;
            // BUGBUG fix this value for 64 bit TEB
            GetAddrOff ( addr ) = lpthd->uoffTEB + 0x2C;
            ReadBuffer ( lpprc->hpid,
                         lpthd->htid,
                         &addr,
                         sizeof ( ULONG64 ),
                         (LPBYTE) &d,
                         &cb
                         );
            if ( cb != sizeof ( ULONG64 ) ) {
                xosd = xosdRead;
            } else {
                SetAddrOff(&addr, d);
            }
        } else {
            ULONG d;
            GetAddrOff ( addr ) = lpthd->uoffTEB + 0x2C;
            ReadBuffer ( lpprc->hpid,
                         lpthd->htid,
                         &addr,
                         sizeof ( ULONG ),
                         (LPBYTE) &d,
                         &cb
                         );
            if ( cb != sizeof ( ULONG ) ) {
                xosd = xosdRead;
            } else {
                SE_SetAddrOff(&addr, d);
            }
        }


    }

    if ( xosd == xosdNone && GetAddrOff ( addr ) != 0 ) {
        lpthd->rguoffTlsBase = (UOFFSET *) MHAlloc ( lpprc->cmdlTLS * sizeof ( UOFFSET ) );

        if (lpprc->dmi.fPtr64) {
            ReadBuffer ( lpprc->hpid,
                         lpthd->htid,
                         &addr,
                         lpprc->cmdlTLS * sizeof ( UOFFSET ),
                         (LPBYTE)lpthd->rguoffTlsBase,
                         &cb
                         );
            if ( cb != lpprc->cmdlTLS * sizeof ( UOFFSET ) ) {
                xosd = xosdRead;
            }
        } else {
            PULONG pd = (PULONG) MHAlloc ( lpprc->cmdlTLS * sizeof ( ULONG ) );
            ReadBuffer(lpprc->hpid,
                       lpthd->htid,
                       &addr,
                       lpprc->cmdlTLS * sizeof ( ULONG ),
                       (LPBYTE)pd,
                       &cb
                       );
            if ( cb != lpprc->cmdlTLS * sizeof ( ULONG ) ) {
                xosd = xosdRead;
            } else {
                DWORD i;
                for (i = 0; i < lpprc->cmdlTLS; i++) {
                    lpthd->rguoffTlsBase[i] = pd[i];
                }
            }
            MHFree(pd);
        }
    }

    LLUnlock ( hprc );
    LLUnlock ( hthd );

    return xosd;
}

XOSD
ReadTlsIndex(
    HPRC hprc,
    LPADDR lpAddr,
    DWORD * lpiTls
    )
{
    LPPRC pprc = (LPPRC) LLLock( hprc );
    DWORD iTls;
    DWORD cb;
    XOSD xosd = sizeof(iTls);

    assert(hprc != hprcInvalid && hprc != hprcNull);
    assert(!ADDR_IS_LI(*lpAddr));

    if ( pprc->stat == statRunning ) {
        // Always fetch a new value if we are running.
        xosd = ReadBuffer(pprc->hpid, NULL, lpAddr, sizeof (iTls), (LPBYTE)&iTls, &cb);

        // This is not required but invalidate the tls cache anyway.
        pprc->iTlsCache.fValid = FALSE;
    }
    else {

        // If the cache is not valid or the addresses don't match fetch a new value.
        if ( !pprc->iTlsCache.fValid || !FAddrsEq(*lpAddr, pprc->iTlsCache.addr) ) {
            xosd = ReadBuffer(pprc->hpid, NULL, lpAddr, sizeof( iTls), (LPBYTE)&(pprc->iTlsCache.iTls), &cb);
            if ( xosd == xosdNone && cb == sizeof(iTls) ) {
                pprc->iTlsCache.addr = *lpAddr ;
                pprc->iTlsCache.fValid = TRUE;
            }
            else {
                pprc->iTlsCache.fValid = FALSE;
            }
        }

        iTls =  pprc->iTlsCache.iTls;
    }

    LLUnlock( hprc );

    *lpiTls = iTls;
    return xosd;
}

UOFFSET
GetTlsBase (
    HPRC hprc,
    HTHD hthd,
    UOFFSET uoffiTls,
    DWORD * lpiTls
    )
{
    LPTHD   lpthd = (LPTHD) LLLock ( hthd );
    UOFFSET uoffRet = 0;
    DWORD   iTls = *lpiTls - 1;

    assert ( lpthd->uoffTEB );

    if ( iTls == (DWORD) -1 && uoffiTls ) {
        // freshen the iTls value with the value from the data section
        LPPRC   lpprc = (LPPRC) LLLock ( hprc );
        ADDR    addr = {0};

        assert ( lpprc->cmdlTLS );
        GetAddrOff ( addr ) = uoffiTls;
        GetAddrSeg ( addr ) = 0; //lpthd->regx86.ds;
        emiAddr ( addr ) = 0;
        ADDR_IS_OFF32 ( addr ) = TRUE;
        ADDR_IS_FLAT( addr ) = TRUE;
        ReadTlsIndex ( hprc, &addr, &iTls);
        if ( iTls < lpprc->cmdlTLS ) {
            *lpiTls = iTls + 1;
        }
        else {
            iTls = (DWORD)-1;
        }
        LLUnlock ( hprc );
    }

    if ( lpthd->rguoffTlsBase == NULL ) {
        // We haven't retrieved the TLSBase array from the DM yet
        // REVIEW:POSSIBLE BUG: NULL == 0 and with dead threads, we may
        // be trying to do too much with them.  maybe another sentinel value?
        InitTLS ( hprc, hthd );
    }

    if ( lpthd->rguoffTlsBase != NULL ) {
        if ( iTls != (DWORD)-1 ) {
            uoffRet = lpthd->rguoffTlsBase[ iTls ];
        }
    }

    LLUnlock ( hthd );

    return uoffRet;
}



XOSD
GetFrameRegValue (
    HPID hpid,
    HTID htid,
    DWORD ireg,
    LPVOID lpvRegValue
    )
{
    LPTHD       lpthd;
    HTHD hthd = HthdFromHtid(HprcFromHpid(hpid), htid);

    assert ( hthd != hthdNull );

    lpthd = (LPTHD) LLLock ( hthd );

    lpvRegValue = DoGetReg(hpid, lpthd->frameRegs, ireg & 0xff, lpvRegValue );

    if ( lpvRegValue != NULL ) {
        ireg = ireg >> 8;
        if ( ireg != CV_REG_NONE ) {
            lpvRegValue = DoGetReg( hpid, lpthd->frameRegs, ireg, lpvRegValue );
        }
    }

    LLUnlock ( hthd );

    if ( lpvRegValue == NULL ) {
        return xosdInvalidParameter;
    }

    return xosdNone;
}                             /* GetFrameRegValue */




XOSD
XXSetFlagValue (
    HPID   hpid,
    HTID   htid,
    DWORD  iFlag,
    LPVOID lpvRegValue
    )
{
    HPRC        hprc;
    HTHD        hthd;
    LPTHD       lpthd;
    LPVOID      lpregs;
    LONG        mask;
    LONG        l;

    hprc = ValidHprcFromHpid(hpid);
    if (!hprc) {
        return xosdBadProcess;
    }
    hthd = HthdFromHtid(hprc, htid);
    assert ( hthd != hthdNull );

    lpthd = (LPTHD) LLLock( hthd );

    lpregs = lpthd->regs;

    if ( !( lpthd->drt & drtAllPresent )) {
        UpdateRegisters ( lpthd->hprc, hthd );
    }


    if ( DoGetReg( hpid, lpregs, Rgfd(hpid)[iFlag].fd.dwId, &l ) == NULL) {
        LLUnlock( hthd );
        return xosdInvalidParameter;
    }

    mask = (1 << Rgfd(hpid)[iFlag].fd.dwcbits) - 1;
    mask <<= Rgfd(hpid)[iFlag].iShift;
    l &= ~mask;
    l |= ((*((ULONG *) lpvRegValue)) << Rgfd(hpid)[iFlag].iShift) & mask;
    DoSetReg(hpid, lpregs, Rgfd(hpid)[iFlag].fd.dwId, &l );

    lpthd->drt = (DRT) (lpthd->drt | drtAllDirty);
    LLUnlock ( hthd );
    return xosdNone;
}                             /* SetFlagValue */


XOSD
XXGetFlagValue (
    HPID hpid,
    HTID htid,
    DWORD iFlag,
    LPVOID lpvRegValue
    )
{
    HPRC      hprc;
    HTHD      hthd;
    LPTHD     lpthd;
    LPCONTEXT lpregs;
    DWORD     value;

    hprc = ValidHprcFromHpid(hpid);
    if (!hprc) {
        return xosdBadProcess;
    }
    hthd = HthdFromHtid(hprc, htid);

    assert ( hthd != NULL );
    lpthd = (LPTHD) LLLock ( hthd );

    lpregs = (LPCONTEXT) lpthd->regs;

    if ( !(lpthd->drt & drtAllPresent) ) {
        UpdateRegisters ( hprc, hthd );
    }

    if (DoGetReg ( hpid, lpregs, Rgfd(hpid)[iFlag].fd.dwId, &value ) == NULL) {
        LLUnlock( hthd );
        return xosdInvalidParameter;
    }

    value = (value >> Rgfd(hpid)[iFlag].iShift) & ((1 << Rgfd(hpid)[iFlag].fd.dwcbits) - 1);
    *( (LPLONG) lpvRegValue) = value;

    LLUnlock(hthd);
    return xosdNone;
}


XOSD
SaveRegs(
    HPID hpid,
    HTID htid,
    LPHIND lphmem
    )
{
    HPRC      hprc;
    HTHD      hthd;
    LPTHD     lpthd;

    hprc = ValidHprcFromHpid(hpid);
    if (!hprc) {
        return xosdBadProcess;
    }

    *(LPVOID*)lphmem = MHAlloc(SizeOfContext(hpid));
    if (!*lphmem) {
        return xosdOutOfMemory;
    }

    hthd = HthdFromHtid(hprc, htid);
    assert ( hthd != NULL );
    lpthd = (LPTHD) LLLock ( hthd );

    _fmemcpy ( *(LPVOID*)lphmem, lpthd->regs, SizeOfContext(hpid) );

    LLUnlock(hthd);
    return xosdNone;
}


RestoreRegs(
    HPID hpid,
    HTID htid,
    HIND hmem
    )
{
    HPRC      hprc;
    HTHD      hthd;
    LPTHD     lpthd;

    hprc = ValidHprcFromHpid(hpid);
    if (!hprc) {
        return xosdBadProcess;
    }

    hthd = HthdFromHtid(hprc, htid);
    assert ( hthd != NULL );
    lpthd = (LPTHD) LLLock ( hthd );

    _fmemcpy ( lpthd->regs, (LPVOID)hmem, SizeOfContext(hpid) );

    lpthd->drt = (DRT) (lpthd->drt | drtAllDirty);

    MHFree((LPVOID)hmem);

    LLUnlock(hthd);

    return xosdNone;
}




UOFFSET
ConvertOmapFromSrc(
    LPMDI       lpmdi,
    UOFFSET     addr
    )
{
    DWORD   rva;
    DWORD   comap;
    LPOMAP  pomapLow;
    LPOMAP  pomapHigh;
    DWORD   comapHalf;
    LPOMAP  pomapMid;

    assert( Is64PtrSE(addr) );

    if (!lpmdi) {
        return addr;
    }

    if ( lpmdi->lpgsi == NULL ) {
        SHWantSymbols( (HEXE)lpmdi->hemi );
        lpmdi->lpgsi = (LPGSI)SHLpGSNGetTable( (HEXE)lpmdi->hemi );
    }

    if ((!lpmdi->lpDebug) || (!lpmdi->lpDebug->lpOmapFrom)) {
        return addr;
    }

    assert( Is64PtrSE(lpmdi->lpBaseOfDll) );

    rva = (DWORD)(addr - lpmdi->lpBaseOfDll);

    comap = lpmdi->lpDebug->cOmapFrom;
    pomapLow = lpmdi->lpDebug->lpOmapFrom;
    pomapHigh = pomapLow + comap;

    while (pomapLow < pomapHigh) {

        comapHalf = comap / 2;

        pomapMid = pomapLow + ((comap & 1) ? comapHalf : (comapHalf - 1));

        if (rva == pomapMid->rva) {
            return lpmdi->lpBaseOfDll + pomapMid->rvaTo;
        }

        if (rva < pomapMid->rva) {
            pomapHigh = pomapMid;
            comap = (comap & 1) ? comapHalf : (comapHalf - 1);
        } else {
            pomapLow = pomapMid + 1;
            comap = comapHalf;
        }
    }

    assert(pomapLow == pomapHigh);

    //
    // If no exact match, pomapLow points to the next higher address
    //
    if (pomapLow == lpmdi->lpDebug->lpOmapFrom) {
        //
        // This address was not found
        //
        return 0;
    }

    if (pomapLow[-1].rvaTo == 0) {
        //
        // This address is not translated so just return the original
        //
        return addr;
    }

    //
    // Return the closest address plus the bias
    //
    return lpmdi->lpBaseOfDll + pomapLow[-1].rvaTo + (rva - pomapLow[-1].rva);
}


UOFFSET
ConvertOmapToSrc(
    LPMDI       lpmdi,
    UOFFSET     addr
    )
{
    DWORD   rva;
    DWORD   comap;
    LPOMAP  pomapLow;
    LPOMAP  pomapHigh;
    DWORD   comapHalf;
    LPOMAP  pomapMid;
    INT     i;

    assert( Is64PtrSE(addr) );

    if (!lpmdi) {
        return addr;
    }

    if ( lpmdi->lpgsi == NULL ) {
        SHWantSymbols( (HEXE)lpmdi->hemi );
        lpmdi->lpgsi = (LPGSI)SHLpGSNGetTable( (HEXE)lpmdi->hemi );
    }

    if ((!lpmdi->lpDebug) || (!lpmdi->lpDebug->lpOmapTo)) {
        return addr;
    }

    assert( Is64PtrSE(lpmdi->lpBaseOfDll) );

    rva = (DWORD)(addr - lpmdi->lpBaseOfDll);

    comap = lpmdi->lpDebug->cOmapTo;
    pomapLow = lpmdi->lpDebug->lpOmapTo;
    pomapHigh = pomapLow + comap;

    while (pomapLow < pomapHigh) {

        comapHalf = comap / 2;

        pomapMid = pomapLow + ((comap & 1) ? comapHalf : (comapHalf - 1));

        if (rva == pomapMid->rva) {
            if (pomapMid->rvaTo == 0) {
                //
                // We are probably in the middle of a routine
                //
                i = -1;
                while ((&pomapMid[i] != lpmdi->lpDebug->lpOmapTo) && pomapMid[i].rvaTo == 0) {
                    //
                    // Keep on looping back until the beginning
                    //
                    i--;
                }
                return lpmdi->lpBaseOfDll + pomapMid[i].rvaTo;
            } else {
                return lpmdi->lpBaseOfDll + pomapMid->rvaTo;
            }
        }

        if (rva < pomapMid->rva) {
            pomapHigh = pomapMid;
            comap = (comap & 1) ? comapHalf : (comapHalf - 1);
        } else {
            pomapLow = pomapMid + 1;
            comap = comapHalf;
        }
    }

    assert(pomapLow == pomapHigh);

    //
    // If no exact match, pomapLow points to the next higher address
    //
    if (pomapLow == lpmdi->lpDebug->lpOmapTo) {
        //
        // This address was not found
        //
        return 0;
    }

    if (pomapLow[-1].rvaTo == 0) {
        return 0;
    }

    //
    // Return the new address plus the bias
    //
    return lpmdi->lpBaseOfDll + pomapLow[-1].rvaTo + (rva - pomapLow[-1].rva);
}


void
VerifyDebugDataLoaded(
    HPID hpid,
    HTID htid,
    LPMDI lpmdi
    )
{
    ADDR     addr;
    LPSGI    lpsgi;
    LPSGI    lpsgiMax;
    USHORT   usOvlMax = 0;
    HPRC     hprc = HprcFromHpid(hpid);
    LPPRC    lpprc;

    //
    //  Check to see if we have already loaded the debugger symbol data, if
    //  so then return now
    //

    if (lpmdi->lpDebug != LpDebugToBeLoaded) {
        return;
    }

    //
    //  Debugger symbol data not yet loaded -- do so now
    //

    lpmdi->lpDebug = (LPDEBUGDATA)SHGetDebugData( (HIND)(lpmdi->hemi) );

    // Get the GSN info table from the symbol handler
    if ( lpmdi->lpgsi = (LPGSI)SHLpGSNGetTable( (HIND)(lpmdi->hemi) ) ) {

        //
        //  If real mode, do some patch magic.
        //
        if ( lpmdi->fRealMode ) {

            int i;

            lpmdi->cobj   = lpmdi->lpgsi->csgMax+1;
            lpmdi->rgobjd = (OBJD *) MHRealloc(lpmdi->rgobjd,
                                      sizeof(OBJD)*lpmdi->cobj);
            memset(lpmdi->rgobjd, 0, sizeof(OBJD)*(lpmdi->cobj));

            lpsgi    = lpmdi->lpgsi->rgsgi;
            lpsgiMax = lpsgi + lpmdi->lpgsi->csgMax;

            for( i=0; lpsgi < lpsgiMax; lpsgi++, i++ ) {

                lpmdi->rgobjd[ i ].wSel = (WORD)(lpsgi->doffseg + lpmdi->StartingSegment);
                lpmdi->rgobjd[ i ].wPad = 1;
                lpmdi->rgobjd[ i ].cb   = (DWORD) -1;

                lpsgi->doffseg = 0;

            }
        }

        // Determine if child is overlaid and, if so, how many overlays
        lpsgi = lpmdi->lpgsi->rgsgi;
        lpsgiMax = lpsgi + lpmdi->lpgsi->csgMax;
        for( ; lpsgi < lpsgiMax; lpsgi++ ) {

            // iovl == 0xFF is reserved, it means no overlay specified.
            // we should ignore 0xFF in iovl.  Linker uses it
            // to (insert lots of hand-waving here) support COMDATS
            if ( lpsgi->iovl < 0xFF ) {                             // [02]
                usOvlMax = max( usOvlMax, lpsgi->iovl );
            }
        }
#ifndef TARGET32
        // Setup the overlay table
        if ( usOvlMax ) {
            lpmdi->lpsel = MHRealloc( lpmdi->lpsel, sizeof( WORD ) * usOvlMax + 1 );
            _fmemset( &lpmdi->lpsel [ 1 ], 0, sizeof( WORD ) * usOvlMax );
        }
#endif // !TARGET32
    }
    //
    // SHGetPublicAddr will use emi of out parameter to specify
    // which exe/dll to search
    //

    lpprc = (LPPRC)LLLock(hprc);
    if (lpprc->dmi.fNonLocalGoto) {

        emiAddr( addr ) = lpmdi->hemi;
        memset(&lpmdi->nlg, 0, sizeof(lpmdi->nlg));
        if ( SHGetPublicAddr ( &addr, "__NLG_Dispatch")) {
            FixupAddr( hpid, htid, &addr );
            lpmdi->nlg.addrNLGDispatch = addr;
            emiAddr( addr ) = lpmdi->hemi;
            if ( SHGetPublicAddr ( &addr, "__NLG_Destination")) {
                FixupAddr( hpid, htid, &addr );
                lpmdi->nlg.addrNLGDestination = addr;
                emiAddr( addr ) = lpmdi->hemi;
                lpmdi->fSendNLG = TRUE;
                lpmdi->nlg.fEnable = TRUE;
                lpmdi->nlg.hemi = lpmdi->hemi;
                if ( SHGetPublicAddr ( &addr, "__NLG_Return")) {
                    FixupAddr( hpid, htid, &addr );
                    lpmdi->nlg.addrNLGReturn = addr;
                    emiAddr( addr ) = lpmdi->hemi;
                }

                if ( SHGetPublicAddr ( &addr, "__NLG_Return2") || 1) {
                    FixupAddr( hpid, htid, &addr );
                    lpmdi->nlg.addrNLGReturn2 = addr;
                }
            }
        }
    }
    LLUnlock(hprc);

}


XOSD
GetSectionObjectsFromDM(
    HPID   hpid,
    LPMDI  lpmdi
    )
{
    XOSD xosd;

    assert( Is64PtrSE(lpmdi->lpBaseOfDll) );

    xosd = SendRequestX( dmfGetSections,
                         hpid,
                         0,
                         sizeof(lpmdi->lpBaseOfDll),
                         &lpmdi->lpBaseOfDll );

    assert( Is64PtrSE(lpmdi->lpBaseOfDll) );

    if (xosd != xosdNone) {
        return xosd;
    }

    lpmdi->cobj = *(LPDWORD)LpDmMsg->rgb;
    lpmdi->rgobjd = (LPOBJD) MHAlloc ( sizeof(OBJD) * lpmdi->cobj);
    if ( lpmdi->rgobjd == NULL ) {
        assert( "load cannot create rgobjd" && FALSE );
        return xosdOutOfMemory;
    }

    memcpy( lpmdi->rgobjd,
            LpDmMsg->rgb+sizeof(DWORD),
            sizeof(OBJD) * lpmdi->cobj );

    return xosdNone;
}
