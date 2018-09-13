/*
 * isexicon.cpp - IExtractIcon implementation for URL class.
 */


#include "priv.h"
#include "htregmng.h"
#include "ishcut.h"
#include "resource.h"


// We still have to use url.dll as the source of the internet shortcut
// icons because the icons need to still be valid on uninstall.

#ifndef UNIX
#define c_szIntshcutDefaultIcon     TEXT("url.dll")
#else
// IEUNIX(perf) : use unixstyle dll name
#ifdef ux10 
#define c_szIntshcutDefaultIcon     TEXT("liburl.sl")
#else
#define c_szIntshcutDefaultIcon     TEXT("liburl.so")
#endif
#endif

#define IDEFICON_NORMAL             0

#define II_OVERLAY_UPDATED          1

typedef struct
    {
    HIMAGELIST himl;          
    HIMAGELIST himlSm;
    } URLIMAGES;

HRESULT
URLGetLocalFileName(
    LPCTSTR pszURL,
    LPTSTR szLocalFile,
    int cch,
    FILETIME* pftLastMod);


/*----------------------------------------------------------
Purpose: Initializes the images lists used by the URL icon  
         handler.

         There are just two icons placed in each imagelist:
         the given hicon and an overlay for the updated
         asterisk.

Returns: 
Cond:    --
*/
STDMETHODIMP
InitURLImageLists(
    IN URLIMAGES * pui,
    IN HICON       hicon,
    IN HICON       hiconSm)
    {
    HRESULT hres = E_OUTOFMEMORY;

    LoadCommonIcons();
    _InitSysImageLists();

    pui->himl = ImageList_Create(g_cxIcon, g_cyIcon, ILC_MASK, 2, 2);
    
    if (pui->himl)
    {
        pui->himlSm = ImageList_Create(g_cxSmIcon, g_cySmIcon, ILC_MASK, 2, 2);
        
        if ( !pui->himlSm ) 
            ImageList_Destroy(pui->himl);
        else
        {
            ImageList_SetBkColor(pui->himl, GetSysColor(COLOR_WINDOW));
            ImageList_SetBkColor(pui->himlSm, GetSysColor(COLOR_WINDOW));
         
            // Add the given icons
            ImageList_ReplaceIcon(pui->himl, -1, hicon);
            ImageList_ReplaceIcon(pui->himlSm, -1, hiconSm);

            // Add the overlay icon to the list
            ASSERT(IS_VALID_HANDLE(g_hiconSplat, ICON));
            ASSERT(IS_VALID_HANDLE(g_hiconSplatSm, ICON));

            if (g_hiconSplat)
            {
                int iOverlay = ImageList_ReplaceIcon(pui->himl, -1, g_hiconSplat);
                ImageList_ReplaceIcon(pui->himlSm, -1, g_hiconSplatSm);

                ImageList_SetOverlayImage(pui->himl, iOverlay, II_OVERLAY_UPDATED);
                ImageList_SetOverlayImage(pui->himlSm, iOverlay, II_OVERLAY_UPDATED);
            }
            
            hres = S_OK;
        }
    }

    return hres;
    }


/*----------------------------------------------------------
Purpose: Destroys the url image lists

Returns: 
Cond:    --
*/
STDMETHODIMP
DestroyURLImageLists(
    IN URLIMAGES * pui)
{
    if (pui->himl)       
    {
        ImageList_Destroy(pui->himl);
        pui->himl = NULL;
    }

    if (pui->himlSm)       
    {
        ImageList_Destroy(pui->himlSm);
        pui->himlSm = NULL;
    }

    return S_OK;
}



/*----------------------------------------------------------
Purpose: Gets the icon location (filename and index) from the registry
         of the given key.

         BUGBUG (scotth): this looks like really generic code.  There
                          must be something like this already in shell32

Returns: 
Cond:    --
*/
HRESULT 
GetURLIcon(
    IN  HKEY    hkey, 
    IN  LPCTSTR pcszKey, 
    IN  LPTSTR  pszIconFile,
    IN  UINT    cchIconFile, 
    OUT PINT    pniIcon)
{
    HRESULT hres = S_FALSE;
    DWORD dwSize = CbFromCch(cchIconFile);

    ASSERT(IS_VALID_HANDLE(hkey, KEY));
    ASSERT(IS_VALID_STRING_PTR(pcszKey, -1));
    ASSERT(IS_VALID_WRITE_BUFFER(pszIconFile, TCHAR, cchIconFile));
    ASSERT(IS_VALID_WRITE_PTR(pniIcon, INT));

    if (NO_ERROR == SHGetValue(hkey, pcszKey, NULL, NULL, pszIconFile, &dwSize))
    {
        *pniIcon = PathParseIconLocation(pszIconFile);
        hres = S_OK;
    }

    ASSERT(IsValidIconIndex(hres, pszIconFile, cchIconFile, *pniIcon));

    return hres;
}


/*
** GetFallBackGenericURLIcon()
**
**
**
** Arguments:
**
** Returns:       S_OK if fallback generic icon information retrieved
**                successfully.
**                E_FAIL if not.
**
** Side Effects:  none
*/
HRESULT 
GetFallBackGenericURLIcon(
    LPTSTR pszIconFile,
    UINT cchIconFile,
    PINT pniIcon)
{
    HRESULT hr;

    ASSERT(IS_VALID_WRITE_BUFFER(pszIconFile, TCHAR, cchIconFile));
    ASSERT(IS_VALID_WRITE_PTR(pniIcon, INT));

    // Fall back to first icon in this module.

    StrCpyN(pszIconFile, c_szIntshcutDefaultIcon, cchIconFile);
    *pniIcon = IDEFICON_NORMAL;

    hr = S_OK;

    TraceMsg(TF_INTSHCUT, "GetFallBackGenericURLIcon(): Using generic URL icon file %s, index %d.",
              pszIconFile, *pniIcon);

    ASSERT(IsValidIconIndex(hr, pszIconFile, cchIconFile, *pniIcon));

    return(hr);
}


/*
** GetGenericURLIcon()
**
**
**
** Arguments:
**
** Returns:       S_OK if generic icon information retrieved successfully.
**                Otherwise error.
**
** Side Effects:  none
*/
HRESULT 
GetGenericURLIcon(
    LPTSTR pszIconFile,
    UINT cchIconFile, 
    PINT pniIcon)
{
    HRESULT hr;

    ASSERT(IS_VALID_WRITE_BUFFER(pszIconFile, TCHAR, cchIconFile));
    ASSERT(IS_VALID_WRITE_PTR(pniIcon, INT));

    hr = GetURLIcon(HKEY_CLASSES_ROOT, TEXT("InternetShortcut\\DefaultIcon"), pszIconFile,
                    cchIconFile, pniIcon);

    if (hr == S_FALSE)
        hr = GetFallBackGenericURLIcon(pszIconFile, cchIconFile, pniIcon);

    ASSERT(IsValidIconIndex(hr, pszIconFile, cchIconFile, *pniIcon));

    return(hr);
}


/****************************** Public Functions *****************************/


/*----------------------------------------------------------
Purpose: Given a full URL path, this function returns the 
         registry path to the associated protocol (plus the
         subkey path).

         pszBuf must be MAX_PATH.

Returns: 
Cond:    --
*/
HRESULT 
GetURLKey(
    LPCTSTR pcszURL, 
    LPCTSTR pszSubKey, 
    LPTSTR  pszBuf,
    int cchBuf)
{
    HRESULT hres;
    PTSTR pszProtocol;

    ASSERT(IS_VALID_STRING_PTR(pcszURL, -1));
    ASSERT(IS_VALID_STRING_PTR(pszSubKey, -1));
    ASSERT(IS_VALID_WRITE_BUFFER(pszBuf, TCHAR, MAX_PATH));

    *pszBuf = '\0';

    hres = CopyURLProtocol(pcszURL, &pszProtocol, NULL);

    if (hres == S_OK)
    {
        StrCpyN(pszBuf, pszProtocol, cchBuf);
        PathAppend(pszBuf, pszSubKey);

        LocalFree(pszProtocol);
    }

    return hres;
}


/********************************** Methods **********************************/

/*----------------------------------------------------------
Purpose : To help determine if the file to which this shortcut
          is persisted is in the favorites hierarchy
          
Returns : Returns TRUE if this shortcut is in the favorites 
         folder
*/


BOOL Intshcut::_IsInFavoritesFolder()
{
    BOOL fRet = FALSE;

    if(m_pszFile)
    {
        TCHAR szPath[MAX_PATH];
        if(SHGetSpecialFolderPath(NULL, szPath, CSIDL_FAVORITES, TRUE))
        {
            // Is szPath (i.e. the favorites dir) a prefix of the file associated with this
            // shortcut ?
            fRet = PathIsPrefix(szPath, m_pszFile);
        }
    }

    return fRet;
    
}
         
/*----------------------------------------------------------
Purpose: Get the icon location of the given url.

Returns: S_FALSE if the location is default for the type
         S_OK if the location is custom

         The way this extracticon stuff works is very strange and not
         well-documented.  In particular, there are multiple levels of
         name munging going on, and it's not clear how information is
         passed between IExtractIcon::GetIconLocation and
         IExtractIcon::Extract.  (In particular, it seems that we maintain
         state in our object in order to do secret communication between
         the two methods, which is out of spec.  The shell is allowed to
         instantiate you, call GetIconLocation, then destroy you.  Then
         the next day, it can instantiate you and call Extract with the
         result from yesterday's GetIconLocation.)

         I'm not going to try to fix it; I'm just
         pointing it out in case somebody has to go debugging into this
         code and wonders what is going on.

Cond:    --
*/
STDMETHODIMP
Intshcut::GetURLIconLocation(
    IN  UINT    uInFlags,
    IN  LPTSTR  pszBuf,
    IN  UINT    cchBuf,
    OUT int *   pniIcon,
    BOOL fRecentlyChanged,
    OUT PUINT  puOutFlags)
{
    // Call the IShellLink::GetIconLocation method
    HRESULT hres = _GetIconLocationWithURLHelper(pszBuf, cchBuf, pniIcon, NULL, 0, fRecentlyChanged);
    BOOL fNeedQualify = TRUE;
    hres = S_FALSE;
    if (*pszBuf)
    {
        if(puOutFlags && (FALSE == PathFileExists(pszBuf)))
            SetFlag(*puOutFlags, GIL_NOTFILENAME);
    }
    else
    {
        
        if(FALSE == _IsInFavoritesFolder() || (IsIEDefaultBrowserQuick()))
        {
            // This shortcut is not in the favorites folder as far as we know 
            TCHAR szURL[INTERNET_MAX_URL_LENGTH];

            *szURL = 0;

            hres = InitProp();
            if (SUCCEEDED(hres))
                m_pprop->GetProp(PID_IS_URL, szURL, SIZECHARS(szURL));

            if (*szURL)
            {
                TCHAR szT[MAX_PATH];

                hres = E_FAIL;

                // If it's a file:// URL, then default to the icon from
                // the file target.  Must use IExtractIconA in case we're
                // on Win95.
                IExtractIconA *pxi;
                if (_TryLink(IID_IExtractIconA, (void **)&pxi))
                {
                    // S_FALSE means "I don't know what icon to use",
                    // so treat only S_OK as successful icon extraction.
                    if (IExtractIcon_GetIconLocation(pxi, uInFlags, pszBuf, cchBuf, pniIcon, puOutFlags) == S_OK)
                    {
                        hres = S_OK;
                        fNeedQualify = FALSE;
                    }

                    pxi->Release();
                }

                // If couldn't get target icon or not a file:// URL, then
                // go get some default icon based on the URL scheme.
                if (FAILED(hres))
                {
                    // Look up URL icon based on protocol handler.

                    hres = GetURLKey(szURL, TEXT("DefaultIcon"), szT, ARRAYSIZE(szT));

                    if (hres == S_OK)
                    {
                        hres = GetURLIcon(HKEY_CLASSES_ROOT, szT, pszBuf, 
                                          cchBuf, pniIcon);
                    }
                }
            }
        }
        
        if (hres == S_FALSE)
        {
            // Use generic URL icon.

            hres = GetFallBackGenericURLIcon(pszBuf, cchBuf, pniIcon); // Make sure we have the E icon and 
                                                                       // Not any of netscape's icons

            if (hres == S_OK)
                TraceMsg(TF_INTSHCUT, "Intshcut::GetIconLocation(): Using generic URL icon.");
        }

        if (hres == S_OK && fNeedQualify)
        {
            TCHAR szFullPath[MAX_PATH];

            if (PathSearchAndQualify(pszBuf, szFullPath, SIZECHARS(szFullPath)))
            {
                hres = S_OK;

                if ((UINT)lstrlen(szFullPath) < cchBuf)
                    StrCpyN(pszBuf, szFullPath, cchBuf);
                else
                    hres = E_FAIL;
            }
            else
                hres = E_FILE_NOT_FOUND;
        }
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: Helper function that determines the icon location based
         on the flags property of the internet site property set.

Returns: 
Cond:    --
*/
STDMETHODIMP
Intshcut::GetIconLocationFromFlags(
    IN  UINT   uInFlags,
    OUT LPTSTR pszIconFile,
    IN  UINT   cchIconFile,
    OUT PINT   pniIcon,
    OUT PUINT  puOutFlags,
    IN  DWORD  dwPropFlags)
{
    HRESULT hres = S_FALSE;

    *puOutFlags = 0;

    ClearFlag(m_dwFlags, ISF_SPECIALICON);

    // Normally, the icon is the standard icon that is retrieved.
    // If the url has been updated, though, we want to add the
    // overlay, in which case we return GIL_NOTFILENAME so the
    // Extract method will be called.

    hres = GetURLIconLocation(uInFlags, pszIconFile, cchIconFile, pniIcon,
                                IsFlagSet(dwPropFlags, PIDISF_RECENTLYCHANGED), puOutFlags);
    if (SUCCEEDED(hres))
    {
        // BUGBUG (scotth): we don't support red splats on browser
        //                  only because it requires new SHELL32 APIs.

        // Has this item been updated since last viewed? 
        
        if (IsFlagSet(dwPropFlags, PIDISF_RECENTLYCHANGED) && 
                    (FALSE == (*puOutFlags & GIL_NOTFILENAME)))
        {
            // Yes; cache the item as a non-file so we get the
            // dynamically created icon 
            SetFlag(*puOutFlags, GIL_NOTFILENAME);

            // Add the icon index at the end of the filename, so
            // it will be hashed differently from the filename 
            // instance.
            wnsprintf(&pszIconFile[lstrlen(pszIconFile)], cchIconFile - lstrlen(pszIconFile),
                      TEXT(",%d"), *pniIcon);

            // cdturner
            // this is done for browser only mode to stop the shell hacking the path
            // down to the dll and not calling us
            
            // remove the dot from the string
            LPTSTR pszDot = StrRChr( pszIconFile, NULL, TCHAR('.'));
            if ( pszDot )
            {
                *pszDot = TCHAR('*');  // should be DBCS safe as it is in the lower 7 bits ASCII
            }
            
            SetFlag(m_dwFlags, ISF_SPECIALICON);
        }

    }
    else
    {
        // Init to default values
        *pniIcon = IDEFICON_NORMAL;
        if (cchIconFile > 0)
            StrCpyN(pszIconFile, c_szIntshcutDefaultIcon, cchIconFile);
    }

    return S_OK;
}


/*----------------------------------------------------------
Purpose: IExtractIcon::GetIconLocation handler for Intshcut

Returns: 
Cond:    --
*/
// This is the real one for the platform...
HRESULT
Intshcut::_GetIconLocation(
    IN  UINT   uInFlags,
    OUT LPWSTR pszIconFile,
    IN  UINT   cchIconFile,
    OUT PINT   pniIcon,
    OUT PUINT  puOutFlags)
{
    HRESULT hres;

    if(IsFlagSet(uInFlags, GIL_ASYNC))
    {
        hres = GetGenericURLIcon(pszIconFile, cchIconFile, pniIcon);
        return ((SUCCEEDED(hres)) ? E_PENDING : hres);
    }

    hres = LoadFromAsyncFileNow();
    if(FAILED(hres))
        return hres;

    hres = S_FALSE;

    // We also use this method to perform the mirroring
    // of the values between the internet shortcut file and
    // the central database.  IExtractIcon is a good interface
    // to do this because it is virtually guaranteed to be 
    // called for a URL.
    MirrorProperties();

    // Init to default values
    *puOutFlags = 0;
    *pniIcon = 0;
    if (cchIconFile > 0)
        *pszIconFile = TEXT('\0');


    DWORD dwVal = 0;

    if (m_psiteprop)
        m_psiteprop->GetProp(PID_INTSITE_FLAGS, &dwVal);

    hres = GetIconLocationFromFlags(uInFlags, pszIconFile, cchIconFile, pniIcon,
                                    puOutFlags, dwVal);


    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));

    return hres;
}

HRESULT Intshcut::_CreateShellLink(LPCTSTR pszPath, IUnknown **ppunk)
{
    IUnknown *punk;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void **)&punk);
    
    if (SUCCEEDED(hr))
    {
        if (g_fRunningOnNT)
        {
            IShellLink *psl;
            hr = punk->QueryInterface(IID_IShellLink, (void **)&psl);
            if (SUCCEEDED(hr))
            {
                hr = psl->SetPath(pszPath);
                psl->Release();
            }
        }
        else
        {
            IShellLinkA *psl;
            hr = punk->QueryInterface(IID_IShellLinkA, (void **)&psl);
            if (SUCCEEDED(hr))
            {
                CHAR sz[MAX_PATH];
                SHTCharToAnsi(pszPath, sz, SIZECHARS(sz));
                hr = psl->SetPath(sz);
                psl->Release();
            }
        }

        if (SUCCEEDED(hr))
        {
            *ppunk = punk;
        }
        else
            punk->Release();
    }

    return hr;
}
    
HRESULT
Intshcut::GetIconLocation(
    IN  UINT   uInFlags,
    OUT LPTSTR pszIconFile,
    IN  UINT   cchIconFile,
    OUT PINT   pniIcon,
    OUT PUINT  puOutFlags)
{
    HRESULT hr = E_FAIL;
    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT(IS_VALID_WRITE_BUFFER(pszIconFile, TCHAR, cchIconFile));
    ASSERT(IS_VALID_WRITE_PTR(pniIcon, INT));
    ASSERT(IS_VALID_WRITE_PTR(puOutFlags, UINT));

    if (FAILED(hr))
    {
        hr = _GetIconLocation(uInFlags, pszIconFile, cchIconFile, pniIcon, puOutFlags);
    }

    return hr;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\
//
// *** URLGetLocalFileName ***
//
//
// Description:
//
//
// Parameters:
//
//
// Return:
//
//
// Comments:
//
//
///////////////////////////////////////////////////////////////////////////////

HRESULT
URLGetLocalFileName(
    LPCTSTR pszURL,
    LPTSTR szLocalFile,
    int cch,
    FILETIME* pftLastMod
)
{
    ASSERT(pszURL);
    ASSERT(szLocalFile || 0 == cch);

    HRESULT hr = E_FAIL;

    if (pftLastMod)
    {
        pftLastMod->dwLowDateTime  = 0;
        pftLastMod->dwHighDateTime = 0;
    }

    // by using the internal shlwapi function, we avoid loading WININET 
    // unless we really really need it...
    DWORD scheme = GetUrlScheme(pszURL);
    if (scheme != URL_SCHEME_INVALID)
    {
        switch(scheme)
        {
        case URL_SCHEME_HTTP:
        case URL_SCHEME_FTP:
        case URL_SCHEME_GOPHER:
            {
                ULONG cbSize  = MAX_CACHE_ENTRY_INFO_SIZE;

                INTERNET_CACHE_ENTRY_INFO* piceiAlloced = 
                (INTERNET_CACHE_ENTRY_INFO*) new BYTE[cbSize];

                if (piceiAlloced)
                {
                    piceiAlloced->dwStructSize =
                                      sizeof(INTERNET_CACHE_ENTRY_INFO);

                    if (GetUrlCacheEntryInfoEx(pszURL, piceiAlloced,
                                               &cbSize, NULL, NULL,
                                               NULL, 0))
                    {
                        if (StrCpyN(szLocalFile,
                                    piceiAlloced->lpszLocalFileName, cch))
                        {
                            if (pftLastMod)
                            {
                                *pftLastMod = piceiAlloced->LastModifiedTime;
                            }

                            hr = S_OK;
                        }
                    }

                    delete [] piceiAlloced;
                }
            }
            break;

        case URL_SCHEME_FILE:
            hr = PathCreateFromUrl(pszURL, szLocalFile, (LPDWORD)&cch, 0);
            break;

        }
    }
    else
    {
        if (StrCpyN(szLocalFile, pszURL, cch))
            hr = S_OK;
    }

    return hr;
}


BOOL
PretendFileIsICONFileAndLoad(
    IN LPTSTR lpszTempBuf,
    OUT HICON * phiconLarge,
    OUT HICON * phiconSmall,
    IN  UINT    ucIconSize)
{
    WORD wSizeSmall = HIWORD(ucIconSize);
    WORD wSizeLarge = LOWORD(ucIconSize);

    BOOL fRet = FALSE;
    // Pretend that the file is a .ico file and load it

    ASSERT(phiconLarge);
    ASSERT(phiconSmall);
    
    *phiconSmall = (HICON)LoadImage(NULL, lpszTempBuf, IMAGE_ICON, wSizeSmall, wSizeSmall, LR_LOADFROMFILE);
    if(*phiconSmall)
    {
        fRet = TRUE;
        *phiconLarge = (HICON)LoadImage(NULL, lpszTempBuf, IMAGE_ICON, wSizeLarge, wSizeLarge, LR_LOADFROMFILE);
    }
                

    return fRet;
}




BOOL
Intshcut::ExtractIconFromWininetCache(
    IN  LPCTSTR pszIconString,
    IN  UINT    iIcon,
    OUT HICON * phiconLarge,
    OUT HICON * phiconSmall,
    IN  UINT    ucIconSize,
    BOOL *pfFoundUrl,
    DWORD dwPropFlags)
{
    IPropertyStorage *ppropstg = NULL;
    BOOL fRet = FALSE;
    INT iTempIconIndex;
    HRESULT hr;
    BOOL fFoundURL = FALSE;


    ASSERT(pfFoundUrl && (FALSE == *pfFoundUrl));
    ASSERT((lstrlen(pszIconString) + 1)<= MAX_PATH);
    
    TCHAR szTempBuf[MAX_URL_STRING + 1];
    *szTempBuf = TEXT('\0');
    TCHAR szTempIconBuf[MAX_PATH + 1];
    *szTempIconBuf = TEXT('\0');
      
    hr =  _GetIconLocationWithURLHelper(
                    szTempIconBuf, ARRAYSIZE(szTempIconBuf), &iTempIconIndex, 
                    szTempBuf, ARRAYSIZE(szTempBuf), IsFlagSet(dwPropFlags, PIDISF_RECENTLYCHANGED));
    
    if((S_OK == hr) && (*szTempIconBuf))
    {
        if((UINT)iTempIconIndex == iIcon)
        {
            if(0 == StrCmp(szTempIconBuf, pszIconString))
            {
                if(*szTempBuf)
                {
                    BOOL fUsesCache=FALSE;
                    DWORD dwBufSize=0;
                    CoInternetQueryInfo(szTempBuf, QUERY_USES_CACHE, 0,
                                     &fUsesCache, sizeof(fUsesCache), &dwBufSize, 0);

                    if(fUsesCache)
                    {
                        fFoundURL = TRUE;
                    }
                }
            }
        }
    }



    if(fFoundURL)
    {
        // Now szTempBuf has the URL of the ICON
        // now look and see if the shortcut file itself has the icon and if so
        // simply use it  --- TBD 
        
        
        // we need to grovel in the cache and see if we can get
        // it there and then convert it to an icon
        TCHAR szIconFile[MAX_PATH + 1];
        hr = URLGetLocalFileName(szTempBuf, szIconFile, ARRAYSIZE(szIconFile), NULL);

        if(S_OK == hr)
        {

            if(PretendFileIsICONFileAndLoad(szIconFile, phiconLarge, phiconSmall, ucIconSize))
            {
                fRet = TRUE;
            }

            // It's a bitmap, gif or a jpeg          
        }
    }
    

    if(pfFoundUrl)
        *pfFoundUrl = fFoundURL;
    return fRet;
}

/*----------------------------------------------------------
Purpose: IExtractIcon::Extract method for Intshcut

         Extract the icon.  This function really returns an icon
         that is dynamically created, based upon the properties
         of the URL (recently changed, etc).

         Expect that for normal cases, when the icon does not
         need to be munged (an overlay added), the GetIconLocation
         method should suffice.  Otherwise, this method will get
         called.

Returns: 
Cond:    --
*/
// This is the real one for the platform...
HRESULT
Intshcut::_Extract(
    IN  LPCTSTR pszIconFile,
    IN  UINT    iIcon,
    OUT HICON * phiconLarge,
    OUT HICON * phiconSmall,
    IN  UINT    ucIconSize)
{
    HRESULT hres;
    HICON hiconLarge = NULL;
    HICON hiconSmall = NULL;
    TCHAR szPath[MAX_PATH];
    int nIndex;
    BOOL fSpecialUrl = FALSE;
    *phiconLarge = NULL;
    *phiconSmall = NULL;
    DWORD dwPropFlags = 0;

    hres = LoadFromAsyncFileNow();
    if(FAILED(hres))
        return hres;

    hres = S_FALSE;    
    
    InitSiteProp();

    // Get the property Flags
    if (m_psiteprop)
            m_psiteprop->GetProp(PID_INTSITE_FLAGS, &dwPropFlags);
    
    // First check to see if this is a special icon
    // This function returns a usable value for fSpecialUrl even if it returns FALSE
    if(ExtractIconFromWininetCache(pszIconFile, iIcon, &hiconLarge, &hiconSmall, ucIconSize, &fSpecialUrl, dwPropFlags))
    {
        hres = S_OK;
    } 
    else 
    {
        if(TRUE == fSpecialUrl)
        {
            // The extract failed even though this was a special URL
            // we need to revert back to using the default IE icon
            hres = GetGenericURLIcon(szPath, MAX_PATH, (int *)(&iIcon));
            
            if (hres == S_OK)
            {
                fSpecialUrl = FALSE; // It's no longer a special URL
                hres = InitProp();
                if (SUCCEEDED(hres))
                {
                    hres = m_pprop->SetProp(PID_IS_ICONFILE, szPath);
                    if (SUCCEEDED(hres))
                    {
                        hres = m_pprop->SetProp(PID_IS_ICONINDEX, (INT)iIcon);
                    }
                }
            }
            
            if(S_OK != hres)
            {
                ASSERT(0);
                goto DefIcons;
            }
        } 
        else
        {
            StrCpyN(szPath, pszIconFile, ARRAYSIZE(szPath));
            // The path may be munged.  Get the icon index as appropriate.
            if (IsFlagSet(m_dwFlags, ISF_SPECIALICON) && (!fSpecialUrl) )
            {
                // Get the icon location from the munged path
                iIcon = PathParseIconLocation(szPath);

                // cdturner
                // now replace the '*' with the dot
                // this is done for browser only mode to stop the shell hacking the path
                // down to the dll and not calling us
                LPTSTR pszPlus = StrRChr( szPath, NULL, TCHAR('*'));
                if ( pszPlus )
                {
                    *pszPlus = TCHAR('.');
                }
                
                
            }
        }
        
        
        nIndex = iIcon;

        if(!fSpecialUrl)
        {
            if ( WhichPlatform() == PLATFORM_INTEGRATED )
            {
                // Extract the icons 
                CHAR szTempPath[MAX_PATH + 1];
                SHTCharToAnsi(szPath, szTempPath, ARRAYSIZE(szTempPath));
                hres = SHDefExtractIconA(szTempPath, nIndex, 0, &hiconLarge, &hiconSmall, 
                                        ucIconSize);
            }
            else
            {
                // cdturner
                // use a more hacky solution to support browser only mode..
                _InitSysImageLists();
                
                int iIndex = Shell_GetCachedImageIndex( szPath, nIndex, 0 );
                if ( iIndex > 0 )
                {
                    hiconLarge = ImageList_GetIcon( g_himlSysLarge, iIndex, 0 );
                    hiconSmall = ImageList_GetIcon( g_himlSysSmall, iIndex, 0 );

                    hres = NOERROR;
                }
                else 
                {
                    hiconLarge = hiconSmall = NULL;
                    
                    // it will get the windows icon if it should be gleamed, and 
                    // it will the normal icon otherwsie
                    hres = IsFlagSet(dwPropFlags, PIDISF_RECENTLYCHANGED) ? E_FAIL : S_FALSE;
                    goto DefIcons;
                }
            }
        }
    }

    

    if (SUCCEEDED(hres))
    {
        // Has this URL changed recently? 
        if (IsFlagSet(dwPropFlags, PIDISF_RECENTLYCHANGED))
        {
            // Yes 
            URLIMAGES ui;

            if (SUCCEEDED(InitURLImageLists(&ui, hiconLarge, hiconSmall)))
            {
                *phiconLarge = ImageList_GetIcon(ui.himl, 0, INDEXTOOVERLAYMASK(II_OVERLAY_UPDATED));
                *phiconSmall = ImageList_GetIcon(ui.himlSm, 0, INDEXTOOVERLAYMASK(II_OVERLAY_UPDATED));

                DestroyURLImageLists(&ui);

                // these were created, they are not global handles, so they must be cleanedup.
                DestroyIcon( hiconLarge );
                DestroyIcon( hiconSmall );
            }
            else
                goto DefIcons;
        }
        else
        {
            // No
DefIcons:
            *phiconLarge = hiconLarge;
            *phiconSmall = hiconSmall;
        }
    }

    return hres;
}

STDMETHODIMP
Intshcut::Extract(
    IN  LPCTSTR pszIconFile,
    IN  UINT    iIcon,
    OUT HICON * phiconLarge,
    OUT HICON * phiconSmall,
    IN  UINT    ucIconSize)
{
    if (URL_SCHEME_FILE == GetScheme() && _punkLink)
        return IExtractIcon_Extract(_punkLink, pszIconFile, iIcon, phiconLarge, phiconSmall, ucIconSize);
    else
        return _Extract(pszIconFile, iIcon, phiconLarge, phiconSmall, ucIconSize);
}

// Now handle the
// Unicode or Ansi one for the "Other" platform...

STDMETHODIMP
Intshcut::GetIconLocation(UINT uInFlags, LPSTR pszIconFile, UINT cchIconFile,
        PINT pniIcon, PUINT  puOutFlags)
{
    HRESULT hres;
    WCHAR   wszIconFile[MAX_PATH];

    // IconFile is output so...
    // Note, we will only handle up to MAXPATH
    if (cchIconFile > ARRAYSIZE(wszIconFile))
        cchIconFile = ARRAYSIZE(wszIconFile);

    ASSERT(IS_VALID_WRITE_BUFFER(pszIconFile, TCHAR, cchIconFile));
    hres = GetIconLocation(uInFlags, wszIconFile, cchIconFile, pniIcon, puOutFlags);

    if (cchIconFile > 0 && SUCCEEDED(hres))
    {
        WideCharToMultiByte(CP_ACP, 0, wszIconFile, -1, pszIconFile, cchIconFile, NULL, NULL);
    }
    return hres;
}


STDMETHODIMP Intshcut::Extract(IN  LPCSTR pszIconFile, IN  UINT    iIcon,
    OUT HICON * phiconLarge, OUT HICON * phiconSmall, IN  UINT    ucIconSize)
{
    WCHAR wszIconFile[MAX_PATH];

    // First convert the string...
    MultiByteToWideChar(CP_ACP, 0, pszIconFile, -1, wszIconFile, ARRAYSIZE(wszIconFile));

    return Extract(wszIconFile, iIcon, phiconLarge, phiconSmall, ucIconSize);
}
