#include "pch.h"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ Misc data
/----------------------------------------------------------------------------*/

static UINT g_cfDsPropPageInfo = 0;

#define PATH_IS             TEXT("?path=")
#define PROPERTY_PAGES_ROOT TEXT("PropertyPagesRoot")

HRESULT _GetAbsoluteURL(LPTSTR* ppAbsoluteURL, LPTSTR pRelativeURL, LPTSTR pDsObjectName);

//
// IDataObject wrapper for the passing the extra parameter information for the 
// Win32 property pages
//

class CDsPropPageDataObject : public IDataObject, CUnknown
{
    private:
        IDataObject* m_pDataObject;
        LPWSTR m_pParameters;

    public:
        CDsPropPageDataObject(IDataObject* pDataObject, LPWSTR pParameters);
        ~CDsPropPageDataObject();

        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IDataObject
        STDMETHODIMP GetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium);
        STDMETHODIMP GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium);
        STDMETHODIMP QueryGetData(FORMATETC *pformatetc);
        STDMETHODIMP GetCanonicalFormatEtc(FORMATETC *pformatectIn, FORMATETC *pformatetcOut);
        STDMETHODIMP SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium, BOOL fRelease);
        STDMETHODIMP EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc);
        STDMETHODIMP DAdvise(FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection);
        STDMETHODIMP DUnadvise(DWORD dwConnection);
        STDMETHODIMP EnumDAdvise(IEnumSTATDATA **ppenumAdvise);
};


/*-----------------------------------------------------------------------------
/ Tab Collector bits
/----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
/ TabCollector_AddPages
/ ---------------------
/   Given a string that represents a page reference add the pages required
/   to support that reference
/
/ In:
/   pPageReference -> string reperesenting the page
/   pDsObjectName -> ADs path of object
/   pDataObject -> data object interface for the Win32 extensions
/   lpfnAddPage, lParam => parameters used for adding each page
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT TabCollector_AddPages(LPWSTR pPageReference, LPWSTR pDsObjectName, 
                                IUnknown* punkSite, IDataObject* pDataObject, 
                                    LPFNADDPROPSHEETPAGE pAddPageProc, LPARAM lParam)
{
    HRESULT hres;
    HPROPSHEETPAGE hPage;
    IUnknown* pUnknown = NULL;
    IShellExtInit* pShellExtInit = NULL;
    IShellPropSheetExt* pShellPropSheetExt = NULL;
    IObjectWithSite *pows = NULL;
    WCHAR szBuffer[MAX_PATH];
    WCHAR szGUID[MAX_PATH];
    WCHAR szURL[INTERNET_MAX_URL_LENGTH];
    LPTSTR pAbsoluteURL = NULL;
    USES_CONVERSION;
    GUID guid;

    TraceEnter(TRACE_TABS, "TabCollector_AddPages");
    Trace(TEXT("Page reference is %s"), W2T(pPageReference));

    // The name is either a CLSID, or a URL description.  Therefore lets try and
    // parse it as a GUID, if that fails then we can just attempt to break out the
    // other components.

    if ( SUCCEEDED(GetStringElementW(pPageReference, 0, szGUID, ARRAYSIZE(szGUID))) &&
                            GetGUIDFromStringW(pPageReference, &guid) )
    {
        if ( SUCCEEDED(CoCreateInstance(guid, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (LPVOID*)&pUnknown)) )
        {
            // We have found the object, lets try and initialize it passing it the data
            // object we have, if that fails then we ignore this entry.

            if ( punkSite && SUCCEEDED(pUnknown->QueryInterface(IID_IObjectWithSite, (void **)&pows)) )
            {
                hres = pows->SetSite(punkSite);
                FailGracefully(hres, "Failed when setting site on the object");
            }

            if ( SUCCEEDED(pUnknown->QueryInterface(IID_IShellExtInit, (LPVOID*)&pShellExtInit)) )
            {
                if ( SUCCEEDED(GetStringElementW(pPageReference, 1, szBuffer, ARRAYSIZE(szBuffer))) && szBuffer[0] )
                {
                    CDsPropPageDataObject* pDsPropPageDataObject = new CDsPropPageDataObject(pDataObject, szBuffer);
                    TraceAssert(pDsPropPageDataObject);

                    if ( !pDsPropPageDataObject )
                        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate IDataObject wrapper");
            
                    Trace(TEXT("IDsPropPageDataObject constructed with: %s"), szBuffer);
                                    
                    hres = pShellExtInit->Initialize(NULL, pDsPropPageDataObject, NULL);
                    pDsPropPageDataObject->Release();
                }
                else
                {
                    TraceMsg("No extra parameters for property page, invoking with original IDataObject");
                    hres = pShellExtInit->Initialize(NULL, pDataObject, NULL);
                }

                DoRelease(pShellExtInit);

                if ( FAILED(hres) )
                    ExitGracefully(hres, S_OK, "Failed to Initialize the Win32 extension - PAGE IGNORED");
            }

            // We have tried to Initialize the object, so lets get it to add the pages if it
            // supports the IShellPropSheetExt interface.

            if ( SUCCEEDED(pUnknown->QueryInterface(IID_IShellPropSheetExt, (LPVOID*)&pShellPropSheetExt)) )
            {
                hres = pShellPropSheetExt->AddPages(pAddPageProc, lParam);
                DoRelease(pShellPropSheetExt);

                if (hres == HRESULT_FROM_WIN32(ERROR_BAD_NET_RESP))
                    FailGracefully(hres, "Cannot talk to the DC");
            }   
        }
        else
        {
            TraceGUID("Failed to CoCreateInstance ", guid);
        }
    }
    else
    {
#if 1
        ExitGracefully(hres, E_NOTIMPL, "HTML property pages are not supported");
#else
        // The page reference didn't parse as GUID, therefore it must be a
        // Trident reference. If it is then we must attempt to parse the string
        // extracting the tab position, tab title, and URL.
        //
        // The string is formatted as follows:
        //
        //  <tab name>,<URL>

        Trace(TEXT("Parsing: %s"), pPageReference);

        if ( SUCCEEDED(GetStringElementW(pPageReference, 0, szBuffer, ARRAYSIZE(szBuffer))) &&
             SUCCEEDED(GetStringElementW(pPageReference, 1, szURL, ARRAYSIZE(szURL))) )
        {
            LPTSTR pURL_T = W2T(szURL);
            LPTSTR pBuffer_T = W2T(szBuffer);

            Trace(TEXT("Tab name: %s, URL: %s"), pBuffer_T, pURL_T);

            if ( szBuffer[0] && szURL[0] )
            {
                if ( SUCCEEDED(_GetAbsoluteURL(&pAbsoluteURL, pURL_T, W2T(pDsObjectName))) )
                {
                    Trace(TEXT("Opening with absolute URL %s"), pAbsoluteURL);

                    if ( SUCCEEDED(CHtmlPropPage::CreatePage(pAbsoluteURL, pBuffer_T, GLOBAL_HINSTANCE, &hPage)) )
                    {
                        Trace(TEXT("Calling pAddPageProc with hPage %08x"), hPage);                            
                        (*pAddPageProc)(hPage, lParam);
                    }
                }
            }
        }
#endif    
    }

    hres = S_OK;              // success

exit_gracefully:

    LocalFreeString(&pAbsoluteURL);

    DoRelease(pUnknown);
    DoRelease(pows);
    DoRelease(pShellExtInit);
    DoRelease(pShellPropSheetExt);

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ TabCollector_Collect
/ --------------------
/   Given the IDataObject interface and a callback function add the
/   pages that represent that object class.
/
/ In:
/   pDataObject -> data object interface that we can query for the object names
/   lpfnAddPage, lParam => parameters used for adding each page
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT TabCollector_Collect(IUnknown *punkSite, IDataObject* pDataObject, LPFNADDPROPSHEETPAGE pAddPageProc, LPARAM lParam)
{
    HRESULT hres;
    STGMEDIUM medium = { TYMED_NULL };
    FORMATETC fmte = {g_cfDsObjectNames, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    LPDSOBJECTNAMES pDsObjectNames;
    LPWSTR pPath;
    LPWSTR pObjectClass;
    CLASSCACHEGETINFO ccgi = { 0 };
    LPCLASSCACHEENTRY pCacheEntry = NULL;
    USES_CONVERSION;
    INT i;
    HDPA hdpa = NULL;

    TraceEnter(TRACE_TABS, "TabCollector_Collect");

    if ( !pDataObject || !pAddPageProc )
        ExitGracefully(hres, E_INVALIDARG, "pDataObject || pAddPageProc == NULL");

    // From the IDataObject we must attempt to get the DSOBJECTNAMES structure
    // that defines the objects we are being invoked on.  If we cannot get that
    // format, or the structure doesn't contain enough entries then bail out.

    hres = pDataObject->GetData(&fmte, &medium);
    FailGracefully(hres, "Failed to GetData using CF_DSOBJECTNAMES");

    pDsObjectNames = (LPDSOBJECTNAMES)medium.hGlobal;

    if ( pDsObjectNames->cItems < 1 )
        ExitGracefully(hres, E_FAIL, "Not enough objects in DSOBJECTNAMES structure");

    pPath = (LPWSTR)ByteOffset(pDsObjectNames, pDsObjectNames->aObjects[0].offsetName);
    pObjectClass = (LPWSTR)ByteOffset(pDsObjectNames, pDsObjectNames->aObjects[0].offsetClass);

    // fill the CLASSCACHEGETINFO record so we can cache the information from the
    // display specifiers.

    ccgi.dwFlags = CLASSCACHE_PROPPAGES;
    ccgi.pPath = pPath;
    ccgi.pObjectClass = pObjectClass;
    ccgi.pDataObject = pDataObject;

    hres = GetServerAndCredentails(&ccgi);
    FailGracefully(hres, "Failed to get the server name");

    hres = GetAttributePrefix(&ccgi.pAttributePrefix, pDataObject);
    FailGracefully(hres, "Failed to get attributePrefix");

    Trace(TEXT("Class: %s; Attribute Prefix: %s; Server: %s"), 
                W2T(pObjectClass), W2T(ccgi.pAttributePrefix), ccgi.pServer ? W2T(ccgi.pServer):TEXT("<none>"));

    hres = ClassCache_GetClassInfo(&ccgi, &pCacheEntry);
    FailGracefully(hres, "Failed to get page list (via the cache)");

    // Just keep what is needed and then release the cache
    if ( (pCacheEntry->dwCached & CLASSCACHE_PROPPAGES) && pCacheEntry->hdsaPropertyPages )
    {
        hdpa = DPA_Create(16);         // grow size
        if ( !hdpa )
            ExitGracefully(hres, E_OUTOFMEMORY, "Failed to create DPA");

        for ( i = 0 ; i < DSA_GetItemCount(pCacheEntry->hdsaPropertyPages); i++ )
        {
            LPDSPROPERTYPAGE pPage =(LPDSPROPERTYPAGE)DSA_GetItemPtr(pCacheEntry->hdsaPropertyPages, i);
            TraceAssert(pPage);
            hres = StringDPA_AppendStringW(hdpa, pPage->pPageReference, NULL);
            FailGracefully(hres, "Failed to append the string");
        }
    }

    ClassCache_ReleaseClassInfo(&pCacheEntry);

    if (NULL != hdpa)
    {
        for ( i = 0 ; i < DPA_GetPtrCount(hdpa); i++ )
        {
            LPCWSTR pwszPageRef = StringDPA_GetStringW(hdpa, i);
            hres = TabCollector_AddPages(const_cast<LPWSTR>(pwszPageRef),
                                         pPath,
                                         punkSite,
                                         pDataObject,
                                         pAddPageProc,
                                         lParam);
            FailGracefully(hres, "Failed to add page to the list");
        }
    }
    
    hres = S_OK;

exit_gracefully:

    StringDPA_Destroy(&hdpa);

    ClassCache_ReleaseClassInfo(&pCacheEntry);
    ReleaseStgMedium(&medium);
  
    LocalFreeStringW(&ccgi.pAttributePrefix);
    LocalFreeStringW(&ccgi.pUserName);
    LocalFreeStringW(&ccgi.pPassword);
    LocalFreeStringW(&ccgi.pServer);

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ _GetAbsoluteURL
/ ---------------
/   Build a URL from the two components (ADsPath & relative URL).  We grock the
/   registry for the URL formatting component we are going to use and then
/   we bundle that all up into a string.
/
/ In:
/   ppAbsoluteURL = receives the pointer to the absolute URL we are going to be using
/   pRelative URL -> relative URL string
/   pDsObjectName = name of DS object to construct URL for
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT _GetAbsoluteURL(LPTSTR* ppAbsoluteURL, LPTSTR pRelativeURL, LPTSTR pDsObjectName)
{
#if 1
// BUGBUG: Eric this is busted, we used to call DsGetDcName in a very broken way, I 
// BUGBUG: will leave this for you to fix and determine what the virtual root
// BUGBUG: should be for the object.

    return E_NOTIMPL;
#else
    HRESULT hres;
    HKEY hKey = NULL;
    LPTSTR pDcName;
    LPTSTR pTemplate = NULL;
    LPTSTR pObjectURL = NULL;
    TCHAR szBuffer[MAX_PATH];
    TCHAR szGuid[GUIDSTR_MAX];
    DWORD dwLen;

    TraceEnter(TRACE_TABS, "_GetAbsoluteURL");

    // Get the DS server name, without that we cannot do anything

    hres = DsGetServer(&pDcName);
    FailGracefully(hres, "Failed to get DC Name");

    // Lets look up the base string in the registry for the URL that
    // we are trying to build.

    hres = GetKeyForCLSID(CLSID_DsPropertyPages, NULL, &hKey);
    FailGracefully(hres, "Failed to open the DsPropertyPages regstiry key");

    hres = LocalQueryString(&pTemplate, hKey, PROPERTY_PAGES_ROOT);
    FailGracefully(hres, "Failed to get the ASP file registry entry");

    dwLen = lstrlen(pTemplate) + lstrlen(pDcName) + lstrlen(pRelativeURL);

    // If the caller gave us an object to invoke on then add that to the end
    // of the URL as a ?path= parameter.

    if ( pDsObjectName )
    {
        hres = StringToURL(pDsObjectName, &pObjectURL);
        FailGracefully(hres, "Failed to convert the object name to a URL");

        Trace(TEXT("Before encoding -%s-"), pDsObjectName);
        Trace(TEXT("Encoded DS object name -%s-"), pObjectURL);

        dwLen += lstrlen(PATH_IS)+lstrlen(pObjectURL);
    }

    // Now allocate the buffer and fill it with the components
    // that make up our URL.

    hres = LocalAllocStringLen(ppAbsoluteURL, dwLen);
    FailGracefully(hres, "Buffer alloc failed");

    wsprintf(*ppAbsoluteURL, pTemplate, pDcName);
    lstrcat(*ppAbsoluteURL, pRelativeURL);

    if ( pObjectURL )
    {
        lstrcat(*ppAbsoluteURL, PATH_IS);
        lstrcat(*ppAbsoluteURL, pObjectURL);
    }

    hres = S_OK;

exit_gracefully:

    LocalFreeString(&pTemplate);
    LocalFreeString(&pObjectURL);

    if ( hKey )
        RegCloseKey(hKey);

    TraceLeaveResult(hres);
#endif
}


/*-----------------------------------------------------------------------------
/ CDsPropPageDataObject
/----------------------------------------------------------------------------*/

CDsPropPageDataObject::CDsPropPageDataObject(IDataObject* pDataObject, LPWSTR pParameters)
{
    TraceEnter(TRACE_TABS, "CDsPropPageDataObject::CDsPropPage");
    
    m_pDataObject = pDataObject;
    m_pDataObject->AddRef();

    LocalAllocStringW(&m_pParameters, pParameters);

    TraceLeave();
}

CDsPropPageDataObject::~CDsPropPageDataObject()
{
    TraceEnter(TRACE_TABS, "CDsPropPageDataObject::~CDsPropPage");

    DoRelease(m_pDataObject);
    LocalFreeStringW(&m_pParameters);

    TraceLeave();
}

// IUnknown

#undef CLASS_NAME
#define CLASS_NAME CDsPropPageDataObject
#include "unknown.inc"

STDMETHODIMP CDsPropPageDataObject::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[]=
    {
        &IID_IDataObject, (LPDATAOBJECT)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}


/*-----------------------------------------------------------------------------
/ IDataObject methods
/----------------------------------------------------------------------------*/

STDMETHODIMP CDsPropPageDataObject::GetData(FORMATETC* pFmt, STGMEDIUM* pMedium)
{
    HRESULT hres;

    TraceEnter(TRACE_TABS, "CDsPropPageDataObject::GetData");

    if ( !pFmt || !pMedium )
        ExitGracefully(hres, E_INVALIDARG, "Bad arguments to GetData");

    // if its not our clipboard format, or there are no parameters
    // then we call the original handler, otherwise we add our stuff

    if ( !g_cfDsPropPageInfo )
    {
        g_cfDsPropPageInfo = RegisterClipboardFormat(CFSTR_DSPROPERTYPAGEINFO);
        TraceAssert(g_cfDsPropPageInfo);
    }

    if ( (pFmt->cfFormat == g_cfDsPropPageInfo) && m_pParameters )
    {
        LPDSPROPERTYPAGEINFO pPropPageInfo;
        DWORD cbSize = SIZEOF(LPDSPROPERTYPAGEINFO)+StringByteSizeW(m_pParameters);

        // allocate a structure that contains the propage page information
        // we were initialized with

        Trace(TEXT("Property page parameter: %s"), m_pParameters);
        Trace(TEXT("Size of structure for DSPROPPAGEINFO %d"), cbSize);

        hres = AllocStorageMedium(pFmt, pMedium, cbSize, (LPVOID*)&pPropPageInfo);
        FailGracefully(hres, "Failed to allocate the storage medium");

        pPropPageInfo->offsetString = SIZEOF(DSPROPERTYPAGEINFO);
        StringByteCopyW(pPropPageInfo, pPropPageInfo->offsetString, m_pParameters);

        hres = S_OK;                  // success
    }
    else
    {
        hres = m_pDataObject->GetData(pFmt, pMedium);
        FailGracefully(hres, "Failed when calling real IDataObject");
    }

exit_gracefully:

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsPropPageDataObject::GetDataHere(FORMATETC* pFmt, STGMEDIUM* pMedium)
{
    HRESULT hres;

    TraceEnter(TRACE_TABS, "CDsPropPageDataObject::GetDataHere");

    hres = m_pDataObject->GetDataHere(pFmt, pMedium);
    FailGracefully(hres, "Failed when calling real IDataObject");

exit_gracefully:

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsPropPageDataObject::QueryGetData(FORMATETC* pFmt)
{
    HRESULT hres;

    TraceEnter(TRACE_TABS, "CDsPropPageDataObject::QueryGetData");

    hres = m_pDataObject->QueryGetData(pFmt);
    FailGracefully(hres, "Failed when calling real IDataObject");

exit_gracefully:

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsPropPageDataObject::GetCanonicalFormatEtc(FORMATETC* pFmtIn, FORMATETC *pFmtOut)
{
    HRESULT hres;

    TraceEnter(TRACE_TABS, "CDsPropPageDataObject::GetCanonicalFormatEtc");
    
    hres = m_pDataObject->GetCanonicalFormatEtc(pFmtIn, pFmtOut);
    FailGracefully(hres, "Failed when calling real IDataObject");

exit_gracefully:

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsPropPageDataObject::SetData(FORMATETC* pFormatEtc, STGMEDIUM* pMedium, BOOL fRelease)
{
    HRESULT hres;

    TraceEnter(TRACE_TABS, "CDsPropPageDataObject::SetData");
    
    hres = m_pDataObject->SetData(pFormatEtc, pMedium, fRelease);
    FailGracefully(hres, "Failed when calling real IDataObject");

exit_gracefully:

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsPropPageDataObject::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppEnumFormatEtc)
{
    HRESULT hres;

    TraceEnter(TRACE_TABS, "CDsPropPageDataObject::EnumFormatEtc");
    
    hres = m_pDataObject->EnumFormatEtc(dwDirection, ppEnumFormatEtc);
    FailGracefully(hres, "Failed when calling real IDataObject");

exit_gracefully:

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsPropPageDataObject::DAdvise(FORMATETC* pFormatEtc, DWORD advf, IAdviseSink* pAdvSink, DWORD* pdwConnection)
{
    HRESULT hres;

    TraceEnter(TRACE_TABS, "CDsPropPageDataObject::DAdvise");
    
    hres = m_pDataObject->DAdvise(pFormatEtc, advf, pAdvSink, pdwConnection);
    FailGracefully(hres, "Failed when calling real IDataObject");

exit_gracefully:

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsPropPageDataObject::DUnadvise(DWORD dwConnection)
{
    HRESULT hres;

    TraceEnter(TRACE_TABS, "CDsPropPageDataObject::DUnadvise");
    
    hres = m_pDataObject->DUnadvise(dwConnection);
    FailGracefully(hres, "Failed when calling real IDataObject");

exit_gracefully:

    TraceLeaveResult(hres);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CDsPropPageDataObject::EnumDAdvise(IEnumSTATDATA** ppenumAdvise)
{
    HRESULT hres;

    TraceEnter(TRACE_TABS, "CDsPropPageDataObject::EnumDAdvise");
    
    hres = m_pDataObject->EnumDAdvise(ppenumAdvise);
    FailGracefully(hres, "Failed when calling real IDataObject");

exit_gracefully:

    TraceLeaveResult(hres);
}
