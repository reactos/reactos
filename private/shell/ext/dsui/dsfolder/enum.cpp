/*----------------------------------------------------------------------------
/ Title;
/   enum.cpp
/
/ Authors;
/   David De Vorchik (daviddv)
/
/ Notes;
/   Enumerates the DS for the shell
/----------------------------------------------------------------------------*/
#include "pch.h"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ Constants and other helpers
/----------------------------------------------------------------------------*/

class CDsEnum : public IEnumIDList, CUnknown
{
    private:
        BOOL              _fFirstEnum:1;             // 1st enumeration
        LPITEMIDLIST      _pidl;                     // namespace point to enumerate
        HWND              _hwndOwner;                // used for MsgBox etc
        DWORD             _grfFlags;                 // for masking out items
        BSTR              _bstrSearchRoot;           // container to search from (scope)=
        ADS_SEARCH_HANDLE _hSearch;                  // search handle
        LPDOMAINTREE      _pDomainTree;              // current domain tree
        LPDOMAINDESC      _pDomainDesc;              // current domain node

        IDsBrowseDomainTree* _pdbdt;                 // domain tree browser
        IDirectorySearch*    _pds;                   // IDSSearch interface for the results
        IMalloc*             _pm;                    // IMalloc used for delegate folder support
        IADsPathname*        _pap;                   // IADsPathname interface

        IDsDisplaySpecifier* _pdds;                  // IDsDisplaySpecifier interface
        HRESULT _GetDsDisplaySpecifier(void);

    public:
        CDsEnum(LPCITEMIDLIST pidl, HWND hwndOwner, DWORD grfFlags, IMalloc *pm);
        ~CDsEnum();

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IEnumIDList
        STDMETHODIMP Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
        STDMETHODIMP Skip(ULONG celt);
        STDMETHODIMP Reset();
        STDMETHODIMP Clone(LPENUMIDLIST *ppenum);
};

//
// Volumes are shortcuts
//

#define VOLUMES_ARE_SHORTCUTS 0

//
// Page size used for paging the result sets (better performance)
//

#define PAGE_SIZE       32
#define LDAP_PREFIX     L"LDAP://"

//
// Query issued to get all the un-hidden objects 
//

WCHAR c_szQuery[] = L"(&(objectClass=*)(!showInAdvancedViewOnly=TRUE))";

//
// Properties cached back
//

LPWSTR pProperties[] = 
{
    L"name",
    L"objectClass",
    L"ADsPath",
    L"UNCname",
};


/*-----------------------------------------------------------------------------
/ CDsEnum
/----------------------------------------------------------------------------*/

CDsEnum::CDsEnum(LPCITEMIDLIST pidl, HWND hwndOwner, DWORD grfFlags, IMalloc* pm) :
    _fFirstEnum(TRUE),
    _hwndOwner(hwndOwner),
    _grfFlags(grfFlags),
    _bstrSearchRoot(NULL),
    _pDomainTree(NULL),
    _pDomainDesc(NULL),
    _pdbdt(NULL),
    _pds(NULL),
    _pap(NULL),
    _pdds(NULL),
    _pm(pm)
{
    TraceEnter(TRACE_ENUM, "CDsEnum::CDsEnum");

    CoInitialize(NULL);

    _pidl = ILClone(pidl);
    TraceAssert(_pidl);

    if ( _pm )
        _pm->AddRef();

    TraceLeave();
}

CDsEnum::~CDsEnum()
{
    if ( _pidl )
        ILFree(_pidl);

    SysFreeString(_bstrSearchRoot);
    _bstrSearchRoot = NULL;

    if ( _hSearch && _pds )
        _pds->CloseSearchHandle(_hSearch);

    if ( _pdbdt )
        _pdbdt->FreeDomains(&_pDomainTree);

    DoRelease(_pap);
    DoRelease(_pdbdt);
    DoRelease(_pds);
    DoRelease(_pm);
    DoRelease(_pdds);
}

// IUnknown bits

#undef CLASS_NAME
#define CLASS_NAME CDsEnum
#include "unknown.inc"

STDMETHODIMP CDsEnum::QueryInterface(REFIID riid, void ** ppvObject)
{
    INTERFACES iface[] =
    {
        &IID_IEnumIDList, (LPENUMIDLIST)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}

//
// our enumerator object
//

HRESULT CDsEnum_CreateInstance(LPCITEMIDLIST pidl, HWND hwndOwner, DWORD grfFlags, IMalloc* pm, IEnumIDList **ppeidl)
{
    CDsEnum *pDsEnum = new CDsEnum(pidl, hwndOwner, grfFlags, pm);
    if ( !pDsEnum )
        return E_OUTOFMEMORY;

    *ppeidl = SAFECAST(pDsEnum, IEnumIDList*);
    return S_OK;
}

//
// helper functions
//

HRESULT CDsEnum::_GetDsDisplaySpecifier(void)
{
    HRESULT hres = S_OK;

    TraceEnter(TRACE_FOLDER, "CDsEnum::_GetDsDisplaySpecifier");

    if ( !_pdds )
    {
        TraceMsg("CoCreating the object");
        hres = CoCreateInstance(CLSID_DsDisplaySpecifier, NULL, CLSCTX_INPROC_SERVER, IID_IDsDisplaySpecifier, (void **)&_pdds);
    }

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ IEnumIDList methods
/----------------------------------------------------------------------------*/

STDMETHODIMP CDsEnum::Next(ULONG celt, LPITEMIDLIST* rgelt, ULONG* pceltFetched)
{
    HRESULT hres = S_FALSE;
    ADS_SEARCH_COLUMN column;
    ADS_SEARCHPREF_INFO prefInfo[3];
    BSTR bstrDomain = NULL;
    LPWSTR pName = NULL;
    LPWSTR pPath = NULL;
    LPWSTR pObjectClass = NULL;
    LPWSTR pUNC = NULL;
    LPWSTR pSearchRoot = NULL;
    ULONG index = 0;
    INT i;
    USES_CONVERSION;

    TraceEnter(TRACE_ENUM, "CDsEnum::Next");

    if ( pceltFetched )
        *pceltFetched = 0;

    // Get the IADsPathname interface if we don't have one yet, we will need it

    if ( !_pap )
    {
        hres = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER, IID_IADsPathname, (void **)&_pap);
        FailGracefully(hres, "Failed to get the IADsPathname interface");
    }

    // Get the search root if we need it, this is used to generate relitive
    // IDLISTs within the namespace.

    if ( !ILIsEmpty(_pidl) && !_bstrSearchRoot )
    {
        hres = PathFromIdList(_pidl, &pSearchRoot, NULL);
        FailGracefully(hres, "Failed to get the search root from the IDLIST");

        hres = _pap->Set(pSearchRoot, ADS_SETTYPE_FULL);
        FailGracefully(hres, "Failed to set path");

        hres = _pap->Retrieve(ADS_FORMAT_WINDOWS, &_bstrSearchRoot);
        FailGracefully(hres, "Failed to retrieve the domain path");
    }

    // have we checked the domain list yet?

    if ( !_pDomainTree )
    {
        TraceMsg("Fetching the domain trust list");

        hres = CoCreateInstance(CLSID_DsDomainTreeBrowser, NULL, CLSCTX_INPROC_SERVER,  
                                        IID_IDsBrowseDomainTree, (void **)&_pdbdt);
        FailGracefully(hres, "Failed to get domain tree iface");

        hres = _pdbdt->GetDomains(&_pDomainTree, DBDTF_RETURNFQDN);
        FailGracefully(hres, "Failed when getting domain tree information");  

#ifdef DSUI_DEBUG
        Trace(TEXT("Domain tree entries %d"), _pDomainTree->dwCount);
        for ( index = 0; index != _pDomainTree->dwCount; index++ )
        {
            Trace(TEXT("(%d) Domain: %s"), index, W2T(_pDomainTree->aDomains[index].pszNCName));
            Trace(TEXT("(%d) Child object pointer %08x"), _pDomainTree->aDomains[index].pdChildList);
        }
#endif

        // now try and work out where in the domain tree we are, if the pidl is empty
        // then we can just use the root domain (and its siblings, otherwise 
        // we must walk the names trying to find a suitable parent).

        if ( ILIsEmpty(_pidl) )
        {
            TraceMsg("At the root of the tree, therefore defaulting to first node");
            _pDomainDesc = _pDomainTree->aDomains;
        }
     else
        {
            // Walk all the elements in the domain structure, when we hit one then
            // break the loop setting the _pDomainDesc pointer to the children
            // of that point.

            Trace(TEXT("Searching for: %s"), W2T(_bstrSearchRoot));

            for ( index = 0; index != _pDomainTree->dwCount; index++ )
            {
                LocalFreeStringW(&pPath);

                SysFreeString(bstrDomain);
                bstrDomain = NULL;

                hres = LocalAllocStringLenW(&pPath, lstrlenW(LDAP_PREFIX)+lstrlenW(_pDomainTree->aDomains[index].pszNCName));
                FailGracefully(hres, "Failed to allocate ADsPath");

                StrCpyW(pPath, LDAP_PREFIX);
                StrCatW(pPath, _pDomainTree->aDomains[index].pszNCName);

                Trace(TEXT("LDAP foramtted domain: %s"), W2T(pPath));

                hres = _pap->Set(pPath, ADS_SETTYPE_FULL);
                FailGracefully(hres, "Failed to set path");

                hres = _pap->Retrieve(ADS_FORMAT_WINDOWS, &bstrDomain);
                FailGracefully(hres, "Failed to retrieve the domain path");

                Trace(TEXT("Domain name (windows): %s"), W2T(bstrDomain));
                
                if ( !StrCmpIW(_bstrSearchRoot, bstrDomain) )
                {                   
                    _pDomainDesc = _pDomainTree->aDomains[index].pdChildList;
                    break;
                }
            }
        }                    
    }

    // when at the root of the domain tree we handle things differently, instead of issuing a search
    // we just give back the top level domain objects. 

    if ( !ILIsEmpty(_pidl) && !_pds )
    {
        Trace(TEXT("Requesting IDirectorySearch from: %s"), W2T(_bstrSearchRoot));

        hres = ADsOpenObject(_bstrSearchRoot, NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IDirectorySearch, (void **)&_pds);
        FailGracefully(hres, "Failed when getting IDirectorySearch on search root");

        TraceMsg("We have a pDsSearch iface");
           
        // Set the query prefernece to single level scope, and async retrevial rather
        // than waiting for all objects
        
        prefInfo[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
        prefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
        prefInfo[0].vValue.Integer = ADS_SCOPE_ONELEVEL;

        prefInfo[1].dwSearchPref = ADS_SEARCHPREF_ASYNCHRONOUS;
        prefInfo[1].vValue.dwType = ADSTYPE_BOOLEAN;
        prefInfo[1].vValue.Boolean = TRUE;

        prefInfo[2].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;         // paged results
        prefInfo[2].vValue.dwType = ADSTYPE_INTEGER;
        prefInfo[2].vValue.Integer = PAGE_SIZE;

        TraceMsg("Setting search preferences");

        hres = _pds->SetSearchPreference(prefInfo, ARRAYSIZE(prefInfo));
        FailGracefully(hres, "Failed to set search preferences");

        TraceMsg("Executing search");

        hres = _pds->ExecuteSearch(c_szQuery, pProperties, ARRAYSIZE(pProperties), &_hSearch);
        FailGracefully(hres, "Failed in ExecuteSearch");
    }

    // By this point we have the search interface so we can start packaging the
    // results into senisble IDLIST structures to be returned to the caller.

    Trace(TEXT("Start to enumerate, returning %d items"), celt);

    for ( index = 0 ; celt > 0 ; )
    {
        LPITEMIDLIST pidl = NULL;
        BOOL fIsContainer;

        LocalFreeStringW(&pName);
        LocalFreeStringW(&pObjectClass);
        LocalFreeStringW(&pPath);
        LocalFreeStringW(&pUNC);

        if ( _pDomainDesc )
        {
            // we have a domain description pointer, therefore lets make an ADSI path from it
            // and construct an IDLIST by binding.  having made the IDLIST we can then
            // check for a container and discard as needed.

            hres = LocalAllocStringLenW(&pPath, lstrlenW(LDAP_PREFIX)+lstrlenW(_pDomainDesc->pszNCName));
            FailGracefully(hres, "Failed to allocate ADsPath");

            StrCpyW(pPath, LDAP_PREFIX);
            StrCatW(pPath, _pDomainDesc->pszNCName);

            hres = LocalAllocStringW(&pObjectClass, _pDomainDesc->pszObjectClass);
            FailGracefully(hres, "Failed to allocate objectClass string");

            _pDomainDesc = _pDomainDesc->pdNextSibling;
        }
        else if ( _hSearch )
        {    
            // issuing a search therefore lets pick up the next row of results, if that
            // fails then lets return S_FALSE.

            TraceMsg("Calling IDirectorySearch::GetNextRow");

            hres = _pds->GetNextRow(_hSearch);
            FailGracefully(hres, "Failed in GetNextRow");

            if ( hres == S_ADS_NOMORE_ROWS )
                ExitGracefully(hres, S_FALSE, "No more rows left in query");

            // Get the columns for each of the properties we are interested in, if we failed
            // to get any of the base properties for the object then lets just skip
            // this entry as we cannot build a valid IDLIST for it.  The properties that
            // we request should be present on all objects.

            if ( FAILED(_pds->GetColumn(_hSearch, pProperties[0], &column)) )
                continue;

            hres = StringFromSearchColumn(&column, &pName);
            _pds->FreeColumn(&column);
            FailGracefully(hres, "Failed to get the name from the property");

            if ( FAILED(_pds->GetColumn(_hSearch, pProperties[1], &column)) )
                continue;

            hres = ObjectClassFromSearchColumn(&column, &pObjectClass);
            _pds->FreeColumn(&column);
            FailGracefully(hres, "Failed to get the object class from the property");

            if ( FAILED(_pds->GetColumn(_hSearch, pProperties[2], &column)) )
                continue;

            hres = StringFromSearchColumn(&column, &pPath);
            _pds->FreeColumn(&column);
            FailGracefully(hres, "Failed to convert the ADsPath column to a string");

#if VOLUMES_ARE_SHORTCUTS
            if ( SUCCEEDED(_pds->GetColumn(_hSearch, pProperties[3], &column)) )
            {
                StringFromSearchColumn(&column, &pUNC);
                _pds->FreeColumn(&column);
            }
#endif
        }
        else
        {
            // neither a search or domain description, therefore just bail with
            // S_FALSE so we don't get called again.

            ExitGracefully(hres, S_FALSE, "Nothing left to enumerate");
        }

        // Having extracted the properties we are interested in lets convert
        // this to a suitable IDLIST.  First hit the cache and get information
        // about the object, then having done that we can filter the object (containers only, 
        // objects only).

        Trace(TEXT("pName: %s"), pName ? W2T(pName):TEXT("<none>"));            
        Trace(TEXT("pPath: %s"), W2T(pPath));
        Trace(TEXT("pObjectClass: %s"), W2T(pObjectClass));
        Trace(TEXT("pUNC: %s"), pUNC ? W2T(pUNC):TEXT("<not defined>"));

        TraceMsg("Reading container flag for class");

#if VOLUMES_ARE_SHORTCUTS
        if ( pUNC )
        {
            // the object has a UNC, therefore lets check to see if this is something
            // we should honor as a junction or not.

            if ( lstrcmpiW(pObjectClass, L"volume") )
                LocalFreeStringW(&pUNC);

            fIsContainer = (pUNC != NULL);
        }
        else
#endif
        {
            hres = _GetDsDisplaySpecifier();
            FailGracefully(hres, "Failed to get the IDsDisplaySpecifier object");

            fIsContainer = _pdds->IsClassContainer(pObjectClass, pPath, 0x0);
        }

        TraceMsg("Filtering result");

        if ( ( fIsContainer && !(_grfFlags & SHCONTF_FOLDERS)) ||
                (!fIsContainer && !(_grfFlags & SHCONTF_NONFOLDERS)) )
        {          
            TraceMsg("Doesn't match the filter supplied, therefore ignoring");      
            continue;
        }

        TraceMsg("Constructing the IDLIST");

        hres = CreateIdListFromPath(&pidl, pName, pPath, pObjectClass, pUNC, _pm, _pdds);
        FailGracefully(hres, "Failed to pack the IDLIST");

        TraceMsg("Writing IDLIST into the array");

        rgelt[index++] = pidl;
        celt--;
    }

    hres = S_OK;                      // success

exit_gracefully:

    Trace(TEXT("index %d"), index);

    if ( pceltFetched )
        *pceltFetched += index;

    LocalFreeStringW(&pName);
    LocalFreeStringW(&pObjectClass);
    LocalFreeStringW(&pPath);
    LocalFreeStringW(&pUNC);

    LocalFreeStringW(&pSearchRoot);

    SysFreeString(bstrDomain);

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsEnum::Skip(ULONG celt)
{
    TraceEnter(TRACE_ENUM, "CDsEnum::Skip");
    TraceLeaveResult(E_NOTIMPL);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsEnum::Reset()
{
    TraceEnter(TRACE_ENUM, "CDsEnum::Next");
    TraceLeaveResult(E_NOTIMPL);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsEnum::Clone(LPENUMIDLIST* ppenum)
{
    TraceEnter(TRACE_ENUM, "CDsEnum::Clone");
    TraceLeaveResult(E_NOTIMPL);
}
