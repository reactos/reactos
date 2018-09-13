#include "priv.h"
#include "dochost.h"
#include "resource.h"
#include "urlprop.h"
#include "ishcut.h"
#include "shlguid.h"
#include "mlang.h"

#include <mluisupp.h>

#define DM_HISTORY 0

HRESULT PersistShortcut(IUniformResourceLocator * purl, LPCWSTR pwszFile)
{
    IPersistFile *ppf;
    HRESULT hres = purl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
    if (SUCCEEDED(hres))
    {
        hres = ppf->Save(pwszFile, TRUE);

        if (SUCCEEDED(hres))
            ppf->SaveCompleted(pwszFile);   // return value always S_OK

        ppf->Release();
    }

    return hres;
}


/*************************************************************\
    FUNCTION: GenerateUnknownShortcutName

    PARAMETERS:
        pwzSourceFilename - TCHAR Source Path and Filename that cannot be created.
                      This value will be changed to a valid path\filename
        pwzDestFilename - After pwzSourceFilename is converted to a valid filename,
                      the valid path will be returned here in a UNICODE string.
        dwSize - Size of the pwzDestFilename buffer in chars.

    DESCRIPTION:
      This function will replace the filename at the end of 
    the path in pwzFilename with "Untitled.url".  If that file
    exists, it will try, "Untitled1.url" and so on until it can
    be unique.

    WARNING:
       This function will only allow the incoming value be in ANSI
    because these helper functions (like PathRemoveFileSpecW) won't
    work on Win95 when compiled in UNICODE.  (CharNextW isn't supported
    on Win95)
\*************************************************************/


#define MAX_GEN_TRIES    100
#define GEN_EXTION_LEN   (7 * sizeof(TCHAR))    // size == L"000.url" in chars

BOOL GenerateUnknownShortcutName(
                     IN  LPCTSTR  pszSourceFilename,    
                     IN  LPWSTR  pwzDestFilename, 
                     IN  DWORD   dwSize)
{
    TCHAR	szTempFilename[MAX_PATH];
    TCHAR	szUntitledStr[MAX_PATH];
    LONG	lTry	= 1;

    if (MLLoadString(IDS_UNTITLE_SHORTCUT, szUntitledStr, ARRAYSIZE(szUntitledStr)))
    {
        StrCpyN(szTempFilename, pszSourceFilename, ARRAYSIZE(szTempFilename));
        PathRemoveFileSpec(szTempFilename);   // "Path"
        PathAddBackslash(szTempFilename);     // "Path\"

        // Make sure the string is large enough (including terminator).  (Counting chars, not bytes)
        if (dwSize > (DWORD)(lstrlen(szTempFilename) + lstrlen(szUntitledStr) + GEN_EXTION_LEN))  
        {
            PathCombine(szUntitledStr, szTempFilename, szUntitledStr);    // "Path\untitled"
            wnsprintf(szTempFilename, ARRAYSIZE(szTempFilename), TEXT("%s.url"), szUntitledStr);           // "Path\untitled.url"

            // Make a reasonable number of tries (MAX_GEN_TRIES) to find a unique
            // filename.  "path\Untitled.url", "path\Untitled1.url", ...
            while ((PathFileExists(szTempFilename)) && (lTry < MAX_GEN_TRIES))
                wnsprintf(szTempFilename, ARRAYSIZE(szTempFilename), TEXT("%s%ld.url"), szUntitledStr, lTry++);
        
            if (!PathFileExists(szTempFilename))
            {
                if (SHTCharToUnicode(szTempFilename, pwzDestFilename, dwSize) > 0)
                    return(TRUE);
            }
        }
    }
    return(FALSE);
}

//
// If no directory is specified then it is simply made into a path name without any dir attached
// 
STDAPI_(BOOL) GetShortcutFileName(LPCTSTR pszTarget, LPCTSTR pszTitle, LPCTSTR pszDir, LPTSTR pszOut, int cchOut)
{
    TCHAR szFullName[MAX_PATH];
    BOOL fAddDotUrl = TRUE;
    UINT cchMax;

    static const TCHAR c_szDotURL[] = TEXT(".url");

    TraceMsg(DM_HISTORY, "GetShortcutFileName pszDir          = %s", pszDir);

     // Check if the title has some characters that just cannot be 
     // displayed 
    if(IsOS(OS_WIN95) || (IsOS(OS_NT) && !IsOS(OS_NT5)))
    {


        
        IMultiLanguage2 *pMultiLanguage  = NULL;
        if (S_OK == CoCreateInstance(
                    CLSID_CMultiLanguage,
                    NULL,
                    CLSCTX_INPROC_SERVER,
                    IID_IMultiLanguage2,
                    (void**)&pMultiLanguage))
        {
            ASSERT(pMultiLanguage);
            DWORD dwMode = 0;
            UINT  uSrcSize = lstrlenW(pszTitle);
            UINT  uDestSize;
            if (S_OK != pMultiLanguage->ConvertStringFromUnicodeEx(&dwMode, GetACP(), (LPTSTR)pszTitle, &uSrcSize, 
                                            NULL, &uDestSize, MLCONVCHARF_NOBESTFITCHARS, NULL))

            {
                pszTitle = NULL; // Don't Use the title
            }

            pMultiLanguage->Release();    
        }
    }

    cchMax = ARRAYSIZE(szFullName) - lstrlen(c_szDotURL);

    if (pszTitle && pszTitle[0])
        StrCpyN(szFullName, pszTitle, cchMax);
    else if (pszTarget && pszTarget[0])
    {
        UINT cchLen;
        StrCpyN(szFullName, PathFindFileName(pszTarget), cchMax);
        cchLen = lstrlen(szFullName);
        if(szFullName[cchLen -1] == TEXT('/')) // Catch the common case of ftp://foo/
            szFullName[cchLen -1] = TEXT('\0');   
    }
    else
    {
        fAddDotUrl = FALSE;
        MLLoadString(IDS_NEW_INTSHCUT, szFullName, SIZECHARS(szFullName));
    }

    if(fAddDotUrl)
        StrCatBuff(szFullName, c_szDotURL, ARRAYSIZE(szFullName));

    if(pszDir)
    {
        if (PathCleanupSpec(pszDir, szFullName) & PCS_FATAL)
        {
            return FALSE;
        }
        PathCombine(pszOut, pszDir, szFullName);
    }
    else
    {
        StrCpyN(pszOut, szFullName, cchOut);
    }
    
    TraceMsg(DM_HISTORY, "GetShortcutFileName pszOut      = %s", pszOut);
    
    return TRUE;
}

// Unfortunately we do not already have something like this around
// If you find duplicate, please nuke this (dli)
// Warning: This function does not consider all possible URL cases.  
BOOL _GetPrettyURLName(LPCTSTR pcszURL, LPCTSTR pcszDir, LPTSTR pszUrlFile, int cchUrlFile)
{
    BOOL bRet = FALSE;
    PARSEDURL pu = {0};
    pu.cbSize = SIZEOF(PARSEDURL);
    
    if (SUCCEEDED(ParseURL(pcszURL, &pu)))
    {
        LPCTSTR pszPrettyName = pu.pszSuffix;
        
        // Get rid of the forward '/' 
        while (*pszPrettyName && *pszPrettyName == TEXT('/'))
            pszPrettyName++;
        
        if (!StrCmpN(pszPrettyName, TEXT("www."), 4))
            pszPrettyName += 4;
        
        if (*pszPrettyName)
            bRet = GetShortcutFileName(pcszURL, pszPrettyName, pcszDir, pszUrlFile, cchUrlFile);
    }
    return bRet;
}
/*
 * pcszURL -> "ftp://ftp.microsoft.com"
 * pcszPath -> "c:\windows\desktop\internet\Microsoft FTP.url"
 */
HRESULT 
CreateNewURLShortcut(
                     IN  LPCTSTR pcszURL, 
                     IN  LPCITEMIDLIST pidlURL, 
                     IN  LPCTSTR pcszURLFile,
                     IN  LPCTSTR pcszDir,
                     OUT LPTSTR  pszOut,
                     IN  int     cchOut,
                     IN  BOOL    bUpdateProperties,
                     IN  BOOL    bUpdateIcon,
                     IN  IOleCommandTarget *pCommandTarget)
{
    HRESULT hr;

    WCHAR wszFile[MAX_URL_STRING];

    if (SHTCharToUnicode(pcszURLFile, wszFile, ARRAYSIZE(wszFile)))
    {
        IUniformResourceLocator *purl;

        hr = CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
            IID_IUniformResourceLocator, (LPVOID*)&purl);

        if (SUCCEEDED(hr))
        {
            if (pidlURL)
            {
                // if we're given a pidl, try to set pidl first.
                
                IShellLink *psl;
                
                hr = purl->QueryInterface(IID_IShellLink, (LPVOID*)&psl);
                if (SUCCEEDED(hr))
                {
                    hr = psl->SetIDList(pidlURL);
                    psl->Release();
                }
            }
            
            if (!pidlURL || FAILED(hr))
                hr = purl->SetURL(pcszURL, 0);

            if(S_OK == hr)
                IUnknown_SetSite(purl, pCommandTarget);
                
           
            if (SUCCEEDED(hr))
            {
                // Persist the internet shortcut
                hr = PersistShortcut(purl, wszFile);

                // If the previous call fails, try again with a new Filename.
                // This is needed because the other filename could have been invalid,
                // which will happen if the web page's title was stored in DBCS with
                // a non-English code page.
    
                // (dli) First try a file name related to the URL, then the default untitled
                if (FAILED(hr))
                {
                    TCHAR tszFile[MAX_PATH];
                    BOOL bURLname = _GetPrettyURLName(pcszURL, pcszDir, tszFile, ARRAYSIZE(tszFile));
                    if ((bURLname && SHTCharToUnicode(tszFile, wszFile, ARRAYSIZE(wszFile)) > 0) ||
                        (!bURLname && GenerateUnknownShortcutName(pcszURLFile, wszFile, ARRAYSIZE(wszFile))))
                    {
                        hr = PersistShortcut(purl, wszFile);
                    }

                }

                if (SUCCEEDED(hr))
                {
                    TCHAR   szFile[MAX_PATH];
                    VARIANT varIn = {0};

                    if(bUpdateIcon)
                    {
                        HRESULT hrTemp = IUnknown_Exec(purl, &CGID_ShortCut, ISHCUTCMDID_DOWNLOADICON, 0, NULL, NULL);
                        ASSERT(SUCCEEDED(hrTemp));
                    }

                    
                    varIn.vt = VT_UNKNOWN;
                    varIn.punkVal = purl;
                
                    OleStrToStrN(szFile,  ARRAYSIZE(szFile), wszFile, -1);
#ifndef UNIX
                    SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, szFile, NULL);
#else
                    // IEUNIX : Synchronous notifications for unix.
                    SHChangeNotify(SHCNE_CREATE, ( SHCNF_PATH | SHCNF_FLUSH ), szFile, NULL);
#endif

                    if (pszOut) {
                        StrCpyN(pszOut, wszFile, cchOut);
                    }
                }
            }
            purl->Release();
        }
    }
    else
        hr = E_FAIL;

    return(hr);
}


BOOL ILCanCreateLNK(LPCITEMIDLIST pidl)
{
    HRESULT hr = S_FALSE;
    DWORD dwAttributes = SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR;

    // BUGBUG: Should call IsBrowserFrameOptionsPidlSet(BIF_PREFER_INTERNET_SHORTCUT) instead.  Some URL delegate
    //         NSEs (FTP for one) may want .lnks instead of .url files.
    //         This would be great for CDocObjFolder to not set this bit so
    //         for .doc files so they will use the .lnk versions.
    if (!pidl || IsURLChild(pidl, TRUE))
        return FALSE;

    hr = IEGetAttributesOf(pidl, &dwAttributes);
    return (SUCCEEDED(hr) && 
         (IsFlagSet(dwAttributes, SFGAO_FOLDER) ||
          IsFlagSet(dwAttributes, SFGAO_FILESYSANCESTOR) )
        );
}


// This API makes a callback via the IN parameter
// pCommand to inform that shortcut creation is over.
// The callback it currently sends back are : 
// 


STDAPI
CreateShortcutInDirEx(ISHCUT_PARAMS *pParams)
{
    LPCITEMIDLIST pidlTarget = pParams->pidlTarget;
    TCHAR szFileName[MAX_PATH];
    TCHAR szTarget[MAX_URL_STRING];
    HRESULT hres;
    BOOL bIsURL = IsURLChild(pidlTarget, TRUE);

    if (!ILCanCreateLNK(pidlTarget) &&
        SUCCEEDED(IEGetDisplayName(pidlTarget, szTarget, SHGDN_FORPARSING)) &&
        _ValidateURL(szTarget, UQF_DEFAULT))
    {
        BOOL bUsePidl;
        
        // Note that _ValidateURL() calls IURLQualify() which adds "file://"
        // prefix to szTarget as appropriate.
        
        if (bIsURL ||
            (GetUrlScheme(szTarget) == URL_SCHEME_FILE))
        {
            bUsePidl = FALSE;
        }
        else
        {
            // use pidl if it's not URL or file: compatible.
            bUsePidl = TRUE;
        }
        
        GetShortcutFileName(szTarget, pParams->pszTitle, pParams->pszDir, szFileName, ARRAYSIZE(szFileName));
        if(pParams->bUniqueName)
            PathYetAnotherMakeUniqueName(szFileName, szFileName, NULL, NULL);
        hres = CreateNewURLShortcut(szTarget, bUsePidl ? pidlTarget : NULL, szFileName, pParams->pszDir, 
                                    pParams->pszOut, pParams->cchOut, pParams->bUpdateProperties,
                                    pParams->bUpdateIcon, pParams->pCommand);

        
    } else {
        hres = CreateLinkToPidl(pidlTarget, pParams->pszDir, pParams->pszTitle, pParams->pszOut, pParams->cchOut);
    }

    return hres;
}


// pidlTarget ... the thing the shortcut is going to point to
// pszDir .. the directory that should hold the shortcut

// WARNING:  if you change any parameters for this function, you 
//           need to fix up explorer.exe
STDAPI CreateShortcutInDirA(
                     IN  LPCITEMIDLIST pidlTarget, 
                     IN  LPSTR   pszTitle, 
                     IN  LPCSTR  pszDir, 
                     OUT LPSTR   pszOut,
                     IN  BOOL    bUpdateProperties)
{
    HRESULT hres = E_FAIL;
    TCHAR szTitle[MAX_PATH];
    TCHAR szDir[MAX_PATH];
    TCHAR szOut[MAX_URL_STRING];
    ISHCUT_PARAMS ShCutParams = {0};
    SHAnsiToTChar(pszTitle, szTitle, ARRAYSIZE(szTitle));
    SHAnsiToTChar(pszDir, szDir, ARRAYSIZE(szDir));

    ShCutParams.pidlTarget = pidlTarget;
    ShCutParams.pszTitle = szTitle; 
    ShCutParams.pszDir = szDir; 
    ShCutParams.pszOut = (pszOut ? szOut : NULL);
    ShCutParams.cchOut = (pszOut ? ARRAYSIZE(szOut) : 0);
    ShCutParams.bUpdateProperties = bUpdateProperties;
    ShCutParams.bUniqueName = FALSE;
    ShCutParams.bUpdateIcon = FALSE;
    ShCutParams.pCommand = NULL;
    ShCutParams.pDoc = NULL;
    
    hres = CreateShortcutInDirEx(&ShCutParams);

    if (pszOut && SUCCEEDED(hres))
        SHTCharToAnsi(szOut, pszOut, MAX_URL_STRING);

    return hres;
}


STDAPI CreateShortcutInDirW(
                     IN  LPCITEMIDLIST pidlTarget, 
                     IN  LPWSTR  pwszTitle, 
                     IN  LPCWSTR pwszDir, 
                     OUT LPWSTR  pwszOut,
                     IN  BOOL    bUpdateProperties)
{
    HRESULT hres = E_FAIL;
    TCHAR szTitle[MAX_PATH];
    TCHAR szDir[MAX_PATH];
    TCHAR szOut[MAX_URL_STRING];
    ISHCUT_PARAMS ShCutParams = {0};
    
    SHUnicodeToTChar(pwszTitle, szTitle, ARRAYSIZE(szTitle));
    SHUnicodeToTChar(pwszDir, szDir, ARRAYSIZE(szDir));

    ShCutParams.pidlTarget = pidlTarget;
    ShCutParams.pszTitle = szTitle; 
    ShCutParams.pszDir = szDir; 
    ShCutParams.pszOut = (pwszOut ? szOut : NULL);
    ShCutParams.cchOut = (pwszOut ? ARRAYSIZE(szOut) : 0);
    ShCutParams.bUpdateProperties = bUpdateProperties;
    ShCutParams.bUniqueName = FALSE;
    ShCutParams.bUpdateIcon = FALSE;
    ShCutParams.pCommand = NULL;
    ShCutParams.pDoc = NULL;
    hres = CreateShortcutInDirEx(&ShCutParams);

    if (pwszOut && SUCCEEDED(hres))
        SHTCharToUnicode(szOut, pwszOut, MAX_URL_STRING);

    return hres;
}


//////////////////////////////
//
// Adds the given URL to the history storage
//
//   pwzTitle may be NULL if no title exists
//   
//   Note this function may be called multiple times in a single
//   page-visit.  bUpdateProperties is TRUE only once during 
//   those sequence of calls.
//
HRESULT 
AddUrlToUrlHistoryStg(
    IN LPCWSTR   pwszUrl, 
    IN LPCWSTR   pwszTitle, 
    IN LPUNKNOWN punk,
    IN BOOL fWriteHistory,
    IN IOleCommandTarget *poctNotify,
    IN IUnknown *punkSFHistory,
    OUT UINT* pcodepage)
{
    TraceMsg(DM_HISTORY, "AddUrlToUrlHistoryStg() entered url = %s, title = %s, punk = %X, fwrite = %d, poct = %X, punkHist = %X, cp = %d",
        pwszUrl, pwszTitle, punk,fWriteHistory,poctNotify,punkSFHistory,pcodepage);


    IUrlHistoryPriv *pUrlHistStg;
    HRESULT hr;
    
    if (!pwszUrl)
        return E_POINTER;

    if (punk == NULL)
    {
        
        hr = CoCreateInstance(CLSID_CUrlHistory, NULL, CLSCTX_INPROC_SERVER, 
            IID_IUrlHistoryPriv, (void **)&pUrlHistStg);
    }
    else
    {       
        
        // query the pointer for IServiceProvider so we can get the IUrlHistoryStg
        hr = IUnknown_QueryService(punk,SID_SUrlHistory, IID_IUrlHistoryPriv, (LPVOID *)&pUrlHistStg);
    }
    
    if (SUCCEEDED(hr))
    {
        //
        // This demostrate the mechanism to get the codepage for URL.
        //
        hr = pUrlHistStg->AddUrlAndNotifyCP(pwszUrl, 
                                 pwszTitle, 
                                 0, 
                                 fWriteHistory, 
                                 poctNotify,
                                 punkSFHistory,
                                 pcodepage);
        pUrlHistStg->Release();
    }
    
    return hr;
}


