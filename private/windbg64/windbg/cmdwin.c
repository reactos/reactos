/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    cmdwin.c

Abstract:

    This file contains the window procedure code for dealing with the
    command window.  This window uses the document manager to deal with
    keeping track of the characters in the buffer.

    This window has the following strange properties:

           It is read-only except on the last line
           The first portion of the last line is read-only

Author:

    Jim Schaad (jimsch)

Environment:

    Win32 - User

--*/
/************************** INCLUDE FILES *******************************/

#include "precomp.h"
#pragma hdrstop

#include <ime.h>

extern LPSHF    Lpshf;
extern CXF      CxfIp;


/************************** Externals *************************/

BOOL            FCmdDoingInput;

extern BOOL    fWaitForDebugString;
extern LPSTR   lpCmdString;
extern ADDR    addrLastDisasmStart;

/************************** Internal Prototypes *************************/
/************************** Data declaration    *************************/

static BOOL FAutoRunSuppress  = FALSE;

PCTRLC_HANDLER pPolledCtrlCHandler = NULL;
BOOL fCtrlCPressed     = FALSE; // Ctrl-C pressed flag for EditWndProc

static PCTRLC_HANDLER PcchHead = NULL;

static BOOL fEatEOLWhitespace = TRUE;  // eat space at end of line
static BOOL FAutoHistOK       = FALSE;

static char szOldPrompt[PROMPT_SIZE];

/**************************       Code          *************************/

void NEAR PASCAL SelPosXY (int view, int X, int Y);


void
BPCallbackHbpt(
    HBPT hBpt,
    BPSTATUS bpstatus
    )
/*++

Routine Description:

    This function will be called for each breakpoint which
    mets its criteria

Arguments:

    hBpt     - Supplies breakpoint which met the breakpoint criteria
    bpstatus - Supplies breakpoint status code during evaluation

Return Value:

    None

--*/
{
    ULONG       i;
    char        rgchT[256];

    if (bpstatus != BPNOERROR) {
        Dbg( BPIFromHbpt( &i, hBpt ) == BPNOERROR);

        CmdInsertInit();
        CmdLogFmt("Error checking breakpoint #%d\r\n", i);

        return;
    }

    if (LptdCur->fInFuncEval == FALSE) {
        Dbg( BPIFromHbpt( &i, hBpt) == BPNOERROR );

        CmdInsertInit();
        CmdLogFmt("Breakpoint #%d hit\r\n", i);

        Dbg( BPQueryCmdOfHbpt( hBpt, rgchT, sizeof(rgchT)-1) == BPNOERROR );

        if (*rgchT != 0) {
            CmdPrependCommands(LptdCur, rgchT);
        }
    }

    return;
}                                       /* BPCallbackHbpt() */

BOOL
DoStopEvent(
    LPTD lptd
    )
/*++

Routine Description:

    This routine is called whenever a thread stops.  It handles any special
    processing that should be done when a thread stops, such as print the
    registers or executing special commands associated with stopping.

Arguments:

    lptd - Supplies the thread that stopped

Return Value:

    TRUE if the caller should be silent.
    FALSE if normal processing should continue, and normal messages
        should be printed.

--*/
{
    if (*szStopEventCmd) {
        CmdPrependCommands(lptd, szStopEventCmd);
    }

    if (lptd && lptd->fDisasm && !AutoTest) {
        CmdPrependCommands(lptd, "u . l1");
        lptd->fDisasm = FALSE;
    }

    if (lptd && lptd->fRegisters) {
        CmdPrependCommands(lptd, "r");
        lptd->fRegisters = FALSE;
    }

    //
    // forget where we disassembled last, so that "u" will default to
    // the current IP.
    //
    addrLastDisasmStart.addr.seg = 0;
    addrLastDisasmStart.addr.off = 0;

    return FALSE;
}


BOOL
CmdHandleInputString(
    LPSTR lpsz
    )
{
    CmdSetDefaultPrompt( szOldPrompt );
    CmdSetDefaultCmdProc();
    OSDInfoReply(LppdCur->hpid, LptdCur->htid, lpsz, strlen(lpsz) + 1);
    return TRUE;
}

VOID
GetPointersFromQueue(
    HPID * phpid,
    HTID * phtid,
    LPPD * plppd,
    LPTD * plptd
    )
{
    *plptd = (LPTD) GetQueueItemQuad();
    if (*plptd != 0) {
        *plppd = (*plptd)->lppd;
        *phtid = (*plptd)->htid;
        *phpid = (*plppd)->hpid;
    } else {
        *phpid = (HPID)GetQueueItemQuad();
        *phtid = (HTID)GetQueueItemQuad();
        *plppd = LppdOfHpid(*phpid);
        Assert(*plppd);
        *plptd = LptdOfLppdHtid(*plppd, *phtid);
    }
}



BOOL
CmdExecNext(
    DBC        unusedIW,
    LPARAM     unusedL
    )
/*++

Routine Description:

    This function causes two things to occur.  First the command window
    is updated to reflect the command/event which just occurred and
    secondly the command processor state machine is consulted to see
    if there are any more commands to be executed from the command
    window.

Arguments:

    dbcCallback     - Supplies the call back which caused this update to occur
    lParam          - Supplies information about the callback

Return Value:

    TRUE if update the screen and FALSE otherwise

--*/
{
    long        lExit;
    LPSTR       lpch;
    LPSTR       lpsz;
    long        l;
    DWORD       dw;
    int         dbcCallback;

    HEXE        emi;
    SHE         she;
    BPSTATUS    bpstatus;
    XOSD        xosd;
    CXT         cxt;
    ADDR        addr;
    DWORD64     qwNotify;
    EXOP        exop;
    HPDS        HpdsOld;

    HPID        hpid;
    HTID        htid;
    LPPD        lppd;
    LPTD        lptd;

    LPPD        LppdT;
    LPTD        LptdT;
    BOOL        fExecNext;
    int         bfRefresh;
    BOOL        fGetFocus;
    BOOL        fRunning;
    BOOL        fReply;
    int         iter = 0;
    BOOL        ThreadIsStopped;

    static int  fRecurse = FALSE;

    extern BOOL FKilling;

    if (fRecurse) {
        return TRUE;
    }

    fRecurse = TRUE;

    //
    // This call will AV if
    //
    __try {
        while (GetQueueLength() != 0) {

            if (iter++ == 10) {
                fRecurse = FALSE;
                PostMessage(hwndFrame, DBG_REFRESH, 0, 0);
                return FALSE;
            }


            /*
             * Inititalization of variables;
             */

            LppdT = LppdCur;
            LptdT = LptdCur;
            fExecNext = TRUE;
            bfRefresh = UPDATE_ALLDBGWIN;
            fGetFocus = FALSE;

            /*
             * Get the next command to be processed
             */

            dbcCallback = (int) GetQueueItemQuad();

    #if 0
    // BUGBUG -- jls Need to put in new code to deal with this problem.
            if ((DbgState == ds_error) && (dbcCallback != dbcInfoAvail)) {
                fRecurse = FALSE;
                return FALSE;
            }
    #endif

            /*
             *  Set up the command window to allow for doing input.
             */

            CmdInsertInit();

            /*
             *
             */

            switch ((DBC)dbcCallback) {

            case dbcError:             /* DBG_INFO */
                bfRefresh = UPDATE_NONE;
                hpid = (HPID)GetQueueItemQuad();
                htid = (HTID)GetQueueItemQuad();
                xosd = (XOSD)GetQueueItemQuad();
                lpsz = (LPSTR)GetQueueItemQuad();

                LppdCur = LppdOfHpid(hpid);
                LptdCur = LptdOfLppdHtid(LppdCur, htid);

                CmdLogFmt("ERROR - %s\r\n", lpsz);
                MHFree(lpsz);

                fGetFocus = TRUE;
                fExecNext = FALSE;

                if (LppdCur->pstate == psPreRunning) {
                    /*
                     *  we aren't going to get the breakpoint...
                     */

                    LppdCur->pstate = psRunning;
                    for (lptd = LppdCur->lptdList; lptd; lptd = lptd->lptdNext) {
                        lptd->tstate = tsRunning;
                    }
                    Dbg( BPFreeHbpt( (HBPT)LppdCur->hbptSaved ) == BPNOERROR) ;
                    LppdCur->hbptSaved = NULL;
                    BPTResolveAll(LppdCur->hpid,TRUE);
                    SetProcessExceptions(LppdCur);
                    VarMsgBox(NULL, DBG_Attach_Deadlock,
                              MB_OK | MB_ICONINFORMATION|MB_SETFOREGROUND|MB_TASKMODAL);
                }

                // Any time we update the context we must invalidate the Watch window.
                bfRefresh = UPDATE_CONTEXT | UPDATE_ALLDBGWIN | UPDATE_SYMBOLS_CHANGED;
                break;




            case dbcLoadComplete:
                lptd = (LPTD) GetQueueItemQuad();
                Assert(lptd != NULL);

                qwNotify = GetQueueItemQuad();

                LptdCur = lptd;
                LppdCur = lptd->lppd;

                Assert(LppdCur->pstate == psPreRunning);

                LptdCur->tstate = tsStopped;

                // Any time we update the context we must invalidate the Watch window.
                UpdateDebuggerState(UPDATE_CONTEXT | UPDATE_SYMBOLS_CHANGED | UPDATE_WATCH);

                BPTResolveAll(LppdCur->hpid,TRUE);

                /*
                 *  The process is no longer "PreRunning",
                 * but the thread is.
                 */

                LppdCur->pstate = psStopped;

                SetProcessExceptions(LppdCur);

    #ifdef SHOW_MAGIC
                CmdLogFmt("(Process loaded)\r\n");
    #endif
                fExecNext = FALSE;
                bfRefresh  = UPDATE_NONE;

                /*
                 * Handle go/stop for child here.
                 * For attachee, it is handled in AttachDebuggee()
                 */

                if (LppdCur->fChild) {
                    if (g_contWorkspace_WkSp.m_bChildGo) {
                        Go();
                    } else {
                        DoStopEvent(lptd);
                        lptd->lppd->fStopAtEntry = FALSE;
                        fGetFocus = TRUE;
                        fExecNext = TRUE;
                        bfRefresh  = UPDATE_ALLDBGWIN;
                    }
                }

                break;


            case dbcEntryPoint:

                lptd = (LPTD) GetQueueItemQuad();
                Assert(lptd != NULL);

                qwNotify = GetQueueItemQuad();

                LptdCur = lptd;
                lppd =
                LppdCur = lptd->lppd;

                LptdCur->tstate = tsStopped;

                UpdateDebuggerState(UPDATE_CONTEXT);


                fRunning = FALSE;

                if (lppd->fInitialStep) {

                    // If stepping in source mode,
                    // we will land here.

                    // If we are in "regular" mode, try
                    // to find main or WINMAIN or whatever.

                    if (!g_contWorkspace_WkSp.m_bEPIsFirstStep) {
                        cxt = *SHpCXTFrompCXF(&CxfIp);
                        fRunning = get_initial_context(&cxt, FALSE);
                        //fRunning = get_initial_context(&cxt, TRUE);
                        if (fRunning) {
                            SYFixupAddr(SHpAddrFrompCxt(&cxt));
                            //
                            //v-vadimp - skip the prolog on ia64
                            //
                            // kentf - I think this is the wrong place to do this,
                            //         and you probably shouldn't get the focus
                            //         until later.
                            //
                            if(LppdCur->mptProcessorType == mptia64) {
                                while( SHIsInProlog( &cxt ) ) {
                                    cxt.addr.addr.off++;
                                }
                            }
                            GoUntil( SHpAddrFrompCxt(&cxt) );
                        } else {
                            VarMsgBox (NULL,
                                       ERR_NoSymbols,
                                       MB_OK|MB_ICONINFORMATION|MB_SETFOREGROUND|MB_TASKMODAL);
                        }
                        fGetFocus = TRUE;
                    }

                } else if (lppd->hbptSaved) {

                    if (!get_initial_context(SHpCXTFrompCXF(&CxfIp), FALSE)) {
                        get_initial_context(SHpCXTFrompCXF(&CxfIp), TRUE);
                    }
                    if (BPBindHbpt( (HBPT)lppd->hbptSaved, &CxfIp ) != BPNOERROR) {
                        Dbg( BPFreeHbpt( (HBPT)lppd->hbptSaved ) == BPNOERROR) ;
                        lppd->hbptSaved = NULL;
                        CmdLogVar(ERR_Unable_To_Complete_Gountil);
                    } else {
                        Dbg( BPAddrFromHbpt( (HBPT)lppd->hbptSaved, &addr ) == BPNOERROR) ;
                        Dbg( BPFreeHbpt( (HBPT)lppd->hbptSaved ) == BPNOERROR) ;
                        lppd->hbptSaved = NULL;

                        fRunning = TRUE;
                        GoUntil(&addr);
                    }

                } else if (!lppd->fStopAtEntry) {

                    // if stepping in ASM mode, fStopAtEntry
                    // is like a safety BP to stop running after
                    // leaving the loader APC.

                    fRunning = TRUE;
                    Go();
                }

                if (fRunning) {
                    fExecNext = FALSE;
                    bfRefresh = UPDATE_NONE;
                    LppdCur = LppdT;
                    LptdCur = LptdT;
                } else {
                    BPClearAllTmp( lppd->hpid, lptd->htid );
                    if (!DoStopEvent(lptd)) {
                        CmdLogVar(DBG_At_Entry_Point);
                    }
                }

                lppd->fInitialStep = FALSE;
                lppd->fStopAtEntry = FALSE;

                break;



            case dbcCheckBpt:

                Assert(!"dbcCheckBpt should not get here.");
                break;



            case dbcBpt:               /* DBG_REFRESH */

                lptd = (LPTD) GetQueueItemQuad();
                Assert(lptd != NULL);

                qwNotify = GetQueueItemQuad();

                LptdCur = lptd;
                LppdCur = lptd->lppd;

                LptdCur->tstate = tsStopped;

                UpdateDebuggerState(UPDATE_CONTEXT);

                if ( FKilling || !DebuggeeActive() ) {
                    LppdCur = LppdT;
                    LptdCur = LptdT;
                } else {

                    bpstatus = BPCheckHbpt( CxfIp, BPCallbackHbpt,
                                    lptd->lppd->hpid, lptd->htid, qwNotify );

                    if (bpstatus == BPNoBreakpoint) {

                        BPClearAllTmp( lptd->lppd->hpid, lptd->htid );

                        DoStopEvent(lptd);
                        lptd->lppd->fStopAtEntry = FALSE;
                        fGetFocus = TRUE;

                        CmdLogVar(DBG_Hard_Coded_Breakpoint);

                    } else if (bpstatus == BPPassBreakpoint) {

                        /*
                         *  -- do a go as no breakpoint was matched
                         */


                        //Assert(!"Unexpected BPPassBreakpoint");

                        fExecNext = FALSE;
                        bfRefresh = UPDATE_NONE;
                        Go();
                        LppdCur = LppdT;
                        LptdCur = LptdT;

                    } else {

                        BPClearAllTmp( lptd->lppd->hpid, lptd->htid );
                        DoStopEvent(lptd);
                        lptd->lppd->fStopAtEntry = FALSE;
                        fGetFocus = TRUE;
                    }
                }

                break;



            case dbcAsyncStop:         /* DBG_REFRESH */

                fExecNext = FALSE;


            case dbcStep:
                bfRefresh = UPDATE_ALLDBGWIN | UPDATE_CONTEXT;

                lptd = (LPTD) GetQueueItemQuad();
                Assert(lptd != NULL);

                qwNotify = GetQueueItemQuad();

                LptdCur = lptd;
                LppdCur = lptd->lppd;
                SetPTState(psInvalidState, tsStopped);
                BPClearAllTmp( lptd->lppd->hpid, lptd->htid );
                if (!DoStopEvent(lptd)) {
                    if (dbcCallback == dbcAsyncStop) {
                        CmdLogFmt("Stopped in process %d, thread %d\r\n",
                                  lptd->lppd->ipid,
                                  lptd->itid);
                    }
                }
                if (dbcCallback == dbcAsyncStop) {
                    LptdCur->cStepsLeft = 0;
                }
                lptd->lppd->fStopAtEntry = FALSE;
                fGetFocus = TRUE;
                break;



            case dbcModLoad:           /* DBG_INFO */

                lptd = (LPTD)GetQueueItemQuad();

                if (lptd == NULL) {
                    //
                    // this is the first modload, which is really
                    // part of the process creation.  The thread isn't
                    // here yet.
                    //
                    hpid = (HPID)GetQueueItemQuad();
                    htid = (HTID)GetQueueItemQuad();
                    lppd = LppdOfHpid(hpid);
                    lptd = LptdOfLppdHtid(lppd, htid);

                } else {

                    lppd = lptd->lppd;
                    hpid = lppd->hpid;
                    htid = lptd->htid;
                }

                lpsz = (LPSTR)GetQueueItemQuad();
                ThreadIsStopped = (BOOL)GetQueueItemQuad();

                Assert(lppd);
                Assert(lpsz);

                //
                // Any time we update the context we must invalidate the
                // Watch window.
                //

                bfRefresh = UPDATE_ALLDBGWIN | UPDATE_SYMBOLS_CHANGED;

                //
                // set the SH to this process, load the symbols,
                // tell the EM, and reset the SH.
                //

                HpdsOld = SHChangeProcess(lppd->hpds);

                SetFindExeBaseName(lppd->lpBaseExeName);
                she = SHLoadDll( lpsz, TRUE );
                emi = SHGethExeFromName((char FAR *) lpsz);
                Assert(emi != 0);
                OSDRegisterEmi( hpid, (HEMI)emi, lpsz);

                MHFree(lpsz);

                SHChangeProcess(HpdsOld);

                //
                // update the modload status report
                //
                ModListModLoad( SHGetExeName( emi ), she );

                //
                // If this is the first modload, remember it as the base exe
                //

                if (!lppd->lpBaseExeName) {
                    lppd->lpBaseExeName = MHStrdup(SHGetExeName( emi ));
                }

                //
                // spew to the cmdwin
                //

                if (she != sheSuppressSyms && g_contWorkspace_WkSp.m_bVerbose) {
                    lpch = SHLszGetErrorText(she);
                    if (she == sheNoSymbols) {
                        CmdLogFmt("Module Load: %s", SHGetExeName( emi ));
                    } else {
                        CmdLogFmt("Module Load: %s", SHGetSymFName( emi ));
                    }
                    if (lpch) {
                        CmdLogFmt("  (%s)", lpch);
                    }
                    CmdLogFmt("\r\n");
                }

                if (lppd->pstate != psPreRunning) {
                    //
                    //  A new module has been loaded. If we are not loading
                    //  the statically-linked DLLs of the debuggee, we must
                    //  try to resolve any unresolved breakpoints.
                    //
                    if (BPTIsUnresolvedCount( hpid )) {
                        LptdCur = lptd;
                        LppdCur = lppd;

                        UpdateDebuggerState( bfRefresh );

                        BPTResolveAll( hpid, FALSE );
                        LppdCur = LppdT;
                        LptdCur = LptdT;
                        bfRefresh |= UPDATE_CONTEXT;
                    }
                }

                //
                // Continue the thread or stay stopped:
                //

                if (!ThreadIsStopped) {

                    fExecNext = FALSE;
                } else if (g_contWorkspace_WkSp.m_bGoOnModLoad || lptd == NULL) {

                    fExecNext = FALSE;
                    OSDGo(hpid, htid, &exop);

                } else {

                    LptdCur = lptd;
                    LppdCur = lppd;

                    SetPTState(psInvalidState, tsStopped);
                    fExecNext = TRUE;
                    fGetFocus = TRUE;
                }

                // When debugging memory dumps, the loading of the kernel
                // needs to be synchronous, else data structures in the DM
                // will not be initialized to sane values.
                if ( g_contWorkspace_WkSp.m_bKernelDebugger 
                    && g_contKernelDbgPreferences_WkSp.m_bUseCrashDump) {

                    TCHAR szFName[_MAX_FNAME];
                    TCHAR szExt[_MAX_EXT];

                    _tsplitpath(lppd->lpBaseExeName, 0, 0, szFName, szExt);

                    if (!_tcsicmp(szExt, _T(".EXE"))
                        && ( 
                            !_tcsicmp(szFName, _T("NTOSKRNL")) ||
                            !_tcsicmp(szFName, _T("NTKRNLMP")) 
                           )
                        ) {

                        OSDSignalKernelLoadCompleted(hpid);
                    }
                }

            
                break;

            case dbcModFree:           /* DBG_REFRESH */
                //
                // Any time we update the context we must invalidate the
                // Watch window.
                //
                bfRefresh = UPDATE_CONTEXT | UPDATE_SYMBOLS_CHANGED | UPDATE_WATCH;
                lptd = (LPTD)GetQueueItemQuad();

                if (lptd == NULL) {
                    hpid = (HPID)GetQueueItemQuad();
                    htid = (HTID)GetQueueItemQuad();
                    lppd = LppdOfHpid(hpid);
                    lptd = LptdOfLppdHtid(lppd, htid);
                } else {
                    lppd = lptd->lppd;
                    hpid = lppd->hpid;
                    htid = lptd->htid;
                }

                Assert(lppd);
                Assert(lptd);

                emi = (HEXE) GetQueueItemQuad();
                Assert(emi != 0);

                HpdsOld = SHChangeProcess(lppd->hpds);

            
                lpsz = SHGetExeName( emi );

                //
                // unloading user mode modules in kernel mode
                // can result in nameless emis
                //
                if (lpsz && *lpsz) {
                    ModListModUnload( SHGetExeName( emi ) );
                }

                if (g_contWorkspace_WkSp.m_bVerbose && !ExitingDebugger) {
                    if (SHIsDllLoaded(emi) && lpsz) {
                        CmdLogFmt("Module Unload: %s\r\n", lpsz);
                    }
                }

                //
                //  A module has been unloaded. We must unresolve all
                //  the breakpoints in the module.
                //
                if (lppd->pstate == psRunning) {
                    BPTUnResolve( emi );
                }

                OSDUnRegisterEmi(hpid, (HEMI) emi);
                SHUnloadDll( emi );

                fExecNext = FALSE;
                break;



            case dbcCreateThread:      /* DBG_INFO */

                bfRefresh = UPDATE_NONE;
                fExecNext = FALSE;

                Dbg(GetQueueItemQuad() == 0);

                hpid = (HPID)GetQueueItemQuad();
                htid = (HTID)GetQueueItemQuad();

                lppd = LppdOfHpid(hpid);

                if (!lppd && !LppdFirst) {

                    //
                    // when blowing off the debuggee, some stray events
                    // may be in this queue...
                    //

                    break;
                }
                Assert(lppd);

                lptd = CreateTd(lppd, htid);
                Assert(lptd);

                LppdT = LppdCur = lppd;
                LptdT = LptdCur = lptd;

                if (g_contWorkspace_WkSp.m_bNotifyThreadCreate) {
                    CmdLogFmt("Thread Create:  Process=%d, Thread=%d\r\n",
                              lptd->lppd->ipid, lptd->itid);
                }

                //
                // Don't do the update and BPTResolveAll if this is a
                // thread which is being created when we are already
                // stopped at a debug event.
                //

                if (!IsProcStopped(lppd)) {

                    UpdateDebuggerState( UPDATE_CONTEXT );

                    if (LppdCur->pstate == psRunning) {
                        BPTResolveAll( hpid, TRUE );
                    }

                    SetPTState(psInvalidState, tsStopped);
                    Go();
                }

                break;



            case dbcThreadTerm:        /* DBG_REFRESH */

                lptd = (LPTD) GetQueueItemQuad();
                if ( lptd == 0) {
                    hpid = (HPID)GetQueueItemQuad();
                    htid = (HTID)GetQueueItemQuad();
                    lppd = LppdOfHpid(hpid);
                    lptd = LptdOfLppdHtid(lppd, htid);
                }
                lExit = (long) GetQueueItemQuad();

                if (!lptd && !LppdFirst) {

                    //
                    // when blowing off the debuggee, some stray events
                    // may be in this queue...
                    //

                    break;
                }

                Assert(lptd != NULL);
                lppd = lptd->lppd;

                LppdCur = lppd;
                LptdCur = lptd;
                SetPTState(psInvalidState, tsExited);

                if (g_contWorkspace_WkSp.m_bNotifyThreadTerm) {
                    CmdLogFmt("Thread Terminate:  Process=%d, Thread=%d, Exit Code=%ld\r\n",
                              lptd->lppd->ipid, lptd->itid, lExit);
                }

                if (lptd->fInFuncEval) {
                    fExecNext = FALSE;
                    LptdFuncEval = NULL;
                }

                BPTUnResolvePidTid( lppd->hpid, lptd->htid );

                if (lptd->fGoOnTerm ||
                    g_contWorkspace_WkSp.m_bGoOnThreadTerm ||
                    !g_contWorkspace_WkSp.m_bNotifyThreadTerm) {
                    Go();
                    fExecNext = FALSE;
                    bfRefresh = UPDATE_NONE;
                } else {
                    DoStopEvent(NULL);
                    lptd->lppd->fStopAtEntry = FALSE;
                    fGetFocus = TRUE;
                    bfRefresh |= UPDATE_NOFORCE;
                }
                break;

            case dbcDeleteThread:     /* DBG_INFO */
                // Any time we update the context we must invalidate the Watch window.
                bfRefresh = UPDATE_WINDOWS | UPDATE_SYMBOLS_CHANGED;
                lptd = (LPTD) GetQueueItemQuad();
                if ( lptd ) {
                    lppd = lptd->lppd;
                    Assert(lppd != NULL);
    #ifdef SHOW_MAGIC
                    CmdLogFmt("DBG: Thread Destroy:  Process=%d, Thread=%d\r\n",
                              lptd->lppd->ipid, lptd->itid);
    #endif
                    if (lptd->fInFuncEval) {
                        LptdFuncEval = NULL;
                    }
                    OSDDestroyHtid(lptd->lppd->hpid, lptd->htid);
                    DestroyTd(lptd);
                    if (LppdCur == lppd && LptdCur == lptd) {
                        LptdCur = NULL;
                    }
                    lptd = NULL;
                }
                fExecNext = FALSE;
                break;

          case dbcNewProc:           /* DBG_INFO */
                bfRefresh = UPDATE_NONE;
                /*
                 *  This will occur on all but the first process created.
                 */
                hpid = (HPID)GetQueueItemQuad();
                lppd = LppdOfHpid(hpid);

                Assert(lppd == NULL);

                lppd = CreatePd(hpid);

                SetPdInfo(lppd);

                lppd->hpds = SHCreateProcess();
                SHSetHpid(hpid);

                SetProcessExceptions(lppd);

                /*
                 * proc is PreRunning until ldr BP
                 */

                lppd->pstate = psPreRunning;

                /*
                 * If it is an attach, not a child, AttachDebuggee() will
                 * clear this flag in a moment.
                 */
                lppd->fChild = TRUE;

                CmdLogFmt("Process Create:  Process=%d\r\n", lppd->ipid);

                /*
                 *  This won't be the current process yet, because
                 *  there isn't a thread for it until we get notified.
                 */

                fExecNext = FALSE;
                break;


            case dbcProcTerm:          /* DBG_INFO */
                bfRefresh = UPDATE_NONE;
                lppd = LppdOfHpid((HPID) GetQueueItemQuad());
                LppdCur = lppd;
                LptdCur = NULL;
                SetPTState(psExited, tsInvalidState);
                lExit = (long) GetQueueItemQuad();

                if (!FKilling) {
                      CmdLogFmt("Process Terminate:  Process=%d, Exit Code=%ld\r\n",
                                lppd->ipid, lExit);
                }

                /*
                 *  Unresolve all the breakpoints. This way they can be
                 *  resolved again upon restarting.
                 */

                BPClearAllTmp( lppd->hpid, 0 );
                BPTUnResolveAll(lppd->hpid);

                fExecNext = FALSE;
                LppdCur = NULL;
                LptdCur = NULL;

                bfRefresh |= UPDATE_NOFORCE;
                break;

            case dbcDeleteProc:        /* DBG_INFO */
                bfRefresh = UPDATE_NONE;
                hpid = (HPID)GetQueueItemQuad();
                lppd = LppdOfHpid(hpid);

    #ifdef SHOW_MAGIC
                CmdLogFmt("DBG: Process Destroy:  Process=%d\r\n", lppd->ipid);
    #endif

                SHChangeProcess(lppd->hpds);

                while ( emi = SHGetNextExe( (HEXE) NULL ) ) {
                    SHUnloadDll( emi );
                }

                ClearProcessExceptions(lppd);

                if (!lppd->fPrecious) {
                    SHDeleteProcess(lppd->hpds);
                    OSDDestroyHpid(hpid);
                }

                DestroyPd(lppd, FALSE);
                RecycleIpid1();

                if (lppd == LppdCur) {
                    LppdCur = NULL;
                    LptdCur = NULL;
                } else if (LppdCur) {
                    SHChangeProcess(LppdCur->hpds);
                    // Any time we update the context we must invalidate the Watch window.
                    bfRefresh |= (UPDATE_NOFORCE | UPDATE_CONTEXT | UPDATE_WINDOWS | UPDATE_SYMBOLS_CHANGED);
                }

                break;

            case dbcInfoAvail:         /* DBG_INFO */

                bfRefresh = UPDATE_NONE;
                fExecNext = FALSE;

                fReply = (BOOL) GetQueueItemQuad(); // Reply flag

                if (!GetQueueItemQuad()) { /* fUniCode */
                    // ANSI
                    lpsz = (LPSTR) GetQueueItemQuad(); /* String */
                    CmdLogDebugString( lpsz, TRUE );
                } else {
                    // Unicode
                    LPWSTR lpw;

                    lpw = (LPWSTR) GetQueueItemQuad(); /* String */
                    l = WideCharToMultiByte(CP_ACP,
                                            0,
                                            lpw,
                                            -1,
                                            NULL,
                                            0,
                                            NULL,
                                            NULL
                                            );
                    lpsz = (PSTR) MHAlloc(l+1);
                    Assert(lpsz);
                    WideCharToMultiByte(CP_ACP,
                                        0,
                                        lpw,
                                        -1,
                                        lpsz,
                                        l+1,
                                        NULL,
                                        NULL
                                        );
                    CmdLogDebugString(lpsz, TRUE);
                    MHFree(lpw);
                }

                if (fWaitForDebugString) {
                    char *lpsz1=lpsz;
                    while (lpsz1 && *lpsz1) {
                        if (*lpsz1 == '\r' || *lpsz1 == '\n') {
                            *lpsz1 = '\0';
                        } else {
                            lpsz1 = CharNext(lpsz1);
                        }
                    }
                    if (_stricmp(lpsz, lpCmdString)==0) {
                        MHFree(lpCmdString);
                        fExecNext = TRUE;
                    }
                }
                MHFree(lpsz);

                if (fReply && LppdCur) {
                    OSDInfoReply(LppdCur->hpid, LptdCur->htid, NULL, 0);
                }

                break;

          case dbcInfoReq:

                bfRefresh = UPDATE_NONE;
                fExecNext = TRUE;

                if (!GetQueueItemQuad()) { /* fUniCode */
                    // ANSI
                    lpsz = (LPSTR) GetQueueItemQuad(); /* String */
                } else {
                    // Unicode
                    LPWSTR lpw;

                    lpw = (LPWSTR) GetQueueItemQuad(); /* String */
                    l = WideCharToMultiByte(CP_ACP,
                                            0,
                                            lpw,
                                            -1,
                                            NULL,
                                            0,
                                            NULL,
                                            NULL
                                            );
                    lpsz = (PSTR) MHAlloc(l+1);
                    Assert(lpsz);
                    WideCharToMultiByte(CP_ACP,
                                        0,
                                        lpw,
                                        -1,
                                        lpsz,
                                        l+1,
                                        NULL,
                                        NULL
                                        );
                    MHFree(lpw);
                }

                CmdGetDefaultPrompt(szOldPrompt);
                CmdSetDefaultPrompt(lpsz);
                CmdSetCmdProc(CmdHandleInputString, CmdExecutePrompt);
                CmdDoPrompt(TRUE, TRUE);
                FCmdDoingInput = TRUE;
                MHFree(lpsz);
                break;

            case dbcException:         /* DBG_REFRESH */
            {
                EXCEPTION_LIST *eList;
                LPEPR          lpepr;

                lptd = (LPTD) GetQueueItemQuad();
                if (lptd != 0) {
                    lppd = lptd->lppd;
                    htid = lptd->htid;
                    hpid = lppd->hpid;
                } else {
                    hpid = (HPID)GetQueueItemQuad();
                    htid = (HTID)GetQueueItemQuad();
                    lppd = LppdOfHpid(hpid);
                    Assert(lppd);
                    lptd = LptdOfLppdHtid(lppd, htid);
                }


                LppdCur = lppd;
                LptdCur = lptd;

                lpepr = (LPEPR) GetQueueItemQuad();
                Assert(lpepr);

                if (LppdCur->pstate == psPreRunning) {

                    /*
                     * We hit an exception before the process
                     * had finished loading.  We probably won't ever
                     * hit the loader BP, so mark this as loaded now.
                     */

                    LppdCur->pstate = psStopped;

                    BPTResolveAll(LppdCur->hpid,TRUE);
                    SetProcessExceptions(LppdCur);

    #ifdef SHOW_MAGIC
                    CmdLogFmt("(Exception caught while loading)\r\n");
    #endif
                }

                for ( eList = LppdCur->exceptionList;
                      eList;
                      eList = eList->next )
                {
                    if ( eList->dwExceptionCode == lpepr->ExceptionCode ) {
                        break;
                    }
                }

                //
                // The local list should always match the DM's list
                // except on second chance, when the DM always punts
                //

                Assert(eList == 0 ||
                       lpepr->efd == eList->efd ||
                       (!lpepr->dwFirstChance &&
                        (lpepr->efd == efdStop || lpepr->efd == efdCommand)));

                if (lpepr->dwFirstChance) {
                    SetPTState(psInvalidState, tsException1);
                } else {
                    SetPTState(psInvalidState, tsException2);
                }

                AuxPrintf(1, "Exception %d, FirstChance == %d, efd == %d",
                             lpepr->ExceptionCode,
                             lpepr->dwFirstChance,
                             lpepr->efd);

                switch (lpepr->efd) {
                case efdNotify:
                    SetPTState(psInvalidState, tsRunning);

                case efdStop:
                case efdCommand:
                    CmdLogVar((WORD)((lpepr->dwFirstChance)?
                                     DBG_Exception1_Occurred : DBG_Exception2_Occurred),
                              lpepr->ExceptionCode,
                              (eList == NULL || eList->lpName == NULL) ?
                              "Unknown" : eList->lpName);

                    if (lpepr->efd == efdNotify) {
                        bfRefresh = UPDATE_NONE;
                    } else {
                        fGetFocus = TRUE;
                        lptd->lppd->fStopAtEntry = FALSE;
                        if ( eList ) {
                            if ( lpepr->dwFirstChance && eList->lpCmd ) {
                                CmdPrependCommands(lptd, eList->lpCmd);
                            } else if ( !lpepr->dwFirstChance && eList->lpCmd2 ) {
                                CmdPrependCommands(lptd, eList->lpCmd2);
                            }
                        }
                        if (!DoStopEvent(lptd)) {
                            CmdLogVar(DBG_Thread_Stopped);
                        }
                    }
                    break;

                case efdIgnore:
                    bfRefresh = UPDATE_NONE;
                    CmdLogFmt("WINDBG: benign error - \r\n  efdIgnore should never reach the shell - \r\n");
                    CmdLogFmt("  ExceptionCode == %08x\r\n", lpepr->ExceptionCode);
                    break;

                default:
                    // undefined efd value
                    bfRefresh = UPDATE_NONE;
                    CmdLogFmt("INTERNAL ERROR: unrecognized efd %d\r\n", lpepr->efd);
                    Assert(FALSE);
                    break;
                }
                MHFree(lpepr);

                break;
            }


                /*
                 *      This message is recieved when a function call from the
                 *      expression evaluator is finished.
                 *
                 *      DO  NOTHING
                 */

            case dbcExecuteDone:       /* DBG_INFO */
                bfRefresh = UPDATE_NONE;
                fRecurse = FALSE;
                fExecNext = FALSE;
                break;

            case dbcServiceDone:
                //
                // Some Ioctl has finished executing
                //
                // Signal the synchronization event to release the command window
                //

                GetQueueItemQuad();
                SetEvent( hEventIoctl );
                bfRefresh = UPDATE_NONE;
                fExecNext = FALSE;
                break;

            case dbcCanStep:
                CmdLogFmt("WINDBG: dbcCanStep is obsolete\r\n");
                RAssert(FALSE);
                break;

            case (DBC)dbcRemoteQuit:
                bfRefresh = UPDATE_NONE;
                CmdLogFmt("Connection to remote has been broken\r\n" );
                CmdLogFmt("Stopped debugging\r\n" );
                DisconnectDebuggee();
                fRecurse = FALSE;
                return bfRefresh;

            case (DBC)dbcMemoryChanged:
                bfRefresh = UPDATE_NONE;
                BPUpdateMemory( (ULONG) GetQueueItemQuad() );
                fExecNext = FALSE;
                bfRefresh = UPDATE_NONE;
                break;

            case dbcSegLoad:

                lptd    = (LPTD)GetQueueItemQuad();
                Assert(lptd != NULL);
                lppd    = lptd->lppd;
                LppdCur = lppd;
                LptdCur = lptd;

                bfRefresh = UPDATE_NONE;
                fExecNext = FALSE;

                SHChangeProcess(lppd->hpds);
                BPSegLoad( (ULONG)GetQueueItemQuad() );

                Go();
                break;

            case dbcExitedFunction:
                lptd = (LPTD) GetQueueItemQuad();
                if (lptd == NULL) {
                    hpid = (HPID)GetQueueItemQuad();
                    htid = (HTID)GetQueueItemQuad();
                }
                // return value:
                dw = (DWORD) GetQueueItemQuad();
                fExecNext = FALSE;
                bfRefresh = UPDATE_NONE;
                break;


            case (DBC)dbcCommError:
            default:
                lptd = (LPTD) GetQueueItemQuad();
                if (lptd == NULL) {
                    hpid = (HPID)GetQueueItemQuad();
                    htid = (HTID)GetQueueItemQuad();
                }
                CmdLogFmt("WINDBG: Unknown DBC %x\r\n", dbcCallback);
                fExecNext = FALSE;
                break;

            }

            //
            //  Thread and/or process may have been changed - try to make
            //  sure that both are valid, and update status line.
            //

            if (LptdCur != NULL) {
                LppdCur = LptdCur->lppd;
            } else if (LppdCur != NULL) {
                LptdCur = LppdCur->lptdList;
            }

            if (LptdCur == NULL) {
                GetFirstValidPDTD(&LppdCur, &LptdCur);
            }

            if (LppdCur == NULL) {
                AuxPrintf(1, "(CmdExecNext: There are no processes)");
            } else if (LptdCur == NULL) {
                AuxPrintf(1, "(CmdExecNext: There are no threads)");
            } else if (LppdCur != LppdT || LptdCur != LptdT) {
                //
                // Any time we update the context we must invalidate the
                // Watch window.
                //
                bfRefresh |= UPDATE_CONTEXT |
                             UPDATE_ALLDBGWIN |
                             UPDATE_SYMBOLS_CHANGED;
            }

            //
            // update status line
            //

            SetPidTid_StatusBar(LppdCur, LptdCur);

            //
            //  Update the users view of the world to reflect the last debug
            //  event.
            //

            if ((bfRefresh != UPDATE_NONE) &&
                (LptdCur != NULL) && (!LptdCur->fInFuncEval)) {

                UpdateDebuggerState(bfRefresh);
            }

            //
            //  Worry about executing the next command on the list
            //

            if (fExecNext && (LptdCur == NULL || LptdCur->fInFuncEval == FALSE) ) {

                if (CmdExecuteLine(NULL)) {
                    if ((AutoRun == arSource || AutoRun == arCmdline)
                             && !FAutoRunSuppress ) {
                        PostMessage(Views[cmdView].hwndClient, WU_AUTORUN, 0, 0);
                    } else {
                        CmdDoPrompt(!FCmdDoingInput, FALSE);
                    }
                } else {
                    fGetFocus = FALSE;
                }
            }


            //
            // Grab focus if we really stopped somewhere.
            //

            if (fGetFocus) {

                HEXE    hexe;
                ADDR    Addr;

                if (LppdCur && LptdCur) {
                    OSDGetAddr(LppdCur->hpid, LptdCur->htid, adrPC, &Addr);
                    SHChangeProcess(LppdCur->hpds);

                    if ( (HPID)emiAddr( Addr ) == LppdCur->hpid ) {
                        //
                        //  Get right EMI and load symbols if defered.
                        //
                        emiAddr( Addr ) = 0;
                        OSDSetEmi(LppdCur->hpid,LptdCur->htid,&Addr);
                    }

                    hexe = (HEXE)emiAddr( Addr );
                    if ( hexe && (HPID)hexe != LppdCur->hpid ) {
                        SHWantSymbols( hexe );
                    }
                }

                if (!RemoteRunning) {
                    EnsureFocusDebugger();
                }
            }
        }
    } __except(GenericExceptionFilter(GetExceptionInformation())) {
        //DAssert(FALSE);
    }

    fRecurse = FALSE;
    return(bfRefresh);
}                                       /* CmdExecNext() */



BOOL
GetAutoRunSuppress(
    void
    )
{
    return FAutoRunSuppress;
}


BOOL
SetAutoRunSuppress(
    BOOL f
    )
{
    BOOL ff = FAutoRunSuppress;
    FAutoRunSuppress = f;
    return ff;
}

/***********************************************************************

 Hotkey handler

 ^C is used for a general interrupt signal.  When a ^C is caught,
 a list of handlers is walked.  Each handler is executed, and the return
 value checked.  If the return value is TRUE, the walk continues.  If
 the value is FALSE, the walk is terminated.

 The list is walked in MRU order; the most recently registered handler
 is called first.

***********************************************************************/


PCTRLC_HANDLER
AddCtrlCHandler(
    CTRLC_HANDLER_PROC pfnFunc,
    DWORD              dwParam
    )
/*++

Routine Description:

    Add a CtrlC handler routine to the handler list.

Arguments:

    pfnFunc  - Supplies pointer to handler function
    dwParam  - Supplies parameter to pass to function

Return Value:

    A pointer to the registered handler.  This is only used
    for removing the handler from the chain.

--*/
{
    PCTRLC_HANDLER pcch = (PCTRLC_HANDLER) malloc(sizeof(CTRLC_HANDLER));
    if (pcch) {
        pcch->pfnFunc = pfnFunc;
        pcch->dwParam = dwParam;
        pcch->next = PcchHead;
        PcchHead = pcch;
    }
    return pcch;
}


BOOL
RemoveCtrlCHandler(
    PCTRLC_HANDLER  pcch
    )
/*++

Routine Description:

    Remove a CtrlC handler from the list.  This can remove a specific
    handler, or remove the last one added.

Arguments:

    pcch  - Supplies pointer to the handler to remove.  If this is
            NULL, the last one added will be removed.

Return Value:

    TRUE if the handler was removed, FALSE if it did not exist.

--*/
{
    PCTRLC_HANDLER *ppcch;
    if (!pcch) {
        pcch = PcchHead;
    }

    for ( ppcch = &PcchHead; *ppcch; ppcch = &((*ppcch)->next) ) {
        if (*ppcch == pcch) {
            *ppcch = pcch->next;
            free(pcch);
            return TRUE;
        }
    }
    return FALSE;
}


VOID
DispatchCtrlCEvent(
    VOID
    )
/*++

Routine Description:

    Walk the list of Ctrl C handlers, calling each one until one
    returns FALSE.

Arguments:

    None

Return Value:

    None

--*/
{
    PCTRLC_HANDLER pcch;
    for (pcch = PcchHead; pcch; pcch = pcch->next) {
        if (!(*pcch->pfnFunc)(pcch->dwParam)) {
            break;
        }
    }
}


/*******************************************************************



*******************************************************************/

BOOL
DoCtrlCAsyncStop(
    DWORD dwParam
    )
{
    CmdInsertInit();
    CmdLogFmt("Ctrl+C <process stopping...>\r\n");
    AsyncStop();

    return dwParam;
}


BOOL
PolledCtrlCHandler(
    DWORD dwParam
    )
{
    CmdInsertInit();
    CmdLogFmt("^C\r\n");
    fCtrlCPressed = TRUE;
    return dwParam;
}


void
SetCtrlCTrap(
    void
    )
/*++

Routine Description:

    This manages a polled Ctrl C trap.  Add a handler which sets
    a flag.

Arguments:

    None

Return Value:

    None

--*/
{
    fCtrlCPressed = FALSE;
    pPolledCtrlCHandler = AddCtrlCHandler(PolledCtrlCHandler, FALSE);
}


void
ClearCtrlCTrap(
    void
    )
/*++

Routine Description:

    Clear trap for polled CTRL-C

Arguments:

    None

Return Value:

    None

--*/
{
    if (pPolledCtrlCHandler) {
        RemoveCtrlCHandler(pPolledCtrlCHandler);
        pPolledCtrlCHandler = NULL;
    }
    fCtrlCPressed = FALSE;
}


ULONG
WDBGAPI
CheckCtrlCTrap(
    void
    )
/*++

Routine Description:

    Allow any hotkey events to be handled, then check the POLLED Ctrl C
    flag.  If the Ctrl C polling handler is not in use, this will return
    FALSE and not drain the message queue.

Arguments:

    None

Return Value:

    TRUE if CTRL-C was pressed since last checked.

--*/
{
    BOOL f;
    MSG  msg;
    BOOL fIgnore;

    if (!pPolledCtrlCHandler) {
        return FALSE;
    }

    if (cmdView == -1) {
        return FALSE;
    }


    //while (PeekMessage(&msg, NULL, WM_HOTKEY, WM_HOTKEY, PM_REMOVE))
    while (PeekMessage(&msg, Views[cmdView].hwndClient, WM_KEYDOWN, WM_DEADCHAR, PM_REMOVE)
        || PeekMessage(&msg, Views[cmdView].hwndClient, WM_COMMAND, WM_COMMAND, PM_REMOVE))
    {
        switch (msg.message) {
        default:
            fIgnore = FALSE;
            break;

        case WM_KEYDOWN:
        case WM_KEYUP:
            switch (msg.wParam) {
            default:
                fIgnore = TRUE;
                break;

            case 'C':
            case VK_CONTROL:
            case VK_RCONTROL:
            case VK_LCONTROL:
                fIgnore = FALSE;
                break;
            }
            break;

        case WM_CHAR:
            switch (msg.wParam) {
            default:
                fIgnore = TRUE;
                break;

            case 'C':
            case VK_CONTROL:
            case VK_RCONTROL:
            case VK_LCONTROL:
                fIgnore = FALSE;
                break;
            }
            break;
        }

        if (!fIgnore) {
            ProcessQCQPMessage(&msg);
        }
    }
    f = fCtrlCPressed;
    fCtrlCPressed = FALSE;
    return f;
}


BOOL
CmdSetEatEOLWhitespace(
    BOOL ff
    )
/*++

Routine Description:

    Set value of EatEOLWhitespace flag to control behaviour of
    data entry in command window.

Arguments:

    ff - Supplies new value for fEatEOLWhitespace

Return Value:

    Old value of fEatEOLWhitespace

--*/
{
    BOOL f = fEatEOLWhitespace;
    fEatEOLWhitespace = ff;
    return f;
}

void
CmdSetAutoHistOK(
    BOOL f
    )
{
    FAutoHistOK = f;
}


BOOL
CmdGetAutoHistOK(
    void
    )
{
    return FAutoHistOK;
}


LRESULT
CmdEditProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

    This function is the window message processor for the command
    window class.  It processes those messages which are of interest
    to this specific window class and passes all other messages on to
    the default MDI window procedure handler

Arguments:

    hwnd    - Supplies window handle to the command window

    msg     - Supplies message to be processed

    wParam  - Supplies info about the message

    lParam  - Supplies info about the message

Return Value:

    various

--*/
{
    int         i;
    int         x;
    LRESULT     lRet = 0;
    int         XPos, YPos;
    int         Xro, Yro;
    BOOL        fShift;
    BOOL        fCtrl;
    long        first;
    LPLINEREC   pl;
    LPBLOCKDEF  pb;
    char        szStr[MAX_LINE_SIZE];
    int         doc = Views[cmdView].Doc;
    int         nLines;
    BOOL        fDoHist;

    static BOOL fShowingLine = FALSE;
    static BOOL fEdited      = FALSE;
    static int  nHistoryLine = 0;
    static int  nHistBufTop  = 0;   // oldest command in history
    static int  nHistBufBot  = 0;   // where next one goes
    static LPSTR alpszHistory[MAX_CMDWIN_HISTORY] = {0};


    switch (msg) {

    case WU_INITDEBUGWIN:

        /*
         * set up ctrlc handler
         */
        AddCtrlCHandler(DoCtrlCAsyncStop, FALSE);

        /*
        ** Initialize cmd processor, show initial prompt.
        */

        CmdSetDefaultCmdProc();
        if (!AutoRun) {
            CmdDoPrompt(TRUE, TRUE);
            GetRORegion(cmdView, &Xro, &Yro);
            PosXY(cmdView, Xro, Yro, FALSE);
        }

        return FALSE;

    case WM_SETFOCUS:
        InterlockedExchangePointer((PVOID *) &hCurrAccTable, hCmdWinAccTable);
#if 0
        // Under development
        {
            // Set the context sensitive menu
            HMENU hmenuCurr = GetMenu(hwndFrame);

            if (hmenuCurr != hmenuCmdWin) {
                SendMessage(g_hwndMDIClient, WM_MDISETMENU, (WPARAM) hwndFrame, (LPARAM) hmenuCmdWin);
                Assert(DrawMenuBar(hwndFrame));
            }
        }
#endif
        break;

    case WM_KILLFOCUS:
        InterlockedExchangePointer((PVOID *) &hCurrAccTable, hMainAccTable);
        break;

    case WM_KEYDOWN:

        fShift = (GetKeyState(VK_SHIFT) < 0);
        fCtrl  = (GetKeyState(VK_CONTROL) < 0);
        GetRORegion(cmdView, &Xro, &Yro);
        XPos = Views[cmdView].X;
        YPos = Views[cmdView].Y;
        nLines = max(Docs[doc].NbLines, 1);

        if (YPos >= Yro && XPos >= Xro) {

            if (YPos > Yro) {
                Xro = 0;
            }

            // In writeable region

            switch (wParam) {

            case 'R':
                if (fCtrl) {
                    // hack...
                    // this is just a macro for ".resync"
                    InsertBlock(doc, Xro, YPos, 7, ".resync");
                    InvalidateLines(cmdView, nLines-1, nLines-1, TRUE);
                    PosXY(cmdView, Xro + 7, nLines-1, FALSE);
                    lRet = 0;
                    break;
                }

            default:
                fEdited = TRUE;
                lRet = CallWindowProc(lpfnEditProc, hwnd, msg, wParam, lParam);
                break;

            case VK_UP:

                lRet = 0;

                if (fCtrl) {

                    // Execute magic ctrl-up
                    KeyDown(cmdView, wParam, fShift, FALSE);

                } else {

                    // get previous history line, if any:
                    if (!fShowingLine) {

                        // up from new line; show last cmd
                        goto GotHistLine;

                    } else if (nHistoryLine != nHistBufTop) {

                        if (--nHistoryLine < 0) {
                            nHistoryLine = MAX_CMDWIN_HISTORY-1;
                        }
                        goto GotHistLine;
                    } /* else don't do anything */
                }

                lRet = 0;
                break;

            case VK_DOWN:

                // get next history line

                lRet = 0;

                i = nHistoryLine;

                if (i != nHistBufBot && ++i >= MAX_CMDWIN_HISTORY) {
                    i = 0;
                }

                if (i != nHistBufBot) {
                    nHistoryLine = i;
                } else {
                    // no more history; forget it.
                    break;
                }

                GotHistLine:

                // is history empty?
                if (!alpszHistory[nHistoryLine]) {
                    break;
                }

                fShowingLine = TRUE;
                fEdited = FALSE;

                // erase command line...
                DeleteBlock(doc, Xro, nLines - 1, MAX_USER_LINE, nLines - 1);

                // insert new line:
                InsertBlock(doc, Xro, nLines-1,
                            strlen(alpszHistory[nHistoryLine]),
                            alpszHistory[nHistoryLine]);
                InvalidateLines(cmdView, nLines-1, nLines-1, TRUE);
                PosXY(cmdView, Xro + strlen(alpszHistory[nHistoryLine]), nLines-1, FALSE);

                break;

            case VK_LEFT:

                lRet = 0;
                if (XPos > Xro) {
                    lRet = CallWindowProc( lpfnEditProc, hwnd, msg, wParam, lParam );
                } else if (fCtrl) {
                    KeyDown(cmdView, wParam, fShift, FALSE);
                }
                break;

            case VK_BACK:

                lRet = 0;
                if (XPos > Xro) {
                    fEdited = TRUE;
                    lRet = CallWindowProc( lpfnEditProc, hwnd, msg, wParam, lParam );
                }
                break;

            case VK_HOME:
                if (fCtrl && fShift || fCtrl) {
                    // Ctrl+Shift+Home or Ctrl+Home pressed
                    lRet = CallWindowProc( lpfnEditProc, hwnd, msg, wParam, lParam );
                }if (fShift) {
                    // Shift+Home pressed
                    // The farthest position to the left that can be selected is 2
                    int tmp_int = FirstNonBlank(doc, YPos);
                    int X = max(tmp_int, 2);

                    if (XPos != X) {
                        SelPosXY(cmdView, X, YPos);
                    } else {
                        SelPosXY(cmdView, 2, YPos);
                    }
                } else {
                    // Home pressed
                    PosXY(cmdView, Xro, nLines-1, FALSE);
                }
                break;

            case VK_ESCAPE:

                // erase command line...
                DeleteBlock(doc, Xro, nLines - 1, MAX_USER_LINE, nLines - 1);
                PosXY(cmdView, Xro, nLines - 1, FALSE);
                InvalidateLines(cmdView, nLines-1, nLines-1, TRUE);

                fShowingLine = FALSE;
                fEdited = TRUE;
                break;

            case VK_RETURN:

                first = YPos;
                FirstLine(doc, &pl, &first, &pb);
                if (!fEatEOLWhitespace) {
                    x = pl->Length - LHD;
                } else {
                    // put cursor after last non-white char on line
                    // ExpandTabs() expands the string into global
                    // el[] and remembers the length in elLen.
                    ExpandTabs(&pl);
                    x = elLen-1;
                    while (x > -1) {
                        if (isspace(el[x])) {
                            --x;
                        } else {
                            break;
                        }
                    }
                    x++;
                }
                x = max(x, Xro);


                if (!g_contGlobalPreferences_WkSp.m_bCommandRepeat || !CmdGetAutoHistOK() || x != Xro) {
                    PosXY(cmdView, x, YPos, FALSE);
                    fDoHist = TRUE;
                } else {
                    fDoHist = FALSE;
                    if (!alpszHistory[nHistoryLine]) {
                        *szStr = 0;
                    } else {
                        strcpy(szStr, alpszHistory[nHistoryLine]);

                        if (tolower(*szStr) == 'g' || tolower(*szStr) == '.' ||
                            tolower(*szStr) == 'l') {

                            *szStr = 0;

                        } else {

                            InsertBlock(doc, Xro, YPos,
                                        strlen(alpszHistory[nHistoryLine]),
                                        alpszHistory[nHistoryLine]);
                            InvalidateLines(cmdView, nLines-1, nLines-1, TRUE);
                            PosXY(cmdView,
                                  Xro + strlen(alpszHistory[nHistoryLine]),
                                  YPos,
                                  FALSE);

                        }
                    }
                }

                // give user visual feedback
                SetCaret(cmdView, 0, YPos, -1);
                UpdateWindow(hwnd);

                // give CR to edit mangler
                lRet = CallWindowProc( lpfnEditProc, hwnd, msg, wParam, lParam );

                // the line we just entered (reload it, it may have been mangled):
                first = YPos;
                FirstLine(doc, &pl, &first, &pb);

                if (fDoHist) {
                    strncpy(szStr, pl->Text + Xro, x - Xro);
                    szStr[x - Xro] = 0;
                    // remember history
                    // this version remembers everything in order,
                    // but still only resets the history line when
                    // it has been edited.
                    if (*szStr) {
                        if (alpszHistory[nHistBufBot]) {
                            Assert(nHistBufBot == nHistBufTop);
                            free(alpszHistory[nHistBufTop++]);
                            if (nHistBufTop >= MAX_CMDWIN_HISTORY) {
                                nHistBufTop = 0;
                            }
                        }
                        if (!fShowingLine || fEdited) {
                            nHistoryLine = nHistBufBot;
                        }
                        alpszHistory[nHistBufBot++] = _strdup(szStr);
                        if (nHistBufBot >= MAX_CMDWIN_HISTORY) {
                            nHistBufBot = 0;
                        }
                    }
                }

                fShowingLine = FALSE;

                CmdFileString(szStr);
                CmdFileString("\r\n");
                SendClientOutput(szStr, strlen(szStr));
                SendClientOutput("\r\n", 2);
                CmdDoLine(szStr);
                CmdDoPrompt(TRUE, TRUE);

                lRet = 0;
                break;
            }

        } else {

            // in readonly region
            // everything has to work here...  the edit mangler will protect
            // the readonly region, so we can just throw everything at it, but
            // we need to decide when to fall onto the command line, and when
            // to leave things alone...

            first = nLines - 1;
            FirstLine(doc, &pl, &first, &pb);

            if (wParam == CTRL_M) {
                // position at end of cmd line
                ClearSelection(cmdView);
                PosXY(cmdView, pl->Length-LHD, nLines - 1, FALSE);
                lRet = 0;
            } else {

                EnableReadOnlyBeep(FALSE);
                lRet = CallWindowProc(lpfnEditProc, hwnd, msg, wParam, lParam);
                if (QueryReadOnlyError()) {
                    // position at end of command line, and try again
                    ClearSelection(cmdView);
                    PosXY(cmdView, pl->Length-LHD, nLines - 1, FALSE);
                    lRet = CallWindowProc(lpfnEditProc, hwnd, msg, wParam,
                                          lParam);
                }
                EnableReadOnlyBeep(TRUE);

            }
        }

        return lRet;

    case WM_IME_REPORT:
        if (IR_STRING != wParam) {
            break;
        }
        // Fall through

    case WM_CHAR:

        // the interesting cases have already been handled;  we just want
        // to fall onto the command line in case of an error

        if (wParam == CTRL_R) {
            return 0;
        }

        EnableReadOnlyBeep(FALSE);
        lRet = CallWindowProc( lpfnEditProc, hwnd, msg, wParam, lParam );
        if (QueryReadOnlyError()) {
            // position at end of command line, and try again
            ClearSelection(cmdView);
            nLines = max(Docs[doc].NbLines, 1);
            first = nLines - 1;
            FirstLine(doc, &pl, &first, &pb);
            PosXY(cmdView, pl->Length-LHD, nLines - 1, FALSE);
            lRet = CallWindowProc( lpfnEditProc, hwnd, msg, wParam, lParam );
        }
        EnableReadOnlyBeep(TRUE);

        return lRet;

    case WM_PASTE:

        if (OpenClipboard(hwndFrame)) {
            HANDLE      hData;
            size_t      size;
            LPSTR       p1;
            LPSTR       p;

            hData = GetClipboardData(CF_TEXT);

            if (hData && (size = GlobalSize (hData))) {
                if (size >= MAX_CLIPBOARD_SIZE) {
                    ErrorBox(ERR_Clipboard_Overflow);
                } else if ( p = (PSTR) GlobalLock(hData) ) {
                    int x, y;
                    x = Views[cmdView].X;
                    y = Views[cmdView].Y;

                    p1 = p;
                    while (size && *p1) {
                        size--;
                        if (IsDBCSLeadByte(*p1) && *(p1+1)) {
                            p1 += 2;
                            size--;
                            continue;
                        }
                        if (*p1 == '\r' || *p1 == '\n') {
                            break;
                        }
                        p1++;
                    }
                    size = (size_t) (p1 - p);

                    InsertStream(cmdView, x, y, size, p, TRUE);
                    PosXY(cmdView, x + size, y, TRUE);
                    DbgX(GlobalUnlock (hData) == FALSE);
                }
                CloseClipboard();
            }
        }
        return 0;

    case WM_DESTROY:
        /*
        **  Destroy this instance of the window proc
        */

        //UnregisterHotKey(NULL, IDH_CTRLC);
        UnregisterHotKey(hwnd, IDH_CTRLC);

        while (RemoveCtrlCHandler(NULL)) {
            ;
        }

        FreeProcInstance((WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC,
                                                (DWORD_PTR)lpfnEditProc));
        break;

    case WU_AUTORUN:
        /*
        **      Need to get and process an auto-run command
        */

        CmdAutoRunNext();
        break;

    case WU_LOG_REMOTE_CMD:
        //
        // Echo and handle a command sent by a remote client
        //

        if (wParam) {
            CmdLogFmtEx( TRUE, FALSE, TRUE, (LPSTR)lParam);
        }

        CmdDoLine( (LPSTR)lParam );
        CmdDoPrompt(!FCmdDoingInput, TRUE);
        FCmdDoingInput = FALSE;

        if (wParam) {
            free((LPVOID)lParam);
        }
        break;

    case WU_LOG_REMOTE_MSG:
        //
        // print some random junk from a remote client
        //
        CmdInsertInit();
        if (!wParam) {
            GetRORegion(cmdView, &Xro, &Yro);
            first = Yro;
            FirstLine(doc, &pl, &first, &pb);

            strncpy(szStr, pl->Text, Xro);
            szStr[Xro] = 0;

            CmdLogDebugString(szStr, FALSE);
        }

        CmdLogDebugString( (LPSTR)lParam, TRUE);

        free( (LPSTR)lParam );

        break;

    }
    return ( CallWindowProc( lpfnEditProc, hwnd, msg, wParam, lParam ) );
}                                       /* CmdEditProc() */
