//**************************************************************************
// wusercli.c :
//     Contains all functions that execute USER32 client code on 16bitside.
//     Most of these functions don't exist on x86 builds. So any changes
//     to these files must be reflected in wow16\user\usercli.asm
//
//                                                          - nanduri
//**************************************************************************

#include "precomp.h"
#pragma hdrstop

MODNAME(wusercli.c);


//**************************************************************************
//  WU32ClientToScreen -
//
//**************************************************************************

ULONG FASTCALL WU32ClientToScreen(PVDMFRAME pFrame)
{
    POINT t2;
    register PCLIENTTOSCREEN16 parg16;

    GETARGPTR(pFrame, sizeof(CLIENTTOSCREEN16), parg16);
    GETPOINT16(parg16->f2, &t2);

    ClientToScreen( HWND32(parg16->f1), &t2 );

    PUTPOINT16(parg16->f2, &t2);
    FREEARGPTR(parg16);
    RETURN(0);
}


//**************************************************************************
//  WU32GetClientRect -
//
//**************************************************************************

ULONG FASTCALL WU32GetClientRect(PVDMFRAME pFrame)
{
    RECT t2;
    register PGETCLIENTRECT16 parg16;

    GETARGPTR(pFrame, sizeof(GETCLIENTRECT16), parg16);

    /*
     * Home Design Gold 2.0
     *
     * If the call fails, don't overwrite the passed-in
     * rect.
     */
    if (GetClientRect(HWND32(parg16->hwnd), &t2)) {
        PUTRECT16(parg16->vpRect, &t2);
    }

    FREEARGPTR(parg16);
    RETURN(0);
}



//**************************************************************************
//  WU32GetCursorPos -
//
//**************************************************************************

ULONG FASTCALL WU32GetCursorPos(PVDMFRAME pFrame)
{
    POINT t1;
    register PGETCURSORPOS16 parg16;

    GETARGPTR(pFrame, sizeof(GETCURSORPOS16), parg16);

    GetCursorPos( &t1 );

    PUTPOINT16(parg16->f1, &t1);
    FREEARGPTR(parg16);
    RETURN(0);
}


//**************************************************************************
//  WU32GetDesktopWindow -
//
//**************************************************************************

ULONG FASTCALL WU32GetDesktopWindow(PVDMFRAME pFrame)
{
    ULONG ul;

    UNREFERENCED_PARAMETER(pFrame);

    ul = GETHWND16(GetDesktopWindow());

    RETURN(ul);
}


//**************************************************************************
//  WU32GetDlgItem -
//
//**************************************************************************

ULONG FASTCALL WU32GetDlgItem(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETDLGITEM16 parg16;

    //
    // pass the child ID zero-extended.  this ID is the hMenu param to
    // CreateWindow, so USER gets this ID with hiword = 0.
    // Visual Basic relies on this.
    //


    GETARGPTR(pFrame, sizeof(GETDLGITEM16), parg16);

    ul = GETHWND16(GetDlgItem(HWND32(parg16->f1),WORD32(parg16->f2)));

    if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_DBASEHANDLEBUG) {
        ((PTDB)SEGPTR(pFrame->wTDB,0))->TDB_CompatHandle = (USHORT) ul;
    }


    FREEARGPTR(parg16);
    RETURN(ul);
}


//**************************************************************************
//  WU32GetMenu -
//
//**************************************************************************

ULONG FASTCALL WU32GetMenu(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETMENU16 parg16;

    GETARGPTR(pFrame, sizeof(GETMENU16), parg16);

    ul = GETHMENU16(GetMenu(HWND32(parg16->f1)));

    FREEARGPTR(parg16);
    RETURN(ul);
}



//**************************************************************************
//  WU32GetMenuItemCount -
//
//**************************************************************************

ULONG FASTCALL WU32GetMenuItemCount(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETMENUITEMCOUNT16 parg16;

    GETARGPTR(pFrame, sizeof(GETMENUITEMCOUNT16), parg16);

    ul = GETWORD16(GetMenuItemCount( HMENU32(parg16->f1) ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


//**************************************************************************
//  WU32GetSysColor -
//
//**************************************************************************

ULONG FASTCALL WU32GetSysColor(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETSYSCOLOR16 parg16;

    GETARGPTR(pFrame, sizeof(GETSYSCOLOR16), parg16);

    ul = GETDWORD16(GetSysColor( INT32(parg16->f1) ));

    FREEARGPTR(parg16);
    RETURN(ul);
}



//**************************************************************************
//  WU32GetSystemMetrics -
//
//**************************************************************************

ULONG FASTCALL WU32GetSystemMetrics(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETSYSTEMMETRICS16 parg16;
    int     sm;

    GETARGPTR(pFrame, sizeof(GETSYSTEMMETRICS16), parg16);

    sm = INT32(parg16->f1);

    ul = GETINT16(GetSystemMetrics(sm) );

    FREEARGPTR(parg16);
    RETURN(ul);
}


//**************************************************************************
//  WU32GetTopWindow -
//
//**************************************************************************

ULONG FASTCALL WU32GetTopWindow(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETTOPWINDOW16 parg16;

    GETARGPTR(pFrame, sizeof(GETTOPWINDOW16), parg16);

    ul = GETHWND16(GetTopWindow(HWND32(parg16->f1)));

    FREEARGPTR(parg16);
    RETURN(ul);
}



//**************************************************************************
//  WU32GetWindowRect -
//
//**************************************************************************

ULONG FASTCALL WU32GetWindowRect(PVDMFRAME pFrame)
{
    RECT t2;
    register PGETWINDOWRECT16 parg16;

    GETARGPTR(pFrame, sizeof(GETWINDOWRECT16), parg16);

    /*
     * Home Design Gold 2.0
     *
     * If the call fails, don't overwrite the passed-in
     * rect.
     */
    if (GetWindowRect(HWND32(parg16->f1), &t2)) {
        PUTRECT16(parg16->f2, &t2);
    }

    FREEARGPTR(parg16);
    RETURN(0);
}



//**************************************************************************
//  WU32IsWindow -
//
//**************************************************************************

ULONG FASTCALL WU32IsWindow(PVDMFRAME pFrame)
{
    ULONG  ul;
    HWND   hWnd;
    register PISWINDOW16 parg16;

    GETARGPTR(pFrame, sizeof(ISWINDOW16), parg16);

    hWnd = HWND32(parg16->f1);

    ul = GETBOOL16(IsWindow(hWnd));

    // For apps that get burned by recycled handles -- ie. the old handle they
    // had has been destroyed & realloc'd to a different window -- not the one
    // they were expecting.  This needs to be handled on an app by app basis.
    if(ul && (CURRENTPTD()->dwWOWCompatFlagsEx & WOWCFEX_FAKENOTAWINDOW)) {

        // NetScape 4.0x install (the bug is in InstallShield)
        // Test the offset portion of the 16:16 return address to this call.
        // Bug #132616 et al
        switch(pFrame->vpCSIP & 0x0000FFFF) {

            case 0x4880:  // (InstallShield 3.00.104.0)
            case 0x44E4:  // (InstallShield 3.00.091.0)

            {
                ULONG  result;
                LPVOID lp;

                // we only want this to fail for calls during Int.Shld cleanup
                // we probably shouldn't fail it if was created by a WOW process
                result = GetWindowLong(hWnd, GWL_WNDPROC);
                if(!IsWOWProc(result)) {
                    goto IW_HACK;
                }

                // extra sanity check: InstallSheild calls GetWindowLong & uses
                // the returned value as a 16:16 ptr
                result = GetWindowLong(hWnd, DWL_MSGRESULT);
                GETVDMPTR(result, sizeof(VPVOID), lp);
                if(!lp) {
                    goto IW_HACK;
                }
                break;
                    
            }
        }
    }

    FREEARGPTR(parg16);
    RETURN(ul);

IW_HACK:
    WOW32WARNMSG((0),"WOW32::IsWindow hack hit!\n");
    RETURN(0);
   
}



//**************************************************************************
//  WU32ScreenToClient -
//
//**************************************************************************

ULONG FASTCALL WU32ScreenToClient(PVDMFRAME pFrame)
{
    POINT t2;
    register PSCREENTOCLIENT16 parg16;

    GETARGPTR(pFrame, sizeof(SCREENTOCLIENT16), parg16);
    GETPOINT16(parg16->f2, &t2);

    ScreenToClient( HWND32(parg16->f1), &t2 );

    PUTPOINT16(parg16->f2, &t2);
    FREEARGPTR(parg16);
    RETURN(0);
}


//**************************************************************************
//  WU32IsChild -
//
//**************************************************************************

ULONG FASTCALL WU32IsChild(PVDMFRAME pFrame)
{
    ULONG ul;
    register PISCHILD16 parg16;

    GETARGPTR(pFrame, sizeof(ISCHILD16), parg16);

    ul = GETBOOL16(IsChild( HWND32(parg16->f1), HWND32(parg16->f2) ));

    FREEARGPTR(parg16);
    RETURN(ul);
}



//**************************************************************************
//  WU32IsIconic -
//
//**************************************************************************

ULONG FASTCALL WU32IsIconic(PVDMFRAME pFrame)
{
    ULONG ul;
    register PISICONIC16 parg16;

    GETARGPTR(pFrame, sizeof(ISICONIC16), parg16);

    ul = GETBOOL16(IsIconic( HWND32(parg16->f1) ));

    FREEARGPTR(parg16);
    RETURN(ul);
}



//**************************************************************************
//  WU32IsWindowEnabled -
//
//**************************************************************************

ULONG FASTCALL WU32IsWindowEnabled(PVDMFRAME pFrame)
{
    ULONG ul;
    register PISWINDOWENABLED16 parg16;

    GETARGPTR(pFrame, sizeof(ISWINDOWENABLED16), parg16);

    ul = GETBOOL16(IsWindowEnabled( HWND32(parg16->f1) ));

    FREEARGPTR(parg16);
    RETURN(ul);
}



//**************************************************************************
//  WU32IsWindowVisible -
//
//**************************************************************************

ULONG FASTCALL WU32IsWindowVisible(PVDMFRAME pFrame)
{
    ULONG ul;
    register PISWINDOWVISIBLE16 parg16;

    GETARGPTR(pFrame, sizeof(ISWINDOWVISIBLE16), parg16);

    ul = GETBOOL16(IsWindowVisible( HWND32(parg16->f1) ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


//**************************************************************************
//  WU32IsZoomed -
//
//**************************************************************************

ULONG FASTCALL WU32IsZoomed(PVDMFRAME pFrame)
{
    ULONG ul;
    register PISZOOMED16 parg16;

    GETARGPTR(pFrame, sizeof(ISZOOMED16), parg16);

    ul = GETBOOL16(IsZoomed( HWND32(parg16->f1) ));

    FREEARGPTR(parg16);
    RETURN(ul);
}



//**************************************************************************
//  WU32GetTickCount -
//
//**************************************************************************

ULONG FASTCALL WU32GetTickCount(PVDMFRAME pFrame)
{
    ULONG   ul;

    UNREFERENCED_PARAMETER(pFrame);

    ul = (ULONG)GetTickCount();

    if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_GRAINYTICS) {

        //
        // round down to the nearest 55ms    this is for RelayGold, which
        // spins calling this API until consecutive calls return a delta
        // greater than 52.
        //

        ul = ul - (ul % 55);
    }

    RETURN(ul);
}



//**************************************************************************
//  On I386 all these functions her handled on clientside. But conditionally
//  they may endup doing the actual work via these thunks.
//
//  So any changes here like 'win31 compatiblity code' may have to be added
//  in mvdm\wow16\user\usercli.asm too.
//
//                                                               - nanduri
//**************************************************************************


//**************************************************************************
//  WU32DefHookProc -
//
//**************************************************************************

ULONG FASTCALL WU32DefHookProc(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    register PDEFHOOKPROC16 parg16;
    HOOKSTATEDATA HkData;
    ULONG hHook16;
    INT iHookCode;
    INT nCode;
    LONG wParam;
    LONG lParam;
    LPINT lpiFunc;

    GETARGPTR(pFrame, sizeof(DEFHOOKPROC16), parg16);

    nCode = INT32(parg16->f1);
    wParam = WORD32(parg16->f2);
    lParam = DWORD32(parg16->f3);

    GETMISCPTR(parg16->f4, lpiFunc);
    hHook16 = FETCHDWORD(*lpiFunc);
    FREEVDMPTR(lpiFunc);

    if (ISVALIDHHOOK(hHook16)) {
        iHookCode = GETHHOOKINDEX(hHook16);
        HkData.iIndex = (BYTE)iHookCode;
        if ( W32GetHookStateData( &HkData ) ) {
            ul = (ULONG)WU32StdDefHookProc(nCode, wParam, lParam, iHookCode);
        }
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


//**************************************************************************
//  WU32GetKeyState -
//
//**************************************************************************

ULONG FASTCALL WU32GetKeyState(PVDMFRAME pFrame)
{
    ULONG ul;
    SHORT sTmp;
    register PGETKEYSTATE16 parg16;

    GETARGPTR(pFrame, sizeof(GETKEYSTATE16), parg16);

    sTmp = GetKeyState(INT32(parg16->f1));

    // compatiblity:
    // MSTEST (testdrvr.exe) tests the bit 0x80 for checking the
    // shift key state. This works in win31 because the keystate in win31 is
    // one byte long and because of similar code below
    //
    // win31 code is similar to:
    //             mov al, byte ptr keystate
    //             cbw
    //             ret
    //
    // if 'al' is 0x80, cbw will make ax = 0xff80 and thus in win31
    // (state & 0x8000) and (state & 0x0080) will work and mean the same.
    //

    ul = (ULONG)((sTmp & 0x8000) ? (sTmp | 0x80) : sTmp);



    FREEARGPTR(parg16);
    RETURN(ul);
}



//**************************************************************************
//  WU32GetKeyboardState -
//
//**************************************************************************

ULONG FASTCALL WU32GetKeyboardState(PVDMFRAME pFrame)
{
    PBYTE pb1;
    register PGETKEYBOARDSTATE16 parg16;

    GETARGPTR(pFrame, sizeof(GETKEYBOARDSTATE16), parg16);
    ALLOCVDMPTR(parg16->f1, 256, pb1);

#ifdef HACK32   // bug 5704
    if (pb1) {
        GetKeyboardState( pb1 );
    }
#else
        GetKeyboardState( pb1 );
#endif

    FLUSHVDMPTR(parg16->f1, 256, pb1);
    FREEVDMPTR(pb1);
    FREEARGPTR(parg16);
    RETURN(0);
}
