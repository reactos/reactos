/****************************** Module Header ******************************\
* Module Name: hdata.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* DDE Manager data handle functions
*
* Created: 11/12/91 Sanford Staab
*
\***************************************************************************/

#define DDEMLDB
#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* DdeCreateDataHandle (DDEML API)
*
* Description
*
* History:
* 11-1-91 sanfords Created.
\***************************************************************************/
HDDEDATA DdeCreateDataHandle(
DWORD idInst,
LPBYTE pSrc,
DWORD cb,
DWORD cbOff,
HSZ hszItem,
UINT wFmt,
UINT afCmd)
{
    PCL_INSTANCE_INFO pcii;
    HDDEDATA hRet = 0;

    if (cb == -1) {
        RIPMSG0(RIP_ERROR, "DdeCreateDataHandle called with cb == -1\n");
        return NULL;
    }

    EnterDDECrit;

    pcii = ValidateInstance((HANDLE)LongToHandle( idInst ));
    if (pcii == NULL) {
        BestSetLastDDEMLError(DMLERR_INVALIDPARAMETER);
        goto Exit;
    }
    if (afCmd & ~HDATA_APPOWNED) {
        SetLastDDEMLError(pcii, DMLERR_INVALIDPARAMETER);
        goto Exit;
    }

    if (cb + cbOff < sizeof(DWORD) && pSrc == NULL &&
            (wFmt == CF_METAFILEPICT ||
             wFmt == CF_DSPMETAFILEPICT ||
             wFmt == CF_DIB ||
             wFmt == CF_BITMAP ||
             wFmt == CF_DSPBITMAP ||
             wFmt == CF_PALETTE ||
             wFmt == CF_ENHMETAFILE ||
             wFmt == CF_DSPENHMETAFILE)) {
        /*
         * We have the nasty possibility of blowing up in FreeDDEData if we
         * don't initialize the data for formats with indirect data to 0.
         * This is because GlobalLock/GlobalSize do not adequately validate
         * random numbers given to them.
         */
        cb += 4;
    }
    hRet = InternalCreateDataHandle(pcii, pSrc, cb, cbOff,
            hszItem ? afCmd : (afCmd | HDATA_EXECUTE),
            (WORD)((afCmd & HDATA_APPOWNED) ? 0 : DDE_FRELEASE), (WORD)wFmt);

    if (!hRet) {
        SetLastDDEMLError(pcii, DMLERR_MEMORY_ERROR);
    }
Exit:
    LeaveDDECrit;
    return (hRet);
}


/***************************************************************************\
* InternalCreateDataHandle
*
* Description:
* Worker function for creating a data handle. If cb is -1, pSrc is
* a GMEM_DDESHARE data handle. 0 is return ed on error.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
HDDEDATA InternalCreateDataHandle(
PCL_INSTANCE_INFO pcii,
LPBYTE pSrc,
DWORD cb, // cb of actual data to initialize with
DWORD cbOff, // offset from start of data
DWORD flags,
WORD wStatus,
WORD wFmt)
{
    PDDEMLDATA pdd;
    HDDEDATA hRet;
    LPBYTE p;
    DWORD cbOff2;

    CheckDDECritIn;

    pdd = (PDDEMLDATA)DDEMLAlloc(sizeof(DDEMLDATA));
    if (pdd == NULL) {
        return (0);
    }
    if (cb == -1) {
        pdd->hDDE = (HANDLE)pSrc;
    } else {
        if (flags & HDATA_EXECUTE) {
            cbOff2 = 0;
        } else {
            cbOff2 = sizeof(WORD) + sizeof(WORD); // skip wStatus, wFmt
        }
        pdd->hDDE = UserGlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE | GMEM_ZEROINIT,
                cb + cbOff + cbOff2);
        if (pdd->hDDE == NULL) {
            DDEMLFree(pdd);
            return (0);
        }

        if (!(flags & HDATA_EXECUTE)) {
            PDDE_DATA pdde;

            USERGLOBALLOCK(pdd->hDDE, pdde);
            UserAssert(pdde);
            pdde->wStatus = wStatus;
            pdde->wFmt = wFmt;
            USERGLOBALUNLOCK(pdd->hDDE);
        }
    }
    pdd->flags = (WORD)flags;
    hRet = (HDDEDATA)CreateHandle((ULONG_PTR)pdd, HTYPE_DATA_HANDLE,
            InstFromHandle(pcii->hInstClient));
    if (!hRet) {
        WOWGLOBALFREE(pdd->hDDE);
        DDEMLFree(pdd);
        return (0);
    }
    if (cb != -1 && pSrc != NULL) {
        USERGLOBALLOCK(pdd->hDDE, p);
        UserAssert(p);
        RtlCopyMemory(p + cbOff + cbOff2, pSrc, cb);
        USERGLOBALUNLOCK(pdd->hDDE);
        pdd->flags |= HDATA_INITIALIZED;
    }
    return (hRet);
}

/***************************************************************************\
* DdeAddData (DDEML API)
*
* Description:
* Copys data from a user buffer to a data handles. Reallocates if needed.
*
* History:
* 11-1-91 sanfords Created.
\***************************************************************************/
HDDEDATA DdeAddData(
HDDEDATA hData,
LPBYTE pSrc,
DWORD cb,
DWORD cbOff)
{
    LPSTR pMem;
    PDDEMLDATA pdd;
    PCL_INSTANCE_INFO pcii;
    HDDEDATA hRet = 0;

    EnterDDECrit;

    pdd = (PDDEMLDATA)ValidateCHandle((HANDLE)hData, HTYPE_DATA_HANDLE, HINST_ANY);
    if (pdd == NULL) {
        goto Exit;
    }
    pcii = PciiFromHandle((HANDLE)hData);
    if (pcii == NULL) {
        BestSetLastDDEMLError(DMLERR_INVALIDPARAMETER);
        goto Exit;
    }
    if (!(pdd->flags & HDATA_EXECUTE)) {
        cbOff += 4;
    }
    if (cb + cbOff > UserGlobalSize(pdd->hDDE)) {
        pdd->hDDE = UserGlobalReAlloc(pdd->hDDE, cb + cbOff,
                GMEM_MOVEABLE | GMEM_ZEROINIT);
    }

    USERGLOBALLOCK(pdd->hDDE, pMem);

    if (pMem == NULL) {
        SetLastDDEMLError(pcii, DMLERR_MEMORY_ERROR);
        goto Exit;
    }

    hRet = hData;

    if (pSrc != NULL) {
        try {
            RtlCopyMemory(pMem + cbOff, pSrc, cb);
            pdd->flags |= HDATA_INITIALIZED;
        } except(W32ExceptionHandler(FALSE, RIP_WARNING)) {
            SetLastDDEMLError(pcii, DMLERR_INVALIDPARAMETER);
            hRet = 0;
        }
    }

    USERGLOBALUNLOCK(pdd->hDDE);

Exit:
    LeaveDDECrit;
    return (hRet);
}




/***************************************************************************\
* DdeGetData (DDEML API)
*
* Description:
* Copys data from a data handle into a user buffer.
*
* History:
* 11-1-91 sanfords Created.
\***************************************************************************/
DWORD DdeGetData(
HDDEDATA hData,
LPBYTE pDst,
DWORD cbMax,
DWORD cbOff)
{
    DWORD cbCopied = 0;
    DWORD cbSize;
    PDDEMLDATA pdd;
    PCL_INSTANCE_INFO pcii;

    EnterDDECrit;

    pdd = (PDDEMLDATA)ValidateCHandle((HANDLE)hData,
            HTYPE_DATA_HANDLE, HINST_ANY);
    if (pdd == NULL) {
        BestSetLastDDEMLError(DMLERR_INVALIDPARAMETER);
        goto Exit;
    }
    pcii = PciiFromHandle((HANDLE)hData);
    if (pcii == NULL) {
        BestSetLastDDEMLError(DMLERR_INVALIDPARAMETER);
        goto Exit;
    }
    if (!(pdd->flags & HDATA_EXECUTE)) {
        cbOff += 4;
    }
    cbSize = (DWORD)UserGlobalSize(pdd->hDDE);
    if (cbOff >= cbSize) {
        SetLastDDEMLError(pcii, DMLERR_INVALIDPARAMETER);
        goto Exit;
    }
    if (pDst == NULL) {
        cbCopied = cbSize - cbOff;
        goto Exit;
    } else {
        LPSTR pMem;

        cbCopied = min(cbMax, cbSize - cbOff);
        USERGLOBALLOCK(pdd->hDDE, pMem);
        UserAssert(pMem);
        try {
            RtlCopyMemory(pDst, pMem + cbOff, cbCopied);
        } except(W32ExceptionHandler(FALSE, RIP_WARNING)) {
            SetLastDDEMLError(pcii, DMLERR_INVALIDPARAMETER);
            cbCopied = 0;
        }
        if (pMem != NULL) {
            USERGLOBALUNLOCK(pdd->hDDE);
        }
    }

Exit:
    LeaveDDECrit;
    return (cbCopied);
}





/***************************************************************************\
* DdeAccessData (DDEML API)
*
* Description:
* Locks a data handle for access.
*
* History:
* 11-1-91 sanfords Created.
\***************************************************************************/
LPBYTE DdeAccessData(
HDDEDATA hData,
LPDWORD pcbDataSize)
{
    PCL_INSTANCE_INFO pcii;
    PDDEMLDATA pdd;
    LPBYTE pRet = NULL;
    DWORD cbOff;

    EnterDDECrit;

    pdd = (PDDEMLDATA)ValidateCHandle((HANDLE)hData,
            HTYPE_DATA_HANDLE, HINST_ANY);
    if (pdd == NULL) {
        goto Exit;
    }
    pcii = PciiFromHandle((HANDLE)hData);
    cbOff = pdd->flags & HDATA_EXECUTE ? 0 : 4;
    if (pcbDataSize != NULL) {
        *pcbDataSize = (DWORD)UserGlobalSize(pdd->hDDE) - cbOff;
    }
    USERGLOBALLOCK(pdd->hDDE, pRet);
    UserAssert(pRet);
    pRet = (LPBYTE)pRet + cbOff;

Exit:
    LeaveDDECrit;
    return (pRet);
}




/***************************************************************************\
* DdeUnaccessData (DDEML API)
*
* Description:
* Unlocks a data handle
*
* History:
* 11-1-91 sanfords Created.
\***************************************************************************/
BOOL DdeUnaccessData(
HDDEDATA hData)
{
    PDDEMLDATA pdd;
    BOOL fSuccess = FALSE;

    EnterDDECrit;

    pdd = (PDDEMLDATA)ValidateCHandle((HANDLE)hData,
            HTYPE_DATA_HANDLE, HINST_ANY);
    if (pdd == NULL) {
        goto Exit;
    }
    USERGLOBALUNLOCK(pdd->hDDE);
    fSuccess = TRUE;

Exit:
    LeaveDDECrit;
    return (fSuccess);
}



/***************************************************************************\
* DdeFreeDataHandle (DDEML API)
*
* Description:
* Releases application interest in a data handle.
*
* History:
* 11-1-91 sanfords Created.
\***************************************************************************/
BOOL DdeFreeDataHandle(
HDDEDATA hData)
{
    PDDEMLDATA pdd;
    BOOL fSuccess = FALSE;

    EnterDDECrit;

    pdd = (PDDEMLDATA)ValidateCHandle((HANDLE)hData,
            HTYPE_DATA_HANDLE, HINST_ANY);
    if (pdd == NULL) {
        goto Exit;
    }
    if (pdd->flags & HDATA_NOAPPFREE) {
        fSuccess = TRUE;
        goto Exit;
    }

    fSuccess = InternalFreeDataHandle(hData, TRUE);

Exit:
    LeaveDDECrit;
    return (fSuccess);
}




/***************************************************************************\
* InternalFreeDataHandle
*
* Description:
* Frees a data handle and its contents. The contents are NOT freed for
* APPOWNED data handles unless fIgnorefRelease is set.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
BOOL InternalFreeDataHandle(
HDDEDATA hData,
BOOL fIgnorefRelease)
{
    PDDEMLDATA pdd;

    CheckDDECritIn;

    pdd = (PDDEMLDATA)ValidateCHandle((HANDLE)hData,
            HTYPE_DATA_HANDLE, HINST_ANY);
    if (pdd == NULL) {
        return (FALSE);
    }
    if (pdd->flags & HDATA_EXECUTE) {
        if (!(pdd->flags & HDATA_APPOWNED) || fIgnorefRelease) {
            WOWGLOBALFREE(pdd->hDDE);
        }
    } else {
        FreeDDEData(pdd->hDDE, fIgnorefRelease, TRUE);
    }
    DDEMLFree(pdd);
    DestroyHandle((HANDLE)hData);
    return (TRUE);
}


/***************************************************************************\
* ApplyFreeDataHandle
*
* Description:
* Used during data handle cleanup.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
BOOL ApplyFreeDataHandle(
HANDLE hData)
{
    BOOL fRet;

    CheckDDECritOut;
    EnterDDECrit;
    fRet = InternalFreeDataHandle((HDDEDATA)hData, FALSE);
    LeaveDDECrit;
    return(fRet);
}


/***************************************************************************\
* FreeDDEData
*
* Description:
* Used for freeing DDE data including any special indirect objects
* associated with the data depending on the format. This function
* SHOULD NOT BE USED TO FREE EXECUTE DATA!
*
* The data is not freed if the fRelease bit is clear and fIgnoreRelease
* is FALSE.
*
*   The fFreeTruelyGlobalObjects parameter is used to distinguish tracking
*   layer frees from DDEML frees.  Data in certain formats (CF_BITMAP,
*   CF_PALETTE) is maintained on the gdi CSR server side.  When this is
*   passed between processes, gdi is not able to maintain multiple process
*   ownership on these objects so the objects must be made global.  Thus
*   the tracking layer should NOT free these objects on behalf of another
*   process because they are truely global- however, DDEML can do this
*   because it is following the protocol which delclares who is in charge
*   of freeing global data.  (YUCK!)
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
/*
 * WARNING: This is exported for NetDDE use - DO NOT CHANGE THE PARAMETERS!
 */
VOID FreeDDEData(
HANDLE hDDE,
BOOL fIgnorefRelease,
BOOL fFreeTruelyGlobalObjects)
{
    PDDE_DATA pdde;
    LPMETAFILEPICT pmfPict;
    DWORD cb;

    USERGLOBALLOCK(hDDE, pdde);
    if (pdde == NULL) {
        return ;
    }

    if ((pdde->wStatus & DDE_FRELEASE) || fIgnorefRelease) {
        cb = (DWORD)GlobalSize(hDDE);
        /*
         * Because there is the possibility that the data never got
         * initialized we need to do this in a try-except so we
         * behave nicely.
         */
        switch (pdde->wFmt) {
        case CF_BITMAP:
        case CF_DSPBITMAP:
        case CF_PALETTE:
            if (cb >= sizeof(HANDLE)) {
                if (fFreeTruelyGlobalObjects) {
                    if (pdde->Data != 0) {
                        DeleteObject((HANDLE)pdde->Data);
                    }
                } else {
                    /*
                     * !fFreeTruelyGlobalObject implies we are only freeing
                     * the Gdi proxy.  (another process may still have this
                     * object in use.)
                     *
                     * ChrisWil: removed this call.  No longer
                     *           applicable in KMode.
                     *
                     * GdiDeleteLocalObject((ULONG)pdde->Data);
                     *
                     */
                }
            }
            break;

        case CF_DIB:
            if (cb >= sizeof(HANDLE)) {
                if (pdde->Data != 0) {
                    WOWGLOBALFREE((HANDLE)pdde->Data);
                }
            }
            break;

        case CF_METAFILEPICT:
        case CF_DSPMETAFILEPICT:
            if (cb >= sizeof(HANDLE)) {
                if (pdde->Data != 0) {
                    USERGLOBALLOCK(pdde->Data, pmfPict);
                    if (pmfPict != NULL) {
                        if (GlobalSize((HANDLE)pdde->Data) >= sizeof(METAFILEPICT)) {
                            DeleteMetaFile(pmfPict->hMF);
                        }
                        USERGLOBALUNLOCK((HANDLE)pdde->Data);
                        WOWGLOBALFREE((HANDLE)pdde->Data);
                    }
                }
            }
            break;

        case CF_ENHMETAFILE:
        case CF_DSPENHMETAFILE:
            if (cb >= sizeof(HANDLE)) {
                if (pdde->Data != 0) {
                    DeleteEnhMetaFile((HANDLE)pdde->Data);
                }
            }
            break;
        }
        USERGLOBALUNLOCK(hDDE);
        WOWGLOBALFREE(hDDE);
    } else {
        USERGLOBALUNLOCK(hDDE);
    }
}



HBITMAP CopyBitmap(
HBITMAP hbm)
{
    BITMAP bm;
    HBITMAP hbm2, hbmOld1, hbmOld2;
    HDC hdc, hdcMem1, hdcMem2;

    if (!GetObject(hbm, sizeof(BITMAP), &bm)) {
        return(0);
    }
    hdc = NtUserGetDC(NULL);  // screen DC
    if (!hdc) {
        return(0);
    }
    hdcMem1 = CreateCompatibleDC(hdc);
    if (!hdcMem1) {
        goto Cleanup3;
    }
    hdcMem2 = CreateCompatibleDC(hdc);
    if (!hdcMem2) {
        goto Cleanup2;
    }
    hbmOld1 = SelectObject(hdcMem1, hbm);
    hbm2 = CreateCompatibleBitmap(hdcMem1, bm.bmWidth, bm.bmHeight);
    if (!hbm2) {
        goto Cleanup1;
    }
    hbmOld2 = SelectObject(hdcMem2, hbm2);
    BitBlt(hdcMem2, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem1, 0, 0, SRCCOPY);
    SelectObject(hdcMem1, hbmOld1);
    SelectObject(hdcMem2, hbmOld2);
Cleanup1:
    DeleteDC(hdcMem2);
Cleanup2:
    DeleteDC(hdcMem1);
Cleanup3:
    NtUserReleaseDC(NULL, hdc);
    return(hbm2);
}


HPALETTE CopyPalette(
HPALETTE hpal)
{
    int cPalEntries;
    LOGPALETTE *plp;

    if (!GetObject(hpal, sizeof(int), &cPalEntries)) {
        return(0);
    }
    plp = (LOGPALETTE *)DDEMLAlloc(sizeof(LOGPALETTE) +
            (cPalEntries - 1) * sizeof(PALETTEENTRY));
    if (!plp) {
        return(0);
    }
    if (!GetPaletteEntries(hpal, 0, cPalEntries, plp->palPalEntry)) {
        DDEMLFree(plp);
        return(0);
    }
    plp->palVersion = 0x300;
    plp->palNumEntries = (WORD)cPalEntries;
    hpal = CreatePalette(plp);
    if (hpal  &&
            !SetPaletteEntries(hpal, 0, cPalEntries, plp->palPalEntry)) {
        hpal = 0;
    }
    DDEMLFree(plp);
    return(hpal);
}



/***************************************************************************\
* CopyDDEData
*
* Description:
* Used to copy DDE data apropriately.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
HANDLE CopyDDEData(
HANDLE hDDE,
BOOL fIsExecute)
{
    HANDLE hDDENew;
    PDDE_DATA pdde, pddeNew;
    LPMETAFILEPICT pmfPict;
    HANDLE hmfPict;

    hDDENew = UserGlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE,
            UserGlobalSize(hDDE));
    if (!hDDENew) {
        return (0);
    }
    USERGLOBALLOCK(hDDE, pdde);
    if (pdde == NULL) {
        UserGlobalFree(hDDENew);
        return (0);
    }
    USERGLOBALLOCK(hDDENew, pddeNew);
    UserAssert(pddeNew);
    RtlCopyMemory(pddeNew, pdde, UserGlobalSize(hDDE));

    if (!fIsExecute) {
        switch (pdde->wFmt) {
        case CF_BITMAP:
        case CF_DSPBITMAP:
            pddeNew->Data = (ULONG_PTR)CopyBitmap((HBITMAP)pdde->Data);
            break;

        case CF_PALETTE:
            pddeNew->Data = (ULONG_PTR)CopyPalette((HPALETTE)pdde->Data);
            break;

        case CF_DIB:
            pddeNew->Data = (ULONG_PTR)CopyDDEData((HANDLE)pdde->Data, TRUE);
            break;

        case CF_METAFILEPICT:
        case CF_DSPMETAFILEPICT:
            hmfPict = CopyDDEData((HANDLE)pdde->Data, TRUE);
            USERGLOBALLOCK(hmfPict, pmfPict);
            if (pmfPict == NULL) {
                WOWGLOBALFREE(hmfPict);
                USERGLOBALUNLOCK(hDDENew);
                WOWGLOBALFREE(hDDENew);
                USERGLOBALUNLOCK(hDDE);
                return (FALSE);
            }
            pmfPict->hMF = CopyMetaFile(pmfPict->hMF, NULL);
            USERGLOBALUNLOCK(hmfPict);
            pddeNew->Data = (ULONG_PTR)hmfPict;
            break;

        case CF_ENHMETAFILE:
        case CF_DSPENHMETAFILE:
            pddeNew->Data = (ULONG_PTR)CopyEnhMetaFile((HANDLE)pdde->Data, NULL);
            break;
        }
    }
    USERGLOBALUNLOCK(hDDENew);
    USERGLOBALUNLOCK(hDDE);
    return (hDDENew);
}
