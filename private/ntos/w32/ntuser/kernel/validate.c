/****************************** Module Header ******************************\
* Module Name: validate.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains functions for validating windows, menus, cursors, etc.
*
* History:
* 01-02-91 DarrinM      Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop
#include <ntsdexts.h>

/*
 * these defines are used for using the validation macros
 * StartValidateHandleMacro and EndValidateHandleMacro
 */
#define ClientSharedInfo()  (&gSharedInfo)
#define ServerInfo()  (gpsi)

#include "wow.h"

#if DBG
#if defined(_X86_)
    CRITSTACK  gCritStack;
#endif // defined(_X86_)
#endif // DBG

/*
 * Globals used only in his file.
 */
__int64   gCSTimeExclusiveWhenEntering;

/***************************************************************************\
* ValidateHwinsta
*
* Validate windowstation handle
*
* History:
* 03-29-91 JimA             Created.
* 06-20-95 JimA             Kernel-mode objects.
\***************************************************************************/

NTSTATUS ValidateHwinsta(
    HWINSTA         hwinsta,
    KPROCESSOR_MODE AccessMode,
    ACCESS_MASK     amDesired,
    PWINDOWSTATION* ppwinsta)
{
    NTSTATUS Status;

    Status = ObReferenceObjectByHandle(
            hwinsta,
            amDesired,
            *ExWindowStationObjectType,
            AccessMode,
            ppwinsta,
            NULL);

    if (!NT_SUCCESS(Status)) {
        RIPNTERR1(Status, RIP_VERBOSE, "ValidateHwinsta failed for %#p", hwinsta);

    } else if ((*ppwinsta)->dwSessionId != gSessionId) {

        RIPNTERR3(STATUS_INVALID_HANDLE, RIP_WARNING,
                  "SessionId %d. Wrong session id %d for pwinsta %#p",
                  gSessionId, (*ppwinsta)->dwSessionId, *ppwinsta);

        ObDereferenceObject(*ppwinsta);
#if DBG
        *ppwinsta = NULL;
#endif // DBG

        return STATUS_INVALID_HANDLE;
    }

    return Status;
}

/***************************************************************************\
* ValidateHdesk
*
* Validate desktop handle
*
* History:
* 03-29-91 JimA             Created.
* 06-20-95 JimA             Kernel-mode objects.
\***************************************************************************/

NTSTATUS ValidateHdesk(
    HDESK           hdesk,
    KPROCESSOR_MODE AccessMode,
    ACCESS_MASK     amDesired,
    PDESKTOP*       ppdesk)
{
    NTSTATUS Status;

    Status = ObReferenceObjectByHandle(
            hdesk,
            amDesired,
            *ExDesktopObjectType,
            AccessMode,
            ppdesk,
            NULL);

    if (NT_SUCCESS(Status)) {

        if ((*ppdesk)->dwSessionId != gSessionId) {

            RIPNTERR3(STATUS_INVALID_HANDLE, RIP_WARNING,
                      "SessionId %d. Wrong session id %d for pdesk %#p",
                      gSessionId, (*ppdesk)->dwSessionId, *ppdesk);

            goto Error;
        }

        LogDesktop(*ppdesk, LDL_VALIDATE_HDESK, TRUE, (ULONG_PTR)PtiCurrent());

        if ((*ppdesk)->dwDTFlags & (DF_DESTROYED | DF_DESKWNDDESTROYED | DF_DYING)) {
            RIPNTERR1(STATUS_INVALID_HANDLE, RIP_ERROR,
                      "ValidateHdesk: destroyed desktop %#p",
                      *ppdesk);
Error:
            ObDereferenceObject(*ppdesk);
#if DBG
            *ppdesk = NULL;
#endif // DBG

            return STATUS_INVALID_HANDLE;
        }
    } else {
        RIPNTERR1(Status, RIP_VERBOSE, "ValidateHdesk failed for %#p", hdesk);
    }

    return Status;
}

/***************************************************************************\
* UserValidateCopyRgn
*
* Validates a region-handle.  This essentially tries to copy the region
* in order to verify the region is valid.  If hrgn isn't a valid region,
* then the combine will fail.  We return a copy of the region.
*
* History:
* 24=Jan-1996   ChrisWil    Created.
\***************************************************************************/

HRGN UserValidateCopyRgn(
    HRGN hrgn)
{
    HRGN hrgnCopy = NULL;


    if (hrgn && (GreValidateServerHandle(hrgn, RGN_TYPE))) {

        hrgnCopy = CreateEmptyRgn();

        if (CopyRgn(hrgnCopy, hrgn) == ERROR) {

            GreDeleteObject(hrgnCopy);

            hrgnCopy = NULL;
        }
    }

    return hrgnCopy;
}

/***************************************************************************\
* ValidateHmenu
*
* Validate menu handle and open it
*
* History:
* 03-29-91 JimA             Created.
\***************************************************************************/

PMENU ValidateHmenu(
    HMENU hmenu)
{
    PTHREADINFO pti = PtiCurrentShared();
    PMENU pmenuRet;

    pmenuRet = (PMENU)HMValidateHandle(hmenu, TYPE_MENU);

    if (pmenuRet != NULL &&
            ((pti->rpdesk != NULL &&  // hack so console initialization works.
            pmenuRet->head.rpdesk != pti->rpdesk))) {
        RIPERR1(ERROR_INVALID_MENU_HANDLE, RIP_WARNING, "Invalid menu handle (%#p)", hmenu);
        return NULL;
    }

    return pmenuRet;
}



/***************************************************************************\
* ValidateHmonitor
*
* Validate monitor handle and open it.
*
* History:
* 03-29-91 JimA             Created.
\***************************************************************************/

PMONITOR ValidateHmonitor(
        HMONITOR hmonitor)
{
    return (PMONITOR)HMValidateSharedHandle(hmonitor, TYPE_MONITOR);
}

/*
 * The handle validation routines should be optimized for time, not size,
 * since they get called so often.
 */
#pragma optimize("t", on)

/***************************************************************************\
* IsHandleEntrySecure
*
* Validate a user handle for a restricted process bypassing the routine to
* get the handle entry.
*
* History:
* August 22, 97   CLupu      Created.
\***************************************************************************/

BOOL IsHandleEntrySecure(
    HANDLE h,
    PHE    phe)
{
    DWORD        bCreateFlags;
    PPROCESSINFO ppiOwner;
    PPROCESSINFO ppiCurrent;
    PW32JOB      pW32Job;
    DWORD        ind;
    PULONG_PTR    pgh;

    /*
     * get the current process
     */
    ppiCurrent = PpiCurrent();

    if (ppiCurrent == NULL)
        return TRUE;

    UserAssert(ppiCurrent->pW32Job != NULL);

    UserAssert(ppiCurrent->W32PF_Flags & W32PF_RESTRICTED);

    /*
     * get the process that owns the handle
     */

    bCreateFlags = gahti[phe->bType].bObjectCreateFlags;

    ppiOwner = NULL;

    if (bCreateFlags & OCF_PROCESSOWNED) {
        ppiOwner = (PPROCESSINFO)phe->pOwner;
    } else if (bCreateFlags & OCF_THREADOWNED) {

        PTHREADINFO pti;

        pti = (PTHREADINFO)phe->pOwner;

        if (pti != NULL) {
            ppiOwner = pti->ppi;
        }
    }

    /*
     * if the owner is NULL then consider the handle secure
     */
    if (ppiOwner == NULL)
        return FALSE;


    /*
     * if the handle is owned by a process in the same job, then it's secure
     */
    if (ppiOwner->pW32Job == ppiCurrent->pW32Job)
        return TRUE;

    /*
     * the handle is not owned by the current process
     */

    pW32Job = ppiCurrent->pW32Job;

    if (pW32Job->pgh == NULL)
        return FALSE;

    pgh = pW32Job->pgh;

    UserAssert(pW32Job->ughCrt <= pW32Job->ughMax);

    for (ind = 0; ind < pW32Job->ughCrt; ind++) {
        if (*(pgh + ind) == (ULONG_PTR)h) {
            return TRUE;
        }
    }

    return FALSE;
}


/***************************************************************************\
* ValidateHandleSecure
*
* Validate a user handle for a restricted process.
*
* History:
* July 29, 97   CLupu      Created.
\***************************************************************************/

BOOL ValidateHandleSecure(
    HANDLE h)
{
    PVOID pobj;

    CheckCritInShared();

    StartValidateHandleMacro(h)
    BeginTypeValidateHandleMacro(pobj, TYPE_GENERIC)

        if (IsHandleEntrySecure(h, phe)) {
            return TRUE;
        }

    EndTypeValidateHandleMacro
    EndValidateHandleMacro

    return FALSE;
}

/***************************************************************************\
* ValidateHwnd
*
* History:
* 08-Feb-1991 mikeke
\***************************************************************************/

PWND FASTCALL ValidateHwnd(
    HWND hwnd)
{
    StartValidateHandleMacro(hwnd)

        /*
         * Now make sure the app is
         * passing the right handle
         * type for this api. If the
         * handle is TYPE_FREE, this'll
         * catch it.
         */
        if (phe->bType == TYPE_WINDOW) {

            PTHREADINFO pti = PtiCurrentShared();

            /*
             * This is called from thunks for routines in the shared critsec.
             */
            PWND pwndRet = (PWND)phe->phead;

            /*
             * This test establishes that the window belongs to the current
             * 'desktop'.. The two exceptions are for the desktop-window of
             * the current desktop, which ends up belonging to another desktop,
             * and when pti->rpdesk is NULL.  This last case happens for
             * initialization of TIF_SYSTEMTHREAD threads (ie. console windows).
             * IanJa doesn't know if we should be test TIF_CSRSSTHREAD here, but
             * JohnC thinks the whole test below is no longer required ??? LATER
             */

            if (pwndRet != NULL) {
                if (phe->bFlags & HANDLEF_DESTROY) {
                    RIPERR2(ERROR_INVALID_WINDOW_HANDLE,
                        RIP_WARNING,"ValidateHwnd, hwnd %#p, pwnd %#p already destroyed\n",
                            hwnd, pwndRet);
                    return NULL;
                }
                if (GETPTI(pwndRet) == pti ||
                       (
                        (pwndRet->head.rpdesk == pti->rpdesk ||
                         (pti->TIF_flags & TIF_SYSTEMTHREAD) ||  // | TIF_CSRSSTHREAD I think
                         GetDesktopView(pti->ppi, pwndRet->head.rpdesk) !=
                                NULL))) {

                    if (IS_THREAD_RESTRICTED(pti, JOB_OBJECT_UILIMIT_HANDLES)) {

                        /*
                         * make sure this window belongs to this process
                         */
                        if (!IsHandleEntrySecure(hwnd, phe)) {
                            RIPERR1(ERROR_INVALID_WINDOW_HANDLE,
                                    RIP_WARNING,
                                    "ValidateHwnd: Invalid hwnd (%#p) for restricted process\n",
                                    hwnd);
                            pwndRet = NULL;
                        }
                    }
                    return pwndRet;
                }
            }
        }

    EndValidateHandleMacro

    RIPERR1(ERROR_INVALID_WINDOW_HANDLE,
            RIP_WARNING,
            "ValidateHwnd: Invalid hwnd (%#p)",
            hwnd);
    return NULL;
}

/*
 * Switch back to default optimization.
 */
#pragma optimize("", on)

/******************************Public*Routine******************************\
*
* UserCritSec routines
*
* Exposes an opaque interface to the user critical section for
* the WNDOBJ code in GRE
*
* Exposed as functions because they aren't time critical and it
* insulates GRE from rebuilding if the definitions of Enter/LeaveCrit change
*
* History:
*  Wed Sep 20 11:19:14 1995 -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

#if DBG

#if defined(_X86_)

    #define GetCallStack()  \
    {                                                               \
        ULONG Hash;                                                 \
                                                                    \
        gCritStack.thread  = PsGetCurrentThread();                  \
        gCritStack.nFrames = GetStackTrace(1,                       \
                                           MAX_STACK_CALLS,         \
                                           gCritStack.trace,        \
                                           &Hash);                  \
    }

    #define FlushCallStack()    \
    {                                                               \
        gCritStack.thread  = NULL;                                  \
        gCritStack.nFrames = 0;                                     \
    }

#else // defined(_X86_)

    #define GetCallStack()
    #define FlushCallStack()

#endif // defined(_X86_)


#endif // DBG

VOID UserEnterUserCritSec(VOID)
{
    EnterCrit();
}

VOID UserLeaveUserCritSec(VOID)
{
    LeaveCrit();
}

#if DBG
VOID UserAssertUserCritSecIn(VOID)
{
    _AssertCritInShared();
}

VOID UserAssertUserCritSecOut(VOID)
{
    _AssertCritOut();
}
#endif // DBG

BOOL UserGetCurrentDesktopId(DWORD* pdwDesktopId)
{
    PDESKTOP pdesktop;

    CheckCritIn();

    /*
     * PtiCurrent()->rpdesk can be NULL !!! (in the case of thread shutdown).
     */

    pdesktop = PtiCurrent()->rpdesk;

    if (pdesktop != grpdeskRitInput) {
        RIPMSG0(RIP_WARNING, "UserGetCurrentDesktopId on wrong desktop pdesk\n");
        return FALSE;
    }

    *pdwDesktopId = pdesktop->dwDesktopId;

    return TRUE;
}

#if 0

//
// Temporary arrays used to track critsec frees
//

#define ARRAY_SIZE 20
#define LEAVE_TYPE 0xf00d0000
#define ENTER_TYPE 0x0000dead

typedef struct _DEBUG_STASHCS {
    RTL_CRITICAL_SECTION Lock;
    DWORD Type;
} DEBUG_STASHCS, *PDEBUG_STASHCS;

DEBUG_STASHCS UserSrvArray[ARRAY_SIZE];

ULONG UserSrvIndex;

VOID
DumpArray(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString,
    LPDWORD IndexAddress,
    LPDWORD ArrayAddress
    )
{
    PNTSD_OUTPUT_ROUTINE Print;
    PNTSD_GET_EXPRESSION EvalExpression;
    PNTSD_GET_SYMBOL GetSymbol;

    DWORD History;
    int InitialIndex;
    PDEBUG_STASHCS Array;
    BOOL b;
    PRTL_CRITICAL_SECTION CriticalSection;
    CHAR Symbol[64], Symbol2[64];
    DWORD Displacement, Displacement2;
    int Position;
    LPSTR p;

    DBG_UNREFERENCED_PARAMETER(hCurrentThread);
    DBG_UNREFERENCED_PARAMETER(dwCurrentPc);

    Print = lpExtensionApis->lpOutputRoutine;
    EvalExpression = lpExtensionApis->lpGetExpressionRoutine;
    GetSymbol = lpExtensionApis->lpGetSymbolRoutine;

    p = lpArgumentString;

    History = 0;

    if ( *p ) {
        History = EvalExpression(p);
        }
    if ( History == 0 || History >= ARRAY_SIZE ) {
        History = 10;
        }

    //
    // Get the Current Index and the array.
    //

    b = ReadProcessMemory(
            hCurrentProcess,
            (LPVOID)IndexAddress,
            &InitialIndex,
            sizeof(InitialIndex),
            NULL
            );
    if ( !b ) {
        return;
        }

    Array = RtlAllocateHeap(RtlProcessHeap(), 0, sizeof(UserSrvArray));
    if ( !Array ) {
        return;
        }

    b = ReadProcessMemory(
            hCurrentProcess,
            (LPVOID)ArrayAddress,
            Array,
            sizeof(UserSrvArray),
            NULL
            );
    if ( !b ) {
        RtlFreeHeap(RtlProcessHeap(), 0, Array);
        return;
        }

    Position = 0;
    while ( History ) {
        InitialIndex--;
        if ( InitialIndex < 0 ) {
            InitialIndex = ARRAY_SIZE-1;
            }

        if (Array[InitialIndex].Type == LEAVE_TYPE ) {
            (Print)("\n(%d) LEAVING Critical Section \n", Position);
            } else {
            (Print)("\n(%d) ENTERING Critical Section \n", Position);
            }

        CriticalSection = &Array[InitialIndex].Lock;

        if ( CriticalSection->LockCount == -1) {
            (Print)("\tLockCount NOT LOCKED\n");
            } else {
            (Print)("\tLockCount %ld\n", CriticalSection->LockCount);
            }
        (Print)("\tRecursionCount %ld\n", CriticalSection->RecursionCount);
        (Print)("\tOwningThread %lx\n", CriticalSection->OwningThread );
#if DBG
        (GetSymbol)(CriticalSection->OwnerBackTrace[ 0 ], Symbol, &Displacement);
        (GetSymbol)(CriticalSection->OwnerBackTrace[ 1 ], Symbol2, &Displacement2);
        (Print)("\tCalling Address %s+%lx\n", Symbol, Displacement);
        (Print)("\tCallers Caller %s+%lx\n", Symbol2, Displacement2);
#endif // DBG
        Position--;
        History--;
        }
    RtlFreeHeap(RtlProcessHeap(), 0, Array);
}


VOID
dsrv(
    HANDLE hCurrentProcess,
    HANDLE hCurrentThread,
    DWORD dwCurrentPc,
    PNTSD_EXTENSION_APIS lpExtensionApis,
    LPSTR lpArgumentString
    )
{
    DumpArray(
        hCurrentProcess,
        hCurrentThread,
        dwCurrentPc,
        lpExtensionApis,
        lpArgumentString,
        &UserSrvIndex,
        (LPDWORD)&UserSrvArray[0]
        );
}

#endif // if 0

#if DBG

#ifdef EXTRAHEAPCHECKING

VOID ValidateUserHeaps( VOID )
{
    PWINDOWSTATION  pwinsta;
    PDESKTOP        pdesk;

    for (pwinsta = grpwinstaList; pwinsta; pwinsta = pwinsta->rpwinstaNext) {
        for (pdesk = pwinsta->rpdeskList; pdesk; pdesk = pdesk->rpdeskNext) {
            if (!wcscmp(pdesk->lpszDeskName, L"Default")) {
                RtlValidateHeap(Win32HeapGetHandle(pdesk->pheapDesktop), 0, NULL);  // desktop heaps
            }
        }
    }
}

#endif // EXTRAHEAPCHECKING

/***************************************************************************\
* _EnterCrit
* _LeaveCrit
*
* These are temporary routines that are used by USER.DLL until the critsect,
* validation, mapping code is moved to the server-side stubs generated by
* SMeans' Thank compiler.
*
* History:
* 01-02-91 DarrinM      Created.
\***************************************************************************/

void _AssertCritIn()
{
    UserAssert(gpresUser != NULL);
    UserAssert(ExIsResourceAcquiredExclusiveLite(gpresUser) == TRUE);
}

void _AssertDeviceInfoListCritIn()
{
    UserAssert(gpresDeviceInfoList != NULL);
    UserAssert(ExIsResourceAcquiredExclusiveLite(gpresDeviceInfoList) == TRUE);
}

void _AssertCritInShared()
{
    UserAssert(gpresUser != NULL);
    UserAssert( (ExIsResourceAcquiredExclusiveLite(gpresUser) == TRUE) ||
            (ExIsResourceAcquiredSharedLite(gpresUser) == TRUE));
}


void _AssertCritOut()
{
    UserAssert(gpresUser != NULL);
    UserAssert(ExIsResourceAcquiredExclusiveLite(gpresUser) == FALSE);
}

void _AssertDeviceInfoListCritOut()
{
    UserAssert(gpresDeviceInfoList != NULL);
    UserAssert(ExIsResourceAcquiredExclusiveLite(gpresDeviceInfoList) == FALSE);
}

/***************************************************************************\
* BeginAtomicCheck()
* EndAtomicCheck()
*
* Routine that verify we never leave the critical section and that an
* operation is truely atomic with the possiblity of other code being run
* because we left the critical section
*
\***************************************************************************/

void BeginAtomicCheck()
{
    gdwInAtomicOperation++;
}

void EndAtomicCheck()
{
    UserAssert(gdwInAtomicOperation > 0);
    gdwInAtomicOperation--;
}

void BeginAtomicDeviceInfoListCheck()
{
    gdwInAtomicDeviceInfoListOperation++;
}

void EndAtomicDeviceInfoListCheck()
{
    UserAssert(gdwInAtomicDeviceInfoListOperation > 0);
    gdwInAtomicDeviceInfoListOperation--;
}

#define INCCRITSECCOUNT (gdwCritSecUseCount++)
#define INCDEVICEINFOLISTCRITSECCOUNT (gdwDeviceInfoListCritSecUseCount++)

#else // else DBG

#define INCCRITSECCOUNT
#define INCDEVICEINFOLISTCRITSECCOUNT

#endif // endif DBG

BOOL UserIsUserCritSecIn()
{
    UserAssert(gpresUser != NULL);
    return( (ExIsResourceAcquiredExclusiveLite(gpresUser) == TRUE) ||
            (ExIsResourceAcquiredSharedLite(gpresUser) == TRUE));

    return(TRUE);
}

#if DBG
void CheckDevLockOut()
{
    /*
     * gpDispInfo can be NULL if Win32UserInitialize fails before allocationg it.
     * hDev is initialized later in InitVideo, after the critical section has been
     *  released at least once; so we better check it too.
     */
    if ((gpDispInfo != NULL) && (gpDispInfo->hDev != NULL)) {
        UserAssert(!GreIsDisplayLocked(gpDispInfo->hDev));
    }
}
#else
#define CheckDevLockOut()
#endif

void EnterCrit(void)
{
    CheckCritOut();
    CheckDeviceInfoListCritOut();
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(gpresUser, TRUE);
    CheckDevLockOut();
    UserAssert(!ISATOMICCHECK());
    UserAssert(gptiCurrent == NULL);
    gptiCurrent = ((PTHREADINFO)(W32GetCurrentThread()));
    INCCRITSECCOUNT;
#if defined (USER_PERFORMANCE)
    {
        __int64 i64Frecv;
        *(LARGE_INTEGER*)(&gCSTimeExclusiveWhenEntering) = KeQueryPerformanceCounter((LARGE_INTEGER*)&i64Frecv);
        InterlockedIncrement(&gCSStatistics.cExclusive);
    }
#endif // (USER_PERFORMANCE)

#if DBG
    GetCallStack();
#endif // DBG
}

#if DBG
void EnterDeviceInfoListCrit(void)
{
    CheckDeviceInfoListCritOut();
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(gpresDeviceInfoList, TRUE);
    UserAssert(!ISATOMICDEVICEINFOLISTCHECK());
    INCDEVICEINFOLISTCRITSECCOUNT;
}
#endif // DBG

void EnterSharedCrit(void)
{
    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite(gpresUser, TRUE);
    CheckDevLockOut();
    UserAssert(!ISATOMICCHECK());
#if defined (USER_PERFORMANCE)
    InterlockedIncrement(&gCSStatistics.cShared);
#endif // (USER_PERFORMANCE)

    INCCRITSECCOUNT;
}

void LeaveCrit(void)
{
    INCCRITSECCOUNT;
#if DBG
    UserAssert(!ISATOMICCHECK());
    UserAssert(IsWinEventNotifyDeferredOK());
    CheckDevLockOut();
    FlushCallStack();
    gptiCurrent = NULL;
#endif // DBG

#if defined (USER_PERFORMANCE)
    /*
     * A non null gCSTimeExclusiveWhenEntering means the
     * critical section is owned exclusive
     */
    if (gCSTimeExclusiveWhenEntering) {
        __int64 i64Temp, i64Frecv;

        *(LARGE_INTEGER*)(&i64Temp) = KeQueryPerformanceCounter((LARGE_INTEGER*)&i64Frecv);
        gCSStatistics.i64TimeExclusive += i64Temp - gCSTimeExclusiveWhenEntering;
        gCSTimeExclusiveWhenEntering = 0;
    }
#endif // (USER_PERFORMANCE)
    ExReleaseResource(gpresUser);
    KeLeaveCriticalRegion();
    CheckCritOut();
}

#if DBG
void _LeaveDeviceInfoListCrit(void)
{
    INCDEVICEINFOLISTCRITSECCOUNT;
    UserAssert(!ISATOMICDEVICEINFOLISTCHECK());

    ExReleaseResource(gpresDeviceInfoList);
    KeLeaveCriticalRegion();
    CheckDeviceInfoListCritOut();
}
#endif // DBG

VOID ChangeAcquireResourceType(
    VOID)
{
#if DBG
    FlushCallStack();
    CheckDevLockOut();
    UserAssert(!ISATOMICCHECK());
#endif // DBG
    ExReleaseResource(gpresUser);
    ExAcquireResourceExclusiveLite(gpresUser, TRUE);
    gptiCurrent = ((PTHREADINFO)(W32GetCurrentThread()));
#if DBG
    GetCallStack();
#endif // DBG
}


#if DBG

PTHREADINFO _ptiCrit(void)
{
    UserAssert(gpresUser);
    UserAssert(ExIsResourceAcquiredExclusiveLite(gpresUser) == TRUE);
    UserAssert(gptiCurrent);
    UserAssert(gptiCurrent == ((PTHREADINFO)(W32GetCurrentThread())));
    UserAssert(gptiCurrent);
    return gptiCurrent;
}

PTHREADINFO _ptiCritShared(void)
{
    UserAssert(W32GetCurrentThread());
    return ((PTHREADINFO)(W32GetCurrentThread()));
}

#undef KeUserModeCallback

NTSTATUS
_KeUserModeCallback (
    IN ULONG ApiNumber,
    IN PVOID InputBuffer,
    IN ULONG InputLength,
    OUT PVOID *OutputBuffer,
    OUT PULONG OutputLength
    )
{

    UserAssert(ExIsResourceAcquiredExclusiveLite(gpresUser) == FALSE);

    /*
     * Added this so we can detect an erroneous user mode callback
     * with a checked win32k on top of a free system.
     */
    ASSERT(KeGetPreviousMode() == UserMode);

    return KeUserModeCallback( ApiNumber, InputBuffer, InputLength,
            OutputBuffer, OutputLength);
}

#endif
