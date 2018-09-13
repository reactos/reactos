#include "pch.h"
#include "iids.h"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ Column information used by the shell to Detail view mode.
/----------------------------------------------------------------------------*/

//
// column information
//

const struct
{
    UINT idString;              // resource ID for the column title
    INT  fmt;                   // formatting flags of the column
    INT  cxChar;                // average char with of this column
}
columns[] =
{
    IDS_OBJECTNAME, LVCFMT_LEFT, 64,                // DSVMID_ARRANGEBYNAME
    IDS_TYPE,       LVCFMT_LEFT, 32,                // DSVMID_ARRANGEBYCLASS
};

//
// CDsFolder
//

#define NAMESPACE_ATTRIBUTES (SFGAO_FOLDER|SFGAO_FILESYSANCESTOR|SFGAO_HASSUBFOLDER|SFGAO_CANLINK)

class CDsFolder : public IDsFolderInternalAPI, IPersistFolder2, IDelegateFolder, IShellFolder, CUnknown
{
    friend HRESULT _GetDetailsOf(CDsFolder *pdf, PDETAILSINFO pDetails, UINT iColumn);
    friend HRESULT _MergeArrangeMenu(CDsFolder *pdf, LPARAM arrangeParam, LPQCMINFO pInfo);

    private:
        CLSID             _clsidNamespace;          // where in the registry to look for our configuration

        LPITEMIDLIST      _pidl;                    // absolute IDLIST to our object
        INT               _cbOffset;                // offset to start of ds elements
        LPTSTR            _pPrettyPath;             // prettified version of the folder path

        LPWSTR            _pAttribPrefix;           // attribute prefix (set via internal API)
        DWORD             _dwProviderAND;           // extra AND for the provider flags
        DWORD             _dwProviderXOR;           // extra XOR for the provider flags

        LPWSTR            _pServer;                 // server
        LPWSTR            _pUserName;               // user name 
        LPWSTR            _pPassword;               // password

        HRESULT           _hresCoInit;              // did we do a CoInitialize?
        IADsPathname      *_padp;                   // IADsPathname iface for name cracking
        IDsDisplaySpecifier *_pdds;                 // IDsDisplaySpecifier for DsXXX APIs
        IMalloc           *_pm;                     // IMalloc used for delegate folder support

    private:
        HRESULT _RealInitialize(LPCITEMIDLIST pidlRoot, LPCITEMIDLIST pidlBindTo, INT cbOffset);
        BOOL    _InitCOM(void);
        HRESULT _GetPathname(void);
        HRESULT _GetDsDisplaySpecifier(void);
        HRESULT _GetJunction(LPITEMIDLIST pidl, LPITEMIDLIST *ppidlRight, IDLISTDATA *pData);
        HRESULT _GetJunctionSF(LPITEMIDLIST pidlFull, IBindCtx *pbc, IDLISTDATA *pData, LPITEMIDLIST *ppidlRight, IShellFolder **ppsf);
        HRESULT _TryToParsePath(LPITEMIDLIST* ppidl, IADsPathname *padp, LPWSTR pObjectClass, LPDOMAINTREE pDomainTree);
        HRESULT _SetDispSpecOptions(IDataObject *pdo);

    public:
        CDsFolder();
        ~CDsFolder();

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IDsFolderInternalAPI
        STDMETHOD(SetAttributePrefix)(LPWSTR pAttributePrefix);
        STDMETHOD(SetProviderFlags)(DWORD dwAND, DWORD dwXOR);
        STDMETHOD(SetComputer)(LPCWSTR pszComputerName, LPCWSTR pszUserName, LPCWSTR pszPassword);

        // IPersistFolder
        STDMETHOD(GetClassID)(LPCLSID pClassID);
        STDMETHOD(Initialize)(LPCITEMIDLIST pidl);

        // IPersistFolder2
        STDMETHOD(GetCurFolder)(LPITEMIDLIST *ppidl);

        // IDelegateFolder
        STDMETHOD(SetItemAlloc)(IMalloc *pm);

        // IShellFolder
        STDMETHOD(ParseDisplayName)(HWND hwndOwner, LPBC pbc, LPOLESTR pDisplayName, 
                                      ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes);

        STDMETHOD(EnumObjects)(HWND hwndOwner, DWORD grfFlags, LPENUMIDLIST * ppEnumIDList);
        STDMETHOD(BindToObject)(LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, void **ppv);
        STDMETHOD(BindToStorage)(LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, void **ppv);
        STDMETHOD(CompareIDs)(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
        STDMETHOD(CreateViewObject)(HWND hwndOwner, REFIID riid, void **ppv);
        STDMETHOD(GetAttributesOf)(UINT cidl, LPCITEMIDLIST * apidl, ULONG * rgfInOut);
        STDMETHOD(GetUIObjectOf)(HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl, REFIID riid, UINT * prgfInOut, void **ppv);
        STDMETHOD(GetDisplayNameOf)(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET pName);
        STDMETHOD(SetNameOf)(HWND hwndOwner, LPCITEMIDLIST pidl, LPCOLESTR pszName, DWORD uFlags, LPITEMIDLIST* ppidlOut);
};


//
// CDsExtractIcon 
//

class CDsExtractIcon : public IExtractIcon, CUnknown
{
    private:
        LPITEMIDLIST _pidl;     
        IDsDisplaySpecifier *_pdds;

    public:
        CDsExtractIcon(IDsDisplaySpecifier *pdds, LPCITEMIDLIST pidl);
        ~CDsExtractIcon();

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IExtractIcon
        STDMETHOD(GetIconLocation)(UINT uFlags, LPTSTR szIconFile, UINT cchMax, int* pIndex, UINT* pwFlags);
        STDMETHOD(Extract)(LPCTSTR pszFile, UINT nIconIndex, HICON* pLargeIcon, HICON* pSmallIcon, UINT nIconSize);
};


//
// CDsFolderProperties
//

class CDsFolderProperties : public IDsFolderProperties, CUnknown
{
    public:
        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IDsFolderProperties
        STDMETHOD(ShowProperties)(HWND hwndParent, IDataObject *pDataObject);
};


/*-----------------------------------------------------------------------------
/ Callback functions used by this IShellFolder implementation
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ _StrRetFromString
/ -----------------
/   Package a WIDE string into a LPSTRRET structure.
/
/ In:
/   pStrRet -> receieves the newly allocate string
/   pString -> string to be copied.
/
/ Out:
/   -
/----------------------------------------------------------------------------*/

HRESULT _StrRetFromString(LPSTRRET lpStrRet, LPCTSTR pString)
{
    HRESULT hres;

    TraceEnter(TRACE_FOLDER, "_StrRetFromString");
    Trace(TEXT("pStrRet %08x, lpszString -%s-"), lpStrRet, pString);

    TraceAssert(lpStrRet);
    TraceAssert(pString);

#ifdef UNICODE
    lpStrRet->pOleStr = (LPWSTR)SHAlloc(StringByteSize(pString));

    if ( !lpStrRet->pOleStr )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate buffer for string");

    lpStrRet->uType = STRRET_OLESTR;
    StrCpy(lpStrRet->pOleStr, pString);                
#else
    if ( lstrlen(pString) > (MAX_PATH-1) )
        ExitGracefully(hres, E_OUTOFMEMORY, "Buffer too small for string");

    lpStrRet->uType = STRRET_CSTR;
    StrCpy(lpStrRet->cStr, pString);
#endif

    hres = S_OK;                              // success

exit_gracefully:

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ _GetDetailsOf
/ -------------
/   Handle the GetDetailsOf call back for this view.  If PIDL is NULL on
/   entry then just return the column heading, otherwise get that information
/   for the IDLIST and column combination.
/
/ In:
/   pDetails -> structure to fill
/   iColumn -> column being invoked on
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT _GetDetailsOf(CDsFolder *pdf, PDETAILSINFO pDetails, UINT iColumn)
{
    HRESULT hres;
    IDLISTDATA data;
    USES_CONVERSION;

    TraceEnter(TRACE_FOLDER, "_GetDetailsOf");
    TraceAssert(pdf != NULL);

    if ( iColumn > ARRAYSIZE(columns) )
        ExitGracefully(hres, E_INVALIDARG, "Bad column index");

    // Fill out the structure with the formatting information,
    // and a dummy string incase we fail.

    pDetails->fmt = columns[iColumn].fmt;
    pDetails->cxChar = columns[iColumn].cxChar;
    pDetails->str.uType = STRRET_CSTR;
    pDetails->str.cStr[0] = TEXT('\0');

    if ( !pDetails->pidl )
    {
        TCHAR szBuffer[MAX_PATH];

        if ( !LoadString(GLOBAL_HINSTANCE, columns[iColumn].idString, szBuffer, ARRAYSIZE(szBuffer)) )
            ExitGracefully(hres, E_FAIL, "Failed to load column heading");

        hres = _StrRetFromString(&pDetails->str, szBuffer);
        FailGracefully(hres, "Failed making a StrRet from the column heading");
    }
    else
    {
        hres = UnpackIdList(pDetails->pidl, DSIDL_HASCLASS, &data);
        FailGracefully(hres, "Failed to unpack the IDLIST");

        if ( iColumn == DSVMID_ARRANGEBYCLASS )
        {
            WCHAR szBuffer[MAX_PATH];

            if ( SUCCEEDED(pdf->_GetDsDisplaySpecifier()) )
                pdf->_pdds->GetFriendlyClassName(data.pObjectClass, szBuffer, ARRAYSIZE(szBuffer));
            else
                StrCpyW(szBuffer, data.pObjectClass);

            hres = _StrRetFromString(&pDetails->str, W2CT(szBuffer));
            FailGracefully(hres, "Failed to make StrRet from class name");
        }
        else
        {
            TraceAssert(FALSE);
            ExitGracefully(hres, E_FAIL, "Bad column specified");
        }
    }

    // hres = S_OK;

exit_gracefully:

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ _MergeArrangeMenu
/ -----------------
/   Merge our verbs into the view menu
/
/ In:
/   arrangeParam = current sort parameter
/   pInfo -> QCMINFO structure
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT _MergeArrangeMenu(CDsFolder *pdf, LPARAM arrangeParam, LPQCMINFO pInfo)
{
    HRESULT hres;
    MENUITEMINFO mii = { SIZEOF(MENUITEMINFO), MIIM_SUBMENU };
    UINT idCmdFirst = pInfo->idCmdFirst;
    HMENU hMyArrangeMenu;

    TraceEnter(TRACE_FOLDER, "_MergeArrangeMenu");
    TraceAssert(pdf != NULL);
    Trace(TEXT("arrangeParam %08x, pInfo->idCmdFirst %08x"), arrangeParam, pInfo->idCmdFirst);

    if ( GetMenuItemInfo(pInfo->hmenu, SFVIDM_MENU_ARRANGE, FALSE, &mii) )
    {
        hMyArrangeMenu = LoadMenu(GLOBAL_HINSTANCE, MAKEINTRESOURCE(IDR_ARRANGE));

        if ( hMyArrangeMenu )
        {
            pInfo->idCmdFirst = Shell_MergeMenus(mii.hSubMenu,
                                                 GetSubMenu(hMyArrangeMenu, 0), 
                                                 0, 
                                                 pInfo->idCmdFirst, pInfo->idCmdLast,
                                                 0);                    
            DestroyMenu(hMyArrangeMenu);
        }
    } 

    TraceLeaveResult(S_OK);
}


/*-----------------------------------------------------------------------------
/ _FolderCallBack
/ ---------------
/   Handles callbacks from the shell for the view object we are the parent of.
/
/ In:
/   psvView -> view object
/   psf -> shell folder (our object)
/   hwndMain = window handle for a dialog parent
/   uMsg, wParam, lParam = message specific information
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
HRESULT CALLBACK _FolderCallBack(LPSHELLVIEW psvOuter, LPSHELLFOLDER psf, HWND hwndView, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hres = NOERROR;
    CDsFolder *pdf;

    psf->QueryInterface(CLSID_DsFolderSF, (void **)&pdf);

    switch (uMsg)
    {
        case SFVM_INITMENUPOPUP:
            break;

        case SFVM_MERGEMENU:
            hres = _MergeArrangeMenu(pdf, ShellFolderView_GetArrangeParam(hwndView), (LPQCMINFO)lParam);
            break;

        case SFVM_INVOKECOMMAND:
        {
            UINT idCmd = (UINT)wParam;

            switch ( idCmd )
            {
                case DSVMID_ARRANGEBYNAME:
                case DSVMID_ARRANGEBYCLASS:
                    ShellFolderView_ReArrange(hwndView, idCmd);
                    break;

                default:
                    hres = S_FALSE;
                    break;
            }

            break;
        }

        case SFVM_GETHELPTEXT:
        {
            hres = S_OK;
        
            switch ( LOWORD(wParam) )
            {
                case DSVMID_ARRANGEBYNAME:
                    LoadString(GLOBAL_HINSTANCE, IDS_BYOBJECTNAME, (LPTSTR)lParam, HIWORD(wParam));
                    break;

                case DSVMID_ARRANGEBYCLASS:
                    LoadString(GLOBAL_HINSTANCE, IDS_BYTYPE, (LPTSTR)lParam, HIWORD(wParam));
                    break;

                default:
                    hres = S_FALSE;
                    break;
            }
        }

        case SFVM_GETDETAILSOF:
            hres = _GetDetailsOf(pdf, (PDETAILSINFO)lParam, (UINT)wParam);
            break;

        case SFVM_COLUMNCLICK:
            ShellFolderView_ReArrange(hwndView, (UINT)wParam);
            break;

        case SFVM_BACKGROUNDENUM:
            hres = S_OK;
            break;

        default:
            hres = E_NOTIMPL;
            break;
    }

    return hres;
}


/*-----------------------------------------------------------------------------
/ _FolderCFMCallBack
/ ------------------
/   Handles callbacks for the context menu which is displayed when the user
/   right clicks on objects within the view.
/
/ In:
/   psf -> shell folder (our object)
/   hwndView = window handle for a dialog parent
/   pDataObject -> data object for the menu
/   uMsg, wParam, lParam = message specific information
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
HRESULT CALLBACK _FolderCFMCallback(LPSHELLFOLDER psf, HWND hwndView, LPDATAOBJECT pDataObject, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hres = NOERROR;
    CDsFolder* pdf;

    TraceEnter(TRACE_FOLDER, "_FolderCFMCallback");
    Trace(TEXT("psf %08x, hwndView %08x, pDataObject %08x"), psf, hwndView, pDataObject);
    Trace(TEXT("uMsg %08x, wParam %08x, lParam %08x"), uMsg, wParam, lParam);

    psf->QueryInterface(CLSID_DsFolderSF, (void **)&pdf);

    switch ( uMsg )
    {
        case DFM_MERGECONTEXTMENU:
            hres = _MergeArrangeMenu(pdf, ShellFolderView_GetArrangeParam(hwndView), (LPQCMINFO)lParam);
            break;

        case DFM_GETHELPTEXTW:
        {
            hres = S_OK;
        
            switch ( LOWORD(wParam) )
            {
                case DSVMID_ARRANGEBYNAME:
                    LoadString(GLOBAL_HINSTANCE, IDS_BYOBJECTNAME, (LPTSTR)lParam, HIWORD(wParam));
                    break;

                case DSVMID_ARRANGEBYCLASS:
                    LoadString(GLOBAL_HINSTANCE, IDS_BYTYPE, (LPTSTR)lParam, HIWORD(wParam));
                    break;

                default:
                    hres = S_FALSE;
                    break;
            }
        }

        case DFM_INVOKECOMMAND:
        {
            UINT idCmd = (UINT)wParam;

            switch ( idCmd )
            {
                case (UINT)DFM_CMD_PROPERTIES:
                    hres = ShowObjectProperties(hwndView, pDataObject);
                    break;

                case DSVMID_ARRANGEBYNAME:
                case DSVMID_ARRANGEBYCLASS:
                    ShellFolderView_ReArrange(hwndView, idCmd);
                    break;

                default:
                    hres = S_FALSE;
                    break;
            }

            break;
        }

        case DFM_GETDEFSTATICID:
        {
            *((UINT*)lParam) = (UINT)DFM_CMD_PROPERTIES;
            break;
        }

        default:
            hres = E_NOTIMPL;
            break;
    }

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ CDsFolder
/   This is our DS IShellFolder implementation, it provides a mapping from
/   the shell namespace down to Active Directory.
/----------------------------------------------------------------------------*/

CDsFolder::CDsFolder() :
    _dwProviderAND(0xffffffff),
    _hresCoInit(E_FAIL)
{
}

CDsFolder::~CDsFolder()
{
    DoILFree(_pidl);

    LocalFreeString(&_pPrettyPath);
    LocalFreeStringW(&_pAttribPrefix);

    LocalFreeStringW(&_pServer);
    LocalFreeStringW(&_pUserName);
    LocalFreeStringW(&_pPassword);

    DoRelease(_padp);
    DoRelease(_pdds);
    DoRelease(_pm);
}

#undef CLASS_NAME
#define CLASS_NAME CDsFolder
#include "unknown.inc"

STDMETHODIMP CDsFolder::QueryInterface( REFIID riid, void **ppv)
{
    INTERFACES iface[] =
    {
        &IID_IDsFolderInternalAPI, (IDsFolderInternalAPI*)this,
        &IID_IShellFolder, (LPSHELLFOLDER)this,
        &IID_IPersistFolder, (LPPERSISTFOLDER)this,
        &IID_IPersistFolder2, (IPersistFolder2*)this,
        &IID_IDelegateFolder, (IDelegateFolder*)this,
    };

    // requesting CLSID_DsFolder gets the CDsFolder object without a reference
    // count.

    if ( IsEqualIID(CLSID_DsFolderSF, riid) )
    {
        *ppv = this;
        return S_OK;
    }

   return HandleQueryInterface(riid, ppv, iface, ARRAYSIZE(iface));
}

//
// handle create instance of the CDsFolder object
//

STDAPI CDsFolder_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CDsFolder *pdf = new CDsFolder();
    if ( !pdf )
        return E_OUTOFMEMORY;

    HRESULT hres = pdf->QueryInterface(IID_IUnknown, (void **)ppunk);
    pdf->Release();
    return hres;
}


//-----------------------------------------------------------------------------
// Helper functions
//-----------------------------------------------------------------------------

//
// ensure we have COM alive...
//

BOOL CDsFolder::_InitCOM(void)
{
    TraceEnter(TRACE_FOLDER, "CDsFolder::_InitCOM");

    if ( FAILED(_hresCoInit) )
    {
        TraceMsg("Calling CoInitialize");
        _hresCoInit = CoInitialize(NULL);
    }

    TraceLeaveValue(SUCCEEDED(_hresCoInit));
}

//
// ensure we have the IADsPathname object
//

HRESULT CDsFolder::_GetPathname(void)
{
    HRESULT hres;

    TraceEnter(TRACE_FOLDER, "CDsFolder::_GetPathname");

    if ( _InitCOM() && !_padp )
    {
        hres = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER, IID_IADsPathname, (void **)&_padp);
        FailGracefully(hres, "Failed to get the IADsPathname interface");
    }

    hres = S_OK;

exit_gracefully:

    if ( SUCCEEDED(hres) )
        _padp->SetDisplayType(ADS_DISPLAY_FULL);

    TraceLeaveResult(hres);
}

//
// ensure we have the IDsDisplaySpecifier object
//

HRESULT CDsFolder::_GetDsDisplaySpecifier(void)
{
    HRESULT hres = S_OK;

    TraceEnter(TRACE_FOLDER, "CDsFolder::_GetDsDisplaySpecifier");

    if ( _InitCOM() && !_pdds )
    {
        hres = CoCreateInstance(CLSID_DsDisplaySpecifier, NULL, CLSCTX_INPROC_SERVER, IID_IDsDisplaySpecifier, (void **)&_pdds);
        if ( SUCCEEDED(hres) )
            _pdds->SetServer(_pServer, _pUserName, _pPassword, 0x0);
    }

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ IDsFolderInternalAPI
/----------------------------------------------------------------------------*/

STDMETHODIMP CDsFolder::SetAttributePrefix(LPWSTR pAttributePrefix)
{
    HRESULT hres;
    USES_CONVERSION;
        
    TraceEnter(TRACE_FOLDER, "CDsFolder::SetAttributePrefix");

    LocalFreeStringW(&_pAttribPrefix);

    hres = LocalAllocStringW(&_pAttribPrefix, pAttributePrefix);
    FailGracefully(hres, "Failed to store attribute prefix");

    Trace(TEXT("Setting attribute prefix to: %s"), W2T(pAttributePrefix));
    
exit_gracefully:

    TraceLeaveResult(hres);
}

//-----------------------------------------------------------------------------

STDMETHODIMP CDsFolder::SetProviderFlags(DWORD dwAND, DWORD dwXOR)
{
    TraceEnter(TRACE_FOLDER, "CDsFolder::SetProviderFlags");
    Trace(TEXT("dwAND %08x, dwXOR %08x"), dwAND, dwXOR);

    _dwProviderAND = dwAND;
    _dwProviderXOR = dwXOR;

    TraceLeaveResult(S_OK);
}

STDMETHODIMP CDsFolder::SetComputer(LPCWSTR pszComputerName, LPCWSTR pszUserName, LPCWSTR pszPassword)
{
    HRESULT hres;

    TraceEnter(TRACE_FOLDER, "CDsFolder::SetComputer");

    LocalFreeStringW(&_pServer);
    LocalFreeStringW(&_pUserName);
    LocalFreeStringW(&_pPassword);

    hres = LocalAllocStringW(&_pServer, pszComputerName);
    if ( SUCCEEDED(hres) )
        hres = LocalAllocStringW(&_pUserName, pszUserName);
    if ( SUCCEEDED(hres) )
        hres = LocalAllocStringW(&_pPassword, pszPassword);

    if ( FAILED(hres) )
    {
        LocalFreeStringW(&_pServer);
        LocalFreeStringW(&_pUserName);
        LocalFreeStringW(&_pPassword);
    }

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ IPersistFolder methods
/----------------------------------------------------------------------------*/

STDMETHODIMP CDsFolder::GetClassID(LPCLSID pClassID)
{
    TraceEnter(TRACE_FOLDER, "CDsFolder::GetClassID");

    TraceAssert(pClassID);
    *pClassID = CLSID_MicrosoftDS;

    TraceLeaveResult(S_OK);
}

/*---------------------------------------------------------------------------*/

// Initialization we handle in two ways.  We assume that IPersistFolder::Initialize
// is called with an absolute IDLIST to the start of our name space, further to
// that (eg. when we bind to an object) we call an internal version.
//
// We assume this because we need to have an idea of how much to skip to get
// to the DS specific parts of an absolute IDLIST.

STDMETHODIMP CDsFolder::Initialize(LPCITEMIDLIST pidl)
{
    HRESULT hres;

    TraceEnter(TRACE_FOLDER, "CDsFolder::Initialize");

    hres = _RealInitialize(pidl, NULL, ILGetSize(pidl)-SIZEOF(SHORT));
    FailGracefully(hres, "Failed when calling CDsFolder::_RealInitialize");

exit_gracefully:

    TraceLeaveResult(hres);
}

HRESULT CDsFolder::_RealInitialize(LPCITEMIDLIST pidlRoot, LPCITEMIDLIST pidlBindTo, INT cbOffset)
{
    HRESULT hres;
    LPITEMIDLIST pidl;

    TraceEnter(TRACE_FOLDER, "CDsFolder::_RealInitialize");

    if ( !pidlRoot )
        ExitGracefully(hres, E_FAIL, "Failed due to no pidlRoot");

    _cbOffset = cbOffset;

    if ( !pidlBindTo )
    {
        _pidl = ILClone(pidlRoot);
    }
    else
    {
        _pidl = ILCombine(pidlRoot, pidlBindTo);
    }

    if ( !_pidl )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to create root IDLIST");

    hres = S_OK;                  // success

exit_gracefully:

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ IPersistFolder2 methods
/----------------------------------------------------------------------------*/

STDMETHODIMP CDsFolder::GetCurFolder(LPITEMIDLIST *ppidl)
{
    HRESULT hres;

    TraceEnter(TRACE_FOLDER, "CDsFolder::GetCurFolder");

    if ( !ppidl || !_pidl )
        ExitGracefully(hres, E_INVALIDARG, "Bad ppidl pointer, or perhaps no PIDL");

    *ppidl = ILClone(_pidl);
    TraceAssert(*ppidl);

    if ( !*ppidl )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to clone our current folder pidl");

    hres = S_OK;

exit_gracefully:

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ IDelegateFolder methods
/----------------------------------------------------------------------------*/

STDMETHODIMP CDsFolder::SetItemAlloc(IMalloc *pm)
{
    TraceEnter(TRACE_FOLDER, "CDsFolder::SetItemAlloc");

    DoRelease(_pm);
    
    if ( pm )
    {
        pm->AddRef();
        _pm = pm;
    }

    TraceLeaveResult(S_OK);
}


/*-----------------------------------------------------------------------------
/ IShellFolder methods
/----------------------------------------------------------------------------*/

#define SKIP_PREFIX 7 // 'LDAP://'

HRESULT CDsFolder::_TryToParsePath(LPITEMIDLIST* ppidl, IADsPathname *padp, LPWSTR pObjectClass, LPDOMAINTREE pDomainTree)
{
    HRESULT hres;
    LPDOMAINDESC pDomainDesc;
    BSTR bstrPath = NULL;
    BSTR bstrPathWithServer = NULL;
    LONG lElements;
    BOOL fDomainRoot = FALSE;
    LPITEMIDLIST pidl = NULL;
    LPITEMIDLIST pidlCombined = NULL;
    DWORD i;
    USES_CONVERSION;

    TraceEnter(TRACE_PARSE, "_TryToParsePath");

    hres = padp->Retrieve(ADS_FORMAT_X500_NO_SERVER, &bstrPath);
    FailGracefully(hres, "Failed to retrieve the path (without server reference)");

    hres = padp->Retrieve(ADS_FORMAT_X500, &bstrPathWithServer);
    FailGracefully(hres, "Failed to retrieve the path");

    // Get the path we are parsing, is it a domain root, if so then we
    // don't want to look any deeper.  

    Trace(TEXT("Path is %s"), W2T(bstrPath+SKIP_PREFIX));

    for ( pDomainDesc = pDomainTree->aDomains ; pDomainDesc ; pDomainDesc = pDomainDesc->pdNextSibling )
    {
        Trace(TEXT("Comparing to domain %s"), W2T(pDomainDesc->pszNCName));
        if ( !StrCmpIW(bstrPath+SKIP_PREFIX, pDomainDesc->pszNCName) )
        {
            TraceMsg("Found root domain object");
            fDomainRoot = TRUE;
            break;
        }
    }

    // if this is the domain root then lets pack the PIDL for it.

    if ( !fDomainRoot ) 
    {
        hres = padp->RemoveLeafElement();                                     // remove the leaf element
        FailGracefully(hres, "Failed to remove the leaf element");

        padp->GetNumElements(&lElements);
        Trace(TEXT("lElements %08x"), lElements);

        if ( !lElements )        
            ExitGracefully(hres, E_INVALIDARG, "Bad pathname passed");

        hres = _TryToParsePath(ppidl, padp, NULL, pDomainTree);     // we don't know the class
        FailGracefully(hres, "Failed recursing into the names");
    }

    // now convert this into an IDLIST and append it to the one we 
    // already have to return to the caller.

    hres = _GetDsDisplaySpecifier();
    FailGracefully(hres, "Failed to get the IDsDisplaySpecifier object");

    hres = CreateIdListFromPath(&pidl, NULL, bstrPathWithServer, pObjectClass, NULL, _pm, _pdds);
    FailGracefully(hres, "Failed to create an IDLIST");

    if ( *ppidl )
    {
        pidlCombined = ILCombine(*ppidl, pidl);
        TraceAssert(pidlCombined);

        if ( !pidlCombined )
            ExitGracefully(hres, E_OUTOFMEMORY, "Failed to combine the pidl");

        *ppidl = pidlCombined;
        pidlCombined = NULL;
    }
    else
    {
        *ppidl = pidl;
        pidl = NULL;
    }
    
    hres = S_OK;    

exit_gracefully:

    SysFreeString(bstrPath);
    SysFreeString(bstrPathWithServer);

    DoILFree(pidl);
    DoILFree(pidlCombined);

    TraceLeaveResult(hres);
}

//
// parse display name for IShellFolder 
// BUGBUG: fix to cope with the NTDS scheme
//

STDMETHODIMP CDsFolder::ParseDisplayName(HWND hwndOwner, 
                                         LPBC pbc, 
                                         LPOLESTR pDisplayName,
                                         ULONG* pchEaten, 
                                         LPITEMIDLIST* ppidl, 
                                         ULONG *pdwAttributes)
{
    HRESULT hres = E_INVALIDARG;
    BSTR bstrObjectClass = NULL;
    IPropertyBag* pPropertyBag = NULL;
    IDsBrowseDomainTree* pdbdt = NULL;
    LPDOMAINTREE pDomainTree = NULL;
    VARIANT variant;    
    USES_CONVERSION;

    TraceEnter(TRACE_FOLDER, "CDsFolder::ParseDisplayName");
    Trace(TEXT("pDisplayName: %s"), W2T(pDisplayName));

    *ppidl = NULL;

    VariantInit(&variant);

    if ( !*pDisplayName )
        ExitGracefully(hres, S_OK, "Name is NULL, nothing to parse");

    // Look and see if we have the DS property bag, if we do then
    // we can check to see if we have an object class property
    // within that.

    if ( pbc && SUCCEEDED(pbc->GetObjectParam(DS_PDN_PROPERTYBAG, (IUnknown**)&pPropertyBag)) )
    {
        hres = pPropertyBag->Read(DS_PDN_OBJECTLCASS, &variant, NULL);
        if ( SUCCEEDED(hres) )
        {
            if ( V_VT(&variant) != VT_BSTR )
                ExitGracefully(hres, E_UNEXPECTED, "Failed to get the object class");

            bstrObjectClass = V_BSTR(&variant);
            Trace(TEXT("ObjectClass from property bag is: %s"), W2T(bstrObjectClass));
        }
   }

    // ensure we have the IADsPathname interface then lets call the parser,
    // this can be quite a quick process although it does eat quite alot
    // of stack.  The resulting IDLIST is allocated using the malloc
    // interface to we give.

    hres = _GetPathname();        
    FailGracefully(hres, "Failed to get the IADsPathname object");

    hres = _padp->Set(pDisplayName, ADS_SETTYPE_FULL);
    FailGracefully(hres, "Failed to set the path of the name");

    _padp->SetDisplayType(ADS_DISPLAY_FULL);

    // get the domain list, we use this to determine the domain root and
    // therefore the point where parsing is no long needed.

    if ( !_InitCOM() )
        TraceMsg("Failed to initialize COM");

    hres = CoCreateInstance(CLSID_DsDomainTreeBrowser, NULL, CLSCTX_INPROC_SERVER, IID_IDsBrowseDomainTree, (void **)&pdbdt);
    FailGracefully(hres, "Failed to get the domain tree interface");

    hres = pdbdt->SetComputer(_pServer, _pUserName, _pPassword);
    FailGracefully(hres, "Failed to set browsing attributes");

    hres = pdbdt->GetDomains(&pDomainTree, DBDTF_RETURNFQDN);
    FailGracefully(hres, "Failed when getting domain tree information");  

    // now build a set of IDLISTs using this information, the pPathname contains a structure

    hres = _TryToParsePath(ppidl, _padp, bstrObjectClass, pDomainTree);
    FailGracefully(hres, "Failed when calling recursive parser");

    hres = S_OK;          // successs

exit_gracefully:        

    if ( pdbdt )
        pdbdt->FreeDomains(&pDomainTree);

    DoRelease(pPropertyBag);
    DoRelease(pdbdt);

    VariantClear(&variant);

    if ( hres == E_ADS_BAD_PATHNAME )
    {
        TraceMsg("Name was not an ADSI one, therefore making the result E_INVALIDARG");
        hres = E_INVALIDARG;
    }

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsFolder::EnumObjects(HWND hwndOwner, DWORD grfFlags, LPENUMIDLIST* ppEnumIdList)
{
    if ( !_InitCOM() )
        return E_FAIL;

    return CDsEnum_CreateInstance(_ILSkip(_pidl, _cbOffset), hwndOwner, grfFlags, _pm, ppEnumIdList);
}

/*---------------------------------------------------------------------------*/

HRESULT CDsFolder::_GetJunction(LPITEMIDLIST pidl, LPITEMIDLIST *ppidlRight, IDLISTDATA *pData)
{
    HRESULT hres;

    TraceEnter(TRACE_FOLDER, "CDsFolder::_GetJunction");

    // walk the pidl attempting to find the junction point, if we find one then we return both
    // the pidl data to the right and the main pidl.

    *ppidlRight = NULL;

    if (ILIsEmpty(pidl))
    {
        ExitGracefully(hres, E_INVALIDARG, "pidl is empty");
    }

    while ( !ILIsEmpty(pidl) )
    {
        hres = UnpackIdList(pidl, 0x0, pData);
        FailGracefully(hres, "Failed to unpack pidl");

        pidl = _ILNext(pidl);

        if ( pData->pUNC )
        {
            if ( !ILIsEmpty(pidl) )
            {
                *ppidlRight = ILClone(pidl);
                TraceAssert(*ppidlRight);

                if ( !*ppidlRight )
                    ExitGracefully(hres, E_OUTOFMEMORY, "Failed to clone the rest of the pidl");
            }

            pidl->mkid.cb = 0;       // truncate the pidl we have
            break;
        }
    }

    hres = S_OK;

exit_gracefully:

    TraceLeaveResult(hres);
}

HRESULT CDsFolder::_GetJunctionSF(LPITEMIDLIST pidlFull, IBindCtx *pbc, 
                                    IDLISTDATA *pData, LPITEMIDLIST *ppidlRight, IShellFolder **ppsf)
{
    HRESULT hres;
    IShellFolder *psf = NULL;
    IPersistFolder3 *ppf3 = NULL;
    IShellFolder* psfDesktop = NULL;
    IDLISTDATA data = { 0 };
    PERSIST_FOLDER_TARGET_INFO pfti = {0};
    LPITEMIDLIST pidl = NULL;

    TraceEnter(TRACE_FOLDER, "CDsFolder::_GetJunctionSF");

    if ( !pData )
        pData = &data;

    //
    // get the junction information from the object, if we have a UNC then we must
    // create the IShellFolder that represents it.
    //

    hres = _GetJunction(_ILSkip(pidlFull, _cbOffset), ppidlRight, pData);
    FailGracefully(hres, "Failed to bind to junction object");

    if ( !pData->pUNC )
        ExitGracefully(hres, S_FALSE, "Object is not a junction");

    if ( !_InitCOM() )
        TraceMsg("Failed to initialize COM");

    hres = CoCreateInstance(CLSID_ShellDesktop, NULL, CLSCTX_INPROC_SERVER, IID_IShellFolder, (LPVOID*)&psfDesktop);
    FailGracefully(hres, "Failed to get IShellFolder for the desktop object");

    hres = psfDesktop->ParseDisplayName(NULL, NULL, pData->pUNC, NULL, &pidl, NULL);
    FailGracefully(hres, "Failed to parse the UNC to a PIDL");
    
    //
    // In this case lets create the FS folder and initializeit accordingly with a PIDL.
    //

    hres = CoCreateInstance(CLSID_ShellFSFolder, NULL, CLSCTX_INPROC_SERVER, IID_IShellFolder, (void **)&psf);
    FailGracefully(hres, "Failed to get ShellFsFolder");

    hres = psf->QueryInterface(IID_IPersistFolder3, (void **)&ppf3);
    FailGracefully(hres, "Failed to get the IPersistFolderAlias");

    pfti.pidlTargetFolder = pidl;            
    pfti.dwAttributes = -1;        
    pfti.csidl = -1;

    hres = ppf3->InitializeEx(pbc, pidlFull, &pfti);
    if ( SUCCEEDED(hres) )
        psf->QueryInterface(IID_IShellFolder, (void **)ppsf);

exit_gracefully:

    DoRelease(psf);
    DoRelease(ppf3);
    DoRelease(psfDesktop);

    if ( pidl )
        ILFree(pidl);

    TraceLeaveResult(hres);
}

STDMETHODIMP CDsFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppv)
{
    HRESULT hres = E_FAIL;
    IShellFolder *psf = NULL;
    LPITEMIDLIST pidlFull = NULL;
    LPITEMIDLIST pidlRight = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_FOLDER, "CDsFolder::BindToObject");
    Trace(TEXT("Entry IDLIST is %08x"), pidl);
    TraceGUID("Interface being requested", riid);

    pidlFull = ILCombine(_pidl, pidl);

    if ( !pidlFull )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to alloc temporary pidl");

    // attempt to get the sf that has junction.  this call can return S_FALSE
    // indicating there is no junction, therefore we create a new instance
    // of CDsFolder.
    //
    // if it was a junction then continue the bind process through the
    // junction to the real namespace.

    hres = _GetJunctionSF(pidlFull, pbc, NULL, &pidlRight, &psf);
    if ( SUCCEEDED(hres) )
    {
        if ( hres == S_FALSE )
        {
            // Create a new CDsFolder object, pass on the IMalloc interface we have
            // etc.

            CDsFolder *pDsFolder = new CDsFolder();
            TraceAssert(pDsFolder);

            if ( !pDsFolder )
                ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate new CDsFolder");

            hres = pDsFolder->_RealInitialize(_pidl, pidl, _cbOffset);     

            if ( SUCCEEDED(hres) )
                hres = pDsFolder->SetItemAlloc(_pm);

            if ( SUCCEEDED(hres) )
                hres = pDsFolder->QueryInterface(riid, ppv);

            pDsFolder->Release();
        }
        else
        {
            // pidlRight?  if not then we are at the right point in the namespace,
            // otherwise we must continue the bind.

            if ( pidlRight )
                hres = psf->BindToObject(pidlRight, pbc, riid, ppv);
            else
                hres = psf->QueryInterface(riid, ppv);
        }
    }

exit_gracefully:

    DoILFree(pidlFull);
    DoILFree(pidlRight);

    DoRelease(psf);

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbcReserved, REFIID riid, void **ppv)
{
    TraceEnter(TRACE_FOLDER, "CDsFolder::BindToStorage");
    TraceLeaveResult(E_NOTIMPL);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsFolder::CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    IDLISTDATA data1, data2;
    HRESULT hres = E_INVALIDARG;
    INT iResult = 0;
    LPITEMIDLIST pidlT1, pidlT2;
    LPITEMIDLIST pidlT = NULL;
    WCHAR szName1[MAX_PATH];
    WCHAR szName2[MAX_PATH];
    IShellFolder *pShellFolder = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_COMPARE, "CDsFolder::CompareIDs");

    hres = UnpackIdList(pidl1, DSIDL_HASCLASS, &data1);
    FailGracefully(hres, "Failed to unpack pidl1");

    hres = UnpackIdList(pidl2, DSIDL_HASCLASS, &data2);
    FailGracefully(hres, "Failed to unpack pidl2");

    // Ensure that containers end up at the top of the list

    if ( (data1.dwFlags & DSIDL_ISCONTAINER) != (data2.dwFlags & DSIDL_ISCONTAINER) )
    {
        iResult = (data1.dwFlags & DSIDL_ISCONTAINER) ? -1:+1;
        goto exit_result;
    }

    // lParam indicates which column we are sorting on, therefore using that compare
    // the values we have just unpacked from IDLISTs.

    switch ( lParam & SHCIDS_COLUMNMASK )
    {
        case DSVMID_ARRANGEBYNAME:
        {
            _GetPathname();

            if ( SUCCEEDED(NameFromIdList(pidl1, _ILSkip(_pidl, _cbOffset), szName1, ARRAYSIZE(szName1), _padp)) &&
                    SUCCEEDED(NameFromIdList(pidl2, _ILSkip(_pidl, _cbOffset), szName2, ARRAYSIZE(szName2), _padp)) )
            {
                Trace(TEXT("Comparing -%s-, -%s-"), W2T(szName1), W2T(szName2));
                iResult = StrCmpIW(szName1, szName2);
            }

            break;
        }

        case DSVMID_ARRANGEBYCLASS:
            Trace(TEXT("Classes -%s-, -%s-"), W2T(data1.pObjectClass), W2T(data2.pObjectClass));
            iResult = StrCmpIW(data1.pObjectClass, data2.pObjectClass);
            break;

        default:
            TraceAssert(FALSE);
            ExitGracefully(hres, E_INVALIDARG, "Bad sort column");
            break;
    }

    // If they match then check that they are absolutely identical, if that is
    // the case then continue down the IDLISTs if more elements present.
   
    if ( iResult == 0 )
    {
        iResult = memcmp(pidl1, pidl2, ILGetSize(pidl1));
        Trace(TEXT("memcmp of pidl1, pidl2 yeilds %d"), iResult);

        if ( iResult != 0 )
            goto exit_result;

        pidlT1 = _ILNext(pidl1);
        pidlT2 = _ILNext(pidl2);

        if ( ILIsEmpty(pidlT1) )
        {
            if ( ILIsEmpty(pidlT2) )
            {
                iResult = 0;
            }
            else
            {
                iResult = -1;
            }

            goto exit_result;
        }
        else if ( ILIsEmpty(pidlT2) )
        {
            iResult = 1;
            goto exit_result;
        }

        // Both IDLISTs have more elements, therefore continue down them
        // binding to the next element in 1st IDLIST and then calling its
        // compare method.

        pidlT = ILClone(pidl1);

        if ( !pidlT )
            ExitGracefully(hres, E_OUTOFMEMORY, "Failed to clone IDLIST for binding");

        _ILNext(pidlT)->mkid.cb = 0;

        hres = BindToObject(pidlT, NULL, IID_IShellFolder, (void **)&pShellFolder);
        FailGracefully(hres, "Failed to get the IShellFolder implementation from pidl1");

        hres = pShellFolder->CompareIDs(lParam, pidlT1, pidlT2);
        Trace(TEXT("CompareIDs returned %08x"), ShortFromResult(hres));

        goto exit_gracefully;
    }

exit_result:

    Trace(TEXT("Exiting with iResult %d"), iResult);
    hres = ResultFromShort(iResult);

exit_gracefully:

    DoRelease(pShellFolder);
    DoILFree(pidlT);

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsFolder::CreateViewObject(HWND hwndOwner, REFIID riid, void **ppv)
{
    HRESULT hres;

    TraceEnter(TRACE_FOLDER, "CDsFolder::CreateViewObject");
    TraceGUID("View object requested", riid);

    TraceAssert(ppv);

    if ( IsEqualIID(riid, IID_IShellView) )
    {
        CSFV csfv;

        csfv.cbSize = SIZEOF(csfv);
        csfv.pshf = (LPSHELLFOLDER)this;
        csfv.psvOuter = NULL;
        csfv.pidl = _pidl;
        csfv.lEvents = 0;
        csfv.pfnCallback = _FolderCallBack;
        csfv.fvm = FVM_ICON;

        hres = SHCreateShellFolderViewEx(&csfv, (LPSHELLVIEW*)ppv);
        FailGracefully(hres, "SHCreateShellFolderViewEx failed");
    }
    else if ( IsEqualIID(riid, IID_IContextMenu) )
    {
        hres = CDefFolderMenu_Create2(_pidl, 
                                    hwndOwner,
                                    NULL, 0,
                                    (LPSHELLFOLDER)this,
                                    _FolderCFMCallback,
                                    NULL, 
                                    0, 
                                    (LPCONTEXTMENU*)ppv);        
    }
    else
    {
        ExitGracefully(hres, E_NOINTERFACE, "View object not supported");
    }

exit_gracefully:

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST* apidl, ULONG* rgfInOut)
{
    HRESULT hres;
    IDLISTDATA data;
    UINT i;

    TraceEnter(TRACE_FOLDER, "CDsFolder::GetAttributesOf");
    Trace(TEXT("cidl = %d"), cidl);

    if ( !rgfInOut )
        ExitGracefully(hres, E_INVALIDARG, "Bad rgfInOut value");

    *rgfInOut = 0;

    if ( cidl == 0 )
    {
        // get the namespace attributes we want to invoke for this namespace
        // checking the policy to see if we should be hiding this object

        *rgfInOut = NAMESPACE_ATTRIBUTES;

        if ( CheckDsPolicy(NULL, c_szHideNamespace) || !ShowDirectoryUI() )
        {
            TraceMsg("Namespace being marked as hidden");
            *rgfInOut |= SFGAO_NONENUMERATED;
        }           
    }
    else if ( cidl >= 1 )
    {
        // cidl > 1 && rgfInOut != NULL
        //  - walk all the IDLISTs we were given and get their attributes, and compose
        //    a mask containing the bits

        TraceAssert(apidl);
        TraceAssert(rgfInOut);

        for ( i = 0; i != cidl; i++ )
        {
            hres = UnpackIdList(apidl[i], DSIDL_HASCLASS, &data);
            FailGracefully(hres, "Failed to unpack IDLIST");

            *rgfInOut |= AttributesFromIdList(&data);
        }
    }

    hres = S_OK;

exit_gracefully:

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

HRESULT CDsFolder::_SetDispSpecOptions(IDataObject *pdo)
{
    HRESULT hres;
    CLIPFORMAT cfDsDispSpecOptions = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSDISPLAYSPECOPTIONS);
    FORMATETC fmte = {cfDsDispSpecOptions, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium = { TYMED_NULL, NULL, NULL };
    DSDISPLAYSPECOPTIONS *pddso = NULL;;
    DWORD cbStruct = SIZEOF(DSDISPLAYSPECOPTIONS);
    DWORD offsetStrings = SIZEOF(DSDISPLAYSPECOPTIONS);
    LPWSTR pAttribPrefix = _pAttribPrefix;

    TraceEnter(TRACE_FOLDER, "CDsFolder::_SetAttribPrefix");

    if ( !pAttribPrefix )
        pAttribPrefix = DS_PROP_SHELL_PREFIX;        

    cbStruct += StringByteSizeW(pAttribPrefix);
    cbStruct += StringByteSizeW(_pUserName);
    cbStruct += StringByteSizeW(_pPassword);
    cbStruct += StringByteSizeW(_pServer);

    // allocate and fill the structure

    hres = AllocStorageMedium(&fmte, &medium, cbStruct, (void **)&pddso);
    FailGracefully(hres, "Failed to allocate STGMEDIUM for data");

    pddso->dwSize = SIZEOF(DSDISPLAYSPECOPTIONS);
    pddso->dwFlags = DSDSOF_HASUSERANDSERVERINFO|DSDSOF_DSAVAILABLE;

    //pddso->offsetAttribPrefix = 0x0;
    //pddso->offsetUserName = 0x0;
    //pddso->offsetPassword = 0x0;
    //pddso->offsetServer = 0x0;
    //pddso->offsetServerConfigPath = 0x0;

    pddso->offsetAttribPrefix = offsetStrings;
    StringByteCopyW(pddso, offsetStrings, pAttribPrefix);
    offsetStrings += StringByteSizeW(pAttribPrefix);

    if ( _pUserName )
    {
        pddso->offsetUserName = offsetStrings;
        StringByteCopyW(pddso, offsetStrings, _pUserName);
        offsetStrings += StringByteSizeW(_pUserName);
    }

    if ( _pPassword )
    {
        pddso->offsetPassword = offsetStrings;
        StringByteCopyW(pddso, offsetStrings, _pPassword);
        offsetStrings += StringByteSizeW(_pPassword);
    }

    if ( _pServer )
    {
        pddso->offsetServer = offsetStrings;
        StringByteCopyW(pddso, offsetStrings, _pServer);
        offsetStrings += StringByteSizeW(_pServer);
    }

    // lets set into the IDataObject

    TraceMsg("Setting CF_DSDISPLAYSPECOPTIONS into the IDataObject");

    hres = pdo->SetData(&fmte, &medium, TRUE);
    FailGracefully(hres, "Failed to set the DSDISPLAYSPECOPTIONS into the IDataObject");

exit_gracefully:

    ReleaseStgMedium(&medium);

    TraceLeaveResult(hres);
}

STDMETHODIMP CDsFolder::GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST* aidl, REFIID riid, UINT* prgfReserved, void **ppv)
{
    HRESULT hres;
    LPITEMIDLIST pidlAbs = NULL;
    HKEY aKeys[UIKEY_MAX];
    LPITEMIDLIST pidl = NULL;
    INT i;

    TraceEnter(TRACE_FOLDER, "CDsFolder::GetUIObjectOf");
    TraceGUID("UI object requested", riid);

    TraceAssert(cidl > 0);
    TraceAssert(aidl);
    TraceAssert(ppv);

    ZeroMemory(aKeys, SIZEOF(aKeys));

    if ( IsEqualIID(riid, IID_IContextMenu) )
    {
        // IContextMenu requested, therefore lets use the shell to construct one for us,
        // then we can provide verbs on the namespace

        if ( cidl )
        {
            hres = GetKeysForIdList(*aidl, NULL, ARRAYSIZE(aKeys), aKeys);
            FailGracefully(hres, "Failed to get object keys");
        }

        hres = CDefFolderMenu_Create2(_pidl, hwndOwner,
                                    cidl, aidl,
                                    this,
                                    _FolderCFMCallback,
                                    ARRAYSIZE(aKeys), aKeys,
                                    (LPCONTEXTMENU*)ppv);

        FailGracefully(hres, "Failed to create context menu");
    }
    else if ( IsEqualIID(riid, IID_IDataObject) )
    {
        DSDATAOBJINIT ddoi = { 0 };
        IDataObject *pdo = NULL;

        ddoi.hwnd = hwndOwner;
        ddoi.pidlRoot = _pidl;
        ddoi.cbOffset = _cbOffset;
        ddoi.cidl = cidl;
        ddoi.aidl = aidl;
        ddoi.dwProviderAND = _dwProviderAND;
        ddoi.dwProviderXOR = _dwProviderXOR;

        hres = CDsDataObject_CreateInstance(&ddoi, IID_IDataObject, (void **)&pdo);
        FailGracefully(hres, "Failed to create the IDataObject");

        hres = _SetDispSpecOptions(pdo);
        FailGracefully(hres, "Failed to set the display spec options");

        hres = pdo->QueryInterface(riid, ppv);
        pdo->Release();
    }
    else if ( IsEqualIID(riid, IID_IExtractIcon) )
    {
        // IExtractIcon is used to support our object extraction, we can only cope
        // with extracting a single object at a time.  First however we must
        // build an IDLIST that represents the object we want to extract
        // and get the icon for it.

        if ( cidl != 1 )
            ExitGracefully(hres, E_FAIL, "Bad number of objects to get icon from");

        pidl = ILCombine(_pidl, *aidl);
        TraceAssert(pidl);

        Trace(TEXT("_pidl %08x, *aidl %08x, _cbOffset %d"), _pidl, *aidl, _cbOffset);

        if ( !pidl )
            ExitGracefully(hres, E_OUTOFMEMORY, "Failed to combine the IDLISTs");

        hres = _GetDsDisplaySpecifier();
        FailGracefully(hres, "Failed to get the IDsDisplaySpecifier object");

        CDsExtractIcon* pExtractIcon = new CDsExtractIcon(_pdds, _ILSkip(pidl, _cbOffset));
        TraceAssert(pExtractIcon);

        if ( !pExtractIcon )
            ExitGracefully(hres, E_OUTOFMEMORY, "Failed to create CDsExtractIcon");

        hres = pExtractIcon->QueryInterface(riid, ppv);
        pExtractIcon->Release();
    }
    else
    {
        ExitGracefully(hres, E_NOINTERFACE, "UI object not supported");
    }

exit_gracefully:

    TidyKeys(ARRAYSIZE(aKeys), aKeys);

    DoILFree(pidl);
        
    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET pName)
{
    HRESULT hres = E_FAIL;
    LPITEMIDLIST pidlFull = NULL;
    LPITEMIDLIST pidlRight = NULL;
    IDLISTDATA data =  {0};
    IShellFolder *psf = NULL;
    LPWSTR pPath = NULL;
    BSTR bstrName = NULL;
    USES_CONVERSION;
    DECLAREWAITCURSOR = GetCursor();

    TraceEnter(TRACE_FOLDER, "CDsFolder::GetDisplayName");

    pidlFull = ILCombine(_pidl, pidl);
    TraceAssert(pidlFull);
         
    if ( !pidlFull )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to combine PIDLs");

    // attempt to get the junction namespace, if not then we must convert the name,
    // if it is a junction the handle accordingly.

    hres = _GetJunctionSF(pidlFull, NULL, &data, &pidlRight, &psf);
    if ( SUCCEEDED(hres))
    {
        hres = _GetPathname();
        FailGracefully(hres, "Failed to get the IADsPathname interface");

        if ( (hres == S_FALSE) || !pidlRight || ILIsEmpty(pidlRight) )
        {
            TraceMsg("We don't have a trailing pidl, and its not a junction");

            // check to see if the name is the junction, and if we are requesting the 
            // for parsing && !in folder.  if that is valid then return the
            // junction name.

            if ( data.pUNC && (uFlags & SHGDN_FORPARSING) && !(uFlags & SHGDN_INFOLDER) )
            {
                TraceMsg("Return the full parsing name of the junction");

                // the junction is hit, pidlRight is NULL but the caller wants
                // the for parsing version of the name which is not-infolder (phew).

                hres = _StrRetFromString(pName, W2T(data.pUNC));
                FailGracefully(hres, "Failed to get parsing juntion name");
            }
            else
            {
                TraceMsg("Return path/name of DS object");

                // pre-populate the IADsPathname object with a path to be
                // worked on.

                hres = PathFromIdList(_ILSkip(pidlFull, _cbOffset), &pPath, _padp);
                FailGracefully(hres, "Failed to get the DS path");

                hres = _padp->Set(pPath, ADS_SETTYPE_FULL);
                FailGracefully(hres, "Failed to set the path of the name");

                if ( uFlags & SHGDN_INFOLDER )
                {
                    //
                    // get the in folder name, including removing all the crap if we can
                    //

                    if ( !(uFlags & SHGDN_FORPARSING) && data.pName )
                    {
                        hres = _StrRetFromString(pName, W2T(data.pName));
                    }
                    else
                    {
                        if ( !(uFlags & SHGDN_FORPARSING) )
                            _padp->SetDisplayType(ADS_DISPLAY_VALUE_ONLY);

                        hres = _padp->Retrieve(ADS_FORMAT_LEAF, &bstrName);
                        FailGracefully(hres, "Failed to get the pretty name");

                        hres = _StrRetFromString(pName, W2T(bstrName));
                    }
                }
                else
                {
                    //
                    // get the display name from the ADsPath we have - convert to conanical format
                    // for displaying in the title / wbe view
                    //

                    WCHAR szBuffer[MAX_PATH];

                    hres = GetDisplayNameFromADsPath(NULL, szBuffer, ARRAYSIZE(szBuffer), _padp, TRUE);
                    FailGracefully(hres, "Failed to get display name from ADsPath");

                    hres = _StrRetFromString(pName, W2T(szBuffer));
                }

                FailGracefully(hres, "Failed to convert to BSTR");
            }
        }
        else
        {
            TraceMsg("Requesting name from junction object");

            // the pidlRight is non-null (therefore we must delegate) so 
            // lets call the psf we have to get the DisplayName of the object.

            hres = psf->GetDisplayNameOf(pidlRight, uFlags, pName);
        }
    }

exit_gracefully:

    DoRelease(psf);

    DoILFree(pidlFull);
    DoILFree(pidlRight);

    LocalFreeStringW(&pPath);
    SysFreeString(bstrName);

    ResetWaitCursor();

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsFolder::SetNameOf(HWND hwndOwner, LPCITEMIDLIST pidl, LPCOLESTR pName, DWORD uFlags, LPITEMIDLIST * ppidlOut)
{
    TraceEnter(TRACE_FOLDER, "CDsFolder::SetDisplayNameOf");
    TraceLeaveResult(E_NOTIMPL);
}


/*-----------------------------------------------------------------------------
/ CDsExtractIcon
/----------------------------------------------------------------------------*/

CDsExtractIcon::CDsExtractIcon(IDsDisplaySpecifier *pdds, LPCITEMIDLIST pidl) :
    _pdds(pdds)
{
    TraceEnter(TRACE_UI, "CDsExtractIcon::CDsExtractIcon");
    
    _pidl = ILClone(pidl);
    _pdds->AddRef();
    
    TraceLeaveVoid();
}

CDsExtractIcon::~CDsExtractIcon()
{
    TraceEnter(TRACE_UI, "CDsExtractIcon::~CDsExtractIcon");

    DoILFree(_pidl);
    DoRelease(_pdds);

    TraceLeaveVoid();
}

// IUnknown bits

#undef CLASS_NAME
#define CLASS_NAME CDsExtractIcon
#include "unknown.inc"

STDMETHODIMP CDsExtractIcon::QueryInterface(REFIID riid, void **ppv)
{
    INTERFACES iface[] =
    {
        &IID_IExtractIcon, (LPEXTRACTICON)this,
    };

    return HandleQueryInterface(riid, ppv, iface, ARRAYSIZE(iface));
}


/*-----------------------------------------------------------------------------
/ IExtractIcon methods
/----------------------------------------------------------------------------*/

STDMETHODIMP CDsExtractIcon::GetIconLocation(UINT uFlags, LPTSTR szIconFile, UINT cchMax, INT* pIndex, UINT* pwFlags)
{
    HRESULT hres;
    LPTSTR pLocation = NULL;
    IDLISTDATA data;
    DWORD dwFlags = 0;
#ifndef UNICODE
    WCHAR szIconLocation[MAX_PATH];
#endif
    USES_CONVERSION;

    TraceEnter(TRACE_UI, "CDsExtractIcon::GetIconLocation");

    if ( !_pidl || ILIsEmpty(_pidl) )
        ExitGracefully(hres, E_INVALIDARG, "No IDLIST to extract icon from");

    // If we have an empty IDLIST then lets ensure that we get the the icon location
    // for the namespace root.  Otherwise unpack the IDLIST and get the icon location
    // from the cache.

    hres = UnpackIdList(ILFindLastID(_pidl), DSIDL_HASCLASS, &data);
    FailGracefully(hres, "Failed to unpack IDLIST");

    dwFlags |= (uFlags & GIL_OPENICON) ?  DSGIF_ISOPEN:DSGIF_ISNORMAL;

#ifdef UNICODE
    hres = _pdds->GetIconLocation(data.pObjectClass, dwFlags, szIconFile, cchMax, pIndex);
    FailGracefully(hres, "Failed to get the icon location");            
#else
    // when building ANSI we must call the DsGetIcon location to get the icon
    // we are interested in, this API is ANSI only and therefore we must
    // thunk the call back as required.

    hres = _pdds->GetIconLocation(data.pObjectClass, dwFlags, szIconLocation, ARRAYSIZE(szIconLocation), pIndex);
    FailGracefully(hres, "Failed to get the icon location");            

    if ( hres != S_FALSE )
    {
        WideCharToMultiByte(CP_ACP, 0, szIconLocation, -1, szIconFile, cchMax, 0, FALSE);
        Trace(TEXT("Thunked icon location: %s"), szIconFile);
    }
#endif    

exit_gracefully:

    LocalFreeString(&pLocation);

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsExtractIcon::Extract(LPCTSTR pszIconFile, UINT nIconIndex, HICON* pLargeIcon, HICON* pSmallIcon, UINT nIconSize)
{
    return S_FALSE;         // let the shell extract the image
}


/*----------------------------------------------------------------------------
/ PropertyUI iface for the query UI to invoke use with
/----------------------------------------------------------------------------*/

// IUnknown handlers

#undef CLASS_NAME
#define CLASS_NAME CDsFolderProperties
#include "unknown.inc"

STDMETHODIMP CDsFolderProperties::QueryInterface( REFIID riid, void **ppv)
{
    INTERFACES iface[] =
    {
        &IID_IDsFolderProperties, (IDsFolderProperties*)this,
    };

   return HandleQueryInterface(riid, ppv, iface, ARRAYSIZE(iface));
}

//
// handle create instance of the CDsFolder object
//

STDAPI CDsFolderProperties_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CDsFolderProperties *pdfp = new CDsFolderProperties();
    if ( !pdfp )
        return E_OUTOFMEMORY;

    HRESULT hres = pdfp->QueryInterface(IID_IUnknown, (void **)ppunk);
    pdfp->Release();
    return hres;
}


/*----------------------------------------------------------------------------
/ IDsFolderProperties
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ IDsFolderProperties::ShowProperties
/ -----------------------------------
/   Display the property page set that relates to the given IDataObject, it is
/   assumed that the caller is passing a IDataObject containing a valid DS
/   clipboard data blob.
/
/ In:
/   hwndParent = parent window for the dialog
/   pDataObject -> data object containing the sleection data
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
STDMETHODIMP CDsFolderProperties::ShowProperties(HWND hwndParent, IDataObject *pDataObject)
{
    HRESULT hres;

    TraceEnter(TRACE_API, "CDsFolderProperties::ShowProperties");

    if ( !pDataObject )  
        ExitGracefully(hres, E_INVALIDARG, "No pDataObject given");

    CoInitialize(NULL);            // ensure we have COM

    hres = ShowObjectProperties(hwndParent, pDataObject);
    FailGracefully(hres, "Failed to open property pages");

    // hres = S_OK;               // success

exit_gracefully:

    TraceLeaveResult(hres);        
}
