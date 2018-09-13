//+--------------------------------------------------------------------------
//
//  File:       help.cxx
//
//  Contents:   Helpers for help
//
//---------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

EXTERN_C const GUID LIBID_MSHTML;
EXTERN_C const GUID CLSID_HTMLDocument;

//+-------------------------------------------------------------------------
//
//  Function:   FormsHelp
//
//  Synopsis:   Helper for WinHelp
//
//  Arguments:  [uCmd]      type of help - see WinHelp
//              [dwData]    additional data - see WinHelp
//
//--------------------------------------------------------------------------

HRESULT
FormsHelp(TCHAR * szHelpFile, UINT uCmd, DWORD dwData)
{
    BOOL        fRet;

    fRet = WinHelp(
                TLS(gwnd.hwndGlobalWindow),
                szHelpFile,
                uCmd,
                dwData);

    RRETURN(fRet ? S_OK : E_FAIL);
}

//+-------------------------------------------------------------------------
//
//  Method:     GetTypeInfoForCLSID
//
//  Synopsis:   Gets the TypeInfo for the CLSID by chasing through the
//              registry and into the TypeLib.
//
//--------------------------------------------------------------------------

HRESULT
GetTypeInfoForCLSID(HKEY hkRoot, REFCLSID clsid, ITypeInfo ** ppTI)
{
    OLECHAR       szGuidTyp[128];
    TCHAR       szVersion[128];
    TCHAR       szTypeLib[128];
    IID         iid;
    ITypeLib *  pTypeLib = NULL;
    ITypeInfo * pTypeInfo;
    long        cb;
    long        err;
    HKEY        hkType = NULL;
    HRESULT     hr;

    // Get TypeLib GUID.
    Format(0, szGuidTyp, ARRAY_SIZE(szGuidTyp), _T("<0g>\\TypeLib"), &clsid);
    cb = ARRAY_SIZE(szGuidTyp) * sizeof(TCHAR);
    err = TW32_NOTRACE(1, RegQueryValue(hkRoot, szGuidTyp, szGuidTyp, &cb));
    if (err != ERROR_SUCCESS)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // Translate the guid from the registry into an iid.
    hr = THR(IIDFromString(szGuidTyp, &iid));
    if (hr)
        goto Cleanup;

    // See if this idd matches our TypeLib.
    if (IsEqualIID(iid, LIBID_MSHTML))
    {
        hr = THR(LoadF3TypeInfo(clsid, &pTypeInfo));
        if (!hr)
            goto Complete;

        goto Cleanup;
    }

    // Get Version.
    Format(0, szVersion, ARRAY_SIZE(szVersion), _T("<0g>\\Version"), &clsid);
    cb = ARRAY_SIZE(szVersion) * sizeof(TCHAR);
    err = TW32_NOTRACE(1, RegQueryValue(hkRoot, szVersion, szVersion, &cb));
    if (err != ERROR_SUCCESS)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // Open TypeLib key.
    err = TW32_NOTRACE(1,RegOpenKey(HKEY_CLASSES_ROOT,TEXT("TypeLib"),&hkType));
    if (err != ERROR_SUCCESS)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // CONSIDER - At some point getting the localized TypeLib.  Here we always
    // get the default TypeLib. (rodc)
    //
    // Get TypeLib path and filename.
    Format(0, szTypeLib, ARRAY_SIZE(szTypeLib), _T("<0s>\\<1s>\\0\\win32"),
            szGuidTyp,
            szVersion);
    cb = ARRAY_SIZE(szTypeLib) * sizeof(TCHAR);
    err = TW32_NOTRACE(1, RegQueryValue(hkType, szTypeLib, szTypeLib, &cb));
    if (err != ERROR_SUCCESS)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // Load the type library.
    hr = THR(LoadTypeLib(szTypeLib, &pTypeLib));
    if (hr)
        goto Cleanup;

    // Get the type info for this specific classid.
    hr = THR(pTypeLib->GetTypeInfoOfGuid(clsid, &pTypeInfo));
    if (hr)
        goto Cleanup;

Complete:
    *ppTI = pTypeInfo;

Cleanup:
    if (hkType)
        RegCloseKey(hkType);
    ReleaseInterface(pTypeLib);
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     GetDocumentationForCLSID
//
//  Synopsis:   Gets the TypeInfo documentation for the CLSID.
//
//--------------------------------------------------------------------------

HRESULT
GetDocumentationForCLSID(
        HKEY hkRoot,
        REFCLSID clsid,
        BSTR * pbstrName,
        DWORD * pdwHelpContextId,
        BSTR * pbstrHelpFile)
{
    ITypeInfo * pTypeInfo = NULL;
    HRESULT     hr;

    // Get the TypeInfo for this specific classid.
    hr = THR_NOTRACE(GetTypeInfoForCLSID(hkRoot, clsid, &pTypeInfo));
    if (hr)
        goto Cleanup;

    // Get the documentation for this classid from the TypeInfo.
    hr = THR(pTypeInfo->GetDocumentation(
            MEMBERID_NIL,
            pbstrName,
            NULL,
            pdwHelpContextId,
            pbstrHelpFile));
    if (hr)
        goto Cleanup;

Cleanup:
    ReleaseInterface(pTypeInfo);
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     GetNameForCLSID
//
//  Synopsis:   Gets the name for the CLSID.
//
//--------------------------------------------------------------------------

HRESULT
GetNameForCLSID(HKEY hkRoot, REFCLSID clsid, TCHAR * szName, int cch)
{
    TCHAR       szUser[128];
    BSTR        bstrName = NULL;
    long        cb;
    long        err;
    HRESULT     hr;

    Assert(szName);

    // Otherwise, try the name for this classid.
    hr = THR_NOTRACE(GetDocumentationForCLSID(
            hkRoot,
            clsid,
            &bstrName,
            NULL,
            NULL));
    if (!hr)
    {
        // If we got the name, copy it and get out.
        _tcsncpy(szName, bstrName, cch);
        goto Cleanup;
    }

    // Next, try to get the AuxUserType name.
    Format(0, szUser, ARRAY_SIZE(szUser), _T("<0g>\\AuxUserType\\2"), &clsid);
    cb = cch * sizeof(TCHAR);
    err = TW32_NOTRACE(1, RegQueryValue(hkRoot, szUser, szName, &cb));
    if (err == ERROR_SUCCESS)
    {
        // If we got the name, get out.
        hr = S_OK;
        goto Cleanup;
    }

    // Finally, if nothing else worked, load unknown.
    if (LoadString(GetResourceHInst(), IDS_UNKNOWN, szName, cch))
        hr = S_OK;
    else
        hr = GetLastWin32Error();

Cleanup:
    FormsFreeString(bstrName);
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     GetHelpForCLSID
//
//  Synopsis:   Gets the help for the CLSID.
//
//--------------------------------------------------------------------------

HRESULT
GetHelpForCLSID(
        HKEY hkRoot,
        REFCLSID clsid,
        DWORD * pdwId,
        TCHAR * szHelpFile,
        int cch)
{
    BSTR        bstrHelp = NULL;
    HRESULT     hr;

    // Get the help info for this classid.
    hr = THR_NOTRACE(GetDocumentationForCLSID(
            hkRoot,
            clsid,
            NULL,
            pdwId,
            &bstrHelp));
    if (hr)
        goto Cleanup;

    // If we didn't get a help path\name, then get out.
    if (!bstrHelp || !FormsStringLen(bstrHelp))
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // If we got the name, copy it and get out.
    _tcsncpy(szHelpFile, bstrHelp, cch);

Cleanup:
    FormsFreeString(bstrHelp);
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
//
//  Method:     OnDialogHelp
//
//  Synopsis:   Display dialog box's help
//
//--------------------------------------------------------------------------

HRESULT
OnDialogHelp(
        CBase * pBase,
        HELPINFO * phi, 
        DWORD dwHelpContextID) 
{
    HRESULT     hr;
    TCHAR       szPath[_MAX_PATH];
    HKEY        hkRoot = NULL;

    // Get the key to the CLSID root.
    hr = THR(RegDbOpenCLSIDKey(&hkRoot));
    if (hr)
        goto Cleanup;

    // Get the help file name.
    hr = THR_NOTRACE(GetHelpForCLSID(
            hkRoot,
            CLSID_HTMLDocument,
            NULL,
            szPath,
            ARRAY_SIZE(szPath)));
    if (hr)
        goto Cleanup;

    _tcscat(szPath, _T(">LangRef"));

    BOOL    fRet;
    
    fRet = WinHelp(
            TLS(gwnd.hwndGlobalWindow), 
            szPath, 
            HELP_CONTEXT, 
            dwHelpContextID);
    hr = THR(fRet ? S_OK : E_FAIL);
Cleanup:
    if (hkRoot)
        Verify(RegCloseKey(hkRoot) == ERROR_SUCCESS);
    RRETURN(hr);  
}
