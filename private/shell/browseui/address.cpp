/**************************************************************\
    FILE: address.cpp

    DESCRIPTION:
        The Class CAddressBand exists to support the Address
    ToolBand in either the main browser toolbar or as a
    ShellToolBand.
\**************************************************************/

#include "priv.h"
#include "sccls.h"
#include "dbgmem.h"
#include "addrlist.h"
#include "itbar.h"
#include "itbdrop.h"
#include "util.h"
#include "aclhist.h"
#include "aclmulti.h"
#include "autocomp.h"
#include "address.h"
#include "inpobj.h"
#include "shellurl.h"
#include "resource.h"
#include "uemapp.h"

#include "mluisupp.h"

#define SUPERCLASS CToolBand
#define MIN_DROPWIDTH 200
const static TCHAR c_szAddressBandProp[]   = TEXT("CAddressBand_This");


//=================================================================
// Implementation of CAddressBand
//=================================================================

//===========================
// *** IUnknown Interface ***

HRESULT CAddressBand::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IWinEventHandler))
    {
        *ppvObj = SAFECAST(this, IWinEventHandler*);
    }
    else if (IsEqualIID(riid, IID_IAddressBand))
    {
        *ppvObj = SAFECAST(this, IAddressBand*);
    }
    else if (IsEqualIID(riid, IID_IPersistStream))
    {
        *ppvObj = SAFECAST(this, IPersistStream*);
    }
    else if (IsEqualIID(riid, IID_IServiceProvider))
    {
        *ppvObj = SAFECAST(this, IServiceProvider*);
    }
    else if (IsEqualIID(riid, IID_IInputObjectSite))
    {
        *ppvObj = SAFECAST(this, IInputObjectSite*);
    }
    else
    {
        return SUPERCLASS::QueryInterface(riid, ppvObj);
    }

    AddRef();
    return S_OK;
}


//================================
// *** IDockingWindow Interface ***
/****************************************************\
    FUNCTION: ShowDW

    DESCRIPTION:
        fShow == TRUE means show the window, FALSE means
    remove the window from the view.  The window will
    be created if needed.
\****************************************************/
HRESULT CAddressBand::ShowDW(BOOL fShow)
{
    if (!_hwnd)
        return S_FALSE; // The window needs to be created first.

    ShowWindow(_hwnd, fShow ? SW_SHOW : SW_HIDE);

    // Refresh if we are becoming visible because we could have
    // received and ignored FileSysChange() events while
    // we where hidden.
    if (fShow && !_fVisible)
        Refresh(NULL);

    _fVisible = BOOLIFY(fShow);
    return SUPERCLASS::ShowDW(fShow);
}


HRESULT CAddressBand::CloseDW(DWORD dw)
{
    if(_paeb)
        _paeb->Save(0);

    return SUPERCLASS::CloseDW(dw);
}



/****************************************************\
    FUNCTION: SetSite

    DESCRIPTION:
        This function will be called to have this
    Toolband try to obtain enough information about its
    parent Toolbar to create the Band window and maybe
    connect to a Browser Window.
\****************************************************/
HRESULT CAddressBand::SetSite(IUnknown *punkSite)
{
    HRESULT hr;
    BOOL fSameHost = punkSite == _punkSite;

    if (!punkSite && _paeb)
    {
        IShellService * pss;

        hr = _paeb->QueryInterface(IID_IShellService, (LPVOID *)&pss);
        if (SUCCEEDED(hr))
        {
            hr = pss->SetOwner(NULL);
            pss->Release();
        }
    }

    hr = SUPERCLASS::SetSite(punkSite);
    if (punkSite && !fSameHost)
    {
        hr = _CreateAddressBand(punkSite);
        // This call failing is expected when the host doesn't have a Browser Window.
    }

    // Set or reset the AddressEditBox's Browser IUnknown.
    if (_paeb)
    {
        IShellService * pss;

        hr = _paeb->QueryInterface(IID_IShellService, (LPVOID *)&pss);
        if (SUCCEEDED(hr))
        {
            // CAddressBand and the BandSite(host) have a ref count cycle.  This cycle
            // is broken when BandSite calls SetSite(NULL) which will cause
            // CAddressBand to break the cycle by releasing it's punk to the BandSite.
            //
            // CAddressEditBox and CAddressBand have the same method of breaking the
            // cycle.  This is accomplished by passing NULL to IAddressEditBox(NULL, NULL)
            // if our caller is breaking the cycle.  This will cause CAddressEditBox to
            // release it's ref count on CAddressBand.
            hr = pss->SetOwner((punkSite ? SAFECAST(this, IAddressBand *) : NULL));
            pss->Release();
        }
    }

    return hr;
}


//================================
// *** IInputObject Methods ***
HRESULT CAddressBand::TranslateAcceleratorIO(LPMSG lpMsg)
{
    BOOL fForwardToView = FALSE;
    static CHAR szAccel[2] = "\0"; // Alt-D needs to be localizable

    switch (lpMsg->message)
    {
    case WM_KEYDOWN:    // process these
        if (IsVK_TABCycler(lpMsg))
        {
            // If we are tabbing away, let the edit box know so
            // that it clears its dirty flag.
            SendMessage(_hwndEdit, WM_KEYDOWN, VK_TAB, 0);
        }
        else
        {
            fForwardToView = TRUE;
        }

        switch (lpMsg->wParam)
        {
            case VK_F1:     // help
            {
                //
                // bugbug: Should add and accelerator for this and simply return S_FALSE, but that
                // causes two instances of the help dialog to come up when focus is in Trident.
                // This is the quick fix for IE5B2.
                //
                IOleCommandTarget* poct;
                IServiceProvider* psp;
                if (_punkSite && SUCCEEDED(_punkSite->QueryInterface(IID_IServiceProvider, (void**)&psp)))
                {
                    if (SUCCEEDED(psp->QueryService(SID_STopLevelBrowser, IID_IOleCommandTarget, (LPVOID*)&poct)))
                    {
                        poct->Exec(&CGID_ShellBrowser, DVIDM_HELPSEARCH, 0, NULL, NULL);
                        poct->Release();
                    }
                    psp->Release();
                }
                return S_FALSE;
            }
            case VK_F11:    // fullscreen
            {
                return S_FALSE;
            }

            case VK_F4:
            {
                if (_fVisible)
                {
                    if (HasFocusIO() == S_FALSE)
                        SetFocus(_hwnd);

                    // toggle the dropdown state
                    SendMessage(_hwnd, CB_SHOWDROPDOWN,
                                !SendMessage(_hwnd, CB_GETDROPPEDSTATE, 0, 0L), 0);

                    // Leave focus in the edit box so you can keep typing
                    if (_hwndEdit)
                        SetFocus(_hwndEdit);
                }
                else
                {
                    ASSERT(0); // Should this really be ignored?
                }

                return S_OK;
            }
            case VK_TAB:
            {
                // See if the editbox wants the tab character
                if (SendMessage(_hwndEdit, WM_GETDLGCODE, lpMsg->wParam, (LPARAM)lpMsg) == DLGC_WANTTAB)
                {
                    // We want the tab character
                    return S_OK;
                }
                break;
            }

            case VK_RETURN:
            {
                //
                // Ctrl-enter is used for quick complete, so pass it through
                //
                if (GetKeyState(VK_CONTROL) & 0x80000000)
                {
                    TranslateMessage(lpMsg);
                    DispatchMessage(lpMsg);
                    return S_OK;
                }
                break;
            }
        }
        break;
    case WM_KEYUP:      // eat any that WM_KEYDOWN processes
        switch (lpMsg->wParam)
        {
            case VK_F1:     // help
            case VK_F11:    // fullscreen
                return S_FALSE;

            case VK_RETURN:
            case VK_F4:
            case VK_TAB:
                return S_OK;
            default:
                break;
        }
        break;

    case WM_SYSCHAR:
        {
            CHAR   szChar [2] = "\0";
            if ('\0' == szAccel[0]) {
                MLLoadStringA(IDS_ADDRBAND_ACCELLERATOR,szAccel,ARRAYSIZE(szAccel));
            }
            szChar[0] = (CHAR)lpMsg->wParam;

            if (lstrcmpiA(szChar,szAccel) == 0)
            {
                ASSERT(_fVisible);
                if (_fVisible && (HasFocusIO() == S_FALSE))
                {
                    SetFocus(_hwnd);
                }
                return S_OK;
            }
        }
        break;

    case WM_SYSKEYUP:   // eat any that WM_SYSKEYDOWN processes
        if ('\0' == szAccel[0]) {
            MLLoadStringA(IDS_ADDRBAND_ACCELLERATOR,szAccel,ARRAYSIZE(szAccel));
        }

        if ((CHAR)lpMsg->wParam == szAccel[0]) {
            return S_OK;
        }
        break;
    }

    HRESULT hres = EditBox_TranslateAcceleratorST(lpMsg);

    if (hres == S_FALSE && fForwardToView)
    {
        IShellBrowser *psb;
        // we did not process this try the view before we return
        if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_IShellBrowser, (void **)&psb)))
        {
            IShellView *psv;

            if (SUCCEEDED(psb->QueryActiveShellView(&psv)))
            {
                hres = psv->TranslateAccelerator(lpMsg);
                psv->Release();
            }
            psb->Release();
        }
    }

    return hres;
}


HRESULT CAddressBand::HasFocusIO()
{
    if ((_hwndEdit&& (GetFocus() == _hwndEdit)) ||
        SendMessage(_hwnd, CB_GETDROPPEDSTATE, 0, 0))
        return S_OK;

    return S_FALSE;
}


//=====================================
// *** IInputObjectSite Interface ***
HRESULT CAddressBand::OnFocusChangeIS(IUnknown *punk, BOOL fSetFocus)
{
    HRESULT hr;

    ASSERT(_punkSite);
    hr = UnkOnFocusChangeIS(_punkSite, punk, fSetFocus);
    return hr;
}


//=====================================
// *** IOleCommandTarget Interface ***
HRESULT CAddressBand::QueryStatus(const GUID *pguidCmdGroup,
    ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    ASSERT(_paeb);
    return IUnknown_QueryStatus(_paeb, pguidCmdGroup, cCmds, rgCmds, pcmdtext);
}


HRESULT CAddressBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt,
                        VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    HRESULT hr = OLECMDERR_E_UNKNOWNGROUP;

    if (pguidCmdGroup && IsEqualGUID(CGID_Explorer, *pguidCmdGroup))
    {

        switch (nCmdID)
        {
        case SBCMDID_GETADDRESSBARTEXT:
            hr = S_OK;

            TCHAR wz[MAX_URL_STRING];
            UINT   cb = 0;
            BSTR   bstr = NULL;
            VariantInit(pvarargOut);

            if (_hwndEdit)
                cb = Edit_GetText(_hwndEdit, (TCHAR *)&wz, ARRAYSIZE(wz));
            if (cb)
                bstr = SysAllocStringLen(NULL, cb);
            if (bstr)
            {
                SHTCharToUnicode(wz, bstr, cb);
                pvarargOut->vt = VT_BSTR|VT_BYREF;
                pvarargOut->byref = bstr;
            }
            else
            {
                // VariantInit() might do this for us.
                pvarargOut->vt = VT_EMPTY;
                pvarargOut->byref = NULL;
                return E_FAIL;   // Edit_GetText gave us nothing
            }
            break;
        }
    }

    if (FAILED(hr))
    {
        hr = IUnknown_Exec(_paeb, pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
    }

    return(hr);
}

extern HRESULT IsDesktopBrowser(IUnknown *punkSite);

//================================
// *** IDeskBand Interface ***
/****************************************************\
    FUNCTION: GetBandInfo

    DESCRIPTION:
        This function will give the caller information
    about this Band, mainly the size of it.
\****************************************************/
HRESULT CAddressBand::GetBandInfo(DWORD dwBandID, DWORD fViewMode,
                                DESKBANDINFO* pdbi)
{
    HRESULT hr  = S_OK;

    _dwBandID = dwBandID;
    _fVertical = ((fViewMode & (DBIF_VIEWMODE_VERTICAL | DBIF_VIEWMODE_FLOATING)) != 0);

    pdbi->dwModeFlags = DBIMF_FIXEDBMP;

    pdbi->ptMinSize.x = 0;
    pdbi->ptMinSize.y = 0;
    if (_fVertical) {
        pdbi->ptMaxSize.y = -1; // random
        pdbi->ptIntegral.y = 1;
        pdbi->dwModeFlags |= DBIMF_VARIABLEHEIGHT;
    } else {
        if (_hwnd) {
            HWND hwndCombo;
            RECT rcCombo;

            hwndCombo = (HWND)SendMessage(_hwnd, CBEM_GETCOMBOCONTROL, 0, 0);
            ASSERT(hwndCombo);
            GetWindowRect(hwndCombo, &rcCombo);
            pdbi->ptMinSize.y = RECTHEIGHT(rcCombo);
        }
        ASSERT(pdbi->ptMinSize.y < 200);

    }

    MLLoadStringW(IDS_BAND_ADDRESS2, pdbi->wszTitle, ARRAYSIZE(pdbi->wszTitle));
    if (IsDesktopBrowser(_punkSite) != S_FALSE) {
        // non- shell browser host (e.g. desktop or tray)
        //
        // this is slightly (o.k., very) hoaky.  the only time we want to
        // show a mnemonic is when we're in a browser app.  arguably we
        // should generalize this to all bands/bandsites by having a
        // DBIMF_WITHMNEMONIC or somesuch, but that would mean adding a
        // CBandSite::_dwModeFlag=0 and overriding it in itbar::CBandSite.
        // that seems like a lot of work for a special case so instead we
        // hack it in here based on knowledge of our host.
        TraceMsg(DM_TRACE, "cab.gbi: nuke Address mnemonic");
        MLLoadStringW(IDS_BAND_ADDRESS, pdbi->wszTitle, ARRAYSIZE(pdbi->wszTitle));
    }

    return hr;
}

//================================
//  ** IWinEventHandler Interface ***
/****************************************************\
    FUNCTION: OnWinEvent

    DESCRIPTION:
        This function will give receive events from
    the parent ShellToolbar.
\****************************************************/
HRESULT CAddressBand::OnWinEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres)
{
    switch (uMsg)
    {
    case WM_WININICHANGE:
        if (SHIsExplorerIniChange(wParam, lParam) & (EICH_KINET | EICH_KINETMAIN))
        {
            _InitGoButton();
        }

        if (wParam == SPI_SETNONCLIENTMETRICS)
        {
            // Tell the combobox so that it can update its font
            SendMessage(_hwnd, uMsg, wParam, lParam);

            // Inform the band site that our height may have changed
            _BandInfoChanged();
        }
        break;

    case WM_COMMAND:
        {
            UINT idCmd = GET_WM_COMMAND_ID(wParam, lParam);
            if (idCmd == FCIDM_VIEWGOBUTTON)
            {
                // Toggle the go-button visibility
                BOOL fShowGoButton = !SHRegGetBoolUSValue(REGSTR_PATH_MAIN,
                    TEXT("ShowGoButton"), FALSE, /*default*/TRUE);

                SHRegSetUSValue(REGSTR_PATH_MAIN,
                            TEXT("ShowGoButton"),
                            REG_SZ,
                            (LPVOID)(fShowGoButton ? L"yes" : L"no"),
                            (fShowGoButton ? 4 : 3)*sizeof(TCHAR),
                            SHREGSET_FORCE_HKCU);

                // Tell the world that something has changed
                SendShellIEBroadcastMessage(WM_WININICHANGE, 0, (LPARAM)REGSTR_PATH_MAIN, 3000);
            }
        }
    }

    if (_pweh)
        return _pweh->OnWinEvent(_hwnd, uMsg, wParam, lParam, plres);
    else
        return S_OK;
}


/****************************************************\
    FUNCTION: IsWindowOwner

    DESCRIPTION:
        This function will return TRUE if the HWND
    passed in is a HWND owned by this band.
\****************************************************/
HRESULT CAddressBand::IsWindowOwner(HWND hwnd)
{
    if (_pweh)
        return _pweh->IsWindowOwner(hwnd);
    else
        return S_FALSE;
}


//================================
// *** IAddressBand Interface ***
/****************************************************\
    FUNCTION: FileSysChange

    DESCRIPTION:
        This function will handle file system change
    notifications.
\****************************************************/
HRESULT CAddressBand::FileSysChange(DWORD dwEvent, LPCITEMIDLIST * ppidl)
{
    HRESULT hr = S_OK;

    if (_fVisible)
    {
        hr = IUnknown_FileSysChange(_paeb, dwEvent, ppidl);
    }
    return hr;
}


/****************************************************\
    FUNCTION: Refresh

    PARAMETERS:
        pvarType - NULL for a refress of everything.
                   OLECMD_REFRESH_TOPMOST will only update the top most.

    DESCRIPTION:
        This function will force a refress of part
    or all of the AddressBand.
\****************************************************/
HRESULT CAddressBand::Refresh(VARIANT * pvarType)
{
    HRESULT hr = S_OK;
    IAddressBand * pab;

    if (_paeb)
    {
        hr = _paeb->QueryInterface(IID_IAddressBand, (LPVOID *)&pab);
        if (SUCCEEDED(hr))
        {
            hr = pab->Refresh(pvarType);
            pab->Release();
        }
    }

    return hr;
}

/****************************************************\
    Address Band Constructor
\****************************************************/
CAddressBand::CAddressBand()
{
    TraceMsg(TF_SHDLIFE, "ctor CAddressBand %x", this);

    // This needs to be allocated in Zero Inited Memory.
    // ASSERT that all Member Variables are inited to Zero.
    ASSERT(!_hwndEdit);
    ASSERT(!_paeb);
    ASSERT(!_pweh);

    _fCanFocus = TRUE;      // we accept focus (see CToolBand::UIActivateIO)
}


/****************************************************\
    Address Band destructor
\****************************************************/
CAddressBand::~CAddressBand()
{
    ATOMICRELEASE(_paeb);
    ATOMICRELEASE(_pweh);

    //
    // Make sure the toolbar is destroyed before we free
    // the image lists
    //
    if (_hwndTools && IsWindow(_hwndTools))
    {
        DestroyWindow(_hwndTools);
    }
    if (_himlDefault) ImageList_Destroy(_himlDefault);
    if (_himlHot)  ImageList_Destroy(_himlHot);

    //
    // Our window must be destroyed before we are freed
    // so that the window doesn't try to reference us.
    //
    if (_hwnd && IsWindow(_hwnd))
    {
        DestroyWindow(_hwnd);

        // Null out base classes window handle because
        // its destructor is next
        _hwnd = NULL;
    }

    TraceMsg(TF_SHDLIFE, "dtor CAddressBand %x", this);
}


/****************************************************\
    FUNCTION: CAddressBand_CreateInstance

    DESCRIPTION:
        This function will create an instance of the
    AddressBand COM object.
\****************************************************/
HRESULT CAddressBand_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory

    *ppunk = NULL;
    CAddressBand * p = new CAddressBand();
    if (p)
    {
        *ppunk = SAFECAST(p, IDeskBand *);
        return NOERROR;
    }

    return E_OUTOFMEMORY;
}


/****************************************************\
    FUNCTION: _CreateAddressBand

    DESCRIPTION:
        This function will create the AddressBand window
    with the ComboBox.
\****************************************************/
HRESULT CAddressBand::_CreateAddressBand(IUnknown * punkSite)
{
    HRESULT hr = S_OK;

    if (_hwnd)
    {
        IShellService * pss;

        if (_hwndTools)
        {
            DestroyWindow(_hwndTools);
            _hwndTools = NULL;
        }

        DestroyWindow(_hwnd);
        _hwnd = NULL;

        ASSERT(_punkSite);
        if (_paeb)
        {
            hr = _paeb->QueryInterface(IID_IShellService, (LPVOID *)&pss);
            if (SUCCEEDED(hr))
            {
                hr = pss->SetOwner(NULL);
                pss->Release();
            }
        }
        ATOMICRELEASE(_paeb);
        ATOMICRELEASE(_pweh);
    }

    //
    // Create address window.
    //

    ASSERT(_hwndParent);        // Call us after SetSite()
    if (!_hwndParent)
    {
            // The caller hasn't called SetSite(), so we can't
            // create our window because we can't find out our parent's
            // HWND.
            return E_FAIL;
    }
    _InitComCtl32();    // don't check result, if this fails our CreateWindows will fail


    DWORD dwWindowStyles = WS_TABSTOP | WS_CHILD | WS_CLIPCHILDREN | WS_TABSTOP | CBS_DROPDOWN | CBS_AUTOHSCROLL;

    // WARNING: MSN and other Rooted Explorers may not have implemented all
    // of the ParseDisplayName and other IShellFolder members
    // If we want to continue to support MSN, we will need to turn on the
    // CBS_DROPDOWNLIST if ISROOTEDCLASS() and the clsid is equal to the MSN clsid.

    // dwWindowStyles |= CBS_DROPDOWNLIST; // (This turns off the ComboBox's Editbox)

    DWORD dwExStyle = WS_EX_TOOLWINDOW;

    if (IS_WINDOW_RTL_MIRRORED(_hwndParent))
    {
        // If the parent window is mirrored then the ComboBox window will inheret the mirroring flag
        // And we need the reading order to be Left to right, which is the right to left in the mirrored mode.
        dwExStyle |= WS_EX_RTLREADING;
    }

    _hwnd = CreateWindowEx(dwExStyle, WC_COMBOBOXEX, NULL, dwWindowStyles,
                           0, 0, 100, 250, _hwndParent,
                           (HMENU) FCIDM_VIEWADDRESS, HINST_THISDLL, NULL);

    if (EVAL(_hwnd))
    {
        //
        // Initial combobox parameters.
        //
        SendMessage(_hwnd, CBEM_SETEXTENDEDSTYLE,
                CBES_EX_NOSIZELIMIT | CBES_EX_CASESENSITIVE,
                CBES_EX_NOSIZELIMIT | CBES_EX_CASESENSITIVE);

        // NOTE: _hwndEdit will be NULL if the CBS_DROPDOWNLIST flag has been turned on
        _hwndEdit  = (HWND)SendMessage(_hwnd, CBEM_GETEDITCONTROL, 0, 0L);
        _hwndCombo = (HWND)SendMessage(_hwnd, CBEM_GETCOMBOCONTROL, 0, 0L);

        ASSERT(!_paeb && !_pweh);
        hr = CoCreateInstance(CLSID_AddressEditBox, NULL, CLSCTX_INPROC_SERVER, IID_IAddressEditBox, (void **)&_paeb);
        // If this object fails to initialize, it won't work!!!  Make sure you REGSVR32ed and RUNDLL32ed shdocvw.dll
        ASSERT(SUCCEEDED(hr));
        if (SUCCEEDED(hr))
        {
            hr = _paeb->QueryInterface(IID_IWinEventHandler, (LPVOID *)&_pweh);
            ASSERT(SUCCEEDED(hr));
            hr = _paeb->Init(_hwnd, _hwndEdit, AEB_INIT_AUTOEXEC, SAFECAST(this, IAddressBand *));
        }

        //
        // Create the go button if it's enabled
        //
        _InitGoButton();
    }
    else
    {
        hr = E_OUTOFMEMORY;
        TraceMsg(TF_ERROR, "CAddressBand::_CreateAddressBand() Could not create Address Band");
    }

    return hr;
}


//================================
// *** IPersistStream Interface ***

/****************************************************\
    FUNCTION: Load

    DESCRIPTION:
        This function will currently only persist the
    CAddressEditBox object.

    HISTORY:
    Ver 1: Contains the CAddressEditBox::Save() stream.
\****************************************************/
#define STREAM_VERSION_CADDRESSBAND      0x00000001

HRESULT CAddressBand::Load(IStream *pstm)
{
    HRESULT hr;
    DWORD dwSize;
    DWORD dwVersion;

    hr = LoadStreamHeader(pstm, STREAMHEADER_SIG_CADDRESSBAND, STREAM_VERSION_CADDRESSBAND,
        STREAM_VERSION_CADDRESSBAND, &dwSize, &dwVersion);
    ASSERT(SUCCEEDED(hr));

    if (S_OK == hr)
    {
        switch (dwVersion)
        {
        case 1:     // Ver 1.
            // Nothing.
            break;
        default:
            ASSERT(0);  // Should never get here.
            break;
        }
    }
    else if (S_FALSE == hr)
        hr = S_OK;  // We already have our default data set.

    return hr;
}


/****************************************************\
    FUNCTION: Save

    DESCRIPTION:
        This function will currently only persist the
    CAddressEditBox object.

    HISTORY:
    Ver 1: Contains the CAddressEditBox::Save() stream.
\****************************************************/
HRESULT CAddressBand::Save(IStream *pstm, BOOL fClearDirty)
{
    HRESULT hr;

    hr = SaveStreamHeader(pstm, STREAMHEADER_SIG_CADDRESSBAND,
                STREAM_VERSION_CADDRESSBAND, 0);
    ASSERT(SUCCEEDED(hr));

    if (SUCCEEDED(hr))
    {
        IPersistStream * pps;

        ASSERT(_paeb);
        if (_paeb)
        {
            hr = _paeb->QueryInterface(IID_IPersistStream, (LPVOID *)&pps);
            if(EVAL(SUCCEEDED(hr)))
            {
                hr = pps->Save(pstm, fClearDirty);
                pps->Release();
            }
        }
    }

    return hr;
}


//+-------------------------------------------------------------------------
// Subclassed window procedure of the combobox in the address band
//--------------------------------------------------------------------------
LRESULT CALLBACK CAddressBand::_ComboExWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CAddressBand* pThis = (CAddressBand*)GetProp(hwnd, c_szAddressBandProp);

    if (!pThis)
        return DefWindowProcWrap(hwnd, uMsg, wParam, lParam);

    WNDPROC pfnOldWndProc = pThis->_pfnOldWndProc;

    switch (uMsg)
    {
    case WM_NOTIFYFORMAT:
        if (NF_QUERY == lParam)
        {
            return (DLL_IS_UNICODE ? NFR_UNICODE : NFR_ANSI);
        }
        break;

    case WM_WINDOWPOSCHANGING:
        {
            // Break out if the go button is hidden
            if (!pThis->_fGoButton)
                break;

            //
            // Make room for the go button on the right side
            //
            WINDOWPOS wp = *(LPWINDOWPOS)lParam;

            // Get the dimensions of our 'go' button
            RECT rc;
            SendMessage(pThis->_hwndTools, TB_GETITEMRECT, 0, (LPARAM)&rc);
            int cxGo = RECTWIDTH(rc);
            int cyGo = RECTHEIGHT(rc);

            // Make room for the go button on the right side
            wp.cx -= cxGo + 2;
            CallWindowProc(pfnOldWndProc, hwnd, uMsg, wParam, (LPARAM)&wp);

            // Paint underneath the 'go' button
            RECT rcGo = {wp.cx, 0, wp.cx + cxGo + 2, wp.cy};
            InvalidateRect(pThis->_hwnd, &rcGo, TRUE);

            // The outer window can be much higher than the internal combobox.
            // We want to center the go button on the combobox
            int y;
            if (pThis->_hwndCombo)
            {
                // Center vertically with inner combobox
                RECT rcCombo;
                GetWindowRect(pThis->_hwndCombo, &rcCombo);
                y = (rcCombo.bottom - rcCombo.top - cyGo)/2;
            }
            else
            {
                y = (wp.cy - cyGo)/2;
            }

            // Position the 'go' button on the right.  Note that the height will always be ok
            // because the addressbar displays 16x16 icons within it.
            SetWindowPos(pThis->_hwndTools, NULL, wp.cx + 2, y, cxGo, cyGo, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOOWNERZORDER);

            // Adjust the drop-down width
            SendMessage(pThis->_hwndCombo, CB_SETDROPPEDWIDTH, MIN_DROPWIDTH, 0L);
            return 0;
        }
    case WM_SIZE:
        {
            // Break out if the go button is hidden
            if (!pThis->_fGoButton)
                break;

            //
            // Make room for the go button on the right side
            //
            int cx = LOWORD(lParam);
            int cy = HIWORD(lParam);

            // Get the dimensions of our 'go' button
            RECT rc;
            SendMessage(pThis->_hwndTools, TB_GETITEMRECT, 0, (LPARAM)&rc);
            int cxGo = RECTWIDTH(rc);
            int cyGo = RECTHEIGHT(rc);

            // Make room for the go button on the right side
            LPARAM lParamTemp = MAKELONG(cx - cxGo - 2, cy);
            CallWindowProc(pfnOldWndProc, hwnd, uMsg, wParam, lParamTemp);

            // Paint underneath the 'go' button
            RECT rcGo = {cx-cxGo, 0, cx, cy};
            InvalidateRect(pThis->_hwnd, &rcGo, TRUE);

            // The outer window can be much higher than the internal combobox.
            // We want to center the go button on the combobox
            int y;
            if (pThis->_hwndCombo)
            {
                // Center vertically with inner combobox
                RECT rcCombo;
                GetWindowRect(pThis->_hwndCombo, &rcCombo);
                y = (rcCombo.bottom - rcCombo.top - cyGo)/2;
            }
            else
            {
                y = (cy - cyGo)/2;
            }

            // Position the 'go' button on the right.  Note that the height will always be ok
            // because the addressbar displays 16x16 icons within it.
            SetWindowPos(pThis->_hwndTools, NULL, cx - cxGo, y, cxGo, cyGo, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOOWNERZORDER);

            // Adjust the drop-down width
            SendMessage(pThis->_hwndCombo, CB_SETDROPPEDWIDTH, MIN_DROPWIDTH, 0L);
            return 0;
        }
    case WM_NOTIFY:
        {
            LPNMHDR pnm = (LPNMHDR)lParam;
            if (pnm->hwndFrom == pThis->_hwndTools)
            {
                switch (pnm->code)
                {
                case NM_CLICK:
                    // Simulate an enter key press in the combobox
                    SendMessage(pThis->_hwndEdit, WM_KEYDOWN, VK_RETURN, 0);
                    SendMessage(pThis->_hwndEdit, WM_KEYUP, VK_RETURN, 0);
                    // n.b. we also got a NAVADDRESS from the simulate
                    UEMFireEvent(&UEMIID_BROWSER, UEME_INSTRBROWSER, UEMF_INSTRUMENT, UIBW_NAVIGATE, UIBL_NAVGO);
                    break;

                case NM_TOOLTIPSCREATED:
                {
                    //
                    // Make the tooltip show up even when the app is nit active
                    //
                    NMTOOLTIPSCREATED* pnmTTC = (NMTOOLTIPSCREATED*)pnm;
                    SHSetWindowBits(pnmTTC->hwndToolTips, GWL_STYLE, TTS_ALWAYSTIP | TTS_TOPMOST | TTS_NOPREFIX, TTS_ALWAYSTIP | TTS_TOPMOST | TTS_NOPREFIX);
                }
                break;
                case TBN_GETINFOTIP:
                    //
                    // Format a tooltip: "go to <contents of address bar>"
                    //
                    LPNMTBGETINFOTIP pnmTT = (LPNMTBGETINFOTIP)pnm;
                    WCHAR szAddress[MAX_PATH];
                    if (GetWindowText(pThis->_hwndEdit, szAddress, ARRAYSIZE(szAddress)))
                    {
                        WCHAR szFormat[MAX_PATH];
                        const int MAX_TOOLTIP_LENGTH = 100;
                        int cchMax = (pnmTT->cchTextMax < MAX_TOOLTIP_LENGTH) ? pnmTT->cchTextMax : MAX_TOOLTIP_LENGTH;

                        MLLoadString(IDS_GO_TOOLTIP, szFormat, ARRAYSIZE(szFormat));
                        int cch = wnsprintf(pnmTT->pszText, cchMax, szFormat, szAddress);

                        // Append ellipses?
                        if (cch == cchMax - 1)
                        {
                            // Note that Japan has a single character for ellipses, so we load
                            // as a resource.
                            WCHAR szEllipses[10];
                            cch = MLLoadString(IDS_ELLIPSES, szEllipses, ARRAYSIZE(szEllipses));
                            StrCpyN(pnmTT->pszText + cchMax - cch - 1, szEllipses, cch + 1);
                        }
                    }
                    else if (pnmTT->cchTextMax > 0)
                    {
                        // Use button text for tooltip
                        *pnmTT->pszText = L'\0';
                    }
                    break;
                }
                return 0;
            }
            break;
        }
    case WM_ERASEBKGND:
        {
            // Break out if the go button is hidden
            if (!pThis->_fGoButton)
                break;

            //
            // Forward the erase background to the parent so that
            // we appear transparent under the go button
            //
            HDC hdc = (HDC)wParam;
            HWND hwndParent = GetParent(hwnd);
            LRESULT lres = 0;

            if (hwndParent)
            {
                // Adjust the origin so the parent paints in the right place
                POINT pt = {0,0};

                MapWindowPoints(hwnd, hwndParent, &pt, 1);
                OffsetWindowOrgEx(hdc,
                                  pt.x,
                                  pt.y,
                                  &pt);

                lres = SendMessage(hwndParent, WM_ERASEBKGND, (WPARAM)hdc, 0L);

                SetWindowOrgEx(hdc, pt.x, pt.y, NULL);
            }

            if (lres != 0)
            {
                // We handled it
                return lres;
            }

            break;
         }
    case WM_DESTROY:
        //
        // Unsubclass myself.
        //
        RemoveProp(hwnd, c_szAddressBandProp);
        if (pfnOldWndProc)
        {
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR) pfnOldWndProc);
            pThis->_pfnOldWndProc = NULL;
        }
        break;
    default:
        break;
    }

    return CallWindowProc(pfnOldWndProc, hwnd, uMsg, wParam, lParam);
}

//+-------------------------------------------------------------------------
// Creates and shows the go button
//--------------------------------------------------------------------------
BOOL CAddressBand::_CreateGoButton()
{
    ASSERT(_hwndTools == NULL);

    BOOL fRet = FALSE;

    COLORREF crMask = RGB(255, 0, 255);
    if (_himlDefault == NULL)
    {
        _himlDefault = ImageList_LoadImage(HINST_THISDLL, MAKEINTRESOURCE(IDB_GO), 16, 0, crMask,
                                               IMAGE_BITMAP, LR_CREATEDIBSECTION);
    }
    if (_himlHot == NULL)
    {
        _himlHot  = ImageList_LoadImage(HINST_THISDLL, MAKEINTRESOURCE(IDB_GOHOT), 16, 0, crMask,
                                           IMAGE_BITMAP, LR_CREATEDIBSECTION);
    }

    // If we have the image lists, go ahead and create the toolbar control for the go button
    if (_himlDefault && _himlHot)
    {
        //
        // Subclass the comboboxex so that we can place the go botton within it.  The toolbad class
        // assumes one window per band, so this trick allows us to add the button using existing windows.
        // Note that comboex controls have a separate window used to wrap the internal combobox.  This
        // is the window that we use to host our "go" button.  We must subclass before creating the
        // go button so that we respond to WM_NOTIFYFORMAT with NFR_UNICODE.
        //
        //
        if (SetProp(_hwnd, c_szAddressBandProp, this))
        {
           _pfnOldWndProc = (WNDPROC) SetWindowLongPtr(_hwnd, GWLP_WNDPROC, (LONG_PTR) _ComboExWndProc);
        }

        // Create the toolbar control for the go button
        _hwndTools = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, NULL,
                                WS_CHILD | TBSTYLE_FLAT |
                                TBSTYLE_TOOLTIPS |
                                TBSTYLE_LIST |
                                WS_CLIPCHILDREN |
                                WS_CLIPSIBLINGS | CCS_NODIVIDER | CCS_NOPARENTALIGN |
                                CCS_NORESIZE,
                                0, 0, 0, 0, _hwnd, NULL, HINST_THISDLL, NULL);
    }

    if (_hwndTools)
    {
        // Init the toolbar control
        SendMessage(_hwndTools, TB_BUTTONSTRUCTSIZE, SIZEOF(TBBUTTON), 0);
        SendMessage(_hwndTools, TB_SETMAXTEXTROWS, 1, 0L);
        SendMessage(_hwndTools, TB_SETBUTTONWIDTH, 0, (LPARAM) MAKELONG(0, 500));
        SendMessage(_hwndTools, TB_SETIMAGELIST, 0, (LPARAM)_himlDefault);
        SendMessage(_hwndTools, TB_SETHOTIMAGELIST, 0, (LPARAM)_himlHot);

        LRESULT nRet = SendMessage(_hwndTools, TB_ADDSTRING, (WPARAM)MLGetHinst(), (LPARAM)IDS_ADDRESS_TB_LABELS);
        ASSERT(nRet == 0);

        static const TBBUTTON tbb[] =
        {
            {0, 1, TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, 0},
        };
        SendMessage(_hwndTools, TB_ADDBUTTONS, ARRAYSIZE(tbb), (LPARAM)tbb);

        fRet = TRUE;
    }
    else
    {
        // If no toolbar control, don't subclass the comboboxex
        if (_pfnOldWndProc)
        {
            RemoveProp(_hwnd, c_szAddressBandProp);
            SetWindowLongPtr(_hwnd, GWLP_WNDPROC, (LONG_PTR) _pfnOldWndProc);
            _pfnOldWndProc = NULL;
        }
    }

    return fRet;
}


//+-------------------------------------------------------------------------
// Shows/hides the go button depending on the current registry settings
//--------------------------------------------------------------------------
void CAddressBand::_InitGoButton()
{
    BOOL fUpdate = FALSE;
    //
    // Create the go button if it's enabled
    //
    // down-level client fix: only show Go in shell areas when NT5 or greater
    // or on a window that was originally IE

    BOOL fShowGoButton = SHRegGetBoolUSValue(REGSTR_PATH_MAIN,
        TEXT("ShowGoButton"), FALSE, /*default*/TRUE)
        && (WasOpenedAsBrowser(_punkSite) || GetUIVersion() >= 5);

    if (fShowGoButton && (_hwndTools || _CreateGoButton()))
    {
        ShowWindow(_hwndTools, SW_SHOW);
        _fGoButton = TRUE;
        fUpdate = TRUE;
    }
    else if (_hwndTools && IsWindowVisible(_hwndTools))
    {
        ShowWindow(_hwndTools, SW_HIDE);
        _fGoButton = FALSE;
        fUpdate = TRUE;
    }

    // If the go button was hidden or shown, get the combobox to adjust itself
    if (fUpdate)
    {
        // Resetting the item height gets the combobox to update the size of the editbox
        int iHeight = (int) SendMessage(_hwnd, CB_GETITEMHEIGHT, -1, 0);
        if (iHeight != CB_ERR)
        {
            SendMessage(_hwnd, CB_SETITEMHEIGHT, -1, iHeight);
        }
    }
}
