/*
 * @(#)securitymanager.cxx 1.0 10/01/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of IInternetHostSecurityManager object for XTL scripting
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include <comcat.h>
#include "securitymanager.hxx"

const GUID GUID_CUSTOM_CONFIRMOBJECTSAFETY = 
	{ 0x10200490, 0xfa38, 0x11d0, { 0xac, 0xe, 0x0, 0xa0, 0xc9, 0xf, 0xff, 0xc0 }};

// Uncomment the lines below to allow checking of safe script engines, safe object initialization (respectively)
// #define CHECK_ENGINE_SAFETY
// #define CHECK_INIT_SAFETY

SecurityManager::SecurityManager()
{
    _bstrURL = NULL;
    _fBlanketDisallow = false;
    _fSecurityContext = true;
}

SecurityManager::~SecurityManager()
{
    ::SysFreeString(_bstrURL);
}

extern HRESULT CreateSecurityManager(IInternetSecurityManager ** ppUnk);
static IInternetSecurityManager *g_pSecurityMgr = NULL;

HRESULT
SecurityManager::Init(DWORD dwSafetyOptions, IUnknown *pSite, String *sURL)
{
    HRESULT hr = S_OK;

    _fBlanketDisallow = false;

    // if there are no safetyOptions then we don't have a security context
    _fSecurityContext = ((dwSafetyOptions & (INTERFACESAFE_FOR_UNTRUSTED_CALLER | INTERFACESAFE_FOR_UNTRUSTED_DATA)) != 0);

    // if we don't have a security context, allow free access
    if (!_fSecurityContext)
        goto Cleanup;

    // if we have a security context but no site, then we blanket disallow instantiation
    // we also need a URL to pass through to the security manager
    // If we don't have that, we can't guarantee safety, so again blanket disallow instantiation
    if (!pSite || !sURL)
    {
        _fBlanketDisallow = true;
        goto Cleanup;
    }


    TRY
    {
        // Set the URL for delegation.  Check for the URL being the empty string and again disallow if so
        _bstrURL = sURL->getBSTR();
        if (!_bstrURL)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        if (lstrlenW(_bstrURL) == 0)
        {   
            _fBlanketDisallow = true;
            goto Cleanup;
        }   
    
        // finally, discover where the security manager is
        hr = CreateSecurityManager(&g_pSecurityMgr);

        // If class_not_registered is returned, then somehow the user has
        // copied our dll to somewhere where IE is not installed. 
        // To be safe we disallow everything.  Others errors are returned, 
        // it means the security manager is around but we can't get to it.
        //
        if (hr == REGDB_E_CLASSNOTREG)
        {
            _fBlanketDisallow = true;
            hr = S_OK;
        }
    }
    CATCH
    {
        hr = E_FAIL;
    }
    ENDTRY

Cleanup:    
    return hr;
}

HRESULT 
SecurityManager::GetSecurityId( 
    BYTE *pbSecurityId,
    DWORD *pcbSecurityId,
    DWORD_PTR dwReserved)
{
    if (_fSecurityContext)
    {
        if (_fBlanketDisallow)
        {
            return E_FAIL;
        }

        Assert(_bstrURL && g_pSecurityMgr);
        return g_pSecurityMgr->GetSecurityId(
                    (LPCWSTR)_bstrURL, 
                    pbSecurityId, 
                    pcbSecurityId,
                    dwReserved);
    }
    else
    {
        // This routine is supposed to return a string in the form <scheme>:<domain>+<zone>
        // The code in the default security manager in URLMON is not exported 
        // in a public function, and is a lot of code to add to the dll

        // I'm not sure if the current client (script engine) calls this
        // at all.  Typically clients do not parse the string but simply
        // use it as a cookie, using strcmp to test for equality.
    
        // so let's return null string to allow such methodolgies to work.

        if (!pbSecurityId || *pcbSecurityId == 0)
        {
            return E_INVALIDARG;
        }

        *pbSecurityId = '\0';

        return S_OK;
    }
}
    
HRESULT 
SecurityManager::ProcessUrlAction( 
    DWORD dwAction,
    BYTE *pPolicy,
    DWORD cbPolicy,
    BYTE *pContext,
    DWORD cbContext,
    DWORD dwFlags,
    DWORD dwReserved)
{
    if (_fSecurityContext && !_fBlanketDisallow)
    {
        Assert(g_pSecurityMgr && _bstrURL);
        return (g_pSecurityMgr->ProcessUrlAction(
                    (LPCWSTR)_bstrURL, 
                     dwAction,
                     pPolicy,
                     cbPolicy,
                     pContext,
                     cbContext,
                     dwFlags,
                     dwReserved)
               );
    }
    else
    {
        if (!pPolicy)
            return E_POINTER;

        *pPolicy = NULL;

        if (cbPolicy < sizeof(DWORD))
            return E_INVALIDARG;

        *((DWORD *)pPolicy) = _fSecurityContext ? URLPOLICY_DISALLOW : URLPOLICY_ALLOW;

        return S_OK;
    }
}

HRESULT 
SecurityManager::QueryCustomPolicy( 
    REFGUID guidKey,
    BYTE **ppPolicy,
    DWORD *pcbPolicy,
    BYTE *pContext,
    DWORD cbContext,
    DWORD dwReserved)
{
    DWORD policy;
    bool fAllow;
    HRESULT hr;
    IActiveScript * pScript = NULL;

    if (!ppPolicy || !pcbPolicy)
        return E_POINTER;

    *ppPolicy = NULL;
    *pcbPolicy = 0;

    policy = URLPOLICY_DISALLOW;

    if (_fSecurityContext)
    {
        if (!_fBlanketDisallow)
        {
            Assert(_bstrURL && g_pSecurityMgr);
            hr = g_pSecurityMgr->QueryCustomPolicy(
                    (LPCWSTR)_bstrURL,
                    guidKey,
                    ppPolicy,
                    pcbPolicy,
                    pContext,
                    cbContext,
                    dwReserved);

            if (hr != INET_E_DEFAULT_ACTION &&
                hr != HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
                    goto Cleanup;

            if (guidKey == GUID_CUSTOM_CONFIRMOBJECTSAFETY)
            {
                CONFIRMSAFETY * pConfirm;
                DWORD dwAction;
                SAFETYOPERATION safety;
                const IID *     piid;

                // This code has been adapted from Trident
                // 
                // This is a special guid meaning that some embedded object
                // within is itself trying to create another object.  For us
                // currently, this is an xsl script engine creating an activex obj.
                //
                // Basically we have to see if this new object is safe for scripting before 
                // allowing it to pass. We also attempt to MAKE it safe as well via IObjectSafety.
                //
                // We get the clsid and the IUnknown of the object passed in from
                // the context.

                // Theoretically the security manager could be augmented to check for other types
                // of security (See SAFETYOPERATION enumeration in .hxx).  These include
                // checking security of a script engine itself (SAFETY_SCRIPTENGINE) or the
                // initialization of an object (SAFETY_INIT).  For now these paths are
                // commented out as they are not used

                if (cbContext != sizeof(CONFIRMSAFETY))
                {
                    hr = E_INVALIDARG;
                    goto Cleanup;
                }

                if (pContext == NULL)   // good ol' paranoia
                {
                    hr = E_POINTER;
                    goto Cleanup;
                }

                pConfirm = (CONFIRMSAFETY *)pContext;
                if (!pConfirm->pUnk)
                    goto Cleanup;
            
                // safety handling is a little different depending on whether the
                // newly nested embedded object is a script engine or a regular ol' object
#ifdef CHECK_ENGINE_SAFETY
                if (!SUCCEEDED(pConfirm->pUnk->QueryInterface(IID_IActiveScript, (void **)&pScript)) && pScript)
                {
#endif
                    dwAction = URLACTION_ACTIVEX_OVERRIDE_SCRIPT_SAFETY;
                    safety = SAFETY_SCRIPT;
                    piid = &IID_IDispatch;
#ifdef CHECK_ENGINE_SAFETY
                }
                else
                {
                    dwAction = URLACTION_SCRIPT_OVERRIDE_SAFETY;
                    safety = SAFETY_SCRIPTENGINE;
                    piid = &IID_IActiveScript;
                }
#endif        

       
                // first see if the internet permissions allow all objects to be scripted
                // (surfaces in UI as "Initialize and Script ActiveX Controls not marked as safe - enable)"
                hr = IsAllowableAction(dwAction, &fAllow);
                if (!SUCCEEDED(hr))
                    goto Cleanup;
                if (fAllow)
                    policy = URLPOLICY_ALLOW;
                else
                {
                    // check to see if this object is safe 
                    if (CheckSafety(safety, *piid, pConfirm->clsid, pConfirm->pUnk))
                        policy = URLPOLICY_ALLOW;
                }
                hr = S_OK;
                goto AllocPolicy;
            }
        }
    }
    else /* no security manager */
    {
        policy = URLPOLICY_ALLOW;
        hr = S_OK;
    }

AllocPolicy:
    *ppPolicy = (BYTE *)CoTaskMemAlloc(sizeof(DWORD));
    if (*ppPolicy)
    {
        *(DWORD *)*ppPolicy = policy;
        *pcbPolicy = sizeof(DWORD);
    }
    else
        hr = E_OUTOFMEMORY;
Cleanup:
    if (pScript)
        pScript->Release();
    return hr;
}


extern HRESULT CreateCatalogInformation(ICatInformation ** ppUnk);
static ICatInformation *g_pCatInfo = NULL;

// Verify safety for a particular action (on a particular interface).
bool
SecurityManager::CheckSafety(SAFETYOPERATION sOperation, REFIID riid, CLSID clsid, IUnknown *pUnk)
{
    bool fSafe = false;
    HRESULT hr = E_FAIL;
    DWORD   dwCompat = 0;
    CATID   catid = GUID_NULL;  // category of safety
    DWORD   dwXSetMask = 0; // options to set
    DWORD   dwXOptions = 0; // options to make safe for
                            // (either INTERFACESAFE_FOR_UNTRUSTED_CALLER or
                            // INTERFACESAFE_FOR_UNTRUSTED_DATA)

    IObjectSafety *pOSafe = NULL;

    switch (sOperation)
    {
    case SAFETY_INIT:
#ifdef CHECK_INIT_SAFETY
        catid = CATID_SafeForInitializing;
        dwXSetMask = INTERFACESAFE_FOR_UNTRUSTED_DATA;
        dwXOptions = INTERFACESAFE_FOR_UNTRUSTED_DATA;
#else
        Assert(FALSE);
#endif
        break;

    case SAFETY_SCRIPT:
        catid = CATID_SafeForScripting;
        dwXSetMask = INTERFACESAFE_FOR_UNTRUSTED_CALLER;
        dwXOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER;
        break;

    case SAFETY_SCRIPTENGINE:
#ifdef CHECK_ENGINE_SAFETY
        catid = GUID_NULL;  // Registry check is not sufficient for script engines
        dwXOptions = dwXSetMask =   INTERFACESAFE_FOR_UNTRUSTED_DATA
                                  | INTERFACE_USES_DISPEX 
                                  | INTERFACE_USES_SECURITY_MANAGER;
#else
        Assert(FALSE);
#endif
        break;

    default:
        Assert(FALSE);
    }

    // Trident maintains a "compatibility table" which basically
    // is a list of controls that it doesn't believe are safe
    // (IObjectSafety implementation bogus) OR are downright
    // nasty and should never be loaded.  
    //
    // To be consistent with Trident we look at this table assuming
    // it is available.
    hr = CompatFlagsFromClsid(clsid, &dwCompat);
    if (!SUCCEEDED(hr))
        goto Cleanup;

    // If the control is downright evil then get out immediately
    if (dwCompat & COMPAT_EVIL_DONT_LOAD)
    {
        Assert(!fSafe);
        goto Cleanup;
    }

    // If no object safety is allowed, go straight to confirm because we 
    // don't believe what this control says about it's safety.  
    // Basically we think the control is lying to us, or implemented IObjectSafety 
    // incorrectly.
    if (dwCompat & COMPAT_NO_OBJECTSAFETY)
        goto Confirm;
        
    // Otherwise see if IObjectSafety is supported, and if so, 
    // ask the object to make itself safe
    hr = pUnk->QueryInterface(IID_IObjectSafety, (void **) &pOSafe);
    if (SUCCEEDED(hr) && pOSafe)
    {
#ifdef CHECK_ENGINE_SAFETY
        //
        // If we're trying to make it safe to script, ask object
        // if it knows about dispex2 & sec mgr.  If not, then bail out
        // if it's a script engine, continue otherwise.
        //
        if (sOperation == SAFETY_SCRIPTENGINE)
        {
            DWORD   dwMask = 0;
            DWORD   dwEnabled;
            
            hr = pOSafe->GetInterfaceSafetyOptions(
                    riid,
                    &dwMask,
                    &dwEnabled);
            if (hr || !(dwMask & INTERFACE_USES_DISPEX))
                goto Cleanup;
        }
#endif

        //
        // If we're going for safe for scripting, try making the object
        // safe on IDispatchEx first, then drop to IDispatch (typically supplied by client)
        //
        hr = E_FAIL;
        if (sOperation == SAFETY_SCRIPT)
            hr = pOSafe->SetInterfaceSafetyOptions(IID_IDispatchEx, dwXSetMask, dwXOptions);
        if (!SUCCEEDED(hr))
            hr = pOSafe->SetInterfaceSafetyOptions(riid, dwXSetMask, dwXOptions);
        if (!SUCCEEDED(hr))
        {
            // Give user an opportunity to override.
            goto Confirm;
        }

        // Don't check registry if object supports IObjectSafety.
        goto EnsureSafeForScripting;
    }

    // otherwise looking in the registry to see if the object
    // belongs to the appropriate component category
    hr = CreateCatalogInformation(&g_pCatInfo);
    if (hr)
        goto Cleanup;
        
    CATID rgcatid[1];
    rgcatid[0] = catid;

    // Ask if the object belongs to the specified category
    // returns S_FALSE if not
    hr = g_pCatInfo->IsClassOfCategories(clsid, 1, rgcatid, 0, NULL);
    if (hr)
        goto Confirm;

EnsureSafeForScripting:
    // Object is safe on this interface!
    //
    // Though object appears to be safe, we still need to
    // see if scripting to objects is allowed at all.
    //
    if (sOperation == SAFETY_SCRIPT)
        IsAllowableAction(URLACTION_SCRIPT_SAFE_ACTIVEX, &fSafe);   // ignore error, assume not safe
    else
        fSafe = true;

    // all done, cleanup and return
    goto Cleanup;

Confirm:
    // We come here for unsafe objects 
    IsAllowableAction(URLACTION_ACTIVEX_CONFIRM_NOOBJECTSAFETY, &fSafe);    // ignore error, assume not safe

Cleanup:
    if (pOSafe)
        pOSafe->Release();
    return fSafe;
}


/* see if the specified action has allowable policy */
HRESULT 
SecurityManager::IsAllowableAction(DWORD dwAction, bool *pfAllow)
{
    HRESULT hr;
    DWORD policy;

    Assert(pfAllow);

    *pfAllow = false;

    hr = ProcessUrlAction(dwAction, (BYTE *)&policy, sizeof(DWORD), NULL, 0, 0, 0);

    if (hr == S_FALSE)  // disallow typically returns FALSE
        hr = S_OK;

    if (SUCCEEDED(hr))
        *pfAllow = (GetUrlPolicyPermissions(policy) == URLPOLICY_ALLOW);

    return hr;
}


//+------------------------------------------------------------------------
//
// Function:: HexUCSToL
//
// Converts unsigned Hex String to unsigned long.  Not in SHLWAPI, so roll our own.
// Only handles unsigned numbers, doesn't handle overflow
//
//-------------------------------------------------------------------------
static DWORD UHexStrToL(LPTSTR lpStr)
{
    DWORD res = 0;
    DWORD digit;
    TCHAR ch;
     
    do {
        ch = *lpStr++;
        if (ch >= _T('A') && ch <= _T('F'))
            digit = 10 + (ch - _T('A'));
        else if (ch >= _T('a') && ch <= _T('f'))
            digit = 10 + (ch - _T('a'));
        else if (ch >= _T('0') && ch <= _T('9'))
            digit = (ch - _T('0'));
        else 
            break;
        res = (res << 4) + digit;
    } while (true);

    return res;
}

//+------------------------------------------------------------------------
//
//  Function:   CompatFlagsFromClsid
//
//  Synopsis:   Get compatibility flags for given clsid.
//

//
// Trident maintains a "compatibility table" which is a list of controls that 
// it stores some information about.  We are interested in what
// the table says about how safe the object is.  There are two
// levels (1) don't trust what the object say about its own safety
// (IObjectSafety implementation bogus), 92) the control is downright
// nasty and should never be loaded.  The table is in the registry. 
// It's propagated there from the mshtml.dll at RegSvr32 time, accounting for versions etc.
//
// This routine reads the flags from the registry and returns them to the client for
// further processing
//
// Registry keys from Trident to get to the table
static TCHAR szTableRootKey[] = _T("Software\\Microsoft\\Internet Explorer\\ActiveX Compatibility");
static TCHAR szCompatFlags[] = _T("Compatibility Flags");

HRESULT CompatFlagsFromClsid(REFCLSID clsid, DWORD *pdwCompat)
{
    HRESULT hr;   
    HKEY hkeyRoot = NULL;
    HKEY hkeyClsid = NULL;
    DWORD dwSize, dwType;
    LPOLESTR pszClsid = NULL;
    union RKValue { 
        DWORD dw;
        TCHAR tch[11];
    } rkValue;

    Assert(pdwCompat != NULL );
    *pdwCompat = 0;  

    // find the table root
    hr = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE, 
        szTableRootKey,
        0,
        KEY_READ,
        &hkeyRoot);

    // if it's not there then bypass checking.  Trident is not around (server side, probably)
    if (hr != ERROR_SUCCESS)
    {
        hr = S_FALSE;    
        goto Cleanup;
    }

    // now go for the flags based on classid
    // error is E_OUTOFMEMORY typically
    hr = StringFromCLSID(clsid, &pszClsid);
    if(FAILED(hr))
        goto Cleanup;

    // Open the {####-####-####...} Clsid subkey:
    hr = RegOpenKeyEx(
        hkeyRoot,
        pszClsid,
        0,
        KEY_READ,
        &hkeyClsid);

    // if the object is not on the list, assume it's not special
    if (hr != ERROR_SUCCESS)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    // Get the flags from the subkeys named values. 
    dwSize = sizeof(rkValue);
    hr = RegQueryValueEx(
        hkeyClsid, 
        szCompatFlags,
        NULL,
        &dwType, 
        (LPBYTE)&rkValue,
        &dwSize);

    // if there are no flags , assume it's not special
    if (hr != ERROR_SUCCESS)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    // The rkValue struct mechanism is due to the fact that the .inf tools
    // sometimes put STRINGS rather than DWORDS in the registry on win95. (UGH)
    // Again, if either value is missing we just assume the object is not special
    if (dwType == REG_DWORD)
        *pdwCompat = rkValue.dw;
    else if (dwType == REG_SZ && dwSize > 2 )
        *pdwCompat = UHexStrToL(rkValue.tch+2);     // skip past 0x
        
Cleanup:
    if (hkeyClsid)
        RegCloseKey(hkeyClsid);

    if (hkeyRoot)
        RegCloseKey(hkeyRoot);

    if (pszClsid)
        CoTaskMemFree(pszClsid);

    return(hr);
}
