//  SHsymlb0.c - general library routines to find an omff symbol by name or address.
//
//  Copyright <C> 1988-94, Microsoft Corporation
//
//  Purpose: To supply a concise interface to the debug omf for symbols
//
//      10-Nov-94 BryanT
//          Merge in NT changes.
//          SHIsAddrInMod -> IsAddrInMod
//          Delete SHszDir/SHszDrive/SHszDebuggeeDrive/SHszDebuggeeDir.  Not used
//          Make SHSetDebuggeeDir a shell since there is no usage for the dir.
//          Delete special case Mac targeting S_CEXMODEL32 handling.
//          Remove ems code from SHpSymlplLabLoc.
//          Add support for S_THUNK16/S_THUNK32 in SHpSymlplLabLoc.
//          Replace local statics with a Cache entry.
//          Remove SHModelFromCxt (no callers and not exposed).

#include "shinc.hpp"
#pragma hdrstop

//
//  fundamental source line lookup routines
//

VOID
SHSetDebuggeeDir (
    LSZ lszDir
    )
{
    // Functionality not used any longer.
    return;
}


//  SHpSymlplLabLoc
//
//  Purpose: To completely fill in a plpl pkt. The hmod and addr must already
//           be valid. The locals and labels are searched based on paddr. The
//           whole module is search for now. Better decisions may be made in the
//           future.
//
//
//  Input:
//      plpl    - lpl packet with a valid module and address in it.
//
//  Output:
//      plpl    - Is updated with Proc, Local, and Label.
//
// Notes: This includes locals and lables

VOID
SHpSymlplLabLoc (
    LPLBS lplbs
    )
{
    SYMPTR      lpSym = NULL;
    SYMPTR      lpSymEnd;
    LPMDS       lpmds;
    ULONG       cbMod = 0;
    CV_uoff32_t obModelMin = 0;
    CV_uoff32_t obModelMax = CV_MAXOFFSET;
    CV_uoff32_t obTarget;
    CV_uoff32_t doffNew;
    CV_uoff32_t doffOld;

    // for now we are doing the whole module

    lplbs->tagLoc   = NULL;
    lplbs->tagLab   = NULL;
    lplbs->tagProc  = NULL;
    lplbs->tagThunk = NULL;
    lplbs->tagModelMin = NULL;
    lplbs->tagModelMax = NULL;

    if (!lplbs->tagMod) {
        return;
    }

    // because segments of locals don't have to match the segment of the
    // searched module, check segment here is wrong. However we can set
    // a flag up for proc and labels

    lpmds    = (LPMDS) lplbs->tagMod;
    obTarget = (CV_uoff32_t)GetAddrOff (lplbs->addr);

    // add/subtract the size of the hash table ptr

    lpSym = (SYMPTR) ((LPB) GetSymbols (lpmds) + sizeof(long));
    cbMod    = lpmds->cbSymbols;
    lpSymEnd = (SYMPTR) ((BYTE *) lpSym + cbMod - sizeof (long));

    while(lpSym < lpSymEnd) {

        switch(lpSym->rectyp) {
            case S_CEXMODEL16:
                if (((WORD)(((CEXMPTR16)lpSym)->seg) == (WORD)GetAddrSeg (lplbs->addr))) {
                    CV_uoff32_t obTemp = (CV_uoff32_t)(((CEXMPTR16)lpSym)->off);
                    if (obTemp <= obModelMax) {
                        if (obTemp > obTarget) {
                            lplbs->tagModelMax = (CEXMPTR16)lpSym;
                            obModelMax = obTemp;
                        }
                        else if (obTemp >= obModelMin) {
                            lplbs->tagModelMin = (CEXMPTR16)lpSym;
                            obModelMin = obTemp;
                        }
                    }
                }
                break;

            case S_LPROC16:
            case S_GPROC16:
                if (((WORD)(((PROCPTR16)lpSym)->seg) == (WORD)GetAddrSeg (lplbs->addr)) &&
                  ((CV_uoff32_t)(((PROCPTR16)lpSym)->off) <= obTarget) &&
                  (obTarget < ((CV_uoff32_t)(((PROCPTR16)lpSym)->off) + (CV_uoff32_t)(((PROCPTR16)lpSym)->len)))) {
                    lplbs->tagProc = (SYMPTR)lpSym;
                }
            break;

            case S_LABEL16:
                if (((WORD)(((LABELPTR16)lpSym)->seg) == (WORD)GetAddrSeg (lplbs->addr)) &&
                    (((CV_uoff32_t)((LABELPTR16)lpSym)->off) <= obTarget)) {
                    doffNew = obTarget - (CV_uoff32_t)(((LABELPTR16)lpSym)->off);

                    // calculate what the old offset was, this requires no
                    // use of static variables

                    doffOld = obTarget;

                    if (lplbs->tagLab) {
                        doffOld -= (CV_uoff32_t)(((LABELPTR16)lplbs->tagLab)->off);
                    }

                    if (doffNew <= doffOld) {
                        lplbs->tagLab = (SYMPTR)lpSym;
                    }
                }
                break;

            case S_LDATA16:
            case S_GDATA16:
                if (((WORD)(((DATAPTR16)lpSym)->seg) == (WORD)GetAddrSeg (lplbs->addr)) &&
                  ((CV_uoff32_t)(((DATAPTR16)lpSym)->off) <= obTarget)) {
                    doffNew = obTarget - (CV_uoff32_t)(((DATAPTR16)lpSym)->off);

                    // calculate what the old offset was.
                    doffOld = obTarget;

                    if (lplbs->tagLoc) {
                        doffOld -= (CV_uoff32_t)(((DATAPTR16)lplbs->tagLoc)->off);
                    }

                    if (doffNew <= doffOld) {
                        lplbs->tagLoc = (SYMPTR) lpSym;
                    }
                }
                break;

            case S_THUNK16:
                if (((WORD)(((THUNKPTR16)lpSym)->seg) == (WORD)GetAddrSeg (lplbs->addr)) &&
                    ((CV_uoff32_t)(((THUNKPTR16)lpSym)->off) <= obTarget) &&
                    (obTarget < ((CV_uoff32_t)(((THUNKPTR16)lpSym)->off) + (CV_uoff32_t)(((THUNKPTR16)lpSym)->len)))) {
                    lplbs->tagThunk = (SYMPTR)lpSym;
                }
                break;

            case S_CEXMODEL32:
                if (((WORD)(((CEXMPTR32)lpSym)->seg) == (WORD)GetAddrSeg (lplbs->addr))) {
                    CV_uoff32_t obTemp = (CV_uoff32_t)(((CEXMPTR32)lpSym)->off);
                    if (obTemp <= obModelMax) {
                        if (obTemp > obTarget) {
                            lplbs->tagModelMax = (CEXMPTR16)(CEXMPTR32)lpSym;
                            obModelMax = obTemp;
                        }
                        else if (obTemp >= obModelMin) {
                            lplbs->tagModelMin = (CEXMPTR16)(CEXMPTR32)lpSym;
                            obModelMin = obTemp;
                        }
                    }
                }
                break;

            case S_LPROC32:
            case S_GPROC32:
                if (((WORD)(((PROCPTR32)lpSym)->seg) == (WORD)GetAddrSeg (lplbs->addr)) &&
                  ((CV_uoff32_t)(((PROCPTR32)lpSym)->off) <= obTarget) &&
                  (obTarget < ((CV_uoff32_t)(((PROCPTR32)lpSym)->off) + (CV_uoff32_t)(((PROCPTR32)lpSym)->len)))) {
                    lplbs->tagProc = (SYMPTR)lpSym;
                }
                break;

            case S_LPROCMIPS:
            case S_GPROCMIPS:
                if (((WORD)(((PROCPTRMIPS)lpSym)->seg) == (WORD)GetAddrSeg (lplbs->addr)) &&
                  ((CV_uoff32_t)(((PROCPTRMIPS)lpSym)->off) <= obTarget) &&
                  (obTarget < ((CV_uoff32_t)(((PROCPTRMIPS)lpSym)->off) + (CV_uoff32_t)(((PROCPTRMIPS)lpSym)->len)))) {
                    lplbs->tagProc = (SYMPTR)lpSym;
                }
                break;

            case S_LPROCIA64:
            case S_GPROCIA64:
                if (((WORD)(((PROCPTRIA64)lpSym)->seg) == (WORD)GetAddrSeg (lplbs->addr)) &&
                  ((CV_uoff32_t)(((PROCPTRIA64)lpSym)->off) <= obTarget) &&
                  (obTarget < ((CV_uoff32_t)(((PROCPTRIA64)lpSym)->off) + (CV_uoff32_t)(((PROCPTRIA64)lpSym)->len)))) {
                    lplbs->tagProc = (SYMPTR)lpSym;
                }
                break;

            case S_LABEL32:
                if (((WORD)(((LABELPTR32)lpSym)->seg) == (WORD)GetAddrSeg (lplbs->addr)) &&
                    (((CV_uoff32_t)((LABELPTR32)lpSym)->off) <= obTarget)) {
                    doffNew = obTarget - (CV_uoff32_t)(((LABELPTR32)lpSym)->off);

                    // calculate what the old offset was, this requires no
                    // use of static variables

                    doffOld = obTarget;

                    if (lplbs->tagLab) {
                        doffOld -= (CV_uoff32_t)(((LABELPTR32)lplbs->tagLab)->off);
                    }

                    if (doffNew <= doffOld) {
                        lplbs->tagLab = (SYMPTR)lpSym;
                    }
                }
                break;

            case S_LDATA32:
            case S_GDATA32:
            case S_LTHREAD32:
            case S_GTHREAD32:
                if (((WORD)(((DATAPTR32)lpSym)->seg) == (WORD)GetAddrSeg (lplbs->addr)) &&
                  ((CV_uoff32_t)(((DATAPTR32)lpSym)->off) <= obTarget)) {
                    doffNew = obTarget - (CV_uoff32_t)(((DATAPTR32)lpSym)->off);

                    // calculate what the old offset was.
                    doffOld = obTarget;

                    if (lplbs->tagLoc) {
                        doffOld -= (CV_uoff32_t)(((DATAPTR32)lplbs->tagLoc)->off);
                    }

                    if (doffNew <= doffOld) {
                        lplbs->tagLoc = (SYMPTR) lpSym;
                    }
                }
                break;

            case S_THUNK32:
                if (((WORD)(((THUNKPTR32)lpSym)->seg) == (WORD)GetAddrSeg (lplbs->addr)) &&
                    ((CV_uoff32_t)(((THUNKPTR32)lpSym)->off) <= obTarget) &&
                    (obTarget < ((CV_uoff32_t)(((THUNKPTR32)lpSym)->off) + (CV_uoff32_t)(((THUNKPTR32)lpSym)->len)))) {
                    lplbs->tagThunk = (SYMPTR)lpSym;
                }
                break;

        }
        lpSym = NEXTSYM (SYMPTR, lpSym);
    }
}


//  SHdNearestSymbol
//
//  Purpose: To find the closest label/proc to the specified address is
//          found and put in pch. Both the symbol table and the
//          publics tables are searched.
//
//  Input:
//      ptxt -  a pointer to the context, address and mdi must
//              be filled in.
//
//      sop  -  Determine what type of symbols to look for
//
//  Notes: If CV_MAXOFFSET is returned in the lpodr, there is no closest
//          symbol Also all symbols in the module are searched so only the
//          cxt.addr and cxt.mdi have meaning.

VOID
SHdNearestSymbol (
    PCXT pcxt,
    SOP sop,
    LPODR lpodr
    )
{
    HSYM    hSym;
    SYMPTR  pSym;
    LBS     lbs;
    ULONG   doff = CV_MAXOFFSET;
    ULONG   doffNew = CV_MAXOFFSET;
    LPCH    lpch = lpodr->lszName;

    lpodr->fst = fstNone;
    lpodr->fcd = fcdUnknown;
    lpodr->fpt = fptUnknown;
    lpodr->cbProlog = 0;
    lpodr->dwDeltaOff = 0;

    *lpch = '\0';
    if (SHHMODFrompCXT (pcxt)) {
        BOOL bAddrInProc = FALSE;

        // at some point we may wish to specify only a scope to search for
        // a label. So we may wish to initialize the lbs differently

        // get the Labels
        lbs.tagMod = SHHMODFrompCXT (pcxt);
        lbs.addr   = *SHpADDRFrompCXT (pcxt);
        SHpSymlplLabLoc (&lbs);

        // check for closest data local, if requested
        if ((sop & sopData)  &&  lbs.tagLoc) {
            pSym = (SYMPTR) lbs.tagLoc;
            switch (pSym->rectyp) {
                case S_LDATA16:
                case S_GDATA16:
                    doff = (ULONG)(GetAddrOff (lbs.addr) -
                      (CV_uoff32_t)(((DATAPTR16)pSym)->off));
                    _tcsncpy (lpch,
                              (char *)&((DATAPTR16)pSym)->name[1],
                              (BYTE)(*((DATAPTR16)pSym)->name));
                    lpch[(BYTE)(*((DATAPTR16)pSym)->name)] = '\0';
                    break;

                case S_LDATA32:
                case S_GDATA32:
                case S_LTHREAD32:
                case S_GTHREAD32:
                    doff = (ULONG)(GetAddrOff (lbs.addr) -
                      (CV_uoff32_t)(((DATAPTR32)pSym)->off));
                    _tcsncpy (lpch,
                              (char *)&((DATAPTR32)pSym)->name[1],
                              (BYTE)(*((DATAPTR32)pSym)->name));
                    lpch[(BYTE)(*((DATAPTR32)pSym)->name)] = '\0';
                    break;
            }
        }

        // check for closest label
        if (!(sop & sopFcn) && lbs.tagLab) {
            pSym = (SYMPTR) lbs.tagLab;
            switch (pSym->rectyp) {
                case S_LABEL16:
                    doff = (ULONG)(GetAddrOff (lbs.addr) -
                      (CV_uoff32_t)(((LABELPTR16)pSym)->off) );
                    _tcsncpy (lpch,
                              (char *)&((LABELPTR16)pSym)->name[1],
                              (BYTE)(*((LABELPTR16)pSym)->name));
                    lpch[(BYTE)(*((LABELPTR16)pSym)->name)] = '\0';
                    break;

                case S_LABEL32:
                    doff = (ULONG)(GetAddrOff (lbs.addr) -
                      (CV_uoff32_t)(((LABELPTR32)pSym)->off) );
                    _tcsncpy (lpch,
                              (char *)&((LABELPTR32)pSym)->name[1],
                              (BYTE)(*((LABELPTR32)pSym)->name));
                    lpch[(BYTE)(*((LABELPTR32)pSym)->name)] = '\0';
                    break;
            }
        }

        // if the proc name is closer
        if (lbs.tagProc) {
            pSym = (SYMPTR) lbs.tagProc;
            switch (pSym->rectyp) {
                case S_LPROC16:
                case S_GPROC16:
                    doffNew = (ULONG)(GetAddrOff (lbs.addr) -
                                (CV_uoff32_t)(((PROCPTR16)pSym)->off));
                    if (doffNew <= doff) {
                        doff = doffNew;
                        _tcsncpy (lpch,
                                  (char *)&((PROCPTR16)pSym)->name[1],
                                  (BYTE)(*((PROCPTR16)pSym)->name));
                        lpch[(BYTE)(*((PROCPTR16)pSym)->name)] = '\0';
                        lpodr->cbProlog = ((PROCPTR16)pSym)->DbgStart - 1;
                        lpodr->fcd = (((PROCPTR16)pSym)->flags.CV_PFLAG_FAR) ? fcdFar : fcdNear;
                        lpodr->fst = fstSymbol;

                        if ( doff < (CV_uoff32_t)((PROCPTR16)pSym)->len ) {
                            bAddrInProc = TRUE;
                        }
                    }
                    break;

                case S_LPROC32:
                case S_GPROC32:
                    doffNew = (ULONG)(GetAddrOff (lbs.addr) -
                                (CV_uoff32_t)(((PROCPTR32)pSym)->off));
                    if (doffNew <= doff) {
                        doff = doffNew;
                        _tcsncpy (lpch,
                                  (char *)&((PROCPTR32)pSym)->name[1],
                                  (BYTE)(*((PROCPTR32)pSym)->name));
                        lpch[(BYTE)(*((PROCPTR32)pSym)->name)] = '\0';

                        // cbProlog is a WORD, so until we change that, we'll
                        // have to make sure the prolog is <64K (a safe bet)

                        assert (((PROCPTR32)pSym)->DbgStart <= 65535);
                        lpodr->cbProlog = (WORD)(((PROCPTR32)pSym)->DbgStart);

                        lpodr->fcd = (((PROCPTR32)pSym)->flags.CV_PFLAG_FAR) ? fcdFar : fcdNear;
                        lpodr->fst = fstSymbol;
                        if (((PROCPTR32)pSym)->flags.CV_PFLAG_NOFPO ) {
                            lpodr->fpt = fptPresent;
                        }
                        if ( doff < (CV_uoff32_t)((PROCPTR32)pSym)->len ) {
                            bAddrInProc = TRUE;
                        }
                    }
                    break;

                case S_LPROCMIPS:
                case S_GPROCMIPS:
                    doffNew = (ULONG)(GetAddrOff (lbs.addr) -
                                (CV_uoff32_t)(((PROCPTRMIPS)pSym)->off));
                    if (doffNew <= doff) {
                        doff = doffNew;
                        _tcsncpy (lpch,
                                  (char *)&((PROCPTRMIPS)pSym)->name[1],
                                  (BYTE)(*((PROCPTRMIPS)pSym)->name));
                        lpch[(BYTE)(*((PROCPTRMIPS)pSym)->name)] = '\0';

                        // cbProlog is a WORD, so until we change that, we'll
                        // have to make sure the prolog is <64K (a safe bet)
                        if (((PROCPTRMIPS)pSym)->DbgStart == 0) {
                            lpodr->cbProlog = 0;
                        } else {
                            // TEMPORARY HACK !!!!!!! - sanjays
                            assert (((PROCPTRMIPS)pSym)->DbgStart - 1 <= 65535);
                            lpodr->cbProlog = (WORD)(((PROCPTRMIPS)pSym)->DbgStart - 1);
                        }

                        lpodr->fcd = fcdNear;
                        lpodr->fst = fstSymbol;

                        if ( doff < (CV_uoff32_t)((PROCPTRMIPS)pSym)->len ) {
                            bAddrInProc = TRUE;
                        }
                    }
                    break;

                case S_LPROCIA64:
                case S_GPROCIA64:
                    doffNew = (ULONG)(GetAddrOff (lbs.addr) -
                                (CV_uoff32_t)(((PROCPTRIA64)pSym)->off));
                    if (doffNew <= doff) {
                        doff = doffNew;
                        _tcsncpy (lpch,
                                  (char *)&((PROCPTRIA64)pSym)->name[1],
                                  (BYTE)(*((PROCPTRIA64)pSym)->name));
                        lpch[(BYTE)(*((PROCPTRIA64)pSym)->name)] = '\0';

                        // cbProlog is a WORD, so until we change that, we'll
                        // have to make sure the prolog is <64K (a safe bet)

                        assert (((PROCPTRIA64)pSym)->DbgStart <= 65535);
                        lpodr->cbProlog = (WORD)(((PROCPTRIA64)pSym)->DbgStart);

                        lpodr->fcd = (((PROCPTRIA64)pSym)->flags.CV_PFLAG_FAR) ? fcdFar : fcdNear;
                        lpodr->fst = fstSymbol;
                        if (((PROCPTRIA64)pSym)->flags.CV_PFLAG_NOFPO ) {
                            lpodr->fpt = fptPresent;
                        }
                        if ( doff < (CV_uoff32_t)((PROCPTRIA64)pSym)->len ) {
                            bAddrInProc = TRUE;
                        }
                    }
                    break;
            }
        }

        if (!doff) {
            lpodr->dwDeltaOff = 0;  // Exact Match
            return;
        }

        // Avoid searching the publics if the address we were searching for
        // is in the range of the proc we found.
        if ( bAddrInProc && !(sop & sopData))
        {
            lpodr->dwDeltaOff = doff;
            return;
        }
    }

    // now check the publics

    doffNew = PHGetNearestHsym (SHpADDRFrompCXT (pcxt), (HEXE) SHpADDRFrompCXT(pcxt)->emi, &hSym);

    if (doffNew < doff) {
        doff = doffNew;
        pSym = (SYMPTR) hSym;
        switch (pSym->rectyp) {
            case S_GDATA16:
            case S_PUB16:
                _tcsncpy (lpch,
                          (char *)&((DATAPTR16)pSym)->name[1],
                          (BYTE)(*((DATAPTR16)pSym)->name));
                lpch[(BYTE)(*((DATAPTR16)pSym)->name)] = '\0';
                lpodr->fst = fstPublic;
                break;

            case S_GDATA32:
            case S_PUB32:
                _tcsncpy (lpch,
                          (char *)&((DATAPTR32)pSym)->name[1],
                          (BYTE)(*((DATAPTR32)pSym)->name));
                lpch[(BYTE)(*((DATAPTR32)pSym)->name)] = '\0';
                lpodr->fst = fstPublic;
                break;
        }
    }

    lpodr->dwDeltaOff = doff;
    return;
}

// the next function is provided to osdebug via callbacks and
//  should not be called within the CV kernel

//  SHModelFromAddr
//
//  Purpose: To fill the supplied buffer with the relevant Change
//       Execution Model record from the symbols section.
//
//  Input:
//      pcxt    -   a pointer to an addr,
//
//  Output:
//      pch     -   The Change Execution Model record is copied here.
//
//  Returns
//      True if there is symbol information for the module.
//
//  Notes:  If there is no symbol information for the module, the supplied
//          buffer is not changed and the function returns FALSE.

// UNDONE: The statics in this function s/b moved to a CACHE struct.  Better yet,
// it, simply test for mac targetting and return native if not.

int
SHModelFromAddr (
    LPADDR paddr,
    LPW lpwModel,
    LPB lpbModel,
    UOFFSET *pobMax
    )
{
    static CEXMPTR16    tagOld;
    static CV_uoff32_t  obMax = 0;
    static CV_uoff32_t  obMin = 0;
    static HEMI         emiOld = 0;
    static WORD         segOld = 0;

    SYMPTR *lppModel = (SYMPTR *) lpbModel;
    LBS   lbs;
    ADDR  addr;
    LPMDS lpmds;
    HMOD  hmod;
    CXT   cxt = {0};
    CB cbSecContrib;
    BOOL fTmp;

    // if physical, unfix it up
    if (!ADDR_IS_LI (*paddr)) {
        SYUnFixupAddr (paddr);
    }

    cxt.addr = *paddr;
    cxt.hMod = 0;

    if ((segOld != (WORD) GetAddrSeg (*SHpADDRFrompCXT(&cxt))) ||
        (emiOld !=  emiAddr (*SHpADDRFrompCXT(&cxt)))          ||
        (GetAddrOff (*SHpADDRFrompCXT(&cxt)) >= obMax)         ||
        (GetAddrOff (*SHpADDRFrompCXT(&cxt)) < obMin))
    {
        if (!SHHMODFrompCXT (&cxt)) {
            addr = *SHpADDRFrompCXT (&cxt);
            memset(&cxt, 0, sizeof(CXT));
            if (!SHSetCxtMod(&addr, &cxt)) {
                return FALSE;
            }
        }

        hmod = (HMOD)SHHGRPFrompCXT(&cxt);
        if (!hmod) {
            return FALSE;
        }
        lpmds = (LPMDS) hmod;
        emiOld = emiAddr (*SHpADDRFrompCXT(&cxt));
        fTmp = IsAddrInMod (lpmds, &cxt.addr, &segOld, (OFF *)&obMin, &cbSecContrib);
        assert(fTmp);
        obMax = obMin + cbSecContrib + 1;
        tagOld = NULL;

        // at some point we may wish to specify only a scope to search for
        // a label. So we may wish to initialize the lbs differently

        // get the Relevant change model records

        if (GetSymbols ((LPMDS) (lbs.tagMod = SHHMODFrompCXT (&cxt)))) {
            lbs.addr   = *SHpADDRFrompCXT(&cxt);
            SHpSymlplLabLoc (&lbs);
            if (tagOld = lbs.tagModelMin) {
                if (((SYMPTR)(lbs.tagModelMin))->rectyp == S_CEXMODEL32) {
                    obMin = ((CEXMPTR32)(lbs.tagModelMin))->off;
                } else {
                    obMin = (lbs.tagModelMin)->off;
                }
            }
            if (lbs.tagModelMax) {
                if (((SYMPTR)(lbs.tagModelMax))->rectyp == S_CEXMODEL32) {
                    obMax = ((CEXMPTR32)(lbs.tagModelMax))->off;
                } else {
                    obMax = (lbs.tagModelMax)->off;
                }
            }
        }
    }

    if (tagOld != NULL) {
        // pass on ptr to the SYM
        *lppModel = (SYMPTR) tagOld;
        if (((SYMPTR)tagOld)->rectyp == S_CEXMODEL32) {
            *lpwModel = ((CEXMPTR32) *lppModel) -> model;
        } else {
            *lpwModel = ((CEXMPTR16) *lppModel) -> model;
        }

        if (*lpwModel != CEXM_MDL_cobol
            && *lpwModel != CEXM_MDL_pcode32Mac
            && *lpwModel != CEXM_MDL_pcode32MacNep
            ) {
            *lpwModel &= 0xfff0;
        }
    } else {
        // no model record, must be native
        *lppModel = NULL;
        *lpwModel = CEXM_MDL_native;
    }
    *pobMax = obMax;
    return TRUE;
}
