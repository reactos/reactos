/****************************** Module Header ******************************\
* Module Name: handtabl.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Implements the USER handle table.
*
* 01-13-92 ScottLu      Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#pragma alloc_text(INIT, HMInitHandleTable)

#if DBG

#define HTIENTRY(szObjectType, structName, fnDestroy, dwAllocTag, bObjectCreateFlags) \
    {szObjectType, sizeof(structName), (FnDestroyUserObject)fnDestroy, (CONST DWORD)dwAllocTag, (CONST BYTE)(bObjectCreateFlags)}

#else // DBG

#define HTIENTRY(szObjectType, structName, fnDestroy, dwAllocTag, bObjectCreateFlags) \
    {(FnDestroyUserObject)fnDestroy, (CONST DWORD)dwAllocTag, (CONST BYTE)(bObjectCreateFlags)}

#endif // DBG

void HMNullFnDestroy(PVOID pobj)
{
    RIPMSG1(RIP_WARNING, "HM: No clean up function for %#p", pobj);
    HMDestroyObject(pobj);
    return;
}
/***************************************************************************\
*
* Table of user objects statistics.  Used by userkdx.dumhmgr debugger extension
*
\***************************************************************************/

#if DBG

/*
 * Note that we need to statically initialize gaPrevhti so that it is
 * included during the link phase. It is not refered to anywher in win32k.sys,
 * only in userkdx.dll, so it will be optimized out in the link phase otherwise.
 * Marking it volatile did not seem to help.
 */

PERFHANDLEINFO gaPerfhti[TYPE_CTYPES] = {0};  /* stores current counts */
PERFHANDLEINFO gaPrevhti[TYPE_CTYPES] = {0};  /* stores previous counts */

#endif // DBG

/***************************************************************************\
*
* Table of handle type information.
*
* Desktop and Shared Heap objects can't be tagged as yet
* (TAG_WINDOW is bogus for heap windows, but not for desktop and other
* windows allocated in pool).
*
* WARNING: Keep it in sync with aszTypeNames table from ntuser\kdexts\userexts.c
*
* All HM objects must start with a HEAD strucutre. In addition:
* (If you find these comments to be wrong, please fix them)
*
* OCF_PROCESSOWNED: Object must start with a PROC*HEAD structure
*                   A ptiOwner must be provided
*                   The object affects the handle quota (ppi->UserHandleCount)
*                   The object will be destroyed if the process goes away.
*
* OCF_MARKPROCESS:  Object must start with a PROCMARKHEAD structure
*                   A ptiOwner must be provided
*                   It must not use OCF_DESKTOPHEAP (implementation limitation)
*
* OCF_THREADOWNED:  Object must start with a THR*HEAD structure
*                   The object affects the handle quota (ppi->UserHandleCount)
*                   The object will be destroyed if the thread goes away.
*
* OCF_DESKTOPHEAP:  Object must start with a *DESKHEAD structure
*                   A pdeskSrc must be provided at allocation time
*                   It must not use OCF_MARKPROCESS (implementation limitation)
*
\***************************************************************************/

#if (TYPE_FREE != 0)
#error TYPE_FREE must be zero.
#endif

CONST HANDLETYPEINFO    gahti[TYPE_CTYPES] = {
    /* TYPE_FREE - HEAD */
    HTIENTRY("Free", HEAD,
             NULL,
             0,
             0),

    /* TYPE_WINDOW - WND(THRDESKHEAD) */
    HTIENTRY("Window", WND,
             xxxDestroyWindow,
             TAG_WINDOW,
             OCF_THREADOWNED | OCF_USEPOOLQUOTA | OCF_DESKTOPHEAP | OCF_USEPOOLIFNODESKTOP | OCF_VARIABLESIZE),

    /* TYPE_MENU - MENU(PROCDESKHEAD) */
    HTIENTRY("Menu", MENU,
             _DestroyMenu,
             0,
             OCF_PROCESSOWNED | OCF_DESKTOPHEAP),

    /* TYPE_CURSOR - CURSOR(PROCMARKHEAD) or ACON(PROCMARKHEAD) */
    HTIENTRY("Icon/Cursor", CURSOR,
             DestroyUnlockedCursor,
             TAG_CURSOR,
             OCF_PROCESSOWNED | OCF_MARKPROCESS | OCF_USEPOOLQUOTA),

    /* TYPE_SETWINDOWPOS - SMWP(HEAD) */
    HTIENTRY("WPI(SWP) structure", SMWP,
             DestroySMWP,
             TAG_SWP,
             OCF_THREADOWNED | OCF_USEPOOLQUOTA),

    /* TYPE_HOOK - HOOK(THRDESKHEAD) */
    HTIENTRY("Hook", HOOK,
             FreeHook,
             0,
             OCF_THREADOWNED | OCF_DESKTOPHEAP),

    /* TYPE_CLIPDATA -  CLIPDATA(HEAD) */
    HTIENTRY("Clipboard Data", CLIPDATA,
             HMNullFnDestroy,
             TAG_CLIPBOARD,
             OCF_VARIABLESIZE),

    /* TYPE_CALLPROC - CALLPROCDATA(THRDESKHEAD) */
    HTIENTRY("CallProcData", CALLPROCDATA,
             HMDestroyObject,
             0,
             OCF_PROCESSOWNED | OCF_DESKTOPHEAP),

    /* TYPE_ACCELTABLE - ACCELTABLE(PROCOBJHEAD) */
    HTIENTRY("Accelerator", ACCELTABLE,
             HMDestroyObject,
             TAG_ACCEL,
             OCF_PROCESSOWNED | OCF_USEPOOLQUOTA | OCF_VARIABLESIZE),

    /* TYPE_DDEACCESS - SVR_INSTANCE_INFO(THROBJHEAD) */
    HTIENTRY("DDE access", SVR_INSTANCE_INFO,
             HMNullFnDestroy,
             TAG_DDE9,
             OCF_THREADOWNED | OCF_USEPOOLQUOTA),

    /* TYPE_DDECONV - DDECONV(THROBJHEAD) */
    HTIENTRY("DDE conv", DDECONV,
             FreeDdeConv,
             TAG_DDEa,
             OCF_THREADOWNED | OCF_USEPOOLQUOTA),

    /* TYPE_DDEXACT - XSTATE(THROBJHEAD) */
    HTIENTRY("DDE Transaction", XSTATE,
             FreeDdeXact,
             TAG_DDEb,
             OCF_THREADOWNED | OCF_USEPOOLQUOTA),

    /* TYPE_MONITOR - MONITOR(HEAD) */
    HTIENTRY("Monitor", MONITOR,
             DestroyMonitor,
             TAG_DISPLAYINFO,
             OCF_SHAREDHEAP),

    /* TYPE_KBDLAYOUT - KL(HEAD) */
    HTIENTRY("Keyboard Layout",  KL,
             DestroyKL,
             TAG_KBDLAYOUT,
             0),

    /* TYPE_KBDFILE - KBDFILE(HEAD) */
    HTIENTRY("Keyboard File", KBDFILE,
             DestroyKF,
             TAG_KBDFILE,
             0),

    /* TYPE_WINEVENTHOOK - EVENTHOOK(THROBJHEAD) */
    HTIENTRY("WinEvent hook", EVENTHOOK,
             DestroyEventHook,
             TAG_WINEVENT,
             OCF_THREADOWNED),


    /* TYPE_TIMER - TIMER(HEAD) */
    HTIENTRY("Timer", TIMER,
             FreeTimer,
             TAG_TIMER,
             0),

    /* TYPE_INPUTCONTEXT - IMC(THRDESKHEAD) */
    HTIENTRY("Input Context", IMC,
             FreeInputContext,
             0,
             OCF_THREADOWNED | OCF_DESKTOPHEAP),
};

/*
 * Handle table allocation globals.  The purpose of keeping per-page free
 * lists is to keep the table as small as is practical and to minimize
 * the number of pages touched while performing handle table operations.
 */
#define CPAGEENTRIESINIT    4

DWORD gcHandlePages;
PHANDLEPAGE gpHandlePages;

#if DBG
PPAGED_LOOKASIDE_LIST LockRecordLookaside;

NTSTATUS InitLockRecordLookaside();
void FreeLockRecord(PLR plr);
void InitGlobalThreadLockArray(DWORD dwIndex);
#endif

void HMDestroyUnlockedObject(PHE phe);
void HMRecordLock(PVOID ppobj, PVOID pobj, DWORD cLockObj);
BOOL HMUnrecordLock(PVOID ppobj, PVOID pobj);
VOID ShowLocks(PHE);


/***************************************************************************\
* DBGValidateHandleQuota
*
* 11-19-97 GerardoB         Created.
\***************************************************************************/
#ifdef VALIDATEHANDLEQUOTA
void DBGValidateHandleQuota (void)
{
    BYTE bCreateFlags;
    DWORD dw;
    HANDLEENTRY * phe;

    PPROCESSINFO ppiT = gppiList;

    while (ppiT != NULL) {
        ppiT->lHandles = 0;
        ppiT = ppiT->ppiNextRunning;
    }

    phe = gSharedInfo.aheList;
    for (dw = 0; dw <= giheLast; dw++, phe++) {
        if (phe->bType == TYPE_FREE) {
            UserAssert(phe->pOwner == NULL);
            continue;
        }
        bCreateFlags = gahti[phe->bType].bObjectCreateFlags;
        if (bCreateFlags & OCF_PROCESSOWNED) {
            ((PPROCESSINFO)phe->pOwner)->lHandles++;
            continue;
        }
        if (bCreateFlags & OCF_THREADOWNED) {
            ((PTHREADINFO)phe->pOwner)->ppi->lHandles++;
            continue;
        }
        UserAssert(phe->pOwner == NULL);
    }

    ppiT = gppiList;
    while (ppiT != NULL) {
        UserAssert(ppiT->lHandles == ppiT->UserHandleCount);
        ppiT = ppiT->ppiNextRunning;
    }
}
#else
#define DBGValidateHandleQuota()
#endif
/***************************************************************************\
* DBGHMPheFromObject
*
* Validates and returns the HANDLEENTRY corresponding to a given object
*
* 09-23-97 GerardoB         Created.
\***************************************************************************/
#if DBG
PHE DBGHMPheFromObject (PVOID p)
{
    PHE phe = _HMPheFromObject(p);

    UserAssert(phe->phead == p);
    UserAssert(_HMObjectFromHandle(phe->phead->h) == p);
    UserAssert(phe->wUniq == HMUniqFromHandle(phe->phead->h));
    UserAssert(phe->bType < TYPE_CTYPES);
    UserAssert((phe->pOwner != NULL)
                || !(gahti[phe->bType].bObjectCreateFlags & (OCF_PROCESSOWNED | OCF_THREADOWNED)));
    UserAssert(!(phe->bFlags & ~HANDLEF_VALID));

    return phe;
}
#endif
/***************************************************************************\
* DBGHMPheFromObject
*
* Validates and returns the object corresponding to a given handle.
*
* 09-23-97 GerardoB         Created.
\***************************************************************************/
#if DBG
PVOID DBGHMObjectFromHandle (HANDLE h)
{
    PVOID p = _HMObjectFromHandle(h);

    UserAssert((h != NULL) ^ (p == NULL));
    if (p != NULL) {
        UserAssert(HMIndexFromHandle(((PHEAD)p)->h) == HMIndexFromHandle(h));
        UserAssert(p == HMRevalidateCatHandle(h));
    /*
     * This routine, unlike Validation, should return a real pointer if
     * the object exists, even if it is destroyed.  But we should still
     * generate a warning.
     */
        if (HMPheFromObject(p)->bFlags & HANDLEF_DESTROY) {
            RIPMSG1(RIP_WARNING, "HMObjectFromHandle: Object p %#p is destroyed",
                    p);
        }
    }

    return p;
}
PVOID DBGHMCatObjectFromHandle (HANDLE h)
{
    /*
     * Note -- at this point, _HMObjectFromHandle does not check
     *   to see if an object is destroyed.
     */
    PVOID p = _HMObjectFromHandle(h);

    UserAssert((h != NULL) ^ (p == NULL));
    if (p != NULL) {
        UserAssert(HMIndexFromHandle(((PHEAD)p)->h) == HMIndexFromHandle(h));
        UserAssert(p == HMRevalidateCatHandle(h));
    }

    return p;
}
#endif
/***************************************************************************\
* DBGPtoH and DBGPtoHq
*
* Validates and returns the handle corresponding to a given object
*
* 09-23-97 GerardoB         Created.
\***************************************************************************/
#if DBG
void DBGValidatePtoH (PVOID p, HANDLE h)
{
    UserAssert((h != NULL) ^ (p == NULL));
    if (h != NULL) {
        UserAssert(p == HMRevalidateCatHandle(h));
    }
}
HANDLE DBGPtoH (PVOID p)
{
    HANDLE h = _PtoH(p);
    DBGValidatePtoH(p, h);
    return h;
}
HANDLE DBGPtoHq (PVOID p)
{
    HANDLE h;
    UserAssert(p != NULL);
    h = _PtoHq(p);
    DBGValidatePtoH(p, h);
    return h;
}
#endif
/***************************************************************************\
* DBGHW and DBGHWq
*
* Validates and returns the hwnd corresponding to a given pwnd
*
* 09-23-97 GerardoB         Created.
\***************************************************************************/
#if DBG
void DBGValidateHW(PWND pwnd, HWND hwnd)
{
    UserAssert((hwnd != NULL) ^ (pwnd == NULL));
    if (hwnd != NULL) {
        UserAssert(pwnd == HMValidateCatHandleNoSecure(hwnd, TYPE_WINDOW));
    }
}
void DBGValidateHWCCX(PWND ccxPwnd, HWND hwnd, PCLIENTINFO ccxPci)
{
    UserAssert((hwnd != NULL) ^ (ccxPwnd == NULL));
    if (hwnd != NULL) {
        UserAssert(ccxPwnd == HMValidateCatHandleNoSecureCCX(hwnd, TYPE_WINDOW, ccxPci));
    }
}
HWND DBGHW (PWND pwnd)
{
    HWND hwnd = _HW(pwnd);
    DBGValidateHW(pwnd, hwnd);
    return hwnd;
}
HWND DBGHWCCX (PWND ccxPwnd)
{
    HWND hwnd = _HWCCX(ccxPwnd);
    PCLIENTINFO ccxPci = _GETPTI(ccxPwnd)->pClientInfo;
    if (KeIsAttachedProcess()) {
        UserAssert(KeGetCurrentThread()->ApcState.Process == &(_GETPTI(ccxPwnd)->ppi->Process->Pcb));
    } else {
        UserAssert(PpiCurrent() == _GETPTI(ccxPwnd)->ppi);
    }
    DBGValidateHWCCX(ccxPwnd, hwnd, ccxPci);
    return hwnd;
}
HWND DBGHWq (PWND pwnd)
{
    HWND hwnd;
    UserAssert(pwnd != NULL);
    hwnd = _HWq(pwnd);
    DBGValidateHW(pwnd, hwnd);
    return hwnd;
}
#endif
/***************************************************************************\
* DBGHMValidateFreeLists
*
* Walks all handle free lists to make sure all links are fine.
*
* 10/08/97  GerardoB    Created
\***************************************************************************/
#if DBG
void DBGHMValidateFreeList (ULONG_PTR iheFreeNext, BOOL fEven)
{
    PHE phe;
    do {
        UserAssert(fEven ^ !!(iheFreeNext & 0x1));
        UserAssert(iheFreeNext < gpsi->cHandleEntries);
        phe = &gSharedInfo.aheList[iheFreeNext];
        UserAssert(phe->bType == TYPE_FREE);
        UserAssert(phe->pOwner == NULL);
        UserAssert(phe->bFlags == 0);
        iheFreeNext = (ULONG_PTR)phe->phead;
    } while (iheFreeNext != 0);
}
void DBGHMValidateFreeLists (void)
{
    DWORD dw;
    PHANDLEPAGE php = gpHandlePages;

    for (dw = 0; dw < gcHandlePages; ++dw, ++php) {
        if (php->iheFreeEven != 0) {
            DBGHMValidateFreeList(php->iheFreeEven, TRUE);
        }
        if (php->iheFreeOdd != 0) {
            DBGHMValidateFreeList(php->iheFreeOdd, FALSE);
        }
    } /* for */
}
#else
#define DBGHMValidateFreeLists()
#endif

#if DBG

/***************************************************************************\
* DbgDumpHandleTable
*
\***************************************************************************/
DWORD DbgDumpHandleTable(
    VOID)
{
    DWORD dw;
    PHE   phe;
    DWORD dwHandles = 0;

    phe = gSharedInfo.aheList;

    if (phe == NULL) {
        KdPrint(("\nTERMSRV\nEmpty handle table\n"));
        return 0;
    }

    KdPrint(("\nTERMSRV\nDump the handle table\n"));
    KdPrint(("---------------------------------------------------\n"));
    KdPrint(("     phead    handle   lock     pOwner   type flags\n"));
    KdPrint(("---------------------------------------------------\n"));

    for (dw = 0; dw <= giheLast; dw++, phe++) {
        if (phe->bType == TYPE_FREE) {
            UserAssert(phe->pOwner == NULL);
            continue;
        }

        KdPrint(("%04d %08x %08x %08d %08x %04x %05x\n",
                 dwHandles++,
                 phe->phead,
                 phe->phead->h,
                 phe->phead->cLockObj,
                 phe->pOwner,
                 phe->bType,
                 phe->bFlags));
    }

    KdPrint(("----------------------------------------------\n"));
    KdPrint(("Number of handles left: %d\n", dwHandles));
    KdPrint(("End of handle table\n"));

    UserAssert(dwHandles == 0);

    return dwHandles;
}

/***************************************************************************\
* HMCleanUpHandleTable
*
\***************************************************************************/
VOID HMCleanUpHandleTable(
    VOID)
{
    DbgDumpHandleTable();

    if (LockRecordLookaside != NULL) {
        ExDeletePagedLookasideList(LockRecordLookaside);
        UserFreePool(LockRecordLookaside);
    }
}
#endif // DBG

/***************************************************************************\
* HMInitHandleEntries
*
* 10/10/97  GerardoB    Extracted from HMInitHandleTable and HMGrowHandleTable
\***************************************************************************/
void HMInitHandleEntries (ULONG_PTR iheFirstFree)
{
    ULONG_PTR ihe;
    PHE      pheT;
    /*
     * Zero out all the new entries
     */
    RtlZeroMemory (&gSharedInfo.aheList[iheFirstFree],
                    (gpsi->cHandleEntries - iheFirstFree) * sizeof(HANDLEENTRY));
    /*
     * Link them together.
     * Each free odd/even entry points to the next odd/even free entry.
     */
    ihe = iheFirstFree;
    for (pheT = &gSharedInfo.aheList[ihe]; ihe < gpsi->cHandleEntries; ihe++, pheT++) {
        pheT->phead = (PHEAD)(ihe + 2);
        /* pheT->bType = TYPE_FREE; */
        pheT->wUniq = 1;
    }
    /*
     * Terminate the lists.
     */
    if (gpsi->cHandleEntries > iheFirstFree) {
        UserAssert(pheT - 1 >= &gSharedInfo.aheList[iheFirstFree]);
        (pheT - 1)->phead = NULL;
    }
    if (gpsi->cHandleEntries > iheFirstFree + 1) {
        UserAssert(pheT - 2 >= &gSharedInfo.aheList[iheFirstFree]);
        (pheT - 2)->phead = NULL;
    }
    /*
     * Let's check that we got it right
     */
    DBGHMValidateFreeLists();
}
/***************************************************************************\
* HMInitHandleTable
*
* Initialize the handle table. Unused entries are linked together.
*
* 01-13-92 ScottLu      Created.
\***************************************************************************/

#define CHANDLEENTRIESINIT 200
#define CLOCKENTRIESINIT   100

BOOL HMInitHandleTable(
    PVOID pReadOnlySharedSectionBase)
{
    NTSTATUS Status;
    SIZE_T   ulCommit;

    /*
     * Allocate the handle page array.  Make it big enough
     * for 4 pages, which should be sufficient for nearly
     * all instances.
     */
    gpHandlePages = UserAllocPool(
            CPAGEENTRIESINIT * sizeof(HANDLEPAGE), TAG_SYSTEM);

    if (gpHandlePages == NULL)
        return FALSE;

#if DBG
    if (!NT_SUCCESS(InitLockRecordLookaside()))
        return FALSE;
#endif

    /*
     * Allocate the array.  We have the space from
     * NtCurrentPeb()->ReadOnlySharedMemoryBase to
     * NtCurrentPeb()->ReadOnlySharedMemoryHeap reserved for
     * the handle table.  All we need to do is commit the pages.
     *
     * Compute the minimum size of the table.  The allocation will
     * round this up to the next page size.
     */
    ulCommit = gpsi->cbHandleTable = PAGE_SIZE;
    Status = CommitReadOnlyMemory(ghSectionShared, &ulCommit, 0, NULL);

    if (!NT_SUCCESS(Status))
        return FALSE;

    gSharedInfo.aheList = pReadOnlySharedSectionBase;
    gpsi->cHandleEntries = gpsi->cbHandleTable / sizeof(HANDLEENTRY);
    gcHandlePages = 1;

    /*
     * Initialize the handlepage info. Handle 0 is reserved so even free list
     *  starts at 2.
     */
    gpHandlePages[0].iheFreeOdd = 1;
    gpHandlePages[0].iheFreeEven = 2;
    gpHandlePages[0].iheLimit = gpsi->cHandleEntries;
    /*
     * Initialize the handle entries.
     */
    HMInitHandleEntries(0);
    /*
     * PW(NULL) (ie, handle 0) must map to a NULL pointer.
     * Old comment:
     * Reserve the first handle table entry so that PW(NULL) maps to a
     * NULL pointer. Set it to TYPE_FREE so the cleanup code doesn't think
     * it is allocated. Set wUniq to 1 so that RevalidateHandles on NULL
     * will fail.
     */
    gSharedInfo.aheList[0].phead = NULL;
    UserAssert(gSharedInfo.aheList[0].bType == TYPE_FREE);
    UserAssert(gSharedInfo.aheList[0].wUniq == 1);

#if DBG
    /*
     * Make sure we don't need to add the special case to handle HMINDEXBITS in this function.
     */
    UserAssert(gpsi->cHandleEntries <= HMINDEXBITS);
    /*
     * PDESKOBJHEAD won't do the right casting unless these structs have
     *  the same size.
     */
    UserAssert(sizeof(THROBJHEAD) == sizeof(PROCOBJHEAD));
    UserAssert(sizeof(THRDESKHEAD) == sizeof(PROCDESKHEAD));
    UserAssert(sizeof(THRDESKHEAD) == sizeof(DESKOBJHEAD));
    /*
     * Validate type flags to make sure that assumptions made
     *  throughout HM code are OK.
     */
    {
        HANDLETYPEINFO * pahti = (HANDLETYPEINFO *) gahti;
        UINT uTypes = TYPE_CTYPES;
        BYTE bObjectCreateFlags;
        while (uTypes-- != 0) {
            bObjectCreateFlags = pahti->bObjectCreateFlags;
            /*
             * Illegal flag combinations
             */
            UserAssert(!((bObjectCreateFlags & OCF_DESKTOPHEAP) && (bObjectCreateFlags & OCF_MARKPROCESS)));
            /*
             * Pointless (and probably illegal) flag combinations
             */
            UserAssert(!((bObjectCreateFlags & OCF_DESKTOPHEAP) && (bObjectCreateFlags & OCF_SHAREDHEAP)));
            UserAssert(!((bObjectCreateFlags & OCF_USEPOOLQUOTA) && (bObjectCreateFlags & OCF_SHAREDHEAP)));
            UserAssert(!((bObjectCreateFlags & OCF_THREADOWNED) && (bObjectCreateFlags & OCF_PROCESSOWNED)));
            UserAssert(!(bObjectCreateFlags & OCF_USEPOOLQUOTA)
                        || !(bObjectCreateFlags & OCF_DESKTOPHEAP)
                        || (bObjectCreateFlags & OCF_USEPOOLIFNODESKTOP));

            /*
             * Required flag combinations
             */
            UserAssert(!(bObjectCreateFlags & OCF_DESKTOPHEAP)
                        || (bObjectCreateFlags & (OCF_PROCESSOWNED | OCF_THREADOWNED)));

            UserAssert(!(bObjectCreateFlags & OCF_MARKPROCESS)
                        || (bObjectCreateFlags & OCF_PROCESSOWNED));

            UserAssert(!(bObjectCreateFlags & OCF_USEPOOLIFNODESKTOP)
                        || (bObjectCreateFlags & OCF_DESKTOPHEAP));


            pahti++;
        } /* while (uTypes-- != 0) */
    }
#endif

    return TRUE;
}

/***************************************************************************\
* HMGrowHandleTable
*
* Grows the handle table. Assumes the handle table already exists.
*
* 01-13-92 ScottLu      Created.
\***************************************************************************/

BOOL HMGrowHandleTable()
{
    ULONG_PTR   i, iheFirstFree;
    PHE         pheT;
    PVOID       p;
    PHANDLEPAGE phpNew;
    DWORD       dwCommitOffset;
    SIZE_T      ulCommit;
    NTSTATUS    Status;

    /*
     * If we've run out of handle space, fail.
     */
    i = gpsi->cHandleEntries;
    if (i & ~HMINDEXBITS)
        return FALSE;

    /*
     * Grow the page table if need be.
     */
    i = gcHandlePages + 1;
    if (i > CPAGEENTRIESINIT) {
        DWORD dwSize = gcHandlePages * sizeof(HANDLEPAGE);

        phpNew = UserReAllocPool(
                gpHandlePages, dwSize, dwSize + sizeof(HANDLEPAGE), TAG_SYSTEM);

        if (phpNew == NULL)
            return FALSE;

        gpHandlePages = phpNew;
    }

    /*
     * Commit some more pages to the table.  First find the
     * address where the commitment needs to be.
     */
    p = (PBYTE)gSharedInfo.aheList + gpsi->cbHandleTable;

    if (p >= Win32HeapGetHandle(gpvSharedAlloc)) {
        return FALSE;
    }

    dwCommitOffset = (ULONG)((PBYTE)p - (PBYTE)gpvSharedBase);

    ulCommit = PAGE_SIZE;

    Status = CommitReadOnlyMemory(ghSectionShared, &ulCommit, dwCommitOffset, NULL);

    if (!NT_SUCCESS(Status))
        return FALSE;

    phpNew = &gpHandlePages[gcHandlePages++];

    /*
     * Update the global information to include the new
     * page.
     */
    iheFirstFree = gpsi->cHandleEntries;
    if (gpsi->cHandleEntries & 0x1) {
        phpNew->iheFreeOdd = gpsi->cHandleEntries;
        phpNew->iheFreeEven = gpsi->cHandleEntries + 1;
    } else {
        phpNew->iheFreeEven = gpsi->cHandleEntries;
        phpNew->iheFreeOdd = gpsi->cHandleEntries + 1;
    }
    gpsi->cbHandleTable += PAGE_SIZE;

    /*
     * Check for handle overflow
     */
    gpsi->cHandleEntries = gpsi->cbHandleTable / sizeof(HANDLEENTRY);
    if (gpsi->cHandleEntries & ~HMINDEXBITS) {
        gpsi->cHandleEntries = (HMINDEXBITS + 1);
    }

    phpNew->iheLimit = gpsi->cHandleEntries;
    if (phpNew->iheFreeEven >= phpNew->iheLimit) {
        phpNew->iheFreeEven = 0;
    }
    if (phpNew->iheFreeOdd >= phpNew->iheLimit) {
        phpNew->iheFreeOdd = 0;
    }

    HMInitHandleEntries(iheFirstFree);

    /*
     * HMINDEXBITS has a special meaning. We used to handle this in HMAllocObject.
     * Now we handle it here right after adding that handle to the table.
     * Old Comment:
     * Reserve this table entry so that PW(HMINDEXBITS) maps to a
     * NULL pointer. Set it to TYPE_FREE so the cleanup code doesn't think
     * it is allocated. Set wUniq to 1 so that RevalidateHandles on HMINDEXBITS
     * will fail.
     */
    if ((gpsi->cHandleEntries > HMINDEXBITS)
            && (phpNew->iheFreeOdd != 0)
            && (phpNew->iheFreeOdd <= HMINDEXBITS)) {

        pheT = &gSharedInfo.aheList[HMINDEXBITS];
        if (phpNew->iheFreeOdd == HMINDEXBITS) {
            phpNew->iheFreeOdd = (ULONG_PTR)pheT->phead;
        } else {
            UserAssert(pheT - 2 >= &gSharedInfo.aheList[iheFirstFree]);
            UserAssert((pheT - 2)->phead == (PVOID)HMINDEXBITS);
            (pheT - 2)->phead = pheT->phead;
        }
        pheT->phead = NULL;
        UserAssert(pheT->bType == TYPE_FREE);
        UserAssert(pheT->wUniq == 1);
    }

    return TRUE;
}

/***************************************************************************\
* HMAllocObject
*
* Allocs a non-secure object by allocating a handle and memory for
* the object.
*
* 01-13-92 ScottLu      Created.
\***************************************************************************/

PVOID HMAllocObject(
    PTHREADINFO ptiOwner,
    PDESKTOP pdeskSrc,
    BYTE bType,
    DWORD size)
{
    DWORD       i;
    PHEAD       phead;
    PHE         pheT;
    ULONG_PTR    iheFree, *piheFreeHead;
    PHANDLEPAGE php;
    BYTE        bCreateFlags;
    PPROCESSINFO ppiQuotaCharge = NULL;
    BOOL        fUsePoolIfNoDesktop;
    BOOL        fEven;

    CheckCritIn();
    bCreateFlags = gahti[bType].bObjectCreateFlags;

#if DBG
    /*
     * Validate size
     */
    if (bCreateFlags & OCF_VARIABLESIZE) {
        UserAssert(gahti[bType].uSize <= size);
    } else {
        UserAssert(gahti[bType].uSize == size);
    }
#endif

    /*
     * Check for process handle quota
     */
    if (bCreateFlags & (OCF_PROCESSOWNED | OCF_THREADOWNED)) {
        UserAssert(ptiOwner != NULL);
        ppiQuotaCharge = ptiOwner->ppi;
        if (ppiQuotaCharge->UserHandleCount >= gUserProcessHandleQuota) {
            RIPERR0(ERROR_NO_MORE_USER_HANDLES,
                   RIP_WARNING,
                "USER: HMAllocObject: out of handle quota\n");
            return NULL;
        }
    }

    /*
     * Find the next free handle
     * Window handles must be even; hence we try first to use odd handles
     *  for all other objects.
     * Old comment:
     * Some wow apps, like WinProj, require even Window handles so we'll
     * accomodate them; build a list of the odd handles so they won't get lost
     * 10/13/97: WinProj never fixed this; even the 32 bit version has the problem.
     */
    fEven = (bType == TYPE_WINDOW);
    piheFreeHead = NULL;
    do {
        php = gpHandlePages;
        for (i = 0; i < gcHandlePages; ++i, ++php) {
            if (fEven) {
                if (php->iheFreeEven != 0) {
                    piheFreeHead = &php->iheFreeEven;
                    break;
                }
            } else {
                if (php->iheFreeOdd != 0) {
                    piheFreeHead = &php->iheFreeOdd;
                    break;
                }
            }
        } /* for */
        /*
         * If we couldn't find an odd handle, then search for an even one
         */
        fEven = ((piheFreeHead == NULL) && !fEven);
    } while (fEven);
    /*
     * If there are no free handles we can use, grow the table
     */
    if (piheFreeHead == NULL) {
        HMGrowHandleTable();
        /*
         * If the table didn't grow, get out.
         */
        if (i == gcHandlePages) {
            RIPMSG0(RIP_WARNING, "HMAllocObject: could not grow handle space");
            return NULL;
        }
        /*
         * Because the handle page table may have moved,
         * recalc the page entry pointer.
         */
        php = &gpHandlePages[i];
        piheFreeHead = (bType == TYPE_WINDOW ? &php->iheFreeEven : &php->iheFreeOdd);
        if (*piheFreeHead == 0) {
            UserAssert(gpsi->cHandleEntries == (HMINDEXBITS + 1));
            RIPMSG0(RIP_WARNING, "HMAllocObject: handle table is full");
            return NULL;
        }
    }
    /*
     * HMINDEXBITS is a reserved value that should never be in the free lists
     *  (see HMGrowHandleTable());
     */
    UserAssert(HMIndexFromHandle(*piheFreeHead) != HMINDEXBITS);
    /*
     * Try to allocate the object. If this fails, bail out.
     */
    if ((bCreateFlags & OCF_DESKTOPHEAP) && pdeskSrc) {
        phead = (PHEAD)DesktopAlloc(pdeskSrc, size, DTAG_HANDTABL);
        if (phead) {
            LockDesktop(&((PDESKOBJHEAD)phead)->rpdesk, pdeskSrc, LDL_OBJ_DESK, (ULONG_PTR)phead);
            ((PDESKOBJHEAD)phead)->pSelf = (PBYTE)phead;
        }
    } else if (bCreateFlags & OCF_SHAREDHEAP) {
        UserAssert(!pdeskSrc);
        phead = (PHEAD)SharedAlloc(size);
    } else {
        fUsePoolIfNoDesktop = !pdeskSrc && (bCreateFlags & OCF_USEPOOLIFNODESKTOP);
        UserAssert(!(bCreateFlags & OCF_DESKTOPHEAP) || fUsePoolIfNoDesktop);

        if ((bCreateFlags & OCF_USEPOOLQUOTA) && !fUsePoolIfNoDesktop) {
            phead = (PHEAD)UserAllocPoolWithQuotaZInit(size, gahti[bType].dwAllocTag);
        } else {
            phead = (PHEAD)UserAllocPoolZInit(size, gahti[bType].dwAllocTag);
        }
    }

    if (phead == NULL) {
        RIPERR0(ERROR_NOT_ENOUGH_MEMORY,
                RIP_WARNING,
                "USER: HMAllocObject: out of memory");
        return NULL;
    }
    /*
     * We're going to use this handle so get it off its free list.
     * The free handle phead points to the next free handle.
     */
    iheFree = *piheFreeHead;
    pheT = &gSharedInfo.aheList[iheFree];
    *piheFreeHead = (ULONG_PTR)pheT->phead;
    DBGHMValidateFreeLists();
    /*
     * Track high water mark for handle allocation.
     */
    if ((DWORD)iheFree > giheLast) {
        giheLast = (DWORD)iheFree;
    }
    /*
     * Setup the handle contents, plus initialize the object header.
     */
    pheT->bType = bType;
    pheT->phead = phead;
    UserAssert(pheT->bFlags == 0);
    if (bCreateFlags & OCF_PROCESSOWNED) {
        if ((ptiOwner->TIF_flags & TIF_16BIT) && (ptiOwner->ptdb)) {
            ((PPROCOBJHEAD)phead)->hTaskWow = ptiOwner->ptdb->hTaskWow;
        } else {
            ((PPROCOBJHEAD)phead)->hTaskWow = 0;
        }
        pheT->pOwner = ptiOwner->ppi;
        if (bCreateFlags & OCF_MARKPROCESS) {
            ((PPROCMARKHEAD)phead)->ppi = ptiOwner->ppi;
        }
    } else if (bCreateFlags & OCF_THREADOWNED) {
        ((PTHROBJHEAD)phead)->pti = pheT->pOwner = ptiOwner;
    } else {
        /*
         * The caller is wasting time if ptiOwner != NULL
         * The handle entry must already have pOwner == NULL.
         */
        UserAssert(ptiOwner == NULL);
        UserAssert(pheT->pOwner == NULL);
    }

    phead->h = HMHandleFromIndex(iheFree);

    if (ppiQuotaCharge) {
        ppiQuotaCharge->UserHandleCount++;
        DBGValidateHandleQuota();
    }

#if DBG
    /*
     *   performance counter dumphmgr
     *   dwPrevCount is used for the snapshot option
     */

    gaPerfhti[bType].lTotalCount++;
    gaPerfhti[bType].lCount++;
    if (gaPerfhti[bType].lCount > gaPerfhti[bType].lMaxCount) {
        gaPerfhti[bType].lMaxCount = gaPerfhti[bType].lCount;
    }
    if ((bCreateFlags & OCF_DESKTOPHEAP) && (pdeskSrc != NULL)) {
        gaPerfhti[bType].lSize += RtlSizeHeap(Win32HeapGetHandle(pdeskSrc->pheapDesktop), 0, phead);
    } else if (bCreateFlags & OCF_SHAREDHEAP) {
        gaPerfhti[bType].lSize += RtlSizeHeap(Win32HeapGetHandle(gpvSharedAlloc), 0, phead);
    } else {
        unsigned char  notUsed;
        gaPerfhti[bType].lSize += ExQueryPoolBlockSize(phead, &notUsed);
    }

#endif // DBG

    /*
     * Return a handle entry pointer.
     */
    return pheT->phead;
}



#if 0
#define HANDLEF_FREECHECK 0x80
VOID CheckHMTable(
    PVOID pobj)
{
    PHE pheT, pheMax;

    if (giheLast) {
        pheMax = &gSharedInfo.aheList[giheLast];
        for (pheT = gSharedInfo.aheList; pheT <= pheMax; pheT++) {
            if (pheT->bType == TYPE_FREE) {
                continue;
            }
            if (pheT->phead == pobj && !(pheT->bFlags & HANDLEF_FREECHECK)) {
                UserAssert(FALSE);
            }
        }
    }
}
#endif


/***************************************************************************\
* HMFreeObject
*
* Frees an object - the handle and the referenced memory.
*
* 01-13-92 ScottLu      Created.
\***************************************************************************/

BOOL HMFreeObject(
    PVOID pobj)
{
    PHE         pheT;
    WORD        wUniqT;
    PHANDLEPAGE php;
    DWORD       i;
    ULONG_PTR    iheCurrent, *piheCurrentHead;
    BYTE        bCreateFlags;
    PDESKTOP    pdesk;
    PPROCESSINFO ppiQuotaCharge = NULL;
#if DBG
    PLR         plrT, plrNextT;
    unsigned char  notUsed;
#endif

    UserAssert(((PHEAD)pobj)->cLockObj == 0);
    UserAssert(pobj == HtoPqCat(PtoHq(pobj)));
    /*
     * Free the object first.
     */
    pheT = HMPheFromObject(pobj);
    bCreateFlags = gahti[pheT->bType].bObjectCreateFlags;

    UserAssertMsg1(pheT->bType != TYPE_FREE,
                   "Object already marked as freed!!! %#p", pobj);

    /*
     * decr process handle use
     */
    if (bCreateFlags & OCF_PROCESSOWNED) {
        ppiQuotaCharge = (PPROCESSINFO)pheT->pOwner;
        UserAssert(ppiQuotaCharge != NULL);
    } else if (bCreateFlags & OCF_THREADOWNED) {
        ppiQuotaCharge = (PPROCESSINFO)(((PTHREADINFO)(pheT->pOwner))->ppi);
        UserAssert(ppiQuotaCharge != NULL);
    } else {
        ppiQuotaCharge = NULL;
    }

    if (ppiQuotaCharge != NULL) {
        ppiQuotaCharge->UserHandleCount--;
    }

    if (pheT->bFlags & HANDLEF_GRANTED) {
        HMCleanupGrantedHandle(pheT->phead->h);
        pheT->bFlags &= ~HANDLEF_GRANTED;
    }

#if DBG
    /*
     *   performance counters
     */
    gaPerfhti[pheT->bType].lCount--;

    if ((bCreateFlags & OCF_DESKTOPHEAP) && ((PDESKOBJHEAD)pobj)->rpdesk) {
        pdesk = ((PDESKOBJHEAD)pobj)->rpdesk;
        gaPerfhti[pheT->bType].lSize -= RtlSizeHeap(Win32HeapGetHandle(pdesk->pheapDesktop), 0, pobj);
    } else if (bCreateFlags & OCF_SHAREDHEAP) {
        gaPerfhti[pheT->bType].lSize -= RtlSizeHeap(Win32HeapGetHandle(gpvSharedAlloc), 0, pobj);
    } else {
        gaPerfhti[pheT->bType].lSize -= ExQueryPoolBlockSize(pobj, &notUsed);
    }

#endif // DBG

    if ((bCreateFlags & OCF_DESKTOPHEAP)) {
#if DBG
        BOOL bSuccess;
#endif
        UserAssert(((PDESKOBJHEAD)pobj)->rpdesk != NULL);

        pdesk = ((PDESKOBJHEAD)pobj)->rpdesk;
        UnlockDesktop(&((PDESKOBJHEAD)pobj)->rpdesk, LDU_OBJ_DESK, (ULONG_PTR)pobj);

        if (pheT->bFlags & HANDLEF_POOL) {
            UserFreePool(pobj);
        } else {

#if DBG
            bSuccess =
#endif
            DesktopFree(pdesk, pobj);
#if DBG
            if (!bSuccess) {
                /*
                 * We would hit this assert in HYDRA trying to free the
                 * mother desktop window which was allocated out of pool
                 */
                RIPMSG1(RIP_ERROR, "Object already freed from desktop heap! %#p", pobj);
            }
#endif
        }

    } else if (bCreateFlags & OCF_SHAREDHEAP) {
        SharedFree(pobj);
    } else {
        UserFreePool(pobj);
    }

#if DBG
    /*
     * Go through and delete the lock records, if they exist.
     */
    for (plrT = pheT->plr; plrT != NULL; plrT = plrNextT) {

        /*
         * Remember the next one before freeing this one.
         */
        plrNextT = plrT->plrNext;
        FreeLockRecord((HANDLE)plrT);
    }
#endif

    /*
     * Clear the handle contents. Need to remember the uniqueness across
     * the clear. Also, advance uniqueness on free so that uniqueness checking
     * against old handles also fails.
     */
    wUniqT = (WORD)((pheT->wUniq + 1) & HMUNIQBITS);
    RtlZeroMemory(pheT, sizeof(HANDLEENTRY));
    pheT->wUniq = wUniqT;

    /*
     * Change the handle type to TYPE_FREE so we know what type this handle
     * is. (TYPE_FREE is defined as zero)
     */
    /* pheT->bType = TYPE_FREE; */
    UserAssert(pheT->bType == TYPE_FREE);

    /*
     * Put the handle on the free list of the appropriate page.
     */
    php = gpHandlePages;
    iheCurrent = pheT - gSharedInfo.aheList;
    for (i = 0; i < gcHandlePages; ++i, ++php) {
        if (iheCurrent < php->iheLimit) {
            piheCurrentHead = (iheCurrent & 0x1 ? &php->iheFreeOdd : &php->iheFreeEven);
            pheT->phead = (PHEAD)*piheCurrentHead;
            *piheCurrentHead = iheCurrent;
            DBGHMValidateFreeLists();
            break;
        }
    }
    /*
     * We must have found it.
     */
    UserAssert(i < gcHandlePages);

    UserAssert(pheT->pOwner == NULL);

    DBGValidateHandleQuota();

    return TRUE;
}


/***************************************************************************\
* HMMarkObjectDestroy
*
* Marks an object for destruction.
*
* Returns TRUE if the object can be destroyed; that is, if it's
* lock count is 0.
*
* 02-10-92 ScottLu      Created.
\***************************************************************************/

BOOL HMMarkObjectDestroy(
    PVOID pobj)
{
    PHE phe;

    phe = HMPheFromObject(pobj);

#if DEBUGTAGS
    /*
     * Record where the object was marked for destruction.
     */
    if (IsDbgTagEnabled(DBGTAG_TrackLocks)) {
        if (!(phe->bFlags & HANDLEF_DESTROY)) {
            HMRecordLock(LOCKRECORD_MARKDESTROY, pobj, ((PHEAD)pobj)->cLockObj);
        }
    }
#endif

    /*
     * Set the destroy flag so our unlock code will know we're trying to
     * destroy this object.
     */
    phe->bFlags |= HANDLEF_DESTROY;

    /*
     * If this object can't be destroyed, then CLEAR the HANDLEF_INDESTROY
     * flag - because this object won't be currently "in destruction"!
     * (if we didn't clear it, when it was unlocked it wouldn't get destroyed).
     */
    if (((PHEAD)pobj)->cLockObj != 0) {
        phe->bFlags &= ~HANDLEF_INDESTROY;

        /*
         * Return FALSE because we can't destroy this object.
         */
        return FALSE;
    }

#if DBG
    /*
     * Ensure that this function only returns TRUE once.
     */
    UserAssert(!(phe->bFlags & HANDLEF_MARKED_OK));
    phe->bFlags |= HANDLEF_MARKED_OK;
#endif

    /*
     * Return TRUE because Lock count is zero - ok to destroy this object.
     */
    return TRUE;
}


/***************************************************************************\
* HMDestroyObject
*
* This routine marks an object for destruction, and frees it if
* it is unlocked.
*
* 10-13-94 JimA         Created.
\***************************************************************************/

BOOL HMDestroyObject(
    PVOID pobj)
{
    /*
     * First mark the object for destruction.  This tells the locking code
     * that we want to destroy this object when the lock count goes to 0.
     * If this returns FALSE, we can't destroy the object yet (and can't get
     * rid of security yet either.)
     */

    if (!HMMarkObjectDestroy(pobj))
        return FALSE;

    /*
     * Ok to destroy...  Free the handle (which will free the object
     * and the handle).
     */
    HMFreeObject(pobj);
    return TRUE;
}

#if DBG

NTSTATUS
InitLockRecordLookaside()
{
    LockRecordLookaside = UserAllocPoolNonPaged(sizeof(PAGED_LOOKASIDE_LIST),
                                                TAG_LOOKASIDE);
    if (LockRecordLookaside == NULL) {
        return STATUS_NO_MEMORY;
    }

    ExInitializePagedLookasideList(LockRecordLookaside,
                                   NULL,
                                   NULL,
                                   SESSION_POOL_MASK,
                                   sizeof(LOCKRECORD),
                                   TAG_LOCKRECORD,
                                   1000);
    return STATUS_SUCCESS;
}

PLR AllocLockRecord()
{
    PLR plr;

    /*
     * Allocate a LOCKRECORD structure.
     */
    if ((plr = ExAllocateFromPagedLookasideList(LockRecordLookaside)) == NULL) {
        return NULL;
    }

    RtlZeroMemory(plr, sizeof(*plr));

    return plr;
}


void FreeLockRecord(
    PLR plr)
{
    ExFreeToPagedLookasideList(LockRecordLookaside, plr);
}


/***************************************************************************\
* HMRecordLock
*
* This routine records a lock on a "lock list", so that locks and unlocks
* can be tracked in the debugger. Only called if DBGTAG_TrackLocks is enabled.
*
* 02-27-92 ScottLu      Created.
\***************************************************************************/
void HMRecordLock(
    PVOID ppobj,
    PVOID pobj,
    DWORD cLockObj)
{
    PHE   phe;
    PLR   plr;
    int   i;
    ULONG hash;

    phe = HMPheFromObject(pobj);

    if ((plr = AllocLockRecord()) == NULL) {
        RIPMSG0(RIP_ERROR, "HMRecordLock failed to allocate memory");
        return;
    }

    /*
     * Link it in front of the list
     */
    plr->plrNext = phe->plr;
    phe->plr = plr;

    /*
     * This propably happens only for unmatched locks
     */
    if (((PHEAD)pobj)->cLockObj > cLockObj) {

        RIPMSG3(RIP_WARNING, "Unmatched lock. ppobj %#p pobj %#p cLockObj %d",
               ppobj, pobj, cLockObj);

        i = (int)cLockObj;
        i = -i;
        cLockObj = (DWORD)i;
    }

    plr->ppobj    = ppobj;
    plr->cLockObj = cLockObj;

    GetStackTrace(1,
                  LOCKRECORD_STACK,
                  plr->trace,
                  &hash);
    return;
}
#endif // DBG


/***************************************************************************\
* HMLockObject
*
* This routine locks an object. This is a macro in retail systems.
*
* 02-24-92 ScottLu      Created.
\***************************************************************************/

#if DBG
void HMLockObject(
    PVOID pobj)
{
    HANDLE h;
    PVOID  pobjValidate;

    /*
     * Validate by going through the handle entry so that we make sure pobj
     * is not just pointing off into space. This may GP fault, but that's
     * ok: this case should not ever happen if we're bug free.
     */

    h = HMPheFromObject(pobj)->phead->h;
    pobjValidate = HMRevalidateCatHandle(h);
    if (!pobj || pobj != pobjValidate) {
        RIPMSG2(RIP_ERROR,
                "HMLockObject invalid object %#p, handle %#p",
                pobj, h);
        return;
    }

    /*
     * Inc the reference count.
     */
    ((PHEAD)pobj)->cLockObj++;

    if (((PHEAD)pobj)->cLockObj == 0)
        RIPMSG1(RIP_ERROR, "Object lock count has overflowed: %#p", pobj);
}
#endif // DBG


/***************************************************************************\
* HMUnlockObjectInternal
*
* This routine is called from the macro HMUnlockObject when an object's
* reference count drops to zero. This routine will destroy an object
* if is has been marked for destruction.
*
* 01-21-92 ScottLu      Created.
\***************************************************************************/

PVOID HMUnlockObjectInternal(
    PVOID pobj)
{
    PHE phe;

    /*
     * The object is not reference counted. If the object is not a zombie,
     * return success because the object is still around.
     */
    phe = HMPheFromObject(pobj);
    if (!(phe->bFlags & HANDLEF_DESTROY))
        return pobj;

    /*
     * We're destroying the object based on an unlock... Make sure it isn't
     * currently being destroyed! (It is valid to have lock counts go from
     * 0 to != 0 to 0 during destruction... don't want recursion into
     * the destroy routine.
     */
    if (phe->bFlags & HANDLEF_INDESTROY)
        return pobj;

    HMDestroyUnlockedObject(phe);
    return NULL;
}


/***************************************************************************\
* HMAssignmentLock
*
* This api is used for structure and global variable assignment.
* Returns pobjOld if the object was *not* destroyed. Means the object is
* still valid.
*
* 02-24-92 ScottLu      Created.
\***************************************************************************/

PVOID FASTCALL HMAssignmentLock(
    PVOID *ppobj,
    PVOID pobj)
{
    PVOID pobjOld;

    pobjOld = *ppobj;
    *ppobj = pobj;

    /*
     * Unlocks the old, locks the new.
     */
    if (pobjOld != NULL) {

        /*
         * if we are locking in the same object that is there then
         * it is a no-op but we don't want to do the Unlock and the Lock
         * because the unlock could free object and the lock would lock
         * in a freed pointer; 6410.
         */
        if (pobjOld == pobj) {
            return pobjOld;
        }

#if DEBUGTAGS

        /*
         * Track assignment locks.
         */
        if (IsDbgTagEnabled(DBGTAG_TrackLocks)) {
            if (!HMUnrecordLock(ppobj, pobjOld)) {
                HMRecordLock(ppobj, pobjOld, ((PHEAD)pobjOld)->cLockObj - 1);
            }
        }
#endif

    }


    if (pobj != NULL) {
        UserAssert(pobj == HMValidateCatHandleNoSecure(((PHEAD)pobj)->h, TYPE_GENERIC));
        if (HMIsMarkDestroy(pobj)) {
            RIPERR2(ERROR_INVALID_PARAMETER,
                    RIP_WARNING,
                    "HMAssignmentLock, locking object %#p marked for destruction at %#p\n",
                    pobj, ppobj);
        }
#if DEBUGTAGS

        /*
         * Track assignment locks.
         */
        if (IsDbgTagEnabled(DBGTAG_TrackLocks)) {
            HMRecordLock(ppobj, pobj, ((PHEAD)pobj)->cLockObj + 1);
            if (HMIsMarkDestroy(pobj)) {

                RIPMSG2(RIP_WARNING,
                        "Locking object %#p marked for destruction at %#p",
                        pobj, ppobj);
            }
        }
#endif
        HMLockObject(pobj);
    }

/*
 * This unlock has been moved from up above, so that we implement a
 * "lock before unlock" strategy.  Just in case pobjOld was the
 * only object referencing pobj, pobj won't go away when we unlock
 * pobjNew -- it will have been locked above.
 */

    if (pobjOld) {
        pobjOld = HMUnlockObject(pobjOld);
    }

    return pobjOld;
}


/***************************************************************************\
* HMAssignmentLock
*
* This api is used for structure and global variable assignment.
* Returns pobjOld if the object was *not* destroyed. Means the object is
* still valid.
*
* 02-24-92 ScottLu      Created.
\***************************************************************************/

PVOID FASTCALL HMAssignmentUnlock(
    PVOID *ppobj)
{
    PVOID pobjOld;

    pobjOld = *ppobj;
    *ppobj = NULL;

    /*
     * Unlocks the old, locks the new.
     */
    if (pobjOld != NULL) {

#if DEBUGTAGS

        /*
         * Track assignment locks.
         */
        if (IsDbgTagEnabled(DBGTAG_TrackLocks)) {
            if (!HMUnrecordLock(ppobj, pobjOld)) {
                HMRecordLock(ppobj, pobjOld, ((PHEAD)pobjOld)->cLockObj - 1);
            }
        }
#endif
        pobjOld = HMUnlockObject(pobjOld);
    }

    return pobjOld;
}


/***************************************************************************\
* IsValidThreadLock
*
* This routine checks to make sure that the thread lock structures passed
* in are valid.
*
* 03-17-92 ScottLu      Created.
* 02-22-99 MCostea      Also validate the shadow of the stack TL
*                       from gThreadLocksArray
\***************************************************************************/
#if DBG
VOID IsValidThreadLock(PTHREADINFO pti, PTL ptl, ULONG_PTR dwLimit, BOOLEAN fHM)
{
    /*
     * Check that ptl is a valid stack address
     * Allow (ULONG_PTR)ptl == dwLimit so we can call ValidateThreadLocks passing
     *  the address of the last thing we locked.
     */
    UserAssert((ULONG_PTR)ptl >= dwLimit);
    UserAssert((ULONG_PTR)ptl < (ULONG_PTR)KeGetCurrentThread()->StackBase);
    /*
     * Check ptl owner.
     */
    UserAssert(ptl->pW32Thread == (PW32THREAD)pti);
    /*
     * If this is an HM object, verify handle and lock count (guess max value)
     */
    if (fHM && (ptl->pobj != NULL)) {
        /*
         * The locked object could be a destroyed object.
         */
        UserAssert(ptl->pobj == HtoPqCat(PtoHq(ptl->pobj)));
        if (((PHEAD)ptl->pobj)->cLockObj >= 32000) {
            RIPMSG2(RIP_WARNING, "IsValidThreadLock: Object %#p has %d locks", ptl->pobj, ((PHEAD)ptl->pobj)->cLockObj);
        }
    }
    /*
     * Make sure the shadow in gThreadLocksArray is doing fine
     */
    UserAssert(ptl->ptl->ptl == ptl);
}
#endif

/***************************************************************************\
* ValidateThreadLocks
*
* This routine validates the thread lock list of a thread.
*
* 03-10-92 ScottLu      Created.
\***************************************************************************/

#if DBG
ULONG ValidateThreadLocks(PTL NewLock, PTL OldLock, ULONG_PTR dwLimit, BOOLEAN fHM)
{
    UINT uTLCount = 0;
    PTL ptlTopLock = OldLock;
    PTHREADINFO ptiCurrent;

    BEGIN_REENTERCRIT();

    ptiCurrent = PtiCurrent();

    /*
     * Validate the new thread lock.
     */
    if (NewLock != NULL) {
        UserAssert(NewLock->next == OldLock);
        IsValidThreadLock(ptiCurrent, NewLock, dwLimit, fHM);
        uTLCount++;
    }

    /*
     * Loop through the list of thread locks and check to make sure the
     * new lock is not in the list and that list is valid.
     */
    while (OldLock != NULL) {
        /*
         * The new lock must not be the same as the old lock.
         */
        UserAssert(NewLock != OldLock);
        /*
         * Validate the old thread lock.
         */
        IsValidThreadLock(ptiCurrent, OldLock, dwLimit, fHM);
        uTLCount++;
        OldLock = OldLock->next;
    }
    /*
     * If this is thread lock, set uTLCount, else verify it
     */
    if (NewLock != NULL) {
        NewLock->uTLCount = uTLCount;
    } else {
        if (ptlTopLock == NULL) {
            RIPMSG0(RIP_WARNING, "ptlTopLock is NULL, the system will AV now");
        }
        UserAssert(uTLCount == ptlTopLock->uTLCount);
    }

    END_REENTERCRIT();

    return uTLCount;
}
#endif


/***************************************************************************\
* CreateShadowTL
*
* This function creates a shaddow for the stack allocated ptl parameter
* in the global thread locks arrays
*
* 08-04-99 MCostea      Created.
\***************************************************************************/

#if DBG
void
CreateShadowTL(PTL ptl)
{
    PTL pTLNextFree;
    if (gFreeTLList->next == NULL) {
        UserAssert(gcThreadLocksArraysAllocated < MAX_THREAD_LOCKS_ARRAYS &&
                   "No more room in gpaThreadLocksArrays!  The system will bugcheck.");
        gFreeTLList->next = gpaThreadLocksArrays[gcThreadLocksArraysAllocated] =
            UserAllocPoolZInit(sizeof(TL)*MAX_THREAD_LOCKS, TAG_GLOBALTHREADLOCK);
        if (gFreeTLList->next == NULL) {
            UserAssert("Can't allocate memory for gpaThreadLocksArrays: the system will bugcheck soon!");
        }
        InitGlobalThreadLockArray(gcThreadLocksArraysAllocated);
        gcThreadLocksArraysAllocated++;
    }
    pTLNextFree = gFreeTLList->next;
    RtlCopyMemory(gFreeTLList, ptl, sizeof(TL));
    gFreeTLList->ptl = ptl;
    ptl->ptl = gFreeTLList;
    gFreeTLList = pTLNextFree;
}
#endif // DBG

/***************************************************************************\
* ThreadLock
*
* This api is used for locking objects across callbacks, so they are still
* there when the callback returns.
*
* 03-04-92 ScottLu      Created.
\***************************************************************************/

#if DBG
void
ThreadLock(
    PVOID pobj,
    PTL ptl)

{

    PTHREADINFO ptiCurrent;
    PVOID pfnT;


    /*
     * This is a handy place, because it is called so often, to see if User is
     * eating up too much stack.
     */
    UserAssert(((ULONG_PTR)&pfnT - (ULONG_PTR)KeGetCurrentThread()->StackLimit) > KERNEL_STACK_MINIMUM_RESERVE);

    /*
     * Store the address of the object in the thread lock structure and
     * link the structure into the thread lock list.
     *
     * N.B. The lock structure is always linked into the thread lock list
     *      regardless of whether the object address is NULL. The reason
     *      this is done is so the lock address does not need to be passed
     *      to the unlock function since the first entry in the lock list
     *      is always the entry to be unlocked.
     */

    UserAssert(!(PpiCurrent()->W32PF_Flags & W32PF_TERMINATED));
    ptiCurrent = PtiCurrent();
    UserAssert(ptiCurrent);

    /*
     * Get the callers address and validate the thread lock list.
     */
    RtlGetCallersAddress(&ptl->pfnCaller, &pfnT);
    ptl->pW32Thread = (PW32THREAD)ptiCurrent;

    ptl->next = ptiCurrent->ptl;
    ptiCurrent->ptl = ptl;
    ptl->pobj = pobj;
    if (pobj != NULL) {
        HMLockObject(pobj);
    }
    CreateShadowTL(ptl);
    ValidateThreadLocks(ptl, ptl->next, (ULONG_PTR)&pobj, TRUE);
    return;
}
#endif


/***************************************************************************\
* ThreadLockExchange
*
* Reuses a TL structure by locking the new object and unlocking
* the old one. This is used where you enumerate a list of
* structure locked objects, e.g. the window list.
*
* History:
* 05-Mar-1997 adams     Created.
\***************************************************************************/

#if DBG
PVOID
ThreadLockExchange(PVOID pobj, PTL ptl)
{
    PTHREADINFO ptiCurrent;
    PVOID       pobjOld;
    PVOID       pfnT;

    /*
     * This is a handy place, because it is called so often, to see if User is
     * eating up too much stack.
     */
    UserAssert((ULONG_PTR)&pfnT - (ULONG_PTR)KeGetCurrentThread()->StackLimit > KERNEL_STACK_MINIMUM_RESERVE);

    /*
     * Store the address of the object in the thread lock structure and
     * link the structure into the thread lock list.
     *
     * N.B. The lock structure is always linked into the thread lock list
     *      regardless of whether the object address is NULL. The reason
     *      this is done is so the lock address does not need to be passed
     *      to the unlock function since the first entry in the lock list
     *      is always the entry to be unlocked.
     */

    UserAssert(!(PpiCurrent()->W32PF_Flags & W32PF_TERMINATED));
    ptiCurrent = PtiCurrent();
    UserAssert(ptiCurrent);

    /*
     * Get the callers address.
     */
    RtlGetCallersAddress(&ptl->pfnCaller, &pfnT);
    UserAssert(ptl->pW32Thread == (PW32THREAD)ptiCurrent);

    /*
     * Remember the old object.
     */
    pobjOld = ptl->pobj;

    /*
     * Store and lock the new object. It is important to do this step
     * before unlocking the old object, since the new object might be
     * structure locked by the old object.
     */
    ptl->pobj = pobj;
    if (pobj != NULL) {
        HMLockObject(pobj);
    }

    /*
     * Unlock the old object.
     */
    if (pobjOld) {
        pobjOld = HMUnlockObject((PHEAD)pobjOld);
    }
    /*
     * Validate the entire thread lock list.
     */
    ValidateThreadLocks(NULL, ptiCurrent->ptl, (ULONG_PTR)&pobj, TRUE);
    {
        /*
         * Maintain gFreeTLList
         */
        UserAssert(ptl->ptl->ptl == ptl);
        UserAssert(ptl->ptl->pobj == pobjOld);
        ptl->ptl->pobj = pobj;
        ptl->ptl->pfnCaller = ptl->pfnCaller;
    }

    return pobjOld;
}
#endif


/*
 * The thread locking routines should be optimized for time, not size,
 * since they get called so often.
 */
#pragma optimize("t", on)

/***************************************************************************\
* ThreadUnlock1
*
* This api unlocks a thread locked object. Returns pobj if the object
* was *not* destroyed (meaning the pointer is still valid).
*
* N.B. In a free build the first entry in the thread lock list is unlocked.
*
* 03-04-92 ScottLu      Created.
\***************************************************************************/

#if DBG
PVOID
ThreadUnlock1(
    PTL ptlIn)
#else
PVOID
ThreadUnlock1(
    VOID)
#endif
{
    PHEAD phead;
    PTHREADINFO ptiCurrent;
    PTL ptl;

    ptiCurrent = PtiCurrent();
    ptl = ptiCurrent->ptl;
    UserAssert(ptl != NULL);
     /*
      * Validate the thread lock list.
      */
     ValidateThreadLocks(NULL, ptl, (ULONG_PTR)&ptlIn, TRUE);
    /*
     * Make sure the caller wants to unlock the top lock.
     */
    UserAssert(ptlIn == ptl);
    ptiCurrent->ptl = ptl->next;
    /*
     * If the object address is not NULL, then unlock the object.
     */
    phead = (PHEAD)(ptl->pobj);
    if (phead != NULL) {

        /*
         * Unlock the object.
         */

        phead = (PHEAD)HMUnlockObject(phead);
    }
#if DBG
    {
        /*
         * Remove the corresponding element from gFreeTLList
         */
        ptl->ptl->next = gFreeTLList;
        ptl->ptl->uTLCount += TL_FREED_PATTERN;
        gFreeTLList = ptl->ptl;
    }
#endif
    return (PVOID)phead;
}

/*
 * Switch back to default optimization.
 */
#pragma optimize("", on)

/***************************************************************************\
* CheckLock
*
* This routine only exists in DBG builds - it checks to make sure objects
* are thread locked.
*
* 03-09-92 ScottLu      Created.
\***************************************************************************/

#if DBG
void CheckLock(
    PVOID pobj)
{
    PTHREADINFO ptiCurrent = PtiCurrentShared();
    PTL ptl;

    if (pobj == NULL) {
        return;
    }

    /*
     * Validate all locks first
     */
    UserAssert(ptiCurrent != NULL);
    ValidateThreadLocks(NULL, ptiCurrent->ptl, (ULONG_PTR)&pobj, TRUE);

    for (ptl = ptiCurrent->ptl; ptl != NULL; ptl=ptl->next) {
        if (ptl->pobj == pobj)
            return;
    }

    /*
     * WM_FINALDESTROY messages get sent without thread locking, so if
     * marked for destruction, don't print the message.
     */
    if (HMPheFromObject(pobj)->bFlags & HANDLEF_DESTROY)
        return;

    RIPMSG1(RIP_ERROR, "Object not thread locked! %#p", pobj);
}
#endif


/***************************************************************************\
* HMDestroyUnlockedObject
*
* Destroy an object based on an unlock or cleanup from thread or
* process termination.
*
* The functions called to destroy a particular object can be called
* directly from code as well as the result of an unlock. Destroy
* functions have the following 4 sections.
*
*     (1) Remove the object from a list or other global
*     context. If the destroy function has to leave the
*     critical section (e.g. make an xxx call), it must
*     do so in this step.
*
*     (2) Call HMMarkDestroy, and return if HMMarkDestroy
*     returns FALSE. This is required.
*
*     (3) Destroy resources held by the objects - locks to
*     other objects, alloc'd memory, etc. This is required.
*
*     (4) Free the memory of the object and its handle by calling
*     HMFreeObject. This is required.
*
* Note that if the object is locked when it's destroy function
* is called directly, step (1) will be repeated when the object is
* unlocked. We should probably check for this in the destroy functions,
* which we currently do not do.
*
* Note that we could be destroying this object in a context different
* than the one that created it. This is very important to understand
* since in lots of code the "current thread" is referenced and assumed
* as the creator.
*
* 02-10-92 ScottLu      Created.
\***************************************************************************/

void HMDestroyUnlockedObject(
    PHE phe)
{
    BEGINATOMICCHECK();

    /*
     * Remember that we're destroying this object so we don't try to destroy
     * it again when the lock count goes from != 0 to 0 (especially true
     * for thread locks).
     */
    phe->bFlags |= HANDLEF_INDESTROY;

    /*
     * This'll call the destroy handler for this object type.
     */
    (*gahti[phe->bType].fnDestroy)(phe->phead);

    /*
     * HANDLEF_INDESTROY is supposed to be cleared either by HMMarkObjectDestroy
     *  or by HMFreeObject; the destroy handler was supposed to call at least
     *  the former.
     */
    UserAssert(!(phe->bFlags & HANDLEF_INDESTROY));

    /*
     * If the object wasn't freed, it must be marked as destroyed
     *  and must have a lock count
     */
    UserAssert((phe->bType == TYPE_FREE)
                || ((phe->bFlags & HANDLEF_DESTROY) && (phe->phead->cLockObj > 0)));

    ENDATOMICCHECK();
}


/***************************************************************************\
* HMChangeOwnerThread
*
* Changes the owning thread of an object.
*
* 09-13-93 JimA         Created.
\***************************************************************************/

VOID HMChangeOwnerThread(
    PVOID pobj,
    PTHREADINFO pti)
{
    PHE phe = HMPheFromObject(pobj);
    PTHREADINFO ptiOld = ((PTHROBJHEAD)(pobj))->pti;
    PWND pwnd;
    PPCLS ppcls;
    PPROCESSINFO   ppi;

    UserAssert(HMObjectFlags(pobj) & OCF_THREADOWNED);
    UserAssert(pti != NULL);

    ((PTHREADINFO)phe->pOwner)->ppi->UserHandleCount--;

    ((PTHROBJHEAD)pobj)->pti = phe->pOwner = pti;

    ((PTHREADINFO)phe->pOwner)->ppi->UserHandleCount++;

    DBGValidateHandleQuota();

    /*
     * If this is a window, update the window counts.
     */
    switch (phe->bType) {
    case TYPE_WINDOW:
        /*
         * Desktop thread used to hit this assert in HYDRA
         * because pti == ptiOld
         */
        UserAssert(ptiOld->cWindows > 0 || ptiOld == pti);
        pti->cWindows++;
        ptiOld->cWindows--;

        pwnd = (PWND)pobj;

        /*
         * Make sure thread visible window count is properly updated.
         */
        if (TestWF(pwnd, WFVISIBLE) && FVisCountable(pwnd)) {
            pti->cVisWindows++;
            ptiOld->cVisWindows--;
        }

        /*
         * If the owning process is changing, fix up
         * the window class.
         */
        if (pti->ppi != ptiOld->ppi) {

            ppcls = GetClassPtr(pwnd->pcls->atomClassName, pti->ppi, hModuleWin);

            if (ppcls == NULL) {
                if (pwnd->head.rpdesk)
                    ppi = pwnd->head.rpdesk->rpwinstaParent->pTerm->ptiDesktop->ppi;
                else
                    ppi = PpiCurrent();
                ppcls = GetClassPtr(gpsi->atomSysClass[ICLS_ICONTITLE], ppi, hModuleWin);
            }
            UserAssert(ppcls);

            /*
             * The window is either destroyed or the desktop of the class is
             * the same as the window's desktop
             */
            UserAssert((*ppcls)->rpdeskParent == pwnd->head.rpdesk ||
                       TestWF(pwnd, WFDESTROYED));

            DereferenceClass(pwnd);
            pwnd->pcls = *ppcls;
            pwnd->pcls->cWndReferenceCount++;
        }
        break;

    case TYPE_HOOK:
        /*
         * If this is a global hook, remember this hook's desktop so we'll be
         * able to unlink it later (gptiRit might switch to a different desktop
         * at any time).
         */
        UserAssert(!!(((PHOOK)pobj)->flags & HF_GLOBAL) ^ (((PHOOK)pobj)->ptiHooked != NULL));
        if (((PHOOK)pobj)->flags & HF_GLOBAL) {
            UserAssert(pti == gptiRit);
            LockDesktop(&((PHOOK)pobj)->rpdesk, ptiOld->rpdesk, LDL_HOOK_DESK, 0);
        } else {
            /*
             * This must be a hook on another thread or it was supposed to be
             *  gone by now.
             */
            UserAssert(((PHOOK)pobj)->ptiHooked != ptiOld);
        }
        break;

    default:
        break;
    }
}

/***************************************************************************\
* HMChangeOwnerProcess
*
* Changes the owning process of an object.
*
* 04-15-97 JerrySh      Created.
* 09-23-97 GerardoB     Changed parameters (and name) so HMDestroyUnlockedObject
*                        could use this function (instead of duplicating the code there)
\***************************************************************************/

VOID HMChangeOwnerPheProcess(
    PHE phe,
    PTHREADINFO pti)
{
    PPROCESSINFO ppiOwner = (PPROCESSINFO)(phe->pOwner);
    PVOID pobj = phe->phead;

    UserAssert(HMObjectFlags(pobj) & OCF_PROCESSOWNED);
    UserAssert(pti != NULL);
    /*
     * Dec current owner handle count
     */
    ppiOwner->UserHandleCount--;
    /*
     * hTaskWow
     */
    if ((pti->TIF_flags & TIF_16BIT) && (pti->ptdb)) {
        ((PPROCOBJHEAD)pobj)->hTaskWow = pti->ptdb->hTaskWow;
    } else {
        ((PPROCOBJHEAD)pobj)->hTaskWow = 0;
    }
    /*
     * ppi
     */
    if (gahti[phe->bType].bObjectCreateFlags & OCF_MARKPROCESS) {
        ((PPROCMARKHEAD)pobj)->ppi = pti->ppi;
    }
    /*
     * Set new owner in handle entry
     */
    phe->pOwner = pti->ppi;
    /*
     * Inc new owner handle count
     */
    ((PPROCESSINFO)(phe->pOwner))->UserHandleCount++;
    /*
     * If the handle is a cursor, adjust GDI cursor handle count
     */
    if (phe->bType == TYPE_CURSOR) {
        GreDecQuotaCount((PW32PROCESS)ppiOwner);
        GreIncQuotaCount((PW32PROCESS)phe->pOwner);

        if (((PCURSOR)pobj)->hbmColor) {
            GreDecQuotaCount((PW32PROCESS)ppiOwner);
            GreIncQuotaCount((PW32PROCESS)phe->pOwner);
        }
    }

    DBGValidateHandleQuota();
}

/***************************************************************************\
* DestroyThreadsObjects
*
* Goes through the handle table list and destroy all objects owned by this
* thread, because the thread is going away (either nicely, it faulted, or
* was terminated). It is ok to destroy the objects in any order, because
* object locking will ensure that they get destroyed in the right order.
*
* This routine gets called in the context of the thread that is exiting.
*
* 02-08-92 ScottLu      Created.
\***************************************************************************/

VOID DestroyThreadsObjects()
{
    PTHREADINFO ptiCurrent;
    HANDLEENTRY volatile * (*pphe);
    PHE pheT;
    DWORD i;

    ptiCurrent = PtiCurrent();
    DBGValidateHandleQuota();

    /*
     * Before any window destruction occurs, we need to destroy any dcs
     * in use in the dc cache. When a dc is checked out, it is marked owned,
     * which makes gdi's process cleanup code delete it when a process
     * goes away. We need to similarly destroy the cache entry of any dcs
     * in use by the exiting process.
     */
    DestroyCacheDCEntries(ptiCurrent);

    /*
     * Remove any thread locks that may exist for this thread.
     */
    while (ptiCurrent->ptl != NULL) {

        UserAssert((ULONG_PTR)ptiCurrent->ptl > (ULONG_PTR)&i);
        UserAssert((ULONG_PTR)ptiCurrent->ptl < (ULONG_PTR)KeGetCurrentThread()->StackBase);
        ThreadUnlock(ptiCurrent->ptl);
    }

    /*
     * CleanupPool stuff must happen before handle table clean up (as it always has been).
     * This is because SMWPs can be HM objects and still be locked in ptlPool.
     * If the handle is destroyed first (and it's not locked) we would end up with a
     *  bogus pointer in ptlPool. If ptlPool is cleaned up first, the handle will be freed
     *  or properly preserved if locked.
     */
    CleanupW32ThreadLocks((PW32THREAD)ptiCurrent);

    /*
     * Eventhough HMDestroyUnlockedObject might call xxxDestroyWindow, the
     *  following loop is not supposed to leave the critical section. We must
     *  have called PatchThreadWindows before coming here.
     */
    BEGINATOMICCHECK();

    /*
     * Loop through the table destroying all objects created by the current
     * thread. All objects will get destroyed in their proper order simply
     * because of the object locking.
     */
    pphe = &gSharedInfo.aheList;
    for (i = 0; i <= giheLast; i++) {
        /*
         * This pointer is done this way because it can change when we leave
         * the critical section below.  The above volatile ensures that we
         * always use the most current value
         */
        pheT = (PHE)((*pphe) + i);

        /*
         * Check against free before we look at pti... because pq is stored
         * in the object itself, which won't be there if TYPE_FREE.
         */
        if (pheT->bType == TYPE_FREE)
            continue;

        /*
         * If a menu refererences a window owned by this thread, unlock
         * the window.  This is done to prevent calling xxxDestroyWindow
         * during process cleanup.
         */
        if (gahti[pheT->bType].bObjectCreateFlags & OCF_PROCESSOWNED) {
            if (pheT->bType == TYPE_MENU) {
                PWND pwnd = ((PMENU)pheT->phead)->spwndNotify;

                if (pwnd != NULL && GETPTI(pwnd) == ptiCurrent)
                    Unlock(&((PMENU)pheT->phead)->spwndNotify);
            }
            continue;
        }

        /*
         * Destroy those objects created by this queue.
         */
        if ((PTHREADINFO)pheT->pOwner != ptiCurrent)
            continue;

        UserAssert(gahti[pheT->bType].bObjectCreateFlags & OCF_THREADOWNED);

        /*
         * Make sure this object isn't already marked to be destroyed - we'll
         * do no good if we try to destroy it now since it is locked.
         */
        if (pheT->bFlags & HANDLEF_DESTROY) {
            continue;
        }

        /*
         * Destroy this object.
         */
        HMDestroyUnlockedObject(pheT);
    }

    ENDATOMICCHECK();
    DBGValidateHandleQuota();
}

#if DBG
VOID ShowLocks(
    PHE phe)
{
    PLR plr = phe->plr;
    INT c;

    RIPMSG2(RIP_WARNING | RIP_THERESMORE,
            "Lock records for %s %#p:",
            gahti[phe->bType].szObjectType, phe->phead->h);
    /*
     * We have the handle entry: 'head' and 'he' are both filled in. Dump
     * the lock records. Remember the first record is the last transaction!!
     */
    c = 0;
    while (plr != NULL) {
        char achPrint[80];

        if (plr->ppobj == LOCKRECORD_MARKDESTROY) {
            strcpy(achPrint, "Destroyed with");
        } else if ((int)plr->cLockObj <= 0) {
            strcpy(achPrint, "        Unlock");
        } else {
            /*
             * Find corresponding unlock;
             */
            {
               PLR plrUnlock;
               DWORD cT;
               DWORD cUnlock;

               plrUnlock = phe->plr;
               cT =  0;
               cUnlock = (DWORD)-1;

               while (plrUnlock != plr) {
                   if (plrUnlock->ppobj == plr->ppobj) {
                       if ((int)plrUnlock->cLockObj <= 0) {
                           // a matching unlock found
                           cUnlock = cT;
                       } else {
                           // the unlock #cUnlock matches this lock #cT, thus
                           // #cUnlock is not the unlock we were looking for.
                           cUnlock = (DWORD)-1;
                       }
                   }
                   plrUnlock = plrUnlock->plrNext;
                   cT++;
               }
               if (cUnlock == (DWORD)-1) {
                   /*
                    * Corresponding unlock not found!
                    * This may not mean something is wrong: the structure
                    * containing the pointer to the object may have moved
                    * during a reallocation.  This can cause ppobj at Unlock
                    * time to differ from that recorded at Lock time.
                    * (Warning: moving structures like this may cause a Lock
                    * and an Unlock to be misidentified as a pair, if by a
                    * stroke of incredibly bad luck, the new location of a
                    * pointer to an object is now where an old pointer to the
                    * same object used to be)
                    */
                   sprintf(achPrint, "Unmatched Lock");
               } else {
                   sprintf(achPrint, "lock   #%ld", cUnlock);
               }
            }
        }

        RIPMSG4(RIP_WARNING | RIP_NONAME | RIP_THERESMORE,
                "        %s cLock=%d, pobj at %#p, code at %#p",
                achPrint,
                abs((int)plr->cLockObj),
                plr->ppobj,
                plr->trace[0]);

        plr = plr->plrNext;
        c++;
    }

    RIPMSG1(RIP_WARNING | RIP_NONAME, "        0x%lx records", c);
}
#endif

void FixupCursor(
    PCURSOR      pcur,
    PPROCESSINFO ppi)
{
    int   i;
    PACON pacon = (PACON)pcur;

    if (pcur->head.ppi == NULL) {
        pcur->head.ppi = ppi;
    }

    if (pacon->CURSORF_flags & CURSORF_ACON) {
        for (i = 0; i < pacon->cpcur; i++) {

            UserAssert(pacon->aspcur[i]->CURSORF_flags & CURSORF_ACONFRAME);

            if (pacon->aspcur[i]->head.ppi == NULL) {
                pacon->aspcur[i]->head.ppi = ppi;
            }
        }
    }
}

/***************************************************************************\
* DestroyProcessesObjects
*
* Goes through the handle table list and destroy all objects owned by this
* process, because the process is going away (either nicely, it faulted, or
* was terminated). It is ok to destroy the objects in any order, because
* object locking will ensure that they get destroyed in the right order.
*
* This routine gets called in the context of the last thread in the process.
*
* 08-17-92 JimA         Created.
\***************************************************************************/

VOID DestroyProcessesObjects(
    PPROCESSINFO ppi)
{
    PHE  pheT, pheMax;
    BOOL bGlobal = (ISTS() && ppi->Process == gpepCSRSS);

    /*
     * Loop through the table destroying all objects created by the current
     * process. All objects will get destroyed in their proper order simply
     * because of the object locking.
     */
    DBGValidateHandleQuota();
    pheMax = &gSharedInfo.aheList[giheLast];

    for (pheT = gSharedInfo.aheList; pheT <= pheMax; pheT++) {

        /*
         * Check against free before we look at ppi... because pq is stored
         * in the object itself, which won't be there if TYPE_FREE.
         */
        if (pheT->bType == TYPE_FREE)
            continue;

        /*
         * Destroy those objects created by this queue.
         */
        if (!(gahti[pheT->bType].bObjectCreateFlags & OCF_PROCESSOWNED) ||
                (PPROCESSINFO)pheT->pOwner != ppi)
            continue;

        /*
         * Make sure this object isn't already marked to be destroyed - we'll
         * do no good if we try to destroy it now since it is locked.
         * Change the owner since the process is going away
         */
        if (pheT->bFlags & HANDLEF_DESTROY) {
            HMChangeOwnerPheProcess(pheT, gptiRit);
            continue;
        }

        if (bGlobal) {

            /*
             * For global cursors set the ppi in the pcur
             * to be CSRSS
             */
            if (pheT->bType == TYPE_CURSOR) {
                FixupCursor((PCURSOR)pheT->phead, ppi);
            }
        }

        /*
         * Destroy this object.
         */
        HMDestroyUnlockedObject(pheT);

        /*
         * When the object is freed, the handle type is set to TYPE_FREE.
         * Zombie this object, since its process is going away and it couldn't
         * be freed. This may include unlinking it and resetting the owner.
         */
        if (pheT->bType != TYPE_FREE) {
            if (pheT->bType == TYPE_CURSOR) {
                RIPMSG1(RIP_WARNING, "About to zombie pcur %#p\n",
                        pheT->phead);

                ZombieCursor((PCURSOR)pheT->phead);
            } else {
                HMChangeOwnerPheProcess(pheT, gptiRit);
            }
        }
    } /* for loop */
    DBGValidateHandleQuota();
}

/***************************************************************************\
* MarkThreadsObjects
*
* This is called for the *final* exiting condition when a thread
* may have objects still around... in which case their owner must
* be changed to something "safe" that won't be going away.
*
* 03-02-92 ScottLu      Created.
\***************************************************************************/
void MarkThreadsObjects(
    PTHREADINFO pti)
{
    PHE pheT, pheMax;

    pheMax = &gSharedInfo.aheList[giheLast];
    for (pheT = gSharedInfo.aheList; pheT <= pheMax; pheT++) {
        /*
         * Check against free before we look at pti... because pti is stored
         * in the object itself, which won't be there if TYPE_FREE.
         */
        if (pheT->bType == TYPE_FREE)
            continue;

        /*
         * Change ownership!
         */
        if (gahti[pheT->bType].bObjectCreateFlags & OCF_PROCESSOWNED ||
                (PTHREADINFO)pheT->pOwner != pti)
            continue;

#if DBG
        /*
         * This is just to make sure that RIT or DT never get here
         */
        if (ISTS() && (pti == gptiRit || pti == gTermIO.ptiDesktop)) {
            RIPMSG2(RIP_ERROR, "pti %#p is RIT or DT. phe %#p\n", pti, pheT);
        }
#endif

        HMChangeOwnerThread(pheT->phead, gptiRit);

#if DEBUGTAGS

        if (IsDbgTagEnabled(DBGTAG_TrackLocks)) {
            /*
             * Object still around: print warning message.
             */
            if (pheT->bFlags & HANDLEF_DESTROY) {
                    TAGMSG2(DBGTAG_TrackLocks,
                          "Zombie %s %#p still locked",
                           gahti[pheT->bType].szObjectType, pheT->phead->h);
            } else {
                TAGMSG1(DBGTAG_TrackLocks,
                        "Thread object %#p not destroyed.\n",
                        pheT->phead->h);
            }

            ShowLocks(pheT);
        }

#endif // DEBUGTAGS
    }
}

/***************************************************************************\
* HMRelocateLockRecord
*
* If a pointer to a locked object has been relocated, then this routine will
* adjust the lock record accordingly.  Must be called after the relocation.
*
* The arguments are:
*   ppobjNew - the address of the new pointer
*              MUST already contain the pointer to the object!!
*   cbDelta  - the amount by which this pointer was moved.
*
* Using this routine appropriately will prevent spurious "unmatched lock"
* reports.  See mnchange.c for an example.
*
*
* 03-18-93 IanJa        Created.
\***************************************************************************/

#if DBG

BOOL HMRelocateLockRecord(
    PVOID ppobjNew,
    LONG_PTR cbDelta)
{
    PHE phe;
    PVOID ppobjOld = (PBYTE)ppobjNew - cbDelta;
    PHEAD pobj;
    PLR plr;

    if (ppobjNew == NULL) {
        return FALSE;
    }

    pobj = *(PHEAD *)ppobjNew;

    if (pobj == NULL) {
        return FALSE;
    }

    phe = HMPheFromObject(pobj);
    if (phe->phead != pobj) {
        RIPMSG3(RIP_WARNING,
                "HmRelocateLockRecord(%#p, %lx) - %#p is bad pobj\n",
                ppobjNew, cbDelta, pobj);

        return FALSE;
    }

    plr = phe->plr;

    while (plr != NULL) {
        if (plr->ppobj == ppobjOld) {
            (PBYTE)(plr->ppobj) += cbDelta;
            return TRUE;
        }
        plr = plr->plrNext;
    }

    RIPMSG2(RIP_WARNING,
            "HmRelocateLockRecord(%#p, %lx) - couldn't find lock record\n",
            ppobjNew, cbDelta);

    ShowLocks(phe);
    return FALSE;
}


BOOL HMUnrecordLock(
    PVOID ppobj,
    PVOID pobj)
{
    PHE phe;
    PLR plr;
    PLR *pplr;

    phe = HMPheFromObject(pobj);

    pplr = &(phe->plr);
    plr = *pplr;

    /*
     * Find corresponding lock;
     */
    while (plr != NULL) {
        if (plr->ppobj == ppobj) {
            /*
             * Remove the lock from the list...
             */
            *pplr = plr->plrNext;   // unlink it
            plr->plrNext = NULL;    // make the dead entry safe (?)

            /*
             * ...and free it.
             */
            FreeLockRecord(plr);
            return TRUE;
        }
        pplr = &(plr->plrNext);
        plr = *pplr;
    }

    RIPMSG2(RIP_WARNING, "Could not find lock for ppobj %#p pobj %#p",
            ppobj, pobj);

    return FALSE;
}

#endif // DBG

/***************************************************************************\
* _QueryUserHandles
*
* This function retrieves the USER handle counters for all processes
* specified by their client ID in the paPids array
* Specify QUC_PID_TOTAL to retrieve totals for all processes in the system
*
* Parameters:
*    paPids   - pointer to an array of pids (DWORDS) that we're interested in
*    dwNumInstances - number of DWORDS in paPids
*    pdwResult - will receive TYPES_CTYPESxdwNumInstances counters
*
* returns: none
*
* 07-25-97 mcostea        Created
\***************************************************************************/

VOID _QueryUserHandles(
    LPDWORD  paPids,
    DWORD    dwNumInstances,
    DWORD    dwResult[][TYPE_CTYPES])
{
    PHE         pheCurPos;                 // Current position in the table
    PHE         pheMax;                    // address of last table entry
    DWORD       index;
    DWORD       pid;
    DWORD       dwTotalCounters[TYPE_CTYPES]; // system wide counters

    RtlZeroMemory(dwTotalCounters, TYPE_CTYPES*sizeof(DWORD));
    RtlZeroMemory(dwResult, dwNumInstances*TYPE_CTYPES*sizeof(DWORD));
    /*
     * Walk the handle table and update the counters
     */
    pheMax = &gSharedInfo.aheList[giheLast];
    for(pheCurPos = gSharedInfo.aheList; pheCurPos <= pheMax; pheCurPos++) {

        UserAssert(pheCurPos->bType < TYPE_CTYPES);

        pid = 0;

        if (pheCurPos->pOwner) {

            if (gahti[pheCurPos->bType].bObjectCreateFlags & OCF_PROCESSOWNED) {
                /*
                 * Object is owned by process
                 * some objects may not have an owner
                 */
                    pid = HandleToUlong(((PPROCESSINFO)pheCurPos->pOwner)->Process->UniqueProcessId);
                }
            else if (gahti[pheCurPos->bType].bObjectCreateFlags & OCF_THREADOWNED) {
                /*
                 * Object owned by thread
                 */
                    pid = HandleToUlong(((PTHREADINFO)pheCurPos->pOwner)->pEThread->ThreadsProcess->UniqueProcessId);
                }
        }
        /*
         * search to see if we are interested in this process
         * unowned handles are reported for the "System" process whose pid is 0
         */
        for (index = 0; index < dwNumInstances; index++) {

            if (paPids[index] == pid) {
                dwResult[index][pheCurPos->bType]++;
            }
        }
        /*
         *  update the totals
         */
         dwTotalCounters[pheCurPos->bType]++;
    }

    /*
     * search to see if we are interested in the totals
     */
    for (index = 0; index < dwNumInstances; index++) {

        if (paPids[index] == QUC_PID_TOTAL) {

            RtlMoveMemory(dwResult[index], dwTotalCounters, sizeof(dwTotalCounters));
        }
    }

}

/***************************************************************************\
* HMCleanupGrantedHandle
*
* This function is called to cleanup this handle from pW32Job->pgh arrays.
* It walks the job list to find jobs that have the handle granted.
*
* HISTORY:
* 22 Jul 97      CLupu            Created
\***************************************************************************/
void HMCleanupGrantedHandle(
    HANDLE h)
{
    PW32JOB pW32Job;

    pW32Job = gpJobsList;

    while (pW32Job != NULL) {
        PULONG_PTR pgh;
        DWORD  dw;

        pgh = pW32Job->pgh;

        /*
         * search for the handle in the array.
         */
        for (dw = 0; dw < pW32Job->ughCrt; dw++) {
            if (*(pgh + dw) == (ULONG_PTR)h) {

                /*
                 * found the handle granted to this process
                 */
                RtlMoveMemory(pgh + dw,
                              pgh + dw + 1,
                              (pW32Job->ughCrt - dw - 1) * sizeof(*pgh));

                (pW32Job->ughCrt)--;

                /*
                 * we should shrink the array also
                 */

                break;
            }
        }

        pW32Job = pW32Job->pNext;
    }
}
