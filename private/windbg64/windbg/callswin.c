/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    callswin.c

Abstract:

    This module contains the main line code for display of calls window.

Author:

    Wesley Witt (wesw) 6-Sep-1993

Environment:

    Win32, User Mode

--*/


#include "precomp.h"
#pragma hdrstop

#include "include\cntxthlp.h"

extern  CXF      CxfIp;
extern  LPSHF    Lpshf;


//
// Magic values found in reserved fields of STACKFRAME
//
#define SAVE_EBP(f)        f.Reserved[0]
#define TRAP_TSS(f)        f.Reserved[1]
#define TRAP_EDITED(f)     f.Reserved[1]
#define SAVE_TRAP(f)       f.Reserved[2]

#define ADDREQ(a,b) ((a).Offset  == (b).Offset &&   \
                     (a).Segment == (b).Segment &&  \
                     (a).Mode    == (b).Mode )

#ifndef NEW_WINDOWING_CODE
HWND              hWndCalls;
#endif

DWORD             FrameCountSave;
STACKFRAME        stkFrameSave;
STACKINFO         StackInfo[MAX_FRAMES];
DWORD             FrameCount;
HWND              hwndList;
int               myView;
HFONT             hFontList;
HBRUSH            hbrBackground;


extern LPPD    LppdCommand;
extern LPTD    LptdCommand;

extern HWND GetLocalHWND(void);

VOID
FillStackFrameWindow(
    HWND hwndList,
    BOOL fRefresh
    );

#if !defined( NEW_WINDOWING_CODELONG )
LONG
CreateCallsWindow(
    HWND        hWnd,
    WPARAM      wParam,
    LPARAM      lParam
    );
#endif

void
FormatStackFrameString(
    LPSTR             lsz,
    DWORD             cb,
    const STACKINFO * const si,
    DWORD             width
    );

BOOL
GoUntilStackFrame(
    LPSTACKINFO si
    );

VOID
BuildStackInfo(
    LPSTACKINFO    si,
    LPSTACKFRAME   stkFrame,
    int            frameNum,
    HPID           hpid,
    HTID           vhtid,
    BOOL           fFull
    );

HTID
WalkToFrame(
    HPID hpid,
    HTID htid,
    int iCall
    );


HWND
GetCallsHWND(
    VOID
    )

/*++

Routine Description:

    Helper function that returns the window handle
    for the calls window.

Arguments:

    None.

Return Value:

    HWND for the calls window, or NULL if the window is closed.

--*/

{
#ifdef NEW_WINDOWING_CODE
    return(g_DebuggerWindows.hwndCalls);
#else
    return(hWndCalls);
#endif
}

BOOL
IsCallsInFocus(
    VOID
    )

/*++

Routine Description:

    Determines whether the calls window is in focus.

Arguments:

    None.

Return Value:

    TRUE       - the calls window is in focus
    FALSE      - the calls window is not in focus

--*/

{
    HWND hwndFocus = GetFocus();

    if (hwndFocus == GetCallsHWND() ||
        hwndFocus == hwndList) {
        
        return TRUE;

    }

    return FALSE;
}

#ifdef NEW_WINDOWING_CODE
LRESULT
CALLBACK
NewCalls_WindowProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    LRESULT lbItem;
    PCALLSWIN_DATA pCallsWinData = GetCallsWinData(hwnd);

    switch (uMsg) {
    case WM_CREATE:
        {
            RECT rc;

            Assert(NULL == pCallsWinData);

            pCallsWinData = new CALLSWIN_DATA;
            if (!pCallsWinData) {
                return -1; // Fail window creation
            }

            pCallsWinData->hwndList = CreateWindowEx(
                0,                                          // Extended style
                "LISTBOX",                                  // class name
                NULL,                                       // title
                WS_CHILD | WS_VISIBLE
                | WS_MAXIMIZE
                | WS_HSCROLL | WS_VSCROLL
                | LBS_NOTIFY | LBS_WANTKEYBOARDINPUT
                | LBS_DISABLENOSCROLL,                      // style
                0,                                          // x
                0,                                          // y
                CW_USEDEFAULT,                              // width
                CW_USEDEFAULT,                              // height
                hwnd,                                       // parent
                (HMENU) IDC_LIST_CALLS,                     // control id
                g_hInst,                                    // hInstance
                NULL);                                      // user defined data

            if (!pCallsWinData->hwndList) {
                delete pCallsWinData;
                return -1; // Fail window creation
            }

            g_DebuggerWindows.hwndCalls = hwnd;
 
            // store this in the window
            SetCallsWinData(hwnd, pCallsWinData);
        }
        memset( &stkFrameSave.AddrFrame,  0, sizeof(ADDRESS) );
        memset( &stkFrameSave.AddrReturn, 0, sizeof(ADDRESS) );
        return 0;

    case WM_COMMAND:
        if (HIWORD(wParam) == LBN_DBLCLK) {
            lbItem = SendMessage( hwndList, LB_GETCURSEL, 0, 0 );
            
            if (LB_ERR != lbItem) {
                GotoFrame( (int) lbItem, TRUE ); // User activated
            }
        }
        if (LOWORD(wParam) == IDM_DEBUG_RUNTOCURSOR) {
            lbItem = SendMessage( hwndList, LB_GETCURSEL, 0, 0 );

            if (LB_ERR != lbItem) {
                if (!GoUntilStackFrame( &StackInfo[lbItem] )) {
                    MessageBeep( MB_OK );
                }
            }
        }
        break;

    case WM_VKEYTOITEM:
        if (LOWORD(wParam) == VK_RETURN) {
            lbItem = SendMessage( hwndList, LB_GETCURSEL, 0, 0 );
            if (LB_ERR != lbItem) {
                // User activated
                GotoFrame( (int) lbItem, TRUE );
            }
        } else if ('G' == LOWORD(wParam)) {
            lbItem = SendMessage( hwndList, LB_GETCURSEL, 0, 0 );
            if (LB_ERR != lbItem) {
                if (!GoUntilStackFrame( &StackInfo[lbItem] )) {
                    MessageBeep( MB_OK );
                }
            }
        } else if ('R' == LOWORD(wParam)) {
            FillStackFrameWindow( hwndList, TRUE );
        }
        break;

    case WU_OPTIONS:
        FillStackFrameWindow( hwndList, TRUE );
        return TRUE;

    case WU_UPDATE:
        FillStackFrameWindow( hwndList, TRUE );
        return TRUE;

    case WU_INFO:
        return TRUE;

    case WU_DBG_LOADEM:
        return TRUE;

    case WU_DBG_LOADEE:
        return TRUE;

    case WU_SETWATCH:
        return TRUE;

    case WU_AUTORUN:
        return TRUE;

    case WU_DBG_UNLOADEE:
    case WU_DBG_UNLOADEM:
    case WU_INVALIDATE:
        SendMessage( hwndList, LB_RESETCONTENT, 0, 0 );
        FrameCount = 0;
        return FALSE;

    case WM_SETFONT:
        //SendMessage( hwndList, WM_SETFONT, (WPARAM)hFontList, (LPARAM)FALSE );
        //InvalidateRect(hwndList, NULL, TRUE);
        //InvalidateRect(hwnd, NULL, TRUE);
        //FillStackFrameWindow( hwndList, TRUE );
        return TRUE;

    case WU_CLR_BACKCHANGE:
        DeleteObject( hbrBackground );
        hbrBackground = CreateSolidBrush( StringColors[CallsWindow].BkgndColor);
        return TRUE;

    case WU_CLR_FORECHANGE:
        return TRUE;

    case WM_CTLCOLORLISTBOX:
        SetBkColor( (HDC)wParam, StringColors[CallsWindow].BkgndColor );
        SetTextColor( (HDC)wParam, StringColors[CallsWindow].FgndColor);
        return (LRESULT)(hbrBackground);

    case WM_DESTROY:
        DeleteObject( hbrBackground );
        DeleteWindowMenuItem (myView);
        g_DebuggerWindows.hwndCalls = NULL;
        FrameCount = 0;
        hwndList = NULL;
        break;
    }

    return DefMDIChildProc(hwnd, uMsg, wParam, lParam);
}

#else //NEW_WINDOWING_CODE

LRESULT
CallsWndProc(
    HWND   hwnd,
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

    Window procedure for the calls stack window.

Arguments:

    hwnd       - window handle
    msg        - message number
    wParam     - first message parameter
    lParam     - second message parameter

Return Value:

    TRUE       - did not process the message
    FALSE      - did process the message

--*/
{
    DWORD_PTR    lbItem;
    RECT         cRect;
    LPCHOOSEFONT cf;


    switch (msg) {
        case WM_CREATE:
            memset( &stkFrameSave.AddrFrame,  0, sizeof(ADDRESS) );
            memset( &stkFrameSave.AddrReturn, 0, sizeof(ADDRESS) );
            return CreateCallsWindow( hwnd, wParam, lParam );

        case WM_MDIACTIVATE:
            if (hwnd == (HWND) lParam) {
                hwndActive = hwnd;
                hwndActiveEdit = hwnd;
                curView = myView;
                EnableToolbarControls();
            } else {
                hwndActive = NULL;
                hwndActiveEdit = NULL;
                curView = -1;
            }
            break;

        case WM_MOUSEACTIVATE:
            return MA_ACTIVATE;

        case WM_WINDOWPOSCHANGED:
            if (((LPWINDOWPOS)lParam)->flags & SWP_NOSIZE) {
                break;
            }
            GetClientRect( hwnd, &cRect );
            MoveWindow( hwndList,
                        0,
                        0,
                        cRect.right - cRect.left,
                        cRect.bottom - cRect.top,
                        TRUE
                      );
            FillStackFrameWindow( hwndList, FALSE );
            break;

        case WM_SETFOCUS:
            SetFocus( hwndList );
            return 0;

        case WM_COMMAND:
            if (HIWORD(wParam) == LBN_DBLCLK) {
                lbItem = SendMessage( hwndList, LB_GETCURSEL, 0, 0 );
                // User activated
                GotoFrame( (int) lbItem, TRUE );
            }
            if (LOWORD(wParam) == IDM_DEBUG_RUNTOCURSOR) {
                lbItem = SendMessage( hwndList, LB_GETCURSEL, 0, 0 );
                if (!GoUntilStackFrame( &StackInfo[lbItem] )) {
                    MessageBeep( MB_OK );
                }
            }
            break;

        case WM_VKEYTOITEM:
            if (LOWORD(wParam) == VK_RETURN) {
                lbItem = SendMessage( hwndList, LB_GETCURSEL, 0, 0 );
                // User activated
                GotoFrame( (int) lbItem, TRUE );
            } else if ('G' == LOWORD(wParam)) {
                lbItem = SendMessage( hwndList, LB_GETCURSEL, 0, 0 );
                if (!GoUntilStackFrame( &StackInfo[lbItem] )) {
                    MessageBeep( MB_OK );
                }
            } else if ('R' == LOWORD(wParam)) {
                FillStackFrameWindow( hwndList, TRUE );
            }
            break;

        case WU_OPTIONS:
            FillStackFrameWindow( hwndList, TRUE );
            return TRUE;

        case WU_UPDATE:
            FillStackFrameWindow( hwndList, TRUE );
            return TRUE;

        case WU_INFO:
            return TRUE;

        case WU_DBG_LOADEM:
            return TRUE;

        case WU_DBG_LOADEE:
            return TRUE;

        case WU_SETWATCH:
            return TRUE;

        case WU_AUTORUN:
            return TRUE;

        case WU_DBG_UNLOADEE:
        case WU_DBG_UNLOADEM:
        case WU_INVALIDATE:
            SendMessage( hwndList, LB_RESETCONTENT, 0, 0 );
            FrameCount = 0;
            return FALSE;

        case WM_SETFONT:
            cf = (LPCHOOSEFONT)lParam;
            DeleteObject( hFontList );
            hFontList = CreateFontIndirect( cf->lpLogFont );
            Views[myView].font = hFontList;
            SendMessage( hwndList, WM_SETFONT, (WPARAM)hFontList, (LPARAM)FALSE );
            InvalidateRect(hwndList, NULL, TRUE);
            InvalidateRect(hwnd, NULL, TRUE);
            FillStackFrameWindow( hwndList, TRUE );
            return TRUE;

        case WU_CLR_BACKCHANGE:
            DeleteObject( hbrBackground );
            hbrBackground = CreateSolidBrush( StringColors[CallsWindow].BkgndColor);
            return TRUE;

        case WU_CLR_FORECHANGE:
            return TRUE;

        case WM_CTLCOLORLISTBOX:
            SetBkColor( (HDC)wParam, StringColors[CallsWindow].BkgndColor );
            SetTextColor( (HDC)wParam, StringColors[CallsWindow].FgndColor);
            return (LRESULT)(hbrBackground);

        case WM_DESTROY:
            DeleteObject( hbrBackground );
            DeleteWindowMenuItem (myView);
            Views[myView].Doc = -1;
            hWndCalls = NULL;
            FrameCount = 0;
            hwndList = NULL;
            break;
    }

    return DefMDIChildProc( hwnd, msg, wParam, lParam );
}
#endif // NEW_WINDOWING_CODE

VOID
FillStackFrameWindow(
    HWND hwndList,
    BOOL fRefresh
    )

/*++

Routine Description:

    This functions clears the calls stack window and fills it with the
    stack trace information contained in the StackInfo structure.  If
    the fRefresh flag is TRUE then the stack trace is refreshed.

Arguments:

    hwndList    - Window handle for the listbox that contains the call stack
    fRefresh    - TRUE   - the stack trace should be refreshed
                  FALSE  - the current stack trace is simply displayed

Return Value:

    None.

--*/

{
    DWORD       i;
    char        buf[1024*4];
    HFONT       hFont;
    HDC         hdc;
    TEXTMETRIC  tm;
    DWORD       width;
    RECT        rect;
    DWORD_PTR   lbItem;
    SIZE        size;
    WPARAM      cxExtent = 0;


    i = g_contGlobalPreferences_WkSp.m_dwMaxFrames;
    if (GetCompleteStackTrace( 0, 0, 0, StackInfo, &i, !fRefresh, TRUE )) {
        FrameCount = i;
    }

    lbItem = SendMessage( hwndList, LB_GETCURSEL, 0, 0 );
    SendMessage( hwndList, WM_SETREDRAW, FALSE, 0L );
    SendMessage( hwndList, LB_RESETCONTENT, 0, 0 );

    //
    // get the text metrics for the font currently in use
    //
    hdc = GetDC( hwndList );
    hFont = (HFONT)SendMessage( hwndList, WM_GETFONT, 0, 0 );
    if (hFont != NULL) {
        SelectObject( hdc, hFont );
    }
    GetTextMetrics( hdc, &tm );

    //
    // is it a fixed pitched font?  (yes using not is correct!)
    //
    if (!(tm.tmPitchAndFamily & TMPF_FIXED_PITCH)) {

        //
        // calculate the width of the listbox in characters
        //
        GetWindowRect( hwndList, &rect );

        if (IsRectEmpty(&rect)) {

            width = 0;

        } else {

            if (FrameCount > (DWORD)((rect.bottom - rect.top) / tm.tmHeight)) {
                //
                // there will be a vertical scroll bar in the listbox
                // so we must subtract the width of the scroll bar
                // from the width
                //
                width = (rect.right - rect.left - GetSystemMetrics(SM_CXVSCROLL) - 1)
                        / tm.tmMaxCharWidth;
            } else {
                width = (rect.right - rect.left) / tm.tmMaxCharWidth;
            }

        }

    } else {

        //
        // zero says do not right justify the source info
        //
        width = 0;

    }

    for (i=0; i<FrameCount; i++) {

        //
        // format the string
        //
        FormatStackFrameString( buf, sizeof(buf), &StackInfo[i], width );

        //
        // get the width of the item
        //
        GetTextExtentPoint( hdc, buf, strlen(buf), &size );
        if (size.cx > (LONG)cxExtent) {
            cxExtent = size.cx;
        }

        //
        // now lets add the data to the listbox
        //
        SendMessage( hwndList, LB_ADDSTRING, 0, (LPARAM) buf );
    }

    cxExtent = cxExtent + tm.tmMaxCharWidth;

    if (g_contGlobalPreferences_WkSp.m_bHorzScrollBars) {
        SendMessage( hwndList, LB_SETHORIZONTALEXTENT, cxExtent, 0L );
    }
    SendMessage( hwndList, LB_SETCURSEL, (lbItem>FrameCount)?0:lbItem, 0 );
    SendMessage( hwndList, WM_SETREDRAW, TRUE, 0L );

    ReleaseDC( hwndList, hdc );

    return;
}

void
CreateAddress(
    LPADDRESS64     lpaddress,
    LPADDR          lpaddr
    )

/*++

Routine Description:

    Helper function that transforms a LPADDRESS into a LPADDR.  This is
    necessary because DBGHELP and WINDBG do not use the same address
    packet structures.

Arguments:

    lpaddress   - pointer to a source LPADDRESS structure
    lpaddr      - pointer to a destination LPADDR structure

Return Value:

    None.

--*/

{
    ZeroMemory( lpaddr, sizeof(*lpaddr) );

    lpaddr->addr.off     = lpaddress->Offset;
    lpaddr->addr.seg     = lpaddress->Segment;
    lpaddr->mode.fFlat   = lpaddress->Mode == AddrModeFlat;
    lpaddr->mode.fOff32  = lpaddress->Mode == AddrMode1632;
    lpaddr->mode.fReal   = lpaddress->Mode == AddrModeReal;

    OSDUnFixupAddr( LppdCur->hpid, LptdCur->htid, lpaddr );
}

void
LoadSymbols(
    LPADDRESS64 lpaddress
    )

/*++

Routine Description:

    Helper function that ensures that symbols are loaded for the
    address specified in the lpaddress argument.

Arguments:

    lpaddress   - pointer to a source LPADDRESS structure

Return Value:

    None.

--*/

{
    ADDR addr;


    CreateAddress( lpaddress, &addr );

    if ( (HPID)emiAddr( addr ) == LppdCur->hpid ) {
        //
        //  Get right EMI and load symbols if defered
        //
        emiAddr( addr ) = 0;
        ADDR_IS_LI( addr ) = FALSE;

#ifdef OSDEBUG4
        OSDSetEmi( LppdCur->hpid, LptdCur->htid, &addr );
#else
        OSDPtrace( osdSetEmi, wNull, &addr, LppdCur->hpid, LptdCur->htid );
#endif
        if ( (HPID)emiAddr( addr ) != LppdCur->hpid ) {

            SHWantSymbols( (HEXE)emiAddr( addr ) );

        }
    }
}


VOID
GetSymbolFromAddr(
    LPADDR       lpaddr,
    PHSYM        lpsymbol,
    LPDWORD      lpclt,
    LPBOOL       lpfInProlog
    )

/*++

Routine Description:

    This function gets an HSYM for the address provided.

Arguments:

    lpaddr       -  An ADDR struct contining the address of the symbol.
    lpsymbol     -  The HSYM that receives the HSYM for lpaddr.
    lpclt        -  Indicates the symbol type.
    lpfInProlog  -  Is the address in the function prolog?

Return Value:

    None.

--*/

{
    ADDR        addrT;
    HBLK        hblk;
    ADDR        addr;
    ADDR        addr2;
    CHAR        rgch[256];
    CXT         cxt;



    addr = *lpaddr;

    addr2 = *lpaddr;
    SYUnFixupAddr( &addr2 );

    ZeroMemory( &cxt, sizeof ( CXT ) );
    SHSetCxt( &addr, &cxt );

    if (SHHPROCFrompCXT(&cxt)) {

        *lpsymbol = (HSYM) SHHPROCFrompCXT ( &cxt );
        *lpclt = cltProc;
        SHAddrFromHsym( &addrT, *lpsymbol );
        SetAddrOff( &addr, GetAddrOff(addrT) );
        *lpfInProlog = SHIsInProlog( &cxt );

    } else if (SHHBLKFrompCXT( &cxt )) {

        hblk = SHHBLKFrompCXT( &cxt );
        ZeroMemory( &addrT, sizeof ( ADDR ) );
        SHAddrFromHsym( &addrT, (HSYM) hblk );
        if (SHGetSymName((HSYM)hblk,rgch) != NULL ) {
            *lpsymbol = (HSYM) hblk;
            *lpclt = cltBlk;
        } else {
            *lpsymbol = (HSYM) NULL;
            *lpclt = cltNone;
        }
        SetAddrOff( &addr, GetAddrOff ( addrT ) );
        *lpfInProlog = FALSE;

    } else
    if ( PHGetNearestHsym( &addr2,
                           (HEXE)(SHpADDRFrompCXT( &cxt ) ->emi),
                           (PHSYM) lpsymbol ) < 0xFFFFFFFF ) {

        ZeroMemory( &addrT, sizeof ( ADDR ) );
        SHAddrFromHsym( &addrT, *lpsymbol );
        *lpclt = cltPub;
        SetAddrOff( &addr, GetAddrOff( addrT ) );
        *lpfInProlog = FALSE;

    } else {

        *lpsymbol = (HSYM) NULL;
        *lpclt = cltNone;
        *lpfInProlog = FALSE;

    }
}

VOID
GetContextString(
    LPADDR       lpaddr,
    HSYM         symbol,
    DWORD        clt,
    LPSTR        lpszContext
    )

/*++

Routine Description:

    This function formats a context string for the address provided.

Arguments:

    lpaddr       -  An ADDR struct contining the address of the symbol.
    symbol       -  The HSYM for lpaddr.
    clt          -  Indicates the symbol type.
    lpszContext  -  Buffer that receives the context string.

Return Value:

    None.

--*/

{
    ADDR        addrT;
    ADDR        addr;
    HDEP        hstr;
    CXT         cxt;


    if (clt == cltNone) {
        return;
    }

    if (symbol == 0) {
        return;
    }

    addr = *lpaddr;

    ZeroMemory( &cxt, sizeof(CXT) );
    SHSetCxt( &addr, &cxt );
    if (clt != cltNone) {
        *SHpADDRFrompCXT( &cxt ) = addr;
    } else {
        *SHpADDRFrompCXT( &cxt ) = *lpaddr;
    }
    SHHMODFrompCXT( &cxt ) = SHHMODFrompCXT( &cxt );

    if (clt == cltProc) {

        SHAddrFromHsym( &addrT, symbol );
        SetAddrOff( SHpADDRFrompCXT( &cxt ), GetAddrOff( addrT ) );
        SHHPROCFrompCXT( &cxt ) = (HPROC) symbol;

    } else if (clt == cltBlk) {

        SHHBLKFrompCXT( &cxt ) = (HBLK) symbol;

    }

    EEFormatCXTFromPCXT( &cxt, &hstr, g_contWorkspace_WkSp.m_bShortContext );
    if (g_contWorkspace_WkSp.m_bShortContext) {
        strcpy( lpszContext, (LPSTR)MMLpvLockMb(hstr) );
    } else {
        BPShortenContext( (LPSTR)MMLpvLockMb(hstr), lpszContext );
    }

    MMbUnlockMb(hstr);
    EEFreeStr(hstr);
}


VOID
GetDisplacement(
    LPADDR       lpaddr,
    HSYM         symbol,
    ADDRESS64    addrPC,
    PDWORDLONG   lpqwDisplacement,
    LPADDR       lpProcAddr
    )

/*++

Routine Description:

    This function get the function address and the displacement that lpaddr is
    from the beginning of the function.

Arguments:

    lpaddr            -  An ADDR struct contining the address of the symbol.
    symbol            -  The HSYM for lpaddr.
    addrPC            -  Current program counter that is used for the displacement
                         calculation.
    lpdwDisplacement  -  Pointer to a DWORD that receives the displacement.
    lpProcAddr        -  Pointer to an ADDR that receives the function address.

Return Value:

    None.

--*/

{
    ADDR addr;
    CXT  cxt;


    if (symbol == 0) {
        return;
    }

    addr = *lpaddr;
    ZeroMemory( &cxt, sizeof ( CXT ) );
    SHSetCxt( &addr, &cxt );

    addr = *SHpAddrFrompCxt( &cxt );
    SHAddrFromHsym( &addr, symbol );
    SYFixupAddr( &addr );
    *lpProcAddr = addr;
    SYUnFixupAddr( lpProcAddr );
    *lpqwDisplacement = addrPC.Offset - GetAddrOff(addr);
}


VOID
GetModuleName(
    LPADDR       lpaddr,
    LPSTR        lpszModule
    )

/*++

Routine Description:

    This function get the module name for the lpaddr provided.

Arguments:

    lpaddr            -  An ADDR struct contining the address of the symbol.
    lpszModule        -  Buffer that receives the module name.

Return Value:

    None.

--*/

{
    ADDR addr;

    addr = *lpaddr;
    SYFixupAddr( &addr );
    SHGetModule( &addr, lpszModule );
    CharUpper( lpszModule );
}


VOID
GetFrameInfo(
    LPADDR      lpaddr,
    ADDRESS64   AddrFrame,
    PCXF        lpCxf,
    HTID        vhtid
    )

/*++

Routine Description:

    This function fills in the FRAME structure in the lpCxf passed in.
    The FRAME information is used to position the debugger at a virtual
    frame for the purposes of examining the locals at that frame.  It
    is also used to evaluate an expression in the context of a previous
    stack frame.

Arguments:

    lpaddr      -  Supplies an ADDR struct contining the address of the symbol.

    AddrFrame   -  Supplies desired stack frame address.

    lpCxf       -  Supplies pointer to a CXF structure that receives the FRAME.

    vhtid       - Supplies OSDebug handle for frame

Return Value:

    None.

--*/

{
    ADDR        addrData;
    ADDR        addrT;

    addrT = *lpaddr;
    SYFixupAddr( &addrT );
    *SHpADDRFrompCXT(SHpCXTFrompCXF( lpCxf )) = addrT;

    SHhFrameFrompCXF(lpCxf) = (HFRAME) vhtid;

    if (!ADDR_IS_LI(addrT)) {
        SYUnFixupAddr( &addrT );
    }
    SHSetCxt(&addrT, SHpCXTFrompCXF( lpCxf ));
}


VOID
GetFunctionName(
    LPADDR       lpaddr,
    HSYM         symbol,
    LPSTR        lpszProcName
    )

/*++

Routine Description:

    This function get the function name for the provided symbol.

Arguments:

    lpaddr       -  An ADDR struct contining the address of the symbol.
    symbol       -  The HSYM for lpaddr.
    lpszProcName -  Buffer that receives the function name.

Return Value:

    None.

--*/

{
    if (symbol) {
        SHGetSymName( symbol, lpszProcName );
    } else {
        sprintf( lpszProcName, "0x%016I64X", lpaddr->addr.off );
    }
}


VOID
GetFunctionParameters(
    LPADDR       lpaddr,
    HSYM         symbol,
    DWORD        FrameNum,
    LPSTR        lpszParams,
    CXF          Cxf
    )

/*++

Routine Description:

    This function formats a string that represents the parameters to
    the function specified by symbol.

    THIS FUNCTION SHOULD ONLY BE CALLED BY BuildStackInfo!

Arguments:

    lpaddr       -  An ADDR struct contining the address of the symbol.

    symbol       -  The HSYM for lpaddr.

    FrameNum     -  Frame number (zero relative).

    lpszParams   -  Buffer that receives the parameters string.

    Cxf          -  Frame context.  This must have been properly initialized
                    for the current stack frame.

Return Value:

    None.

--*/

{
    ADDR        addr;
    CXT         cxt;
    HTM         htm;
    DWORD       strIndex;
    DWORD       cParm = 0;
    SHFLAG      shflag = FALSE;
    LPSTR       p;
    DWORD       i;
    HTM         htmParm;
    EEHSTR      hName;
    LPSTR       lpch;
    HFRAME      hframe;

    if (symbol == 0) {
        return;
    }

    addr = *lpaddr;
    ZeroMemory( &cxt, sizeof ( CXT ) );
    SHSetCxt( &addr, &cxt );

    if (EEGetTMFromHSYM( symbol, &cxt, &htm, &strIndex, TRUE, FALSE ) != EENOERROR) {
        goto exit;
    }

    if (EEcParamTM( &htm, &cParm, &shflag ) != EENOERROR) {
        goto exit;
    }

    p = lpszParams;
    hframe = SHhFrameFrompCXF( &Cxf );

    for ( i = 0; i < cParm; i++ ) {
        if (EEGetParmTM( &htm, (EERADIX) i, &htmParm, &strIndex, FALSE ) == EENOERROR ) {
            EEvaluateTM( &htmParm, hframe, EEVERTICAL );
            if (EEGetValueFromTM( &htmParm, radix, (PEEFORMAT)"p", &hName ) == EENOERROR ) {
                lpch = (PSTR) MMLpvLockMb ( hName );
                memcpy ( p, lpch, strlen(lpch) );
                p += strlen(lpch);
                MMbUnlockMb ( hName );
                EEFreeStr ( hName );
                *p++ = ',';
                *p++ = ' ';
            }
            EEFreeTM (&htmParm);
        }
    }

    if (shflag) {
        memcpy( p, "...", 3 );
        p += 3;
    }

    while (( *( p - 1 ) == ' ' ) || ( *( p - 1 ) == ',' ) ) {
        p--;
    }
    *p = '\0';

exit:
    EEFreeTM ( &htm );
}

VOID
BuildStackInfo(
    LPSTACKINFO    si,
    LPSTACKFRAME   stkFrame,
    int            frameNum,
    HPID           hpid,
    HTID           vhtid,
    BOOL           fFull
    )

/*++

Routine Description:

    This function fills in the STACKINFO structure with all of the
    necessary information for a stack trace display.

    THIS FUNCTION MUST BE CALLED WITH A VALID VHTID FROM OSDGetFrame.
    Only the most recent call to OSDGetFrame is valid.

Arguments:

    si          - pointer to a STACKINFO structure

    stkFrame

    frameNum

    hpid        - valid HPID. Supplies process.

    vhtid

    fFull

Return Value:

    None.

--*/

{
    HSYM        symbol;
    ADDR        addr;
    DWORD       clt;


    ZeroMemory( si, sizeof(STACKINFO));

    si->StkFrame = *stkFrame;
    si->FrameNum = frameNum;

    //
    // load symbols
    //
    LoadSymbols( &si->StkFrame.AddrPC );
    LoadSymbols( &si->StkFrame.AddrReturn );

    //
    // fixup the address
    //
    CreateAddress( &si->StkFrame.AddrPC, &addr );

    GetSymbolFromAddr( &addr, &symbol, &clt, &si->fInProlog );
    if (fFull) {
        GetContextString( &addr, symbol, clt, si->Context );
        GetFunctionName( &addr, symbol, si->ProcName );
        GetModuleName( &addr, si->Module );
        //
        // HACK: kcarlos - If SAPI did not load any symbols for this module
        //  then we use the module list to get the module name.
        //
        if (!*si->Module) {
            //
            // If SAPI did not load any symbols, the module name will be
            // blank. Use the EM's module list to get the module name.
            //
            OSDGetModuleNameFromAddress(hpid,
                                        addr.addr.off,
                                        si->Module,
                                        sizeof(si->Module)
                                        );
        }
    }
    GetDisplacement( &addr, symbol, si->StkFrame.AddrPC, &si->Displacement, &si->ProcAddr );
    GetFrameInfo( &addr, si->StkFrame.AddrFrame, &si->Cxf, vhtid );
    if (fFull) {
        GetFunctionParameters( &addr, symbol, si->FrameNum, si->Params, si->Cxf );
    }
}


BOOL
GotoFrame(
    int         iCall,
    BOOL        bUserActivated
    )

/*++

Routine Description:

    This function positions either the source window or the
    disassembly window to the address referenced by the
    STACKINFO structure that is indexed by iCall.

Arguments:

    iCall       - index of the desired STACKINFO structure
    bUserActivated - Indicates whether this action was initiated by the
                user or by windbg. The value is to determine the Z order of
                any windows that are opened.

Return Value:

    None.

--*/

{
    PCXF cxf = ChangeFrame(iCall);

    if (!cxf) {
        return FALSE;
    }

    if (!MoveEditorToAddr(SHpADDRFrompCXT(SHpCXTFrompCXF(cxf)), bUserActivated)) {
        if (disasmView == -1) {
            OpenDebugWindow(DISASM_WIN, bUserActivated);
        }
        ActivateMDIChild(Views[disasmView].hwndFrame, bUserActivated);
    }

    if (disasmView != -1) {
        ViewDisasm(SHpADDRFrompCXT(SHpCXTFrompCXF(cxf)), disasmForce);
    }

    if ( GetLocalHWND() != 0) {
        SendMessage(GetLocalHWND(), WU_UPDATE, (WPARAM)(LPSTR)cxf, 0L);
    }

    return TRUE;
}


void
OpenCallsWindow(
    int       type,
    LPWININFO lpWinInfo,
    int       Preference,
    BOOL      bUserActivated
    )

/*++

Routine Description:

    This function opend a calls window.

Arguments:

    type        - window type (calls window)

    lpWinInfo   - initial window position

    Preference  - view index

    bUserActivated - Indicates whether this action was initiated by the
                user or by windbg. The value is to determine the Z order of
                any windows that are opened.

Return Value:

    None.

--*/

{
    WORD  classId;
    WORD  winTitle;
    HWND  hWnd;
    int   view;

    MDICREATESTRUCT mcs;
    char  szClass[MAX_MSG_TXT];
    char  title[MAX_MSG_TXT];
    char  final[MAX_MSG_TXT+4];

    ZeroMemory(&mcs, sizeof(mcs));

    classId = SYS_Calls_wClass;
    winTitle = SYS_CallsWin_Title;
    hWnd = GetCallsHWND();

    if (hWnd) {
        if (IsIconic(hWnd)) {
            OpenIcon(hWnd);
        }

        ActivateMDIChild(hWnd, bUserActivated);
        return;
    }

    //
    //  Determine which view index we are going to use
    //
    if ( (Preference != -1) && (Views[Preference].Doc == -1) ) {
        view = Preference;
    }

    else {
        for (view=0; (view<MAX_VIEWS)&&(Views[view].Doc!=-1); view++);
    }

    if (view == MAX_VIEWS) {
        ErrorBox(ERR_Too_Many_Opened_Views);
        return;
    }

    // Get the Window Title and Window Class

    Dbg(LoadString(g_hInst, classId,  szClass, MAX_MSG_TXT));
    Dbg(LoadString(g_hInst, winTitle, title, MAX_MSG_TXT));
    RemoveMnemonic(title,title);
    sprintf(final,"%s", title);

    // Make sure the Menu gets setup
    AddWindowMenuItem(-type, view);

    // Have MDI Client create the Child

    mcs.szTitle = final;
    mcs.szClass = szClass;
    mcs.hOwner  = g_hInst;
    if (lpWinInfo) {
        mcs.x = lpWinInfo->coord.left;
        mcs.y = lpWinInfo->coord.top;
        mcs.cx = lpWinInfo->coord.right - lpWinInfo->coord.left;
        mcs.cy = lpWinInfo->coord.bottom - lpWinInfo->coord.top;
        mcs.style = lpWinInfo->style;
    } else {
        mcs.x = mcs.cx = CW_USEDEFAULT;
        mcs.y = mcs.cy = CW_USEDEFAULT;
        mcs.style = 0;
    }

    mcs.style |= WS_VISIBLE;
    if (hwndActive && ( IsZoomed(hwndActive) || IsZoomed(GetParent(hwndActive)) ) ) {
        mcs.style |= WS_MAXIMIZE;
    }

    mcs.lParam  = (ULONG) (type | (view << 16));

    hWnd = (HWND) SendMessage(g_hwndMDIClient, WM_MDICREATE, 0, (LPARAM) &mcs);

    SetWindowWord(hWnd, GWW_VIEW, (WORD)view);

    Views[view].hwndFrame = hWnd;
    Views[view].hwndClient = 0;
    Views[view].NextView = -1;  /* No next view */
    Views[view].Doc = -type;
}

#if defined( NEW_WINDOWING_CODE )
HWND
CreateWindow_NewCalls(
    HWND hwndParent
    )
/*++
Routine Description:

  Create the command window.

Arguments:

    hwndParent - The parent window to the command window. In an MDI document,
        this is usually the handle to the MDI client window: g_hwndMDIClient

Return Value:

    If successful, creates a valid window handle to the new command window.

    NULL if the window was not created.

--*/
{
    char szClassName[MAX_MSG_TXT];
    char szWinTitle[MAX_MSG_TXT];
    CREATESTRUCT cs;
    HWND hwnd;

    // get class name
    Dbg(LoadString(g_hInst, SYS_NewCalls_wClass, szClassName, sizeof(szClassName)));

    // Get the title
    {
        char sz[MAX_MSG_TXT];

        Dbg(LoadString(g_hInst, SYS_CmdWin_Title, sz, sizeof(sz)));

        RemoveMnemonic(sz, szWinTitle);
    }

    ZeroMemory(&cs, sizeof(cs));

    return CreateWindowEx(
        WS_EX_MDICHILD | WS_EX_CONTROLPARENT,       // Extended style
        szClassName,                                // class name
        szWinTitle,                                 // title
        WS_CLIPCHILDREN | WS_CLIPSIBLINGS
        | WS_OVERLAPPEDWINDOW | WS_VISIBLE,         // style
        CW_USEDEFAULT,                              // x
        CW_USEDEFAULT,                              // y
        CW_USEDEFAULT,                              // width
        CW_USEDEFAULT,                              // height
        hwndParent,                                 // parent
        NULL,                                       // menu
        g_hInst,                                    // hInstance
        NULL                                        // user defined data
        );
}

#else //NEW_WINDOWING_CODE

LONG
CreateCallsWindow(
    HWND        hwnd,
    WPARAM      wParam,
    LPARAM      lParam
    )
/*++

Routine Description:

    This function creates a calls window and fills the window
    with a current stack trace.

Arguments:

    hwnd        - calls window handle
    wParam      - font handle
    lParam      - pointer to a CREATESTRUCT

Return Value:

    Zero.

--*/
{
    MDICREATESTRUCT *mdi;
    int             iView;
    int             iType;
    RECT            cRect;
    DWORD           dwStyle;


    mdi = (MDICREATESTRUCT*)(((CREATESTRUCT*)lParam)->lpCreateParams);
    myView = iView = HIWORD(mdi->lParam);
    iType = (WORD)(mdi->lParam & 0xffff);
    hWndCalls = hwnd;
    GetClientRect( hwnd, &cRect );

    dwStyle = WS_CHILD             |
              WS_VISIBLE           |
              LBS_NOTIFY           |
              LBS_NOINTEGRALHEIGHT |
              LBS_WANTKEYBOARDINPUT;

    if (g_contGlobalPreferences_WkSp.m_bVertScrollBars) {
        dwStyle |= WS_VSCROLL;
    }

    if (g_contGlobalPreferences_WkSp.m_bHorzScrollBars) {
        dwStyle |= WS_HSCROLL;
    }

    hwndList = CreateWindow( "LISTBOX",
                  NULL,
                  dwStyle,
                  cRect.left,
                  cRect.top,
                  cRect.right  - cRect.left,
                  cRect.bottom - cRect.top,
                  hwnd,
                  NULL,
                  GetModuleHandle(NULL),
                  NULL
                );

    hFontList = Views[myView].font;
    if (!hFontList) {
        Views[myView].font = hFontList = CreateFontIndirect( &g_logfontDefault );
    }
    SendMessage( hwndList, WM_SETFONT, (WPARAM)hFontList, (LPARAM)FALSE );
    hbrBackground = CreateSolidBrush( StringColors[CallsWindow].BkgndColor );

    FillStackFrameWindow( hwndList, TRUE );

    return 0;
}
#endif //NEW_WINDOWING_CODE


LOGERR NEAR PASCAL
LogCallStack(
    LPSTR lpsz
    )
/*++

Routine Description:

    This function will dump a call stack to the command window.

Arguments:

    lpsz - arguments to callstack

Return Value:

    log error code

--*/
{
    LPSTACKINFO StackInfo = NULL;
    BOOL        bSpecial = FALSE;
    BOOL        bColumnTitles = FALSE;
    LPPD        LppdT = LppdCur;
    LPTD        LptdT = LptdCur;
    LOGERR      rVal = LOGERROR_NOERROR;
    DWORD       i;
    DWORD       k = MAX_FRAMES;
    int         err;
    int         cch;
    XOSD        xosd = xosdNone;
    CHAR        buf[512];
    CHAR        errmsg[512];
    DWORD64     StackAddr = 0;
    DWORD64     FrameAddr = 0;
    DWORD64     PCAddr    = 0;
    LPSTR       p;
    DWORD       dwFrames = g_contGlobalPreferences_WkSp.m_dwMaxFrames;



    CmdInsertInit();
    IsKdCmdAllowed();

    PDWildInvalid();

    PreRunInvalid();

    if (LptdCommand == (LPTD)-1) {
        if (LppdCur == NULL) {
            return LOGERROR_UNKNOWN;
        }

        for (LptdCommand = LppdCur->lptdList;
             LptdCommand && rVal == LOGERROR_NOERROR;
             LptdCommand = LptdCommand->lptdNext) {

            ThreadStatForThread(LptdCommand);
            rVal = LogCallStack(lpsz);
            if (LptdCommand->lptdNext) {
                CmdLogFmt("\n");
            }

        }
        return rVal;
    }

    LppdCur = LppdCommand;
    LptdCur = LptdCommand;

    i = sizeof(STACKINFO) * dwFrames;
    StackInfo = (LPSTACKINFO) malloc( i );
    if (!StackInfo) {
        CmdLogFmt( "Could not allocate memory for stack trace\n" );
        goto done;
    }
    ZeroMemory( StackInfo,  i );

    //
    // this code processes the 'K' command modifiers
    //
    //     b - print first 3 parameters from stack
    //     s - print source information (file & lineno)
    //     v - runtime function information (fpo/pdata)
    //     t - display column titles
    //
    lpsz = CPSkipWhitespace(lpsz);
    while (lpsz && *lpsz && *lpsz != ' ') {
        switch (tolower(*lpsz)) {
            case 'b':
                g_contGlobalPreferences_WkSp.m_bFrameptr     = TRUE;
                g_contGlobalPreferences_WkSp.m_bRetAddr      = TRUE;
                g_contGlobalPreferences_WkSp.m_bDisplacement = TRUE;
                g_contGlobalPreferences_WkSp.m_bStack        = TRUE;
                g_contGlobalPreferences_WkSp.m_bModule       = TRUE;
                bSpecial             = TRUE;
                bColumnTitles        = TRUE;
                break;

            case 's':
                g_contGlobalPreferences_WkSp.m_bSource       = TRUE;
                break;

            case 'v':
                g_contGlobalPreferences_WkSp.m_bRtf          = TRUE;
                break;

            case 'n':
                g_contGlobalPreferences_WkSp.m_bFrameNum     = TRUE;
                break;

            case 't':
                bColumnTitles        = TRUE;
                break;

            case ' ':
                break;

            default:
                goto nextparse;
        }
        ++lpsz;
    }

nextparse:
    lpsz = CPSkipWhitespace(lpsz);

    if (*lpsz == '=') {
        lpsz++;

        p = CPSzToken(&lpsz, NULL);
        if (!p || !*p) {
            CmdLogFmt( "Missing stack frame\r\n" );
            goto done;
        }
        FrameAddr = CPGetNbr( p,
                              radix,
                              TRUE,
                              &CxfIp,
                              errmsg,
                              &err,
                              g_contWorkspace_WkSp.m_bMasmEval
                              );
        if (err) {
            CmdLogFmt( "Bad frame address\r\n" );
            goto done;
        }

        p = CPSzToken(&lpsz, NULL);
        if (!p || !*p) {
            CmdLogFmt( "Missing stack address\r\n" );
            goto done;
        }
        StackAddr = CPGetNbr( p,
                              radix,
                              TRUE,
                              &CxfIp,
                              errmsg,
                              &err,
                              g_contWorkspace_WkSp.m_bMasmEval
                              );
        if (err) {
            CmdLogFmt( "Bad stack address\r\n" );
            goto done;
        }

        p = CPSzToken(&lpsz, NULL);
        if (!p || !*p) {
            CmdLogFmt( "Missing PC\r\n" );
            goto done;
        }
        PCAddr = CPGetNbr( p,
                           radix,
                           TRUE,
                           &CxfIp,
                           errmsg,
                           &err,
                           g_contWorkspace_WkSp.m_bMasmEval
                           );
        if (err) {
            CmdLogFmt( "Bad PC address\r\n" );
            goto done;
        }
    }

    lpsz = CPSkipWhitespace(lpsz);
    if (lpsz && *lpsz) {
        if (*lpsz == 'l' || *lpsz == 'L') {
            lpsz = CPSkipWhitespace(lpsz+1);
        }
        if (!*lpsz) {
            err = 1;
        } else {
            k = CPGetInt(lpsz, &err, &cch);
        }
        if (err || k < 1) {
            CmdLogVar(ERR_Bad_Count);
            rVal = LOGERROR_QUIET;
            goto done;
        }
    }

    if (!DebuggeeActive()) {
        CmdLogVar(ERR_Debuggee_Not_Alive);
        rVal = LOGERROR_QUIET;
        goto done;
    }

    GetCompleteStackTrace( FrameAddr, StackAddr, PCAddr, StackInfo, &dwFrames, FALSE, TRUE );

    k = min( k, dwFrames );

    for (i=0; i<k; i++) {

        if (i==0) {
            if (bColumnTitles) {
                TCHAR szFmt[200] = {0};
                PTSTR pszFmt = szFmt;
                PCTSTR pszFrmPtr = _T("FramePtr");
                PCTSTR pszRetAdr = _T("RetAddr");
                PCTSTR pszParam1  = _T("Param1");
                PCTSTR pszParam2  = _T("Param2");
                PCTSTR pszParam3  = _T("Param3");
                int nPtrWidth = 0;
                int nParamWidth = 0;

                switch( StackInfo[i].StkFrame.AddrPC.Mode ) {
                default:
                    case AddrModeFlat:
                        nPtrWidth = 16;
                        nParamWidth = 8;
                        break;
                    case AddrModeReal:
                    case AddrMode1616:
                        nPtrWidth = 9;
                        nParamWidth = 4;
                        break;

                    case AddrMode1632:
                        nPtrWidth = 13;
                        nParamWidth = 4;
                        break;
                }

                if (g_contGlobalPreferences_WkSp.m_bFrameNum){
                    _tcscpy(pszFmt, "#   ");
                    pszFmt += (_tcslen(pszFmt) / sizeof(TCHAR));
                }

                if (bSpecial) {
                    // Display Params
                    _stprintf(pszFmt, "%-*s  %-*s  %-*s %-*s %-*s Function Name\r\n",
                        nPtrWidth, pszFrmPtr,
                        nPtrWidth, pszRetAdr,
                        nParamWidth, pszParam1,
                        nParamWidth, pszParam2,
                        nParamWidth, pszParam3);
                } else {
                    _stprintf(pszFmt, "%-*s  %-*s  Function Name\r\n",
                        nPtrWidth, pszFrmPtr,
                        nPtrWidth, pszRetAdr);
                }

                CmdLogFmt( szFmt );
            }
        }

        FormatStackFrameString( buf, sizeof(buf), &StackInfo[i], 0 );

        CmdLogFmt( buf );
        CmdLogFmt( "\r\n" );
    }

done:
    if (StackInfo) {
        free( StackInfo );
    }
    LppdCur = LppdT;
    LptdCur = LptdT;
    return rVal;
}


INT
LookupFrameAddress(
    LPSTACKINFO     lpsi,
    DWORD           frames,
    ADDR            addr
    )

/*++

Routine Description:

    Locates an address in the callback stack area.

Arguments:

    addr       -  An ADDR struct contining the address to find in the calls stack

Return Value:

    Index of the located stack frame or -1 on error.

--*/

{
    int i;
    ADDR addrPC;

    SYUnFixupAddr( &addr );

    for (i=0; i<(INT)frames; i++) {
        CreateAddress( &lpsi[i].StkFrame.AddrPC, &addrPC );
        SYUnFixupAddr( &addrPC );
        if ((GetAddrSeg(lpsi[i].ProcAddr) == GetAddrSeg(addr)) &&
            ((GetAddrOff(addr) >= GetAddrOff(lpsi[i].ProcAddr)) &&
             (GetAddrOff(addr) <  lpsi[i].StkFrame.AddrPC.Offset))) {
            return i;
        }
    }

    return -1;
}


PCXF
CLGetFuncCXF(
    PADDR paddr,
    PCXF  pcxf
    )

/*++

Routine Description:

    This function is used to resolve a context operator by filling in the
    frame structure in the CXF passed in.  Because this function may be called
    while in the middle of an expression evaluation care must be taken to NEVER
    call the EE while in this function.  The EE is not re-entrant!

Arguments:

    paddr      -  An ADDR struct contining the address of the evaluation
    pcxf       -  Pointer to a CXF struture

Return Value:

    Always return pcxf.

--*/

{
    LPSTACKINFO StackInfo;
    DWORD       dwFrames = MAX_FRAMES;
    INT         frame;
    ADDR        addr;


    Assert ( ADDR_IS_LI (*paddr));
    addr = *paddr;
    ZeroMemory( pcxf, sizeof ( CXF ) );

    frame = sizeof(STACKINFO) * dwFrames;
    StackInfo = (LPSTACKINFO) malloc( frame );
    if (!StackInfo) {
        goto exit;
    }
    ZeroMemory( StackInfo,  frame );

    GetCompleteStackTrace( 0, 0, 0, StackInfo, &dwFrames, FALSE, FALSE );

    if (dwFrames == 0) {
        goto exit;
    }

    frame = LookupFrameAddress( StackInfo, MAX_FRAMES, addr );

    if (frame == -1) {
        goto exit;
    }

    CreateAddress( &StackInfo[frame].StkFrame.AddrPC, &addr );

    SHSetCxt( &addr, &pcxf->cxt );

    pcxf->hFrame = StackInfo[frame].Cxf.hFrame;
    pcxf->cxt.addr = addr;

exit:
    if (StackInfo) {
        free( StackInfo );
    }

    return pcxf;
}




void
FormatStackFrameString(
    LPSTR             lsz,
    DWORD             cb,
    const STACKINFO * const si,
    DWORD             width
    )

/*++

Routine Description:

    This function formats a string that reresents a stack frame and places
    the data in the buffer pointed to by lsz.  The current options control
    the format of the data.

Arguments:

    nView       - view number for calls window

    lsz         - Pointer to a buffer that receives the formatted string.

    cb          - length of the lsz buffer.

    si          - Stack frame that is to be formatted.

    width       - If non-zero this value indicates that the source information
                  is to be right justified in the buffer.

Return Value:

    None.

--*/

{
    LPSTR       beg = lsz;
    DWORD       i;
    ADDR        addr;
    CHAR        SrcFname[MAX_PATH];
    DWORD       lineno;
    CHAR        szFname[_MAX_FNAME];
    CHAR        szExt[_MAX_EXT];
    UINT        processor;
    PFPO_DATA   pFpoData;


    *lsz = '\0';

    if (g_contGlobalPreferences_WkSp.m_bFrameNum) {
        if (radix == 10) {
            sprintf( lsz, "%02d  ", si->FrameNum );
        } else if (radix == 16) {
            sprintf( lsz, "%02x  ", si->FrameNum );
        } else if (radix == 8) {
            sprintf( lsz, "%02o  ", si->FrameNum );
        }
        lsz += strlen( lsz );
    }

    switch( si->StkFrame.AddrReturn.Mode ) {
    case AddrModeFlat:
        if (g_contGlobalPreferences_WkSp.m_bFrameptr) {
            sprintf( lsz, "%016I64x  ", si->StkFrame.AddrFrame.Offset );
            lsz += strlen( lsz );
        }
        if (g_contGlobalPreferences_WkSp.m_bRetAddr) {
            sprintf( lsz, "%016I64x ", si->StkFrame.AddrReturn.Offset );
            lsz += strlen( lsz );
        }
        break;

    case AddrMode1616:
    case AddrModeReal:
        if (g_contGlobalPreferences_WkSp.m_bFrameptr) {
            sprintf( lsz, "%04x:%04x ",
                     si->StkFrame.AddrFrame.Segment,
                     (DWORD)si->StkFrame.AddrFrame.Offset
                   );
            lsz += strlen( lsz );
        }
        if (g_contGlobalPreferences_WkSp.m_bRetAddr) {
            sprintf( lsz, "%04x:%04x ",
                     si->StkFrame.AddrReturn.Segment,
                     (DWORD)si->StkFrame.AddrReturn.Offset
                   );
            lsz += strlen( lsz );
        }
        break;

    case AddrMode1632:
        if (g_contGlobalPreferences_WkSp.m_bFrameptr) {
            sprintf( lsz, "%04x:%08x ",
                     si->StkFrame.AddrFrame.Segment,
                     (DWORD)si->StkFrame.AddrFrame.Offset
                   );
            lsz += strlen( lsz );
        }
        if (g_contGlobalPreferences_WkSp.m_bRetAddr) {
            sprintf( lsz, "%04x:%08x ",
                     si->StkFrame.AddrReturn.Segment,
                     (DWORD)si->StkFrame.AddrReturn.Offset
                   );
            lsz += strlen( lsz );
        }
        break;
    }

    if (g_contGlobalPreferences_WkSp.m_bRetAddr || g_contGlobalPreferences_WkSp.m_bFrameptr) {
        strcat( lsz, " " );
        lsz += strlen( lsz );
    }

    if (g_contGlobalPreferences_WkSp.m_bStack) {
        if (si->StkFrame.AddrReturn.Mode == AddrModeFlat) {
            sprintf( lsz, 
                     sizeof(si->StkFrame.Params[0] == 8) 
                            ? "%016I64x %016I64x %016I64x ":"%08x %08x %08x ",
                     si->StkFrame.Params[0],
                     si->StkFrame.Params[1],
                     si->StkFrame.Params[2]
                   );
        } else {
            sprintf( lsz, 
                     "%04x   %04x   %04x   ",
                     LOWORD(si->StkFrame.Params[0]),
                     HIWORD(si->StkFrame.Params[0]),
                     LOWORD(si->StkFrame.Params[1])
                   );
        }
        lsz += strlen( lsz );
    }

    if (g_contGlobalPreferences_WkSp.m_bModule) {
        if (si->Context[0]) {
            strcat( lsz, si->Context );
            lsz += strlen( lsz );
        } else if (*si->Module) {
            strcat( lsz, si->Module );
            strcat( lsz, "!" );
            lsz += strlen( lsz );
        }
    }

    if (g_contGlobalPreferences_WkSp.m_bFuncName) {
        if (si->ProcName[0] == '_') {
            char *p;
            strcat( lsz, &si->ProcName[1] );
            p = (PSTR) strchr( (LPBYTE) lsz, '@');
            if (p) {
                *p = '\0';
            }
            lsz += strlen( lsz );
        } else {
            strcat( lsz, si->ProcName );
            lsz += strlen( lsz );
        }
    }

    if (g_contGlobalPreferences_WkSp.m_bDisplacement && si->Displacement) {
        sprintf( lsz, "+0x%I64x", si->Displacement );
        lsz += strlen( lsz );
    }

    if (g_contGlobalPreferences_WkSp.m_bParams && si->Params[0]) {
        sprintf( lsz, "(%s)", si->Params );
        lsz += strlen( lsz );
    }

    if (g_contGlobalPreferences_WkSp.m_bRtf) {
        OSDGetDebugMetric ( LppdCur->hpid, 0, mtrcProcessorType, &processor );
        if (processor == mptix86) {
            pFpoData = (PFPO_DATA) si->StkFrame.FuncTableEntry;
            if (!pFpoData) {
                sprintf(lsz, " (No FPO)");
            } else {
                switch (pFpoData->cbFrame) {
                    case FRAME_NONFPO:
                        sprintf(lsz, " (EBP)");
                        break;

                    case FRAME_FPO:
                        if (pFpoData->fHasSEH) {
                            sprintf(lsz, " (FPO: [seh] ");
                        } else if (pFpoData->fUseBP) {
                            sprintf(lsz, " (FPO: [ebp %08x] ", SAVE_EBP(si->StkFrame));
                        } else {
                            sprintf(lsz, " (FPO: ");
                        }
                        lsz += strlen( lsz );
                        sprintf(lsz, "[%d,%d,%d])", pFpoData->cdwParams,
                                                    pFpoData->cdwLocals,
                                                    pFpoData->cbRegs);
                        break;

                    case FRAME_TRAP:
                        {
                        sprintf(lsz, " (FPO: [%d,%d] TrapFrame%s @ %08lx)",
                            pFpoData->cdwParams,
                            pFpoData->cdwLocals,
                            (TRAP_EDITED(si->StkFrame)) ? "" : "-EDITED",
                            SAVE_TRAP(si->StkFrame) );
                        }
                        break;

                    case FRAME_TSS:
                        sprintf(lsz, " (FPO: TaskGate %lx:0)", TRAP_TSS(si->StkFrame));
                        break;

                    default:
                        sprintf(lsz, "(UKNOWN FPO TYPE)");
                        break;
                }
            }
            lsz += strlen(lsz);
        }
    }

    if (g_contGlobalPreferences_WkSp.m_bSource) {
        ZeroMemory( &addr, sizeof(addr) );
        addr.addr.off     = (OFFSET)si->StkFrame.AddrPC.Offset;
        addr.addr.seg     = si->StkFrame.AddrPC.Segment;
        addr.mode.fFlat   = si->StkFrame.AddrPC.Mode == AddrModeFlat;
        addr.mode.fOff32  = si->StkFrame.AddrPC.Mode == AddrMode1632;
        addr.mode.fReal   = si->StkFrame.AddrPC.Mode == AddrModeReal;
        if (GetSourceFromAddress( &addr, SrcFname, sizeof(SrcFname), &lineno )) {
            _splitpath( SrcFname, NULL, NULL, szFname, szExt );
            if (width) {

                //------------------------------------------------------
                // this code right justifies the source info string
                //------------------------------------------------------

                //
                // put the string in a safe place
                //
                sprintf( SrcFname, " [ %s%s @ %4lu ]", szFname, szExt, lineno );

                if (width > (lsz-beg) + strlen(SrcFname)) {

                    //
                    // pad the string with spaces
                    //
                    for (i=0; i<width-(lsz-beg)+1; i++) {
                        *(lsz+i) = ' ';
                    }

                    //
                    // re-position the pointer to the end
                    //
                    lsz = beg + width - strlen(SrcFname);

                }

                //
                // spew the string to it's new home
                //
                sprintf( lsz, "%s", SrcFname );

            } else {

                sprintf( lsz, " [ %s%s @ %lu ]", szFname, szExt, lineno );

            }

            lsz += strlen( lsz );
        }
    }

    return;
}


BOOL
GoUntilStackFrame(
    LPSTACKINFO si
    )

/*++

Routine Description:

    This function sets a breakpoint at the address indicated in the requested
    stack frame and then continues the thread.

Arguments:

    si          - Stack frame that is to control the bp.

Return Value:

    TRUE        - Thread was continued to the requested address.
    FALSE       - Thread could not be continued or bp could not be set.

--*/

{
    ADDR    addr;
    HBPT    hbpt = NULL;
    CHAR    buf[256];
    CXF     cxf = CxfIp;

    sprintf( buf, "0x%I64x", si->StkFrame.AddrPC.Offset );

    if (BPParse(&hbpt, buf, NULL, NULL, LppdCur ? LppdCur->hpid: 0) != BPNOERROR) {
        return FALSE;
    }

    if (BPBindHbpt( hbpt, &cxf ) == BPNOERROR) {
        if (BPAddrFromHbpt( hbpt, &addr ) != BPNOERROR) {
            return FALSE;
        }
        if (BPFreeHbpt( hbpt )  != BPNOERROR) {
            return FALSE;
        }
        if (!GoUntil(&addr)) {
            return FALSE;
        }
        UpdateDebuggerState(UPDATE_CONTEXT);
        return TRUE;
    }

    BPFreeHbpt( hbpt );
    return FALSE;
}


BOOL
GetCompleteStackTrace(
    DWORD64       FramePointer,
    DWORD64       StackPointer,
    DWORD64       ProgramCounter,
    LPSTACKINFO   StackInfo,
    LPDWORD       lpdwFrames,
    BOOL          fQuick,
    BOOL          fFull
    )

/*++

Routine Description:

    This function calls OSDEBUG to get the individual stack frames.
    The STACKINFO structure is filled in with the information returned from
    OSDEBUG.

Arguments:

    FramePointer     - If non-zero this is the beginning frame pointer.

    StackPointer     - If non-zero this is the beginning stack pointer .

    ProgramCounter   - If non-zero this is the beginning program counter.

    StackInfo        - Pointer to an array of STACKINFO structures.

    lpdwFrames       - Number of frames to get from OSDEBUG.

    fQuick           - If TRUE and the frame pointer is the same as the last
                     call to this function the only the first frame is updated.

    fFull            - If TRUE all fields in the frame are filled in

Return Value:

    TRUE             - Stack trace completed ok.

    FALSE            - Could not obtain a stack trace.

--*/

{
    DWORD       i;
    STACKFRAME  stkFrame;
    XOSD        xosd;
    DWORD       numFrames = *lpdwFrames;
    DWORD       dw;
    HTID        vhtid;
    HIND        hSavedRegs;
    ADDR        addr;

    *lpdwFrames = 0;

    if ((!LppdCur) || (!LptdCur)) {
        return FALSE;
    }

    hSavedRegs = NULL;
    if (FramePointer || StackPointer || ProgramCounter) {
        OSDSaveRegs(LppdCur->hpid, LptdCur->htid, &hSavedRegs);
    }

    if (FramePointer) {
        AddrInit(&addr,
                 NULL,
                 0,
                 FramePointer,
                 TRUE,
                 TRUE,
                 FALSE,
                 FALSE
                 );

        OSDSetAddr(LppdCur->hpid, LptdCur->htid, adrBase, &addr);
    }

    if (StackPointer) {
        AddrInit(&addr,
                 NULL,
                 0,
                 StackPointer,
                 TRUE,
                 TRUE,
                 FALSE,
                 FALSE
                 );

        OSDSetAddr(LppdCur->hpid, LptdCur->htid, adrStack, &addr);
    }

    if (ProgramCounter) {
        AddrInit(&addr,
                 NULL,
                 0,
                 ProgramCounter,
                 TRUE,
                 TRUE,
                 FALSE,
                 FALSE
                 );

        OSDSetAddr(LppdCur->hpid, LptdCur->htid, adrPC, &addr);
    }

    ZeroMemory( &stkFrame, sizeof(stkFrame) );

    vhtid = LptdCur->htid;

    for (i=0; i<numFrames; i++) {

        xosd = OSDGetFrame(LppdCur->hpid, vhtid, 1, &vhtid);
        if (xosd != xosdNone) {
            break;
        }
        xosd = OSDSystemService (
            LppdCur->hpid,
            LptdCur->htid,
            (SSVC) ssvcGetStackFrame,
            &stkFrame,
            sizeof(stkFrame),
            &dw
            );

        if (xosd != xosdNone) {
            break;
        }


        BuildStackInfo(&StackInfo[i], 
                       &stkFrame, 
                       i, 
                       LppdCur->hpid,
                       vhtid, 
                       fFull
                       );

        if ( fQuick && i== 0 ) {
            //
            //  If the first frame has the same frame and return
            //  addresses as the previously cached one, then we
            //  don't bother getting the rest of the stack trace since
            //  it has not changed.
            //
            if ( ADDREQ( stkFrame.AddrFrame,  stkFrameSave.AddrFrame  ) &&
                 ADDREQ( stkFrame.AddrReturn, stkFrameSave.AddrReturn ) &&
                 ADDREQ( stkFrame.AddrStack, stkFrameSave.AddrStack ) ) {
                i = FrameCountSave;
                break;
            }
            stkFrameSave = stkFrame;

        } else if (i) {

            //
            //  If the current frame is the same as the previous frame,
            //  we stop so we don't end up with a long list of bogus
            //  frames.
            //
            if ( ADDREQ( stkFrame.AddrPC,    StackInfo[i-1].StkFrame.AddrPC    ) &&
                 ADDREQ( stkFrame.AddrFrame, StackInfo[i-1].StkFrame.AddrFrame ) ) {
                break;
            }
        }
    }

    *lpdwFrames = i;
    if (fQuick) {
        // Just in case cached frame count changes.
        FrameCountSave = i;
    }

    if (hSavedRegs) {
        OSDRestoreRegs(LppdCur->hpid, LptdCur->htid, hSavedRegs);
    }

    return TRUE;
}

LPSTR
GetLastFrameFuncName(
    VOID
    )

/*++

Routine Description:

    This function gets the symbol name of the last stack frame.  What this
    really accomplishes is getting the name of the function that was
    passed to CreateThread().  If the last symbol name is BaseThreadStart() then
    the previous frame's symbol name is used.

Arguments:

    None.

Return Value:

    Pointer to a string that contains the function name.  The caller is
    responsible for free()ing the memory.

--*/

{
    LPSTR       fname = NULL;
    LPSTACKINFO StackInfo = NULL;
    DWORD       dwFrames = MAX_FRAMES;
    DWORD       i;


    i = sizeof(STACKINFO) * dwFrames;
    StackInfo = (LPSTACKINFO) malloc( i );
    if (!StackInfo) {
        goto exit;
    }
    ZeroMemory( StackInfo,  i );

    GetCompleteStackTrace( 0, 0, 0, StackInfo, &dwFrames, FALSE, TRUE );

    if (dwFrames == 0) {
        goto exit;
    }

    if (_stricmp(StackInfo[dwFrames-1].ProcName,"BaseThreadStart")==0 ||
        _stricmp(StackInfo[dwFrames-1].ProcName,"_BaseThreadStart@8")==0) {
        if (dwFrames > 1) {
            fname = _strdup(StackInfo[dwFrames-2].ProcName);
            goto exit;
        }
    }

    fname = _strdup(StackInfo[dwFrames-1].ProcName);

exit:
    if (StackInfo) {
        free( StackInfo );
    }
    return fname;
}

BOOL
IsValidFrameNumber(
    INT FrameNumber
    )
{
    DWORD i;

    //
    // update the call stack
    //
    i = g_contGlobalPreferences_WkSp.m_dwMaxFrames;
    if (GetCompleteStackTrace( 0, 0, 0, StackInfo, &i, TRUE, TRUE )) {
        FrameCount = i;
    }

    //
    // update the window if one exists
    //
    if (GetCallsHWND() && hwndList) {
        FillStackFrameWindow( hwndList, FALSE );
    }

    if (FrameNumber >= 0 && FrameNumber <= (INT)FrameCount) {
        return TRUE;
    }

    return FALSE;
}




//
//  M00BUG -- this function should not re ally be needed -- but it is until
//      KentF manages to fix what HFRAMEs really do.
//

HTID
WalkToFrame(
    HPID hpid,
    HTID htid,
    int iCall
    )
{
    int i;
    HTID vhtid;
    XOSD xosd;

    if (!LppdCur || !LptdCur) {
        return NULL;
    }

    vhtid = LptdCur->htid;

    if (iCall > 0) {
        xosd = OSDGetFrame(LppdCur->hpid, LptdCur->htid, iCall+1, &vhtid);
        if (xosd != xosdNone) {
            return NULL;
        }
    }
    return vhtid;
}

PCXF
ChangeFrame(
    int iCall
    )
{
    PCXF cxf;
    HTID vhtid;

    if (!LppdCur || !LptdCur) {
        return NULL;
    } else if (iCall == 0) {
        cxf = &CxfIp;
    } else {
        cxf = &StackInfo[iCall].Cxf;
        vhtid = WalkToFrame(LppdCur->hpid, LptdCur->htid, iCall);
        if (!vhtid) {
            return NULL;
        }
        cxf->hFrame = (HFRAME) vhtid;
    }
    return cxf;
}


INT_PTR
CALLBACK
DlgProc_CallStack(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static DWORD HelpArray[]=
    {
        ID_CWOPT_FRAMEPTR, IDH_FP,
        ID_CWOPT_FUNCNAME, IDH_FUNC,
        ID_CWOPT_PARAMS, IDH_PARAM,
        ID_CWOPT_MODULE, IDH_MODULE,
        ID_CWOPT_STACK, IDH_4DWORDS,
        ID_CWOPT_RETADDR, IDH_RETADDR,
        ID_CWOPT_DISPLACEMENT, IDH_DISPLACE,
        ID_CWOPT_SOURCE, IDH_SRC,
        ID_CWOPT_RTF, IDH_RUNTIME,
        ID_CWOPT_MAXFRAMES_LABEL, IDH_MAXFRAMES,
        ID_CWOPT_MAXFRAMES, IDH_MAXFRAMES,
        0, 0
    };

    switch (uMsg) {
    case WM_INITDIALOG:
        SendDlgItemMessage(hwndDlg, ID_CWOPT_MAXFRAMES, EM_SETLIMITTEXT, 3, 0);

        CheckDlgButton( hwndDlg, ID_CWOPT_FRAMEPTR,      g_contGlobalPreferences_WkSp.m_bFrameptr     );
        CheckDlgButton( hwndDlg, ID_CWOPT_RETADDR,       g_contGlobalPreferences_WkSp.m_bRetAddr      );
        CheckDlgButton( hwndDlg, ID_CWOPT_FUNCNAME,      g_contGlobalPreferences_WkSp.m_bFuncName     );
        CheckDlgButton( hwndDlg, ID_CWOPT_DISPLACEMENT,  g_contGlobalPreferences_WkSp.m_bDisplacement );
        CheckDlgButton( hwndDlg, ID_CWOPT_PARAMS,        g_contGlobalPreferences_WkSp.m_bParams       );
        CheckDlgButton( hwndDlg, ID_CWOPT_STACK,         g_contGlobalPreferences_WkSp.m_bStack        );
        CheckDlgButton( hwndDlg, ID_CWOPT_SOURCE,        g_contGlobalPreferences_WkSp.m_bSource       );
        CheckDlgButton( hwndDlg, ID_CWOPT_MODULE,        g_contGlobalPreferences_WkSp.m_bModule       );
        CheckDlgButton( hwndDlg, ID_CWOPT_RTF,           g_contGlobalPreferences_WkSp.m_bRtf          );
        SetDlgItemInt ( hwndDlg, ID_CWOPT_MAXFRAMES,     g_contGlobalPreferences_WkSp.m_dwMaxFrames, 500 );
        return FALSE;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, "windbg.hlp", HELP_WM_HELP,
            (DWORD_PTR)(PVOID) HelpArray );
        return TRUE;

    case WM_CONTEXTMENU:
        WinHelp ((HWND) wParam, "windbg.hlp", HELP_CONTEXTMENU,
            (DWORD_PTR)(PVOID) HelpArray );
        return TRUE;

    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code) {
        case PSN_APPLY:
            {
                BOOL bSuccess;

                g_contGlobalPreferences_WkSp.m_bFrameptr     = IsDlgButtonChecked( hwndDlg, ID_CWOPT_FRAMEPTR     );
                g_contGlobalPreferences_WkSp.m_bRetAddr      = IsDlgButtonChecked( hwndDlg, ID_CWOPT_RETADDR      );
                g_contGlobalPreferences_WkSp.m_bFuncName     = IsDlgButtonChecked( hwndDlg, ID_CWOPT_FUNCNAME     );
                g_contGlobalPreferences_WkSp.m_bDisplacement = IsDlgButtonChecked( hwndDlg, ID_CWOPT_DISPLACEMENT );
                g_contGlobalPreferences_WkSp.m_bParams       = IsDlgButtonChecked( hwndDlg, ID_CWOPT_PARAMS       );
                g_contGlobalPreferences_WkSp.m_bStack        = IsDlgButtonChecked( hwndDlg, ID_CWOPT_STACK        );
                g_contGlobalPreferences_WkSp.m_bSource       = IsDlgButtonChecked( hwndDlg, ID_CWOPT_SOURCE       );
                g_contGlobalPreferences_WkSp.m_bModule       = IsDlgButtonChecked( hwndDlg, ID_CWOPT_MODULE       );
                g_contGlobalPreferences_WkSp.m_bRtf          = IsDlgButtonChecked( hwndDlg, ID_CWOPT_RTF          );
                g_contGlobalPreferences_WkSp.m_dwMaxFrames    = GetDlgItemInt( hwndDlg, ID_CWOPT_MAXFRAMES, &bSuccess, 500 );
                UpdateDebuggerState( UPDATE_CALLS );
            }
            return TRUE;
        }
        break;
    }

    return FALSE;
}

