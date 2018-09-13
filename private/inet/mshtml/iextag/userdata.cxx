//===============================================================
//
//  userdata.cxx : Implementation of the CPersistUserData Peer
//
//  Synposis : this class is repsponsible for handling the "generic"
//      persistence of author data in the expanded cookie cahce
//
//===============================================================
                                                              
#include "headers.h"

#ifndef __X_IEXTAG_H_
#define __X_IEXTAG_H_
#include "iextag.h"
#endif

#ifndef __X_USERDATA_HXX_
#define __X_USERDATA_HXX_
#include "userdata.hxx"
#endif

#ifndef __X_UTILS_HXX_
#define __X_UTILS_HXX_
#include "utils.hxx"
#endif

#include <shlobj.h>

// Static data members

// User name related
TCHAR CPersistUserData::s_rgchUserData[] = TEXT("userdata");

// This global string will store the string userdata:username@. This is useful is several 
// situations and we want to avoid computing this over and over again. 
TCHAR CPersistUserData::s_rgchCacheUrlPrefix[ARRAY_SIZE(s_rgchUserData) + 1 /* for : */ +  MAX_PATH + 1 /* for @ */];
DWORD CPersistUserData::s_cchCacheUrlPrefix = 0;
BOOL  CPersistUserData::s_bCheckedUserName = FALSE;
BOOL  CPersistUserData::s_bCacheUrlPrefixRet = FALSE;


// cache container related
HRESULT CPersistUserData::s_hrCacheContainer;
BOOL  CPersistUserData::s_bCheckedCacheContainer = FALSE;

// Values below are the default ones used if not administrator provided override is seen.
// These values are in Kilo Bytes. 

// Initialize with hard-coded values if not admin limit specified.
DWORD CPersistUserData::s_rgdwDomainLimit[URLZONE_UNTRUSTED + 1] = 
        { 1024, 10240, 1024, 1024, 640 };
DWORD CPersistUserData::s_dwUnkZoneDomainLimit = 640;

DWORD CPersistUserData::s_rgdwDocLimit[URLZONE_UNTRUSTED + 1] = 
        { 128, 512, 128, 128, 64 };
DWORD CPersistUserData::s_dwUnkZoneDocLimit = 64;


// Critical sections for synchronization.

CRITICAL_SECTION CPersistUserData::s_csCacheUrlPrefix;
CRITICAL_SECTION CPersistUserData::s_csCacheContainer;
CRITICAL_SECTION CPersistUserData::s_csSiteTable;

CSiteTable * CPersistUserData::s_pSiteTable = NULL; 

DWORD CPersistUserData::s_dwClusterSizeMinusOne;
DWORD CPersistUserData::s_dwClusterSizeMask;
                     
// Registry location and keys to read the admin specified values from
#define KEY_PERSISTENCE     TEXT("Software\\Policies\\microsoft\\Internet Explorer\\Persistence")
#define VALUE_DOCUMENTLIMIT      TEXT("DocumentLimit")
#define VALUE_DOMAINLIMIT   TEXT("DomainLimit")

// Some character definitions which are useful in this file
const TCHAR chSLASH = TEXT('/');
const TCHAR chBACKSLASH = TEXT('\\');
const TCHAR chAT = TEXT('@');
const TCHAR chCOLON = TEXT(':');
// Character at the begining of a Unicode file.
const WCHAR wchLEADUNICODEFILE = 0xFEFF ;

const TCHAR rgchAnyUser[] = TEXT("anyuser");
const CHAR rgchRelAppData[] = "Microsoft\\Internet Explorer\\UserData";
const CHAR rgchRelCookies[] = "..\\UserData";

#ifndef unix
const CHAR schDIR_SEPERATOR = '\\';
#else
const CHAR schDIR_SEPERATOR = '/';
#endif

typedef HRESULT (*PFNSHGETFOLDERPATHA)(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPSTR pszPath);

//+----------------------------------------------------------------------------
//
//  member : CPersistUserData::~CPersistUserData
//
//-----------------------------------------------------------------------------

CPersistUserData::~CPersistUserData()
{
    ClearInterface(&_pPeerSite);
    ClearInterface(&_pPeerSiteOM);
    ClearOMInterfaces();
}

//+----------------------------------------------------------------------------
//
//  Member : GlobalInit  (static)
//
//  Synopsis : this method is called once a DLL attach time to init static variables.
//
//-----------------------------------------------------------------------------
BOOL CPersistUserData::GlobalInit()
{
    // Initialize the critical sections we use.
    InitializeCriticalSection(&s_csCacheUrlPrefix);
    InitializeCriticalSection(&s_csCacheContainer);
    InitializeCriticalSection(&s_csSiteTable);

    // Read out admin specified limits if any.
    DWORD   dwLimit;
    DWORD   dwSize = sizeof(dwLimit);
    DWORD   dwType;
    HUSKEY  huskey = NULL;

    if ((SHRegOpenUSKey(KEY_PERSISTENCE, KEY_QUERY_VALUE, NULL, &huskey, FALSE)) == NOERROR)
    {
        // Should be more than enough to represent a zone id as a string.
        TCHAR rgchZone[10];

        for ( int i = 0 ; i < ARRAY_SIZE(s_rgdwDomainLimit) ; i++ )
        {
            HUSKEY husZoneKey = NULL;
            wnsprintf(rgchZone, ARRAY_SIZE(rgchZone), TEXT("%d"), i);

            if (SHRegOpenUSKey(rgchZone,  KEY_QUERY_VALUE, huskey, &husZoneKey, FALSE) == NOERROR)
            {  
                if ( (SHRegQueryUSValue(husZoneKey, VALUE_DOMAINLIMIT, &dwType, 
                                &dwLimit, &dwSize, FALSE, NULL, 0) == NOERROR) && 
                  
                      (dwType == REG_DWORD)
                    )
                {
                    s_rgdwDomainLimit[i] = dwLimit;
                }
                dwSize = sizeof(dwLimit);

                if ( (SHRegQueryUSValue(husZoneKey, VALUE_DOCUMENTLIMIT, &dwType, 
                                &dwLimit, &dwSize, FALSE, NULL, 0) == NOERROR) && 
                  
                      (dwType == REG_DWORD)
                    )
                {
                    s_rgdwDocLimit[i] = dwLimit;
                }
                SHRegCloseUSKey(husZoneKey);
            }

            dwSize = sizeof(dwLimit);
        }
    }

    if (huskey)
    {
        SHRegCloseUSKey(huskey);
    }

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Member : GlobalUninit  (static)
//
//  Synopsis : this method is called at DLL detach time to uninit static variables.
//
//-----------------------------------------------------------------------------

BOOL CPersistUserData::GlobalUninit( )
{
    DeleteSiteTable();
    DeleteCriticalSection(&s_csCacheUrlPrefix);
    DeleteCriticalSection(&s_csCacheContainer);
    DeleteCriticalSection(&s_csSiteTable);
    return TRUE;
}
        
    
//+----------------------------------------------------------------------------
//
//  Member : Init
//
//  Synopsis : this method is called by MSHTML.dll to initialize peer object
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CPersistUserData::Init(IElementBehaviorSite * pPeerSite)
{
    if (!pPeerSite)
        return E_INVALIDARG;

    _pPeerSite = pPeerSite;
    _pPeerSite->AddRef();


    return _pPeerSite->QueryInterface(IID_IElementBehaviorSiteOM, 
                                         (void**)&_pPeerSiteOM);
}

//+----------------------------------------------------------------------------
//
//  Member : Notify
//
//  Synopsis : peer Interface, called for notification of document events.
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CPersistUserData::Notify(LONG lEvent, VARIANT *)
{
    return S_OK;
}


//+-----------------------------------------------------------------------
//
//  Member : ClearOMInterfaces ()
//
//  Synopsis : this helper function is called after the Save/Load persistenceCache
//      operations are finished, and is responsible for freeing up any of hte OM 
//      interfaces that were cached.
//
//------------------------------------------------------------------------
void
CPersistUserData::ClearOMInterfaces()
{
    ClearInterface(&_pRoot);
    ClearInterface(&_pInnerXMLDoc);
}

//+----------------------------------------------------------------
//
// Member : initXMLCache
//
//  Synopsis : Helper function, this is responsible for cocreateing the 
//      XML object and initializing it for use.
//              the fReset flag indicates that the contents of teh xmlCache 
//      should be cleared  It is assumed in this case that the cache already exists
//
//-----------------------------------------------------------------
HRESULT
CPersistUserData::initXMLCache(BOOL fReset/*=false*/)
{
    HRESULT             hr = S_OK;
    BSTR                bstrStub = NULL;
    VARIANT_BOOL        vtbCleanThisUp;
    IObjectSafety     * pObjSafe = NULL;

    if (!fReset)
    {
        // are we already initialized?
        if (_pInnerXMLDoc && _fCoCreateTried)
            goto Cleanup;

        // protect against expensive retries if over network
        _fCoCreateTried = true;  

        hr = CoCreateInstance(CLSID_DOMDocument,
                                  0,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IXMLDOMDocument,
                                  (void **)&_pInnerXMLDoc);

        if (hr)
            goto Cleanup;

        hr = _pInnerXMLDoc->QueryInterface(IID_IObjectSafety, 
                                           (void **)&pObjSafe);
        if (hr)
            goto ErrorCase;

        hr = pObjSafe->SetInterfaceSafetyOptions( IID_NULL, 
                                                  INTERFACE_USES_SECURITY_MANAGER, 
                                                  INTERFACE_USES_SECURITY_MANAGER);
        if (hr)
            goto ErrorCase;
    }

    // one last error check
    if (_pInnerXMLDoc)
    {
        // now create some XML tree
        bstrStub = SysAllocString(L"<ROOTSTUB />");
        if (bstrStub )
        {
            hr = _pInnerXMLDoc->loadXML(bstrStub, &vtbCleanThisUp);
            SysFreeString( bstrStub );
            if (hr)
                goto ErrorCase;
        }

        // BUGBUG  - we no longer need to really cache the _pRoot, why not
        hr = _pInnerXMLDoc->get_documentElement( &_pRoot);
        if (hr)
            goto ErrorCase;
    }


Cleanup:
    ReleaseInterface(pObjSafe);

    if (hr == S_FALSE)
        hr = S_OK;
    return hr ;

ErrorCase:
    ClearInterface(&_pInnerXMLDoc);
    goto Cleanup;
}

//+----------------------------------------------------------------
//
//  Member : SecureDomainAndPath
//
//  synopsis : this helper function is responsisble for the domain
//      and path level security when a user wants to save or load a 
//      store.
//          We use the security manager if possible so that HTA's
//      are handled properly.
//          This will return a substring of the pstrFileName, if the
//      user explicitly specified a store.  
//      The logic of this function is basically:
//          if pstrFileName is only a name return TRUE (and use the DirPath)
//          else { extract the store from pstrFileName
//                 compare the Name's path to the DirPath via domain et al
//                      using the security manager
//                 if that passes, then do a path comparison, since the 
//                      security mgr only handles domains
//
//    valid formats for pstrFileName:
//      Foo
//      userdata:foo
//      file://c:/temp/foo
//      userdata:file://c:/temp/foo
//+----------------------------------------------------------------
BOOL
CPersistUserData::SecureDomainAndPath(LPOLESTR pstrFileName,
                                      LPOLESTR *ppstrStore,
                                      LPOLESTR *ppstrPath,
                                      LPOLESTR *ppstrName)
{
    BOOL               fRet = FALSE;
    DWORD              cchPath; 
    LPTSTR             pszPath = NULL, pStore = NULL;
    LPOLESTR           pstrDirPath  = NULL;
    IHTMLElement     * pPeerElement   = NULL;
    IInternetSecurityManager * pSecMgr = NULL;
    DWORD              dwPolicy = 0;
    DWORD              dwZone = URLZONE_INVALID;

    if (!pstrFileName)
        goto Cleanup;

    Assert(ppstrStore && ppstrPath && ppstrName);

    *ppstrStore = *ppstrPath = *ppstrName = NULL;

    // get the doc's path
    if (FAILED(GetDirPath(&pstrDirPath)))
        goto Cleanup;

    Assert(pstrDirPath != NULL);

    // Only allow userdata saves and loads from supported schemes. 
    if (!IsSchemeAllowed(pstrDirPath))
        goto Cleanup;

    // pull the store off the front
    //-----------------------------------------------
    pStore = _tcschr(pstrFileName, _T(':'));
    if (pStore)
    {
        // we found one, if the next character is a / or a \
        //   then the format must be scheme://  otherwise
        //   userdata:http:// or store:file://c:\temp  is assumed
        //-------------------------------------------------
        if ( pStore[1]  != chSLASH && pStore[1] != chBACKSLASH)
        {
            long cchStore = (long)(pStore - pstrFileName);

            // copy the 'store:' over to the out parameter,
            // advance the Name to 
            (*ppstrStore) = new TCHAR [cchStore+1];
            if (!*ppstrStore)
                goto Cleanup;
                
            memcpy(*ppstrStore, pstrFileName, cchStore);
            (*ppstrStore)[cchStore] = _T('\0');
            pstrFileName = pStore +1;
        }
    }
    
    // first determine if the pstrFileName is just a name or 
    // a fully specified path. if there is a slash, then its
    // fully specified.
    //------------------------------------------------------
    if (!_tcschr(pstrFileName, _T('\\')) && 
        !_tcschr(pstrFileName, _T('/')))
    {
        long cchName = (_tcslen(pstrFileName)+1);
        cchPath = (_tcslen(pstrDirPath)+1);

        *ppstrName = new TCHAR [cchName];
        *ppstrPath = new TCHAR [cchPath];
        if (!*ppstrName || !*ppstrPath)
            goto Cleanup;
            
        memcpy(*ppstrName, pstrFileName, cchName*sizeof(TCHAR));
        memcpy(*ppstrPath, pstrDirPath, cchPath*sizeof(TCHAR));

        fRet = TRUE;
        // Fall through and make sure that the zone policy for pstrDirPath 
        // allows us to do userdata operations.
    }

    if (FAILED(CoInternetCreateSecurityManager(NULL, &pSecMgr, 0)))
    {
        fRet = FALSE;
        goto Cleanup;
    }

    if (FAILED(pSecMgr->MapUrlToZone(pstrDirPath, &dwZone, 0)))
    {
        fRet = FALSE;
        goto Cleanup;
    }

    SetZone(dwZone);
                            
    if ( FAILED(pSecMgr->ProcessUrlAction(pstrDirPath,
                                URLACTION_HTML_USERDATA_SAVE,
                                (BYTE *)&dwPolicy,
                                sizeof(dwPolicy),
                                NULL,
                                0,
                                PUAF_NOUI,
                                0))
            ||
          (GetUrlPolicyPermissions(dwPolicy) != URLPOLICY_ALLOW)
        )
    {
        fRet = FALSE;
        goto Cleanup;
    }

    // If fRet is TRUE it implies that the filename was a relative path.
    // In this case we don't need to do further checks, we can return 
    // success at this point.
    if (fRet)
        goto Cleanup;

    // at this point the store is pulled of the front (if it was specified);
    // the pstrFileName is ready to be domain and path compared for security
    //
    // do security mgr (domain) test first
    // ---------------------------------------------------------------------
    {
        TCHAR  achURL[INTERNET_MAX_URL_LENGTH];
        BYTE   abSID1[MAX_SIZE_SECURITY_ID];
        BYTE   abSID2[MAX_SIZE_SECURITY_ID];
        DWORD  cbSID1 = ARRAY_SIZE(abSID1);
        DWORD  cbSID2 = ARRAY_SIZE(abSID2);
        DWORD  dwSize;


        // Unescape the URL of the name. (be safe)
        //------------------------------------------
        if (FAILED(CoInternetParseUrl(pstrFileName, PARSE_ENCODE, 0, achURL, 
                                      ARRAY_SIZE(achURL), &dwSize, 0)))
            goto Cleanup;

        if (FAILED(pSecMgr->GetSecurityId(pstrDirPath, (LPBYTE)&abSID1, &cbSID1, 0)))
            goto Cleanup;

        if (FAILED(pSecMgr->GetSecurityId(achURL, (LPBYTE)&abSID2, &cbSID2, 0)))
            goto Cleanup;

        // compare the security ID's
        if (cbSID1 != cbSID2 || (0 != memcmp(abSID1, abSID2, cbSID1)))
            goto Cleanup;
    }

    // at this point, we know that the domains match so we now need to
    // make sure that if there are paths, then they follow the rules:
    //    if dirPath is "a\b\c"  then the Name path
    // can be :  "\a\b\c\d\name" or "\a\b\name or a\name"
    // but NOT : "\g\name"  or "a\f\name" or "a\b\f\name"
    //----------------------------------------------------------------

    URL_COMPONENTS ucFile, ucDoc;
    memset(&ucFile, 0, sizeof(ucFile));
    memset(&ucDoc, 0, sizeof(ucDoc));

    ucFile.dwStructSize = sizeof(ucFile);
    ucFile.lpszUrlPath = NULL; 
    ucFile.dwUrlPathLength = 1;

    ucDoc.dwStructSize = sizeof(ucDoc);
    ucDoc.lpszUrlPath = NULL; 
    ucDoc.dwUrlPathLength = 1;

    if (!InternetCrackUrl(pstrFileName, 0, 0, &ucFile))
        goto Cleanup;

    if (!InternetCrackUrl(pstrDirPath, 0, 0, &ucDoc))
        goto Cleanup;

    // Strip off the document from the filepath.
    // because the doc path came from DirPath() I know
    // that it has no trailing name or slash
    cchPath = ucFile.dwUrlPathLength;
    pszPath = ucFile.lpszUrlPath;

    for (;cchPath > 0; cchPath--) 
    {
        if ( pszPath[cchPath - 1]  == chSLASH || pszPath[cchPath - 1] == chBACKSLASH)
        {
            cchPath--;  // Skip over the /
            break;
        }
    }

    // yes, this changes the passed in string... SO DON"T change cchPath or
    // add a "goto Cleanup" in the following block of code.
    {
        TCHAR  chReplace = pszPath[cchPath];
        pszPath[cchPath] = _T('\0'); // chop into 2 parts

        // now, one of the two paths better be a substring of the other 
        //  how do we do this case - insensitive?
        //--------------------------------------------------------------------
        if (_tcsistr(ucDoc.lpszUrlPath, pszPath) ||
            _tcsistr(pszPath, ucDoc.lpszUrlPath))
        {
            // we have a valid name!!! 
            fRet = TRUE;

            // extract the File Name
            long cch = (ucFile.dwUrlPathLength- cchPath);
            *ppstrName = new TCHAR [cch];
            if (!*ppstrName)
            {
                fRet = FALSE;
                goto Cleanup;
            }
            
            memcpy(*ppstrName, (LPTSTR)&(pszPath[cchPath+1]), cch*sizeof(TCHAR));

            // and the path
            cch = (1+cchPath);
            *ppstrPath = new TCHAR [cch];
            memcpy(*ppstrPath, pszPath, cch*sizeof(TCHAR));
        }
        pszPath[cchPath] = chReplace;
    }


Cleanup:
    delete [] pstrDirPath;
    ReleaseInterface( pPeerElement );
    ReleaseInterface( pSecMgr );
    return fRet;
}

//+----------------------------------------------------------------
//
// Member: IsSchemeAllowed
// Synopsis: Given the directory path of the document trying to save the 
//           userdata object determines if the scheme (i.e. protocol) it uses
//           is supported. 
//------------------------------------------------------------------

BOOL CPersistUserData::IsSchemeAllowed(LPCOLESTR pStrDirPath)
{
    PARSEDURL pu;
    BOOL fRet = FALSE;

    pu.cbSize = sizeof(pu);
    HRESULT hr = ParseURL(pStrDirPath, &pu);

    if (hr == S_OK)
    {
        switch(pu.nScheme)
        {
            case URL_SCHEME_MAILTO:
            case URL_SCHEME_NEWS:
            case URL_SCHEME_NNTP:
            case URL_SCHEME_TELNET:
            case URL_SCHEME_WAIS:
            case URL_SCHEME_SNEWS:
            case URL_SCHEME_SHELL:
            case URL_SCHEME_JAVASCRIPT:
            case URL_SCHEME_VBSCRIPT:
            case URL_SCHEME_ABOUT:
            case URL_SCHEME_MK:
                fRet = FALSE;
                break;
            default:
                fRet = TRUE;
                break;
        }
    }

    return fRet;
}

//+----------------------------------------------------------------
//
// Member : GetDirPath
//
// Synopsis :  Helper function to figures out the directory which the current doc lives in
//          For example if the doc URL is http://www.foo.com/bar/goo/abc.htm?x=y
//          This function will extract out http://www.foo.com/bar/goo
//          
//-----------------------------------------------------------------

HRESULT 
CPersistUserData::GetDirPath(LPOLESTR *ppDirPath)
{
    HRESULT          hr;
    IHTMLElement   * pPeerElement   = NULL;
    IDispatch      * pdispBrowseDoc = NULL;
    IHTMLDocument2 * pBrowseDoc     = NULL;
    BSTR             bstrURL = NULL;
    LPOLESTR         pStr = NULL;

    if (ppDirPath == NULL)
    {
        hr = E_POINTER;
        return hr;
    }

    _pPeerSite->GetElement(&pPeerElement);
    Assert(pPeerElement != NULL);

    if ((hr = pPeerElement->get_document(&pdispBrowseDoc)) != S_OK)
        goto cleanup;

    if ((hr = pdispBrowseDoc->QueryInterface(IID_IHTMLDocument2, 
                                        (void**)&pBrowseDoc)) != S_OK)
        goto cleanup;

    if ((hr = pBrowseDoc->get_URL(&bstrURL)) != S_OK)
        goto cleanup;
    
    URL_COMPONENTS uc;
    memset(&uc, 0, sizeof(uc));

    uc.dwStructSize = sizeof(uc);
    uc.lpszScheme = NULL;
    uc.dwSchemeLength = 1;      // let's us get the scheme back.
    uc.lpszHostName = NULL;
    uc.dwHostNameLength = 1;
    uc.lpszUrlPath = NULL;
    uc.dwUrlPathLength = 1;
    // Important: even though we don't give a rat's ass about the extra info
    // we have to ask for it. If we don't InternetCrackUrl returns everything
    // after the HostName as the urlPath. We don't want the query string or the 
    // fragment in our dwUrlPathLength.

    uc.lpszExtraInfo = NULL;
    uc.dwExtraInfoLength = 1;

    if (!InternetCrackUrl(bstrURL, 0, 0, &uc))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    // Allocate enough memory for the whole URL. 
    DWORD dwStrLen;

    dwStrLen = lstrlen(bstrURL) + 1;

    pStr = new WCHAR[dwStrLen];

    if (pStr == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    // Strip off the document from the path. 
    DWORD cchPath; 
    LPTSTR pszPath;

    cchPath = uc.dwUrlPathLength;
    pszPath = uc.lpszUrlPath;

    for (;cchPath > 0; cchPath--) 
    {
        if ( pszPath[cchPath - 1]  == chSLASH  || pszPath[cchPath - 1] == chBACKSLASH)
        {
            cchPath--;  // Skip over the /
            break;
        }
    }

    if (uc.dwUrlPathLength && !cchPath)
    {
        // this case can only happen when there is no path. e.g. http://server/document.htm
        // we can't just use 0 for cchPath since InternetCreateUrl won't properly remove
        // the document name.  so let cchPath=1, and point to the "/" this will allow ICU to
        // create the url properly without the name.
        cchPath = 1;
    }

    uc.dwUrlPathLength = cchPath;
    // When creating the path we don't want the extra info to be included in the 
    // URL.
    uc.lpszExtraInfo = NULL;
    uc.dwExtraInfoLength = 0;

    if (!InternetCreateUrl(&uc, 0, pStr, &dwStrLen))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    // It is important that we do this so that www%20foo%20com and www.foo.com are 
    // treated as the same site. 
    hr = UrlUnescape(pStr, NULL, 0, URL_UNESCAPE_INPLACE);
    if (hr != S_OK)
        goto cleanup;
    
    *ppDirPath = pStr;
    pStr = NULL;    // So we don't free it...
    hr = S_OK;

cleanup:

    ReleaseInterface(pPeerElement);
    ReleaseInterface(pdispBrowseDoc);
    ReleaseInterface(pBrowseDoc);
    SysFreeString(bstrURL);
    delete [] pStr;
        
    return hr;
}

//+----------------------------------------------------------------
//
// Member : save
//
//-----------------------------------------------------------------
STDMETHODIMP 
CPersistUserData::save(BSTR  bstrName)
{
    HRESULT       hr = S_OK;
    BSTR          bstrValue = NULL;
    DWORD         cbDataLen = 0;
    LPOLESTR      pstrCacheStore = NULL;
    LPOLESTR      pstrCachePath  = NULL;
    LPOLESTR      pstrCacheName  = NULL;
    

    // verify parameters
    //---------------------------------------
    if (!bstrName || !SysStringLen(bstrName))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!_pInnerXMLDoc || !_pRoot)
        goto Cleanup;

    // do a security check between the DirPath() and the bstrName 
    // the user provided.  This bool test also returns the various store
    // names
    //---------------------------------------------------------
    if (!SecureDomainAndPath(bstrName, 
                             &pstrCacheStore, 
                             &pstrCachePath, 
                             &pstrCacheName))
    {
        hr = E_ACCESSDENIED;
        goto Cleanup;
    }

    // security test passed. prepare the data
    // get the text of the XML document and
    // load the variant 
    //-----------------------------------------------------------
    hr = _pInnerXMLDoc->get_xml( &bstrValue);
    if (hr)
        goto Cleanup;

    // now convert the data to an appendable/useable format
    cbDataLen = SysStringLen(bstrValue) * sizeof(WCHAR);

    // If there was no explicit store, or explictly "userdata"
    // then pass this off to the userData store.  otherwise
    // (for future implementions) pass this off to the appropriate
    // storage mechanism.
    //
    // BUGBUG: TODO - change this API to take a IPersist<something>
    //   interface rather than a string/byte pointer.  This will make
    //   it more general and veratile for the future.
    //------------------------------------------------------------
    if (!pstrCacheStore || !_tcscmp(pstrCacheStore, s_rgchUserData) )
    {
        hr = SaveUserData(pstrCachePath, 
                          pstrCacheName, 
                          bstrValue, 
                          cbDataLen, 
                          _ftExpires, 
                          UDF_BSTR); 
    }
    // else
    //  save to alternative store: NYI

Cleanup:
    delete [] pstrCacheStore;
    delete [] pstrCacheName;
    delete [] pstrCachePath;
    SysFreeString(bstrValue);

    return hr ;
}


//---------------------------------------------------------------------------
//
//  Member:     CPersistUserData::load
//
//  Synopsis:   IHTMLUserData OM method implementation
//
//---------------------------------------------------------------------------

STDMETHODIMP
CPersistUserData::load (BSTR strName)
{
    HRESULT        hr = S_OK;
    BSTR           strData = NULL;
    DWORD          dwSize = 0;
    LPOLESTR       pstrCacheStore = NULL;
    LPOLESTR       pstrCachePath  = NULL;
    LPOLESTR       pstrCacheName  = NULL;
    VARIANT_BOOL   vtbCleanThisUp;

    if (!strName || !SysStringLen(strName))
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = initXMLCache();
    if (hr)
        goto Cleanup;

    // do a security check between the DirPath() and the bstrName 
    // the user provided.  This bool test also returns the various store
    // names
    //---------------------------------------------------------
    if (!SecureDomainAndPath(strName, 
                             &pstrCacheStore, 
                             &pstrCachePath, 
                             &pstrCacheName))
    {
        hr = E_ACCESSDENIED;
        goto Cleanup;
    }

    // If there was no explicit store, or explictly "userdata"
    // then pass this off to the userData store.  otherwise
    // (for future implementions) pass this off to the appropriate
    // storage mechanism.
    //
    // BUGBUG: TODO - change this API to take a IPersist<something>
    //   interface rather than a string/byte pointer.  This will make
    //   it more general and veratile for the future.
    //------------------------------------------------------------
    if (!pstrCacheStore || !_tcscmp(pstrCacheStore, s_rgchUserData) )
    {
        // If the UDF_BSTR flag is specified we get back a BSTR in 
        hr = LoadUserData(pstrCachePath, 
                          pstrCacheName, 
                          (LPVOID *)&strData, 
                          &dwSize, 
                          UDF_BSTR);
    }
    // else
    //     load from alternate store : NYI 

    if (hr)  // Couldn't read the data.
        goto Cleanup;

    ClearInterface(&_pRoot);
    hr = _pInnerXMLDoc->loadXML(strData, &vtbCleanThisUp);
    if (hr)
        goto Cleanup;

    hr = _pInnerXMLDoc->get_documentElement( &_pRoot );

Cleanup:
    if (hr)
    {
        // If the document didn't exist return an empty document.
        if ( hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) ||
             hr == S_FALSE )
            hr = S_OK;

        // we need to make sure that there is a usable cache 
        // coming out of this function.  Even if we were unable 
        // to loadit
        initXMLCache(true);
    }
    delete [] pstrCacheStore;
    delete [] pstrCacheName;
    delete [] pstrCachePath;
    SysFreeString(strData);

    return hr;
}


//---------------------------------------------------------------------------
//
//  Member:     CPersistUserData::get_xmlDocument
//
//  Synopsis:   IHTMLUserData OM proeprty implementation. this is the default 
//                  property for this object, and as such it exposes the XMLOM
//                  of the user data.
//
//---------------------------------------------------------------------------

STDMETHODIMP
CPersistUserData::get_XMLDocument (IDispatch ** ppDisp)
{
    HRESULT hr = S_OK;

    if (!ppDisp)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = initXMLCache();
    if (hr)
        goto Cleanup;

    if (_pInnerXMLDoc)
        hr = _pInnerXMLDoc->QueryInterface(IID_IDispatch, (void**)ppDisp);

Cleanup:
    return hr ;
}

//---------------------------------------------------------------------------
//
//  Member:     CPersistUserData::getAttribute
//
//  Synopsis:   IHTMLUserData OM method implementation
//
//---------------------------------------------------------------------------

HRESULT
CPersistUserData::getAttribute (BSTR strName, VARIANT * pvarValue )
{
    HRESULT hr = S_OK;

    if (!pvarValue)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    VariantClear(pvarValue);

    if (!strName)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = initXMLCache();
    if (hr)
        goto Cleanup;

    // get the child of the root that has the name strName
    if (_pRoot)
    {
        hr = _pRoot->getAttribute(strName, pvarValue);
        if (hr ==S_FALSE)
            hr = S_OK;
    }

Cleanup:
    return hr ;
}


//---------------------------------------------------------------------------
//
//  Member:     CPersistUserData::setAttribute
//
//  Synopsis:   IHTMLUserData OM method implementation
//
//---------------------------------------------------------------------------

STDMETHODIMP
CPersistUserData::setAttribute (BSTR strName, VARIANT varValue)
{
    HRESULT  hr = S_OK;

    if (!strName)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = initXMLCache();
    if (hr)
        goto Cleanup;

    // save this value as an attribute on the root.
    if (_pRoot)
    {
        VARIANT * pvar;
        CVariant cvarTemp;

        if ((V_VT(&varValue)==VT_BSTR) || 
             V_VT(&varValue)==(VT_BYREF|VT_BSTR))
        {
            pvar = (V_VT(&varValue) & VT_BYREF) ?
                    V_VARIANTREF(&varValue) : &varValue;
        }
        else if ((V_VT(&varValue)==VT_BOOL ||
                 V_VT(&varValue)==(VT_BYREF|VT_BOOL)))
        {
            // sadly, do our own bool conversion...
            VARIANT_BOOL vbFlag = (V_VT(&varValue)==VT_BOOL) ?
                                   V_BOOL(&varValue) :
                                   V_BOOL( V_VARIANTREF(&varValue) );

            V_VT(&cvarTemp) = VT_BSTR;
            V_BSTR(&cvarTemp) = vbFlag ? SysAllocString(L"true") :
                                         SysAllocString(L"false");

            pvar = & cvarTemp;
        }
        else
        {
            pvar = &varValue;

            hr = VariantChangeTypeEx(pvar, pvar, LCID_SCRIPTING, 0, VT_BSTR);
            if (hr)
                goto Cleanup;
        }

        hr = _pRoot->setAttribute(strName, *pvar);
        if (hr ==S_FALSE)
            hr = S_OK;
    }

Cleanup:
    return hr ;
}



//---------------------------------------------------------------------------
//
//  Member:     CPersistUserData::removeAttribute
//
//  Synopsis:   IHTMLUserData OM method implementation
//
//---------------------------------------------------------------------------

STDMETHODIMP
CPersistUserData::removeAttribute (BSTR strName)
{
    HRESULT hr = S_OK;

    if (!strName)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = initXMLCache();
    if (hr)
        goto Cleanup;

    // get the child of the root that has the name strName
    if (_pRoot)
    {
        hr = _pRoot->removeAttribute(strName);
        if (hr ==S_FALSE)
            hr = S_OK;
    }

Cleanup:
    return hr ;
}


//+--------------------------------------------------------------------------
//
//  Member : get_expires
//
//  Synopsis : returns the expiration time of the currently loaded data store
//      If there is not expiration time set, it returns an null string.
//
//---------------------------------------------------------------------------
HRESULT
CPersistUserData::get_expires(BSTR * pstrDate)
{
    HRESULT  hr = S_OK;
    FILETIME ftZero = {0};

    if (!pstrDate)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pstrDate = NULL;

    // only set the return string if there is a non-zero time set
    if (CompareFileTime(&ftZero, &_ftExpires) != 0)
    {
        TCHAR    achDate[DATE_STR_LENGTH];

        hr = ConvertGmtTimeToString(_ftExpires, 
                                    (LPTSTR)&achDate, 
                                    DATE_STR_LENGTH);

        (*pstrDate) = SysAllocString(achDate);
        if (!*pstrDate)
            hr = E_OUTOFMEMORY;
    }

Cleanup:
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Member : put_expires
//
//  Synopsis : set the expiration time to be used when the current data store
//      is saved.  If the save method is not called, this has no real effect
//              The date string passed in is expected in GMT format just like
//      other script dates.  Specifically, this uses the same format as cookies
//      which is the format returned by the Date.toGMTString() script call.
//      e.g. "Tue, 02 Apr 1996 02:04:57 UTC"
//
//---------------------------------------------------------------------------
HRESULT
CPersistUserData::put_expires(BSTR bstrDate)
{
    HRESULT   hr;
    FILETIME  ftTemp;

    if (!bstrDate)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = ParseDate(bstrDate, &ftTemp);
    // S_FALSE means that ere  was a parse error
    if (hr ==S_FALSE)
        hr = E_INVALIDARG;

    if (SUCCEEDED(hr))
        _ftExpires = ftTemp;

Cleanup:
    return hr;
}

//============================================================================
//
// Private members of CPersistData to help with the persistence
//
//============================================================================
//---------------------------------------------------------------------------
//
//  Member:     CPersistUserData::LoadUserData
//
//  Synopsis:   IHTMLUserData OM method implementation
//
//  If UDF_BSTR is passed the size is the number of bytes in the string, 
//          excluding the null terminator
//
//---------------------------------------------------------------------------

HRESULT 
CPersistUserData::LoadUserData(
    LPCOLESTR pwzDirPath, 
    LPCOLESTR pwzName, 
    LPVOID *ppData, 
    LPDWORD pdwSize, 
    DWORD dwFlags
    )
{
    HRESULT hr = E_UNEXPECTED;
    LPTSTR pszCacheUrl = NULL;
    DWORD dwError;
    BYTE *buffer = NULL;
    BYTE * pOutBuffer = NULL;
    FILETIME ftZero = {0};


    if (pdwSize == NULL)
    {
        hr = E_POINTER;
        goto cleanup;
    }

    if ((hr = EnsureCacheContainer()) != S_OK)
        return hr;

    // Get the cache url for this document. 
    if ((hr = GetCacheUrlFromDirPath(pwzDirPath, pwzName, &pszCacheUrl)) != S_OK)
    {
        goto cleanup;
    }
    
    Assert(pszCacheUrl != NULL);

    // Now try to retreive the information from the cache.
    LPINTERNET_CACHE_ENTRY_INFO pICEI;
    DWORD cbICEI;

    cbICEI = sizeof(INTERNET_CACHE_ENTRY_INFO) + MAX_PATH /* for the path */ + 1024 /* for URL dir */;
    buffer = new BYTE[cbICEI];
    pICEI = (LPINTERNET_CACHE_ENTRY_INFO)buffer;
    
    if (pICEI == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }
    
    HANDLE hUrlCacheStream;
    DWORD dwBufSize;
    dwBufSize = cbICEI;

    hUrlCacheStream = NULL;          
    if ((hUrlCacheStream = RetrieveUrlCacheEntryStream(pszCacheUrl, pICEI, &dwBufSize, FALSE,0)) == NULL)
    {

        dwError = ::GetLastError();
        // Try with more memory if the buffer was insufficient.
        if (dwError == ERROR_INSUFFICIENT_BUFFER)
        {
            // If we failed because of insufficient buffer, dwBufSize 
            // better be bigger than the original buffer.
            Assert(dwBufSize > cbICEI);

            delete [] buffer;
            cbICEI = dwBufSize;
            buffer = new BYTE[cbICEI];
            pICEI = (LPINTERNET_CACHE_ENTRY_INFO)buffer;
            if (!pICEI)
            {
                hr = E_OUTOFMEMORY;
                goto cleanup;
            }

            if ((hUrlCacheStream = RetrieveUrlCacheEntryStream(pszCacheUrl, pICEI, &dwBufSize, FALSE,0)) == NULL)
                dwError = ::GetLastError();
            else
                dwError = NOERROR;
        }

        if (dwError != NOERROR)
        {
            // Try to nuke it so it doesn't bother us in the future.
            hr = HRESULT_FROM_WIN32(dwError);
            goto cleanup;
        }
    }

    Assert(hUrlCacheStream != NULL);

    FILETIME currTime;
    GetSystemTimeAsFileTime(&currTime);

    if ((CompareFileTime(&ftZero, &(pICEI->ExpireTime)) != 0) && // ExpireTime is not zero
        (CompareFileTime(&currTime, &(pICEI->ExpireTime))  > 0)  //currentTime is not beyond expired time.
       ) 
    {
        // It is okay to delete the entry even though we have a handle open to it. 
        // the file will get destroyed once we release the handle. 
        DeleteUrlCacheEntry(pszCacheUrl);

#if 0  // BUGBUG - sanjays - The logic below is incorrect and needs to be fixed. 
        // To ensure correctness at this point we should delete the sizeof the
        // document from the quota
        LPTSTR pszSite;
        DWORD cchSite;
        if (CreateSiteTable() == S_OK && 
            GetSiteFromCacheUrl(pszCacheUrl, &pszSite, &cchSite) == S_OK)
        {
            pszSite[cchSite] = 0;
            ModifySitesSpaceUsage(pszSite, -(int)RealDiskUsage(pICEI->dwSizeLow));
        }
#endif
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto cleanup;
    }

    DWORD dwOutBufSize;

    dwOutBufSize = pICEI->dwSizeLow;

    // Caller is not interested in the buffer, just wants to the know the amount of 
    // memory required.

    if (ppData == NULL)
    {
        *pdwSize = dwOutBufSize;
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto cleanup;
    } 

    DWORD dwStreamPosition;
    dwStreamPosition = 0;

    // Allocate memory to read in the buffer. 
    if (dwFlags & UDF_BSTR)
    {
        // If we are retrieving a BSTR, we better have an even number of bytes in the stream.
        Assert((dwOutBufSize & 0x1) == 0x0);

        // Since BSTR's are stored as Unicode files they have the lead unicode bytes 
        // We should skip over these and real the actual data.
        DWORD dwLeadCharSize = sizeof(OLECHAR);
        OLECHAR wchLead;

        if (!ReadUrlCacheEntryStream(hUrlCacheStream, 0, &wchLead, &dwLeadCharSize, 0))
        {
            dwError = ::GetLastError();
            hr = HRESULT_FROM_WIN32(dwError);
            goto cleanup;
        }

        // the first two bytes should be the lead unicode file character.
        Assert(wchLead == wchLEADUNICODEFILE);

        dwOutBufSize -= sizeof(OLECHAR);
        dwStreamPosition += sizeof(OLECHAR);
            
        DWORD cch = (dwOutBufSize/sizeof(OLECHAR)); 
        pOutBuffer = (BYTE *) SysAllocStringLen(NULL, cch);
        // SysAllocStringLen will null terminate the string;
    }           
    else
    {
        pOutBuffer = new BYTE[dwOutBufSize];
    }

    if (pOutBuffer == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    if (!ReadUrlCacheEntryStream(hUrlCacheStream, dwStreamPosition, pOutBuffer, &dwOutBufSize, 0))
    {
        dwError = ::GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
        goto cleanup;
    }

    // set the pICEI->ExpireTime to the local value to support get_expires.
    if (CompareFileTime(&ftZero, &(pICEI->ExpireTime)) != 0)
    {
        _ftExpires = pICEI->ExpireTime;
    }

    *ppData = (LPVOID)pOutBuffer;
    *pdwSize = dwOutBufSize;
    pOutBuffer = NULL;
    hr = S_OK;

cleanup:

    delete [] pszCacheUrl;
    delete [] buffer;
    delete [] pOutBuffer;

    if (hUrlCacheStream != NULL)
        UnlockUrlCacheEntryStream(hUrlCacheStream, 0);

    return hr;
}


//---------------------------------------------------------------------------
//
//  Member:     CPersistUserData::SaveUserData
//
//  Synopsis:   IHTMLUserData OM method implementation
//
//---------------------------------------------------------------------------

HRESULT 
CPersistUserData::SaveUserData(
    LPCOLESTR pwzDirPath, 
    LPCOLESTR pwzName, 
    LPVOID pData, 
    DWORD dwSize, 
    FILETIME ftExpire, 
    DWORD dwFlags 
    )
{
    // Do Parameter validation.
    HRESULT hr;
    DWORD dwError = NOERROR;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    LPTSTR pszCacheUrl = NULL;
    FILETIME ftZero = { 0 };
    BOOL fAddLeadUnicodeChar = (dwFlags & UDF_BSTR) ? TRUE : FALSE;
     
    if ((hr = EnsureCacheContainer()) != S_OK)
        return hr;

    // make sure we have created the site table which tracks the amount of memory
    // allocated by each site.
    if ((hr = CreateSiteTable()) != S_OK)
        return hr;

    if ((hr = GetCacheUrlFromDirPath(pwzDirPath, pwzName, &pszCacheUrl)) != S_OK)
    {
        return hr;
    }

    // dwOldSize is the size of the document we will be overwriting by saving
    // this document.
    DWORD dwOldDiskUsage = 0;
    DWORD dwDiskUsage = RealDiskUsage(dwSize + (fAddLeadUnicodeChar ? sizeof(WCHAR) : 0));
    
    if ((hr = EnforceStorageLimits(pwzDirPath, pszCacheUrl, dwDiskUsage, &dwOldDiskUsage, TRUE)) != S_OK)
    {
        delete [] pszCacheUrl;
        return hr;
    }

    Assert(pszCacheUrl != NULL);

    TCHAR achFileName[MAX_PATH];

    // Convert the string that we send in to a url name    
    if (!CreateUrlCacheEntry(pszCacheUrl,
                            0,
                            TEXT("xml"),
                            achFileName,
                            0))
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    hFile = CreateFile(
            achFileName,
            GENERIC_WRITE,
            0,  // no sharing.
            NULL,
            TRUNCATE_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
            
    if (hFile == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }


    DWORD dwBytesWritten;
    dwBytesWritten = 0;

    // Need to add lead character to indicate a Unicode file.
    if ( fAddLeadUnicodeChar )
    {
        if ( !WriteFile(hFile, (LPVOID) &wchLEADUNICODEFILE, sizeof(WCHAR), &dwBytesWritten, NULL))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            CloseHandle(hFile);
            goto cleanup;
        } 
    }
    
    if (!WriteFile(hFile, pData, dwSize, &dwBytesWritten, NULL))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        CloseHandle(hFile);
        goto cleanup;
    }

    // We must close the handle before commiting the cache entry. 
    // If we don't some of the file system api's fail on Win95 and
    // consequently the CommitUrlCacheEntry fails as well. 
    CloseHandle(hFile);     

    if (!CommitUrlCacheEntry(pszCacheUrl,
                             achFileName,
                             ftExpire,
                             ftZero,
                             NORMAL_CACHE_ENTRY,
                             NULL,
                             0,
                             NULL,
                             0))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        hr = S_OK;
    }

cleanup:
    // EnforceStorageLimits assumes this store will succeed and updates the size usage
    // Since we didn't we should undo that now.
    if (hr != S_OK)
    {
        // In this case we have to get the site string and undo the changes
        // we made earlier in EnforceStorageLimits.
        LPTSTR pszSite;
        DWORD cchSite;
        if (GetSiteFromCacheUrl(pszCacheUrl, &pszSite, &cchSite) == S_OK)
        {
            pszSite[cchSite] = 0;
            ModifySitesSpaceUsage(pszSite, (int)dwOldDiskUsage - dwDiskUsage);
        }
    }

    delete [] pszCacheUrl;

    return hr ;
}




HRESULT CPersistUserData::GetCacheUrlFromDirPath(LPCOLESTR pwzDirPath, LPCOLESTR pwzFileName, LPTSTR * ppszCacheUrl)
{
    if (pwzDirPath == NULL)
    {
        return E_POINTER;
    }

    if (pwzFileName == NULL)
        pwzFileName = TEXT("_default");
    
    LPCTSTR pszCacheUrlPrefix;
    DWORD cchCacheUrlPrefix ; 

    // IMPORTANT: Do not free the memory returned by GetCacheUrlPrefix.
    if (!GetCacheUrlPrefix(&pszCacheUrlPrefix, &cchCacheUrlPrefix))
    {
        Assert(FALSE);
        return E_FAIL;
    }


    LPTSTR pszUrl;

    HRESULT hr = S_OK;


    // The length of the string remaining to be processed.
    DWORD cch = lstrlen(pwzDirPath);
    // If the path includes the trailing / ignore it for now, we will add it back when composing the URL.
    if (cch > 0 && (pwzDirPath[cch - 1] == chSLASH || pwzDirPath[cch - 1] == chBACKSLASH))
        cch-- ; 

    DWORD cchFileName = lstrlen(pwzFileName);

       
    // The fake URL name we produce is of the form
    // For https://www.microsoft.com/foobar  and document name 'default'
    // userdata:sanjays@https@www.microsoft.com/foobar/default
    
    // First figure out the constituent parts. 
    LPCOLESTR pszScan;

    PARSEDURL pu;
    pu.cbSize = sizeof(pu);
    
    hr = ParseURL(pwzDirPath, &pu);
    
    if (hr == S_OK)
    {
        pszScan = pu.pszSuffix;
        cch -= (DWORD)(pszScan - pwzDirPath);    // we processed the scheme: part of the string.
    }
    else
        return hr;

    // Now skip over any / or \'s till we get to the begining of the host name.
    while (*pszScan == chSLASH || *pszScan == chBACKSLASH)
    {
        *pszScan++;
        cch--;
    }
    
    // This implies the directory path passed in was bogus

    if (*pszScan == 0)
        return E_INVALIDARG;

    DWORD numChars = cchCacheUrlPrefix; // For "userdata:sanjays@" NULL terminator will be used for the ':'
    numChars += pu.cchProtocol + 1 /* for scheme followed by @ */ ;
    numChars += cch + 1;            /* for hostname and path followed by / */
    numChars += cchFileName + 1;  /* filename and the  NULL terminator */ ;
    
                       
    LPTSTR pszCacheUrl = new TCHAR[numChars];

    if (pszCacheUrl == NULL)
        return E_OUTOFMEMORY;
    else
        *ppszCacheUrl = pszCacheUrl;          


    memcpy(pszCacheUrl, pszCacheUrlPrefix, sizeof(TCHAR) * cchCacheUrlPrefix);
    pszCacheUrl += cchCacheUrlPrefix;
    memcpy(pszCacheUrl, pu.pszProtocol, sizeof(TCHAR) * pu.cchProtocol);
    pszCacheUrl += pu.cchProtocol;
    *pszCacheUrl++ = chAT; 
    memcpy(pszCacheUrl, pszScan, sizeof(TCHAR) * cch);
    pszCacheUrl += cch;
    *pszCacheUrl++ = chSLASH;
    memcpy(pszCacheUrl, pwzFileName, sizeof(TCHAR) * (cchFileName + 1));

    return S_OK;
}


// Returns a string of the form userdata:username@. This string is used as a prefix
// on all userdata cache items stored on behalf of this user.
// The caller should not free the memory returned here. 
// If no user name is found ( could happen on Win9x system with no logon it uses the 
// string "anyuser" for the username. 

BOOL CPersistUserData::GetCacheUrlPrefix(LPCTSTR * ppszCacheUrlPrefix, DWORD *pcchCacheUrlPrefix)
{
    BOOL bRet;

    EnterCriticalSection(&s_csCacheUrlPrefix);

    if (s_bCheckedUserName)
    {
        bRet = s_bCacheUrlPrefixRet;
    }            
    else
    {
        TCHAR rgchUserName[MAX_PATH];
        DWORD cchUserNameLen = ARRAY_SIZE(rgchUserName);

        if (!::GetUserName(rgchUserName, &cchUserNameLen))
        {
            DWORD dwError = GetLastError();
            Assert(dwError != ERROR_INSUFFICIENT_BUFFER);
            // Just user the string anyuser
            cchUserNameLen = lstrlen(rgchAnyUser);
            memcpy(rgchUserName, rgchAnyUser, (cchUserNameLen + 1) * sizeof(TCHAR));
            bRet = TRUE;
        }
        else
        {
            bRet = TRUE;
            cchUserNameLen--;  // Includes the count for null termination
        }
            
        // If we succeed we should calculate the cache url prefix as well.
        // Create a string of the form userdata:username@ 

        if (bRet)
        {
            LPTSTR psz = s_rgchCacheUrlPrefix;
            memcpy(psz, s_rgchUserData, sizeof(TCHAR) * (ARRAY_SIZE(s_rgchUserData) - 1));
            psz += (ARRAY_SIZE(s_rgchUserData) - 1);
            *psz++ = chCOLON;
            memcpy(psz, rgchUserName, sizeof(TCHAR) * cchUserNameLen);
            psz += cchUserNameLen;
            *psz++ = chAT;
            *psz = 0;
            s_cchCacheUrlPrefix = (DWORD) (psz - s_rgchCacheUrlPrefix);
        }

    }

    s_bCheckedUserName = TRUE;
    s_bCacheUrlPrefixRet = bRet;
    if (bRet)
    {
        *ppszCacheUrlPrefix = s_rgchCacheUrlPrefix;
        *pcchCacheUrlPrefix = s_cchCacheUrlPrefix;
    }


    LeaveCriticalSection(&s_csCacheUrlPrefix);

    return bRet;
}

// This is an ANSI function in an otherwise unicode world. 
// This is because Unicode versions of the cache folder functions used here 
// are not supported and return ERROR_NOT_IMPLEMENTED

HRESULT CPersistUserData::EnsureCacheContainer()
{                                            
    CHAR achUserData[] = "UserData";
    HRESULT hr = S_OK;
    DWORD dwError = NOERROR;
    CHAR rgchPath[MAX_PATH]; 
    PFNSHGETFOLDERPATHA pfnSHGetFolderPathA = NULL;
    LPCSTR pszDirPath = NULL;
    LPCSTR pszRelPath = NULL;
    HMODULE hSHFolder = NULL;
    BYTE buffer[sizeof(INTERNET_CACHE_CONFIG_INFOA) + sizeof(INTERNET_CACHE_CONFIG_PATH_ENTRYA)];
    INTERNET_CACHE_CONFIG_INFOA *pCacheConfigInfo = (INTERNET_CACHE_CONFIG_INFOA *)buffer;
    DWORD dwBufSize = sizeof(buffer);

    EnterCriticalSection(&s_csCacheContainer);

    if (s_bCheckedCacheContainer)
    {   
        hr = s_hrCacheContainer;
        goto cleanup;

    }
                
    // First try to see if shfolder.dll is available
    hSHFolder = LoadLibraryA("shfolder.dll");

    if (hSHFolder != NULL)
        pfnSHGetFolderPathA = (PFNSHGETFOLDERPATHA) GetProcAddress(hSHFolder, "SHGetFolderPathA");

    // If it is available try to get the "Application Data" folder.
    if (pfnSHGetFolderPathA && 
        SUCCEEDED(pfnSHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, rgchPath)))
    {
       // We will create the cache container under the appdata folder.
       pszDirPath = rgchPath;
       pszRelPath = rgchRelAppData;
    }
    else if (GetUrlCacheConfigInfoA(pCacheConfigInfo, &dwBufSize, CACHE_CONFIG_COOKIES_PATHS_FC))
    {
        // In this case we will create the cache container in the same directory as cookies.
        pszDirPath = pCacheConfigInfo->CachePaths[0].CachePath;
        pszRelPath = rgchRelCookies;
    }
    else
    {
        Assert(GetLastError() != ERROR_INSUFFICIENT_BUFFER);
        Assert(pCacheConfigInfo->dwNumCachePaths == 1);     

        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;                     
    }

    // Compose the path for the directory. 
    // Combine the dirpath and relative path to get the location of the userdata folder.
    if (PathCombineA(rgchPath, pszDirPath, pszRelPath) == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    // Now try to create the cache entry. 
                
    if (!CreateUrlCacheContainerA(
                achUserData,
                achUserData,
                rgchPath,
                1000,
                0, 
                (INTERNET_CACHE_CONTAINER_NODESKTOPINIT 
                ),
                NULL,
                NULL)
        )
    {
        // If this was because the cache already existed we are okay. 
        dwError = GetLastError();
        
        if (dwError != ERROR_ALREADY_EXISTS)
            hr = HRESULT_FROM_WIN32(dwError);
    }           

    // Figure out the cluster size information for the disk that the cache container
    // will live on.
    // The directory name lives in rgchPath, we will clobber it to create a string
    // which just has the device part.
    if (rgchPath[0] != chSLASH && rgchPath[0] != chBACKSLASH)
    {
        // Dir path is of the from c:\..... We just need the c:\ part.
        rgchPath[3] = 0;
    }
    else
    {
         // Dir path is of the form \\foo\bar\.... 
        // In this case we pass the name passed to GetDiskSpace is the whole string
        DWORD dwLen = lstrlenA(rgchPath);
        if (dwLen + 1 < ARRAY_SIZE(rgchPath))
        {
            rgchPath[dwLen] = schDIR_SEPERATOR;
            rgchPath[dwLen + 1] = '\0';
        }
    }

    DWORD dwSectorsPerCluster;
    DWORD dwBytesPerSector;
    DWORD dwFreeClusters;
    DWORD dwClusters;
    DWORD dwClusterSize;
    if (GetDiskFreeSpaceA(rgchPath, &dwSectorsPerCluster, &dwBytesPerSector, &dwFreeClusters, &dwClusters))
    {
        s_dwClusterSizeMinusOne = dwSectorsPerCluster * dwBytesPerSector - 1;
        s_dwClusterSizeMask     = ~s_dwClusterSizeMinusOne;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

cleanup:

    if (hSHFolder)
        FreeLibrary(hSHFolder);

    s_bCheckedCacheContainer = TRUE;
    s_hrCacheContainer = hr;

    LeaveCriticalSection(&s_csCacheContainer);
    return hr;
}                        

// Adds iSizeDelta to the amount of space taken by this site. 
// Creates a new entry if non-exists to date. 

HRESULT CPersistUserData::ModifySitesSpaceUsage(LPCTSTR pszKey, int iSizeDelta)
{
    HRESULT hr = S_OK;
    EnterCriticalSection(&s_csSiteTable);

    Assert(s_pSiteTable);

    if (s_pSiteTable != NULL)
    {
        DWORD dwSpace;

        if (!s_pSiteTable->Lookup(pszKey, dwSpace))
            dwSpace = 0;

        if (iSizeDelta < 0 && (-iSizeDelta  > (int)dwSpace))
        {
            // Our quota is going to be set to a number less than zero.
            // Assert but recover by setting space usage to zero.
            Assert(FALSE);
            dwSpace = 0;                   
        }
        else
             dwSpace += iSizeDelta;


        hr = s_pSiteTable->SetAt(pszKey, dwSpace);
    }
    else
        hr = E_FAIL;
                  
    LeaveCriticalSection(&s_csSiteTable);

    return hr;
}

// Helper function to get the site string given a cache URL name. 
//  arguments
//      pszCacheUrl [in] - Cache Url for which we want the site URL.
//      *ppszBegPos [out[ - will contain the pointer where the site begins.
//      *pcch = number of characters in the site name. 
// Returns
//          S_OK if entry is for current user, the two out arguments are valid in this case.
//          S_FALSE if entry is for a different user.
//          E_* on failure

HRESULT CPersistUserData::GetSiteFromCacheUrl(LPCTSTR pszCacheUrl, LPTSTR * ppszBegPos, DWORD *pcch)
{
    LPCTSTR pszCacheUrlPrefix;
    DWORD cchCacheUrlPrefix ; 

    if (pszCacheUrl == NULL)
        return E_INVALIDARG;    

    // IMPORTANT: Do not free the memory returned by GetCacheUrlPrefix.
    if (!GetCacheUrlPrefix(&pszCacheUrlPrefix, &cchCacheUrlPrefix))
    {
        Assert(FALSE);
        return E_FAIL;
    }
    
    if (StrCmpNI(pszCacheUrl, pszCacheUrlPrefix, cchCacheUrlPrefix) == 0)
    {
        LPCTSTR psz = pszCacheUrl + cchCacheUrlPrefix;

        // At this point psz points to the begining of the protocol. 
        // Everything from this point to the first / or \ is what we use
        // to track the site name.

        LPCTSTR pszEnd = psz;
        while (*pszEnd != 0 && *pszEnd != chSLASH && *pszEnd != chBACKSLASH)
            pszEnd++;

        Assert(*pszEnd != 0);
        if (*pszEnd == 0)
            return E_INVALIDARG;
        
        *ppszBegPos = (LPTSTR)psz;
        *pcch = (DWORD)(pszEnd - psz);
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

        
        
// Create the Site Table and populate it.
#define CACHE_ENTRY_BUFFER_SIZE (sizeof(INTERNET_CACHE_ENTRY_INFO) \
                                 + MAX_PATH * sizeof(TCHAR)\
                                 + INTERNET_MAX_URL_LENGTH * sizeof(TCHAR))

HRESULT CPersistUserData::CreateSiteTable( )
{           
    HRESULT hr = S_OK;

    // First grab the critical section so no else tries to do this at
    // the same time.
    EnterCriticalSection(&s_csSiteTable);

    if (s_pSiteTable == NULL)
    {
        s_pSiteTable = new CSiteTable();
        if (s_pSiteTable == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }


        // Now iterate over the cache container and populate the entries
        // in the table.
        BYTE buffer[CACHE_ENTRY_BUFFER_SIZE];
        LPINTERNET_CACHE_ENTRY_INFO lpICEI = (LPINTERNET_CACHE_ENTRY_INFO)buffer;
        DWORD dwBufSize = CACHE_ENTRY_BUFFER_SIZE;

        lpICEI->dwStructSize = sizeof(INTERNET_CACHE_ENTRY_INFO);

        HANDLE hEnum = FindFirstUrlCacheEntry(
                           TEXT("userdata:"),
                           lpICEI,
                           &dwBufSize
                           );

        if (hEnum == NULL)
        {
            DWORD dwError = GetLastError();
            // If this assert fires the
            // assumption that our buffer size is 
            // big enough is getting violated. 
            // Either up the limit or change to use
            // a dynamic allocation where we retry the
            // call
            Assert(dwError != ERROR_INSUFFICIENT_BUFFER);

            // If we can't get an enumerator because the container is empty we are 
            // fine.
            if (dwError == ERROR_NO_MORE_ITEMS)
                hr = S_OK;
            else
                hr = HRESULT_FROM_WIN32(dwError);
            goto cleanup;
        }

        // make sure the entry has not expired.
        FILETIME ftZero = {0};
        FILETIME currTime;
        GetSystemTimeAsFileTime(&currTime);

        do 
        {
            // The URL name we created is of the form.
            // userdata:username@http:www.microsoft.com/foobar/default

            if (lpICEI->lpszSourceUrlName != NULL)
            {
                DWORD cchSite; 
                LPTSTR pszSite;

                if ((CompareFileTime(&ftZero, &(lpICEI->ExpireTime)) != 0) && // ExpireTime is not zero
                    (CompareFileTime(&currTime, &(lpICEI->ExpireTime))  > 0)  //currentTime is beyond expires time.
                   ) 
                {
                    // TODO TODO TODO TODO TODO TODO
                    // BUGBUG BUGBUG BUGBUG
                    // Is it okay to delete the cache entry while we are still enumerating it.
                    DeleteUrlCacheEntry(lpICEI->lpszSourceUrlName);
                }
                else
                {
                    hr = GetSiteFromCacheUrl(lpICEI->lpszSourceUrlName, &pszSite, &cchSite);
                    if (hr == S_FALSE)
                    {
                        // This implies that the document does not belong to this user.
                        // Continue on to the next entry.
                        hr = S_OK;
                        continue;
                    }
                    else if (FAILED(hr))
                    {
                        break;
                    }
                                

                    // If we reach here the document belongs to this user. 

                    // Null terminate the site string. We are not interested
                    // in the parts beyond the site name for tracking memory
                    // usage
                    pszSite[cchSite] = 0;

                    // We don't support userdata files beyond 4GB
                    Assert(lpICEI->dwSizeHigh == 0);

                    hr = ModifySitesSpaceUsage(pszSite, RealDiskUsage(lpICEI->dwSizeLow));

                    // Only reasonable way in which this function could fail is if it out of memory.
                    // In that case we will just return failure from the API.
                    if (hr != S_OK)
                        break;
                }
            }            
            // Previous call might have modified these values.
            // Re-init to the actual sizes of the buffers.
            dwBufSize = CACHE_ENTRY_BUFFER_SIZE;
            lpICEI->dwStructSize = sizeof(INTERNET_CACHE_ENTRY_INFO);

        } while (FindNextUrlCacheEntry(hEnum, lpICEI, &dwBufSize));

        FindCloseUrlCache(hEnum);
    }

cleanup:
    LeaveCriticalSection(&s_csSiteTable);

    return hr;
}


HRESULT CPersistUserData::DeleteSiteTable()
{
    HRESULT hr = S_OK;

    // Destroy the site table object. It should clean up all the allocated memory.
    EnterCriticalSection(&s_csSiteTable);
    delete s_pSiteTable;
    s_pSiteTable = NULL;
    LeaveCriticalSection(&s_csSiteTable);

    return hr;
}


// This function enforces the storage limit for a given site.
// Arguments :
//      pwzDirPath - URL of the HTML document storing the data.
//      pszCacheUrl - corresponding cache URL of the document. 
//      dwDiskUsage - disk space used by the userdata file to be stored.
//      *pdwOldDiskUsage - disk usage of the document being overwritten if the save is done.
//      bModify - if this flag is TRUE, the function will change the size usage
//          of this site to reflect the saving of this document. 

HRESULT CPersistUserData::EnforceStorageLimits(
    LPCOLESTR /* pwzDirPath */, 
    LPCTSTR pszCacheUrl, 
    DWORD dwDiskUsage,
    DWORD* pdwOldDiskUsage,
    BOOL bModify
    )
{
    HRESULT hr = S_OK;

    LPTSTR pszBegSite;
    DWORD cchSite;
    LPTSTR pszSite;
    DWORD dwDomainLimit;
    DWORD dwDocLimit;
    DWORD dwZone = URLZONE_INVALID;

    dwZone = GetZone();
    Assert(ARRAY_SIZE(s_rgdwDomainLimit) == ARRAY_SIZE(s_rgdwDocLimit));

    if (dwZone < ARRAY_SIZE(s_rgdwDomainLimit))
    {
        dwDomainLimit = s_rgdwDomainLimit[dwZone];
        dwDocLimit = s_rgdwDocLimit[dwZone];
    }
    else
    {
        dwDomainLimit = s_dwUnkZoneDomainLimit;
        dwDocLimit = s_dwUnkZoneDocLimit;
    }

    if (dwDiskUsage > (dwDocLimit * 1024))
        return HRESULT_FROM_WIN32(ERROR_HANDLE_DISK_FULL);

    

    // First get the site string we will use as a key to save this sites space usage.
    hr = GetSiteFromCacheUrl(pszCacheUrl, &pszBegSite, &cchSite);

    if (hr == S_OK)     
    {
        // Copy the string into our own buffer and         
        pszSite = (LPTSTR) _alloca((cchSite + 1) * sizeof(TCHAR));
       
        memcpy(pszSite, pszBegSite, cchSite * sizeof(TCHAR));
        pszSite[cchSite] = 0;
    }                              
    else if (hr == S_FALSE)
    {
        // This implies that we are being asked to check the storage limit for a user
        // who is not the current user. We don't support this currently.
        hr = E_INVALIDARG;
    }

    if (hr != S_OK)
        return hr;

    // Now figure ouf the size of the document in the cache.
    BYTE buffer[CACHE_ENTRY_BUFFER_SIZE];
    LPINTERNET_CACHE_ENTRY_INFO lpICEI = (LPINTERNET_CACHE_ENTRY_INFO) buffer;
    DWORD dwBufSize = CACHE_ENTRY_BUFFER_SIZE;

    lpICEI->dwStructSize = sizeof(INTERNET_CACHE_ENTRY_INFO);

    DWORD dwOldDiskUsage = 0;

    if (GetUrlCacheEntryInfo(pszCacheUrl, lpICEI, &dwBufSize))
    {
        // Found an old entry. get the size of the document. 
        Assert(lpICEI->dwSizeHigh == 0);

        dwOldDiskUsage = RealDiskUsage(lpICEI->dwSizeLow);
    }
    else
    {
        DWORD dwError = GetLastError();

        // We have allocated a big enough buffer. The call 
        // should not return not enough memory
        Assert(dwError != ERROR_INSUFFICIENT_BUFFER);
        if (dwError = ERROR_FILE_NOT_FOUND)
            dwOldDiskUsage = 0;
        else
        {
            hr = HRESULT_FROM_WIN32(dwError);
            return hr;
        }
    }

    int iSizeDelta = (int)(dwDiskUsage - dwOldDiskUsage);

    EnterCriticalSection(&s_csSiteTable);
    if (iSizeDelta > 0)
    {
        DWORD dwCurrentSize;

        // We are increasng the size allocation. Check if we have enough space.
        if (!s_pSiteTable->Lookup(pszSite, dwCurrentSize))
            dwCurrentSize = 0;

        dwCurrentSize += iSizeDelta;
        if (dwCurrentSize > (dwDomainLimit * 1024) )
        {
            // We might want to define a new error such as E_QUOTAEXCEEDED here.
            hr = HRESULT_FROM_WIN32(ERROR_HANDLE_DISK_FULL);
        }
    }

    if (pdwOldDiskUsage)
        *pdwOldDiskUsage = dwOldDiskUsage;

    if (hr == S_OK && bModify && iSizeDelta != 0)
    {
        hr = ModifySitesSpaceUsage(pszSite, iSizeDelta);
    }

    LeaveCriticalSection(&s_csSiteTable);

    return hr;
}

           

    
// CSiteTable is a helper class we use to keep track of the amount of allocations per-site. 
// This is used to decide if the site is exceeding it's quota when a new document is being 
// saved.
// It seems like a much simpler approach would be to enumerate the entries in the container
// when the site is trying to write a new document. However the cache enumeration API's don't allow
// for filtering based on sitename. Since we are forced to enumerate all the documents
// we remember the current amount of memory allocated by each site.
CSiteTable::CSiteTable(int nTableSize)
{
    m_pHashTable = NULL;
    m_nHashTableSize = nTableSize;
    m_nCount = 0;
}

CSiteTable::~CSiteTable()
{
    RemoveAll();
}

inline UINT CSiteTable::HashKey(LPCTSTR key) const
{
    UINT nHash = 0;            

    while (*key)
        nHash = (nHash<<5) + nHash + *key++;

    return nHash;
}


HRESULT CSiteTable::InitHashTable()
{
    Assert(m_nCount == 0);
    Assert(m_nHashTableSize > 0);
    
    m_pHashTable = new CAssoc*[m_nHashTableSize];
    if (m_pHashTable == NULL)
        return E_OUTOFMEMORY;

    memset(m_pHashTable, 0, sizeof(CAssoc*) * m_nHashTableSize);
    
    return S_OK;
}

// Change the hash table to the new size, This is an expensive
// operation but is done infrequently. 

HRESULT CSiteTable::ChangeHashTableSize(int nHashSize)
{
    Assert(nHashSize != 0);

    if (m_pHashTable == NULL)
    {
        // Easy we are done
        m_nHashTableSize = nHashSize;
        return S_OK;
    }

    // Allocate an alternate hash table.
    CAssoc **pNewHashTable = new CAssoc*[nHashSize];
    if (pNewHashTable == NULL)
        return E_OUTOFMEMORY;

    memset(pNewHashTable, 0, sizeof(CAssoc *) * nHashSize);

    // Loop over the entries in the current hash table and 
    // add them to the new hash table.
    for (UINT nHash = 0 ; nHash < m_nHashTableSize ; nHash++)
    {
        CAssoc *pAssoc = m_pHashTable[nHash];
        while (pAssoc != NULL)
        {
            UINT nNewHash = HashKey(pAssoc->pStr) % nHashSize;
            CAssoc * pAssocCurrent = pAssoc; 
            pAssoc = pAssoc->pNext;

            pAssocCurrent->pNext = pNewHashTable[nNewHash];
            pNewHashTable[nNewHash] = pAssocCurrent;
        }
    }

    delete [] m_pHashTable;
    m_pHashTable = pNewHashTable;
    return S_OK;
}

void CSiteTable::RemoveAll()
{
    if (m_pHashTable != NULL)
    {
        for (UINT nHash = 0 ; nHash < m_nHashTableSize; nHash++)
        { 
           CAssoc *pAssoc = m_pHashTable[nHash];
           while (pAssoc != NULL)
           {
                LocalFree(pAssoc->pStr);
                CAssoc * pAssocDel = pAssoc;
                pAssoc = pAssoc->pNext;
                delete pAssocDel;
           }
        }
        delete [] m_pHashTable;
        m_pHashTable = NULL;
    }
    m_nCount = 0;
}

CSiteTable::CAssoc *
CSiteTable::GetAssocAt(LPCTSTR key, UINT &nHash) const
{
    nHash = HashKey(key) % m_nHashTableSize;

    if (m_pHashTable == NULL)
        return NULL;

    CAssoc *pAssoc;
    for (pAssoc = m_pHashTable[nHash] ;
         pAssoc != NULL;
         pAssoc = pAssoc->pNext
        )
    {
        if (StrCmpI(key, pAssoc->pStr) == 0)
            return pAssoc;
    
    }
    
    return NULL;
}

BOOL CSiteTable::Lookup(LPCTSTR key, DWORD &rdw) const
{
    UINT nHash;
    CAssoc *pAssoc = GetAssocAt(key, nHash);
    if (pAssoc == NULL)
        return FALSE;   // not in table.
    rdw = pAssoc->dwValue;
    return TRUE;

}

HRESULT CSiteTable::SetAt(LPCTSTR key, DWORD dwValue)
{
    UINT nHash;

    CAssoc *pAssoc;
    if ((pAssoc = GetAssocAt(key, nHash)) == NULL)
    {
        HRESULT hr = S_OK;
        if (m_pHashTable == NULL)
        {
            hr = InitHashTable();
        }
        else if (m_nHashTableSize * 2 <= m_nCount)
        { 
            // If we have twice as many sites as entries in 
            // the hash table we will grow the size

            // If this doesn't suceed we still have a 
            // hash table of the previous size. 
            // We will attempt to use that.
            if (ChangeHashTableSize(m_nHashTableSize * 2) == S_OK)
                nHash = HashKey(key) % m_nHashTableSize;   //recalculate the hash value. 
        }
        
        if (hr != S_OK)
            return hr;

        pAssoc = new CAssoc();
        if (pAssoc == NULL)
            return E_OUTOFMEMORY;

        pAssoc->pStr = StrDup(key);
        if (pAssoc->pStr == NULL)
        {
            delete pAssoc;
            return E_OUTOFMEMORY;
        }

        //. Put into the hash table.
        pAssoc->pNext = m_pHashTable[nHash];
        m_pHashTable[nHash] = pAssoc;
        m_nCount++;
    }
    pAssoc->dwValue = dwValue;
    return S_OK;
}


