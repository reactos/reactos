#include "priv.h"
#include "inetnot.h"

//+-------------------------------------------------------------------------
// Static initialization
//--------------------------------------------------------------------------
HWND  CWinInetNotify::s_hwnd = NULL;
ULONG CWinInetNotify::s_ulEnabled = 0;

//+-------------------------------------------------------------------------
// Constructor - Creates invisible top-level window.
//--------------------------------------------------------------------------
CWinInetNotify::CWinInetNotify()
:   _hMutex(NULL),
    _fEnabled(FALSE)
{
}

//+-------------------------------------------------------------------------
// Enables/disables wininet notifications
//--------------------------------------------------------------------------
void CWinInetNotify::Enable(BOOL fEnable)
{
    if (fEnable && !_fEnabled)
    {
        //
        // Enable the notifications
        //
        ENTERCRITICAL;
        ++s_ulEnabled;
        if (NULL == s_hwnd)
        {
            // create an invisible top-level window to receive notifications
            WNDCLASS  wc;
            ZeroMemory(&wc, SIZEOF(wc));

            wc.lpfnWndProc      = _WndProc;
            wc.hInstance        = HINST_THISDLL;
            wc.lpszClassName    = CWinInetNotify_szWindowClass;

            SHRegisterClass(&wc);

            s_hwnd = CreateWindow(CWinInetNotify_szWindowClass, NULL, WS_POPUP,
                        0, 0, 1, 1, NULL, NULL, HINST_THISDLL, this);
        }

        if (s_hwnd)
        {
            _fEnabled = TRUE;
        }

        LEAVECRITICAL;
    }
    else if (!fEnable && _fEnabled)
    {
        //
        // Disable the notifications
        //
        ENTERCRITICAL;
        if (--s_ulEnabled == 0)
        {
            //
            // We use a mutex here because we can have multiple instances of
            // iexplore.  We want to avoid setting up a window to accept wininet 
            // notifications if it is in the process of being destroyed.
            //
            _EnterMutex();

            // Look for another window to receive wininet notifications
            if (EnumWindows(EnumWindowsProc, NULL))
            {
                // No one left so turn off notifications
                RegisterUrlCacheNotification(0, 0, 0, 0, 0);
            }

            //
            // Handle any queued notifications.
            //
            // Note that we have a small window in which a notification
            // can be lost!  Something could be posted to us after we are
            // destroyed!
            //
            MSG msg;
            if (PeekMessage(&msg, s_hwnd, CWM_WININETNOTIFY, CWM_WININETNOTIFY, PM_REMOVE))
            {
                _OnNotify(msg.wParam);
            }

            DestroyWindow(s_hwnd);
            s_hwnd = NULL;

            // Now that our window is gone, we can allow other processes to
            // look for windows to receive notifications.
            _LeaveMutex();
        }
        LEAVECRITICAL;

        _fEnabled = FALSE;
    }
}

//+-------------------------------------------------------------------------
// Destructor - Destroys top-level window when last instance is destroyed
//--------------------------------------------------------------------------
CWinInetNotify::~CWinInetNotify()
{
    Enable(FALSE);
}

//+-------------------------------------------------------------------------
// Called for each top level window to find another one to accept wininet
// notifications.
//--------------------------------------------------------------------------
BOOL CALLBACK CWinInetNotify::EnumWindowsProc
(
    HWND hwnd,      // handle to top-level window
    LPARAM lParam   // application-defined value 
 
)
{
    // Ignore our own window
    if (hwnd == s_hwnd)
        return TRUE;

    // See if it's one of our windows
    TCHAR szWindowClass[30];
    if (GetClassName(hwnd, szWindowClass, ARRAYSIZE(szWindowClass)) &&
        StrCmp(CWinInetNotify_szWindowClass, szWindowClass) == 0)
    {
        _HookInetNotifications(hwnd);
        return FALSE;
    }
    return TRUE;
}
 
//+-------------------------------------------------------------------------
// Hooks up wininet notifications.
//--------------------------------------------------------------------------
void CWinInetNotify::_HookInetNotifications(HWND hwnd)
{
    // We always want to know when cache items become sticky or unstickey
    // or transition between online and offline
    DWORD dwFlags = CACHE_NOTIFY_URL_SET_STICKY |
                    CACHE_NOTIFY_URL_UNSET_STICKY |
                    CACHE_NOTIFY_SET_ONLINE |
                    CACHE_NOTIFY_SET_OFFLINE ;

    //
    // We only care about things being added to or removed from the
    // cache when we are offline.  The name-space-control greys unavailable
    // items when we are offline.
    //
    if (SHIsGlobalOffline())
    {
        dwFlags |= CACHE_NOTIFY_ADD_URL | CACHE_NOTIFY_DELETE_URL | CACHE_NOTIFY_DELETE_ALL;
    }

    RegisterUrlCacheNotification(hwnd, CWM_WININETNOTIFY, 0, dwFlags, 0);
}

//+-------------------------------------------------------------------------
// Re-broadcasts the notification using SHChangeNotify
//--------------------------------------------------------------------------
void CWinInetNotify::_OnNotify(DWORD_PTR dwFlags)
{
    // Remove any other queued notifications
    MSG msg;
    while (PeekMessage(&msg, s_hwnd, CWM_WININETNOTIFY, CWM_WININETNOTIFY, PM_REMOVE))
    {
        // Combine the notification bits
        dwFlags |= msg.wParam;
    }

    SHChangeDWORDAsIDList dwidl;
    // Align for UNIX
    dwidl.cb      = (unsigned short) PtrDiff(& dwidl.cbZero, &dwidl);
    dwidl.dwItem1 = SHCNEE_WININETCHANGED;
    dwidl.dwItem2 = (DWORD)dwFlags;
    dwidl.cbZero  = 0;

    SHChangeNotify(SHCNE_EXTENDED_EVENT, SHCNF_FLUSH | SHCNF_FLUSHNOWAIT, (LPCITEMIDLIST)&dwidl, NULL);

    // If we are switching between online and offline, we need to update the
    // events that we are interested in.
    if (dwFlags & (CACHE_NOTIFY_SET_ONLINE | CACHE_NOTIFY_SET_OFFLINE))
    {
        _HookInetNotifications(s_hwnd);
    }
}

//+-------------------------------------------------------------------------
// Window procedure for our invisible top-level window.  Receives
// notifications from wininet.
//--------------------------------------------------------------------------
LRESULT CALLBACK CWinInetNotify::_WndProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    switch (uMessage)
    {
        case WM_CREATE:
        {
            // Hook us up to get the notifications
            _HookInetNotifications(hwnd);
            break;
        }

        case CWM_WININETNOTIFY:
        {
            _OnNotify(wParam);
            return 0;
        }
    }

    return DefWindowProcWrap(hwnd, uMessage, wParam, lParam);
}

//+-------------------------------------------------------------------------
// Protect simultaneous access by multiple processes
//--------------------------------------------------------------------------
void CWinInetNotify::_EnterMutex()
{
    ASSERT(_hMutex == NULL);

    // This gets an existing mutex if one exists
    _hMutex = CreateMutex(NULL, FALSE, CWinInetNotify_szWindowClass);

    // Wait for up to 20 seconds
    if (!_hMutex || WaitForSingleObject(_hMutex, 20000) == WAIT_TIMEOUT)
    {
        ASSERT(FALSE);
    }
}

void CWinInetNotify::_LeaveMutex()
{
    if (_hMutex)
    {
        ReleaseMutex(_hMutex);
        CloseHandle(_hMutex);
        _hMutex = NULL;
    }
}
