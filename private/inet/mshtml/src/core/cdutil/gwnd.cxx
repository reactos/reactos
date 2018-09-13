//+-------------------------------------------------------------------------
//
//  File:       gwnd.cxx
//
//  Contents:   Class implementation for CFormsGlobalWindow and helpers.
//
//--------------------------------------------------------------------------

#include "headers.hxx"

#ifndef WS_EX_LAYOUTRTL
#define WS_EX_LAYOUTRTL         0x00400000L // Right to left mirroring
#else
#error "WS_EX_LAYOUTRTL is already defined in winuser.h"
#endif

PerfDbgTag(tagPerfWatch, "Perf", "PerfWatch: Trace MSHTML performance points")

MtDefine(THREADSTATE, PerThread, "THREADSTATE")
MtDefine(CTlsOptionAry, PerThread, "CTlsOptionAry::_pv")
MtDefine(CTlsDocAry, PerThread, "CTlsDocAry::_pv")
MtDefine(CTlsDocSvrAry, PerThread, "CTlsDocSvrAry::_pv")

#define WM_METHODCALL (WM_APP + 2)

struct TIMERENTRY
{
    void *              pvObject;
    PFN_VOID_ONTICK     pfnOnTick;
    UINT                idTimer;
    UINT                idTimerAlias;
};

// We have to do this because of the forward declare in thread state.
MtDefine(CAryTimers, PerThread, "CAryTimers")
MtDefine(CAryTimers_pv, CAryTimers, "CAryTimers::_pv")
DECLARE_CDataAry(CAryTimers, TIMERENTRY, Mt(CAryTimers), Mt(CAryTimers_pv))

struct CALLENTRY
{
    void *              pvObject;
    PFN_VOID_ONCALL     pfnOnCall;
    DWORD_PTR           dwContext;
#if DBG==1 || defined(PERFTAGS)
    char *              pszOnCall;
#endif
};

// We have to do this because of the forward declare in thread state.

MtDefine(CAryCalls, PerThread, "CAryCalls")
MtDefine(CAryCalls_pv, CAryCalls, "CAryCalls::_pv")
DECLARE_CDataAry(CAryCalls, CALLENTRY, Mt(CAryCalls), Mt(CAryCalls_pv))

static ATOM                 s_atomGlobalWndClass = NULL;

// Menu stuff

static void  *              s_pvCommand = NULL;
static PFN_VOID_ONCOMMAND   s_pfnOnCommand;

// Mouse capture

LRESULT CALLBACK GWMouseProc(int  nCode, WPARAM  wParam, LPARAM  lParam);

extern void                 DllUpdateSettings(UINT msg);

#if DBG==1 || defined(PERFTAGS)
char *
DecodeMessage(UINT msg)
{
    switch (msg)
    {
        case WM_NULL:               return("WM_NULL");
        case WM_CREATE:             return("WM_CREATE");
        case WM_DESTROY:            return("WM_DESTROY");
        case WM_MOVE:               return("WM_MOVE");
        case WM_SIZE:               return("WM_SIZE");
        case WM_ACTIVATE:           return("WM_ACTIVATE");
        case WM_SETFOCUS:           return("WM_SETFOCUS");
        case WM_KILLFOCUS:          return("WM_KILLFOCUS");
        case WM_ENABLE:             return("WM_ENABLE");
        case WM_SETREDRAW:          return("WM_SETREDRAW");
        case WM_SETTEXT:            return("WM_SETTEXT");
        case WM_GETTEXT:            return("WM_GETTEXT");
        case WM_GETTEXTLENGTH:      return("WM_GETTEXTLENGTH");
        case WM_PAINT:              return("WM_PAINT");
        case WM_CLOSE:              return("WM_CLOSE");
        case WM_QUERYENDSESSION:    return("WM_QUERYENDSESSION");
        case WM_QUERYOPEN:          return("WM_QUERYOPEN");
        case WM_ENDSESSION:         return("WM_ENDSESSION");
        case WM_QUIT:               return("WM_QUIT");
        case WM_ERASEBKGND:         return("WM_ERASEBKGND");
        case WM_SYSCOLORCHANGE:     return("WM_SYSCOLORCHANGE");
        case WM_SHOWWINDOW:         return("WM_SHOWWINDOW");
        case WM_WININICHANGE:       return("WM_WININICHANGE");
        case WM_DEVMODECHANGE:      return("WM_DEVMODECHANGE");
        case WM_ACTIVATEAPP:        return("WM_ACTIVATEAPP");
        case WM_FONTCHANGE:         return("WM_FONTCHANGE");
        case WM_TIMECHANGE:         return("WM_TIMECHANGE");
        case WM_CANCELMODE:         return("WM_CANCELMODE");
        case WM_SETCURSOR:          return("WM_SETCURSOR");
        case WM_MOUSEACTIVATE:      return("WM_MOUSEACTIVATE");
        case WM_CHILDACTIVATE:      return("WM_CHILDACTIVATE");
        case WM_QUEUESYNC:          return("WM_QUEUESYNC");
        case WM_GETMINMAXINFO:      return("WM_GETMINMAXINFO");
        case WM_PAINTICON:          return("WM_PAINTICON");
        case WM_ICONERASEBKGND:     return("WM_ICONERASEBKGND");
        case WM_NEXTDLGCTL:         return("WM_NEXTDLGCTL");
        case WM_SPOOLERSTATUS:      return("WM_SPOOLERSTATUS");
        case WM_DRAWITEM:           return("WM_DRAWITEM");
        case WM_MEASUREITEM:        return("WM_MEASUREITEM");
        case WM_DELETEITEM:         return("WM_DELETEITEM");
        case WM_VKEYTOITEM:         return("WM_VKEYTOITEM");
        case WM_CHARTOITEM:         return("WM_CHARTOITEM");
        case WM_SETFONT:            return("WM_SETFONT");
        case WM_GETFONT:            return("WM_GETFONT");
        case WM_SETHOTKEY:          return("WM_SETHOTKEY");
        case WM_GETHOTKEY:          return("WM_GETHOTKEY");
        case WM_QUERYDRAGICON:      return("WM_QUERYDRAGICON");
        case WM_COMPAREITEM:        return("WM_COMPAREITEM");
        case WM_COMPACTING:         return("WM_COMPACTING");
        case WM_COMMNOTIFY:         return("WM_COMMNOTIFY");
        case WM_WINDOWPOSCHANGING:  return("WM_WINDOWPOSCHANGING");
        case WM_WINDOWPOSCHANGED:   return("WM_WINDOWPOSCHANGED");
        case WM_POWER:              return("WM_POWER");
        case WM_COPYDATA:           return("WM_COPYDATA");
        case WM_CANCELJOURNAL:      return("WM_CANCELJOURNAL");
        case WM_NOTIFY:             return("WM_NOTIFY");
        case WM_INPUTLANGCHANGEREQUEST: return("WM_INPUTLANGCHANGEREQUEST");
        case WM_INPUTLANGCHANGE:    return("WM_INPUTLANGCHANGE");
        case WM_TCARD:              return("WM_TCARD");
        case WM_HELP:               return("WM_HELP");
        case WM_USERCHANGED:        return("WM_USERCHANGED");
        case WM_NOTIFYFORMAT:       return("WM_NOTIFYFORMAT");
        case WM_CONTEXTMENU:        return("WM_CONTEXTMENU");
        case WM_STYLECHANGING:      return("WM_STYLECHANGING");
        case WM_STYLECHANGED:       return("WM_STYLECHANGED");
        case WM_DISPLAYCHANGE:      return("WM_DISPLAYCHANGE");
        case WM_GETICON:            return("WM_GETICON");
        case WM_SETICON:            return("WM_SETICON");
        case WM_NCCREATE:           return("WM_NCCREATE");
        case WM_NCDESTROY:          return("WM_NCDESTROY");
        case WM_NCCALCSIZE:         return("WM_NCCALCSIZE");
        case WM_NCHITTEST:          return("WM_NCHITTEST");
        case WM_NCPAINT:            return("WM_NCPAINT");
        case WM_NCACTIVATE:         return("WM_NCACTIVATE");
        case WM_GETDLGCODE:         return("WM_GETDLGCODE");
        case WM_SYNCPAINT:          return("WM_SYNCPAINT");
        case WM_NCMOUSEMOVE:        return("WM_NCMOUSEMOVE");
        case WM_NCLBUTTONDOWN:      return("WM_NCLBUTTONDOWN");
        case WM_NCLBUTTONUP:        return("WM_NCLBUTTONUP");
        case WM_NCLBUTTONDBLCLK:    return("WM_NCLBUTTONDBLCLK");
        case WM_NCRBUTTONDOWN:      return("WM_NCRBUTTONDOWN");
        case WM_NCRBUTTONUP:        return("WM_NCRBUTTONUP");
        case WM_NCRBUTTONDBLCLK:    return("WM_NCRBUTTONDBLCLK");
        case WM_NCMBUTTONDOWN:      return("WM_NCMBUTTONDOWN");
        case WM_NCMBUTTONUP:        return("WM_NCMBUTTONUP");
        case WM_NCMBUTTONDBLCLK:    return("WM_NCMBUTTONDBLCLK");
        case WM_KEYDOWN:            return("WM_KEYDOWN");
        case WM_KEYUP:              return("WM_KEYUP");
        case WM_CHAR:               return("WM_CHAR");
        case WM_DEADCHAR:           return("WM_DEADCHAR");
        case WM_SYSKEYDOWN:         return("WM_SYSKEYDOWN");
        case WM_SYSKEYUP:           return("WM_SYSKEYUP");
        case WM_SYSCHAR:            return("WM_SYSCHAR");
        case WM_SYSDEADCHAR:        return("WM_SYSDEADCHAR");
        case WM_IME_STARTCOMPOSITION:   return("WM_IME_STARTCOMPOSITION");
        case WM_IME_ENDCOMPOSITION: return("WM_IME_ENDCOMPOSITION");
        case WM_IME_COMPOSITION:    return("WM_IME_COMPOSITION");
        case WM_INITDIALOG:         return("WM_INITDIALOG");
        case WM_COMMAND:            return("WM_COMMAND");
        case WM_SYSCOMMAND:         return("WM_SYSCOMMAND");
        case WM_TIMER:              return("WM_TIMER");
        case WM_HSCROLL:            return("WM_HSCROLL");
        case WM_VSCROLL:            return("WM_VSCROLL");
        case WM_INITMENU:           return("WM_INITMENU");
        case WM_INITMENUPOPUP:      return("WM_INITMENUPOPUP");
        case WM_MENUSELECT:         return("WM_MENUSELECT");
        case WM_MENUCHAR:           return("WM_MENUCHAR");
        case WM_ENTERIDLE:          return("WM_ENTERIDLE");
        case WM_CTLCOLORMSGBOX:     return("WM_CTLCOLORMSGBOX");
        case WM_CTLCOLOREDIT:       return("WM_CTLCOLOREDIT");
        case WM_CTLCOLORLISTBOX:    return("WM_CTLCOLORLISTBOX");
        case WM_CTLCOLORBTN:        return("WM_CTLCOLORBTN");
        case WM_CTLCOLORDLG:        return("WM_CTLCOLORDLG");
        case WM_CTLCOLORSCROLLBAR:  return("WM_CTLCOLORSCROLLBAR");
        case WM_CTLCOLORSTATIC:     return("WM_CTLCOLORSTATIC");
        case WM_MOUSEMOVE:          return("WM_MOUSEMOVE");
        case WM_LBUTTONDOWN:        return("WM_LBUTTONDOWN");
        case WM_LBUTTONUP:          return("WM_LBUTTONUP");
        case WM_LBUTTONDBLCLK:      return("WM_LBUTTONDBLCLK");
        case WM_RBUTTONDOWN:        return("WM_RBUTTONDOWN");
        case WM_RBUTTONUP:          return("WM_RBUTTONUP");
        case WM_RBUTTONDBLCLK:      return("WM_RBUTTONDBLCLK");
        case WM_MBUTTONDOWN:        return("WM_MBUTTONDOWN");
        case WM_MBUTTONUP:          return("WM_MBUTTONUP");
        case WM_MBUTTONDBLCLK:      return("WM_MBUTTONDBLCLK");
        case WM_MOUSEWHEEL:         return("WM_MOUSEWHEEL");
        case WM_PARENTNOTIFY:       return("WM_PARENTNOTIFY");
        case WM_ENTERMENULOOP:      return("WM_ENTERMENULOOP");
        case WM_EXITMENULOOP:       return("WM_EXITMENULOOP");
        case WM_NEXTMENU:           return("WM_NEXTMENU");
        case WM_SIZING:             return("WM_SIZING");
        case WM_CAPTURECHANGED:     return("WM_CAPTURECHANGED");
        case WM_MOVING:             return("WM_MOVING");
        case WM_POWERBROADCAST:     return("WM_POWERBROADCAST");
        case WM_DEVICECHANGE:       return("WM_DEVICECHANGE");
        case WM_MDICREATE:          return("WM_MDICREATE");
        case WM_MDIDESTROY:         return("WM_MDIDESTROY");
        case WM_MDIACTIVATE:        return("WM_MDIACTIVATE");
        case WM_MDIRESTORE:         return("WM_MDIRESTORE");
        case WM_MDINEXT:            return("WM_MDINEXT");
        case WM_MDIMAXIMIZE:        return("WM_MDIMAXIMIZE");
        case WM_MDITILE:            return("WM_MDITILE");
        case WM_MDICASCADE:         return("WM_MDICASCADE");
        case WM_MDIICONARRANGE:     return("WM_MDIICONARRANGE");
        case WM_MDIGETACTIVE:       return("WM_MDIGETACTIVE");
        case WM_MDISETMENU:         return("WM_MDISETMENU");
        case WM_ENTERSIZEMOVE:      return("WM_ENTERSIZEMOVE");
        case WM_EXITSIZEMOVE:       return("WM_EXITSIZEMOVE");
        case WM_DROPFILES:          return("WM_DROPFILES");
        case WM_MDIREFRESHMENU:     return("WM_MDIREFRESHMENU");
        case WM_IME_SETCONTEXT:     return("WM_IME_SETCONTEXT");
        case WM_IME_NOTIFY:         return("WM_IME_NOTIFY");
        case WM_IME_CONTROL:        return("WM_IME_CONTROL");
        case WM_IME_COMPOSITIONFULL:    return("WM_IME_COMPOSITIONFULL");
        case WM_IME_SELECT:         return("WM_IME_SELECT");
        case WM_IME_CHAR:           return("WM_IME_CHAR");
        case WM_IME_KEYDOWN:        return("WM_IME_KEYDOWN");
        case WM_IME_KEYUP:          return("WM_IME_KEYUP");
        case WM_MOUSEHOVER:         return("WM_MOUSEHOVER");
        case WM_MOUSELEAVE:         return("WM_MOUSELEAVE");
        case WM_CUT:                return("WM_CUT");
        case WM_COPY:               return("WM_COPY");
        case WM_PASTE:              return("WM_PASTE");
        case WM_CLEAR:              return("WM_CLEAR");
        case WM_UNDO:               return("WM_UNDO");
        case WM_RENDERFORMAT:       return("WM_RENDERFORMAT");
        case WM_RENDERALLFORMATS:   return("WM_RENDERALLFORMATS");
        case WM_DESTROYCLIPBOARD:   return("WM_DESTROYCLIPBOARD");
        case WM_DRAWCLIPBOARD:      return("WM_DRAWCLIPBOARD");
        case WM_PAINTCLIPBOARD:     return("WM_PAINTCLIPBOARD");
        case WM_VSCROLLCLIPBOARD:   return("WM_VSCROLLCLIPBOARD");
        case WM_SIZECLIPBOARD:      return("WM_SIZECLIPBOARD");
        case WM_ASKCBFORMATNAME:    return("WM_ASKCBFORMATNAME");
        case WM_CHANGECBCHAIN:      return("WM_CHANGECBCHAIN");
        case WM_HSCROLLCLIPBOARD:   return("WM_HSCROLLCLIPBOARD");
        case WM_QUERYNEWPALETTE:    return("WM_QUERYNEWPALETTE");
        case WM_PALETTEISCHANGING:  return("WM_PALETTEISCHANGING");
        case WM_PALETTECHANGED:     return("WM_PALETTECHANGED");
        case WM_HOTKEY:             return("WM_HOTKEY");
        case WM_PRINT:              return("WM_PRINT");
        case WM_PRINTCLIENT:        return("WM_PRINTCLIENT");
        case WM_USER:               return("WM_USER");
        case WM_USER+1:             return("WM_USER+1");
        case WM_USER+2:             return("WM_USER+2");
        case WM_USER+3:             return("WM_USER+3");
        case WM_USER+4:             return("WM_USER+4");
    }

    return("");
}

char *
DecodeWindowClass(HWND hwnd)
{
    static char ach[40];
    ach[0] = 0;
    GetClassNameA(hwnd, ach, sizeof(ach));
    return(ach);
}

#endif

//+-------------------------------------------------------------------------
//
//  Method:     ResetTimer
//
//  Synopsis:   Resets the timer identified by dwCookie.  ResetTimer
//              results in the timer setting being changed for the
//              given timer without allocating a new, unique timer id.
//
//--------------------------------------------------------------------------

static HRESULT
ResetTimer(void * pvObject, UINT idTimer, UINT uTimeout)
{
    TIMERENTRY *    pte;
    int             c;
    THREADSTATE *   pts = GetThreadState();

    // Windows NT rounds the time up to 10.  If time is less 
    // than 10, NT spews to the debugger.  Work around
    // this problem by rounding up to 10.

    if (uTimeout < 10)
        uTimeout = 10;

    for (c = pts->gwnd.paryTimers->Size(), pte = *pts->gwnd.paryTimers;
        c > 0;
        c--, pte++)
    {
        if ((pte->pvObject == pvObject) && (pte->idTimer == idTimer))
        {
            if (SetTimer(pts->gwnd.hwndGlobalWindow,
                    pte->idTimerAlias, uTimeout, NULL) == 0)
            {
                RRETURN(E_FAIL);
            }

            return S_OK;
        }
    }

    return S_FALSE;
}


//+-------------------------------------------------------------------------
//
//  Method:     OnTimer
//
//  Synopsis:   Handles timer event from the global window.
//
//--------------------------------------------------------------------------

static void  CALLBACK
OnTimer(HWND hwnd, UINT id)
{
    TIMERENTRY *    pte;
    int             c;
    THREADSTATE *   pts = GetThreadState();

    PerfDbgLog(tagPerfWatch, NULL, "+Gwnd OnTimer");

    for (c = pts->gwnd.paryTimers->Size(), pte = *pts->gwnd.paryTimers;
        c > 0;
        c--, pte++)
    {
        if (pte->idTimerAlias == id)
        {
            PerfDbgLog(tagPerfWatch, NULL, "+Gwnd OnTimer OnTick");

#ifdef WIN16
            (*pte->pfnOnTick)((CVoid *)pte->pvObject, pte->idTimer);
#else
            CALL_METHOD((CVoid *)pte->pvObject,pte->pfnOnTick,(pte->idTimer));
#endif

            PerfDbgLog(tagPerfWatch, NULL, "-Gwnd OnTimer OnTick");

            break;
        }
    }

    PerfDbgLog(tagPerfWatch, NULL, "-Gwnd OnTimer");
}


//+-------------------------------------------------------------------------
//
//  Method:     OnCommand
//
//  Synopsis:   Handles menu commands.
//
//--------------------------------------------------------------------------

static void
OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    if (s_pvCommand != NULL)
    {
#ifdef WIN16
        (*s_pfnOnCommand)((CVoid *)s_pvCommand, id, hwndCtl, codeNotify);
#else
        CALL_METHOD((CVoid *)s_pvCommand, s_pfnOnCommand, (id, hwndCtl, codeNotify));
#endif
        s_pvCommand = NULL;
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     GetUniqueID
//
//  Synopsis:   Fetches a unique timer ID by checking our sorted array
//              of used IDs for a new value.
//
//--------------------------------------------------------------------------

static UINT
GetUniqueID()
{
    THREADSTATE *   pts;
    int             c;
    TIMERENTRY *    pte;
    BOOL            fDone = FALSE;

    pts = GetThreadState();
    while (fDone == FALSE)
    {
        pts->gwnd.uID++;

        // Note: We don't use the range 0x0000 through 0x1FFF.  This is
        // reserved for hard-coded timer identifiers.  Compuserve incorrectly
        // intercepts timer id 0x000F, which is yet another reason for doing
        // this.

        if (pts->gwnd.uID < 0x2000)
        {
            pts->gwnd.uID = 0x2000;
        }

        fDone = TRUE;

        for (c = (*(pts->gwnd.paryTimers)).Size(), pte = *(pts->gwnd.paryTimers);
            c > 0;
            c--, pte++)
        {
            if (pts->gwnd.uID == pte->idTimerAlias)
            {
                fDone = FALSE;
                break;
            }
        }
    }

    return pts->gwnd.uID;
}

//+-------------------------------------------------------------------------
//
//  Function:   FormsSetTimer
//
//  Synopsis:   Sets a timer using the forms global window.
//
//  Arguments:  [pGWS]      Pointer to a timer sink
//              [idTimer]   Caller-specified id that will be passed back
//                          on a timer event.
//              [uTimeout]  Elapsed time between timer events
//
//--------------------------------------------------------------------------

HRESULT
FormsSetTimer(
        void *pvObject,
        PFN_VOID_ONTICK pfnOnTick,
        UINT idTimer, UINT uTimeout)
{
    THREADSTATE *   pts;
    UINT            idTimerAlias;
    HRESULT         hr;

    Assert(pvObject);

    pts = GetThreadState();

    if (pts->gwnd.hwndGlobalWindow == NULL)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    //
    //  Attempt to reset the timer.  If this fails, ie, no matching
    //  timer exists, continue on and set the timer.
    //

    hr = ResetTimer(pvObject, idTimer, uTimeout);
    if (hr == S_OK)
        goto Cleanup;

    hr = (*(pts->gwnd.paryTimers)).EnsureSize((*(pts->gwnd.paryTimers)).Size() + 1);
    if (hr)
        goto Cleanup;

    idTimerAlias = GetUniqueID();

    if (    g_dwPlatformID == VER_PLATFORM_WIN32_NT
#ifndef WIN16
        &&  g_dwPlatformVersion >= 0x00050000
#endif
        )
    {
        // BUGBUG
        // Windows NT 5.0 rounds the time up to 10.  If time is less 
        // than 10, it spews to the debugger.  Work around
        // this problem by rounding up to 10.

        if (uTimeout < 10)
            uTimeout = 10;
    }

    if (SetTimer(pts->gwnd.hwndGlobalWindow, idTimerAlias, uTimeout, NULL) == 0)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    (*(pts->gwnd.paryTimers))[(*(pts->gwnd.paryTimers)).Size()].pvObject = pvObject;
    (*(pts->gwnd.paryTimers))[(*(pts->gwnd.paryTimers)).Size()].pfnOnTick = pfnOnTick;
    (*(pts->gwnd.paryTimers))[(*(pts->gwnd.paryTimers)).Size()].idTimerAlias = idTimerAlias;
    (*(pts->gwnd.paryTimers))[(*(pts->gwnd.paryTimers)).Size()].idTimer = idTimer;
    (*(pts->gwnd.paryTimers)).SetSize((*(pts->gwnd.paryTimers)).Size() + 1);

Cleanup:
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Function:   FormsKillTimer
//
//  Synopsis:   Kills a forms timer.
//
//  Arguments:  [dwCookie]  Cookie identifying the timer.  Obtained from
//                          FormsSetTimer.
//
//--------------------------------------------------------------------------

HRESULT
FormsKillTimer(void * pvObject, UINT idTimer)
{
    extern DWORD g_dwTls;
    THREADSTATE *   pts;
    TIMERENTRY *    pte;
    int             i, c;
    UINT            idTimerAlias;

    // Note: We do not use pts = GetThreadState() function because
    // this function is called from the DLL process detach
    // code after TlsGetValue() has ceased to function correctly.
    // This scenario is probably a result of a bug in Windows '95.
    pts = (THREADSTATE *)(TlsGetValue(g_dwTls));
    if (!pts || pts->gwnd.paryTimers == NULL)
        return S_FALSE;

    for (c = (*(pts->gwnd.paryTimers)).Size(), i = 0, pte = (*(pts->gwnd.paryTimers));
        c > 0;
        c--, i++, pte++)
    {
        if ((pte->pvObject == pvObject) && (pte->idTimer == idTimer))
        {
            idTimerAlias = pte->idTimerAlias;
            (*(pts->gwnd.paryTimers)).Delete(i);
            KillTimer(TLS(gwnd.hwndGlobalWindow), idTimerAlias);
            return S_OK;
        }
    }
    return S_FALSE;
}


//+-------------------------------------------------------------------------
//
//  Function:   FormsTrackPopupMenu
//
//  Synopsis:   Allows windowless controls to use the global window for
//              command routing of popup menu selection.
//
//--------------------------------------------------------------------------

HRESULT
FormsTrackPopupMenu(
        HMENU hMenu,
        UINT fuFlags,
        int x,
        int y,
        HWND hwndMessage,
        int *piSelection)
{
    BOOL    fAvail;
    HRESULT hr = S_OK;
    MSG     msg;
    HWND    hwnd;

    hwnd = (hwndMessage) ? (hwndMessage) : (TLS(gwnd.hwndGlobalWindow));

    if (::TrackPopupMenu(hMenu, fuFlags, x, y, 0, hwnd, (RECT *)NULL))
    {
        // The menu popped up and the item was chosen.  Peek messages
        // until the specified command was found.
        fAvail = PeekMessage(&msg,
            hwnd,
            WM_COMMAND,
            WM_COMMAND,
            PM_REMOVE);

        if (fAvail)
        {
            *piSelection = GET_WM_COMMAND_ID(msg.wParam, msg.lParam);
            hr = S_OK;
        }
        else
        {
            // No WM_COMMAND was available, so this means that the
            // menu was brought down
            hr = S_FALSE;
        }
    }
    else
    {
        hr = GetLastWin32Error();
    }

    RRETURN1(hr, S_FALSE);
}

#ifndef WIN16
//+-------------------------------------------------------------------------
//
//  Function:   InvalidateProcessWindow
//
//--------------------------------------------------------------------------

static BOOL CALLBACK
InvalidateProcessWindow(HWND hwnd, LPARAM lparam)
{
    DWORD dwProcessId;

    GetWindowThreadProcessId(hwnd, &dwProcessId);
    if (dwProcessId == (DWORD)lparam && hwnd)
    {
        RedrawWindow(
            hwnd,
            NULL,
            NULL,
            RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
    }
    return TRUE;
}

void InvalidateProcessWindows()
{
    EnumWindows(InvalidateProcessWindow, GetCurrentProcessId());
}
#endif //!WIN16

//+-------------------------------------------------------------------------
//
//  Function:   _GWPostMethodCallEx
//
//  Synopsis:   Call given method on given object a later time.
//              It is the caller's responsiblity to insure that the
//              object remains valid until the call is made or the
//              posted call is killed.
//
//  Arguments:  pvObject    The object
//              pfnOnCall   Method to call.
//              dwContext   Context for method
//
//--------------------------------------------------------------------------

HRESULT _GWPostMethodCallEx(THREADSTATE * pts, void *pvObject,
    PFN_VOID_ONCALL pfnOnCall, DWORD_PTR dwContext, BOOL fIgnoreContext
#if DBG==1 || defined(PERFTAGS)
    , char * pszOnCall
#endif
    )
{
    HRESULT         hr;
    CALLENTRY *     pce;
    int             c;

    EnterCriticalSection(&pts->gwnd.cs);

    for (c = (*(pts->gwnd.paryCalls)).Size(), pce = (*(pts->gwnd.paryCalls));
        c > 0;
        c--, pce++)
    {
        if (pce->pvObject == pvObject &&
            pce->pfnOnCall == pfnOnCall &&
            pce->dwContext == dwContext &&
            !fIgnoreContext)
        {
            hr = S_OK;
            goto Cleanup;
        }
    }

    c = pts->gwnd.paryCalls->Size();

    hr = (*(pts->gwnd.paryCalls)).EnsureSize(c + 1);
    if (hr)
        goto Cleanup;

    pce = &(*(pts->gwnd.paryCalls))[c];

    pce->pvObject = pvObject;
    pce->pfnOnCall = pfnOnCall;
    pce->dwContext = dwContext;

#if DBG==1 || defined(PERFTAGS)
    pce->pszOnCall = pszOnCall;
#endif

    if (!pts->gwnd.fMethodCallPosted)
    {
        if (!PostMessage(pts->gwnd.hwndGlobalWindow, WM_METHODCALL, 0, 0))
        {
            hr = GetLastWin32Error();
            goto Cleanup;
        }

        pts->gwnd.fMethodCallPosted = TRUE;
    }

    (*(pts->gwnd.paryCalls)).SetSize(c + 1);

Cleanup:
    LeaveCriticalSection(&pts->gwnd.cs);

    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Function:   GWKillMethodCallEx
//
//  Synopsis:   Kill method call posted with GWPostMethodCall.
//
//  Arguments:  pvObject    The object
//              pfnOnCall   Method to call.  If null, kills all calls
//                          for pvObject.
//              dwContext   Context.  If zero, kills all calls for pvObject
//                          and pfnOnCall.
//
//--------------------------------------------------------------------------

void
GWKillMethodCallEx(THREADSTATE * pts, void *pvObject, PFN_VOID_ONCALL pfnOnCall,
    DWORD_PTR dwContext)
{
    CALLENTRY *     pce;
    int             c;

    // Handle pts being NULL for
    Assert(pts);

    // check for no calls before entering critical section
    if (!pts || (pts->gwnd.paryCalls->Size() == 0))
        return;

    EnterCriticalSection(&pts->gwnd.cs);

    // Null pointer instead of deleting array element to
    // handle calls to this function when OnMethodCall
    // is iterating over the array.

    for (c = (*(pts->gwnd.paryCalls)).Size(), pce = (*(pts->gwnd.paryCalls));
        c > 0;
        c--, pce++)
    {
        if (pce->pvObject == pvObject)
        {
            if (pfnOnCall == NULL)
            {
                pce->pvObject = NULL;
            }
            else if (pce->pfnOnCall == pfnOnCall)
            {
                if (pce->dwContext == dwContext || dwContext == 0)
                {
                    pce->pvObject = NULL;
                }
            }
        }
    }

    LeaveCriticalSection(&pts->gwnd.cs);
}

void
GWKillMethodCall(void *pvObject, PFN_VOID_ONCALL pfnOnCall, DWORD_PTR dwContext)
{
    GWKillMethodCallEx(GetThreadState(), pvObject, pfnOnCall, dwContext);
}

#if DBG == 1

BOOL
GWHasPostedMethod( void *pvObject )
{
    extern DWORD    g_dwTls;
    THREADSTATE *   pts = (THREADSTATE *)(TlsGetValue(g_dwTls));
    CALLENTRY *     pce;
    int             c;
    BOOL            fRet = FALSE;

    if (!pts)
        return FALSE;

    EnterCriticalSection(&pts->gwnd.cs);

    // Null pointer instead of deleting array element to
    // handle calls to this function when OnMethodCall
    // is iterating over the array.

    for (c = (*(pts->gwnd.paryCalls)).Size(), pce = (*(pts->gwnd.paryCalls));
        c > 0;
        c--, pce++)
    {
        if (pce->pvObject == pvObject)
        {
            fRet = TRUE;
            break;
        }
    }

    LeaveCriticalSection(&pts->gwnd.cs);

    return fRet;
}

#endif

//+-------------------------------------------------------------------------
//
//  Funciton:   GlobalWndOnMethodCall
//
//  Synopsis:   Handles deferred method calls.
//
//--------------------------------------------------------------------------

void
GlobalWndOnMethodCall()
{
    THREADSTATE *   pts = GetThreadState();
    CALLENTRY *     pce;
    CALLENTRY *     pceLive;
    int             i;
    int             c;

    PerfDbgLog(tagPerfWatch, NULL, "+Gwnd OnMethodCall");

    //
    // Re-enable posting of layout messages because the function call
    // could cause us to enter a modal loop and then OnMethodCall would
    // not process further messages.  This will happen if the function
    // call fires an event procedure that calls Userform.Show in VB.
    //

    EnterCriticalSection(&pts->gwnd.cs);

    pts->gwnd.fMethodCallPosted = FALSE;

    for (i = 0; i < (*(pts->gwnd.paryCalls)).Size(); i++)
    {
        // Pointer into array is fetched at every iteration in order
        // to handle calls to GWPostMethodCall during the loop.

        pce = &(*(pts->gwnd.paryCalls))[i];
        if (pce->pvObject)
        {
            CALLENTRY ce = *pce;
            pce->pvObject = NULL;

            LeaveCriticalSection(&pts->gwnd.cs);

            PerfDbgLog1(tagPerfWatch, NULL, "+Gwnd OnMethodCall (%s)", ce.pszOnCall);

#ifdef WIN16
            (*ce.pfnOnCall)((CVoid *)ce.pvObject, ce.dwContext);
#else
            CALL_METHOD((CVoid *)ce.pvObject, ce.pfnOnCall, (ce.dwContext));
#endif

            PerfDbgLog1(tagPerfWatch, NULL, "-Gwnd OnMethodCall (%s)", ce.pszOnCall);

            EnterCriticalSection(&pts->gwnd.cs);

            //
            // DO NOT USE pce after the method call - if the object calls
            // GWPostMethodCall it may cause gwnd.paryCalls to do a ReAlloc,
            // which invalidates pce!
            //
        }
    }

    pceLive = (*(pts->gwnd.paryCalls));
    i = 0;
    for (c = (*(pts->gwnd.paryCalls)).Size(), pce = (*(pts->gwnd.paryCalls));
        c > 0;
        c--, pce++)
    {
        if (pce->pvObject)
        {
            *pceLive++ = *pce;
            i++;
        }
    }

    (*(pts->gwnd.paryCalls)).SetSize(i);

    LeaveCriticalSection(&pts->gwnd.cs);

    PerfDbgLog(tagPerfWatch, NULL, "-Gwnd OnMethodCall");
}

//+-------------------------------------------------------------------------
//
//  Function:   GWSetCapture
//
//  Synopsis:   Capture the mouse.
//
//--------------------------------------------------------------------------

HRESULT
GWSetCapture(void *pvObject, PFN_VOID_ONMESSAGE pfnOnMouseMessage, HWND hwnd)
{
    THREADSTATE *   pts;

    Assert(pvObject);

    pts = GetThreadState();

    if (pvObject != pts->gwnd.pvCapture)
    {
        if (pts->gwnd.pvCapture)
#ifdef WIN16
            (*(pts->gwnd.pfnOnMouseMessage))((CVoid *)pts->gwnd.pvCapture, WM_CAPTURECHANGED, 0, 0);
#else
            CALL_METHOD((CVoid *)pts->gwnd.pvCapture, pts->gwnd.pfnOnMouseMessage,
                        (WM_CAPTURECHANGED, 0, 0));
#endif
        pts->gwnd.pvCapture = pvObject;
        pts->gwnd.pfnOnMouseMessage = pfnOnMouseMessage;
        pts->gwnd.hwndCapture = hwnd;
    }

    if (GetCapture() != TLS(gwnd.hwndGlobalWindow))
    {
        SetCapture(TLS(gwnd.hwndGlobalWindow));

#ifndef WIN16
        if (((g_dwPlatformID == VER_PLATFORM_WIN32_NT) ||
             (g_dwPlatformID == VER_PLATFORM_WIN32_UNIX)) &&
            !pts->gwnd.hhookGWMouse)
        {
            pts->gwnd.hhookGWMouse = SetWindowsHookEx(
                                           WH_MOUSE,
                                           GWMouseProc,
                                           (HINSTANCE) NULL, GetCurrentThreadId());
        }
#endif //!WIN16
    }

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   GWReleaseCapture
//
//  Synopsis:   Release the mouse capture.
//
//--------------------------------------------------------------------------

void
GWReleaseCapture(void *pvObject)
{
    THREADSTATE *   pts;

    Assert(pvObject);

    pts = GetThreadState();

    if (pts->gwnd.pvCapture == pvObject)
    {
        CVoid * pvCapture = (CVoid *) pts->gwnd.pvCapture;
        pts->gwnd.pvCapture = NULL;
#ifdef WIN16
        (*(pts->gwnd.pfnOnMouseMessage))(pvCapture, WM_CAPTURECHANGED, 0, 0);
#else
        CALL_METHOD(pvCapture,pts->gwnd.pfnOnMouseMessage,(WM_CAPTURECHANGED, 0, 0));

        if (((g_dwPlatformID == VER_PLATFORM_WIN32_NT) ||
             (g_dwPlatformID == VER_PLATFORM_WIN32_UNIX)) && 
            pts->gwnd.hhookGWMouse)
        {
            UnhookWindowsHookEx(pts->gwnd.hhookGWMouse);
            pts->gwnd.hhookGWMouse = NULL;
        }
#endif //!WIN16

        if (GetCapture() == TLS(gwnd.hwndGlobalWindow))
        {
#if DBG==1
            Assert(!TLS(fHandleCaptureChanged));
#endif DBG==1
            ReleaseCapture();
        }
    }
}



//+-------------------------------------------------------------------------
//
//  Function:   GWGetCapture
//
//  Synopsis:   Return the object that has captured the mouse.
//
//--------------------------------------------------------------------------

BOOL
GWGetCapture(void * pvObject)
{
    if (GetCapture() == TLS(gwnd.hwndGlobalWindow))
        return pvObject == TLS(gwnd.pvCapture);
    return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Function:   GWMouseProc
//
//  Synopsis:   Mouse proc for the global window.  This mouse hook is installed
//              under WinNT when the global window has mouse capture and removed
//              when it releases capture.  Under windows 95, the global window
//              receives WM_CAPTURECHANGED messages and this hook isn't necessary.
//              Under WinNT this mouse proc simulates the WM_CAPTURECHANGED.
//
//  BUGBUG - This global mouse proc should be modified  so that WM_CAPTURECHANGED
//           is sent to all forms3 windows when mouse capture changes.
//--------------------------------------------------------------------------

LRESULT CALLBACK 
GWMouseProc(int  nCode, WPARAM  wParam, LPARAM  lParam)
{
    if (nCode < 0)  /* do not process the message */
        return CallNextHookEx(TLS(gwnd.hhookGWMouse), nCode,
            wParam, lParam);

    if (nCode == HC_ACTION)
    {
        MOUSEHOOKSTRUCT *   pmh = (MOUSEHOOKSTRUCT *) lParam;

        if (wParam >= WM_MOUSEFIRST &&
            wParam <= WM_MOUSELAST &&
            TLS(gwnd.pvCapture) &&
            pmh->hwnd != TLS(gwnd.hwndGlobalWindow))
        {
            GWReleaseCapture(TLS(gwnd.pvCapture));
        }
    }

    return CallNextHookEx(TLS(gwnd.hhookGWMouse), nCode, wParam, lParam);
}


//+-------------------------------------------------------------------------
//
//  Function:   GlobalWndProc
//
//  Synopsis:   Window proc for the global window
//
//--------------------------------------------------------------------------

#ifdef WIN16
STDAPI
#else
LRESULT CALLBACK
#endif
GlobalWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PerfDbgLog4(tagPerfWatch, NULL, "+GlobalWndProc msg=%04lX (%s) hwnd=%lX (%s)",
                msg, DecodeMessage(msg), hwnd, DecodeWindowClass(hwnd));

#ifndef WIN16
    extern DWORD g_dwTls;
#endif
    THREADSTATE *   pts;
    LRESULT         lRet = 0;
    LONG            lDllCount;
    BOOL            fCallDefWndProc = FALSE;

#ifdef WIN16
    THREAD_HANDLE dwOldImpersonatedTId;

    if (msg == WM_NCCREATE)
    {
        dwOldImpersonatedTId = GetCurrentThreadId();
        
        // assume WM_NCCreate will always be called from the correct thread.
        SetWindowLong(hwnd, GWL_USERDATA, (LONG) dwOldImpersonatedTId);
     
        // the 'old impersonated thread' may well be '0'.
        dwOldImpersonatedTId = w16ImpersonateThread(dwOldImpersonatedTId);
    }
    else
    {
        dwOldImpersonatedTId = w16ImpersonateThread(GetWindowLong(hwnd, GWL_USERDATA));
    }
#else // !WIN16
	// (paulnel) 74803 - turn off mirroring if the system supports it. This causes all
	//           sorts of problems in Trident.
    if (msg == WM_NCCREATE)
    {
        DWORD dwExStyles;
        if ((dwExStyles = GetWindowLong(hwnd, GWL_EXSTYLE)) & WS_EX_LAYOUTRTL)
        {
            SetWindowLong(hwnd, GWL_EXSTYLE, dwExStyles &~ WS_EX_LAYOUTRTL);
        }
    }
#endif // WIN16

    pts = (THREADSTATE *)TlsGetValue(g_dwTls);
    lDllCount = pts ? pts->dll.lObjCount : 0;

    // We need to guard against a Release() call made during
    // message processing causing the ref count to go to zero 
    // and the TLS getting blown away.  This will be done
    // by manually adjusting the counter (to be fast) and 
    // only call DecrementObjectCount if necessary.

    if (lDllCount)
        Verify(++pts->dll.lObjCount > 0);

#ifdef _MAC
    if ( msg == WM_MACINTOSH && LOWORD(wParam) == WLM_SETMENUBAR)
            // dont change the menu bar
                return TRUE;
    if ((msg >= WM_LBUTTONDOWN) && msg <= WM_LBUTTONDBLCLK)
    {
        // We only want to process the Left Button messages
        MacSimulateMouseButtons (msg, wParam);
    }
#endif

    CHECK_HEAP();

    // Note: When adding new messages to this loop do the 
    //       following:
    //  - Handle the message
    //  - set lRet to be the lResult to return (if not 0)
    //  - set fCallDefWndProc to TRUE if DefWindowProc() should
    //    be called (default is to NOT call DefWindowProc())
    //  - use 'break' to exit the switch statement
    
    switch (msg)
    {
    case WM_TIMER:
        OnTimer(hwnd, (UINT)wParam);
        break;

    HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        
    case WM_METHODCALL:
        GlobalWndOnMethodCall();
        break;    

    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK:
        if (pts && pts->gwnd.pvCapture)
        {
            POINT  pt;
            pt.x = MAKEPOINTS(lParam).x;
            pt.y = MAKEPOINTS(lParam).y;
            ScreenToClient(pts->gwnd.hwndCapture, &pt);
#ifdef WIN16
            lRet = (*(pts->gwnd.pfnOnMouseMessage))((CVoid *)pts->gwnd.pvCapture,
                msg,
                wParam,
                MAKELONG(pt.x, pt.y));
#else
            lRet = CALL_METHOD((CVoid *)pts->gwnd.pvCapture, pts->gwnd.pfnOnMouseMessage,
                               (msg,
                                wParam,
                                MAKELONG(pt.x, pt.y)));
#endif
        }
        else
        {
            fCallDefWndProc = TRUE;
        }
        break;

    case WM_CAPTURECHANGED:
#if DBG==1
        TLS(fHandleCaptureChanged) = TRUE;
#endif DBG==1

        if (pts && pts->gwnd.pvCapture)
        {
            CVoid * pvCapture = (CVoid *) pts->gwnd.pvCapture;
            pts->gwnd.pvCapture = 0;
#ifdef WIN16
            lRet = (*(pts->gwnd.pfnOnMouseMessage))(pvCapture,
                msg,
                wParam,
                lParam);
#else
            lRet = CALL_METHOD(pvCapture, pts->gwnd.pfnOnMouseMessage,
                               (msg,
                                wParam,
                                lParam));
#endif
        }
        else
        {
            fCallDefWndProc = TRUE;
        }
#if DBG==1
        TLS(fHandleCaptureChanged) = FALSE;
#endif DBG==1
        break;

    // case WM_WININICHANGE: obsolete, should not be used anymore
    // replaced with WM_SETTINGCHANGE (which has the same ID, but uses wParam)    
#ifdef WIN16
    case WM_WININICHANGE:
#else
    case WM_SETTINGCHANGE:
#endif
    case WM_SYSCOLORCHANGE:
    case WM_DEVMODECHANGE:
    case WM_FONTCHANGE:
#if(WINVER >= 0x0400)
    case WM_DISPLAYCHANGE:
#endif
    case WM_USER + 338:         // sent by properties change dialog
        TraceTag((tagError, "Processing system change %d", msg));
        DllUpdateSettings(msg);
        break;

    default:
        fCallDefWndProc = TRUE;
        goto Cleanup;
    }

    CHECK_HEAP();

Cleanup:

    if (lDllCount && pts->dll.lObjCount == 1)
    {
        // TLS about to go away.  Let the Passivates occur
        // and then say we handled the message.  Since
        // DecrementObjectCount plays with the secondary
        // count as well we need to increment it as well.

        IncrementSecondaryObjectCount( 3 );
        DecrementObjectCount(NULL);
        lRet = 0;
    }
    else
    {
        if (lDllCount)
            Verify(--pts->dll.lObjCount >= 0);
        if (fCallDefWndProc)
        {
            lRet = DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }

#ifdef WIN16
    w16ImpersonateThread(dwOldImpersonatedTId);
#endif

    PerfDbgLog(tagPerfWatch, NULL, "-GlobalWndProc");

    return lRet;

}


//+-------------------------------------------------------------------------
//
//  Function:   DeinitGlobalWindow
//
//--------------------------------------------------------------------------

void
DeinitGlobalWindow(
    THREADSTATE *   pts)
{
    Assert(pts);

#if DBG==1
    if (pts->gwnd.paryTimers)
    {
        Assert((*(pts->gwnd.paryTimers)).Size() == 0);
    }
    if (pts->gwnd.paryCalls)
    {
        for (int i = 0; i < (*(pts->gwnd.paryCalls)).Size(); i++)
        {
            Assert((*(pts->gwnd.paryCalls))[i].pvObject == NULL);
        }
    }
#endif

    if (pts->gwnd.paryCalls)
        (*(pts->gwnd.paryCalls)).DeleteAll();

    // Delete per-thread dynamic arrays
    delete pts->gwnd.paryTimers;
    delete pts->gwnd.paryCalls;

    if (pts->gwnd.hwndGlobalWindow)
    {
#if !defined(_MAC)
        if (pts->dll.idThread == GetCurrentThreadId())
            Verify(DestroyWindow(pts->gwnd.hwndGlobalWindow));
#endif

        // BUGBUG: if we're on the wrong thread we can't destroy the window

        pts->gwnd.hwndGlobalWindow = NULL;

        DeleteCriticalSection(&pts->gwnd.cs);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   InitGlobalWindow
//
//  Synopsis:   Initializes and creates the global hwnd.
//
//--------------------------------------------------------------------------

HRESULT
InitGlobalWindow(
    THREADSTATE *   pts)
{
    HRESULT     hr = S_OK;
    TCHAR * pszBuf;

    Assert(pts);

#if !defined(_MAC) 
    // Create the per-thread "global" window
    if (!s_atomGlobalWndClass)
    {
        hr = THR(RegisterWindowClass(
                _T("Hidden"),
                GlobalWndProc,
                0,
                NULL,
                NULL,
                &s_atomGlobalWndClass));
        if (hr)
            goto Error;
    }
#endif // _MAC

#if defined(WIN16) || defined(_MAC)
    TCHAR szBuf[128];
    GlobalGetAtomName(s_atomGlobalWndClass, szBuf, ARRAY_SIZE(szBuf));
    pszBuf = szBuf;
#else
    pszBuf = (TCHAR *)(DWORD_PTR)s_atomGlobalWndClass; 
#endif

#if defined(_MAC) 
	pts->gwnd.hwndGlobalWindow = (HWND) GlobalWndProc;
#else
    pts->gwnd.hwndGlobalWindow = TW32(NULL, CreateWindow(
            pszBuf,
            NULL,
            WS_POPUP,
            0, 0, 0, 0,
            NULL,
            NULL,
            g_hInstCore,
            NULL));
    if (pts->gwnd.hwndGlobalWindow == NULL)
    {
        hr = GetLastWin32Error();
        goto Error;
    }
#endif // _MAC

    InitializeCriticalSection(&pts->gwnd.cs);

    // Allocate per-thread dynamic arrays
    pts->gwnd.paryTimers = new CAryTimers;
    pts->gwnd.paryCalls  = new CAryCalls;
    if (!pts->gwnd.paryTimers || !pts->gwnd.paryCalls)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

Error:
    RRETURN(hr);
}

#ifdef _MAC // macbugbug:  don't think this function is necessary anymore ???
Boolean GWMouseCaptured(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
	Boolean Result = false;
	
	switch ( msg )
	{
		case WM_MOUSEMOVE:
		case WM_LBUTTONUP:
		 	if ( ::GetCapture() == TLS(gwnd.hwndGlobalWindow) )
		 	{
				(*plResult) = GlobalWndProc(nil, msg, wParam, lParam);
				Result = true;
			}
			break;
			
		default:
			break;
	}
	
	return Result;
}

#endif

