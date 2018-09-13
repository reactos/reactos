#include "pch.h"
#include <dsgetdc.h>        // DsGetDCName and DS structures
#include <lm.h>
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ GetGlobalCatalogPath
/ --------------------
/   Look up the GC using DsGcDcName and return a string containing the path.
/
/ In:
/   pszServer, server to get the path for
/   pszBuffer, cchBuffer  = buffer to fill
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

#define GC_PREFIX     L"GC://"
#define CCH_GC_PREFIX 5

// BUGBUG: ChandanaS says use LocalFree when building for Win95 as the 
// BUGBUG: NetApiBufferFree is not available, she will fix this in
// BUGBUG: time.

#if !defined(UNICODE) 
#define NetApiBufferFree(x) LocalFree(x)
#endif

HRESULT GetGlobalCatalogPath(LPCWSTR pszServer, LPWSTR pszPath, INT cchBuffer)
{
    HRESULT hres;
    DWORD dwres;
    PDOMAIN_CONTROLLER_INFOW pdci = NULL;
    ULONG uFlags = DS_RETURN_DNS_NAME|DS_DIRECTORY_SERVICE_REQUIRED;
    USES_CONVERSION;

    TraceEnter(TRACE_SCOPES, "GetGlobalCatalogPath");

    dwres = DsGetDcNameW(pszServer, NULL, NULL, NULL, uFlags, &pdci);

    if ( ERROR_NO_SUCH_DOMAIN == dwres )
    {
        TraceMsg("Trying with rediscovery bit set");
        dwres = DsGetDcNameW(pszServer, NULL, NULL, NULL, uFlags|DS_FORCE_REDISCOVERY, &pdci);
    }
    
    if ( (NO_ERROR != dwres) || !pdci->DnsForestName )
        ExitGracefully(hres, E_UNEXPECTED, "Failed to find the GC");

    if ( (lstrlenW(pdci->DnsForestName)+CCH_GC_PREFIX) > cchBuffer )
        ExitGracefully(hres, E_UNEXPECTED, "Buffer too small for the GC path");

    StrCpyW(pszPath, GC_PREFIX);
    StrCatW(pszPath, pdci->DnsForestName);

    Trace(TEXT("Resulting GC path is: %s"), W2T(pszPath));
    hres = S_OK;

exit_gracefully:

    NetApiBufferFree(pdci);

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ Scope handling
/----------------------------------------------------------------------------*/

typedef struct 
{
    LPSCOPETHREADDATA ptd;                  // thread data structure
    INT       index;                        // insert index into the visible scope list
    INT       cScopes;                      // number of items enumerated
    LPWSTR    pszDefaultDnsDomain;             // default domain to be selected
} ENUMSTATE, * LPENUMSTATE;


/*-----------------------------------------------------------------------------
/ _ScopeProc
/ ----------
/   Handle scope messages from for the scope blocks we have allocated.
/
/ In:
/   pScope -> refernce to scope block
/   uMsg = message
/   pVoid = arguments to message / = NULL
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT CALLBACK _ScopeProc(LPCQSCOPE pScope, UINT uMsg, LPVOID pVoid)
{
    HRESULT hres = S_OK;
    LPDSQUERYSCOPE pDsQueryScope = (LPDSQUERYSCOPE)pScope;
    LPWSTR pScopeADsPath = OBJECT_NAME_FROM_SCOPE(pDsQueryScope);
    LPWSTR pScopeObjectClass = OBJECT_CLASS_FROM_SCOPE(pDsQueryScope);
    IADsPathname* pDsPathname = NULL;
    IDsDisplaySpecifier* pdds = NULL;
    BSTR bstrProvider = NULL;
    BSTR bstrLeaf = NULL;
    WCHAR szBuffer[MAX_PATH];
    USES_CONVERSION;
    
    TraceEnter(TRACE_SCOPES, "_ScopeProc");

    switch ( uMsg )
    {
        case CQSM_INITIALIZE:
        case CQSM_RELEASE:
            break;
  
        case CQSM_GETDISPLAYINFO:
        {
            LPCQSCOPEDISPLAYINFO pDisplayInfo = (LPCQSCOPEDISPLAYINFO)pVoid;
            LPTSTR pDirectoryName;            

            TraceAssert(pDisplayInfo);
            TraceAssert(pDisplayInfo->pDisplayName);

            pDisplayInfo->iIndent = pDsQueryScope->iIndent;

            hres = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER, IID_IADsPathname, (LPVOID*)&pDsPathname);
            FailGracefully(hres, "Failed to get the IADsPathname interface");

            hres = pDsPathname->Set(pScopeADsPath, ADS_SETTYPE_FULL);
            FailGracefully(hres, "Failed to set the path of the name");

            hres = pDsPathname->Retrieve(ADS_FORMAT_PROVIDER, &bstrProvider);
            FailGracefully(hres, "Failed to get the provider");

            Trace(TEXT("Provider name is: %s"), W2T(bstrProvider));

            if ( !StrCmpW(bstrProvider, L"GC") )
            {
                TraceMsg("Provider is GC: so changing to Entire Directory");

                GetModuleFileName(GLOBAL_HINSTANCE, pDisplayInfo->pIconLocation, pDisplayInfo->cchIconLocation);
                pDisplayInfo->iIconResID = -IDI_GLOBALCATALOG;

                if ( SUCCEEDED(FormatDirectoryName(&pDirectoryName, GLOBAL_HINSTANCE, IDS_GLOBALCATALOG)) )
                {
                    StrCpyN(pDisplayInfo->pDisplayName, pDirectoryName, pDisplayInfo->cchDisplayName);
                    LocalFreeString(&pDirectoryName);
                }
            }
            else
            {
                TraceMsg("Non GC provider, so looking up icon and display name");

                //
                // get the leaf name for the object we want to display in the scope picker for the
                // DS.
                //

                pDsPathname->SetDisplayType(ADS_DISPLAY_VALUE_ONLY);

                if ( SUCCEEDED(pDsPathname->Retrieve(ADS_FORMAT_LEAF, &bstrLeaf)) )
                {
#if UNICODE
                    StrCpyNW(pDisplayInfo->pDisplayName, bstrLeaf, pDisplayInfo->cchDisplayName);
#else
                    WideCharToMultiByte(CP_ACP, 0, bstrLeaf, -1, 
                                        pDisplayInfo->pDisplayName, pDisplayInfo->cchDisplayName,
                                        0, FALSE);
#endif
                    SysFreeString(bstrLeaf);
                }

                //
                // Now retrieve the display specifier information for the object.
                //

                hres = CoCreateInstance(CLSID_DsDisplaySpecifier, NULL, CLSCTX_INPROC_SERVER, IID_IDsDisplaySpecifier, (LPVOID*)&pdds);                
                FailGracefully(hres, "Failed to get the IDsDisplaySpecifier object");

#ifdef UNICODE
                pdds->GetIconLocation(pScopeObjectClass, DSGIF_GETDEFAULTICON, 
                                      pDisplayInfo->pIconLocation, pDisplayInfo->cchIconLocation, 
                                      &pDisplayInfo->iIconResID);
#else

                pdds->GetIconLocation(pScopeObjectClass, DSGIF_GETDEFAULTICON, 
                                      szBuffer, ARRAYSIZE(szBuffer), 
                                      &pDisplayInfo->iIconResID);

                WideCharToMultiByte(CP_ACP, 0, szBuffer, -1, 
                                    pDisplayInfo->pIconLocation, pDisplayInfo->cchIconLocation,
                                    0, FALSE);
#endif                
            }

            break;
        }

        case CQSM_SCOPEEQUAL:
        {
            LPDSQUERYSCOPE pDsQueryScope2 = (LPDSQUERYSCOPE)pVoid;
            LPWSTR pScopeADsPath2 = OBJECT_NAME_FROM_SCOPE(pDsQueryScope2);

            Trace(TEXT("Comparing %s against %s"), W2T(pScopeADsPath), W2T(pScopeADsPath2));
            hres = StrCmpIW(pScopeADsPath, pScopeADsPath2) ? S_FALSE:S_OK;

            break;
        }

        default:
            hres = E_NOTIMPL;
            break;
    }

exit_gracefully:

    SysFreeString(bstrProvider);

    DoRelease(pDsPathname);
    DoRelease(pdds);

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ AddScope
/ --------
/   Given an ADs path, get it converted to a scope block and then 
/   call the add function to add it to the list of scopes we are going to be using.
/
/ In:
/   ptd -> SCOPETHREADDATA structure
/   pDsQuery -> IQueryHandler interface to be AddRef'd
    i = index to insert the scope at
/   iIndent = horizontal indent
/   pPath -> ADS path to store as the scope
/   pObjectClass = object class to select
/   fSelect = if the scope should be selected
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT AddScope(HWND hwndFrame, INT index, INT iIndent, LPWSTR pPath, LPWSTR pObjectClass, BOOL fSelect)
{
    HRESULT hres;
    LPCQSCOPE pScope = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_SCOPES, "AddScope");
    Trace(TEXT("index %d, iIndent %d, fSelect %d"), index, iIndent, fSelect);
    Trace(TEXT("Object name: %s"), W2T(pPath));
    Trace(TEXT("Class: %s"), pObjectClass ? W2T(pObjectClass):TEXT("<none>"));
    
    hres = AllocScope(&pScope, iIndent, pPath, pObjectClass);
    FailGracefully(hres, "Failed to allocate DSQUERYSCOPE");

    if ( !SendMessage(hwndFrame, CQFWM_ADDSCOPE, (WPARAM)pScope, MAKELPARAM(fSelect, index)) )
        ExitGracefully(hres, E_FAIL, "Failed when sending ADDSCOPE message");

    hres = S_OK;               // success

exit_gracefully:

    if ( pScope )
        CoTaskMemFree(pScope);

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ AllocScope
/ ----------
/   Convert the given ADs path into a scope block that can be passed to the
/   common query interfaces. 
/
/ In:
/   iIndent = index to indent the scope by
/   ppScope = receives the newly allocated scope block
/   pPath -> name to package for the DS scope
/   pObjectClass -> object class of scope
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT AllocScope(LPCQSCOPE* ppScope, INT iIndent, LPWSTR pPath, LPWSTR pObjectClass)
{
    HRESULT hres;
    LPDSQUERYSCOPE pDsQueryScope = NULL;
    IADsPathname* pPathname = NULL;
    DWORD cb, offset;
    USES_CONVERSION;

    TraceEnter(TRACE_SCOPES, "AllocScope");
    Trace(TEXT("indent %d"), iIndent);
    Trace(TEXT("pPath: %s"), W2T(pPath));
    Trace(TEXT("pObjectClass: %s"), W2T(pObjectClass));

    // Allocate a new structure, note that the buffer for the ADs path is variable
    // size and lives at the end of the allocation.

    cb = SIZEOF(DSQUERYSCOPE) + StringByteSizeW(pPath) + StringByteSizeW(pObjectClass);;
    
    pDsQueryScope = (LPDSQUERYSCOPE)CoTaskMemAlloc(cb);
    TraceAssert(pDsQueryScope);

    if ( !pDsQueryScope )
        ExitGracefully(hres, E_OUTOFMEMORY, "Failed to allocate scope");

    pDsQueryScope->cq.cbStruct = cb;
    pDsQueryScope->cq.dwFlags = 0;
    pDsQueryScope->cq.pScopeProc = _ScopeProc;
    pDsQueryScope->cq.lParam = 0;

    pDsQueryScope->iIndent = iIndent;
    pDsQueryScope->dwOffsetADsPath = SIZEOF(DSQUERYSCOPE);
    pDsQueryScope->dwOffsetClass = 0;

    StringByteCopyW(pDsQueryScope, pDsQueryScope->dwOffsetADsPath, pPath);
    pDsQueryScope->dwOffsetClass = pDsQueryScope->dwOffsetADsPath + StringByteSizeW(pPath);
    StringByteCopyW(pDsQueryScope, pDsQueryScope->dwOffsetClass, pObjectClass);

    hres = S_OK;          // success

exit_gracefully:

    if ( ppScope )
        *ppScope = SUCCEEDED(hres) ? (LPCQSCOPE)pDsQueryScope:NULL;

    DoRelease(pPathname);       

    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ AddScopesThread
/ ---------------
/   Gather the scopes in the background and pass them to the
/   query window to allow it to populate the view scope controls.
/
/ In:
/   pThreadParams -> structure that defines out thread information
/
/ Out:
/   -
/----------------------------------------------------------------------------*/

// Walk the DOMAINDESC structures building ADSI paths and adding 
// them as search scopes to the scope list by calling AddScope
// with the ADSI path stored in the domainDesc strucutre.  If a domainDesc
// entry has any children then recurse (increasing the indent).  Otherwise
// just continue through the piers.

HRESULT _AddFromDomainTree(LPENUMSTATE pState, LPDOMAINDESC pDomainDesc, INT indent)
{
    HRESULT hres;
    WCHAR szBuffer[MAX_PATH];
    DWORD dwIndex;
    BOOL fDefault = FALSE;
    USES_CONVERSION;

    TraceEnter(TRACE_SCOPES, "_AddFromDomainTree");

    while ( pDomainDesc )
    {
        //
        // include the server name in the path we are generating if we have one
        //

        StrCpyW(szBuffer, L"LDAP://");

        if ( pState->ptd->pServer )
        {
            StrCatW(szBuffer, pState->ptd->pServer);
            StrCatW(szBuffer, L"/");
        }

        StrCatW(szBuffer, pDomainDesc->pszNCName);
        Trace(TEXT("Scope is: %s"), W2T(szBuffer));

        // 
        // now check to see if this is the default scope for the machine
        //        

        if ( pState->pszDefaultDnsDomain )
        {
            if ( !StrCmpIW(pState->pszDefaultDnsDomain, pDomainDesc->pszName) )
            {
                TraceMsg("Default domain found in the domain list");
                fDefault = TRUE;
            }
        }

        //
        // add the scope, bumping the counters are required
        //

        hres = AddScope(pState->ptd->hwndFrame, pState->index, indent, 
                        szBuffer, pDomainDesc->pszObjectClass, fDefault);

        FailGracefully(hres, "Failed to add scope");

        pState->index++;
        pState->cScopes++;             // bump the count before recursing

        if ( pDomainDesc->pdChildList )
        {
            hres = _AddFromDomainTree(pState, pDomainDesc->pdChildList, indent+1);
            FailGracefully(hres, "Failed to add children");
        }

        pDomainDesc = pDomainDesc->pdNextSibling;
    }

    hres = S_OK;

exit_gracefully:

    TraceLeaveResult(hres);
}

DWORD WINAPI AddScopesThread(LPVOID pThreadParams)
{
    HRESULT hres, hresCoInit;
    LPSCOPETHREADDATA ptd = (LPSCOPETHREADDATA)pThreadParams;
    IADs *pDsObject = NULL;
    IDsBrowseDomainTree* pDsDomains = NULL;
    BSTR bstrObjectClass = NULL;
    LPDOMAINTREE pDomainTree = NULL;
    ENUMSTATE enumState = { 0 };
    WCHAR szPath[MAX_PATH];
    WCHAR szDefaultDnsDomain[MAX_PATH];
    USES_CONVERSION;

    TraceEnter(TRACE_SCOPES, "AddScopesThread");

    hres = hresCoInit = CoInitialize(NULL);
    FailGracefully(hres, "Failed in CoInitialize");

    // Initialize ready to go and enumerate the scopes from the DS, this can be
    // quite a lengthy process therefore we live on a seperate thread.

    enumState.ptd = ptd;
    //enumState.index = 0;
    //enumState.cScopes = 0;
    //enumState.pszDefaultDnsDomain = NULL;

    // If the caller specified a scope we should be using then add it, if this
    // scope is already in the list we will end up select it anyway.

    if ( ptd->pDefaultScope )
    {
        Trace(TEXT("Adding default scope is: %s"), ptd->pDefaultScope);

        hres = ADsOpenObject(ptd->pDefaultScope, ptd->pUserName, ptd->pPassword, 
                                ADS_SECURE_AUTHENTICATION, IID_IADs, (LPVOID*)&pDsObject);
        if ( SUCCEEDED(hres) )
        {
            hres = pDsObject->get_Class(&bstrObjectClass);
            FailGracefully(hres, "Failed to get the object class");

            hres = AddScope(ptd->hwndFrame, 0, 0, ptd->pDefaultScope, bstrObjectClass,  TRUE);
            FailGracefully(hres, "Failed to add the default scope during AddScopes");
        
            enumState.cScopes++;
        }
    }

    // Enumerate the GC using the GC: ADSI provider, this allows us to 
    // have a single scope in the list, and avoids us having to pass
    // around the GC path to all and sundry.

    if ( SUCCEEDED(GetGlobalCatalogPath(ptd->pServer, szPath, ARRAYSIZE(szPath))) )
    {
        hres = AddScope(ptd->hwndFrame, 
                        enumState.index, 0, 
                        szPath, GC_OBJECTCLASS,  
                        FALSE);

        FailGracefully(hres, "Failed to add GC: too to the scope list");

        enumState.index++;
        enumState.cScopes++;
    }
    else if (  ptd->pDefaultScope )
    {
        //
        // get the domain the user has logged into, and use it to generate a default
        // scope that we can select in the list.
        //

        DWORD dwres;
        PDOMAIN_CONTROLLER_INFOW pdci = NULL;
        ULONG uFlags = DS_RETURN_DNS_NAME|DS_DIRECTORY_SERVICE_REQUIRED;
        INT cchDefaultDnsDomain;

        TraceMsg("No GC discovered, nor was a default scope, so setting default DNS domain accordingly");

        dwres = DsGetDcNameW(ptd->pServer, NULL, NULL, NULL, uFlags, &pdci);

        if ( ERROR_NO_SUCH_DOMAIN == dwres )
        {
            TraceMsg("Trying with rediscovery bit set");
            dwres = DsGetDcNameW(ptd->pServer, NULL, NULL, NULL, uFlags|DS_FORCE_REDISCOVERY, &pdci);
        }
    
        if ( (NO_ERROR == dwres) && pdci->DomainName && (pdci->Flags && DS_DNS_DOMAIN_FLAG) )
        {
            Trace(TEXT("Default domain name is: %s"), W2T(pdci->DomainName));
            
            StrCpyW(szDefaultDnsDomain, pdci->DnsForestName);
            cchDefaultDnsDomain = lstrlenW(szDefaultDnsDomain)-1;

            if ( cchDefaultDnsDomain && szDefaultDnsDomain[cchDefaultDnsDomain] == L'.' )
            {
                TraceMsg("Removing trailing . from the DNS name");
                szDefaultDnsDomain[cchDefaultDnsDomain] = L'\0';
            }

            enumState.pszDefaultDnsDomain = szDefaultDnsDomain;
        }

        NetApiBufferFree(pdci);
    }

    // Get the IDsBrowseDomainTree interface and ask it for the list of 
    // trusted domains.  Once we have that blob add them to the scope list,
    // indenting as requried to indicate the relationship.  If we found a GC
    // then we must indent further, to indicate that all these are to be found
    // in the GC (as it encompases the entire org).

    hres = CoCreateInstance(CLSID_DsDomainTreeBrowser, NULL, CLSCTX_INPROC_SERVER, 
                                        IID_IDsBrowseDomainTree, (LPVOID*)&pDsDomains);
    if ( SUCCEEDED(hres) )
    {
        hres = pDsDomains->SetComputer(ptd->pServer, ptd->pUserName, ptd->pPassword);
        FailGracefully(hres, "Failed when setting computer in the IDsBrowseDomainTree object");

        if ( SUCCEEDED(pDsDomains->GetDomains(&pDomainTree, DBDTF_RETURNFQDN)) ) 
        {
            Trace(TEXT("Domain count from GetDomains %d"), pDomainTree->dwCount);

            hres = _AddFromDomainTree(&enumState, &pDomainTree->aDomains[0], 0);
            FailGracefully(hres, "Failed to add from domain tree");
        }
    }

    hres = S_OK;           // success

exit_gracefully:
    
    // Release all our dangly bits

    DoRelease(pDsObject);
    SysFreeString(bstrObjectClass);

    if ( !enumState.cScopes )
    {
        // we have no scopes, therefore lets inform the user and post a close
        // message to the parent window so we can close it.

        FormatMsgBox(ptd->hwndFrame,
                     GLOBAL_HINSTANCE, IDS_WINDOWTITLE, IDS_ERR_NOSCOPES, 
                     MB_OK|MB_ICONERROR);                        

        PostMessage(ptd->hwndFrame, WM_SYSCOMMAND, SC_CLOSE, 0L);
    }
    else
    {
        // tell tell the frame we ahve added all the scopes we will, that
        // way it can issue the query if the caller wants that.
    
        TraceMsg("Informing frame all scopes have been enumerated");    
        SendMessage(ptd->hwndFrame, CQFWM_ALLSCOPESADDED, 0, 0);           
    }

    if ( pDsDomains )
    {
        pDsDomains->FreeDomains(&pDomainTree);
        DoRelease(pDsDomains);
    }

    if ( ptd )
    {
        LocalFreeStringW(&ptd->pDefaultScope);

        LocalFreeStringW(&ptd->pServer);
        LocalFreeStringW(&ptd->pUserName);
        LocalFreeStringW(&ptd->pPassword);

        LocalFree((HLOCAL)ptd);
    }

    if ( SUCCEEDED(hresCoInit) )
        CoUninitialize();

    TraceLeave();

    InterlockedDecrement(&GLOBAL_REFCOUNT);
    ExitThread(0);
    return 0;
}
