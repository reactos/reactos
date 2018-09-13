/*********************************************************************

        File:                                   makeeng.c

        Date created:           27/8/90

        Author:                         Tim Bell

        Description:

        Windows Make Engine API

        Modified:

*********************************************************************/

#include "precomp.h"
#pragma hdrstop


BOOL
StartDebuggee(void)
{
    char    *argv[2];
    int     argc = 2;
    char    Executable[ MAX_PATH ];

    argv[0] = GetExecutableFilename(Executable, sizeof(Executable));

    //
    // Must start with space or lose first character
    //
    argv[1] = LpszCommandLine;

    if (g_contWorkspace_WkSp.m_bUserCrashDump) {
        Assert(g_contWorkspace_WkSp.m_pszUserCrashDump);
        argv[0] = g_contWorkspace_WkSp.m_pszUserCrashDump;
    } else if (argv[0] == NULL) {
        if (g_contWorkspace_WkSp.m_bKernelDebugger) {
            argv[0] = NT_KERNEL_NAME;
        } else {
            ErrorBox(ERR_No_DLL_Caller);
            return FALSE;
        }
    }

    //
    //  Note that RestartDebuggee disables the appropriate
    // toolbar controls.
    //
    if (!RestartDebuggee(argv[0], argv[1])) {
        return FALSE;
    }

    //
    //  Clear out the current source window view information
    //
    TraceInfo.doc = -1;

    Assert( DebuggeeActive());

    return TRUE;
}



/***    ExecDebuggee
**
**  Synopsis:
**      bool = ExecDebuggee(ExecType)
**
**  Entry:
**      ExecType - type of execution to be done
**
**  Returns:
**      TRUE on success and FALSE on failure
**
**  Description:
**      Executes the debuggee in the passed manner.  Makes all
**      the necessary tests against the make to see if a
**      build should take place.
**
*/

BOOL
ExecDebuggee(
    EXECTYPE ExecType
    )
{
    BOOL rVal = TRUE;
    int  View = 0;
    int  line = 0;

    //
    //  If a process is half started, bail out
    //
    if (DbgState != ds_normal) {
        MessageBeep(0);
        return FALSE;
    }

    //
    //  For a restart we need to get the system to kill off any debuggee
    //  which may currently be active.
    //
    if ((ExecType == EXEC_RESTART) && DebuggeeAlive()) {
        if (!KillDebuggee()) {
            MessageBeep(0);
            return FALSE;
        }
    }

    //
    // remember this before anybody gets a chance to
    // mess with windows.
    //
    if (ExecType == EXEC_TOCURSOR) {
        View = curView;
        line = Views[View].Y;
    }

    //
    //  Check for legal thread and process states for issuing a command
    //  to the debugger.  If it is not legal then beep and return.
    //
    //  If there is not current process descriptor then all commands are
    //  legal.
    //
    if (LppdCur != NULL) {

        //
        //  If no thread descriptor then all commands are legal:
        //
        if (LptdCur != NULL) {

            //
            //  Check to see if the current thread is actually in a running state
            //  if so then beep and return immeadiately
            //
            if (LptdCur->tstate == tsRunning) {
                MessageBeep(0);
                return FALSE;
            }
        }
    }

    //
    //  If there is no child process running then we need to play some
    //  games to get it running.
    //
    if (!DebuggeeActive()) {
        //
        //      Start up the child process.
        //
        if (!StartDebuggee()) {
            //
            //  An error message has been printed out already.
            //
            return FALSE;
        }

    }

    //
    //  Either there was a child running when we first came into this
    //  procedure or we successfully spawned up a child
    //

    //
    //  check with all windows for their approval about running the
    //  program.  This will give then a change to update machine state
    //

    if (!DbgCommandOk()) {
        return FALSE;
    }

    //
    //  Stepping of frozen threads and processes
    //
    switch (ExecType) {
        case EXEC_RESTART:
        case EXEC_GO:
            break;

        case EXEC_STEPANDGO:
        case EXEC_TOCURSOR:
        case EXEC_TRACEINTO:
        case EXEC_STEPOVER:
            if (LppdCur->fFrozen || LptdCur->fFrozen) {
                MessageBeep(0);
                return FALSE;
            }
    }

    switch (ExecType) {

        case EXEC_RESTART:
            //
            //  We have gotten the system to get started.  Now make sure
            //  that the screen gets updated since we will not be getting
            //  any "funny" messages from the system to get the screen
            //  updated.
            //
            UpdateDebuggerState(UPDATE_ALLSTATES & ~UPDATE_SOURCE);
            break;

        case EXEC_GO:
            //
            //  Just run the program and be done with it.  We will later
            //  get back a message which will case updating of the
            //  screen
            //
            Go();
            break;

        case EXEC_STEPANDGO:
            break;

        case EXEC_TOCURSOR:
            if (!ContinueToCursor(View, line)) {
                MessageBeep(0);
                return FALSE;
            }
            break;

        case EXEC_TRACEINTO:
            //
            //  Execute the command.  If in source mode and the debuggee
            //  is not yet running then these routines will cause the
            //  child to be run to either the main entry point or a
            //  debuggee event which stops execution.
            //
            if (Step(FALSE, GetSrcMode_StatusBar() ? SRCSTEPPING : ASMSTEPPING) == FALSE) {
                return FALSE;
            }
            break;

        case EXEC_STEPOVER:
            if (Step(TRUE, GetSrcMode_StatusBar() ? SRCSTEPPING : ASMSTEPPING) == FALSE) {
                return FALSE;
            }
            break;

    }
    return TRUE;
}
