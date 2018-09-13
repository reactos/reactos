#include "precomp.h"
#pragma hdrstop

#include <wdbgexts.h>

#include "dmsql.h"
#include "fbrdbg.h"
#include "fiber.h"

HANDLE hEventContinue;
extern LPDM_MSG             LpDmMsg;
extern SYSTEM_INFO          SystemInfo;
extern WT_STRUCT            WtStruct;          // ..  for wt
extern DEBUG_ACTIVE_STRUCT  DebugActiveStruct; // ... for DebugActiveProcess()
extern RELOAD_STRUCT        ReloadStruct;      // for !reload, usermode

extern PKILLSTRUCT          KillQueue;
extern CRITICAL_SECTION     csKillQueue;
extern CRITICAL_SECTION     csProcessDebugEvent;

extern HTHDX                thdList;
extern HPRCX                prcList;
extern CRITICAL_SECTION     csThreadProcList;

extern BOOL                 fSmartRangeStep;
extern HANDLE               hEventNoDebuggee;
extern HANDLE               hEventRemoteQuit;
extern char                 nameBuffer[];
extern DEBUG_EVENT64        falseSSEvent;

extern BOOL                 fDisconnected;
extern BOOL                 fUseRoot;
extern BOOL                 FLoading16;
extern BOOL                 FDMRemote;
extern BOOL                 fUseRealName;




BOOL
DbgWriteMemory(
    HPRCX       hprc,
    ULONG64     qwOffset,
    LPBYTE      lpb,
    DWORD       cb,
    LPDWORD     pcbWritten
    )
/*++

Routine Description:

    Write to a flat address in a process.

Arguments:

    hprc - Supplies the handle to the process

    lpOffset - Supplies address of data in process

    lpb    - Supplies a pointer to the bytes to be written

    cb     - Supplies the count of bytes to be written

    pcbWritten - Returns the number of bytes actually written

Return Value:

    TRUE if successful and FALSE otherwise

--*/

{
    BOOL        fRet;
    LPVOID lpOffset = (LPVOID)qwOffset;

    assert(hprc->rwHand != (HANDLE)-1);

    // For now we will not modify fibers
    /*if ((hprc->pFbrCntx) && (hprc->fUseFbrs)) {
        return FALSE;
    }*/

    if (hprc->rwHand == (HANDLE)-1) {
        return FALSE;
    }

    fRet = WriteProcessMemory(hprc->rwHand, lpOffset, lpb, cb, pcbWritten);

#if  defined(JLS_RWBP) && DBG
    {
        DWORD   cbT;
        LPBYTE  lpbT = malloc(cb);

        assert( fRet );
        assert( *pcbWritten == cb );
        fRet = ReadProcessMemory(hprc->rwHand, lpOffset, lpbT, cb, &cbT);
        assert(fRet);
        assert( cb == cbT);
        assert(memcmp(lpbT, lpb) == 0);
        free lpbT;
    }
#endif

    return fRet;
}


BOOL
DbgReadMemory(
    HPRCX   hprc,
    ULONG64 qwOffset,
    LPVOID  lpb,
    DWORD   cb,
    LPDWORD lpRead
    )
/*++

Routine Description:

    Read from a flat address in a process.

Arguments:

    hprc - Supplies the handle to the process

    lpOffset - Supplies address of data in process

    lpb    - Supplies a pointer to the local buffer

    cb     - Supplies the count of bytes to read

    lpRead - Returns the number of bytes actually read

Return Value:

    TRUE if successful and FALSE otherwise

--*/

{
    DWORD cbr;
    LPVOID lpOffset = (LPVOID)qwOffset;

    assert( Is64PtrSE(qwOffset) );

    if (CrashDump) {
        cbr = DmpReadMemory( qwOffset, lpb, cb );
        if (lpRead) {
            *lpRead = cbr;
        }
        return (cbr > 0) || (cbr == cb);
    }

    assert(hprc->rwHand != (HANDLE)-1);

    if (hprc->rwHand == (HANDLE)-1) {
        return FALSE;
    }

    if (ReadProcessMemory(hprc->rwHand, lpOffset, lpb, cb, lpRead)) {

        return TRUE;

    } else {

#if DBG
        int e = GetLastError();
#endif
        //
        //  Reads across page boundaries will not work if the
        //  second page is not accessible.
        //
#define PAGE_SIZE (SystemInfo.dwPageSize)
#define PAGE_MASK (~(DWORD_PTR)(PAGE_SIZE-1))

        DWORD firstsize;
        DWORD dwRead;

        firstsize = (DWORD)((((DWORD_PTR)lpOffset + PAGE_SIZE) & PAGE_MASK) - (DWORD_PTR)lpOffset);
        if (cb < firstsize) {
            firstsize = cb;
        }

        DPRINT(0,("ReadMemory @%p %i bytes failed. Error:%i Now trying reading %i bytes\n",lpOffset,cb, e, firstsize));
        //
        // read from the first page.  If the first read fails,
        // fail the whole thing.
        //

        if (!ReadProcessMemory(hprc->rwHand,
                               lpOffset,
                               lpb,
                               firstsize,
                               lpRead)) {
           DPRINT(0,("ReadMemory @%p %i bytes failed. Error:%i \n",lpOffset, firstsize, GetLastError()));
            return FALSE;
        }

        //
        // read intermediate complete pages.
        // if any of these reads fail, succeed with a short read.
        //

        assert(*lpRead == firstsize);
        cb -= firstsize;
        lpb = (LPVOID)((LPBYTE)lpb + firstsize);
        lpOffset = (LPVOID)((LPBYTE)lpOffset + firstsize);

        while (cb >= PAGE_SIZE) {

            if (!ReadProcessMemory(hprc->rwHand, lpOffset, lpb, PAGE_SIZE,
                                                                 &dwRead)) {
                return TRUE;
            } else {
                assert(dwRead == PAGE_SIZE);
                lpb = (LPVOID)((LPBYTE)lpb + PAGE_SIZE);
                lpOffset = (LPVOID)((LPBYTE)lpOffset + PAGE_SIZE);
                *lpRead += dwRead;
                cb -= PAGE_SIZE;
            }
        }

        if (cb > 0) {
            if (ReadProcessMemory(hprc->rwHand, lpOffset, lpb, cb, &dwRead)) {
                assert(dwRead == cb);
                *lpRead += dwRead;
            }
        }

        return TRUE;
    }

}

BOOL
DbgGetThreadContext(
    HTHDX hthd,
    PCONTEXT lpcontext
    )
{
    if (CrashDump) {
        return DmpGetContext(hthd->tid-1, lpcontext);
    } else if (hthd->fWowEvent) {
        return WOWGetThreadContext(hthd, lpcontext);
    } else {
        if ((hthd->hprc->pFbrCntx) && (hthd->hprc->fUseFbrs)) {
            DWORD lpRead;
            DbgReadMemory(hthd->hprc,
                         (ULONG64) hthd->hprc->pFbrCntx,
                         lpcontext,
                         sizeof(CONTEXT),
                         &lpRead);
            if(lpRead == sizeof(CONTEXT)){
                return TRUE;
            } else {
                return FALSE;
            }
        }
#if defined(TARGET_IA64)
        {
            //
            // v-vadimp - on IA64 CONTEXT has to be 16 byte aligned, while
            // what we may get here is a member of HTHDX (or whatever else)
            // where it may not be (compiler bug?), so make a local CONTEXT
            // var which will be aligned by the compiler, and do the call
            // with it
            //
            CONTEXT Context = {0};
            BOOL i;
            Context.ContextFlags = lpcontext->ContextFlags;
            DPRINT(1,("GetThreadContext:Thread %p\n",hthd->rwHand));
            i = GetThreadContext(hthd->rwHand, &Context);
            DPRINT(1,("GetThreadContext:%i-%i\n",i,GetLastError()));
            memcpy((LPVOID)lpcontext,(LPVOID)&Context,sizeof(CONTEXT));
            DPRINT(1,("Returned context:IIP:%016I64x IPSR:%016I64x Sp:%016I64x\n",
                      lpcontext->StIIP,
                      lpcontext->StIPSR,
                      lpcontext->IntSp));
            return i;
        }
#endif
        return GetThreadContext(hthd->rwHand, lpcontext);
    }
}

BOOL
DbgSetThreadContext(
    HTHDX hthd,
    PCONTEXT lpcontext
    )
{
    assert(!CrashDump);

    if (CrashDump) {
        return FALSE;
    } else if (hthd->fWowEvent) {
        return WOWSetThreadContext(hthd, lpcontext);
    } else {
#if defined (TARGET_IA64)
        { // see comment in DbgGetThreadContext
            CONTEXT Context = {0};
            BOOL i;
            memcpy(&Context,lpcontext,sizeof(CONTEXT));
            i=SetThreadContext(hthd->rwHand, &Context);
            DPRINT(1,("Setting context:IIP:%016I64x IPSR:%016I64x Sp:%016I64x:%i:%i\n",
                   Context.StIIP,
                   Context.StIPSR,
                   Context.IntSp,
                   i,
                   GetLastError()));
            return i;
        }
#endif
        return SetThreadContext(hthd->rwHand, lpcontext);
    }
}

BOOL
WriteBreakPoint(
    IN PBREAKPOINT Breakpoint
    )
{
    DWORD cb;
    BP_UNIT opcode = BP_OPCODE;
#if defined(TARGET_IA64)
    BOOL r;
    BP_UNIT Content;
    r = AddrReadMemory(Breakpoint->hprc,Breakpoint->hthd,&Breakpoint->addr,&Content,BP_SIZE,&cb);

    switch(GetAddrOff(Breakpoint->addr) & 0xf) {
        case 0:
            Content = (Content & ~(INST_SLOT0_MASK)) | (opcode << 5);
            break;

        case 4:
            Content = (Content & ~(INST_SLOT1_MASK)) | (opcode << 14);
            break;

        case 8:
            Content = (Content & ~(INST_SLOT2_MASK)) | (opcode << 23);
            break;

        default:
            assert(!"Bad bundle slot - breakpoint not written");
        }

        DPRINT(0, ("Writing BP @ %I64x; Content:%I64x\n",
               GetAddrOff(Breakpoint->addr),
               (ULONG64)Content
               ));

        r = AddrWriteMemory(Breakpoint->hprc,Breakpoint->hthd,&Breakpoint->addr,&Content,BP_SIZE,&cb);

        // read in intruction template if current instruction is slot 2.
        // check for two-slot MOVL instruction. Reject request if attempt to
        // set break in slot 2 of MLI template.
        if ((GetAddrOff(Breakpoint->addr) & 0xf) != 0) {
            ADDR BundleAddr = Breakpoint->addr;
            GetAddrOff(BundleAddr) &= ~0xf;
            r = AddrReadMemory(Breakpoint->hprc,Breakpoint->hthd,&BundleAddr,&Content,BP_SIZE,&cb);
                if (((Content & INST_TEMPL_MASK) >> 1) == 0x2) {
                    if ((GetAddrOff(Breakpoint->addr) & 0xf) == 4) {
                        // if template= type 2 MLI, change to type 0
                        Content &= ~((INST_TEMPL_MASK >> 1) << 1);
                        Breakpoint->flags |= BREAKPOINT_IA64_MOVL;
                        r = AddrWriteMemory(Breakpoint->hprc,Breakpoint->hthd,&BundleAddr,&Content,BP_SIZE,&cb);
                    } else {
                        // set breakpoint at slot 2 of MOVL is illegal
                                        assert(!"Attempting to set a BP on the second slot of MOVL");
                        return 0;
                    }
                }
            }
#else
    BOOL r = AddrWriteMemory(Breakpoint->hprc,
                             Breakpoint->hthd,
                             &Breakpoint->addr,
                             &opcode,
                             BP_SIZE,
                             &cb);
#endif
    return (r && (cb == BP_SIZE));
}

BOOL
RestoreBreakPoint(
    IN PBREAKPOINT Breakpoint
    )
{
    DWORD cb;
    BOOL r;

    assert(Breakpoint->bpType == bptpExec || Breakpoint->bpType == bptpMessage);

#if defined(TARGET_IA64)
{
    BP_UNIT Content;
    ADDR BundleAddr;
    DWORD k;

    // Read in memory since adjancent instructions in the same bundle may have
    // been modified after we save them. Restore only the content of the slot which has
    // the break instruction inserted.

    DPRINT(0, ("Restoring BP @ %p\n",GetAddrOff(Breakpoint->addr)));
    if(! AddrReadMemory(Breakpoint->hprc, Breakpoint->hthd, &Breakpoint->addr,(LPBYTE) &Content, BP_SIZE, &cb) ) {
        return FALSE;
    }

    switch (GetAddrOff(Breakpoint->addr) & 0xf) {
        case 0:
            Content = (Content & ~(INST_SLOT0_MASK)) | (Breakpoint->instr1 & INST_SLOT0_MASK);
            break;

        case 4:
            Content = (Content & ~(INST_SLOT1_MASK)) | (Breakpoint->instr1 & INST_SLOT1_MASK);
            break;

        case 8:
            Content = (Content & ~(INST_SLOT2_MASK)) | (Breakpoint->instr1 & INST_SLOT2_MASK);
            break;

        default:
            break;
    }
    if (!(r = AddrWriteMemory(Breakpoint->hprc, Breakpoint->hthd, &Breakpoint->addr, (LPBYTE) &Content, BP_SIZE, &cb)) ) {
        return FALSE;
    }

    // restore template to MLI if displaced instruction was MOVL
    if (Breakpoint->flags & BREAKPOINT_IA64_MOVL) {
         BundleAddr.addr.off = GetAddrOff(Breakpoint->addr) & ~(0xf);
         if ( !(AddrReadMemory(Breakpoint->hprc, Breakpoint->hthd, &BundleAddr,(LPBYTE) &Content, BP_SIZE, &k)) ) {
            return FALSE;
         }
         Content &= ~((INST_TEMPL_MASK >> 1) << 1);  // set template to MLI
         Content |= 0x4;
        if ( !(AddrWriteMemory(Breakpoint->hprc, Breakpoint->hthd, &BundleAddr,(LPBYTE) &Content, BP_SIZE, &k)) ) {
            return FALSE;
         }
    }
}
#else
    r = AddrWriteMemory(Breakpoint->hprc,
                        Breakpoint->hthd,
                        &Breakpoint->addr,
                        &Breakpoint->instr1,
                        BP_SIZE,
                        &cb);
#endif
    return (r && (cb == BP_SIZE));
}



/****************************************************************************/
/****************************************************************************/


VOID
GetMachineType(
    LPPROCESSOR p
    )
{
    // Look Ma, no ifdefs!!

    SYSTEM_INFO SystemInfo;

    GetSystemInfo(&SystemInfo);
    switch (SystemInfo.wProcessorArchitecture) {

        case PROCESSOR_ARCHITECTURE_INTEL:
            p->Level = SystemInfo.wProcessorLevel;
            p->Type = mptix86;
            p->Endian = endLittle;
            break;

        case PROCESSOR_ARCHITECTURE_ALPHA:
            p->Level = SystemInfo.wProcessorLevel;
            p->Type = mptdaxp;
            p->Endian = endLittle;
            p->Level = 21064;           // BUGBUG - why?
            break;

        case PROCESSOR_ARCHITECTURE_ALPHA64:
            p->Level = SystemInfo.wProcessorLevel;
            p->Type = mptdaxp;
            p->Endian = endLittle;
            break;

        case PROCESSOR_ARCHITECTURE_IA64:
            p->Level = SystemInfo.wProcessorLevel;
            p->Type = mptia64;
            p->Endian = endLittle;
            break;

        default:
            assert(!"Unknown target machine");
            break;
    }
}

typedef struct
{
    PID pidWanted;
    HWND hFound;
} FWINDOWSTRUCT;

static
BOOL
CALLBACK
EnumAllWindows(
    HWND hwnd,
    LPARAM lParam
    )
{
        PID pid;
        FWINDOWSTRUCT *pfWindow = (FWINDOWSTRUCT*)lParam;

        // what we want is windows *without* an owner, hence !GetWindow...
        // and visible windows
        // and those owned by the debuggee

        if (
                !GetWindow( hwnd, GW_OWNER ) &&
                IsWindowVisible( hwnd ) &&
                (GetWindowThreadProcessId( hwnd, &pid ), (pid==pfWindow->pidWanted))
           )
        {
                pfWindow->hFound = hwnd;
                return FALSE;                                   // found it so don't enum any more
        }
        return TRUE;
}

// find the window associated with the debuggee
// Original code used GetWindow( GW_HWNDFIRST / GW_HWNDNEXT ) but this would
// often, for no reason, fail to give us the debuggee's window. New version uses
// EnumWindows to find it, which seems much more reliable

HWND
HwndFromPid (
    PID pid
    )
{
    FWINDOWSTRUCT fWindow;
    fWindow.pidWanted = pid;
    fWindow.hFound = NULL;

    EnumWindows( EnumAllWindows, (LPARAM)&fWindow );
    return fWindow.hFound;
}


VOID
DmSetFocus (
    HPRCX phprc
    )
{
    PID     pidGer;         // debugger pid
    PID     pidCurFore;     // owner of foreground window
    HWND    hwndCurFore;    // current foreground window
    HWND    phprc_hwndProcess;
    HWND    hwndT;


    // decide if we are the foreground app currently
    pidGer = GetCurrentProcessId(); // debugger pid
    hwndCurFore = GetForegroundWindow();
    if ( hwndCurFore &&
        GetWindowThreadProcessId ( hwndCurFore, &pidCurFore ) ) {

        if ( pidCurFore != pidGer ) {
            // foreground is not debugger, bail out
            return;
        }
    }


    phprc_hwndProcess = HwndFromPid ( phprc->pid );
    if ( !phprc_hwndProcess ) {
        // no window attached to pid; bail out
        return;
    }

    // continuing with valid hwnd's and we have foreground window
    assert ( phprc_hwndProcess );

    // now, get the last active window in that group!
    hwndT = GetLastActivePopup ( phprc_hwndProcess );

    // NOTE: taskman has a check at this point for state disabled...
    //  don't know if I should do it either...

        SetForegroundWindow ( hwndT );
}



/****************************************************************************/
//
// ContinueDebugEvent() queue.
//  We can only have one debug event pending per process, but there may be
//  one event pending for each process we are debugging.
//
//  There are 200 entries in a static sized queue.  If there are ever more
//  than 200 debug events  pending, AND windbg actually handles them all in
//  less than 1/5 second, we will be in trouble.  Until then, sleep soundly.
//
/****************************************************************************/
typedef struct tagCQUEUE {
    struct tagCQUEUE *next;
    DWORD  pid;
    DWORD  tid;
    DWORD  dwContinueStatus;
} CQUEUE, *LPCQUEUE;

static LPCQUEUE lpcqFirst;
static LPCQUEUE lpcqLast;
static LPCQUEUE lpcqFree;
static CQUEUE cqueue[200];
static CRITICAL_SECTION csContinueQueue;

static BOOL DequeueContinueDebugEvents( void );


/***************************************************************************/
/***************************************************************************/



VOID
QueueContinueDebugEvent(
    DWORD   dwProcessId,
    DWORD   dwThreadId,
    DWORD   dwContinueStatus
    )

/*++

Routine Description:

    Queue a debug event continue for later execution.

Arguments:

    dwProcessId = pid to continue

    dwThreadId = tid to continue

    dwContinueStatus - Supplies the continue status code

Return Value:

    None.

--*/

{
    LPCQUEUE lpcq;

    EnterCriticalSection(&csContinueQueue);

    lpcq = lpcqFree;
    assert(lpcq);

    lpcqFree = lpcq->next;

    lpcq->next = NULL;
    if (lpcqLast) {
        lpcqLast->next = lpcq;
    }
    lpcqLast = lpcq;

    if (!lpcqFirst) {
        lpcqFirst = lpcq;
    }

    lpcq->pid = dwProcessId;
    lpcq->tid = dwThreadId;
    lpcq->dwContinueStatus = dwContinueStatus;

    LeaveCriticalSection(&csContinueQueue);

    return;
}                               /* QueueContinueDebugEvent() */


BOOL
DequeueContinueDebugEvents(
    VOID
    )
/*++

Routine Description:

    Remove any pending continues from the queue and execute them.  Updates the
    threads' context if necessary.

    NOTE: the lpcq->tid parameter MUST BE the thread that the debug event
    occured on.

Arguments:

    none

Return Value:

    TRUE if one or more events were continued.
    FALSE if none were continued.

--*/
{
    LPCQUEUE    lpcq;
    BOOL        fDid = FALSE;
    HTHDX       hthd;

    EnterCriticalSection(&csContinueQueue);

    while ( lpcq=lpcqFirst ) {

        hthd = HTHDXFromPIDTID(lpcq->pid, lpcq->tid);

        if (hthd) {
            if (hthd->fContextDirty) {
                DbgSetThreadContext(hthd, &hthd->context);
                hthd->fContextDirty = FALSE;
            }
            hthd->fWowEvent = FALSE;
        }

        ContinueDebugEvent(lpcq->pid, lpcq->tid, lpcq->dwContinueStatus);

        lpcqFirst = lpcq->next;
        if (lpcqFirst == NULL) {
            lpcqLast = NULL;
        }

        lpcq->next = lpcqFree;
        lpcqFree   = lpcq;

        fDid = TRUE;
    }

    LeaveCriticalSection(&csContinueQueue);
    return fDid;
}                               /* DequeueContinueDebugEvents() */


VOID
AddQueue(
    DWORD   dwType,
    DWORD   dwProcessId,
    DWORD   dwThreadId,
    DWORD64 dwData,
    DWORD   dwLen
    )

/*++

Routine Description:



Arguments:

    dwType

    dwProcessId

    dwThreadId

    dwData

    dwLen

Return Value:

    none

--*/

{
    switch (dwType) {
        case QT_CONTINUE_DEBUG_EVENT:
        case QT_TRACE_DEBUG_EVENT:

            if (!CrashDump) {
                QueueContinueDebugEvent(dwProcessId, dwThreadId, (DWORD)dwData);
            }
            break;

        case QT_RELOAD_MODULES:
            if (!ReloadStruct.Flag) {
                ReloadStruct.Hthd = HTHDXFromPIDTID(dwProcessId, dwThreadId);
                ReloadStruct.String = malloc(dwLen);
                _tcscpy( ReloadStruct.String, (PUCHAR)dwData);
                ReloadStruct.Flag = 1;
            }
            break;

        case QT_REBOOT:
        case QT_CRASH:
        case QT_RESYNC:
            assert(!"Unsupported usermode QType in AddQueue.");
            break;

        case QT_DEBUGSTRING:
            assert(!"Is this a bad idea?");
            DMPrintShellMsg( "%s", (LPSTR)dwData );
            free((LPSTR)dwData);
            break;

    }

    if (dwType == QT_CONTINUE_DEBUG_EVENT) {
        SetEvent( hEventContinue );
    }

    return;
}

BOOL
DequeueAllEvents(
    BOOL fForce,       // force a dequeue even if the dm isn't initialized
    BOOL fConsume      // delete all events from the queue with no action
    )
{
    return DequeueContinueDebugEvents();
}

VOID
InitEventQueue(
    VOID
    )
{
    int n;
    int i;

    InitializeCriticalSection(&csContinueQueue);

    n = sizeof(cqueue) / sizeof(CQUEUE);
    for (i = 0; i < n-1; i++) {
        cqueue[i].next = &cqueue[i+1];
    }
    cqueue[n-1].next = NULL;
    lpcqFree = &cqueue[0];
    lpcqFirst = NULL;
    lpcqLast = NULL;
}


VOID
ProcessQueryTlsBaseCmd(
    HPRCX    hprcx,
    HTHDX    hthdx,
    LPDBB    lpdbb
    )

/*++

Routine Description:

    This function is called in response to an EM request to get the base
    of the thread local storage for a given thread and DLL.

Arguments:

    hprcx       - Supplies a process handle

    hthdx       - Supplies a thread handle

    lpdbb       - Supplies the command information packet

Return Value:

    None.

--*/

{
    XOSD       xosd;
    OFFSET      offRgTls;
    DWORD       iTls;
    LPADDR      lpaddr = (LPADDR) LpDmMsg->rgb;
    OFFSET      offResult;
    DWORD       cb;
    int         iDll;
    OFFSET      offDll = * (OFFSET *) lpdbb->rgbVar;

    assert( Is64PtrSE(offDll) );

    /*
     * Read number 1.  Get the pointer to the Thread Local Storage array.
     */


    if ((DbgReadMemory(hprcx, hthdx->offTeb+0x2c,
                           &offRgTls, sizeof(OFFSET), &cb) == 0) ||
        (cb != sizeof(OFFSET))) {
    err:
        xosd = xosdUnknown;
        Reply(0, &xosd, lpdbb->hpid);
        return;
    }

    assert( Is64PtrSE(offRgTls) );

    /*
     *  Read number 2.  Get the TLS index for this dll
     */

    for (iDll=0; iDll<hprcx->cDllList; iDll+=1 ) {
        if (hprcx->rgDllList[iDll].fValidDll) {

            assert( Is64PtrSE(hprcx->rgDllList[iDll].offBaseOfImage) );

            if (hprcx->rgDllList[iDll].offBaseOfImage == offDll) {
                break;
            }
        }
    }

    if (iDll == hprcx->cDllList) {
        goto err;
    }

    if (!DbgReadMemory(hprcx,
                      hprcx->rgDllList[iDll].offTlsIndex,
                      &iTls,
                      sizeof(iTls),
                      &cb) ||
            (cb != sizeof(iTls))) {
        goto err;
    }


    /*
     * Read number 3.  Get the actual TLS base pointer
     */

    if ((DbgReadMemory(hprcx, offRgTls+iTls*sizeof(OFFSET),
                           &offResult, sizeof(OFFSET), &cb) == 0) ||
        (cb != sizeof(OFFSET))) {
        goto err;
    }

    assert( Is64PtrSE(offResult) );

    memset(lpaddr, 0, sizeof(ADDR));

    lpaddr->addr.off = offResult;
#ifdef TARGET_i386
    lpaddr->addr.seg = (SEGMENT) hthdx->context.SegDs;
#else
    lpaddr->addr.seg = 0;
#endif
    ADDR_IS_FLAT(*lpaddr) = TRUE;

    LpDmMsg->xosdRet = xosdNone;
    Reply( sizeof(ADDR), LpDmMsg, lpdbb->hpid );
    return;
}                               /* ProcessQueryTlsBaseCmd() */


VOID
ProcessQuerySelectorCmd(
    HPRCX   hprcx,
    HTHDX   hthdx,
    LPDBB   lpdbb
    )
/*++

Routine Description:

    This command is sent from the EM to fill in an LDT_ENTRY structure
    for a given selector.

Arguments:

    hprcx  - Supplies the handle to the process

    hthdx  - Supplies the handle to the thread and is optional

    lpdbb  - Supplies the pointer to the full query packet

Return Value:

    None.

--*/

{
    XOSD               xosd;

#if defined( TARGET_i386 )
    SEGMENT             seg;

    seg = *((SEGMENT *) lpdbb->rgbVar);

    if (hthdx == hthdxNull) {
        hthdx = hprcx->hthdChild;
    }

    if ((hthdx != NULL) &&
        (GetThreadSelectorEntry(hthdx->rwHand, seg, (LDT_ENTRY *) LpDmMsg->rgb))) {
        LpDmMsg->xosdRet = xosdNone;
        Reply( sizeof(LDT_ENTRY), LpDmMsg, lpdbb->hpid);
        return;
    }
#endif

    xosd = xosdInvalidParameter;

    Reply( sizeof(xosd), &xosd, lpdbb->hpid);

    return;
}                            /* ProcessQuerySelectorCmd */


VOID
ProcessVirtualQueryCmd(
    HPRCX hprc,
    LPDBB lpdbb
    )
{
    XOSD xosd = xosdNone;
    ADDR addr;
    BOOL fRet;
    DWORD dwSize;

    if (!hprc->rwHand || hprc->rwHand == (HANDLE)(-1)) {
        xosd = xosdBadProcess;
    }

    addr = *(LPADDR)(lpdbb->rgbVar);

    if (!ADDR_IS_FLAT(addr)) {
        fRet = TranslateAddress(hprc, 0, &addr, TRUE);
        assert(fRet);
        if (!fRet) {
            xosd = xosdBadAddress;
            goto reply;
        }
    }

    {
        MEMORY_BASIC_INFORMATION mbi;
        LPMEMINFO lpmi;

        dwSize = VirtualQueryEx(hprc->rwHand,
                                (LPVOID)addr.addr.off,
                                &mbi,
                                sizeof(MEMORY_BASIC_INFORMATION)
                                );
        if (dwSize != sizeof(MEMORY_BASIC_INFORMATION)) {
            xosd = xosdUnknown;
            goto reply;
        }

        //
        // the structure layouts are different, so we have to copy by fields,
        // even on 64 bit machines.
        //

        lpmi = (LPMEMINFO)LpDmMsg->rgb;

        ZeroMemory(lpmi, sizeof(*lpmi));

        //
        // These should never have to be sign extended, but let's be safe.
        //
        lpmi->addr.addr.off = SEPtrTo64(mbi.BaseAddress);
        lpmi->addrAllocBase.addr.off = SEPtrTo64(mbi.AllocationBase);
        lpmi->dwAllocationProtect = mbi.AllocationProtect;
        lpmi->uRegionSize = mbi.RegionSize;
        lpmi->dwState = mbi.State;
        lpmi->dwProtect = mbi.Protect;
        lpmi->dwType = mbi.Protect;

    }

  reply:

    LpDmMsg->xosdRet = xosd;
    Reply( sizeof(MEMINFO), LpDmMsg, lpdbb->hpid );

    return;
}                                  /* ProcessVirtualQueryCmd */

VOID
ProcessGetDmInfoCmd(
    HPRCX hprc,
    LPDBB lpdbb,
    DWORD cb
    )
{
    LPDMINFO lpi = (LPDMINFO)LpDmMsg->rgb;

    LpDmMsg->xosdRet = xosdNone;

    lpi->mAsync = asyncRun |
                  asyncMem |
                  asyncStop |
                  asyncBP  |
                  asyncKill |
                  asyncWP |
                  asyncSpawn;
    lpi->fHasThreads = 1;
    lpi->fReturnStep = 0;
    //lpi->fRemote = ???
#ifdef TARGET_i386
    lpi->fAlwaysFlat = 0;
#else
    lpi->fAlwaysFlat = 1;
#endif
    lpi->fHasReload  = 1;
    lpi->fNonLocalGoto = 1;
    lpi->fKernelMode = 0;

    lpi->cbSpecialRegs = 0;
    lpi->MajorVersion = 0;
    lpi->MinorVersion = 0;

    lpi->Breakpoints = bptsExec |
                       bptsDataC |
                       bptsDataW |
                       bptsDataR |
                       bptsDataExec;

    GetMachineType(&lpi->Processor);

    lpi->fDMInfoCacheable = TRUE;

    //
    // hack so that TL can call tlfGetVersion before
    // reply buffer is initialized.
    //
    if ( cb >= (FIELD_OFFSET(DBB, rgbVar) + sizeof(DMINFO)) ) {
        memcpy(lpdbb->rgbVar, lpi, sizeof(DMINFO));
    }

    Reply( sizeof(DMINFO), LpDmMsg, lpdbb->hpid );
}                                        /* ProcessGetDMInfoCmd */



VOID
ActionFunctionCallComplete(
    LPDEBUG_EVENT64 pde,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    )
/*++

Routine Description:

    This is the action function which is always called when a debuggee
    function call which was initiated by CallFunction completes.

    This retrieves the return value, if any, restores the thread context,
    and calls the function provided to CallFunction.

Arguments:


Return Value:


--*/
{
    PCALLSTRUCT pcs = (PCALLSTRUCT)lparam;
    DWORDLONG qw = pcs->HasReturnValue ? GetFunctionResult(pcs) : 0;
    ACVECTOR Action = pcs->Action;
    DWORDLONG lparam1 = pcs->lparam;

    RemoveBP(pcs->pbp);

    hthd->context = pcs->context;
    hthd->fContextDirty = TRUE;
    hthd->pcs = NULL;
    free(pcs);

    if (Action) {
        (*Action)(pde, hthd, qw, lparam1);
    }
}



BOOL
WINAPIV
CallFunction(
    HTHDX       hthd,
    ACVECTOR    Action,
    LPARAM      lparam,
    BOOL        HasReturnValue,
    DWORDLONG   Function,
    int         cArgs,
    ...
    )
/*++

Routine Description:

    Call a function in the debuggee.  When the function returns, the action
    function will be called with the return value.

Arguments:

    hthd - Supplies the thread that will be used

    Action - Supplies the DM's completion function

    lparam -

    HasReturnValue - Supplies flag for whether to expect a result

    Function - Supplies the address of the function to call

    cArgs - Supplies the number of args to the function

    ... - Supplies arguments.  There must be cArgs arguments.

Return Value:

    TRUE if call was initiated, FALSE if it failed

--*/
{
    PCALLSTRUCT pcs;
    ADDR        addr;
    va_list     vargs;

    //
    // Don't try to do this with a 16 bit thread.
    //

    assert(!hthd->fWowEvent);

    //
    // can't nest 'em
    //

    assert(!hthd->pcs);


    pcs = malloc(sizeof(*pcs));
    assert(pcs || !"malloc failed in CallFunction");
    if (!pcs) {
        return 0;
    }

    //
    // Remember the current context
    //
    hthd->pcs = pcs;
    pcs->context = hthd->context;

    //
    // Remember the action function, and what do do when
    // function returns.
    //
    pcs->Action = Action;
    pcs->HasReturnValue = HasReturnValue;
    pcs->lparam = lparam;


    //
    // set a BP on the current PC, and register a persistent
    // expected event to catch it later.
    //

    AddrInit(&addr, 0, 0, PC(hthd), TRUE, TRUE, FALSE, FALSE);
    pcs->pbp = SetBP( hthd->hprc, hthd, bptpExec, bpnsStop, &addr, (HPID) INVALID);

    //
    // don't try to step off of BP.
    //
    pcs->atBP = hthd->atBP;
    hthd->atBP = NULL;

    RegisterExpectedEvent(
            hthd->hprc,
            hthd,
            BREAKPOINT_DEBUG_EVENT,
            (UINT_PTR)pcs->pbp,
            NULL,
            ActionFunctionCallComplete,
            TRUE,
            (UINT_PTR)pcs);

    //
    // do machine dependent part
    //
    //  GetCurrentThread() returns a token which means "this thread"
    //  regardless of context.
    //

    va_start(vargs, cArgs);
    vCallFunctionHelper(hthd, Function, cArgs, vargs);
    va_end(vargs);

    return TRUE;

}


VOID
ActionResumeThread(
    DEBUG_EVENT64 * pde,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    )
{
    //
    // This thread just fell out of SuspendThread.
    // The context has been restored; just continue.
    //

    ContinueThread(hthd);
}



BOOL
MakeThreadSuspendItself(
    HTHDX   hthd
    )
/*++

Routine Description:

    Set up the thread to call SuspendThread.  This relies on kernel32
    being present in the debuggee, and the current implementation gives
    up if the thread is in a 16 bit context.

Arguments:

    hthd    - Supplies thread

Return Value:

    TRUE if the thread will be suspended, FALSE if not.

--*/
{
    HANDLE          hdll;
    FARPROC         lpSuspendThread;

    //
    // the only time this should fail is when the debuggee
    // does not use kernel32, which is rare.
    //

    if (!hthd->hprc->dwKernel32Base) {

        if (sizeof(hthd->hprc->dwKernel32Base) == sizeof(DWORD64) ) {
            assert( Is64PtrSE(hthd->hprc->dwKernel32Base) );
        }

        DPRINT(1, ("can't suspend thread %p: Kernel32 not loaded\n", hthd));
        DMPrintShellMsg("*** Unable to suspend thread.\n");
        return 0;
    }

    //
    // Oh, yeah... don't try to do this with a 16 bit thread, either.
    // maybe someday...
    //

    if (hthd->fWowEvent) {
        DMPrintShellMsg("*** Can't leave 16 bit thread suspended.\n");
        return 0;
    }

    //
    // find the address of SuspendThread
    //

    hdll = GetModuleHandle("KERNEL32");
    assert(hdll || !"kernel32 not found in DM!!!");
    if (!hdll) {
        return 0;
    }

    lpSuspendThread = GetProcAddress(hdll, "SuspendThread");
    assert(lpSuspendThread || !"SuspendThread not found in kernel32!!!");
    if (!lpSuspendThread) {
        return 0;
    }

    //
    // this is probably unneccessary, because I think kernel32
    // may not be relocated.
    //
    lpSuspendThread = (FARPROC)((UINT_PTR)lpSuspendThread - (UINT_PTR)hdll
                                                 + hthd->hprc->dwKernel32Base);

    //
    //  GetCurrentThread() returns a token which means "this thread"
    //  regardless of context.
    //

    return CallFunction(hthd,
                        ActionResumeThread,
                        0,
                        TRUE,
                        (UINT_PTR)lpSuspendThread,
                        1,
                        GetCurrentThread());
}

VOID
GetActiveFibers (
    HPRCX hprc,
    LPVOID *buf
    )
{
    HTHDX hthd;
    TEB teb;
    int ct = 0;
    int cbr;

    //
    // Determine the fiber loaded on each thread
    // Don't display that fiber
    //

    for (hthd = hprc->hthdChild; hthd; hthd = hthd->nextSibling) {
        DbgReadMemory(hprc,
                      hthd->offTeb,
                      (LPVOID)&teb,
                      sizeof(teb),
                      &cbr);
        DbgReadMemory(hprc,
                      (UINT_PTR)teb.NtTib.Self,
                      (LPVOID)&teb,
                      sizeof(teb),
                      &cbr);
        buf[ct++] = teb.NtTib.FiberData;
    }

}


VOID
LocalProcessSystemServiceCmd(
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
    LPSSS       lpsss =  (LPSSS) lpdbb->rgbVar;

    switch( lpsss->ssvc )
        {
        case ssvcGetProcessHandle:
            LpDmMsg->xosdRet = xosdNone;
            *((HANDLE *)LpDmMsg->rgb) = hprc->rwHand;
            Reply( sizeof(HANDLE), LpDmMsg, lpdbb->hpid );
            return;

        case ssvcGetThreadHandle:
            LpDmMsg->xosdRet = xosdNone;
            *((HANDLE *)LpDmMsg->rgb) = hthd->rwHand;
            Reply( sizeof(HANDLE), LpDmMsg, lpdbb->hpid );
            return;

        case ssvcOleRpc:
        {
            BOOL    fStartOrpcDebugging = lpsss->rgbData [0];

            switch (hprc->OrpcDebugging) {
                case ORPC_NOT_DEBUGGING:

                    if (fStartOrpcDebugging) {
                        hprc->OrpcDebugging = ORPC_START_DEBUGGING;
                    }

                    // ignore stop debugging request when we arent debugging

                    break;

                case ORPC_DEBUGGING:

                    if (!fStartOrpcDebugging)
                        hprc->OrpcDebugging = ORPC_STOP_DEBUGGING;

                    // ignore start debugging request when we arent debugging

                    break;

                case ORPC_STOP_DEBUGGING:

                    if (fStartOrpcDebugging)
                        hprc->OrpcDebugging = ORPC_DEBUGGING;

                    // ignore redundant stop debugging request

                    break;

                case ORPC_START_DEBUGGING:

                    if (!fStartOrpcDebugging)
                        hprc->OrpcDebugging = ORPC_NOT_DEBUGGING;

                    // ignore redundant start debugging requst

                    break;

                default:
                    assert (FALSE);
            }
        }

        LpDmMsg->xosdRet = xosdNone;
        Reply (0, LpDmMsg, lpdbb->hpid);
        break;

#if SQLDBG
        case ssvcSqlDebug:
            LpDmMsg->xosdRet = DMSqlSystemService( hprc, lpsss->rgbData );
            Reply(0, LpDmMsg, lpdbb->hpid);
            break;
#endif
        case ssvcFiberDebug:
            {
                HFBRX hfbr = hprc->FbrLst;
                OFBRS   ofbrs = *((OFBRS *) lpsss->rgbData);
                DWORD iAfbrs;
                LPVOID *Actvfbrs;
                int cb=0;
                iAfbrs = NumberOfThreadsInProcess(hthd->hprc);
                Actvfbrs = malloc(iAfbrs*sizeof(LPVOID));
                GetActiveFibers(hthd->hprc,Actvfbrs);

                if(ofbrs.op == OFBR_SET_FBRCNTX){
                    hprc->pFbrCntx = ofbrs.FbrCntx;
                } else if(ofbrs.op == OFBR_ENABLE_FBRS) {
                    hprc->fUseFbrs = TRUE;
                } else if(ofbrs.op == OFBR_DISABLE_FBRS) {
                    hprc->fUseFbrs = FALSE;
                } else if(ofbrs.op == OFBR_QUERY_LIST_SIZE) {
                    cb = sizeof(int);
                    //count size of the list of fibers
                    while(hfbr){
                        DWORD i;
                        BOOL fskip;//skip fibers loaded in threads
                        for(fskip = FALSE,i=0;i < iAfbrs; i++){
                            if(Actvfbrs[i] == hfbr->fbrstrt)
                                fskip = TRUE;
                        }
                        if(!fskip){
                            cb +=4;
                        }
                        hfbr = hfbr->next;
                    }
                    //put byte count at the beginning
                    memcpy(lpsss->rgbData+sizeof(int),&cb,sizeof(int));
                    cb = 2*sizeof(int);
                    memcpy(lpsss->rgbData,&cb,sizeof(int));
                    lpsss->cbReturned = cb;
                } else if(ofbrs.op == OFBR_GET_LIST){
                    cb = sizeof(int);
                    while(hfbr){
                        BOOL fskip;//skip fibers loaded in threads
                        DWORD i;
                        for(fskip = FALSE,i=0;i<iAfbrs;i++){
                            if(Actvfbrs[i] == hfbr->fbrstrt)
                                fskip = TRUE;
                        }
                        if(!fskip){
                            memcpy(lpsss->rgbData+cb,&(hfbr->fbrcntx),4);
                            cb +=4;
                        }
                        hfbr = hfbr->next;
                    }
                    //put byte count at the beginning
                    memcpy(lpsss->rgbData,&cb,sizeof(int));
                    lpsss->cbReturned = cb;
                }

                LpDmMsg->xosdRet = xosdNone;
                memcpy(LpDmMsg->rgb,lpsss->rgbData,cb);
                Reply (cb, LpDmMsg, lpdbb->hpid);
                free(Actvfbrs);
            }
            break;

        default:
            LpDmMsg->xosdRet = xosdUnsupported;
            Reply(0, LpDmMsg, lpdbb->hpid);
            return;
    }
}                               /* LocalProcessSystemServiceCmd() */


VOID
ProcessIoctlGenericCmd(
    HPRCX   hprc,
    HTHDX   hthd,
    LPDBB   lpdbb
    )
{
    LPSSS              lpsss  = (LPSSS)lpdbb->rgbVar;

    PIOCTLGENERIC      InputPig = (PIOCTLGENERIC)lpsss->rgbData;
    PVOID              InputBuffer = InputPig->data;
    DWORD              InputType = InputPig->ioctlSubType;

    DWORD              ReplyXosd;
    DWORD              ReplyLength;
    PVOID              ReplyBuffer;
    PIOCTLGENERIC      ReplyPig = (PIOCTLGENERIC)LpDmMsg->rgb;

    ADDR               addr;
    PDETOSAVE          pDeToSave;
    PGET_TEB_ADDRESS   Gta;

    *ReplyPig = *InputPig;
    ReplyBuffer = ReplyPig->data;
    ReplyLength = 0;

    ReplyXosd = xosdNone;

    switch( InputPig->ioctlSubType ) {
        case IG_DEBUG_EVENT:
            pDeToSave = GetMostRecentDebugEvent(hthd->hprc->pid, hthd->tid);

            if (!pDeToSave) {
                memset( ReplyBuffer, 0, sizeof(DEBUG_EVENT) );
                // No error; just nothing to return
                ReplyXosd = xosdQueueEmpty;
            } else {
                memcpy( ReplyBuffer, &pDeToSave->de, sizeof(DEBUG_EVENT) );
                ReplyXosd = xosdNone;
            }
            ReplyLength = sizeof(DEBUG_EVENT);
            break;

        case IG_TRANSLATE_ADDRESS:
            memcpy( &addr, InputBuffer, sizeof(addr) );
            if (TranslateAddress( hprc, hthd, &addr, TRUE )) {
                memcpy( ReplyBuffer, &addr, sizeof(addr) );
                ReplyLength = sizeof(addr);
                ReplyXosd = xosdNone;
            } else {
                ReplyXosd = xosdUnknown;
            }
            break;

        case IG_WATCH_TIME:
            WtRangeStep( hthd );
            break;

        case IG_WATCH_TIME_STOP:
        case IG_WATCH_TIME_RECALL:
        case IG_WATCH_TIME_PROCS:
            WtStruct.fWt = TRUE;
            WtStruct.dwType = InputType;
            WtStruct.hthd = hthd;
            break;

        case IG_GET_TEB_ADDRESS:

            Gta = (PGET_TEB_ADDRESS)ReplyBuffer;

            if (hthd->offTeb == 0) {
                ReplyXosd = xosdUnknown;
            } else {
                Gta->Address = hthd->offTeb;
                ReplyLength = sizeof(*Gta);
                ReplyXosd = xosdNone;
            }

            break;

        case IG_TASK_LIST:
        {
            DWORD TasksReturned;
            DWORD MaxTasks;

            if (CrashDump) {

                ReplyXosd = xosdUnknown;

            } else {

                //
                // get max parameter
                //
                MaxTasks = ((PTASK_LIST)InputBuffer)->dwProcessId;

                //
                // copy header to return buffer
                //

                GetTaskList( (PTASK_LIST)ReplyBuffer, MaxTasks, &TasksReturned );
                ReplyLength = sizeof(TASK_LIST) * TasksReturned;
                ReplyXosd = xosdNone;
            }
        }
        break;

        case IG_RELOAD:
            AddQueue( QT_RELOAD_MODULES,
                      hthd->hprc->pid,
                      hthd->tid,
                      (UINT_PTR)InputPig->data,
                      _tcslen((LPSTR)InputPig->data)+1 );
            ReplyLength = 0;
            break;

        case IG_GET_OS_VERSION:
        {
            PDBG_GET_OS_VERSION posv = (PDBG_GET_OS_VERSION)ReplyPig->data;

            memset(posv, 0, sizeof(*posv));

            posv->osi.dwOSVersionInfoSize = sizeof(posv->osi);
            GetVersionEx(&posv->osi);
            GetSystemInfo(&posv->si);

            if (VER_PLATFORM_WIN32_NT == posv->osi.dwPlatformId) {
                HKEY hkey = NULL;
                TCHAR sz[20] = {0};
                DWORD dwType;
                DWORD dwSize = sizeof(sz);

                if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                                "Software\\Microsoft\\Windows NT\\CurrentVersion",
                                                0,
                                                KEY_READ,
                                                &hkey)) {

                    if (ERROR_SUCCESS == RegQueryValueEx(hkey,
                                                        "CurrentType",
                                                        NULL,
                                                        &dwType,
                                                        (PUCHAR) sz,
                                                        &dwSize)) {

                        if (*sz) {
                            _tcslwr(sz);
                            posv->fChecked = (NULL != _tcsstr(sz, "checked"));
                        }
                    }

                    RegCloseKey(hkey);
                }
            }
            ReplyLength = sizeof(*posv);
        }
        break;

        case IG_GET_PROCESS_PARAMETERS:
        {
            TEB Teb;
            PEB Peb;
            PRTL_USER_PROCESS_PARAMETERS64 upp = (PRTL_USER_PROCESS_PARAMETERS64)ReplyPig->data;
            RTL_USER_PROCESS_PARAMETERS32 upp32Tmp;
            DWORD dw;

            ReplyXosd = xosdUnknown;

            //
            // get the peb and find the RTL_USER_PROCESS_PARAMETERS
            //

            if (IsChicago()) {
                break;
            }

            if (!hprc->PebAddress) {

                //
                // grab the first thread and read its teb
                //

                if (!hthd) {
                    hthd = hprc->hthdChild;
                }

                if ( hthd &&
                     hthd->offTeb &&
                     DbgReadMemory(hprc, hthd->offTeb, &Teb, sizeof(Teb), &dw)) {

                    hprc->PebAddress = (UOFFSET)Teb.ProcessEnvironmentBlock;

                } else {

                    break;

                }
            }

            //
            // read the peb
            //

            if (!DbgReadMemory(hprc, hprc->PebAddress, &Peb, sizeof(Peb), &dw)) {
                break;
            }

            //
            // copy some info
            //

#ifdef _WIN64
            if (!DbgReadMemory(hprc, (ULONG64)Peb.ProcessParameters, upp,
                sizeof(RTL_USER_PROCESS_PARAMETERS), &dw)) {

                break;
            }
#else       
            if (!DbgReadMemory(hprc, (DWORDLONG) Peb.ProcessParameters, &upp32Tmp,
                sizeof(RTL_USER_PROCESS_PARAMETERS32), &dw)) {

                break;
            }

            UserProcessParameters32To64(&upp32Tmp, upp);
#endif

            ReplyLength = sizeof(RTL_USER_PROCESS_PARAMETERS64);
            ReplyXosd = xosdNone;
        }
        break;


        default:
            ReplyXosd = xosdUnknown;
            break;
    }

    ReplyPig->length = ReplyLength;
    LpDmMsg->xosdRet = ReplyXosd;
    Reply(ReplyLength + sizeof(IOCTLGENERIC), LpDmMsg, lpdbb->hpid );

    return;
}


VOID
ProcessSSVCCustomCmd(
    HPRCX   hprc,
    HTHDX   hthd,
    LPDBB   lpdbb
    )
{
    LPSSS   lpsss = (LPSSS)lpdbb->rgbVar;
    LPTSTR   p    = (LPTSTR) lpsss->rgbData;


    LpDmMsg->xosdRet = xosdUnsupported;

    //
    // parse the command
    //
    while (*p && !_istspace(*p)) {
        p = _tcsinc(p);
    }
    if (*p) {
        *p = _T('\0');
                p = _tcsinc(p);
    }

    //
    // this is what the code should look like:
    //
    // at this point the 'p' variable points to any arguments
    // to the dot command
    //
    //      if (_tcsicmp( lpsss->rgbData, "dot-command" ) == 0) {
    //          -----> do your thing <------
    //          LpDmMsg->xosdRet = xosdNone;
    //      }
    //
    if ( !_ftcsicmp((LPTSTR)lpsss->rgbData, _T("FastStep")) ) {
        fSmartRangeStep = TRUE;
        LpDmMsg->xosdRet = xosdNone;
    } else if ( !_ftcsicmp((LPTSTR)lpsss->rgbData, _T("SlowStep")) ) {
        fSmartRangeStep = FALSE;
        LpDmMsg->xosdRet = xosdNone;
    }

    //
    // send back our response
    //
    Reply(0, LpDmMsg, lpdbb->hpid);
}                             /* ProcessSSVCCustomCmd() */




DWORD WINAPI
DoTerminate(
    LPVOID lpv
    )
{
    HPRCX hprcx = (HPRCX)lpv;

    if (CrashDump) {
        ProcessUnloadCmd(hprcx, NULL, NULL);
        return 0;
    }

#ifdef TARGET_i386
    ClearAllDebugRegisters(hprcx);                              // just in case Win95 gets confused
#endif

    TerminateProcess(hprcx->rwHand, 1);

    //
    // now that TerminateThread has completed, put priority
    // back before calling out of DM
    //

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

    WaitForSingleObject(hprcx->hExitEvent, INFINITE);

    ProcessUnloadCmd(hprcx, NULL, NULL);

    return  0;
}

VOID
CompleteTerminateProcessCmd(
    VOID
    )
{
    DEBUG_EVENT64 devent, *de=&devent;
    HANDLE      hThread;
    DWORD       dwTid;
    BREAKPOINT *pbpT;
    BREAKPOINT *pbp;
    PKILLSTRUCT pk;
    HPRCX       hprc;
    HTHDX       hthd;

    DEBUG_PRINT("CompleteTerminateProcessCmd");

    EnterCriticalSection(&csKillQueue);

    pk = KillQueue;
    if (pk) {
        KillQueue = pk->next;
    }

    LeaveCriticalSection(&csKillQueue);

    assert(pk);
    if (!pk) {
        return;
    }

    hprc = pk->hprc;
    free(pk);

    ConsumeAllProcessEvents(hprc, TRUE);

    /*
     * see if process is already dead
     */

    if ((hprc->pstate & ps_dead) || (hprc->rwHand == (HANDLE)INVALID)) {

        //
        // Queue a continue if any thread is stopped
        //

        for (hthd = hprc->hthdChild; hthd; hthd = hthd->nextSibling) {
            if (hthd->tstate & ts_stopped) {
                hthd->fExceptionHandled = TRUE;
                ContinueThread(hthd);
            }
        }

        ProcessUnloadCmd(hprc, NULL, NULL);

    } else {

        RemoveAllHprcBP(hprc);

        //
        // Start another thread to kill the thing.  This thread needs to
        // continue any threads which are stopped.  The new thread will then
        // wait until this one (the poll thread) has handled all of the
        // events, and then send destruction notifications to the shell.
        //

        hThread = CreateThread(NULL,
                               4096,
                               DoTerminate,
                               (LPVOID)hprc,
                               0,
                               &dwTid);
        assert(hThread);
        if ( !hThread ) {
            return;
        }

        //
        //  Yield so DoTerminate can do its thing before we start posting
        //  ContinueDebugEvents, so we minimize the time that the app
        //  runs before it is terminated.
        //

        hprc->pstate |= ps_killed;

        // setting the priority to TIME_CRITICAL causes Win95 to hang
        // DS96:6580 so we are less aggressive
        if (!IsChicago()) {
            SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL );
        }

        Sleep(0);

        CloseHandle(hThread);

        //
        // Queue a continue if any thread is stopped
        //

        for (hthd = hprc->hthdChild; hthd; hthd = hthd->nextSibling) {
            if (hthd->tstate & ts_stopped) {
                hthd->fExceptionHandled = TRUE;
                ContinueThread(hthd);
            }
        }

    }

}


DWORD
ProcessTerminateProcessCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    PKILLSTRUCT pk;

    if (!hprc) {
        return FALSE;
    }

    Unreferenced( lpdbb );

    pk = (PKILLSTRUCT)malloc(sizeof(KILLSTRUCT));
    pk->hprc = hprc;

    EnterCriticalSection(&csKillQueue);

    pk->next = KillQueue;
    KillQueue = pk;

    LeaveCriticalSection(&csKillQueue);

    return TRUE;
}


VOID
ProcessAllProgFreeCmd(
    HPRCX hprcXX,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    HPRCX hprc;

    Unreferenced(hprcXX);
    Unreferenced(hthd);

    for (;;) {

        EnterCriticalSection(&csThreadProcList);
        for (hprc = prcList; hprc; hprc = hprc->next) {
            if (hprc->pstate != (ps_root | ps_destroyed)) {
                break;
            }
        }
        LeaveCriticalSection(&csThreadProcList);

        if (hprc) {
            ProcessTerminateProcessCmd(hprc, hthd, lpdbb);
            ProcessUnloadCmd(hprc, hthd, lpdbb);
        } else {
            break;
        }

    }

    WaitForSingleObject(hEventNoDebuggee, INFINITE);
}


DWORD
ProcessAsyncGoCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    XOSD       xosd = xosdNone;
    DEBUG_EVENT64 de;

    DEBUG_PRINT("ProcessAsyncGoCmd called.\n\r");

    if ((hthd->tstate & ts_frozen)) {
        if (hthd->tstate & ts_stopped) {
            //
            // if at a debug event, it won't really be suspended,
            // so just clear the flag.
            //
            hthd->tstate &= ~ts_frozen;

        } else if (ResumeThread(hthd->rwHand) == -1L ) {

            xosd = xosdBadThread;

        } else {

            hthd->tstate &= ~ts_frozen;

            /*
             * deal with dead, frozen, continued thread:
             */
            if ((hthd->tstate & ts_dead) && !(hthd->tstate & ts_stopped)) {

                de.dwDebugEventCode = DESTROY_THREAD_DEBUG_EVENT;
                de.dwProcessId = hprc->pid;
                de.dwThreadId = hthd->tid;
                NotifyEM(&de, hthd, 0, 0);
                FreeHthdx(hthd);

                hprc->pstate &= ~ps_deadThread;
                for (hthd = hprc->hthdChild; hthd; hthd = hthd->nextSibling) {
                    if (hthd->tstate & ts_dead) {
                        hprc->pstate |= ps_deadThread;
                    }
                }

            }
        }
    }

    Reply(0, &xosd, lpdbb->hpid);
    return(xosd);
}


void
ActionAsyncStop(
    DEBUG_EVENT64 * pde,
    HTHDX           hthd,
    DWORDLONG       unused,
    DWORDLONG       lparam
    )
/*++

Routine Description:

    This routine is called if a breakpoint is hit which is part of a
    Async Stop request.  When hit is needs to do the following:  clean
    out any expected events on the current thread, clean out all breakpoints
    which are setup for doing the current async stop.

Arguments:

    pde - Supplies a pointer to the debug event which just occured

    hthd - Supplies a pointer to the thread for the debug event

    pbp  - Supplies a pointer to the breakpoint for the ASYNC stop

Return Value:

    None.

--*/

{
    BPR         bpr;
    HPRCX       hprc = hthd->hprc;
    BREAKPOINT * pbpT;
    BREAKPOINT * pbp;
    BREAKPOINT * pbpStop = (PBREAKPOINT)lparam;

    //
    // remove all expected events we set up for this
    // (which also removes their breakpoints).
    // Just removing the bps causes heap corruption,
    // as ConsumeAllProcessEvents will delete the outstanding EEs
    // and their BPs, which we don't want to do twice.
    // Note: don't delete this expected event as the caller does that
    //

    for (pbp = BPNextHprcPbp(hprc, NULL); pbp != NULL; pbp = pbpT) {

        pbpT = BPNextHprcPbp(hprc, pbp);

        if ( (pbp!=pbpStop) && (pbp->id == (HPID)ASYNC_STOP_BP) ) {
            // get the expected event off the queue, free it, and free the bp
            EXPECTED_EVENT *ee = PeeIsEventExpected(NULL, BREAKPOINT_DEBUG_EVENT, (UINT_PTR)pbp, TRUE);
            if (ee) {
                MHFree(ee);
            }
            RemoveBP(pbp);
        }
    }

    //
    //  We no longer need to have this breakpoint set (our EE was not freed above)
    //

    RemoveBP( pbpStop );

    //
    // Setup a return packet which says we hit an async stop breakpoint
    //

#ifdef TARGET_i386
    bpr.segCS  = (SEGMENT) hthd->context.SegCs;
    bpr.segSS  = (SEGMENT) hthd->context.SegSs;
    bpr.offEBP = (UOFFSET) hthd->context.Ebp;
#endif

    bpr.offEIP = PC(hthd);

    bpr.fFlat  =  hthd->fAddrIsFlat;
    bpr.fOff32 =  hthd->fAddrOff32;
    bpr.fReal  =  hthd->fAddrIsReal;

    DMSendDebugPacket(dbcAsyncStop,
                      hprc->hpid,
                      hthd->htid,
                      sizeof(BPR),
                      &bpr
                      );
    return;
}                               /* ActionAsyncStop() */



VOID
ProcessAsyncStopCmd(
    HPRCX       hprc,
    HTHDX       hthd,
    LPDBB       lpdbb
    )

/*++

Routine Description:

    This function is called in response to a asynchronous stop request.
    In order to do this we will set breakpoints the current PC for
    every thread in the system and wait for the fireworks to start.

Arguments:

    hprc        - Supplies a process handle

    hthd        - Supplies a thread handle

    lpdbb       - Supplies the command information packet

Return Value:

    None.

--*/

{
    CONTEXT     regs;
    BREAKPOINT * pbp;
    ADDR        addr;
    BOOL        fSetFocus = * ( BOOL *) lpdbb->rgbVar;

    //
    // If we are debugging a dump file, don't
    // bother trying to stop it. Just reply to it as
    // if it were sucessful.
    //
    if (CrashDump) {
        LpDmMsg->xosdRet = xosdNone;
        Reply(0, LpDmMsg, lpdbb->hpid);
        return;
    }

    regs.ContextFlags = CONTEXT_CONTROL;


    //
    //  Step 1.  Enumerate through the threads and freeze them all.
    //

    for (hthd = hprc->hthdChild; hthd != NULL; hthd = hthd->nextSibling) {
        if (SuspendThread(hthd->rwHand) == -1L) {
            ; // Internal error;
        }
    }

    //
    //  Step 2.  Place a breakpoint on every PC address
    //

    for (hthd = hprc->hthdChild; hthd != NULL; hthd = hthd->nextSibling) {
        DbgGetThreadContext( hthd, &regs );

        AddrInit(&addr, 0, 0, cPC(&regs), TRUE, TRUE, FALSE, FALSE);
        pbp = SetBP(hprc, hthd, bptpExec, bpnsStop, &addr, (HPID) ASYNC_STOP_BP);

        if (!pbp) {
            assert ( IsChicago() ); // UNDONE: deal with error
            continue;
        }

        RegisterExpectedEvent(hthd->hprc,
                              hthd,
                              BREAKPOINT_DEBUG_EVENT,
                              (UINT_PTR)pbp,
                              DONT_NOTIFY,
                              ActionAsyncStop,
                              FALSE,
                              (UINT_PTR)pbp);
    }

    //
    //  Step 3.  Unfreeze all threads
    //

    if (fSetFocus) {
        DmSetFocus(hprc);
    }

        for (hthd = hprc->hthdChild; hthd != NULL; hthd = hthd->nextSibling) {
        if (ResumeThread(hthd->rwHand) == -1) {
            ; // Internal error
        }

        //
        // post a dummy message so we will see the BP
        //
        PostThreadMessage (hthd->tid, WM_NULL, 0, 0);
    }

    LpDmMsg->xosdRet = xosdNone;
    Reply(0, LpDmMsg, lpdbb->hpid);
    return;
}                            /* ProcessAsyncStopCmd() */


VOID
ProcessDebugActiveCmd(
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    LPDAP lpdap = ((LPDAP)(lpdbb->rgbVar));

    Unreferenced(hprc);
    Unreferenced(hthd);

    if (FDMRemote && !fDisconnected) {

        SetEvent( hEventRemoteQuit );

    } else if (!StartDmPollThread()) {

        //
        // CreateThread() failed; fail and send a dbcError.
        //
        LpDmMsg->xosdRet = xosdUnknown;
        Reply(0, LpDmMsg, lpdbb->hpid);


        //
        // Wait for previous attach to complete or fail:
        //
    } else if (WaitForSingleObject(DebugActiveStruct.hEventReady, INFINITE) != 0) {
        //
        // the wait failed.  why?  are there cases where we
        // should restart the wait?
        //
        LpDmMsg->xosdRet = xosdUnknown;
        Reply(0, LpDmMsg, lpdbb->hpid);

    } else {

        *nameBuffer = 0;

        ResetEvent(DebugActiveStruct.hEventReady);
        ResetEvent(DebugActiveStruct.hEventApiDone);

        DebugActiveStruct.dwProcessId = lpdap->dwProcessId;
        DebugActiveStruct.hEventGo    = lpdap->hEventGo;

        //
        // Open a waitable handle to the process.  The poll thread
        // will use this to determine whether the target process has
        // exited before the debugger was connected.
        //
        if (lpdap->dwProcessId != 0 &&
            lpdap->dwProcessId != (UINT)-1) {

            DebugActiveStruct.hProcess = OpenProcess(STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE,
                                                     FALSE,
                                                     lpdap->dwProcessId
                                                     );
        } else {
            DebugActiveStruct.hProcess = INVALID_HANDLE_VALUE;
        }

        //
        //      *** Do this last!! ***
        //
        DebugActiveStruct.fAttach     = TRUE;
        //
        //      **********************
        //

        //
        // Wait for poll thread to complete the API call:
        //

        WaitForSingleObject(DebugActiveStruct.hEventApiDone, INFINITE);

        if (DebugActiveStruct.fReturn != 0) {

            LpDmMsg->xosdRet = xosdNone;
            //
            // the poll thread will reply when creating the "root" process.
            // we reply when this is really a reattach.
            //
            if (!fUseRoot || lpdap->dwProcessId == 0) {
                Reply(0, LpDmMsg, lpdbb->hpid);
            }

        } else {
            //
            // DebugActiveProcess failed
            //
            if (DebugActiveStruct.hProcess != INVALID_HANDLE_VALUE) {
                CloseHandle(DebugActiveStruct.hProcess);
            }
            SetEvent(DebugActiveStruct.hEventReady);
            LpDmMsg->xosdRet = xosdUnknown;
            Reply(0, LpDmMsg, lpdbb->hpid);
        }
    }
}


// Non local goto support

/*** ACTIONNLGDISPATCH
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *
 * RETURN:
 *
 * DESCRIPTION:
 *
 ****************************************************************************/

VOID
ActionNLGDispatch (
    DEBUG_EVENT64* pde,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    )
{
    ADDR                addrPC, addrReturn;
    HNLG                hnlg;
    LPNLG               lpnlg;
    NLG_DESTINATION     nlgDest;
    XOSD                xosd;
    BOOL                fStop = FALSE;
    DWORD               cb;
    BREAKPOINT          *bp = (BREAKPOINT*)lparam;
    METHOD              *ContinueSSMethod;

    assert(bp);

    AddrFromHthdx(&addrPC, hthd);

    hnlg = CheckNLG ( hthd->hprc, hthd, NLG_DISPATCH, &addrPC );
    assert ( hnlg );
    ClearBPFlag(hthd);

    RestoreInstrBP(hthd, bp);

    lpnlg = LLLock ( hnlg );

    addrReturn = lpnlg->addrNLGReturn;

    xosd = AddrReadMemory (
        hthd->hprc,
        hthd,
        &lpnlg->addrNLGDestination,
        &nlgDest,
        sizeof ( NLG_DESTINATION ),
        &cb
        );

    LLUnlock ( hnlg );

    if ( ( nlgDest.dwSig == NLG_SIG ) &&
        ( nlgDest.dwCode != NLG_DESTRUCTOR_ENTER )
    ) {
        BREAKPOINT* bp;
        HPID    hpid  = hthd->hprc->hpid;
        CANSTEP CanStep;
        ADDR    addrSP;

        GetAddrOff ( addrSP ) = GetSPFromNLGDest(hthd, &nlgDest);

        DPRINT(2, ("NLG Dispatch old SP=%I64x, new SP=%I64x", 
               GetAddrOff ( hthd->addrStack ) , 
               GetAddrOff( addrSP )
               ));
        if ( GetAddrOff ( hthd->addrStack ) < GetAddrOff ( addrSP ) ) {

            ConsumeAllProcessEvents ( hthd->hprc, FALSE );
            fStop = TRUE;

            /*
            ** Set breakpoint at the destination, which we'll hit after it's done
            */

            SetAddrOff ( &addrPC, nlgDest.uoffDestination);

#ifndef TARGET_i386
            GetCanStep(hthd->hprc->hpid, hthd->htid, &addrPC, &CanStep);

            switch ( CanStep.Flags ) {

                case CANSTEP_YES:
                    GetAddrOff(addrPC) += CanStep.PrologOffset;
                    break;

                default:
                    fStop = FALSE;
//                  assert(FALSE);
                    break;
            }
#endif

            if (fStop) {
                bp = SetBP(hthd->hprc, hthd, bptpExec, bpnsStop, &addrPC, (HPID)INVALID);

                RegisterExpectedEvent (
                    hthd->hprc,
                    hthd,
                    BREAKPOINT_DEBUG_EVENT,
                    (UINT_PTR)bp,
                    DONT_NOTIFY,
                    ActionNLGDestination,
                    FALSE,
                    (UINT_PTR)bp
                    );
            }
        }
    }

    /*
     * Keep ourselves registered. Then Consume will remove this BP
     */
    RegisterExpectedEvent (
        hthd->hprc,
        hthd,
        BREAKPOINT_DEBUG_EVENT,
        (UINT_PTR)lparam,
        DONT_NOTIFY,
        ActionNLGDispatch,
        FALSE,
        lparam
        );

    ContinueSSMethod = (METHOD*)MHAlloc(sizeof(METHOD));
    assert(ContinueSSMethod);
    ContinueSSMethod->notifyFunction = MethodContinueSS;
    ContinueSSMethod->lparam         = (UINT_PTR)ContinueSSMethod;
    ContinueSSMethod->lparam2        = (LPVOID)lparam;
    SingleStep(hthd, ContinueSSMethod, FALSE, FALSE);

} // ActionNLGDispatch

/*** PROCESSDMFNONLOCALGOTO
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *
 * RETURN:
 *
 * DESCRIPTION:
 *
 *      This is called in response to a dmfNonLocalGoto command from the EM.
 *
 ****************************************************************************/

VOID
ProcessNonLocalGoto (
    HPRCX hprc,
    HTHDX hthd,
    LPDBB lpdbb
    )
{
    LPNLG   lpnlg = (LPNLG)lpdbb->rgbVar;
    HNLG    hnlg;
    XOSD    xosd = xosdNone;

    if ( lpnlg->fEnable ) {
        hnlg = LLCreate ( hprc->llnlg );

        if ( !hnlg ) {
            /*
            ** REVIEW: Memory Failure
            */
            assert ( FALSE );
        }
        else {
            LPNLG lpnlgT;
            BREAKPOINT *bp;
            LLAdd ( hprc->llnlg, hnlg );
            lpnlgT = LLLock ( hnlg );

            *lpnlgT = *lpnlg;

            emiAddr ( lpnlgT->addrNLGDispatch ) = 0;
            emiAddr ( lpnlgT->addrNLGReturn ) = 0;
            emiAddr ( lpnlgT->addrNLGReturn2 ) = 0;
            emiAddr ( lpnlgT->addrNLGDestination ) = 0;

            LLUnlock ( hnlg );
        }
    }
    else {
        hnlg = LLFind ( hprc->llnlg, NULL, &lpnlg->hemi, (LONG)nfiHEMI );
        if ( !hnlg ) {
            /*
            ** We better have it otherwise the EM shouldn't be telling us
            ** about to remove it.
            */
            //
            // well, like it or not, this assertion happens constantly.
            //
            //assert ( FALSE );
        } else {
            LPNLG   lpnlgT = LLLock ( hnlg );

            BREAKPOINT *bp = FindBP( hprc, hthd, bptpExec, bpnsStop, &lpnlgT->addrNLGDispatch, TRUE);
            EXPECTED_EVENT *ee = PeeIsEventExpected(NULL, BREAKPOINT_DEBUG_EVENT, (UINT_PTR)bp, TRUE);
            if (ee) {
                ConsumeSpecifiedEvent(ee);
            }

            LLUnlock ( hnlg );

            LLDelete ( hprc->llnlg, hnlg );
        }

    }

    Reply(0, &xosd, lpdbb->hpid);
} // ProcessDmfNonLocalGoto

/***
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *
 * RETURN:
 *
 * DESCRIPTION:
 *
 ****************************************************************************/

INT
WINAPI
NLGComp (
    LPNLG lpnlg,
    LPV lpvKey,
    LONG lParam
    )
{
    NFI nfi = (NFI)lParam;

    switch ( nfi ) {
        case nfiHEMI :
            if ( lpnlg->hemi == *(LPHEMI)lpvKey ) {
                return fCmpEQ;
            } else {
                return fCmpLT;
            }
            break;

        default :
            // Should not reach here
            assert ( FALSE );
            return fCmpGT;
            break;
    }

} // NLGComp
/*** CHECKNLG
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *
 * RETURN:
 *
 * DESCRIPTION:
 *
 ****************************************************************************/

HNLG
CheckNLG (
    HPRCX hprc,
    HTHDX hthd,
    NLG_LOCATION nlgLoc,
    LPADDR lpaddrPC
    )
{
    HNLG    hnlg = hnlgNull;
    HNLG    hnlgRet = hnlgNull;

    while ( !hnlgRet && ( hnlg = LLNext ( hprc->llnlg, hnlg ) ) ) {
                LPNLG   lpnlg = LLLock ( hnlg );
                LPADDR  lpaddr;
                LPADDR  lpaddr2 = NULL;

        switch ( nlgLoc ) {
            case NLG_DISPATCH :
                lpaddr = &lpnlg->addrNLGDispatch;
                break;

            case NLG_RETURN :
                lpaddr = &lpnlg->addrNLGReturn;
                lpaddr2 = &lpnlg->addrNLGReturn2;
                break;

            default :
                assert ( FALSE );
        }

        if ( FAddrsEq ( *lpaddr, *lpaddrPC ) ) {
            hnlgRet = hnlg;
        } else if ((lpaddr2 != NULL) && FAddrsEq ( *lpaddr2, *lpaddrPC ) ) {
            hnlgRet = hnlg;
        }
        LLUnlock ( hnlg );
    }

    return ( hnlgRet );

} // CheckNLG

/*** ACTIONNLGDESTINATION
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *
 * RETURN:
 *
 * DESCRIPTION:
 *
 ****************************************************************************/

VOID
ActionNLGDestination (
    DEBUG_EVENT64* pde,
    HTHDX hthd,
    DWORDLONG unused,
    DWORDLONG lparam
    )
{
    BREAKPOINT* bp = (BREAKPOINT*)lparam;

    assert(bp);
    RemoveBP(bp);
    hthd->fReturning = FALSE;
    ConsumeAllThreadEvents(hthd, FALSE);

    NotifyEM(&falseSSEvent, hthd, 0, 0);
} // ActionNLGDestination

VOID
ProcessFiberEvent(
    DEBUG_EVENT64* pde,
    HTHDX        hthd
    )
{
    HPRCX hprc = hthd->hprc;
    HFBRX hfbr;
    EXCEPTION_DEBUG_INFO64 *dbginfo = (EXCEPTION_DEBUG_INFO64 *) &(pde->u);
    EFBR efbr = (EFBR)dbginfo->ExceptionRecord.ExceptionInformation[0];

    // There are only two cases - Create Fiber and Delete Fiber

    switch(efbr) {
    case ecreate_fiber:

        hfbr = (HFBRX)MHAlloc(sizeof(HFBRXSTRUCT));
        memset(hfbr, 0, sizeof(*hfbr));

        // Add to the process's list of fibers
        hfbr->next = hprc->FbrLst;
        hprc->FbrLst = hfbr;

        // Grab the start and context pointers for the fiber

        hfbr->fbrstrt = (LPVOID)dbginfo->ExceptionRecord.ExceptionInformation[1];
        hfbr->fbrcntx = (LPVOID)dbginfo->ExceptionRecord.ExceptionInformation[2];

        // Have the debuggee continue after the exception
        hthd->fExceptionHandled = TRUE;
        ContinueThread(hthd);
        break;
    case edelete_fiber:
        {
            HFBRX  prevhfbr;
            LPVOID fbrstrt = (LPVOID)dbginfo->ExceptionRecord.ExceptionInformation[1];
            LPVOID fbrcntx = (LPVOID)dbginfo->ExceptionRecord.ExceptionInformation[2];

            hfbr = hprc->FbrLst;
            if((hfbr->fbrstrt == fbrstrt) &&
                    (hfbr->fbrcntx == fbrcntx)){
                    hprc->FbrLst = hfbr->next;
                    MHFree(hfbr);
            } else {
                prevhfbr = hfbr;
                hfbr = hfbr->next;
                while(hfbr){
                    if((hfbr->fbrstrt == fbrstrt) &&
                        (hfbr->fbrcntx == fbrcntx)){
                        prevhfbr->next = hfbr->next;
                        MHFree(hfbr);
                        break;
                    } else {
                        prevhfbr = hfbr;
                        hfbr = hfbr->next;
                    }
                }
            }
            // Have the debuggee continue after the exception
            hthd->fExceptionHandled = TRUE;
            ContinueThread(hthd);
            break;
        }
    }
}

VOID
RemoveFiberList(
    HPRCX hprc
    )
{
    HFBRX hfbr = hprc->FbrLst;
    HFBRX next = NULL;
    while(hfbr){
        next = hfbr->next;
        MHFree(hfbr);
        hfbr = next;
    }
    hprc->FbrLst = NULL;
}


void
ContinueThreadEx(
    HTHDX hthd,
    DWORD ContinueStatus,
    DWORD EventType,
    TSTATEX NewState
    )
{
    //
    // If NewState is ts_running, do the "usual magic" as a special
    // case.  If it is anything else, set the flag to NewState
    //
    if (NewState == ts_running) {
        hthd->tstate &= ~(ts_stopped | ts_first | ts_second);
        hthd->tstate |= ts_running;
    } else {
        hthd->tstate = NewState;
    }

    hthd->fExceptionHandled = FALSE;

    AddQueue (EventType,
              hthd->hprc->pid,
              hthd->tid,
              ContinueStatus,
              0);
}


void
ContinueProcess(
    HPRCX hprc
    )
{
    HTHDX hthd;
    for (hthd = hprc->hthdChild; hthd; hthd = hthd->nextSibling) {
        if (hthd->tstate & ts_stopped) {
            ContinueThread(hthd);
        }
    }
}


void
ContinueThread(
    HTHDX hthd
    )
{
    DWORD ContinueStatus = DBG_CONTINUE;
    if ( (hthd->tstate & (ts_first | ts_second)) &&
         !hthd->fExceptionHandled) {
        ContinueStatus = (DWORD)DBG_EXCEPTION_NOT_HANDLED;
    }
    ContinueThreadEx(hthd,
                     ContinueStatus,
                     QT_CONTINUE_DEBUG_EVENT,
                     ts_running
                     );
}


////////////////////////////////////////////////////////////////////

//  Helper functions for LoadDll.

////////////////////////////////////////////////////////////////////


PTCHAR
CopyFileNameFromDBGFileName(
    PTCHAR Dest,
    PTCHAR Src
    )
{
    TCHAR    rgchPath[_MAX_PATH];
    TCHAR    rgchBase[_MAX_FNAME];
    TCHAR    rgchExt[_MAX_EXT];

    _tsplitpath(Src, NULL, rgchPath, rgchBase, rgchExt);

    if (_tcsicmp(rgchExt, _T(".DBG")) != 0) {
        if (Dest != Src) {
            _tcscpy(Dest, Src);
        }
    } else if (rgchPath[0] && rgchPath[1] &&
              _tcschr(rgchPath, '\\') == &rgchPath[_tcslen(rgchPath)-1]) {
        rgchPath[_tcslen(rgchPath)-1] = 0;
        _tcscpy(Dest, rgchBase);
        _tcscat(Dest, _T("."));
        _tcscat(Dest, rgchPath);
    } else {
        _tcscpy(Dest, rgchBase);
        _tcscat(Dest, _T(".exe"));
    }

    return Dest;
}

//
// MagicModuleId is a magic module ID number for when we have to make
// up a name for a module.
//

MagicModuleId = 0;


BOOL
GetModnameFromImage(
    PIMAGE_NT_HEADERS       pNtHdr,
    PIMAGE_SECTION_HEADER   pSH,
    LOAD_DLL_DEBUG_INFO64 * pldd,
    LPTSTR                  lpName,
    int                     cbName
    )
/*++

Routine Description:

    This routine attempts to get the name of the exe as placed
    in the debug section section by the linker.

Arguments:

    pNtHdr  - Supplies pointer to NT headers in image PE header

    pSH     - Supplies pointer to section headers

    pldd    - Supplies the info structure from the debug event

    lpName  - Returns the exe name

    cbName  - Supplies the size of the buffer at lpName

Return Value:

    TRUE if a name was found, FALSE if not.
    The exe name is returned as an ANSI string in lpName.

--*/
{
    /*
     * See if the exe name is in the image
     */
    PIMAGE_OPTIONAL_HEADER      pOptHdr = &pNtHdr->OptionalHeader;
    PIMAGE_DEBUG_DIRECTORY      pDebugDir;
    IMAGE_DEBUG_DIRECTORY       DebugDir;
    PIMAGE_DEBUG_MISC           pMisc;
    PIMAGE_DEBUG_MISC           pT;
    DWORD                       rva;
    int                         nDebugDirs;
    int                         i;
    int                         l;
    BOOL                        rVal = FALSE;

    nDebugDirs = pOptHdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size /
                 sizeof(IMAGE_DEBUG_DIRECTORY);

    if (!nDebugDirs) {
        return FALSE;
    }

    rva = pOptHdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;

    for (i = 0; i < pNtHdr->FileHeader.NumberOfSections; i++) {
        if (rva >= pSH[i].VirtualAddress
          && rva < pSH[i].VirtualAddress + pSH[i].SizeOfRawData)
        {
            break;
        }
    }

    if (i >= pNtHdr->FileHeader.NumberOfSections) {
        return FALSE;
    }

    //
    // this is a pointer in the debuggee image:
    //
    if (pldd->hFile == 0) {
        pDebugDir = (PIMAGE_DEBUG_DIRECTORY)
                    ((rva - pSH[i].VirtualAddress) + pSH[i].VirtualAddress);
    } else {
        pDebugDir = (PIMAGE_DEBUG_DIRECTORY)
                    (rva - pSH[i].VirtualAddress + pSH[i].PointerToRawData);
    }

    for (i = 0; i < nDebugDirs; i++) {

        SetReadPointer((ULONG)(ULONG_PTR)(&pDebugDir[i]), FILE_BEGIN);
        DoRead((LPV)&DebugDir, sizeof(DebugDir));

        if (DebugDir.Type == IMAGE_DEBUG_TYPE_MISC) {

            l = DebugDir.SizeOfData;
            pMisc = pT = MHAlloc(l);

            if (pldd->hFile == 0) {
                SetReadPointer((ULONG)DebugDir.AddressOfRawData, FILE_BEGIN);
            } else {
                SetReadPointer((ULONG)DebugDir.PointerToRawData, FILE_BEGIN);
            }

            DoRead((LPV)pMisc, l);

            while (l > 0) {
                if (pMisc->DataType != IMAGE_DEBUG_MISC_EXENAME) {
                    l -= pMisc->Length;
                    if (l > (int)DebugDir.SizeOfData) {
                        l = 0; // Avoid AV on bad exe
                        break;
                    }
                    pMisc = (PIMAGE_DEBUG_MISC)
                                (((LPSTR)pMisc) + pMisc->Length);
                } else {

                    PVOID pExeName;

                    pExeName = (PVOID)&pMisc->Data[ 0 ];

#if !defined(_UNICODE)
                    if (!pMisc->Unicode) {
                        _tcscpy(lpName, (LPSTR)pExeName);
                        rVal = TRUE;
                    } else {
                        WideCharToMultiByte(CP_ACP,
                                            0,
                                            (LPWSTR)pExeName,
                                            -1,
                                            lpName,
                                            cbName,
                                            NULL,
                                            NULL);
                        rVal = TRUE;
                    }
#else
                    if (pMisc->Unicode) {
                        wcscpy(lpName, (LPTSTR)pExeName);
                        rVal = TRUE;
                    } else {
                        MultiByteToWideChar(CP_ACP,
                                            0,
                                            (LPTSTR)pExeName,
                                            -1,
                                            lpName,
                                            cbName);
                        rVal = TRUE;
                    }
#endif
                    CopyFileNameFromDBGFileName(lpName, lpName);

                    break;
                }
            }

            MHFree(pT);

            break;

        }
    }

    return rVal;
}


BOOL
GetModnameFromExportTable(
    PIMAGE_NT_HEADERS       pNtHdr,
    PIMAGE_SECTION_HEADER   pSH,
    LOAD_DLL_DEBUG_INFO64 * pldd,
    LPTSTR                  lpName,
    int                     cbName
    )
/*++

Routine Descriotion:

    This routine attempts to invent an exe name for a DLL
    from the module name found in the export table.  This
    will fail if there is no export table, so it is not
    usually useful for EXEs.

Arguments:

    pNtHdr  - Supplies pointer to NT header in image PE header

    pSH     - Supplies pointer to section header table

    pldd    - Supplies ptr to info record from debug event

    lpName  - Returns name when successful

    cbName  - Supplies size of buffer at lpName

Return Value:

    TRUE if successful and name is copied to lpName, FALSE
    if not successful.

--*/
{
    IMAGE_EXPORT_DIRECTORY      expDir;
    ULONG                       ExportVA;
    ULONG                       oExports;
    int                         iobj;
    int                         cobj;

    /*
     * Find object which has the same RVA as the
     * export table.
     */

    cobj = pNtHdr->FileHeader.NumberOfSections;

    ExportVA = pNtHdr->
                OptionalHeader.
                 DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].
                  VirtualAddress;

    if (!ExportVA) {
        return FALSE;
    }

    for (iobj=0; iobj<cobj; iobj++) {
        if (pSH[iobj].VirtualAddress == ExportVA) {
            oExports = pSH[iobj].PointerToRawData;
            break;
        }
    }

    if (iobj >= cobj) {
        return FALSE;
    }

    if (  (SetReadPointer(oExports, FILE_BEGIN) == -1L)
       || !DoRead(&expDir, sizeof(expDir)) ) {

        return FALSE;
    }

    SetReadPointer(oExports + (ULONG) expDir.Name - ExportVA,
                   FILE_BEGIN);

    _ftcscpy(lpName, _T("#:\\"));

    if (!DoRead(lpName+3, cbName - 3)) {
        // It's a DLL, but we can't get the name...
        _stprintf(lpName+3, _T("DLL%02d.DLL"), ++MagicModuleId);
    }

    return TRUE;
}

typedef DWORD ( WINAPI *LPFNGETNAME ) (
    HANDLE hProcess,
    HMODULE hModule,
    LPSTR lpFilename,
    DWORD nSize
    );

BOOL
GetModNameUsingPsApi(
    HTHDX hthd,
    LOAD_DLL_DEBUG_INFO64 *pldd,
    LPTSTR              lpName,
    int                 cbName
    )
/*++

Routine Description:

    This routine attempts to get the fullpathname for a DLL
    by calling an entry point in psapi.dll.  This
    will fail on Win95

Arguments:

    hthd    - Ptr to the current thread structure.

    pldd    - Supplies ptr to info record from debug event

    lpName  - Returns name when successful

    cbName  - Supplies size of buffer at lpName

Return Value:

    TRUE if successful and name is copied to lpName, FALSE
    if not successful.

--*/
{
    HANDLE hModule = NULL;
    BOOL fRet = FALSE;

    assert( Is64PtrSE(pldd->lpBaseOfDll) );

    if ( IsChicago( ) ) {
        return FALSE;
    }

    if ((hModule = LoadLibrary("psapi.dll")) != NULL) {
        LPFNGETNAME ProcAddr = (LPFNGETNAME)GetProcAddress(hModule, "GetModuleFileNameExA");

        if ( (*ProcAddr) (  hthd->hprc->rwHand,
                            (HMODULE)pldd->lpBaseOfDll, /* Same as hModule */
                            lpName,
                            cbName )
            )
        {
            fRet = TRUE;
        }

        FreeLibrary(hModule);
    }

    return fRet;
}



#ifdef INTERNAL
void
DeferIt(
    HTHDX       hthd,
    DEBUG_EVENT64 *pde
    )
{
    PDLL_DEFER_LIST pddl;
    PDLL_DEFER_LIST *ppddl;

    pddl = MHAlloc(sizeof(DLL_DEFER_LIST));
    pddl->next = NULL;
    pddl->LoadDll = pde->u.LoadDll;
    for (ppddl = &hthd->hprc->pDllDeferList; *ppddl; ) {
         ppddl = & ((*ppddl)->next);
    }
    *ppddl = pddl;
}
#endif  // INTERNAL

/*** FFilesIdentical
 *
 * PURPOSE:
 *      Determine whether a filename and a file handle refer to the same file.
 *
 * INPUT:
 *      szFilename: a filename
 *      hFile: a file handle
 *
 * OUTPUT:
 *      Returns TRUE if these refer to the same file.
 */

BOOL
FFilesIdentical(
    LPTSTR szFileName,
    HANDLE hFile
    )
{
    HANDLE  hFile2;
    BY_HANDLE_FILE_INFORMATION bhfi1, bhfi2;
    BOOL fIdentical = FALSE;

    hFile2 = CreateFile(szFileName, GENERIC_READ,
        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, 0);

    if (hFile2 != INVALID_HANDLE_VALUE)
    {
        if (GetFileInformationByHandle(hFile , &bhfi1) &&
            GetFileInformationByHandle(hFile2, &bhfi2))
        {
            if (bhfi1.dwVolumeSerialNumber == bhfi2.dwVolumeSerialNumber &&
                bhfi1.nFileIndexHigh       == bhfi2.nFileIndexHigh &&
                bhfi1.nFileIndexLow        == bhfi2.nFileIndexLow)
            {
                fIdentical = TRUE;
            }
        }

        VERIFY(CloseHandle(hFile2));
    }

    return fIdentical;
}

/*** FINDMODULE
 *
 * PURPOSE:
 *      Find a module (DLL or EXE) on the path.
 *
 * INPUT:
 *      szModName   Buffer with the name of a module, e.g. "FOO.EXE",
 *                  "BAR.DLL", etc.  May have mixed case.
 *      cchModName  Length of buffer szModName.
 *
 * OUTPUT:
 *      szModName   Replaced with a more specific (but not necessarily
 *                  canonical) path to the module, e.g.
 *                  "C:\NT\SYSTEM\FOO.EXE", "MYDIR\BAR.DLL", if the
 *                  module can be found.  If it can't be found or
 *                  there isn't enough space in the buffer, the buffer
 *                  is left unchanged.
 *
 ********************************************************************/

BOOL
FindModule(
    LPTSTR szModName,
    UINT cchModName
    )
{
    LPTSTR pSlash;
    LPTSTR szFullPath = (LPTSTR)_alloca( cchModName*sizeof(TCHAR) );

    /*
    ** We call SearchPath which is a convenient way to
    ** find a file along the path that Windows uses to load a DLL.
    ** REVIEW: BUG: In order to find the DLL correctly, the call to
    ** SearchPath must be made with the Current Directory set to the
    ** Current Directory of the process that has just loaded the DLL.
    */
    DWORD result = SearchPath( NULL, szModName, NULL, cchModName, szFullPath, &pSlash );
    if ( (result!=0) && (result!=cchModName) )
    {
        _tcscpy( szModName, szFullPath );
        return TRUE;
    }

    return(FALSE);
}

/*** GetNTDebugModuleFileNameLastChance
 *
 * PURPOSE:
 *      Do the best job we can of getting a filename for an EXE or DLL
 *      which has just been loaded.  There is no good way to do this under
 *      NT, so we kludge around a lot.  For EXEs, we saved the name of
 *      the EXE we started before starting it.  For DLLs, we look in the
 *      export table of the DLL for its name, which may or may not be
 *      correct.
 *
 * INPUT:
 *      hProcess    Handle to the process in question (either the
 *                  process which has just started, or the process
 *                  for which a DLL has just been loaded)
 *      hFile       Handle to disk file for the EXE or DLL
 *      lpBaseOfImage   Address in hProcess's address space of the
 *                  beginning of the newly loaded EXE or DLL
 *      cchModName  Size of szModName
 *
 * OUTPUT:
 *      szModName   Buffer where module name (possibly with path)
 *                  is written
 *
 ********************************************************************/

BOOL
FGetNTFileName (
    HTHDX           hthd,
    PDLLLOAD_ITEM   pdll,
    HANDLE          hFile,
    LPTSTR          szModName,
    UINT            cchModName
    )
{
    BOOL fRet = FindModule(szModName, cchModName);

    /* If this isn't the right filename, keep looking ... */
    if (FFilesIdentical(szModName, hFile)) {
        fRet = TRUE;
    } else {
        /* Try to determine the filename from the handle, by whatever
        ** means available
        */
#if 1
        fRet = FALSE;
#else
        if (!FTrojanGetDebugModuleFileName(hthd, pdll, szModName, cchModName))
        {
            /* We shouldn't get here!  We've got to do everything
            ** we possibly can to get the filename, because if we don't
            ** get the filename, we'll have major problems later on.
            ** If we ever hit the assertion, we've just got to come up
            ** with some other ways to try to determine the filename.
            */
            assert(FALSE);
        } else {
            fRet = TRUE;
        }
#endif
    }
    return(fRet);
}


/*** FIXCASE
 *
 * PURPOSE:
 *      Fix the upper/lower case of a filename so that it matches what
 *      is on disk.  If the disk in question supports mixed-case
 *      filenames, we change the name we have to match the case of the
 *      name on disk; otherwise, we set the name to lower case (because
 *      that's prettier than upper case).
 *
 * INPUT:
 *      szFilename  Name of file.
 *
 * OUTPUT:
 *      szFilename  Upper/lower case changed.
 *
 ********************************************************************/

VOID
FixCase(
    LPTSTR szFilename
    )
{

#ifdef WIN32

    TCHAR           rgchDrive[4];   /* "X:\" */
    LPTSTR          szDrive;
    DWORD           dwFlags;
    WIN32_FIND_DATA wfd;
    LPTSTR          pch;
    LPTSTR          pchStart;
    TCHAR           ch;
    HANDLE          hSearch;

    if (szFilename[0] && szFilename[1] == _T(':')) {
        _stprintf(rgchDrive, _T("%c:\\"), szFilename[0]);
        szDrive = rgchDrive;
    }
    else {
        szDrive = NULL;
    }

    if (GetVolumeInformation(szDrive, NULL, 0, NULL, NULL, &dwFlags, NULL, 0)
        &&
        (dwFlags & FS_CASE_IS_PRESERVED)
        ) {

        /*
        ** For each filename component, check what case it has on disk.
        */
        pch = szFilename;

        if (pch[0] && pch[1] == _T(':')) {  /* path has drive letter?   */
            *pch = (TCHAR)_totupper ( *pch ); /* upper case drive letter  */
            pch += 2;
        }

        if (*pch == _T('/') || *pch == _T('\\')) {
            *pch = _T('\\');
            ++pch;
        }

        while (*pch) {

            pchStart = pch;

            while (*pch && *pch != _T('\\') && *pch != _T('/')) {
                pch = _tcsinc( pch );
            }

            ch = *pch;
            *pch = _T('\0');

            /*
            ** Find this filename component
            */
            hSearch = FindFirstFile(szFilename, &wfd);

            /*
            ** If the search failed, or if it returned a name of a
            ** different length than the one we asked for (e.g. we
            ** asked for "FOO." and it gave us "FOO"), we'll give
            ** up and convert the rest of the name to lower case
            */
            if (hSearch == INVALID_HANDLE_VALUE ||
                _ftcslen(pchStart) != _ftcslen(wfd.cFileName)) {
                *pch = ch;
                _ftcslwr ( pchStart );
                return;
            }

            /*
            ** Copy the correct case into our filename
            */
            _tcscpy ( pchStart, wfd.cFileName );

            /*
            ** Close the search
            */
            assert ( !FindNextFile(hSearch, &wfd) );
            VERIFY(FindClose ( hSearch ));

            /*
            ** Restore the slash or NULL
            */
            *pch = ch;

            /*
            ** If we're on a separator, move to next filename component
            */
            if (*pch) {
                *pch = _T('\\');
                pch++;
            }
        }

        return;
    }

#endif

    /*
    ** Convert to lower case, with backslashes
    */

    _ftcslwr(szFilename);

    for (; *szFilename; szFilename = _tcsinc( szFilename) ) {
        if (*szFilename == _T('/')) {
            *szFilename = _T('\\');
        }
    }
}

void
FixFilename(
    TCHAR *szTempFilename,
    const TCHAR *szInFilename
    )
/*++

/*** FIXFILENAME
 *
 * PURPOSE:
 *      Fix the upper/lower case of a filename and so that it matches what
 *      is on disk.  Also if we have a 8.3 name for a long filename
 *      convert the whole path to its long filename equivalen.
 *      If the disk in question supports mixed-case
 *      filenames, we change the name we have to match the case of the
 *      name on disk; otherwise, we set the name to lower case (because
 *      that's prettier than upper case).
 *
 * INPUT:
 *      szInFilename  Name of file. .
 *
 * OUTPUT:
 *      szTempFilename  Upper/lower case changed, long filename variant.
 *                      Assumes size is big enough to hold long
 *                  filename
 *
 * Note: Win95 has an API to do this (called via an int 31h or somesuch),
 * but NT has no such API. Why? To make everyone go through this pain I guess.
 *
 ********************************************************************/

{
    CHAR            rgchDrive[4];   /* "X:\" */
    CHAR *          szDrive;
    DWORD           dwFlags;
    WIN32_FIND_DATA wfd;
    CHAR *          pch;
    CHAR *          pchTemp;
    CHAR *          pchStart;
    CHAR            ch;
    HANDLE          hSearch;
    CHAR            szFilename[512];

    _tcscpy( szFilename, szInFilename );                // make local copy

    if (szFilename[0] && szFilename[1] == ':') {
        sprintf(rgchDrive, "%c:\\", szFilename[0]);
        szDrive = rgchDrive;
    }
    else {
        szDrive = NULL;
    }

    if (!GetVolumeInformation(szDrive, NULL, 0, NULL, NULL, &dwFlags, NULL, 0)
        ||
        !(dwFlags & FS_CASE_IS_PRESERVED)
        ) {

        _tcscpy(szTempFilename, szFilename );           // just copy it if not case sensitive

    } else {


        /*
        ** For each filename component, check what case it has on disk.
        */
        pch = szFilename;
        pchTemp = szTempFilename;

        if (pch[0] && pch[1] == ':') {  /* path has drive letter?   */
            _tcsncpy(pchTemp, pch, 2);

            /* upper case drive letter  */
            *pchTemp = (char) _totupper ( (*pchTemp) ); /* upper case drive letter  */
            pch += 2;
            pchTemp += 2;
        }

        if (*pch == '/' || *pch == '\\') {
            *pch = *pchTemp = '\\';
            ++pchTemp;
            ++pch;
        }

        while (*pch) {
            size_t iLen;

            pchStart = pch;

            while (*pch && *pch != '\\' && *pch != '/') {
                pch = _tcsinc( pch );
            }

            ch = *pch;
            *pch = '\0';

            /*
            ** Find this filename component
            */
            hSearch = FindFirstFile(szFilename, &wfd);

            /*
            ** If the search failed, we'll give
            ** up and convert the rest of the name to lower case
            */
            if (hSearch == INVALID_HANDLE_VALUE ) {
                *pch = ch;
                CharLower ( pchStart );
                // Copy over the rest of the filename to the temporary buffer.
                // this will now have the best we can do about converting
                // this filename.
                _tcscpy(pchTemp, pchStart);

                _tcscpy(szFilename, szTempFilename);

                return;
            }

            /*
            ** Copy the correct name into the temp filename,
            */
            iLen = _tcslen(wfd.cFileName);
            _tcsncpy ( pchTemp, wfd.cFileName, iLen );
            pchTemp += iLen;

            /*
            ** Close the search
            */
            assert ( !FindNextFile(hSearch, &wfd) );
            FindClose ( hSearch );

            /*
            ** Restore the slash or NULL
            */
            *pch = ch;

            /*
            ** If we're on a separator, move to next filename component
            */
            if (*pch) {
                *pchTemp = *pch = '\\';
                pch++; pchTemp++;
            }
        }

        *pchTemp = '\0';
    }
}


BOOL
LoadDll(
    DEBUG_EVENT64 * de,
    HTHDX           hthd,
    LPWORD          lpcbPacket,
    LPBYTE *        lplpbPacket,
    BOOL            fThreadIsStopped
    )
/*++

Routine Description:

    This routine is used to load the signification information about
    a PE exe file.  This information consists of the name of the exe
    just loaded (hopefully this will be provided later by the OS) and
    a description of the sections in the exe file.

Arguments:

    de         - Supplies a pointer to the current debug event

    hthd       - Supplies a pointer to the current thread structure

    lpcbPacket - Returns the count of bytes in the created packet

    lplpbPacket - Returns the pointer to the created packet

Return Value:

    True on success and FALSE on failure

--*/

{
    LOAD_DLL_DEBUG_INFO64      *ldd = &de->u.LoadDll;
    LPMODULELOAD                lpmdl;
    TCHAR                       szModName[512];
    TCHAR                       szAnsiName[512];
    DWORD                       cbObject;
    ULONG64                     offset;
    DWORD                       lenSz, lenTable;
    DWORD                       cobj, iobj;
    DWORD                       isecTLS;
    IMAGE_DOS_HEADER            dosHdr;
    IMAGE_NT_HEADERS            ntHdr;
    IMAGE_SECTION_HEADER *      rgSecHdr = NULL;
    HANDLE                      hFile;
    int                         iDll;
    HPRCX                       hprc = hthd->hprc;
    DWORD                       cb;
    TCHAR                       rgch[512];
    LPVOID                      lpv;
    LPTSTR                      lpsz;
    ADDR                        addr;
    BOOL                        fTlsPresent;
    OFFSET                      off;
    TCHAR                       fname[_MAX_FNAME];
    TCHAR                       ext[_MAX_EXT];


    if ( hprc->pstate & (ps_killed | ps_dead) ) {
        //
        //  Process is dead, don't bother doing anything.
        //
        return FALSE;
    }

    assert( Is64PtrSE(ldd->lpBaseOfDll) );

    //
    //  Remember this dll in the process dll list.
    //

    //
    // run through the list and see if there is already an entry for this address
    //

    assert( Is64PtrSE(ldd->lpBaseOfDll) );

    for (iDll=0; iDll<hprc->cDllList; iDll+=1) {
        if (hprc->rgDllList[iDll].fValidDll) {

            assert( Is64PtrSE(hprc->rgDllList[iDll].offBaseOfImage) );

            if (hprc->rgDllList[iDll].offBaseOfImage == ldd->lpBaseOfDll) {
                break;
            }
        }
    }

    if (iDll == hprc->cDllList) {

        //
        // didn't find it; find an empty record
        //

        for (iDll=0; iDll<hprc->cDllList; iDll+=1) {
            if (!hprc->rgDllList[iDll].fValidDll) {
                break;
            }
        }
    }

    if (iDll == hprc->cDllList) {

        //
        // the dll list needs to be expanded
        //

        hprc->cDllList += 10;
        if (!hprc->rgDllList) {
            hprc->rgDllList = (PDLLLOAD_ITEM) MHAlloc(sizeof(DLLLOAD_ITEM) * 10);
            memset(hprc->rgDllList, 0, sizeof(DLLLOAD_ITEM)*10);
        } else {
            hprc->rgDllList = MHRealloc(hprc->rgDllList,
                                  hprc->cDllList * sizeof(DLLLOAD_ITEM));
            memset(&hprc->rgDllList[hprc->cDllList-10], 0, 10*sizeof(DLLLOAD_ITEM));
        }

    } else if (hprc->rgDllList[iDll].offBaseOfImage != ldd->lpBaseOfDll) {

        //
        // if this is an empty entry, make sure it is cleared.
        //

        memset(&hprc->rgDllList[iDll], 0, sizeof(DLLLOAD_ITEM));

    }

    //
    // stick the demarcator at the start of the string so that we
    // don't have to move the string over later.
    //

    *szModName = CMODULEDEMARCATOR;

    //
    //   Process the DOS header.  It is currently regarded as mandatory
    //
    //   This has to be read before attempting to resolve the
    //   name of a module on NT.  If the name resolution is aborted,
    //   the memory allocated for the IMAGE_SECTION_HEADER must be
    //   freed.
    //
    if (ldd->hFile == 0) {
        assert( Is64PtrSE(ldd->lpBaseOfDll) );
        SetPointerToMemory(hprc, ldd->lpBaseOfDll);
    } else {
        SetPointerToFile(ldd->hFile);
    }

    SetReadPointer(0, FILE_BEGIN);

    if (DoRead(&dosHdr, sizeof(dosHdr)) == FALSE) {
        DPRINT(1, (_T("ReadFile got error %u\r\n"), GetLastError()));
        return FALSE;
    }

    //
    //  Read in the PE header record
    //

    if ((dosHdr.e_magic != IMAGE_DOS_SIGNATURE) ||
        (SetReadPointer(dosHdr.e_lfanew, FILE_BEGIN) == -1L)) {
        return FALSE;
    }

    if (!DoRead(&ntHdr, sizeof(ntHdr))) {
        return FALSE;
    }

    //
    //      test whether we have a TLS directory
    //
    fTlsPresent = !!ntHdr.OptionalHeader.DataDirectory[ IMAGE_DIRECTORY_ENTRY_TLS ].Size;

    if (sizeof(ntHdr.OptionalHeader) != ntHdr.FileHeader.SizeOfOptionalHeader) {
        SetReadPointer(ntHdr.FileHeader.SizeOfOptionalHeader -
                       sizeof(ntHdr.OptionalHeader), FILE_CURRENT);
    }

    //
    //   Save off the count of objects in the dll/exe file
    //
    cobj = ntHdr.FileHeader.NumberOfSections;

    //
    //   Save away the offset in the file where the object table
    //   starts.  We will need this later to get information about
    //   each of the objects.
    //
    rgSecHdr = (IMAGE_SECTION_HEADER *) MHAlloc( cobj * sizeof(IMAGE_SECTION_HEADER));
    if (!DoRead( rgSecHdr, cobj * sizeof(IMAGE_SECTION_HEADER))) {
        assert (FALSE );
        MHFree(rgSecHdr);
        return FALSE;
    }

    //
    // if the dll record is marked valid, it already matches the dll
    //

    if (hprc->rgDllList[iDll].fValidDll) {

        //
        // in this case we are re-doing a mod load for a dll
        // that is part of a process that is being reconnected
        //

        assert( hprc->rgDllList[iDll].szDllName != NULL );
        _tcscpy( szModName + 1, hprc->rgDllList[iDll].szDllName );

    } else {

        if (CrashDump) {
            CopyFileNameFromDBGFileName( szModName+1, (PVOID)ldd->lpImageName );
            if ((hprc->rgDllList[iDll].szDllName = MHAlloc(_tcslen(szModName+1) + 1)) != NULL) {
                _tcscpy(hprc->rgDllList[iDll].szDllName, szModName+1);
            } else {
                return FALSE;
            }

            //
            // Normal case: image name is in the debug event
            //
        } else if (((PVOID)ldd->lpImageName != NULL)
            && DbgReadMemory(hprc,
                             ldd->lpImageName,
                             &lpv,
                             sizeof(lpv),
                             (int *) &cb)
            && (cb == sizeof(lpv))
            && (lpv != NULL)
            && DbgReadMemory(hprc,
                             SEPtrTo64(lpv),
                             rgch,
                             sizeof(rgch),
                             (int *) &cb))
        {
            // we're happy...
#if !defined(_UNICODE)
            if (!ldd->fUnicode) {
                FixFilename(szModName+1, rgch );
            } else {
                WideCharToMultiByte(CP_ACP,
                                    0,
                                    (LPWSTR)rgch,
                                    -1,
                                    szAnsiName,
                                    _tsizeof(szAnsiName),
                                    NULL,
                                    NULL);
                FixFilename(szModName + 1, szAnsiName );
            }
            if ((hprc->rgDllList[iDll].szDllName = MHAlloc(_tcslen(szModName+1) + 1)) != NULL)
            {
                _tcscpy(hprc->rgDllList[iDll].szDllName, szModName+1);
            } else {
                return FALSE;
            }
#else
            if (ldd->fUnicode) {
                FixFilename(szModName+1, rgch );
            } else {
                MultiByteToWideChar(CP_ACP,
                                    0,
                                    (LPTSTR)rgch,
                                    -1,
                                    szAnsiName,
                                    _tsizeof(szAnsiName));
                FixFilename(szModName + 1, szAnsiName );
            }
            if ((hprc->rgDllList[iDll].szDllName = MHAlloc((_tcslen(szModName+1) + 1)) * sizeof(TCHAR)) != NULL)
            {
                _tcscpy(hprc->rgDllList[iDll].szDllName, szModName+1);
            } else {
                return FALSE;
            }
#endif

        }

            //
            // If *nameBuffer != 0 then we know we are really
            // dealing with the root exe and we can steal the
            // name from there.
            //

        else if (*nameBuffer && !FLoading16) {

            if (FDMRemote) {
                _tsplitpath( nameBuffer, NULL, NULL, fname, ext );
                sprintf( szModName+1, _T("#:\\%s%s"), fname, ext );
            } else {
                _tcscpy(szModName + 1, nameBuffer);     // do NOT FixFilename this one
            }

            if ((hprc->rgDllList[iDll].szDllName = MHAlloc(_tcslen(szModName+1) + 1)) != NULL) {
                _tcscpy(hprc->rgDllList[iDll].szDllName, szModName+1);
            } else {
                return FALSE;
            }


            //
            // Try to use PSAPI stuff to get the image name
            //

        } else if (GetModNameUsingPsApi(hthd, ldd, rgch, _tsizeof(rgch))) {
            // cool...
            FixFilename(szModName + 1, rgch);
            if ((hprc->rgDllList[iDll].szDllName = MHAlloc(_tcslen(szModName+1) + 1)) != NULL) {
                _tcscpy(hprc->rgDllList[iDll].szDllName, szModName+1);
            } else {
                return FALSE;
            }

            //
            // Look for a debug misc resord in the image
            //
        } else if (GetModnameFromImage(&ntHdr, rgSecHdr, ldd, rgch, _tsizeof(rgch))) {

            // joyful...
            lpsz = _ftcsrchr(rgch, _T('\\'));
            if (!lpsz) {
                lpsz = _ftcsrchr(rgch, _T(':'));
            }
            if (lpsz) {
                lpsz = _ftcsinc(lpsz);
            } else {
                lpsz = rgch;
            }

#if defined(DOLPHIN)
            if (FGetNTFileName(hthd, &hprc->rgDllList[iDll], ldd->hFile, rgch, _tsizeof(rgch))) {
                _ftcscpy(szModName + 1, rgch);
            }
            else
#endif
            {
                _ftcscpy(szModName + 1, _T("#:\\"));
                _ftcscpy(szModName + 4, lpsz);
            }

            if ((hprc->rgDllList[iDll].szDllName = MHAlloc(_tcslen(lpsz) + 1)) != NULL)
            {
                _tcscpy(hprc->rgDllList[iDll].szDllName, lpsz);
            }
            else
            {
                return FALSE;
            }


            //
            // If it is a dll, it probably exports something, so it has a module name.
            //

        } else if (GetModnameFromExportTable(&ntHdr, rgSecHdr, ldd, rgch, _tsizeof(rgch))) {

            // serene...
            _ftcscpy(szModName + 1, rgch);
            if ((hprc->rgDllList[iDll].szDllName = MHAlloc(_tcslen(rgch) + 1)) != NULL)
            {
                _tcscpy(hprc->rgDllList[iDll].szDllName, rgch);
            }
            else
            {
                return FALSE;
            }


            //
            // if it isn't available, make something up.
            //

        } else {

            // hopeless...
#if defined(DOLPHIN)
            if (!LoadString(hInstance, IDS_UnknownExe, rgch, _tsizeof(rgch))) {
                assert(FALSE);
            }
            if (FGetNTFileName(hthd, &hprc->rgDllList[iDll], ldd->hFile, rgch, _tsizeof(rgch))) {
                _ftcscpy(szModName + 1, rgch);
            } else
#endif
            sprintf(szModName+1, _T("#:\\APP%02d.EXE"), ++MagicModuleId);
            if ((hprc->rgDllList[iDll].szDllName = MHAlloc(_tcslen(szModName + 1) + 1)) != NULL)
            {
                _tcscpy(hprc->rgDllList[iDll].szDllName, szModName + 1);
            }
            else
            {
                return FALSE;
            }
        }

        if (!FLoading16) {
            *nameBuffer = 0;
        }
    }

    //
    // for remote case, kill the drive letter to
    // prevent finding same exe on wrong platform,
    // except when user gave path to exe.
    //
    if (fUseRealName) {
        fUseRealName = FALSE;
    }
    lenSz=_ftcslen(szModName);
    DPRINT(10, (_T("*** LoadDll %s  base=%I64x\n"), szModName, ldd->lpBaseOfDll));

    szModName[lenSz] = 0;

    lpsz = _ftcsrchr(szModName, _T('\\'));
    if (!lpsz) {
        lpsz = _ftcsrchr(szModName, _T(':'));
    }
    if (lpsz) {
        lpsz = _ftcsinc(lpsz);
    } else {
        lpsz = szModName;
    }

    if (_ftcsicmp(lpsz, _T("kernel32.dll")) == 0) {
        hprc->dwKernel32Base = (UINT_PTR)ldd->lpBaseOfDll;
    }

    assert( Is64PtrSE(hprc->rgDllList[iDll].offBaseOfImage) );
    assert( Is64PtrSE(ldd->lpBaseOfDll) );

    if (hprc->rgDllList[iDll].offBaseOfImage != ldd->lpBaseOfDll) {
        //
        // new dll to add to the list
        //
        hprc->rgDllList[iDll].fValidDll = TRUE;
        hprc->rgDllList[iDll].offBaseOfImage = (OFFSET) ldd->lpBaseOfDll;
        hprc->rgDllList[iDll].cbImage = ntHdr.OptionalHeader.SizeOfImage;
    }

    //
    //  Find address of OLE RPC tracing export (if any)
    //
    // If hFile is NULL, we cannot do any RPC debugging. This may happen
    // in certain platforms. So, check for this.
    //
    if (ldd->hFile != NULL) {
        FGetExport(&hprc->rgDllList[iDll],
                   (HFILE)(DWORD_PTR)ldd->hFile,
                   _T("DllDebugObjectRPCHook"),
                   &hprc->rgDllList[iDll].lpvOleRpc);
    } else {
        hprc->rgDllList[iDll].lpvOleRpc = NULL;
    }

    DMSqlLoadDll( hprc, ldd, iDll );

    szModName[lenSz] = CMODULEDEMARCATOR;

    if (/* FDMRemote */ TRUE ) {
        if (ldd->hFile != 0 && ldd->hFile != (HANDLE)-1) {
            CloseHandle(ldd->hFile);  //  don't need this anymore
        }
        hFile = (HANDLE)-1; // remote: can't send file handle across wire
    } else {

        if (ldd->hFile == 0) {
            hFile = (HANDLE)-1;
        } else {
            hFile = ldd->hFile; // local: let SH use our handle
        }
    }

    /*
     *  Make up a record to send back from the name.
     *  Additionally send back:
     *          The file handle (if local)
     *          The load base of the dll
     *          The time and date stamp of the exe
     *          The checksum of the file
     */
    assert( Is64PtrSE(ldd->lpBaseOfDll) );
    sprintf( szModName+lenSz+1, _T("0x%08lX%c0x%08lX%c0x%016I64X%c0x%016I64X%c%08lX%c"),
            ntHdr.FileHeader.TimeDateStamp,     CMODULEDEMARCATOR,
            ntHdr.OptionalHeader.CheckSum,      CMODULEDEMARCATOR,
            SEPtrTo64(hFile),                   CMODULEDEMARCATOR,
            ldd->lpBaseOfDll,                   CMODULEDEMARCATOR,
            ntHdr.OptionalHeader.SizeOfImage,   CMODULEDEMARCATOR
             );
    lenSz = _ftcslen(szModName);
    /*
     * Allocate the packet which will be sent across to the EM.
     * The packet will consist of:
     *     The MODULELOAD structure             sizeof(MODULELOAD) +
     *     The section description array        cobj*sizeof(OBJD) +
     *     The name of the DLL                  lenSz+1
     */

    lenTable = (cobj * sizeof(OBJD));
    *lpcbPacket = (WORD)(sizeof(MODULELOAD) + lenTable + (lenSz+1) * sizeof(TCHAR));
    *lplpbPacket= (LPBYTE)(lpmdl=(LPMODULELOAD)MHAlloc(*lpcbPacket));
    lpmdl->lpBaseOfDll = ldd->lpBaseOfDll;
    lpmdl->cobj = cobj;
    lpmdl->mte = (WORD) -1;

#ifdef TARGET_i386
    lpmdl->CSSel    = (unsigned short)hthd->context.SegCs;
    lpmdl->DSSel    = (unsigned short)hthd->context.SegDs;
#else
    lpmdl->CSSel = lpmdl->DSSel = 0;
#endif // i386

    /*
     *  Set up the descriptors for each of the section headers
     *  so that the EM can map between section numbers and flat addresses.
     */

    lpmdl->uoffDataBase = 0;
    for (iobj=0; iobj<cobj; iobj++) {
        OLESEG oleseg;

        offset = rgSecHdr[iobj].VirtualAddress + ldd->lpBaseOfDll;
        cbObject = rgSecHdr[iobj].Misc.VirtualSize;
        if (cbObject == 0) {
            cbObject = rgSecHdr[iobj].SizeOfRawData;
        }

        lpmdl->rgobjd[iobj].offset = (UOFFSET)offset;
        lpmdl->rgobjd[iobj].cb = cbObject;
        lpmdl->rgobjd[iobj].wPad = 1;

#if defined(TARGET_i386)
        if (IMAGE_SCN_CNT_CODE & rgSecHdr[iobj].Characteristics) {
            lpmdl->rgobjd[iobj].wSel = (WORD) hthd->context.SegCs;
        } else {
            lpmdl->rgobjd[iobj].wSel = (WORD) hthd->context.SegDs;
        }
#else
        lpmdl->rgobjd[iobj].wSel = 0;
#endif  // TARGET_i386

        if (!_fmemcmp( rgSecHdr[iobj].Name, ".data\0\0", IMAGE_SIZEOF_SHORT_NAME)) {
            if (lpmdl->uoffDataBase == 0) {
                lpmdl->uoffDataBase = offset;
            }
        }
        /*
         * If the section is one of the special OLE segments, we
         * keep track of the address ranges in the OLERG structure.
         */
        if ((oleseg = GetOleSegType(rgSecHdr[iobj].Name)) != olenone) {
            OLERG FAR*  lpolerg;
            DWORD         i;

            hprc->rgDllList[iDll].fContainsOle = TRUE;

            /* expand (or create) our buffer */
            ++hprc->colerg;
            if (hprc->rgolerg) {
                hprc->rgolerg = MHRealloc(hprc->rgolerg,
                                        hprc->colerg * sizeof(OLERG));
            }
            else {
                hprc->rgolerg = MHAlloc(hprc->colerg * sizeof(OLERG));
            }

            /* find place to insert new OLE range */
            for (i = 0; i < hprc->colerg - 1; ++i) {
                if (offset < hprc->rgolerg[i].uoffMin) {
                    break;
                }
            }

            /* insert an OLERG */
            memmove(&hprc->rgolerg[i+1],
                    &hprc->rgolerg[i],
                    sizeof(OLERG) * (hprc->colerg - i - 1));

            /* insert new OLE range */
            lpolerg = &hprc->rgolerg[i];
            lpolerg->uoffMin   = offset;
            lpolerg->uoffMax   = offset + cbObject;
            lpolerg->segType   = oleseg;
        }
        if ( fTlsPresent && !_fmemcmp ( rgSecHdr[iobj].Name, ".tls\0\0\0", IMAGE_SIZEOF_SHORT_NAME ) ) {
            isecTLS = iobj + 1;
        }
    }

    lpmdl->fRealMode = FALSE;
    lpmdl->fFlatMode = TRUE;
    lpmdl->fOffset32 = TRUE;
    lpmdl->dwSizeOfDll = ntHdr.OptionalHeader.SizeOfImage;
    lpmdl->uoffiTls = 0;
    lpmdl->isecTLS = 0;
    lpmdl->iTls = 0;       // cache it later on demand if uoffiTls != 0
    lpmdl->fThreadIsStopped = fThreadIsStopped;

    /*
     *  Copy the name of the dll to the end of the packet.
     */

    _fmemcpy(((BYTE*)&lpmdl->rgobjd)+lenTable, szModName, lenSz+1);

    /*
     *  Locate the TLS section if one exists.  If so then get the
     *      pointer to the TLS index
     *
     *  Structure at the address is:
     *
     *          VA      lpRawData
     *          ULONG   cbRawData
     *          VA      lpIndex
     *          VA      lpCallBacks
     */

     if (ntHdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress != 0) {
         if ((DbgReadMemory(hprc,
                            ntHdr.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress +
                                ldd->lpBaseOfDll + 8,
                            &off,
                            sizeof(OFFSET),
                            &cb) == 0) ||
             (cb != sizeof(OFFSET))) {
             assert(FALSE);
         }

         hprc->rgDllList[iDll].offTlsIndex = off;
         lpmdl->uoffiTls = off;
         lpmdl->isecTLS = isecTLS;
     }

    /*
     * free up the memory used for holding the section headers
     */

    MHFree(rgSecHdr);

    if (fDisconnected) {

        //
        // this will prevent the dm from sending a message up to
        // the shell.  the dm's data structures are setup just fine
        // so that when the debugger re-connects we can deliver the
        // mod loads correctly.
        //

        return FALSE;

    }

    return TRUE;
}                               /* LoadDll() */


VOID
UnloadAllModules(
    HPRCX           hprc,
    HTHDX           hthd,
    BOOL            AlwaysNotify,
    BOOL            ReallyDestroy
    )
{
    DEBUG_EVENT64   de;
    DWORD           i;

    for (i=0; i<(DWORD)hprc->cDllList; i++) {
        if (hprc->rgDllList[i].fValidDll) { 
            
            assert( Is64PtrSE(hprc->rgDllList[i].offBaseOfImage) );

            if (!hprc->rgDllList[i].fWow) {
                de.dwDebugEventCode = UNLOAD_DLL_DEBUG_EVENT;
                de.dwProcessId = hprc->pid;
                de.dwThreadId = hthd ? hthd->tid : 0;
                de.u.UnloadDll.lpBaseOfDll = hprc->rgDllList[i].offBaseOfImage;
                if (AlwaysNotify || !IsChicago()) {
                    NotifyEM( &de, hthd, 0, 0 );
                }
                if (ReallyDestroy) {
                    DestroyDllLoadItem( &hprc->rgDllList[i] );
                }
            }
        }
    }

    return;
}

VOID
ReloadUsermodeModules(
    HTHDX hthd,
    PTCHAR String
    )
{
    int i;
    DEBUG_EVENT64 de;
    HPRCX hprc = hthd->hprc;
    LPRTP rtp;
    TCHAR szTmpDllName[_MAX_PATH * 2];

    //
    // All or nothing...  Ignore the String arg for now.
    //


    //
    // First unload everything:
    //

    UnloadAllModules(hthd->hprc, hthd, TRUE, FALSE);

    //
    // reload it all
    //

    for (i=0; i < hprc->cDllList; i++) {
        if (hprc->rgDllList[i].fValidDll) {
            LPBYTE lpbPacket;
            WORD   cbPacket;

            assert( Is64PtrSE(hprc->rgDllList[i].offBaseOfImage) );

            de.dwDebugEventCode        = LOAD_DLL_DEBUG_EVENT;
            de.dwProcessId             = hprc->pid;
            de.dwThreadId              = hthd->tid;

            //
            // We have closed the file already...
            //
            de.u.LoadDll.hFile         = NULL;

            de.u.LoadDll.lpBaseOfDll   = hprc->rgDllList[i].offBaseOfImage;
            de.u.LoadDll.fUnicode      = FALSE;


            //
            // Copy the name to a tmp before we free it in the
            // 'DestroyDllLoadItem' function.
            //
            if (!hprc->rgDllList[i].szDllName) {
                de.u.LoadDll.lpImageName = 0;
            } else {
                _tcsncpy(szTmpDllName,
                         hprc->rgDllList[i].szDllName,
                         sizeof(szTmpDllName)/sizeof(TCHAR));
                szTmpDllName[sizeof(szTmpDllName)/sizeof(TCHAR) -1] = 0;
                de.u.LoadDll.lpImageName = (UINT_PTR)szTmpDllName;
            }

            //
            // We destroy the entry to make room. By doing it here, we make sure
            // we can always load the max amount instead of the max amount-1.
            //
            DestroyDllLoadItem( &hprc->rgDllList[i] );

            if (LoadDll(&de, hthd, &cbPacket, &lpbPacket, FALSE) || (cbPacket ==0)) {
                NotifyEM(&de, hthd, cbPacket, (UINT_PTR)lpbPacket);
            }
        }
    }

    //
    // tell the shell that the !reload is finished
    //
    rtp = (LPRTP)MHAlloc(FIELD_OFFSET(RTP, rgbVar)+sizeof(DWORD));
    rtp->hpid = hthd->hprc->hpid;
    rtp->htid = hthd->htid;
    rtp->dbc = dbcServiceDone;
    rtp->cb = sizeof(DWORD);
    *(LPDWORD)rtp->rgbVar = 1;
    DmTlFunc( tlfDebugPacket, rtp->hpid, FIELD_OFFSET(RTP, rgbVar)+rtp->cb, (LONG_PTR)rtp );
    MHFree( rtp );
}


BOOL
DMWaitForDebugEvent(
    LPDEBUG_EVENT64 de64,
    DWORD timeout
    )
{
#if defined (_WIN64)
    return WaitForDebugEvent((LPDEBUG_EVENT)de64, timeout);
#else
    DEBUG_EVENT de;
    BOOL rval;

    rval = WaitForDebugEvent(&de, timeout);

    if (rval) {
        DebugEvent32To64((LPDEBUG_EVENT32)&de, de64);
    }

    return rval;
#endif
}
