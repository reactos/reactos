/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WCALL16.C
 *  WOW32 16-bit message/callback support
 *
 *  History:
 *  Created 11-Mar-1991 by Jeff Parsons (jeffpar)
 *  Changed 18-Aug-1992 by Mike Tricker (MikeTri) Added DOS PDB and SFT functions
--*/


#include "precomp.h"
#pragma hdrstop


MODNAME(wcall16.c);

#define WOWFASTEDIT

#ifdef WOWFASTEDIT

typedef struct _LOCALHANDLEENTRY {
    WORD    lhe_address;    // actual address of object
    BYTE    lhe_flags;      // flags and priority level
    BYTE    lhe_count;      // lock count
} LOCALHANDLEENTRY, *PLOCALHANDLEENTRY;

#define LA_MOVEABLE     0x0002      // moveable or fixed?

#define LHE_DISCARDED   0x0040      // Marks objects that have been discarded.

#endif

/* Common callback functions
 */
HANDLE LocalAlloc16(WORD wFlags, INT cb, HANDLE hInstance)
{
    PARM16 Parm16;
    VPVOID vp = 0;

    if (LOWORD(hInstance) == 0 ) {     /* if lo word == 0, then this is a 32-bit
					   hInstance, which makes no sense */
	WOW32ASSERT(LOWORD(hInstance));
        return (HANDLE)0;
    }

    if (cb < 0 || cb > 0xFFFF) {
        WOW32ASSERT(cb > 0 && cb <= 0xFFFF);
        return (HANDLE)0;
    }

    Parm16.WndProc.wMsg = LOWORD(hInstance) | 1;

    Parm16.WndProc.wParam = wFlags;
    Parm16.WndProc.lParam = cb;
    CallBack16(RET_LOCALALLOC, &Parm16, 0, &vp);

    if (LOWORD(vp) == 0)
        vp = 0;

    return (HANDLE)vp;
}


HANDLE LocalReAlloc16(HANDLE hMem, INT cb, WORD wFlags)
{
    PARM16 Parm16;
    VPVOID vp = 0;

    if (HIWORD(hMem) == 0 || cb < 0 || cb > 0xFFFF) {
        WOW32ASSERT(HIWORD(hMem) && cb >= 0 && cb <= 0xFFFF);
        return (HANDLE)0;
    }

    LOGDEBUG(4,("LocalRealloc DS = %x, hMem = %x, bytes = %x, flags = %x\n",HIWORD(hMem),LOWORD(hMem),cb,wFlags));
    Parm16.WndProc.lParam = (LONG)hMem;
    Parm16.WndProc.wParam = wFlags;
    Parm16.WndProc.wMsg = (WORD)cb;
    CallBack16(RET_LOCALREALLOC, &Parm16, 0, &vp);

    if (LOWORD(vp) == 0)
        vp = 0;

    return (HANDLE)vp;
}

#ifndef WOWFASTEDIT

VPVOID LocalLock16(HANDLE hMem)
{
    PARM16 Parm16;
    VPVOID vp = 0;

    if (HIWORD(hMem) == 0) {
        WOW32ASSERT(HIWORD(hMem) != 0);
        return (VPVOID)0;
    }

    Parm16.WndProc.lParam = (LONG)hMem;
    CallBack16(RET_LOCALLOCK, &Parm16, 0, &vp);

    return vp;
}

BOOL LocalUnlock16(HANDLE hMem)
{
    PARM16 Parm16;
    VPVOID vp = FALSE;

    if (HIWORD(hMem) == 0) {
        WOW32ASSERT(HIWORD(hMem));
        return FALSE;
    }

    Parm16.WndProc.lParam = (LONG)hMem;
    CallBack16(RET_LOCALUNLOCK, &Parm16, 0, &vp);

    return (BOOL)vp;
}

#else

VPVOID LocalLock16(HANDLE hMem)
{
    WORD    h16;
    LONG    retval;

    if (HIWORD(hMem) == 0) {
        WOW32ASSERT(HIWORD(hMem) != 0);
        return (VPVOID)0;
    }

    h16 = LOWORD(hMem);
    retval = (VPVOID)hMem;

    if (h16 & LA_MOVEABLE) {
        PLOCALHANDLEENTRY plhe;

        GETVDMPTR(hMem, sizeof(*plhe), plhe);

        if (plhe->lhe_flags & LHE_DISCARDED) {
            goto LOCK1;
        }

        plhe->lhe_count++;
        if (!plhe->lhe_count)
            plhe->lhe_count--;

LOCK1:
        LOW(retval) = plhe->lhe_address;
        FLUSHVDMPTR((ULONG)hMem, sizeof(*plhe), plhe);
        FREEVDMPTR(plhe);
    }

    if (LOWORD(retval) == 0)
        retval = 0;

    return retval;
}

BOOL LocalUnlock16(HANDLE hMem)
{
    WORD    h16;
    BOOL    rc;
    PLOCALHANDLEENTRY plhe;
    BYTE    count;

    if (HIWORD(hMem) == 0) {
        WOW32ASSERT(HIWORD(hMem));
        return FALSE;
    }

    rc = FALSE;
    h16 = LOWORD(hMem);

    if (!(h16 & LA_MOVEABLE)) {
        goto UNLOCK2;
    }

    GETVDMPTR(hMem, sizeof(*plhe), plhe);

    if (plhe->lhe_flags & LHE_DISCARDED)
        goto UNLOCK1;

    count = plhe->lhe_count;
    count--;

    if (count >= (BYTE)(0xff-1))
        goto UNLOCK1;

    plhe->lhe_count = count;
    rc = (BOOL)((SHORT)count);

    FLUSHVDMPTR((ULONG)hMem, sizeof(*plhe), plhe);

UNLOCK1:
    FREEVDMPTR(plhe);

UNLOCK2:
    return rc;
}

#endif  // WOWFASTEDIT


WORD LocalSize16(HANDLE hMem)
{
    PARM16 Parm16;
    VPVOID vp = 0;

    if (HIWORD(hMem) == 0) {
        WOW32ASSERT(HIWORD(hMem));
        return FALSE;
    }

    Parm16.WndProc.lParam = (LONG)hMem;
    CallBack16(RET_LOCALSIZE, &Parm16, 0, &vp);

    return (WORD)vp;
}


HANDLE LocalFree16(HANDLE hMem)
{
    PARM16 Parm16;
    VPVOID vp = FALSE;

    if (HIWORD(hMem) == 0) {
        WOW32ASSERT(HIWORD(hMem));
        return (HANDLE)0;
    }

    Parm16.WndProc.lParam = (LONG)hMem;
    CallBack16(RET_LOCALFREE, &Parm16, 0, &vp);

    if (LOWORD(vp) == 0) {
        vp = 0;
    } else {
        WOW32ASSERT(LOWORD(vp) == LOWORD(hMem));
        vp = (VPVOID)hMem;
    }

    return (HANDLE)vp;
}


BOOL LockSegment16(WORD wSeg)
{
    PARM16 Parm16;
    VPVOID vp = FALSE;

    Parm16.WndProc.wParam = wSeg;
    CallBack16(RET_LOCKSEGMENT, &Parm16, 0, &vp);

    return (BOOL)vp;
}


BOOL UnlockSegment16(WORD wSeg)
{
    PARM16 Parm16;
    VPVOID vp = FALSE;

    Parm16.WndProc.wParam = wSeg;
    CallBack16(RET_UNLOCKSEGMENT, &Parm16, 0, &vp);

    return (BOOL)vp;
}


VPVOID  WOWGlobalAllocLock16(WORD wFlags, DWORD cb, HMEM16 *phMem)
{
    PARM16 Parm16;
    VPVOID vp = 0;

    Parm16.WndProc.wParam = wFlags;
    Parm16.WndProc.lParam = cb;
    CallBack16(RET_GLOBALALLOCLOCK, &Parm16, 0, &vp);

    if (vp) {

        // Get handle of 16-bit object
        if (phMem) {
            *phMem = Parm16.WndProc.wParam;
        }
    }
    return vp;
}


HMEM16 WOWGlobalAlloc16(WORD wFlags, DWORD cb)
{
    HMEM16 hMem;
    VPVOID vp;

    if (vp = WOWGlobalAllocLock16(wFlags, cb, &hMem)) {
        WOWGlobalUnlock16(hMem);
    } else {
        hMem = 0;
    }

    return hMem;
}


VPVOID  WOWGlobalLockSize16(HMEM16 hMem, PDWORD pcb)
{
    PARM16 Parm16;
    VPVOID vp = 0;

    Parm16.WndProc.wParam = hMem;
    CallBack16(RET_GLOBALLOCK, &Parm16, 0, &vp);

    // Get size of 16-bit object    (will be 0 if lock failed)
    if (pcb) {
        *pcb = Parm16.WndProc.lParam;
    }

    return vp;
}


VPVOID WOWGlobalLock16(HMEM16 hMem)
{
    return WOWGlobalLockSize16(hMem, NULL);
}


BOOL WOWGlobalUnlock16(HMEM16 hMem)
{
    PARM16 Parm16;
    VPVOID vp = FALSE;

    Parm16.WndProc.wParam = hMem;
    CallBack16(RET_GLOBALUNLOCK, &Parm16, 0, &vp);

    return (BOOL)vp;
}


HMEM16 WOWGlobalUnlockFree16(VPVOID vpMem)
{
    PARM16 Parm16;
    VPVOID vp = FALSE;

    Parm16.WndProc.lParam = vpMem;
    CallBack16(RET_GLOBALUNLOCKFREE, &Parm16, 0, &vp);

    return (HMEM16)vp;
}


HMEM16 WOWGlobalFree16(HMEM16 hMem)
{
    VPVOID vp;

    if (vp = WOWGlobalLock16(hMem)) {
        hMem = WOWGlobalUnlockFree16(vp);
    } else {
        // On failure we return the passed-in handle,
        // so there's nothing to do.
    }

    return hMem;
}


HAND16 GetExePtr16( HAND16 hInst )
{
    PARM16 Parm16;
    ULONG ul;
    PTD ptd;
    INT i;

    if (hInst == 0) return (HAND16)0;

    //
    // see if this is the hInst for the current task
    //

    ptd = CURRENTPTD();

    if (hInst == ptd->hInst16) {
        return ptd->hMod16;
    }

    //
    // check the cache
    //

    for (i = 0; i < CHMODCACHE; i++) {
        if (ghModCache[i].hInst16 == hInst)
            return ghModCache[i].hMod16;
    }

    /*
    ** Function returns a hModule, given an hInstance
    */
    Parm16.WndProc.wParam = hInst;
    CallBack16(RET_GETEXEPTR, &Parm16, 0, &ul);


    //
    // GetExePtr(hmod) returns hmod, don't cache these.
    //

    if (hInst != (HAND16)LOWORD(ul)) {

        //
        // update the cache
        // slide everybody down 1 entry, put this new guy at the top
        //

        RtlMoveMemory(ghModCache+1, ghModCache, sizeof(HMODCACHE)*(CHMODCACHE-1));
        ghModCache[0].hInst16 = hInst;
        ghModCache[0].hMod16 = (HAND16)LOWORD(ul);
    }

    return (HAND16)LOWORD(ul);
}


WORD GetModuleFileName16( HAND16 hInst, VPVOID lpszModuleName, WORD cchModuleName )
{
    PARM16 Parm16;
    ULONG ul;


    if (hInst == 0) return 0;

    Parm16.WndProc.wParam = hInst;
    Parm16.WndProc.lParam = lpszModuleName;
    Parm16.WndProc.wMsg   = cchModuleName;
    CallBack16(RET_GETMODULEFILENAME, &Parm16, 0, &ul );

    return( LOWORD(ul) );
}


ULONG GetDosPDB16(VOID)
{
    PARM16 Parm16;
    DWORD dwReturn = 0;

    CallBack16(RET_GETDOSPDB, &Parm16, 0, &dwReturn);

    return (ULONG)dwReturn;
}


ULONG GetDosSFT16(VOID)
{
    PARM16 Parm16;
    DWORD dwReturn = 0;

    CallBack16(RET_GETDOSSFT, &Parm16, 0, &dwReturn);

    return (ULONG)dwReturn;
}

// Given a data selector change it into a code selector

WORD ChangeSelector16(WORD wSeg)
{
    PARM16 Parm16;
    VPVOID vp = FALSE;

    Parm16.WndProc.wParam = wSeg;
    CallBack16(RET_CHANGESELECTOR, &Parm16, 0, &vp);

    return LOWORD(vp);
}

VPVOID RealLockResource16(HMEM16 hMem, PINT pcb)
{
    PARM16 Parm16;
    VPVOID vp = 0;

    Parm16.WndProc.wParam = hMem;
    CallBack16(RET_LOCKRESOURCE, &Parm16, 0, &vp);

    // Get size of 16-bit object    (will be 0 if lock failed)
    if (pcb) {
        *pcb = Parm16.WndProc.lParam;
    }

    return vp;
}

int WINAPI WOWlstrcmp16(LPCWSTR lpString1, LPCWSTR lpString2)
{
    PARM16 Parm16;
    DWORD dwReturn = 0;
    DWORD cb1, cb2;
    VPSTR vp1, vp2;
    LPSTR p1, p2;

    //
    // to handle DBCS correctly allocate enough room
    // for two DBCS bytes for every unicode char.
    //

    cb1 = sizeof(WCHAR) * (wcslen(lpString1) + 1);
    cb2 = sizeof(WCHAR) * (wcslen(lpString2) + 1);

    // be sure allocation size matches stackfree16() size below
    vp1 = stackalloc16(cb1 + cb2);
    vp2 = vp1 + cb1;

    p1 = VDMPTR(vp1, cb1);
    p2 = p1 + cb1;

    RtlUnicodeToMultiByteN(
        p1,
        cb1,
        NULL,
        (LPWSTR) lpString1,   // cast because arg isn't declared const
        cb1
        );

    RtlUnicodeToMultiByteN(
        p2,
        cb2,
        NULL,
        (LPWSTR) lpString2,   // cast because arg isn't declared const
        cb2
        );

    FREEVDMPTR(p1);

    Parm16.lstrcmpParms.lpstr1 = vp1;
    Parm16.lstrcmpParms.lpstr2 = vp2;

    CallBack16(RET_LSTRCMP, &Parm16, 0, &dwReturn);

    stackfree16(vp1, (cb1 + cb2));

    return (int)(short int)LOWORD(dwReturn);
}


DWORD WOWCallback16(DWORD vpFn, DWORD dwParam)
{
    PARM16 Parm16;
    VPVOID vp;

    //
    // Copy DWORD parameter to PARM16 structure.
    //

    RtlCopyMemory(&Parm16.WOWCallback16.wArgs, &dwParam, sizeof(dwParam));

    //
    // Use semi-slimy method to pass argument size to CallBack16.
    //

    vp = (VPVOID) sizeof(dwParam);

    CallBack16(RET_WOWCALLBACK16, &Parm16, (VPPROC)vpFn, &vp);

    return (DWORD)vp;
}


BOOL WOWCallback16Ex(
         DWORD vpFn,
         DWORD dwFlags,
         DWORD cbArgs,
         PVOID pArgs,
         PDWORD pdwRetCode
         )
{
#ifdef DEBUG
    static BOOL fFirstTime = TRUE;

    if (fFirstTime) {

        //
        // Ensure that wownt32.h's definition of WCB16_MAX_CBARGS
        // matches wow.h's definition of PARMWCB16.
        //

        WOW32ASSERT( WCB16_MAX_CBARGS == sizeof(PARMWCB16) );

        //
        // If the PARMWCB16 structure is smaller than the PARM16
        // union, we should increase the size of PARMWCB16 and
        // WCB16_MAX_CBARG to allow the use of the extra bytes.
        //

        WOW32ASSERT( sizeof(PARMWCB16) == sizeof(PARM16) );

        fFirstTime = FALSE;
    }
#endif // DEBUG

    if (cbArgs > sizeof(PARM16)) {
        LOGDEBUG(LOG_ALWAYS, ("WOWCallback16V: cbArgs = %u, must be <= %u",
                              cbArgs, (unsigned) sizeof(PARM16)));
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // For cdecl functions we don't want to "sub SP, cbArgs" after calling
    // the function, so we pass 0 as cbArgs to the 16-bit side.
    //

    if (dwFlags & WCB16_CDECL) {
        cbArgs = 0;
    }

    //
    // Use semi-slimy method to pass argument size to CallBack16.
    //

    *pdwRetCode = cbArgs;

    CallBack16(RET_WOWCALLBACK16, (PPARM16)pArgs, (VPPROC)vpFn, (PVPVOID)pdwRetCode);

    return TRUE;
}


BOOL CallBack16(INT iRetID, PPARM16 pParm16, VPPROC vpfnProc, PVPVOID pvpReturn)
{
#ifdef DEBUG
    static PSZ apszCallBacks[] = {
    "ERROR:RETURN",         // RET_RETURN       (not a callback!)
    "ERROR:DEBUGRETURN",    // RET_DEBUGRETURN  (not a callback!)
    "DEBUG",                // RET_DEBUG
    "WNDPROC",              // RET_WNDPROC
    "ENUMFONTPROC",         // RET_ENUMFONTPROC
    "ENUMWINDOWPROC",       // RET_ENUMWINDOWPROC
    "LOCALALLOC",           // RET_LOCALALLOC
    "LOCALREALLOC",         // RET_LOCALREALLOC
    "LOCALLOCK",            // RET_LOCALLOCK
    "LOCALUNLOCK",          // RET_LOCALUNLOCK
    "LOCALSIZE",            // RET_LOCALSIZE
    "LOCALFREE",            // RET_LOCALFREE
    "GLOBALALLOCLOCK",      // RET_GLOBALALLOCLOCK
    "GLOBALLOCK",           // RET_GLOBALLOCK
    "GLOBALUNLOCK",         // RET_GLOBALUNLOCK
    "GLOBALUNLOCKFREE",     // RET_GLOBALUNLOCKFREE
    "FINDRESOURCE",         // RET_FINDRESOURCE
    "LOADRESOURCE",         // RET_LOADRESOURCE
    "FREERESOURCE",         // RET_FREERESOURCE
    "LOCKRESOURCE",         // RET_LOCKRESOURCE
    "UNLOCKRESOURCE",       // RET_UNLOCKRESOURCE
    "SIZEOFRESOURCE",       // RET_SIZEOFRESOURCE
    "LOCKSEGMENT",          // RET_LOCKSEGMENT
    "UNLOCKSEGMENT",        // RET_UNLOCKSEGMENT
    "ENUMMETAFILEPROC",     // RET_ENUMMETAFILEPROC
    "TASKSTARTED    ",      // RET_TASKSTARTED
    "HOOKPROC",             // RET_HOOKPROC
    "SUBCLASSPROC",         // RET_SUBCLASSPROC
    "LINEDDAPROC",          // RET_LINEDDAPROC
    "GRAYSTRINGPROC",       // RET_GRAYSTRINGPROC
    "FORCETASKEXIT",        // RET_FORCETASKEXIT
    "SETCURDIR",            // RET_SETCURDIR
    "ENUMOBJPROC",          // RET_ENUMOBJPROC
    "SETCURSORICONFLAG",    // RET_SETCURSORICONFLAG
    "SETABORTPROC",         // RET_SETABORTPROC
    "ENUMPROPSPROC",        // RET_ENUMPROPSPROC
    "FORCESEGMENTFAULT",    // RET_FORCESEGMENTFAULT
    "LSTRCMP",              // RET_LSTRCMP
    "UNUSEDFUNC",           // 
    "UNUSEDFUNC",           // 
    "UNUSEDFUNC",           // 
    "UNUSEDFUNC",           // 
    "GETEXEPTR",            // RET_GETEXEPTR
    "UNUSEDFUNC",           // 
    "FORCETASKFAULT",       // RET_FORCETASKFAULT
    "GETEXPWINVER",         // RET_GETEXPWINVER
    "GETCURDIR",            // RET_GETCURDIR
    "GETDOSPDB",            // RET_GETDOSPDB
    "GETDOSSFT",            // RET_GETDOSSFT
    "FOREGROUNDIDLE",       // RET_FOREGROUNDIDLE
    "WINSOCKBLOCKHOOK",     // RET_WINSOCKBLOCKHOOK
    "WOWDDEFREEHANDLE",     // RET_WOWDDEFREEHANDLE
    "CHANGESELECTOR",       // RET_CHANGESELECTOR
    "GETMODULEFILENAME",    // RET_GETMODULEFILENAME
    "WORDBREAKPROC",        // RET_WORDBREAKPROC
    "WINEXEC",              // RET_WINEXEC
    "WOWCALLBACK16",        // RET_WOWCALLBACK16
    "GETDIBSIZE",           // RET_GETDIBSIZE
    "GETDIBFLAGS",          // RET_GETDIBFLAGS
    "SETDIBSEL",            // RET_SETDIBSEL
    "FREEDIBSEL",           // RET_FREEDIBSEL
    };
#endif
    register PTD ptd;
    register PVDMFRAME pFrame;
    register PCBVDMFRAME pCBFrame;
    WORD wAX;
    BOOL fComDlgSync = FALSE;
    INT  cStackAlloc16;
    VPVOID   vpCBStack;  // See NOTES in walloc16.c\stackalloc16()

#if FASTBOPPING
#else
    USHORT SaveIp;
#endif
#ifdef DEBUG
    VPVOID   vpStackT;
#endif

    WOW32ASSERT(iRetID != RET_RETURN && iRetID != RET_DEBUGRETURN);

    ptd = CURRENTPTD();

    // ssync 16-bit & 32-bit common dialog structs (see wcommdlg.c)
    if(ptd->CommDlgTd) {

        // only ssync for stuff that might actually callback into the app
        // ie. we don't need to ssync every time wow32 calls GlobalLock16
        switch(iRetID) {
            case RET_WNDPROC:           // try to get these in a most frequently
            case RET_HOOKPROC:          // used order
            case RET_WINSOCKBLOCKHOOK:
            case RET_ENUMFONTPROC:
            case RET_ENUMWINDOWPROC:
            case RET_ENUMOBJPROC:
            case RET_ENUMPROPSPROC:
            case RET_LINEDDAPROC:
            case RET_GRAYSTRINGPROC:
            case RET_SETWORDBREAKPROC:
            case RET_SETABORTPROC:
                // Note: This call can invalidate flat ptrs to 16-bit mem
                Ssync_WOW_CommDlg_Structs(ptd->CommDlgTd, w32to16, 0);
                fComDlgSync = TRUE;   // set this for return ssync
                break;
            default:
                break;
        }
    }

    GETFRAMEPTR(ptd->vpStack, pFrame);

    // Just making sure that this thread matches the current 16-bit task

    WOW32ASSERT((pFrame->wTDB == ptd->htask16) ||
                (ptd->dwFlags & TDF_IGNOREINPUT) ||
                (ptd->htask16 == 0));


    // set up the callback stack frame from the correct location
    // & make it word aligned.
    // if stackalloc16() hasn't been called since the app called into wow32
    if (ptd->cStackAlloc16 == 0) {
        vpCBStack = ptd->vpStack;
        ptd->vpCBStack = (ptd->vpStack - sizeof(CBVDMFRAME)) & (~0x1);
    }
    else {
        vpCBStack = ptd->vpCBStack;
        ptd->vpCBStack = (ptd->vpCBStack - sizeof(CBVDMFRAME)) & (~0x1);
    }

    GETFRAMEPTR(ptd->vpCBStack, (PVDMFRAME)pCBFrame);
    pCBFrame->vpStack    = ptd->vpStack;
    pCBFrame->wRetID     = (WORD)iRetID;
    pCBFrame->wTDB       = pFrame->wTDB;
    pCBFrame->wLocalBP   = pFrame->wLocalBP;

    // save the current context stackalloc16() count and set the count to
    // 0 for the next context.  This will force ptd->vpCBStack to be calc'd
    // correctly in any future calls to stackalloc16() if the app callsback
    // into WOW
    cStackAlloc16      = ptd->cStackAlloc16;
    ptd->cStackAlloc16 = 0;

#ifdef DEBUG
    // Save

    vpStackT = ptd->vpStack;
#endif

    if (pParm16)
        RtlCopyMemory(&pCBFrame->Parm16, pParm16, sizeof(PARM16));

    //if (vpfnProc)     // cheaper to just do it
        STOREDWORD(pCBFrame->vpfnProc, vpfnProc);

    wAX = HIWORD(ptd->vpStack);         // Put SS in AX register for callback

    if ( iRetID == RET_WNDPROC ) {
        if ( pParm16->WndProc.hInst )
            wAX = pParm16->WndProc.hInst | 1;
    }

    pCBFrame->wAX = wAX;                // Use this AX for the callback

    //
    // Semi-slimy way we pass byte count of arguments into this function
    // for generic callbacks (WOWCallback16).
    //

    if (RET_WOWCALLBACK16 == iRetID) {
        pCBFrame->wGenUse1 = (WORD)(DWORD)*pvpReturn;
    }

#ifdef DEBUG
    if (iRetID == RET_WNDPROC) {
        LOGDEBUG(9,("%04X          Calling WIN16 WNDPROC(%08lx:%04x,%04x,%04x,%04x,%04x)\n",
            pFrame->wTDB,
            vpfnProc,
            pParm16->WndProc.hwnd,
            pParm16->WndProc.wMsg,
            pParm16->WndProc.wParam,
            HIWORD(pParm16->WndProc.lParam),
            LOWORD(pParm16->WndProc.lParam)
           )
        );

    } else if (iRetID == RET_HOOKPROC) {
        LOGDEBUG(9,("%04X         Calling WIN16 HOOKPROC(%08lx: %04x,%04x,%04x,%04x)\n",
            pFrame->wTDB,
            vpfnProc,
            pParm16->HookProc.nCode,
            pParm16->HookProc.wParam,
            HIWORD(pParm16->HookProc.lParam),
            LOWORD(pParm16->HookProc.lParam)
            )
        );


    } else {
        LOGDEBUG(9,("%04X         Calling WIN16 %s(%04x,%04x,%04x)\n",
            pFrame->wTDB,
            apszCallBacks[iRetID],
            pParm16->WndProc.wParam,
            HIWORD(pParm16->WndProc.lParam),
            LOWORD(pParm16->WndProc.lParam)
           )
        );
    }
#endif

    FREEVDMPTR(pFrame);
    FLUSHVDMPTR(ptd->vpCBStack, sizeof(CBVDMFRAME), pCBFrame);
    FREEVDMPTR(pCBFrame);

    // Set up to use the right 16-bit stack for this thread

#if FASTBOPPING
    SETFASTVDMSTACK(ptd->vpCBStack);
#else
    SETVDMSTACK(ptd->vpCBStack);
#endif

    //
    // do the callback!
    //

#if FASTBOPPING
    CurrentMonitorTeb = NtCurrentTeb();
    FastWOWCallbackCall();
    // fastbop code refreshes ptd->vpStack
#else
    // Time to get the IEU running task-time code again
    SaveIp = getIP();
    host_simulate();
    setIP(SaveIp);
    ptd->vpStack = VDMSTACK();
#endif

    // after return from callback ptd->vpStack will point to PCBVDMFRAME
    ptd->vpCBStack = ptd->vpStack;

    // reset the stackalloc16() count back to this context
    ptd->cStackAlloc16 = cStackAlloc16;

    GETFRAMEPTR(ptd->vpCBStack, (PVDMFRAME)pCBFrame);

    // Just making sure that this thread matches the current 16-bit task

    WOW32ASSERT((pCBFrame->wTDB == ptd->htask16) ||
        (ptd->htask16 == 0));

    if (pvpReturn) {
        LOW(*pvpReturn) = pCBFrame->wAX;
        HIW(*pvpReturn) = pCBFrame->wDX;
    } 

    switch(iRetID) {

        case RET_GLOBALLOCK: 
        case RET_LOCKRESOURCE:
            if(pParm16) {
                pParm16->WndProc.lParam = 
                            pCBFrame->wGenUse2 | (LONG)pCBFrame->wGenUse1 << 16;
            }
            break;

        case RET_GLOBALALLOCLOCK:
            if(pParm16) {
                    pParm16->WndProc.wParam = pCBFrame->wGenUse1;
                }
            break;

        case RET_FINDRESOURCE:
            if(pParm16) {
                pParm16->WndProc.lParam = (ULONG)pCBFrame->wGenUse1;
            }
            break;

    } // end switch

    LOGDEBUG(9,("%04X          WIN16 %s returning: %lx\n",
        pCBFrame->wTDB, apszCallBacks[iRetID], (pvpReturn) ? *pvpReturn : 0));

    // restore the stack to its original value.
    // ie. fake the 'pop' of callback stack by resetting the vpStack
    // to its original value. The ss:sp will actually be updated when
    // the 'api thunk' returns.

    // consistency check
    WOW32ASSERT(pCBFrame->vpStack == vpStackT);

    // restore the stack & callback frame ptrs to original values
    ptd->vpStack = pCBFrame->vpStack;
    ptd->vpCBStack = vpCBStack;

    // ssync 16-bit & 32-bit common dialog structs (see wcommdlg.c)
    if(fComDlgSync) {
        // Note: This call can invalidate flat ptrs to 16-bit mem
        Ssync_WOW_CommDlg_Structs(ptd->CommDlgTd, w16to32, 0);
    }

    FREEVDMPTR(pCBFrame);

    return TRUE;
}
