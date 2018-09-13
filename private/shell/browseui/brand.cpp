#include "priv.h"
#include "sccls.h"
#include "itbar.h"
#include "itbdrop.h"
#include "brand.h"
#include "theater.h"
#include "resource.h"

#include "mluisupp.h"

typedef struct {
    HPALETTE    hpal;
    HBITMAP     hbm;

    int         cyBrand;
    int         cxBrandExtent;
    int         cyBrandExtent;
    int         cyBrandLeadIn;

    COLORREF    clrBkStat;
    COLORREF    clrBkAnim;

    LPTSTR      pszBitmap;
    LPTSTR      pszStaticBitmap;
} BRANDCONTEXT;    

class CBrandBand :  public CToolBand,
                    public IWinEventHandler,
                    public IDispatch
{
public:
    // IUnknown
    virtual STDMETHODIMP_(ULONG) AddRef(void)   { return CToolBand::AddRef(); }
    virtual STDMETHODIMP_(ULONG) Release(void)  { return CToolBand::Release(); }
    virtual STDMETHODIMP         QueryInterface(REFIID riid, LPVOID * ppvObj);

    // IObjectWithSite
    virtual STDMETHODIMP SetSite(IUnknown* punkSite);
    
    // *** IDeskBand methods ***
    virtual STDMETHODIMP GetBandInfo(DWORD dwBandID, DWORD fViewMode, DESKBANDINFO* pdbi);
    // IOleCommandTarget
    virtual STDMETHODIMP Exec(const GUID *pguidCmdGroup,
                              DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn,
                              VARIANTARG *pvarargOut);
    
    // IPersist
    virtual STDMETHODIMP GetClassID(CLSID *pClassID);

    // IPersistStream
    virtual STDMETHODIMP Load(IStream *pStm);
    virtual STDMETHODIMP Save(IStream *pStm, BOOL fClearDirty);

    // IWinEventHandler
    virtual STDMETHODIMP OnWinEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plre);
    virtual STDMETHODIMP IsWindowOwner(HWND hwnd);

    // IDispatch
    virtual STDMETHODIMP GetTypeInfoCount(UINT *pctinfo){return(E_NOTIMPL);}
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo,LCID lcid,ITypeInfo **pptinfo){return(E_NOTIMPL);}
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid,OLECHAR **rgszNames,UINT cNames, LCID lcid, DISPID * rgdispid){return(E_NOTIMPL);}
    virtual STDMETHODIMP Invoke(DISPID dispidMember,REFIID riid,LCID lcid,WORD wFlags,
                  DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo,UINT * puArgErr);
    
protected:    
    CBrandBand();
    virtual ~CBrandBand();

    friend HRESULT CBrandBand_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);    // for ctor

    BITBOOL        _fVertical:1;
    BITBOOL        _fAnimating:1;
    BITBOOL        _fMinAlways:1;
    BITBOOL        _fTheater :1;

    BOOL _fShellView;
    
    BRANDCONTEXT *_pbc;
    int         _yOrg;
    
    static HDC          s_hdc;
    static BRANDCONTEXT s_bcWebSmall;     // BrandContext for the small web view bitmap
    static BRANDCONTEXT s_bcWebLarge;     // BrandContext for the large web view bitmap
    static BRANDCONTEXT s_bcWebMicro;     // BrandContext for the micro web view bitmap
    static BRANDCONTEXT s_bcShellSmall;   // BrandContext for the small shell view bitmap
    static BRANDCONTEXT s_bcShellLarge;   // BrandContext for the large shell view bitmap
    

    IWebBrowserApp *    _pdie;          // Used when Navigating a Browser Window with a URL String
    IBrowserService *   _pbs;           // Only valid when we are in a Browser Windows Toolbar. (Not Toolband)
    IWebBrowser2 *      _pwb2;          // Only valid when we are a Toolband (not toolbar).
    DWORD               _dwcpCookie;    // ConnectionPoint cookie for DWebBrowserEvents from the Browser Window.

    // Helper functions
    void _UpdateCompressedSize();
    HRESULT _CreateBrandBand();
    HRESULT _LoadBrandingBitmap();
    void    _DrawBranding(HDC hdc);
    int     _GetLinksExtent();
    void    _OnTimer(WPARAM id);
    static void _InitGlobals();
    static void _InitBrandContexts();
    static void _InitBrandContext(BRANDCONTEXT* pbc, LPCTSTR pszBrandLeadIn, LPCTSTR pszBrandHeight,
        LPCTSTR pszBrandBitmap, LPCTSTR pszBitmap, int idBrandBitmap);

    HRESULT _ConnectToBrwsrWnd(IUnknown* punk);        
    HRESULT _ConnectToBrwsrConnectionPoint(BOOL fConnect);

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    friend void CBrandBand_CleanUp();
    friend void Brand_InitBrandContexts();

    static void _GetBrandContextHeight(BRANDCONTEXT* pbc, LPCTSTR pszBrandLeadIn, LPCTSTR pszBrandHeight, 
        LPCTSTR pszBrandBitmap, LPCTSTR pszBitmap, int idBrandBitmap);

private:
};


#define SUPERCLASS  CToolBand

#define BM_BANDINFOCHANGED  (WM_USER + 1)

#ifdef DEBUG
extern unsigned long DbStreamTell(IStream *pstm);
#else
#define DbStreamTell(pstm)      ((ULONG) 0)
#endif
extern HRESULT VariantClearLazy(VARIANTARG *pvarg);

#define ANIMATION_TIMER         5678

#define MICROBITMAPID()     (IDB_IEMICROBRAND)
#define SMALLBITMAPID()     (IDB_IESMBRAND)
#define LARGEBITMAPID()     (IDB_IEBRAND)


BRANDCONTEXT CBrandBand::s_bcWebMicro   = {NULL};   // BrandContext for the micro web view bitmap
BRANDCONTEXT CBrandBand::s_bcWebSmall   = {NULL};   // BrandContext for the small web view bitmap
BRANDCONTEXT CBrandBand::s_bcWebLarge   = {NULL};   // BrandContext for the large web view bitmap
BRANDCONTEXT CBrandBand::s_bcShellSmall = {NULL};   // BrandContext for the small shell view bitmap
BRANDCONTEXT CBrandBand::s_bcShellLarge = {NULL};   // BrandContext for the large shell view bitmap


// The heights of the bitmaps (each frame!) stored is this module's resources

// ** NOTE **
// If you change the animated brands that are stored in browseui:
// MAKE SURE THESE HEIGHTS are correct!!
// ** - dsheldon - **

#define BRANDHEIGHT_WEBLARGE    38
#define BRANDHEIGHT_WEBSMALL    26
#define BRANDHEIGHT_WEBMICRO    22

HDC CBrandBand::s_hdc = NULL;
BOOL g_fUseMicroBrand = TRUE;
UINT g_cySmBrand = 0;
static const TCHAR szRegKeyIE20[]           = TEXT("SOFTWARE\\Microsoft\\Internet Explorer\\Main");

static const TCHAR szValueLargeBitmap[]     = TEXT("BigBitmap");
static const TCHAR szValueSmallBitmap[]     = TEXT("SmallBitmap");
static const TCHAR szValueBrandBitmap[]     = TEXT("BrandBitmap");
static const TCHAR szValueBrandHeight[]     = TEXT("BrandHeight");
static const TCHAR szValueBrandLeadIn[]     = TEXT("BrandLeadIn");
static const TCHAR szValueSmBrandBitmap[]   = TEXT("SmBrandBitmap");
static const TCHAR szValueSmBrandHeight[]   = TEXT("SmBrandHeight");
static const TCHAR szValueSmBrandLeadIn[]   = TEXT("SmBrandLeadIn");

static const TCHAR szValueSHLargeBitmap[]     = TEXT("SHBigBitmap");
static const TCHAR szValueSHSmallBitmap[]     = TEXT("SHSmallBitmap");
static const TCHAR szValueSHBrandBitmap[]     = TEXT("SHBrandBitmap");
static const TCHAR szValueSHBrandHeight[]     = TEXT("SHBrandHeight");
static const TCHAR szValueSHBrandLeadIn[]     = TEXT("SHBrandLeadIn");
static const TCHAR szValueSHSmBrandBitmap[]   = TEXT("SHSmBrandBitmap");
static const TCHAR szValueSHSmBrandHeight[]   = TEXT("SHSmBrandHeight");
static const TCHAR szValueSHSmBrandLeadIn[]   = TEXT("SHSmBrandLeadIn");

static const TCHAR szRegKeyCoolbar[]        = TEXT("Software\\Microsoft\\Internet Explorer\\Toolbar");
    // NOTE: szRegKeyCoolbar is duplicated from itbar.cpp!

void CBrandBand_CleanUp()
{
    if (CBrandBand::s_hdc)
    {
        HDC     hdcT;
        HBITMAP hbmT, * pbmp = NULL;

        // pick out any old bitmap to flush dc with
        if (CBrandBand::s_bcWebLarge.hbm)
            pbmp = &CBrandBand::s_bcWebLarge.hbm;
        else if (CBrandBand::s_bcWebSmall.hbm)
            pbmp = &CBrandBand::s_bcWebSmall.hbm;        

        // We need to get rid of the branding bitmap from the s_hdc
        // before we delete it else we leak. Do this the hard way since
        // we don't have a stock bitmap available to us.
        if (pbmp)
        {
            hdcT = CreateCompatibleDC(NULL);
            hbmT = (HBITMAP)SelectObject(hdcT, *pbmp);
            SelectObject(CBrandBand::s_hdc, hbmT);
            SelectObject(hdcT, hbmT);
            DeleteObject(hdcT);
        }
        
        DeleteDC(CBrandBand::s_hdc);
    }

    // no palette to delete as we use the global one..
    // delete the shared palette
    
    if (CBrandBand::s_bcWebSmall.hbm)
        DeleteObject(CBrandBand::s_bcWebSmall.hbm);

    if (CBrandBand::s_bcWebLarge.hbm)
        DeleteObject(CBrandBand::s_bcWebLarge.hbm);

    if (!g_fUseMicroBrand) {
        if (CBrandBand::s_bcShellSmall.hbm)
            DeleteObject(CBrandBand::s_bcShellSmall.hbm);

        if (CBrandBand::s_bcShellLarge.hbm)
            DeleteObject(CBrandBand::s_bcShellLarge.hbm);
    } else {
        if (CBrandBand::s_bcWebMicro.hbm)
            DeleteObject(CBrandBand::s_bcWebMicro.hbm);
    }
}

/////////////////////////////////////////////////////////////////////////////
// CBrandBand
/////////////////////////////////////////////////////////////////////////////
HRESULT CBrandBand_CreateInstance( IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    // Aggregation checking is handled in class factory

    CBrandBand * p = new CBrandBand();
    if (p != NULL) 
    {
        *ppunk = SAFECAST(p, IDeskBand *);
        return NOERROR;
    }

    return E_OUTOFMEMORY;
}

CBrandBand::CBrandBand() : SUPERCLASS()
{
    ASSERT(_fAnimating == FALSE);
    _pbc = &s_bcShellLarge;
}

CBrandBand::~CBrandBand()
{
    ASSERT(!_pdie || !_pwb2 || !_pbs)
}

// IUnknown::QueryInterface
HRESULT CBrandBand::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IWinEventHandler))
    {
        *ppvObj = SAFECAST(this, IWinEventHandler*);
    }
    else if (IsEqualIID(riid, IID_IPersistStream))
    {
        *ppvObj = SAFECAST(this, IPersistStream*);
    }
    else if (IsEqualIID(riid, IID_IDispatch))
    {
        *ppvObj = SAFECAST(this, IDispatch*);
    }
    else
    {
        return SUPERCLASS::QueryInterface(riid, ppvObj);
    }

    AddRef();
    return S_OK;
}

// IDockingWindow::SetSite
HRESULT CBrandBand::SetSite(IUnknown * punkSite)
{
    HRESULT hr;

    if (_pdie || _pwb2 || _pbs)
        _ConnectToBrwsrWnd(NULL);    // On-connect from Browser Window.

    hr = SUPERCLASS::SetSite(punkSite);

    if (punkSite)
    {
        hr = _CreateBrandBand();

        // This call will fail if the host doesn't have a Browser Window.
        _ConnectToBrwsrWnd(punkSite);
    }

    return hr;
}

// IDeskBand::GetBandInfo
HRESULT CBrandBand::GetBandInfo(DWORD dwBandID, DWORD fViewMode, DESKBANDINFO* pdbi) 
{
    _dwBandID = dwBandID;

    _fVertical = ((fViewMode & DBIF_VIEWMODE_VERTICAL) != 0);

    _LoadBrandingBitmap();
    pdbi->dwModeFlags = DBIMF_FIXEDBMP;
    if (!_fMinAlways)
        pdbi->dwModeFlags |= DBIMF_VARIABLEHEIGHT;

    int cxWidth = _fTheater ? CX_FLOATERSHOWN : _GetLinksExtent();
    
    pdbi->ptMinSize.x = max(s_bcWebSmall.cxBrandExtent, max(s_bcShellSmall.cxBrandExtent, cxWidth));
    pdbi->ptMaxSize.x = max(s_bcWebLarge.cxBrandExtent, max(s_bcShellLarge.cxBrandExtent, cxWidth));
    
    pdbi->ptMaxSize.y = max(s_bcWebLarge.cyBrand, s_bcShellLarge.cyBrand);
    
    if (g_fUseMicroBrand)
        pdbi->ptMinSize.y = s_bcWebMicro.cyBrand;
    else
        pdbi->ptMinSize.y = max(s_bcWebSmall.cyBrand, s_bcShellSmall.cyBrand);
    
    pdbi->ptIntegral.y = -1;


    return S_OK;
}

// IWinEventHandler::OnWinEvent
HRESULT CBrandBand::OnWinEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres)
{
    *plres = 0;
    
    switch (uMsg)
    {
    case WM_WININICHANGE:
        if (SHIsExplorerIniChange(wParam, lParam))
        {
            _InitBrandContexts();
            InvalidateRect(_hwnd, NULL, TRUE);
        }
        *plres = SendMessage(_hwnd, uMsg, wParam, lParam);
        break;
    }
    
    return S_OK;
} 

// IWinEventHandler::IsWindowOwner
HRESULT CBrandBand::IsWindowOwner(HWND hwnd)
{
    if (hwnd == _hwnd)
        return S_OK;
    
    return S_FALSE;
}

// IPersistStream::GetClassID
HRESULT CBrandBand::GetClassID(CLSID * pClassID)
{
    *pClassID = CLSID_BrandBand;
    return S_OK;
}

// IPersistStream::Load
HRESULT CBrandBand::Load(IStream *pstm)
{
    return S_OK;
}

// IPersistStream::Load
HRESULT CBrandBand::Save(IStream *pstm, BOOL fClearDirty)
{
    return S_OK;
}

#define ANIMATION_PERIOD    30

// IDispatch::Invoke
HRESULT CBrandBand::Invoke
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

    switch (dispidMember)
    {
        case DISPID_DOWNLOADBEGIN:
        {
            // The timer does a invalidate of _hwnd which in cases
            // of no toolbar showing caused the entire desktop to
            // repaint and repaint and...
            if (_hwnd) {
                SetTimer(_hwnd, ANIMATION_TIMER, ANIMATION_PERIOD, NULL);
            }
            _yOrg = 0;
            _fAnimating = TRUE;
            
            IUnknown_Exec(_punkSite, &CGID_Theater, THID_ACTIVATE, 0, NULL, NULL);
            break;
        }

        case DISPID_DOWNLOADCOMPLETE:
        {

            _fAnimating = FALSE;

            KillTimer(_hwnd, ANIMATION_TIMER);
            InvalidateRect(_hwnd, NULL, FALSE);
            UpdateWindow(_hwnd);
            IUnknown_Exec(_punkSite, &CGID_Theater, THID_DEACTIVATE, 0, NULL, NULL);

            break;
        }

        default:
            return E_INVALIDARG;
    }
    
    return S_OK;
}

void CBrandBand::_InitGlobals()
{
    if (!s_hdc) {
        ENTERCRITICAL;
        if (!s_hdc)
        {
            s_hdc = CreateCompatibleDC(NULL);
            if (GetDeviceCaps(s_hdc, RASTERCAPS) & RC_PALETTE)
            {
                // share the global palette ....
                ASSERT( g_hpalHalftone );
                s_bcWebMicro.hpal = g_hpalHalftone;
                s_bcWebSmall.hpal = s_bcShellSmall.hpal = g_hpalHalftone;
                s_bcWebLarge.hpal = s_bcShellLarge.hpal = g_hpalHalftone;
            }   
        }        
        LEAVECRITICAL;
    }
}

HRESULT CBrandBand::_CreateBrandBand()
{
    HRESULT hr;

    ASSERT(_hwndParent);        // Call us after SetSite()
    if (!_hwndParent)
    {
        // The caller hasn't called SetSite(), so we can't
        // create our window because we can't find out our parent's
        // HWND.
        return E_FAIL;
    }

    // create branding window
    _hwnd = SHCreateWorkerWindow(WndProc, _hwndParent, 0, WS_CHILD, NULL, this);
    if (_hwnd)
    {
        _InitGlobals();
        hr = S_OK;
    }
    else
    {
        hr = E_OUTOFMEMORY;
        TraceMsg(TF_ERROR, "Could not create Brand Band");
    }

    return hr;
}

void Brand_InitBrandContexts()
{
    CBrandBand::_InitGlobals();
    CBrandBand::_InitBrandContexts();
}

void CBrandBand::_InitBrandContexts()
{
    // note: these calls set g_fUseMicroBrand
    _GetBrandContextHeight(&s_bcWebSmall, szValueSmBrandLeadIn, szValueSmBrandHeight,
            szValueSmBrandBitmap, szValueSmallBitmap, SMALLBITMAPID());
    _GetBrandContextHeight(&s_bcWebLarge, szValueBrandLeadIn, szValueBrandHeight,
            szValueBrandBitmap, szValueLargeBitmap, LARGEBITMAPID());

    // if no third party brands found
    if (g_fUseMicroBrand) {
        // init micro brand
        _GetBrandContextHeight(&s_bcWebMicro, NULL, NULL,
            NULL, NULL, MICROBITMAPID());
    } else {
        // init shell brands
        _GetBrandContextHeight(&s_bcShellSmall, szValueSHSmBrandLeadIn, szValueSHSmBrandHeight,
                szValueSHSmBrandBitmap, szValueSHSmallBitmap, SMALLBITMAPID());
        _GetBrandContextHeight(&s_bcShellLarge, szValueSHBrandLeadIn, szValueSHBrandHeight,
                szValueSHBrandBitmap, szValueSHLargeBitmap, LARGEBITMAPID());
    }
}


/****************************************************************************
CBrandBand::_GetBrandContextHeight

  Sets the cyBrand member of the supplied brand context. This function
  uses the height information stored in the registry if it is available.
  If an alternate source for bitmaps is found and the height information
  is available in the registry, it is assumed we will not be using our
  micro brand (g_fUseMicroBrand = FALSE).

  Otherwise, it is assumed that no custom bitmaps are available and
  the cyBrand will be set to constant values representing the height
  of our standard branding.

  Note that if it appears that there are custom bitmaps available but the
  height cannot be read, we will attempt to read the custom bitmaps and
  determine the height that way (by delegating to _InitBrandContext)
****************************************************************************/
void CBrandBand::_GetBrandContextHeight(BRANDCONTEXT* pbc, LPCTSTR pszBrandLeadIn, LPCTSTR pszBrandHeight, 
    LPCTSTR pszBrandBitmap, LPCTSTR pszBitmap, int idBrandBitmap)
{
    HKEY hKey;
    DWORD cbData;
    DWORD dwType;
    BOOL fThirdPartyBitmap = FALSE;
    TCHAR szScratch[MAX_PATH];
    szScratch[0] = 0;

    // try to determine if there is a third party bitmap available for the specified
    // brand... Check if the anitmated bitmap exists
    if (pszBrandBitmap && ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, szRegKeyCoolbar, 0, KEY_QUERY_VALUE, &hKey))
    {
        // See if an alternate file was specified for the animated bmp
        cbData = sizeof(szScratch);
        if ((ERROR_SUCCESS == SHQueryValueEx(hKey, pszBrandBitmap, NULL, &dwType,
            (LPBYTE)szScratch, &cbData)))
        {
            if (szScratch[0] != 0)
                fThirdPartyBitmap = TRUE;
        }


        // It appears there are third party bitmaps

        // try to find the height of the animated bitmap
        if (pszBrandHeight && fThirdPartyBitmap)
        {
            cbData = sizeof(pbc->cyBrand);

            if (ERROR_SUCCESS == SHQueryValueEx(hKey, pszBrandHeight, NULL, &dwType,
                (LPBYTE)&pbc->cyBrand, &cbData))
            {
                // Third party brands probably exist
                g_fUseMicroBrand = FALSE;
            }
            else
            {
                // In this case, we know there should be 3rd party bitmaps but no
                // height was specified in the registry. We have to bite the bullet
                // and load the bitmaps now: Delegate to _InitBrandContext()
                _InitBrandContext(pbc, pszBrandLeadIn, pszBrandHeight, 
                    pszBrandBitmap, pszBitmap, idBrandBitmap);
            }
        }

        RegCloseKey(hKey);
    }

    if (!fThirdPartyBitmap && pszBitmap && 
        ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, szRegKeyCoolbar, 0, KEY_QUERY_VALUE, &hKey))
    {
        // See if an alternate file was specified for the static bmp
        cbData = sizeof(szScratch);
        if ((ERROR_SUCCESS == SHQueryValueEx(hKey, pszBitmap, NULL, &dwType,
            (LPBYTE)szScratch, &cbData)))
        {
            if (szScratch[0] != 0)
            {
                // In this case, we know there is a 3rd party static bitmap but no
                // animated bitmap was specified in the registry. We have to bite the bullet
                // and load the bitmaps now: Delegate to _InitBrandContext()
                fThirdPartyBitmap = TRUE;
                _InitBrandContext(pbc, pszBrandLeadIn, pszBrandHeight, 
                    pszBrandBitmap, pszBitmap, idBrandBitmap);
            }
        }

        RegCloseKey(hKey);
    }

    // If we didn't find any third party bitmaps, we need to set it the height 
    // to the size of the bitmaps in this module's resources
    if (!fThirdPartyBitmap)
    {
        // Set the height based on which bitmap ID is requested
        switch (idBrandBitmap)
        {
        case IDB_IEMICROBRAND:
            pbc->cyBrand = BRANDHEIGHT_WEBMICRO;
            break;
        case IDB_IESMBRAND:
            pbc->cyBrand = BRANDHEIGHT_WEBSMALL;
            break;
        case IDB_IEBRAND:
            pbc->cyBrand = BRANDHEIGHT_WEBLARGE;
            break;
        default:
            // bad ID passed in!
            ASSERT(FALSE);
        }
    }
}

void CBrandBand::_InitBrandContext(BRANDCONTEXT* pbc, LPCTSTR pszBrandLeadIn, LPCTSTR pszBrandHeight, 
    LPCTSTR pszBrandBitmap, LPCTSTR pszBitmap, int idBrandBitmap)
{
    ENTERCRITICAL;

    HKEY        hKey = NULL;
    DWORD       dwType = 0;
    TCHAR       szScratch[MAX_PATH];
    DWORD       dwcbData;

    BOOL        fBitmapInvalid = !pbc->hbm;
    LPTSTR      pszNewBitmap = NULL;
    LPTSTR      pszOldBitmap = pbc->pszBitmap;
    HBITMAP     hbmp = NULL;
    BOOL        fExternalAnimatedBitmap = FALSE;

    // process animated brand bitmap
    
    // see if the location spec for the bitmap has been changed    
    if (pszBrandBitmap && ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, szRegKeyCoolbar, 0, KEY_QUERY_VALUE, &hKey))
    {
        dwcbData = SIZEOF(szScratch);
        if ((ERROR_SUCCESS == SHQueryValueEx(hKey, pszBrandBitmap, NULL, &dwType,
            (LPBYTE)szScratch, &dwcbData)))
        {               
            pszNewBitmap = szScratch;
            fExternalAnimatedBitmap = TRUE;
        }
    }

    if (!(pszNewBitmap == pszOldBitmap || (pszNewBitmap && pszOldBitmap && !lstrcmpi(pszNewBitmap, pszOldBitmap))))
        fBitmapInvalid = TRUE;

    if (fBitmapInvalid) {
        Str_SetPtr(&pbc->pszBitmap, pszNewBitmap);

        if (pszNewBitmap) {
            if (pszNewBitmap[0]) {    // not empty string

                hbmp = (HBITMAP) LoadImage(NULL, szScratch, IMAGE_BITMAP, 0, 0, 
                                           LR_DEFAULTSIZE | LR_CREATEDIBSECTION | LR_LOADFROMFILE);
            }
        }

        if (!hbmp) 
        {
            if (hKey != NULL)
            {
                RegCloseKey(hKey); 
                hKey = NULL;
            }
            hbmp = (HBITMAP) LoadImage(HINST_THISDLL, MAKEINTRESOURCE(idBrandBitmap), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_CREATEDIBSECTION);
        } else
            g_fUseMicroBrand = FALSE;

        if (!hbmp) goto ErrorDone;

        if (pbc->hbm) DeleteObject(pbc->hbm);
        pbc->hbm = hbmp;

        // set the background to be the first pixel
        SelectObject(s_hdc, pbc->hbm);
        pbc->clrBkAnim = GetPixel(s_hdc, 0, 0);

        DIBSECTION  dib;
        GetObject(pbc->hbm, sizeof(DIBSECTION), &dib);
        pbc->cxBrandExtent = dib.dsBm.bmWidth;
        pbc->cyBrandExtent = dib.dsBm.bmHeight;

        dwcbData = sizeof(DWORD);

        // BUGBUG:: hkey is not setup when the second instance calls in
        if (!hKey || (ERROR_SUCCESS != SHQueryValueEx(hKey, pszBrandHeight, NULL, &dwType,
            (LPBYTE)&pbc->cyBrand, &dwcbData)))
            pbc->cyBrand = pbc->cxBrandExtent;

#define EXTERNAL_IMAGE_OFFSET   4
#define INTERNAL_IMAGE_OFFSET   0

        if (!hKey || (ERROR_SUCCESS != SHQueryValueEx(hKey, pszBrandLeadIn, NULL, &dwType,
            (LPBYTE)&pbc->cyBrandLeadIn, &dwcbData)))
        {
#ifndef UNIX
            if (fExternalAnimatedBitmap)
                // use old 4-image offset for back compat
                pbc->cyBrandLeadIn = EXTERNAL_IMAGE_OFFSET;
            else                
                pbc->cyBrandLeadIn = INTERNAL_IMAGE_OFFSET;
#else
            // IEUNIX : We use a different branding bitmap.   
            pbc->cyBrandLeadIn = EXTERNAL_IMAGE_OFFSET;
#endif
        }

        pbc->cyBrandLeadIn *= pbc->cyBrand;
    }

    if (hKey)
        RegCloseKey(hKey);

    // process the static bitmap

    pszNewBitmap = NULL;
    pszOldBitmap = pbc->pszStaticBitmap;
    hbmp = NULL;

    // see if the location spec for the bitmap has been changed
    dwcbData = SIZEOF(szScratch);
    if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, szRegKeyCoolbar, pszBitmap, &dwType, szScratch, &dwcbData))
    {
        pszNewBitmap = szScratch;
    }

    if (!(pszNewBitmap == pszOldBitmap || (pszNewBitmap && pszOldBitmap && !lstrcmpi(pszNewBitmap, pszOldBitmap))))
        fBitmapInvalid = TRUE;

    if (fBitmapInvalid) {
        Str_SetPtr(&pbc->pszStaticBitmap, pszNewBitmap);

        if (pszNewBitmap) {
            if (pszNewBitmap[0]) {    // not empty string

                hbmp = (HBITMAP) LoadImage(NULL, szScratch, IMAGE_BITMAP, 0, 0, 
                                           LR_DEFAULTSIZE | LR_CREATEDIBSECTION | LR_LOADFROMFILE);
            }
        }

        if (hbmp) {

            DIBSECTION  dib;

            HDC hdcOld = CreateCompatibleDC(s_hdc);

            if (hdcOld)
            {
                SelectObject(s_hdc, pbc->hbm);
                GetObject(hbmp, sizeof(DIBSECTION), &dib);
                SelectObject(hdcOld, hbmp);
                // Set background to color of first pixel
                pbc->clrBkStat = GetPixel(hdcOld, 0, 0);
                StretchBlt(s_hdc, 0, 0, pbc->cxBrandExtent, pbc->cyBrand, hdcOld, 0, 0,
                           dib.dsBm.bmWidth, dib.dsBm.bmHeight, SRCCOPY);
                DeleteDC(hdcOld);
            }

            DeleteObject(hbmp);
            
            // If there was a custom brand, we can't use our micro brand
            g_fUseMicroBrand = FALSE;
        }        

        if (pbc == &s_bcShellSmall)
            g_cySmBrand = pbc->cyBrand;       
    }

ErrorDone:
    LEAVECRITICAL;
}

void CBrandBand::_UpdateCompressedSize()
{
    RECT rc;
    BOOL fCompressed = FALSE;
    BRANDCONTEXT *pbcOld = _pbc;

    GetClientRect(_hwnd, &rc);
    if (RECTHEIGHT(rc) < max(s_bcWebLarge.cyBrand, s_bcShellLarge.cyBrand)) {
        if (g_fUseMicroBrand && RECTHEIGHT(rc) < s_bcWebSmall.cyBrand)
        {
            if (s_bcWebMicro.hbm == NULL)
            {
                _InitBrandContext(&s_bcWebMicro, NULL, NULL,
                    NULL, NULL, MICROBITMAPID());
            }
            _pbc = &s_bcWebMicro;

        }
        else
        {
            if (_fShellView)
            {
                if (s_bcShellSmall.hbm == NULL)
                {
                    if (g_fUseMicroBrand)
                    {
                        // In this case, the shell and web bitmaps are always the same;
                        // load the web bitmap and use it for the shell also
                        if (s_bcWebSmall.hbm == NULL)
                        {
                            _InitBrandContext(&s_bcWebSmall, szValueSmBrandLeadIn, szValueSmBrandHeight,
                                szValueSmBrandBitmap, szValueSmallBitmap, SMALLBITMAPID());
                        }

                        s_bcShellSmall = s_bcWebSmall;
                    }
                    else
                    {
                        // We have different web and shell bitmaps; load the shell one
                        _InitBrandContext(&s_bcShellSmall, szValueSHSmBrandLeadIn, szValueSHSmBrandHeight,
                                szValueSHSmBrandBitmap, szValueSHSmallBitmap, SMALLBITMAPID());
                    }
                }

                _pbc = &s_bcShellSmall;
            }
            else
            {
                // We are in web view mode
                if (s_bcWebSmall.hbm == NULL)
                {
                    _InitBrandContext(&s_bcWebSmall, szValueSmBrandLeadIn, szValueSmBrandHeight,
                        szValueSmBrandBitmap, szValueSmallBitmap, SMALLBITMAPID());
                }

                _pbc = &s_bcWebSmall;
            }
        }
    } 
    else
    {
        if (_fShellView)
        {
            if (s_bcShellLarge.hbm == NULL)
            {
                if (g_fUseMicroBrand)
                {
                    // Shell and Web bitmaps are the same. Load the web one and copy it
                    if (s_bcWebLarge.hbm == NULL)
                    {
                        _InitBrandContext(&s_bcWebLarge, szValueBrandLeadIn, szValueBrandHeight,
                            szValueBrandBitmap, szValueLargeBitmap, LARGEBITMAPID());
                    }
                    s_bcShellLarge = s_bcWebLarge;
                }
                else
                {
                    // Need to load the shell bitmap separately
                    _InitBrandContext(&s_bcShellLarge, szValueSHBrandLeadIn, szValueSHBrandHeight,
                        szValueSHBrandBitmap, szValueSHLargeBitmap, LARGEBITMAPID());
                }
            }
            _pbc = &s_bcShellLarge;
        }
        else
        {
            // We're in web view
            if (s_bcWebLarge.hbm == NULL)
            {
                _InitBrandContext(&s_bcWebLarge, szValueBrandLeadIn, szValueBrandHeight,
                    szValueBrandBitmap, szValueLargeBitmap, LARGEBITMAPID());
            }
            _pbc = &s_bcWebLarge;
        }
    }

    if (_pbc != pbcOld) {
        MSG msg;
        
        _yOrg = 0;
        InvalidateRect(_hwnd, NULL, TRUE);
        if (!PeekMessage(&msg, _hwnd, BM_BANDINFOCHANGED, BM_BANDINFOCHANGED, PM_NOREMOVE))
            PostMessage(_hwnd, BM_BANDINFOCHANGED, 0, 0);                   
    }
}

HRESULT CBrandBand::_LoadBrandingBitmap()
{
    if (_pbc->hbm)
        return S_OK;    // Nothing to do, already loaded.

    _yOrg = 0;

    _InitBrandContexts();

    return(S_OK);
}

void CBrandBand::_DrawBranding(HDC hdc)
{
    HPALETTE    hpalPrev;
    RECT        rcPaint;
    COLORREF    clrBk = _fAnimating? _pbc->clrBkAnim : _pbc->clrBkStat;
    int         x, y, cx, cy;
    int         yOrg = 0;
    DWORD       dwRop = SRCCOPY;

    if (_fAnimating)
        yOrg = _yOrg;

    if (_pbc->hpal)
    {
        // select in our palette so the branding will get mapped to 
        // whatever the current system palette is. Note we do not
        // pass FALSE, so we will no actually select this palette into
        // system palette FG. Otherwise the branding will flash the
        // palette
        hpalPrev = SelectPalette(hdc, _pbc->hpal, TRUE);
        RealizePalette(hdc);
    }

    GetClientRect(_hwnd, &rcPaint);

    x  = rcPaint.left;
    cx = RECTWIDTH(rcPaint);
    y  = rcPaint.top;
    cy = RECTHEIGHT(rcPaint);
    
    if (cx > _pbc->cxBrandExtent)
    {
        RECT rc = rcPaint;
        int dx = ((cx - _pbc->cxBrandExtent) / 2) + 1;
        rc.right = rc.left + dx;
        SHFillRectClr(hdc, &rc, clrBk);
        rc.right = rcPaint.right;
        rc.left = rc.right - dx;
        SHFillRectClr(hdc, &rc, clrBk);        
    }
    if (cy > _pbc->cyBrand)
    {
        RECT rc = rcPaint;
        int dy = ((cy - _pbc->cyBrand) / 2) + 1;
        rc.bottom = rc.top + dy;
        SHFillRectClr(hdc, &rc, clrBk);
        rc.bottom = rcPaint.bottom;
        rc.top = rc.bottom - dy;
        SHFillRectClr(hdc, &rc, clrBk);
    }
    
    // center it
    if (cx > _pbc->cxBrandExtent)
        x += (cx - _pbc->cxBrandExtent) / 2;
    if (cy > _pbc->cyBrand)     
        y += (cy - _pbc->cyBrand) / 2;    

    //
    // To prevent the transform from flipping
    // calculations should be based on the bm width
    // when the DC is Right-To-Left mirrored and
    // not to flip the IE logo bitmap [samera]
    //
    if (IS_WINDOW_RTL_MIRRORED(_hwnd))
    {
        // Actual width
        cx = _pbc->cxBrandExtent;

        // Don't flip the logo here
        dwRop |= DONTMIRRORBITMAP;
    }


    ENTERCRITICAL;
    SelectObject(s_hdc, _pbc->hbm);
    BitBlt(hdc, x, y, cx, _pbc->cyBrand, s_hdc, 0, yOrg, dwRop);
    LEAVECRITICAL;

    if (_pbc->hpal)
    {
        // reselect in the old palette
        SelectPalette(hdc, hpalPrev, TRUE);
        RealizePalette(hdc);
    }
}

int CBrandBand::_GetLinksExtent()
{
    return 0x26;
}

void CBrandBand::_OnTimer(WPARAM id)
{
    _yOrg += _pbc->cyBrand;
    if (_yOrg >= _pbc->cyBrandExtent)
        _yOrg = _pbc->cyBrandLeadIn;
    RedrawWindow(_hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

// The IUnknown parameter needs to point to an object that supports the
// IBrowserService and IWebBrowserApp interfaces.
HRESULT CBrandBand::_ConnectToBrwsrWnd(IUnknown * punk)
{
    HRESULT hr = S_OK;

    if (_pdie)
    {
        // Remove the tie from the AddressBand to the Browser Window
        _ConnectToBrwsrConnectionPoint(FALSE);
        ATOMICRELEASE(_pdie);
    }
    
    ATOMICRELEASE(_pwb2);
    ATOMICRELEASE(_pbs);

    if (punk)
    {
        // Tie the AddressBand to the Browser Window passed in.
        IServiceProvider*   psp     = NULL;
        hr = punk->QueryInterface(IID_IServiceProvider, (void **)&psp);

        if (SUCCEEDED(hr))
        {
            // NOTE: We are either a Toolbar, in which case _pbs is valid
            //       and _pwb2 is NULL, or we are a Toolband and _pbs is
            //       NULL and _pwb2 is valid. Both will be NULL when the
            //       Toolband has yet to create a Browser Window.

            if (FAILED(psp->QueryService(SID_STopLevelBrowser, IID_IBrowserService, (void**)&_pbs)))
                hr = psp->QueryService(SID_SWebBrowserApp, IID_IWebBrowser2, (void**)&_pwb2);
            hr = psp->QueryService(SID_SWebBrowserApp, IID_IWebBrowserApp, (void**)&_pdie);
            psp->Release();

            if (_pdie && (_pwb2 || _pbs))
                _ConnectToBrwsrConnectionPoint(TRUE);
            else
            {
                ATOMICRELEASE(_pdie);
                ATOMICRELEASE(_pwb2);
                ATOMICRELEASE(_pbs);

                hr = E_FAIL;
            }
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Connect to Browser Window's ConnectionPoint that will provide events
// to let us keep up to date.
/////////////////////////////////////////////////////////////////////////////
HRESULT CBrandBand::_ConnectToBrwsrConnectionPoint(BOOL fConnect)
{
    return ConnectToConnectionPoint(SAFECAST(this, IDeskBand*), 
        DIID_DWebBrowserEvents, fConnect, _pdie, &_dwcpCookie, NULL);
}


LRESULT CALLBACK CBrandBand::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CBrandBand * ptc= (CBrandBand *)GetWindowPtr0(hwnd);   // GetWindowLong(hwnd, 0)

    switch (uMsg)
    {
        case WM_TIMER:
            ptc->_OnTimer(wParam);
            break;

        case WM_ERASEBKGND:
        {
            HDC hdc = (HDC)wParam;
            RECT rc;
            GetClientRect(hwnd, &rc);
            SHFillRectClr(hdc, &rc, ptc->_fAnimating ? ptc->_pbc->clrBkAnim : ptc->_pbc->clrBkStat);
            break;
        }
            
        case WM_PAINT:
            ptc->_UpdateCompressedSize();
            if (GetUpdateRect(hwnd, NULL, FALSE))
            {
                PAINTSTRUCT ps;

                BeginPaint(hwnd, &ps);
                ptc->_DrawBranding(ps.hdc);
                EndPaint(hwnd, &ps);
            }
            break;
            
        case WM_SIZE:
            InvalidateRect(ptc->_hwnd, NULL, TRUE);
            ptc->_UpdateCompressedSize();
            break;

        case BM_BANDINFOCHANGED:
            ptc->_BandInfoChanged();
            break;

        default:
            return DefWindowProcWrap(hwnd, uMsg, wParam, lParam);
    }
 
    return 0;       
}

HRESULT CBrandBand::Exec(const GUID *pguidCmdGroup,
                         DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    HRESULT hr = OLECMDERR_E_UNKNOWNGROUP;  // assume failure
    if (pguidCmdGroup) {
        
        if (IsEqualGUID(CGID_PrivCITCommands, *pguidCmdGroup))
        {
            hr = S_OK;
            switch (nCmdID)
            {
            case CITIDM_BRANDSIZE:
                if (pvarargIn && pvarargIn->vt == VT_I4) {
                    BOOL fMin = BOOLIFY(pvarargIn->lVal);
                    if (fMin != BOOLIFY(_fMinAlways)) {
                        _fMinAlways = fMin;
                        _BandInfoChanged();
                    }
                }
                break;

            case CITIDM_ONINTERNET:
                switch (nCmdexecopt)
                {                
                case CITE_SHELL:
                    hr = E_FAIL;
                    if (_pbs)
                    {
                        LPITEMIDLIST pidl;
                        
                        hr = _pbs->GetPidl(&pidl);
                        if (SUCCEEDED(hr))
                        {
                            // We may really be an IShellView for an internet NSE (like FTP)
                            // Find out if they want this feature
                            _fShellView = !IsBrowserFrameOptionsPidlSet(pidl, BFO_USE_IE_LOGOBANDING);
                            ILFree(pidl);
                        }
                        break;
                    }

                case CITE_INTERNET:
                    _fShellView = FALSE;
                    break;
                }
                _UpdateCompressedSize();
                break;
            
            case CITIDM_THEATER:
                switch(nCmdexecopt) {
                case THF_ON:
                    _fTheater = TRUE;
                    break;

                case THF_OFF:
                    _fTheater = FALSE;
                    break;

                default:
                    goto Bail;
                }

                _BandInfoChanged();
                break;                       
            
            case CITIDM_GETDEFAULTBRANDCOLOR:
                if (pvarargOut && pvarargOut->vt == VT_I4)
                    pvarargOut->lVal = g_fUseMicroBrand ? s_bcWebSmall.clrBkStat : s_bcShellSmall.clrBkStat;
                break;
            }
        }
    }
Bail:
    return hr;
}
