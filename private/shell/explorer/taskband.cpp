#include "cabinet.h"
#include "taskband.h"
#include "bandsite.h"

#define DM_FOCUS    0               // focus
#define DM_COMWIN       0           // com/win32 interaction

extern "C" IDropTarget* Tray_GetDropTarget();

CSimpleOleWindow::~CSimpleOleWindow()
{
}

CSimpleOleWindow::CSimpleOleWindow(HWND hwnd) : _cRef(1), _hwnd(hwnd)
{
}

ULONG CSimpleOleWindow::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CSimpleOleWindow::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CSimpleOleWindow::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IOleWindow))
    {
        *ppvObj = SAFECAST(this, IOleWindow*);
    } 
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
        
    }
    
    AddRef();
    return S_OK;
}


HRESULT CSimpleOleWindow::GetWindow(HWND * lphwnd) 
{
    *lphwnd = _hwnd; 
    if (_hwnd)
        return S_OK; 
    return E_FAIL;
        
}


#define SUPERCLASS CSimpleOleWindow

class CTaskBand : public CSimpleOleWindow
        ,public IDeskBand
        ,public IObjectWithSite
        ,public IDropTarget
        ,public IInputObject
        ,public IWinEventHandler
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) {return SUPERCLASS::AddRef();};
    virtual STDMETHODIMP_(ULONG) Release(void){return SUPERCLASS::Release();};

    // *** IOleWindow methods ***
    virtual STDMETHODIMP GetWindow(HWND * lphwnd) {return SUPERCLASS::GetWindow(lphwnd);};
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) { return SUPERCLASS::ContextSensitiveHelp(fEnterMode); };

    // *** IDockingWindow methods ***
    virtual STDMETHODIMP ShowDW(BOOL fShow);
    virtual STDMETHODIMP CloseDW(DWORD dwReserved);
    virtual STDMETHODIMP ResizeBorderDW(LPCRECT prcBorder,
                                             IUnknown* punkToolbarSite,
                                             BOOL fReserved);

    // *** IObjectWithSite methods ***
    virtual STDMETHODIMP SetSite(IUnknown* punkSite);
    // BUGBUG is E_NOTIMPL ok?
    virtual STDMETHODIMP GetSite(REFIID riid, void** ppvSite) { ASSERT(0); return E_NOTIMPL; };

    // *** IDeskBand methods ***
    virtual STDMETHODIMP GetBandInfo(DWORD dwBandID, DWORD fViewMode, 
                                     DESKBANDINFO* pdbi) ;

    
    // *** IDropTarget methods ***
    virtual STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    virtual STDMETHODIMP DragLeave(void);
    virtual STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    // *** IInputObject methods ***
    virtual STDMETHODIMP TranslateAcceleratorIO(LPMSG lpMsg);
    virtual STDMETHODIMP HasFocusIO();
    virtual STDMETHODIMP UIActivateIO(BOOL fActivate, LPMSG lpMsg);

    // *** IWinEventHandler methods ***
    virtual STDMETHODIMP OnWinEvent(HWND hwnd, UINT dwMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres);
    virtual STDMETHODIMP IsWindowOwner(HWND hwnd);

protected:
    
    friend IUnknown* Tasks_CreateInstance();
        
    CTaskBand();
    ~CTaskBand();

    DWORD _dwBandID;

private:
    IUnknown *  _punkSite;
    
} ;


CTaskBand::CTaskBand() : 
    SUPERCLASS(NULL)
{
    _dwBandID = (DWORD)-1;   
}

CTaskBand::~CTaskBand()
{
    if (_punkSite)
        _punkSite->Release();
}

HRESULT CTaskBand::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    if (IsEqualIID(riid, IID_IDockingWindow)
        || IsEqualIID(riid, IID_IDeskBand))
    {
        *ppvObj = SAFECAST(this, IDeskBand*);
    }
    else if (IsEqualIID(riid, IID_IObjectWithSite)) 
    {
        *ppvObj = SAFECAST(this, IObjectWithSite*);
    } 
    else if (IsEqualIID(riid, IID_IDropTarget)) 
    {
        *ppvObj = SAFECAST(this, IDropTarget*);
    } 
    else if (IsEqualIID(riid, IID_IInputObject)) {
        *ppvObj = SAFECAST(this, IInputObject*);
    }
    else if (IsEqualIID(riid, IID_IWinEventHandler)) {
        *ppvObj = SAFECAST(this, IWinEventHandler*);
    }
    else
    {
        return SUPERCLASS::QueryInterface(riid, ppvObj);
    }
    AddRef();
    return NOERROR;
}


IUnknown* Tasks_CreateInstance()
{
    CTaskBand* pstb = new CTaskBand();

    return (IDeskBand*)pstb;
}




HRESULT CTaskBand::ShowDW(BOOL f)
{
    return S_OK;
}

HRESULT CTaskBand::CloseDW(DWORD dwReserved)
{
    return S_OK;
}

HRESULT CTaskBand::ResizeBorderDW(LPCRECT prcBorder, IUnknown* punkToolbarSite, BOOL fReserved)
{
    return E_NOTIMPL;
}

//***   CTaskBand::IInputObject::* {

HRESULT CTaskBand::TranslateAcceleratorIO(LPMSG lpMsg)
{
#if 0
#ifdef DEBUG
    extern BOOL IsVK_TABCycler(WPARAM wParam);

    if (lpMsg && lpMsg->message == WM_KEYDOWN
      && IsVK_TABCycler(lpMsg->wParam)) {
        TraceMsg(DM_FOCUS, "ctb.taio: TAB hres=E_NOTIMPL");
    }
#endif
#endif
    return E_NOTIMPL;
}

extern "C" BOOL IsChildOrHWND(HWND hwnd, HWND hwndChild);

HRESULT CTaskBand::HasFocusIO()
{
    BOOL f;
    HWND hwndFocus = GetFocus();

    f = IsChildOrHWND(_hwnd, hwndFocus);
    ASSERT(hwndFocus != NULL || !f);
    ASSERT(_hwnd != NULL || !f);
    if (!f)
        TraceMsg(DM_FOCUS, "ctb.hfio: hres=S_FALSE");
    return f ? S_OK : S_FALSE;
}

//***
// NOTES
//  BUGBUG should default be to SetFocus or to ignore?
HRESULT CTaskBand::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    ASSERT(NULL == lpMsg || IS_VALID_WRITE_PTR(lpMsg, MSG));

    TraceMsg(DM_FOCUS, "ctb.uiaio(fActivate=%d)", fActivate);

    #define _fCanFocus 1
    if (!_fCanFocus) {
        TraceMsg(DM_FOCUS, "ctb.uiaio: !_fCanFocus ret S_FALSE");
        return S_FALSE;
    }
    #undef  _fCanFocus

    if (fActivate) {
        UnkOnFocusChangeIS(_punkSite, SAFECAST(this, IInputObject*), TRUE);
        SetFocus(_hwnd);
    }
    else {
        // if we don't have focus, we're fine;
        // if we do have focus, there's nothing we can do about it...
        /*NOTHING*/
#ifdef DEBUG
        TraceMsg(DM_FOCUS, "ctb.uiaio: GetFocus()=%x _hwnd=%x", GetFocus(), _hwnd);
#endif
    }

    return S_OK;
}

// }

extern "C" BOOL Tasks_Create(HWND hwndParent);

HRESULT CTaskBand::SetSite(IUnknown* punk)
{
    HRESULT hres = S_OK;

    if (punk != _punkSite) {
        if (_punkSite)
            _punkSite->Release();

        _punkSite = punk;
        _hwnd = NULL;

        if (punk) {
            HWND hwndParent;

            hres = E_FAIL;

            punk->AddRef();

            if (SUCCEEDED(IUnknown_GetWindow(punk, &hwndParent)))
            {
                ASSERT(!_hwnd);
                if (Tasks_Create(hwndParent))
                {
                    _hwnd = g_tasks.hwnd;
                    hres = S_OK;
                }
            }
        }
    }

    return hres;
}


HRESULT CTaskBand::GetBandInfo(DWORD dwBandID, DWORD fViewMode, 
                                DESKBANDINFO* pdbi) 
{
    _dwBandID = dwBandID;

    pdbi->ptMaxSize.y = -1;
    pdbi->ptActual.y =  g_cySize + 2*g_cyEdge;
    
    if (fViewMode & DBIF_VIEWMODE_VERTICAL) {
        pdbi->ptMinSize.y = 1;
        pdbi->ptMinSize.x = (g_cySize + 2*g_cyEdge + g_cyTabSpace + 1);
        pdbi->ptIntegral.y = 1;
    } else {

        pdbi->ptMinSize.y = g_cySize + 2*g_cyEdge;
        pdbi->ptMinSize.x = pdbi->ptIntegral.y = g_cySize + 2*g_cyEdge + g_cyTabSpace;
        pdbi->ptMinSize.x *= 3;
    }

    pdbi->dwModeFlags = DBIMF_VARIABLEHEIGHT;
    pdbi->dwMask &= ~DBIM_TITLE;    // no title for us (ever)

    return S_OK;
}


// IDropTarget implementation
//

HRESULT CTaskBand::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    return Tray_GetDropTarget()->DragEnter(pdtobj, grfKeyState, pt, pdwEffect);
}
HRESULT CTaskBand::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    return Tray_GetDropTarget()->DragOver(grfKeyState, pt, pdwEffect);
}
HRESULT CTaskBand::DragLeave(void)
{
    return Tray_GetDropTarget()->DragLeave();
}
HRESULT CTaskBand::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    return Tray_GetDropTarget()->Drop(pdtobj, grfKeyState, pt, pdwEffect);
}

// *** IWinEventHandler methods ***
HRESULT CTaskBand::OnWinEvent(HWND hwnd, UINT dwMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres)
{
    *plres = 0;
    
    switch (dwMsg) 
    {
    case WM_WININICHANGE:
    case WM_SYSCOLORCHANGE:
    case WM_PALETTECHANGED:
        TraceMsg(DM_COMWIN, "ctb.owe: fwd WM_WININICHG(etc.)");
        SendMessage(_hwnd, dwMsg, wParam, lParam);
        break;

    case WM_NOTIFY:
        if (EVAL(lParam))
        {
            switch (((LPNMHDR)lParam)->code)
            {
            case NM_SETFOCUS:
                UnkOnFocusChangeIS(_punkSite, SAFECAST(this, IInputObject*), TRUE);
                break;
            }
        }
        break;
    }

    return S_OK;
}

HRESULT CTaskBand::IsWindowOwner(HWND hwnd)
{
    BOOL bRet = IsChildOrHWND(_hwnd, hwnd);
    ASSERT (_hwnd || !bRet);
    ASSERT (hwnd || !bRet);
    return bRet ? S_OK : S_FALSE;
}
