/****************************** Module Header ******************************\
* Module Name: init.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains all the init code for the USERSRV.DLL.  When
* the DLL is dynlinked by the SERVER EXE its initialization procedure
* (xxxUserServerDllInitialize) is called by the loader.
*
* History:
* 18-Sep-1990 DarrinM   Created.
\***************************************************************************/

#define OEMRESOURCE 1

#include "precomp.h"
#pragma hdrstop

#if DBG
LIST_ENTRY gDesktopList;
#endif

//
// External references
//
extern PVOID *apObjects;
extern PKWAIT_BLOCK   gWaitBlockArray;
extern PVOID UserAtomTableHandle;
extern PKTIMER gptmrWD;

extern UNICODE_STRING* gpastrSetupExe;
extern WCHAR* glpSetupPrograms;

extern PHANDLEPAGE gpHandlePages;

extern PBWL pbwlCache;

//
// Forward references
//
BOOL
Win32kNtUserCleanup();

#if DBG
void InitGlobalThreadLockArray(DWORD dwIndex);
#endif // DBG

/*
 * Local Constants.
 */
#define GRAY_STRLEN         40

/*
 * Globals local to this file only.
 */
BOOL bPermanentFontsLoaded = FALSE;
int  LastFontLoaded = -1;

/*
 * Globals shared with W32
 */
ULONG W32ProcessSize = sizeof(PROCESSINFO);
ULONG W32ProcessTag = TAG_PROCESSINFO;
ULONG W32ThreadSize = sizeof(THREADINFO);
ULONG W32ThreadTag = TAG_THREADINFO;
PFAST_MUTEX gpW32FastMutex;

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath);

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, LW_LoadSomeStrings)

NTSTATUS Win32UserInitialize(VOID);

#if defined(_X86_)
ULONG Win32UserProbeAddress;
#endif

void FreeSMS(PSMS psms);
VOID FreeImeHotKeys(VOID);

/***************************************************************************\
* Win32kNtUserCleanup
*
* History:
* 5-Jan-1997 CLupu   Created.
\***************************************************************************/
extern PPAGED_LOOKASIDE_LIST SMSLookaside;
extern PPAGED_LOOKASIDE_LIST QEntryLookaside;

// Max time is 10 minutes, and the count is 10 min * 60 sec * 4 for 250 mili seconds
#define MAX_TIME_OUT    (10*60*4)

BOOL Win32kNtUserCleanup(
    VOID)
{
    int i;

    TRACE_HYDAPI(("Win32kNtUserCleanup: Cleanup Resources\n"));

    EnterCrit();

    DbgDumpTrackedDesktops(TRUE);

    /*
     * Anything in this list must be cleaned up when threads go away.
     */
    UserAssert(gpbwlList == NULL);

    UserAssert(gpWinEventHooks == NULL);

    UserAssert(gpScancodeMap == NULL);

    /*
     * Free IME hotkeys
     */
    FreeImeHotKeys();

    /*
     * Free the wallpaper name string
     */
    if (gpszWall != NULL) {
        UserFreePool(gpszWall);
        gpszWall = NULL;
    }
    /*
     * Free the hung redraw stuff
     */
    if (gpvwplHungRedraw != NULL) {
        UserFreePool(gpvwplHungRedraw);
        gpvwplHungRedraw = NULL;
    }

    /*
     * Free the arrary of setup app names
     */
    if (gpastrSetupExe) {
        UserFreePool(gpastrSetupExe);
        gpastrSetupExe = NULL;
    }

    if (glpSetupPrograms) {
        UserFreePool(glpSetupPrograms);
        glpSetupPrograms = NULL;
    }

    /*
     * Free the cached window list
     */
    if (pbwlCache != NULL) {
        UserFreePool(pbwlCache);
        pbwlCache = NULL;
    }

    /*
     * Free outstanding timers
     */
    while (gptmrFirst != NULL) {
        FreeTimer(gptmrFirst);
    }

    if (gptmrWD) {
        KeCancelTimer(gptmrWD);
        UserFreePool(gptmrWD);
        gptmrWD = NULL;
    }

    if (gptmrMaster) {
        KeCancelTimer(gptmrMaster);
        UserFreePool(gptmrMaster);
        gptmrMaster = NULL;
    }

    /*
     * Cleanup mouse & kbd change events
     */
    for (i = 0; i <= DEVICE_TYPE_MAX; i++) {
        UserAssert(gptiRit == NULL);
        if (aDeviceTemplate[i].pkeHidChange) {
            FreeKernelEvent(&aDeviceTemplate[i].pkeHidChange);
        }
    }

    EnterDeviceInfoListCrit();
    while (gpDeviceInfoList) {
        /*
         * Assert that there is no outstanding read or PnP thread waiting
         * in RequestDeviceChanges() for this device.
         * Clear these flags anyway, to force the free.
         */
        UserAssert((gpDeviceInfoList->bFlags & GDIF_READING) == 0);
        UserAssert((gpDeviceInfoList->usActions & GDIAF_PNPWAITING) == 0);
        gpDeviceInfoList->bFlags &= ~GDIF_READING;
        gpDeviceInfoList->usActions &= ~GDIAF_PNPWAITING;
        FreeDeviceInfo(gpDeviceInfoList);
    }
    LeaveDeviceInfoListCrit();

    /*
     * Cleanup object references
     */
    if (gThinwireFileObject)
        ObDereferenceObject(gThinwireFileObject);

    if (gVideoFileObject)
        ObDereferenceObject(gVideoFileObject);

    if (gpRemoteBeepDevice)
        ObDereferenceObject(gpRemoteBeepDevice);

    /*
     * Cleanup our resources. There should be no threads in here
     * when we get called.
     */
    if (gpresMouseEventQueue) {
        ExDeleteResource(gpresMouseEventQueue);
        ExFreePool(gpresMouseEventQueue);
        gpresMouseEventQueue = NULL;
    }

    if (gpresDeviceInfoList) {
        ExDeleteResource(gpresDeviceInfoList);
        ExFreePool(gpresDeviceInfoList);
        gpresDeviceInfoList = NULL;
    }

    if (gpkeMouseData != NULL) {
        FreeKernelEvent(&gpkeMouseData);
    }

    if (apObjects) {
        UserFreePool(apObjects);
        apObjects = NULL;
    }

    if (gWaitBlockArray) {
        UserFreePool(gWaitBlockArray);
        gWaitBlockArray = NULL;
    }

    if (gpEventDiconnectDesktop != NULL) {
        FreeKernelEvent(&gpEventDiconnectDesktop);
    }

    if (gpevtDesktopDestroyed != NULL) {
        FreeKernelEvent(&gpevtDesktopDestroyed);
    }

    if (UserAtomTableHandle != NULL) {
        RtlDestroyAtomTable(UserAtomTableHandle);
        UserAtomTableHandle = NULL;
    }

    /*
     * cleanup the SMS lookaside buffer
     */
    {
        PSMS psmsNext;

        while (gpsmsList != NULL) {
            psmsNext = gpsmsList->psmsNext;
            UserAssert(gpsmsList->spwnd == NULL);
            FreeSMS(gpsmsList);
            gpsmsList = psmsNext;
        }

        if (SMSLookaside != NULL) {
            ExDeletePagedLookasideList(SMSLookaside);
            UserFreePool(SMSLookaside);
            SMSLookaside = NULL;
        }
    }

    /*
     * Let go of the attached queue for hard error handling.
     * Do this before we free the Qlookaside !
     */
    if (gHardErrorHandler.pqAttach != NULL) {

        UserAssert(gHardErrorHandler.pqAttach > 0);
        UserAssert(gHardErrorHandler.pqAttach->QF_flags & QF_INDESTROY);

        FreeQueue(gHardErrorHandler.pqAttach);
        gHardErrorHandler.pqAttach = NULL;
    }

    /*
     * Free the cached array of queues
     */
    FreeCachedQueues();

    /*
     * cleanup the QEntry lookaside buffer
     */
    if (QEntryLookaside != NULL) {
        ExDeletePagedLookasideList(QEntryLookaside);
        UserFreePool(QEntryLookaside);
        QEntryLookaside = NULL;
    }

    /*
     * Unlock the keyboard layouts
     */
    if (gspklBaseLayout != NULL) {

        PKL pkl;
        PKL pklNext;

        pkl = gspklBaseLayout->pklNext;

        while (pkl->pklNext != pkl) {
            pklNext = pkl->pklNext;

            DestroyKL(pkl);

            pkl = pklNext;
        }

        UserAssert(pkl == gspklBaseLayout);

        if (!HMIsMarkDestroy(gspklBaseLayout)) {
            HMMarkObjectDestroy(gspklBaseLayout);
        }

        HYDRA_HINT(HH_KBDLYOUTGLOBALCLEANUP);

        if (Unlock(&gspklBaseLayout)) {
            DestroyKL(pkl);
        }
    }

    UserAssert(gpkfList == NULL);

    {
        PWOWTHREADINFO pwti;

        /*
         * Cleanup gpwtiFirst list. This list is supposed to be empty
         * at this point. In one case during stress we hit the case where
         * it was not empty. The assert is to catch that case in
         * checked builds.
         */

        while (gpwtiFirst != NULL) {
            pwti = gpwtiFirst;
            gpwtiFirst = pwti->pwtiNext;
            UserFreePool(pwti);
        }
    }

    /*
     * Cleanup cached SMWP array
     */
    if (gSMWP.acvr != NULL) {
        UserFreePool(gSMWP.acvr);
    }

    /*
     * Free gpsdInitWinSta. This is != NULL only if the session didn't
     * make it to UserInitialize.
     */
    if (gpsdInitWinSta != NULL) {
        UserFreePool(gpsdInitWinSta);
        gpsdInitWinSta = NULL;
    }

    if (gpHandleFlagsMutex != NULL) {
        ExFreePool(gpHandleFlagsMutex);
        gpHandleFlagsMutex = NULL;
    }

    /*
     * Delete the power request structures.
     */
    DeletePowerRequestList();

    LeaveCrit();

    if (gpresUser != NULL) {
        ExDeleteResource(gpresUser);
        ExFreePool(gpresUser);
        gpresUser = NULL;
    }
#if DBG
    /*
     * Cleanup the global thread lock structures
     */
    for (i = 0; i < gcThreadLocksArraysAllocated; i++) {
        UserFreePool(gpaThreadLocksArrays[i]);
        gpaThreadLocksArrays[i] = NULL;
    }
#endif // DBG

    return TRUE;
}

#if DBG

/***************************************************************************\
* TrackAddDesktop
*
* Track desktops for cleanup purposes
*
* History:
* 04-Dec-1997 clupu     Created.
\***************************************************************************/
VOID TrackAddDesktop(
    PVOID pDesktop)
{
    PLIST_ENTRY Entry;
    PVOID       Atom;

    TRACE_HYDAPI(("TrackAddDesktop %#p\n", pDesktop));

    Atom = (PVOID)UserAllocPool(sizeof(PVOID) + sizeof(LIST_ENTRY),
                                TAG_TRACKDESKTOP);
    if (Atom) {

        *(PVOID*)Atom = pDesktop;

        Entry = (PLIST_ENTRY)(((PCHAR)Atom) + sizeof(PVOID));

        InsertTailList(&gDesktopList, Entry);
    }
}

/***************************************************************************\
* TrackRemoveDesktop
*
* Track desktops for cleanup purposes
*
* History:
* 04-Dec-1997 clupu     Created.
\***************************************************************************/
VOID TrackRemoveDesktop(
    PVOID pDesktop)
{
    PLIST_ENTRY NextEntry;
    PVOID       Atom;

    TRACE_HYDAPI(("TrackRemoveDesktop %#p\n", pDesktop));

    NextEntry = gDesktopList.Flink;

    while (NextEntry != &gDesktopList) {

        Atom = (PVOID)(((PCHAR)NextEntry) - sizeof(PVOID));

        if (pDesktop == *(PVOID*)Atom) {

            RemoveEntryList(NextEntry);

            UserFreePool(Atom);

            break;
        }

        NextEntry = NextEntry->Flink;
    }
}

/***************************************************************************\
* DumpTrackedDesktops
*
* Dumps the tracked desktops
*
* History:
* 04-Dec-1997 clupu     Created.
\***************************************************************************/
VOID DumpTrackedDesktops(
    BOOL bBreak)
{
    PLIST_ENTRY NextEntry;
    PVOID       pdesk;
    PVOID       Atom;
    int         nAlive = 0;

    TRACE_HYDAPI(("DumpTrackedDesktops\n"));

    NextEntry = gDesktopList.Flink;

    while (NextEntry != &gDesktopList) {

        Atom = (PVOID)(((PCHAR)NextEntry) - sizeof(PVOID));

        pdesk = *(PVOID*)Atom;

        KdPrint(("pdesk %#p\n", pdesk));

        /*
         * Restart at the begining of the list since our
         * entry got deleted
         */
        NextEntry = NextEntry->Flink;

        nAlive++;
    }
    if (bBreak && nAlive > 0) {
        RIPMSG0(RIP_ERROR, "Desktop objects still around\n");
    }
}

#endif // DBG

VOID DestroyRegion(
    HRGN* prgn)
{
    if (*prgn != NULL) {
        GreSetRegionOwner(*prgn, OBJECT_OWNER_CURRENT);
        GreDeleteObject(*prgn);
        *prgn = NULL;
    }
}

VOID DestroyBrush(
    HBRUSH* pbr)
{
    if (*pbr != NULL) {
        //GreSetBrushOwner(*pbr, OBJECT_OWNER_CURRENT);
        GreDeleteObject(*pbr);
        *pbr = NULL;
    }
}

VOID DestroyBitmap(
    HBITMAP* pbm)
{
    if (*pbm != NULL) {
        GreSetBitmapOwner(*pbm, OBJECT_OWNER_CURRENT);
        GreDeleteObject(*pbm);
        *pbm = NULL;
    }
}

VOID DestroyDC(
    HDC* phdc)
{
    if (*phdc != NULL) {
        GreSetDCOwner(*phdc, OBJECT_OWNER_CURRENT);
        GreDeleteDC(*phdc);
        *phdc = NULL;
    }
}

VOID DestroyFont(
    HFONT* pfnt)
{
    if (*pfnt != NULL) {
        GreDeleteObject(*pfnt);
        *pfnt = NULL;
    }
}

/***************************************************************************\
* CleanupGDI
*
* Cleanup all the GDI global objects used in USERK
*
* History:
* 29-Jan-1998 clupu     Created.
\***************************************************************************/
VOID CleanupGDI(
    VOID)
{
    int i;

    /*
     * Free gpDispInfo stuff
     */
    DestroyDC(&gpDispInfo->hdcScreen);
    DestroyDC(&gpDispInfo->hdcBits);
    DestroyDC(&gpDispInfo->hdcGray);
    DestroyDC(&ghdcMem);
    DestroyDC(&ghdcMem2);
    DestroyDC(&gfade.hdc);

    /*
     * Free the cache DC stuff before the GRE cleanup.
     * Also notice that we call DelayedDestroyCacheDC which
     * we usualy call from DestroyProcessInfo. We do it
     * here because this is the last WIN32 thread.
     */
    DestroyCacheDCEntries(PtiCurrent());
    DestroyCacheDCEntries(NULL);
    DelayedDestroyCacheDC();

    UserAssert(gpDispInfo->pdceFirst == NULL);

    /*
     * Free bitmaps
     */
    DestroyBitmap(&gpDispInfo->hbmGray);
    DestroyBitmap(&ghbmBits);
    DestroyBitmap(&ghbmCaption);

    /*
     * Cleanup brushes
     */
    DestroyBrush(&ghbrHungApp);
    DestroyBrush(&gpsi->hbrGray);
    DestroyBrush(&ghbrWhite);
    DestroyBrush(&ghbrBlack);

    for (i = 0; i < COLOR_MAX; i++) {
        DestroyBrush(&(SYSHBRUSH(i)));
    }

    /*
     * Cleanup regions
     */
    DestroyRegion(&gpDispInfo->hrgnScreen);
    DestroyRegion(&ghrgnInvalidSum);
    DestroyRegion(&ghrgnVisNew);
    DestroyRegion(&ghrgnSWP1);
    DestroyRegion(&ghrgnValid);
    DestroyRegion(&ghrgnValidSum);
    DestroyRegion(&ghrgnInvalid);
    DestroyRegion(&ghrgnInv0);
    DestroyRegion(&ghrgnInv1);
    DestroyRegion(&ghrgnInv2);
    DestroyRegion(&ghrgnGDC);
    DestroyRegion(&ghrgnSCR);
    DestroyRegion(&ghrgnSPB1);
    DestroyRegion(&ghrgnSPB2);
    DestroyRegion(&ghrgnSW);
    DestroyRegion(&ghrgnScrl1);
    DestroyRegion(&ghrgnScrl2);
    DestroyRegion(&ghrgnScrlVis);
    DestroyRegion(&ghrgnScrlSrc);
    DestroyRegion(&ghrgnScrlDst);
    DestroyRegion(&ghrgnScrlValid);

    /*
     * Cleanup fonts
     */
    DestroyFont(&ghSmCaptionFont);
    DestroyFont(&ghMenuFont);
    DestroyFont(&ghMenuFontDef);
    DestroyFont(&ghStatusFont);
    DestroyFont(&ghIconFont);
    DestroyFont(&ghFontSys);

    /*
     * wallpaper stuff.
     */
    if (ghpalWallpaper != NULL) {
        GreSetPaletteOwner(ghpalWallpaper, OBJECT_OWNER_CURRENT);
        GreDeleteObject(ghpalWallpaper);
        ghpalWallpaper = NULL;
    }
    DestroyBitmap(&ghbmWallpaper);

    /*
     * Unload the video driver
     */
    if (gpDispInfo->pmdev) {
        DrvDestroyMDEV(gpDispInfo->pmdev);
        GreFreePool(gpDispInfo->pmdev);
        gpDispInfo->pmdev = NULL;
        gpDispInfo->hDev = NULL;
    }

    /*
     * Free the monitor stuff
     */
    {
        PMONITOR pMonitor;
        PMONITOR pMonitorNext;

        pMonitor = gpDispInfo->pMonitorFirst;

        while (pMonitor != NULL) {
            pMonitorNext = pMonitor->pMonitorNext;
            DestroyMonitor(pMonitor);
            pMonitor = pMonitorNext;
        }

        UserAssert(gpDispInfo->pMonitorFirst == NULL);

        if (gpMonitorCached != NULL) {
            DestroyMonitor(gpMonitorCached);
        }
    }
}


/***************************************************************************\
*   DestroyHandleTableObjects
*
*   Destroy any object still in the handle table.
*
\***************************************************************************/
VOID DestroyHandleTableObjects(VOID)
{
    HANDLEENTRY volatile * (*pphe);
    PHE         pheT;
    DWORD       i;

    /*
     * Make sure the handle table was created !
     */
    if (gSharedInfo.aheList == NULL) {
        return;
    }

    /*
     * Loop through the table destroying all remaining objects.
     */
    pphe = &gSharedInfo.aheList;

    for (i = 0; i <= giheLast; i++) {

        pheT = (PHE)((*pphe) + i);

        if (pheT->bType == TYPE_FREE)
            continue;

        UserAssert(!(gahti[pheT->bType].bObjectCreateFlags & OCF_PROCESSOWNED) &&
                   !(gahti[pheT->bType].bObjectCreateFlags & OCF_THREADOWNED));

        UserAssert(!(pheT->bFlags & HANDLEF_DESTROY));

        /*
         * Destroy the object.
         */
        if (pheT->phead->cLockObj > 0) {

            RIPMSG1(RIP_ERROR, "pheT %#p still locked !", pheT);

            /*
             * We're going to die, why bothered by the lock count?
             * We're forcing the lockcount to 0, and call the destroy routine.
             */
            pheT->phead->cLockObj = 0;
        }
        HMDestroyUnlockedObject(pheT);
        UserAssert(pheT->bType == TYPE_FREE);
    }
}

/***************************************************************************\
* Win32KDriverUnload
*
*     Exit point for win32k.sys
*
\***************************************************************************/
#ifdef TRACE_MAP_VIEWS
    extern PWin32Section gpSections;
#endif // TRACE_MAP_VIEWS;

#if DBG
ULONG DriverUnloadExceptionHandler(PEXCEPTION_POINTERS pexi)
{
    KdPrint(("\nEXCEPTION IN Win32kDriverUnload !!!\n\n"
             "Exception:  %#x\n"
             "at address: %#p\n"
             "flags:      %#x\n\n"
             "!exr %#p\n"
             "!cxr %#p\n\n",
             pexi->ExceptionRecord->ExceptionCode,
             CONTEXT_TO_PROGRAM_COUNTER(pexi->ContextRecord),
             pexi->ExceptionRecord->ExceptionFlags,
             pexi->ExceptionRecord,
             pexi->ContextRecord));

    return EXCEPTION_EXECUTE_HANDLER;
}
#endif // DBG

VOID Win32KDriverUnload(
    IN PDRIVER_OBJECT DriverObject)
{
    TRACE_HYDAPI(("Win32KDriverUnload\n"));

#if DBG
    /*
     * Have a try/except around the driver unload code to catch
     * bugs. We need to do this because NtSetSystemInformation which
     * smss calls has a try/except when calling the driver entry and
     * the driver unload routines which does nothing but returns the
     * status code to the caller and doesn't break in the debugger!
     * So w/o this try/except here we wouldn't even know we hit
     * an AV !!!
     */
    try {
#endif // DBG

    HYDRA_HINT(HH_DRIVERUNLOAD);

    /*
     * Cleanup all resources in GRE
     */
    MultiUserNtGreCleanup();

    HYDRA_HINT(HH_GRECLEANUP);

    /*
     * BUG 305965. There might be cases when we end up with DCEs still
     * in the list. Go ahead and clean it up here.
     */
    if (gpDispInfo != NULL && gpDispInfo->pdceFirst != NULL) {

        PDCE pdce;
        PDCE pdceNext;

        RIPMSG0(RIP_ERROR, "Win32KDriverUnload: the DCE list is not empty !");

        pdce = gpDispInfo->pdceFirst;

        while (pdce != NULL) {
            pdceNext = pdce->pdceNext;

            UserFreePool(pdce);

            pdce = pdceNext;
        }
        gpDispInfo->pdceFirst = NULL;
    }

    /*
     * Cleanup all resources in ntuser
     */
    Win32kNtUserCleanup();

    /*
     * Cleanup the handle table for any object that is neither process
     * owned nor thread owned
     */
    DestroyHandleTableObjects();


    HYDRA_HINT(HH_USERKCLEANUP);

#if DBG
    HMCleanUpHandleTable();
#endif

    /*
     * Free the handle page array
     */

    if (gpHandlePages != NULL) {
        UserFreePool(gpHandlePages);
        gpHandlePages = NULL;
    }

    if (CsrApiPort != NULL) {
        ObDereferenceObject(CsrApiPort);
        CsrApiPort = NULL;
    }

    /*
     * destroy the shared memory
     */
    if (ghSectionShared != NULL) {

        NTSTATUS Status;

        /*
         * Set gpsi to NULL
         */
        gpsi = NULL;

        if (gpvSharedBase != NULL) {
            Win32HeapDestroy(gpvSharedAlloc);
            Status = Win32UnmapViewInSessionSpace(gpvSharedBase);
            UserAssert(NT_SUCCESS(Status));
        }
        Win32DestroySection(ghSectionShared);
    }

    CleanupWin32HeapStubs();

#ifdef TRACE_MAP_VIEWS
#if DBG
    UserAssert(gpSections == NULL);
#else
    if (gpSections != NULL) {
        DbgBreakPoint();
    }
#endif // DBG
#endif // TRACE_MAP_VIEWS

    /*
     * Cleanup all the user pool allocations by hand
     */
    CleanupPoolAllocations();

    CleanUpPoolLimitations();
    CleanUpSections();

    /*
     * Cleanup W32 structures.
     */
    if (gpW32FastMutex != NULL) {
        ExFreePool(gpW32FastMutex);
        gpW32FastMutex = NULL;
    }

#if DBG
    } except (DriverUnloadExceptionHandler(GetExceptionInformation())) {
        DbgBreakPoint();
    }
#endif // DBG

    return;
    UNREFERENCED_PARAMETER(DriverObject);
}

/***************************************************************************\
* DriverEntry
*
* Entry point needed to initialize win32k.sys
*
\***************************************************************************/
NTSTATUS DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath)
{
    PVOID             countTable;
    NTSTATUS          Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES obja;
    UNICODE_STRING    strName;
    HANDLE            hEventFirstSession;

    HYDRA_HINT(HH_DRIVERENTRY);

    /*
     * Find out if this is a remote session
     */
    RtlInitUnicodeString(&strName, L"\\UniqueSessionIdEvent");

    InitializeObjectAttributes(&obja,
                               &strName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwCreateEvent(&hEventFirstSession,
                           EVENT_ALL_ACCESS,
                           &obja,
                           SynchronizationEvent,
                           FALSE);

    if (Status == STATUS_OBJECT_NAME_EXISTS ||
        Status == STATUS_OBJECT_NAME_COLLISION) {

        gbRemoteSession = TRUE;
    } else {
        UserAssert(NT_SUCCESS(Status));
        gbRemoteSession = FALSE;
    }

    /*
     * Set the unload address
     */
    DriverObject->DriverUnload = Win32KDriverUnload;

    /*
     * Initialize data used for the timers.  We want to do this really early,
     * before any Win32 Timer will be created.  We need to be very careful to
     * not do anything that will need Win32 initialized yet.
     */

    gcmsLastTimer = NtGetTickCount();

    /*
     * Initialize the Win32 structures. We need to do this before we create
     * any threads.
     */
    gpW32FastMutex = ExAllocatePoolWithTag(NonPagedPool,
                                           sizeof(FAST_MUTEX),
                                           TAG_SYSTEM);
    if (gpW32FastMutex == NULL) {
        Status = STATUS_NO_MEMORY;
        goto Failure;
    }
    ExInitializeFastMutex(gpW32FastMutex);

    if (gbRemoteSession)
        goto executeAlsoForRemote;

#if !DBG
    countTable = NULL;
#else

    /*
     * Allocate and zero the system service count table.
     * Do not use UserAllocPool for this one !
     */
    countTable = (PULONG)ExAllocatePoolWithTag(NonPagedPool,
                                               W32pServiceLimit * sizeof(ULONG),
                                               'llac');
    if (countTable == NULL ) {
        Status = STATUS_NO_MEMORY;
        goto Failure;
    }

    RtlZeroMemory(countTable, W32pServiceLimit * sizeof(ULONG));
#endif  // #else DBG

    /*
     * We only establish the system entry table once for the
     * whole system, even though WIN32K.SYS is instanced on a winstation
     * basis. This is because the VM changes assure that all loads of
     * WIN32K.SYS are at the exact same address, even if a fixup had
     * to occur.
     */
    UserVerify(KeAddSystemServiceTable(W32pServiceTable,
                                       countTable,
                                       W32pServiceLimit,
                                       W32pArgumentTable,
                                       W32_SERVICE_NUMBER));

    /*
     * Initialize the critical section before establishing the callouts so
     *  we can assume that it's always valid
     */
    if (!InitCreateUserCrit()) {
        Status = STATUS_NO_MEMORY;
        goto Failure;
    }

    PsEstablishWin32Callouts(W32pProcessCallout,
                             W32pThreadCallout,
                             UserGlobalAtomTableCallout,
                             UserPowerEventCallout,
                             UserPowerStateCallout,
                             UserJobCallout,
                             (PVOID)NtGdiFlushUserBatch);

executeAlsoForRemote:

    if (gbRemoteSession) {

        if (!InitCreateUserCrit()) {
            Status = STATUS_NO_MEMORY;
            goto Failure;
        }
    }

    InitSectionTrace();

    InitWin32HeapStubs();

    /*
     * Initialize pool limitation array.
     */
    InitPoolLimitations();

#if defined(_X86_)
    /*
     * Keep our own copy of this to avoid double indirections on probing
     */
    Win32UserProbeAddress = *MmUserProbeAddress;
#endif

    if ((hModuleWin = MmPageEntireDriver(DriverEntry)) == NULL) {
        RIPMSG0(RIP_WARNING, "MmPageEntireDriver failed");
        Status = STATUS_NO_MEMORY;
        goto Failure;
    }

#if DBG
    /*
     * Initialize the desktop tracking list
     */
    InitializeListHead(&gDesktopList);
    /*
     * Initialize the gpaThreadLocksArray mechanism
     */
    gFreeTLList = gpaThreadLocksArrays[gcThreadLocksArraysAllocated] =
    UserAllocPoolZInit(sizeof(TL)*MAX_THREAD_LOCKS, TAG_GLOBALTHREADLOCK);
    if (gFreeTLList == NULL) {
        Status = STATUS_NO_MEMORY;
        goto Failure;
    }
    InitGlobalThreadLockArray(0);
    gcThreadLocksArraysAllocated = 1;
#endif // DBG

    if (!InitializeGre()) {
        RIPMSG0(RIP_WARNING, "InitializeGre failed");
        Status = STATUS_NO_MEMORY;
        goto Failure;
    }

    Status = Win32UserInitialize();

    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING, "Win32UserInitialize failed with Status %x",
                Status);
        goto Failure;
    }

    return STATUS_SUCCESS;

Failure:

    RIPMSG1(RIP_WARNING, "Initialization of WIN32K.SYS failed with Status = 0x%x!!!",
            Status);

    Win32KDriverUnload(NULL);
    return Status;

    UNREFERENCED_PARAMETER(RegistryPath);
}

/***************************************************************************\
* xxxAddFontResourceW
*
*
* History:
\***************************************************************************/

int xxxAddFontResourceW(
    LPWSTR lpFile,
    FLONG  flags,
    DESIGNVECTOR *pdv)
{
    UNICODE_STRING strFile;

    RtlInitUnicodeString(&strFile, lpFile);

    /*
     * Callbacks leave the critsec, so make sure that we're in it.
     */

    return xxxClientAddFontResourceW(&strFile, flags, pdv);
}

/***************************************************************************\
* LW_DriversInit
*
*
* History:
\***************************************************************************/

VOID LW_DriversInit(VOID)
{
    /*
     * Initialize the keyboard typematic rate.
     */
    SetKeyboardRate((UINT)gnKeyboardSpeed);

    /*
     * Adjust VK modification table if not default (type 4) kbd.
     */
    if (gKeyboardInfo.KeyboardIdentifier.Type == 3)
        gapulCvt_VK = gapulCvt_VK_84;

    /*
     * Adjust VK modification table for IBM 5576 002/003 keyboard.
     */
    if (JAPANESE_KEYBOARD(gKeyboardInfo.KeyboardIdentifier) &&
        (gKeyboardInfo.KeyboardIdentifier.Subtype == 3))
        gapulCvt_VK = gapulCvt_VK_IBM02;

    /*
     * Initialize NLS keyboard globals.
     */
    NlsKbdInitializePerSystem();
}

/***************************************************************************\
* LoadCPUserPreferences
*
* 06/07/96  GerardoB  Created
\***************************************************************************/
BOOL LoadCPUserPreferences(PUNICODE_STRING pProfileUserName)
{
    DWORD pdwValue [SPI_BOOLMASKDWORDSIZE];
    DWORD dw;
    PPROFILEVALUEINFO ppvi = gpviCPUserPreferences;

    UserAssert(1 + SPI_DWORDRANGECOUNT == sizeof(gpviCPUserPreferences) / sizeof(*gpviCPUserPreferences));

    /*
     * The first value in gpviCPUserPreferences corresponds to the bit mask
     */
    dw =  FastGetProfileValue(pProfileUserName,
            ppvi->uSection,
            ppvi->pwszKeyName,
            NULL,
            (LPBYTE)pdwValue,
            sizeof(*pdwValue)
            );

    /*
     * Copy only the amount of data read and no more than what we expect
     */
    if (dw != 0) {
        if (dw > sizeof(gpdwCPUserPreferencesMask)) {
            dw = sizeof(gpdwCPUserPreferencesMask);
        }
        RtlCopyMemory(gpdwCPUserPreferencesMask, pdwValue, dw);
    }

    ppvi++;

    /*
     * DWORD values
     */
    for (dw = 1; dw < 1 + SPI_DWORDRANGECOUNT; dw++, ppvi++) {
        if (FastGetProfileValue(pProfileUserName,
                ppvi->uSection,
                ppvi->pwszKeyName,
                NULL,
                (LPBYTE)pdwValue,
                sizeof(DWORD)
                )) {

            ppvi->dwValue = *pdwValue;
        }
    }

    if (gbRemoteSession) {

        /*
         * Default is w/o UIEFFECTS for remote connections
         */
        gpdwCPUserPreferencesMask[0] &= ~0x80000000;
    }

    /*
     * Propagate gpsi flags
     */
    PropagetUPBOOLTogpsi(COMBOBOXANIMATION);
    PropagetUPBOOLTogpsi(LISTBOXSMOOTHSCROLLING);
    PropagetUPBOOLTogpsi(KEYBOARDCUES);
    gpsi->bKeyboardPref = TEST_BOOL_ACCF(ACCF_KEYBOARDPREF);

    gpsi->uCaretWidth = UP(CARETWIDTH);

    PropagetUPBOOLTogpsi(UIEFFECTS);

    EnforceColorDependentSettings();

    return TRUE;
}

/***************************************************************************\
* LW_LoadProfileInitData
*
* Only stuff that gets initialized at boot should go here. Per user settings
* should be initialized in xxxUpdatePerUserSystemParameters.
*
* History:
\***************************************************************************/
VOID LW_LoadProfileInitData(PUNICODE_STRING pProfileUserName)
{
    guDdeSendTimeout = FastGetProfileIntFromID(pProfileUserName,
                                               PMAP_WINDOWSM,
                                               STR_DDESENDTIMEOUT,
                                               0);
}

/***************************************************************************\
* LW_LoadResources
*
*
* History:
\***************************************************************************/

VOID LW_LoadResources(PUNICODE_STRING pProfileUserName)
{
    WCHAR rgch[4];

    /*
     * See if the Mouse buttons need swapping.
     */
    FastGetProfileStringFromIDW(pProfileUserName,
                                PMAP_MOUSE,
                                STR_SWAPBUTTONS,
                                szN,
                                rgch,
                                sizeof(rgch) / sizeof(WCHAR));
    SYSMET(SWAPBUTTON) = (*rgch == '1' || *rgch == *szY || *rgch == *szy);

    /*
     * See if we should beep.
     */
    FastGetProfileStringFromIDW(pProfileUserName,
                                PMAP_BEEP,
                                STR_BEEP,
                                szY,
                                rgch,
                                sizeof(rgch) / sizeof(WCHAR)
                                );

    SET_OR_CLEAR_PUDF(PUDF_BEEP, (rgch[0] == *szY) || (rgch[0] == *szy));

    /*
     * See if we should have extended sounds.
     */
    FastGetProfileStringFromIDW(pProfileUserName,
                                PMAP_BEEP,
                                STR_EXTENDEDSOUNDS,
                                szN,
                                rgch,
                                sizeof(rgch) / sizeof(WCHAR)
                                );

    SET_OR_CLEAR_PUDF(PUDF_EXTENDEDSOUNDS, (rgch[0] == *szY || rgch[0] == *szy));

}
/***************************************************************************\
* xxxInitWindowStation
*
* History:
* 6-Sep-1996 CLupu   Created.
* 21-Jan-98  SamerA  Renamed to xxxInitWindowStation since it may leave the
*                    critical section.
\***************************************************************************/

BOOL xxxInitWindowStation(
    PWINDOWSTATION pwinsta)
{
    TL tlName;
    PUNICODE_STRING pProfileUserName = CreateProfileUserName(&tlName);
    BOOL fRet;

    /*
     * Load all profile data first
     */
    LW_LoadProfileInitData(pProfileUserName);

    /*
     * Initialize User in a specific order.
     */
    LW_DriversInit();

    /*
     * This is the initialization from Chicago
     */
    if (!(fRet = xxxSetWindowNCMetrics(pProfileUserName, NULL, TRUE, -1))) {
        RIPMSG0(RIP_WARNING, "xxxInitWindowStation failed in xxxSetWindowNCMetrics");
        goto Exit;
    }

    SetMinMetrics(pProfileUserName, NULL);

    if (!(fRet = SetIconMetrics(pProfileUserName, NULL))) {
        RIPMSG0(RIP_WARNING, "xxxInitWindowStation failed in SetIconMetrics");
        goto Exit;
    }

    if (!(fRet = FinalUserInit())) {
        RIPMSG0(RIP_WARNING, "xxxInitWindowStation failed in FinalUserInit");
        goto Exit;
    }

    /*
     * Initialize the key cache index.
     */
    gpsi->dwKeyCache = 1;

Exit:
    FreeProfileUserName(pProfileUserName, &tlName);

    return fRet;
    UNREFERENCED_PARAMETER(pwinsta);
}

/***************************************************************************\
* CreateTerminalInput
*
*
* History:
* 6-Sep-1996 CLupu   Created.
\***************************************************************************/

BOOL CreateTerminalInput(
    PTERMINAL pTerm)
{
    UserAssert(pTerm != NULL);

    /*
     * call to the client side to clean up the [Fonts] section
     * of the registry. This will only take significant chunk of time
     * if the [Fonts] key changed during since the last boot and if
     * there are lots of fonts loaded
     */
    ClientFontSweep();

    /*
     * Load the standard fonts before we create any DCs.
     * At this time we can only add the fonts that do not
     * reside on the net. They may be needed by winlogon.
     * Our winlogon needs only ms sans serif, but private
     * winlogon's may need some other fonts as well.
     * The fonts on the net will be added later, right
     * after all the net connections have been restored.
     */
    /*
     * This call should be made in UserInitialize.
     */
    xxxLW_LoadFonts(FALSE);

    /*
     * Initialize the input system.
     */
    if (!xxxInitInput(pTerm))
        return FALSE;

    return TRUE;
}

/***************************************************************************\
* LW_LoadSomeStrings
*
* This function loads a bunch of strings from the string table
* into DS or INTDS. This is done to keep all localizable strings
* in the .RC file.
*
* History:
\***************************************************************************/

VOID LW_LoadSomeStrings(VOID)
{
   int i, str, id;

   /*
    * MessageBox strings
    */
   for (i = 0, str = STR_OK, id = IDOK; i<MAX_MB_STRINGS; i++, str++, id++) {
       gpsi->MBStrings[i].uStr = str;
       gpsi->MBStrings[i].uID = id;
       ServerLoadString(hModuleWin, str, gpsi->MBStrings[i].szName, sizeof(gpsi->MBStrings[i].szName) / sizeof(WCHAR));
   }

   /*
    * Tooltips strings
    */
   ServerLoadString(hModuleWin, STR_MIN,    gszMIN,    sizeof(gszMIN)    / sizeof(WCHAR));
   ServerLoadString(hModuleWin, STR_MAX,    gszMAX,    sizeof(gszMAX)    / sizeof(WCHAR));
   ServerLoadString(hModuleWin, STR_RESUP,  gszRESUP,  sizeof(gszRESUP)  / sizeof(WCHAR));
   ServerLoadString(hModuleWin, STR_RESDOWN,gszRESDOWN,sizeof(gszRESDOWN)/ sizeof(WCHAR));
   /* Commented out due to TandyT ...
    * ServerLoadString(hModuleWin, STR_SMENU,  gszSMENU,  sizeof(gszSMENU)  / sizeof(WCHAR));
    */
   ServerLoadString(hModuleWin, STR_SCLOSE, gszSCLOSE, sizeof(gszSCLOSE) / sizeof(WCHAR));
}

/***************************************************************************\
* CI_GetClrVal
*
* Returns the RGB value of a color string from WIN.INI.
*
* History:
\***************************************************************************/

DWORD CI_GetClrVal(
    LPWSTR p)
{
    LPBYTE pl;
    BYTE   val;
    int    i;
    DWORD  clrval;

    /*
     * Initialize the pointer to the LONG return value.  Set to MSB.
     */
    pl = (LPBYTE)&clrval;

    /*
     * Get three goups of numbers seprated by non-numeric characters.
     */
    for (i = 0; i < 3; i++) {

        /*
         * Skip over any non-numeric characters.
         */
        while (!(*p >= TEXT('0') && *p <= TEXT('9')))
            p++;

        /*
         * Get the next series of digits.
         */
        val = 0;
        while (*p >= TEXT('0') && *p <= TEXT('9'))
            val = (BYTE)((int)val*10 + (int)*p++ - '0');

        /*
         * HACK! Store the group in the LONG return value.
         */
        *pl++ = val;
    }

    /*
     * Force the MSB to zero for GDI.
     */
    *pl = 0;

    return clrval;
}

/***************************************************************************\
* xxxODI_ColorInit
*
*
* History:
\***************************************************************************/

VOID xxxODI_ColorInit(PUNICODE_STRING pProfileUserName)
{
    int      i;
    COLORREF colorVals[STR_COLOREND - STR_COLORSTART + 1];
    INT      colorIndex[STR_COLOREND - STR_COLORSTART + 1];
    WCHAR    rgchValue[25];

#if COLOR_MAX - (STR_COLOREND - STR_COLORSTART + 1)
#error "COLOR_MAX value conflicts with STR_COLOREND - STR_COLORSTART"
#endif

    /*
     * Now set up default color values.
     * These are not in display drivers anymore since we just want default.
     * The real values are stored in the profile.
     */
    RtlCopyMemory(gpsi->argbSystem, gargbInitial, sizeof(COLORREF) * COLOR_MAX);

    for (i = 0; i < COLOR_MAX; i++) {

        /*
         * Try to find a WIN.INI entry for this object.
         */
        *rgchValue = 0;
        FastGetProfileStringFromIDW(pProfileUserName,
                                    PMAP_COLORS,
                                    STR_COLORSTART + i,
                                    szNull,
                                    rgchValue,
                                    sizeof(rgchValue) / sizeof(WCHAR)
                                    );

        /*
         * Convert the string into an RGB value and store.  Use the
         * default RGB value if the profile value is missing.
         */
        colorVals[i]  = *rgchValue ? CI_GetClrVal(rgchValue) : gpsi->argbSystem[i];
        colorIndex[i] = i;
    }

    xxxSetSysColors(pProfileUserName,
                    i,
                    colorIndex,
                    colorVals,
                    SSCF_FORCESOLIDCOLOR | SSCF_SETMAGICCOLORS);
}


/***********************************************************************\
* _LoadIconsAndCursors
*
* Used in parallel with the client side - LoadIconsAndCursors.  This
* assumes that only the default configurable cursors and icons have
* been loaded and searches the global icon cache for them to fixup
* the default resource ids to standard ids.  Also initializes the
* rgsys arrays allowing SYSCUR and SYSICO macros to work.
*
* 14-Oct-1995 SanfordS  created.
\***********************************************************************/

VOID _LoadCursorsAndIcons(VOID)
{
    PCURSOR pcur;
    int     i;

    pcur = gpcurFirst;

    /*
     * Only CSR can call this (and only once).
     */
    if (PsGetCurrentProcess() != gpepCSRSS) {
        return;
    }

    HYDRA_HINT(HH_LOADCURSORS);

    while (pcur) {

        UserAssert(!IS_PTR(pcur->strName.Buffer));

        switch (pcur->rt) {
        case RT_ICON:
            UserAssert((LONG_PTR)pcur->strName.Buffer >= OIC_FIRST_DEFAULT);

            UserAssert((LONG_PTR)pcur->strName.Buffer <
                    OIC_FIRST_DEFAULT + COIC_CONFIGURABLE);

            i = PTR_TO_ID(pcur->strName.Buffer) - OIC_FIRST_DEFAULT;
            pcur->strName.Buffer = (LPWSTR)gasysico[i].Id;

            if (pcur->CURSORF_flags & CURSORF_LRSHARED) {
                UserAssert(gasysico[i].spcur == NULL);
                Lock(&gasysico[i].spcur, pcur);
            } else {
                UserAssert(gpsi->hIconSmWindows == NULL);
                UserAssert((int)pcur->cx == SYSMET(CXSMICON));
                /*
                 * The special small winlogo icon is not shared.
                 */
                gpsi->hIconSmWindows = PtoH(pcur);
            }
            break;

        case RT_CURSOR:
            UserAssert((LONG_PTR)pcur->strName.Buffer >= OCR_FIRST_DEFAULT);

            UserAssert((LONG_PTR)pcur->strName.Buffer <
                    OCR_FIRST_DEFAULT + COCR_CONFIGURABLE);

            i = PTR_TO_ID(pcur->strName.Buffer) - OCR_FIRST_DEFAULT;
            pcur->strName.Buffer = (LPWSTR)gasyscur[i].Id;
            Lock(&gasyscur[i].spcur ,pcur);
            break;

        default:
            UserAssert(FALSE);  // should be nothing in the cache but these!
        }

        pcur = pcur->pcurNext;
    }

    /*
     * copy special icon handles to global spots for later use.
     */
    gpsi->hIcoWindows = PtoH(SYSICO(WINLOGO));
}

/***********************************************************************\
* UnloadCursorsAndIcons
*
* used for cleanup of win32k
*
* Dec-10-1997 clupu  created.
\***********************************************************************/
VOID UnloadCursorsAndIcons(
    VOID)
{
    PCURSOR pcur;
    int     ind;

    TRACE_HYDAPI(("UnloadCursorsAndIcons\n"));

    /*
     * unlock the icons
     */
    for (ind = 0; ind < COIC_CONFIGURABLE; ind++) {
        pcur = gasysico[ind].spcur;

        if (pcur == NULL)
            continue;

        pcur->head.ppi = PpiCurrent();
        Unlock(&gasysico[ind].spcur);
    }

    /*
     * unlock the cursors
     */
    for (ind = 0; ind < COCR_CONFIGURABLE; ind++) {
        pcur = gasyscur[ind].spcur;

        if (pcur == NULL)
            continue;

        pcur->head.ppi = PpiCurrent();
        Unlock(&gasyscur[ind].spcur);
    }
}

/***********************************************************************\
* xxxUpdateSystemCursorsFromRegistry
*
* Reloads all customizable cursors from the registry.
*
* 09-Oct-1995 SanfordS  created.
\***********************************************************************/

VOID xxxUpdateSystemCursorsFromRegistry(PUNICODE_STRING pProfileUserName)
{
    int            i;
    UNICODE_STRING strName;
    TCHAR          szFilename[MAX_PATH];
    PCURSOR        pCursor;
    UINT           LR_flags;

    for (i = 0; i < COCR_CONFIGURABLE; i++) {

        FastGetProfileStringFromIDW(pProfileUserName,
                                    PMAP_CURSORS,
                                    gasyscur[i].StrId,
                                    TEXT(""),
                                    szFilename,
                                    sizeof(szFilename) / sizeof(TCHAR));

        if (*szFilename) {
            RtlInitUnicodeString(&strName, szFilename);
            LR_flags = LR_LOADFROMFILE | LR_ENVSUBST;
        } else {
            RtlInitUnicodeStringOrId(&strName,
                                     MAKEINTRESOURCE(i + OCR_FIRST_DEFAULT));
            LR_flags = LR_ENVSUBST;
        }

        pCursor = xxxClientLoadImage(&strName,
                                     0,
                                     IMAGE_CURSOR,
                                     0,
                                     0,
                                     LR_flags,
                                     FALSE);

        if (pCursor) {
            zzzSetSystemImage(pCursor, gasyscur[i].spcur);
        } else {
            RIPMSG1(RIP_WARNING, "Unable to update cursor. id=%x.", i + OCR_FIRST_DEFAULT);

        }
    }
}

/***********************************************************************\
* xxxUpdateSystemIconsFromRegistry
*
* Reloads all customizable icons from the registry.
*
* 09-Oct-1995 SanfordS  created.
\***********************************************************************/
VOID xxxUpdateSystemIconsFromRegistry(PUNICODE_STRING pProfileUserName)
{
    int            i;
    UNICODE_STRING strName;
    TCHAR          szFilename[MAX_PATH];
    PCURSOR        pCursor;
    UINT           LR_flags;

    for (i = 0; i < COIC_CONFIGURABLE; i++) {

        FastGetProfileStringFromIDW(pProfileUserName,
                                    PMAP_ICONS,
                                    gasysico[i].StrId,
                                    TEXT(""),
                                    szFilename,
                                    sizeof(szFilename) / sizeof(TCHAR));

        if (*szFilename) {
            RtlInitUnicodeString(&strName, szFilename);
            LR_flags = LR_LOADFROMFILE | LR_ENVSUBST;
        } else {
            RtlInitUnicodeStringOrId(&strName,
                                     MAKEINTRESOURCE(i + OIC_FIRST_DEFAULT));
            LR_flags = LR_ENVSUBST;
        }

        pCursor = xxxClientLoadImage(&strName,
                                     0,
                                     IMAGE_ICON,
                                     0,
                                     0,
                                     LR_flags,
                                     FALSE);

        RIPMSG3(RIP_VERBOSE,
                (!IS_PTR(strName.Buffer)) ?
                        "%#.8lx = Loaded id %ld" :
                        "%#.8lx = Loaded file %ws for id %ld",
                PtoH(pCursor),
                strName.Buffer,
                i + OIC_FIRST_DEFAULT);

        if (pCursor) {
            zzzSetSystemImage(pCursor, gasysico[i].spcur);
        } else {
            RIPMSG1(RIP_WARNING, "Unable to update icon. id=%ld", i + OIC_FIRST_DEFAULT);
        }

        /*
         * update the small winlogo icon which is referenced by gpsi.
         * Seems like we should load the small version for all configurable
         * icons anyway.  What is needed is for CopyImage to support
         * copying of images loaded from files with LR_COPYFROMRESOURCE
         * allowing a reaload of the bits. (SAS)
         */
        if (i == OIC_WINLOGO_DEFAULT - OIC_FIRST_DEFAULT) {

            PCURSOR pCurSys = HtoP(gpsi->hIconSmWindows);

            if (pCurSys != NULL) {
                pCursor = xxxClientLoadImage(&strName,
                                             0,
                                             IMAGE_ICON,
                                             SYSMET(CXSMICON),
                                             SYSMET(CYSMICON),
                                             LR_flags,
                                             FALSE);

                if (pCursor) {
                    zzzSetSystemImage(pCursor, pCurSys);
                } else {
                    RIPMSG0(RIP_WARNING, "Unable to update small winlogo icon.");
                }
            }
        }
    }
}

/***************************************************************************\
* LW_BrushInit
*
*
* History:
\***************************************************************************/

BOOL LW_BrushInit(VOID)
{
    HBITMAP hbmGray;
    CONST static WORD patGray[8] = {0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa};

    /*
     * Create a gray brush to be used with GrayString
     */
    hbmGray   = GreCreateBitmap(8, 8, 1, 1, (LPBYTE)patGray);

    if (hbmGray == NULL) {
        return FALSE;
    }

    gpsi->hbrGray  = GreCreatePatternBrush(hbmGray);
    ghbrWhite = GreGetStockObject(WHITE_BRUSH);
    ghbrBlack = GreGetStockObject(BLACK_BRUSH);

    UserAssert(ghbrWhite != NULL && ghbrBlack != NULL);

    if (gpsi->hbrGray == NULL) {
        return FALSE;
    }

    GreDeleteObject(hbmGray);
    GreSetBrushOwnerPublic(gpsi->hbrGray);
    ghbrHungApp = GreCreateSolidBrush(0);

    if (ghbrHungApp == NULL) {
        return FALSE;
    }

    GreSetBrushOwnerPublic(ghbrHungApp);

    return TRUE;
}

/***************************************************************************\
* LW_RegisterWindows
*
*
* History:
\***************************************************************************/
BOOL LW_RegisterWindows(
    BOOL fSystem)
{
#ifdef HUNGAPP_GHOSTING
#define CCLASSES 9
#else // HUNGAPP_GHOSTING
#define CCLASSES 8
#endif // HUNGAPP_GHOSTING

    int        i;
    WNDCLASSEX wndcls;

    CONST static struct {
        BOOLEAN     fSystem;
        BOOLEAN     fGlobalClass;
        WORD        fnid;
        UINT        style;
        WNDPROC     lpfnWndProc;
        int         cbWndExtra;
        BOOL        fNormalCursor : 1;
        HBRUSH      hbrBackground;
        LPCTSTR     lpszClassName;
    } rc[CCLASSES] = {
        { TRUE, TRUE, FNID_DESKTOP,
            CS_DBLCLKS,
            (WNDPROC)xxxDesktopWndProc,
            sizeof(DESKWND) - sizeof(WND),
            TRUE,
            (HBRUSH)(COLOR_BACKGROUND + 1),
            DESKTOPCLASS},
        { TRUE, FALSE, FNID_SWITCH,
            CS_VREDRAW | CS_HREDRAW | CS_SAVEBITS,
            (WNDPROC)xxxSwitchWndProc,
            sizeof(SWITCHWND) - sizeof(WND),
            TRUE,
            NULL,
            SWITCHWNDCLASS},
        { TRUE, FALSE, FNID_MENU,
            CS_DBLCLKS | CS_SAVEBITS,
            (WNDPROC)xxxMenuWindowProc,
            sizeof(PPOPUPMENU),
            FALSE,
            (HBRUSH)(COLOR_MENU + 1),
            MENUCLASS},
        { FALSE, FALSE, FNID_SCROLLBAR,
            CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS | CS_PARENTDC,
            (WNDPROC)xxxSBWndProc,
            sizeof(SBWND) - sizeof(WND),
            TRUE,
            NULL,
            L"ScrollBar"},
        { TRUE, FALSE, FNID_TOOLTIP,
            CS_DBLCLKS | CS_SAVEBITS,
            (WNDPROC)xxxTooltipWndProc,
            sizeof(TOOLTIPWND) - sizeof(WND),
            TRUE,
            NULL,
            TOOLTIPCLASS},
        { TRUE, TRUE, FNID_ICONTITLE,
            0,
            (WNDPROC)xxxDefWindowProc,
            0,
            TRUE,
            NULL,
            ICONTITLECLASS},
        { FALSE, FALSE, 0,
            0,
            (WNDPROC)xxxEventWndProc,
            sizeof(PSVR_INSTANCE_INFO),
            FALSE,
            NULL,
            L"DDEMLEvent"},
#ifdef HUNGAPP_GHOSTING
        { TRUE, TRUE, 0,
            0,
            (WNDPROC)xxxGhostWndProc,
            0,
            TRUE,
            NULL,
            L"Ghost"},
#endif // HUNGAPP_GHOSTING
        { TRUE, TRUE, 0,
            0,
            (WNDPROC)xxxDefWindowProc,
            4,
            TRUE,
            NULL,
            L"Message"}
    };


    /*
     * All other classes are registered via the table.
     */
    wndcls.cbClsExtra   = 0;
    wndcls.hInstance    = hModuleWin;
    wndcls.hIcon        = NULL;
    wndcls.hIconSm      = NULL;
    wndcls.lpszMenuName = NULL;

    for (i = 0; i < CCLASSES; i++) {
        if (fSystem && !rc[i].fSystem) {
            continue;
        }
        wndcls.style        = rc[i].style;
        wndcls.lpfnWndProc  = rc[i].lpfnWndProc;
        wndcls.cbWndExtra   = rc[i].cbWndExtra;
        wndcls.hCursor      = rc[i].fNormalCursor ? PtoH(SYSCUR(ARROW)) : NULL;
        wndcls.hbrBackground= rc[i].hbrBackground;
        wndcls.lpszClassName= rc[i].lpszClassName;

        if (InternalRegisterClassEx(&wndcls,
                                    rc[i].fnid,
                                    CSF_SERVERSIDEPROC | CSF_WIN40COMPAT) == NULL) {
            RIPMSG0(RIP_WARNING, "LW_RegisterWindows: InternalRegisterClassEx failed");
            return FALSE;
        }

        if (fSystem && rc[i].fGlobalClass) {
            if (InternalRegisterClassEx(&wndcls,
                                    rc[i].fnid,
                                    CSF_SERVERSIDEPROC | CSF_SYSTEMCLASS | CSF_WIN40COMPAT) == NULL) {

                RIPMSG0(RIP_WARNING, "LW_RegisterWindows: InternalRegisterClassEx failed");
                return FALSE;
            }
        }
    }
    return TRUE;
}

/**********************************************************\
* VOID vCheckMMInstance
*
* History:
*  Feb-06-98    Xudong Wu [TessieW]
* Wrote it.
\**********************************************************/
VOID vCheckMMInstance(
    LPWSTR pchSrch,
    DESIGNVECTOR  *pdv)
{
    LPWSTR  pKeyName;
    WCHAR   szName[MAX_PATH], *pszName = szName;
    WCHAR   szCannonicalName[MAX_PATH];
    ULONG   NumAxes;

    pdv->dvNumAxes = 0;
    pKeyName = pchSrch;
    while (*pKeyName && (*pKeyName++ != TEXT('(')))
        ;

    if (*pKeyName){
        if (!_wcsicmp(pKeyName, L"OpenType)"))
        {
            pKeyName = pchSrch;
            while(*pKeyName != TEXT('('))
                *pszName++ = *pKeyName++;
            *pszName = 0;

            GreGetCannonicalName(szName, szCannonicalName, &NumAxes, pdv);
        }
    }
}


/***************************************************************************\
* LW_LoadFonts
*
*
* History:
\***************************************************************************/

BOOL bEnumerateRegistryFonts(
    BOOL bPermanent)
{
    LPWSTR pchKeys;
    LPWSTR pchSrch;
    LPWSTR lpchT;
    int    cchReal;
    int    cFont;
    WCHAR  szFontFile[MAX_PATH];
    FLONG  flAFRW;
    TL     tlPool;
    DESIGNVECTOR  dv;

    WCHAR  szPreloadFontFile[MAX_PATH];

    /*
     * if we are not just checking whether this is a registry font
     */
    flAFRW = (bPermanent ? AFRW_ADD_LOCAL_FONT : AFRW_ADD_REMOTE_FONT);

    cchReal = (int)FastGetProfileKeysW(NULL,
            PMAP_FONTS,
            TEXT("vgasys.fnt"),
            &pchKeys
            );

#if DBG
    if (cchReal == 0) {
        RIPMSG0(RIP_WARNING, "bEnumerateRegistryFonts: cchReal is 0");
    }
#endif

    if (!pchKeys)
        return FALSE;

    ThreadLockPool(PtiCurrent(), pchKeys, &tlPool);

    /*
     * If we got here first, we load the fonts until this preload font.
     * Preload fonts will be used by Winlogon UI, then we need to make sure
     * the font is available when Winlogon UI comes up.
     */
    if (LastFontLoaded == -1) {
        FastGetProfileStringW(NULL, PMAP_WINLOGON,
                              TEXT("PreloadFontFile"),
                              TEXT("Micross.ttf"),
                              szPreloadFontFile,
                              MAX_PATH
                              );
        RIPMSG1(RIP_VERBOSE, "Winlogon preload font = %ws\n",szPreloadFontFile);
    }

    /*
     * Now we have all the key names in pchKeys.
     */
    if (cchReal != 0) {

        cFont   = 0;
        pchSrch = pchKeys;

        do {
            // check to see whether this is MM(OpenType) instance
            vCheckMMInstance(pchSrch, &dv);

            if (FastGetProfileStringW(NULL,
                                      PMAP_FONTS,
                                      pchSrch,
                                      TEXT("vgasys.fon"),
                                      szFontFile,
                                      (MAX_PATH - 5)
                                      )) {

                /*
                 * If no extension, append ".FON"
                 */
                for (lpchT = szFontFile; *lpchT != TEXT('.'); lpchT++) {

                    if (*lpchT == 0) {
                        wcscat(szFontFile, TEXT(".FON"));
                        break;
                    }
                }

                if ((cFont > LastFontLoaded) && bPermanent) {

                    /*
                     * skip if we've already loaded this local font.
                     */
                    xxxAddFontResourceW(szFontFile, flAFRW, dv.dvNumAxes ? &dv : NULL);
                }

                if (!bPermanent)
                    xxxAddFontResourceW(szFontFile, flAFRW, dv.dvNumAxes ? &dv : NULL);

                if ((LastFontLoaded == -1) &&
                    /*
                     * Compare with the font file name from Registry.
                     */
                    (!_wcsnicmp(szFontFile, szPreloadFontFile, wcslen(szPreloadFontFile))) &&
                    (bPermanent)) {

                    /*
                     * On the first time through only load up until
                     * ms sans serif for winlogon to use.       Later we
                     * will spawn off a thread which loads the remaining
                     * fonts in the background.
                     */
                    LastFontLoaded = cFont;

                    ThreadUnlockAndFreePool(PtiCurrent(), &tlPool);
                    return TRUE;
                }
            }

            /*
             * Skip to the next key.
             */
            while (*pchSrch++);

            cFont += 1;

        } while (pchSrch < ((LPWSTR)pchKeys + cchReal));
    }

    /*
     * signal that all the permanent fonts have been loaded
     */
    bPermanentFontsLoaded = TRUE;

    ThreadUnlockAndFreePool(PtiCurrent(), &tlPool);

    if (!bPermanent)
        SET_PUDF(PUDF_FONTSARELOADED);

    return TRUE;

}

extern VOID CloseFNTCache(VOID);

/***************************************************************************\
* xxxLW_LoadFonts
*
*
* History:
\***************************************************************************/


VOID xxxLW_LoadFonts(
    BOOL bRemote)
{
    BOOL    bTimeOut = FALSE;

    if(bRemote) {

        LARGE_INTEGER li;
        ULONG         ulWaitCount = 0;

        /*
         * Before we can proceed we must make sure that all the permanent
         * fonts  have been loaded.
         */

        while (!bPermanentFontsLoaded) {

            if (!gbRemoteSession || (ulWaitCount < MAX_TIME_OUT))
            {
                LeaveCrit();
                li.QuadPart = (LONGLONG)-10000 * CMSSLEEP;
                KeDelayExecutionThread(KernelMode, FALSE, &li);
                EnterCrit();
            }
            else
            {
                bTimeOut = TRUE;
                break;
            }

            ulWaitCount++;
        }

        if (!bTimeOut)
        {
            if (!bEnumerateRegistryFonts(FALSE))
                return; // nothing you can do

        // add  remote type 1 fonts.

            ClientLoadRemoteT1Fonts();
        }

    } else {

        xxxAddFontResourceW(L"marlett.ttf", AFRW_ADD_LOCAL_FONT,NULL);
        if (!bEnumerateRegistryFonts(TRUE))
            return; // nothing you can do

    // add local type 1 fonts.

    // only want to be called once, the second time after ms sans serif was installed

        if (bPermanentFontsLoaded)
        {
            ClientLoadLocalT1Fonts();

    // All the fonts loaded, we can close the FNTCache

            CloseFNTCache();
        }

    }
}

/***************************************************************************\
* FinalUserInit
*
* History:
\***************************************************************************/
BOOL FinalUserInit(VOID)
{
    HBITMAP hbm;
    PPCLS   ppcls;

    gpDispInfo->hdcGray = GreCreateCompatibleDC(gpDispInfo->hdcScreen);

    if (gpDispInfo->hdcGray == NULL) {
        return FALSE;
    }

    GreSelectFont(gpDispInfo->hdcGray, ghFontSys);
    GreSetDCOwner(gpDispInfo->hdcGray, OBJECT_OWNER_PUBLIC);

    gpDispInfo->cxGray = gpsi->cxSysFontChar * GRAY_STRLEN;
    gpDispInfo->cyGray = gpsi->cySysFontChar + 2;
    gpDispInfo->hbmGray = GreCreateBitmap(gpDispInfo->cxGray, gpDispInfo->cyGray, 1, 1, 0L);

    if (gpDispInfo->hbmGray == NULL) {
        return FALSE;
    }

    GreSetBitmapOwner(gpDispInfo->hbmGray, OBJECT_OWNER_PUBLIC);

    hbm = GreSelectBitmap(gpDispInfo->hdcGray, gpDispInfo->hbmGray);
    GreSetTextColor(gpDispInfo->hdcGray, 0x00000000L);
    GreSelectBrush(gpDispInfo->hdcGray, gpsi->hbrGray);
    GreSetBkMode(gpDispInfo->hdcGray, OPAQUE);
    GreSetBkColor(gpDispInfo->hdcGray, 0x00FFFFFFL);

    /*
     * Setup menu animation dc for global menu state
     */
    if (MNSetupAnimationDC(&gMenuState)) {
        GreSetDCOwner(gMenuState.hdcAni, OBJECT_OWNER_PUBLIC);
    } else {
        RIPMSG0(RIP_WARNING, "FinalUserInit: MNSetupAnimationDC failed");
    }

    /*
     * Creation of the queue registers some bogus classes.  Get rid
     * of them and register the real ones.
     */
    ppcls = &PpiCurrent()->pclsPublicList;
    while ((*ppcls != NULL) && !((*ppcls)->style & CS_GLOBALCLASS))
        DestroyClass(ppcls);

    return TRUE;
}

/***************************************************************************\
* InitializeClientPfnArrays
*
* This routine gets called by the client to tell the kernel where
* its important functions can be located.
*
* 18-Apr-1995 JimA  Created.
\***************************************************************************/

NTSTATUS InitializeClientPfnArrays(
    CONST PFNCLIENT *ppfnClientA,
    CONST PFNCLIENT *ppfnClientW,
    CONST PFNCLIENTWORKER *ppfnClientWorker,
    HANDLE hModUser)
{
    static BOOL fHaveClientPfns = FALSE;
    /*
     * Remember client side addresses in this global structure.  These are
     * always constant, so this is ok.  Note that if either of the
     * pointers are invalid, the exception will be handled in
     * the thunk and fHaveClientPfns will not be set.
     */
    if (!fHaveClientPfns && ppfnClientA != NULL) {
        if (!ISCSRSS()) {
            RIPMSG0(RIP_WARNING, "InitializeClientPfnArrays failed !csrss");
            return STATUS_UNSUCCESSFUL;
        }
        gpsi->apfnClientA = *ppfnClientA;
        gpsi->apfnClientW = *ppfnClientW;
        gpsi->apfnClientWorker = *ppfnClientWorker;

        gpfnwp[ICLS_BUTTON]  = gpsi->apfnClientW.pfnButtonWndProc;
        gpfnwp[ICLS_EDIT]  = gpsi->apfnClientW.pfnDefWindowProc;
        gpfnwp[ICLS_STATIC]  = gpsi->apfnClientW.pfnStaticWndProc;
        gpfnwp[ICLS_LISTBOX]  = gpsi->apfnClientW.pfnListBoxWndProc;
        gpfnwp[ICLS_SCROLLBAR]  = (PROC)xxxSBWndProc;
        gpfnwp[ICLS_COMBOBOX]  = gpsi->apfnClientW.pfnComboBoxWndProc;
        gpfnwp[ICLS_DESKTOP]  = (PROC)xxxDesktopWndProc;
        gpfnwp[ICLS_DIALOG]  = gpsi->apfnClientW.pfnDialogWndProc;
        gpfnwp[ICLS_MENU]  = (PROC)xxxMenuWindowProc;
        gpfnwp[ICLS_SWITCH]  = (PROC)xxxSwitchWndProc;
        gpfnwp[ICLS_ICONTITLE] = gpsi->apfnClientW.pfnTitleWndProc;
        gpfnwp[ICLS_MDICLIENT] = gpsi->apfnClientW.pfnMDIClientWndProc;
        gpfnwp[ICLS_COMBOLISTBOX] = gpsi->apfnClientW.pfnComboListBoxProc;
        gpfnwp[ICLS_DDEMLEVENT] = NULL;
        gpfnwp[ICLS_DDEMLMOTHER] = NULL;
        gpfnwp[ICLS_DDEML16BIT] = NULL;
        gpfnwp[ICLS_DDEMLCLIENTA] = NULL;
        gpfnwp[ICLS_DDEMLCLIENTW] = NULL;
        gpfnwp[ICLS_DDEMLSERVERA] = NULL;
        gpfnwp[ICLS_DDEMLSERVERW] = NULL;
        gpfnwp[ICLS_IME] = NULL;
        gpfnwp[ICLS_TOOLTIP] = (PROC)xxxTooltipWndProc;

        /*
         * Change this assert when new classes are added.
         */
        UserAssert(ICLS_MAX == ICLS_TOOLTIP+1);

        hModClient = hModUser;
        fHaveClientPfns = TRUE;
    }
#if DBG
    /*
     * BradG - Verify that user32.dll on the client side has loaded
     *   at the correct address.  If not, do an RIPMSG.
     */

    if((ppfnClientA != NULL) &&
       (gpsi->apfnClientA.pfnButtonWndProc != ppfnClientA->pfnButtonWndProc))
        RIPMSG0(RIP_ERROR, "Client side user32.dll not loaded at same address.");
#endif

    return STATUS_SUCCESS;
}

/***************************************************************************\
* GetKbdLangSwitch
*
* read the kbd language hotkey setting - if any - from the registry and set
* LangToggle[] appropriately.
*
* values are:
*              1 : VK_MENU     (this is the default)
*              2 : VK_CONTROL
*              3 : none
* History:
\***************************************************************************/

BOOL GetKbdLangSwitch(PUNICODE_STRING pProfileUserName)
{
    DWORD dwToggle;
    BOOL  bStatus = TRUE;
    LCID  lcid;

    dwToggle = FastGetProfileIntW(pProfileUserName,
                                  PMAP_UKBDLAYOUTTOGGLE,
                                  TEXT("Hotkey"),
                                  1);

    gbGraveKeyToggle = FALSE;

    switch (dwToggle) {
    case 4:
        /*
         * Grave accent keyboard switch for thai locales
         */
        ZwQueryDefaultLocale(FALSE, &lcid);
        gbGraveKeyToggle = (PRIMARYLANGID(lcid) == LANG_THAI) ? TRUE : FALSE;
        /*
         * fall through (intentional) and disable the ctrl/alt toggle mechanism
         */
    case 3:
        gLangToggle[0].bVkey = 0;
        gLangToggle[0].bScan = 0;
        break;

    case 2:
        gLangToggle[0].bVkey = VK_CONTROL;
        break;

    default:
        gLangToggle[0].bVkey = VK_MENU;
        break;
    }

    return bStatus;
}

/***************************************************************************\
* xxxUpdatePerUserSystemParameters
*
* Called by winlogon to set Window system parameters to the current user's
* profile.
*
* != 0 is failure.
*
* 18-Sep-1992 IanJa     Created.
* 18-Nov-1993 SanfordS  Moved more winlogon init code to here for speed.
\***************************************************************************/

BOOL xxxUpdatePerUserSystemParameters(
    HANDLE hToken,
    BOOL   bUserLoggedOn)
{
    int            i;
    HANDLE         hKey;
    DWORD          dwFontSmoothing;
    BOOL           fDragFullWindows;
    TL tlName;
    PUNICODE_STRING pProfileUserName = NULL;


    static struct {
        UINT idSection;
        UINT id;
        UINT idRes;
        UINT def;
    } spi[] = {
        { PMAP_DESKTOP,  SPI_SETSCREENSAVETIMEOUT, STR_SCREENSAVETIMEOUT, 0 },
        { PMAP_DESKTOP,  SPI_SETSCREENSAVEACTIVE,  STR_SCREENSAVEACTIVE,  0 },
        { PMAP_DESKTOP,  SPI_SETDRAGHEIGHT,        STR_DRAGHEIGHT,        4 },
        { PMAP_DESKTOP,  SPI_SETDRAGWIDTH,         STR_DRAGWIDTH,         4 },
        { PMAP_DESKTOP,  SPI_SETWHEELSCROLLLINES,  STR_WHEELSCROLLLINES,  3 },
        { PMAP_KEYBOARD, SPI_SETKEYBOARDDELAY,     STR_KEYDELAY,          0 },
        { PMAP_KEYBOARD, SPI_SETKEYBOARDSPEED,     STR_KEYSPEED,         15 },
        { PMAP_MOUSE,    SPI_SETDOUBLECLICKTIME,   STR_DBLCLKSPEED,     500 },
        { PMAP_MOUSE,    SPI_SETDOUBLECLKWIDTH,    STR_DOUBLECLICKWIDTH,  4 },
        { PMAP_MOUSE,    SPI_SETDOUBLECLKHEIGHT,   STR_DOUBLECLICKHEIGHT, 4 },
        { PMAP_MOUSE,    SPI_SETSNAPTODEFBUTTON,   STR_SNAPTO,            0 },
        { PMAP_WINDOWSU, SPI_SETMENUDROPALIGNMENT, STR_MENUDROPALIGNMENT, 0 },
        { PMAP_INPUTMETHOD, SPI_SETSHOWIMEUI,      STR_SHOWIMESTATUS,     1 },
    };

    PROFINTINFO apii[] = {
        { PMAP_MOUSE,    (LPWSTR)STR_MOUSETHRESH1,          6, &gMouseThresh1 },
        { PMAP_MOUSE,    (LPWSTR)STR_MOUSETHRESH2,         10, &gMouseThresh2 },
        { PMAP_MOUSE,    (LPWSTR)STR_MOUSESPEED,            1, &gMouseSpeed },
        { PMAP_DESKTOP,  (LPWSTR)STR_MENUSHOWDELAY,       400, &gdtMNDropDown },
        { PMAP_DESKTOP,  (LPWSTR)STR_DRAGFULLWINDOWS,       2, &fDragFullWindows },
        { PMAP_DESKTOP,  (LPWSTR)STR_FASTALTTABROWS,        3, &gnFastAltTabRows },
        { PMAP_DESKTOP,  (LPWSTR)STR_FASTALTTABCOLUMNS,     7, &gnFastAltTabColumns },
        { PMAP_DESKTOP,  (LPWSTR)STR_MAXLEFTOVERLAPCHARS,   3, &(gpsi->wMaxLeftOverlapChars) },
        { PMAP_DESKTOP,  (LPWSTR)STR_MAXRIGHTOVERLAPCHARS,  3, &(gpsi->wMaxRightOverlapChars) },
        { PMAP_DESKTOP,  (LPWSTR)STR_FONTSMOOTHING,         0, &dwFontSmoothing },
        { PMAP_INPUTMETHOD, (LPWSTR)STR_HEXNUMPAD,          0, &gfEnableHexNumpad },
        { 0, NULL, 0, NULL }
    };

    UserAssert(IsWinEventNotifyDeferredOK());

    UNREFERENCED_PARAMETER(hToken);

    /*
     * Make sure the caller is the logon process
     */
    if (GetCurrentProcessId() != gpidLogon) {
        RIPMSG0(RIP_WARNING, "Access denied in xxxUpdatePerUserSystemParameters");
        return FALSE;
    }

    pProfileUserName = CreateProfileUserName(&tlName);

    /*
     * Check for new policy.
     */
    CheckDesktopPolicyChange(pProfileUserName);

    /*
     * Get the timeout for low level hooks from the registry
     */
    FastGetProfileValue(pProfileUserName,
            PMAP_DESKTOP,
            (LPWSTR)STR_LLHOOKSTIMEOUT,
            NULL,
            (LPBYTE)&gnllHooksTimeout,
            sizeof(int)
            );

    /*
     * Control Panel User Preferences
     */
    LoadCPUserPreferences(pProfileUserName);

    /*
     * Set syscolors from registry.
     */

    xxxODI_ColorInit(pProfileUserName);

    LW_LoadResources(pProfileUserName);

    /*
     * This is the initialization from Chicago
     */
    xxxSetWindowNCMetrics(pProfileUserName, NULL, TRUE, -1); // Colors must be set first
    SetMinMetrics(pProfileUserName, NULL);
    SetIconMetrics(pProfileUserName, NULL);

    /*
     * Read the keyboard layout switching hot key
     */
    GetKbdLangSwitch(pProfileUserName);

    /*
     * Set the default thread locale for the system based on the value
     * in the current user's registry profile.
     */
    ZwSetDefaultLocale( TRUE, 0 );

    /*
     * Set the default UI language based on the value in the current
     * user's registry profile.
     */
    ZwSetDefaultUILanguage(0);

    /*
     * And then Get it.
     */
    ZwQueryDefaultUILanguage(&(gpsi->UILangID));


    /*
     * Destroy the desktop system menus, so that they're recreated with
     * the correct UI language if the current user's UI language is different
     * from the previous one. This is done by finding the interactive
     * window station and destroying all its desktops's system menus.
     */
    if (grpWinStaList != NULL) {
        PDESKTOP        pdesk;
        PMENU           pmenu;

        UserAssert(!(grpWinStaList->dwWSF_Flags & WSF_NOIO));
        for (pdesk = grpWinStaList->rpdeskList; pdesk != NULL; pdesk = pdesk->rpdeskNext) {
            if (pdesk->spmenuSys != NULL) {
                pmenu = pdesk->spmenuSys;
                if (UnlockDesktopSysMenu(&pdesk->spmenuSys))
                    _DestroyMenu(pmenu);
            }
            if (pdesk->spmenuDialogSys != NULL) {
                pmenu = pdesk->spmenuDialogSys;
                if (UnlockDesktopSysMenu(&pdesk->spmenuDialogSys))
                    _DestroyMenu(pmenu);
            }
        }
    }

    xxxUpdateSystemCursorsFromRegistry(pProfileUserName);

    /*
     * desktop Pattern now.  Note no parameters.  It just goes off
     * and reads win.ini and sets the desktop pattern.
     */
    xxxSystemParametersInfo(SPI_SETDESKPATTERN, (UINT)-1, 0L, 0); // 265 version

    /*
     * Initialize IME show status
     */
    if (bUserLoggedOn) {
        gfIMEShowStatus = IMESHOWSTATUS_NOTINITIALIZED;
    }

    /*
     * now go set a bunch of random values from the win.ini file.
     */
    for (i = 0; i < ARRAY_SIZE(spi); i++) {

        xxxSystemParametersInfo(
                spi[i].id,
                FastGetProfileIntFromID(pProfileUserName,
                                         spi[i].idSection,
                                         spi[i].idRes,
                                         spi[i].def
                                         ),
                0L,
                0
                );
    }

    /*
     * read profile integers and do any fixups
     */
    FastGetProfileIntsW(pProfileUserName, apii);

    if (gnFastAltTabColumns < 2)
        gnFastAltTabColumns = 7;

    if (gnFastAltTabRows < 1)
        gnFastAltTabRows = 3;

    /*
     * If this is the first time the user logs on, set the DragFullWindows
     * to the default. If we have an accelerated device, enable full drag.
     */
    if (fDragFullWindows == 2) {

        LPWSTR pwszd = L"%d";
        WCHAR  szTemp[40];
        WCHAR  szDragFullWindows[40];

        SET_OR_CLEAR_PUDF(
                PUDF_DRAGFULLWINDOWS,
                GreGetDeviceCaps(gpDispInfo->hdcScreen, BLTALIGNMENT) == 0);

        if (bUserLoggedOn) {
            swprintf(szTemp, pwszd, TEST_BOOL_PUDF(PUDF_DRAGFULLWINDOWS));

            ServerLoadString(hModuleWin,
                             STR_DRAGFULLWINDOWS,
                             szDragFullWindows,
                             sizeof(szDragFullWindows) / sizeof(WCHAR));

            FastWriteProfileStringW(pProfileUserName,
                                    PMAP_DESKTOP,
                                    szDragFullWindows,
                                    szTemp);
        }
    } else {
        SET_OR_CLEAR_PUDF(PUDF_DRAGFULLWINDOWS, fDragFullWindows);
    }

    /*
     * !!!LATER!!! (adams) See if the following profile retrievals can't
     * be done in the "spi" array above (e.g. SPI_SETSNAPTO).
     */

    /*
     * Set mouse settings
     */
    gMouseSensitivity = FastGetProfileIntFromID(pProfileUserName,PMAP_MOUSE, STR_MOUSESENSITIVITY, MOUSE_SENSITIVITY_DEFAULT);
    if ((gMouseSensitivity < MOUSE_SENSITIVITY_MIN) || (gMouseSensitivity > MOUSE_SENSITIVITY_MAX)) {
        gMouseSensitivity = MOUSE_SENSITIVITY_DEFAULT ;
    }
    gMouseSensitivityFactor = CalculateMouseSensitivity(gMouseSensitivity) ;

    _SetCaretBlinkTime(FastGetProfileIntFromID(pProfileUserName,PMAP_DESKTOP, STR_BLINK, 500));

    /*
     * Font Information
     */
    GreSetFontEnumeration( FastGetProfileIntW(pProfileUserName,PMAP_TRUETYPE, TEXT("TTOnly"), FALSE));

    /*
     * Mouse tracking variables
     */
    gcxMouseHover = FastGetProfileIntFromID(pProfileUserName,PMAP_MOUSE, STR_MOUSEHOVERWIDTH, SYSMET(CXDOUBLECLK));
    gcyMouseHover = FastGetProfileIntFromID(pProfileUserName,PMAP_MOUSE, STR_MOUSEHOVERHEIGHT, SYSMET(CYDOUBLECLK));
    gdtMouseHover = FastGetProfileIntFromID(pProfileUserName,PMAP_MOUSE, STR_MOUSEHOVERTIME, gdtMNDropDown);

    /*
     * Window animation
     */
    SET_OR_CLEAR_PUDF(PUDF_ANIMATE,
                      FastGetProfileIntFromID(pProfileUserName,PMAP_METRICS, STR_MINANIMATE, TRUE));

    /*
     * Initial Keyboard state:  ScrollLock, NumLock and CapsLock state;
     * global (per-user) kbd layout attributes (such as ShiftLock/CapsLock)
     */
    UpdatePerUserKeyboardIndicators(pProfileUserName);

    gdwKeyboardAttributes = KLL_GLOBAL_ATTR_FROM_KLF(FastGetProfileDwordW(pProfileUserName,PMAP_UKBDLAYOUT, L"Attributes", 0));

    xxxUpdatePerUserAccessPackSettings(pProfileUserName);

    /*
     * If we successfully opened this, we assume we have a network.
     */
    if (hKey = OpenCacheKeyEx(NULL, PMAP_NETWORK, KEY_READ, NULL)) {
        RIPMSG0(RIP_WARNING | RIP_NONAME, "");
        SYSMET(NETWORK) = RNC_NETWORKS;

        ZwClose(hKey);
    }

    SYSMET(NETWORK) |= RNC_LOGON;

    /*
     * Font smoothing
     */
    UserAssert ((dwFontSmoothing == 0) || (dwFontSmoothing == FE_AA_ON));
    GreSetFontEnumeration( dwFontSmoothing | FE_SET_AA );

    /*
     * Desktop Build Number Painting
     */
    if (USER_SHARED_DATA->SystemExpirationDate.QuadPart || gfUnsignedDrivers) {
        gdwCanPaintDesktop = 1;
    } else {
        gdwCanPaintDesktop = FastGetProfileDwordW(pProfileUserName, PMAP_DESKTOP, L"PaintDesktopVersion", 0);
    }

    FreeProfileUserName(pProfileUserName, &tlName);
    return TRUE;
}

/*
 * Called by InitOemXlateTables via SFI_INITANSIOEM
 */
void InitAnsiOem(PCHAR pOemToAnsi, PCHAR pAnsiToOem) {

    UserAssert(gpsi);
    UserAssert(pOemToAnsi);
    UserAssert(pAnsiToOem);

    try {
        ProbeForRead(pOemToAnsi, NCHARS, sizeof(BYTE));
        ProbeForRead(pAnsiToOem, NCHARS, sizeof(BYTE));

        RtlCopyMemory(gpsi->acOemToAnsi, pOemToAnsi, NCHARS);
        RtlCopyMemory(gpsi->acAnsiToOem, pAnsiToOem, NCHARS);


    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
    }
}

/***************************************************************************\
* RegisterLPK
*
* Called by InitializeLpkHooks on the client side after an LPK is
* loaded for the current process.
*
* 05-Nov-1996 GregoryW  Created.
\***************************************************************************/
VOID RegisterLPK(
    DWORD dwLpkEntryPoints)
{
    PpiCurrent()->dwLpkEntryPoints = dwLpkEntryPoints;
}

/***************************************************************************\
* Enforce color-depth dependent settings on systems with less
* then 256 colors.
*
* 2/13/1998   vadimg          created
\***************************************************************************/

void EnforceColorDependentSettings(void)
{
    if (gpDispInfo->fAnyPalette) {
        gbDisableAlpha = TRUE;
    } else if (GreGetDeviceCaps(gpDispInfo->hdcScreen, NUMCOLORS) == -1) {
        gbDisableAlpha = FALSE;
    } else {
        gbDisableAlpha = TRUE;
    }
}

#if DBG
void InitGlobalThreadLockArray(DWORD dwIndex)
{
    PTL pTLArray = gpaThreadLocksArrays[dwIndex];
    int i;
    for (i = 0; i < MAX_THREAD_LOCKS-1; i++) {
        pTLArray[i].next = &pTLArray[i+1];
    }
    pTLArray[MAX_THREAD_LOCKS-1].next = NULL;
}
#endif // DBG

