#ifndef _INETNOT_H
#define _INETNOT_H

//+-------------------------------------------------------------------------
// Wininet currently only support sending notifications of changes to the
// cache to one window.  This class creates one top-level window per process
// for receiving and re-broadcasting these notifications.  When the
// process shuts down, we look for another window to take over these
// messages. 
//
// This is an imperfect solution. It would have been much easier if the
// wininet guys could have been convinced to call SHChangeNotify instead. 
// However, they are planning to enhace this later. (stevepro))
//--------------------------------------------------------------------------
class CWinInetNotify
{
public:
    CWinInetNotify();
    ~CWinInetNotify();

    void Enable(BOOL fEnable = TRUE);

protected:
    void _EnterMutex();
    void _LeaveMutex();

    static void _HookInetNotifications(HWND hwnd);
    static void _OnNotify(DWORD_PTR dwFlags);
    static LRESULT CALLBACK _WndProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);
    enum
    {
        CWM_WININETNOTIFY  = WM_USER + 410
    };

    HANDLE          _hMutex;
    BOOL            _fEnabled;

    static HWND     s_hwnd;
    static ULONG    s_ulEnabled;
};

#define CWinInetNotify_szWindowClass TEXT("Inet Notify_Hidden")



#endif //_INETNOT_H