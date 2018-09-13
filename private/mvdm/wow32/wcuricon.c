//*****************************************************************************
//
// Cursor and Icon compatibility Support -
//
//     Support for apps - which do a GlobalLock on Cursors and Icons to
//     create headaches for us.
//
//     A compatibility issue.
//
//
// 21-Apr-92  NanduriR   Created.
//
//*****************************************************************************

#include "precomp.h"
#pragma hdrstop

MODNAME(wcuricon.c);


extern void FreeAccelAliasEntry(LPACCELALIAS lpT);

LPCURSORICONALIAS lpCIAlias = NULL;
UINT              cPendingCursorIconUpdates = 0;

//*****************************************************************************
//
// W32CreateCursorIcon32 -
//
//     Creates a 32bit Cursor or Icon given a WIN31 Cursor or Icon HANDLE.
//     The Cursor of Icon handle must correspond to an object that has
//     been created (like CreateIcon). That is because the format of a
//     resource cursor differs from that of a 'created'  cursor.
//
//     Returns the 32bit handle
//
//*****************************************************************************


HANDLE W32CreateCursorIcon32(LPCURSORICONALIAS lpCIAliasIn)
{
    HANDLE  hT;
    PCURSORSHAPE16 pcurs16;
    UINT   flType;

    int     nWidth;
    int     nHeight;
    int     nPlanes;
    int     nBitsPixel;
    DWORD   nBytesAND;
    LPBYTE  lpBitsAND;
    LPBYTE  lpBitsXOR;
    int     ScanLen16;


    pcurs16 = (PCURSORSHAPE16)lpCIAliasIn->pbDataNew;

    flType = lpCIAliasIn->flType;
    if (flType & HANDLE_TYPE_UNKNOWN) {
        if (PROBABLYCURSOR(FETCHWORD(pcurs16->BitsPixel),
                                              FETCHWORD(pcurs16->Planes)))
            flType = HANDLE_TYPE_CURSOR;
        else
            flType = HANDLE_TYPE_ICON;
    }

    nWidth     = INT32(FETCHWORD(pcurs16->cx));
    nHeight    = INT32(FETCHWORD(pcurs16->cy));

    nPlanes    = 1;
    nBitsPixel = 1;                                  // Monochrome

    // Get the AND mask bits

    ScanLen16 = (((nWidth*nBitsPixel)+15)/16) * 2 ;  // bytes/scan in 16 bit world
                                                     // effectively nBitsPixel is 1
    nBytesAND = ScanLen16*nHeight*nPlanes;
    lpBitsAND = (LPBYTE)pcurs16 + sizeof(CURSORSHAPE16);

    // Get the XOR mask bits

    if (flType == HANDLE_TYPE_ICON) {
        nPlanes    = INT32(FETCHWORD(pcurs16->Planes));
        nBitsPixel = INT32(FETCHWORD(pcurs16->BitsPixel));  // the actual value
    }

    lpBitsXOR = (LPBYTE)lpBitsAND + nBytesAND;

    lpCIAliasIn->flType = (BYTE)flType;

    if (flType & HANDLE_TYPE_CURSOR) {
        hT = CreateCursor(HMODINST32(lpCIAliasIn->hInst16),
                              (INT)FETCHWORD(pcurs16->xHotSpot),
                              (INT)FETCHWORD(pcurs16->yHotSpot),
                              nWidth, nHeight, lpBitsAND, lpBitsXOR);
    }
    else if (flType & HANDLE_TYPE_ICON) {
        hT = CreateIcon(HMODINST32(lpCIAliasIn->hInst16), nWidth, nHeight,
                              (BYTE)nPlanes, (BYTE)nBitsPixel, lpBitsAND, lpBitsXOR);

    }

    return hT;
}


//*****************************************************************************
//
// W32Create16BitCursorIcon -
//
//     Creates a WIN31 compatible Cursor or Icon  given the full 16bit
//     definition of the object to be created.
//
//
//*****************************************************************************


HAND16 W32Create16BitCursorIcon(HAND16 hInst16, INT xHotSpot, INT yHotSpot,
                         INT nWidth, INT nHeight,
                         INT nPlanes, INT nBitsPixel,
                         LPBYTE lpBitsAND, LPBYTE lpBitsXOR,
                         INT   nBytesAND, INT nBytesXOR                    )
{
    WORD h16 = 0;
    WORD wTotalSize;
    PCURSORSHAPE16 pcshape16;
    VPVOID vp;
    LPBYTE lpT;

    UNREFERENCED_PARAMETER(hInst16);

    wTotalSize = (WORD)(sizeof(CURSORSHAPE16) + nBytesAND + nBytesXOR);

    vp = GlobalAllocLock16(GMEM_MOVEABLE | GMEM_ZEROINIT | GMEM_SHARE,
                                                             wTotalSize, &h16);
    if (vp != (VPVOID)NULL) {
        GETVDMPTR(vp, wTotalSize, pcshape16);

        STOREWORD(pcshape16->xHotSpot, xHotSpot);
        STOREWORD(pcshape16->yHotSpot, yHotSpot);
        STOREWORD(pcshape16->cx, nWidth);
        STOREWORD(pcshape16->cy, nHeight);
        STOREWORD(pcshape16->cbWidth, (((nWidth + 0x0F) & ~0x0F) >> 3));
        pcshape16->Planes = (BYTE)nPlanes;
        pcshape16->BitsPixel = (BYTE)nBitsPixel;

        lpT = (LPBYTE)pcshape16 + sizeof(CURSORSHAPE16);
        RtlCopyMemory(lpT, lpBitsAND, nBytesAND);
        RtlCopyMemory(lpT+nBytesAND, lpBitsXOR, nBytesXOR);

        FLUSHVDMPTR(vp, wTotalSize, pcshape16);
        FREEVDMPTR(pcshape16);
    }

    GlobalUnlock16(h16);
    return (HAND16)h16;
}



//*****************************************************************************
//
// GetCursorIconAlias32 -
//
//     Returns a 32bit handle  given a 16bit Cursor or Icon HANDLE
//     Creates the 32bit Cursor or Icon if necessary.
//
//     Returns the 32bit handle
//
//*****************************************************************************


HANDLE GetCursorIconAlias32(HAND16 h16, UINT flType)
{

    LPCURSORICONALIAS lpT;
    VPVOID vp;
    UINT   cb;
    PCURSORSHAPE16 pcurs16;

    if (h16 == (HAND16)0)
        return (ULONG)NULL;

    lpT = FindCursorIconAlias((ULONG)h16, HANDLE_16BIT);
    if (lpT) {
        return lpT->h32;
    }
    else {

        //
        // BEGIN: Check for Bogus handle
        //

        if (BOGUSHANDLE(h16))
            return (HANDLE)NULL;

#if defined(FE_SB)
        //In Excel95, XLVISEX.EXE use wrong cursor handle
        //that is already freed. So, we double check this handle
        //whether it is valid or not. 09/27/96 bklee.

        if (!FindCursorIconAliasInUse((ULONG)h16))
            return (HANDLE)NULL;
#endif

        vp = RealLockResource16(h16, (PINT)&cb);
        if (vp == (VPVOID)NULL)
            return (ULONG)NULL;

        GETVDMPTR(vp, cb, pcurs16);

        if (pcurs16->cbWidth !=  (SHORT)(((pcurs16->cx + 0x0f) & ~0x0f) >> 3))
            return (ULONG)NULL;

        //
        // END: Check for Bogus handle
        //

        lpT = AllocCursorIconAlias();
        lpT->h16 = h16;
        lpT->hTask16 = CURRENTPTD()->htask16;

        lpT->vpData = vp;
        lpT->cbData = (WORD)cb;
        lpT->pbDataNew = (LPBYTE)pcurs16;

        lpT->pbDataOld = malloc_w(cb);
        if (lpT->pbDataOld) {
            RtlCopyMemory(lpT->pbDataOld, lpT->pbDataNew, cb);
        }

        lpT->h32  = (HAND32)W32CreateCursorIcon32(lpT);

        GlobalUnlock16(h16);
        FREEVDMPTR(pcurs16);
        lpT->pbDataNew = (LPBYTE)NULL;

        if (lpT->h32) {
            lpT->fInUse = TRUE;
            SetCursorIconFlag(h16, TRUE);
        }
        else
            lpT->fInUse = FALSE;

        return lpT->h32;
    }
}


//*****************************************************************************
//
// GetCursorIconAlias16 -
//
//     Returns a 16bit handle  given a 32bit Cursor or Icon HANDLE
//     Creates the 16bit Cursor or Icon if necessary.
//
//     Returns the 16bit handle
//
//*****************************************************************************


HAND16 GetCursorIconAlias16(HAND32 h32, UINT flType)
{

    LPCURSORICONALIAS lpT;

    if (h32 == (HAND32)0)
        return (HAND16)NULL;

    lpT = FindCursorIconAlias((ULONG)h32, HANDLE_32BIT);
    if (lpT) {
        return lpT->h16;
    }
    else {
        HAND16 h16;

        // HACK:
        // From experience: numeric values of 32bit standard cursors and icons
        //                  are very small. so check for these handles.
        //                  we should not create aliases for standard cursors and
        //                  icons here.

        WOW32ASSERT((UINT)h32 >= 100);

        //
        // Always generate valid handles.
        //

        h16 = W32Create16BitCursorIconFrom32BitHandle(h32, (HAND16)NULL,
                                                                  (PUINT)NULL);
        if (h16) {
            h16 = SetupCursorIconAlias((HAND16)NULL, h32, h16, flType,
                                                      NULL, (WORD)NULL);
        }
        return h16;
    }
}


//*****************************************************************************
//
// AllocCursorIconAlias -
//
//     Allocates and reurns pointer to CURSORICONALIAS buffer.
//
//*****************************************************************************


LPCURSORICONALIAS AllocCursorIconAlias()
{
    LPCURSORICONALIAS  lpT;

    for (lpT = lpCIAlias; lpT != NULL; lpT = lpT->lpNext) {
         if (!lpT->fInUse)
             break;
    }

    if (lpT == NULL) {
        lpT = (LPCURSORICONALIAS)malloc_w_small(sizeof(CURSORICONALIAS));
        if (lpT) {
            lpT->lpNext = lpCIAlias;
            lpCIAlias = lpT;
        }
        else {
            LOGDEBUG(0, ("AllocCursorIconAlias: malloc_w_small for alias failed\n"));
        }
    }

    if (lpT != NULL) {
        lpT->fInUse = TRUE;
        lpT->h16 = (HAND16)0;
        lpT->h32 = (HAND32)0;
        lpT->vpData   = (VPVOID)NULL;
        lpT->cLock    = 0;
        lpT->cbData   = 0;
        lpT->pbDataOld = (LPBYTE)NULL;
        lpT->pbDataNew = (LPBYTE)NULL;
        lpT->lpszName  = (LPBYTE)NULL;

        lpT->flType = HANDLE_TYPE_UNKNOWN;
        lpT->hInst16 = (HAND16)0;
        lpT->hMod16  = (HAND16)0;
        lpT->hTask16 = (HTASK16)0;
        lpT->hRes16 = 0;
    }

    return lpT;
}


//*****************************************************************************
//
// FindCursorIconAlias -
//
//     Searches for the given handle and returns corresponding
//     LPCURSORICONALIAS.
//
//*****************************************************************************


LPCURSORICONALIAS FindCursorIconAlias(ULONG hCI, UINT flHandleSize)
{
    LPCURSORICONALIAS  lpT;
    LPCURSORICONALIAS  lpTprev;

    lpTprev = (LPCURSORICONALIAS)NULL;
    for (lpT = lpCIAlias; lpT != NULL; lpTprev = lpT, lpT = lpT->lpNext) {
         if (lpT->fInUse) {
             if ((flHandleSize == HANDLE_16BIT && lpT->h16 == (HAND16)hCI) ||
                      (flHandleSize == HANDLE_32BIT && lpT->h32 == (HAND32)hCI))
                 break;
             else if (flHandleSize == HANDLE_16BITRES && lpT->hRes16 &&
                                 (lpT->hRes16 == (HAND16)hCI))


                 break;
         }

    }

    if (lpT) {
        if (lpTprev) {
            lpTprev->lpNext = lpT->lpNext;
            lpT->lpNext = lpCIAlias;
            lpCIAlias = lpT;
        }
    }
    return lpT;
}

#if defined(FE_SB)
//*****************************************************************************
//
// FindCursorIconAliasInUse -
//
//     Searches for the given handle and returns corresponding
//     lpT->fInUse.
//
//     09/27/96 bklee
//*****************************************************************************


BOOL FindCursorIconAliasInUse(ULONG hCI)
{
    LPCURSORICONALIAS  lpT;
    LPCURSORICONALIAS  lpTprev;

    lpTprev = (LPCURSORICONALIAS)NULL;
    for (lpT = lpCIAlias; lpT != NULL; lpTprev = lpT, lpT = lpT->lpNext) {
         if (lpT->h16 == (HAND16)hCI)
               return lpT->fInUse;
    }

    return TRUE;
}
#endif


//*****************************************************************************
//
// DeleteCursorIconAlias -
//
//     Searches for the given handle and if a 16bit handle frees the memory
//     allocated for the Object. The alias table is not freed.
//
//*****************************************************************************


BOOL DeleteCursorIconAlias(ULONG hCI, UINT flHandleSize)
{
    LPCURSORICONALIAS  lpT;

    WOW32ASSERT(flHandleSize == HANDLE_16BIT);

    for (lpT = lpCIAlias; lpT != NULL; lpT = lpT->lpNext) {
         if (lpT->fInUse && !(lpT->flType & HANDLE_TYPE_WOWGLOBAL)) {

             // Have we found the handle mapping?

             if (flHandleSize == HANDLE_16BIT && lpT->h16 == (HAND16)hCI) {

                 if (lpT->hTask16) {

                     // We don't want to free the handle mapping when
                     // the handle corresponds to a 16-bit resource, i.e.
                     // hRes16 is non-null.

                     if (!(lpT->hRes16)) {
                         SetCursorIconFlag(lpT->h16, FALSE);
                         GlobalUnlockFree16(RealLockResource16((HMEM16)hCI, NULL));
                         free_w(lpT->pbDataOld);
                         lpT->fInUse = FALSE;
                         return TRUE;
                     }
                 }
                 else {
                     WOW32ASSERT(FALSE);
                 }

                 break;
             }
         }

    }

    return FALSE;
}




//*****************************************************************************
//
// FreeCursorIconAlias -
//
//     Frees all Cursors and Icons of the specified task.
//
//
//*****************************************************************************


BOOL FreeCursorIconAlias(HAND16 hand16, ULONG ulFlags)
{
    LPCURSORICONALIAS  lpT;

    for (lpT = lpCIAlias; lpT != NULL; lpT = lpT->lpNext) {
         if (lpT->fInUse &&
            (((ulFlags & CIALIAS_HMOD)  && (lpT->hMod16  == hand16)) ||
             ((ulFlags & CIALIAS_HTASK) && (lpT->hTask16 == hand16)))) {

             if (ulFlags & CIALIAS_TASKISGONE) {
                 // We're here if this function is called after the task
                 // cleanup on the 16bit side... then we really can't
                 // callback. Setting appropriate fields to NULL will
                 // avoid callbacks, but will leak the corresponding
                 // memory. The asserts will catch this on a checked
                 // build.
                 WOW32ASSERT(lpT->h16==(HAND16)NULL);
                 WOW32ASSERT(lpT->hRes16==(HAND16)NULL);
                 lpT->h16 = (HAND16)NULL;
                 lpT->hRes16 = (HAND16)NULL;
             }
             InvalidateCursorIconAlias(lpT);
         }
    }

    return TRUE;
}


//*****************************************************************************
//
// SetupCursorIconAlias -
//
//     Sets up association (alias) between a 32bit and a 16bit handle.
//     given both the handles.
//
//
//*****************************************************************************


HAND16 SetupCursorIconAlias(HAND16 hInst16, HAND32 h32, HAND16 h16, UINT flType,
                            LPBYTE lpResName, WORD hRes16)

{
    LPCURSORICONALIAS  lpT;
    VPVOID             vp;
    INT                cb;

    lpT = AllocCursorIconAlias();
    lpT->fInUse = TRUE;
    lpT->h16 = h16;
    lpT->h32 = h32;
    lpT->flType = (BYTE)flType;
    if (!(flType & HANDLE_TYPE_WOWGLOBAL)) {
        lpT->hInst16 = hInst16;
        lpT->hMod16  = GETHMOD16(HMODINST32(hInst16));
        lpT->hTask16 = CURRENTPTD()->htask16;
        lpT->hRes16 = hRes16;

        vp = RealLockResource16(h16, &cb);
        if (vp == (VPVOID)NULL)
            return (HAND16)NULL;

        lpT->vpData = vp;
        lpT->cbData = (WORD)cb;
        GETVDMPTR(vp, cb, lpT->pbDataNew);

        lpT->pbDataOld = malloc_w(cb);
        if (lpT->pbDataOld) {
            RtlCopyMemory(lpT->pbDataOld, lpT->pbDataNew, cb);
        }

        if (hRes16) {
            lpT->lpszName = lpResName;
            if ((WORD)HIWORD(lpResName) != (WORD)NULL) {
                UINT   cb;
                cb = strlen(lpResName)+1;
                if (lpT->lpszName = malloc_w_small(cb)) {
                    memcpy (lpT->lpszName, lpResName, cb);
                }
            }
        }


    }
    // the alias has been setup. Now turn on the GAH_CURSORICON flag.

    SetCursorIconFlag(h16, TRUE);

    return h16;
}



//*****************************************************************************
//
// SetupResCursorIconAlias -
//
//     Sets up association (alias) between a 32bit and a 16bit handle.
//     given the 32bit handle and a handle to a 16bit resource.
//
//
//*****************************************************************************


HAND16 SetupResCursorIconAlias(HAND16 hInst16, HAND32 h32, LPBYTE lpResName, WORD hRes16, UINT flType)
{
    LPCURSORICONALIAS  lpT;
    HAND16 h16 = 0;
    HAND16 h16Res = 0;
    UINT   cb;


    if (hRes16) {
        // 16bit resource has been loaded. We always want to return the
        // SAME 16bit handle no matter howmany times the 'LoadIcon' or
        // LoadCursor has been called.

        h16Res = LOWORD(hRes16);
        lpT = FindCursorIconAlias(h16Res, HANDLE_16BITRES);
    }
    else {

        // Resource handle is NULL. The Resource must have been a
        // standard predefined resource like ARROW etc.

        lpT = FindCursorIconAlias((ULONG)h32, HANDLE_32BIT);
        flType |= HANDLE_TYPE_WOWGLOBAL;
    }

    if (lpT == NULL) {
        h16 = W32Create16BitCursorIconFrom32BitHandle(h32, hInst16, &cb);
        h16 = SetupCursorIconAlias(hInst16, h32, h16, flType, lpResName, hRes16);
    }
    else {
        if (lpT->flType & HANDLE_TYPE_WOWGLOBAL) {

            // eachtime we should get the same h32 from usersrv.
            //

            WOW32ASSERT(lpT->h32 == h32);
        }
        else {
            if (lpT->h32 != h32) {
                if (lpT->flType == HANDLE_TYPE_CURSOR)
                    DestroyCursor(h32);
                else
                    DestroyIcon(h32);
            }
            ReplaceCursorIcon(lpT);
        }

        h16 = lpT->h16;
    }

    return h16;
}


//*****************************************************************************
//
// SetCursorIconFlag  -
//
//     Sets/Clears the GAH_CURSORICONFLAG in the global arean header. This flag
//     is used to identify Cursors and Icon when they are GlobaLocked and
//     GlobalUnlocked
//
//*****************************************************************************

ULONG SetCursorIconFlag(HAND16 h16, BOOL fSet)
{
    PARM16 Parm16;
    VPVOID vp = 0;

    Parm16.WndProc.wParam = h16;
    Parm16.WndProc.wMsg = (WORD)fSet;
    CallBack16(RET_SETCURSORICONFLAG, &Parm16, 0, &vp);
    return (ULONG)0;
}


//*****************************************************************************
//
// UpdateCursorIcon  -
//
//     Compares the new object data with the old. If any of the bytes differ
//     the old object is replaced with the new.
//
//*****************************************************************************

VOID UpdateCursorIcon()
{
    LPCURSORICONALIAS  lpT;
    UINT               cbData;
    LPBYTE             lpBitsNew, lpBitsOld;
    UINT               i = 0;

    for (lpT = lpCIAlias; lpT != NULL ; lpT = lpT->lpNext) {
         if (lpT->fInUse && lpT->cLock) {
             GETVDMPTR(lpT->vpData, lpT->cbData, lpT->pbDataNew);
             if (lpT->hRes16) {
                 if (lpT->flType == HANDLE_TYPE_ICON) {
                     lpBitsNew = lpT->pbDataNew + sizeof(BITMAPINFOHEADER16);
                     lpBitsOld = lpT->pbDataOld + sizeof(BITMAPINFOHEADER16);
                     cbData    = lpT->cbData    - sizeof(BITMAPINFOHEADER16);
                 }
                 else {
                     lpBitsNew = lpT->pbDataNew + sizeof(CURSORRESOURCE16);
                     lpBitsOld = lpT->pbDataOld + sizeof(CURSORRESOURCE16);
                     cbData    = lpT->cbData    - sizeof(CURSORRESOURCE16);
                 }

             }
             else {
                 lpBitsNew = lpT->pbDataNew + sizeof(CURSORSHAPE16);
                 lpBitsOld = lpT->pbDataOld + sizeof(CURSORSHAPE16);
                 cbData    = lpT->cbData    - sizeof(CURSORSHAPE16);
             }

             if (! RtlEqualMemory(lpBitsNew, lpBitsOld, cbData))
                 ReplaceCursorIcon(lpT);

             if (cPendingCursorIconUpdates == ++i)
                 break;
         }

    }

}

//*****************************************************************************
//
// ReplaceCursorIcon  -
//
//     Updates the current cursor or icon. Creates a new icon or cursor and
//     replaces the contents of the old handle with that of the new.
//
//     returns TRUE for success.
//
//*****************************************************************************

BOOL ReplaceCursorIcon(LPCURSORICONALIAS lpIn)
{
    HANDLE hT32;


    if (lpIn != NULL) {

        // Get the data

        GETVDMPTR(lpIn->vpData, lpIn->cbData, lpIn->pbDataNew);

        // Create the object

        hT32  = (HAND32)W32CreateCursorIcon32(lpIn);

        // SetCursorConents will replace the contents of OLD cursor/icon
        // with that of the new handle and destroy the new handle

        SetCursorContents(lpIn->h32, hT32);

        // replace the old object data with the new

        RtlCopyMemory(lpIn->pbDataOld, lpIn->pbDataNew, lpIn->cbData);
        FREEVDMPTR(lpIn->pbDataNew);
        lpIn->pbDataNew = (LPBYTE)NULL;

    }


    return (BOOL)TRUE;

}


//*****************************************************************************
//
// WK32WowCursorIconOp -
//
//     Gets called when/from  GlobalLock or GlobalUnlock are called. The fLock
//     flag is TRUE if called from GlobalLock else it is FALSE.
//
//*****************************************************************************

BOOL FASTCALL WK32WowCursorIconOp(PVDMFRAME pFrame)
{

    PWOWCURSORICONOP16 prci16;
    HAND16 h16;
    LPCURSORICONALIAS lpT;
    BOOL   fLock;
    WORD   wFuncId;
    UINT   cLockT;


    GETARGPTR(pFrame, sizeof(WOWCURSORICONOP16), prci16);
    wFuncId = FETCHWORD(prci16->wFuncId);
    h16 = (HAND16)FETCHWORD(prci16->h16);

    lpT = FindCursorIconAlias((ULONG)h16, HANDLE_16BIT);
    // This is a Cursor or Icon
    if (lpT != NULL) {

        if (wFuncId == FUN_GLOBALLOCK || wFuncId == FUN_GLOBALUNLOCK) {

            if (!(lpT->flType & HANDLE_TYPE_WOWGLOBAL)) {

                fLock = (wFuncId == FUN_GLOBALLOCK);

                // Store the current lockcount.

                cLockT = lpT->cLock;

                // Update the Lock count

                lpT->cLock = fLock ? ++lpT->cLock : --lpT->cLock;

                if (lpT->cLock == 0) {

                    // New lock count == 0 implies that it was decremented from
                    // 1 to 0 thereby impling that it was one of the cursors that
                    // was being updated regularly.

                    // Decrement the global count and update the cursor one last
                    // time

                    cPendingCursorIconUpdates--;
                    ReplaceCursorIcon(lpT);
                }
                else if (fLock && cLockT == 0) {

                    // If previous Lockcount was zero and the object is being locked
                    // then it means that this is the very first time that the object
                    // is being locked

                    cPendingCursorIconUpdates++;
                }
            }
        }
        else if (wFuncId == FUN_GLOBALFREE) {

            // The h16 has not yet been GlobalFreed. We return TRUE if h16 can
            // be freed else FALSE. The h16 can be freed only if it is not a
            // global handle. ie, it doesn't correspond to a predefined cursor

            // Also we donot free the handle if h16 corresponds to a resource.
            // CorelDraw 3.0 calls FreeResource(h16) and then SetCursor(h16)
            // thus  GPing.

            BOOL fFree;

            fFree = !((lpT->flType & HANDLE_TYPE_WOWGLOBAL) || lpT->hRes16);
            if (fFree) {
                // Set handle to NULL so that InvalidateCursorIconAlias
                // doesn't try to free it.

                lpT->h16 = 0;
                InvalidateCursorIconAlias(lpT);
            }

            return (BOOL)fFree;

        }
        else {
            LOGDEBUG(0, ("WK32WowCursorIconOp: Unknown Func Id\n"));
        }
    }

    // else if this is a GlobalFree call
    else if (wFuncId == FUN_GLOBALFREE) {

        // and if this is a handle to an accelerator
        if(lpT = (LPCURSORICONALIAS)FindAccelAlias((HANDLE)h16, HANDLE_16BIT)) {

            // free it from the accelerator alias list
            FreeAccelAliasEntry((LPACCELALIAS) lpT);

            // cause this hMem16 to really be free'd in 16-bit GlobalFree
            return TRUE;
        }
    }

    return TRUE;
}


//*****************************************************************************
//
// W32Create16BitResCursorIconFrom32BitHandle -
//
//     Creates a WIN31 compatible Cursor or Icon given a 32bit cursor or icon
//     handle. This is primarily used to create a 16bit Cursor or Icon which
//     has been loaded from a 16bit resource.
//
//
//     returns 16bit handle
//*****************************************************************************


HAND16 W32Create16BitCursorIconFrom32BitHandle(HANDLE h32, HAND16 hInst16,
                                                 PUINT pcbData)
{
    HAND16   h16 = 0;
    ICONINFO iinfo;
    BITMAP   bm;
    BITMAP   bmClr;
    UINT     nBytesAND = 0;
    UINT     nBytesXOR = 0;
    LPBYTE   lpBitsAND, lpBitsXOR;

    if (GetIconInfo(h32, &iinfo)) {
        if (GetObject(iinfo.hbmMask, sizeof(BITMAP), &bm)) {
            nBytesAND = GetBitmapBits(iinfo.hbmMask, 0, (LPBYTE)NULL);
            WOW32WARNMSG(nBytesAND,("WOW: W32C16BCIFBH: nBytesAND == 0\n"));
            if (iinfo.hbmColor) {
                GetObject(iinfo.hbmColor, sizeof(BITMAP), &bmClr);
                nBytesXOR = GetBitmapBits(iinfo.hbmColor, 0, (LPBYTE)NULL);
                WOW32WARNMSG(nBytesXOR,("WOW: W32C16BCIFBH: nBytesAND == 0\n"));
            }
            else {
                bm.bmHeight /= 2;
                nBytesAND /= 2;
                nBytesXOR = nBytesAND;
            }


            if (pcbData) {
                *pcbData = nBytesAND + nBytesXOR + sizeof(CURSORSHAPE16);
            }

            lpBitsAND = malloc_w(nBytesAND + nBytesXOR);
            if (lpBitsAND != NULL) {
                lpBitsXOR = lpBitsAND + nBytesAND;
                GetBitmapBits(iinfo.hbmMask,
                              (iinfo.hbmColor) ? nBytesAND : (nBytesAND * 2),
                                                                    lpBitsAND);
                if (iinfo.hbmColor)
                    GetBitmapBits(iinfo.hbmColor, nBytesXOR, lpBitsXOR);

                h16 = W32Create16BitCursorIcon(hInst16,
                                       iinfo.xHotspot, iinfo.yHotspot,
                                       bm.bmWidth, bm.bmHeight,
                                       (iinfo.hbmColor) ? bmClr.bmPlanes :
                                                                    bm.bmPlanes,
                                       (iinfo.hbmColor) ? bmClr.bmBitsPixel :
                                                                 bm.bmBitsPixel,
                                       lpBitsAND, lpBitsXOR,
                                       (INT)nBytesAND, (INT)nBytesXOR);
                free_w(lpBitsAND);

            }

        }
        DeleteObject(iinfo.hbmMask);
        if (iinfo.hbmColor) {
            DeleteObject(iinfo.hbmColor);
        }
    }

    return h16;

}

//*****************************************************************************
//
// GetClassCursorIconAlias32 -
//
//     Returns a 32bit handle  given a 16bit Cursor or Icon HANDLE
//     DOES NOT Create the 32bit Cursor or Icon if there is no alias.
//     This is called in RegisterClass only - to support those apps which
//     pass a bogus handle for WNDCLASS.hIcon.
//
//     Returns the 32bit handle
//
//*****************************************************************************


HANDLE GetClassCursorIconAlias32(HAND16 h16)
{

    LPCURSORICONALIAS lpT;

    if (h16 == (HAND16)0)
        return (ULONG)NULL;

    lpT = FindCursorIconAlias((ULONG)h16, HANDLE_16BIT);
    if (lpT) {
        return lpT->h32;
    }
    else
        return (HANDLE)NULL;
}



//*****************************************************************************
//
// InvalidateCursorIconAlias -
//
//     Frees the allocated objects.
//
//*****************************************************************************


VOID InvalidateCursorIconAlias(LPCURSORICONALIAS lpT)
{
    VPVOID vp=0;
    PARM16 Parm16;

    if (!lpT->fInUse)
        return;

    if (lpT->h16) {
        SetCursorIconFlag(lpT->h16, FALSE);
        GlobalUnlockFree16(RealLockResource16((HMEM16)lpT->h16, NULL));
    }

    if (lpT->hRes16) {
        Parm16.WndProc.wParam = (HAND16) lpT->hRes16;
        CallBack16(RET_FREERESOURCE, &Parm16, 0, &vp);
    }

    if (lpT->h32) {
        if (lpT->flType == HANDLE_TYPE_CURSOR)
            DestroyCursor(lpT->h32);
        else
            DestroyIcon(lpT->h32);
    }

    if (lpT->pbDataOld)
        free_w(lpT->pbDataOld);

    if (lpT->cLock)
        cPendingCursorIconUpdates--;


    if ((WORD)HIWORD(lpT->lpszName) != (WORD)NULL) {
        free_w_small ((PVOID)lpT->lpszName);
    }

    lpT->fInUse = FALSE;
}


//*****************************************************************************
//
// InitStdCursorIconAlias -
//
//     Creates the aliases of standard cursors and icons.
//
// NOTES:
//
// The idea is to createaliases for all the standard cursors and icons to
// make sure that we indeed generate valid handles.
//
// This problem cameup because of the following scenario
// the app turbotax does the following:
//
//          h16Cursor1 = GetClassWord(hwndEditControl, GCL_HCURSOR);
//                              (bydefault, this is an I-beam)
//                         .....
//          h16Cursor2 = LoadCursor(NULL, IDC_IBEAM);
// Because of the way we create and maintain our 32-16 alias hCursor1 is a
// a WOW bogus handle (ie > 0xf000) and  since by default the "Edit" class is
// registered with hCursor = IDC_IBEAM, the h32s are same ie.
//
//     GetClassWord(hwndEditControl, GCL_HCURSOR) == LoadCursor(..IDC_IBEAM);
//
// Thus h16Cursor2 will be same as h16Cursor1 and that's a problem because we
// are NOT returning a valid wow handle for a predefined cursor.
//
//
// The solution is to createaliases for all standard cursors and icons during
// init time so that we don't run into this problem. However I think this
// approach as wasteful and am creating the alias for the only known case
// ie IDC_IBEAM.
//
//                                           - Nanduri Ramakrishna
//*****************************************************************************

DWORD InitCursorIds[] = {
                          (DWORD)IDC_ARROW,
                          (DWORD)IDC_IBEAM,
                          (DWORD)IDC_WAIT,
                          (DWORD)IDC_CROSS,
                          (DWORD)IDC_UPARROW,
                          (DWORD)IDC_SIZE,
                          (DWORD)IDC_ICON,
                          (DWORD)IDC_SIZENWSE,
                          (DWORD)IDC_SIZENESW,
                          (DWORD)IDC_SIZEWE,
                          (DWORD)IDC_SIZENS
                        };

BOOL InitStdCursorIconAlias()
{

    HCURSOR h32;
    UINT i;

    for (i = 0; i < (sizeof(InitCursorIds) / sizeof(DWORD)); i++) {

         //
         // Create the alias for each standard cursor in the list
         //

         h32 = (HCURSOR)LoadCursor((HINSTANCE)NULL, (LPCSTR)InitCursorIds[i]);
         WOW32ASSERT(h32);

         if (h32) {
             SetupResCursorIconAlias((HAND16)NULL, (HAND32)h32, NULL, (WORD)NULL,
                                                          HANDLE_TYPE_CURSOR);
         }

    }

    //
    // Add similar lines for  standard icons.
    //

    return TRUE;
}


//*****************************************************************************
//
// W32CheckIfAlreadyLoaded -
//
//     returns h16 if a cursoricon has previously been loaded.
//
//*****************************************************************************

HAND16 W32CheckIfAlreadyLoaded(VPVOID pData, WORD ResType)
{
    LPCURSORICONALIAS  lpT;
    PICONCUR16 parg16;
    PSZ psz;


    GETMISCPTR(pData, parg16);
    GETPSZIDPTR(parg16->lpStr, psz);

    ResType = (ResType == NW_LOADCURSOR) ?  HANDLE_TYPE_CURSOR : HANDLE_TYPE_ICON;
    for (lpT = lpCIAlias; lpT != NULL; lpT = lpT->lpNext) {
         if (lpT->fInUse) {
             LPBYTE lpszNameT = lpT->lpszName;
             if (lpszNameT &&  (lpT->flType & ResType) &&
                                            lpT->hInst16 == parg16->hInst) {
                 WOW32ASSERT(!(lpT->flType & HANDLE_TYPE_WOWGLOBAL));
                 if (HIWORD(lpszNameT) && HIWORD(psz)) {
                     if (!(WOW32_stricmp(psz, (LPSTR)lpszNameT)))
                         break;
                 }
                 else if (lpszNameT == psz) {
                    break;
                 }
             }
         }
    }


    FREEPSZIDPTR(psz);
    FREEMISCPTR(parg16);


    if (lpT && lpT->cLock)
        ReplaceCursorIcon(lpT);

    return (lpT ? lpT->h16 : 0);
}
