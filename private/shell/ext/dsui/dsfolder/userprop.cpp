/*----------------------------------------------------------------------------
/ Title;
/   userprop.cpp
/
/ Authors;
/   David De Vorchik (daviddv)
/
/ Notes;
/   Hook properties for users, groups and contacts to use WAB to
/   show the property UI.
/----------------------------------------------------------------------------*/
#include "pch.h"
#include "wab.h"
#include "lm.h"
#include "ntdsapi.h"
#include "dsgetdc.h"
#include "winldap.h"
#include "wininet.h"
#pragma hdrstop


#if WAB_SUPPORT

/*-----------------------------------------------------------------------------
/ Information we use for WAB properties
/----------------------------------------------------------------------------*/

class CDsUserProperties : public IDsFolderProperties, CUnknown
{
    public:
        // IUnknown
        STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject);
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        // IDsFolderProperties
        STDMETHOD(ShowProperites)(THIS_ HWND hwndParent, IDataObject* pDataObject);
};

//
// IUnknown bits
//

#undef CLASS_NAME
#define CLASS_NAME CDsUserProperties
#include "unknown.inc"

STDMETHODIMP CDsUserProperties::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    INTERFACES iface[] =
    {
        &IID_IDsFolderProperties, (IDsFolderProperties*)this,
    };

    return HandleQueryInterface(riid, ppvObject, iface, ARRAYSIZE(iface));
}

//
// Handle creating an instance of IDsFolderProperties for talking to WAB
//

STDAPI CDsUserProperties_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CDsUserProperties *pdup = new CDsUserProperties();
    if ( !pdup )
        return E_OUTOFMEMORY;

    HRESULT hres = pdup->QueryInterface(IID_IUnknown, (void **)ppunk);
    pdup->Release();
    return hres;
}


/*----------------------------------------------------------------------------
/ IDsFolderProperties
/----------------------------------------------------------------------------*/

STDMETHODIMP CDsUserProperties::ShowProperites(HWND hwndParent, IDataObject* pDataObject)
{
    HRESULT hr;
    HMODULE hWAB = NULL;
    LPTSTR pWABLocation = NULL;
    LPWABOPEN lpfnWABOpen = NULL;
    LPWABOBJECT pWABObject = NULL;
    LPADRBOOK pAddrBook = NULL;
    HKEY hKey = NULL;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium = { TYMED_NULL };
    LPDSOBJECTNAMES pDsObjectNames;
    LPWSTR pURL;
    CHAR szEncodedURL[INTERNET_MAX_URL_LENGTH];
    DWORD dwLen = ARRAYSIZE(szEncodedURL);
    IADsPathname* pPathname = NULL;
    BSTR bstrPathWithServer = NULL;
    PDOMAIN_CONTROLLER_INFOW pDcInfo = NULL;
    LPWSTR pDomainController;
    DWORD dwError;
    USES_CONVERSION;

    TraceEnter(TRACE_USERPROP, "CDsUserProperties::ShowProperites");

    // Do we have an IDataObject, if so then lets the the DSOBJECTNAMES from
    // it and extract the URL we are going to use to display the properties
    // for the objects.

    if ( !pDataObject )
        ExitGracefully(hr, E_UNEXPECTED, "No pDataObject, failing");

    RegisterDsClipboardFormats();
    fmte.cfFormat = g_cfDsObjectNames;

    hr = pDataObject->GetData(&fmte, &medium);
    FailGracefully(hr, "Failed ot get data from IDataObject");

    pDsObjectNames = (LPDSOBJECTNAMES)medium.hGlobal;

    Trace(TEXT("Selection count is %d"), pDsObjectNames->cItems);

    if ( pDsObjectNames->cItems > 1 )
        ExitGracefully(hr, S_FALSE, "Selection is too big!");

    pURL = (LPWSTR)ByteOffset(pDsObjectNames, pDsObjectNames->aObjects[0].offsetName);
    TraceAssert(pURL);

    Trace(TEXT("URL for object is %s"), W2T(pURL));

    // lets load the WAB DLL, it doesn't live on the path so we need to
    // read its location from the registry.

// BUGBUG: daviddv - this should be CoCreate'able.  Using LoadLibrary is wrong.

    if ( ERROR_SUCCESS != (RegOpenKey(HKEY_LOCAL_MACHINE, WAB_DLL_PATH_KEY, &hKey)) )
        ExitGracefully(hr, E_FAIL, "Failed to get the WAB key under HKLM");

    hr = LocalQueryString(&pWABLocation, hKey, NULL);
    FailGracefully(hr, "Failed to read the WAB location");
    
    Trace(TEXT("pWABLocation: %s"), pWABLocation);

    hWAB = LoadLibrary(pWABLocation);
    TraceAssert(hWAB);
    
    if ( !hWAB )
        ExitGracefully(hr, E_FAIL, "Failed to LoadLibrary the WAB DLL");     
    
    lpfnWABOpen = (LPWABOPEN)GetProcAddress(hWAB, "WABOpen");
    TraceAssert(lpfnWABOpen);

    if ( !lpfnWABOpen )
        ExitGracefully(hr, E_FAIL, "Failed to get the 'WABOpen' entry point");

    hr = lpfnWABOpen(&pAddrBook, &pWABObject, NULL, 0);
    FailGracefully(hr, "Failed in lpfnWABOpen");

    if ( !pAddrBook || !pWABObject )
        ExitGracefully(hr, E_UNEXPECTED, "Either pAddrBook or pWABObject == NULL");

    // lets get the server we have been using

    dwError = DsGetDcNameW(NULL, NULL, NULL, NULL, DS_DIRECTORY_SERVICE_REQUIRED, &pDcInfo);    

    if ( dwError != ERROR_SUCCESS )
        ExitGracefully(hr, E_FAIL, "Failed to get the DC name");

    if ( !(pDcInfo->Flags & DS_DS_FLAG) )
        ExitGracefully(hr, E_FAIL, "Failed to get the DS DC");

    Trace(TEXT("Domain controller name %s"), W2T(pDcInfo->DomainControllerName));

    // now retreive the entire name (domain etc) encode that and lets pass it
    // down to WAB.

    // we have the interface to WAB, so lets try and format the URL into something
    // that WAB will understand.  it cannot cope with serverless names, and it
    // expects the URL to be encoded.

    hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER, IID_IADsPathname, (LPVOID*)&pPathname);
    FailGracefully(hr, "Failed to get the IADsPathname interface");

    hr = pPathname->Set(pURL, ADS_SETTYPE_FULL);
    FailGracefully(hr, "Failed to set the URL into IADsPathname interface");

    pDomainController = pDcInfo->DomainControllerName;

    if ( (pDomainController[0] == L'\\') && (pDomainController[1] == L'\\') )
    {
        TraceMsg("Triming the \\\\ from the name");
        pDomainController += 2;
    }

    hr = pPathname->Set(pDomainController, ADS_SETTYPE_SERVER);
    FailGracefully(hr, "Failed to set the server name");    

    hr = pPathname->Retrieve(ADS_FORMAT_X500, &bstrPathWithServer);
    FailGracefully(hr, "Failed to get URL with server name");

    hr = UrlEscapeA(W2A(bstrPathWithServer), szEncodedURL, &dwLen, 0);
    FailGracefully(hr, "Failed to convert URL to encoded format");

    Trace(TEXT("Trying to display properties on: %s"), A2T(szEncodedURL)); 

    hr = pWABObject->LDAPUrl(pAddrBook, hwndParent, LDAP_AUTH_SICILY, (LPTSTR)szEncodedURL, NULL);
    FailGracefully(hr, "Failed when calling IWABObject::LDAPUrl");

    hr = S_OK;

exit_gracefully:

    DoRelease(pWABObject);
    DoRelease(pAddrBook);
    DoRelease(pPathname);

    LocalFreeString(&pWABLocation);    
    SysFreeString(bstrPathWithServer);

    ReleaseStgMedium(&medium);

    if ( pDcInfo )
        NetApiBufferFree(pDcInfo);
                       
    if ( hKey )
        RegCloseKey(hKey);

    if ( hWAB )
        FreeLibrary(hWAB);

    TraceLeaveResult(hr);
}

#endif
