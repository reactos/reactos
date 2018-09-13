#include "priv.h"
#include "theater.h"
#include "browbs.h"
#include "resource.h"
#include "tbmenu.h"

#include "mluisupp.h"

#define SUPERCLASS CBandSite

TCHAR GetAccelerator(LPCTSTR psz, BOOL bUseDefault);

#define ABS(x) (((x) < 0) ? -(x) : (x))
#define CX_TEXTOFFSET   6
#define CY_TEXTOFFSET   4
#define CX_TBOFFSET     1
#define CY_TBPADDING    1
#define CY_ETCH         2
#define CY_FLUFF        7


// *** IInputObject methods ***
HRESULT CBrowserBandSite::HasFocusIO()
{
    HWND hwnd = GetFocus();
    if (hwnd && (hwnd == _hwndTB || hwnd == _hwndCloseTB))
        return S_OK;
    else
        return SUPERCLASS::HasFocusIO();
}

// *** IDeskBarClient methods ***
HRESULT CBrowserBandSite::SetModeDBC(DWORD dwMode)
{
    if ((dwMode ^ _dwMode) & DBIF_VIEWMODE_VERTICAL) {
        // switching horizontal/vertical; need to toggle toolbar
        // since we hide toolbar for horizontal bars
        if (_hwndTBRebar) {
            if (dwMode & DBIF_VIEWMODE_VERTICAL) {
                ShowWindow(_hwndTBRebar, SW_SHOW);
                _fToolbar = _pCmdTarget ? TRUE : FALSE;
            } else {
                ShowWindow(_hwndTBRebar, SW_HIDE);
                _fToolbar = FALSE;
            }
        }
    }

    return SUPERCLASS::SetModeDBC(dwMode);
}

HRESULT CBrowserBandSite::TranslateAcceleratorIO(LPMSG lpMsg)
{
    HRESULT hr = S_FALSE;

    ASSERT((lpMsg->message >= WM_KEYFIRST) && (lpMsg->message <= WM_KEYLAST));

#if 0   //Disabled until a better key combination can be determined
    // check for Control-Shift arrow keys and resize if necessary
    if ((GetKeyState(VK_SHIFT) < 0)  && (GetKeyState(VK_CONTROL) < 0))
    {
        switch (lpMsg->wParam)
        {
            case VK_UP:
            case VK_DOWN:
            case VK_LEFT:
            case VK_RIGHT:
                IUnknown_Exec(_punkSite, &CGID_DeskBarClient, DBCID_RESIZE, (DWORD)lpMsg->wParam, NULL, NULL);
                return S_OK;
        }
    }
#endif

    //  Give toolbar a crack
    if (hr != S_OK && _hwndTB && SendMessage(_hwndTB, TB_TRANSLATEACCELERATOR, 0, (LPARAM)lpMsg))
        return S_OK;
    else if (hr != S_OK && SendMessage(_hwndCloseTB, TB_TRANSLATEACCELERATOR, 0, (LPARAM)lpMsg))
        return S_OK;

    if ((NULL != _hwndTB) && (NULL != _pCmdTarget) && 
        (WM_SYSCHAR == lpMsg->message))
    {
        UINT idBtn;
    
        if (SendMessage(_hwndTB, TB_MAPACCELERATOR, lpMsg->wParam, (LPARAM)&idBtn))
        {
            TCHAR szButtonText[MAX_PATH];

            //  comctl says this one is the one, let's make sure we aren't getting
            //  one of the unwanted "use the first letter" accelerators that it
            //  will return.
        
            if ((SendMessage(_hwndTB, TB_GETBUTTONTEXT, idBtn, (LPARAM)szButtonText) > 0) &&
                (GetAccelerator(szButtonText, FALSE) != (TCHAR)-1))
            {
                //  (tnoonan) - it feels kinda cheesy to send mouse messages, but 
                //  I don't know of a cleaner way which will accomplish what we
                //  want (like deal with split buttons, mutually exclusive 
                //  buttons, etc.).
            
                RECT rc;

                SendMessage(_hwndTB, TB_GETRECT, idBtn, (LPARAM)&rc);

                SendMessage(_hwndTB, WM_LBUTTONDOWN, MK_LBUTTON, MAKELONG(rc.left, rc.top));
                SendMessage(_hwndTB, WM_LBUTTONUP, 0, MAKELONG(rc.left, rc.top));

                hr = S_OK;
            }
        }
    }

    if (hr != S_OK)
        hr = SUPERCLASS::TranslateAcceleratorIO(lpMsg);

    return hr;
}

HRESULT CBrowserBandSite::_TrySetFocusTB(int iDir)
{
    HRESULT hres = S_FALSE;
    if (_hwndTB)
    {
        int cBtns = (int) SendMessage(_hwndTB, TB_BUTTONCOUNT, 0, 0);
        if (cBtns > 0)
        {
            // Set focus on tb.  This will also set the first button to hottracked,
            // generating a hot item change notify, but _OnHotItemChange will ignore
            // the notify as neither HICF_RESELECT, HICF_ARROWKEYS nor HICF_ACCELERATOR will be set.
            SetFocus(_hwndTB);

            // If going back, make rightmost button hottracked,
            // else make first button hottracked.
            int iHotPos = (iDir == -1) ? cBtns - 1 : 0;

            // Pass HICF_RESELECT so that if we're reselecting the same item, another notify
            // is generated, and so that the filter in _OnHotItemChange will let the notification
            // through (and pop down the chevron menu if necessary).
            SendMessage(_hwndTB, TB_SETHOTITEM2, iHotPos, HICF_RESELECT);

            hres = S_OK;
        }
    }
    return hres;
}

HRESULT CBrowserBandSite::_CycleFocusBS(LPMSG lpMsg)
{
    //
    // Tab order goes: (out)->_hwndCloseTB->bands->(out)
    //
    // The order is reversed when shift is pressed.
    // 
    // When control is pressed, and we have focus (i.e., have already been tabbed
    // into), we reject focus since ctl-tab is supposed to tab between contexts.
    //
    // Once _hwndCloseTB gets focus, user can arrow over to _hwndTB.  If
    // that happens, replace _hwndCloseTB with _hwndTB in order above.
    //

    BOOL fHasFocus = (HasFocusIO() == S_OK);
    ASSERT(fHasFocus || !_ptbActive);

    if (fHasFocus && IsVK_CtlTABCycler(lpMsg))
    {
        // Bail on ctl-tab if one of our guys already has focus
        return S_FALSE;
    }

    HWND hwnd = GetFocus();
    BOOL fHasTBFocus = (hwnd && (hwnd == _hwndTB || hwnd == _hwndCloseTB));
    BOOL fShift = (GetKeyState(VK_SHIFT) < 0);
    HRESULT hres = S_FALSE;

    if (fHasTBFocus)
    {
        if (!fShift)
            hres = SUPERCLASS::_CycleFocusBS(lpMsg);
    }
    else
    {
        // Here, since !fHasTBFocus, fHasFocus => a band has focus

        if (fHasFocus || fShift)
            hres = SUPERCLASS::_CycleFocusBS(lpMsg);

        if (hres != S_OK && (!fHasFocus || (fHasFocus && fShift)))
        {
            SetFocus(_hwndCloseTB);
            hres = S_OK;
        }
    }

    return hres;
}

// this class subclasses the CBandSite class and adds functionality specific to
// being hosted in the browser....
//
// it implements close as hide 
// it has its own title drawing

void CBrowserBandSite::_OnCloseBand(DWORD dwBandID)
{
    int iIndex = _BandIDToIndex(dwBandID);
    LPBANDITEMDATA pbid = _GetBandItem(iIndex);
    if (pbid)
    {
        _ShowBand(pbid, FALSE);
        if (_pct) 
        {
            BOOL fShowing = FALSE;

            for (int i = _GetBandItemCount() - 1; i >= 0; i--)
            {
                LPBANDITEMDATA pbid = _GetBandItem(i);
                if (pbid)
                {
                    fShowing |= pbid->fShow;
                }
            }    
            if (!fShowing)
            {
                _pct->Exec(&CGID_DeskBarClient, DBCID_EMPTY, 0, NULL, NULL);
            }
        }
    }
}


// don't allow d/d of any band in here
LRESULT CBrowserBandSite::_OnBeginDrag(NMREBAR* pnm)
{
    return 1;
}

CBrowserBandSite::CBrowserBandSite() : CBandSite(NULL)
{
    _dwBandIDCur = -1;
}

HFONT CBrowserBandSite::_GetTitleFont(BOOL fForceRefresh)
{
    if (_hfont && fForceRefresh)
        DeleteObject(_hfont);

    if (!_hfont || fForceRefresh) {
        // create our font to use for title & toolbar text
        // use A version for win9x compat
        NONCLIENTMETRICSA ncm;

        ncm.cbSize = sizeof(ncm);
        SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);

        if (!(_dwMode & DBIF_VIEWMODE_VERTICAL)) {
            // horizontal band w/ vertical caption, so rotate the font
            ncm.lfMenuFont.lfEscapement = 900;  // rotate by 90 degrees
            ncm.lfMenuFont.lfOutPrecision = OUT_TT_ONLY_PRECIS; // TT can be rotated
        }

        _hfont = CreateFontIndirectA(&ncm.lfMenuFont);
    }

    return _hfont;
}

void CBrowserBandSite::_InitLayout()
{
    // force update font
    _GetTitleFont(TRUE);
 
    // update toolbar font
    _UpdateToolbarFont();

    // recalc title and toolbar heights
    _CalcHeights();

    _UpdateLayout();
}

void CBrowserBandSite::_UpdateAllBands(BOOL fBSOnly, BOOL fNoAutoSize)
{
    if (!fBSOnly && !fNoAutoSize)
        _InitLayout();

    SUPERCLASS::_UpdateAllBands(fBSOnly, fNoAutoSize);
}

HRESULT CBrowserBandSite::_Initialize(HWND hwndParent)
{
    HRESULT hres = SUPERCLASS::_Initialize(hwndParent);
    SendMessage(_hwnd, CCM_SETUNICODEFORMAT, DLL_IS_UNICODE, 0);

    _CreateCloseTB();
    _InitLayout();

    return hres;
}

void CBrowserBandSite::_CalcHeights()
{
    // calc title height
    // HACKHACK: use height of 'All Folders' as standard title height
    TCHAR szTitle[64];

    if (MLLoadStringW(IDS_TREETITLE, szTitle, ARRAYSIZE(szTitle))) {
        HDC hdc = GetDC(_hwnd);

        HFONT hfont = _GetTitleFont(FALSE);
        HFONT hfontOld = (HFONT)SelectObject(hdc, hfont);

        int iLen = lstrlen(szTitle);

        SIZE size;
        GetTextExtentPoint32(hdc, szTitle, iLen, &size);
        _uTitle = size.cy;

        // make space for etch line + space
        _uTitle += CY_ETCH + CY_FLUFF;

        SelectObject(hdc, hfontOld);

        ReleaseDC(_hwnd, hdc);
    } else {
        // no string; use a default height
        _uTitle = BROWSERBAR_TITLEHEIGHT;
    }

    // calc toolbar height
    _uToolbar = _uTitle + (2 * CY_TBPADDING) + CY_ETCH;
}

void CBrowserBandSite::_UpdateToolbarFont()
{
    if (_hwndTB && (_dwMode & DBIF_VIEWMODE_VERTICAL)) {
        // use same font for title and toolbar
        HFONT hfont = _GetTitleFont(FALSE);
        if (hfont)
            SendMessage(_hwndTB, WM_SETFONT, (WPARAM)hfont, TRUE);
    }
}

void CBrowserBandSite::_ShowBand(LPBANDITEMDATA pbid, BOOL fShow)
{
    if (fShow && (_dwBandIDCur != pbid->dwBandID)) {
        _dwBandIDCur = pbid->dwBandID;
        _UpdateLayout();
    } else if (!fShow && _dwBandIDCur == pbid->dwBandID) {
        _dwBandIDCur = -1;
    }

    SUPERCLASS::_ShowBand(pbid, fShow);
}

void CBrowserBandSite::_UpdateLayout()
{
    // update toolbar button size
    if (_hwndTB)
    {
        // want 1 pixel of space between button and etchlines above and below
        LONG lSize = MAKELONG(0, _uToolbar - (CY_ETCH + 2 * CY_TBPADDING));
        SendMessage(_hwndTB, TB_SETBUTTONSIZE, 0, lSize);
    }

    // update header height for the current band
    if (_dwBandIDCur != -1)
    {
        REBARBANDINFO rbbi;
        rbbi.cbSize = SIZEOF(rbbi);
        rbbi.fMask = RBBIM_HEADERSIZE;
        rbbi.cxHeader = _uTitle + (_fToolbar ? _uToolbar : 0);
        SendMessage(_hwnd, RB_SETBANDINFO, _BandIDToIndex(_dwBandIDCur), (LPARAM)&rbbi);
    }

    // update toolbar size
    _UpdateToolbarBand();

    // reposition toolbars
    _PositionToolbars(NULL);
}

void CBrowserBandSite::_BandInfoFromBandItem(REBARBANDINFO *prbbi, LPBANDITEMDATA pbid, BOOL fBSOnly)
{
    SUPERCLASS::_BandInfoFromBandItem(prbbi, pbid, fBSOnly);

    if (prbbi) {
        // we override header width so we can fit browbs's fancy ui (title,
        // toolbar, close & autohide buttons) in the band's header area.
        prbbi->cxHeader = _uTitle + (_fToolbar ? _uToolbar : 0);
    }
}

void CBrowserBandSite::_DrawEtchline(HDC hdc, LPRECT prc, int iOffset, BOOL fVertEtch)
{
    RECT rc;
    CopyRect(&rc, prc);

    if (fVertEtch) {
        rc.left += iOffset - CY_ETCH;
        rc.right = rc.left + 1;
    } else {
        rc.top += iOffset - CY_ETCH;
        rc.bottom = rc.top + 1;
    }
    SHFillRectClr(hdc, &rc, GetSysColor(COLOR_BTNSHADOW));

    if (fVertEtch) {
        rc.left++;
        rc.right++;
    } else {
        rc.bottom++;
        rc.top++;
    }
    SHFillRectClr(hdc, &rc, GetSysColor(COLOR_BTNHILIGHT));
}

LRESULT CBrowserBandSite::_OnCDNotify(LPNMCUSTOMDRAW pnm)
{
    switch (pnm->dwDrawStage) {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;

    case CDDS_PREERASE:
        return CDRF_NOTIFYITEMDRAW;

    case CDDS_ITEMPREPAINT:
    {
        // horz bar has vert caption and vice versa
        BOOL fVertCaption = (_dwMode&DBIF_VIEWMODE_VERTICAL) ? FALSE:TRUE;
        
        LPBANDITEMDATA pbid = (LPBANDITEMDATA)pnm->lItemlParam;
        if (pbid) 
        {
            int iLen;
            HFONT hfont, hfontOld = NULL;
            LPCTSTR pszTitle;
            SIZE size;
            USES_CONVERSION;

            hfont = _GetTitleFont(FALSE);
            hfontOld = (HFONT)SelectObject(pnm->hdc, hfont);
            pszTitle = W2CT(pbid->szTitle);
            iLen = lstrlen(pszTitle);
            GetTextExtentPoint32(pnm->hdc, pszTitle, iLen, &size);

            // center text inside caption and draw edge at bottom/right.
            if (!fVertCaption) 
            {
                // vertical bar, has horizontal text
                int x = pnm->rc.left + CX_TEXTOFFSET;
                int y = pnm->rc.top + ((_uTitle - CY_ETCH) - size.cy) / 2;
                ExtTextOut(pnm->hdc, x, y, NULL, NULL, pszTitle, iLen, NULL);

                _DrawEtchline(pnm->hdc, &pnm->rc, RECTHEIGHT(pnm->rc), fVertCaption);
                if (_fToolbar)
                    _DrawEtchline(pnm->hdc, &pnm->rc, _uTitle, fVertCaption);
            }
            else 
            {
                // horizontal bar, has vertical text
                UINT nPrevAlign = SetTextAlign(pnm->hdc, TA_BOTTOM);
                int x = pnm->rc.right - ((_uTitle - CY_ETCH) - size.cy) / 2;
                int y = pnm->rc.bottom - CY_TEXTOFFSET;
                ExtTextOut(pnm->hdc, x, y, NULL, NULL, pszTitle, iLen, NULL);
                SetTextAlign(pnm->hdc, nPrevAlign);

                _DrawEtchline(pnm->hdc, &pnm->rc, RECTWIDTH(pnm->rc), fVertCaption);
                ASSERT(!_fToolbar);
            }

            if (hfontOld)
                SelectObject(pnm->hdc, hfontOld);
        }
    }
    return CDRF_SKIPDEFAULT;
    }
    return CDRF_DODEFAULT;
}

LRESULT CBrowserBandSite::_OnNotify(LPNMHDR pnm)
{
    switch (pnm->idFrom) {

    case FCIDM_REBAR:
        switch (pnm->code) {

        case NM_CUSTOMDRAW:
            return _OnCDNotify((LPNMCUSTOMDRAW)pnm);

        case NM_NCHITTEST:
            {
                NMMOUSE *pnmMouse = (NMMOUSE*)pnm;
                RECT rc;
                GetClientRect(_hwnd, &rc);
                if (pnmMouse->dwItemSpec == (DWORD)-1) {
Lchktrans:      
                    //
                    // Edges are mirrored if the window is mirrored. [samera]
                    //
                    if (IS_WINDOW_RTL_MIRRORED(_hwnd)) {
                        int iTmp = rc.right;
                        rc.right = rc.left;
                        rc.left  = iTmp;
                    }

                    // gotta check all 4 edges or non-left-side bars (e.g.
                    // commbar) won't work.
                    // (we separate this into 2 checks to give a trace,
                    // since the old code only checked the right side)
                    if (pnmMouse->pt.x > rc.right)  {
                        return HTTRANSPARENT;
                    }
                    if (pnmMouse->pt.x < rc.left ||
                        pnmMouse->pt.y > rc.bottom || pnmMouse->pt.y < rc.top) {
                        return HTTRANSPARENT;
                    }
                } else if (pnmMouse->dwHitInfo == RBHT_CLIENT) {
                    InflateRect(&rc, -(GetSystemMetrics(SM_CXFRAME)),
                        -(GetSystemMetrics(SM_CYFRAME)));
                    goto Lchktrans;
                }

                return SUPERCLASS::_OnNotify(pnm);
            }

        default:
            return SUPERCLASS::_OnNotify(pnm);
        }

    default:
        return SUPERCLASS::_OnNotify(pnm);
    }

    return 0;
}


IDropTarget* CBrowserBandSite::_WrapDropTargetForBand(IDropTarget* pdtBand)
{
    pdtBand->AddRef();
    return pdtBand;
}


HRESULT CBrowserBandSite::v_InternalQueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CBrowserBandSite, IExplorerToolbar),
        { 0 },
    };

    if (IsEqualIID(riid, IID_IDropTarget))
        return E_NOINTERFACE;

    HRESULT hres = QISearch(this, qit, riid, ppvObj);

    if (FAILED(hres))
        hres = SUPERCLASS::v_InternalQueryInterface(riid, ppvObj);

    return hres;
}


DWORD CBrowserBandSite::_GetWindowStyle(DWORD *pdwExStyle)
{
    *pdwExStyle = 0;
    return RBS_REGISTERDROP |
            RBS_VERTICALGRIPPER | 
            RBS_VARHEIGHT | RBS_DBLCLKTOGGLE |
            WS_VISIBLE |  WS_CHILD | WS_CLIPCHILDREN | WS_BORDER |
            WS_CLIPSIBLINGS | CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN;
}

// *** IOleCommandTarget ***

HRESULT CBrowserBandSite::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt,
                        VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    if (pguidCmdGroup == NULL) 
    {
        
    } 
#ifdef UNIX
    // IEUNIX: Special case to handle the case where the band wants to
    // close itself. Used in Cache Warning pane (msgband.cpp)
    else if (IsEqualGUID(CGID_Explorer, *pguidCmdGroup)) {
        switch (nCmdID) {
        case SBCMDID_MSGBAND: {
            IDockingWindow * pdw;
            if(SUCCEEDED(_punkSite->QueryInterface(IID_IDockingWindow, (LPVOID*)&pdw))) {
                pdw->ShowDW((BOOL)nCmdexecopt);
                pdw->Release();
                }
            }
        }
    }
#endif
    else if (IsEqualGUID(CGID_Theater, *pguidCmdGroup)) {
        switch (nCmdID) {
        case THID_ACTIVATE:
            _fTheater = TRUE;
            SHSetWindowBits(_hwnd, GWL_EXSTYLE, WS_EX_CLIENTEDGE, 0);
            // fall through
        case THID_SETBROWSERBARAUTOHIDE:
            if (pvarargIn && pvarargIn->vt == VT_I4)
                _fNoAutoHide = !(pvarargIn->lVal);
            SendMessage(_hwndCloseTB, TB_CHANGEBITMAP, IDM_AB_AUTOHIDE, _fNoAutoHide ? 2 : 0);
            break;

        case THID_DEACTIVATE:
            _fTheater = FALSE;
            SHSetWindowBits(_hwnd, GWL_EXSTYLE, WS_EX_CLIENTEDGE, WS_EX_CLIENTEDGE);
            break;
        }
        SetWindowPos(_hwnd, NULL, 0,0,0,0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
        _SizeCloseTB();

        return S_OK;
    }

    return SUPERCLASS::Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
}

#define BBSC_REBAR      0x00000001
#define BBSC_TOOLBAR    0x00000002

void CBrowserBandSite::_CreateTBRebar()
{
    ASSERT(!_hwndTBRebar);

    _hwndTBRebar = CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, NULL,
                           WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN |
                           WS_CLIPSIBLINGS | CCS_NODIVIDER | CCS_NOPARENTALIGN,
                           0, 0, 100, 36,
                           _hwnd, (HMENU) BBSC_REBAR, HINST_THISDLL, NULL);

    if (_hwndTBRebar)
        SendMessage(_hwndTBRebar, CCM_SETVERSION, COMCTL32_VERSION, 0);
}

void CBrowserBandSite::_InsertToolbarBand()
{
    if (_hwndTBRebar && _hwndTB)
    {
        // Assert that we haven't added the toolbar band yet
        ASSERT(SendMessage(_hwndTBRebar, RB_GETBANDCOUNT, 0, 0) == 0);

        // Assert that we've calculated toolbar height
        ASSERT(_uToolbar);

        REBARBANDINFO rbbi;
        rbbi.cbSize = SIZEOF(REBARBANDINFO);
        rbbi.fMask = RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE;

        // RBBIM_CHILD
        rbbi.hwndChild = _hwndTB;

        // RBBIM_CHILDSIZE
        rbbi.cxMinChild = 0;
        rbbi.cyMinChild = _uToolbar - (CY_ETCH + 2 * CY_TBPADDING);

        // RBBIM_STYLE
        rbbi.fStyle = RBBS_NOGRIPPER | RBBS_USECHEVRON;

        SendMessage(_hwndTBRebar, RB_INSERTBAND, -1, (LPARAM)&rbbi);
    }
}

void CBrowserBandSite::_UpdateToolbarBand()
{
    if (_hwndTBRebar && _hwndTB)
    {
        // Assert that we've added the toolbar band
        ASSERT(SendMessage(_hwndTBRebar, RB_GETBANDCOUNT, 0, 0) == 1);

        // Assert that we've calculated toolbar height
        ASSERT(_uToolbar);

        REBARBANDINFO rbbi;
        rbbi.cbSize = SIZEOF(REBARBANDINFO);
        rbbi.fMask = RBBIM_CHILDSIZE;

        SIZE size = {0, _uToolbar};
        if (SendMessage(_hwndTB, TB_GETIDEALSIZE, FALSE, (LPARAM)&size))
        {
            // RBBIM_IDEALSIZE
            rbbi.fMask |= RBBIM_IDEALSIZE;
            rbbi.cxIdeal = size.cx;
        }

        // RBBIM_CHILDSIZE
        rbbi.cxMinChild = 0;
        rbbi.cyMinChild = _uToolbar - (CY_ETCH + 2 * CY_TBPADDING);

        SendMessage(_hwndTBRebar, RB_SETBANDINFO, 0, (LPARAM)&rbbi);
    }
}

void CBrowserBandSite::_CreateTB()
{
    ASSERT(!_hwndTB);

    // Create a rebar too so we get the chevron
    _CreateTBRebar();

    if (_hwndTBRebar)
    {
        _hwndTB = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
                        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TBSTYLE_TOOLTIPS |
                        TBSTYLE_FLAT | TBSTYLE_LIST | CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE,
                        0, 0, 0, 0,
                        _hwndTBRebar, (HMENU) BBSC_TOOLBAR, HINST_THISDLL, NULL);
    }

    if (_hwndTB)
    {
        SendMessage(_hwndTB, TB_BUTTONSTRUCTSIZE, SIZEOF(TBBUTTON), 0);
        SendMessage(_hwndTB, CCM_SETVERSION, COMCTL32_VERSION, 0);

        // BUGBUG: use TBSTYLE_EX_HIDECLIPPEDBUTTONS here?  looks kinda goofy so i'm leaving it out for now.
        SendMessage(_hwndTB, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_MIXEDBUTTONS);

        SendMessage(_hwndTB, TB_SETMAXTEXTROWS, 1, 0L);

        _UpdateToolbarFont();

        _InsertToolbarBand();
    }
}

void CBrowserBandSite::_RemoveAllButtons()
{
    if (!_hwndTB || !_hwndTBRebar)
        return;

    ShowWindow(_hwndTBRebar, SW_HIDE);
    _fToolbar = FALSE;

    INT_PTR nCount = SendMessage(_hwndTB, TB_BUTTONCOUNT, 0, 0L);
    while (nCount-- > 0)
        SendMessage(_hwndTB, TB_DELETEBUTTON, nCount, 0L);

    _UpdateLayout();
}

void CBrowserBandSite::_Close()
{
    ATOMICRELEASE(_pCmdTarget);

    //
    // Destroying _hwndTBRebar will take care of _hwndTB too
    //
    ASSERT(!_hwndTB || IsChild(_hwndTBRebar, _hwndTB));

    DESTROY_OBJ_WITH_HANDLE(_hwndTBRebar, DestroyWindow);
    DESTROY_OBJ_WITH_HANDLE(_hwndCloseTB, DestroyWindow);

    DESTROY_OBJ_WITH_HANDLE(_hfont, DeleteObject);

    SUPERCLASS::_Close();
}

LRESULT CBrowserBandSite::_OnHotItemChange(LPNMTBHOTITEM pnmtb)
{
    LRESULT lres = 0;

    // We might want to drop down the chevron menu if the hot item change
    // flags has these characteristics:
    //
    //  - not HICF_LEAVING, since if HICF_LEAVING, the hot item should instead wrap to _hwndClose
    //  - and not HICF_MOUSE, since we only drop down on keyboard hot item change
    //  - HICF_ACCELERATOR | HICF_ARROWKEYS, since we only drop down on keyboard hot item change
    //  - or HICF_RESELECT, since we force a reselect in _TrySetFocusTB
    //
    if (!(pnmtb->dwFlags & (HICF_LEAVING | HICF_MOUSE)) &&
        (pnmtb->dwFlags & (HICF_RESELECT | HICF_ACCELERATOR | HICF_ARROWKEYS)))
    {
        // Check to see if new hot button is clipped.  If it is,
        // then we pop down the chevron menu.
        RECT rc;
        GetClientRect(_hwndTB, &rc);

        int iButton = (int)SendMessage(_hwndTB, TB_COMMANDTOINDEX, pnmtb->idNew, 0);

        if (SHIsButtonObscured(_hwndTB, &rc, iButton))
        {
            // Clear hot item
            SendMessage(_hwndTB, TB_SETHOTITEM, -1, 0);

            // Figure out whether to highlight first or last item in menu
            UINT uSelect;
            int cButtons = (int)SendMessage(_hwndTB, TB_BUTTONCOUNT, 0, 0);
            if (iButton == cButtons - 1)
                uSelect = DBPC_SELECTLAST;
            else
                uSelect = DBPC_SELECTFIRST;

            // Pop it down
            SendMessage(_hwndTBRebar, RB_PUSHCHEVRON, 0, uSelect);

            lres = 1;
        }
    }

    return lres;
}

LRESULT CBrowserBandSite::_OnNotifyBBS(LPNMHDR pnm)
{
    switch (pnm->code)
    {
    case TBN_DROPDOWN:
        if (EVAL(_pCmdTarget))
        {
            LPNMTOOLBAR pnmtoolbar = (LPNMTOOLBAR)pnm;
            VARIANTARG  var;
            RECT rc = pnmtoolbar->rcButton;
            
            var.vt = VT_I4;
            MapWindowPoints(_hwndTB, HWND_DESKTOP, (LPPOINT)&rc, 2);
            var.lVal = MAKELONG(rc.left, rc.bottom);
            
            _pCmdTarget->Exec(&_guidButtonGroup, pnmtoolbar->iItem, OLECMDEXECOPT_PROMPTUSER, &var, NULL);
        }
        break;

    case TBN_WRAPHOTITEM:
        {
            LPNMTBWRAPHOTITEM pnmwh = (LPNMTBWRAPHOTITEM) pnm;

            if (pnmwh->nReason & HICF_ARROWKEYS) {
                if (pnm->hwndFrom == _hwndCloseTB) {
                    if (_TrySetFocusTB(pnmwh->iDir) != S_OK)
                        return 0;
                } else {
                    ASSERT(pnm->hwndFrom == _hwndTB);
                    SetFocus(_hwndCloseTB);
                }
                return 1;
            }
        }
        break;

    case TBN_HOTITEMCHANGE:
        if (pnm->hwndFrom == _hwndTB)
            return _OnHotItemChange((LPNMTBHOTITEM)pnm);
        break;

    case TBN_GETINFOTIP:
        //  [scotthan] We'll ask the toolbar owner for tip text via
        //  IOleCommandTarget::QueryStatus, like we do w/ defview for itbar buttons
        if (_pCmdTarget && pnm->hwndFrom == _hwndTB)
        {
            NMTBGETINFOTIP* pgit = (NMTBGETINFOTIP*)pnm ;

            OLECMDTEXTV<MAX_TOOLTIP_STRING> cmdtv;
            OLECMDTEXT *pcmdText = &cmdtv;
 
            pcmdText->cwBuf    = MAX_TOOLTIP_STRING;
            pcmdText->cmdtextf = OLECMDTEXTF_NAME;
            pcmdText->cwActual = 0;
 
            OLECMD rgcmd = {pgit->iItem, 0};
 
            HRESULT hr = _pCmdTarget->QueryStatus(&_guidButtonGroup, 1, &rgcmd, pcmdText);
            if (SUCCEEDED(hr) && (pcmdText->cwActual))
            {
                SHUnicodeToTChar(pcmdText->rgwz, pgit->pszText, pgit->cchTextMax);
                return 1;
            }
        }
        break ;

    case RBN_CHEVRONPUSHED:
        LPNMREBARCHEVRON pnmch = (LPNMREBARCHEVRON) pnm;

        MapWindowPoints(pnmch->hdr.hwndFrom, HWND_DESKTOP, (LPPOINT)&pnmch->rc, 2);
        ToolbarMenu_Popup(_hwnd, &pnmch->rc, NULL, _hwndTB, 0, (DWORD)pnmch->lParamNM);

        return 1;
    }

    return 0;
}

// *** IWinEventHandler ***
HRESULT CBrowserBandSite::OnWinEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres)
{
    switch (uMsg)
    {
    case WM_COMMAND:
        {
            HWND hwndControl = GET_WM_COMMAND_HWND(wParam, lParam);
            UINT idCmd = GET_WM_COMMAND_ID(wParam, lParam);

            if (hwndControl && hwndControl == _hwndTB)
            {
                if (EVAL(_pCmdTarget))
                {
                    RECT rc;
                    VARIANTARG var;

                    var.vt = VT_I4;
                    SendMessage(_hwndTB, TB_GETRECT, idCmd, (LPARAM)&rc);
                    MapWindowPoints(_hwndTB, HWND_DESKTOP, (LPPOINT)&rc, 2);
                    var.lVal = MAKELONG(rc.left, rc.bottom);

                    _pCmdTarget->Exec(&_guidButtonGroup, idCmd, 0, &var, NULL);
                }
                return S_OK;
            }
            else if (hwndControl == _hwndCloseTB) {
                switch (idCmd) {
                case IDM_AB_CLOSE:
                    IUnknown_Exec(_punkSite, &CGID_DeskBarClient, DBCID_EMPTY, 0, NULL, NULL);
                    break;

                case IDM_AB_AUTOHIDE:
                    { 
                        VARIANTARG v = {0};
                        v.vt = VT_I4;
                        v.lVal = _fNoAutoHide;
                        IUnknown_Exec(_punkSite, &CGID_Theater, THID_SETBROWSERBARAUTOHIDE, 0, &v, NULL);

                        break;
                    }
                }
                return S_OK;
            }
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnm = (LPNMHDR)lParam;
            if (pnm && (pnm->hwndFrom == _hwndTB || pnm->hwndFrom == _hwndCloseTB || pnm->hwndFrom == _hwndTBRebar)) {
                *plres = _OnNotifyBBS(pnm);
                return S_OK;
            }
        }
        break;

    case WM_SIZE:
        {
            POINT pt = {LOWORD(lParam), HIWORD(lParam)};
            _PositionToolbars(&pt);
        }
        break;
    }

    return SUPERCLASS::OnWinEvent(hwnd, uMsg, wParam, lParam, plres);
}

HRESULT CBrowserBandSite::IsWindowOwner(HWND hwnd)
{
    if (hwnd && (hwnd == _hwndTB) || (hwnd == _hwndCloseTB) || (hwnd == _hwndTBRebar))
        return S_OK;

    return SUPERCLASS::IsWindowOwner(hwnd);
}

// *** IBandSite ***
HRESULT CBrowserBandSite::SetBandSiteInfo(const BANDSITEINFO * pbsinfo)
{
    // recompute our layout if vertical viewmode is changing
    BOOL fUpdate = ((pbsinfo->dwMask & BSIM_STATE) && 
                    ((pbsinfo->dwState ^ _dwMode) & DBIF_VIEWMODE_VERTICAL));

    HRESULT hres = SUPERCLASS::SetBandSiteInfo(pbsinfo);

    if (fUpdate) {
        _InitLayout();
    }

    return hres;
}


// *** IExplorerToolbar ***
HRESULT CBrowserBandSite::SetCommandTarget(IUnknown* punkCmdTarget, const GUID* pguidButtonGroup, DWORD dwFlags)
{
    HRESULT hres = S_OK;
    BOOL fRemoveButtons = TRUE;

    // dwFlags is not used
    ASSERT(!(dwFlags));

    ATOMICRELEASE(_pCmdTarget);
    if (punkCmdTarget && pguidButtonGroup)
    {
        hres = punkCmdTarget->QueryInterface(IID_IOleCommandTarget, (void**)&(_pCmdTarget));

        if (!_hwndTB)
        {
            _CreateTB();
        }
        else if (_fToolbar && IsEqualGUID(_guidButtonGroup, *pguidButtonGroup))
        {
            fRemoveButtons = FALSE;
            hres = S_FALSE;
        }

        _guidButtonGroup = *pguidButtonGroup;
    }
    else
        ASSERT(!punkCmdTarget);

    if (fRemoveButtons)
        _RemoveAllButtons();

    ASSERT(SUCCEEDED(hres));
    return hres;
}

// client should have already called AddString
HRESULT CBrowserBandSite::AddButtons(const GUID* pguidButtonGroup, UINT nButtons, const TBBUTTON* lpButtons)
{
    if (!_hwndTB || !nButtons)
        return E_FAIL;

    _RemoveAllButtons();

    if (SendMessage(_hwndTB, TB_ADDBUTTONS, nButtons, (LPARAM)lpButtons))
    {
        ShowWindow(_hwndTBRebar, SW_SHOW);
        _fToolbar = TRUE;

        _UpdateLayout();

        return S_OK;
    }

    return E_FAIL;
}

HRESULT CBrowserBandSite::AddString(const GUID* pguidButtonGroup, HINSTANCE hInst, UINT_PTR uiResID, LRESULT* pOffset)
{
    *pOffset = -1;
    if (!_hwndTB)
        return E_FAIL;

    *pOffset = SendMessage(_hwndTB, TB_ADDSTRING, (WPARAM)hInst, (LPARAM)uiResID);

    if (*pOffset != -1)
        return S_OK;

    return E_FAIL;
}

HRESULT CBrowserBandSite::GetButton(const GUID* pguidButtonGroup, UINT uiCommand, LPTBBUTTON lpButton)
{
    if (!_hwndTB)
        return E_FAIL;

    UINT_PTR uiIndex = SendMessage(_hwndTB, TB_COMMANDTOINDEX, uiCommand, 0L);
    if (SendMessage(_hwndTB, TB_GETBUTTON, uiIndex, (LPARAM)lpButton))
        return S_OK;

    return E_FAIL;
}

HRESULT CBrowserBandSite::GetState(const GUID* pguidButtonGroup, UINT uiCommand, UINT* pfState)
{
    if (!_hwndTB)
        return E_FAIL;

    *pfState = (UINT)SendMessage(_hwndTB, TB_GETSTATE, uiCommand, 0L);
    return S_OK;
}

HRESULT CBrowserBandSite::SetState(const GUID* pguidButtonGroup, UINT uiCommand, UINT fState)
{
    if (!_hwndTB)
        return E_FAIL;

    UINT_PTR uiState = SendMessage(_hwndTB, TB_GETSTATE, uiCommand, NULL);
    uiState ^= fState;
    if (uiState)
        SendMessage(_hwndTB, TB_SETSTATE, uiCommand, (LPARAM)fState);

    return S_OK;
}

HRESULT CBrowserBandSite::SetImageList( const GUID* pguidCmdGroup, HIMAGELIST himlNormal, HIMAGELIST himlHot, HIMAGELIST himlDisabled)
{
    if (IsEqualGUID(*pguidCmdGroup, _guidButtonGroup)) {
        SendMessage(_hwndTB, TB_SETIMAGELIST, 0, (LPARAM)himlNormal);
        SendMessage(_hwndTB, TB_SETHOTIMAGELIST, 0, (LPARAM)himlHot);
        SendMessage(_hwndTB, TB_SETDISABLEDIMAGELIST, 0, (LPARAM)himlDisabled);
    }
    return S_OK;
};

BYTE TBStateFromIndex(HWND hwnd, int iIndex)
{
    TBBUTTONINFO tbbi;
    tbbi.cbSize = SIZEOF(TBBUTTONINFO);
    tbbi.dwMask = TBIF_BYINDEX | TBIF_STATE;
    tbbi.fsState = 0;
    SendMessage(hwnd, TB_GETBUTTONINFO, iIndex, (LPARAM)&tbbi);

    return tbbi.fsState;
}

int CBrowserBandSite::_ContextMenuHittest(LPARAM lParam, POINT* ppt)
{
    if (lParam == (LPARAM)-1)
    {
        //
        // Keyboard activation.  If one of our toolbars has
        // focus, and it has a hottracked button, put up the
        // context menu below that button.
        //
        HWND hwnd = GetFocus();
        if (hwnd && (hwnd == _hwndTB || hwnd == _hwndCloseTB))
        {
            INT_PTR iHot = SendMessage(hwnd, TB_GETHOTITEM, 0, 0);
            if (iHot == -1)
            {
                // couldn't find a hot item, just use the first visible button
                iHot = 0;
                while (TBSTATE_HIDDEN & TBStateFromIndex(hwnd, (int)iHot))
                    iHot++;

                ASSERT(iHot < SendMessage(hwnd, TB_BUTTONCOUNT, 0, 0));
            }

            RECT rc;
            SendMessage(hwnd, TB_GETITEMRECT, iHot, (LPARAM)&rc);

            ppt->x = rc.left;
            ppt->y = rc.bottom;

            MapWindowPoints(hwnd, HWND_DESKTOP, ppt, 1);

            return -1;
        }
    }

    return SUPERCLASS::_ContextMenuHittest(lParam, ppt);
}

HMENU CBrowserBandSite::_LoadContextMenu()
{
    HMENU hmenu = SUPERCLASS::_LoadContextMenu();
    DeleteMenu(hmenu, BSIDM_SHOWTITLEBAND, MF_BYCOMMAND);
    return hmenu;
}

#define TBCLOSE_Y   1

// create the close/hide buttons
void CBrowserBandSite::_CreateCloseTB()
{
    _hwndCloseTB = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
                                WS_VISIBLE | 
                                WS_CHILD | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS |
                                WS_CLIPCHILDREN |
                                WS_CLIPSIBLINGS | CCS_NODIVIDER | CCS_NOMOVEY | CCS_NOPARENTALIGN |
                                CCS_NORESIZE,
                                0, TBCLOSE_Y, 30, 18, _hwnd, 0, HINST_THISDLL, NULL);

    if (_hwndCloseTB)
    {
        static const TBBUTTON c_tb[] =
        {
            { 0, IDM_AB_AUTOHIDE, TBSTATE_ENABLED, TBSTYLE_CHECK, {0,0}, 0, 0 },
            { 1, IDM_AB_CLOSE, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0, 1 }
        };

        SendMessage(_hwndCloseTB, TB_BUTTONSTRUCTSIZE,    SIZEOF(TBBUTTON), 0);
        SendMessage(_hwndCloseTB, CCM_SETVERSION, COMCTL32_VERSION, 0);

        SendMessage(_hwndCloseTB, TB_SETBITMAPSIZE, 0, (LPARAM) MAKELONG(13, 11));
        TBADDBITMAP tbab = { HINST_THISDLL, IDB_BROWSERTOOLBAR };
        SendMessage(_hwndCloseTB, TB_ADDBITMAP, 3, (LPARAM)&tbab);

        LONG_PTR cbOffset = SendMessage(_hwndCloseTB, TB_ADDSTRING, (WPARAM)MLGetHinst(), (LPARAM)IDS_BANDSITE_CLOSE_LABELS);
        TBBUTTON tb[ARRAYSIZE(c_tb)];
        UpdateButtonArray(tb, c_tb, ARRAYSIZE(c_tb), cbOffset);

        SendMessage(_hwndCloseTB, TB_SETMAXTEXTROWS, 0, 0);

        SendMessage(_hwndCloseTB, TB_ADDBUTTONS, ARRAYSIZE(tb), (LPARAM)tb);

        SendMessage(_hwndCloseTB, TB_SETINDENT, (WPARAM)0, 0);
        
        _SizeCloseTB();
    }    
}

void CBrowserBandSite::_PositionToolbars(LPPOINT ppt)
{
    RECT rc;

    if (ppt) {
        rc.left = 0;
        rc.right = ppt->x;
    } else {
        GetClientRect(_hwnd, &rc);
    }

    if (_hwndCloseTB) {
        // always put the close restore at the top right of the floater window
        int x;

        if (_dwMode & DBIF_VIEWMODE_VERTICAL) {
            RECT rcTB;
            GetWindowRect(_hwndCloseTB, &rcTB);
            x = rc.right - RECTWIDTH(rcTB) - 1;
        } else {
            x = rc.left;
        }
        SetWindowPos(_hwndCloseTB, HWND_TOP, x, TBCLOSE_Y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
    }

    if (_hwndTBRebar) {
        if (_fToolbar) {
            // toolbar goes on its own line below title
            SetWindowPos(_hwndTBRebar, HWND_TOP, 
                            rc.left + CX_TBOFFSET,
                            _uTitle + CX_TBOFFSET,
                            rc.right - 2 * CX_TBOFFSET,
                            _uToolbar,
                            SWP_SHOWWINDOW);

        } else {
            ASSERT(!IsWindowVisible(_hwndTBRebar));
        }
    }
}

// sets the size of the toolbar.  if we're in theate mode, we need to show the pushpin.
// otherwise just show theclose
void CBrowserBandSite::_SizeCloseTB()
{
    RECT rc;
    GetWindowRect(_hwndCloseTB, &rc);
    LRESULT lButtonSize = SendMessage(_hwndCloseTB, TB_GETBUTTONSIZE, 0, 0L);
    SetWindowPos(_hwndCloseTB, NULL, 0, 0, LOWORD(lButtonSize) * (_fTheater ? 2 : 1),
                 RECTHEIGHT(rc), SWP_NOMOVE | SWP_NOACTIVATE);

    DWORD_PTR dwState = SendMessage(_hwndCloseTB, TB_GETSTATE, IDM_AB_AUTOHIDE, 0);
    dwState &= ~(TBSTATE_HIDDEN | TBSTATE_CHECKED);
    if (!_fTheater)
        dwState |= TBSTATE_HIDDEN;
    if (_fNoAutoHide)
        dwState |= TBSTATE_CHECKED;
    SendMessage(_hwndCloseTB, TB_SETSTATE, IDM_AB_AUTOHIDE, dwState);
    _PositionToolbars(NULL);
}
