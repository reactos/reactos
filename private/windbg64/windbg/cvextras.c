/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    cvextras.c

Abstract:

    This file contains some routines stolen from CodeView.  They should all
    be cleaned up and moved to apisupp.c or codemgr.c

Author:

    Jim Schaad (jimsch)

Environment:

    Win32 - User

--*/

#include "precomp.h"
#pragma hdrstop





#define MAXERRMSG       256

extern  LPSHF   Lpshf;




SHFLAG PASCAL
PHExactCmp (
    LPSSTR lpsstr,
    LPV lpv,
    LSZ lpb,
    SHFLAG fCase
    )
{
    unsigned char  cb;
    SHFLAG         shf = TRUE;

    Unreferenced( lpv );

    if ( lpb ) {
        cb = *lpb;

        // if length is diff, they are not equal
        if ( lpsstr && lpsstr->cb == cb ) {
            if ( fCase ) {
                shf = (SHFLAG) memcmp ( lpb + 1, lpsstr->lpName, cb );
            } else {
                shf = (SHFLAG) _strnicmp( lpb + 1, (LPSTR) lpsstr->lpName, cb );
            }
        }
    }
    return shf;
}


SHFLAG PASCAL
PHAtCmp(
    PVOID   pv1,
    LPV     lpv,
    LSZ     lpb,
    SHFLAG  fCase
    )
/*++

--*/
{
    LPSSTR lpsstr = (LPSSTR) pv1;
    UINT cbm;
    UINT cbt;
    UINT i;
    char *pm;

    // lpsstr is what we are looking for; we will
    // accept an exact match, or we will match an '@'
    // in lpb with our EOS.

    if (!lpsstr || !lpb) {
        return 1;
    }

    cbm = lpsstr->cb;
    cbt = *lpb++;

    pm = (PSTR) lpsstr->lpName;

    if (fCase) {
        for (i = 0; i < cbt; i++) {
            if (IsDBCSLeadByte(*lpb)) {
                if (*lpb++ != *pm++ || *lpb++ != *pm++) {
                    return 1;
                }
                i++;
                continue;
            }
            if (i == cbm && *lpb == '@') {
                return 0;
            }
            if (*lpb++ != *pm++) {
                return 1;
            }
        }
    } else {
        for (i = 0; i < cbt; i++) {
            if (IsDBCSLeadByte(*lpb)) {
                if (*lpb++ != *pm++ || *lpb++ != *pm++) {
                    return 1;
                }
                i++;
                continue;
            }
            if (i == cbm && *lpb == '@') {
                return 0;
            }
            if (toupper(*lpb++) != toupper(*pm++)) {
                return 1;
            }
        }
    }

    // i == cbt...
    if (i == cbm) {
        return 0;
    } else {
        return 1;
    }
}


//! LCLWN.H
typedef int lts;                //* error status number

//! LCLWN.C
LTS pascal FTError(LTS ltsCode)
{
        AuxPrintf(1, "FtError code: %d", (int)ltsCode);
        return ltsCode;
}

#ifdef WIN32
char in386mode = TRUE;
#else
char in386mode = FALSE;
#endif

/***    get_a_procedure
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

int
get_a_procedure(
    PCXT pCXT,
    char *szName
    )
{
    HSYM    hSym;
    HMOD    hMod;
    ADDR    addr = {0};
    HEXE    hexe = (HEXE) NULL;
    SSTR    sstr;
    BOOL    fFound = FALSE;

    sstr.cb         =   (BYTE) strlen(szName);
    sstr.lpName     =   (PUCHAR) szName;
    sstr.searchmask =   SSTR_NoHash;

    if( !pCXT  || !szName || !(*szName) ) {
        return FALSE;
    }

    hMod = SHHMODFrompCXT(pCXT);
    if (!hMod) {
        // no symbols
        return FALSE;
    }
    hexe = SHHexeFromHmod(hMod);
    DAssert(hexe);
    if (!hexe) {
        return FALSE;
    }
    hSym = PHFindNameInPublics (
            (HSYM) NULL,
            hexe,
            &sstr,
            TRUE,
            PHAtCmp);
    fFound = (hSym != NULL);

    if (!fFound) {
        return FALSE;
    } else {
        Dbg(SHAddrFromHsym( &addr, hSym));

        emiAddr( addr ) = (HEMI)hexe;
        ADDR_IS_LI( addr ) = TRUE;

        // v-vadimp - on Merced we are getting plabel - read the actual procedure entry point address from it
        if(LppdCur->mptProcessorType == mptia64) { 
            SYFixupAddr(&addr);
            DWORDLONG dwPLabel[2]; DWORD cb;
            OSDReadMemory(LppdCur->hpid, LptdCur->htid, &addr, &dwPLabel, sizeof(dwPLabel), &cb);
            GetAddrOff(addr) = dwPLabel[0]; 
            SYUnFixupAddr(&addr);
        }

        return (SHSetCxt ( &addr, pCXT ) != NULL);
    }
}                                       /* get_a_procedure() */

int
get_initial_context_helper(
    PCXT pCXT
    )
{
    // Look for main for a windows exe first

    // dotdot names are for the PPC
    if(get_a_procedure(pCXT,"..main")) return(TRUE);
    if(get_a_procedure(pCXT,"..wmain")) return(TRUE);
    if(get_a_procedure(pCXT,"..WinMain")) return(TRUE);

    if(get_a_procedure(pCXT,"_WinMain")) return(TRUE);
    if(get_a_procedure(pCXT,"WinMain")) return(TRUE);
    if(get_a_procedure(pCXT,"_wWinMain")) return(TRUE);
    if(get_a_procedure(pCXT,"wWinMain")) return(TRUE);
    if(get_a_procedure(pCXT,"WINMAIN")) return(TRUE);

    // Not there? Try for a Windows TTY

    if(get_a_procedure(pCXT,"_main")) return(TRUE);
    if(get_a_procedure(pCXT,"main")) return(TRUE);
    if(get_a_procedure(pCXT,"_wmain")) return(TRUE);
    if(get_a_procedure(pCXT,"wmain")) return(TRUE);
    if(get_a_procedure(pCXT,"MAIN")) return(TRUE);

    if(get_a_procedure(pCXT,"ENTGQQ")) return(TRUE);

    return(FALSE);
}                                       /* get_initial_context() */

/***    get_initial_context
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

int
get_initial_context(
    PCXT pCXT,
    BOOL fSearchAll
    )
{
    CXT Cxt;
    HEXE        hexe;
    HMOD        hMod;
    SHE         she;

    if ( !fSearchAll ) {

        return get_initial_context_helper( pCXT );

    } else {

        Cxt = *pCXT;

        for (hexe = (HEXE)NULL; hexe = SHGetNextExe ( hexe );  ) {

            SHSymbolsLoaded(hexe, &she);
            if ( she == sheDeferSyms ) {
                SHWantSymbols(hexe);
            }

            hMod = SHGetNextMod( hexe, (HMOD)NULL );
            if ( hMod ) {
                SHHMODFrompCXT( &Cxt ) = hMod;

                if ( get_initial_context_helper( &Cxt ) ) {
                    *pCXT = Cxt;
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}


void
CVMessage (
    MSGTYPE     msgtype,
    MSGID       msgid,
    MSGWHERE    msgwhere,
    ...
    )
{
    static char szCVErr[] = "Error";
    static char szCVWarn[] = "Warning";
    static char szCVMsg[] = "Message";
    static char szEmpty[] = "";
    static char szFormat[] = "CV%04u %s:  ";
    static char gszErrStr[ MAXERRMSG ] = {'\0'};
    static MSGID gMSGID = 0;

    LPSTR   szMsg;
    LPSTR   szStringLoc = NULL;
    LPSTR   szErr;
    char    rgch[ MAXERRMSG ];
    va_list va_mark;

    AuxPrintf(1, (LPSTR)"!!!CVMessage!!!");
    //
    // Set the beginning of the variable argument marker, and call vsprintf
    //  with that marker.  If no template vars in error_string (szMsg), this
    //  is just the same as using szMsg.
    //
    va_start (va_mark, msgwhere );

    // if we want it in a string, get the sting pointer
    if (msgwhere == MSGSTRING) {
        szStringLoc = va_arg(va_mark, LPSTR);
    }

    if (msgid == GEXPRERR) {
        msgid = gMSGID;
        strcpy(rgch, gszErrStr);
    } else {
        switch (msgtype) {
            case INFOMSG:
                szMsg = (char *) msgid;
                vsprintf(rgch, szMsg, va_mark);
                break;

            case EXPREVALMSG:
                strcpy(rgch, va_arg(va_mark, LPSTR));
                break;

            default:
                break;
        }
    }

    va_end ( va_mark );
    szMsg = rgch;
    //
    //  if not able to use cw or we want to go to the cmd window, go there.
    //

    switch (msgtype) {
        case EXPREVALMSG:
        case ERRORMSG:
            szErr = szCVErr;
            break;
        case WARNMSG:
            szErr = szCVWarn;
            break;
        case MESSAGEMSG:
            szErr = szCVMsg;
            break;
        default:
            szErr = szEmpty;
            break;
    }

    if (msgwhere == MSGSTRING) {
        strcpy(szStringLoc, szMsg);
    } else if (msgwhere == MSGGERRSTR) {
        gMSGID = msgid;
        strcpy(gszErrStr, szMsg);
    } else if (msgwhere == CMDWINDOW) {
        AuxPrintf(1, szFormat, msgid, (LPSTR)szErr);
        AuxPrintf(1, szMsg);
        if (strlen(szErr) == 0) szErr = szCVMsg;

        CmdLogFmt("%s\r\n", (LPSTR)szMsg);
    } else if (msgwhere == MSGBOX) {
        VarMsgBox(NULL, SYS_My_String, MB_OK|MB_APPLMODAL|MB_ICONHAND, (LPSTR)szMsg);
    }
}

// CVExprErr appears unmodified...

/*** CVExprErr
*
* Purpose: To get or print an expression evaluator error message
*
* Input:
*   Err     - The expression evaluator msg number
*   msgwhere - The place to print the message
*             CMDWINDOW, MSGBOX, STATLINE
*   phTM     - The expression evlauators TM
*   szErr    - If this is non NULL, the message is not printed, but rather
*               it is returned in the string. The size of this buffer must
*               be at least ???
*
* Output:
*
*   None
*
* Exceptions:
*
* Notes:
*
*/
void pascal
CVExprErr(
    EESTATUS    Err,
    MSGWHERE    msgwhere,
    PHTM        phTM,
    char far *  szErr
)
{
    EEHSTR          hErrStr;

    char FAR *  pErrStr;

    // check for internal catastrophic conditions
    if( !phTM  ||  !(*phTM) ) {
        Err = EECATASTROPHIC;
    }

    // now print this expression evaluator error
    switch( Err ) {

      default:

        // get the error string from the EE
        if( !EEGetError(phTM, Err, &hErrStr) ) {

            // lock the string in memory
            if( (pErrStr = (PSTR) MMLpvLockMb( (HDEP) hErrStr )) ) {
                if( msgwhere == MSGSTRING ) {
                    CVMessage(EXPREVALMSG, EXPRERROR, msgwhere, szErr, pErrStr);
                } else {
                    CVMessage(EXPREVALMSG, EXPRERROR, msgwhere, pErrStr);
                }
                MMbUnlockMb ( (HDEP) hErrStr );

             } else {

                // we really are having problems
                Err = EECATASTROPHIC;

            }

            // free the error string
                  EEFreeStr( hErrStr );
        }

        // only exit if not catastrophic
        if( Err != EECATASTROPHIC ) {
            break;
        }

        // otherwise say catastrophic error

      case EECATASTROPHIC:
      case EENOMEMORY:
        CVMessage(ERRORMSG, Err, msgwhere, szErr);
        break;

        // don't print an error
      case EENOERROR:
        break;

    }
}                                       /* CVExprErr() */


ULONG
SYGetReg (
    WORD ireg
    )
{
    ULONG lValue = 0;

    OSDReadRegister ( LppdCur->hpid, LptdCur->htid, ireg, &lValue );

    return lValue;
}
