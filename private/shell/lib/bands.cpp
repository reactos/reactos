#include "bands.h"

#define DM_PERSIST      0           // trace IPS::Load, ::Save, etc.
#define DM_MENU         0           // menu code
#define DM_FOCUS        0           // focus
#define DM_FOCUS2       0           // like DM_FOCUS, but verbose

//=================================================================
// Implementation of CToolBand
//=================================================================

ULONG CToolBand::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CToolBand::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CToolBand::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] =
    {
        QITABENT(CToolBand, IDeskBand),         // IID_IDeskBand
        QITABENTMULTI(CToolBand, IOleWindow, IDeskBand),        // IID_IOleWindod
        QITABENTMULTI(CToolBand, IDockingWindow, IDeskBand),    // IID_IDockingWindow
        QITABENT(CToolBand, IInputObject),      // IID_IInputObject
        QITABENT(CToolBand, IOleCommandTarget), // IID_IOleCommandTarget
        QITABENT(CToolBand, IServiceProvider),  // IID_IServiceProvider
        QITABENT(CToolBand, IPersistStream),    // IID_IPersistStream
        QITABENTMULTI(CToolBand, IPersist, IPersistStream),     // IID_IPersist
        QITABENT(CToolBand, IObjectWithSite),   // IID_IObjectWithSite
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

//  *** IOleCommandTarget methods ***

HRESULT CToolBand::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    return E_NOTIMPL;
}

HRESULT CToolBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    return E_NOTIMPL;
}

//  *** IServiceProvider methods ***

HRESULT CToolBand::QueryService(REFGUID guidService,
                                  REFIID riid, void **ppvObj)
{
    return IUnknown_QueryService(_punkSite, guidService, riid, ppvObj);
}

//  *** IOleWindow methods ***

HRESULT CToolBand::GetWindow(HWND * lphwnd)
{
    *lphwnd = _hwnd;

    if (*lphwnd)
        return S_OK;

    return E_FAIL;
}

//  *** IInputObject methods ***

HRESULT CToolBand::TranslateAcceleratorIO(LPMSG lpMsg)
{
    return E_NOTIMPL;
}

HRESULT CToolBand::HasFocusIO()
{
    HRESULT hres;
    HWND hwndFocus = GetFocus();

    hres = SHIsChildOrSelf(_hwnd, hwndFocus);
    ASSERT(hwndFocus != NULL || hres == S_FALSE);
    ASSERT(_hwnd != NULL || hres == S_FALSE);

    return hres;
}

HRESULT CToolBand::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    ASSERT(NULL == lpMsg || IS_VALID_WRITE_PTR(lpMsg, MSG));

    TraceMsg(DM_FOCUS, "ctb.uiaio(fActivate=%d) _fCanFocus=%d _hwnd=%x GF()=%x", fActivate, _fCanFocus, _hwnd, GetFocus());

    if (!_fCanFocus) {
        TraceMsg(DM_FOCUS, "ctb.uiaio: !_fCanFocus ret S_FALSE");
        return S_FALSE;
    }

    if (fActivate)
    {
        UnkOnFocusChangeIS(_punkSite, SAFECAST(this, IInputObject*), TRUE);
        SetFocus(_hwnd);
    }

    return S_OK;
}

HRESULT CToolBand::ResizeBorderDW(LPCRECT prcBorder,
                                         IUnknown* punkToolbarSite,
                                         BOOL fReserved)
{
    return S_OK;
}


HRESULT CToolBand::ShowDW(BOOL fShow)
{
    return S_OK;
}

HRESULT CToolBand::SetSite(IUnknown *punkSite)
{
    if (punkSite != _punkSite)
    {
        IUnknown_Set(&_punkSite, punkSite);
        IUnknown_GetWindow(_punkSite, &_hwndParent);
    }
    return S_OK;
}

HRESULT CToolBand::_BandInfoChanged()
{
    VARIANTARG v = {0};
    VARIANTARG* pv = NULL;
    if (_dwBandID != (DWORD)-1)
    {
        v.vt = VT_I4;
        v.lVal = _dwBandID;
        pv = &v;
    }
    else
    {
        // if this fires, fix your band's GetBandInfo to set _dwBandID.
        // o.w. it's a *big* perf loss since we refresh *all* bands rather
        // than just yours.
        // do *not* remove this ASSERT, bad perf *is* a bug.
        ASSERT(_dwBandID != (DWORD)-1);
    }
    return IUnknown_Exec(_punkSite, &CGID_DeskBand, DBID_BANDINFOCHANGED, 0, pv, NULL);
}

//  *** IPersistStream methods ***

HRESULT CToolBand::IsDirty(void)
{
    return S_FALSE;     // never be dirty
}

HRESULT CToolBand::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    return E_NOTIMPL;
}

CToolBand::CToolBand() : _cRef(1)
{
    _dwBandID = (DWORD)-1;
    DllAddRef();
}

CToolBand::~CToolBand()
{
    ASSERT(_hwnd == NULL);      // CloseDW was called
    ASSERT(_punkSite == NULL);  // SetSite(NULL) was called

    DllRelease();
}

HRESULT CToolBand::CloseDW(DWORD dw)
{
    if (_hwnd)
    {
        DestroyWindow(_hwnd);
        _hwnd = NULL;
    }
    
    return S_OK;
}


//=================================================================
// Implementation of CToolbarBand
//=================================================================
// Class for bands whose _hwnd is a toolbar control.  Implements
// functionality generic to all such bands (e.g. hottracking 
// behavior).
//=================================================================

HRESULT CToolbarBand::_PushChevron(BOOL bLast)
{
    if (_dwBandID == (DWORD)-1)
        return E_UNEXPECTED;

    VARIANTARG v;
    v.vt = VT_I4;
    v.lVal = bLast ? DBPC_SELECTLAST : DBPC_SELECTFIRST;

    return IUnknown_Exec(_punkSite, &CGID_DeskBand, DBID_PUSHCHEVRON, _dwBandID, &v, NULL);
}

LRESULT CToolbarBand::_OnHotItemChange(LPNMTBHOTITEM pnmtb)
{
    LRESULT lres = 0;

    if (!(pnmtb->dwFlags & (HICF_LEAVING | HICF_MOUSE)))
    {
        // check to see if new hot button is clipped.  if it is,
        // then we pop down the chevron menu.
        RECT rc;
        GetClientRect(_hwnd, &rc);

        int iButton = (int)SendMessage(_hwnd, TB_COMMANDTOINDEX, pnmtb->idNew, 0);
        DWORD dwEdge = SHIsButtonObscured(_hwnd, &rc, iButton);
        if (dwEdge)
        {
            //
            // Only pop down the menu if the button is obscured
            // along the axis of the toolbar
            //
            BOOL fVertical = (ToolBar_GetStyle(_hwnd) & CCS_VERT);

            if ((fVertical && (dwEdge & (EDGE_TOP | EDGE_BOTTOM)))
                || (!fVertical && (dwEdge & (EDGE_LEFT | EDGE_RIGHT))))
            {
                // clear hot item
                SendMessage(_hwnd, TB_SETHOTITEM, -1, 0);

                // figure out whether to highlight first or last button in dd menu
                int cButtons = (int)SendMessage(_hwnd, TB_BUTTONCOUNT, 0, 0);
                BOOL bLast = (iButton == cButtons - 1);
                _PushChevron(bLast);
                lres = 1;
            }
        }
    }

    return lres;
}

LRESULT CToolbarBand::_OnNotify(LPNMHDR pnmh)
{
    LRESULT lres = 0;

    switch (pnmh->code)
    {
    case TBN_HOTITEMCHANGE:
        lres = _OnHotItemChange((LPNMTBHOTITEM)pnmh);
        break;
    }

    return lres;
}

// *** IWinEventHandler methods ***

HRESULT CToolbarBand::OnWinEvent(HWND hwnd, UINT dwMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres)
{
    HRESULT hres = S_OK;

    switch (dwMsg)
    {
    case WM_NOTIFY:
        *plres = _OnNotify((LPNMHDR)lParam);
        break;

    case WM_WININICHANGE:
        InvalidateRect(_hwnd, NULL, TRUE);
        _BandInfoChanged();
        break;
    }

    return hres;
}

HRESULT CToolbarBand::IsWindowOwner(HWND hwnd)
{
    if (hwnd == _hwnd)
        return S_OK;
    else
        return S_FALSE;
}
