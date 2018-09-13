/*++

Copyright (c) 1992  Microsoft Corporation

Author:

    Griffith Kadnier (v-griffk)

Environment:

    Win32 - User

Notes:

--*/


#include "precomp.h"
#pragma hdrstop

#define strpbrk _mbspbrk

#include "include\cntxthlp.h"

/************************** Data declaration    *************************/

extern  CXF     CxfIp;
extern  SHF FAR *Lpshf;


typedef struct _FUNCLIST {
    PSTR    Str;
    ADDR    Addr;
} FUNCLIST, *PFUNCLIST;


PFUNCLIST   FuncList;
int         FuncListSize;
BOOL        FuncFound;
ADDR        FuncAddr;

INT_PTR
PASCAL
DlgFuncResolve(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

/**************************       Code          *************************/

/***    LocateFunction
**
**  Synopsis:
**      bool = LocateFunction(szStr, pCXTOut)
**
**  Entry:
**      szStr   - string containning the address to goto
**      pCXTOut - pointer to context in which to place the address
**
**  Returns:
**      TRUE if the address was successfuly found and FALSE otherwise
**
**  Description:
**
*/

BOOL NEAR PASCAL
LocateFunction(
    PSTR szStr,
    PADDR pAddr
    )
{
    TML         TMLT;
    TML         TMLT2;
    DWORD       bpsegtype = EECODE;
    DWORD       stringindex;
    PHTM        TMList;
    PHTM        pTMList;
    PHTM        pTMList2;
    int         i;

    EESTATUS    eeErr;
    PSTR        p;
    PSTR        Function;
    PSTR        Module = NULL;
    char        Buffer[ 256 ];
    BOOL        Ok      = FALSE;
    BOOL        Found   = FALSE;
    HEXE        hexe    = (HEXE) NULL;
    PSTR        Str;



    FuncList     = NULL;
    FuncListSize = 0;

    eeErr = EEParseBP(szStr, radix, fCaseSensitive, &CxfIp, &TMLT, 0,
                    &stringindex, FALSE);

    if (eeErr == EEGENERAL) {

        /*
         * Kill current error message
         */

        EEFreeTML(&TMLT);


        //
        //  If no module was specified, try all modules.
        //

        strcpy( Buffer, szStr );

        if ( p = (PSTR) strchr( (PUCHAR) Buffer, ',' ) ) {

            if ( p = (PSTR) strpbrk( (PUCHAR) p+1, (PUCHAR) ",}" ) ) {

                if ( *p == '}' ) {

                    *p++    = ',';
                    *p      = '\0';
                    Ok      = TRUE;

                } else {

                  if (!IsDBCSLeadByte(*p)) {
                    p++;
                    while ( *p == ' ' || *p == '\t' ) {
                        p++;
                    }

                    if ( *p == '}' ) {
                        *p  = '\0';
                        Ok  = TRUE;
                    }
                  }
                }

                if ( Ok ) {
                    Function = szStr + (p - Buffer);
                    Module   = p;
                }
            }

        } else {

            strcpy( Buffer, "{,," );
            Module   = Buffer + strlen( Buffer );
            Function = szStr;
            Ok       = TRUE;
        }

        if ( Ok ) {
            Str = NULL;
            TMLT.hTMList = (HDEP) NULL;
            pTMList = NULL;

            while ((( hexe = SHGetNextExe( hexe ) ) != 0) ) {
                *Module = '\0';
                strcat( Buffer, SHGetExeName(hexe));
                strcat( Buffer, "}" );
                strcat( Buffer, Function );

                eeErr = EEParseBP(Buffer, radix, fCaseSensitive, &CxfIp,
                                  &TMLT2, 0, &stringindex, FALSE);

                if ( eeErr == EENOERROR ) {
                    if ( !pTMList ) {

                        TMLT = TMLT2;
                        pTMList = (HTM FAR *) MMLpvLockMb( TMLT.hTMList );

                    } else {

                        pTMList2 = (HTM FAR *) MMLpvLockMb( TMLT2.hTMList );

                        for (i=0; i< (int) TMLT2.cTMListAct; i++) {
                            DAssert( TMLT2.cTMListAct < TMLT.cTMListMax );
                            pTMList[ TMLT.cTMListAct++ ] = pTMList2[ i ];
                            pTMList2[i] = (HDEP) NULL;
                        }

                        MMbUnlockMb( TMLT2.hTMList );

                        TMLT2.cTMListAct = 0;
                        EEFreeTML(&TMLT2);
                    }
                }
            }

            if (pTMList) {
                MMbUnlockMb( TMLT.hTMList );
                eeErr = EENOERROR;
            } else {
                eeErr = EEGENERAL;
            }
        }
    }

    if (eeErr == EENOERROR) {
        TMList = (PHTM) MMLpvLockMb( TMLT.hTMList );

        while (TMLT.cTMListAct > 1) {
            eeErr = (EESTATUS) BPTResolve( szStr, &TMLT, &CxfIp, FALSE);

            if (eeErr != EENOERROR) {
                break;
            }

            if (TMLT.cTMListAct > 1) {
                ErrorBox(0);
            }
        }

        if (TMLT.cTMListAct == 1) {
            if (!(eeErr = EEvaluateTM( &TMList[0], SHhFrameFrompCXF(&CxfIp),
                                    EEBPADDRESS))) {
                eeErr = BPADDRFromTM( &TMList[0], &bpsegtype, pAddr);
            }
        }
    }

    return eeErr == EENOERROR;
}                               /* LocateFunction() */


INT_PTR
DlgFuncResolve(
    HWND hDlg,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    int Index;

    static DWORD HelpArray[]=
    {
       ID_FUNCRES_LABEL2, IDH_FMT,
       ID_FUNCRES_LIST, IDH_EXPAND,
       ID_FUNCRES_USE, IDH_NOTEXPAND,
       0, 0
    };

    Unreferenced( lParam );

    switch( msg ) {

        case WM_INITDIALOG:

            FuncFound = FALSE;

            for ( Index = 0; Index < FuncListSize; Index++ ) {
                SendMessage( GetDlgItem(hDlg, ID_FUNCRES_LIST ),
                           LB_ADDSTRING, 0, (LPARAM)(LPSTR)FuncList[Index].Str );
            }
            break;

        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, "windbg.hlp", HELP_WM_HELP,
               (DWORD_PTR)(LPVOID) HelpArray );
            return TRUE;

        case WM_CONTEXTMENU:
            WinHelp ((HWND) wParam, "windbg.hlp", HELP_CONTEXTMENU,
               (DWORD_PTR)(LPVOID) HelpArray );
            return TRUE;

        case WM_COMMAND:

            switch( LOWORD( wParam ) ) {

                case ID_FUNCRES_USE:
                    Index = (int) SendMessage( GetDlgItem(hDlg, ID_FUNCRES_LIST ),
                                         LB_GETCURSEL, 0, 0L );
                    FuncAddr  = FuncList[Index].Addr;
                    FuncFound = TRUE;
                    EndDialog(hDlg, TRUE);
                    return TRUE;


                case IDCANCEL:
                    FuncFound = FALSE;
                    EndDialog(hDlg, TRUE);
                    return TRUE;

            }
            break;
    }
    return FALSE;
}



/***    DlgFunction
**
**  Synopsis:
**      bool = DlgFunction(hDlg, message, wParam, lParam)
**
**  Entry:
**      hDlg    - Handle to dialog box
**      message - Message to be processed
**      wParam  - info about current message
**      lParam  - info about current message
**
**  Returns:
**      TRUE if we handled the message and FALSE otherwise
**
**  Description:
**      This function is the dialog proc for the View.Address menu item.
**      It will get an address from the user and will change the the requested
**      source window and move the cursor to that line.
*/

INT_PTR
CALLBACK
DlgFunction(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    char        szFuncName[128];
    HWND        hFuncName;
    ADDR        addr;

    static DWORD HelpArray[]=
    {
       ID_FUNCTION_FUNCNAME, IDH_ADDR,
       0, 0
    };

    Unreferenced( lParam );

    switch (message) {

      case WM_INITDIALOG:
        /*
        **   Initialize the edit item for the address field
        */

        Dbg((hFuncName = GetDlgItem(hDlg, ID_FUNCTION_FUNCNAME))!=NULL);
        Dbg(SendMessage(hFuncName, EM_LIMITTEXT, sizeof(szFuncName)-1, 0L));

        /*
        **      Initialise entry field with current selection
        */

        *szFuncName = '\0';

        if (Views[curView].Doc > -1) {
            if ((hwndActiveEdit != NULL) &&
                               (Docs[Views[curView].Doc].docType == DOC_WIN)) {
                BOOL lookAround = TRUE;

                GetCurrentText(curView, &lookAround, (LPSTR)szFuncName,
                        min(MAX_USER_LINE, sizeof(szFuncName)-1),
                        NULL, NULL);
            }
        }

        /*
        **      Send the text and select it
        */

        SendMessage(hFuncName, WM_SETTEXT, 0, (LPARAM)(LPSTR)szFuncName);
#ifdef WIN32
        SendMessage(hFuncName, EM_SETSEL, 0, (LPARAM) -1);
#else
        SendMessage(hFuncName, EM_SETSEL, 0, MAKELONG(0, 0x7FFF));
#endif

        SetFocus(hFuncName);
        return (FALSE);

      case WM_HELP:
          WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, "windbg.hlp", HELP_WM_HELP,
             (DWORD_PTR)(LPVOID) HelpArray );
          return TRUE;

      case WM_CONTEXTMENU:
          WinHelp ((HWND) wParam, "windbg.hlp", HELP_CONTEXTMENU,
             (DWORD_PTR)(LPVOID) HelpArray );
          return TRUE;

      case WM_COMMAND:
        switch (wParam) {
            /*
            **  Get the address string and attempt to change to the
            **  desired address
            */

          case IDOK :
            Dbg((hFuncName = GetDlgItem(hDlg, ID_FUNCTION_FUNCNAME))!=NULL);
            GetDlgItemText(hDlg, ID_FUNCTION_FUNCNAME,
                  (LPSTR)szFuncName, sizeof(szFuncName));

            if (!LocateFunction(szFuncName, &addr)) {

                ErrorBox2(hDlg, MB_TASKMODAL, ERR_Function_Locate);
                SendMessage(hFuncName, EM_SETSEL, 0, (DWORD) -1);
                SetFocus(hFuncName);

            } else {

                if (!MoveEditorToAddr(&addr, TRUE)) {
                    if (disasmView == -1) {
                        OpenDebugWindow(DISASM_WIN, TRUE); // User activated
                    }
                }

                if (disasmView != -1) {
                    OpenDebugWindow(DISASM_WIN, TRUE); // User activated
                    ViewDisasm(&addr, disasmForce);
                }

                EndDialog(hDlg, TRUE);
            }

            return TRUE;

          case IDCANCEL :
            EndDialog(hDlg, FALSE);
            return (TRUE);

        }

        break;
    }

    return (FALSE);
}                                       /* DlgFunction() */
