//  ph.cxx
//
//  Copyright <C> 1989-94, Microsoft Corporation
//
//      [01] 03-dec-91 DavidGra
//          Correct the BSearch to return the nearest element
//          less than the key element when there is not an
//          exact match.
//
//      [00] 15-nov-91 DavidGra
//          Suppress hashing when the SSTR_NoHash bit it set.
//
//      11/1/94 BryanT
//          Merge in NT changes
//          Remove fChild arg from SHFindNameInGlobal (unused)

#include "shinc.hpp"
#pragma hdrstop

         ULONG  GSTBSearch (UOFFSET, LPALM, ULONG, ULONG, UOFFSET *);
__inline SYMPTR FindNameLinearInGlobs (LPEXG, SYMPTR, LPSSTR, PFNCMP, SHFLAG);
__inline SYMPTR FindNameLinearInPubs (LPEXG, SYMPTR, LPSSTR, PFNCMP, SHFLAG);
         SYMPTR GSTFindNameLinear (LPEXG, LPGST, SYMPTR, LPSSTR, PFNCMP, SHFLAG);
__inline SYMPTR FindNameHashedInGlobs(LPEXG, SYMPTR, LPSSTR, PFNCMP, SHFLAG, HMOD *);
__inline SYMPTR FindNameHashedInStatics(LPEXG, SYMPTR, LPSSTR, PFNCMP, SHFLAG, HMOD *);
__inline SYMPTR FindNameHashedInPubs (LPEXG, SYMPTR, LPSSTR, PFNCMP, SHFLAG, HMOD *);
         SYMPTR GSTFindNameHashed (LPEXG, LPGST, SYMPTR, LPSSTR, PFNCMP, SHFLAG, HMOD *);
__inline SYMPTR FindNearest (LPEXG, LPADDR, LPL);
         SYMPTR GSTFindNearest (LPEXG, LPGST, LPADDR, LPL);
         SYMPTR PsymFromRef (LPEXG, SYMPTR, HMOD *);
         BOOL   GSTCmpName (SYMPTR, LPSSTR, PFNCMP, SHFLAG);
         SYMPTR GSIFindNameLinear (LPEXG, GSI*, SYMPTR, LPSSTR, PFNCMP, SHFLAG);
         SYMPTR GSIFindNameHashed (LPEXG, GSI*, SYMPTR, LPSSTR, PFNCMP, SHFLAG, HMOD *);

__inline void lpStrToSz(LPSSTR lpsstr, char* sz)
{
    assert(lpsstr->cb < 256);
    memcpy(sz, lpsstr->lpName, lpsstr->cb);
    sz[lpsstr->cb] = 0;
}

//  PHExactCmp
//
//  Purpose: Compare two strings.
//
//  Input:
//      lsz     Zero terminated string for compare
//      hsym    -- not used --
//      lstz    Zero terminated length prefixed string for compare
//      fCase   Perform case sensitive compare?
//
//  Output: Return ZERO if strings are equal, else non-zero.

SHFLAG
PHExactCmp(
    HVOID hvStr,
    HVOID hvSym,
    LSZ lpb,
    SHFLAG fCase
    )
{
    LPSSTR lpsstr = (LPSSTR) hvStr;
    size_t  cb;
    SHFLAG  shf = TRUE;

    Unreferenced (hvSym);

    if (lpb) {
        cb = (size_t)*lpb;

        // if length is diff, they are not equal
        if (lpsstr && (size_t) lpsstr->cb == cb) {
            if (fCase) {
                shf = (SHFLAG) memcmp (lpb + 1, lpsstr->lpName, cb);
            } else {
                shf = (SHFLAG) _tcsnicmp(lpb + 1, (_TCHAR *)lpsstr->lpName, cb);
            }
        }
    }
    return shf;
}

//  PHGetNearestHsym
//
//  Purpose: To find a public symbol within a module
//
//  Input:
//      paddr   - The address of the symbol to find
//      hExe    - The exe to look in for the symbol in
//      phsym   - The symbol pointer
//
//  Output:
//      Returns How far (in bytes) the found symbol is from the address.
//      CV_MAXOFFSET is returned if non is found.

CV_uoff32_t
PHGetNearestHsym(
    LPADDR  lpaddr,
    HEXE    hexe,
    PHSYM   phsym
    )
{
    CV_uoff32_t dCur = CV_MAXOFFSET;
    LPEXE  lpexe;
    LPEXG  lpexg;

    *phsym = NULL;

    if (!VerifyHexe (hexe)) {
        return dCur;
    }

    SHWantSymbols(hexe);

    lpexe = (LPEXE) LLLock (hexe);

    if (lpexe->hexg != hexgNull) {
        lpexg = (LPEXG) LLLock (lpexe->hexg);
        *phsym = FindNearest (lpexg, lpaddr, (LPL) &dCur);
        LLUnlock(lpexe->hexg);
    }

    LLUnlock(hexe);

    return dCur;
}


BOOL
PHGetAddr(
    LPADDR paddr,
    LSZ lszName
    )
{
    HSYM    hsym = NULL;
    HEXE    hexe = 0;
    SSTR    sstr = {0};

    if (lszName == NULL || *lszName == '\0') {
        return FALSE;
    }

    sstr.lpName = (LPB) lszName;
    sstr.cb  = (BYTE) _tcslen (lszName);

    if (!(hsym = PHFindNameInPublics (NULL,
                                      hexe = SHGetNextExe (hexeNull),
                                      (LPSSTR) &sstr,
                                      TRUE,
                                      PHExactCmp)))
    {
        return FALSE;
    }

    SHAddrFromHsym (paddr, hsym);

    assert (hexe);
    emiAddr (*paddr) = (HEMI) hexe;

    return TRUE;
}

//  PHFindNameInPublics
//
//  Purpose: To find a public symbol
//
//  Input:
//      hsym    - This must be NULL! In the future this routine may
//                  be a find first find next behavior. For a first
//                  find use NULL, for a next find use the last symbol.
//      hExe    - The exe to search
//      hInfo   - The info packet to give to the comparison routine
//      fCase   - If TRUE do a case sensitive search.
//      pfnCm   - A pointer to the comparison function
//
//  Output:
//      Returns A public symbol or NULL on error
//
//  Exceptions: For now, the input hsym MUST BE NULL!

#define dwrd_toupper(dw) (dw & 0xDFDFDFDF)
#define byt_toupper(b) (b & 0xDF)

#define HASHFUNC(i,lpsstr,wModulo,lpul) \
    ((i==6)?DWordXor(lpsstr,wModulo,lpul):DWordXorShift(lpsstr,wModulo,lpul))

ushort
DWordXor(
    LPSSTR lpsstr,
    WORD wModulo,
    LPUL lpul
    )
{
    LPB   lpbName  = lpsstr->lpName;
    UNALIGNED ULONG * lpulName = (UNALIGNED ULONG*) lpbName;
    int   cb       = lpsstr->cb;
    int   cul;
    int   iul;
    ULONG ulSum    = 0;
    ULONG ulEnd    = 0;

    while (cb & 3) {
        ulEnd |= byt_toupper (lpbName [ cb - 1 ]);
        ulEnd <<= 8;
        cb -= 1;
    }

    cul = cb / 4;

    for (iul = 0; iul < cul; iul++) {
        ulSum ^= dwrd_toupper(lpulName[iul]);
    }
    ulSum ^= ulEnd;

    *lpul = ulSum;

    return (ushort) (ulSum % wModulo);
}

ushort
DWordXorShift(
    LPSSTR lpsstr,
    WORD wModulo,
    LPUL lpul
    )
{
    LPB   lpbName  = lpsstr->lpName;
    UNALIGNED ULONG* lpulName = (UNALIGNED ULONG*) lpbName;
    int   cb       = lpsstr->cb;
    int   cul;
    int   iul;
    ULONG ulSum    = 0;
    ULONG ulEnd    = 0;

    while (cb & 3) {
        ulEnd |= byt_toupper (lpbName [ cb - 1 ]);
        ulEnd <<= 8;
        cb -= 1;
    }

    cul = cb / 4;

    for (iul = 0; iul < cul; iul++) {
        ulSum ^= dwrd_toupper(lpulName[iul]);
        ulSum = _lrotl (ulSum, 4);
    }
    ulSum ^= ulEnd;

    *lpul = ulSum;

    return (ushort) (ulSum % wModulo);
}


HSYM
PHFindNameInPublics(
    HSYM    hsym,
    HEXE    hexe,
    LPSSTR  lpsstr,
    SHFLAG  fCase,
    PFNCMP  pfnCmp
    )
{
    SYMPTR psym = NULL;
    LPEXE  lpexe;
    LPEXG  lpexg;

    SHWantSymbols(hexe);

    if (hexe != hexeNull) {

        lpexe = (LPEXE) LLLock (hexe);

        if (lpexe->hexg == hexgNull) {
            psym = (SYMPTR)NULL;
        } else {
            lpexg  = (LPEXG) LLLock (lpexe->hexg);
            if (lpsstr->searchmask & SSTR_NoHash) {
                psym = FindNameLinearInPubs (lpexg, (SYMPTR)hsym, lpsstr, pfnCmp, fCase);
            } else {
                psym = FindNameHashedInPubs (lpexg, (SYMPTR)hsym, lpsstr, pfnCmp, fCase, NULL);
            }
            LLUnlock (lpexe->hexg);
        }

        LLUnlock (hexe);
    }

    return psym;
}


int
SHPublicNameToAddr(
    PADDR loc,
    PADDR pMpAddr,
    LSZ lszName,
    PFNCMP pfnCmp
    )
{
    CXT     cxt  = {0};
    HSYM    hsym = NULL;
    int     wRet = FALSE;
    ADDR    addr = *loc;
    SSTR    sstr = {0};

    sstr.lpName = (LPB) lszName;
    sstr.cb  = (BYTE) _tcslen (lszName);

    // Look for the name in the public symbols of that .EXE
    hsym = PHFindNameInPublics( NULL,
                                (HEXE) emiAddr(addr),
                                &sstr,
                                0,
                                pfnCmp? pfnCmp : PHExactCmp
                              );

    if (hsym) {
        SYMPTR psym = (SYMPTR) hsym;

        switch (psym->rectyp) {
            case S_PUB16:
                ADDRSEG16 (*pMpAddr);
                SetAddrSeg (pMpAddr, ((DATAPTR16)psym)->seg);
                SetAddrOff (pMpAddr, ((DATAPTR16)psym)->off);
                break;

            case S_PUB32:
                ADDRLIN32 (*pMpAddr);
                SetAddrSeg (pMpAddr, ((DATAPTR32)psym)->seg);
                SE_SetAddrOff (pMpAddr, ((DATAPTR32)psym)->off );
                break;
        }

        ADDR_IS_LI (*pMpAddr) = TRUE;
        emiAddr (*pMpAddr) = emiAddr (addr);
        wRet = TRUE;
    }

    return wRet;
}

ULONG
GSTBSearch(
    UOFFSET uoffKey,
    LPALM   lpalm,
    ULONG   ibBase,
    ULONG   cb,
    UOFFSET *lpoff
    )
{
    ULONG ibLow;
    ULONG ibHigh;
    ULONG ibMid = 0;
    ULONG cbWidth = sizeof (ULP);
    ULONG uoffFound = 0;

    LPULP lpulp = NULL;

    ibLow  = 0;
    ibHigh = cb;

    while (ibLow < ibHigh) {

        ibMid = (ibLow + ibHigh) / 2;

        uoffFound =
             ((LPULP) (LpvFromAlmLfo (lpalm, ibBase + ibMid * cbWidth)))->ulId;

        if (uoffFound > uoffKey) {
            ibHigh = ibMid;
        }
        else if (uoffKey > uoffFound) {
            ibLow = ibMid + 1;
        }
        else {
            break;
        }
    }

    if (uoffFound > uoffKey && ibMid != 0) {
        ibMid -= 1;
    }

    lpulp = (LPULP) LpvFromAlmLfo (lpalm, ibBase + ibMid * cbWidth);

    *lpoff = lpulp->ulId;
    return lpulp->ib;

}

__inline SYMPTR
FindNameLinearInGlobs(
    LPEXG  lpexg,
    SYMPTR psym,
    LPSSTR lpsstr,
    PFNCMP pfnCmp,
    SHFLAG fCase
    )
{
    if (lpexg->pgsiGlobs) {
        return GSIFindNameLinear(lpexg, lpexg->pgsiGlobs, psym, lpsstr, pfnCmp, fCase);
    } else {
        return GSTFindNameLinear(lpexg, &lpexg->gstGlobals, psym, lpsstr, pfnCmp, fCase);
    }
}

__inline SYMPTR
FindNameLinearInPubs(
    LPEXG  lpexg,
    SYMPTR psym,
    LPSSTR lpsstr,
    PFNCMP pfnCmp,
    SHFLAG fCase
    )
{
    SYMPTR p;
    if (lpexg->pgsiPubs) {
        p = GSIFindNameLinear(lpexg, lpexg->pgsiPubs, psym, lpsstr, pfnCmp, fCase);
    } else {
        p = GSTFindNameLinear(lpexg, &lpexg->gstPublics, psym, lpsstr, pfnCmp, fCase);
    }
    return p;
}

SYMPTR
GSTFindNameLinear(
    LPEXG  lpexg,
    LPGST  lpgst,
    SYMPTR psym,
    LPSSTR lpsstr,
    PFNCMP pfnCmp,
    SHFLAG fCase
    )
{
    SYMPTR  psymRet = NULL;

    if (lpgst->lpalm == NULL) {
        return (SYMPTR)NULL;
    }

    if (psym == NULL) {
        psym = (SYMPTR) LpvFromAlmLfo (lpgst->lpalm, 0);
    } else {
        psym = GetNextSym (psym, lpgst->lpalm);
    }

    while (psym != NULL) {
        SYMPTR psymT = psym;

        // NOTE: Don't find referenced procs/data in linear search

        if (psym->rectyp != S_PROCREF &&
            psym->rectyp != S_DATAREF &&
            GSTCmpName (psymT, lpsstr, pfnCmp, fCase))
        {
            psymRet = psymT;
            break;
        }

        psym = GetNextSym (psym, lpgst->lpalm);
    }

    return psym;
}

SYMPTR
GSIFindNameLinear(
    LPEXG  lpexg,
    GSI*   pgsi,
    SYMPTR psym,
    LPSSTR lpsstr,
    PFNCMP pfnCmp,
    SHFLAG fCase
    )
{
    SYMPTR  psymRet = NULL;

    if (pgsi == NULL) {
        return (SYMPTR)NULL;
    }

    psym = (SYMPTR) GSINextSym (pgsi, (PB) psym);
    while (psym != NULL) {
        SYMPTR psymT = psym;

        // NOTE: Don't find referenced procs/data in linear search

        if ((psym->rectyp != S_PROCREF) &&
            (psym->rectyp != S_LPROCREF) &&
            GSTCmpName (psymT, lpsstr, pfnCmp, fCase)
       ) {
            psymRet = psymT;
            break;
        }

        psym = (SYMPTR) GSINextSym (pgsi, (PB) psym);
    }

    return psym;
}

__inline SYMPTR
FindNameHashedInGlobs(
    LPEXG   lpexg,
    SYMPTR  psym,
    LPSSTR  lpsstr,
    PFNCMP  pfnCmp,
    SHFLAG  fCase,
    HMOD   *lphmod
    )
{
    if (lpexg->pgsiGlobs) {
        return GSIFindNameHashed (lpexg, lpexg->pgsiGlobs, psym, lpsstr, pfnCmp, fCase, lphmod);
    } else {
        return GSTFindNameHashed (lpexg, &lpexg->gstGlobals, psym, lpsstr, pfnCmp, fCase, lphmod);
    }
}

__inline SYMPTR
FindNameHashedInStatics(
    LPEXG   lpexg,
    SYMPTR  psym,
    LPSSTR  lpsstr,
    PFNCMP  pfnCmp,
    SHFLAG  fCase,
    HMOD   *lphmod
   )
{
    if (lpexg->pgsiGlobs) {
        // this assumes that the client has already completed an unsucessful search
        // thru the globals
       return (SYMPTR)NULL;
    } else {
       return GSTFindNameHashed(lpexg,
                                &lpexg->gstStatics,
                                psym,
                                lpsstr,
                                pfnCmp,
                                fCase,
                                lphmod
                               );
    }
}

__inline SYMPTR
FindNameHashedInPubs(
    LPEXG   lpexg,
    SYMPTR  psym,
    LPSSTR  lpsstr,
    PFNCMP  pfnCmp,
    SHFLAG  fCase,
    HMOD   *lphmod
    )
{
    if (lpexg->pgsiPubs) {
        return GSIFindNameHashed (lpexg, lpexg->pgsiPubs, psym, lpsstr, pfnCmp, fCase, lphmod);
    } else {
        return GSTFindNameHashed (lpexg, &lpexg->gstPublics, psym, lpsstr, pfnCmp, fCase, lphmod);
    }
}

SYMPTR
GSTFindNameHashed(
    LPEXG   lpexg,
    LPGST   lpgst,
    SYMPTR  psym,
    LPSSTR  lpsstr,
    PFNCMP  pfnCmp,
    SHFLAG  fCase,
    HMOD   *lphmod
    )
{
    LPSHT  lpsht   = &lpgst->shtName;

    if (lpsht->HashIndex == 0) {
        // No hash exists.  Punt to the linear code.
        return(GSTFindNameLinear(lpexg, lpgst, psym, lpsstr, pfnCmp, fCase));
    }

    ULONG  ulId    = 0;
    WORD   iib     = 0;
    SYMPTR psymRet = NULL;

    ULONG  ib      = 0;
    ULONG  culp    = 0;
    ULONG  iulp    = 0;

    BOOL   fNext   = psym != NULL;

    if ((lpsht->ccib == 0) ||(lpgst->lpalm == NULL))
        return (SYMPTR)NULL;

    iib  = HASHFUNC(lpsht->HashIndex, lpsstr, lpsht->ccib, &ulId);

    ib   = lpsht->rgib  [ iib ];
    culp = lpsht->rgcib [ iib ];

    // Loop through all the entries in this bucket

    for (iulp = 0; iulp < culp; iulp++) {

        LPULP lpulp = (LPULP) LpvFromAlmLfo (lpsht->lpalm,
                                    ib + (iulp * sizeof (ULP)));

        if (lpulp == NULL) {
            return NULL;
        }

        if (lpulp->ulId == ulId) {
            HMOD hmodT = hmodNull;

            // Checksums match, now check the symbols themselves

            SYMPTR psymT = (SYMPTR) LpvFromAlmLfo (lpgst->lpalm, lpulp->ib);

            if (psymT == NULL) {
                return NULL;
            }

            if (psymT->rectyp == S_PROCREF ||
                psymT->rectyp == S_DATAREF) {

                psymT = PsymFromRef (lpexg, psymT, &hmodT);

                // catch case where there are no module symbols... [rm]

                if (psymT == NULL) {
                    return NULL;
                }
            }

            if (fNext) {

                // We need to get back to the Current hsym before get can
                // get just one more.  Soon as we are there we know we can
                // get the next one.

                if (psymT == psym) {
                    fNext = FALSE;
                }
                continue;
            }

            if (GSTCmpName (psymT, lpsstr, pfnCmp, fCase)) {
                if (lphmod) {
                    *lphmod = hmodT;
                }
                psymRet = psymT;
                break;
            }
        }
    }

    return psymRet;
}

SYMPTR
GSIFindNameHashed(
    LPEXG   lpexg,
    GSI*    pgsi,
    SYMPTR  psym,
    LPSSTR  lpsstr,
    PFNCMP  pfnCmp,
    SHFLAG  fCase,
    HMOD   *lphmod
    )
{
    HMOD    hmodT = hmodNull;
    char    szBuf[256];
    SYMPTR  psymGS;
    SYMPTR psymRet;

    BOOL   fNext   = psym != NULL;

    if (!pgsi)
        return (SYMPTR)NULL;

    lpStrToSz(lpsstr, szBuf);
    psymGS = (SYMPTR) GSIHashSym(pgsi, szBuf, (PB) psym);

    for (; psymGS; psymGS = (SYMPTR) GSIHashSym(pgsi, szBuf, (PB) psymGS)) {
        assert(psymGS->rectyp != S_DATAREF);    // no datarefs
        if ((psymGS->rectyp == S_PROCREF) ||
            (psymGS->rectyp == S_LPROCREF)) {

            psymRet = PsymFromRef (lpexg, psymGS, &hmodT);

            // catch case where there are no module symbols... [rm]
            if (psymRet == NULL) {
                return NULL;
            }
        } else {
            psymRet = psymGS;
        }

        if (fNext) {

            // We need to get back to the Current hsym before get can
            // get just one more.  Soon as we are there we know we can
            // get the next one.

            if (psymRet == psym) {
                fNext = FALSE;
            }
          continue;
        }

        if (GSTCmpName (psymRet, lpsstr, pfnCmp, fCase)) {
            if (lphmod) {
                *lphmod = hmodT;
            }
            return psymRet;
        }
    }

    return NULL;
}


__inline SYMPTR
FindNearest(
    LPEXG lpexg,
    LPADDR lpaddr,
    LPL lpdb
    )
{
    SYMPTR  symptr;

    if (lpexg->pgsiPubs) {
#pragma message ("Need to search pgsiGlobals in FindNearest for NB10")
// BUGBUG need 64 bit support
        return (SYMPTR) GSINearestSym (lpexg->pgsiPubs,
                                       (USHORT)GetAddrSeg(*lpaddr),
                                       (UOFF32)GetAddrOff(*lpaddr),
                                       lpdb);
    } else {

        SYMPTR  symptrT;
        LONG    db;

        // Search first in globals
        symptr = GSTFindNearest(lpexg, &lpexg->gstGlobals, lpaddr, lpdb);

        // Search in publics
        symptrT = GSTFindNearest(lpexg, &lpexg->gstPublics, lpaddr, &db);

        // If there's an entry in the publics AND there was either
        // no match in the globals or the delta for the public is
        // less than the delta for the global, use the public
        if (symptrT && (!symptr || labs(db) < labs (*lpdb))) {
            *lpdb = db;
            symptr = symptrT;
        }
    }

    return symptr;
}

SYMPTR
GSTFindNearest(
    LPEXG lpexg,
    LPGST lpgst,
    LPADDR lpaddr,
    LPL lpdb
    )
{
    SYMPTR psym;
    ULONG  db    = CV_MAXOFFSET;
    LPSHT  lpsht = &lpgst->shtAddr;
    WORD   iseg;
    ULONG  ibSym = 0;
    OFFSET off   = GetAddrOff (*lpaddr);

    if (lpgst->lpalm == NULL) {
        return NULL;
    }

    // See if there's a hash index.  If not, do a linear search.
    if (lpsht->HashIndex == 0) {
        SYMPTR  psymClosest = NULL, psymT, psymEnd;
        HMOD    hmodT;
        psym    = (SYMPTR) LpvFromAlmLfo(lpgst->lpalm, ibSym);
        psymEnd = (SYMPTR) ((LPB) psym + lpgst->lpalm->cb);
        iseg    = (WORD)GetAddrSeg (*lpaddr);

        for (; psym < psymEnd; psym = NEXTSYM(SYMPTR, psym)) {
            if ((psym->rectyp == S_PROCREF) || (psym->rectyp == S_DATAREF)) {
                hmodT = hmodNull;
                psymT = PsymFromRef(lpexg, psym, &hmodT);
                if (psymT) {
                    psymT = psym;
                }
            } else {
                psymT = psym;
            }

            switch (psymT->rectyp) {
                case S_PUB16:
                    if ((iseg == ((DATAPTR16)psymT)->seg) &&
                        ((long) (off - ((DATAPTR16)psymT)->off) >= 0) &&
                        (db > (off - ((DATAPTR16)psymT)->off ))) {

                        // we are closer, so save this symbol and offset

                        db = (ULONG)(off - ((DATAPTR16)psymT)->off);
                        psymClosest = psymT;
                    }
                    break;

                case S_PUB32:
                    if ((iseg == ((DATAPTR32)psymT)->seg) &&
                        ((off - ((DATAPTR16)psymT)->off) >= 0) &&
                        (db > (off - ((DATAPTR32)psymT)->off))) {

                        // we are closer, so save this symbol and offset

                        db = (ULONG) (off - ((DATAPTR32)psymT)->off);
                        psymClosest = psymT;
                    }
                    break;
            }

            if (db == 0) {
                // Got an exact match.
                break;
            }
        }

        psym = psymClosest;

    } else {
        iseg = (WORD)GetAddrSeg (*lpaddr) - 1;
        if (iseg != 0xFFFF &&
            iseg < lpsht->ccib &&
            lpsht->rgcib [ iseg ] > 0)
        {
            UOFFSET offT = 0;

            ibSym = GSTBSearch (off,
                                lpsht->lpalm,
                                lpsht->rgib [ iseg ],
                                (WORD) lpsht->rgcib [ iseg ],
                                &offT);

// BUGBUG need 64 bits
            db = (DWORD)(off - offT);

            psym = (SYMPTR) LpvFromAlmLfo (lpgst->lpalm, ibSym);

            if (psym->rectyp == S_PROCREF || psym->rectyp == S_DATAREF) {
                HMOD    hmodT = hmodNull;

                psym = PsymFromRef(lpexg, psym, &hmodT);

                // Couldn't get the ref, set the db to what
                // would be returned in an error condition
                if (!psym) {
                    db = CV_MAXOFFSET;
                }
            }
        }
    }

    *lpdb = (LONG) db;
    return psym;
}

SYMPTR
PsymFromRef(
    LPEXG lpexg,
    SYMPTR psymRef,
    HMOD *lphmod
    )
{
    REFSYM  *lpref      = (REFSYM *) psymRef;
    LPB      lpbSymbols;
    LPMDS    lpmds;

    assert(lpref->imod <= lpexg->cMod);

    lpmds  = &lpexg->rgMod[lpref->imod];

    lpbSymbols = (LPB) GetSymbols (lpmds);

    if (lphmod) {
        *lphmod = (HMOD) lpmds;
    }

    // catch case where there are no module symbols...
    if (lpbSymbols == NULL) {
        return NULL;
    }

    // catch the case where the ibSym is beyond the symbol array
    if (lpref->ibSym > lpmds->cbSymbols) {
        return(NULL);
    }

    return (SYMPTR) (lpbSymbols + lpref->ibSym);
}

BOOL
GSTCmpName(
    SYMPTR psym,
    LPSSTR lpsstr,
    PFNCMP pfnCmp,
    SHFLAG fCase
    )
{
    LSZ lsz = NULL;

    // We are basically ignoring publics with set == 0

    if ((psym->rectyp == S_PUB16 && ((DATAPTR16) psym)->seg == 0) ||
        (psym->rectyp == S_PUB32 && ((DATAPTR32) psym)->seg == 0))
    {
        return FALSE;
    }

    lsz  = (LSZ) SHlszGetSymName (psym);

    if (lsz != NULL) {
        return (!(lpsstr->searchmask & SSTR_symboltype) ||
                 (psym->rectyp == lpsstr->symtype)) &&
                !(*pfnCmp) (lpsstr, psym, lsz, fCase);
    } else {
        return FALSE;
    }
}


//  SHFindNameInGlobal
//
//  Purpose:    To look for the name in the global symbol table.
//
//  Input:
//      hSym    - The starting symbol, if NULL, then the first symbol
//                  in the global symbol table is used. (NULL is find first).
//      pCXT    - The context to do the search.
//      lpsstr  - pointer to search parameters (passed to the compare routine)
//      fCaseSensitive - TRUE/FALSE on a case sensitive search
//      pfnCmp  - A pointer to the comparison routine
//      fChild  - TRUE if all child block are to be searched, FALSE if
//                  only the current block is to be searched.
//
//  Output:
//      pCXTOut - The context generated
//
//  Returns:
//      - A handle to the symbol found, NULL if not found
//
//  Notes:
//      If an hSym is specified, the hMod, hGrp and addr MUST be
//      valid and consistant with each other! If hSym is NULL only
//      the hMod must be valid.  The specification of an hSym
//      forces a search from the next symbol to the end of the
//      module scope.  Continues searches may only be done at
//      module scope.
//
//      If an hGrp is given it must be consistant with the hMod!

HSYM
SHFindNameInGlobal(
    HSYM   hsym,
    PCXT   pcxt,
    LPSSTR lpsstr,
    SHFLAG fCase,
    PFNCMP pfnCmp,
    PCXT   pcxtOut
    )
{
    HEXG    hexg;
    LPEXG   lpexg;
    HEXE    hexe;
    LPEXE   lpexe;
    HMOD    hmod   = hmodNull;
    LPGST   lpgst  = NULL;
    SYMPTR  psym   = NULL;

    *pcxtOut = *pcxt;
    pcxtOut->hProc = NULL;
    pcxtOut->hBlk = NULL;

    if (!pcxt->hMod) {
        return NULL;
    }

    hexe    = SHHexeFromHmod(pcxt->hGrp ? pcxt->hGrp : pcxt->hMod);
    lpexe   = (LPEXE) LLLock(hexe);
    hexg    = lpexe->hexg;
    lpexg   = (LPEXG) LLLock (hexg);
    LLUnlock(hexe);

    // Make sure these symbols are loaded.
//    SHWantSymbols(hexe);

    if (lpsstr->searchmask & SSTR_NoHash) {
        psym = FindNameLinearInGlobs (lpexg, (SYMPTR) hsym, lpsstr, pfnCmp, fCase);
    } else {
        psym = FindNameHashedInGlobs (lpexg, (SYMPTR) hsym, lpsstr, pfnCmp, fCase, &hmod);

        if (hmod != hmodNull) {
            pcxtOut->hMod = pcxt->hGrp = hmod;

            switch (psym->rectyp) {

                case S_LPROC32:
                case S_GPROC32:
                case S_LPROC16:
                case S_GPROC16:
                case S_LPROCMIPS:
                case S_GPROCMIPS:
                    emiAddr (pcxtOut->addr) = (HEMI) SHHexeFromHmod (hmod);
                    SHAddrFromHsym (&pcxtOut->addr, psym);
                    SHHPROCFrompCXT(pcxtOut) = psym;
                    break;

                default: {
                    LPMDS lpmds = (LPMDS) hmod;

                    SetAddrFromMod(lpmds, &pcxtOut->addr);
                    emiAddr (pcxtOut->addr) = (HEMI) SHHexeFromHmod (hmod);
                    ADDR_IS_LI (pcxtOut->addr) = TRUE;
                    break;
                }
            }
        }
    }

    LLUnlock (hexg);
    return psym;
}



HSYM
FindNameInStatics(
    HSYM   hsym,
    PCXT   pcxt,
    LPSSTR lpsstr,
    SHFLAG fCase,
    PFNCMP pfnCmp,
    PCXT   pcxtOut
    )
{
    HEXG    hexg   = hexgNull;
    HMOD    hmod   = hmodNull;
    LPEXG   lpexg  = NULL;
    LPGST   lpgst  = NULL;
    SYMPTR  psym   = NULL;

    *pcxtOut = *pcxt;
    pcxtOut->hProc = NULL;
    pcxtOut->hBlk = NULL;

    if (!pcxt->hGrp) {
        return NULL;
    }

    hexg = SHHexgFromHmod (pcxt->hGrp);
    lpexg  = (LPEXG) LLLock (hexg);

    psym = FindNameHashedInStatics(lpexg, (SYMPTR) hsym, lpsstr, pfnCmp, fCase, &hmod);

    if (hmod != hmodNull) {
        pcxtOut->hMod = pcxt->hGrp = hmod;

        switch (psym->rectyp) {
            case S_LPROC32:
            case S_GPROC32:
            case S_LPROC16:
            case S_GPROC16:
            case S_LPROCMIPS:
            case S_GPROCMIPS:
                emiAddr (pcxtOut->addr) = (HEMI) SHHexeFromHmod (hmod);
                SHAddrFromHsym (&pcxtOut->addr, psym);
                SHHPROCFrompCXT(pcxtOut) = psym;
                break;

            default: {
                LPMDS lpmds = (LPMDS) hmod;

                SetAddrFromMod(lpmds, &pcxtOut->addr);
                emiAddr (pcxtOut->addr) = (HEMI) SHHexeFromHmod (hmod);
                ADDR_IS_LI (pcxtOut->addr) = TRUE;
                break;
            }
        }
    }

    LLUnlock (hexg);
    return psym;
}

BOOL
VerifyHexe(
    HEXE hexe
    )
{
    HEXE hexeT = 0;
    static BOOL fFound = FALSE;
    static HEXE hexeSave = 0;

    if (hexe == hexeNull) {
        return FALSE;
    }

    if (hexe == hexeSave) {
        return fFound;
    }

    hexeSave = hexe;
    fFound = FALSE;

    while(!fFound && (hexeT = SHGetNextExe(hexeT))) {
        if (hexeT == hexe) {
            fFound = TRUE;
        }
    }

    return fFound;
}


//  SHFindNameInTypes
//
//  Purpose:    To look for the name in the UDT table.
//
//  Input:
//      pCXT    - The context to do the search.
//      lpsstr  - pointer to search parameters (passed to the compare routine)
//      fCaseSensitive - TRUE/FALSE on a case sensitive search
//      pfnCmp  - A pointer to the comparison routine
//
//  Output:
//      pCXTOut - The context generated
//
//  Returns:
//      - A handle to the type found, NULL if not found
//
//  Notes:
//      The hMod in the context must be valid.

HSYM
SHFindNameInTypes(
    PCXT pcxt,
    LPSSTR lpsstr,
    SHFLAG fCase,
    PFNCMP pfnCmp,
    PCXT pcxtOut
    )
{
    HEXG    hexg   = hexgNull;
    LPEXG   lpexg  = NULL;
    UDTPTR  psym   = NULL;
    char    szBuf[256];
    unsigned iHash;
    TI ti;

    *pcxtOut = *pcxt;
    pcxtOut->hProc = NULL;
    pcxtOut->hBlk = NULL;

    if (!pcxt->hMod || ( lpsstr->searchmask & SSTR_NoHash )) {
        return NULL;
    }

    hexg = SHHexgFromHmod ( pcxt->hMod );

    lpexg  = (LPEXG)LLLock ( hexg );
    if (!lpexg->ptpi) {
        LLUnlock ( hexg );
        return NULL;
    }

    if (!STABFindUDTSym(lpexg->pstabUDTSym, lpsstr, pfnCmp, fCase, &psym, &iHash)) {
        TPI* ptpi = 0;
#ifdef HOST32
#if CC_LAZYTYPES
        if ( ((LPMDS)pcxt->hMod)->pmod ) {    /* use the mod's type server */
            ModQueryTpi( ((LPMDS)pcxt->hMod)->pmod, &ptpi );
        }
#endif
#endif
        if ( ptpi == 0 ) ptpi = lpexg->ptpi;
        lpStrToSz(lpsstr, szBuf);
        assert(ptpi);
        if (!TypesQueryTiForUDT(ptpi, szBuf, fCase, &ti) ||
            !STABAddUDTSym(lpexg->pstabUDTSym, lpsstr, iHash, &psym)) {
            LLUnlock ( hexg );
            return NULL;
        }
        assert(psym);
        psym->typind = ti;
    }

    LLUnlock ( hexg );
    return (SYMPTR) psym;
}
