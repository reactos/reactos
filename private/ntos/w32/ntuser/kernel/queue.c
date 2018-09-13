/****************************** Module Header ******************************\
* Module Name: queue.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains the low-level code for working with the Q structure.
*
* History:
* 12-02-90 DavidPe      Created.
* 02-06-91 IanJa        HWND revalidation added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

VOID DestroyProcessesObjects(PPROCESSINFO ppi);
VOID DestroyThreadsMessages(PQ pq, PTHREADINFO pti);
void CheckProcessForeground(PTHREADINFO pti);
void ScreenSaverCheck(PTHREADINFO pti);
DWORD xxxPollAndWaitForSingleObject(PKEVENT pEvent, PVOID pExecObject,
        DWORD dwMilliseconds);

NTSTATUS InitiateShutdown(PETHREAD Thread, PULONG lpdwFlags);
NTSTATUS EndShutdown(PETHREAD Thread, NTSTATUS StatusShutdown);
void SetVDMCursorBounds(LPRECT lprc);
NTSTATUS InitQEntryLookaside(VOID);
void SetAppStarting (PPROCESSINFO ppi);

#pragma alloc_text(INIT, InitQEntryLookaside)

PW32PROCESS gpwpCalcFirst;

#if DBG
#define MSG_SENT    0
#define MSG_POST    1
#define MSG_RECV    2
#define MSG_PEEK    3
VOID TraceDdeMsg(UINT msg, HWND hwndFrom, HWND hwndTo, UINT code);
#else
#define TraceDdeMsg(m, h1, h2, c)
#endif // DBG

PPAGED_LOOKASIDE_LIST QLookaside;
PPAGED_LOOKASIDE_LIST QEntryLookaside;

#if DBG
void DebugValidateMLIST(PMLIST pml)
{
    int     c;
    PQMSG   pqmsg;

    /*
     * Check that the message list is properly terminated.
     */
    UserAssert(!pml->pqmsgRead || !pml->pqmsgRead->pqmsgPrev);
    UserAssert(!pml->pqmsgWriteLast || !pml->pqmsgWriteLast->pqmsgNext);

    /*
     * Check that there aren't loops in the Next list.
     */
    c = pml->cMsgs;
    UserAssert(c >= 0);
    pqmsg = pml->pqmsgRead;
    while (--c >= 0) {
        UserAssert(pqmsg);
        if (c == 0) {
            UserAssert(pqmsg == pml->pqmsgWriteLast);
        }

        pqmsg = pqmsg->pqmsgNext;
    }

    UserAssert(!pqmsg);

    /*
     * Check that there aren't loops in the Prev list.
     */
    c = pml->cMsgs;
    pqmsg = pml->pqmsgWriteLast;
    while (--c >= 0) {
        UserAssert(pqmsg);
        if (c == 0) {
            UserAssert(pqmsg == pml->pqmsgRead);
        }

        pqmsg = pqmsg->pqmsgPrev;
    }

    UserAssert(!pqmsg);
}

void DebugValidateMLISTandQMSG(PMLIST pml, PQMSG pqmsg)
{
    PQMSG pqmsgT;

    DebugValidateMLIST(pml);
    for (pqmsgT = pml->pqmsgRead; pqmsgT; pqmsgT = pqmsgT->pqmsgNext) {
        if (pqmsgT == pqmsg) {
            return;
        }
    }

    UserAssert(pqmsgT == pqmsg);
}

#else
#define DebugValidateMLIST(pml)
#define DebugValidateMLISTandQMSG(pml, pqmsg)
#endif

/***************************************************************************\
* xxxSetProcessInitState
*
* Set process initialization state.  What state is set depends
* on whether another process is waiting on this process.
*
* 04-02-95 JimA         Created.
\***************************************************************************/

BOOL xxxSetProcessInitState(
    PEPROCESS Process,
    DWORD dwFlags)
{
    PW32PROCESS W32Process;
    NTSTATUS Status;

    CheckCritIn();
    UserAssert(IsWinEventNotifyDeferredOK());

    /*
     * If the W32Process structure has not been allocated, do it now.
     */
    W32Process = (PW32PROCESS)Process->Win32Process;
    if (W32Process == NULL) {
        Status = AllocateW32Process(Process);
        if (!NT_SUCCESS(Status)) {
            return FALSE;
        }
        W32Process = (PW32PROCESS)Process->Win32Process;
#if DBG
        /*
         * The above AllocateW32Process(Process, FALSE) won't set the
         * W32PF_PROCESSCONNECTED flag (and if it wasn't previously set),
         * make sure we're not on the gppiStarting list, because if we are,
         * we will not be removed without the W32PF_PROCESSCONNECTED bit.
         */
        if ((W32Process->W32PF_Flags & W32PF_PROCESSCONNECTED) == 0) {
            UserAssert((W32Process->W32PF_Flags & W32PF_APPSTARTING) == 0);
        }
#endif
    }

    /*
     * Defer WinEvent notifications, because the thread isn't initialized yet.
     */
    DeferWinEventNotify();
    if (dwFlags == 0) {
        if (!(W32Process->W32PF_Flags & W32PF_WOW)) {

            /*
             * Check to see if the startglass is on, and if so turn it off and update.
             */
            if (W32Process->W32PF_Flags & W32PF_STARTGLASS) {
                W32Process->W32PF_Flags &= ~W32PF_STARTGLASS;
                zzzCalcStartCursorHide(NULL, 0);
            }

            /*
             * Found it.  Set the console bit and reset the wait event so any sleepers
             * wake up.
             */
            W32Process->W32PF_Flags |= W32PF_CONSOLEAPPLICATION;
            SET_PSEUDO_EVENT(&W32Process->InputIdleEvent);
        }
    } else if (!(W32Process->W32PF_Flags & W32PF_INITIALIZED)) {
        W32Process->W32PF_Flags |= W32PF_INITIALIZED;

        /*
         * Set global state to allow the new process to become
         * foreground.  xxxInitProcessInfo() will set
         * W32PF_ALLOWFOREGROUNDACTIVATE when the process initializes.
         */
        SET_PUDF(PUDF_ALLOWFOREGROUNDACTIVATE);
        TAGMSG1(DBGTAG_FOREGROUND, "xxxSetProcessInitState set PUDF. %#p", W32Process);


        /*
         * If this is the win32 server process, force off start glass feedback
         */
        if (Process == gpepCSRSS) {
            dwFlags |= STARTF_FORCEOFFFEEDBACK;
        }

        /*
         * Show the app start cursor for 2 seconds if it was requested from
         * the application.
         */
        if (dwFlags & STARTF_FORCEOFFFEEDBACK) {
            W32Process->W32PF_Flags |= W32PF_FORCEOFFFEEDBACK;
            zzzCalcStartCursorHide(NULL, 0);
        } else if (dwFlags & STARTF_FORCEONFEEDBACK) {
            zzzCalcStartCursorHide(W32Process, 2000);
        }
    }
    /*
     * Have to defer without processing, because we don't have a ptiCurrent yet
     */
    EndDeferWinEventNotifyWithoutProcessing();
    return TRUE;
}

/***************************************************************************\
* CheckAllowForeground
*
* Bug 273518 - joejo
*
* Removed this loop from xxxInitProcessInfo to allow code shareing between
* that function and xxxUserNotifyConsoleApplication. This will allow console
* windows to set foreground correctly on new process' it launches, as opposed
* it just forcing foreground.
\***************************************************************************/
BOOL CheckAllowForeground(
    PEPROCESS pep)
{
    BOOL fCreator = TRUE;
    HANDLE hpid = (HANDLE)pep->InheritedFromUniqueProcessId;
    LUID luid;
    PACCESS_TOKEN pat;
    PEPROCESS pepParent;
    PPROCESSINFO ppiParent;
    UINT uAncestors = 0;
    BOOL fAllowForeground = FALSE;
    NTSTATUS Status;

    do {
        /*
         * Get the ppi for the parent process.
         */
        LeaveCrit();
        Status = LockProcessByClientId(hpid, &pepParent);
        EnterCrit();
        if (!NT_SUCCESS(Status)) {
            /*
             * Bug 294193 - joejo
             *
             * If this is a process that was created after it'a creator was
             * destroyed, then lets attempt to give it foreground. This is a
             * typical scenario when a stub exe trys to create another process
             * in it's place.
             */
            if (HasForegroundActivateRight(pep->InheritedFromUniqueProcessId)) {
                fAllowForeground = TRUE;
            }
            break;
        }

        ppiParent = PpiFromProcess(pepParent);
        if (ppiParent == NULL) {
            UnlockProcess(pepParent);
            break;
        }
        /*
         * If we're walking the parent chain,
         * stop when we get to the shell or to a process that
         * is not running on the IO winsta
         */
        if (!fCreator
                && (IsShellProcess(ppiParent)
                    || ((ppiParent->rpwinsta != NULL)
                        && (ppiParent->rpwinsta->dwWSF_Flags & WSF_NOIO)))) {

            UnlockProcess(pepParent);
            break;
        }
        fAllowForeground = CanForceForeground(ppiParent);
        if (!fAllowForeground) {
            /*
             * Bug 285639 - joejo
             *
             * If the first thread of the parent process has allow set foreground
             * than we allow the setting of the foreground.
             */
            if (ppiParent->ptiList != NULL
                && (ppiParent->ptiList->TIF_flags & TIF_ALLOWFOREGROUNDACTIVATE)) {
                    fAllowForeground = TRUE;
            }

            if (!fAllowForeground){
                /*
                 * Let's try an ancestor (this might be a worker process).
                 */
                hpid = (HANDLE)pepParent->InheritedFromUniqueProcessId;
                /*
                 * If this is launched by a system process, let it come to
                 *  the foreground (i.e. CSRSS launching an OLE server).
                 */
                if (fCreator) {
                    fCreator = FALSE;
                    pat = PsReferencePrimaryToken(pepParent);
                    if (pat != NULL) {
                        Status = SeQueryAuthenticationIdToken(pat, &luid);
                        if (NT_SUCCESS(Status)) {
                            fAllowForeground = RtlEqualLuid(&luid, &luidSystem);
                            /*
                             * If it is a system process, give it the
                             *  permanent right so we won't have to check
                             *  its luid again
                             */
                             if (fAllowForeground) {
                                 ppiParent->W32PF_Flags |= W32PF_ALLOWSETFOREGROUND;
                             }
                        }
                        ObDereferenceObject(pat);
                    }
                }
            }
        }
        UnlockProcess(pepParent);
      /*
       * InheritedFromUniqueProcessId cannot be quite trusted because
       *  process ids get reused very often. So we just check few levels up
       */
    } while (!fAllowForeground && (uAncestors++ < 5));

    return  fAllowForeground || GiveUpForeground();
}

/***************************************************************************\
* xxxUserNotifyConsoleApplication
*
* This is called by the console init code - it tells us that the starting
* application is a console application. We want to know this for various
* reasons, one being that WinExec() doesn't wait on a starting console
* application.
*
* 09-18-91 ScottLu      Created.
* 01-12-99  JoeJo       Bug 273518
*           Adding bug fix and optimization for bug fix
\***************************************************************************/

void xxxUserNotifyConsoleApplication(
    PCONSOLE_PROCESS_INFO pcpi)
{
    NTSTATUS  Status;
    PEPROCESS Process;
    BOOL retval;

    /*
     * First search for this process in our process information list.
     */
    LeaveCrit();
    Status = LockProcessByClientId((HANDLE)LongToHandle( pcpi->dwProcessID ), &Process);
    EnterCrit();

    if (!NT_SUCCESS(Status)) {
        RIPMSG2(RIP_WARNING, "xxxUserNotifyConsoleApplication: Failed with Process ID == %X, Status = %x\n",
                pcpi->dwProcessID, Status);
        return;
    }

    retval = xxxSetProcessInitState(Process, 0);
    /*
     * Bug 273518 - joejo
     *
     * This will allow console windows to set foreground correctly on new
     * process' it launches, as opposed it just forcing foreground.
     */
    if (retval) {
        if (pcpi->dwFlags & CPI_NEWPROCESSWINDOW) {
            PPROCESSINFO ppiCurrent = PpiCurrent();
            if (CheckAllowForeground(Process)) {
                if (!(ppiCurrent->W32PF_Flags & W32PF_APPSTARTING)) {
                    SetAppStarting(ppiCurrent);
                }
                SET_PUDF(PUDF_ALLOWFOREGROUNDACTIVATE);
                TAGMSG0(DBGTAG_FOREGROUND, "xxxUserNotifyConsoleApplication set PUDF");
                ppiCurrent->W32PF_Flags |= W32PF_ALLOWFOREGROUNDACTIVATE;
            }

            TAGMSG3(DBGTAG_FOREGROUND, "xxxUserNotifyConsoleApplication %s W32PF %#p-%#p",
                    ((ppiCurrent->W32PF_Flags & W32PF_ALLOWFOREGROUNDACTIVATE) ? "set" : "NOT"),
                    ppiCurrent, PpiFromProcess(Process));
        }
    } else {
        RIPMSG1(RIP_WARNING, "xxxUserNotifyConsoleApplication - SetProcessInitState failed on %#p", Process);
    }



    UnlockProcess(Process);
}


/***************************************************************************\
* UserSetConsoleProcessWindowStation
*
* This is called by the console init code - it tells us that the starting
* application is a console application and which window station they are associated
* with.  The window station pointer is stored in the EPROCESS for the Global atom
* calls to find the correct global atom table when called from a console application
*
\***************************************************************************/

void UserSetConsoleProcessWindowStation(
    DWORD idProcess,
    HWINSTA hwinsta
    )
{
    NTSTATUS  Status;
    PEPROCESS Process;

    /*
     * First search for this process in our process information list.
     */
    LeaveCrit();
    Status = LockProcessByClientId((HANDLE)LongToHandle( idProcess ), &Process);
    EnterCrit();

    if (!NT_SUCCESS(Status)) {
        RIPMSG2(RIP_WARNING, "UserSetConsoleProcessWindowStation: Failed with Process ID == %X, Status = %x\n",
                idProcess, Status);
        return;
    }

    Process->Win32WindowStation = hwinsta;

    UnlockProcess(Process);
}


/***************************************************************************\
* xxxUserNotifyProcessCreate
*
* This is a special notification that we get from the base while process data
* structures are being created, but before the process has started. We use
* this notification for startup synchronization matters (winexec, startup
* activation, type ahead, etc).
*
* This notification is called on the server thread for the client thread
* starting the process.
*
* 09-09-91 ScottLu      Created.
\***************************************************************************/

BOOL xxxUserNotifyProcessCreate(
    DWORD idProcess,
    DWORD idParentThread,
    ULONG_PTR dwData,
    DWORD dwFlags)
{
    PEPROCESS Process;
    PETHREAD Thread;
    PTHREADINFO pti;
    NTSTATUS Status;
    BOOL retval;

    CheckCritIn();


    GiveForegroundActivateRight((HANDLE)idProcess);

    /*
     * 0x1 bit means give feedback (app start cursor).
     * 0x2 bit means this is a gui app (meaning, call CreateProcessInfo()
     *     so we get app start synchronization (WaitForInputIdle()).
     * 0x8 bit means this process is a WOW process, set W32PF_WOW.  0x1
     *     and 0x2 bits will also be set.
     * 0x4 value means this is really a shared WOW task starting
     */

    /*
     * If we want feedback, we need to create a process info structure,
     * so do it: it will be properly cleaned up.
     */
    if ((dwFlags & 0xb) != 0) {
        LeaveCrit();
        Status = LockProcessByClientId((HANDLE)LongToHandle( idProcess ), &Process);
        EnterCrit();

        if (!NT_SUCCESS(Status)) {
            RIPMSG2(RIP_WARNING, "xxxUserNotifyProcessCreate: Failed with Process ID == %X, Status = %x\n",
                    idProcess, Status);
            return FALSE;
        }

        retval = xxxSetProcessInitState(Process, ((dwFlags & 1) ? STARTF_FORCEONFEEDBACK : STARTF_FORCEOFFFEEDBACK));
        if (!retval) {
            RIPMSG1(RIP_WARNING, "xxxUserNotifyProcessCreate - SetProcessInitState failed on %#p", Process);
        }
        if (dwFlags & 0x8) {
            if (Process->Win32Process)
                ((PW32PROCESS)Process->Win32Process)->W32PF_Flags |= W32PF_WOW;
        }

        UnlockProcess(Process);

        /*
         * Find out who is starting this app. If it is a 16 bit app, allow
         * it to bring itself back to the foreground if it calls
         * SetActiveWindow() or SetFocus(). This is because this could be
         * related to OLE to DDE activation. Notes has a case where after it
         * lauches pbrush to edit an embedded bitmap, it brings up a message
         * box on top if the bitmap is read only. This message box won't appear
         * foreground unless we allow it to. This usually isn't a problem
         * because most apps don't bring up windows on top of editors
         * like this. 32 bit apps will call SetForegroundWindow().
         */

        LeaveCrit();
        Status = LockThreadByClientId((HANDLE)LongToHandle( idParentThread ), &Thread);
        EnterCrit();

        if (!NT_SUCCESS(Status)) {
            RIPMSG2(RIP_WARNING, "xxxUserNotifyProcessCreate: Failed with Thread ID == %X, Status = %x\n",
                    idParentThread, Status);
            return FALSE;
        }

        pti = PtiFromThread(Thread);
        if (pti && (pti->TIF_flags & TIF_16BIT)) {
            pti->TIF_flags |= TIF_ALLOWFOREGROUNDACTIVATE;
            TAGMSG1(DBGTAG_FOREGROUND, "xxxUserNotifyProcessCreate set TIF %#p", pti);
        }

        UnlockThread(Thread);

    } else if (dwFlags == 4) {
        /*
         * A WOW task is starting up. Create the WOW per thread info
         * structure here in case someone calls WaitForInputIdle
         * before the thread is created.
         */
        PWOWTHREADINFO pwti;

        /*
         * Look for a matching thread in the WOW thread info list.
         */
        for (pwti = gpwtiFirst; pwti != NULL; pwti = pwti->pwtiNext) {
            if (pwti->idTask == idProcess) {
                break;
            }
        }

        /*
         * If we didn't find one, allocate a new one and add it to
         * the head of the list.
         */
        if (pwti == NULL) {
            pwti = (PWOWTHREADINFO)UserAllocPoolWithQuota(
                    sizeof(WOWTHREADINFO), TAG_WOWTHREADINFO);
            if (pwti == NULL) {
                return FALSE;
            }
            INIT_PSEUDO_EVENT(&pwti->pIdleEvent);
            pwti->idTask = idProcess;
            pwti->pwtiNext = gpwtiFirst;
            gpwtiFirst = pwti;
        } else {
            RESET_PSEUDO_EVENT(&pwti->pIdleEvent);
        }

        pwti->idWaitObject = dwData;
        LeaveCrit();
        Status = LockThreadByClientId((HANDLE)LongToHandle( idParentThread ), &Thread);
        EnterCrit();
        if (!NT_SUCCESS(Status))
            return FALSE;

        if (!NT_SUCCESS(Status)) {
            RIPMSG2(RIP_WARNING, "xxxUserNotifyProcessCreate: Failed with Thread ID == %X, Status = %x\n",
                    idParentThread, Status);
            return FALSE;
        }

        pwti->idParentProcess = HandleToUlong(Thread->Cid.UniqueProcess);
        UnlockThread(Thread);
    }

    return TRUE;
}


/***************************************************************************\
* zzzCalcStartCursorHide
*
* Calculates when to hide the startup cursor.
*
* 05-14-92 ScottLu      Created.
\***************************************************************************/

void zzzCalcStartCursorHide(
    PW32PROCESS pwp,
    DWORD timeAdd)
{
    DWORD timeNow;
    PW32PROCESS pwpT;
    PW32PROCESS *ppwpT;


    timeNow = NtGetTickCount();

    if (pwp != NULL) {

        /*
         * We were passed in a timeout. Recalculate when we timeout
         * and add the pwp to the starting list.
         */
        if (!(pwp->W32PF_Flags & W32PF_STARTGLASS)) {

            /*
             * Add it to the list only if it is not already in the list
             */
            for (pwpT = gpwpCalcFirst; pwpT != NULL; pwpT = pwpT->NextStart) {
                if (pwpT == pwp)
                    break;
            }

            if (pwpT != pwp) {
                pwp->NextStart = gpwpCalcFirst;
                gpwpCalcFirst = pwp;
            }
        }
        pwp->StartCursorHideTime = timeAdd + timeNow;
        pwp->W32PF_Flags |= W32PF_STARTGLASS;
    }

    gtimeStartCursorHide = 0;
    for (ppwpT = &gpwpCalcFirst; (pwpT = *ppwpT) != NULL; ) {

        /*
         * If the app isn't starting or feedback is forced off, remove
         * it from the list so we don't look at it again.
         */
        if (!(pwpT->W32PF_Flags & W32PF_STARTGLASS) ||
                (pwpT->W32PF_Flags & W32PF_FORCEOFFFEEDBACK)) {
            *ppwpT = pwpT->NextStart;
            continue;
        }

        /*
         * Find the greatest hide cursor timeout value.
         */
        if (gtimeStartCursorHide < pwpT->StartCursorHideTime)
            gtimeStartCursorHide = pwpT->StartCursorHideTime;

        /*
         * If this app has timed out, it isn't starting anymore!
         * Remove it from the list.
         */
        if (ComputeTickDelta(timeNow, pwpT->StartCursorHideTime) > 0) {
            pwpT->W32PF_Flags &= ~W32PF_STARTGLASS;
            *ppwpT = pwpT->NextStart;
            continue;
        }

        /*
         * Step to the next pwp in the list.
         */
        ppwpT = &pwpT->NextStart;
    }

    /*
     * If the hide time is still less than the current time, then turn off
     * the app starting cursor.
     */
    if (gtimeStartCursorHide <= timeNow)
        gtimeStartCursorHide = 0;

    /*
     * Update the cursor image with the new info (doesn't do anything unless
     * the cursor is really changing).
     */
    zzzUpdateCursorImage();
}


#define QUERY_VALUE_BUFFER 80

/*
 * Install hack.
 *
 * We have a hack inherited from Chicago that allows the shell to
 * clean up registry information after a setup program runs.  A
 * setup program is defined as an app with one of a list of names. -- FritzS
 */

PUNICODE_STRING gpastrSetupExe;    // These are initialized in the routine
int giSetupExe;                    // CreateSetupNameArray in setup.c


/***************************************************************************\
* SetAppImeCompatFlags - NOTE pstrModName->Buffer must be zero terminated.
*
*
* History:
* 07-17-97 DaveHart Split from SetAppCompatFlags -- misleadingly it also
*                   returns a BOOL indicating whether the filename is
*                   recognized as a setup program.  Used by SetAppCompatFlags
*                   for 32-bit apps and zzzInitTask for 16-bit ones.
\***************************************************************************/

BOOL SetAppImeCompatFlags(
    PTHREADINFO pti,
    PUNICODE_STRING pstrModName,
    PUNICODE_STRING pstrBaseFileName)
{
    DWORD dwImeFlags = 0;
    WCHAR szHex[QUERY_VALUE_BUFFER];
    WORD wPrimaryLangID;
    LCID lcid;
    int iSetup;
    BOOL fSetup = FALSE;
    int iAppName;
    int cAppNames;
    PUNICODE_STRING rgpstrAppNames[2];
    UNICODE_STRING strHex;

    /*
     * Because can't access pClientInfo of another process
     */
    UserAssert(pti->ppi == PpiCurrent());

    /*
     * Because it is used as a zero-terminated profile key name.
     */
    UserAssert(0 == pstrModName->Buffer[ pstrModName->Length / sizeof(WCHAR) ]);

    if (FastGetProfileStringW(
                NULL,
                PMAP_IMECOMPAT,
                pstrModName->Buffer,
                NULL,
                szHex,
                sizeof(szHex)
                )) {

        /*
         * Found some flags.  Attempt to convert the hex string
         * into numeric value. Specify base 0, so
         * RtlUnicodeStringToInteger will handle the 0x format
         */
        RtlInitUnicodeString(&strHex, szHex);
        RtlUnicodeStringToInteger(&strHex, 0, (PULONG)&dwImeFlags);
    }

    /*
     * if current layout is not IME layout, Actually, we don't need to
     * get compatible flags for IME. But now, we don't have any scheme
     * to get this flags when the keyboard layout is switched. then
     * we get it here, even this flags are not nessesary for non-IME
     * keyboard layouts.
     */
    ZwQueryDefaultLocale(FALSE, &lcid);
    wPrimaryLangID = PRIMARYLANGID(lcid);

    if ((wPrimaryLangID == LANG_KOREAN || wPrimaryLangID == LANG_JAPANESE) &&
            (LOWORD(pti->dwExpWinVer) <= VER31)) {
        /*
         * IME compatibility flags are needed even it's a 32 bit app
         */
        pti->ppi->dwImeCompatFlags = dwImeFlags;
    } else {
        pti->ppi->dwImeCompatFlags = dwImeFlags & (IMECOMPAT_NOFINALIZECOMPSTR | IMECOMPAT_HYDRACLIENT);
        if (dwImeFlags & IMECOMPAT_NOFINALIZECOMPSTR) {
            RIPMSG1(RIP_WARNING, "IMECOMPAT_NOFINALIZECOMPSTR is set to ppi=0x%p", pti->ppi);
        }
        if (dwImeFlags & IMECOMPAT_HYDRACLIENT) {
            RIPMSG1(RIP_WARNING, "IMECOMPAT_HYDRACLIENT is set to ppi=0x%p", pti->ppi);
        }
    }


    if (gpastrSetupExe == NULL) {
        return fSetup;
    }

    rgpstrAppNames[0] = pstrModName;
    cAppNames = 1;
    if (pstrBaseFileName) {
        rgpstrAppNames[1] = pstrBaseFileName;
        cAppNames = 2;
    }

    for (iAppName = 0;
         iAppName < cAppNames && !fSetup;
         iAppName++) {

        iSetup = 0;
        while (iSetup < giSetupExe) {
            int i;
            if ((i = RtlCompareUnicodeString(rgpstrAppNames[iAppName], &(gpastrSetupExe[iSetup]), TRUE)) == 0) {
                fSetup = TRUE;
                break;
            }
            iSetup++;
        }
    }

    return fSetup;
}

/***************************************************************************\
* SetAppCompatFlags
*
*
* History:
* 03-23-92 JimA     Created.
* 07-17-97 FritzS   add return for fSetup -- returns TRUE if app is a setup app.
* 09-03-97 DaveHart Split out IME, WOW doesn't use this function anymore.
* 07-14-98 MCostea  Add Compatibility2 flags
* 01-21-99 MCostea  Add DesiredOSVersion
\***************************************************************************/

BOOL SetAppCompatFlags(
    PTHREADINFO pti)
{
    DWORD dwFlags = 0;
    DWORD dwFlags2 = 0;
    WCHAR szHex[QUERY_VALUE_BUFFER];
    WCHAR szKey[90];
    WCHAR *pchStart, *pchEnd;
    DWORD cb;
    PUNICODE_STRING pstrAppName;
    UNICODE_STRING strKey;
    UNICODE_STRING strImageName;
    ULONG cbSize;
    typedef struct tagCompat2Key {
        DWORD dwCompatFlags2;
        DWORD dwMajorVersion;
        DWORD dwMinorVersion;
        DWORD dwBuildNumber;
        DWORD dwPlatformId;
    } COMPAT2KEY, *PCOMPAT2KEY;
    COMPAT2KEY Compat2Key;

    /*
     * Because can't access pClientInfo of another process
     */
    UserAssert(pti->ppi == PpiCurrent());

    UserAssert(pti->ppi->ptiList);

    UserAssert(!(pti->TIF_flags & TIF_16BIT));

    /*
     * We assume here that pti was just inserted in at the head of ptiList
     */
    UserAssert(pti == pti->ppi->ptiList);

    if (pti->ptiSibling) {
        pti->pClientInfo->dwCompatFlags = pti->dwCompatFlags = pti->ptiSibling->dwCompatFlags;
        pti->pClientInfo->dwCompatFlags2 = pti->dwCompatFlags2 = pti->ptiSibling->dwCompatFlags2;
        return FALSE;
    }

    try {
        /*
         * PEB can be trashed from the client side, so we need to probe pointers in it
         * MCostea 317180
         */
        /*
         * Find end of app name
         */
        if (pti->pstrAppName != NULL)
            pstrAppName = pti->pstrAppName;
        else {
            struct _RTL_USER_PROCESS_PARAMETERS *ProcessParameters = pti->pEThread->ThreadsProcess->Peb->ProcessParameters;

            ProbeForRead(ProcessParameters, sizeof(*ProcessParameters), sizeof(BYTE));
            strImageName = ProbeAndReadUnicodeString(&ProcessParameters->ImagePathName);
            ProbeForReadUnicodeStringBuffer(strImageName);
            pstrAppName = &strImageName;
        }
        pchStart = pchEnd = pstrAppName->Buffer +
                (pstrAppName->Length / sizeof(WCHAR));

        /*
         * Locate start of extension
         */
        while (TRUE) {
            if (pchEnd == pstrAppName->Buffer) {
                pchEnd = pchStart;
                break;
            }

            if (*pchEnd == TEXT('.'))
                break;

            pchEnd--;
        }

        /*
         * Locate start of filename
         */
        pchStart = pchEnd;

        while (pchStart != pstrAppName->Buffer) {
            if (*pchStart == TEXT('\\') || *pchStart == TEXT(':')) {
                pchStart++;
                break;
            }

            pchStart--;
        }

    #define MODULESUFFIXSIZE    (8*sizeof(WCHAR))
    #define MAXMODULENAMELEN    (sizeof(szKey) - MODULESUFFIXSIZE)
        /*
         * Get a copy of the filename
         * Allow extra spaces for the 'ImageSubsystemMajorVersionMinorVersion'
         * i.e. 3.5 that will get appended at the end of the module name
         */
        cb = (DWORD)(pchEnd - pchStart) * sizeof(WCHAR);
        if (cb >= MAXMODULENAMELEN)
            cb = MAXMODULENAMELEN - sizeof(WCHAR);
        RtlCopyMemory(szKey, pchStart, cb);
    } except (W32ExceptionHandler(FALSE, RIP_ERROR)) {
        return FALSE;
    }

    szKey[(cb / sizeof(WCHAR))] = 0;
#undef MAXMODULENAMELEN

    if (FastGetProfileStringW(
                NULL,
                PMAP_COMPAT32,
                szKey,
                NULL,
                szHex,
                sizeof(szHex)
                )) {

        UNICODE_STRING strHex;

        /*
         * Found some flags.  Attempt to convert the hex string
         * into numeric value. Specify base 0, so
         * RtlUnicodeStringToInteger will handle the 0x format
         */
        RtlInitUnicodeString(&strHex, szHex);
        RtlUnicodeStringToInteger(&strHex, 0, (PULONG)&dwFlags);
    }

    pti->dwCompatFlags = dwFlags;
    pti->pClientInfo->dwCompatFlags = dwFlags;

    /*
     * Retrieve the image version
     */
    {
        PIMAGE_NT_HEADERS pnthdr;
        USHORT uMinorImage, uMajorImage;
        PWCHAR pWritePtr = szKey + cb/sizeof(WCHAR);

        try {
            pnthdr = RtlImageNtHeader(pti->pEThread->ThreadsProcess->SectionBaseAddress);
            if (pnthdr != NULL) {
                uMinorImage = pnthdr->OptionalHeader.MinorImageVersion & 0xFF;
                uMajorImage = pnthdr->OptionalHeader.MajorImageVersion & 0xFF;
            } else {
                uMinorImage = uMajorImage = 0;
            }
        } except (W32ExceptionHandler(FALSE, RIP_ERROR)) {
            goto Compat2Failed;
        }
        swprintf(pWritePtr, L"%u.%u", uMajorImage, uMinorImage);
    }
    cbSize = FastGetProfileValue(
            NULL,
            PMAP_COMPAT2,
            szKey,
            NULL,
            (LPBYTE)&Compat2Key,
            sizeof(Compat2Key));
    /*
     * The first DWORD in Compat2 is the CompatFlags.  There might be or not
     * version information but dwCompatFlags2 is always there if the key exist.
     * We will be able to include extra data in these keys by setting the
     * MajorVersion to zero and thus the version information will not be changed
     */
    if (cbSize >= sizeof(DWORD)) {
        pti->dwCompatFlags2 = pti->pClientInfo->dwCompatFlags2 = Compat2Key.dwCompatFlags2;

        if (cbSize >= sizeof(COMPAT2KEY) && Compat2Key.dwMajorVersion != 0) {
            PPEB Peb = pti->pEThread->ThreadsProcess->Peb;

            Peb->OSMajorVersion = Compat2Key.dwMajorVersion;
            Peb->OSMinorVersion = Compat2Key.dwMinorVersion;
            Peb->OSBuildNumber = (USHORT)(Compat2Key.dwBuildNumber);
            Peb->OSPlatformId = Compat2Key.dwPlatformId;
        }
    }

Compat2Failed:
    /*
     * Restore the string
     */
    szKey[(cb / sizeof(WCHAR))] = 0;
    RtlInitUnicodeString(&strKey, szKey);

    return SetAppImeCompatFlags(pti, &strKey, NULL);
}

/***************************************************************************\
* GetAppCompatFlags
*
* Compatibility flags for < Win 3.1 apps running on 3.1
*
* History:
* 04-??-92 ScottLu      Created.
* 05-04-92 DarrinM      Moved to USERRTL.DLL.
\***************************************************************************/

DWORD GetAppCompatFlags(
    PTHREADINFO pti)
{
    // From GRE with pti = NULL
    // We got to use PtiCurrentShared()
    if (pti == NULL)
        pti = PtiCurrentShared();

    return pti->dwCompatFlags;
}
/***************************************************************************\
* GetAppCompatFlags2
*
* Compatibility flags for < wVer apps
*
* History:
* 07-01-98 MCostea      Created.
\***************************************************************************/

DWORD GetAppCompatFlags2(
    WORD wVer)
{
    return GetAppCompatFlags2ForPti(PtiCurrentShared(), wVer);
}

DWORD GetAppImeCompatFlags(
    PTHREADINFO pti)
{
    if (pti == NULL) {
        pti = PtiCurrentShared();
    }

    UserAssert(pti->ppi);
    return pti->ppi->dwImeCompatFlags;
}

/***************************************************************************\
* CheckAppStarting
*
* This is a timer proc (see SetAppStarting) which removes ppi's from the
*  starting list once their initialization time has expired.
*
* History:
* 08/26/97 GerardoB     Created
\***************************************************************************/
VOID CheckAppStarting(PWND pwnd, UINT message, UINT_PTR nID, LPARAM lParam)
{
    LARGE_INTEGER liStartingTimeout;
    PPROCESSINFO *pppi = &gppiStarting;

    KeQuerySystemTime(&liStartingTimeout); /* 1 unit == 100ns */
    liStartingTimeout.QuadPart -= (LONGLONG)(CMSAPPSTARTINGTIMEOUT * 10000);
    while (*pppi != NULL) {
        if (liStartingTimeout.QuadPart  > (*pppi)->Process->CreateTime.QuadPart) {
            (*pppi)->W32PF_Flags &= ~(W32PF_APPSTARTING | W32PF_ALLOWFOREGROUNDACTIVATE);
            TAGMSG1(DBGTAG_FOREGROUND, "CheckAppStarting clear W32PF %#p", *pppi);
            *pppi = (*pppi)->ppiNext;
        } else {
            pppi = &(*pppi)->ppiNext;
        }
    }

    TAGMSG0(DBGTAG_FOREGROUND, "Removing all entries from ghCanActivateForegroundPIDs array");
    RtlZeroMemory(ghCanActivateForegroundPIDs, sizeof(ghCanActivateForegroundPIDs));
    return;
    UNREFERENCED_PARAMETER(pwnd);
    UNREFERENCED_PARAMETER(message);
    UNREFERENCED_PARAMETER(nID);
    UNREFERENCED_PARAMETER(lParam);
}
/***************************************************************************\
* SetAppStarting
*
* Add a process to the starting list and mark it as such. The process will
*  remain in the list until it activates a window, our timer goes off or the
*  process goes away, whichever happens first.
*
* History:
* 08/26/97 GerardoB     Created
\***************************************************************************/
void SetAppStarting (PPROCESSINFO ppi)
{
    static UINT_PTR guAppStartingId = 0;

    // This ppi had better not be in the list already, or we will be creating
    // a loop (as seen in stress)
    UserAssert((ppi->W32PF_Flags & W32PF_APPSTARTING) == 0);

    /*
     * if we add this to the gppiStartingList without this bit set, we will
     * skip removing it from the list in DestroyProcessInfo(), but continue
     * to free it in FreeW32Process called by W32pProcessCallout
     */
    UserAssert((ppi->W32PF_Flags & W32PF_PROCESSCONNECTED));

    ppi->W32PF_Flags |= W32PF_APPSTARTING;
    ppi->ppiNext = gppiStarting;
    gppiStarting = ppi;
    /*
     * Some system processes are initialized before the RIT has setup the master
     *  timer; so check for it
     */
    if (gptmrMaster != NULL) {
        guAppStartingId = InternalSetTimer(NULL, guAppStartingId,
                                           CMSAPPSTARTINGTIMEOUT + CMSHUNGAPPTIMEOUT,
                                           CheckAppStarting, TMRF_RIT | TMRF_ONESHOT);
    }
}
/***************************************************************************\
* ClearAppStarting
*
* Remove a process from the app starting list and clear the W32PF_APPSTARTING
* flag. No major action here, just a centralized place to take care of this.
*
* History:
* 08/26/97 GerardoB     Created
\***************************************************************************/
void ClearAppStarting (PPROCESSINFO ppi)
{
    REMOVE_FROM_LIST(PROCESSINFO, gppiStarting, ppi, ppiNext);
    ppi->W32PF_Flags &= ~W32PF_APPSTARTING;
}

/***************************************************************************\
* zzzInitTask -- called by WOW startup for each app
*
*
* History:
* 02-21-91 MikeHar  Created.
* 02-23-92 MattFe   Altered for WOW
* 09-03-97 DaveHart WOW supplies compat flags, we tell it about setup apps.
\***************************************************************************/

NTSTATUS zzzInitTask(
    UINT dwExpWinVer,
    DWORD dwAppCompatFlags,
    PUNICODE_STRING pstrModName,
    PUNICODE_STRING pstrBaseFileName,
    DWORD hTaskWow,
    DWORD dwHotkey,
    DWORD idTask,
    DWORD dwX,
    DWORD dwY,
    DWORD dwXSize,
    DWORD dwYSize)
{
    PTHREADINFO ptiCurrent;
    PTDB ptdb;
    PPROCESSINFO ppi;
    PWOWTHREADINFO pwti;

    ptiCurrent = PtiCurrent();
    ppi = ptiCurrent->ppi;

    /*
     * Set the real name of the module.  (Instead of 'NTVDM')
     * We've already probed pstrModName->Buffer for Length+sizeof(WCHAR) so
     * we can copy the UNICODE_NULL terminator as well.
     */
    if (ptiCurrent->pstrAppName != NULL)
        UserFreePool(ptiCurrent->pstrAppName);
    ptiCurrent->pstrAppName = UserAllocPoolWithQuota(sizeof(UNICODE_STRING) +
            pstrModName->Length + sizeof(WCHAR), TAG_TEXT);
    if (ptiCurrent->pstrAppName != NULL) {
        ptiCurrent->pstrAppName->Buffer = (PWCHAR)(ptiCurrent->pstrAppName + 1);
        try {
            RtlCopyMemory(ptiCurrent->pstrAppName->Buffer, pstrModName->Buffer,
                    pstrModName->Length);
            ptiCurrent->pstrAppName->Buffer[pstrModName->Length / sizeof(WCHAR)] = 0;
        } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
            UserFreePool(ptiCurrent->pstrAppName);
            ptiCurrent->pstrAppName = NULL;
            return STATUS_OBJECT_NAME_INVALID;
        }
        ptiCurrent->pstrAppName->MaximumLength = pstrModName->Length + sizeof(WCHAR);
        ptiCurrent->pstrAppName->Length = pstrModName->Length;
    } else
        return STATUS_OBJECT_NAME_INVALID;

    /*
     * An app is starting!
     */
    if (!(ppi->W32PF_Flags & W32PF_APPSTARTING)) {
        SetAppStarting(ppi);
    }

    /*
     * We never want to use the ShowWindow defaulting mechanism for WOW
     * apps.  If STARTF_USESHOWWINDOW was set in the client-side
     * STARTUPINFO structure, WOW has already picked it up and used
     * it for the first (command-line) app.
     */
    ppi->usi.dwFlags &= ~STARTF_USESHOWWINDOW;

    /*
     * If WOW passed us a hotkey for this app, save it for CreateWindow's use.
     */
    if (dwHotkey != 0) {
        ppi->dwHotkey = dwHotkey;
    }

    /*
     * If WOW passed us a non-default window position use it, otherwise clear it.
     */
    ppi->usi.cb = sizeof(ppi->usi);

    if (dwX == CW_USEDEFAULT || dwX == CW2_USEDEFAULT) {
        ppi->usi.dwFlags &= ~STARTF_USEPOSITION;
    } else {
        ppi->usi.dwFlags |= STARTF_USEPOSITION;
        ppi->usi.dwX = dwX;
        ppi->usi.dwY = dwY;
    }

    /*
     * If WOW passed us a non-default window size use it, otherwise clear it.
     */
    if (dwXSize == CW_USEDEFAULT || dwXSize == CW2_USEDEFAULT) {
        ppi->usi.dwFlags &= ~STARTF_USESIZE;
    } else {
        ppi->usi.dwFlags |= STARTF_USESIZE;
        ppi->usi.dwXSize = dwXSize;
        ppi->usi.dwYSize = dwYSize;
    }

    /*
     * Alloc and Link in new task into the task list
     */
    if ((ptdb = (PTDB)UserAllocPoolWithQuota(sizeof(TDB), TAG_WOWTDB)) == NULL)
        return STATUS_NO_MEMORY;
    ptiCurrent->ptdb = ptdb;

    /*
     * Set the flags to say this is a 16-bit thread - before attaching
     * queues!
     */
    ptiCurrent->TIF_flags |= TIF_16BIT | TIF_FIRSTIDLE;

    /*
     * If this task is running in the shared WOW VDM, we handle
     * WaitForInputIdle a little differently than separate WOW
     * VDMs.  This is because CreateProcess returns a real process
     * handle when you start a separate WOW VDM, so the "normal"
     * WaitForInputIdle works.  For the shared WOW VDM, CreateProcess
     * returns an event handle.
     */
    ptdb->pwti = NULL;
    if (idTask) {
        ptiCurrent->TIF_flags |= TIF_SHAREDWOW;

        /*
         * Look for a matching thread in the WOW thread info list.
         */
        if (idTask != (DWORD)-1) {
            for (pwti = gpwtiFirst; pwti != NULL; pwti = pwti->pwtiNext) {
                if (pwti->idTask == idTask) {
                    ptdb->pwti = pwti;
                    break;
                }
            }
#if DBG
            if (pwti == NULL) {
                RIPMSG0(RIP_WARNING, "InitTask couldn't find WOW struct\n");
            }
#endif
        }
    }
    ptiCurrent->pClientInfo->dwTIFlags |= ptiCurrent->TIF_flags;

    /*
     * We need this thread to share the queue of other win16 apps.
     * If we're journalling, all apps are sharing a queue, so we wouldn't
     * want to interrupt that - so only cause queue recalculation
     * if we aren't journalling.
     * ptdb may be freed by DestroyTask during a callback, so defer WinEvent
     * notifications until we don't need ptdb any more.
     */
    DeferWinEventNotify();
    if (!FJOURNALRECORD() && !FJOURNALPLAYBACK())
        zzzReattachThreads(FALSE);

    /*
     * Save away the 16 bit task handle: we use this later when calling
     * wow back to close a WOW task.
     */
    ptdb->hTaskWow = LOWORD(hTaskWow);

    /*
     * Setup the app start cursor for 5 second timeout.
     */
    zzzCalcStartCursorHide((PW32PROCESS)ppi, 5000);

    /*
     * HIWORD: != 0 if wants proportional font
     * LOWORD: Expected windows version (3.00 [300], 3.10 [30A], etc)
     */
    ptiCurrent->dwExpWinVer = dwExpWinVer;
    ptiCurrent->pClientInfo->dwExpWinVer = dwExpWinVer;

    /*
     * Mark this guy and add him to the global task list so he can run.
     */
#define NORMAL_PRIORITY_TASK 10

    /*
     * To be Compatible it super important that the new task run immediately
     * Set its priority accordingly.  No other task should ever be set to
     * CREATION priority
     */
    ptdb->nPriority = NORMAL_PRIORITY_TASK;
    ptdb->nEvents = 0;
    ptdb->pti = ptiCurrent;
    ptdb->ptdbNext = NULL;
    ptdb->TDB_Flags = 0;

    InsertTask(ppi, ptdb);
    zzzEndDeferWinEventNotify();

    ptiCurrent->dwCompatFlags = dwAppCompatFlags;
    ptiCurrent->pClientInfo->dwCompatFlags = dwAppCompatFlags;

    UserAssert(ptiCurrent->ppi->ptiList);
    /*
     * We haven't captured pstrBaseFileName's buffer, we
     * may fault touching it in SetAppImeCompatFlags.  If
     * so the IME flags have been set already and we
     * can safely assume it's not a setup app.
     */

    try {
        if (SetAppImeCompatFlags(ptiCurrent, ptiCurrent->pstrAppName,
                             pstrBaseFileName)) {
            /*
             * Flag task as a setup app.
             */
            ptdb->TDB_Flags = TDBF_SETUP;
            ppi->W32PF_Flags |= W32PF_SETUPAPP;
        }
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
    }

    /*
     * Force this new task to be the active task (WOW will ensure the
     * currently running task does a Yield which will put it into the
     * non preemptive scheduler.
     */
    ppi->pwpi->ptiScheduled = ptiCurrent;
    ppi->pwpi->CSLockCount = -1;

    EnterWowCritSect(ptiCurrent, ppi->pwpi);

    /*
     * ensure app gets focus
     */
    zzzShowStartGlass(10000);

    return STATUS_SUCCESS;
}

/***************************************************************************\
* zzzShowStartGlass
*
* This routine is called by WOW when first starting or when starting an
* additional WOW app.
*
* 12-07-92 ScottLu      Created.
\***************************************************************************/

void zzzShowStartGlass(
    DWORD dwTimeout)
{
    PPROCESSINFO ppi;

    /*
     * If this is the first call to zzzShowStartGlass(), then the
     * W32PF_ALLOWFOREGROUNDACTIVATE bit has already been set in the process
     * info - we don't want to set it again because it may have been
     * purposefully cleared when the user hit a key or mouse clicked.
     */
    ppi = PpiCurrent();
    if (ppi->W32PF_Flags & W32PF_SHOWSTARTGLASSCALLED) {
        /*
         * Allow this wow app to come to the foreground. This'll be cancelled
         * if the user mouse clicks or hits any keys.
         */
        SET_PUDF(PUDF_ALLOWFOREGROUNDACTIVATE);
        TAGMSG0(DBGTAG_FOREGROUND, "zzzShowStartGlass set PUDF");
        ppi->W32PF_Flags |= W32PF_ALLOWFOREGROUNDACTIVATE;
        TAGMSG1(DBGTAG_FOREGROUND, "zzzShowStartGlass set W32PF %#p", ppi);
}
    ppi->W32PF_Flags |= W32PF_SHOWSTARTGLASSCALLED;

    /*
     * Show the start glass cursor for this much longer.
     */
    zzzCalcStartCursorHide((PW32PROCESS)ppi, dwTimeout);
}
/***************************************************************************\
* GetJournallingQueue
*
* 03/21/97  GerardoB     Created
\***************************************************************************/
PQ GetJournallingQueue(PTHREADINFO pti)
{
    PHOOK phook;
    /*
     * fail if we cannot journal this thread
     */
    if ((pti->TIF_flags & TIF_DONTJOURNALATTACH)
            || (pti->rpdesk == NULL)) {

        return NULL;
    }
    /*
     * Get the journalling hook if any.
     */
    phook = PhkFirstGlobalValid(pti, WH_JOURNALPLAYBACK);
    if (phook == NULL) {
        phook = PhkFirstGlobalValid(pti, WH_JOURNALRECORD);
    }
    /*
     * Validate fsHooks bits.
     */
    UserAssert((phook == NULL)
                ^ IsHooked(pti, (WHF_FROM_WH(WH_JOURNALPLAYBACK) | WHF_FROM_WH(WH_JOURNALRECORD))));

    /*
     * return the queue if we found a journalling hook
     */
    return ((phook == NULL) ? NULL : GETPTI(phook)->pq);
}

/***************************************************************************\
* ClearQueueServerEvent
*
* This function should be called when a thread needs to wait for some kind of
*  input. This clears pEventQueueServer which means we won't return from the
*  wait until new input of the required type arrives. Setting the wake mask
*  controls what input will wake us up.  WOW apps skip this since their
*  scheduler controls when they wake up.
*
* History:
* 09/12/97 GerardoB     Created
\***************************************************************************/
void ClearQueueServerEvent (WORD wWakeMask)
{
    PTHREADINFO ptiCurrent = PtiCurrent();
    UserAssert(wWakeMask != 0);
    ptiCurrent->pcti->fsWakeMask = wWakeMask;
    KeClearEvent(ptiCurrent->pEventQueueServer);
}
/***************************************************************************\
* xxxCreateThreadInfo
*
* Allocate the main thread information structure
*
* History:
* 03-18-95 JimA         Created.
\***************************************************************************/

ULONG ParseReserved(
    WCHAR *pchReserved,
    WCHAR *pchFind)
{
    ULONG dw;
    WCHAR *pch, *pchT, ch;
    UNICODE_STRING uString;

    dw = 0;
    if (pchReserved != NULL && (pch = wcsstr(pchReserved, pchFind)) != NULL) {
        pch += wcslen(pchFind);

        pchT = pch;
        while (*pchT >= '0' && *pchT <= '9')
            pchT++;

        ch = *pchT;
        *pchT = 0;
        RtlInitUnicodeString(&uString, pch);
        *pchT = ch;

        RtlUnicodeStringToInteger(&uString, 0, &dw);
    }

    return dw;
}

NTSTATUS xxxCreateThreadInfo(
    PETHREAD pEThread,
    BOOL     IsSystemThread)
{
    DWORD                        dwTIFlags = 0;
    PPROCESSINFO                 ppi;
    PTHREADINFO                  ptiCurrent;
    PEPROCESS                    pEProcess = pEThread->ThreadsProcess;
    PUSERSTARTUPINFO             pusi;
    PRTL_USER_PROCESS_PARAMETERS ProcessParams;
    PDESKTOP                     pdesk = NULL;
    HDESK                        hdesk = NULL;
    HWINSTA                      hwinsta;
    PQ                           pq;
    NTSTATUS                     Status;
    BOOL                         fFirstThread;
    PTEB                         pteb = NtCurrentTeb();
    TL                           tlpdesk;

    CheckCritIn();
    UserAssert(IsWinEventNotifyDeferredOK());

    ValidateProcessSessionId(pEProcess);

    /*
     * If CleanupResources was called for the last GUI thread then
     * we should not allow any more GUI threads
     */
    if (gbCleanedUpResources) {
        RIPMSG0(RIP_ERROR, "No more GUI threads should be created");
        return STATUS_PROCESS_IS_TERMINATING;
    }

    /*
     * Increment the number of GUI threads in the session
     */
    gdwGuiThreads++;

    /*
     * Although all threads now have a ETHREAD structure, server-side
     * threads (RIT, Console, etc) don't have a client-server eventpair
     * handle.  We use this to distinguish the two cases.
     */

    if (IsSystemThread) {
        dwTIFlags = TIF_SYSTEMTHREAD | TIF_DONTATTACHQUEUE | TIF_DISABLEIME;
    }

    if (!(dwTIFlags & TIF_SYSTEMTHREAD) && pEProcess == gpepCSRSS) {
        dwTIFlags = TIF_CSRSSTHREAD | TIF_DONTATTACHQUEUE | TIF_DISABLEIME;
    }

    ProcessParams = (pEProcess->Peb ? pEProcess->Peb->ProcessParameters : NULL);

    /*
     * Locate the processinfo structure for the new thread.
     */
    ppi = PpiCurrent();

#if defined(_WIN64)
    /*
     * If the process is marked as an emulated 32bit app thus,
     * mark the thread as an emulated 32bit thread.
     * This is to be consistent with the way WOW16 marks threads.
     */
    if (ppi->W32PF_Flags & W32PF_WOW64) {
        dwTIFlags |= TIF_WOW64;
    }
#endif //defined(_WIN64)

    /*
     * For Winlogon, only the first thread can have IME processing.
     */
    if (gpidLogon == pEThread->Cid.UniqueProcess) {
        if (ppi->ptiList != NULL) {
            dwTIFlags |= TIF_DISABLEIME;
            RIPMSG1(RIP_VERBOSE, "WinLogon, second or other thread. pti=%x", pEThread->Tcb.Win32Thread);
        }
    }

    /*
     * Allocate the thread-info structure.  If it's a SYSTEMTHREAD, then
     * make sure we have enough space for the (pwinsta) pointer.  This
     * is referenced in (paint.c: DoPaint) to assure desktop/input can
     * have a winsta to view.
     */
    ptiCurrent = (PTHREADINFO)pEThread->Tcb.Win32Thread;

    ptiCurrent->TIF_flags = dwTIFlags;
    Lock(&ptiCurrent->spklActive, gspklBaseLayout);
    ptiCurrent->pcti      = &(ptiCurrent->cti);

    /*
     * Check if no IME processing for all threads
     * in the same process.
     */
    if (ppi->W32PF_Flags & W32PF_DISABLEIME)
        ptiCurrent->TIF_flags |= TIF_DISABLEIME;

    /*
     * Hook up this queue to this process info structure, increment
     * the count of threads using this process info structure. Set up
     * the ppi before calling SetForegroundPriority().
     */
    UserAssert(ppi != NULL);

    ptiCurrent->ppi        = ppi;
    ptiCurrent->ptiSibling = ppi->ptiList;
    ppi->ptiList    = ptiCurrent;
    ppi->cThreads++;


    if (pteb != NULL)
        pteb->Win32ThreadInfo = ptiCurrent;

    /*
     * Point to the client info.
     */
    if (dwTIFlags & TIF_SYSTEMTHREAD) {
        ptiCurrent->pClientInfo = UserAllocPoolWithQuota(sizeof(CLIENTINFO),
                                                  TAG_CLIENTTHREADINFO);
        if (ptiCurrent->pClientInfo == NULL) {
            Status = STATUS_NO_MEMORY;
            goto CreateThreadInfoFailed;
        }
    } else {
        /*
         * If this is not a system thread then grab the user mode client info
         * elsewhere we use the GetClientInfo macro which looks here
         */
        UserAssert(NtCurrentTeb() != NULL);
        ptiCurrent->pClientInfo = ((PCLIENTINFO)((NtCurrentTeb())->Win32ClientInfo));

        /*
         * set the SECURE flag in the thread flags if this is a secure process
         */
        if (((PW32PROCESS)ppi)->W32PF_Flags & W32PF_RESTRICTED) {
            ptiCurrent->TIF_flags |= TIF_RESTRICTED;
        }
    }


    /*
     * Create the input event.
     */
    Status = ZwCreateEvent(&ptiCurrent->hEventQueueClient,
                           EVENT_ALL_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE);

    if (NT_SUCCESS(Status)) {
        Status = ObReferenceObjectByHandle(ptiCurrent->hEventQueueClient,
                                           EVENT_ALL_ACCESS,
                                           *ExEventObjectType,
                                           UserMode,
                                           &ptiCurrent->pEventQueueServer,
                                           NULL);
        if (NT_SUCCESS(Status)) {
            Status = ProtectHandle(ptiCurrent->hEventQueueClient, TRUE);
        } else if (Status == STATUS_INVALID_HANDLE) {
            ptiCurrent->hEventQueueClient = NULL;
        }
    }
    if (!NT_SUCCESS(Status)) {
        goto CreateThreadInfoFailed;
    }

    /*
     * Mark the process as having threads that need cleanup.  See
     * DestroyProcessesObjects().
     */
    fFirstThread = !(ppi->W32PF_Flags & W32PF_THREADCONNECTED);
    ppi->W32PF_Flags |= W32PF_THREADCONNECTED;

    /*
     * If we haven't copied over our startup info yet, do it now.
     * Don't bother copying the info if we aren't going to use it.
     */
    if (ProcessParams) {

        pusi = &ppi->usi;

        if ((pusi->cb == 0) && (ProcessParams->WindowFlags != 0)) {
            pusi->cb          = sizeof(USERSTARTUPINFO);
            pusi->dwX         = ProcessParams->StartingX;
            pusi->dwY         = ProcessParams->StartingY;
            pusi->dwXSize     = ProcessParams->CountX;
            pusi->dwYSize     = ProcessParams->CountY;
            pusi->dwFlags     = ProcessParams->WindowFlags;
            pusi->wShowWindow = (WORD)ProcessParams->ShowWindowFlags;
        }

        if (fFirstThread) {

            /*
             * Set up the hot key, if there is one.
             *
             * If the STARTF_USEHOTKEY flag is given in the startup info, then
             * the hStdInput is the hotkey (new from Chicago).  Otherwise, parse
             * it out in string format from the lpReserved string.
             */
            if (ProcessParams->WindowFlags & STARTF_USEHOTKEY) {
                ppi->dwHotkey = HandleToUlong(ProcessParams->StandardInput);
            } else {
                ppi->dwHotkey = ParseReserved(ProcessParams->ShellInfo.Buffer,
                                              L"hotkey.");
            }

            /*
             * Copy the monitor handle, if there is one.
             */
            UserAssert(!ppi->hMonitor);
            if (ProcessParams->WindowFlags & STARTF_HASSHELLDATA) {
                HMONITOR    hMonitor;

                hMonitor = (HMONITOR)(ProcessParams->StandardOutput);
                if (ValidateHmonitor(hMonitor)) {
                    ppi->hMonitor = hMonitor;
                }
            }
        }
    }

    /*
     * Open the windowstation and desktop.  If this is a system
     * thread only use the desktop that might be stored in the teb.
     */
    UserAssert(ptiCurrent->rpdesk == NULL);
    if (!(ptiCurrent->TIF_flags & (TIF_SYSTEMTHREAD | TIF_CSRSSTHREAD)) &&
        grpWinStaList) {

        BOOL bShutDown = FALSE;

        hdesk = xxxResolveDesktop(
                NtCurrentProcess(),
                &ProcessParams->DesktopInfo,
                &hwinsta, (ProcessParams->WindowFlags & STARTF_DESKTOPINHERIT),
                &bShutDown);

        if (hdesk == NULL) {

            if (bShutDown) {
                /*
                 * Trying to create a new process during logoff
                 */
                ULONG_PTR adwParameters[5] = {0, 0, 0, 0, MB_DEFAULT_DESKTOP_ONLY};
                ULONG ErrorResponse;

                LeaveCrit();

                ExRaiseHardError((NTSTATUS)STATUS_DLL_INIT_FAILED_LOGOFF,
                                 ARRAY_SIZE(adwParameters),
                                 0,
                                 adwParameters,
                                 OptionOkNoWait,
                                 &ErrorResponse);

                ZwTerminateProcess(NtCurrentProcess(), STATUS_DLL_INIT_FAILED);

                EnterCrit();
            }

            Status = STATUS_DLL_INIT_FAILED;
            goto CreateThreadInfoFailed;

        } else {

            xxxSetProcessWindowStation(hwinsta, KernelMode);

            /*
             * Reference the desktop handle
             */
            Status = ObReferenceObjectByHandle(
                    hdesk,
                    0,
                    *ExDesktopObjectType,
                    KernelMode,
                    &pdesk,
                    NULL);

            if (!NT_SUCCESS(Status)) {
                UserAssert(pdesk == NULL);
                goto CreateThreadInfoFailed;
            }

            ThreadLockDesktop(ptiCurrent, pdesk, &tlpdesk, LDLT_FN_CREATETHREADINFO);

            ObDereferenceObject(pdesk);

            /*
             * The first desktop is the default for all succeeding threads.
             */
            if ((ppi->hdeskStartup == NULL) &&
                (pEProcess->UniqueProcessId != gpidLogon)) {

                LockDesktop(&ppi->rpdeskStartup, pdesk, LDL_PPI_DESKSTARTUP2, (ULONG_PTR)ppi);
                ppi->hdeskStartup = hdesk;
            }
        }
    }

    /*
     * Remember dwExpWinVer. This is used to return GetAppVer() (and
     * GetExpWinVer(NULL).
     */
    if (pEProcess->Peb != NULL)
        ptiCurrent->dwExpWinVer = RtlGetExpWinVer(pEProcess->SectionBaseAddress);
    else
        ptiCurrent->dwExpWinVer = VER40;

    ptiCurrent->pClientInfo->dwExpWinVer = ptiCurrent->dwExpWinVer;
    ptiCurrent->pClientInfo->dwTIFlags   = ptiCurrent->TIF_flags;

    if (ptiCurrent->spklActive) {
        ptiCurrent->pClientInfo->CodePage = ptiCurrent->spklActive->CodePage;
        ptiCurrent->pClientInfo->hKL = ptiCurrent->spklActive->hkl;
    } else {
        ptiCurrent->pClientInfo->CodePage = CP_ACP;
        ptiCurrent->pClientInfo->hKL = 0;
    }

    /*
     * Set the desktop even if it is NULL to ensure that ptiCurrent->pDeskInfo
     * is set.
     * NOTE: This adds the pti to the desktop's PtiList, but we don't yet have
     * a pti->pq. zzzRecalcThreadAttachment loops through this PtiList expects
     * a pq, so we must not leave the critsect until we have a queue.
     * zzzSetDesktop only zzz leaves the critsect if there is a pti->pq, so we
     * can BEGINATOMICCHECK to ensure this, and make sure we allocate the queue
     * before we leave the critical section.
     */
    BEGINATOMICCHECK();
    zzzSetDesktop(ptiCurrent, pdesk, hdesk);
    ENDATOMICCHECK();

    /*
     * If we have a desktop and are journalling on that desktop, use
     * the journal queue, otherwise create a new queue.
     */
    if (pdesk == grpdeskRitInput) {
        PQ pq;
        UserAssert((pdesk == NULL) || (ptiCurrent->pDeskInfo == pdesk->pDeskInfo));
        UserAssert(ptiCurrent->rpdesk == pdesk);
        pq = GetJournallingQueue(ptiCurrent);
        if (pq != NULL) {
            ptiCurrent->pq = pq;
            pq->cThreads++;
        }
    }

    /*
     * If not journalling, give this thread its own queue
     */
    if (ptiCurrent->pq == NULL) {
        if ((pq = AllocQueue(NULL, NULL)) == NULL) {
            Status = STATUS_NO_MEMORY;
            goto CreateThreadInfoFailed;
        }
        /*
         * Attach the Q to the THREADINFO.
         */
        ptiCurrent->pq      = pq;
        pq->ptiMouse = pq->ptiKeyboard = ptiCurrent;
        pq->cThreads++;
    }

    /*
     * Remember that this is a screen saver. That way we can set its
     * priority appropriately when it is idle or when it needs to go
     * away.  At first we set it to normal priority, then we set the
     * TIF_IDLESCREENSAVER bit so that when it activates it will get
     * lowered in priority.
     */
    if (ProcessParams && ProcessParams->WindowFlags & STARTF_SCREENSAVER) {

        if (fFirstThread) {
            UserAssert(gppiScreenSaver == NULL);

            /*
             * Make sure the parent's process is WinLogon, since only WinLogon is allowed to
             * use the STARTF_SCREENSAVER flag.
             */
            if (gpidLogon == 0 || pEProcess->InheritedFromUniqueProcessId != gpidLogon) {
                RIPMSG0(RIP_WARNING,"Only the Logon process can launch a screen saver.");
                ProcessParams->WindowFlags &= ~STARTF_SCREENSAVER;
                goto NotAScreenSaver;
            }

            gppiScreenSaver = ppi;
            gptSSCursor = gpsi->ptCursor;
            ppi->W32PF_Flags |= W32PF_SCREENSAVER;
        }
#if DBG
        else {
            UserAssert(ppi->W32PF_Flags & W32PF_SCREENSAVER);
        }
#endif

        SetForegroundPriority(ptiCurrent, TRUE);

        if (fFirstThread) {
            ppi->W32PF_Flags |= W32PF_IDLESCREENSAVER;
        }

        /*
         * Screen saver doesn't need any IME processing.
         */
        ptiCurrent->TIF_flags |= TIF_DISABLEIME;
    }

NotAScreenSaver:

    /*
     * Do special processing for the first thread of a process.
     */
    if (!(ptiCurrent->TIF_flags & (TIF_SYSTEMTHREAD | TIF_CSRSSTHREAD))) {

        /*
         * I changed the code a while ago to unregister classes when the last
         * GUI thread is destroyed.  Simply, there was too much stuff getting
         * unlocked and destroyed to guarantee that it would work on a non-GUI
         * thread.  So if a process destroys its last GUI thread and then makes
         * a thread GUI later, we need to re-register the classes.
         */

        if (!(ppi->W32PF_Flags & W32PF_CLASSESREGISTERED)) {
            if (!LW_RegisterWindows(FALSE)) {
                RIPMSG0(RIP_WARNING, "xxxCreateThreadInfo: LW_RegisterWindows failed");
                Status = STATUS_UNSUCCESSFUL;
                goto CreateThreadInfoFailed;
            }
            ppi->W32PF_Flags |= W32PF_CLASSESREGISTERED;
            if (ptiCurrent->pClientInfo) {
                ptiCurrent->pClientInfo->CI_flags |= CI_REGISTERCLASSES;
            }
        }

        if (fFirstThread) {

            /*
             * If this is an application starting (ie. not some thread of
             * the server context), enable the app-starting cursor.
             */
            DeferWinEventNotify();
            zzzCalcStartCursorHide((PW32PROCESS)pEProcess->Win32Process, 5000);
            EndDeferWinEventNotifyWithoutProcessing();

            /*
             * Open the windowstation
             */
            if (grpWinStaList && ppi->rpwinsta == NULL) {
                RIPERR0(ERROR_CAN_NOT_COMPLETE, RIP_WARNING, "System is not initialized\n");
                Status = STATUS_UNSUCCESSFUL;
                goto CreateThreadInfoFailed;
            }
        }
    } else {

        /*
         * Don't register system windows until cursors and icons
         * have been loaded.
         */
        if ((SYSCUR(ARROW) != NULL) &&
                !(ppi->W32PF_Flags & W32PF_CLASSESREGISTERED)) {

            ppi->W32PF_Flags |= W32PF_CLASSESREGISTERED;
            if (!LW_RegisterWindows(ptiCurrent->TIF_flags & TIF_SYSTEMTHREAD)) {
                RIPMSG0(RIP_WARNING, "xxxCreateThreadInfo: LW_RegisterWindows failed");
                Status = STATUS_UNSUCCESSFUL;
                goto CreateThreadInfoFailed;
            }
        }
    }


    /*
     * Initialize hung timer value
     */

    SET_TIME_LAST_READ(ptiCurrent);

    /*
     * If someone is waiting on this process propagate that info into
     * the thread info
     */
    if (ppi->W32PF_Flags & W32PF_WAITFORINPUTIDLE)
        ptiCurrent->TIF_flags |= TIF_WAITFORINPUTIDLE;

    /*
     * Mark the thread as initialized.
     */
    ptiCurrent->TIF_flags |= TIF_GUITHREADINITIALIZED;

    /*
     * Allow the thread to come to foreground when it is created
     * if the current process is the foreground process or the last input owner
     * This Flag is a hack to fix Bug 28502.  When we click on
     * "Map Network Drive" button on the toolbar, the explorer (Bobday)
     * creates another thread to create the dialog box. This will create
     * the dialog in the background. We are adding this fix at the request
     * of the Shell team so that this dialog comes up as foreground.
     *
     * If the process already has the foreground right, we don't give it
     *  to this thread (it doesn't need it). We do this to narrow the number
     *  of ways this process can force the foreground.
     * Also, if the process is starting, it already has the right unless
     *  the user has canceled it -- in which case we don't want to give it back.
     *
     */
     if (!(ppi->W32PF_Flags & (W32PF_ALLOWFOREGROUNDACTIVATE | W32PF_APPSTARTING))) {
         if (((gptiForeground != NULL) && (ppi == gptiForeground->ppi))
                || ((glinp.ptiLastWoken != NULL) && (ppi == glinp.ptiLastWoken->ppi))) {

            ptiCurrent->TIF_flags |= TIF_ALLOWFOREGROUNDACTIVATE;
            TAGMSG1(DBGTAG_FOREGROUND, "xxxCreateThreadInfo set TIF %#p", ptiCurrent);
         }
     }

    if (IS_IME_ENABLED()) {
        /*
         * Create per-thread default input context
         */
        CreateInputContext(0);
    }

    /*
     * Call back to the client to finish initialization.
     */
    if (!(dwTIFlags & (TIF_SYSTEMTHREAD | TIF_CSRSSTHREAD))) {

        if (SetAppCompatFlags(ptiCurrent)) {
            /*
             * Flag this process as a setup app.
             */
            ppi->W32PF_Flags |= W32PF_SETUPAPP;
        }

        Status = ClientThreadSetup();
        if (!NT_SUCCESS(Status)) {
            RIPMSG1(RIP_WARNING, "ClientThreadSetup failed with NTSTATUS %lx", Status);
            goto CreateThreadInfoFailed;
        }
    }

    if ((NT_SUCCESS(Status) && fFirstThread) &&
        !(ppi->W32PF_Flags & W32PF_CONSOLEAPPLICATION)) {

        /*
         * Don't play the sound for console processes
         * since we will play it when the console window
         * will be created
         */
        PlayEventSound(USER_SOUND_OPEN);
    }

    /*
     * Release desktop.
     * Some other thread might have been waiting to destroy this desktop
     *  when xxxResolveDestktop got a handle to it. So let's double
     *  check this now that we have called back several times after getting
     *  the handle back.
     */
    if (pdesk != NULL) {
        if (pdesk->dwDTFlags & DF_DESTROYED) {
            RIPMSG1(RIP_WARNING, "xxxCreateThreadInfo: pdesk destroyed:%#p", pdesk);
            Status = STATUS_UNSUCCESSFUL;
            goto CreateThreadInfoFailed;
        }
        ThreadUnlockDesktop(ptiCurrent, &tlpdesk, LDUT_FN_CREATETHREADINFO1);
    }

    // We must return a success here. If the failure status is returned
    // W32Thread will be freed without us going thru xxxDestroyProcessInfo.
    UserAssert(NT_SUCCESS(Status));

    return Status;

CreateThreadInfoFailed:

    RIPMSG2(RIP_WARNING, "xxxCreateThreadInfo: failed: pti %#p pdesk %#p",
            ptiCurrent, pdesk);

    if (pdesk != NULL) {
        ThreadUnlockDesktop(ptiCurrent, &tlpdesk, LDUT_FN_CREATETHREADINFO2);
    }
    xxxDestroyThreadInfo();
    return Status;
}

/***************************************************************************\
* AllocQueue
*
* Allocates the memory for a TI structure and initializes its fields.
* Each Win32 queue has it's own TI while all Win16 threads share the same
* TI.
*
* History:
* 02-21-91 MikeHar      Created.
\***************************************************************************/

PQ AllocQueue(
    PTHREADINFO ptiKeyState,    // if non-Null then use this key state
                                // other wise use global AsyncKeyState
    PQ pq)                      // non-null == preallocated object
{
    USHORT cLockCount;

    if (pq == NULL) {
        pq = ExAllocateFromPagedLookasideList(QLookaside);
        if (pq == NULL) {
            return NULL;
        }
        cLockCount = 0;
    } else {
        DebugValidateMLIST(&pq->mlInput);
        /*
         * Preserve lock count.
         */
        cLockCount = pq->cLockCount;
    }
    RtlZeroMemory(pq, sizeof(Q));
    pq->cLockCount = cLockCount;

    /*
     * This is a new queue; we need to update its key state table before
     * the first input event is put in the queue.
     * We do this by copying the current keystate table and NULLing the recent
     * down state table.  If a key is really down it will be updated when
     * we get it repeats.
     *
     * He is the old way that did not work because if the first key was say an
     * alt key the Async table would be updated, then the UpdateKeyState
     * message and it would look like the alt key was PREVIOUSLY down.
     *
     * The queue will get updated when it first reads input: to allow the
     * app to query the key state before it calls GetMessage, set its initial
     * key state to the asynchronous key state.
     */
    if (ptiKeyState) {
        RtlCopyMemory(pq->afKeyState, ptiKeyState->pq->afKeyState, CBKEYSTATE);
    } else {
        RtlCopyMemory(pq->afKeyState, gafAsyncKeyState, CBKEYSTATE);
    }

    /*
     * If there isn't a mouse set iCursorLevel to -1 so the
     * mouse cursor won't be visible on the screen.
     */
    if (
        !TEST_GTERMF(GTERMF_MOUSE)) {
            pq->iCursorLevel--;
    }
    /*
     * While the thread is starting up...  it has the wait cursor.
     */
    LockQCursor(pq, SYSCUR(WAIT));

    DebugValidateMLIST(&pq->mlInput);
    return pq;
}

/***************************************************************************\
* FreeQueue
*
* 04-04-96 GerardoB    Created.
\***************************************************************************/
VOID FreeQueue(
    PQ pq)
{
#if DBG
    /*
     * Turn off the flag indicating that this queue is in destruction.
     * We do this in either case that we are putting this into the free
     * list, or truly destroying the handle.  We use this to try and
     * track cases where someone tries to lock elements into the queue
     * structure while it's going through destuction.
     */
    pq->QF_flags &= ~QF_INDESTROY;
#endif

    UserAssertMsg0(pq != gpqForeground, "FreeQueue(gpqForeground) !");
    UserAssertMsg0(pq != gpqForegroundPrev, "FreeQueue(gpqForegroundPrev) !");
    UserAssertMsg0(pq != gpqCursor, "FreeQueue(gpqCursor) !");
    ExFreeToPagedLookasideList(QLookaside, pq);
}

/***************************************************************************\
* FreeCachedQueues
*
* 14-Jan-98 CLupu    Created.
\***************************************************************************/
VOID FreeCachedQueues(
    VOID)
{
    if (QLookaside != NULL) {
        ExDeletePagedLookasideList(QLookaside);
        UserFreePool(QLookaside);
        QLookaside = NULL;
    }
}

/***************************************************************************\
* zzzDestroyQueue
*
*
* History:
* 05-20-91 MikeHar      Created.
\***************************************************************************/

void zzzDestroyQueue(
    PQ          pq,
    PTHREADINFO pti)
{
    PTHREADINFO ptiT;
    PTHREADINFO ptiAny, ptiBestMouse, ptiBestKey;
    PLIST_ENTRY pHead, pEntry;

#if DBG
    USHORT cDying = 0;
#endif

    BOOL fSetFMouseMoved = FALSE;

    DebugValidateMLIST(&pq->mlInput);

    UserAssert(pq->cThreads);
    pq->cThreads--;

    if (pq->cThreads != 0) {

        /*
         * Since we aren't going to destroy this queue, make sure
         * it isn't pointing to the THREADINFO that's going away.
         */
        if (pq->ptiSysLock == pti) {
            CheckSysLock(6, pq, NULL);
            pq->ptiSysLock = NULL;
        }

        if ((pq->ptiKeyboard == pti) || (pq->ptiMouse == pti)) {

            /*
             * Run through THREADINFOs looking for one pointing to pq.
             */
            ptiAny = NULL;
            ptiBestMouse = NULL;
            ptiBestKey = NULL;

            pHead = &pti->rpdesk->PtiList;
            for (pEntry = pHead->Flink; pEntry != pHead; pEntry = pEntry->Flink) {
                ptiT = CONTAINING_RECORD(pEntry, THREADINFO, PtiLink);

                /*
                 * Skip threads that are going away or belong to a
                 * different queue.
                 */
                if ((ptiT->TIF_flags & TIF_INCLEANUP) || (ptiT->pq != pq)) {
#if DBG
                    if (ptiT->pq == pq && (ptiT->TIF_flags & TIF_INCLEANUP))
                        cDying++;
#endif
                    continue;
                }

                ptiAny = ptiT;

                if (pti->pcti->fsWakeBits & QS_MOUSE) {
                    if (ptiT->pcti->fsWakeMask & QS_MOUSE)
                        ptiBestMouse = ptiT;
                }

                if (pti->pcti->fsWakeBits & QS_KEY) {
                    if (ptiT->pcti->fsWakeMask & QS_KEY)
                        ptiBestKey = ptiT;
                }
            }

            if (ptiBestMouse == NULL)
                ptiBestMouse = ptiAny;
            if (ptiBestKey == NULL)
                ptiBestKey = ptiAny;

            /*
             * Transfer any wake-bits to this new queue.  This
             * is a common problem for QS_MOUSEMOVE which doesn't
             * get set on coalesced WM_MOUSEMOVE events, so we
             * need to make sure the new thread tries to process
             * any input waiting in the queue.
             */
            if (ptiBestMouse != NULL)
                SetWakeBit(ptiBestMouse, pti->pcti->fsWakeBits & QS_MOUSE);
            if (ptiBestKey != NULL)
                SetWakeBit(ptiBestKey, pti->pcti->fsWakeBits & QS_KEY);

            if (pq->ptiKeyboard == pti)
                pq->ptiKeyboard = ptiBestKey;

            if (pq->ptiMouse == pti)
                pq->ptiMouse = ptiBestMouse;

#if DBG
            /*
             * Bad things happen if ptiKeyboard or ptiMouse are NULL
             */
            if (pq->cThreads != cDying && (pq->ptiKeyboard == NULL || pq->ptiMouse == NULL)) {
                RIPMSG6(RIP_ERROR,
                        "pq %#p pq->cThreads %x cDying %x pti %#p ptiK %#p ptiM %#p",
                        pq, pq->cThreads, cDying, pti, pq->ptiKeyboard, pq->ptiMouse);
            }
#endif
        }

        return;
    }

    /*
     * Unlock any potentially locked globals now that we know absolutely
     * that this queue is going away.
     */
    UnlockCaptureWindow(pq);
    Unlock(&pq->spwndFocus);
    Unlock(&pq->spwndActive);
    Unlock(&pq->spwndActivePrev);
    Unlock(&pq->caret.spwnd);
    LockQCursor(pq, NULL);

#if DBG
    /*
     * Mark this queue as being in the destruction process.  This is
     * cleared in FreeQueue() once we have determined it's safe to
     * place in the free-list, or destroy the handle.  We use this
     * to track cases where someone will lock a cursor into the queue
     * while it's in the middle of being destroyed.
     */
    pq->QF_flags |= QF_INDESTROY;
#endif

    /*
     * Free everything else that was allocated/created by AllocQueue.
     */
    FreeMessageList(&pq->mlInput);

    /*
     * If this queue is in the foreground, set gpqForeground
     * to NULL so no input is routed.  At some point we'll want
     * to do slightly more clever assignment of gpqForeground here.
     */
    if (gpqForeground == pq) {
        gpqForeground = NULL;
    }

    if (gpqForegroundPrev == pq) {
        gpqForegroundPrev = NULL;
    }

    if (gpqCursor == pq) {
        gpqCursor = NULL;
        fSetFMouseMoved = TRUE;
    }

    if (pq->cLockCount == 0) {
        FreeQueue(pq);
    }

    if (fSetFMouseMoved) {
        zzzSetFMouseMoved();
    }

}

/**************************************************************************\
* UserDeleteW32Thread
*
* This function is called when the W32THREAD reference count goes
*  down to zero. So everything left around by xxxDestroyThreadInfo
*  must be cleaned up here.
*
* SO VERY IMPORTANT:
* Note that this call is not in the context of the pti being cleaned up,
*  in other words, pti != PtiCurrent(). So only kernel calls are allowed here.
*
* 04-01-96 GerardoB   Created
\**************************************************************************/
VOID UserDeleteW32Thread (PW32THREAD pW32Thread)
{
    PTHREADINFO pti = (PTHREADINFO)pW32Thread;

    BEGIN_REENTERCRIT();

    /*
     * Make sure the ref count didn't get bumped up while we were waiting.
     */
    if (pW32Thread->RefCount == 0) {

        /*
         * Events
         */
        if (pti->pEventQueueServer != NULL) {
            ObDereferenceObject(pti->pEventQueueServer);
        }
        if (pti->apEvent != NULL) {
            UserFreePool(pti->apEvent);
        }

        /*
         * App name.
         */
        if (pti->pstrAppName != NULL) {
            UserFreePool(pti->pstrAppName);
        }

        /*
         * Unlock the queues and free them if no one is using them
         * (the queues were already destroyed in DestroyThreadInfo)
         */
        if (pti->pq != NULL) {

            UserAssert(pti->pq->cLockCount);
            --(pti->pq->cLockCount);

            if ((pti->pq->cLockCount == 0)
                    && (pti->pq->cThreads == 0)) {
                FreeQueue(pti->pq);
            }

        }
        /*
         * zzzReattachThreads shouldn't call back while using pqAttach
         */
        UserAssert(pti->pqAttach == NULL);
        #if 0
        if (pti->pqAttach != NULL) {

            UserAssert(pti->pqAttach->cLockCount);
            --(pti->pqAttach->cLockCount);

            if ((pti->pqAttach->cLockCount == 0)
                    && (pti->pqAttach->cThreads == 0)) {
                FreeQueue(pti->pqAttach);
            }

        }
        #endif
        /*
         * Unlock the desktop (pti already unlinked from ptiList)
         */
        if (pti->rpdesk != NULL) {
            UnlockDesktop(&pti->rpdesk, LDU_PTI_DESK, (ULONG_PTR)pti);
        }

        /*
         * Remove the pointer to this W32Thread and free the associated memory.
         */
        InterlockedCompareExchangePointer(&pW32Thread->pEThread->Tcb.Win32Thread, NULL, pW32Thread);
        Win32FreePool(pW32Thread);
    }

    END_REENTERCRIT();
}

/**************************************************************************\
* UserDeleteW32Process
*
* This function is called when the W32PROCESS reference count goes
*  down to zero. So everything left around by DestroyProcessInfo
*  must be cleaned up here.
*
* SO VERY IMPORTANT:
* Note that this call may not be in the context of the ppi being cleaned up,
*  in other words, ppi != PpiCurrent(). So only kernel calls are allowed here.
*
* 04-01-96 GerardoB   Created
\**************************************************************************/
VOID UserDeleteW32Process(PW32PROCESS pW32Process)
{
    PPROCESSINFO ppi = (PPROCESSINFO)pW32Process;

    BEGIN_REENTERCRIT();

    /*
     * Make sure the ref count didn't get bumped up while we were waiting.
     */
    if (pW32Process->RefCount == 0) {

        /*
         * Grab the handle flags lock. We can't call into the object manager when
         * we have this or we might deadlock.
         */
        EnterHandleFlagsCrit();

        /*
         * Delete handle flags attribute bitmap
         */
        if (ppi->bmHandleFlags.Buffer) {
            UserFreePool(ppi->bmHandleFlags.Buffer);
            RtlInitializeBitMap(&ppi->bmHandleFlags, NULL, 0);
        }

        /*
         * Remove the pointer to this W32Process and free the associated memory.
         */
        InterlockedCompareExchangePointer(&pW32Process->Process->Win32Process, NULL, pW32Process);
        Win32FreePool(pW32Process);

        /*
         * Release the handle flags lock.
         */
        LeaveHandleFlagsCrit();
    }

    END_REENTERCRIT();
}

/***************************************************************************\
* FLastGuiThread
*
* Check if this is the last GUI thread in the process.
\***************************************************************************/

__inline BOOL FLastGuiThread(PTHREADINFO pti)
{
    return (pti->ppi &&
            pti->ppi->ptiList == pti &&
            pti->ptiSibling == NULL);
}

/***************************************************************************\
* xxxDestroyThreadInfo
*
* Destroys a THREADINFO created by xxxCreateThreadInfo().
*
*  Note that the current pti can be locked so it might be used after this
*   function returns, eventhough the thread execution has ended.
*  We want to stop any activity on this thread so we clean up any USER stuff
*   like messages, clipboard, queue, etc and specially anything that assumes
*   to be running on a Win32 thread and client side stuff.
*  The final cleanup will take place in UserDeleteW32Thread
*
*  This function must not go into the user mode because the ntos data
*  structures may no longer support it and it may bluescreen the system.
*
* Make all callbacks before the thread objects are destroyed. If you callback
*  afterwards, new objects might be created and won't be cleaned up.
*
* History:
* 02-15-91 DarrinM      Created.
* 02-27-91 mikeke       Made it work
* 02-27-91 Mikehar      Removed queue from the global list
\***************************************************************************/

VOID xxxDestroyThreadInfo(VOID)
{
    PTHREADINFO ptiCurrent;
    PTHREADINFO *ppti;

    ptiCurrent = PtiCurrent();
    UserAssert (ptiCurrent != NULL);
    UserAssert(IsWinEventNotifyDeferredOK());

    /*
     * If this thread is blocking input, stop it
     */
    if (gptiBlockInput == ptiCurrent) {
        gptiBlockInput = NULL;
    }

    /*
     * Don't mess with this ptiCurrent anymore.
     */
    ptiCurrent->TIF_flags |= (TIF_DONTATTACHQUEUE | TIF_INCLEANUP);

    /*
     * First do any preparation work: windows need to be "patched" so that
     * their window procs point to server only windowprocs, for example.
     */
    PatchThreadWindows(ptiCurrent);

    /*
     * If this thread terminated abnormally and was tracking tell
     * GDI to hide the trackrect.
     */
    if (ptiCurrent->pmsd != NULL) {
        xxxCancelTrackingForThread(ptiCurrent);
    }

    /*
     * Unlock the pmsd window.
     */
    if (ptiCurrent->pmsd != NULL) {
        Unlock(&ptiCurrent->pmsd->spwnd);
        UserFreePool(ptiCurrent->pmsd);
        ptiCurrent->pmsd = NULL;
    }

    /*
     * Free the clipboard if owned by this thread
     */
    {
        PWINDOWSTATION pwinsta;
        pwinsta = _GetProcessWindowStation(NULL);
        if (pwinsta != NULL) {
            if (pwinsta->ptiClipLock == ptiCurrent) {
                xxxCloseClipboard(pwinsta);
            }
            if (pwinsta->ptiDrawingClipboard == ptiCurrent) {
                pwinsta->ptiDrawingClipboard = NULL;
            }
        }
    }

    /*
     * Unlock all the objects stored in the menustate structure
     */
    while (ptiCurrent->pMenuState != NULL) {
        PMENUSTATE pMenuState;
        PPOPUPMENU ppopupmenuRoot;

        pMenuState = ptiCurrent->pMenuState;
        ppopupmenuRoot = pMenuState->pGlobalPopupMenu;

        /*
         * If menu mode was running on this thread
         */
        if (ptiCurrent == pMenuState->ptiMenuStateOwner) {
            /*
             * The menu's going away, so anyone who's locked it
             * is SOL anyway. Bug #375467.
             */
            pMenuState->dwLockCount = 0;

            /*
             * Close this menu.
             */
            if (pMenuState->fModelessMenu) {
                xxxEndMenuLoop(pMenuState, ppopupmenuRoot);
                xxxMNEndMenuState(TRUE);
            } else {
                pMenuState->fInsideMenuLoop = FALSE;
                ptiCurrent->pq->QF_flags &= ~QF_CAPTURELOCKED;
                xxxMNCloseHierarchy(ppopupmenuRoot, pMenuState);
                xxxMNEndMenuState(ppopupmenuRoot->fIsMenuBar || ppopupmenuRoot->fDestroyed);
            }
        } else {
            /*
             * Menu mode is running on another thread. This thread
             *  must own spwndNotify which is going away soon.
             * When spwndNotify is destroyed, we will clean up pMenuState
             *  from this pti. So do nothing now as we'll need this
             *  pMenuState at that time.
             */
            UserAssert((ppopupmenuRoot->spwndNotify != NULL)
                    && (GETPTI(ppopupmenuRoot->spwndNotify) == ptiCurrent));

            /*
             * Nested menus are not supposed to involve multiple threads
             */
            UserAssert(pMenuState->pmnsPrev == NULL);
            break;
        }

    } /* while (ptiCurrent->pMenuState != NULL) */

#if DBG
    /*
     * This thread must not be using the desktop menu
     */
    if ((ptiCurrent->rpdesk != NULL) && (ptiCurrent->rpdesk->spwndMenu != NULL)) {
        UserAssert(ptiCurrent != GETPTI(ptiCurrent->rpdesk->spwndMenu));
    }
#endif

    /*
     * Unlock all the objects stored in the sbstate structure.
     */
    if (ptiCurrent->pSBTrack) {
        Unlock(&ptiCurrent->pSBTrack->spwndSB);
        Unlock(&ptiCurrent->pSBTrack->spwndSBNotify);
        Unlock(&ptiCurrent->pSBTrack->spwndTrack);
        UserFreePool(ptiCurrent->pSBTrack);
        ptiCurrent->pSBTrack = NULL;
    }

    /*
     * If this is the main input thread of this application, zero out
     * that field.
     */
    if (ptiCurrent->ppi != NULL && ptiCurrent->ppi->ptiMainThread == ptiCurrent)
        ptiCurrent->ppi->ptiMainThread = NULL;

    while (ptiCurrent->psiiList != NULL) {
        xxxDestroyThreadDDEObject(ptiCurrent, ptiCurrent->psiiList);
    }

    if (ptiCurrent->TIF_flags & TIF_PALETTEAWARE) {
        PWND pwnd;
        TL tlpwnd;

        UserAssert(ptiCurrent->rpdesk != NULL);

        pwnd = ptiCurrent->rpdesk->pDeskInfo->spwnd;

        ThreadLock(pwnd, &tlpwnd);
        xxxFlushPalette(pwnd);
        ThreadUnlock(&tlpwnd);
    }

    /*
     * If this is the last GUI thread for the process that made a temporary
     * (fullscreen) mode change, restore the mode to what's in the registry.
     */
    if (FLastGuiThread(ptiCurrent) && (gppiFullscreen == ptiCurrent->ppi)) {
        xxxUserChangeDisplaySettings(NULL, NULL, NULL, NULL, 0, 0, KernelMode);

        UserAssert(gppiFullscreen != ptiCurrent->ppi);
    }

/*******************************************************************************************\
 *                                                                                         *
 *          CLEANING THREAD OBJECTS. AVOID CALLING BACK AFTER THIS POINT                   *
 *      New objects might be created while calling back and won't be cleaned up            *
 *                                                                                         *
\*******************************************************************************************/

    /*
     * This thread might have some outstanding timers.  Destroy them
     */
    DestroyThreadsTimers(ptiCurrent);

    /*
     * Free any windows hooks this thread has created.
     */
    FreeThreadsWindowHooks();

    /*
     * Free any hwnd lists the thread was using
     */
    {
       PBWL pbwl, pbwlNext;
       for (pbwl = gpbwlList; pbwl != NULL; ) {
           pbwlNext = pbwl->pbwlNext;
           if (pbwl->ptiOwner == ptiCurrent) {
               FreeHwndList(pbwl);
           }
           pbwl = pbwlNext;
       }
    }

    /*
     * Destroy all the public objects created by this thread.
     */
    DestroyThreadsHotKeys();

    DestroyThreadsObjects();

    /*
     * Free any synchronous Notifies pending for this thread and
     * free any Win Event Hooks this thread created.
     */
    FreeThreadsWinEvents(ptiCurrent);

    /*
     * Unlock the keyboard layouts here
     */
    Unlock(&ptiCurrent->spklActive);

    /*
     * Cleanup the global resources if this is the last GUI
     * thread for this session
     */
    if (gdwGuiThreads == 1) {
        CleanupResources();
    }


    if (FLastGuiThread(ptiCurrent)) {

        /*
         * Check if this was a setup app.
         */
        if (ptiCurrent->ppi->W32PF_Flags & W32PF_SETUPAPP) {
            PDESKTOPINFO pdeskinfo = GETDESKINFO(ptiCurrent);
            if (pdeskinfo->spwndShell) {
                _PostMessage(pdeskinfo->spwndShell, DTM_SETUPAPPRAN, 0, 0);
            }
        }

        DestroyProcessesClasses(ptiCurrent->ppi);
        ptiCurrent->ppi->W32PF_Flags &= ~(W32PF_CLASSESREGISTERED);


        DestroyProcessesObjects(ptiCurrent->ppi);
    }

#ifdef FE_IME
    /*
     * Unlock default input context.
     */
    Unlock(&ptiCurrent->spDefaultImc);
#endif

    if (ptiCurrent->pq != NULL) {
        /*
         * Remove this thread's cursor count from the queue.
         */
        ptiCurrent->pq->iCursorLevel -= ptiCurrent->iCursorLevel;

        /*
         * Have to recalc queue ownership after this thread
         * leaves if it is a member of a shared input queue.
         */
        if (ptiCurrent->pq->cThreads != 1)
        {
            gpdeskRecalcQueueAttach = ptiCurrent->rpdesk;
            /*
             * Because we are in thread cleanup, we won't callback due
             * to WinEvents (zzzSetFMouseMoved calls zzzUpdateCursorImage)
             */
            UserAssert(ptiCurrent->TIF_flags & TIF_INCLEANUP);
            UserAssert(gbExitInProgress == FALSE);
            zzzSetFMouseMoved();
        }
    }

    /*
     * Remove from the process' list, also.
     */
    ppti = &PpiCurrent()->ptiList;
    if (*ppti != NULL) {
        while (*ppti != ptiCurrent && (*ppti)->ptiSibling != NULL) {
            ppti = &((*ppti)->ptiSibling);
        }
        if (*ppti == ptiCurrent) {
            *ppti = ptiCurrent->ptiSibling;
            ptiCurrent->ptiSibling = NULL;
        }
    }

    {
        PDESKTOP rpdesk;
        PATTACHINFO *ppai;

        /*
         * Temporarily lock the desktop until the THREADINFO structure is
         * freed.  Note that locking a NULL ptiCurrent->rpdesk is OK.  Use a
         * normal lock instead of a thread lock because the lock must
         * exist past the freeing of the ptiCurrent.
         */
        rpdesk = NULL;
        LockDesktop(&rpdesk, ptiCurrent->rpdesk, LDL_FN_DESTROYTHREADINFO, (ULONG_PTR)PtiCurrent());

        /*
         * Cleanup SMS structures attached to this thread.  Handles both
         * pending send and receive messages. MUST make sure we do SendMsgCleanup
         * AFTER window cleanup.
         */
        SendMsgCleanup(ptiCurrent);


        /*
         * Allow this thread to be swapped
         */
        if (ptiCurrent->cEnterCount) {
            BOOLEAN bool;

            RIPMSG1(RIP_WARNING, "Thread exiting with stack locked.  pti:%#p\n", ptiCurrent);
            bool = KeSetKernelStackSwapEnable(TRUE);
            ptiCurrent->cEnterCount = 0;
            UserAssert(!bool);
        }

        if (ptiCurrent->ppi != NULL) {
            ptiCurrent->ppi->cThreads--;
            UserAssert(ptiCurrent->ppi->cThreads >= 0);
        }

        /*
         * If this thread is a win16 task, remove it from the scheduler.
         */
        if (ptiCurrent->TIF_flags & TIF_16BIT) {
            if ((ptiCurrent->ptdb) && (ptiCurrent->ptdb->hTaskWow != 0)) {
                _WOWCleanup(NULL, ptiCurrent->ptdb->hTaskWow);
            }
            DestroyTask(ptiCurrent->ppi, ptiCurrent);
        }

        if (ptiCurrent->hEventQueueClient != NULL) {
            ProtectHandle(ptiCurrent->hEventQueueClient, FALSE);
            ZwClose(ptiCurrent->hEventQueueClient);
            ptiCurrent->hEventQueueClient = NULL;
        }


        if (gspwndInternalCapture != NULL) {
            if (GETPTI(gspwndInternalCapture) == ptiCurrent) {
                Unlock(&gspwndInternalCapture);
            }
        }

        /*
         * Set gptiForeground to NULL if equal to this pti before exiting
         * this routine.
         */
        if (gptiForeground == ptiCurrent) {
            if (FWINABLE()) {
                /*
                 * Post these (WEF_ASYNC), since we can't make callbacks from here.
                 */
                xxxWindowEvent(EVENT_OBJECT_FOCUS, NULL, OBJID_CLIENT, INDEXID_CONTAINER, WEF_ASYNC);
                xxxWindowEvent(EVENT_SYSTEM_FOREGROUND, NULL, OBJID_WINDOW, INDEXID_CONTAINER, WEF_ASYNC);
            }

            /*
             * Call the Shell to ask it to activate its main window.
             * This will be accomplished with a PostMessage() to itself,
             * so the actual activation will take place later.
             */
            UserAssert(rpdesk != NULL);

            if (rpdesk->pDeskInfo->spwndProgman)
                _PostMessage(rpdesk->pDeskInfo->spwndProgman, guiActivateShellWindow, 0, 0);

            /*
             * Set gptiForeground to NULL because we're destroying it.
             */
            SetForegroundThread(NULL);

            /*
             * If this thread is attached to gpqForeground AND it's the
             * last thread in the queue, then zzzDestroyQueue will NULL out
             * qpqForeground. Due to journalling attaching, gptiForegrouund
             * is not always attached to gpqForeground. This is one reason
             * why we no longer NULL out gpqForeground as stated in the old
             * comment. The other reason is that there might be other threads
             * in the foreground queue so there is no need to zap it. This was
             * messing up MsTest (now called VisualTest)
             * This is the old comment:
             * "Since gpqForeground is derived from the foreground thread
             * structure, set it to NULL as well, since there now is no
             * foreground thread structure"
             *
             * qpqForeground = NULL;
             */
        }


        /*
         * If this thread got the last input event, pass ownership to another
         *  thread in this process or to the foreground thread.
         */
        if (ptiCurrent == glinp.ptiLastWoken) {
            UserAssert(PpiCurrent() == ptiCurrent->ppi);
            if (ptiCurrent->ppi->ptiList != NULL) {
                UserAssert (ptiCurrent != ptiCurrent->ppi->ptiList);
                glinp.ptiLastWoken = ptiCurrent->ppi->ptiList;
            } else {
                glinp.ptiLastWoken = gptiForeground;
            }
        }

        /*
         * Make sure none of the other global thread pointers are pointing to us.
         */
        if (gptiShutdownNotify == ptiCurrent) {
            gptiShutdownNotify = NULL;
        }
        if (gptiTasklist == ptiCurrent) {
            gptiTasklist = NULL;
        }
        if (gHardErrorHandler.pti == ptiCurrent) {
            gHardErrorHandler.pti = NULL;
        }

        /*
         * May be called from xxxCreateThreadInfo before the queue is created
         * so check for NULL queue.
         * Lock the queues since this pti might be locked. They will be unlocked
         *  in UserDeleteW32Thread
         */
        if (ptiCurrent->pq != NULL) {
            UserAssert(ptiCurrent->pq != ptiCurrent->pqAttach);
            DestroyThreadsMessages(ptiCurrent->pq, ptiCurrent);
            (ptiCurrent->pq->cLockCount)++;
            zzzDestroyQueue(ptiCurrent->pq, ptiCurrent);
        }

        /*
         * zzzReattachThreads shouldn't call back while using pqAttach
         */
        UserAssert(ptiCurrent->pqAttach == NULL);
        #if 0
        if (ptiCurrent->pqAttach != NULL) {
            DestroyThreadsMessages(ptiCurrent->pqAttach, ptiCurrent);
            (ptiCurrent->pqAttach->cLockCount)++;
            zzzDestroyQueue(ptiCurrent->pqAttach, ptiCurrent);
        }
        #endif

        /*
         * Remove the pti from its pti list and reset the pointers.
         */
        if (ptiCurrent->rpdesk != NULL) {
            RemoveEntryList(&ptiCurrent->PtiLink);
            InitializeListHead(&ptiCurrent->PtiLink);
        }

        FreeMessageList(&ptiCurrent->mlPost);

        /*
         * Free any attachinfo structures pointing to this thread
         */
        ppai = &gpai;
        while ((*ppai) != NULL) {
            if ((*ppai)->pti1 == ptiCurrent || (*ppai)->pti2 == ptiCurrent) {
                PATTACHINFO paiKill = *ppai;
                *ppai = (*ppai)->paiNext;
                UserFreePool((HLOCAL)paiKill);
            } else {
                ppai = &(*ppai)->paiNext;
            }
        }

        /*
         * Change ownership of any objects that didn't get freed (because they
         * are locked or we have a bug and the object didn't get destroyed).
         */
        MarkThreadsObjects(ptiCurrent);

        /*
         * Free thread information visible from client
         */
        if (rpdesk && ptiCurrent->pcti != NULL && ptiCurrent->pcti != &(ptiCurrent->cti)) {
            DesktopFree(rpdesk, ptiCurrent->pcti);
            ptiCurrent->pcti = &(ptiCurrent->cti);
        }

        /*
         * Free the client info for system threads
         */
        if (ptiCurrent->TIF_flags & TIF_SYSTEMTHREAD && ptiCurrent->pClientInfo != NULL) {
            UserFreePool(ptiCurrent->pClientInfo);
            ptiCurrent->pClientInfo = NULL;
        }

        /*
         * Unlock the temporary desktop lock. ptiCurrent->rpdesk is still locked
         *  and will be unlocked in UserDeleteW32Thread.
         */
        UnlockDesktop(&rpdesk, LDU_FN_DESTROYTHREADINFO, (ULONG_PTR)PtiCurrent());
    }

    /*
     * One more thread died.
     */
    gdwGuiThreads--;
}


/***************************************************************************\
* CleanEventMessage
*
* This routine takes a message and destroys and event message related pieces,
* which may be allocated.
*
* 12-10-92 ScottLu      Created.
\***************************************************************************/

void CleanEventMessage(
    PQMSG pqmsg)
{
    PASYNCSENDMSG pmsg;

    /*
     * Certain special messages on the INPUT queue have associated
     * bits of memory that need to be freed.
     */
    switch (pqmsg->dwQEvent) {
    case QEVENT_SETWINDOWPOS:
        UserFreePool((PSMWP)pqmsg->msg.wParam);
        break;

    case QEVENT_UPDATEKEYSTATE:
        UserFreePool((PBYTE)pqmsg->msg.wParam);
        break;

    case QEVENT_NOTIFYWINEVENT:
        DestroyNotify((PNOTIFY)pqmsg->msg.lParam);
        break;

    case QEVENT_ASYNCSENDMSG:
        pmsg = (PASYNCSENDMSG)pqmsg->msg.wParam;
        UserDeleteAtom((ATOM)pmsg->lParam);
        UserFreePool(pmsg);
        break;
    }
}

/***************************************************************************\
* FreeMessageList
*
* History:
* 02-27-91  mikeke      Created.
* 11-03-92  scottlu     Changed to work with MLIST structure.
\***************************************************************************/

VOID FreeMessageList(
    PMLIST pml)
{
    PQMSG pqmsg;

    DebugValidateMLIST(pml);

    while ((pqmsg = pml->pqmsgRead) != NULL) {
        CleanEventMessage(pqmsg);
        DelQEntry(pml, pqmsg);
    }

    DebugValidateMLIST(pml);
}

/***************************************************************************\
* DestroyThreadsMessages
*
* History:
* 02-21-96  jerrysh     Created.
\***************************************************************************/

VOID DestroyThreadsMessages(
    PQ pq,
    PTHREADINFO pti)
{
    PQMSG pqmsg;
    PQMSG pqmsgNext;

    DebugValidateMLIST(&pq->mlInput);

    pqmsg = pq->mlInput.pqmsgRead;
    while (pqmsg != NULL) {
        pqmsgNext = pqmsg->pqmsgNext;
        if (pqmsg->pti == pti) {
            /*
             * Make sure we don't leave any bogus references to this message
             * lying around.
             */
            if (pq->idSysPeek == (ULONG_PTR)pqmsg) {
                CheckPtiSysPeek(8, pq, 0);
                pq->idSysPeek = 0;
            }
            CleanEventMessage(pqmsg);
            DelQEntry(&pq->mlInput, pqmsg);
        }
        pqmsg = pqmsgNext;
    }

    DebugValidateMLIST(&pq->mlInput);
}

/***************************************************************************\
* InitQEntryLookaside
*
* Initializes the Q entry lookaside list. This improves Q entry locality
* by keeping Q entries in a single page
*
* 09-09-93  Markl   Created.
\***************************************************************************/


NTSTATUS
InitQEntryLookaside()
{
    QEntryLookaside = UserAllocPoolNonPaged(sizeof(PAGED_LOOKASIDE_LIST), TAG_LOOKASIDE);
    if (QEntryLookaside == NULL) {
        return STATUS_NO_MEMORY;
    }

    ExInitializePagedLookasideList(QEntryLookaside,
                                   NULL,
                                   NULL,
                                   SESSION_POOL_MASK,
                                   sizeof(QMSG),
                                   TAG_QMSG,
                                   16);

    QLookaside = UserAllocPoolNonPaged(sizeof(PAGED_LOOKASIDE_LIST), TAG_LOOKASIDE);
    if (QLookaside == NULL) {
        return STATUS_NO_MEMORY;
    }

    ExInitializePagedLookasideList(QLookaside,
                                   NULL,
                                   NULL,
                                   SESSION_POOL_MASK,
                                   sizeof(Q),
                                   TAG_Q,
                                   16);
    return STATUS_SUCCESS;
}

/***************************************************************************\
* AllocQEntry
*
* Allocates a message on a message list. DelQEntry deletes a message
* on a message list.
*
* 10-22-92 ScottLu      Created.
\***************************************************************************/

PQMSG AllocQEntry(
    PMLIST pml)
{
    PQMSG pqmsg;

    DebugValidateMLIST(pml);

    if (pml->cMsgs >= gUserPostMessageLimit) {
        RIPMSG3(RIP_WARNING, "AllocQEntry: # of post messages exceeds the limit(%d) in pti=0x%p, pml=0x%p",
               gUserPostMessageLimit, W32GetCurrentThread(), pml);
        return NULL;
    }

    /*
     * Allocate a Q message structure.
     */
    if ((pqmsg = ExAllocateFromPagedLookasideList(QEntryLookaside)) == NULL) {
        return NULL;
    }

    RtlZeroMemory(pqmsg, sizeof(*pqmsg));

    if (pml->pqmsgWriteLast != NULL) {
        pml->pqmsgWriteLast->pqmsgNext = pqmsg;
        pqmsg->pqmsgPrev = pml->pqmsgWriteLast;
        pml->pqmsgWriteLast = pqmsg;
    } else {
        pml->pqmsgWriteLast = pml->pqmsgRead = pqmsg;
    }

    pml->cMsgs++;

    DebugValidateMLISTandQMSG(pml, pqmsg);

    return pqmsg;
}

/***************************************************************************\
* DelQEntry
*
* Simply removes a message from a message queue list.
*
* 10-20-92 ScottLu      Created.
\***************************************************************************/

void DelQEntry(
    PMLIST pml,
    PQMSG pqmsg)
{
    DebugValidateMLISTandQMSG(pml, pqmsg);
    UserAssert((int)pml->cMsgs > 0);
    UserAssert(pml->pqmsgRead);
    UserAssert(pml->pqmsgWriteLast);

    /*
     * Unlink this pqmsg from the message list.
     */
    if (pqmsg->pqmsgPrev != NULL)
        pqmsg->pqmsgPrev->pqmsgNext = pqmsg->pqmsgNext;

    if (pqmsg->pqmsgNext != NULL)
        pqmsg->pqmsgNext->pqmsgPrev = pqmsg->pqmsgPrev;

    /*
     * Update the read/write pointers if necessary.
     */
    if (pml->pqmsgRead == pqmsg)
        pml->pqmsgRead = pqmsg->pqmsgNext;

    if (pml->pqmsgWriteLast == pqmsg)
        pml->pqmsgWriteLast = pqmsg->pqmsgPrev;

    /*
     * Adjust the message count and free the message structure.
     */
    pml->cMsgs--;

    ExFreeToPagedLookasideList(QEntryLookaside, pqmsg);

    DebugValidateMLIST(pml);
}

/***************************************************************************\
* CheckRemoveHotkeyBit
*
* We have a special bit for the WM_HOTKEY message - QS_HOTKEY. When there
* is a WM_HOTKEY message in the queue, that bit is on. When there isn't,
* that bit is off. This checks for more than one hot key, because the one
* is about to be deleted. If there is only one, the hot key bits are cleared.
*
* 11-12-92 ScottLu      Created.
\***************************************************************************/

void CheckRemoveHotkeyBit(
    PTHREADINFO pti,
    PMLIST pml)
{
    PQMSG pqmsg;
    DWORD cHotkeys;

    /*
     * Remove the QS_HOTKEY bit if there is only one WM_HOTKEY message
     * in this message list.
     */
    cHotkeys = 0;
    for (pqmsg = pml->pqmsgRead; pqmsg != NULL; pqmsg = pqmsg->pqmsgNext) {
        if (pqmsg->msg.message == WM_HOTKEY)
            cHotkeys++;
    }

    /*
     * If there is 1 or fewer hot keys, remove the hotkey bits.
     */
    if (cHotkeys <= 1) {
        pti->pcti->fsWakeBits &= ~QS_HOTKEY;
        pti->pcti->fsChangeBits &= ~QS_HOTKEY;
    }
}

/***************************************************************************\
* FindQMsg
*
* Finds a qmsg that fits the filters by looping through the message list.
*
* 10-20-92 ScottLu      Created.
* 06-06-97 CLupu        added processing for WM_DDE_ACK messages
\***************************************************************************/

PQMSG FindQMsg(
    PTHREADINFO pti,
    PMLIST pml,
    PWND pwndFilter,
    UINT msgMin,
    UINT msgMax,
    BOOL bProcessAck)
{
    PWND pwnd;
    PQMSG pqmsgRead;
    PQMSG pqmsgRet = NULL;
    UINT message;

    DebugValidateMLIST(pml);

    pqmsgRead = pml->pqmsgRead;

    while (pqmsgRead != NULL) {

        /*
         * Make sure this window is valid and doesn't have the destroy
         * bit set (don't want to send it to any client side window procs
         * if destroy window has been called on it).
         */
        pwnd = RevalidateHwnd(pqmsgRead->msg.hwnd);

        if (pwnd == NULL && pqmsgRead->msg.hwnd != NULL) {
            /*
             * If we're removing a WM_HOTKEY message, we may need to
             * clear the QS_HOTKEY bit, since we have a special bit
             * for that message.
             */
            if (pqmsgRead->msg.message == WM_HOTKEY) {
                CheckRemoveHotkeyBit(pti, pml);
            }
            /*
             * If the current thread's queue is locked waiting for this message,
             *  we have to unlock it because we're eating the message. If there's
             *  no more input/messages for this thread, the thread is going to
             *  sleep; hence there might not be a next Get/PeekMessage call to
             *  unlock the queue (ie, updating pti->idLast is not enough);
             *  so we must unlock it now.
             * Win95 doesn't have this problem because their FindQMsg doesn't
             *  eat messages; they call ReadPostMessage from FreeWindow
             *  to take care of this scenario (== message for a destroyed window).
             *  We could also do this if we have some problems with this fix.
             */
            if ((pti->pq->idSysLock == (ULONG_PTR)pqmsgRead)
                    && (pti->pq->ptiSysLock == pti)) {
                /* CheckSysLock(What number?, pti->pq, NULL); */
                RIPMSG2(RIP_VERBOSE, "FindQMsg: Unlocking queue:%#p. Msg:%#lx",
                                        pti->pq, pqmsgRead->msg.message);
                pti->pq->ptiSysLock = NULL;
            }

            DelQEntry(pml, pqmsgRead);
            goto nextMsgFromPml;
        }

        /*
         * Process the WM_DDE_ACK messages if bProcessAck is set.
         */
        if (bProcessAck && (PtoH(pwndFilter) == pqmsgRead->msg.hwnd) &&
            (pqmsgRead->msg.message == (WM_DDE_ACK | MSGFLAG_DDE_MID_THUNK))) {

            PXSTATE pxs;

            pxs = (PXSTATE)HMValidateHandleNoRip((HANDLE)pqmsgRead->msg.lParam, TYPE_DDEXACT);

            if (pxs != NULL && (pxs->flags & XS_FREEPXS)) {
                FreeDdeXact(pxs);
                DelQEntry(pml, pqmsgRead);
                goto nextMsgFromPml;
            }
        }

        /*
         * Make sure this message fits both window handle and message
         * filters.
         */
        if (!CheckPwndFilter(pwnd, pwndFilter))
            goto nextMsg;

        /*
         * If this is a fixed up dde message, then turn it into a normal
         * dde message for the sake of message filtering.
         */
        message = pqmsgRead->msg.message;
        if (CheckMsgFilter(message,
                (WM_DDE_FIRST + 1) | MSGFLAG_DDE_MID_THUNK,
                WM_DDE_LAST | MSGFLAG_DDE_MID_THUNK)) {
            message = message & ~MSGFLAG_DDE_MID_THUNK;
        }

        if (!CheckMsgFilter(message, msgMin, msgMax))
            goto nextMsg;

        /*
         * Found it. If bProcessAck is set, remember this pointer and go on
         * till we finish walking the list to process all WM_DDE_ACK messages.
         */
        if (!bProcessAck) {
            DebugValidateMLIST(pml);
            return pqmsgRead;
        }

        if (pqmsgRet == NULL) {
            pqmsgRet = pqmsgRead;
        }
nextMsg:
        pqmsgRead = pqmsgRead->pqmsgNext;
        continue;

nextMsgFromPml:
        pqmsgRead = pml->pqmsgRead;
        continue;
    }

    DebugValidateMLIST(pml);
    return pqmsgRet;
}

/***************************************************************************\
* CheckQuitMessage
*
* Checks to see if a WM_QUIT message should be generated.
*
* 11-06-92 ScottLu      Created.
\***************************************************************************/

BOOL CheckQuitMessage(
    PTHREADINFO pti,
    LPMSG lpMsg,
    BOOL fRemoveMsg)
{
    /*
     * If there are no more posted messages in the queue and cQuit is !=
     * 0, then generate a quit!
     */
    if (pti->cQuit != 0 && pti->mlPost.cMsgs == 0) {
        /*
         * If we're "removing" the quit, set cQuit to 0 so another one isn't
         * generated.
         */
        if (fRemoveMsg)
            pti->cQuit = 0;
        StoreMessage(lpMsg, NULL, WM_QUIT, (DWORD)pti->exitCode, 0, 0);
        return TRUE;
    }

    return FALSE;
}


/***************************************************************************\
* ReadPostMessage
*
* If queue is not empty, read message satisfying filter conditions from
* this queue to *lpMsg. This routine is used for the POST MESSAGE list only!!
*
* 10-19-92 ScottLu      Created.
\***************************************************************************/

BOOL xxxReadPostMessage(
    PTHREADINFO pti,
    LPMSG lpMsg,
    PWND pwndFilter,
    UINT msgMin,
    UINT msgMax,
    BOOL fRemoveMsg)
{
    PQMSG pqmsg;
    PMLIST pmlPost;

    /*
     * Check to see if it is time to generate a quit message.
     */
    if (CheckQuitMessage(pti, lpMsg, fRemoveMsg))
        return TRUE;

    /*
     * Loop through the messages in this list looking for the one that
     * fits the passed in filters.
     */
    pmlPost = &pti->mlPost;
    pqmsg = FindQMsg(pti, pmlPost, pwndFilter, msgMin, msgMax, FALSE);
    if (pqmsg == NULL) {
        /*
         * Check again for quit... FindQMsg deletes some messages
         * in some instances, so we may match the conditions
         * for quit generation here.
         */
        if (CheckQuitMessage(pti, lpMsg, fRemoveMsg))
            return TRUE;
    } else {
        /*
         * Update the thread info fields with the info from this qmsg.
         */
        pti->timeLast = pqmsg->msg.time;
        if (!RtlEqualMemory(&pti->ptLast, &pqmsg->msg.pt, sizeof(POINT))) {
            pti->TIF_flags |= TIF_MSGPOSCHANGED;
        }
        pti->ptLast = pqmsg->msg.pt;

        pti->idLast = (ULONG_PTR)pqmsg;
        pti->pq->ExtraInfo = pqmsg->ExtraInfo;

        /*
         * Are we supposed to yank out the message? If not, stick some
         * random id into idLast so we don't unlock the input queue until we
         * pull this message from the queue.
         */
        *lpMsg = pqmsg->msg;
        if (!fRemoveMsg) {
            pti->idLast = 1;
        } else {
            /*
             * If we're removing a WM_HOTKEY message, we may need to
             * clear the QS_HOTKEY bit, since we have a special bit
             * for that message.
             */
            if (pmlPost->pqmsgRead->msg.message == WM_HOTKEY) {
                CheckRemoveHotkeyBit(pti, pmlPost);
            }


            /*
             * Since we're removing an event from the queue, we
             * need to check priority.  This resets the TIF_SPINNING
             * since we're no longer spinning.
             */
// We disable all MS badapp code and do it our way
            if (pti->TIF_flags & TIF_SPINNING)
                CheckProcessForeground(pti);

            DelQEntry(pmlPost, pqmsg);
        }

        /*
         * See if this is a dde message that needs to be fixed up.
         */
        if (CheckMsgFilter(lpMsg->message,
                (WM_DDE_FIRST + 1) | MSGFLAG_DDE_MID_THUNK,
                WM_DDE_LAST | MSGFLAG_DDE_MID_THUNK)) {
            /*
             * Fixup the message value.
             */
            lpMsg->message &= (UINT)~MSGFLAG_DDE_MID_THUNK;

            /*
             * Call back the client to allocate the dde data for this message.
             */
            xxxDDETrackGetMessageHook(lpMsg);

            /*
             * Copy these values back into the queue if this message hasn't
             * been removed from the queue. Need to search through the
             * queue again because the pqmsg may have been removed when
             * we left the critical section above.
             */
            if (!fRemoveMsg) {
                if (pqmsg == FindQMsg(pti, pmlPost, pwndFilter, msgMin, msgMax, FALSE)) {
                    pqmsg->msg = *lpMsg;
                }
            }
        }
#if DBG
        else if (CheckMsgFilter(lpMsg->message, WM_DDE_FIRST, WM_DDE_LAST)) {
            if (fRemoveMsg) {
                TraceDdeMsg(lpMsg->message, (HWND)lpMsg->wParam, lpMsg->hwnd, MSG_RECV);
            } else {
                TraceDdeMsg(lpMsg->message, (HWND)lpMsg->wParam, lpMsg->hwnd, MSG_PEEK);
            }
        }
#endif
    }

    /*
     * If there are no posted messages available, clear the post message
     * bit so we don't go looking for them again.
     */
    if (pmlPost->cMsgs == 0 && pti->cQuit == 0) {
        pti->pcti->fsWakeBits &= ~(QS_POSTMESSAGE | QS_ALLPOSTMESSAGE);
        pti->pcti->fsChangeBits &= ~QS_ALLPOSTMESSAGE;
    }

    return pqmsg != NULL;
}

#ifdef HUNGAPP_GHOSTING

/***************************************************************************\
* xxxProcessHungThreadEvent
*
* We check when a thread gets unhung when it reads the posted queue message.
*
* 6-10-99   vadimg      created
\***************************************************************************/

void xxxProcessHungThreadEvent(PWND pwnd)
{
    PTHREADINFO ptiCurrent = PtiCurrent();
    PWND pwndGhost;
    HWND hwnd;
    TL tlpwndT1, tlpwndT2;

    CheckLock(pwnd);

    /*
     * The app processed this queue message, so update time last read
     * used for hung app calculations.
     */
    SET_TIME_LAST_READ(ptiCurrent);

    pwndGhost = FindGhost(pwnd);

    ThreadLockAlwaysWithPti(ptiCurrent, pwndGhost, &tlpwndT1);
    ThreadLockAlwaysWithPti(ptiCurrent, pwnd, &tlpwndT2);

    if (pwndGhost != NULL) {
        PCHECKPOINT pcp, pcpGhost;

        /*
         * Try to set the state of the hung window to the current state of
         * the ghost window.
         */
        if (TestWF(pwndGhost, WFMAXIMIZED)) {
            xxxMinMaximize(pwnd, SW_MAXIMIZE, MINMAX_KEEPHIDDEN);
        } else if (TestWF(pwndGhost, WFMINIMIZED)) {
            xxxMinMaximize(pwnd, SW_SHOWMINNOACTIVE, MINMAX_KEEPHIDDEN);
        } else {
            DWORD dwFlags;
            PTHREADINFO pti = GETPTI(pwndGhost);

            /*
             * If the ghost is the active foreground window, allow this
             * activation to bring the hung window to the foreground.
             */
            if (pti->pq == gpqForeground && pti->pq->spwndActive == pwndGhost) {
                dwFlags = 0;
                GETPTI(pwnd)->TIF_flags |= TIF_ALLOWFOREGROUNDACTIVATE;
            } else {
                dwFlags = SWP_NOACTIVATE;
            }

            /*
             * This will appropriately zorder, activate, and position the
             * hung window.
             */
            xxxSetWindowPos(pwnd, pwndGhost,
                    pwndGhost->rcWindow.left, pwndGhost->rcWindow.top,
                    pwndGhost->rcWindow.right - pwndGhost->rcWindow.left,
                    pwndGhost->rcWindow.bottom - pwndGhost->rcWindow.top,
                    dwFlags);
        }

        /*
         * Since the ghost window could've been minimized or maximized during
         * its lifetime, copy over the positioning checkpoint.
         */
        if ((pcpGhost = (PCHECKPOINT)_GetProp(pwndGhost,
                PROP_CHECKPOINT, PROPF_INTERNAL)) != NULL) {

            if ((pcp = (PCHECKPOINT)_GetProp(pwnd,
                    PROP_CHECKPOINT, PROPF_INTERNAL)) == NULL) {
                pcp = CkptRestore(pwnd, &pwnd->rcWindow);
            }

            if (pcp != NULL) {
                RtlCopyMemory(pcp, pcpGhost, sizeof(CHECKPOINT));
            }
        }
    }

    /*
     * Toggle the visible bit of the hung window and remove the ghost window
     * corresponding to this previously hung window.
     */
    SetVisible(pwnd, SV_SET);
    RemoveGhost(pwnd);

    /*
     * Make the shell aware again of the hung window.
     */
    hwnd = PtoHq(pwnd);
    PostShellHookMessages(HSHELL_WINDOWCREATED, (LPARAM)hwnd);
    xxxCallHook(HSHELL_WINDOWCREATED, (WPARAM)hwnd, (LPARAM)0, WH_SHELL);

    /*
     * Completely invalidate the hung window, since it became visible again.
     */
    xxxRedrawWindow(pwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE |
            RDW_ALLCHILDREN | RDW_FRAME);

    ThreadUnlock(&tlpwndT2);
    ThreadUnlock(&tlpwndT1);
}

#else // HUNGAPP_GHOSTING

void xxxProcessHungThreadEvent(PWND pwnd)
{
    CheckLock(pwnd);

    if (TestWF(pwnd, WFVISIBLE)) {
        RIPMSG0(RIP_WARNING, "xxxProcessHungThreadEvent: window is already visible");
    } else {
        SetVisible(pwnd, SV_SET);

        if (TestWF(pwnd, WFMINIMIZED)) {
            RIPMSG0(RIP_WARNING, "xxxProcessHungThreadEvent: window is already minmized");
        } else {
            xxxMinMaximize(pwnd, SW_SHOWMINNOACTIVE, MINMAX_KEEPHIDDEN);
        }
    }
}

#endif // HUNGAPP_GHOSTING

BEEPPROC pfnBP[] = {
    UpSiren,
    DownSiren,
    LowBeep,
    HighBeep,
    KeyClick};

/***************************************************************************\
* xxxProcessEventMessage
*
* This handles our processing for 'event' messages.  We return a BOOL
* here telling the system whether or not to continue processing messages.
*
* History:
* 06-17-91 DavidPe      Created.
\***************************************************************************/

VOID xxxProcessEventMessage(
    PTHREADINFO ptiCurrent,
    PQMSG pqmsg)
{
    PWND pwnd;
    TL tlpwndT;
    TL tlMsg;
    PQ pq;

    UserAssert(IsWinEventNotifyDeferredOK());
    UserAssert(ptiCurrent == PtiCurrent());

    ThreadLockPoolCleanup(ptiCurrent, pqmsg, &tlMsg, CleanEventMessage);

    pq = ptiCurrent->pq;
    switch (pqmsg->dwQEvent) {
    case QEVENT_DESTROYWINDOW:
        /*
         * These events are posted from xxxDW_DestroyOwnedWindows
         * for owned windows that are not owned by the owner
         * window thread.
         */
        pwnd = RevalidateHwnd((HWND)pqmsg->msg.wParam);
        if (pwnd != NULL) {
            if (!TestWF(pwnd, WFCHILD))
                xxxDestroyWindow(pwnd);
            else {
                ThreadLockAlwaysWithPti(ptiCurrent, pwnd, &tlpwndT);
                xxxFreeWindow(pwnd, &tlpwndT);
            }
        }
        break;

    case QEVENT_SHOWWINDOW:
        /*
         * These events are mainly used from within CascadeChildWindows()
         * and TileChildWindows() so that taskmgr doesn't hang while calling
         * these apis if it is trying to tile or cascade a hung application.
         */
        /* The HIWORD of lParam now has the preserved state of gfAnimate at the
         * time of the call.
         */
        pwnd = RevalidateHwnd((HWND)pqmsg->msg.wParam);
        if (pwnd != NULL && !TestWF(pwnd, WFINDESTROY)) {
            ThreadLockAlwaysWithPti(ptiCurrent, pwnd, &tlpwndT);
            xxxShowWindow(pwnd, (DWORD)pqmsg->msg.lParam);
            /*
             * If this is coming from an async SetWindowPlacement, update the
             *  check point settings if the window is minimized.
             */
            if ((pqmsg->msg.message & WPF_ASYNCWINDOWPLACEMENT)
                    && TestWF(pwnd, WFMINIMIZED)) {

                WPUpdateCheckPointSettings(pwnd, (UINT)pqmsg->msg.message);
            }
            ThreadUnlock(&tlpwndT);
        }
        break;

    case QEVENT_NOTIFYWINEVENT:
        UserAssert(((PNOTIFY)pqmsg->msg.lParam)->dwWEFlags & WEF_POSTED);
        UserAssert(((PNOTIFY)pqmsg->msg.lParam)->dwWEFlags & WEF_ASYNC);
        xxxProcessNotifyWinEvent((PNOTIFY)pqmsg->msg.lParam);
        break;

    case QEVENT_SETWINDOWPOS:
        /*
         * QEVENT_SETWINDOWPOS events are generated when a thread calls
         * SetWindowPos with a list of windows owned by threads other than
         * itself.  This way all WINDOWPOSing on a window is done the thread
         * that owns (created) the window and we don't have any of those
         * nasty inter-thread synchronization problems.
         */
        xxxProcessSetWindowPosEvent((PSMWP)pqmsg->msg.wParam);
        break;

    case QEVENT_UPDATEKEYSTATE:
        /*
         * Update the local key state with the state from those
         * keys that have changed since the last time key state
         * was synchronized.
         */
        ProcessUpdateKeyStateEvent(pq, (PBYTE)pqmsg->msg.wParam, (PBYTE)pqmsg->msg.wParam + CBKEYSTATE);
        break;

    case QEVENT_ACTIVATE:
    {
        if (pqmsg->msg.lParam == 0) {

            /*
             * Clear any visible tracking going on in system.  We
             * only bother to do this if lParam == 0 since
             * xxxSetForegroundWindow2() deals with this in the
             * other case.
             */
            xxxCancelTracking();

            /*
             * Remove the clip cursor rectangle - it is a global mode that
             * gets removed when switching.  Also remove any LockWindowUpdate()
             * that's still around.
             */
            zzzClipCursor(NULL);
            LockWindowUpdate2(NULL, TRUE);

            /*
             * Reload pq because it may have changed.
             */
            pq = ptiCurrent->pq;

            /*
             * If this event didn't originate from an initializing app
             * coming to the foreground [wParam == 0] then go ahead
             * and check if there's already an active window and if so make
             * it visually active.  Also make sure we're still the foreground
             * queue.
             */
            if ((pqmsg->msg.wParam != 0) && (pq->spwndActive != NULL) &&
                    (pq == gpqForeground)) {
                PWND pwndActive;

                ThreadLockAlwaysWithPti(ptiCurrent, pwndActive = pq->spwndActive, &tlpwndT);
                xxxSendMessage(pwndActive, WM_NCACTIVATE, TRUE, 0);
                xxxUpdateTray(pwndActive);
                xxxSetWindowPos(pwndActive, PWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
                ThreadUnlock(&tlpwndT);
            } else if (pq != gpqForeground) {

                /*
                 * If we're not being activated, make sure we don't become foreground.
                 */
                ptiCurrent->TIF_flags &= ~TIF_ALLOWFOREGROUNDACTIVATE;
                TAGMSG1(DBGTAG_FOREGROUND, "xxxProcessEventMessage clear TIF %#p", ptiCurrent);
                ptiCurrent->ppi->W32PF_Flags &= ~W32PF_ALLOWFOREGROUNDACTIVATE;
                TAGMSG1(DBGTAG_FOREGROUND, "xxxProcessEventMessage clear W32PF %#p", ptiCurrent->ppi);
            }

        } else {

            pwnd = RevalidateHwnd((HWND)pqmsg->msg.lParam);
            if (pwnd == NULL)
                break;

            ThreadLockAlwaysWithPti(ptiCurrent, pwnd, &tlpwndT);

            /*
             * If nobody is foreground, allow this app to become foreground.
             */
            if (gpqForeground == NULL) {
                xxxSetForegroundWindow2(pwnd, ptiCurrent, 0);
            } else {
                if (pwnd != pq->spwndActive) {
                    if (xxxActivateThisWindow(pwnd, (UINT)pqmsg->msg.wParam,
                            (ATW_SETFOCUS | ATW_ASYNC) |
                            ((pqmsg->msg.message & PEM_ACTIVATE_NOZORDER) ? ATW_NOZORDER : 0))) {

                        /*
                         * This event was posted by SetForegroundWindow2
                         * (i.e. pqmsg->msg.lParam != 0) so make sure
                         * mouse is on this window.
                         */
                        if (TestUP(ACTIVEWINDOWTRACKING)) {
                            zzzActiveCursorTracking(pwnd);
                        }
                    }
                } else {
                    BOOL fActive = (GETPTI(pwnd)->pq == gpqForeground);

                    xxxSendMessage(pwnd, WM_NCACTIVATE,
                            (DWORD)(fActive), 0);
                    if (fActive) {
                        xxxUpdateTray(pwnd);
                    }

                    /*
                     * Only bring the window to the top if it is becoming active.
                     */
                    if (fActive && !(pqmsg->msg.message & PEM_ACTIVATE_NOZORDER))
                        xxxSetWindowPos(pwnd, PWND_TOP, 0, 0, 0, 0,
                                SWP_NOSIZE | SWP_NOMOVE);
                }
            }

            /*
             * Check here to see if the window needs to be restored. This is a
             * hack so that we're compatible with what msmail expects out of
             * win3.1 alt-tab. msmail expects to always be active when it gets
             * asked to be restored. This will ensure that during alt-tab
             * activate.
             */
            if (pqmsg->msg.message & PEM_ACTIVATE_RESTORE) {
                if (TestWF(pwnd, WFMINIMIZED)) {
                    _PostMessage(pwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
                }
            }

            ThreadUnlock(&tlpwndT);
        }

    }
        break;

    case QEVENT_DEACTIVATE:
        xxxDeactivate(ptiCurrent, (DWORD)pqmsg->msg.wParam);
        break;

    case QEVENT_CANCELMODE:
        if (pq->spwndCapture != NULL) {
            ThreadLockAlwaysWithPti(ptiCurrent, pq->spwndCapture, &tlpwndT);
            xxxSendMessage(pq->spwndCapture, WM_CANCELMODE, 0, 0);
            ThreadUnlock(&tlpwndT);

            /*
             * Set QS_MOUSEMOVE so any sleeping modal loops,
             * like the move/size code, will wake up and figure
             * out that it should abort.
             */
            SetWakeBit(ptiCurrent, QS_MOUSEMOVE);
        }
        break;


    case QEVENT_POSTMESSAGE:
        /*
         * This event is used in situations where we need to ensure that posted
         * messages are processed after previous QEVENT's.  Normally, posting a
         * queue event and then calling postmessage will result in the posted
         * message being seen first by the app (because posted messages are
         * processed before input.) Instead we will post a QEVENT_POSTMESSAGE
         * instead of doing a postmessage directly, which will result in the
         * correct ordering of messages.
         *
         */

        if (pwnd = RevalidateHwnd((HWND)pqmsg->msg.hwnd)) {

            _PostMessage(pwnd,pqmsg->msg.message,pqmsg->msg.wParam,pqmsg->msg.lParam);
        }
        break;


    case QEVENT_ASYNCSENDMSG:
        xxxProcessAsyncSendMessage((PASYNCSENDMSG)pqmsg->msg.wParam);
        break;

    case QEVENT_HUNGTHREAD:
        pwnd = RevalidateHwnd((HWND)pqmsg->msg.hwnd);
        if (pwnd != NULL) {
            ThreadLockAlwaysWithPti(ptiCurrent, pwnd, &tlpwndT);
            xxxProcessHungThreadEvent(pwnd);
            ThreadUnlock(&tlpwndT);
        }
        break;

    case QEVENT_CANCELMOUSEMOVETRK: {
        /*
         * hwnd: hwndTrack. message: dwDTFlags.
         * wParam: htEx. lParam: dwDTCancel
         */
        PDESKTOP pdesk = ptiCurrent->rpdesk;
        pwnd = RevalidateHwnd((HWND)pqmsg->msg.hwnd);
        /*
         * Let's check that the app didn't manage to restart mouse leave
         *  tracking before we had a chance to cancel it.
         */
        UserAssert(!(pqmsg->msg.message & DF_TRACKMOUSELEAVE)
                    || !(pdesk->dwDTFlags & DF_TRACKMOUSELEAVE)
                    || (PtoHq(pdesk->spwndTrack) != pqmsg->msg.hwnd)
                    || !((pdesk->htEx == HTCLIENT) ^ ((int)pqmsg->msg.wParam == HTCLIENT)));
        /*
         * If we're back tracking at the same spot, bail
         */
        if ((pdesk->dwDTFlags & DF_MOUSEMOVETRK)
                && (PtoHq(pdesk->spwndTrack) == pqmsg->msg.hwnd)
                && (pdesk->htEx == (int)pqmsg->msg.wParam)) {
            /*
             * If we're tracking mouse leave,
             */
            break;
        }
        /*
         * Don't nuke the tooltip if it has been reactivated.
         */
        if (pdesk->dwDTFlags & DF_TOOLTIPACTIVE) {
            pqmsg->msg.lParam &= ~DF_TOOLTIP;
        }
        /*
         * Cancel tracking if the window is still around
         */
        if (pwnd != NULL) {
            ThreadLockAlwaysWithPti(ptiCurrent, pwnd, &tlpwndT);
            xxxCancelMouseMoveTracking(pqmsg->msg.message, pwnd,
                                   (int)pqmsg->msg.wParam,
                                   (DWORD)pqmsg->msg.lParam);
            ThreadUnlock(&tlpwndT);
        } else if ((pqmsg->msg.lParam & DF_TOOLTIP)
                && (pqmsg->msg.message & DF_TOOLTIPSHOWING)) {
            /*
             * The window is gone and so must be tracking.
             * Just take care of the tooltip which is still showing.
             */
            pwnd = pdesk->spwndTooltip;
            ThreadLockAlwaysWithPti(ptiCurrent, pwnd, &tlpwndT);
            xxxResetTooltip((PTOOLTIPWND)pwnd);
            ThreadUnlock(&tlpwndT);
        }
    }
    break;

    case QEVENT_RITACCESSIBILITY:
        if (IsHooked(ptiCurrent, WHF_SHELL)) {
            xxxCallHook((UINT)pqmsg->msg.wParam,
                        (WPARAM)pqmsg->msg.lParam,
                        (LPARAM)0,
                        WH_SHELL);
        }

        PostShellHookMessages((UINT)pqmsg->msg.wParam, pqmsg->msg.lParam);
        break;

    case QEVENT_RITSOUND:
        /*
         * This should only happen on the desktop thread.
         */
        switch(pqmsg->msg.message) {
        case RITSOUND_UPSIREN:
        case RITSOUND_DOWNSIREN:
        case RITSOUND_LOWBEEP:
        case RITSOUND_HIGHBEEP:
        case RITSOUND_KEYCLICK:
            (pfnBP[pqmsg->msg.message])();
            break;

        case RITSOUND_DOBEEP:
            switch(pqmsg->msg.wParam) {
            case RITSOUND_UPSIREN:
            case RITSOUND_DOWNSIREN:
            case RITSOUND_LOWBEEP:
            case RITSOUND_HIGHBEEP:
            case RITSOUND_KEYCLICK:
                DoBeep(pfnBP[pqmsg->msg.wParam], (DWORD)pqmsg->msg.lParam);
            }
            break;
        }
        break;

    case QEVENT_APPCOMMAND: {
        /*
         * qevent app commands so we can post a wm_appcommand to the queue
         */
        THREADINFO  *ptiWindowOwner;
        int         cmd;
        UINT        keystate;

        /*
         * check the appcommand's are within reasonable ranges
         * if they aren't then we have an internal consistency error since xxxKeyEvent should
         * have generated correct ones for us
         */
        UserAssert( pqmsg->msg.lParam >= VK_APPCOMMAND_FIRST &&
                    pqmsg->msg.lParam <= VK_APPCOMMAND_LAST );

        /*
         * We need to work out which window to send to here. Using the same
         * rules as from xxxScanSysQueue:
         * Assign the input to the focus window. If there is no focus
         * window, assign it to the active window as a SYS message.
         */
        pwnd = ptiCurrent->pq->spwndFocus;
        if (!pwnd) {
            pwnd = ptiCurrent->pq->spwndActive;
            if (!pwnd ) {
                /*
                 * At the moment we will just eat the message since we can't find a foreground q
                 * This follows the method that any other app (eg hidserv) would mimic to
                 * find the window to send to.
                 */
                break;
            }
        }

        /*
         * We don't want to block on another thread since the xxxSendMessage is a synchronous call
         * so we post the message to the queue of the window owner thread
         */
        ptiWindowOwner = GETPTI(pwnd);
        if (ptiCurrent != ptiWindowOwner) {
            /*
             * Post the event message to the window who should get it
             */
            PostEventMessage(ptiWindowOwner, ptiWindowOwner->pq, QEVENT_APPCOMMAND,
                             NULL, 0, (WPARAM)0, pqmsg->msg.lParam);

            /*
             * break out of this since we've now posted the message to a different q - we
             * don't want to deal with it here
             */
            break;
        }

        cmd = APPCOMMAND_FIRST + ((UINT)pqmsg->msg.lParam - VK_APPCOMMAND_FIRST);
        keystate = GetMouseKeyFlags(ptiWindowOwner->pq);
        pqmsg->msg.lParam = MAKELPARAM(keystate, cmd);


        /*
         * Generate a WM_APPCOMMAND message from the keyboard keys
         */
        ThreadLockAlwaysWithPti(ptiCurrent, pwnd, &tlpwndT);
        xxxSendMessage(pwnd, WM_APPCOMMAND, (WPARAM)pwnd, pqmsg->msg.lParam);
        ThreadUnlock(&tlpwndT);

        break;
    }
    default:
        RIPMSG1(RIP_ERROR, "xxxProcessEventMessage Bad pqmsg->dwQEvent:%#lx", pqmsg->dwQEvent);
        break;
    }

    ThreadUnlockPoolCleanup(ptiCurrent, &tlMsg);
}

/***************************************************************************\
* _GetInputState (API)
*
* Returns the current input state for mouse buttons or keys.
*
* History:
* 11-06-90 DavidPe      Created.
\***************************************************************************/

#define QS_TEST_AND_CLEAR (QS_INPUT | QS_POSTMESSAGE | QS_TIMER | QS_PAINT | QS_SENDMESSAGE)
#define QS_TEST           (QS_MOUSEBUTTON | QS_KEY)

BOOL _GetInputState(VOID)
{
    if (LOWORD(_GetQueueStatus(QS_TEST_AND_CLEAR)) & QS_TEST) {
        return TRUE;
    } else {
        return FALSE;
    }
}

#undef QS_TEST_AND_CLEAR
#undef QS_TEST

/***************************************************************************\
* _GetQueueStatus (API)
*
* Returns the changebits in the lo-word and wakebits in
* the hi-word for the current queue.
*
* History:
* 12-17-90 DavidPe      Created.
\***************************************************************************/

DWORD _GetQueueStatus(
    UINT flags)
{
    PTHREADINFO ptiCurrent;
    UINT fsChangeBits;

    ptiCurrent = PtiCurrentShared();

    flags &= (QS_ALLINPUT | QS_ALLPOSTMESSAGE | QS_TRANSFER);

    fsChangeBits = ptiCurrent->pcti->fsChangeBits;

    /*
     * Clear out the change bits the app is looking at
     * so it'll know what changed since it's last call
     * to GetQueueStatus().
     */
    ptiCurrent->pcti->fsChangeBits &= ~flags;

    /*
     * Return the current change/wake-bits.
     */
    return MAKELONG(fsChangeBits & flags,
            (ptiCurrent->pcti->fsWakeBits | ptiCurrent->pcti->fsWakeBitsJournal) & flags);
}

/***************************************************************************\
* xxxMsgWaitForMultipleObjects (API)
*
* Blocks until an 'event' satisifying dwWakeMask occurs for the
* current thread as well as all other objects specified by the other
* parameters which are the same as the base call WaitForMultipleObjects().
*
* pfnNonMsg indicates that pHandles is big enough for nCount+1 handles
*     (empty slot at end, and to call pfnNonMsg for non message events.
*
* History:
* 12-17-90 DavidPe      Created.
\***************************************************************************/
#ifdef LOCK_MOUSE_CODE
#pragma alloc_text(MOUSE, xxxMsgWaitForMultipleObjects)
#endif

DWORD xxxMsgWaitForMultipleObjects(
    DWORD nCount,
    PVOID *apObjects,
    MSGWAITCALLBACK pfnNonMsg,
    PKWAIT_BLOCK WaitBlockArray)
{
    PTHREADINFO ptiCurrent = PtiCurrent();
    NTSTATUS Status;

    ptiCurrent = PtiCurrent();
    UserAssert(IsWinEventNotifyDeferredOK());

    /*
     * Setup the wake mask for this thread. Wait for QS_EVENT or the app won't
     * get event messages like deactivate.
     */
    ClearQueueServerEvent(QS_ALLINPUT | QS_EVENT);

    /*
     * Stuff the event handle for the current queue at the end.
     */
    apObjects[nCount] = ptiCurrent->pEventQueueServer;

    /*
     * Check to see if any input came inbetween when we
     * last checked and the NtClearEvent() call.
     */
    if (!(ptiCurrent->pcti->fsChangeBits & QS_ALLINPUT)) {

        /*
         * This app is going idle. Clear the spin count check to see
         * if we need to make this process foreground again.
         */
        if (ptiCurrent->TIF_flags & TIF_SPINNING) {
            CheckProcessForeground(ptiCurrent);
        }
        ptiCurrent->pClientInfo->cSpins = 0;

        if (ptiCurrent == gptiForeground &&
                IsHooked(ptiCurrent, WHF_FOREGROUNDIDLE)) {
            xxxCallHook(HC_ACTION, 0, 0, WH_FOREGROUNDIDLE);
        }

        CheckForClientDeath();

        /*
         * Set the input idle event to wake up any threads waiting
         * for this thread to go into idle state.
         */
        zzzWakeInputIdle(ptiCurrent);

Again:
        LeaveCrit();

        Status = KeWaitForMultipleObjects(nCount + 1, apObjects,
                WaitAny, WrUserRequest,
                UserMode, FALSE,
                NULL, WaitBlockArray);

        EnterCrit();

        CheckForClientDeath();

        UserAssert(NT_SUCCESS(Status));


        if ((Status == STATUS_WAIT_0) && (pfnNonMsg != NULL)) {
            /*
             * Call pfnNonMsg for the first event
             */
            pfnNonMsg(DEVICE_TYPE_MOUSE);

            /*
             * Setup again the wake mask for this thread.
             * Wait for QS_EVENT or the app won't
             * get event messages like deactivate.
             */
            ptiCurrent->pcti->fsWakeMask = QS_ALLINPUT | QS_EVENT;
            goto Again;
        }

        if (Status == (NTSTATUS)(STATUS_WAIT_0 + nCount)) {

            /*
             * Reset the input idle event to block and threads waiting
             * for this thread to go into idle state.
             */
            SleepInputIdle(ptiCurrent);
        }
    } else {
        Status = nCount;
    }

    /*
     * Clear fsWakeMask since we're no longer waiting on the queue.
     */
    ptiCurrent->pcti->fsWakeMask = 0;

    return (DWORD)Status;
}

/***************************************************************************\
* xxxSleepThread
*
* Blocks until an 'event' satisifying fsWakeMask occurs for the
* current thread.
*
* History:
* 10-28-90 DavidPe      Created.
\***************************************************************************/

BOOL xxxSleepThread(
    UINT fsWakeMask,
    DWORD Timeout,
    BOOL fInputIdle)
{
    PTHREADINFO ptiCurrent;
    LARGE_INTEGER li, *pli;
    NTSTATUS status = STATUS_SUCCESS;
    BOOL fExclusive = fsWakeMask & QS_EXCLUSIVE;
    WORD fsWakeMaskSaved;

    UserAssert(IsWinEventNotifyDeferredOK());

    if (fExclusive) {
        /*
         * the exclusive bit is a 'dummy' arg, turn it off to
         * avoid any possible conflictions
         */
        fsWakeMask = fsWakeMask & ~QS_EXCLUSIVE;
    }

    if (Timeout) {
        /*
         * Convert dwMilliseconds to a relative-time(i.e.  negative)
         * LARGE_INTEGER.  NT Base calls take time values in 100 nanosecond
         * units.
         */
        li.QuadPart = Int32x32To64(-10000, Timeout);
        pli = &li;
    } else
        pli = NULL;

    CheckCritIn();

    ptiCurrent = PtiCurrent();

    fsWakeMaskSaved = ptiCurrent->pcti->fsWakeMask;

    while (TRUE) {

        /*
         * First check if the input has arrived.
         */
        if (ptiCurrent->pcti->fsChangeBits & fsWakeMask) {
            /*
             * Restore the wake mask to what it was before we went to sleep
             * to allow possible callbacks before KeWait... but after the mask
             * has been set and also APCs from KeWait... to still be able to
             * wake up. Simply clearing the mask here if we're in such a
             * callback or in an APC means that the thread will never wake up.
             */
            ptiCurrent->pcti->fsWakeMask = fsWakeMaskSaved;

            /*
             * Update timeLastRead - it is used for hung app calculations.
             * If the thread is waking up to process input, it isn't hung!
             */
            SET_TIME_LAST_READ(ptiCurrent);
            return TRUE;
        }

        /*
         * Next check for SendMessages
         */
        if (!fExclusive && ptiCurrent->pcti->fsWakeBits & QS_SENDMESSAGE) {
            xxxReceiveMessages(ptiCurrent);

            /*
             * Restore the change bits we took out in PeekMessage()
             */
            ptiCurrent->pcti->fsChangeBits |= (ptiCurrent->pcti->fsWakeBits & ptiCurrent->fsChangeBitsRemoved);
            ptiCurrent->fsChangeBitsRemoved = 0;
        }

        /*
         * Check to see if some resources need expunging.
         * This will unload Hook DLLs, including WinEvent ones
         */
        if (ptiCurrent->ppi->cSysExpunge != gcSysExpunge) {
            ptiCurrent->ppi->cSysExpunge = gcSysExpunge;
            if (ptiCurrent->ppi->dwhmodLibLoadedMask & gdwSysExpungeMask)
                xxxDoSysExpunge(ptiCurrent);
        }

        /*
         * OR QS_SENDMESSAGE in since ReceiveMessage() will end up
         * trashing pq->fsWakeMask.  Do the same for QS_SYSEXPUNGE.
         */
        ClearQueueServerEvent((WORD)(fsWakeMask | (fExclusive ? 0 : QS_SENDMESSAGE)));

        /*
         * If we have timed out then return our error to the caller.
         */
        if (status == STATUS_TIMEOUT) {
            RIPERR1(ERROR_TIMEOUT, RIP_VERBOSE, "SleepThread: The timeout has expired %lX", Timeout);
            return FALSE;
        }

        /*
         * Because we do a non-alertable wait, we know that a status
         * of STATUS_USER_APC means that the thread was terminated.
         * If we have terminated, get back to user mode.
         */
        if (status == STATUS_USER_APC) {
            ClientDeliverUserApc();
            return FALSE;
        }

        /*
         * If this is the power state callout thread, we might need to bail
         * out early.
         */
        if (gPowerState.pEvent == ptiCurrent->pEventQueueServer) {
            if (gPowerState.fCritical) {
                return FALSE;
            }
        }

        UserAssert(status == STATUS_SUCCESS);
        /*
         * Check to see if any input came inbetween when we
         * last checked and the NtClearEvent() call.
         *
         * We call NtWaitForSingleObject() rather than
         * WaitForSingleObject() so we can set fAlertable
         * to TRUE and thus allow timer APCs to be processed.
         */
        if (!(ptiCurrent->pcti->fsChangeBits & ptiCurrent->pcti->fsWakeMask)) {
            /*
             * This app is going idle. Clear the spin count check to see
             * if we need to make this process foreground again.
             */
            if (fInputIdle) {
                if (ptiCurrent->TIF_flags & TIF_SPINNING) {
                    CheckProcessForeground(ptiCurrent);
                }
                ptiCurrent->pClientInfo->cSpins = 0;
            }


            if (!(ptiCurrent->TIF_flags & TIF_16BIT))  {
                if (fInputIdle && ptiCurrent == gptiForeground &&
                        IsHooked(ptiCurrent, WHF_FOREGROUNDIDLE)) {
                    xxxCallHook(HC_ACTION, 0, 0, WH_FOREGROUNDIDLE);
                }

                CheckForClientDeath();

                /*
                 * Set the input idle event to wake up any threads waiting
                 * for this thread to go into idle state.
                 */
                if (fInputIdle)
                    zzzWakeInputIdle(ptiCurrent);

                xxxSleepTask(fInputIdle, NULL);

                LeaveCrit();
                status = KeWaitForSingleObject(ptiCurrent->pEventQueueServer,
                        WrUserRequest, UserMode, FALSE, pli);
                CheckForClientDeath();
                EnterCrit();

                /*
                 * Reset the input idle event to block and threads waiting
                 * for this thread to go into idle state.
                 */
                SleepInputIdle(ptiCurrent);

                /*
                 *  ptiCurrent is 16bit!
                 */
            } else {
                if (fInputIdle)
                    zzzWakeInputIdle(ptiCurrent);

                xxxSleepTask(fInputIdle, NULL);
            }
        }
    }
}


/***************************************************************************\
* SetWakeBit
*
* Adds the specified wake bit to specified THREADINFO and wakes its
* thread up if the bit is in its fsWakeMask.
*
* Nothing will happen in the system unless we come to this function.
*  so be fast and small.
*
* History:
* 10-28-90 DavidPe      Created.
\***************************************************************************/

VOID SetWakeBit(
    PTHREADINFO pti,
    UINT wWakeBit)
{
    CheckCritIn();

    UserAssert(pti);

    /*
     * Win3.1 changes ptiKeyboard and ptiMouse accordingly if we're setting
     * those bits.
     */
    if (wWakeBit & QS_MOUSE)
        pti->pq->ptiMouse = pti;

    if (wWakeBit & QS_KEY)
        pti->pq->ptiKeyboard = pti;

    /*
     * OR in these bits - these bits represent what input this app has
     * (fsWakeBits), or what input has arrived since that last look
     * (fsChangeBits).
     */
    pti->pcti->fsWakeBits |= wWakeBit;
    pti->pcti->fsChangeBits |= wWakeBit;

    /*
     * Before waking, do screen saver check to see if it should
     * go away.
     */
    if ((wWakeBit & QS_INPUT)
            && (pti->ppi->W32PF_Flags & W32PF_IDLESCREENSAVER)) {
        if ((wWakeBit & QS_MOUSEMOVE)
            && (gpsi->ptCursor.x == gptSSCursor.x)
            && (gpsi->ptCursor.y == gptSSCursor.y)) {
            goto SkipScreenSaverStuff;
        }

        /*
         * Our idle screen saver needs to be given a priority boost so that it
         * can process input.
         */
        pti->ppi->W32PF_Flags &= ~W32PF_IDLESCREENSAVER;
        SetForegroundPriority(pti, TRUE);
    }

SkipScreenSaverStuff:
    if (wWakeBit & pti->pcti->fsWakeMask) {
        /*
         * Wake the Thread
         */
        if (pti->TIF_flags & TIF_16BIT) {
            pti->ptdb->nEvents++;
            gpsi->nEvents++;
            WakeWowTask(pti);
        } else {
            KeSetEvent(pti->pEventQueueServer, 2, FALSE);
        }
    }
}

/***************************************************************************\
* TransferWakeBit
*
* We have a mesasge from the system queue. If out input bit for this
* message isn't set, set ours and clear the guy whose bit was set
* because of this message.
*
* 10-22-92 ScottLu      Created.
\***************************************************************************/

void TransferWakeBit(
    PTHREADINFO pti,
    UINT message)
{
    PTHREADINFO ptiT;
    UINT fsMask;

    /*
     * Calculate the mask from the message range. Only interested
     * in hardware input here: mouse and keys.
     */
    fsMask = CalcWakeMask(message, message, 0) & (QS_MOUSE | QS_KEY);

    /*
     * If it is set in this thread's wakebits, nothing to do.
     * Otherwise transfer them from the owner to this thread.
     */
    if (!(pti->pcti->fsWakeBits & fsMask)) {
        /*
         * Either mouse or key is set (not both). Remove this bit
         * from the thread that currently owns it, and change mouse /
         * key ownership to this thread.
         */
        if (fsMask & QS_KEY) {
            ptiT = pti->pq->ptiKeyboard;
            pti->pq->ptiKeyboard = pti;
        } else {
            ptiT = pti->pq->ptiMouse;
            pti->pq->ptiMouse = pti;
        }
        ptiT->pcti->fsWakeBits &= ~fsMask;

        /*
         * Transfer them to this thread (certainly this may be the
         * same thread for win32 threads not sharing queues).
         */
        pti->pcti->fsWakeBits |= fsMask;
        pti->pcti->fsChangeBits |= fsMask;
    }
}

/***************************************************************************\
* ClearWakeBit
*
* Clears wake bits. If fSysCheck is TRUE, this clears the input bits only
* if no messages are in the input queue. Otherwise, it clears input bits
* unconditionally.
*
* 11-05-92 ScottLu      Created.
\***************************************************************************/

VOID ClearWakeBit(
    PTHREADINFO pti,
    UINT wWakeBit,
    BOOL fSysCheck)
{

    /*
     * If fSysCheck is TRUE, clear bits only if we are not doing journal
     * playback and there are no more messages in the queue. fSysCheck
     * is TRUE if clearing because of no more input.  FALSE if just
     * transfering input ownership from one thread to another.
     */
    if (fSysCheck) {
        if (pti->pq->mlInput.cMsgs != 0 || FJOURNALPLAYBACK())
            return;
        if (pti->pq->QF_flags & QF_MOUSEMOVED)
            wWakeBit &= ~QS_MOUSEMOVE;
    }

    /*
     * Only clear the wake bits, not the change bits as well!
     */
    pti->pcti->fsWakeBits &= ~wWakeBit;
}



/***************************************************************************\
* PtiFromThreadId
*
* Returns the THREADINFO for the specified thread or NULL if thread
* doesn't exist or doesn't have a THREADINFO.
*
* History:
* 01-30-91  DavidPe     Created.
\***************************************************************************/

PTHREADINFO PtiFromThreadId(
    DWORD dwThreadId)
{
    PETHREAD pEThread;
    PTHREADINFO pti;

    /*
     * BUG BUG: Pretty much every place else we do call LockThreadByClientId
     * while outside the user critical section so we don't cause any
     * potential deadlock in the kernel.
     * It's too late to change it now. 2/17/99
     */

    if (!NT_SUCCESS(LockThreadByClientId((HANDLE)LongToHandle( dwThreadId ), &pEThread)))
        return NULL;

    /*
     * If the thread is not terminating, look up the pti.  This is
     * needed because the value returned by PtiFromThread() is
     * undefined if the thread is terminating.  See PspExitThread in
     * ntos\ps\psdelete.c.
     */
    if (!PsIsThreadTerminating(pEThread)) {
        pti = PtiFromThread(pEThread);
    } else {
        pti = NULL;
    }

    /*
     * Do a sanity check on the pti to make sure it's really valid.
     */
    if (pti != NULL) {
        try {
            if (pti->pEThread->Cid.UniqueThread != (HANDLE)LongToHandle( dwThreadId )) {
                pti = NULL;
            } else if (!(pti->TIF_flags & TIF_GUITHREADINITIALIZED)) {
                RIPMSG1(RIP_WARNING, "PtiFromThreadId: pti %#p not initialized", pti);
                pti = NULL;
            } else if (pti->TIF_flags & TIF_INCLEANUP) {
                RIPMSG1(RIP_WARNING, "PtiFromThreadId: pti %#p in cleanup", pti);
                pti = NULL;
            }
        } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
            pti = NULL;
        }
    }

    UnlockThread(pEThread);

    return pti;
}


/***************************************************************************\
* StoreMessage
*
*
*
* History:
* 10-31-90 DarrinM      Ported from Win 3.0 sources.
\***************************************************************************/

void StoreMessage(
    LPMSG pmsg,
    PWND pwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    DWORD time)
{
    CheckCritIn();

    pmsg->hwnd = HW(pwnd);
    pmsg->message = message;
    pmsg->wParam = wParam;
    pmsg->lParam = lParam;
    pmsg->time = (time != 0 ? time : NtGetTickCount());

    pmsg->pt = gpsi->ptCursor;
}


/***************************************************************************\
* StoreQMessage
*
* If 'time' is 0 grab the current time, if not, it means that this message
* is for an input event and eventually the mouse/keyboard hooks want to see
* the right time stamps.
*
* History:
* 02-27-91 DavidPe      Created.
* 06-15-96 CLupu        Add 'time' parameter
\***************************************************************************/

void StoreQMessage(
    PQMSG pqmsg,
    PWND  pwnd,
    UINT  message,
    WPARAM wParam,
    LPARAM lParam,
    DWORD time,
    DWORD dwQEvent,
    ULONG_PTR dwExtraInfo)
{
    CheckCritIn();

    pqmsg->msg.hwnd    = HW(pwnd);
    pqmsg->msg.message = message;
    pqmsg->msg.wParam  = wParam;
    pqmsg->msg.lParam  = lParam;
    pqmsg->msg.time    = (time == 0) ? NtGetTickCount() : time;

#ifdef REDIRECTION
    if (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST) {
        pqmsg->msg.pt.x = LOWORD(lParam);
        pqmsg->msg.pt.y = HIWORD(lParam);
    } else {
        pqmsg->msg.pt = gpsi->ptCursor;
    }
#else
    pqmsg->msg.pt      = gpsi->ptCursor;
#endif
    pqmsg->dwQEvent    = dwQEvent;
    pqmsg->ExtraInfo   = dwExtraInfo;
}


/***************************************************************************\
* xxxInitProcessInfo
*
* This initializes the process info. Usually gets created before the
* CreateProcess() call returns (so we can synchronize with the starting
* process in several different ways).
*
* 09-18-91 ScottLu      Created.
\***************************************************************************/

NTSTATUS xxxInitProcessInfo(
    PW32PROCESS pwp)
{
    PPROCESSINFO ppi = (PPROCESSINFO)pwp;
    NTSTATUS Status;

    CheckCritIn();

    /*
     * Check if we need to initialize the process.
     */
    if (pwp->W32PF_Flags & W32PF_PROCESSCONNECTED) {
        return STATUS_ALREADY_WIN32;
    }
    pwp->W32PF_Flags |= W32PF_PROCESSCONNECTED;

#if defined(_WIN64)
    /* Tag as emulated 32bit.  Flag is copied to be consistent with
     * the way WOW16 apps are tagged for win32k.
     */
    if (pwp->Process->Wow64Process) {
        pwp->W32PF_Flags |= W32PF_WOW64;
    }
#endif

    /*
     * Mark this app as "starting" - it will be starting until its first
     * window activates.
     */
    UserVerify(xxxSetProcessInitState(pwp->Process, STARTF_FORCEOFFFEEDBACK));

    /*
     * link it into the starting processes list
     */
    SetAppStarting(ppi);
    /*
     * link it into the global processes list
     */
    ppi->ppiNextRunning = gppiList;
    gppiList = ppi;
    /*
     * If foreground activation has not been canceled and the parent process
     *  (or an ancestor) can force a foreground change, then allow this process
     *  to come to the foreground when it does its first activation.
     *
     * Bug 273518 - joejo
     *
     * This will allow console windows to set foreground correctly on new
     * process' it launches, as opposed it just forcing foreground.
     */
    if (TEST_PUDF(PUDF_ALLOWFOREGROUNDACTIVATE) && CheckAllowForeground(pwp->Process)) {
        ppi->W32PF_Flags |= W32PF_ALLOWFOREGROUNDACTIVATE;
    }
    TAGMSG2(DBGTAG_FOREGROUND, "xxxInitProcessInfo %s W32PF %#p",
            ((ppi->W32PF_Flags & W32PF_ALLOWFOREGROUNDACTIVATE) ? "set" : "NOT"),
            ppi);

    /*
     * Get the logon session id.  This is used to determine which
     * windowstation to connect to and to identify attempts to
     * call hooks across security contexts.
     */
    Status = GetProcessLuid(NULL, &ppi->luidSession);
    UserAssert(NT_SUCCESS(Status));

    /*
     * Ensure that we're in sync with the expunge count
     */
    ppi->cSysExpunge = gcSysExpunge;

    /*
     * Don't perform any LPK callbacks until GDI notifies
     * us that the LPK(s) are loaded and initialized.
     */
    ppi->dwLpkEntryPoints = 0;

    return STATUS_SUCCESS;
}

/***************************************************************************\
* DestroyProcessInfo
*
* This function is executed when the last thread of a process goes
*  away.
*
* SO VERY IMPORTANT:
*  Note that the last thread of the process might not be a w32 thread.
*  So do not make any calls here that assume a w32 pti. Do avoid
*   any function calling PtiCurrent() as it probably assumes it is
*   on a nice w32 thread.
*  Also note that if the process is locked, the ppi is not going away; this
*   simply means that execution on this process has ended. So make sure to clean
*   up in a way that the ppi data is still valid (for example, if you free a pointer,
*   set it to NULL).
*
* zzz Note: Not a zzz routine although it calls zzzCalcStartCursorHide() -
*           Since we can't make callbacks on a non-GUI thread, we use
*           DeferWinEventNotify() & EndDeferWinEventNotifyWithoutProcessing()
*           to prevent callbacks.
*
* 04/08/96 GerardoB     Added header
\***************************************************************************/
BOOL DestroyProcessInfo(
    PW32PROCESS pwp)
{
    PPROCESSINFO ppi = (PPROCESSINFO)pwp;
    PDESKTOPVIEW pdv, pdvNext;
    BOOL  fHadThreads;
    PPUBOBJ ppo;

    CheckCritIn();

    /*
     * Free up input idle event if it exists - wake everyone waiting on it
     * first.  This object will get created sometimes even for non-windows
     * processes (usually for WinExec(), which calls WaitForInputIdle()).
     */
    CLOSE_PSEUDO_EVENT(&pwp->InputIdleEvent);

    /*
     * Check to see if the startglass is on, and if so turn it off and update.
     * DeferWinEventNotify to because we cannot process notifications for this
     * thread now (we may have no PtiCurrent, see comment above)
     */
    BEGINATOMICCHECK();
    DeferWinEventNotify();
    if (pwp->W32PF_Flags & W32PF_STARTGLASS) {
        pwp->W32PF_Flags &= ~W32PF_STARTGLASS;
        zzzCalcStartCursorHide(NULL, 0);
    }
    /*
     * This is bookkeeping - restore original notification deferral but without
     * attempting to process any deferred notifications because we have no pti.
     */
    EndDeferWinEventNotifyWithoutProcessing();
    ENDATOMICCHECK();

    /*
     * If the process never called Win32k, we're done.
     */
    if (!(pwp->W32PF_Flags & W32PF_PROCESSCONNECTED)) {
        return FALSE;
    }

    /*
     * Play the Process Close sound for non-console processes
     * running on the I/O windowstation.
     */

    if ((ppi->W32PF_Flags & W32PF_IOWINSTA) &&
        !(ppi->W32PF_Flags & W32PF_CONSOLEAPPLICATION) &&
        (gspwndLogonNotify != NULL) &&
        !(ppi->rpwinsta->dwWSF_Flags & WSF_OPENLOCK)) {

        PTHREADINFO pti = GETPTI(gspwndLogonNotify);
        PQMSG pqmsg;

        if ((pqmsg = AllocQEntry(&pti->mlPost)) != NULL) {
            StoreQMessage(pqmsg, gspwndLogonNotify, WM_LOGONNOTIFY,
                    LOGON_PLAYEVENTSOUND, USER_SOUND_CLOSE, 0, 0, 0);

            SetWakeBit(pti, QS_POSTMESSAGE | QS_ALLPOSTMESSAGE);
        }

    }

    /*
     * Be like WIN95.
     * If this is the shell process, then send a LOGON_RESTARTSHELL
     *  notification to the winlogon process (only if not logging off)
     */
    if (IsShellProcess(ppi)) {

        /*
         * The shell process will get killed and it's better to set this
         * in the desktop info.
         */
        ppi->rpdeskStartup->pDeskInfo->ppiShellProcess = NULL;

        /*
         * If we're not logging off, notify winlogon
         */
        if ((gspwndLogonNotify != NULL) &&
             !(ppi->rpwinsta->dwWSF_Flags & WSF_OPENLOCK)) {

            PTHREADINFO pti = GETPTI(gspwndLogonNotify);
            PQMSG pqmsg;

            if ((pqmsg = AllocQEntry(&pti->mlPost)) != NULL) {
                StoreQMessage(pqmsg, gspwndLogonNotify, WM_LOGONNOTIFY,
                        LOGON_RESTARTSHELL, ppi->Process->ExitStatus, 0, 0, 0);
                SetWakeBit(pti, QS_POSTMESSAGE | QS_ALLPOSTMESSAGE);
            }
        }
    }

    if (ppi->cThreads)
        RIPMSG1(RIP_ERROR, "Disconnect with %d threads remaining\n", ppi->cThreads);

    /*
     * If the app is still starting, remove it from the startup list
     */
    if (ppi->W32PF_Flags & W32PF_APPSTARTING) {
        /*
         * Bug 294193 - joejo
         *
         * Handle case when creator process exits before the child
         * process makes it to CheckAllowForeground code. This is typical with
         * stub EXEs that do nothing but create other processes.
         */
        GiveForegroundActivateRight(ppi->Process->UniqueProcessId);
        ClearAppStarting(ppi);
    }

    /*
     * remove it from the global list
     */
    REMOVE_FROM_LIST(PROCESSINFO, gppiList, ppi, ppiNextRunning);

    /*
     * If any threads ever connected, there may be DCs, classes,
     * cursors, etc. still lying around.  If no threads connected
     * (which is the case for console apps), skip all of this cleanup.
     */
    fHadThreads = ppi->W32PF_Flags & W32PF_THREADCONNECTED;
    if (fHadThreads) {

        /*
         * When a process dies we need to make sure any DCE's it owns
         * and have not been deleted are cleanup up.  The clean up
         * earlier may have failed if the DC was busy in GDI.
         */
        if (ppi->W32PF_Flags & W32PF_OWNDCCLEANUP) {
            DelayedDestroyCacheDC();
        }

#if DBG
        {
            PHE pheT, pheMax;

            /*
             * Loop through the table destroying all objects created by the current
             * process. All objects will get destroyed in their proper order simply
             * because of the object locking.
             */
            pheMax = &gSharedInfo.aheList[giheLast];
            for (pheT = gSharedInfo.aheList; pheT <= pheMax; pheT++) {

                /*
                 * We should have no process objects left for this process.
                 */
                UserAssertMsg0(
                        pheT->bFlags & HANDLEF_DESTROY ||
                        !(gahti[pheT->bType].bObjectCreateFlags & OCF_PROCESSOWNED) ||
                        (PPROCESSINFO)pheT->pOwner != ppi,
                        "We should have no process objects left for this process!");
            }
        }
#endif
    }

    if (pwp->UserHandleCount)
        RIPMSG1(RIP_ERROR, "Disconnect with %d User handle objects remaining\n", pwp->UserHandleCount);

    /*
     * check if we need to zap PID's for DDE objects
     */
    for (ppo = gpPublicObjectList;
            ppo != NULL;
                ppo = ppo->next) {
        if (ppo->pid == pwp->W32Pid) {
            ppo->pid = OBJECT_OWNER_PUBLIC;
        }
    }


    if (gppiScreenSaver == ppi) {
        UserAssert(ppi->W32PF_Flags & W32PF_SCREENSAVER);

        gppiScreenSaver = NULL;
    }

    if (gppiForegroundOld == ppi) {
        gppiForegroundOld = NULL;
    }

    UnlockWinSta(&ppi->rpwinsta);
    UnlockDesktop(&ppi->rpdeskStartup, LDU_PPI_DESKSTARTUP3, (ULONG_PTR)ppi);

    /*
     * Close the startup desktop handle now if it's still around. If we wait
     * until handle table cleanup time we could potentially deadlock.
     */
    if (ppi->hdeskStartup) {
        UserVerify(NT_SUCCESS(CloseProtectedHandle(ppi->hdeskStartup)));
        ppi->hdeskStartup = NULL;
    }

    /*
     * Mark the process as terminated so access checks will work.
     */
    ppi->W32PF_Flags |= W32PF_TERMINATED;

    /*
     * Cleanup wow process info struct, if any
     */
    if (ppi->pwpi) {
        PWOWPROCESSINFO pwpi = ppi->pwpi;

        ObDereferenceObject(pwpi->pEventWowExec);

        REMOVE_FROM_LIST(WOWPROCESSINFO, gpwpiFirstWow, pwpi, pwpiNext);

        UserFreePool(pwpi);
        ppi->pwpi = NULL;
    }

    /*
     * Delete desktop views.  System will do unmapping.
     */
    pdv = ppi->pdvList;
    while (pdv) {
        pdvNext = pdv->pdvNext;
        UserFreePool(pdv);
        pdv = pdvNext;
    }
    ppi->pdvList = NULL;

    /*
     * Clear the SendInput/Journalling hook caller ppi
     */
    if (ppi == gppiInputProvider) {
        gppiInputProvider = NULL;
    }
    /*
     * If this ppi locked SetForegroundWindow, clean up
     */
    if (ppi == gppiLockSFW) {
        gppiLockSFW = NULL;
    }

    return fHadThreads;
}

/***************************************************************************\
* ClearWakeMask
*
\***************************************************************************/

VOID ClearWakeMask(VOID)
{
    PtiCurrent()->pcti->fsWakeMask = 0;
}

/***************************************************************************\
* xxxGetInputEvent
*
* Returns a duplicated event-handle that the client process can use to
* wait on input events.
*
* History:
* 05-02-91  DavidPe     Created.
\***************************************************************************/

HANDLE xxxGetInputEvent(
    DWORD dwWakeMask)
{
    PTHREADINFO ptiCurrent;
    WORD wFlags = HIWORD(dwWakeMask);
    UserAssert(IsWinEventNotifyDeferredOK());

    ptiCurrent = PtiCurrent();

    /*
     * If our wait condition is satisfied, signal the event and return.
     * (Since the wake mask could have been anything at the time the input
     *  arrived, the event might not be signaled)
     */
    if (GetInputBits(ptiCurrent->pcti, LOWORD(dwWakeMask), (wFlags & MWMO_INPUTAVAILABLE))) {
        KeSetEvent(ptiCurrent->pEventQueueServer, 2, FALSE);
        return ptiCurrent->hEventQueueClient;
    }

    /*
     * If an idle hook is set, call it.
     */
    if (ptiCurrent == gptiForeground &&
            IsHooked(ptiCurrent, WHF_FOREGROUNDIDLE)) {
        xxxCallHook(HC_ACTION, 0, 0, WH_FOREGROUNDIDLE);
    }

    CheckForClientDeath();

    /*
     * What is the criteria for an "idle process"?
     * Answer: The first thread that calls zzzWakeInputIdle, or SleepInputIdle or...
     * Any thread that calls xxxGetInputEvent with any of the following
     * bits set in its wakemask: (sanfords)
     */
    if (dwWakeMask & (QS_POSTMESSAGE | QS_INPUT)) {
        ptiCurrent->ppi->ptiMainThread = ptiCurrent;
    }

    /*
     * When we return, this app is going to sleep. Since it is in its
     * idle mode when it goes to sleep, wake any apps waiting for this
     * app to go idle.
     */
    zzzWakeInputIdle(ptiCurrent);
    /*
     * Setup the wake mask for this thread. Wait for QS_EVENT or the app won't
     * get event messages like deactivate.
     */
    ClearQueueServerEvent((WORD)(dwWakeMask | QS_EVENT));
    /*
     * This app is going idle. Clear the spin count check to see
     * if we need to make this process foreground again.
     */
    ptiCurrent->pClientInfo->cSpins = 0;
    if (ptiCurrent->TIF_flags & TIF_SPINNING) {
        CheckProcessForeground(ptiCurrent);
    }

    UserAssert(ptiCurrent->pcti->fsWakeMask != 0);
    return ptiCurrent->hEventQueueClient;
}

/***************************************************************************\
* xxxWaitForInputIdle
*
* This routine waits on a particular input queue for "input idle", meaning
* it waits till that queue has no input to process.
*
* 09-13-91 ScottLu      Created.
\***************************************************************************/

DWORD xxxWaitForInputIdle(
    ULONG_PTR idProcess,
    DWORD dwMilliseconds,
    BOOL fSharedWow)
{
    PTHREADINFO ptiCurrent;
    PTHREADINFO pti;
    PEPROCESS Process;
    PW32PROCESS W32Process;
    PPROCESSINFO ppi;
    DWORD dwResult;
    NTSTATUS Status;
    TL tlProcess;

    ptiCurrent = PtiCurrent();

    /*
     * If fSharedWow is set, the client passed in a fake process
     * handle which CreateProcess returns for Win16 apps started
     * in the shared WOW VDM.
     *
     * CreateProcess returns a real process handle when you start
     * a Win16 app in a separate WOW VDM.
     */

    if (fSharedWow) {  // Waiting for a WOW task to go idle.
        PWOWTHREADINFO pwti;


        /*
         * Look for a matching thread in the WOW thread info list.
         */
        for (pwti = gpwtiFirst; pwti != NULL; pwti = pwti->pwtiNext) {
            if (pwti->idParentProcess == HandleToUlong(ptiCurrent->pEThread->Cid.UniqueProcess) &&
                pwti->idWaitObject == idProcess) {
                break;
            }
        }

        /*
         * If we couldn't find the right thread, bail out.
         */
        if (pwti == NULL) {
            RIPMSG0(RIP_WARNING, "WaitForInputIdle couldn't find 16-bit task\n");
            return (DWORD)-1;
        }

        /*
         * Now wait for it to go idle and return.
         */
        dwResult = WaitOnPseudoEvent(&pwti->pIdleEvent, dwMilliseconds);
        if (dwResult == STATUS_ABANDONED) {
            dwResult = xxxPollAndWaitForSingleObject(pwti->pIdleEvent,
                                                     NULL,
                                                     dwMilliseconds);
        }
        return dwResult;

    }

    /*
     * We shouldn't get here for system threads.
     */
    UserAssert(!(ptiCurrent->TIF_flags & TIF_SYSTEMTHREAD));

    /*
     * If the app is waiting for itself to go idle, error.
     */
    if (ptiCurrent->pEThread->Cid.UniqueProcess == (HANDLE)idProcess &&
            ptiCurrent == ptiCurrent->ppi->ptiMainThread) {
        RIPMSG0(RIP_WARNING, "WaitForInputIdle waiting on self\n");
        return (DWORD)-1;
    }

    /*
     * Now find the ppi structure for this process.
     */
    LeaveCrit();
    Status = LockProcessByClientId((HANDLE)idProcess, &Process);
    EnterCrit();

    if (!NT_SUCCESS(Status))
        return (DWORD)-1;

    if (Process->ExitProcessCalled) {
        UnlockProcess(Process);
        return (DWORD)-1;
    }

    W32Process = (PW32PROCESS)Process->Win32Process;

    /*
     * Couldn't find that process info structure....  return error.
     */
    if (W32Process == NULL) {
        RIPMSG0(RIP_WARNING, "WaitForInputIdle process not GUI process\n");
        UnlockProcess(Process);
        return (DWORD)-1;
    }


    ppi = (PPROCESSINFO)W32Process;

    /*
     * If this is a console application, don't wait on it.
     */
    if (W32Process->W32PF_Flags & W32PF_CONSOLEAPPLICATION) {
        RIPMSG0(RIP_WARNING, "WaitForInputIdle process is console process\n");
        UnlockProcess(Process);
        return (DWORD)-1;
    }

    /*
     * Wait on this event for the passed in time limit.
     */
    CheckForClientDeath();

    /*
     * We have to wait mark the Process as one which others are waiting on
     */
    ppi->W32PF_Flags |= W32PF_WAITFORINPUTIDLE;
    for (pti = ppi->ptiList; pti != NULL; pti = pti->ptiSibling) {
        pti->TIF_flags |= TIF_WAITFORINPUTIDLE;
    }

    /*
     * Thread lock the process to ensure that it will be dereferenced
     * if the thread exits.
     */
    LockW32Process(W32Process, &tlProcess);
    UnlockProcess(Process);

    dwResult = WaitOnPseudoEvent(&W32Process->InputIdleEvent, dwMilliseconds);
    if (dwResult == STATUS_ABANDONED) {
        dwResult = xxxPollAndWaitForSingleObject(W32Process->InputIdleEvent,
                                                 Process,
                                                 dwMilliseconds);
    }

    /*
     * Clear all thread TIF_WAIT bits from the process.
     */
    ppi->W32PF_Flags &= ~W32PF_WAITFORINPUTIDLE;
    for (pti = ppi->ptiList; pti != NULL; pti = pti->ptiSibling) {
        pti->TIF_flags &= ~TIF_WAITFORINPUTIDLE;
    }

    UnlockW32Process(&tlProcess);

    return dwResult;
}


#define INTERMEDIATE_TIMEOUT    (500)       // 1/2 second

/***************************************************************************\
* xxxPollAndWaitForSingleObject
*
* Sometimes we have to wait on an event but still want to periodically
* wake up and see if the client process has been terminated.
*
* dwMilliseconds is initially the total amount of time to wait and after
* each intermediate wait reflects the amount of time left to wait.
* -1 means wait indefinitely.
*
* 02-Jul-1993 johnc      Created.
\***************************************************************************/

// LATER!!! can we get rid of the Polling idea and wait additionally on
// LATER!!! the hEventServer and set that when a thread dies

DWORD xxxPollAndWaitForSingleObject(
    PKEVENT pEvent,
    PVOID pExecObject,
    DWORD dwMilliseconds)
{
    DWORD dwIntermediateMilliseconds, dwStartTickCount;
    PTHREADINFO ptiCurrent;
    UINT cEvent = 2;
    NTSTATUS Status = -1;
    LARGE_INTEGER li;
    TL tlEvent;

    ptiCurrent = PtiCurrent();

    if (ptiCurrent->apEvent == NULL) {
        ptiCurrent->apEvent = UserAllocPoolNonPaged(POLL_EVENT_CNT * sizeof(PKEVENT), TAG_EVENT);
        if (ptiCurrent->apEvent == NULL)
            return (DWORD)-1;
    }

    /*
     * Refcount the event to ensure that it won't go
     * away during the wait.  By using a thread lock, the
     * event will be dereferenced if the thread exits
     * during a callback.  The process pointer has already been
     * locked.
     */
    ThreadLockObject(pEvent, &tlEvent);

    /*
     * If a process was passed in, wait on it too.  No need
     * to reference this because the caller has it referenced.
     */
    if (pExecObject) {
        cEvent++;
    }

    /*
     * We want to wake if there're sent messages pending
     */
    ClearQueueServerEvent(QS_SENDMESSAGE);

    /*
     * Wow Tasks MUST be descheduled while in the wait to allow
     * other tasks in the same wow scheduler to run.
     *
     * For example, 16 bit app A calls WaitForInputIdle on 32 bit app B.
     * App B starts up and tries to send a message to 16 bit app C. App C
     * will never be able to process the message unless app A yields
     * control to it, so app B will never go idle.
     */

    if (ptiCurrent->TIF_flags & TIF_16BIT) {
        xxxSleepTask(FALSE, HEVENT_REMOVEME);
        // caution: the wow task is no longer scheduled.
    }

    dwStartTickCount = NtGetTickCount();
    while (TRUE) {
        if (dwMilliseconds > INTERMEDIATE_TIMEOUT) {
            dwIntermediateMilliseconds = INTERMEDIATE_TIMEOUT;

            /*
             * If we are not waiting an infinite amount of time then subtract
             * the last loop duration from time left to wait.
             */
            if (dwMilliseconds != INFINITE) {
                DWORD dwNewTickCount = NtGetTickCount();
                DWORD dwDelta = ComputePastTickDelta(dwNewTickCount, dwStartTickCount);
                dwStartTickCount = dwNewTickCount;
                if (dwDelta < dwMilliseconds) {
                    dwMilliseconds -= dwDelta;
                } else {
                    dwMilliseconds = 0;
                }
            }
        } else {
            dwIntermediateMilliseconds = dwMilliseconds;
            dwMilliseconds = 0;
        }

        /*
         * Convert dwMilliseconds to a relative-time(i.e.  negative) LARGE_INTEGER.
         * NT Base calls take time values in 100 nanosecond units.
         */
        if (dwIntermediateMilliseconds != INFINITE)
            li.QuadPart = Int32x32To64(-10000, dwIntermediateMilliseconds);

        /*
         * Load events into the wait array.  Do this every time
         * through the loop in case of recursion.
         */
        ptiCurrent->apEvent[IEV_IDLE] = pEvent;
        ptiCurrent->apEvent[IEV_INPUT] = ptiCurrent->pEventQueueServer;
        ptiCurrent->apEvent[IEV_EXEC] = pExecObject;

        LeaveCrit();

        Status = KeWaitForMultipleObjects(cEvent,
                                          &ptiCurrent->apEvent[IEV_IDLE],
                                          WaitAny,
                                          WrUserRequest,
                                          UserMode,
                                          FALSE,
                                          (dwIntermediateMilliseconds == INFINITE ?
                                                  NULL : &li),
                                          NULL);

        EnterCrit();

        if (!NT_SUCCESS(Status)) {
            Status = -1;
        } else {

            /*
             * Because we do a non-alertable wait, we know that a status
             * of STATUS_USER_APC means that the thread was terminated.
             * If we have terminated, get back to user mode
             */
            if (Status == STATUS_USER_APC) {
                ClientDeliverUserApc();
                Status = -1;
            }
        }

        if (ptiCurrent->pcti->fsChangeBits & QS_SENDMESSAGE) {
            /*
             *  Wow Tasks MUST wait to be rescheduled in the wow non-premptive
             *  scheduler before doing anything which might invoke client code.
             */
            if (ptiCurrent->TIF_flags & TIF_16BIT) {
                xxxDirectedYield(DY_OLDYIELD);
            }

            xxxReceiveMessages(ptiCurrent);

            if (ptiCurrent->TIF_flags & TIF_16BIT) {
                xxxSleepTask(FALSE, HEVENT_REMOVEME);
                // caution: the wow task is no longer scheduled.
            }
        }

        /*
         * If we returned from the wait for some other reason than a timeout
         * or to receive messages we are done.  If it is a timeout we are
         * only done waiting if the overall time is zero.
         */
        if (Status != STATUS_TIMEOUT && Status != 1)
            break;

        if (dwMilliseconds == 0) {
            /*
             * Fix up the return if the last poll was interupted by a message
             */
            if (Status == 1)
                Status = WAIT_TIMEOUT;
            break;
        }

    }

    /*
     * reschedule the 16 bit app
     */
    if (ptiCurrent->TIF_flags & TIF_16BIT) {
        xxxDirectedYield(DY_OLDYIELD);
    }

    /*
     * Unlock the events.
     */
    ThreadUnlockObject(&tlEvent);

    return Status;
}



/***************************************************************************\
 * WaitOnPseudoEvent
 *
 * Similar semantics to WaitForSingleObject() but works with pseudo events.
 * Could fail if creation on the fly fails.
 * Returns STATUS_ABANDONED_WAIT if caller needs to wait on the event and event is
 * created and ready to be waited on.
 *
 * This assumes the event was created with fManualReset=TRUE, fInitState=FALSE
 *
 * 10/28/93 SanfordS    Created
\***************************************************************************/
DWORD WaitOnPseudoEvent(
    HANDLE *phE,
    DWORD dwMilliseconds)
{
    HANDLE hEvent;
    NTSTATUS Status;

    CheckCritIn();
    if (*phE == PSEUDO_EVENT_OFF) {
        if (!NT_SUCCESS(ZwCreateEvent(&hEvent, EVENT_ALL_ACCESS, NULL,
                NotificationEvent, FALSE))) {
            UserAssert(!"Could not create event on the fly.");
            if (dwMilliseconds != INFINITE) {
                return STATUS_TIMEOUT;
            } else {
                return (DWORD)-1;
            }
        }
        Status = ObReferenceObjectByHandle(hEvent, EVENT_ALL_ACCESS, *ExEventObjectType,
                KernelMode, phE, NULL);
        ZwClose(hEvent);
        if (!NT_SUCCESS(Status))
            return (DWORD)-1;
    } else if (*phE == PSEUDO_EVENT_ON) {
        return STATUS_WAIT_0;
    }
    return(STATUS_ABANDONED);
}

/***************************************************************************\
* xxxSetCsrssThreadDesktop
*
* Set/clear and lock/unlock a desktop for a csrss thread
* When setting a desktop, ppdeskRestore must be valid and will receive
*  the old (previous) desktop, if any; the caller is expected to restore
*  this pdesk when done.
* When restoring a desktop, ppdeskRestore must be NULL. pdesk must have been
*  previously returned by this same function (in *ppdeskRestore).
*
* History:
* 02-18-97 GerardoB     Extracted from SetInformationThread
\***************************************************************************/
NTSTATUS xxxSetCsrssThreadDesktop(PDESKTOP pdesk, PDESKRESTOREDATA pdrdRestore)
{
    PTHREADINFO ptiCurrent = PtiCurrent();
    NTSTATUS Status = STATUS_SUCCESS;
    MSG msg;

    /*
     * Only csr should come here
     */
    UserAssert(ISCSRSS());
    UserAssert(pdrdRestore);
    UserAssert(pdrdRestore->pdeskRestore == NULL);

    if (pdesk->dwDTFlags & DF_DESTROYED) {
        RIPMSG1(RIP_WARNING, "xxxSetCsrssThreadDesktop: pdesk 0x%x destroyed",
                pdesk);
        return STATUS_UNSUCCESSFUL;
    }

    /*
     * Lock the current desktop (set operation).  Also, create and save a
     * handle to the new desktop.
     */
    pdrdRestore->pdeskRestore = ptiCurrent->rpdesk;

    if (pdrdRestore->pdeskRestore != NULL) {
        Status = ObReferenceObjectByPointer(pdrdRestore->pdeskRestore,
                                       MAXIMUM_ALLOWED,
                                       *ExDesktopObjectType,
                                       KernelMode);

        if (!NT_SUCCESS(Status)) {
            pdrdRestore->pdeskRestore = NULL;
            pdrdRestore->hdeskNew = NULL;
            return Status;
        }
        LogDesktop(pdrdRestore->pdeskRestore, LD_REF_FN_SETCSRSSTHREADDESKTOP, TRUE, (ULONG_PTR)PtiCurrent());
    }

    Status = ObOpenObjectByPointer(
             pdesk,
             0,
             NULL,
             EVENT_ALL_ACCESS,
             NULL,
             KernelMode,
             &(pdrdRestore->hdeskNew));


    if (!NT_SUCCESS(Status)) {
        RIPNTERR2(Status, RIP_WARNING, "SetCsrssThreadDesktop, can't open handle, pdesk %#p. Status: %#x", pdesk, Status);
        if (pdrdRestore->pdeskRestore) {
            LogDesktop(pdrdRestore->pdeskRestore, LD_DEREF_FN_SETCSRSSTHREADDESKTOP1, FALSE, (ULONG_PTR)PtiCurrent());
            ObDereferenceObject(pdrdRestore->pdeskRestore);
            pdrdRestore->pdeskRestore = NULL;
        }
        pdrdRestore->hdeskNew = NULL;
        return Status;
    }
    /*
     * Set the new desktop, if switching
     */
    if (pdesk != ptiCurrent->rpdesk) {
        /*
         * Process any remaining messages before we leave the desktop
         */
        if (ptiCurrent->rpdesk) {
            while (xxxPeekMessage(&msg, NULL, 0, 0, PM_REMOVE | PM_NOYIELD))
                xxxDispatchMessage(&msg);
        }

        if (!xxxSetThreadDesktop(NULL, pdesk)) {
            RIPMSG1(RIP_WARNING, "xxxSetCsrssThreadDesktop: xxxSetThreadDesktop(%#p) failed", pdesk);
            Status = STATUS_INVALID_HANDLE;
            /*
             * We're failing so deref if needed.
             */
            if (pdrdRestore->pdeskRestore != NULL) {
                LogDesktop(pdrdRestore->pdeskRestore, LD_DEREF_FN_SETCSRSSTHREADDESKTOP1, FALSE, (ULONG_PTR)PtiCurrent());
                ObDereferenceObject(pdrdRestore->pdeskRestore);
                pdrdRestore->pdeskRestore = NULL;
            }
            CloseProtectedHandle(pdrdRestore->hdeskNew);
            pdrdRestore->hdeskNew = NULL;
        }
    }

    UserAssert(NT_SUCCESS(Status));
    return Status;
}

NTSTATUS xxxRestoreCsrssThreadDesktop(PDESKRESTOREDATA pdrdRestore)
{
    PTHREADINFO ptiCurrent = PtiCurrent();
    NTSTATUS Status = STATUS_SUCCESS;
    MSG msg;

    /*
     * Only csr should come here
     */
    UserAssert(ISCSRSS());
    UserAssert(pdrdRestore);

    /*
     * Set the new desktop, if switching
     */
    if (pdrdRestore->pdeskRestore != ptiCurrent->rpdesk) {
        /*
         * Process any remaining messages before we leave the desktop
         */
        if (ptiCurrent->rpdesk) {
            while (xxxPeekMessage(&msg, NULL, 0, 0, PM_REMOVE | PM_NOYIELD))
                xxxDispatchMessage(&msg);
        }

        if (!xxxSetThreadDesktop(NULL, pdrdRestore->pdeskRestore)) {
            RIPMSG1(RIP_WARNING, "xxxRestoreCsrssThreadDesktop: xxxRestoreThreadDesktop(%#p) failed", pdrdRestore->pdeskRestore);
            Status = STATUS_INVALID_HANDLE;
        }
    }

    /*
     * Dereference the desktop,
     *   even if failing the call.
     */
    if (pdrdRestore->pdeskRestore != NULL) {
        LogDesktop(pdrdRestore->pdeskRestore, LD_DEREF_FN_SETCSRSSTHREADDESKTOP2, FALSE, 0);
        ObDereferenceObject(pdrdRestore->pdeskRestore);
        pdrdRestore->pdeskRestore = NULL;
    }

    if(pdrdRestore->hdeskNew) {
        CloseProtectedHandle(pdrdRestore->hdeskNew);
        UserAssert(NT_SUCCESS(Status));
        pdrdRestore->hdeskNew = NULL;
    }
    return Status;
}

/***************************************************************************\
* GetTaskName
*
* Gets the application name from a thread.
\***************************************************************************/
ULONG GetTaskName(
    PTHREADINFO pti,
    PWSTR Buffer,
    ULONG BufferLength)
{
    ANSI_STRING strAppName;
    UNICODE_STRING strAppNameU;
    NTSTATUS Status;
    ULONG NameLength = 0;

    if (pti == NULL) {
        *Buffer = 0;
        return 0;
    }
    if (pti->pstrAppName != NULL) {
        NameLength = min(pti->pstrAppName->Length + sizeof(WCHAR), BufferLength);
        RtlCopyMemory(Buffer, pti->pstrAppName->Buffer, NameLength);
    } else {
        RtlInitAnsiString(&strAppName, pti->pEThread->ThreadsProcess->ImageFileName);
        if (BufferLength < sizeof(WCHAR))
            NameLength = (strAppName.Length + 1) * sizeof(WCHAR);
        else {
            strAppNameU.Buffer = Buffer;
            strAppNameU.MaximumLength = (SHORT)BufferLength - sizeof(WCHAR);
            Status = RtlAnsiStringToUnicodeString(&strAppNameU, &strAppName,
                    FALSE);
            if (NT_SUCCESS(Status))
                NameLength = strAppNameU.Length + sizeof(WCHAR);
        }
    }
    Buffer[(NameLength / sizeof(WCHAR)) - 1] = 0;

    return NameLength;
}

/***************************************************************************\
* QueryInformationThread
*
* Returns information about a thread.
*
* History:
* 03-01-95 JimA         Created.
\***************************************************************************/

NTSTATUS xxxQueryInformationThread(
    IN HANDLE hThread,
    IN USERTHREADINFOCLASS ThreadInfoClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    OUT PULONG ReturnLength OPTIONAL)
{
    PUSERTHREAD_SHUTDOWN_INFORMATION pShutdown;
    PUSERTHREAD_WOW_INFORMATION pWow;
    PETHREAD Thread;
    PTHREADINFO pti;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG LocalReturnLength = 0;
    DWORD dwClientFlags;

    UNREFERENCED_PARAMETER(ThreadInformationLength);
    /*
     * Only allow CSRSS to make this call
     */
    UserAssert(ISCSRSS());

    Status = ObReferenceObjectByHandle(hThread,
                                        THREAD_QUERY_INFORMATION,
                                        *PsThreadType,
                                        UserMode,
                                        &Thread,
                                        NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    pti = PtiFromThread(Thread);

    switch (ThreadInfoClass) {
    case UserThreadShutdownInformation:
        LocalReturnLength = sizeof(USERTHREAD_SHUTDOWN_INFORMATION);
        UserAssert(ThreadInformationLength == sizeof(USERTHREAD_SHUTDOWN_INFORMATION));
        pShutdown = ThreadInformation;
        /*
         * Read the client flags and zero out the structure,
         *  except for pdeskRestore (which is supposed
         *  to be the last field)
         */
        dwClientFlags = pShutdown->dwFlags;
        UserAssert(FIELD_OFFSET(USERTHREAD_SHUTDOWN_INFORMATION, drdRestore)
            == (sizeof(USERTHREAD_SHUTDOWN_INFORMATION) - sizeof(DESKRESTOREDATA)));
        RtlZeroMemory(pShutdown,
            sizeof(USERTHREAD_SHUTDOWN_INFORMATION) - sizeof(DESKRESTOREDATA));

        /*
         * Return the desktop window handle if the thread
         * has a desktop and the desktop is on a visible
         * windowstation.
         */
        if (pti != NULL && pti->rpdesk != NULL &&
                !(pti->rpdesk->rpwinstaParent->dwWSF_Flags & WSF_NOIO))
            pShutdown->hwndDesktop = HW(pti->rpdesk->pDeskInfo->spwnd);

        /*
         * Return shutdown status.  Zero indicates that the thread
         * has windows and can be shut down in the normal manner.
         */
        if (Thread->Cid.UniqueProcess == gpidLogon) {
            /*
             * Do not shutdown the logon process.
             */
            pShutdown->StatusShutdown = SHUTDOWN_KNOWN_PROCESS;
        } else if (pti == NULL || pti->rpdesk == NULL) {

            /*
             * The thread either is not a gui thread or it doesn't
             * have a desktop.  Make console do the shutdown.
             */
            pShutdown->StatusShutdown = SHUTDOWN_UNKNOWN_PROCESS;
        }

        /*
         * Return flags
         */
        if (pti != NULL && pti->cWindows != 0)
            pShutdown->dwFlags |= USER_THREAD_GUI;

        /*
         * If we return the desktop window handle and the
         * app should be shut down, switch to the desktop
         *  and assign it to the shutdown worker thread.
         */
        if ((pShutdown->dwFlags & USER_THREAD_GUI) &&
                pShutdown->StatusShutdown == 0) {
            /*
             * The current csrss thread is going to
             *  make activation calls, send messages,
             *  switch video modes, etc  so we need to
             *  assign it to a dekstop
             */
            PTHREADINFO ptiCurrent = PtiCurrent();
            UserAssert(pti->rpdesk != NULL);

            if (ptiCurrent->rpdesk != pti->rpdesk) {
                /*
                 * If this thread already has a desktop,
                 *   restore the old one first.
                 *  This might happen when threads of the same
                 *  process are attached to different desktops.
                 */
                if (ptiCurrent->rpdesk != NULL) {
                    Status = xxxRestoreCsrssThreadDesktop(&pShutdown->drdRestore);
                    UserAssert(pti == PtiFromThread(Thread));
                }
                if (NT_SUCCESS(Status)) {
                    Status = xxxSetCsrssThreadDesktop(pti->rpdesk, &pShutdown->drdRestore);
                    UserAssert(pti == PtiFromThread(Thread));
                }
            }
            /*
             * If we're forcing a shutdown, then there is no need to switch
             *  since we won't send any messages or bring up the EndTask dialog
             * (We still want to have a proper rpdesk since BoostHardError might
             *   call PostThreadMessage)
             */
            if (!(dwClientFlags & WMCS_NORETRY)) {
                if (NT_SUCCESS(Status)) {
                    #if DBG
                        BOOL fSwitch =
                    #endif
                        xxxSwitchDesktop(pti->rpdesk->rpwinstaParent, pti->rpdesk, FALSE);
                    #if DBG
                        UserAssert(pti == PtiFromThread(Thread));
                        if (!fSwitch) {
                            if ((pti->rpdesk->rpwinstaParent == NULL)
                                    || !(pti->rpdesk->rpwinstaParent->dwWSF_Flags & WSF_NOIO)) {

                                if (!(pti->rpdesk->rpwinstaParent->dwWSF_Flags & WSF_REALSHUTDOWN))
                                    RIPMSG1(RIP_ERROR, "UserThreadShutdownInformation: xxxSwitchDesktop failed. pti:%#p", pti);

                            }
                        }
                    #endif
                }
            }
        }
        break;

    case UserThreadFlags:
        LocalReturnLength = sizeof(DWORD);
        if (pti == NULL)
            Status = STATUS_INVALID_HANDLE;
        else {
            UserAssert(ThreadInformationLength == sizeof(DWORD));
            *(LPDWORD)ThreadInformation = pti->TIF_flags;
        }
        break;

    case UserThreadTaskName:
        LocalReturnLength = GetTaskName(pti, ThreadInformation, ThreadInformationLength);
        break;

    case UserThreadWOWInformation:
        LocalReturnLength = sizeof(USERTHREAD_WOW_INFORMATION);
        UserAssert(ThreadInformationLength == sizeof(USERTHREAD_WOW_INFORMATION));
        pWow = ThreadInformation;
        RtlZeroMemory(pWow, sizeof(USERTHREAD_WOW_INFORMATION));

        /*
         * If the thread is 16-bit, Status = the exit task function
         * and task id.
         */
        if (pti && pti->TIF_flags & TIF_16BIT) {
            pWow->lpfnWowExitTask = pti->ppi->pwpi->lpfnWowExitTask;
            if (pti->ptdb)
                pWow->hTaskWow = pti->ptdb->hTaskWow;
            else
                pWow->hTaskWow = 0;
        }
        break;

    case UserThreadHungStatus:
        LocalReturnLength = sizeof(DWORD);
        UserAssert(ThreadInformationLength >= sizeof(DWORD));

        /*
         * Return hung status.
         */
        if (pti)
            *(PDWORD)ThreadInformation =
                    (DWORD) FHungApp(pti, (DWORD)*(PDWORD)ThreadInformation);
        else
            *(PDWORD)ThreadInformation = FALSE;
        break;

    default:
        Status = STATUS_INVALID_INFO_CLASS;
        UserAssert(FALSE);
        break;
    }

    if (ARGUMENT_PRESENT(ReturnLength) ) {
        *ReturnLength = LocalReturnLength;
        }

    UnlockThread(Thread);

    return Status;
}

/***************************************************************************\
* xxxSetInformationThread
*
* Sets information about a thread.
*
* History:
* 03-01-95 JimA         Created.
\***************************************************************************/

NTSTATUS xxxSetInformationThread(
    IN HANDLE hThread,
    IN USERTHREADINFOCLASS ThreadInfoClass,
    IN PVOID ThreadInformation,
    IN ULONG ThreadInformationLength)
{
    PUSERTHREAD_FLAGS pFlags;
    HANDLE hClientThread;
    DWORD dwOldFlags;
    PTHREADINFO ptiT;
    NTSTATUS Status = STATUS_SUCCESS;
    PETHREAD Thread;
    PETHREAD ThreadClient;
    PTHREADINFO pti;
    HANDLE CsrPortHandle;

    UNREFERENCED_PARAMETER(ThreadInformationLength);

    /*
     * Only allow CSRSS to make this call
     */
    UserAssert(ISCSRSS());

    Status = ObReferenceObjectByHandle(hThread,
                                        THREAD_SET_INFORMATION,
                                        *PsThreadType,
                                        UserMode,
                                        &Thread,
                                        NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    pti = PtiFromThread(Thread);

    switch (ThreadInfoClass) {
    case UserThreadFlags:
        if (pti == NULL)
            Status = STATUS_INVALID_HANDLE;
        else {
            UserAssert(ThreadInformationLength == sizeof(USERTHREAD_FLAGS));
            pFlags = ThreadInformation;
            dwOldFlags = pti->TIF_flags;
            pti->TIF_flags ^= ((dwOldFlags ^ pFlags->dwFlags) & pFlags->dwMask);
        }
        break;

    case UserThreadHungStatus:
        if (pti == NULL)
            Status = STATUS_INVALID_HANDLE;
        else {

            /*
             * No arguments, simple set the last time read.
             */
            SET_TIME_LAST_READ(pti);
        }
        break;

    case UserThreadInitiateShutdown:
        UserAssert(ThreadInformationLength == sizeof(ULONG));
        Status = InitiateShutdown(Thread, (PULONG)ThreadInformation);
        break;

    case UserThreadEndShutdown:
        UserAssert(ThreadInformationLength == sizeof(NTSTATUS));
        Status = EndShutdown(Thread, *(NTSTATUS *)ThreadInformation);
        break;

    case UserThreadUseDesktop:
        UserAssert(ThreadInformationLength == sizeof(USERTHREAD_USEDESKTOPINFO));
        if (pti == NULL) {
            Status = STATUS_INVALID_HANDLE;
            break;
        }

        /*
         * If the caller provides a thread handle, then we use that
         *  thread's pdesk and return the pdesk currently used
         *  by the caller (set operation). Otherwise,
         *  we use the pdesk provided by the caller (restore operation).
         */
        hClientThread = ((PUSERTHREAD_USEDESKTOPINFO)ThreadInformation)->hThread;
        if (hClientThread != NULL) {
            Status = ObReferenceObjectByHandle(hClientThread,
                                            THREAD_QUERY_INFORMATION,
                                            *PsThreadType,
                                            UserMode,
                                            &ThreadClient,
                                            NULL);
            if (!NT_SUCCESS(Status))
                break;

            ptiT = PtiFromThread(ThreadClient);
            if ((ptiT == NULL) || (ptiT->rpdesk == NULL)) {
                Status = STATUS_INVALID_HANDLE;
                goto DerefClientThread;
            }
            Status = xxxSetCsrssThreadDesktop(ptiT->rpdesk, &((PUSERTHREAD_USEDESKTOPINFO)ThreadInformation)->drdRestore);
        } else {
            Status = xxxRestoreCsrssThreadDesktop(&((PUSERTHREAD_USEDESKTOPINFO)ThreadInformation)->drdRestore);
        }


        if (hClientThread != NULL) {
DerefClientThread:
            ObDereferenceObject(ThreadClient);
        }
        break;

    case UserThreadUseActiveDesktop:
    {
        UserAssert(ThreadInformationLength == sizeof(USERTHREAD_USEDESKTOPINFO));
        if (pti == NULL || grpdeskRitInput == NULL) {
            Status = STATUS_INVALID_HANDLE;
            break;
        }
        Status = xxxSetCsrssThreadDesktop(grpdeskRitInput,
                    &((PUSERTHREAD_USEDESKTOPINFO)ThreadInformation)->drdRestore);
        break;
    }
    case UserThreadCsrApiPort:

        /*
         * Only CSR can call this
         */
        if (Thread->ThreadsProcess != gpepCSRSS) {
            Status = STATUS_ACCESS_DENIED;
            break;
        }

        UserAssert(ThreadInformationLength == sizeof(HANDLE));

        /*
         * Only set it once.
         */
        if (CsrApiPort != NULL)
            break;

        CsrPortHandle = *(PHANDLE)ThreadInformation;
        Status = ObReferenceObjectByHandle(
                CsrPortHandle,
                0,
                NULL, //*LpcPortObjectType,
                UserMode,
                &CsrApiPort,
                NULL);
        if (!NT_SUCCESS(Status)) {
            CsrApiPort = NULL;
            RIPMSG1(RIP_WARNING,
                    "CSR port reference failed, Status=%#lx",
                    Status);
        }

        break;

    default:
        Status = STATUS_INVALID_INFO_CLASS;
        UserAssert(FALSE);
        break;
    }

    UnlockThread(Thread);

    return Status;
}

#ifdef USE_MIRRORING
/***************************************************************************\
* _GetProcessDefaultLayout (API)
*
* Retreives the default layout information about a process.
*
* History:
* 23-01-98 SamerA         Created.
\***************************************************************************/
BOOL _GetProcessDefaultLayout(
    OUT DWORD *pdwDefaultLayout)
{
    NTSTATUS Status = STATUS_SUCCESS;

    /*
     * Do not allow CSRSS to make this call. This call might
     * happen due to the inheritence code. See xxxCreateWindowEx(...)
     */
    if (PsGetCurrentProcess() == gpepCSRSS) {
        return FALSE;
    }

    try {
        *pdwDefaultLayout = PpiCurrent()->dwLayout;
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        Status = GetExceptionCode();
    }

    return NT_SUCCESS(Status);
}

/***************************************************************************\
* _SetProcessDefaultLayout (API)
*
* Sets the default layout information about a process.
*
* History:
* 23-01-98 SamerA         Created.
\***************************************************************************/
BOOL _SetProcessDefaultLayout(
    IN DWORD dwDefaultLayout)
{
    /*
     * Do not allow CSRSS to make this call
     */
    UserAssert(PsGetCurrentProcess() != gpepCSRSS);

    /*
     * Validate dwDefaultLayout
     */
    if (dwDefaultLayout & ~LAYOUT_ORIENTATIONMASK)
    {
        RIPERR1(ERROR_INVALID_PARAMETER,
                RIP_WARNING,
                "Calling SetProcessDefaultLayout with invalid layout = %lX",
                dwDefaultLayout);
        return FALSE;
    }

    /*
     * Update the process default layout param
     */
    PpiCurrent()->dwLayout = dwDefaultLayout;

    return TRUE;
}
#endif

/***************************************************************************\
* SetInformationProcess
*
* Sets information about a process.
*
* History:
* 09-27-96 GerardoB         Created.
\***************************************************************************/

NTSTATUS SetInformationProcess(
    IN HANDLE hProcess,
    IN USERPROCESSINFOCLASS ProcessInfoClass,
    IN PVOID ProcessInformation,
    IN ULONG ProcessInformationLength)
{
    PUSERPROCESS_FLAGS pFlags;
    DWORD dwOldFlags;
    NTSTATUS Status = STATUS_SUCCESS;
    PEPROCESS Process;
    PPROCESSINFO ppi;

    UNREFERENCED_PARAMETER(ProcessInformationLength);

    UserAssert(ISCSRSS());

    Status = ObReferenceObjectByHandle(hProcess,
                                        PROCESS_SET_INFORMATION,
                                        *PsProcessType,
                                        UserMode,
                                        &Process,
                                        NULL);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    ppi = PpiFromProcess(Process);

    switch (ProcessInfoClass) {
    case UserProcessFlags:
        if (ppi == NULL) {
            Status = STATUS_INVALID_HANDLE;
        } else {
            UserAssert(ProcessInformationLength == sizeof(USERPROCESS_FLAGS));
            pFlags = ProcessInformation;
            dwOldFlags = ppi->W32PF_Flags;
            ppi->W32PF_Flags ^= ((dwOldFlags ^ pFlags->dwFlags) & pFlags->dwMask);
        }
        break;

    default:
        Status = STATUS_INVALID_INFO_CLASS;
        UserAssert(FALSE);
        break;
    }

    UnlockProcess(Process);

    return Status;
}


/***************************************************************************\
* xxxSetConsoleCaretInfo
*
* Store information about the console's homegrown caret and notify any
* interested apps that it changed. We need this for accessibility.
*
* History:
* 26-May-1999 JerrySh   Created.
\***************************************************************************/

VOID xxxSetConsoleCaretInfo(
    PCONSOLE_CARET_INFO pcci)
{
    PWND pwnd;
    TL tlpwnd;

    if (FWINABLE()) {
        pwnd = ValidateHwnd(pcci->hwnd);
        if (pwnd && pwnd->head.rpdesk) {
            pwnd->head.rpdesk->cciConsole = *pcci;
            ThreadLock(pwnd, &tlpwnd);
            xxxWindowEvent(EVENT_OBJECT_LOCATIONCHANGE, pwnd, OBJID_CARET, INDEXID_CONTAINER, WEF_ASYNC);
            ThreadUnlock(&tlpwnd);
        }
    }
}

/***************************************************************************\
* xxxConsoleControl
*
* Performs special control operations for console.
*
* History:
* 03-01-95 JimA         Created.
\***************************************************************************/

NTSTATUS xxxConsoleControl(
    IN CONSOLECONTROL ConsoleControl,
    IN PVOID ConsoleInformation,
    IN ULONG ConsoleInformationLength)
{
    PCONSOLEDESKTOPCONSOLETHREAD pDesktopConsole;
    PCONSOLEWINDOWSTATIONPROCESS pConsoleWindowStationInfo;
    PDESKTOP pdesk;
    DWORD dwThreadIdOld;
    NTSTATUS Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(ConsoleInformationLength);
    UserAssert(ISCSRSS());

    switch (ConsoleControl) {
    case ConsoleDesktopConsoleThread:
        UserAssert(ConsoleInformationLength == sizeof(CONSOLEDESKTOPCONSOLETHREAD));
        pDesktopConsole = (PCONSOLEDESKTOPCONSOLETHREAD)ConsoleInformation;

        Status = ObReferenceObjectByHandle(
                pDesktopConsole->hdesk,
                0,
                *ExDesktopObjectType,
                UserMode,
                &pdesk,
                NULL);
        if (!NT_SUCCESS(Status))
            return Status;

        LogDesktop(pdesk, LD_REF_FN_CONSOLECONTROL1, TRUE, (ULONG_PTR)PtiCurrent());

        dwThreadIdOld = pdesk->dwConsoleThreadId;

        if (pDesktopConsole->dwThreadId != (DWORD)-1) {
            pdesk->dwConsoleThreadId =
                    pDesktopConsole->dwThreadId;
        }

        pDesktopConsole->dwThreadId = dwThreadIdOld;
        LogDesktop(pdesk, LD_DEREF_FN_CONSOLECONTROL1, FALSE, (ULONG_PTR)PtiCurrent());
        ObDereferenceObject(pdesk);
        break;

    case ConsoleClassAtom:
        UserAssert(ConsoleInformationLength == sizeof(ATOM));
        gatomConsoleClass = *(ATOM *)ConsoleInformation;
        break;

    case ConsoleNotifyConsoleApplication:
        /*
         * Bug 273518 - joejo
         *
         * Adding optimization to bug fix
         */
        UserAssert(ConsoleInformationLength == sizeof(CONSOLE_PROCESS_INFO));
        xxxUserNotifyConsoleApplication((PCONSOLE_PROCESS_INFO)ConsoleInformation);
        break;

    case ConsoleSetVDMCursorBounds:
        UserAssert((ConsoleInformation == NULL) ||
            (ConsoleInformationLength == sizeof(RECT)));
        SetVDMCursorBounds(ConsoleInformation);
        break;

    case ConsolePublicPalette:
        UserAssert(ConsoleInformationLength == sizeof(HPALETTE));
        GreSetPaletteOwner(*(HPALETTE *)ConsoleInformation, OBJECT_OWNER_PUBLIC);
        break;

    case ConsoleWindowStationProcess:
        UserAssert(ConsoleInformationLength == sizeof(CONSOLEWINDOWSTATIONPROCESS));

        pConsoleWindowStationInfo = (PCONSOLEWINDOWSTATIONPROCESS)ConsoleInformation;
        UserSetConsoleProcessWindowStation(pConsoleWindowStationInfo->dwProcessId,
                                           pConsoleWindowStationInfo->hwinsta);
        break;

#if defined(FE_IME)
    /*
     * For console IME issue
     *
     * Console IME do register thread ID in DESKTOP info.
     * So should be per desktop.
     */
    case ConsoleRegisterConsoleIME:
        {
            PCONSOLE_REGISTER_CONSOLEIME RegConIMEInfo;
            DWORD dwConsoleIMEThreadIdOld;

            UserAssert(ConsoleInformationLength == sizeof(CONSOLE_REGISTER_CONSOLEIME));

            RegConIMEInfo = (PCONSOLE_REGISTER_CONSOLEIME)ConsoleInformation;
            RegConIMEInfo->dwConsoleInputThreadId = 0;

            Status = ObReferenceObjectByHandle(
                    RegConIMEInfo->hdesk,
                    0,
                    *ExDesktopObjectType,
                    UserMode,
                    &pdesk,
                    NULL);
            if (!NT_SUCCESS(Status))
                return Status;

            LogDesktop(pdesk, LD_REF_FN_CONSOLECONTROL2, TRUE, (ULONG_PTR)PtiCurrent());

            Status = STATUS_SUCCESS;
            if (pdesk->dwConsoleThreadId)
            {
                /*
                 * Exists console input thread
                 */
                RegConIMEInfo->dwConsoleInputThreadId = pdesk->dwConsoleThreadId;

                dwConsoleIMEThreadIdOld = pdesk->dwConsoleIMEThreadId;

                if (RegConIMEInfo->dwAction != REGCONIME_QUERY) {
                    PTHREADINFO ptiConsoleIME;

                    if ((ptiConsoleIME = PtiFromThreadId(RegConIMEInfo->dwThreadId)) != NULL)
                    {
                        if ( (RegConIMEInfo->dwAction == REGCONIME_REGISTER) &&
                             !(ptiConsoleIME->TIF_flags & TIF_DONTATTACHQUEUE) )
                        {
                            /*
                             * Register
                             */
                            ptiConsoleIME->TIF_flags |= TIF_DONTATTACHQUEUE;
                            pdesk->dwConsoleIMEThreadId = RegConIMEInfo->dwThreadId;
                        }
                        else if ( (RegConIMEInfo->dwAction == REGCONIME_UNREGISTER) &&
                                  (ptiConsoleIME->TIF_flags & TIF_DONTATTACHQUEUE) )
                        {
                            /*
                             * Unregister
                             */
                            ptiConsoleIME->TIF_flags &= ~TIF_DONTATTACHQUEUE;
                            pdesk->dwConsoleIMEThreadId = 0;
                        }
                        else if (RegConIMEInfo->dwAction == REGCONIME_TERMINATE)
                        {
                            /*
                             * Terminate console IME (Logoff/Shutdown)
                             */
                            pdesk->dwConsoleIMEThreadId = 0;
                        }
                    }
                    else if (RegConIMEInfo->dwAction == REGCONIME_TERMINATE)
                    {
                        /*
                         * Abnormal end console IME
                         */
                        pdesk->dwConsoleIMEThreadId = 0;
                    }
                    else
                        Status = STATUS_ACCESS_DENIED;
                }
                RegConIMEInfo->dwThreadId = dwConsoleIMEThreadIdOld;
            }
            LogDesktop(pdesk, LD_DEREF_FN_CONSOLECONTROL2, FALSE, (ULONG_PTR)PtiCurrent());
            ObDereferenceObject(pdesk);
            return Status;
        }
        break;
#endif

    case ConsoleFullscreenSwitch:
        UserAssert(ConsoleInformationLength == sizeof(CONSOLE_FULLSCREEN_SWITCH));
        xxxbFullscreenSwitch(((PCONSOLE_FULLSCREEN_SWITCH)ConsoleInformation)->bFullscreenSwitch,
                             ((PCONSOLE_FULLSCREEN_SWITCH)ConsoleInformation)->hwnd);
        break;

    case ConsoleSetCaretInfo:
        UserAssert(ConsoleInformationLength == sizeof(CONSOLE_CARET_INFO));
        xxxSetConsoleCaretInfo((PCONSOLE_CARET_INFO)ConsoleInformation);
        break;

    default:
        RIPMSG0(RIP_ERROR, "xxxConsoleControl - invalid control class\n");
        UserAssert(FALSE);
        return STATUS_INVALID_INFO_CLASS;
    }
    return STATUS_SUCCESS;
}
