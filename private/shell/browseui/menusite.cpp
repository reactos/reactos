#include "priv.h"
#include "sccls.h"
#include "menusite.h"
#include "inpobj.h"

CMenuSite::CMenuSite() : _cRef(1)
{
}


CMenuSite::~CMenuSite()
{
    // Make sure that SetDeskBarSite(NULL) was called
    ASSERT(_punkSite == NULL);
    ASSERT(_punkSubActive == NULL);
    ASSERT(_pweh == NULL);
    ASSERT(_pdb == NULL);
    ASSERT(_hwnd == NULL);
}


STDAPI CMenuBandSite_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CMenuSite *pbs = new CMenuSite();
    if (pbs)
    {
        *ppunk = SAFECAST(pbs, IOleWindow*);
        return S_OK;
    }
    *ppunk = NULL;
    return E_OUTOFMEMORY;
}


/*----------------------------------------------------------
Purpose: IUnknown::QueryInterface method

*/
STDMETHODIMP CMenuSite::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CMenuSite, IBandSite),
        QITABENT(CMenuSite, IDeskBarClient),
        QITABENT(CMenuSite, IOleCommandTarget),
        QITABENT(CMenuSite, IInputObject),
        QITABENT(CMenuSite, IInputObjectSite),
        QITABENT(CMenuSite, IWinEventHandler),
        QITABENT(CMenuSite, IServiceProvider),
        QITABENT(CMenuSite, IOleWindow),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


/*----------------------------------------------------------
Purpose: IUnknown::AddRef method

*/
STDMETHODIMP_(ULONG) CMenuSite::AddRef(void)
{
    _cRef++;
    return _cRef;
}


/*----------------------------------------------------------
Purpose: IUnknown::Release method

*/
STDMETHODIMP_(ULONG) CMenuSite::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}


/*----------------------------------------------------------
Purpose: IServiceProvider::QueryService method

*/
STDMETHODIMP CMenuSite::QueryService(REFGUID guidService, REFIID riid, void **ppvObj)
{
    HRESULT hres = E_FAIL;

    *ppvObj = NULL;     // assume error

    if (IsEqualIID(guidService, SID_SMenuBandBottom) ||
        IsEqualIID(guidService, SID_SMenuBandBottomSelected)||
        IsEqualIID(guidService, SID_SMenuBandChild))
    {
        if (_punkSubActive)
            hres = IUnknown_QueryService(_punkSubActive, guidService, riid, ppvObj);
    }
    else
    {
        ASSERT(_punkSite);
        hres = IUnknown_QueryService(_punkSite, guidService, riid, ppvObj);
    }

    return hres;
}    


/*----------------------------------------------------------
Purpose: IOleCommandTarget::QueryStatus

*/
STDMETHODIMP CMenuSite::QueryStatus(const GUID *pguidCmdGroup,
        ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    ASSERT(_punkSite);

    return IUnknown_QueryStatus(_punkSite, pguidCmdGroup, cCmds, rgCmds, pcmdtext);
}

/*----------------------------------------------------------
Purpose: IOleCommandTarget::Exec

*/
STDMETHODIMP CMenuSite::Exec(const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt,
        VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    ASSERT(_punkSite);

    return IUnknown_Exec(_punkSite, pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
}


/*----------------------------------------------------------
Purpose: IInputObjectSite::OnFocusChangeIS

         This function is called by the client band to negotiate
         which band in this bandsite gets the focus.  Typically
         this function will then change its focus to the given
         client band.

         CMenuSite only maintains one and only one band, which
         is set at AddBand time, so this function is a nop.

*/
STDMETHODIMP CMenuSite::OnFocusChangeIS(IUnknown *punk, BOOL fSetFocus)
{
    // Return S_OK since the menu site only ever has one band.
    // No need to negotiate which other band in this bandsite 
    // might have the "activation".
    return S_OK;
}


/*----------------------------------------------------------
Purpose: IInputObject::UIActivateIO method

*/
STDMETHODIMP CMenuSite::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    ASSERT(NULL == lpMsg || IS_VALID_WRITE_PTR(lpMsg, MSG));

    // Forward onto the client band
    return UnkUIActivateIO(_punkSubActive, fActivate, lpMsg);
}


/*----------------------------------------------------------
Purpose: IInputObject::HasFocusIO

         Since the menuband can never have true activation (from
         the browser's perspective) this always returns S_FALSE.

         See comments in CMenuBand::UIActivateIO for more details
         about this.

*/
STDMETHODIMP CMenuSite::HasFocusIO()
{
    return S_FALSE;
}


/*----------------------------------------------------------
Purpose: IInputObject::TranslateAcceleratorIO

         Menubands cannot ever have the activation, so this method 
         should never be called.
*/
STDMETHODIMP CMenuSite::TranslateAcceleratorIO(LPMSG lpMsg)
{
    AssertMsg(0, TEXT("Menuband has the activation but it shouldn't!"));

    return S_FALSE;
}


// Utility Functions

void CMenuSite::_CacheSubActiveBand(IUnknown * punk)
{
    if (SHIsSameObject(punk, _punkSubActive))
        return;
    
    IUnknown_SetSite(_punkSubActive, NULL);

    ATOMICRELEASE(_punkSubActive);
    ATOMICRELEASE(_pdb);
    ATOMICRELEASE(_pweh);
    _hwndChild = NULL;

    if (punk != NULL) 
    {
        EVAL(SUCCEEDED(punk->QueryInterface(IID_IDeskBand, (void **)&_pdb)));
        EVAL(SUCCEEDED(punk->QueryInterface(IID_IWinEventHandler, (void**)&_pweh)));

        IUnknown_SetSite(punk, SAFECAST(this, IOleWindow*));
        IUnknown_GetWindow(punk, &_hwndChild);

        _punkSubActive = punk;
        _punkSubActive->AddRef();
    }
}


/*----------------------------------------------------------
Purpose: IBandSite::AddBand

*/
STDMETHODIMP CMenuSite::AddBand(IUnknown* punk)
{
    _CacheSubActiveBand(punk);

    return NOERROR;
}


/*----------------------------------------------------------
Purpose: IBandSite::EnumBands

*/
STDMETHODIMP CMenuSite::EnumBands(UINT uBand, DWORD* pdwBandID)
{
    HRESULT hres = NOERROR;

    // The menusite only holds one band ever
    if (0 == uBand)
        *pdwBandID = 0;
    else
        hres = E_FAIL;

    return hres;
}


/*----------------------------------------------------------
Purpose: IBandSite::QueryBand

*/
HRESULT CMenuSite::QueryBand(DWORD dwBandID, IDeskBand** ppstb, DWORD* pdwState, LPWSTR pszName, int cchName)
{
    HRESULT hres = E_NOINTERFACE;

    ASSERT(dwBandID == 0);
    ASSERT(IS_VALID_WRITE_PTR(ppstb, IDeskBand *));

    if (_punkSubActive && 0 == dwBandID)
    {
        hres = _punkSubActive->QueryInterface(IID_IDeskBand, (void**)ppstb);
        *pdwState = BSSF_VISIBLE; // Only band....

        if (cchName > 0)
            *pszName = L'\0';
    }
    else
        *ppstb = NULL;

    return hres;
}


/*----------------------------------------------------------
Purpose: IBandSite::SetBandState

*/
HRESULT CMenuSite::SetBandState(DWORD dwBandID, DWORD dwMask, DWORD dwState)
{
    return E_NOTIMPL;
}


/*----------------------------------------------------------
Purpose: IBandSite::RemoveBand

*/
HRESULT CMenuSite::RemoveBand(DWORD dwBandID)
{
    return E_NOTIMPL;
}


/*----------------------------------------------------------
Purpose: IBandSite::GetBandObject

*/
HRESULT CMenuSite::GetBandObject(DWORD dwBandID, REFIID riid, LPVOID *ppvObj)
{
    HRESULT hres;

    ASSERT(dwBandID == 0);

    if (_punkSubActive && 0 == dwBandID)
        hres = _punkSubActive->QueryInterface(riid, ppvObj);
    else
    {
        *ppvObj = NULL;
        hres = E_NOINTERFACE;
    }
    return hres;
}


/*----------------------------------------------------------
Purpose: IBandSite::SetBandSiteInfo

*/
HRESULT CMenuSite::SetBandSiteInfo(const BANDSITEINFO * pbsinfo)
{
    return E_NOTIMPL;
}


/*----------------------------------------------------------
Purpose: IBandSite::GetBandSiteInfo

*/
HRESULT CMenuSite::GetBandSiteInfo(BANDSITEINFO * pbsinfo)
{
    return E_NOTIMPL;
}


/*----------------------------------------------------------
Purpose: IOleWindow::GetWindow

*/
HRESULT CMenuSite::GetWindow(HWND * lphwnd)
{
    ASSERT(IS_VALID_HANDLE(_hwnd, WND));

    *lphwnd = _hwnd;
    return NOERROR;
}

/*----------------------------------------------------------
Purpose: IOleWindow::ContextSensitiveHelp

*/
HRESULT CMenuSite::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}


/*----------------------------------------------------------
Purpose: IDeskBarClient::SetDeskBarSite

*/
HRESULT CMenuSite::SetDeskBarSite(IUnknown* punkSite)
{
    ATOMICRELEASE(_punkSite);

    if (punkSite)
    {
        HWND hwnd;
        IUnknown_GetWindow(punkSite, &hwnd);

        if (hwnd)
        {
            _CreateSite(hwnd);

            _punkSite = punkSite;
            _punkSite->AddRef();
        }
    }
    else
    {
        _pdb->CloseDW(0);
        _CacheSubActiveBand(NULL);      // This is asymetric by design

        if (_hwnd)
        {
            DestroyWindow(_hwnd);
            _hwnd = NULL;
        }
    }

    return _hwnd ? NOERROR : E_FAIL;
}


/*----------------------------------------------------------
Purpose: IDeskBarClient::SetModeDBC

*/
HRESULT CMenuSite::SetModeDBC(DWORD dwMode)
{
    return E_NOTIMPL;
}


/*----------------------------------------------------------
Purpose: IDeskBarClient::UIActivateDBC

*/
HRESULT CMenuSite::UIActivateDBC(DWORD dwState)
{
    ASSERT(_pdb);
    if (_pdb)
        _pdb->ShowDW(0 != dwState);

    return NOERROR;
}

/*----------------------------------------------------------
Purpose: IDeskBarClient::GetSize

*/
HRESULT CMenuSite::GetSize(DWORD dwWhich, LPRECT prc)
{
    if (dwWhich == DBC_GS_IDEAL)
    {
        if (_pdb)
        {
            DESKBANDINFO dbi = {0};
            _pdb->GetBandInfo(0, 0, &dbi);
            prc->right = dbi.ptMaxSize.x;
            prc->bottom = dbi.ptMaxSize.y;
        }
    }

    return NOERROR;
}


/*----------------------------------------------------------
Purpose: IWinEventHandler::OnWinEvent

*/
HRESULT CMenuSite::OnWinEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres)
{
    if (_pweh)
        return _pweh->OnWinEvent(hwnd, uMsg, wParam, lParam, plres);

    return NOERROR;
}


/*----------------------------------------------------------
Purpose: IWinEventHandler::IsWindowOwner

*/
HRESULT CMenuSite::IsWindowOwner(HWND hwnd)
{
    if (_hwnd == hwnd || (_pweh && _pweh->IsWindowOwner(hwnd) != S_FALSE))
        return S_OK;
    else
        return S_FALSE;
}


LRESULT CMenuSite::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lres = 0;

    EnterModeless();

    switch(uMsg)
    {
    case WM_SIZE:
        {
            IMenuPopup* pmp;

            if (_punkSubActive && SUCCEEDED(_punkSubActive->QueryInterface(IID_IMenuPopup, (void**)&pmp)))
            {
                RECT rc = {0};

                GetClientRect(_hwnd, &rc);

                pmp->OnPosRectChangeDB(&rc);
                pmp->Release();
            }
            lres = 1;
        }
        break;

    case WM_NOTIFY:
        hwnd = ((LPNMHDR)lParam)->hwndFrom;
        break;
        
    case WM_COMMAND:
        hwnd = GET_WM_COMMAND_HWND(wParam, lParam);
        break;
        
    default:
        ExitModeless();
        return DefWindowProcWrap(hwnd, uMsg, wParam, lParam);
        break;
    }

    if (hwnd && _pweh && _pweh->IsWindowOwner(hwnd) == S_OK) 
    {
        _pweh->OnWinEvent(hwnd, uMsg, wParam, lParam, &lres);
    }

    ExitModeless();
    return lres;
}


void CMenuSite::_CreateSite(HWND hwndParent)
{
    if (_hwnd)
    {
        ASSERT(IS_VALID_HANDLE(_hwnd, WND));    // just to be safe...
        return;
    }

    WNDCLASS  wc = {0};
    wc.style            = 0;
    wc.lpfnWndProc      = s_WndProc;
    //wc.cbClsExtra       = 0;
    wc.cbWndExtra       = SIZEOF(CMenuSite*);
    wc.hInstance        = HINST_THISDLL;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH) (COLOR_MENU+1);
    //wc.lpszMenuName     =  NULL;
    wc.lpszClassName    = TEXT("MenuSite");
    //wc.hIcon            = NULL;

    SHRegisterClass(&wc);

    _hwnd = CreateWindow(TEXT("MenuSite"), NULL, WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, 
        hwndParent, NULL, HINST_THISDLL, (LPVOID)SAFECAST(this, CImpWndProc*));

    ASSERT(_hwnd);
}
