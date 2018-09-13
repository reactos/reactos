#include "precomp.h"
#pragma hdrstop

#include "resource.h"

extern DMTLFUNCTYPE DmTlFunc;

extern DEBUG_EVENT64  falseSSEvent;
extern METHOD       EMNotifyMethod;

extern BYTE abEMReplyBuf[];
extern HINSTANCE hInstance; // The DM DLLs hInstance

void
SSActionRBAndContinue(
    DEBUG_EVENT64 *de,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    );


VOID
SendDBCErrorStep(
    HPRCX hprc
    )
/*++

Routine Description:

    This function notifies the user when an invalid step command is
    attempted

Arguments:

    hprc        - Supplies the thread handle to be stepped.

Return Value:

    None.

--*/
{
    char buf[1000];
    if (!LoadString(hInstance, IDS_CANT_TRACE, buf, sizeof(buf))) {
        assert(FALSE);
    }

    SendDBCError(hprc, xosdCannotStep, buf);
} // SendDBCErrorStep



VOID
SingleStep(
    HTHDX hthd,
    METHOD* notify,
    BOOL stopOnBP,
    BOOL fInFuncEval
    )
/*++

Routine Description:

    This function is used to do a single step operation on the specified
    thread.

Arguments:

    hthd        - Supplies the thread handle to be stepped.

    notify      -

    stopOnBp    - Supplies TRUE if a bp at current PC should cause a stop

    fInFuncEval - Supplies TRUE if called by the fucntion evaluation code

Return Value:

    None.

--*/
{
    ReturnStepEx( hthd, notify, stopOnBP, fInFuncEval, NULL, NULL, TRUE );
}


VOID
SingleStepEx(
    HTHDX hthd,
    METHOD* notify,
    BOOL stopOnBP,
    BOOL fInFuncEval,
    BOOL fDoContinue
    )
/*++

Routine Description:

    This function is used to do a single step operation on the specified
    thread.

Arguments:

    hthd        - Supplies the thread handle to be stepped.

    notify      -

    stopOnBp    - Supplies TRUE if a bp at current PC should cause a stop

    fInFuncEval - Supplies TRUE if called by the fucntion evaluation code

    fDoContinue - Supplies TRUE if thread should be continued

Return Value:

    None.

--*/
{
    ReturnStepEx( hthd, notify, stopOnBP, fInFuncEval, NULL, NULL, fDoContinue );
}


VOID
ReturnStep(
    HTHDX hthd,
    METHOD* notify,
    BOOL stopOnBP,
    BOOL fInFuncEval,
    LPADDR addrRA,
    LPADDR addrStack
    )
/*++

Routine Description:

    This function is used to do a single step operation on the specified
    thread.

Arguments:

    hthd        - Supplies the thread handle to be stepped.

    notify      -

    stopOnBp    - Supplies TRUE if a bp at current PC should cause a stop

    fInFuncEval - Supplies TRUE if called by the fucntion evaluation code

    addrRA      -

    addrStack    -

Return Value:

    None.

--*/
{
    ReturnStepEx( hthd, notify, stopOnBP, fInFuncEval, addrRA, addrStack, TRUE) ;
}


UOFFSET
GetReturnDestination(
    HTHDX   hthd
    )
/*++

Routine Description:

        Gets the destination of the return address from the current PC.

Arguments:


Return Value:


--*/
{
    ADDR    CurrentAddress;
    UOFFSET ReturnAddress = 0;

// BUGBUG 64 bit

    AddrFromHthdx (&CurrentAddress, hthd);

    assert (IsRet (hthd, &CurrentAddress));

    if (IsRet (hthd, &CurrentAddress)) {
#ifdef TARGET_i386

        ULONG   cb;

        DbgReadMemory (hthd->hprc,
                       STACK_POINTER (hthd),
                       &ReturnAddress,
                       sizeof (ReturnAddress),
                       &cb
                       );

#else

        ReturnAddress = GetNextOffset (hthd, FALSE);

#endif
    }

    return ReturnAddress;
}



VOID
ReturnStepEx(
    HTHDX hthd,
    METHOD* notify,
    BOOL stopOnBP,
    BOOL fInFuncEval,
    LPADDR addrRA,
    LPADDR addrStack,
    BOOL fDoContinue
    )
/*++

Routine Description:

    This function is used to do a single step operation on the specified
    thread.

Arguments:

    hthd        - Supplies the thread handle to be stepped.

    notify      -

    stopOnBp    - Supplies TRUE if a bp at current PC should cause a stop

    fInFuncEval - Supplies TRUE if called by the fucntion evaluation code

    addrRA      - Supplies address to step to

    addrStack    - Supplies

    fDoContinue - Supplies TRUE if thread should be continued now

Return Value:

    None.

--*/
{
    ADDR                currAddr;
    ADDR                currAddrActual;
    ACVECTOR            action = NO_ACTION;
    int                 lpf = 0;
    PBREAKPOINT         bp;
    LPCONTEXT           lpContext = &(hthd->context);
    ULONG               ContinueCmd;

#ifndef KERNEL
    //
    //  If we are stepping into an ORPC section, CheckAndSetup ... will take
    //  care of everything for us.  Just return.
    //

    if (CheckAndSetupForOrpcSection (hthd)) {
        return;
    }
#endif

    //
    //  Get the current IP of the thread
    //

    AddrFromHthdx(&currAddr, hthd);
    currAddrActual = currAddr;

    bp = AtBP(hthd);

    if (!stopOnBP && !bp) {
        bp = FindBP(hthd->hprc, hthd, bptpExec, (BPNS)-1, &currAddr, FALSE);
        SetBPFlag(hthd, bp);
    }

    //
    //  Check if we are on a BP
    //
    if (bp == EMBEDDED_BP) {

        DPRINT(3, ("-- At embedded BP, skipping it.\n"));

        //
        //  If it isnt a BP we set then just increment past it
        //  & pretend that a single step actually took place
        //


        ClearBPFlag(hthd);
        hthd->fIsCallDone = FALSE;
        if (addrRA == NULL) {
            IncrementIP(hthd);
            NotifyEM(&falseSSEvent, hthd, 0, 0);
        } else {
            AddrFromHthdx(&hthd->addrFrom, hthd);
            IncrementIP(hthd);
            hthd->fReturning = TRUE;
            SetupReturnStep(hthd, fDoContinue, addrRA, addrStack);
        }

    } else if (bp) {

        RestoreInstrBP(hthd, bp);
        ClearBPFlag(hthd);

        //
        // Issue the single step command
        //

        if (!addrRA) {
            RegisterExpectedEvent(hthd->hprc,
                                  hthd,
                                  EXCEPTION_DEBUG_EVENT,
                                  (DWORD_PTR)EXCEPTION_SINGLE_STEP,
                                  notify,
                                  SSActionReplaceByte,
                                  FALSE,
                                  (UINT_PTR)bp);

            SetupSingleStep(hthd, TRUE);
        } else {
            // Step over the bp and then continue.
           RegisterExpectedEvent(hthd->hprc,
                                  hthd,
                                  EXCEPTION_DEBUG_EVENT,
                                  (DWORD_PTR)EXCEPTION_SINGLE_STEP,
                                  DONT_NOTIFY,
                                  SSActionRBAndContinue,
                                  FALSE,
                                  (UINT_PTR)bp);

            SetupSingleStep(hthd, FALSE);
            AddrFromHthdx(&hthd->addrFrom, hthd);
            hthd->fReturning = TRUE;
            SetupReturnStep(hthd, fDoContinue, addrRA, addrStack);
        }

    } else {    // bp == NULL

        //
        // Determine if the current instruction is a breakpoint
        //  instruction.  If it is then based on the stopOnBP flag we either
        //  execute to hit the breakpoint or skip over it and create a
        //  single step event
        //

        IsCall(hthd, &currAddr, &lpf, FALSE);

#if 0
        //
        //  If there is an exception and we're passing it on to the program,
        //  we do not want to trace.
        //

        if (IsPassingException (hthd) && lpf == INSTR_TRACE_BIT) {
            lpf = INSTR_CANNOT_TRACE;
        }
#endif

        if (lpf == INSTR_CANNOT_STEP) {
            SendDBCErrorStep(hthd->hprc);
            return;
        }

        if (lpf == INSTR_BREAKPOINT) {

            if (stopOnBP) {
                /*
                 * We were instructed to stop on breakpoints
                 * Just issue an execute command and execute
                 * the breakpoint.
                 */

                ContinueProcess (hthd->hprc);
            } else {
                /*
                 * else just increment past it
                 * & pretend that a single step actually took place
                 */

                DPRINT(3, ("    At an embedded bp -- ignoring\n\r"));

                IncrementIP(hthd);

                hthd->fIsCallDone = FALSE;
                ClearBPFlag(hthd);
                if (notify) {
                    (notify->notifyFunction)(&falseSSEvent,
                                             hthd,
                                             0,
                                             notify->lparam);
                } else {
                    NotifyEM(&falseSSEvent, hthd, 0, 0);
                }
            }

        } else {


            if (lpf == INSTR_CANNOT_TRACE) {

                bp = SetBP( hthd->hprc,
                            hthd,
                            bptpExec,
                            bpnsStop,
                            &currAddr,
                            (HPID) INVALID);
                bp->isStep = TRUE;

            }


            //
            // Place this on our list of expected events
            //

            RegisterExpectedEvent(hthd->hprc,
                                  hthd,
                                  EXCEPTION_DEBUG_EVENT,
                                  (DWORD_PTR)EXCEPTION_SINGLE_STEP,
                                  notify,
                                  NO_ACTION,
                                  FALSE,
                                  0);

            //
            // Issue the single step command
            //

            if (!addrRA) {
                if (hthd->fDisplayReturnValues && IsRet (hthd, &currAddrActual)) {
                    AddrFromHthdx(&hthd->addrFrom, hthd);
                    NotifyEM(&FuncExitEvent, hthd, 0, (UINT_PTR)&currAddrActual);
                }

                SetupSingleStep(hthd, TRUE);
            } else {
                AddrFromHthdx(&hthd->addrFrom, hthd);
                hthd->fReturning = TRUE;
                SetupReturnStep(hthd, fDoContinue, addrRA, addrStack);
            }
        }
    }

    return;
}                               /* ReturnStepEx() */



void
IncrementIP(
    HTHDX hthd
    )
{
#if defined(TARGET_IA64)
    //
    // EM's IIP contains bundle address. Single step must be done on the
    // instruction slot.
    //
    DPRINT(1,("Incrementing IP: Was IIP: %I64x Slot:%i",
              hthd->context.StIIP,
              (hthd->context.StIPSR & IPSR_RI_MASK) >> PSR_RI));

    switch( PC(hthd) & 0xf ) {
        case 0:
            hthd->context.StIPSR = (hthd->context.StIPSR & ~(IPSR_RI_MASK)) |
                                     ((ULONGLONG)0x1 << PSR_RI);
            break;

        case 4:
            hthd->context.StIPSR = (hthd->context.StIPSR & ~(IPSR_RI_MASK)) |
                                     ((ULONGLONG)0x2 << PSR_RI);
            //
            // now let's see if we may have an MLI template here (we'll have
            // to move to slot 1 from slot 2 then
            //
            {
                ADDR addr;
                DWORD InstLen;
                ULONGLONG Template;
                AddrFromHthdx(&addr,hthd);
                GetAddrOff(addr) &= ~0xF;
                if (!AddrReadMemory(hthd->hprc,
                                    hthd,
                                    &addr,
                                    &Template,
                                    sizeof(ULONGLONG),
                                    &InstLen)) {
                    DPRINT(0,("AddrReadMemory @ %p failed in IncrementIP!",
                              GetAddrOff(addr)));
                }

                Template = (Template & INST_TEMPL_MASK) >> 1;
                if ( Template == 0x02 ) {
                    DPRINT(0,("Moving slot from 1 to 0 of the next bundle for MLI template\n"));
                    hthd->context.StIPSR = hthd->context.StIPSR & ~(IPSR_RI_MASK);
                    hthd->context.StIIP += BUNDLE_SIZE;
                }
            }
            break;

            case 8:
                hthd->context.StIPSR = hthd->context.StIPSR & ~(IPSR_RI_MASK);
                hthd->context.StIIP += BUNDLE_SIZE;
                break;
        
            default:
                assert(!"Bad Slot Number");
    }
    DPRINT(1,("NOW IIP: %I64x Slot:%i\n",
              hthd->context.StIIP,
              (hthd->context.StIPSR & IPSR_RI_MASK) >> PSR_RI));

#else

    Set_PC(hthd, PC(hthd) + BP_SIZE);

#endif

    assert(hthd->tstate & ts_stopped);
    hthd->fContextDirty = TRUE;

    return;
}                               /* IncrementIP() */




void
DecrementIP(
    HTHDX hthd
    )
{

#if defined(TARGET_IA64)

    //
    // EM's IIP contains bundle address. Single step must be done on the
    // instruction slot.
    //

    switch( PC(hthd) & 0xf ) {
        case 0:
            hthd->context.StIPSR = (hthd->context.StIPSR & ~(IPSR_RI_MASK)) |
                                     ((ULONGLONG)0x2 << PSR_RI);
            hthd->context.StIIP -= BUNDLE_SIZE;
            //
            // now let's see if we may have an MLI template here (we'll have
            // to move to slot 1 from slot 2 then
            //
            {
                ADDR addr;
                DWORD InstLen;
                ULONGLONG Template;
                AddrFromHthdx(&addr,hthd);
                GetAddrOff(addr) &= ~0xF;
                if (!AddrReadMemory(hthd->hprc,
                                    hthd,
                                    &addr,
                                    &Template,
                                    sizeof(ULONGLONG),
                                    &InstLen)) {
                    DPRINT(0,("AddrReadMemory @ %p failed in DecrementIP!",
                              GetAddrOff(addr)));
                }
    
                Template = (Template & INST_TEMPL_MASK) >> 1;
                if ( Template == 0x02 ) {
                    DPRINT(0,("Moving slot from 2 to 1 for MLI template\n"));
                    hthd->context.StIPSR = (hthd->context.StIPSR & ~(IPSR_RI_MASK)) |
                                            ((ULONGLONG)0x1 << PSR_RI);
                }
            }
            break;

        case 4:
            hthd->context.StIPSR = hthd->context.StIPSR & ~(IPSR_RI_MASK);
            break;

        case 8:
            hthd->context.StIPSR = (hthd->context.StIPSR & ~(IPSR_RI_MASK)) |
                                     ((ULONGLONG)0x1 << PSR_RI);
            break;

        default:
            assert(!"Bad Slot Number");
    }

#else

    // M00BUG -- two byte version of int 3
    Set_PC(hthd, PC(hthd) - BP_SIZE);

#endif

    assert(hthd->tstate & ts_stopped);
    hthd->fContextDirty = TRUE;

    return;
}                               /* DecrementIP() */


VOID
MoveIPToException(
    HTHDX hthd,
    LPDEBUG_EVENT64 pde
    )
/*++

Routine Description:

    This function moves the EIP for a thread to where an exception occurred.
    This is primarily used for breakpoints.  There are two advantages of
    this over simply decrementing the IP: (1) its CPU-independent, and
    (2) it helps work around an NT bug.  The NT bug is, if an app which is
    NOT being debugged has a hard-coded INT 3, and the user starts post-
    mortem debugging, then when NT gives us the INT 3 exception, it will
    give us the wrong EIP: it gives us the address of the INT 3, rather
    than one byte past that.  But it gives us the correct ExceptionAddress.

Arguments:


Return Value:

    None

--*/

{
#if defined(TARGET_IA64)
    hthd->context.StIIP = (UOFFSET) EXADDR(pde) & ~0xF;  //bundle address
    hthd->context.StIPSR &= ~IPSR_RI_MASK; //clear the bundle flag
    hthd->context.StIPSR |= (((UOFFSET) EXADDR(pde) & 0xF) >> 2) << PSR_RI; //set it to bundle <0,1,2> <-> <0,4,8> of the address
#else
    Set_PC(hthd, (IP_TYPE) EXADDR(pde) );
#endif
    assert(hthd->tstate & ts_stopped);
    hthd->fContextDirty = TRUE;
}



VOID
StepOver(
    HTHDX hthd,
    METHOD* notify,
    BOOL stopOnBP,
    BOOL fInFuncEval
    )
/*++

Routine Desription:


Arguments:


Return Value:


--*/
{
    ADDR        currAddr;
    int         lpf = 0;
    PBREAKPOINT bp;
    PBREAKPOINT atbp;
    LPCONTEXT   lpContext = &hthd->context;
    HPRCX       hprc=hthd->hprc;
    METHOD      *method;

    DPRINT(3, ("** SINGLE STEP OVER  "));

#ifndef KERNEL
    //
    //      if we are stepping into an ORPC section, the following function will
    //      take care of everythging for us and return TRUE.  Just return.
    //

    if (CheckAndSetupForOrpcSection (hthd)) {
        return;
    }
#endif // !KERNEL

    //
    //  Get the current IP of the thread
    //

    AddrFromHthdx(&currAddr, hthd);

    //
    //  Determine what type of instruction we are presently on
    //

    IsCall(hthd, &currAddr, &lpf, TRUE);

    //
    //  If the instruction is not a call or an intrpt then do a SS
    //

    if (lpf == INSTR_TRACE_BIT) {
        DPRINT(0, ("lpf == INSTR_TRACE_BIT: Doing SingleStep from Addr %I64x\n",GetAddrOff(currAddr)));
        SingleStep(hthd, notify, stopOnBP, fInFuncEval);
        return;
    } else if (lpf == INSTR_CANNOT_STEP) {
        DPRINT(0, ("lpf == INSTR_CANNOT_STEP: @ Addr %I64x\n",GetAddrOff(currAddr)));
        SendDBCErrorStep(hthd->hprc);
        return;
    }

    ExprBPContinue(hthd->hprc, hthd);

    //
    //  If the instruction is a BP then "uncover" it
    //

    if (lpf == INSTR_BREAKPOINT) {

        DPRINT(0, ("lpf == INSTR_BREAKPOINT:  We have hit a breakpoint instruction @ %I64x\n",GetAddrOff(currAddr)));

        if (stopOnBP) {
            /*
            **  We were instructed to stop on breakpoints
            **  Just issue an execute command and execute
            **  the breakpoint.
            */

            ContinueProcess (hthd->hprc);

        } else {

            IncrementIP(hthd);
            hthd->fIsCallDone = FALSE;
            ClearBPFlag(hthd);
            if (notify) {
                (notify->notifyFunction)(&falseSSEvent, hthd, 0, notify->lparam);
            } else {
                NotifyEM(&falseSSEvent, hthd, 0, 0);
            }
        }

    } else {

        /*
        **  If control gets to this point, then the instruction
        **  that we are at is either a call or an interrupt.
        */
        BOOL fDisplayReturnValues = hthd->fDisplayReturnValues && notify->lparam;

        DPRINT(0, ("lpf == INSTR_CALL or INTERRUPT @ %I64x\n",GetAddrOff(currAddr)));
        atbp = bp = AtBP(hthd);
        if (!stopOnBP && !atbp) {
            atbp = bp = FindBP(hthd->hprc, hthd, bptpExec, (BPNS)-1, &currAddr, FALSE);
            SetBPFlag(hthd, bp);
        }

        if (atbp) {
            /*
            ** Put the single step on our list of expected
            ** events and set the action to "Replace Byte and Continue"
            ** without notifying the EM
            */


            DPRINT(0, ("Restoring bp @ %I64x\n",GetAddrOff(bp->addr)));
            RestoreInstrBP(hthd, bp);
            ClearBPFlag(hthd);

            //
            // We don't want to Continue automatically when return values are
            // being displayed because we need to note the addr the Call is
            // going to after all the thunk walking is done.
            //
            if (!fDisplayReturnValues) {
                RegisterExpectedEvent(hthd->hprc,
                                        hthd,
                                        EXCEPTION_DEBUG_EVENT,
                                        (DWORD_PTR)EXCEPTION_SINGLE_STEP,
                                        DONT_NOTIFY,
                                        SSActionRBAndContinue,
                                        FALSE,
                                        (UINT_PTR)bp);
                /*
                **  Issue the single step
                */

                if (lpf != INSTR_CANNOT_TRACE) {
                    DPRINT(0, ("Setting SingleStep\n"));
                    SetupSingleStep(hthd, FALSE);
                }
            }
        }

        if (lpf == INSTR_IS_CALL || lpf == INSTR_CANNOT_TRACE) {

            DPRINT(0, ("lpf == INSTR_IS_CALL || lpf == INSTR_CANNOT_TRACE:  Placing a Breakpoint @ %I64x  ", 
                   GetAddrOff(currAddr)));

            /*
            **  Set a BP after this call instruction
            */

            bp = SetBP(hprc, hthd, bptpExec, bpnsStop, &currAddr, (HPID)INVALID);

            assert(bp);

            /*
            **  Make a copy of the notification method
            */

            method  = (METHOD*)MHAlloc(sizeof(METHOD));
            *method = *notify;

            //
            //  Store the breakpoint with this notification method
            //

            method->lparam2 = (LPVOID)bp;

            //
            // lparam is NULL if we're doing StepOver from disassembly,
            // which doesn't do a range step.  We'll have to make
            // ProcessSingleStep do a RangeStep to change this.
            //

            if (fDisplayReturnValues && lpf == INSTR_IS_CALL) {

                ACVECTOR action = NO_ACTION;
                DWORDLONG lParam = 0;
                RANGESTEP *rs = (RANGESTEP *)method -> lparam;   // ??? OK?

                rs -> fGetReturnValue = TRUE;
                rs -> safetyBP = bp;

                if ( atbp ) {
                    action = SSActionReplaceByte;
                    lParam = (DWORDLONG)atbp;
                }

                assert (!hthd -> fReturning);

                RegisterExpectedEvent(
                    hthd->hprc,
                    hthd,
                    EXCEPTION_DEBUG_EVENT,
                    (DWORD_PTR)EXCEPTION_SINGLE_STEP,
                    method,
                    action,
                    FALSE,
                    lParam);

                SetupSingleStep (hthd, TRUE);
                return;
            }




            /*
            ** Place this on our list of expected events
            ** (Let the action function do the notification, other-
            ** wise the EM will receive a breakpoint notification,
            ** rather than a single step notification).NOTE:
            ** This is the reason why we make a copy of the notif-
            ** ication method -- because the action function must
            ** know which notification method to use, in addition
            ** to the breakpoint that was created.
            */


            RegisterExpectedEvent(hthd->hprc,
                    hthd,
                    BREAKPOINT_DEBUG_EVENT,
                    (DWORD_PTR)bp,
                    DONT_NOTIFY,
                    SSActionRemoveBP,
                    FALSE,
                    (UINT_PTR)method);


            DPRINT(7, ("PID= %lx  TID= %lx\n", hprc->pid, hthd->tid));

        }


        /*
        **  Issue the execute command
        */

        ContinueProcess (hthd->hprc);

        //
        // If we hit a DIFFERENT BP while we are in the called routine we
        // must clear this (and ALL other consumable events) from the
        // expected event queue.
        //

    }
    return;
}                               /* StepOver() */



void
SSActionRemoveBP(
    DEBUG_EVENT64* de,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    )
/*++

Routine Description:

    This action function is called upon the receipt of a breakpoint
    event in order to remove the breakpoint and fake a single step event.
    Note that the lparam to this action function must be a METHOD* and
    the BREAKPOINT* to remove must be stored in the lparam2 field of the
    method.

Arguments:


Return Value:


--*/
{
    METHOD* method = (METHOD *)lparam;
    BREAKPOINT* bp = (BREAKPOINT*)method->lparam2;

    Unreferenced( de );

    DEBUG_PRINT("** SS Action Remove BP called\n");

    //
    // Remove the temporary breakpoint
    //
    RemoveBP(bp);

    //
    // Notify whoever is concerned, that a SS event has occured
    //
    (method->notifyFunction)(&falseSSEvent, hthd, 0, method->lparam);

    //
    // Free the temporary notification method.
    //
    MHFree(method);
}




void
SSActionReplaceByte(
    DEBUG_EVENT64 *de,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    )
/*++

Routine Description:

    This action function is called upon the receipt of a single step
    event in order to replace the breakpoint instruction (INT 3, 0xCC)
    that was written over.

Arguments:


Return Value:


--*/
{
    PBREAKPOINT bp = (PBREAKPOINT)lparam;
    Unreferenced( de );
    Unreferenced( unused );

    if (bp->hWalk) {
        DPRINT(5, ("** SS Action Replace Byte is really a data BP\n"));
        ExprBPResetBP(hthd, bp);
    } else {
        DPRINT(5, ("** SS Action Replace Byte at %d:%04x:%016I64x with %I64x\n",
                   ADDR_IS_FLAT(bp->addr), 
                   bp->addr.addr.seg, 
                   bp->addr.addr.off, 
                   (ULONG64) BP_OPCODE
                   ));
        WriteBreakPoint( bp );
    }

    return;
}



void
SSActionRBAndContinue(
    DEBUG_EVENT64 *de,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    )
/*++

Routine Description:

    This action function is called upon the receipt of a single step
    event in order to replace the breakpoint instruction (INT 3, 0xCC)
    that was written over, and then continuing execution.

Arguments:


Return Value:


--*/
{
    PBREAKPOINT bp = (PBREAKPOINT)lparam;
    Unreferenced( de );
    Unreferenced( unused );

    if (bp->hWalk) {
        //
        // Really a hardware BP, let walk manager fix it.
        //
        DPRINT(5, ("** SS Action RB and continue is really a data BP\n"));
        ExprBPResetBP(hthd, bp);
    } else {
        DPRINT(5, ("** SS Action RB and Continue: Replace byte @ %d:%04x:%016I64x with %I64x\n",
                    ADDR_IS_FLAT(bp->addr), 
                    bp->addr.addr.seg, 
                    bp->addr.addr.off, 
                    (ULONG64)BP_OPCODE
                    ));

        WriteBreakPoint( bp );
    }

    ContinueThread (hthd);
}


BOOL InsideRange( HTHDX, ADDR*, ADDR*, ADDR* );
BRANCH_LIST * GetBranchList ( HTHDX, ADDR*, ADDR* );
RANGESTRUCT * SetupRange ( HTHDX, ADDR*, ADDR*, BRANCH_LIST *, BOOL, BOOL, METHOD* );
VOID AddRangeBp( RANGESTRUCT*, ADDR*, BOOL );
VOID SetRangeBp( RANGESTRUCT* );
VOID RemoveRangeBp( RANGESTRUCT* );
BOOL GetThunkTarget( HTHDX, RANGESTRUCT*, ADDR*, ADDR* );

VOID RecoverFromSingleStep( ADDR*, RANGESTRUCT*);
BOOL ContinueFromInsideRange( ADDR*, RANGESTRUCT*);
BOOL ContinueFromOutsideRange( ADDR*, RANGESTRUCT*);



BOOL
SmartRangeStep(
    HTHDX       hthd,
    UOFFSET      offStart,
    UOFFSET      offEnd,
    BOOL        fStopOnBP,
    BOOL        fStepOver
    )

/*++

Routine Description:

    This function is used to implement range stepping the the DM.  Range
    stepping is used to cause all instructions between a pair of addresses
    to be executed.

    The segment is implied to be the current segment.  This is validated
    in the EM.

Arguments:

    hthd      - Supplies the thread to be stepped.

    offStart  - Supplies the initial offset in the range

    offEnd    - Supplies the final offset in the range

    fStopOnBP - Supplies TRUE if stop on an initial breakpoint

    fStepOver - Supplies TRUE if to step over call type instructions

Return Value:

    TRUE if successful.  If the disassembler fails, a breakpoint cannot
    be set or other problems, FALSE will be returned, and the caller will
    fall back to the slow range step method.

--*/

{
    BRANCH_LIST  *BranchList;
    METHOD       *Method;
    RANGESTRUCT  *RangeStruct;
    ADDR         AddrStart;
    ADDR         AddrEnd;

    //
    //  Initialize start and end addresses
    //

    AddrInit(&AddrStart, 0, PcSegOfHthdx(hthd), offStart,
                hthd->fAddrIsFlat, hthd->fAddrOff32, FALSE, hthd->fAddrIsReal);

    AddrInit(&AddrEnd, 0, PcSegOfHthdx(hthd), offEnd,
                hthd->fAddrIsFlat, hthd->fAddrOff32, FALSE, hthd->fAddrIsReal);



    //
    //  Locate all the branch instructions inside the range (and their
    //  targets if available) and obtain a branch list.
    //
    BranchList  = GetBranchList( hthd, &AddrStart, &AddrEnd );

    if (!BranchList) {
        return FALSE;
    }

    //
    //  Setup range step method
    //
    Method = (METHOD*)MHAlloc(sizeof(METHOD));
    assert( Method );

    Method->notifyFunction  = MethodSmartRangeStep;

    //
    //  Set up the range structure (this will set all safety breakpoints).
    //
    RangeStruct = SetupRange( hthd,
                              &AddrStart,
                              &AddrEnd,
                              BranchList,
                              fStopOnBP,
                              fStepOver,
                              Method );
    if (!RangeStruct) {
        MHFree(BranchList);
        return FALSE;
    }

    //
    //  Now let the thread run.
    //

    ContinueProcess (hthd->hprc);

#ifdef KERNEL
    hthd->tstate |= ts_stepping;
#endif

    return TRUE;
}



BOOL
InsideRange(
    HTHDX   hthd,
    ADDR   *AddrStart,
    ADDR   *AddrEnd,
    ADDR   *Addr
    )
{
    ADDR    AddrS;
    ADDR    AddrE;
    ADDR    AddrC;

    assert( AddrStart );
    assert( AddrEnd );
    assert( Addr );

    if ( ADDR_IS_LI(*Addr) ) {
        return FALSE;
    }

    AddrS = *AddrStart;
    AddrE = *AddrEnd;
    AddrC = *Addr;

    if (!ADDR_IS_FLAT(AddrS)) {
        if ( !TranslateAddress(hthd->hprc, hthd, &AddrS, TRUE) ) {
            return FALSE;
        }
    }

    if (!ADDR_IS_FLAT(AddrE)) {
        if ( !TranslateAddress(hthd->hprc, hthd, &AddrE, TRUE) ) {
            return FALSE;
        }
    }

    if (!ADDR_IS_FLAT(AddrC)) {
        if ( !TranslateAddress(hthd->hprc, hthd, &AddrC, TRUE) ) {
            return FALSE;
        }
    }

    if ( GetAddrOff( AddrC ) >= GetAddrOff( AddrS ) &&
                            GetAddrOff( AddrC ) <= GetAddrOff( AddrE ) ) {

        return TRUE;
    }

    return FALSE;
}



BRANCH_LIST *
GetBranchList (
    HTHDX   hthd,
    ADDR   *AddrStart,
    ADDR   *AddrEnd
    )
/*++

Routine Description:

    Locates all the branch instructions within a range and builds a
    branch list.

Arguments:

    hthd        -   Supplies thread

    AddrStart   -   Supplies start of range

    AddrEnd     -   Supplies end of range

Return Value:

    BRANCH_LIST *   -   Pointer to branch list.

--*/
{
    void        *Memory;
    BRANCH_LIST *BranchList = NULL;
    BRANCH_LIST *BranchListTmp;
    DWORD        RangeSize;
    LONG         Length;
    BYTE        *Instr;
    DWORD        ListSize;
    DWORD        i;
    ADDR         Addr;

    assert( AddrStart );
    assert( AddrEnd );

    RangeSize  =  (DWORD)(GetAddrOff(*AddrEnd) - GetAddrOff(*AddrStart) + 1);

    //
    //  Read the code.
    //
    Memory = MHAlloc( RangeSize );
    assert( Memory );

    if (!Memory) {
        return NULL;
    }

    //
    //  Allocate and initialize the branch list structure
    //
    ListSize   = sizeof( BRANCH_LIST );
    BranchList = (BRANCH_LIST *)MHAlloc( ListSize );

    assert( BranchList );

    if (!BranchList) {
        MHFree(Memory);
        return NULL;
    }

    BranchList->AddrStart = *AddrStart;
    BranchList->AddrEnd   = *AddrEnd;
    BranchList->Count     = 0;


    Addr = *AddrStart;

    AddrReadMemory(hthd->hprc, hthd, &Addr, Memory, RangeSize, &Length );
#ifndef KERNEL
    assert(Length==(LONG)RangeSize);
#endif
    //
    // If the code crosses a page boundary and the second
    // page is not present, the read will be short.
    // Fail, and we will fall back to the slow step code.
    //

    if (Length != (LONG)RangeSize) {
        MHFree(BranchList);
        MHFree(Memory);
        return NULL;
    }


    //
    //  Unassemble the code and determine where all branches are.
    //
    Instr  = (BYTE *)Memory;

#if defined(TARGET_IA64)
    // IA64 Address must begin at the bundle bundary or else can not be unassembled
    Instr += ((GetAddrOff(Addr) + 0xf) & ~(0xf) - GetAddrOff(Addr)); 
    GetAddrOff(Addr) = (GetAddrOff(Addr) + 0xf) & ~(0xf);
#endif

    while ( Length > 0 ) {

        BOOL    IsBranch;
        BOOL    TargetKnown;
        BOOL    IsCall;
        BOOL    IsTable;
        ADDR    Target;
        DWORD   Consumed;

        //
        //  Unassemble one instruction
        //
        Consumed = BranchUnassemble(hthd,
                                    (void *)Instr,
                                    Length,
                                    &Addr,
                                    &IsBranch,
                                    &TargetKnown,
                                    &IsCall,
                                    &IsTable,
                                    &Target );

        assert( Consumed > 0 );
        if ( Consumed == 0 ) {

            //
            //  Could not unassemble the instruction, give up.
            //

            MHFree(BranchList);
            BranchList = NULL;
            Length = 0;

        } else {

            if (IsBranch && IsTable &&
                    InsideRange(hthd, AddrStart, AddrEnd, &Target)) {

                //
                // this is a vectored jump with the table included
                // in the source range.  Rather than try to figure
                // out what we can and cannot disassemble, punt
                // here and let the slow step code catch it.
                //

                MHFree(BranchList);
                BranchList = NULL;
                Length = 0;

            } else if ( IsBranch ) {

                //
                //  If instruction is a branch, and the branch falls outside
                //  of the range, add a branch node to the list.
                //

                BOOLEAN fAdded = FALSE;

                if ( TargetKnown ) {
                    if ( ADDR_IS_FLAT(Target) ) {
                        if ( GetAddrOff(Target) != 0 ) {
                            GetAddrSeg(Target) = PcSegOfHthdx(hthd);
                        }
                    } else {
                        ADDR_IS_REAL(Target) = (BYTE)hthd->fAddrIsReal;
                    }
                }

                if ( !InsideRange( hthd, AddrStart, AddrEnd, &Target ) ||
                     !TargetKnown ) {

                    //
                    // this loop is to ensure that we dont get duplicate
                    // breakpoints set
                    //
                    for (i=0; i<BranchList->Count; i++) {

                        if ( TargetKnown &&
                        FAddrsEq( BranchList->BranchNode[i].Target, Target ) ) {
                            break;
                        }
                    }

                    if (i == BranchList->Count) {
                        ListSize += sizeof( BRANCH_NODE );
                        BranchListTmp = (BRANCH_LIST *)MHRealloc( BranchList, ListSize );
                        assert( BranchListTmp );
                        BranchList = BranchListTmp;

                        BranchList->BranchNode[ BranchList->Count ].TargetKnown= TargetKnown;
                        BranchList->BranchNode[ BranchList->Count ].IsCall = IsCall;
                        BranchList->BranchNode[ BranchList->Count ].Addr   = Addr;
                        BranchList->BranchNode[ BranchList->Count ].Target = Target;

                        BranchList->Count++;

                        fAdded = TRUE;
                    }
                }

            }

#if defined(TARGET_IA64)
    // each bundle must be unassembled three times before advancing to next bundle
    switch(GetAddrOff(Addr) & 0xf) {
        case 0:
        case 4:
            GetAddrOff(Addr) += 4;
            break;

        case 8:
            GetAddrOff(Addr) += 8;
            Instr += Consumed;
            break;

        default:
            Instr += ((GetAddrOff(Addr) + 0xf) & ~(0xf) - GetAddrOff(Addr)); 
            GetAddrOff(Addr) = (GetAddrOff(Addr) + 0xf) & ~(0xf);
            break;
        }
#else
            Instr            += Consumed;
            GetAddrOff(Addr) += Consumed;
#endif
            Length           -= Consumed;
        }
    }

    MHFree( Memory );

    return BranchList;
}



RANGESTRUCT *
SetupRange (
    HTHDX        hthd,
    ADDR        *AddrStart,
    ADDR        *AddrEnd,
    BRANCH_LIST *BranchList,
    BOOL         fStopOnBP,
    BOOL         fStepOver,
    METHOD      *Method
    )
/*++

Routine Description:

    Helper function for RangeStep.

Arguments:

    hthd        -   Supplies thread

    AddrStart   -   Supplies start of range

    AddrEnd     -   Supplies end of range

    BranchList  -   Supplies branch list

    fStopOnBP   -   Supplies fStopOnBP flag

    fStepOver   -   Supplies fStepOver flag

Return Value:

    RANGESTRUCT *   -   Pointer to range structure

--*/
{
    RANGESTRUCT *RangeStruct;
    BREAKPOINT  *Bp;
    DWORD        i;
    BOOLEAN      fAddedAtEndOfRange = FALSE;
    ADDR         Addr;
    CANSTEP      CanStep;
    UOFFSET      dwOffset;
    DWORD        dwSize;
    UOFFSET      dwPC;


    assert( AddrStart );
    assert( AddrEnd );
    assert( BranchList );
    assert( Method );

    //
    //  Allocate and initialize the range structure
    //
    RangeStruct = (RANGESTRUCT *)MHAlloc( sizeof(RANGESTRUCT) );
    assert( RangeStruct );

    RangeStruct->hthd        = hthd;
    RangeStruct->BranchList  = BranchList;
    RangeStruct->fStepOver   = fStepOver;
    RangeStruct->fStopOnBP   = fStopOnBP;
    RangeStruct->BpCount     = 0;
    RangeStruct->BpAddrs     = NULL;
    RangeStruct->BpList      = NULL;
    RangeStruct->fSingleStep = FALSE;
    RangeStruct->fInCall     = FALSE;
    RangeStruct->Method      = Method;

    Method->lparam           = (UINT_PTR)RangeStruct;


    DPRINT(0,("Setting step range to %I64x-%I64x\n",GetAddrOff(*AddrStart),GetAddrOff(*AddrEnd)));
    
    //
    //  If the given range has branches, set branch breakpoints according to
    //  the fStepOver flag.
    //
    if ( BranchList->Count > 0 ) {

        if ( fStepOver ) {

            //
            //  Ignore calls (Were stepping over them), set BPs in all
            //  known target (if outside of range) and all branch instructions
            //  with unknown targets.
            //
            for ( i=0; i < BranchList->Count; i++ ) {

                if ( !BranchList->BranchNode[i].IsCall ) {
                    if ( !BranchList->BranchNode[i].TargetKnown ) {

                        AddRangeBp( RangeStruct, &BranchList->BranchNode[i].Addr, FALSE );

                    } else if ( !InsideRange( hthd, AddrStart, AddrEnd, &BranchList->BranchNode[i].Target ) ) {

                        AddRangeBp( RangeStruct, &BranchList->BranchNode[i].Target, FALSE );
                    }
                }
            }

        } else {

            //
            //  Set BPs in all branches/calls with unknown targets, all
            //  branch targets (if outside of range) and all  call targets
            //  for which we have source.
            //
            for ( i=0; i < BranchList->Count; i++ ) {

                if ( !BranchList->BranchNode[i].TargetKnown ) {

                    AddRangeBp( RangeStruct, &BranchList->BranchNode[i].Addr, FALSE );

                } else if ( !InsideRange( hthd, AddrStart, AddrEnd, &BranchList->BranchNode[i].Target ) ) {

                    if ( !BranchList->BranchNode[i].IsCall ) {

                        AddRangeBp( RangeStruct, &BranchList->BranchNode[i].Target, FALSE );

                    } else {

                        //
                        //  BUGBUG - If debugging WOW, we dont set a
                        //  breakpoint in a function prolog, instead we set the
                        //  breakpoint in the call instruction and single step
                        //  to the function.
                        //
                        if (!ADDR_IS_FLAT(BranchList->BranchNode[i].Addr) ) {

                            AddRangeBp( RangeStruct, &BranchList->BranchNode[i].Addr, FALSE );

                        } else {

                            //
                            // Dont look for source lines until we hit the call target.
                            //
                            // If the call target is a thunk, we might not be able to interpret
                            // it until the thread is sitting on it.  Therefore, we always have
                            // to stop at the call target, so there is no point in looking now
                            // to see whether there is source data for it until we get there.
                            //

                            //
                            // Walk thunks, as far as we can.
                            //

                            dwPC = GetAddrOff(BranchList->BranchNode[i].Target);
                            while (IsThunk(hthd, dwPC, NULL, &dwPC, NULL )) {
                                GetAddrOff(BranchList->BranchNode[i].Target) = dwPC;
                            }

                            AddRangeBp( RangeStruct, &BranchList->BranchNode[i].Target, FALSE );

                        }
                    }
                }
            }
        }
    }

    if ( !fAddedAtEndOfRange ) {
        //
        //  We always set a safety breakpoint at the instruction past the end
        //  of the range.
        //
        ADDR Addr = *AddrEnd;
        GetAddrOff(Addr) += 1;
        AddRangeBp( RangeStruct, &Addr, FALSE );
    }

    //
    //  If we currently are at a BP and the address is not already in the
    //  list, then we must setup a single step for the instruction.
    //
    Bp = AtBP(hthd);

    if ( Bp == EMBEDDED_BP ) {

        //
        // we must step off the harcoded bp
        //

        ClearBPFlag( hthd );
        IncrementIP( hthd );
        hthd->fIsCallDone = FALSE;

    } else if ( Bp ) {

        //
        //  Make sure that the BP is not in the list
        //
        for ( i=0; i<RangeStruct->BpCount; i++ ) {
            if ( FAddrsEq( RangeStruct->BpAddrs[i], Bp->addr )) {
                break;
            }
        }

        if ( i >= RangeStruct->BpCount ) {
            //
            //  We have to single step the breakpoint.
            //
            ClearBPFlag( hthd );
            RestoreInstrBP( RangeStruct->hthd, Bp );
            RangeStruct->PrevAddr  = Bp->addr;
            RangeStruct->fSingleStep = TRUE;

            //
            //  Set the fInCall flag so that the stepping method knows whether
            //  or not it should stop stepping in case we get out of the range.
            //
            for ( i=0; i < RangeStruct->BranchList->Count; i++ ) {
                if ( FAddrsEq( Bp->addr,
                               RangeStruct->BranchList->BranchNode[i].Addr ) ) {
                    RangeStruct->fInCall =
                                 RangeStruct->BranchList->BranchNode[i].IsCall;
                    break;
                }
            }

#ifdef TARGET_i386

#ifndef KERNEL
            RangeStruct->hthd->context.EFlags |= TF_BIT_MASK;
            RangeStruct->hthd->fContextDirty = TRUE;
#endif

#elif defined (TARGET_IA64)

#ifndef KERNEL
            RangeStruct->hthd->context.StIPSR |= TF_BIT_MASK;
            RangeStruct->hthd->fContextDirty = TRUE;
#endif

#else   // i386
            {
                ADDR         Addr;
                UOFFSET      NextOffset;

                NextOffset = GetNextOffset( hthd, RangeStruct->fStepOver );

#ifndef KERNEL
                if ( NextOffset != 0x00000000 ) {
                    AddrInit( &Addr, 0, 0, NextOffset, TRUE, TRUE, FALSE, FALSE );
                    RangeStruct->TmpAddr = Addr;
                    RangeStruct->TmpBp = SetBP( RangeStruct->hthd->hprc,
                                                RangeStruct->hthd,
                                                bptpExec,
                                                bpnsStop,
                                                &Addr,
                                                (HPID) INVALID);
                }
                assert( RangeStruct->TmpBp );

#else   // KERNEL


                AddrInit( &Addr, 0, 0, NextOffset, TRUE, TRUE, FALSE, FALSE );

                GetCanStep(RangeStruct->hthd->hprc->hpid,
                           RangeStruct->hthd->htid,
                           &Addr,
                           &CanStep);

                if (CanStep.Flags == CANSTEP_YES) {
                    GetAddrOff(Addr) += CanStep.PrologOffset;
                }

                RangeStruct->TmpAddr = Addr;
                RangeStruct->TmpBp = SetBP( RangeStruct->hthd->hprc,
                                            RangeStruct->hthd,
                                            bptpExec,
                                            bpnsStop,
                                            &Addr,
                                            (HPID) INVALID);
                assert( RangeStruct->TmpBp );
#endif   // KERNEL

            }
#endif   // i386
        }
    }

    SetRangeBp( RangeStruct );

    return RangeStruct;
}

VOID
AddRangeBp(
    RANGESTRUCT *RangeStruct,
    ADDR        *Addr,
    BOOL         fSet
    )
/*++

Routine Description:

    Sets a breakpoint at a particular address and adds it to the breakpoint
    list in a RANGESTRUCT

Arguments:

    RangeStruct -   Supplies pointer to range structure

    Offset      -   Supplies flat address of breakpoint

    fSet        -   Supplies flag which if true causes the BP to be set

Return Value:

    None

--*/
{
    BREAKPOINT      **BpList;
    ADDR            *BpAddrs;
    DWORD           i;

    assert( RangeStruct );
    assert( Addr );

    //
    //  Add the breakpoint to the list in the range structure
    //
    if ( RangeStruct->BpList ) {
        assert( RangeStruct->BpCount > 0 );
        assert( RangeStruct->BpAddrs );

        //
        //  Do not add duplicates
        //
        for ( i=0; i<RangeStruct->BpCount; i++ ) {
            if ( FAddrsEq( RangeStruct->BpAddrs[i], *Addr ) ) {
                return;
            }
        }

        BpList  = ( BREAKPOINT** )MHRealloc( RangeStruct->BpList, sizeof( BREAKPOINT *) * (RangeStruct->BpCount + 1) );
        BpAddrs = ( ADDR* )MHRealloc( RangeStruct->BpAddrs, sizeof( ADDR ) * (RangeStruct->BpCount + 1) );
    } else {
        assert( RangeStruct->BpCount == 0 );
        assert( RangeStruct->BpAddrs == NULL );
        BpList  = ( BREAKPOINT** )MHAlloc( sizeof( BREAKPOINT * ) );
        BpAddrs = ( ADDR* )MHAlloc( sizeof( ADDR ) );
    }

    assert( BpList );
    assert( BpAddrs );

    BpList[RangeStruct->BpCount]   = NULL;
    BpAddrs[ RangeStruct->BpCount] = *Addr;

    if ( fSet ) {

        BpList[ RangeStruct->BpCount ] =
            SetBP( RangeStruct->hthd->hprc,
                    RangeStruct->hthd,
                    bptpExec,
                    bpnsStop,
                    Addr,
                    (HPID) INVALID
                    );

        assert( BpList[ RangeStruct->BpCount ] );
    }

    RangeStruct->BpCount++;
    RangeStruct->BpList     = BpList;
    RangeStruct->BpAddrs    = BpAddrs;
}


VOID
SetRangeBp(
    RANGESTRUCT *RangeStruct
    )
/*++

Routine Description:

    Sets the breakpoints in the range

Arguments:

    RangeStruct -   Supplies pointer to range structure

Return Value:

    None

--*/

{
    BOOL    BpSet;
    DWORD   Class;
    DWORD_PTR SubClass;

    assert( RangeStruct );

    if ( RangeStruct->fSingleStep ) {
#ifdef TARGET_i386
        Class    =  EXCEPTION_DEBUG_EVENT;
        SubClass =  (DWORD_PTR)STATUS_SINGLE_STEP;
#else
        Class    =  BREAKPOINT_DEBUG_EVENT;
        SubClass =  NO_SUBCLASS;
#endif
    } else {
        Class    =  BREAKPOINT_DEBUG_EVENT;
        SubClass =  NO_SUBCLASS;
    }

    //
    //  Register the expected breakpoint event.
    //
    RegisterExpectedEvent(RangeStruct->hthd->hprc,
                          RangeStruct->hthd,
                          Class,
                          SubClass,
                          RangeStruct->Method,
                          NO_ACTION,
                          FALSE,
                          0);

    if ( RangeStruct->BpCount ) {

        assert( RangeStruct->BpList );
        assert( RangeStruct->BpAddrs );

        //
        //  Set all the breakpoints at once
        //
        BpSet = SetBPEx( RangeStruct->hthd->hprc,
                        RangeStruct->hthd,
                        (HPID) INVALID,
                        RangeStruct->BpCount,
                        RangeStruct->BpAddrs,
                        RangeStruct->BpList,
                        0
                        //DBG_CONTINUE
                        );

        assert( BpSet );
    }
}

VOID
RemoveRangeBp(
    RANGESTRUCT *RangeStruct
    )
/*++

Routine Description:

    Sets the breakpoints in the range

Arguments:

    RangeStruct -   Supplies pointer to range structure

Return Value:

    None

--*/

{
    BOOL        BpRemoved;

    assert( RangeStruct );

    if ( RangeStruct->BpCount ) {

        assert( RangeStruct->BpList );
        assert( RangeStruct->BpAddrs );

        //
        //  Reset all the breakpoints at once
        //
        BpRemoved = RemoveBPEx( RangeStruct->BpCount,
                                RangeStruct->BpList
                                );
    }
}


void
MethodSmartRangeStep(
    DEBUG_EVENT64* de,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    )
{
    RANGESTRUCT* RangeStruct = (RANGESTRUCT*)lparam;
    ADDR    AddrCurrent;
    BOOL    fSingleStep = FALSE;

    assert( de );
    assert( RangeStruct );

    //
    //  Get the current address
    //
    AddrFromHthdx( &AddrCurrent, hthd );

    if ( RangeStruct->fSingleStep ) {
        //
        //  Recover from single step
        //
        RecoverFromSingleStep( &AddrCurrent, RangeStruct );
        fSingleStep = TRUE;
    }

    //
    //  See what we must do now.
    //
    if ( InsideRange( hthd,
                    &RangeStruct->BranchList->AddrStart,
                    &RangeStruct->BranchList->AddrEnd,
                    &AddrCurrent ) ) {


        //
        //  Still inside the range.
        //
        if ( ContinueFromInsideRange( &AddrCurrent, RangeStruct ) ) {
            return;
        }

    } else {

        //
        //  Outside the range
        //
        if ( fSingleStep && RangeStruct->fStepOver && RangeStruct->fInCall ) {
            //
            //  Recovering from a single step, continue.
            //
            RangeStruct->fInCall = FALSE;
            RegisterExpectedEvent(  RangeStruct->hthd->hprc,
                                    RangeStruct->hthd,
                                    BREAKPOINT_DEBUG_EVENT,
                                    NO_SUBCLASS,
                                    RangeStruct->Method,
                                    NO_ACTION,
                                    FALSE,
                                    0);

            ContinueThreadEx (hthd,
                              DBG_CONTINUE,
                              RangeStruct->fSingleStep ?
                                QT_TRACE_DEBUG_EVENT :
                                QT_CONTINUE_DEBUG_EVENT,
                              ts_running
                              );

            return;
        }
        if ( ContinueFromOutsideRange( &AddrCurrent, RangeStruct ) ) {
            return;
        }
    }

    //
    //  If we get here then we must clean up all the allocated resources
    //  and notify the EM.
    //

    if ( RangeStruct->BpCount > 0 ) {

        assert( RangeStruct->BpList );
        assert( RangeStruct->BpAddrs );

        RemoveRangeBp( RangeStruct );

        MHFree( RangeStruct->BpList );
        MHFree( RangeStruct->BpAddrs );
    }

    assert( RangeStruct->BranchList );
    MHFree( RangeStruct->BranchList );

    assert( RangeStruct->Method );
    MHFree( RangeStruct->Method );

    MHFree( RangeStruct );

    //
    //  Notify the EM that this thread has stopped.
    //
#ifdef KERNEL
    hthd->tstate &= ~ts_stepping;
#endif
    hthd->tstate &= ~ts_running;
    hthd->tstate |=  ts_stopped;
    NotifyEM(&falseSSEvent, hthd, 0, 0);
}



VOID
RecoverFromSingleStep(
    ADDR        *AddrCurrent,
    RANGESTRUCT* RangeStruct
    )
{
    BREAKPOINT *Bp;
    DWORD       i;
    BP_UNIT     opcode    = BP_OPCODE;

    assert( AddrCurrent );
    assert( RangeStruct );
    assert( RangeStruct->fSingleStep );



    //
    //  Recovering from a single step.
    //  Reset previous BP
    //

    Bp = FindBP( RangeStruct->hthd->hprc,
                 RangeStruct->hthd,
                 bptpExec,
                 (BPNS)-1,
                 &RangeStruct->PrevAddr,
                 FALSE );

    assert( Bp );

    if ( Bp ) {
        WriteBreakPoint( Bp );
    }



#ifdef TARGET_i386

#ifndef KERNEL
    //
    //  Clear trace flag
    //
    //
    RangeStruct->hthd->context.EFlags &= ~(TF_BIT_MASK);
    RangeStruct->hthd->fContextDirty = TRUE;

#endif  // KERNEL


#elif defined (TARGET_IA64)

#ifndef KERNEL
    //
    //  Clear trace flag
    //
    //
    RangeStruct->hthd->context.StIPSR &= ~TF_BIT_MASK;
    RangeStruct->hthd->fContextDirty = TRUE;

#endif  // KERNEL

#else   // TARGET_i386

    //
    //  Remove temporary breakpoint
    //
    assert( FAddrsEq( RangeStruct->TmpBp->addr, *AddrCurrent ) );

    assert( RangeStruct->TmpBp );
    RemoveBP( RangeStruct->TmpBp );
    RangeStruct->TmpBp = NULL;

#endif  // TARGET_i386

    RangeStruct->fSingleStep = FALSE;
}


BOOL
ContinueFromInsideRange(
    ADDR        *AddrCurrent,
    RANGESTRUCT *RangeStruct
    )
{

    DWORD       i;
    BREAKPOINT *Bp;
    UOFFSET     NextOffset;
    ADDR        NextAddr;
    BOOL        fContinue   = FALSE;

    assert( AddrCurrent );
    assert( RangeStruct );
    assert( !RangeStruct -> hthd -> fDisplayReturnValues); // AutoDisplay of return values code should be added to this routine.  See RAID 2914

    Bp = AtBP(RangeStruct->hthd);

    if ( RangeStruct->BranchList->Count > 0 && Bp ) {

        if ( Bp != EMBEDDED_BP ) {

            //
            //  Look for the branch node corresponding to this address.
            //  When found, determine the target address and set a
            //  safety breakpoint there if necessary. Then let the
            //  thread continue.
            //
            for ( i=0; i < RangeStruct->BranchList->Count; i++ ) {

                if ( FAddrsEq( RangeStruct->BranchList->BranchNode[i].Addr,
                                                            *AddrCurrent ) ) {

                    //
                    //  This is our guy.
                    //

                    //
                    //  Determine the next address
                    //
                    RangeStruct->fInCall = RangeStruct->BranchList->BranchNode[i].IsCall;
#ifdef TARGET_i386
                    UNREFERENCED_PARAMETER( NextOffset );
                    UNREFERENCED_PARAMETER( NextAddr );
#else   // TARGET_i386
                    NextOffset = GetNextOffset( RangeStruct->hthd, RangeStruct->fStepOver );
                    AddrInit(&NextAddr, 0, 0, NextOffset, TRUE, TRUE, FALSE, FALSE );
#endif  // TARGET_i386


                    //
                    //  We have to single step the current instruction.
                    //  We set a temporary breakpoint at the next offset,
                    //  recover the current breakpoint and set the flags to
                    //  reset the breakpoint when we hit the temporary
                    //  breakpoint.
                    //
                    ClearBPFlag( RangeStruct->hthd );
                    RestoreInstrBP( RangeStruct->hthd, Bp );
                    RangeStruct->PrevAddr = *AddrCurrent;

                    RangeStruct->fSingleStep = TRUE;
#ifdef TARGET_i386
#ifndef KERNEL
                    RangeStruct->hthd->context.EFlags |= TF_BIT_MASK;
                    RangeStruct->hthd->fContextDirty = TRUE;
#endif // KERNEL
#elif defined (TARGET_IA64)
#ifndef KERNEL
                    RangeStruct->hthd->context.StIPSR |= TF_BIT_MASK;
                    RangeStruct->hthd->fContextDirty = TRUE;
#endif // KERNEL
#else   // TARGET_i386
                    RangeStruct->TmpAddr  = NextAddr;
                    RangeStruct->TmpBp = SetBP( RangeStruct->hthd->hprc,
                                                RangeStruct->hthd,
                                                bptpExec,
                                                bpnsStop,
                                                &NextAddr,
                                                (HPID) INVALID);
                    assert( RangeStruct->TmpBp );
#endif  // TARGET_i386

                    //
                    //  Register the expected event.
                    //
                    RegisterExpectedEvent(
                                        RangeStruct->hthd->hprc,
                                        RangeStruct->hthd,
#if (defined(TARGET_i386) || defined(TARGET_IA64)) && !defined(NO_TRACE_FLAG)
                                        EXCEPTION_DEBUG_EVENT,
                                        (DWORD_PTR)STATUS_SINGLE_STEP,
#else   // TARGET_i386 ...
                                        BREAKPOINT_DEBUG_EVENT,
                                        NO_SUBCLASS,
#endif  // TARGET_i386 ...
                                        RangeStruct->Method,
                                        NO_ACTION,
                                        FALSE,
                                        0);

                    ContinueThreadEx (RangeStruct->hthd,
                                      DBG_CONTINUE,
                                      RangeStruct->fSingleStep ?
                                        QT_TRACE_DEBUG_EVENT :
                                        QT_CONTINUE_DEBUG_EVENT,
                                      ts_running
                                     );
                    fContinue = TRUE;
                    break;
                }
            }
        }

    } else {

        //
        //  We might end up here if continuing from a single step.
        //
        RegisterExpectedEvent( RangeStruct->hthd->hprc,
                               RangeStruct->hthd,
                               BREAKPOINT_DEBUG_EVENT,
                               NO_SUBCLASS,
                               RangeStruct->Method,
                               NO_ACTION,
                               FALSE,
                               0);

        ContinueThreadEx (RangeStruct->hthd,
                          DBG_CONTINUE,
                          RangeStruct->fSingleStep ?
                            QT_TRACE_DEBUG_EVENT :
                            QT_CONTINUE_DEBUG_EVENT,
                          ts_running
                         );

        fContinue = TRUE;
    }

    return fContinue;
}


BOOL
ContinueFromOutsideRange(
    ADDR        *AddrCurrent,
    RANGESTRUCT *RangeStruct
    )
{

    BOOL        fContinue = FALSE;
    ADDR        Addr;
    CANSTEP     CanStep;
    BREAKPOINT *Bp;

    assert( AddrCurrent );
    assert( RangeStruct );
    assert( !RangeStruct -> hthd -> fDisplayReturnValues); // AutoDisplay of return values code should be added to this routine.  See RAID 2914

    Bp = AtBP(RangeStruct->hthd);

    if (Bp == EMBEDDED_BP) {

        //
        // always stop.
        //

    } else if (!RangeStruct->fInCall) {

        //
        // if we weren't in a call, this should just be some other line
        // of code in the same function (or the parent function?), so stop.
        //

    } else {

        //
        // stopping after a call instruction.
        // this might actually not be a new function; we just know
        // that there was a call instruction in the source line.
        //

        GetCanStep(RangeStruct->hthd->hprc->hpid,
                   RangeStruct->hthd->htid,
                   AddrCurrent,
                   &CanStep
                   );

        switch ( CanStep.Flags ) {

        case CANSTEP_YES:

            //
            // if there is a known prolog, run ahead to the end
            //
            if ( CanStep.PrologOffset > 0 ) {

                Addr = *AddrCurrent;
                GetAddrOff(Addr) += CanStep.PrologOffset;
                AddRangeBp( RangeStruct, &Addr, TRUE );

                RegisterExpectedEvent(
                    RangeStruct->hthd->hprc,
                    RangeStruct->hthd,
                    BREAKPOINT_DEBUG_EVENT,
                    NO_SUBCLASS,
                    RangeStruct->Method,
                    NO_ACTION,
                    FALSE,
                    0);

                ClearBPFlag( RangeStruct->hthd );
                if ( Bp ) {
                    RestoreInstrBP( RangeStruct->hthd, Bp );
                }

                ContinueThread (RangeStruct->hthd);
                fContinue = TRUE;
            }
            break;

        case CANSTEP_NO:

            //
            // no source here.
            // step some more...
            // what is going to happen here? do we just keep stepping
            // until we get somewhere that has source?
            //

            //
            //  Register the expected event.
            //
            RegisterExpectedEvent(
                RangeStruct->hthd->hprc,
                RangeStruct->hthd,
                BREAKPOINT_DEBUG_EVENT,
                NO_SUBCLASS,
                RangeStruct->Method,
                NO_ACTION,
                FALSE,
                0);

            ClearBPFlag( RangeStruct->hthd );
            if ( Bp ) {
                RestoreInstrBP( RangeStruct->hthd, Bp );
            }

            ContinueThreadEx (RangeStruct->hthd,
                              DBG_CONTINUE,
                              RangeStruct->fSingleStep ?
                                QT_TRACE_DEBUG_EVENT :
                                QT_CONTINUE_DEBUG_EVENT,
                              ts_running
                              );
            fContinue = TRUE;
            break;
        }
    }

    return fContinue;
}

VOID
RangeStepContinue(
        HTHDX   hthd,
        LPVOID  Args
        );


typedef struct _RANGE_STEP_CONTINUE_ARGS
{
    UOFFSET offStart;
    UOFFSET offEnd;
    BOOL    fStopOnBP;
    BOOL    fstepOver;
} RANGE_STEP_CONTINUE_ARGS;

VOID
RangeStep(
    HTHDX       hthd,
    UOFFSET     offStart,
    UOFFSET     offEnd,
    BOOL        fStopOnBP,
    BOOL        fstepOver
    )
/*++

Routine Description:

    This function is used to implement range stepping the the DM.  Range
    stepping is used to cause all instructions between a pair of addresses
    to be executed.

    The segment is implied to be the current segment.  This is validated
    in the EM.

    Range stepping is done by registering an expected debug event at the
    end of a step and seeing if the current program counter is still in
    the correct range.  If it is not then the range step is over, if it
    is then a new event is register and we loop.

Arguments:

    hthd      - Supplies the thread to be stepped.

    offStart  - Supplies the initial offset in the range

    offEnd    - Supplies the final offset in the range

    fStopOnBP - Supplies TRUE if stop on an initial breakpoint

    fStepOver - Supplies TRUE if to step over call type instructions

Return Value:

    None.

Comments:

    This function has been broken into two.  RangeStep and RangeStepContinue.
    See RangeStepContinue for most of the functionality.

--*/
{
    RANGE_STEP_CONTINUE_ARGS*       Args;

    Args = (RANGE_STEP_CONTINUE_ARGS*) MHAlloc (sizeof (*Args));

    Args->offStart = offStart;
    Args->offEnd = offEnd;
    Args->fStopOnBP = fStopOnBP;
    Args->fstepOver = fstepOver;

#ifndef KERNEL
    EnsureOleRpcStatus (hthd, RangeStepContinue, Args);
#else
    RangeStepContinue(hthd, Args);
#endif
}


VOID
RangeStepContinue(
    HTHDX   hthd,
    LPVOID  Args
    )
/*++

Routine Description:

    See the function RangeStep for a description of the Args fields.  In the
    normal case -- where we are not changing our OLE debugging state -- this
    function is just called from EnsureOleRpcStatus ().

--*/
{
    RANGE_STEP_CONTINUE_ARGS*       rscArgs = (RANGE_STEP_CONTINUE_ARGS*) Args;

    UOFFSET offStart = rscArgs->offStart;
    UOFFSET offEnd   = rscArgs->offEnd;
    BOOL    fStopOnBP= rscArgs->fStopOnBP;
    BOOL    fstepOver= rscArgs->fstepOver;

    RANGESTEP * rs;
    METHOD *    method;
    HPRCX       hprc = hthd->hprc;
    int         lpf  = 0;
    ADDR        addr;


    MHFree (rscArgs);
    rscArgs = NULL;

    //
    //  Create and fill a range step structure
    //

    rs = (RANGESTEP*) MHAlloc(sizeof(RANGESTEP));
    rs->hthd        = hthd;
    rs->addrStart   = offStart;
    rs->addrEnd     = offEnd;
    rs->segCur      = PcSegOfHthdx(hthd);
    rs->fInThunk    = FALSE;
    rs->safetyBP    = NULL;
    rs->SavedSeg    = 0;
    rs->SavedAddrStart = 0;
    rs->SavedAddrEnd = 0;
#if defined (TARGET_IA64)
    rs->fSkipProlog = TRUE;
#else
    rs->fSkipProlog = FALSE;
#endif
    rs->fGetReturnValue = FALSE;
    rs->fIsCall     = FALSE;

    //
    //  Create a notification method for this range step
    //

    method  = (METHOD*) MHAlloc(sizeof(METHOD));
    method->notifyFunction  = MethodRangeStep;
    method->lparam          = (UINT_PTR)rs;
    rs->method              = method;

    if ( fstepOver ) {
        rs->stepFunction = StepOver;
    } else {
        rs->stepFunction = SingleStep;

       /*
        *  Check to see if we are currently at a call instruction.  If we
        *      are then we need to set a breakpoint at the end of the call
        *      instruction as the "safety" breakpoint.
        *
        *      This will allow us to recover back to the current level
        *      if the call we are just about to step into does not have
        *      any source information (in which case the range step
        *      is defined to continue).
        */

        AddrInit(&addr, 0, rs->segCur, offStart,
                hthd->fAddrIsFlat, hthd->fAddrOff32, FALSE, hthd->fAddrIsReal);
        IsCall( hthd, &addr, &lpf, FALSE);
        if ( lpf == INSTR_IS_CALL ) {
            rs->safetyBP = SetBP(hprc, hthd, bptpExec, bpnsStop, &addr, (HPID)INVALID);
            rs->fIsCall = TRUE;
        } else if (lpf == INSTR_CANNOT_STEP) {
            SendDBCErrorStep(hthd->hprc);
            return;
        }
    }

    //
    //  Call the step over function to send notifications
    //  to the RangeStepper (NOT THE EM!)
    //

    if (rs->fIsCall) {
        SingleStep(hthd, method, fStopOnBP, FALSE);
    } else {
        (rs->stepFunction)(hthd, method, fStopOnBP, FALSE);
    }

}                               /* RangeStep() */




VOID
MethodRangeStep(
    DEBUG_EVENT64* de,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    )
/*++

Routine Description:

    This is the range step method for the "stupid" or single stepping range
    step method.  This is called for every instruction executed during a range
    step, except when it has decided to run free to the safety breakpoint.  It
    does that when the destination code has no source line information.

Arguments:

    de - Supplies the single step or breakpoint debug event

    hthd - Supplies the thread

    unused -

    rs - Supplies a structure containing state information for the step

Return Value:

    none

--*/
{
    RANGESTEP* rs = (RANGESTEP*)lparam;

    LPCONTEXT   lpContext   = &hthd->context;
    DWORDLONG   currAddr    = cPC(lpContext);
    int         lpf         = 0;
    HPRCX       hprc        = hthd->hprc;
    METHOD *    method;
    ADDR        AddrPC;
    ADDR        AddrTmp;
    HPID        hpid = hprc->hpid;
    HTID        htid = hthd->htid;
    CANSTEP     CanStep;
    PBREAKPOINT bp;
    DWORD       dwSize;

    Unreferenced( de );

    DEBUG_PRINT_3("** MethodRangeStep called: %08x-%08x  PC=%08x",
                    rs->addrStart, rs->addrEnd, currAddr);

    AddrFromHthdx(&AddrPC, hthd);

    //
    //  auto return value

    if (hthd -> fReturning) {
        //
        // Out of a call.
        //
        hthd -> fReturning = FALSE;

        assert (rs -> fGetReturnValue);
        //
        // Got it!
        //
        rs->fGetReturnValue = FALSE;

        NotifyEM(&FuncExitEvent, hthd, 0, (DWORDLONG)&hthd->addrFrom );
    }


    //
    //  see if we ran past a call or hit a BP.
    //

    bp = AtBP(hthd);

    if (bp && bp == rs->safetyBP) {

        //
        // we stepped over the function.  continue the range step...
        //

        //
        // Note: we only get here if we didn't stop in the called function.
        // In the usual case, the safety BP has been changed into an expected
        // event and is gone by the time we get here.
        //
        RemoveBP(bp);
        rs->safetyBP = NULL;
        rs->fIsCall = FALSE;

    } else if (bp) {

        //
        // always stop on a real bp.
        //

        goto EndStep;

    } else if (rs->fIsCall) {

        rs->fIsCall = FALSE;

        if (rs->stepFunction == StepOver) {

            //
            // This is the destination of a CALL which we are supposed to
            // step over.
            // if it is a nested function, we change it into a "Step Into",
            // otherwise we want to just run to the Safety BP.
            //

            if (GetAddrOff(rs->CallSiteInfo.AddrStart) > 0 &&
                GetAddrOff(AddrPC) >= GetAddrOff(rs->CallSiteInfo.AddrStart) &&
                GetAddrOff(AddrPC) <= GetAddrOff(rs->CallSiteInfo.AddrEnd)) {

                //
                // it's nested - (try to) stay here.
                //

                ;

            } else {
                //
                // We don't want to go here, so continue.
                //
                // make the safety BP an expected event and run free.
                //

                method = (METHOD*)MHAlloc(sizeof(METHOD));
                *method = *rs->method;

                method->lparam2 = (LPVOID)rs->safetyBP;

                RegisterExpectedEvent(
                                hthd->hprc,
                                hthd,
                                BREAKPOINT_DEBUG_EVENT,
                                (DWORD_PTR)rs->safetyBP,
                                DONT_NOTIFY,
                                SSActionRemoveBP,
                                FALSE,
                                (UINT_PTR)method);

                //
                // The safety is the expected event now, so the rs struct
                // can forget about it.
                //

                rs->safetyBP = NULL;

                ContinueThread (hthd);
                return;
            }
        }
    }


    //
    //  Check if we are still within the range
    //

    if ((rs->addrStart > currAddr) ||
            (currAddr > rs->addrEnd) ||
            (PcSegOfHthdx(hthd) != rs->segCur)) {

                // For thunking.
        if (rs->SavedAddrStart) {
            rs->addrStart = rs->SavedAddrStart;
            rs->addrEnd = rs->SavedAddrEnd;
            rs->segCur = rs->SavedSeg;
            rs->SavedAddrStart = 0;
            rs->SavedAddrEnd = 0;
            rs->SavedSeg = 0;
        }
    }

    if ((rs->addrStart <= currAddr) &&
            (currAddr <= rs->addrEnd) &&
            (PcSegOfHthdx(hthd) == rs->segCur)) {

        //
        //  We still are in the range, continue stepping
        //

        //
        //  On Win95 if we try to step into a system call, the system
        //  forces execution to the instruction after the call and sends
        //  a single step.  This is because it doesnt want user-mode
        //  debuggers to step into system code.  In that case we are
        //  already at the safety bp and we should consume it.
        //

        if (IsChicago() && rs->safetyBP &&
            (rs->stepFunction != (STEPPER)StepOver || rs->fGetReturnValue))
        {
            bp = FindBP(hprc, hthd, bptpExec, (BPNS)-1, &AddrPC, FALSE);
            if (bp == rs->safetyBP) {
                RemoveBP(rs->safetyBP);
                rs->safetyBP = NULL;
            }

            if (rs->fGetReturnValue) {
                rs->fGetReturnValue = FALSE;
            }
        }


        AddrTmp = AddrPC;
        IsCall(hthd, &AddrPC, &lpf, FALSE);

        if (lpf != INSTR_IS_CALL) {

            (rs->stepFunction)(hthd, rs->method, TRUE, FALSE);

        } else {

            //
            // Get the function information for the call site,
            // so we can tell when we get to the target whether
            // it is a nested function.
            //
            ZeroMemory(&rs->CallSiteInfo, sizeof(FUNCTION_INFORMATION));
            DMSendRequestReply(dbceGetFunctionInformation,
                               hpid,
                               htid,
                               sizeof(ADDR),
                               &AddrTmp,
                               sizeof(FUNCTION_INFORMATION),
                               &rs->CallSiteInfo
                               );


            //
            // Before we step into this function, lets
            // put a "safety-net" breakpoint on the instruction
            // after this call. This way if we don't have
            // source for this function, we can always continue
            // and break at this safety-net breakpoint.
            //
            assert(!rs->safetyBP);
            rs->safetyBP = SetBP(hprc, hthd, bptpExec, bpnsStop, &AddrPC, (HPID)INVALID);
            rs->fIsCall = TRUE;

#ifndef KERNEL
            if (rs->stepFunction != (STEPPER)StepOver) {

                //
                // Are we stepping into ORPC code?
                //

                if (hthd->hprc->OrpcDebugging == ORPC_DEBUGGING) {

                    RegisterExpectedEvent (hthd->hprc,
                                           hthd,
                                           OLE_DEBUG_EVENT,
                                           orpcClientGetBufferSize,
                                           DONT_NOTIFY,
                                           ActionOrpcClientGetBufferSize,
                                           FALSE,
                                           0
                                           );
                }
            }
#endif // !KERNEL

            SingleStep(hthd, rs->method, TRUE, FALSE);

        }
    } else {

        //
        // we have left the range.
        //
        // If we are at the safety bp && chicago && return values,
        //    we have attempted to step over a system call, failed,
        //    and ended up here.  Remove the safety bp and continue on.
        //
        // If we are at an NLG_RETURN label, we need to continue on.
        //
        // If there is source here,
        //   if we are in a prolog
        //      run to the end of the prolog
        //   else
        //      stop.
        //
        // If we were in a thunk or a call was pending,
        // we have either hit a thunk or a new function.
        //
        //  if we hit a thunk
        //      set the range to cover the thunk,
        //      set the thunk flag
        //      and continue stepping.
        //  else
        //      run to the safety bp
        //
        // if there is no source and no safety bp, stop.
        //

        //
        // Ask the debugger if we can step on this instruction
        //


        GetCanStep(hthd->hprc->hpid, hthd->htid, &AddrPC, &CanStep);

        if (CanStep.Flags == CANSTEP_YES && !rs -> fGetReturnValue ) {
            if (rs->safetyBP) {
                RemoveBP(rs->safetyBP);
                rs->safetyBP = NULL;
            }
        }

#ifndef KERNEL
        if (CheckNLG(hthd->hprc, hthd, NLG_RETURN, &AddrPC)) {
            // We should have just stepped over a ret instruction,
            // there should be no safety BP's.
            assert(rs->safetyBP == NULL);
            SetupNLG(hthd, NULL);
            ContinueThread (hthd);

        } else
#endif // !KERNEL
        if ((CanStep.Flags == CANSTEP_YES) && (rs->fSkipProlog) &&
            (CanStep.PrologOffset > 0) && !rs -> fGetReturnValue ) {

            //
            // if there is a known prolog, run ahead to the end
            //

            ADDR Addr = AddrPC;
            GetAddrOff(Addr) += CanStep.PrologOffset;
            bp = SetBP(hprc, hthd,  bptpExec, bpnsStop, &Addr, (HPID)INVALID);
            assert(bp);

            method = (METHOD*)MHAlloc(sizeof(METHOD));
            *method = *rs->method;

            method->lparam2 = (LPVOID)bp;

            RegisterExpectedEvent(
                    hthd->hprc,
                    hthd,
                    BREAKPOINT_DEBUG_EVENT,
                    NO_SUBCLASS,
                    DONT_NOTIFY,
                    SSActionRemoveBP,
                    FALSE,
                    (UINT_PTR)method);

            ContinueThread (hthd);

        } else if ((CanStep.Flags == CANSTEP_THUNK) ||
                   (CanStep.Flags == CANSTEP_NO) &&
                   (rs->safetyBP || rs->fInThunk) || rs -> fGetReturnValue) {

            //
            // came from a call or thunk, and found no source.
            //

            rs->fInThunk = IsThunk(hthd, PC(hthd), NULL, NULL, &dwSize );

            if (rs->fInThunk) {
                // TODO what about THUNK_SYSTEM here??

                //
                // set new range and continue.
                //

                rs->SavedAddrStart = rs->addrStart;
                rs->SavedAddrEnd = rs->addrEnd;
                rs->SavedSeg = rs->segCur;
                rs->addrStart = currAddr;
                rs->addrEnd = currAddr + dwSize;
                rs->segCur = PcSegOfHthdx(hthd);

                (rs->stepFunction)(hthd, rs->method, TRUE, FALSE);

            } else if (!rs->safetyBP) {
                //
                // came from a thunk, but there was no BP
                //
                assert(0);
            } else {

                if (rs -> fGetReturnValue) {
                    hthd -> fReturning = TRUE;
                    //
                    // Must the seg be set as well?
                    //
                    SE_SetAddrOff (&hthd -> addrFrom, currAddr);
                }

                //
                // make the safety BP an expected event and run free.
                //

                method = (METHOD*)MHAlloc(sizeof(METHOD));
                *method = *rs->method;

                method->lparam2 = (LPVOID)rs->safetyBP;

                RegisterExpectedEvent(
                                hthd->hprc,
                                hthd,
                                BREAKPOINT_DEBUG_EVENT,
                                (DWORD_PTR)rs->safetyBP,
                                DONT_NOTIFY,
                                (ACVECTOR) SSActionRemoveBP,
                                FALSE,
                                (DWORDLONG)method);

                //
                // The safety is the expected event now, so the rs struct
                // can forget about it.
                //

                rs->safetyBP = NULL;

                ContinueThread (hthd);
            }
        } else {

EndStep:

            DEBUG_PRINT("  Ending range step\n");

            //
            // We are no longer in the range, free all consumable
            // events on the queue for this thread
            //
            ConsumeAllThreadEvents(hthd, FALSE);

            //
            //  Free the structures created for range-stepping
            //
            if (rs->safetyBP) {
                RemoveBP(rs->safetyBP);
            }
            MHFree(rs->method);
            MHFree(rs);

            //
            //  Notify the EM that this thread has stopped on a SS
            //
            hthd->tstate &= ~ts_running;
            hthd->tstate |=  ts_stopped;
            NotifyEM(&falseSSEvent, hthd, 0, 0);

        }
    }

    return;
}                   /* MethodRangeStep */


void
WtPrintCallNode(
    LPWTNODE wt
    )
{
    DWORD i;
    static CHAR margin[] =
"                                                                                ";
    i = wt->lex*3;
    if (i > 60) {
        i = 60;
    }
    DMPrintShellMsg( "%4d  %4d  %*.*s%s\r\n",
        wt->icnt,
        wt->scnt,
        i, i, margin,
        wt->fname );
}

void
WtGetSymbolName(
    HTHDX    hthd,
    LPADDR   lpaddr,
    LPSTR   *lpFname,
    LPUOFFSET  lpdwSymAddress,
    LPUOFFSET  lpdwReturnAddress
    )
/*++

Routine Description:



Arguments:


Return Value:


--*/
{
    DMSYM   DmSym;

    __try {

        DMSendRequestReply(dbceGetSymbolFromOffset,
                           hthd->hprc->hpid,
                           hthd->htid,
                           sizeof(ADDR),
                           lpaddr,
                           sizeof(DMSYM),
                           &DmSym
                           );

        *lpFname = MHStrdup( DmSym.fname );
        *lpdwSymAddress = GetAddrOff(DmSym.AddrSym);
        *lpdwReturnAddress = DmSym.Ra;

    } __except(EXCEPTION_EXECUTE_HANDLER) {

        *lpFname = NULL;
        *lpdwReturnAddress = 0;
        *lpdwSymAddress = 0;

    }
}


/***    WtMethodRangeStep
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**      This method is called upon the receipt of a single step event
**      while inside of a range step. It checks if the IP is still in the
**      specified range, if it isnt then the EM is notified that the
**      process has stopped outside the range, and all the RS structs and
**      notification method are freed.
*/

void
WtMethodRangeStep(
    DEBUG_EVENT64  *de,
    HTHDX        hthd,
    DWORDLONG    unused,
    DWORDLONG    lparam
    )
{
    RANGESTEP   *rs = (RANGESTEP*)lparam;
    UOFFSET     currAddr    = PC(hthd);
    HPRCX       hprc        = hthd->hprc;
    ADDR        addr;
    LPWTNODE    wt;
    LPWTNODE    wt1;
    UOFFSET     ra;
    UOFFSET     symaddr;


    AddrInit( &addr, 0, PcSegOfHthdx(hthd), currAddr,
              hthd->fAddrIsFlat, hthd->fAddrOff32, FALSE, hthd->fAddrIsReal );

    hthd->wtcurr->icnt++;

    if (rs->fIsRet) {
        rs->fIsRet = FALSE;
        WtPrintCallNode( hthd->wtcurr );
        if (hthd->wtcurr->caller) {
            wt1 = hthd->wtcurr;
            wt = wt1->caller;
            wt->scnt += wt1->icnt + wt1->scnt;
            wt->callee = NULL;
            hthd->wtcurr = wt;
            MHFree(wt1);
        }
    }


    if (rs->addrEnd == 0 || currAddr == rs->addrEnd || hthd->wtmode == 2) {

        //
        // unwind the stack, print totals
        //

        wt = hthd->wtcurr;
        while (wt) {
            WtPrintCallNode( wt );
            if (wt1 = wt->caller ) {
                wt1->scnt += wt->icnt + wt->scnt;
                MHFree(wt);
            }
            wt = wt1;
        }

    finished:
        hthd->wtmode = 0;
        ConsumeAllThreadEvents(hthd, FALSE);
        MHFree(rs->method);
        MHFree(rs);
        hthd->tstate &= ~ts_running;
        hthd->tstate |=  ts_stopped;
        NotifyEM(&falseSSEvent, hthd, 0, 0);
        return;
    }

    if (rs->fIsCall) {
        LPSTR p;
        //
        // we just completed a call instruction
        // the pc is the first instruction of a new function
        //
        wt = MHAlloc( sizeof(WTNODE) );
        ZeroMemory( wt, sizeof(WTNODE) );

        hthd->wtcurr->callee = wt;
        wt->caller = hthd->wtcurr;
        wt->lex = hthd->wtcurr->lex + 1;
        wt->offset = currAddr;
        wt->sp = STACK_POINTER(hthd);

        WtGetSymbolName( hthd, &addr, &p, &symaddr, &ra );

        if (!p) {
            p = MHAlloc( 16 );
            sprintf( p, "0x%016I64x", currAddr );
        } else if (symaddr != currAddr) {
            DWORD l = _tcslen(p);
            p = MHRealloc(p, l + 12);
            sprintf(p + l, "+0x%I64x", currAddr - symaddr);
        }
        wt->fname = p;

        //
        // put new node at head of chain.
        //

        hthd->wtcurr = wt;
    }

    if (STACK_POINTER(hthd) > hthd->wtcurr->sp) {

        //
        // attempt to compensate for unwinds and longjumps.
        // also catches cases that miss the target address.
        //

        //
        // unwind the stack, print totals
        //

        wt = hthd->wtcurr;
        while (wt && STACK_POINTER(hthd) > wt->sp) {
            WtPrintCallNode( wt );
            if (wt1 = wt->caller ) {
                wt1->scnt += wt->icnt + wt->scnt;
                MHFree(wt);
            }
            wt = wt1;
        }
        if (wt) {
            hthd->wtcurr = wt;
        } else {
            hthd->wtcurr = &hthd->wthead;
            goto finished;
        }

    }

    rs->fIsCall = FALSE;

    rs->fIsRet = IsRet(hthd, &addr);

    if (!rs->fIsRet) {
        int CallFlag;
        IsCall( hthd, &addr, &CallFlag, FALSE );
        if (CallFlag == INSTR_IS_CALL) {
            //
            // we are about to trace a call instruction
            //
            rs->fIsCall = TRUE;
            WtPrintCallNode( hthd->wtcurr );
        }
    }

    SingleStep( hthd, rs->method, TRUE, FALSE );

    return;
}                               /* WtMethodRangeStep() */



void
WtRangeStep(
    HTHDX       hthd
    )

/*++

Routine Description:

    This function is used to implement the watch trace feature in the DM.  Range
    stepping is used to cause all instructions between a pair of addresses
    to be executed.

    The segment is implied to be the current segment.  This is validated
    in the EM.

    Range stepping is done by registering an expected debug event at the
    end of a step and seeing if the current program counter is still in
    the correct range.  If it is not then the range step is over, if it
    is then a new event is register and we loop.

Arguments:

    hthd      - Supplies the thread to be stepped.

Return Value:

    None.

--*/

{
    RANGESTEP   *rs;
    METHOD      *method;
    HPRCX       hprc = hthd->hprc;
    int         CallFlag  = 0;
    ADDR        addr;
    LPSTR       fname;
    UOFFSET     ra;
    UOFFSET     symaddr;
    UOFFSET     instrOff;


    if (hthd->wtmode != 0) {
        DMPrintShellMsg( "wt command already running for this thread\r\n" );
        return;
    }

    AddrInit( &addr, 
              0, 
              PcSegOfHthdx(hthd), 
              PC(hthd),
              hthd->fAddrIsFlat, 
              hthd->fAddrOff32, 
              FALSE, 
              hthd->fAddrIsReal 
              );
    WtGetSymbolName( hthd, &addr, &fname, &symaddr, &ra );


    //
    //  Create and fill a range step structure
    //
    rs = (RANGESTEP*) MHAlloc(sizeof(RANGESTEP));
    ZeroMemory( rs, sizeof(RANGESTEP) );

    //
    //  Create a notification method for this range step
    //
    method  = (METHOD*) MHAlloc(sizeof(METHOD));
    method->notifyFunction  = (ACVECTOR)WtMethodRangeStep;
    method->lparam          = (UINT_PTR)rs;

    rs->hthd             = hthd;
    rs->segCur           = PcSegOfHthdx(hthd);
    rs->method           = method;
    rs->safetyBP         = NULL;
    rs->stepFunction     = NULL;
    rs->addrStart        = PC(hthd);

    //
    // always tell the watch stepper that the first instruction
    // was a call.  that way, it makes a frame for the place that
    // we are returning to.
    //
    rs->fIsCall          = TRUE;

    hthd->wtcurr         = &hthd->wthead;
    ZeroMemory( hthd->wtcurr, sizeof(WTNODE) );
    hthd->wtcurr->offset = PC(hthd);
    hthd->wtcurr->sp     = STACK_POINTER(hthd);
    hthd->wtcurr->fname  = fname;
    hthd->wtmode         = 1;


    IsCall( hthd, &addr, &CallFlag, FALSE);
    if (CallFlag == INSTR_IS_CALL) {
        ra = GetAddrOff(addr);
    }

    rs->addrEnd = ra;
    DMPrintShellMsg( "Tracing %s to return address %08x\r\n", fname, ra );

    if (CallFlag == INSTR_IS_CALL) {

        //
        // This is a call instruction.  Assume that we
        // want to trace the function that is about to
        // be called.  The call instruction will be the
        // only instruction counted in the current frame.
        //

        //
        //  Call the step over function to send notifications
        //  to the RangeStepper (NOT THE EM!)
        //

        SingleStep(hthd, method, TRUE, FALSE);

    } else {

        //
        // tracing to return address.
        //
        // tell it that we just did a call so that a new
        // frame will be pushed, leaving the current frame
        // to contain this functions caller.
        //

        hthd->wtcurr->icnt = -1;
        WtMethodRangeStep(&falseSSEvent, hthd, 0, (UINT_PTR)rs);
    }

    return;
}                               /* WtRangeStep() */


#ifndef KERNEL
BOOL
SetupNLG(
    HTHDX hthd,
    LPADDR lpaddr
    )
{
    HNLG hnlg = hnlgNull;
    BOOL fRetVal = FALSE;

    hthd->fStopOnNLG = TRUE;
    while (NULL != (hnlg = LLNext ( hthd->hprc->llnlg, hnlg ))) {
        LPNLG lpnlg = LLLock ( hnlg );

        PBREAKPOINT bp = SetBP( hthd->hprc,
                                hthd,
                                bptpExec,
                                bpnsStop,
                                &lpnlg->addrNLGDispatch,
                                (HPID)INVALID);

        RegisterExpectedEvent( hthd->hprc,
                               hthd,
                               BREAKPOINT_DEBUG_EVENT,
                               (DWORD_PTR)bp,
                               DONT_NOTIFY,
                               ActionNLGDispatch,
                               FALSE,
                               (UINT_PTR)bp);
        fRetVal = TRUE;
        LLUnlock ( hnlg );
    }
    if (lpaddr == NULL) {
        SetAddrOff ( &hthd->addrStack, STACK_POINTER(hthd));
    } else {
        SetAddrOff ( &hthd->addrStack, GetAddrOff(*lpaddr) );
    }
    return( fRetVal );
}
#endif

void
ActionExceptionDuringStep(
    DEBUG_EVENT64* de,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    )
/*++

Routine Description:

    If an exception is hit while stepping then ask the EM for the addresses
    of all possible catches and set SS breakpoints thereat.

    Dolphin V3 may go to a new method that does not involve disassembly EH
    registration nodes. See Dolphin 9036 and 8510 for details.

Arguments:

    de       - Current debug event

    hthd     - Thread where debug event occurred

Return Value:

    none

--*/
{
    DWORD subclass = de->u.Exception.ExceptionRecord.ExceptionCode;
    DWORD firstChance = de->u.Exception.dwFirstChance;
    XOSD xosd;
#if !defined(TARGET_i386)
    DWORD size;
    EXHDLR ExHdlr;
#endif
    EXHDLR *pExHdlr = NULL;

    if ((subclass == STATUS_SINGLE_STEP) || !firstChance) {
        ProcessExceptionEvent(de, hthd);
        return;
    }
    switch (ExceptionAction(hthd->hprc, subclass)) {
        case efdNotify:
            NotifyEM(de, hthd, 0, efdNotify);
            break;
        // Dont bother asking for handler addresses
        case efdStop:
        case efdCommand:
            ProcessExceptionEvent(de, hthd);
            return;
    }


#if defined(TARGET_i386)
    assert(lparam != 0);

    //
    // Walk the exception registration stack and get the catch locations
    //

    if ((pExHdlr = GetExceptionCatchLocations(hthd, (LPVOID)lparam)) != NULL) {
        xosd = xosdNone;
    } else {
        xosd = xosdGeneral;  /* so we dont instantiate bps below*/
    }

#else   // TARGET_i386
    /* Ask the EM for catch locations */
    xosd = DMSendRequestReply(dbceExceptionDuringStep,
                              hthd->hprc->hpid,
                              hthd->htid,
                              0,
                              NULL,
                              sizeof(EXHDLR),
                              &ExHdlr
                              );

    size = sizeof(EXHDLR) + ExHdlr.count * sizeof(ADDR);
    pExHdlr = (EXHDLR *)DMCopyLargeReply(size);
#endif  // TARGET_i386


    if (xosd == xosdNone) {
        DWORD i;
        for (i=0; i<pExHdlr->count; i++) {
            PBREAKPOINT bp = SetBP(hthd->hprc,
                                   hthd,
                                   bptpExec,
                                   bpnsStop,
                                   pExHdlr->addr+i,
                                   0);

            if (bp != NULL) {
                METHOD * method = (METHOD *)MHAlloc(sizeof(METHOD));
                method->lparam2 = (LPVOID) bp;
                method->notifyFunction = ConsumeThreadEventsAndNotifyEM;

                RegisterExpectedEvent(hthd->hprc,
                                      hthd,
                                      BREAKPOINT_DEBUG_EVENT,
                                      (DWORD_PTR)bp,
                                      DONT_NOTIFY,
                                      (ACVECTOR) SSActionRemoveBP,
                                      FALSE,
                                      (UINT_PTR)method);
            }
        }
    }

#if defined(TARGET_i386)
    if (pExHdlr) {
        MHFree(pExHdlr);
    }
#endif  // TARGET_i386

    /* Re-enable myself */
    RegisterExpectedEvent(hthd->hprc,
                          (HTHDX)hthd,
                          EXCEPTION_DEBUG_EVENT,
                          NO_SUBCLASS,
                          DONT_NOTIFY,
                          ActionExceptionDuringStep,
                          FALSE,
                          lparam);

    ContinueThreadEx(hthd,
                     (ULONG)DBG_EXCEPTION_NOT_HANDLED,
                     QT_CONTINUE_DEBUG_EVENT,
                     ts_running);
}

/*** ISINSYSTEMDLL
 *
 * PURPOSE:
 *      Determine if the given uoffDest is in a system DLL
 * INPUT:
 *
 * OUTPUT:
 *     Returns TRUE if uoffDest is in a system dll otherwise FALSE
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *      This function takes a uoffset and determines if it is in the range of
 *      one of the system DLLs by examining the LPIAL list ( Pointer to
 *      invalid address List) that was built up in LoadDll.
 *
 ****************************************************************************/

extern SYSTEM_INFO SystemInfo;

BOOL
IsInSystemDll (
    UOFFSET uoffDest
    )
{
#ifdef KERNEL
    return FALSE;
#else   // KERNEL
    if (IsChicago()) {

        //
        // On Chicago, any address above the maximum application address
        // is part of a system DLL.
        //

        return (uoffDest > (UOFFSET)SystemInfo.lpMaximumApplicationAddress);
    } else {

        //
        // On NT, we return FALSE for *any* address, because system
        // DLLs don't need special treatment.
        //

        return FALSE;
    }
#endif  // KERNEL
} /* ISINSYSTEMDLL */

