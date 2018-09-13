#include "priv.h"
#include "sccls.h"
#include "iface.h"
#include "resource.h"
#include "caggunk.h"
#include "menuisf.h"
#include "menubar.h"
#include "menuband.h"
#include "iaccess.h"
#include "apithk.h"

//=================================================================
// Implementation of CMenuAgent
//
//  The single global object of this class (g_menuagent) is the
//  manager of the message filter proc used to track mouse and
//  keyboard messages on behalf of CTrackPopupBar while a menu is
//  in a modal menu loop in TrackPopupMenu.
//
//  We track these messages so we can pop out of the menu, behaving
//  as if the visual menu bar consisted of a homogeneous menu
//  object.
//
//=================================================================

extern "C" void DumpMsg(LPCTSTR pszLabel, MSG * pmsg);

struct CMenuAgent
{
public:
    HHOOK       _hhookMsg;
    HWND        _hwndSite;          // hwnd to receive forwarded messages
    HWND        _hwndParent;
    CTrackPopupBar * _ptpbar;
    IMenuPopup * _pmpParent;
    void*       _pvContext;
    HANDLE      _hEvent;

    BITBOOL     _fEscHit: 1;

    // we need to keep track of whether the last selected
    // menu item was on a popup or not.  we can do this by storing the 
    // last WM_MENUSELECT flags
    UINT        _uFlagsLastSelected; 
    HMENU       _hmenuLastSelected;
    POINT       _ptLastMove;

    void    Init(void* pvContext, CTrackPopupBar * ptpbar, IMenuPopup * pmpParent, HWND hwndParent, HWND hwndSite);
    void    Reset(void* pvContext);
    void    CancelMenu(void* pvContext);

    static LRESULT CALLBACK MsgHook(int nCode, WPARAM wParam, LPARAM lParam);

//private:
    void _OnMenuSelect(HMENU hmenu, int i, UINT uFlags);
    BOOL _OnKey(WPARAM vkey);
};

// Just one of these, b/c we only need one message filter
CMenuAgent g_menuagent = { 0 };     


/*----------------------------------------------------------
Purpose: Initialize the message filter hook

*/
void CMenuAgent::Init(void* pvContext, CTrackPopupBar * ptpbar, IMenuPopup * pmpParent, 
                      HWND hwndParent, HWND hwndSite)
{
    TraceMsg(TF_MENUBAND, "Initialize CMenuAgent");

    ASSERT(IS_VALID_READ_PTR(ptpbar, CTrackPopupBar));
    ASSERT(IS_VALID_CODE_PTR(pmpParent, IMenuPopup));
    ASSERT(IS_VALID_HANDLE(hwndSite, WND));

    if (_pvContext != pvContext)
    {
        // When switching contexts, we need to collapse the old menu. This keeps us from
        // hosing the menubands when switching from one browser to another.
        CancelMenu(_pvContext);
        ATOMICRELEASE(_ptpbar);
        ATOMICRELEASE(_pmpParent);
        _pvContext = pvContext;
    }

    pmpParent->SetSubMenu(ptpbar, TRUE);

    _hwndSite = hwndSite;
    _hwndParent = hwndParent;

    // Since the message hook wants to forward messages to the toolbar,
    // we need to ask the pager control to do this
    Pager_ForwardMouse(_hwndSite, TRUE);

    _pmpParent = pmpParent;
    _pmpParent->AddRef();

    _ptpbar = ptpbar;
    _ptpbar->AddRef();

    // HACKHACKHACKHACKHACK (lamadio)
    // On Windows 9x kernel can't handle the reentrancy problem where you have two hooks
    // in two separate processes. When one process looses focus, we collapse the Menu.
    // After that we remove our hook. Problem is: Between the Loosing focus and
    // removing the hook, the other IE process popped up a menu and installed a hook
    // the two hooks mutilate each other.
    if (IsOS(OS_WINDOWS))
    {
        ASSERT(_hEvent == NULL);
        
        _hEvent = OpenEventA(EVENT_ALL_ACCESS, FALSE, "Shell.MenuAgent");

        if (!_hEvent)   //event routines return NULL on failure.
            // Don't need to use CreateAllAccessSecurityAttributes since this
            // is Win9x-only code anyway
            _hEvent = CreateEventA(NULL, TRUE, TRUE, "Shell.MenuAgent");

        if (_hEvent)
            WaitForSingleObject(_hEvent, INFINITE);
    }

    if (NULL == _hhookMsg)
    {
        if (_hEvent)
            ResetEvent(_hEvent);
        _hhookMsg = SetWindowsHookEx(WH_MSGFILTER, MsgHook, HINST_THISDLL, 0);
        if (!_hhookMsg && _hEvent)
        {
            SetEvent(_hEvent);
            CloseHandle(_hEvent);
            _hEvent = NULL;
        }
    }

    _fEscHit = FALSE;

    GetCursorPos(&_ptLastMove);
}    


/*----------------------------------------------------------
Purpose: Reset the menu agent; no longer track mouse and keyboard
         messages.  The menuband calls this when it exits menu mode.

*/
void CMenuAgent::Reset(void* pvContext)
{
    if (_pvContext == pvContext)
    {
        _pmpParent->SetSubMenu(_ptpbar, FALSE);

        // The only time to not send MPOS_FULLCANCEL is if the escape
        // key caused the menu to terminate.

        if ( !_fEscHit )
            _pmpParent->OnSelect(MPOS_FULLCANCEL);

        // Eat any mouse-down/up sequence left in the queue.  This is how 
        // we keep the toolbar from getting a mouse-down if the user 
        // clicks on the same menuitem as what is currently popped down.
        // (E.g., click File, then click File again.  W/o this, the menu
        // would never toggle up.)

        MSG msg;

        while (PeekMessage(&msg, _hwndSite, WM_LBUTTONDOWN, WM_LBUTTONUP, PM_REMOVE))
            ;   // Do nothing

        Pager_ForwardMouse(_hwndSite, FALSE);

        _hwndSite = NULL;
        _hwndParent = NULL;

        ATOMICRELEASE(_pmpParent);
        ATOMICRELEASE(_ptpbar);

        if (_hhookMsg)
        {
            TraceMsg(TF_MENUBAND, "CMenuAgent: Hook removed");

            UnhookWindowsHookEx(_hhookMsg);
            _hhookMsg = NULL;

            if (_hEvent)
            {
                SetEvent(_hEvent);
                CloseHandle(_hEvent);
                _hEvent = NULL;
            }
        }

        _pvContext = NULL;
    }
}    


/*----------------------------------------------------------
Purpose: Make the menu go away

*/
void CMenuAgent::CancelMenu(void* pvContext)
{
    if (_pvContext == pvContext)
    {
        if (_hwndParent)
        {
            ASSERT(IS_VALID_HANDLE(_hwndParent, WND));

            TraceMsg(TF_MENUBAND, "Sending cancel mode to menu");

            // Use PostMessage so USER32 doesn't RIP on us in 
            // MsgHook when it returns from the WM_MOUSEMOVE
            // that triggered this code path in the first place.

            PostMessage(_hwndParent, WM_CANCELMODE, 0, 0);

            // Disguise this as if the escape key was hit,
            // since this is called when the mouse hovers over
            // another menu sibling.
            _fEscHit = TRUE;

            _pmpParent->SetSubMenu(_ptpbar, FALSE);
        }
    }
}    

// store away the identity of the selected menu item.
// if uFlags & MF_POPUP then i is the index.
// otherwise it's the command and we need to convert it to the index.
// we store index always because some popups don't have ids

void CMenuAgent::_OnMenuSelect(HMENU hmenu, int i, UINT uFlags)
{
    _uFlagsLastSelected = uFlags;
    _hmenuLastSelected = hmenu;
}

BOOL CMenuAgent::_OnKey(WPARAM vkey)
{
    //
    // If the menu window is RTL mirrored, then the arrow keys should
    // be mirrored to reflect proper cursor movement. [samera]
    //
    if (IS_WINDOW_RTL_MIRRORED(_hwndSite))
    {
        switch (vkey)
        {
        case VK_LEFT:
          vkey = VK_RIGHT;
          break;

        case VK_RIGHT:
          vkey = VK_LEFT;
          break;
        }
    }

    switch (vkey)
    {
    case VK_RIGHT:
        if (!_hmenuLastSelected || !(_uFlagsLastSelected & MF_POPUP) || (_uFlagsLastSelected & MF_DISABLED) ) 
        {
            // if the currently selected item does not have a cascade, then 
            // we need to cancel out of all of this and tell the top menu bar to go right
            _pmpParent->OnSelect(MPOS_SELECTRIGHT);
        }
        break;
        
    case VK_LEFT:
        if (!_hmenuLastSelected || _hmenuLastSelected == _ptpbar->GetPopupMenu()) {
            // if the currently selected menu item is in our top level menu,
            // then we need to cancel out of all this menu loop and tell the top menu bar
            // to go left 
            _pmpParent->OnSelect(MPOS_SELECTLEFT);
        }
        break;
        
    default:
        return FALSE;
        
    }
    
    return TRUE;
}


/*----------------------------------------------------------
Purpose: Message hook used to track keyboard and mouse messages
         while in a TrackPopupMenu modal loop.

*/
LRESULT CMenuAgent::MsgHook(int nCode, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRet = 0;
    MSG * pmsg = (MSG *)lParam;

    switch (nCode)
    {
    case MSGF_MENU:
#ifdef DEBUG
        if (IsFlagSet(g_dwDumpFlags, DF_MSGHOOK))
            DumpMsg(TEXT("MsgHook"), pmsg);
#endif

        switch (pmsg->message)
        {
        case WM_MENUSELECT:
            // keep track of the items as the are selected.
            g_menuagent._OnMenuSelect(GET_WM_MENUSELECT_HMENU(pmsg->wParam, pmsg->lParam),
                                      GET_WM_MENUSELECT_CMD(pmsg->wParam, pmsg->lParam),
                                      GET_WM_MENUSELECT_FLAGS(pmsg->wParam, pmsg->lParam));
            break;
            
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            // Since we've received this msg, any previous escapes
            // (like escaping out of a cascaded menu) should be cleared
            // to prevent a false reason for termination.
            g_menuagent._fEscHit = FALSE;
            break;

        case WM_KEYDOWN:
            if (g_menuagent._OnKey(pmsg->wParam))
                break;
            
        case WM_SYSKEYDOWN:
            g_menuagent._fEscHit = (VK_ESCAPE == pmsg->wParam);
            break;

        case WM_MOUSEMOVE:
            // HACKHACK (isn't all of this a hack?): ignore zero-move
            // mouse moves, so the mouse does not contend with the keyboard.

            POINT pt;
            
            // In screen coords....
            pt.x = GET_X_LPARAM(pmsg->lParam);
            pt.y = GET_Y_LPARAM(pmsg->lParam);

            if (g_menuagent._ptLastMove.x == pt.x && 
                g_menuagent._ptLastMove.y == pt.y)
            {
                TraceMsg(TF_MENUBAND, "CMenuAgent: skipping dup mousemove");
                break;
            }
            g_menuagent._ptLastMove = pt;

            // Since we got a WM_MOUSEMOVE, we need to tell the Menuband global message hook.
            // We need to do this because this message hook steels all of the messages, and
            // the Menuband message hook never updates it's internal cache for removing duplicate
            // WM_MOUSEMOVE messages which cause problems as outlined in CMsgFilter::_HandleMouseMessages
            g_msgfilter.AcquireMouseLocation();

            // Forward the mouse moves to the toolbar so the toolbar still
            // has a chance to hot track.  Must convert the points to the 
            // toolbar's client space.
            
            ScreenToClient(g_menuagent._hwndSite, &pt);

            SendMessage(g_menuagent._hwndSite, pmsg->message, pmsg->wParam, 
                        MAKELPARAM(pt.x, pt.y));
            break;
        }
        break;

    default:
        if (0 > nCode)
            return CallNextHookEx(g_menuagent._hhookMsg, nCode, wParam, lParam);
        break;
    }

    // Pass it on to the next hook in the chain
    if (0 == lRet)
        lRet = CallNextHookEx(g_menuagent._hhookMsg, nCode, wParam, lParam);

    return lRet;
}    



//=================================================================
// Implementation of a menu deskbar object that uses TrackPopupMenu.
//
// This object uses traditional USER32 menus (via TrackPopupMenu)
// to implement menu behavior.  It uses the CMenuAgent object to 
// help get its work done.  Since the menu deskbar site (_punkSite) 
// sits in a modal loop while any menu is up, it needs to know when
// to quit its loop.  The child object accomplishes this by sending
// an OnSelect(MPOS_FULLCANCEL).
//
// The only time that TrackPopupMenu returns (but we don't want to
// send an MPOS_FULLCANCEL) is if it's b/c the Escape key was hit.
// This just means cancel the current level.  Returning from Popup
// is sufficient for this case.  Otherwise, all other cases of
// returning from TrackPopupMenu means we send a MPOS_FULLCANCEL.
//
// Summary:
//
//  1) User clicked outside the menu.  This is a full cancel.
//  2) User hit the Alt key.  This is a full cancel.
//  3) User hit the Esc key.  This just cancels the current level.
//     (TrackPopupMenu handles this fine.  No notification needs
//     to be sent b/c we want the top-level menu to stay in its
//     modal loop.)
//  4) User selected a menu item.  This is a full cancel.
//
//=================================================================


#undef THISCLASS
#undef SUPERCLASS
#define SUPERCLASS  CMenuDeskBar

// Constructor
CTrackPopupBar::CTrackPopupBar(void* pvContext, int id, HMENU hmenu, HWND hwnd) :
   _hmenu(hmenu),
   _hwndParent(hwnd),
   _id(id),
   _pvContext(pvContext)
{
    _nMBIgnoreNextDeselect = RegisterWindowMessage(TEXT("CMBIgnoreNextDeselect"));
}

// Destructor
CTrackPopupBar::~CTrackPopupBar()
{
    SetSite(NULL);
}


STDMETHODIMP_(ULONG) CTrackPopupBar::AddRef()
{
    return SUPERCLASS::AddRef();
}

STDMETHODIMP_(ULONG) CTrackPopupBar::Release()
{
    return SUPERCLASS::Release();
}

STDMETHODIMP CTrackPopupBar::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CTrackPopupBar, IMenuPopup),
        QITABENT(CTrackPopupBar, IObjectWithSite),
        { 0 },
    };

    HRESULT hres = QISearch(this, qit, riid, ppvObj);

    if (FAILED(hres))
    {
        hres = SUPERCLASS::QueryInterface(riid, ppvObj);
    }

    return hres;
}

/*----------------------------------------------------------
Purpose: IServiceProvider::QueryService method

*/
STDMETHODIMP CTrackPopupBar::QueryService(REFGUID guidService, REFIID riid, void **ppvObj)
{
    if (IsEqualGUID(guidService, SID_SMenuBandChild)) 
    {
        if (IsEqualIID(riid, IID_IAccessible))
        {
            HRESULT hres = E_OUTOFMEMORY;
            CAccessible* pacc = new CAccessible(_hmenu, _id);

            if (pacc)
            {
                hres = pacc->InitAcc();
                if (SUCCEEDED(hres))
                {
                    hres = pacc->QueryInterface(riid, ppvObj);
                }
                pacc->Release();
            }

            return hres;
        }
        else
            return QueryInterface(riid, ppvObj);
    }
    else
        return SUPERCLASS::QueryService(guidService, riid, ppvObj);
}

/*----------------------------------------------------------
Purpose: IMenuPopup::OnSelect method

         This allows the parent menubar to tell us when to
         bail out of the TrackPopupMenu
*/
STDMETHODIMP CTrackPopupBar::OnSelect(DWORD dwType)
{
    switch (dwType)
    {
    case MPOS_CANCELLEVEL:
    case MPOS_FULLCANCEL:
        g_menuagent.CancelMenu(_pvContext);
        break;

    default:
        TraceMsg(TF_WARNING, "CTrackPopupBar doesn't handle this MPOS_ value: %d", dwType);
        break;
    }
    return S_OK;    
}    


/*----------------------------------------------------------
Purpose: IMenuPopup::SetSubMenu method

*/
STDMETHODIMP CTrackPopupBar::SetSubMenu(IMenuPopup * pmp, BOOL bSet)
{
    return E_NOTIMPL;
}    

// HACKHACK: DO NOT TOUCH! This is the only way to select
// the first item for a user menu. TrackMenuPopup by default does
// not select the first item. We pump these messages to our window. 
// User snags these messages, and thinks the user pressed the down button
// and selects the first item for us. The lParam is needed because Win95 gold
// validated this message before using it. Another solution would be to listen
// to WM_INITMENUPOPUP and look for the HWND of the menu. Then send that 
// window the private message MN_SELECTFIRSTVALIDITEM. But thats nasty compared 
// to this. - lamadio 1.5.99
void CTrackPopupBar::SelectFirstItem()
{
    HWND hwndFocus = GetFocus();
    // pulled the funny lparam numbers out of spy's butt.
    if (hwndFocus) {
        PostMessage(hwndFocus, WM_KEYDOWN, VK_DOWN, 0x11500001);
        PostMessage(hwndFocus, WM_KEYUP, VK_DOWN, 0xD1500001);
#ifdef UNIX
        /* HACK HACK
         * The above PostMessages were causing the second menu item
         * to be selected if you access the menu from the keyboard.
         * The following PostMessages will nullify the above effect.
         * This is to make sure that menus in shdocvw work properly
         * with user32 menus.
         */
        PostMessage(hwndFocus, WM_KEYDOWN, VK_UP, 0x11500001);
        PostMessage(hwndFocus, WM_KEYUP, VK_UP, 0xD1500001);
#endif /* UNIX */
    }
}
           
DWORD GetBuildNumber()
{
    OSVERSIONINFO osvi;
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (GetVersionEx(&osvi))
        return osvi.dwBuildNumber;
    else
        return 0;
}

/*----------------------------------------------------------
Purpose: IMenuPopup::Popup method

         Invoke the menu.
*/
STDMETHODIMP CTrackPopupBar::Popup(POINTL *ppt, RECTL* prcExclude, DWORD dwFlags)
{
    static dwBuildNumber = GetBuildNumber();
    ASSERT(IS_VALID_READ_PTR(ppt, POINTL));
    ASSERT(NULL == prcExclude || IS_VALID_READ_PTR(prcExclude, RECTL));
    ASSERT(IS_VALID_CODE_PTR(_pmpParent, IMenuPopup));

    // We must be able to talk to the parent menu bar 
    if (NULL == _pmpParent)
        return E_FAIL;

    ASSERT(IS_VALID_HANDLE(_hmenu, MENU));
    ASSERT(IS_VALID_CODE_PTR(_punkSite, IUnknown));
    
    HMENU hmenu = GetSubMenu(_hmenu, _id);
    HWND hwnd;
    TPMPARAMS tpm;
    TPMPARAMS * ptpm = NULL;

    // User32 does not want to fix this for compatibility reasons,
    // but TrackPopupMenu does not snap to the nearest monitor on Single and Multi-Mon
    // systems. This has the side effect that if we pass a non-visible coordinate, then
    // User places menu at a random location on screen. So instead, we're going to bias
    // the point to the monitor.

    MONITORINFO mi = {0};
    mi.cbSize = sizeof(mi);

    HMONITOR hMonitor = MonitorFromPoint(*((POINT*)ppt), MONITOR_DEFAULTTONEAREST);
    GetMonitorInfo(hMonitor, &mi);

    if (ppt->x >= mi.rcMonitor.right)
        ppt->x = mi.rcMonitor.right;

    if (ppt->y >= mi.rcMonitor.bottom)
        ppt->y = mi.rcMonitor.bottom;

    if (ppt->x <= mi.rcMonitor.left)
        ppt->x = mi.rcMonitor.left;

    if (ppt->y <= mi.rcMonitor.top)
        ppt->y = mi.rcMonitor.top;


    if (prcExclude)
    {
        tpm.cbSize = SIZEOF(tpm);
        tpm.rcExclude = *((LPRECT)prcExclude);
        ptpm = &tpm;
    }

    // The forwarding code in CShellBrowser::_ShouldForwardMenu
    // and CDocObjectHost::_ShouldForwardMenu expects the first
    // WM_MENUSELECT to be sent for the top-level menu item.
    // 
    // We need to fake an initial menu select on the top menu band
    // to mimic USER and satisfy this expectation.
    //
    UINT uMSFlags = MF_POPUP;
    SendMessage(_hwndParent, WM_MENUSELECT, MAKEWPARAM(_id, uMSFlags), (LPARAM)_hmenu);
    
    SendMessage(_hwndParent, _nMBIgnoreNextDeselect, NULL, NULL);

    // Initialize the menu agent
    IUnknown_GetWindow(_punkSite, &hwnd);
    
    VARIANTARG v = {0};
    UINT uFlags = TPM_VERTICAL | TPM_TOPALIGN;
    UINT uAnimateFlags = 0;
    if (SUCCEEDED(IUnknown_Exec(_punkSite, &CGID_MENUDESKBAR, MBCID_GETSIDE, 0, NULL, &v))) {
        if (v.vt == VT_I4 && 
            (v.lVal == MENUBAR_RIGHT ||
             v.lVal == MENUBAR_LEFT))
        {
            uFlags = TPM_TOPALIGN;
        }

        switch (v.lVal)
        {
        case MENUBAR_LEFT:      uAnimateFlags = TPM_HORNEGANIMATION;
            break;
        case MENUBAR_RIGHT:     uAnimateFlags = TPM_HORPOSANIMATION;
            break;
        case MENUBAR_TOP:       uAnimateFlags = TPM_VERNEGANIMATION;
            break;
        case MENUBAR_BOTTOM:    uAnimateFlags = TPM_VERPOSANIMATION;
            break;
        }
    }

    g_menuagent.Init(_pvContext, this, _pmpParent, _hwndParent, hwnd);

    ASSERT(IS_VALID_HANDLE(hmenu, MENU));
    if (dwFlags & MPPF_INITIALSELECT)
        SelectFirstItem();

    // This feature only works on build 1794 or greater.
    if (g_bRunOnNT5 && dwBuildNumber >= 1794 && dwFlags & MPPF_NOANIMATE)
        uFlags |= TPM_NOANIMATION;

#ifndef MAINWIN

    if (g_bRunOnMemphis || g_bRunOnNT5)
        uFlags |= uAnimateFlags;

    TrackPopupMenuEx(hmenu, uFlags,
                   ppt->x, ppt->y, _hwndParent, ptpm);
#else
    // Current MainWin's implementation of TrackPopupMenuEx is buggy.
    // I failed to fix it, so I replaced the call by TrackPopupMenu,
    // that provides partial functionality.
    // Hopefully, jluu will be able to fix it.
    TrackPopupMenu(hmenu, uFlags, ppt->x, ppt->y, 0, _hwndParent, 
                   &ptpm->rcExclude);
#endif

    // Tell the parent that the menu is now gone
    SendMessage(_hwndParent, WM_MENUSELECT, MAKEWPARAM(0, 0xFFFF), NULL);

    g_menuagent.Reset(_pvContext);

    return S_FALSE;
}
