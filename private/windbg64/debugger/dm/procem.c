/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    procem.c

Abstract:


Author:


Environment:

    NT 3.1

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#ifndef KERNEL
#include <crash.h>
extern BOOL CrashDump;
#endif

#include "dmsql.h"

#include "resource.h"

SetFile()


extern HPRCX        prcList;
extern CRITICAL_SECTION csThreadProcList;

extern METHOD       EMNotifyMethod;

extern CRITICAL_SECTION csProcessDebugEvent;
extern HANDLE hEventCreateProcess;
extern HANDLE hEventRemoteQuit;
extern HANDLE hEventContinue;

extern LPDM_MSG     LpDmMsg;
extern BYTE abEMReplyBuf[];

extern HPID         hpidRoot;
extern BOOL         fUseRoot;
extern BOOL         fDisconnected;

extern SYSTEM_INFO  SystemInfo;

#define EFDDEFAULT efdNotify
EXCEPTION_FILTER_DEFAULT EfdDefault = EFDDEFAULT;

extern char       nameBuffer[];


VOID
ProcessIoctlGenericCmd(
    HPRCX   hprc,
    HTHDX   hthd,
    LPDBB   lpdbb
    );

VOID
ProcessIoctlCustomCmd(
    HPRCX   hprc,
    HTHDX   hthd,
    LPDBB   lpdbb
    );

XOSD
GetThreadSuspendCount(
    HTHDX hthd,
    LPDWORD lpdwSuspendCount
    );

#ifdef KERNEL
extern BOOL       DmKdBreakIn;
#endif

#if 0
BOOL    fSmartRangeStep = TRUE;
#else
BOOL    fSmartRangeStep = FALSE;
#endif


void
ActionRemoveBP(
    DEBUG_EVENT64* de,
    HTHDX hthd,
    DWORD unused,
    DWORDLONG lparam
    )
{
    Unreferenced( de );
    Unreferenced( hthd );

    RemoveBP((PBREAKPOINT)lparam);
}




VOID
ProcessCreateProcessCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
/*++

Routine Description:

    Create a process requested by the EM.

Arguments:

    hprc   -

    hthd   -

    lpdbb  -

Return Value:

    None.

--*/

{
    XOSD       xosd = xosdNone;

    Unreferenced( hprc );
    Unreferenced( hthd );


    DEBUG_PRINT_2(
        "ProcessCreateProcessCmd called with HPID=%p, (sizeof(HPID)=%d)",
        lpdbb->hpid, sizeof(HPID));

    hpidRoot = lpdbb->hpid;
    fUseRoot = TRUE;

    Reply(0, &xosd, lpdbb->hpid);

#ifndef KERNEL
    //if (fDisconnected) {
        //SetEvent( hEventRemoteQuit );
    //}
#endif

    return;
}                               /* ProcessCreateProcessCmd() */

DWORD
ProcessProcStatCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    LPPST lppst = (LPPST)LpDmMsg->rgb;

    Unreferenced( lpdbb );

    DEBUG_PRINT("ProcessProcStatCmd\n");

    lppst->dwProcessID = hprc->pid;
    if (hprc->pid <= 99999) {
        sprintf(lppst->rgchProcessID, "%5d", hprc->pid);
    } else {
        sprintf(lppst->rgchProcessID, "0x%08x", hprc->pid);
    }


    /*
     *  Check if any of this process's threads are running
     */

    if (hprc->pstate & ps_exited) {
        lppst->dwProcessState = pstExited;
        _tcscpy(lppst->rgchProcessState, "Exited");
    } else if (hprc->pstate & ps_dead) {
        lppst->dwProcessState = pstDead;
        _tcscpy(lppst->rgchProcessState, "Dead");
    } else {
        lppst->dwProcessState = pstRunning;
        _tcscpy(lppst->rgchProcessState, "Running");

        EnterCriticalSection(&csThreadProcList);
        for (hthd = (HTHDX)hprc->hthdChild;hthd;hthd=hthd->nextSibling) {
            if (hthd->tstate & ts_stopped) {
                lppst->dwProcessState = pstStopped;
                _tcscpy(lppst->rgchProcessState, "Stopped");
                break;
            }
        }
        LeaveCriticalSection(&csThreadProcList);
    }

    return sizeof(PST);
}

XOSD
GetThreadSuspendCount(
    HTHDX hthd,
    LPDWORD lpdwSuspendCount
    )
{
    DWORD dw = 0;
    XOSD xosd = xosdNone;

    assert(lpdwSuspendCount);
    assert(hthd != NULL);

    dw = SuspendThread(hthd->rwHand);
    if (dw != 0xffffffff) {
        ResumeThread(hthd->rwHand);
    } else {
        switch (GetLastError()) {
          case (DWORD)STATUS_SUSPEND_COUNT_EXCEEDED:
            dw = MAXIMUM_SUSPEND_COUNT;
            break;

          case (DWORD)STATUS_THREAD_IS_TERMINATING:
            dw = 0;
            break;

          default:
            dw = 0;
            xosd = xosdBadThread;
        }
    }

    *lpdwSuspendCount = dw;
    return xosd;
}


DWORD
ProcessThreadStatCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    LPTST       lptst = (LPTST) LpDmMsg->rgb;
    XOSD        xosd = xosdNone;

#ifndef KERNEL
    typedef NTSTATUS (* QTHREAD)(HANDLE,THREADINFOCLASS,PVOID,ULONG,PULONG);

    NTSTATUS                   Status;
    THREAD_BASIC_INFORMATION   ThreadBasicInfo;
    QTHREAD                    Qthread;
    DWORD                      dw;
#endif  // KERNEL


    Unreferenced( hprc );
    DEBUG_PRINT("ProcessThreadStatCmd : ");

    if (!hthd) {
        WaitForSingleObject(hprc->hEventCreateThread, INFINITE);
        hthd = HTHDXFromHPIDHTID(lpdbb->hpid, lpdbb->htid);

        //
        // this happens when a stale thread gets queried.
        // no need to fix the race, just fail.
        //
        //assert(hthd != 0);
        if (!hthd) {
            LpDmMsg->xosdRet = xosdBadThread;
            return sizeof(TST);
        }
    }

    ZeroMemory(lptst, sizeof(TST));

#ifdef KERNEL

    lptst->dwThreadID = hthd->tid;
    if (lptst->dwThreadID <= 99999) {
        sprintf(lptst->rgchThreadID, "%5d", lptst->dwThreadID);
    } else {
        sprintf(lptst->rgchThreadID, "0x%08x", lptst->dwThreadID);
    }

    lptst->dwSuspendCount = 0;
    lptst->dwSuspendCountMax = 0;
    lptst->dwPriority = 1;
    lptst->dwPriorityMax = 1;
    sprintf(lptst->rgchPriority, "%2d", lptst->dwPriority);

#else // !KERNEL

    if (CrashDump) {
        lptst->dwThreadID = hthd->CrashThread.ThreadId;
    } else {
        lptst->dwThreadID = hthd->tid;
    }

    if (lptst->dwThreadID <= 99999) {
        sprintf(lptst->rgchThreadID, "%5d", lptst->dwThreadID);
    } else {
        sprintf(lptst->rgchThreadID, "0x%08x", lptst->dwThreadID);
    }

    if (CrashDump) {
        lptst->dwSuspendCount = hthd->CrashThread.SuspendCount;
    } else {
        xosd = GetThreadSuspendCount(hthd, &lptst->dwSuspendCount);
    }

    lptst->dwSuspendCountMax = MAXIMUM_SUSPEND_COUNT;

    if (CrashDump) {
        dw = hthd->CrashThread.PriorityClass;
    } else {
        dw = GetPriorityClass(hprc->rwHand);
    }

    if (!dw) {

        xosd = xosdBadThread;

    } else {

        switch (dw) {

          case IDLE_PRIORITY_CLASS:
            lptst->dwPriority = 4;
            lptst->dwPriorityMax = 15;
            break;

          case NORMAL_PRIORITY_CLASS:
            lptst->dwPriority = 9;
            lptst->dwPriorityMax = 15;
            break;

          case HIGH_PRIORITY_CLASS:
            lptst->dwPriority = 13;
            lptst->dwPriorityMax = 15;
            break;

          case REALTIME_PRIORITY_CLASS:
            lptst->dwPriority = 4;
            lptst->dwPriorityMax = 31;
            break;
        }

        if (CrashDump) {
            dw = hthd->CrashThread.Priority;
        } else {
            dw = GetThreadPriority(hthd->rwHand);
        }
        if (dw == THREAD_PRIORITY_ERROR_RETURN) {
            xosd = xosdBadThread;
        } else {
            lptst->dwPriority += dw;
            if ((long)lptst->dwPriority > (long)lptst->dwPriorityMax) {
                lptst->dwPriority = lptst->dwPriorityMax;
            } else if ((long)lptst->dwPriority < (long)(lptst->dwPriorityMax - 15)) {
                lptst->dwPriority = lptst->dwPriorityMax - 15;
            }
            sprintf(lptst->rgchPriority, "%2d", lptst->dwPriority);
        }
    }

#endif // !KERNEL

    if        (hthd->tstate & ts_running) {
        lptst->dwState = tstRunning;
        _tcscpy(lptst->rgchState, "Running");
    } else if (hthd->tstate & ts_stopped) {
        lptst->dwState = tstStopped;
        if (hthd->tstate & ts_frozen) {
            lptst->dwSuspendCount = 1;
        }
        _tcscpy(lptst->rgchState, "Stopped");
    } else if (hthd->tstate & ts_dead) {
        lptst->dwState = tstExiting;
        _tcscpy(lptst->rgchState, "Exiting");
    } else if (hthd->tstate & ts_destroyed) {
        lptst->dwState = tstDead;
        _tcscpy(lptst->rgchState, "Dead");
    } else {
        lptst->dwState = tstRunnable;
        _tcscpy(lptst->rgchState, "Pre-run");
    }


    if (hthd->tstate & ts_rip ) {
        lptst->dwState |= tstRip;
        _tcscat(lptst->rgchState, ", RIPped");
    } else if (hthd->tstate & ts_first) {
        lptst->dwState |= tstExcept1st;
        _tcscat(lptst->rgchState, ", 1st chance");
    } else if (hthd->tstate & ts_second) {
        lptst->dwState |= tstExcept2nd;
        _tcscat(lptst->rgchState, ", 2nd chance");
    }


    if (hthd->tstate & ts_frozen) {
        lptst->dwState |= tstFrozen;
        _tcscat(lptst->rgchState, ", suspended");
    }

    lptst->dwTeb = 0;
#ifndef KERNEL
    if (CrashDump) {
        lptst->dwTeb = hthd->CrashThread.Teb;
    } else {
        Qthread = (QTHREAD)GetProcAddress( GetModuleHandle( "ntdll.dll" ),
                                           "NtQueryInformationThread" );
        if (Qthread) {
            Status = Qthread( hthd->rwHand,
                             ThreadBasicInformation,
                             &ThreadBasicInfo,
                             sizeof(ThreadBasicInfo),
                             NULL
                            );
            if (NT_SUCCESS(Status)) {
                lptst->dwTeb = (DWORDLONG)ThreadBasicInfo.TebBaseAddress;
            }
        }
    }
#endif  // !KERNEL

    LpDmMsg->xosdRet = xosd;
    return sizeof(TST);
}


/***    UnpackLoadCmd
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**      Crack the load command from the debugger
*/
BOOL
UnpackLoadCmd(
    CONST LPVOID lpv,
    LPPRL lpprl
    )
{
    BYTE * lpb = lpv;

    if ( !lpv || !lpprl ) {
        return FALSE;
    }

    lpprl->dwChildFlags = *(DWORD *)lpb;
    lpb += sizeof(DWORD);

    lpprl->lszRemoteExe = lpb;
    lpb += _tcslen(lpb) + 1;

    lpprl->lszCmdLine = lpb;
    lpb += _tcslen(lpb) + 1;

    lpprl->lszRemoteDir = lpb;
    lpb += _tcslen(lpb) + 1;

    if (*lpb) {
        lpprl -> lpso = (LPSPAWNORPHAN)lpb;
    } else {
        lpprl -> lpso = NULL;
    }

    return TRUE;
}


/***    ProcessLoadCmd
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**      Process a load command from the debugger
*/

void
ProcessProgLoadCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
#ifdef KERNEL

    PRL         prl;
    LPSTR       p;
    char        progname[MAX_PATH];
    char        fname[_MAX_FNAME];
    char        ext[_MAX_EXT];

    if (fDisconnected) {
        DmKdBreakIn = TRUE;
        SetEvent( hEventRemoteQuit );
        LpDmMsg->xosdRet = xosdNone;
        Reply(0, LpDmMsg, lpdbb->hpid);
        return;
    }

    if (!UnpackLoadCmd(lpdbb->rgbVar, &prl)) {
        assert(FALSE); // Can't deal with failure
    }


    _splitpath( prl.lszRemoteExe, NULL, NULL, fname, ext );
    if (_tcsicmp(ext,"exe") != 0) {
        _tcscpy(ext, "exe" );
    }
    _makepath( progname, NULL, NULL, fname, ext );

    if ((_tcsicmp(progname,KERNEL_IMAGE_NAME)==0) ||
        (_tcsicmp(progname,OSLOADER_IMAGE_NAME)==0)) {

        if (!DmKdConnectAndInitialize( progname )) {
            LpDmMsg->xosdRet = xosdFileNotFound;
        } else {
            LpDmMsg->xosdRet = xosdNone;
        }

    } else {
        LpDmMsg->xosdRet = xosdFileNotFound;
    }

    Reply(0, LpDmMsg, lpdbb->hpid);
    return;

#else   // !KERNEL

    char **     szEnvironment=NULL;
    DWORD       dwCreationFlags;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    XOSD        xosd;
    PRL         prl;
    HPRCX       hprc1;
    HPRCX       hprcT;



    assert(!fDisconnected);


    if (!UnpackLoadCmd(lpdbb->rgbVar, &prl)) {
        assert(FALSE); // Can't deal with failure
    }

    assert (lpdbb -> dmf == dmfProgLoad);

     /*
     * For various strange reasons the list of processes may not have
     * been completely cleared.  If not do so now
     */

    for (hprc1 = prcList; hprc1 != hprcxNull; hprc1 = hprcT) {
        hprcT = hprc1->next;

        if (hprc1->pstate & ps_dead) {
            FreeProcess( hprc1, FALSE );
        }
    }


    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);

    si.dwFlags = STARTF_USESHOWWINDOW;

    switch ( prl.dwChildFlags & (ulfMinimizeApp | ulfNoActivate) )
    {
      case 0:
        si.wShowWindow = SW_SHOWNORMAL;
        break;
      case ulfMinimizeApp:
        si.wShowWindow = SW_SHOWMINIMIZED;
        break;
      case ulfNoActivate:
        si.wShowWindow = SW_SHOWNOACTIVATE;
        break;
      case (ulfMinimizeApp | ulfNoActivate):
        si.wShowWindow = SW_SHOWMINNOACTIVE;
        break;
    }

    dwCreationFlags = (prl.dwChildFlags & ulfMultiProcess)?
                         DEBUG_PROCESS :
                         DEBUG_ONLY_THIS_PROCESS;

    if (prl.dwChildFlags & ulfWowVdm) {
        dwCreationFlags |= CREATE_SEPARATE_WOW_VDM;
    }

    DPRINT(3,("Load Program: \"%s\"  HPRC=0x%p\n",
                  prl.lszRemoteExe, hprc));

        DMSqlPreLoad( prl.dwChildFlags );

    xosd = Load(hprc,
                prl.lszRemoteExe,
                prl.lszCmdLine,
                (LPVOID)0,                // &lc->processAttributes,
                (LPVOID)0,                // &lc->threadAttributes,
                dwCreationFlags,
                (prl.dwChildFlags & ulfInheritHandles) != 0,
                szEnvironment,
                prl.lszRemoteDir,
                &si,
                &pi);

    if (!fUseRoot || xosd != xosdNone) {
        Reply(0, &xosd, lpdbb->hpid);
    }

    /*
    **  If the load failed then we need to reply right now.  Otherwise
    **  we will delay the reply until we get the All Dlls loaded exception.
    */

    if (!fUseRoot || xosd != xosdNone) {
        Reply(0, &xosd, lpdbb->hpid);
    }

    return;
#endif // !KERNEL
}

#ifndef KERNEL
void
ProcessSpawnOrphanCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    STARTUPINFO             si;
    PROCESS_INFORMATION     pi;
    SOS                     sos;
    XOSD                    xosd;
    DWORD                   dwCreationFlags = 0;
    HPRCX                   hprc1;
    HPRCX                   hprcT;

    fDisconnected = FALSE;

    if (!UnpackLoadCmd(lpdbb->rgbVar, &sos)) {
        assert(FALSE); // Can't deal with failure
    }

    assert (lpdbb -> dmf == dmfSpawnOrphan);

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);

    si.dwFlags = STARTF_USESHOWWINDOW;

    switch ( sos.dwChildFlags & (ulfMinimizeApp | ulfNoActivate) ) {
      case 0:
        si.wShowWindow = SW_SHOWNORMAL;
        break;
      case ulfMinimizeApp:
        si.wShowWindow = SW_SHOWMINIMIZED;
        break;
      case ulfNoActivate:
        si.wShowWindow = SW_SHOWNOACTIVATE;
        break;
      case (ulfMinimizeApp | ulfNoActivate):
        si.wShowWindow = SW_SHOWMINNOACTIVE;
        break;
    }

    if (sos.dwChildFlags & ulfWowVdm) {
        dwCreationFlags |= CREATE_SEPARATE_WOW_VDM;
    }

    DEBUG_PRINT_2("Spawn Orphan: \"%s\"  HPRC=0x%p\n",
                  sos.lszRemoteExe, hprc);

    xosd = Load(hprc,
                sos.lszRemoteExe,
                sos.lszCmdLine,
                (LPVOID)0,                // &lc->processAttributes,
                (LPVOID)0,                // &lc->threadAttributes,
                dwCreationFlags,
                (sos.dwChildFlags & ulfInheritHandles) != 0,
                NULL,                   // szEnvironment
                sos.lszRemoteDir,
                &si,
                &pi);

    if (xosd == xosdNone) {
        sos.lpso -> dwPid = pi.dwProcessId;
        LoadString (hInstance, IDS_EXECUTE_OK, sos.lpso -> rgchErr, sizeof (sos.lpso -> rgchErr));
    } else {
        sos.lpso -> dwPid = 0;
        LoadString (hInstance, IDS_EXECUTE_FAILED, sos.lpso -> rgchErr, sizeof (sos.lpso -> rgchErr));
    }
    memcpy (LpDmMsg -> rgb, sos.lpso, sizeof (SPAWNORPHAN));
    LpDmMsg->xosdRet = xosd;
    Reply (sizeof (SPAWNORPHAN), LpDmMsg, lpdbb -> hpid);

    return;


} // ProcessSpawnOrphanCmd
#endif

DWORD
ProcessUnloadCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    DEBUG_EVENT64 devent, *pde=&devent;
    HTHDXSTRUCT tHthdS;
    HTHDX       hthdT;

    Unreferenced( lpdbb );

    DEBUG_PRINT("ProcessUnloadCmd called.\n");

    /*
     * Verify we got a valid HPRCX
     */

    if (!hprc) {
        return FALSE;
    }

    if (hprc->pstate != (ps_root | ps_destroyed)) {

        if (hprc->hthdChild != 0) {
            tHthdS = *((HTHDX)(hprc->hthdChild));
        } else {
            memset( &tHthdS, 0, sizeof( HTHDXSTRUCT ) );
            tHthdS.hprc   = hprc;
            tHthdS.rwHand = (HANDLE)-1;
        }

        //
        // Send destruction notifications to the shell.
        //

        pde->dwDebugEventCode = DESTROY_THREAD_DEBUG_EVENT;
        pde->dwProcessId      = hprc->pid;

        EnterCriticalSection(&csThreadProcList);

        for (hthd = hprc->hthdChild; hthd; hthd = hthdT){

            hthdT = hthd->nextSibling;

            //
            // Don't send destroys for threads that the shell
            // has already seen destroys for.
            //

            //
            // this used to close the handle for the thread,
            // which was bad; the debugger does not own the
            // handle, it is owned by the session manager.
            //

            if (!(hthd->tstate & ts_destroyed)) {
                pde->dwThreadId = hthd->tid;
                NotifyEM(pde, hthd, 0, (DWORDLONG)hprc);
            }
            FreeHthdx(hthd);
        }

        hprc->hthdChild = NULL;

        LeaveCriticalSection(&csThreadProcList);

        pde->dwDebugEventCode = EXIT_PROCESS_DEBUG_EVENT;
        pde->u.ExitProcess.dwExitCode = hprc->dwExitCode;
        NotifyEM(pde, &tHthdS, 0, (DWORDLONG)hprc);

        pde->dwDebugEventCode = DESTROY_PROCESS_DEBUG_EVENT;
        NotifyEM(pde, &tHthdS, 0, (DWORDLONG)hprc);

    }

    return TRUE;
}                              /* ProcessUnloadCmd() */


XOSD
FreeProcess(
    HPRCX hprc,
    BOOL  fKillRoot
    )
{
    HPRCX               chp;
    HPRCX *             pphp;
    PBREAKPOINT         pbp;
    PBREAKPOINT         pbpT;
    int                 iDll;

    EnterCriticalSection(&csThreadProcList);

    pphp = &prcList->next;
    chp = *pphp;

    while (chp) {
        if (chp != hprc) {
            pphp = &chp->next;
        } else {
#ifndef KERNEL
            if (chp->rwHand != (HANDLE)INVALID && chp->CloseProcessHandle) {
                CloseHandle(chp->rwHand);
                chp->rwHand = (HANDLE)INVALID;
            }
#endif
            RemoveExceptionList(chp);

            for (pbp = BPNextHprcPbp(hprc, NULL); pbp; pbp = pbpT) {
                pbpT = BPNextHprcPbp(hprc, pbp);
                RemoveBP(pbp);
            }

            for (iDll = 0; iDll < chp->cDllList; iDll++) {
                DestroyDllLoadItem(&chp->rgDllList[iDll]);
            }
            MHFree(chp->rgDllList);

            if (!fKillRoot && (chp->pstate & ps_root)) {
                chp->pid    = (PID)-1;
                chp->pstate = ps_root | ps_destroyed;
                ResetEvent(chp->hExitEvent);
                pphp = &chp->next;
            } else {
                CloseHandle(chp->hExitEvent);
                CloseHandle(chp->hEventCreateThread);
                *pphp = chp->next;
                MHFree(chp);
            }
        }
        chp = *pphp;
    }

    /*
     * special case:
     * if everything has been deleted except for the "sticky"
     * root process, delete it now, and set fUseRoot.
     * The hpid remains the same.  If that changes, the EM needs
     * to send a DestroyPid/CreatePid to change it here.
     */
    if (prcList->next
          && prcList->next->next == NULL
            && prcList->next->pstate == (ps_root | ps_destroyed)) {

        CloseHandle(prcList->next->hExitEvent);
        CloseHandle(prcList->next->hEventCreateThread);
        MHFree(prcList->next);
        prcList->next = NULL;
        fUseRoot = TRUE;
    }


    LeaveCriticalSection(&csThreadProcList);

    return xosdNone;
}                               /* FreeProcess() */


XOSD
HandleWatchpoints(
    HPRCX hprcx,
    BOOL fSet,
    LPBPIS lpbpis,
    PDWORDLONG lpqwNotification
    )
{
    BOOL fRet;
    ADDR addr = {0};
    DWORD cb;
    HTHDX hthdx;
    XOSD        xosd  = xosdNone;
    PBREAKPOINT pbp;
    HANDLE      hWalk;

    assert(lpqwNotification != 0);

    hthdx = !lpbpis->fOneThd ? 0:
                  HTHDXFromHPIDHTID(hprcx->hpid, lpbpis->htid);

    switch ( lpbpis->bptp ) {

      case bptpDataR:
      case bptpDataW:
      case bptpDataC:
      case bptpDataExec:
      case bptpRange:

        if (lpbpis->bptp == bptpRange) {
            addr = lpbpis->rng.addr;
            cb = lpbpis->rng.cb;
        } else {
            addr = lpbpis->data.addr;
            cb = lpbpis->data.cb;
        }

        if (cb != 0) {
            fRet = ADDR_IS_FLAT(addr) ||
                         TranslateAddress(hprcx, hthdx, &addr, TRUE);
            assert(fRet);
            if (!fRet) {
                xosd = xosdBadAddress;
                break;
            }
        }

        if (fSet) {
            hWalk = SetWalk(hprcx,
                            hthdx,
                            GetAddrOff(addr),
                            cb,
                            lpbpis->bptp );
            if (!hWalk) {
                xosd = xosdUnknown;
            } else {
                pbp = GetNewBp(hprcx,
                               hthdx,
                               lpbpis->bptp,
                               lpbpis->bpns,
                               NULL,
                               NULL,
                               NULL);
                assert(pbp);
                pbp->hWalk = hWalk;
                AddBpToList(pbp);
                *lpqwNotification = (DWORDLONG)pbp;
            }
        } else {

            assert(*lpqwNotification != 0);

            if (!RemoveBP((PBREAKPOINT)*lpqwNotification)) {
                xosd = xosdUnknown;
            }
        }

        break;

      case bptpRegC:
      case bptpRegR:
      case bptpRegW:

      default:

        xosd = xosdUnsupported;
        break;
    }

    return xosd;
}


XOSD
HandleBreakpoints(
    HPRCX   hprcx,
    BOOL    fSet,
    LPBPIS  lpbpis,
    PDWORDLONG lpqwNotification
    )
{
    LPADDR lpaddr;
    HTHDX hthdx;
    BREAKPOINT  *bp;
    XOSD xosd = xosdNone;

    assert(lpqwNotification != NULL);

    switch (lpbpis->bptp) {
      case bptpExec:
        lpaddr = &lpbpis->exec.addr;
        break;

      case bptpMessage:
        lpaddr = &lpbpis->msg.addr;
        break;

      case bptpMClass:
        assert (FALSE);
        lpaddr = &lpbpis->mcls.addr;
        break;
    }

    if (fSet) {
        DPRINT(1, ("Set a breakpoint: %d @%p:%x:%I64x",
               ADDR_IS_FLAT(*lpaddr), 
               lpaddr->emi,
               lpaddr->addr.seg, 
               lpaddr->addr.off));

        hthdx = lpbpis->fOneThd? 0 :
                               HTHDXFromHPIDHTID(hprcx->hpid, lpbpis->htid);

        bp = SetBP(hprcx, hthdx, lpbpis->bptp, lpbpis->bpns, lpaddr, 0);

        if (bp == NULL) {
            xosd = xosdUnknown;
        } else {
            *lpqwNotification = (DWORDLONG)bp;
        }

    } else {

        DEBUG_PRINT("Clear a breakpoint");

        hthdx = lpbpis->fOneThd? 0 :
                               HTHDXFromHPIDHTID(hprcx->hpid, lpbpis->htid);

        bp = FindBP(hprcx, hthdx, lpbpis->bptp, lpbpis->bpns, lpaddr, TRUE);

        if (bp != NULL) {
            assert((DWORDLONG)bp == *lpqwNotification);
            RemoveBP(bp);
        } else if ( (hprcx->pstate & (ps_destroyed | ps_killed)) == 0) {
            // Don't fail if this process is already trashed.
            xosd = xosdUnknown;
        }
    }

    return xosd;
}


VOID
ProcessBreakpointCmd(
    HPRCX hprcx,
    HTHDX hthdx,
    LPDBB lpdbb
    )
{
    XOSD xosd = xosdNone;
    XOSD * lpxosd;
    LPDWORD lpdwMessage;
    PDWORDLONG lpqwNotification;
    LPBPS lpbps = (LPBPS)lpdbb->rgbVar;
    LPBPIS lpbpis;
    UINT i;
    DWORD SizeofBps = SizeofBPS(lpbps);

    
    if (!lpbps->cbpis) {
        // enable or disable all extant bps.
        // is this used?
        assert(0 && "clear/set all BPs not implemented in DM");
        xosd = xosdUnsupported;
        Reply(0, &xosd, lpdbb->hpid);
        return;
    }

    lpdwMessage = DwMessage(lpbps);
    lpxosd = RgXosd(lpbps);
    lpqwNotification = QwNotification(lpbps);
    lpbpis = RgBpis(lpbps);

    // walk the list of breakpoint commands

    for (i = 0; i < lpbps->cbpis; i++) {
        switch( lpbpis[i].bptp ) {
          case bptpDataC:
          case bptpDataR:
          case bptpDataW:
          case bptpDataExec:
          case bptpRegC:
          case bptpRegR:
          case bptpRegW:
          case bptpRange:

            //
            // dispatch to watchpoint handler
            //
            lpxosd[i] = HandleWatchpoints(hprcx,
                                          lpbps->fSet,
                                          &lpbpis[i],
                                          &lpqwNotification[i]
                                          );
            if (xosd == xosdNone) {
                xosd = lpxosd[i];
            }
            break;

          case bptpMessage:
          case bptpMClass:

            //
            // handle as address BP - let debugger handle the details
            //

          case bptpExec:
            lpxosd[i] = HandleBreakpoints(hprcx,
                                          lpbps->fSet,
                                          &lpbpis[i],
                                          &lpqwNotification[i]
                                          );
            if (xosd == xosdNone) {
                xosd = lpxosd[i];
            }
            break;

          case bptpInt:
            // ???
            assert(0 && "don't know what these are supposed to do");
            break;
        }
    }

    // send whole structure back to EM

    LpDmMsg->xosdRet = xosd;
    memcpy(LpDmMsg->rgb, lpbps, SizeofBps);
    Reply(SizeofBps, LpDmMsg, lpdbb->hpid);
}

VOID
ProcessSelLimCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )

/*++

Routine Description:

    This function is called in response to a request from the EM to return
    the max valid address in a given segment. Since there are no segments
    in a flat-model system, we just return the max valid user address in the
    address space.

Arguments:

    hprc    - Supplies the handle to the process descriptor

    hthd    - Supplies the handle to the thread descriptor

    lpdbb   - Supplies the request packet.

Return Value:

    None.

--*/

{
    DWORD_PTR * lpdw = (DWORD_PTR *)LpDmMsg->rgb; // For non-Win95 systems the whole addr space is valid.

    Unreferenced(hprc);
    Unreferenced(hthd);

#ifndef KERNEL
    if ( IsChicago( ) ) {
        *lpdw = (ULONG_PTR)SystemInfo.lpMaximumApplicationAddress;
    } else
#endif
    {
        *lpdw = ~0;  // For NT all addresses are valid.
    }

    LpDmMsg->xosdRet = xosdNone;
    Reply( sizeof( DWORD), LpDmMsg, lpdbb->hpid );

    return;
}                   /* ProcessSelLimCmd() */



VOID
ProcessReadMemoryCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )

/*++

Routine Description:

    This function is called in response to a request from the EM to read
    the debuggees memory.  It will take care of any address translations
    which need to be done and request the read operation from the OS.

Arguments:

    hprc    - Supplies the handle to the process descriptor

    hthd    - Supplies the handle to the thread descriptor

    lpdbb   - Supplies the request packet.

Return Value:

    None.

--*/

{
    LPRWP       lprwp       = (LPRWP) lpdbb->rgbVar;
    DWORD       cb          = (DWORD) lprwp->cb;
    LPDM_MSG    lpm         = (LPDM_MSG)MHAlloc( cb + sizeof(DWORD) + FIELD_OFFSET(DM_MSG, rgb) );
    LPVOID      buffer      = lpm->rgb + sizeof(DWORD);
    DWORD       length;

    DPRINT(1, ("ProcessReadMemoryCmd : %p %p:%04x:%I64x:%p %d\n", 
           hprc,
           lprwp->addr.emi, 
           lprwp->addr.addr.seg,
           lprwp->addr.addr.off, 
           buffer, 
           cb
           ));


    if (AddrReadMemory(hprc, hthd, &(lprwp->addr), buffer, cb, &length) == 0) {
        lpm->xosdRet = xosdUnknown;
        Reply(0, lpm, lpdbb->hpid);
    } else {
        lpm->xosdRet = xosdNone;
        *((DWORD *) (lpm->rgb)) = length;
        Reply( length + sizeof(DWORD), lpm, lpdbb->hpid);
    }
    MHFree(lpm);
    return;
}                   /* ProcessReadMemoryCmd() */



VOID
ProcessWriteMemoryCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )

/*++

Routine Description:

    this routine is called to case a write into a debuggess memory.

Arguments:

    hprc        - Supplies a handle to the process to write memory in
    hthd        - Supplies a handle to a thread
    lpdbb       - points to data for the command

Return Value:

    XOSD error code

--*/

{
    LPRWP       lprwp = (LPRWP)lpdbb->rgbVar;
    DWORD       cb    = lprwp->cb;
    char *      buffer    = lprwp->rgb;

    HANDLE      rwHand;
    DWORD       length = 0;
    DWORD       offset;
    XOSD        xosd = xosdUnknown;
    BP_UNIT     instr;
    BREAKPOINT  *bp;

    DEBUG_PRINT("ProcessWriteMemoryCmd called\n");

    /*
     * Sanitize the memory block before writing it into memory :
     * ie: replace any breakpoints that might be in the block
     */

    for(bp=bpList->next; bp; bp=bp->next) {
        if (BPInRange(hprc, hthd, bp, &lprwp->addr, cb, &offset, &instr)) {
#if defined (TARGET_IA64)
        bp->instr1 = *((BP_UNIT *) (buffer + offset));
 
        switch (GetAddrOff(bp->addr) & 0xf) {
            case 0:
                *((BP_UNIT *) (buffer + offset)) &= ((~INST_SLOT0_MASK) | (BP_OPCODE << 5));
                break;
 
            case 4:
                *((BP_UNIT *) (buffer + offset)) &= ((~INST_SLOT1_MASK) | (BP_OPCODE << 14));
                break;
 
            case 8:
                *((BP_UNIT *) (buffer + offset)) &= ((~INST_SLOT2_MASK) | (BP_OPCODE << 23));
            break;
 
            default:
            break;
        }
 
        // change template from MLI(2) to MII(0) if displaced instruction was MOVL
        if ((bp->flags & BREAKPOINT_IA64_MOVL) && ((GetAddrOff(bp->addr) & 0xf) == 4) ) {
        *((char *)buffer + offset - 4)  &= ~(0x1e);
        }
#else
            bp->instr1 = *((BP_UNIT *) (buffer + offset));
            *((BP_UNIT *) (buffer + offset)) = BP_OPCODE;
#endif
        }
    }

    rwHand = hprc->rwHand;

    if (AddrWriteMemory(hprc, hthd, &lprwp->addr, buffer, cb, &length)) {
        LpDmMsg->xosdRet = xosdNone;
    } else {
        LpDmMsg->xosdRet = xosdUnknown;
    }

    *((DWORD *) (LpDmMsg->rgb)) = length;

    Reply(sizeof(DWORD), LpDmMsg, lpdbb->hpid);

    return;
}                               /* ProcessWriteMemoryCmd() */


VOID
ProcessGetContextCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )

/*++

Routine Description:

    This routine is called in response to a request to get the full
    context of a thread for a particular frame.
    The current frame is 0.  They count back positively; caller is 1.

Arguments:

    hprc        - Supplies the handle of process for the thread

    hthd        - Supplies the handle of the thread

    lpdbb       - Supplies pointer to argument area for request

Return Value:

    None.

--*/

{
    LPCONTEXT       lpreg = (LPCONTEXT)LpDmMsg->rgb;
    BOOL            rval;


    Unreferenced(hprc);

    DEBUG_PRINT( "ProcessGetContextCmd\n");

    if (hthd == 0) {
        LpDmMsg->xosdRet = xosdUnknown;
        Reply( 0, LpDmMsg, lpdbb->hpid );
        return;
    }

    lpreg->ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;

#ifndef KERNEL
    //
    // if it is in the middle of a function call, we want
    // to show the saved context, not the real one.
    //
    if (hthd->pcs) {
        memcpy(lpreg, &hthd->pcs->context, sizeof(CONTEXT));
        LpDmMsg->xosdRet = xosdNone;
        Reply( sizeof(CONTEXT), LpDmMsg, lpdbb->hpid );
    } else
#endif  // !KERNEL
    //
    // if the thread is stopped and the stored context is good, use it.
    // SJS - Unless updating a fiber Context.
    //
#ifdef KERNEL
    if ((hthd->tstate & ts_stopped) && (!hthd->fContextStale) )
#else
    if ((hthd->tstate & ts_stopped) && (!hthd->hprc->pFbrCntx))
#endif  // KERNEL
    {
        memcpy(lpreg, &hthd->context, sizeof(CONTEXT));
        LpDmMsg->xosdRet = xosdNone;
        Reply( sizeof(CONTEXT), LpDmMsg, lpdbb->hpid );
    } else if (DbgGetThreadContext(hthd,lpreg)) {
        LpDmMsg->xosdRet = xosdNone;
        Reply( sizeof(CONTEXT), LpDmMsg, lpdbb->hpid );
    } else {
        LpDmMsg->xosdRet = xosdUnknown;
        Reply( 0, LpDmMsg, lpdbb->hpid );
    }
    return;
}                               /* ProcessGetContextCmd() */


VOID
ProcessSetContextCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
/*++

Routine Description:

    This function is used to update the register set for a thread

Arguments:

    hprc        - Supplies a handle to a process

    hthd        - Supplies the handle to the thread to be updated

    lpdbb       - Supplies the set of context information

Return Value:

    None.

--*/

{
    LPCONTEXT   lpcxt = (LPCONTEXT)(lpdbb->rgbVar);
    XOSD        xosd = xosdNone;
    ADDR        addr;

    Unreferenced(hprc);

    DPRINT(5, ("ProcessSetContextCmd : "));

#ifndef KERNEL
    //
    // If we are fiber debugging don't do anything
    //
    if(hthd->hprc->pFbrCntx) {
        Reply(0, &xosd, lpdbb->hpid);
        return;
    }
#endif // !KERNEL

    lpcxt->ContextFlags = CONTEXT_FULL | CONTEXT_FLOATING_POINT;

#ifndef KERNEL
    //
    // if it is in a trojan function call, we want to set the
    // saved context.
    //
    if (hthd->pcs) {
        memcpy(&hthd->pcs->context, lpcxt, sizeof(CONTEXT));
    } else
#endif // !KERNEL
    {
        memcpy(&hthd->context, lpcxt, sizeof(CONTEXT));

        if (hthd->tstate & ts_stopped) {
            hthd->fContextDirty = TRUE;
            /*
             *  If we change the program counter then we may be pointing
             *      at a different breakpoint.  If so then setup to point
             *      to the new breakpoint
             */

            AddrFromHthdx(&addr, hthd);
            SetBPFlag(hthd, FindBP(hthd->hprc, hthd, bptpExec, (BPNS)-1, &addr, FALSE));
#ifndef KERNEL
        } else if (hthd->fWowEvent) {
            WOWSetThreadContext(hthd, lpcxt);
#endif
        } else {
            DbgSetThreadContext(hthd, lpcxt);
        }
    }

    Reply(0, &xosd, lpdbb->hpid);

    return;
}                               /* ProcessSetContextCmd() */




void
PushStoppedThreads(
    HPRCX hprc
    )
/*++

Routine Description:

    Someone's trying to step a thread that didn't stop. We must push
    the stopped thread(s), otherwise it(they) will hit the same BP
    it's(they're) currently at.

Arguments:

    hprc - Supplies the process to push

Return Value:

    none

--*/
{
    BREAKPOINT* bp;
    HTHDX hthd;
    METHOD *ContinueSSMethod;

    for (hthd = hprc->hthdChild; hthd; hthd = hthd->nextSibling) {

    if (bp = AtBP(hthd)) {
        if (bp != EMBEDDED_BP && bp->isStep) {
          // Hit SS again
        } else {
                //
                // We are recovering from a breakpoint, so restore the
                // original instruction, single step and then finally go.
                //

            DEBUG_PRINT("***Recovering from a breakpoint");

            ClearBPFlag(hthd);
            if (bp == EMBEDDED_BP) {

                IncrementIP(hthd);

            } else {

                ContinueSSMethod = (METHOD*)MHAlloc(sizeof(METHOD));
                ContinueSSMethod->notifyFunction = (ACVECTOR)MethodContinueSS;
                ContinueSSMethod->lparam         = (DWORDLONG)ContinueSSMethod;
                ContinueSSMethod->lparam2        = NULL;

                RestoreInstrBP(hthd, bp);
                SingleStepEx(hthd, ContinueSSMethod, FALSE, FALSE, FALSE);

            }
        }
    }
}

}

VOID
ProcessSingleStepCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
/*++

Routine Description:

    This command is called to do a single step of the processor.  If
    calls are made then it will step into the command.

Arguments:

    hprc        - Supplies process handle

    hthd        - Supplies thread handle

    lpdbb       - Supplies information on command

Return Value:

    None.

--*/

{
    LPEXOP     lpexop = (LPEXOP)lpdbb->rgbVar;
    XOSD       xosd = xosdNone;

    Unreferenced( hprc );

    DEBUG_PRINT("ProcessSingleStepCmd called\n");


    if (hprc->pstate & ps_dead) {
        //
        //  The process has exited, and we have
        //  announced the death of all its threads (but one).
        //  All that remains is to clean up the remains.
        //

        hthd->fExceptionHandled = TRUE;
        ContinueThread(hthd);
        ProcessUnloadCmd(hprc, hthd, lpdbb);
        Reply(0, &xosd, lpdbb->hpid);

        return;
    }

    SetHandledStateInStoppedThreads(hprc, !lpexop->fPassException);

#ifndef KERNEL
     if (lpexop->fSetFocus) {
       DmSetFocus(hprc);
    }

    if (!(hthd->tstate & ts_stopped)) {
        PushStoppedThreads(hprc);
    }
    //
    // Catch any exception that changes flow of control
    //
        if (!SetupNLG(hthd, NULL)) {
            RegisterExpectedEvent(hthd->hprc,
                                  (HTHDX)hthd,
                                  EXCEPTION_DEBUG_EVENT,
                                  NO_SUBCLASS,
                                  DONT_NOTIFY,
                                  ActionExceptionDuringStep,
                                  FALSE,
                                  (UINT_PTR)InfoExceptionDuringStep(hthd)
                                  );
        }
#endif  // !KERNEL


    if (hthd->tstate & ts_stepping) {
        xosd = xosdUnknown;
    } else if (lpexop->fStepOver) {
        StepOver(hthd, &EMNotifyMethod, FALSE, FALSE);
    } else {
        SingleStep(hthd, &EMNotifyMethod, FALSE, FALSE);
    }

    Reply(0, &xosd, lpdbb->hpid);
    return;
}                               /* ProcessSingleStepCmd() */



VOID
ProcessRangeStepCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
/*++

Routine Description:

    This routine is called to start a range step.  This will continue
    to do steps as long as the current PC is between the starting
    and ending addresses

Arguments:

    hprc        - Supplies the process handle to be stepped

    hthd        - Supples the thread handle to be stepped

    lpdbb       - Supples the information about the command

Return Value:

    None.

--*/

{
    LPRST       lprst = (LPRST)lpdbb->rgbVar;
    XOSD        xosd = xosdNone;
    HTHDX       hthdT;

    DPRINT(0,("RangeStep [%p - %p]\n", lprst->offStart, lprst->offEnd));

    if (hprc->pstate & ps_dead) {

        //
        //  The process has exited, and we have
        //  announced the death of all its threads (but one).
        //  All that remains is to clean up the remains.
        //

        hthd->fExceptionHandled = TRUE;
        ContinueThread(hthd);
        ProcessUnloadCmd(hprc, hthd, lpdbb);
        Reply(0, &xosd, lpdbb->hpid);
        return;
    }

    assert(hthd);

    hthd->fDisplayReturnValues = lprst->exop.fReturnValues;

    SetHandledStateInStoppedThreads(hprc, !lprst->exop.fPassException);

#if !defined(KERNEL)
    if (!(hthd->tstate & ts_stopped)) {
        PushStoppedThreads(hprc);
    }
    /* Catch any exception that changes flow of control */
        if (!SetupNLG(hthd, NULL)) {
            RegisterExpectedEvent (hthd->hprc,
                                   (HTHDX)hthd,
                                   EXCEPTION_DEBUG_EVENT,
                                   NO_SUBCLASS,
                                   DONT_NOTIFY,
                                   ActionExceptionDuringStep,
                                   FALSE,
                                   (UINT_PTR)InfoExceptionDuringStep(hthd)
                                   );
        }

#endif  // !KERNEL

#if 0
    if ( !fSmartRangeStep ||
            !SmartRangeStep(hthd,
                            lprst->offStart,
                            lprst->offEnd,
                            !lprst->exop.fInitialBP,
                            lprst->exop.fStepOver
                            )) {

#endif // 0
        RangeStep(hthd,
                  lprst->offStart,
                  lprst->offEnd,
                  !lprst->exop.fInitialBP,
                  lprst->exop.fStepOver
                  );
#if 0
    }
#endif // 0

    Reply(0, &xosd, lpdbb->hpid);
    return;
}                               /* ProcessRangeStepCmd() */


VOID
ProcessReturnStepCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
/*++

Routine Description:

Arguments:

    hprc        - Supplies the process handle to be stepped

    hthd        - Supples the thread handle to be stepped

    lpdbb       - Supples the information about the command

Return Value:

    None.

--*/

{
    LPRTRNSTP  lprtrnstp = (LPRTRNSTP)lpdbb->rgbVar;
    XOSD       xosd = xosdNone;

    Unreferenced( hprc );

    if (hprc->pstate & ps_dead) {

        //
        //  The process has exited, and we have
        //  announced the death of all its threads (but one).
        //  All that remains is to clean up the remains.
        //

        hthd->fExceptionHandled = TRUE;
        ContinueThread(hthd);
        ProcessUnloadCmd(hprc, hthd, lpdbb);
        Reply(0, &xosd, lpdbb->hpid);
        return;
    }
    SetHandledStateInStoppedThreads(hprc, !lprtrnstp->exop.fPassException);

    hthd->fDisplayReturnValues = lprtrnstp->exop.fReturnValues;

#if !defined(KERNEL)
    if (lprtrnstp->exop.fSetFocus) {
       DmSetFocus(hprc);
    }

    if (!(hthd->tstate & ts_stopped)) {
        PushStoppedThreads(hprc);
    }
    //
    // Catch any exception that changes flow of control
    //
        if (!SetupNLG(hthd, &lprtrnstp->addrStack)) {
            RegisterExpectedEvent (hthd->hprc,
                                   (HTHDX)hthd,
                                   EXCEPTION_DEBUG_EVENT,
                                   NO_SUBCLASS,
                                   DONT_NOTIFY,
                                   ActionExceptionDuringStep,
                                   FALSE,
                                   (UINT_PTR)InfoExceptionDuringStep(hthd)
                                   );
        }
#endif // !KERNEL
    ExprBPContinue(hthd->hprc, hthd);
    ReturnStep(hthd,
               &EMNotifyMethod,
               FALSE,
               FALSE,
               &(lprtrnstp->addrRA),
               &(lprtrnstp->addrStack)
               );
    Reply(0, &xosd, lpdbb->hpid);
    return;

}                               /* ProcessReturnStepCmd() */



VOID
ProcessContinueCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
/*++

Routine Description:

    This function is used to cause a process to be executed.
    It is called in response to a GO command.

Arguments:

    hprc        - Supplies handle to process to execute

    hthdd       - Supplies handle to thread

    lpdbb       - Command buffer

Return Value:

    xosd Error code

TODO:
    Are there any times where we do not want to allow a GO command
    to be executed.

    Two other possible problems here that need to be dealt with are:

    1.  Single thread go commands

    2.  The current thread not being the thread where the last debug
        event occured.  In this case the DoContinueDebugEvent
        command SHOULD NOT WORK.

--*/

{
    LPEXOP      lpexop = (LPEXOP)lpdbb->rgbVar;
    BREAKPOINT  *bp;
    XOSD        xosd = xosdNone;
    DEBUG_EVENT64 de;
    HTHDXSTRUCT hthdS;
    DWORD       cs;
    HTHDX       hthdT;

    DPRINT(5, ("ProcessContinueCmd : pid=%08lx, tid=%08lx, hthd=%08lx",
            hprc->pid, hthd ? hthd->tid : -1, hthd));

    //
    // If the thread is marked "being connected", this event is
    // used to sync the stream of debug events with the continue
    // commands from the shell
    //

    if (hprc->pstate & ps_connect) {
        Reply(0, &xosd, lpdbb->hpid);
        SetEvent( hEventContinue );
        return;
    }

#ifndef KERNEL
    if (lpexop->fSetFocus) {
       DmSetFocus(hprc);
    }
#endif

    //
    //  Don't enter during event processing, because we
    //  might be here before the DM has finished with the
    //  event we are responding to.
    //
    //  Don't worry about new events during our processing,
    //  since they won't apply to this process.
    // BUGBUG: kentf That's not really true.
    //

    EnterCriticalSection(&csProcessDebugEvent);
    LeaveCriticalSection(&csProcessDebugEvent);

    SetHandledStateInStoppedThreads(hprc, !lpexop->fPassException);

    if (!hthd) {
        WaitForSingleObject(hprc->hEventCreateThread, INFINITE);
        hthd = HTHDXFromHPIDHTID(lpdbb->hpid, lpdbb->htid);
        assert(hthd != 0);
        if (!hthd) {
            xosd = xosdBadThread;
            Reply(0, &xosd, lpdbb->hpid);
            return;
        }
    }

#ifndef KERNEL

    //
    // This is only for keeping a copy of the debug event for building
    // a usermode crashdump.
    //

    ClearPendingDebugEvents(hprc->pid, hthd->tid);

#endif // KERNEL

    if (hprc->pstate & ps_dead) {

        //
        //  The process has exited, and we have announced
        //  the death of all its threads (but one).
        //  All that remains is to clean up the remains.
        //

        hthd->fExceptionHandled = TRUE;
        ContinueThread(hthd);
        ProcessUnloadCmd(hprc, hthd, lpdbb);
        Reply(0, &xosd, lpdbb->hpid);
        return;
    }


    if (hthd->tstate & ts_dead) {

        //
        //  Note that if a terminated thread is frozen
        //  then we do not send a destroy on it yet:
        //  ProcessAsyncGoCmd() deals with those cases.
        //

        hthdS = *hthd;    // keep some info

        //
        // If it isn't frozen, destroy it.
        //

        if ( !(hthd->tstate & ts_frozen)) {
            de.dwDebugEventCode = DESTROY_THREAD_DEBUG_EVENT;
            NotifyEM(&de, hthd, 0, 0);
            FreeHthdx(hthd);
            hprc->pstate &= ~ps_deadThread;
        }

        //
        // if there are other dead threads (how??)
        // put the deadThread bit back.
        //

        for (hthd = hprc->hthdChild; hthd; hthd = hthd->nextSibling) {
            if (hthd->tstate & ts_dead) {
                hprc->pstate |= ps_deadThread;
            }
        }

        hthdS.fExceptionHandled = TRUE;
        ContinueThread(&hthdS);
        Reply(0, &xosd, lpdbb->hpid);
        return;
    }


#if !defined(KERNEL)
    if (hthd->tstate & ts_frozen) {
        //
        // We are about to freeze the thread we are continuing
        // If all the other threads are suspended we should
        // return an error since no execution is possible.
        //
        HTHDX hthdNext;
        BOOL fAllThreadsSuspended = TRUE;

        for (hthdNext = (HTHDX)hprc->hthdChild;hthdNext;hthdNext=hthdNext->nextSibling) {
            if (hthdNext != hthd) {
                DWORD dwSuspendCount;
                if ((GetThreadSuspendCount(hthdNext, &dwSuspendCount) == xosdNone) &&
                        (dwSuspendCount == 0)) {

                    //
                    // Found a thread that is not suspended
                    //
                    fAllThreadsSuspended = FALSE;
                    break;
                }
            }
        }

        if (fAllThreadsSuspended) {
            xosd = xosdAllThreadsSuspended ;
            SendDBCError(hprc, xosd, NULL);
            Reply(0, &xosd, lpdbb->hpid);
            return;
        }

        //
        // this thread is not really suspended.  We need to
        // continue it and cause it to be suspended before
        // allowing it to actually execute the user's code.
        //
        if (!MakeThreadSuspendItself(hthd)) {
            hthd->tstate &= ~ts_frozen;
        }
    }
#endif  // !KERNEL

    //
    // This thread is going to run.  Clear the pending breakpoint record.
    //

    //
    //  If the current thread is sitting on a breakpoint then it is necessary
    //  to do a step over it and then try and do a go.  Steps are necessary
    //  to ensure that the breakpoint will be restored.
    //
    //  If the breakpoint is embedded in the code path and not one we
    //  set then just advance the IP past the breakpoint.
    //
    //  NOTENOTE - jimsch - it is necessary to do a single thread step
    //          to insure that no other threads of execution would have
    //          hit the breakpoint we are disabling while the step on
    //          the current thead is being executed.
    //
    //  NOTENOTE - jimsch - INTEL - two byte int 3 is not deal with
    //          correctly if it is embedded.
    //

    if (bp = AtBP(hthd)) {
        //
        // We are recovering from a breakpoint, so restore the
        // original instruction, single step and then finally go.
        //

        METHOD *ContinueSSMethod;

        DEBUG_PRINT("Recovering from a breakpoint\n");

        if (bp == EMBEDDED_BP) {

            //
            // "step" past the bp and continue.
            //
            if (!hthd->fDontStepOff) {
                ClearBPFlag(hthd);
                hthd->fIsCallDone = FALSE;
                IncrementIP(hthd);
            }

        } else {

            ContinueSSMethod = (METHOD*)MHAlloc(sizeof(METHOD));
            ContinueSSMethod->notifyFunction = (ACVECTOR)MethodContinueSS;
            ContinueSSMethod->lparam         = (UINT_PTR)ContinueSSMethod;
            ContinueSSMethod->lparam2        = NULL;

            SingleStep(hthd, ContinueSSMethod, FALSE, FALSE);
            Reply(0, &xosd, lpdbb->hpid);
            return;
        }
    }

    //
    //  Have the Expression BP manager know that we are continuing
    //
    ExprBPContinue( hprc, hthd );


    //
    //  Do a continue debug event and continue execution
    //

    assert ( (hprc->pstate & ps_destroyed) == 0 );

    Reply(0, &xosd, lpdbb->hpid);

#ifndef KERNEL
    //
    // In user mode crashdumps, this is how we emulate the
    // continuation of the loader breakpoint.
    //
    if (CrashDump) {
        SetEvent( hEventContinue );
    } else
#endif
    {
        ContinueProcess(hprc);
    }

    return;
}                               /* ProcessContinueCmd() */

void
MethodContinueSS(
    DEBUG_EVENT64 *pde,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    )
{
    METHOD *method = (METHOD*)lparam;
    PBREAKPOINT bp = (BREAKPOINT*) method->lparam2;

    Unreferenced( pde );

    if (bp && bp != EMBEDDED_BP && !bp->hWalk) {
        WriteBreakPoint( bp );
    }

    MHFree((LPVOID)method->lparam);

    //
    //  Have the Expression BP manager know that we are continuing
    //
    ExprBPContinue( hthd->hprc, hthd );

    ContinueThread(hthd);

    return;
}



DWORD
ProcessFreezeThreadCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{

#ifdef KERNEL

    XOSD   xosd = xosdNone;
    hthd->tstate |= ts_frozen;
    Reply(0, &xosd, lpdbb->hpid);
    return( xosd );

#else   // KERNEL

    XOSD   xosd = xosdNone;

    Unreferenced( hprc );

    DEBUG_PRINT("ProcessFreezeThreadCmd called.\n\r");

    if (!(hthd->tstate & ts_frozen)) {

        if (hthd->tstate & ts_stopped) {
            //
            // If the thread is at a debug event, don't suspend it -
            // let it suspend itself later when we continue it.
            //
            hthd->tstate |= ts_frozen;
        } else if (SuspendThread(hthd->rwHand) != -1L) {
            hthd->tstate |= ts_frozen;
        } else {
            xosd = xosdBadThread;
        }
    }

    Reply(0, &xosd, lpdbb->hpid);
    return( xosd );

#endif  // KERNEL
}



EXCEPTION_DESCRIPTION ExceptionList[] = {
                // DBG_CONTROL_C and DBG_CONTROL_BREAK are *only*
                // raised if the app is being debugged.  The system
                // remotely creates a thread in the debuggee and then
                // raises one of these exceptions; the debugger must
                // respond to the first-chance exception if it wants
                // to trap it at all, because it will never see a
                // last-chance notification.
    {(DWORD)DBG_CONTROL_C,                    efdStop,   0, "Control-C"},
    {(DWORD)DBG_CONTROL_BREAK,                efdStop,   0, "Control-Break"},
    {(DWORD)EXCEPTION_DATATYPE_MISALIGNMENT,  EFDDEFAULT, 0, "Datatype Misalignment"},
    {(DWORD)EXCEPTION_ACCESS_VIOLATION,       EFDDEFAULT, 0, "Access Violation"},
    {(DWORD)EXCEPTION_IN_PAGE_ERROR,          EFDDEFAULT, 0, "In Page Error"},
    {(DWORD)STATUS_ILLEGAL_INSTRUCTION,       EFDDEFAULT, 0, "Illegal Instruction"},
    {(DWORD)EXCEPTION_ARRAY_BOUNDS_EXCEEDED,  EFDDEFAULT, 0, "Array Bounds Exceeded"},
                // Floating point exceptions will only be raised if
                // the user calls _controlfp() to turn them on.
    {(DWORD)EXCEPTION_FLT_DENORMAL_OPERAND,   EFDDEFAULT, 0, "Float Denormal Operand"},
    {(DWORD)EXCEPTION_FLT_DIVIDE_BY_ZERO,     EFDDEFAULT, 0, "Float Divide by Zero"},
    {(DWORD)EXCEPTION_FLT_INEXACT_RESULT,     EFDDEFAULT, 0, "Float Inexact Result"},
    {(DWORD)EXCEPTION_FLT_INVALID_OPERATION,  EFDDEFAULT, 0, "Float Invalid Operation"},
    {(DWORD)EXCEPTION_FLT_OVERFLOW,           EFDDEFAULT, 0, "Float Overflow"},
    {(DWORD)EXCEPTION_FLT_STACK_CHECK,        EFDDEFAULT, 0, "Float Stack Check"},
    {(DWORD)EXCEPTION_FLT_UNDERFLOW,          EFDDEFAULT, 0, "Float Underflow"},
                // STATUS_NO_MEMORY can be raised by HeapAlloc and
                // HeapRealloc.
    {(DWORD)STATUS_NO_MEMORY,                 EFDDEFAULT, 0, "No Memory"},
                // STATUS_NONCONTINUABLE_EXCEPTION is raised if a
                // noncontinuable exception happens and an exception
                // filter return -1, meaning to resume execution.
    {(DWORD)STATUS_NONCONTINUABLE_EXCEPTION,  EFDDEFAULT, 0, "Noncontinuable Exception"},
                // STATUS_INVALID_DISPOSITION means an NT exception
                // filter (which is slightly different from an MS C
                // exception filter) returned some value other than
                // 0 or 1 to the system.
    {(DWORD)STATUS_INVALID_DISPOSITION,       EFDDEFAULT, 0, "Invalid Disposition"},
    {(DWORD)EXCEPTION_INT_DIVIDE_BY_ZERO,     EFDDEFAULT, 0, "Integer Divide by Zero"},
    {(DWORD)EXCEPTION_INT_OVERFLOW,           EFDDEFAULT, 0, "Integer Overflow"},
    {(DWORD)EXCEPTION_PRIV_INSTRUCTION,       EFDDEFAULT, 0, "Privileged Instruction"},
    {(DWORD)STATUS_STACK_OVERFLOW,            EFDDEFAULT, 0, "Stack Overflow"},
    {(DWORD)STATUS_DLL_NOT_FOUND,             EFDDEFAULT, 0, "DLL Not Found"},
    {(DWORD)STATUS_DLL_INIT_FAILED,           EFDDEFAULT, 0, "DLL Initialization Failed"},
    {(DWORD)(0xE0000000 | 'msc'),             efdNotify, 0, "Microsoft C++ Exception"},
};

#define SIZEOFELIST ( sizeof(ExceptionList) / sizeof(ExceptionList[0]) )

void
ProcessGetExceptionState(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
/*++

Routine Description:

    This function is used to query the dm about exception handling.

Arguments:

    hprc        - Supplies process handle

    hthd        - Supplies thread handle

    lpdbb       - Supplies info about the command

Return Value:

    None.

--*/

{
    LPEXCMD lpexcmd = (LPEXCMD)lpdbb->rgbVar;
    LPEXCEPTION_DESCRIPTION lpexdesc = (LPEXCEPTION_DESCRIPTION)LpDmMsg->rgb;
    EXCEPTION_LIST  *eList;
    XOSD           xosd = xosdNone;
    int i = 0;
    DWORD val = 1;

    Unreferenced     (hthd);

    DEBUG_PRINT("ProcessGetExceptionStateCmd");

    switch( lpexcmd->exc ) {

        case exfFirst:

            if (!hprc || !hprc->exceptionList) {
                *lpexdesc = ExceptionList[0];
            } else {
                *lpexdesc = hprc->exceptionList->excp;
            }
            break;

        case exfNext:

            xosd = xosdEndOfStack;
            if (hprc && hprc->exceptionList) {
                for (eList=hprc->exceptionList; eList; eList=eList->next) {
                    if (eList->excp.dwExceptionCode ==
                                                   lpexdesc->dwExceptionCode) {
                        eList = eList->next;
                        if (eList) {
                            *lpexdesc = eList->excp;
                            xosd = xosdNone;
                        } else {
                            lpexdesc->dwExceptionCode = 0;
                        }
                        break;
                    }
                }
            } else {
                for (i = 0; i < SIZEOFELIST; i++) {
                    if (ExceptionList[i].dwExceptionCode ==
                                                   lpexdesc->dwExceptionCode) {
                        if (i+1 < SIZEOFELIST) {
                            *lpexdesc = ExceptionList[i+1];
                            xosd = xosdNone;
                        } else {
                            lpexdesc->dwExceptionCode = 0;
                        }
                        break;
                    }
                }
            }

            break;

        case exfSpecified:

            xosd = xosdEndOfStack;
            if (hprc && hprc->exceptionList) {
                for (eList = hprc->exceptionList; eList; eList = eList->next) {
                    if (eList->excp.dwExceptionCode ==
                                                   lpexdesc->dwExceptionCode) {
                        *lpexdesc = eList->excp;
                        xosd = xosdNone;
                        break;
                    }
                }
            } else {
                for (i = 0; i < SIZEOFELIST; i++) {
                    if (ExceptionList[i].dwExceptionCode ==
                                                   lpexdesc->dwExceptionCode) {
                        *lpexdesc = ExceptionList[i];
                        xosd = xosdNone;
                        break;
                    }
                }
            }

            break;

        case exfDefault:
            lpexdesc->dwExceptionCode = 0;
            lpexdesc->efd = EfdDefault;
            lpexdesc->rgchDescription[0] = 0;
            break;

        default:
           DPRINT(1,("Invalid exf to ProcessGetExceptionState %i\n",lpexcmd->exc));
           xosd = xosdUnknown;
           break;
    }

    LpDmMsg->xosdRet = xosd;
    Reply(sizeof(EXCEPTION_DESCRIPTION), LpDmMsg, lpdbb->hpid);
    return;
}


VOID
ProcessSetExceptionState(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
/*++

Routine Description:

    This function is used to change how the debugger will handle exceptions.

Arguments:

    hprc        - Supplies process handle

    hthd        - Supplies thread handle

    lpdbb       - Supplies info about the command

Return Value:

    None.

--*/

{
    LPEXCEPTION_DESCRIPTION lpexdesc = (LPEXCEPTION_DESCRIPTION)lpdbb->rgbVar;
    EXCEPTION_LIST  *eList;
    XOSD           xosd = xosdNone;

    Unreferenced     (hthd);

    DEBUG_PRINT("ProcessSetExceptionStateCmd");

    if (lpexdesc->exc == exfDefault) {
        EfdDefault = lpexdesc->efd;
        Reply(0, &xosd, lpdbb->hpid);
        return;
    }

    if (!hprc) {
        WaitForSingleObject(hEventCreateProcess, INFINITE);
        hprc = HPRCFromHPID(lpdbb->hpid);
        if (!hprc) {
            xosd = xosdUnknown;
            Reply(0, &xosd, lpdbb->hpid);
            return;
        }
    }

    for (eList=hprc->exceptionList; eList; eList=eList->next) {
        if (eList->excp.dwExceptionCode==lpexdesc->dwExceptionCode) {
            break;
        }
    }

    if (eList) {
        // update it:
        eList->excp = *lpexdesc;
    } else {
        // add it:
        InsertException(&(hprc->exceptionList), lpexdesc);
    }

    Reply(0, &xosd, lpdbb->hpid);
    return;
}


EXCEPTION_FILTER_DEFAULT
ExceptionAction(
    HPRCX hprc,
    DWORD dwExceptionCode
    )
{
    EXCEPTION_LIST   *eList;

    for (eList=hprc->exceptionList; eList; eList=eList->next) {
        if (eList->excp.dwExceptionCode==dwExceptionCode ) {
            break;
        }
    }

    if (eList != NULL) {
        return eList->excp.efd;
    } else {
        return EfdDefault;
    }
}


void
RemoveExceptionList(
    HPRCX hprc
    )
{
    EXCEPTION_LIST *el, *elt;
    for(el = hprc->exceptionList; el; el = elt) {
        elt = el->next;
        MHFree(el);
    }
    hprc->exceptionList = NULL;
}


EXCEPTION_LIST *
InsertException(
    EXCEPTION_LIST ** ppeList,
    LPEXCEPTION_DESCRIPTION lpexc
    )
{
    LPEXCEPTION_LIST pnew;
    while ((*ppeList) &&
             (*ppeList)->excp.dwExceptionCode < lpexc->dwExceptionCode) {
        ppeList = &((*ppeList)->next);
    }
    pnew = (LPEXCEPTION_LIST)MHAlloc(sizeof(EXCEPTION_LIST));
    pnew->next = *ppeList;
    *ppeList = pnew;
    pnew->excp = *lpexc;
    return pnew;
}


void
InitExceptionList(
    HPRCX hprc
    )
{
    int i;
    for (i = 0; i < SIZEOFELIST; i++) {
        InsertException(&(hprc->exceptionList), ExceptionList + i);
    }
}



VOID
ProcessSystemServiceCmd(
    HPRCX   hprc,
    HTHDX   hthd,
    LPDBB   lpdbb
    )

/*++

Routine Description:

    This function is called in response to a SystemService command from
    the shell.  It is used as a catch all to get and set strange information
    which is not covered elsewhere.  The set of SystemServices is OS and
    implemenation dependent.

Arguments:

    hprc        - Supplies a process handle

    hthd        - Supplies a thread handle

    lpdbb       - Supplies the command information packet

Return Value:

    None.

--*/

{
    LPSSS lpsss =  (LPSSS) lpdbb->rgbVar;

    switch( lpsss->ssvc ) {
        case ssvcGeneric:
            ProcessIoctlGenericCmd( hprc, hthd, lpdbb );
            return;

        case ssvcCustomCommand:
            ProcessSSVCCustomCmd( hprc, hthd, lpdbb );
            return;

        default:
            LocalProcessSystemServiceCmd( hprc, hthd, lpdbb );
            return;
    }
}                               /* ProcessSystemServiceCmd() */


VOID
ProcessSetPathCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
/*++

Routine Description:

    Sets the search path;

Arguments:

    hprc   -

    hthd   -

    lpdbb  -

Return Value:

    None.

--*/

{
    SETPTH *SetPath = (SETPTH *)lpdbb->rgbVar;

    SearchPathSet = SetPath->Set;
    if ( SearchPathSet ) {
        _tcscpy(SearchPathString, SetPath->Path );
    } else {
        SearchPathString[0] = '\0';
    }

    LpDmMsg->xosdRet = xosdNone;
    Reply(0, LpDmMsg, lpdbb->hpid);
}

ULONG
ProcessGetTimeStamp(
        HPRCX   hprc,
        HTHDX   hthd,
        LPDBB   lpdbb
        )
/*++

Routine Description:

        This routine gets the TimeStamp and CheckSum from the specified image
        file and returns them.

--*/
{
    HANDLE              hFile = INVALID_HANDLE_VALUE;
    IMAGE_DOS_HEADER    dosHdr;
    IMAGE_NT_HEADERS    ntHdr;
        BOOL                            fSucc;
        ULONG                           rSize = 0;
        LPTSTR                          ImageName = (LPTSTR) lpdbb->rgbVar;
        LPTCSR                          lpReply = (LPTCSR) LpDmMsg->rgb;
        ULONG                           cbRead;

        LpDmMsg->xosdRet = xosdFileNotFound;

        hFile = CreateFile (ImageName,
                                                GENERIC_READ,
                                                FILE_SHARE_READ,
                                                NULL,
                                                OPEN_EXISTING,
                                                FILE_ATTRIBUTE_NORMAL,
                                                NULL
                                                );

        if (hFile == INVALID_HANDLE_VALUE) {
                goto Return;
    }

        LpDmMsg->xosdRet = xosdBadFormat ;

        fSucc = ReadFile (hFile, &dosHdr,  sizeof (dosHdr), &cbRead, NULL);

        if (!fSucc || dosHdr.e_magic != IMAGE_DOS_SIGNATURE) {
                goto Return;
    }

        if (!SetFilePointer (hFile, dosHdr.e_lfanew, NULL, FILE_BEGIN)) {
                goto Return;
    }

        if (!ReadFile (hFile, &ntHdr, sizeof (ntHdr), &cbRead, NULL) ||
                ntHdr.Signature != IMAGE_NT_SIGNATURE) {
                goto Return;
    }

        rSize = sizeof (*lpReply);
        LpDmMsg->xosdRet = xosdNone;

        lpReply->TimeStamp = ntHdr.FileHeader.TimeDateStamp ;
        lpReply->CheckSum = ntHdr.OptionalHeader.CheckSum ;

Return:

        if (hFile != INVALID_HANDLE_VALUE) {
                CloseHandle (hFile);
    }

        return rSize;
}



