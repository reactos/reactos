/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WSTRUC.C
 *  WOW32 16-bit structure conversion support
 *
 *  History:
 *  Created 27-Jan-1991 by Jeff Parsons (jeffpar)
 *  Wrote DDE data conversion routines, etc Chandan CHauhan (ChandanC)
 *
--*/


#include "precomp.h"
#pragma hdrstop
#include "wowshlp.h"
#ifdef FE_IME
#include "ime.h"
#endif // FE_IME

MODNAME(wstruc.c);


/* Structure copying functions
 *
 * For input structures, there are GETxxxx16 macros;  for output structures
 * there are PUTxxxx16 macros.  Most or all of these macros will simply call
 * the corresponding function below.  Nothing magical here....
 */

VOID getstr16(VPSZ vpszSrc, LPSZ lpszDst, INT cb)
{
    PSZ pszSrc;

    if (cb == 0)
        return;

    if (cb != -1)
        GETVDMPTR(vpszSrc, cb, pszSrc);
    else {
        GETPSZPTR(vpszSrc, pszSrc);
        cb = strlen(pszSrc)+1;
    }

    RtlCopyMemory(lpszDst, pszSrc, cb);

    FREEVDMPTR(pszSrc);

    RETURN(NOTHING);
}


VOID putstr16(VPSZ vpszDst, LPCSTR lpszSrc, INT cb)
{
    PSZ pszDst;

    if (cb == 0)
        return;

    if (cb == -1)
        cb = strlen(lpszSrc)+1;

    GETVDMPTR(vpszDst, cb, pszDst);

    RtlCopyMemory(pszDst, lpszSrc, cb);

    FLUSHVDMPTR(vpszDst, cb, pszDst);
    FREEVDMPTR(pszDst);

    RETURN(NOTHING);
}


LPRECT getrect16(VPRECT16 vpRect, LPRECT lpRect)
{
    register PRECT16 pRect16;

    // Some APIs (eg, InvalidateRect) have OPTIONAL input pointers -JTP
    if (vpRect) {

        GETVDMPTR(vpRect, sizeof(RECT16), pRect16);

        lpRect->left    = FETCHSHORT(pRect16->left);
        lpRect->top     = FETCHSHORT(pRect16->top);
        lpRect->right   = FETCHSHORT(pRect16->right);
        lpRect->bottom  = FETCHSHORT(pRect16->bottom);

        FREEVDMPTR(pRect16);

    } else {

        lpRect = NULL;

    }

    return lpRect;
}


VOID putrect16(VPRECT16 vpRect, LPRECT lpRect)
{
    register PRECT16 pRect16;

    // Some APIs (ScrollDC may be the only one) have OPTIONAL output pointers
    if (vpRect) {

        GETVDMPTR(vpRect, sizeof(RECT16), pRect16);

        STORESHORT(pRect16->left,   (SHORT)lpRect->left);
        STORESHORT(pRect16->top,    (SHORT)lpRect->top);
        STORESHORT(pRect16->right,  (SHORT)lpRect->right);
        STORESHORT(pRect16->bottom, (SHORT)lpRect->bottom);

        FLUSHVDMPTR(vpRect, sizeof(RECT16), pRect16);
        FREEVDMPTR(pRect16);
    }
}


VOID getpoint16(VPPOINT16 vpPoint, INT c, LPPOINT lpPoint)
{
    register PPOINT16 pPoint16;

    // The assumption here is that no APIs have OPTIONAL POINT pointers
    // (regardless of whether they're input or output) -JTP
    WOW32ASSERT(vpPoint);

    if (lpPoint) {
    GETVDMPTR(vpPoint, sizeof(POINT16), pPoint16);

    while (c--) {
        lpPoint->x  = pPoint16->x;
        lpPoint->y  = pPoint16->y;
        lpPoint++;
        pPoint16++;     // ZOWIE Batman, you wouldn't be able to increment
    }                   // this pointer if FREEVDMPTR wasn't a no-op! -JTP

    FREEVDMPTR(pPoint16);
    }
}


VOID putpoint16(VPPOINT16 vpPoint, INT c, LPPOINT lpPoint)
{
    INT               cb;
    PPOINT16          pPoint16ptr;
    register PPOINT16 pPoint16;

    // The assumption here is that no APIs have OPTIONAL POINT pointers
    // (regardless of whether they're input or output) -JTP
    WOW32ASSERT(vpPoint);

    if (lpPoint) {

        GETVDMPTR(vpPoint, sizeof(POINT16), pPoint16);

        cb = c;
        pPoint16ptr = pPoint16;
        while (c--) {
            if (lpPoint->x > (LONG)0x7fff)
                pPoint16->x = (SHORT)0x7fff;
            else if (lpPoint->x < (LONG)0xffff8000)
                pPoint16->x = (SHORT)0x8000;
            else
                pPoint16->x = (SHORT)lpPoint->x;

            if (lpPoint->y > (LONG)0x7fff)
                pPoint16->y = (SHORT)0x7fff;
            else if (lpPoint->y < (LONG)0xffff8000)
                pPoint16->y = (SHORT)0x8000;
            else
                pPoint16->y = (SHORT)lpPoint->y;

            lpPoint++;
            pPoint16++;
        }

        FLUSHVDMPTR(vpPoint, cb*sizeof(POINT16), pPoint16ptr);
        FREEVDMPTR(pPoint16ptr);
    }
}



VOID getintarray16(VPINT16 vpInt, INT c, LPINT lpInt)
{
    INT i;
    register INT16 *pInt16;

    if (lpInt && GETVDMPTR(vpInt, sizeof(INT16)*c, pInt16)) {
        for (i=0; i < c; i++) {
            lpInt[i] = FETCHSHORT(pInt16[i]);
        }
    }
    FREEVDMPTR(pInt16);
}


//
// getuintarray16
//
// copies an array of 16-bit unsigned words to an array of 32-bit unsigned
// words.  the pointer to 32-bit array must be freed by the caller.
//

VOID getuintarray16(VPWORD vp, INT c, PUINT pu)
{
    register WORD   *pW16;

    if (pu && GETVDMPTR(vp, sizeof(WORD)*c, pW16)) {
        while (c--) {
            pu[c] = FETCHWORD(pW16[c]);
        }
        FREEVDMPTR(pW16);
    }
}


VOID putintarray16(VPINT16 vpInt, INT c, LPINT lpInt)
{
    INT i;
    register INT16 *pInt16;

    if (lpInt && GETVDMPTR(vpInt, sizeof(INT16)*c, pInt16)) {
        for (i=0; i < c; i++) {
            STOREWORD(pInt16[i], lpInt[i]);
        }
        FLUSHVDMPTR(vpInt, sizeof(INT16)*c, pInt16);
        FREEVDMPTR(pInt16);
    }
}


VOID putkerningpairs16(VPKERNINGPAIR16 vpKrn, UINT c, LPKERNINGPAIR lpKrn)
{
    UINT i;
    register PKERNINGPAIR16 pKrn16;

    WOW32ASSERT(vpKrn);

    GETVDMPTR(vpKrn, sizeof(KERNINGPAIR16), pKrn16);

    for (i=0; i < c; i++) {
        pKrn16[i].wFirst = (WORD) lpKrn[i].wFirst;
        pKrn16[i].wSecond = (WORD) lpKrn[i].wSecond;
        pKrn16[i].iKernAmount = (INT16) lpKrn[i].iKernAmount;
    }

    FLUSHVDMPTR(vpKrn, sizeof(KERNINGPAIR16), pKrn16);
    FREEVDMPTR(pKrn16);
}




// Fill in a 32 bit DRAWITEMSTRUCT from a 16 bit one
VOID getdrawitem16(VPDRAWITEMSTRUCT16 vpDI16, LPDRAWITEMSTRUCT lpDI)
{
    register PDRAWITEMSTRUCT16 pDI16;

    GETVDMPTR(vpDI16, sizeof(DRAWITEMSTRUCT16), pDI16);

    lpDI->CtlType = FETCHWORD(pDI16->CtlType);
    lpDI->CtlID   = FETCHWORD(pDI16->CtlID);
    if ((SHORT)(lpDI->itemID = FETCHWORD(pDI16->itemID)) == -1) {
        lpDI->itemID = (UINT)-1;
    }
    lpDI->itemAction  = FETCHWORD(pDI16->itemAction);
    lpDI->itemState   = FETCHWORD(pDI16->itemState);
    lpDI->hwndItem = HWND32(FETCHWORD(pDI16->hwndItem));
    lpDI->hDC      = HDC32(FETCHWORD(pDI16->hDC));
    lpDI->rcItem.left   = (SHORT)pDI16->rcItem.left;
    lpDI->rcItem.top    = (SHORT)pDI16->rcItem.top;
    lpDI->rcItem.right  = (SHORT)pDI16->rcItem.right;
    lpDI->rcItem.bottom = (SHORT)pDI16->rcItem.bottom;
    lpDI->itemData = FETCHDWORD(pDI16->itemData);

    FREEVDMPTR(pDI16);
    return;
}


// Fill in a 16 bit DRAWITEMSTRUCT from a 32 bit one
VOID putdrawitem16(VPDRAWITEMSTRUCT16 vpDI16, LPDRAWITEMSTRUCT lpDI)
{
    register PDRAWITEMSTRUCT16 pDI16;

    GETVDMPTR(vpDI16, sizeof(DRAWITEMSTRUCT16), pDI16);

    STOREWORD(pDI16->CtlType, lpDI->CtlType);
    STOREWORD(pDI16->CtlID,   lpDI->CtlID);
    STOREWORD(pDI16->itemID,  lpDI->itemID);
    STOREWORD(pDI16->itemAction,  lpDI->itemAction);
    STOREWORD(pDI16->itemState,   lpDI->itemState);
    STOREWORD(pDI16->hwndItem, GETHWND16(lpDI->hwndItem));
    STOREWORD(pDI16->hDC, GETHDC16(lpDI->hDC));
    pDI16->rcItem.left   = (SHORT)lpDI->rcItem.left;
    pDI16->rcItem.top    = (SHORT)lpDI->rcItem.top;
    pDI16->rcItem.right  = (SHORT)lpDI->rcItem.right;
    pDI16->rcItem.bottom = (SHORT)lpDI->rcItem.bottom;
    STOREDWORD(pDI16->itemData,  lpDI->itemData);

    FREEVDMPTR(pDI16);
    return;
}

#ifdef NOTUSED
//
// Allocate and fill a 32bit DropFileStruct based on a 16bit one.
// 16 bit structure is not freed.
//
BOOL getdropfilestruct16(HAND16 hand16, PHANDLE phand32)
{
    PDROPFILESTRUCT16  lpdfs16;
    LPDROPFILESTRUCT   lpdfs32;
    VPVOID vp;
    INT cb;

    vp = GlobalLock16(hand16, &cb);

    if (vp) {

        GETMISCPTR(vp, lpdfs16);

        *phand32 = WOWGLOBALALLOC(GMEM_DDESHARE,
                                  cb-sizeof(DROPFILESTRUCT16)+
                                  sizeof(DROPFILESTRUCT));

        if (*phand32) {

            lpdfs32 = GlobalLock(*phand32);

            lpdfs32->pFiles = sizeof(DROPFILESTRUCT);
            lpdfs32->pt.x = lpdfs16->x;
            lpdfs32->pt.y = lpdfs16->y;
            lpdfs32->fNC  = lpdfs16->fNC;
            lpdfs32->fWide = FALSE;

            RtlCopyMemory((LPBYTE)lpdfs32+lpdfs32->pFiles,
                          (LPBYTE)lpdfs16+lpdfs16->pFiles,
                          cb-sizeof(DROPFILESTRUCT16));

            FREEMISCPTR(lpdfs16);
            GlobalUnlock16(hand16);

            GlobalUnlock(*phand32);

            return TRUE;

        } else {

            FREEMISCPTR(lpdfs16);
            GlobalUnlock16(hand16);

            return FALSE;
        }
    } else {
        *phand32 = NULL;
    }

    return FALSE;
}

#endif


INT GetBMI16Size(PVPVOID vpbmi16, WORD fuColorUse, LPDWORD lpdwClrUsed)
{
    int      nHdrSize;
    int      nEntSize;
    int      nEntries;
    int      nBitCount;
    int      nClrUsed = 0;
    register PBITMAPINFOHEADER16 pbmi16;

    GETVDMPTR(vpbmi16, sizeof(BITMAPINFO16), pbmi16);

    nHdrSize = (int) FETCHDWORD(pbmi16->biSize);

    if ( fuColorUse == DIB_RGB_COLORS ) {
        nEntSize = sizeof(RGBQUAD);
        if ( nHdrSize == sizeof(BITMAPCOREHEADER) ) {
            nEntSize = sizeof(RGBTRIPLE);
        }
    }
    else {
        nEntSize = sizeof(USHORT);
    }

    if( nHdrSize == sizeof(BITMAPINFOHEADER) ) {
        nBitCount    = FETCHWORD (pbmi16->biBitCount);
        nClrUsed     = FETCHDWORD(pbmi16->biClrUsed);
        *lpdwClrUsed = nClrUsed;
    }
    else {
        nBitCount    = FETCHWORD(((LPBITMAPCOREHEADER)pbmi16)->bcBitCount);
        *lpdwClrUsed = 0;
    }

/* the following block of code should be this:
 *
 *  if ( nBitCount >= 9 ) { // this is how Win3.1 code says == 24
 *      nEntries = 1;
 *  }
 *  else if ( dwClrUsed ) {
 *      nEntries = dwClrUsed;
 *  }
 *  else {
 *      nEntries = 1 << nBitCount;
 *  }
 *
 *  but due to the fact that many apps don't initialize the biBitCount &
 *  biClrUsed fields (especially biClrUsed) we have to do the following
 *  sanity checking instead.  cmjones
 */

    if ( nBitCount <= 8 ) {
        nEntries = 1 << nBitCount;

        // for selected apps ? What apps will be broken by this ?
        if (nClrUsed > 0 && nClrUsed < nEntries)
            nEntries = nClrUsed;
    }
    else if (( nBitCount == 16 ) || ( nBitCount == 32)) {
        nEntries = 3;
    }
    // everything else including (nBitCount == 24)
    else {
        nEntries = 0;
    }

    // if this asserts it's an app bug
    // Changed assert to warning at Craig's request - DaveHart
#ifdef DEBUG
    if (!(nEntries > 1) && !(nBitCount == 24)) {
        LOGDEBUG(LOG_ALWAYS, ("WOW::GetBMI16Size:bad BitCount:cmjones\n"));
    }
#endif

    // this will never be true for a CORE type DIB
    if(*lpdwClrUsed > (DWORD)nEntries) {
        *lpdwClrUsed = nEntries;
    }

    FREEVDMPTR(pbmi16);

    return ( nHdrSize + (nEntries * nEntSize) );
}



INT GetBMI32Size(LPBITMAPINFO lpbmi32, WORD fuColorUse)
{
    int      nHdrSize;
    int      nEntSize;
    int      nEntries;
    int      nBitCount;
    DWORD    dwClrUsed;

    nHdrSize =  (int)((LPBITMAPINFOHEADER)lpbmi32)->biSize;

    if ( fuColorUse == DIB_RGB_COLORS ) {
        nEntSize = sizeof(RGBQUAD);
        if ( nHdrSize == sizeof(BITMAPCOREHEADER) ) {
            nEntSize = sizeof(RGBTRIPLE);
        }
    }
    else {
        nEntSize = sizeof(USHORT);
    }

    if( nHdrSize == sizeof(BITMAPINFOHEADER) ) {
        nBitCount = ((LPBITMAPINFOHEADER)lpbmi32)->biBitCount;
        dwClrUsed = ((LPBITMAPINFOHEADER)lpbmi32)->biClrUsed;
    }
    else {
        nBitCount = (int)((LPBITMAPCOREHEADER)lpbmi32)->bcBitCount;
        dwClrUsed = 0;
    }

    nEntries = 0;
    if ( nBitCount < 9 ) {
        if((nBitCount == 4) || (nBitCount == 8) || (nBitCount == 1)) {
            nEntries = 1 << nBitCount;
        }
        else if (( nBitCount == 16 ) || ( nBitCount == 32)) {
            nEntries = 3;
        }
        // if this asserts it's an app bug -- we'll try to salvage things
        WOW32ASSERTMSG((nEntries > 1),
                       ("WOW::GetBMI32Size:bad BitCount: cmjones\n"));

        // sanity check for apps (lots) that don't init the dwClrUsed field
        if(dwClrUsed) {
            nEntries = (int)min((DWORD)nEntries, dwClrUsed);
        }
    }

    return ( nHdrSize + (nEntries * nEntSize) );
}



LPBITMAPINFO CopyBMI16ToBMI32(PVPVOID vpbmi16, LPBITMAPINFO lpbmi32, WORD fuColorUse)
{
    INT      nbmiSize = 0;
    DWORD    dwClrUsed;
    register LPVOID pbmi16;

    if(vpbmi16) {

        if((nbmiSize = GetBMI16Size(vpbmi16, fuColorUse, &dwClrUsed)) != 0) {

            GETVDMPTR(vpbmi16, nbmiSize, pbmi16);

       // We can't trust the size we get since apps don't fill in the
            // structure correctly so we do a try except around the copy
            // to the 32 bit structure.   That way if we go off the end of
            // memory it doesn't matter, we continue going.  - mattfe june 93

            try {
            RtlCopyMemory ((VOID *)lpbmi32, (CONST VOID *)pbmi16, nbmiSize);
            } except (TRUE) {
            // Continue if we wen't off the end of 16 bit memory
            }

            // patch color use field in 32-bit BMIH
            // INFO headers only - CORE headers will have dwClrUsed == 0
            if(dwClrUsed) {
                ((LPBITMAPINFOHEADER)lpbmi32)->biClrUsed = dwClrUsed;
            }

            FREEVDMPTR(pbmi16);

            return (lpbmi32);
        }
    }

    return (NULL);
}



LPBITMAPINFOHEADER CopyBMIH16ToBMIH32(PVPVOID vpbmih16, LPBITMAPINFOHEADER lpbmih32)
{
    DWORD    dwHeaderSize = 0L;
    register PBITMAPINFOHEADER16 pbmih16;

    if(vpbmih16) {

        GETVDMPTR(vpbmih16, sizeof(BITMAPINFOHEADER16), pbmih16);

        dwHeaderSize = FETCHDWORD(pbmih16->biSize);

        RtlCopyMemory((VOID *)lpbmih32,(CONST VOID *)pbmih16,(int)dwHeaderSize);

        FREEVDMPTR(pbmih16);

        return(lpbmih32);

    }

    return(NULL);
}



// Fill in a 32 bit MEASUREITEMSTRUCT from a 16 bit one
VOID getmeasureitem16(VPMEASUREITEMSTRUCT16 vpMI16, LPMEASUREITEMSTRUCT lpMI, HWND16 hwnd16 )
{
    register PMEASUREITEMSTRUCT16 pMI16;
    DWORD   dw;
    BOOL    fHasStrings;

    GETVDMPTR(vpMI16, sizeof(MEASUREITEMSTRUCT16), pMI16);

    lpMI->CtlType = FETCHWORD(pMI16->CtlType);
    lpMI->CtlID   = FETCHWORD(pMI16->CtlID);
    lpMI->itemID  = FETCHWORD(pMI16->itemID);
    lpMI->itemWidth   = FETCHWORD(pMI16->itemWidth);
    lpMI->itemHeight  = FETCHWORD(pMI16->itemHeight);

    fHasStrings = FALSE;
    if ( lpMI->CtlType == ODT_COMBOBOX ) {
        dw = GetWindowLong( GetDlgItem(HWND32(hwnd16),lpMI->CtlID), GWL_STYLE );
        fHasStrings = (dw & CBS_HASSTRINGS);
    }
    if ( lpMI->CtlType == ODT_LISTBOX ) {
        dw = GetWindowLong( GetDlgItem(HWND32(hwnd16),lpMI->CtlID), GWL_STYLE );
        fHasStrings = (dw & LBS_HASSTRINGS);
    }

    if ( fHasStrings ) {
        GETPSZPTR(pMI16->itemData, (PVOID)lpMI->itemData);
    } else {
        lpMI->itemData    = FETCHDWORD(pMI16->itemData);
    }

    FREEVDMPTR(pMI16);
    return;
}

// Fill in a 16 bit MEASUREITEMSTRUCT from a 32 bit one
VOID putmeasureitem16(VPMEASUREITEMSTRUCT16 vpMI16, LPMEASUREITEMSTRUCT lpMI)
{
    register PMEASUREITEMSTRUCT16 pMI16;

    GETVDMPTR(vpMI16, sizeof(MEASUREITEMSTRUCT16), pMI16);

    STOREWORD(pMI16->CtlType, lpMI->CtlType);
    STOREWORD(pMI16->CtlID,   lpMI->CtlID);
    STOREWORD(pMI16->itemID,  lpMI->itemID);
    STOREWORD(pMI16->itemWidth,  lpMI->itemWidth);
    STOREWORD(pMI16->itemHeight, lpMI->itemHeight);
    STOREDWORD(pMI16->itemData,  lpMI->itemData);

    FREEVDMPTR(pMI16);
    return;
}


// Fill in a 32 bit DELETEITEMSTRUCT from a 16 bit one
VOID getdeleteitem16(VPDELETEITEMSTRUCT16 vpDI16, LPDELETEITEMSTRUCT lpDI)
{
    register PDELETEITEMSTRUCT16 pDI16;

    GETVDMPTR(vpDI16, sizeof(DELETEITEMSTRUCT16), pDI16);

    lpDI->CtlType = FETCHWORD(pDI16->CtlType);
    lpDI->CtlID   = FETCHWORD(pDI16->CtlID);
    lpDI->itemID  = FETCHWORD(pDI16->itemID);
    lpDI->hwndItem = HWND32(FETCHWORD(pDI16->hwndItem));
    lpDI->itemData = FETCHDWORD(pDI16->itemData);

    FREEVDMPTR(pDI16);
    return;
}



// Fill in a 16 bit DELETEITEMSTRUCT from a 32 bit one
VOID putdeleteitem16(VPDELETEITEMSTRUCT16 vpDI16, LPDELETEITEMSTRUCT lpDI)
{
    register PDELETEITEMSTRUCT16 pDI16;

    GETVDMPTR(vpDI16, sizeof(DELETEITEMSTRUCT16), pDI16);

    STOREWORD(pDI16->CtlType, lpDI->CtlType);
    STOREWORD(pDI16->CtlID,   lpDI->CtlID);
    STOREWORD(pDI16->itemID,  lpDI->itemID);
    STOREWORD(pDI16->hwndItem, GETHWND16(lpDI->hwndItem));

    FLUSHVDMPTR(vpDI16, sizeof(DELETEITEMSTRUCT16), pDI16);
    FREEVDMPTR(pDI16);
    return;
}


// Fill in a 32 bit COMPAREITEMSTRUCT from a 16 bit one
VOID getcompareitem16(VPCOMPAREITEMSTRUCT16 vpCI16, LPCOMPAREITEMSTRUCT lpCI)
{
    register PCOMPAREITEMSTRUCT16 pCI16;

    GETVDMPTR(vpCI16, sizeof(COMPAREITEMSTRUCT16), pCI16);

    lpCI->CtlType    = FETCHWORD (pCI16->CtlType);
    lpCI->CtlID      = FETCHWORD (pCI16->CtlID);
    lpCI->hwndItem   = HWND32(pCI16->hwndItem);
    lpCI->itemID1    = FETCHWORD (pCI16->itemID1);
    lpCI->itemID2    = FETCHWORD (pCI16->itemID2);
    lpCI->itemData1  = FETCHDWORD(pCI16->itemData1);
    lpCI->itemData2  = FETCHDWORD(pCI16->itemData2);

    FREEVDMPTR(pCI16);
    return;
}

// Fill in a 16 bit COMPAREITEMSTRUCT from a 32 bit one
VOID putcompareitem16(VPCOMPAREITEMSTRUCT16 vpCI16, LPCOMPAREITEMSTRUCT lpCI)
{
    register PCOMPAREITEMSTRUCT16 pCI16;

    GETVDMPTR(vpCI16, sizeof(COMPAREITEMSTRUCT16), pCI16);

    STOREWORD (pCI16->CtlType,    (lpCI)->CtlType);
    STOREWORD (pCI16->CtlID,      (lpCI)->CtlID);
    STOREWORD (pCI16->hwndItem,   GETHWND16((lpCI)->hwndItem));
    STOREWORD (pCI16->itemID1,    (lpCI)->itemID1);
    STOREWORD (pCI16->itemID2,    (lpCI)->itemID2);
    STOREDWORD(pCI16->itemData1,  (lpCI)->itemData1);
    STOREDWORD(pCI16->itemData2,  (lpCI)->itemData2);

    FLUSHVDMPTR(vpCI16, sizeof(COMPAREITEMSTRUCT16), pCI16);
    FREEVDMPTR(pCI16);
    return;
}

// NOTE: The below two routines are useful to changing a simple 16-bit message
// into a 32-bit message and vice-versa.  The routines take into account only
// those messages which can be returned from GetMessage(), or passed to
// DispatchMessage(), etc.  Normally these are only input messages (all other
// messages are sent rather than posted.  This type of message has been
// extended to include DDE messages (they are posted too) and WM_TIMER messages.
// If you question this ability, please talk with me.  -BobDay
// These routines should only be called from these routines:
//    CallMsgFilter
//    DispatchMessage
//    GetMessage
//    IsDialogMessage
//    TranslateAccelerator
//    TranslateMDISysAccel
//    TranslateMessage
//    PeekMessage
//    WM32GetDlgCode
//    ThunkWMMsg16
// Don't call them from any other function!
//
// WARNING: May cause 16-bit memory movement, invalidating flat pointers.

// Fill in a 32 bit MSG from a 16 bit one
// See NOTE above!
VOID getmsg16(VPMSG16 vpmsg16, LPMSG lpmsg, LPMSGPARAMEX lpmpex)
{
    register PMSG16 pmsg16;
    UINT            message;
    HWND16          hwnd16;
    WPARAM          wParam;
    LPARAM          lParam;
    WPARAM          uParam;

    GETVDMPTR(vpmsg16, sizeof(MSG16), pmsg16);

    hwnd16  = FETCHWORD(pmsg16->hwnd);
    message = FETCHWORD(pmsg16->message);
    wParam  = FETCHWORD(pmsg16->wParam);
    lParam  = FETCHLONG(pmsg16->lParam);
    uParam  = INT32(wParam);              // default thunking - sign extend

#ifdef DEBUG
    if (HIWORD(wParam) &&
       !((message == WM_TIMER       ||
          message == WM_COMMAND     ||
          message == WM_DROPFILES   ||
          message == WM_HSCROLL     ||
          message == WM_VSCROLL     ||
          message == WM_MENUSELECT  ||
          message == WM_MENUCHAR    ||
          message == WM_ACTIVATEAPP ||
          message == WM_ACTIVATE)   ||
         (message >= WM_DDE_FIRST && message <= WM_DDE_LAST))) {

        LOGDEBUG(LOG_ALWAYS,("WOW: getmsg16 - losing HIWORD(wParam)!"));
        WOW32ASSERT(FALSE);
    }
#endif

    if ( message == WM_TIMER ) {
        HAND16  htask16;
        PTMR    ptmr;
        WORD    wIDEvent;

        htask16  = CURRENTPTD()->htask16;
        wIDEvent = (WORD)wParam;

        ptmr = FindTimer16( hwnd16, htask16, wIDEvent );

        if ( !ptmr ) {
            LOGDEBUG(LOG_TRACE,("    getmsg16 WARNING: cannot find timer %04x\n", wIDEvent));
        } else {
            uParam = (WPARAM)wIDEvent;   // wParam is unsigned word
            lParam = ptmr->dwTimerProc32;   // 32-bit proc or NULL
        }

    } else if ((message == WM_SYSCOMMAND  ||
                message == WM_COMMAND     ||
                message == WM_DROPFILES   ||
                message == WM_HSCROLL     ||
                message == WM_VSCROLL     ||
                message == WM_MENUSELECT  ||
                message == WM_MENUCHAR    ||
                message == WM_ACTIVATEAPP ||
                message == WM_ACTIVATE)   ||
               (message >= WM_DDE_FIRST && message <= WM_DDE_LAST)) {
        HWND hwnd32;

        lpmpex->Parm16.WndProc.hwnd = hwnd16;
        lpmpex->Parm16.WndProc.wMsg = (WORD)message;
        lpmpex->Parm16.WndProc.wParam = (WORD)wParam;
        lpmpex->Parm16.WndProc.lParam = lParam;
        lpmpex->iMsgThunkClass = WOWCLASS_WIN16;

        hwnd32 = ThunkMsg16(lpmpex);
        // memory may have moved
        FREEVDMPTR(pmsg16);
        GETVDMPTR(vpmsg16, sizeof(MSG16), pmsg16);

        lParam = lpmpex->lParam;
        lpmsg->hwnd = hwnd32;
        lpmsg->message = lpmpex->uMsg;
        lpmsg->wParam = lpmpex->uParam;

        if (message == WM_DDE_INITIATE) {
            LOGDEBUG(LOG_ALWAYS,("WOW::getmsg16:*** ERROR: saw WM_DDE_INITIATE message %04x\n", message));
            WI32DDEDeleteInitiator((HAND16) FETCHWORD(pmsg16->wParam));
        }
        goto finish_up;

    } else if (message == WM_SYSTIMER) {
        uParam  = UINT32(wParam);        // un-sign extend this
    } else if (message == WM_SETTEXT) {

        LONG lParamMap;


        LOGDEBUG(LOG_ALWAYS, ("WOW::Warning::getmsg16 processing WM_SETTEXT\n"));
        LOGDEBUG(LOG_ALWAYS, ("WOW::Check for possible rogue PostMessage\n"));


        // this is thy meaning: we have a message that comes from the 32-bit world
        // yet carries 16-bit payload. We have an option of converting this
        // to 32-bits at this very point via the param tracking mechanism

        GETPSZPTR(lParam, (LPSZ)lParamMap);
        lParam = (LPARAM)AddParamMap(lParamMap, lParam);
        if (lParam != lParamMap) {
           FREEPSZPTR((LPSZ)lParamMap);
        }

        //
        // now lParam is a "normal" 32-bit lParam with a map entry and
        // a ref count of "0" (meaning undead) so it could be deleted
        // later (during the thunking)
        //

        SetParamRefCount(lParam, PARAM_32, 0);
    }




    lpmsg->hwnd      = HWND32(hwnd16);
    lpmsg->message   = message;
    lpmsg->wParam    = uParam;
finish_up:
    lpmsg->lParam    = lParam;
    lpmsg->time      = FETCHLONG(pmsg16->time);
    lpmsg->pt.x      = FETCHSHORT(pmsg16->pt.x);
    lpmsg->pt.y      = FETCHSHORT(pmsg16->pt.y);

    FREEVDMPTR(pmsg16);
    return;
}

/*
 * Int 16 state key state bits (in order of Int 16 flags)
 */
BYTE abStateVK[] =   { VK_RSHIFT,
                       VK_LSHIFT,
                       VK_CONTROL,
                       VK_MENU} ;

/*
 * Int 16 state key toggle bits (in order of Int 16 flags)
 */
BYTE abToggleVK[] =  { VK_SCROLL,
                       VK_NUMLOCK,
                       VK_CAPITAL,
                       VK_INSERT};

// Updates the Int16 bios shift key state info
// (uses "synchronous" key state)
// GetKeyState is a very fast call (GetAsyncKeyState is not)
void UpdateInt16State(void)
{
    BYTE bInt16State = 0;
    LPBYTE lpbInt16Data;
    int iBit;

    /*
     * Get the toggled keys and OR in their toggle state
     */
    for( iBit = sizeof(abToggleVK)/sizeof(abToggleVK[0])-1;
            iBit >= 0; iBit--) {
        bInt16State = bInt16State << 1;
        if (GetKeyState(abToggleVK[iBit]) & 1)
            bInt16State |= 1;
    }

    /*
     * Get the state keys and OR in their current state
     */
    for( iBit = sizeof(abStateVK)/sizeof(abStateVK[0])-1;
            iBit >= 0; iBit--) {
        bInt16State = bInt16State << 1;
        if (GetKeyState(abStateVK[iBit]) & 0x8000)
            bInt16State |= 1;
    }

    // Int 16 keyboard state is at 40:17
    //
    // We need to update this address with the current state of
    // the keyboard buffer.
    lpbInt16Data = (LPBYTE)GetRModeVDMPointer(0x400017);
    *lpbInt16Data = bInt16State;

}

// Fill in a 16 bit MSG from a 32 bit one
// See NOTE above!
ULONG putmsg16(VPMSG16 vpmsg16, LPMSG lpmsg)
{
    register PMSG16 pmsg16;
    UINT    message;
    WPARAM  wParam;
    LPARAM  lParam;
    WM32MSGPARAMEX wm32mpex;
    ULONG ulReturn = 0;

    message = lpmsg->message;
    wParam = lpmsg->wParam;
    lParam = lpmsg->lParam;

    if (message == WM_TIMER) {
        PTMR ptmr;

        ptmr = FindTimer32((HWND16)(GETHWND16(lpmsg->hwnd)), (DWORD)wParam);

        if ( !ptmr ) {
            LOGDEBUG(LOG_TRACE,("    putmsg16 ERROR: cannot find timer %08x\n", lpmsg->wParam));
        } else {
            lParam = ptmr->vpfnTimerProc;   // 16-bit address or NULL
        }
    }
    else if ((message == WM_COMMAND     ||
              message == WM_DROPFILES   ||
              message == WM_HSCROLL     ||
              message == WM_VSCROLL     ||
              message == WM_MENUSELECT  ||
              message == WM_MENUCHAR    ||
              message == WM_ACTIVATEAPP ||
              message == WM_ACTIVATE)   ||
             (message >= WM_DDE_FIRST && message <= WM_DDE_LAST)) {

        WORD wParamNew = (WORD)wParam;
        LONG lParamNew = (LONG)lParam;

        wm32mpex.Parm16.WndProc.wParam = (WORD)wParam;
        wm32mpex.Parm16.WndProc.lParam = (LONG)lParam;
        wm32mpex.fThunk = THUNKMSG;
        wm32mpex.hwnd = lpmsg->hwnd;
        wm32mpex.uMsg = message;
        wm32mpex.uParam = wParam;
        wm32mpex.lParam = lParam;
        wm32mpex.pww = (PWW)NULL;
        wm32mpex.fFree = FALSE;
        wm32mpex.lpfnM32 = aw32Msg[message].lpfnM32;
        ulReturn = (wm32mpex.lpfnM32)(&wm32mpex);

        if (ulReturn) {
            wParam = (WPARAM) wm32mpex.Parm16.WndProc.wParam;
            lParam = (LPARAM) wm32mpex.Parm16.WndProc.lParam;
        }
        else {
            if ((message == WM_DDE_DATA) || (message == WM_DDE_POKE)) {
                wParam = (WPARAM) wm32mpex.Parm16.WndProc.wParam;
                lParam = (LPARAM) wm32mpex.Parm16.WndProc.lParam;
            }
        }
    } else if (message >= WM_KEYFIRST && message <= WM_KEYLAST) {
        UpdateInt16State();
#ifdef FE_IME
    // WM_IMEKEYDOWN & WM_IMEKEYUP  32 -> 16
    // 32bit:wParam  HIWORD charactor code, LOWORD virtual key
    // 16bit:wParam  HIBYTE charactor code, LOBYTE virtual key
    // kksuzuka:#4281 1994.11.19 MSKK V-HIDEKK
    } else if (message >= WM_IMEKEYDOWN && message <= WM_IMEKEYUP) {
        LONG wParamNew = wParam;
        wParamNew >>= 8;
        wParamNew |= (0xffff & wParam);
        wParam = wParamNew;
#endif // FE_IME
    } else if (message & WOWPRIVATEMSG) {

       LOGDEBUG(LOG_ALWAYS, ("WOW::Warning::putmsg16 caught private message 0x%lx\n", message));

       message &= ~WOWPRIVATEMSG; // clear the private bit...

       /* If we had some special processing to do for private msgs sent with
          a WOWPRIVATEMSG flag the code here would look like

          if (WM_SETTEXT == message) {
             //
             // this message is already equipped with 16-bit lParam
             //
          }
        */
    }

    GETVDMPTR(vpmsg16, sizeof(MSG16), pmsg16);

    STOREWORD(pmsg16->hwnd,    GETHWND16(lpmsg->hwnd));
    STOREWORD(pmsg16->message, message);
    STOREWORD(pmsg16->wParam,  wParam);
    STORELONG(pmsg16->lParam,  lParam);
    STORELONG(pmsg16->time,    lpmsg->time);
    STOREWORD(pmsg16->pt.x,    lpmsg->pt.x);
    STOREWORD(pmsg16->pt.y,    lpmsg->pt.y);

    FLUSHVDMPTR(vpmsg16, sizeof(MSG16), pmsg16);
    FREEVDMPTR(pmsg16);
    return (ulReturn);
}

// Fill in a 32 bit LOGFONT from a 16 bit LOGFONT
VOID getlogfont16(VPLOGFONT16 vplf, LPLOGFONT lplf)
{
    register PLOGFONT16 plf16;
  //  PBYTE    p1, p2;

    // The assumption here is that no APIs have OPTIONAL LOGFONT pointers
    WOW32WARNMSG((vplf),("WOW:getlogfont16: NULL 16:16 logfont ptr\n"));

    GETVDMPTR(vplf, sizeof(LOGFONT16), plf16);

    if(plf16) {
        lplf->lfHeight         = FETCHSHORT(plf16->lfHeight);
        lplf->lfWidth          = FETCHSHORT(plf16->lfWidth);
        lplf->lfEscapement     = FETCHSHORT(plf16->lfEscapement);
        lplf->lfOrientation    = FETCHSHORT(plf16->lfOrientation);
        lplf->lfWeight         = FETCHSHORT(plf16->lfWeight);
        lplf->lfItalic         = plf16->lfItalic;
        lplf->lfUnderline      = plf16->lfUnderline;
        lplf->lfStrikeOut      = plf16->lfStrikeOut;
        lplf->lfCharSet        = plf16->lfCharSet;
        lplf->lfOutPrecision   = plf16->lfOutPrecision;
        lplf->lfClipPrecision  = plf16->lfClipPrecision;
        lplf->lfQuality        = plf16->lfQuality;
        lplf->lfPitchAndFamily = plf16->lfPitchAndFamily;

#if 0
        //
        // can't do it this way, an app can have an unitialized lfFaceName
        // that's a looong stream of non-null chars, in which case we blow
        // out our stack.
        //

        p1 = lplf->lfFaceName;
        p2 = plf16->lfFaceName;

        while (*p1++ = *p2++);
#else

#if (LF_FACESIZE != 32)
#error Win16/Win32 LF_FACESIZE constants differ
#endif

        WOW32_strncpy(lplf->lfFaceName, (const char *)plf16->lfFaceName, LF_FACESIZE);

#endif


//      if (*p2) {
//          i = 0;
//          while ((i < LF_FACESIZE) && (*p2)) {
//              *p1++ = *p2++;
//              i++;
//          }
//          *p1 = *p2;
//
//      } else {
//          lstrcpy(lplf->lfFaceName, "System");
//      }

        FREEVDMPTR(plf16);
    }
}


// Fill in a 16 bit LOGFONT from a 32 bit LOGFONT
VOID putlogfont16(VPLOGFONT16 vplf, INT cb, LPLOGFONT lplf)
{
    register PLOGFONT16 plf16;
    PBYTE    p1, p2;
    INT      cbCopied;

    // The assumption here is that no APIs have OPTIONAL LOGFONT pointers
    WOW32WARNMSG((vplf),("WOW:putlogfont16: NULL 16:16 logfont ptr\n"));

    GETVDMPTR(vplf, sizeof(LOGFONT16), plf16);

    if(plf16) {
        switch (cb)
        {
        default:
            plf16->lfPitchAndFamily   = lplf->lfPitchAndFamily;
        case 17:
            plf16->lfQuality          = lplf->lfQuality;
        case 16:
            plf16->lfClipPrecision    = lplf->lfClipPrecision;
        case 15:
            plf16->lfOutPrecision     = lplf->lfOutPrecision;
        case 14:
            plf16->lfCharSet          = lplf->lfCharSet;
        case 13:
            plf16->lfStrikeOut        = lplf->lfStrikeOut;
        case 12:
            plf16->lfUnderline        = lplf->lfUnderline;
        case 11:
            plf16->lfItalic           = lplf->lfItalic;
        case 10:
            STORESHORT(plf16->lfWeight,     lplf->lfWeight);
        case 9:
        case 8:
            STORESHORT(plf16->lfOrientation,lplf->lfOrientation);
        case 7:
        case 6:
            STORESHORT(plf16->lfEscapement, lplf->lfEscapement);
        case 5:
        case 4:
            STORESHORT(plf16->lfWidth,      lplf->lfWidth);
        case 3:
        case 2:
            STORESHORT(plf16->lfHeight,     lplf->lfHeight);
        case 1:
            break;
        }

        p1 = lplf->lfFaceName;
        p2 = (PBYTE)plf16->lfFaceName;
        cbCopied = 18;

        while ((++cbCopied <= cb) && (*p2++ = *p1++));

        FLUSHVDMPTR(vplf, sizeof(LOGFONT16), plf16);
        FREEVDMPTR(plf16);
    }
}


// Fill in a 16 bit ENUMLOGFONT from a 32 bit ENUMLOGFONT
VOID putenumlogfont16(VPENUMLOGFONT16 vpelf, LPENUMLOGFONT lpelf)
{
    register PENUMLOGFONT16 pelf16;
    PBYTE    p1, p2;

    // The assumption here is that no APIs have OPTIONAL ENUMLOGFONT pointers
    WOW32ASSERT(vpelf);

    GETVDMPTR(vpelf, sizeof(ENUMLOGFONT16), pelf16);

    STORESHORT(pelf16->elfLogFont.lfHeight,     lpelf->elfLogFont.lfHeight);
    STORESHORT(pelf16->elfLogFont.lfWidth,      lpelf->elfLogFont.lfWidth);
    STORESHORT(pelf16->elfLogFont.lfEscapement, lpelf->elfLogFont.lfEscapement);
    STORESHORT(pelf16->elfLogFont.lfOrientation,lpelf->elfLogFont.lfOrientation);
    STORESHORT(pelf16->elfLogFont.lfWeight,     lpelf->elfLogFont.lfWeight);
    pelf16->elfLogFont.lfItalic           = lpelf->elfLogFont.lfItalic;
    pelf16->elfLogFont.lfUnderline        = lpelf->elfLogFont.lfUnderline;
    pelf16->elfLogFont.lfStrikeOut        = lpelf->elfLogFont.lfStrikeOut;
    pelf16->elfLogFont.lfCharSet          = lpelf->elfLogFont.lfCharSet;
    pelf16->elfLogFont.lfOutPrecision     = lpelf->elfLogFont.lfOutPrecision;
    pelf16->elfLogFont.lfClipPrecision    = lpelf->elfLogFont.lfClipPrecision;
    pelf16->elfLogFont.lfQuality          = lpelf->elfLogFont.lfQuality;
    pelf16->elfLogFont.lfPitchAndFamily   = lpelf->elfLogFont.lfPitchAndFamily;

    p1 = lpelf->elfLogFont.lfFaceName;
    p2 = (PBYTE)pelf16->elfLogFont.lfFaceName;

    while ((*p2++ = *p1++) != '\0');

    p1 = lpelf->elfFullName;
    p2 = (PBYTE)pelf16->elfFullName;

    while ((*p2++ = *p1++) != '\0');

    p1 = lpelf->elfStyle;
    p2 = (PBYTE)pelf16->elfStyle;

    while ((*p2++ = *p1++) != '\0');

    FLUSHVDMPTR(vpelf, sizeof(ENUMLOGFONT16), pelf16);
    FREEVDMPTR(pelf16);
}


VOID puttextmetric16(VPTEXTMETRIC16 vptm, LPTEXTMETRIC lptm)
{
    register PTEXTMETRIC16 ptm16;

    // The assumption here is that no APIs have OPTIONAL TEXTMETRIC pointers
    WOW32ASSERT(vptm);

    GETVDMPTR(vptm, sizeof(TEXTMETRIC16), ptm16);

    STORESHORT(ptm16->tmHeight,     (lptm)->tmHeight);
    STORESHORT(ptm16->tmAscent,     (lptm)->tmAscent);
    STORESHORT(ptm16->tmDescent,    (lptm)->tmDescent);
    STORESHORT(ptm16->tmInternalLeading,(lptm)->tmInternalLeading);
    STORESHORT(ptm16->tmExternalLeading,(lptm)->tmExternalLeading);
    STORESHORT(ptm16->tmAveCharWidth,   (lptm)->tmAveCharWidth);
    STORESHORT(ptm16->tmMaxCharWidth,   (lptm)->tmMaxCharWidth);
    STORESHORT(ptm16->tmWeight,     (lptm)->tmWeight);
    ptm16->tmItalic           = (lptm)->tmItalic;
    ptm16->tmUnderlined           = (lptm)->tmUnderlined;
    ptm16->tmStruckOut            = (lptm)->tmStruckOut;
    ptm16->tmFirstChar            = (lptm)->tmFirstChar;
    ptm16->tmLastChar             = (lptm)->tmLastChar;
    ptm16->tmDefaultChar          = (lptm)->tmDefaultChar;
    ptm16->tmBreakChar            = (lptm)->tmBreakChar;
    ptm16->tmPitchAndFamily       = (lptm)->tmPitchAndFamily;
    ptm16->tmCharSet              = (lptm)->tmCharSet;
    STORESHORT(ptm16->tmOverhang,    (lptm)->tmOverhang);
    STORESHORT(ptm16->tmDigitizedAspectX,(lptm)->tmDigitizedAspectX);
    STORESHORT(ptm16->tmDigitizedAspectY,(lptm)->tmDigitizedAspectY);

    FLUSHVDMPTR(vptm, sizeof(TEXTMETRIC16), ptm16);
    FREEVDMPTR(ptm16);
}


VOID putnewtextmetric16(VPNEWTEXTMETRIC16 vpntm, LPNEWTEXTMETRIC lpntm)
{
    register PNEWTEXTMETRIC16 pntm16;

    // The assumption here is that no APIs have OPTIONAL TEXTMETRIC pointers
    WOW32ASSERT(vpntm);

    GETVDMPTR(vpntm, sizeof(NEWTEXTMETRIC16), pntm16);

    STORESHORT(pntm16->tmHeight,           (lpntm)->tmHeight);
    STORESHORT(pntm16->tmAscent,           (lpntm)->tmAscent);
    STORESHORT(pntm16->tmDescent,          (lpntm)->tmDescent);
    STORESHORT(pntm16->tmInternalLeading,  (lpntm)->tmInternalLeading);
    STORESHORT(pntm16->tmExternalLeading,  (lpntm)->tmExternalLeading);
    STORESHORT(pntm16->tmAveCharWidth,     (lpntm)->tmAveCharWidth);
    STORESHORT(pntm16->tmMaxCharWidth,     (lpntm)->tmMaxCharWidth);
    STORESHORT(pntm16->tmWeight,           (lpntm)->tmWeight);

    pntm16->tmItalic               = (lpntm)->tmItalic;
    pntm16->tmUnderlined           = (lpntm)->tmUnderlined;
    pntm16->tmStruckOut            = (lpntm)->tmStruckOut;
    pntm16->tmFirstChar            = (lpntm)->tmFirstChar;
    pntm16->tmLastChar             = (lpntm)->tmLastChar;
    pntm16->tmDefaultChar          = (lpntm)->tmDefaultChar;
    pntm16->tmBreakChar            = (lpntm)->tmBreakChar;
    pntm16->tmPitchAndFamily       = (lpntm)->tmPitchAndFamily;
    pntm16->tmCharSet              = (lpntm)->tmCharSet;

    STORESHORT(pntm16->tmOverhang,         (lpntm)->tmOverhang);
    STORESHORT(pntm16->tmDigitizedAspectX, (lpntm)->tmDigitizedAspectX);
    STORESHORT(pntm16->tmDigitizedAspectY, (lpntm)->tmDigitizedAspectY);

    STOREDWORD(pntm16->ntmFlags,           (lpntm)->ntmFlags);
    STOREWORD( pntm16->ntmSizeEM,          (WORD)((lpntm)->ntmSizeEM));
    STOREWORD( pntm16->ntmCellHeight,      (WORD)((lpntm)->ntmCellHeight));
    STOREWORD( pntm16->ntmAvgWidth,        (WORD)((lpntm)->ntmAvgWidth));

    FLUSHVDMPTR(vpntm, sizeof(NEWTEXTMETRIC16), pntm16);
    FREEVDMPTR(pntm16);
}

VOID putoutlinetextmetric16(VPOUTLINETEXTMETRIC16 vpotm, INT cb, LPOUTLINETEXTMETRIC lpotm)
{
    register POUTLINETEXTMETRIC16 potm16;
    OUTLINETEXTMETRIC16    otm16;
    int     count;

    GETVDMPTR(vpotm, cb, potm16);

    otm16.otmSize = (WORD)lpotm->otmSize;

    /*
    ** Copy the TEXTMETRIC structure
    */
    otm16.otmTextMetrics.tmHeight           = (SHORT)lpotm->otmTextMetrics.tmHeight;
    otm16.otmTextMetrics.tmAscent           = (SHORT)lpotm->otmTextMetrics.tmAscent;
    otm16.otmTextMetrics.tmDescent          = (SHORT)lpotm->otmTextMetrics.tmDescent;
    otm16.otmTextMetrics.tmInternalLeading  = (SHORT)lpotm->otmTextMetrics.tmInternalLeading;
    otm16.otmTextMetrics.tmExternalLeading  = (SHORT)lpotm->otmTextMetrics.tmExternalLeading;
    otm16.otmTextMetrics.tmAveCharWidth     = (SHORT)lpotm->otmTextMetrics.tmAveCharWidth;
    otm16.otmTextMetrics.tmMaxCharWidth     = (SHORT)lpotm->otmTextMetrics.tmMaxCharWidth;
    otm16.otmTextMetrics.tmWeight           = (SHORT)lpotm->otmTextMetrics.tmWeight;
    otm16.otmTextMetrics.tmItalic           = lpotm->otmTextMetrics.tmItalic;
    otm16.otmTextMetrics.tmUnderlined       = lpotm->otmTextMetrics.tmUnderlined;
    otm16.otmTextMetrics.tmStruckOut        = lpotm->otmTextMetrics.tmStruckOut;
    otm16.otmTextMetrics.tmFirstChar        = lpotm->otmTextMetrics.tmFirstChar;
    otm16.otmTextMetrics.tmLastChar         = lpotm->otmTextMetrics.tmLastChar;
    otm16.otmTextMetrics.tmDefaultChar      = lpotm->otmTextMetrics.tmDefaultChar;
    otm16.otmTextMetrics.tmBreakChar        = lpotm->otmTextMetrics.tmBreakChar;
    otm16.otmTextMetrics.tmPitchAndFamily   = lpotm->otmTextMetrics.tmPitchAndFamily;
    otm16.otmTextMetrics.tmCharSet          = lpotm->otmTextMetrics.tmCharSet;
    otm16.otmTextMetrics.tmOverhang         = (SHORT)lpotm->otmTextMetrics.tmOverhang;
    otm16.otmTextMetrics.tmDigitizedAspectX = (SHORT)lpotm->otmTextMetrics.tmDigitizedAspectX;
    otm16.otmTextMetrics.tmDigitizedAspectY = (SHORT)lpotm->otmTextMetrics.tmDigitizedAspectY;

    otm16.otmFiller = lpotm->otmFiller;

    /*
    ** Panose
    */
    otm16.otmPanoseNumber.bFamilyType      = lpotm->otmPanoseNumber.bFamilyType;
    otm16.otmPanoseNumber.bSerifStyle      = lpotm->otmPanoseNumber.bSerifStyle;
    otm16.otmPanoseNumber.bWeight          = lpotm->otmPanoseNumber.bWeight;
    otm16.otmPanoseNumber.bProportion      = lpotm->otmPanoseNumber.bProportion;
    otm16.otmPanoseNumber.bContrast        = lpotm->otmPanoseNumber.bContrast;
    otm16.otmPanoseNumber.bStrokeVariation = lpotm->otmPanoseNumber.bStrokeVariation;
    otm16.otmPanoseNumber.bArmStyle        = lpotm->otmPanoseNumber.bArmStyle;
    otm16.otmPanoseNumber.bMidline         = lpotm->otmPanoseNumber.bMidline;
    otm16.otmPanoseNumber.bXHeight         = lpotm->otmPanoseNumber.bXHeight;

    otm16.otmfsSelection    = (WORD)lpotm->otmfsSelection;
    otm16.otmfsType         = (WORD)lpotm->otmfsType;
    otm16.otmsCharSlopeRise = (SHORT)lpotm->otmsCharSlopeRise;
    otm16.otmsCharSlopeRun  = (SHORT)lpotm->otmsCharSlopeRun;
    otm16.otmItalicAngle    = (SHORT)lpotm->otmItalicAngle;
    otm16.otmEMSquare       = (WORD)lpotm->otmEMSquare;
    otm16.otmAscent         = (SHORT)lpotm->otmAscent;
    otm16.otmDescent        = (SHORT)lpotm->otmDescent;
    otm16.otmLineGap        = (WORD)lpotm->otmLineGap;

    otm16.otmsCapEmHeight   = (WORD)lpotm->otmsCapEmHeight;
    otm16.otmsXHeight       = (WORD)lpotm->otmsXHeight;

    /*
    ** Font Box Rectangle (ZOWIE!, I sure wish I could use putrect16 but alas!)
    */
    otm16.otmrcFontBox.left    = (SHORT)lpotm->otmrcFontBox.left;
    otm16.otmrcFontBox.top     = (SHORT)lpotm->otmrcFontBox.top;
    otm16.otmrcFontBox.right   = (SHORT)lpotm->otmrcFontBox.right;
    otm16.otmrcFontBox.bottom  = (SHORT)lpotm->otmrcFontBox.bottom;

    otm16.otmMacAscent  = (SHORT)lpotm->otmMacAscent;
    otm16.otmMacDescent = (SHORT)lpotm->otmMacDescent;

    otm16.otmMacLineGap    = (WORD)lpotm->otmMacLineGap;
    otm16.otmusMinimumPPEM = (WORD)lpotm->otmusMinimumPPEM;

    otm16.otmptSubscriptSize.x     = (SHORT)lpotm->otmptSubscriptSize.x;
    otm16.otmptSubscriptSize.y     = (SHORT)lpotm->otmptSubscriptSize.y;

    otm16.otmptSubscriptOffset.x   = (SHORT)lpotm->otmptSubscriptOffset.x;
    otm16.otmptSubscriptOffset.y   = (SHORT)lpotm->otmptSubscriptOffset.y;

    otm16.otmptSuperscriptSize.x   = (SHORT)lpotm->otmptSuperscriptSize.x;
    otm16.otmptSuperscriptSize.y   = (SHORT)lpotm->otmptSuperscriptSize.y;

    otm16.otmptSuperscriptOffset.x = (SHORT)lpotm->otmptSuperscriptOffset.x;
    otm16.otmptSuperscriptOffset.y = (SHORT)lpotm->otmptSuperscriptOffset.y;

    otm16.otmsStrikeoutSize        =  (WORD)lpotm->otmsStrikeoutSize;
    otm16.otmsStrikeoutPosition    = (SHORT)lpotm->otmsStrikeoutPosition;

    otm16.otmsUnderscorePosition   = (SHORT)lpotm->otmsUnderscorePosition;
    otm16.otmsUnderscoreSize       = (SHORT)lpotm->otmsUnderscoreSize;

    otm16.otmpFamilyName = (WORD) (lpotm->otmpFamilyName -
           sizeof(OUTLINETEXTMETRIC) + sizeof(OUTLINETEXTMETRIC16));

    otm16.otmpFaceName = (WORD) (lpotm->otmpFaceName -
           sizeof(OUTLINETEXTMETRIC) + sizeof(OUTLINETEXTMETRIC16));

    otm16.otmpStyleName = (WORD) (lpotm->otmpStyleName -
           sizeof(OUTLINETEXTMETRIC) + sizeof(OUTLINETEXTMETRIC16));

    otm16.otmpFullName = (WORD) (lpotm->otmpFullName -
           sizeof(OUTLINETEXTMETRIC) + sizeof(OUTLINETEXTMETRIC16));

    count = sizeof(OUTLINETEXTMETRIC16);
    if ( cb <= count ) {
        count = cb;
    } else {
        /*
        ** Copy the rest of the buffer (strings, etc.) over verbatim.
        */
        RtlCopyMemory( (LPSTR)potm16 + sizeof(OUTLINETEXTMETRIC16),
                (LPSTR)lpotm + sizeof(OUTLINETEXTMETRIC),
                cb - sizeof(OUTLINETEXTMETRIC16) );
    }

    /*
    ** Now really copy it (the structure portion) into the 16-bit memory
    */
    RtlCopyMemory((VOID *)potm16, (CONST VOID *)&otm16, count );

    FLUSHVDMPTR(vpotm, cb, potm16);
    FREEVDMPTR(potm16);
}

// Converts a 16 bit handle table to 32 bit
VOID gethandletable16(VPWORD vpht, UINT c, LPHANDLETABLE lpht)
{
    PHANDLETABLE16 pht16;
    WORD w;
    GETVDMPTR(vpht, sizeof(HAND16)*c, pht16);

    // be careful, we need to get the correct 32 obj handle from alias

    while (c--)
    {
        w = FETCHWORD(pht16->objectHandle[c]);
        if (w)
            lpht->objectHandle[c] = HOBJ32(w);
        else
            lpht->objectHandle[c] = (HANDLE)NULL;
    }

    FREEVDMPTR(pht16);
}

// Converts a 32 bit handle table to 16 bit
VOID puthandletable16(VPWORD vpht, UINT c, LPHANDLETABLE lpht)
{
    PHANDLETABLE16 pht16;
    DWORD dw;
    GETVDMPTR(vpht, sizeof(HAND16)*c, pht16);

    // be careful, we need to get the correct 16 alias the 32 obj handle

    while (c--) {
        dw = FETCHDWORD(lpht->objectHandle[c]);
        if (dw) {
            pht16->objectHandle[c] = GETHOBJ16((HAND32)dw);
        }
        else {
            pht16->objectHandle[c] = (HAND16)NULL;
        }
    }

    FREEVDMPTR(pht16);
}







/*
 * To solve a ton of devmode compatibility issues we are now going to return
 * Win3.1 devmodes to 16-bit apps instead of NT devmodes.
 *
 * The most common problem we encounter is that apps determine the size to
 * allocate for a DEVMODE buffer by: sizeof(DEVMODE) + dm->dmDriverExtra
 * Apps seem to handle the DriverExtra stuff pretty well but there is a wide-
 * spread belief that the public DEVMODE structure is a fixed size.
 * We hide the NT specific DEVMODE stuff and the WOW devmode thunk info in
 * what the app thinks is the DriverExtra part of the devmode:
 *
 *        ____________________________  _____
 *       | Win 3.1 DEVMODE            |      |
 *       | dmSize = sizeof(DEVMODE31) |      |
 *       | dmDriverExtra =            |      |
 *    ___|__/ (sizeof(DEVMODENT)   -  |   Win 3.1 DEVMODE
 *   |   |  \  sizeof(DEVMODE31))  +  |      |
 *  -|---|--- original DriverExtra +  |      |
 * | |  _|__/ sizeof(DWORD)        +  |      |
 * | | | |  \ (sizeof(WORD) * 3)      |      |
 * | | | |____________________________| _____|  <-- where app thinks driver
 * | | | | NT DEVMODE stuff not in    |      |      extra starts
 * | `-->| the Win3.1 DEVMODE struct  |   NT specific DEVMODE stuff
 * |   | |____________________________| _____|  <-- where driver extra really
 * `---->| actual NT driver extra     |             starts
 *     | |____________________________|         <-- where WOWDM31 struct starts
 *     | | DWORD with "DM31"          | <--- WOW DEVMODE31 signature
 *     ->| WORD original dmSpecVersion|\
 *       | WORD original dmSize       | <--- values returned by the driver
 *       | WORD original dmDriverExtra|/
 *       | WORD to pad to even DWORD  | <--- requried for ptr arithmetic
 *       |____________________________|
 *
 * NOTE: We may see Win3.0 & Win3.1 DevModes that are returned by 16-bit fax
 *       drivers.
 *
*/
LPDEVMODE ThunkDevMode16to32(VPDEVMODE31 vpdm16)
{
    INT        nSize, nDriverExtra;
    LPDEVMODE  lpdm32;
    PDEVMODE31 pdm16;
    PWOWDM31   pWOWDM31;

    if(FETCHDWORD(vpdm16) == 0L) {
        return(NULL);
    }

    GETVDMPTR(vpdm16, sizeof(DEVMODE31), pdm16);


    // we will generally see only Win3.1 DevMode's here but 16-bit fax
    // drivers can return a Win3.0 DevMode.
    nSize = FETCHWORD(pdm16->dmSize);
    WOW32WARNMSGF((nSize==sizeof(DEVMODE31)),
                  ("ThunkDevMode16to32: Unexpected dmSize(16) = %d\n", nSize));

    // check for bad DEVMODE (PageMaker & MSProfit are known culprits)
    // (PageMaker 5.0a passes a 16:16 ptr to NULL!!)
    // this test taken from gdi\client\object.c!bConvertToDevmodeW
    if ( (nSize < (offsetof(DEVMODE, dmDriverExtra) + sizeof(WORD))) ||
         (nSize > sizeof(DEVMODE)) ) {
        LOGDEBUG(LOG_ALWAYS,("WOW::ThunkDevMode16to32:Bail out case!!\n"));

        return(NULL);
    }

    // note this might include the "extra" DriverExtra we added in
    // ThunkDevMode32to16()
    nDriverExtra = FETCHWORD(pdm16->dmDriverExtra);

    // allocate 32-bit DEVMODE -- don't worry if we alloc a little too much due
    // to the WOW stuff we added to the end of the driver extra
    if(lpdm32 = malloc_w(nSize + nDriverExtra)) {

        // fill in the 32-bit devmode
        RtlCopyMemory((VOID *)lpdm32,(CONST VOID *)pdm16, nSize + nDriverExtra);

        // if this is a Win3.1 size DEVMODE, it may be one of our special ones
        if(nSize == sizeof(DEVMODE31)) {

            // see if it has our "DM31" signature at the end of the DEVMODE
            pWOWDM31  = (PWOWDM31)((PBYTE)lpdm32     +
                                   sizeof(DEVMODE31) +
                                   nDriverExtra      -
                                   sizeof(WOWDM31));

            // if it does, adjust the dmSpecVersion, dmSize & dmDriverExtra
            // back to the values we got from the driver
            if(pWOWDM31->dwWOWSig == WOW_DEVMODE31SIG) {
                lpdm32->dmSpecVersion = pWOWDM31->dmSpecVersion;
                lpdm32->dmSize        = pWOWDM31->dmSize;
                lpdm32->dmDriverExtra = pWOWDM31->dmDriverExtra;
            }
#ifdef DEBUG
            // somehow the app got a DEVMODE and either lost our thunking info
            // or threw it away (#205327)
            else {
                LOGDEBUG(LOG_ALWAYS, ("WOW::ThunkDevMode16to32: Signature missing from DEVMODE!!\n"));
            }
#endif

        }
    }

    FREEVDMPTR(pdm16);

    return(lpdm32);
}






BOOL ThunkDevMode32to16(VPDEVMODE31 vpdm16, LPDEVMODE lpdm32, UINT nBytes)
{

    WORD nSize, nDriverExtra;

    PDEVMODE31 pdm16;
    PWOWDM31   pWOWDM31;


    GETVDMPTR(vpdm16, sizeof(DEVMODE31), pdm16);

    if((FETCHDWORD(vpdm16) == 0L) || (!lpdm32) || (!pdm16)) {
        return(FALSE);
    }

    nSize = lpdm32->dmSize;

    // We should only see DevModes of the current NT size because the spooler
    // converts all devmodes to the current version
    WOW32WARNMSGF((nSize==sizeof(DEVMODE)),
                  ("ThunkDevMode32to16: Unexpected devmode size = %d\n",nSize));

    nDriverExtra = lpdm32->dmDriverExtra;

    // fill in the 16-bit devmode
    RtlCopyMemory((VOID *)pdm16,
                  (CONST VOID *)lpdm32,
                  min((nSize + nDriverExtra), (WORD)nBytes));

    // Convert NT sized devmodes to Win3.1 devmodes.
    // Note: Winfax.drv passes back an NT size DevMode with dmSpecVersion=0x300
    //       also it passes a hard coded 0xa9 to GetEnvironment() as the max
    //       size of its buffer (see GetEnvironment() notes in wgdi.c)
    //       If there is a buffer constraint, we'll just have to be satisfied
    //       with copying the nBytes worth of the devmode which should work
    //       in the case of WinFax.
    if((nSize == sizeof(DEVMODE)) && ((nSize + nDriverExtra) <= (WORD)nBytes)) {

        // save our signature along with the original dmSpecVersion, dmSize,
        // and dmDriverExtra at the end of the DriverExtra memory
        pWOWDM31  = (PWOWDM31)((PBYTE)pdm16    +
                               sizeof(DEVMODE) +
                               nDriverExtra);
        pWOWDM31->dwWOWSig      = WOW_DEVMODE31SIG;
        pWOWDM31->dmSpecVersion = lpdm32->dmSpecVersion;
        pWOWDM31->dmSize        = nSize;
        pWOWDM31->dmDriverExtra = nDriverExtra;

        // Make our special adjustments to the public devmode stuff.
        // We can't tell an app a Win3.0 DevMode is a Win3.1 version or it might
        // try to write to the new Win3.1 fields
        if(lpdm32->dmSpecVersion > WOW_DEVMODE31SPEC) {
            pdm16->dmSpecVersion  = WOW_DEVMODE31SPEC;
        }
        pdm16->dmSize         = sizeof(DEVMODE31);
        pdm16->dmDriverExtra += WOW_DEVMODEEXTRA;
    }

    FLUSHVDMPTR(vpdm16, sizeof(DEVMODE31), pdm16);
    FREEVDMPTR(pdm16);

    return(TRUE);
}




VOID getwindowpos16( VPWINDOWPOS16 vpwp, LPWINDOWPOS lpwp )
{
    register PWINDOWPOS16   pwp16;

    GETVDMPTR(vpwp, sizeof(WINDOWPOS16), pwp16);

    lpwp->hwnd            = HWND32(pwp16->hwnd);
    lpwp->hwndInsertAfter = HWNDIA32(pwp16->hwndInsertAfter);
    lpwp->x               = (INT) FETCHSHORT(pwp16->x);
    lpwp->y               = (INT) FETCHSHORT(pwp16->y);
    lpwp->cx              = (INT) FETCHSHORT(pwp16->cx);
    lpwp->cy              = (INT) FETCHSHORT(pwp16->cy);
    lpwp->flags           = (WORD) FETCHWORD(pwp16->flags);

    FREEVDMPTR(pwp16);
}

VOID putwindowpos16( VPWINDOWPOS16 vpwp, LPWINDOWPOS lpwp )
{
    register PWINDOWPOS16   pwp16;

    GETVDMPTR(vpwp, sizeof(WINDOWPOS16), pwp16);

    STOREWORD(pwp16->hwnd, GETHWND16(lpwp->hwnd));
    STOREWORD(pwp16->hwndInsertAfter, GETHWNDIA16(lpwp->hwndInsertAfter));
    STORESHORT(pwp16->x, lpwp->x);
    STORESHORT(pwp16->y, lpwp->y);
    STORESHORT(pwp16->cx, lpwp->cx);
    STORESHORT(pwp16->cy, lpwp->cy);
    STOREWORD(pwp16->flags, lpwp->flags);

    FLUSHVDMPTR(vpwp, sizeof(WINDOWPOS16), pwp16);
    FREEVDMPTR(pwp16);
}


VOID W32CopyMsgStruct(VPMSG16 vpmsg16, LPMSG lpmsg, BOOL fThunk16To32)
{
    register PMSG16 pmsg16;

    GETVDMPTR(vpmsg16, sizeof(MSG16), pmsg16);

    if (fThunk16To32) {
        lpmsg->hwnd      = HWND32(pmsg16->hwnd);
        lpmsg->message   = pmsg16->message;
        lpmsg->wParam    = pmsg16->wParam;
        lpmsg->lParam    = pmsg16->lParam;
        lpmsg->time      = pmsg16->time;
        lpmsg->pt.x      = pmsg16->pt.x;
        lpmsg->pt.y      = pmsg16->pt.y;
    }
    else {
        // for later use.
    }

    FREEVDMPTR(pmsg16);
    return;
}

VOID getpaintstruct16(VPVOID vp, LPPAINTSTRUCT lp)
{
    PPAINTSTRUCT16 pps16;
    GETVDMPTR(vp, sizeof(PAINTSTRUCT16), pps16);
    (lp)->hdc       = HDC32(FETCHWORD(pps16->hdc));
    (lp)->fErase    = FETCHSHORT(pps16->fErase);
    (lp)->rcPaint.left  = FETCHSHORT(pps16->rcPaint.left);
    (lp)->rcPaint.top   = FETCHSHORT(pps16->rcPaint.top);
    (lp)->rcPaint.right = FETCHSHORT(pps16->rcPaint.right);
    (lp)->rcPaint.bottom= FETCHSHORT(pps16->rcPaint.bottom);
    (lp)->fRestore  = FETCHSHORT(pps16->fRestore);
    (lp)->fIncUpdate    = FETCHSHORT(pps16->fIncUpdate);
    RtlCopyMemory((lp)->rgbReserved,
       pps16->rgbReserved, sizeof(pps16->rgbReserved));
    FREEVDMPTR(pps16);
}


VOID putpaintstruct16(VPVOID vp, LPPAINTSTRUCT lp)
{
    PPAINTSTRUCT16 pps16;
    GETVDMPTR(vp, sizeof(PAINTSTRUCT16), pps16);
    STOREWORD(pps16->hdc,       GETHDC16((lp)->hdc));
    STORESHORT(pps16->fErase,       (lp)->fErase);
    STORESHORT(pps16->rcPaint.left, (lp)->rcPaint.left);
    STORESHORT(pps16->rcPaint.top,  (lp)->rcPaint.top);
    STORESHORT(pps16->rcPaint.right,    (lp)->rcPaint.right);
    STORESHORT(pps16->rcPaint.bottom,   (lp)->rcPaint.bottom);
    STORESHORT(pps16->fRestore,     (lp)->fRestore);
    STORESHORT(pps16->fIncUpdate,   (lp)->fIncUpdate);
    RtlCopyMemory(pps16->rgbReserved,
       (lp)->rgbReserved, sizeof(pps16->rgbReserved));
    FLUSHVDMPTR(vp, sizeof(PAINTSTRUCT16), pps16);
    FREEVDMPTR(pps16);
}

VOID FASTCALL getmenuiteminfo16(VPVOID vp, LPMENUITEMINFO pmii32)
{
    PMENUITEMINFO16 pmii16;

    GETVDMPTR(vp, sizeof(*pmii16), pmii16);

    pmii32->cbSize = sizeof(*pmii32);
    pmii32->fMask = pmii16->fMask;

    if (pmii32->fMask & MIIM_CHECKMARKS) {
        pmii32->hbmpChecked = HBITMAP32(pmii16->hbmpChecked);
        pmii32->hbmpUnchecked = HBITMAP32(pmii16->hbmpUnchecked);
    } else {
        pmii32->hbmpChecked = pmii32->hbmpUnchecked = NULL;
    }

    pmii32->dwItemData = (pmii32->fMask & MIIM_DATA)
                             ? pmii16->dwItemData
                             : 0;

    pmii32->wID = (pmii32->fMask & MIIM_ID)
                  ? pmii16->wID
                  : 0;

    pmii32->fState = (pmii32->fMask & MIIM_STATE)
                         ? pmii16->fState
                         : 0;

    pmii32->hSubMenu = (pmii32->fMask & MIIM_SUBMENU)
                           ? HMENU32(pmii16->hSubMenu)
                           : NULL;

    if (pmii32->fMask & MIIM_TYPE) {

        pmii32->fType = pmii16->fType;

        if (pmii32->fType & MFT_BITMAP) {
            pmii32->dwTypeData = (LPTSTR) HBITMAP32(pmii16->dwTypeData);
        } else if (!(pmii32->fType & MFT_NONSTRING)) {  // like (pmii32->fType & MFT_STRING) but MFT_STRING is zero
            GETPSZPTR(pmii16->dwTypeData, pmii32->dwTypeData);
            AddParamMap( (DWORD) pmii32->dwTypeData, pmii16->dwTypeData);
        } else {
            pmii32->dwTypeData = (LPTSTR) pmii16->dwTypeData;
        }
    } else {
        pmii32->dwTypeData = (LPSTR) pmii32->fType = 0;
    }

    pmii32->cch = pmii16->cch;

    FREEVDMPTR(pmii16);
}


VOID FASTCALL putmenuiteminfo16(VPVOID vp, LPMENUITEMINFO pmii32)
{
    PMENUITEMINFO16 pmii16;

    GETVDMPTR(vp, sizeof(*pmii16), pmii16);

    pmii16->cbSize = sizeof(*pmii16);
    pmii16->fMask = (WORD) pmii32->fMask;

    if (pmii32->fMask & MIIM_CHECKMARKS) {
        pmii16->hbmpChecked = GETHBITMAP16(pmii32->hbmpChecked);
        pmii16->hbmpUnchecked = GETHBITMAP16(pmii32->hbmpUnchecked);
    }

    if (pmii32->fMask & MIIM_DATA) {
        pmii16->dwItemData =  pmii32->dwItemData;
    }

    if (pmii32->fMask & MIIM_ID) {
        pmii16->wID = (WORD) pmii32->wID;
    }

    if (pmii32->fMask & MIIM_STATE) {
        pmii16->fState = (WORD) pmii32->fState;
    }

    if (pmii32->fMask & MIIM_SUBMENU) {
        pmii16->hSubMenu = GETHMENU16(pmii32->hSubMenu);
    }

    if (pmii32->fMask & MIIM_TYPE) {

        pmii16->fType = (WORD) pmii32->fType;

        if (pmii32->fType & MFT_BITMAP) {
            pmii16->dwTypeData = GETHBITMAP16(pmii32->dwTypeData);
        } else if (!(pmii32->fType & MFT_NONSTRING)) {  // like (pmii32->fType & MFT_STRING) but MFT_STRING is zero
            pmii16->dwTypeData = GetParam16( (DWORD) pmii32->dwTypeData);
        } else {
            pmii16->dwTypeData = (VPSTR) pmii32->dwTypeData;
        }
    }

    FREEVDMPTR(pmii16);
}

