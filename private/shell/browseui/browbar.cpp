// coming soon: new deskbar (old deskbar moved to browbar base class)

#include "priv.h"
#include "sccls.h"
#include "resource.h"
#include "browbs.h"
#include "browbar.h"
#include "theater.h"
#include "shbrows2.h"

#ifdef UNIX
#include <mainwin.h>
#endif

#define SUPERCLASS  CDockingBar

static const WCHAR c_szExplorerBars[]  = TEXT("Software\\Microsoft\\Internet Explorer\\Explorer Bars\\");

//***   CBrowserBar_CreateInstance --
//
STDAPI CBrowserBar_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory

    CBrowserBar *pwbar = new CBrowserBar();
    if (pwbar) {
        *ppunk = SAFECAST(pwbar, IDockingWindow*);
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

//***
// NOTES
// this creates the BrowserBar (infobar) and sets up it's specific style
// such as captionless rebar and such
HRESULT BrowserBar_Init(CBrowserBar* pdb, IUnknown** ppbs, int idBar)
{
    HRESULT hres;

    if (ppbs)
        *ppbs = NULL;
    
    CBrowserBandSite *pcbs = new CBrowserBandSite();
    if (pcbs)
    {

        IDeskBarClient *pdbc = SAFECAST(pcbs, IDeskBarClient*);

        BANDSITEINFO bsinfo;

        bsinfo.dwMask = BSIM_STYLE;
        bsinfo.dwStyle = BSIS_NOGRIPPER | BSIS_LEFTALIGN;

        pcbs->SetBandSiteInfo(&bsinfo);

        hres = pdb->SetClient(pdbc);
        if (SUCCEEDED(hres))
        {
            if (ppbs) {
                *ppbs = pdbc;
                pdbc->AddRef();
            }
        }
        pdbc->Release();

        ASSERT(idBar == IDBAR_VERTICAL || idBar == IDBAR_HORIZONTAL);
        pdb->SetIdBar(idBar);
    }
    else
    {
        hres = E_OUTOFMEMORY;
    }

    return hres;
}

//*** CBrowserBar::IPersistStream*::* {

HRESULT CBrowserBar::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_BrowserBar;
    return S_OK;
}

HRESULT CBrowserBar::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt,
                        VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    if (!pguidCmdGroup)
    {
        // nothing
    }
    else if (IsEqualGUID(CGID_Explorer, *pguidCmdGroup))
    {
        return IUnknown_Exec(_punkChild, pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
    }
    else if (IsEqualGUID(CGID_DeskBarClient, *pguidCmdGroup))
    {
        switch (nCmdID) {
        case DBCID_EMPTY:
            if (_ptbSite) {
                // if we have no bands left, hide
                VARIANT var = {VT_UNKNOWN};
                var.punkVal = SAFECAST(this, IDeskBar*);
                AddRef();

                _StopCurrentBand();
                
                IUnknown_Exec(_ptbSite, &CGID_Explorer, SBCMDID_TOOLBAREMPTY, nCmdexecopt, &var, NULL);
                VariantClearLazy(&var);
            }
            break;

        case DBCID_RESIZE:
            goto ForwardUp;
            break;

        case DBCID_CLSIDOFBAR:
            ASSERT(nCmdexecopt == 0 || nCmdexecopt == 1);
            
            if (nCmdexecopt == 0)
            {
                //bar is being hidden
                _StopCurrentBand();
                _clsidCurrentBand = GUID_NULL;
            }
            else if (pvarargIn && pvarargIn->vt == VT_BSTR)
            {
                CLSID clsidTemp;

                GUIDFromString(pvarargIn->bstrVal, &clsidTemp);

                //if given a clsid and it's a new one, save the old one's settings
                //then set it as the current clsid
                if (!IsEqualIID(clsidTemp, _clsidCurrentBand))
                {
                    _PersistState(_hwnd, TRUE);
                    _StopCurrentBand();
                }
                _clsidCurrentBand = clsidTemp;

                if (_hwnd && IsWindow(_hwnd))
                {
                    UINT uiNewWidthOrHeight = _PersistState(_hwnd, FALSE);
                    RECT rc = {0};

                    GetWindowRect(_hwnd, &rc);
        
                    if (_idBar == IDBAR_VERTICAL)
                        rc.right = rc.left + uiNewWidthOrHeight;
                    else
                        rc.top = rc.bottom - uiNewWidthOrHeight;
                    SetWindowPos(_hwnd, NULL, 0, 0, RECTWIDTH(rc), RECTHEIGHT(rc), SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
                }
            }
            else if (!pvarargIn && pvarargOut)
            {
                //we weren't given a clsid, caller must want one
                //can't be SA_BSTRGUID
                WCHAR wsz[GUIDSTR_MAX];

                StringFromGUID2(_clsidCurrentBand, wsz, ARRAYSIZE(wsz));

                pvarargOut->vt = VT_BSTR;
                pvarargOut->bstrVal = SysAllocString(wsz);
                if (!pvarargOut->bstrVal)
                {
                    pvarargOut->vt = VT_NULL;
                    return E_FAIL;
                }
            }
            else
                ASSERT(FALSE);
            break;
        }
        return S_OK;
    } 
    else if (IsEqualGUID(CGID_Theater, *pguidCmdGroup)) {
        switch (nCmdID) {
        case THID_ACTIVATE:
            // if we're on a small monitor, start off as autohide
            _fTheater = TRUE;
            ResizeBorderDW(NULL, NULL, FALSE);
            _OnSize();

            // pass back pin button's state
            pvarargOut->vt = VT_I4;
            pvarargOut->lVal = !_fNoAutoHide;

            break;

        case THID_DEACTIVATE:
            _fTheater = FALSE;
            // if we're on a small monitor, restore to theater default width
            _szChild.cx = _iTheaterWidth;
            _AdjustToChildSize();
            break;

        case THID_SETBROWSERBARAUTOHIDE:
            // pvarargIn->lVal contains new _fAutoHide.
            // fake message pin button was pressed only when _fNoAutoHide == pvarargIn->lVal
            // which means new _fNoAutoHide != old _fNoAutoHide
            if ((_fNoAutoHide && pvarargIn->lVal) || !(_fNoAutoHide || pvarargIn->lVal)) {
                // first update state and change bitmap
                _fNoAutoHide = !pvarargIn->lVal;

                // then notify theater mode manager because it owns the msg hook and does 
                // the hiding
                IUnknown_Exec(_ptbSite, &CGID_Theater, THID_SETBROWSERBARAUTOHIDE, 0, pvarargIn, NULL);

                // negotiate with the browser for space
                _Recalc();
                _PersistState(_hwnd, FALSE);
            }
            break;
                
        default:
            goto ForwardUp;
        }
        
        IUnknown_Exec(_punkChild, pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);

    }
    
ForwardUp:
    
    return SUPERCLASS::Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
}

#define ABS(i)  (((i) < 0) ? -(i) : (i))

void CBrowserBar::_HandleWindowPosChanging(LPWINDOWPOS pwp)
{
    if (_fDragging) {
        int cxMin = GetSystemMetrics(SM_CXVSCROLL) * 4;
        
        if (pwp->cx < cxMin)
            pwp->cx = cxMin;
    }
}

LRESULT CBrowserBar::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_NCHITTEST:
    {
        LRESULT lres = _OnNCHitTest(wParam, lParam);
#ifdef DEBUG
        // non-LHS bar useful for testing discussion bar etc. stuff
        // so allow drag to get it there
        if (0)
#endif
        {
            // don't allow drag in browbar
            if (lres == HTCAPTION)
                lres = HTCLIENT;
        }
        return lres;
    }
        
    case WM_ERASEBKGND:
        if (_fTheater) {
            HDC hdc = (HDC) wParam;
            RECT rc;
            GetClientRect(hwnd, &rc);
            SHFillRectClr(hdc, &rc, RGB(0,0,0));
            return 1;
        }
        break;
        
    case WM_EXITSIZEMOVE:
        {  // save explorer bar's new width to registry
            _PersistState(hwnd, TRUE);
        }
        break;

    case WM_SIZE:
        {
            // browser bandsite needs to hear about resizing
            LRESULT lres;
            _CheckForwardWinEvent(uMsg, wParam, lParam, &lres);
        }
        break;
    } 
    return SUPERCLASS::v_WndProc(hwnd, uMsg, wParam, lParam);
}

BOOL CBrowserBar::_CheckForwardWinEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres)
{
    HWND hwnd = NULL;

    switch (uMsg) {
    case WM_SIZE:
        {
            // HACKHACK: munge the size so that width is browbandsite's 
            // new width.  bbs needs to hear about resizing so that it
            // can reposition its close/autohide window.
            POINT pt = {LOWORD(lParam), HIWORD(lParam)};
            pt.x -= 4 * GetSystemMetrics(SM_CXEDGE);
            lParam = MAKELONG(pt.x, pt.y);
            hwnd = _hwndChild;
            break;
        }
    }

    if (hwnd && _pWEH && _pWEH->IsWindowOwner(hwnd) == S_OK) {
        _pWEH->OnWinEvent(_hwnd, uMsg, wParam, lParam, plres);
        return TRUE;
    }
    return SUPERCLASS::_CheckForwardWinEvent(uMsg, wParam, lParam, plres);
}

void CBrowserBar::_GetChildPos(LPRECT prc)
{
    GetClientRect(_hwnd, prc);
    if (_fTheater) 
        prc->right--;
    else 
    {
        // Make room for the resizing bar and make sure the right scrollbar is
        // tucked under the right edge of the parent if we are on the top or bottom
        switch(_uSide)
        {
            case ABE_TOP:
                prc->bottom -= GetSystemMetrics(SM_CYFRAME);
                prc->right += GetSystemMetrics(SM_CXFRAME);
                break;
            case ABE_BOTTOM:
                prc->top += GetSystemMetrics(SM_CYFRAME);
                prc->right += GetSystemMetrics(SM_CXFRAME);
                break;
            case ABE_LEFT:
                prc->right -= GetSystemMetrics(SM_CXFRAME);
                break;
            case ABE_RIGHT:
                prc->left += GetSystemMetrics(SM_CXFRAME);
                break;
        }
    }

    if (prc->left > prc->right)
        prc->right = prc->left;
    if (prc->top > prc->bottom)
        prc->bottom = prc->top;
}

void CBrowserBar::_GetStyleForMode(UINT eMode, LONG* plStyle, LONG *plExStyle, HWND* phwndParent)
{
    *plStyle = WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS;
    *plExStyle= 0;
    *phwndParent = PARENT_BBTMMOST();
}


// bSetNewRect controls whether we override the current rect or autohide setting
UINT CBrowserBar::_PersistState(HWND hwnd, BOOL bSetNewRect)
{
    BROWBARSAVE bbs = {0};
    RECT rc = {0};
    UINT retval = 0;

    if (IsEqualIID(_clsidCurrentBand, GUID_NULL))
    {
        //BUGBUG this assert is getting hit, why?
        //ASSERT(FALSE);  //do we even need to check this anymore?
        return 0;
    }

    // use current uiWidthOrHeight and fAutoHide in case there is no value in registry yet
    if (hwnd)
    {
        GetWindowRect(hwnd, &rc); // bad hack
        if (_idBar == IDBAR_VERTICAL)
            bbs.uiWidthOrHeight = RECTWIDTH(rc);
        else
            bbs.uiWidthOrHeight = RECTHEIGHT(rc);
    }
    bbs.fAutoHide = !_fNoAutoHide; 

    WCHAR wszClsid[GUIDSTR_MAX];
    DWORD dwType = REG_BINARY;
    DWORD cbSize = SIZEOF(BROWBARSAVE);
    SHStringFromGUID(_clsidCurrentBand, wszClsid, ARRAYSIZE(wszClsid));

    WCHAR wszKeyPath[MAX_PATH];
    StrCpyN(wszKeyPath, c_szExplorerBars, ARRAYSIZE(wszKeyPath));
    StrCatBuff(wszKeyPath, wszClsid, ARRAYSIZE(wszKeyPath));
    
    SHRegGetUSValueW(wszKeyPath, L"BarSize", &dwType, (LPBYTE)&bbs, &cbSize, FALSE, NULL, 0);

    //if there is no window yet and no saved size, pick a reasonable default
    if (bbs.uiWidthOrHeight == 0)
        bbs.uiWidthOrHeight = (IDBAR_VERTICAL == _idBar) ? COMMBAR_HEIGHT : INFOBAR_WIDTH;

    if (bSetNewRect)
    {
        if (_idBar == IDBAR_VERTICAL)
        {
            bbs.uiWidthOrHeight = RECTWIDTH(rc);
            retval = RECTWIDTH(rc);
        }
        else
        {
            bbs.uiWidthOrHeight = RECTHEIGHT(rc);
            retval = RECTHEIGHT(rc);
        }
    }        
    else
    {
        bbs.fAutoHide = !_fNoAutoHide;
        retval = bbs.uiWidthOrHeight;
    }

    if (bSetNewRect)
        SHRegSetUSValueW(wszKeyPath, L"BarSize", dwType, (LPBYTE)&bbs, cbSize, SHREGSET_FORCE_HKCU);

    return retval;
}

void CBrowserBar::_StopCurrentBand()
{
    //stop any streaming content or navigations, except for the search band if we stop it
    // then we could have incompletely loaded ui
    if (!IsEqualGUID(CLSID_SearchBand, _clsidCurrentBand))
    {
        IUnknown_Exec(_punkChild, NULL, OLECMDID_STOP, 0, NULL, NULL);
    }
}


// }
