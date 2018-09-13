/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    Cmdexec1.c

Abstract:

    This file contains the code for the execution related commands in
    the command window.

Author:

    Kent Forschmiedt (a-kentf) 20-Jul-92

Environment:

    Win32, User Mode

--*/

#include "precomp.h"
#pragma hdrstop

#include "dbugexcp.h"

#include <mbstring.h>
#define strcspn _mbscspn

/************************** Data declaration    *************************/

/****** Publics ********/

extern LPPD    LppdCommand;
extern LPTD    LptdCommand;

extern BOOL    FSetLptd;                 // Was thread specified
extern BOOL    FSetLppd;                 // Was process specified

extern INT     BpCmdPid;
extern INT     BpCmdTid;
extern char    is_assign;

extern EXCEPTION_LIST *DefaultExceptionList;

extern ULONG ulPseudo[];

/****** Locals ********/

int LocalFrameNumber = 0;


/****** Externs from ??? *******/

extern  LPSHF    Lpshf;   // vector table for symbol handler
extern  CXF      CxfIp;


LPSTR ParseContext_FileName(LPSTR lpContext);

void LogSx( EXCEPTION_LIST  *eList );


/**************************       Code          *************************/
/****************************************************************************
 *
 * Helper and common functions
 *
 ****************************************************************************/

BOOL
GoOK(
    LPPD lppd,
    LPTD lptd
    )
/*++

Routine Description:

    Determine whether a thread is GOable

Arguments:

    lppd - Supplies pointer to process struct
    lptd - Supplies pointer to thread struct

Return Value:

    TRUE if runnable, FALSE if not.

--*/
{
    if (!lppd || !lptd) {
        return FALSE;
    }
    if (lppd->fFrozen) {
        return FALSE;
    }

    switch (lppd->pstate) {
      case psNoProgLoaded:
      case psPreRunning:
      case psExited:
      case psDestroyed:
        return FALSE;

      case psRunning:
      case psStopped:
        break;
    }

    switch (lptd->tstate) {
      case tsPreRunning:
      case tsStopped:
      case tsRipped:
      case tsExited:
        break;

      case tsRunning:
      case tsException1:
      case tsException2:
        return FALSE;
    }

    return TRUE;
}          /* GoOK() */

BOOL
GoExceptOK(
    LPPD lppd,
    LPTD lptd
    )
/*++

Routine Description:

    Determine whether a thread is in an exception,
    and can therefore GoHandled or GoUnHandled.

Arguments:

    lppd - Supplies pointer to process struct
    lptd - Supplies pointer to thread struct

Return Value:

    TRUE if runnable, FALSE if not.

--*/
{
    if (!lppd || !lptd) {
        return FALSE;
    }
    if (lppd->fFrozen) {
        return FALSE;
    }

    switch (lppd->pstate) {
      case psNoProgLoaded:
      case psPreRunning:
      case psExited:
      case psDestroyed:
        return FALSE;

      case psRunning:
      case psStopped:
        break;
    }

    switch (lptd->tstate) {
      case tsPreRunning:
      case tsStopped:
      case tsRipped:
      case tsExited:
      case tsRunning:
        break;

      case tsException1:
      case tsException2:
        return TRUE;
    }

    return FALSE;
}          /* GoExceptOK() */


BOOL
StepOK(
    LPPD lppd,
    LPTD lptd
    )
/*++

Routine Description:

    Determine whether a thread is steppable.
    This will return TRUE for frozen threads, so caller will
    have to deal with that specially.

Arguments:

    lppd  - Supplies pointer to process structure
    lptd  - Supplies pointer to thread structure

Return Value:

    TRUE if steppable, FALSE if not

--*/
{
    if (!lppd || !lptd) {
        return FALSE;
    }

    if (lppd->fFrozen || lptd->fFrozen) {
        return FALSE;
    }

    switch (lppd->pstate) {
      case psNoProgLoaded:
      case psPreRunning:
      case psExited:
      case psDestroyed:
        return FALSE;

      case psRunning:
      case psStopped:
        break;
    }

    switch (lptd->tstate) {
      case tsPreRunning:
      case tsStopped:
        break;

      case tsRunning:
      case tsException1:
      case tsException2:
      case tsRipped:
      case tsExited:
        return FALSE;
    }

    return TRUE;
}              /* StepOK() */


void
NoRunExcuse(
    LPPD lppd,
    LPTD lptd
    )
/*++

Routine Description:

    Print a message about why we can't run or step

Arguments:

    lppd  - Supplies pointer to process that isn't runnable
    lptd  - Supplies pointer to thread that isn't runnable

Return Value:

    None

--*/
{
    if (!lppd) {
       CmdLogVar(ERR_Debuggee_Not_Alive);
       return;
    }

    if (lppd->fFrozen) {
        CmdLogVar(ERR_Cant_Run_Frozen_Proc);
        return;
    }

    switch (lppd->pstate) {

      case psNoProgLoaded:
        CmdLogVar(ERR_Debuggee_Not_Loaded);
        break;
      case psExited:
      case psDestroyed:
        CmdLogVar(ERR_Debuggee_Not_Alive);
        break;
      case psPreRunning:
        CmdLogVar(ERR_Debuggee_Starting);
        break;

      default:
        Assert(lptd);
        if (!lptd) {
            CmdLogVar(ERR_Debuggee_Not_Alive);
            return;
        }
        if (lptd->fFrozen) {
            CmdLogVar(ERR_Cant_Step_Frozen);
            return;
        }
        switch (lptd->tstate) {
          case tsRunning:
            CmdLogVar(ERR_Already_Running);
            break;
          case tsException1:
          case tsException2:
            CmdLogVar(ERR_Cant_Go_Exception);
            break;
          case tsRipped:
            CmdLogVar(ERR_Cant_Step_Rip);
            break;
          case tsExited:
            CmdLogVar(ERR_Thread_Exited);
            break;
          default:
            CmdLogVar(ERR_Command_Error);
            break;
        }
    }

    return;
}               /* NoRunExcuse() */

SHFLAG
PASCAL
PHCmpAlwaysMatch(
    LPSSTR lpsstr,
    LPV    lpv,
    LSZ    lpb,
    SHFLAG fcase
    )
/*++

Routine Description:

    Compare function for PHFindNameInPublics which always succeeds.

Arguments:

    lpsstr
    lpv
    lpb
    fcase

Return Value:

    Always 0

--*/
{
    return 0;
}


VOID
ThreadStatForThread(
    LPTD lptd
    )
{
    LPTD lptdSave;
    TST  tst;
    XOSD xosd;

    xosd = OSDGetThreadStatus(lptd->lppd->hpid, lptd->htid, &tst);
    if (xosd != xosdNone) {
        CmdLogFmt("No status for thread %d\r\n", lptd->itid);
    } else {
        CmdLogFmt("%s%2d  %s %s %s",
                  (lptd == LptdCur)? "*" : " ",
                  lptd->itid,
                  tst.rgchThreadID,
                  tst.rgchState,
                  tst.rgchPriority );

        if (tst.dwTeb) {
            CmdLogFmt(" 0x%08x", tst.dwTeb );
        }

        lptdSave = LptdCur;
        LptdCur = lptd;
        CmdLogFmt( " %s", GetLastFrameFuncName() );
        LptdCur = lptdSave;

        CmdLogFmt( "\r\n" );
    }
}

void
ThreadStatForProcess(
    LPPD lppd
    )
/*++

Routine Description:

    Prints status for all threads in a process

Arguments:

    lppd  - Supplies process to look at

Return Value:

    None.

--*/
{
    LPTD    lptd;

    if (lppd->lptdList == NULL) {
        CmdLogVar(ERR_No_Threads);
        return;
    }

    for (lptd = lppd->lptdList; lptd != NULL; lptd = lptd->lptdNext) {
        ThreadStatForThread(lptd);
    }
}


static BOOL
makemask(
    char ** ppch,
    WORD  * pmask
    )
{
    WORD mask = 0;

    Assert ( ppch != NULL );
    Assert ( *ppch != NULL );

    if (!**ppch || **ppch == ' ' || **ppch == '\t') {
        mask = HSYMR_lexical  |
               HSYMR_function |
               HSYMR_module   |
               HSYMR_exe      |
               HSYMR_public   |
               HSYMR_global;
    }

    while ( **ppch != '\0' && **ppch != ' ' && **ppch != '\t' ) {
        BOOL fAdd = TRUE;

        fAdd = (BOOL)( *( *ppch + 1 ) != '-' );

        switch ( **ppch ) {

            case 'l':
            case 'L':
                if( fAdd ) {
                    mask |= HSYMR_lexical;
                } else {
                    mask &= ~HSYMR_lexical;
                }
                break;

            case 'f':
            case 'F':
                if( fAdd ) {
                    mask |= HSYMR_function;
                } else {
                    mask &= ~HSYMR_function;
                }
                break;

            case 'c':
            case 'C':
                if ( fAdd ) {
                    mask |= HSYMR_class;
                } else {
                    mask &= ~HSYMR_class;
                }
                break;

            case 'm':
            case 'M':
                if( fAdd ) {
                    mask |= HSYMR_module;
                } else {
                    mask &= ~HSYMR_module;
                }
                break;

            case 'e':
            case 'E':
                if( fAdd ) {
                    mask |= HSYMR_exe;
                } else {
                    mask &= ~HSYMR_exe;
                }
                break;

            case 'p':
            case 'P':
                if( fAdd ) {
                    mask |= HSYMR_public;
                } else {
                    mask &= ~HSYMR_public;
                }
                break;

            case 'g':
            case 'G':
                if( fAdd ) {
                    mask |= HSYMR_global;
                } else {
                    mask &= ~HSYMR_global;
                }
                break;

            case '*':
                if( fAdd ) {
                    mask = HSYMR_allscopes;
                } else {
                    mask = 0x0000;
                }
                break;

            default:

                // invalid syntax
                return FALSE;
        }

        (*ppch)++;

        if( (**ppch == '+') || (**ppch == '-') ) {
            (*ppch)++;
        }
    }

    *pmask = mask;
    return TRUE;
}


typedef struct _tagHTMLIST {
    struct _tagHTMLIST      *next;
    LPSTR                   lpszName;
    LPSTR                   lpszValue;
    BOOL                    fArg;
} HTMLIST, *LPHTMLIST;

LOGERR PrintAllLocals(CXF * pcxf)
{
    DWORD               cParm = 0;
    EESTATUS            eeErr;
    LPHTMLIST           found;
    HTMLIST             head = {0};
    EEHSTR              hName  = 0;
    HSYM                hSym;
    HMEM                hsyml = 0;
    HTM                 hTm = NULL;
    HTM                 hTmParm = NULL;
    EEHSTR              hValue = 0;
    DWORD               i;
    DWORD               j;
    DWORD               len;
    PHSL_HEAD           lphsymhead;
    PHSL_LIST           lphsyml;
    LPSTR               lpszName;
    LPSTR               lpszValue;
    LOGERR              rval = LOGERROR_QUIET;
    SHFLAG              shflag;
    DWORD               strIndex;
    LPHTMLIST           tail = NULL;


    if (!LppdCur) {
        CmdLogVar(ERR_Debuggee_Not_Alive);
        eeErr = EENOERROR;
        rval = LOGERROR_QUIET;
        goto exit;
    }

    tail = &head;

    SHGetNearestHSYM( &pcxf->cxt.addr, pcxf->cxt.hMod, EECODE, &hSym );
    if (hSym) {
        eeErr = EEGetTMFromHSYM( hSym, &pcxf->cxt, &hTm, &i, /*ForceBind=*/TRUE, /*EnableProlog=*/FALSE );
        if (eeErr == EENOERROR) {

            eeErr = EEcParamTM( &hTm, &cParm, &shflag );
            if (eeErr) {
                goto exit;
            }

            for ( i=0; i<(DWORD)cParm; i++ ) {
                eeErr = EEGetParmTM( &hTm, (EERADIX)i, &hTmParm, &strIndex, FALSE );
                if (eeErr) {
                    continue;
                }
                eeErr = EEvaluateTM( &hTmParm, SHhFrameFrompCXF(pcxf),  EEVERTICAL);
                if (eeErr) {
                    EEFreeTM(  &hTmParm );
                    continue;
                }
                eeErr = EEGetNameFromTM( &hTmParm, &hName );
                if (eeErr) {
                    EEFreeTM(  &hTmParm );
                    continue;
                }
                eeErr = EEGetValueFromTM( &hTmParm, radix, NULL, &hValue );
                if (eeErr) {
                    EEFreeStr( hName );
                    EEFreeTM(  &hTmParm );
                    continue;
                }

                lpszName = (PSTR) MMLpvLockMb( hName );
                lpszValue = (PSTR) MMLpvLockMb( hValue );

                tail->next = (LPHTMLIST) malloc( sizeof(HTMLIST) );
                tail = tail->next;
                tail->next = NULL;
                tail->lpszName = _strdup( lpszName );
                tail->lpszValue = _strdup( lpszValue );
                tail->fArg = TRUE;

                MMbUnlockMb( hValue );
                MMbUnlockMb( hName );
                EEFreeStr( hValue );
                EEFreeStr( hName );
            }
            EEFreeTM(  &hTm );
        }
    }

    eeErr = EEGetHSYMList ( &hsyml, &pcxf->cxt, HSYMR_lexical + HSYMR_function, NULL, FALSE );
    if (!hsyml) {
        goto exit;
    }

    len = 0;
    lphsymhead = (PHSL_HEAD) MMLpvLockMb ( hsyml );

    lphsyml = (PHSL_LIST)(lphsymhead + 1);

    for ( i = 0; i != lphsymhead->blockcnt; i++ ) {
        for ( j = 0; j != lphsyml->symbolcnt; j++ ) {
            if ( SHCanDisplay ( lphsyml->hSym[j] ) ) {
                hSym = lphsyml->hSym[j];
                if (!hSym) {
                    continue;
                }
                eeErr = EEGetTMFromHSYM( hSym, &pcxf->cxt, &hTm, &strIndex, TRUE, FALSE );
                if (eeErr) {
                    continue;
                }

                eeErr = EEvaluateTM( &hTm, SHhFrameFrompCXF(pcxf),  EEVERTICAL);
                if (eeErr) {
                    EEFreeTM(  &hTm );
                    break;
                }

                eeErr = EEGetNameFromTM( &hTm, &hName );
                if (eeErr) {
                    EEFreeTM(  &hTm );
                    continue;
                }

                eeErr = EEGetValueFromTM( &hTm, radix, NULL, &hValue );
                if (eeErr) {
                    EEFreeStr( hName );
                    EEFreeTM(  &hTm );
                    continue;
                }

                lpszValue = (PSTR) MMLpvLockMb( hValue );
                lpszName = (PSTR) MMLpvLockMb( hName );

                {
                    DWORD tmp_len;
                    tmp_len = strlen(lpszName);
                    len = max( len, tmp_len);
                }

                found = head.next;
                while (found) {
                    if (found->fArg && (_stricmp(found->lpszName,lpszName)==0)) {
                        break;
                    }
                    found = found->next;
                }

                if (!found) {
                    tail->next = (LPHTMLIST) malloc( sizeof(HTMLIST) );
                    tail = tail->next;
                    tail->next = NULL;
                    tail->lpszName = _strdup( lpszName );
                    tail->lpszValue = _strdup( lpszValue );
                    tail->fArg = FALSE;
                }

                MMbUnlockMb( hValue );
                MMbUnlockMb( hName );
                EEFreeStr( hValue );
                EEFreeStr( hName );
                EEFreeTM(  &hTm );
            }
        }
        lphsyml = (PHSL_LIST) &(lphsyml->hSym[j]);
    }

    MMbUnlockMb ( hsyml );

    found = head.next;
    while (found) {
        if (found->fArg) {
            CmdLogFmt( "   <arg> " );
        } else {
            CmdLogFmt( "         " );
        }
        CmdLogFmt( "%-*s  %s\r\n", len, found->lpszName, found->lpszValue );
        found = found->next;
    }

    rval = LOGERROR_NOERROR;

exit:
    if (tail) {
        found = tail->next;
        while (found) {
            free( found->lpszName );
            free( found->lpszValue );
            tail = found->next;
            free( found );
            found = tail;
        }
    }

    return rval;
}

/*****************************************************************************
 *
 * Command Entry Points
 *
 *****************************************************************************/

LOGERR
LogAttach(
    LPSTR lpsz,
    DWORD dwUnused
    )
{
    LONG pid = 0;
    HANDLE  hEvent;
    LPSTR   lpsz1;
    char    szError[300];
    BOOL    fReconnect = FALSE;

    CmdInsertInit();

    PDWildInvalid();
    TDWildInvalid();

    /*
    **  Check for no argument
    */

    lpsz = CPSkipWhitespace(lpsz);
    if (*lpsz == 0) {

        fReconnect = TRUE;

    } else {

        lpsz1 = CPSzToken(&lpsz, NULL);
        if (!lpsz1) {
            return LOGERROR_UNKNOWN;
        }

        /*
        **  Process ID is always 32 bits
        */

        if (CPGetCastNbr(lpsz1,
                         T_LONG,
                         10,
                         fCaseSensitive,
                         &CxfIp,
                         (LPSTR)&pid,
                         szError,
                         FALSE) != EENOERROR) {
            CmdLogFmt("%s\r\n", szError);
            return LOGERROR_QUIET;
        }

    }

    hEvent = (HANDLE)0;

    if (!AttachDebuggee(pid, hEvent)) {

        CmdLogVar(ERR_Attach_Failed);
        return LOGERROR_QUIET;

    }

    //
    // AttachDebuggee() guarantees that the proc
    // is finished loading on return.
    //

    if (g_contWorkspace_WkSp.m_bAttachGo) {
        LptdCur->fGoOnTerm = TRUE;
        Go();
        CmdLogVar(DBG_Attach_Running);
    } else {
        CmdLogVar(DBG_Attach_Stopped);
        if (LptdCur) {
            if ( !(LptdCur->tstate & tsException1) && !(LptdCur->tstate & tsException2)) {
                SetPTState(psInvalidState, tsStopped);
            }
        }
        UpdateDebuggerState(UPDATE_WINDOWS);
    }

    return LOGERROR_NOERROR;
}

LOGERR
LogStart(
    LPSTR   lpsz,
    DWORD   dwUnused
    )
{
    return LogStartWithArgs(lpsz, NULL);
}


LOGERR
LogStartWithArgs(
    LPSTR   lpsz,
    LPSTR   lpszArgs
    )
{
    lpsz = CPSkipWhitespace(lpsz);
    if (*lpsz == 0) {
        return LOGERROR_UNKNOWN;
    }

    if (!RestartDebuggee(lpsz, lpszArgs)) {
        CmdLogVar(ERR_Start_Failed);
        return LOGERROR_QUIET;
    }

    return LOGERROR_NOERROR;
}

LOGERR
LogKill(
    LPSTR   lpsz,
    DWORD   dwUnused
    )
{
    LPPD    lppd;
    XOSD    xosd;
    int     err;
    int     cch;
    int     i;

    CmdInsertInit();

    PDWildInvalid();
    TDWildInvalid();

    /*
    **  Check for no argument
    */

    lpsz = CPSkipWhitespace(lpsz);
    if (*lpsz == 0) {
        return LOGERROR_UNKNOWN;
    }

    /*
    **  An argument -- must be a number in base 10
    */

    i = (int) CPGetInt(lpsz, &err, &cch);
    if (err) {
        return LOGERROR_UNKNOWN;
    }

    lppd = ValidLppdOfIpid(i);
    if (!lppd) {
        CmdLogVar(ERR_Process_Not_Exist);
        return LOGERROR_QUIET;
    }

    xosd = OSDProgramFree(lppd->hpid);

    if (xosd != xosdNone) {
        CmdLogVar(ERR_Kill_Failed);
        return LOGERROR_QUIET;
    }

    return LOGERROR_NOERROR;
}

LOGERR
LogConnect(
    LPSTR   lpsz,
    DWORD   dwUnused
    )
{
    CmdInsertInit();

    PDWildInvalid();
    TDWildInvalid();

    AttachDebuggee(0, NULL);

    return LOGERROR_NOERROR;
}

LOGERR
LogDisconnect(
    LPSTR   lpsz,
    DWORD   dwUnused
    )
{
    XOSD    xosd;

    CmdInsertInit();

    PDWildInvalid();
    TDWildInvalid();

    if (!LppdFirst) {
        CmdLogFmt("not connected\r\n");
        return LOGERROR_QUIET;
    }

    CmdLogFmt("Connection to remote has been broken\r\n" );
    CmdLogFmt("Stopped debugging\r\n" );

    if (g_contWorkspace_WkSp.m_bDisconnectOnExit && LppdCur && LptdCur) {
        xosd = OSDDisconnect( LppdCur->hpid, LptdCur->htid  );
    } else {
        xosd = OSDDisconnect( LppdFirst->hpid, NULL );
    }

    DisconnectDebuggee();

    return LOGERROR_NOERROR;
}

LOGERR
LogTitle(
    LPSTR   lpsz,
    DWORD   dwUnused
    )
{
    CmdInsertInit();

    lpsz = CPSkipWhitespace(lpsz);

    FREE_STR(g_contWorkspace_WkSp.m_pszWindowTitle);
    g_contWorkspace_WkSp.m_pszWindowTitle = _strdup(lpsz);

    SetWindowText( hwndFrame, lpsz );

    CmdLogFmt("Window title has been changed to %s\r\n", lpsz );

    return LOGERROR_NOERROR;
}

LOGERR
LogRemote(
    LPSTR   lpsz
    )
{
    BOOL fAppend = FALSE;

    CmdInsertInit();
    if (OsVersionInfo.dwPlatformId != VER_PLATFORM_WIN32_NT) {
        CmdLogFmt("Remote is only supported on Windows NT and Windows 2000\r\n");
        return LOGERROR_QUIET;
    }

    lpsz = CPSkipWhitespace(lpsz);

    if (((*lpsz == '/') || (*lpsz == '-')) && (tolower(*(lpsz+1)) == 'a')) {
        lpsz += 2;
        lpsz = CPSkipWhitespace(lpsz);
        fAppend = TRUE;
    }

    StartRemoteServer( lpsz, fAppend );

    return LOGERROR_NOERROR;
}

LOGERR
LogBPSet(
    BOOL  fDataBp,
    LPSTR lpsz
    )
/*++

Routine Description:

    This routine is used by the command processor to add a breakpoint
    to the set of breakpoints.  The breakpoint will be added and, if
    the debugger is running, committed.

Arguments:

    fDataBp  - TRUE  if the command is BA command
               FALSE if the command is a BP command

    lpsz     - Supplies string containing the breakpoint command to be added

Return Value:

    log error code

--*/
{
    BPSTATUS    bpstatus;
    HBPT        hbpt;
    int         iBp = -1;
    int         err, nRet;
    int         cch;
    EESTATUS    eest;
    ADDR        addr;
    CHAR        BaSize;
    CHAR        BaType;
    LOGERR      rVal = LOGERROR_NOERROR;
    LPPD        LppdT = LppdCur;
    LPTD        LptdT = LptdCur;
    char        szStr[MAX_USER_LINE], szC[MAX_USER_LINE];
    LPSTR       lpsz1, lpFile = 0;
    BOOL        bMap = FALSE;

    CmdInsertInit();

    IsKdCmdAllowed();

    PDWildInvalid();
    PreRunInvalid();

    if (*CPSkipWhitespace(lpsz) == 0) {
        return LOGERROR_UNKNOWN;
    }

    if (fDataBp) {
        lpsz = CPSkipWhitespace( lpsz );
        switch (tolower(*lpsz)) {
            case 'e':
            case 'r':
            case 'w':
                BaType = *lpsz;
                break;

            default:
                CmdLogFmt( "BA command missing type [e|r|w]\n" );
                return LOGERROR_QUIET;
        }
        lpsz++;
        lpsz = CPSkipWhitespace( lpsz );
        switch (*lpsz) {
            case '1':
            case '2':
            case '4':
                BaSize = *lpsz;
                break;

            default:
                CmdLogFmt( "BA command missing size [1|2|4]\n" );
                return LOGERROR_QUIET;
        }
        lpsz++;

        eest = CPGetAddress(lpsz,
                            &cch,
                            &addr,
                            radix,
                            &CxfIp,
                            fCaseSensitive,
                            g_contWorkspace_WkSp.m_bMasmEval
                            );

        if (eest != EENOERROR) {
            CmdLogFmt( "Invalid address\n" );
            return LOGERROR_QUIET;
        }

        SYFixupAddr(&addr);

        sprintf( szStr, "=\"0x%016I64x\" /R%c /A%c ", addr.addr.off, BaSize, BaType );

        if (FSetLppd) {
            sprintf(szStr + strlen(szStr), " /H%d", BpCmdPid);
        }

        if (FSetLptd && BpCmdTid != -1) {
            sprintf(szStr + strlen(szStr), " /T%d", BpCmdTid);
        }

        bpstatus = BPParse(
            &hbpt,
            szStr,
            NULL,
            NULL,
            LppdCur ? LppdCur->hpid : 0
            );

    } else {

        strcpy(szStr, lpsz);
        lpsz = szStr;

        if (FSetLppd) {
            sprintf(szStr + strlen(szStr), " /H%d", BpCmdPid);
        }

        if (FSetLptd && BpCmdTid != -1) {
            sprintf(szStr + strlen(szStr), " /T%d", BpCmdTid);
        }

        if (isdigit(*lpsz)) {
            iBp = CPGetInt(lpsz, &err, &cch);
            lpsz += cch;
            if (BPHbptFromI(&hbpt, iBp) != BPNoMatch) {
                CmdLogVar(ERR_Breakpoint_Already_Used, iBp);
                rVal = LOGERROR_QUIET;
                goto done;
            }
        }

        //
        // require space after bp[n]
        //
        lpsz1 = CPSkipWhitespace(lpsz);
        if (lpsz1 == lpsz) {
            rVal = LOGERROR_UNKNOWN;
            goto done;
        }

        if (!DebuggeeActive()) {
            //try to parse a filename out of the context set
            strcpy (szC, lpsz1);
            lpFile = ParseContext_FileName(szC);

            if (lpFile != (LPSTR)NULL) {
                bMap = FALSE;

                nRet = SrcMapSourceFilename (lpFile,_MAX_PATH,
                                             SRC_MAP_OPEN, NULL,
                                             // Not user activated
                                             FALSE);

                if ((nRet >= 1) && (nRet <= 2)) {
                    bMap = TRUE;
                }
            }
        }

        bpstatus = BPParse(
            &hbpt,
            lpsz1,
            NULL,
            (bMap == TRUE) ? lpFile : NULL,
            LppdCur ? LppdCur->hpid : 0
            );

    }

    if (bpstatus != BPNOERROR) {

        //
        // NOTENOTE a-kentf we can do better than "command error" here
        //
        rVal = LOGERROR_UNKNOWN;

    } else if ( BPAddToList(hbpt, iBp) != BPNOERROR ) {

        //
        // NOTENOTE a-kentf here, too
        //
        rVal = LOGERROR_UNKNOWN;

    } else {

        if (DebuggeeActive() ) {

            BPSTATUS Status;

            Status = BPBindHbpt( hbpt, &CxfIp );

            if ( LppdCur != NULL ) {
                if ( Status == BPCancel ) {
                     CmdLogVar(ERR_Breakpoint_Not_Set);
                } else if ( Status != BPNOERROR ) {
                     CmdLogVar(ERR_Breakpoint_Not_Instantiated);
                }
            }
        }
        Dbg(BPCommit() == BPNOERROR);
    }



done:
    LppdCur = LppdT;
    LptdCur = LptdT;
    UpdateDebuggerState(UPDATE_CONTEXT);
    return rVal;
}                   /* LogBPSet() */


LPSTR
ParseContext_FileName(
    LPSTR lpContext
    )
/*++

Routine Description:

    This routine is used by the command processor to parse the context
    set for filenames

Arguments:

    lpContext - Supplies string containing the context set to be be parsed

Return Value:

    LPSTR or (NULL)

--*/

{
    LPSTR   lpBegin;
    LPSTR   lpEnd;
    LPSTR   lpTarget = (LPSTR)NULL;

    //
    // Skip module
    //
    if ( lpBegin = (PSTR) strchr( (PBYTE) lpContext, ',' ) ) {

        //
        //  Skip blanks
        //
        lpBegin++;
        lpBegin += strspn( lpBegin, " \t" );

        //
        //  Get end of filename
        //
        lpEnd  = lpBegin + strcspn( (PBYTE) lpBegin, (PBYTE) ",} " );
        *lpEnd = '\0';

        if ( lpEnd > lpBegin ) {
            lpTarget = lpBegin;
        }
    }

    return lpTarget;
}



LOGERR
LogBPList(
    void
    )
/*++

Routine Description:

    This routine will display a list of the breakpoints in
    the command window.

Arguments:

    None

Return Value:

    log error code

--*/
{
    HBPT    hbpt = 0;
    HPID    hpid;
    char    rgch[256];

    CmdInsertInit();
    PreRunInvalid();

    Dbg(BPNextHbpt(&hbpt, bptNext) == BPNOERROR);

    if (hbpt == NULL) {
        /*
        **  No breakpoints to list
        */

        // not really an error
        CmdLogVar(ERR_No_Breakpoints);

    } else {
        for ( ; hbpt != NULL; BPNextHbpt( &hbpt, bptNext )) {

            if (FSetLppd && LppdCommand && LppdCommand != (LPPD)-1) {
                BPGetHpid(hbpt, &hpid);
                if (hpid != LppdCommand->hpid) {
                    continue;
                }
            }

            Dbg( BPFormatHbpt( hbpt, rgch, sizeof(rgch), BPFCF_ITEM_COUNT) == BPNOERROR );
            CmdLogFmt("%s\r\n", rgch );
        }
    }

    return LOGERROR_NOERROR;
}                   /* LogBPList() */


LOGERR
LogBPChange(
    LPSTR lpszArgs,
    int iAction
    )
/*++

Routine Description:

    This function will go through the set of arguments looking for
    a set of breakpoint numbers (or asterisk) and perform the action
    on each of the requested breakpoints.

Arguments:

    lpszArgs - Supplies string containing the set of breakpoints to change
    iAction  - Supplies which action to perform - Enable, Disable, Delete

Return Value:

    log error code

--*/
{
    HBPT     hbpt;
    HBPT     hbptN;
    int      i, j;
    int      err;
    int      cb;
    DWORD    ipid;
    LOGERR   rVal = LOGERROR_NOERROR;
    LPPD     LppdT = LppdCur;
    LPTD     LptdT = LptdCur;
    BPSTATUS Status;

    CmdInsertInit();
    IsKdCmdAllowed();
    PreRunInvalid();

    lpszArgs = CPSkipWhitespace(lpszArgs);

    /*
    **  There are two possible sets of values at this point.  The first
    **  is an asterisk ('*'), the second is a number or set of whitespace
    **  separated numbers.
    */

    if (*lpszArgs == '\0') {
        rVal = LOGERROR_UNKNOWN;
        goto done;
    }

    if (*lpszArgs == '*') {
        if (*CPSkipWhitespace(lpszArgs+1)) {
            rVal = LOGERROR_UNKNOWN;
            goto done;
        }
        Dbg( BPNextHbpt(&hbpt, bptFirst) == BPNOERROR);
        for ( ; hbpt != NULL; hbpt = hbptN) {

            hbptN = hbpt;
            Dbg( BPNextHbpt( &hbptN, bptNext ) == BPNOERROR);

            if (LppdCommand && LppdCommand != (LPPD)-1) {
                BPGetIpid(hbpt, &ipid);
                if (LppdCommand->ipid != ipid) {
                    continue;
                }
            }

            switch ( iAction ) {
              case LOG_BP_DISABLE:
                BPDisable( hbpt );
                break;

              case LOG_BP_ENABLE:
                Status = BPEnable( hbpt );
                if ( Status != BPNOERROR ) {
                     CmdLogVar(ERR_Breakpoint_Not_Instantiated);
                }
                break;

              case LOG_BP_CLEAR:
                BPDelete( hbpt );
                break;
            }
        }
    } else {
        while ( *(lpszArgs = CPSkipWhitespace(lpszArgs)) ) {

            i = (int) CPGetInt(lpszArgs, &err, &cb);
            if (err) {
                BPUnCommit();
                rVal = LOGERROR_UNKNOWN;
                goto done;
            }

            lpszArgs = CPSkipWhitespace(lpszArgs + cb);
            j = i;
            if (*lpszArgs == '-') {
                j = CPGetInt((lpszArgs = CPSkipWhitespace(lpszArgs+1)), &err, &cb);
                if (err) {
                    BPUnCommit();
                    rVal = LOGERROR_UNKNOWN;
                    goto done;
                }
                lpszArgs = CPSkipWhitespace(lpszArgs + cb);
            }

            if (*lpszArgs == ',') {
                lpszArgs++;
            }

            for ( ; i <= j; i++) {
                err = BPHbptFromI( &hbpt, i );
                if (err != BPNOERROR) {
                    CmdLogVar(ERR_Breakpoint_Not_Exist, i);
                } else {
                    switch ( iAction ) {
                      case LOG_BP_DISABLE:
                        BPDisable( hbpt );
                        break;

                      case LOG_BP_ENABLE:
                        Status = BPEnable( hbpt );
                        if ( Status != BPNOERROR ) {
                             CmdLogVar(ERR_Breakpoint_Not_Instantiated);
                        }
                        break;

                      case LOG_BP_CLEAR:
                        BPDelete( hbpt );
                        break;
                    }
                }
            }
        }
    }
    BPCommit();

done:
    LppdCur = LppdT;
    LptdCur = LptdT;
    return rVal;
}                   /* LogBPChange() */


LOGERR
LogEvaluate(
    LPSTR lpsz,
    BOOL fSpecialNtsdEval
    )
/*++

Routine Description:

    This function will take a string, evalute it and display either
    the result or an error message

Arguments:

    lpsz                - pointer to string to be evaluated
    fSpecialNtsdEval    - use NTSD expresion evaluator

Return Value:

    log error code

--*/
{
    long        cChild = 0;
    long        cPtr = 0;
    CXF         cxf;
    LOGERR      logerror = LOGERROR_NOERROR;
    EESTATUS    eeErr = EENOERROR;
    EEPDTYP     ExpTyp;
    BOOL        fFmtStr = FALSE;
    EEHSTR      hErrStr;
    EEHSTR      hName  = 0;
    HTI         hti;
    HTM         hTm = NULL;
    HTM         hTmChild = NULL;
    HTM         hTmPtr = NULL;
    EEHSTR      hValue = 0;
    long        i;
    UINT        len;
    LPPD        LppdT = LppdCur;
    LPSTR       lpszName;
    LPSTR       lpszValue;
    LPTD        LptdT = LptdCur;
    LPSTR       pErrStr;
    PTI         pti;
    RTMI        rti = {0};
    SHFLAG      shflag;
    DWORD       strIndex;
#if 0
    HTMLIST     head = {0};
    LPHTMLIST   tail = NULL;
    LPHTMLIST   found;
    HTM         hTmParm = NULL;
    DWORD       j;
    BOOL        fExpandable;
    HMEM        hsyml = 0;
    PHSL_HEAD   lphsymhead;
    PHSL_LIST   lphsyml;
    HSYM        hSym;
    DWORD       cParm = 0;
#endif // 0

    //
    //  Start by doing standard state checking on the command.
    //

    CmdInsertInit();
    IsKdCmdAllowed();

    PDWildInvalid();
    TDWildInvalid();

    PreRunInvalid();

    //
    //  Check for no expression
    //

    lpsz = CPSkipWhitespace(lpsz);

    if (*lpsz == 0) {
        return LOGERROR_UNKNOWN;
    }

    //
    //  If a different thread or process that the current process is
    //  specified -- switch the debugger to be looking at the requested
    //  process not the current process
    //

    LppdCur = LppdCommand;
    LptdCur = LptdCommand;
    if (LptdCur != LptdT  ||  LppdCur != LppdT) {
        UpdateDebuggerState(UPDATE_CONTEXT);
    }

    //
    //  Force command window open
    //

    CmdNoLogString("");

    //
    //  If we need to -- change the context to point to the frame
    //  on the stack rather than the frame associated with the current PC
    //

    if (LocalFrameNumber) {
        cxf = *ChangeFrame( LocalFrameNumber );
    } else {
        cxf = CxfIp;
    }


    //
    // Use NTSD expression evaluator
    //
    if (fSpecialNtsdEval) {
        // Use NTSD expresion evaluator.
        ADDR addr;
        int nStatus, nCntCharsUsed;
        XOSD xosd;

        nStatus = CPGetAddress(lpsz,
            &nCntCharsUsed,
            &addr,
            radix,
            &cxf,
            FALSE,
            TRUE);

        if (CPNOERROR == nStatus && (xosdNone == (xosd = SYFixupAddr(&addr)) ) ) {
            CmdLogFmt("Evaluate expression: %I64d = 0x%016I64x\r\n", addr.addr.off, addr.addr.off);
        } else {
            logerror = LOGERROR_CP;
        }
        goto exit;
    }


    //
    //  If we see a lone '.', we are going to list all of the locals for
    //  the current procedure (this is mainly for remote debugging as
    //  otherwise there is a locals window which shows the same thing)
    //

    if (*lpsz == '.' && strlen(lpsz) == 1) {
        PrintAllLocals(&cxf);
        goto exit;
    }


    //
    //  Normal expression evaluation.
    //
    //    Start with the normal sequence of Parse, Bind and Evaluate
    //  for the expression
    //

    is_assign = FALSE;
    eeErr = EEParse(lpsz, radix, fCaseSensitive, &hTm, &strIndex);
    if (eeErr) {
        goto exit;
    }

    eeErr = EEBindTM( &hTm, SHpCXTFrompCXF(&cxf), TRUE, FALSE );
    if (eeErr) {
        goto exit;
    }

    eeErr = EEvaluateTM( &hTm, SHhFrameFrompCXF(&cxf), EEVERTICAL );
    if (eeErr) {
        goto exit;
    }

    //
    //  Extract some info from the result.  There are certain types of
    //  operations we can't or don't do.
    //
    //  1.  Function calls are not evaluated if the process is running
    //

    eeErr = EEInfoFromTM( &hTm, &rti, &hti );
    if (eeErr || hti == NULL) {
        goto exit;
    }
    pti = (PTI) MMLpvLockMb( hti );
    if (pti == NULL) {
        goto exit;
    }
    if (pti->fFunction && IsProcRunning(LppdCur)) {
        MMbUnlockMb(hti);
        EEFreeTI( &hti );
        goto exit;
    }

    fFmtStr = pti->fFmtStr;
    MMbUnlockMb(hti);
    EEFreeTI( &hti );

    //
    //  Display the result we are first interested in
    //

    eeErr = EEGetValueFromTM( &hTm, radix, NULL, &hValue);
    if (eeErr) {
        goto exit;
    }

    lpszValue = (PSTR) MMLpvLockMb(hValue);
    ulPseudo[CV_REG_PSEUDO9-CV_REG_PSEUDO1] = strtoul(lpszValue, NULL, 0);
    CmdLogFmt("%s\r\n", lpszValue);
    MMbUnlockMb(hValue);
    EEFreeStr(hValue);

    //
    //  Now lets do some interesting play as well --
    //
    //  Specifically they want to display an expanded version of the
    //  item if the conditions are correct.
    //
    //  What suppressed expanded display are the following things:
    //  1.  The item is not expandable
    //  2.  The expression has a side effect
    //  3.  There is a format string on the expression
    //

    if (fFmtStr || is_assign) {
        goto exit;
    }

    ExpTyp = EEIsExpandable(&hTm);
    if ((ExpTyp == EENOTEXP) || (ExpTyp == EETYPENOTEXP)) {
        goto exit;
    }

    eeErr = EEcChildrenTM(&hTm, &cChild, &shflag);
    if (eeErr) {
        goto exit;
    }
    if (ExpTyp == EEPOINTER) {
        eeErr = EEDereferenceTM(&hTm, &hTmPtr, &strIndex, fCaseSensitive);
        if (eeErr) {
            goto exit;
        }

        eeErr = EEcChildrenTM(&hTmPtr, &cPtr, &shflag);
        if (eeErr) {
            goto exit;
        }
    }

#if 0 // Why is this here?  // JLS
    eeErr = EEGetNameFromTM(&hTm, &hName);
    if (eeErr == EENOERROR) {
        lpszName = MMLpvLockMb(hName);
        CmdLogFmt("%s ", lpszName);
        MMbUnlockMb(hName);
        EEFreeStr(hName);
    }
#endif // Why is this here?

    //
    //  This is a pointer to a class or a structure
    //
    if ((ExpTyp == EEPOINTER) && (cChild == 1)) {
        if (!cPtr) {
            goto exit;
        } else {
            cChild = cPtr;
            EEFreeTM(&hTm);
            hTm = hTmPtr;
            hTmPtr = NULL;
            eeErr = EEvaluateTM(&hTm, SHhFrameFrompCXF(&cxf), EEVERTICAL);
            if (eeErr) {
                goto exit;
            }
        }
    }

    eeErr = EEGetValueFromTM(&hTm, radix, NULL, &hValue);
    if (eeErr) {
        goto exit;
    }

    lpszValue = (PSTR) MMLpvLockMb(hValue);
    if ((ExpTyp != EEPOINTER) || (cChild != 0)) {
        MMbUnlockMb( hValue );
        EEFreeStr( hValue );
    } else {
        //
        //  this is a pointer to an aggregate type or the user wants a
        //      string printed
        //

        eeErr = EEvaluateTM(&hTmPtr, SHhFrameFrompCXF(&cxf), EEVERTICAL);
        if (eeErr) {
            goto exit;
        }

        eeErr = EEGetValueFromTM(&hTmPtr, radix, NULL, &hName);
        if (eeErr) {
            goto exit;
        }

        lpszName = (PSTR) MMLpvLockMb(hName);
        CmdLogFmt("%s   %s\r\n", lpszValue, lpszName);

        EEFreeStr(hValue);
        EEFreeStr(hName);
        MMbUnlockMb(hValue);
        MMbUnlockMb(hName);
        goto exit;
    }

    //
    //  From now on may take a while -- allow user to abort operation
    //

    SetCtrlCTrap();

    //
    // first loop thru all of the children to see what the longest name is
    //
    for (i=0,len=0; i<cChild; i++) {
        if (CheckCtrlCTrap()) {
            ClearCtrlCTrap();
            goto exit;
        }

        eeErr = EEGetChildTM ( &hTm, i, &hTmChild, &strIndex, fCaseSensitive, radix );
        if (eeErr) {
            break;
        }

        eeErr = EEGetNameFromTM( &hTmChild, &hName );
        if (eeErr) {
            EEFreeTM(  &hTmChild );
            continue;
        }
        lpszName = (PSTR) MMLpvLockMb( hName );

        {
            UINT tmp_len;
            tmp_len = strlen(lpszName);
            len = max( len, tmp_len);
        }

        MMbUnlockMb( hName );
        EEFreeStr( hName );
        EEFreeTM(  &hTmChild );
    }

    for (i=0; i<cChild; i++) {
        if (CheckCtrlCTrap()) {
            break;
        }

        eeErr = EEGetChildTM ( &hTm, i, &hTmChild, &strIndex, fCaseSensitive, radix );
        if (eeErr) {
            break;
        }

        eeErr = EEvaluateTM( &hTmChild, SHhFrameFrompCXF(&cxf),  EEVERTICAL);
        if (eeErr) {
            EEFreeTM(  &hTmChild );
            break;
        }

        eeErr = EEGetNameFromTM( &hTmChild, &hName );
        if (eeErr) {
            EEFreeTM(  &hTmChild );
            break;
        }

        eeErr = EEGetValueFromTM( &hTmChild, radix, NULL, &hValue);
        if (eeErr) {
            EEFreeTM(  &hTmChild );
            EEFreeStr( hName );
            break;
        }

        lpszValue = (PSTR) MMLpvLockMb( hValue );
        lpszName = (PSTR) MMLpvLockMb( hName );

        CmdLogFmt( "    %-*s  %s\r\n", len, lpszName, lpszValue );

        MMbUnlockMb( hValue );
        MMbUnlockMb( hName );
        EEFreeStr( hValue );
        EEFreeStr( hName );
        EEFreeTM(  &hTmChild );
    }

    ClearCtrlCTrap();

#if 0
    ExpTyp = EEIsExpandable ( &hTm );
    if ( ExpTyp == EENOTEXP || ExpTyp == EETYPENOTEXP ) {
        fExpandable = FALSE;
    } else {
        fExpandable = TRUE;
        eeErr = EEcChildrenTM( &hTm, &cChild, &shflag );
        if (eeErr) {
            goto exit;
        }
        if (ExpTyp == EEPOINTER) {
            eeErr = EEDereferenceTM ( &hTm, &hTmPtr, &strIndex, fCaseSensitive );
            if (eeErr) {
                goto exit;
            }
            eeErr = EEcChildrenTM( &hTmPtr, &cPtr, &shflag );
            if (eeErr) {
                goto exit;
            }
            eeErr = EEvaluateTM( &hTm, SHhFrameFrompCXF(&cxf), EEVERTICAL );
            if (eeErr) {
                goto exit;
            }
        }
    }

    eeErr = EEInfoFromTM( &hTm, &rti, &hti );
    if (eeErr || hti == NULL) {
        goto exit;
    }
    pti = MMLpvLockMb( hti );
    if (pti == NULL) {
        goto exit;
    }
    if (pti->fFunction && IsProcRunning(LppdCur)) {
        EEFreeTI( &hti );
        goto exit;
    }

    fFmtStr = pti->fFmtStr;
    EEFreeTI( &hti );

    if (fExpandable) {
        eeErr = EEGetNameFromTM( &hTm, &hName );
        if (eeErr == EENOERROR) {
            lpszName = MMLpvLockMb( hName );
            CmdLogFmt( "%s  ", lpszName );
            MMbUnlockMb( hName );
            EEFreeStr( hName );
        }

        if (ExpTyp == EEPOINTER && cChild == 1) {
            //
            // this is a pointer to a class or a structure
            //
            if (cPtr) {
                //
                // the class/structure has members
                //
                cChild = cPtr;
                EEFreeTM(  &hTm );
                hTm = hTmPtr;
                eeErr = EEvaluateTM( &hTm, SHhFrameFrompCXF(&cxf), EEVERTICAL );
                if (eeErr) {
                    goto exit;
                }
            } else {
                //
                // this is an empty class/structure
                //
                fExpandable = FALSE;
                fEmpty = TRUE;
            }
        }
    }

    eeErr = EEGetValueFromTM( &hTm, radix, NULL, &hValue);
    if (eeErr) {
        goto exit;
    }

    lpszValue = MMLpvLockMb( hValue );
    if (!fExpandable) {
        ulPseudo[CV_REG_PSEUDO9-CV_REG_PSEUDO1] = strtoul(lpszValue, NULL, 0);
    }

    if (fExpandable && ExpTyp == EEPOINTER && cChild == 0 && (!fFmtStr)) {

        //
        // this is pointer to an agregate type or the user wants a string printed
        //
        eeErr = EEvaluateTM( &hTmPtr, SHhFrameFrompCXF(&cxf), EEVERTICAL );
        if (eeErr) {
            goto exit;
        }

        eeErr = EEGetValueFromTM( &hTmPtr, radix, NULL, &hName );
        if (eeErr) {
            goto exit;
        }

        lpszName = MMLpvLockMb( hName );

        CmdLogFmt( "%s  %s\r\n", lpszValue, lpszName );

        EEFreeStr( hValue );
        EEFreeStr( hName );
        MMbUnlockMb( hValue );
        MMbUnlockMb( hName );
        goto exit;

    } else {

        if (fEmpty) {
            CmdLogFmt("%s (empty class/structure)\r\n", lpszValue);
        } else {
            CmdLogFmt("%s\r\n", lpszValue);
        }
        MMbUnlockMb( hValue );
        EEFreeStr( hValue );

        if (fExpandable && cChild > 0 && fFmtStr) {
            goto exit;
        }
    }

    if (fExpandable) {

        //
        // first loop thru all of the children to see what the longest name is
        //
        for (i=0,len=0; i<cChild; i++) {
            eeErr = EEGetChildTM ( &hTm, i, &hTmChild, &strIndex, fCaseSensitive, radix );
            if (eeErr) {
                break;
            }

            eeErr = EEGetNameFromTM( &hTmChild, &hName );
            if (eeErr) {
                EEFreeTM(  &hTmChild );
                continue;
            }
            lpszName = MMLpvLockMb( hName );
            len = max( len, strlen(lpszName) );

            MMbUnlockMb( hName );
            EEFreeStr( hName );
            EEFreeTM(  &hTmChild );
        }

        SetCtrlCTrap();

        for (i=0; i<cChild; i++) {
            if (CheckCtrlCTrap()) {
                break;
            }

            eeErr = EEGetChildTM ( &hTm, i, &hTmChild, &strIndex, fCaseSensitive, radix );
            if (eeErr) {
                break;
            }

            eeErr = EEvaluateTM( &hTmChild, SHhFrameFrompCXF(&cxf),  EEVERTICAL);
            if (eeErr) {
                EEFreeTM(  &hTmChild );
                break;
            }

            eeErr = EEGetNameFromTM( &hTmChild, &hName );
            if (eeErr) {
                EEFreeTM(  &hTmChild );
                break;
            }

            eeErr = EEGetValueFromTM( &hTmChild, radix, NULL, &hValue);
            if (eeErr) {
                EEFreeTM(  &hTmChild );
                EEFreeStr( hName );
                break;
            }

            lpszValue = MMLpvLockMb( hValue );
            lpszName = MMLpvLockMb( hName );

            CmdLogFmt( "    %-*s  %s\r\n", len, lpszName, lpszValue );

            MMbUnlockMb( hValue );
            MMbUnlockMb( hName );
            EEFreeStr( hValue );
            EEFreeStr( hName );
            EEFreeTM(  &hTmChild );
        }

        ClearCtrlCTrap();
    }
#endif // 0

exit:

    if (eeErr != EENOERROR) {
        pErrStr = NULL;
        if (!EEGetError( &hTm, eeErr, &hErrStr)) {
            pErrStr = (PSTR) MMLpvLockMb( hErrStr );
        }
        if (!pErrStr) {
            CmdLogFmt( "Unknown error\r\n");
        } else {
            CmdLogFmt( "%s\r\n", pErrStr );
            MMbUnlockMb ( (HDEP) hErrStr );
            EEFreeStr( hErrStr );
        }
    }

    if (hTm) {
        EEFreeTM(  &hTm );
    }

    if (hTmPtr) {
        EEFreeTM(&hTmPtr);
    }

    if (LocalFrameNumber) {
        ChangeFrame( 0 );
    }

    if (LptdCur != LptdT  ||  LppdCur != LppdT) {
        LppdCur = LppdT;
        LptdCur = LptdT;
        UpdateDebuggerState(UPDATE_CONTEXT);
    }

    UpdateDebuggerState(UPDATE_DATAWINS);

    return logerror;
}


LOGERR
LogFrameChange(
    LPSTR lpsz
    )

/*++

Routine Description:

    This function will take a string, evalute it and display either
    the result or an error message

Arguments:

    lpsz    - pointer to string to be evaluated

Return Value:

    log error code

--*/

{
    LPSTR   lpsz1;
    DWORD   frame;
    CHAR    szError[300];



    lpsz1 = CPSzToken( &lpsz, NULL );
    if (!lpsz1) {
        return LOGERROR_UNKNOWN;
    }

    if (!LppdCur) {
        CmdLogVar(ERR_Debuggee_Not_Alive);
        return LOGERROR_QUIET;
    }

    if (CPGetCastNbr( lpsz1,
                      T_LONG,
                      radix,
                      fCaseSensitive,
                      &CxfIp,
                      (LPSTR)&frame,
                      szError,
                      FALSE
                      ) != EENOERROR) {
        CmdLogFmt("%s\r\n", szError);
        return LOGERROR_QUIET;
    }

    if (IsValidFrameNumber( frame )) {
        LocalFrameNumber = frame;
    } else {
        CmdLogFmt( "Invalid frame number\r\n" );
        return LOGERROR_QUIET;
    }

    UpdateDebuggerState(UPDATE_DATAWINS);

    return LOGERROR_NOERROR;
}


LOGERR
XWorker(
    LPSTR lpRE,
    WORD  mask,
    PCXF  lpCxf
    )
{
    HMEM        hsyml = 0;
    PHSL_LIST   lphsyml;
    PHSL_HEAD   lphsymhead = NULL;
    EESTATUS    eest;
    BOOL        fAbort;
    HTM         hTM;
    ADDR        addr;

    UINT        i;
    UINT        j;
    int         cch;
    LOGERR      err = LOGERROR_QUIET;

    char        szAddr[100];
    char        szNameBuf[257];
    char        szContext[257];
    HMEM        hStr = 0;
    LPSTR       lpStr;



    SetCtrlCTrap();
    fAbort = FALSE;
    do {
        eest = EEGetHSYMList ( &hsyml, &lpCxf->cxt, mask, (PBYTE) lpRE, TRUE );
        if ( eest ) {

            // error occured, display error msg and get out
            CVExprErr ( eest, CMDWINDOW, &hTM, NULL);
            fAbort = TRUE;

        } else {

            // display the syms
            lphsymhead = (PHSL_HEAD) MMLpvLockMb ( hsyml );
            lphsyml = (PHSL_LIST)(lphsymhead + 1);

            for ( i = 0; !fAbort && i != (UINT)lphsymhead->blockcnt; i++ ) {

                *szContext = 0;
                if ( lphsyml->status.hascxt &&
                     !EEFormatCXTFromPCXT(&lphsyml->Cxt, &hStr, g_contWorkspace_WkSp.m_bShortContext)) {

                    lpStr = (PSTR) MMLpvLockMb( hStr );
                    if (g_contWorkspace_WkSp.m_bShortContext) {
                        strcpy( szContext, lpStr );
                    } else {
                        BPShortenContext( lpStr, szContext);
                        strcat( szContext, " " );
                    }

                    MMbUnlockMb(hStr);
                    EEFreeStr(hStr);

                }

                szNameBuf[0] = '&';

                for ( j = 0; !fAbort && j < (UINT)lphsyml->symbolcnt; j++ ) {

                    if ( SHGetSymName ( lphsyml->hSym[j],
                                        (LPSTR)szNameBuf+1 ) ) {

                        eest = EENOERROR;
                        addr = *SHpAddrFrompCxt(&lpCxf->cxt);
                        if (!SHAddrFromHsym(&addr, lphsyml->hSym[j])) {
                            eest = CPGetAddress(szNameBuf,
                                                &cch,
                                                &addr,
                                                radix,
                                                lpCxf,
                                                fCaseSensitive,
                                                g_contWorkspace_WkSp.m_bMasmEval);
                        }
                        if (eest == EENOERROR) {
                            SYFixupAddr(&addr);
                            EEFormatAddress(&addr,
                                            szAddr,
                                            sizeof(szAddr),
                                            g_contWorkspace_WkSp.m_bShowSegVal? EEFMT_SEG : 0
                                            );
                            CmdLogFmt("%s   %s%s\n",
                                      szAddr,
                                      szContext,
                                      szNameBuf+1 );
                        }
                    }
                    fAbort = CheckCtrlCTrap();
                }
                lphsyml = (PHSL_LIST) &(lphsyml->hSym[j]);
            }

            MMbUnlockMb ( hsyml );
        }
    } while ( !fAbort && ! lphsymhead->status.endsearch );

    if (!fAbort) {
        err = LOGERROR_NOERROR;
    }

    ClearCtrlCTrap();

    if ( hsyml ) {
        EEFreeHSYMList ( &hsyml );
    }

    return fAbort;
}



LOGERR
LogExamine(
    LPSTR lpsz
    )
/*++

Routine Description:

    eXamine symbols command:
    x <pattern>

    pattern may include * and ? as in DOS filename matching.

Arguments:

    lpsz  - Supplies pointer to command tail

Return Value:

    LOGERROR code

--*/
{
#define CONTEXT_TEMPLATE    "{,,%s}0"

    CXF         cxf;
    EESTATUS    eest;
    HTM         hTM;
    HCXTL       hCXTL = 0;
    PCXTL       pCXTL;
    LPSTR       lpRE;
    LPSTR       lpCxt;
    LPSTR       p;
    char        szStr[257];
    WORD        mask;
    DWORD       cc;
    ADDR        addr;
    int         err = LOGERROR_NOERROR;
    LPPD        LppdT = LppdCur;
    LPTD        LptdT = LptdCur;


    CmdInsertInit();

    PDWildInvalid();
    TDWildInvalid();

    PreRunInvalid();

    LppdCur = LppdCommand;
    LptdCur = LptdCommand;

    UpdateDebuggerState(UPDATE_CONTEXT);

    if (!DebuggeeActive()) {
        CmdLogVar(ERR_Debuggee_Not_Alive);
        err = LOGERROR_QUIET;
        goto done;
    }

    p = CPSkipWhitespace( lpsz );
    if (_stricmp(p,"*!")==0) {
        LogListModules("", FALSE);
        return LOGERROR_NOERROR;
    }

    //
    // set up the mask
    //
    if (!makemask ( &lpsz, &mask )) {
        err = LOGERROR_UNKNOWN;
        goto done;
    }

    if (!fCaseSensitive) {
        mask |= HSYMR_nocase;
    }

    memset(&cxf, 0, sizeof(cxf));

    lpCxt = NULL;
    p = NULL;
    lpsz = CPSkipWhitespace( lpsz );

    if ( *lpsz == '{' ) {

        for (p = lpsz; *p && *p != '}'; ) {
            p = CharNext(p);
        }

        if (!*p) {
            err = LOGERROR_UNKNOWN;
            goto done;
        }

        lpCxt = (PSTR) malloc( (size_t) (p-lpsz + 3) );
        Assert(lpCxt);
        strncpy(lpCxt, lpsz, (size_t) (p-lpsz+1));
        strcpy(lpCxt + (p-lpsz+1), "0");

        lpsz = CPSkipWhitespace(p+1);

    } else if ( p = (PSTR) strchr( (PBYTE) lpsz, '!') ) {

        lpsz = CPSkipWhitespace(lpsz);

        lpCxt = (PSTR) malloc( (size_t) ((p-lpsz) + strlen(CONTEXT_TEMPLATE)) );
        Assert(lpCxt);

        *p = '\0';
        sprintf( lpCxt, CONTEXT_TEMPLATE, lpsz );
        *p = '!';
        lpsz = CPSkipWhitespace(p+1);

    } else {

        addr = CxfIp.cxt.addr;
        SYFixupAddr(&addr);
        SHGetModule(&addr,szStr);

        lpCxt = (PSTR) malloc( strlen(szStr) + strlen(CONTEXT_TEMPLATE) );
        sprintf( lpCxt, CONTEXT_TEMPLATE, szStr );

        lpsz = CPSkipWhitespace(lpsz);
    }

    eest = EEParse(lpCxt, radix, fCaseSensitive, &hTM, &cc);
    if (!eest) {
        eest = EEBindTM(&hTM, &CxfIp.cxt, TRUE, FALSE);
        if (!eest) {
            eest = EEGetCXTLFromTM(&hTM, &hCXTL);
        }
    }

    free ( lpCxt );

    if (!eest) {

        pCXTL = (PCXTL) MMLpvLockMb (hCXTL);
        cxf.cxt = pCXTL->rgHCS[0].CXT;
        MMbUnlockMb (hCXTL);

    } else if ( hTM ) {

        // error occured, bail out
        CVExprErr (eest, CMDWINDOW, &hTM, NULL);
        EEFreeTM(&hTM);
        err = LOGERROR_QUIET;
        goto done;

    } else {

        EEFreeTM(&hTM);
        CmdLogVar(ERR_Bad_Context);
        goto done;
    }

    EEFreeTM(&hTM);

    lpRE = lpsz;

    // UNDONE: The tests below don't make sense to me.  Why check for either alpha or '_', '@'?  BryanT

    if ( isalpha(*lpRE) || *lpRE == '_' ) {
        *szStr = '_';
        strcpy(szStr+1, lpRE);
        err = XWorker(szStr, mask, &cxf);
    }
    if ( isalpha(*lpRE) || *lpRE == '@' ) {
        *szStr = '@';
        strcpy(szStr+1, lpRE);
        err = XWorker(szStr, mask, &cxf);
    }
    if ( isalpha(*lpRE) || (*lpRE == '.' && *(lpRE + 1) == '.') ) {
        *szStr   = '.';
        *(szStr+1) = '.';
        strcpy(szStr+2, lpRE);
        err = XWorker(szStr, mask, &cxf);
    }
    if (err == LOGERROR_NOERROR) {
        err = XWorker(lpRE, mask, &cxf);
    }

done:
    LppdCur = LppdT;
    LptdCur = LptdT;
    UpdateDebuggerState(UPDATE_CONTEXT);

    return err;
}


LOGERR
LogException(
    LPSTR lpsz
    )
/*++

Routine Description:

    This function will take a string, evalute it as a hex digit and
    either disable or enable it. If the string is null then we will
    print out a list of all the known exceptions and whether they
    are handled or not.

Arguments:

    lpsz    - pointer to string holding the hex numbered exception

Return Value:

    NOTENOTE No error code at this point

--*/
{
    LPPD            LppdT       = LppdCur;
    LPTD            LptdT       = LptdCur;
    LOGERR          rVal        = LOGERROR_NOERROR;
    char            chCmd;
    BOOL            fException  = FALSE;
    BOOL            fName       = FALSE;
    BOOL            fCmd        = FALSE;
    BOOL            fCmd2       = FALSE;
    BOOL            fInvalid    = FALSE;
    DWORD           Exception;
    LPSTR           lpName      = NULL;
    LPSTR           lpCmd       = NULL;
    LPSTR           lpCmd2      = NULL;
    EXCEPTION_LIST *eList = NULL;
    EXCEPTION_DESCRIPTION Exd = {0};


    CmdInsertInit();

    PDWildInvalid();
    TDWildInvalid();

    //PreRunInvalid();

    LppdCur = LppdCommand;
    LptdCur = LptdCommand;

    //
    //  Get default exception list if necessary
    //
    if ( !LppdCur && !DefaultExceptionList ) {
        if ( !GetDefaultExceptionList() ) {
            CmdLogVar(ERR_No_ExceptionList);
            rVal = LOGERROR_QUIET;
            goto done;
        }
    }

    //
    //  Get an Action:
    //
    //      D       - Disable
    //      E       - Enable
    //      N       - Notify
    //      Blank   - List
    //
    chCmd = *lpsz;
    if ( chCmd != '\0' ) {
        if ( IsDBCSLeadByte( chCmd ) ) {
            rVal = LOGERROR_UNKNOWN;
            goto done;
        }
        lpsz++;
        if ( strchr( (PBYTE) " \t", chCmd ) ) {
            chCmd = '\0';
        } else if ( !strchr( (PBYTE) "dDeEnN", chCmd ) ) {
            rVal = LOGERROR_UNKNOWN;
            goto done;
        }
    }

    //
    // handle sx[d|e|n] *
    //

    lpsz = CPSkipWhitespace(lpsz);
    if (chCmd && *lpsz == '*') {

        lpsz = CPSkipWhitespace(lpsz+1);
        if (*lpsz) {
            rVal = LOGERROR_UNKNOWN;
            goto done;
        }
        Exd.dwExceptionCode = 0;
        Exd.exc = exfDefault;
        if (chCmd == 'd' || chCmd == 'D') {
            Exd.efd = efdIgnore;
        } else if (chCmd == 'e' || chCmd == 'E') {
            Exd.efd = efdStop;
        } else {
            Exd.efd = efdNotify;
        }

        if (LppdCur) {
            OSDSetExceptionState(LppdCur->hpid, LptdCur->htid, &Exd);
        } else {
            EfdPreset = Exd.efd;
        }

        rVal = LOGERROR_NOERROR;
        goto done;
    }


    //
    //  Parse line
    //
    rVal = ParseException( lpsz,
                           radix,
                           &fException,
                           NULL,
                           &fName,
                           &fCmd,
                           &fCmd2,
                           &fInvalid,
                           &Exception,
                           NULL,
                           &lpName,
                           &lpCmd,
                           &lpCmd2 );

    if ( rVal != LOGERROR_NOERROR ) {
        if ( fInvalid ) {
            CmdLogVar(ERR_Exception_Invalid);
        }

        goto done;
    }

    if (fException && fInvalid) {
        rVal = HandleSpecialExceptions( Exception,
                                        chCmd,
                                        lpName
                                        );
        if (lpName) {
            free(lpName);
        }
        goto done;
    }

    //
    //  Validate arguments & Execute the command
    //
    switch (chCmd) {
        case '\0':
            //
            // Plain sx command
            //
            if ( fName || fCmd || fCmd2 ) {
                rVal = LOGERROR_UNKNOWN;
                goto done;
            }
            break;

        case 'd':
        case 'D':
            //
            // Can contain: Name
            //
            if ( !fException ) {
                rVal = LOGERROR_QUIET;
                CmdLogVar(ERR_Exception_Invalid);
                goto done;
            } else if ( fCmd || fCmd2 ) {
                rVal = LOGERROR_UNKNOWN;
                goto done;
            }
            Exd.dwExceptionCode = Exception;
            Exd.exc = exfSpecified;
            Exd.efd = efdIgnore;
            if (fName) {
                strncpy(Exd.rgchDescription,
                        lpName? lpName : "",
                        EXCEPTION_STRING_SIZE);
            }
            if (LppdCur) {
                OSDSetExceptionState(LppdCur->hpid, LptdCur->htid, &Exd);
            }
            break;

        case 'n':
        case 'N':
            //
            // Can contain: Name & Cmd2
            //
            if ( !fException ) {
                rVal = LOGERROR_QUIET;
                CmdLogVar(ERR_Exception_Invalid);
                goto done;
            } else if ( fCmd ) {
                rVal = LOGERROR_UNKNOWN;
                goto done;
            }
            Exd.dwExceptionCode = Exception;
            Exd.exc = exfSpecified;
            Exd.efd = efdNotify;
            if (fName) {
                strncpy(Exd.rgchDescription,
                        lpName? lpName : "",
                        EXCEPTION_STRING_SIZE);
            }
            if (LppdCur) {
                OSDSetExceptionState(LppdCur->hpid, LptdCur->htid, &Exd);
            }
            break;

        case 'e':
        case 'E':
            //
            // Can contain: Name, Cmd & Cmd2
            //
            if ( !fException ) {
                rVal = LOGERROR_QUIET;
                CmdLogVar(ERR_Exception_Invalid);
                goto done;
            }
            Exd.dwExceptionCode = Exception;
            Exd.exc = exfSpecified;
            Exd.efd = efdStop;
            if (fName) {
                strncpy(Exd.rgchDescription,
                        lpName? lpName : "",
                        EXCEPTION_STRING_SIZE);
            }
            if (LppdCur) {
                OSDSetExceptionState(LppdCur->hpid, LptdCur->htid, &Exd);
            }
            break;
    }

    if ( fException ) {

        //
        // Try to find this exception on the processes exception list
        //
        for (eList = LppdCur ? LppdCur->exceptionList : DefaultExceptionList;
             eList;
             eList = eList->next) {
            if (eList->dwExceptionCode==Exception) {
                break;
            }
        }
    }

    if ( chCmd == '\0' ) {
        //
        //  Execute plain sx command if requested.
        //
        if ( fException ) {
            //
            //  Display specified exception
            //
            if ( eList ) {

                LogSx( eList );

            } else {

                CmdLogVar(ERR_Exception_Unknown, Exception);
                rVal = LOGERROR_QUIET;
                goto done;
            }
        } else {
            char szString[MAX_PATH];
            int n;

            if (LppdCur) {
                Exd.exc = exfDefault;
                OSDGetExceptionState(LppdCur->hpid, LptdCur->htid, &Exd);
                n = Exd.efd;
            } else {
                n = EfdPreset;
            }
            switch (n) {
                case efdIgnore:
                    n = DBG_Default_Exception_Ignore;
                    break;

                case efdCommand:
                case efdStop:
                    n = DBG_Default_Exception_Stop;
                    break;

                case efdNotify:
                    n = DBG_Default_Exception_Notify;
                    break;
            }

            if (n != -1) {
                Dbg(LoadString(g_hInst, n, (LPSTR)szString, MAX_MSG_TXT));
                CmdLogVar(DBG_Default_Exception_Text, szString);
            }

            //
            //  Display all exceptions
            //
            for (eList = LppdCur ? LppdCur->exceptionList :
                                    DefaultExceptionList;
                    eList;
                    eList = eList->next)
            {
                LogSx( eList );
            }
        }

    } else {
        //
        //  Add exception if not in list
        //
        if (!eList) {
            eList=(EXCEPTION_LIST*)malloc(sizeof(EXCEPTION_LIST));
            Assert(eList);
            eList->dwExceptionCode = Exception;
            eList->lpName          = NULL;
            eList->lpCmd           = NULL;
            eList->lpCmd2          = NULL;
            eList->efd             = efdIgnore;

            if ( LppdCur ) {
                InsertException( &LppdCur->exceptionList, eList);
                if ( LppdCur->ipid == 0 ) {
                    DefaultExceptionList = LppdCur->exceptionList;
                }
            } else {
                InsertException( &DefaultExceptionList, eList);
            }
        }

        //
        //  Set appropriate fields
        //
        eList->efd = Exd.efd;

        if (fName) {
            if (eList->lpName) {
                free(eList->lpName);
            }
            eList->lpName = lpName;
        }

        if (fCmd) {
            if (eList->lpCmd) {
                free(eList->lpCmd);
            }
            eList->lpCmd  = lpCmd;
        }

        if (fCmd2) {
            if (eList->lpCmd2) {
                free(eList->lpCmd2);
            }
            eList->lpCmd2  = lpCmd2;
        }
    }

done:
    LppdCur = LppdT;
    LptdCur = LptdT;
    if ( rVal != LOGERROR_NOERROR ) {
        //
        //  Free allocated memory
        //
        if ( lpName ) {
            free( lpName );
        }

        if ( lpCmd ) {
            free( lpCmd );
        }

        if ( lpCmd2 ) {
            free( lpCmd2 );
        }
    }
    return rVal;
}




void
LogSx(
    EXCEPTION_LIST  *eList
    )
/*++

Routine Description:

    Displays the given exception in the command window

Arguments:

    eList - Supplies exception to display

Return Value:

    none

--*/
{

    char    Buffer[512];

    FormatException( eList->efd,
                     eList->dwExceptionCode,
                     eList->lpName,
                     eList->lpCmd,
                     eList->lpCmd2,
                     " ",
                     Buffer );

    CmdLogFmt("%s\r\n", Buffer );
}




LOGERR
LogFreeze(
    LPSTR lpsz,
    BOOL fFreeze
    )
/*++

Routine Description:

    This function is used from the command line to freeze and thaw
    debuggee threads.  We use LptdCommand to determine which thread
    should be frozen or thawed.

Arguments:

    lpsz    - Supplies argument list; should be empty
    fFreeze - Supplies TRUE if freeze the thread, FALSE if thaw the thread

Return Value:

    log error code

--*/
{
    LPPD     LppdT = LppdCur;
    LPTD     LptdT = LptdCur;
    LOGERR   rVal = LOGERROR_NOERROR;

    CmdInsertInit();
    PreRunInvalid();

    if (*lpsz != 0) {

        rVal = LOGERROR_UNKNOWN;

    } else if (LptdCommand == (LPTD)-1) {

        if (LppdCur == NULL) {

            rVal = LOGERROR_UNKNOWN;

        } else {

            for (LptdCur = LppdCur->lptdList; LptdCur; LptdCur = LptdCur->lptdNext) {
#ifdef OSDEBUG4
                if (OSDFreezeThread(LptdCur->lppd->hpid, LptdCur->htid,
                                                         fFreeze) == xosdNone)
#else
                if (OSDPtrace(fFreeze ? osdFreeze : osdThaw,
                        0, 0, LptdCur->lppd->hpid, LptdCur->htid)
                    == xosdNone)
#endif
                {
                    LptdCur->fFrozen = fFreeze;
                }
                else
                {
                    CmdLogVar((WORD)(fFreeze? ERR_Cant_Freeze : ERR_Cant_Thaw));
                    rVal = LOGERROR_QUIET;
                    break;
                }
            }
        }

    } else {

        LppdCur = LppdCommand;
        LptdCur = LptdCommand;

        if (LptdCur == NULL)
        {
            rVal = LOGERROR_UNKNOWN;
        }
        else if (LptdCur == (LPTD) -1)
        {
            Assert(FALSE);
        }
        else if (fFreeze && LptdCur->fFrozen)
        {
            CmdLogVar(ERR_Thread_Is_Frozen);
            rVal = LOGERROR_QUIET;
        }
        else if (!fFreeze && !LptdCur->fFrozen)
        {
            CmdLogVar(ERR_Thread_Not_Frozen);
            rVal = LOGERROR_QUIET;
        }
#ifdef OSDEBUG4
        else if (OSDFreezeThread(LptdCur->lppd->hpid, LptdCur->htid, fFreeze)
                                                                  == xosdNone)
#else
        else if (OSDPtrace(fFreeze ? osdFreeze : osdThaw,
                        0, 0, LptdCur->lppd->hpid, LptdCur->htid)
                == xosdNone)
#endif
        {
            LptdCur->fFrozen = fFreeze;
        }
        else
        {
            CmdLogVar((WORD)(fFreeze? ERR_Cant_Freeze : ERR_Cant_Thaw));
            rVal = LOGERROR_QUIET;
        }
    }

    LppdCur = LppdT;
    LptdCur = LptdT;
    return rVal;
}                   /* LogFreeze() */


LOGERR
LogGoException(
    LPSTR lpsz,
    BOOL  fHandled
    )
/*++

Routine Description:

    GH and GN commands.  Continue handled or unhandled from exception.

Arguments:

    lpsz     - Supplies pointer to tail of command

    fHandled - Supplies flag indicating whether exception should be
               handled or not.

Return Value:

    LOGERROR code

--*/
{
    XOSD     xosd;
    LOGERR   rVal = LOGERROR_NOERROR;
    LPPD     LppdT = LppdCur;
    LPTD     LptdT = LptdCur;

    CmdInsertInit();

    if (g_contWorkspace_WkSp.m_bKernelDebugger && g_contKernelDbgPreferences_WkSp.m_bUseCrashDump) {
        CmdLogFmt( "Go is not allowed for crash dumps\n" );
        return rVal;
    }

    TDWildInvalid();
    PDWildInvalid();

    LppdCur = LppdCommand;
    LptdCur = LptdCommand;

    if (*CPSkipWhitespace(lpsz) != '\0') {
        rVal = LOGERROR_UNKNOWN;
    } if (LptdCur == NULL) {
        CmdLogVar(ERR_Debuggee_Not_Alive);
        rVal = LOGERROR_QUIET;
    } else if (LptdCur->tstate != tsException1 && LptdCur->tstate != tsException2) {
        CmdLogVar(ERR_Not_At_Exception);
        rVal = LOGERROR_QUIET;
    } else {
        EXOP exop = {0};
        exop.fSingleThread = TRUE;
        exop.fInitialBP = TRUE;
        exop.fPassException = !fHandled;
        xosd = OSDGo(LppdCur->hpid, LptdCur->htid, &exop);
        if (xosd == xosdNone) {
            SetPTState(psRunning, tsRunning);
        } else {
            CmdLogVar(ERR_Cant_Cont_Exception);
            rVal = LOGERROR_QUIET;
        }
    }

    LppdCur = LppdT;
    LptdCur = LptdT;
    return rVal;
}


LOGERR
LogGoUntil(
    LPSTR lpsz
    )
/*++

Routine Description:

    This routine will parse out an address, set a temp breakpoint
    at the address and issue a go command

    Syntax:
        g [=startaddr] [address]

Arguments:

    lpsz    - address to put the temporary breakpoint at

Return Value:

    LOGERR code

--*/
{
    ADDR    addr;
    HBPT    hbpt = NULL;
    CXF     cxf = CxfIp;
    LPPD    LppdT = NULL;
    LPTD    LptdT = NULL;
    LOGERR  rVal = LOGERROR_NOERROR;


    CmdInsertInit();

    if (g_contWorkspace_WkSp.m_bKernelDebugger && g_contKernelDbgPreferences_WkSp.m_bUseCrashDump) {
        CmdLogFmt( "Go is not allowed for crash dumps\n" );
        return rVal;
    }

    PDWildInvalid();

    /*
     * If debugger is initializing, bail out.
     */

    PreRunInvalid();

    /*
     * Different process, implies any thread:
     */
    if (FSetLppd && !FSetLptd) {

        /*
         * change process and make thread wildcard
         */

        FSetLppd = FALSE;
        LppdT = LppdCur;
        LppdCur = LppdCommand;
        LptdT = LptdCur;
        LptdCur = LppdCur->lptdList;
        UpdateDebuggerState(UPDATE_CONTEXT);

        LptdCommand = (LPTD)-1;
        FSetLptd = TRUE;

        rVal = LogGoUntil(lpsz);

        LppdCur = LppdT;
        LptdCur = LptdT;
        UpdateDebuggerState(UPDATE_CONTEXT);

        return rVal;
    }


    /*
     * Any process, any thread:
     */
    if (LptdCommand == (LPTD)-1) {

        BOOL fDidSomething = FALSE;

        FSetLptd = TRUE;  // this should already be true, in fact.

        for (LptdCommand = LppdCur->lptdList; LptdCommand; LptdCommand = LptdCommand->lptdNext) {
            if (GoOK(LppdCur, LptdCommand)) {
                fDidSomething = TRUE;
                if ((rVal = LogGoUntil(lpsz)) != LOGERROR_NOERROR) {
                    return rVal;
                }
            }
        }

        if (!fDidSomething) {
            CmdLogVar(ERR_Process_Cant_Go);
            rVal = LOGERROR_QUIET;
        }

        return rVal;
    }

    /*
     * switch debugger context to requested proc/thread
     */

    if (FSetLppd || FSetLptd) {
        LppdT = LppdCur;
        LppdCur = LppdCommand;
        LptdT = LptdCur;
        LptdCur = LptdCommand;
        UpdateDebuggerState(UPDATE_CONTEXT);
        cxf = CxfIp;
    }


    if (DebuggeeActive()) {
        if (!GoOK(LppdCur, LptdCur)) {
            NoRunExcuse(LppdCur, LptdCur);
            rVal = LOGERROR_QUIET;
            goto done;
        }
    } else if (DebuggeeAlive()) {
        NoRunExcuse(GetLppdHead(), NULL);
        rVal = LOGERROR_QUIET;
        goto done;
    }

    /*
    **  If this is a no argument go command then just do the go and be done
    **  with the whole mess.
    */
    if (*lpsz == 0) {
        if (LptdCur && LptdCur->fFrozen) {
            CmdLogVar(DBG_Go_When_Frozen);
        }
        if (ExecDebuggee(EXEC_GO)) {
            rVal = LOGERROR_NOERROR;
            goto done;
        } else {
            CmdLogVar(ERR_Go_Failed);
            rVal = LOGERROR_QUIET;
            goto done;
        }
    }

    if (LptdCur && LptdCur->fFrozen) {
        CmdLogVar(ERR_Simple_Go_Frozen);
        return LOGERROR_QUIET;
    }

    /*
    **  If the debuggee is not loaded then we need to get it loaded so that
    **  we have symbols that can be evaluated
    */

    if (!DebuggeeAlive()) {
        if (!ExecDebuggee(EXEC_RESTART)) {
            CmdLogVar(ERR_Cant_Start_Proc);
            return LOGERROR_QUIET;
        }
        LppdCommand = LppdCur;
        LptdCommand = LppdCur->lptdList;
    }

    /*
    ** Can this happen?
    */

    if (LppdCur->pstate == psExited) {
        return LOGERROR_UNKNOWN;
    }

    /*
    **  Check for a starting address optional argument.  If no arguments
    **  are left then issue a go command after changing the current
    **  instruction pointer.
    */

    if (*lpsz == '=') {
        CmdLogFmt("   NYI: starting address\r\n");
        return LOGERROR_QUIET;
    }

    /*
    **  Now get the termination address, note that we need to replace any
    **  leading periods ('.') with '@'s as these must really be line numbers
    */

    lpsz = CPSkipWhitespace(lpsz);

    if (BPParse(&hbpt,
                lpsz,
                NULL,
                NULL,
                LppdCur ? LppdCur->hpid: 0)
           != BPNOERROR)
    {
        Assert( hbpt == NULL );
        CmdLogVar(ERR_AddrExpr_Invalid);
        rVal = LOGERROR_QUIET;

    } else if (BPBindHbpt( hbpt, &cxf ) == BPNOERROR) {

        /*
        **  go the go until
        */
        Dbg( BPAddrFromHbpt( hbpt, &addr ) == BPNOERROR) ;
        Dbg( BPFreeHbpt( hbpt ) == BPNOERROR) ;
        GoUntil(&addr);

    } else if (!LppdCur->fHasRun) {

        /*
         * haven't executed entrypoint: save it for later
         */
        LppdCur->hbptSaved = (HANDLE)hbpt;
        Go();

    } else {

        /*
         * bind failed
         */
        Dbg( BPFreeHbpt( hbpt ) == BPNOERROR );
        CmdLogVar(ERR_AddrExpr_Invalid);
        rVal = LOGERROR_QUIET;
    }


done:
    if (FSetLppd || FSetLptd) {
        LppdCur = LppdT;
        LptdCur = LptdT;
        UpdateDebuggerState(UPDATE_CONTEXT);
    }
    return rVal;
}                   /* LogGoUntil() */

int
WINAPIV
CompareModEntryBySelector(
    const void *pMod1,
    const void *pMod2
    )
{
    LPMODULE_ENTRY  Mod1 = (LPMODULE_ENTRY)pMod1;
    LPMODULE_ENTRY  Mod2 = (LPMODULE_ENTRY)pMod2;

    if ( ModuleEntrySelector(Mod1) < ModuleEntrySelector(Mod2) ) {
        return -1;
    } else if ( ModuleEntrySelector(Mod1) > ModuleEntrySelector(Mod2) ) {
        return 1;
    } else {
        return 0;
    }
}


int
WINAPIV
CompareModEntryByAddress(
    const void *pMod1,
    const void *pMod2
    )
{
    return ModuleEntryBase((LPMODULE_ENTRY)pMod1) < ModuleEntryBase((LPMODULE_ENTRY)pMod2) ? -1 : 1;
}


BOOL
FormatHSym(
    HSYM    hsym,
    PCXT    cxt,
    char   *szStr
    )
{
    DWORD   retval;
    HTM     htm;
    DWORD   strIndex;
    EEHSTR  eehstr;
    DWORD   cb;
    BOOL    Ok = FALSE;
    EERADIX uradix = radix;

    retval = EEGetTMFromHSYM ( hsym, cxt, &htm, &strIndex, TRUE, FALSE );

    if ( retval == EENOERROR ) {
        retval = EEGetExprFromTM( &htm, &uradix, &eehstr, &cb);

        if ( retval == EENOERROR ) {
            strcpy(szStr, (PSTR) MMLpvLockMb( eehstr ));
            MMbUnlockMb( eehstr );
            EEFreeStr( eehstr );
            Ok = TRUE;
        }

        EEFreeTM( &htm );
    }

    if ( !Ok ) {
        Ok = PtrToInt(SHGetSymName(hsym, szStr));
    }

    return Ok;
}




LOGERR
LogListNear(
    LPSTR lpsz
    )
/*++

Routine Description:

    ln "list near" command:
    ln <addr>

Arguments:

    lpsz  - Supplies pointer to command tail

Return Value:

    LOGERROR code

--*/
{
    ADDR        addr;
    ADDR        addr1;
    HSYM        hsymP;
    HSYM        hsymN;
    DWORDLONG   dwOff;
    DWORDLONG   dwOffP;
    DWORDLONG   dwOffN;

    HDEP        hsyml;
    PHSL_HEAD   lphsymhead;
    PHSL_LIST   lphsyml;
    LPMODULE_LIST   lpModList;
    LPMODULE_ENTRY  lpModEntry;

    int         cch;
    char        szStr[MAX_USER_LINE];
    char        szContext[MAX_USER_LINE];
    LPSTR       lpContext;
    CXT         cxt;
    HDEP        hstr;
    UINT        n;
    EESTATUS    eest;
    XOSD        xosd;

    LOGERR      rVal = LOGERROR_NOERROR;
    LPPD        LppdT = LppdCur;
    LPTD        LptdT = LptdCur;


    CmdInsertInit();
    IsKdCmdAllowed();

    TDWildInvalid();
    PDWildInvalid();
    PreRunInvalid();

    LppdCur = LppdCommand;
    LptdCur = LptdCommand;
    if (LptdCur != LptdT  ||  LppdCur != LppdT) {
        UpdateDebuggerState(UPDATE_CONTEXT);
    }

    if (!DebuggeeActive()) {
        CmdLogVar(ERR_Debuggee_Not_Alive);
        rVal = LOGERROR_QUIET;
        goto done;
    }

    /*
    **  get address
    */

    lpsz = CPSkipWhitespace(lpsz);

    if (*lpsz == 0) {
        lpsz = ".";
    }

    if (CPGetAddress(lpsz,
                     &cch,
                     &addr,
                     radix,
                     &CxfIp,
                     fCaseSensitive,
                     g_contWorkspace_WkSp.m_bMasmEval) == CPNOERROR) {
        if (*CPSkipWhitespace(lpsz + cch) != 0) {
            rVal = LOGERROR_UNKNOWN;
            goto done;
        }
    } else {
        CmdLogVar(ERR_AddrExpr_Invalid);
        rVal = LOGERROR_QUIET;
        goto done;
    }

    if ((HPID)emiAddr( addr ) == LppdCur->hpid && ADDR_IS_LI(addr)) {
        emiAddr( addr )    = 0;
        ADDR_IS_LI( addr ) = FALSE;
#ifdef OSDEBUG4
        OSDSetEmi(LppdCur->hpid,LptdCur->htid,&addr);
#else
        OSDPtrace(osdSetEmi, wNull, &addr, LppdCur->hpid, LptdCur->htid);
#endif
        if ( (HPID)emiAddr( addr ) != LppdCur->hpid ) {
            SHWantSymbols( (HEXE)emiAddr( addr ) );
        }
        SYUnFixupAddr(&addr);
    }

    memset(&cxt, 0, sizeof(cxt));
    SHSetCxt(&addr, &cxt);


    //
    // the following hack works around a shortcoming of CV info.
    // We only have module maps for code contributor segments, so
    // we can't get a useful CXT for a data address.  A little
    // brute force abuse will generate a usable HMOD:
    //

    //
    // (this is going to be harder if the address is segmented...
    //  I didn't try to solve that one.)
    //

    if ( SHHMODFrompCXT( &cxt ) == NULL ) {

        addr1 = addr;
        SYFixupAddr(&addr1);

        // get module table

        xosd = OSDGetModuleList( LppdCur->hpid,
                                 LptdCur->htid,
                                 NULL,
                                 &lpModList );

        if (xosd == xosdNone && ModuleListCount(lpModList) > 0 ) {
            qsort( FirstModuleEntry(lpModList),
                   ModuleListCount(lpModList),
                   sizeof(MODULE_ENTRY),
                   CompareModEntryByAddress );

            // find nearest exe

            lpModEntry = FirstModuleEntry(lpModList);

            for ( n = 0; n < ModuleListCount( lpModList ) - 1; n++) {

                if (GetAddrOff(addr1) <
                           ModuleEntryBase(NextModuleEntry(lpModEntry)) )
                {
                    break;
                }

                lpModEntry = NextModuleEntry(lpModEntry);
            }

            SHHMODFrompCXT( &cxt ) = SHGetNextMod((HEXE)ModuleEntryEmi(lpModEntry), NULL);
        }
    }

    hsyml = 0;
    eest = EEGetHSYMList(&hsyml, &cxt,
                     HSYMR_module | HSYMR_global | HSYMR_public, NULL, TRUE);
    if (eest != EENOERROR) {
        rVal = LOGERROR_CP;
        goto done;
    }

    lphsymhead = (PHSL_HEAD) MMLpvLockMb ( hsyml );
    lphsyml = (PHSL_LIST)(lphsymhead + 1);

    dwOffP = (DWORDLONG)(LONGLONG)-1;
    dwOffN = (DWORDLONG)(LONGLONG)-1;
    hsymP = 0;
    hsymN = 0;

    SYFixupAddr(&addr);
    addr1 = addr;
    for ( n = 0; n < (UINT)lphsyml->symbolcnt; n++ ) {
        if (SHAddrFromHsym(&addr1, lphsyml->hSym[n])) {
            SYFixupAddr(&addr1);
            if (!ADDR_IS_FLAT(addr1) && !ADDR_IS_FLAT(addr) &&
                    GetAddrSeg(addr1) != GetAddrSeg(addr))
            {
                continue;
            }
            if (GetAddrOff(addr1) <= GetAddrOff(addr)) {
                dwOff = GetAddrOff(addr) - GetAddrOff(addr1);
                if (dwOff < dwOffP) {
                    dwOffP = dwOff;
                    hsymP = lphsyml->hSym[n];
                }
            } else {
                dwOff = GetAddrOff(addr1) - GetAddrOff(addr);
                if (dwOff < dwOffN) {
                    dwOffN = dwOff;
                    hsymN = lphsyml->hSym[n];
                }
            }
        }
    }

    MMbUnlockMb(hsyml);

    if (!hsymP && !hsymN) {
        CmdLogVar(DBG_Symbol_Not_Found);
    } else {

        EEFormatCXTFromPCXT( &cxt, &hstr, g_contWorkspace_WkSp.m_bShortContext );

        lpContext = (PSTR) MMLpvLockMb( hstr );

        if (g_contWorkspace_WkSp.m_bShortContext) {
            strcpy( szContext, lpContext );
        } else {
            BPShortenContext( lpContext, szContext);
        }

        MMbUnlockMb(hstr);
        EEFreeStr(hstr);

        if ( hsymP && FormatHSym(hsymP, &cxt, szStr) ) {
            CmdLogFmt("%s%s+%#0x\r\n", szContext, szStr, dwOffP);
        }

        if ( hsymN && FormatHSym(hsymN, &cxt, szStr) ) {
            CmdLogFmt("%s%s-%#0x\r\n", szContext, szStr, dwOffN);
        }
    }

    MMFreeHmem(hsyml);

done:
    if (LptdCur != LptdT  ||  LppdCur != LppdT) {
        LppdCur = LppdT;
        LptdCur = LptdT;
        UpdateDebuggerState(UPDATE_CONTEXT);
    }
    return rVal;
}                       /* LogListNear() */



LOGERR
ListModules(
    LOGERR *prVal,
    BOOL    Flat,
    BOOL    SelSort,
    LPSTR   ModName,
    BOOL    fExtendedLoadInfo
    )
/*++

Routine Description:

    Lists modules

Arguments:

    rVal    - Pointer to LOGERR

    Flat    - Flat flag

    SelSort - Sort by selector flag

    ModName - Module name to look for (optional)

    fExtendedInfo - TRUE - Display extended load errors
                    FALSE - Don't display extended load errors

Return Value:

    TRUE  if modules listed.

--*/
{
    XOSD            xosd;
    LPMODULE_LIST   ModList;
    LPMODULE_ENTRY  ModEntry;
    DWORD           Mod;
    LPSTR           LastName;
    BOOL            Ok   = FALSE;
    LOGERR          rVal = LOGERROR_NOERROR;
    LPSTR           lpch;
    LPSTR           lpModName;
    LPSTR           lpSymName;
    CHAR            buf[100];
    SHE             sheLoadStatus;
    SHE             sheLoadError;
    BOOL            fDoTitle = TRUE;


    //
    //  Get module list
    //
    if ( !LppdCur || !LptdCur ) {

        rVal = LOGERROR_UNKNOWN;

    } else {

        xosd = OSDGetModuleList( LppdCur->hpid,
                                 LptdCur->htid,
                                 ModName,
                                 &ModList );

        if ( xosd != xosdNone ) {

            //
            //  Could not get module list!
            //
            rVal = LOGERROR_UNKNOWN;

        } else {

            if ( !ModName ) {
                if ( Flat ) {
                    CmdLogFmt("\r\n" );
                    CmdLogFmt("Flat Modules:\r\n" );
                } else {
                    CmdLogFmt("\r\n" );
                    CmdLogFmt("Segmented Modules:\r\n" );
                }
            }

            if ( ModuleListCount(ModList) > 0 ) {

                Ok = TRUE;

                //
                //  Sort Module list
                //
                if ( SelSort ) {

                    //
                    //  Sort by selector
                    //
                    qsort( FirstModuleEntry(ModList),
                           ModuleListCount(ModList),
                           sizeof(MODULE_ENTRY),
                           CompareModEntryBySelector );
                } else {

                    //
                    //  Sort by Base address
                    //
                    qsort( FirstModuleEntry(ModList),
                           ModuleListCount(ModList),
                           sizeof(MODULE_ENTRY),
                           CompareModEntryByAddress );
                }

                //CmdLogFmt("\r\n" );
                ModEntry = FirstModuleEntry(ModList);
                LastName = NULL;

                for ( Mod = 0, ModEntry = FirstModuleEntry(ModList);
                      Mod < ModuleListCount( ModList );
                      Mod++, ModEntry = NextModuleEntry(ModEntry) ) {

                    if ((!!Flat) != (!!ModuleEntryFlat(ModEntry))) {
                        continue;
                    }

                    if (fDoTitle) {
                        if ( Flat ) {
                            CmdLogFmt("    Base               Limit            Name         ");

                            // Display extended load errors?
                            if (fExtendedLoadInfo) {
                                CmdLogFmt("Load Status                  Advanced Status Message" );
                            }

                            CmdLogFmt("\r\n");

                            CmdLogFmt("    ----------------   ---------------- ------------ ");

                            // Display extended load errors?
                            if (fExtendedLoadInfo) {
                                CmdLogFmt("---------------------------- ------------------------------");
                            }

                            CmdLogFmt("\r\n");
                        } else {
                            CmdLogFmt("    Sel   Base              Limit             Seg   Name\r\n" );
                            CmdLogFmt("    ----  ----------------  ----------------  ----  ------------\r\n");
                        }
                        fDoTitle = FALSE;
                    }

                    lpSymName = SHGetSymFName( (HEXE)ModuleEntryEmi(ModEntry) );
                    lpModName = SHGetModNameFromHexe( (HEXE)ModuleEntryEmi(ModEntry) );

                    if ( ModuleEntryFlat(ModEntry) ) {

                        CmdLogFmt( "    %016I64X - %016I64X %-12s ",
                                   ModuleEntryBase(ModEntry),
                                   ModuleEntryBase(ModEntry) + ModuleEntryLimit(ModEntry),
                                   lpModName
                                 );

                        // Get the load status
                        SHSymbolsLoaded((HEXE) ModuleEntryEmi(ModEntry), &sheLoadStatus);

                        // Does the user want extended symbol load info?
                        if (fExtendedLoadInfo) {

                            // Get extended error text
                            lpch = SHLszGetErrorText(sheLoadStatus);
                            if (!lpch) {
                                lpch = "";
                            }

                            sprintf( buf, "(%s)", lpch );
                            CmdLogFmt( "%-28s ", buf );

                            // If the symbols have been loaded, report any INTERESTING load errors
                            if (sheDeferSyms == sheLoadStatus
                                || sheSuppressSyms == sheLoadStatus) {

                                lpch = "";

                            } else {

                                SHSymbolsLoadError((HEXE) ModuleEntryEmi(ModEntry), &sheLoadError);

                                if (sheNone == sheLoadError || sheNoSymbols == sheLoadError) {
                                    lpch = "";
                                } else {
                                    lpch = SHLszGetErrorText(sheLoadError);
                                }
                            }

                            // Prevent us from simply printing ()
                            *buf = 0;
                            if (lpch && *lpch) {
                                sprintf( buf, "(%s)", lpch );
                            }
                            CmdLogFmt( "%-30s ", buf );
                        }

                        // Print the sym/module name
                        CmdLogFmt( "%s", lpSymName );

                        CmdLogFmt("\r\n");

                    } else {


                        if ( !SelSort  &&
                              LastName &&
                              _stricmp( LastName, lpModName )) {

                            CmdLogFmt("\r\n" );
                        }

                        LastName = lpModName;

                        if ( ModuleEntryReal(ModEntry ) ) {
                            CmdLogFmt("    %04x     (Real mode)      %04x  ",
                                      ModuleEntrySelector(ModEntry),
                                      ModuleEntrySegment(ModEntry) );

                        } else {
                            CmdLogFmt("    %04x  %016I64x  %016I64X  %04x  ",
                                      ModuleEntrySelector(ModEntry),
                                      ModuleEntryBase(ModEntry),
                                      ModuleEntryLimit(ModEntry),
                                      ModuleEntrySegment(ModEntry) );
                        }

                        CmdLogFmt( "\t%s", lpModName );

                        CmdLogFmt( "\t%s", lpSymName );

                        CmdLogFmt("\r\n");
                    }
                }
            }

            //
            //  Deallocate module list
            //
            MHFree( ModList );
        }
    }

    *prVal = rVal;
    return Ok;
}


LOGERR
LogListModules(
    LPSTR lpsz,
    BOOL fExtendedLoadInfo
    )
/*++

Routine Description:

    lm "list modules" command

Arguments:

    lpsz  - Supplies pointer to command tail

    fExtendedInfo - TRUE - Display extended load errors
                    FALSE - Don't display extended load errors

Return Value:

    LOGERROR code

--*/
{
    char            ModNameBuffer[ MAX_PATH ];
    char            *ModName;
    LOGERR          rVal     = LOGERROR_NOERROR;
    BOOL            Flat     = FALSE;
    BOOL            Sgm      = FALSE;
    BOOL            SelSort  = FALSE;
    BOOL            Ok;

    CmdInsertInit();
    IsKdCmdAllowed();

    if ( !DebuggeeActive() ) {

        //
        //  No debuggee - nothing to do.
        //
        CmdLogVar(ERR_Debuggee_Not_Alive);
        rVal = LOGERROR_QUIET;

    } else {

        //
        //  Parse arguments
        //
        *ModNameBuffer = '\0';
        if ( lpsz && *lpsz ) {
            lpsz = CPSkipWhitespace(lpsz);
            while ( rVal == LOGERROR_NOERROR && lpsz && *lpsz ) {
                switch (*lpsz) {
                    case '/':
                        lpsz++;
                        switch( *lpsz ) {
                            case 'f':
                            case 'F':
                                if ( !Flat ) {
                                    Flat = TRUE;
                                    lpsz++;
                                }
                                break;

                            case 's':
                            case 'S':
                                if ( !Sgm ) {
                                    Sgm = TRUE;
                                    lpsz++;
                                }
                                break;

                            case 'o':
                            case 'O':
                                if ( !SelSort ) {
                                    SelSort = TRUE;
                                    lpsz++;
                                }
                                break;

                            default:
                                break;
                        }

                        if ( *lpsz && *lpsz != ' ' && *lpsz != '\t' ) {
                            rVal = LOGERROR_UNKNOWN;
                        }
                        break;

                    default:
                        if ( *ModNameBuffer ) {
                            rVal = LOGERROR_UNKNOWN;
                        } else {
                            char *p = ModNameBuffer;
                            while ( *lpsz && *lpsz != ' ' && *lpsz != '\t' ) {
                                if (IsDBCSLeadByte(*lpsz)) {
                                    *p++ = *lpsz++;
                                }
                                *p++ = *lpsz++;
                            }
                            *p++ = '\0';
                        }
                        break;
                }

                lpsz = CPSkipWhitespace(lpsz);
            }
        }

        if ( rVal == LOGERROR_NOERROR ) {

            //
            //  Validate switches
            //
            if ( !Flat && !Sgm ) {
                //
                //  If command did not specify Flat or Sgm, we set
                //  both by default.
                //
                Flat = Sgm = TRUE;
            }

            //
            //  SelSort is valid only if listing segmented modules
            //
            if ( SelSort && !Sgm ) {
                rVal = LOGERROR_UNKNOWN;
            }
        }

        if ( rVal == LOGERROR_NOERROR ) {

            //
            //  Now do the listing
            //
            Ok      = FALSE;
            ModName = *ModNameBuffer ? ModNameBuffer : NULL;

            //
            //  List Segmented modules
            //
            if ( Sgm ) {

                Ok = ListModules(&rVal,
                                 FALSE,
                                 SelSort,
                                 ModName,
                                 fExtendedLoadInfo
                                 );

                if ( !ModName && !Ok ) {
                    CmdLogVar(ERR_NoModulesFound);
                }
            }

            //
            //  List flat modules, unless we are looking for a specific
            //  module and we already found it.
            //
            if ( rVal == LOGERROR_NOERROR && Flat && !( Ok && ModName ) ) {

                Ok = ListModules(
                                 &rVal,
                                 TRUE,
                                 FALSE,
                                 ModName,
                                 fExtendedLoadInfo
                                 );

                if ( !ModName && !Ok ) {
                    CmdLogVar(ERR_NoModulesFound);
                }
            }

            if ( rVal == LOGERROR_NOERROR ) {
                if ( ModName && !Ok ) {
                    CmdLogVar(ERR_ModuleNotFound, ModName);
                }
            }
        }
    }

    return rVal;
}





LOGERR
LogProcess(
    void
    )
/*++

Routine Description:

    Enumerate processes

Arguments:

    None

Return Value:

    LOGERR   code

--*/
{
    LPPD    lppd;

    PST pst;
    WCHAR   Ustr[MAX_PATH];
    PWCHAR  p;
    DWORD   cb;
    ADDR    addr;
    XOSD    xosd;

    CmdInsertInit();

    if (!DebuggeeActive()) {
        CmdLogVar(ERR_Debuggee_Not_Alive);
        return LOGERROR_NOERROR;
    }

    lppd = GetLppdHead();
    Assert(lppd != NULL);

    for ( ;lppd != NULL; lppd = lppd->lppdNext) {

        if (lppd->pstate == psDestroyed) {
            continue;
        }

        if (OSDGetProcessStatus(lppd->hpid, &pst) != xosdNone) {
            CmdLogFmt("No status for process %d\r\n", lppd->ipid);
        } else {
            *Ustr = 0;
            p = Ustr;

            if (lppd->ProcessParameters.ImagePathName.Length > 0 &&
                lppd->ProcessParameters.ImagePathName.Buffer != 0) {

                AddrInit(&addr,
                         0,
                         0,
                         lppd->ProcessParameters.ImagePathName.Buffer,
                         TRUE,
                         TRUE,
                         FALSE,
                         FALSE
                         );

                xosd = OSDReadMemory( lppd->hpid,
                                      NULL,
                                      &addr,
                                      Ustr,
                                      lppd->ProcessParameters.ImagePathName.Length,
                                      &cb
                                      );

                if (xosd == xosdNone && cb > 0) {
                    Ustr[lppd->ProcessParameters.ImagePathName.Length] = 0;
                    p = &Ustr[lppd->ProcessParameters.ImagePathName.Length] - 1;
                    while (p > Ustr) {
                        if (*p == '\\') {
                            ++p;
                            break;
                        } else {
                            --p;
                        }
                    }
                }
            }

            CmdLogFmt("%c%2d  %s  %s  %ls\r\n",
                      lppd == LppdCur? '*' : ' ',
                      lppd->ipid,
                      pst.rgchProcessID,
                      pst.rgchProcessState,
                      p
                      );
        }
    }

    return LOGERROR_NOERROR;
}                   /* LogProcess() */


LOGERR
LogRestart(
    LPSTR lpsz
    )
/*++

Routine Description:

    This routine will restart the debuggee.  If needed it will change
    the command line as well.  If there is no command line present
    then the command line is not changed.

Arguments:

    lpsz    - Supplies new command line to be used

Return Value:

    log error code

--*/
{
    lpsz = CPSkipWhitespace(lpsz);

    CmdInsertInit();

    // Restarting is not allowed on dumps
    if (g_contKernelDbgPreferences_WkSp.m_bUseCrashDump
        || g_contWorkspace_WkSp.m_bUserCrashDump) {

        CmdLogFmt("Neither kernel nor user memory dumps can be restarted\r\n");
        return LOGERROR_QUIET;
    }

    if ( LppdCur && IsProcRunning( LppdCur ) ) {
        CmdLogVar(ERR_Stop_B4_Restart);
        return LOGERROR_QUIET;
    }

    if (g_contWorkspace_WkSp.m_bKernelDebugger && DebuggeeActive()) {
        CmdLogFmt("Target system already running\r\n");
        return LOGERROR_QUIET;
    }

    if (*lpsz) {
        if (LpszCommandLine) {
            free(LpszCommandLine);
        }
        LpszCommandLine = _strdup(lpsz);
    }

    if (!ExecDebuggee(EXEC_RESTART)) {
        return LOGERROR_UNKNOWN;
    }
    return LOGERROR_NOERROR;
}                   /* LogRestart() */


LOGERR
LogStep(
    LPSTR lpsz,
    BOOL fStep
    )
/*++

Routine Description:

    This function is used from the command line to do either a
    step or a trace.  If an argument is present it is assumed
    to be a count for a number of steps to make

Arguments:

    lpsz  - Supplies argument list for step/trace count
    fStep - Supplies TRUE to step or FALSE to trace

Return Value:

    log error code

--*/
{
    int     cStep;
    int     err;
    int     cch;
    CXF     cxf = CxfIp;
    LPPD    LppdT;
    LPTD    LptdT;
    static BOOL    fRegisters = FALSE;
    LOGERR  rVal = LOGERROR_NOERROR;


    CmdInsertInit();
    if (g_contWorkspace_WkSp.m_bKernelDebugger && g_contKernelDbgPreferences_WkSp.m_bUseCrashDump) {
        CmdLogFmt( "Steps are not allowed for crash dumps\n" );
        return rVal;
    }

    // NOTENOTE a-kentf thread wildcard is supposed to be valid here (LogStep)
    TDWildInvalid();
    PDWildInvalid();


    PreRunInvalid();

    /*
     *  Check for no argument -- then just do a single step or trace
     */

    if (*lpsz == 'r') {
        fRegisters = !fRegisters;
        lpsz++;
    }

    lpsz = CPSkipWhitespace(lpsz);

    cStep = 1;

    if (*lpsz != 0) {
        cStep = (int) CPGetInt(lpsz, &err, &cch);
        if (err || cStep < 1) {
            CmdLogVar(ERR_Bad_Count);
            return LOGERROR_QUIET;
        }
    }

    /*
    **  If the debuggee is not loaded then we need to get it loaded so that
    **  we have symbols that can be evaluated
    */
    if (!DebuggeeAlive()) {
        if (!ExecDebuggee(EXEC_RESTART)) {
            CmdLogVar(ERR_Cant_Start_Proc);
            return LOGERROR_QUIET;
        }
        LppdCommand = LppdCur;
        LptdCommand = LppdCur->lptdList;
    }

    /*
     * make sure thread is now runnable
     */

    if (!StepOK(LppdCommand, LptdCommand)) {
        NoRunExcuse(LppdCommand, LptdCommand);
        return LOGERROR_QUIET;
    }

    LppdT = LppdCur;
    LptdT = LptdCur;

    if (LptdCur != LptdCommand) {
        LppdCur = LppdCommand;
        LptdCur = LptdCommand;
        UpdateDebuggerState(UPDATE_CONTEXT);
    }

    if (ExecDebuggee( fStep ? EXEC_STEPOVER : EXEC_TRACEINTO ) == 0) {
        if (LptdCur) {
            LptdCur->cStepsLeft = 0;
        }
        rVal = LOGERROR_UNKNOWN;
    } else {
        Assert(LptdCur);
        LptdCur->cStepsLeft = cStep - 1;
        LptdCur->flags &= ~tfStepOver;
        LptdCur->flags |= (fStep ? tfStepOver : 0);
        LptdCur->fRegisters = fRegisters;
        LptdCur->fDisasm = fRegisters;
        rVal = LOGERROR_NOERROR;
    }

    if (LptdT != LptdCur) {
        LppdCur = LppdT;
        LptdCur = LptdT;
        UpdateDebuggerState(UPDATE_CONTEXT);
    }

    return rVal;
}                                       /* LogStep() */


LOGERR
LogThread(
    void
    )
/*++

Routine Description:

    Enumerate threads

Arguments:

    None

Return Value:

    log error code

--*/
{
    LPPD lppd;

    CmdInsertInit();

    if (!DebuggeeActive()) {
        CmdLogVar(ERR_Debuggee_Not_Alive);
        return LOGERROR_QUIET;
    }

    if (LppdCommand == NULL) {

        CmdLogVar(ERR_No_Threads);

    } else if (LppdCommand != (LPPD)-1) {

        ThreadStatForProcess(LppdCommand);

    } else {
        for (lppd = GetLppdHead(); lppd; lppd = lppd->lppdNext) {
            if (lppd->pstate == psDestroyed) {
                continue;
            }
            CmdLogFmt("\r\nProcess %d:\r\n", lppd->ipid);
            ThreadStatForProcess(lppd);
        }
    }

    return LOGERROR_NOERROR;
}                   /* LogThread() */


LOGERR
LogLoadDefered(
    LPSTR lpsz
    )
/*++

Routine Description:

    Loads defered symbols for a module

Arguments:

    lpsz    - Supplies new command line to be used

Return Value:

    log error code

--*/
{
    LOGERR      LogErr = LOGERROR_QUIET;
    HEXE        hexe;
    SHE         she;

    CmdInsertInit();

    if (!DebuggeeActive()) {
        CmdLogVar(ERR_Debuggee_Not_Alive);
        return LOGERROR_QUIET;
    }

    lpsz = CPSkipWhitespace(lpsz);

    if ( *lpsz == 0 ) {

        CmdLogFmt( "Load: Must specify module name\r\n" );

    } else {

        hexe = SHGethExeFromName( lpsz );

        if ( hexe ) {

            SHSymbolsLoaded(hexe, &she);

            if (she == sheDeferSyms ) {
                SHWantSymbols( hexe );
            }

        } else {

            CmdLogFmt( "Load: Could not load %s\r\n", lpsz );
        }
    }

    return LogErr;
}


LOGERR
LogWatchTime(
    LPSTR lpsz
    )
/*++

Routine Description:

    The WT command generates a call tree for all of the code run
    under a call site.

Arguments:

    lpsz - Supplies command tail

Return Value:

    log error code

--*/
{
    LOGERR          LogErr = LOGERROR_QUIET;
    int             cch;
    ADDR            addr;
    ADDR            addr1;
    CXT             cxt;
    HDEP            hsyml;
    PHSL_HEAD       lphsymhead;
    PHSL_LIST       lphsyml;
    UINT            n;
    DWORDLONG       dwOff;
    DWORDLONG       dwOffP;
    DWORDLONG       dwOffN;
    DWORDLONG       dwAddrP;
    DWORDLONG       dwAddrN;
    HSYM            hsymP;
    HSYM            hsymN;
    HDEP            hstr;
    EESTATUS        eest;
    char            szContext[MAX_USER_LINE];
    char            szStr[MAX_USER_LINE];
    PIOCTLGENERIC   pig;
    LPBYTE          lpb;
    DWORD           dw;


    CmdInsertInit();

    lpsz = CPSkipWhitespace(lpsz);

    if (_stricmp(lpsz,"stop")==0) {

        pig = (PIOCTLGENERIC) malloc( sizeof(IOCTLGENERIC) );
        if (!pig) {
            CmdLogFmt("Could not allocate memory for wt command\r\n");
            return LOGERROR_QUIET;
        }

        pig->ioctlSubType = IG_WATCH_TIME_STOP;
        pig->length = 0;

        OSDSystemService( LppdCur->hpid,
                          LptdCur->htid,
                          (SSVC) ssvcGeneric,
                          (LPV)pig,
                          sizeof(IOCTLGENERIC),
                          &dw
                          );

        free( pig );

        return LogErr;
    }

    //
    // first lets do an effective loglistnear command
    //

    if (CPGetAddress(".",
                      &cch,
                      &addr,
                      16,
                      &CxfIp,
                      FALSE,
                      g_contWorkspace_WkSp.m_bMasmEval) != CPNOERROR) {
        CmdLogFmt("Could not find a symbol for current location\r\n");
        return LOGERROR_QUIET;
    }

    ZeroMemory(&cxt, sizeof(cxt));
    SHSetCxt(&addr, &cxt);

    hsyml = 0;
    eest = EEGetHSYMList(&hsyml, &cxt, HSYMR_public, NULL, TRUE);
    if (eest != EENOERROR) {
        CmdLogFmt("Could not find a symbol for current location\r\n");
        return LOGERROR_QUIET;
    }

    lphsymhead = (PHSL_HEAD) MMLpvLockMb ( hsyml );
    lphsyml = (PHSL_LIST)(lphsymhead + 1);

    dwOffP = (DWORDLONG)(LONGLONG)-1;
    dwOffN = (DWORDLONG)(LONGLONG)-1;
    dwAddrP = (DWORDLONG)(LONGLONG)-1;
    dwAddrN = (DWORDLONG)(LONGLONG)-1;
    hsymP = 0;
    hsymN = 0;

    SYFixupAddr(&addr);
    addr1 = addr;
    for ( n = 0; n < (UINT)lphsyml->symbolcnt; n++ ) {

        if (SHAddrFromHsym(&addr1, lphsyml->hSym[n])) {
            SYFixupAddr(&addr1);
            if (GetAddrSeg(addr1) != GetAddrSeg(addr)) {
                continue;
            }
            if (GetAddrOff(addr1) <= GetAddrOff(addr)) {
                dwOff = GetAddrOff(addr) - GetAddrOff(addr1);
                if (dwOff < dwOffP) {
                    dwOffP = dwOff;
                    dwAddrP = GetAddrOff(addr1);
                    hsymP = lphsyml->hSym[n];
                }
            } else {
                dwOff = GetAddrOff(addr1) - GetAddrOff(addr);
                if (dwOff < dwOffN) {
                    dwOffN = dwOff;
                    dwAddrN = GetAddrOff(addr1);
                    hsymN = lphsyml->hSym[n];
                }
            }
        }

    }

    MMbUnlockMb(hsyml);
    MMFreeHmem(hsyml);

    if (!hsymP || !hsymN) {
        CmdLogFmt("Could not find a symbol for current location\r\n");
        return LOGERROR_QUIET;
    }

    EEFormatCXTFromPCXT( &cxt, &hstr, g_contWorkspace_WkSp.m_bShortContext );
    if (g_contWorkspace_WkSp.m_bShortContext) {
        strcpy( szContext, (LPSTR)MMLpvLockMb(hstr) );
    } else {
        BPShortenContext( (LPSTR)MMLpvLockMb(hstr), szContext );
    }
    MMbUnlockMb(hstr);
    EEFreeStr(hstr);
    FormatHSym(hsymP, &cxt, szStr);
    strcat(szContext,szStr);

    //
    // now notify the dm that the wt command should start now
    //

    n = (2 * sizeof(DWORDLONG)) + strlen(szContext) + 1;

    pig = (PIOCTLGENERIC) malloc( n + sizeof(IOCTLGENERIC) );
    if (!pig) {
        CmdLogFmt("Could not allocate memory for wt command\r\n");
        return LOGERROR_QUIET;
    }

    pig->ioctlSubType = IG_WATCH_TIME;
    pig->length = n;

    lpb = (LPBYTE) pig->data;
    *(PDWORDLONG)lpb = dwAddrP;
    lpb += sizeof(DWORD);
    *(PDWORDLONG)lpb = dwAddrN - 1;
    lpb += sizeof(DWORD);
    strcpy( (PSTR) lpb, szContext);

    OSDSystemService( LppdCur->hpid,
                      LptdCur->htid,
                      (SSVC) ssvcGeneric,
                      (LPV)pig,
                      n + sizeof(IOCTLGENERIC),
                      &dw
                      );

    free( pig );

    return LogErr;
}

LOGERR
LogUnload(
    LPSTR lpsz,
    DWORD dw
    )
{
    HEXE emi;
    HPDS HpdsOld;

    CmdInsertInit();

    if (LppdCur == NULL) {
        CmdLogVar(ERR_Debuggee_Not_Alive);
        return LOGERROR_QUIET;
    }

    //
    //  Skip over any leading blanks
    //

    lpsz = CPSkipWhitespace(lpsz);

    if (!*lpsz) {
        CmdLogFmt("Unload: Must specify a module name\r\n");
        return LOGERROR_QUIET;
    }

    //
    //  Get the symbol handle for the desired module
    //

    emi = SHGethExeFromName(lpsz);

    HpdsOld = SHChangeProcess(LppdCur->hpds);

    //
    // unloading user mode modules in kernel mode
    // can result in nameless emis
    //
    ModListModUnload( lpsz );

    if (SHIsDllLoaded(emi) && lpsz) {
        CmdLogFmt("Module Unload: %s\r\n", lpsz);
    }

    //
    //  A module has been unloaded. We must unresolve all
    //  the breakpoints in the module.
    //
    if (LppdCur->pstate == psRunning) {
        BPTUnResolve( emi );
    }

    //
    // Unload the symbols
    //

    SHUnloadSymbols(emi);

    //
    // Rebind the emi to the module record, but don't update the debugger
    // since doing so may cause the symbols to reload instantly.
    //

    OSDRegisterEmi(LppdCur->hpid, (HEMI)emi, "");

    CmdLogFmt("Use \".refresh\" or \".reload\" to update the debugger");

    //BPTResolveAll(LppdCur->hpid, FALSE);
    //UpdateDebuggerState(UPDATE_CONTEXT);

    SHChangeProcess(HpdsOld);

    return LOGERROR_NOERROR;
}

LOGERR
LogRefresh(
    LPSTR lpsz,
    DWORD dw
    )
{

    CmdInsertInit();

    if (LppdCur == NULL) {
        CmdLogVar(ERR_Debuggee_Not_Alive);
        return LOGERROR_QUIET;
    }

        BPTResolveAll(LppdCur->hpid, FALSE);
        UpdateDebuggerState(UPDATE_ALLDBGWIN);
    return LOGERROR_NOERROR;
}
