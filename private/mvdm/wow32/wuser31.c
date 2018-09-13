/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WUSER31.C
 *  WOW32 16-bit Win 3.1 User API support
 *
 *  History:
 *  Created 16-Mar-1992 by Chandan S. Chauhan (ChandanC)
--*/

#include "precomp.h"
#pragma hdrstop

MODNAME(wuser31.c);

ULONG FASTCALL WU32DlgDirSelectComboBoxEx(PVDMFRAME pFrame)
{
    ULONG    ul;
    PSZ      psz2;
    VPVOID   vp;
    register PDLGDIRSELECTCOMBOBOXEX16 parg16;

    GETARGPTR(pFrame, sizeof(DLGDIRSELECTCOMBOBOXEX16), parg16);
    GETVDMPTR(parg16->f2, INT32(parg16->f3), psz2);
    vp = parg16->f2;

    // note: this calls back to 16-bit code and could invalidate the flat ptrs
    ul = GETBOOL16(DlgDirSelectComboBoxEx(
    HWND32(parg16->f1),
    psz2,
    INT32(parg16->f3), 
    WORD32(parg16->f4) // we zero-extend window IDs everywhere
    ));

    // special case to keep common dialog structs in sync (see wcommdlg.c)
    Check_ComDlg_pszptr(CURRENTPTD()->CommDlgTd, vp);

    FLUSHVDMPTR(parg16->f2, INT32(parg16->f3), psz2);
    FREEVDMPTR(psz2);
    FREEARGPTR(parg16);
    RETURN (ul);
}


ULONG FASTCALL WU32DlgDirSelectEx(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz2;
    VPVOID vp;
    register PDLGDIRSELECTEX16 parg16;

    GETARGPTR(pFrame, sizeof(DLGDIRSELECTEX16), parg16);
    GETVDMPTR(parg16->f2, INT32(parg16->f3), psz2);
    vp = parg16->f2;

    ul = GETBOOL16(DlgDirSelectEx(
    HWND32(parg16->f1),
    psz2,
    INT32(parg16->f3),
    WORD32(parg16->f4)
    ));

    // special case to keep common dialog structs in sync (see wcommdlg.c)
    Check_ComDlg_pszptr(CURRENTPTD()->CommDlgTd, vp);

    FLUSHVDMPTR(parg16->f2, INT32(parg16->f3), psz2);
    FREEVDMPTR(psz2);
    FREEARGPTR(parg16);
    RETURN (ul);
}


ULONG FASTCALL WU32GetClipCursor(PVDMFRAME pFrame)
{
    RECT Rect;
    register PGETCLIPCURSOR16 parg16;

    GETARGPTR(pFrame, sizeof(GETCLIPCURSOR16), parg16);

    GetClipCursor(&Rect);

    PUTRECT16(parg16->f1, &Rect);

    FREEARGPTR(parg16);

    RETURN (0);  // GetClipCursor has no return value
}


ULONG FASTCALL WU32GetDCEx(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETDCEX16 parg16;
    HAND16 htask16 = pFrame->wTDB;

    GETARGPTR(pFrame, sizeof(GETDCEX16), parg16);

    ul = GETHDC16(GetDCEx(HWND32(parg16->f1),
                          HRGN32(parg16->f2),
                          DWORD32(parg16->f3)));

    if (ul)
        StoreDC(htask16, parg16->f1, (HAND16)ul);

    FREEARGPTR(parg16);

    RETURN (ul);
}


ULONG FASTCALL WU32RedrawWindow(PVDMFRAME pFrame)
{
    ULONG ul;
    RECT Rect, *p2;
    register PREDRAWWINDOW16 parg16;

    GETARGPTR(pFrame, sizeof(REDRAWWINDOW16), parg16);

    p2 = GETRECT16 (parg16->f2, &Rect);

    ul = GETBOOL16(RedrawWindow(HWND32(parg16->f1),
                                p2,
                                HRGN32(parg16->f3),
                                WORD32(parg16->f4)));

    FREEARGPTR(parg16);

    RETURN (ul);
}


ULONG FASTCALL WU32ScrollWindowEx(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSCROLLWINDOWEX16 parg16;

    RECT RectScroll, *p4;
    RECT RectClip, *p5;
    RECT RectUpdate;

    GETARGPTR(pFrame, sizeof(SCROLLWINDOWEX16), parg16);
    p4 = GETRECT16 (parg16->f4, &RectScroll);
    p5 = GETRECT16 (parg16->f5, &RectClip);

    ul = GETINT16(ScrollWindowEx(HWND32(parg16->f1),
                                 INT32(parg16->f2),
                                 INT32(parg16->f3),
                                 p4,
                                 p5,
                                 HRGN32(parg16->f6),
                                 &RectUpdate,
                                 UINT32(parg16->f8)));

    PUTRECT16 (parg16->f7, &RectUpdate);

    FREEARGPTR(parg16);

    RETURN (ul);
}


ULONG FASTCALL WU32SystemParametersInfo(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    register PSYSTEMPARAMETERSINFO16 parg16;
    UINT    wParam;
    LONG    vParam;
    LOGFONT lf;
    INT     iMouse[3];
    PVOID   lpvParam;
    PWORD16 lpw;
    PDWORD16 lpdw;
    RECT    rect;
#ifndef _X86_
    DWORD   dwSize;
    LPBYTE  lpFree = NULL;
#endif

    GETARGPTR(pFrame, sizeof(SYSTEMPARAMETERSINFO16), parg16);

    // Assume these parameters fly straight through; fix them up per option
    // if they don't
    wParam = parg16->f2;
    lpvParam = &vParam;

    switch (parg16->f1) {

    case SPI_GETICONTITLELOGFONT:
        wParam = sizeof(LOGFONT);
        lpvParam = &lf;
        break;

    case SPI_SETICONTITLELOGFONT:
        GETLOGFONT16(parg16->f3, &lf);
        wParam = sizeof(LOGFONT);
        lpvParam = &lf;
        break;

    case SPI_GETMOUSE:
    case SPI_SETMOUSE:
        lpvParam = iMouse;
        break;

    case SPI_SETDESKPATTERN:
        // For the pattern if wParam == -1 then no string for lpvParam copy as is
        if (parg16->f2 == 0xFFFF) {
            wParam = 0xFFFFFFFF;
            lpvParam = (PVOID)parg16->f3;
            break;
        }
        // Otherwise fall through and do a string check

    case SPI_SETDESKWALLPAPER:
        // lpvParam (f3) is may be 0,-1 or a string
        if (parg16->f3 == 0xFFFF) {
            lpvParam = (PVOID)0xFFFFFFFF;
            break;
        }
        if (parg16->f3 == 0) {
            lpvParam = (PVOID)NULL;
            break;
        }
        // Otherwise fall through and do a string copy

    case SPI_LANGDRIVER:
        GETPSZPTR(parg16->f3, lpvParam);
        break;

    //
    // SPI_GET structures pointed to by pvParam, size in first dword of struct.
    // Note all these assume the Win16 and Win32 structures are equal.
    // These are all new for Win95 and thankfully that's true.  However unlike
    // Win95 we need to ensure the buffer passed to Win32 is aligned on RISC.
    // To have common code to thunk all these various structures, we align to
    // 16 bytes.
    //

    case SPI_GETACCESSTIMEOUT:
    case SPI_GETANIMATION:
    case SPI_GETNONCLIENTMETRICS:
    case SPI_GETMINIMIZEDMETRICS:
    case SPI_GETICONMETRICS:
    case SPI_GETFILTERKEYS:
    case SPI_GETSTICKYKEYS:
    case SPI_GETTOGGLEKEYS:
    case SPI_GETMOUSEKEYS:
    case SPI_GETSOUNDSENTRY:
#ifndef _X86_
        GETMISCPTR(parg16->f3, lpdw);
        dwSize = *lpdw;
        lpFree = malloc_w(dwSize + 16);
        lpvParam = (LPVOID)(((DWORD)lpFree + 16) & ~(16 - 1));
        *(PDWORD16)lpvParam = dwSize;
        break;
#endif             // otherwise fall through to simple struct case

    //
    // SPI_SET structures pointed to by pvParam, size in first dword of struct.
    // Note all these assume the Win16 and Win32 structures are equal.
    // These are all new for Win95 and thankfully that's true.  However unlike
    // Win95 we need to ensure the buffer passed to Win32 is aligned on RISC.
    // To have common code to thunk all these various structures, we align to
    // 16 bytes.
    //

    case SPI_SETANIMATION:
    case SPI_SETICONMETRICS:
    case SPI_SETMINIMIZEDMETRICS:
    case SPI_SETNONCLIENTMETRICS:
    case SPI_SETACCESSTIMEOUT:
#ifndef _X86_
        GETMISCPTR(parg16->f3, lpdw);
        dwSize = *lpdw;
        lpFree = malloc_w(dwSize + 16);
        lpvParam = (LPVOID)(((DWORD)lpFree + 16) & ~(16 - 1));
        RtlCopyMemory(lpvParam, lpdw, dwSize);
        break;
#endif             // otherwise fall through to simple struct case

    //
    // structures pointed to by pvParam, size in uiParam or first dword.
    // Note all these assume the Win16 and Win32 structures are equal.
    // These are all new for Win95 and thankfully that's true.
    //

    case SPI_GETHIGHCONTRAST:
    case SPI_GETSERIALKEYS:
    case SPI_SETDEFAULTINPUTLANG:
    case SPI_SETFILTERKEYS:
    case SPI_SETHIGHCONTRAST:
    case SPI_SETMOUSEKEYS:
    case SPI_SETSERIALKEYS:
    case SPI_SETSHOWSOUNDS:
    case SPI_SETSOUNDSENTRY:
    case SPI_SETSTICKYKEYS:
    case SPI_SETTOGGLEKEYS:
        GETMISCPTR(parg16->f3, lpvParam);
        break;

    //
    // pvParam points to WORD or BOOL
    //

    case SPI_GETBEEP:
    case SPI_GETBORDER:
    case SPI_GETDRAGFULLWINDOWS:
    case SPI_GETFASTTASKSWITCH:
    case SPI_GETFONTSMOOTHING:
    case SPI_GETGRIDGRANULARITY:
    case SPI_GETICONTITLEWRAP:
    case SPI_GETKEYBOARDSPEED:
    case SPI_GETKEYBOARDDELAY:
    case SPI_GETKEYBOARDPREF:
    case SPI_GETLOWPOWERACTIVE:
    case SPI_GETLOWPOWERTIMEOUT:
    case SPI_GETMENUDROPALIGNMENT:
    case SPI_GETMOUSETRAILS:
    case SPI_GETPOWEROFFACTIVE:
    case SPI_GETPOWEROFFTIMEOUT:
    case SPI_GETSCREENREADER:
    case SPI_GETSCREENSAVEACTIVE:
    case SPI_GETSCREENSAVETIMEOUT:
    case SPI_GETSHOWSOUNDS:
    case SPI_SCREENSAVERRUNNING:
        break;

    //
    // pvParam points to DWORD
    //

    case SPI_GETDEFAULTINPUTLANG:
        break;

    //
    // pvParam not used
    //

    case SPI_GETWINDOWSEXTENSION:
    case SPI_ICONHORIZONTALSPACING:
    case SPI_ICONVERTICALSPACING:
    case SPI_SETBEEP:
    case SPI_SETBORDER:
    case SPI_SETDOUBLECLICKTIME:
    case SPI_SETDOUBLECLKHEIGHT:
    case SPI_SETDOUBLECLKWIDTH:
    case SPI_SETDRAGFULLWINDOWS:
    case SPI_SETDRAGHEIGHT:
    case SPI_SETDRAGWIDTH:
    case SPI_SETFASTTASKSWITCH:
    case SPI_SETFONTSMOOTHING:
    case SPI_SETGRIDGRANULARITY:
    case SPI_SETHANDHELD:
    case SPI_SETICONTITLEWRAP:
    case SPI_SETKEYBOARDDELAY:
    case SPI_SETKEYBOARDPREF:
    case SPI_SETKEYBOARDSPEED:
    case SPI_SETLANGTOGGLE:
    case SPI_SETLOWPOWERACTIVE:
    case SPI_SETLOWPOWERTIMEOUT:
    case SPI_SETMENUDROPALIGNMENT:
    case SPI_SETMOUSEBUTTONSWAP:
    case SPI_SETMOUSETRAILS:
    case SPI_SETPENWINDOWS:
    case SPI_SETPOWEROFFACTIVE:
    case SPI_SETPOWEROFFTIMEOUT:
    case SPI_SETSCREENREADER:
    case SPI_SETSCREENSAVEACTIVE:
    case SPI_SETSCREENSAVETIMEOUT:
        break;

    //
    // pvParam points to a RECT
    //

    case SPI_GETWORKAREA:
    case SPI_SETWORKAREA:
        GETRECT16(parg16->f3, &rect);
        lpvParam = &rect;
        break;


    default:
#ifdef DEBUG
        {
            DWORD dwSaveOptions = flOptions;
            flOptions |= OPT_DEBUG;
            LOGDEBUG(0, ("WARNING SystemParametersInfo case %d not pre-thunked in WOW!\n", parg16->f1));
            flOptions = dwSaveOptions;
        }
#endif
        break;
    }


    ul = SystemParametersInfo(
        UINT32(parg16->f1),
        wParam,
        lpvParam,
        UINT32(parg16->f4)
        );


    switch (parg16->f1) {
    case SPI_GETICONTITLELOGFONT:
        PUTLOGFONT16(parg16->f3, sizeof(LOGFONT), lpvParam);
        break;

    case SPI_SETICONTITLELOGFONT:
        break;

    case SPI_GETMOUSE:
    case SPI_SETMOUSE:
        PUTINTARRAY16(parg16->f3, 3, lpvParam);
        break;

    case SPI_LANGDRIVER:
    case SPI_SETDESKWALLPAPER:
        FREEPSZPTR(lpvParam);
        break;

    case SPI_ICONHORIZONTALSPACING:
    case SPI_ICONVERTICALSPACING:
        // optional outee
        if (!parg16->f3)
            break;

        // fall through


    //
    // pvParam points to WORD or BOOL
    //

    case SPI_GETBEEP:
    case SPI_GETBORDER:
    case SPI_GETDRAGFULLWINDOWS:
    case SPI_GETFASTTASKSWITCH:
    case SPI_GETFONTSMOOTHING:
    case SPI_GETGRIDGRANULARITY:
    case SPI_GETICONTITLEWRAP:
    case SPI_GETKEYBOARDSPEED:
    case SPI_GETKEYBOARDDELAY:
    case SPI_GETKEYBOARDPREF:
    case SPI_GETLOWPOWERACTIVE:
    case SPI_GETLOWPOWERTIMEOUT:
    case SPI_GETMENUDROPALIGNMENT:
    case SPI_GETMOUSETRAILS:
    case SPI_GETPOWEROFFACTIVE:
    case SPI_GETPOWEROFFTIMEOUT:
    case SPI_GETSCREENREADER:
    case SPI_GETSCREENSAVEACTIVE:
    case SPI_GETSCREENSAVETIMEOUT:
    case SPI_GETSHOWSOUNDS:
    case SPI_SCREENSAVERRUNNING:
        GETVDMPTR(FETCHDWORD(parg16->f3), sizeof(*lpw), lpw);

        *lpw = (WORD)(*(LPLONG)lpvParam);

        FLUSHVDMPTR(FETCHDWORD(parg16->f3), sizeof(*lpw), lpw);
        FREEVDMPTR(lpw);

        break;

    //
    // pvParam points to DWORD
    //

    case SPI_GETDEFAULTINPUTLANG:
        GETVDMPTR(FETCHDWORD(parg16->f3), sizeof(*lpdw), lpdw);

        *lpdw = *(LPDWORD)lpvParam;

        FLUSHVDMPTR(FETCHDWORD(parg16->f3), sizeof(*lpdw), lpdw);
        FREEVDMPTR(lpdw);

        break;

    //
    // SPI_GET structures pointed to by pvParam, size in first dword of struct.
    // Note all these assume the Win16 and Win32 structures are equal.
    // These are all new for Win95 and thankfully that's true.  However unlike
    // Win95 we need to ensure the buffer passed to Win32 is aligned.  In order
    // to have common code to thunk all these various structures, we align to
    // 16 bytes.
    //

    case SPI_GETACCESSTIMEOUT:
    case SPI_GETANIMATION:
    case SPI_GETNONCLIENTMETRICS:
    case SPI_GETMINIMIZEDMETRICS:
    case SPI_GETICONMETRICS:
    case SPI_GETFILTERKEYS:
    case SPI_GETSTICKYKEYS:
    case SPI_GETTOGGLEKEYS:
    case SPI_GETMOUSEKEYS:
    case SPI_GETSOUNDSENTRY:
#ifndef _X86_
        RtlCopyMemory(lpdw, lpvParam, dwSize);
        FREEMISCPTR(lpdw);
        break;
#endif             // otherwise fall through to simple struct case

    //
    // SPI_SET structures pointed to by pvParam, size in first dword of struct.
    // Note all these assume the Win16 and Win32 structures are equal.
    // These are all new for Win95 and thankfully that's true.  However unlike
    // Win95 we need to ensure the buffer passed to Win32 is aligned.  In order
    // to have common code to thunk all these various structures, we align to
    // 16 bytes.
    //

    case SPI_SETANIMATION:
    case SPI_SETICONMETRICS:
    case SPI_SETMINIMIZEDMETRICS:
    case SPI_SETNONCLIENTMETRICS:
    case SPI_SETACCESSTIMEOUT:
#ifndef _X86_
        FREEMISCPTR(lpdw);
        break;
#endif             // otherwise fall through to simple struct case

    //
    // structures pointed to by pvParam, size in uiParam or first dword.
    // Note all these assume the Win16 and Win32 structures are equal.
    // These are all new for Win95 and thankfully that's true.
    //

    case SPI_GETHIGHCONTRAST:
    case SPI_GETSERIALKEYS:
    case SPI_SETDEFAULTINPUTLANG:
    case SPI_SETFILTERKEYS:
    case SPI_SETHIGHCONTRAST:
    case SPI_SETMOUSEKEYS:
    case SPI_SETSERIALKEYS:
    case SPI_SETSHOWSOUNDS:
    case SPI_SETSOUNDSENTRY:
    case SPI_SETSTICKYKEYS:
    case SPI_SETTOGGLEKEYS:
        FREEMISCPTR(lpvParam);
        break;


    //
    // pvParam not used
    //

    case SPI_GETWINDOWSEXTENSION:
    case SPI_SETBEEP:
    case SPI_SETBORDER:
    case SPI_SETDOUBLECLICKTIME:
    case SPI_SETDOUBLECLKHEIGHT:
    case SPI_SETDOUBLECLKWIDTH:
    case SPI_SETDRAGFULLWINDOWS:
    case SPI_SETDRAGHEIGHT:
    case SPI_SETDRAGWIDTH:
    case SPI_SETFASTTASKSWITCH:
    case SPI_SETFONTSMOOTHING:
    case SPI_SETGRIDGRANULARITY:
    case SPI_SETHANDHELD:
    case SPI_SETICONTITLEWRAP:
    case SPI_SETKEYBOARDDELAY:
    case SPI_SETKEYBOARDPREF:
    case SPI_SETKEYBOARDSPEED:
    case SPI_SETLANGTOGGLE:
    case SPI_SETLOWPOWERACTIVE:
    case SPI_SETLOWPOWERTIMEOUT:
    case SPI_SETMENUDROPALIGNMENT:
    case SPI_SETMOUSEBUTTONSWAP:
    case SPI_SETMOUSETRAILS:
    case SPI_SETPENWINDOWS:
    case SPI_SETPOWEROFFACTIVE:
    case SPI_SETPOWEROFFTIMEOUT:
    case SPI_SETSCREENREADER:
    case SPI_SETSCREENSAVEACTIVE:
    case SPI_SETSCREENSAVETIMEOUT:
        break;

    //
    // pvParam points to a RECT
    //

    case SPI_GETWORKAREA:
    case SPI_SETWORKAREA:
        PUTRECT16(parg16->f3, &rect);
        break;


    default:
#ifdef DEBUG
        {
            DWORD dwSaveOptions = flOptions;
            flOptions |= OPT_DEBUG;
            LOGDEBUG(0, ("WARNING SystemParametersInfo case %d not post-thunked in WOW!\n", parg16->f1));
            flOptions = dwSaveOptions;
        }
#endif
        break;
    }

#ifndef _X86_
    if (lpFree) {
        free_w(lpFree);
    }
#endif

    FREEARGPTR(parg16);
    RETURN (ul);
}


ULONG FASTCALL WU32SetWindowPlacement(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    register PSETWINDOWPLACEMENT16 parg16;

    WINDOWPLACEMENT wndpl;


    GETARGPTR(pFrame, sizeof(SETWINDOWPLACEMENT16), parg16);

    WINDOWPLACEMENT16TO32(parg16->f2, &wndpl);

    ul = GETBOOL16(SetWindowPlacement(HWND32(parg16->f1),
                                      &wndpl));

    FREEARGPTR(parg16);
    RETURN (ul);
}


ULONG FASTCALL WU32GetWindowPlacement(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    register PGETWINDOWPLACEMENT16 parg16;

    WINDOWPLACEMENT wndpl;


    GETARGPTR(pFrame, sizeof(GETWINDOWPLACEMENT16), parg16);

    wndpl.length = sizeof(WINDOWPLACEMENT);

    ul = GETBOOL16(GetWindowPlacement(HWND32(parg16->f1),
                                      &wndpl));

    WINDOWPLACEMENT32TO16(parg16->f2, &wndpl);

    FREEARGPTR(parg16);
    RETURN (ul);
}



ULONG FASTCALL WU32GetFreeSystemResources(PVDMFRAME pFrame)
{
    ULONG ul = 90;

    UNREFERENCED_PARAMETER( pFrame );

    RETURN (ul);
}


ULONG FASTCALL WU32ExitWindowsExec(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    register PEXITWINDOWSEXEC16 parg16;
    LPSTR   lpstrProgName;
    LPSTR   lpstrCmdLine;
    UINT    lengthProgName;
    UINT    lengthCmdLine;
    BYTE    abT[512];


    GETARGPTR(pFrame, sizeof(EXITWINDOWSEXEC16), parg16);

    GETPSZPTR(parg16->vpProgName, lpstrProgName);
    GETPSZPTR(parg16->vpCmdLine, lpstrCmdLine);

    lengthProgName = (lpstrProgName) ? strlen(lpstrProgName) : 0;
    lengthCmdLine  = (lpstrCmdLine)  ? strlen(lpstrCmdLine)  : 0;

    WOW32ASSERT(sizeof(abT) > (lengthProgName+lengthCmdLine+2));

    strcpy(abT, "" );
    if ( lpstrProgName ) {
        strcpy(abT, lpstrProgName );
    }
    if ( lpstrCmdLine ) {
        strcat(abT, " " );
        strcat(abT, lpstrCmdLine );
    }

    //
    // We write the commandline to registry "WOW/EWExecCmdLine"
    // If the system logs off successfully, after reboot, we read
    // the registry and exec the specfied app before launching any
    // wow app in any wow vdm. We donot launch the app before logoff
    // because winlogon doesn't allow any app to be execed during
    // the logoff process.
    //                                                - nanduri

    // only one exitwindowsexec call at a time.
    // if value/key exists, return error.

    if (!W32EWExecData(EWEXEC_QUERY, abT, sizeof(abT))) {
        HANDLE hevT;

        // only one exitwindowsexec call at a time.
        // if event exits, return error.

        if (hevT = CreateEvent(NULL, TRUE, FALSE, WOWSZ_EWEXECEVENT)) {
            if (GetLastError() == 0) {
                // wake up any waiting threads (in w32ewexecer)

                SetEvent(hevT);

                // Write the data to the registry

                if (W32EWExecData(EWEXEC_SET, abT, strlen(abT)+1)) {
                    DWORD   dwlevel;
                    DWORD   dwflags;

                    if (!GetProcessShutdownParameters(&dwlevel, &dwflags)) {
                        dwlevel = 0x280;    // default level per docs
                        dwflags = 0;
                    }

                    //
                    // 0xff = last system reserved level  Logically makes this last user
                    // process to shutdown. This takes care of Multiple WOW VDMs
                    //

                    SetProcessShutdownParameters(0xff, 0);

                    //
                    // EWX_NOTIFY private bit for WOW. Generates queue message
                    // WM_ENDSESSION, if any process cancels logoff/shutdown.

                    if (ExitWindowsEx(EWX_LOGOFF | EWX_NOTIFY, 0)) {
                        MSG msg;

                        //
                        //  PeekMessage yields to other WOW tasks. We effectively
                        //  freeze the current task by removing all input messages.
                        //  Loop terminates only if WM_ENDSESSION message has been
                        //  received. This message is posted by winsrv if any process
                        //  in the system cancels logoff.
                        //

                        while (TRUE) {
                            if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
                                if ((msg.message >= WM_MOUSEFIRST &&
                                        msg.message <= WM_MOUSELAST) ||
                                     (msg.message >= WM_KEYFIRST &&
                                        msg.message <= WM_KEYLAST) ||
                                     (msg.message >= WM_NCMOUSEMOVE &&
                                        msg.message <= WM_NCMBUTTONDBLCLK)) {

                                    // don't dispatch the message

                                }
                                else if (msg.message == WM_ENDSESSION) {
                                    WOW32ASSERT((msg.hwnd == 0) && (msg.wParam == 0));
                                    break;
                                }
                                else {
                                    TranslateMessage(&msg);
                                    DispatchMessage(&msg);
                                }
                            }
                        }
                    }

                    //
                    // Here if logoff was cancelled.
                    // Set defaults and delete the associated value from registry.
                    //

                    SetProcessShutdownParameters(dwlevel, dwflags);
                    if (!W32EWExecData(EWEXEC_DEL, (LPSTR)NULL, 0)) {
                        WOW32ASSERT(FALSE);
                    }
                }
            }
            CloseHandle(hevT);
        }
    }

    LOGDEBUG(0,("WOW: ExitWindowsExec failed\r\n"));
    FREEARGPTR(parg16);
    return 0;
}

ULONG FASTCALL WU32MapWindowPoints(PVDMFRAME pFrame)
{
    LPPOINT p3;
    register PMAPWINDOWPOINTS16 parg16;
    POINT  BufferT[128];


    GETARGPTR(pFrame, sizeof(MAPWINDOWPOINTS16), parg16);
    p3 = STACKORHEAPALLOC(parg16->f4 * sizeof(POINT), sizeof(BufferT), BufferT);
    getpoint16(parg16->f3, parg16->f4, p3);

    MapWindowPoints(
        HWND32(parg16->f1),
        HWND32(parg16->f2),
        p3,
        INT32(parg16->f4)
    );

    PUTPOINTARRAY16(parg16->f3, parg16->f4, p3);
    STACKORHEAPFREE(p3, BufferT);
    FREEARGPTR(parg16);

    RETURN(1);
}
