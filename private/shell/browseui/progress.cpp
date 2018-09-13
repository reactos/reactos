/*****************************************************************************\
    FILE: progres.cpp

    DESCRIPTION:
        Display the Progress Dialog for the progress on the completion of some
    generic operation.  This is most often used for Deleting, Uploading, Copying,
    Moving and Downloading large numbers of files.
\*****************************************************************************/

#include "priv.h"
#include "resource.h"
#include "progress.h"

#include "mluisupp.h"

//#define TF_PROGRESS 0xFFFFFFFF
#define TF_PROGRESS 0x00000000

// REVIEW, we should tune this size down as small as we can
// to get smoother multitasking (without effecting performance)
#define MIN_MINTIME4FEEDBACK    5       // is it worth showing estimated time to completion feedback?
#define MS_TIMESLICE            2000    // ms, (MUST be > 1000!) first average time to completion estimate

#define SHOW_PROGRESS_TIMEOUT   1000    // 1 second
#define MINSHOWTIME             2000    // 2 seconds

// progress dialog message
#define PDM_SHUTDOWN     WM_APP
#define PDM_NOOP        (WM_APP + 1)
#define PDM_UPDATE      (WM_APP + 2)

// progress dialog timer messages
#define ID_SHOWTIMER    1


#ifndef UNICODE
#error "This code will only compile UNICODE for perf reasons.  If you really need an ANSI browseui, write all the code to convert."
#endif // !UNICODE



//===========================
// Private Utility Functions
#define SZ_EMPTYW       L""

/****************************************************\
    FUNCTION: CompactProgressPath

    DESCRIPTION: compacts path strings to fit into
                 the Text1 and Text2 fields
\****************************************************/
HRESULT CProgressDialog::_CompactProgressPath(LPCWSTR pwzStrIn, BOOL fCompactPath, UINT idDlgItem, LPCVOID pvResevered, LPWSTR pwzStrOut, DWORD cchSize)
{
    WCHAR wzFinalPath[MAX_PATH];
    LPWSTR pwzPathToUse = (LPWSTR)pwzStrIn;

    // We don't compact the path if the dialog isn't displayed yet.
    if (fCompactPath && _hwndProgress)
    {
        RECT rc;
        int cxWidth;
        HWND hwnd;

        StrCpyNW(wzFinalPath, (pwzStrIn ? pwzStrIn : L""), ARRAYSIZE(wzFinalPath));

        // get the size of the text boxes
        hwnd = GetDlgItem(_hwndProgress, idDlgItem);
        if (EVAL(hwnd))
        {
            GetWindowRect(hwnd, &rc);
            cxWidth = rc.right - rc.left;

            ASSERT(cxWidth >= 0);
            PathCompactPathW(NULL, wzFinalPath, cxWidth);
        }
        pwzPathToUse = wzFinalPath;
    }

    StrCpyNW(pwzStrOut, (pwzPathToUse ? pwzPathToUse : SZ_EMPTYW), cchSize);
    return S_OK;
}


/****************************************************\
    FUNCTION: _SetLineHelper

    DESCRIPTION:
\****************************************************/
HRESULT CProgressDialog::_SetLineHelper(LPCWSTR pwzNew, LPWSTR * ppwzDest, UINT idDlgItem, BOOL fCompactPath, LPCVOID pvResevered)
{
    WCHAR wzFinalPath[MAX_PATH];

    if (EVAL(SUCCEEDED(_CompactProgressPath(pwzNew, fCompactPath, idDlgItem, pvResevered, wzFinalPath, ARRAYSIZE(wzFinalPath)))))
    {
        // Does the dialog exist?
        if (_hwndProgress)
           EVAL(SetDlgItemText(_hwndProgress, idDlgItem, wzFinalPath));
        else
            Str_SetPtrW(ppwzDest, wzFinalPath); // No, so cache the value for later.
    }

    return S_OK;
}


HRESULT CProgressDialog::_DisplayDialog(void)
{
    TraceMsg(TF_PROGRESS, "CProgressDialog::_DisplayDialog()");
    // Don't force ourselves into the foreground if a window we parented is already in the foreground:
    
    // This is part of the fix for NT bug 298163 (the confirm replace dialog was deactivated
    // by the progress dialog)
    HWND hwndCurrent = GetForegroundWindow();
    BOOL fChildIsForeground = FALSE;
    while (NULL != (hwndCurrent = GetParent(hwndCurrent)))
    {
        if (_hwndProgress == hwndCurrent)
        {
            fChildIsForeground = TRUE;
            break;
        }
    }
    
    if (fChildIsForeground)
    {
        ShowWindow(_hwndProgress, SW_SHOWNOACTIVATE);
    }
    else
    {
        ShowWindow(_hwndProgress, SW_SHOW);
        SetForegroundWindow(_hwndProgress);
    }
    
    SetFocus(GetDlgItem(_hwndProgress, IDCANCEL));
    return S_OK;
}


/****************************************************\
    FUNCTION: _ProgressUIThreadProc

    DESCRIPTION:
\****************************************************/
DWORD CProgressDialog::_ProgressUIThreadProc(void)
{
    _InitComCtl32();        // Get ready for the Native Font Control
    _hwndProgress = CreateDialogParam(MLGetHinst(), MAKEINTRESOURCE(DLG_PROGRESSDIALOG),
                                          _hwndDlgParent, ProgressDialogProc, (LPARAM)this);

    // Signal the main thread that we have created the hwnd
    if (_hCreatedHwnd)
        SetEvent(_hCreatedHwnd);

    SetThreadPriority(_hThread, THREAD_PRIORITY_BELOW_NORMAL);

    // we give up the remainder of our timeslice here so that our parent thread has time to run
    // and will notice that we have signaled the _hCreateHwnd event and can therefore return
    Sleep(0);

    if (_hwndProgress)
    {
        SetTimer(_hwndProgress, ID_SHOWTIMER, SHOW_PROGRESS_TIMEOUT, NULL);

        // Did if finish while we slept?
        if (!_fDone)
        {
            // No, so display the dialog.
            MSG msg;

            while(GetMessage(&msg, NULL, 0, 0))
            {
                if (_fDone && (GetTickCount() - _dwFirstShowTime) > MINSHOWTIME)
                {
                    // we were signaled to finish and we have been visible MINSHOWTIME,
                    // so its ok to quit
                    break;
                }

                if (!IsDialogMessage(_hwndProgress, &msg))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }

        DestroyWindow(_hwndProgress);
        _hwndProgress = NULL;
    }

    Release();
    return 0;
}


DWORD FormatMessageWrapW(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageID, DWORD dwLangID, LPWSTR pwzBuffer, DWORD cchSize, ...)
{
    va_list vaParamList;

    va_start(vaParamList, cchSize);
    DWORD dwResult = FormatMessageW(dwFlags, lpSource, dwMessageID, dwLangID, pwzBuffer, cchSize, &vaParamList);
    va_end(vaParamList);

    return dwResult;
}


DWORD FormatMessageWrapA(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageID, DWORD dwLangID, LPSTR pszBuffer, DWORD cchSize, ...)
{
    va_list vaParamList;

    va_start(vaParamList, cchSize);
    DWORD dwResult = FormatMessageA(dwFlags, lpSource, dwMessageID, dwLangID, pszBuffer, cchSize, &vaParamList);
    va_end(vaParamList);

    return dwResult;
}


#define TIME_DAYS_IN_YEAR               365
#define TIME_HOURS_IN_DAY               24
#define TIME_MINUTES_IN_HOUR            60
#define TIME_SECONDS_IN_MINUTE          60


void _FormatMessageWrapper(LPCTSTR pszTemplate, DWORD dwNum1, DWORD dwNum2, LPTSTR pszOut, DWORD cchSize)
{
    // Is FormatMessageWrapW implemented?
    if (g_bRunOnNT5)
    {
        EVAL(FormatMessageWrapW(FORMAT_MESSAGE_FROM_STRING, pszTemplate, 0, 0, pszOut, cchSize, dwNum1, dwNum2));
    }
    else
    {
        CHAR szOutAnsi[MAX_PATH];
        CHAR szTemplateAnsi[MAX_PATH];

        SHTCharToAnsi(pszTemplate, szTemplateAnsi, ARRAYSIZE(szTemplateAnsi));
        EVAL(FormatMessageWrapA(FORMAT_MESSAGE_FROM_STRING, szTemplateAnsi, 0, 0, szOutAnsi, ARRAYSIZE(szOutAnsi), dwNum1, dwNum2));
        SHAnsiToTChar(szOutAnsi, pszOut, cchSize);
    }
}


#define CCH_TIMET_TEMPLATE_SIZE         120     // Should be good enough, even with localization bloat.
#define CCH_TIME_SIZE                    170     // Should be good enough, even with localization bloat.

void _SetProgressLargeTimeEst(DWORD dwSecondsLeft, LPTSTR pszOut, DWORD cchSize)
{
    // Yes.
    TCHAR szTemplate[CCH_TIMET_TEMPLATE_SIZE];
    DWORD dwMinutes = (dwSecondsLeft / TIME_SECONDS_IN_MINUTE);
    DWORD dwHours = (dwMinutes / TIME_MINUTES_IN_HOUR);
    DWORD dwDays = (dwHours / TIME_HOURS_IN_DAY);

    if (dwDays)
    {
        dwHours %= TIME_HOURS_IN_DAY;

        // It's more than a day, so display days and hours.
        if (1 == dwDays)
        {
            if (1 == dwHours)
                EVAL(LoadString(MLGetHinst(), IDS_TIMEEST_DAYHOUR, szTemplate, ARRAYSIZE(szTemplate)));
            else
                EVAL(LoadString(MLGetHinst(), IDS_TIMEEST_DAYHOURS, szTemplate, ARRAYSIZE(szTemplate)));
        }
        else
        {
            if (1 == dwHours)
                EVAL(LoadString(MLGetHinst(), IDS_TIMEEST_DAYSHOUR, szTemplate, ARRAYSIZE(szTemplate)));
            else
                EVAL(LoadString(MLGetHinst(), IDS_TIMEEST_DAYSHOURS, szTemplate, ARRAYSIZE(szTemplate)));
        }

        _FormatMessageWrapper(szTemplate, dwDays, dwHours, pszOut, cchSize);
    }
    else
    {
        // It's let than a day, so display hours and minutes.
        dwMinutes %= TIME_MINUTES_IN_HOUR;

        // It's more than a day, so display days and hours.
        if (1 == dwHours)
        {
            if (1 == dwMinutes)
                EVAL(LoadString(MLGetHinst(), IDS_TIMEEST_HOURMIN, szTemplate, ARRAYSIZE(szTemplate)));
            else
                EVAL(LoadString(MLGetHinst(), IDS_TIMEEST_HOURMINS, szTemplate, ARRAYSIZE(szTemplate)));
        }
        else
        {
            if (1 == dwMinutes)
                EVAL(LoadString(MLGetHinst(), IDS_TIMEEST_HOURSMIN, szTemplate, ARRAYSIZE(szTemplate)));
            else
                EVAL(LoadString(MLGetHinst(), IDS_TIMEEST_HOURSMINS, szTemplate, ARRAYSIZE(szTemplate)));
        }

        _FormatMessageWrapper(szTemplate, dwHours, dwMinutes, pszOut, cchSize);
    }
}


// This sets the "Seconds Left" text in the progress dialog
void CProgressDialog::_SetProgressTimeEst(DWORD dwSecondsLeft)
{
    TCHAR szFmt[CCH_TIMET_TEMPLATE_SIZE];
    TCHAR szOut[CCH_TIME_SIZE];
    DWORD dwTime;
    DWORD dwTickCount = GetTickCount();


    // Since the progress time has either a 1 minute or 5 second granularity (depending on whether the total time
    // remaining is greater or less than 1 minute), we only update it every 20 seconds if the total time is > 1 minute,
    // and ever 4 seconds if the time is < 1 minute. This keeps the time from flashing back and forth between 
    // boundaries (eg 45 secondsand 40 seconds remaining).
    if (dwTickCount < _dwLastUpdatedTimeRemaining + ((dwSecondsLeft > 60) ? 20000 : 4000))
        return;

    if (_dwFlags & PROGDLG_NOTIME)
    {
        szOut[0] = TEXT('\0');
    }
    else
    {
        // Is it more than an hour?
        if (dwSecondsLeft > (TIME_SECONDS_IN_MINUTE * TIME_MINUTES_IN_HOUR))
            _SetProgressLargeTimeEst(dwSecondsLeft, szOut, ARRAYSIZE(szOut));
        else
        {
            // No.
            if (dwSecondsLeft > TIME_SECONDS_IN_MINUTE)
            {
                // Note that dwTime is at least 2, so we only need a plural form
                LoadString(MLGetHinst(), IDS_TIMEEST_MINUTES, szFmt, ARRAYSIZE(szFmt));
                dwTime = (dwSecondsLeft / TIME_SECONDS_IN_MINUTE) + 1;
            }
            else
            {
                LoadString(MLGetHinst(), IDS_TIMEEST_SECONDS, szFmt, ARRAYSIZE(szFmt));
                // Round up to 5 seconds so it doesn't look so random
                dwTime = ((dwSecondsLeft + 4) / 5) * 5;
            }

            wnsprintf(szOut, ARRAYSIZE(szOut), szFmt, dwTime);
        }
    }

    // we are updating now, so set the _dwLastUpdatedTimeRemaining to now
    _dwLastUpdatedTimeRemaining = dwTickCount;

    // update the Time remaining field
    SetDlgItemText(_hwndProgress, IDD_PROGDLG_LINE3, szOut);
}

#define MAX(x, y)    ((x) > (y) ? (x) : (y))
//
// This function updates the ProgressTime field (aka Line3)
//
HRESULT CProgressDialog::_SetProgressTime(void)
{
    DWORD dwSecondsLeft;
    DWORD dwTotal;
    DWORD dwCompleted;
    DWORD dwCurrentRate;
    DWORD dwTickDelta;
    DWORD dwLeft;
    DWORD dwCurrentTickCount;

    _iNumTimesSetProgressCalled++;

    // grab these in the crit sec (because they can change, and we need a matched set)
    ENTERCRITICAL;
    dwTotal = _dwTotal;
    dwCompleted = _dwCompleted;
    dwCurrentTickCount = _dwLastUpdatedTickCount;
    LEAVECRITICAL;

    dwLeft = dwTotal - dwCompleted;

    dwTickDelta = dwCurrentTickCount - _dwPrevTickCount;

    if (!dwTotal || !dwCompleted)
        return dwTotal ? S_FALSE : E_FAIL;

    // we divide the TickDelta by 100 to give tenths of seconds, so if we have recieved an
    // update faster than that, just skip it
    if (dwTickDelta < 100)
    {
        return S_FALSE;
    }
    
    TraceMsg(TF_PROGRESS, "Current tick count = %lu", dwCurrentTickCount);
    TraceMsg(TF_PROGRESS, "Total work     = %lu", dwTotal);
    TraceMsg(TF_PROGRESS, "Completed work = %lu", dwCompleted);
    TraceMsg(TF_PROGRESS, "Prev. comp work= %lu", _dwPrevCompleted);
    TraceMsg(TF_PROGRESS, "Work left      = %lu", dwLeft);
    TraceMsg(TF_PROGRESS, "Tick delta         = %lu", dwTickDelta);

    if (dwTotal < dwCompleted)
    {
        // we can get into this case if we are applying attributes to sparse files
        // on a volume. As we add up the file sizes, we end up with a number that is bigger
        // than the drive size. We get rid of the time so that we wont show the user something
        // completely bogus
        _dwFlags |= PROGDLG_NOTIME;
        dwTotal = dwCompleted + (dwCompleted >> 3);  // fudge dwTotal forward a bit
        TraceMsg(TF_PROGRESS, "!! (Total < Completed), fudging Total work to = %lu", dwTotal);
    }

    if(dwCompleted <= _dwPrevCompleted)
    {
        // woah, we are going backwards, we dont deal w/ negative or zero rates so...
        dwCurrentRate = (_dwPrevRate ? _dwPrevRate : 2);
    }
    else
    {
        // calculate the current rate in points per tenth of a second
        dwTickDelta /= 100;
        if (0 == dwTickDelta)
            dwTickDelta = 1; // Protect from divide by zero

        dwCurrentRate = (dwCompleted - _dwPrevCompleted) / dwTickDelta;
    }

    TraceMsg(TF_PROGRESS, "Current rate = %lu", dwCurrentRate);
    TraceMsg(TF_PROGRESS, "Prev.   rate = %lu", _dwPrevRate);

    // time remaining in seconds (we take a REAL average to smooth out random fluxuations)
    DWORD dwAverageRate = (DWORD)((dwCurrentRate + (_int64)_dwPrevRate * _iNumTimesSetProgressCalled) / (_iNumTimesSetProgressCalled + 1));
    TraceMsg(TF_PROGRESS, "Average rate= %lu", dwAverageRate);

    dwAverageRate = MAX(dwAverageRate, 1); // Protect from divide by zero

    dwSecondsLeft = (dwLeft / dwAverageRate) / 10;
    TraceMsg(TF_PROGRESS, "Seconds left = %lu", dwSecondsLeft);
    TraceMsg(TF_PROGRESS, "");

    // It would be silly to show "1 second left" and then immediately clear it, and to avoid showing 
    // rediculous early estimates, we dont show anything until we have at least 5 data points
    if ((dwSecondsLeft >= MIN_MINTIME4FEEDBACK) && (_iNumTimesSetProgressCalled >= 5))
    {
        // display new estimate of time left
        _SetProgressTimeEst(dwSecondsLeft);
    }

    // set all the _dwPrev stuff for next time
    _dwPrevRate = dwAverageRate;
    _dwPrevTickCount = dwCurrentTickCount;
    _dwPrevCompleted = dwCompleted;

    return S_OK;
}




/****************************************************\
    FUNCTION: ProgressDialogProc

    DESCRIPTION:
\****************************************************/
INT_PTR CALLBACK CProgressDialog::ProgressDialogProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    CProgressDialog * ppd = (CProgressDialog *)GetWindowLongPtr(hDlg, DWLP_USER);

    if (WM_INITDIALOG == wMsg)
    {
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        ppd = (CProgressDialog *)lParam;
    }

    if (ppd)
        return ppd->_ProgressDialogProc(hDlg, wMsg, wParam, lParam);

    return DefWindowProc(hDlg, wMsg, wParam, lParam);
}


BOOL CProgressDialog::_OnInit(HWND hDlg)
{
    if (PROGDLG_NOMINIMIZE & _dwFlags)
    {
        // The caller wants us to remove the Minimize Box or button in the caption bar.
        SHSetWindowBits(hDlg, GWL_EXSTYLE, WS_MINIMIZEBOX, 0);
    }

    if (PROGDLG_NOPROGRESSBAR & _dwFlags)
    {
        SetWindowPos(GetDlgItem(hDlg, IDD_PROGDLG_PROGRESSBAR), NULL, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    return FALSE;
}


/****************************************************\
    FUNCTION: _ProgressDialogProc

    DESCRIPTION:
\****************************************************/
BOOL CProgressDialog::_ProgressDialogProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fHandled = TRUE;   // handled

    switch (wMsg)
    {
    case WM_INITDIALOG:
        return _OnInit(hDlg);

    case WM_SHOWWINDOW:
        if (wParam)
        {
            ASSERT(_hwndProgress);
            EVAL(SUCCEEDED(SetAnimation(_hInstAnimation, _idAnimation)));

            // set the initial text values
            if (_pwzTitle)  EVAL(SUCCEEDED(SetTitle(_pwzTitle)));
            if (_pwzLine1)  EVAL(SUCCEEDED(SetLine(1, _pwzLine1, FALSE, NULL)));
            if (_pwzLine2)  EVAL(SUCCEEDED(SetLine(2, _pwzLine2, FALSE, NULL)));
            if (_pwzLine3)  EVAL(SUCCEEDED(SetLine(3, _pwzLine3, FALSE, NULL)));
        }
        break;

    case WM_DESTROY:
        // TODO: We may want to save state so _pwzLineX and _pwzTitle persist across
        //       StartProgressDialog(), StopProgressDialog(), StartProgressDialog() calls.
        break;

    case WM_ENABLE:
        if (wParam)
        {
            // we assume that we were previously disabled and thus restart our tick counter
            // because we also naively assume that no work was being done while we were disabled
            _dwPrevTickCount = GetTickCount();
        }

        _PauseAnimation(wParam == 0);
        break;

    case WM_TIMER:
        if (wParam == ID_SHOWTIMER)
        {
            KillTimer(hDlg, ID_SHOWTIMER);

            _DisplayDialog();
 
            _dwFirstShowTime = GetTickCount();
        }
        break;

    case WM_COMMAND:
        if (IDCANCEL == GET_WM_COMMAND_ID(wParam, lParam))
            _UserCancelled();
        break;

    case PDM_SHUTDOWN:
        // Make sure this window is shown before telling the user there
        // is a problem.  Ignore FOF_NOERRORUI here because of the 
        // nature of the situation
        MLShellMessageBox(hDlg, MAKEINTRESOURCE(IDS_CANTSHUTDOWN), NULL, (MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND));
        break;


    case PDM_NOOP:
        // a dummy id that we can take so that folks can post to us and make
        // us go through the main loop
        break;

    case WM_SYSCOMMAND:
        switch(wParam)
        {
        case SC_MINIMIZE:
            _fMinimized = TRUE;
            break;
        case SC_RESTORE:
            SetTitle(_pwzTitle);    // Restore title to original text.
            _fMinimized = FALSE;
            break;
        }
        fHandled = FALSE;
        break;

    case PDM_UPDATE:
        if (!_fCancel && IsWindowEnabled(hDlg))
        {
            _SetProgressTime();
            _UpdateProgressDialog();
        }
        // we are done processing the update
        _fChangePosted = FALSE;
        break;

    case WM_QUERYENDSESSION:
        // Post a message telling the dialog to show the "We can't shutdown now"
        // dialog and return to USER right away, so we don't have to worry about
        // the user not clicking the OK button before USER puts up its "this
        // app didn't respond" dialog
        PostMessage(hDlg, PDM_SHUTDOWN, 0, 0);

        // Make sure the dialog box procedure returns FALSE
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
        return(TRUE);

    default:
        fHandled = FALSE;   // Not handled
    }

    return fHandled;
}


// This is used to asyncronously update the progess dialog.
void CProgressDialog::_AsyncUpdate(void)
{
    if (!_fChangePosted && _hwndProgress)   // Prevent from posting too many messages.
    {
        // set the flag first because with async threads
        // the progress window could handle it and clear the
        // bit before we set it.. then we'd lose further messages
        // thinking that one was still pending
        _fChangePosted = TRUE;
        if (!PostMessage(_hwndProgress, PDM_UPDATE, 0, 0))
        {
            _fChangePosted = FALSE;
        }
    }
}


/****************************************************\
    FUNCTION: _UpdateProgressDialog

    DESCRIPTION:
\****************************************************/
void CProgressDialog::_UpdateProgressDialog(void)
{
    if (_fTotalChanged)
    {
        _fTotalChanged = FALSE;
        if (0x80000000 & _dwTotal)
            _fScaleBug = TRUE;
            
        SendMessage(GetDlgItem(_hwndProgress, IDD_PROGDLG_PROGRESSBAR), PBM_SETRANGE32, 0, (_fScaleBug ? (_dwTotal >> 1) : _dwTotal));
    }

    if (_fCompletedChanged)
    {
        _fCompletedChanged = FALSE;
        SendMessage(GetDlgItem(_hwndProgress, IDD_PROGDLG_PROGRESSBAR), PBM_SETPOS, (WPARAM) (_fScaleBug ? (_dwCompleted >> 1) : _dwCompleted), 0);
    }
}


/****************************************************\
    FUNCTION: _PauseAnimation

    DESCRIPTION:
\****************************************************/
void CProgressDialog::_PauseAnimation(BOOL bStop)
{
    // only called from within the hwndProgress wndproc so assum it's there
    if (bStop)
    {
        Animate_Stop(GetDlgItem(_hwndProgress, IDD_PROGDLG_ANIMATION));
    }
    else
    {
        Animate_Play(GetDlgItem(_hwndProgress, IDD_PROGDLG_ANIMATION), -1, -1, -1);
    }
}


/****************************************************\
    FUNCTION: _UserCancelled

    DESCRIPTION:
\****************************************************/
void CProgressDialog::_UserCancelled(void)
{
    // Don't hide the dialog because the caller may not pole
    // ::HasUserCancelled() for quite a while.
    // ShowWindow(hDlg, SW_HIDE);
    _fCancel = TRUE;

    _dwFlags &= ~PROGDLG_AUTOTIME;  // Clear this bit.
    // If the user cancels, Line 1 & 2 will be cleared and Line 3 will get this msg.
    if (!_pwzCancelMsg)
    {
        WCHAR wzDefaultMsg[MAX_PATH];

        EVAL(LoadStringW(MLGetHinst(), IDS_DEFAULT_CANCELPROG, wzDefaultMsg, ARRAYSIZE(wzDefaultMsg)));
        Str_SetPtr(&_pwzCancelMsg, wzDefaultMsg);
    }

    EVAL(SUCCEEDED(SetLine(1, L"", FALSE, NULL)));
    EVAL(SUCCEEDED(SetLine(2, L"", FALSE, NULL)));
    EVAL(SUCCEEDED(SetLine(3, _pwzCancelMsg, FALSE, NULL)));
}




//===========================
// *** IProgressDialog Interface ***

/****************************************************\
    FUNCTION: StartProgressDialog

    DESCRIPTION:
\****************************************************/
HRESULT CProgressDialog::StartProgressDialog(HWND hwndParent, IUnknown * punkEnableModless, DWORD dwFlags, LPCVOID pvResevered)
{
    HRESULT hr = S_OK;
    DWORD idThread;

    if (_fWorking)
        return S_OK;

    // TODO: Save punkEnableModless and use it when we display the UI.
    _hwndDlgParent = hwndParent;
    _dwFlags = dwFlags;

    // if the user is requesting a modal window, disable the parent now.
    if (_hwndDlgParent && (_dwFlags & PROGDLG_MODAL))
        EnableWindow(_hwndDlgParent, FALSE);

    _fDone = FALSE;
    _fTotalChanged = TRUE;

    // Always create a progress dialog
    // Note that it will be created invisible, and will be shown if the
    // operation takes longer than a second to perform
    // Note the parent of this window is NULL so it will get the QUERYENDSESSION
    // message
    _hThread = CreateThread(NULL, 0, CProgressDialog::ProgressUIThreadProc, this, 0, &idThread);
    if (_hThread)
    {
        AddRef();

        // We wait WAIT_PROGRESS_HWND for the new thread to create the hwnd (callers depend on
        // having the _hwndProgress existing after this call completes)
        if (_hCreatedHwnd)
        {
            DWORD dwRet = WaitForSingleObject(_hCreatedHwnd, WAIT_PROGRESS_HWND);
            ASSERT(dwRet != WAIT_TIMEOUT);
        }
    }

    _fWorking = TRUE;

    // initialize the _dwPrev counters
    _dwPrevRate = 0;
    _dwPrevCompleted = 0;
    _dwPrevTickCount = GetTickCount();

    TraceMsg(TF_PROGRESS, "Initial tick count = %lu", _dwPrevTickCount);
    return hr;
}


/****************************************************\
    FUNCTION: StopProgressDialog

    DESCRIPTION:
\****************************************************/
HRESULT CProgressDialog::StopProgressDialog(void)
{
    HRESULT hr = S_OK;

    // shut down the progress dialog
    if (_hThread)
    {
        DWORD dwWaitResult;

        _fDone = TRUE;

        // Since we are shutting the thread down, crank up the priority
        // so it can have time to get through the message pump and exit
        SetThreadPriority(_hThread, THREAD_PRIORITY_ABOVE_NORMAL);

        if (_hwndProgress)
            PostMessage(_hwndProgress, PDM_NOOP, 0, 0);

        dwWaitResult = SHWaitForSendMessageThread(_hThread, 3000);

        // Did the thread kill it self when I just asked it to?
        if ((dwWaitResult == WAIT_TIMEOUT) || (dwWaitResult == WAIT_FAILED))
        {
            // No, so I will need to kill it...
            TerminateThread(_hThread, (DWORD)-1);
            if (IsWindow(_hwndProgress))
                DestroyWindow(_hwndProgress);
        }
        _fWorking = FALSE;
        CloseHandle(_hThread);
        _hThread = NULL;
    }

    if (_hwndDlgParent)
    {
        EnableWindow(_hwndDlgParent, TRUE);
        SetForegroundWindow(_hwndDlgParent);
    }

    return hr;
}

/****************************************************\
    FUNCTION: SetTitle

    DESCRIPTION:
\****************************************************/
HRESULT CProgressDialog::SetTitle(LPCWSTR pwzTitle)
{
    HRESULT hr = S_OK;

    // Does the dialog exist?
    if (_hwndProgress)
    {
        // Yes, so put the value directly into the dialog.
        ASSERT(_hwndProgress);
        if (!EVAL(SetWindowTextW(_hwndProgress, (pwzTitle ? pwzTitle : L""))))
            hr = E_FAIL;
    }
    else
        Str_SetPtrW(&_pwzTitle, pwzTitle);

    return hr;
}

/****************************************************\
    FUNCTION: SetAnimation

    DESCRIPTION:
\****************************************************/
HRESULT CProgressDialog::SetAnimation(HINSTANCE hInstAnimation, UINT idAnimation)
{
    HRESULT hr = S_OK;

    // Does the dialog exist?
    if (_hwndProgress)
    {
        // Yes, so put the value directly into the dialog.
        ASSERT(_hwndProgress);
        if (!EVAL(Animate_OpenEx(GetDlgItem(_hwndProgress, IDD_PROGDLG_ANIMATION), _hInstAnimation, _idAnimation)))
            hr = E_FAIL;
    }
    else
    {
        _hInstAnimation = hInstAnimation;
        _idAnimation = idAnimation;
    }

    return hr;
}


/****************************************************\
    FUNCTION: SetLine

    DESCRIPTION:
\****************************************************/
HRESULT CProgressDialog::SetLine(DWORD dwLineNum, LPCWSTR pwzString, BOOL fCompactPath, LPCVOID pvResevered)
{
    HRESULT hr = E_INVALIDARG;

    switch (dwLineNum)
    {
    case 1:
        hr = _SetLineHelper(pwzString, &_pwzLine1, IDD_PROGDLG_LINE1, fCompactPath, pvResevered);
        break;
    case 2:
        hr = _SetLineHelper(pwzString, &_pwzLine2, IDD_PROGDLG_LINE2, fCompactPath, pvResevered);
        break;
    case 3:
        if (_dwFlags & PROGDLG_AUTOTIME)
        {
            // you cant change line3 directly if you want PROGDLG_AUTOTIME, because
            // this is updated by the progress dialog automatically
            ASSERT(FALSE);
            hr = E_INVALIDARG;
            break;
        }
        hr = _SetLineHelper(pwzString, &_pwzLine3, IDD_PROGDLG_LINE3, fCompactPath, pvResevered);
        break;

    default:
        ASSERT(0);
    }

    return hr;
}


/****************************************************\
    FUNCTION: SetCancelMsg

    DESCRIPTION:
\****************************************************/
HRESULT CProgressDialog::SetCancelMsg(LPCWSTR pwzCancelMsg, LPCVOID pvResevered)
{
    Str_SetPtr(&_pwzCancelMsg, pwzCancelMsg);              // If the user cancels, Line 1 & 2 will be cleared and Line 3 will get this msg.
    return S_OK;
}


/****************************************************\
    FUNCTION: Timer

    DESCRIPTION:
\****************************************************/
HRESULT CProgressDialog::Timer(DWORD dwAction, LPCVOID pvResevered)
{
    HRESULT hr = E_NOTIMPL;

    switch (dwAction)
    {
    case PDTIMER_RESET:
        _dwPrevTickCount = GetTickCount();
        hr = S_OK;
        break;
    }

    return hr;
}


/****************************************************\
    FUNCTION: SetProgress

    DESCRIPTION:
\****************************************************/
HRESULT CProgressDialog::SetProgress(DWORD dwCompleted, DWORD dwTotal)
{
    DWORD dwTickCount = GetTickCount(); // get the tick count before taking the critical section

    // we grab the crit section in case the UI thread is trying to access
    // _dwCompleted, _dwTotal or _dwLastUpdatedTickCount to do its time update.
    ENTERCRITICAL;
    if (_dwCompleted != dwCompleted)
    {
        _dwCompleted = dwCompleted;
        _fCompletedChanged = TRUE;
    }

    if (_dwTotal != dwTotal)
    {
        _dwTotal = dwTotal;
        _fTotalChanged = TRUE;
    }
 
    if (_fCompletedChanged || _fTotalChanged)
        _dwLastUpdatedTickCount = dwTickCount;

    LEAVECRITICAL;

    ASSERT(_dwTotal >= _dwCompleted);

    if (_fCompletedChanged || _fTotalChanged)
    {
        // something changed, so update the progress dlg
        _AsyncUpdate();
    }

    TraceMsg(TF_PROGRESS, "CProgressDialog::SetProgress(Complete=%#08lx, Total=%#08lx)", dwCompleted, dwTotal);
    if (_fMinimized)
        _SetTitleBarProgress(dwCompleted, dwTotal);

    return S_OK;
}


/****************************************************\
    FUNCTION: SetProgress

    DESCRIPTION:
\****************************************************/
HRESULT CProgressDialog::SetProgress64(ULONGLONG ullCompleted, ULONGLONG ullTotal)
{
    ULARGE_INTEGER uliCompleted, uliTotal;
    uliCompleted.QuadPart = ullCompleted;
    uliTotal.QuadPart = ullTotal;

    // If we are using the top 32 bits, scale both numbers down.
    // Note that I'm using the attribute that dwTotalHi is always
    // larger than dwCompletedHi
    ASSERT(uliTotal.HighPart >= uliCompleted.HighPart);
    while (uliTotal.HighPart)
    {
        uliCompleted.QuadPart >>= 1;
        uliTotal.QuadPart >>= 1;
    }

    ASSERT((0 == uliCompleted.HighPart) && (0 == uliTotal.HighPart));       // Make sure we finished scaling down.
    return SetProgress(uliCompleted.LowPart, uliTotal.LowPart);
}


/****************************************************\
    FUNCTION: SetProgress

    DESCRIPTION:
\****************************************************/
HRESULT CProgressDialog::_SetTitleBarProgress(DWORD dwCompleted, DWORD dwTotal)
{
    TCHAR szTemplate[MAX_PATH];
    TCHAR szTitle[MAX_PATH];
    int nPercent = 0;

    if (dwTotal)    // Disallow divide by zero.
    {
        // Will scaling it up cause a wrap?
        if ((100 * 100) <= dwTotal)
        {
            // Yes, so scale down.
            nPercent = (dwCompleted / (dwTotal / 100));
        }
        else
        {
            // No, so scale up.
            nPercent = ((100 * dwCompleted) / dwTotal);
        }
    }

    EVAL(LoadString(MLGetHinst(), IDS_TITLEBAR_PROGRESS, szTemplate, ARRAYSIZE(szTemplate)));
    wnsprintf(szTitle, ARRAYSIZE(szTitle), szTemplate, nPercent);
    EVAL(SetWindowText(_hwndProgress, szTitle));

    return S_OK;
}


/****************************************************\
    FUNCTION: HasUserCancelled

    DESCRIPTION:
    This queries the progress dialog for a cancel and yields.
    it also will show the progress dialog if a certain amount of time has passed
    
     returns:
        TRUE      cacnel was pressed, abort the operation
        FALSE     continue
\****************************************************/
BOOL CProgressDialog::HasUserCancelled(void)
{
    if (!_fCancel && _hwndProgress)
    {
        MSG msg;

        // win95 handled messages in here.
        // we need to do the same in order to flush the input queue as well as
        // for backwards compatability.

        // we need to flush the input queue now because hwndProgress is
        // on a different thread... which means it has attached thread inputs
        // inorder to unlock the attached threads, we need to remove some
        // sort of message until there's none left... any type of message..
        while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (!IsDialogMessage(_hwndProgress, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        if (_fTotalChanged || _fCompletedChanged)
            _AsyncUpdate();
    }

    return _fCancel;
}


//===========================
// *** IOleWindow Interface ***
HRESULT CProgressDialog::GetWindow(HWND * phwnd)
{
    HRESULT hr = E_FAIL;

    *phwnd = _hwndProgress;
    if (_hwndProgress)
        hr = S_OK;

    return hr;
}


//===========================
// *** IUnknown Interface ***
HRESULT CProgressDialog::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CProgressDialog, IProgressDialog),
        QITABENT(CProgressDialog, IOleWindow),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


ULONG CProgressDialog::AddRef(void)
{
    _cRef++;
    return _cRef;
}

ULONG CProgressDialog::Release(void)
{
    ASSERT(_cRef > 0);

    _cRef--;
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

/****************************************************\
    Constructor
\****************************************************/
CProgressDialog::CProgressDialog() : _cRef(1)
{
    DllAddRef();

    // ASSERT zero initialized because we can only be created in the heap. (Private destructor)
    ASSERT(!_pwzLine1);
    ASSERT(!_pwzLine2);
    ASSERT(!_pwzLine3);
    ASSERT(!_fCancel);
    ASSERT(!_fDone);
    ASSERT(!_fWorking);
    ASSERT(!_hThread);
    ASSERT(!_hwndProgress);
    ASSERT(!_hwndDlgParent);
    ASSERT(!_fChangePosted);
    ASSERT(!_dwLastUpdatedTimeRemaining);
    ASSERT(!_dwCompleted);
    ASSERT(!_fCompletedChanged);
    ASSERT(!_fTotalChanged);
    ASSERT(!_fMinimized);

    _dwTotal = 1;     // Init to Completed=0, Total=1 so we are at 0%.

    // We use this event to signal the primary thread that the hwnd was created on the UI thread.
    _hCreatedHwnd = CreateEvent(NULL, FALSE, FALSE, NULL); 
}


/****************************************************\
    Destructor
\****************************************************/
CProgressDialog::~CProgressDialog()
{
    Str_SetPtrW(&_pwzTitle, NULL);
    Str_SetPtrW(&_pwzLine1, NULL);
    Str_SetPtrW(&_pwzLine2, NULL);
    Str_SetPtrW(&_pwzLine3, NULL);

    if (_hCreatedHwnd)
        CloseHandle(_hCreatedHwnd);

    ASSERT(!_fWorking);
    DllRelease();
}




STDAPI CProgressDialog_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory
    *ppunk = NULL;
    CProgressDialog * pProgDialog = new CProgressDialog();
    if (pProgDialog) 
    {
        *ppunk = SAFECAST(pProgDialog, IProgressDialog *);
        return NOERROR;
    }

    return E_OUTOFMEMORY;
}
