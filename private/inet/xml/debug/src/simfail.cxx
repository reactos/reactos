//+---------------------------------------------------------------------------
//
//  Microsoft Windows
* Copyright (c) 1992 - 1999 Microsoft Corporation. All rights reserved.//
//  File:       simfail.cxx
//
//  Contents:   Simulated failure testing.
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#ifndef _INC_SHLWAPI
#include <shlwapi.h>
#endif // _INC_SHLWAPI

#ifndef X_OLECTL_H_
#define X_OLECTL_H_
#include <olectl.h>
#endif

#ifndef X_RESOURCE_H_
#define X_RESOURCE_H_
#include "resource.h"
#endif

#ifdef WIN16
#undef _sntprintf
#define _sntprintf(a, b, c, d) wsprintf(a, c, d)
#endif // WIN16

// Timer used to update Count display.
const UINT ID_TIMER = 1;

// Interval of update, in milliseconds.
const UINT TIMER_INTERVAL = 500;

// Number of times FFail is called after g_cfirstFailure is hit.
int     g_cFFailCalled;

// Number of success calls before first failure.  If 0, all calls successful.
int     g_firstFailure;

// Interval to repeat failures after first failure.
int     g_cInterval = 1;

// User defined error for simulated win32 failures.
const DWORD ERR_SIMWIN32 = 0x0200ABAB;

// Handle of simulated failures dialog.
HWND    g_hwndSimFailDlg;

DWORD_PTR WINAPI SimFailDlgThread(LPVOID lpThreadParameter);
extern "C" LRESULT CALLBACK SimFailDlgProc( HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam );

//+---------------------------------------------------------------------------
//
//  Function:   ResetFailCount
//
//  Synopsis:   Resets the count of calls to FFail.
//
//----------------------------------------------------------------------------

void
ResetFailCount()
{
    LOCK_GLOBALS;
    Assert(g_firstFailure >= 0);
    g_cFFailCalled = (g_firstFailure != 0) ? -g_firstFailure : INT_MIN;
}



//+---------------------------------------------------------------------------
//
//  Function:   SetSimFailCounts
//
//  Synopsis:   Sets the parameters for simulated failures, and resets the
//              count of failures.
//
//  Arguments:  [firstFailure] -- Number of successes - 1 before first failure.
//                                If 0, simulated failures are turned off.
//                                If -1, parameter is ignored.
//
//              [cInterval]    -- Interval at which success are repeated.
//                                If 0, set to 1.
//                                If -1, parameter is ignored.
//
//  Notes:      To reset the count of failures,
//              call SetSimFailCounts(-1, -1).
//
//----------------------------------------------------------------------------

void
SetSimFailCounts(int firstFailure, int cInterval)
{
    EnsureThreadState();

    LOCK_GLOBALS;

    if (firstFailure >= 0)
    {
        g_firstFailure = firstFailure;
    }

    if (cInterval > 0)
    {
        g_cInterval = cInterval;
    }
    else if (cInterval == 0)
    {
        g_cInterval = 1;
    }

    ResetFailCount();
}



//+---------------------------------------------------------------------------
//
//  Function:   ShowSimFailDlg
//
//  Synopsis:   Displays the simulated failures dialog in a separate thread.
//
//----------------------------------------------------------------------------

void
ShowSimFailDlg(void)
{
#ifndef _MAC
    THREAD_HANDLE  hThread = NULL;
    ULONG   idThread;

    EnsureThreadState();

    LOCK_RESDLG;

    if (g_hwndSimFailDlg)
    {
        BringWindowToTop(g_hwndSimFailDlg);
    }
    else
    {
        hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) SimFailDlgThread, NULL, 0, &idThread);
        if (!hThread)
        {
            TraceTag((tagError, "CreateThread failed ShowSimFailDlg"));
            goto Cleanup;
        }

        CloseThread(hThread);
    }
Cleanup:
    ;
#else
    SimFailDlgThread(NULL);
#endif      // _MAC
}



//+---------------------------------------------------------------------------
//
//  Function:   SimFailDlgThread
//
//  Synopsis:   Creates the simulated failures dialog and runs a message loop
//              until the dialog is closed.
//
//----------------------------------------------------------------------------

DWORD_PTR WINAPI
SimFailDlgThread(LPVOID lpThreadParameter)
{
#ifndef _MAC
    MSG         msg;

    EnsureThreadState();

    g_hwndSimFailDlg = CreateDialog(
            g_hinstMain,
            MAKEINTRESOURCE(IDD_SIMFAIL),
            NULL,
            (DLGPROC) SimFailDlgProc);

    if (!g_hwndSimFailDlg)
    {
        TraceTag((tagError, "CreateDialogA failed in SimFailDlgEntry"));
        return (DWORD_PTR) -1;
    }

    SetWindowPos(
            g_hwndSimFailDlg,
            HWND_TOP,
            0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);

    while (GetMessage((LPMSG) &msg, (HWND) NULL, 0, 0))
    {
        if (!g_hwndSimFailDlg || (!IsDialogMessage(g_hwndSimFailDlg, &msg)))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (DWORD_PTR)msg.wParam;
#else
    int r;
    r = DialogBox(g_hinstMain, MAKEINTRESOURCE(IDD_SIMFAIL),
                                    NULL, (DLGPROC) SimFailDlgProc);
    if (r == -1)
    {
        MessageBoxA(NULL, "TRI Couldn't create sim failures dialog", "Error",
                   MB_OK | MB_ICONSTOP);
    }

    return (DWORD_PTR) g_hwndSimFailDlg;
#endif  // _MAC
}



//+---------------------------------------------------------------------------
//
//  Function:   SimFailDlg_UpdateTextControls
//
//  Synopsis:   Updates the FirstFail and FailInterval text controls.
//
//----------------------------------------------------------------------------

void
SimFailDlg_UpdateTextControls(HWND hwnd)
{
#ifndef _MACUNICODE
    TCHAR   ach[16];

    LOCK_GLOBALS;

    wnsprintf(ach, ARRAY_SIZE(ach), _T("%d"), g_firstFailure);
    Edit_SetText(GetDlgItem(hwnd, ID_TXTFAIL), ach);
    wnsprintf(ach, ARRAY_SIZE(ach), _T("%d"), g_cInterval);
    Edit_SetText(GetDlgItem(hwnd, ID_TXTINTERVAL), ach);
#else
    CHAR   ach[16];

    wnsprintfAX(ach, ARRAY_SIZE(ach), ("%d"), g_firstFailure);
    SetWindowTextA((GetDlgItem(hwnd, ID_TXTFAIL)), ach);
    wnsprintfAX(ach, ARRAY_SIZE(ach), ("%d"), g_cInterval);
    SetWindowTextA((GetDlgItem(hwnd, ID_TXTINTERVAL)), ach);
#endif
}



//+---------------------------------------------------------------------------
//
//  Function:   SimFailDlg_UpdateCount
//
//  Synopsis:   Updates the count text control.
//
//----------------------------------------------------------------------------

void
SimFailDlg_UpdateCount(HWND hwnd)
{
#ifndef _MACUNICODE
    TCHAR   ach[16];

    wnsprintf(ach, ARRAY_SIZE(ach), _T("%d"), GetFailCount());
    Edit_SetText(GetDlgItem(hwnd, ID_TXTCOUNT), ach);
#else
    CHAR   ach[16];

    wnsprintfAX(ach, ARRAY_SIZE(ach), ("%d"), GetFailCount());
    SetWindowTextA(GetDlgItem(hwnd, ID_TXTCOUNT), ach);
#endif
}



//+---------------------------------------------------------------------------
//
//  Function:   SimFailDlg_UpdateValues
//
//  Synopsis:   Sets the simulated failure counts with values from the
//              dialog.
//
//----------------------------------------------------------------------------

void
SimFailDlg_UpdateValues(HWND hwnd)
{
    int     firstFail;
    int     cInterval;
#ifndef _MACUNICODE
    TCHAR   ach[16];
    LOCK_GLOBALS;

    Edit_GetText(GetDlgItem(hwnd, ID_TXTFAIL), ach, ARRAY_SIZE(ach));
    firstFail = StrToInt(ach);

    Edit_GetText(GetDlgItem(hwnd, ID_TXTINTERVAL), ach, ARRAY_SIZE(ach));
    cInterval = StrToInt(ach);
#else
    CHAR   ach[16];
    GetWindowTextA(GetDlgItem(hwnd, ID_TXTFAIL), ach, ARRAY_SIZE(ach));
    firstFail = StrToInt(ach);

    GetWindowTextA(GetDlgItem(hwnd, ID_TXTINTERVAL), ach, ARRAY_SIZE(ach));
    cInterval = StrToInt(ach);
#endif
    if (firstFail < 0)
    {
        firstFail = 0;
    }
    if (g_cInterval <= 0)
    {
        cInterval = 1;
    }

    SetSimFailCounts(firstFail, cInterval);
    SimFailDlg_UpdateTextControls(hwnd);
    SimFailDlg_UpdateCount(hwnd);
}



//+---------------------------------------------------------------------------
//
//  Function:   SimFailDlg_OnInitDialog
//
//  Synopsis:   Initializes the dialog.
//
//----------------------------------------------------------------------------

BOOL
SimFailDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    Edit_LimitText(GetDlgItem(hwnd, ID_TXTFAIL), 9);
    Edit_LimitText(GetDlgItem(hwnd, ID_TXTINTERVAL), 9);
    SimFailDlg_UpdateTextControls(hwnd);
    SimFailDlg_UpdateCount(hwnd);
    SetTimer(hwnd, ID_TIMER, TIMER_INTERVAL, NULL);
    return TRUE;
}



//+---------------------------------------------------------------------------
//
//  Function:   SimFailDlg_OnCommand
//
//  Synopsis:   Handles button clicks.
//
//----------------------------------------------------------------------------

void
SimFailDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    if (codeNotify != BN_CLICKED)
        return;

    switch (id)
    {
    case ID_BTNUPDATE:
        SimFailDlg_UpdateValues(hwnd);
        break;

    case ID_BTNNEVER:
        SetSimFailCounts(0, 1);
        SimFailDlg_UpdateTextControls(hwnd);
        SimFailDlg_UpdateCount(hwnd);
        break;

    case ID_BTNRESET:
        ResetFailCount();
        SimFailDlg_UpdateCount(hwnd);
        break;
    }
}



//+---------------------------------------------------------------------------
//
//  Function:   SimFailDlg_OnTimer
//
//  Synopsis:   Updates the failure count.
//
//----------------------------------------------------------------------------

void
SimFailDlg_OnTimer(HWND hwnd, UINT id)
{
    Assert(id == ID_TIMER);
    SimFailDlg_UpdateCount(hwnd);
}



//+---------------------------------------------------------------------------
//
//  Function:   SimFailDlg_OnClose
//
//  Synopsis:   Closes the dialog.
//
//----------------------------------------------------------------------------

void
SimFailDlg_OnClose(HWND hwnd)
{
    Verify(DestroyWindow(g_hwndSimFailDlg));
    g_hwndSimFailDlg = NULL;
}



//+---------------------------------------------------------------------------
//
//  Function:   SimFailDlg_OnDestroy
//
//  Synopsis:   Cleans up.
//
//----------------------------------------------------------------------------

void
SimFailDlg_OnDestroy(HWND hwnd)
{
    g_hwndSimFailDlg = NULL;
    KillTimer(hwnd, ID_TIMER);
}



//+---------------------------------------------------------------------------
//
//  Function:   SimFailDlgProc
//
//  Synopsis:   Dialog proc for simulated failures dialog.
//
//----------------------------------------------------------------------------

extern "C"
LRESULT CALLBACK
SimFailDlgProc( HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam )
{
    switch (wMsg)
    {
    case WM_INITDIALOG:
        HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, SimFailDlg_OnInitDialog);
        return TRUE;

    case WM_COMMAND:
        HANDLE_WM_COMMAND(hwnd, wParam, lParam, SimFailDlg_OnCommand);
        return TRUE;

    case WM_TIMER:
        HANDLE_WM_TIMER(hwnd, wParam, lParam, SimFailDlg_OnTimer);
        return TRUE;

    case WM_CLOSE:
        HANDLE_WM_CLOSE(hwnd, wParam, lParam, SimFailDlg_OnClose);
        return TRUE;

    case WM_DESTROY:
        HANDLE_WM_DESTROY(hwnd, wParam, lParam, SimFailDlg_OnDestroy);
        return TRUE;
    }

    return FALSE;
}



//+---------------------------------------------------------------------------
//
//  Function:   TraceFailL
//
//  Synopsis:   Traces failures.  Enable tagTestFailures to see trace output.
//
//              Don't call the function directly, but use the tracing macros
//              in f3debug.h instead.
//
//  Arguments:  [errExpr]  -- The expression to test.
//              [errTest]  -- The fail code to test against.  The only
//                              distinguishing factor is whether it is
//                              zero, negative, or positive.
//              [fIgnore]  -- Is this error being ignored?
//              [pstrExpr] -- The expression as a string.
//              [pstrFile] -- File where expression occurs.
//              [line]     -- Line on which expression occurs.
//
//  Returns:    [errExpr].
//
//----------------------------------------------------------------------------

extern "C" long
TraceFailL(long errExpr, long errTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line)
{
    LPSTR aapstr[2][2] =
    {
        "TRI TFAIL: Failure of \"%s\" at %s:%d <%d>",
        "TRI TFAIL: Simulated failure of \"%s\" at %s:%d <%d>",
        "TRI IGNORE_FAIL: Failure of \"%s\" at %s:%d <%d>",
        "TRI IGNORE_FAIL: Simulated failure of \"%s\" at %s:%d <%d>",
    };

    TraceExit(pstrExpr, pstrFile, line);

    Verify(!IsTagEnabled(tagValidate) || ValidateInternalHeap());

    //
    // Check if errExpr is a success code:
    //     (a) If errTest < 0, then errExpr > 0.  This is for HRESULTs,
    //         list box error codes, etc.
    //     (b) If errTest == 0, the errExpr != 0.  This is for pointers.
    //     (c) If errTest > 0, then errExpr == 0.  This is for the case
    //         where any non-zero error code is an error.  Note that
    //         errTest must be less than the greatest signed integer
    //         (0x7FFFFFFF) for this to work.
    //

    if ((errTest < 0 && errExpr >= 0) ||
        (errTest == 0 && errExpr != 0) ||
        (errTest > 0 && errExpr == 0))
    {
        return errExpr;
    }

    TraceTagEx((
            fIgnore ? tagTestFailuresIgnore : tagTestFailures,
            TAG_NONAME,
            aapstr[fIgnore][JustFailed()],
            pstrExpr,
            pstrFile,
            line,
            errExpr));

    return errExpr;
}



//+---------------------------------------------------------------------------
//
//  Function:   TraceWin32L
//
//  Synopsis:   Traces Win32 failures, displaying the value of GetLastError if
//              the failure is not simulated.  Enable tagTestFailures to see
//              trace output.
//
//              Don't call the function directly, but use the tracing macros
//              in f3debug.h instead.
//
//  Arguments:  [errExpr]  -- The expression to test.
//              [errTest]  -- The fail code to test against.  The only
//                              distinguishing factor is whether it is
//                              zero, negative, or positive.
//              [fIgnore]  -- Is this error being ignored?
//              [pstrExpr] -- The expression as a string.
//              [pstrFile] -- File where expression occurs.
//              [line]     -- Line on which expression occurs.
//
//  Returns:    [errExpr].
//
//----------------------------------------------------------------------------

extern "C" long
TraceWin32L(long errExpr, long errTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line)
{
    LPSTR aapstr[2][2] =
    {
        "TRI TW32: Failure of \"%s\" at %s:%d <%d> GetLastError=<%d>",
        "TRI TW32: Simulated failure of \"%s\" at %s:%d <%d>",
        "TRI IGNORE_W32: Failure of \"%s\" at %s:%d <%d>",
        "TRI IGNORE_W32: Simulated failure of \"%s\" at %s:%d <%d>",
    };

    TraceExit(pstrExpr, pstrFile, line);

    Verify(!IsTagEnabled(tagValidate) || ValidateInternalHeap());

    //
    // Check if errExpr is a success code:
    //     (a) If errTest < 0, then errExpr > 0.  This is for HRESULTs,
    //         list box error codes, etc.
    //     (b) If errTest == 0, the errExpr != 0.  This is for pointers.
    //     (c) If errTest > 0, then errExpr == 0.  This is for the case
    //         where any non-zero error code is an error.  Note that
    //         errTest must be less than the greatest signed integer
    //         (0x7FFFFFFF) for this to work.
    //

    if ((errTest < 0 && errExpr >= 0) ||
        (errTest == 0 && errExpr != 0) ||
        (errTest > 0 && errExpr == 0))
    {
        return errExpr;
    }

    if (JustFailed())
    {
#ifndef WIN16
        SetLastError(ERR_SIMWIN32);
#endif
    }

    TraceTagEx((
            fIgnore ? tagTestFailuresIgnore : tagTestFailures,
            TAG_NONAME,
            aapstr[fIgnore][JustFailed()],
            pstrExpr,
            pstrFile,
            line,
            errExpr,
            GetLastError()));

    return errExpr;
}



//+---------------------------------------------------------------------------
//
//  Function:   TraceHR
//
//  Synopsis:   Traces HRESULT failures.  Enable tagTestFailures to see
//              trace output.
//
//              Don't call the function directly, but use the tracing macros
//              in f3debug.h instead.
//
//  Arguments:  [hrTest]   -- The expression to test.
//              [fIgnore]  -- Is this error being ignored?
//              [pstrExpr] -- The expression as a string.
//              [pstrFile] -- File where expression occurs.
//              [line]     -- Line on which expression occurs.
//
//  Returns:    [hrTest].
//
//----------------------------------------------------------------------------

extern "C" HRESULT
TraceHR(HRESULT hrTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line)
{
    static LPSTR aapstr[2][2] =
    {
        "TRI THR: Failure of \"%s\" at %s:%d %hr",
        "TRI THR: Simulated failure of \"%s\" at %s:%d %hr",
        "TRI IGNORE_HR: Failure of \"%s\" at %s:%d %hr",
        "TRI IGNORE_HR: Simulated failure of \"%s\" at %s:%d %hr",
    };

    // Assert if we get one of the following errors.
    // The caller is doing something wrong if it gets one.

    static long ahrEvil[] = 
    {
        RPC_E_CANTPOST_INSENDCALL,
        RPC_E_CANTCALLOUT_INASYNCCALL,
        RPC_E_CANTCALLOUT_INEXTERNALCALL,
        RPC_E_CANTCALLOUT_AGAIN,
#ifndef WIN16
        RPC_E_CANTCALLOUT_ININPUTSYNCCALL,
        RPC_E_WRONG_THREAD,
        RPC_E_THREAD_NOT_INIT,
#endif
    };

    TraceExit(pstrExpr, pstrFile, line);

    Verify(!IsTagEnabled(tagValidate) || ValidateInternalHeap());

    if (SUCCEEDED(hrTest))
        return hrTest;
    
    TraceTagEx((
            fIgnore ? tagTestFailuresIgnore : tagTestFailures,
            TAG_NONAME,
            aapstr[fIgnore][JustFailed()],
            pstrExpr,
            pstrFile,
            line,
            hrTest));

    for (int i = ARRAY_SIZE(ahrEvil); --i >= 0;)
    {
        if (hrTest == ahrEvil[i])
            Assert(0 && "Unexpected error code encoutered.");
    }

    if ( fIgnore ) {
        // Added if statement to be able to put a breakpoint
        // for failing THRs but not IGNOREHRs
        return hrTest;
    }

    return hrTest;
}

//+---------------------------------------------------------------------------
//
//  Function:   TraceEnter
//
//  Synopsis:   Traces entrace to a THR-wrapped function call
//
//              Don't call the function directly, but use the tracing macros
//              in f3debug.h instead.
//
//  Arguments:  [pstrExpr] -- The expression as a string.
//              [pstrFile] -- File where expression occurs.
//              [line]     -- Line on which expression occurs.
//
//  Returns:    [hrTest].
//
//----------------------------------------------------------------------------

#ifdef WIN16
void EXPORT WINAPI
#else
extern "C" void
#endif
TraceEnter(LPSTR pstrExpr, LPSTR pstrFile, int line)
{
//
//  THIS IS DEBUG TRACE CODE
//  To enter the call wrapped by THR, step out and back in. (VC Shift-F11, F11)
//
    TraceTagEx((
            tagTraceCalls,
            TAG_NONAME | TAG_INDENT,
            "TRI THR Enter \"%s\" at %s:%d",
            pstrExpr,
            pstrFile,
            line));
}

//+---------------------------------------------------------------------------
//
//  Function:   TraceExit
//
//  Synopsis:   Traces exit from a THR-wrapped function call
//
//              Don't call the function directly, but use the tracing macros
//              in f3debug.h instead.
//
//  Arguments:  [pstrExpr] -- The expression as a string.
//              [pstrFile] -- File where expression occurs.
//              [line]     -- Line on which expression occurs.
//
//  Returns:    [hrTest].
//
//----------------------------------------------------------------------------

extern "C" void
TraceExit(LPSTR pstrExpr, LPSTR pstrFile, int line)
{
    TraceTagEx((
            tagTraceCalls,
            TAG_NONAME | TAG_OUTDENT,
            "TRI THR Exit  \"%s\" at %s:%d",
            pstrExpr,
            pstrFile,
            line));
}

//+---------------------------------------------------------------------------
//
//  Function:   TraceOLE
//
//  Synopsis:   Traces OLE interface calls. Enable tagOLEWatch to see
//              trace output.
//
//              Don't call the function directly, but use the tracing macros
//              in f3debug.h instead.
//
//  Arguments:  [hrTest]   -- The expression to test.
//              [fIgnore]  -- Is this error being ignored?
//              [pstrExpr] -- The expression as a string.
//              [pstrFile] -- File where expression occurs.
//              [line]     -- Line on which expression occurs.
//
//  Returns:    [hrTest].
//
//----------------------------------------------------------------------------

extern "C" HRESULT
TraceOLE(HRESULT hrTest, BOOL fIgnore, LPSTR pstrExpr, LPSTR pstrFile, int line, LPVOID lpsite)
{
    Verify(!IsTagEnabled(tagValidate) || ValidateInternalHeap());

	// Note that in the case of failure an imbedded call to the TRH() macro
	// performs the trace dump.
    if (FAILED(hrTest))
        return hrTest;

    TraceTagEx((
            tagOLEWatch,
            TAG_NONAME,
            "TRI THR_OLE:\"%s\" returns %hr at %s:%d site 0x%x",
            pstrExpr,
            hrTest,
            pstrFile,
            line,
			lpsite));

    return hrTest;
}


//+---------------------------------------------------------------------------
//
//  Function:   CheckAndReturnResult
//
//  Synopsis:   Issues a warning if the HRESULT indicates failure, and asserts
//              if the HRESULT is not a permitted success code.
//
//  Arguments:  [hr]        -- the HRESULT to be checked.
//              [pstrFile]  -- the file where the HRESULT is being checked.
//              [line]      -- the line in the file where the HRESULT is
//                                  being checked.
//              [cSuccess]  -- the number of permitted non-zero success codes
//                               or failure SCODES that should not be traced.
//              [...]       -- list of HRESULTS.
//
//  Returns:    The return value is exactly the HRESULT passed in.
//
//  Notes:      This function should not be used directly.  Use
//              the SRETURN and RRETURN macros instead.
//
// HRESULTs passed in should either be permitted success codes, permitted
// non-OLE error codes, or expected OLE error codes.  Expected OLE error codes
// prevent a warning from being printed to the debugger, while the rest cause
// asserts if they're not given as an argument.
//
// An OLE error code has a facility not equal to FACILITY_ITF or is equal to
// FACILITY_ITF and the code is less than the current maximum value used.
//
//----------------------------------------------------------------------------

HRESULT EXPORT CDECL
CheckAndReturnResult(
        HRESULT hr,
        BOOL    fTrace,
        LPSTR   pstrFile,
        UINT    line,
        int     cHResult,
        ...)
{
    BOOL    fOLEError;
    BOOL    fOLEDBError;
    BOOL    fOKReturnCode;
    va_list va;
    int     i;
    HRESULT hrArg;

    Verify(!IsTagEnabled(tagValidate) || ValidateInternalHeap());

    //
    //  These xxx_E_LAST hresults are the highest-valued ones in FACILITY_ITF
    //  currently (at the time of coding).  These asserts should alert us if
    //  the numbers change, and our usage of CONNECT_E_LAST is no longer valid.
    //
    Assert(HRESULT_CODE(CONNECT_E_LAST) == HRESULT_CODE(SELFREG_E_LAST));
    Assert(HRESULT_CODE(CONNECT_E_LAST) == HRESULT_CODE(PERPROP_E_LAST));

    //
    // Check if code is a permitted error or success.
    //

    fOLEError = (hr < 0 &&
                 (HRESULT_FACILITY(hr) != FACILITY_ITF ||
                  HRESULT_CODE(hr) < HRESULT_CODE(CONNECT_E_LAST)));

    // Codes 0x0e00-0x0eff are reserved for the OLE DB group of
    // interfaces.  (So sayeth <oledberr.h>)
    fOLEDBError = HRESULT_FACILITY(hr) == FACILITY_ITF &&
                    0x0E00 <= HRESULT_CODE(hr) && HRESULT_CODE(hr) <= 0x0EFF;
    
    fOKReturnCode = ((cHResult == -1) || fOLEError || fOLEDBError || (hr == S_OK));

    if (cHResult > 0)
    {
        va_start(va, cHResult);
        for (i = 0; i < cHResult; i++)
        {
            hrArg = va_arg(va, HRESULT);
            if (hr == hrArg)
            {
                fOKReturnCode = TRUE;

                if (fOLEError)
                    fTrace = FALSE;

                va_end(va);
                break;
            }
        }

        va_end(va);
    }

    //
    // Assert on non-permitted success code.
    //

    if (!fOKReturnCode)
    {
        TraceTag((
                tagError,
                "%s:%d returned unpermitted HRESULT %hr",
                pstrFile,
                line,
                hr));
        AssertLocSz(hr <= 0, pstrFile, line,
                    "An unpermitted success code was returned.");
        AssertLocSz(!(HRESULT_FACILITY(hr) == FACILITY_ITF && HRESULT_CODE(hr) >= 0x0200), pstrFile, line,
                    "An unpermitted FACILITY_ITF HRESULT was returned.");
    }

    //
    // Warn on error result.
    //

    if (fTrace && FAILED(hr))
    {
        TraceTagEx((
                tagRRETURN,
                TAG_NONAME,
                "RRETURN: %s:%d returned %hr",
                pstrFile,
                line,
                hr));
    }

    return hr;
}


