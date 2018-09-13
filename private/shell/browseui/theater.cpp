#include "priv.h"
#include "theater.h"
#include "itbar.h"
#include "sccls.h"
#include "resource.h"
#include "brand.h"
#include "menuband.h"

#include "mluisupp.h"

#if defined(MAINWIN)
#include <mainwin.h>
#endif

#ifndef DISABLE_FULLSCREEN

#define IDT_INITIAL             1
#define IDT_UNHIDE              2
#define IDT_TOOLBAR             3
#define IDT_BROWBAR             4
#define IDT_TASKBAR             5
#define IDT_DELAY               6
#define IDT_HIDETOOLBAR         7
#define IDT_HIDEBROWBAR         8
#define IDT_HIDEFLOATER         9
#define IDT_HIDEFLOATER1SEC     10
#define IDT_INITIALBROWSERBAR   11

#define TF_THEATER              0

#define E_TOP       0
#define E_LEFT      1
#define E_RIGHT     2
#define E_BOTTOM    3

// association list.  sort of like dpa, but by association key rather than by index
// we need this because windows hooks are global and have no data associated with them.
// on the callback, we use our thread id as the key
CAssociationList CTheater::_al; // associate threadid with CTheater objects


// _GetWindowRectRel: gets window's coordinates relative to _hwndBrowser
BOOL CTheater::_GetWindowRectRel(HWND hWnd, LPRECT lpRect)
{
    BOOL bResult = GetWindowRect(hWnd, lpRect);
    if (bResult)
        MapWindowPoints(HWND_DESKTOP, _hwndBrowser, (LPPOINT)lpRect, 2);
    return bResult;
}

CTheater::CTheater(HWND hwnd, HWND hwndToolbar, IUnknown* punkOwner) :
   _hwndBrowser(hwnd), _hwndToolbar(hwndToolbar), _cRef(1)
{
    ASSERT(punkOwner);
    _punkOwner = punkOwner;
    _punkOwner->AddRef();

    _al.Add(GetCurrentThreadId(), this);

    _wp.length = SIZEOF(_wp);
    GetWindowPlacement(_hwndBrowser, &_wp);
    GetWindowRect(_hwndBrowser, &_rcOld);
#ifndef FULL_DEBUG
    SetWindowZorder(_hwndBrowser, HWND_TOPMOST);
#endif
    ShowWindow(_hwndBrowser, SW_MAXIMIZE);
    _Initialize();

    
    _fAutoHideBrowserBar = TRUE;
}

CTheater::~CTheater()
{
    SetWindowZorder(_hwndBrowser, HWND_NOTOPMOST);
    SetBrowserBar(NULL, 0, 0);
    if (_hhook)
    {
        UnhookWindowsHookEx(_hhook);
        _hhook = NULL;
    }
    _al.Delete(GetCurrentThreadId());
    
    KillTimer(_hwndFloater, IDT_UNHIDE);
    KillTimer(_hwndFloater, IDT_DELAY);
    KillTimer(_hwndFloater, IDT_HIDETOOLBAR);
    KillTimer(_hwndFloater, IDT_HIDEBROWBAR);

    if (_pdbBrand) {
        IUnknown_SetSite(_pdbBrand, NULL);
        _pdbBrand->CloseDW(0);
        _pdbBrand->Release();
    }
    
    if (_hwndClose) {
        HIMAGELIST himl = (HIMAGELIST)SendMessage(_hwndClose, TB_SETIMAGELIST, 0, 0);
        ImageList_Destroy(himl);
    }
    
    if (_hwndFloater) {
        DestroyWindow(_hwndFloater);

    }

    _punkOwner->Release();
}

BOOL CTheater::_IsBrowserActive()
{
    HRESULT hr = IUnknown_Exec(_punkOwner, &CGID_Explorer, SBCMDID_ISBROWSERACTIVE, 0, NULL, NULL);
    return (hr == S_OK);
}

void CTheater::_ShowTaskbar()
{    
    if (SHForceWindowZorder(_hwndTaskbar, HWND_TOPMOST))
        _fTaskbarShown = TRUE;
}

void CTheater::_HideTaskbar()
{
    if (_IsBrowserActive())
    {
        HWND hwnd = GetForegroundWindow();
        if (!GetCapture() && (SHIsChildOrSelf(_hwndTaskbar, hwnd) != S_OK))
        {
            if (SetWindowZorder(_hwndTaskbar, _hwndBrowser))
                _fTaskbarShown = FALSE;
        } 
    }
}

void CTheater::_ShowToolbar()
{
    if (!_fShown)
    {
        KillTimer(_hwndFloater, IDT_HIDETOOLBAR);                

        if (_hwndToolbar)
        {
            RECT rcParent;
            RECT rc;

            GetWindowRect(_hwndToolbar, &rc);
            GetClientRect(_hwndBrowser, &rcParent);
        
            IUnknown_Exec(_punkOwner, &CGID_PrivCITCommands, CITIDM_THEATER, THF_UNHIDE, NULL, NULL);
            
            SetWindowPos(_hwndToolbar, _hwndFloater, 0, 0, RECTWIDTH(rcParent), RECTHEIGHT(rc), SWP_NOACTIVATE | SWP_SHOWWINDOW);
            _ShowFloater();
        }        
        _fShown = TRUE;
    }
}

void CTheater::_HideToolbar()
{
    // don't start hiding if floater is still active
    if (!_cActiveRef) 
    {
        if (_fAutoHideToolbar && (GetCapture() == NULL) && !IsChild(_hwndToolbar, GetFocus()))
        {        
            _HideFloater();

            SetTimer(_hwndFloater, IDT_HIDETOOLBAR, 50, NULL);
            _cyLast = -1;
            _fShown = FALSE;       
        }
    }
}

void CTheater::_ContinueHideToolbar()
{
    while (1) {
        RECT rc;
        int cy;

        _GetWindowRectRel(_hwndToolbar, &rc);

#ifdef MAINWIN
    if (MwIsMwwmAllwm(_hwndBrowser))
    {
        // Simulating
        rc.right = rc.right - rc.left;
        rc.bottom = rc.bottom - rc.top;
        rc.top = 0;
        rc.bottom = 0;
    }
#endif

        cy = rc.bottom;
        OffsetRect(&rc, 0, -4);
        
        if (cy > 0 && cy != _cyLast) {
            RECT rcT;
            _GetWindowRectRel(_hwndToolbar, &rcT);
            
            SetWindowPos(_hwndToolbar, NULL, rcT.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            UpdateWindow(_hwndToolbar);
            Sleep(10);
            _cyLast = cy;
        } else {
            IUnknown_Exec(_punkOwner, &CGID_PrivCITCommands, CITIDM_THEATER, THF_HIDE, NULL, NULL);
            ShowWindow(_hwndToolbar, SW_HIDE);

            // Hide floater and restore parenthood so msgs are picked up again
            ShowWindow(_hwndFloater, SW_HIDE);            
            SetParent(_hwndFloater, _hwndBrowser);            
            
            break;
        }
    }
}

void CTheater::_ShowBrowBar()
{
    if (!_fBrowBarShown)
    {
        RECT rc;        

        KillTimer(_hwndFloater, IDT_HIDEBROWBAR);
        _GetWindowRectRel(_hwndBrowBar, &rc);        

        rc.left = 0;
        rc.right = _cxBrowBarShown;
        SetWindowPos(_hwndBrowBar, _hwndToolbar, rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc), SWP_NOACTIVATE);

        _SanityCheckZorder();
        
        _fBrowBarShown = TRUE;
    }
}

void CTheater::_HideBrowBar()
{
    // don't start hiding if something has capture
    if (_fBrowBarShown && _CanHideWindow(_hwndBrowBar))
    {
        SetTimer(_hwndFloater, IDT_HIDEBROWBAR, 50, NULL);        
        _fBrowBarShown = FALSE;
        if (_fInitialBrowserBar)
            KillTimer(_hwndFloater, IDT_INITIALBROWSERBAR);                        
    }
}

// BUGBUG: We should signal browser bar to suppress resizing during autohide.  Current scheme 
// works only because browser bar doesn't resize itself upon SetWindowPos to zero width.
void CTheater::_ContinueHideBrowBar()
{
    RECT rc;
    POINT pt;
    INT cxOffset, cxEdge;
    
    if (!_GetWindowRectRel(_hwndBrowBar, &rc))
        return;             // bail
    cxEdge = rc.left;
    
    if (_fInitialBrowserBar)
        cxOffset = -2;      // slow hide
    else
        cxOffset = -6;      // fast hide
    _fInitialBrowserBar = FALSE;
    
    while (rc.right > cxEdge)
    {
        // If mouse has moved over the bar, kill the hide
        GetCursorPos(&pt);
        MapWindowPoints(HWND_DESKTOP, _hwndBrowser, &pt, 1);
        if (PtInRect(&rc, pt))
        {
            _ShowBrowBar();
            return;
        }
        OffsetRect(&rc, cxOffset, 0);                
        SetWindowPos(_hwndBrowBar, NULL, rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc), SWP_NOZORDER | SWP_NOACTIVATE);
        RedrawWindow(_hwndBrowBar, NULL, NULL, RDW_UPDATENOW | RDW_ALLCHILDREN);
        Sleep(5);
    }
}

BOOL CTheater::_CanHideWindow(HWND hwnd)
{
    return (!GetCapture() && !IsChild(hwnd, GetFocus()));
}

void CTheater::_ShowFloater()
{    
    if (!_fFloaterShown) 
    {
        _fFloaterShown = TRUE;
        SetParent(_hwndFloater, _hwndBrowser);

        KillTimer(_hwndFloater, IDT_HIDEFLOATER);
        
        _SizeFloater();
        SetWindowPos(_hwndFloater, HWND_TOP, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);
        InvalidateRect(_hwndFloater, NULL, TRUE);
        UpdateWindow(_hwndFloater);
        
        ShowWindow(_hwndFloater, SW_SHOW);
        if (!_fShown && _fAutoHideToolbar)
            _DelayHideFloater();
    }    
}

void CTheater::_DelayHideFloater()
{
    if (!_cActiveRef)
    {
        SetTimer(_hwndFloater, IDT_HIDEFLOATER1SEC, 1000, NULL);
    }
}

void CTheater::_HideFloater()
{
    if (_fAutoHideToolbar && _fFloaterShown)
    {
        if (!_fShown)
        {
            // don't start hiding if something has capture
            if (_CanHideWindow(_hwndFloater))
            {
                SetTimer(_hwndFloater, IDT_HIDEFLOATER, 50, NULL);        
                _fFloaterShown = FALSE;
                ASSERT(!_cActiveRef);
                _cActiveRef++;
                return;
            }
            else
            {
                _DelayHideFloater();
            }
        }
        else
        {
            SetParent(_hwndFloater, _hwndToolbar);
            _fFloaterShown = FALSE;
        }
    }
}

void CTheater::_ContinueHideFloater()
{
    while (1) 
    {
        RECT rc;        
        _GetWindowRectRel(_hwndFloater, &rc);        

        rc.left += 6;
        
        if (rc.left < rc.right) 
        {            
            SetWindowPos(_hwndFloater, NULL, rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc), SWP_NOZORDER | SWP_NOACTIVATE);
            UpdateWindow(_hwndFloater);
            Sleep(5);            
        } 
        else 
        {                        
            ShowWindow(_hwndFloater, SW_HIDE);
            _cActiveRef--;            
            break;
        }
    }
}

void CTheater::_Unhide(int iWhich, UINT uDelay)
{
    _iUnhidee = iWhich;
    SetTimer(_hwndFloater, IDT_UNHIDE, uDelay, NULL);    
}

BOOL CTheater::_PtNearWindow(POINT pt, HWND hwnd)
{
    RECT rc;

    _GetWindowRectRel(hwnd, &rc);
    InflateRect(&rc, 60, 60);
    return (PtInRect(&rc, pt));
}

int GetWindowHeight(HWND hwnd)
{
    ASSERT(hwnd);

    RECT rc;
    GetWindowRect(hwnd, &rc);
    return RECTHEIGHT(rc);
}

BOOL CTheater::_PtOnEdge(POINT pt, int iEdge)
{
    RECT rc;
    _GetWindowRectRel(_hwndBrowser, &rc);    

    switch (iEdge)
    {
        case E_LEFT:
            rc.right = rc.left + CX_HIT;
            goto leftright;

        case E_RIGHT:
            rc.left = rc.right - CX_HIT;
leftright:
            rc.top += GetWindowHeight(_hwndToolbar);
            rc.bottom -= GetSystemMetrics(SM_CXVSCROLL);
            break;

        case E_TOP:
            rc.bottom = rc.top + CX_HIT;
            goto topbottom;

        case E_BOTTOM:
            rc.top = rc.bottom - CX_HIT;
topbottom:
            InflateRect(&rc, - GetSystemMetrics(SM_CXVSCROLL), 0);
            break;
    }
    return (PtInRect(&rc, pt));
}

LRESULT CTheater::_OnMsgHook(int nCode, WPARAM wParam, MOUSEHOOKSTRUCT *pmhs, BOOL fFake)
{    
    if (nCode >= 0) 
    {
        POINT pt;
        GetCursorPos(&pt);
        MapWindowPoints(HWND_DESKTOP, _hwndBrowser, &pt, 1);

        BOOL bTryUnhideTaskbar = !_fTaskbarShown;

        // The timer business is so that we don't unhide 
        // on a user trying to get to the scrollbar
        if (_iUnhidee) 
        {
            KillTimer(_hwndFloater, IDT_UNHIDE);
            _iUnhidee = 0;
        }
        
        if (_PtOnEdge(pt, E_LEFT))
        {
            if (!_fBrowBarShown && _hwndBrowBar)
                _Unhide(IDT_BROWBAR, SHORT_DELAY);
        }
        else if (_PtOnEdge(pt, E_TOP))
        {
            if (!_fShown)
                _Unhide(IDT_TOOLBAR, SHORT_DELAY);
        }
        else if (!_PtOnEdge(pt, E_RIGHT) && !_PtOnEdge(pt, E_BOTTOM))
        {
            bTryUnhideTaskbar = FALSE;
        }
        
#ifndef UNIX
        if (bTryUnhideTaskbar && !_fDelay && !_iUnhidee)
        {
            RECT rc;
            _GetWindowRectRel(_hwndTaskbar, &rc);
            if (PtInRect(&rc, pt))
                _Unhide(IDT_TASKBAR, GetCapture() ? LONG_DELAY : SHORT_DELAY);
        }
#endif

        if (_fAutoHideBrowserBar && _fBrowBarShown && !_PtNearWindow(pt, _hwndBrowBar))
            _HideBrowBar();
        
        if (_fAutoHideToolbar && _fShown && !_PtNearWindow(pt, _hwndToolbar))
            _HideToolbar();        
        
#ifndef UNIX
        if (_fTaskbarShown && !_PtNearWindow(pt, _hwndTaskbar))
           _HideTaskbar();
#endif
    }

    if (fFake)
        return 0;
    else
        return CallNextHookEx(_hhook, nCode, wParam, (LPARAM)pmhs);
}

LRESULT CTheater::_MsgHook(int nCode, WPARAM wParam, LPARAM lParam)
{
    CTheater* pTheater;
    if (SUCCEEDED(_al.Find(GetCurrentThreadId(), (LPVOID*)&pTheater)))
    {
        return pTheater->_OnMsgHook(nCode, wParam, (MOUSEHOOKSTRUCT*)lParam, FALSE);
    }
    return 0;
}

void CTheater::Begin()
{
    _ShowToolbar();    
    SetTimer(_hwndFloater, IDT_INITIAL, 1500, NULL);
}

void CTheater::_SizeBrowser()
{
    // position & size the browser window
    RECT rc;
    HMONITOR hMon = MonitorFromWindow(_hwndBrowser, MONITOR_DEFAULTTONEAREST);
    GetMonitorRect(hMon, &rc);
    InflateRect(&rc, CX_BROWOVERLAP, CX_BROWOVERLAP);
    SetWindowPos(_hwndBrowser, HWND_TOP, rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc), 0);
}

void CTheater::_SizeFloater()
{
    // position & size the floater
    RECT rc;
    GetWindowRect(_hwndBrowser, &rc);
    int x = RECTWIDTH(rc) - (CX_FLOATERSHOWN + CX_BROWOVERLAP);
    int y = 0;

    int cy = GetWindowHeight(_hwndToolbar);

    SetWindowPos(_hwndFloater, HWND_TOP, x, y, CX_FLOATERSHOWN, cy, SWP_NOACTIVATE);
}

void CTheater::_CreateCloseMinimize()
{
    _hwndClose = CreateWindowEx( WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, NULL,
                                WS_VISIBLE | WS_CHILD | 
                                TBSTYLE_TOOLTIPS | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT |
                                WS_CLIPCHILDREN | WS_CLIPSIBLINGS | 
                                CCS_NODIVIDER | CCS_NOPARENTALIGN |
                                CCS_NORESIZE,
                                0, 0,
                                CLOSEMIN_WIDTH, CLOSEMIN_HEIGHT,
                                _hwndFloater, 0, HINST_THISDLL, NULL);

    if (_hwndClose) {
        static const TBBUTTON tb[] =
        {
            { 0, SC_MINIMIZE, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0, 0 },
            { 1, SC_RESTORE, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0, 0 },
            { 2, SC_CLOSE, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0, 0 }
        };

        HIMAGELIST himl = ImageList_LoadImage(HINST_THISDLL,
                                              MAKEINTRESOURCE(IDB_THEATERCLOSE),
                                              10, 0, RGB(255,0,255),
                                              IMAGE_BITMAP, LR_CREATEDIBSECTION);
        ImageList_SetBkColor(himl, RGB(0,0,0));

        SendMessage(_hwndClose, TB_SETIMAGELIST, 0, (LPARAM)himl);
        SendMessage(_hwndClose, TB_BUTTONSTRUCTSIZE,    SIZEOF(TBBUTTON), 0);
        SendMessage(_hwndClose, TB_ADDBUTTONS, ARRAYSIZE(tb), (LPARAM)tb);
        SendMessage(_hwndClose, TB_SETMAXTEXTROWS,      0, 0L);
        TBBUTTONINFO tbbi;
        TCHAR szBuf[256];
        tbbi.cbSize = SIZEOF(TBBUTTONINFO);
        tbbi.dwMask = TBIF_TEXT;
        tbbi.pszText = szBuf;
        MLLoadString(IDS_CLOSE, szBuf, ARRAYSIZE(szBuf));
        SendMessage(_hwndClose, TB_SETBUTTONINFO, SC_CLOSE, (LPARAM)&tbbi);
        MLLoadString(IDS_RESTORE, szBuf, ARRAYSIZE(szBuf));
        SendMessage(_hwndClose, TB_SETBUTTONINFO, SC_RESTORE, (LPARAM)&tbbi);
        MLLoadString(IDS_MINIMIZE, szBuf, ARRAYSIZE(szBuf));
        SendMessage(_hwndClose, TB_SETBUTTONINFO, SC_MINIMIZE, (LPARAM)&tbbi);
    }    
}

void CTheater::_Initialize()
{
    _SizeBrowser();
    
#ifndef UNIX
    _hwndTaskbar = FindWindow(TEXT("Shell_TrayWnd"), NULL);

#ifdef DEBUG
    if (!_hwndTaskbar)
    {
        TraceMsg(TF_WARNING, "CTheater::_Initialize -- couldn't find taskbar window");
    }
#endif // DEBUG

#else
    _hwndTaskbar   = NULL;
#endif
    _fTaskbarShown = FALSE;

    _hwndFloater = SHCreateWorkerWindow(_FloaterWndProc, _hwndBrowser,  
#if defined(MAINWIN)
                                      // Removing window manager decors
                                      WS_EX_MW_UNMANAGED_WINDOW |
#endif
                                      WS_EX_TOOLWINDOW, 
                                      WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, NULL, this);
    if (_hwndFloater) {

        int cx = 0;        
        
        // create animating E logo
        IUnknown* punk;
        CBrandBand_CreateInstance(NULL, (IUnknown **)&punk, NULL);
        if (punk) {
            punk->QueryInterface(IID_IDeskBand, (LPVOID*)&_pdbBrand);
            if (_pdbBrand) {
                HWND hwndBrand;
                
                IUnknown_SetSite(_pdbBrand, SAFECAST(this, IOleWindow*));                                
                IUnknown_GetWindow(_pdbBrand, &hwndBrand);
                                
                ASSERT(hwndBrand);
#ifdef DEBUG
                // Make sure brand isn't too tall
                DESKBANDINFO dbi = {0};
                _pdbBrand->GetBandInfo(0, 0, &dbi);
                ASSERT(!(dbi.ptMinSize.y > BRAND_HEIGHT));
#endif
                if (hwndBrand) 
                {
                    SetWindowPos(hwndBrand, NULL, 
                        cx, BRAND_YOFFSET, BRAND_WIDTH, BRAND_HEIGHT,
                        SWP_NOZORDER | SWP_SHOWWINDOW);
                    cx += BRAND_WIDTH + CLOSEMIN_XOFFSET;                    
                }
                // get floater background color
                VARIANTARG var = {VT_I4};
                IUnknown_Exec(_pdbBrand, &CGID_PrivCITCommands, CITIDM_GETDEFAULTBRANDCOLOR, 0, NULL, &var);
                _clrBrandBk = (COLORREF) var.lVal;
            }
            punk->Release();
        }
        
        // now create the progress bar        
        _hwndProgress = CreateWindowEx(0, PROGRESS_CLASS, NULL,
                                       WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | PBS_SMOOTH,
                                       cx - 1, PROGRESS_YPOS, 
                                       PROGRESS_WIDTH, PROGRESS_HEIGHT,
                                       _hwndFloater, (HMENU)TMC_PROGRESSBAR,
                                       HINST_THISDLL, NULL);
        if (_hwndProgress)
        {
            SendMessage(_hwndProgress, PBM_SETBKCOLOR, 0, _clrBrandBk);
            SendMessage(_hwndProgress, PBM_SETBARCOLOR, 0, GetSysColor(COLOR_BTNSHADOW));

            // hack of the 3d client edge that WM_BORDER implies in dialogs
            // add the 1 pixel static edge that we really want
            SHSetWindowBits(_hwndProgress, GWL_EXSTYLE, WS_EX_STATICEDGE, 0);
            SetWindowPos(_hwndProgress, NULL, 0,0,0,0, 
                SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        }
        
        // make the close/minimize buttons & position them
        _CreateCloseMinimize();
        SetWindowPos(_hwndClose, HWND_TOP, cx, CLOSEMIN_YOFFSET, 0, 0, 
            SWP_NOSIZE | SWP_NOACTIVATE);

        _SizeFloater();
    }
}

void CTheater::_SwapParents(HWND hwndOldParent, HWND hwndNewParent)
{
    HWND hwnd = ::GetWindow(hwndOldParent, GW_CHILD);

    while (hwnd) {
        //
        //  Note that we must get the next sibling BEFORE we set the new
        // parent.
        //
        HWND hwndNext = ::GetWindow(hwnd, GW_HWNDNEXT);
        if (hwnd != _hwndToolbar) {
            ::SetParent(hwnd, hwndNewParent);
        }
        hwnd = hwndNext;
    }
}

//////  begin floating palette (floater) window implementation
///  
/// floater keeps a ref count of activation (via command target)
/// when the last activity goes away, it sets a timer and hides after a second.
/// this is a regional window that will host the animation icon, progress bar, and 
/// close / minimize buttons

ULONG CTheater::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CTheater::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CTheater::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CTheater, IOleWindow),
        QITABENT(CTheater, IOleCommandTarget),
        QITABENT(CTheater, IServiceProvider),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

HRESULT CTheater::GetWindow(HWND * lphwnd) 
{
    *lphwnd = _hwndFloater; 
    if (_hwndFloater)
        return S_OK; 
    return E_FAIL;
}

void CTheater::_SanityCheckZorder()
{
    //
    // The view may have jumped to HWND_TOP, so we need to
    // fix up the floater, toolbar, and browbar positions
    // within the z-order.
    //
    SetWindowZorder(_hwndFloater, HWND_TOP);
    SetWindowZorder(_hwndToolbar, _hwndFloater);
    SetWindowZorder(_hwndBrowBar, _hwndToolbar);
}

HRESULT CTheater::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, 
                                  OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    return OLECMDERR_E_UNKNOWNGROUP;
}

HRESULT CTheater::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, 
                           VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    HRESULT hres = OLECMDERR_E_NOTSUPPORTED;
    if (pguidCmdGroup == NULL)
    {
        // nothing
    }
    else if (IsEqualGUID(CGID_Theater, *pguidCmdGroup))
    {
        switch (nCmdID)
        {
        case THID_ACTIVATE:
            _cActiveRef++;
            if (_cActiveRef == 1)
                _ShowFloater();            
            break;
            
        case THID_DEACTIVATE:
            // we can get a deactivate before the first activate if 
            // we come up during a navigate
            if (_cActiveRef > 0)
            {
                _cActiveRef--;               
                _DelayHideFloater();
            }
            break;
            
        case THID_SETBROWSERBARAUTOHIDE:
            if (pvarargIn && pvarargIn->vt == VT_I4)
            {
                _fAutoHideBrowserBar = pvarargIn->lVal;
                if (!_fAutoHideBrowserBar)
                {
                    // clear initial hide anymore.  they are well aware of it if
                    // they hit this switch
                    _fInitialBrowserBar = FALSE;
                    _ShowBrowBar();
                }
            }            
            break;
            
        case THID_SETBROWSERBARWIDTH:
            if (pvarargIn && pvarargIn->vt == VT_I4)
                _cxBrowBarShown = pvarargIn->lVal;
            break;
            
        case THID_SETTOOLBARAUTOHIDE:
            if (pvarargIn && pvarargIn->vt == VT_I4)
            {
                _fAutoHideToolbar = pvarargIn->lVal;
                if (pvarargIn->lVal)
                    _HideToolbar();
                else                             
                    _ShowToolbar();                                    
            }
            break;         

        case THID_ONINTERNET:
            IUnknown_Exec(_pdbBrand, &CGID_PrivCITCommands, CITIDM_ONINTERNET, nCmdexecopt, pvarargIn, pvarargOut);
            break;
        }
    }
    else if (IsEqualGUID(CGID_Explorer, *pguidCmdGroup))
    {
        switch (nCmdID)
        {
        case SBCMDID_ONVIEWMOVETOTOP:
            _SanityCheckZorder();
            hres = S_OK;
            break;
        }
    }
    
    return hres;
}

void CTheater::_OnCommand(UINT idCmd)
{
    PostMessage(_hwndBrowser, WM_SYSCOMMAND, idCmd, 0);
}

LRESULT CTheater::_FloaterWndProc(HWND  hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CTheater *t = (CTheater*)GetWindowPtr0(hwnd);
    if (!t)
        return 0;

    switch (uMsg)
    {
    case WM_COMMAND:
        t->_OnCommand(GET_WM_COMMAND_ID(wParam, lParam));
        break;

    case WM_CLOSE:
    case WM_NOTIFY:
        return SendMessage(t->_hwndBrowser, uMsg, wParam, lParam);

    case WM_TIMER:
    {           
        switch (wParam) {

        case IDT_HIDEFLOATER1SEC:
            t->_HideFloater();
            break;                       

        case IDT_INITIALBROWSERBAR:
            // _fAutoHideBrowserBar may have changed after the timer was set
            if (t->_fAutoHideBrowserBar)
                t->_HideBrowBar();
            return 1;
            
        case IDT_INITIAL:
            {
                t->_HideToolbar();
                t->_hhook = SetWindowsHookEx(WH_MOUSE, _MsgHook, MLGetHinst(), GetCurrentThreadId());

                HWND hwndInsertAfter;
                if (t->_IsBrowserActive())
                {
                    // We're active, just move to non-topmost
                    hwndInsertAfter = HWND_NOTOPMOST;
                }
                else
                {
                    // Another window became active while we were
                    // moving to fullscreen mode; move ourselves below
                    // that window.  We need to walk up the parent chain
                    // so that if the window has a modal dialog open we
                    // won't insert ourselves between the dialog and the
                    // app.
                    hwndInsertAfter = GetForegroundWindow();
                    HWND hwnd;
                    while (hwnd = GetParent(hwndInsertAfter))
                    {
                        hwndInsertAfter = hwnd;
                    }
                }

                SetWindowZorder(t->_hwndBrowser, hwndInsertAfter);

                // Call the hook handler manually to insure that even if there's no mouse
                // movement the handler will get called once.  That way, bar unhiding will
                // still work if the user has already given up and stopped moving the mouse.

                t->_OnMsgHook(0, 0, NULL, TRUE);
            }

            break;
            
        case IDT_UNHIDE:
            switch (t->_iUnhidee)
            {
                case IDT_TOOLBAR:
                    t->_ShowToolbar();
                    break;
                
                case IDT_BROWBAR:
                    t->_ShowBrowBar();
                    break;
                
                case IDT_TASKBAR:
                    t->_ShowTaskbar();
                    break;

            }
            SetTimer(t->_hwndFloater, IDT_DELAY, LONG_DELAY, NULL);
            t->_fDelay = TRUE;            
            t->_iUnhidee = 0;
            break;

        case IDT_DELAY:
            t->_fDelay = FALSE;
            break;        
            
        case IDT_HIDETOOLBAR:
            t->_ContinueHideToolbar();
            break;
            
        case IDT_HIDEBROWBAR:
            t->_ContinueHideBrowBar();
            break;

        case IDT_HIDEFLOATER:
            t->_ContinueHideFloater();
            break;
        }
        KillTimer(hwnd, wParam);
        break;
    }

    case WM_SETTINGCHANGE:
        if (wParam == SPI_SETNONCLIENTMETRICS)
        {
            t->RecalcSizing();
        }
        break;

    case WM_ERASEBKGND:
    {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hwnd, &rc);
        SHFillRectClr(hdc, &rc, t->_clrBrandBk);        
        return 1;
    }        

    default:
        return ::DefWindowProcWrap(hwnd, uMsg, wParam, lParam);
    }        

    return 0;
}

void CTheater::RecalcSizing()
{
    _SizeBrowser();
    _SizeFloater();
}

HRESULT CTheater::QueryService(REFGUID guidService, REFIID riid, void **ppvObj)
{
    return IUnknown_QueryService(_punkOwner, guidService, riid, ppvObj);
}

HRESULT CTheater::SetBrowserBar(IUnknown* punk, int cxHidden, int cxExpanded)
{
    if (punk != _punkBrowBar) {
        IUnknown_Exec(_punkBrowBar, &CGID_Theater, THID_DEACTIVATE, 0, NULL, NULL);
        ATOMICRELEASE(_punkBrowBar);

        IUnknown_GetWindow(punk, &_hwndBrowBar);
        _punkBrowBar = punk;
        if (punk)
            punk->AddRef();
    
        if (_hwndBrowBar) 
        {
            _cxBrowBarShown = cxExpanded;
            
            // tell the browser bar to only ever request the hidden window size
            VARIANT var = { VT_I4 };
            var.lVal = _fAutoHideBrowserBar;
            IUnknown_Exec(_punkBrowBar, &CGID_Theater, THID_ACTIVATE, 0, &var, &var);
            _fAutoHideBrowserBar = var.lVal;            
        }
    } 

    if (punk) {
        _ShowBrowBar();                
   
        // if we're in autohide mode, the first hide should be slow
        if (_fAutoHideBrowserBar) {
            _fInitialBrowserBar = TRUE;        
            SetTimer(_hwndFloater, IDT_INITIALBROWSERBAR, 1000, NULL);
        }
    }
    return S_OK;
}

#endif /* !DISABLE_FULLSCREEN */
