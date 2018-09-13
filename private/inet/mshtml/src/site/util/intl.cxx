//---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       intl.cxx
//
//  Contents:   Internationalization Support Functions
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_MLANG_H_
#define X_MLANG_H_
#include <mlang.h>
#endif

#ifndef X_COMMCTRL_H_
#define X_COMMCTRL_H_
#undef WINCOMMCTRLAPI
#include <commctrl.h>
#endif

#ifndef X_USP_HXX_
#define X_USP_HXX_
#include "usp.hxx"
#endif

#ifndef X_WCHDEFS_H
#define X_WCHDEFS_H
#include <wchdefs.h>
#endif

#ifndef X_TXTDEFS_H
#define X_TXTDEFS_H
#include "txtdefs.h"
#endif

#ifndef X_ALTFONT_H
#define X_ALTFONT_H
#include <altfont.h>
#endif

#include <inetreg.h>


// We could make a wrapper class for these
IMultiLanguage *g_pMultiLanguage = NULL;
IMultiLanguage2 *g_pMultiLanguage2 = NULL; // JIT langpack support

// Globals
PMIMECPINFO g_pcpInfo = NULL;
ULONG       g_ccpInfo = 0;
BOOL        g_cpInfoInitialized = FALSE;
TCHAR       g_szMore[32];

extern BYTE g_bUSPJitState;

#ifdef UNIX
// Unix MIDL doesn't output this.
extern "C" const CLSID CLSID_CMultiLanguage;
#endif

MtDefine(MimeCpInfo, PerProcess, "MIMECPINFO")
MtDefine(MimeCpInfoTemp, Locals, "MIMECPINFO_temp")
MtDefine(CpCache, PerProcess, "CpCache")
MtDefine(achKeyForCpCache, PerProcess, "achKeyForCpCache")

//BUGBUG - Remove Thai, Arabic DOS and Hebrew DOS
//         from this array after MLANG adds it to
//         the MIME database
#define BREAK_ITEM 20

static struct {
    CODEPAGE cp;
    BYTE     bGDICharset;
} s_aryCpMap[] = {
    { CP_1252,  ANSI_CHARSET },
    { CP_1252,  SYMBOL_CHARSET },
#if !defined(WIN16) && !defined(WINCE)
    { CP_1250,  EASTEUROPE_CHARSET },
    { CP_1251,  RUSSIAN_CHARSET },
    { CP_1253,  GREEK_CHARSET },
    { CP_1254,  TURKISH_CHARSET },
    { CP_1255,  HEBREW_CHARSET },
    { CP_1256,  ARABIC_CHARSET },
    { CP_1257,  BALTIC_CHARSET },
    { CP_1258,  VIETNAMESE_CHARSET },
    { CP_THAI,  THAI_CHARSET },
#endif // !WIN16 && !WINCE
    { CP_UTF_8, ANSI_CHARSET }
};

//
// Note: This list is a small subset of the mlang list, used to avoid
// loading mlang in certain cases.
//
// *** There should be at least one entry in this list for each entry
//     in s_aryCpMap so that we can look up the name quickly. ***
//
// NB (cthrash) List obtained from the following URL, courtesy of ChristW
//
//   http://iptdweb/intloc/internal/redmond/projects/ie4/charsets.htm

//BUGBUG - Remove Thai, Arabic DOS and Hebrew DOS
//         from this array after MLANG adds it to
//         the MIME database

static const MIMECSETINFO s_aryInternalCSetInfo[] = {
    { CP_1252,  CP_ISO_8859_1,  _T("iso-8859-1") },
    { CP_1252,  CP_1252,  _T("windows-1252") },
    { NATIVE_UNICODE_CODEPAGE, CP_UTF_8, _T("utf-8") },
    { NATIVE_UNICODE_CODEPAGE, CP_UTF_8, _T("unicode-1-1-utf-8") },
    { NATIVE_UNICODE_CODEPAGE, CP_UTF_8, _T("unicode-2-0-utf-8") },
    { CP_1250,  CP_1250,  _T("windows-1250") },     // Eastern European windows
    { CP_1251,  CP_1251,  _T("windows-1251") },     // Cyrillic windows
    { CP_1253,  CP_1253,  _T("windows-1253") },     // Greek windows
    { CP_1254,  CP_1254,  _T("windows-1254") },     // Turkish windows
    { CP_1257,  CP_1257,  _T("windows-1257") },     // Baltic windows
    { CP_1258,  CP_1258,  _T("windows-1258") },     // Vietnamese windows
    { CP_1255,  CP_1255,  _T("windows-1255") },     // Hebrew windows
    { CP_1256,  CP_1256,  _T("windows-1256") },     // Arabic windows
    { CP_THAI,  CP_THAI,  _T("windows-874") },      // Thai Windows
};



#define CCPDEFAULT 1 // new spec: the real default is CP_ACP only


// declaration for static members
ULONG CCachedCPInfo::_ccpInfo = CCPDEFAULT;
BOOL  CCachedCPInfo::_fCacheLoaded  = FALSE;
BOOL  CCachedCPInfo::_fCacheChanged = FALSE;
LPTSTR CCachedCPInfo::_pchRegKey = NULL;
CPCACHE CCachedCPInfo::_CpCache[5] = 
{
    {CP_ACP, 0, 0},
};

// useful macros...

//------------------------------------------------------------------------
//
//  Function:   IsPrimaryCodePage
//
//  Synopsis:   Return whether the structure in cpinfo represents the
//              'primary' codepage for a language.   Only primary codepages
//              are added, for example, to the language drop down menu.
//
//  Note:       Add special cases to pick primary codepage here.  In most
//              cases the comparison between the uiCodePage and
//              uiFamilyCodePage suffices.  HZ and Korean autodetect are 
//              other cases to consider.
//
//------------------------------------------------------------------------

inline UINT GetAutoDetectCp(PMIMECPINFO pcpinfo)
{
    UINT cpAuto;
    switch (pcpinfo->uiFamilyCodePage)
    {
        case CP_JPN_SJ:
           cpAuto = CP_AUTO_JP;
           break;
        default:
           cpAuto = CP_UNDEFINED;
           break;
    }
    return cpAuto;
}

inline BOOL IsPrimaryCodePage(MIMECPINFO *pcpinfo)
{
    UINT cpAuto = GetAutoDetectCp(pcpinfo);
    if (cpAuto != CP_UNDEFINED)
        return pcpinfo->uiCodePage == cpAuto;
    else
        return pcpinfo->uiCodePage == pcpinfo->uiFamilyCodePage;
}

// clean up the last unsupport cp if it's there
void CCachedCPInfo::RemoveInvalidCp (void)
{
    for (UINT iCache = CCPDEFAULT; iCache < _ccpInfo; iCache++)
    {
        if (_CpCache[iCache].ulIdx == (ULONG)-1)
        {
            _ccpInfo--;
            if (_ccpInfo > iCache)
                memmove(&_CpCache[iCache], &_CpCache[iCache+1], sizeof(_CpCache[iCache])*(_ccpInfo-iCache));

            break;
        }
    }
}

static const TCHAR s_szCNumCpCache[] = TEXT("CNum_CpCache");
static const TCHAR s_szCpCache[] = TEXT("CpCache");
const TCHAR s_szPathInternational[] = TEXT("\\International");
//------------------------------------------------------------------------
//
//  Function:   CCachedCPInfo::InitCpCache
//  
//              Initialize the cache with default codepages
//              which do not change through the session
//
//------------------------------------------------------------------------
void CCachedCPInfo::InitCpCache (OPTIONSETTINGS *pOS, PMIMECPINFO pcp, ULONG ccp)
{
    UINT iCache, iCpInfo;

    
    if  (pcp &&  ccp > 0)
    {
        // load the existing setting from registry one time
        //
        LoadSetting(pOS);
        
        // clean up the last unsupport cp if it's there
        RemoveInvalidCp();
        
        // initialize the current setting including default
        for (iCache= 0; iCache < _ccpInfo; iCache++)
        {
            for (iCpInfo= 0; iCpInfo < ccp; iCpInfo++)
            {
                if ( pcp[iCpInfo].uiCodePage == _CpCache[iCache].cp )
                {
                    _CpCache[iCache].ulIdx = iCpInfo;

                    break;
                }   
            }
        }
    }
}

//------------------------------------------------------------------------
//
//  Function:   CCachedCPInfo::SaveCodePage
//  
//              Cache the given codepage along with the index to
//              the given array of MIMECPINFO
//
//------------------------------------------------------------------------
void CCachedCPInfo::SaveCodePage (UINT codepage, PMIMECPINFO pcp, ULONG ccp)
{
    int ccpSave = -1;
    BOOL bCached = FALSE;
    UINT i;

    // we'll ignore CP_AUTO, it's a toggle item on our menu
    if (codepage == CP_AUTO)
        return;

    // first check if we already have this cp
    for (i = 0; i < _ccpInfo; i++)
    {
        if (_CpCache[i].cp == codepage)
        {
            ccpSave = i;
            bCached = TRUE;
            break;
        }
    }
    
    // if cache is not full, use the current
    // index to an entry
    if (ccpSave < 0  && _ccpInfo < ARRAY_SIZE(_CpCache))
    {
        ccpSave =  _ccpInfo;
    }

    //  otherwise, popout the least used entry. 
    //  the default codepages always stay
    //  this also decriments the usage count
    int cMinUsed = ARRAY_SIZE(_CpCache);
    UINT iMinUsed = 0; 
    for ( i = CCPDEFAULT; i < _ccpInfo; i++)
    {
        if (_CpCache[i].cUsed > 0)
            _CpCache[i].cUsed--;
        
        if ( ccpSave < 0 && _CpCache[i].cUsed < cMinUsed)
        {
            cMinUsed =  _CpCache[i].cUsed;
            iMinUsed =  i;
        }
    }
    if (ccpSave < 0)
        ccpSave = iMinUsed; 
    
    // set initial usage count, which goes down to 0 if it doesn't get 
    // chosen twice in a row (with current array size)
    _CpCache[ccpSave].cUsed = ARRAY_SIZE(_CpCache)-1;
    
    // find a matching entry from given array of
    // mimecpinfo. Note that we always reassign the index
    // to the global array even when we've already had it 
    // in our cache because the index can be different 
    // after we jit-in a langpack
    //
    if (pcp &&  ccp > 0)
    {
        for (i= 0; i < ccp; i++)
        {
            if ( pcp[i].uiCodePage == codepage )
            {
                _CpCache[ccpSave].cp = codepage;
                _CpCache[ccpSave].ulIdx = i;

                break;
            }   
        }
    }
    
    if (!bCached)
    {
        _fCacheChanged = TRUE;
        if (_ccpInfo < ARRAY_SIZE(_CpCache))

            _ccpInfo++;

        // fallback case for non support codepages
        // we want have it on the tier1 menu but make
        // it disabled
        if ( i >= ccp )
        {
            _CpCache[ccpSave].cp = codepage;
            _CpCache[ccpSave].ulIdx = (ULONG)-1;
        }
    }
}

//------------------------------------------------------------------------
//
//  Function:   CCachedCPInfo::SaveSetting
//  
//              Writes out the current cache setting to registry
//              [REVIEW]
//              Can this be called from DeinitMultiLanguage?
//              Can we guarantee shlwapi is on memory there?
//
//------------------------------------------------------------------------
void CCachedCPInfo::SaveSetting()
{
    UINT iCache;
    DWORD dwcbData, dwcNumCpCache;
    PUINT pcpCache;
    HRESULT hr;
    
    if (!_fCacheLoaded || !_fCacheChanged)
        goto Cleanup;

    RemoveInvalidCp();

    dwcNumCpCache = _ccpInfo-CCPDEFAULT; 
    
    dwcbData = sizeof(_CpCache[0].cp)*dwcNumCpCache; // DWORD for a terminator ?
    if (dwcNumCpCache > 0 && _pchRegKey)
    {
        pcpCache = (PUINT)MemAlloc(Mt(CpCache), dwcbData + sizeof (DWORD));
        if (pcpCache)
        {
            // Save the current num of cache
            hr =  SHSetValue(HKEY_CURRENT_USER, _pchRegKey, 
                       s_szCNumCpCache, REG_DWORD, (void *)&dwcNumCpCache, sizeof(dwcNumCpCache));

            if (hr == ERROR_SUCCESS)
            {
               // Save the current tier1 codepages
               for (iCache = 0; iCache <  dwcNumCpCache; iCache++)
                   pcpCache[iCache] = _CpCache[iCache+CCPDEFAULT].cp;
    
               IGNORE_HR(SHSetValue(HKEY_CURRENT_USER, _pchRegKey, 
                          s_szCpCache, REG_BINARY, (void *)pcpCache, dwcbData));
            }
            MemFree(pcpCache);
        }
    }
Cleanup:
    if (_pchRegKey)
    {
        MemFree(_pchRegKey);
        _pchRegKey = NULL;
    }
}

void CCachedCPInfo::LoadSetting(OPTIONSETTINGS *pOS)
{
    UINT iCache;
    DWORD dwType, dwcbData, dwcNumCpCache;
    PUINT pcpCache;
    HRESULT hr;

    if (_fCacheLoaded)
        return;

    _ccpInfo = CCPDEFAULT;

    // load registry path from option settings
    if (_pchRegKey == NULL)
    {
        LPTSTR pchKeyRoot = pOS ? pOS->achKeyPath : TSZIEPATH;
        _pchRegKey = (LPTSTR)MemAlloc(Mt(achKeyForCpCache), 
                  (_tcslen(pchKeyRoot)+ARRAY_SIZE(s_szPathInternational)+1) * sizeof(TCHAR));
        if (!_pchRegKey)
            goto loaddefault;

        _tcscpy(_pchRegKey, pchKeyRoot);
        _tcscat(_pchRegKey, s_szPathInternational);
    }

    // first see if we have any entry 
    dwcbData = sizeof (dwcNumCpCache);
    hr =  SHGetValue(HKEY_CURRENT_USER, _pchRegKey, 
                   s_szCNumCpCache, &dwType, (void *)&dwcNumCpCache, &dwcbData);

    if (hr == ERROR_SUCCESS && dwcNumCpCache > 0)
    {
        if (dwcNumCpCache > ARRAY_SIZE(_CpCache)-CCPDEFAULT)
            dwcNumCpCache = ARRAY_SIZE(_CpCache)-CCPDEFAULT;
    
        dwcbData = sizeof(_CpCache[0].cp)*dwcNumCpCache; // DWORD for a terminator 
        pcpCache = (PUINT)MemAlloc(Mt(CpCache), dwcbData + sizeof (DWORD));
        if (pcpCache)
        {
            hr = THR(SHGetValue(HKEY_CURRENT_USER, _pchRegKey, 
                s_szCpCache, &dwType, (void *)pcpCache, &dwcbData));
                
            if (hr == ERROR_SUCCESS && dwType == REG_BINARY)
            {
                PUINT p = pcpCache;
                for (iCache=CCPDEFAULT; iCache < dwcNumCpCache+CCPDEFAULT; iCache++) 
                {
                    _CpCache[iCache].cp = *p;
                    _CpCache[iCache].cUsed = ARRAY_SIZE(_CpCache)-1;
                    p++;
                }
                _ccpInfo += dwcNumCpCache;
            }
            
            MemFree(pcpCache);
        }
    }
    
    // Load default entries that stay regardless of menu selection
    // Add ACP to the default entry of the cache
    //
loaddefault:
    MIMECPINFO mimeCpInfo;
    UINT       cpAuto;
    for (iCache = 0; iCache < CCPDEFAULT; iCache++)
    {
        _CpCache[iCache].cUsed = ARRAY_SIZE(_CpCache)-1;
        if (_CpCache[iCache].cp == CP_ACP )
            _CpCache[iCache].cp = GetACP();

        IGNORE_HR(QuickMimeGetCodePageInfo( _CpCache[iCache].cp, &mimeCpInfo ));
        
        cpAuto = GetAutoDetectCp(&mimeCpInfo);
        _CpCache[iCache].cp = (cpAuto != CP_UNDEFINED ? cpAuto : _CpCache[iCache].cp);
    }

    // this gets set even if we fail to read reg setting.
    _fCacheLoaded = TRUE;
}

//+-----------------------------------------------------------------------
//
//  Function:   EnsureCodePageInfo
//
//  Synopsis:   Hook up to mlang.  Note: it is not an error condition to
//              get back no codepage info structures from mlang.
//
//  IE5 JIT langpack support:
//              Now user can install fonts and nls files on the fly 
//              without restarting the session.
//              This means we always have to get real information
//              as to which codepage is valid and not.
//
//------------------------------------------------------------------------

HRESULT
EnsureCodePageInfo(BOOL fForceRefresh)
{
    if (g_cpInfoInitialized && !fForceRefresh)
        return S_OK;

    HRESULT         hr;
    UINT            cNum;
    IEnumCodePage * pEnumCodePage = NULL;

    PMIMECPINFO     pcpInfo = NULL;
    ULONG           ccpInfo = 0;

    hr = THR(EnsureMultiLanguage());
    if (hr)
        goto Cleanup;

    hr = MlangEnumCodePages(MIMECONTF_BROWSER, &pEnumCodePage);
    if (hr)
        goto Cleanup;

    if (g_pMultiLanguage2)
        hr = THR(g_pMultiLanguage2->GetNumberOfCodePageInfo(&cNum));
    else
        hr = THR(g_pMultiLanguage->GetNumberOfCodePageInfo(&cNum));
    if (hr)
        goto Cleanup;

    if (cNum > 0)
    {
        pcpInfo = (PMIMECPINFO)MemAlloc(Mt(MimeCpInfo), sizeof(MIMECPINFO) * (cNum));
        if (!pcpInfo)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        hr = THR(pEnumCodePage->Next(cNum, pcpInfo, &ccpInfo));
        if (hr)
            goto Cleanup;

        if (ccpInfo > 0)
        {
            IGNORE_HR(MemRealloc(Mt(MimeCpInfo), (void **)&pcpInfo, sizeof(MIMECPINFO) * (ccpInfo)));
        }
        else
        {
            MemFree(pcpInfo);
            pcpInfo = NULL;
        }
    }

Cleanup:
    {
        LOCK_GLOBALS;

        if (!hr)
        {
            if (g_pcpInfo)
            {
                MemFree(g_pcpInfo);
            }

            g_pcpInfo = pcpInfo;
            g_ccpInfo = ccpInfo;
        }

        // If any part of the initialization fails, we do not want to keep
        // returning this function, unless, of course, fForceRefresh is true.

        g_cpInfoInitialized = TRUE;
    }

    ReleaseInterface(pEnumCodePage);
    RRETURN(hr);
}

#ifndef NO_MULTILANG
//+-----------------------------------------------------------------------
//
//  Function:   DeinitMultiLanguage
//
//  Synopsis:   Detach from mlang.  Called from DllAllThreadsDetach.
//              Globals are locked during the call.
//
//------------------------------------------------------------------------

void
DeinitMultiLanguage()
{
    CCachedCPInfo::SaveSetting(); // [review] can this be dengerous? [yutakan]
    MemFree(g_pcpInfo);
    g_cpInfoInitialized = FALSE;
    g_pcpInfo = NULL;
    g_ccpInfo = 0;
    ClearInterface(&g_pMultiLanguage2);
    ClearInterface(&g_pMultiLanguage);

}
#endif

//+-----------------------------------------------------------------------
//
//  Function:   EnsureMultiLanguage
//
//  Synopsis:   Make sure mlang is loaded.
//
//------------------------------------------------------------------------
HRESULT
EnsureMultiLanguage()
{
    if (g_pMultiLanguage)
        return S_OK;

    HRESULT hr = S_OK;

    LOCK_GLOBALS;

    // Need to check again after locking globals.
    if (g_pMultiLanguage)
        goto Cleanup;

    hr = THR(CoCreateInstance(
            CLSID_CMultiLanguage,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IMultiLanguage,
            (void**)&g_pMultiLanguage));
    if (hr)
        goto Cleanup;

    // JIT langpack
    // get the aggregated ML2, this may fail if IE5 is not present
    //
    IGNORE_HR(g_pMultiLanguage->QueryInterface(
            IID_IMultiLanguage2, 
            (void **)&g_pMultiLanguage2));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Function:   MlangEnumCodePages
//
//              Just a utility function that wraps IML(2)::EnumCodePages
//              supposedly use the curernt UI language for mlang call
//
// bugbug: do we really want to live without IE5?
//
//------------------------------------------------------------------------
HRESULT
MlangEnumCodePages(DWORD grfFlags, IEnumCodePage **ppEnumCodePage)
{
    HRESULT hr = EnsureMultiLanguage();

    if (hr == S_OK && ppEnumCodePage)
    {
        if (g_pMultiLanguage2)
        {
            hr = THR(g_pMultiLanguage2->EnumCodePages(
                grfFlags, MLGetUILanguage(),
                ppEnumCodePage));
        }
        else
        {
            hr = THR(g_pMultiLanguage->EnumCodePages(
                grfFlags,  ppEnumCodePage));
        }
    }
    RRETURN(hr);
}

HRESULT
MlangValidateCodePage(CDoc *pDoc, CODEPAGE cp, HWND hwnd, BOOL fUserSelect)
{
    HRESULT hr = EnsureMultiLanguage();
    if (hr == S_OK)
    {
        if (g_pMultiLanguage2)
        {
            DWORD dwfIODControl = 0;
            if (pDoc)
                dwfIODControl |= (pDoc->_dwLoadf&DLCTL_SILENT) ? CPIOD_PEEK : 0;
            if (fUserSelect)
                dwfIODControl |= CPIOD_FORCE_PROMPT;
          
            hr = THR(g_pMultiLanguage2->ValidateCodePageEx(cp, hwnd, dwfIODControl ));
        }
        else
        {
            // it's not a success but we don't want to fail
            hr = S_OK;
        }
    }
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Function:   QuickMimeGetCodePageInfo
//
//  Synopsis:   This function gets cp info from mlang, but caches some
//              values to avoid loading mlang unless necessary.
//
//------------------------------------------------------------------------

HRESULT
SlowMimeGetCodePageInfo(CODEPAGE cp, PMIMECPINFO pMimeCpInfo)
{
    HRESULT hr = THR(EnsureMultiLanguage());
    if (hr == S_OK)
    {
        if (g_pMultiLanguage2)
            hr = THR(g_pMultiLanguage2->GetCodePageInfo(cp, MLGetUILanguage(), pMimeCpInfo));
        else
            hr = THR(g_pMultiLanguage->GetCodePageInfo(cp, pMimeCpInfo));
    }
    return hr;
}

HRESULT
QuickMimeGetCodePageInfo(CODEPAGE cp, PMIMECPINFO pMimeCpInfo)
{
    HRESULT hr = S_OK;
#ifdef WIN16
    // ## v-gsrir -> TIMES NEW ROMAN default font
    static MIMECPINFO s_mimeCpInfoBlank = {
        0, 0, 0, _T(""), _T(""), _T(""), _T(""),
        _T("Courier New"), _T("TIMES NEW ROMAN"), DEFAULT_CHARSET
    };
#else
    static MIMECPINFO s_mimeCpInfoBlank = {
        0, 0, 0, _T(""), _T(""), _T(""), _T(""),
        _T("Courier New"), _T("MS Sans Serif"), DEFAULT_CHARSET
    };
#endif

    // If mlang is not loaded and it is acceptable to use our defaults,
    //  try to avoid loading it by searching through our internal
    //  cache.
    for (int n = 0; n < ARRAY_SIZE(s_aryCpMap); ++n)
    {
        if (cp == s_aryCpMap[n].cp)
        {
            memcpy(pMimeCpInfo, &s_mimeCpInfoBlank, sizeof(MIMECPINFO));
            pMimeCpInfo->uiCodePage = pMimeCpInfo->uiFamilyCodePage = s_aryCpMap[n].cp;
            pMimeCpInfo->bGDICharset = s_aryCpMap[n].bGDICharset;

            // Search for the name in the cset array
            for (int j = 0; j < ARRAY_SIZE(s_aryInternalCSetInfo); ++j)
            {
                if (cp == s_aryInternalCSetInfo[j].uiInternetEncoding)
                {
                    _tcscpy(pMimeCpInfo->wszWebCharset,
                            s_aryInternalCSetInfo[j].wszCharset);
                    break;
                }
            }

            // Assert there was an entry in s_aryInternalCSetInfo for this codepage
            Assert(j < ARRAY_SIZE(s_aryInternalCSetInfo));
            return S_OK;
        }
    }

#ifndef NO_MULTILANG
    hr = SlowMimeGetCodePageInfo(cp, pMimeCpInfo);
    if (hr)
        goto Error;
#endif // !NO_MULTILANG

Cleanup:
    return hr;

Error:
    // Could not load mlang, fill in with a default but return the error
    memcpy(pMimeCpInfo, &s_mimeCpInfoBlank, sizeof(MIMECPINFO));
    goto Cleanup;
}


#ifndef NO_MULTILANG
//+-----------------------------------------------------------------------
//
//  Function:   QuickMimeGetCharsetInfo
//
//  Synopsis:   This function gets charset info from mlang, but caches some
//              values to avoid loading mlang unless necessary.
//
//------------------------------------------------------------------------

HRESULT
QuickMimeGetCharsetInfo(LPCTSTR lpszCharset, PMIMECSETINFO pMimeCSetInfo)
{
    HRESULT hr = S_OK;

    // If mlang is not loaded and it is acceptable to use our defaults,
    //  try to avoid loading it by searching through our internal
    //  cache.
    for (int n = 0; n < ARRAY_SIZE(s_aryInternalCSetInfo); ++n)
    {
        if (_tcsicmp((TCHAR*)lpszCharset, s_aryInternalCSetInfo[n].wszCharset) == 0)
        {
            *pMimeCSetInfo = s_aryInternalCSetInfo[n];
            return S_OK;
        }
    }

    hr = THR(EnsureMultiLanguage());
    if (hr)
        goto Cleanup;

    if (g_pMultiLanguage2)
        hr = THR(g_pMultiLanguage2->GetCharsetInfo((LPTSTR)lpszCharset, pMimeCSetInfo));
    else
        hr = THR(g_pMultiLanguage->GetCharsetInfo((LPTSTR)lpszCharset, pMimeCSetInfo));
    if (hr)
        goto Cleanup;

Cleanup:
    return hr;
}
#endif // !NO_MULTILANG

//+-----------------------------------------------------------------------
//
//  Function:   GetCodePageFromMlangString
//
//  Synopsis:   Return a codepage id from an mlang codepage string.
//
//------------------------------------------------------------------------

HRESULT
GetCodePageFromMlangString(LPCTSTR pszMlangString, CODEPAGE* pCodePage)
{
#ifdef NO_MULTILANG
    return E_NOTIMPL;
#else
    HRESULT hr;
    MIMECSETINFO mimeCharsetInfo;

    hr = QuickMimeGetCharsetInfo(pszMlangString, &mimeCharsetInfo);
    if (hr)
    {
        hr = E_INVALIDARG;
        *pCodePage = CP_UNDEFINED;
    }
    else
    {
        *pCodePage = mimeCharsetInfo.uiInternetEncoding;
    }

    return hr;
#endif
}

//+-----------------------------------------------------------------------
//
//  Function:   GetMlangStringFromCodePage
//
//  Synopsis:   Return an mlang codepage string from a codepage id.
//
//------------------------------------------------------------------------

HRESULT
GetMlangStringFromCodePage(CODEPAGE codepage, LPTSTR pMlangStringRet,
                           size_t cchMlangString)
{
    HRESULT    hr;
    MIMECPINFO mimeCpInfo;
    TCHAR*     pCpString = _T("<undefined-cp>");
    size_t     cchCopy;

    Assert(codepage != CP_ACP);

    hr = QuickMimeGetCodePageInfo(codepage, &mimeCpInfo);
    if (hr == S_OK)
    {
        pCpString = mimeCpInfo.wszWebCharset;
    }

    cchCopy = min(cchMlangString-1, _tcslen(pCpString));
    _tcsncpy(pMlangStringRet, pCpString, cchCopy);
    pMlangStringRet[cchCopy] = 0;

    return hr;
}

#ifndef NO_MULTILANG

//+-----------------------------------------------------------------------
//
//  Function:   CheckEncodingMenu
//
//  Synopsis:   Given a mime charset menu, check the specified codepage.
//
//------------------------------------------------------------------------
void
CheckEncodingMenu(CODEPAGE codepage, HMENU hMenu, BOOL fAutoMode)
{
    ULONG i;
    

    IGNORE_HR(EnsureCodePageInfo(FALSE));

    Assert(codepage != CP_ACP);

    if (1 < g_ccpInfo)
    {
        MIMECPINFO mimeCpInfo;

        LOCK_GLOBALS;
        
        IGNORE_HR(QuickMimeGetCodePageInfo( codepage, &mimeCpInfo ));
        
        // find index to our cache
        for(i = 0; i < CCachedCPInfo::GetCcp() && codepage != CCachedCPInfo::GetCodePage(i); i++)
            ;

        if (i >= CCachedCPInfo::GetCcp())
        {
            // the codepage is not on the tier1 menu
            // put a button to the tier2 menu if possible
            LOCK_GLOBALS;
            for (i = 0;  i < g_ccpInfo;  i++)
            {
                if (g_pcpInfo[i].uiCodePage == codepage)
                {
                    // IDM_MIMECSET__LAST__-1 is now temporarily
                    // assigned to CP_AUTO
                    CheckMenuRadioItem(hMenu, 
                                       IDM_MIMECSET__FIRST__, IDM_MIMECSET__LAST__-1, 
                                       i + IDM_MIMECSET__FIRST__,
                                       MF_BYCOMMAND);
                    break;
                }
            }
        }
        else
        {
#ifndef UNIX // Unix doesn't support AutoDetect
            UINT uiPos = CCachedCPInfo::GetCcp() + 2 - 1 ; 
#else
            UINT uiPos = CCachedCPInfo::GetCcp() - 1 ; 
#endif
            ULONG uMenuId = CCachedCPInfo::GetMenuIdx(i);
            UINT uidItem;

            // the menuid is -1 if this is the one not available for browser
            // we have to put a radiobutton by position for that case.
            // 
            uMenuId = uMenuId == (ULONG)-1 ? uMenuId : uMenuId + IDM_MIMECSET__FIRST__;
           
#ifndef UNIX // Unix uiPos == 0 isn't AutoDetect 
            while (uiPos > 0)
#else
            while (uiPos >= 0)
#endif
            {
                // On Win95, GetMenuItemID can't return (ULONG)-1
                // so we use lower word only
                uidItem = GetMenuItemID(hMenu, uiPos);
                if ( LOWORD(uidItem) == LOWORD(uMenuId) )
                {
                    CheckMenuRadioItem(
                        hMenu, 
#ifndef UNIX // No AutoDetect for UNIX
                        1, CCachedCPInfo::GetCcp() + 2 - 1, // +2 for cp_auto -1 for 0 based
#else
                        0, CCachedCPInfo::GetCcp() - 1, // +2 for cp_auto -1 for 0 based
#endif
                        uiPos,
                        MF_BYPOSITION );

                    break; 
                }
                uiPos--;
            }
        }
    }
#ifndef UNIX // Unix doesn't support AutoDetect
    // then lastly, we put check mark for the auto detect mode
    // the AutoDetect item is always at pos 0
    CheckMenuItem(hMenu, 0, fAutoMode?
                            (MF_BYPOSITION|MF_CHECKED) :
                            (MF_BYPOSITION|MF_UNCHECKED) );
#endif
    
}

//+-----------------------------------------------------------------------
//
//  Function:   CheckFontMenu
//
//  Synopsis:   Given a font menu, check the specified font size.
//
//------------------------------------------------------------------------
void
CheckFontMenu(short sFontSize, HMENU hMenu)
{
    CheckMenuRadioItem(hMenu, IDM_BASELINEFONT1, IDM_BASELINEFONT5, 
                              sFontSize-BASELINEFONTMIN+IDM_BASELINEFONT1, MF_BYCOMMAND);
}

//+-----------------------------------------------------------------------
//
//  Function:   CheckMenuCharSet
//
//  Synopsis:   Assuming hMenu has both encoding/font size
//              this puts checks marks on it based on the given
//              codepage and font size
//
//------------------------------------------------------------------------
void 
CheckMenuMimeCharSet(CODEPAGE codepage, short sFontSize, HMENU hMenuLang, BOOL fAutoMode)
{
    CheckEncodingMenu(codepage, hMenuLang, fAutoMode);
    CheckFontMenu(sFontSize, hMenuLang);
}

#endif
//+-----------------------------------------------------------------------
//
//  Function:   CreateMimeCSetMenu
//
//  Synopsis:   Create the mime charset menu.  Can return an empty menu.
//
//------------------------------------------------------------------------
HMENU
CreateMimeCSetMenu(OPTIONSETTINGS *pOS, CODEPAGE cp)
{
    BOOL fBroken = FALSE;
    
    HMENU hMenu = CreatePopupMenu();

    IGNORE_HR(EnsureCodePageInfo(TRUE));

    if (hMenu && 0 < g_ccpInfo)
    {
        MIMECPINFO mimeCpInfo;

        Assert(g_pcpInfo);
        LOCK_GLOBALS;
       
#ifndef UNIX // Unix doesn't have Auto Detect 
        // always add the 'Auto Detect' entry
        // get the description from mlang
        // the id needs to be special - IDM_MIMECSET__LAST__
        // is fairly unique because there are 90 codepages to
        // have between FIRST and LAST
        IGNORE_HR(QuickMimeGetCodePageInfo( CP_AUTO, &mimeCpInfo ));
        AppendMenu(hMenu, MF_ENABLED, IDM_MIMECSET__LAST__, mimeCpInfo.wszDescription);
        
        // Also add a separator
        AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
        // add the tier 1 entries to the first
        // level popup menu
#endif // UNIX

        ULONG i;
        ULONG iMenuIdx;

        CCachedCPInfo::InitCpCache(pOS, g_pcpInfo, g_ccpInfo);
        
        // Cache an autodetect codepage if it's available
        // for the given codepage
        // note that we have "AutoSelect"(CP_AUTO) for character set 
        // detection across whole codepages, and at the same time
        // "AutoDetect" within A codepage, such as JPAUTO (50932)
        // CP_AUTO is added to the menu as toggle item, so
        // we'll ignore it at SaveCodePage(), should it come down here
        //
        IGNORE_HR(QuickMimeGetCodePageInfo( cp, &mimeCpInfo ));
        UINT cpAuto = GetAutoDetectCp(&mimeCpInfo);
        
        if (cpAuto != CP_UNDEFINED)
            CCachedCPInfo::SaveCodePage(cpAuto, g_pcpInfo, g_ccpInfo);

        CCachedCPInfo::SaveCodePage(cp, g_pcpInfo, g_ccpInfo);

        for(i = 0; i < CCachedCPInfo::GetCcp(); i++)
        {
            iMenuIdx = CCachedCPInfo::GetMenuIdx(i);

            if (iMenuIdx == (ULONG)-1)
            {
                // the codepage is either not supported or 
                // not for browser. We need to add it to the
                // tier1 menu and make it disabled
                if (SUCCEEDED(QuickMimeGetCodePageInfo(CCachedCPInfo::GetCodePage(i), &mimeCpInfo)))
                {
                    AppendMenu(hMenu, MF_GRAYED, iMenuIdx, // == -1
                               mimeCpInfo.wszDescription);
                }
            }
            else
            {
                AppendMenu(hMenu, MF_ENABLED, iMenuIdx+IDM_MIMECSET__FIRST__,
                    g_pcpInfo[iMenuIdx].wszDescription);

                // mark the cp entry so we can skip it for submenu
                // this assumes we'd never use the MSB for MIMECONTF
                g_pcpInfo[iMenuIdx].dwFlags |= 0x80000000;
            }
        } 
        
        // add the submenu for the rest of encodings
        HMENU hSubMenu = CreatePopupMenu();
        UINT  uiLastFamilyCp = 0;
        if (hSubMenu)
        {
            // calculate the max # of menuitem we can show in one monitor
            unsigned int iMenuY, iScrY, iBreakItem, iItemAdded = 0;
            NONCLIENTMETRICSA ncma;
            HRESULT hr;
            
            if (g_dwPlatformVersion < 0x050000)
            {
                // use systemparametersinfoA so it won't fail on w95
                // BUGBUG: shlwapi W wrap should support this
                ncma.cbSize = sizeof(ncma);
                SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, sizeof(ncma), &ncma, FALSE);
                iMenuY = ncma.lfMenuFont.lfHeight > 0 ? 
                         ncma.lfMenuFont.lfHeight: -ncma.lfMenuFont.lfHeight;
                iScrY  = GetSystemMetrics(SM_CYSCREEN);

                if ( iScrY > 0 && iMenuY > 0) 
                    iBreakItem = (iScrY + iMenuY - 1) / iMenuY; // round up
                else
                    iBreakItem = BREAK_ITEM; // a fallback
            }
            else // NT5 has nice auto menu scroll so let's not break
                iBreakItem = (UINT)-1;
                
            for (i = 0;  i < g_ccpInfo;  i++)
            {
                // skip codepages that are on teir1 menu
                // skip also Auto Select
                if (!(g_pcpInfo[i].dwFlags & 0x80000000)
                    && g_pcpInfo[i].uiCodePage != CP_AUTO
#ifdef UNIX // filter out Thai and Vietnamese
                    && g_pcpInfo[i].uiCodePage != CP_THAI
                    && g_pcpInfo[i].uiCodePage != CP_1258 // Vietnamese
                    && g_pcpInfo[i].uiCodePage != CP_1257 // Baltic
                    && g_pcpInfo[i].uiCodePage != CP_1256 // Arabic
#endif
                    )
                {
                    // if the codepage is not yet installed, 
                    // we show the primary codepage only
                    //
                    // we only see if the primary cp is valid, 
                    // then make the entire family shown without checking 
                    // if they're valid. this is for the case that just
                    // part of langpack (nls/font) is installed.
                    // this may need some more tweak after usability check

                    // Actually now we've decided to hide the menu items if
                    // the encodings are neither valid or installable
                    //
                    if (g_pMultiLanguage2 &&
                        S_OK != g_pMultiLanguage2->IsCodePageInstallable(g_pcpInfo[i].uiCodePage))
                        continue;
                    
                    // we need to call the slow version that invokes mlang
                    // because quick version wouldn't set valid flags.
                    hr = SlowMimeGetCodePageInfo(
                             g_pcpInfo[i].uiFamilyCodePage, 
                             &mimeCpInfo);
                    
                    if ( hr == S_OK 
                      && (mimeCpInfo.dwFlags & MIMECONTF_VALID)
                      ||  IsPrimaryCodePage(g_pcpInfo+i))
                    {
                        UINT uiFlags = MF_ENABLED;

                        if (uiLastFamilyCp > 0 
                        && uiLastFamilyCp != g_pcpInfo[i].uiFamilyCodePage)
                        {
                            // add separater between different family unless
                            // we will be adding the menu bar break
                            if(iItemAdded < iBreakItem || fBroken)
                            {
                                AppendMenu(hSubMenu, MF_SEPARATOR, 0, 0);
                                iItemAdded++;
                            }
                            else
                            {
                                // This menu gets really long. Let's break it so all
                                // fit on the screen
                                uiFlags |= MF_MENUBARBREAK;
                                fBroken = TRUE;
                            }
                        }
                        
                        TCHAR wszDescription[ARRAY_SIZE(g_pcpInfo[0].wszDescription)];
                        _tcscpy(wszDescription, g_pcpInfo[i].wszDescription);

                        if ( IsPrimaryCodePage(g_pcpInfo+i) 
                         && !(g_pcpInfo[i].dwFlags & MIMECONTF_VALID))
                        {
                            // this codepage will be added as a place holder
                            // let's rip out the detail to make it general
                            LPTSTR psz = StrChr(wszDescription, TEXT('('));
                            if (psz)
                                *psz = TEXT('\0');        
                        }
                        
                        AppendMenu(hSubMenu, 
                                   uiFlags, 
                                   i+IDM_MIMECSET__FIRST__,
                                   wszDescription);
                        iItemAdded++;

                        // save the family of added codepage
                        uiLastFamilyCp = g_pcpInfo[i].uiFamilyCodePage;
                    }
                }
                else
                    g_pcpInfo[i].dwFlags &= 0x7FFFFFFF;

            }

            // add this submenu to the last of tier1 menu
            if (!g_szMore[0])
            {
                Verify(LoadString( GetResourceHInst(), 
                                   RES_STRING_ENCODING_MORE,
                                   g_szMore,
                                   ARRAY_SIZE(g_szMore)));
            }
            if (GetMenuItemCount(hSubMenu) > 0)
            {
                AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hSubMenu, g_szMore);
            }
            else
            {
                DestroyMenu(hSubMenu);
            }
        }
    }
    return hMenu;
}
#ifndef NO_MULTILANG

//+-----------------------------------------------------------------------
//
//  Function:   CreateDocDirMenu
//
//  Synopsis:   Add document direction menu into mime charset menu.
//
//------------------------------------------------------------------------

HMENU
CreateDocDirMenu(BOOL fDocRTL, HMENU hMenuTarget)
{
    HMENU hMenu;
    hMenu = TFAIL_NOTRACE(0, LoadMenu(
                          GetResourceHInst(),
                          MAKEINTRESOURCE(IDR_HTMLFORM_DOCDIR)));

    if(hMenu != NULL)
    {

        int nItem = GetMenuItemCount(hMenu);

        if (0 < nItem)
        {
            if(hMenuTarget)
            {
                // append the menu at the bottom
                InsertMenu(hMenuTarget, 0xFFFFFFFF, MF_SEPARATOR | MF_BYPOSITION, 0, 0);
            }

            int nCheckItem= 0;

            for (int i = 0; i < nItem; i++)
            {
                UINT uiID;
                UINT uChecked = MF_BYCOMMAND | MF_ENABLED;
                TCHAR szBuf[MAX_MIMECP_NAME];

                uiID = GetMenuItemID(hMenu, i);
                GetMenuString(hMenu, i, szBuf, ARRAY_SIZE(szBuf), MF_BYPOSITION);

                if((uiID == IDM_DIRRTL) ^ (!fDocRTL))  
                    nCheckItem = i;
     
                if(hMenuTarget)
                {
                    InsertMenu(hMenuTarget, 0xFFFFFFFF, uChecked | MF_STRING, uiID, szBuf);
                }
            }
            if(hMenuTarget)
            {
                CheckMenuRadioItem(hMenuTarget, IDM_DIRLTR, IDM_DIRRTL, 
                                   IDM_DIRLTR + nCheckItem, MF_BYCOMMAND);
            }
            else
            {
                CheckMenuRadioItem(hMenu, IDM_DIRLTR, IDM_DIRRTL, 
                                   IDM_DIRLTR + nCheckItem, MF_BYCOMMAND);
            }
        }
    }
    if(hMenuTarget)
    {
        if(hMenu)
            DestroyMenu(hMenu);
        return NULL;
    }
    else
    {
        return hMenu;
    }
}

//+-----------------------------------------------------------------------
//
//  Function:   AddFontSizeMenu
//
//  Synopsis:   Add font size menu into mime charset menu.
//
//------------------------------------------------------------------------


void
AddFontSizeMenu(HMENU hMenu)
{
    if (NULL != hMenu)
    {
        HMENU hMenuRes = TFAIL_NOTRACE(0, LoadMenu(
                GetResourceHInst(),
                MAKEINTRESOURCE(IDR_HTMLFORM_MENURUN)));

        if (NULL != hMenuRes)
        {
            HMENU hMenuView = GetSubMenu(hMenuRes, 1);
            HMENU hMenuFont = GetSubMenu(hMenuView, 3);
            int nItem = GetMenuItemCount(hMenuFont);

            if (0 < nItem)
            {
                for (int i = nItem - 1; i >= 0; i--)
                {
                    UINT uiID;
                    TCHAR szBuf[MAX_MIMECP_NAME];

                    uiID = GetMenuItemID(hMenuFont, i);
                    GetMenuString(hMenuFont, i, szBuf, ARRAY_SIZE(szBuf), MF_BYPOSITION);
                    InsertMenu(hMenu, 0, MF_STRING | MF_BYPOSITION, uiID, szBuf);
                }
            }
            DestroyMenu(hMenuRes);
        }
    }
}

HMENU
CreateFontSizeMenu(void)
{
    HMENU hMenuFontSize = CreatePopupMenu();
    
    AddFontSizeMenu(hMenuFontSize);
    return hMenuFontSize;
}

//+-----------------------------------------------------------------------
//
//  Function:   ShowMimeCSetMenu
//
//  Synopsis:   Display the mime charset menu.
//
//------------------------------------------------------------------------

HRESULT
ShowMimeCSetMenu(OPTIONSETTINGS *pOS, int * pnIdm, CODEPAGE codepage, LPARAM lParam, BOOL fDocRTL, BOOL fAutoMode)
{
    HMENU hMenuEncoding = NULL;
    HRESULT hr = E_FAIL;

    Assert(NULL != pnIdm);

    hMenuEncoding = CreateMimeCSetMenu(pOS, codepage);
    if (NULL != hMenuEncoding)
    {
        POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

        CheckEncodingMenu(codepage, hMenuEncoding, fAutoMode);

        GetOrAppendDocDirMenu(codepage, fDocRTL, hMenuEncoding);

        hr = FormsTrackPopupMenu(hMenuEncoding, TPM_LEFTALIGN, pt.x, pt.y,
                            NULL, pnIdm);
        DestroyMenu(hMenuEncoding);
    }

    return hr;
}

HRESULT
ShowFontSizeMenu(int * pnIdm, short sFontSize, LPARAM lParam)
{
    HMENU hMenuFont = NULL;
    HRESULT hr = E_FAIL;

    Assert(NULL != pnIdm);

    hMenuFont = CreateFontSizeMenu();
    if (NULL != hMenuFont)
    {
        POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

        CheckFontMenu(sFontSize, hMenuFont);
        hr = FormsTrackPopupMenu(hMenuFont, TPM_LEFTALIGN, pt.x, pt.y,
                                 NULL, pnIdm);
        DestroyMenu(hMenuFont);
    }

    return hr;
}

//+-----------------------------------------------------------------------
//
//  Function:   GetEncodingMenu
//
//  Synopsis:   Return the mime charset menu handle.
//
//------------------------------------------------------------------------
HMENU
GetEncodingMenu(OPTIONSETTINGS *pOS, CODEPAGE codepage, BOOL fDocRTL, BOOL fAutoMode)
{
    HMENU hMenuEncoding = CreateMimeCSetMenu(pOS, codepage);
    if (NULL != hMenuEncoding)
    {
        CheckEncodingMenu(codepage, hMenuEncoding, fAutoMode);

        GetOrAppendDocDirMenu(codepage, fDocRTL, hMenuEncoding);
    }
    
    return hMenuEncoding;
}

//+-----------------------------------------------------------------------
//
//  Function:   GetOrAppendDocDirMenu
//
//  Synopsis:   Return the document direction menu handle.
//              or append it to a given menu handle.
//
//------------------------------------------------------------------------
HMENU
GetOrAppendDocDirMenu(CODEPAGE codepage, BOOL fDocRTL, HMENU hMenuTarget)
{
    HMENU hMenuDocDir = NULL;

    // put up the document dir menu only if it is likely to have
    // right-to-left
    if(g_fBidiSupport || IsRTLCodepage(codepage))
    {
        hMenuDocDir = CreateDocDirMenu(fDocRTL, hMenuTarget);
    }
    return  hMenuDocDir;
}

//+-----------------------------------------------------------------------
//
//  Function:   GetFontSizeMenu
//
//  Synopsis:   Retrun the FontSize menu handle
//
//------------------------------------------------------------------------
HMENU
GetFontSizeMenu(short sFontSize)
{
    HMENU hMenuFontSize = CreateFontSizeMenu();
    if (NULL != hMenuFontSize)
    {
        CheckFontMenu(sFontSize, hMenuFontSize);
    }
    
    return hMenuFontSize;
}
#endif // ndef NO_MULTILANG
//+-----------------------------------------------------------------------
//
//  Function:   GetCodePageFromMenuID
//
//  Synopsis:   Given a menu id return the associated codepage.
//
//------------------------------------------------------------------------

CODEPAGE
GetCodePageFromMenuID(int nIdm)
{
    UINT idx = nIdm - IDM_MIMECSET__FIRST__;

    IGNORE_HR(EnsureCodePageInfo(FALSE));

    LOCK_GLOBALS;

    if (NULL != g_pcpInfo && idx < g_ccpInfo)
        return g_pcpInfo[idx].uiCodePage;
    return CP_UNDEFINED;
}

#ifndef NO_MULTILANG
//+-----------------------------------------------------------------------
//
//  Function:   WindowsCodePageFromCodePage
//
//  Synopsis:   Return a Windows codepage from an mlang CODEPAGE
//
//------------------------------------------------------------------------

UINT
WindowsCodePageFromCodePage( CODEPAGE cp )
{
    HRESULT hr;
    
    Assert( cp != CP_UNDEFINED );

    if (cp == CP_AUTO)
    {
        // cp 50001 (CP_AUTO)is designated to cross-language detection,
        // it really should not come in here but we'd return the default 
        // code page.
        return g_cpDefault;
    }

    if (   cp == CP_1252
        || cp == CP_ISO_8859_1)
    {
        return 1252; // short-circuit most common case
    }
    else if (IsStraightToUnicodeCodePage(cp))
    {
        return CP_UCS_2; // BUGBUG (cthrash) Should be NATIVE_UNICODE_CODEPAGE?
    }
    else if (g_cpInfoInitialized)
    {
        // Get the codepage from mlang
        hr = THR(EnsureCodePageInfo(FALSE));
        if (hr)
            goto Cleanup;
        
        {
            LOCK_GLOBALS;

            for (UINT n = 0; n < g_ccpInfo; n++)
            {
                if (cp == g_pcpInfo[n].uiCodePage)
                    return g_pcpInfo[n].uiFamilyCodePage;
            }
        }
    }

    // BUGBUG (cthrash) There's a chance that this codepage is a 'hidden'
    // codepage and that MLANG may actually know the family codepage.
    // So we ask them again, only differently.

    hr = THR(EnsureMultiLanguage());
    if (hr)
        goto Cleanup;

    {
        UINT uiFamilyCodePage;

        if (g_pMultiLanguage2)
            hr = THR(g_pMultiLanguage2->GetFamilyCodePage( cp, &uiFamilyCodePage ));
        else
            hr = THR(g_pMultiLanguage->GetFamilyCodePage( cp, &uiFamilyCodePage ));
        if (hr)
            goto Cleanup;
        else
            return uiFamilyCodePage;
    }

Cleanup:
    
    return GetACP();
}

//+-----------------------------------------------------------------------
//
//  Function:   WindowsCharsetFromCodePage
//
//  Synopsis:   Return a Windows charset from an mlang CODEPAGE id
//
//------------------------------------------------------------------------

BYTE
WindowsCharsetFromCodePage( CODEPAGE cp )
{
    HRESULT    hr;
    MIMECPINFO mimeCpInfo;

    if (cp == CP_ACP)
    {
        return DEFAULT_CHARSET;
    }

    hr = QuickMimeGetCodePageInfo(cp, &mimeCpInfo);
    if (hr)
    {
        return DEFAULT_CHARSET;
    }
    else
    {
        return mimeCpInfo.bGDICharset;
    }
}
#endif // !NO_MULTILANG

//+-----------------------------------------------------------------------
//
//  Function:   DefaultCodePageFromCharSet
//
//  Synopsis:   Return a Windows codepage from a Windows font charset
//
//------------------------------------------------------------------------

UINT
DefaultCodePageFromCharSet(
    BYTE bCharSet,
    CODEPAGE cp,
    LCID lcid )
{
    HRESULT hr;
    UINT    n;
    static  BYTE bCharSetPrev = DEFAULT_CHARSET;
    static  CODEPAGE cpPrev = CP_UNDEFINED;
    static  CODEPAGE cpDefaultPrev = CP_UNDEFINED;
    CODEPAGE cpDefault;

#ifdef NO_MULTILANG
    return GetACP();
#else
    if (   DEFAULT_CHARSET == bCharSet
        || (   bCharSet == ANSI_CHARSET
            && (   cp == CP_UCS_2
                   || cp == CP_UTF_8
               )
           )
       )
    {
        return g_cpDefault;  // Don't populate the statics.
    }
    else if (bCharSet == bCharSetPrev && cpPrev == cp)
    {
        // Here's our gamble -- We have a high likelyhood of calling with
        // the same arguments over and over.  Short-circuit this case.

        return cpDefaultPrev;
    }

    // First pick the *TRUE* codepage
    if (lcid)
    {
        char pszCodePage[5];
        
        GetLocaleInfoA( lcid, LOCALE_IDEFAULTANSICODEPAGE,
                        pszCodePage, ARRAY_SIZE(pszCodePage) );

        cp = atoi(pszCodePage);
    }

    // First check our internal lookup table in case we can avoid mlang
    for (n = 0; n < ARRAY_SIZE(s_aryCpMap); ++n)
    {
        if (cp == s_aryCpMap[n].cp && bCharSet == s_aryCpMap[n].bGDICharset)
        {
            cpDefault = cp;
            goto Cleanup;
        }
    }

    hr = THR(EnsureCodePageInfo(FALSE));
    if (hr)
    {
        cpDefault = WindowsCodePageFromCodePage(cp);
        goto Cleanup;
    }

    {
        LOCK_GLOBALS;
        
        // First see if we find an exact match for both cp and bCharset.
        for (n = 0; n < g_ccpInfo; n++)
        {
            if (cp == g_pcpInfo[n].uiCodePage &&
                bCharSet == g_pcpInfo[n].bGDICharset)
            {
                cpDefault = g_pcpInfo[n].uiFamilyCodePage;
                goto Cleanup;
            }
        }

        // Settle for the first match of bCharset.

        for (n = 0; n < g_ccpInfo; n++)
        {
            if (bCharSet == g_pcpInfo[n].bGDICharset)
            {
                cpDefault = g_pcpInfo[n].uiFamilyCodePage;
                goto Cleanup;
            }
        }

        cpDefault = g_cpDefault;
    }

Cleanup:

    bCharSetPrev = bCharSet;
    cpPrev = cp;
    cpDefaultPrev = cpDefault;

    return cpDefault;
#endif // !NO_MULTILANG
}

//+-----------------------------------------------------------------------
//
//  Function:   DefaultFontInfoFromCodePage
//
//  Synopsis:   Fills a LOGFONT structure with appropriate information for
//              a 'default' font for a given codepage.
//
//------------------------------------------------------------------------

HRESULT
DefaultFontInfoFromCodePage( CODEPAGE cp, LOGFONT * lplf )
{
    MIMECPINFO mimeCpInfo;
    HFONT      hfont = (HFONT)GetStockObject( DEFAULT_GUI_FONT );

    // The strategy is thus: If we ask for a stock font in the default
    // (CP_ACP) codepage, return the logfont information of that font.
    // If we don't, replace key pieces of information.  Note we could
    // do better -- we could get the right lfPitchAndFamily.

#ifndef NO_MULTILANG
    if (!hfont)
    {
        CPINFO cpinfo;

        GetCPInfo( WindowsCodePageFromCodePage(cp), &cpinfo );

        hfont = (HFONT)((cpinfo.MaxCharSize == 1)
                        ? GetStockObject( ANSI_VAR_FONT )
                        : GetStockObject( SYSTEM_FONT ));

        AssertSz( hfont, "We'd better have a font now.");
    }
#endif

    GetObject( hfont, sizeof(LOGFONT), (LPVOID)lplf );

#ifndef NO_MULTILANG
    if (   cp != CP_ACP
        && cp != g_cpDefault
        && (   cp != CP_ISO_8859_1
            || g_cpDefault != CP_1252))
    {
        IGNORE_HR(QuickMimeGetCodePageInfo( cp, &mimeCpInfo ));

        _tcscpy(lplf->lfFaceName, mimeCpInfo.wszProportionalFont);
        lplf->lfCharSet = mimeCpInfo.bGDICharset;
    }
    else
    {
        // NB (cthrash) On both simplified and traditional Chinese systems,
        // we get a bogus lfCharSet value when we ask for the DEFAULT_GUI_FONT.
        // This later confuses CCcs::MakeFont -- so override.

        if (cp == 950)
        {
            lplf->lfCharSet = CHINESEBIG5_CHARSET;
        }
        else if (cp == 936)
        {
            lplf->lfCharSet = GB2312_CHARSET;
        }
    }
#endif // !NO_MULTILANG

   return S_OK;
}

//+-----------------------------------------------------------------------
//
//  Function:   CodePageFromString
//
//  Synopsis:   Map a charset to a forms3 CODEPAGE enum.  Searches in the
//              argument a string of the form charset=xxx.  This is used
//              by the META tag handler in the HTML preparser.
//
//              If fLookForWordCharset is TRUE, pch is presumed to be in
//              the form of charset=XXX.  Otherwise the string is
//              expected to contain just the charset string.
//
//------------------------------------------------------------------------

inline BOOL
IsWhite(TCHAR ch)
{
    return ch == TEXT(' ') || InRange(ch, TEXT('\t'), TEXT('\r'));
}

CODEPAGE
CodePageFromString( TCHAR * pch, BOOL fLookForWordCharset )
{
    CODEPAGE cp = CP_UNDEFINED;

    while (pch && *pch)
    {
        for (;IsWhite(*pch);pch++);

        if (!fLookForWordCharset || (_tcslen(pch) >= 7 &&
            _tcsnicmp(pch, 7, _T("charset"), 7) == 0))
        {
            if (fLookForWordCharset)
            {
                pch = _tcschr(pch, L'=');
                pch = pch ? ++pch : NULL;
            }

            if (pch)
            {
                for (;IsWhite(*pch);pch++);

                if (*pch)
                {
                    TCHAR *pchEnd, chEnd;

                    for (pchEnd = pch;
                         *pchEnd && !(*pchEnd == L';' || IsWhite(*pchEnd));
                         pchEnd++);

                    chEnd = *pchEnd;
                    *pchEnd = L'\0';

                    cp = CodePageFromAlias( pch );

                    *pchEnd = chEnd;

                    break;
                }
            }
        }

        if (pch)
        {
            pch = _tcschr( pch, L';');
            if (pch) pch++;
        }

    }

    return cp;
}

LCID
LCIDFromString( TCHAR * pchArg )
{
    LCID lcid = 0;
    HRESULT hr;

    if (!pchArg)
        goto Cleanup;

    hr = THR( EnsureMultiLanguage() );
    if (hr || !g_pMultiLanguage)
        goto Cleanup;

    hr = THR( g_pMultiLanguage->GetLcidFromRfc1766(&lcid, pchArg) );
    if (hr)
        lcid = 0;

Cleanup:

    return lcid;
}

#ifndef NO_MULTILANG
// **************************************************************************
// NB (cthrash) From RichEdit (_uwrap/unicwrap) start {

/*
 *  CharSetIndexFromChar (ch)
 *
 *  @mfunc
 *      returns index into CharSet/CodePage table rgCpgChar corresponding
 *      to the Unicode character ch provided such an assignment is
 *      reasonably unambiguous, that is, the currently assigned Unicode
 *      characters in various ranges have Windows code-page equivalents.
 *      Ambiguous or impossible assignments return UNKNOWN_INDEX, which
 *      means that the character can only be represented by Unicode in this
 *      simple model.  Note that both UNKNOWN_INDEX and HAN_INDEX are negative
 *      values, i.e., they imply further processing to figure out what (if
 *      any) charset index to use.  Other indices may also require run
 *      processing, such as the blank in BiDi text.  We need to mark our
 *      right-to-left runs with an Arabic or Hebrew char set, while we mark
 *      left-to-right runs with a left-to-right char set.
 *
 *  @rdesc
 *      returns CharSet index
 */
CHARSETINDEX
CharSetIndexFromChar( TCHAR ch )        // Unicode character to examine
{
    if(ch < 256)
        return ANSI_INDEX;

    if(ch < 0x700)
    {
        if(ch >= 0x600)
            return ARABIC_INDEX;

        if(ch > 0x590)
            return HEBREW_INDEX;

        if(ch < 0x500)
        {
            if(ch >= 0x400)
                return RUSSIAN_INDEX;

            if(ch >= 0x370)
                return GREEK_INDEX;
        }
    }
    else if (ch < 0xAC00)
    {
        if(ch >= 0x3400)                // CJK Ideographs
            return HAN_INDEX;

        if(ch > 0x3040 && ch < 0x3100)  // Katakana and Hiragana
            return SHIFTJIS_INDEX;

        if(ch < 0xe80 && ch >= 0xe00)   // Thai
            return THAI_INDEX;
    }
    else if (ch < 0xD800)
        return HANGUL_INDEX;

    else if (ch > 0xff00)
    {
        if(ch < 0xff65)                 // Fullwidth ASCII and halfwidth
            return HAN_INDEX;           //  CJK punctuation

        if(ch < 0xffA0)                 // Halfwidth Katakana
            return SHIFTJIS_INDEX;

        if(ch < 0xffe0)                 // Halfwidth Jamo
            return HANGUL_INDEX;

        if(ch < 0xffef)                 // Fullwidth punctuation and currency
            return HAN_INDEX;           //  signs; halfwidth forms, arrows
    }                                   //  and shapes

    return UNKNOWN_INDEX;
}

/*
 *  CheckDBCInUnicodeStr (ptext)
 *
 *  @mfunc
 *      returns TRUE if there is a DBC in the Unicode buffer
 *
 *  @rdesc
 *      returns TRUE | FALSE
 */
BOOL CheckDBCInUnicodeStr(TCHAR *ptext)
{
    CHARSETINDEX iCharSetIndex;

    if ( ptext )
    {
        while (*ptext)
        {
            iCharSetIndex = CharSetIndexFromChar( *ptext++ );

            if ( iCharSetIndex == HAN_INDEX ||
                 iCharSetIndex == SHIFTJIS_INDEX ||
                 iCharSetIndex == HANGUL_INDEX )
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}
#endif // !NO_MULTILANG

/*
 *  GetLocaleLCID ()
 *
 *  @mfunc      Maps an LCID for thread to a Code Page
 *
 *  @rdesc      returns Code Page
 */
LCID GetLocaleLCID()
{
#if !defined(MACPORT) && !defined(WIN16) && !defined(WINCE)
    return GetThreadLocale();
#else
    return GetSystemDefaultLCID();
#endif
}


/*
 *  GetLocaleCodePage ()
 *
 *  @mfunc      Maps an LCID for thread to a Code Page
 *
 *  @rdesc      returns Code Page
 */
UINT GetLocaleCodePage()
{
#if !defined(MACPORT) && !defined(WIN16) && !defined(WINCE)
    return ConvertLanguageIDtoCodePage(GetThreadLocale());
#else
    return ConvertLanguageIDtoCodePage(GetSystemDefaultLCID());
#endif
}

/*
 *  GetKeyboardLCID ()
 *
 *  @mfunc      Gets LCID for keyboard active on current thread
 *
 *  @rdesc      returns Code Page
 */
LCID GetKeyboardLCID()
{
#if !defined(MACPORT) && !defined(WIN16) && !defined(WINCE)
#ifndef NO_IME
    extern HRESULT GetKeyboardLCIDDIMM(LCID *); // imm32.cxx
    LCID lcid;
    if (SUCCEEDED(GetKeyboardLCIDDIMM(&lcid)))
        return lcid;
#endif
    return (WORD)(DWORD_PTR)GetKeyboardLayout(0 /*idThread*/);
#else
    return GetUserDefaultLCID();
#endif
}

/*
 *  IsFELCID(lcid)
 *
 *  @mfunc
 *      Returns TRUE iff lcid is for a FE country.
 *
 *  @rdesc
 *      TRUE iff lcid is for a FE country.
 *
 */
BOOL IsFELCID(LCID lcid)
{
    switch(PRIMARYLANGID(LANGIDFROMLCID(lcid)))
    {
        case LANG_CHINESE:
        case LANG_JAPANESE:
        case LANG_KOREAN:
            return TRUE;
    }

    return FALSE;
}

/*
 *  IsFECharset(bCharSet)
 *
 *  @mfunc
 *      Returns TRUE iff charset may be for a FE country.
 *
 *  @rdesc
 *      TRUE iff charset may be for a FE country.
 *
 */
BOOL IsFECharset(BYTE bCharSet)
{
    switch(bCharSet)
    {
        case CHINESEBIG5_CHARSET:
        case SHIFTJIS_CHARSET:
        case HANGEUL_CHARSET:
#if !defined(WIN16) && !defined(WINCE) && !defined(UNIX)
        case JOHAB_CHARSET:
        case GB2312_CHARSET:
#endif // !WIN16 && !WINCE && !UNIX
            return TRUE;
    }

    return FALSE;
}

/*
 *  IsRtlLCID(lcid)
 *
 *  @mfunc
 *      Returns TRUE iff lcid is for a RTL script.
 *
 *  @rdesc
 *      TRUE iff lcid is for a RTL script.
 *
 */
BOOL IsRtlLCID(LCID lcid)
{
    return (IsRTLLang(PRIMARYLANGID(LANGIDFROMLCID(lcid))));
}

/*
 *  IsRTLLang(lcid)
 *
 *  @mfunc
 *      Returns TRUE iff lang is for a RTL script.
 *
 *  @rdesc
 *      TRUE iff lang is for a RTL script.
 *
 */
BOOL IsRTLLang(LANGID lang)
{
    switch (lang)
    {
        case LANG_ARABIC:
        case LANG_HEBREW:
        case LANG_URDU:
        case LANG_FARSI:
        case LANG_YIDDISH:
        case LANG_SINDHI:
        case LANG_KASHMIRI:
            return TRUE;
    }

    return FALSE;
}

//+----------------------------------------------------------------------------
//  Member:     GetScriptProperties(eScript)
//
//  Synopsis:   Return a pointer to the script properties describing the script
//              eScript.
//-----------------------------------------------------------------------------

static const SCRIPT_PROPERTIES ** s_ppScriptProps = NULL;
static int s_cScript = 0;
static const SCRIPT_PROPERTIES s_ScriptPropsDefault =
{
    LANG_NEUTRAL,   // langid
    FALSE,          // fNumeric
    FALSE,          // fComplex
    FALSE,          // fNeedsWordBreaking
    FALSE,          // fNeedsCaretInfo
    ANSI_CHARSET,   // bCharSet
    FALSE,          // fControl
    FALSE,          // fPrivateUseArea
    FALSE,          // fReserved
};

const SCRIPT_PROPERTIES * GetScriptProperties(
    WORD eScript)
{
    if (s_ppScriptProps == NULL)
    {
        HRESULT hr;

        Assert(s_cScript == 0);
        if(g_bUSPJitState == JIT_OK)
            hr = ::ScriptGetProperties(&s_ppScriptProps, &s_cScript);
        else
            hr = E_PENDING;

        if (FAILED(hr))
        {
            // This should only fail if USP cannot be loaded. We shouldn't
            // really have made it here in the first place if this is true,
            // but you never know...
            return &s_ScriptPropsDefault;
        }
    }
    Assert(s_ppScriptProps != NULL && eScript < s_cScript &&
           s_ppScriptProps[eScript] != NULL);
    return s_ppScriptProps[eScript];
}

//+----------------------------------------------------------------------------
//  Member:     GetNumericScript(lang)
//
//  Synopsis:   Returns the script that should be used to shape digits in the
//              given language.
//-----------------------------------------------------------------------------

const WORD GetNumericScript(DWORD lang)
{
    WORD eScript = 0;

    // We should never get here without having called GetScriptProperties().
    Assert(s_ppScriptProps != NULL && eScript < s_cScript &&
           s_ppScriptProps[eScript] != NULL);
    for (eScript = 0; eScript < s_cScript; eScript++)
    {
        if (s_ppScriptProps[eScript]->langid == lang &&
            s_ppScriptProps[eScript]->fNumeric)
        {
            return eScript;
        }
    }

    return SCRIPT_UNDEFINED;
}

//+----------------------------------------------------------------------------
//  Member:     ScriptItemize(...)
//
//  Synopsis:   Dynamically grows the needed size of paryItems as needed to
//              successfully itemize the input string.
//-----------------------------------------------------------------------------
HRESULT WINAPI ScriptItemize(
    PCWSTR                  pwcInChars,     // In   Unicode string to be itemized
    int                     cInChars,       // In   Character count to itemize
    int                     cItemGrow,      // In   Items to grow by if paryItems is too small
    const SCRIPT_CONTROL   *psControl,      // In   Analysis control (optional)
    const SCRIPT_STATE     *psState,        // In   Initial bidi algorithm state (optional)
    CDataAry<SCRIPT_ITEM>  *paryItems,      // Out  Array to receive itemization
    PINT                    pcItems)        // Out  Count of items processed
{
    HRESULT hr;

    // ScriptItemize requires that the max item buffer size be AT LEAST 2
    Assert(cItemGrow > 2);

    if(paryItems->Size() < 2)
    {
        hr = paryItems->Grow(cItemGrow);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

    do {
        hr = ScriptItemize(pwcInChars, cInChars, paryItems->Size(),
                           psControl, psState, (SCRIPT_ITEM*)*paryItems, pcItems);

        if (hr == E_OUTOFMEMORY)
        {
            if (FAILED(paryItems->Grow(paryItems->Size() + cItemGrow)))
            {
                goto Cleanup;
            }
        }
    } while(hr == E_OUTOFMEMORY);

Cleanup:
    if (SUCCEEDED(hr))
    {
        // NB (mikejoch) *pcItems doesn't include the sentinel item.
        Assert(*pcItems < paryItems->Size());
        paryItems->SetSize(*pcItems + 1);
    }
    else
    {
        *pcItems = 0;
        paryItems->DeleteAll();
    }

    return hr;
}



#ifdef NO_MULTILANG
UINT
WindowsCodePageFromCodePage( CODEPAGE cp )
{
    return GetACP();
}

BYTE
WindowsCharsetFromCodePage( CODEPAGE cp )
{
    return DEFAULT_CHARSET;
}
#endif

// (_uwrap/unicwrap) end }
// **************************************************************************

// BUGBUG (cthrash) This class (CIntlFont) should be axed when we implement
// a light-weight fontlinking implementation of Line Services.  This LS
// implementation will work as a DrawText replacement, and can be used by
// intrinsics as well.

CIntlFont::CIntlFont(
    HDC hdc,
    CODEPAGE codepage,
    LCID lcid,
    SHORT sBaselineFont,
    const TCHAR * psz)
{
    BYTE bCharSet = WindowsCharsetFromCodePage( codepage );

    _hdc = hdc;
    _hFont = NULL;

    Assert(sBaselineFont >= 0 && sBaselineFont <= 4);
    Assert(psz);

    if (IsStraightToUnicodeCodePage(codepage))
    {
        BOOL fSawHan = FALSE;
        SCRIPT_ID sid;
        
        // If the document is in a Unicode codepage, we need determine the
        // best-guess charset for this string.  To do so, we pick the first
        // interesting script id.

        while (*psz)
        {
            sid = ScriptIDFromCh(*psz++);

            if (sid == sidHan)
            {
                fSawHan = TRUE;
                continue;
            }
            
            if (sid > sidAsciiLatin)
                break;
        }

        if (*psz)
        {
            // We found something interesting, go pick that font up

            codepage = DefaultCodePageFromScript( &sid, CP_UCS_2, lcid );

        }
        else if (!fSawHan)
        {
            // the string contained nothing interesting, go with the stock GUI font

            _hFont = (HFONT)GetStockObject( DEFAULT_GUI_FONT );
            _fIsStock = sBaselineFont == 2;
        }
        else
        {
            // We saw a Han character, but nothing else which would
            // disambiguate the script.  Furthermore, we don't have a good
            // fallback as we did in the above case.

            sid = sidHan;
            codepage = DefaultCodePageFromScript( &sid, CP_UCS_2, lcid );
        }
    }
    else if (ANSI_CHARSET == bCharSet
#ifndef WIN16
         || (VER_PLATFORM_WIN32_WINDOWS == g_dwPlatformID &&
         !IsFECharset(bCharSet))
#endif //!WIN16
        )
    {
        // If we're looking for an ANSI font, or if we're under NT and
        // looking for a non-FarEast font, the stockfont will suffice.

        _hFont = (HFONT)GetStockObject( ANSI_VAR_FONT );
        _fIsStock = sBaselineFont == 2;
    }
    else
    {
        codepage = WindowsCodePageFromCodePage( codepage );
    }

    if (!_hFont && codepage == g_cpDefault)
    {
        // If we're going to get the correct native charset the, the
        // GUI font will work nicely.

        _hFont = (HFONT)GetStockObject( DEFAULT_GUI_FONT );
        _fIsStock = sBaselineFont == 2;
    }

    if (_hFont)
    {
        if (!_fIsStock)
        {
            LOGFONT lf;

            GetObject(_hFont, sizeof(lf), &lf);

            lf.lfHeight = MulDivQuick( lf.lfHeight, 4 + sBaselineFont, 6 );
            lf.lfWidth  = 0;
            lf.lfOutPrecision |= OUT_TT_ONLY_PRECIS;

            _hFont = CreateFontIndirect( &lf );
        }
    }
    else
    {
        // We'd better cook up a font if all else fails.

        LOGFONT lf;

        DefaultFontInfoFromCodePage( codepage, &lf );

        lf.lfHeight = MulDivQuick( lf.lfHeight, 4 + sBaselineFont, 6 );
        lf.lfWidth  = 0;
        lf.lfOutPrecision |= OUT_TT_ONLY_PRECIS;

        _hFont = CreateFontIndirect( &lf );
        _fIsStock = FALSE;
    }

    _hOldFont = (HFONT)SelectObject( hdc, _hFont );
}

CIntlFont::~CIntlFont()
{
    SelectObject( _hdc, _hOldFont );

    if (!_fIsStock)
    {
        DeleteObject( _hFont );
    }
}

// NB (cthrash) If we have a NativeFontCtl embedded in our dialog template,
// this will enable us to use the correct "native" font for all the controls
// in the dialog.

BOOL CommCtrlNativeFontSupport()
{
    INITCOMMONCONTROLSEX icc;

    icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icc.dwICC = ICC_NATIVEFNTCTL_CLASS;

    return InitCommonControlsEx(&icc);
}

//
// "Far East" characters
//

BOOL
IsFEChar(TCHAR ch)
{
    const SCRIPT_ID sid = ScriptIDFromCh(ch);

    return (sid - sidFEFirst) <= (sidFELast - sidFEFirst);
}

//+-----------------------------------------------------------------------------
//
//  Function:   MlangGetDefaultFont
//
//  Synopsis:   Queries MLANG for a font that supports a particular script.
//
//  Returns:    An atom value for the script-appropriate font.  0 on error.
//              0 implies the system font.
//
//------------------------------------------------------------------------------

HRESULT
MlangGetDefaultFont( 
    SCRIPT_ID sid,
    SCRIPTINFO *psi )
{
    IEnumScript * pEnumScript = NULL;
    HRESULT hr;

    Assert(psi);
    
    hr = THR( EnsureMultiLanguage() );
    if (hr)
        goto Cleanup;

    if (g_pMultiLanguage2)
    {
        UINT cNum;
        SCRIPTINFO si;

        hr = THR( g_pMultiLanguage2->GetNumberOfScripts( &cNum ) );
        if (hr)
            goto Cleanup;
        
        hr = THR( g_pMultiLanguage2->EnumScripts( SCRIPTCONTF_SCRIPT_USER, 0, &pEnumScript ) );
        if (hr)
            goto Cleanup;

        while (cNum--)
        {
            ULONG c;
            
            hr = THR( pEnumScript->Next( 1, &si, &c ) );
            if (hr || !c)
            {
                hr = !c ? E_FAIL : hr;
                goto Cleanup;
            }

            if (si.ScriptId == sid)
            {
                *psi = si;
                break;
            }
        }
    }

Cleanup:
            
    ReleaseInterface( pEnumScript );

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Function:   IsNarrowCharset(BYTE bCharSet)
//
//  Returns:    FALSE if charset is SHIFTJIS (128), HANGEUL (129), GB2312 (134)
//              or CHINESEBIG5 (136).
//              TRUE for anything else.
//
//------------------------------------------------------------------------------

BOOL
IsNarrowCharSet(BYTE bCharSet)
{
    // Hack (cthrash) GDI does not define (currently, anyway) charsets 130, 131
    // 132, 133 or 135.  Make the test simpler by rejecting everything outside
    // of [SHIFTJIS,CHINESEBIG5].

    return (unsigned int)(bCharSet - SHIFTJIS_CHARSET) > (unsigned int)(CHINESEBIG5_CHARSET - SHIFTJIS_CHARSET);
}

CODEPAGE
DefaultCodePageFromCharSet(
    BYTE bCharSet,
    UINT uiFamilyCodePage )
{
    CODEPAGE cp;

    switch (bCharSet)
    {
        default:
        case DEFAULT_CHARSET:
        case OEM_CHARSET:
        case SYMBOL_CHARSET:        cp = uiFamilyCodePage;  break;
        case SHIFTJIS_CHARSET:      cp = CP_JPN_SJ;         break;
        case HANGUL_CHARSET:        cp = CP_KOR_5601;       break;
        case GB2312_CHARSET:        cp = CP_CHN_GB;         break;
        case CHINESEBIG5_CHARSET:   cp = CP_TWN;            break;
        case JOHAB_CHARSET:         cp = CP_KOR_5601;       break;
        case EASTEUROPE_CHARSET:    cp = CP_1250;           break;
        case RUSSIAN_CHARSET:       cp = CP_1251;           break;
        case ANSI_CHARSET:          cp = CP_1252;           break;
        case GREEK_CHARSET:         cp = CP_1253;           break;
        case TURKISH_CHARSET:       cp = CP_1254;           break;
        case HEBREW_CHARSET:        cp = CP_1255;           break;
        case ARABIC_CHARSET:        cp = CP_1256;           break;
        case BALTIC_CHARSET:        cp = CP_1257;           break;                                    
        case VIETNAMESE_CHARSET:    cp = CP_1258;           break;
        case THAI_CHARSET:          cp = CP_THAI;           break;
    }

    return cp;
}

const WCHAR g_achLatin1MappingInUnicodeControlArea[32] =
{
#ifndef UNIX
    0x20ac, // 0x80
    0x0081, // 0x81
    0x201a, // 0x82
    0x0192, // 0x83
    0x201e, // 0x84
    0x2026, // 0x85
    0x2020, // 0x86
    0x2021, // 0x87
    0x02c6, // 0x88
    0x2030, // 0x89
    0x0160, // 0x8a
    0x2039, // 0x8b
    0x0152, // 0x8c <min>
    0x008d, // 0x8d
    0x017d, // 0x8e
    0x008f, // 0x8f
    0x0090, // 0x90
    0x2018, // 0x91
    0x2019, // 0x92
    0x201c, // 0x93
    0x201d, // 0x94
    0x2022, // 0x95
    0x2013, // 0x96
    0x2014, // 0x97
    0x02dc, // 0x98
    0x2122, // 0x99 <max>
    0x0161, // 0x9a
    0x203a, // 0x9b
    0x0153, // 0x9c
    0x009d, // 0x9d
    0x017e, // 0x9e
    0x0178  // 0x9f
#else
    _T('?'),  // 0x80
    _T('?'),  // 0x81
    _T(','),  // 0x82
    0x00a6,   // 0x83
    _T('?'),  // 0x84
    0x00bc,   // 0x85
    _T('?'),  // 0x86
    _T('?'),  // 0x87
    0x00d9,   // 0x88
    _T('?'),  // 0x89
    _T('?'),  // 0x8a
    _T('<'),  // 0x8b
    _T('?'),  // 0x8c
    _T('?'),  // 0x8d
    _T('?'),  // 0x8e
    _T('?'),  // 0x8f
    _T('?'),  // 0x90
    0x00a2,   // 0x91
    0x00a2,   // 0x92
    0x00b2,   // 0x93
    0x00b2,   // 0x94
    0x00b7,   // 0x95
    _T('-'),  // 0x96
    0x00be,   // 0x97
    _T('~'),  // 0x98
    0x00d4,   // 0x99
    _T('s'),  // 0x9a
    _T('>'),  // 0x9b
    _T('?'),  // 0x9c
    _T('?'),  // 0x9d
    _T('?'),  // 0x9e
    0x0055    // 0x9f
#endif
};

