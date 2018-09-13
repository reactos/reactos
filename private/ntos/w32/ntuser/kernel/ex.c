/**************************** Module Header ********************************\
* Module Name: ex.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Executive support routines
*
* History:
* 03-04-95 JimA       Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


NTSTATUS
OpenEffectiveToken(
    PHANDLE phToken)
{
    NTSTATUS Status;

    /*
     * Open the client's token.
     */
    Status = ZwOpenThreadToken(
                 NtCurrentThread(),
                 TOKEN_QUERY,
                 (BOOLEAN)TRUE,     // OpenAsSelf
                 phToken
                 );
    if (Status == STATUS_NO_TOKEN) {

        /*
         * Client wasn't impersonating anyone.  Open its process token.
         */
        Status = ZwOpenProcessToken(
                     NtCurrentProcess(),
                     TOKEN_QUERY,
                     phToken
                     );
    }

    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING, "Can't open client's token! - Status = %lx", Status);
    }
    return Status;
}

NTSTATUS
GetProcessLuid(
    PETHREAD Thread,
    PLUID LuidProcess
    )
{
    PACCESS_TOKEN UserToken = NULL;
    BOOLEAN fCopyOnOpen;
    BOOLEAN fEffectiveOnly;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    NTSTATUS Status;

    if (Thread == NULL)
        Thread = PsGetCurrentThread();

    //
    // Check for a thread token first
    //

    UserToken = PsReferenceImpersonationToken(Thread,
            &fCopyOnOpen, &fEffectiveOnly, &ImpersonationLevel);

    if (UserToken == NULL) {

        //
        // No thread token, go to the process
        //

        UserToken = PsReferencePrimaryToken(Thread->ThreadsProcess);
        if (UserToken == NULL)
            return STATUS_NO_TOKEN;
    }

    Status = SeQueryAuthenticationIdToken(UserToken, LuidProcess);

    //
    // We're finished with the token
    //

    ObDereferenceObject(UserToken);

    return Status;
}


BOOLEAN
IsRestricted(
    PETHREAD Thread
    )
{
    PACCESS_TOKEN UserToken;
    BOOLEAN fCopyOnOpen;
    BOOLEAN fEffectiveOnly;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    BOOLEAN fRestricted = FALSE;

    /*
     * Check for a thread token first.
     */
    UserToken = PsReferenceImpersonationToken(Thread,
            &fCopyOnOpen, &fEffectiveOnly, &ImpersonationLevel);

    /*
     * If no thread token, go to the process.
     */
    if (UserToken == NULL) {
        UserToken = PsReferencePrimaryToken(Thread->ThreadsProcess);
    }

    /*
     * If we got a token, is it restricted?
     */
    if (UserToken != NULL) {
        fRestricted = SeTokenIsRestricted(UserToken);
        ObDereferenceObject(UserToken);
    }

    return fRestricted;
}


NTSTATUS
CreateSystemThread(
    PKSTART_ROUTINE lpThreadAddress,
    PVOID           pvContext,
    PHANDLE         phThread)
{
    NTSTATUS          Status;
    OBJECT_ATTRIBUTES Obja;
    HANDLE            hProcess;

    CheckCritOut();

    InitializeObjectAttributes(&Obja,
                               NULL,
                               0,
                               NULL,
                               NULL);

    /*
     * On WinFrame WIN32K.SYS is in WINSTATION SPACE. We can not
     * allow any system threads to access WIN32K.SYS since
     * this space is not mapped into the system process.
     *
     * We need to access the CSRSS
     * process regardless of who our caller is. IE: We could be called from
     * a CSRSS client who does not have a handle to the CSRSS process in
     * its handle table.
     */
    UserAssert(gpepCSRSS != NULL);

    Status = ObOpenObjectByPointer(
                 gpepCSRSS,
                 0,
                 NULL,
                 PROCESS_CREATE_THREAD,
                 NULL,
                 KernelMode,
                 &hProcess);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    UserAssert(hProcess != NULL);

    Status = PsCreateSystemThread(
                    phThread,
                    THREAD_ALL_ACCESS,
                    &Obja,
                    hProcess,
                    NULL,
                    lpThreadAddress,
                    pvContext);

    ZwClose(hProcess);

    return Status;
}


NTSTATUS
InitSystemThread(
    PUNICODE_STRING pstrThreadName)
{
    PETHREAD pEThread;
    PEPROCESS Process;
    PTHREADINFO pti;
    NTSTATUS Status;

    CheckCritOut();

    pEThread = PsGetCurrentThread();
    Process = pEThread->ThreadsProcess;

    ValidateThreadSessionId(pEThread);

    /*
     * check to see if process is already set, if not, we
     * need to set it up as well
     */
    if (Process->Win32Process == NULL) {
        Status = W32pProcessCallout(Process, TRUE);
        if (!NT_SUCCESS(Status)) {
            return Status;
        }
    }

    /*
     * We have the W32 process (or don't need one). Now get the thread data
     * and the kernel stack
     */
    Status = AllocateW32Thread(pEThread);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    EnterCrit();

    /*
     * Allocate a pti for this thread
     *
     * Flag this as a system thread
     */
    Status = xxxCreateThreadInfo(pEThread, TRUE);
    if (!NT_SUCCESS(Status)) {
        FreeW32Thread(pEThread);
        LeaveCrit();
        return Status;
    }

    pti = PtiCurrentShared();
    if (pstrThreadName) {
        if (pti->pstrAppName != NULL)
            UserFreePool(pti->pstrAppName);
        pti->pstrAppName = UserAllocPoolWithQuota(sizeof(UNICODE_STRING) +
                pstrThreadName->Length + sizeof(WCHAR), TAG_TEXT);
        if (pti->pstrAppName != NULL) {
            pti->pstrAppName->Buffer = (PWCHAR)(pti->pstrAppName + 1);
            RtlCopyMemory(pti->pstrAppName->Buffer, pstrThreadName->Buffer,
                    pstrThreadName->Length);
            pti->pstrAppName->Buffer[pstrThreadName->Length / sizeof(WCHAR)] = 0;
            pti->pstrAppName->MaximumLength = pstrThreadName->Length + sizeof(WCHAR);
            pti->pstrAppName->Length = pstrThreadName->Length;
        }
    }

    /*
     *  Need to clear the W32PF_APPSTARTING bit so that windows created by
     *  the RIT don't cause the cursor to change to the app starting
     *  cursor.
     */
    if ((pti->ppi != NULL) && (pti->ppi->W32PF_Flags & W32PF_APPSTARTING)) {
        ClearAppStarting(pti->ppi);
    }

    LeaveCrit();

    return STATUS_SUCCESS;
}

VOID
UserRtlRaiseStatus(
    NTSTATUS Status)
{
    ExRaiseStatus(Status);
}

NTSTATUS
CommitReadOnlyMemory(
    HANDLE hSection,
    PSIZE_T pCommitSize,
    DWORD  dwCommitOffset,
    int*   pdCommit)
{
    SIZE_T        ulViewSize;
    LARGE_INTEGER liOffset;
    PEPROCESS     Process;
    PVOID         pUserBase, pvt;
    NTSTATUS      Status;

    ulViewSize = 0;
    pUserBase = NULL;
    liOffset.QuadPart = 0;
    Process = PsGetCurrentProcess();

    Status = MmMapViewOfSection(
            hSection,
            Process,
            &pUserBase,
            0,
            PAGE_SIZE,
            &liOffset,
            &ulViewSize,
            ViewUnmap,
            SEC_NO_CHANGE,
            PAGE_EXECUTE_READ);

    if (NT_SUCCESS(Status)) {

        /*
         * Commit the memory
         */
        pUserBase = pvt = (PVOID)((PBYTE)pUserBase + dwCommitOffset);

        Status = ZwAllocateVirtualMemory(
                NtCurrentProcess(),
                &pUserBase,
                0,
                pCommitSize,
                MEM_COMMIT,
                PAGE_EXECUTE_READ);

        if (pdCommit) {
            *pdCommit = (int)((PBYTE)pUserBase - (PBYTE)pvt);
        }
#if DBG
          else {
            UserAssert(pvt == pUserBase);
        }
#endif

        MmUnmapViewOfSection(Process, pUserBase);
    }
    return Status;
}

/***************************************************************************\
* CreateKernelEvent
*
* Creates a kernel event.  This is used when reference counted events
* created by ZwCreateEvent are not needed.
*
* History:
* 06-26-95 JimA             Created.
\***************************************************************************/

PKEVENT CreateKernelEvent(
    IN EVENT_TYPE Type,
    IN BOOLEAN State)
{
    PKEVENT pEvent;

    pEvent = UserAllocPoolNonPaged(sizeof(KEVENT), TAG_SYSTEM);
    if (pEvent != NULL) {
        KeInitializeEvent(pEvent, Type, State);
    }
    return pEvent;
}

/***************************************************************************\
* LockObjectAssignment
*
* References an object into a data structure
*
* History:
* 06-26-95 JimA             Created.
\***************************************************************************/

VOID LockObjectAssignment(
    PVOID *pplock,
    PVOID pobject
#ifdef LOGDESKTOPLOCKS
    ,DWORD tag,
    ULONG_PTR extra
#endif
    )
{
    PVOID pobjectOld;

    /*
     * Save old object to dereference AFTER the new object is
     * referenced.  This will avoid problems with relocking
     * the same object.
     */
    pobjectOld = *pplock;

    /*
     * Reference the new object.
     */
    if (pobject != NULL) {
        ObReferenceObject(pobject);
#ifdef LOGDESKTOPLOCKS
        if (OBJECT_TO_OBJECT_HEADER(pobject)->Type == *ExDesktopObjectType) {
            LogDesktop(pobject, tag, TRUE, extra);
        }
#endif
    }
    *pplock = pobject;

    /*
     * Dereference the old object
     */
    if (pobjectOld != NULL) {
#ifdef LOGDESKTOPLOCKS
        if (OBJECT_TO_OBJECT_HEADER(pobjectOld)->Type == *ExDesktopObjectType) {
            LogDesktop(pobjectOld, tag, FALSE, extra);
        }
#endif
        ObDereferenceObject(pobjectOld);
    }
}

/***************************************************************************\
* UnlockObjectAssignment
*
* Dereferences an object locked into a data structure
*
* History:
* 06-26-95 JimA             Created.
\***************************************************************************/

VOID UnlockObjectAssignment(
    PVOID *pplock
#ifdef LOGDESKTOPLOCKS
    ,DWORD tag,
    ULONG_PTR extra
#endif
    )
{
    if (*pplock != NULL) {
#ifdef LOGDESKTOPLOCKS
        if (OBJECT_TO_OBJECT_HEADER(*pplock)->Type == *ExDesktopObjectType) {
            LogDesktop(*pplock, tag, FALSE, extra);
        }
#endif
        ObDereferenceObject(*pplock);
        *pplock = NULL;
    }
}

/***************************************************************************\
* UserDereferenceObject
*
* We need this for thread locking stuff since ObDereferenceObject is a macro.
*
* 09-21-98 JerrySh          Created.
\***************************************************************************/

VOID UserDereferenceObject(
    PVOID pobj)
{
    ObDereferenceObject(pobj);
}


/***************************************************************************\
* ProtectHandle
*
* This api is used set and clear close protection on handles used
* by the kernel.
*
* 08-18-95 JimA             Created.
\***************************************************************************/

NTSTATUS ProtectHandle(
    IN HANDLE Handle,
    IN BOOLEAN Protect)
{
    OBJECT_HANDLE_FLAG_INFORMATION HandleInfo;
    NTSTATUS Status;

    Status = ZwQueryObject(
            Handle,
            ObjectHandleFlagInformation,
            &HandleInfo,
            sizeof(HandleInfo),
            NULL);
    if (NT_SUCCESS(Status)) {
        HandleInfo.ProtectFromClose = Protect;

        Status = ZwSetInformationObject(
                Handle,
                ObjectHandleFlagInformation,
                &HandleInfo,
                sizeof(HandleInfo));
    }

    return Status;
}

#ifdef LOGDESKTOPLOCKS

/***************************************************************************\
* LogDesktop
*
* Log the lock/unlock calls for desktop objects
*
* Dec-2-97 clupu            Created.
\***************************************************************************/

#define LOG_DELTA   8

PLogD GrowLogIfNecessary(
    PDESKTOP pdesk)
{
    if (pdesk->nLogCrt < pdesk->nLogMax) {
        UserAssert(pdesk->pLog != NULL);
        return pdesk->pLog;
    }

    /*
     * Grow the buffer
     */
    if (pdesk->pLog == NULL) {

        UserAssert(pdesk->nLogMax == 0 && pdesk->nLogCrt == 0);

        pdesk->pLog = (PLogD)UserAllocPool(LOG_DELTA * sizeof(LogD), TAG_LOGDESKTOP);

    } else {
        pdesk->pLog = (PLogD)UserReAllocPool(pdesk->pLog,
                                             pdesk->nLogCrt * sizeof(LogD),
                                             (pdesk->nLogCrt + LOG_DELTA) * sizeof(LogD),
                                             TAG_LOGDESKTOP);
    }

    UserAssert(pdesk->pLog != NULL);

    pdesk->nLogMax += LOG_DELTA;

    return pdesk->pLog;
}


void LogDesktop(
    PDESKTOP pdesk,
    DWORD    tag,
    BOOL     bLock,
    ULONG_PTR extra)
{
    DWORD tag1 = 0, tag2 = 0;
    PLogD pLog;

    if (pdesk == NULL) {
        return;
    }

    /*
     * the tag stored in LogD structure is actually a WORD
     */
    UserAssert(HIWORD(tag) == 0);

    if (bLock) {

        ULONG hash;

        (pdesk->nLockCount)++;

growAndAdd:
        /*
         * grow the table if necessary and add the new
         * lock/unlock information to it
         */
        pLog = GrowLogIfNecessary(pdesk);

        pLog += pdesk->nLogCrt;

        pLog->tag   = (WORD)tag;
        pLog->type  = (WORD)bLock;
        pLog->extra = extra;

        RtlZeroMemory(pLog->trace, 6 * sizeof(PVOID));

        GetStackTrace(2,
                      6,
                      pLog->trace,
                      &hash);

        (pdesk->nLogCrt)++;
        return;
    }

    /*
     * It's an unlock.
     * First search for a matching lock
     */
    UserAssert(pdesk->nLockCount > 0);

    switch (tag) {
    case LDU_CLS_DESKPARENT1:
        tag1 = LDL_CLS_DESKPARENT1;
        break;
    case LDU_CLS_DESKPARENT2:
        tag1 = LDL_CLS_DESKPARENT1;
        tag2 = LDL_CLS_DESKPARENT2;
        break;
    case LDU_FN_DESTROYCLASS:
        tag1 = LDL_FN_DESTROYCLASS;
        break;
    case LDU_FN_DESTROYMENU:
        tag1 = LDL_FN_DESTROYMENU;
        break;
    case LDU_FN_DESTROYTHREADINFO:
        tag1 = LDL_FN_DESTROYTHREADINFO;
        break;
    case LDU_FN_DESTROYWINDOWSTATION:
        tag1 = LDL_FN_DESTROYWINDOWSTATION;
        break;
    case LDU_DESKDISCONNECT:
        tag1 = LDL_DESKDISCONNECT;
        break;
    case LDU_DESK_DESKNEXT:
        tag1 = LDL_DESK_DESKNEXT1;
        break;
    case LDU_OBJ_DESK:
        tag1 = LDL_OBJ_DESK;
        tag2 = LDL_MOTHERDESK_DESK1;
        break;
    case LDL_PTI_DESK:
        tag1 = LDL_PTI_DESK;
        tag2 = LDL_DT_DESK;
        break;
    case LDU_PTI_DESK:
        tag1 = LDL_PTI_DESK;
        break;
    case LDU_PPI_DESKSTARTUP1:
    case LDU_PPI_DESKSTARTUP2:
    case LDU_PPI_DESKSTARTUP3:
        tag1 = LDL_PPI_DESKSTARTUP1;
        tag2 = LDL_PPI_DESKSTARTUP2;
        break;
    case LDU_DESKLOGON:
        tag1 = LDL_DESKLOGON;
        break;

    case LDUT_FN_FREEWINDOW:
        tag1 = LDLT_FN_FREEWINDOW;
        break;
    case LDUT_FN_DESKTOPTHREAD_DESK:
        tag1 = LDLT_FN_DESKTOPTHREAD_DESK;
        break;
    case LDUT_FN_DESKTOPTHREAD_DESKTEMP:
        tag1 = LDLT_FN_DESKTOPTHREAD_DESKTEMP;
        break;
    case LDUT_FN_SETDESKTOP:
        tag1 = LDLT_FN_SETDESKTOP;
        break;
    case LDUT_FN_NTUSERSWITCHDESKTOP:
        tag1 = LDLT_FN_NTUSERSWITCHDESKTOP;
        break;
    case LDUT_FN_SENDMESSAGEBSM1:
    case LDUT_FN_SENDMESSAGEBSM2:
        tag1 = LDLT_FN_SENDMESSAGEBSM;
        break;
    case LDUT_FN_SYSTEMBROADCASTMESSAGE:
        tag1 = LDLT_FN_SYSTEMBROADCASTMESSAGE;
        break;
    case LDUT_FN_CTXREDRAWSCREEN:
        tag1 = LDLT_FN_CTXREDRAWSCREEN;
        break;
    case LDUT_FN_CTXDISABLESCREEN:
        tag1 = LDLT_FN_CTXDISABLESCREEN;
        break;

    case LD_DEREF_FN_CREATEDESKTOP1:
    case LD_DEREF_FN_CREATEDESKTOP2:
    case LD_DEREF_FN_CREATEDESKTOP3:
        tag1 = LD_REF_FN_CREATEDESKTOP;
        break;
    case LD_DEREF_FN_OPENDESKTOP:
        tag1 = LD_REF_FN_OPENDESKTOP;
        break;
    case LD_DEREF_FN_SETDESKTOP:
        tag1 = LD_REF_FN_SETDESKTOP;
        break;
    case LD_DEREF_FN_GETTHREADDESKTOP:
        tag1 = LD_REF_FN_GETTHREADDESKTOP;
        break;
    case LD_DEREF_FN_CLOSEDESKTOP1:
    case LD_DEREF_FN_CLOSEDESKTOP2:
        tag1 = LD_REF_FN_CLOSEDESKTOP;
        break;
    case LD_DEREF_FN_RESOLVEDESKTOP:
        tag1 = LD_REF_FN_RESOLVEDESKTOP;
        break;
    case LD_DEREF_VALIDATE_HDESK1:
    case LD_DEREF_VALIDATE_HDESK2:
    case LD_DEREF_VALIDATE_HDESK3:
    case LD_DEREF_VALIDATE_HDESK4:
        tag1 = LDL_VALIDATE_HDESK;
        break;
    case LDUT_FN_CREATETHREADINFO1:
    case LDUT_FN_CREATETHREADINFO2:
        tag1 = LDLT_FN_CREATETHREADINFO;
        break;
    case LD_DEREF_FN_SETCSRSSTHREADDESKTOP1:
    case LD_DEREF_FN_SETCSRSSTHREADDESKTOP2:
        tag1 = LD_REF_FN_SETCSRSSTHREADDESKTOP;
        break;
    case LD_DEREF_FN_CONSOLECONTROL1:
        tag1 = LD_REF_FN_CONSOLECONTROL1;
        break;
    case LD_DEREF_FN_CONSOLECONTROL2:
        tag1 = LD_REF_FN_CONSOLECONTROL2;
        break;
    case LD_DEREF_FN_GETUSEROBJECTINFORMATION:
        tag1 = LD_REF_FN_GETUSEROBJECTINFORMATION;
        break;
    case LD_DEREF_FN_SETUSEROBJECTINFORMATION:
        tag1 = LD_REF_FN_SETUSEROBJECTINFORMATION;
        break;
    case LD_DEREF_FN_CREATEWINDOWSTATION:
        tag1 = LD_REF_FN_CREATEWINDOWSTATION;
        break;

    case LDL_TERM_DESKDESTROY1:
        tag1 = LDL_TERM_DESKDESTROY2;
        break;
    case LDL_MOTHERDESK_DESK1:
        tag1 = LDL_MOTHERDESK_DESK1;
        tag2 = LDL_MOTHERDESK_DESK2;
        break;
    case LDL_WINSTA_DESKLIST2:
        tag1 = LDL_WINSTA_DESKLIST1;
        break;
    case LDL_DESKRITINPUT:
    case LDU_DESKRITINPUT:
        tag1 = LDL_DESKRITINPUT;
        break;
    }

    if (tag1 != 0) {

        int ind;

        /*
         * this is an unlock we know about. Let's find the
         * matching lock in the table. We start searching
         * the table backwords.
         */
        for (ind = pdesk->nLogCrt - 1; ind >= 0; ind--) {
            pLog = pdesk->pLog + ind;

            if (pLog->type == 1 &&
                (pLog->tag == tag1 || pLog->tag == tag2) &&
                pLog->extra == extra) {

                /*
                 * match found. remove the lock
                 */
                RtlMoveMemory(pdesk->pLog + ind,
                              pdesk->pLog + ind + 1,
                              (pdesk->nLogCrt - ind - 1) * sizeof(LogD));

                (pdesk->nLogCrt)--;

                (pdesk->nLockCount)--;

                if (pdesk->nLockCount == 0) {
                    RIPMSG1(RIP_VERBOSE, "Lock count 0 for pdesk %#p\n", pdesk);
                }

                return;
            }
        }

        /*
         * We didn't find the matching lock and we were supposed to.
         * Just add it to the table and we'll look at it.
         */
        RIPMSG3(RIP_WARNING, "Didn't find matching lock for pdesk %#p tag %d extra %lx\n",
                pdesk, tag, extra);
    }
    (pdesk->nLockCount)--;

    goto growAndAdd;

    return;
}

#endif // LOGDESKTOPLOCKS
