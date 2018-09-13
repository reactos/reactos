#include "pch.h"
#include "dsrole.h"
#pragma hdrstop


/*-----------------------------------------------------------------------------
/ Display specifier helpers/cache functions
/----------------------------------------------------------------------------*/

#define DEFAULT_LANGUAGE      0x409

#define DISPLAY_SPECIFIERS    L"CN=displaySpecifiers"
#define SPECIFIER_PREFIX      L"CN="
#define SPECIFIER_POSTFIX     L"-Display"
#define DEFAULT_SPECIFIER     L"default"


/*-----------------------------------------------------------------------------
/ GetDisplaySpecifier
/ -------------------
/   Get the specified display specifier (sic), given it an LANGID etc.
/
/ In:
/   pccgi -> CLASSCACHEGETINFO structure.
/   riid = interface
/   ppvObject = object requested
/
/ Out:
    HRESULT
/----------------------------------------------------------------------------*/

HRESULT _GetServerConfigPath(LPWSTR *ppszServerConfigPath, LPCLASSCACHEGETINFO pccgi)
{
    HRESULT hres;
    IADs* padsRootDSE = NULL;
    BSTR bstrConfigContainer = NULL;
    VARIANT variant;
    INT cchString;
    LPWSTR pszServer = pccgi->pServer;
#if !DOWNLEVEL_SHELL
    LPWSTR pszMachineServer = NULL;
#endif
    USES_CONVERSION;

    *ppszServerConfigPath = NULL;
    VariantInit(&variant);

    //
    // open the RootDSE for the server we are interested in, if we are using the default
    // server then lets just use the cached version.
    //

    hres = GetCacheInfoRootDSE(pccgi, &padsRootDSE);
#if !DOWNLEVEL_SHELL
    if ( (hres == HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN)) && !pccgi->pServer )
    {
        TraceMsg("Failed to get the RootDSE from the server - not found");

        DSROLE_PRIMARY_DOMAIN_INFO_BASIC *pInfo;
        if ( DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic, (BYTE**)&pInfo) == WN_SUCCESS )
        {
            if ( pInfo->DomainNameDns )
            {
                Trace(TEXT("Machine domain is: %s"), W2T(pInfo->DomainNameDns));

                CLASSCACHEGETINFO ccgi = *pccgi;
                ccgi.pServer = pInfo->DomainNameDns;

                hres = GetCacheInfoRootDSE(&ccgi, &padsRootDSE);
                if ( SUCCEEDED(hres) )
                {
                    hres = LocalAllocStringW(&pszMachineServer, pInfo->DomainNameDns);
                    pszServer = pszMachineServer;
                }
            }

            DsRoleFreeMemory(pInfo);
        }
    }
#endif

    FailGracefully(hres, "Failed to get the IADs for the RootDSE");

    //
    // we now have the RootDSE, so lets read the config container path and compose
    // a string that the outside world cna use
    //

    hres = padsRootDSE->Get(L"configurationNamingContext", &variant);
    FailGracefully(hres, "Failed to get the 'configurationNamingContext' property");

    if ( V_VT(&variant) != VT_BSTR )
        ExitGracefully(hres, E_FAIL, "configurationNamingContext is not a BSTR");

    cchString = lstrlenW(L"LDAP://") + lstrlenW(V_BSTR(&variant));
    
    if ( pszServer )
        cchString += lstrlenW(pszServer) + 1;   // NB: +1 for '/'

    //
    // allocate the buffer we want to use, and fill it
    //

    hres = LocalAllocStringLenW(ppszServerConfigPath, cchString);
    FailGracefully(hres, "Failed to allocate buffer for server path");

    StrCpyW(*ppszServerConfigPath, L"LDAP://");
    
    if ( pszServer )
    {
        StrCatW(*ppszServerConfigPath, pszServer);
        StrCatW(*ppszServerConfigPath, L"/");
    }

    StrCatW(*ppszServerConfigPath, V_BSTR(&variant)); 

    Trace(TEXT("Server config path is: %s"), W2T(*ppszServerConfigPath));
    hres = S_OK;                    // success

exit_gracefully:

    DoRelease(padsRootDSE);
    SysFreeString(bstrConfigContainer);
#if !DOWNLEVEL_SHELL
    LocalFreeStringW(&pszMachineServer);
#endif
    VariantClear(&variant);

    return hres;
}

HRESULT _ComposeSpecifierPath(LPWSTR pSpecifier, LANGID langid, LPWSTR pConfigPath, IADsPathname* pDsPathname, BSTR *pbstrDisplaySpecifier)
{
    TCHAR szLANGID[16];
    WCHAR szSpecifierFull[MAX_PATH];
    USES_CONVERSION;
    
    pDsPathname->Set(pConfigPath, ADS_SETTYPE_FULL);
    pDsPathname->AddLeafElement(DISPLAY_SPECIFIERS);

    if ( !langid )
        langid = GetUserDefaultUILanguage();

    wsprintf(szLANGID, TEXT("CN=%x"), langid);
    pDsPathname->AddLeafElement(T2W(szLANGID));

    if ( pSpecifier )
    {
        StrCpyW(szSpecifierFull, SPECIFIER_PREFIX);
        StrCatW(szSpecifierFull, pSpecifier);
        StrCatW(szSpecifierFull, SPECIFIER_POSTFIX);

        Trace(TEXT("szSpecifierFull: %s"), W2T(szSpecifierFull));
        pDsPathname->AddLeafElement(szSpecifierFull);           // add to the name we are dealing with
    }

    return pDsPathname->Retrieve(ADS_FORMAT_WINDOWS, pbstrDisplaySpecifier);
}

HRESULT GetDisplaySpecifier(LPCLASSCACHEGETINFO pccgi, REFIID riid, LPVOID* ppvObject)
{
    HRESULT hr;
    IADsPathname* pDsPathname = NULL;
    BSTR bstrDisplaySpecifier = NULL;
    LPWSTR pszServerConfigPath = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_CACHE, "GetDisplaySpecifier");
    Trace(TEXT("Display specifier %s, LANGID %x"), W2T(pccgi->pObjectClass), pccgi->langid);

    // When dealing with the local case lets ensure that we enable/disable the flags
    // accordingly.

    if ( !(pccgi->dwFlags & CLASSCACHE_DSAVAILABLE) && !ShowDirectoryUI() )
    {
        ExitGracefully(hr, HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT), "ShowDirectoryUI returned FALSE, and the CLASSCAHCE_DSAVAILABLE flag is not set");
    }
    
    hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER, IID_IADsPathname, (LPVOID*)&pDsPathname);
    FailGracefully(hr, "Failed to get the IADsPathname interface");

    // check to see if we have a valid server config path

    pszServerConfigPath = pccgi->pServerConfigPath;

    if ( !pszServerConfigPath )
    {
        hr = _GetServerConfigPath(&pszServerConfigPath, pccgi);
        FailGracefully(hr, "Failed to allocate server config path");
    }

    hr = _ComposeSpecifierPath(pccgi->pObjectClass, pccgi->langid, pszServerConfigPath, pDsPathname, &bstrDisplaySpecifier);
    FailGracefully(hr, "Failed to retrieve the display specifier path");

    // attempt to bind to the display specifier object, if we fail to find the object
    // then try defaults.

    Trace(TEXT("Calling GetObject on: %s"), W2T(bstrDisplaySpecifier));

    hr = ADsOpenObject(bstrDisplaySpecifier, 
                       pccgi->pUserName, pccgi->pPassword, 
                       pccgi->dwFlags & CLASSCACHE_SIMPLEAUTHENTICATE ? 0:ADS_SECURE_AUTHENTICATION, 
                       riid, ppvObject);

    SysFreeString(bstrDisplaySpecifier);
    if ( hr == HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT) )
    {
        // Display specifier not found. Try the default specifier in the
        // caller's locale. The default specifier is the catch-all for classes
        // that don't have their own specifier.

        hr = _ComposeSpecifierPath(DEFAULT_SPECIFIER, pccgi->langid, pszServerConfigPath, pDsPathname, &bstrDisplaySpecifier);
        FailGracefully(hr, "Failed to retrieve the display specifier path");
        Trace(TEXT("Calling GetObject on: %s"), W2T(bstrDisplaySpecifier));

        hr = ADsOpenObject(bstrDisplaySpecifier, 
                           pccgi->pUserName, pccgi->pPassword, 
                           pccgi->dwFlags & CLASSCACHE_SIMPLEAUTHENTICATE ? 0:ADS_SECURE_AUTHENTICATION, 
                           riid, ppvObject);

        SysFreeString(bstrDisplaySpecifier);
        if ((hr == HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT)) && (pccgi->langid != DEFAULT_LANGUAGE))
        {
            // Now try the object's specifier in the default locale.

            hr = _ComposeSpecifierPath(pccgi->pObjectClass, DEFAULT_LANGUAGE, pszServerConfigPath, pDsPathname, &bstrDisplaySpecifier);
            FailGracefully(hr, "Failed to retrieve the display specifier path");
            Trace(TEXT("Calling GetObject on: %s"), W2T(bstrDisplaySpecifier));

            hr = ADsOpenObject(bstrDisplaySpecifier, 
                               pccgi->pUserName, pccgi->pPassword, 
                               pccgi->dwFlags & CLASSCACHE_SIMPLEAUTHENTICATE ? 0:ADS_SECURE_AUTHENTICATION, 
                               riid, ppvObject);

            SysFreeString(bstrDisplaySpecifier);
            if (hr == HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT))
            {
                // Finally try the default specifier in the default locale.

                hr = _ComposeSpecifierPath(DEFAULT_SPECIFIER, DEFAULT_LANGUAGE, pszServerConfigPath, pDsPathname, &bstrDisplaySpecifier);
                FailGracefully(hr, "Failed to retrieve the display specifier path");
                Trace(TEXT("Calling GetObject on: %s"), W2T(bstrDisplaySpecifier));

                hr = ADsOpenObject(bstrDisplaySpecifier, 
                                   pccgi->pUserName, pccgi->pPassword, 
                                   pccgi->dwFlags & CLASSCACHE_SIMPLEAUTHENTICATE ? 0:ADS_SECURE_AUTHENTICATION, 
                                   riid, ppvObject);

                SysFreeString(bstrDisplaySpecifier);
            }
        }
    }

    FailGracefully(hr, "Failed in ADsOpenObject for display specifier");

    // hr = S_OK;                   // success

exit_gracefully:

    DoRelease(pDsPathname);

    if ( !pccgi->pServerConfigPath )
        LocalFreeStringW(&pszServerConfigPath);

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ GetServerAndCredentails
/ -----------------------
/   Read the server and credentails information from the IDataObject.
/
/ In:
/   pccgi -> CLASSCACHEGETINFO structure to be filled
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT GetServerAndCredentails(CLASSCACHEGETINFO *pccgi)
{
    HRESULT hres;
    STGMEDIUM medium = { TYMED_NULL };
    FORMATETC fmte = {g_cfDsDispSpecOptions, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    USES_CONVERSION;

    TraceEnter(TRACE_UI, "GetServerAndCredentails");

    // we can only get this information if we have a pDataObject to call.

    pccgi->pUserName = NULL;
    pccgi->pPassword = NULL;
    pccgi->pServer = NULL;
    pccgi->pServerConfigPath = NULL;

    if ( pccgi->pDataObject )
    {
        if ( SUCCEEDED(pccgi->pDataObject->GetData(&fmte, &medium)) )
        {
            DSDISPLAYSPECOPTIONS *pdso = (DSDISPLAYSPECOPTIONS*)medium.hGlobal;          
            TraceAssert(pdso);

            // mirror the flags into the CCGI structure

            if ( pdso->dwFlags & DSDSOF_SIMPLEAUTHENTICATE )
            {
                TraceMsg("Setting simple authentication");
                pccgi->dwFlags |= CLASSCACHE_SIMPLEAUTHENTICATE;
            }

            if ( pdso->dwFlags & DSDSOF_DSAVAILABLE )
            {
                TraceMsg("Setting 'DS is available' flags");
                pccgi->dwFlags |= CLASSCACHE_DSAVAILABLE;
            }

            // if we have credentail information that should be copied then lets grab
            // that and put it into the structure.

            if ( pdso->dwFlags & DSDSOF_HASUSERANDSERVERINFO )
            {
                if ( pdso->offsetUserName )
                {
                    LPCWSTR pszUserName = (LPCWSTR)ByteOffset(pdso, pdso->offsetUserName);
                    hres = LocalAllocStringW(&pccgi->pUserName, pszUserName);
                    FailGracefully(hres, "Failed to copy the user name");
                }

                if ( pdso->offsetPassword )
                {
                    LPCWSTR pszPassword = (LPCWSTR)ByteOffset(pdso, pdso->offsetPassword);
                    hres = LocalAllocStringW(&pccgi->pPassword, pszPassword);
                    FailGracefully(hres, "Failed to copy the password");
                }

                if ( pdso->offsetServer )
                {
                    LPCWSTR pszServer = (LPCWSTR)ByteOffset(pdso, pdso->offsetServer);
                    hres = LocalAllocStringW(&pccgi->pServer, pszServer);
                    FailGracefully(hres, "Failed to copy the server");
                }

                if ( pdso->offsetServerConfigPath )
                {
                    LPCWSTR pszServerConfigPath = (LPCWSTR)ByteOffset(pdso, pdso->offsetServerConfigPath);
                    hres = LocalAllocStringW(&pccgi->pServerConfigPath, pszServerConfigPath);
                    FailGracefully(hres, "Failed to copy the server config path");
                }
            }
        }
    }

    hres = S_OK;            // success

exit_gracefully:
    
    if ( FAILED(hres) )
    {
        LocalFreeStringW(&pccgi->pUserName);
        LocalFreeStringW(&pccgi->pPassword);
        LocalFreeStringW(&pccgi->pServer);
        LocalFreeStringW(&pccgi->pServerConfigPath);
    }

    ReleaseStgMedium(&medium);
    
    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ GetAttributePrefix
/ ------------------
/   Get the attribtue prefix we must use to pick up information from the
/   cache / DS.  This is part of the IDataObject we are given, if not then
/   we default to shell behaviour.
/
/ In:
/   ppAttributePrefix -> receives the attribute prefix string
/   pDataObject = IDataObject to query against.
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT GetAttributePrefix(LPWSTR* ppAttributePrefix, IDataObject* pDataObject)
{   
    HRESULT hr;
    STGMEDIUM medium = { TYMED_NULL };
    FORMATETC fmte = {g_cfDsDispSpecOptions, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    PDSDISPLAYSPECOPTIONS pOptions;
    LPWSTR pPrefix = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_UI, "GetAttributePrefix");

    if ( (SUCCEEDED(pDataObject->GetData(&fmte, &medium))) && (medium.tymed == TYMED_HGLOBAL) )
    {
        pOptions = (PDSDISPLAYSPECOPTIONS)medium.hGlobal;
        pPrefix = (LPWSTR)ByteOffset(pOptions, pOptions->offsetAttribPrefix);

        Trace(TEXT("pOptions->dwSize %d"), pOptions->dwSize);
        Trace(TEXT("pOptions->dwFlags %08x"), pOptions->dwFlags);
        Trace(TEXT("pOptions->offsetAttribPrefix %d (%s)"), pOptions->offsetAttribPrefix, W2T(pPrefix));

        hr = LocalAllocStringW(ppAttributePrefix, pPrefix);
        FailGracefully(hr, "Failed when copying prefix from StgMedium");
    }
    else
    {
        hr = LocalAllocStringW(ppAttributePrefix, DS_PROP_SHELL_PREFIX);
        FailGracefully(hr, "Failed when defaulting the attribute prefix string");
    }

    Trace(TEXT("Resulting prefix: %s"), W2T(*ppAttributePrefix));

    // hr = S_OK;                       // success
       
exit_gracefully:

    ReleaseStgMedium(&medium);

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ GetRootDSE
/ ----------
/   Get the RootDSE given an CLASSCACHEGETINFO structure
/
/ In:
/   pccgi -> CLASSCACHEGETINFO structure.
/   pads -> IADs* interface
/
/ Out:
    HRESULT
/----------------------------------------------------------------------------*/
HRESULT GetRootDSE(LPCWSTR pszUserName, LPCWSTR pszPassword, LPCWSTR pszServer, BOOL fNotSecure, IADs **ppads)
{
    HRESULT hres;
    LPWSTR pszRootDSE = L"/RootDSE";
    WCHAR szBuffer[MAX_PATH];
    USES_CONVERSION;

    TraceEnter(TRACE_CACHE, "GetRootDSE");

    StrCpyW(szBuffer, L"LDAP://");

    if ( pszServer )
        StrCatW(szBuffer, pszServer);
    else
        pszRootDSE++;

    StrCatW(szBuffer, pszRootDSE);

    Trace(TEXT("RootDSE path is: %s"), W2T(szBuffer));

    hres = ADsOpenObject(szBuffer, 
                         (LPWSTR)pszUserName, (LPWSTR)pszPassword, 
                         fNotSecure ? 0:ADS_SECURE_AUTHENTICATION, 
                         IID_IADs, (void **)ppads);
    
    TraceLeaveResult(hres);
}

HRESULT GetCacheInfoRootDSE(LPCLASSCACHEGETINFO pccgi, IADs **ppads)
{
    return GetRootDSE(pccgi->pUserName, pccgi->pPassword, pccgi->pServer,
                      (pccgi->dwFlags & CLASSCACHE_SIMPLEAUTHENTICATE),
                      ppads);
}

