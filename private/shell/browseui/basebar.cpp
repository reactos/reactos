#include "priv.h"
#include "apithk.h"
#include "mshtmhst.h"
#include "basebar.h"
#include "inpobj.h"

#ifdef MAINWIN
#include <mainwin.h>
EXTERN_C MwPaintSpecialEOBorder( HWND hWnd, HDC hDC );
#endif

#define DBM_ONPOSRECTCHANGE  (WM_USER)


//*** CBaseBar::IDeskBar::* {
//


/*----------------------------------------------------------
Purpose: IDeskBar::SetClient

         Usually the function that composes a bar/bandsite/band
         union is responsible for calling this method to inform
         the bar what the client (bandsite) is.

*/
HRESULT CBaseBar::SetClient(IUnknown *punkChild)
{
    if (_punkChild != NULL) {
        // 4, 3, 2, 1 Release
        _hwndChild = NULL;

        if (_pDBC)
        {
            // This must happen first, before _pWEH becomes NULL so cleanup
            // notifications can still go thru
            _pDBC->SetDeskBarSite(NULL);
        }

        ATOMICRELEASE(_pDBC);

        ATOMICRELEASE(_pWEH);

        ATOMICRELEASE(_punkChild);
    }

    _punkChild = punkChild;

    if (_punkChild != NULL) {
        HRESULT hr;

       // 1, 2, 3, 4 QI/AddRef/etc.
       _punkChild->AddRef();
       if (!_hwnd) {
            _RegisterDeskBarClass();
            _CreateDeskBarWindow();
            if (!_hwnd) {
                return E_OUTOFMEMORY;
            }

            // can't do CBaseBar::_Initialize yet (haven't done SetSite yet)
        }


        hr = _punkChild->QueryInterface(IID_IWinEventHandler, (LPVOID*)&_pWEH);
        ASSERT(SUCCEEDED(hr));

        hr = _punkChild->QueryInterface(IID_IDeskBarClient, (LPVOID*)&_pDBC);
        ASSERT(SUCCEEDED(hr));

        // nothing to cache yet due to lazy CreateWindow
        hr = _pDBC->SetDeskBarSite(SAFECAST(this, IDeskBar*));

        IUnknown_GetWindow(_punkChild, &_hwndChild);
    }

    return S_OK;
}

HRESULT CBaseBar::GetClient(IUnknown **ppunk)
{
    *ppunk = _punkChild;
    if (_punkChild)
        _punkChild->AddRef();
    return _punkChild ? S_OK : E_FAIL;
}

HRESULT CBaseBar::OnPosRectChangeDB(LPRECT prc)
{
    MSG msg;

    _szChild.cx = RECTWIDTH(*prc);
    _szChild.cy = RECTHEIGHT(*prc);

    // We can't change our size right away because we haven't returned from processing
    // this WM_SIZE message. If we resize right now, USER gets confused...
    //
    if (!PeekMessage(&msg, _hwnd, DBM_ONPOSRECTCHANGE, DBM_ONPOSRECTCHANGE, PM_NOREMOVE))
        PostMessage(_hwnd, DBM_ONPOSRECTCHANGE, 0, 0);

    return S_OK;
}

void CBaseBar::_OnPostedPosRectChange()
{
}

// }

HRESULT CBaseBar::ShowDW(BOOL fShow)
{
    fShow = BOOLIFY(fShow);

    if (BOOLIFY(_fShow) == fShow)
        return S_OK;

    _fShow = fShow;

    if (_pDBC)
        return _pDBC->UIActivateDBC(fShow ? DBC_SHOW : DBC_HIDE);
    else
        return E_UNEXPECTED;
}

void CBaseBar::_OnCreate()
{
    SendMessage(_hwnd, WM_CHANGEUISTATE, MAKEWPARAM(UIS_INITIALIZE, 0), 0);
}

LRESULT CBaseBar::_OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lres = 0;

    _CheckForwardWinEvent(uMsg, wParam, lParam, &lres);

    return lres;
}


/***
 */
LRESULT CBaseBar::v_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lres = 0;

    switch (uMsg) {
    case WM_CREATE:
        _OnCreate();
        break;

    case WM_COMMAND:
        return _OnCommand(uMsg, wParam, lParam);
        

    case WM_SIZE:     
        _OnSize();    
        break;

    case WM_NOTIFY:
        return _OnNotify(uMsg, wParam, lParam);

#if 0
    // we'd like to set focus to the 1st band when somebody clicks in
    // 'dead space' on the bar (i.e. make it look like they TABed in).
    // however for some reason the below code has the bad effect of
    // de-selecting text in (e.g.) the addr edit control (it's as if
    // the control thinks we've clicked there 2x rather than 1x).
    case WM_SETFOCUS:
        if (UnkHasFocusIO(_pDBC) == S_FALSE)
            UnkUIActivateIO(_pDBC, TRUE, NULL);
        break;
#endif
        
    case WM_SYSCOLORCHANGE:
    case WM_WININICHANGE:
    case WM_CONTEXTMENU:
    case WM_PALETTECHANGED:
        _CheckForwardWinEvent(uMsg, wParam, lParam, &lres);
        break;

    case DBM_ONPOSRECTCHANGE:
        _OnPostedPosRectChange();
        break;

#ifdef MAINWIN
    case WM_NCPAINTSPECIALFRAME:
        // In  case  of  motif look  the  MwPaintBorder paints a Etched In
        // border if WM_NCPAINTSPECIALFRAME returns FALSE. We are handling
        // this message here and drawing the Etched Out frame explicitly.
        // wParam - HDC
        if (MwCurrentLook() == LOOK_MOTIF)
        {
            MwPaintSpecialEOBorder( hwnd, (HDC)wParam );
            return TRUE;
        }
#endif

    default:
        return DefWindowProcWrap(hwnd, uMsg, wParam, lParam);
    }

    return lres;
}

/***
 */
CBaseBar::CBaseBar() : _cRef(1)
{
    DllAddRef();
}

/***
 */
CBaseBar::~CBaseBar()
{
    // see Release, where we call virtuals (which can't be called from dtor)
    DllRelease();
}

/***
 */
void CBaseBar::_RegisterDeskBarClass()
{
    WNDCLASS  wc = {0};
    wc.style            = _GetClassStyle();
    wc.lpfnWndProc      = s_WndProc;
    //wc.cbClsExtra       = 0;
    wc.cbWndExtra       = SIZEOF(CBaseBar*);
    wc.hInstance        = HINST_THISDLL;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH) (COLOR_3DFACE+1);
    //wc.lpszMenuName     =  NULL;
    wc.lpszClassName    = TEXT("BaseBar");
    //wc.hIcon            = NULL;

    SHRegisterClass(&wc);
}

DWORD CBaseBar::_GetExStyle()
{
    return WS_EX_TOOLWINDOW;
}

DWORD CBaseBar::_GetClassStyle()
{
    return 0;
}

void CBaseBar::_CreateDeskBarWindow()
{
#ifndef UNIX
    // _hwnd is set in s_WndProc
    DWORD dwExStyle = _GetExStyle();    
    dwExStyle |= IS_BIDI_LOCALIZED_SYSTEM() ? dwExStyleRTLMirrorWnd : 0L;
    HWND hwndDummy = CreateWindowEx(
                                    dwExStyle,
                                    TEXT("BaseBar"), NULL,
                                    _hwndSite ? WS_CHILD | WS_CLIPCHILDREN : WS_POPUP | WS_CLIPCHILDREN,
                                    0,0,100,100,
                                    _hwndSite, NULL, HINST_THISDLL,
                                    (LPVOID)SAFECAST(this, CImpWndProc*));
#else
    // This change removes a flash at the corner of the
    // screen. We create a small 1,1 window.

    HWND hwndDummy = CreateWindowEx(
                                    _GetExStyle(),
                                    TEXT("BaseBar"), NULL,
                                    _hwndSite ? WS_CHILD | WS_CLIPCHILDREN : WS_POPUP | WS_CLIPCHILDREN,
                                    -100,-100,1,1,
                                    _hwndSite, NULL, HINST_THISDLL,
                                    (LPVOID)SAFECAST(this, CImpWndProc*));
#endif
}


void CBaseBar::_OnSize(void)
{
    RECT rc;

    if (!_hwndChild)
        return;

    GetClientRect(_hwnd, &rc);
    SetWindowPos(_hwndChild, 0,
            rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc),
            SWP_NOACTIVATE|SWP_NOZORDER);
}

void CBaseBar::_NotifyModeChange(DWORD dwMode)
{
    if (_pDBC) {
        _dwMode = dwMode;
        // BUGBUG should we add an STBBIF_VIEWMODE_FLOAT?
        _pDBC->SetModeDBC(_dwMode);
    }
}

BOOL CBaseBar::_CheckForwardWinEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres)
{
    HWND hwnd = NULL;

    *plres = 0;
    switch (uMsg) {
    case WM_CONTEXTMENU:
        hwnd = _hwndChild;
        break;

    case WM_NOTIFY:
        hwnd = ((LPNMHDR)lParam)->hwndFrom;
        break;
        
    case WM_COMMAND:
        hwnd = GET_WM_COMMAND_HWND(wParam, lParam);
        break;
        
    case WM_SYSCOLORCHANGE:
    case WM_WININICHANGE:
    case WM_PALETTECHANGED:
        hwnd = _hwndChild;
        break;
    }
    
    if (hwnd && _pWEH && _pWEH->IsWindowOwner(hwnd) == S_OK) {
        _pWEH->OnWinEvent(_hwnd, uMsg, wParam, lParam, plres);
        return TRUE;
    }
    return FALSE;
}

/***
 */
LRESULT CBaseBar::_OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lres = 0;

    _CheckForwardWinEvent(uMsg, wParam, lParam, &lres);

    return lres;
}

HRESULT CBaseBar::CloseDW(DWORD dwReserved)
{
    SetClient(NULL);
    if (_hwnd) {
        DestroyWindow(_hwnd);
        _hwnd = NULL;
    }
    return S_OK;
}


HRESULT CBaseBar::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CBaseBar, IOleWindow),
        QITABENT(CBaseBar, IDeskBar),
        QITABENT(CBaseBar, IInputObject),
        QITABENT(CBaseBar, IInputObjectSite),
        QITABENT(CBaseBar, IServiceProvider),
        QITABENT(CBaseBar, IOleCommandTarget),
        { 0 },
    };

    return QISearch(this, (LPCQITAB)qit, riid, ppvObj);
}


ULONG CBaseBar::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CBaseBar::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    // 'virtual dtor'
    // gotta do virtual stuff here (not in dtor) because can't call
    // any virtuals in the dtor
    // CBaseBar::Destroy() {
    CloseDW(0);
    // }

    delete this;
    return 0;
}

//*** CBaseBar::IOleWindow::* {
//

HRESULT CBaseBar::GetWindow(HWND * lphwnd)
{
    *lphwnd = _hwnd;
    return (_hwnd) ? S_OK : E_FAIL;
}

HRESULT CBaseBar::ContextSensitiveHelp(BOOL fEnterMode)
{
    // BUGBUG: Visit here later.
    return E_NOTIMPL;
}
// }


// }
// some helpers... {

// BUGBUG REVIEW: What's the point of having
// these empty implementations in the base class?
//

//*** CBaseBar::IServiceProvider::*
//
HRESULT CBaseBar::QueryService(REFGUID guidService,
                                REFIID riid, void **ppvObj)
{
    HRESULT hres = E_FAIL;
    *ppvObj = NULL;

    return hres;
}

//*** CBaseBar::IOleCommandTarget::*
//
HRESULT CBaseBar::QueryStatus(const GUID *pguidCmdGroup,
    ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    return MayQSForward(_pDBC, OCTD_DOWN, pguidCmdGroup, cCmds, rgCmds, pcmdtext);
}

HRESULT CBaseBar::Exec(const GUID *pguidCmdGroup,
    DWORD nCmdID, DWORD nCmdexecopt,
    VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    return MayExecForward(_pDBC, OCTD_DOWN, pguidCmdGroup, nCmdID, nCmdexecopt,
        pvarargIn, pvarargOut);
}

// }


//*** CDeskBar::IInputObject::* {

HRESULT CBaseBar::HasFocusIO()
{
    HRESULT hres;

    hres = UnkHasFocusIO(_pDBC);
    return hres;
}

HRESULT CBaseBar::TranslateAcceleratorIO(LPMSG lpMsg)
{
    HRESULT hres;

    hres = UnkTranslateAcceleratorIO(_pDBC, lpMsg);
    return hres;
}

HRESULT CBaseBar::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    HRESULT hres;

    hres = UnkUIActivateIO(_pDBC, fActivate, lpMsg);
    return hres;
}

// }

//***   CDeskBar::IInputObjectSite::* {

HRESULT CBaseBar::OnFocusChangeIS(IUnknown *punk, BOOL fSetFocus)
{
    return NOERROR;
}

// }

