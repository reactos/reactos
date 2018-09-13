#include "pch.h"
#include "helpids.h"
#pragma hdrstop

/*-----------------------------------------------------------------------------
/ Constants and other helpers
/----------------------------------------------------------------------------*/

//                                                                                    
// Page size used for paging the result sets (better performance)
//
#define PAGE_SIZE                  128

WCHAR c_szQueryNormal[]            = L"(&(objectClass=*)(&(!hideFromAB=TRUE)(!showInAdvancedViewOnly=TRUE)))";
WCHAR c_szQueryAll[]               = L"(objectClass=*)";
WCHAR c_szShowInAdvancedViewOnly[] = L"showInAdvancedViewOnly";
WCHAR c_szHideFromAB[]             = L"hideFromAB";
WCHAR c_szObjectClass[]            = L"objectClass";
WCHAR c_szADsPath[]                = L"ADsPath";
WCHAR c_szName[]                   = L"name";
WCHAR c_szRDN[]                    = L"rdn";
WCHAR c_szLDAPPrefix[]             = L"LDAP://";


/*-----------------------------------------------------------------------------
/ CBrowseDlg class definition
/----------------------------------------------------------------------------*/

class CBrowseDlg
{
private:
    
    // a UNICODE version of the structure
    DSBROWSEINFOW _bi;         

    // an IADsPathname object for usto use
    IADsPathname* _pPathCracker;
    
    // server being referenced (cracked out of the pszRoot path);
    LPWSTR        _pServer;          

    // browse information (initialized during startup)
    WCHAR         _szFilter[INTERNET_MAX_URL_LENGTH];
    WCHAR         _szNameAttribute[MAX_PATH];

public:
    CBrowseDlg(PDSBROWSEINFOW pbi);
    ~CBrowseDlg();

private:
    HRESULT GetPathCracker(void);
    LPCWSTR GetSelectedPath(HWND hDlg) const;
    LPCWSTR GetSelectedObjectClass(HWND hDlg) const;
    int     SetSelectedPath(HWND hDlg, LPCWSTR pszADsPath);
    HRESULT BuildNodeString(LPCWSTR pszADsPath,
                            LPCWSTR pszClass,
                            LPWSTR *ppszResult);
    HRESULT ExpandNode(LPWSTR    pszRootPath,
                       HWND      hwndTree,
                       HTREEITEM hParentItem);
    HRESULT ExpandNode(IADs     *pRootObject,
                       HWND      hwndTree,
                       HTREEITEM hParentItem);
    HRESULT EnumerateNode(IADsContainer *pDsContainer,
                          HWND          hwndTree,
                          HTREEITEM     hParentItem,
                          LPDWORD       pdwAdded);
    HRESULT SearchNode(IDirectorySearch   *pDsSearch,
                       HWND         hwndTree,
                       HTREEITEM    hParentItem,
                       LPDWORD      pdwAdded);
    HRESULT AddTreeNode(IADs *pDsObject,
                        LPCWSTR pObjectPath,
                        HWND hwndTree,
                        HTREEITEM hParentItem,
                        HTREEITEM* phitem);    
    HRESULT AddTreeNode(LPCWSTR pszPath,
                        LPCWSTR pszClass,
                        LPCWSTR pszName,
                        HWND hwndTree,
                        HTREEITEM hParentItem,
                        HTREEITEM* phitem);
    HRESULT AddTreeNode(LPDOMAINDESC pDomainDesc,
                        HWND hwndTree,
                        HTREEITEM hParentItem,
                        HTREEITEM* phitem);
    BOOL InitDlg(HWND hDlg);
    BOOL OnNotify(HWND hDlg, int idCtrl, LPNMHDR pnmh);
    void OnOK(HWND hDlg);
    BOOL DlgProc(HWND, UINT, WPARAM, LPARAM);

    static INT_PTR CALLBACK _DlgProc(HWND, UINT, WPARAM, LPARAM);

    friend STDMETHODIMP_(int) DsBrowseForContainerW(PDSBROWSEINFOW pbi);
};
typedef CBrowseDlg *PBROWSEDLG;


/*-----------------------------------------------------------------------------
/ Helper function to create an IADsPathname "path cracker" object
/----------------------------------------------------------------------------*/

HRESULT
CreatePathCracker(IADsPathname **ppPath)
{
    HRESULT hres;

    TraceEnter(TRACE_BROWSE, "CreatePathCracker");
    TraceAssert(ppPath != NULL);

    hres =  CoCreateInstance(CLSID_Pathname,
                             NULL,
                             CLSCTX_INPROC_SERVER,
                             IID_IADsPathname,
                             (LPVOID*)ppPath);

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ DsBrowseForContainer API implementation
/----------------------------------------------------------------------------*/

STDMETHODIMP_(int)
DsBrowseForContainerA(PDSBROWSEINFOA pbi)
{
    int nResult = -1;
    DSBROWSEINFOW bi = {0};
    USES_CONVERSION;

    TraceEnter(TRACE_BROWSE, "DsBrowseForContainerA");

    if (pbi == NULL ||
        IsBadReadPtr(pbi, SIZEOF(DWORD)) ||
        pbi->cbStruct < FIELD_OFFSET(DSBROWSEINFOA, dwReturnFormat) ||
        IsBadReadPtr(pbi, pbi->cbStruct) ||
        pbi->pszPath == NULL ||
        pbi->cchPath == 0 ||
        IsBadWritePtr(pbi->pszPath, pbi->cchPath))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        TraceLeaveValue(nResult);
    }

    CopyMemory(&bi, pbi, min(SIZEOF(bi), pbi->cbStruct));

    // Thunk the ANSI strings to UNICODE as required, then 
    // call the UNICODE version of this API

    if ( pbi->pszTitle )
        bi.pszTitle = A2CW(pbi->pszTitle);

    if ( pbi->pszCaption )
        bi.pszCaption = A2CW(pbi->pszCaption);

    nResult = DsBrowseForContainerW(&bi);

    TraceLeaveValue(nResult);
}

STDMETHODIMP_(int)
DsBrowseForContainerW(PDSBROWSEINFOW pbi)
{
    HRESULT hresCoInit;
    int nResult = -1;

    TraceEnter(TRACE_BROWSE, "DsBrowseForContainerW");

    hresCoInit = CoInitialize(NULL);
    FailGracefully(hresCoInit, "Failed when calling CoInitialize");

    if (pbi == NULL ||
        IsBadReadPtr(pbi, SIZEOF(DWORD)) ||
        pbi->cbStruct < FIELD_OFFSET(DSBROWSEINFOW, dwReturnFormat) ||
        IsBadReadPtr(pbi, pbi->cbStruct) ||
        pbi->pszPath == NULL ||
        pbi->cchPath == 0 ||
        IsBadWritePtr(pbi->pszPath, pbi->cchPath * SIZEOF(WCHAR)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto exit_gracefully;
    }

    if (pbi->pszRoot == NULL)
        pbi->pszRoot = L"LDAP:";

    if (pbi->pszRoot != NULL)
    {
        CBrowseDlg dlg(pbi);
        nResult = (int)DialogBoxParam(GLOBAL_HINSTANCE,
                                      MAKEINTRESOURCE(IDD_DSBROWSEFORCONTAINER),
                                      pbi->hwndOwner,
                                      CBrowseDlg::_DlgProc,
                                      (LPARAM)&dlg);
    }

exit_gracefully:

    if ( SUCCEEDED(hresCoInit) )
        CoUninitialize();

    TraceLeaveValue(nResult);
}


/*-----------------------------------------------------------------------------
/ CBrowseDlg class implementation
/----------------------------------------------------------------------------*/

CBrowseDlg::CBrowseDlg(PDSBROWSEINFOW pbi) : 
    _pPathCracker(NULL),
    _pServer(NULL)
{
    USES_CONVERSION;

    TraceEnter(TRACE_BROWSE, "CBrowseDlg::CBrowseDlg");

    TraceAssert(pbi != NULL && !IsBadReadPtr(pbi, pbi->cbStruct));
    TraceAssert(pbi->pszPath != NULL &&
                pbi->cchPath != 0 &&
                !IsBadWritePtr(pbi->pszPath, pbi->cchPath * SIZEOF(WCHAR)));

    CopyMemory(&_bi, pbi, min(SIZEOF(_bi), pbi->cbStruct));

    //
    // initialize the fitler we are using
    //

    StrCpyW(_szFilter, (_bi.dwFlags & DSBI_INCLUDEHIDDEN) ? c_szQueryAll:c_szQueryNormal);
    Trace(TEXT("_szFilter: %s"), W2T(_szFilter));

    _szNameAttribute[0] = L'\0';

    TraceLeaveVoid();
}


CBrowseDlg::~CBrowseDlg()
{
    LocalFreeStringW(&_pServer);
    DoRelease(_pPathCracker);
}


HRESULT
CBrowseDlg::GetPathCracker(void)
{
    HRESULT hres = S_OK;

    if ( _pPathCracker == NULL )
        hres = CreatePathCracker(&_pPathCracker);

    if ( SUCCEEDED(hres) && _pPathCracker )
        _pPathCracker->SetDisplayType(ADS_DISPLAY_FULL);       // ensure we are set to full

    return hres;
}


LPCWSTR
CBrowseDlg::GetSelectedPath(HWND hDlg) const
{
    LPCWSTR pszResult = NULL;
    HWND hwndTree;
    HTREEITEM hti = NULL;

    TraceEnter(TRACE_BROWSE, "CBrowseDlg::GetSelectedPath");
    TraceAssert(hDlg != NULL);

    hwndTree = GetDlgItem(hDlg, DSBID_CONTAINERLIST);

    if (hwndTree != NULL)
    {
        hti = TreeView_GetSelection(hwndTree);
    }

    if (hti != NULL)
    {
        TV_ITEM tvi;
        tvi.mask = TVIF_PARAM | TVIF_HANDLE;
        tvi.hItem = hti;

        if ( TreeView_GetItem(hwndTree, &tvi) )
        {
            pszResult = (LPCWSTR)tvi.lParam;
        }
    }

    TraceLeaveValue(pszResult);
}


LPCWSTR
CBrowseDlg::GetSelectedObjectClass(HWND hDlg) const
{
    LPCWSTR pszResult = NULL;
    HWND hwndTree;
    HTREEITEM hti = NULL;

    TraceEnter(TRACE_BROWSE, "CBrowseDlg::GetSelectedObjectClass");
    TraceAssert(hDlg != NULL);

    hwndTree = GetDlgItem(hDlg, DSBID_CONTAINERLIST);

    if (hwndTree != NULL)
        hti = TreeView_GetSelection(hwndTree);

    if (hti != NULL)
    {
        TV_ITEM tvi;
        tvi.mask = TVIF_PARAM | TVIF_HANDLE;
        tvi.hItem = hti;

        if (TreeView_GetItem(hwndTree, &tvi))
        {
            LPCWSTR pszPathAndClass = (LPWSTR)tvi.lParam;
            pszResult = (LPCWSTR)ByteOffset(tvi.lParam, StringByteSizeW(pszPathAndClass));
        }
    }

    TraceLeaveValue(pszResult);
}


int
CBrowseDlg::SetSelectedPath(HWND hDlg, LPCWSTR pszADsPath)
{
    int nResult = -1;
    HRESULT hres;
    BSTR bstrPath = NULL;
    UINT nPathLength;
    HWND hwndTree;
    HTREEITEM hItem;
    TV_ITEM tvi;
    tvi.mask = TVIF_HANDLE | TVIF_PARAM;
    USES_CONVERSION;

    TraceEnter(TRACE_BROWSE, "CBrowseDlg::SetSelectedPath");

    // Run the path through a path cracker to get a known format
    hres = GetPathCracker();
    FailGracefully(hres, "Unable to create ADsPathname object");

    hres = _pPathCracker->Set((LPWSTR)pszADsPath, ADS_SETTYPE_FULL);
    FailGracefully(hres, "Unable to parse path");

    hres = _pPathCracker->Retrieve(ADS_FORMAT_WINDOWS, &bstrPath);
    FailGracefully(hres, "Unable to build ADS Windows path");

    nPathLength = lstrlenW(bstrPath);
    Trace(TEXT("bstrPath: %s (%d)"), W2T(bstrPath), nPathLength);

    hwndTree = GetDlgItem(hDlg, DSBID_CONTAINERLIST);
    hItem = TreeView_GetChild(hwndTree, NULL);

    while (hItem != NULL)
    {
        LPWSTR pszCompare;
        UINT nCompareLength;

        tvi.hItem = hItem;
        TreeView_GetItem(hwndTree, &tvi);

        pszCompare = (LPWSTR)tvi.lParam;
        nCompareLength = lstrlenW(pszCompare);

        Trace(TEXT("Comparing against: %s"), W2T(pszCompare));

        // Does bstrPath contain pszCompare?
        if (2 == CompareStringW(LOCALE_SYSTEM_DEFAULT,
                                NORM_IGNORECASE,
                                bstrPath,
                                min(nCompareLength, nPathLength),
                                pszCompare,
                                nCompareLength))
        {
            TraceMsg("Current item, contains bstrPath");
            
            TreeView_SelectItem(hwndTree, hItem);

            if (nCompareLength == nPathLength)
            {
                TraceMsg("... and it was an exact match");
                TreeView_Expand(hwndTree, hItem, TVE_EXPAND);
                nResult = 0;
                break;
            }
            else
            {
                TraceMsg("... checking children for match");
                TreeView_Expand(hwndTree, hItem, TVE_EXPAND);
                hItem = TreeView_GetChild(hwndTree, hItem);
            }
        }
        else
        {
            TraceMsg("Checking sibling as no match found");
            hItem = TreeView_GetNextSibling(hwndTree, hItem);
        }
    }

exit_gracefully:

    if (bstrPath != NULL)
        SysFreeString(bstrPath);

    TraceLeaveValue(nResult);
}


HRESULT
CBrowseDlg::BuildNodeString(LPCWSTR pszADsPath,
                            LPCWSTR pszClass,
                            LPWSTR *ppszResult)
{
    HRESULT hres = S_OK;
    INT cbPath, cbClass;

    TraceEnter(TRACE_BROWSE, "CBrowseDlg::BuildNodeString");
    TraceAssert(pszADsPath != NULL);
    TraceAssert(pszClass != NULL);
    TraceAssert(ppszResult != NULL);

    cbPath = StringByteSizeW(pszADsPath);
    cbClass = StringByteSizeW(pszClass);

    *ppszResult = (LPWSTR)LocalAlloc(LPTR, cbPath+cbClass);

    if ( !*ppszResult )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate node string");

    CopyMemory(*ppszResult, pszADsPath, cbPath);
    CopyMemory(ByteOffset(*ppszResult, cbPath), pszClass, cbClass);

exit_gracefully:

    TraceLeaveResult(hres);
}


HRESULT
CBrowseDlg::ExpandNode(LPWSTR    pszRootPath,
                       HWND      hwndTree,
                       HTREEITEM hParentItem)
{
    HRESULT hres = S_OK;
    IADs *pRootObject = NULL;
    HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    TraceEnter(TRACE_BROWSE, "CBrowseDlg::ExpandNode");
    TraceAssert(pszRootPath != NULL);
    TraceAssert(hwndTree != NULL);

    Trace(TEXT("Scope is: %s"), pszRootPath);

    hres = ADsOpenObject(pszRootPath, 
                         (_bi.dwFlags & DSBI_HASCREDENTIALS) ? (LPWSTR)_bi.pUserName:NULL,
                         (_bi.dwFlags & DSBI_HASCREDENTIALS) ? (LPWSTR)_bi.pPassword:NULL,
                         (_bi.dwFlags & DSBI_SIMPLEAUTHENTICATE) ? 0:ADS_SECURE_AUTHENTICATION, 
                         IID_IADs, (LPVOID*)&pRootObject);
    if (SUCCEEDED(hres))
    {
        hres = ExpandNode(pRootObject, hwndTree, hParentItem);
        DoRelease(pRootObject);
    }

    SetCursor(hcur);
    TraceLeaveResult(hres);
}


HRESULT
CBrowseDlg::ExpandNode(IADs     *pRootObject,
                       HWND      hwndTree,
                       HTREEITEM hParentItem)
{
    HRESULT hres = S_OK;
    CLASSCACHEGETINFO ccgi = { 0 };
    LPCLASSCACHEENTRY pCacheEntry = NULL;
    IADsContainer *pDsContainer = NULL;
    IDirectorySearch *pDsSearch = NULL;
    BOOL fIsContainer = TRUE;
    ULONG cAdded = 0;
    TV_ITEM tvi;
    BSTR bstrADsPath = NULL;
    BSTR bstrClass = NULL;
    HCURSOR hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    TraceEnter(TRACE_BROWSE, "CBrowseDlg::ExpandNode");
    TraceAssert(pRootObject != NULL);
    TraceAssert(hwndTree != NULL);

    //
    // The IDirectorySearch method is better, but doesn't work for all objects
    // (e.g. "LDAP:") so try enumerating if IDirectorySearch isn't supported.
    //

    hres = pRootObject->get_Class(&bstrClass);
    FailGracefully(hres, "Failed to get class from object");

    hres = pRootObject->get_ADsPath(&bstrADsPath);
    FailGracefully(hres, "Failed to get class from object");

    ccgi.dwFlags = CLASSCACHE_CONTAINER|CLASSCACHE_TREATASLEAF|CLASSCACHE_DSAVAILABLE;
    ccgi.pPath = bstrADsPath;
    ccgi.pObjectClass = bstrClass;
    ccgi.pServer = _pServer;

    if ( _bi.dwFlags & DSBI_HASCREDENTIALS )
    {
        TraceMsg("Passing credential information to ClassCache API");            
        ccgi.pUserName = (LPWSTR)_bi.pUserName;
        ccgi.pPassword = (LPWSTR)_bi.pPassword;
    }

    if ( _bi.dwFlags & DSBI_SIMPLEAUTHENTICATE )
    {
        TraceMsg("Setting the CLASSCACHE_SIMPLEAUTHENTICATE flag");
        ccgi.dwFlags |= CLASSCACHE_SIMPLEAUTHENTICATE;
    }

    if ( SUCCEEDED(ClassCache_GetClassInfo(&ccgi, &pCacheEntry)) )
    {
        fIsContainer = _IsClassContainer(pCacheEntry, _bi.dwFlags & DSBI_IGNORETREATASLEAF);
        ClassCache_ReleaseClassInfo(&pCacheEntry);
    }

    if ( fIsContainer )
    {
        if ( SUCCEEDED(pRootObject->QueryInterface(IID_IDirectorySearch, (LPVOID*)&pDsSearch)) )
        {
            hres = SearchNode(pDsSearch, hwndTree, hParentItem, &cAdded);
            DoRelease(pDsSearch);
        }
        else if (SUCCEEDED(pRootObject->QueryInterface(IID_IADsContainer, (LPVOID*)&pDsContainer)))
        {
            hres = EnumerateNode(pDsContainer, hwndTree, hParentItem, &cAdded);
            DoRelease(pDsContainer);
        }
        else
        {
            TraceMsg("No IDsSearch, no IDsContainer - were screwed");
        }
    }

    // If we did not add anything we should update this item to let
    // the user know something happened.
    if (cAdded == 0)
    {
        tvi.mask = TVIF_CHILDREN | TVIF_HANDLE;
        tvi.hItem = hParentItem;
        tvi.cChildren = 0;
        TreeView_SetItem(hwndTree, &tvi);
    }

exit_gracefully:

    SysFreeString(bstrADsPath);
    SysFreeString(bstrClass);

    TraceLeaveResult(hres);
}


HRESULT
CBrowseDlg::EnumerateNode(IADsContainer *pDsContainer,
                          HWND          hwndTree,
                          HTREEITEM     hParentItem,
                          LPDWORD       pdwAdded)
{
    HRESULT hres = S_OK;
    IEnumVARIANT *pEnum = NULL;
    VARIANT aVariant[8];
    ULONG cAdded = 0;

    TraceEnter(TRACE_BROWSE, "CBrowseDlg::EnumerateNode");
    TraceAssert(pDsContainer != NULL);
    TraceAssert(hwndTree != NULL);

    hres = ADsBuildEnumerator(pDsContainer, &pEnum);
    FailGracefully(hres, "Unable to build container enumerator object");

    //
    // Enumerate the given container
    //
    for (;;)
    {
        ULONG cFetched = 0;
        ULONG i;

        //
        // Get a bunch of child containers and add them to the tree.
        //
        ADsEnumerateNext(pEnum, ARRAYSIZE(aVariant), aVariant, &cFetched);

        if (cFetched == 0)
            break;

        for (i = 0; i < cFetched; i++)
        {
            IADs *pDsObject;

            TraceAssert(V_VT(&aVariant[i]) == VT_DISPATCH);
            TraceAssert(V_DISPATCH(&aVariant[i]));

            if (SUCCEEDED(V_DISPATCH(&aVariant[i])->QueryInterface(IID_IADs,
                                                       (LPVOID*)&pDsObject)))
            {
                hres = AddTreeNode(pDsObject, NULL, hwndTree, hParentItem, NULL);
                if (SUCCEEDED(hres))
                    cAdded++;

                DoRelease(pDsObject);
            }

            VariantClear(&aVariant[i]);
        }
    }

    hres = S_OK;

exit_gracefully:

    DoRelease(pEnum);

    if (pdwAdded != NULL)
        *pdwAdded = cAdded;

    TraceLeaveResult(hres);
}


HRESULT
CBrowseDlg::SearchNode(IDirectorySearch *pDsSearch,
                       HWND         hwndTree,
                       HTREEITEM    hParentItem,
                       LPDWORD      pdwAdded)
{
    HRESULT hres = S_OK;
    ULONG cAdded = 0;
    ADS_SEARCH_HANDLE hSearch = NULL;
    ADS_SEARCH_COLUMN column;
    ADS_SEARCHPREF_INFO prefInfo[3];
    LPWSTR pszName = NULL;
    LPWSTR pszADsPath = NULL;
    LPWSTR pszObjectClass = NULL;
    BSTR bstrName;
    LPWSTR aProperties[] = { _szNameAttribute, c_szObjectClass, c_szADsPath, c_szName, c_szRDN };
    LPWSTR *pProperties = aProperties;
    INT cProperties = ARRAYSIZE(aProperties);
    USES_CONVERSION;

    TraceEnter(TRACE_BROWSE, "CBrowseDlg::SearchNode");
    TraceAssert(pDsSearch != NULL);
    TraceAssert(hwndTree != NULL);

    // Set the query prefernece to single level scope, and async retrevial
    // rather than waiting for all objects

    prefInfo[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    prefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[0].vValue.Integer = ADS_SCOPE_ONELEVEL;

    prefInfo[1].dwSearchPref = ADS_SEARCHPREF_ASYNCHRONOUS;
    prefInfo[1].vValue.dwType = ADSTYPE_BOOLEAN;
    prefInfo[1].vValue.Boolean = TRUE;

    prefInfo[2].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;         // paged results
    prefInfo[2].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[2].vValue.Integer = PAGE_SIZE;

    hres = pDsSearch->SetSearchPreference(prefInfo, ARRAYSIZE(prefInfo));
    FailGracefully(hres, "Failed to set search preferences");

    if ( !_szNameAttribute[0] )
    {
        TraceMsg("_szNameAttribute is NULL, so removing from query attributes");
        pProperties++;
        cProperties--;   
    }

    hres = pDsSearch->ExecuteSearch(_szFilter, pProperties, cProperties, &hSearch);
    FailGracefully(hres, "Failed in ExecuteSearch");

    for (;;)
    {
        CLASSCACHEENTRY cce;

        LocalFreeStringW(&pszObjectClass);
        LocalFreeStringW(&pszADsPath);
        LocalFreeStringW(&pszName);

        hres = pDsSearch->GetNextRow(hSearch);
        FailGracefully(hres, "Failed in GetNextRow");

        if (hres == S_ADS_NOMORE_ROWS)
        {
            break;
        }

        //
        // Get the columns for each of the properties we are interested in, if
        // we failed to get any of the base properties for the object then lets
        // just skip this entry as we cannot build a valid IDLIST for it.  The
        // properties that we request should be present on all objects.
        //

        if (FAILED(pDsSearch->GetColumn(hSearch, c_szObjectClass, &column)))
        {
            TraceMsg("Failed to get objectClass from search");
            continue;
        }

        hres = ObjectClassFromSearchColumn(&column, &pszObjectClass);
        pDsSearch->FreeColumn(&column);
        FailGracefully(hres, "Failed to get the object class from the property");

        if (FAILED(pDsSearch->GetColumn(hSearch, c_szADsPath, &column)))
        {
            TraceMsg("Failed to get ADsPath from search");
            continue;
        }

        hres = StringFromSearchColumn(&column, &pszADsPath);
        pDsSearch->FreeColumn(&column);
        FailGracefully(hres, "Failed to convert the ADsPath column to a string");

        // 
        // Try and read the name attribute from the query results, if that fails
        // then lets pass the ADsPath into the pathname API and get the 
        // LEAF name (RDN) back.
        //

        if ( _szNameAttribute[0] )
        {
            TraceMsg("Name attribute specified, therefore trying to decode that");
            hres = pDsSearch->GetColumn(hSearch, _szNameAttribute, &column);
        }
        
        if ( !_szNameAttribute[0] || FAILED(hres) )
        {
            TraceMsg("Either _szNameAttribute == NULL, or failed to read it");

            hres = pDsSearch->GetColumn(hSearch, c_szName, &column);
            if ( FAILED(hres) )
            {
                TraceMsg("'name' not returned as a column, so checking for RDN");
                hres = pDsSearch->GetColumn(hSearch, c_szRDN, &column);
            }
        }

        if ( SUCCEEDED(hres) )
        {
            //
            // so that succeeded and we have a search column that we can decode, so lets
            // do so and put that value into a string
            //

            hres = StringFromSearchColumn(&column, &pszName);
            pDsSearch->FreeColumn(&column);
            FailGracefully(hres, "Failed to convert the name column to a string");
        }
        else
        {
            BSTR bstrName;

            //
            // so now we attempt to use the path cracker as the string doesn't exist
            //

            TraceMsg("Failed to get the name, rdn etc, so using the path cracker");

            hres = GetPathCracker();
            FailGracefully(hres, "Unable to create ADsPathname object");

            hres = _pPathCracker->Set((LPWSTR)pszADsPath, ADS_SETTYPE_FULL);
            FailGracefully(hres, "Unable to parse path");

            hres = _pPathCracker->SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
            FailGracefully(hres, "Failed to set the display type for this value");

            hres = _pPathCracker->Retrieve(ADS_FORMAT_X500_DN, &bstrName);
            FailGracefully(hres, "Unable to retrieve requested path format");

            hres = LocalAllocStringW(&pszName, bstrName);
            SysFreeString(bstrName);
            FailGracefully(hres, "Failed to alloc clone of the name");
        }

        Trace(TEXT("class %s, name %s, ADsPath %s"), W2T(pszObjectClass), W2T(pszName), W2T(pszADsPath));

        hres = AddTreeNode(pszADsPath, pszObjectClass, pszName, hwndTree, hParentItem, NULL);
        if (SUCCEEDED(hres))
            cAdded++;
    }
    hres = S_OK;

exit_gracefully:

    if (hSearch != NULL)
        pDsSearch->CloseSearchHandle(hSearch);

    LocalFreeStringW(&pszObjectClass);
    LocalFreeStringW(&pszADsPath);
    LocalFreeStringW(&pszName);

    if (pdwAdded != NULL)
        *pdwAdded = cAdded;

    TraceLeaveResult(hres);
}


HRESULT
CBrowseDlg::AddTreeNode(IADs *pDsObject,
                        LPCWSTR pObjectPath,
                        HWND hwndTree,
                        HTREEITEM hParentItem,
                        HTREEITEM* phitem)
{
    HRESULT hres = E_FAIL;
    BSTR bstrClass = NULL;
    BSTR bstrPath = NULL;
    BSTR bstrName = NULL;
    VARIANT var = {0};
    LPWSTR pszName = NULL;

    TraceEnter(TRACE_BROWSE, "CBrowseDlg::AddTreeNode");
    TraceAssert(pDsObject != NULL);
    TraceAssert(hwndTree != NULL);

    // Do we want to include hidden objects?

    if ( !(_bi.dwFlags & DSBI_INCLUDEHIDDEN) )
    {
        if ( SUCCEEDED(pDsObject->Get(c_szShowInAdvancedViewOnly, &var)) ||
                SUCCEEDED(pDsObject->Get(c_szHideFromAB, &var)) )
        {
            TraceAssert(V_VT(&var) == VT_BOOL);

            if (!V_BOOL(&var))
                ExitGracefully(hres, E_FAIL, "Hidden node");

            VariantClear(&var);
        }
    }

    // Get the path and class name
    if ( !pObjectPath )
        pDsObject->get_ADsPath(&bstrPath);

    pDsObject->get_Class(&bstrClass);

    // Try to get the name property, if that fails then try RDN (for X5 connectivity)

    if ( SUCCEEDED(pDsObject->Get(c_szName, &var)) 
            || SUCCEEDED(pDsObject->Get(c_szRDN, &var))
                || (_szNameAttribute[0] && SUCCEEDED(pDsObject->Get(_szNameAttribute, &var))) )
    {
        TraceAssert(V_VT(&var) == VT_BSTR);
        pszName = V_BSTR(&var);
    }
    else if ( SUCCEEDED(pDsObject->get_Name(&bstrName)) )
    {
        pszName = bstrName;
    }

    if (pszName != NULL)
    {
        hres = AddTreeNode(pObjectPath ? pObjectPath : bstrPath, 
                           bstrClass, pszName, hwndTree, hParentItem, phitem);
    }                        

exit_gracefully:

    if ( bstrPath )
        SysFreeString(bstrPath);

    SysFreeString(bstrClass);
    SysFreeString(bstrName);
    VariantClear(&var);

    TraceLeaveResult(hres);
}


HRESULT
CBrowseDlg::AddTreeNode(LPCWSTR pszPath,
                        LPCWSTR pszClass,
                        LPCWSTR pszName,
                        HWND hwndTree,
                        HTREEITEM hParentItem,
                        HTREEITEM* phitem)
{
    HRESULT hres;
    BSTR bstrWinPath = NULL;
    CLASSCACHEGETINFO ccgi = { 0 };
    LPCLASSCACHEENTRY pCacheEntry = NULL;
    TV_INSERTSTRUCT tvi;
    BSTR bstrName = NULL;
    BOOL fIsContainer = TRUE;
    long nElements;
    INT iResult = 0;
    HTREEITEM hitem;
    WCHAR szIconLocation[MAX_PATH];
    INT iIconResID;
    USES_CONVERSION;

    TraceEnter(TRACE_BROWSE, "CBrowseDlg::AddTreeNode");
    TraceAssert(hwndTree != NULL);

    if (!pszPath  || !*pszPath || !pszClass || !*pszClass )
        ExitGracefully(hres, E_INVALIDARG, "Missing required string parameter");

    hres = GetPathCracker();
    FailGracefully(hres, "Failed to get the path cracker");

    hres = _pPathCracker->Set((LPWSTR)pszPath, ADS_SETTYPE_FULL);
    FailGracefully(hres, "Failed to set the path into the cracker");

    //
    // we can get the name from the cracker       
    //

    if ( !pszName || !*pszName )
    {
        _pPathCracker->SetDisplayType(ADS_DISPLAY_VALUE_ONLY);      // value only pls.

        hres = _pPathCracker->Retrieve(ADS_FORMAT_LEAF, &bstrName);
        FailGracefully(hres, "Failed to get leaf name");

        Trace(TEXT("bstrName: is %s"), W2CT(bstrName));
        pszName = bstrName;
    }
    
    tvi.hParent = hParentItem;
    tvi.hInsertAfter = TVI_SORT;
    tvi.item.mask = TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE |
                    TVIF_STATE | TVIF_TEXT | TVIF_PARAM;
    tvi.item.state = 0;
    tvi.item.stateMask = (UINT)-1;
    tvi.item.cchTextMax = 0;
    tvi.item.cChildren = 1;
    tvi.item.pszText = (LPTSTR)W2CT(pszName);
    tvi.item.iImage = 0;
    tvi.item.iSelectedImage = 0;
    tvi.item.lParam = 0;

    //
    // See if this object is a container, and get its image indexes
    // from the class cache.
    //

    ccgi.dwFlags = CLASSCACHE_CONTAINER|CLASSCACHE_TREATASLEAF|CLASSCACHE_ICONS|CLASSCACHE_DSAVAILABLE;
    ccgi.pPath = (LPWSTR)pszPath;
    ccgi.pObjectClass = (LPWSTR)pszClass;
    ccgi.pServer = _pServer;

    if ( _bi.dwFlags & DSBI_HASCREDENTIALS )
    {
        TraceMsg("Passing credential information to ClassCache API");            
        ccgi.pUserName = (LPWSTR)_bi.pUserName;
        ccgi.pPassword = (LPWSTR)_bi.pPassword;
    }

    if ( _bi.dwFlags & DSBI_SIMPLEAUTHENTICATE )
    {
        TraceMsg("Setting the CLASSCACHE_SIMPLEAUTHENTICATE flag");
        ccgi.dwFlags |= CLASSCACHE_SIMPLEAUTHENTICATE;
    }
    
    if ( SUCCEEDED(ClassCache_GetClassInfo(&ccgi, &pCacheEntry)) ) 
    {
        fIsContainer = _IsClassContainer(pCacheEntry, _bi.dwFlags & DSBI_IGNORETREATASLEAF);
        _GetIconLocation(pCacheEntry, DSGIF_DEFAULTISCONTAINER|DSGIF_GETDEFAULTICON, szIconLocation, ARRAYSIZE(szIconLocation), &iIconResID);
        ClassCache_ReleaseClassInfo(&pCacheEntry);
    }

    if ( !fIsContainer )
        ExitGracefully(hres, E_FAIL, "Not a container");

    // 
    // If we have a callback function then call it taking a note of
    // the changes the caller wants to make to the node we are adding
    // to the tree, internally the LPARAM of this item still points at
    // the ADsPath/Class structure, but the display information has
    // been suitably modified.
    //

    if ( _bi.pfnCallback )
    {
        DSBITEM dsbItem;

        dsbItem.cbStruct = SIZEOF(dsbItem);
        dsbItem.pszADsPath = pszPath;
        dsbItem.pszClass = pszClass;
        dsbItem.dwMask = DSBF_STATE | DSBF_DISPLAYNAME;
        dsbItem.dwState = 0;
        dsbItem.dwStateMask = DSBS_ROOT;
        StrCpyN(dsbItem.szDisplayName, W2CT(pszName), ARRAYSIZE(dsbItem.szDisplayName));
        StrCpyN(dsbItem.szIconLocation, W2CT(szIconLocation), ARRAYSIZE(dsbItem.szIconLocation));
        dsbItem.iIconResID = iIconResID;

        if ( _bi.dwFlags & DSBI_CHECKBOXES )
        {
// BUGBUG: handle the checked case properly;
            dsbItem.dwStateMask |= DSBS_CHECKED;
        }

        if ( (hParentItem == TVI_ROOT) || !hParentItem )
        {
            dsbItem.dwState |= DSBS_ROOT;
        }

        iResult = _bi.pfnCallback(GetParent(hwndTree), DSBM_QUERYINSERT, (LPARAM)&dsbItem, _bi.lParam);

#ifdef UNICODE
        //
        // For the UNICODE version of this call we must check to see if the 
        // caller handled it, if not then we will attempt to send an ANSI version
        // and thunk the parameters accordingly.
        //

        if ( !iResult )
        {
            DSBITEMA dsbItemA;
            CopyMemory(&dsbItemA, &dsbItem, SIZEOF(dsbItemA));

            WideCharToMultiByte(CP_ACP, 0, 
                                pszName, -1, 
                                dsbItemA.szDisplayName, ARRAYSIZE(dsbItemA.szDisplayName), 
                                NULL, NULL);

            WideCharToMultiByte(CP_ACP, 0, 
                                dsbItem.szIconLocation, -1, 
                                dsbItemA.szIconLocation, ARRAYSIZE(dsbItemA.szIconLocation), 
                                NULL, NULL);

            iResult = _bi.pfnCallback(GetParent(hwndTree), DSBM_QUERYINSERTA, (LPARAM)&dsbItemA, _bi.lParam);
            if ( iResult )
            {
                TraceMsg("ANSI DSBM_QUERYINSERT was successful");

                Assert(SIZEOF(DSBITEMA) == SIZEOF(DSBITEMW));
                CopyMemory(&dsbItem, &dsbItemA, SIZEOF(dsbItem));

                MultiByteToWideChar(CP_ACP, 0, 
                                    dsbItemA.szDisplayName, -1, 
                                    dsbItem.szDisplayName, ARRAYSIZE(dsbItem.szDisplayName)); 

                MultiByteToWideChar(CP_ACP, 0, 
                                    dsbItemA.szIconLocation, -1, 
                                    dsbItem.szIconLocation, ARRAYSIZE(dsbItem.szIconLocation)); 
            }
        }
#endif

        //
        // iResult == TRUE then the user has modified the structure and we
        // should attempt to apply the changes they have made to the 
        // item we are about to add to the view.
        //

        if ( iResult )
        {
            if ((dsbItem.dwMask & DSBF_STATE) &&
                (dsbItem.dwStateMask & DSBS_HIDDEN))
            {
                ExitGracefully(hres, E_FAIL, "Item is hidden, therefore skipping");
            }
                
            if ((_bi.dwFlags & DSBI_CHECKBOXES) &&
                    (dsbItem.dwMask & DSBF_STATE) &&
                    (dsbItem.dwStateMask & DSBS_CHECKED))
            {
// BUGBUG set the state image
            }

            if ( dsbItem.dwMask & DSBF_ICONLOCATION )
            {
                StrCpyW(szIconLocation, T2CW(dsbItem.szIconLocation));
                iIconResID = dsbItem.iIconResID;
            }

            if (dsbItem.dwMask & DSBF_DISPLAYNAME)
            {
                dsbItem.szDisplayName[DSB_MAX_DISPLAYNAME_CHARS-1] = TEXT('\0');
                tvi.item.pszText = dsbItem.szDisplayName;
            }
        }
    } 

    //
    // convert the icon location to an index that we can use in the tree view
    //

    Trace(TEXT("Icon location is: %s,%d"), W2T(szIconLocation), iIconResID);

    tvi.item.iImage = tvi.item.iSelectedImage = Shell_GetCachedImageIndex(W2T(szIconLocation), iIconResID, 0x0);
    Trace(TEXT("Index into the shell image list %d"), tvi.item.iImage);

    //
    // Make a copy of the path to store as the node data.
    // Try the path cracker first, so we get a known format.
    // If that fails, just make a copy of what we've got.
    //
    // BUGBUG: The problem with the path cracker is that it just cannot cope
    // BUGBUG: with names with no elements, therefore we have to work around
    // BUGBUG: this by checking for no elements then looking at the retrieved
    // BUGBUG: path to see if it terminates in a bogus way, if it does then
    // BUGBUG: lets fix it in a local buffer before creating the tree view node.
    //

    hres = _pPathCracker->GetNumElements(&nElements);
    if ( SUCCEEDED(hres) )
    {
        _pPathCracker->SetDisplayType(ADS_DISPLAY_FULL);

        Trace(TEXT("nElements %d"), nElements);
        hres = _pPathCracker->Retrieve(ADS_FORMAT_WINDOWS, &bstrWinPath);
        if ( SUCCEEDED(hres) )
        {
            int cchWinPath = lstrlenW(bstrWinPath);

            Trace(TEXT("bstrWinPath %s (%d), nElements %d"), 
                                    W2T(bstrWinPath), cchWinPath, nElements);
            if ( (!nElements) &&
                 (cchWinPath >= 3) &&
                 (bstrWinPath[cchWinPath-1] == L'/') &&
                 (bstrWinPath[cchWinPath-2] == L'/') &&
                 (bstrWinPath[cchWinPath-3] == L':') )
            {
                LPWSTR pFixedWinPath = NULL;

                hres = LocalAllocStringW(&pFixedWinPath, bstrWinPath);
                if ( SUCCEEDED(hres) )
                {
                    pFixedWinPath[cchWinPath-2] = L'\0';
                    Trace(TEXT("pFixedWinPath %s"), W2T(pFixedWinPath));

                    hres = BuildNodeString(pFixedWinPath, pszClass, (LPWSTR*)&tvi.item.lParam);
                    LocalFreeStringW(&pFixedWinPath);
                }
            }
            else
            {
                hres = BuildNodeString(bstrWinPath, pszClass, (LPWSTR*)&tvi.item.lParam);
            }
        }
    }

    if ( FAILED(hres) )
        hres = BuildNodeString(pszPath, pszClass, (LPWSTR*)&tvi.item.lParam);

    FailGracefully(hres, "Unable to build node data");

    //
    // finally lets add the item to the tree, if that fails then ensure we free the
    // structure hanging from the TVI.
    //

    hitem = TreeView_InsertItem(hwndTree, &tvi);
    if ( !hitem )
    {
        LocalFreeStringW((LPWSTR*)&tvi.item.lParam);
        hres = E_FAIL;
    }

exit_gracefully:

    SysFreeString(bstrName);
    SysFreeString(bstrWinPath);

    if ( phitem )
        *phitem = hitem;

    TraceLeaveResult(hres);
}


HRESULT
CBrowseDlg::AddTreeNode(LPDOMAINDESC pDomainDesc,
                        HWND hwndTree,
                        HTREEITEM hParentItem,
                        HTREEITEM* phitem)
{
    HRESULT hres;
    WCHAR szBuffer[MAX_PATH];
    DWORD dwIndex;
    HTREEITEM hitem;
    USES_CONVERSION;

    TraceEnter(TRACE_BROWSE, "CBrowseDlg::AddTreeNode");

    while ( pDomainDesc )
    {
        StrCpyW(szBuffer, c_szLDAPPrefix);

        if ( _pServer )
        {
            StrCatW(szBuffer, _pServer);
            StrCatW(szBuffer, L"/");
        }

        StrCatW(szBuffer, pDomainDesc->pszNCName);

        Trace(TEXT("Scope is: %s"), W2T(szBuffer));

        hres = AddTreeNode(szBuffer,
                           pDomainDesc->pszObjectClass, NULL,
                           hwndTree, hParentItem,
                           &hitem);

        FailGracefully(hres, "Failed to add location node");        

        if ( pDomainDesc->pdChildList )
        {
            hres = AddTreeNode(pDomainDesc->pdChildList,
                               hwndTree, hitem,
                               NULL);

            FailGracefully(hres, "Failed to add children");
        }

        pDomainDesc = pDomainDesc->pdNextSibling;
    }

    hres = S_OK;

exit_gracefully:

    if ( phitem )
        *phitem = hitem;

    TraceLeaveResult(hres);
}


BOOL
CBrowseDlg::InitDlg(HWND hDlg)
{
    HRESULT hres;
    HWND hwndTree;
    IADs *pRoot = NULL;
    IDsBrowseDomainTree* pDsDomains = NULL;
    BSTR bstrServer = NULL;
    LPDOMAINTREE pDomainTree = NULL;
    HTREEITEM hitemRoot = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_BROWSE, "CBrowseDlg::InitDlg");
    TraceAssert(hDlg != NULL);

    if (_bi.pszCaption != NULL)
        SetWindowText(hDlg, W2CT(_bi.pszCaption));

    if (_bi.pszTitle == NULL)
    {
        RECT rc;
        LONG yPos;
        HWND hwnd;

        // Get the position of the title window and hide it
        hwnd = GetDlgItem(hDlg, DSBID_BANNER);
        GetWindowRect(hwnd, &rc);
        yPos = rc.top;
        ShowWindow(hwnd, SW_HIDE);

        // Get the position of the tree control and adjust it
        // to cover the title window.
        hwnd = GetDlgItem(hDlg, DSBID_CONTAINERLIST);

        GetWindowRect(hwnd, &rc);
        rc.top = yPos;

        MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);
        MoveWindow(hwnd, 
                   rc.left, rc.top,
                   rc.right - rc.left, rc.bottom - rc.top,
                   FALSE);
    }
    else
    {
        SetDlgItemText(hDlg, DSBID_BANNER, W2CT(_bi.pszTitle));
    }

    hwndTree = GetDlgItem(hDlg, DSBID_CONTAINERLIST);
    TraceAssert(hwndTree != NULL);

    // Update the TreeView style according to what the caller wants
    if ( _bi.dwFlags & (DSBI_NOBUTTONS | DSBI_NOLINES | DSBI_NOLINESATROOT | DSBI_CHECKBOXES) )
    {
        DWORD dwStyle = GetWindowLong(hwndTree, GWL_STYLE);
        dwStyle &= ~(_bi.dwFlags & (DSBI_NOBUTTONS | DSBI_NOLINES | DSBI_NOLINESATROOT));
        dwStyle |= (_bi.dwFlags & DSBI_CHECKBOXES);
        SetWindowLong(hwndTree, GWL_STYLE, dwStyle);
    }

    if (_bi.dwFlags & DSBI_CHECKBOXES)
    {
        // BUGBUG load and set the state imagelist (unchecked and checked squares)
    }

    //
    // ensure we set the shared image list for the tree, this comes from shell32.
    //

    HIMAGELIST himlSmall;
    Shell_GetImageLists(NULL, &himlSmall);
    TreeView_SetImageList(hwndTree, himlSmall, TVSIL_NORMAL);

    TraceAssert(_bi.pszRoot != NULL);
    Trace(TEXT("pszRoot is: %s"), W2CT(_bi.pszRoot));

    //
    // if we have a callback function then we need to call it to get the information we need
    // to browse the DS namespace.
    //

    if ( _bi.pfnCallback) 
    {
        DSBROWSEDATA dbd = { 0 };

        dbd.pszFilter = _szFilter;
        dbd.cchFilter = ARRAYSIZE(_szFilter);
        dbd.pszNameAttribute = _szNameAttribute;
        dbd.cchNameAttribute = ARRAYSIZE(_szNameAttribute);

        TraceMsg("Calling callback to see if it can provide enumeration information");

        if ( _bi.pfnCallback(hDlg, DSBM_GETBROWSEDATA, (LPARAM)&dbd, _bi.lParam) )
        {
            Trace(TEXT("szFilter after DSBM_GETBROWSEDATA: %s"), 
                                _szFilter[0] ? W2T(_szFilter):TEXT("<not specified>"));

            Trace(TEXT("szNameAttribute after DSBM_GETBROWSEDATA: %s"), 
                                _szNameAttribute[0] ? W2T(_szNameAttribute):TEXT("<not specified>"));

            if ( !_szFilter[0] )
                ExitGracefully(hres, E_FAIL, "szFilter returned was NULL");
        }
    }


    //
    // Bind to the root object (make sure it's a valid object)
    //

    hres = ADsOpenObject((LPWSTR)_bi.pszRoot, 
                         (_bi.dwFlags & DSBI_HASCREDENTIALS) ? (LPWSTR)_bi.pUserName:NULL,
                         (_bi.dwFlags & DSBI_HASCREDENTIALS) ? (LPWSTR)_bi.pPassword:NULL,
                         (_bi.dwFlags & DSBI_SIMPLEAUTHENTICATE) ? 0:ADS_SECURE_AUTHENTICATION, 
                         IID_IADs, (LPVOID*)&pRoot);

    FailGracefully(hres, "Unable to bind to root object");

    // attempt to decode the root path we have been given, if this includes a server
    // name then lets store that so that we can call the cache codes.  internally
    // we preserver this.

    hres = GetPathCracker();
    FailGracefully(hres, "Failed to get the path cracker API");

    hres = _pPathCracker->Set((LPWSTR)_bi.pszRoot, ADS_SETTYPE_FULL);
    FailGracefully(hres, "Unable to put the path into the path cracker");

    if ( SUCCEEDED(_pPathCracker->Retrieve(ADS_FORMAT_SERVER, &bstrServer)) )
    {
        Trace(TEXT("Root path contains a server: %s"), W2T(bstrServer));
        hres = LocalAllocStringW(&_pServer, bstrServer);
        FailGracefully(hres, "Failed to allocate copy of ADsPath");
    }

    // DSBI_ENTIREDIRECTORY contains 2 bits which means that 
    // "if ( _bi.dwFlags & DSBI_ENTIREDIRECTORY )" gives a false
    // positive when a client calls with only DSBI_NOROOT set. This
    // causes the entire directory to be displayed when it shouldn't.

    //if ( _bi.dwFlags & DSBI_ENTIREDIRECTORY )
    if ( _bi.dwFlags & (DSBI_ENTIREDIRECTORY & ~DSBI_NOROOT) )
    {
        TV_ITEM tvi;
        LPDOMAINDESC pDomain;

        if ( !(_bi.dwFlags & DSBI_NOROOT) )
        {
            hres = AddTreeNode(pRoot, _bi.pszRoot, hwndTree, NULL, &hitemRoot);
            FailGracefully(hres, "Failed when adding in root node");

            tvi.mask = TVIF_STATE|TVIF_HANDLE;
            tvi.hItem = hitemRoot;
            tvi.stateMask = -1;
            TreeView_GetItem(hwndTree, &tvi);
            tvi.state |= TVIS_EXPANDEDONCE;
            TreeView_SetItem(hwndTree, &tvi);
        }

        hres = CoCreateInstance(CLSID_DsDomainTreeBrowser, NULL,CLSCTX_INPROC_SERVER, IID_IDsBrowseDomainTree, (LPVOID*)&pDsDomains);
        FailGracefully(hres, "Failed to get IDsDomainTreeBrowser");

        LPCWSTR pUserName = (_bi.dwFlags & DSBI_HASCREDENTIALS) ? _bi.pUserName:NULL;
        LPCWSTR pPassword = (_bi.dwFlags & DSBI_HASCREDENTIALS) ? _bi.pPassword:NULL;

        hres = pDsDomains->SetComputer(_pServer, pUserName, pPassword);

        if ( SUCCEEDED(hres) )
            hres = pDsDomains->GetDomains(&pDomainTree, DBDTF_RETURNFQDN);

        if ( SUCCEEDED(hres) )
            AddTreeNode(pDomainTree->aDomains, hwndTree, hitemRoot, NULL);
    }
    else if (_bi.dwFlags & DSBI_NOROOT)
    {
        // Skip root node and add its children as toplevel nodes
        hres = ExpandNode(pRoot, hwndTree, NULL);
    }
    else
    {
        // Add the root node
        hres = AddTreeNode(pRoot, _bi.pszRoot, hwndTree, NULL, &hitemRoot);
    }

exit_gracefully:

    if (SUCCEEDED(hres))
    {
        //
        // Set the selected path to expand the tree, this can either be NULL, or
        // a ADSI path.  If we fail to do that and we have a root node then
        // lets expand the root node to at least have something highlighted / expanded.
        //

        if ( !(_bi.dwFlags & DSBI_EXPANDONOPEN) || (-1 == SetSelectedPath(hDlg, _bi.pszPath)) )
        {
            if ( !(_bi.dwFlags & DSBI_NOROOT) )
            {
                TraceMsg("Failed to select node, therefore expanding root");
                SetSelectedPath(hDlg, _bi.pszRoot);
            }
        }

        if ( _bi.pfnCallback )
            _bi.pfnCallback(hDlg, BFFM_INITIALIZED, 0, _bi.lParam);
    }
    else
    {
        // BUGBUG: SetLastError(???);
        EndDialog(hDlg, -1);
    }

    DoRelease(pRoot);
    DoRelease(pDsDomains);

    SysFreeString(bstrServer);

    TraceLeaveValue(TRUE);
}


BOOL
CBrowseDlg::OnNotify(HWND hDlg, int idCtrl, LPNMHDR pnmh)
{
    LPNM_TREEVIEW pnmtv = (LPNM_TREEVIEW)pnmh;
    LPWSTR pszClass;
    HRESULT hres;
    USES_CONVERSION;

    switch (pnmh->code)
    {
    case TVN_DELETEITEM:
        LocalFree((HLOCAL)pnmtv->itemOld.lParam);
        break;

    case TVN_ITEMEXPANDING:
        if ((pnmtv->action == TVE_EXPAND) &&
            !(pnmtv->itemNew.state & TVIS_EXPANDEDONCE))
        {
            if (pnmtv->itemNew.hItem == NULL)
            {
                pnmtv->itemNew.hItem = TreeView_GetSelection(pnmh->hwndFrom);
            }

            // Whether we succeed or not, mark the node as having been
            // expanded once.
            pnmtv->itemNew.mask = TVIF_STATE;
            pnmtv->itemNew.stateMask = TVIS_EXPANDEDONCE;
            pnmtv->itemNew.state = TVIS_EXPANDEDONCE;

            if (!pnmtv->itemNew.lParam ||
                 FAILED(ExpandNode((LPWSTR)pnmtv->itemNew.lParam,
                                   pnmh->hwndFrom, pnmtv->itemNew.hItem)))
            {
                // Mark this node as having no children
                pnmtv->itemNew.mask |= TVIF_CHILDREN;
                pnmtv->itemNew.cChildren = 0;
            }

            TreeView_SetItem(pnmh->hwndFrom, &pnmtv->itemNew);
        }
        break;

    case TVN_ITEMEXPANDED:
    {
        CLASSCACHEGETINFO ccgi = { 0 };
        LPCLASSCACHEENTRY pCacheEntry = NULL;

        pszClass = (LPWSTR)ByteOffset(pnmtv->itemNew.lParam, StringByteSizeW((LPWSTR)pnmtv->itemNew.lParam));
        if ( !pszClass )
            return FALSE;

        // Switch to the "open" image

        ccgi.dwFlags = CLASSCACHE_DSAVAILABLE|CLASSCACHE_ICONS|(DSGIF_ISOPEN << CLASSCACHE_IMAGEMASK_BIT);
        ccgi.pPath = (LPWSTR)pnmtv->itemNew.lParam;
        ccgi.pObjectClass = pszClass;
        ccgi.pServer = _pServer;

        if ( _bi.dwFlags & DSBI_HASCREDENTIALS )
        {
            ccgi.pUserName = (LPWSTR)_bi.pUserName;
            ccgi.pPassword = (LPWSTR)_bi.pPassword;
        }

        if ( _bi.dwFlags & DSBI_SIMPLEAUTHENTICATE )
            ccgi.dwFlags |= CLASSCACHE_SIMPLEAUTHENTICATE;

        if ( (pnmtv->action == TVE_EXPAND) || (pnmtv->action == TVE_COLLAPSE) )
        {
            //
            // handle the expand and colapse of the icon in the tree, assuming that
            // we show the correct state of corse (for those who don't have open states).
            //

            if ( SUCCEEDED(ClassCache_GetClassInfo(&ccgi, &pCacheEntry)) )
            {
                WCHAR szIconLocation[MAX_PATH];
                INT iIconResID;
                
                hres = _GetIconLocation(pCacheEntry, DSGIF_DEFAULTISCONTAINER|DSGIF_ISOPEN, szIconLocation, ARRAYSIZE(szIconLocation), &iIconResID);
                ClassCache_ReleaseClassInfo(&pCacheEntry);

                if ( SUCCEEDED(hres) )
                {
                    pnmtv->itemNew.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                    pnmtv->itemNew.iImage = pnmtv->itemNew.iSelectedImage = Shell_GetCachedImageIndex(W2T(szIconLocation), iIconResID, 0x0);
                    TreeView_SetItem(pnmh->hwndFrom, &pnmtv->itemNew);
                }
            }
        }

        break;
    }

    case TVN_SELCHANGED:
    {
        if ( _bi.pfnCallback )
            _bi.pfnCallback(hDlg, BFFM_SELCHANGED, (LPARAM)GetSelectedPath(hDlg), _bi.lParam);

        break;
    }
    
    default:
        return FALSE;
    }

    return TRUE;
}


void
CBrowseDlg::OnOK(HWND hDlg)
{
    int nResult = -1;
    LPCWSTR pszADsPath;
    LPCWSTR pszObjectClass;
    BSTR bstrPath = NULL;
    DWORD dwFormat = _pServer ? ADS_FORMAT_X500:ADS_FORMAT_X500_NO_SERVER;
    USES_CONVERSION;

    TraceEnter(TRACE_BROWSE, "CBrowseDlg::OnNotify");
    TraceAssert(hDlg != NULL);

    pszADsPath = GetSelectedPath(hDlg);
    pszObjectClass = GetSelectedObjectClass(hDlg);

    if ( !pszADsPath || !pszObjectClass )
        ExitGracefully(nResult, -1, "Failed to get selected object");

    // Honor the return type if they gave us one.

    if ((_bi.dwFlags & DSBI_RETURN_FORMAT) && _bi.dwReturnFormat)
    {
        TraceMsg("Caller specified a dwReturnFormat");
        dwFormat = _bi.dwReturnFormat;
    }

    // CBrowseDlg uses ADS_FORMAT_WINDOWS internally, so no need to convert if 
    // that's what they want.

    if ( dwFormat != ADS_FORMAT_WINDOWS )
    {
        HRESULT hres;

        hres = GetPathCracker();
        FailGracefully(hres, "Unable to create ADsPathname object");

        hres = _pPathCracker->Set((LPWSTR)pszADsPath, ADS_SETTYPE_FULL);
        FailGracefully(hres, "Unable to parse path");

        hres = _pPathCracker->Retrieve(dwFormat, &bstrPath);
        FailGracefully(hres, "Unable to retrieve requested path format");

        pszADsPath = bstrPath;
    }

    // return the object class to the caller

    StrCpyNW(_bi.pszPath, pszADsPath, _bi.cchPath);

    if ( (_bi.dwFlags & DSBI_RETURNOBJECTCLASS) && _bi.pszObjectClass )
    {
        Trace(TEXT("Returning objectClass: %s"), W2CT(pszObjectClass));
        StrCpyNW(_bi.pszObjectClass, pszObjectClass, _bi.cchObjectClass);
    }

    nResult = IDOK;

exit_gracefully:

    SysFreeString(bstrPath);
    EndDialog(hDlg, nResult);

    TraceLeaveVoid();
}


BOOL
CBrowseDlg::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    USES_CONVERSION;

    switch (uMsg)
    {
    case WM_NOTIFY:
        return OnNotify(hDlg, (int)wParam, (LPNMHDR)lParam);

    case BFFM_ENABLEOK:
        EnableWindow(GetDlgItem(hDlg, IDOK), (BOOL)wParam);
        break;

    case BFFM_SETSELECTIONA:
    case BFFM_SETSELECTIONW:
        // lParam points to an ADSI path, which we require to be UNICODE
        SetSelectedPath(hDlg, (LPCWSTR)lParam);
        break;

#if 0
    case BFFM_SETSTATUSTEXTA:
    case BFFM_SETSTATUSTEXTW:
        // unimplemented
        break;
#endif

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDOK:
            OnOK(hDlg);
            break;

        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            break;

        default:
            // Message not handled
            return FALSE;
        }
        break;

    case WM_INITDIALOG:
        return InitDlg(hDlg);

    case WM_HELP:
    case WM_CONTEXTMENU:
    {
        //
        // check to see if we have a callback, if so then lets call so that they 
        // can display help on this object.
        //

        if ( !_bi.pfnCallback )
            return FALSE;

        _bi.pfnCallback(hDlg,
                        (uMsg == WM_HELP) ? DSBM_HELP:DSBM_CONTEXTMENU,
                        (uMsg == WM_HELP) ? lParam:(LPARAM)wParam,
                        _bi.lParam);
        break;
    }

    default:
        return FALSE;           // not handled
    }

    return TRUE;
}


INT_PTR
CALLBACK
CBrowseDlg::_DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INT_PTR ipr = FALSE;
    PBROWSEDLG pThis = (PBROWSEDLG)GetWindowLongPtr(hDlg, DWLP_USER);

    if (uMsg == WM_INITDIALOG)
    {
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        pThis = (PBROWSEDLG)lParam;
    }

    if (pThis != NULL)
        ipr = (INT_PTR)pThis->DlgProc(hDlg, uMsg, wParam, lParam);

    return ipr;
}
