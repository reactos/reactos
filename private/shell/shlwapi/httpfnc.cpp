//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       httpfnc.c
//
//  Contents:   Common HTTP helper functions
//
//  Classes:
//
//  Functions:
//
//  History:    1-20-97   tonyci
//
//--------------------------------------------------------------------------

#include "priv.h"
#pragma  hdrstop

#include "objidl.h"
#include "urlmon.h"
#include "exdisp.h"     // IWebBrowserApp
#include "shlobj.h"     // IShellBrowser
#include "inetreg.h"

// Internal helper functions
HRESULT wrap_CreateFormatEnumerator( UINT cfmtetc, FORMATETC* rgfmtetc, IEnumFORMATETC** ppenumfmtetc);
HRESULT wrap_RegisterFormatEnumerator(LPBC pBC, IEnumFORMATETC *pEFetc, DWORD reserved);          
STDAPI common_GetAcceptLanguages(CHAR *psz, LPDWORD pcch);

#define SETFMTETC(p, a)	{(p)->cfFormat = (a); \
                         (p)->dwAspect = DVASPECT_CONTENT; \
                         (p)->lindex = -1; \
                         (p)->tymed = TYMED_ISTREAM; \
                         (p)->ptd = NULL;}

typedef struct _SA_BSTRGUID {
    UINT  cb;
    WCHAR wsz[39];
} SA_BSTRGUID;

const SA_BSTRGUID s_sstrEFM = {
    38 * SIZEOF(WCHAR),
    L"{D0FCA420-D3F5-11CF-B211-00AA004AE837}"    
};


//+-------------------------------------------------------------------------
//
//  Function:  
//
//  Synopsis:   
//
//
//  Returns:
//
//  History:  
//
//  Notes:
//
//--------------------------------------------------------------------------

STDAPI CreateDefaultAcceptHeaders(VARIANT* pvar, IWebBrowserApp* pdie)
{
    IEnumFORMATETC* pEFM;
    HKEY hkey;
    HRESULT hr = S_OK;

    if (RegOpenKey(HKEY_LOCAL_MACHINE,
            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Accepted Documents"),
            &hkey) == ERROR_SUCCESS)
    {
        FORMATETC *prgfmtetc = NULL;
        DWORD iValue = 0;
        TCHAR szValueName[128];
        DWORD cchValueName = ARRAYSIZE(szValueName);
        DWORD dwType;

        // Count the types in the registry
        while (RegEnumValue(hkey, iValue++, szValueName, &cchValueName,
                            NULL, &dwType, NULL, NULL)==ERROR_SUCCESS)
        { // purpose is to increment iValue
            cchValueName = ARRAYSIZE(szValueName);
        };

        // Previous loop ends +1, so no need to add +1 for CF_NULL
        
        prgfmtetc = (FORMATETC *)LocalAlloc (LPTR, iValue * sizeof(FORMATETC));
        if (prgfmtetc)
        {
            FORMATETC   *pcurfmtetc = prgfmtetc;
            for (DWORD nValue=0; nValue < iValue; nValue++)
            {
                TCHAR szFormatName[128];
                DWORD cchFormatName = ARRAYSIZE(szFormatName);

                cchValueName = ARRAYSIZE(szValueName);
                if (RegEnumValue(hkey, nValue, szValueName, &cchValueName, NULL,
                                 &dwType, (LPBYTE) szFormatName, &cchFormatName)==ERROR_SUCCESS)
                {
                    pcurfmtetc->cfFormat = (CLIPFORMAT)
                        RegisterClipboardFormat(szFormatName);

                    ASSERT (pcurfmtetc->cfFormat != 0);

                    if (pcurfmtetc->cfFormat)
                    {
                        SETFMTETC (pcurfmtetc, pcurfmtetc->cfFormat);
                    }
                    pcurfmtetc++;   // move to next fmtetc
                } // if RegEnum
            } // for nValue

            // for the last pcurfmtetc, we fill in for CF_NULL
            // no need to do RegisterClipboardFormat("*/*")
    
            // iValue was calculated above and already includes space for
            // the CF_NULL enumerator.  It is the correct number of
            // enumerators already.
    
            SETFMTETC (pcurfmtetc, CF_NULL);
    
            hr = wrap_CreateFormatEnumerator (iValue, prgfmtetc, &pEFM);
            if (EVAL(SUCCEEDED(hr)))
            {
                ASSERT(pvar->vt == VT_EMPTY);
                pvar->vt = VT_UNKNOWN;
                pvar->punkVal = (IUnknown *)pEFM;
                hr = pdie->PutProperty((BSTR)s_sstrEFM.wsz, *pvar);
                if (FAILED(hr))
                {
                    pEFM->Release();  // if we failed to pass ownership on, free EFM
                    pvar->vt = VT_EMPTY;
                    pvar->punkVal = NULL;
                }
            }
    
            LocalFree (prgfmtetc);
        }
        else
        {  // prgfmtetc
            hr = E_OUTOFMEMORY;
        }

        RegCloseKey(hkey);
    }
    else
    {
        DebugMsg(TF_ERROR, TEXT("RegOpenkey failed!"));
        hr = E_FAIL;
    }

    return hr;
}  // CreateDefaultAcceptHeaders

//+-------------------------------------------------------------------------
//
//  Function:  
//
//  Synopsis:   
//
//
//  Returns:
//
//  History:  
//
//  Notes:
//
//--------------------------------------------------------------------------

STDAPI RegisterDefaultAcceptHeaders(IBindCtx* pbc, LPSHELLBROWSER psb)
{
    IEnumFORMATETC* pEFM;
    IWebBrowserApp* pdie;

    HRESULT hres; 

    ASSERT(pbc);
    ASSERT(psb);

    hres = IUnknown_QueryService(psb, IID_IWebBrowserApp, IID_IWebBrowserApp, (LPVOID*)&pdie);
    if (SUCCEEDED(hres))
    {
            VARIANT var = { VT_EMPTY };

            hres = pdie->GetProperty((BSTR)s_sstrEFM.wsz, &var);
            if (SUCCEEDED(hres))
            {

                // To avoid linking to OleAut32, we will handle ONLY the
                // expected VT_UNKNOWN and VT_EMPTY cases.

                if (var.vt == VT_EMPTY) {
#ifdef FULL_DEBUG
                    DebugMsg(DM_TRACE, TEXT("RegisterDefaultAcceptHeaders var.vt == VT_EMPTY"));
#endif
                    CreateDefaultAcceptHeaders(&var, pdie);

                    if (var.vt == VT_UNKNOWN)
                        var.punkVal->Release();
                } else

                if (var.vt == VT_UNKNOWN) {
#ifdef FULL_DEBUG
                    DebugMsg(DM_TRACE, TEXT("RegisterDefaultAcceptHeaders var.vt == VT_UNKNOWN"));
#endif
                    hres = var.punkVal->QueryInterface(IID_IEnumFORMATETC, (LPVOID*)&pEFM);
                    if (SUCCEEDED(hres)) {
                        IEnumFORMATETC* pEFMClone = NULL;
                        hres = pEFM->Clone(&pEFMClone);
                        if (SUCCEEDED(hres)) {
#ifdef FULL_DEBUG
                            DebugMsg(DM_TRACE, TEXT("RegisterDefaultAcceptHeaders registering FormatEnum %x"), pEFMClone);
#endif
                            hres = wrap_RegisterFormatEnumerator(pbc, pEFMClone, 0);
                            pEFMClone->Release();
                        } else {
                            DebugMsg(TF_ERROR, TEXT("RegisterDefaultAcceptHeaders Clone failed %x"), hres);
                        }
                        pEFM->Release();
                    }
                    var.punkVal->Release();

                } else {
                    DebugMsg(TF_ERROR, TEXT("GetProperty() returned illegal Variant Type: %x"), var.vt);
                    DebugMsg(TF_ERROR, TEXT("RegisterDefaultAcceptHeaders not registering FormatEnum"));
                    ASSERT(var.vt == VT_UNKNOWN);
                }

            } else {
                DebugMsg(TF_ERROR, TEXT("RegisterDefaultAcceptHeaders pdie->GetProperty() failed %x"), hres);
            }

            pdie->Release();
    } else {
        DebugMsg(TF_ERROR, TEXT("RegisterDefaultAcceptHeaders QueryService(ISP) failed %x"), hres);
    }

    return hres;
} // RegisterDefaultAcceptHeaders

STDAPI GetAcceptLanguagesA(LPSTR pszLanguages, LPDWORD pcchLanguages)
{
    return common_GetAcceptLanguages(pszLanguages, pcchLanguages);
}  // GetAcceptLanguagesA

STDAPI GetAcceptLanguagesW(LPWSTR pwzLanguages, LPDWORD pcchLanguages)
{
    DWORD   dwcchMaxOut;
    HRESULT hr;

    if (!pwzLanguages || !pcchLanguages || !*pcchLanguages)
        return E_FAIL;

    dwcchMaxOut = *pcchLanguages;

    LPSTR  psz = (LPSTR) LocalAlloc (LPTR, dwcchMaxOut);
    if (!psz)
        return E_OUTOFMEMORY;

    hr = common_GetAcceptLanguages(psz, &dwcchMaxOut);

    if (SUCCEEDED(hr))
    {
        *pcchLanguages = MultiByteToWideChar(
                             CP_ACP, 0, psz, -1,
                             pwzLanguages, *pcchLanguages - 1);

        pwzLanguages[*pcchLanguages] = 0;
    }

    LocalFree(psz);
    return hr;
} // GetAcceptLanguagesW

STDAPI common_GetAcceptLanguages(CHAR *psz, LPDWORD pcch)
{
// BUGBUG 15feb97 this is a copy of the inetcpl's language table.  we should
// share some header that defines this, instead of duplciate maint.  (tonyci)

    typedef struct _languagetbl
    {
        DWORD   dwLCID;         // LCID
        CHAR   *pAcceptLang;    // Accept Language string
    } LANGUAGETBL;

// BUGBUG 15feb97 these should be ordered by frequency, not alphabet, to reduce
// search time (tonyci)

// this is intentionally CHAR, since it is going to become an http header
    const static LANGUAGETBL aLanguageTable[] =
    {
        { 0x0436, "af"    },
        { 0x041C, "sq"    },
        { 0x0401, "ar-sa" },
        { 0x0801, "ar-iq" },
        { 0x0C01, "ar-eg" },
        { 0x1001, "ar-ly" },
        { 0x1401, "ar-dz" },
        { 0x1801, "ar-ma" },
        { 0x1C01, "ar-tn" },
        { 0x2001, "ar-om" },
        { 0x2401, "ar-ye" },
        { 0x2801, "ar-sy" },
        { 0x2C01, "ar-jo" },
        { 0x3001, "ar-lb" },
        { 0x3401, "ar-kw" },
        { 0x3801, "ar-ae" },
        { 0x3C01, "ar-bh" },
        { 0x4001, "ar-qa" },
        { 0x042D, "eu"    },
        { 0x0402, "bg"    },
        { 0x0423, "be"    },
        { 0x0403, "ca"    },
        { 0x0404, "zh-tw" },
        { 0x0804, "zh-cn" },
        { 0x0C04, "zh-hk" },
        { 0x1004, "zh-sg" },
        { 0x041A, "hr"    },
        { 0x0405, "cs"    },
        { 0x0406, "da"    },
        { 0x0413, "nl"    },
        { 0x0813, "nl-be" },
        { 0x0000, "en"    },
        { 0x0409, "en-us" },
        { 0x0809, "en-gb" },
        { 0x0C09, "en-au" },
        { 0x1009, "en-ca" },
        { 0x1409, "en-nz" },
        { 0x1809, "en-ie" },
        { 0x1C09, "en-za" },
        { 0x2009, "en-jm" },
        { 0x2409, "en"    },
        { 0x2809, "en-bz" },
        { 0x2C09, "en-tt" },
        { 0x0425, "et"    },
        { 0x0438, "fo"    },
        { 0x0429, "fa"    },
        { 0x040B, "fi"    },
        { 0x040C, "fr"    },
        { 0x080C, "fr-be" },
        { 0x0C0C, "fr-ca" },
        { 0x100C, "fr-ch" },
        { 0x140C, "fr-lu" },
        { 0x043C, "gd"    },
        { 0x0407, "de"    },
        { 0x0807, "de-ch" },
        { 0x0C07, "de-at" },
        { 0x1007, "de-lu" },
        { 0x1407, "de-li" },
        { 0x0408, "el"   },
        { 0x040D, "he"    },
        { 0x0439, "hi"    },
        { 0x040E, "hu"    },
        { 0x040F, "is"    },
        { 0x0421, "in"    },
        { 0x0410, "it"    },
        { 0x0810, "it-ch" },
        { 0x0411, "ja"    },
        { 0x0412, "ko"    },
        { 0x0812, "ko"    },
        { 0x0426, "lv"    },
        { 0x0427, "lt"    },
        { 0x042F, "mk"    },
        { 0x043E, "ms"    },
        { 0x043A, "mt"    },
        { 0x0414, "no"    },
        { 0x0814, "no"    },
        { 0x0415, "pl"    },
        { 0x0416, "pt-br" },
        { 0x0816, "pt"    },
        { 0x0417, "rm"    },
        { 0x0418, "ro"    },
        { 0x0818, "ro-mo" },
        { 0x0419, "ru"    },
        { 0x0819, "ru-mo" },
        { 0x0C1A, "sr"    },
        { 0x081A, "sr"    },
        { 0x041B, "sk"    },
        { 0x0424, "sl"    },
        { 0x042E, "sb"    },
        { 0x040A, "es"    },
        { 0x080A, "es-mx" },
        { 0x0C0A, "es"    },
        { 0x100A, "es-gt" },
        { 0x140A, "es-cr" },
        { 0x180A, "es-pa" },
        { 0x1C0A, "es-do" },
        { 0x200A, "es-ve" },
        { 0x240A, "es-co" },
        { 0x280A, "es-pe" },
        { 0x2C0A, "es-ar" },
        { 0x300A, "es-ec" },
        { 0x340A, "es-cl" },
        { 0x380A, "es-uy" },
        { 0x3C0A, "es-py" },
        { 0x400A, "es-bo" },
        { 0x440A, "es-sv" },
        { 0x480A, "es-hn" },
        { 0x4C0A, "es-ni" },
        { 0x500A, "es-pr" },
        { 0x0430, "sx"    },
        { 0x041D, "sv"    },
        { 0x081D, "sv-fi" },
        { 0x041E, "th"    },
        { 0x0431, "ts"    },
        { 0x0432, "tn"    },
        { 0x041F, "tr"    },
        { 0x0422, "uk"    },
        { 0x0420, "ur"    },
        { 0x042A, "vi"    },
        { 0x0434, "xh"    },
        { 0x043D, "ji"    },
        { 0x0435, "zu"    },
    };

    HKEY hk;
    BOOL hr = E_FAIL;

    if (!psz || !pcch || !*pcch)
        return hr;

    if ((RegOpenKey (HKEY_CURRENT_USER, REGSTR_PATH_INTERNATIONAL, &hk) == ERROR_SUCCESS) && hk) {
        DWORD dwType;

        if (RegQueryValueEx (hk, REGSTR_VAL_ACCEPT_LANGUAGE, NULL, &dwType, (UCHAR *)psz, pcch) != ERROR_SUCCESS) {

            // When there is no AcceptLanguage key, we have to default
            DWORD LCID = GetUserDefaultLCID();
            hr = S_OK;   // assume we will find locale in the table
            *psz = 0;

            for (int i = 0; i < ARRAYSIZE(aLanguageTable); i++) {
                if (LCID == aLanguageTable[i].dwLCID) {
                    lstrcpynA(psz, aLanguageTable[i].pAcceptLang, *pcch);
                    *pcch = lstrlenA(psz);
                    break;
                }
            }
            if (i == ARRAYSIZE(aLanguageTable)) {
                // failed to find user's locale in our table
                AssertMsg(*psz, TEXT("We should add LCID 0x%lx to the language table"), LCID);
                *pcch = 0;
                hr = E_FAIL;
            }
            
        } else {
            hr = S_OK;
            if (!*psz) {
                // A NULL AcceptLanguage means send no A-L: header
                hr = S_FALSE;
            }
        }

        RegCloseKey (hk);
    } 

    return hr;
}  // w_GetAcceptLanguages
    

//
// Both of these functions will be called only once per browser session - the
// first time we create the FormatEnumerator.  After that, we will use the one
// we created, rather than needing to call these to allocate a new one.
//

static const TCHAR sszURLMON[] = TEXT("URLMON.DLL");

HRESULT wrap_RegisterFormatEnumerator(LPBC pBC, IEnumFORMATETC *pEFetc, DWORD reserved)
{

    HRESULT     hr = E_FAIL;
    HINSTANCE   hurl = NULL;
    BOOL        fLoaded = FALSE;

    hurl = GetModuleHandle(sszURLMON);
    if (!hurl) {
        hurl = LoadLibrary (sszURLMON);
        fLoaded = TRUE;
    }
    if (hurl) {
        
        HRESULT (*pfnRFE)(LPBC pBC, IEnumFORMATETC * pEFetc, DWORD reserved);

        pfnRFE = (HRESULT (*)(LPBC, IEnumFORMATETC*, DWORD))GetProcAddress (hurl, "RegisterFormatEnumerator");
        if (pfnRFE) {
            hr = pfnRFE(pBC, pEFetc, reserved);
        }

        if (fLoaded)
            FreeLibrary (hurl);
    }

    return hr;
}  // wrap_RegisterFormatEnumerator

HRESULT wrap_CreateFormatEnumerator( UINT cfmtetc, FORMATETC* rgfmtetc, IEnumFORMATETC** ppenumfmtetc)
{

    HRESULT     hr = E_FAIL;
    HINSTANCE   hurl = NULL;
    BOOL        fLoaded = FALSE;

    hurl = GetModuleHandle(sszURLMON);
    if (!hurl) {
        hurl = LoadLibrary (sszURLMON);
        fLoaded = TRUE;
    }
    if (hurl) {
        
        HRESULT (*pfnCFE)(UINT cfmtetc, FORMATETC* rgfmtetc, IEnumFORMATETC **ppenumfmtetc);

        pfnCFE = (HRESULT (*)(UINT, FORMATETC*, IEnumFORMATETC **))GetProcAddress (hurl, "CreateFormatEnumerator");
        if (pfnCFE) {
            hr = pfnCFE(cfmtetc, rgfmtetc, ppenumfmtetc);
        }

        if (fLoaded)
            FreeLibrary (hurl);
    }

    return hr;
} // wrap_CreateFormatEnumerator
