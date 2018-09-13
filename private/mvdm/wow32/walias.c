/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WALIAS.C
 *  WOW32 16-bit handle alias support
 *
 *  History:
 *  Created 27-Jan-1991 by Jeff Parsons (jeffpar)
 *  Modified 12-May-1992 by Mike Tricker (miketri) to add MultiMedia support
--*/

#include "precomp.h"
#pragma hdrstop

MODNAME(walias.c);

extern CRITICAL_SECTION gcsWOW;
extern PTD gptdTaskHead;

//BUGBUG - this must be removed once MM_MCISYSTEM_STRING is defined in MMSYSTEM.H.
#ifndef MM_MCISYSTEM_STRING
    #define MM_MCISYSTEM_STRING 0x3CA
#endif

#ifdef  DEBUG
extern  BOOL fSkipLog;          // TRUE to temporarily skip certain logging
#endif

typedef struct _stdclass {
    LPSTR   lpszClassName;
    ATOM    aClassAtom;
    WNDPROC lpfnWndProc;
    INT     iOrdinal;
    DWORD   vpfnWndProc;
} STDCLASS;

// Some cool defines stolen from USERSRV.H
#define MENUCLASS       MAKEINTATOM(0x8000)
#define DESKTOPCLASS    MAKEINTATOM(0x8001)
#define DIALOGCLASS     MAKEINTATOM(0x8002)
#define SWITCHWNDCLASS  MAKEINTATOM(0x8003)
#define ICONTITLECLASS  MAKEINTATOM(0x8004)

// See WARNING below!
STDCLASS stdClasses[] = {
    NULL,           0,                      NULL,   0,                      0,  // WOWCLASS_UNKNOWN
    NULL,           0,                      NULL,   0,                      0,  // WOWCLASS_WIN16
    "BUTTON",       0,                      NULL,   FUN_BUTTONWNDPROC,      0,  // WOWCLASS_BUTTON,
    "COMBOBOX",     0,                      NULL,   FUN_COMBOBOXCTLWNDPROC, 0,  // WOWCLASS_COMBOBOX,
    "EDIT",         0,                      NULL,   FUN_EDITWNDPROC,        0,  // WOWCLASS_EDIT,
    "LISTBOX",      0,                      NULL,   FUN_LBOXCTLWNDPROC,     0,  // WOWCLASS_LISTBOX,
    "MDICLIENT",    0,                      NULL,   FUN_MDICLIENTWNDPROC,   0,  // WOWCLASS_MDICLIENT,
    "SCROLLBAR",    0,                      NULL,   FUN_SBWNDPROC,          0,  // WOWCLASS_SCROLLBAR,
    "STATIC",       0,                      NULL,   FUN_STATICWNDPROC,      0,  // WOWCLASS_STATIC,
    "#32769",       (WORD)DESKTOPCLASS,     NULL,   FUN_DESKTOPWNDPROC,     0,  // WOWCLASS_DESKTOP,
    "#32770",       (WORD)DIALOGCLASS,      NULL,   FUN_DEFDLGPROCTHUNK,    0,  // WOWCLASS_DIALOG,
    "#32772",       (WORD)ICONTITLECLASS,   NULL,   FUN_TITLEWNDPROC,       0,  // WOWCLASS_ICONTITLE,
    "#32768",       (WORD)MENUCLASS,        NULL,   FUN_MENUWNDPROC,        0,  // WOWCLASS_MENU,
    "#32771",       (WORD)SWITCHWNDCLASS,   NULL,   0,                      0,  // WOWCLASS_SWITCHWND,
    "COMBOLBOX",    0,                      NULL,   FUN_LBOXCTLWNDPROC,     0,  // WOWCLASS_COMBOLBOX
};
//
// WARNING! The above sequence and values must be maintained otherwise the
// table in WMSG16.C for message thunking must be changed.  Same goes for
// the #define's in WALIAS.H
//
// The above COMBOLBOX case is special because it is class that is
// almost identical to a listbox.  Therefore we lie about it.

INT GetStdClassNumber(
    PSZ pszClass
) {
    INT     i;

    if ( HIWORD(pszClass) ) {

        // They passed us a string

        for ( i = WOWCLASS_BUTTON; i < NUMEL(stdClasses); i++ ) {
            if ( WOW32_stricmp(pszClass, stdClasses[i].lpszClassName) == 0 ) {
                return( i );
            }
        }
    } else {

        // They passed us an atom

        for ( i = WOWCLASS_BUTTON; i < NUMEL(stdClasses); i++ ) {
            if ( stdClasses[i].aClassAtom == 0 ) {
                // RegisterWindowMessage is an undocumented way of determining
                // an atom value in the context of the server-side heap.
                stdClasses[i].aClassAtom = (ATOM)RegisterWindowMessage(stdClasses[i].lpszClassName);
            }
            if ( (ATOM)LOWORD(pszClass) == stdClasses[i].aClassAtom ) {
                return( i );
            }
        }
    }
    return( WOWCLASS_WIN16 );  // private 16-bit class created by the app
}

// Returns a 32 window proc given a class index

WNDPROC GetStdClassWndProc(
    DWORD   iClass
) {
    WNDPROC lpfn32;

    if ( iClass < WOWCLASS_WIN16 || iClass > WOWCLASS_MAX ) {
        WOW32ASSERT(FALSE);
        return( NULL );
    }

    lpfn32 = stdClasses[iClass].lpfnWndProc;

    if ( lpfn32 == NULL ) {
        WNDCLASS    wc;
        BOOL        f;

        f = GetClassInfo( NULL, stdClasses[iClass].lpszClassName, &wc );

        if ( f ) {
            VPVOID  vp;
	    DWORD UNALIGNED * lpdw;

            lpfn32 = wc.lpfnWndProc;
            stdClasses[iClass].lpfnWndProc = lpfn32;

            vp = GetStdClassThunkProc(iClass);
            vp = (VPVOID)((DWORD)vp - sizeof(DWORD)*3);

            GETVDMPTR( vp, sizeof(DWORD)*3, lpdw );

            WOW32ASSERT(*lpdw == SUBCLASS_MAGIC);   // Are we editing the right stuff?

            if (!lpdw)
                *(lpdw+2) = (DWORD)lpfn32;

            FLUSHVDMCODEPTR( vp, sizeof(DWORD)*3, lpdw );
            FREEVDMPTR( lpdw );

        }
    }
    return( lpfn32 );
}

// Returns a 16 window proc thunk given a class index

DWORD GetStdClassThunkProc(
    INT     iClass
) {
    DWORD   dwResult;
    SHORT   iOrdinal;
    PARM16  Parm16;

    if ( iClass < WOWCLASS_WIN16 || iClass > WOWCLASS_MAX ) {
        WOW32ASSERT(FALSE);
        return( 0 );
    }

    iOrdinal = (SHORT)stdClasses[iClass].iOrdinal;

    if ( iOrdinal == 0 ) {
        return( (DWORD)NULL );
    }

    // If we've already gotten this proc, then don't bother calling into 16-bit
    dwResult = stdClasses[iClass].vpfnWndProc;

    if ( dwResult == (DWORD)NULL ) {

        // Callback into the 16-bit world asking for the 16:16 address

        Parm16.SubClassProc.iOrdinal = iOrdinal;

        if (!CallBack16(RET_SUBCLASSPROC, &Parm16, (VPPROC)NULL,
                          (PVPVOID)&dwResult)) {
            WOW32ASSERT(FALSE);
            return( 0 );
        }
        // Save it since it is a constant.
        stdClasses[iClass].vpfnWndProc = dwResult;
    }
    return( dwResult );
}

/*
 * PWC GetClassWOWWords(hInst, pszClass)
 *   is a ***private*** API for WOW only. It returns a pointer to the
 *   WOW Class structure in the server's window class structure.
 *   This is similar to GetClassLong(hwnd32, GCL_WOWWORDS) (see FindPWC),
 *   but in this case we don't have a hwnd32, we have the class name
 *   and instance handle.
 */

PWC FindClass16(LPCSTR pszClass, HAND16 hInst)
{
    register PWC pwc;

    pwc = (PWC)(pfnOut.pfnGetClassWOWWords)(HMODINST32(hInst), pszClass);
    WOW32WARNMSGF(
        pwc,
        ("WOW32 warning: GetClassWOWWords('%s', %04x) returned NULL\n", pszClass, hInst)
        );

    return (pwc);
}



#ifdef DEBUG

INT nAliases;
INT iLargestListSlot;

PSZ apszHandleClasses[] = {
    "Unknown",      // WOWCLASS_UNKNOWN
    "Window",       // WOWCLASS_WIN16
    "Button",       // WOWCLASS_BUTTON
    "ComboBox",     // WOWCLASS_COMBOBOX
    "Edit",         // WOWCLASS_EDIT
    "ListBox",      // WOWCLASS_LISTBOX
    "MDIClient",    // WOWCLASS_MDICLIENT
    "Scrollbar",    // WOWCLASS_SCROLLBAR
    "Static",       // WOWCLASS_STATIC
    "Desktop",      // WOWCLASS_DESKTOP
    "Dialog",       // WOWCLASS_DIALOG
    "Menu",         // WOWCLASS_MENU
    "IconTitle",    // WOWCLASS_ICONTITLE
    "Accel",        // WOWCLASS_ACCEL
    "Cursor",       // WOWCLASS_CURSOR
    "Icon",         // WOWCLASS_ICON
    "DC",           // WOWCLASS_DC
    "Font",         // WOWCLASS_FONT
    "MetaFile",     // WOWCLASS_METAFILE
    "Region",       // WOWCLASS_RGN
    "Bitmap",       // WOWCLASS_BITMAP
    "Brush",        // WOWCLASS_BRUSH
    "Palette",      // WOWCLASS_PALETTE
    "Pen",          // WOWCLASS_PEN
    "Object"        // WOWCLASS_OBJECT
};


BOOL MessageNeedsThunking(UINT uMsg)
{
    switch (uMsg) {
        case WM_CREATE:
        case WM_ACTIVATE:
        case WM_SETFOCUS:
        case WM_KILLFOCUS:
        case WM_SETTEXT:
        case WM_GETTEXT:
        case WM_ERASEBKGND:
        case WM_WININICHANGE:
        case WM_DEVMODECHANGE:
        case WM_ACTIVATEAPP:
        case WM_SETCURSOR:
        case WM_MOUSEACTIVATE:
        case WM_GETMINMAXINFO:
        case WM_ICONERASEBKGND:
        case WM_NEXTDLGCTL:
        case WM_DRAWITEM:
        case WM_MEASUREITEM:
        case WM_DELETEITEM:
        case WM_VKEYTOITEM:
        case WM_CHARTOITEM:
        case WM_SETFONT:
        case WM_GETFONT:
        case WM_QUERYDRAGICON:
        case WM_COMPAREITEM:
        case WM_OTHERWINDOWCREATED:
        case WM_OTHERWINDOWDESTROYED:
        case WM_COMMNOTIFY:
        case WM_WINDOWPOSCHANGING:
        case WM_WINDOWPOSCHANGED:
        case WM_NCCREATE:
        case WM_NCCALCSIZE:
        case WM_COMMAND:
        case WM_HSCROLL:
        case WM_VSCROLL:
        case WM_INITMENU:
        case WM_INITMENUPOPUP:
        case WM_MENUSELECT:
        case WM_MENUCHAR:
        case WM_ENTERIDLE:
        case WM_CTLCOLORMSGBOX:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSCROLLBAR:
        case WM_CTLCOLORSTATIC:
        case WM_PARENTNOTIFY:
        case WM_MDICREATE:
        case WM_MDIDESTROY:
        case WM_MDIACTIVATE:
        case WM_MDIGETACTIVE:
        case WM_MDISETMENU:
        case WM_RENDERFORMAT:
        case WM_PAINTCLIPBOARD:
        case WM_VSCROLLCLIPBOARD:
        case WM_SIZECLIPBOARD:
        case WM_ASKCBFORMATNAME:
        case WM_CHANGECBCHAIN:
        case WM_HSCROLLCLIPBOARD:
        case WM_PALETTEISCHANGING:
        case WM_PALETTECHANGED:
        case MM_JOY1MOVE:
        case MM_JOY2MOVE:
        case MM_JOY1ZMOVE:
        case MM_JOY2ZMOVE:
        case MM_JOY1BUTTONDOWN:
        case MM_JOY2BUTTONDOWN:
        case MM_JOY1BUTTONUP:
        case MM_JOY2BUTTONUP:
        case MM_MCINOTIFY:
        case MM_MCISYSTEM_STRING:
        case MM_WOM_OPEN:
        case MM_WOM_CLOSE:
        case MM_WOM_DONE:
        case MM_WIM_OPEN:
        case MM_WIM_CLOSE:
        case MM_WIM_DATA:
        case MM_MIM_OPEN:
        case MM_MIM_CLOSE:
        case MM_MIM_DATA:
        case MM_MIM_LONGDATA:
        case MM_MIM_ERROR:
        case MM_MIM_LONGERROR:
        case MM_MOM_OPEN:
        case MM_MOM_CLOSE:
        case MM_MOM_DONE:
            LOGDEBUG(LOG_IMPORTANT,
                ("MessageNeedsThunking: WM_msg %04x is not thunked\n", uMsg));
            return TRUE;

        default:
            return FALSE;

    }
}

#endif


PTD ThreadProcID32toPTD(DWORD dwThreadID, DWORD dwProcessID)
{
    PTD ptd, ptdThis;
    PWOAINST pWOA;

    //
    // If we have active child instances of WinOldAp,
    // try to map the process ID of a child Win32 app
    // to the corresponding WinOldAp PTD.
    //

    ptdThis = CURRENTPTD();

    EnterCriticalSection(&ptdThis->csTD);

    pWOA = ptdThis->pWOAList;

    while (pWOA && pWOA->dwChildProcessID != dwProcessID) {
        pWOA = pWOA->pNext;
    }

    if (pWOA) {

        ptd = pWOA->ptdWOA;

        LeaveCriticalSection(&ptdThis->csTD);

    } else {

        LeaveCriticalSection(&ptdThis->csTD);

        //
        // We didn't find a WinOldAp PTD to return, see
        // if the thread ID matches one of our app threads.
        //

        EnterCriticalSection(&gcsWOW);

        ptd = gptdTaskHead;

        while (ptd && ptd->dwThreadID != dwThreadID) {
            ptd = ptd->ptdNext;
        }

        LeaveCriticalSection(&gcsWOW);
    }

    return ptd;

}

PTD Htask16toPTD(
    HTASK16 htask16
) {
    PTD  ptd;

    EnterCriticalSection(&gcsWOW);

    ptd = gptdTaskHead;

    while(ptd) {

        if ( ptd->htask16 == htask16 ) {
            break;
        }
        ptd = ptd->ptdNext;
    }

    LeaveCriticalSection(&gcsWOW);

    return ptd;
}


HTASK16 ThreadID32toHtask16(
    DWORD   ThreadID32
) {
    PTD ptd;
    HTASK16 htask16;


    if ( ThreadID32 == 0 ) {
        WOW32ASSERTMSG(ThreadID32, "WOW::ThreadID32tohTask16: Thread ID is 0\n");
        htask16 = 0;
    } else {

        ptd = ThreadProcID32toPTD( ThreadID32, (DWORD)-1 );
        if ( ptd ) {
            // Good, its one of our wow threads.
            htask16 = ptd->htask16;
        } else {
            // Nope, its is some other 32-bit thread
            htask16 = FindHtaskAlias( ThreadID32 );
            if ( htask16 == 0 ) {
                //
                // See the comment in WOLE2.C for a nice description
                //
                htask16 = AddHtaskAlias( ThreadID32 );
            }
        }
    }

    return htask16;
}

DWORD Htask16toThreadID32(
    HTASK16 htask16
) {
    if ( htask16 == 0 ) {
        return( 0 );
    }

    if ( ISTASKALIAS(htask16) ) {
        return( GetHtaskAlias(htask16,NULL) );
    } else {
        return( THREADID32(htask16) );
    }
}

//***************************************************************************
// GetGCL_HMODULE - returns the valid hmodule if the window corresponds to
//                  a 16bit class else returns the hmodule of 16bit user.exe
//                  if the window is of a standard class.
//
// These cases are required for compatibility sake.
//         apps like VirtualMonitor, hDC etc depend on such behaviour.
//                                                              - Nanduri
//***************************************************************************
WORD gUser16hInstance = 0;

ULONG GetGCL_HMODULE(HWND hwnd)
{
    ULONG    ul;
    PTD      ptd;
    PWOAINST pWOA;
    DWORD    dwProcessID;

    ul = (ULONG)GetClassLong(hwnd, GCL_HMODULE);

    //
    // hMod32 = 0xZZZZ0000
    //

    if (ul != 0 && LOWORD(ul) == 0) {

        //
        // If we have active WinOldAp children, see if this window
        // belongs to a Win32 process spawned by one of the
        // active winoldap's.  If it is, return the hmodule
        // of the corresponding winoldap.  Otherwise we
        // return user.exe's hinstance (why not hmodule?)
        //

        dwProcessID = (DWORD)-1;
        GetWindowThreadProcessId(hwnd, &dwProcessID);

        ptd = CURRENTPTD();

        EnterCriticalSection(&ptd->csTD);

        pWOA = ptd->pWOAList;
        while (pWOA && pWOA->dwChildProcessID != dwProcessID) {
            pWOA = pWOA->pNext;
        }

        if (pWOA) {
            ul = pWOA->ptdWOA->hMod16;
            LOGDEBUG(LOG_ALWAYS, ("WOW32 GetClassLong(0x%x, GWW_HMODULE) returning 0x%04x\n",
                                  hwnd, ul));
        } else {
            ul = (ULONG) gUser16hInstance;
            WOW32ASSERT(ul);
        }

        LeaveCriticalSection(&ptd->csTD);
    }
    else {
        ul = (ULONG)GETHMOD16(ul);      // 32-bit hmod is HMODINST32
    }

    return ul;
}

//
// EXPORTED handle mapping functions.  WOW32 code should use the
// macros defined in walias.h -- these functions are for use by
// third-party 32-bit code running in WOW, for example called
// using generic thunks from WOW-specific 16-bit code.
//

HANDLE WOWHandle32 (WORD h16, WOW_HANDLE_TYPE htype)
{
    switch (htype) {
        case WOW_TYPE_HWND:
            return HWND32(h16);
        case WOW_TYPE_HMENU:
            return HMENU32(h16);
        case WOW_TYPE_HDWP:
            return HDWP32(h16);
        case WOW_TYPE_HDROP:
            return HDROP32(h16);
        case WOW_TYPE_HDC:
            return HDC32(h16);
        case WOW_TYPE_HFONT:
            return HFONT32(h16);
        case WOW_TYPE_HMETAFILE:
            return HMETA32(h16);
        case WOW_TYPE_HRGN:
            return HRGN32(h16);
        case WOW_TYPE_HBITMAP:
            return HBITMAP32(h16);
        case WOW_TYPE_HBRUSH:
            return HBRUSH32(h16);
        case WOW_TYPE_HPALETTE:
            return HPALETTE32(h16);
        case WOW_TYPE_HPEN:
            return HPEN32(h16);
        case WOW_TYPE_HACCEL:
            return HACCEL32(h16);
        case WOW_TYPE_HTASK:
            return (HANDLE)HTASK32(h16);
        case WOW_TYPE_FULLHWND:
            return (HANDLE)FULLHWND32(h16);
        default:
            return(INVALID_HANDLE_VALUE);
    }
}

WORD WOWHandle16 (HANDLE h32, WOW_HANDLE_TYPE htype)
{
    switch (htype) {
        case WOW_TYPE_HWND:
            return GETHWND16(h32);
        case WOW_TYPE_HMENU:
            return GETHMENU16(h32);
        case WOW_TYPE_HDWP:
            return GETHDWP16(h32);
        case WOW_TYPE_HDROP:
            return GETHDROP16(h32);
        case WOW_TYPE_HDC:
            return GETHDC16(h32);
        case WOW_TYPE_HFONT:
            return GETHFONT16(h32);
        case WOW_TYPE_HMETAFILE:
            return GETHMETA16(h32);
        case WOW_TYPE_HRGN:
            return GETHRGN16(h32);
        case WOW_TYPE_HBITMAP:
            return GETHBITMAP16(h32);
        case WOW_TYPE_HBRUSH:
            return GETHBRUSH16(h32);
        case WOW_TYPE_HPALETTE:
            return GETHPALETTE16(h32);
        case WOW_TYPE_HPEN:
            return GETHPEN16(h32);
        case WOW_TYPE_HACCEL:
            return GETHACCEL16(h32);
        case WOW_TYPE_HTASK:
            return GETHTASK16(h32);
        default:
            return(0xffff);
    }
}

PVOID gpGdiHandleInfo = (PVOID)-1;

//WARNING: This structure must match ENTRY in ntgdi\inc\hmgshare.h

typedef struct _ENTRYWOW
{
    LONG   l1;
    LONG   l2;
    USHORT FullUnique;
    USHORT us1;
    LONG   l3;
} ENTRYWOW, *PENTRYWOW;

//
// this routine converts a 16bit GDI handle to a 32bit handle.  There
// is no need to do any validation on the handle since the 14bit space
// for handles ignoring the low two bits is completely contained in the
// valid 32bit handle space.
//

HANDLE hConvert16to32(int h16)
{
    ULONG h32;
    int i = h16 >> 2;

    h32 = i | (ULONG)(((PENTRYWOW)gpGdiHandleInfo)[i].FullUnique) << 16;

    return((HANDLE)h32);
}

// We probably don't need to worry about this buffer being too small since we're
// really only interested in the predefined standard classes which tend to
// be rather short-named.
#define MAX_CLASSNAME_LEN  64

// There is a time frame (from when an app calls CreateWindow until USER32 gets
// a message at one of its WndProc's for the window - see FritzS) during which
// the fnid (class type) can't be set officially for the window.  If the
// GETICLASS macro is invoked during this period, it will be unable to find the
// iClass for windows created on any of the standard control classes using the
// fast fnid index method (see walias.h).  Once the time frame is passed, the
// fast fnid method will work fine for these windows.
//
// This is manifested in apps that set CBT hooks and try to subclass the
// standard classes while in their hook proc.  See bug #143811.
INT GetiClassTheHardWay(HWND hwnd)
{

    INT  iClass = WOWCLASS_WIN16;       // default return: app private class
    char ClassName[MAX_CLASSNAME_LEN];


    if(GetClassName(hwnd, ClassName, MAX_CLASSNAME_LEN)) {
        iClass = GetStdClassNumber(ClassName);
    }

    return(iClass);
}




INT GetIClass(PWW pww, HWND hwnd)
{
    INT   iClass;

    // if it is a standard class
    if(((pww->fnid & 0xfff) >= FNID_START) &&
                 ((pww->fnid & 0xfff) <= FNID_END)) {

        // return the class id for this initialized window
        iClass = pfnOut.aiWowClass[( pww->fnid & 0xfff) - FNID_START];

    }

    else {

        // if the window is initialized
        if(pww->state2 & WINDOW_IS_INITIALIZED) {

            // we know this is a private 16-bit class registered by the app
            iClass = WOWCLASS_WIN16;
        }

        // else find the class the hard way
        else {
            iClass = GetiClassTheHardWay(hwnd);
        }
    }

    return(iClass);
}

