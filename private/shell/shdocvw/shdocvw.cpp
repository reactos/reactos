#include "priv.h"
#include "hlframe.h"
#include "dochost.h"
#include "bindcb.h"
#include "iface.h"
#include "resource.h"
#include "idhidden.h"
#include "shdocfl.h"

const ITEMIDLIST s_idNull = { {0} };
extern HRESULT VariantClearLazy(VARIANTARG *pvarg);

#define DM_CACHETRACE   0
#define DM_ZONECROSSING 0

#define NAVMSG3(psz, x, y)	TraceMsg(0, "NAV::%s %x %x", psz, x, y)
#define PAINTMSG(psz,x)		TraceMsg(0, "TraceMsgPAINT::%s %x", psz, x)
#define JMPMSG(psz, psz2)	TraceMsg(0, "TraceMsgCDOV::%s %s", psz, psz2)
#define JMPMSG2(psz, x)		TraceMsg(0, "TraceMsgCDOV::%s %x", psz, x)
#define DOFMSG(psz)		TraceMsg(0, "TraceMsgDOF::%s", psz)
#define DOFMSG2(psz, x)		TraceMsg(0, "TraceMsgDOF::%s %x", psz, x)
#define URLMSG(psz)		TraceMsg(0, "TraceMsgDOF::%s", psz)
#define URLMSG2(psz, x)		TraceMsg(0, "TraceMsgDOF::%s %x", psz, x)
#define URLMSG3(psz, x, y)	TraceMsg(0, "TraceMsgDOF::%s %x %x", psz, x, y)
#define BSCMSG(psz, i, j)	TraceMsg(0, "TraceMsgBSC::%s %x %x", psz, i, j)
#define BSCMSG3(psz, i, j, k)	TraceMsg(0, "TraceMsgBSC::%s %x %x %x", psz, i, j, k)
#define BSCMSG4(psz, i, j, k, l)	TraceMsg(0, "TraceMsgBSC::%s %x %x %x %x", psz, i, j, k, l)
#define BSCMSGS(psz, sz)	TraceMsg(0, "TraceMsgBSC::%s %s", psz, sz)
#define OIPSMSG(psz)		TraceMsg(0, "TraceMsgOIPS::%s", psz)
#define OIPSMSG3(psz, sz, p)	TraceMsg(0, "TraceMsgOIPS::%s %s,%x", psz, sz,p)
#define VIEWMSG(psz)		TraceMsg(0, "CDOV::%s", psz)
#define VIEWMSG2(psz,xx)	TraceMsg(0, "CDOV::%s %x", psz,xx)
#define CACHEMSG(psz, d)        TraceMsg(0, "CDocObjectCtx::%s %d", psz, d)
#define OPENMSG(psz)		TraceMsg(0, "OPENING %s", psz)
#define OPENMSG2(psz, x)	TraceMsg(0, "OPENING %s %x", psz, x)
#define HFRMMSG(psz)		TraceMsg(0, "HFRM::%s", psz)
#define HFRMMSG2(psz, x, y)	TraceMsg(0, "HFRM::%s %x %x", psz, x, y)
#define MNKMSG(psz, psz2)	TraceMsg(0, "MNK::%s (%s)", psz, psz2)
#define CHAINMSG(psz, x)	TraceMsg(0, "CHAIN::%s %x", psz, x)
#define SHVMSG(psz, x, y)	TraceMsg(0, "SHV::%s %x %x", psz, x, y)
#define HOMEMSG(psz, psz2, x)	TraceMsg(0, "HOME::%s %s %x", psz, psz2, x)
#define SAVEMSG(psz, x)		TraceMsg(0, "SAVE::%s %x", psz, x)
#define PERFMSG(psz, x)		TraceMsg(TF_SHDPERF, "PERF::%s %d msec", psz, x)

// this saves the view information for this shell view class
typedef struct {
    UINT cbSize;

    BOOL fCoolbar:1;
    BOOL fToolbar:1;
    BOOL fStatusbar:1;

} IEVIEWINFO;



class CDocObjectView :
    /* Group 1 */    public IShellView2, public IDropTarget
                   , public IViewObject, public IAdviseSink
                   , public IOleCommandTarget
                   , public IDocViewSite
                   , public IPrivateOleObject
                   , public IPersistFolder 
{
protected:
    CDocObjectHost* _pdoh;
    IDocHostObject* _pdho;

    UINT _cRef;
    IShellFolder *_psf;
    IShellBrowser* _psb;
    IOleCommandTarget* _pctShellBrowser;
    FOLDERSETTINGS _fs;
    LPITEMIDLIST _pidl;
    LPTSTR _pszLocation;
    UINT _uiCP;

    // Advisory connection
    IAdviseSink *_padvise;
    DWORD _advise_aspect;
    DWORD _advise_advf;

    BOOL _fInHistory : 1;
    BOOL _fSaveViewState : 1;
    BOOL _fStopped :1;  // stores whether the user has stopped load or not
    BOOL _fIsGet : 1;
    BOOL _fCanCache : 1;
    BOOL _fCanCacheFetched : 1;
    BOOL _fPrevViewIsDocView : 1;
    BOOL _fSelfDragging : 1;       // DocObject is the drag source

    SYSTEMTIME _stLastRefresh;
    HWND    _hwndParent;

    UINT _uState;
    // DragContext
    DWORD _dwDragEffect;

    ~CDocObjectView();

    void    _RestoreViewSettings();
    void    _SaveViewState();
    void    _GetViewSettings(IEVIEWINFO* pievi);
    int     _ShowControl(UINT idControl, int idCmd);
    void _CreateDocObjHost(IShellView * psvPrev);
    void _CompleteDocHostPassing(IShellView *psvPrev, HRESULT hres);
    BOOL _CanUseCache();
    void _SetLastRefreshTime() { GetSystemTime(&_stLastRefresh); };


    void _ConnectHostSink();
    void _DisconnectHostSink();

public:
    CDocObjectView(LPCITEMIDLIST pidl, IShellFolder *psf);

    // *** IUnknown methods ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) ;
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IOleWindow methods ***
    virtual STDMETHODIMP GetWindow(HWND * lphwnd);
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);

    // *** IShellView methods ***
    virtual STDMETHODIMP TranslateAccelerator(LPMSG lpmsg);
    virtual STDMETHODIMP EnableModelessSV(BOOL fEnable);
    virtual STDMETHODIMP UIActivate(UINT uState);
    virtual STDMETHODIMP Refresh();

    virtual STDMETHODIMP CreateViewWindow(IShellView  *lpPrevView,
                    LPCFOLDERSETTINGS lpfs, IShellBrowser  * psb,
                    RECT * prcView, HWND  *phWnd);
    virtual STDMETHODIMP DestroyViewWindow();
    virtual STDMETHODIMP GetCurrentInfo(LPFOLDERSETTINGS lpfs);
    virtual STDMETHODIMP AddPropertySheetPages(DWORD dwReserved,
                    LPFNADDPROPSHEETPAGE lpfn, LPARAM lparam);
    virtual STDMETHODIMP SaveViewState();
    virtual STDMETHODIMP SelectItem(LPCITEMIDLIST pidlItem, UINT uFlags);
    virtual STDMETHODIMP GetItemObject(UINT uItem, REFIID riid,
                    LPVOID *ppv);

    virtual STDMETHODIMP GetView(SHELLVIEWID* pvid, ULONG uView) ;
    virtual STDMETHODIMP CreateViewWindow2(LPSV2CVW2_PARAMS lpParams) ;
    virtual STDMETHODIMP HandleRename(LPCITEMIDLIST pidl) ;
    virtual STDMETHODIMP SelectAndPositionItem(LPCITEMIDLIST pidlItem, UINT uFlags, POINT *ppt) {
        return E_NOTIMPL;
    }


    // IViewObject
    virtual STDMETHODIMP Draw(DWORD, LONG, void *, DVTARGETDEVICE *, HDC, HDC,
        const RECTL *, const RECTL *, BOOL (*)(ULONG_PTR), ULONG_PTR);
    virtual STDMETHODIMP GetColorSet(DWORD, LONG, void *, DVTARGETDEVICE *,
        HDC, LOGPALETTE **);
    virtual STDMETHODIMP Freeze(DWORD, LONG, void *, DWORD *);
    virtual STDMETHODIMP Unfreeze(DWORD);
    virtual STDMETHODIMP SetAdvise(DWORD, DWORD, IAdviseSink *);
    virtual STDMETHODIMP GetAdvise(DWORD *, DWORD *, IAdviseSink **);

    // IAdviseSink
    virtual STDMETHODIMP_(void) OnDataChange(FORMATETC *, STGMEDIUM *);
    virtual STDMETHODIMP_(void) OnViewChange(DWORD dwAspect, LONG lindex);
    virtual STDMETHODIMP_(void) OnRename(IMoniker *);
    virtual STDMETHODIMP_(void) OnSave();
    virtual STDMETHODIMP_(void) OnClose();

    // IOleCommandTarget
    virtual STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup,
    ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup,
    DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);


    // IDropTarget
    virtual STDMETHODIMP DragEnter(
    IDataObject *pdtobj,
    DWORD grfKeyState,
    POINTL pt,
    DWORD *pdwEffect);

    virtual STDMETHODIMP DragOver(
    DWORD grfKeyState,
    POINTL pt,
    DWORD *pdwEffect);

    virtual STDMETHODIMP DragLeave(void);

    virtual STDMETHODIMP Drop(
    IDataObject *pdtobj,
    DWORD grfKeyState,
    POINTL pt,
    DWORD *pdwEffect);

    // IDocViewSite
    virtual STDMETHODIMP OnSetTitle(VARIANTARG *pvTitle);

    // IPrivateOleObject
    virtual HRESULT STDMETHODCALLTYPE SetExtent( DWORD dwDrawAspect, SIZEL *psizel);
    virtual HRESULT STDMETHODCALLTYPE GetExtent( DWORD dwDrawAspect, SIZEL *psizel);

    // IPersist methods
    virtual STDMETHODIMP GetClassID(CLSID *pclsid);
    
    // IPersistFolder methods 
    virtual STDMETHODIMP Initialize(LPCITEMIDLIST pidl);
    
};

//--------------------------------------------------------------------------
// Detecting a memory leak
//--------------------------------------------------------------------------

CDocObjectView::~CDocObjectView()
{
    // just in case
    DestroyViewWindow();

    if (_pidl) 
    {
        ILFree(_pidl);
        _pidl = NULL;
    }

    ATOMICRELEASE(_psf);

    if (_pszLocation)
        LocalFree(_pszLocation);

    ATOMICRELEASE(_padvise);

    TraceMsg(TF_SHDLIFE, "dtor CDocObjectView(%x) being destructed", this);
}

CDocObjectView::CDocObjectView(LPCITEMIDLIST pidl, IShellFolder* psf) :
    _psf(psf),
    _cRef(1)
{
    TraceMsg(TF_SHDLIFE, "ctor CDocObjectView(%x) being constructed", this);

    _dwDragEffect = DROPEFFECT_NONE;
    if (pidl) 
    {
        _pidl = ILClone(pidl);

        if (_pidl)
        {
#ifndef UNIX
            WCHAR wszPath[MAX_URL_STRING];
#else
            WCHAR wszPath[MAX_URL_STRING] = TEXT("");
#endif
            if(IEILGetFragment(_pidl, wszPath, SIZECHARS(wszPath)))
                Str_SetPtr(&_pszLocation, wszPath);

            _uiCP = IEILGetCP(_pidl);
        }
    }

    ASSERT(psf);
    if (_psf) {
        _psf->AddRef();
    }
}

HRESULT CDocObjectView_Create(IShellView** ppvOut, IShellFolder * psf, LPCITEMIDLIST pidl)
{
    *ppvOut = new CDocObjectView(pidl, psf);
    return (*ppvOut) ? S_OK : E_OUTOFMEMORY;
}

HRESULT CDocObjectView::GetWindow(HWND * lphwnd)
{
    *lphwnd = NULL;
    if (_pdoh)
        return _pdoh->GetWindow(lphwnd);
    return S_OK;
}

HRESULT CDocObjectView::ContextSensitiveHelp(BOOL fEnterMode)
{
    // NOTES: This is optional
    return E_NOTIMPL;	// As specified in Kraig's document (optional)
}



// IShellView::TranslateAccelerator
//  From Browser -> DocView -> DocObject
HRESULT CDocObjectView::TranslateAccelerator(LPMSG lpmsg)
{
    HRESULT hres = S_FALSE;
    if (_pdoh)
        hres = _pdoh->_xao.TranslateAccelerator(lpmsg);

    if (hres == S_FALSE && lpmsg->message == WM_KEYDOWN) {
        HWND hwndFocus = GetFocus();
        HWND hwndView = NULL;
        
        if(_pdoh) //BUGBUGZEKEL i found this NULL
            _pdoh->GetWindow(&hwndView);

        if (hwndView && IsChildOrSelf(hwndView, hwndFocus) == S_OK) {
            
            switch (lpmsg->wParam) {

            case VK_BACK:
                TranslateMessage(lpmsg);
                DispatchMessage(lpmsg);
                hres = NOERROR;
                break;

            }
        }
    }

    return hres;
}

// IShellView::EnableModelessSV
//  From Browser -> DocView -> DocObject
HRESULT CDocObjectView::EnableModelessSV(BOOL fEnable)
{
    HRESULT hres = S_OK;
    // We have no modeless window to be enabled/disabled
    TraceMsg(0, "sdv TR - ::EnableModelessSV(%d) called", fEnable);
    if (_pdoh) {
        hres = _pdoh->_xao.EnableModeless(fEnable);
        TraceMsg(0, "sdv TR - _piact->EnableModeless returned %x", hres);
    }
    return hres;
}


HRESULT CDocObjectView::UIActivate(UINT uState)
{
    HRESULT hres = E_FAIL;
    if (_pdoh) {
        hres = _pdoh->UIActivate(uState, _fPrevViewIsDocView);
    }
    _uState = uState;
    return hres;
}

HRESULT CDocObjectView::Refresh()
{
    _fStopped = FALSE;

    if (_pdoh) {
        VARIANT v = {0};
        v.vt = VT_I4;
        v.lVal = OLECMDIDF_REFRESH_NO_CACHE;
        // send this to exec so that it will update last refresh time.
        // all refresh to dochost should go through our own exec
        return Exec(NULL, OLECMDID_REFRESH, OLECMDEXECOPT_PROMPTUSER, &v, NULL);
    }

    return S_OK;
}


// Return:
//   S_OK: It is a folder shortcut and ppidlTarget is filled out if provided.
//   S_FALSE: It is not a folder shortcut and ppidlTarget is filled with NULL if provided.
//   FAILED: An error occured trying to detect.
//
// Don't use this function if there is a better way.  Look into using
// IBrowserFrameOptions. -BryanSt
HRESULT IsFolderShortcutPidl(IN LPCITEMIDLIST pidl)
{
    IShellFolder * psf = NULL;
    HRESULT hr = IEBindToObject(pidl, &psf);

    if (SUCCEEDED(hr))
    {
        IShellLinkA * psl = NULL;

        hr = psf->QueryInterface(IID_IShellLinkA, (void **) &psl);
        if (SUCCEEDED(hr))
        {
            hr = S_OK;
            psl->Release();
        }
        else
            hr = S_FALSE;   // It's not a folder shortcut.

        psf->Release();
    }

    return hr;
}


// WARNING: This function explicitely creates URLMON monikers
//   because that's what the caller needs.  Some NSEs will
//   support SFGAO_CANMONIKER and IShellFolder::BindToObject(IMoniker)
//   but this function doesn't use that logicn intentinally.
STDAPI _URLMONMonikerFromPidl(LPCITEMIDLIST pidl, IMoniker** ppmk, BOOL* pfFileProtocol)
{
    TCHAR szPath[MAX_URL_STRING];
    HRESULT hres = E_UNEXPECTED;
        
    *ppmk = NULL;
    *pfFileProtocol = FALSE;
    MNKMSG(TEXT("_CreateMonikerForPidl"), TEXT("called"));

    AssertMsg((S_OK != IsFolderShortcutPidl(pidl)), TEXT("We shouldn't get Folder Shortcuts here because we don't deref them to get the target. And we should never need to."));

    // Is this a child of the "Internet Explorer" name space?
    if (!IsURLChild(pidl, TRUE))
    {
        // No, so we want to get the display name to use to
        // create the moniker.  We will try to turn it into
        // an URL if it isn't already an URL.

        // NOTE: We don't try SFGAO_CANMONIKER/IEBindToObject(pidl, IID_IMoniker)
        //       because the caller requires that this
        //       IMoniker come from URLMON.
        HRESULT hrTemp = SHGetPathFromIDList(pidl, szPath);

        AssertMsg(SUCCEEDED(hrTemp), TEXT("_URLMONMonikerFromPidl() failed SHGetPathFromIDList() which is really bad because we probably won't be able to create a moniker from it.  We will try to create a URLMON moniker below."));
        if (SUCCEEDED(hrTemp))
        {        
            //  this should never be a fully qualified URL
            DWORD cchPath = ARRAYSIZE(szPath);
            
            ASSERT(URL_SCHEME_INVALID == GetUrlScheme(szPath));
            if(SUCCEEDED(hres = UrlCreateFromPath(szPath, szPath, &cchPath, 0))) 
            {
                MNKMSG(TEXT("_CreateMonikerForPidl Creating File Moniker"), szPath);
            }
            
            *pfFileProtocol = TRUE;
        }
        
    } 
    else 
    {
        // Yes so we are guaranteed this is from the "Internet Explorer"
        // name space so remove Fragment hidden itemID.
        // We do this because we navigate to the fragment later,
        // after the page is downloaded.  This is also needed
        // for delegates of the IE name space. (FTP is one example of
        // a delegate).
        ASSERT(pidl);
        ASSERT(!ILIsEmpty(_ILNext(pidl)));  // Make sure it's not the start page URL.

        LPITEMIDLIST pidlClone = ILClone(pidl);
        if (pidlClone)
        {
            //  we dont want to pass the fragment into URLMON
            //  so we pull it off before calling into GDN.
            //  Note that pidlClone needs to be the pidlTarget
            //  of folder shortcuts or the dislpay name will
            //  be the file system path of the folder shortcut
            //  if at the top level.
            ILRemoveHiddenID(pidlClone, IDLHID_URLFRAGMENT);
            IEGetDisplayName(pidlClone, szPath, SHGDN_FORPARSING);
            hres = S_OK;
            ILFree(pidlClone);
        }
    }

    if (SUCCEEDED(hres) && szPath[0])
        hres = MonikerFromString(szPath, ppmk);

    return hres;
}

HRESULT CDocObjectView::HandleRename(LPCITEMIDLIST pidl)
{
    return E_NOTIMPL;
}

HRESULT CDocObjectView::CreateViewWindow(IShellView  *psvPrev,
                LPCFOLDERSETTINGS lpfs, IShellBrowser  * psb,
                RECT * prcView, HWND  *phWnd)
{
    SV2CVW2_PARAMS cParams;

    cParams.cbSize   = SIZEOF(SV2CVW2_PARAMS);
    cParams.psvPrev  = psvPrev;
    cParams.pfs      = lpfs;
    cParams.psbOwner = psb;
    cParams.prcView  = prcView;
    cParams.pvid     = NULL;

    HRESULT hres = CreateViewWindow2(&cParams);

    *phWnd = cParams.hwndView;
    IOleWindow *pOleWindow;

    // need top level frame available for D&D
    HRESULT hr = IUnknown_QueryService(_psb, SID_STopLevelBrowser, IID_IOleWindow, (void **)&pOleWindow);
    if(SUCCEEDED(hr))
    { 
        ASSERT(pOleWindow);
        pOleWindow->GetWindow(&_hwndParent);
        pOleWindow->Release();
    }
    return(hres);
}

void CDocObjectView::_CompleteDocHostPassing(IShellView *psvPrev, HRESULT hresBinding)
{
    BOOL fPassedSV = FALSE;
    // if there was a previous shell view, see if it was our class.
    if (psvPrev) {
        HRESULT hres;

        CDocObjectView* pdovPrev;
        hres = psvPrev->QueryInterface(CLSID_CDocObjectView, (LPVOID*)&pdovPrev);
        if (SUCCEEDED(hres)) {
            //
            //  Previous shell view is also an instance of CDocObjectView,
            // remember that for optimization later. 
            //
            _fPrevViewIsDocView = TRUE;

            // it was, and we have the same doc host as they do.
            // we've succeeded in taking over, so null them out if we
            // succeeeded in our bind.  null ourselves out if we failed
            if (pdovPrev->_pdoh == _pdoh) {
                CDocObjectHost* pdoh = _pdoh;
                _fStopped = pdovPrev->_fStopped;

                if (SUCCEEDED(hresBinding)) {

                    pdovPrev->_DisconnectHostSink();    // just in case
                    _ConnectHostSink();                 // just in case

                    ATOMICRELEASET(pdovPrev->_pdoh, CDocObjectHost);

                } else {

                    _DisconnectHostSink();              // unhook
                    pdovPrev->_ConnectHostSink();       // kick other guy

                    ATOMICRELEASET(_pdoh, CDocObjectHost);
                }
                fPassedSV = TRUE;
            }
            pdovPrev->Release();
        }
    }

    if (!fPassedSV) {
        if (FAILED(hresBinding)) {
            DestroyViewWindow();
        }
    }
}

BOOL CDocObjectView::_CanUseCache()
{
    // NOTE:  this function is more like _DontHaveToHitTheNet()
    //  the name is legacy from the objectcache.
    if (!_fCanCacheFetched) {
        IServiceProvider *psp;
        IBindStatusCallback *pbsc;

        _fCanCache = TRUE;
        _fCanCacheFetched = TRUE;
        _fIsGet = TRUE;

        _psb->QueryInterface(IID_IServiceProvider, (LPVOID*)&psp);
        if (psp) {
            if (SUCCEEDED(GetTopLevelBindStatusCallback(psp,&pbsc))) {

                BINDINFO binfo;
                ZeroMemory(&binfo, sizeof(BINDINFO));
                binfo.cbSize = sizeof(BINDINFO);

                DWORD grfBINDF = BINDF_ASYNCHRONOUS;

                HRESULT hr = pbsc->GetBindInfo(&grfBINDF, &binfo);
                if (SUCCEEDED(hr)) {
                    if ((grfBINDF) & (BINDF_GETNEWESTVERSION|BINDF_RESYNCHRONIZE )) {

                        _fCanCache = FALSE;

                    }

                    _fIsGet = (binfo.dwBindVerb == BINDVERB_GET);
                    ReleaseBindInfo(&binfo);

                }

                pbsc->Release();
            } 
            // I believe that failing to get the Bindstatuscallback should
            //  not happen since we no longer use the object cache.
            //  and if it does, then we no that we didnt have to repost
            //  or something similar
            psp->Release();
        }

    }

    return _fCanCache;
}


void CDocObjectView::_ConnectHostSink()
{
    if (_pdoh)
    {
        IAdviseSink *pSink;

        if (FAILED(_pdoh->GetAdvise(NULL, NULL, &pSink)))
            pSink = NULL;

        if (pSink != (IAdviseSink *)this)
            _pdoh->SetAdvise(DVASPECT_CONTENT, ADVF_PRIMEFIRST, this);

        if (pSink)
            pSink->Release();
    }
}

void CDocObjectView::_DisconnectHostSink()
{
    IAdviseSink *pSink;

    // paranoia: only blow away the advise sink if it is still us
    if (_pdoh && SUCCEEDED(_pdoh->GetAdvise(NULL, NULL, &pSink)) && pSink)
    {
        if (pSink == (IAdviseSink *)this)
            _pdoh->SetAdvise(0, 0, NULL);

        pSink->Release();
    }

    OnViewChange(DVASPECT_CONTENT, -1);
}

#if 0
BOOL _SameLocations(LPSTR pszLoc1, LPSTR pszLoc2)
{
    // if they're both the same pointer (null)
    // or if they both exist and have the same string, then
    // they are the same location
    if (pszLoc1 == pszLoc2 ||
        (pszLoc1 && pszLoc2 &&
         !lstrcmp(pszLoc1, pszLoc2))) {
        return TRUE;
    }

    return FALSE;
}
#endif

//
//  This function either (1) Create a new CDocObjectHost or (2) reuse it from
// the previous view. If also returns the DisplayName of previous moniker.
//
void CDocObjectView::_CreateDocObjHost(IShellView * psvPrev)
{

    // if there was a previous shell view, see if it was our class.
    if (psvPrev) {
        HRESULT hres;

        CDocObjectView* pdovPrev = NULL;
        hres = psvPrev->QueryInterface(CLSID_CDocObjectView, (LPVOID*)&pdovPrev);
        if (SUCCEEDED(hres)) {

            ASSERT(_psb == pdovPrev->_psb);

            // find out if we should be saving the view state when we close
            // if the one we're coming from says yes, then we'll take over that
            // job instead of them.
            _fSaveViewState = pdovPrev->_fSaveViewState;
            pdovPrev->_fSaveViewState = FALSE;


            //
            //  if this is a local anchor navigation, 
            //  we treat it substantially differently.
            //  we can reuse the DOH and the OLE Object that 
            //  it holds.       - zekel - 31-JUL-97
            //
            //  WARNING:  we should not reuse these objects for any other 
            //  reason than a local anchor (fragment) navigation.
            //  we used to be a lot more lenient about reusing the DOH
            //  but i think this was mostly because of the old object cache.
            //
            //  we check for equal pidls so that we know we are on 
            //  the same URL.  we require that _pszLocation be set.
            //  this is for Netscape compat.  this means that anytime
            //  we get a navigate to ourself it will refresh completely
            //  if there was no fragment.  we also check to make sure that 
            //  the binding does not require us to hit the net for 
            //  this request.
            //

            if (_pidl && pdovPrev->_pidl && pdovPrev->_pdoh &&
                IEILIsEqual(_pidl, pdovPrev->_pidl, TRUE) && _pszLocation &&
                _CanUseCache()) {
                
                IBrowserService *pbs;
                
                if(SUCCEEDED(_psb->QueryInterface(IID_IBrowserService, (LPVOID *)&pbs)))
                {
                    //
                    //  if SetHistoryObject() fails, then that means there is already
                    //  an object there for us to use.  this means that we should
                    //  not treat this as a local anchor navigation.  and 
                    //  we shouldnt reuse the DOH even though the pidl is identical
                    //  
                    if(SUCCEEDED(pbs->SetHistoryObject(pdovPrev->_pdoh->_pole, TRUE)))
                    {
                        TraceMsg(TF_TRAVELLOG, "CDOV::CreateDocObjHost reusing current DOH on local anchor navigate");

                        //
                        //  we cant update in ActivateNow because at that point
                        //  we have already switched the dochost over to the new
                        //  (reused) view.  so we need to update the entry now
                        //
                        ITravelLog *ptl;
                        pbs->GetTravelLog(&ptl);
                        if(ptl)
                        {
                            ptl->UpdateEntry(pbs, TRUE);
                            ptl->Release();
                        }

                        pdovPrev->_DisconnectHostSink();    // we will connect below

                        // same target!  pass the docobj host
                        _pdoh = pdovPrev->_pdoh;
                        _pdoh->AddRef();


                        if ((_pdoh->_dwAppHack & BROWSERFLAG_SUPPORTTOP) && !_pszLocation) {
                            // if there's no location for us to go to, and
                            // we're traversing in the same document,
                            // assume top.
                            _pszLocation = StrDup(TEXT("#top"));
                            _uiCP = CP_ACP;
                        }

                    }
                    pbs->Release();
                }
            }


            //
            // BUGBUG: We should take care of _pibscNC as well
            // to 'chain' the IBindStatusCallback.
            //
            pdovPrev->Release();
        }
    }


    // if we didn't pass the docobj host, create a new one and
    // pass the doc context
    if (!_pdoh)
        _pdoh = new CDocObjectHost();

    if (_pdoh)
        _ConnectHostSink();

}

extern HRESULT TargetQueryService(LPUNKNOWN punk, REFIID riid, void **ppvObj);

HRESULT CDocObjectView::CreateViewWindow2(LPSV2CVW2_PARAMS lpParams)
{
    IShellView *psvPrev     =  lpParams->psvPrev;
    LPCFOLDERSETTINGS lpfs  =  lpParams->pfs;
    IShellBrowser *psb      =  lpParams->psbOwner;
    RECT *prcView           =  lpParams->prcView;
    HWND *phWnd             = &lpParams->hwndView;
    HRESULT hres = S_OK;


    if (_pdoh || !_pidl) 
    {
        *phWnd = NULL;
        ASSERT(0);
        return E_UNEXPECTED;
    }

    _fs = *lpfs;

    ASSERT(_psb==NULL);
    _psb = psb;
    psb->AddRef();

    ASSERT(_pctShellBrowser==NULL);
    _psb->QueryInterface(IID_IOleCommandTarget, (LPVOID*)&_pctShellBrowser);

    //if somebody that is not a ShellBrowser  (like the FileOpenBrowser)
    // tries to use us, we want to block them.  we will fault later on 
    // if we dont have the right stuff.
    if (!_pctShellBrowser)
        return E_UNEXPECTED;

    // prime the cache bit.  this needs to be done while we're *the* guy navigating.
    // otherwise, if we ask later when there's a different pending navigation,
    // we'll get his info
    _CanUseCache();
    _SetLastRefreshTime();

    //  Either create a new CDocObjectHost or reuse it from the previous view
    // and set it in _pdoh.
    _CreateDocObjHost(psvPrev);

    if (!_pdoh || !_pdoh->InitHostWindow(this, psb, prcView))
    {
        ATOMICRELEASE(_psb);
        _pctShellBrowser->Release();
        _pctShellBrowser = NULL;
        return E_OUTOFMEMORY;
    }

    _pdoh->GetWindow(phWnd);

    ASSERT(NULL==_pdho);
    _pdoh->QueryInterface(IID_IDocHostObject, (LPVOID*)&_pdho);

    IMoniker* pmk = NULL;
    BOOL fFileProtocol;
    hres = ::_URLMONMonikerFromPidl(_pidl, &pmk, &fFileProtocol);

    if (SUCCEEDED(hres))
    {
        hres = _pdoh->SetTarget(pmk, _uiCP, _pszLocation, _pidl, psvPrev, fFileProtocol);
        _CompleteDocHostPassing(psvPrev, hres);
        pmk->Release();
    }

    return hres;
}

void CDocObjectView::_GetViewSettings(IEVIEWINFO* pievi)
{
    DWORD dwType, dwSize;

    // REVIEW:  Currently, we have on setting for all docobj class views
    //  (everything hosted by shdocvw).  we may want to subclassify them by clsid
    // of the docobj or maybe special case mshtml...

    dwSize = sizeof(IEVIEWINFO);
    if (SHGetValueGoodBoot(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Internet Explorer\\Main"),
                TEXT("ViewSettings"), &dwType, (PBYTE)pievi, &dwSize) == ERROR_SUCCESS) {

        if (pievi->cbSize != sizeof(IEVIEWINFO)) {
            goto DefaultInfo;
        }

    } else {

DefaultInfo:

        // can't count on 0 init because registry could have read stuff, but
        // of the wrong size (corruption)

        pievi->fToolbar = FALSE;
        pievi->fCoolbar = TRUE;
        pievi->fStatusbar = TRUE;
    }
}

void CDocObjectView::_SaveViewState()
{
    IEVIEWINFO ievi;
    int id;

    // First ask up if it is ok for us to save view state.  If we get the return value of
    //  S_FALSE bail as we were told no.
    if (_pctShellBrowser &&
            (_pctShellBrowser->Exec(&CGID_Explorer, SBCMDID_MAYSAVEVIEWSTATE, 0, NULL, NULL) == S_FALSE))
        return;

    // first load to preserve things we're not going to set
    _GetViewSettings(&ievi);

    ievi.cbSize = sizeof(ievi);

    id = _ShowControl(FCW_STATUS, SBSC_QUERY);
    // bail if it's not supported
    if (id == -1)
        return;
    ievi.fStatusbar = (id == SBSC_SHOW);

    id = _ShowControl(FCW_TOOLBAR, SBSC_QUERY);
    if (id != -1) {
        // this is allowed to fail if toolbar isn't supported (ie30 case)
        ievi.fToolbar = (id == SBSC_SHOW);
    }

    id = _ShowControl(FCW_INTERNETBAR, SBSC_QUERY);
    if (id != -1) {
        // this is allowed to fail if coolbar isn't supported
        ievi.fCoolbar = (id == SBSC_SHOW);
    }

    SHSetValue(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Internet Explorer\\Main"),
                    TEXT("ViewSettings"), REG_BINARY, (const BYTE *)&ievi, sizeof(ievi));
}

int CDocObjectView::_ShowControl(UINT idControl, int idCmd)
{
    VARIANTARG var;

    VariantInit(&var);
    var.vt = VT_I4;
    var.lVal = MAKELONG(idControl, idCmd);

    if (_pctShellBrowser  &&
        SUCCEEDED(_pctShellBrowser->Exec(&CGID_Explorer, SBCMDID_SHOWCONTROL, OLECMDEXECOPT_DODEFAULT,
                                    &var, &var)))
        return var.lVal;


    return -1;
}




HRESULT CDocObjectView::DestroyViewWindow()
{

    ATOMICRELEASE(_pdho);

    if (_pdoh) {
        _DisconnectHostSink();
        if (_fSaveViewState)
            _SaveViewState();
        _pdoh->DestroyHostWindow();
        ATOMICRELEASET(_pdoh, CDocObjectHost);
    }

    ATOMICRELEASE(_pctShellBrowser);

    // Note that we should release _psb at the very end.
    ATOMICRELEASE(_psb);

    return S_OK;
}

HRESULT CDocObjectView::GetCurrentInfo(LPFOLDERSETTINGS lpfs)
{
    *lpfs = _fs;
    return S_OK;
}

HRESULT CDocObjectView::AddPropertySheetPages(DWORD dwReserved,
        LPFNADDPROPSHEETPAGE lpfn, LPARAM lParam)
{
    if (_pdoh)
        return _pdoh->AddPages(lpfn, lParam);

    return E_FAIL;
}

HRESULT CDocObjectView::SaveViewState()
{
    // No viewsate to be saved
    return S_OK;
}

HRESULT CDocObjectView::SelectItem(LPCITEMIDLIST pidlItem, UINT uFlags)
{
    // No item
    return E_FAIL;
}

//
// IShellView::GetItemObject
//
//   For this IShellView object, the only valid uItem is SVGIO_BACKGROUND,
//  which allows the browser to access an interface pointer to the
//  currently active document object.
//
// Notes:
//   The browser should be aware that IShellView::CreateViewWindow might be
//  asynchronous. This method will fail with E_FAIL if the document is not
//  instanciated yet.
//
HRESULT CDocObjectView::GetItemObject(UINT uItem, REFIID riid, LPVOID *ppv)
{
    HRESULT hres = E_INVALIDARG;
    *ppv = NULL;	// assumes error
    switch(uItem)
    {
    case SVGIO_BACKGROUND:
        if (_pdoh)
        {
            if (_pdoh->_pole) 
            {
                hres = _pdoh->_pole->QueryInterface(riid, ppv);
                break;
            }
            else if (_pdoh->_punkPending)
            {
                hres = _pdoh->_punkPending->QueryInterface(riid, ppv);
                break;
            }
        }

    // fall through on the else's
    default:
        hres = E_FAIL;
        break;
    }
    return hres;
}

HRESULT CDocObjectView::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    _fSelfDragging = FALSE;

    //
    // Check if this is a self-dragging or not.
    //
    if (_pdoh && _pdoh->_pmsot) {
        VARIANT var = { 0 };
        HRESULT hresT = _pdoh->_pmsot->Exec(
                    &CGID_ShellDocView, SHDVID_ISDRAGSOURCE, 0, NULL, &var);
        if (SUCCEEDED(hresT) && var.vt==VT_I4 && var.lVal) {
            _fSelfDragging = TRUE;
        }
        VariantClearLazy(&var);
    }

    ASSERT(pdtobj);
    _DragEnter(_hwndParent, ptl, pdtobj);
    _dwDragEffect = CommonDragEnter(pdtobj, grfKeyState, ptl);

    return DragOver(grfKeyState, ptl, pdwEffect);
}

HRESULT CDocObjectView::DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    *pdwEffect &= _dwDragEffect;
    _DragMove(_hwndParent, ptl);
    if (_fSelfDragging && _pdoh && _pdoh->_hwnd) {
        RECT rc;
        GetClientRect(_pdoh->_hwnd, &rc);
        POINT ptMap =  { ptl.x, ptl.y };
        MapWindowPoints(HWND_DESKTOP, _pdoh->_hwnd, &ptMap, 1);
        if (PtInRect(&rc, ptMap)) {
            *pdwEffect = DROPEFFECT_NONE;
        }
    }

    return S_OK;
}

HRESULT CDocObjectView::DragLeave(void)
{
    DAD_DragLeave();
    return S_OK;
}

HRESULT CDocObjectView::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    LPITEMIDLIST pidlTarget;
    HRESULT hr = SHPidlFromDataObject(pdtobj, &pidlTarget, NULL, 0);
    if (SUCCEEDED(hr)) {
        ASSERT(pidlTarget);
        if((!ILIsWeb(pidlTarget) && _pdoh && SHIsRestricted2W(_pdoh->_hwnd, REST_NOFILEURL, NULL, 0)) ||
            (_pdoh && !IEIsLinkSafe(_pdoh->_hwnd, pidlTarget, ILS_NAVIGATE)))
        {
            ILFree(pidlTarget);
            hr = E_ACCESSDENIED;
        }
        else
        {
            DWORD flags = GetKeyState(VK_CONTROL) < 0 ?
            (SBSP_NEWBROWSER | SBSP_ABSOLUTE) :
            (SBSP_SAMEBROWSER | SBSP_ABSOLUTE);
            hr = _psb->BrowseObject(pidlTarget, flags);
            HFRMMSG2(TEXT("::Drop _psb->BrowseObject returned"), hr, 0);
            ILFree(pidlTarget);
        }
    }
    DAD_DragLeave();
    return hr;
}


ULONG CDocObjectView::AddRef()
{
    _cRef++;
    TraceMsg(TF_SHDREF, "CDocObjectView(%x)::AddRef called new _cRef=%d", this, _cRef);
    return _cRef;
}

ULONG CDocObjectView::Release()
{
    _cRef--;
    TraceMsg(TF_SHDREF, "CDocObjectView(%x)::Release called new _cRef=%d", this, _cRef);

    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CDocObjectView::GetView(SHELLVIEWID* pvid, ULONG uView)
{
    return E_NOTIMPL;
}


#ifdef DEBUG
#define _AddRef(psz) { ++_cRef; TraceMsg(TF_SHDREF, "CDocObjectView(%x)::QI(%s) is AddRefing _cRef=%d", this, psz, _cRef); }
#else
#define _AddRef(psz)	++_cRef
#endif

HRESULT CDocObjectView::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    HRESULT hres;

    static const QITAB qit[] = {
        QITABENT(CDocObjectView, IShellView2),
        QITABENTMULTI(CDocObjectView, IShellView, IShellView2),
        QITABENTMULTI(CDocObjectView, IOleWindow, IShellView2),
        QITABENT(CDocObjectView, IDropTarget),
        QITABENT(CDocObjectView, IViewObject),
        QITABENT(CDocObjectView, IAdviseSink),
        QITABENT(CDocObjectView, IOleCommandTarget),
        QITABENT(CDocObjectView, IDocViewSite),
        QITABENT(CDocObjectView, IPrivateOleObject ),
        QITABENT(CDocObjectView, IPersistFolder),
        QITABENTMULTI(CDocObjectView, IPersist, IPersistFolder),
        { 0 },
    };

    hres = QISearch(this, qit, riid, ppvObj);

    if (S_OK != hres)
    {
        if (IsEqualIID(riid, CLSID_CDocObjectView))
        {
            *ppvObj = (void*)this;
            _AddRef(TEXT("CLSID_CDocObjectView"));
            return S_OK;
        }
    }

    return hres;
}



/// ***** IViewObject ******

HRESULT CDocObjectView::GetColorSet(DWORD dwAspect, LONG lindex,
    void *pvAspect, DVTARGETDEVICE *ptd, HDC hicTargetDev,
    LOGPALETTE **ppColorSet)
{
    if (_pdoh)
    {
        return _pdoh->GetColorSet(dwAspect, lindex, pvAspect, ptd,
            hicTargetDev, ppColorSet);
    }

    if (ppColorSet)
        *ppColorSet = NULL;

    return S_FALSE;
}

HRESULT CDocObjectView::Freeze(DWORD, LONG, void *, DWORD *pdwFreeze)
{
    return E_NOTIMPL;
}

HRESULT CDocObjectView::Unfreeze(DWORD)
{
    return E_NOTIMPL;
}

HRESULT CDocObjectView::SetAdvise(DWORD dwAspect, DWORD advf,
    IAdviseSink *pSink)
{
    if (dwAspect != DVASPECT_CONTENT)
        return DV_E_DVASPECT;

    if (advf & ~(ADVF_PRIMEFIRST | ADVF_ONLYONCE))
        return E_INVALIDARG;

    if (pSink != _padvise)
    {
        ATOMICRELEASE(_padvise);

        _padvise = pSink;

        if (_padvise)
            _padvise->AddRef();
    }

    if (_padvise)
    {
        _advise_aspect = dwAspect;
        _advise_advf = advf;

        if (advf & ADVF_PRIMEFIRST)
            OnViewChange(dwAspect, -1);
    }
    else
        _advise_aspect = _advise_advf = 0;

    return S_OK;
}

HRESULT CDocObjectView::GetAdvise(DWORD *pdwAspect, DWORD *padvf,
    IAdviseSink **ppSink)
{
    if (pdwAspect)
        *pdwAspect = _advise_aspect;

    if (padvf)
        *padvf = _advise_advf;

    if (ppSink)
    {
        if (_padvise)
            _padvise->AddRef();

        *ppSink = _padvise;
    }

    return S_OK;
}

HRESULT CDocObjectView::Draw(DWORD dwDrawAspect, LONG lindex, void *pvAspect,
    DVTARGETDEVICE *ptd, HDC hicTargetDev, HDC hdcDraw,
    const RECTL *lprcBounds, const RECTL *lprcWBounds,
    BOOL (*pfnContinue)(ULONG_PTR), ULONG_PTR dwContinue)
{
    if (_pdoh) {
        return _pdoh->Draw(dwDrawAspect, lindex, pvAspect, ptd, hicTargetDev,
            hdcDraw, lprcBounds, lprcWBounds, pfnContinue, dwContinue);
    }

    return OLE_E_BLANK;
}

// IAdviseSink
void CDocObjectView::OnDataChange(FORMATETC *, STGMEDIUM *)
{
}

void CDocObjectView::OnViewChange(DWORD dwAspect, LONG lindex)
{
    dwAspect &= _advise_aspect;

    if (dwAspect && _padvise)
    {
        IAdviseSink *pSink = _padvise;
        IUnknown *punkRelease;

        if (_advise_advf & ADVF_ONLYONCE)
        {
            punkRelease = pSink;
            _padvise = NULL;
            _advise_aspect = _advise_advf = 0;
        }
        else
            punkRelease = NULL;

        pSink->OnViewChange(dwAspect, lindex);

        if (punkRelease)
            punkRelease->Release();
    }
}

void CDocObjectView::OnRename(IMoniker *)
{
}

void CDocObjectView::OnSave()
{
}

void CDocObjectView::OnClose()
{
    //
    // the doc object host went away so tell anybody above what happened
    //
    OnViewChange(_advise_aspect, -1);
}


HRESULT CDocObjectView::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    HRESULT hres = OLECMDERR_E_UNKNOWNGROUP;

    if (_pdho && _pdoh)
        hres = _pdho->QueryStatusDown(pguidCmdGroup, cCmds, rgCmds, pcmdtext);

    return hres;
}

HRESULT CDocObjectView::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    HRESULT hres = OLECMDERR_E_UNKNOWNGROUP;

    if (!pguidCmdGroup) {
        switch (nCmdID) {
        case OLECMDID_STOP:
            // the user stopped navigation
            if (nCmdexecopt == OLECMDEXECOPT_DODEFAULT) {
                _fStopped = TRUE;
            }
            break;

        case OLECMDID_REFRESH:
            _fStopped = FALSE;
            _SetLastRefreshTime();
            break;
        }
    }

    // only forward on if we aren't 'stolen'.
    // BUGBUG ie4: clean this up to steal _pdho along w/ _pdoh.
    if (_pdho && _pdoh)
        hres = _pdho->ExecDown(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);

    // REVIEW: if _pdoh->ExecDown fails && pguidCmdGroup==NULL && nCmdID is
    //            OLECMDID_STOP or OLECMDID_REFRESH, then we are lying
    //            by returning a failure error code.

    return hres;
}

HRESULT CDocObjectView::OnSetTitle(VARIANTARG *pvTitle)
{
    return E_NOTIMPL;
}

HRESULT CDocObjectView::SetExtent( DWORD dwDrawAspect, SIZEL *psizel)
{
    if ( _pdoh && _pdoh->GetOleObject() )
    {
        return _pdoh->GetOleObject()->SetExtent( dwDrawAspect, psizel );
    }

    return E_NOTIMPL;
}

HRESULT CDocObjectView::GetExtent( DWORD dwDrawAspect, SIZEL *psizel)
{
    if ( _pdoh && _pdoh->GetOleObject() )
    {
        return _pdoh->GetOleObject()->GetExtent( dwDrawAspect, psizel );
    }

    return E_NOTIMPL;
}

HRESULT CDocObjectView::GetClassID(CLSID *pclsid)
{
    if (pclsid)
    {
        *pclsid = CLSID_CDocObjectView;
        return S_OK;
    }
    return E_INVALIDARG;
}

HRESULT CDocObjectView::Initialize(LPCITEMIDLIST pidl)
{
    HRESULT hres = E_OUTOFMEMORY;
    
    LPITEMIDLIST pidlClone = ILClone(pidl);
    if (pidlClone)
    {
        IShellFolder* psf;
        if (SUCCEEDED(IEBindToObject(_pidl, &psf)))
        {
            ILFree(_pidl);
            ATOMICRELEASE(_psf);

            _pidl = pidlClone;
            _psf = psf;
            
            hres = S_OK;
        }
        else
        {
            ILFree(pidlClone);
        }
    }

    return hres;
}

