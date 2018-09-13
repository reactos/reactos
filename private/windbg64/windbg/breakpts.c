/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    breakpts.c

Abstract:

    This file contains the shell portion of the breakpoint handler (except
    for the UI portion).

Author:

    Jim Schaad (jimsch)

Environment:

    Win32 - User

--*/

#include "precomp.h"
#pragma hdrstop

extern  CXF     CxfIp;
extern  LPSHF   Lpshf;

#include "include\cntxthlp.h"

INT_PTR
WINAPI
DlgBpResolve(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR
WINAPI
DlgBpCanIUseThunk(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

LPSTR    BpString;
TML     *TmlBpResolve;
PCXF     PcxfBpResolve;
BPSTATUS RetBpResolve;
BOOL     BpDlgCalled;
BOOL     ResolvingBp;
BOOL     RetCanIUseThunk;

/***    CheckExpression
**
**  Synopsis:
**      bool = CheckExpression(Expr, Radix, Case)
**
**  Entry:
**      Expr
**      Radix
**      Case
**
**  Returns:
**      TRUE if expression parses successfully, FALSE otherwise.
**
**  Description:
**      Takes an expression in string form and validates
**      it by running it through the EE.
**
*/

BOOL
CheckExpression(
    LPSTR Expr,
    EERADIX Radix,
    int Case
    )
{
    HTM hTM = (HTM)NULL;
    EESTATUS status;
    ULONG end;

    status = EEParse(Expr, Radix, Case, &hTM, &end);
    if (hTM)
    {
        EEFreeTM (&hTM);
    }

    return (status == EENOERROR);
}                                       /* CheckExpression() */

/***    DlgUnresolved
**
**  Synopsis:
**      bool = DlgUnresolved( hDlg, msg, wParam, lParam)
**
**  Entry:
**      hDlg    - handle to the dialog box
**      msg     - message to be processed
**      wParam  - info about the message
**      lParam  - info about the message
**
**  Return:
**      TRUE if we handled the message and FALSE otherwise
**
**  Description:
**      This is the dialog proc for the unresolved breakpoints dialog.  It
**      is used to inform the user that the breakpoint was not set for
**      some reason and should have been.
*/

static  HBPT    HbptUnresolved;

INT_PTR
WINAPI
DlgUnresolved(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    char        rgchT[255];
    DWORD       dw;
    
    static DWORD HelpArray[]=
    {
        ID_UNRESOLVED_CLEAR, IDH_UNRESOLVED_CLEAR,
            ID_UNRESOLVED_DISABLE, IDH_UNRESOLVED_DISABLE,    
            ID_UNRESOLVED_DEFER, IDH_UNRESOLVED_DEFER,
            ID_UNRESOLVED_QUIET, IDH_UNRESOLVED_QUIET,
            ID_UNRESOLVED_BREAKPOINTS, IDH_UNRESOLVED_BREAKPOINTS,
            0, 0
    };
    
    Unreferenced( lParam );
    
    switch( msg ) {
    case WM_INITDIALOG:
        Dbg( BPIFromHbpt(&dw, HbptUnresolved)  == BPNOERROR );
        sprintf(rgchT, "Breakpoint #%lu", dw);

        SendMessage(GetDlgItem(hDlg, ID_UNRESOLVED_HBPT),
                    WM_SETTEXT,
                    0,
                    (LPARAM)rgchT);
        Dbg(BPFormatHbpt(HbptUnresolved, rgchT, sizeof(rgchT), 0) == BPNOERROR);
        SendMessage(GetDlgItem(hDlg, ID_UNRESOLVED_DESC),
                    WM_SETTEXT,
                    0,
                    (LPARAM)rgchT);
        SetFocus(GetDlgItem(hDlg, ID_UNRESOLVED_DEFER));
        return FALSE;
        
    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle,
                "windbg.hlp",
                HELP_WM_HELP,
                (DWORD_PTR) HelpArray );
        return TRUE;
        
    case WM_CONTEXTMENU:
        WinHelp ((HWND) wParam,
                 "windbg.hlp",
                 HELP_CONTEXTMENU,
                 (DWORD_PTR) HelpArray );
        return TRUE;
        
    case WM_COMMAND:
        switch( LOWORD( wParam ) ) {
        case ID_UNRESOLVED_DEFER:
            EndDialog( hDlg, TRUE );
            return TRUE;
            
        case ID_UNRESOLVED_CLEAR:
            Dbg( BPDelete( HbptUnresolved ) == BPNOERROR);
            EndDialog( hDlg, TRUE );
            return TRUE;
            
        case ID_UNRESOLVED_QUIET:
            BPSetQuiet( HbptUnresolved );
            EndDialog(hDlg, TRUE);
            break;
            
        case ID_UNRESOLVED_DISABLE:
            Dbg( BPDisable( HbptUnresolved ) == BPNOERROR);
            EndDialog(hDlg, TRUE);
            return TRUE;
            
        case ID_UNRESOLVED_BREAKPOINTS:
            StartDialog(DLG_BREAKPOINTS, DlgSetBreak);
            BpDlgCalled = TRUE;
            EndDialog(hDlg, TRUE);
            return TRUE;
        }
        break;
    }
    return FALSE;
}                                       /* DlgUnresolved() */



VOID PASCAL
BPTResolveAll(
    HPID hpid,
    BOOL fSearchAll
    )
/*++

Routine Description:


Arguments:
    hpid - hpid of the process we are trying to resolve the breakpoints
        for.

        Breakpoints are resolved using the currently loaded symbols. If the
        breakpoint cannot be resolved using the currently loaded symbols; the
        debugger may attempt to load symbols for the remaining modules 
        attempting to resolve the breakpoint with each new symbol loaded.

    fSearchAll
        TRUE - If the breakpoint could not be resolved using the symbols
                currently loaded. Start loading the symbols for the rest
                of the modules.
        FALSE - If the breakpoint could not be resolved, just give up.


Return Value:

    none

--*/
{
    HBPT        hBpt;
    BPSTATUS    bpstat;
    DWORD       bpIpid;
    UINT        Ipid = 0;
    CXF         cxf = CxfIp;
    LPPD        lppd;
    HEXE        hexe;

    if ( hpid && (lppd = LppdOfHpid(hpid))) {
        Ipid = lppd->ipid;
    }

    BpDlgCalled = TRUE;

    while ( BpDlgCalled ) {

        BpDlgCalled = FALSE;

        hBpt = 0;
        Dbg(BPNextHbpt( &hBpt, bptNext) == BPNOERROR);

        while (hBpt != NULL) {

            //
            //  If unresolved breakpoint, try to resolve it
            //
            if ( !BPIsInstantiated( hBpt ) &&
                 !BPIsDisabled( hBpt ) ) {

                BPGetIpid( hBpt, &bpIpid );

                if ( bpIpid == Ipid )  {

                    if ( BPSymbolsMayBeAvailable( hBpt ) ) {

                        bpstat = BPBindHbpt( hBpt, &cxf );

                        if ( bpstat == BPCancel ) {
                            hBpt = NULL;
                            continue;
                        }

                        if (bpstat != BPNOERROR && fSearchAll) {
                            hexe = NULL;

                            while (bpstat != BPNOERROR &&
                                                (hexe = SHGetNextExe(hexe))) {

                                SHWantSymbols( hexe );
                                memset(&cxf, 0, sizeof(cxf));
                                SHGetCxtFromHexe(hexe, &cxf.cxt);
                                bpstat = BPBindHbpt( hBpt, &cxf );
                            }
                        }

                        if ( (bpstat != BPNOERROR) &&
                                    !BPIsQuiet( hBpt )    &&
                                    !AutoTest ) {
                            HbptUnresolved = hBpt;
                            Dbg(BPNextHbpt( &hBpt, bptNext) == BPNOERROR);
                            EnsureFocusDebugger();
                            StartDialog(DLG_UNRESOLVED, DlgUnresolved);
                            if ( BpDlgCalled ) {
                                break;
                            }
                            continue;
                        }
                    }
                }
            }

            Dbg(BPNextHbpt( &hBpt, bptNext) == BPNOERROR);
        }
    }
    BPCommit();
    return;
}                                       /* BPTResolveAll() */

DWORD PASCAL
BPTIsUnresolvedCount(
    HPID hpid
    )
{
    DWORD       bpCount = 0;
    HBPT        hBpt;
    UINT        Ipid = 0;
    CXF         cxf = CxfIp;
    LPPD        lppd;
    BOOL        fGotInitialContext = FALSE;

    if ( hpid && (lppd = LppdOfHpid(hpid))) {
        Ipid = lppd->ipid;
    }

    BpDlgCalled = TRUE;

    while ( BpDlgCalled ) {

        BpDlgCalled = FALSE;

        hBpt = 0;
        Dbg(BPNextHbpt( &hBpt, bptNext) == BPNOERROR);

        while (hBpt != NULL) {

            if ( !BPIsInstantiated( hBpt ) &&
                 !BPIsDisabled( hBpt ) ) {

                ++bpCount;
            }

            Dbg(BPNextHbpt( &hBpt, bptNext) == BPNOERROR);
        }
    }

    return bpCount;
}



/***    BPTUnResolve
**
**  Synopsis:
**      void = BPTUnResolve( hexe )
**
**  Entry:
**      hexe - module whose breakpoints are beint unresolved
**
**  Returns:
**      Nothing
**
**  Description:
**
*/
VOID
BPTUnResolve(
    HEXE hexe
    )
{
    HBPT        hBpt = 0;
    ADDR        Addr;


    Dbg(BPNextHbpt( &hBpt, bptNext) == BPNOERROR);

    //
    //  Unresolve all breakpoints in this module.
    //
    while (hBpt != NULL) {

        if ( BPIsInstantiated( hBpt ) ) {

            BPAddrFromHbpt( hBpt, &Addr );

            if ( hexe == (HEXE)emiAddr( Addr ) ) {
                BPUninstantiate( hBpt );
            }
        }

        Dbg(BPNextHbpt( &hBpt, bptNext) == BPNOERROR);
    }

    return;
}

VOID
BPTUnResolvePidTid(
    HPID hpid,
    HTID htid
    )
{
    HBPT    hBpt = 0;
    HPID    hPidB;
    HTID    hTidB;

    Dbg(BPNextHbpt( &hBpt, bptNext) == BPNOERROR);

    //
    //  Unresolve all breakpoints.
    //
    while (hBpt != NULL) {

        BPGetHpid(hBpt, &hPidB);
        BPGetHtid(hBpt, &hTidB);

        if ( (hpid == hPidB) && (htid == hTidB) ) {
            BPUninstantiate( hBpt );
        }
        Dbg(BPNextHbpt(&hBpt, bptNext) == BPNOERROR);
    }

    return;
}


/***    BPTUnResolveAll
**
**  Synopsis:
**      void = BPTUnResolveAll(hpid)
**
**  Entry:
**      hpid or NULL
**
**  Returns:
**      Nothing
**
**  Description:
**
*/
VOID
BPTUnResolveAll(
    HPID hpid
    )
{
    HBPT    hBpt = 0;
    HPID    hPidB;

    Dbg(BPNextHbpt( &hBpt, bptNext) == BPNOERROR);

    //
    //  Unresolve all breakpoints.
    //
    while (hBpt != NULL) {

        if (!hpid || (BPGetHpid(hBpt, &hPidB), hPidB) == hpid) {
            BPUninstantiate( hBpt );
        }
        Dbg(BPNextHbpt(&hBpt, bptNext) == BPNOERROR);

    }

    return;
}




BOOL
BPCBBindHbpt(
    HBPT hbpt
    )
{
    Unreferenced( hbpt);
    DebugBreak();
    return FALSE;
}                                       /* BPCBBindHbpt() */


/***    BPCBCBSetHighlight
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**
*/

BOOL PASCAL
BPCBSetHighlight(
    char FAR * szFile,
    DWORD line,
    BOOL fSet,
    BOOL fAsmOnly,
    WORD state
    )
{
    int         doc;
    char        rgch[255];
    int         GotNext;

    if (disasmView != -1) {
        ViewDisasm(NULL, disasmHighlight);
    }

    if ( !fAsmOnly ) {
        Assert( sizeof(rgch)-1 > strlen(szFile) );
        strcpy(rgch, szFile);

        GotNext = SrcMapSourceFilename(rgch, sizeof(rgch), SRC_MAP_ONLY, NULL,
            FALSE); // Not user activated

        if ((GotNext == 1) && FindDoc( rgch, &doc, TRUE) ) {
            LineStatus( doc, line, state,
                    fSet ? LINESTATUS_ON : LINESTATUS_OFF, FALSE, TRUE);
            return TRUE;
        }
    }
    return FALSE;
}                                       /* BPCBSetHighlight() */


int
PASCAL
BPTResolve(
    LPSTR BpBuf,
    PVOID tml,
    PCXF  pcxf,
    BOOL  fBp
    )
{
    BpString        = BpBuf;
    TmlBpResolve    = (TML *)tml;
    PcxfBpResolve   = pcxf;

    ResolvingBp = fBp;

    if ( AutoTest ) {
        //
        //  In test mode we use everything.
        //
        RetBpResolve = BPNOERROR;

    } else {

        StartDialog( DLG_BPRESOLVE, DlgBpResolve );
    }

    return RetBpResolve;
}


INT_PTR
WINAPI
DlgBpResolve(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{

    HDC         hdc;
    int         i;
    int         j;
    int         Idx;
    char *      pch;
    int         LargestString = 0;
    SIZE        Size;
    HWND        hList;
    PHTM        pTmList;
    EESTATUS    Ret;
    ADDR        Addr;
    DWORD       bpsegtype = EECODE;
    CXT         Cxt;
    char        rgch[512];
    EEHSTR      eehstr;
    EEHSTR      eehstr2;
    DWORD       cb;
    EERADIX     localradix;

    static DWORD HelpArray[]=
    {
        ID_BPRESOLVE_LIST, IDH_BPRESOLVE_LIST,
        ID_BPRESOLVE_USE, IDH_BPRESOLVE_USE,
        0, 0
    };

    Unreferenced( lParam );

    switch( msg ) {

      case WM_INITDIALOG:

        if ( BpString ) {

            char *Buffer = rgch;
            char *p;
            char c;

            *Buffer = '\0';

            if ( *BpString != '{' || !(p = (PSTR) strchr( (LPBYTE) BpString+1, '}' )) ) {
                p = BpString;
            } else {
                c = *++p;
                *p = '\0';
                BPShortenContext( BpString, Buffer );
                *p = c;
            }

            strcat(Buffer, p );

            SendMessage(GetDlgItem(hDlg, ID_BPRESOLVE_STRING), WM_SETTEXT,
                        0, (LPARAM) (LPSTR) Buffer);
        }

        LoadString(
                g_hInst,
                ResolvingBp ? DLG_ResolveBpCaption: DLG_ResolveFuncCaption,
                rgch,
                sizeof(rgch) );

        SetWindowText( hDlg, rgch );

        hList = GetDlgItem(hDlg, ID_BPRESOLVE_LIST);
        Assert(hList);

        pTmList = (PHTM)MMLpvLockMb( TmlBpResolve->hTMList );

        for (i = 0; i < (int)TmlBpResolve->cTMListAct; i++ ) {

            Ret = EEvaluateTM(&pTmList[i],
                              SHhFrameFrompCXF(PcxfBpResolve),
                              EEBPADDRESS);

            if (Ret == EENOERROR) {
                Ret = BPADDRFromTM(&pTmList[i], &bpsegtype, &Addr );
            }

            if (Ret != EENOERROR) {
                continue;
            }

            memset( &Cxt, 0, sizeof(Cxt ));
            SHSetCxt( &Addr, &Cxt );

            Dbg( EEFormatCXTFromPCXT(&Cxt, &eehstr2, FALSE) == EENOERROR);

            *rgch = '\0';
            if ( eehstr2 ) {
                pch = (PSTR) MMLpvLockMb( eehstr2 );
                if ( *pch ) {
                    BPShortenContext( pch, rgch );
                }
                MMbUnlockMb( eehstr2 );
                EEFreeStr( eehstr2 );
            }

            eehstr = 0;
            Dbg(EEGetExprFromTM( &pTmList[i], &localradix, &eehstr, &cb) == EENOERROR);
            if (eehstr) {
                pch = (PSTR) MMLpvLockMb( eehstr );
                if ( pch && strchr( (PBYTE) pch,'{') ) {
                    char *p = (PSTR) strchr( (PBYTE) pch, '}' );
                    if ( p ) {
                        if ( *rgch == '\0' ) {
                            char c;
                            c      = *(p+1);
                            *(p+1) = '\0';
                            BPShortenContext( pch, rgch );
                            *(p+1) = 'c';
                        }

                        pch = p+1;
                    }
                }
                Assert(rgch);
                strcat(rgch, pch );

                hdc = GetDC( hList );
                GetTextExtentPoint(hdc, rgch, strlen(rgch), &Size );
                ReleaseDC( hList, hdc );

                if ( Size.cx > LargestString ) {

                    LargestString = Size.cx;

                    SendMessage(hList,
                                LB_SETHORIZONTALEXTENT,
                                (WPARAM)LargestString,
                                0 );
                }

                SendMessage( hList, LB_ADDSTRING, 0, (LPARAM) rgch);

                MMbUnlockMb( eehstr );
                EEFreeStr( eehstr );
            }

        }

        MMbUnlockMb( TmlBpResolve->hTMList );

        SendMessage(hList, LB_SETSEL, 1, 0L);

        return TRUE;


      case WM_HELP:
          WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle,
                  "windbg.hlp",
                  HELP_WM_HELP,
                  (DWORD_PTR) HelpArray );
          return TRUE;

      case WM_CONTEXTMENU:
          WinHelp ((HWND) wParam,
                   "windbg.hlp",
                   HELP_CONTEXTMENU,
                   (DWORD_PTR) HelpArray );
          return TRUE;

      case WM_COMMAND:

        switch( LOWORD( wParam ) ) {


           case ID_BPRESOLVE_LIST:
                switch (HIWORD(wParam)) {
                    case LBN_SELCHANGE:
                        if ( !ResolvingBp ) {
                            //
                            //  We only allow one selection.
                            //
                            hList   = GetDlgItem(hDlg, ID_BPRESOLVE_LIST);
                            Assert(hList);

                            j       = (int) SendMessage( hList,
                                                         LB_GETCOUNT,
                                                         0,
                                                         0 );
                            Idx     = (int) SendMessage( hList,
                                                         LB_GETCARETINDEX,
                                                         0,
                                                         0 );
                            for ( i=0; i < j; i++ ) {
                                if ( i != Idx && SendMessage( hList,
                                                              LB_GETSEL,
                                                              i,
                                                              0 ) ) {
                                    SendMessage( hList,
                                                 LB_SETSEL,
                                                 FALSE,
                                                 i );
                                }
                            }
                            if ( Idx != LB_ERR ) {
                                SendMessage( hList, LB_SETSEL, TRUE, Idx );
                            }
                        }
                }
                break;


          case ID_BPRESOLVE_USE:

            hList = GetDlgItem(hDlg, ID_BPRESOLVE_LIST);
            Assert(hList);

            pTmList = (PHTM)MMLpvLockMb( TmlBpResolve->hTMList );

            //
            //  Get rid of all the entries which are not selected
            //
            for (i=0, j=0; i < (int)TmlBpResolve->cTMListAct; i++ ) {

                if ( !SendMessage( hList, LB_GETSEL, i, 0L ) ) {

                    EEFreeTM( &(pTmList[i]) );
                    pTmList[i] = (HTM)NULL;

                } else {

                    if ( i != j ) {
                        pTmList[j] = pTmList[i];
                        pTmList[i] = (HTM)NULL;
                    }
                    j++;
                }
            }

            TmlBpResolve->cTMListAct = j;

            MMbUnlockMb( TmlBpResolve->hTMList );

            RetBpResolve = (j > 0) ? BPNOERROR : BPCancel;
            EndDialog(hDlg, TRUE);
            return TRUE;


          case IDCANCEL:

            RetBpResolve = BPCancel;
            EndDialog(hDlg, TRUE);
            return TRUE;

        }
        break;
    }
    return FALSE;
}


BOOL
BPTCanIUseThunk(
    LPSTR BpBuf
    )
{
    if ( AutoTest ) {
        return FALSE;
    }

    BpString = BpBuf;
    StartDialog( DLG_BPCANIUSETHUNK, DlgBpCanIUseThunk );
    return RetCanIUseThunk;
}


INT_PTR
WINAPI
DlgBpCanIUseThunk(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    char Buffer[256];
    char *p;
    char c;

    Unreferenced( lParam );

    switch( msg ) {

      case WM_INITDIALOG:
        if ( BpString ) {

            *Buffer = '\0';

            if ( *BpString == '{' ) {
                if (p  = (PSTR) strchr( (PBYTE) BpString+1, '}' ) ) {
                    p++;
                    c  = *p;
                    *p = '\0';
                    BPShortenContext( BpString, Buffer );
                    *p = c;
                } else {
                    p = BpString;
                }
            } else {
                p = BpString;
            }

            strcat(Buffer, p );

            SendMessage(GetDlgItem(hDlg, ID_BPCANIUSETHUNK_STRING), WM_SETTEXT,
                        0, (LPARAM)(LPSTR)Buffer);
        }

        break;

      case WM_COMMAND:

        switch( LOWORD( wParam ) ) {

          case IDOK:
            RetCanIUseThunk = TRUE;
            EndDialog(hDlg, TRUE);
            return TRUE;

          case IDCANCEL:
            RetCanIUseThunk = FALSE;
            EndDialog(hDlg, TRUE);
            return TRUE;

        }
        break;
    }

    return FALSE;
}
