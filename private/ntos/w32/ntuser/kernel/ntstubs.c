/****************************** Module Header ******************************\
* Module Name: ntstubs.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Kernel-mode stubs
*
* History:
* 03-16-95 JimA             Created.
* 08-12-96 jparsons         Added lparam validate for WM_NCCREATE [51986]
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include "cscall.h"

#include <dbt.h>
#define PROTOS_ONLY 1
#include "msgdef.h"

#if DBG
int ThreadLockCount(void)
{
    PTHREADINFO ptiCurrent = PtiCurrentShared();
    PTL ptl = ptiCurrent->ptl;
    int nLocks = 0;

    while (ptl != NULL) {
        nLocks++;
        ptl = ptl->next;
    }
    ptl = ptiCurrent->ptlW32;
    while (ptl != NULL) {
        nLocks++;
        ptl = ptl->next;
    }
    return nLocks;
}
void ThreadLockCheck(int nLocks)
{
    int nNewCount = ThreadLockCount();
    if (nLocks != nNewCount) {

        if (**((PBOOLEAN*)&KdDebuggerEnabled)) {

            /*
             * The below is as is because it is intended to work on both
             * free and checked builds when thread lock counting is on.
             */
            #undef DbgPrint
            DbgPrint("ThreadLocks mismatch %d %d\n", nLocks, nNewCount);
            DbgBreakPoint();
        }
    }
}
// The parameter is used ensure BEGINRECV* matches ENDRECV* in each stub
#define DBG_THREADLOCK_START(s)   { int nLocks ## s = ThreadLockCount();
#define DBG_THREADLOCK_END(s)     ThreadLockCheck(nLocks ## s); }
#else // DBG_THREADLOCK
#define DBG_THREADLOCK_START(s)
#define DBG_THREADLOCK_END(s)
#endif // DBG_THREADLOCK

/*
 * Setup and control macros
 */
#define BEGINRECV(type, err)    \
    type retval;                \
    type errret = err;          \
    EnterCrit();                \
    DBG_THREADLOCK_START(_)
#define ENDRECV() ENDRECV_(_)

#define BEGINATOMICRECV(type, err)    \
    type retval;                \
    type errret = err;          \
    EnterCrit();                \
    DBG_THREADLOCK_START(_)     \
    BEGINATOMICCHECK()
#define ENDATOMICRECV() ENDATOMICRECV_(_)

#define BEGINRECVCSRSS(type, err)      \
    type retval;                       \
    type errret = err;                 \
    EnterCrit();                       \
    DBG_THREADLOCK_START(CSRSS)          \
    if (!ISCSRSS()) {                  \
        retval = STATUS_ACCESS_DENIED; \
        goto errorexit;                \
    }
#define ENDRECVCSRSS() ENDRECV_(CSRSS)

#define BEGINRECV_SHARED(type, err) \
    type retval;                    \
    type errret = err;              \
    EnterSharedCrit();              \
    DBG_THREADLOCK_START(SHARED)
#define ENDRECV_SHARED() ENDRECV_(SHARED)

#define BEGINRECV_NOCRIT(type, err) \
    type retval;                    \
    type errret = err;

#define BEGINRECV_VOID() \
    EnterCrit();         \
    DBG_THREADLOCK_START(_VOID)
#define ENDRECV_VOID() ENDRECV_VOID_(_VOID)

#define BEGINRECV_HWND(type, err, hwnd)           \
    type retval;                                  \
    type errret = err;                            \
    PWND pwnd;                                    \
    EnterCrit();                                  \
    DBG_THREADLOCK_START(HWND)                    \
    if ((pwnd = ValidateHwnd((hwnd))) == NULL) {  \
        retval = errret;                          \
        goto errorexit;                           \
    }
#define ENDRECV_HWND() ENDRECV_HWND_(HWND)

#define BEGINATOMICRECV_HWND(type, err, hwnd)     \
    type retval;                                  \
    type errret = err;                            \
    PWND pwnd;                                    \
    EnterCrit();                                  \
    DBG_THREADLOCK_START(HWND)                    \
    BEGINATOMICCHECK()                            \
    if ((pwnd = ValidateHwnd((hwnd))) == NULL) {  \
        retval = errret;                          \
        goto errorexit;                           \
    }
#define ENDATOMICRECV_HWND() ENDATOMICRECV_HWND_(HWND)

#define BEGINRECV_HWND_VOID(hwnd)                 \
    PWND pwnd;                                    \
    EnterCrit();                                  \
    DBG_THREADLOCK_START(HWND_VOID)               \
    if ((pwnd = ValidateHwnd((hwnd))) == NULL) {  \
        goto errorexit;                           \
    }
#define ENDRECV_HWND_VOID() ENDRECV_VOID_(HWND_VOID)

#define BEGINRECV_HWND_SHARED(type, err, hwnd)    \
    type retval;                                  \
    type errret = err;                            \
    PWND pwnd;                                    \
    EnterSharedCrit();                            \
    DBG_THREADLOCK_START(HWND_SHARED)             \
    if ((pwnd = ValidateHwnd((hwnd))) == NULL) {  \
        retval = errret;                          \
        goto errorexit;                           \
    }
#define ENDRECV_HWND_SHARED() ENDRECV_HWND_(HWND_SHARED)

#define BEGINRECV_HWNDOPT_SHARED(type, err, hwnd)    \
    type retval;                                     \
    type errret = err;                               \
    PWND pwnd;                                       \
    EnterSharedCrit();                               \
    DBG_THREADLOCK_START(HWND_OPT_SHARED)              \
    if (hwnd) {                                      \
        if ((pwnd = ValidateHwnd((hwnd))) == NULL) { \
            retval = errret;                         \
            goto errorexit;                          \
        }                                            \
    } else {                                         \
        pwnd = NULL;                                 \
    }
#define ENDRECV_HWNDOPT_SHARED() ENDRECV_HWND_(HWND_OPT_SHARED)

#define BEGINRECV_HWNDLOCK(type, err, hwnd)      \
    type retval;                                 \
    type errret = err;                           \
    PWND pwnd;                                   \
    TL tlpwnd;                                   \
    PTHREADINFO ptiCurrent;                      \
    EnterCrit();                                 \
    DBG_THREADLOCK_START(HWNDLOCK)                 \
    if ((pwnd = ValidateHwnd((hwnd))) == NULL) { \
        retval = errret;                         \
        goto errorexit2;                         \
    }                                            \
    ptiCurrent = PtiCurrent();                   \
    ThreadLockAlwaysWithPti(ptiCurrent, pwnd, &tlpwnd);
#define ENDRECV_HWNDLOCK() ENDRECV_HWNDLOCK_(HWNDLOCK)

#define BEGINRECV_HWNDLOCK_ND(type, err, hwnd)       \
    type retval;                                     \
    type errret = err;                               \
    PWND pwndND;                                     \
    TL tlpwnd;                                       \
    PTHREADINFO ptiCurrent;                          \
    EnterCrit();                                     \
    DBG_THREADLOCK_START(HWNDLOCK_ND)                \
    if (((pwndND = ValidateHwnd((hwnd))) == NULL) || \
            (pwndND == _GetDesktopWindow()) ||       \
            (pwndND == _GetMessageWindow())) {       \
        retval = errret;                             \
        goto errorexit2;                             \
    }                                                \
    ptiCurrent = PtiCurrent();                       \
    ThreadLockAlwaysWithPti(ptiCurrent, pwndND, &tlpwnd);
#define ENDRECV_HWNDLOCK_ND() ENDRECV_HWNDLOCK_(HWNDLOCK_ND)

#define BEGINRECV_HWNDLOCK_OPT(type, err, hwnd) \
    type retval;                                    \
    type errret = err;                              \
    PWND pwnd;                                      \
    TL tlpwnd;                                      \
    PTHREADINFO ptiCurrent;                         \
    EnterCrit();                                    \
    DBG_THREADLOCK_START(HWNDLOCK_OPT)                \
    if (hwnd) {                                     \
        if ((pwnd = ValidateHwnd(hwnd)) == NULL) {  \
            retval = errret;                        \
            goto errorexit2;                        \
        }                                           \
    } else {                                        \
        pwnd = NULL;                                \
    }                                               \
    ptiCurrent = PtiCurrent();                      \
    ThreadLockWithPti(ptiCurrent, pwnd, &tlpwnd);
#define ENDRECV_HWNDLOCK_OPT() ENDRECV_HWNDLOCK_(HWNDLOCK_OPT)

#define BEGINRECV_HWNDLOCK_VOID(hwnd) \
    PWND pwnd;                                          \
    TL tlpwnd;                                          \
    PTHREADINFO ptiCurrent;                             \
    EnterCrit();                                        \
    DBG_THREADLOCK_START(HWNDLOCK_VOID)                   \
    if ((pwnd = ValidateHwnd((hwnd))) == NULL) {        \
        goto errorexit2;                                \
    }                                                   \
    ptiCurrent = PtiCurrent();                          \
    ThreadLockAlwaysWithPti(ptiCurrent, pwnd, &tlpwnd);

#define ISXPFNPROCINRANGE(range)                        \
    ((xpfnProc >= SFI_BEGINTRANSLATE ## range)          \
        && (xpfnProc < SFI_ENDTRANSLATE ## range))

#define VALIDATEXPFNPROC(range)                                 \
    UserAssert(SFI_ENDTRANSLATETABLE == ulMaxSimpleCall);       \
    UserAssert(SFI_ENDTRANSLATE ## range <= ulMaxSimpleCall);   \
    if (!ISXPFNPROCINRANGE(range)) {                            \
        MSGERROR(0);                                             \
    }



#define CLEANUPRECV() \
cleanupexit:

#define ENDRECV_(s)       \
    goto errorexit;       \
errorexit:                \
    DBG_THREADLOCK_END(s) \
    LeaveCrit();          \
    return retval;

#define ENDATOMICRECV_(s) \
    goto errorexit;       \
errorexit:                \
    ENDATOMICCHECK()      \
    DBG_THREADLOCK_END(s) \
    LeaveCrit();          \
    return retval;

#define ENDRECV_NOCRIT() \
    goto errorexit;      \
errorexit:               \
    return retval;

#define ENDRECV_VOID_(s) \
    goto errorexit;      \
errorexit:               \
    DBG_THREADLOCK_END(s)  \
    LeaveCrit();         \
    return;

#define ENDRECV_HWND_(s) \
    goto errorexit;         \
errorexit:                  \
    DBG_THREADLOCK_END(s)     \
    LeaveCrit();            \
    return retval;

#define ENDATOMICRECV_HWND_(s) \
    goto errorexit;         \
errorexit:                  \
    ENDATOMICCHECK()        \
    DBG_THREADLOCK_END(s)   \
    LeaveCrit();            \
    return retval;

#define ENDRECV_HWNDLOCK_(s) \
    goto errorexit;          \
errorexit:                   \
    ThreadUnlock(&tlpwnd);   \
errorexit2:                  \
    DBG_THREADLOCK_END(s)      \
    LeaveCrit();             \
    return retval;

#define ENDRECV_HWNDLOCK_VOID() \
    goto errorexit;                 \
errorexit:                          \
    ThreadUnlock(&tlpwnd);          \
errorexit2:                         \
    DBG_THREADLOCK_END(HWNDLOCK_VOID) \
    LeaveCrit();                    \
    return;

/*
 * MSGERROR - exit the stub with an error condition.
 * Parameters:
 * LastError (OPTIONAL):
 * == 0 If LastError is 0, the compiler will optimize away the call to
 *      UserSetLastError().  The last error will not be set to zero!
 * != 0 If you want to SetLastError, provide a non-zero value.
 */
#define MSGERROR(LastError) {        \
    retval = errret;                 \
    if (LastError) {                 \
        UserSetLastError(LastError); \
    }                                \
    goto errorexit; }


#define MSGERROR_VOID() { \
    goto errorexit; }

/*
 * Same as MSGERROR but jumps to cleanup code instead of straight to the return
 */
#define MSGERRORCLEANUP(LastError) { \
    retval = errret;                 \
    if (LastError) {                 \
        UserSetLastError(LastError); \
    }                                \
    goto cleanupexit; }

#define StubExceptionHandler(fSetLastError)  W32ExceptionHandler((fSetLastError), RIP_WARNING)

#define TESTFLAGS(flags, mask) \
    if (((flags) & ~(mask)) != 0) { \
        RIPERR2(ERROR_INVALID_FLAGS, RIP_WARNING, "Invalid flags, %x & ~%x != 0 " #mask, \
            flags, mask); \
        MSGERROR(0);   \
    }

#define LIMITVALUE(value, limit, szText) \
    if ((UINT)(value) > (UINT)(limit)) {     \
        RIPERR3(ERROR_INVALID_PARAMETER, RIP_WARNING, "Invalid parameter, %d > %d in %s", \
             value, limit, szText); \
        MSGERROR(0);     \
    }

#define MESSAGECALL(api) \
LRESULT NtUserfn ## api( \
    PWND pwnd,           \
    UINT msg,            \
    WPARAM wParam,       \
    LPARAM lParam,       \
    ULONG_PTR xParam,     \
    DWORD xpfnProc,      \
    BOOL bAnsi)

#define BEGINRECV_MESSAGECALL(err)      \
    LRESULT retval;                     \
    LRESULT errret = err;               \
    PTHREADINFO ptiCurrent = PtiCurrent(); \
    CheckCritIn();

#define ENDRECV_MESSAGECALL()           \
    goto errorexit;                     \
errorexit:                              \
    return retval;

#define BEGINRECV_HOOKCALL()            \
    LRESULT retval;                     \
    LRESULT errret = 0;                 \
    CheckCritIn();

#define ENDRECV_HOOKCALL()              \
    goto errorexit;                     \
errorexit:                              \
    return retval;

#define CALLPROC(p) FNID(p)

/*
 * Validation macros
 */
#define ValidateHWNDNoRIP(p,h)              \
    if ((p = ValidateHwnd(h)) == NULL)      \
        MSGERROR(0);

#define ValidateHWND(p,h)                   \
    if ((p = ValidateHwnd(h)) == NULL)      \
        MSGERROR(0);

#define ValidateHWNDND(p,h)                 \
    if ( ((p = ValidateHwnd(h)) == NULL) || \
         (p == _GetDesktopWindow()) ||      \
         (p == _GetMessageWindow())         \
        )       \
        MSGERROR(0);

#define ValidateHWNDOPT(p,h) \
    if (h) {                                \
        if ((p = ValidateHwnd(h)) == NULL)  \
            MSGERROR(0);                     \
    } else {                                \
        p = NULL;                           \
    }

#define ValidateHWNDIA(p,h)                      \
    if (h != HWND_TOP &&                         \
        h != HWND_BOTTOM &&                      \
        h != HWND_TOPMOST &&                     \
        h != HWND_NOTOPMOST) {                   \
        if ( ((p = ValidateHwnd(h)) == NULL) ||  \
             (p == _GetDesktopWindow()) ||       \
             (p == _GetMessageWindow())          \
            )        \
            MSGERROR(0);                          \
    } else {                                     \
        p = (PWND)h;                             \
    }

#define ValidateHWNDFF(p,h) \
    if ((h != (HWND)-1) && (h !=(HWND)0xffff)) { \
        if ((p = ValidateHwnd(h)) == NULL)       \
            MSGERROR(0);                          \
    } else {                                     \
        p = PWND_BROADCAST;                      \
    }

#define ValidateHMENUOPT(p,h) \
    if (h) {                                \
        if ((p = ValidateHmenu(h)) == NULL) \
            MSGERROR(0);                     \
    } else {                                \
        p = NULL;                           \
    }

#define ValidateHMENU(p,h) \
    if ((p = ValidateHmenu(h)) == NULL) \
        MSGERROR(0);

#define ValidateHMENUMODIFY(p,h) \
    if ((p = ValidateHmenu(h)) == NULL)  {          \
        MSGERROR(0);                                 \
    } else if (TestMF(p, MFDESKTOP)) { \
        RIPMSG1(RIP_WARNING, "ValidateHMENUMODIFY: Attempt to modify desktop menu:%#p", p); \
        MSGERROR(0);                                 \
    }

#define ValidateHACCEL(p,h) \
    if ((p = HMValidateHandle(h, TYPE_ACCELTABLE)) == NULL) \
        MSGERROR(0);

#define ValidateHCURSOR(p,h) \
    if ((p = HMValidateHandle(h, TYPE_CURSOR)) == NULL) \
        MSGERROR(0);

#define ValidateHCURSOROPT(p,h) \
    if (h) {                                 \
        if ((p = HMValidateHandle(h, TYPE_CURSOR)) == NULL) \
        MSGERROR(0);                          \
    } else {                                \
        p = NULL;                           \
    }

#define ValidateHICON(p,h) \
    if ((p = HMValidateHandle(h, TYPE_CURSOR)) == NULL) \
        MSGERROR(0);

#define ValidateHHOOK(p,h) \
    if ((p = HMValidateHandle(h, TYPE_HOOK)) == NULL) \
        MSGERROR(0);

#define ValidateHWINEVENTHOOK(p,h) \
    if ((p = HMValidateHandle(h, TYPE_WINEVENTHOOK)) == NULL) \
        MSGERROR(0);

#define ValidateHDWP(p,h) \
    if ((p = HMValidateHandle(h, TYPE_SETWINDOWPOS)) == NULL) \
        MSGERROR(0);

#define ValidateHMONITOR(p,h) \
    if ((p = ValidateHmonitor(h)) == NULL) \
        MSGERROR(0);

#define ValidateHIMC(p,h) \
    if ((p = HMValidateHandle((HANDLE)h, TYPE_INPUTCONTEXT)) == NULL) \
        MSGERROR(0);

#define ValidateHIMCOPT(p,h) \
    if (h) {                                                              \
        if ((p = HMValidateHandle((HANDLE)h, TYPE_INPUTCONTEXT)) == NULL) \
            MSGERROR(0);                                                   \
    } else {                                                              \
        p = NULL;                                                         \
    }

#define ValidateIMMEnabled()                                                                    \
    if (!IS_IME_ENABLED()) {                                                                    \
        RIPERR0(ERROR_CALL_NOT_IMPLEMENTED, RIP_VERBOSE, "IME is disabled in this system.");    \
        MSGERROR(0); \
    }

#define ValidateIMMEnabledVOID()                                                                \
    if (!IS_IME_ENABLED()) {                                                                    \
        RIPERR0(ERROR_CALL_NOT_IMPLEMENTED, RIP_VERBOSE, "IME is disabled in this system.");    \
        MSGERROR_VOID();                                                                        \
    }

NTSTATUS
NtUserRemoteConnect(
    IN PDOCONNECTDATA pDoConnectData,
    IN ULONG DisplayDriverNameLength,
    IN PWCHAR DisplayDriverName )
{
    BEGINRECVCSRSS(NTSTATUS, STATUS_UNSUCCESSFUL);
    if (!ISTS()) {
        errret = STATUS_ACCESS_DENIED;
        MSGERROR(0);
    }

    /*
     * Probe all read arguments -- no try block.
     */
#if DBG
    ProbeForRead(DisplayDriverName, DisplayDriverNameLength, sizeof(BYTE));
    ProbeForRead(pDoConnectData, sizeof(DOCONNECTDATA), sizeof(BYTE));
#endif

    retval = RemoteConnect(
                 pDoConnectData,
                 DisplayDriverNameLength,
                 DisplayDriverName);


    TRACE("NtUserRemoteConnect");
    ENDRECVCSRSS();

}

NTSTATUS
NtUserRemoteRedrawRectangle(
    IN WORD Left,
    IN WORD Top,
    IN WORD Right,
    IN WORD Bottom )
{
    BEGINRECVCSRSS(NTSTATUS, STATUS_UNSUCCESSFUL);
    if (!ISTS()) {
        errret = STATUS_ACCESS_DENIED;
        MSGERROR(0);
    }

    retval = RemoteRedrawRectangle(
                 Left,
                 Top,
                 Right,
                 Bottom);

    TRACE("NtUserRemoteRedrawRectangle");
    ENDRECVCSRSS();
}


NTSTATUS
NtUserRemoteRedrawScreen(
    VOID)
{
    BEGINRECVCSRSS(NTSTATUS, STATUS_UNSUCCESSFUL);
    if (!ISTS()) {
        errret = STATUS_ACCESS_DENIED;
        MSGERROR(0);
    }

    /*
     * If there are any shadow connections, or it's not disconnected
     */
    if (gnShadowers > 0 || gbConnected)
        retval = RemoteRedrawScreen();
    else
        retval = STATUS_UNSUCCESSFUL;

    TRACE("NtUserRemoteRedrawScreen");
    ENDRECVCSRSS();
}


NTSTATUS
NtUserRemoteStopScreenUpdates(
    VOID)
{
    BEGINRECVCSRSS(NTSTATUS, STATUS_UNSUCCESSFUL);
    if (!ISTS()) {
        errret = STATUS_ACCESS_DENIED;
        MSGERROR(0);
    }

    retval = xxxRemoteStopScreenUpdates(TRUE);

    TRACE("NtUserRemoteStopScreenUpdates");
    ENDRECVCSRSS();
}

NTSTATUS
NtUserCtxDisplayIOCtl(
    IN ULONG  DisplayIOCtlFlags,
    IN PUCHAR pDisplayIOCtlData,
    IN ULONG  cbDisplayIOCtlData)
{
    BEGINRECVCSRSS(NTSTATUS, STATUS_UNSUCCESSFUL);
    if (!ISTS()) {
        errret = STATUS_ACCESS_DENIED;
        MSGERROR(0);
    }

    /*
     * Probe all read arguments
     */
#if DBG
    ProbeForRead(pDisplayIOCtlData, cbDisplayIOCtlData, sizeof(BYTE));
#endif

    retval = CtxDisplayIOCtl(
                 DisplayIOCtlFlags,
                 pDisplayIOCtlData,
                 cbDisplayIOCtlData);

    TRACE("NtUserCtxDisplayIOCtl");
    ENDRECVCSRSS();

}

UINT NtUserHardErrorControl(
    IN HARDERRORCONTROL dwCmd,
    IN HANDLE handle,
    OUT PDESKRESTOREDATA pdrdRestore OPTIONAL)
{
    BEGINRECVCSRSS(BOOL, HEC_ERROR);

#if DBG
    if (ARGUMENT_PRESENT(pdrdRestore)) {
        ProbeForWrite(pdrdRestore, sizeof(DESKRESTOREDATA), sizeof(DWORD));
    }
#endif

    retval = xxxHardErrorControl(dwCmd, handle, pdrdRestore);

    TRACE("NtUserHardErrorControl");
    ENDRECVCSRSS();
}

BOOL NtUserGetObjectInformation(  // API GetUserObjectInformationA/W
    IN HANDLE hObject,
    IN int nIndex,
    OUT PVOID pvInfo,
    IN DWORD nLength,
    OUT OPTIONAL LPDWORD pnLengthNeeded)
{
    DWORD dwAlign;
    DWORD dwLocalLength;
    BEGINATOMICRECV(BOOL, FALSE);

    /*
     * Probe arguments
     */
    try {
#if defined(_X86_)
        dwAlign = sizeof(BYTE);
#else
        if (nIndex == UOI_FLAGS)
            dwAlign = sizeof(DWORD);
        else
            dwAlign = sizeof(WCHAR);
#endif
        ProbeForWrite(pvInfo, nLength, dwAlign);
        if (ARGUMENT_PRESENT(pnLengthNeeded))
            ProbeForWriteUlong(pnLengthNeeded);

    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    /*
     * Make sure the handle doesn't get closed while we're playing with it.
     */
    SetHandleInUse(hObject);

    /*
     * pvInfo is client-side.  GetUserObjectInformation
     * protects use of this pointer with try blocks.
     */

    retval = _GetUserObjectInformation(hObject,
            nIndex, pvInfo,
            nLength, &dwLocalLength);

    /*
     * OK, we're done with the handle.
     */
    SetHandleInUse(NULL);

    if (ARGUMENT_PRESENT(pnLengthNeeded)) {
        try {
            *pnLengthNeeded = dwLocalLength;
        } except (StubExceptionHandler(TRUE)) {
            MSGERROR(0);
        }
    }

    TRACE("NtUserGetObjectInformation");
    ENDATOMICRECV();
}

BOOL NtUserWin32PoolAllocationStats(
    IN  LPDWORD parrTags,
    IN  SIZE_T  tagsCount,
    OUT SIZE_T* lpdwMaxMem,
    OUT SIZE_T* lpdwCrtMem,
    OUT LPDWORD lpdwMaxAlloc,
    OUT LPDWORD lpdwCrtAlloc)
{
#ifdef POOL_INSTR_API
    SIZE_T  dwMaxMem, dwCrtMem;
    DWORD   dwMaxAlloc, dwCrtAlloc;

    BEGINRECV(BOOL, FALSE);

    retval = _Win32PoolAllocationStats(parrTags,
                                       tagsCount,
                                       &dwMaxMem,
                                       &dwCrtMem,
                                       &dwMaxAlloc,
                                       &dwCrtAlloc);
    /*
     * Probe and copy
     */
    try {
        if (lpdwMaxMem != NULL) {
            ProbeForWrite(lpdwMaxMem, sizeof(SIZE_T), sizeof(DWORD));
            *lpdwMaxMem = dwMaxMem;
        }
        if (lpdwCrtMem != NULL) {
            ProbeForWrite(lpdwCrtMem, sizeof(SIZE_T), sizeof(DWORD));
            *lpdwCrtMem = dwCrtMem;
        }
        if (lpdwMaxAlloc != NULL) {
            ProbeForWrite(lpdwMaxAlloc, sizeof(DWORD), sizeof(DWORD));
            *lpdwMaxAlloc = dwMaxAlloc;
        }
        if (lpdwCrtAlloc != NULL) {
            ProbeForWrite(lpdwCrtAlloc, sizeof(DWORD), sizeof(DWORD));
            *lpdwCrtAlloc = dwCrtAlloc;
        }

    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    TRACE("NtUserWin32PoolAllocationStats");
    ENDRECV();
#else
    UNREFERENCED_PARAMETER(parrTags);
    UNREFERENCED_PARAMETER(tagsCount);
    UNREFERENCED_PARAMETER(lpdwMaxMem);
    UNREFERENCED_PARAMETER(lpdwCrtMem);
    UNREFERENCED_PARAMETER(lpdwMaxAlloc);
    UNREFERENCED_PARAMETER(lpdwCrtAlloc);
    return FALSE;
#endif // POOL_INSTR_API
}

#if DBG

VOID NtUserDbgWin32HeapFail( // private DbgWin32HeapFail
    IN DWORD dwFlags,
    IN BOOL  bFail)
{
    BEGINRECV_VOID();

    if ((dwFlags | WHF_VALID) != WHF_VALID) {
        RIPMSG1(RIP_WARNING, "Invalid flags for DbgWin32HeapFail %x", dwFlags);
        MSGERROR_VOID();
    }

    Win32HeapFailAllocations(bFail);

    TRACEVOID("NtUserDbgWin32HeapFail");
    ENDRECV_VOID();
}

DWORD  NtUserDbgWin32HeapStat( // private DbgWin32HeapStat
    PDBGHEAPSTAT phs,
    DWORD dwLen)
{
    extern DWORD Win32HeapStat(PDBGHEAPSTAT phs, DWORD dwLen, BOOL bTagsAreShift);

    DBGHEAPSTAT localHS[30];
    BEGINRECV(DWORD, 0);

    LIMITVALUE(dwLen, sizeof(localHS), "DbgWin32HeapStat");

    try{
        ProbeForRead(phs, dwLen, CHARALIGN);
        RtlCopyMemory(localHS, phs, dwLen);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = Win32HeapStat(localHS, dwLen, FALSE);

    try {
        ProbeForWrite(phs, dwLen, CHARALIGN);
        RtlCopyMemory(phs, localHS, dwLen);
    } except (StubExceptionHandler(FALSE)) {
    }
    TRACE("NtUserDbgWin32HeapStat");
    ENDRECV();
}
#endif // DBG

BOOL NtUserSetObjectInformation(  // API SetUserObjectInformationA/W
    IN HANDLE hObject,
    IN int nIndex,
    IN PVOID pvInfo,
    IN DWORD nLength)
{
    BEGINATOMICRECV(BOOL, FALSE);

    /*
     * Probe arguments
     */
    try {
        ProbeForRead(pvInfo, nLength, DATAALIGN);

    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    /*
     * Make sure the handle doesn't get closed while we're playing with it.
     */
    SetHandleInUse(hObject);

    /*
     * pvInfo is client-side.  SetUserObjectInformation
     * protects use of this pointer with try blocks.
     */
    retval = _SetUserObjectInformation(hObject,
                nIndex, pvInfo, nLength);

    /*
     * OK, we're done with the handle.
     */
    SetHandleInUse(NULL);

    TRACE("NtUserSetObjectInformation");
    ENDATOMICRECV();
}

NTSTATUS NtUserConsoleControl(  // private NtUserConsoleControl
    IN CONSOLECONTROL ConsoleCommand,
    IN PVOID ConsoleInformation,
    IN DWORD ConsoleInformationLength)
{
    BEGINRECVCSRSS(NTSTATUS, STATUS_UNSUCCESSFUL);

#if DBG
    ProbeForWrite(ConsoleInformation,
                  ConsoleInformationLength,
                  sizeof(WORD));
#endif

    retval = xxxConsoleControl(ConsoleCommand,
                                ConsoleInformation,
                                ConsoleInformationLength);

    TRACE("NtUserConsoleControl");
    ENDRECVCSRSS();
}

HWINSTA NtUserCreateWindowStation(  // API CreateWindowStationA/W
    IN POBJECT_ATTRIBUTES pObja,
    IN ACCESS_MASK        amRequest,
    IN HANDLE             hKbdLayoutFile,
    IN DWORD              offTable,
    IN PUNICODE_STRING    pstrKLID,
    UINT                  uKbdInputLocale)

{
    NTSTATUS                    Status;
    OBJECT_ATTRIBUTES           CapturedAttributes;
    SECURITY_QUALITY_OF_SERVICE qosCaptured;
    PSECURITY_DESCRIPTOR        psdCaptured = NULL;
    LUID                        luidService;
    UNICODE_STRING              strWinSta;
    UNICODE_STRING              strKLID;
    WCHAR                       awchName[MAX_SESSION_PATH];
    WCHAR                       awchKF[sizeof(((PKL)0)->spkf->awchKF)];
    UINT                        chMax;
    KPROCESSOR_MODE             OwnershipMode;

    BEGINRECV(HWINSTA, NULL);

    /*
     * Set status so we can clean up in case of failure
     */
    Status = STATUS_SUCCESS;

    try {
        /*
         * Probe and capture the ??? string
         */
        strKLID = ProbeAndReadUnicodeString(pstrKLID);
        ProbeForReadUnicodeStringBuffer(strKLID);
        chMax = min(sizeof(awchKF) - sizeof(WCHAR), strKLID.Length) / sizeof(WCHAR);
        wcsncpycch(awchKF, strKLID.Buffer, chMax);
        awchKF[chMax] = 0;

        /*
         * Probe and capture the object attributes
         */
        CapturedAttributes = ProbeAndReadStructure(pObja, OBJECT_ATTRIBUTES);

        /*
         * Probe and capture all other components of the object attributes.
         */
        if (CapturedAttributes.ObjectName == NULL && CapturedAttributes.RootDirectory == NULL) {

            /*
             * Use the logon authentication id to form the windowstation
             * name.
             */
            Status = GetProcessLuid(NULL, &luidService);
            if (NT_SUCCESS(Status)) {
                swprintf(awchName, L"%ws\\Service-0x%x-%x$",
                        szWindowStationDirectory,
                        luidService.HighPart, luidService.LowPart);
                RtlInitUnicodeString(&strWinSta, awchName);
                CapturedAttributes.ObjectName = &strWinSta;
            }
            OwnershipMode = KernelMode;
        } else {
            strWinSta = ProbeAndReadUnicodeString(CapturedAttributes.ObjectName);
            ProbeForReadUnicodeStringBuffer(strWinSta);
            /*
             * Use the StaticUnicodeBuffer on the TEB as the buffer for the object name.
             * Even if this is client side and we pass KernelMode to the Ob call in
             * _OpenWindowStation this is safe because the TEB is not going to go away
             * during this call. The worst it can happen is to have the buffer in the TEB
             * trashed.
             */
            strWinSta.Length = min(strWinSta.Length, STATIC_UNICODE_BUFFER_LENGTH * sizeof(WCHAR));

            RtlCopyMemory(NtCurrentTeb()->StaticUnicodeBuffer,
                          strWinSta.Buffer,
                          strWinSta.Length);

            strWinSta.Buffer = NtCurrentTeb()->StaticUnicodeBuffer;
            CapturedAttributes.ObjectName = &strWinSta;
            OwnershipMode = UserMode;
        }

        if (CapturedAttributes.SecurityQualityOfService) {
            PSECURITY_QUALITY_OF_SERVICE pqos;

            pqos = CapturedAttributes.SecurityQualityOfService;
            qosCaptured = ProbeAndReadStructure(pqos, SECURITY_QUALITY_OF_SERVICE);
            CapturedAttributes.SecurityQualityOfService = &qosCaptured;
        }

        if (NT_SUCCESS(Status) && CapturedAttributes.SecurityDescriptor != NULL) {
            Status = SeCaptureSecurityDescriptor(
                    CapturedAttributes.SecurityDescriptor,
                    UserMode,
                    PagedPool,
                    FALSE,
                    &psdCaptured);
            if (!NT_SUCCESS(Status)) {
                psdCaptured = NULL;
            }
            CapturedAttributes.SecurityDescriptor = psdCaptured;
        }

    } except (StubExceptionHandler(FALSE)) {
        Status = GetExceptionCode();
    }
    if (!NT_SUCCESS(Status))
        MSGERRORCLEANUP(ERROR_INVALID_PARAMETER);

    /*
     * Create the windowstation and return a kernel handle.
     */
    retval = xxxCreateWindowStation(&CapturedAttributes,
                                    OwnershipMode,
                                    amRequest,
                                    hKbdLayoutFile,
                                    offTable,
                                    awchKF,
                                    uKbdInputLocale);

    CLEANUPRECV();

    /*
     * Release captured security descriptor.
     */
    if (psdCaptured != NULL) {
        SeReleaseSecurityDescriptor(
                psdCaptured,
                UserMode,
                FALSE);
    }

    TRACE("NtUserCreateWindowStation");
    ENDRECV();
}


HWINSTA NtUserOpenWindowStation(
    IN OUT POBJECT_ATTRIBUTES pObja,
    IN ACCESS_MASK amRequest)
{
    NTSTATUS                    Status;
    LUID                        luidService;
    WCHAR                       awchName[sizeof(L"Service-0x0000-0000$") / sizeof(WCHAR)];
    OBJECT_ATTRIBUTES           Obja;
    UNICODE_STRING              ObjectName;

    BEGINRECV(HWINSTA, NULL);

    retval = NULL;

    /*
     * NT Bug 387849: We want to allow the caller to pass in a "template" to be 
     * filled in for the Service windowstation.  So, we need to walk through the
     * pObja structure and check the string out, replacing it with the real
     * object name if necessary.
     *
     * It is VERY important that we pass the pObja object to the executive
     * (through _OpenWindowStation) and not the Obja object.  This is because we
     * will request UserMode for this object, and the executive will check that
     * it is actually getting a user-mode address.
     *
     * We will still make a copy of the structures to manipulate while we are 
     * walking everything so that we don't need to worry about the rug being
     * removed from beneath us.  The executive will do its own checking.
     */

    try {
        /*
         * Probe the object attributes.  We need to be able to read the
         * OBJECT_ATTRIBUTES and to write the ObjectName (UNICODE_STRING).
         */
        Obja = ProbeAndReadStructure(pObja, OBJECT_ATTRIBUTES);

        ProbeForWrite(Obja.ObjectName, sizeof(*(Obja.ObjectName)), DATAALIGN);

        ObjectName = ProbeAndReadUnicodeString(Obja.ObjectName);

        /*
         * If we are trying to open the NULL or "" WindowStation, remap this
         * benign name to Service-0x????-????$.
         */
        if (Obja.RootDirectory != NULL &&
                ObjectName.Buffer != NULL &&
                ObjectName.MaximumLength == sizeof(awchName) &&
                ObjectName.Length == (sizeof(awchName) - sizeof(UNICODE_NULL))) {

            /*
             * Use the logon authentication id to form the windowstation
             * name.  Put this in the user's buffer since we were the one
             * who allocated it in OpenWindowStation.
             */

            ProbeForWrite(ObjectName.Buffer, ObjectName.MaximumLength, CHARALIGN);

            if (!_wcsicmp(ObjectName.Buffer, L"Service-0x0000-0000$")) {
                Status = GetProcessLuid(NULL, &luidService);
                if (NT_SUCCESS(Status)) {
                    swprintf(ObjectName.Buffer,
                              L"Service-0x%x-%x$",
                              luidService.HighPart,
                              luidService.LowPart);
                    /*
                     * We need to re-initialize the string to get the counted
                     * length correct.  Otherwise the hashing function used
                     * by ObpLookupDirectoryEntry will fail.
                     */
                    RtlInitUnicodeString(Obja.ObjectName, ObjectName.Buffer);
                }
            }
        }

    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    /*
     * Open the WindowStation
     */
    retval = _OpenWindowStation(pObja, amRequest, UserMode);

    TRACE("NtUserOpenWindowStation");
    ENDRECV();

    UNREFERENCED_PARAMETER(awchName);
}

BOOL NtUserCloseWindowStation(
    IN HWINSTA hwinsta)
{
    PWINDOWSTATION pwinsta;
    NTSTATUS Status;

    BEGINRECV(BOOL, FALSE);

    retval = FALSE;

    Status = ValidateHwinsta(hwinsta, UserMode, 0, &pwinsta);
    if (NT_SUCCESS(Status)) {
        retval = _CloseWindowStation(hwinsta);
        ObDereferenceObject(pwinsta);
    }

    TRACE("NtUserCloseWindowStation");
    ENDRECV();
}


BOOL NtUserSetProcessWindowStation(
    IN HWINSTA hwinsta)
{
    BEGINRECV(BOOL, FALSE);

    retval = xxxSetProcessWindowStation(hwinsta, UserMode);

    TRACE("NtUserSetProcessWindowStation");
    ENDRECV();
}

HWINSTA NtUserGetProcessWindowStation(
    VOID)
{
    BEGINRECV_SHARED(HWINSTA, NULL);

    _GetProcessWindowStation(&retval);

    TRACE("NtUserGetProcessWindowStation");
    ENDRECV_SHARED();
}

BOOL NtUserLockWorkStation(
    VOID)
{
    BEGINRECV(BOOL, FALSE);

    retval = _LockWorkStation();

    TRACE("NtUserLockWorkStation");
    ENDRECV();
}


HDESK NtUserCreateDesktop(  // API CreateDesktopA/W
    IN POBJECT_ATTRIBUTES pObja,
    IN PUNICODE_STRING pstrDevice,
    IN LPDEVMODEW pDevmode,
    IN DWORD dwFlags,
    IN ACCESS_MASK amRequest)
{
    BEGINRECV(HDESK, NULL);

    /*
     * Fail this call for restricted threads
     */
    if (IS_CURRENT_THREAD_RESTRICTED(JOB_OBJECT_UILIMIT_DESKTOP)) {
        RIPMSG0(RIP_WARNING, "NtUserCreateDesktop failed for restricted thread");
        MSGERROR(0);
    }

    /*
     * Probe arguments
     */
    try {
        ProbeForRead(pObja, sizeof(*pObja), sizeof(DWORD));
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    /*
     * pObja, pDevmode, and pstrDevice are all client side addresses.
     *
     * pstrDevice and pDevmode are put into the Context info,
     * and they are used in ntgdi\dre\drvsup.cxx,
     * where thay are captured before use.

     */

    retval = xxxCreateDesktop(pObja,
                              UserMode,
                              pstrDevice,
                              pDevmode,
                              dwFlags,
                              amRequest);

    TRACE("NtUserCreateDesktop");
    ENDRECV();
}

HDESK NtUserOpenDesktop(
    IN POBJECT_ATTRIBUTES pObja,
    IN DWORD dwFlags,
    IN ACCESS_MASK amRequest)
{
    BOOL bShutDown;

    BEGINRECV(HDESK, NULL);

    retval = xxxOpenDesktop(pObja, UserMode, dwFlags, amRequest, &bShutDown);

    TRACE("NtUserOpenDesktop");
    ENDRECV();
}


HDESK NtUserOpenInputDesktop(
    IN DWORD dwFlags,
    IN BOOL fInherit,
    IN DWORD amRequest)
{
    HWINSTA        hwinsta;
    PWINDOWSTATION pwinsta;
    PDESKTOP       pdesk;
    NTSTATUS       Status;

    BEGINRECV(HDESK, NULL);

    if (grpdeskRitInput == NULL) {
        MSGERROR(ERROR_OPEN_FAILED);
    } else {
        pwinsta = _GetProcessWindowStation(&hwinsta);
        if (pwinsta == NULL) {
            MSGERROR(ERROR_ACCESS_DENIED);
        }
        if (pwinsta->dwWSF_Flags & WSF_NOIO) {
            MSGERROR(ERROR_INVALID_FUNCTION);
        } else {

            /*
             * We should never return the 'Disconnect' desktop to
             * an app. We should return instead the desktop that we will
             * restore to from the Disconnect desktop.
             */
            
            pdesk = (gbDesktopLocked ? gspdeskShouldBeForeground : grpdeskRitInput);
            
            /*
             * Require read/write access
             */
            amRequest |= DESKTOP_READOBJECTS | DESKTOP_WRITEOBJECTS;

            Status = ObOpenObjectByPointer(
                    pdesk,
                    fInherit ? OBJ_INHERIT : 0,
                    NULL,
                    amRequest,
                    *ExDesktopObjectType,
                    UserMode,
                    &retval);
            if (NT_SUCCESS(Status)) {

                BOOL bShutDown;
                /*
                 * Complete the desktop open
                 */
                if (!OpenDesktopCompletion(pdesk, retval,
                    dwFlags, &bShutDown)) {

                    CloseProtectedHandle(retval);
                    retval = NULL;
                } else {
                    SetHandleFlag(retval, HF_PROTECTED, TRUE);
                }
            } else
                retval = NULL;
        }
    }

    TRACE("NtUserOpenInputDesktop");
    ENDRECV();
}


NTSTATUS NtUserResolveDesktopForWOW (  // WOW ResolveDesktopForWOW
    IN OUT PUNICODE_STRING pstrDesktop)
{
    UNICODE_STRING strDesktop, strDesktop2;
    PTHREADINFO ptiCurrent;
    TL tlBuffer;
    LPWSTR lpBuffer = NULL;
    BOOL fFreeBuffer = FALSE;
    BEGINRECV(NTSTATUS, STATUS_UNSUCCESSFUL);

    retval = STATUS_SUCCESS;

    ptiCurrent = PtiCurrent();
    /*
     * Probe arguments
     */
    try {
        strDesktop = ProbeAndReadUnicodeString(pstrDesktop);
        ProbeForReadUnicodeStringFullBuffer(strDesktop);
        RtlCopyMemory(&strDesktop2, &strDesktop, sizeof strDesktop);
        if (strDesktop.MaximumLength > 0) {
            PWSTR pszCapture = strDesktop.Buffer;
            strDesktop.Buffer = UserAllocPoolWithQuota(strDesktop.MaximumLength, TAG_TEXT2);
            if (strDesktop.Buffer) {
                fFreeBuffer = TRUE;
                ThreadLockPool(ptiCurrent, strDesktop.Buffer, &tlBuffer);
                RtlCopyMemory(strDesktop.Buffer, pszCapture, strDesktop.Length);
            } else
                ExRaiseStatus(STATUS_NO_MEMORY);

        } else {
            strDesktop.Buffer = NULL;
        }
    } except (StubExceptionHandler(FALSE)) {
        MSGERRORCLEANUP(0);
    }

    retval = xxxResolveDesktopForWOW(&strDesktop);

    if (NT_SUCCESS(retval)) {
        try {
            /*
             * The string structure at pstrDesktop could have changed
             * so we will ignore it and drop the one we have already
             * probed down on top of it.  We have already performed the
             * ResolveDesktopForWOW action, so we should not return an
             * error if this copy fails.
             */

            RtlCopyUnicodeString(&strDesktop2, &strDesktop);
            RtlCopyMemory(pstrDesktop, &strDesktop2, sizeof strDesktop2);
        } except (StubExceptionHandler(FALSE)) {
        }
    }

CLEANUPRECV();
    if (fFreeBuffer)
        ThreadUnlockAndFreePool(ptiCurrent, &tlBuffer);
    TRACE("NtUserResolveDesktopForWOW");
    ENDRECV();
}

HDESK NtUserResolveDesktop(
    IN HANDLE hProcess,
    IN PUNICODE_STRING pstrDesktop,
    IN BOOL fInherit,
    OUT HWINSTA *phwinsta)
{
    UNICODE_STRING strDesktop;
    HWINSTA hwinsta = NULL;
    PTHREADINFO pti;
    TL tlBuffer;
    BOOL fFreeBuffer = FALSE;
    BOOL bShutDown = FALSE;

    BEGINRECV(HDESK, NULL);

    pti = PtiCurrent();
    /*
     * Probe and capture desktop path
     */
    try {
        strDesktop = ProbeAndReadUnicodeString(pstrDesktop);
        if (strDesktop.Length > 0) {
            PWSTR pszCapture = strDesktop.Buffer;
            ProbeForReadUnicodeStringBuffer(strDesktop);
            strDesktop.Buffer = UserAllocPoolWithQuota(strDesktop.Length, TAG_TEXT2);
            if (strDesktop.Buffer) {
                fFreeBuffer = TRUE;
                ThreadLockPool(pti, strDesktop.Buffer, &tlBuffer);
                RtlCopyMemory(strDesktop.Buffer, pszCapture, strDesktop.Length);
            } else
                ExRaiseStatus(STATUS_NO_MEMORY);
        } else {
            strDesktop.Buffer = NULL;
        }
    } except (StubExceptionHandler(TRUE)) {
        MSGERRORCLEANUP(0);
    }

    retval = xxxResolveDesktop(hProcess, &strDesktop, &hwinsta,
            fInherit, &bShutDown);

    CLEANUPRECV();
    if (fFreeBuffer)
        ThreadUnlockAndFreePool(pti, &tlBuffer);

    try {
        ProbeAndWriteHandle((PHANDLE)phwinsta, hwinsta);
    } except (StubExceptionHandler(TRUE)) {
        xxxCloseDesktop(retval, KernelMode);
        if (hwinsta)
            _CloseWindowStation(hwinsta);
        MSGERROR(0);
    }

    TRACE("NtUserResolveDesktop");
    ENDRECV();
}

BOOL NtUserCloseDesktop(
    IN HDESK hdesk)
{
    BEGINRECV(BOOL, FALSE);

    retval = xxxCloseDesktop(hdesk, UserMode);

    TRACE("NtUserCloseDesktop");
    ENDRECV();
}

BOOL NtUserSetThreadDesktop(
    IN HDESK hdesk)
{
    PDESKTOP pdesk = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    BEGINRECV(BOOL, FALSE);
    Status = ValidateHdesk(hdesk, UserMode, 0, &pdesk);
    if (NT_SUCCESS(Status)) {
        retval = xxxSetThreadDesktop(hdesk, pdesk);
        LogDesktop(pdesk, LD_DEREF_VALIDATE_HDESK1, FALSE, (ULONG_PTR)PtiCurrent());
        ObDereferenceObject(pdesk);
    } else if (hdesk == NULL && PsGetCurrentProcess() == gpepCSRSS) {
        retval = xxxSetThreadDesktop(NULL, NULL);
    } else
        retval = FALSE;

    TRACE("NtUserSetThreadDesktop");
    ENDRECV();
}

HDESK NtUserGetThreadDesktop(
    IN DWORD dwThreadId,
    IN HDESK hdeskConsole)
{
    BEGINRECV_SHARED(HDESK, NULL);

    retval = xxxGetThreadDesktop(dwThreadId, hdeskConsole, UserMode);

    TRACE("NtUserGetThreadDesktop");
    ENDRECV_SHARED();
}

BOOL NtUserSwitchDesktop(
    IN HDESK hdesk)
{
    PDESKTOP pdesk;
    TL tlpdesk;
    PTHREADINFO ptiCurrent;
    NTSTATUS Status;

    BEGINRECV(BOOL, FALSE);

    ptiCurrent = PtiCurrent();

    /*
     * Fail this call for restricted threads
     */
    if (IS_CURRENT_THREAD_RESTRICTED(JOB_OBJECT_UILIMIT_DESKTOP)) {
        RIPMSG0(RIP_WARNING, "NtUserSwitchDesktop failed for restricted thread");
        MSGERROR(0);
    }

    Status = ValidateHdesk(hdesk, UserMode, DESKTOP_SWITCHDESKTOP, &pdesk);
    if (NT_SUCCESS(Status)) {
        if (pdesk->rpwinstaParent->dwWSF_Flags & WSF_NOIO) {
            LogDesktop(pdesk, LD_DEREF_VALIDATE_HDESK2, FALSE, (ULONG_PTR)PtiCurrent());
            ObDereferenceObject(pdesk);
            RIPERR0(ERROR_ACCESS_DENIED, RIP_VERBOSE, "");
            retval = FALSE;
        } else {
            ThreadLockDesktop(ptiCurrent, pdesk, &tlpdesk, LDLT_FN_NTUSERSWITCHDESKTOP);
            LogDesktop(pdesk, LD_DEREF_VALIDATE_HDESK3, FALSE, (ULONG_PTR)PtiCurrent());
            ObDereferenceObject(pdesk);
            retval = xxxSwitchDesktop(NULL, pdesk, FALSE);
            ThreadUnlockDesktop(ptiCurrent, &tlpdesk, LDUT_FN_NTUSERSWITCHDESKTOP);
        }
    } else
        retval = FALSE;

    TRACE("NtUserSwitchDesktop");
    ENDRECV();
}

NTSTATUS NtUserInitializeClientPfnArrays(  // private
    IN CONST PFNCLIENT *ppfnClientA OPTIONAL,
    IN CONST PFNCLIENT *ppfnClientW OPTIONAL,
    IN CONST PFNCLIENTWORKER *ppfnClientWorker OPTIONAL,
    IN HANDLE hModUser)
{
    BEGINRECV(NTSTATUS, STATUS_UNSUCCESSFUL);

    /*
     * Probe all read arguments
     */
    try {
        if (ARGUMENT_PRESENT(ppfnClientA)) {
            ProbeForRead(ppfnClientA, sizeof(*ppfnClientA), sizeof(DWORD));
        }
        if (ARGUMENT_PRESENT(ppfnClientW)) {
            ProbeForRead(ppfnClientW, sizeof(*ppfnClientW), sizeof(DWORD));
        }

        if (ARGUMENT_PRESENT(ppfnClientWorker)) {
            ProbeForRead(ppfnClientWorker, sizeof(*ppfnClientWorker), sizeof(DWORD));
        }

        retval = InitializeClientPfnArrays(
                ppfnClientA, ppfnClientW, ppfnClientWorker, hModUser);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    TRACE("NtUserInitializeThreadInfo");
    ENDRECV();
}

BOOL NtUserWaitForMsgAndEvent(
    IN HANDLE hevent)
{
    BEGINRECV(BOOL, FALSE);

    retval = xxxSleepTask(FALSE, hevent);

    TRACE("NtUserWaitForMsgAndEvent");
    ENDRECV();
}

DWORD NtUserDragObject(
    IN HWND hwndParent,
    IN HWND hwndFrom,
    IN UINT wFmt,
    IN ULONG_PTR dwData,
    IN HCURSOR hcur)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    PWND pwndFrom;
    PCURSOR pcur;
    TL tlpwndFrom;
    TL tlpcur;

    BEGINRECV_HWNDLOCK(DWORD, 0, hwndParent);

    ValidateHWNDOPT(pwndFrom, hwndFrom);
    ValidateHCURSOROPT(pcur, hcur);

    ThreadLockWithPti(ptiCurrent, pwndFrom, &tlpwndFrom);
    ThreadLockWithPti(ptiCurrent, pcur, &tlpcur);

    retval = xxxDragObject(
            pwnd,
            pwndFrom,
            wFmt,
            dwData,
            pcur);

    ThreadUnlock(&tlpcur);
    ThreadUnlock(&tlpwndFrom);

    TRACE("NtUserDragObject");
    ENDRECV_HWNDLOCK();
}

BOOL NtUserGetIconInfo(  // API GetIconInfo
    IN  HICON hIcon,
    OUT PICONINFO piconinfo,
    IN OUT OPTIONAL PUNICODE_STRING pstrInstanceName,
    IN OUT OPTIONAL PUNICODE_STRING pstrResName,
    OUT LPDWORD pbpp,
    IN  BOOL fInternal)
{
    PICON pIcon;
    UNICODE_STRING strInstanceName, *pstrInstanceLocal;
    UNICODE_STRING strResName, *pstrResLocal;

    BEGINATOMICRECV(BOOL, FALSE);

    /*
     * NOTE -- this can't be _SHARED since it calls Gre code with system HDC's.
     */

    ValidateHCURSOR(pIcon, hIcon);

    /*
     * Probe arguments
     */
    try {
        if (pstrInstanceName != NULL) {
            strInstanceName = ProbeAndReadUnicodeString(pstrInstanceName);
            ProbeForWrite(strInstanceName.Buffer, strInstanceName.MaximumLength, CHARALIGN);
            pstrInstanceLocal = &strInstanceName;
        } else {
            pstrInstanceLocal = NULL;
        }

        if (pstrResName != NULL) {
            strResName = ProbeAndReadUnicodeString(pstrResName);
            ProbeForWrite(strResName.Buffer, strResName.MaximumLength, CHARALIGN);
            pstrResLocal = &strResName;
        } else {
            pstrResLocal = NULL;
        }

        if (pbpp != NULL) {
            ProbeForWrite(pbpp, sizeof(DWORD), sizeof(DWORD));
        }
        ProbeForWrite(piconinfo, sizeof(*piconinfo), DATAALIGN);
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    /*
     * All use of client-side pointers in InternalGetIconInfo
     * is protected by try/except.
     */

    retval = _InternalGetIconInfo(
            pIcon,
            piconinfo,
            pstrInstanceLocal,
            pstrResLocal,
            pbpp,
            fInternal);

    try {
        if (pstrInstanceName != NULL) {
            RtlCopyMemory(pstrInstanceName, pstrInstanceLocal, sizeof(UNICODE_STRING));
        }
        if (pstrResName != NULL) {
            RtlCopyMemory(pstrResName, pstrResLocal, sizeof(UNICODE_STRING));
        }
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    TRACE("NtUserGetIconInfo");
    ENDATOMICRECV();
}

BOOL NtUserGetIconSize(  // private
    IN HICON hIcon,
    IN UINT istepIfAniCur,
    OUT int *pcx,
    OUT int *pcy)
{
    PCURSOR picon;

    BEGINRECV_SHARED(BOOL, FALSE);

    ValidateHICON(picon, hIcon);

    if (picon->CURSORF_flags & CURSORF_ACON) {
        PACON pacon = (PACON)picon;
        if (istepIfAniCur < (UINT)pacon->cpcur) {
            picon = pacon->aspcur[pacon->aicur[istepIfAniCur]];
        } else {
            RIPMSG2(RIP_WARNING, "NtUserGetIconSize: Invalid istepIfAniCur:%#lx. picon:%#p",
                    istepIfAniCur, picon);
            MSGERROR(0);
        }
    }

    /*
     * Probe arguments
     */
    try {
        ProbeAndWriteLong(pcx, picon->cx);
        ProbeAndWriteLong(pcy, picon->cy);

        retval = 1;
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    TRACE("NtUserGetIconSize");
    ENDRECV_SHARED();
}



BOOL NtUserDrawIconEx(  // API DrawIconEx
    IN HDC hdc,
    IN int x,
    IN int y,
    IN HICON hicon,
    IN int cx,
    IN int cy,
    IN UINT istepIfAniCur,
    IN HBRUSH hbrush,
    IN UINT diFlags,
    IN BOOL fMeta,
    OUT DRAWICONEXDATA *pdid)
{
    PCURSOR picon;

    BEGINATOMICRECV(BOOL, FALSE);

    ValidateHICON(picon, hicon);

    if (fMeta) {
        if (picon->CURSORF_flags & CURSORF_ACON)
            picon = ((PACON)picon)->aspcur[((PACON)picon)->aicur[0]];

        /*
         * Probe arguments
         */
        try {
            ProbeForWrite(pdid, sizeof(*pdid), DATAALIGN);

            pdid->hbmMask  = picon->hbmMask;
            pdid->hbmColor = picon->hbmColor;

            pdid->cx = cx ? cx : picon->cx ;
            pdid->cy = cy ? cy : picon->cy ;

            retval = 1;
        } except (StubExceptionHandler(TRUE)) {
            MSGERROR(0);
        }

    } else {
        retval = _DrawIconEx(hdc, x, y, picon,
                            cx, cy,
                            istepIfAniCur, hbrush,
                            diFlags );
    }

    TRACE("NtUserDrawIconEx");
    ENDATOMICRECV();
}

HANDLE NtUserDeferWindowPos(
    IN HDWP hWinPosInfo,
    IN HWND hwnd,
    IN HWND hwndInsertAfter,
    IN int x,
    IN int y,
    IN int cx,
    IN int cy,
    IN UINT wFlags)
{
    PWND pwnd;
    PWND pwndInsertAfter;
    PSMWP psmwp;

    BEGINATOMICRECV(HANDLE, NULL);

    TESTFLAGS(wFlags, SWP_VALID);

    ValidateHWNDND(pwnd, hwnd);
    ValidateHWNDIA(pwndInsertAfter, hwndInsertAfter);
    ValidateHDWP(psmwp, hWinPosInfo);

    if (wFlags & ~(SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER |
            SWP_NOREDRAW | SWP_NOACTIVATE | SWP_FRAMECHANGED |
            SWP_SHOWWINDOW | SWP_HIDEWINDOW | SWP_NOCOPYBITS |
            SWP_NOOWNERZORDER)) {
        RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "Invalid flags (0x%lx) passed to DeferWindowPos",
                wFlags);
        MSGERROR(0);
    }

    /*
     * Make sure the window coordinates can fit in WORDs.
     */
    if (!(wFlags & SWP_NOMOVE)) {
        if (x > SHRT_MAX) {
            x = SHRT_MAX;
        } else if (x < SHRT_MIN) {
            x = SHRT_MIN;
        }
        if (y > SHRT_MAX) {
            y = SHRT_MAX;
        } else if (y < SHRT_MIN) {
            y = SHRT_MIN;
        }
    }

    /*
     * Actually, if we were going to be really strict about this we'd
     * make sure that x + cx < SHRT_MAX, etc but since we do maintain
     * signed 32-bit coords internally this case doesn't cause a problem.
     */
    if (!(wFlags & SWP_NOSIZE)) {
        if (cx < 0) {
            cx = 0;
        } else if (cx > SHRT_MAX) {
            cx = SHRT_MAX;
        }
        if (cy < 0) {
            cy = 0;
        } else if (cy > SHRT_MAX) {
            cy = SHRT_MAX;
        }
    }

#ifdef NEVER
//
// do not fail these conditions because real apps use them.
//
    if (!(wFlags & SWP_NOMOVE) &&
            (x > SHRT_MAX || x < SHRT_MIN ||
             y > SHRT_MAX || y < SHRT_MIN)) {
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "Invalid coordinate passed to SetWindowPos");
        MSGERROR(0);
    }

    /*
     * Actually, if we were going to be really strict about this we'd
     * make sure that x + cx < SHRT_MAX, etc but since we do maintain
     * signed 32-bit coords internally this case doesn't cause a problem.
     */
    if (!(wFlags & SWP_NOSIZE) &&
            (cx < 0 || cx > SHRT_MAX ||
             cy < 0 || cy > SHRT_MAX)) {
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "Invalid width/height passed to SetWindowPos");
        MSGERROR(0);
    }
#endif

    retval = _DeferWindowPos(
            psmwp,
            pwnd,
            pwndInsertAfter,
            x,
            y,
            cx,
            cy,
            wFlags);
    retval = PtoH((PVOID)retval);

    TRACE("NtUserDeferWindowPos");
    ENDATOMICRECV();
}

BOOL NtUserEndDeferWindowPosEx(
    IN HDWP hWinPosInfo,
    IN BOOL fAsync)
{
    PSMWP psmwp;
    TL tlpsmp;

    BEGINRECV(BOOL, FALSE);

    ValidateHDWP(psmwp, hWinPosInfo);

    ThreadLockAlways(psmwp, &tlpsmp);

    retval = xxxEndDeferWindowPosEx(
            psmwp,
            fAsync);

    ThreadUnlock(&tlpsmp);

    TRACE("NtUserEndDeferWindowPosEx");
    ENDRECV();
}

BOOL NtUserGetMessage(  // API GetMessageA/W
    OUT LPMSG pmsg,
    IN HWND hwnd,
    IN UINT wMsgFilterMin,
    IN UINT wMsgFilterMax)
{
    MSG msg;

    BEGINRECV(BOOL, FALSE);

    retval = xxxGetMessage(
            &msg,
            hwnd,
            wMsgFilterMin,
            wMsgFilterMax);

    /*
     * Probe arguments
     */
    try {
        ProbeAndWriteStructure(pmsg, msg, MSG);
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    TRACE("NtUserGetMessage");
    ENDRECV();
}

BOOL NtUserMoveWindow(
    IN HWND hwnd,
    IN int x,
    IN int y,
    IN int cx,
    IN int cy,
    IN BOOL fRepaint)
{
    BEGINRECV_HWNDLOCK_ND(BOOL, FALSE, hwnd);

    /*
     * Make sure the window coordinates can fit in WORDs.
     */
    if (x > SHRT_MAX) {
        x = SHRT_MAX;
    } else if (x < SHRT_MIN) {
        x = SHRT_MIN;
    }
    if (y > SHRT_MAX) {
        y = SHRT_MAX;
    } else if (y < SHRT_MIN) {
        y = SHRT_MIN;
    }

    /*
     * Actually, if we were going to be really strict about this we'd
     * make sure that x + cx < SHRT_MAX, etc but since we do maintain
     * signed 32-bit coords internally this case doesn't cause a problem.
     */
    if (cx < 0) {
        cx = 0;
    } else if (cx > SHRT_MAX) {
        cx = SHRT_MAX;
    }
    if (cy < 0) {
        cy = 0;
    } else if (cy > SHRT_MAX) {
        cy = SHRT_MAX;
    }

#ifdef NEVER
//
// do not fail these conditions because real apps use them.
//
    if (x > SHRT_MAX || x < SHRT_MIN ||
            y > SHRT_MAX || y < SHRT_MIN) {
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "Invalid coordinate passed to MoveWindow");
        MSGERROR(0);
    }

    /*
     * Actually, if we were going to be really strict about this we'd
     * make sure that x + cx < SHRT_MAX, etc but since we do maintain
     * signed 32-bit coords internally this case doesn't cause a problem.
     */
    if (cx < 0 || cx > SHRT_MAX ||
            cy < 0 || cy > SHRT_MAX) {
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "Invalid width/height passed to MoveWindow");
        MSGERROR(0);
    }
#endif

    retval = xxxMoveWindow(
            pwndND,
            x,
            y,
            cx,
            cy,
            fRepaint);

    TRACE("NtUserMoveWindow");
    ENDRECV_HWNDLOCK_ND();
}

int NtUserTranslateAccelerator(  // API TranslateAcceleratorA/W
    IN HWND hwnd,
    IN HACCEL haccel,
    IN LPMSG lpmsg)
{
    PWND pwnd;
    LPACCELTABLE pat;
    TL tlpwnd;
    TL tlpat;
    PTHREADINFO ptiCurrent;
    MSG msg;

    BEGINRECV(int, 0);

    /*
     * Probe arguments
     */
    try {
        msg = ProbeAndReadMessage(lpmsg);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    /*
     * This is called within a message loop. If the window gets destroyed,
     * there still may be other messages in the queue that get returned
     * after the window is destroyed. The app will call TranslateAccelerator()
     * on every one of these, causing RIPs.... Make it nice so it just
     * returns FALSE.
     */
    ValidateHWNDNoRIP(pwnd, hwnd);
    ValidateHACCEL(pat, haccel);

    ptiCurrent = PtiCurrent();
    ThreadLockAlwaysWithPti(ptiCurrent, pwnd, &tlpwnd);
    ThreadLockAlwaysWithPti(ptiCurrent, pat, &tlpat);

    retval = xxxTranslateAccelerator(
            pwnd,
            pat,
            &msg);

    ThreadUnlock(&tlpat);
    ThreadUnlock(&tlpwnd);

    TRACE("NtUserTranslateAccelerator");
    ENDRECV();
}

LONG_PTR NtUserSetClassLongPtr(  // API SetClassLongPtrA/W
    IN  HWND hwnd,
    IN  int nIndex,
    OUT LONG_PTR dwNewLong,
    IN  BOOL bAnsi)
{
    UNICODE_STRING strMenuName;
    CLSMENUNAME cmn, *pcmnSave;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_HWNDLOCK(ULONG_PTR, 0, hwnd);

    switch (nIndex) {
    case GCLP_MENUNAME:
        try {
            /*
             *  There is no callback from the routine for
             *  this value, so we can protect it with a try/except.
             *  This is cheaper than  capturing the name
             *  and copying it back.  FritzS
             */

            pcmnSave = (PCLSMENUNAME) dwNewLong;
            cmn = ProbeAndReadStructure(pcmnSave, CLSMENUNAME);
            strMenuName = ProbeAndReadUnicodeString(cmn.pusMenuName);
            ProbeForReadUnicodeStringBufferOrId(strMenuName);
        } except (StubExceptionHandler(TRUE)) {
            MSGERROR(0);
        }
        cmn.pusMenuName = &strMenuName;
        dwNewLong = (ULONG_PTR) &cmn;
        retval = xxxSetClassLongPtr(
            pwnd,
            nIndex,
            dwNewLong,
            bAnsi);
        try {
            ProbeAndWriteStructure(pcmnSave, cmn, CLSMENUNAME);
        } except (StubExceptionHandler(TRUE)) {
            MSGERROR(0);
        }
        break;

    case GCL_STYLE:
        /*
         * I'm not sure how CS_VALID mask will affect existing apps,
         * so leave it for now --- except CS_IME flag, on which the system
         * deeply depends.
         */
#if DBG
        if (dwNewLong & ~CS_VALID) {
            RIPMSG1(RIP_WARNING, "NtUserSetClassLongPtr: Invalid style (%x) specified.", dwNewLong);
        }
#endif
        if (dwNewLong & CS_IME) {
           RIPERR1(ERROR_INVALID_DATA, RIP_VERBOSE, "NtUserSetClassLongPtr: CS_IME is specified in new style (%x).", dwNewLong);
           MSGERROR(0);
        }
    default:
        retval = xxxSetClassLongPtr(
                pwnd,
                nIndex,
                dwNewLong,
                bAnsi);

    }

    TRACE("NtUserSetClassLongPtr");
    ENDRECV_HWNDLOCK();
}

#ifdef _WIN64
LONG NtUserSetClassLong(
    IN  HWND hwnd,
    IN  int nIndex,
    OUT LONG dwNewLong,
    IN  BOOL bAnsi)
{
    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_HWNDLOCK(DWORD, 0, hwnd);

    switch (nIndex) {
    case GCL_STYLE:
        /*
         * I'm not sure how CS_VALID mask will affect existing apps,
         * so leave it for now --- except CS_IME flag, on which the system
         * deeply depends.
         */
#if DBG
        if (dwNewLong & ~CS_VALID) {
            RIPMSG1(RIP_WARNING, "NtUserSetClassLong: Invalid style (%x) specified.", dwNewLong);
        }
#endif
        if (dwNewLong & CS_IME) {
           RIPERR1(ERROR_INVALID_DATA, RIP_VERBOSE, "NtUserSetClassLong: CS_IME is specified in new style (%x).", dwNewLong);
           MSGERROR(0);
        }
    }

    retval = xxxSetClassLong(
            pwnd,
            nIndex,
            dwNewLong,
            bAnsi);

    TRACE("NtUserSetClassLong");
    ENDRECV_HWNDLOCK();
}
#endif

BOOL NtUserSetKeyboardState(  // API SetKeyboardState
    IN CONST BYTE *lpKeyState)
{
    BEGINRECV(BOOL, FALSE);

    /*
     * Probe arguments
     */
    try {
        ProbeForRead(lpKeyState, 256, sizeof(BYTE));

        retval = _SetKeyboardState(lpKeyState);
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    TRACE("NtUserSetKeyboardState");
    ENDRECV();
}

BOOL NtUserSetWindowPos(
    IN HWND hwnd,
    IN HWND hwndInsertAfter,
    IN int x,
    IN int y,
    IN int cx,
    IN int cy,
    IN UINT dwFlags)
{
    PWND        pwndT;
    PWND        pwndInsertAfter;
    TL          tlpwndT;

    BEGINRECV_HWNDLOCK_ND(BOOL, FALSE, hwnd);

    TESTFLAGS(dwFlags, SWP_VALID);

    ValidateHWNDIA(pwndInsertAfter, hwndInsertAfter);

    /*
     * Let's not allow the window to be shown/hidden once we
     * started the destruction of the window.
     */
    if (TestWF(pwndND, WFINDESTROY)) {
        RIPERR1(ERROR_INVALID_PARAMETER,
                RIP_WARNING,
                "SetWindowPos: Window is being destroyed (pwnd == %#p)",
                pwndND);
        MSGERROR(0);
    }

    if (dwFlags & ~SWP_VALID) {
        RIPERR1(ERROR_INVALID_PARAMETER,
                RIP_WARNING,
                "SetWindowPos: Invalid flags passed in (flags == 0x%lx)",
                dwFlags);
        MSGERROR(0);
    }

    /*
     * Make sure the window coordinates can fit in WORDs.
     */
    if (!(dwFlags & SWP_NOMOVE)) {
        if (x > SHRT_MAX) {
            x = SHRT_MAX;
        } else if (x < SHRT_MIN) {
            x = SHRT_MIN;
        }
        if (y > SHRT_MAX) {
            y = SHRT_MAX;
        } else if (y < SHRT_MIN) {
            y = SHRT_MIN;
        }
    }

    /*
     * Actually, if we were going to be really strict about this we'd
     * make sure that x + cx < SHRT_MAX, etc but since we do maintain
     * signed 32-bit coords internally this case doesn't cause a problem.
     */
    if (!(dwFlags & SWP_NOSIZE)) {
        if (cx < 0) {
            cx = 0;
        } else if (cx > SHRT_MAX) {
            cx = SHRT_MAX;
        }
        if (cy < 0) {
            cy = 0;
        } else if (cy > SHRT_MAX) {
            cy = SHRT_MAX;
        }
    }

#ifdef NEVER
//
// do not fail these conditions because real apps use them.
//
    if (!(dwFlags & SWP_NOMOVE) &&
            (x > SHRT_MAX || x < SHRT_MIN ||
             y > SHRT_MAX || y < SHRT_MIN)) {
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "Invalid coordinate passed to SetWindowPos");
        MSGERROR(0);
    }

    /*
     * Actually, if we were going to be really strict about this we'd
     * make sure that x + cx < SHRT_MAX, etc but since we do maintain
     * signed 32-bit coords internally this case doesn't cause a problem.
     */
    if (!(dwFlags & SWP_NOSIZE) &&
            (cx < 0 || cx > SHRT_MAX ||
             cy < 0 || cy > SHRT_MAX)) {
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "Invalid width/height passed to SetWindowPos");
        MSGERROR(0);
    }
#endif

    switch((ULONG_PTR)pwndInsertAfter) {
    case (ULONG_PTR)HWND_TOPMOST:
    case (ULONG_PTR)HWND_NOTOPMOST:
    case (ULONG_PTR)HWND_TOP:
    case (ULONG_PTR)HWND_BOTTOM:
        pwndT = NULL;
        break;

    default:
        pwndT = pwndInsertAfter;
        break;
    }

    ThreadLockWithPti(ptiCurrent, pwndT, &tlpwndT);

    retval = xxxSetWindowPos(
            pwndND,
            pwndInsertAfter,
            x,
            y,
            cx,
            cy,
            dwFlags);

    ThreadUnlock(&tlpwndT);

    TRACE("NtUserSetWindowPos");
    ENDRECV_HWNDLOCK_ND();
}

BOOL NtUserSetShellWindowEx(
    IN HWND hwnd,
    IN HWND hwndBkGnd)
{
    PWND        pwndBkGnd;
    TL          tlpwndBkGnd;

    BEGINRECV_HWNDLOCK_ND(BOOL, FALSE, hwnd);

    ValidateHWNDND(pwndBkGnd, hwndBkGnd);

    ThreadLockAlwaysWithPti(ptiCurrent, pwndBkGnd, &tlpwndBkGnd);

    retval = xxxSetShellWindow(pwndND, pwndBkGnd);

    ThreadUnlock(&tlpwndBkGnd);

    TRACE("NtUserSetShellWindowEx");
    ENDRECV_HWNDLOCK_ND();
}

DWORD
NtUserGetGuiResources(
    HANDLE hProcess,
    DWORD dwFlags)

{
    PEPROCESS Process;
    PW32PROCESS pW32Process;
    BEGINRECV_SHARED(DWORD, 0);

    /*
     * Probe arguments
     */
    if (dwFlags > GR_MAXOBJECT) {
        RIPERR1(ERROR_INVALID_PARAMETER, RIP_VERBOSE, "GetGuiResources: invalid flag bits %x\n",
                        dwFlags);
        MSGERROR(0);
    }


    if (hProcess == NtCurrentProcess()) {
        pW32Process = W32GetCurrentProcess();
    } else {
        NTSTATUS Status;
        Status = ObReferenceObjectByHandle(
            hProcess,
            PROCESS_QUERY_INFORMATION,
            *PsProcessType,
            UserMode,
            &Process,
            NULL);

        if (!NT_SUCCESS(Status)) {
            RIPERR2(ERROR_INVALID_PARAMETER, RIP_WARNING, "GetGuiResources: Failed with Process handle == %X, Status = %x\n",
                    hProcess, Status);
            MSGERROR(0);
        }

        /*
         * Make sure they are from the same session
         */
        if (Process->SessionId != gSessionId) {
            RIPERR2(ERROR_INVALID_PARAMETER, RIP_VERBOSE, "GetGuiResources: Different session. Failed with Process handle == %X, Status = %x\n",
                    hProcess, Status);
            ObDereferenceObject(Process);
            MSGERROR(0);
        }

        pW32Process = Process->Win32Process;
    }

    if (pW32Process) {
        switch(dwFlags) {
        case GR_GDIOBJECTS:
            retval = pW32Process->GDIHandleCount;
            break;
        case GR_USEROBJECTS:
            retval = pW32Process->UserHandleCount;
            break;
        }
    } else
        retval = 0;

    if (hProcess != NtCurrentProcess()) {
        ObDereferenceObject(Process);
    }

    TRACE("NtUserGetGuiResources");
    ENDRECV_SHARED();
}


BOOL NtUserSystemParametersInfo(  // API SystemParametersInfoA/W
    IN UINT   wFlag,
    IN DWORD  wParam,
    IN OUT LPVOID lpData,
    IN UINT   flags)
{
    UNICODE_STRING strData;
    ULONG          ulLength, ulLength2;
    LPVOID         lpDataSave;
    union {
        INT              MouseData[3];
        LOGFONTW         LogFont;
        MOUSEKEYS        MouseKeys;
        FILTERKEYS       FilterKeys;
        STICKYKEYS       StickyKeys;
        TOGGLEKEYS       ToggleKeys;
        SOUNDSENTRY      SoundSentry;
        ACCESSTIMEOUT    AccessTimeout;
        RECT             Rect;
        ANIMATIONINFO    AnimationInfo;
        NONCLIENTMETRICS NonClientMetrics;
        MINIMIZEDMETRICS MinimizedMetrics;
        ICONMETRICS      IconMetrics;
        HKL              hkl;
        INTERNALSETHIGHCONTRAST     ihc;
        HIGHCONTRAST     hc;
        WCHAR            szTemp[MAX_PATH];
    } CaptureBuffer;
    PTHREADINFO pti;
    TL tlBuffer;
    BOOL fFreeBuffer = FALSE;
    BOOL fWrite = FALSE;




    BEGINRECV(BOOL, FALSE);

    /*
     * Prevent restricted processes from setting system
     * parameters.
     *
     * clupu: this is ineficient and temporary. I'll change this
     * soon !!!
     */
    if (IS_CURRENT_THREAD_RESTRICTED(JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS)) {

        switch (wFlag) {
        case SPI_SETBEEP:
        case SPI_SETMOUSE:
        case SPI_SETBORDER:
        case SPI_SETKEYBOARDSPEED:
        case SPI_ICONHORIZONTALSPACING:
        case SPI_SETSCREENSAVETIMEOUT:
        case SPI_SETSCREENSAVEACTIVE:
        case SPI_SETGRIDGRANULARITY:
        case SPI_SETDESKWALLPAPER:
        case SPI_SETDESKPATTERN:
        case SPI_SETKEYBOARDDELAY:
        case SPI_ICONVERTICALSPACING:
        case SPI_SETICONTITLEWRAP:
        case SPI_SETMENUDROPALIGNMENT:
        case SPI_SETDOUBLECLKWIDTH:
        case SPI_SETDOUBLECLKHEIGHT:
        case SPI_SETDOUBLECLICKTIME:
        case SPI_SETMOUSEBUTTONSWAP:
        case SPI_SETICONTITLELOGFONT:
        case SPI_SETFASTTASKSWITCH:
        case SPI_SETDRAGFULLWINDOWS:
        case SPI_SETNONCLIENTMETRICS:
        case SPI_SETMINIMIZEDMETRICS:
        case SPI_SETICONMETRICS:
        case SPI_SETWORKAREA:
        case SPI_SETPENWINDOWS:
        case SPI_SETHIGHCONTRAST:
        case SPI_SETKEYBOARDPREF:
        case SPI_SETSCREENREADER:
        case SPI_SETANIMATION:
        case SPI_SETFONTSMOOTHING:
        case SPI_SETDRAGWIDTH:
        case SPI_SETDRAGHEIGHT:
        case SPI_SETHANDHELD:
        case SPI_SETLOWPOWERTIMEOUT:
        case SPI_SETPOWEROFFTIMEOUT:
        case SPI_SETLOWPOWERACTIVE:
        case SPI_SETPOWEROFFACTIVE:
        case SPI_SETCURSORS:
        case SPI_SETICONS:
        case SPI_SETDEFAULTINPUTLANG:
        case SPI_SETLANGTOGGLE:
        case SPI_SETMOUSETRAILS:
        case SPI_SETSCREENSAVERRUNNING:
        case SPI_SETFILTERKEYS:
        case SPI_SETTOGGLEKEYS:
        case SPI_SETMOUSEKEYS:
        case SPI_SETSHOWSOUNDS:
        case SPI_SETSTICKYKEYS:
        case SPI_SETACCESSTIMEOUT:
        case SPI_SETSOUNDSENTRY:
        case SPI_SETSNAPTODEFBUTTON:
        case SPI_SETMOUSEHOVERWIDTH:
        case SPI_SETMOUSEHOVERHEIGHT:
        case SPI_SETMOUSEHOVERTIME:
        case SPI_SETWHEELSCROLLLINES:
        case SPI_SETMENUSHOWDELAY:
        case SPI_SETSHOWIMEUI:
        case SPI_SETMOUSESPEED:
        case SPI_SETACTIVEWINDOWTRACKING:
        case SPI_SETMENUANIMATION:
        case SPI_SETCOMBOBOXANIMATION:
        case SPI_SETLISTBOXSMOOTHSCROLLING:
        case SPI_SETGRADIENTCAPTIONS:
        case SPI_SETKEYBOARDCUES:
        case SPI_SETACTIVEWNDTRKZORDER:
        case SPI_SETHOTTRACKING:
        case SPI_SETMENUFADE:
        case SPI_SETSELECTIONFADE:
        case SPI_SETTOOLTIPANIMATION:
        case SPI_SETTOOLTIPFADE:
        case SPI_SETFOREGROUNDLOCKTIMEOUT:
        case SPI_SETACTIVEWNDTRKTIMEOUT:
        case SPI_SETFOREGROUNDFLASHCOUNT:
            MSGERROR(0);
            break;
        }
    }

    try {
        switch(wFlag) {

        case SPI_SETDESKPATTERN:
            /*
             * If wParam is -1, that means read the new wallpaper from
             * win.ini. If wParam is not -1, lParam points to the wallpaper
             * string.
             */
            if (wParam == (WPARAM)-1)
                break;

            /*
             * SetDeskPattern may take a string in lpData; if lpData
             * is one of the magic values it obviously is not a string
             */
            if (lpData == (PVOID)IntToPtr( 0xFFFFFFFF ) || lpData == (PVOID)NULL) {
                /*
                 * These are not really magic values, but in order not to break apps
                 * we have to keep them valid.  wParam =-1 will make lParam be ignored
                 * MCostea 208416
                 */
                wParam = -1;
                break;
            }
            goto ProbeString;

        case SPI_SETDESKWALLPAPER:

            /*
             * If the caller passed in (-1) in the wParam, then the
             * wallpaper-name is to be loaded later.  Otherwise,
             * they passed in a unicode-string in the lParam.
             */
            if (wParam == (WPARAM)-1)
                break;

            if (((LPWSTR)lpData == NULL)                 ||
                ((LPWSTR)lpData == SETWALLPAPER_METRICS) ||
                ((LPWSTR)lpData == SETWALLPAPER_DEFAULT)) {
                break;
            }

ProbeString:

            /*
             * Probe and capture the string.  Capture is necessary to
             * the pointer to be passed directly to the registry routines
             * which cannot cleanly handle exceptions.
             */
            strData = ProbeAndReadUnicodeString((PUNICODE_STRING)lpData);
#if defined(_X86_)
            ProbeForRead(strData.Buffer, strData.Length, sizeof(BYTE));
#else
            ProbeForRead(strData.Buffer, strData.Length, sizeof(WCHAR));
#endif
            lpData = UserAllocPoolWithQuota(strData.Length + sizeof(WCHAR), TAG_TEXT2);
            if (lpData == NULL) {
                ExRaiseStatus(STATUS_NO_MEMORY);
            }
            pti = PtiCurrent();
            ThreadLockPool(pti, lpData, &tlBuffer);
            fFreeBuffer = TRUE;
            RtlCopyMemory(lpData,
                          strData.Buffer,
                          strData.Length);
            ((PWSTR)lpData)[strData.Length / sizeof(WCHAR)] = 0;
            break;

        case SPI_SETMOUSE:
            ulLength = sizeof(INT) * 3;
            goto ProbeRead;
        case SPI_SETICONTITLELOGFONT:
            if (!ARGUMENT_PRESENT(lpData))
                break;
            ulLength = sizeof(LOGFONTW);
            goto ProbeRead;
        case SPI_SETMOUSEKEYS:
            ulLength = sizeof(MOUSEKEYS);
            goto ProbeRead;
        case SPI_SETFILTERKEYS:
            ulLength = sizeof(FILTERKEYS);
            goto ProbeRead;
        case SPI_SETSTICKYKEYS:
            ulLength = sizeof(STICKYKEYS);
            goto ProbeRead;
        case SPI_SETTOGGLEKEYS:
            ulLength = sizeof(TOGGLEKEYS);
            goto ProbeRead;
        case SPI_SETSOUNDSENTRY:
            ulLength = sizeof(SOUNDSENTRY);
            goto ProbeRead;
        case SPI_SETACCESSTIMEOUT:
            ulLength = sizeof(ACCESSTIMEOUT);
            goto ProbeRead;
        case SPI_SETWORKAREA:
            ulLength = sizeof(RECT);
            goto ProbeRead;
        case SPI_SETANIMATION:
            ulLength = sizeof(ANIMATIONINFO);
            goto ProbeRead;
        case SPI_SETNONCLIENTMETRICS:
            ulLength = sizeof(NONCLIENTMETRICS);
            goto ProbeRead;
        case SPI_SETMINIMIZEDMETRICS:
            ulLength = sizeof(MINIMIZEDMETRICS);
            goto ProbeRead;
        case SPI_SETICONMETRICS:
            ulLength = sizeof(ICONMETRICS);
            goto ProbeRead;
        case SPI_SETDEFAULTINPUTLANG:
            ulLength = sizeof(HKL);
            goto ProbeRead;
        case SPI_SETHIGHCONTRAST:
            CaptureBuffer.ihc = ProbeAndReadStructure((INTERNALSETHIGHCONTRAST *)lpData, INTERNALSETHIGHCONTRAST);
            lpData = &CaptureBuffer.ihc;

            /*
             * Now probe High Contrast string -- note that we send a client-side buffer pointer to the routine.
             */

            ProbeForReadUnicodeStringBuffer(CaptureBuffer.ihc.usDefaultScheme);
            if (CaptureBuffer.ihc.usDefaultScheme.Length == 0) {
                CaptureBuffer.ihc.usDefaultScheme.Buffer = NULL;
            }
            break;

            /*
             * Probe and capture the data.  Capture is necessary to
             * allow the pointer to be passed to the worker routines
             * where exceptions cannot be cleanly handled.
             */
ProbeRead:
#if defined(_X86_)
            ProbeForRead(lpData, ulLength, sizeof(BYTE));
#else
            ProbeForRead(lpData, ulLength, sizeof(DWORD));
#endif
            RtlCopyMemory(&CaptureBuffer, lpData, ulLength);
            lpData = &CaptureBuffer;
            break;

        case SPI_ICONHORIZONTALSPACING: // returns INT
        case SPI_ICONVERTICALSPACING:   // returns INT
            if (!IS_PTR(lpData))
                break;

            /*
             * Fall through and probe the data
             */
        case SPI_GETBEEP:               // returns BOOL
        case SPI_GETBORDER:             // returns INT
        case SPI_GETKEYBOARDSPEED:      // returns DWORD
        case SPI_GETKEYBOARDDELAY:      // returns INT
        case SPI_GETSCREENSAVETIMEOUT:  // returns INT
        case SPI_GETLOWPOWERTIMEOUT:    // returns INT
        case SPI_GETPOWEROFFTIMEOUT:    // returns INT
        case SPI_GETSCREENSAVEACTIVE:   // returns BOOL
        case SPI_GETLOWPOWERACTIVE:     // returns BOOL
        case SPI_GETPOWEROFFACTIVE:     // returns BOOL
        case SPI_GETGRIDGRANULARITY:    // returns INT
        case SPI_GETICONTITLEWRAP:      // returns BOOL
        case SPI_GETMENUDROPALIGNMENT:  // returns BOOL
        case SPI_GETFASTTASKSWITCH:     // returns BOOL
        case SPI_GETDRAGFULLWINDOWS:    // returns INT
        case SPI_GETSHOWSOUNDS:         // returns BOOL
        case SPI_GETFONTSMOOTHING:      // returns INT
        case SPI_GETSNAPTODEFBUTTON:    // returns BOOL
        case SPI_GETKEYBOARDPREF:       // returns BOOL
        case SPI_GETSCREENREADER:       // returns BOOL
        case SPI_GETDEFAULTINPUTLANG:
        case SPI_GETMOUSEHOVERWIDTH:
        case SPI_GETMOUSEHOVERHEIGHT:
        case SPI_GETMOUSEHOVERTIME:
        case SPI_GETWHEELSCROLLLINES:
        case SPI_GETMENUSHOWDELAY:
        case SPI_GETMOUSESPEED:
        case SPI_GETSCREENSAVERRUNNING:
        case SPI_GETSHOWIMEUI:
            goto ProbeWriteUlong;

        case SPI_GETICONTITLELOGFONT:   // returns LOGFONT
            ulLength = sizeof(LOGFONT);
            goto ProbeWrite;
        case SPI_GETMOUSE:              // returns 3 INTs
            ulLength = sizeof(INT) * 3;
            goto ProbeWrite;
        case SPI_GETFILTERKEYS:         // returns FILTERKEYS
            ulLength = sizeof(FILTERKEYS);
            goto ProbeWrite;
        case SPI_GETSTICKYKEYS:         // returns STICKYKEYS
            ulLength = sizeof(STICKYKEYS);
            goto ProbeWrite;
        case SPI_GETMOUSEKEYS:          // returns MOUSEKEYS
            ulLength = sizeof(MOUSEKEYS);
            goto ProbeWrite;
        case SPI_GETTOGGLEKEYS:         // returns TOGGLEKEYS
            ulLength = sizeof(TOGGLEKEYS);
            goto ProbeWrite;
        case SPI_GETSOUNDSENTRY:        // returns SOUNDSENTRY
            ulLength = sizeof(SOUNDSENTRY);
            goto ProbeWrite;
        case SPI_GETACCESSTIMEOUT:      // returns ACCESSTIMEOUT
            ulLength = sizeof(ACCESSTIMEOUT);
            goto ProbeWrite;
        case SPI_GETANIMATION:          // returns ANIMATIONINFO
            ulLength = sizeof(ANIMATIONINFO);
            goto ProbeWrite;
        case SPI_GETNONCLIENTMETRICS:   // returns NONCLIENTMETRICS
            ulLength = sizeof(NONCLIENTMETRICS);
            goto ProbeWrite;
        case SPI_GETMINIMIZEDMETRICS:   // returns MINIMIZEDMETRICS
            ulLength = sizeof(MINIMIZEDMETRICS);
            goto ProbeWrite;
        case SPI_GETICONMETRICS:        // returns ICONMETRICS
            ulLength = sizeof(ICONMETRICS);
            goto ProbeWrite;

        case SPI_GETHIGHCONTRAST:       // returns HIGHCONTRAST
            ulLength = sizeof(HIGHCONTRASTW);
            ProbeForWrite(lpData, ulLength, DATAALIGN);
            lpDataSave = lpData;
            CaptureBuffer.hc = *((LPHIGHCONTRAST)lpData);
            lpData = &CaptureBuffer.hc;

            /*
             * now probe string address
             */

            ulLength2 = MAX_SCHEME_NAME_SIZE * sizeof(WCHAR);

            ProbeForWrite(((LPHIGHCONTRAST)lpData)->lpszDefaultScheme, ulLength2, CHARALIGN);
            fWrite = TRUE;
            break;
        case SPI_GETWORKAREA:           // returns RECT
            ulLength = sizeof(RECT);
            goto ProbeWrite;


        /*
         * Bug 257718 - joejo
         * Add SPI_GETDESKWALLPAPER to SystemParametersInfo
         */
        case SPI_GETDESKWALLPAPER:
            lpDataSave = lpData;
            lpData = CaptureBuffer.szTemp;
            ProbeForWriteBuffer((PWSTR)lpDataSave, wParam, CHARALIGN);
            wParam = (wParam < MAX_PATH) ? wParam : MAX_PATH;
            ulLength = wParam * sizeof(WCHAR);
            fWrite = TRUE;
            break;

        default:
            if (wFlag < SPI_MAX) {
                break;
            } else if (!UPIsBOOLRange(wFlag)
                    && !UPIsDWORDRange(wFlag)) {

                RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "NtUserSystemParametersInfo: Invalid SPI_:%#lx", wFlag);
                retval = FALSE;
                MSGERRORCLEANUP(0);
            }

            /*
             * Let's enforce this or this parameter is gone for good.
             */
            if (wParam != 0) {
                /*
                 * Too late, Winstone98 is alreay using it (incorrectly).
                 * Bummer, this has never been shipped and it's hacked already
                 * Allow a special case to go through
                 */
                if ((PtiCurrent()->dwExpWinVer > VER40)
                        || (wFlag != SPI_SETUIEFFECTS)
                        || (wParam != 1)) {
                    RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "NtUserSystemParametersInfo: uiParam must be zero for SPI %#lx", wFlag);
                    retval = FALSE;
                    MSGERRORCLEANUP(0);
                }
            }

            UserAssert(wFlag & SPIF_RANGETYPEMASK);

            if (wFlag & SPIF_SET) {
                /*
                 * If your dword data needs to be validated (i.e, range, value),
                 *  switch here on wFlag and do it here
                 */
                switch (wFlag) {
                    case SPI_SETFOREGROUNDLOCKTIMEOUT:
                        if (!CanForceForeground(PpiCurrent())) {
                            RIPERR0(ERROR_ACCESS_DENIED, RIP_VERBOSE, "");
                            retval = FALSE;
                            MSGERRORCLEANUP(0);
                        }
                        break;
                }

            } else { /* if (wFlag & SPIF_SET) */
ProbeWriteUlong:
                ulLength = sizeof(ULONG);
                lpDataSave = lpData;
                lpData = &CaptureBuffer;
                ProbeForWriteUlong((PULONG)lpDataSave);
                fWrite = TRUE;
            }
            break;

            /*
             * Probe the data.  wParam contains the length
             */
ProbeWrite:
            lpDataSave = lpData;
            lpData = &CaptureBuffer;
            ProbeForWrite(lpDataSave, ulLength, DATAALIGN);
            fWrite = TRUE;
            /*
             * Copy the first DWORD of the buffer.  This will make sure that
             * the cbSize parameter of some structures gets copied.
             */

            UserAssert(ulLength >= sizeof(DWORD));
            *(LPDWORD)lpData=*(LPDWORD)lpDataSave;
        }
    } except (StubExceptionHandler(TRUE)) {
        MSGERRORCLEANUP(0);
    }

   retval = xxxSystemParametersInfo(wFlag, wParam, lpData, flags);

   /*
     * Copy out any data we need to.
     */
    if (fWrite) {
        try {
            RtlCopyMemory(lpDataSave, lpData, ulLength);
        } except (StubExceptionHandler(TRUE)) {
            MSGERRORCLEANUP(0);
        }
    }

    CLEANUPRECV();
    if (fFreeBuffer)
        ThreadUnlockAndFreePool(pti, &tlBuffer);

    TRACE("NtUserSystemParametersInfo");
    ENDRECV();
}

BOOL NtUserUpdatePerUserSystemParameters(
    IN HANDLE hToken,
    IN BOOL   bUserLoggedOn)
{
    BEGINRECV(BOOL, FALSE);

    retval = xxxUpdatePerUserSystemParameters(hToken, bUserLoggedOn);

    TRACE("NtUserUpdatePerUserSystemParameters");
    ENDRECV();
}

DWORD NtUserDdeInitialize(  // API DdeInitializeA/W
    OUT PHANDLE phInst,
    OUT HWND *phwnd,
    OUT LPDWORD pMonFlags,
    IN DWORD afCmd,
    IN PVOID pcii)
{
    HANDLE hInst;
    HWND hwnd;
    DWORD MonFlags;

    BEGINRECV(DWORD, DMLERR_INVALIDPARAMETER);

    /*
     * NOTE -- pcii is a value that is passed back to the client side
     *  for event callbacks.  It is not probed because it is not used
     *  on the kernel side.
     */

    retval = xxxCsDdeInitialize(&hInst, &hwnd,
            &MonFlags, afCmd, pcii);

    /*
     * Probe arguments.  pcii is not dereferenced in the kernel so probing
     * is not needed.
     */
    if (retval == DMLERR_NO_ERROR) {
        try {
            ProbeAndWriteHandle(phInst, hInst);
            ProbeAndWriteHandle((PHANDLE)phwnd, hwnd);
            ProbeAndWriteUlong(pMonFlags, MonFlags);
        } except (StubExceptionHandler(TRUE)) {
            xxxDestroyThreadDDEObject(PtiCurrent(), HtoP(hInst));
            MSGERROR(0);
        }
    }

    TRACE("NtUserDdeInitialize");
    ENDRECV();
}

DWORD NtUserUpdateInstance( // private, but pMonFlags from API DdeInitializeA/W
    IN HANDLE hInst,
    OUT LPDWORD pMonFlags,
    IN DWORD afCmd)
{
    DWORD MonFlags;
    BEGINRECV(DWORD, DMLERR_INVALIDPARAMETER);

    /*
     * Probe arguments
     */
    try {
        ProbeForWriteUlong(pMonFlags);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }
    retval = _CsUpdateInstance(hInst, &MonFlags, afCmd);
    try {
        *pMonFlags = MonFlags;
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    TRACE("NtUserUpdateInstance");
    ENDRECV();
}

DWORD NtUserEvent(  // private
    IN PEVENT_PACKET pep)
{
    WORD cbEventData;
    BEGINRECV(DWORD, 0);

    /*
     * Probe arguments
     */
    try {
        ProbeForRead(pep, sizeof(*pep), DATAALIGN);
        /*
         * capture so that another thread can't change it after the probe.
         */
        cbEventData = pep->cbEventData;
        ProbeForRead(&pep->Data, cbEventData, DATAALIGN);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    /*
     * The buffer is captured within a try/except in xxxCsEvent.
     */

    retval = xxxCsEvent((PEVENT_PACKET)pep, cbEventData);

    TRACE("NtUserEvent");
    ENDRECV();
}

BOOL NtUserFillWindow(
    IN HWND hwndBrush,
    IN HWND hwndPaint,
    IN HDC hdc,
    IN HBRUSH hbr)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    PWND pwndBrush;
    TL tlpwndBrush;

    BEGINRECV_HWNDLOCK(DWORD, 0, hwndPaint);

    if (hdc == NULL)
        MSGERROR(0);

    ValidateHWNDOPT(pwndBrush, hwndBrush);

    ThreadLockWithPti(ptiCurrent, pwndBrush, &tlpwndBrush);

    retval = xxxFillWindow(
            pwndBrush,
            pwnd,
            hdc,
            hbr);

    ThreadUnlock(&tlpwndBrush);

    TRACE("NtUserFillWindow");
    ENDRECV_HWNDLOCK();
}

PCLS NtUserGetWOWClass(  // wow
    IN HINSTANCE hInstance,
    IN PUNICODE_STRING pString)
{
    UNICODE_STRING strClassName;

    BEGINRECV_SHARED(PCLS, NULL);

    /*
     * Probe arguments
     */
    try {
        strClassName = ProbeAndReadUnicodeString(pString);
        ProbeForReadUnicodeStringBuffer(strClassName);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = _GetWOWClass(
            hInstance,
            strClassName.Buffer);

    TRACE("NtUserGetWOWClass");
    ENDRECV_SHARED();
}

UINT NtUserGetInternalWindowPos(  // private
    IN HWND hwnd,
    OUT LPRECT lpRect OPTIONAL,
    OUT LPPOINT lpPoint OPTIONAL)
{
    WINDOWPLACEMENT wp;

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    BEGINRECV_HWND_SHARED(DWORD, 0, hwnd);

    /*
     * Probe arguments
     */
    try {
        if (ARGUMENT_PRESENT(lpRect)) {
            ProbeForWriteRect(lpRect);
        }
        if (ARGUMENT_PRESENT(lpPoint)) {
            ProbeForWritePoint(lpPoint);
        }

    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    wp.length = sizeof(WINDOWPLACEMENT);

    _GetWindowPlacement(pwnd, &wp);

    retval = wp.showCmd;
    try {
        if (ARGUMENT_PRESENT(lpRect)) {
            RtlCopyMemory(lpRect, &wp.rcNormalPosition, sizeof (RECT));
        }
        if (ARGUMENT_PRESENT(lpPoint)) {
            RtlCopyMemory(lpPoint, &wp.ptMinPosition, sizeof (POINT));
        }

    } except (StubExceptionHandler(FALSE)) {
    }

    TRACE("NtUserGetInternalWindowPos");
    ENDRECV_HWND_SHARED();
}

NTSTATUS NtUserInitTask(  // wow
    IN UINT dwExpWinVer,
    IN DWORD dwAppCompatFlags,
    IN PUNICODE_STRING pstrModName,
    IN PUNICODE_STRING pstrBaseFileName,
    IN DWORD hTaskWow,
    IN DWORD dwHotkey,
    IN DWORD idTask,
    IN DWORD dwX,
    IN DWORD dwY,
    IN DWORD dwXSize,
    IN DWORD dwYSize)
{
    UNICODE_STRING strModName;
    UNICODE_STRING strBaseFileName;

    BEGINRECV(NTSTATUS, STATUS_UNSUCCESSFUL);

    /*
     * Make sure this is really a WOW process.
     */
    if (PpiCurrent()->pwpi == NULL) {
        MSGERROR(0);
    }

    /*
     * Probe arguments
     */
    try {
        strModName = ProbeAndReadUnicodeString(pstrModName);
        /*
         * pstrModName->Buffer has a UNICODE_NULL that's not counted in
         * the length, but we want to include it for convenience. The
         * probe routine does this.
         */
        ProbeForReadUnicodeStringBuffer(strModName);

        if (pstrBaseFileName) {
            strBaseFileName = ProbeAndReadUnicodeString(pstrBaseFileName);
            ProbeForReadUnicodeStringBuffer(strBaseFileName);
        }

    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = zzzInitTask(
            dwExpWinVer,
            dwAppCompatFlags,
            &strModName,
            pstrBaseFileName ? &strBaseFileName : NULL,
            hTaskWow,
            dwHotkey,
            idTask,
            dwX,
            dwY,
            dwXSize,
            dwYSize);

    TRACE("NtUserInitTask");
    ENDRECV();
}

BOOL NtUserPostThreadMessage(
    IN DWORD id,
    IN UINT msg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PTHREADINFO ptiCurrent, pti;

    BEGINRECV(BOOL, FALSE);

    /*
     * Prevent apps from setting hi 16 bits so we can use them internally.
     */
    if (msg & MSGFLAG_MASK) {
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "Invalid message");
        MSGERROR(0);
    }

    pti = PtiFromThreadId(id);
    if (pti == NULL) {
        struct tagWOWPROCESSINFO *pwpi;
        PTDB ptdb;

        for (pwpi=gpwpiFirstWow; pwpi; pwpi=pwpi->pwpiNext) {
            for (ptdb=pwpi->ptdbHead; ptdb; ptdb=ptdb->ptdbNext) {
                if (ptdb->hTaskWow == id) {
                    pti=ptdb->pti;
                    goto PTM_DoIt;
                }
            }
        }

        RIPERR0(ERROR_INVALID_THREAD_ID, RIP_VERBOSE, "");
        MSGERROR(0);
    }

PTM_DoIt:

    /*
     * Should be OK if any of the following are true
     *   threads are running on the same desktop
     *   request is on behalf of a system process
     *   this process owns the desktop the thread is running in
     *   the LUIDs match
     */
    ptiCurrent = PtiCurrent();
    if ( !(ptiCurrent->rpdesk == pti->rpdesk) &&
         !(ptiCurrent->TIF_flags & TIF_CSRSSTHREAD) &&
         !(GetDesktopView(ptiCurrent->ppi, pti->rpdesk))) {

        LUID     luidCurrent, luidTo;

        if (!NT_SUCCESS(GetProcessLuid(ptiCurrent->pEThread, &luidCurrent)) ||
            !NT_SUCCESS(GetProcessLuid(pti->pEThread, &luidTo)) ||
            !RtlEqualLuid(&luidCurrent, &luidTo)) {
            RIPERR3(ERROR_INVALID_THREAD_ID,
                    RIP_WARNING,
                    "NtUserPostThreadMessage failed LUID check: msg(%lx), t1(%#p) -> t2(%#p)",
                    msg, ptiCurrent, pti);
            MSGERROR(0);
        }
    }

    retval = _PostThreadMessage(
            pti,
            msg,
            wParam,
            lParam);

    TRACE("NtUserPostThreadMessage");
    ENDRECV();
}

BOOL NtUserRegisterTasklist(
    IN HWND hwnd)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    BEGINRECV_HWND(DWORD, 0, hwnd);

    retval = _RegisterTasklist(
            pwnd);

    TRACE("NtUserRegisterTasklist");
    ENDRECV_HWND();
}

BOOL NtUserCloseClipboard(
    VOID)
{
    BEGINRECV(BOOL, FALSE);

    retval = xxxCloseClipboard(NULL);

    TRACE("NtUserCloseClipboard");
    ENDRECV();
}

BOOL NtUserEmptyClipboard(
    VOID)
{
    BEGINRECV(BOOL, FALSE);

    retval = xxxEmptyClipboard(NULL);

    TRACE("NtUserEmptyClipboard");
    ENDRECV();
}

BOOL NtUserSetClipboardData(  // API SetClipboardData
    IN UINT          fmt,
    IN HANDLE        hData,
    IN PSETCLIPBDATA pscd)
{
    SETCLIPBDATA scd;

    BEGINRECV(BOOL, FALSE);

    /*
     * Check for jobs with JOB_OBJECT_UILIMIT_WRITECLIPBOARD
     */
    if (IS_CURRENT_THREAD_RESTRICTED(JOB_OBJECT_UILIMIT_WRITECLIPBOARD)) {
        RIPMSG0(RIP_WARNING, "NtUserSetClipboardData failed for restricted thread");
        MSGERROR(0);
    }

    /*
     * Probe arguments
     */
    try {
        scd = ProbeAndReadSetClipBData(pscd);
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    retval = _SetClipboardData(
            fmt,
            hData,
            scd.fGlobalHandle,
            scd.fIncSerialNumber);

    TRACE("NtUserSetClipboardData");
    ENDRECV();
}

HANDLE NtUserConvertMemHandle(  // worker routine, lpData not from API
    IN LPBYTE lpData,
    IN UINT   cbData)
{
    BEGINRECV(HANDLE, NULL);

    /*
     * Probe arguments
     */
    try {

        ProbeForRead(lpData, cbData, sizeof(BYTE));

    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    /*
     * lpData is client-side.
     */
    retval = _ConvertMemHandle(lpData, cbData);

    TRACE("NtUserConvertMemHandle");
    ENDRECV();
}

NTSTATUS NtUserCreateLocalMemHandle(  // helper routine
    IN HANDLE hMem,
    OUT LPBYTE lpData OPTIONAL,
    IN UINT cbData,
    OUT PUINT lpcbNeeded OPTIONAL)
{
    PCLIPDATA pClipData;

    BEGINRECV(NTSTATUS, STATUS_INVALID_HANDLE);

    pClipData = HMValidateHandle(hMem, TYPE_CLIPDATA);
    if (pClipData == NULL)
        MSGERROR(0);

    /*
     * Probe arguments
     */
    try {
        if (ARGUMENT_PRESENT(lpData)) {
            ProbeForWrite(lpData, cbData, sizeof(BYTE));
        }

        if (ARGUMENT_PRESENT(lpcbNeeded)) {
            ProbeAndWriteUlong(lpcbNeeded, pClipData->cbData);
        }

        if (!ARGUMENT_PRESENT(lpData) || cbData < pClipData->cbData) {
            retval = STATUS_BUFFER_TOO_SMALL;
        } else {
            RtlCopyMemory(lpData, &pClipData->abData, pClipData->cbData);
            retval = STATUS_SUCCESS;
        }
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    TRACE("NtUserCreateLocalMemHandle");
    ENDRECV();
}

HHOOK NtUserSetWindowsHookEx(
    IN HANDLE hmod,
    IN PUNICODE_STRING pstrLib OPTIONAL,
    IN DWORD idThread,
    IN int nFilterType,
    IN PROC pfnFilterProc,
    IN DWORD dwFlags)
{
    PTHREADINFO ptiThread;

    BEGINRECV(HHOOK, NULL);

    if (idThread != 0) {
        ptiThread = PtiFromThreadId(idThread);
        if (ptiThread == NULL) {
            RIPERR0(ERROR_INVALID_PARAMETER, RIP_VERBOSE, "");
            MSGERROR(0);
        }
    } else {
        ptiThread = NULL;
    }

    /*
     * Probe pstrLib in GetHmodTableIndex().
     */
    retval = (HHOOK)zzzSetWindowsHookEx(
            hmod,
            pstrLib,
            ptiThread,
            nFilterType,
            pfnFilterProc,
            dwFlags);
    retval = PtoH((PVOID)retval);

    TRACE("NtUserSetWindowsHookEx");
    ENDRECV();
}

HWINEVENTHOOK NtUserSetWinEventHook(
    IN DWORD           eventMin,
    IN DWORD           eventMax,
    IN HMODULE         hmodWinEventProc,
    IN PUNICODE_STRING pstrLib OPTIONAL,
    IN WINEVENTPROC    pfnWinEventProc,
    IN DWORD           idEventProcess,
    IN DWORD           idEventThread,
    IN DWORD           dwFlags)
{
    BEGINRECV(HWINEVENTHOOK, NULL);

    TESTFLAGS(dwFlags, WINEVENT_VALID);

    /*
     * Probe pstrLib in GetHmodTableIndex().
     */
    retval = (HWINEVENTHOOK)_SetWinEventHook(
            eventMin,
            eventMax,
            hmodWinEventProc,
            pstrLib,
            pfnWinEventProc,
            (HANDLE)LongToHandle( idEventProcess ),
            idEventThread,
            dwFlags);
    retval = PtoH((PVOID)retval);

    TRACE("NtUserSetWinEventHook");
    ENDRECV();
}

BOOL NtUserUnhookWinEvent(
    IN HWINEVENTHOOK hWinEventUnhook)
{
    PEVENTHOOK peh;

    BEGINATOMICRECV(BOOL, FALSE);

    ValidateHWINEVENTHOOK(peh, hWinEventUnhook);

    retval = _UnhookWinEvent(peh);

    TRACE("NtUserUnhookWinEvent");
    ENDATOMICRECV();
}

VOID NtUserNotifyWinEvent(
    IN DWORD event,
    IN HWND  hwnd,
    IN LONG  idObject,
    IN LONG  idChild)
{
    BEGINRECV_HWNDLOCK_VOID(hwnd);

    if (!FWINABLE()) {
        MSGERROR_VOID();
    }
    xxxWindowEvent(event, pwnd, idObject, idChild, WEF_USEPWNDTHREAD);

    TRACEVOID("NtUserNotifyWinEvent");
    ENDRECV_HWNDLOCK_VOID();
}


BOOL NtUserGetGUIThreadInfo(  // API GetGUIThreadInfo
    IN DWORD idThread,
    IN OUT PGUITHREADINFO pgui)
{
    PTHREADINFO ptiThread;
    GUITHREADINFO gui;

    BEGINRECV_SHARED(BOOL, FALSE);

    if (idThread) {
        ptiThread = PtiFromThreadId(idThread);
        if (ptiThread == NULL) {
            RIPERR0(ERROR_INVALID_PARAMETER, RIP_VERBOSE, "Bad thread id");
            MSGERROR(0);
        }
    } else {
        ptiThread = NULL;
    }

    /*
     * Probe arguments and copy results
     * C2: test pti & current thread on same desktop within _GetGUIThreadInfo
     */
    try {
        ProbeForWrite(pgui, sizeof(*pgui), DATAALIGN);
        gui.cbSize = pgui->cbSize;
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    retval = _GetGUIThreadInfo(ptiThread, &gui);
    if (retval) {
        try {
            *pgui = gui;
        } except (StubExceptionHandler(TRUE)) {
            MSGERROR(0);
        }
    }

    TRACE("NtUserGetGUIThreadInfo");
    ENDRECV_SHARED();
}

/*****************************************************************************\
* GetTitleBarInfo
*
* Gets information about the title bar
\*****************************************************************************/
BOOL NtUserGetTitleBarInfo(IN HWND hwnd, IN OUT PTITLEBARINFO ptbi)
{
    TITLEBARINFO tbi;

    BEGINRECV_HWNDLOCK(BOOL, FALSE, hwnd);

    /*
     * Probe arguments and copy out results
     */
    try {
        ProbeForWrite(ptbi, sizeof(*ptbi), DATAALIGN);
        tbi.cbSize = ptbi->cbSize;
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }
    /*
     * Get the titlebar info
     */
    retval = xxxGetTitleBarInfo(pwnd, &tbi);
    if (retval) {
        try {
            *ptbi = tbi;
        } except (StubExceptionHandler(TRUE)) {
            MSGERROR(0);
        }
    }

    TRACE("NtUserGetTitleBarInfo");
    ENDRECV_HWNDLOCK();
}


/*****************************************************************************\
* NtUserGetComboBoxInfo
*
* Gets information about the combo box
\*****************************************************************************/
BOOL NtUserGetComboBoxInfo(IN HWND hwnd, IN OUT PCOMBOBOXINFO pcbi)
{
    COMBOBOXINFO cbi;

    BEGINRECV_HWND_SHARED(BOOL, FALSE, hwnd);

    /*
     * Probe arguments and copy out results
     */
    try {
        ProbeForWrite(pcbi, sizeof(*pcbi), DATAALIGN);
        cbi.cbSize = pcbi->cbSize;
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

        /*
         * Get the combobox info
         */
    retval = _GetComboBoxInfo(pwnd, &cbi);

    if (retval) {
        try {
            *pcbi = cbi;
        } except (StubExceptionHandler(TRUE)) {
             MSGERROR(0);
        }
    }

    TRACE("NtUserGetComboBoxInfo");
    ENDRECV_HWND_SHARED();
}


/*****************************************************************************\
* NtUserGetListBoxInfo
*
* Gets information about the list box
\*****************************************************************************/
DWORD NtUserGetListBoxInfo(IN HWND hwnd)
{
    BEGINRECV_HWND_SHARED(DWORD, 0, hwnd);

    /*
     * Get the listbox info
     */
    retval = _GetListBoxInfo(pwnd);

    TRACE("NtUserGetListBoxInfo");
    ENDRECV_HWND_SHARED();
}


/*****************************************************************************\
* GetCursorInfo
*
* Gets information about the global cursor
\*****************************************************************************/
BOOL NtUserGetCursorInfo(IN OUT PCURSORINFO pci)
{
    CURSORINFO ci = {0};

    BEGINRECV_SHARED(BOOL, FALSE);

    ci.ptScreenPos = gpsi->ptCursor;
    ci.flags = 0;

    if (gpcurPhysCurrent)
        ci.flags |= CURSOR_SHOWING;

    /*
     * Get the current LOGICAL cursor (the one apps actually see from LoadCursor())
     */
    ci.hCursor = (HCURSOR)PtoH(gpcurLogCurrent);

    retval = TRUE;

    /*
     * Probe arguments and copy out result
     */
    try {
        ProbeForWrite(pci, sizeof(*pci), DATAALIGN);
        if (pci->cbSize != sizeof(CURSORINFO)) {
            RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "CURSORINFO.cbSize %d is wrong", pci->cbSize);
            retval = FALSE;
        } else {
            *pci = ci;
        }

    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    TRACE("NtUserGetCursorInfo");
    ENDRECV_SHARED();
}

/*****************************************************************************\
* GetScrollBarInfo
*
* Gets information about the scroll bar
\*****************************************************************************/
BOOL NtUserGetScrollBarInfo(IN HWND hwnd, IN LONG idObject, IN OUT PSCROLLBARINFO psbi)
{
    SCROLLBARINFO sbi;

    BEGINRECV_HWND_SHARED(BOOL, FALSE, hwnd);

    /*
     * Probe arguments and copy out results
     */
    try {
        ProbeForWrite(psbi, sizeof(*psbi), DATAALIGN);
        sbi.cbSize = psbi->cbSize;
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }
    /*
     * Get the scrollbar info
     */
    retval = _GetScrollBarInfo(pwnd, idObject, &sbi);

    if (retval) {
        try {
            *psbi = sbi;
        } except (StubExceptionHandler(TRUE)) {
            MSGERROR(0);
        }
    }

    TRACE("NtUserGetScrollBarInfo");
    ENDRECV_HWND_SHARED();
}

HWND NtUserGetAncestor(IN HWND hwndChild, IN UINT gaFlags)
{
    BEGINRECV_HWND_SHARED(HWND, NULL, hwndChild);

    if ((gaFlags < GA_MIN) || (gaFlags > GA_MAX)) {
        RIPERR3(ERROR_INVALID_PARAMETER, RIP_WARNING,
                "NtUserGetAncestor: Invalid gaFlags parameter %d, not %d - %d",
                 gaFlags, GA_MIN, GA_MAX);
        MSGERROR(0);
    }
    retval = (HWND)_GetAncestor(pwnd, gaFlags);
    retval = PtoH((PVOID)retval);

    TRACE("NtUserGetAncestor");
    ENDRECV_HWND_SHARED();
}

HWND NtUserRealChildWindowFromPoint(IN HWND hwndParent, IN POINT pt)
{
    BEGINRECV_HWND_SHARED(HWND, NULL, hwndParent);

    retval = (HWND)_RealChildWindowFromPoint(pwnd, pt);
    retval = PtoH((PVOID)retval);

    TRACE("NtUserRealChildWindowFromPoint");
    ENDRECV_HWND_SHARED();
}

BOOL NtUserGetAltTabInfo(
    IN HWND hwnd,
    IN int iItem,
    IN OUT PALTTABINFO pati,
    OUT LPWSTR lpszItemText,
    IN UINT cchItemText OPTIONAL,
    BOOL bAnsi)
{
    ALTTABINFO ati;

    BEGINRECV_HWNDOPT_SHARED(BOOL, FALSE, hwnd);

    /*
     * If the specified window is not a switch window, then fail the call.
     * It's a dumb API we got from Windows 95, I am hereby allowing NULL hwnd.
     */
    if (pwnd && (pwnd != gspwndAltTab)) {
        MSGERROR(ERROR_INVALID_WINDOW_HANDLE);
    }

    /*
     * Probe arguments
     */
    try {
        ProbeForWrite(pati, sizeof(*pati), DATAALIGN);
        if (bAnsi) {
            ProbeForWriteBuffer((LPSTR)lpszItemText, cchItemText, CHARALIGN);
        } else {
            ProbeForWriteBuffer(lpszItemText, cchItemText, CHARALIGN);
        }

        /*
         * Validate AltTabInfo structure
         */
        if (pati->cbSize != sizeof(ALTTABINFO)) {
            RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "ALTTABINFO.cbSize %d is wrong", pati->cbSize);
            MSGERROR(0);
        }
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    /*
     * Get the alt tab info
     */
    ati.cbSize = sizeof(ALTTABINFO);
    retval = _GetAltTabInfo(iItem, &ati, lpszItemText, cchItemText, bAnsi);
    if (retval) {
        try {
            *pati = ati;
        } except (StubExceptionHandler(TRUE)) {
            MSGERROR(0);
        }
    }

    TRACE("NtUserGetAltTabInfo");
    ENDRECV_HWNDOPT_SHARED();
}

BOOL NtUserGetMenuBarInfo(
    IN HWND hwnd,
    IN long idObject,
    IN long idItem,
    IN OUT PMENUBARINFO pmbi)
{
    MENUBARINFO mbi;

    BEGINRECV_HWNDLOCK(BOOL, FALSE, hwnd);

    /*
     * Probe arguments
     */
    try {
#if defined(_X86_)
        ProbeForWrite(pmbi, sizeof(*pmbi), sizeof(BYTE));
#else
        ProbeForWrite(pmbi, sizeof(*pmbi), sizeof(DWORD));
#endif
        mbi.cbSize = pmbi->cbSize;
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    /*
     * Get the menubar info
     */
    retval = xxxGetMenuBarInfo(pwnd, idObject, idItem, &mbi);

    if (retval) {
        try {
            *pmbi = mbi;
        } except (StubExceptionHandler(TRUE)) {
            MSGERROR(0);
        }
    }

    TRACE("NtUserGetMenuBarInfo");
    ENDRECV_HWNDLOCK();
}

BOOL NtUserSetInternalWindowPos(  // private SetInternalWindowPos
    IN HWND hwnd,
    IN UINT cmdShow,
    IN CONST RECT *lpRect,
    IN CONST POINT *lpPoint)
{
    RECT rc;
    POINT pt;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_HWNDLOCK(DWORD, 0, hwnd);

    /*
     * Probe arguments
     */
    try {
        rc = ProbeAndReadRect(lpRect);
        pt = ProbeAndReadPoint(lpPoint);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = xxxSetInternalWindowPos(
            pwnd,
            cmdShow,
            &rc,
            &pt);

    TRACE("NtUserSetInternalWindowPos");
    ENDRECV_HWNDLOCK();
}


BOOL NtUserChangeClipboardChain(
    IN HWND hwndRemove,
    IN HWND hwndNewNext)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    PWND pwndNewNext;
    TL tlpwndNewNext;

    BEGINRECV_HWNDLOCK(DWORD, 0, hwndRemove);

    ValidateHWNDOPT(pwndNewNext, hwndNewNext);

    ThreadLockWithPti(ptiCurrent, pwndNewNext, &tlpwndNewNext);
    retval = xxxChangeClipboardChain(
            pwnd,
            pwndNewNext);

    ThreadUnlock(&tlpwndNewNext);

    TRACE("NtUserChangeClipboardChain");
    ENDRECV_HWNDLOCK();
}

DWORD NtUserCheckMenuItem(
    IN HMENU hmenu,
    IN UINT wIDCheckItem,
    IN UINT wCheck)
{
    PMENU pmenu;

    BEGINATOMICRECV(DWORD, (DWORD)-1);

    TESTFLAGS(wCheck, MF_VALID);

    ValidateHMENUMODIFY(pmenu, hmenu);

    retval = _CheckMenuItem(
            pmenu,
            wIDCheckItem,
            wCheck);

    TRACE("NtUserCheckMenuItem");
    ENDATOMICRECV();
}

HWND NtUserChildWindowFromPointEx(
    IN HWND hwndParent,
    IN POINT point,
    IN UINT flags)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    BEGINRECV_HWND(HWND, NULL, hwndParent);

    retval = (HWND)_ChildWindowFromPointEx(pwnd, point, flags);
    retval = PtoH((PVOID)retval);

    TRACE("NtUserChildWindowFromPointEx");
    ENDRECV_HWND();
}

BOOL NtUserClipCursor(  // API ClipCursor
    IN CONST RECT *lpRect OPTIONAL)
{
    RECT rc;

    BEGINRECV(BOOL, FALSE);

    /*
     * Probe arguments
     */
    if (ARGUMENT_PRESENT(lpRect)) {
        try {
            rc = ProbeAndReadRect(lpRect);
            lpRect = &rc;
        } except (StubExceptionHandler(TRUE)) {
            MSGERROR(0);
        }
    }

    retval = zzzClipCursor(lpRect);

    TRACE("NtUserClipCursor");
    ENDRECV();
}

HACCEL NtUserCreateAcceleratorTable(  // API CreateAcceleratorTableA/W
    IN LPACCEL paccel,
    IN INT cAccel)
{
    BEGINRECV(HACCEL, NULL);

    if (cAccel <= 0) {
        MSGERROR(0);
    }

    /*
     * Probe arguments
     */
    try {
        ProbeForReadBuffer(paccel, cAccel, DATAALIGN);
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    retval = (HACCEL)_CreateAcceleratorTable(
            paccel,
            cAccel * sizeof(ACCEL));
    retval = PtoH((PVOID)retval);

    TRACE("NtUserCreateAcceleratorTable");
    ENDRECV();
}

BOOL NtUserDeleteMenu(
    IN HMENU hmenu,
    IN UINT nPosition,
    IN UINT dwFlags)
{
    PMENU pmenu;
    TL tlpmenu;

    BEGINRECV(BOOL, FALSE);

    TESTFLAGS(dwFlags, MF_VALID);

    ValidateHMENUMODIFY(pmenu, hmenu);
    ThreadLock(pmenu, &tlpmenu);
    retval = xxxDeleteMenu(
            pmenu,
            nPosition,
            dwFlags);
    ThreadUnlock(&tlpmenu);

    TRACE("NtUserDeleteMenu");
    ENDRECV();
}

BOOL NtUserDestroyAcceleratorTable(
    IN HACCEL hAccel)
{
    LPACCELTABLE pat;

    BEGINRECV(BOOL, FALSE);

    ValidateHACCEL(pat, hAccel);

    /*
     * Mark the object for destruction - if it says it's ok to free,
     * then free it.
     */
    if (HMMarkObjectDestroy(pat))
        HMFreeObject(pat);
    retval = TRUE;

    TRACE("NtUserDestroyAcceleratorTable");
    ENDRECV();
}

BOOL NtUserDestroyCursor(
    IN HCURSOR hcurs,
    IN DWORD cmd)
{
    PCURSOR pcurs;

    BEGINATOMICRECV(BOOL, FALSE);

    ValidateHCURSOR(pcurs, hcurs);

    retval = _DestroyCursor(
            pcurs, cmd);

    TRACE("NtUserDestroyCursor");
    ENDATOMICRECV();
}

HANDLE NtUserGetClipboardData(  // API GetClipboardData
    IN  UINT          fmt,
    OUT PGETCLIPBDATA pgcd)
{
    PTHREADINFO    ptiCurrent;
    TL             tlpwinsta;
    PWINDOWSTATION pwinsta;
    GETCLIPBDATA   gcd;

    BEGINRECV(HANDLE, NULL);

    ptiCurrent = PtiCurrent();
    if ((pwinsta = CheckClipboardAccess()) == NULL)
        MSGERROR(0);

    /*
     * Check for jobs with JOB_OBJECT_UILIMIT_READCLIPBOARD
     */
    if (IS_THREAD_RESTRICTED(ptiCurrent, JOB_OBJECT_UILIMIT_READCLIPBOARD)) {
        RIPERR0(ERROR_ACCESS_DENIED, RIP_WARNING, "NtUserGetClipboardData failed for restricted thread");
        MSGERROR(0);
    }

    ThreadLockWinSta(ptiCurrent, pwinsta, &tlpwinsta);

    /*
     * Start out assuming the format requested
     * will be the format returned.
     */
    gcd.uFmtRet = fmt;

    retval = xxxGetClipboardData(pwinsta, fmt, &gcd);

    /*
     * Probe arguments
     */
    try {
        ProbeAndWriteStructure(pgcd, gcd, GETCLIPBDATA);
    } except (StubExceptionHandler(TRUE)) {
        MSGERRORCLEANUP(0);
    }

    CLEANUPRECV();
    ThreadUnlockWinSta(ptiCurrent, &tlpwinsta);

    TRACE("NtUserGetClipboardData");
    ENDRECV();

}

BOOL NtUserDestroyMenu(
    IN HMENU hmenu)
{
    PMENU pmenu;

    BEGINATOMICRECV(BOOL, FALSE);

    ValidateHMENUMODIFY(pmenu, hmenu);

    retval = _DestroyMenu(pmenu);

    TRACE("NtUserDestroyMenu");
    ENDATOMICRECV();
}

BOOL NtUserDestroyWindow(
    IN HWND hwnd)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    BEGINRECV_HWND(DWORD, 0, hwnd);

    retval  = xxxDestroyWindow(pwnd);

    TRACE("NtUserDestroyWindow");
    ENDRECV_HWND();
}

LRESULT NtUserDispatchMessage(  // API DispatchMessageA/W
    IN CONST MSG *pmsg)
{
    MSG msg;

    BEGINRECV(LRESULT, 0);

    /*
     * Probe arguments
     */
    try {
        msg = ProbeAndReadMessage(pmsg);
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    retval = xxxDispatchMessage(&msg);

    TRACE("NtUserDispatchMessage");
    ENDRECV();
}

BOOL NtUserEnableMenuItem(
    IN HMENU hMenu,
    IN UINT wIDEnableItem,
    IN UINT wEnable)
{
    PMENU pmenu;
    TL tlmenu;

    BEGINRECV(BOOL, -1);

    TESTFLAGS(wEnable, MF_VALID);

    ValidateHMENUMODIFY(pmenu, hMenu);

    ThreadLockAlways(pmenu, &tlmenu);
    retval = xxxEnableMenuItem(
            pmenu,
            wIDEnableItem,
            wEnable);
    ThreadUnlock(&tlmenu);

    TRACE("NtUserEnableMenuItem");
    ENDRECV();
}

BOOL NtUserAttachThreadInput(
    IN DWORD idAttach,
    IN DWORD idAttachTo,
    IN BOOL fAttach)
{
    PTHREADINFO ptiAttach;
    PTHREADINFO ptiAttachTo;

    BEGINRECV(BOOL, FALSE);

    /*
     * Always must attach or detach from a real thread id.
     */
    if ((ptiAttach = PtiFromThreadId(idAttach)) == NULL) {
        MSGERROR(0);
    }
    if ((ptiAttachTo = PtiFromThreadId(idAttachTo)) == NULL) {
        MSGERROR(0);
    }

    retval = zzzAttachThreadInput(
            ptiAttach,
            ptiAttachTo,
            fAttach);

    TRACE("NtUserAttachThreadInput");
    ENDRECV();
}

BOOL NtUserGetWindowPlacement(  // API GetWindowPlacement
    IN HWND hwnd,
    OUT PWINDOWPLACEMENT pwp)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    /*
     * Note -- this routine updates the checkpoint, so it needs exclusive
     * use of the crit sect.
     */

    WINDOWPLACEMENT wp;
    BEGINRECV_HWND(DWORD, 0, hwnd);

    /*
     * Probe arguments
     */
    try {
        ProbeForWriteWindowPlacement(pwp);
        wp.length = pwp->length;
#ifdef LATER
        if (pwp->length != sizeof(WINDOWPLACEMENT)) {
            if (TestWF(pwnd, WFWIN40COMPAT)) {
                RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "GetWindowPlacement: invalid length %lX", pwp->length);
                MSGERROR(0);
            } else {
                RIPMSG1(RIP_WARNING, "GetWindowPlacement: invalid length %lX", pwp->length);
                pwp->length = sizeof(WINDOWPLACEMENT);
            }
        }
#endif

    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    retval = _GetWindowPlacement(pwnd, &wp);

    try {
        RtlCopyMemory(pwp, &wp, sizeof (WINDOWPLACEMENT));
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    TRACE("NtUserGetWindowPlacement");
    ENDRECV_HWND();
}

BOOL NtUserSetWindowPlacement(  // API SetWindowPlacement
    IN HWND hwnd,
    IN CONST WINDOWPLACEMENT *pwp)
{
    WINDOWPLACEMENT wp;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_HWNDLOCK_ND(BOOL, FALSE, hwnd);

    /*
     * Probe arguments
     */
    try {
        wp = ProbeAndReadWindowPlacement(pwp);
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    if (wp.length != sizeof(WINDOWPLACEMENT)) {
        if (Is400Compat(ptiCurrent->dwExpWinVer)) {
            RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "SetWindowPlacement: invalid length %lX", pwp->length);
            MSGERROR(0);
        } else {
            RIPMSG1(RIP_WARNING, "SetWindowPlacement: invalid length %lX", pwp->length);
        }
    }

    retval = xxxSetWindowPlacement(pwndND, &wp);

    TRACE("NtUserSetWindowPlacement");
    ENDRECV_HWNDLOCK_ND();
}

BOOL NtUserLockWindowUpdate(  // API LockWindowUpdate
    IN HWND hwnd)
{
    PWND pwnd;

    BEGINATOMICRECV(BOOL, FALSE);

    ValidateHWNDOPT(pwnd, hwnd);

    retval = LockWindowUpdate2(pwnd, FALSE);

    TRACE("NtUserLockWindowUpdate");
    ENDATOMICRECV();
}

BOOL NtUserGetClipCursor(  // API GetClipCursor
    OUT LPRECT lpRect)
{
    /*
     * Check if the caller has the proper access rights: if not, this will
     * SetLastError to ERROR_ACCESS_DENIED and return FALSE.  Do this *before*
     * BEGINRECV_SHARED, else we must use MSGERROR to release the critsect!
     */
    RETURN_IF_ACCESS_DENIED(PpiCurrent()->amwinsta,
                            WINSTA_READATTRIBUTES,
                            FALSE);
    {
        BEGINRECV_SHARED(BOOL, FALSE);

        /*
         * Probe arguments
         */
        try {
            ProbeForWriteRect(lpRect);

            *lpRect = grcCursorClip;
            retval = TRUE;
        } except (StubExceptionHandler(TRUE)) {
            MSGERROR(0);
        }

        TRACE("NtUserGetClipCursor");
        ENDRECV_SHARED();
    }
}

BOOL NtUserEnableScrollBar(
    IN HWND hwnd,
    IN UINT wSBflags,
    IN UINT wArrows)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_HWNDLOCK_ND(BOOL, FALSE, hwnd);

    LIMITVALUE(wSBflags, SB_MAX, "EnableScrollBar");

    retval = xxxEnableScrollBar(
            pwndND,
            wSBflags,
            wArrows);

    TRACE("NtUserEnableScrollBar");
    ENDRECV_HWNDLOCK_ND();
}

BOOL NtUserDdeSetQualityOfService(  // API DdeSetQualityOfService
    IN HWND hwndClient,
    IN CONST SECURITY_QUALITY_OF_SERVICE *pqosNew,
    OUT PSECURITY_QUALITY_OF_SERVICE pqosPrev OPTIONAL)
{
    SECURITY_QUALITY_OF_SERVICE qosNew, qosPrev;

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    BEGINRECV_HWND(BOOL, FALSE, hwndClient);

    if (GETPTI(pwnd) != PtiCurrent()) {
        MSGERROR(0);
    }

    /*
     * Probe arguments
     */
    try {
        qosNew = ProbeAndReadStructure(pqosNew, SECURITY_QUALITY_OF_SERVICE);
        if (ARGUMENT_PRESENT(pqosPrev))
            ProbeForWrite(pqosPrev, sizeof(*pqosPrev), sizeof(DWORD));

    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    retval = _DdeSetQualityOfService(
                pwnd,
                &qosNew,
                &qosPrev);

    try {
        if (ARGUMENT_PRESENT(pqosPrev))
            *pqosPrev = qosPrev;
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    TRACE("NtUserDdeSetQualityOfService");
    ENDRECV_HWND();
}

BOOL NtUserDdeGetQualityOfService(  // private DdeGetQualityOfService
    IN HWND hwndClient,
    IN HWND hwndServer,
    OUT PSECURITY_QUALITY_OF_SERVICE pqos)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    PWND pwndServer;
    PTHREADINFO ptiCurrent;
    SECURITY_QUALITY_OF_SERVICE qos;

    BEGINATOMICRECV_HWND(BOOL, FALSE, hwndClient);

    ValidateHWNDOPT(pwndServer, hwndServer);
    ptiCurrent = PtiCurrent();
    if (GETPTI(pwnd) != ptiCurrent && pwndServer != NULL &&
            GETPTI(pwndServer) != ptiCurrent) {
        MSGERROR(0);
    }

    /*
     * Probe arguments
     */
    try {
        ProbeForWrite(pqos, sizeof(*pqos), DATAALIGN);

    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }
    retval = _DdeGetQualityOfService(
                pwnd,
                pwndServer,
                &qos);
    try {
        RtlCopyMemory(pqos, &qos, sizeof (SECURITY_QUALITY_OF_SERVICE));
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    TRACE("NtUserDdeGetQualityOfService");
    ENDATOMICRECV_HWND();
}

DWORD NtUserGetMenuIndex(
    IN HMENU hMenu,
    IN HMENU hSubMenu)
{

    PMENU pmenu;
    PMENU psubmenu;
    DWORD idx;

    BEGINRECV_SHARED(DWORD, 0);

    ValidateHMENU(pmenu, hMenu);
    ValidateHMENU(psubmenu, hSubMenu);

    retval = (DWORD)-1;

    if (pmenu && psubmenu) {
        for (idx=0; idx<pmenu->cItems; idx++)
            if ((pmenu->rgItems[idx].spSubMenu == psubmenu)) {
                retval = idx;
                break;
            }
    }

    TRACE("NtUserGetMenuIndex");
    ENDRECV_SHARED();
}

VOID NtUserSetRipFlags(DWORD dwRipFlags, DWORD dwPID)
{
    BEGINRECV_VOID();

    _SetRipFlags(dwRipFlags, dwPID);

    TRACEVOID("NtUserSetRipFlags");
    ENDRECV_VOID();
}

VOID NtUserSetDbgTag(int tag, DWORD dwBitFlags)
{
    BEGINRECV_VOID();

    _SetDbgTag(tag, dwBitFlags);

    TRACEVOID("NtUserSetRipFlags");
    ENDRECV_VOID();
}

ULONG_PTR NtUserCallNoParam(
    IN DWORD xpfnProc)
{
    BEGINRECV(ULONG_PTR, 0);

    VALIDATEXPFNPROC(NOPARAM);

    retval = (apfnSimpleCall[xpfnProc]());
    if (ISXPFNPROCINRANGE(NOPARAMANDRETURNHANDLE)) {
        retval = (ULONG_PTR)PtoH((PVOID)retval);
    }

    TRACE(apszSimpleCallNames[xpfnProc]);
    ENDRECV();
}

ULONG_PTR NtUserCallOneParam(
    IN ULONG_PTR dwParam,
    IN DWORD xpfnProc)
{
    BEGINRECV(ULONG_PTR, 0);

    VALIDATEXPFNPROC(ONEPARAM);

    retval = (apfnSimpleCall[xpfnProc](dwParam));
    if (ISXPFNPROCINRANGE(ONEPARAMANDRETURNHANDLE)) {
        retval = (ULONG_PTR)PtoH((PVOID)retval);
    }

    TRACE(apszSimpleCallNames[xpfnProc]);
    ENDRECV();
}

ULONG_PTR NtUserCallHwnd(
    IN HWND hwnd,
    IN DWORD xpfnProc)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    BEGINRECV_HWNDLOCK(ULONG_PTR, 0, hwnd);

    VALIDATEXPFNPROC(HWND);

    retval = (apfnSimpleCall[xpfnProc](pwnd));

    TRACE(apszSimpleCallNames[xpfnProc]);
    ENDRECV_HWNDLOCK();
}

ULONG_PTR NtUserCallHwndLock(
    IN HWND hwnd,
    IN DWORD xpfnProc)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_HWNDLOCK(ULONG_PTR, 0, hwnd);

    VALIDATEXPFNPROC(HWNDLOCK);

    retval = (apfnSimpleCall[xpfnProc](pwnd));

    TRACE(apszSimpleCallNames[xpfnProc]);
    ENDRECV_HWNDLOCK();
}

ULONG_PTR NtUserCallHwndOpt(
    IN HWND hwnd,
    IN DWORD xpfnProc)
{
    PWND pwnd;

    BEGINATOMICRECV(ULONG_PTR, 0);

    ValidateHWNDOPT(pwnd, hwnd);

    VALIDATEXPFNPROC(HWNDOPT);

    retval = (apfnSimpleCall[xpfnProc](pwnd));

    TRACE(apszSimpleCallNames[xpfnProc]);
    ENDATOMICRECV();
}

ULONG_PTR NtUserCallHwndParam(
    IN HWND hwnd,
    IN ULONG_PTR dwParam,
    IN DWORD xpfnProc)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    BEGINRECV_HWNDLOCK(ULONG_PTR, 0, hwnd);

    VALIDATEXPFNPROC(HWNDPARAM);

    retval = (apfnSimpleCall[xpfnProc](pwnd, dwParam));
    if (ISXPFNPROCINRANGE(HWNDPARAMANDRETURNHANDLE)) {
        retval = (ULONG_PTR)PtoH((PVOID)retval);
    }

    TRACE(apszSimpleCallNames[xpfnProc]);
    ENDRECV_HWNDLOCK();
}

ULONG_PTR NtUserCallHwndParamLock(
    IN HWND hwnd,
    IN ULONG_PTR dwParam,
    IN DWORD xpfnProc)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_HWNDLOCK(ULONG_PTR, 0, hwnd);

    VALIDATEXPFNPROC(HWNDPARAMLOCK);

    retval = (apfnSimpleCall[xpfnProc](pwnd, dwParam));

    TRACE(apszSimpleCallNames[xpfnProc]);
    ENDRECV_HWNDLOCK();
}

ULONG_PTR NtUserCallTwoParam(
    ULONG_PTR dwParam1,
    ULONG_PTR dwParam2,
    IN DWORD xpfnProc)
{
    BEGINRECV(ULONG_PTR, 0);

    VALIDATEXPFNPROC(TWOPARAM);

    retval = (apfnSimpleCall[xpfnProc](dwParam1, dwParam2));

    TRACE(apszSimpleCallNames[xpfnProc]);
    ENDRECV();
}

BOOL NtUserThunkedMenuItemInfo(  // worker for various menu APIs
    IN HMENU hMenu,
    IN UINT nPosition,
    IN BOOL fByPosition,
    IN BOOL fInsert,
    IN LPMENUITEMINFOW lpmii,
    IN PUNICODE_STRING pstrItem OPTIONAL)
{
    PMENU pmenu;
    MENUITEMINFO mii;
    UNICODE_STRING strItem;
    TL tlpmenu;

    BEGINRECV(BOOL, FALSE);

    /*
     * Probe arguments
     * No need to SetLastError because lpmii always the address of
     * local stack structure in USER code, not an application address.
     */
    try {
        mii = ProbeAndReadMenuItem(lpmii);

        if (ARGUMENT_PRESENT(pstrItem)) {
            strItem = ProbeAndReadUnicodeString(pstrItem);
            ProbeForReadUnicodeStringBuffer(strItem);
        } else {
            RtlInitUnicodeString(&strItem, NULL);
        }
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    ValidateHMENUMODIFY(pmenu, hMenu);

    ThreadLock(pmenu, &tlpmenu);
    /*
     * These routines only use the buffer in a try/except (actually in
     * xxxSetLPITEMInfo).
     */
    if (fInsert) {
        retval = xxxInsertMenuItem(
                pmenu,
                nPosition,
                fByPosition,
                &mii,
                &strItem);
    } else {
        retval = xxxSetMenuItemInfo(
                pmenu,
                nPosition,
                fByPosition,
                &mii,
                &strItem);
    }
    ThreadUnlock(&tlpmenu);

    TRACE("NtUserThunkedMenuItemInfo");
    ENDRECV();
}

/***************************************************************************\
* NtUserThunkedMenuInfo
*
* History:
*  07-23-96 GerardoB - Added header & fixed up for 5.0
\***************************************************************************/
BOOL NtUserThunkedMenuInfo(  // API SetMenuInfo
    IN HMENU hMenu,
    IN LPCMENUINFO lpmi)
{
    PMENU pmenu;
    MENUINFO mi;
    TL tlpmenu;

    BEGINRECV(BOOL, FALSE);

    /*
     * Probe arguments
     */
    try {
        mi = ProbeAndReadMenuInfo(lpmi);
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    ValidateHMENUMODIFY(pmenu, hMenu);

    ThreadLock(pmenu, &tlpmenu);
    retval = xxxSetMenuInfo(pmenu, &mi);
    ThreadUnlock(&tlpmenu);

    TRACE("NtUserThunkedMenuInfo");
    ENDRECV();
}

BOOL NtUserSetMenuDefaultItem(
    IN HMENU hMenu,
    IN UINT wID,
    IN UINT fByPosition)
{
    PMENU pmenu;

    BEGINATOMICRECV(BOOL, FALSE);

    ValidateHMENUMODIFY(pmenu, hMenu);

    retval = _SetMenuDefaultItem(
            pmenu,
            wID,
            fByPosition);

    TRACE("NtUserSetMenuDefaultItem");
    ENDATOMICRECV();
}

BOOL NtUserSetMenuContextHelpId(
    IN HMENU hMenu,
    IN DWORD dwContextHelpId)
{
    PMENU pmenu;

    BEGINATOMICRECV(BOOL, FALSE);

    ValidateHMENUMODIFY(pmenu, hMenu);

    retval = _SetMenuContextHelpId(
            pmenu,
            dwContextHelpId);

    TRACE("NtUserSetMenuContextHelpId");
    ENDATOMICRECV();
}

BOOL NtUserSetMenuFlagRtoL(
    IN HMENU hMenu)
{
    PMENU pmenu;

    BEGINATOMICRECV(BOOL, FALSE);

    ValidateHMENU(pmenu, hMenu);

    retval = _SetMenuFlagRtoL(pmenu);

    TRACE("NtUserSetMenuFlagRtoL");
    ENDATOMICRECV();
}

BOOL NtUserDrawAnimatedRects(  // API DrawAnimatedRects
    IN HWND hwnd,
    IN int idAni,
    IN CONST RECT *lprcFrom,
    IN CONST RECT *lprcTo)
{
    PWND pwnd;
    TL tlpwnd;
    RECT rcFrom;
    RECT rcTo;

    BEGINRECV(BOOL, FALSE);

    ValidateHWNDOPT(pwnd, hwnd);

    /*
     * Probe arguments
     */
    try {
        rcFrom = ProbeAndReadRect(lprcFrom);
        rcTo = ProbeAndReadRect(lprcTo);
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    ThreadLock(pwnd, &tlpwnd);

    retval = xxxDrawAnimatedRects(
        pwnd,
        idAni,
        &rcFrom,
        &rcTo
        );

    ThreadUnlock(&tlpwnd);

    TRACE("NtUserDrawAnimatedRects");
    ENDRECV();
}

BOOL NtUserDrawCaption(  // API DrawCaption
    IN HWND hwnd,
    IN HDC hdc,
    IN CONST RECT *lprc,
    IN UINT flags)
{
    RECT rc;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_HWNDLOCK(DWORD, FALSE, hwnd);

    /*
     * Probe arguments
     */
    try {
        rc = ProbeAndReadRect(lprc);
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    retval = xxxDrawCaptionTemp(pwnd, hdc, &rc, NULL, NULL, NULL, flags);

    TRACE("NtUserDrawCaption");
    ENDRECV_HWNDLOCK();
}

BOOL NtUserPaintDesktop(
    IN HDC hdc)
{
    PTHREADINFO ptiCurrent;
    PWND        pwndDesk;
    TL          tlpwndDesk;

    BEGINRECV(BOOL, FALSE);

    ptiCurrent = PtiCurrent();

    if (ptiCurrent->rpdesk != NULL) {
        pwndDesk = ptiCurrent->rpdesk->pDeskInfo->spwnd;
        ThreadLockWithPti(ptiCurrent, pwndDesk, &tlpwndDesk);
        retval = xxxInternalPaintDesktop(pwndDesk, hdc, TRUE);
        ThreadUnlock(&tlpwndDesk);
    } else {
        MSGERROR(0);
    }

    TRACE("NtUserPaintDesktop");
    ENDRECV();
}

SHORT NtUserGetAsyncKeyState(
    IN int vKey)
{

    PTHREADINFO ptiCurrent;
    BEGINRECV_SHARED(SHORT, 0);


    ptiCurrent = PtiCurrentShared();
    UserAssert(ptiCurrent);

    /*
     * Don't allow other processes to spy on other deskops or a process
     * to spy on the foreground if the desktop does not allow input spying
     */
    if ((ptiCurrent->rpdesk != grpdeskRitInput) ||
            ( ((gptiForeground == NULL) || (PpiCurrent() != gptiForeground->ppi)) &&
              !RtlAreAnyAccessesGranted(ptiCurrent->amdesk, (DESKTOP_HOOKCONTROL | DESKTOP_JOURNALRECORD)))) {
        RIPERR0(ERROR_ACCESS_DENIED, RIP_WARNING, "GetAysncKeyState: not"
                " foreground desktop or no desktop hooking (input spying)");
        MSGERROR(0);
    }
    UserAssert(!(ptiCurrent->rpdesk->rpwinstaParent->dwWSF_Flags & WSF_NOIO));

    retval = _GetAsyncKeyState(vKey);

    /*
     * Update the client side key state cache.
     */
    ptiCurrent->pClientInfo->dwAsyncKeyCache = gpsi->dwAsyncKeyCache;
    RtlCopyMemory(ptiCurrent->pClientInfo->afAsyncKeyState,
                  gafAsyncKeyState,
                  CBASYNCKEYCACHE);
    RtlCopyMemory(ptiCurrent->pClientInfo->afAsyncKeyStateRecentDown,
                  gafAsyncKeyStateRecentDown,
                  CBASYNCKEYCACHE);

    TRACE("NtUserGetAsyncKeyState");
    ENDRECV_SHARED();
}

HBRUSH NtUserGetControlBrush(
    IN HWND hwnd,
    IN HDC hdc,
    IN UINT msg)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_HWNDLOCK(HBRUSH, NULL, hwnd);

    if (hdc == NULL)
        MSGERROR(0);

    retval = xxxGetControlBrush(
            pwnd,
            hdc,
            msg);

    TRACE("NtUserGetControlBrush");
    ENDRECV_HWNDLOCK();
}

HBRUSH NtUserGetControlColor(
    IN HWND hwndParent,
    IN HWND hwndCtl,
    IN HDC hdc,
    IN UINT msg)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    PWND pwndCtl;
    TL tlpwndCtl;

    BEGINRECV_HWNDLOCK(HBRUSH, NULL, hwndParent);

    ValidateHWND(pwndCtl, hwndCtl);

    ThreadLockAlwaysWithPti(ptiCurrent, pwndCtl, &tlpwndCtl);

    retval = xxxGetControlColor(
            pwnd,
            pwndCtl,
            hdc,
            msg);

    ThreadUnlock(&tlpwndCtl);

    TRACE("NtUserGetControlColor");
    ENDRECV_HWNDLOCK();
}

BOOL NtUserEndMenu(VOID)
{
    PTHREADINFO ptiCurrent;
    PWND pwnd;

    BEGINRECV(BOOL, FALSE);

    ptiCurrent = PtiCurrent();

    /*
     * The menu might be in the middle of some callback, so calling xxxEndMenu
     *  directly might screw things up. So we post it a message to signal it to
     *  go away at a good moment
     */
    if (ptiCurrent->pMenuState != NULL) {
        pwnd = GetMenuStateWindow(ptiCurrent->pMenuState);

        if (pwnd != NULL) {
            _PostMessage(pwnd, MN_ENDMENU, 0, 0);
        } else {
            /*
             * Is this menu messed up?
             */
            UserAssert(pwnd != NULL);
            ptiCurrent->pMenuState->fInsideMenuLoop = FALSE;
        }
    }

    retval = TRUE;

    TRACEVOID("NtUserEndMenu");
    ENDRECV();
}

int NtUserCountClipboardFormats(
    VOID)
{
    PWINDOWSTATION pwinsta;

    BEGINRECV_SHARED(int, 0);

    /*
     * Blow it off if the caller doesn't have the proper access rights
     */
    if ((pwinsta = CheckClipboardAccess()) == NULL) {
        MSGERROR(0);
    }

    retval = pwinsta->cNumClipFormats;

    TRACE("NtUserCountClipboardFormats");
    ENDRECV_SHARED();
}

DWORD NtUserGetClipboardSequenceNumber(
    VOID)
{
    PWINDOWSTATION pwinsta;

    BEGINRECV_SHARED(DWORD, 0);

    /*
     * Blow it off if the caller doesn't have the proper access rights
     */
    if ((pwinsta = CheckClipboardAccess()) == NULL) {
        MSGERROR(0);
    }

    retval = pwinsta->iClipSequenceNumber;

    TRACE("NtUserGetClipboardSequenceNumber");
    ENDRECV_SHARED();
}

UINT NtUserGetCaretBlinkTime(VOID)
{
    BEGINRECV_SHARED(UINT, 0);

    /*
     * Blow it off if the caller doesn't have the proper access rights.  However, allow the
     * CSR process to use this value internally to the server.  Note that if the client
     * tries to retrieve this value itself, the access check will function normally.
     */
    if ((PpiCurrent()->Process != gpepCSRSS) &&
        (!CheckGrantedAccess(PpiCurrent()->amwinsta, WINSTA_READATTRIBUTES))) {
        MSGERROR(0);
    }

    retval = gpsi->dtCaretBlink;

    TRACE("NtUserGetCaretBlinkTime");
    ENDRECV_SHARED();
}

HWND NtUserGetClipboardOwner(
    VOID)
{
    PWINDOWSTATION pwinsta;

    BEGINRECV_SHARED(HWND, NULL);

    /*
     * Blow it off if the caller doesn't have the proper access rights
     */
    if ((pwinsta = CheckClipboardAccess()) == NULL) {
        MSGERROR(0);
    }

    retval = PtoH(pwinsta->spwndClipOwner);

    TRACE("NtUserGetClipboardOwner");
    ENDRECV_SHARED();
}

HWND NtUserGetClipboardViewer(
    VOID)
{
    PWINDOWSTATION pwinsta;

    BEGINRECV_SHARED(HWND, NULL);

    /*
     * Blow it off if the caller doesn't have the proper access rights
     */
    if ((pwinsta = CheckClipboardAccess()) == NULL) {
        MSGERROR(0);
    }

    retval = PtoH(pwinsta->spwndClipViewer);

    TRACE("NtUserGetClipboardViewer");
    ENDRECV_SHARED();
}

UINT NtUserGetDoubleClickTime(
    VOID)
{
    BEGINRECV_SHARED(UINT, 0);

    /*
     * Blow it off if the caller doesn't have the proper access rights.  However, allow the
     * CSR process to use this value internally to the server.  Note that if the client
     * tries to retrieve this value itself, the access check will function normally.
     */
    if ((PpiCurrent()->Process != gpepCSRSS) &&
        (!CheckGrantedAccess(PpiCurrent()->amwinsta, WINSTA_READATTRIBUTES))) {
        MSGERROR(0);
    }

    retval = gdtDblClk;

    TRACE("NtUserGetDoubleClickTime");
    ENDRECV_SHARED();
}

HWND NtUserGetForegroundWindow(
    VOID)
{
    BEGINRECV_SHARED(HWND, NULL);

    /*
     * Only return a window if there is a foreground queue and the
     * caller has access to the current desktop.
     */
    if (gpqForeground == NULL || gpqForeground->spwndActive == NULL ||
            PtiCurrentShared()->rpdesk != gpqForeground->spwndActive->head.rpdesk) {
        MSGERROR(0);
    }

    retval = PtoHq(gpqForeground->spwndActive);

    TRACE("NtUserGetForegroundWindow");
    ENDRECV_SHARED();
}

HWND NtUserGetOpenClipboardWindow(
    VOID)
{
    PWINDOWSTATION pwinsta;

    BEGINRECV_SHARED(HWND, NULL);

    /*
     * Blow it off if the caller doesn't have the proper access rights
     */
    if ((pwinsta = CheckClipboardAccess()) == NULL) {
        MSGERROR(0);
    }

    retval = PtoH(pwinsta->spwndClipOpen);

    TRACE("NtUserGetOpenClipboardWindow");
    ENDRECV_SHARED();
}

int NtUserGetPriorityClipboardFormat(  // API GetPriorityClipboardFormat
    IN UINT *paFormatPriorityList,
    IN int cFormats)
{
    BEGINRECV_SHARED(int, 0);

    /*
     * Probe arguments
     */
    try {
        ProbeForReadBuffer(paFormatPriorityList, cFormats, DATAALIGN);

        retval = _GetPriorityClipboardFormat(
                paFormatPriorityList,
                cFormats);
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    TRACE("NtUserGetPriorityClipboardFormat");
    ENDRECV_SHARED();
}

HMENU NtUserGetSystemMenu(
    IN HWND hwnd,
    IN BOOL bRevert)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    BEGINRECV_HWNDLOCK(HMENU, NULL, hwnd);

    retval = (HMENU)xxxGetSystemMenu(pwnd, bRevert);
    retval = PtoH((PVOID)retval);

    TRACE("NtUserGetSystemMenu");
    ENDRECV_HWNDLOCK();
}

BOOL NtUserGetUpdateRect(  // API GetUpdateRect
    IN HWND hwnd,
    IN LPRECT prect OPTIONAL,
    IN BOOL bErase)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    RECT rect2;
    BEGINRECV_HWNDLOCK(DWORD, 0, hwnd);

    retval = xxxGetUpdateRect(
            pwnd,
            prect? &rect2:NULL,
            bErase);
    /*
     * Probe arguments
     */
    if (ARGUMENT_PRESENT(prect)) {
        try {
            ProbeAndWriteStructure(prect, rect2, RECT);
        } except (StubExceptionHandler(TRUE)) {
            MSGERROR(0);
        }
    }

    TRACE("NtUserGetUpdateRect");
    ENDRECV_HWNDLOCK();
}

BOOL NtUserHideCaret(
    IN HWND hwnd)
{
    PWND pwnd;

    BEGINRECV(BOOL, FALSE);

    ValidateHWNDOPT(pwnd, hwnd);

    retval = zzzHideCaret(pwnd);

    TRACE("NtUserHideCaret");
    ENDRECV();
}

BOOL NtUserHiliteMenuItem(
    IN HWND hwnd,
    IN HMENU hMenu,
    IN UINT uIDHiliteItem,
    IN UINT uHilite)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    PMENU pmenu;
    TL tlpmenu;

    BEGINRECV_HWNDLOCK(DWORD, 0, hwnd);

    TESTFLAGS(uHilite, MF_VALID);

    ValidateHMENUMODIFY(pmenu, hMenu);

    ThreadLockAlwaysWithPti(ptiCurrent, pmenu, &tlpmenu);

    retval = xxxHiliteMenuItem(
            pwnd,
            pmenu,
            uIDHiliteItem,
            uHilite);

    ThreadUnlock(&tlpmenu);

    TRACE("NtUserHiliteMenuItem");
    ENDRECV_HWNDLOCK();
}

BOOL NtUserInvalidateRect(  // API InvalidateRect
    IN HWND hwnd,
    IN CONST RECT *prect OPTIONAL,
    IN BOOL bErase)
{
    PWND pwnd;
    TL tlpwnd;
    RECT rc;

    BEGINRECV(BOOL, FALSE);

    ValidateHWNDOPT(pwnd, hwnd);

    /*
     * Probe arguments
     */
    if (ARGUMENT_PRESENT(prect)) {
        try {
            rc = ProbeAndReadRect(prect);
            prect = &rc;
        } except (StubExceptionHandler(TRUE)) {
            MSGERROR(0);
        }
    }

    ThreadLock(pwnd, &tlpwnd);

    retval = xxxInvalidateRect(
            pwnd,
            (PRECT)prect,
            bErase);

    ThreadUnlock(&tlpwnd);

    TRACE("NtUserInvalidateRect");
    ENDRECV();
}

BOOL NtUserIsClipboardFormatAvailable(
    IN UINT nFormat)
{
    PWINDOWSTATION pwinsta;

    BEGINRECV_SHARED(BOOL, FALSE);

    /*
     * Blow it off if the caller doesn't have the proper access rights
     */
    if ((pwinsta = CheckClipboardAccess()) == NULL) {
        MSGERROR(0);
    }

    retval = (FindClipFormat(pwinsta, nFormat) != NULL);

    TRACE("NtUserIsClipboardFormatAvailable");
    ENDRECV_SHARED();
}

BOOL NtUserKillTimer(
    IN HWND hwnd,
    IN UINT_PTR nIDEvent)
{
    PWND pwnd;

    BEGINATOMICRECV(BOOL, FALSE);

    ValidateHWNDOPT(pwnd, hwnd);

    retval = _KillTimer(
            pwnd,
            nIDEvent);

    TRACE("NtUserKillTimer");
    ENDATOMICRECV();
}

HWND NtUserMinMaximize(
    IN HWND hwnd,
    IN UINT nCmdShow,
    IN BOOL fKeepHidden)
{
    BEGINRECV_HWNDLOCK_ND(HWND, NULL, hwnd);

    retval = (HWND)xxxMinMaximize(
            pwndND,
            nCmdShow,
            ((fKeepHidden) ? MINMAX_KEEPHIDDEN : 0) | TEST_PUDF(PUDF_ANIMATE));
    retval = PtoH((PVOID)retval);

    TRACE("NtUserMinMaximize");
    ENDRECV_HWNDLOCK_ND();
}

/**************************************************************************\
* NtUserMNDragOver
*
* Called from the IDropTarget interface to let menus update the selection
*  given the mouse position. It also returns the handle of the menu the
*  the index of the item  the point is on.
*
* 10/28/96 GerardoB     Created
\**************************************************************************/
BOOL NtUserMNDragOver(  // worker for menu drag & drop
    IN POINT * ppt,
    OUT PMNDRAGOVERINFO pmndoi)
{
    POINT pt;
    MNDRAGOVERINFO mndoi;

    BEGINRECV(BOOL, FALSE);

    /*
     * No need to SetLastError since ppt and pmndoi are always addresses of
     * local stack variables in USER, not addresses from an application
     */
    try {
        pt = ProbeAndReadPoint(ppt);
    } except (StubExceptionHandler(FALSE)) {
        RIPMSG1(RIP_WARNING, "NtUserMNDragOver: Exception:%#lx", GetExceptionCode());
        MSGERROR(0);
    }

    retval = xxxMNDragOver(&pt, &mndoi);

    if (retval) {
        try {
            ProbeAndWriteStructure(pmndoi, mndoi, MNDRAGOVERINFO);
        } except (StubExceptionHandler(FALSE)) {
            RIPMSG1(RIP_WARNING, "NtUserMNDragOver: Exception:%#lx", GetExceptionCode());
            MSGERROR(0);
        }
    }

    TRACE("NtUserMNDragOver");
    ENDRECV();
}
/**************************************************************************\
* NtUserMNDragLeave
*
* Called from the IDropTarget interface to let the menu clean up
*
* 10/28/96 GerardoB     Created
\**************************************************************************/
BOOL NtUserMNDragLeave(VOID)
{
    BEGINRECV(BOOL, FALSE);
    retval = xxxMNDragLeave();
    TRACE("NtUserMNDragLeave");
    ENDRECV();
}

BOOL NtUserOpenClipboard(  // API OpenClipboard
    IN HWND hwnd,
    OUT PBOOL pfEmptyClient)
{
    PWND pwnd;
    TL tlpwnd;
    BOOL fEmptyClient;

    BEGINRECV(BOOL, FALSE);

    ValidateHWNDOPT(pwnd, hwnd);

    ThreadLock(pwnd, &tlpwnd);

    retval = xxxOpenClipboard(pwnd, &fEmptyClient);

    ThreadUnlock(&tlpwnd);

    /*
     * Probe arguments
     * No need to SetLastError since pfEmptyClient is the address of a local
     * variable in USER client code, not an application address.
     */
    try {
        ProbeAndWriteUlong(pfEmptyClient, fEmptyClient);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    TRACE("NtUserOpenClipboard");
    ENDRECV();
}

BOOL NtUserPeekMessage(
    OUT LPMSG pmsg,
    IN HWND hwnd,
    IN UINT wMsgFilterMin,
    IN UINT wMsgFilterMax,
    IN UINT wRemoveMsg)
{
    MSG msg;

    BEGINRECV(BOOL, FALSE);

    TESTFLAGS(wRemoveMsg, PM_VALID);

    retval = xxxPeekMessage(
            &msg,
            hwnd,
            wMsgFilterMin,
            wMsgFilterMax,
            wRemoveMsg);

    /*
     * Probe and write arguments only if PeekMessage suceeds otherwise
     * we want to leave MSG undisturbed (bug 16224) to be compatible.
     */
    if (retval) {
        try {
            ProbeAndWriteStructure(pmsg, msg, MSG);
        } except (StubExceptionHandler(TRUE)) {
            MSGERROR(0);
        }
    }

    TRACE("NtUserPeekMessage");
    ENDRECV();
}

BOOL NtUserPostMessage(
    IN HWND hwnd,
    IN UINT msg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    PWND pwnd;

    BEGINRECV(BOOL, FALSE);

    /*
     * Prevent apps from setting hi 16 bits so we can use them internally.
     */
    if (msg & MSGFLAG_MASK) {
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "Invalid message");
        MSGERROR(0);
    }

    switch ((ULONG_PTR)hwnd) {
    case -1:
    case 0x0000FFFF:
        pwnd = PWND_BROADCAST;
        break;

    case 0:
        pwnd = NULL;
        break;

    default:
        if ((pwnd = ValidateHwnd(hwnd)) == NULL) {
            /*
             * We fake terminates to dead windows! (SAS)
             */
            errret = (msg == WM_DDE_TERMINATE);
            MSGERROR(0);
        }
        break;
    }

    retval = _PostMessage(
            pwnd,
            msg,
            wParam,
            lParam);

    TRACE("NtUserPostMessage");
    ENDRECV();
}

BOOL NtUserSendNotifyMessage(  // API SendNotifyMessageA/W
    IN HWND hwnd,
    IN UINT Msg,
    IN WPARAM wParam,
    IN LPARAM lParam OPTIONAL)
{
    PWND pwnd;
    TL tlpwnd;
    LARGE_STRING strLParam;

    BEGINRECV(BOOL, FALSE);

    /*
     * Prevent apps from setting hi 16 bits so we can use them internally.
     */
    if (Msg & MSGFLAG_MASK) {
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "Invalid message");
        MSGERROR(0);
    }

    if ((Msg == WM_WININICHANGE || Msg == WM_DEVMODECHANGE) &&
            ARGUMENT_PRESENT(lParam)) {
        try {
            strLParam = ProbeAndReadLargeString((PLARGE_STRING)lParam);
            ProbeForReadUnicodeStringBuffer(strLParam);
            lParam = (LPARAM)&strLParam;
        } except (StubExceptionHandler(TRUE)) {
            MSGERROR(0);
        }
    }

    ValidateHWNDFF(pwnd, hwnd);

    if (pwnd != PWND_BROADCAST)
        ThreadLockAlways(pwnd, &tlpwnd);

    retval = xxxSendNotifyMessage(
            pwnd,
            Msg,
            wParam,
            lParam );

    if (pwnd != PWND_BROADCAST)
        ThreadUnlock(&tlpwnd);

    TRACE("NtUserSendNotifyMessage");
    ENDRECV();
}

BOOL NtUserSendMessageCallback(
    IN HWND hwnd,
    IN UINT wMsg,
    IN WPARAM wParam,
    IN LPARAM lParam,
    IN SENDASYNCPROC lpResultCallBack,
    IN ULONG_PTR dwData)
{
    PWND pwnd;
    TL tlpwnd;

    BEGINRECV(BOOL, FALSE);

    /*
     * Prevent apps from setting hi 16 bits so we can use them internally.
     */
    if (wMsg & MSGFLAG_MASK) {
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "Invalid message");
        MSGERROR(0);
    }

    ValidateHWNDFF(pwnd, hwnd);

    if (pwnd != PWND_BROADCAST)
        ThreadLockAlways(pwnd, &tlpwnd);

    retval = xxxSendMessageCallback(
            pwnd,
            wMsg,
            wParam,
            lParam,
            lpResultCallBack,
            dwData,
            TRUE );

    if (pwnd != PWND_BROADCAST)
        ThreadUnlock(&tlpwnd);

    TRACE("NtUserSendMessageCallback");
    ENDRECV();
}

BOOL NtUserRegisterHotKey(
    IN HWND hwnd,
    IN int id,
    IN UINT fsModifiers,
    IN UINT vk)
{
    PWND pwnd;

    BEGINATOMICRECV(BOOL, FALSE);

    TESTFLAGS(fsModifiers, MOD_VALID);

    ValidateHWNDOPT(pwnd, hwnd);

    retval = _RegisterHotKey(
            pwnd,
            id,
            fsModifiers,
            vk
            );

    TRACE("NtUserRegisterHotKey");
    ENDATOMICRECV();
}

BOOL NtUserRemoveMenu(
    IN HMENU hmenu,
    IN UINT nPosition,
    IN UINT dwFlags)
{
    PMENU pmenu;
    TL tlpmenu;

    BEGINRECV(BOOL, FALSE);

    TESTFLAGS(dwFlags, MF_VALID);

    ValidateHMENUMODIFY(pmenu, hmenu);

    ThreadLock( pmenu, &tlpmenu);
    retval = xxxRemoveMenu(
            pmenu,
            nPosition,
            dwFlags);
    ThreadUnlock(&tlpmenu);

    TRACE("NtUserRemoveMenu");
    ENDRECV();
}

BOOL NtUserScrollWindowEx(  // API ScrollWindowEx
    IN HWND hwnd,
    IN int dx,
    IN int dy,
    IN CONST RECT *prcScroll OPTIONAL,
    IN CONST RECT *prcClip OPTIONAL,
    IN HRGN hrgnUpdate,
    OUT LPRECT prcUpdate OPTIONAL,
    IN UINT flags)
{
    RECT rcScroll;
    RECT rcClip;
    RECT rcUpdate;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_HWNDLOCK(DWORD, 0, hwnd);

    /*
     * Probe arguments
     */
    try {
        if (ARGUMENT_PRESENT(prcScroll)) {
            rcScroll = ProbeAndReadRect(prcScroll);
            prcScroll = &rcScroll;
        }
        if (ARGUMENT_PRESENT(prcClip)) {
            rcClip = ProbeAndReadRect(prcClip);
            prcClip = &rcClip;
        }

    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    retval = xxxScrollWindowEx(
                pwnd,
                dx,
                dy,
                (PRECT)prcScroll,
                (PRECT)prcClip,
                hrgnUpdate,
                prcUpdate ? &rcUpdate : NULL,
                flags);

    if (ARGUMENT_PRESENT(prcUpdate)) {
        try {
            ProbeAndWriteStructure(prcUpdate, rcUpdate, RECT);
        } except (StubExceptionHandler(TRUE)) {
            MSGERROR(0);
        }
    }

    TRACE("NtUserScrollWindow");
    ENDRECV_HWNDLOCK();
}

HWND NtUserSetActiveWindow(
    IN HWND hwnd)
{
    PWND pwnd;
    TL tlpwnd;

    BEGINRECV(HWND, NULL);

    ValidateHWNDOPT(pwnd, hwnd);

    ThreadLock(pwnd, &tlpwnd);

    retval = (HWND)xxxSetActiveWindow(pwnd);
    retval = PtoH((PVOID)retval);

    ThreadUnlock(&tlpwnd);

    TRACE("NtUserSetActiveWindow");
    ENDRECV();
}

HWND NtUserSetCapture(
    IN HWND hwnd)
{
    PWND pwnd;
    TL tlpwnd;

    BEGINRECV(HWND, NULL);

    ValidateHWNDOPT(pwnd, hwnd);

    ThreadLock(pwnd, &tlpwnd);

    retval = (HWND)xxxSetCapture(pwnd);
    retval = PtoH((PVOID)retval);

    ThreadUnlock(&tlpwnd);

    TRACE("NtUserSetCapture");
    ENDRECV();
}

WORD NtUserSetClassWord(
    IN HWND hwnd,
    IN int nIndex,
    IN WORD wNewWord)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    BEGINRECV_HWND(WORD, 0, hwnd);

    retval = _SetClassWord(
            pwnd,
            nIndex,
            wNewWord);

    TRACE("NtUserSetClassWord");
    ENDRECV_HWND();
}

HWND NtUserSetClipboardViewer(
    IN HWND hwnd)
{
    PWND pwnd;
    TL tlpwnd;

    BEGINRECV(HWND, NULL);

    ValidateHWNDOPT(pwnd, hwnd);

    ThreadLock(pwnd, &tlpwnd);

    retval = (HWND)xxxSetClipboardViewer(pwnd);
    retval = PtoH((PVOID)retval);

    ThreadUnlock(&tlpwnd);

    TRACE("NtUserSetClipboardViewer");
    ENDRECV();
}

HCURSOR NtUserSetCursor(
    IN HCURSOR hCursor)
{
    PCURSOR pCursor;

    BEGINRECV(HCURSOR, NULL);

    ValidateHCURSOROPT(pCursor, hCursor);

    retval = (HCURSOR)zzzSetCursor(pCursor);
    retval = PtoH((PVOID)retval);

    TRACE("NtUserSetCursor");
    ENDRECV();
}

HWND NtUserSetFocus(
    IN HWND hwnd)
{
    PWND pwnd;
    TL tlpwnd;

    BEGINRECV(HWND, NULL);

    ValidateHWNDOPT(pwnd, hwnd);

    ThreadLock(pwnd, &tlpwnd);

    retval = (HWND)xxxSetFocus(pwnd);
    retval = PtoH((PVOID)retval);

    ThreadUnlock(&tlpwnd);

    TRACE("NtUserSetFocus");
    ENDRECV();
}

BOOL NtUserSetMenu(
    IN HWND  hwnd,
    IN HMENU hmenu,
    IN BOOL  fRedraw)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    PMENU pmenu;
    TL    tlpmenu;

    BEGINRECV_HWNDLOCK_ND(DWORD, 0, hwnd);

    ValidateHMENUOPT(pmenu, hmenu);

    ThreadLockWithPti(ptiCurrent, pmenu, &tlpmenu);

    retval = xxxSetMenu(
            pwndND,
            pmenu,
            fRedraw);

    ThreadUnlock(&tlpmenu);

    TRACE("NtUserSetMenu");
    ENDRECV_HWNDLOCK_ND();
}

HWND NtUserSetParent(
    IN HWND hwndChild,
    IN HWND hwndNewParent)
{

    PWND pwndNewParent;
    TL tlpwndNewParent;

    BEGINRECV_HWNDLOCK_ND(HWND, NULL, hwndChild);

    if (hwndNewParent == NULL) {
        pwndNewParent = _GetDesktopWindow();
    } else if (hwndNewParent == HWND_MESSAGE) {
        pwndNewParent = _GetMessageWindow();
    } else {
        ValidateHWND(pwndNewParent, hwndNewParent);
    }

    ThreadLockWithPti(ptiCurrent, pwndNewParent, &tlpwndNewParent);

    retval = (HWND)xxxSetParent(
            pwndND,
            pwndNewParent);
    retval = PtoH((PVOID)retval);

    ThreadUnlock(&tlpwndNewParent);

    TRACE("NtUserSetParent");
    ENDRECV_HWNDLOCK_ND();
}

int NtUserSetScrollInfo(  // API SetScrollInfo
    IN HWND hwnd,
    IN int nBar,
    IN LPCSCROLLINFO pInfo,
    IN BOOL fRedraw)
{
    SCROLLINFO si;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_HWNDLOCK_ND(DWORD, 0, hwnd);

    LIMITVALUE(nBar, SB_MAX, "SetScrollInfo");

    /*
     * Probe arguments
     */
    try {
        si = ProbeAndReadScrollInfo(pInfo);
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    retval = xxxSetScrollBar(
            pwndND,
            nBar,
            &si,
            fRedraw);

    TRACE("NtUserSetScrollInfo");
    ENDRECV_HWNDLOCK_ND();
}

BOOL NtUserSetSysColors(  // API SetSysColors
    IN int nCount,
    IN CONST INT *pSysColor,
    IN CONST COLORREF *pColorValues,
    IN UINT  uOptions)
{
    LPINT lpSysColors = NULL;
    LPDWORD lpSysColorValues = NULL;
    TL tlName, tlSysColors, tlSysColorValues;
    PUNICODE_STRING pProfileUserName = NULL;
    PTHREADINFO ptiCurrent;

    BEGINRECV(BOOL, FALSE);

    ptiCurrent = PtiCurrent();

    /*
     * Prevent restricted threads from changing global stuff
     */
    if (IS_THREAD_RESTRICTED(ptiCurrent, JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS)) {
        MSGERROR(0);
    }

    /*
     * Probe arguments
     */
    if (nCount) {
        try {
            ProbeForReadBuffer(pSysColor, nCount, DATAALIGN);
            ProbeForReadBuffer(pColorValues, nCount, DATAALIGN);
            lpSysColors = UserAllocPoolWithQuota(nCount * sizeof(*pSysColor), TAG_COLORS);
            if (lpSysColors == NULL) {
                ExRaiseStatus(STATUS_NO_MEMORY);
            }
            RtlCopyMemory(lpSysColors, pSysColor, nCount * sizeof(*pSysColor));
            lpSysColorValues = UserAllocPoolWithQuota(nCount * sizeof(*pColorValues), TAG_COLORVALUES);
            if (lpSysColorValues == NULL) {
                ExRaiseStatus(STATUS_NO_MEMORY);
            }
            RtlCopyMemory(lpSysColorValues, pColorValues, nCount * sizeof(*pColorValues));

        } except (StubExceptionHandler(TRUE)) {
            MSGERRORCLEANUP(0);
        }
    }

    ThreadLockPool(ptiCurrent, lpSysColors, &tlSysColors);
    ThreadLockPool(ptiCurrent, lpSysColorValues, &tlSysColorValues);
    pProfileUserName = CreateProfileUserName(&tlName);
    retval = xxxSetSysColors(pProfileUserName,
                nCount,
                lpSysColors,
                lpSysColorValues,
                uOptions
                );
    FreeProfileUserName(pProfileUserName, &tlName);
    ThreadUnlockPool(ptiCurrent, &tlSysColorValues);
    ThreadUnlockPool(ptiCurrent, &tlSysColors);

    CLEANUPRECV();
    if (lpSysColors)
        UserFreePool(lpSysColors);
    if (lpSysColorValues)
        UserFreePool(lpSysColorValues);

    TRACE("NtUserSetSysColors");
    ENDRECV();
}

UINT_PTR NtUserSetTimer(
    IN HWND hwnd,
    IN UINT_PTR nIDEvent,
    IN UINT wElapse,
    IN TIMERPROC pTimerFunc)
{
    PWND pwnd;

    BEGINATOMICRECV(UINT_PTR, 0);

    ValidateHWNDOPT(pwnd, hwnd);

    /*
     * If we let apps set a timer granularity less then 10 the app
     * spend too long processing timer messages.  Some WOW apps like
     * Paradox in WinStone use zero to effectively get the minimal
     * timer value which was ~55ms in Win 3.1.  We also step this
     * value up for 32 bit apps because the NT timer resolution
     * can very depending if the multimedia timers have turned up
     * the resolution.  If they have NT apps that specify a low value
     * will not work properly because they will eat the CPU processing
     * WM_TIMER messages
     */
    if (wElapse < 10) {
        RIPMSG1(RIP_WARNING, "SetTimer: timeout value was %ld set to 10",
                wElapse);
        wElapse = 10;
    }

    retval = _SetTimer(
            pwnd,
            nIDEvent,
            wElapse,
            (TIMERPROC_PWND)pTimerFunc);

    TRACE("NtUserSetTimer");
    ENDATOMICRECV();
}

LONG_PTR NtUserSetWindowLongPtr(
    IN HWND hwnd,
    IN int nIndex,
    IN LONG_PTR dwNewLong,
    IN BOOL bAnsi)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    BEGINRECV_HWNDLOCK(ULONG_PTR, 0, hwnd);

    retval = xxxSetWindowLongPtr(
            pwnd,
            nIndex,
            dwNewLong,
            bAnsi);

    TRACE("NtUserSetWindowLongPtr");
    ENDRECV_HWNDLOCK();
}

#ifdef _WIN64
LONG NtUserSetWindowLong(
    IN HWND hwnd,
    IN int nIndex,
    IN LONG dwNewLong,
    IN BOOL bAnsi)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    BEGINRECV_HWNDLOCK(DWORD, 0, hwnd);

    retval = xxxSetWindowLong(
            pwnd,
            nIndex,
            dwNewLong,
            bAnsi);

    TRACE("NtUserSetWindowLong");
    ENDRECV_HWNDLOCK();
}
#endif

WORD NtUserSetWindowWord(
    IN HWND hwnd,
    IN int nIndex,
    IN WORD wNewWord)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    BEGINRECV_HWND(WORD, 0, hwnd);

    retval = _SetWindowWord(
            pwnd,
            nIndex,
            wNewWord);

    TRACE("NtUserSetWindowWord");
    ENDRECV_HWND();
}

HHOOK NtUserSetWindowsHookAW(
    IN int nFilterType,
    IN HOOKPROC pfnFilterProc,
    IN DWORD dwFlags)
{
    BEGINRECV(HHOOK, NULL);

    retval = (HHOOK)zzzSetWindowsHookAW(
            nFilterType,
            (PROC)pfnFilterProc,
            dwFlags);

    TRACE("NtUserSetWindowsHookAW");
    ENDRECV();
}

BOOL NtUserShowCaret(
    IN HWND hwnd)
{
    PWND pwnd;

    BEGINRECV(BOOL, FALSE);

    ValidateHWNDOPT(pwnd, hwnd);

    retval = zzzShowCaret(
            pwnd);

    TRACE("NtUserShowCaret");
    ENDRECV();
}

BOOL NtUserShowScrollBar(
    IN HWND hwnd,
    IN int iBar,
    IN BOOL fShow)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_HWNDLOCK_ND(DWORD, 0, hwnd);

    LIMITVALUE(iBar, SB_MAX, "ShowScrollBar");

    retval = xxxShowScrollBar(
            pwndND,
            iBar,
            fShow);

    TRACE("NtUserShowScrollBar");
    ENDRECV_HWNDLOCK_ND();
}

BOOL NtUserShowWindowAsync(
    IN HWND hwnd,
    IN int nCmdShow)
{
    BEGINRECV_HWNDLOCK_ND(BOOL, FALSE, hwnd);

    LIMITVALUE(nCmdShow, SW_MAX, "ShowWindowAsync");

    retval = _ShowWindowAsync(pwndND, nCmdShow, 0);

    TRACE("NtUserShowWindowAsync");
    ENDRECV_HWNDLOCK_ND();
}

BOOL NtUserShowWindow(
    IN HWND hwnd,
    IN int nCmdShow)
{
    BEGINRECV_HWNDLOCK_ND(BOOL, FALSE, hwnd);

    LIMITVALUE(nCmdShow, SW_MAX, "ShowWindow");

    /*
     * Let's not allow the window to be shown/hidden once we
     * started the destruction of the window.
     */
    if (TestWF(pwndND, WFINDESTROY)) {
        RIPERR1(ERROR_INVALID_PARAMETER,
                RIP_WARNING,
                "ShowWindow: Window is being destroyed (%#p)",
                pwndND);
        MSGERROR(0);
    }

    retval = xxxShowWindow(pwndND, nCmdShow | TEST_PUDF(PUDF_ANIMATE));

    TRACE("NtUserShowWindow");
    ENDRECV_HWNDLOCK_ND();
}

BOOL NtUserTrackMouseEvent(  // API TrackMouseEvent
    IN OUT LPTRACKMOUSEEVENT lpTME)
{
    TRACKMOUSEEVENT tme;

    BEGINRECV(BOOL, FALSE);

    /*
     * Probe arguments
     */
    try {
        tme = ProbeAndReadTrackMouseEvent(lpTME);

        if (tme.cbSize != sizeof(tme)) {
            RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "TrackMouseEvent: invalid size %lX", tme.cbSize);
            MSGERROR(0);
        }

        TESTFLAGS(tme.dwFlags, TME_VALID);

    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    if (tme.dwFlags & TME_QUERY) {
        retval = QueryTrackMouseEvent(&tme);
        try {
            RtlCopyMemory(lpTME, &tme, sizeof(tme));
        } except (StubExceptionHandler(TRUE)) {
            MSGERROR(0);
        }
    } else {
        retval = TrackMouseEvent(&tme);
    }

    TRACE("NtUserTrackMouseEvent");
    ENDRECV();
}

BOOL NtUserTrackPopupMenuEx(  // API TrackPopupMenuEx
    IN HMENU hMenu,
    IN UINT uFlags,
    IN int x,
    IN int y,
    IN HWND hwnd,
    IN CONST TPMPARAMS *pparamst OPTIONAL)
{
    PWND pwnd;
    PMENU pmenu;
    TL tlpwnd;
    TL tlpmenu;
    PTHREADINFO ptiCurrent;
    TPMPARAMS paramst;

    BEGINRECV(BOOL, FALSE);

    TESTFLAGS(uFlags, TPM_VALID);

    ValidateHMENU(pmenu, hMenu);
    ValidateHWND(pwnd, hwnd);

    ptiCurrent = PtiCurrent();
    ThreadLockAlwaysWithPti(ptiCurrent, pwnd, &tlpwnd);
    ThreadLockAlwaysWithPti(ptiCurrent, pmenu, &tlpmenu);

    /*
     * Probe arguments
     */
    try {
        if (ARGUMENT_PRESENT(pparamst)) {
            paramst = ProbeAndReadPopupParams(pparamst);
            pparamst = &paramst;
        }

    } except (StubExceptionHandler(TRUE)) {
        MSGERRORCLEANUP(0);
    }
    retval = xxxTrackPopupMenuEx(
                pmenu,
                uFlags,
                x,
                y,
                pwnd,
                pparamst);

    CLEANUPRECV();

    ThreadUnlock(&tlpmenu);
    ThreadUnlock(&tlpwnd);

    TRACE("NtUserTrackPopupMenuEx");
    ENDRECV();
}

BOOL NtUserTranslateMessage(  // API TranslateMessage
    IN CONST MSG *lpMsg,
    IN UINT flags)
{
    MSG msg;

    BEGINRECV(BOOL, FALSE);

    /*
     * Probe arguments
     */
    try {
        msg = ProbeAndReadMessage(lpMsg);
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    if (ValidateHwnd(msg.hwnd) == NULL) {
        MSGERROR(0);
    }

    retval = xxxTranslateMessage(
            &msg,
            flags);

    TRACE("NtUserTranslateMessage");
    ENDRECV();
}

BOOL NtUserUnhookWindowsHookEx(
    IN HHOOK hhk)
{
    PHOOK phk;

    BEGINRECV(BOOL, FALSE);

    ValidateHHOOK(phk, hhk);

    retval = zzzUnhookWindowsHookEx(
            phk);

    TRACE("NtUserUnhookWindowsHookEx");
    ENDRECV();
}

BOOL NtUserUnregisterHotKey(
    IN HWND hwnd,
    IN int id)
{
    PWND pwnd;

    BEGINATOMICRECV(BOOL, FALSE);

    ValidateHWNDOPT(pwnd, hwnd);

    retval = _UnregisterHotKey(
            pwnd,
            id);

    TRACE("NtUserUnregisterHotKey");
    ENDATOMICRECV();
}

BOOL NtUserValidateRect(  // API ValidateRect
    IN HWND hwnd,
    IN CONST RECT *lpRect OPTIONAL)
{
    PWND pwnd;
    TL tlpwnd;
    RECT rc;

    BEGINRECV(BOOL, FALSE);

    /*
     * Probe arguments
     */
    if (ARGUMENT_PRESENT(lpRect)) {
        try {
            rc = ProbeAndReadRect(lpRect);
            lpRect = &rc;
        } except (StubExceptionHandler(TRUE)) {
            MSGERROR(0);
        }
    }

    ValidateHWNDOPT(pwnd, hwnd);

    ThreadLock(pwnd, &tlpwnd);

    retval = xxxValidateRect(pwnd, (PRECT)lpRect);

    ThreadUnlock(&tlpwnd);

    TRACE("NtUserValidateRect");
    ENDRECV();
}

DWORD NtUserWaitForInputIdle(
    IN ULONG_PTR idProcess,
    IN DWORD dwMilliseconds,
    IN BOOL fSharedWow)
{
    BEGINRECV(DWORD, (DWORD)-1);

    retval = xxxWaitForInputIdle(
            idProcess,
            dwMilliseconds,
            fSharedWow);

    TRACE("NtUserWaitForInputIdle");
    ENDRECV();
}

HWND NtUserWindowFromPoint(
    IN POINT Point)
{
    BEGINRECV(HWND, NULL);

    retval = (HWND)xxxWindowFromPoint(
            Point);
    retval = PtoH((PVOID)retval);

    TRACE("NtUserWindowFromPoint");
    ENDRECV();
}

HDC NtUserBeginPaint(  // API BeginPaint
    IN HWND hwnd,
    OUT LPPAINTSTRUCT lpPaint)
{
    PAINTSTRUCT ps;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_HWNDLOCK(HDC, NULL, hwnd);

    retval = xxxBeginPaint(pwnd, &ps);

    /*
     * Probe arguments
     */
    try {
        ProbeAndWriteStructure(lpPaint, ps, PAINTSTRUCT);
    } except (StubExceptionHandler(TRUE)) {
        xxxEndPaint(pwnd, &ps);
        MSGERROR(0);
    }

    TRACE("NtUserBeginPaint");
    ENDRECV_HWNDLOCK();
}

BOOL NtUserCreateCaret(
    IN HWND hwnd,
    IN HBITMAP hBitmap,
    IN int nWidth,
    IN int nHeight)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    BEGINRECV_HWNDLOCK(DWORD, 0, hwnd);

    retval = xxxCreateCaret(
            pwnd,
            hBitmap,
            nWidth,
            nHeight
    );

    TRACE("NtUserCreateCaret");
    ENDRECV_HWNDLOCK();
}

BOOL NtUserEndPaint(  // API EndPaint
    IN HWND hwnd,
    IN CONST PAINTSTRUCT *lpPaint)
{
    PAINTSTRUCT ps;

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    BEGINRECV_HWNDLOCK(BOOL, FALSE, hwnd);

    /*
     * Probe arguments
     */
    try {
        ps = ProbeAndReadPaintStruct(lpPaint);
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    retval = xxxEndPaint(pwnd, &ps);

    TRACE("NtUserEndPaint");
    ENDRECV_HWNDLOCK();
}

int NtUserExcludeUpdateRgn(
    IN HDC hdc,
    IN HWND hwnd)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    BEGINRECV_HWND(int, ERROR, hwnd);

    if (hdc == NULL)
        MSGERROR(0);

    retval = _ExcludeUpdateRgn(hdc, pwnd);

    TRACE("NtUserExcludeUpdateRgn");
    ENDRECV_HWND();
}

HDC NtUserGetDC(
    IN HWND hwnd)
{
    PWND pwnd;
    BOOL bValid = TRUE;

    BEGINATOMICRECV(HDC, NULL);

    ValidateHWNDOPT(pwnd, hwnd);

    if (IS_CURRENT_THREAD_RESTRICTED(JOB_OBJECT_UILIMIT_HANDLES) && pwnd == NULL) {

        PDESKTOP pdesk = PtiCurrent()->rpdesk;

        /*
         * make sure it has access to the desktop window
         */
        if (!ValidateHwnd(PtoH(pdesk->pDeskInfo->spwnd))) {
            bValid = FALSE;
        }
    }

    retval = _GetDC(pwnd);

    if (!bValid) {

        HRGN hrgn;

        /*
         * Select a NULL visible region on this DC so that restricted
         * processes don't mess with GetDC(NULL)
         */
        hrgn = CreateEmptyRgn();

        GreSelectVisRgn(retval, hrgn, SVR_DELETEOLD);
    }

    TRACE("NtUserGetDC");
    ENDATOMICRECV();
}

HDC NtUserGetDCEx(
    IN HWND hwnd,
    IN HRGN hrgnClip,
    IN DWORD flags)
{
    PWND pwnd;

    BEGINATOMICRECV(HDC, NULL);

    ValidateHWNDOPT(pwnd, hwnd);

    if (pwnd == NULL) {
        pwnd = PtiCurrent()->rpdesk->pDeskInfo->spwnd;

        if (IS_CURRENT_THREAD_RESTRICTED(JOB_OBJECT_UILIMIT_HANDLES)) {
            /*
             * make sure it has access to the desktop window
             */
            if (!ValidateHwnd(PtoH(pwnd))) {
                RIPMSG0(RIP_WARNING,
                        "NtUserGetDCEx fails desktop window validation");
                MSGERROR(0);
            }
        }
    }

    retval = _GetDCEx(
            pwnd,
            hrgnClip,
            flags);

    TRACE("NtUserGetDCEx");
    ENDATOMICRECV();
}

HDC NtUserGetWindowDC(
    IN HWND hwnd)
{
    PWND pwnd;

    BEGINATOMICRECV(HDC, NULL);

    ValidateHWNDOPT(pwnd, hwnd);

    retval = _GetWindowDC(pwnd);

    TRACE("NtUserGetWindowDC");
    ENDATOMICRECV();
}

int NtUserGetUpdateRgn(
    IN HWND hwnd,
    IN HRGN hrgn,
    IN BOOL bErase)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_HWNDLOCK(int, ERROR, hwnd);

    retval = xxxGetUpdateRgn(
            pwnd,
            hrgn,
            bErase);

    TRACE("NtUserGetUpdateRgn");
    ENDRECV_HWNDLOCK();
}

BOOL NtUserRedrawWindow(  // API RedrawWindow
    IN HWND hwnd,
    IN CONST RECT *lprcUpdate OPTIONAL,
    IN HRGN hrgnUpdate,
    IN UINT flags)
{
    RECT rc;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_HWNDLOCK_OPT(BOOL, FALSE, hwnd);

    /*
     * Probe arguments
     */
    if (ARGUMENT_PRESENT(lprcUpdate)) {
        try {
            rc = ProbeAndReadRect(lprcUpdate);
            lprcUpdate = &rc;
        } except (StubExceptionHandler(TRUE)) {
            MSGERROR(0);
        }
    }

    TESTFLAGS(flags, RDW_VALIDMASK);

    retval = xxxRedrawWindow(
            pwnd,
            (PRECT)lprcUpdate,
            hrgnUpdate,
            flags);

    TRACE("NtUserRedrawWindow");
    ENDRECV_HWNDLOCK_OPT();
}

BOOL NtUserInvalidateRgn(
    IN HWND hwnd,
    IN HRGN hrgn,
    IN BOOL bErase)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_HWNDLOCK(BOOL, FALSE, hwnd);

    retval = xxxInvalidateRgn(
            pwnd,
            hrgn,
            bErase);

    TRACE("NtUserInvalidateRgn");
    ENDRECV_HWNDLOCK();
}

int NtUserSetWindowRgn(
    IN HWND hwnd,
    IN HRGN hrgn,
    IN BOOL bRedraw)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_HWNDLOCK_ND(int, 0, hwnd);

    retval = xxxSetWindowRgn(
            pwndND,
            hrgn,
            bRedraw);

    TRACE("NtUserSetWindowRgn");
    ENDRECV_HWNDLOCK_ND();
}

BOOL NtUserScrollDC(  // API ScrollDC
    IN HDC hdc,
    IN int dx,
    IN int dy,
    IN CONST RECT *prcScroll OPTIONAL,
    IN CONST RECT *prcClip OPTIONAL,
    IN HRGN hrgnUpdate,
    OUT LPRECT prcUpdate OPTIONAL)
{
    RECT rcScroll;
    RECT rcClip;
    RECT rcUpdate;

    BEGINRECV(BOOL, FALSE);

    /*
     * Probe arguments
     */
    try {
        if (ARGUMENT_PRESENT(prcScroll)) {
            rcScroll = ProbeAndReadRect(prcScroll);
            prcScroll = &rcScroll;
        }
        if (ARGUMENT_PRESENT(prcClip)) {
            rcClip = ProbeAndReadRect(prcClip);
            prcClip = &rcClip;
        }

    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }
    retval = _ScrollDC(
                hdc,
                dx,
                dy,
                (PRECT)prcScroll,
                (PRECT)prcClip,
                hrgnUpdate,
                prcUpdate ? &rcUpdate : NULL);

    if (ARGUMENT_PRESENT(prcUpdate)) {
        try {
            ProbeAndWriteStructure(prcUpdate, rcUpdate, RECT);
        } except (StubExceptionHandler(TRUE)) {
            MSGERROR(0);
        }
    }
    ENDRECV();
}

int NtUserInternalGetWindowText(  // private InternalGetWindowText
    IN HWND hwnd,
    OUT LPWSTR lpString,
    IN int nMaxCount)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    BEGINRECV_HWND_SHARED(DWORD, 0, hwnd);

    if (nMaxCount) {
        /*
         * Probe arguments
         */
        try {
            ProbeForWriteBuffer(lpString, nMaxCount, CHARALIGN);
           /*
            * Initialize string empty.
            */
            *lpString = TEXT('\0');
            if (pwnd->strName.Length) {
                retval = TextCopy(&pwnd->strName, lpString, nMaxCount);
            } else {
                retval = 0;
            }

        } except (StubExceptionHandler(FALSE)) {
            MSGERROR(0); // private API, don't SetLastError
        }
    } else {
        MSGERROR(0);
    }

    TRACE("NtUserInternalGetWindowText");
    ENDRECV_HWND_SHARED();
}

int NtUserGetMouseMovePointsEx(  // API GetMouseMovePointsEx
    IN UINT             cbSize,
    IN CONST MOUSEMOVEPOINT *lppt,
    OUT MOUSEMOVEPOINT *lpptBuf,
    IN UINT             nBufPoints,
    IN DWORD            resolution)
{
    MOUSEMOVEPOINT mmp;
    BEGINRECV(int, -1);

    if (cbSize != sizeof(MOUSEMOVEPOINT) || nBufPoints > MAX_MOUSEPOINTS) {

        RIPERR2(ERROR_INVALID_PARAMETER, RIP_VERBOSE,
                "GetMouseMovePointsEx: invalid cbSize %d or nBufPoints %d",
                cbSize, nBufPoints);
        MSGERROR(0);
    }

    /*
     * Probe arguments
     */
    try {
        mmp = ProbeAndReadStructure(lppt, MOUSEMOVEPOINT);
        ProbeForWriteBuffer(lpptBuf, nBufPoints, DATAALIGN);
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    /*
     * GetMouseMovePointsEx protects itself with a try block.
     * No it doesn't!
     */

    retval = _GetMouseMovePointsEx(&mmp, lpptBuf, nBufPoints, resolution);

    TRACE("NtUserGetMouseMovePointsEx");
    ENDRECV();
}

int NtUserToUnicodeEx(  // API ToUnicode/ToUnicodeEx/ToAscii/ToAsciiEx
    IN UINT wVirtKey,
    IN UINT wScanCode,
    IN CONST BYTE *lpKeyState,
    OUT LPWSTR pwszBuff,
    IN int cchBuff,
    IN UINT wFlags,
    IN HKL hKeyboardLayout)
{
    BYTE KeyState[256];
    WCHAR wcBuff[4];
    LPWSTR pwszBuffK;
    BOOL bAlloc = FALSE;
    PTHREADINFO ptiCurrent;
    TL tlInput;

    BEGINRECV(int, 0);

    if (cchBuff <= 0) {
        MSGERROR(ERROR_INVALID_PARAMETER);
    }

    /*
     * Probe arguments
     */
    try {
        ProbeForRead(lpKeyState, 256, sizeof(BYTE));
        RtlCopyMemory(&KeyState, lpKeyState, 256);
        ProbeForWriteBuffer(pwszBuff, cchBuff, CHARALIGN);
        if (cchBuff < 4) {
            pwszBuffK = wcBuff;
        }else {
            pwszBuffK = UserAllocPoolWithQuota(cchBuff * sizeof(WCHAR), TAG_UNICODEBUFFER);
            if (pwszBuffK == NULL) {
                ExRaiseStatus(STATUS_NO_MEMORY);
            }
            bAlloc = TRUE;
            ptiCurrent = PtiCurrent();
            ThreadLockPool(ptiCurrent, pwszBuffK, &tlInput);
        }
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    retval = xxxToUnicodeEx(
                wVirtKey,
                wScanCode,
                KeyState,
                pwszBuffK,
                cchBuff,
                wFlags,
                hKeyboardLayout);

    try {
        RtlCopyMemory(pwszBuff, pwszBuffK, cchBuff*sizeof(WCHAR));
    } except (StubExceptionHandler(TRUE)) {
        MSGERRORCLEANUP(0);
    }

    CLEANUPRECV();

    if (bAlloc) {
        ThreadUnlockAndFreePool(ptiCurrent, &tlInput);
    }

    TRACE("NtUserToUnicodeEx");
    ENDRECV();
}

BOOL NtUserYieldTask(
    VOID)
{
    PTHREADINFO ptiCurrent;

    BEGINRECV(BOOL, FALSE);

    /*
     * Make sure this process is running in the background if it is just
     * spinning.
     */
    ptiCurrent = PtiCurrent();

    ptiCurrent->pClientInfo->cSpins++;

    /*
     * CheckProcessBackground see input.c for comments
     */
    if (ptiCurrent->pClientInfo->cSpins >= CSPINBACKGROUND) {
        ptiCurrent->pClientInfo->cSpins = 0;
        ptiCurrent->TIF_flags |= TIF_SPINNING;
        ptiCurrent->pClientInfo->dwTIFlags |= TIF_SPINNING;

        if (!(ptiCurrent->ppi->W32PF_Flags & W32PF_FORCEBACKGROUNDPRIORITY)) {
            ptiCurrent->ppi->W32PF_Flags |= W32PF_FORCEBACKGROUNDPRIORITY;
            if (ptiCurrent->ppi == gppiWantForegroundPriority) {
                SetForegroundPriority(ptiCurrent, FALSE);
            }
        }
    }

    retval = xxxUserYield(ptiCurrent);

    TRACE("NtUserYieldTask");
    ENDRECV();
}

BOOL NtUserWaitMessage(
    VOID)
{
    BEGINRECV(BOOL, FALSE);

    retval = xxxWaitMessage();

    TRACE("NtUserWaitMessage");
    ENDRECV();
}

UINT NtUserLockWindowStation(
    IN HWINSTA hwinsta)
{
    PWINDOWSTATION pwinsta;
    NTSTATUS Status;

    BEGINRECV(UINT, 0);

    Status = ValidateHwinsta(hwinsta, UserMode, 0, &pwinsta);
    if (!NT_SUCCESS(Status))
        MSGERROR(0);

    retval = _LockWindowStation(pwinsta);

    ObDereferenceObject(pwinsta);

    TRACE("NtUserLockWindowStation");
    ENDRECV();
}

BOOL NtUserUnlockWindowStation(
    IN HWINSTA hwinsta)
{
    PWINDOWSTATION pwinsta;
    NTSTATUS Status;

    BEGINRECV(BOOL, FALSE);

    Status = ValidateHwinsta(hwinsta, UserMode, 0, &pwinsta);
    if (!NT_SUCCESS(Status))
        MSGERROR(0);

    retval = _UnlockWindowStation(pwinsta);

    ObDereferenceObject(pwinsta);

    TRACE("NtUserUnlockWindowStation");
    ENDRECV();
}

UINT NtUserSetWindowStationUser(  // private SetWindowStationUser
    IN HWINSTA hwinsta,
    IN PLUID pLuidUser,
    IN PSID pSidUser OPTIONAL,
    IN DWORD cbSidUser)
{
    PWINDOWSTATION pwinsta;
    NTSTATUS Status;
    LUID luid;
    BEGINATOMICRECV(UINT, FALSE);

    Status = ValidateHwinsta(hwinsta, UserMode, 0, &pwinsta);
    if (!NT_SUCCESS(Status))
        MSGERROR(0);

    try {
        ProbeForRead(pLuidUser, sizeof(*pLuidUser), sizeof(DWORD));
        luid = *pLuidUser;
        if (ARGUMENT_PRESENT(pSidUser)) {
            ProbeForRead(pSidUser, cbSidUser, sizeof(DWORD));
        }
    } except (StubExceptionHandler(FALSE)) {
        MSGERRORCLEANUP(0);  // don't SetLastError for private API
    }

    /*
     * SetWindowStationUser uses pSidUser in a try block.
     */

    retval = _SetWindowStationUser(pwinsta, &luid, pSidUser, cbSidUser);

    CLEANUPRECV();

    ObDereferenceObject(pwinsta);

    TRACE("NtUserSetWindowStationUser");
    ENDATOMICRECV();
}

BOOL NtUserSetLogonNotifyWindow(
    IN HWND hwnd)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    BEGINRECV_HWND(BOOL, FALSE, hwnd);

    retval = _SetLogonNotifyWindow(pwnd);

    TRACE("NtUserSetLogonNotifyWindow");
    ENDRECV_HWND();
}

BOOL NtUserSetSystemCursor(
    IN HCURSOR hcur,
    IN DWORD id)
{
    PCURSOR pcur;

    BEGINRECV(BOOL, FALSE);

    ValidateHCURSOR(pcur, hcur);

    retval = zzzSetSystemCursor(
            pcur,
            id);

    TRACE("NtUserSetSystemCursor");
    ENDRECV();
}

HCURSOR NtUserGetCursorFrameInfo(  // private GetCursorFrameInfo (Obsolete? - IanJa)
    IN HCURSOR hcur,
    IN int iFrame,
    OUT LPDWORD pjifRate,
    OUT LPINT pccur)
{
    PCURSOR pcur;
    DWORD jifRate;
    INT ccur;

    BEGINRECV_SHARED(HCURSOR, NULL);

    ValidateHCURSOR(pcur, hcur);

    /*
     * Probe arguments
     */
    try {
        ProbeForWriteUlong(pjifRate);
        ProbeForWriteLong(pccur);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);  // don't SetLastError for private API
    }

    retval = (HCURSOR)_GetCursorFrameInfo(
                pcur,
                iFrame,
                &jifRate,
                &ccur);
        retval = PtoH((PVOID)retval);
    try {
        *pjifRate = jifRate;
        *pccur = ccur;
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);  // don't SetLastError for private API
    }

    TRACE("NtUserGetCursorFrameInfo");
    ENDRECV_SHARED();
}

BOOL NtUserSetCursorContents(
    IN HCURSOR hCursor,
    IN HCURSOR hCursorNew)
{
    PCURSOR pCursor;
    PCURSOR pCursorNew;

    BEGINATOMICRECV(BOOL, FALSE);

    ValidateHCURSOR(pCursor, hCursor);
    ValidateHCURSOR(pCursorNew, hCursorNew);

    retval = _SetCursorContents(pCursor, pCursorNew);

    TRACE("NtUserSetCursorContents");
    ENDATOMICRECV();
}

HCURSOR NtUserFindExistingCursorIcon(  // Various Icon/Cursor APIs
    IN PUNICODE_STRING pstrModName,
    IN PUNICODE_STRING pstrResName,
    IN PCURSORFIND     pcfSearch)
{
    ATOM           atomModName;
    UNICODE_STRING strModName;
    UNICODE_STRING strResName;
    PCURSOR        pcurSrc;
    CURSORFIND     cfSearch;

    BEGINRECV_SHARED(HCURSOR, NULL);

    /*
     * Probe arguments
     */
    try {

        cfSearch = ProbeAndReadCursorFind(pcfSearch);

        ValidateHCURSOROPT(pcurSrc, cfSearch.hcur);

        strModName = ProbeAndReadUnicodeString(pstrModName);
        ProbeForReadUnicodeStringBuffer(strModName);

        strResName = ProbeAndReadUnicodeString(pstrResName);
        ProbeForReadUnicodeStringBufferOrId(strResName);

    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    /*
     * The ModName buffer is client-side, but UserFindAtom protects
     * access.
     */

    atomModName = UserFindAtom(strModName.Buffer);

    if (atomModName) {

        /*
         * The ResName buffer is client-side.  FindExistincCursorIcon
         * protects access.
         */
        retval = (HCURSOR)_FindExistingCursorIcon(atomModName,
                                                  &strResName,
                                                  pcurSrc,
                                                  &cfSearch);

        retval = (HCURSOR)PtoH((PCURSOR)retval);

    } else {

        retval = 0;
    }


    TRACE("NtUserFindExistingCursorIcon");
    ENDRECV_SHARED();
}

BOOL NtUserSetCursorIconData(  // worker called by CreateIcon, CreateCursor etc.
    IN HCURSOR         hCursor,
    IN PUNICODE_STRING pstrModName,
    IN PUNICODE_STRING pstrResName,
    IN PCURSORDATA     pData)
{
    UNICODE_STRING strModName;
    UNICODE_STRING strResName;
    PCURSOR        pCursor;
    CURSORDATA     curData;
    DWORD          cbData;

    BEGINATOMICRECV(BOOL, FALSE);

    ValidateHCURSOR(pCursor, hCursor);

    /*
     * Probe arguments
     */
    try {

        strModName = ProbeAndReadUnicodeString(pstrModName);
        strResName = ProbeAndReadUnicodeString(pstrResName);

        ProbeForReadUnicodeStringBuffer(strModName);
        ProbeForReadUnicodeStringBufferOrId(strResName);

        curData = ProbeAndReadCursorData(pData);

        if (curData.CURSORF_flags & CURSORF_ACON) {
            /*
             * Avoid overflow here.  Or we might end up probing less
             * MCostea #199188
             */
            if (HIWORD(curData.cpcur) | HIWORD(curData.cicur)) {
                MSGERROR(0);
            }
            /*
             * The code assumes that the memory was allocated in one chunk
             * as in CreateAniIcon().  To prevent evil apps, do this check.
             */
            if ((INT_PTR)curData.ajifRate != curData.cpcur * (INT_PTR) sizeof(HCURSOR) ||
                (INT_PTR)curData.aicur != (INT_PTR)curData.ajifRate + curData.cicur * (INT_PTR) sizeof(JIF)) {
                MSGERROR(0);
            }
            cbData = (curData.cpcur * sizeof(HCURSOR)) +
                     (curData.cicur * sizeof(JIF)) +
                     (curData.cicur * sizeof(DWORD));

        } else {
            cbData = 0;
        }
        ProbeForRead(curData.aspcur, cbData, sizeof(DWORD));

    } except (StubExceptionHandler(FALSE)) {
        /*
         * probed parameters are USER stack variables, not supplied by the
         * application itself, so don't bother to SetLastError.
         */
        MSGERROR(0);
    }

    /*
     * SetCursorIconData guards use of the buffer addresses
     * with try clauses.
     */
    retval = _SetCursorIconData(pCursor,
                                    &strModName,
                                    &strResName,
                                    &curData,
                                    cbData);

    TRACE("NtUserSetCursorIconData");
    ENDATOMICRECV();
}

BOOL NtUserGetMenuItemRect(  // API GetMenuItemRect
    IN HWND hwnd,
    IN HMENU hMenu,
    IN UINT uItem,
    OUT LPRECT lprcItem)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    PMENU pmenu;
    TL tlpmenu;
    RECT rcItem;

    BEGINRECV_HWNDLOCK_OPT(DWORD, 0, hwnd);

    ValidateHMENU(pmenu, hMenu);

    ThreadLockAlwaysWithPti(ptiCurrent, pmenu, &tlpmenu);

    retval = xxxGetMenuItemRect(
                pwnd,
                pmenu,
                uItem,
                &rcItem);
    /*
     * Probe arguments
     */
    try {
        ProbeAndWriteStructure(lprcItem, rcItem, RECT);
    } except (StubExceptionHandler(TRUE)) {
        MSGERRORCLEANUP(0);
    }

    CLEANUPRECV();

    ThreadUnlock(&tlpmenu);

    TRACE("NtUserGetMenuItemRect");
    ENDRECV_HWNDLOCK_OPT();
}

int NtUserMenuItemFromPoint(  // API MenuItemFromPoint
    IN HWND hwnd,
    IN HMENU hMenu,
    IN POINT ptScreen)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    PMENU pmenu;
    TL tlpmenu;

    BEGINRECV_HWNDLOCK_OPT(DWORD, -1, hwnd);

    ValidateHMENU(pmenu, hMenu);

    ThreadLockAlwaysWithPti(ptiCurrent, pmenu, &tlpmenu);

    retval = xxxMenuItemFromPoint(
            pwnd,
            pmenu,
            ptScreen);

    ThreadUnlock(&tlpmenu);

    TRACE("NtUserMenuItemFromPoint");
    ENDRECV_HWNDLOCK_OPT();
}

BOOL NtUserGetCaretPos(  // API GetCaretPos
    OUT LPPOINT lpPoint)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    PTHREADINFO pti;
    PQ pq;
    BEGINRECV_SHARED(BOOL, FALSE);

    /*
     * Probe arguments
     */
    try {
        ProbeForWritePoint(lpPoint);

        pti = PtiCurrentShared();
        pq = pti->pq;
        lpPoint->x = pq->caret.x;
        lpPoint->y = pq->caret.y;
        retval = TRUE;
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    TRACE("NtUserGetCaretPos");
    ENDRECV_SHARED();
}

BOOL NtUserDefSetText(
    IN HWND hwnd,
    IN PLARGE_STRING pstrText OPTIONAL)
{
    LARGE_STRING strText;

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    BEGINRECV_HWND(DWORD, 0, hwnd);

    /*
     * Probe arguments
     */
    if (ARGUMENT_PRESENT(pstrText)) {
        try {
            strText = ProbeAndReadLargeString(pstrText);
#if defined(_X86_)
            ProbeForRead(strText.Buffer, strText.Length, sizeof(BYTE));
#else
            ProbeForRead(strText.Buffer, strText.Length,
                    strText.bAnsi ? sizeof(BYTE) : sizeof(WORD));
#endif
            pstrText = &strText;
        } except (StubExceptionHandler(TRUE)) {
            MSGERROR(0);  // WM_SETTEXT lParam
        }
    }

    /*
     * pstrText buffer is client side.  DefSetText protects uses of the buffer.
     */
    retval = DefSetText(
            pwnd,
            pstrText);

    TRACE("NtUserDefSetText");
    ENDRECV_HWND();
}

NTSTATUS NtUserQueryInformationThread(
    IN HANDLE hThread,
    IN USERTHREADINFOCLASS ThreadInfoClass,
    OUT PVOID ThreadInformation,
    IN ULONG ThreadInformationLength,
    IN OUT PULONG ReturnLength OPTIONAL)
{
    BEGINRECVCSRSS(NTSTATUS, STATUS_UNSUCCESSFUL);
    /*
     * note -- QueryInformationThread can call xxxSwitchDesktop, so it is not sharable
     */

    /*
     * Probe arguments -- no try/except
     */

#if DBG
    if (ARGUMENT_PRESENT(ThreadInformation)) {
        switch (ThreadInfoClass) {
        case UserThreadShutdownInformation:
        case UserThreadFlags:
        case UserThreadWOWInformation:
        case UserThreadHungStatus:
            ProbeForWriteBoolean((PBOOLEAN)ThreadInformation);
            break;
        case UserThreadTaskName:
            ProbeForWrite(ThreadInformation, ThreadInformationLength,
                    sizeof(WCHAR));
            break;
        }
    }
    if (ARGUMENT_PRESENT(ReturnLength))
        ProbeForWriteUlong(ReturnLength);
#endif

    retval = xxxQueryInformationThread(hThread,
            ThreadInfoClass, ThreadInformation,
            ThreadInformationLength, ReturnLength);

    TRACE("NtUserQueryInformationThread");
    ENDRECVCSRSS();
}

NTSTATUS NtUserSetInformationThread(
    IN HANDLE hThread,
    IN USERTHREADINFOCLASS ThreadInfoClass,
    IN PVOID ThreadInformation,
    IN ULONG ThreadInformationLength)
{
    BEGINRECVCSRSS(NTSTATUS, STATUS_UNSUCCESSFUL);

    /*
     * Probe arguments outside a try clause -- CSRSS
     */
#if DBG
    if (ThreadInfoClass == UserThreadUseDesktop) {
        ProbeForWrite(ThreadInformation, ThreadInformationLength,
                sizeof(DWORD));
    } else {
        ProbeForRead(ThreadInformation, ThreadInformationLength,
                sizeof(DWORD));
    }

#endif

    retval = xxxSetInformationThread(hThread,
                ThreadInfoClass, ThreadInformation,
                ThreadInformationLength);


    TRACE("NtUserSetInformationThread");
    ENDRECVCSRSS();
}

NTSTATUS NtUserSetInformationProcess(
    IN HANDLE hProcess,
    IN USERPROCESSINFOCLASS ProcessInfoClass,
    IN PVOID ProcessInformation,
    IN ULONG ProcessInformationLength)
{
    BEGINRECVCSRSS(NTSTATUS, STATUS_UNSUCCESSFUL);

    /*
     * Probe arguments without try/except
     */
#if DBG
    ProbeForRead(ProcessInformation, ProcessInformationLength,
                sizeof(DWORD));
#endif

    retval = SetInformationProcess(hProcess,
                    ProcessInfoClass, ProcessInformation,
                    ProcessInformationLength);


    TRACE("NtUserSetInformationProcess");
    ENDRECVCSRSS();
}

BOOL NtUserNotifyProcessCreate(
    IN DWORD dwProcessId,
    IN DWORD dwParentThreadId,
    IN ULONG_PTR dwData,
    IN DWORD dwFlags)
{
    extern BOOL xxxUserNotifyProcessCreate(DWORD idProcess, DWORD idParentThread,
            ULONG_PTR dwData, DWORD dwFlags);

    BEGINRECVCSRSS(BOOL, FALSE);

    retval = xxxUserNotifyProcessCreate(dwProcessId,
            dwParentThreadId,
            dwData,
            dwFlags);

    TRACE("NtUserNotifyProcessCreate");
    ENDRECVCSRSS();
}

NTSTATUS NtUserSoundSentry(VOID)
{
    BEGINRECV(NTSTATUS, STATUS_UNSUCCESSFUL);

    retval = (_UserSoundSentryWorker() ?
            STATUS_SUCCESS : STATUS_UNSUCCESSFUL);

    TRACE("NtUserSoundSentry");
    ENDRECV();
}

NTSTATUS NtUserTestForInteractiveUser(  // private _UserTestTokenForInteractive
    IN PLUID pluidCaller)
{
    LUID luidCaller;

    BEGINRECV_SHARED(NTSTATUS, STATUS_UNSUCCESSFUL);

    /*
     * Probe arguments
     */
    try {
        luidCaller = ProbeAndReadStructure(pluidCaller, LUID);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = TestForInteractiveUser(&luidCaller);

    TRACE("NtUserTestForInteractiveUser");
    ENDRECV_SHARED();
}

BOOL NtUserSetConsoleReserveKeys(
    IN HWND hwnd,
    IN DWORD fsReserveKeys)
{
    BOOL _SetConsoleReserveKeys(PWND, DWORD);

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    BEGINRECV_HWND(BOOL, FALSE, hwnd);

    retval = _SetConsoleReserveKeys(pwnd, fsReserveKeys);

    TRACE("NtUserSetConsoleReserveKeys");
    ENDRECV_HWND();
}

VOID NtUserModifyUserStartupInfoFlags(
    IN DWORD dwMask,
    IN DWORD dwFlags)
{
    BEGINRECV_VOID();

    PpiCurrent()->usi.dwFlags = (PpiCurrent()->usi.dwFlags & ~dwMask) | (dwFlags & dwMask);

    TRACEVOID("NtUserModifyUserStartupInfoFlags");
    ENDRECV_VOID();
}

BOOL NtUserSetWindowFNID(
    IN HWND hwnd,
    IN WORD fnid)
{
    BEGINRECV_HWND(BOOL, FALSE, hwnd);

    /*
     * Don't let apps mess with windows on other processes.
     */
    if (GETPTI(pwnd)->ppi != PpiCurrent()) {
        MSGERROR(0);
    }

    /*
     * Make sure the fnid is in the correct range.
     */
    if (fnid != FNID_CLEANEDUP_BIT) {
        if ((fnid < FNID_CONTROLSTART) || (fnid > FNID_CONTROLEND) || (GETFNID(pwnd) != 0)) {
            MSGERROR(0);
        }
    }

    /*
     * Remember what window class this window belongs to.  Can't use
     * the real class because any app can call CallWindowProc()
     * directly no matter what the class is!
     */
    pwnd->fnid |= fnid;
    retval = TRUE;

    TRACE("NtUserSetWindowFNID");
    ENDRECV_HWND();
}

#define AWS_MASK (BS_TYPEMASK | BS_RIGHT | BS_RIGHTBUTTON | \
        WS_HSCROLL | WS_VSCROLL | SS_TYPEMASK)

VOID NtUserAlterWindowStyle(
    IN HWND hwnd,
    IN DWORD mask,
    IN DWORD flags)
{
    BEGINRECV_HWND_VOID(hwnd);

    if (GETPTI(pwnd)->ppi == PpiCurrent()) {

#if DBG
        if (mask & ~AWS_MASK) {
            RIPMSG1(RIP_WARNING, "NtUserAlterWindowStyle: bad mask %x", mask);
        }
#endif

        mask &= AWS_MASK;
        pwnd->style = (pwnd->style & (~mask)) | (flags & mask);
    } else {
        RIPMSG1(RIP_WARNING, "NtUserAlterWIndowStyle: current ppi doesn't own pwnd %#p", pwnd);
    }

    TRACEVOID("NtUserAlterWindowStyle");
    ENDRECV_HWND_VOID();
}

VOID NtUserSetThreadState(
    IN DWORD dwFlags,
    IN DWORD dwMask)
{
    PTHREADINFO ptiCurrent;
    DWORD dwOldFlags;

    if (dwFlags & ~(QF_DIALOGACTIVE)) {
        return;
    }

    BEGINRECV_VOID();

    ptiCurrent = PtiCurrent();
    dwOldFlags = ptiCurrent->pq->QF_flags;
    ptiCurrent->pq->QF_flags ^= ((dwOldFlags ^ dwFlags) & dwMask);

    TRACEVOID("NtUserSetThreadState");
    ENDRECV_VOID();
}


ULONG_PTR NtUserGetThreadState(
    IN USERTHREADSTATECLASS ThreadState)
{
    PTHREADINFO ptiCurrent = PtiCurrentShared();

    BEGINRECV_SHARED(ULONG_PTR, 0);

    switch (ThreadState) {
    case UserThreadStateFocusWindow:
        retval = (ULONG_PTR)HW(ptiCurrent->pq->spwndFocus);
        break;
    case UserThreadStateActiveWindow:
        retval = (ULONG_PTR)HW(ptiCurrent->pq->spwndActive);
        break;
    case UserThreadStateCaptureWindow:
        retval = (ULONG_PTR)HW(ptiCurrent->pq->spwndCapture);
        break;
    case UserThreadStateDefaultImeWindow:
        retval = (ULONG_PTR)HW(ptiCurrent->spwndDefaultIme);
        break;
    case UserThreadStateDefaultInputContext:
        retval = (ULONG_PTR)PtoH(ptiCurrent->spDefaultImc);
        break;
    case UserThreadStateImeCompatFlags:
        UserAssert(ptiCurrent->ppi != NULL);
        retval = (DWORD)(ptiCurrent->ppi->dwImeCompatFlags);
        break;
    case UserThreadStatePreviousKeyboardLayout:
        retval = (ULONG_PTR)(ptiCurrent->hklPrev);
        break;
    case UserThreadStateIsWinlogonThread:
        // Client IMM checks if the process is Login;
        // to prevent switching dictionaries, etc.
        // LATER: gpidLogin per WinStation ?
        retval = (DWORD)(GetCurrentProcessId() == gpidLogon);
        break;
    case UserThreadStateIsConImeThread:
        UserAssert(ptiCurrent->rpdesk != NULL);
        retval = (DWORD)(PtiFromThreadId(ptiCurrent->rpdesk->dwConsoleIMEThreadId) == ptiCurrent);
        break;
    case UserThreadStateInputState:
        retval = (DWORD)_GetInputState();
        break;
    case UserThreadStateCursor:
        retval = (ULONG_PTR)PtoH(ptiCurrent->pq->spcurCurrent);
        break;
    case UserThreadStateChangeBits:
        retval = ptiCurrent->pcti->fsChangeBits;
        break;
    case UserThreadStatePeekMessage:
        /*
         * Update the last read time so that hung app painting won't occur.
         */
        SET_TIME_LAST_READ(ptiCurrent);
        retval = (DWORD)FALSE;
        break;
    case UserThreadStateExtraInfo:
        retval = ptiCurrent->pq->ExtraInfo;
        break;

    case UserThreadStateInSendMessage:
        if (ptiCurrent->psmsCurrent != NULL) {
            if (ptiCurrent->psmsCurrent->ptiSender != NULL) {
                retval = ISMEX_SEND;
            } else if (ptiCurrent->psmsCurrent->flags & (SMF_CB_REQUEST | SMF_CB_REPLY)) {
                retval = ISMEX_CALLBACK;
            } else {
                retval = ISMEX_NOTIFY;
            }

            if (ptiCurrent->psmsCurrent->flags & SMF_REPLY) {
                retval |= ISMEX_REPLIED;
            }
        } else {
            retval = ISMEX_NOSEND;
        }
        break;

    case UserThreadStateMessageTime:
        retval = ptiCurrent->timeLast;
        break;
    case UserThreadStateIsForeground:
        retval = (ptiCurrent->pq == gpqForeground);
        break;
    case UserThreadConnect:
        retval = TRUE;
        break;
    default:
        RIPMSG1(RIP_WARNING, "NtUserGetThreadState invalid ThreadState:%#x", ThreadState);
        MSGERROR(0);
    }

    ENDRECV_SHARED();
}

BOOL NtUserValidateHandleSecure(
    IN HANDLE h)
{
    BEGINRECV(BOOL, FALSE);

    retval = ValidateHandleSecure(h);

    TRACE("NtUserValidateHandleSecure");
    ENDRECV();
}

BOOL NtUserUserHandleGrantAccess( // API UserHandleGrantAccess
    IN HANDLE hUserHandle,
    IN HANDLE hJob,
    IN BOOL   bGrant)
{
    NTSTATUS  Status;
    PEJOB     Job;
    PW32JOB   pW32Job;
    DWORD     dw;
    PHE       phe;
    PULONG_PTR pgh;
    BOOL      retval;
    BOOL      errret = FALSE;

    Status = ObReferenceObjectByHandle(
                    hJob,
                    JOB_OBJECT_SET_ATTRIBUTES,
                    *PsJobType,
                    UserMode,
                    (PVOID*)&Job,
                    NULL);

    if (!NT_SUCCESS(Status)) {
        RIPERR1(ERROR_INVALID_PARAMETER,
                RIP_WARNING,
                "UserHandleGrantAccess: invalid job handle %#p\n",
                hJob);
        return FALSE;
    }

    /*
     * aquire the job's lock and after that enter the user
     * critical section.
     */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusive(&Job->JobLock, TRUE);

    EnterCrit();

    /*
     * bail out if it doesn't have UI restrictions
     */
    if (Job->UIRestrictionsClass == 0) {
        RIPERR1(ERROR_INVALID_PARAMETER,
                RIP_WARNING,
                "UserHandleGrantAccess: job %#p doesn't have UI restrictions\n",
                hJob);
        MSGERRORCLEANUP(0);
    }

    /*
     * see if we have a W32JOB structure created for this job
     */
    pW32Job = gpJobsList;

    while (pW32Job) {
        if (pW32Job->Job == Job) {
            break;
        }
        pW32Job = pW32Job->pNext;
    }

    UserAssert(pW32Job != NULL);

    try {
        /*
         * Now, validate the 'unsecure' handle
         */
        if (HMValidateHandle(hUserHandle, TYPE_GENERIC) == NULL) {
            RIPERR1(ERROR_INVALID_PARAMETER,
                    RIP_WARNING,
                    "UserHandleGrantAccess: invalid handle %#p\n",
                    hUserHandle);

            MSGERRORCLEANUP(0);
        }

        dw = HMIndexFromHandle(hUserHandle);

        phe = &gSharedInfo.aheList[dw];

        phe->bFlags |= HANDLEF_GRANTED;

        pgh = pW32Job->pgh;

        if (bGrant) {
            /*
             * Add the handle to the process' list
             */
            if (pW32Job->ughCrt == pW32Job->ughMax) {

                if (pW32Job->ughCrt == 0) {
                    pgh = UserAllocPool(GH_SIZE * sizeof(*pgh), TAG_GRANTEDHANDLES);
                } else {
                    /*
                     * we need to grow the array
                     */
                    DWORD uBytes = (pW32Job->ughMax) * sizeof(*pgh);

                    pgh = UserReAllocPool(pgh,
                                          uBytes,
                                          uBytes + GH_SIZE * sizeof(*pgh),
                                          TAG_GRANTEDHANDLES);
                }

                if (pgh == NULL) {
                    RIPMSG0(RIP_WARNING, "UserHandleGrantAccess: out of memory\n");
                    MSGERRORCLEANUP(ERROR_NOT_ENOUGH_MEMORY);
                }

                pW32Job->pgh     = pgh;
                pW32Job->ughMax += GH_SIZE;
            }

            UserAssert(pW32Job->ughCrt < pW32Job->ughMax);

            /*
             * see if the handle is not already granted to this process
             */
            for (dw = 0; dw < pW32Job->ughCrt; dw++) {
                if (*(pgh + dw) == (ULONG_PTR)hUserHandle) {
                    break;
                }
            }

            if (dw >= pW32Job->ughCrt) {

                /*
                 * add the handle to the granted handles table
                 */
                *(pgh + pW32Job->ughCrt) = (ULONG_PTR)hUserHandle;

                (pW32Job->ughCrt)++;
            }
        } else {
            /*
             * Remove the handle from the granted list
             */
            /*
             * search for the handle in the array.
             */
            for (dw = 0; dw < pW32Job->ughCrt; dw++) {
                if (*(pgh + dw) == (ULONG_PTR)hUserHandle) {

                    /*
                     * found the handle granted to this process
                     */
                    RtlMoveMemory(pgh + dw,
                                  pgh + dw + 1,
                                  (pW32Job->ughCrt - dw - 1) * sizeof(*pgh));

                    (pW32Job->ughCrt)--;
                    break;
                }
            }
#if DBG
            if (dw >= pW32Job->ughCrt) {
                RIPERR1(ERROR_INVALID_HANDLE, RIP_WARNING,
                        "UserHandleGrantAccess(FALSE): handle not found %#p",
                        hUserHandle);
            }
#endif // DBG
        }

        retval = TRUE;

    } except (StubExceptionHandler(TRUE)) {
        MSGERRORCLEANUP(0);
    }

    CLEANUPRECV();

    LeaveCrit();
    ExReleaseResource(&Job->JobLock);
    KeLeaveCriticalRegion();
    ObDereferenceObject(Job);

    TRACE("NtUserUserHandleGrantAccess");

    return retval;
}

HWND NtUserCreateWindowEx(
    IN DWORD dwExStyle,
    IN PLARGE_STRING pstrClassName,
    IN PLARGE_STRING pstrWindowName OPTIONAL,
    IN DWORD dwStyle,
    IN int x,
    IN int y,
    IN int nWidth,
    IN int nHeight,
    IN HWND hwndParent,
    IN HMENU hmenu,
    IN HANDLE hModule,
    IN LPVOID pParam,
    IN DWORD dwFlags)
{
    LARGE_STRING strClassName;
    LARGE_STRING strWindowName;
    PWND pwndParent;
    PMENU pmenu;
    TL tlpwndParent;
    TL tlpmenu;
    BOOL fLockMenu = FALSE;
    PTHREADINFO ptiCurrent;

    BEGINRECV(HWND, NULL);

    if (hwndParent != HWND_MESSAGE) {
        ValidateHWNDOPT(pwndParent, hwndParent);
    } else
        pwndParent = _GetMessageWindow();


    /*
     * Win3.1 only checks for WS_CHILD before treating pmenu as an id. This
     * is a bug, because throughout the code, the real check is TestwndChild(),
     * which checks (style & (WS_CHILD | WS_POPUP)) == WS_CHILD. This is
     * because old style "iconic popup" is WS_CHILD | WS_POPUP. So... if on
     * win3.1 an app used ws_iconicpopup, menu validation would not occur
     * (could crash if hmenu != NULL). On Win32, check for the real thing -
     * but allow NULL!
     */
    ptiCurrent = PtiCurrent();
    if (((dwStyle & (WS_CHILD | WS_POPUP)) != WS_CHILD) &&
            (hmenu != NULL)) {
        ValidateHMENU(pmenu, hmenu);

        ThreadLockAlwaysWithPti(ptiCurrent, pmenu, &tlpmenu);
        fLockMenu = TRUE;

    } else {
        pmenu = (PMENU)hmenu;
    }

    /*
     * Mask out the new 5.0 extended style bits for apps
     * that would try to use them and we'll fail in xxxCreateWindowEx
     */
    if (GetAppCompatFlags2(VER40) & GACF2_NO50EXSTYLEBITSCW) {

#if DBG
        if (dwExStyle & ~(WS_EX_VALID40 | WS_EX_INTERNAL)) {
            RIPMSG0(RIP_WARNING, "CreateWindowEx: appcompat removed 5.0 EX bits");
        }
#endif

        dwExStyle &= (WS_EX_VALID40 | WS_EX_INTERNAL);
    }
    
    /*
     * Probe arguments
     */
    try {
#if defined(_X86_)
        if (IS_PTR(pstrClassName)) {
            strClassName = ProbeAndReadLargeString(pstrClassName);
            ProbeForRead(strClassName.Buffer, strClassName.Length,
                    sizeof(BYTE));
            pstrClassName = &strClassName;
        }
        if (ARGUMENT_PRESENT(pstrWindowName)) {
            strWindowName = ProbeAndReadLargeString(pstrWindowName);
            ProbeForRead(strWindowName.Buffer, strWindowName.Length,
                    sizeof(BYTE));
            pstrWindowName = &strWindowName;
        }
#else
        if (IS_PTR(pstrClassName)) {
            strClassName = ProbeAndReadLargeString(pstrClassName);
            ProbeForRead(strClassName.Buffer, strClassName.Length,
                    sizeof(WORD));
            pstrClassName = &strClassName;
        }
        if (ARGUMENT_PRESENT(pstrWindowName)) {
            strWindowName = ProbeAndReadLargeString(pstrWindowName);
            ProbeForRead(strWindowName.Buffer, strWindowName.Length,
                    (strWindowName.bAnsi ? sizeof(BYTE) : sizeof(WORD)));
            pstrWindowName = &strWindowName;
        }
#endif
    } except (StubExceptionHandler(TRUE)) {
        MSGERRORCLEANUP(0);
    }

    ThreadLockWithPti(ptiCurrent, pwndParent, &tlpwndParent);

    /*
     * The buffers for ClassName and WindowName are still in client space.
     */

    retval = (HWND)xxxCreateWindowEx(
            dwExStyle,
            pstrClassName,
            pstrWindowName,
            dwStyle,
            x,
            y,
            nWidth,
            nHeight,
            pwndParent,
            pmenu,
            hModule,
            pParam,
            dwFlags);
    retval = PtoH((PVOID)retval);

    ThreadUnlock(&tlpwndParent);

    CLEANUPRECV();
    if (fLockMenu)
        ThreadUnlock(&tlpmenu);

    TRACE("NtUserCreateWindowEx");
    ENDRECV();
}

NTSTATUS NtUserBuildHwndList(  // worker for EnumWindows, EnumThreadWindows etc.
    IN HDESK hdesk,
    IN HWND hwndNext,
    IN BOOL fEnumChildren,
    IN DWORD idThread,
    IN UINT cHwndMax,
    OUT HWND *phwndFirst,
    OUT PUINT pcHwndNeeded)
{
    PWND pwndNext;
    PDESKTOP pdesk;
    PBWL pbwl;
    PTHREADINFO pti;
    UINT cHwndNeeded;
    UINT wFlags = BWL_ENUMLIST;
    BEGINATOMICRECV(NTSTATUS, STATUS_INVALID_HANDLE);

    if (IS_IME_ENABLED()) {
        // special treatment of IME Window
        wFlags |= BWL_ENUMIMELAST;
    }

    /*
     * Validate prior to referencing the desktop
     */
    ValidateHWNDOPT(pwndNext, hwndNext);

    if (idThread) {
        pti = PtiFromThreadId(idThread);
        if (pti == NULL || pti->rpdesk == NULL){
            MSGERROR(ERROR_INVALID_PARAMETER);
        }
        pwndNext = pti->rpdesk->pDeskInfo->spwnd->spwndChild;
    } else {
        pti = NULL;
    }

    if (hdesk) {
        retval = ValidateHdesk(hdesk, UserMode, DESKTOP_READOBJECTS, &pdesk);
        if (!NT_SUCCESS(retval)){
            MSGERROR(ERROR_INVALID_HANDLE);
        }
        pwndNext = pdesk->pDeskInfo->spwnd->spwndChild;
    } else {
        pdesk = NULL;
    }


    if (pwndNext == NULL) {
        /*
         * Bug: 262004 joejo
         * If we have a valid desk (just no windows on it), then we need
         * to fall through. Otherwise, we'll just grab the current desktop and enum it's
         * windows!
         */
        if (pdesk == NULL) {
            if (pti != NULL) {
                pwndNext = pti->rpdesk->pDeskInfo->spwnd->spwndChild;
            } else {
                pwndNext = _GetDesktopWindow()->spwndChild;
            }
        }
    } else {
        if (fEnumChildren) {
            wFlags |= BWL_ENUMCHILDREN;
            pwndNext = pwndNext->spwndChild;
        }
    }

    if ((pbwl = BuildHwndList(pwndNext, wFlags, pti)) == NULL) {
        MSGERRORCLEANUP(ERROR_NOT_ENOUGH_MEMORY);
    }

    cHwndNeeded = (UINT)(pbwl->phwndNext - pbwl->rghwnd) + 1;

    /*
     * Probe arguments
     */
    try {
        ProbeForWriteBuffer(phwndFirst, cHwndMax, sizeof(DWORD));
        ProbeForWriteUlong(pcHwndNeeded);

       /*
        * If we have enough space, copy out list of hwnds to user mode buffer.
        */
        if (cHwndNeeded <= cHwndMax) {
            RtlCopyMemory(phwndFirst, pbwl->rghwnd, cHwndNeeded * sizeof(HWND));
            retval = STATUS_SUCCESS;
        } else {
            retval = STATUS_BUFFER_TOO_SMALL;
        }
        *pcHwndNeeded = cHwndNeeded;
    } except (StubExceptionHandler(TRUE)) {
        MSGERRORCLEANUP(0);  // phwndFirst/pcHwndNeeded are USER's, not app's
    }

    CLEANUPRECV();

    if (pbwl != NULL) {
        FreeHwndList(pbwl);
    }

    if (pdesk != NULL) {
        LogDesktop(pdesk, LD_DEREF_VALIDATE_HDESK4, FALSE, (ULONG_PTR)PtiCurrent());
        ObDereferenceObject(pdesk);
    }

    TRACE("NtUserBuildHwndList");
    ENDATOMICRECV();
}

NTSTATUS NtUserBuildPropList(  // worker for EnumProps etc.
    IN HWND hwnd,
    IN UINT cPropMax,
    OUT PPROPSET pPropSet,
    OUT PUINT pcPropNeeded)
{
    BEGINRECV_HWNDLOCK(NTSTATUS, STATUS_INVALID_HANDLE, hwnd);

    if (cPropMax == 0) {
        MSGERROR(0);
    }

    /*
     * Probe arguments
     */
    try {
        ProbeForWriteBuffer(pPropSet, cPropMax, sizeof(DWORD));
        ProbeForWriteUlong(pcPropNeeded);

        retval = _BuildPropList(
                pwnd,
                pPropSet,
                cPropMax,
                pcPropNeeded);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);  // pPropSet/pcPropNeed are USER's, not app's
    }

    TRACE("NtUserBuildPropList");
    ENDRECV_HWNDLOCK();
}

NTSTATUS NtUserBuildNameList(  // worker for EnumWindowStations/EnumDesktops
    IN HWINSTA hwinsta,
    IN UINT cbNameList,
    OUT PNAMELIST pNameList,
    OUT PUINT pcbNeeded)
{
    UINT cbNeeded;
    PWINDOWSTATION pwinsta = NULL;

    BEGINRECV_SHARED(NTSTATUS, STATUS_INVALID_HANDLE);

    if (cbNameList < sizeof(NAMELIST)) {
        MSGERROR(0);
    }

    try {
        ProbeForWriteUlong(pcbNeeded);
        ProbeForWrite(pNameList, cbNameList, sizeof(DWORD));
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    if (hwinsta != NULL) {
        retval = ValidateHwinsta(hwinsta, UserMode, WINSTA_ENUMDESKTOPS, &pwinsta);
    } else {
        retval = STATUS_SUCCESS;
    }

    if (!NT_SUCCESS(retval)) {
        try {
            *pNameList->awchNames = 0;
            pNameList->cb = 1;
        } except (StubExceptionHandler(FALSE)) {
            MSGERROR(0);
        }

     }  else {
        /*
         * Note -- pNameList is a client-side pointers.
         *         BuildNameList protects access with try blocks.
         */

        retval = _BuildNameList(
                    pwinsta,
                    pNameList,
                    cbNameList,
                    &cbNeeded);
        try {
            *pcbNeeded = cbNeeded;
        } except (StubExceptionHandler(FALSE)) {
            retval = STATUS_ACCESS_VIOLATION;
        }
    }

    if (pwinsta != NULL)
        ObDereferenceObject(pwinsta);

    TRACE("NtUserBuildNameList");
    ENDRECV_SHARED();
}

HKL NtUserActivateKeyboardLayout(
    IN HKL hkl,
    IN UINT Flags)
{
    BEGINRECV(HKL, NULL);

    /*
     * Prevent restricted threads from setting the keyboard layout
     * for the entire system.
     */
    if (IS_CURRENT_THREAD_RESTRICTED(JOB_OBJECT_UILIMIT_HANDLES)) {
        MSGERROR(0);
    }

    retval = (HKL)xxxActivateKeyboardLayout(
                     _GetProcessWindowStation(NULL),
                     hkl,
                     Flags, NULL);

    TRACE("NtUserActivateKeyboardLayout");
    ENDRECV();
}

HKL NtUserLoadKeyboardLayoutEx(
    IN HANDLE hFile,
    IN DWORD offTable,
    IN HKL hkl,
    IN PUNICODE_STRING pstrKLID,
    IN UINT KbdInputLocale,
    IN UINT Flags)
{
    UNICODE_STRING strKLID;
    PWINDOWSTATION pwinsta;
    WCHAR awchKF[sizeof(((PKL)0)->spkf->awchKF)];
    UINT chMax;

    BEGINRECV(HKL, NULL);

    TESTFLAGS(Flags, KLF_VALID);

    pwinsta = _GetProcessWindowStation(NULL);

    /*
     * Probe arguments
     */
    try {
        strKLID = ProbeAndReadUnicodeString(pstrKLID);
        ProbeForRead(strKLID.Buffer, strKLID.Length, CHARALIGN);
        chMax = min(sizeof(awchKF) - sizeof(WCHAR), strKLID.Length) / sizeof(WCHAR);
        wcsncpy(awchKF, strKLID.Buffer, chMax);
        awchKF[chMax] = 0;
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    retval = xxxLoadKeyboardLayoutEx(
            pwinsta,
            hFile,
            hkl,
            offTable,
            awchKF,
            KbdInputLocale,
            Flags);

    TRACE("NtUserLoadKeyboardLayoutEx");
    ENDRECV();
}

BOOL NtUserUnloadKeyboardLayout(
    IN HKL hkl)
{
    BEGINRECV(BOOL, FALSE);

    retval = xxxUnloadKeyboardLayout(
                     _GetProcessWindowStation(NULL),
                     hkl);

    TRACE("NtUserUnloadKeyboardLayout");
    ENDRECV();
}

BOOL NtUserSetSystemMenu(
    IN HWND hwnd,
    IN HMENU hmenu)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    PMENU pmenu;
    TL tlpmenu;

    BEGINRECV_HWNDLOCK(DWORD, 0, hwnd);

    ValidateHMENU(pmenu, hmenu);

    ThreadLockAlwaysWithPti(ptiCurrent, pmenu, &tlpmenu);

    retval =  xxxSetSystemMenu(pwnd, pmenu);

    ThreadUnlock(&tlpmenu);

    TRACE("NtUserSetSystemMenu");
    ENDRECV_HWNDLOCK();
}

BOOL NtUserDragDetect(
    IN HWND hwnd,
    IN POINT pt)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_HWNDLOCK(DWORD, 0, hwnd);

    retval = xxxDragDetect(pwnd, pt);

    TRACE("NtUserDragDetect");
    ENDRECV_HWNDLOCK();
}

UINT_PTR NtUserSetSystemTimer(
    IN HWND hwnd,
    IN UINT_PTR nIDEvent,
    IN DWORD dwElapse,
    IN WNDPROC pTimerFunc)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_HWND(UINT_PTR, 0, hwnd);

    UNREFERENCED_PARAMETER(pTimerFunc);

    retval = _SetSystemTimer(pwnd,
            nIDEvent,
            dwElapse,
            NULL);

    TRACE("NtUserSetSystemTimer");
    ENDRECV_HWND();
}

BOOL NtUserQuerySendMessage(  // private QuerySendMessage
    OUT PMSG pmsg OPTIONAL)
{
    PSMS psms;
    BEGINRECV_SHARED(BOOL, FALSE);

    /*
     * This function looks like dead code. If it's not, it can be optimized
     * by using the CTIF_INSENDMESSAGE flag from user mode. - JerrySh
     */
    RIPMSG0(RIP_ERROR, "I don't think QuerySendMessage ever gets called. Remove this assert if it does.");

    if ((psms = PtiCurrentShared()->psmsCurrent) == NULL) {
        MSGERROR(0);
    }

    retval = TRUE;
    if (ARGUMENT_PRESENT(pmsg)) {
        try {
            ProbeForWriteMessage(pmsg);
            pmsg->hwnd = HW(psms->spwnd);
            pmsg->message = psms->message;
            pmsg->wParam = psms->wParam;
            pmsg->lParam = psms->lParam;
            pmsg->time = psms->tSent;
            pmsg->pt.x = 0;
            pmsg->pt.y = 0;
            retval = TRUE;
        } except (StubExceptionHandler(FALSE)) {
            MSGERROR(0);
        }
    }

    TRACE("NtUserQuerySendMessage");
    ENDRECV_SHARED();
}

UINT NtUserSendInput(
    IN UINT    cInputs,
    IN CONST INPUT *pInputs,
    IN int     cbSize)
{
    LPINPUT pInput2 = NULL;
    PTHREADINFO ptiCurrent;
    TL tlInput;
    DWORD dwArgumentError = ERROR_INVALID_PARAMETER;

    BEGINRECV(UINT, 0);

    if (sizeof(INPUT) != cbSize || cInputs == 0) {
        MSGERROR(dwArgumentError);
    }

    ptiCurrent = PtiCurrent();

    /*
     * Probe arguments
     */
    try {
        ProbeForReadBuffer(pInputs, cInputs, DATAALIGN);

        pInput2 = UserAllocPoolWithQuota(cInputs * sizeof(*pInputs), TAG_SENDINPUT);
        if (pInput2 == NULL) {
            ExRaiseStatus(STATUS_NO_MEMORY);
        }
        RtlCopyMemory(pInput2, pInputs, cInputs * sizeof(*pInputs));
    } except (StubExceptionHandler(TRUE)) {
        MSGERRORCLEANUP(0);
    }

    ThreadLockPool(ptiCurrent, pInput2, &tlInput);
    retval = xxxSendInput(cInputs, pInput2);
    ThreadUnlockPool(ptiCurrent, &tlInput);
CLEANUPRECV();
    if (pInput2) {
        UserFreePool(pInput2);
    }
    TRACE("NtUserSendInput");
    ENDRECV();
}

UINT NtUserBlockInput(
    IN BOOL fBlockIt)
{
    BEGINATOMICRECV(BOOL, FALSE);
    retval = _BlockInput(fBlockIt);
    TRACE("NtUserBlockInput");
    ENDATOMICRECV();
}

BOOL NtUserImpersonateDdeClientWindow(
    IN HWND hwndClient,
    IN HWND hwndServer)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    PWND pwndServer;

    BEGINATOMICRECV_HWND(BOOL, FALSE, hwndClient);

    ValidateHWND(pwndServer, hwndServer);
    if (GETPTI(pwndServer) != PtiCurrent()) {
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_VERBOSE, "");
        MSGERROR(0);
    }

    if (GETPWNDPPI(pwnd) == GETPWNDPPI(pwndServer)) {
        retval = TRUE;  // impersonating self is a NOOP
    } else {
        retval = _ImpersonateDdeClientWindow(pwnd, pwndServer);
    }

    TRACE("NtUserImpersonateDdeClientWindow");
    ENDATOMICRECV_HWND();
}

ULONG_PTR NtUserGetCPD(
    IN HWND hwnd,
    IN DWORD options,
    IN ULONG_PTR dwData)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    BEGINRECV_HWND(ULONG_PTR, 0, hwnd);

    switch (options & ~CPD_TRANSITION_TYPES) {
    case CPD_WND:
    case CPD_DIALOG:
    case CPD_WNDTOCLS:
        break;
    default:
        RIPMSG1(RIP_WARNING, "GetCPD: Invalid options %x", options);
        MSGERROR(0);
    }

    retval = GetCPD(pwnd, options, dwData);

    TRACE("NtUserGetCPD");
    ENDRECV_HWND();
}

int NtUserCopyAcceleratorTable(  // API CopyAcceleratorTableA/W
    IN HACCEL hAccelSrc,
    IN OUT LPACCEL lpAccelDst OPTIONAL,
    IN int cAccel)
{
    LPACCELTABLE pat;
    int i;
    BEGINATOMICRECV(int, 0);

    ValidateHACCEL(pat, hAccelSrc);

    if (lpAccelDst == NULL) {
        retval = pat->cAccel;
    } else {

        /*
         * Probe arguments
         */
        try {
            ProbeForWriteBuffer(lpAccelDst, cAccel, DATAALIGN);

            if (cAccel > (int)pat->cAccel)
                cAccel = pat->cAccel;

            retval = cAccel;
            for (i = 0; i < cAccel; i++) {
                RtlCopyMemory(&lpAccelDst[i], &pat->accel[i], sizeof(ACCEL));
                lpAccelDst[i].fVirt &= ~FLASTKEY;
            }
        } except (StubExceptionHandler(TRUE)) {
            MSGERROR(0);
        }
    }

    TRACE("NtUserCopyAcceleratorTable");
    ENDATOMICRECV();
}

HWND NtUserFindWindowEx(  // API FindWindowA/W, FindWindowExA/W
    IN HWND hwndParent,
    IN HWND hwndChild,
    IN PUNICODE_STRING pstrClassName,
    IN PUNICODE_STRING pstrWindowName,
    DWORD dwType)
{
    UNICODE_STRING  strClassName;
    UNICODE_STRING  strWindowName;
    PWND            pwndParent, pwndChild;

    BEGINATOMICRECV(HWND, NULL);

    if (hwndParent != HWND_MESSAGE) {
        ValidateHWNDOPT(pwndParent, hwndParent);
    } else
        pwndParent = _GetMessageWindow();

    ValidateHWNDOPT(pwndChild,  hwndChild);

    /*
     * Probe arguments
     */
    try {
        strClassName = ProbeAndReadUnicodeString(pstrClassName);
        strWindowName = ProbeAndReadUnicodeString(pstrWindowName);
        ProbeForReadUnicodeStringBufferOrId(strClassName);
        ProbeForReadUnicodeStringBuffer(strWindowName);
    } except (StubExceptionHandler(TRUE)) {
        MSGERRORCLEANUP(0);
    }

    /*
     * Use of both buffers is protected by try/except clauses in the code.
     */

    retval = (HWND)_FindWindowEx(
            pwndParent,
            pwndChild,
            strClassName.Buffer,
            strWindowName.Buffer,
            dwType);
    retval = PtoH((PVOID)retval);

    CLEANUPRECV();

    TRACE("NtUserFindWindowEx");
    ENDATOMICRECV();
}

BOOL NtUserGetClassInfo(  // API GetClassInfoA/W
    IN HINSTANCE hInstance OPTIONAL,
    IN PUNICODE_STRING pstrClassName,
    IN OUT LPWNDCLASSEXW lpWndClass,
    OUT LPWSTR *ppszMenuName,
    IN BOOL bAnsi)
{
    UNICODE_STRING strClassName;

    LPWSTR pszMenuName;
    WNDCLASSEXW  wc;
    BEGINRECV(BOOL, FALSE);

    /*
     * Probe arguments
     */
    try {
        strClassName = ProbeAndReadUnicodeString(pstrClassName);

        /*
         * The class name may either be a string or an atom.  Only
         * probe strings.
         */
        ProbeForReadUnicodeStringBufferOrId(strClassName);
        ProbeForWrite(lpWndClass, sizeof(*lpWndClass), DATAALIGN);
        ProbeForWriteUlong((PULONG)ppszMenuName);
        RtlCopyMemory(&wc, lpWndClass, sizeof(WNDCLASSEXW));
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    /*
     * The class name buffer is client-side.
     */
    retval = _GetClassInfoEx(
                hInstance,
                (LPTSTR)strClassName.Buffer,
                &wc,
                &pszMenuName,
                bAnsi);

    try {
        RtlCopyMemory(lpWndClass, &wc, sizeof(WNDCLASSEXW));
        *ppszMenuName = pszMenuName;
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }
    TRACE("NtUserGetClassInfo");
    ENDRECV();
}

/*
 * gaFNIDtoICLS is used only in NtUserGetClassName and should be in sync with
 * the values from user.h
 * ICLS_MAX is an unused value
 */
CONST BYTE gaFNIDtoICLS[] = {
                        //  FNID-START
    ICLS_SCROLLBAR,     //  FNID_SCROLLBAR
    ICLS_ICONTITLE,     //  FNID_ICONTITLE
    ICLS_MENU,          //  FNID_MENU
    ICLS_DESKTOP,       //  FNID_DESKTOP
    ICLS_MAX,           //  FNID_DEFWINDOWPROC
    ICLS_BUTTON,        //  FNID_BUTTON
    ICLS_COMBOBOX,      //  FNID_COMBOBOX
    ICLS_COMBOLISTBOX,  //  FNID_COMBOLISTBOX
    ICLS_DIALOG,        //  FNID_DIALOG
    ICLS_EDIT,          //  FNID_EDIT
    ICLS_LISTBOX,       //  FNID_LISTBOX
    ICLS_MDICLIENT,     //  FNID_MDICLIENT
    ICLS_STATIC,        //  FNID_STATIC
    ICLS_IME,           //  FNID_IME
    ICLS_MAX,           //  FNID_HKINLPCWPEXSTRUCT
    ICLS_MAX,           //  FNID_HKINLPCWPRETEXSTRUCT
    ICLS_MAX,           //  FNID_DEFFRAMEPROC
    ICLS_MAX,           //  FNID_DEFMDICHILDPROC
    ICLS_MAX,           //  FNID_MB_DLGPROC
    ICLS_MAX,           //  FNID_MDIACTIVATEDLGPROC
    ICLS_MAX,           //  FNID_SENDMESSAGE
    ICLS_MAX,           //  FNID_SENDMESSAGEFF
    ICLS_MAX,           //  FNID_SENDMESSAGEEX
    ICLS_MAX,           //  FNID_CALLWINDOWPROC
    ICLS_MAX,           //  FNID_SENDMESSAGEBSM
    ICLS_SWITCH,        //  FNID_SWITCH
    ICLS_TOOLTIP        //  FNID_TOOLTIP
};                      //  FNID_END

int NtUserGetClassName(  // API GetClassNameA/W
    IN HWND hwnd,
    IN BOOL bReal,
    IN OUT PUNICODE_STRING pstrClassName)
{
    UNICODE_STRING strClassName;
    ATOM atom;

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    BEGINRECV_HWND_SHARED(DWORD, 0, hwnd);

    /*
     * Probe arguments
     */
    try {
        strClassName = ProbeAndReadUnicodeString(pstrClassName);
#if defined(_X86_)
        ProbeForWrite(strClassName.Buffer, strClassName.MaximumLength,
            sizeof(BYTE));
#else
        ProbeForWrite(strClassName.Buffer, strClassName.MaximumLength,
            sizeof(WCHAR));
#endif

        atom = pwnd->pcls->atomClassName;
        UserAssert(ARRAY_SIZE(gaFNIDtoICLS) == FNID_END - FNID_START + 1);

        if (bReal) {
            DWORD dwFnid;
            DWORD dwClass;
            dwFnid = GETFNID(pwnd);
            if (dwFnid) {
                dwFnid -= FNID_START;
                if ((dwFnid < ARRAY_SIZE(gaFNIDtoICLS)) && (dwFnid >= 0)) {
                    dwClass = gaFNIDtoICLS[dwFnid];
                    if (dwClass != ICLS_MAX) {
                        atom = gpsi->atomSysClass[dwClass];
                    }
                }
            }
        }
        retval = UserGetAtomName(
            atom,
            strClassName.Buffer,
            strClassName.MaximumLength / sizeof(WCHAR));

    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    TRACE("NtUserGetClassName");
    ENDRECV_HWND_SHARED();
}

int NtUserGetClipboardFormatName(  // API GetclipboardFormatNameA/W
    IN UINT format,
    OUT LPWSTR lpszFormatName,
    IN UINT chMax)
{
    BEGINRECV_NOCRIT(int, 0);

    /*
     * Probe arguments
     */
    try {
        ProbeForWriteBuffer(lpszFormatName, chMax, CHARALIGN);
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    if ((ATOM)format < MAXINTATOM) {
        MSGERROR(ERROR_INVALID_PARAMETER);
    } else {
        /*
         * UserGetAtomName (actually RtlQueryAtomInAtomTable) protects access
         * within a try block, and sets last error appropriately.
         */
        retval = UserGetAtomName((ATOM)format, lpszFormatName, chMax);
    }

    TRACE("NtUserGetClipboardFormatName");
    ENDRECV_NOCRIT();
}

int NtUserGetKeyNameText(
    IN LONG lParam,
    OUT LPWSTR lpszKeyName,
    IN UINT chMax)
{
    BEGINRECV_SHARED(int, 0);

    /*
     * Probe arguments
     */
    try {
        ProbeForWriteBuffer(lpszKeyName, chMax, CHARALIGN);
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    /*
     * Note -- lpszKeyName is a client-side address.  GetKeyNameText
     * protects uses with try blocks, and sets last error accordingly.
     */

    retval = _GetKeyNameText(
                lParam,
                lpszKeyName,
                chMax);

    TRACE("NtUserGetKeyNameText");
    ENDRECV_SHARED();
}

BOOL NtUserGetKeyboardLayoutName(
    IN OUT PUNICODE_STRING pstrKLID)
{
    PTHREADINFO ptiCurrent;
    PKL pklActive;
    UNICODE_STRING strKLID;

    BEGINRECV_SHARED(BOOL, FALSE);

    ptiCurrent = PtiCurrentShared();
    pklActive = ptiCurrent->spklActive;

    if (pklActive == NULL) {
        MSGERROR(0);
    }
    /*
     * Probe arguments
     */
    try {
        strKLID = ProbeAndReadUnicodeString(pstrKLID);
        ProbeForWrite(strKLID.Buffer, strKLID.MaximumLength, CHARALIGN);

        wcsncpycch(strKLID.Buffer,
                pklActive->spkf->awchKF,
                strKLID.MaximumLength / sizeof(WCHAR));

        retval = TRUE;

    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    TRACE("NtUserGetKeyboardLayoutName");
    ENDRECV_SHARED();
}

UINT NtUserGetKeyboardLayoutList(
    IN UINT nItems,
    OUT HKL *lpBuff)
{
    PWINDOWSTATION pwinsta;

    BEGINRECV_SHARED(UINT, 0);

    /*
     * Probe arguments
     */
    try {
        if (!lpBuff) {
            nItems = 0;
        }
        ProbeForWriteBuffer(lpBuff, nItems, DATAALIGN);
        pwinsta = _GetProcessWindowStation(NULL);

    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    /*
     * Access to the client-side buffer lpBuff is protected by try/except
     * inside _GetKeyboardLayoutList()
     */
    retval = (DWORD)_GetKeyboardLayoutList(pwinsta, nItems, lpBuff);
    TRACE("NtUserGetKeyboardLayoutList");
    ENDRECV_SHARED();
}

UINT NtUserMapVirtualKeyEx(
    IN UINT uCode,
    IN UINT uMapType,
    IN ULONG_PTR dwHKLorPKL,
    IN BOOL bHKL)
{
    PKL pkl;

    BEGINRECV_SHARED(UINT, 0);

    /*
     * See if we need to convert an HKL to a PKL.  MapVirtualKey passes a PKL and
     * MapVirtualKeyEx passes an HKL.  The conversion must be done in the kernel.
     */
    if (bHKL) {
        pkl = HKLtoPKL(PtiCurrentShared(), (HKL)dwHKLorPKL);
    } else {
        pkl = PtiCurrentShared()->spklActive;
    }

    if (pkl == NULL) {
        retval = 0;
    } else {
        retval = InternalMapVirtualKeyEx(uCode, uMapType, pkl->spkf->pKbdTbl);
    }

    TRACE("NtUserMapVirtualKeyEx");
    ENDRECV_SHARED();
}

ATOM NtUserRegisterClassExWOW(
    IN WNDCLASSEX *lpWndClass,
    IN PUNICODE_STRING pstrClassName,
    IN PCLSMENUNAME pcmn,
    IN WORD fnid,
    IN DWORD dwFlags,
    IN LPDWORD pdwWOWstuff OPTIONAL)
{
    UNICODE_STRING strClassName;
    UNICODE_STRING strMenuName;
    WNDCLASSEX WndClass;
    WC WowCls;
    CLSMENUNAME cmn;

    BEGINRECV(ATOM, 0);

    TESTFLAGS(dwFlags, CSF_VALID);

    /*
     * Probe arguments
     */
    try {
        strClassName = ProbeAndReadUnicodeString(pstrClassName);
        cmn = ProbeAndReadStructure(pcmn, CLSMENUNAME);
        strMenuName = ProbeAndReadUnicodeString(cmn.pusMenuName);
        WndClass = ProbeAndReadStructure(lpWndClass, WNDCLASSEX);
        ProbeForReadUnicodeStringBufferOrId(strClassName);
        ProbeForReadUnicodeStringBufferOrId(strMenuName);
        if (ARGUMENT_PRESENT(pdwWOWstuff)) {
            ProbeForRead(pdwWOWstuff, sizeof(WC), sizeof(BYTE));
            RtlCopyMemory(&WowCls, pdwWOWstuff, sizeof(WC));
            pdwWOWstuff = (PDWORD)&WowCls;
        }
        WndClass.lpszClassName = strClassName.Buffer;
        WndClass.lpszMenuName = strMenuName.Buffer;
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    /*
     * ClassName and MenuName in WndClass are client-side pointers.
     */

    retval = xxxRegisterClassEx(
            &WndClass,
            &cmn,
            fnid,
            dwFlags,
            pdwWOWstuff);

    TRACE("NtUserRegisterClassExWOW");
    ENDRECV();
}

UINT NtUserRegisterWindowMessage(
    IN PUNICODE_STRING pstrMessage)
{
    UNICODE_STRING strMessage;

    BEGINRECV_NOCRIT(UINT, 0);

    /*
     * Probe arguments
     */
    try {
        strMessage = ProbeAndReadUnicodeString(pstrMessage);
        ProbeForReadUnicodeStringBuffer(strMessage);
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    /*
     * The buffer is in client-side memory.
     * Rtl atom routines protect accesses to strings with their
     * own try/except blocks, UserAddAtom sets last error accordingly.
     */
    retval = UserAddAtom(
            strMessage.Buffer, FALSE);

    TRACE("NtUserRegisterWindowMessage");
    ENDRECV_NOCRIT();
}

HANDLE NtUserRemoveProp(
    IN HWND hwnd,
    IN DWORD dwProp)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    BEGINRECV_HWND(HANDLE, NULL, hwnd);

    retval = InternalRemoveProp(pwnd, (LPWSTR)LOWORD(dwProp), FALSE);

    TRACE("NtUserRemoveProp");
    ENDRECV_HWND();
}

BOOL NtUserSetProp(
    IN HWND hwnd,
    IN DWORD dwProp,
    IN HANDLE hData)
{

    //
    // N.B. This function has implicit window handle translation. This
    //      operation is performed in the User server API dispatcher.
    //

    BEGINRECV_HWND(DWORD, 0, hwnd);

    retval = InternalSetProp(
            pwnd,
            (LPTSTR)LOWORD(dwProp),
            hData,
            HIWORD(dwProp) ? PROPF_STRING : 0);

    TRACE("NtUserSetProp");
    ENDRECV_HWND();
}

BOOL NtUserUnregisterClass(  // API UnregisterClass
    IN PUNICODE_STRING pstrClassName,
    IN HINSTANCE hInstance,
    OUT PCLSMENUNAME pcmn)
{
    UNICODE_STRING strClassName;
    CLSMENUNAME cmn;

    BEGINRECV(BOOL, FALSE);

    /*
     * Probe arguments
     */
    try {
        strClassName = ProbeAndReadUnicodeString(pstrClassName);
        ProbeForReadUnicodeStringBufferOrId(strClassName);
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    /*
     * The buffer is in client-side memory.
     */

    retval = _UnregisterClass(
                strClassName.Buffer,
                hInstance,
                &cmn);

    try {
        ProbeAndWriteStructure(pcmn, cmn, CLSMENUNAME);
    } except (StubExceptionHandler(FALSE)) {
        // no SetLastError, since pcmn is a USER address, not the application's
    }

    TRACE("NtUserUnregisterClass");
    ENDRECV();
}

SHORT NtUserVkKeyScanEx(
    IN WCHAR cChar,
    IN ULONG_PTR dwHKLorPKL,
    IN BOOL bHKL)
{
    PKL pkl;

    BEGINRECV_SHARED(SHORT, -1);

    /*
     * See if we need to convert an HKL to a PKL.  VkKeyScan passes a PKL and
     * VkKeyScanEx passes an HKL.  The conversion must be done on the server side.
     */
    if (bHKL) {
        pkl = HKLtoPKL(PtiCurrentShared(), (HKL)dwHKLorPKL);
    } else {
        pkl = PtiCurrentShared()->spklActive;
    }

    if (pkl == NULL) {
        retval = (SHORT)-1;
    } else {
        retval = InternalVkKeyScanEx(cChar, pkl->spkf->pKbdTbl);
    }

    TRACE("NtUserVkKeyScanEx");
    ENDRECV_SHARED();
}

NTSTATUS
NtUserEnumDisplayDevices(
    IN PUNICODE_STRING pstrDeviceName,
    IN DWORD iDevNum,
    IN OUT LPDISPLAY_DEVICEW lpDisplayDevice,
    IN DWORD dwFlags)
{
    BEGINRECV(NTSTATUS, STATUS_UNSUCCESSFUL);

    //
    // Update the list of devices.
    // If the function returns FALSE (retry update), then we must
    // disable the current hdev, call back, and reanable the hdev.
    //

    if (DrvUpdateGraphicsDeviceList(FALSE, FALSE) == FALSE) {

        if (DrvDisableMDEV(gpDispInfo->pmdev, TRUE)) {

            DrvUpdateGraphicsDeviceList(TRUE, FALSE);

            DrvEnableMDEV(gpDispInfo->pmdev, TRUE);

            //
            // Repaint the screen
            //

            xxxUserResetDisplayDevice();
        }
    }

    /*
     * Address checking, etc., occurs in GRE.
     */

    retval = DrvEnumDisplayDevices(pstrDeviceName,
                                   gpDispInfo->pMonitorPrimary->hDev,
                                   iDevNum,
                                   lpDisplayDevice,
                                   dwFlags,
                                   UserMode);
    TRACE("NtUserEnumDisplayDevices");
    ENDRECV();
}

NTSTATUS
NtUserEnumDisplaySettings(
    IN PUNICODE_STRING pstrDeviceName,
    IN DWORD           iModeNum,
    OUT LPDEVMODEW     lpDevMode,
    IN  DWORD          dwFlags)
{
    BEGINRECV(NTSTATUS, STATUS_UNSUCCESSFUL);

    /*
     * Address checking, etc., occurs in GRE.
     */

    retval = DrvEnumDisplaySettings(pstrDeviceName,
                                    gpDispInfo->pMonitorPrimary->hDev,
                                    iModeNum,
                                    lpDevMode,
                                    dwFlags);

    TRACE("NtUserEnumDisplaySettings");
    ENDRECV();
}

LONG
NtUserChangeDisplaySettings(
    IN PUNICODE_STRING pstrDeviceName,
    IN LPDEVMODEW pDevMode,
    IN HWND hwnd,
    IN DWORD dwFlags,
    IN PVOID lParam)
{
    BEGINRECV(LONG, DISP_CHANGE_FAILED);

    /*
     * Prevent restricted threads from changing
     * display settings
     */
    if (IS_CURRENT_THREAD_RESTRICTED(JOB_OBJECT_UILIMIT_DISPLAYSETTINGS)) {
        MSGERROR(0);
    }

    /*
     * Address checking, etc., occurs in GRE.
     */

    retval = xxxUserChangeDisplaySettings(pstrDeviceName,
                                          pDevMode,
                                          hwnd,
                                          NULL,     // pdesk
                                          dwFlags,
                                          lParam,
                                          UserMode);

    TRACE("NtUserChangeDisplaySettings");
    ENDRECV();
}

BOOL NtUserCallMsgFilter(  // API CallMsgFilterA/W
    IN OUT LPMSG lpMsg,
    IN int nCode)
{
    MSG msg;

    BEGINRECV(BOOL, FALSE);

    /*
     * Probe arguments
     */
    try {
        ProbeForWriteMessage(lpMsg);
        msg = *lpMsg;
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    retval = _CallMsgFilter(
                &msg,
                nCode);
    try {
        *lpMsg = msg;
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    TRACE("NtUserCallMsgFilter");
    ENDRECV();
}

int NtUserDrawMenuBarTemp(  // private DrawMenuBarTemp
    IN HWND hwnd,
    IN HDC hdc,
    IN LPCRECT lprc,
    IN HMENU hMenu,
    IN HFONT hFont)
{
    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    PMENU   pMenu;
    TL      tlpMenu;
    RECT    rc;


    BEGINRECV_HWNDLOCK(int, 0, hwnd);

    /*
     * Probe and capture arguments.
     */
    try {
        rc = ProbeAndReadRect(lprc);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    ValidateHMENU(pMenu, hMenu);

    ThreadLockAlwaysWithPti(ptiCurrent, pMenu, &tlpMenu);

    retval = xxxDrawMenuBarTemp(
            pwnd,
            hdc,
            &rc,
            pMenu,
            hFont);

    ThreadUnlock(&tlpMenu);

    TRACE("NtUserDrawMenuBarTemp");
    ENDRECV_HWNDLOCK();
}

BOOL NtUserDrawCaptionTemp(  // private DrawCaptionTempA/W
    IN HWND hwnd,
    IN HDC hdc,
    IN LPCRECT lprc,
    IN HFONT hFont,
    IN HICON hIcon,
    IN PUNICODE_STRING pstrText,
    IN UINT flags)
{
    PCURSOR         pcur;
    TL              tlpcur;
    RECT            rc;
    UNICODE_STRING  strCapture;
    PWND            pwnd;
    TL              tlpwnd;
    PTHREADINFO     ptiCurrent;
    TL tlBuffer;
    BOOL fFreeBuffer = FALSE;

    BEGINRECV(DWORD, FALSE);

    ptiCurrent = PtiCurrent();

    ValidateHWNDOPT(pwnd, hwnd);
    ValidateHCURSOROPT(pcur, hIcon);

    /*
     * Probe and capture arguments.  Capturing the text is ugly,
     * but must be done because it is passed to GDI.
     */
    try {
        rc = ProbeAndReadRect(lprc);
        strCapture = ProbeAndReadUnicodeString(pstrText);
        if (strCapture.Buffer != NULL) {
            PWSTR pszCapture = strCapture.Buffer;
            ProbeForRead(strCapture.Buffer, strCapture.Length, CHARALIGN);
            strCapture.Buffer = UserAllocPoolWithQuota(strCapture.Length+sizeof(UNICODE_NULL), TAG_TEXT);
            if (strCapture.Buffer != NULL) {
                fFreeBuffer = TRUE;
                ThreadLockPool(ptiCurrent, strCapture.Buffer, &tlBuffer);
                RtlCopyMemory(strCapture.Buffer, pszCapture, strCapture.Length);
                strCapture.Buffer[strCapture.Length/sizeof(WCHAR)]=0;  // null-terminate string
                strCapture.MaximumLength = strCapture.Length+sizeof(UNICODE_NULL);
                pstrText = &strCapture;
            } else {
                ExRaiseStatus(STATUS_NO_MEMORY);
            }
        }
    } except (StubExceptionHandler(FALSE)) {
        MSGERRORCLEANUP(0);
    }

    ThreadLockWithPti(ptiCurrent, pwnd, &tlpwnd);
    ThreadLockWithPti(ptiCurrent, pcur, &tlpcur);

    retval = xxxDrawCaptionTemp(
            pwnd,
            hdc,
            &rc,
            hFont,
            pcur,
            strCapture.Buffer ? &strCapture : NULL,
            flags);

    ThreadUnlock(&tlpcur);
    ThreadUnlock(&tlpwnd);

    CLEANUPRECV();
    if (fFreeBuffer)
        ThreadUnlockAndFreePool(ptiCurrent, &tlBuffer);

    TRACE("NtUserDrawCaptionTemp");
    ENDRECV();
}

BOOL NtUserGetKeyboardState(  // API GetKeyboardState
    OUT PBYTE pb)
{
    int i;
    PQ pq;
    BEGINRECV_SHARED(SHORT, 0)

    /*
     * Probe arguments
     */
    try {
        ProbeForWrite(pb, 256, sizeof(BYTE));

        pq = PtiCurrentShared()->pq;

        for (i = 0; i < 256; i++, pb++) {
            *pb = 0;
            if (TestKeyStateDown(pq, i))
                *pb |= 0x80;

            if (TestKeyStateToggle(pq, i))
                *pb |= 0x01;
        }
        retval = TRUE;
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    ENDRECV_SHARED();
}

SHORT NtUserGetKeyState(
    IN int vk)
{
    PTHREADINFO ptiCurrent;
    BEGINRECV_SHARED(SHORT, 0)

    ptiCurrent = PtiCurrentShared();
    if (ptiCurrent->pq->QF_flags & QF_UPDATEKEYSTATE) {

        /*
         * We are going to change the system state, so we
         * must have an exclusive lock
         */
        ChangeAcquireResourceType();

        /*
         * If this thread needs a key state event, give one to it. There are
         * cases where any app may be looping looking at GetKeyState(), plus
         * calling PeekMessage(). Key state events don't get created unless
         * new hardware input comes along. If the app isn't receiving hardware
         * input, it won't get the new key state. So ResyncKeyState() will
         * ensure that if the app is looping on GetKeyState(), it'll get the
         * right key state.
         */
        if (ptiCurrent->pq->QF_flags & QF_UPDATEKEYSTATE) {
            PostUpdateKeyStateEvent(ptiCurrent->pq);
        }
    }
    retval = _GetKeyState(vk);

    /*
     * Update the client side key state cache.
     */
    ptiCurrent->pClientInfo->dwKeyCache = gpsi->dwKeyCache;
    RtlCopyMemory(ptiCurrent->pClientInfo->afKeyState,
                  ptiCurrent->pq->afKeyState,
                  CBKEYCACHE);

    ENDRECV_SHARED();
}

/**************************************************************************\
* NtUserQueryWindow
*
* 03-18-95 JimA         Created.
\**************************************************************************/

HANDLE NtUserQueryWindow(
    IN HWND hwnd,
    IN WINDOWINFOCLASS WindowInfo)
{
    PTHREADINFO ptiWnd;

    BEGINRECV_HWND_SHARED(HANDLE, NULL, hwnd);

    ptiWnd = GETPTI(pwnd);

    switch (WindowInfo) {
    case WindowProcess:

        /*
         * Special case console windows
         */
        if (ptiWnd->TIF_flags & TIF_CSRSSTHREAD &&
                pwnd->pcls->atomClassName == gatomConsoleClass) {
            retval = (HANDLE)LongToHandle( _GetWindowLong(pwnd, 0) );
        } else {
            retval = (HANDLE)ptiWnd->pEThread->Cid.UniqueProcess;
        }
        break;
    case WindowThread:

        /*
         * Special case console windows
         */
        if (ptiWnd->TIF_flags & TIF_CSRSSTHREAD &&
                pwnd->pcls->atomClassName == gatomConsoleClass) {
            retval = (HANDLE)LongToHandle( _GetWindowLong(pwnd, 4) );
        } else {
            retval = (HANDLE)ptiWnd->pEThread->Cid.UniqueThread;
        }
        break;
    case WindowActiveWindow:
        retval = (HANDLE)HW(ptiWnd->pq->spwndActive);
        break;
    case WindowFocusWindow:
        retval = (HANDLE)HW(ptiWnd->pq->spwndFocus);
        break;
    case WindowIsHung:
        retval = (HANDLE)IntToPtr( FHungApp(ptiWnd, CMSHUNGAPPTIMEOUT) );
        break;
    case WindowIsForegroundThread:
        retval = (HANDLE)IntToPtr( (ptiWnd->pq == gpqForeground) );
        break;
    case WindowDefaultImeWindow:
        retval = (HANDLE)HW(ptiWnd->spwndDefaultIme);
        break;
    case WindowDefaultInputContext:
        retval = (HANDLE)PtoH(ptiWnd->spDefaultImc);
        break;
    case WindowActiveDefaultImeWindow:
        /*
         * Only return a window if there is a foreground queue and the
         * caller has access to the current desktop.
         */
        retval = NULL;
        if (gpqForeground && gpqForeground->spwndActive &&
                PtiCurrentShared()->rpdesk == gpqForeground->spwndActive->head.rpdesk) {
            PWND pwndFG = gpqForeground->spwndActive;

            if (pwndFG && pwndFG->head.pti) {
                retval = (HANDLE)PtoHq(pwndFG->head.pti->spwndDefaultIme);
            }
        }
        break;
    default:
        retval = (HANDLE)NULL;
        break;
    }

    ENDRECV_HWND_SHARED();
}

BOOL NtUserSBGetParms(  // API GetScrollInfo, SBM_GETSCROLLINFO
    IN HWND hwnd,
    IN int code,
    IN PSBDATA pw,
    IN OUT LPSCROLLINFO lpsi)
{
    SBDATA sbd;
    SCROLLINFO si;
    BEGINRECV_HWND_SHARED(BOOL, FALSE, hwnd);

    /*
     * Probe arguments
     */
    try {
        ProbeForWriteScrollInfo(lpsi);

        /*
         * Probe the 4 DWORDS (MIN, MAX, PAGE, POS)
         */
        ProbeForRead(pw, sizeof(SBDATA), sizeof(DWORD));
        RtlCopyMemory(&sbd, pw, sizeof(sbd));
        RtlCopyMemory(&si, lpsi, sizeof(SCROLLINFO));
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    retval = _SBGetParms(pwnd, code, &sbd, &si);
    try {
        RtlCopyMemory(lpsi, &si, sizeof(SCROLLINFO));
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    ENDRECV_HWND_SHARED();
}

BOOL NtUserBitBltSysBmp(
    IN HDC hdc,
    IN int xDest,
    IN int yDest,
    IN int cxDest,
    IN int cyDest,
    IN int xSrc,
    IN int ySrc,
    IN DWORD dwRop)
{
    BEGINRECV(BOOL, FALSE);

    /*
     * Note -- this interface requires exclusive ownership of
     *         the User crit sect in order to serialize use
     *         of HDCBITS.  Only one thread at a time may use
     *         a DC.
     */

    retval = GreBitBlt(hdc,
                       xDest,
                       yDest,
                       cxDest,
                       cyDest,
                       HDCBITS(),
                       xSrc,
                       ySrc,
                       dwRop,
                       0);

    ENDRECV();
}

HPALETTE NtUserSelectPalette(
    IN HDC hdc,
    IN HPALETTE hpalette,
    IN BOOL fForceBackground)
{
    BEGINRECV(HPALETTE, NULL)

    retval = _SelectPalette(hdc, hpalette, fForceBackground);

    ENDRECV();
}

/*
 * Message thunks
 */

LRESULT NtUserMessageCall(
    IN HWND hwnd,
    IN UINT msg,
    IN WPARAM wParam,
    IN LPARAM lParam,
    IN ULONG_PTR xParam,
    IN DWORD xpfnProc,
    IN BOOL bAnsi)
{
    BEGINRECV_HWNDLOCK(LRESULT, 0, hwnd);

    if ((msg & ~MSGFLAG_MASK) >= WM_USER) {
        retval = CALLPROC(xpfnProc)(
                pwnd,
                msg,
                wParam,
                lParam,
                xParam);
    } else {
        retval = gapfnMessageCall[MessageTable[(msg & ~MSGFLAG_MASK)].iFunction](
                pwnd,
                msg,
                wParam,
                lParam,
                xParam,
                xpfnProc,
                bAnsi);
    }

    TRACE("NtUserMessageCall");
    ENDRECV_HWNDLOCK();
}

MESSAGECALL(DWORD)
{
    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnDWORD");

    UNREFERENCED_PARAMETER(bAnsi);

    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            lParam,
            xParam);

    TRACE("fnDWORD");
    ENDRECV_MESSAGECALL();

}

MESSAGECALL(OPTOUTLPDWORDOPTOUTLPDWORD)
{
    DWORD dwwParam, dwlParam;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnOPTOUTLPDWORDOPTOUTLPDWORD");

    UNREFERENCED_PARAMETER(bAnsi);

    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            (WPARAM)&dwwParam,
            (LPARAM)&dwlParam,
            xParam);

    /*
     * Probe arguments
     */
    try {
        if (ARGUMENT_PRESENT(wParam)) {
            ProbeAndWriteUlong((PULONG)wParam, dwwParam);
        }
        if (ARGUMENT_PRESENT(lParam)) {
            ProbeAndWriteUlong((PULONG)lParam, dwlParam);
        }
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);  // should messages with bad wParam/lParam SetLastError?
    }

    TRACE("fnOPTOUTLPDWORDOPTOUTLPDWORD");
    ENDRECV_MESSAGECALL();

}

MESSAGECALL(INOUTNEXTMENU)
{
    MDINEXTMENU mnm;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnINOUTNEXTMENU");

    UNREFERENCED_PARAMETER(bAnsi);

    /*
     * Probe arguments
     */
    try {
        ProbeForWriteMDINextMenu((PMDINEXTMENU)lParam);
        mnm = *(PMDINEXTMENU)lParam;

    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }
    retval = CALLPROC(xpfnProc)(
                pwnd,
                msg,
                wParam,
                (LPARAM)&mnm,
                xParam);

    try {
        *(PMDINEXTMENU)lParam = mnm;
    } except (StubExceptionHandler(FALSE)) {
    }

    TRACE("fnINOUTNEXTMENU");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(DWORDOPTINLPMSG)
{
    MSG msgstruct;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnDWORDOPTINLPMSG");

    UNREFERENCED_PARAMETER(bAnsi);

    /*
     * Probe arguments
     */
    try {
        if (ARGUMENT_PRESENT(lParam)) {
            msgstruct = ProbeAndReadMessage((LPMSG)lParam);
            lParam = (LPARAM)&msgstruct;
        }
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            lParam,
            xParam);

    TRACE("fnDWORDOPTINLPMSG");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(COPYGLOBALDATA)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnCOPYGLOBALDATA");

    UNREFERENCED_PARAMETER(bAnsi);

    /*
     * Probe arguments
     */
    try {
        ProbeForRead((PVOID)lParam, wParam, sizeof(BYTE));
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    /*
     * !!! Data pointed to by lParam must be captured
     * in xxxInterSendMsgEx
     */
    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            lParam,
            xParam);

    TRACE("fnCOPYGLOBALDATA");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(COPYDATA)
{
    COPYDATASTRUCT cds;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnCOPYDATA");

    UNREFERENCED_PARAMETER(bAnsi);

    /*
     * Probe arguments
     */
    try {
        if (ARGUMENT_PRESENT(lParam)) {
            cds = ProbeAndReadCopyDataStruct((PCOPYDATASTRUCT)lParam);
            if (cds.lpData)
                ProbeForRead(cds.lpData, cds.cbData, sizeof(BYTE));
            lParam = (LPARAM)&cds;
        }
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    /*
     * !!! Data pointed to by cds.lpData must be captured
     * in xxxInterSendMsgEx
     */
    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            lParam,
            xParam);

    TRACE("fnCOPYDATA");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(SENTDDEMSG)
{
    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnSENTDDEMSG");

    UNREFERENCED_PARAMETER(bAnsi);

    if (xpfnProc == FNID_CALLWINDOWPROC) {
        retval = CALLPROC(xpfnProc)(pwnd,
                msg | MSGFLAG_DDE_SPECIAL_SEND,
                wParam, lParam, xParam);
    } else if ((ptiCurrent->TIF_flags & TIF_16BIT) &&
               (ptiCurrent->ptdb) &&
               (ptiCurrent->ptdb->hTaskWow)) {
        /*
         * Note that this function may modify msg by ORing in a bit in the
         * high word.  This bit is ignored when thunking messages.
         * This allows the DdeTrackSendMessage() hook to be skipped - which
         * would cause an error - and instead allows this thunk to carry
         * the message all the way across.
         */
        retval = xxxDDETrackPostHook(&msg, pwnd, wParam, &lParam, TRUE);
        switch (retval) {
        case DO_POST:
            /*
             * Or in the MSGFLAG_DDE_SPECIAL_SEND so that
             * xxxSendMessageTimeout() will not pass this on to
             * xxxDdeTrackSendMsg() which would think it was evil.
             *
             * Since the SendMessage() thunks ignore the reserved bits
             * it will still get maped to the fnSENTDDEMSG callback thunk.
             */
            retval = CALLPROC(xpfnProc)(pwnd,
                    msg | MSGFLAG_DDE_SPECIAL_SEND,
                    wParam, lParam, xParam);
            break;

        case FAKE_POST:
        case FAIL_POST:
            retval = 0;
        }
    } else {
        MSGERROR(0);
    }

    TRACE("fnSENTDDEMSG");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(DDEINIT)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    PWND pwndFrom;
    TL tlpwndFrom;
    PDDEIMP pddei;
    PSECURITY_QUALITY_OF_SERVICE pqos;
    NTSTATUS Status;

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnDDEINIT");

    UNREFERENCED_PARAMETER(bAnsi);

    ValidateHWND(pwndFrom, (HWND)wParam);
    ThreadLockAlwaysWithPti(ptiCurrent, pwndFrom, &tlpwndFrom);

    /*
     * Create temporary DDEIMP property for client window - this stays around
     * only during the initiate phase.
     */
    if ((pddei = (PDDEIMP)_GetProp(pwndFrom, PROP_DDEIMP, TRUE))
            == NULL) {
        pddei = (PDDEIMP)UserAllocPoolWithQuota(sizeof(DDEIMP), TAG_DDEd);
        if (pddei == NULL) {
            RIPERR0(ERROR_NOT_ENOUGH_MEMORY, RIP_WARNING, "fnDDEINIT: LocalAlloc failed.");
            MSGERRORCLEANUP(0);
        }
        pqos = (PSECURITY_QUALITY_OF_SERVICE)_GetProp(pwndFrom, PROP_QOS, TRUE);
        if (pqos == NULL) {
            pqos = &gqosDefault;
        }
        pddei->qos = *pqos;
        Status = SeCreateClientSecurity(PsGetCurrentThread(),
                pqos, FALSE, &pddei->ClientContext);
        if (!NT_SUCCESS(Status)) {
            RIPMSG0(RIP_WARNING, "SeCreateClientContext failed.");
            UserFreePool(pddei);
            MSGERRORCLEANUP(0);
        }
        pddei->cRefInit = 1;
        pddei->cRefConv = 0;
        InternalSetProp(pwndFrom, PROP_DDEIMP, pddei, PROPF_INTERNAL);
    } else {
        pddei->cRefInit++;      // cover broadcast case!
    }

    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            lParam,
            xParam);

    /*
     * Reaquire pddei incase pwndFrom was destroyed.
     */
    pddei = (PDDEIMP)_GetProp(pwndFrom, PROP_DDEIMP, TRUE);
    if (pddei != NULL) {
        /*
         * Decrement reference count from DDEImpersonate property and remove property.
         */
        pddei->cRefInit--;
        if (pddei->cRefInit == 0) {
            InternalRemoveProp(pwndFrom, PROP_DDEIMP, TRUE);
            if (pddei->cRefConv == 0) {
                SeDeleteClientSecurity(&pddei->ClientContext);
                UserFreePool(pddei);
            }
        }
    }

    CLEANUPRECV();
    ThreadUnlock(&tlpwndFrom);

    TRACE("fnDDEINIT");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(INPAINTCLIPBRD)
{
    PAINTSTRUCT ps;

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnINPAINTCLIPBRD");

    UNREFERENCED_PARAMETER(bAnsi);

    /*
     * Probe arguments
     */
    try {
        ps = ProbeAndReadPaintStruct((PPAINTSTRUCT)lParam);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            (LPARAM)&ps,
            xParam);

    TRACE("fnINPAINTCLIPBRD");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(INSIZECLIPBRD)
{
    RECT rc;

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnINSIZECLIPBRD");

    UNREFERENCED_PARAMETER(bAnsi);

    /*
     * Probe arguments
     */
    try {
        rc = ProbeAndReadRect((PRECT)lParam);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            (LPARAM)&rc,
            xParam);

    TRACE("fnINSIZECLIPBRD");
    ENDRECV_MESSAGECALL();
}

#if 0

// !!!LATER not needed until we support multiple screens

MESSAGECALL(FULLSCREEN)
{
    TL tlpwnd;

    BEGINRECV(LRESULT, FALSE);
    TRACETHUNK("fnFULLSCREEN");

    /*
     * Probe arguments
     */
    try {
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    ValidateHWND(pwnd, hwnd);

    ThreadLockAlways(pwnd, &tlpwnd);
    retval = CALLPROC(xpfnProc)(
            pwnd,
            pmsg->msg,
            pmsg->wParam,
            (LONG)pdeviceinfo,
            pmsg->xParam);
    ThreadUnlock(&tlpwnd);

    TRACE("fnFULLSCREEN");
    ENDRECV();
}

#endif // 0

MESSAGECALL(INOUTDRAG)
{
    DROPSTRUCT ds;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnINOUTDRAG");

    UNREFERENCED_PARAMETER(bAnsi);

    /*
     * Probe arguments
     */
    try {
        ProbeForWriteDropStruct((PDROPSTRUCT)lParam);
        ds = *(PDROPSTRUCT)lParam;

    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }
    retval = CALLPROC(xpfnProc)(
                pwnd,
                msg,
                wParam,
                (LPARAM)&ds,
                xParam);

    try {
        *(PDROPSTRUCT)lParam = ds;
    } except (StubExceptionHandler(FALSE)) {
    }

    TRACE("fnINOUTDRAG");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(GETDBCSTEXTLENGTHS)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnGETDBCSTEXTLENGTHS");

    UNREFERENCED_PARAMETER(lParam);

    /*
     * This is used by L/CB_GETTEXTLEN which should return -1 (L/CB_ERR)
     *  on error. If any error code path is introduced here, make sure we return the
     *  proper value.This is also used by WM_GETTEXTLEN.
     */

    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            bAnsi,
            xParam);

    TRACE("fnGETDBCSTEXTLENGTHS");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(INLPCREATESTRUCT)
{
    CREATESTRUCTEX csex;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnINLPCREATESTRUCT");

    if (ARGUMENT_PRESENT(lParam)) {
        try {
            csex.cs = ProbeAndReadCreateStruct((LPCREATESTRUCTW)lParam);
            if (bAnsi) {
                ProbeForRead(csex.cs.lpszName, sizeof(CHAR), sizeof(CHAR));
                RtlInitLargeAnsiString((PLARGE_ANSI_STRING)&csex.strName,
                        (LPSTR)csex.cs.lpszName, (UINT)-1);
                if (IS_PTR(csex.cs.lpszClass)) {
                    ProbeForRead(csex.cs.lpszClass, sizeof(CHAR), sizeof(CHAR));
                    RtlInitLargeAnsiString((PLARGE_ANSI_STRING)&csex.strClass,
                            (LPSTR)csex.cs.lpszClass, (UINT)-1);
                }
            } else {
                ProbeForRead(csex.cs.lpszName, sizeof(WCHAR), CHARALIGN);
                RtlInitLargeUnicodeString((PLARGE_UNICODE_STRING)&csex.strName,
                        csex.cs.lpszName, (UINT)-1);
                if (IS_PTR(csex.cs.lpszClass)) {
                    ProbeForRead(csex.cs.lpszClass, sizeof(WCHAR), CHARALIGN);
                    RtlInitLargeUnicodeString((PLARGE_UNICODE_STRING)&csex.strClass,
                            csex.cs.lpszClass, (UINT)-1);
                }
            }
        } except (StubExceptionHandler(FALSE)) {
            MSGERROR(0);
        }
    }

    /*
     * Per Win95, do not allow NULL lpcreatestructs for WM_NCCREATE [51986]
     * Allowed for WM_CREATE in Win95 for ObjectVision
     */
    else if (msg == WM_NCCREATE) {
        MSGERROR(0) ;
    }

    /*
     * !!! Strings pointed to by cs.cs must be captured in xxxInterSendMsgEx
     */
    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            lParam ? (LPARAM)&csex : 0,
            xParam);

    TRACE("fnINLPCREATESTRUCT");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(INLPMDICREATESTRUCT)
{
    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    MDICREATESTRUCTEX mdics;

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnINLPMDICREATESTRUCT");

    /*
     * Probe arguments
     */
    try {
        mdics.mdics = ProbeAndReadMDICreateStruct((LPMDICREATESTRUCTW)lParam);

        if (bAnsi) {
            ProbeForRead(mdics.mdics.szTitle, sizeof(CHAR), sizeof(CHAR));
            RtlInitLargeAnsiString((PLARGE_ANSI_STRING)&mdics.strTitle,
                    (LPSTR)mdics.mdics.szTitle, (UINT)-1);
            if (IS_PTR(mdics.mdics.szClass)) {
                ProbeForRead(mdics.mdics.szClass, sizeof(CHAR), sizeof(CHAR));
                RtlInitLargeAnsiString((PLARGE_ANSI_STRING)&mdics.strClass,
                        (LPSTR)mdics.mdics.szClass, (UINT)-1);
            } else {
                /*
                 * mdics.mdics.szClass may be Atom.
                 */
                RtlInitLargeAnsiString((PLARGE_ANSI_STRING)&mdics.strClass,
                                       NULL, 0);
            }
        } else {
            ProbeForRead(mdics.mdics.szTitle, sizeof(WCHAR), CHARALIGN);
            RtlInitLargeUnicodeString((PLARGE_UNICODE_STRING)&mdics.strTitle,
                    mdics.mdics.szTitle, (UINT)-1);
            if (IS_PTR(mdics.mdics.szClass)) {
                ProbeForRead(mdics.mdics.szClass, sizeof(WCHAR), CHARALIGN);
                RtlInitLargeUnicodeString((PLARGE_UNICODE_STRING)&mdics.strClass,
                        mdics.mdics.szClass, (UINT)-1);
            } else {
                /*
                 * mdics.mdics.szClass may be Atom.
                 */
                RtlInitLargeUnicodeString((PLARGE_UNICODE_STRING)&mdics.strClass,
                                       NULL, 0);
            }
        }
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    /*
     * !!! Strings pointed to by mdics must be captured in xxxInterSendMsgEx
     */
    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            (LPARAM)&mdics,
            xParam);

    TRACE("fnINLPMDICREATESTRUCT");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(INOUTLPSCROLLINFO)
{
    SCROLLINFO scrollinfo;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnINOUTLPSCROLLINFO");

    UNREFERENCED_PARAMETER(bAnsi);

    /*
     * Probe arguments
     */
    try {
        ProbeForWriteScrollInfo((LPSCROLLINFO)lParam);
        scrollinfo = *(LPSCROLLINFO)lParam;
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = CALLPROC(xpfnProc)(
                pwnd,
                msg,
                wParam,
                (LPARAM)&scrollinfo,
                xParam);

    try {
        *(LPSCROLLINFO)lParam = scrollinfo;
    } except (StubExceptionHandler(FALSE)) {
    }

    TRACE("fnINOUTLPSCROLLINFO");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(INOUTLPPOINT5)
{
    POINT5 pt5;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnINOUTLPPOINT5");

    UNREFERENCED_PARAMETER(bAnsi);

    /*
     * Probe arguments
     */
    try {
        ProbeForWritePoint5((LPPOINT5)lParam);
        pt5 = *(LPPOINT5)lParam;
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = CALLPROC(xpfnProc)(
                pwnd,
                msg,
                wParam,
                (LPARAM)&pt5,
                xParam);

    try {
        *(LPPOINT5)lParam = pt5;
    } except (StubExceptionHandler(FALSE)) {
    }

    TRACE("fnINOUTLPPOINT5");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(INSTRING)
{
    LARGE_STRING str;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnINSTRING");

    /*
     * Don't allow any app to send a LB_DIR or CB_DIR with the postmsgs bit
     * set (ObjectVision does this). This is because there is actually a legal
     * case that we need to thunk of user posting a LB_DIR or CB_DIR
     * (DlgDirListHelper()). In the post case, we thunk the lParam (pointer
     * to a string) differently, and we track that post case with the
     * DDL_POSTMSGS bit. If an app sends a message with this bit, then our
     * thunking gets confused, so clear it here. Let's hope that no app
     * depends on this bit set when either of these messages are sent.
     *
     * These messages should return -1 on failure
     */
    switch (msg) {
    case LB_DIR:
    case CB_DIR:
        wParam &= ~DDL_POSTMSGS;
        /* Fall through */

    case LB_ADDFILE:
#if (LB_ERR != CB_ERR)
#error LB_ERR/CB_ERR conflict
#endif
        errret = LB_ERR;
        break;
    }

    /*
     * Probe arguments
     */
    try {
        if (bAnsi) {
            ProbeForRead((LPSTR)lParam, sizeof(CHAR), sizeof(CHAR));
            RtlInitLargeAnsiString((PLARGE_ANSI_STRING)&str,
                    (LPSTR)lParam, (UINT)-1);
        } else {
            ProbeForRead((LPWSTR)lParam, sizeof(WCHAR), CHARALIGN);
            RtlInitLargeUnicodeString((PLARGE_UNICODE_STRING)&str,
                    (LPWSTR)lParam, (UINT)-1);
        }
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    /*
     * !!! str.Buffer must be captured in xxxInterSendMsgEx
     */
    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            (LPARAM)&str,
            xParam);

    TRACE("fnINSTRING");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(INSTRINGNULL)
{
    LARGE_STRING str;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnINSTRINGNULL");

    /*
     * Probe arguments
     */
    if (ARGUMENT_PRESENT(lParam)) {
        try {
            if (bAnsi) {
                ProbeForRead((LPSTR)lParam, sizeof(CHAR), sizeof(CHAR));
                RtlInitLargeAnsiString((PLARGE_ANSI_STRING)&str,
                        (LPSTR)lParam, (UINT)-1);
            } else {
                ProbeForRead((LPWSTR)lParam, sizeof(WCHAR), CHARALIGN);
                RtlInitLargeUnicodeString((PLARGE_UNICODE_STRING)&str,
                        (LPWSTR)lParam, (UINT)-1);
            }
            lParam = (LPARAM)&str;
        } except (StubExceptionHandler(FALSE)) {
            MSGERROR(0);
        }
    }

    /*
     * !!! str.Buffer must be captured in xxxInterSendMsgEx
     */
    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            lParam,
            xParam);

    TRACE("fnINSTRINGNULL");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(INDEVICECHANGE)
{
    BOOL fPtr    = (BOOL)((wParam & 0x8000) == 0x8000);
    DWORD cbSize;
    PBYTE bfr = NULL;
    TL tlBuffer;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnINDEVICECHANGE");

    UNREFERENCED_PARAMETER(bAnsi);

    /*
     * Probe arguments
     */
    if (fPtr && lParam) {
        struct _DEV_BROADCAST_HEADER *pHdr;
        PDEV_BROADCAST_DEVICEINTERFACE_W pInterfaceW;
        PDEV_BROADCAST_PORT_W pPortW;
        PDEV_BROADCAST_HANDLE pHandleW;
        try {
            pHdr = (struct _DEV_BROADCAST_HEADER *)lParam;
            cbSize = ProbeAndReadUlong(&(pHdr->dbcd_size));
            if (cbSize < sizeof(*pHdr)) {
                MSGERROR(ERROR_INVALID_PARAMETER);
            }
            ProbeForRead(pHdr, cbSize, sizeof(BYTE));

            bfr = UserAllocPoolWithQuota(cbSize+2, TAG_DEVICECHANGE); // add space for trailing NULL for test
            if (bfr == NULL) {
                RIPERR0(ERROR_NOT_ENOUGH_MEMORY, RIP_WARNING, "fnINDEVICECHANGE: LocalAlloc failed.");
                MSGERRORCLEANUP(0);
            }

            ThreadLockPool(ptiCurrent, bfr, &tlBuffer);

            RtlCopyMemory(bfr,  (PBYTE)lParam,
                        cbSize);
            ((PWSTR)bfr)[cbSize/sizeof(WCHAR)] = 0;  // trailing null to halt wcslen scan
            lParam = (LPARAM)bfr;
            pHdr = (struct _DEV_BROADCAST_HEADER *)lParam;
            if (pHdr->dbcd_size != cbSize) {
                MSGERRORCLEANUP(0);
            }
            switch(pHdr->dbcd_devicetype) {
            case DBT_DEVTYP_PORT:
                pPortW = (PDEV_BROADCAST_PORT_W)lParam;
                if ((1+wcslen(pPortW->dbcp_name))*sizeof(WCHAR) + FIELD_OFFSET(DEV_BROADCAST_PORT_W, dbcp_name) > cbSize) {
                    MSGERRORCLEANUP(0);
                }
                break;
            case DBT_DEVTYP_DEVICEINTERFACE:
                pInterfaceW = (PDEV_BROADCAST_DEVICEINTERFACE_W)lParam;
                if ((1+wcslen(pInterfaceW->dbcc_name))*sizeof(WCHAR) + FIELD_OFFSET(DEV_BROADCAST_DEVICEINTERFACE_W, dbcc_name) > cbSize) {
                    MSGERRORCLEANUP(0);
                }
                break;
            case DBT_DEVTYP_HANDLE:
                pHandleW = (PDEV_BROADCAST_HANDLE)lParam;
            /*
             * Check if there is any text.
             */

                if (wParam != DBT_CUSTOMEVENT) {
                    if (FIELD_OFFSET(DEV_BROADCAST_HANDLE, dbch_eventguid) > cbSize) {
                        MSGERRORCLEANUP(0);
                    }
                    break;
                }
                if (pHandleW->dbch_nameoffset < 0) {
                    if (FIELD_OFFSET(DEV_BROADCAST_HANDLE, dbch_data) > cbSize) {
                        MSGERRORCLEANUP(0);
                    }
                    break;
                }
                if (pHandleW->dbch_nameoffset & (CHARALIGN - 1)) {
                    ExRaiseDatatypeMisalignment();                                                            \
                }
                if ((DWORD)(FIELD_OFFSET(DEV_BROADCAST_HANDLE, dbch_data) + pHandleW->dbch_nameoffset) > cbSize) {
                    MSGERRORCLEANUP(0);
                }
                if (FIELD_OFFSET(DEV_BROADCAST_HANDLE, dbch_data) + pHandleW->dbch_nameoffset +
                    (1+wcslen((LPWSTR)(pHandleW->dbch_data+pHandleW->dbch_nameoffset)))*sizeof(WCHAR) >
                    cbSize) {
                    MSGERRORCLEANUP(0);
                }
                break;

            }

        } except (StubExceptionHandler(FALSE)) {
            MSGERRORCLEANUP(0);
        }

    }

    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            lParam,
            xParam);

    CLEANUPRECV();
    if (bfr)
        ThreadUnlockAndFreePool(ptiCurrent, &tlBuffer);

    TRACE("fnINDEVICECHANGE");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(INOUTNCCALCSIZE)
{
    NCCALCSIZE_PARAMS params;
    WINDOWPOS pos;
    PWINDOWPOS pposClient;
    RECT rc;
    LPARAM lParamLocal;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnINOUTNCCALCSIZE");

    UNREFERENCED_PARAMETER(bAnsi);

    /*
     * Probe arguments
     */
    try {
        if (wParam != 0) {
            ProbeForWriteNCCalcSize((LPNCCALCSIZE_PARAMS)lParam);
            params = *(LPNCCALCSIZE_PARAMS)lParam;
            ProbeForWriteWindowPos(params.lppos);
            pposClient = params.lppos;
            pos = *params.lppos;
            params.lppos = &pos;
            lParamLocal = (LPARAM)&params;
        } else {
            ProbeForWriteRect((LPRECT)lParam);
            rc = *(LPRECT)lParam;
            lParamLocal = (LPARAM)&rc;
        }
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }
    retval = CALLPROC(xpfnProc)(
                pwnd,
                msg,
                wParam,
                lParamLocal,
                xParam);

    try {
        if (wParam != 0) {
            *(LPNCCALCSIZE_PARAMS)lParam = params;
            ((LPNCCALCSIZE_PARAMS)lParam)->lppos = pposClient;
            *pposClient = pos;
        } else {
            *(LPRECT)lParam = rc;
        }
    } except (StubExceptionHandler(FALSE)) {
    }

    TRACE("fnINOUTNCCALCSIZE");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(INOUTSTYLECHANGE)
{
    STYLESTRUCT ss;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnINOUTSTYLECHANGE");

    UNREFERENCED_PARAMETER(bAnsi);

    /*
     * Probe arguments
     */
    try {
        ProbeForWriteStyleStruct((LPSTYLESTRUCT)lParam);
        ss = *(LPSTYLESTRUCT)lParam;
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = CALLPROC(xpfnProc)(
                pwnd,
                msg,
                wParam,
                (LPARAM)&ss,
                xParam);

    try {
        *(LPSTYLESTRUCT)lParam = ss;
    } except (StubExceptionHandler(FALSE)) {
    }

    TRACE("fnINOUTSTYLECHANGE");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(INOUTLPRECT)
{
    RECT rc;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL((msg == LB_GETITEMRECT ? LB_ERR : 0));
    TRACETHUNK("fnINOUTLPRECT");

    UNREFERENCED_PARAMETER(bAnsi);

    /*
     * Probe arguments
     */
    try {
        ProbeForWriteRect((PRECT)lParam);
        rc = *(PRECT)lParam;
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = CALLPROC(xpfnProc)(
                pwnd,
                msg,
                wParam,
                (LPARAM)&rc,
                xParam);

    try {
        *(PRECT)lParam = rc;
    } except (StubExceptionHandler(FALSE)) {
    }

    TRACE("fnINOUTLPRECT");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(OUTLPSCROLLINFO)
{
    SCROLLINFO scrollinfo;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnOUTLPSCROLLINFO");

    UNREFERENCED_PARAMETER(bAnsi);

    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            (LPARAM)&scrollinfo,
            xParam);

    /*
     * Probe arguments
     */
    try {
        ProbeAndWriteStructure((LPSCROLLINFO)lParam, scrollinfo, SCROLLINFO);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    TRACE("fnOUTLPSCROLLINFO");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(OUTLPRECT)
{
    RECT rc;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnOUTLPRECT");

    UNREFERENCED_PARAMETER(bAnsi);

    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            (LPARAM)&rc,
            xParam);

    /*
     * Probe arguments
     */
    try {
        ProbeAndWriteStructure((PRECT)lParam, rc, RECT);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    TRACE("fnOUTLPRECT");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(INLPCOMPAREITEMSTRUCT)
{
    COMPAREITEMSTRUCT compareitemstruct;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnINLPCOMPAREITEMSTRUCT");

    UNREFERENCED_PARAMETER(bAnsi);

    /*
     * Probe arguments
     */
    try {
        compareitemstruct = ProbeAndReadCompareItemStruct((PCOMPAREITEMSTRUCT)lParam);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            (LPARAM)&compareitemstruct,
            xParam);

    TRACE("fnINLPCOMPAREITEMSTRUCT");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(INLPDELETEITEMSTRUCT)
{
    DELETEITEMSTRUCT deleteitemstruct;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnINLPDELETEITEMSTRUCT");

    UNREFERENCED_PARAMETER(bAnsi);

    /*
     * Probe arguments
     */
    try {
        deleteitemstruct = ProbeAndReadDeleteItemStruct((PDELETEITEMSTRUCT)lParam);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            (LPARAM)&deleteitemstruct,
            xParam);

    TRACE("fnINLPDELETEITEMSTRUCT");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(INLPHLPSTRUCT)
{
    HLP hlp;
    LPHLP phlp = NULL;
    TL tlBuffer;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnINLPHLPSTRUCT");

    UNREFERENCED_PARAMETER(bAnsi);

    /*
     * Probe arguments
     */
    try {
        hlp = ProbeAndReadHelp((LPHLP)lParam);
        if (hlp.cbData < sizeof(HLP)) {
            MSGERROR(0);
        }
        phlp = UserAllocPoolWithQuota(hlp.cbData, TAG_SYSTEM);
        if (phlp == NULL) {
            ExRaiseStatus(STATUS_NO_MEMORY);
        }
        ThreadLockPool(ptiCurrent, phlp, &tlBuffer);
        RtlCopyMemory(phlp, (PVOID)lParam, hlp.cbData);
    } except (StubExceptionHandler(FALSE)) {
        MSGERRORCLEANUP(0);
    }

    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            (LPARAM)phlp,
            xParam);

    CLEANUPRECV();
    if (phlp) {
        ThreadUnlockAndFreePool(ptiCurrent, &tlBuffer);
    }

    TRACE("fnINLPHLPSTRUCT");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(INLPHELPINFOSTRUCT)
{
    HELPINFO helpinfo;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnINLPHELPINFOSTRUCT");

    UNREFERENCED_PARAMETER(bAnsi);

    /*
     * Probe arguments
     */
    try {
        helpinfo = ProbeAndReadHelpInfo((LPHELPINFO)lParam);
        if (helpinfo.cbSize != sizeof(HELPINFO)) {
            RIPMSG1(RIP_WARNING, "HELPINFO.cbSize %d is wrong", helpinfo.cbSize);
            MSGERROR(ERROR_INVALID_PARAMETER);
        }
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            (LPARAM)&helpinfo,
            xParam);

    TRACE("fnINLPHELPINFOSTRUCT");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(INLPDRAWITEMSTRUCT)
{
    DRAWITEMSTRUCT drawitemstruct;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnINLPDRAWITEMSTRUCT");

    UNREFERENCED_PARAMETER(bAnsi);

    /*
     * Probe arguments
     */
    try {
        drawitemstruct = ProbeAndReadDrawItemStruct((PDRAWITEMSTRUCT)lParam);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            (LPARAM)&drawitemstruct,
            xParam);

    TRACE("fnINLPDRAWITEMSTRUCT");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(INOUTLPMEASUREITEMSTRUCT)
{
    MEASUREITEMSTRUCT measureitemstruct;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnINOUTLPMEASUREITEMSTRUCT");

    UNREFERENCED_PARAMETER(bAnsi);

    /*
     * Probe arguments
     */
    try {
        ProbeForWriteMeasureItemStruct((PMEASUREITEMSTRUCT)lParam);
        measureitemstruct = *(PMEASUREITEMSTRUCT)lParam;
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = CALLPROC(xpfnProc)(
                pwnd,
                msg,
                wParam,
                (LPARAM)&measureitemstruct,
                xParam);

    try {
        *(PMEASUREITEMSTRUCT)lParam = measureitemstruct;
    } except (StubExceptionHandler(FALSE)) {
    }

    TRACE("fnINOUTLPMEASUREITEMSTRUCT");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(OUTSTRING)
{
    LARGE_STRING str;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnOUTSTRING");

    /*
     * Probe all arguments
     */
    try {
        str.bAnsi = bAnsi;
        str.MaximumLength = (ULONG)wParam;
        if (!bAnsi) {
            str.MaximumLength *= sizeof(WCHAR);
        }
        str.Length = 0;
        str.Buffer = (PVOID)lParam;
#if defined(_X86_)
        ProbeForWrite((PVOID)str.Buffer, str.MaximumLength, sizeof(BYTE));
#else
        ProbeForWrite((PVOID)str.Buffer, str.MaximumLength,
                str.bAnsi ? sizeof(BYTE) : sizeof(WORD));
#endif
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    /*
     * !!! String buffer must be created in xxxInterSendMsgEx and
     *     lParam probed for write again upon return.
     */
    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            (LPARAM)&str,
            xParam);

    /*
     * A dialog function returning FALSE means no text to copy out,
     * but an empty string also has retval == 0: put a null char in
     * pstr for the latter case.
     */
    if (!retval && wParam != 0) {
        try {
            NullTerminateString((PVOID)lParam, bAnsi);
        } except (StubExceptionHandler(FALSE)) {
            MSGERROR(0);
        }
    }

    TRACE("fnOUTSTRING");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(OUTDWORDINDWORD)
{
    DWORD dw;

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnOUTDWORDINDWORD");

    UNREFERENCED_PARAMETER(bAnsi);

    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            (WPARAM)&dw,
            lParam,
            xParam);

    /*
     * Probe wParam
     */
    try {
        ProbeAndWriteUlong((PULONG)wParam, dw);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    TRACE("fnOUTDWORDINDWORD");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(INCNTOUTSTRING)
{
    LARGE_STRING str;

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnINCNTOUTSTRING");

    /*
     * Probe arguments
     */
    try {
        str.bAnsi = bAnsi;
        str.MaximumLength = ProbeAndReadUshort((LPWORD)lParam);
        if (str.MaximumLength == 0) {
            RIPMSG0(RIP_WARNING, "fnINCNTOUTSTRING asking for 0 characters back\n");
            MSGERROR(0);
        }
        if (!bAnsi) {
            str.MaximumLength *= sizeof(WCHAR);
        }
        str.Length = 0;
        str.Buffer = (LPBYTE)lParam;
#if defined(_X86_)
        ProbeForWrite((PVOID)str.Buffer, str.MaximumLength, sizeof(BYTE));
#else
        ProbeForWrite((PVOID)str.Buffer, str.MaximumLength,
                str.bAnsi ? sizeof(BYTE) : sizeof(WORD));
#endif
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    /*
     * !!! String buffer must be created in xxxInterSendMsgEx and
     *     lParam probed for write again upon return.
     */
    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            (LPARAM)&str,
            xParam);

    /*
     * A dialog function returning FALSE means no text to copy out,
     * but an empty string also has retval == 0: put a null char in
     * pstr for the latter case.
     */
    if (!retval) {
        try {
            NullTerminateString((PVOID)lParam, bAnsi);
        } except (StubExceptionHandler(FALSE)) {
            MSGERROR(0);
        }
    }

    TRACE("fnINCNTOUTSTRING");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(INCNTOUTSTRINGNULL)
{
    LARGE_STRING str;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnINCNTOUTSTRINGNULL");

    /*
     * Probe arguments
     */
    try {
        if (wParam < 2) {       // This prevents a possible GP
            MSGERROR(0);
        }

        str.bAnsi = bAnsi;
        str.MaximumLength = (ULONG)wParam;
        if (!bAnsi) {
            str.MaximumLength *= sizeof(WCHAR);
        }
        str.Length = 0;
        str.Buffer = (LPBYTE)lParam;
#if defined(_X86_)
        ProbeForWrite((PVOID)str.Buffer, str.MaximumLength, sizeof(BYTE));
#else
        ProbeForWrite((PVOID)str.Buffer, str.MaximumLength,
                str.bAnsi ? sizeof(BYTE) : sizeof(WORD));
#endif
        *((LPWSTR)str.Buffer) = 0;    // mark incase message is not handled
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    /*
     * !!! String buffer must be created in xxxInterSendMsgEx and
     *     lParam probed for write again upon return.
     */
    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            (LPARAM)&str,
            xParam);

    TRACE("fnINCNTOUTSTRINGNULL");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(POUTLPINT)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(LB_ERR);
    /*
     * If we use this for other messages, then that return value might not be appropriate.
     */
    UserAssert(msg == LB_GETSELITEMS);
    TRACETHUNK("fnPOUTLPINT");

    UNREFERENCED_PARAMETER(bAnsi);

    /*
     * Probe arguments
     */
    try {
#if defined(_X86_)
        ProbeForWriteBuffer((LPINT)lParam, wParam, sizeof(BYTE));
#else
        ProbeForWriteBuffer((LPINT)lParam, wParam, sizeof(INT));
#endif
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    /*
     * !!! Buffer must be created in xxxInterSendMsgEx and
     *     lParam probed for write again upon return.
     */
    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            lParam,
            xParam);

    TRACE("fnPOUTLPINT");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(POPTINLPUINT)
{

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnPOPTINLPUINT");

    UNREFERENCED_PARAMETER(bAnsi);

    /*
     * Probe arguments
     */
    try {
#if defined(_X86_)
        if (lParam)
            ProbeForReadBuffer((LPUINT)lParam, wParam, sizeof(BYTE));
#else
        if (lParam)
            ProbeForReadBuffer((LPUINT)lParam, wParam, sizeof(DWORD));
#endif
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    /*
     * !!! Data pointed to by lParam must be captured in xxxInterSendMsgEx
     */
    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            lParam,
            xParam);

    TRACE("fnPOPTINLPUINT");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(INOUTLPWINDOWPOS)
{
    WINDOWPOS pos;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnINOUTLPWINDOWPOS");

    UNREFERENCED_PARAMETER(bAnsi);

    /*
     * Probe arguments
     */
    try {
        ProbeForWriteWindowPos((PWINDOWPOS)lParam);
        pos = *(PWINDOWPOS)lParam;
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = CALLPROC(xpfnProc)(
                pwnd,
                msg,
                wParam,
                (LPARAM)&pos,
                xParam);

    try {
        *(PWINDOWPOS)lParam = pos;
    } except (StubExceptionHandler(FALSE)) {
    }

    TRACE("fnINOUTLPWINDOWPOS");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(INLPWINDOWPOS)
{
    WINDOWPOS pos;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnINLPWINDOWPOS");

    UNREFERENCED_PARAMETER(bAnsi);

    /*
     * Probe arguments
     */
    try {
        pos = ProbeAndReadWindowPos((PWINDOWPOS)lParam);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = CALLPROC(xpfnProc)(
                pwnd,
                msg,
                wParam,
                (LPARAM)&pos,
                xParam);

    TRACE("fnINLPWINDOWPOS");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(INLBOXSTRING)
{
    BEGINRECV_MESSAGECALL(LB_ERR);
    TRACETHUNK("fnINLBOXSTRING");

    if (!(pwnd->style & LBS_HASSTRINGS) &&
            (pwnd->style & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE))) {
        retval = CALLPROC(xpfnProc)(
                pwnd,
                msg,
                wParam,
                lParam,
                xParam);
    } else if (msg == LB_FINDSTRING) {
        retval = NtUserfnINSTRINGNULL(
                pwnd,
                msg,
                wParam,
                lParam,
                xParam,
                xpfnProc,
                bAnsi);
    } else {
        retval = NtUserfnINSTRING(
                pwnd,
                msg,
                wParam,
                lParam,
                xParam,
                xpfnProc,
                bAnsi);
    }

    TRACE("fnINLBOXSTRING");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(OUTLBOXSTRING)
{
    LARGE_STRING str;

    BEGINRECV_MESSAGECALL(LB_ERR);
    TRACETHUNK("fnOUTLBOXSTRING");

    /*
     * Need to get the string length ahead of time. This isn't passed in
     * with this message. Code assumes app already knows the size of
     * the string and has passed a pointer to a buffer of adequate size.
     * To do client/server copying of this string, we need to ahead of
     * time the Unicode size of this string. We add one character because
     * GETTEXTLEN excludes the null terminator.
     */
    retval = NtUserfnGETDBCSTEXTLENGTHS(
            pwnd,
            LB_GETTEXTLEN,
            wParam,
            lParam,
            xParam,
            xpfnProc,
            /*IS_DBCS_ENABLED() &&*/ bAnsi);   // HiroYama: LATER
    if (retval == LB_ERR)
        MSGERROR(0);
    retval++;

    /*
     * Probe all arguments
     */
    try {
        str.bAnsi = bAnsi;
        if (bAnsi)
            str.MaximumLength = (ULONG)retval;
        else
            str.MaximumLength = (ULONG)retval * sizeof(WCHAR);
        str.Length = 0;
        str.Buffer = (PVOID)lParam;
#if defined(_X86_)
        ProbeForWrite((PVOID)str.Buffer, str.MaximumLength, sizeof(BYTE));
#else
        ProbeForWrite((PVOID)str.Buffer, str.MaximumLength,
                str.bAnsi ? sizeof(BYTE) : sizeof(WORD));
#endif
    } except (StubExceptionHandler(FALSE)) {
          MSGERROR(0);
    }

    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            (LPARAM)&str,
            xParam);

    /*
     * If the control is ownerdraw and does not have the LBS_HASSTRINGS
     * style, then a 32-bits of application data has been obtained,
     * not a string.
     */
    if (!(pwnd->style & LBS_HASSTRINGS) &&
            (pwnd->style & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE))) {
        if (bAnsi) {
            retval = sizeof(ULONG_PTR)/sizeof(CHAR);     // 4 CHARs just like win3.1
        } else {
            retval = sizeof(ULONG_PTR)/sizeof(WCHAR);    // 2 WCHARs
        }
    }

    TRACE("fnOUTLBOXSTRING");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(INCBOXSTRING)
{
    BEGINRECV_MESSAGECALL(CB_ERR);
    TRACETHUNK("fnINCBOXSTRING");

    if (!(pwnd->style & CBS_HASSTRINGS) &&
            (pwnd->style & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE))) {
        retval = CALLPROC(xpfnProc)(
                pwnd,
                msg,
                wParam,
                lParam,
                xParam);
    } else if (msg == CB_FINDSTRING) {
        retval =  NtUserfnINSTRINGNULL(
                pwnd,
                msg,
                wParam,
                lParam,
                xParam,
                xpfnProc,
                bAnsi);
    } else {
        retval = NtUserfnINSTRING(
                pwnd,
                msg,
                wParam,
                lParam,
                xParam,
                xpfnProc,
                bAnsi);
    }

    TRACE("fnINCBOXSTRING");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(OUTCBOXSTRING)
{
    LARGE_STRING str;

    BEGINRECV_MESSAGECALL(CB_ERR);
    TRACETHUNK("fnOUTCBOXSTRING");

    /*
     * Need to get the string length ahead of time. This isn't passed in
     * with this message. Code assumes app already knows the size of
     * the string and has passed a pointer to a buffer of adequate size.
     * To do client/server copying of this string, we need to ahead of
     * time the size of this string. We add one character because
     * GETTEXTLEN excludes the null terminator.
     */
    retval = NtUserfnGETDBCSTEXTLENGTHS(
            pwnd,
            CB_GETLBTEXTLEN,
            wParam,
            lParam,
            xParam,
            xpfnProc,
            /*IS_DBCS_ENABLED() &&*/ bAnsi);   // HiroYama: LATER
    if (retval == CB_ERR)
        MSGERROR(0);
    retval++;

    /*
     * Probe all arguments
     */
    try {
        str.bAnsi = bAnsi;
        if (bAnsi)
            str.MaximumLength = (ULONG)retval;
        else
            str.MaximumLength = (ULONG)retval * sizeof(WCHAR);
        str.Length = 0;
        str.Buffer = (PVOID)lParam;
#if defined(_X86_)
        ProbeForWrite((PVOID)str.Buffer, str.MaximumLength, sizeof(BYTE));
#else
        ProbeForWrite((PVOID)str.Buffer, str.MaximumLength,
                str.bAnsi ? sizeof(BYTE) : sizeof(WORD));
#endif
    } except (StubExceptionHandler(FALSE)) {
          MSGERROR(0);
    }

    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            (LPARAM)&str,
            xParam);

    /*
     * If the control is ownerdraw and does not have the CBS_HASSTRINGS
     * style, then a 32-bits of application data has been obtained,
     * not a string.
     */
    if (!(pwnd->style & CBS_HASSTRINGS) &&
            (pwnd->style & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE))) {
        if (bAnsi) {
            retval = sizeof(ULONG_PTR)/sizeof(CHAR);     // 4 CHARs just like win3.1
        } else {
            retval = sizeof(ULONG_PTR)/sizeof(WCHAR);    // 2 WCHARs
        }
    }

    TRACE("fnOUTCBOXSTRING");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(INWPARAMCHAR)
{
    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnINWPARAMCHAR");

    /*
     * The server always expects the characters to be unicode so
     * if this was generated from an ANSI routine convert it to Unicode
     */
    if (bAnsi) {
        if (msg == WM_CHARTOITEM || msg == WM_MENUCHAR) {
            WPARAM dwT = wParam & 0xFFFF;                // mask of caret pos
            RtlMBMessageWParamCharToWCS(msg, &dwT);     // convert key portion
            wParam = MAKELONG(LOWORD(dwT),HIWORD(wParam));  // rebuild pos & key wParam
        } else {
            RtlMBMessageWParamCharToWCS(msg, &wParam);
        }
    }

    retval = CALLPROC(xpfnProc)(
            pwnd,
            msg,
            wParam,
            lParam,
            xParam);

    TRACE("fnINWPARAMCHAR");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(KERNELONLY)
{
    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnKERNELONLY");

    UNREFERENCED_PARAMETER(pwnd);
    UNREFERENCED_PARAMETER(msg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(xParam);
    UNREFERENCED_PARAMETER(xpfnProc);
    UNREFERENCED_PARAMETER(bAnsi);

    RIPMSG0(RIP_WARNING,
            "Message sent from client to kernel for a process which has only kernel side\n" );

    retval = 0;

    TRACE("fnKERNELONLY");
    ENDRECV_MESSAGECALL();
}

MESSAGECALL(IMECONTROL)
{
    CANDIDATEFORM   CandForm;
    COMPOSITIONFORM CompForm;
    LOGFONTW        LogFontW;
    LPARAM          lData = lParam;
    PSOFTKBDDATA    pSoftKbdData = NULL;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnIMECONTROL");

    UNREFERENCED_PARAMETER(bAnsi);

    /*
     * wParam range validation:
     * No need to check lower limit, 'cause we assume IMC_FIRST == 0
     * and wParam is unsigned.
     */
    #if (IMC_FIRST != 0)
    #error IMC_FIRST: unexpected value
    #endif
    if (msg != WM_IME_CONTROL || wParam > IMC_LAST) {
        MSGERROR(0);
    }

    /*
     * Probe arguments
     */
    try {
        switch (wParam) {
        case IMC_GETCANDIDATEPOS:
            ProbeForWriteCandidateForm((PCANDIDATEFORM)lParam);
            break;

        case IMC_GETCOMPOSITIONWINDOW:
            ProbeForWriteCompositionForm((PCOMPOSITIONFORM)lParam);
            break;

        case IMC_GETCOMPOSITIONFONT:
        case IMC_GETSOFTKBDFONT:
            ProbeForWriteLogFontW((PLOGFONTW)lParam);
            break;

        case IMC_SETCANDIDATEPOS:
            CandForm = ProbeAndReadCandidateForm((PCANDIDATEFORM)lParam);
            lData = (LPARAM)&CandForm;
            break;

        case IMC_SETCOMPOSITIONWINDOW:
            CompForm = ProbeAndReadCompositionForm((PCOMPOSITIONFORM)lParam);
            lData = (LPARAM)&CompForm;
            break;

        case IMC_SETCOMPOSITIONFONT:
            LogFontW = ProbeAndReadLogFontW((PLOGFONTW)lParam);
            lData = (LPARAM)&LogFontW;
            break;

        case IMC_SETSOFTKBDDATA:
            pSoftKbdData = ProbeAndCaptureSoftKbdData((PSOFTKBDDATA)lParam);
            if (pSoftKbdData == NULL)
                MSGERROR(0);
            lData = (LPARAM)pSoftKbdData;
            break;

        default:
            break;
        }

    } except (StubExceptionHandler(FALSE)) {
        MSGERRORCLEANUP(0);
    }
    retval = CALLPROC(xpfnProc)(
                pwnd,
                msg,
                wParam,
                lData,
                xParam);

    CLEANUPRECV();
    if (pSoftKbdData != NULL) {
        UserFreePool(pSoftKbdData);
    }

    TRACE("fnIMECONTROL");
    ENDRECV_MESSAGECALL();
}

#ifdef LATER
MESSAGECALL(IMEREQUEST)
{
    LPARAM          lData = lParam;

    //
    // N.B. This function has implicit window translation and thread locking
    //      enabled. These operations are performed in the User server API
    //      dispatcher.
    //

    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnIMEREQUEST");

    UNREFERENCED_PARAMETER(bAnsi);

//    RIPMSG0(RIP_ERROR, "MESSAGECALL(IMEREQUEST) called.\n");

    if (GETPTI(pwnd) != PtiCurrent()) {
        /*
         * Does not allow to send WM_IME_REQUEST to
         * the different thread.
         */
        MSGERROR(ERROR_WINDOW_OF_OTHER_THREAD);
    }

    /*
     * Probe arguments
     */
    try {
        switch (wParam) {
        case IMR_COMPOSITIONWINDOW:
            ProbeForWriteCompositionForm((PCOMPOSITIONFORM)lParam);
            break;

        case IMR_CANDIDATEWINDOW:
            ProbeForWriteCandidateForm((PCANDIDATEFORM)lParam);
            break;

        case IMR_COMPOSITIONFONT:
            ProbeForWriteLogFontW((PLOGFONTW)lParam);
            break;

        case IMR_RECONVERTSTRING:
        case IMR_DOCUMENTFEED:
            if (lParam) {
                ProbeForWriteReconvertString((LPRECONVERTSTRING)lParam);
            }
            break;

        case IMR_CONFIRMRECONVERTSTRING:
            //ProbeAndCaptureReconvertString((LPRECONVERTSTRING)lParam);
            //ProbeForWriteReconvertString((LPRECONVERTSTRING)lParam);
            ProbeForReadReconvertString((LPRECONVERTSTRING)lParam);
            break;

        case IMR_QUERYCHARPOSITION:
            ProbeForWriteImeCharPosition((LPPrivateIMECHARPOSITION)lParam);
            break;

        default:
            MSGERROR(0);
        }

    } except (StubExceptionHandler(FALSE)) {
        MSGERRORCLEANUP(0);
    }

    retval = CALLPROC(xpfnProc)(
                pwnd,
                msg,
                wParam,
                lData,
                xParam);

    CLEANUPRECV();

    TRACE("fnIMEREQUEST");
    ENDRECV_MESSAGECALL();
}
#endif

/*
 * Hook stubs
 */

LRESULT NtUserfnHkINLPCBTCREATESTRUCT(
    IN UINT msg,
    IN WPARAM wParam,
    IN LPCBT_CREATEWND pcbt,
    IN BOOL bAnsi)
{
    CBT_CREATEWND cbt;
    CREATESTRUCTEX csex;
    LPCREATESTRUCT lpcsSave;

    BEGINRECV_HOOKCALL();

    /*
     * Probe arguments
     */
    try {
        cbt = ProbeAndReadCBTCreateStruct(pcbt);
        ProbeForWriteCreateStruct(cbt.lpcs);
        lpcsSave = cbt.lpcs;
        csex.cs = *cbt.lpcs;
        cbt.lpcs = (LPCREATESTRUCT)&csex;
        if (bAnsi) {
            ProbeForRead(csex.cs.lpszName, sizeof(CHAR), sizeof(CHAR));
            RtlInitLargeAnsiString((PLARGE_ANSI_STRING)&csex.strName,
                    (LPSTR)csex.cs.lpszName, (UINT)-1);
            if (IS_PTR(csex.cs.lpszClass)) {
                ProbeForRead(csex.cs.lpszClass, sizeof(CHAR), sizeof(CHAR));
                RtlInitLargeAnsiString((PLARGE_ANSI_STRING)&csex.strClass,
                        (LPSTR)csex.cs.lpszClass, (UINT)-1);
            }
        } else {
            ProbeForRead(csex.cs.lpszName, sizeof(WCHAR), CHARALIGN);
            RtlInitLargeUnicodeString((PLARGE_UNICODE_STRING)&csex.strName,
                    csex.cs.lpszName, (UINT)-1);
            if (IS_PTR(csex.cs.lpszClass)) {
                ProbeForRead(csex.cs.lpszClass, sizeof(WCHAR), CHARALIGN);
                RtlInitLargeUnicodeString((PLARGE_UNICODE_STRING)&csex.strClass,
                        csex.cs.lpszClass, (UINT)-1);
            }
        }
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = xxxCallNextHookEx(
                msg,
                wParam,
                (LPARAM)&cbt);

    try {
        pcbt->hwndInsertAfter = cbt.hwndInsertAfter;
        lpcsSave->x = cbt.lpcs->x;
        lpcsSave->y = cbt.lpcs->y;
        lpcsSave->cx = cbt.lpcs->cx;
        lpcsSave->cy = cbt.lpcs->cy;
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    TRACE("NtUserfnHkINLPCBTCREATESTRUCT");
    ENDRECV_HOOKCALL();
}

LRESULT NtUserfnHkINLPRECT(
    IN DWORD nCode,
    IN WPARAM wParam,
    IN LPRECT lParam)
{
    RECT rc;

    BEGINRECV_HOOKCALL();

    /*
     * Probe arguments
     */
    try {
        rc = ProbeAndReadRect((PRECT)lParam);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = xxxCallNextHookEx(
            nCode,
            wParam,
            (LPARAM)&rc);

    TRACE("NtUserfnHkINLPRECT");
    ENDRECV_HOOKCALL();
}

#ifdef REDIRECTION

LRESULT NtUserfnHkINLPPOINT(
    IN DWORD   nCode,
    IN WPARAM  wParam,
    IN LPPOINT lParam)
{
    POINT pt;

    BEGINRECV_HOOKCALL();

    /*
     * Probe arguments
     */
    try {
        pt = ProbeAndReadPoint((LPPOINT)lParam);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = xxxCallNextHookEx(
            nCode,
            wParam,
            (LPARAM)&pt);

    TRACE("NtUserfnHkINLPPOINT");
    ENDRECV_HOOKCALL();
}
#endif // REDIRECTION

LRESULT NtUserfnHkINLPMSG(
    IN int iHook,
    IN DWORD nCode,
    IN WPARAM wParam,
    IN LPMSG lParam)
{
    MSG msg;

    BEGINRECV_HOOKCALL();

    /*
     * Probe arguments
     */
    try {
        msg = ProbeAndReadMessage((PMSG)lParam);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = xxxCallNextHookEx(
            nCode,
            wParam,
            (LPARAM)&msg);

    /*
     * If this is GetMessageHook, the hook should be
     * able to change the message, as stated in SDK document.
     */
    if (iHook == WH_GETMESSAGE) {
        try {
            *(PMSG)lParam = msg;
        } except (StubExceptionHandler(FALSE)) {
            MSGERROR(0);
        }
    }

    TRACE("NtUserfnHkINLPMSG");
    ENDRECV_HOOKCALL();
}

LRESULT NtUserfnHkINLPDEBUGHOOKSTRUCT(
    IN DWORD nCode,
    IN WPARAM wParam,
    IN LPDEBUGHOOKINFO lParam)
{
    DEBUGHOOKINFO hookinfo;
    DWORD cbDbgLParam;

    BEGINRECV_HOOKCALL();

    /*
     * Probe arguments
     */
    try {
        hookinfo = ProbeAndReadHookInfo((PDEBUGHOOKINFO)lParam);

        cbDbgLParam = GetDebugHookLParamSize(wParam, &hookinfo);
        ProbeForRead(hookinfo.lParam, cbDbgLParam, DATAALIGN);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = xxxCallNextHookEx(
            nCode,
            wParam,
            (LPARAM)&hookinfo);

    TRACE("NtUserfnHkINLPDEBUGHOOKSTRUCT");
    ENDRECV_HOOKCALL();
}

LRESULT NtUserfnHkOPTINLPEVENTMSG(
    IN DWORD nCode,
    IN WPARAM wParam,
    IN OUT LPEVENTMSGMSG lParam OPTIONAL)
{
    EVENTMSG event;

    BEGINRECV_HOOKCALL();

    /*
     * Probe arguments
     */
    if (ARGUMENT_PRESENT(lParam)) {
        try {
            ProbeForWriteEvent((LPEVENTMSGMSG)lParam);
            event = *(LPEVENTMSGMSG)lParam;
        } except (StubExceptionHandler(FALSE)) {
            MSGERROR(0);
        }
    }

    retval = xxxCallNextHookEx(
            nCode,
            wParam,
            (LPARAM)(lParam ? &event : NULL));

    if (ARGUMENT_PRESENT(lParam)) {
        try {
            *(LPEVENTMSGMSG)lParam = event;
        } except (StubExceptionHandler(FALSE)) {
            MSGERROR(0);
        }
    }

    TRACE("NtUserfnHkINLPEVENTMSG");
    ENDRECV_HOOKCALL();
}

LRESULT NtUserfnHkINLPMOUSEHOOKSTRUCTEX(
    IN DWORD nCode,
    IN WPARAM wParam,
    IN LPMOUSEHOOKSTRUCTEX lParam)
{
    MOUSEHOOKSTRUCTEX mousehook;

    BEGINRECV_HOOKCALL();

    /*
     * Probe arguments
     */
    try {
        mousehook = ProbeAndReadMouseHook((PMOUSEHOOKSTRUCTEX)lParam);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = xxxCallNextHookEx(
            nCode,
            wParam,
            (LPARAM)&mousehook);

    TRACE("NtUserfnHkINLPMOUSEHOOKSTRUCTEX");
    ENDRECV_HOOKCALL();
}

LRESULT NtUserfnHkINLPKBDLLHOOKSTRUCT(
    IN DWORD nCode,
    IN WPARAM wParam,
    IN LPKBDLLHOOKSTRUCT lParam)
{
    KBDLLHOOKSTRUCT kbdhook;

    BEGINRECV_HOOKCALL();

    /*
     * Probe arguments
     */
    try {
        kbdhook = ProbeAndReadKbdHook((PKBDLLHOOKSTRUCT)lParam);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = xxxCallNextHookEx(
            nCode,
            wParam,
            (LPARAM)&kbdhook);

    TRACE("NtUserfnHkINLPKBDLLHOOKSTRUCT");
    ENDRECV_HOOKCALL();
}

LRESULT NtUserfnHkINLPMSLLHOOKSTRUCT(
    IN DWORD nCode,
    IN WPARAM wParam,
    IN LPMSLLHOOKSTRUCT lParam)
{
    MSLLHOOKSTRUCT msllhook;

    BEGINRECV_HOOKCALL();

    /*
     * Probe arguments
     */
    try {
        msllhook = ProbeAndReadMsllHook((PMSLLHOOKSTRUCT)lParam);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = xxxCallNextHookEx(
            nCode,
            wParam,
            (LPARAM)&msllhook);

    TRACE("NtUserfnHkINLPMSLLHOOKSTRUCT");
    ENDRECV_HOOKCALL();
}

#ifdef REDIRECTION
LRESULT NtUserfnHkINLPHTHOOKSTRUCT(
    IN DWORD nCode,
    IN WPARAM wParam,
    IN LPHTHOOKSTRUCT lParam)
{
    HTHOOKSTRUCT hthook;

    BEGINRECV_HOOKCALL();

    /*
     * Probe arguments
     */
    try {
        hthook = ProbeAndReadHTHook((PHTHOOKSTRUCT)lParam);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = xxxCallNextHookEx(
            nCode,
            wParam,
            (LPARAM)&hthook);

    TRACE("NtUserfnHkINLPHTHOOKSTRUCT");
    ENDRECV_HOOKCALL();
}
#endif // REDIRECTION

LRESULT NtUserfnHkINLPCBTACTIVATESTRUCT(
    IN DWORD nCode,
    IN WPARAM wParam,
    IN LPCBTACTIVATESTRUCT lParam)
{
    CBTACTIVATESTRUCT cbtactivate;

    BEGINRECV_HOOKCALL();

    /*
     * Probe arguments
     */
    try {
        cbtactivate = ProbeAndReadCBTActivateStruct((LPCBTACTIVATESTRUCT)lParam);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = xxxCallNextHookEx(
            nCode,
            wParam,
            (LPARAM)&cbtactivate);

    TRACE("NtUserfnHkINLPCBTACTIVATESTRUCT");
    ENDRECV_HOOKCALL();
}

LRESULT NtUserCallNextHookEx(
    int nCode,
    WPARAM wParam,
    LPARAM lParam,
    BOOL bAnsi)
{
    BEGINRECV(LRESULT, 0);

    if (PtiCurrent()->sphkCurrent == NULL) {
        MSGERROR(0);
    }

    switch (PtiCurrent()->sphkCurrent->iHook) {
    case WH_CBT:
        /*
         * There are many different types of CBT hooks!
         */
        switch (nCode) {
        case HCBT_CLICKSKIPPED:
            goto MouseHook;
            break;

        case HCBT_CREATEWND:
            /*
             * This hook type points to a CREATESTRUCT, so we need to
             * be fancy it's thunking, because a CREATESTRUCT contains
             * a pointer to CREATEPARAMS which can be anything... so
             * funnel this through our message thunks.
             */
            retval =  NtUserfnHkINLPCBTCREATESTRUCT(
                    nCode,
                    wParam,
                    (LPCBT_CREATEWND)lParam,
                    bAnsi);
            break;

#ifdef REDIRECTION
        case HCBT_GETCURSORPOS:

            /*
             * This hook type points to a POINT structure.
             */
            retval = NtUserfnHkINLPPOINT(nCode, wParam, (LPPOINT)lParam);
            break;
#endif // REDIRECTION

        case HCBT_MOVESIZE:

            /*
             * This hook type points to a RECT structure.
             */
            retval = NtUserfnHkINLPRECT(nCode, wParam, (LPRECT)lParam);
            break;

        case HCBT_ACTIVATE:
            /*
             * This hook type points to a CBTACTIVATESTRUCT
             */
            retval = NtUserfnHkINLPCBTACTIVATESTRUCT(nCode, wParam,
                    (LPCBTACTIVATESTRUCT)lParam);
            break;

        default:
            /*
             * The rest of the cbt hooks are all dword parameters.
             */
            retval = xxxCallNextHookEx(
                    nCode,
                    wParam,
                    lParam);
            break;
        }
        break;

    case WH_FOREGROUNDIDLE:
    case WH_KEYBOARD:
    case WH_SHELL:
        /*
         * These are dword parameters and are therefore real easy.
         */
        retval = xxxCallNextHookEx(
                nCode,
                wParam,
                lParam);
        break;

    case WH_MSGFILTER:
    case WH_SYSMSGFILTER:
    case WH_GETMESSAGE:
        /*
         * These take an lpMsg as their last parameter. Since these are
         * exclusively posted parameters, and since nowhere on the server
         * do we post a message with a pointer to some other structure in
         * it, the lpMsg structure contents can all be treated verbatim.
         */
        retval = NtUserfnHkINLPMSG(PtiCurrent()->sphkCurrent->iHook, nCode, wParam, (LPMSG)lParam);
        break;

    case WH_JOURNALPLAYBACK:
    case WH_JOURNALRECORD:
        /*
         * These take an OPTIONAL lpEventMsg.
         */
        retval = NtUserfnHkOPTINLPEVENTMSG(nCode, wParam, (LPEVENTMSGMSG)lParam);
        break;

    case WH_DEBUG:
        /*
         * This takes an lpDebugHookStruct.
         */
        retval = NtUserfnHkINLPDEBUGHOOKSTRUCT(nCode, wParam, (LPDEBUGHOOKINFO)lParam);
        break;

    case WH_KEYBOARD_LL:
        /*
         * This takes an lpKbdllHookStruct.
         */
        retval = NtUserfnHkINLPKBDLLHOOKSTRUCT(nCode, wParam, (LPKBDLLHOOKSTRUCT)lParam);
        break;

    case WH_MOUSE_LL:
        /*
         * This takes an lpMsllHookStruct.
         */
        retval = NtUserfnHkINLPMSLLHOOKSTRUCT(nCode, wParam, (LPMSLLHOOKSTRUCT)lParam);
        break;

    case WH_MOUSE:
        /*
         * This takes an lpMouseHookStructEx.
         */
MouseHook:
        retval = NtUserfnHkINLPMOUSEHOOKSTRUCTEX(nCode, wParam, (LPMOUSEHOOKSTRUCTEX)lParam);
        break;

#ifdef REDIRECTION
    case WH_HITTEST:
        /*
         * This takes an lpHTHookStruct.
         */
        retval = NtUserfnHkINLPHTHOOKSTRUCT(nCode, wParam, (LPHTHOOKSTRUCT)lParam);
        break;
#endif // REDIRECTION

    default:
        RIPMSG1(RIP_WARNING, "NtUserCallNextHookEx: Invalid hook type %x",
                PtiCurrent()->sphkCurrent->iHook);
        MSGERROR(0);
    }

    TRACE("NtUserCallNextHookEx");
    ENDRECV();
}


HIMC NtUserCreateInputContext(
    IN ULONG_PTR dwClientImcData)
{
    BEGINRECV(HIMC, (HIMC)NULL);

    ValidateIMMEnabled();

    if (dwClientImcData == 0) {
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "Invalid hMemClientIC parameter");
        MSGERROR(0);
    }

    retval = (HIMC)CreateInputContext(dwClientImcData);

    retval = (HIMC)PtoH((PVOID)retval);

    TRACE("NtUserCreateInputContext");
    ENDRECV();
}


BOOL NtUserDestroyInputContext(
    IN HIMC hImc)
{
    PIMC pImc;

    BEGINATOMICRECV(BOOL, FALSE);

    ValidateIMMEnabled();

    ValidateHIMC(pImc, hImc);

    retval = DestroyInputContext(pImc);

    TRACE("NtUserDestroyInputContext");
    ENDATOMICRECV();
}


AIC_STATUS NtUserAssociateInputContext(
    IN HWND hwnd,
    IN HIMC hImc,
    IN DWORD dwFlag)
{
    PIMC pImc;

    BEGINATOMICRECV_HWND(AIC_STATUS, AIC_ERROR, hwnd);

    ValidateIMMEnabled();

    ValidateHIMCOPT(pImc, hImc);

    retval = AssociateInputContextEx(pwnd, pImc, dwFlag);

    TRACE("NtUserAssociateInputContext");
    ENDATOMICRECV_HWND();
}

BOOL NtUserUpdateInputContext(
    IN HIMC hImc,
    IN UPDATEINPUTCONTEXTCLASS UpdateType,
    IN ULONG_PTR UpdateValue)
{
    PIMC pImc;

    BEGINATOMICRECV(BOOL, FALSE);

    ValidateIMMEnabled();

    ValidateHIMC(pImc, hImc);

    retval = UpdateInputContext(pImc, UpdateType, UpdateValue);

    TRACE("NtUserUpdateInputContext");
    ENDATOMICRECV();
}


ULONG_PTR NtUserQueryInputContext(
    IN HIMC hImc,
    IN INPUTCONTEXTINFOCLASS InputContextInfo)
{
    PTHREADINFO ptiImc;
    PIMC pImc;

    BEGINRECV_SHARED(ULONG_PTR, 0);

    ValidateIMMEnabled();

    ValidateHIMC(pImc, hImc);

    ptiImc = GETPTI(pImc);

    switch (InputContextInfo) {
    case InputContextProcess:
        retval = (ULONG_PTR)ptiImc->pEThread->Cid.UniqueProcess;
        break;

    case InputContextThread:
        retval = (ULONG_PTR)ptiImc->pEThread->Cid.UniqueThread;
        break;

    case InputContextDefaultImeWindow:
        retval = (ULONG_PTR)HW(ptiImc->spwndDefaultIme);
        break;

    case InputContextDefaultInputContext:
        retval = (ULONG_PTR)PtoH(ptiImc->spDefaultImc);
        break;
    }

    ENDRECV_SHARED();
}

NTSTATUS NtUserBuildHimcList(  // private IMM BuildHimcList
    IN DWORD  idThread,
    IN UINT   cHimcMax,
    OUT HIMC *phimcFirst,
    OUT PUINT pcHimcNeeded)
{
    PTHREADINFO pti;
    UINT cHimcNeeded;

    BEGINATOMICRECV(NTSTATUS, STATUS_UNSUCCESSFUL);

    ValidateIMMEnabled();

    switch (idThread) {
    case 0:
        pti = PtiCurrent();
        break;
    case (DWORD)-1:
        pti = NULL;
        break;
    default:
        pti = PtiFromThreadId(idThread);
        if (pti == NULL || pti->rpdesk == NULL) {
            MSGERROR(0);
        }
        break;
    }

    /*
     * Probe arguments
     */
    try {
        ProbeForWriteBuffer(phimcFirst, cHimcMax, sizeof(DWORD));
        ProbeForWriteUlong(pcHimcNeeded);
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    /*
     * phimcFirst is client-side.
     */

    cHimcNeeded = BuildHimcList(pti, cHimcMax, phimcFirst);

    if (cHimcNeeded <= cHimcMax) {
        retval = STATUS_SUCCESS;
    } else {
        retval = STATUS_BUFFER_TOO_SMALL;
    }
    try {
        *pcHimcNeeded = cHimcNeeded;
    } except (StubExceptionHandler(FALSE)) {
    }

    TRACE("NtUserBuildHimcList");
    ENDATOMICRECV();
}


BOOL NtUserGetImeInfoEx(  // private ImmGetImeInfoEx
    IN OUT PIMEINFOEX piiex,
    IN IMEINFOEXCLASS SearchType)
{
    IMEINFOEX iiex;
    BEGINRECV_SHARED(BOOL, FALSE);

    ValidateIMMEnabled();

    try {
        ProbeForWrite(piiex, sizeof(*piiex), sizeof(BYTE));
        RtlCopyMemory(&iiex, piiex, sizeof(IMEINFOEX));
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = GetImeInfoEx(
                    _GetProcessWindowStation(NULL),
                    &iiex,
                    SearchType);

    try {
        RtlCopyMemory(piiex, &iiex, sizeof(IMEINFOEX));
    } except (StubExceptionHandler(FALSE)) {
    }

    TRACE("NtUserGetImeInfoEx");
    ENDRECV_SHARED();
}


BOOL NtUserSetImeInfoEx(
    IN PIMEINFOEX piiex)
{
    IMEINFOEX iiex;
    BEGINRECV(BOOL, FALSE);

    ValidateIMMEnabled();

    /*
     * Probe arguments
     */
    try {
        ProbeForRead(piiex, sizeof(*piiex), sizeof(BYTE));
        RtlCopyMemory(&iiex, piiex, sizeof(IMEINFOEX));
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }

    retval = SetImeInfoEx(
                    _GetProcessWindowStation(NULL),
                    &iiex);

    TRACE("NtUserSetImeInfoEx");
    ENDRECV();
}

BOOL NtUserGetImeHotKey(
    IN DWORD dwID,
    OUT PUINT puModifiers,
    OUT PUINT puVKey,
    OUT LPHKL phkl)
{
    UINT uModifiers;
    UINT uVKey;
    HKL hkl;
    LPHKL phklIn = NULL;
    BEGINRECV(BOOL, FALSE);

    try {
        ProbeForWriteUlong(((PULONG)puModifiers));
        ProbeForWriteUlong(((PULONG)puVKey));
        if (ARGUMENT_PRESENT(phkl)) {
            ProbeForWriteHandle((PHANDLE)phkl);
            phklIn = &hkl;
        }
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }
    retval = GetImeHotKey( dwID, &uModifiers, &uVKey, phklIn);

    try {
        *puModifiers = uModifiers;
        *puVKey = uVKey;
        if (ARGUMENT_PRESENT(phkl)) {
            *phkl = *phklIn;
        }
    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }
    TRACE("NtUserGetImeHotKey");
    ENDRECV();
}

BOOL NtUserSetImeHotKey(
    IN DWORD dwID,
    IN UINT  uModifiers,
    IN UINT  uVKey,
    IN HKL   hkl,
    IN DWORD dwFlags)
{
    BEGINRECV(BOOL, FALSE);

    retval = SetImeHotKey( dwID, uModifiers, uVKey, hkl, dwFlags );
    TRACE("NtUserSetImeHotKey");
    ENDRECV();
}

/*
 * Set per-window application level for IME control.
 * Used only for Korean 3.x ( both 16 bit and 32 bit)
 * applications.
 *
 * return value
 *
 *      TRUE : success
 *      FALSE: error
 */
BOOL NtUserSetAppImeLevel(
    IN HWND  hwnd,
    IN DWORD dwLevel)
{
    BEGINRECV_HWND(BOOL, FALSE, hwnd);

    ValidateIMMEnabled();

    if ( GETPTI(pwnd)->ppi == PpiCurrent() ) {
        InternalSetProp(pwnd, PROP_IMELEVEL, (HANDLE)LongToHandle( dwLevel ), PROPF_INTERNAL | PROPF_NOPOOL);
        retval = TRUE;
    } else {
        MSGERROR(0);
    }
    TRACE("NtUserSetAppImeLevel");
    ENDRECV_HWND();
}

/*
 * Get per-window application level for IME control.
 * Used only for Korean 3.x ( both 16 bit and 32 bit)
 * applications.
 *
 * return value
 *
 *      0               : error
 *      non zero value  : level
 */
DWORD NtUserGetAppImeLevel(
    IN HWND  hwnd)
{
    BEGINRECV_HWND_SHARED(DWORD, 0, hwnd);

    ValidateIMMEnabled();

    if ( GETPTI(pwnd)->ppi == PtiCurrentShared()->ppi ) {
        retval = (DWORD)(ULONG_PTR)_GetProp(pwnd, PROP_IMELEVEL, TRUE);
    } else {
        MSGERROR(0);
    }
    TRACE("NtUserGetAppImeLevel");
    ENDRECV_HWND_SHARED();
}


DWORD NtUserCheckImeHotKey(
    UINT uVKey,
    LPARAM lParam)
{
    PIMEHOTKEYOBJ pImeHotKeyObj;
    BEGINRECV(DWORD, IME_INVALID_HOTKEY);

    if (gpqForeground == NULL)
        MSGERROR(0);

    ValidateIMMEnabled();

    pImeHotKeyObj = CheckImeHotKey(gpqForeground, uVKey, lParam);
    if (pImeHotKeyObj) {
        retval = pImeHotKeyObj->hk.dwHotKeyID;
    }
    else {
        retval = IME_INVALID_HOTKEY;
    }

    TRACE("NtUserCheckImeHotKey");
    ENDRECV();
}

BOOL NtUserSetImeOwnerWindow(
    IN HWND hwndIme,
    IN HWND hwndFocus)
{
    PWND pwndFocus;

    BEGINATOMICRECV_HWND(BOOL, FALSE, hwndIme);

    ValidateIMMEnabled();

    /*
     * Make sure this really is an IME window.
     */
    if (GETFNID(pwnd) != FNID_IME)
        MSGERROR(0);

    ValidateHWNDOPT(pwndFocus, hwndFocus);

    if (pwndFocus != NULL) {
        PWND pwndTopLevel;
        PWND pwndT;

        if (TestCF(pwndFocus, CFIME) ||
                pwndFocus->pcls->atomClassName == gpsi->atomSysClass[ICLS_IME]) {
            RIPMSG0(RIP_WARNING, "Focus window should not be an IME/UI window!!");
            MSGERROR(0);
        }

        /*
         * Child window cannot be an owner window.
         */
        pwndTopLevel = pwndT = GetTopLevelWindow(pwndFocus);

        /*
         * To prevent the IME window becomes the onwer (HY?)
         */
        while (pwndT != NULL) {
            if (pwndT->pcls->atomClassName == gpsi->atomSysClass[ICLS_IME]) {
                RIPMSG0(RIP_WARNING,
                        "The owner of focus window should not be an IME window!!");
                pwndTopLevel = NULL;
                break;
            }
            pwndT = pwndT->spwndOwner;
        }

        UserAssert(pwnd->pcls->atomClassName == gpsi->atomSysClass[ICLS_IME]);
        UserAssert(pwndTopLevel == NULL || !TestCF(pwndTopLevel, CFIME));
        Lock(&pwnd->spwndOwner, pwndTopLevel);
        ImeCheckTopmost(pwnd);
    }
    else {
        PTHREADINFO ptiImeWnd = GETPTI(pwnd);
        PWND pwndActive = ptiImeWnd->pq->spwndActive;

        /*
         * If pwndFocus == NULL, active window in the queue should become the
         * owner window of the IME window, except: if IME related windows
         * somehow got a focus, or the active window belongs to the other thread.
         */
        if (pwndActive == NULL || pwndActive != pwnd->spwndOwner) {
            if (pwndActive == NULL || IsWndImeRelated(pwndActive) || ptiImeWnd != GETPTI(pwndActive)) {
                /*
                 * We should avoid improper window to be an owner of IME window.
                 */
                ImeSetFutureOwner(pwnd, pwnd->spwndOwner);
            }
            else {
                Lock(&pwnd->spwndOwner, pwndActive);
            }
            ImeCheckTopmost(pwnd);
        }
    }

    retval = TRUE;

    TRACE("NtUserSetImeNewOwner");
    ENDATOMICRECV_HWND();
}


VOID NtUserSetThreadLayoutHandles(
    IN HKL hklNew,
    IN HKL hklOld)
{
    PTHREADINFO ptiCurrent;
    PKL         pklNew;

    BEGINRECV_VOID();

    ptiCurrent = PtiCurrent();

    if (ptiCurrent->spklActive != NULL && ptiCurrent->spklActive->hkl != hklOld)
        MSGERROR_VOID();

    if ((pklNew = HKLtoPKL(ptiCurrent, hklNew)) == NULL)
        MSGERROR_VOID();

    /*
     * hklPrev is only used for IME, non-IME toggle hotkey.
     * The purpose we remember hklPrev is to jump from
     * non-IME keyboard layout to the most recently used
     * IME layout, or to jump from an IME layout to
     * the most recently used non-IME layout. Therefore
     * piti->hklPrev is updated only when [ IME -> non-IME ]
     * or [ non-IME -> IME ] transition is happened.
     */
    if (IS_IME_KBDLAYOUT(hklNew) ^ IS_IME_KBDLAYOUT(hklOld))
        ptiCurrent->hklPrev = hklOld;

    Lock(&ptiCurrent->spklActive, pklNew);

    TRACEVOID("NtUserSetThreadLayoutHandles");
    ENDRECV_VOID();
}

VOID NtUserNotifyIMEStatus(
    IN HWND hwnd,
    IN DWORD dwOpen,
    IN DWORD dwConversion)
{
    BEGINRECV_HWNDLOCK_VOID(hwnd);

    ValidateIMMEnabledVOID();

    xxxNotifyIMEStatus( pwnd, dwOpen, dwConversion );

    TRACEVOID("NtUserNotifyIMEStatus");
    ENDRECV_HWNDLOCK_VOID()
}

BOOL NtUserDisableThreadIme(
    IN DWORD dwThreadId)
{
    PTHREADINFO ptiCurrent, pti;

    BEGINRECV(BOOL, FALSE);

    ValidateIMMEnabled();

    ptiCurrent = PtiCurrent();

    if (dwThreadId == -1) {
        // IME processing is disabled for all the thread in the current process
        ptiCurrent->ppi->W32PF_Flags |= W32PF_DISABLEIME;
        // destory IME stuff
        pti = ptiCurrent->ppi->ptiList;
        while (pti) {
            pti->TIF_flags |= TIF_DISABLEIME;
            if (pti->spwndDefaultIme != NULL) {
                xxxDestroyWindow(pti->spwndDefaultIme);
                // Start the search over from beginning
                // Since the ptilist may be updated
                pti = ptiCurrent->ppi->ptiList;
                continue;
            }
            pti = pti->ptiSibling;
        }
    } else {
        if (dwThreadId == 0) {
            pti = ptiCurrent;
        }
        else {
            pti = PtiFromThreadId(dwThreadId);
            if (pti == NULL || pti->ppi != ptiCurrent->ppi)
                MSGERROR(0);
        }
        pti->TIF_flags |= TIF_DISABLEIME;
        if (pti->spwndDefaultIme != NULL) {
            xxxDestroyWindow(pti->spwndDefaultIme);
        }

    }

    retval = TRUE;

    TRACE("NtUserDisableThreadIme");
    ENDRECV();
}


BOOL
NtUserEnumDisplayMonitors(  // API EnumDisplayMonitors
    IN HDC             hdc,
    IN LPCRECT         lprcClip,
    IN MONITORENUMPROC lpfnEnum,
    IN LPARAM          dwData)
{
    RECT    rc;
    LPRECT  lprc = (LPRECT) lprcClip;

    BEGINRECV(BOOL, FALSE);

    /*
     * Probe arguments
     */
    if (ARGUMENT_PRESENT(lprc)) {
        try {
            rc = ProbeAndReadRect(lprc);
            lprc = &rc;
        } except (StubExceptionHandler(TRUE)) {
            MSGERROR(0);
        }
    }

    retval = xxxEnumDisplayMonitors(
            hdc,
            lprc,
            lpfnEnum,
            dwData,
            FALSE);

    TRACE("NtUserEnumDisplayMonitors");
    ENDRECV();
}

/*
 * NtUserQueryUserCounters() retrieves statistics on win32k
 * QUERYUSER_TYPE_USER retrieves the handle counters
 * QUERYUSER_TYPE_CS will fill the result buffer with USER critical section usage data
 */

BOOL
NtUserQueryUserCounters(  // private QueryUserCounters
    IN  DWORD       dwQueryType,
    IN  LPVOID      pvIn,
    IN  DWORD       dwInSize,
    OUT LPVOID      pvResult,
    IN  DWORD       dwOutSize)
{
    PDWORD  pdwInternalIn = NULL;
    PDWORD  pdwInternalResult = NULL;

    BEGINRECV(BOOL, FALSE);

#if defined (USER_PERFORMANCE)
    if (dwQueryType == QUERYUSER_CS) {
        CSSTATISTICS* pcsData;

        if (dwOutSize != sizeof(CSSTATISTICS)) {
            MSGERROR(0);
        }
        try {
            ProbeForWrite((PDWORD)pvResult, dwOutSize, sizeof(DWORD));

            /*
             * Checking for overflow on these counters is caller responsability
             */
            pcsData = (CSSTATISTICS*)pvResult;
            pcsData->cExclusive       = gCSStatistics.cExclusive;
            pcsData->cShared          = gCSStatistics.cShared;
            pcsData->i64TimeExclusive = gCSStatistics.i64TimeExclusive;

        } except (StubExceptionHandler(FALSE)) {
            MSGERROR(0);
        }
        retval = TRUE;
        MSGERROR_VOID();
    }
    else
#endif // USER_PERFORMANCE

    if (dwQueryType == QUERYUSER_HANDLES) {

        /*
         * Probe arguments, dwInSize should be multiple of 4
         */
        if (dwInSize & (sizeof(DWORD)-1) ||
            dwOutSize != TYPE_CTYPES*dwInSize) {

            MSGERROR(0)
        }

        try {
            ProbeForRead((PDWORD)pvIn, dwInSize, sizeof(DWORD));
            pdwInternalIn = UserAllocPoolWithQuota(dwInSize, TAG_SYSTEM);
            if (!pdwInternalIn) {
                ExRaiseStatus(STATUS_NO_MEMORY);
            }
            RtlCopyMemory(pdwInternalIn, pvIn, dwInSize);

            ProbeForWrite(pvResult, dwOutSize, sizeof(DWORD));
            pdwInternalResult = UserAllocPoolWithQuota(dwOutSize, TAG_SYSTEM);
            if (!pdwInternalResult) {
                ExRaiseStatus(STATUS_NO_MEMORY);
            }

        } except (StubExceptionHandler(FALSE)) {
                MSGERRORCLEANUP(0);
        }

        _QueryUserHandles(pdwInternalIn,
                dwInSize/sizeof(DWORD),
                (DWORD (*)[TYPE_CTYPES])pdwInternalResult);
        retval = TRUE;

        try {
            RtlCopyMemory(pvResult, pdwInternalResult, dwOutSize);

        } except (StubExceptionHandler(FALSE)) {
                MSGERRORCLEANUP(0);
        }
    }

    else {

       MSGERROR(0);
    }

    CLEANUPRECV();
    if (pdwInternalIn) {
        UserFreePool(pdwInternalIn);
    }
    if (pdwInternalResult) {
        UserFreePool(pdwInternalResult);
    }

    TRACE("NtUserQueryCounters");
    ENDRECV();
}


/***************************************************************************\
* NtUserINOUTGETMENUINFO
*
* History:
*  11-12-96 GerardoB - Created
\***************************************************************************/
MESSAGECALL(INOUTMENUGETOBJECT)
{
    MENUGETOBJECTINFO mgoi;
    BEGINRECV_MESSAGECALL(0);
    TRACETHUNK("fnINOUTMENUGETOBJECT");

    UNREFERENCED_PARAMETER(bAnsi);

    try {
        /*
         * Capture now so xxxInterSendMsgEx won't have to.
         */
        mgoi = ProbeAndReadMenuGetObjectInfo((PMENUGETOBJECTINFO)lParam);

    } except (StubExceptionHandler(FALSE)) {
        MSGERROR(0);
    }
    retval = CALLPROC(xpfnProc)(
                pwnd,
                msg,
                wParam,
                (LPARAM)&mgoi,
                xParam);

    try {
        *((PMENUGETOBJECTINFO)lParam) = mgoi;

    } except (StubExceptionHandler(FALSE)) {
    }

    TRACE("fnINOUTMENUGETOBJECT");
    ENDRECV_MESSAGECALL();
}

/***************************************************************************\
* NtUserFlashWindowEx
*
* History:
*  11-16-96 MCostea - Created
\***************************************************************************/
BOOL
NtUserFlashWindowEx(  // API FlashWindowEx
    IN PFLASHWINFO pfwi)
{
    FLASHWINFO fwiInternal;
    TL tlpwnd;
    PWND pwnd;

    BEGINRECV(BOOL, FALSE);
    DBG_THREADLOCK_START(FlashWindowEx);

    /*
     * Probe arguments
     */
    try {
        fwiInternal = ProbeAndReadStructure(pfwi, FLASHWINFO);

    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    if ((pwnd = ValidateHwnd(fwiInternal.hwnd)) == NULL ||
        fwiInternal.cbSize != sizeof(FLASHWINFO) ||
        fwiInternal.dwFlags & ~FLASHW_VALID) {

        RIPMSG0(RIP_WARNING, "NtUserFlashWindowEx: Invalid Parameter");
        MSGERROR(ERROR_INVALID_PARAMETER);
    }
    else {
        ThreadLockAlwaysWithPti(PtiCurrent(), pwnd, &tlpwnd);
        retval = xxxFlashWindow(pwnd,
                            MAKELONG(fwiInternal.dwFlags, fwiInternal.uCount),
                            fwiInternal.dwTimeout);
        ThreadUnlock(&tlpwnd);
    }

    DBG_THREADLOCK_END(FlashWindowEx);

    TRACE("NtUserFlashWindowEx");
    ENDRECV();
}

BOOL NtUserUpdateLayeredWindow(  // API UpdateLayeredWindow
    IN HWND hwnd,
    IN HDC hdcDst,
    IN POINT *pptDst,
    IN SIZE *psize,
    IN HDC hdcSrc,
    IN POINT *pptSrc,
    IN COLORREF crKey,
    IN BLENDFUNCTION *pblend,
    IN DWORD dwFlags)
{
    PWND pwnd;
    POINT ptSrc;
    SIZE size;
    POINT ptDst;
    BLENDFUNCTION blend;

    BEGINATOMICRECV(BOOL, FALSE);

    ValidateHWND(pwnd, hwnd);

    /*
     * Probe and validate arguments.
     */
    try {
        if (ARGUMENT_PRESENT(pptSrc)) {
            ptSrc = ProbeAndReadPoint(pptSrc);
            pptSrc = &ptSrc;
        }
        if (ARGUMENT_PRESENT(psize)) {
            size = ProbeAndReadSize(psize);
            psize = &size;
            if (psize->cx < 0 || psize->cy < 0) {
                MSGERROR(ERROR_INVALID_PARAMETER);  // this is a jump out of try!
            }
        }
        if (ARGUMENT_PRESENT(pptDst)) {
            ptDst = ProbeAndReadPoint(pptDst);
            pptDst = &ptDst;
        }

        if (ARGUMENT_PRESENT(pblend)) {
            blend = ProbeAndReadBlendfunction(pblend);
            pblend = &blend;
        }
    } except (StubExceptionHandler(TRUE)) {
        MSGERROR(0);
    }

    if (dwFlags & ~ULW_VALID) {
        RIPMSG0(RIP_WARNING, "UpdateLayeredWindow: Invalid Parameter");
        MSGERROR(ERROR_INVALID_PARAMETER);
    } else {
        retval = _UpdateLayeredWindow(
                pwnd,
                hdcDst,
                pptDst,
                psize,
                hdcSrc,
                pptSrc,
                crKey,
                pblend,
                dwFlags);
    }

    TRACE("NtUserUpdateLayeredWindow");
    ENDATOMICRECV();
}

BOOL NtUserSetLayeredWindowAttributes(
    IN HWND hwnd,
    IN COLORREF crKey,
    IN BYTE bAlpha,
    IN DWORD dwFlags)
{
    PWND pwnd;

    BEGINATOMICRECV(BOOL, FALSE);

    ValidateHWND(pwnd, hwnd);

    if (dwFlags & ~LWA_VALID) {
        RIPMSG0(RIP_WARNING, "SetLayeredWindowAttributes: Invalid Parameter");
        MSGERROR(ERROR_INVALID_PARAMETER);
    } else {
        retval = _SetLayeredWindowAttributes(pwnd, crKey, bAlpha, dwFlags);
    }

    TRACE("NtUserSetLayeredWindowAttributes");
    ENDATOMICRECV();
}

/***************************************************************************\
* GetHDevName
* Called by NtUserCallTwoParam in GetMonitorInfo to query
* gre about the HDev name
*
* 1-July-1998    MCostea      created
\***************************************************************************/
BOOL GetHDevName(HMONITOR hMon, PWCHAR pName)
{
    PMONITOR pMonitor;

    CheckCritIn();

    pMonitor = ValidateHmonitor(hMon);
    if (!pMonitor) {
        return FALSE;
    }

    try {
        ProbeForWrite(pName, CCHDEVICENAME*sizeof(WCHAR), sizeof(DWORD));
    } except (StubExceptionHandler(TRUE)) {
        return FALSE;
    }
    return DrvGetHdevName(pMonitor->hDev, pName);
}
