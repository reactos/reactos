#include "priv.h"
#include "sccls.h"
#include "bands.h"
#include "util.h"
#include "resource.h"
#include "inpobj.h"
#include "dhuihand.h"
#include "droptgt.h"
#include "iface.h"
#include "stream.h"
#include "isfband.h"
#include "itbdrop.h"
#include "browband.h"

#include "mluisupp.h"

#include "inetsmgr.h"
#ifdef UNIX
#include "unixstuff.h"
#endif

#define DM_PERSIST      0           // trace IPS::Load, ::Save, etc.
#define DM_MENU         0           // menu code
#define DM_FOCUS        0           // focus
#define DM_FOCUS2       0           // like DM_FOCUS, but verbose

HRESULT _BstrVariantFromGUID( REFGUID refguid, VARIANT* pvarGuid )
{
    WCHAR wszGuid[GUIDSTR_MAX];
    HRESULT hr = SHStringFromGUIDW( refguid, wszGuid, ARRAYSIZE(wszGuid) );
    if (FAILED(hr))
        return hr;

    pvarGuid->vt = VT_EMPTY;
    if (NULL == (pvarGuid->bstrVal = SysAllocString( wszGuid )))
        return E_OUTOFMEMORY;

    pvarGuid->vt = VT_BSTR;
    return S_OK;
}

//***   CBrowserBand {
//

////////////////
///  BrowserOC band

CBrowserBand::CBrowserBand() :
    CToolBand()
{
    _fBlockSIDProxy = TRUE;
    _dwModeFlags = DBIMF_FIXEDBMP | DBIMF_VARIABLEHEIGHT;
    _sizeMin.cx = _sizeMin.cy = 0;
    _sizeMax.cx = _sizeMax.cy = 32000;
    _fCustomTitle = FALSE;
    return;
}

CBrowserBand::~CBrowserBand()
{
    if (_pidl)
        ILFree(_pidl);

}

HRESULT CBrowserBand::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CBrowserBand, IContextMenu),          // IID_IContextMenu
        QITABENT(CBrowserBand, IWinEventHandler),      // IID_IWinEventHandler
        QITABENT(CBrowserBand, IDispatch),             // IID_IDispatch
        QITABENT(CBrowserBand, IPersistPropertyBag),   // IID_IPersistPropertyBag
        QITABENT(CBrowserBand, IBrowserBand),
        { 0 },
    };

    HRESULT hres = QISearch(this, qit, riid, ppvObj);

    if (FAILED(hres))
        hres = CToolBand::QueryInterface(riid, ppvObj);

    return hres;
}

HRESULT CBrowserBand_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory

    CBrowserBand * p = new CBrowserBand();
    if (p) 
    {
        *ppunk = SAFECAST(p, IDeskBand *);
        return NOERROR;
    }

    return E_OUTOFMEMORY;
}

HRESULT SHCreateBandForPidl(LPCITEMIDLIST pidl, IUnknown** ppunk, BOOL fAllowBrowserBand)
{
    IDeskBand *ptb = NULL;
    BOOL fBrowserBand;    
    DWORD dwAttrib = SFGAO_FOLDER | SFGAO_BROWSABLE;
    
    // if it's on the file system, we still might want to create a browser
    // band if it's a docobj (including .htm file)
    IEGetAttributesOf(pidl, &dwAttrib);    
    switch (dwAttrib & (SFGAO_FOLDER | SFGAO_BROWSABLE))
    {  
    case (SFGAO_FOLDER | SFGAO_BROWSABLE):
        TraceMsg(TF_WARNING, "SHCreateBandForPidl() Find out what the caller wants.  Last time we checked, nobody would set this - what does the caller want?");
    case SFGAO_BROWSABLE:
        fBrowserBand = TRUE;
        break;

    case SFGAO_FOLDER:
        fBrowserBand = FALSE;
        break;
        
    default:
        // if it's not a folder nor a browseable object, we can't host it.
        // Happens when use drags a text file and we want to turn off the
        // drop to create a band.
        return E_FAIL;

    }
    
    // this was a drag of a link or folder
    if (fBrowserBand)
    {
        if (fAllowBrowserBand)
        {
            // create browser to show web sites                        
            ptb = CBrowserBand_Create(pidl);
        }
    }
    else
    {
        // create an ISF band to show folders as hotlinks
        ptb = CISFBand_CreateEx(NULL, pidl);
    }

    *ppunk = ptb;

    if (ptb)
        return S_OK;

    return E_OUTOFMEMORY;

}


HRESULT CBrowserBand::CloseDW(DWORD dw)
{
    _Connect(FALSE);
    
    return CToolBand::CloseDW(dw);
}

void CBrowserBand::_Connect(BOOL fConnect)
{
    ConnectToConnectionPoint(SAFECAST(this, IDeskBand*), DIID_DWebBrowserEvents2, fConnect, 
                             _pauto, &_dwcpCookie, NULL);
}

/////////////////////////////////////////////////////////////////////////////
// IDispatch::Invoke
/////////////////////////////////////////////////////////////////////////////
HRESULT CBrowserBand::Invoke
(
    DISPID          dispidMember,
    REFIID          riid,
    LCID            lcid,
    WORD            wFlags,
    DISPPARAMS *    pdispparams,
    VARIANT *       pvarResult,
    EXCEPINFO *     pexcepinfo,
    UINT *          puArgErr
)
{
    ASSERT(pdispparams);
    if(!pdispparams)
        return E_INVALIDARG;

    //
    // NOTES: If we have a custom title, we don't need to process this call.
    //  This code assumes DISPID_TITLECHANGE is the only id we support.
    //  If somebody add any other, move this check below. 
    //  
    if (_fCustomTitle)
        return (S_OK);

    switch (dispidMember)
    {
    case DISPID_TITLECHANGE:
    {
        int iArg = pdispparams->cArgs -1;
        if (iArg == 0 &&
            (pdispparams->rgvarg[iArg].vt == VT_BSTR)) {

            BSTR pTitle = pdispparams->rgvarg[iArg].bstrVal;
            StrCpyNW(_wszTitle, pTitle, ARRAYSIZE(_wszTitle));
            _BandInfoChanged();
        }
        break;
    }
    }

    return S_OK;
}


/////  impl of IServiceProvider
HRESULT CBrowserBand::QueryService(REFGUID guidService,
                                  REFIID riid, void **ppvObj)
{
    *ppvObj = NULL; // assume error

    if (_fBlockSIDProxy && IsEqualGUID(guidService, SID_SProxyBrowser)) {
        return E_FAIL;
    } 
    else if (IsEqualGUID(guidService, SID_STopFrameBrowser)) {
        // block this so SearchBand doesn't end up in global history
        return E_FAIL;
    }
    else if (_fBlockDrop && IsEqualGUID(guidService, SID_SDropBlocker))
    {
        return QueryInterface(riid, ppvObj);
    }

    return CToolBand::QueryService(guidService, riid, ppvObj);
}


HRESULT CBrowserBand::SetSite(IUnknown* punkSite)
{
    
    CToolBand::SetSite(punkSite);

    if (punkSite != NULL) {
        
        if (!_hwnd)
            _CreateOCHost();
    } else {

        ATOMICRELEASE(_pauto);
        ATOMICRELEASE(_poipao);
    }

    return S_OK;
}

//***   CBrowserBand::IInputObject::* {

HRESULT CBrowserBand::TranslateAcceleratorIO(LPMSG lpMsg)
{
#ifdef DEBUG
    if (lpMsg && lpMsg->message == WM_KEYDOWN && lpMsg->wParam == VK_F12) {
        // temp debug test code
        _DebugTestCode();
    }
#endif

    if (_poipao)
        return _poipao->TranslateAccelerator(lpMsg);

    return S_FALSE;
}

HRESULT CBrowserBand::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    int iVerb = fActivate ? OLEIVERB_UIACTIVATE : OLEIVERB_INPLACEACTIVATE;

    HRESULT hr = OCHost_DoVerb(_hwnd, iVerb, lpMsg);

    // OCHost UIActivate is different than IInputObject::UIActivateIO.  It
    // doesn't do anything with the lpMsg parameter.  So, we need to pass
    // it to them via TranslateAccelerator.  Since the only case we care
    // about is when they're getting tabbed into (we want them to highlight
    // the first/last link), just do this in the case of a tab.  However,
    // don't give it to them if it's a ctl-tab.  The rule is that you shouldn't
    // handle ctl-tab when UI-active (ctl-tab switches between contexts), and
    // since Trident is always UI-active (for perf?), they'll always reject
    // ctl-tab.

    if (IsVK_TABCycler(lpMsg) && !IsVK_CtlTABCycler(lpMsg) && _poipao)
        hr = _poipao->TranslateAccelerator(lpMsg);

    return hr;
}

// }

//***   CBrowserBand::IOleCommandTarget::* {

HRESULT CBrowserBand::QueryStatus(const GUID *pguidCmdGroup,
    ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    return MayQSForward(_pauto, OCTD_DOWN, pguidCmdGroup, cCmds, rgCmds, pcmdtext);
}

HRESULT CBrowserBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    //  These are broadcast messages to the TRIDENT doc for GUID CGID_MSTHML
    if (pguidCmdGroup && IsEqualGUID(CGID_ExplorerBarDoc, *pguidCmdGroup))
    {
        if (_pauto)
        {
            LPTARGETFRAME2 ptgf;

            if (SUCCEEDED(_pauto->QueryInterface(IID_ITargetFrame2, (LPVOID *) &ptgf)))
            {
                LPOLECONTAINER pocDoc;
                if (SUCCEEDED(ptgf->GetFramesContainer(&pocDoc)) && pocDoc)
                {
                    IUnknown_Exec(pocDoc, &CGID_MSHTML, nCmdID, nCmdexecopt, 
                                    pvarargIn, pvarargOut);
                    pocDoc->Release();
                }
                ptgf->Release();
            }
        }
        return S_OK;
    }
    else
    {
        return MayExecForward(_pauto, OCTD_DOWN, pguidCmdGroup, nCmdID, nCmdexecopt,
            pvarargIn, pvarargOut);
    }
}

// }

HRESULT CBrowserBand::GetBandInfo(DWORD dwBandID, DWORD fViewMode, 
                                DESKBANDINFO* pdbi) 
{

    _dwBandID = dwBandID;

    // nt5:192868 make sure can't size to smaller than title/scrollbars
    // n.b. virt pdbi->pt.x,y is really phys y,x (i.e. phys long,short)
    pdbi->ptMinSize.x = _sizeMin.cx;
    pdbi->ptMinSize.y = max(16, _sizeMin.cy);   // BUGBUG 16 is bogus
#ifdef DEBUG
    if (pdbi->ptMinSize.x != 0 || pdbi->ptMinSize.y != 0)
        TraceMsg(DM_TRACE, "cbb.gbi: ptMinSize.(x,y)=%x,%x", pdbi->ptMinSize.x, pdbi->ptMinSize.y);
#endif
    pdbi->ptMaxSize.x = _sizeMax.cx;
    pdbi->ptMaxSize.y = _sizeMax.cy;
    pdbi->dwModeFlags = _dwModeFlags;

    pdbi->ptActual.y = -1;
    pdbi->ptActual.x = -1;
    pdbi->ptIntegral.y = 1;
    
    if (_wszTitle[0]) {
        StrCpyNW(pdbi->wszTitle, _wszTitle, ARRAYSIZE(pdbi->wszTitle));
    } else if ( _fCustomTitle) {
        pdbi->dwMask &= ~DBIM_TITLE;
    }    
    else{
        SHGetNameAndFlagsW(_pidl, SHGDN_NORMAL, pdbi->wszTitle, SIZECHARS(pdbi->wszTitle), NULL);
    }
    
    return S_OK;
} 


void CBrowserBand::_InitBrowser(void)
{
    ASSERT(IsWindow(_hwnd));

    OCHost_QueryInterface(_hwnd, IID_IWebBrowser2, (LPVOID*)&_pauto);
    OCHost_SetOwner(_hwnd, SAFECAST(this, IContextMenu*));

    if (EVAL(_pauto))
    {
        LPTARGETFRAME2 ptgf;

        if (SUCCEEDED(_pauto->QueryInterface(IID_ITargetFrame2, (LPVOID *) &ptgf)))
        {
            DWORD dwOptions;

            if (SUCCEEDED(ptgf->GetFrameOptions(&dwOptions)))
            {
                dwOptions |= FRAMEOPTIONS_BROWSERBAND | FRAMEOPTIONS_SCROLL_AUTO;
                ptgf->SetFrameOptions(dwOptions);
            }
            ptgf->Release();
        }

        _pauto->put_RegisterAsDropTarget(VARIANT_FALSE);

        // BUG do OCHost_QI
        // note only 1 active object (proxy)
        _pauto->QueryInterface(IID_IOleInPlaceActiveObject, (LPVOID*)&_poipao);
        ASSERT(_poipao != NULL);
        
        // set up the connection point
        _Connect(TRUE);
    }
}

HRESULT CBrowserBand::_NavigateOC()
{
    HRESULT hres = E_FAIL;
    if (_hwnd)
    {
        ASSERT(IsWindow(_hwnd));
        if (!_pidl) {
            if (_pauto) {
                hres = _pauto->GoHome();
            }
        } else {
            IServiceProvider* psp;

            OCHost_QueryInterface(_hwnd, IID_IServiceProvider, (LPVOID*)&psp);
            if (psp)
            {
                IShellBrowser* psb;
                if (EVAL(SUCCEEDED(psp->QueryService(SID_SShellBrowser, IID_IShellBrowser, (LPVOID*)&psb))))
                {
                    hres = psb->BrowseObject(_pidl, SBSP_SAMEBROWSER);
                    psb->Release();
                }
                psp->Release();
            }

        }
    }

    return hres;
}

EXTERN_C BOOL SHDOCVW_DllRegisterWindowClasses(const SHDRC * pshdrc);

HRESULT CBrowserBand::_CreateOCHost()
{
    HRESULT hres = E_FAIL; // assume error

    // Register the OCHost window class
    SHDRC shdrc = {sizeof(SHDRC), SHDRCF_OCHOST};
    shdrc.cbSize = sizeof (SHDRC);
    shdrc.dwFlags |= SHDRCF_OCHOST;
    if (SHDOCVW_DllRegisterWindowClasses(&shdrc))
    {
        // Create an OCHost window
        _hwnd = CreateWindow(OCHOST_CLASS, NULL,
            WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_TABSTOP,
            0, 0, 1, 1,
            _hwndParent, NULL, HINST_THISDLL, NULL);

        if (_hwnd)
        {
            OCHINITSTRUCT ocs;
            ocs.cbSize = SIZEOF(OCHINITSTRUCT);   
            ocs.clsidOC  = CLSID_WebBrowser;
            ocs.punkOwner = SAFECAST(this, IDeskBand*);

            hres = OCHost_InitOC(_hwnd, (LPARAM)&ocs);        

            _InitBrowser();
            _NavigateOC();
            OCHost_DoVerb(_hwnd, OLEIVERB_INPLACEACTIVATE, FALSE);
        }
    }
    return hres;
}

//***   CBrowserBand::IWinEventHandler::* {

HRESULT CBrowserBand::OnWinEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres)
{
    switch (uMsg) {
    case WM_NOTIFY:
        _OnNotify((LPNMHDR)lParam);
        return S_OK;
        
    default:
        break;
    }

    return E_FAIL;
}

HRESULT CBrowserBand::IsWindowOwner(HWND hwnd)
{
    HRESULT hres;

    hres = SHIsChildOrSelf(_hwnd, hwnd);
    ASSERT(hwnd != NULL || hres == S_FALSE);
    ASSERT(_hwnd != NULL || hres == S_FALSE);
    return hres;
}

#if 0
static void HackFocus(HWND hwndFrom)
{
    TraceMsg(DM_FOCUS, "HackFocus: GetFocus()=%x hwndOCHost=%x", GetFocus(), hwndFrom);
    hwndFrom = GetWindow(hwndFrom, GW_CHILD);   // OCHost->shembed
    TraceMsg(DM_FOCUS, "HackFocus: hwndShEmbed=%x", hwndFrom);
    hwndFrom = GetWindow(hwndFrom, GW_CHILD);   // shembed->shdocvw
    TraceMsg(DM_FOCUS, "HackFocus: hwndShDocVw=%x", hwndFrom);
    hwndFrom = GetWindow(hwndFrom, GW_CHILD);   // shdocvw->iesvr
    TraceMsg(DM_FOCUS, "HackFocus: hwndIESvr=%x", hwndFrom);
    if (hwndFrom != 0) {
        TraceMsg(DM_FOCUS, "HackFocus: SetFocus(%x)", hwndFrom);
        SetFocus(hwndFrom);
    }
    return;
}
#endif

LRESULT CBrowserBand::_OnNotify(LPNMHDR pnm)
{
    switch (pnm->code) {
    case OCN_ONUIACTIVATE:  // UIActivate
        TraceMsg(DM_FOCUS, "cbb._on: OCN_ONUIACT");
        ASSERT(SHIsSameObject(((LPOCNONUIACTIVATEMSG)pnm)->punk, _poipao));
        // n.b. we pass up 'this' not pnm->punk, since we always want to
        // be the intermediary (e.g. for UIActivateIO calls to us)
        TraceMsg(DM_FOCUS, "cbb._on: OCN_ONUIACT call ofcis(fSetFocus=TRUE)");
        UnkOnFocusChangeIS(_punkSite, SAFECAST(this, IInputObject*), TRUE);
        return OCNONUIACTIVATE_HANDLED;

    case OCN_ONSETSTATUSTEXT:
        {
            HRESULT hr = E_FAIL;
            IShellBrowser *psb;

            hr = QueryService(SID_STopLevelBrowser, IID_IShellBrowser, (LPVOID*)&psb);
            if (SUCCEEDED(hr)) {
                hr = psb->SetStatusTextSB(((LPOCNONSETSTATUSTEXTMSG)pnm)->pwszStatusText);
                psb->Release();
            }
            //return hr;
        }
        break;

    case OCN_ONPOSRECTCHANGE:
        {
            LPCRECT lprcPosRect = ((LPOCNONPOSRECTCHANGEMSG)pnm)->prcPosRect;
            _sizeMin.cx = lprcPosRect->right - lprcPosRect->left;
            _sizeMin.cy = lprcPosRect->bottom - lprcPosRect->top;

            _BandInfoChanged();

            break;
        }

    default:
        break;
    }

    ASSERT(OCNONUIACTIVATE_HANDLED != 0);
    return 0;
}

// }

//***   CBrowserBand::IPersistStream::* {

HRESULT CBrowserBand::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_BrowserBand;

    return S_OK;
}


// mask flags for BrowserBand persistence
//
#define BB_ILSTREAM   0x00000001
#define BB_PIDLASLINK 0x00000002

// BUGBUG REVIEW: it seems to me like we should let the WebBrowserOC
// persist it's location, not us...
//
HRESULT CBrowserBand::Load(IStream *pstm)
{
    HRESULT hres;
    DWORD dw;
    
    if (_pidl) {
        ILFree(_pidl);
    }

    hres = pstm->Read(&dw, SIZEOF(DWORD), NULL);
    if (SUCCEEDED(hres))
    {
        if (dw & BB_PIDLASLINK)
        {
            hres = LoadPidlAsLink(_punkSite, pstm, &_pidl);
        }
        else if (dw & BB_ILSTREAM) // for backwards compat
        {
            hres = ILLoadFromStream(pstm, &_pidl);
        }
    }
        
    if (SUCCEEDED(hres))
        _NavigateOC();
    
    return hres;
}

HRESULT CBrowserBand::Save(IStream *pstm, BOOL fClearDirty)
{
    HRESULT hres;
    DWORD dw = 0;
    BSTR bstrUrl = NULL;

    if (_pauto && SUCCEEDED(_pauto->get_LocationURL(&bstrUrl)) && bstrUrl) {
        TraceMsg(DM_PERSIST, "cbb.s: current/new url=%s", bstrUrl);
        if (_pidl) {
            ILFree(_pidl);
            _pidl = NULL;       // paranoia
        }
        IECreateFromPath(bstrUrl, &_pidl);
        SysFreeString(bstrUrl);
    }

    if (_pidl)
        dw |= BB_PIDLASLINK;

    hres = pstm->Write(&dw, SIZEOF(DWORD), NULL);

    if (SUCCEEDED(hres) && (dw & BB_PIDLASLINK))
        hres = SavePidlAsLink(_punkSite, pstm, _pidl);
    
    return hres;
}

// }

//***   CBrowserBand::IPersistPropertyBag::* {

HRESULT CBrowserBand::Load(IPropertyBag *pPBag, IErrorLog *pErrLog)
{
    HRESULT hres;
    TCHAR szUrl[MAX_URL_STRING];

    TraceMsg(DM_TRACE, "cbb.l(bag): enter");

    if (_pidl) {
        ILFree(_pidl);
    }

    hres = IPBag_ReadStr(pPBag, OLESTR("Url"), szUrl, ARRAYSIZE(szUrl));
    if (SUCCEEDED(hres)) {
        TCHAR * pszFinalUrl;
        TCHAR   szPlug[MAX_PATH];
        TCHAR   szMuiPath[MAX_PATH];

        pszFinalUrl = szUrl;

        hres = IPBag_ReadStr(pPBag, OLESTR("Pluggable"), szPlug, ARRAYSIZE(szPlug));

        if (SUCCEEDED(hres) && !StrCmpNI(TEXT("yes"), szPlug, ARRAYSIZE(szPlug)))
        {
            TCHAR * pszFile;

            // if this is loading html out of the windows\web folder
            // then we need to call SHGetWebFolderFilePath in order
            // to support pluggable UI

            pszFile = PathFindFileName(szUrl);
            hres = SHGetWebFolderFilePath(pszFile, szMuiPath, ARRAYSIZE(szMuiPath));
            if (SUCCEEDED(hres))
            {
                pszFinalUrl = szMuiPath;
            }
        }

        hres = IECreateFromPath(pszFinalUrl, &_pidl);
        if (SUCCEEDED(hres)) {
            _NavigateOC();
        }
    }
    
    return hres;
}

// }

//***   CBrowserBand::IContextMenu::* {

HRESULT CBrowserBand::QueryContextMenu(HMENU hmenu,
    UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    int i = 0;
    HMENU hmenuMe = LoadMenuPopup_PrivateNoMungeW(MENU_BROWBAND);

    i += Shell_MergeMenus(hmenu, hmenuMe, indexMenu, idCmdFirst + i, idCmdLast, MM_ADDSEPARATOR) - (idCmdFirst + i);
    DestroyMenu(hmenuMe);

    // aka (S_OK|i)
    return MAKE_HRESULT(ERROR_SUCCESS, FACILITY_NULL, i);
}

HRESULT CBrowserBand::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    TraceMsg(DM_MENU, "cbb.ic");

    HRESULT hres;
    int idCmd = -1;

    // BUGBUG todo: id -= _idCmdFirst ???

    if (!HIWORD(pici->lpVerb))
        idCmd = LOWORD(pici->lpVerb);

    //
    // Low memory paranoia safety check
    //
    if (!_pauto) {
        TraceMsg(DM_ERROR, "CBrowserBand::InvokeCommand: _pauto IS NULL");
        return E_OUTOFMEMORY;
    }

    switch (idCmd) {
    case IDM_BROWBAND_REFRESH:
#ifdef DEBUG
        if (GetKeyState(VK_SHIFT) < 0)
            hres = _pauto->GoHome();
        else
#endif
        hres = _pauto->Refresh();
        break;
    case IDM_BROWBAND_OPENNEW:   // clone window into 'real' browser
        {
        BSTR bstrURL = NULL;

        // n.b. this clones the *current page* into a 'real' browser,
        // not the link.

        // BUGBUG todo: we'd really rather get and navigate to
        // a PIDL, but that isn't supported yet in ie4.
        hres = _pauto->get_LocationURL(&bstrURL);
        if (SUCCEEDED(hres)) {
            VARIANT varFlags;

            VariantInit(&varFlags);
            varFlags.vt = VT_I4;
            varFlags.lVal = (navOpenInNewWindow|navNoHistory);

            // n.b. we drop the post data etc. on the floor, oh well...
            hres = _pauto->Navigate(bstrURL, /*flags*/&varFlags, /*targ*/NULL, /*post*/NULL, /*hdrs*/NULL);

            VariantClear(&varFlags);
        }

        if (bstrURL)
            SysFreeString(bstrURL);

        ASSERT(SUCCEEDED(hres));

        break;
        }
    default:
        TraceMsg(DM_ERROR, "cbb::ic cmd=%d not handled", idCmd);
        break;
    }

    return S_OK;
}

// }

SIZE CBrowserBand::_GetCurrentSize()
{
    SIZE size;

    RECT rc;
    GetWindowRect(_hwnd, &rc);

    size.cx = RECTWIDTH(rc);
    size.cy = RECTHEIGHT(rc);

    return size;
}

// *** IBrowserBand methods ***
HRESULT CBrowserBand::GetObjectBB(REFIID riid, LPVOID *ppv)
{
    return _pauto ? _pauto->QueryInterface(riid, ppv) : E_UNEXPECTED;
}

#ifdef DEBUG
void CBrowserBand::_DebugTestCode()
{
    DWORD dwMask = 0x10000000;  // non-NULL bogus mask

    BROWSERBANDINFO bbi;
    bbi.cbSize = SIZEOF(BROWSERBANDINFO);

    GetBrowserBandInfo(dwMask, &bbi);
}
#endif // DEBUG

void CBrowserBand::_MakeSizesConsistent(LPSIZE psizeCur)
{
    // _sizeMin overrules _sizeMax

    if (_dwModeFlags & DBIMF_FIXED) {
        // if they specified a current size, use that instead
        // of min size
        if (psizeCur)
            _sizeMin = *psizeCur;
        _sizeMax = _sizeMin;
    } else {
        _sizeMax.cx = max(_sizeMin.cx, _sizeMax.cx);
        _sizeMax.cy = max(_sizeMin.cy, _sizeMax.cy);

        if (psizeCur) {
            psizeCur->cx = max(_sizeMin.cx, psizeCur->cx);
            psizeCur->cy = max(_sizeMin.cy, psizeCur->cy);

            psizeCur->cx = min(_sizeMax.cx, psizeCur->cx);
            psizeCur->cy = min(_sizeMax.cy, psizeCur->cy);
        }
    }
}

HRESULT CBrowserBand::SetBrowserBandInfo(DWORD dwMask, PBROWSERBANDINFO pbbi)
{
    if (!pbbi || pbbi->cbSize != SIZEOF(BROWSERBANDINFO))
        return E_INVALIDARG;

    if (!dwMask || (dwMask & BBIM_MODEFLAGS))
        _dwModeFlags = pbbi->dwModeFlags;

    if (!dwMask || (dwMask & BBIM_TITLE)) {
        if (pbbi->bstrTitle) {
            _fCustomTitle = TRUE;
            // Change the internal _wszTitle used by Browser band
            StrCpyNW(_wszTitle, pbbi->bstrTitle, ARRAYSIZE(_wszTitle));
        } else {
            _fCustomTitle = FALSE;
        }
    }

    if (!dwMask || (dwMask & BBIM_SIZEMIN))
        _sizeMin = pbbi->sizeMin;

    if (!dwMask || (dwMask & BBIM_SIZEMAX))
        _sizeMax = pbbi->sizeMax;

    if (!dwMask || (dwMask & BBIM_SIZECUR)) {
        SIZE sizeCur = pbbi->sizeCur;
        _MakeSizesConsistent(&sizeCur);

        // HACKHACK: the only way to tell bandsite to change the height of a horizontal
        // band is to give it a new min/max height pair at the desired height.  the same
        // holds for setting the width of a vertical band.  so we temporarily give bandsite
        // new min/max size info, then restore old min/max.

        SIZE sizeMinOld = _sizeMin;
        SIZE sizeMaxOld = _sizeMax;
        _sizeMin = _sizeMax = sizeCur;

        _BandInfoChanged();

        _sizeMin = sizeMinOld;
        _sizeMax = sizeMaxOld;
    } else {
        _MakeSizesConsistent(NULL);
    }

    _BandInfoChanged();

    return S_OK;
}

// we don't have a client to test BBIM_TITLE, so leave it unimplemented for now.
#define BBIM_INVALIDFLAGS (~(BBIM_SIZEMIN | BBIM_SIZEMAX | BBIM_SIZECUR | BBIM_MODEFLAGS))

HRESULT CBrowserBand::GetBrowserBandInfo(DWORD dwMask, PBROWSERBANDINFO pbbi)
{
    if (!pbbi || pbbi->cbSize != SIZEOF(BROWSERBANDINFO))
        return E_INVALIDARG;

    if (dwMask & BBIM_INVALIDFLAGS)
        return E_INVALIDARG;

    pbbi->dwModeFlags = _dwModeFlags;
    pbbi->sizeMin = _sizeMin;
    pbbi->sizeMax = _sizeMax;
    pbbi->sizeCur =_GetCurrentSize();

    return S_OK;
}

IDeskBand* CBrowserBand_Create(LPCITEMIDLIST pidl)
{
    CBrowserBand *p = new CBrowserBand();
    if(p) {
        if (pidl)
            p->_pidl = ILClone(pidl);
    }
    return p;
}

// }

class CSearchSecurityMgrImpl : public CInternetSecurityMgrImpl 
{
    // *** IID_IInternetSecurityManager ***
    
    virtual STDMETHODIMP ProcessUrlAction(LPCWSTR pwszUrl, DWORD dwAction, BYTE *pPolicy, DWORD cbPolicy,
                                  BYTE *pContext, DWORD cbContext, DWORD dwFlags, DWORD dwReserved)
    {
        HRESULT hres = INET_E_DEFAULT_ACTION;

        switch (dwAction)
        {
            case URLACTION_ACTIVEX_RUN:
            case URLACTION_SCRIPT_RUN:
            case URLACTION_SCRIPT_SAFE_ACTIVEX:
            case URLACTION_HTML_SUBMIT_FORMS:
                if (_IsSafeUrl(pwszUrl))
                {
                    if (cbPolicy >= SIZEOF(DWORD))
                    {
                        *(DWORD *)pPolicy = URLPOLICY_ALLOW;
                        hres = S_OK;
                    }
                    else
                    {
                        hres = S_FALSE;
                    }
                }
                break;
        }
        
        return hres;
    }
};

class CCustomizeSearchHelper : public CInternetSecurityMgrImpl,
                               public IServiceProvider
{
public:

    CCustomizeSearchHelper() : _cRef(1) { }
    
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IID_IInternetSecurityManager ***
    virtual STDMETHODIMP ProcessUrlAction(LPCWSTR pwszUrl, DWORD dwAction, BYTE *pPolicy, DWORD cbPolicy,
                                  BYTE *pContext, DWORD cbContext, DWORD dwFlags, DWORD dwReserved);

    // *** IServiceProvider ***
    virtual STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

    virtual BOOL _IsSafeUrl(LPCWSTR pwszUrl) { return TRUE; }

private:
    ~CCustomizeSearchHelper() {};
    
    ULONG   _cRef;
};

STDMETHODIMP_(ULONG) CCustomizeSearchHelper::AddRef()
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG) CCustomizeSearchHelper::Release(void)
{
    if( 0L != --_cRef )
        return _cRef;

    delete this;
    return 0L;
}

HRESULT CCustomizeSearchHelper::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CCustomizeSearchHelper, IServiceProvider),
        QITABENT(CCustomizeSearchHelper, IInternetSecurityManager),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

HRESULT CCustomizeSearchHelper::ProcessUrlAction(LPCWSTR pwszUrl, DWORD dwAction, BYTE *pPolicy, 
                                                 DWORD cbPolicy, BYTE *pContext, DWORD cbContext, 
                                                 DWORD dwFlags, DWORD dwReserved)
{
   HRESULT hres = INET_E_DEFAULT_ACTION;

    switch (dwAction)
    {
        case URLACTION_ACTIVEX_RUN:
        case URLACTION_SCRIPT_RUN:
        case URLACTION_SCRIPT_SAFE_ACTIVEX:
        case URLACTION_HTML_SUBMIT_FORMS:
            if (cbPolicy >= SIZEOF(DWORD))
            {
                *(DWORD *)pPolicy = URLPOLICY_ALLOW;
                hres = S_OK;
            }
            else
            {
                hres = S_FALSE;
            }
            break;
    }
    
    return hres;
}

STDMETHODIMP CCustomizeSearchHelper::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    if (IID_IInternetSecurityManager == guidService)
    {
        return QueryInterface(riid, ppvObject);
    }
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
}


//***   CSearchBand {
//

////////////////
///  Search (BrowserOC) band

//  If you change this, change shdocvw also.
const WCHAR c_wszThisBandIsYourBand[] = L"$$SearchBand$$";

#define SEARCH_MENUID_OFFSET    100

class CSearchBand : public CBrowserBand, 
                    public IBandNavigate,
                    public ISearchBandTBHelper,
                    public CSearchSecurityMgrImpl
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IDeskBand methods ***
    virtual STDMETHODIMP GetBandInfo(DWORD dwBandID, DWORD fViewMode, 
                                   DESKBANDINFO* pdbi);

    // *** IPersistStream methods ***
    // (others use base class implementation) 
    virtual STDMETHODIMP GetClassID(CLSID *pClassID);
    virtual STDMETHODIMP Load(IStream *pStm);
    virtual STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty);

    // *** IBandNavigate ***
    virtual STDMETHODIMP Select(LPCITEMIDLIST pidl);

    // *** IOleCommandTarget methods ***
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup,
        DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    // *** IDockingWindow methods ***
    virtual STDMETHODIMP ShowDW(BOOL fShow);

    // *** ISearchBandTBHelper methods ***
    virtual STDMETHODIMP AddNextMenuItem(LPCWSTR pwszText, int idItem);
    virtual STDMETHODIMP ResetNextMenu();
    virtual STDMETHODIMP SetOCCallback(IOleCommandTarget *pOleCmdTarget);

    // *** IServiceProvider methods ***
    virtual STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, LPVOID* ppvObj);

    // *** IWinEventHandler ***
    virtual STDMETHODIMP OnWinEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres);

protected:
    CSearchBand();
    virtual ~CSearchBand();

    virtual void _Connect(BOOL fConnect);
    virtual void _InitBrowser(void);
    virtual HRESULT _NavigateOC();
    
    void _AddButtons(BOOL fAdd);
    void _OnNextButtonSelect(int x, int y);
    void _OnNew();
    void _DoNext(int newPos);
    void _OnNextButtonClick();
    void _OnCustomize();
    void _OnHelp();
    void _NavigateToUrl(LPCTSTR pszUrl);
    void _EnsureImageListsLoaded();
    void _EnableNext(BOOL bEnable);
    void _NavigateToSearchUrl();

    virtual BOOL _IsSafeUrl(LPCWSTR pwszUrl);
    
    BOOL _fStrsAdded;
    LONG_PTR _lStrOffset;

    IOleCommandTarget *_pOCCmdTarget;

    HIMAGELIST  _himlNormal;
    HIMAGELIST  _himlHot;

    friend HRESULT CSearchBand_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);       // for ctor
    friend IDeskBand* CSearchBand_Create();

    HMENU _hmenuNext;
    HWND _hwndParent;
    int _nextPos;

    BOOL _bNewUrl; // set to true when we are QS'd for IInternetSecurityMgr, i.e. when pane is renavigated
    BOOL _bUseDefault; // true if we should not use our security mgr
    WCHAR _wszCache[MAX_URL_STRING];
    DWORD _nCmpLength;
    BOOL  _bIsCacheSafe;
};

CSearchBand::CSearchBand() :
    CBrowserBand()
{
    _fBlockSIDProxy = FALSE;
    _fBlockDrop = TRUE;
    _bNewUrl    = TRUE;
    ASSERT(_wszCache[0] == TEXT('\0'));
    ASSERT(_nCmpLength == 0);  
    ASSERT(_bIsCacheSafe == FALSE);
}

CSearchBand::~CSearchBand()
{
    ResetNextMenu();

    if (NULL != _himlNormal)
    {
        ImageList_Destroy(_himlNormal);
    }
    
    if (NULL != _himlHot)  
    {
        ImageList_Destroy(_himlHot);
    }

    ATOMICRELEASE(_pOCCmdTarget);
}

void CSearchBand::_NavigateToUrl(LPCTSTR pszUrl)
{
    if (NULL != _pidl)
    {
        ILFree(_pidl);
    }

    IECreateFromPath(pszUrl, &_pidl);
    _NavigateOC();
}

void CSearchBand::_NavigateToSearchUrl()
{
    TCHAR szUrl[INTERNET_MAX_URL_LENGTH];
    BOOL bFound;
    BOOL bWebSearch = FALSE;
    IBrowserService2 *pbs;
        
    if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_IBrowserService2, (void **)&pbs)))
    {
        ITEMIDLIST *pidl;

        if (SUCCEEDED(pbs->GetPidl(&pidl)))
        {
            // BUGBUG: This code should be using IShellFolder2::GetDefaultSearchGUID() and
            //     keying off SRCID_SWebSearch (vs. SRCID_SFileSearch/SRCID_SFindComputer/SRCID_SFindPrinter)
            bWebSearch = ILIsWeb(pidl);
            ILFree(pidl);
        }
        pbs->Release();
    }

    ResetNextMenu();

    if (bWebSearch)
    {
        bFound = GetDefaultInternetSearchUrl(szUrl, ARRAYSIZE(szUrl), TRUE);
    }
    else
    {
        bFound = GetSearchAssistantUrl(szUrl, ARRAYSIZE(szUrl), TRUE, FALSE);
    }

    if (bFound)
    {
        _NavigateToUrl(szUrl);
    }
}

void CSearchBand::_OnNew()
{
    VARIANT var;
    var.vt = VT_BOOL;
    var.boolVal = VARIANT_FALSE;

    if (NULL != _pOCCmdTarget)
    {
        HRESULT hr = _pOCCmdTarget->Exec(NULL, SBID_SEARCH_NEW, 0, NULL, &var);
        
        if (FAILED(hr))
        {
            var.boolVal = VARIANT_FALSE;
        }
    }

    if ((var.vt != VT_BOOL) || (!var.boolVal))
    {
        _NavigateToSearchUrl();
    }
}

void CSearchBand::_OnNextButtonSelect(int x, int y)
{
    HWND hwnd;

    if (SUCCEEDED(IUnknown_GetWindow(_punkSite, &hwnd)))
    {
        int idItem = TrackPopupMenu(_hmenuNext, TPM_RETURNCMD, x, y, 0, hwnd, NULL);

        if (0 != idItem)
        {
            _DoNext(GetMenuPosFromID(_hmenuNext, idItem));
        }        
    }
}

void CSearchBand::_DoNext(int newPos)
{
    if (NULL != _pOCCmdTarget)
    {
        CheckMenuItem(_hmenuNext, _nextPos, MF_BYPOSITION | MF_UNCHECKED);

        _nextPos = newPos;

        CheckMenuItem(_hmenuNext, _nextPos, MF_BYPOSITION | MF_CHECKED);

        VARIANT var;
        
        var.vt = VT_I4;
        var.lVal = GetMenuItemID(_hmenuNext, _nextPos) - SEARCH_MENUID_OFFSET;
        
        HRESULT hr = _pOCCmdTarget->Exec(NULL, SBID_SEARCH_NEXT, 0, &var, NULL);
        
        ASSERT(SUCCEEDED(hr));
    }
}

void CSearchBand::_OnNextButtonClick()
{
    int newPos = _nextPos + 1;
    
    if (newPos >= GetMenuItemCount(_hmenuNext))
    {
        newPos = 0;
    }

    _DoNext(newPos);
}

void CSearchBand::_OnCustomize()
{
    TCHAR szUrl[INTERNET_MAX_URL_LENGTH];
    HWND hwnd;

    IUnknown_GetWindow(_punkSite, &hwnd);

    if (GetSearchAssistantUrl(szUrl, ARRAYSIZE(szUrl), TRUE, TRUE))
    {
        if (InternetGoOnline(szUrl, hwnd, 0))
        {
            IMoniker *pmk;

            if (SUCCEEDED(CreateURLMoniker(NULL, szUrl, &pmk)))
            {
                ITridentAPI *pTridentAPI;
                
                if (SUCCEEDED(CoCreateInstance(CLSID_TridentAPI, NULL, CLSCTX_INPROC_SERVER,
                                               IID_ITridentAPI, (void **)&pTridentAPI)))
                {
                    IUnknown *punkCustHelper = NULL;

                    if (_IsSafeUrl(szUrl))
                    {
                        punkCustHelper = (IUnknown *)(IServiceProvider *)new CCustomizeSearchHelper;
                    }

                    pTridentAPI->ShowHTMLDialog(hwnd, pmk, NULL, L"help:no;resizable:1", NULL, punkCustHelper);

                    if (NULL != punkCustHelper)
                    {
                        punkCustHelper->Release();
                    }

                    pTridentAPI->Release();
                }

                pmk->Release();
            }
        }
    }
}

void CSearchBand::_OnHelp()
{
    HWND hwnd;

    IUnknown_GetWindow(_punkSite, &hwnd);

#ifndef UNIX
    SHHtmlHelpOnDemandWrap(hwnd, TEXT("iexplore.chm > iedefault"), 0, (DWORD_PTR) TEXT("srchasst.htm"), ML_CROSSCODEPAGE);
#else
    {
        IServiceProvider* psp;

        OCHost_QueryInterface(_hwnd, IID_IServiceProvider, (LPVOID*)&psp);
        if (psp)
        {
            IShellBrowser* psb;
            if (EVAL(SUCCEEDED(psp->QueryService(SID_SShellBrowser, IID_IShellBrowser, (LPVOID*)&psb))))
            {
                UnixHelp(L"Search Help", psb);
                psb->Release();
            }
            psp->Release();
        }
    }
#endif
}

HRESULT CSearchBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    if (pguidCmdGroup && IsEqualGUID(CGID_SearchBand, *pguidCmdGroup))
    {
        switch (nCmdID)
        {
        case SBID_SEARCH_NEW:
            _OnNew();
            return S_OK;

        case SBID_SEARCH_NEXT:
            if (nCmdexecopt == OLECMDEXECOPT_PROMPTUSER)
            {
                if ((NULL != pvarargIn) && (pvarargIn->vt == VT_I4))
                {
                    ASSERT(NULL != _hmenuNext);
                    _OnNextButtonSelect(LOWORD(pvarargIn->lVal), HIWORD(pvarargIn->lVal));
                }
            }
            else
            {
                _OnNextButtonClick();
            }
            return S_OK;

        case SBID_SEARCH_CUSTOMIZE:
            _OnCustomize();
            return S_OK;

        case SBID_SEARCH_HELP:
            _OnHelp();
            return S_OK;
            
        case SBID_GETPIDL:
            {
                HRESULT hres = E_INVALIDARG;
                
                if (pvarargOut)
                {
                    hres = E_OUTOFMEMORY;
                    VariantInit(pvarargOut); // zero init it
                    if (!_pidl || InitVariantFromIDList(pvarargOut, _pidl))
                        hres = S_OK;
                }
                return hres;
            }
        }
    }
    return CBrowserBand::Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
}

#define INDEX_NEXT          1
#define INDEX_CUSTOMIZE     3
static const TBBUTTON c_tbSearch[] =
{
    {  0,           SBID_SEARCH_NEW,       TBSTATE_ENABLED,   BTNS_AUTOSIZE | BTNS_SHOWTEXT,                 {0, 0}, 0, 0},
    {  1,           SBID_SEARCH_NEXT,      0,                 BTNS_AUTOSIZE | BTNS_DROPDOWN | BTNS_SHOWTEXT, {0, 0}, 0, 1},
    {  0,           0,                     TBSTATE_ENABLED,   BTNS_SEP,                                      {0, 0}, 0, 0},
    {  I_IMAGENONE, SBID_SEARCH_CUSTOMIZE, TBSTATE_ENABLED,   BTNS_AUTOSIZE | BTNS_SHOWTEXT,                 {0, 0}, 0, 2}
};

void CSearchBand::_EnableNext(BOOL bEnable)
{
    IExplorerToolbar* piet;

    if (SUCCEEDED(_punkSite->QueryInterface(IID_IExplorerToolbar, (void**)&piet)))
    {
        UINT state;

        if (SUCCEEDED(piet->GetState(&CGID_SearchBand, SBID_SEARCH_NEXT, &state)))
        {
            if (bEnable)
            {
                state |= TBSTATE_ENABLED;
            }
            else
            {
                state &= ~TBSTATE_ENABLED;
            }
            piet->SetState(&CGID_SearchBand, SBID_SEARCH_NEXT, state);
        }

        piet->Release();
    }
}

void CSearchBand::_AddButtons(BOOL fAdd)
{
    IExplorerToolbar* piet;

    if (SUCCEEDED(_punkSite->QueryInterface(IID_IExplorerToolbar, (void**)&piet)))
    {
        if (fAdd)
        {
            piet->SetCommandTarget((IUnknown*)SAFECAST(this, IOleCommandTarget*), &CGID_SearchBand, 0);

            if (!_fStrsAdded)
            {
                LONG_PTR   cbOffset;
                piet->AddString(&CGID_SearchBand, MLGetHinst(), IDS_SEARCH_BAR_LABELS, &cbOffset);
                _lStrOffset = cbOffset;
                _fStrsAdded = TRUE;
            }

            _EnsureImageListsLoaded();
            piet->SetImageList(&CGID_SearchBand, _himlNormal, _himlHot, NULL);

            TBBUTTON tbSearch[ARRAYSIZE(c_tbSearch)];
            UpdateButtonArray(tbSearch, c_tbSearch, ARRAYSIZE(c_tbSearch), _lStrOffset);

            if (SHRestricted2(REST_NoSearchCustomization, NULL, 0))
            {
                tbSearch[INDEX_CUSTOMIZE].fsState &= ~TBSTATE_ENABLED;
            }

            if (NULL != _hmenuNext)
            {
                tbSearch[INDEX_NEXT].fsState |= TBSTATE_ENABLED;
            }

            piet->AddButtons(&CGID_SearchBand, ARRAYSIZE(tbSearch), tbSearch);
        }
        else
            piet->SetCommandTarget(NULL, NULL, 0);

        piet->Release();
    }
}

void CSearchBand::_EnsureImageListsLoaded()
{
    if (_himlNormal == NULL)
    {
        _himlNormal = ImageList_LoadImage(HINST_THISDLL, 
                                          MAKEINTRESOURCE(IDB_SEARCHBANDDEF), 
                                          18, 
                                          0, 
                                          RGB(255, 0, 255),
                                          IMAGE_BITMAP, 
                                          LR_CREATEDIBSECTION);
    }

    if (_himlHot == NULL)
    {
        _himlHot = ImageList_LoadImage(HINST_THISDLL, 
                                       MAKEINTRESOURCE(IDB_SEARCHBANDHOT), 
                                       18, 
                                       0, 
                                       RGB(255, 0, 255),
                                       IMAGE_BITMAP, 
                                       LR_CREATEDIBSECTION);
    }
}

HRESULT CSearchBand::AddNextMenuItem(LPCWSTR pwszText, int idItem)
{
    if (NULL == _hmenuNext)
    {
        _hmenuNext = CreatePopupMenu();
    }

    ASSERT(NULL != _hmenuNext);

    if (NULL != _hmenuNext)
    {

#ifdef DEBUG
        //  Check to see if an item with this ID has already been added
        MENUITEMINFO dbgMii = { sizeof(dbgMii) };
        dbgMii.fMask = MIIM_STATE;
        if (GetMenuItemInfo(_hmenuNext, idItem + SEARCH_MENUID_OFFSET, FALSE, &dbgMii))
        {
            TraceMsg(DM_ERROR, "Adding duplicate menu item in CSearchBand::AddNextMenuItem");
        }
#endif

        int nItems = GetMenuItemCount(_hmenuNext);
        
        MENUITEMINFOW mii = { sizeof(mii) };

        mii.fMask = MIIM_ID | MIIM_TYPE;
        mii.wID = (WORD)idItem + SEARCH_MENUID_OFFSET;
        mii.fType = MFT_RADIOCHECK | MFT_STRING;
        mii.dwTypeData = (LPWSTR)pwszText;
        mii.cch = lstrlenW(pwszText);

        BOOL result = InsertMenuItemW(_hmenuNext, nItems, TRUE, &mii);

        if (result)
        {
            if (0 == nItems)
            {
                CheckMenuItem(_hmenuNext, 0, MF_BYPOSITION | MF_CHECKED);
                _EnableNext(TRUE);
            }
        }
    }
    
    return S_OK;
}

HRESULT CSearchBand::ResetNextMenu()
{
    if (NULL != _hmenuNext)
    {
        _nextPos = 0;
        _EnableNext(FALSE);
        DestroyMenu(_hmenuNext);
        _hmenuNext = NULL;
    }
    return S_OK;
}

HRESULT CSearchBand::SetOCCallback(IOleCommandTarget *pOleCmdTarget)
{
    ResetNextMenu();

    ATOMICRELEASE(_pOCCmdTarget);

    _pOCCmdTarget = pOleCmdTarget;

    if (NULL != _pOCCmdTarget)
    {
        _pOCCmdTarget->AddRef();
    }
    
    return S_OK;
}

HRESULT CSearchBand::ShowDW(BOOL fShow)
{
#ifdef UNIX
    // We overide this method in order to provide renavigation if our earlier navigation
    // was cancelled for some reason.

    if (fShow)
    {
         // Should be there.
         // Just being sure we are able to navigate.

         ASSERT(IsWindow(_hwnd));
         ASSERT( _pidl );

         if (_pauto)
         {
             BSTR bstrURL = NULL;
             TCHAR szLocation[MAX_URL_STRING];

             // SHUnicodeToTChar does this - no memset 
             // memset(szLocation, 0, SIZEOF(szLocation));

             if( SUCCEEDED(_pauto->get_LocationURL(&bstrURL)) && bstrURL )
             {
                 // Fire navigation again if the navigation was canceled earlier for
                 // some reason (Maybe the proxy was not set).

                 SHUnicodeToTChar(bstrURL, szLocation, ARRAYSIZE(szLocation));
                 if( !StrCmpI( szLocation, NAVCANCELLED_URL ) ||
                     !StrCmpI( szLocation, OFFLINEINFO_URL  ) )
                     _NavigateOC();

                 SysFreeString(bstrURL);
             }

         }
    }
#endif

    HRESULT hres = CBrowserBand::ShowDW(fShow);
    _AddButtons(fShow);
    return hres;
}


HRESULT CSearchBand_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory
    HRESULT hr;

    *ppunk = NULL;
    CSearchBand *p = new CSearchBand();

    if (p)
    {
        *ppunk = SAFECAST(p, IDeskBand*);
        hr = NOERROR;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

HRESULT CSearchBand::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CSearchBand, IContextMenu),
        QITABENT(CSearchBand, IBandNavigate),
        QITABENT(CSearchBand, ISearchBandTBHelper),
        QITABENT(CSearchBand, IServiceProvider),
        QITABENT(CSearchBand, IInternetSecurityManager),
        { 0 },
    };

    HRESULT hres = QISearch(this, qit, riid, ppvObj);

    if (FAILED(hres))
        hres = CBrowserBand::QueryInterface(riid, ppvObj);

    return hres;
}

ULONG CSearchBand::AddRef()
{
    return CBrowserBand::AddRef();
}

ULONG CSearchBand::Release()
{
    return CBrowserBand::Release();
}

void CSearchBand::_Connect(BOOL fConnect)
{
    CBrowserBand::_Connect(fConnect);

    //  Now we need to expose ourselves so the control in the search assistant
    //  can talk to us.

    if (_pauto) 
    {
        IWebBrowserApp *pWebBrowserApp;
        HRESULT hr = _pauto->QueryInterface(IID_IWebBrowserApp, (void **)&pWebBrowserApp);

        if (SUCCEEDED(hr))
        {
            ASSERT(NULL != pWebBrowserApp);
            
            BSTR bstrProp = SysAllocString(c_wszThisBandIsYourBand);
    
            if (NULL != bstrProp)
            {
                VARIANT varThis;
    
                if (fConnect)
                {
                    varThis.vt = VT_UNKNOWN;
                    varThis.punkVal = (IBandNavigate *)this;
                }
                else
                {
                    varThis.vt = VT_EMPTY;
                }           
    
                pWebBrowserApp->PutProperty(bstrProp, varThis);
                
                SysFreeString(bstrProp);
            }
    
            pWebBrowserApp->Release();
        }
    }
}

void CSearchBand::_InitBrowser(void)
{
    CBrowserBand::_InitBrowser();
}

HRESULT CSearchBand::_NavigateOC()
{
    HRESULT hres = E_FAIL;

    if (_pidl) // don't want search pane to be navigated to home.
        return CBrowserBand::_NavigateOC();

    return hres;
}

HRESULT CSearchBand::GetBandInfo(DWORD dwBandID, DWORD fViewMode, 
                                DESKBANDINFO* pdbi) 
{
    _dwBandID = dwBandID;
    pdbi->dwModeFlags = DBIMF_FIXEDBMP | DBIMF_VARIABLEHEIGHT;
    
    pdbi->ptMinSize.x = 16;
    pdbi->ptMinSize.y = 0;
    pdbi->ptMaxSize.x = 32000; // random
    pdbi->ptMaxSize.y = 32000; // random
    pdbi->ptActual.y = -1;
    pdbi->ptActual.x = -1;
    pdbi->ptIntegral.y = 1;

    MLLoadStringW(IDS_BAND_SEARCH, pdbi->wszTitle, ARRAYSIZE(pdbi->wszTitle));
    
    return S_OK;
} 

//***   CSearchBand::IPersistStream::* {

HRESULT CSearchBand::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_SearchBand;

    return S_OK;
}

HRESULT CSearchBand::Load(IStream *pstm)
{
    _NavigateOC();
    
    return S_OK;
}

HRESULT CSearchBand::Save(IStream *pstm, BOOL fClearDirty)
{
    return S_OK;
}

HRESULT CSearchBand::Select(LPCITEMIDLIST pidl)
{
    HRESULT hres = S_OK;
    IServiceProvider * psp;
    LPITEMIDLIST pidlTemp = NULL;

    OCHost_QueryInterface(_hwnd, IID_IServiceProvider, (LPVOID*)&psp);
    if (psp)
    {
        IBrowserService * pbs;
        if (EVAL(SUCCEEDED(psp->QueryService(SID_SShellBrowser, IID_IBrowserService, (LPVOID*)&pbs))))
        {
            pbs->GetPidl(&pidlTemp);
            pbs->Release();
        }
        psp->Release();
    }

    if ((!pidlTemp) || (!ILIsEqual(pidlTemp, pidl)))
    {
        ILFree(_pidl);
        _pidl = ILClone(pidl);
        hres = _NavigateOC();
    }
    ILFree(pidlTemp);
    return hres;
}

STDMETHODIMP CSearchBand::QueryService(REFGUID guidService, REFIID riid, LPVOID* ppvObj)
{
    HRESULT hres;
    
    if (IsEqualGUID(guidService, SID_SInternetSecurityManager))
    {
        _bNewUrl = TRUE;
        hres = QueryInterface(riid, ppvObj);
    }
    else
        hres = CBrowserBand::QueryService(guidService, riid, ppvObj);

    return hres;
}

HRESULT CSearchBand::OnWinEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres)
{
    if ((WM_WININICHANGE == uMsg) && lParam &&
        ((0 == StrCmpW((LPCWSTR)lParam, SEARCH_SETTINGS_CHANGEDW)) ||
         (0 == StrCmpA((LPCSTR) lParam, SEARCH_SETTINGS_CHANGEDA))))
    {
        _NavigateToSearchUrl();
    }   

    return CBrowserBand::OnWinEvent(hwnd, uMsg, wParam, lParam, plres);
}



BOOL CSearchBand::_IsSafeUrl(LPCWSTR pwszUrl)
{
    BOOL bRet = FALSE;
    HKEY hkey;

    if (_bNewUrl || !_bUseDefault)
    {
        WCHAR wsz[MAX_URL_STRING];
        DWORD cch = ARRAYSIZE(wsz);

        if (SUCCEEDED(UrlCanonicalizeW(pwszUrl, wsz, &cch, 0)) && cch > 0)
        {
            // the first time this f-n is called, url passed in is the url of
            // the top most frame -- if that's not one of our 'safe' urls we
            // don't want to use this security mgr because it is possible 
            // that the outer frame hosts iframe w/ 'safe' site and scripts
            // shell dispatch from the outside thus being able to do anything
            // it wants.
            if (_wszCache[0] != L'\0')
            {
                if ((_nCmpLength && StrCmpNIW(wsz, _wszCache, _nCmpLength) == 0)
                || (!_nCmpLength && StrCmpIW(wsz, _wszCache) == 0))
                    return _bIsCacheSafe;
            }
            
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\SafeSites", 0, KEY_READ, &hkey) == ERROR_SUCCESS)
            {        
                WCHAR wszValue[MAX_PATH];
                WCHAR wszData[MAX_URL_STRING];
                DWORD cbData = SIZEOF(wszData);
                DWORD cchValue = ARRAYSIZE(wszValue);

                for (int i=0; RegEnumValueW(hkey, i, wszValue, &cchValue, NULL, NULL, (LPBYTE)wszData, &cbData) == ERROR_SUCCESS; i++)
                {
                    if (SHExpandEnvironmentStringsW(wszData, _wszCache, ARRAYSIZE(_wszCache)) > 0)
                    {
                        cchValue = ARRAYSIZE(_wszCache);
                        if (SUCCEEDED(UrlCanonicalizeW(_wszCache, _wszCache, &cchValue, 0)) && (cchValue > 0))
                        {
                            if (_wszCache[cchValue - 1] == L'*')
                            {
                                _nCmpLength = cchValue - 1;
                                bRet = StrCmpNIW(wsz, _wszCache, _nCmpLength) == 0;
                            }
                            else
                            {
                                _nCmpLength = 0;
                                bRet = StrCmpIW(wsz, _wszCache) == 0;
                            }

                            _bIsCacheSafe = bRet;
                            if (bRet)
                                break;
                        }
                        cbData = SIZEOF(_wszCache);
                        cchValue = ARRAYSIZE(wszValue);
                    }
                }
                RegCloseKey(hkey);        
            }

            // we did not find the url in the list of 'safe' sites
            // _wszCache now point to the last url read from the registry
            // ajdust it to point pwszUrl, _bIsCacheSafe is correct already
            if (!bRet)
                lstrcpynW(_wszCache, wsz, ARRAYSIZE(_wszCache));

            if (_bNewUrl)
            {
                _bNewUrl = FALSE;
                _bUseDefault = !bRet;
            }
        }
    }
        
    return bRet;
}

// }

IDeskBand* CSearchBand_Create()
{
    HRESULT hr;
    IUnknown *punk;
    IDeskBand* pistb = NULL;

    hr = CSearchBand_CreateInstance(NULL, &punk, NULL);
    if (SUCCEEDED(hr))
    {
        if (FAILED(punk->QueryInterface(IID_IDeskBand,(LPVOID *)&pistb)))
        {
            ASSERT(0);
        }
        // if we succeeded, release the 2nd refcnt and return non-NULL;
        // if we failed   , release the 1st refcnt and return NULL
        punk->Release();
    }
    return pistb;
}

// }


//***   CCommBand {
//

////////////////
///  Comm (BrowserOC) band

class CCommBand : public CBrowserBand
{

public:    
    // *** IPersistStream methods ***
    // (others use base class implementation) 
    virtual STDMETHODIMP GetClassID(CLSID *pClassID);
    virtual STDMETHODIMP Load(IStream *pStm);
    virtual STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty);

    // *** IDockingWindow methods ***
    virtual STDMETHODIMP ShowDW(BOOL fShow);

protected:
    CCommBand();
    virtual ~CCommBand();

    friend HRESULT CCommBand_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);       // for ctor

};

CCommBand::CCommBand() :
    CBrowserBand()
{
    _fBlockSIDProxy = FALSE;
    _fBlockDrop = TRUE;
    _fCustomTitle = TRUE;
    _wszTitle[0] = L'\0';

    _dwModeFlags = DBIMF_VARIABLEHEIGHT;

    return;
}

CCommBand::~CCommBand()
{
}

HRESULT CCommBand_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory
    *ppunk = NULL;
    LPITEMIDLIST pidlNew;
    HRESULT hr = IECreateFromPath(L"about:blank", &pidlNew);
    if (SUCCEEDED(hr))
    {
        CCommBand *p = new CCommBand();
        if (p)
        {
            p->_pidl = pidlNew;
            *ppunk = SAFECAST(p, IDeskBand*);
            hr = NOERROR;
        }
        else
        {
            ILFree(pidlNew);
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}



//***   CCommBand::IPersistStream::* {

HRESULT CCommBand::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_CommBand;

    return S_OK;
}

HRESULT CCommBand::Load(IStream *pstm)
{
//    _NavigateOC();
    
    return S_OK;
}

HRESULT CCommBand::Save(IStream *pstm, BOOL fClearDirty)
{
    return S_OK;
}

HRESULT CCommBand::ShowDW(BOOL fShow)
{
    // so that the contained Browser OC event gets fired
    if (_pauto) {
        _pauto->put_Visible(fShow);
    }

    return CBrowserBand::ShowDW(fShow);
}

// }

extern "C" HRESULT CALLBACK CShellSearchExt_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
extern "C" HRESULT CALLBACK CWebSearchExt_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
#define _FILESEARCHBAND_

// unlike CShellFindExt which is a real context menu implementation CShellSearchExt is a static one
// i.e. it does not get instantiated until the invoke time
class CShellSearchExt : public IContextMenu, public IObjectWithSite
{
public:
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface(THIS_ REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // *** IContextMenu methods ***
    STDMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pici);
    STDMETHODIMP GetCommandString(UINT_PTR idCmd, UINT wFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax);

    // *** IObjectWithSite methods ***

    STDMETHODIMP SetSite(IUnknown *pUnkSite);        
    STDMETHODIMP GetSite(REFIID riid, void **ppvSite);

protected:
    CShellSearchExt();
    virtual ~CShellSearchExt();

    virtual BOOL        _GetSearchUrls(LPGUID lpguid, LPTSTR lpsz, DWORD cch, 
                                       LPTSTR pszUrlNavNew, DWORD cchNavNew, BOOL *pfRunInProcess);
    static STDMETHODIMP _IsShellSearchBand(REFGUID guidSearch);
    static STDMETHODIMP _ShowShellSearchResults(IWebBrowser2* pwb2, BOOL fNewFrame, REFGUID guidSearch);

private:    
    LONG               _cRef;
    IUnknown*          _pSite;

    friend HRESULT CALLBACK CShellSearchExt_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
};

HRESULT CALLBACK CShellSearchExt_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CShellSearchExt* psse;

    psse = new CShellSearchExt(); 
    if (psse)
    {
        *ppunk = SAFECAST(psse, IContextMenu*);
        return S_OK;
    }
    else
    {
        *ppunk = NULL;
        return E_OUTOFMEMORY;
    }
}

STDMETHODIMP CShellSearchExt::QueryInterface(THIS_ REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CShellSearchExt, IContextMenu),         
        QITABENT(CShellSearchExt, IObjectWithSite),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CShellSearchExt::AddRef()
{
    InterlockedIncrement(&_cRef);
    return _cRef;
}

STDMETHODIMP_(ULONG) CShellSearchExt::Release()
{
    if (InterlockedDecrement(&_cRef) > 0)
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CShellSearchExt::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    return E_NOTIMPL;
}

#define SZ_SHELL_SEARCH TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FindExtensions\\Static\\ShellSearch")

BOOL CShellSearchExt::_GetSearchUrls(LPGUID lpguidSearch, LPTSTR pszUrl, DWORD cch, 
        LPTSTR pszUrlNavNew, DWORD cchNavNew, BOOL *pfRunInProcess)
{
    HKEY hkey;
    BOOL bRet = FALSE;


    *pfRunInProcess = FALSE;        // Assume that we are not forcing it to run in process.
    if (pszUrl == NULL || IsEqualGUID(*lpguidSearch, GUID_NULL) || pszUrlNavNew == NULL)
        return bRet;

    *pszUrlNavNew = TEXT('\0');

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, SZ_SHELL_SEARCH, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
    {
        int i;
        TCHAR szSubKey[32];
        HKEY  hkeySub;

        for (i = 0; wnsprintf(szSubKey, ARRAYSIZE(szSubKey), TEXT("%d"), i), 
                    RegOpenKey(hkey, szSubKey, &hkeySub) == ERROR_SUCCESS;
             i++)
        {
            TCHAR szSearchGuid[MAX_PATH];
            DWORD cb;
            DWORD dwType;

            cb = SIZEOF(szSearchGuid);
            if (SHGetValue(hkeySub, TEXT("SearchGUID"), NULL, &dwType, (BYTE*)szSearchGuid, &cb) == ERROR_SUCCESS)
            {
                GUID guid;
                
                if (GUIDFromString(szSearchGuid, &guid) &&
                    IsEqualGUID(guid, *lpguidSearch))
                {
                    cb = cch * sizeof(TCHAR);
                    bRet = (SHGetValue(hkeySub, TEXT("SearchGUID\\Url"), NULL, &dwType, (BYTE*)pszUrl, &cb) == ERROR_SUCCESS);
                    if (bRet || IsEqualGUID(*lpguidSearch, SRCID_SFileSearch))
                    {
                        if (!bRet)
                        {
                            *pszUrl = TEXT('\0');
                            // in file search case we don't need url but we still succeed
                            bRet = TRUE;
                        }
                        // See if there is a URL that we should navigate to if we
                        // are navigating to a new 
                        cb = cchNavNew * sizeof(TCHAR);
                        SHGetValue(hkeySub, TEXT("SearchGUID\\UrlNavNew"), NULL, &dwType, (BYTE*)pszUrlNavNew, &cb);

                        // likewise try to grab the RunInProcess flag, if not there or zero then off, else on
                        // reuse szSearchGuid for now...
                        *pfRunInProcess = (BOOL)SHRegGetIntW(hkeySub, L"RunInProcess", 0);
                    }
                    RegCloseKey(hkeySub);
                    break;
                }
            }
            RegCloseKey(hkeySub);
        }
        RegCloseKey(hkey);
    }
    if (!bRet)
        pszUrl[0] = TEXT('\0');
    
    return bRet;
}

STDMETHODIMP CShellSearchExt::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    HRESULT           hres = E_FAIL;
    IWebBrowser2*     pwb2 = NULL;
    TCHAR             szUrl[MAX_URL_STRING];
    TCHAR             szUrlNavNew[MAX_URL_STRING];
    IServiceProvider* psp;
    BOOL              bNewFrame = FALSE;

    // First get the Urls such that we can see which class we should create...
    GUID        guid, guidSearch = GUID_NULL;
    BOOL        fRunInProcess;
    CLSID       clsidBand; // deskband object for search
    BOOL        fShellSearchBand = TRUE;    // Win32 (vs. html-hosting) band?

    //  Retrieve search ID from invoke params
    if (pici->lpParameters && GUIDFromStringA(pici->lpParameters, &guid))
        guidSearch = guid;

    //  establish the search band
    if ((fShellSearchBand = (S_OK == _IsShellSearchBand( guidSearch ))))
    {
        if (SHRestricted(REST_NOFIND) && IsEqualGUID(guidSearch, SRCID_SFileSearch))
            return HRESULT_FROM_WIN32(ERROR_CANCELLED);   // user saw the error        

        clsidBand = CLSID_FileSearchBand;
    }
    else
    {
        clsidBand = CLSID_SearchBand;
        //  retrieve search URLs from registry
        if (!_GetSearchUrls(&guidSearch, szUrl, ARRAYSIZE(szUrl), szUrlNavNew, ARRAYSIZE(szUrlNavNew), &fRunInProcess))
            return E_FAIL;
    }

    // if invoked from within a browser reuse it, else open a new browser
    hres = IUnknown_QueryService(_pSite, SID_STopLevelBrowser, IID_IServiceProvider, (LPVOID*)&psp);
    if (SUCCEEDED(hres))
    {
        hres = psp->QueryService(SID_SWebBrowserApp, IID_IWebBrowser2, (LPVOID*)&pwb2);
        psp->Release();
    }

    if (FAILED(hres))
    {
        //  Note: we want the frame to display shell characteristics (CLSID_ShellBrowserWindow),
        //  including persistence behavior, if we're loading shell search (CLSID_FileSearchBand).
        if (fRunInProcess || IsEqualGUID( clsidBand, CLSID_FileSearchBand) )
            hres = CoCreateInstance(CLSID_ShellBrowserWindow, NULL, CLSCTX_LOCAL_SERVER, IID_IWebBrowser2, (void **)&pwb2);
        else
            hres = CoCreateInstance(CLSID_InternetExplorer, NULL, CLSCTX_LOCAL_SERVER, IID_IWebBrowser2, (void **)&pwb2);
        if (FAILED(hres))
            return hres;
        bNewFrame = TRUE;
    }

    ASSERT(pwb2);
    hres = E_FAIL;

    // show html-hosting band
    VARIANT var;
    VARIANT varEmpty = {0};
    if (SUCCEEDED( (hres = _BstrVariantFromGUID( clsidBand, &var )) ))
    {
        hres = pwb2->ShowBrowserBar(&var, &varEmpty, &varEmpty);
        VariantClear( &var );
    }

    if (SUCCEEDED(hres))
    {
        if (fShellSearchBand)
        {
            hres = _ShowShellSearchResults( pwb2, bNewFrame, guidSearch);
        }
        else
        {
            SA_BSTR bstr;
            VARIANT varFlags;
            varFlags.vt = VT_I4;
            varFlags.lVal = navBrowserBar;

            SHTCharToUnicode(szUrl, bstr.wsz, ARRAYSIZE(bstr.wsz));
            bstr.cb = lstrlenW(bstr.wsz) * sizeof(WCHAR);

            var.vt = VT_BSTR;
            var.bstrVal = bstr.wsz;

            // if we opened a new window, navigate the right side to about.blank
            if (bNewFrame)
            {
                SA_BSTR       bstrNavNew;

                if (szUrlNavNew[0])
                    SHTCharToUnicode(szUrlNavNew, bstrNavNew.wsz, ARRAYSIZE(bstrNavNew.wsz));
                else
                    lstrcpyW(bstrNavNew.wsz, L"about:blank");

                bstrNavNew.cb = lstrlenW(bstrNavNew.wsz) * sizeof(WCHAR);

                // we don't care about the error here
                pwb2->Navigate(bstrNavNew.wsz, &varEmpty, &varEmpty, &varEmpty, &varEmpty);
            }

            // navigate the search bar to the correct url
            hres = pwb2->Navigate2(&var, &varFlags, &varEmpty, &varEmpty, &varEmpty);
        }
    }

    if (SUCCEEDED(hres) && bNewFrame)
        hres = pwb2->put_Visible(TRUE);

    pwb2->Release();
    return hres;
}

STDMETHODIMP CShellSearchExt::_IsShellSearchBand( REFGUID guidSearch )
{
    if (IsEqualGUID( guidSearch, SRCID_SFileSearch ) ||
        IsEqualGUID( guidSearch, SRCID_SFindComputer) || 
        IsEqualGUID( guidSearch, SRCID_SFindPrinter))
        return S_OK;
    return S_FALSE;
}

STDMETHODIMP CShellSearchExt::_ShowShellSearchResults( 
    IWebBrowser2* pwb2, 
    BOOL bNewFrame, 
    REFGUID guidSearch )
{
    ASSERT( pwb2 );
    ASSERT( S_OK == _IsShellSearchBand( guidSearch ) );

    HRESULT hr;
    VARIANT varBand;
    if (SUCCEEDED( (hr = _BstrVariantFromGUID( CLSID_FileSearchBand, &varBand )) ))
    {
        //  Retrieve the FileSearchBand's unknown from the browser frame as a VT_UNKNOWN property;
        //  (FileSearchBand initialized and this when he was created and hosted.)
        VARIANT varFsb;
        VariantInit( &varFsb );    
        if (SUCCEEDED( (hr = pwb2->GetProperty( varBand.bstrVal, &varFsb )) ))
        {
            if (VT_UNKNOWN == varFsb.vt && varFsb.punkVal != NULL )
            {
                //  Retrieve the IFileSearchBand interface from the punk.
                IFileSearchBand* pfsb;
                if (SUCCEEDED( (hr = varFsb.punkVal->QueryInterface( IID_IFileSearchBand, (LPVOID*)&pfsb )) ))
                {
                    //  Assign the correct search type to the band
                    VARIANT varSearchID;
                    if (SUCCEEDED( (hr = _BstrVariantFromGUID( guidSearch, &varSearchID )) ))
                    {
                        VARIANT      varNil;
                        VARIANT_BOOL bNavToResults = bNewFrame ? VARIANT_TRUE : VARIANT_FALSE ;
                        VariantInit( &varNil );
                        pfsb->SetSearchParameters( &varSearchID.bstrVal, bNavToResults, &varNil, &varNil );
                        VariantClear( &varSearchID );
                    }
                    pfsb->Release();
                }
            }
            VariantClear( &varFsb );
        }
        VariantClear( &varBand );
    }
    return hr;
}

STDMETHODIMP CShellSearchExt::GetCommandString(UINT_PTR idCmd, UINT wFlags, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
    return E_NOTIMPL;
}

STDMETHODIMP CShellSearchExt::SetSite(IUnknown *pUnkSite)
{
    ATOMICRELEASE(_pSite);
    
    _pSite = pUnkSite;
    if (_pSite)
        _pSite->AddRef();

    return NOERROR;
}
    
STDMETHODIMP CShellSearchExt::GetSite(REFIID riid, void **ppvSite)
{
    if (_pSite)
        return _pSite->QueryInterface(riid, ppvSite);

    *ppvSite = NULL;
    return E_NOINTERFACE;
}

CShellSearchExt::CShellSearchExt() : _cRef(1), _pSite(NULL)
{
}

CShellSearchExt::~CShellSearchExt()
{
    ATOMICRELEASE(_pSite);
}

class CWebSearchExt : public CShellSearchExt
{
protected:
    CWebSearchExt();
    virtual ~CWebSearchExt();
    virtual BOOL _GetSearchUrls(LPGUID lpguidSearch, LPTSTR pszUrl, DWORD cch, 
                                LPTSTR pszUrlNavNew, DWORD cchNavNew, BOOL *pfRunInProcess);
    
    friend HRESULT CALLBACK CWebSearchExt_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
};

HRESULT CALLBACK CWebSearchExt_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CWebSearchExt* pwse;

    pwse = new CWebSearchExt();
    if (pwse)
    {
        *ppunk = SAFECAST(pwse, IContextMenu*);
        return S_OK;
    }
    else
    {
        *ppunk = NULL;
        return E_OUTOFMEMORY;
    }
}

CWebSearchExt::CWebSearchExt() : CShellSearchExt()
{
}

CWebSearchExt::~CWebSearchExt()
{
}

BOOL CWebSearchExt::_GetSearchUrls(LPGUID lpguidSearch, LPTSTR pszUrl, DWORD cch, 
                                   LPTSTR pszUrlNavNew, DWORD cchNavNew, BOOL *pfRunInProcess)
{
    BOOL    bRet = FALSE;

    // Currently does not support NavNew, can be extended later if desired, likewise for RunInProcess...
    *pfRunInProcess = FALSE;
    if (pszUrlNavNew && cchNavNew)
        *pszUrlNavNew = TEXT('\0');

    bRet = GetDefaultInternetSearchUrl(pszUrl, cch, TRUE);
    
    return bRet;
}
