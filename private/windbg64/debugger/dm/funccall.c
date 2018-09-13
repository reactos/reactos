/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    funchdr.c

Abstract:

    This file contians the code used for supporting function evaluation
    from the expression evaluator.

Author:

    Jim Schaad (jimsch) 06-04-92

Environment:

    Win32 - User



--*/

#include "precomp.h"
#pragma hdrstop



SetFile()


/**********************************************************************/

void
NotifyFuncCall(
    DEBUG_EVENT64 * lpde,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    );

/**********************************************************************/


METHOD     MthdFuncNotify = {NotifyFuncCall};

extern DMTLFUNCTYPE        DmTlFunc;
extern LPDM_MSG            LpDmMsg;

/**********************************************************************/



VOID
ProcessSetupExecuteCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
/*++

Routine Description:

    This function is called in response to a dmfSetupExecute request.
    It will create a local Execute Object and return the handle to
    the object.

Arguments:

    hprc        - Supplies the process handle for the operation
    hthd        - Supplies the thread handle for the operation
    lpdbb       - Supplies a pointer to possible futher information

Return Value:

    None.

--*/

{
    LPEXECUTE_OBJECT_DM lpeo;

    Unreferenced( hprc );

    DPRINT(1, ("Setup Execute"));

    /*
     *  In order for this to be a valid operation the requested
     *  thread must be in a stopped state (and don't allow evaluation
     *  until we have hit the loader breakpoint)
     */

    if ( ((hthd->tstate & ts_stopped) == 0) ||
         ((hthd->tstate & (ts_dead|ts_destroyed)) != 0) ||
         (hprc->pstate & (ps_preEntry|ps_preStart)) ) {

        LpDmMsg->xosdRet = xosdBadThread;
        goto Exit;
    }

    lpeo = MHAlloc(sizeof(EXECUTE_OBJECT_DM));
    if (lpeo == NULL) {
        LpDmMsg->xosdRet = xosdOutOfMemory;
        goto Exit;
    }
    memset(lpeo, 0, sizeof(EXECUTE_OBJECT_DM));

    LpDmMsg->xosdRet = xosdNone;
    *((HIND *)LpDmMsg->rgb) = (HIND) lpeo;

    hthd->tstate |= ts_funceval;
    hthd->cFuncEval += 1;

Exit:
    Reply(sizeof(HIND), LpDmMsg, lpdbb->hpid);
    return;
}                               /* ProcessSetupExecute() */


VOID
ProcessStartExecuteCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
/*++

Routine Description:

    description-of-function.

Arguments:

    hprc        - Supplies the process handle for the operation
    hthd        - Supplies the thread handle for the operation
    lpdbb       - Supplies a pointer to possible futher information

Return Value:

    None.

--*/

{
    LPEXECUTE_STRUCT    lpes = (LPEXECUTE_STRUCT) (lpdbb->rgbVar);
    LPEXECUTE_OBJECT_DM lpeo;
    XOSD                xosd;


    /*
     *  Start by saving the information in the EXECUTE_STRUCT in
     *  the local Execute Object structure.
     */

    lpeo = (LPEXECUTE_OBJECT_DM) lpes->hindDm;

    DPRINT(1, ("Start Execute @%08x", lpes->addr.addr.off));

    lpeo->addrStart = lpes->addr;
    lpeo->pbpSave = AtBP(hthd);
    lpeo->fIgnoreEvents = lpes->fIgnoreEvents;
    lpeo->hthd = hthd;
    lpeo->lpbp = SetBP(hprc, hthd, bptpExec, bpnsStop, &(lpeo->addrStart), (HPID) lpeo);

    /*
     * Mark a breakpoint at the current address
     */

    SetBPFlag(hthd, lpeo->lpbp);

    /*
     *  Call a machine specific fucntion to setup stacks and some
     *  work in the Execute Object for later use.
     */

    xosd = SetupFunctionCall(lpeo, lpes);

    /*
     *  If the set up worked successfully, the next step to to register
     *  an expected debug event and
     *  step the child.
     */

    if (xosd == xosdNone) {
        SingleStep(hthd, &MthdFuncNotify, FALSE, TRUE);
    }

    LpDmMsg->xosdRet = xosd;
    Reply(0, LpDmMsg, lpdbb->hpid);
    return;
}                               /* ProcessStartExecute() */



VOID
ProcessCleanUpExecuteCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
/*++

Routine Description:

    description-of-function.

Arguments:

    hprc        - Supplies the process handle for the operation
    hthd        - Supplies the thread handle for the operation
    lpdbb       - Supplies a pointer to possible futher information

Return Value:

    None.

--*/

{
    LPEXECUTE_OBJECT_DM lpeo = *(LPEXECUTE_OBJECT_DM *) (lpdbb->rgbVar);

    DPRINT(1, ("Cleanup Execute"));

    hthd->cFuncEval -= 1;
    if (hthd->cFuncEval == 0) {
        hthd->tstate &= ~ts_funceval;
    }

    if (lpeo->lpbp) {
        RemoveBP(lpeo->lpbp);
    }

    /*
     * This is wrong if the BP could be removed during function evaluation
     */

    SetBPFlag(hthd, lpeo->pbpSave);

    LpDmMsg->xosdRet = xosdNone;
    Reply(0, LpDmMsg, lpdbb->hpid);

    MHFree( lpeo );
    return;
}                               /* ProcessCleanUpCmd() */



void
NotifyFuncCall(
    DEBUG_EVENT64 * lpde,
    HTHDX           hthd,
    DWORDLONG       unused,
    DWORDLONG       lparam
    )

/*++

Routine Description:

    This function is called as the notifyer function for expected events
    for executing of functions for the expression evaluator.

Arguments:

    lpde        - Supplies the pointer to the debug event exception structure

    hthd        - Supplies the handle to the thread of the exception

    lpv         - Supplies a pointer to extra data for the expectation

Return Value:

    None.

--*/

{
    Unreferenced( lpde );
    Unreferenced( lparam );

    /*
     *  Debug output first
     */

    DEBUG_PRINT("** NotifyFuncCall **\n");

    ContinueThread(hthd);

    return;
}                               /* NotifyFuncCall() */




VOID
EvntBreakpoint(
    DEBUG_EVENT64 *    de,
    HTHDX            hthd
    )

/*++

Routine Description:

    This routine is called if the current thread is processing a function
    evaluation and a breakpoint debug event occurs.  The routine checks
    to see if the desired breakpoint has been reached.  If it has not
    then the breakpoint will be consumed and the thread continued.  If it
    has then a dbcExecuteDone notification will be sent to the debugger.

Arguments:

    de          - Supplies a pointer to the current debug event.
    hthd        - Supplies the handle of the current thread

Return Value:

    None.

--*/

{
    LPEXECUTE_OBJECT_DM lpeo;
    PBREAKPOINT         lpbpp = (PBREAKPOINT) ((((DWORD_PTR)de->u.Exception.ExceptionRecord.NumberParameters) << 32) | (DWORD)de->u.Exception.ExceptionRecord.ExceptionCode);
    HIND                hzero = 0;
    
#if 0 //v-vadimp - this code is not right - third line should say ibpp = .....<<32 | ibpp, and will probably have pointer extension problems
    *DWORD_PTR           ibpp;
    ibpp = de->u.Exception.ExceptionRecord.ExceptionCode;
    ibpp = de->u.Exception.ExceptionRecord.NumberParameters << 32;
    lpbpp = (PBREAKPOINT) ibpp;
#endif
    
    de->u.Exception.ExceptionRecord.ExceptionCode = 0;
    de->u.Exception.ExceptionRecord.NumberParameters = 0;
    if (lpbpp) {
        lpeo = (LPEXECUTE_OBJECT_DM) lpbpp->id;
    } else {
        lpeo = NULL;
    }

    DPRINT(1, ("Hit Execute Breakpoint (%08x)", lpbpp));

    /*
     *  This may have been a recursive call.  TO deal with this we need
     *  to check that the stack pointers are the same as when we started
     *  doing the function evaluation.
     *
     *  If it is not ok on the stacks then merely continue.  Note that this
     *  requires doing a single step event.
     *
     */

    if ((lpeo != NULL) && !CompareStacks(lpeo)) {
        SetBPFlag(hthd, lpbpp);

#if defined(TARGET_i386) && defined(KERNEL)
        DecrementIP( hthd );
#endif

        DPRINT(1, ("  Compare Stacks fail\n"));

        SingleStep(hthd, &MthdFuncNotify, FALSE, TRUE);

    } else {

        DPRINT(1, ("  Execute done\n"));

        /*
         *  We really did finish so send a message to that effect
         */

        DMSendDebugPacket(dbcExecuteDone,
                          hthd->hprc->hpid,
                          hthd->htid,
                          sizeof(HIND),
                          &hzero
                          );
    }
    return;
}                               /* EvntBreakpoint() */



VOID
EvntException(
    DEBUG_EVENT64 *    de,
    HTHDX            hthd
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    argument-name - Supplies | Returns description of argument.
    .
    .

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{
    HIND hzero = 0;

    DPRINT(1, ("Execute -- Exececption Occured\n"));

    hthd->fExceptionHandled = TRUE;

    DMSendDebugPacket(dbcExecuteDone,
                      hthd->hprc->hpid,
                      hthd->htid,
                      sizeof(HIND),
                      &hzero
                      );
    return;
}                               /* EvntException() */



VOID
EvntExitProcess(
    DEBUG_EVENT64 *    de,
    HTHDX            hthd
    )
/*++

Routine Description:

    This function is called if an exit process event is called while
    a function is being evaluated.

    This is a pain to deal with.  First the function evaluation must
    be killed and then we need to do normal processing

Arguments:

    de   - Supplies the Exit Process Debug event
    hthd - Supplies the handle to the current thread

Return Value:

    None.

--*/

{
    HIND hzero = 0;

    DPRINT(1, ("Execute -- Exit Process Occurred\n"));

    hthd->fExceptionHandled = TRUE;

    DMSendDebugPacket(dbcExecuteDone,
                      hthd->hprc->hpid,
                      hthd->htid,
                      sizeof(HIND),
                      &hzero
                      );

    ProcessExitProcessEvent(de, hthd);

    return;
}                               /* EvntExitProcess() */
