//  shsymbol
//
//  Copyright <C> 1989-94, Microsoft Corporation
//
//      [02] 31-dec-91 DavidGra
//              Add Symbol support for assembler.
//
//      [01] 02-dec-91 DavidGra
//              Pass symbol to compare function when SSTR_symboltype bit
//              is set only when its rectyp field is equal to the symtype
//              field in the SSTR structure.
//
//      [00] 15-nov-91 DavidGra
//              Suppress hashing when the SSTR_NoHash bit it set.
//
//      10-Nov-94 BryanT
//          Merge in NT changes.
//          Replace SPRINTF with sprintf.
//          Change SHAddrFromHsym to ignore S_BPREL16/S_BPREL32/S_REGREL32
//          since the address w/o the stack/register context is useless.
//          SHIsAddrInMod -> IsAddrInMod
//          Change SHSetCxtMod to save the previous value in CxtCache so we
//          can clear it when a process is removed.
//          Change SHGetCxtFromHmod to test for actual code in the mod before
//          setting the context.
//          Add SHGetCxtFromHexe (basically iterate over every hmod and call
//          SHGetCxtFromHmod until successful or out of hmods.
//          Add thunks support to SHGetNearestHsym.  Add default asserts in
//          case something changes.
//          Attempt to simplify the code in SHIsInProlog
//          Provide a shortcut out of SHGoToParent
//          Rewrite SHGethExeFromName to call SHGethExeFromExeName and
//          SHGethExeFromAltName.  Needed for multiple names in the kernel
//          debugger
//          Write SHGethExeFromModuleName
//          Flesh out SHCompareRE
//          Delete DOS compare routines used by the EE.
//          Delete SHIsEmiLoaded (not needed any longer).
//          Remove fChild arg from SHFindNameInContext (all users pass in FALSE)
//          Remove SHIsOmfLocked tests (hLocked was never set)
//
//      07-Jan-96 BryanT
//          Add in the MIPS changes Roger made for nested funcs *before* the parent proc.

#include "shinc.hpp"
#pragma hdrstop

LOCAL   VOID CheckHandles (PCXT);
LOCAL   PCXT SHSetBlksInCXT (PCXT);

//  SHAddrFromHsym - Given a symbol get the offset and seg and return
//
//  Purpose: This function will return the address of a symbol if it has an address.
//      If there is no address for the symbol then the function returns FALSE.
//
//  Input:
//      paddr  - Supplies the address structure to put the address in
//      hsym   - Supplies the handle to the symbol to get the address for.
//
//  Returns:
//      TRUE if the symbol has an address and FALSE if there is not
//      address associated with the symbol.

BOOL
SHAddrFromHsym (
    LPADDR  paddr,
    HSYM    hsym
    )
{
    SYMPTR psym = (SYMPTR) hsym;

    switch (psym->rectyp) {
        case S_GPROC16:
        case S_LPROC16:
        case S_BLOCK16:
            SE_SetAddrOff (paddr, ((PROCPTR16) psym)->off);
            SetAddrSeg (paddr, ((PROCPTR16) psym)->seg);
            ADDRSEG16  (*paddr);
            break;

        case S_LABEL16:
            SE_SetAddrOff (paddr, ((LABELPTR16) psym)->off);
            SetAddrSeg (paddr, ((LABELPTR16) psym)->seg);
            ADDRSEG16 (*paddr);
            break;

        case S_THUNK16:
            SE_SetAddrOff (paddr, ((THUNKPTR16) psym)->off);
            SetAddrSeg (paddr, ((THUNKPTR16) psym)->seg);
            ADDRSEG16 (*paddr);
            break;

        case S_WITH16:
            SE_SetAddrOff (paddr, ((WITHPTR16) psym)->off);
            SetAddrSeg (paddr, ((WITHPTR16) psym)->seg);
            ADDRSEG16 (*paddr);
            break;

        case S_LDATA16:
        case S_GDATA16:
        case S_PUB16:
            SE_SetAddrOff (paddr, ((DATAPTR16) psym)->off);
            SetAddrSeg (paddr, ((DATAPTR16) psym)->seg);
            ADDRSEG16 (*paddr);
            break;

        case S_GPROC32:
        case S_LPROC32:
        case S_BLOCK32:
            SE_SetAddrOff (paddr, ((PROCPTR32) psym)->off);
            SetAddrSeg (paddr, ((PROCPTR32) psym)->seg);
            ADDRLIN32 (*paddr);
            break;

        case S_GPROCMIPS:
        case S_LPROCMIPS:
            SE_SetAddrOff (paddr, (((PROCPTRMIPS) psym)->off));
            SetAddrSeg (paddr, ((PROCPTRMIPS) psym)->seg);
            ADDRLIN32 (*paddr);
            break;

        case S_GPROCIA64:
        case S_LPROCIA64:
            SE_SetAddrOff (paddr, (((PROCPTRIA64) psym)->off));
            SetAddrSeg (paddr, ((PROCPTRIA64) psym)->seg);
            ADDRLIN32 (*paddr);
            break;

        case S_LABEL32:
            SE_SetAddrOff (paddr, (((LABELPTR32) psym)->off));
            SetAddrSeg (paddr, ((LABELPTR32) psym)->seg);
            ADDRLIN32 (*paddr);
            break;

        case S_THUNK32:
            SE_SetAddrOff (paddr, (((THUNKPTR32) psym)->off));
            SetAddrSeg (paddr, ((THUNKPTR32) psym)->seg);
            ADDRLIN32 (*paddr);
            break;

        case S_WITH32:
            SE_SetAddrOff (paddr, (((WITHPTR32) psym)->off));
            SetAddrSeg (paddr, ((WITHPTR32) psym)->seg);
            ADDRLIN32 (*paddr);
            break;

        case S_PUB32:
        case S_LDATA32:
        case S_LTHREAD32:
        case S_GDATA32:
        case S_GTHREAD32:
            SE_SetAddrOff (paddr, ((DATAPTR32) psym)->off);
            SetAddrSeg (paddr, ((DATAPTR32) psym)->seg);
            ADDRLIN32 (*paddr);
            break;

        case S_BPREL16:
        case S_BPREL32:
        case S_REGREL32:  // UNDONE: Can we do better simply bailing here?
        default:
            return FALSE;
    }
    ADDR_IS_LI (*paddr) = TRUE;
    return TRUE;
}

//  SHGetNextMod
//
//  Purpose: To sequence through the modules. Only unique module indexes
//      are checked.
//
//  Input:
//      hMod    The last module, if NULL a starting point is picked
//
//  Returns:
//      The next module (hMod) in the module change or NULL if at end.

HMOD
SHGetNextMod (
    HEXE hexe,
    HMOD hmod
    )
{
    SHWantSymbols(hexe);
    return SHHmodGetNext (hexe, hmod);
}

PCXT
SetModule (
    PADDR paddr,
    PCXT pcxt,
    HEXE hexe,
    HMOD hmod,
    HGRP hgrp
    )
{
    memset(pcxt, 0, sizeof(CXT));
    *SHpADDRFrompCXT(pcxt) = *paddr;

    if (!emiAddr(*paddr)) {
        SHpADDRFrompCXT(pcxt)->emi = (HEMI) hexe;
    }
    SHHMODFrompCXT(pcxt) = hmod;
    SHHGRPFrompCXT(pcxt) = hgrp;

    return pcxt;
}

BOOL
IsAddrInMod (
    LPMDS lpmds,
    LPADDR lpaddr,
    ISECT* pisect,
    OFF* poff,
    CB* pcb
    )
{
    int isgc;

// BUGBUG DBI needs 64 bit api
    if (lpmds->pmod) {
        DBI* pdbi;
        Mod* pmodRet;
        if (!ModQueryDBI(lpmds->pmod, &pdbi) ||
            !DBIQueryModFromAddr(pdbi,
                                 (USHORT)GetAddrSeg (*lpaddr),
                                 (DWORD)GetAddrOff (*lpaddr),
                                 &pmodRet,
                                 pisect,
                                 poff,
                                 pcb))
            return FALSE;
        return (pmodRet == lpmds->pmod);
    } else {
        for (isgc = 0; isgc < (int) lpmds->csgc; isgc++) {
            if (lpmds->lpsgc[isgc].seg == GetAddrSeg (*lpaddr) &&
                lpmds->lpsgc[isgc].off <= GetAddrOff (*lpaddr) &&
                GetAddrOff (*lpaddr) < lpmds->lpsgc[isgc].off + lpmds->lpsgc[isgc].cb
            ) {
                if (pisect)
                    *pisect = (ISECT)lpmds->lpsgc[isgc].seg;
                if (poff)
                    *poff = lpmds->lpsgc[isgc].off;
                if (pcb)
                    *pcb = lpmds->lpsgc[isgc].cb;
                return TRUE;
            }
        }
    }

    return FALSE;
}

//  SHSetCxtMod
//
//  Purpose: To set the Mod and Group of a CXT
//
//  Input:
//      paddr   - The address to find
//
//  Output:
//      pcxt    - The point to the CXT to make.
//
//  Returns:
//      The pointer to the CXT, NULL if failure.
//
//  Notes:
//  The CXT must be all zero or be a valid CXT. Unpredictable results
//  (possible GP) if the CXT has random data in it. If the CXT is valid
//  the module pointed by it will be the first module searched.
//
//  There are no changes to the CXT if a module couldn't be found

PCXT
SHSetCxtMod (
    LPADDR  paddr,
    PCXT    pcxt
    )
{
    // BUGBUG: Check with WesW about this.  For some reason, he
    // removed the Save variables in April/1994.  I replaced the with
    // a Cache arg that is cleared like all the other Cache args...
    // Is this why he made the change?

    // If we're still in the same pds/exe/seg/range, it's the same module...

    if (CxtCache.hpds     == hpdsCur &&
        CxtCache.hexe     == (HEXE)emiAddr(*paddr) &&
        CxtCache.seg      == (WORD)GetAddrSeg (*paddr) &&
        CxtCache.uoffBase <= GetAddrOff (*paddr) &&
        CxtCache.uoffLim  >  GetAddrOff (*paddr))
    {
        return SetModule (paddr, pcxt, CxtCache.hexe, CxtCache.hmod, CxtCache.hgrp);
    } else if (GetAddrSeg(*paddr)) {

        HMOD hmod = hmodNull;
        HEXE hexe = hexeNull;

        while (hexe = SHGetNextExe (hexe)) {

            if (hexe == (HEXE) emiAddr (*paddr)) {
                LPEXE lpexe = (LPEXE) LLLock (hexe);
                LPEXG lpexg = (LPEXG) LLLock (lpexe->hexg);
                if (lpexg->ppdb) {
                    Mod* pmod;
                    CB cb;
                    BOOL fTmp;
                    assert(lpexg->pdbi);
// BUGBUG DBI needs 64 bit api
                    if (DBIQueryModFromAddr(lpexg->pdbi,
                                            (USHORT)GetAddrSeg(*paddr),
                                            (DWORD)GetAddrOff(*paddr),
                                            &pmod,
                                            &CxtCache.seg,
                                            (OFF *)&CxtCache.uoffBase,
                                            &cb))
                    {
                        CxtCache.uoffLim = CxtCache.uoffBase + cb;
                        CxtCache.hpds = hpdsCur;
                        fTmp  = ModGetPvClient(pmod, (void **)&CxtCache.hmod);
                        CxtCache.hgrp = CxtCache.hmod;
                        assert(fTmp);
                        CxtCache.hexe  = hexe;
                        return SetModule (paddr, pcxt, CxtCache.hexe, CxtCache.hmod, CxtCache.hgrp);
                    }
                } else {
                    LPSGD rgsgd = lpexg->lpsgd;

                    LLUnlock (lpexe->hexg);
                    LLUnlock (hexe);

                    if (rgsgd == NULL) {

                        return SetModule(paddr, pcxt, hexe, hmodNull, hmodNull);

                    } else if (GetAddrSeg (*paddr) <= lpexg->csgd) {

                        LPSGD lpsgd = &rgsgd [GetAddrSeg (*paddr) - 1];
                        WORD  isge = 0;

                        for (isge = 0; isge < lpsgd->csge; isge++) {
                            LPSGE lpsge = &lpsgd->lpsge [isge];

                            if (lpsge->sgc.seg == GetAddrSeg (*paddr) &&
                                lpsge->sgc.off <= GetAddrOff (*paddr) &&
                                GetAddrOff (*paddr) < lpsge->sgc.off + lpsge->sgc.cb
                            ) {
                                CxtCache.hpds      = hpdsCur;
                                CxtCache.hmod      = lpsge->hmod;
                                CxtCache.hgrp      = lpsge->hmod;
                                CxtCache.hexe      = hexe;
                                CxtCache.seg       = (WORD)GetAddrSeg (*paddr);
                                CxtCache.uoffBase  = lpsge->sgc.off;
                                CxtCache.uoffLim   = lpsge->sgc.off + lpsge->sgc.cb;

                                return SetModule (paddr, pcxt, CxtCache.hexe, CxtCache.hmod, CxtCache.hgrp);
                            }
                        }
                    }
                }
            }
        }
    }

    // In case we don't get a context match anywhere, at least init the cxt
    // structure (in case the caller forgets to test the return value).

    return SetModule (paddr, pcxt, hexeNull, hmodNull, hmodNull);
}


//  SHSetBlksInCXT
//
//  Purpose:  To update the CXT packet with Proc, and Blk information
//      based on pCXT->addr. It is possible to have a Blk record without
//      a Proc.
//
//      The Procs or Blocks will inclose the pCXT->addr. Also a
//      block will never inclose a Proc.
//
//      The updating of the ctxt will be effecient. If the packet is already
//      updated or partiallly updated, the search reduced or removed.
//
//  Input:
//      pcxt   - A pointer to a CXT with a valid HMOD, HGRP and addr
//
//  Output:
//      pcxt   - HPROC and HBLK are all updated.
//
//  Returns .....
//      pcxt on success or NULL on failure
//
//
//  Notes:  This is the core address to context routine! This particular
//    routine should only be used by other routines in this module
//    (i.e. remain static near!). The reason for this is so symbol
//    lookup can change with easy modification to this module and
//    not effecting other modules.

LOCAL PCXT
SHSetBlksInCXT (
    PCXT pcxt
    )
{
    SYMPTR  psym;
    SYMPTR  psymEnd;
    LPB     lpstart;
    LPMDS   lpmds;
    int     fGo;
    UOFFSET uoffCxt;
    UOFFSET uoffT;

    // determine if we can find anything
    if (!(pcxt->hMod && pcxt->hGrp)) {
        return NULL;
    }

    uoffCxt = GetAddrOff(pcxt->addr);

    // get the module limits

    lpmds = (LPMDS) pcxt->hGrp;
    if (GetSymbols(lpmds)) {
        lpstart = (LPB)(lpmds->symbols);
        psym = (SYMPTR) ((LPB) lpmds->symbols + sizeof (long));
        psymEnd = (SYMPTR) (((LPB) psym) + lpmds->cbSymbols - sizeof (long));
    } else {
        psym = psymEnd = NULL;
    }

    pcxt->hProc = NULL;
    pcxt->hBlk  = NULL;

    if (psym >= psymEnd) {
        return(pcxt);
    }

    // now search the symbol tables starting at psym for the correct block.

    while (psym < psymEnd) {
        switch (psym->rectyp) {
            // check to make sure this address starts before the address
            // of interest

            case S_LPROC16:
            case S_GPROC16:
                if (((PROCPTR16)psym)->seg != GetAddrSeg(pcxt->addr) ||
                    ((PROCPTR16)psym)->off > (CV_uoff16_t) uoffCxt   ||
                    ((PROCPTR16)psym)->off + ((PROCPTR16)psym)->len < (CV_uoff16_t) uoffCxt)
                {
                    psym = (SYMPTR)(lpstart + ((PROCPTR16)psym)->pEnd);
                } else {
                    pcxt->hProc = (HPROC) psym;
                    pcxt->hBlk = (HBLK)NULL;
                }

                break;

            case S_BLOCK16:
                if (((BLOCKPTR16)psym)->seg != GetAddrSeg(pcxt->addr) ||
                    ((BLOCKPTR16)psym)->off > (CV_uoff16_t) uoffCxt   ||
                    ((BLOCKPTR16)psym)->off + ((BLOCKPTR16)psym)->len < (CV_uoff16_t) uoffCxt)
                {
                    psym = (SYMPTR)(lpstart + ((BLOCKPTR16)psym)->pEnd);
                } else {
                    pcxt->hBlk = (HBLK)psym;
                }

                break;

            case S_WITH16:
                if (((WITHPTR16)psym)->seg == GetAddrSeg(pcxt->addr)) {
                    // Withs and Entry are only interesting to keep our nesting correct.

                    if (((WITHPTR16)psym)->off > (CV_uoff16_t) uoffCxt) {
                        // offsets are too high.  No sense continuing.
                        goto returnhere;
                    } else if (uoffCxt >= (UOFF32) ((WITHPTR16)psym)->off + ((WITHPTR16) psym)->len) {
                        psym = (SYMPTR)(lpstart + ((WITHPTR16)psym)->pEnd);
                    }
                }
                break;

            case S_LPROC32:
            case S_GPROC32:
                uoffT = ((PROCPTR32)psym)->off;

                if (((PROCPTR32)psym)->seg != GetAddrSeg(pcxt->addr) ||
                    uoffT > uoffCxt ||
                    (uoffT + ((PROCPTR32)psym)->len < uoffCxt))
                {
                    // Wrong segment/too high/too low.
                    psym = (SYMPTR)(lpstart + ((PROCPTR32)psym)->pEnd);
                } else {
                    // Right proc.
                    pcxt->hProc = (HPROC) psym;
                    pcxt->hBlk = (HBLK) NULL;
                }

                break;

            case S_LPROCIA64:
            case S_GPROCIA64:
                assert(!"need code for SetBlksInCxt for IA64");
                break;

            case S_LPROCMIPS:
            case S_GPROCMIPS:
                if (((PROCPTRMIPS)psym)->seg != GetAddrSeg(pcxt->addr) ||
                    ((PROCPTRMIPS)psym)->off > uoffCxt) {
                    // No match.  Look for the next symbol.
                    psym = (SYMPTR)(lpstart + ((PROCPTRMIPS)psym)->pEnd);
                } else {
                    uoffT = ((PROCPTRMIPS)psym)->off;

                    if (uoffCxt < (uoffT + ((PROCPTRMIPS)psym)->len)) {
                        // The offset is before the end of the parents range.

                        if (uoffCxt >= uoffT) {
                            // And it's after the start of the parent proc... Must be right.
                            psymEnd = (SYMPTR) (lpstart + ((PROCPTRMIPS)psym)->pEnd);

                            // MIPS can have several local functions in a module (for exception
                            //  filters, finally clauses, etc).  Make sure we're looking at
                            //  the right one.

                            if ((pcxt->hProc == NULL) &&
                                (uoffCxt > (uoffT + ((PROCPTRMIPS)psym)->DbgEnd)))
                            {
                                SYMPTR psymProc = NEXTSYM (SYMPTR, psym);
                                BOOL fFound = FALSE;
                                while (psymProc < psymEnd && !fFound) {
                                    if (psymProc->rectyp == S_LPROCMIPS) {
                                        uoffT = ((PROCPTRMIPS)psymProc)->off;
                                        if ((uoffCxt >= uoffT) &&
                                            (uoffCxt < (uoffT + ((PROCPTRMIPS)psymProc)->len))) {
                                            psym = psymProc;
                                            psymEnd = (SYMPTR) (lpstart + ((PROCPTRMIPS)psymProc)->pEnd);
                                            if (uoffCxt <= uoffT + ((PROCPTRMIPS)psymProc)->DbgEnd) {
                                                fFound = TRUE;
                                            }
                                        } else {
                                            psymProc = (SYMPTR)(lpstart + ((PROCPTRMIPS)psym)->pEnd);
                                        }
                                    }
                                    psymProc = NEXTSYM (SYMPTR, psymProc);
                                }
                            }
                            pcxt->hProc = (HPROC) psym;
                                                        pcxt->hBlk = (HBLK) NULL;
                        } else {
                            // Not within this proc.  Myst be before it.
                            // search for nested procs which have offsets < parent.

                            SYMPTR psymProc = ((SYMPTR) (((LPB) (psym)) + ((SYMPTR) (psym))->reclen + 2));
                            SYMPTR psymEndT = (SYMPTR) (lpstart + ((PROCPTRMIPS)psym)->pEnd);
                            BOOL fFound = FALSE;
                            while (psymProc < psymEndT) {
                                if (S_LPROCMIPS == psymProc->rectyp ) {
                                    unsigned long procLen = ((PROCPTRMIPS)psymProc)->len;
                                    uoffT = ((PROCPTRMIPS)psymProc)->off;
                                    if ((uoffCxt >= uoffT) &&
                                        (uoffCxt < (uoffT + procLen)))
                                    {
                                        psymEnd = (SYMPTR) (lpstart + ((PROCPTRMIPS)psym)->pEnd);
                                        psym = psymProc;
                                        pcxt->hProc = (HPROC) psym;
                                                                                pcxt->hBlk = (HBLK) NULL;
                                        fFound = FALSE;
                                        break;
                                    }
                                }
                                psymProc = ((SYMPTR) (((LPB) (psymProc)) + ((SYMPTR) (psymProc))->reclen + 2));     // 2?  Why 2?  BryanT 1/7/96
                            }
                            if (fFound) {
                                psym = (SYMPTR)(lpstart + ((PROCPTRMIPS)psym)->pEnd);
                            }
                        }
                    } else {
                        // The offset is after this symbol's range.. Keep looking.
                        psym = (SYMPTR)(lpstart + ((PROCPTRMIPS)psym)->pEnd);
                    }
                }

                break;

            case S_BLOCK32:
                if (((BLOCKPTR32)psym)->seg != GetAddrSeg(pcxt->addr) ||
                    ((BLOCKPTR32)psym)->off > uoffCxt ||
                    ((BLOCKPTR32)psym)->off + ((BLOCKPTR32)psym)->len < uoffCxt)
                {
                    psym = (SYMPTR)(lpstart + ((BLOCKPTR32)psym)->pEnd);
                } else {
                    pcxt->hBlk = (HBLK)psym;
                }

                break;

            case S_WITH32:
                if (((WITHPTR32)psym)->seg == GetAddrSeg(pcxt->addr)) {
                    if (((WITHPTR32)psym)->off > uoffCxt) {
                        goto returnhere;
                    } else if (uoffCxt >= ((WITHPTR32)psym)->off + ((WITHPTR32) psym)->len) {
                        psym = (SYMPTR)(lpstart + ((WITHPTR32)psym)->pEnd);
                    }
                }
                break;
        }
        // get the next psym address
        psym = NEXTSYM (SYMPTR, psym);
    }

returnhere:

    return pcxt;
}


//  SHSetCxt
//
//  Purpose: To set all field in a CXT to the represent the given address
//
//  Input:  pAddr   -The address to set the CXT to.
//
//  Output: pcxt    -A pointer to the CXT to fill.
//
//  Notes:  The CXT must be all zero or be a valid CXT. Unpredictable results
//          (possible GP) if the CXT has random data in it. If the CXT is valid
//          the module pointed by it will be the first module searched.
//
//  There are no changes to the CXT if a module couldn't be found

PCXT
SHSetCxt(
    LPADDR paddr,
    PCXT pcxt
    )
{
    // get the module part
    if (SHSetCxtMod(paddr, pcxt)) {
        SHSetBlksInCXT(pcxt);
        return(pcxt);
    }
    return NULL;
}


//  SHGetCxtFromHmod
//
//  Purpose: To make a CXT from only an hmod
//
//  Input:  hmod    - The module to make
//
//  Output: pCXT    - A pointer to a CXT to initialize to this hmod
//
//  Returns: A pointer to the CXT or NULL on error.

PCXT
SHGetCxtFromHmod (
    HMOD hmod,
    PCXT pcxt
    )
{
    LPMDS   lpmds = (LPMDS) hmod;

    if (!hmod) {
        return(NULL);
    }

    if (!lpmds->pmod && !lpmds->csgc) {
        return(NULL);
    }

    HEXE    hexe  = SHHexeFromHmod(hmod);

    // clear the CXT
    memset(pcxt, 0, sizeof(CXT));

    // set the module info
    pcxt->hGrp = pcxt->hMod = hmod;

    // put in the address
    SetAddrFromMod(lpmds, &pcxt->addr);
    emiAddr(pcxt->addr) = (HEMI) hexe;
    ADDR_IS_LI(pcxt->addr) = TRUE;
    // Set the fFlat and fOff32 bits based on the exe
    {
        HEXG hexg = ((LPEXE) LLLock(hexe))->hexg;
        LLUnlock (hexe);

        if (((LPEXG) LLLock (hexg))->fIsPE) {
            // REVIEW - billjoy - should we check machine type or something?
            ADDRLIN32 (pcxt->addr);
        }
        else {
            // REVIEW - billjoy - should we check machine type or something?
            //ADDR????
        }
        LLUnlock (hexg);
    }

    return(pcxt);
}


//  SHGetCxtFromHexe
//
//  Purpose: To make a CXT from only an hexe.  The first hmod with code
//          will be used.
//
//  Input:  hexe    - A handle to the exe in question
//
//  Output: pCXT    - A pointer to a CXT to initialize to this hmod
//
//  Returns: A pointer to the CXT or NULL on error
//
//  Notes:  This code depends on SHGetCxtFromHmod returning NULL if
//          lpmds->csgc is zero.

PCXT
SHGetCxtFromHexe (
    HEXE hexe,
    PCXT pcxt
    )
{
    HMOD  hmod;
    PCXT  pcxtRet = NULL;

    if (hexe) {
        hmod = (HMOD)NULL;
        while (!pcxtRet && (hmod = SHGetNextMod(hexe, hmod))) {
            pcxtRet = SHGetCxtFromHmod(hmod, pcxt);
        }
    }
    return pcxtRet;
}

//  SHGetNearestHsym
//
//  Purpose: To find the closest label/proc to the specified address is
//      found and put in pch. Both the symbol table and the
//      publics tables are searched.
//
//  Input:
//      pctxt   - a pointer to the context, address
//                and mdi must be filled in.
//      fIncludeData    - If true, symbol type local will be included
//                      in the closest symbol search.
//  Output:
//      pch     - The name is copied here.
//
//  Returns .....
//      The difference between the address and the symbol
//
//Notes:  If CV_MAXOFFSET is returned, there is no closest symbol
//      Also all symbols in the module are searched so only the
//      ctxt.addr and ctxt.mdi have meaning.

UOFF32
SHGetNearestHsym (
    LPADDR paddr,
    HMOD hmod,
    int mDataCode,
    PHSYM phSym
    )
{
    LBS         lbs;
    CV_uoff32_t doff    = (CV_uoff32_t)CV_MAXOFFSET;
    CV_uoff32_t doffNew = (CV_uoff32_t)CV_MAXOFFSET;
    SYMPTR      psym;

    // get the module to search
    *phSym = NULL;
    if (hmod) {
        // at some point we may wish to specify only a scope to search for
        // a label. So we may wish to initialize the lbs differently

        // get the Labels
        lbs.tagMod = hmod;
        lbs.addr   = *paddr;
        SHpSymlplLabLoc(&lbs);

        // check for closest data local, if requested
        if (((mDataCode & EEDATA) == EEDATA) && lbs.tagLoc) {
            psym = (SYMPTR) (LPB) lbs.tagLoc;
            switch (psym->rectyp) {
                case S_BPREL16:
                    doff = (DWORD)(GetAddrOff(lbs.addr) - ((BPRELPTR16)psym)->off);
                    break;

                case S_BPREL32:
                    doff = (DWORD)(GetAddrOff(lbs.addr) - ((BPRELPTR32)psym)->off);
                    break;

                case S_REGREL32:
                    doff = (DWORD)(GetAddrOff(lbs.addr) - ((LPREGREL32)psym)->off);
                    break;

                default:
                    assert(FALSE);
                    break;
            }
            *phSym = (HSYM) lbs.tagLoc;
        }

        // check for closest label
        if (((mDataCode & EECODE) == EECODE) && lbs.tagLab) {
            psym = (SYMPTR) (LPB) lbs.tagLab;
            switch (psym->rectyp) {
                case S_LABEL16:
                    if ((GetAddrOff(lbs.addr) - (UOFFSET)((LABELPTR16)psym)->off) <= (UOFFSET) doff) {
                        doff = (DWORD)(GetAddrOff(lbs.addr) - (UOFFSET)((LABELPTR16)psym)->off);
                        *phSym = (HSYM) lbs.tagLab;
                    }
                    break;

                case S_LABEL32:
                    if ((UOFFSET) (GetAddrOff(lbs.addr) - (UOFFSET)((LABELPTR32)psym)->off) <= doff) {
                        doff = (DWORD)(GetAddrOff(lbs.addr) - (UOFFSET)((LABELPTR32)psym)->off);
                        *phSym = (HSYM) lbs.tagLab;
                    }
                    break;

                default:
                    assert(FALSE);
                    break;
            }
        }

        //  If a thunk is closer
        if (((mDataCode & EECODE) == EECODE) && lbs.tagThunk) {
            psym = (SYMPTR) (LPB) lbs.tagThunk;
            switch (psym->rectyp) {
                case S_THUNK16:
                    if ((GetAddrOff(lbs.addr) - (UOFFSET)((THUNKPTR16)psym)->off) <= (UOFFSET)doff) {
                        doff = (DWORD)(GetAddrOff(lbs.addr) - (UOFFSET)((THUNKPTR16)psym)->off);
                        *phSym = (HSYM) lbs.tagThunk;
                    }
                    break;

                case S_THUNK32:
                    if ((GetAddrOff(lbs.addr) - (UOFFSET)((THUNKPTR32)psym)->off) <= doff) {
                        doff = (DWORD)(GetAddrOff(lbs.addr) - (UOFFSET)((THUNKPTR32)psym)->off);
                        *phSym = (HSYM) lbs.tagThunk;
                    }
                    break;

                default:
                    assert(FALSE);
                    break;
            }
        }

        // if the proc name is closer
        if (((mDataCode & EECODE) == EECODE) && lbs.tagProc) {
            psym = (SYMPTR) (LPB) lbs.tagProc;
            switch (psym->rectyp) {
                case S_LPROC16:
                case S_GPROC16:
                    if ((GetAddrOff(lbs.addr) - (UOFFSET)((PROCPTR16)psym)->off) <= (UOFFSET)doff) {
                        doff = (DWORD)(GetAddrOff (lbs.addr) - ((PROCPTR16)psym)->off);
                        *phSym = (HSYM) lbs.tagProc;
                    }
                    break;

                case S_LPROC32:
                case S_GPROC32:
                    if ((GetAddrOff(lbs.addr) - (UOFFSET)((PROCPTR32)psym)->off) <= doff) {
                        doff = (DWORD)(GetAddrOff(lbs.addr) - ((PROCPTR32)psym)->off);
                        *phSym = (HSYM) lbs.tagProc;
                    }
                    break;

                case S_LPROCIA64:
                case S_GPROCIA64:
                    if ((GetAddrOff(lbs.addr) - (UOFFSET)((PROCPTRIA64)psym)->off) <= doff) {
                        doff = (DWORD)(GetAddrOff(lbs.addr) - ((PROCPTRIA64)psym)->off);
                        *phSym = (HSYM) lbs.tagProc;
                    }
                    break;

                case S_LPROCMIPS:
                case S_GPROCMIPS:
                    if ((GetAddrOff(lbs.addr) - (UOFFSET)((PROCPTRMIPS)psym)->off) <= doff) {
                        doff = (DWORD)(GetAddrOff(lbs.addr) - ((PROCPTRMIPS)psym)->off);
                        *phSym = (HSYM) lbs.tagProc;
                    }
                    break;

                default:
                    assert(FALSE);
                    break;
            }
        }
    }
    return doff;
}

//  SHIsInProlog
//
//  Purpose: To determine if the addr is in prolog or epilog code of the proc
//
//  Input:
//      pCXT - The context describing the state.  The address here is Linker index
//              based
//
//  Returns:
//      TRUE if it is in prolog or epilog code

SHFLAG
SHIsInProlog (
    PCXT pcxt
    )
{
    SYMPTR pProc;
    UOFFSET CxtOffset, ProcOffset, ProcStart, ProcEnd, ProcLen;

    if (pcxt->hProc == NULL) {
        return FALSE;
    }

    if (!ADDR_IS_LI(*SHpADDRFrompCXT(pcxt))) {
        SYUnFixupAddr(SHpADDRFrompCXT(pcxt));
    }

    pProc = (SYMPTR) pcxt->hProc;

    CxtOffset = GetAddrOff(*SHpADDRFrompCXT(pcxt));

    // check to see if not within the proc
    switch (pProc->rectyp) {
        case S_LPROC16:
        case S_GPROC16:
            ProcOffset =((PROCPTR16)pProc)->off;
            ProcStart = ((PROCPTR16)pProc)->DbgStart;
            ProcEnd   = ((PROCPTR16)pProc)->DbgEnd;
            ProcLen   = ((PROCPTR16)pProc)->len;
            break;

        case S_LPROC32:
        case S_GPROC32:
            ProcOffset =((PROCPTR32)pProc)->off;
            ProcStart = ((PROCPTR32)pProc)->DbgStart;
            ProcEnd   = ((PROCPTR32)pProc)->DbgEnd;
            ProcLen   = ((PROCPTR32)pProc)->len;
            break;

        case S_LPROCIA64:
        case S_GPROCIA64:
            ProcOffset =((PROCPTRIA64)pProc)->off;
            ProcStart = ((PROCPTRIA64)pProc)->DbgStart;
            ProcEnd   = ((PROCPTRIA64)pProc)->DbgEnd;
            ProcLen   = ((PROCPTRIA64)pProc)->len;
            break;

        case S_LPROCMIPS:
        case S_GPROCMIPS:
            ProcOffset =((PROCPTRMIPS)pProc)->off;
            ProcStart = ((PROCPTRMIPS)pProc)->DbgStart;
            ProcEnd   = ((PROCPTRMIPS)pProc)->DbgEnd;
            ProcLen   = ((PROCPTRMIPS)pProc)->len;
            break;

        default:
            assert(FALSE);
            return(FALSE);
    }

    assert((ProcOffset <= CxtOffset) && ((ProcOffset + ProcLen) >= CxtOffset));

    return ( ((ProcOffset + ProcStart) > CxtOffset) ||      // Before the start
            (((ProcOffset + ProcEnd)   < CxtOffset) &&      //   or after the end?
             ((ProcOffset + ProcLen)   > CxtOffset)));
}

//  SHFindNameInContext
//
//  Purpose:  To look for the name at the scoping level specified by ctxt.
//      Only the specified level is searched, children may be searched
//      if fChild is set.
//
//      This routine will assume the desired scope in the following
//      way. If pcxt->hBlk != NULL, use hBlk as the starting scope.
//      If hBlk == NULL and pcxt->hProc != NULL use the proc scope.
//      If hBlk and hProc are both NULL and pcxt->hMod !=
//      NULL, use the module as the scope.
//
//  Input:
//      hSym    - The starting symbol, if NULL, then the first symbol
//          in the context is used. (NULL is find first).
//      pcxt    - The context to do the search.
//      lpsstr - pointer to the search parameters (passed to the compare routine)
//      fCaseSensitive - TRUE/FALSE on a case sensitive search
//      pfnCmp  - A pointer to the comparison routine
//      fChild  - TRUE if all child block are to be searched, FALSE if
//          only the current block is to be searched.
//
//  Output:
//      pcxtOut - The context generated
//
//  Returns:    A handle to the symbol found, NULL if not found
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
//
//      The level at which hSym is nested (cNest) is not passed in
//      to this function, so it must be derived.  Since this
//      could represent a significant speed hit, the level
//      of the last symbol processed is cached.  This should
//      take care of most cases and avoid the otherwise
//      necessary looping through all the previous symbols
//      in the module on each call.

HSYM
SHFindNameInContext (
    HSYM    hSym,
    PCXT    pcxt,
    LPSSTR  lpsstr,
    SHFLAG  fCase,
    PFNCMP  pfnCmp,
    PCXT    pcxtOut
    )
{
    LPMDS   lpmds;
    HMOD    hmod;
    HEXE    hexe;
    SYMPTR  lpsym;
    SYMPTR  lpEnd;
    LPB     lpstart;
    ULONG   cbSym;
    int     fSkip = FALSE;

    if (!ADDR_IS_LI (pcxt->addr)) {
        SYUnFixupAddr (&pcxt->addr);
    }

    memset(pcxtOut, 0, sizeof(CXT));
    if (!pcxt->hMod) {                   // we must always have a module
        return (HSYM) FindNameInStatics(hSym, pcxt, lpsstr, fCase, pfnCmp, pcxtOut);
    }

    hmod = pcxt->hGrp ? pcxt->hGrp : pcxt->hMod;    // Initialize the module
    lpmds = (LPMDS)hmod;

    pcxtOut->hMod   = pcxt->hMod;
    pcxtOut->hGrp   = pcxt->hGrp;

    hexe = SHHexeFromHmod (hmod);

#if 0
    // UNDONE: This needs to be fixed to handle X86 better.  >=3 means
    //      all mptix86 and mptm68k are flaged as ADDRSEG16.  Additionally,
    //      I changed SYProcessor to take a hpid (or NULL).  The code in windbg
    //      and the VC ide s/b update before enabling this code.  Finally, why
    //      is this interesting to do at all?  The only time it will stick is if
    //      we bail from no symbols or an existing hsym..

    lpexe = (LPEXE)LLLock(hexe);
    hpds = lpexe->hpds;
    lppds = (LPPDS)LLLock(hpds);

    if (SYProcessor (lppds->hpid) >= 3) {
        ADDRLIN32 (pcxtOut->addr);
    } else {
        ADDRSEG16 (pcxtOut->addr);
    }

    LLUnlock(hexe);
    LLUnlock(hpds);
    lpexe = NULL;
    lppds = NULL;
#else
    ADDRLIN32 (pcxtOut->addr);
#endif

    SetAddrFromMod(lpmds, &pcxtOut->addr);
    emiAddr (pcxtOut->addr) = (HEMI) hexe;
    ADDR_IS_LI(pcxtOut->addr) = TRUE;

    GetSymbols(lpmds);
    cbSym = lpmds->cbSymbols;

    if (cbSym == 0 || lpmds->symbols == NULL) {
        return NULL;
    }
    // Search the symbol table.

    lpstart = (LPB)(lpmds->symbols);
    lpsym = (SYMPTR) ((LPB) (lpmds->symbols) + sizeof(long));
    lpEnd = (SYMPTR) (((LPB) lpsym + cbSym) -  sizeof(long));

    // now find the start address. Always skip the current symbol because
    // we don't want to pick up the same name over and over again
    // if the user gives the start address
    if (hSym != NULL) {
        pcxtOut->hProc = (HPROC) pcxt->hProc;
        pcxtOut->hBlk = (HBLK) pcxt->hBlk;
        SetAddrOff (&pcxtOut->addr, GetAddrOff(pcxt->addr));
        SetAddrSeg (&pcxtOut->addr, GetAddrSeg(pcxt->addr));
        lpsym = (SYMPTR) hSym;

        switch (lpsym->rectyp) {
            case S_WITH16:
            case S_BLOCK16:
            case S_LPROC16:
            case S_GPROC16:

            case S_WITH32:
            case S_BLOCK32:
            case S_LPROC32:
            case S_GPROC32:
            case S_LPROCIA64:
            case S_GPROCIA64:

            case S_LPROCMIPS:
            case S_GPROCMIPS:
                lpsym = NEXTSYM(SYMPTR, (lpstart + ((PROCPTR)lpsym)->pEnd));
                break;

            default:
                lpsym = NEXTSYM(SYMPTR, lpsym);
        }
    } else if (pcxt->hBlk != NULL) {    // find the start address
        SYMPTR   lpbsp = (SYMPTR) pcxt->hBlk;

        pcxtOut->hProc = pcxt->hProc;
        pcxtOut->hBlk = pcxt->hBlk;
        switch (lpbsp->rectyp) {
            case S_BLOCK16:
                SE_SetAddrOff (&pcxtOut->addr, ((BLOCKPTR16)lpbsp)->off);
                SetAddrSeg (&pcxtOut->addr, (SEGMENT)((BLOCKPTR16)lpbsp)->seg);
                ADDRSEG16 (pcxtOut->addr);
                break;

            case S_BLOCK32:
                SE_SetAddrOff (&pcxtOut->addr, ((BLOCKPTR32)lpbsp)->off);
                SetAddrSeg (&pcxtOut->addr, (SEGMENT)((BLOCKPTR32)lpbsp)->seg);
                ADDRLIN32 (pcxtOut->addr);
                break;
        }
        lpsym = NEXTSYM(SYMPTR, lpbsp);
        lpEnd = (SYMPTR)(lpstart + ((BLOCKPTR16)lpbsp)->pEnd);
    } else if (pcxt->hProc != NULL) {

        // UNDONE: The NT code nuked this case (return NULL).

        SYMPTR lppsp = (SYMPTR) pcxt->hProc;

        switch (lppsp->rectyp) {
            case S_LPROC16:
            case S_GPROC16:
                SE_SetAddrOff (&pcxtOut->addr, ((PROCPTR16)lppsp)->off);
                SetAddrSeg (&pcxtOut->addr, (SEGMENT)((PROCPTR16)lppsp)->seg);
                ADDRSEG16 (pcxtOut->addr);
                break;

            case S_LPROC32:
            case S_GPROC32:
                SE_SetAddrOff (&pcxtOut->addr, ((PROCPTR32)lppsp)->off);
                SetAddrSeg (&pcxtOut->addr, (SEGMENT)((PROCPTR32)lppsp)->seg);
                ADDRLIN32 (pcxtOut->addr);
                break;

            case S_LPROCIA64:
            case S_GPROCIA64:
                SE_SetAddrOff (&pcxtOut->addr, ((PROCPTRIA64)lppsp)->off);
                SetAddrSeg (&pcxtOut->addr, (SEGMENT)((PROCPTRIA64)lppsp)->seg);
                ADDRLIN32 (pcxtOut->addr);
                break;

            case S_LPROCMIPS:
            case S_GPROCMIPS:
                SE_SetAddrOff (&pcxtOut->addr, ((PROCPTRMIPS)lppsp)->off);
                SetAddrSeg (&pcxtOut->addr, (SEGMENT)((PROCPTRMIPS)lppsp)->seg);
                ADDRLIN32 (pcxtOut->addr);
                break;
        }
        pcxtOut->hProc = pcxt->hProc;
        lpsym = NEXTSYM(SYMPTR, lppsp);
        lpEnd = (SYMPTR)(lpstart + ((PROCPTR16)lppsp)->pEnd);
    }

    while (lpsym <lpEnd && ((lpsym->rectyp != S_END))) {
        assert (lpsym->reclen != 0);

        switch (lpsym->rectyp) {
            case S_LABEL16:
                SE_SetAddrOff (&pcxtOut->addr, ((LABELPTR16)lpsym)->off);
                SetAddrSeg (&pcxtOut->addr, (SEGMENT)((LABELPTR16)lpsym)->seg);
                ADDRSEG16 (pcxtOut->addr);
                goto symname;

            case S_LPROC16:
            case S_GPROC16:
                pcxtOut->hBlk = NULL;
                pcxtOut->hProc = (HPROC) lpsym;
                SE_SetAddrOff (&pcxtOut->addr, ((PROCPTR16)lpsym)->off);
                SetAddrSeg (&pcxtOut->addr, (SEGMENT)((PROCPTR16)lpsym)->seg);
                ADDRSEG16 (pcxtOut->addr);
                goto entry16;

            case S_BLOCK16:
                pcxtOut->hBlk = (HBLK) lpsym;
                SE_SetAddrOff (&pcxtOut->addr, ((BLOCKPTR16)lpsym)->off);
                SetAddrSeg (&pcxtOut->addr, (SEGMENT)((BLOCKPTR16)lpsym)->seg);
                ADDRSEG16 (pcxtOut->addr);
                goto entry16;

            case S_THUNK16:
            case S_WITH16:
                ADDRSEG16 (pcxtOut->addr);

            entry16:
                fSkip = TRUE;

                // fall thru and process the symbol

            case S_BPREL16:
            case S_GDATA16:
            case S_LDATA16:
                ADDRSEG16 (pcxtOut->addr);
                goto symname;

            case S_LABEL32:
                SE_SetAddrOff (&pcxtOut->addr, ((LABELPTR32)lpsym)->off);
                SetAddrSeg (&pcxtOut->addr, (SEGMENT)((LABELPTR32)lpsym)->seg);
                ADDRLIN32 (pcxtOut->addr);
                goto symname;

            case S_LPROC32:
            case S_GPROC32:
                pcxtOut->hBlk = NULL;
                pcxtOut->hProc = (HPROC) lpsym;
                SE_SetAddrOff (&pcxtOut->addr, ((PROCPTR32)lpsym)->off);
                SetAddrSeg (&pcxtOut->addr, (SEGMENT)((PROCPTR32)lpsym)->seg);
                ADDRLIN32 (pcxtOut->addr);
                goto entry32;

            case S_LPROCIA64:
            case S_GPROCIA64:
                pcxtOut->hBlk = NULL;
                pcxtOut->hProc = (HPROC) lpsym;
                SE_SetAddrOff (&pcxtOut->addr, ((PROCPTRIA64)lpsym)->off);
                SetAddrSeg (&pcxtOut->addr, (SEGMENT)((PROCPTRIA64)lpsym)->seg);
                ADDRLIN32 (pcxtOut->addr);
                goto entry32;

            case S_LPROCMIPS:
            case S_GPROCMIPS:
                pcxtOut->hBlk = NULL;
                pcxtOut->hProc = (HPROC) lpsym;
                SE_SetAddrOff (&pcxtOut->addr, ((PROCPTRMIPS)lpsym)->off);
                SetAddrSeg (&pcxtOut->addr, (SEGMENT)((PROCPTRMIPS)lpsym)->seg);
                ADDRLIN32 (pcxtOut->addr);
                goto entry32;

            case S_BLOCK32:
                pcxtOut->hBlk = (HBLK) lpsym;
                SE_SetAddrOff (&pcxtOut->addr, ((BLOCKPTR32)lpsym)->off);
                SetAddrSeg (&pcxtOut->addr, (SEGMENT)((BLOCKPTR32)lpsym)->seg);
                ADDRLIN32 (pcxtOut->addr);
                goto entry32;
                    // fall thru to the entry case

            case S_THUNK32:
            case S_WITH32:
                ADDRLIN32 (pcxtOut->addr);

            entry32:
                fSkip = TRUE;

                // fall thru and process the symbol

            case S_BPREL32:
            case S_REGREL32:
            case S_GDATA32:
            case S_LDATA32:
            case S_GTHREAD32:
            case S_LTHREAD32:
                ADDRLIN32 (pcxtOut->addr);
                goto symname;

            case S_REGISTER:
            case S_CONSTANT:
            case S_UDT:
            case S_COBOLUDT:
            case S_COMPILE:
symname:
                if ((!(lpsstr->searchmask & SSTR_symboltype) ||
                      (lpsym->rectyp == lpsstr->symtype)
                    ) &&
                    !(*pfnCmp)(lpsstr, lpsym, (LSZ) SHlszGetSymName (lpsym), fCase)
                   )
                {
                    // save the sym pointer
                    lpsym =  (SYMPTR) lpsym;
                    CheckHandles (pcxtOut);
                    return lpsym;
                }

                // up the scoping level
                if (fSkip) {
                    // Make sure the compiler did the right thing
                    assert(((PROCPTR16)lpsym)->pEnd != 0);
                    lpsym = (SYMPTR)(lpstart + ((PROCPTR16)lpsym)->pEnd);
                    fSkip = FALSE;
                }
                break;
        }
        lpsym = NEXTSYM (SYMPTR, lpsym);
    }
    return NULL;
}


LOCAL VOID
CheckHandles (
    PCXT pcxt
    )
{
    SYMPTR  psym;

    // check and restore all proc and blk handles
    if (pcxt->hProc != NULL) {
        psym = (SYMPTR) pcxt->hProc;
        switch (psym->rectyp) {
            case S_LPROC16:
            case S_GPROC16:
                if ((GetAddrOff (pcxt->addr) < ((PROCPTR16)psym)->off) ||
                    GetAddrOff (pcxt->addr) >= (UOFF32) (((PROCPTR16)psym)->len +
                                                         ((PROCPTR16)psym)->off))
                {
                    pcxt->hProc = NULL;
                }
                break;

            case S_LPROC32:
            case S_GPROC32:
                if ((GetAddrOff (pcxt->addr) <  ((PROCPTR32)psym)->off) ||
                     GetAddrOff (pcxt->addr) >= (((PROCPTR32)psym)->len +
                                                 ((PROCPTR32)psym)->off))
                {
                    pcxt->hProc = NULL;
                }
                break;

            case S_LPROCIA64:
            case S_GPROCIA64:
                if ((GetAddrOff (pcxt->addr) <  ((PROCPTRIA64)psym)->off) ||
                     GetAddrOff (pcxt->addr) >= (((PROCPTRIA64)psym)->len +
                                                 ((PROCPTRIA64)psym)->off))
                {
                    pcxt->hProc = NULL;
                }
                break;

            case S_LPROCMIPS:
            case S_GPROCMIPS:
                if ((GetAddrOff (pcxt->addr) <  ((PROCPTRMIPS)psym)->off) ||
                     GetAddrOff (pcxt->addr) >= (((PROCPTRMIPS)psym)->len +
                                                 ((PROCPTRMIPS)psym)->off))
                {
                    pcxt->hProc = NULL;
                }
                break;
        }
    }
    if (pcxt->hBlk != NULL) {
        psym = (SYMPTR) pcxt->hBlk;
        switch (psym->rectyp) {
            case S_BLOCK16:
                if ((GetAddrOff (pcxt->addr) < ((BLOCKPTR16)psym)->off) ||
                    GetAddrOff (pcxt->addr) >= (UOFF32)(((BLOCKPTR16)psym)->len +
                                                        ((BLOCKPTR16)psym)->off))
                {
                    pcxt->hBlk = NULL;
                }
                break;

            case S_BLOCK32:
                if ((GetAddrOff (pcxt->addr) < ((BLOCKPTR32)psym)->off) ||
                     GetAddrOff (pcxt->addr) >= (((BLOCKPTR32)psym)->len +
                                                 ((BLOCKPTR32)psym)->off))
                {
                    pcxt->hBlk = NULL;
                }
                break;
        }
    }

    // now fill in the proper group
    // because there is not (currently) a unique emi within a
    // module, use the emi set in addr
    pcxt->hGrp = pcxt->hMod;
}


//  SHpSymctxtParent
//
//  Purpose: To return a pointer to the parent block of the current blk or proc.
//     The CXT is updated to the parent context. This may be a new block
//     Proc or module.
//
//  Input:
//      pcxt   - A pointer to the child CXT.
//
//  Output:
//      pcxtOut- an updated CXT to the parent.
//
//  Returns .....
//      - a Symbol point to the first record within the parent, this
//        may be pcxt->hBlk, hProc, or
//        pcxt->hMod->symbols + sizeof (long) or NULL if no parent.

HSYM
SHGoToParent (
    PCXT pcxt,
    PCXT pcxtOut
    )
{

    SYMPTR  lpsym = NULL;
    LPMDS   lpmds;
    SYMPTR  lpsymT;
    HSYM    hsym;
    LPB     lpstart;

    if (!pcxt->hMod) {
        return NULL;
    }

    lpmds = (LPMDS)pcxt->hMod;
    lpstart = (LPB) (lpmds->symbols);
    lpsymT  = (SYMPTR) ((LPB) lpmds->symbols + sizeof(long));

    *pcxtOut = *pcxt;
    // if the block is present, go to his parent
    if (pcxt->hBlk != NULL) {
        // If we are the parent, No further to go.
        assert(pcxt->hBlk != pcxt->hProc);
        // get lpsym upto the parent
        lpsym = (SYMPTR) pcxt->hBlk;
        lpsym = (SYMPTR)(lpstart + ((BLOCKPTR16)lpsym)->pParent);
        pcxtOut->hBlk = NULL;
    } else if (pcxt->hProc != NULL) {
        // otherwise check the proc's parent, and go to his parent
        lpsym = (SYMPTR) pcxt->hProc;       // get lpsym upto the parent
        lpsym = (SYMPTR)(lpstart + (((PROCPTR16)lpsym)->pParent));
        pcxtOut->hProc = NULL;
    } else {
        // otherwise there is no parent
        return NULL;
    }

    // if there is a parent, set the cxt packet.
    if (lpsym != (SYMPTR) lpstart) {
        switch(lpsym->rectyp) {
            case S_LPROC16:
            case S_GPROC16:
            // case S_ENTRY:
            case S_LPROC32:
            case S_GPROC32:
            case S_LPROCIA64:
            case S_GPROCIA64:
            case S_LPROCMIPS:
            case S_GPROCMIPS:
                // UNDONE: The NT code was changed to set pcxtOut->hBlk here... Why?
                pcxtOut->hProc = (HPROC) lpsym;
                break;

            case S_BLOCK16:
            case S_WITH16:
            case S_BLOCK32:
            case S_WITH32:
                pcxtOut->hBlk = (HBLK) lpsym;
                break;

            default:
                return NULL;
        }
        return lpsym;
    } else {
        // return the module as the parent
        hsym = (HSYM) lpsymT;
        return hsym;
    }
}

//  SHFindSLink32
//
//  Purpose: To return a pointer to the SLINK32 for this proc
//
//  Input:
//      pcxt   - A pointer to the child CXT.
//
//
//  Returns .....
//      - a Symbol point to the SLINK32 record

HSYM
SHFindSLink32 (
    PCXT pcxt
    )
{
    SYMPTR  lpsym = NULL;
    LPMDS   lpmds;
    SYMPTR lpsymT;

    if (!pcxt->hMod) {
        return NULL;
    }

    lpmds = (LPMDS) pcxt->hMod;
    lpsymT = (SYMPTR) ((LPB)lpmds->symbols + sizeof(long));

    if (pcxt->hProc != NULL) {
        lpsym = (SYMPTR) pcxt->hProc;
    } else {
        return NULL;            // otherwise there is no SLINK32
    }

    lpsym = NEXTSYM(SYMPTR, lpsym);

    for (; lpsym != NULL && lpsym->rectyp != S_SLINK32;) {
        switch (lpsym->rectyp) {
            case S_LPROC16:
            case S_GPROC16:
            case S_LPROC32:
            case S_GPROC32:
            // case S_ENTRY:
            case S_LPROCIA64:
            case S_GPROCIA64:
            case S_LPROCMIPS:
            case S_GPROCMIPS:
            case S_BLOCK16:
            case S_WITH16:
            case S_BLOCK32:
            case S_WITH32:
            case S_END:
                lpsym = NULL;
                break;

            case S_SLINK32:
                break;

            default:
                lpsym = NEXTSYM(SYMPTR, lpsym);
                break;
        }
    }
    return lpsym;
}

//  SHHsymFromPcxt
//
//  Purpose: To get the inner most hSym given a context
//
//  Input:
//      pcxt    - A pointer to a valid CXT.
//
//  Returns:
//      HSYM of the first symbol, or NULL on Error
//
//  Notes: Used for procedure parameter walking

HSYM
SHHsymFromPcxt(
    PCXT pcxt
    )
{
    HSYM  hsym = NULL;
    LPMDS lpmds;

    if (pcxt->hMod) {
        if (pcxt->hBlk) {
            hsym = pcxt->hBlk;
        } else if (pcxt->hProc) {
            hsym = pcxt->hProc;
        } else {
            SYMPTR  lpsymT;

            // get the first symbol
            lpmds = (LPMDS) pcxt->hMod;
            lpsymT = (SYMPTR) ((LPB) GetSymbols (lpmds) + sizeof(long));
            hsym = lpsymT;
        }
    }
    return hsym;
}

//  SHNextHsym
//
//  Purpose: To get the next symbol in the table
//
//  Input:
//      hMod -A handle to the module containing the current hSym
//      hSym -The current hSym
//
//  Returns:
//      The next hSym, or NULL if no more.

HSYM
SHNextHsym (
    HMOD hmod,
    HSYM hSym
    )
{
    SYMPTR  lpsym;
    SYMPTR  lpsymStart;
    ULONG   cbSym;
    LPMDS   lpmds;
    HSYM    hsymRet = (HSYM)NULL;
    SYMPTR  lpsymT;

    if (hmod) {
        // only if the symbol is valid
        // get module info
        lpmds = (LPMDS) hmod;
        lpsymT = (SYMPTR) ((LPB) GetSymbols (lpmds) + sizeof(long));
        lpsymStart = (SYMPTR) lpsymT;
        cbSym = lpmds->cbSymbols;

        // give him the first symbol record

        if (hSym == NULL) {
            // if the current handle to symbol is null, return the first
            // symbol.  This is actually an error condition since we don't
            // have an hSym to get the next from
            hsymRet = (HSYM)lpsymStart;
        } else {
            // get info about the sym, and then skip it

            lpsym = (SYMPTR) hSym;
            lpsym = NEXTSYM(SYMPTR, lpsym);

            // check to see if still in symbol range

            lpsymStart = (SYMPTR) lpsymStart;
            if (lpsymStart <= lpsym &&
                lpsym < (SYMPTR) (((LPB) lpsymStart) + cbSym)) {
                hsymRet = (HSYM) lpsym;
            }
        }
    }
    return hsymRet;
}


//  SHIsAddrInCxt
//
//  Purpose: To verify weather the address is within the context
//
//  Input:
//      pCXT    - The context to check against
//      pADDR   - The address in question
//
//  Returns:
//      TRUE if within context, FALSE otherwise.

SHFLAG
SHIsAddrInCxt (
    PCXT pcxt,
    LPADDR paddr
    )
{
    HMOD        hmod;
    LPMDS       lpmds;
    SYMPTR      psym;
    SHFLAG      shf = (SHFLAG)FALSE;

    if ((pcxt != NULL) && (pcxt->hMod != 0)) {

        // get the module
        if (pcxt->hGrp != 0) {
            hmod = pcxt->hGrp;
        } else {
            hmod = pcxt->hMod;
            pcxt->hGrp = hmod;
        }
        lpmds = (LPMDS) hmod;

        // The return value is true if these three conditions are all true:
        //  1. The address is in the same executable as the context
        //  2. The address is in the same module as the context
        //  3. Any of the following are true:
        //     a. There is no block or proc so the address offset
        //        can be anywhere
        //     b. The address is in the offset range of the block of
        //        the context
        //     c. The addr is in the offset range of the procedure of
        //        the context

        if (emiAddr (*paddr) != 0                       &&
            emiAddr (*paddr) != (HEMI) hpidCurr         &&
            emiAddr (*paddr) == (HEMI) SHHexeFromHmod (hmod)
       ) {
            // condition 1 is true

            if (IsAddrInMod (lpmds, paddr, NULL, NULL, NULL)) {
                // condition 2 is true

                if (pcxt->hProc == NULL && pcxt->hBlk == NULL) {
                    // condition 3a is true
                    shf = TRUE;
                }

                if (!shf && (psym = (SYMPTR) pcxt->hBlk) != NULL) {
                    // we have not passed test 3a and the block
                    // symbol handle is not null
                    switch (psym->rectyp) {

                        case S_BLOCK16:
                            if ((((UOFFSET)((BLOCKPTR16)psym)->off) <= GetAddrOff (*paddr))  &&
                              (GetAddrOff (*paddr) < (UOFF32) (((BLOCKPTR16)psym)->off +
                                                               ((BLOCKPTR16)psym)->len)))
                            {
                                // case 3b is true for a 16 bit block symbol
                                shf = TRUE;
                            }
                            break;

                        case S_BLOCK32:
                            if ((((BLOCKPTR32) psym)->off <= GetAddrOff (*paddr)) &&
                                (GetAddrOff (*paddr) < (UOFFSET) (((BLOCKPTR32)psym)->off +
                                                                  ((BLOCKPTR32)psym)->len)))
                            {
                                // case 3b is true for a 32 bit block symbol
                                shf = TRUE;
                            }
                            break;
                    }
                }
                if ((shf == FALSE) && ((psym = (SYMPTR) pcxt->hProc) != NULL)) {
                    // we have not passed tests 3a or 3b and the proc
                    // symbol handle is not null
                    switch (psym->rectyp) {

                        case S_LPROC16:
                        case S_GPROC16:
                            if ((((PROCPTR16)psym)->off <= (CV_uoff16_t) GetAddrOff(*paddr)) &&
                                (GetAddrOff (*paddr) < (UOFF32) (((PROCPTR16) psym)->off +
                                                                 ((PROCPTR16) psym)->len)))
                            {
                                // case 3c is true for a 16 bit proc symbol
                                shf = TRUE;
                            }
                            break;

                        case S_LPROC32:
                        case S_GPROC32:
                            if ((((PROCPTR32) psym)->off <= GetAddrOff (*paddr)) &&
                                (GetAddrOff (*paddr) < (UOFFSET) (((PROCPTR32)psym)->off +
                                                                  ((PROCPTR32)psym)->len)))
                            {
                                // case 3b is true for a 32 bit proc symbol
                                shf = TRUE;
                            }
                            break;

                        case S_LPROCIA64:
                        case S_GPROCIA64:
                            if ((((PROCPTRIA64) psym)->off <= GetAddrOff (*paddr)) &&
                                (GetAddrOff (*paddr) < (UOFFSET) (((PROCPTRIA64)psym)->off +
                                                                  ((PROCPTRIA64)psym)->len)))
                            {
                                // case 3b is true for a 32 bit proc symbol
                                shf = TRUE;
                            }
                            break;

                        case S_LPROCMIPS:
                        case S_GPROCMIPS:
                            if ((((PROCPTRMIPS) psym)->off <= GetAddrOff (*paddr)) &&
                                (GetAddrOff (*paddr) < (UOFFSET) (((PROCPTRMIPS)psym)->off +
                                                                  ((PROCPTRMIPS)psym)->len)))
                            {
                                // case 3b is true for a 32 bit proc symbol
                                shf = TRUE;
                            }
                            break;
                    }
                }
            }
        }
    }
    return(shf);
}



//  SHGethExeFromAltName
//
//  Purpose: To get an Exe handle given an alternate name
//
//  Input:   szPath  - The path or filename of the exe
//
//  Returns: A handle to the exe or NULL on error

HEXE
SHGethExeFromAltName(
    LPTSTR AltName
    )
{
    HEXE  hexe;
    LPTSTR p;

    hexe = SHGetNextExe (NULL);

    while (hexe) {
        p = SHGetModNameFromHexe(hexe);
        if (p && (_tcsicmp(p, AltName) == 0)) {
            return hexe;
        }
        hexe = SHGetNextExe(hexe);
    }

    return NULL;
}


//  SHGethExeFromExeName
//
//  Purpose: To get an Exe handle given an Exe name
//
//  Input:   szPath  - The path or filename of the exe
//
//  Returns: A handle to the exe or NULL on error

HEXE
SHGethExeFromExeName(
    LPTSTR ExeName
    )
{
    HEXE  hexe;
    _TCHAR   szOMFPath[_MAX_CVPATH];
    _TCHAR   szOMFFile[_MAX_CVFNAME];
    _TCHAR   szOMFExt[_MAX_CVEXT];
    _TCHAR   szName[_MAX_CVPATH];
    _TCHAR   szFile[_MAX_CVFNAME + 16];
    _TCHAR   szExt[_MAX_CVEXT];
    DWORD i;

    // BUGBUG:  See the end of the file.  This is the code from the VC shsymbol.c
    //          that's supposed to handle long file names.

    if (!_tfullpath (szName, ExeName, sizeof (szName))) {
        _tcscpy(szName, ExeName);
    }

    i = _tcslen(szName);
    if (szName[i-1] == '.') {
        szName[--i] = '\0';
    }

    _tsplitpath(szName, NULL, NULL, szFile, szExt);
    if (!szExt[0] || !szExt[1]) {
        szExt[0] = '\0';
    }

    for (hexe = SHGetNextExe((HEXE)NULL); hexe; hexe = SHGetNextExe(hexe)) {
        _tcscpy(szOMFPath, SHGetExeName(hexe));
        _tsplitpath(szOMFPath, NULL, NULL, szOMFFile, szOMFExt);

        if (_tcsicmp(szOMFFile, szFile) != 0) {
            _tcscpy(szOMFPath, SHGetModNameFromHexe(hexe));
            _tsplitpath(szOMFPath, NULL, NULL, szOMFFile, szOMFExt);
        }

        if (_tcsicmp(szOMFFile, szFile) == 0) {
            if (szExt[0]) {
                if (_tcsicmp(szExt, szOMFExt) != 0) {
                    continue;
                }
            }
            return hexe;
        }
    }

    return NULL;
}


//  SHGethExeFromName
//
//  Purpose: To get an Exe handle given a name, or partial name
//
//  Input:
//      szPath  - The path or filename of the exe
//
//  Returns:
//      A handle to the exe or NULL on error

HEXE
SHGethExeFromName (
    LPTSTR ltszPath
    )
{
    HEXE    hexe;
    _TCHAR  szAltPath[_MAX_CVPATH];
    _TCHAR  szOMFPath[_MAX_CVPATH];
    LSZ     lpch;
    DWORD   i;
    LPTSTR  p;
    LPTSTR  AltName = NULL;

    if (!ltszPath || !(*ltszPath)) {
        return((HEXE)NULL);
    }

    // Parse the input string.  If it starts with a '|', assume this is the
    // string from the DM.  The first field is the image path.  The 7th is the 
    // alternate name.

    if (*ltszPath == '|') {
        i = 0;
        p = ltszPath;
        while (p) {
            p = _tcschr(p, '|');
            if (p) {
                i++;
                p = _tcsinc(p);
                if (i == 7) {
                    if (p && *p) {
                        AltName = p;
                    }
                    break;
                }
            }
        }
        if (AltName) {
            _tcscpy(szAltPath, AltName);
            p = _tcschr(szAltPath, '|');
            if (p) {
                *p = '\0';
            }
        }
        _tcscpy(szOMFPath, &ltszPath[1]);
       for (lpch = szOMFPath; (*lpch != 0) && (*lpch != '|'); lpch = _tcsinc(lpch));
           *lpch = 0;
    } else {
        _tcscpy(szOMFPath, ltszPath);
    }

    if (AltName) {
        hexe = SHGethExeFromAltName(szOMFPath);
        return hexe;
    }

    hexe = SHGethExeFromExeName(szOMFPath);
    if (!hexe) {
        hexe = SHGethExeFromAltName(szOMFPath);
    }

    return(hexe);
}


//  SHGethExeFromName
//
//  Purpose: To get an Exe handle given a module name
//
//  Input:  lszModName - The module name to lookup
//
//  Returns: A handle to the exe or NULL on error

HEXE
SHGethExeFromModuleName(
    LSZ  lszModName
    )
{
    HEXE   hexe = NULL;
    HEXG   hexg;
    LPEXG  lpexg;
    LPSTR  lszmod;

    while (hexe = SHGetNextExe(hexe)) {
        hexg = ((LPEXE) LLLock (hexe))->hexg;
        lpexg = (LPEXG) LLLock(hexg);
        lszmod = lpexg->lszModule;
        LLUnlock (hexe);
        LLUnlock (hexg);
        if (_tcsicmp(lszModName, lszmod) == 0) {
            return hexe;
        }
    }

    return NULL;
}

#define CSOURCESUFFIX 6
#define CBSOURCESUFFIX 4
static _TCHAR  const * const rgszSourceSuffix[CSOURCESUFFIX] = {
    (_TCHAR  *) _T("**.C"),
    (_TCHAR  *) _T(".CPP"),
    (_TCHAR  *) _T(".CXX"),
    (_TCHAR  *) _T(".ASM"),
    (_TCHAR  *) _T(".BAS"),
    (_TCHAR  *) _T(".FOR")
};

//  SHGetModName
//
//  Purpose: To get an name handle given a module handle
//
//  Input:  hmod - the module handle
//
//  Returns: A handle to the exe or NULL on error
//
//  Notes:  The return pointer is only valid until the call to this function

LSZ
SHGetModName (
    HMOD  hmod
    )
{
    // UNDONE: The NT code was modified to eliminate the static szMODName
    // and replace it with a strdup of lsz before returning...  Not sure what
    // the answer is here (I suspect eliminating the static is the right idea,
    // but the VC ide must be modified to free the pointer when it's done.

    _TCHAR      szFullPath[_MAX_CVPATH];
    static _TCHAR  szMODName[_MAX_CVPATH];
    _TCHAR      szExt[_MAX_CVEXT];
    LPCH        lpch;
    LPMDS       lpmds;
    LSZ         lsz = NULL;
    WORD        iFile;
    _TCHAR  *   lpb;
    WORD        iSuffix;
    _TCHAR  *   lpbSuffix;
    BOOL        fMatch;

    if (!hmod) {
        return NULL;
    }

    szFullPath [0] = '\0';

    lpmds = (LPMDS) hmod;

    assert (lpmds);

    // Try to find a familiar source suffix

    iFile = 0;
    while (lpb = (_TCHAR  *) SLNameFromHmod (hmod, (WORD)(iFile + 1))) {

        lpbSuffix = lpb + *lpb + 1 - CBSOURCESUFFIX;
        for (iSuffix = 0; iSuffix < CSOURCESUFFIX; iSuffix++) {
            _TCHAR  *lpbTest = lpbSuffix;
            _TCHAR  const * pbTest = rgszSourceSuffix [ iSuffix ];

            fMatch = TRUE;
            while (fMatch && *pbTest) {
                switch (*pbTest) {
                    case '*':
                        break;

                    default:
                        if (('a'-'A') == (*lpbTest - *pbTest))
                            break;

                    case '.':
                        if (*lpbTest == *pbTest)
                            break;

                        fMatch = FALSE;
                }
                lpbTest = _tcsinc(lpbTest);
                pbTest = _tcsinc(pbTest);
            }

            if (fMatch) {
                break;
            }
        }
        if (fMatch) {
            memmove(szFullPath, lpb + 1, *(lpb));
            szFullPath[*lpb] = 0;
            break;
        }
        iFile++;
    }


    // As a last resort, use the module name from the omf
    if (!szFullPath[0] && lpmds->name) {
        _tcscpy (szFullPath, lpmds->name);
    }

    if (szFullPath[0]) {

        // take off the source name
        if (lpch = _tcschr ((LPCH) szFullPath, '(')) {
            *lpch = '\0';
        }

        // extract the module name (it is in the form of a path)
        _tsplitpath (szFullPath, NULL, NULL, szMODName, szExt);
        lsz = szMODName;
    }

    return lsz;
}



//  SHCmpGlobName
//
//  Purpose: Given a name, and a global symbol, determine if they match
//
//  Input:  pSym - The symbol to compare against
//          lpsstr - The name we're looking for
//          pfnCmp - A function to call to do the compare
//          fCase  - TRUE if case is important, FALSE otherwise
//
//  Returns: TRUE - The symbol matches

LOCAL BOOL
SHCmpGlobName (
    SYMPTR pSym,
    LPSSTR lpsstr,
    PFNCMP pfnCmp,
    SHFLAG fCase
    )
{
    BOOL fRet = FALSE;

    switch (pSym->rectyp) {
        default:
            assert (FALSE); // Should Never be encountered
            break;

        case S_CONSTANT:
        case S_GDATA16:
        case S_GDATA32:
        case S_GTHREAD32:
        case S_UDT:
        case S_COBOLUDT:
            fRet = (!(lpsstr->searchmask & SSTR_symboltype) ||
                     (pSym->rectyp == lpsstr->symtype)) &&
                    !(*pfnCmp)(lpsstr, pSym, (LSZ) SHlszGetSymName(pSym), fCase);

            // save the sym pointer
            break;
    }
    return fRet;
}


//  SHCompareRE
//
//  Purpose: Compare a string (case preserving or not) to a regular expression
//          and return true if the string is in the regex.  Note, we only care
//          about '*' and '?'.
//
//  Input:  pStr - The string to compare
//          pRE  - A regular expression string
//          fCase - TRUE if case preserving, FALSE otherwise.
//
//  Returns: 0 for match, non-0 for no match.

SHFLAG
SHCompareRE (
    LPCH pStr,
    LPCH pRE,
    BOOL fCase
    )
{
    for (;;) {
        switch (*pRE) {
            case 0:
                // End of the pattern:
                if (*pStr == 0) {
                    return 0;
                } else {
                    return 1;
                }

            case '?':
                // Match anything except EOL
                if (!*pStr) {
                    return 1;
                } else {
                    pRE++;
                    pStr++;
                    break;
                }

            case '*':
                // Match 0 or more of anything
                pRE++;
                do {
                    if (!SHCompareRE(pStr, pRE, fCase)) {
                        return 0;
                    }
                } while (*pStr++);
                return 1;

            default:
                if (fCase ? (*pRE != *pStr) : (tolower(*pRE) != tolower(*pStr))) {
                    return 1;
                } else {
                    pRE++;
                    pStr++;
                    break;
                }
        }
    }
}


//  SHFindBpOrReg
//
//  Purpose: Provide a place for SHGetSymbol to lookup a stack symbol.
//
//  Input:   the address of interest, item - the BPoffset or Register
//     and which item we are searching for (S_REG S_BPREL)
//
//  Output:  The buffer rgbName is filled
//
//  Returns TRUE FALSE if found

int
SHFindBpOrReg (
    LPADDR  paddr,
    UOFFSET item,
    WORD    recLoc,
    LPCH    rgbName
    )
{
    // There is only one caller of this function... Can we optimize it?
    SYMPTR  psym;
    SYMPTR  pProc;
    CXT     cxt;
    int     fGo;

    SHHMODFrompCXT (&cxt) = 0;

    if (SHSetCxt (paddr, &cxt) == NULL) {
        return (FALSE);
    }

    for (;;) {
        fGo = FALSE;
        if (SHHBLKFrompCXT(&cxt) != 0) {
            fGo = TRUE;
        } else if ((pProc = (SYMPTR) SHHPROCFrompCXT (&cxt)) != NULL) {
            switch (pProc->rectyp) {

                case S_LPROC16:
                case S_GPROC16:
                    if (((((PROCPTR16)pProc)->off + (CV_uoff32_t)((PROCPTR16)pProc)->DbgStart) <=
                      GetAddrOff (*paddr))  &&
                      (GetAddrOff (*paddr) < (UOFF32)(((PROCPTR16)pProc)->off + ((PROCPTR16)pProc)->DbgEnd))) {
                        fGo = TRUE;
                    }
                    break;

                case S_LPROC32:
                case S_GPROC32:
                    if (((((PROCPTR32)pProc)->off + ((PROCPTR32)pProc)->DbgStart) <=
                      GetAddrOff (*paddr))  &&
                      (GetAddrOff (*paddr) < (((PROCPTR32)pProc)->off + ((PROCPTR32)pProc)->DbgEnd))) {
                        fGo = TRUE;
                    }
                    break;
                case S_LPROCIA64:
                case S_GPROCIA64:
                    if (((((PROCPTRIA64)pProc)->off + ((PROCPTRIA64)pProc)->DbgStart) <=
                      GetAddrOff (*paddr))  &&
                      (GetAddrOff (*paddr) < (((PROCPTRIA64)pProc)->off + ((PROCPTRIA64)pProc)->DbgEnd))) {
                        fGo = TRUE;
                    }
                    break;
                case S_LPROCMIPS:
                case S_GPROCMIPS:
                    if (((((PROCPTRMIPS)pProc)->off + ((PROCPTRMIPS)pProc)->DbgStart) <=
                      GetAddrOff (*paddr))  &&
                      (GetAddrOff (*paddr) < (((PROCPTRMIPS)pProc)->off + ((PROCPTRMIPS)pProc)->DbgEnd))) {
                        fGo = TRUE;
                    }
                    break;
            }
        }
        if (fGo == FALSE) {
            return  (FALSE);
        }
        if (SHHBLKFrompCXT(&cxt)) {
            psym = (SYMPTR) SHHBLKFrompCXT(&cxt);
        } else if (SHHPROCFrompCXT(&cxt)) {
            psym = (SYMPTR) SHHPROCFrompCXT(&cxt);
        }

        // skip block or proc record

        psym = NEXTSYM (SYMPTR, psym);

        fGo = TRUE;
        while(fGo) {
            switch (psym->rectyp) {
                case S_REGISTER:
                    if ((recLoc == S_REGISTER)  &&
                      ((REGPTR)psym)->reg == (WORD)item) {
                        _tcsncpy (rgbName,
                                  (_TCHAR  *) &((REGPTR)psym)->name[1],
                                  (BYTE)*(((REGPTR)psym)->name));
                        rgbName[(BYTE)*(((REGPTR)psym)->name)] = '\0';
                        return(TRUE);
                    }
                    break;

                case S_END:
                    // terminate loop
                    fGo = FALSE;
                    break;

                case S_LPROC16:
                case S_GPROC16:
                case S_BLOCK16:
                    // terminate loop
                    fGo = FALSE;

                case S_BPREL16:
                    if ((recLoc == S_BPREL16) &&
                      ((UOFFSET)((BPRELPTR16)psym)->off) == item) {
                        _tcsncpy (rgbName,
                                  (_TCHAR  *) &((BPRELPTR16)psym)->name[1],
                                  (BYTE)*(((BPRELPTR16)psym)->name));
                        rgbName[(BYTE)*(((BPRELPTR16)psym)->name)] = '\0';
                        return(TRUE);
                    }
                    break;

                case S_LPROC32:
                case S_GPROC32:
                case S_BLOCK32:
                case S_LPROCIA64:
                case S_GPROCIA64:
                case S_LPROCMIPS:
                case S_GPROCMIPS:
                    // terminate loop
                    fGo = FALSE;

                case S_BPREL32:
                    if ((recLoc == S_BPREL32) &&
                      ((UOFFSET)((BPRELPTR32)psym)->off) == item) {
                        _tcsncpy (rgbName,
                                  (_TCHAR  *) &((BPRELPTR32)psym)->name[1],
                                  (BYTE)*(((BPRELPTR32)psym)->name));
                        rgbName[(BYTE)*(((BPRELPTR32)psym)->name)] = '\0';
                        return(TRUE);
                    }
                    break;

                case S_REGREL32:
                    if ((recLoc == S_BPREL32) &&
                      ((UOFFSET)((LPREGREL32)psym)->off) == item) {
                        _tcsncpy (rgbName,
                                  (_TCHAR  *) &((LPREGREL32)psym)->name[1],
                                  (BYTE)*(((LPREGREL32)psym)->name));
                        rgbName[(BYTE)*(((LPREGREL32)psym)->name)] = '\0';
                        return(TRUE);
                    }
                    break;

                case S_LABEL16:
                case S_WITH16:
                case S_LDATA16:
                case S_GDATA16:
                case S_LABEL32:
                case S_WITH32:
                case S_LDATA32:
                case S_GDATA32:
                case S_LTHREAD32:
                case S_GTHREAD32:
                case S_ENDARG:
                case S_CONSTANT:
                case S_UDT:
                case S_COBOLUDT:
                    break;

                default:
                    return(FALSE);          // Bad SYMBOLS data
            }
            psym = NEXTSYM (SYMPTR, psym);
        }

        // get the parent block

        SHGoToParent(&cxt, &cxt);
    }
    return (FALSE);
}


UOFFSET
SHGetDebugStart (
    HSYM hsym
    )
{
    SYMPTR psym = (SYMPTR) hsym;
    UOFFSET uoff = 0;

    switch (psym->rectyp) {
        case S_LPROC16:
        case S_GPROC16: {
                PROCPTR16 psym = (PROCPTR16) hsym;
                uoff = psym->off + psym->DbgStart;
            }
            break;

        case S_LPROC32:
        case S_GPROC32: {
                PROCPTR32 psym = (PROCPTR32) hsym;
                uoff = psym->off + psym->DbgStart;
            }
            break;

        case S_LPROCIA64:
        case S_GPROCIA64: {
                PROCPTRIA64 psym = (PROCPTRIA64) hsym;
                uoff = psym->off + psym->DbgStart;
            }
            break;

        case S_LPROCMIPS:
        case S_GPROCMIPS: {
                PROCPTRMIPS psym = (PROCPTRMIPS) hsym;
                uoff = psym->off + psym->DbgStart;
            }
            break;

        default:
            assert (FALSE);
    }

    return uoff;
}

LSZ
SHGetSymName (
    HSYM hsym,
    LSZ lsz
    )
{
    SYMPTR psym = (SYMPTR) hsym;
    LPCH   lst = NULL;

    switch (psym->rectyp) {
        case S_REGISTER:
            lst = (LPCH)((REGPTR) psym)->name;
            break;

        case S_CONSTANT:
            lst = (LPCH)((CONSTPTR) psym)->name;
            break;

        case S_BPREL16:
            lst = (LPCH)((BPRELPTR16) psym)->name;
            break;

        case S_GDATA16:
        case S_LDATA16:
            lst = (LPCH)((DATAPTR16) psym)->name;
            break;

        case S_PUB16:
            lst = (LPCH)((PUBPTR16) psym)->name;
            break;

        case S_LPROC16:
        case S_GPROC16:
            lst = (LPCH)((PROCPTR16) psym)->name;
            break;

        case S_THUNK16:
            lst = (LPCH)((THUNKPTR16) psym)->name;
            break;

        case S_BLOCK16:
            lst = (LPCH)((BLOCKPTR16) psym)->name;
            break;

        case S_LABEL16:
            lst = (LPCH)((LABELPTR16) psym)->name;
            break;

        case S_BPREL32:
            lst = (LPCH)((BPRELPTR32) psym)->name;
            break;

        case S_REGREL32:
            lst = (LPCH)((LPREGREL32) psym)->name;
            break;

        case S_GDATA32:
        case S_LDATA32:
        case S_GTHREAD32:
        case S_LTHREAD32:
            lst = (LPCH)((DATAPTR32) psym)->name;
            break;

        case S_PUB32:
            lst = (LPCH)((PUBPTR32) psym)->name;
            break;

        case S_LPROC32:
        case S_GPROC32:
            lst = (LPCH)((PROCPTR32) psym)->name;
            break;

        case S_LPROCIA64:
        case S_GPROCIA64:
            lst = (LPCH)((PROCPTRIA64) psym)->name;
            break;

        case S_LPROCMIPS:
        case S_GPROCMIPS:
            lst = (LPCH)((PROCPTRMIPS) psym)->name;
            break;

        case S_THUNK32:
            lst = (LPCH)((THUNKPTR32) psym)->name;
            break;

        case S_BLOCK32:
            lst = (LPCH)((BLOCKPTR32) psym)->name;
            break;

        case S_LABEL32:
            lst = (LPCH)((LABELPTR32) psym)->name;
            break;
    }

    if (lst != NULL && *lst > 0) {
        _tcsncpy (lsz, lst + 1, *lst);
        *(lsz + *((CHAR *)lst)) = '\0';
        return lsz;
    } else {
        return NULL;
    }
}

BOOL
SHIsLabel (
    HSYM hsym
    )
{
    BOOL fFound = FALSE;
    SYMPTR psym = (SYMPTR) hsym;

    switch (psym->rectyp) {
        case S_LPROC16:
        case S_GPROC16:
        case S_LABEL16:
        case S_LPROC32:
        case S_GPROC32:
        case S_LABEL32:
        case S_LPROCIA64:
        case S_GPROCIA64:
        case S_LPROCMIPS:
        case S_GPROCMIPS:
            fFound = TRUE;
            break;
    }

    return fFound;
}


//  SHAddressToLabel
//
//  Purpose: To find the closest label/proc to the specified address is
//      found and put in pch. Both the symbol table and the
//      publics tables are searched.
//
//  Input:      paddr   - Pointer to the address whose label is to be found
//
//  Output:     pch     - The name is copied here.
//
//  Returns:  TRUE if a label was found.

BOOL
SHAddrToLabel(
    LPADDR paddr,
    LSZ lsz
    )
{
    CXT       cxt;
    SYMPTR    psym;
    LBS       lbs;

    // get the module to search

    *lsz = '\0';
    memset((LPV) &cxt, 0, sizeof(CXT));
    memset((LPV) &lbs, 0, sizeof(lbs));
    lbs.addr = *paddr;
    SHSetCxt (paddr, &cxt);

    if (!cxt.hMod) {
        return(FALSE);
    }

    // Get the nearest local labels in this module
    lbs.tagMod     = cxt.hMod;
    lbs.addr.emi   = cxt.addr.emi;
    SHpSymlplLabLoc (&lbs);

    // Check the candidates found

    if (lbs.tagLab) {
        psym = (SYMPTR) lbs.tagLab;
        switch (psym->rectyp) {
            case S_LABEL16:
                if (GetAddrOff (lbs.addr) == ((LABELPTR16)psym)->off) {
                    _tcsncpy(lsz,
                             (_TCHAR  *)&(((LABELPTR16)psym)->name[1]),
                             (BYTE)(((LABELPTR16)psym)->name[0]));
                    lsz[(BYTE)(((LABELPTR16)psym)->name[0])] = '\0';
                    return TRUE;
                }

            case S_LABEL32:
                if (GetAddrOff (lbs.addr) == ((LABELPTR32)psym)->off) {
                    _tcsncpy(lsz,
                             (_TCHAR  *)&(((LABELPTR32)psym)->name[1]),
                             (BYTE)(((LABELPTR32)psym)->name[0]));
                    lsz[(BYTE)(((LABELPTR32)psym)->name[0])] = '\0';
                    return TRUE;
                }
        }
    }

    if (lbs.tagProc) {
        psym = (SYMPTR) lbs.tagProc;
        switch (psym->rectyp) {
            case S_LPROC16:
            case S_GPROC16:
                if (GetAddrOff (lbs.addr) == ((PROCPTR16)psym)->off) {
                    _tcsncpy(lsz,
                             (_TCHAR  *)&(((PROCPTR16)psym)->name[1]),
                             (BYTE)(((PROCPTR16)psym)->name[0]));
                    lsz[(BYTE)(((PROCPTR16)psym)->name[0])] = '\0';
                    return(TRUE);
                }
                break;

            case S_LPROC32:
            case S_GPROC32:
                if (GetAddrOff (lbs.addr) == ((PROCPTR32)psym)->off) {
                    _tcsncpy(lsz,
                             (_TCHAR  *)&(((PROCPTR32)psym)->name[1]),
                             (BYTE)(((PROCPTR32)psym)->name[0]));
                    lsz[(BYTE)(((PROCPTR32)psym)->name[0])] = '\0';
                    return(TRUE);
                }

                break;

            case S_LPROCIA64:
            case S_GPROCIA64:
                if (GetAddrOff (lbs.addr) == ((PROCPTRIA64)psym)->off) {
                    _tcsncpy(lsz,
                             (_TCHAR  *)&(((PROCPTRIA64)psym)->name[1]),
                             (BYTE)(((PROCPTRIA64)psym)->name[0]));
                    lsz[(BYTE)(((PROCPTRIA64)psym)->name[0])] = '\0';
                    return(TRUE);
                }
                break;

            case S_LPROCMIPS:
            case S_GPROCMIPS:
                if (GetAddrOff (lbs.addr) == ((PROCPTRMIPS)psym)->off) {
                    _tcsncpy(lsz,
                             (_TCHAR  *)&(((PROCPTRMIPS)psym)->name[1]),
                             (BYTE)(((PROCPTRMIPS)psym)->name[0]));
                    lsz[(BYTE)(((PROCPTRMIPS)psym)->name[0])] = '\0';
                    return(TRUE);
                }
                break;
        }
    }

    // now check the publics
    if (!PHGetNearestHsym(SHpADDRFrompCXT(&cxt),
                         SHHexeFromHmod(SHHMODFrompCXT(&cxt)),
                         (PHSYM) &psym)) {

        switch (psym->rectyp) {
            case S_PUB16:
                _tcsncpy(lsz,
                         (_TCHAR  *)&(((DATAPTR16)psym)->name[1]),
                         (BYTE)(((DATAPTR16)psym)->name[0]));
                lsz [(BYTE)(((DATAPTR16)psym)->name[0])] = '\0';
                return(TRUE);

            case S_PUB32:
                _tcsncpy(lsz,
                         (_TCHAR  *)&(((DATAPTR32)psym)->name[1]),
                         (BYTE)(((DATAPTR32)psym)->name[0]));
                lsz [(BYTE)(((DATAPTR32)psym)->name[0])] = '\0';
                return(TRUE);
        }
    }
    return(FALSE);
}

BOOL
SHFIsAddrNonVirtual(
    LPADDR paddr
    )
{
    ADDR addr = *paddr;

    // If SYFixupAddr fails, it's because the address is virtual
    // (unless something is seriously wrong)

    return SYFixupAddr(&addr);
}


BOOL
SHIsEmiLoaded(
    HEXE hexe
    )
{
    BOOL fReturn = ((LPEXE)(LLLock(hexe)))->fIsLoaded;
    LLUnlock(hexe);

    return (fReturn);
}


BOOL
SHIsFarProc (
    HSYM hsym
    )
{
    BOOL fReturn = FALSE;

    switch (((SYMPTR) hsym)->rectyp) {

        case S_LPROC16:
        case S_GPROC16:
            fReturn = ((PROCPTR16) hsym)->flags.CV_PFLAG_FAR;
            break;

        case S_LPROC32:
        case S_GPROC32:
            fReturn = ((PROCPTR32) hsym)->flags.CV_PFLAG_FAR;
            break;

        case S_LPROCIA64:
        case S_GPROCIA64:
            fReturn = FALSE;
            break;

        case S_LPROCMIPS:
        case S_GPROCMIPS:
            fReturn = FALSE;
            break;
    }

    return fReturn;
}


char *M68KRegisterName[] =
{
    "D0",       //  0
    "D1",       //  1
    "D2",       //  2
    "D3",       //  3
    "D4",       //  4
    "D5",       //  5
    "D6",       //  6
    "D7",       //  7
    "A0",       //  8
    "A1",       //  9
    "A2",       // 10
    "A3",       // 11
    "A4",       // 12
    "A5",       // 13
    "A6",       // 14
    "A7",       // 15
    "CCR",      // 16
    "SR",       // 17
    "USP",      // 18
    "MSP",      // 19
    "SFC",      // 20
    "DFC",      // 21
    "CACR",     // 22
    "VBR",      // 23
    "CAAR",     // 24
    "ISP",      // 25
    "PC",       // 26
    "reserved", // 27
    "FPCR",     // 28
    "FPSR",     // 29
    "FPIAR",    // 30
    "reserved", // 31
    "FP0",      // 32
    "FP1",      // 33
    "FP2",      // 34
    "FP3",      // 35
    "FP4",      // 36
    "FP5",      // 37
    "FP6",      // 38
    "FP7",      // 39
    NULL
};

char *X86RegisterName[] =
{
    "NONE",    //  0
    "AL",      //  1
    "CL",      //  2
    "DL",      //  3
    "BL",      //  4
    "AH",      //  5
    "CH",      //  6
    "DH",      //  7
    "BH",      //  8
    "AX",      //  9
    "CX",      // 10
    "DX",      // 11
    "BX",      // 12
    "SP",      // 13
    "BP",      // 14
    "SI",      // 15
    "DI",      // 16
    "EAX",     // 17
    "ECX",     // 18
    "EDX",     // 19
    "EBX",     // 20
    "ESP",     // 21
    "EBP",     // 22
    "ESI",     // 23
    "EDI",     // 24
    "ES",      // 25
    "CS",      // 26
    "SS",      // 27
    "DS",      // 28
    "FS",      // 29
    "GS",      // 30
    "IP",      // 31
    "FLAGS",   // 32
    NULL
};

typedef struct _RegInfo {
    WORD    wIndex;
    CHAR *  szName;
} REGINFO;
typedef REGINFO * LPREGINFO;

REGINFO rgRegInfoPPC[] = {
    // PowerPC General Registers (User Level)

    { CV_PPC_GPR0,      "GPR0"  },
    { CV_PPC_GPR1,      "SP"    },
    { CV_PPC_GPR2,      "RTOC"  },
    { CV_PPC_GPR3,      "GPR3"  },
    { CV_PPC_GPR4,      "GPR4"  },
    { CV_PPC_GPR5,      "GPR5"  },
    { CV_PPC_GPR6,      "GPR6"  },
    { CV_PPC_GPR7,      "GPR7"  },
    { CV_PPC_GPR8,      "GPR8"  },
    { CV_PPC_GPR9,      "GPR9"  },
    { CV_PPC_GPR10,     "GPR10" },
    { CV_PPC_GPR11,     "GPR11" },
    { CV_PPC_GPR12,     "GPR12" },
    { CV_PPC_GPR13,     "GPR13" },
    { CV_PPC_GPR14,     "GPR14" },
    { CV_PPC_GPR15,     "GPR15" },
    { CV_PPC_GPR16,     "GPR16" },
    { CV_PPC_GPR17,     "GPR17" },
    { CV_PPC_GPR18,     "GPR18" },
    { CV_PPC_GPR19,     "GPR19" },
    { CV_PPC_GPR20,     "GPR20" },
    { CV_PPC_GPR21,     "GPR21" },
    { CV_PPC_GPR22,     "GPR22" },
    { CV_PPC_GPR23,     "GPR23" },
    { CV_PPC_GPR24,     "GPR24" },
    { CV_PPC_GPR25,     "GPR25" },
    { CV_PPC_GPR26,     "GPR26" },
    { CV_PPC_GPR27,     "GPR27" },
    { CV_PPC_GPR28,     "GPR28" },
    { CV_PPC_GPR29,     "GPR29" },
    { CV_PPC_GPR30,     "GPR30" },
    { CV_PPC_GPR31,     "GPR31" },

    // PowerPC Condition Register (User Level)

    { CV_PPC_CR,        "CR"    },
    { CV_PPC_CR0,       "CR0"   },
    { CV_PPC_CR1,       "CR1"   },
    { CV_PPC_CR2,       "CR2"   },
    { CV_PPC_CR3,       "CR3"   },
    { CV_PPC_CR4,       "CR4"   },
    { CV_PPC_CR5,       "CR5"   },
    { CV_PPC_CR6,       "CR6"   },
    { CV_PPC_CR7,       "CR7"   },

    // PowerPC Floating Point Registers (User Level)

    { CV_PPC_FPR0,      "FPR0"  },
    { CV_PPC_FPR1,      "FPR1"  },
    { CV_PPC_FPR2,      "FPR2"  },
    { CV_PPC_FPR3,      "FPR3"  },
    { CV_PPC_FPR4,      "FPR4"  },
    { CV_PPC_FPR5,      "FPR5"  },
    { CV_PPC_FPR6,      "FPR6"  },
    { CV_PPC_FPR7,      "FPR7"  },
    { CV_PPC_FPR8,      "FPR8"  },
    { CV_PPC_FPR9,      "FPR9"  },
    { CV_PPC_FPR10,     "FPR10" },
    { CV_PPC_FPR11,     "FPR11" },
    { CV_PPC_FPR12,     "FPR12" },
    { CV_PPC_FPR13,     "FPR13" },
    { CV_PPC_FPR14,     "FPR14" },
    { CV_PPC_FPR15,     "FPR15" },
    { CV_PPC_FPR16,     "FPR16" },
    { CV_PPC_FPR17,     "FPR17" },
    { CV_PPC_FPR18,     "FPR18" },
    { CV_PPC_FPR19,     "FPR19" },
    { CV_PPC_FPR20,     "FPR20" },
    { CV_PPC_FPR21,     "FPR21" },
    { CV_PPC_FPR22,     "FPR22" },
    { CV_PPC_FPR23,     "FPR23" },
    { CV_PPC_FPR24,     "FPR24" },
    { CV_PPC_FPR25,     "FPR25" },
    { CV_PPC_FPR26,     "FPR26" },
    { CV_PPC_FPR27,     "FPR27" },
    { CV_PPC_FPR28,     "FPR28" },
    { CV_PPC_FPR29,     "FPR29" },
    { CV_PPC_FPR30,     "FPR30" },
    { CV_PPC_FPR31,     "FPR31" },

    // PowerPC Floating Point Status and Control Register (User Level)

    { CV_PPC_FPSCR,     "FPSCR" },

    // PowerPC Machine State Register (Supervisor Level)

    { CV_PPC_MSR,       "MSR"   },

    // PowerPC Segment Registers (Supervisor Level)

    { CV_PPC_SR0,       "SR0"   },
    { CV_PPC_SR1,       "SR1"   },
    { CV_PPC_SR2,       "SR2"   },
    { CV_PPC_SR3,       "SR3"   },
    { CV_PPC_SR4,       "SR4"   },
    { CV_PPC_SR5,       "SR5"   },
    { CV_PPC_SR6,       "SR6"   },
    { CV_PPC_SR7,       "SR7"   },
    { CV_PPC_SR8,       "SR8"   },
    { CV_PPC_SR9,       "SR9"   },
    { CV_PPC_SR10,      "SR10"  },
    { CV_PPC_SR11,      "SR11"  },
    { CV_PPC_SR12,      "SR12"  },
    { CV_PPC_SR13,      "SR13"  },
    { CV_PPC_SR14,      "SR14"  },
    { CV_PPC_SR15,      "SR15"  },

    // For all of the special purpose registers add 100 to the SPR# that the
    // Motorola/IBM documentation gives with the exception of any imaginary
    // registers.

    // PowerPC Special Purpose Registers (User Level)

    { CV_PPC_PC,        "PC"    },    // PC (imaginary register)

    { CV_PPC_MQ,        "MQ"    },    // MPC601
    { CV_PPC_XER,       "XER"   },
    { CV_PPC_RTCU,      "RTCU"  },    // MPC601
    { CV_PPC_RTCL,      "RTCL"  },    // MPC601
    { CV_PPC_LR,        "LR"    },
    { CV_PPC_CTR,       "CTR"   },

    // PowerPC Special Purpose Registers (Supervisor Level)

    { CV_PPC_DSISR,     "DSISR" },
    { CV_PPC_DAR,       "DAR"   },
    { CV_PPC_DEC,       "DEC"   },
    { CV_PPC_SDR1,      "SDR1"  },
    { CV_PPC_SRR0,      "SRR0"  },
    { CV_PPC_SRR1,      "SRR1"  },
    { CV_PPC_SPRG0,     "SPRG0" },
    { CV_PPC_SPRG1,     "SPRG1" },
    { CV_PPC_SPRG2,     "SPRG2" },
    { CV_PPC_SPRG3,     "SPRG3" },
    { CV_PPC_ASR,       "ASR"   },    // 64-bit implementations only
    { CV_PPC_EAR,       "EAR"   },
    { CV_PPC_PVR,       "PVR"   },
    { CV_PPC_BAT0U,     "BAT0U" },
    { CV_PPC_BAT0L,     "BAT0L" },
    { CV_PPC_BAT1U,     "BAT1U" },
    { CV_PPC_BAT1L,     "BAT1L" },
    { CV_PPC_BAT2U,     "BAT2U" },
    { CV_PPC_BAT2L,     "BAT2L" },
    { CV_PPC_BAT3U,     "BAT3U" },
    { CV_PPC_BAT3L,     "BAT3L" },
    { CV_PPC_DBAT0U,    "DBAT0U" },
    { CV_PPC_DBAT0L,    "DBAT0L" },
    { CV_PPC_DBAT1U,    "DBAT1U" },
    { CV_PPC_DBAT1L,    "DBAT1L" },
    { CV_PPC_DBAT2U,    "DBAT2U" },
    { CV_PPC_DBAT2L,    "DBAT2L" },
    { CV_PPC_DBAT3U,    "DBAT3U" },
    { CV_PPC_DBAT3L,    "DBAT3L" },

    // PowerPC Special Purpose Registers Implementation Dependent (Supervisor Level)

    // Doesn't appear that IBM/Motorola has finished defining these.

    { CV_PPC_PMR0,      "PMR0"  },   // MPC620
    { CV_PPC_PMR1,      "PMR1"  },   // MPC620
    { CV_PPC_PMR2,      "PMR2"  },   // MPC620
    { CV_PPC_PMR3,      "PMR3"  },   // MPC620
    { CV_PPC_PMR4,      "PMR4"  },   // MPC620
    { CV_PPC_PMR5,      "PMR5"  },   // MPC620
    { CV_PPC_PMR6,      "PMR6"  },   // MPC620
    { CV_PPC_PMR7,      "PMR7"  },   // MPC620
    { CV_PPC_PMR8,      "PMR8"  },   // MPC620
    { CV_PPC_PMR9,      "PMR9"  },   // MPC620
    { CV_PPC_PMR10,     "PMR10" },   // MPC620
    { CV_PPC_PMR11,     "PMR11" },   // MPC620
    { CV_PPC_PMR12,     "PMR12" },   // MPC620
    { CV_PPC_PMR13,     "PMR13" },   // MPC620
    { CV_PPC_PMR14,     "PMR14" },   // MPC620
    { CV_PPC_PMR15,     "PMR15" },   // MPC620

    { CV_PPC_DMISS,     "DMISS" },   // MPC603
    { CV_PPC_DCMP,      "DCMP"  },   // MPC603
    { CV_PPC_HASH1,     "HASH1" },   // MPC603
    { CV_PPC_HASH2,     "HASH2" },   // MPC603
    { CV_PPC_IMISS,     "IMISS" },   // MPC603
    { CV_PPC_ICMP,      "ICMP"  },   // MPC603
    { CV_PPC_RPA,       "RPA"   },   // MPC603

    { CV_PPC_HID0,      "HID0"  },   // MPC601, MPC603, MPC620
    { CV_PPC_HID1,      "HID1"  },   // MPC601
    { CV_PPC_HID2,      "HID2"  },   // MPC601, MPC603, MPC620 (IABR)
    { CV_PPC_HID3,      "HID3"  },   // Not Defined
    { CV_PPC_HID4,      "HID4"  },   // Not Defined
    { CV_PPC_HID5,      "HID5"  },   // MPC601, MPC604, MPC620 (DABR)
    { CV_PPC_HID6,      "HID6"  },   // Not Defined
    { CV_PPC_HID7,      "HID7"  },   // Not Defined
    { CV_PPC_HID8,      "HID8"  },   // MPC620 (BUSCSR)
    { CV_PPC_HID9,      "HID9"  },   // MPC620 (L2CSR)
    { CV_PPC_HID10,     "HID10" },   // Not Defined
    { CV_PPC_HID11,     "HID11" },   // Not Defined
    { CV_PPC_HID12,     "HID12" },   // Not Defined
    { CV_PPC_HID13,     "HID13" },   // MPC604 (HCR)
    { CV_PPC_HID14,     "HID14" },   // Not Defined
    { CV_PPC_HID15,     "HID15" }    // MPC601, MPC604, MPC620 (PIR)
};

REGINFO rgRegInfoAlpha[] = {
    { CV_ALPHA_NOREG, "NOREG" },
    { CV_ALPHA_FltF0, "F0" },
    { CV_ALPHA_FltF1, "F1" },
    { CV_ALPHA_FltF2, "F2" },
    { CV_ALPHA_FltF3, "F3" },
    { CV_ALPHA_FltF4, "F4" },
    { CV_ALPHA_FltF5, "F5" },
    { CV_ALPHA_FltF6, "F6" },
    { CV_ALPHA_FltF7, "F7" },
    { CV_ALPHA_FltF8, "F8" },
    { CV_ALPHA_FltF9, "F9" },
    { CV_ALPHA_FltF10, "F10" },
    { CV_ALPHA_FltF11, "F11" },
    { CV_ALPHA_FltF12, "F12" },
    { CV_ALPHA_FltF13, "F13" },
    { CV_ALPHA_FltF14, "F14" },
    { CV_ALPHA_FltF15, "F15" },
    { CV_ALPHA_FltF16, "F16" },
    { CV_ALPHA_FltF17, "F17" },
    { CV_ALPHA_FltF18, "F18" },
    { CV_ALPHA_FltF19, "F19" },
    { CV_ALPHA_FltF20, "F20" },
    { CV_ALPHA_FltF21, "F21" },
    { CV_ALPHA_FltF22, "F22" },
    { CV_ALPHA_FltF23,   "F23" },
    { CV_ALPHA_FltF24,   "F24" },
    { CV_ALPHA_FltF25,   "F25" },
    { CV_ALPHA_FltF26,   "F26" },
    { CV_ALPHA_FltF27,   "F27" },
    { CV_ALPHA_FltF28,   "F28" },
    { CV_ALPHA_FltF29,   "F29" },
    { CV_ALPHA_FltF30,   "F30" },
    { CV_ALPHA_FltF31,   "F31" },
    { CV_ALPHA_IntV0,   "V0" },
    { CV_ALPHA_IntT0,   "T0" },
    { CV_ALPHA_IntT1,   "T1" },
    { CV_ALPHA_IntT2,   "T2" },
    { CV_ALPHA_IntT3,   "T3" },
    { CV_ALPHA_IntT4,   "T4" },
    { CV_ALPHA_IntT5,   "T5" },
    { CV_ALPHA_IntT6,   "T6" },
    { CV_ALPHA_IntT7,   "T7" },
    { CV_ALPHA_IntS0,   "S0" },
    { CV_ALPHA_IntS1,   "S1" },
    { CV_ALPHA_IntS2,   "S2" },
    { CV_ALPHA_IntS3,   "S3" },
    { CV_ALPHA_IntS4,   "S4" },
    { CV_ALPHA_IntS5,   "S5" },
    { CV_ALPHA_IntFP,   "FP" },
    { CV_ALPHA_IntA0,   "A0" },
    { CV_ALPHA_IntA1,   "A1" },
    { CV_ALPHA_IntA2,   "A2" },
    { CV_ALPHA_IntA3,   "A3" },
    { CV_ALPHA_IntA4,   "A4" },
    { CV_ALPHA_IntA5,   "A5" },
    { CV_ALPHA_IntT8,   "T8" },
    { CV_ALPHA_IntT9,   "T9" },
    { CV_ALPHA_IntT10,  "T10" },
    { CV_ALPHA_IntT11,  "T11" },
    { CV_ALPHA_IntRA,   "RA" },
    { CV_ALPHA_IntT12,  "T12" },
    { CV_ALPHA_IntAT,   "AT" },
    { CV_ALPHA_IntGP,   "GP" },
    { CV_ALPHA_IntSP,   "SP" },
    { CV_ALPHA_IntZERO, "ZERO" },
    { CV_ALPHA_Fpcr,    "FPCR" },
    { CV_ALPHA_Fir, "FIR" },
    { CV_ALPHA_Psr, "PSR" },
    { CV_ALPHA_FltFsr,  "FSR" },
};

INT __cdecl
RegInfoCmp (
    const VOID * lpElem1,
    const VOID * lpElem2
    )
{
    if (((LPREGINFO)lpElem1)->wIndex < ((LPREGINFO)lpElem2)->wIndex) {
        return (-1);
    } else if (((LPREGINFO)lpElem1)->wIndex > ((LPREGINFO)lpElem2)->wIndex) {
        return (1);
    } else {
        return (0);
    }
}

//  SHGetSymLoc
//
//  Input:
//      hSym    - A handle to the symbol to get a location.
//      lsz     - Where to write the result.
//      cbMax   - Size of lsz.
//      pcxt    - Context.
//
//  Output:
//      lsz filled in.
//
//  Returns - The number of bytes written to the string.
//
//  Notes: lpSym emspage must be loaded

int
SHGetSymLoc (
    HSYM hsym,
    LSZ lsz,
    UINT cbMax,
    PCXT pcxt
    )
{
    SYMPTR lpsym = (SYMPTR) hsym;
    char rgch[20];
    MPT TargetMachine;

    if (cbMax == 0) {
        return 0;
    }

    // What machine is this?

    TargetMachine = GetTargetMachine();

    memset(rgch, '\0', sizeof(rgch));

    switch (lpsym->rectyp) {
        case S_BPREL16:
            if (((BPRELPTR16) lpsym)->off >= 0) {
                sprintf(rgch, "[BP+%04X]", ((BPRELPTR16) lpsym)->off);
            } else {
                sprintf(rgch, "[BP-%04X]", - ((BPRELPTR16) lpsym)->off);
            }
            break;

        case S_BPREL32:
            {
                long off = (long) ((BPRELPTR32) lpsym)->off;
                char *  szFMT;
                char    ch;
                char *  szBPREG;

                switch (TargetMachine) {
                    case mptm68k:
                        szBPREG = "A6";
                        break;

                    case mptmppc:
                        szBPREG = "[SP]";
                        break;

                    default:
                        szBPREG = "EBP";
                        break;
                }

                if (off < 0) {
                    ch = '-';
                    off = -off;
                } else {
                    ch = '+';
                }

                if (HIWORD(off)) {
                    szFMT = "[%s%c%08lX]";
                } else {
                    szFMT = "[%s%c%04lX]";
                }

                sprintf(rgch, szFMT, szBPREG, ch, off);
            }
            break;

        case S_REGREL32:
            {
                long off = (long) ((LPREGREL32) lpsym)->off;
                short reg = ((LPREGREL32)lpsym)->reg;
                char *lpch = rgch;
                REGINFO     regInfo;
                LPREGINFO   lpRegInfo;
                char    ch;
                char   *szFMT;
                char   *szRegName;

                // UNDONE: Only Alpha really does this right.  Fix the rest.

                switch (TargetMachine) {
                    case mptdaxp:
                        regInfo.wIndex = reg;

                        lpRegInfo = (LPREGINFO) bsearch(&regInfo,
                                            rgRegInfoAlpha,
                                            sizeof (rgRegInfoAlpha) / sizeof (rgRegInfoAlpha[0]),
                                            sizeof (rgRegInfoAlpha[0]),
                                            &RegInfoCmp);

                        assert (lpRegInfo);
                        szRegName = lpRegInfo->szName;
                        break;

                    case mptia64:
                        assert("need code for IA64");
                        break;

                    case mptmips:
                        switch (reg) {
                            case CV_M4_IntSP:
                                szRegName = "SP";
                                break;
                            case CV_M4_IntS8:
                                szRegName = "S8";
                                break;
                            case CV_M4_IntGP:
                                szRegName = "GP";
                                break;
                            default:
                                szRegName = "REG";
                                break;
                        }
                        break;

                    default:
                        szRegName = "REG";
                }

                if (off < 0) {
                    ch = '-';
                    off = -off;
                } else {
                    ch = '+';
                }

                if (HIWORD(off)) {
                    szFMT = "[%s%c%08lX]";
                } else {
                    szFMT = "[%s%c%04lX]";
                }

                sprintf(lpch, szFMT, szRegName, ch, off);
            }
            break;

        case S_REGISTER:
            {
                WORD        iReg1, iReg2;
                REGINFO     regInfo;
                LPREGINFO   lpRegInfo;

                // UNDONE: Again, I think Alpha is the only one to do it right...

                switch (TargetMachine) {
                    case mptm68k:
                        iReg1 = (((REGPTR) lpsym)->reg) & 0x00ff;
                        _tcscpy (rgch, M68KRegisterName [iReg1]);
                        break;

                    case mptdaxp:
                        iReg1 = (((REGPTR) lpsym)->reg);

                        regInfo.wIndex = iReg1;

                        lpRegInfo = (LPREGINFO) bsearch (&regInfo,
                                             rgRegInfoAlpha,
                                             sizeof (rgRegInfoAlpha) / sizeof (rgRegInfoAlpha[0]),
                                             sizeof (rgRegInfoAlpha[0]),
                                             &RegInfoCmp);
                        assert (lpRegInfo);
                        _tcscpy (rgch, lpRegInfo->szName);
                        break;

                    case mptia64:
                        assert("need code for IA64");
                        break;

                    case mptmppc:
                    case mptntppc:
                        iReg1 = (((REGPTR) lpsym)->reg);

                        regInfo.wIndex = iReg1;

                        lpRegInfo = (LPREGINFO) bsearch (&regInfo,
                                             rgRegInfoPPC,
                                             sizeof (rgRegInfoPPC) / sizeof (rgRegInfoPPC[0]),
                                             sizeof (rgRegInfoPPC[0]),
                                             &RegInfoCmp);
                        assert (lpRegInfo);
                        _tcscpy (rgch, lpRegInfo->szName);
                        break;

                    case mptmips:
                        // UNDONE: Need to add the MIPS register definitions
                        rgch[0] = '\0';
                        break;

                    case mptix86:
                        iReg1 = (((REGPTR) lpsym)->reg) & 0x00ff;
                        iReg2 = ((((REGPTR)lpsym)->reg) >> 8) & 0x00ff;

                        rgch[0] = '\0';

                        if (iReg2) {
                            _tcscat (rgch, X86RegisterName [iReg2]);
                            _tcscat (rgch, ":");
                        }

                        _tcscat (rgch, X86RegisterName [iReg1]);
                        break;
                }
                _tcscat (rgch, " reg");
            }
            break;

        case S_CONSTANT: {
            HTYPE   htype;
            lfOEM * ptype;

            htype = THGetTypeFromIndex (
                SHHMODFrompCXT (pcxt),
                ((CONSTSYM *)lpsym)->typind
           );

            if (htype) {
                ptype = (lfOEM *)MMLock ((HDEP)htype);

                ptype = (lfOEM *)&(((TYPTYPE *)ptype)->leaf);

                if (ptype->cvOEM != OEM_MS_FORTRAN90) {
                    _tcscpy (rgch, "constant");
                }

                MMUnlock ((HDEP) htype);
            }

            break;
        }

        case S_PUB16:
        case S_LDATA16:
        case S_GDATA16:
            {
                ADDR addr = {0};

                assert (pcxt->hMod != 0);

                SetAddrSeg (&addr, ((DATAPTR16) lpsym)->seg);
                SE_SetAddrOff (&addr, ((DATAPTR16) lpsym)->off);
                emiAddr (addr) = (HEMI) SHHexeFromHmod (pcxt->hMod);
                ADDR_IS_LI (addr) = TRUE;
                SYFixupAddr (&addr);
                if (ADDR_IS_LI (addr) != TRUE) {
                    sprintf(rgch, "%04X:%04X", GetAddrSeg(addr), GetAddrOff(addr));
                }
            }
            break;

        case S_PUB32:
        case S_LDATA32:
        case S_GDATA32:
        case S_LTHREAD32:
        case S_GTHREAD32:
            {
                ADDR addr = {0};

                assert (pcxt->hMod != 0);

                SetAddrSeg (&addr, ((DATAPTR32) lpsym)->seg);
                SE_SetAddrOff (&addr, ((DATAPTR32) lpsym)->off);
                emiAddr (addr) = (HEMI) SHHexeFromHmod (pcxt->hMod);
                ADDR_IS_LI (addr) = TRUE;
                // REVIEW - billjoy - not necessarily ADDRLIN32.  How do we tell?
                ADDRLIN32 (addr);
                SYFixupAddr (&addr);

                if (ADDR_IS_LI (addr) != TRUE) {
                    if ((GetAddrSeg(addr) != 0) && (TargetMachine == mptm68k))
                        sprintf(rgch, "%04X:%08lX", GetAddrSeg(addr), GetAddrOff(addr));
                    else
                        sprintf(rgch, "%08lX", GetAddrOff(addr));
                }
            }
            break;

        case S_LPROC16:
        case S_GPROC16:
            {
                ADDR addr = {0};

                assert (pcxt->hMod != 0);

                SetAddrSeg (&addr, ((PROCPTR16) lpsym)->seg);
                SE_SetAddrOff (&addr, ((PROCPTR16) lpsym)->off);
                emiAddr (addr) = (HEMI) SHHexeFromHmod (pcxt->hMod);
                ADDR_IS_LI (addr) = TRUE;
                SYFixupAddr (&addr);

                if (ADDR_IS_LI (addr) != TRUE) {
                    sprintf(rgch, "%04X:%04X", GetAddrSeg(addr), GetAddrOff(addr));
                }
            }
            break;

        case S_LPROC32:
        case S_GPROC32:
            {
                ADDR addr = {0};

                assert (pcxt->hMod != 0);

                SetAddrSeg (&addr, ((PROCPTR32) lpsym)->seg);
                SE_SetAddrOff (&addr, ((PROCPTR32) lpsym)->off);
                emiAddr (addr) = (HEMI) SHHexeFromHmod (pcxt->hMod);
                ADDR_IS_LI (addr) = TRUE;
                // REVIEW - billjoy - not necessarily ADDRLIN32.  How do we tell?
                ADDRLIN32 (addr);

                SYFixupAddr (&addr);

                if (ADDR_IS_LI (addr) != TRUE) {
                    if ((GetAddrSeg(addr) != 0) && (TargetMachine == mptm68k))
                        sprintf(rgch, "%04X:%08lX", GetAddrSeg(addr), GetAddrOff(addr));
                    else
                        sprintf(rgch, "%08lX", GetAddrOff(addr));
                }
            }

            break;

        case S_LPROCIA64:
        case S_GPROCIA64:
            {
                ADDR addr = {0};

                assert (pcxt->hMod != 0);

                SetAddrSeg (&addr, ((PROCPTRIA64) lpsym)->seg);
                SE_SetAddrOff (&addr, ((PROCPTRIA64) lpsym)->off);
                emiAddr (addr) = (HEMI) SHHexeFromHmod (pcxt->hMod);
                ADDR_IS_LI (addr) = TRUE;
                // REVIEW - billjoy - not necessarily ADDRLIN32.  How do we tell?
                // Do we even care here (IA64)?
                ADDRLIN32 (addr);

                SYFixupAddr (&addr);

                if (ADDR_IS_LI (addr) != TRUE) {
                    sprintf(rgch, "%16I64X", GetAddrOff(addr));
                }
            }

            break;

        case S_LPROCMIPS:
        case S_GPROCMIPS:
            {
                ADDR addr = {0};

                assert (pcxt->hMod != 0);

                SetAddrSeg (&addr, ((PROCPTRMIPS) lpsym)->seg);
                SE_SetAddrOff (&addr, ((PROCPTRMIPS) lpsym)->off);
                emiAddr (addr) = (HEMI) SHHexeFromHmod (pcxt->hMod);
                ADDR_IS_LI (addr) = TRUE;
                // REVIEW - billjoy - not necessarily ADDRLIN32.  How do we tell?
                // Do we even care here (MIPS)?
                ADDRLIN32 (addr);

                SYFixupAddr (&addr);

                if (ADDR_IS_LI (addr) != TRUE) {
                    if ((GetAddrSeg(addr) != 0) && (TargetMachine == mptm68k))
                        sprintf(rgch, "%04X:%08lX", GetAddrSeg(addr), GetAddrOff(addr));
                    else
                        sprintf(rgch, "%08lX", GetAddrOff(addr));
                }
            }

            break;
    }

    _tcsncpy (lsz, rgch, cbMax);
    lsz[cbMax-1] = '\0';    // ensure that it's null-terminated

    return _tcslen (lsz);
}

LPV
SHLpGSNGetTable(
    HEXE hexe
    )
{
    LPB     lpb = NULL;
    HEXG    hexg;

    if (hexe) {
        //
        //  Force symbols to be loaded now
        //

        SHWantSymbols(hexe);

        //

        hexg = ((LPEXE)LLLock(hexe))->hexg;
        assert(hexg);
        lpb = ((LPEXG)LLLock(hexg))->lpgsi;
        LLUnlock(hexe);
        LLUnlock(hexg);
    }
    return (LPV)lpb;
}

SHFLAG PHExactCmp (HVOID, HVOID, LSZ, SHFLAG);

HSYM
SHFindSymInExe (
    HEXE   hexe,
    LPSSTR lpsstr,
    BOOL   fCaseSensitive
    )
{
    CXT  cxt    = { 0 };
    CXT  cxtOut = { 0 };
    HSYM hsym   = NULL;

    cxt.hMod = 0;

    // First search all of the modules in the exe

    while (!hsym && (cxt.hMod = SHGetNextMod (hexe, cxt.hMod)) != 0)
    {
        hsym = SHFindNameInContext(NULL,
                                    &cxt,
                                    lpsstr,
                                    fCaseSensitive,
                                    PHExactCmp,
                                    &cxtOut);
    }

#pragma message("REVIEW: Should SHFindSymInExe call PHFindNameInPublics???")
#if 0

    // UNDONE: Perhaps for NTSD support?

    // This code is very expensive and yet has no effect!!!
    // It ignores the HSYM which is returned by PHFindNameInPublics!
    //
    // I'm not sure which is the best fix -- putting "hsym ="
    // in front of the call to PHFindNameInPublics, or disabling
    // this code entirely.  Since the old way seems to have
    // works fine without causing any trouble, I'm going to
    // just disable it for now.  But this should be revisited.
    // The name of this function (SHFindSymInExe) implies
    // to the caller that it searches publics as well as other
    // symbols.  [mikemo]
    if (!hsym) {
        PHFindNameInPublics (
            NULL,
            hexe,
            lpsstr,
            fCaseSensitive,
            PHExactCmp
       );
    }
#endif

    return hsym;
}

BOOL
SHFindSymbol (
    LSZ   lsz,
    PADDR lpaddr,
    LPASR lpasr
    )
{
    ADDR addr   = *lpaddr;
    CXT  cxt    = {0};
    CXT  cxtOut = {0};
    SSTR sstr   = {0};
    HSYM hsym   = NULL;
    HEXE hexe   = hexeNull;
    BOOL fCaseSensitive = TRUE;

    // Get a context for the code address that was passed in

    SYUnFixupAddr (&addr);
    SHSetCxt (&addr, &cxt);
    hexe = SHHexeFromHmod (cxt.hMod);

    // Do an outward context search

    sstr.lpName = (LPB) lsz;
    sstr.cb = _tcslen (lsz);

    // Search all of the blocks & procs outward

    while ((cxt.hBlk || cxt.hProc) && !hsym) {
        hsym = SHFindNameInContext(NULL,
                                   &cxt,
                                   &sstr,
                                   fCaseSensitive,
                                   PHExactCmp,
                                   &cxtOut);

        SHGoToParent (&cxt, &cxt);
    }

    if (!hsym) {
        hsym = SHFindSymInExe (hexe, &sstr, fCaseSensitive);
    }

    if (!hsym) {
        hexe = hexeNull;

        while (!hsym && (hexe = SHGetNextExe (hexe))) {
            hsym = SHFindSymInExe (hexe, &sstr, fCaseSensitive);
        }
    }

    if (hsym) {
        // Package up the symbol and send it back

        switch (((SYMPTR) hsym)->rectyp) {
            case S_REGISTER:
                lpasr->ast  = astRegister;
                lpasr->ireg = ((REGPTR) hsym)->reg;
                break;

            case S_BPREL16:
                lpasr->ast = astBaseOff;
                lpasr->off = (LONG) ((BPRELPTR16) hsym)->off;
                break;

            case S_BPREL32:
                lpasr->ast = astBaseOff;
                lpasr->off = ((BPRELPTR32) hsym)->off;
                break;

            case S_REGREL32:
               lpasr->ast = astBaseOff;
               lpasr->off = ((LPREGREL32) hsym)->off;
               break;

            case S_LDATA16:
            case S_LDATA32:
            case S_LTHREAD32:
                lpasr->fcd = fcdData;
                goto setaddress;

            case S_GPROC16:
            case S_LPROC16:

                lpasr->fcd =
                    (((PROCPTR16) hsym)->flags.CV_PFLAG_FAR) ?
                    fcdFar : fcdNear;
                goto setaddress;

            case S_GPROC32:
            case S_LPROC32:

                lpasr->fcd =
                    (((PROCPTR32) hsym)->flags.CV_PFLAG_FAR) ?
                    fcdFar : fcdNear;
                goto setaddress;

            case S_GPROCMIPS:
            case S_LPROCMIPS:
               lpasr->fcd = fcdNear;
               goto setaddress;

            case S_GPROCIA64:
            case S_LPROCIA64:
               lpasr->fcd = fcdNear;
               goto setaddress;

            case S_LABEL16:
            case S_THUNK16:
            case S_WITH16:
            case S_PUB16:
            case S_LABEL32:
            case S_THUNK32:
            case S_WITH32:

            case S_PUB32:
                lpasr->fcd = fcdUnknown;
setaddress:
                lpasr->ast = astAddress;
                SHAddrFromHsym (&lpasr->addr, hsym);
                emiAddr (lpasr->addr) = (HEMI) hexe;
                lpasr->addr.mode.fIsLI = TRUE;
                SYFixupAddr (&lpasr->addr);
                break;

            default:
                hsym = NULL;
                break;
        }
    }

    if (hsym) {
        return TRUE;
    } else {
        // We didn't find anything so return false
        lpasr->ast = astNone;

        return FALSE;
    }
}

void
SetAddrFromMod(
    LPMDS lpmds,
    UNALIGNED ADDR* paddr
    )
{
    if (lpmds->pmod) {
        ISECT isect;
        OFF off;
        BOOL fTmp = ModQuerySecContrib(lpmds->pmod, &isect, &off, NULL, NULL);
        assert(fTmp);
        SetAddrSeg (paddr, isect);
        SE_SetAddrOff (paddr, off);
    } else {
        SetAddrSeg (paddr, lpmds->lpsgc[0].seg);
        SE_SetAddrOff (paddr, lpmds->lpsgc[0].off);
    }
}

LPVOID
SHGetDebugData(
    HEXE hexe
    )
{
    // UNDONE: Why make the copy the debugData?  BryanT - Still to resolve.

    LPDEBUGDATA lpd = NULL;

    if (hexe) {
        LPEXE lpexe = (LPEXE) LLLock(hexe);

        //
        //  If symbols are not yet loaded -- force them to load now
        //

        SHWantSymbols(hexe);

        //

        if (!lpexe->pDebugData) {

            HEXG        hexg;
            LPEXG       lpexg;

            hexg = ((LPEXE)LLLock(hexe))->hexg;
            assert(hexg);

            lpexg = (LPEXG)LLLock(hexg);

            // This is the first time the debug data was requested.  Fix it up appropriately
            if (lpexe->LoadAddress == lpexg->LoadAddress) {
                //
                // The address for the first image is the same as the load address.
                // Nothing else to do.
                //
                lpexe->pDebugData = &lpexg->debugData;

            } else {

                if (lpexg->fIsRisc) {

                    DWORD size;
                    DWORD dwSizRTFEntry;

                    switch (lpexg->machine) {
                        case IMAGE_FILE_MACHINE_ALPHA:
                            dwSizRTFEntry = sizeof(IMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY);
                            break;

                        case IMAGE_FILE_MACHINE_ALPHA64:
                            dwSizRTFEntry = sizeof(IMAGE_AXP64_RUNTIME_FUNCTION_ENTRY);
                            break;

                        case IMAGE_FILE_MACHINE_IA64:
                            dwSizRTFEntry = sizeof(IMAGE_IA64_RUNTIME_FUNCTION_ENTRY);
                            break;
                        
                        default:
                            assert(!"Unsupported platform");
                            break;
                    }

                    lpexe->pDebugData = (LPDEBUGDATA) MHAlloc(sizeof(DEBUGDATA));
                    *lpexe->pDebugData = lpexg->debugData;
                    size = lpexe->pDebugData->cRtf * dwSizRTFEntry;;
                    lpexe->pDebugData->lpRtf = MHAlloc(size);
                    memmove(lpexe->pDebugData->lpRtf, lpexg->debugData.lpRtf, size);

                    //
                    // Fix up any pdata for functions that have moved.
                    //
                    ULONG   index;
                    UOFFSET diff = lpexe->LoadAddress - lpexg->LoadAddress;
                    PVOID rf = lpexe->pDebugData->lpRtf;

                    lpexe->pDebugData->lpOriginalRtf = (PBYTE)lpexe->pDebugData->lpOriginalRtf + diff;

                    switch (lpexg->machine) {
                        case IMAGE_FILE_MACHINE_ALPHA:
                            for (index=0; index < lpexe->pDebugData->cRtf; index++) {
                                ((PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY)rf)[index].BeginAddress += (DWORD)diff;
                                ((PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY)rf)[index].EndAddress += (DWORD)diff;
                                ((PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY)rf)[index].PrologEndAddress += (DWORD)diff;
                            }
                            break;

                        case IMAGE_FILE_MACHINE_ALPHA64:
                            for (index=0; index < lpexe->pDebugData->cRtf; index++) {
                                ((PIMAGE_AXP64_RUNTIME_FUNCTION_ENTRY)rf)[index].BeginAddress += diff;
                                ((PIMAGE_AXP64_RUNTIME_FUNCTION_ENTRY)rf)[index].EndAddress += diff;
                                ((PIMAGE_AXP64_RUNTIME_FUNCTION_ENTRY)rf)[index].PrologEndAddress += diff;
                            }
                            break;

                        case IMAGE_FILE_MACHINE_IA64:
                            break;

                        default:
                            assert(!"Unsupported platform");
                            break;
                    }
                
                } else {
                    lpexe->pDebugData = &lpexg->debugData;
                }
            }

            //
            // Make sure the machine field is set correctly
            //
            lpexe->pDebugData->machine = lpexg->machine;

            LLUnlock(hexg);
        }

        //lpd = (LPDEBUGDATA) MHAlloc(sizeof (*lpd));
        //assert(lpd);
        //if (lpd != NULL) {
           //*lpd = *lpexe->pDebugData;
        //}
        lpd = lpexe->pDebugData;
        LLUnlock(hexe);
    }

    return (LPVOID)lpd;
}

BOOL
SHIsThunk(
    HSYM hsym
    )
{
    SYMPTR psym = (SYMPTR) hsym;
    return (psym->rectyp == S_THUNK16 || psym->rectyp == S_THUNK32);
}


#if 0
// VC 4.1 shsymbol.c version.  Changed to 2 passes to support long file names
// Merge into the above code once you understand the implications...

// BryanT 1-19-96

/*** SHGethExeFromName
*
*   Purpose: To get an Exe handle given a name, or partial name
*
*   Input:
*       szPath  - The path or filename of the exe
*
*   Output:
*
*   Returns:
*           A handle to the exe or NULL on error
*
*   Exceptions:
*
*   Notes:
*
*************************************************************************/
HEXE LOADDS PASCAL SHGethExeFromName ( LPTSTR ltszPath ) {
        HEXE    hexe;
        HEXE    hexeEnd;
        HEXE    hexeMatch = (HEXE)NULL;
        CHAR *  szOMFPath;
        CHAR    szOMFFile[_MAX_CVFNAME];
        CHAR    szOMFExt[_MAX_CVEXT];
        CHAR    szName[_MAX_CVPATH];
        CHAR    szFile[_MAX_CVFNAME];
        CHAR    szExt[_MAX_CVEXT];
        int     iNameEnd;
        LPTSTR  lptchEnd = NULL;
        int             iPass;
        WIN32_FIND_DATA wfd;

        // get the start of the exe list, or return an error if we can't get one
    if( !ltszPath || !(*ltszPath) ||
      !(hexe = hexeEnd = SHGetNextExe ( (HEXE)NULL )) ) {
                return( (HEXE)NULL );
        }

    /*
         *      Does this module come with a file handle attached to it?  If so,
     *  copy the path into the buffer where the path is to go and get
     *  the full path name for the file.
     */

    if (*ltszPath == '|') {
                ltszPath++;
                lptchEnd = _ftcschr(ltszPath, '|');
                assert(lptchEnd);
                if (lptchEnd)
                        *lptchEnd = '\0';
    }

        // split it to the root name and extension

    SHSplitPath ( ltszPath, NULL, NULL, szFile, szExt );
        if ( !szExt[0]  ||      !szExt[1] ) {
                szExt[0] = '\0';
        }

        // we haven't yet determined the full path of the input name
        szName[0] = '\0';

        // Make two passes thru the exes - the second pass checks for matching
        // alternate (long vs 8.3) names
        for (iPass=0; iPass < 2; iPass++)
        {
                if (iPass == 1)
                {
                        if ( !_tfullpath ( szName, ltszPath, sizeof ( szName ) ) ) {
                                return( (HEXE)NULL );
                        }
                        if (FindFirstFile(szName, &wfd) != INVALID_HANDLE_VALUE)
                        {
                                // If the long and short names are identical, don't bother with
                                // this extra pass
                                if ( !_ftcsicmp ( wfd.cFileName, wfd.cAlternateFileName )) {
                                        goto exit;
                                }
                                SHSplitPath( wfd.cFileName, NULL, NULL, szFile, szExt);
                        }
                        else
                        {
                                // can't just return NULL here (hexeMatch may be set)
                                goto exit;
                        }
                        // Reset the iteration mechanism to the first exe
                        hexe = SHGetNextExe ( (HEXE)NULL );
                }

                do {

                        // get the full exe name
                        // WARNING: this assumes pointers are the same as handles!!!
                        szOMFPath = SHGetExeName( hexe );

                        // get the extension
                        SHSplitPath ( szOMFPath, NULL, NULL, szOMFFile, szOMFExt );

                        // check for match
                        if ( !_ftcsicmp ( szOMFFile, szFile ) &&
                                !_ftcsicmp ( szOMFExt, szExt )
                        ) {

                                // if we haven't done _tfullpath yet, do it now
                                if (szName[0] == '\0') {
                                        if ( !_tfullpath ( szName, ltszPath, sizeof ( szName ) ) ) {
                                                return( (HEXE)NULL );
                                        }

                                        iNameEnd = _ftcslen(szName);
                                        if( *_ftcsdec( szName, &szName[iNameEnd] ) == '.' ) {
                                           szName[--iNameEnd] = '\0';
                                        }
                                }

                                // check for exact match, need the full path, but we know
                                // exe names are stored as full paths so szOMFPath is a full path

                                // if no extension, put the current extension on

                                if ( !szExt[0] && szOMFExt[0] ) {
                                        _ftcscpy(szName + iNameEnd, szOMFExt);
                                }

                                // see if these are the same

                                if ( !_ftcsicmp( szOMFPath, szName ) ) {
                                        hexeMatch = hexe;
                                        goto exit;
                                }

                                if ( !szExt[0] ) {
                                        szName[iNameEnd] = '\0';
                                }

                                // save away the first potential match
                                if( !hexeMatch ) {
                                        hexeMatch = hexe;
                                }
                        }
                } while ( hexe = SHGetNextExe ( hexe ) );
        }

exit:
        // restore '|'
        if (lptchEnd)
                *lptchEnd = '|';

        return(hexeMatch);
}

#endif
