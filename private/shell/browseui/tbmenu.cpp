#include "priv.h"
#include "resource.h"
#include "tbmenu.h"
#include "isfband.h"
#include "isfmenu.h"
#include "util.h"

#include "mluisupp.h"
#define SMFORWARD(x) if (!_psm) { return E_FAIL; } else return _psm->x

#define SUPERCLASS CMenuToolbarBase

CToolbarMenu::CToolbarMenu(DWORD dwFlags, HWND hwndTB) :
   CMenuToolbarBase(NULL, dwFlags),
   _hwndSubject(hwndTB)  // this is the toolbar that we are customizing
{
}

void CToolbarMenu::GetSize(SIZE* psize)
{
    ASSERT(_hwndMB);

    if (SendMessage(_hwndMB, TB_GETTEXTROWS, 0, 0) == 0) {
        // no text labels, so set a min width to make menu look 
        // pretty.  use min width of 4 * button width.
        LRESULT lButtonSize = SendMessage(_hwndMB, TB_GETBUTTONSIZE, 0, 0);
        LONG cxMin = 4 * LOWORD(lButtonSize);
        psize->cx = max(psize->cx, cxMin);
    }
    SUPERCLASS::GetSize(psize);
}

void CToolbarMenu::v_Show(BOOL fShow, BOOL fForceUpdate)
{
    if (fShow)
    {
        _fClickHandled = FALSE;
        _FillToolbar();
        _pcmb->SetTracked(NULL);  // Since hot item is NULL
        ToolBar_SetHotItem(_hwndMB, -1);
        if (fForceUpdate)
            v_UpdateButtons(TRUE);
    }
}

void CToolbarMenu::v_Close()
{
    _UnsubclassWindow(_hwndMB);
}


void CToolbarMenu::v_UpdateButtons(BOOL fNegotiateSize)
{
}


HRESULT CToolbarMenu::v_CallCBItem(int idtCmd, UINT dwMsg, WPARAM wParam, LPARAM lParam)
{
    return S_OK;
}

HRESULT CToolbarMenu::v_GetState(int idtCmd, LPSMDATA psmd)
{
    ASSERT(0);
    return E_NOTIMPL;
}

HRESULT CToolbarMenu::v_ExecItem(int idCmd)
{
    HRESULT hres = E_FAIL;
    TraceMsg(TF_TBMENU, "CToolbarMenu::v_ExecItem \tidCmd: %d", idCmd);
    return hres;
}

void CToolbarMenu::CreateToolbar(HWND hwndParent)
{
    if (!_hwndMB)
    {
        DWORD dwStyle = (WS_VISIBLE | WS_CHILD | TBSTYLE_FLAT |
                         WS_CLIPCHILDREN | WS_CLIPSIBLINGS | CCS_NODIVIDER | 
                         CCS_NOPARENTALIGN | CCS_NORESIZE  | TBSTYLE_REGISTERDROP | TBSTYLE_TOOLTIPS);

        INT_PTR nRows = SendMessage(_hwndSubject, TB_GETTEXTROWS, 0, 0);

        if (nRows > 0)
        {
            // We have text labels; make it TBSTYLE_LIST.  The base class will
            // set TBSTYLE_EX_VERTICAL for us.
            ASSERT(_fHorizInVerticalMB == FALSE);
            dwStyle |= TBSTYLE_LIST;
        }
        else
        {
            // No text labels; make it horizontal and TBSTYLE_WRAPABLE.  Set
            // _fHorizInVerticalMB so that the base class does not try and set
            // TBSTYLE_EX_VERTICAL.
            _fHorizInVerticalMB = TRUE;
            dwStyle |= TBSTYLE_WRAPABLE;
        }

        _hwndMB = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, TEXT("Menu"), dwStyle,
                                 0, 0, 0, 0, hwndParent, (HMENU) FCIDM_TOOLBAR, HINST_THISDLL, NULL);

        if (!_hwndMB)
        {
            TraceMsg(TF_TBMENU, "CToolbarMenu::CreateToolbar: Failed to Create Toolbar");
            return;
        }

        HWND hwndTT = (HWND)SendMessage(_hwndMB, TB_GETTOOLTIPS, 0, 0);
        SHSetWindowBits(hwndTT, GWL_STYLE, TTS_ALWAYSTIP, TTS_ALWAYSTIP);
        SendMessage(_hwndMB, TB_BUTTONSTRUCTSIZE, SIZEOF(TBBUTTON), 0);
        SendMessage(_hwndMB, TB_SETMAXTEXTROWS, nRows, 0);
        SendMessage(_hwndMB, CCM_SETVERSION, COMCTL32_VERSION, 0);

        SendMessage(_hwndMB, TB_SETEXTENDEDSTYLE, TBSTYLE_EX_DRAWDDARROWS, TBSTYLE_EX_DRAWDDARROWS);

        int cPimgs = (int)SendMessage(_hwndSubject, TB_GETIMAGELISTCOUNT, 0, 0);
        for (int i = 0; i < cPimgs; i++)
        {
            HIMAGELIST himl = (HIMAGELIST)SendMessage(_hwndSubject, TB_GETIMAGELIST, i, 0);
            SendMessage(_hwndMB, TB_SETIMAGELIST, i, (LPARAM)himl);
            HIMAGELIST himlHot = (HIMAGELIST)SendMessage(_hwndSubject, TB_GETHOTIMAGELIST, i, 0);
            SendMessage(_hwndMB, TB_SETHOTIMAGELIST, i, (LPARAM)himlHot);
        }

        _SubclassWindow(_hwndMB);

        // Set the format to ANSI
        ToolBar_SetUnicodeFormat(_hwndMB, 0);

        CMenuToolbarBase::CreateToolbar(hwndParent);
        
    }
    else if (GetParent(_hwndMB) != hwndParent)
    {
        ::SetParent(_hwndMB, hwndParent);
    }
}


LRESULT CToolbarMenu::_DefWindowProc(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = CMenuToolbarBase::_DefWindowProcMB(hwnd, uMessage, wParam, lParam);

    if (lRes == 0)
        lRes = CNotifySubclassWndProc::_DefWindowProc(hwnd, uMessage, wParam, lParam);

    return lRes;
}

#define MAXLEN 256

void CToolbarMenu::_FillToolbar()
{
    RECT rcTB;
    TCHAR pszBuf[MAXLEN+1];
    LPTSTR psz;
    TBBUTTON tb;
    INT_PTR i, iCount;

    iCount = SendMessage(_hwndSubject, TB_BUTTONCOUNT, 0, 0L);
    GetClientRect(_hwndSubject, &rcTB);

    for (i = 0; i < iCount; i++) {
        if (SHIsButtonObscured(_hwndSubject, &rcTB, i)) {
            SendMessage(_hwndSubject, TB_GETBUTTON, i, (LPARAM)&tb);
            if (!(tb.fsStyle & BTNS_SEP)) {

                // autosize buttons look ugly here
                tb.fsStyle &= ~BTNS_AUTOSIZE;

                // need to rip off wrap bit; new toolbar will
                // figure out where wrapping should happen
                tb.fsState &= ~TBSTATE_WRAP;

                if (tb.iString == -1) {
                    // no string
                    psz = NULL;
                } else if (HIWORD(tb.iString)) {
                    // it's a string pointer
                    psz = (LPTSTR) tb.iString;
                } else {
                    // it's an index into toolbar string array
                    SendMessage(_hwndSubject, TB_GETSTRING, MAKELONG(MAXLEN, tb.iString), (LPARAM)pszBuf);
                    psz = pszBuf;
                }
                if (psz)
                    tb.iString = (INT_PTR)psz;
                else
                    tb.iString = -1;

                if (tb.iBitmap == -1) {
                    int id = GetDlgCtrlID(_hwndSubject);

                    NMTBDISPINFO  tbgdi = {0};
                    tbgdi.hdr.hwndFrom  = _hwndSubject;
                    tbgdi.hdr.idFrom    = id;
                    tbgdi.hdr.code      = TBN_GETDISPINFO;
                    tbgdi.dwMask        = TBNF_IMAGE;
                    tbgdi.idCommand     = tb.idCommand;
                    tbgdi.iImage        = 0;
                    tbgdi.lParam        = tb.dwData;

                    SendMessage(GetParent(_hwndSubject), WM_NOTIFY, (WPARAM)id, (LPARAM)&tbgdi);

                    if(tbgdi.dwMask & TBNF_DI_SETITEM)
                        tb.iBitmap = tbgdi.iImage;
                }

                SendMessage(_hwndMB, TB_ADDBUTTONS, 1, (LPARAM)&tb);
            }
        }
    }
}

STDMETHODIMP CToolbarMenu::IsWindowOwner(HWND hwnd) 
{ 
    if ( hwnd == _hwndMB) 
        return S_OK;
    else 
        return S_FALSE; 
}

void CToolbarMenu::_CancelMenu()
{
    IMenuPopup* pmp;
    if (EVAL(SUCCEEDED(_pcmb->QueryInterface(IID_IMenuPopup, (LPVOID*)&pmp)))) {
        // tell menuband it's time to die
        pmp->OnSelect(MPOS_FULLCANCEL);
        pmp->Release();
    }
}

STDMETHODIMP CToolbarMenu::OnWinEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres)
{
    HRESULT hres = S_FALSE;
    ASSERT(plres);

    switch (uMsg) {

    case WM_COMMAND:
        PostMessage(GetParent(_hwndSubject), WM_COMMAND, wParam, (LPARAM)_hwndSubject);
        _CancelMenu();
        hres = S_OK;
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR) lParam;

            switch (pnmh->code) {

            case TBN_DROPDOWN:
                _fTrackingSubMenu = TRUE;
                pnmh->hwndFrom = _hwndSubject;
                MapWindowPoints(_hwndMB, _hwndSubject, (LPPOINT) &((LPNMTOOLBAR)pnmh)->rcButton, 2);
                *plres = SendMessage(GetParent(_hwndSubject), WM_NOTIFY, wParam, (LPARAM)pnmh);
                _CancelMenu();
                hres = S_OK;
                _fTrackingSubMenu = FALSE;
                break;

            case NM_CUSTOMDRAW:
                // override mnbase custom draw shiznits
                *plres = 0;
                hres = S_OK;
                break;

            default:
                goto DoDefault;
            }

            break;
        }
DoDefault:
    default:
        hres = SUPERCLASS::OnWinEvent(hwnd, uMsg, wParam, lParam, plres);
    }

    return hres;
}

CMenuToolbarBase* ToolbarMenu_Create(HWND hwnd)
{
    return new CToolbarMenu(0, hwnd);
}

typedef struct
{
    WNDPROC pfnOriginal;
    IMenuBand* pmb;
} MENUHOOK;

#define SZ_MENUHOOKPROP TEXT("MenuHookProp")

LRESULT CALLBACK MenuHookWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    MENUHOOK* pmh = (MENUHOOK*)GetProp(hwnd, SZ_MENUHOOKPROP);

    if (pmh)
    {
        MSG msg;
        LRESULT lres;

        msg.hwnd = hwnd;
        msg.message = uMsg;
        msg.wParam = wParam;
        msg.lParam = lParam;

        if (pmh->pmb->TranslateMenuMessage(&msg, &lres) == S_OK)
            return lres;

        wParam = msg.wParam;
        lParam = msg.lParam;
        return CallWindowProc(pmh->pfnOriginal, hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

HRESULT HookMenuWindow(HWND hwnd, IMenuBand* pmb)
{
    HRESULT hres = E_FAIL;

    ASSERT(IsWindow(hwnd));

    // make sure we haven't already hooked this window
    if (GetProp(hwnd, SZ_MENUHOOKPROP) == NULL)
    {
        MENUHOOK* pmh = new MENUHOOK;
        if (pmh)
        {
            pmh->pfnOriginal = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_WNDPROC);
            pmh->pmb = pmb;

            SetProp(hwnd, SZ_MENUHOOKPROP, pmh);

            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)MenuHookWndProc);

            hres = S_OK;
        }
    }
    return hres;
}

void UnHookMenuWindow(HWND hwnd)
{

    MENUHOOK* pmh = (MENUHOOK*)GetProp(hwnd, SZ_MENUHOOKPROP);
    if (pmh)
    {
        SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR) pmh->pfnOriginal);
        SetProp(hwnd, SZ_MENUHOOKPROP, NULL);
        delete pmh;
    }

}


// This class is here to implement a "Menu Filter". We need this because the old style of 
// implementing obscured Menus does not work because user munges the WM_INITMENUPOPUP information
// based on the relative position within the HMENU. So here we keep that information, we just hide the item.

class CShellMenuCallbackWrapper : public IShellMenuCallback,
                                  public CObjectWithSite
{
    int _cRef;
    IShellMenuCallback* _psmc;
    HWND    _hwnd;
    RECT    _rcTB;
    ~CShellMenuCallbackWrapper()
    {
        ATOMICRELEASE(_psmc);
    }

public:
    CShellMenuCallbackWrapper(HWND hwnd, IShellMenuCallback* psmc) : _cRef(1)
    {
        _psmc = psmc;
        if (_psmc)
            _psmc->AddRef();
        _hwnd = hwnd;
        GetClientRect(_hwnd, &_rcTB);
    }

    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface (REFIID riid, LPVOID * ppvObj)
    {
        static const QITAB qit[] = 
        {
            QITABENT(CShellMenuCallbackWrapper, IShellMenuCallback),
            QITABENT(CShellMenuCallbackWrapper, IObjectWithSite),
            { 0 },
        };

        return QISearch(this, qit, riid, ppvObj);
    }

    STDMETHODIMP_(ULONG) AddRef()
    {
        _cRef++;
        return _cRef;
    }

    STDMETHODIMP_(ULONG) Release()
    {
        ASSERT(_cRef > 0);
        _cRef--;

        if (_cRef > 0)
            return _cRef;

        delete this;
        return 0;
    }

    // *** CObjectWithSite methods (override)***
    STDMETHODIMP SetSite(IUnknown* punk)            {   return IUnknown_SetSite(_psmc, punk);   }
    STDMETHODIMP GetSite(REFIID riid, void** ppObj) {   return IUnknown_GetSite(_psmc, riid, ppObj); }

    // *** IShellMenuCallback methods ***
    STDMETHODIMP CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        HRESULT hres = S_FALSE;
        
        if (_psmc)
            hres = _psmc->CallbackSM(psmd, uMsg, wParam, lParam);

        if (uMsg == SMC_GETINFO)
        {
            SMINFO* psminfo = (SMINFO*)lParam;
            int iPos = (int)SendMessage(_hwnd, TB_COMMANDTOINDEX, psmd->uId, 0);

            if (psminfo->dwMask & SMIM_FLAGS &&
                iPos >= 0 && 
                !SHIsButtonObscured(_hwnd, &_rcTB, iPos))
            {
                psminfo->dwFlags |= SMIF_HIDDEN;
                hres = S_OK;
            }
        }

        return hres;
    }
};



//
// CTrackShellMenu implementation
//


STDAPI  CTrackShellMenu_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    HRESULT hres = E_OUTOFMEMORY;
    CTrackShellMenu* pObj = new CTrackShellMenu();
    if (pObj)
    {
        *ppunk = SAFECAST(pObj, IShellMenu *);
        hres = S_OK;
    }

    return hres;
}

CTrackShellMenu::CTrackShellMenu() : _cRef(1)
{ 
    CoCreateInstance(CLSID_MenuBand, NULL, CLSCTX_INPROC_SERVER, 
        IID_IShellMenu, (LPVOID*)&_psm);
}

CTrackShellMenu::~CTrackShellMenu()
{
    ATOMICRELEASE(_psm);
    ATOMICRELEASE(_psmClient);
    ASSERT(!_punkSite);     // else someone neglected to call matching SetSite(NULL)
}

ULONG CTrackShellMenu::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CTrackShellMenu::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CTrackShellMenu::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CTrackShellMenu, ITrackShellMenu, IShellMenu),   // IID_ITrackShellMenu
        QITABENT(CTrackShellMenu, IObjectWithSite),
        QITABENT(CTrackShellMenu, IServiceProvider),
        { 0 },
    };

    HRESULT hres = QISearch(this, qit, riid, ppvObj);

    return hres;
}

// *** IServiceProvider methods ***
HRESULT CTrackShellMenu::QueryService(REFGUID guidService,
                                  REFIID riid, void **ppvObj)
{
    return IUnknown_QueryService(_psm, guidService, riid, ppvObj);
}

// *** IShellMenu methods ***
STDMETHODIMP CTrackShellMenu::Initialize(IShellMenuCallback* psmc, UINT uId, UINT uIdAncestor, DWORD dwFlags)
{ SMFORWARD(Initialize(psmc, uId, uIdAncestor, dwFlags)); }

STDMETHODIMP CTrackShellMenu::GetMenuInfo(IShellMenuCallback** ppsmc, UINT* puId, UINT* puIdAncestor, DWORD* pdwFlags)
{ SMFORWARD(GetMenuInfo(ppsmc, puId, puIdAncestor, pdwFlags)); }

STDMETHODIMP CTrackShellMenu::SetShellFolder(IShellFolder* psf, LPCITEMIDLIST pidlFolder, HKEY hkey, DWORD dwFlags)
{ SMFORWARD(SetShellFolder(psf, pidlFolder, hkey, dwFlags)); }

STDMETHODIMP CTrackShellMenu::GetShellFolder(DWORD* pdwFlags, LPITEMIDLIST* ppidl, REFIID riid, void** ppvObj)
{ SMFORWARD(GetShellFolder(pdwFlags, ppidl, riid, ppvObj)); }

STDMETHODIMP CTrackShellMenu::SetMenu(HMENU hmenu, HWND hwnd, DWORD dwFlags)
{ SMFORWARD(SetMenu(hmenu, hwnd, dwFlags)); }

STDMETHODIMP CTrackShellMenu::GetMenu(HMENU* phmenu, HWND* phwnd, DWORD* pdwFlags)
{ SMFORWARD(GetMenu(phmenu, phwnd, pdwFlags)); }

STDMETHODIMP CTrackShellMenu::InvalidateItem(LPSMDATA psmd, DWORD dwFlags)
{ SMFORWARD(InvalidateItem(psmd, dwFlags)); }

STDMETHODIMP CTrackShellMenu::GetState(LPSMDATA psmd)
{ SMFORWARD(GetState(psmd)); }

STDMETHODIMP CTrackShellMenu::SetMenuToolbar(IUnknown* punk, DWORD dwFlags)
{ SMFORWARD(SetMenuToolbar(punk, dwFlags)); }

// *** ITrackShellMenu methods ***
HRESULT CTrackShellMenu::SetObscured(HWND hwndTB, IUnknown* punkBand, DWORD dwSMSetFlags)
{
    HRESULT hres = E_OUTOFMEMORY;

    // Make sure we created the Inner Shell Menu
    if (!_psm)
        return hres;

    if (punkBand && 
        SUCCEEDED(punkBand->QueryInterface(IID_IShellMenu, (void**)&_psmClient)))
    {
        UINT uId, uIdAncestor;
        DWORD dwFlags;
        IShellMenuCallback* psmcb;

        hres = _psmClient->GetMenuInfo(&psmcb, &uId, &uIdAncestor, &dwFlags);
        if (SUCCEEDED(hres))
        {
            IShellMenuCallback* psmcbClone = NULL;
            if (psmcb)
            {
                if (S_FALSE == psmcb->CallbackSM(NULL, SMC_GETOBJECT, 
                    (WPARAM)&IID_IShellMenuCallback,
                    (LPARAM)(LPVOID*)&psmcbClone))
                {
                    psmcbClone = psmcb;
                    psmcbClone->AddRef();
                }
            }

            dwFlags &= ~SMINIT_HORIZONTAL;

            CShellMenuCallbackWrapper* psmcw = new CShellMenuCallbackWrapper(hwndTB, psmcbClone);

            // We want the bands to think it is:
            // Top level - because it has no menuband parent
            // Vertical  - because it's not a menubar
            dwFlags |= SMINIT_TOPLEVEL | SMINIT_VERTICAL;
            hres = _psm->Initialize(psmcw, uId, ANCESTORDEFAULT, dwFlags);

            if (SUCCEEDED(hres))
            {
                HWND hwndOwner;
                HMENU hmenuObscured;
                hres = _psmClient->GetMenu(&hmenuObscured, &hwndOwner, NULL);
                if (SUCCEEDED(hres))
                {
                    hres = _psm->SetMenu(hmenuObscured, hwndOwner, dwSMSetFlags | SMSET_DONTOWN);   // Menuband takes ownership;
                }
            }

            if (psmcb)
                psmcb->Release();

            if (psmcbClone)
                psmcbClone->Release();

            if (psmcw)
                psmcw->Release();

        }
    }
    else
    {
        CMenuToolbarBase* pmtb = ToolbarMenu_Create(hwndTB);
        hres = E_OUTOFMEMORY;
        if (pmtb) 
        {
            hres = _psm->SetMenuToolbar(SAFECAST(pmtb, IWinEventHandler*), dwSMSetFlags);
            // DONT release! The menus break com identity rules because of a foobar when they were
            // initially designed.
        }

    }

    return hres;
}

HRESULT CTrackShellMenu::Popup(HWND hwnd, POINTL *ppt, RECTL *prcExclude, DWORD dwFlags)
{
    IMenuBand* pmb;
    HRESULT hres = E_INVALIDARG;


    if (!_psm)
        return hres;

    hres = _psm->QueryInterface(IID_IMenuBand, (void**)&pmb);
    if (FAILED(hres))
        return hres;

    HWND hwndParent = GetTopLevelAncestor(hwnd);

    // Did the user set a menu into the Shell Menu?
    HWND hwndSubclassed = NULL;
    GetMenu(NULL, &hwndSubclassed, NULL);
    if (hwndSubclassed == NULL)
    {
        // No; We need to artificially set one so that the message filtering and stuff works
        SetMenu(NULL, hwndParent, 0);
    }

    SetForegroundWindow(hwndParent);

    IMenuPopup* pmp;
    hres = CoCreateInstance(CLSID_MenuDeskBar, NULL, CLSCTX_INPROC_SERVER, IID_IMenuPopup, (LPVOID*)&pmp);
    if (SUCCEEDED(hres))
    {
        IBandSite* pbs;
        hres = CoCreateInstance(CLSID_MenuBandSite, NULL, CLSCTX_INPROC_SERVER, IID_IBandSite, (LPVOID*)&pbs);
        if (SUCCEEDED(hres)) 
        {
            hres = pmp->SetClient(pbs);
            if (SUCCEEDED(hres)) 
            {
                IDeskBand* pdb;
                hres = _psm->QueryInterface(IID_IDeskBand, (void**)&pdb);
                if (SUCCEEDED(hres)) 
                {
                    hres = pbs->AddBand(pdb);
                    pdb->Release();
                }
            }
            pbs->Release();
        }

        // If we've got a site ourselves, have MenuDeskBar use that.
        if (_punkSite)
            IUnknown_SetSite(pmp, _punkSite);

        if (pmb) 
        {
            void* pvContext = g_msgfilter.GetContext();
            hres = HookMenuWindow(hwndParent, pmb);
            if (SUCCEEDED(hres))
            {
                // This collapses any modal menus before we proceed. When switching between
                // Chevron menus, we need to collapse the previous menu. Refer to the comment
                // at the function definition.
                g_msgfilter.ForceModalCollapse();

                pmp->Popup(ppt, (LPRECTL)prcExclude, dwFlags);

                g_msgfilter.SetModal(TRUE);

                MSG msg;
                while (GetMessage(&msg, NULL, 0, 0)) 
                {
                    HRESULT hres = pmb->IsMenuMessage(&msg);
                    if (hres == E_FAIL)
                    {
                        // menuband says it's time to pack up and go home.
                        // re-post this message so that it gets handled after
                        // we've cleaned up the menu (avoid re-entrancy issues &
                        // let rebar restore state of chevron button to unpressed)
                        PostMessage(msg.hwnd, msg.message, msg.wParam, msg.lParam);
                        break;
                    }
                    else if (hres != S_OK) 
                    {
                        // menuband didn't handle this one
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }

                hres = S_OK;
                UnHookMenuWindow(hwndParent);
                // We cannot change the context when modal, so unset the modal flag so that we can undo the context block.
                g_msgfilter.SetModal(FALSE); 
                g_msgfilter.SetContext(pvContext, TRUE);
            }
            pmb->Release();
        }

        if (_psmClient)
        {
            // This is to fix a bug where if there is a cached ISHellMenu in the submenu,
            // and you share the callback (For example, Broweser menu callback and the
            // favorites menu being shared between the browser bar and the chevron menu)
            // when on menu collapsed, we were destroying the sub menu by doing a set site.
            // since we no longer do the set site on the sub menu, we need a way to say "Reset
            // your parent". and this is the best way.
            IUnknown_Exec(_psmClient, &CGID_MenuBand, MBANDCID_REFRESH, 0, NULL, NULL);
        }

        // This call is required regardless of whether we had a _punkSite above;
        // MenuDeskBar does its cleanup on SetSite(NULL).
        IUnknown_SetSite(pmp, NULL);
        pmp->Release();
    }

    return hres;
}

// *** IObjectWithSite methods ***
HRESULT CTrackShellMenu::SetSite(IUnknown* punkSite)
{
    ASSERT(NULL == punkSite || IS_VALID_CODE_PTR(punkSite, IUnknown));

    ATOMICRELEASE(_punkSite);

    _punkSite = punkSite;

    if (punkSite)
        punkSite->AddRef();

    return S_OK;
}

BOOL IsISFBand(IUnknown* punk)
{
    OLECMD rgCmds[] = {
        ISFBID_PRIVATEID, 0,
    };

    IUnknown_QueryStatus(punk, &CGID_ISFBand, SIZEOF(rgCmds), rgCmds, NULL);

    return BOOLIFY(rgCmds[0].cmdf);
}

HRESULT DoISFBandStuff(CTrackShellMenu* ptsm, IUnknown* punk)
{
    HRESULT hr = E_INVALIDARG;

    if (punk && ptsm) {
        IShellFolderBand* psfb;
        hr = punk->QueryInterface(IID_IShellFolderBand, (PVOID*)&psfb);

        if (SUCCEEDED(hr)) {
            BANDINFOSFB bi;
            bi.dwMask = ISFB_MASK_IDLIST | ISFB_MASK_SHELLFOLDER;

            hr = psfb->GetBandInfoSFB(&bi);

            if (SUCCEEDED(hr)) {
                CISFMenuCallback* pCallback = new CISFMenuCallback();

                if (pCallback) {
                    hr = pCallback->Initialize(punk);

                    if (SUCCEEDED(hr)) {
                        ptsm->Initialize(SAFECAST(pCallback, IShellMenuCallback*), 0, 
                            ANCESTORDEFAULT, SMINIT_VERTICAL | SMINIT_TOPLEVEL);

                        hr = ptsm->SetShellFolder(bi.psf, bi.pidl, NULL, SMSET_COLLAPSEONEMPTY);
                    }
                    pCallback->Release();
                } else {
                    hr = E_OUTOFMEMORY;
                }

                bi.psf->Release();
                ILFree(bi.pidl);
            }

            psfb->Release();
        }
    }

    return hr;
}


// An API for internal use.
HRESULT ToolbarMenu_Popup(HWND hwnd, LPRECT prc, IUnknown* punk, HWND hwndTB, int idMenu, DWORD dwFlags)
{
    HRESULT hres = E_OUTOFMEMORY;
    CTrackShellMenu* ptsm = new CTrackShellMenu();

    if (ptsm)
    {
        hres = S_OK;
        if (IsISFBand(punk))
        {
            hres = DoISFBandStuff(ptsm, punk);
        }
        else if (hwndTB)
        {
            ptsm->Initialize(NULL, 0, ANCESTORDEFAULT, SMINIT_TOPLEVEL | SMINIT_VERTICAL | SMINIT_RESTRICT_DRAGDROP);
            hres = ptsm->SetObscured(hwndTB, punk, SMSET_TOP);
        }

        IUnknown* punkSite;
        if (SUCCEEDED(IUnknown_GetSite(punk, IID_IUnknown, (void**)&punkSite)))
            ptsm->SetSite(punkSite);

        HMENU hmenu = idMenu ? LoadMenuPopup(idMenu) : NULL;

        if (SUCCEEDED(hres) && hmenu)
            hres = ptsm->SetMenu(hmenu, hwnd, SMSET_BOTTOM);

        if (SUCCEEDED(hres))
        {
            DWORD dwPopupFlags = MPPF_BOTTOM;

            // select first/last menu item if specified
            if (dwFlags == DBPC_SELECTFIRST)
            {
                dwPopupFlags |= MPPF_INITIALSELECT;
            }
            else if (dwFlags == DBPC_SELECTLAST)
            {
                dwPopupFlags |= MPPF_FINALSELECT;
            }
            else if (dwFlags != 0)
            {
                VARIANT var;
                var.vt = VT_I4;
                var.lVal = dwFlags;
                IUnknown_QueryServiceExec(SAFECAST(ptsm, IShellMenu*), SID_SMenuBandChild, &CGID_MenuBand, MBANDCID_SELECTITEM, 0, &var, NULL);
            }

            POINTL ptPop = {prc->left, prc->bottom};
            hres = ptsm->Popup(hwnd, &ptPop, (LPRECTL)prc, dwPopupFlags);
        }

        ptsm->SetSite(NULL);
        ptsm->Release();
    }
    return hres;
}
