//*****************************************************************************
//
// SUBCLASSING  -
//
//     Support for subclassing of 32bit standard (predefined) classes by
//     WOW apps.
//
//
// 01-10-92  NanduriR   Created.
//
//*****************************************************************************

#include "precomp.h"
#pragma hdrstop

MODNAME(wsubcls.c);

VPVOID vptwpFirst = (VPVOID)NULL;

BOOL ConstructThunkWindowProc(
    LPTWPLIST   lptwp,
    VPVOID      vptwp
) {
/*
** This is the code which would build a thunk window proc but since we already
** have code like this laying around (button window proc thunk) we can just
** copy it.  This also saves us from having to know things like the DGROUP
** of USER16, or the address of the 16-bit CallWindowProc function.
**
** lptwp->Code[0x00] = 0x45;                              inc     bp
** lptwp->Code[0x01] = 0x55;                              push    bp
** lptwp->Code[0x02] = 0x8B;                              mov     bp,sp
** lptwp->Code[0x03] = 0xEC;
** lptwp->Code[0x04] = 0x1E;                              push    ds
** lptwp->Code[0x05] = 0xB8;                              mov     ax,DGROUP
** lptwp->Code[0x06] = LOBYTE(USER_DGROUP);
** lptwp->Code[0x07] = HIBYTE(USER_DGROUP);
** lptwp->Code[0x08] = 0x8E;                              mov     ds,ax
** lptwp->Code[0x09] = 0xD8;
** lptwp->Code[0x0A] = 0xB8;                              mov     ax,OFFSET BUTTONWNDPROC
** lptwp->Code[0x0B] = LOBYTE(LOWORD(ThunkProc16));
** lptwp->Code[0x0C] = HIBYTE(LOWORD(ThunkProc16));
** lptwp->Code[0x0D] = 0xBA;                              mov     dx,SEG BUTTONWNDPROC
** lptwp->Code[0x0E] = LOBYTE(HIWORD(ThunkProc16));
** lptwp->Code[0x0F] = HIBYTE(HIWORD(ThunkProc16));
** lptwp->Code[0x10] = 0x52;                              push    dx
** lptwp->Code[0x11] = 0x50;                              push    ax
** lptwp->Code[0x12] = 0xFF;                              push    WORD PTR [bp+14]        ;hwnd
** lptwp->Code[0x13] = 0x76;
** lptwp->Code[0x14] = 0x0E;
** lptwp->Code[0x15] = 0xFF;                              push    WORD PTR [bp+12]        ;messag
** lptwp->Code[0x16] = 0x76;
** lptwp->Code[0x17] = 0x0C;
** lptwp->Code[0x18] = 0xFF;                              push    WORD PTR [bp+10]        ;wParam
** lptwp->Code[0x19] = 0x76;
** lptwp->Code[0x1A] = 0x0A;
** lptwp->Code[0x1B] = 0xFF;                              push    WORD PTR [bp+8]
** lptwp->Code[0x1C] = 0x76;
** lptwp->Code[0x1D] = 0x08;
** lptwp->Code[0x1E] = 0xFF;                              push    WORD PTR [bp+6]         ;lParam
** lptwp->Code[0x1F] = 0x76;
** lptwp->Code[0x20] = 0x06;
** lptwp->Code[0x21] = 0x9A;                              call    FAR PTR CALLWINDOWPROC
** lptwp->Code[0x22] = LOBYTE(LOWORD(CallWindowProc16));
** lptwp->Code[0x23] = HIBYTE(LOWORD(CallWindowProc16));
** lptwp->Code[0x24] = LOBYTE(HIWORD(CallWindowProc16));
** lptwp->Code[0x25] = HIBYTE(HIWORD(CallWindowProc16));
** lptwp->Code[0x26] = 0x4D;                              dec     bp
** lptwp->Code[0x27] = 0x4D;                              dec     bp
** lptwp->Code[0x28] = 0x8B;                              mov     sp,bp
** lptwp->Code[0x29] = 0xE5;                              dec     bp
** lptwp->Code[0x2A] = 0x1F;                              pop     ds
** lptwp->Code[0x2B] = 0x5D;                              pop     bp
** lptwp->Code[0x2C] = 0x4D;                              dec     bp
** lptwp->Code[0x2D] = 0xCA;                              ret     10
** lptwp->Code[0x2E] = 0x0A;
** lptwp->Code[0x2F] = 0x00;
*/
    VPVOID  vpfn;
    LPVOID  lpfn;
    VPVOID  vpProc16;

    /*
    ** Get the proc address of the Button Window Proc thunk
    */
    vpfn = GetStdClassThunkProc( WOWCLASS_BUTTON );

    if ( vpfn == (VPVOID)NULL ) {
        return( FALSE );
    }

    /*
    ** Now copy it into our thunk
    */
    GETVDMPTR( vpfn, THUNKWP_SIZE, lpfn);

    RtlCopyMemory( lptwp->Code, lpfn, THUNKWP_SIZE );

    FREEVDMPTR( lpfn );

    /*
    ** Patch the "our address" pointer
    */
    vpProc16 = (VPVOID)((DWORD)vptwp + FIELD_OFFSET(TWPLIST,Code[0]));

    lptwp->Code[0x0B] = LOBYTE(LOWORD(vpProc16));
    lptwp->Code[0x0C] = HIBYTE(LOWORD(vpProc16));
    lptwp->Code[0x0E] = LOBYTE(HIWORD(vpProc16));
    lptwp->Code[0x0F] = HIBYTE(HIWORD(vpProc16));

    /*
    ** Initialize the rest of the TWPLIST structure
    */
    lptwp->lpfn32    = 0;
    lptwp->vpfn16    = vpProc16;
    lptwp->vptwpNext = (VPVOID)NULL;
    lptwp->hwnd32    = (HWND)0;
    lptwp->dwMagic   = SUBCLASS_MAGIC;

    return( TRUE );
}


DWORD GetThunkWindowProc(
    DWORD   lpfn32,
    LPSTR   lpszClass,
    PWW     pww,
    HWND    hwnd32
) {
    VPVOID      vptwp;
    LPTWPLIST   lptwp;
    INT         count;
    DWORD       dwResult;
    BOOL        fOk;
    VPVOID      vpAvail = (VPVOID)NULL;
    INT         iClass;

    // Dont try to thunk a NULL 32-bit proc.
    if (!lpfn32) {
        LOGDEBUG(2, ("WOW:GetThunkWindowProc: attempt to thunk NULL proc\n"));
        return 0;
    }

    // the input is either lpstrClass or pww. One of them is always NULL.

    if (lpszClass != NULL) {
        iClass = GetStdClassNumber(lpszClass);
    }
    else {
        iClass = GETICLASS(pww, hwnd32);
    }

    if ( iClass == WOWCLASS_WIN16 ) {
        DWORD dwpid;

        // iClass == WOWCLASS_WIN16 implies that hwnd could either be a 32bit 
        // window belonging to a WOW process (like Ole windows) or a window of
        // a different process. If it is the former return a stub proc
        // else return 0;

        if (!(GetWindowThreadProcessId(hwnd32,&dwpid) &&
              (dwpid == GetCurrentProcessId()))){
            LOGDEBUG(LOG_ALWAYS, ("WOW:GetThunkWindowProc: class unknown\n"));
            return 0;
        }
    } else {
        //
        // If they are subclassing one of the standard classes, and they
        // are doing it for the 1st time, then return the address of the
        // hardcoded thunk in USER16.
        //
        if ( lpfn32 == (DWORD)GetStdClassWndProc(iClass) ) {
            dwResult = GetStdClassThunkProc(iClass);
            return( dwResult );
        }
    }


    /*
    ** Scan the list for available TWPLIST entries or duplicates
    */
    vptwp = vptwpFirst;

    while ( vptwp != (VPVOID)NULL ) {

        GETVDMPTR( vptwp, sizeof(TWPLIST), lptwp );

        if ( lptwp->lpfn32 == 0 && vpAvail == (VPVOID)NULL ) {
            vpAvail = vptwp;
        }
        //
        // If we find that we've already subclassed this proc32
        // then return that thunk proc again.
        //
        if ( lptwp->lpfn32 == lpfn32 ) {
            dwResult = (DWORD)lptwp->vpfn16;
            FREEVDMPTR( lptwp );
            return( dwResult );
        }

        vptwp = lptwp->vptwpNext;

        FREEVDMPTR( lptwp );

    }

    // Obviously we didn't find any duplicate if we got here.

    // If we didn't find a slot to reuse, then allocate more.

    if ( vpAvail == (VPVOID)NULL ) {
        /*
        ** No more available slots, allocate some more
        */
        vptwp = GlobalAllocLock16( GMEM_MOVEABLE,
                                   THUNKWP_BLOCK * sizeof(TWPLIST),
                                   NULL );

        if ( vptwp == (VPVOID)NULL ) {
            LOGDEBUG( 1, ("GetThunkWindowProc: GlobalAllocLock16 failed to allocate memory\n"));
            return( (DWORD)NULL );
        }

        count = THUNKWP_BLOCK;

        while ( count ) {
            GETVDMPTR( vptwp, sizeof(TWPLIST), lptwp );

            fOk = ConstructThunkWindowProc( lptwp, vptwp );

            if ( fOk ) {
                /*
                ** Insert this thunk window proc into the list
                */
                lptwp->vptwpNext = vptwpFirst;
                vptwpFirst = vptwp;
                vpAvail = vptwp;
            }

            FLUSHVDMPTR( vptwp, sizeof(TWPLIST), lptwp );
            FREEVDMPTR( lptwp );

            vptwp = (VPVOID)((DWORD)vptwp + sizeof(TWPLIST));

            --count;
        }

        ChangeSelector16( HIWORD(vptwp) );      // Change into a code selector

    }

    if ( vpAvail != (VPVOID)NULL ) {
        /*
        ** Use that available slot
        */
        GETVDMPTR( vpAvail, sizeof(TWPLIST), lptwp );
        lptwp->lpfn32 = lpfn32;
        lptwp->hwnd32 = hwnd32;
        lptwp->iClass = iClass;

        dwResult = (DWORD)lptwp->vpfn16;
        FLUSHVDMCODEPTR(vpAvail, sizeof(TWPLIST), lptwp);
        FREEVDMPTR( lptwp );

        return( dwResult );
    }

    return( (DWORD)NULL );
}


#if 0  // Currently unused

BOOL FreeThunkWindowProc(
    DWORD vpProc16
) {
    VPVOID      vptwp;
    LPTWPLIST   lptwp;

    /*
    ** Scan the list for available TWPLIST entries
    */
    vptwp = vptwpFirst;

    while ( vptwp != (VPVOID)NULL ) {

        GETVDMPTR( vptwp, sizeof(TWPLIST), lptwp );

        if ( lptwp->vpfn16 == vpProc16 ) {
            /*
            ** Found slot to free
            */
            lptwp->lpfn32 = 0;
            lptwp->hwnd32 = (HWND)0;
            lptwp->iClass = WOWCLASS_UNKNOWN;
            FREEVDMPTR( lptwp );
            return( TRUE );
        }

        vptwp = lptwp->vptwpNext;

        FREEVDMPTR( lptwp );
    }

    return( FALSE );
}

void W32FreeThunkWindowProc(
    DWORD       lpfn32,
    DWORD       lpfn16
) {
    VPVOID      vptwp;
    LPTWPLIST   lptwp;

    /*
    ** Scan the list for available TWPLIST entries
    */
    vptwp = vptwpFirst;

    while ( vptwp != (VPVOID)NULL ) {

        GETVDMPTR( vptwp, sizeof(TWPLIST), lptwp );

        if ( lptwp->lpfn32 == lpfn32 ) {
            /*
            ** Found slot to free
            */
            lptwp->lpfn32 = 0;
            lptwp->hwnd32 = (HWND)0;
            lptwp->iClass = WOWCLASS_UNKNOWN;
            FREEVDMPTR( lptwp );
        }

        vptwp = lptwp->vptwpNext;

        FREEVDMPTR( lptwp );
    }
}
#endif

DWORD IsThunkWindowProc(
    DWORD       vpProc16,       // IN
    PINT        piClass         // OUT OPTIONAL
) {
    VPVOID      vpdw;
    DWORD UNALIGNED *   lpdw;
    DWORD       dwResult;
    INT         iClass;

    /* Screen for valid addresses... */

    if ( (HIWORD(vpProc16) == 0) || (LOWORD(vpProc16) < (sizeof(DWORD)*3)) ) {
        return( 0 );
    }

    /*
    ** If it is a valid sub-class thunk, then it should be preceeded by
    ** three dword values.  The first is the subclassing magic number,
    ** the second is the WOWCLASS_*, the 3rd is the 32-bit proc address.
    */
    vpdw = (VPVOID)((DWORD)vpProc16 - sizeof(DWORD)*3);

    GETVDMPTR( vpdw, sizeof(DWORD)*3, lpdw );

    iClass = (INT)*(lpdw+1);

    dwResult = *(lpdw+2);       // Get the lpfn32 value

    if ( *lpdw != SUBCLASS_MAGIC ) {
        dwResult = 0;           // Zero it if it wasn't valid
        iClass = WOWCLASS_WIN16;
    } else {
        if ( dwResult == 0 ) {
            // They cheated and looked up the export from USER.EXE
            dwResult = (DWORD) GetStdClassWndProc( iClass );
        }
    }

    if (piClass) {
        *piClass = iClass;
    }

    FREEVDMPTR( lpdw );

    return( dwResult );
}
