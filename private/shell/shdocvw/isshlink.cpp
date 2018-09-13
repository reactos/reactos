/*
 * isshlink.cpp - IShellLink implementation for Intshcut class.
 */



#include "priv.h"
#include "ishcut.h"
#include "resource.h"


#include <mluisupp.h>

/* Types
 ********/

typedef enum isl_getpath_flags
{
    // flag combinations

    ALL_ISL_GETPATH_FLAGS   = (SLGP_SHORTPATH |
                               SLGP_UNCPRIORITY)
}
ISL_GETPATH_FLAGS;

typedef enum isl_resolve_flags
{
    // flag combinations

    ALL_ISL_RESOLVE_FLAGS   = (SLR_NO_UI |
                               SLR_ANY_MATCH |
                               SLR_UPDATE)
}
ISL_RESOLVE_FLAGS;


/********************************** Methods **********************************/


/*----------------------------------------------------------
Purpose: IShellLink::SetPath method for Intshcut

Note:
    1. SetURL clears the internal pidl.

*/
STDMETHODIMP
Intshcut::SetPath(
    LPCTSTR pcszPath)
{
    HRESULT hr;

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT(IS_VALID_STRING_PTR(pcszPath, -1));

    // Treat path as literal URL.

    hr = SetURL(pcszPath, 0);

    return(hr);
}


/*----------------------------------------------------------
Purpose: IShellLink::GetPath handler for Intshcut

*/
STDMETHODIMP
Intshcut::GetPath(
    IN  LPTSTR           pszBuf,        
    IN  int              cchBuf,
    OUT PWIN32_FIND_DATA pwfd,          OPTIONAL
    IN  DWORD            dwFlags)
{
    HRESULT hres = E_FAIL;

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT(IS_VALID_WRITE_BUFFER(pszBuf, TCHAR, cchBuf));
    ASSERT(NULL == pwfd || IS_VALID_WRITE_PTR(pwfd, WIN32_FIND_DATA));
    ASSERT(FLAGS_ARE_VALID(dwFlags, ALL_ISL_GETPATH_FLAGS));

    // Init to default values
    if (pwfd)
        ZeroMemory(pwfd, SIZEOF(*pwfd));

    if (cchBuf > 0)
        *pszBuf = '\0';

    // Ignore dwFlags.

    hres = InitProp();
    if (SUCCEEDED(hres))
        hres = m_pprop->GetProp(PID_IS_URL, pszBuf, cchBuf);
     
    return hres;
}


/*----------------------------------------------------------
Purpose: IShellLink::SetRelativePath method for Intshcut

*/
STDMETHODIMP Intshcut::SetRelativePath(LPCTSTR pcszRelativePath, DWORD dwReserved)
{
    HRESULT hr;

    // dwReserved may be any value.

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT(IS_VALID_STRING_PTR(pcszRelativePath, -1));

    hr = E_NOTIMPL;

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));

    return(hr);
}


/*----------------------------------------------------------
Purpose: IShellLink::SetIDList method for Intshcut

Note:
    1. SetIDList also does SetPath implicitly to update the path (URL)
        to match the pidl.
    2. SetPath only clears the pidl to NULL, so internally we know
        if we really have a pidl for the shortcut. Although GetIDList
        will generate a pidl from path (URL) if we don't have a pidl.

*/
STDMETHODIMP Intshcut::SetIDList(LPCITEMIDLIST pcidl)
{
    HRESULT hr;

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT(IS_VALID_READ_PTR(pcidl, ITEMIDLIST));

    hr = InitProp();
    if (SUCCEEDED(hr))
    {
        hr = m_pprop->SetIDListProp(pcidl);
        if (SUCCEEDED(hr))
        {
            // if the pidl was set successfully, update the path.
            TCHAR szURL[INTERNET_MAX_URL_LENGTH];
            
            hr = IEGetDisplayName(pcidl, szURL, SHGDN_FORPARSING);
            if (SUCCEEDED(hr))
                m_pprop->SetURLProp(szURL, 0);
        }
    }

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));

    return(hr);
}


/*----------------------------------------------------------
Purpose: Get the original pidl set by SetIDList.

Note:
    1. Do not generate a pidl from path if we don't have a pidl.
    2. Return S_OK if we have a pidl, caller must NOT check for
        SUCCEEDED() return.

*/
STDMETHODIMP Intshcut::GetIDListInternal(LPITEMIDLIST *ppidl)
{
    HRESULT hres = InitProp();
    if (SUCCEEDED(hres))
    {
        IStream *pStream;
        hres = m_pprop->GetProp(PID_IS_IDLIST, &pStream);
        if ((hres == S_OK) && pStream)
        {
            const LARGE_INTEGER li = {0, 0};
            // reset the seek pointer                                           
            hres = pStream->Seek(li, STREAM_SEEK_SET, NULL);
            if (SUCCEEDED(hres))
                hres = ILLoadFromStream(pStream, ppidl);
        
            pStream->Release();
        }
    }
    return hres;
}


    
/*----------------------------------------------------------
Purpose: IShellLink::GetIDList method for Intshcut

Note:
    1. If we don't have a pidl from SetIDList, generate a pidl
        from path.

*/
STDMETHODIMP Intshcut::GetIDList(LPITEMIDLIST *ppidl)
{
    HRESULT hres;
    ASSERT(IS_VALID_WRITE_PTR(ppidl, LPITEMIDLIST));
    
    if (!ppidl)
        return E_INVALIDARG;

    *ppidl = NULL;

    hres = InitProp();
    if (SUCCEEDED(hres))
    {
        // check if it already as a pidl.
        hres = GetIDListInternal(ppidl);
        if (hres != S_OK)
        {
            // it doesn't have a pidl, get the URL and make a pidl.
            TCHAR szURL[INTERNET_MAX_URL_LENGTH];
    
            hres = m_pprop->GetProp(PID_IS_URL, szURL, ARRAYSIZE(szURL));
            if (SUCCEEDED(hres)) 
            {
                hres = IECreateFromPath(szURL, ppidl);
            }
        }
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellLink::SetDescription method for Intshcut

*/
STDMETHODIMP Intshcut::SetDescription(LPCTSTR pcszDescription)
{
    HRESULT hr;
    BOOL bDifferent;

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT(IS_VALID_STRING_PTR(pcszDescription, -1));

    // Set m_pszFile to description.

    bDifferent = (! m_pszDescription ||
                  StrCmp(pcszDescription, m_pszDescription) != 0);

    if (Str_SetPtr(&m_pszDescription, pcszDescription))
    {
        if (bDifferent)
           Dirty(TRUE);

        hr = S_OK;
    }
    else
        hr = E_OUTOFMEMORY;

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));

    return(hr);
}

STDMETHODIMP Intshcut::_ComputeDescription()
{
    HRESULT hres;
    BSTR bstrTitle = NULL;

    if (_punkSite)
    {
        // Get the title element
        IWebBrowser *pwb;
        hres = _punkSite->QueryInterface(IID_IWebBrowser, (void **)&pwb);
        if (S_OK == hres)
        {
            IDispatch *pDisp;
            hres = pwb->get_Document(&pDisp);
            if (S_OK == hres)
            {
                IHTMLDocument2 *pDoc;
                hres = pDisp->QueryInterface(IID_IHTMLDocument2, (void **)&pDoc);
                if (S_OK == hres)
                {
                    hres = pDoc->get_title(&bstrTitle);
                    pDoc->Release();
                }
                pDisp->Release();
            }
            pwb->Release();
        }
    }
    
    TCHAR *pszUrl;  // The url for this shortcut
    hres = GetURL(&pszUrl);
    if (S_OK == hres)
    {
        TCHAR szDescription[MAX_PATH], *pszTitle = NULL;
        pszTitle = bstrTitle;     


        // We gamble that the URL will always have displayable characters. 
        // This is a bad assumption but if this assumption is violated then
        // there is a good chance that the URL probably cannot even
        // be navigated to
        
        // This description is used as the name of the file verbatim
        // during drag drop - hence it should look like a .url file name

        GetShortcutFileName(pszUrl, pszTitle, NULL, szDescription, ARRAYSIZE(szDescription));
        //PathYetAnotherMakeUniqueName(szTempFileName, szTempFileName, NULL, NULL);
        PathCleanupSpec(NULL, szDescription);

        // Sometimes PathCleanupSpec can end up simply mangling the description if
        // it cannot properly convert the title to ANSI
        // hence we check that we have a proper description

        
        
        if((0 == *szDescription) || (0 == StrCmp(szDescription,TEXT(".url"))))
        {
            // recompute the description without the title
            GetShortcutFileName(pszUrl, NULL, NULL, szDescription, ARRAYSIZE(szDescription));
            PathCleanupSpec(NULL, szDescription);
        }
        hres = SetDescription(szDescription);
        SHFree(pszUrl);
    }


    SysFreeString(bstrTitle);
        
    return hres;
}

// IShellLink::GetDescription method for Intshcut
STDMETHODIMP Intshcut::GetDescription(LPTSTR pszDescription, int cchBuf)
{
    HRESULT hr;

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT(IS_VALID_WRITE_BUFFER(pszDescription, TCHAR, cchBuf));

    // Get description from m_pszDescription.

    if (NULL == m_pszDescription)
    {
        _ComputeDescription();
    }

    if (m_pszDescription)
        StrCpyN(pszDescription, m_pszDescription, cchBuf);
    else if (m_pszFile)
    {
        StrCpyN(pszDescription, m_pszFile, cchBuf);
    }
    else
    {
        // use default shortcut name 
        MLLoadString(IDS_NEW_INTSHCUT, pszDescription, cchBuf);
    }

    hr = S_OK;

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT(hr == S_OK &&
           (cchBuf <= 0 ||
            (IS_VALID_STRING_PTR(pszDescription, -1) &&
             EVAL(lstrlen(pszDescription) < cchBuf))));

    return(hr);
}


// IShellLink::SetArguments method for Intshcut
STDMETHODIMP Intshcut::SetArguments(LPCTSTR pcszArgs)
{
    return E_NOTIMPL;
}

// IShellLink::GetArguments for Intshcut
STDMETHODIMP Intshcut::GetArguments(LPTSTR pszArgs, int cchBuf)
{
    return E_NOTIMPL;
}


// IShellLink::SetWorkingDirectory handler for Intshcut
STDMETHODIMP Intshcut::SetWorkingDirectory(LPCTSTR pcszWorkingDirectory)
{
    HRESULT hres = S_OK;
    TCHAR rgchNewPath[MAX_PATH];
    BOOL bChanged = FALSE;

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT(! pcszWorkingDirectory ||
           IS_VALID_STRING_PTR(pcszWorkingDirectory, -1));

    if (! AnyMeat(pcszWorkingDirectory))
        pcszWorkingDirectory = NULL;

    if (pcszWorkingDirectory)
    {
        LPTSTR pszFileName;

        if (GetFullPathName(pcszWorkingDirectory, SIZECHARS(rgchNewPath),
                            rgchNewPath, &pszFileName) > 0)
            pcszWorkingDirectory = rgchNewPath;
        else
            hres = E_PATH_NOT_FOUND;
    }

    if (hres == S_OK)
    {
        TCHAR szDir[MAX_PATH];

        hres = InitProp();
        if (SUCCEEDED(hres))
        {
            hres = m_pprop->GetProp(PID_IS_WORKINGDIR, szDir, SIZECHARS(szDir));

            bChanged = ! ((! pcszWorkingDirectory && S_FALSE == hres) ||
                          (pcszWorkingDirectory && S_OK == hres &&
                           ! StrCmp(pcszWorkingDirectory, szDir)));

            hres = S_OK;
            if (bChanged)
            {
                hres = m_pprop->SetProp(PID_IS_WORKINGDIR, pcszWorkingDirectory);
            }
        }
    }

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellLink::GetWorkingDirectory handler for Intshcut

*/
STDMETHODIMP
Intshcut::GetWorkingDirectory(
    IN LPTSTR pszBuf,
    IN int    cchBuf)
{
    HRESULT hres;

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT(IS_VALID_WRITE_BUFFER(pszBuf, TCHAR, cchBuf));

    if (cchBuf > 0)
        *pszBuf = '\0';

    hres = InitProp();
    if (SUCCEEDED(hres))
        hres = m_pprop->GetProp(PID_IS_WORKINGDIR, pszBuf, cchBuf);

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellLink::SetHotkey handler for Intshcut

*/
STDMETHODIMP
Intshcut::SetHotkey(
    IN WORD wHotkey)
{
    HRESULT hres;

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));

    hres = InitProp();
    if (SUCCEEDED(hres))
        hres = m_pprop->SetProp(PID_IS_HOTKEY, wHotkey);

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellLink::GetHotkey handler for Intshcut

*/
STDMETHODIMP
Intshcut::GetHotkey(
    PWORD pwHotkey)
{
    HRESULT hres;

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT(IS_VALID_WRITE_PTR(pwHotkey, WORD));

    hres = InitProp();
    if (SUCCEEDED(hres))
    {
        m_pprop->GetProp(PID_IS_HOTKEY, pwHotkey);
        hres = S_OK;
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellLink::SetShowCmd handler for Intshcut

*/
STDMETHODIMP
Intshcut::SetShowCmd(
    IN int nShowCmd)
{
    HRESULT hres;

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT(IsValidShowCmd(nShowCmd));

    hres = InitProp();
    if (SUCCEEDED(hres))
        hres = m_pprop->SetProp(PID_IS_SHOWCMD, nShowCmd);

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellLink::GetShowCmd handler for Intshcut

*/
STDMETHODIMP
Intshcut::GetShowCmd(
    OUT int *pnShowCmd)
{
    HRESULT hres;

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT(IS_VALID_WRITE_PTR(pnShowCmd, INT));

    hres = InitProp();
    if (SUCCEEDED(hres))
    {
        hres = m_pprop->GetProp(PID_IS_SHOWCMD, pnShowCmd);
        if (S_OK != hres)
            *pnShowCmd = SW_NORMAL;
        hres = S_OK;
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: IShellLink::SetIconLocation handler for Intshcut

*/
STDMETHODIMP
Intshcut::SetIconLocation(
    IN LPCTSTR pszFile,
    IN int     niIcon)
{
    HRESULT hres = S_OK;
    BOOL bNewMeat;
    TCHAR szNewPath[MAX_PATH];

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT(IsValidIconIndex(pszFile ? S_OK : S_FALSE, pszFile, MAX_PATH, niIcon));

    bNewMeat = AnyMeat(pszFile);

    if (bNewMeat)
    {
        if (PathSearchAndQualify(pszFile, szNewPath, SIZECHARS(szNewPath)))
        {
            hres = S_OK;
        }
        else
        {
            hres = E_FILE_NOT_FOUND;
        }
    }

    if (hres == S_OK)
    {
        TCHAR szOldPath[MAX_PATH];
        int niOldIcon;
        UINT uFlags;

        hres = GetIconLocation(0, szOldPath, SIZECHARS(szOldPath), &niOldIcon,
                             &uFlags);

        if (SUCCEEDED(hres))
        {
            BOOL bOldMeat;
            BOOL bChanged = FALSE;

            bOldMeat = AnyMeat(szOldPath);

            ASSERT(! *szOldPath ||
                   bOldMeat);

            bChanged = ((! bOldMeat && bNewMeat) ||
                        (bOldMeat && ! bNewMeat) ||
                        (bOldMeat && bNewMeat &&
                         (StrCmp(szOldPath, szNewPath) != 0 ||
                          niIcon != niOldIcon)));

            hres = S_OK;
            if (bChanged && bNewMeat)
            {
                hres = InitProp();
                if (SUCCEEDED(hres))
                {
                    hres = m_pprop->SetProp(PID_IS_ICONFILE, szNewPath);
                    if (SUCCEEDED(hres))
                        hres = m_pprop->SetProp(PID_IS_ICONINDEX, niIcon);
                }
            }
        }
    }

    return hres;
}

VOID UrlMunge(
    TCHAR *lpszSrc,
    TCHAR *lpszDest,
    UINT   cchDestBufSize,
    BOOL fRecentlyChanged)
{
   TCHAR *lpszTemp = lpszSrc;

   if(fRecentlyChanged)
        cchDestBufSize--; // Save up a character

   while(*lpszTemp != TEXT('\0') && (cchDestBufSize > 1)) // not End of line and save up one char for \0 in munged string
   {
        if(TEXT('/') == *lpszTemp)
        {
            *lpszDest = TEXT('\1');
        }
        else
        {
            *lpszDest = *lpszTemp;
        }
        lpszDest++;
        lpszTemp++;
        cchDestBufSize--;
   }
   if(fRecentlyChanged)
   {
        *lpszDest = TEXT('\2');  
        lpszDest++;
   }
   *lpszDest =  TEXT('\0');
   return;
}


HRESULT HelperForReadIconInfoFromPropStg(
    IN  LPTSTR pszBuf,
    IN  int    cchBuf,
    OUT int *  pniIcon,
    IPropertyStorage *pPropStg,
    PROPSPEC *ppropspec,
    IN  LPTSTR pszActualUrlBuf,
    IN INT cchActualUrlBuf,
    BOOL fRecentlyChanged)
{

    HRESULT hres;
    PROPVARIANT rgpropvar[2];


    ASSERT((0 == pszActualUrlBuf) || (cchActualUrlBuf >= MAX_URL_STRING));

    if(pszActualUrlBuf)
        *pszActualUrlBuf = TEXT('\0');
        
    // Init to default values
    *pniIcon = 0;
    if (cchBuf > 0)
        *pszBuf = TEXT('\0');

    

    hres = pPropStg->ReadMultiple(2, ppropspec, rgpropvar);
    if (SUCCEEDED(hres))
    {
        if (VT_LPWSTR == rgpropvar[1].vt)
        {
            if(FALSE == PathFileExistsW(rgpropvar[1].pwszVal))
            {
                UrlMunge(rgpropvar[1].pwszVal, pszBuf, cchBuf, fRecentlyChanged);  
            }
            else
            {
                // We will just send the icon file and index back with no attempt
                // to hash it or fill out the URL field
                if(lstrlenW(rgpropvar[1].pwszVal) >= cchBuf)
                {
                     // need a larger buf - simply fail it
                    hres = E_FAIL;
                }
                else
                {
                    StrCpyN(pszBuf, rgpropvar[1].pwszVal, cchBuf);
                }
            }
            if(SUCCEEDED(hres) && pszActualUrlBuf)
            {
                StrCpyN(pszActualUrlBuf, rgpropvar[1].pwszVal, cchActualUrlBuf);
            }
        }

        if (VT_I4 == rgpropvar[0].vt)
            *pniIcon = rgpropvar[0].lVal;

        FreePropVariantArray(ARRAYSIZE(rgpropvar), rgpropvar);
    }
    return hres;
}

//
// Functions from isexicon.cpp
//

/*----------------------------------------------------------
*
*
Purpose: IShellLink::GetIconLocation handler for Intshcut
*
*----------------------------------------------------------*/
STDMETHODIMP
Intshcut::_GetIconLocationWithURLHelper(
    IN  LPTSTR pszBuf,
    IN  int    cchBuf,
    OUT int *  pniIcon,
    IN  LPTSTR pszActualUrl,
    UINT cchActualUrlBuf,
    BOOL fRecentlyChanged)
{
    HRESULT hres;
    PROPSPEC rgpropspec[2];

    ASSERT(IS_VALID_STRUCT_PTR(this, CIntshcut));
    ASSERT(IS_VALID_WRITE_BUFFER(pszBuf, TCHAR, cchBuf));
    ASSERT(IS_VALID_WRITE_PTR(pniIcon, int));

    if(!pszBuf)
        return E_INVALIDARG;

    rgpropspec[0].ulKind = PRSPEC_PROPID;
    rgpropspec[1].ulKind = PRSPEC_PROPID;

    
    if(pszActualUrl)
        *pszActualUrl = TEXT('\0');
        
    *pszBuf = TEXT('\0');
    
    hres = InitProp();
    if (SUCCEEDED(hres))
    {
        rgpropspec[0].propid = PID_IS_ICONINDEX;
        rgpropspec[1].propid = PID_IS_ICONFILE;
        hres = HelperForReadIconInfoFromPropStg(
                            pszBuf, cchBuf, pniIcon, m_pprop, 
                            rgpropspec, pszActualUrl, cchActualUrlBuf,
                            fRecentlyChanged);
        
    }

    if(TEXT('\0') == *pszBuf) 
    {
        // Didn't find it in the shortcut itself
        // Poke around the intsite database and if it is there,
        // simply stuff it into the shortcut file if you do find
        // one
        IPropertyStorage *ppropstg = NULL;
        hres = Open(FMTID_InternetSite, STGM_READWRITE, &ppropstg);
        if(S_OK == hres)
        {
            // Look for an icon for this specific url
            ASSERT(ppropstg);
            rgpropspec[0].propid = PID_INTSITE_ICONINDEX;
            rgpropspec[1].propid = PID_INTSITE_ICONFILE;
            hres = HelperForReadIconInfoFromPropStg(pszBuf, cchBuf, pniIcon, 
                                                    ppropstg, rgpropspec, pszActualUrl, 
                                                    cchActualUrlBuf, fRecentlyChanged);
            
            
            ppropstg->Release();
        }

        if((S_OK == hres) && (*pszBuf) && pszActualUrl && (*pszActualUrl))
        {
            // Write this info to the shortcut file
            WCHAR *pwszTempBuf;
            pwszTempBuf = pszActualUrl;
            PROPVARIANT var = {0};

            ASSERT(1 == *pniIcon);
            
            var.vt =  VT_BSTR;
            var.bstrVal = SysAllocString(pwszTempBuf);

            if(var.bstrVal)
            {
                hres = WritePropertyNPB(ISHCUT_INISTRING_SECTIONW, ISHCUT_INISTRING_ICONFILEW,
                                            &var);

                SysFreeString(var.bstrVal);
                if(S_OK == hres)
                {
                    var.bstrVal = SysAllocString(L"1");
                    if(var.bstrVal)
                    {
                        hres = WritePropertyNPB(ISHCUT_INISTRING_SECTIONW, ISHCUT_INISTRING_ICONINDEXW,
                                                    &var);
                        SysFreeString(var.bstrVal);
                    }
                }
            } 
            hres = S_OK; // retun OK if you found icon and could not write out for whatever reason
        }
    }

    return hres;
}

// IShellLink::GetIconLocation handler for Intshcut
STDMETHODIMP Intshcut::GetIconLocation(LPTSTR pszBuf, int cchBuf, int *pniIcon)
{
    UINT uTmp;
    return GetIconLocation(0, pszBuf, cchBuf, pniIcon, &uTmp);
}

// IShellLink::Resolve method for Intshcut
STDMETHODIMP Intshcut::Resolve(HWND hwnd,  DWORD dwFlags)
{
    return S_OK;
}

//====================================================================================
// Now the A or W functions that depend on unicode or ansi machines...
// Will setup forwarders to the native one for the OS...
//----------------------------------------------------------
STDMETHODIMP Intshcut::SetPath(LPCSTR pcszPath)
{
    WCHAR wszT[INTERNET_MAX_URL_LENGTH];

    if (!pcszPath)
        return SetPath((LPCWSTR)NULL);

    SHAnsiToUnicode(pcszPath, wszT, ARRAYSIZE(wszT));
    return SetPath(wszT);
}


STDMETHODIMP Intshcut::GetPath(LPSTR pszBuf, int cchBuf, PWIN32_FIND_DATAA pwfd, DWORD dwFlags)
{
    WCHAR wszT[INTERNET_MAX_URL_LENGTH];
    HRESULT hres;

    // Init to default values (Note pwfd is not actually set so don't worry about thunking...
    if (pwfd)
        ZeroMemory(pwfd, SIZEOF(*pwfd));

    hres = GetPath(wszT, ARRAYSIZE(wszT), NULL, dwFlags);
    if (SUCCEEDED(hres))
        SHUnicodeToAnsi(wszT, pszBuf, cchBuf);
    return hres;
}


STDMETHODIMP Intshcut::SetRelativePath(LPCSTR pcszRelativePath, DWORD dwReserved)
{
    WCHAR wszT[MAX_PATH];
    if (!pcszRelativePath)
        return SetRelativePath((LPCWSTR)NULL, dwReserved);

    SHAnsiToUnicode(pcszRelativePath, wszT, ARRAYSIZE(wszT));
    return SetRelativePath(wszT, dwReserved);
}


STDMETHODIMP Intshcut::SetDescription(LPCSTR pcszDescription)
{
    WCHAR wszT[MAX_PATH];
    if (!pcszDescription)
        return SetDescription((LPCWSTR)NULL);

    SHAnsiToUnicode(pcszDescription, wszT, ARRAYSIZE(wszT));
    return SetDescription(wszT);
}

STDMETHODIMP Intshcut::GetDescription(LPSTR pszDescription,int cchBuf)
{
    WCHAR wszT[MAX_PATH];
    HRESULT hres;

    hres = GetDescription(wszT, ARRAYSIZE(wszT));
    if (SUCCEEDED(hres))
        SHUnicodeToAnsi(wszT, pszDescription, cchBuf);
    return hres;
}

STDMETHODIMP Intshcut::SetArguments(LPCSTR pcszArgs)
{
    WCHAR wszT[2*MAX_PATH];
    if (!pcszArgs)
        return SetArguments((LPCWSTR)NULL);

    SHAnsiToUnicode(pcszArgs, wszT, ARRAYSIZE(wszT));
    return SetArguments(wszT);
}


STDMETHODIMP Intshcut::GetArguments(LPSTR pszArgs,int cchBuf)
{
    WCHAR wszT[2*MAX_PATH];
    HRESULT hres;

    hres = GetArguments(wszT, ARRAYSIZE(wszT));
    if (SUCCEEDED(hres))
        SHUnicodeToAnsi(wszT, pszArgs, cchBuf);
    return hres;
} 

STDMETHODIMP Intshcut::SetWorkingDirectory(LPCSTR pcszWorkingDirectory)
{
    WCHAR wszT[MAX_PATH];

    if (!pcszWorkingDirectory)
        return SetWorkingDirectory((LPCWSTR)NULL);

    SHAnsiToUnicode(pcszWorkingDirectory, wszT, ARRAYSIZE(wszT));
    return SetWorkingDirectory(wszT);
}

STDMETHODIMP Intshcut::GetWorkingDirectory(LPSTR pszBuf, int cchBuf)
{
    WCHAR wszT[MAX_PATH];
    HRESULT hres;

    hres = GetWorkingDirectory(wszT, ARRAYSIZE(wszT));
    if (SUCCEEDED(hres))
        SHUnicodeToAnsi(wszT, pszBuf, cchBuf);
    return hres;
}

STDMETHODIMP Intshcut::SetIconLocation(LPCSTR pszFile, int niIcon)
{
    WCHAR wszT[MAX_PATH];

    if (!pszFile)
        return SetIconLocation((LPCWSTR)NULL, niIcon);

    SHAnsiToUnicode(pszFile, wszT, ARRAYSIZE(wszT));
    return SetIconLocation(wszT, niIcon);
}

STDMETHODIMP Intshcut::GetIconLocation(LPSTR pszBuf, int cchBuf, int *pniIcon)
{
    WCHAR wszT[MAX_PATH];
    HRESULT hres;

    hres = GetIconLocation(wszT, ARRAYSIZE(wszT), pniIcon);
    if (SUCCEEDED(hres))
        SHUnicodeToAnsi(wszT, pszBuf, cchBuf);
    return hres;
}

