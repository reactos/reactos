/* Copyright 1996 Microsoft */

#include <priv.h>
#include "sccls.h"
#include "dbgmem.h"
#include "aclisf.h"
#include "shellurl.h"

#define AC_GENERAL          TF_GENERAL + TF_AUTOCOMPLETE

//
// CACLIShellFolder -- An AutoComplete List COM object that
//                  opens an IShellFolder for enumeration.
//

/* IUnknown methods */

HRESULT CACLIShellFolder::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CACLIShellFolder, IEnumString),
        QITABENT(CACLIShellFolder, IACList),
        QITABENT(CACLIShellFolder, IACList2),
        QITABENT(CACLIShellFolder, IShellService),
        QITABENT(CACLIShellFolder, ICurrentWorkingDirectory),
        QITABENT(CACLIShellFolder, IPersistFolder),
        { 0 },
    };
    
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CACLIShellFolder::AddRef(void)
{
    _cRef++;
    return _cRef;
}

ULONG CACLIShellFolder::Release(void)
{
    ASSERT(_cRef > 0);

    _cRef--;

    if (_cRef > 0)
    {
        return _cRef;
    }

    delete this;
    return 0;
}


/* ICurrentWorkingDirectory methods */
HRESULT CACLIShellFolder::SetDirectory(LPCWSTR pwzPath)
{
    HRESULT hr;
    LPITEMIDLIST pidl = NULL;

    hr = IECreateFromPathW(pwzPath, &pidl);
    if (EVAL(SUCCEEDED(hr)))
    {
        hr = Initialize(pidl);
        ILFree(pidl);
    }

    return hr;
}


/* IPersistFolder methods */
HRESULT CACLIShellFolder::Initialize(LPCITEMIDLIST pidl)
{
    HRESULT hr = S_OK;

    hr = _Init();
    if (FAILED(hr))
        return hr;

    if (pidl)
    {
#ifdef DEBUG
        TCHAR szPath[MAX_URL_STRING];
        hr = IEGetNameAndFlags(pidl, SHGDN_FORPARSING, szPath, SIZECHARS(szPath), NULL);
        TraceMsg(AC_GENERAL, "ACListISF::Initialize(%s), SetCurrentWorking Directory happening", szPath);
#endif // DEBUG

        hr = _pshuLocation->SetCurrentWorkingDir(pidl);

        SetDefaultShellPath(_pshuLocation);

    }
    Pidl_Set(&_pidlCWD, pidl);

    return hr;
}

HRESULT CACLIShellFolder::GetClassID(CLSID *pclsid)
{
    *pclsid = CLSID_ACListISF;
    return S_OK;
}


/* IEnumString methods */
HRESULT CACLIShellFolder::Reset(void)
{
    HRESULT hr;
    LPITEMIDLIST pidl = NULL;
    TraceMsg(AC_GENERAL, "ACListISF::Reset()");
    _fExpand = FALSE;
    _nPathIndex = 0;

    hr = _Init();

    // See if we should show hidden files
    SHELLSTATE ss;
    ss.fShowAllObjects = FALSE;
    SHGetSetSettings(&ss, SSF_SHOWALLOBJECTS /*| SSF_SHOWSYSFILES*/, FALSE);
    _fShowHidden = BOOLIFY(ss.fShowAllObjects);
//    _fShowSysFiles = BOOLIFY(ss.fShowSysFiles);

    if (SUCCEEDED(hr) && IsFlagSet(_dwOptions, ACLO_CURRENTDIR))
    {
        // Set the Browser's Current Directory.
        if (_pbs)
        {
            _pbs->GetPidl(&pidl);

            if (pidl)
                Initialize(pidl);
        }

        hr = _SetLocation(pidl);
        if (FAILED(hr))
            hr = S_FALSE;   // If we failed, keep going, we will just end up now doing anything.

        ILFree(pidl);
    }
    
    return hr;
}


// If this is an FTP URL, skip it if:
// 1) It's absolute (has a FTP scheme), and
// 2) it contains a '/' after the server name.
BOOL CACLIShellFolder::_SkipForPerf(LPCWSTR pwzExpand)
{
    BOOL fSkip = FALSE;

    if ((URL_SCHEME_FTP == GetUrlScheme(pwzExpand)))
    {
        // If it's FTP, we want to prevent from hitting the server until
        // after the user has finished AutoCompleting the Server name.
        // Since we can't enum server names, the server names will need
        // to come from the MRU.
        if ((7 >= lstrlen(pwzExpand)) ||                    // There's more than 7 chars "ftp://"
            (NULL == StrChr(&(pwzExpand[7]), TEXT('/'))))   // There is a '/' after the server, "ftp://serv/"
        {
            fSkip = TRUE;
        }
    }

    return fSkip;
}



/* IACList methods */
/****************************************************\
    FUNCTION: Expand

    DESCRIPTION:
        This function will attempt to use the pszExpand
    parameter to bind to a location in the Shell Name Space.
    If that succeeds, this AutoComplete List will then
    contain entries which are the display names in that ISF.
\****************************************************/
HRESULT CACLIShellFolder::Expand(LPCOLESTR pszExpand)
{
    HRESULT hr = S_OK;
    SHSTR strExpand;
    LPITEMIDLIST pidl = NULL;
    DWORD dwParseFlags = SHURL_FLAGS_NOUI;

    TraceMsg(AC_GENERAL, "ACListISF::Expand(%ls)", pszExpand);
    _fExpand = FALSE;
    _nPathIndex = 0;

    strExpand.SetStr(pszExpand);
    hr = Str_SetPtr(&_szExpandStr, strExpand.GetStr());
    ASSERT(SUCCEEDED(hr));

    if (_SkipForPerf(pszExpand)) // Do we want to skip this item for perf reasons?
        hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
    else
        hr = _Init();

    if (FAILED(hr))
        return hr;

    // See if the string points to a location in the Shell Name Space
    hr = _pshuLocation->ParseFromOutsideSource(_szExpandStr, dwParseFlags);
    if (SUCCEEDED(hr))
    {
        // Yes it did, so now AutoComplete from that ISF
        hr = _pshuLocation->GetPidl(&pidl);
        // This may fail if it's something like "ftp:/" and not yet valid".

        DEBUG_CODE(TCHAR szDbgBuffer[MAX_PATH];)
        TraceMsg(AC_GENERAL, "ACListISF::Expand() Pidl=>%s<", Dbg_PidlStr(pidl, szDbgBuffer, SIZECHARS(szDbgBuffer)));
    }

    // Set the ISF that we need to enumerate for AutoComplete.
    hr = _SetLocation(pidl);
    if (pidl)
    {
        ILFree(pidl);


        if (SUCCEEDED(hr))
        {
            _fExpand = TRUE;
        }
    }

    return hr;
}

/* IACList2 methods */
//+-------------------------------------------------------------------------
// Enables/disables various autocomplete features (see ACLO_* flags)
//--------------------------------------------------------------------------
HRESULT CACLIShellFolder::SetOptions(DWORD dwOptions)
{
    _dwOptions = dwOptions;
    return S_OK;
}

//+-------------------------------------------------------------------------
// Returns the current option settings
//--------------------------------------------------------------------------
HRESULT CACLIShellFolder::GetOptions(DWORD* pdwOptions)
{
    HRESULT hr = E_INVALIDARG;
    if (pdwOptions)
    {
        *pdwOptions = _dwOptions;
        hr = S_OK;
    }

    return hr;
}


HRESULT CACLIShellFolder::_GetNextWrapper(LPITEMIDLIST * ppidl, ULONG *pceltFetched)
{
    HRESULT hr = S_OK;
    
    // If this directory (ISF) doesn't contain any more items to enum,
    // then go on to the next directory (ISF) to enum.
    do
    {
        BOOL fFilter;

        do
        {
            fFilter = FALSE;
            hr = _peidl->Next(1, ppidl, pceltFetched);
            if (S_OK == hr)
            {
                hr = _PassesFilter(*ppidl);
                if (FAILED(hr))
                {
                    *pceltFetched = 0;
                    ILFree(*ppidl);
                    *ppidl = NULL;
                    fFilter = TRUE;
                }
            }
        }
        while (fFilter);
    }
    while ((S_OK != hr) && (S_OK == _TryNextPath()));

    return hr;
}


HRESULT CACLIShellFolder::Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
    HRESULT hr = S_OK;
    LPITEMIDLIST pidl;
    ULONG celtFetched;
    BOOL fUsingCachePidl = FALSE;

    *pceltFetched = 0;
    if (!celt)
        return S_OK;

    // If there isn't a Current Working Directory, skip to another
    // Path to enum.
    if (!_peidl)
        hr = _TryNextPath();

    if ((!_peidl) || (!rgelt))
        return S_FALSE;

    // Get the next PIDL.
    if (_pidlInFolder)
    {
        // We have a cached, SHGDN_INFOLDER, so lets try that.
        pidl = _pidlInFolder;
        _pidlInFolder = NULL;
        fUsingCachePidl = TRUE;
        AssertMsg((S_OK == hr), TEXT("CACLIShellFolder::Next() hr doesn't equal S_OK, so we need to call _GetNextWrapper() but we aren't.  Please call BryanSt."));
    }
    else
        hr = _GetNextWrapper(&pidl, &celtFetched);

    if ((S_OK == hr) && (celtFetched))
    {
        STRRET sr;

        // Get the display name of the PIDL.
        if (!fUsingCachePidl)
        {
            hr = _psf->GetDisplayNameOf(pidl, SHGDN_INFOLDER | SHGDN_FORPARSING | SHGDN_FORADDRESSBAR, &sr);

            // some namespaces don't understand _FORADDRESSBAR -- default to IE4 behavior
            if (FAILED(hr))
                hr = _psf->GetDisplayNameOf(pidl, SHGDN_INFOLDER | SHGDN_FORPARSING, &sr);
        }


        if (fUsingCachePidl || FAILED(hr))
        {
            hr = _psf->GetDisplayNameOf(pidl, SHGDN_INFOLDER | SHGDN_FORADDRESSBAR, &sr);

            // some namespaces don't understand _FORADDRESSBAR -- default to IE4 behavior
            if (FAILED(hr))
                hr = _psf->GetDisplayNameOf(pidl, SHGDN_INFOLDER, &sr);
        }

//
// bugbug: This is giving us entries (favorites without .url extension) that cannot be navigated to.
//         So I'm disabling this for IE5B2. (stevepro)
//
//        else
//            Pidl_Set(&_pidlInFolder, pidl);  // We will try (SHGDN_INFOLDER) next time.

        if (SUCCEEDED(hr))
        {
            LPOLESTR pwszPath;

            //
            // Allocate a return buffer (caller will free it).
            //
            pwszPath = (LPOLESTR)CoTaskMemAlloc(MAX_PATH * SIZEOF(WCHAR));

            if (pwszPath)
            {
                LPTSTR pszBegin, pszInsert;
                DWORD cchRemain = MAX_PATH;

#ifdef UNICODE
                pszBegin = pszInsert = pwszPath;
#else //ANSI
                CHAR szTempA[MAX_PATH];
                pszBegin = pszInsert = szTempA;
#endif //ANSI
                *pszBegin = TEXT('\0');

                // Get the display name
                WCHAR szDisplayName[MAX_PATH];
                StrRetToStrN(szDisplayName, ARRAYSIZE(szDisplayName), &sr, pidl);

                // First, prepend the _szExpandStr if necessary.
                // This is needed for sections that don't give
                // the entire path, like "My Computer" items
                // which is (3 == _nPathIndex)
                if (_fExpand && ((_nPathIndex == 0) /*|| (_nPathIndex == 3)*/))
                {
                    DWORD cchExpand = lstrlen(_szExpandStr);

                    // Make sure that for UNC paths the "\\share" is not already
                    // prepended.  NT5 returns the name in this final form.
                    if ((StrCmpNI(szDisplayName, _szExpandStr, cchExpand) != 0) ||
                        (szDisplayName[0] != L'\\') || (szDisplayName[1] != L'\\'))
                    {
                        StrCpyN(pszInsert, _szExpandStr, MAX_PATH);
                        pszInsert += cchExpand;
                        cchRemain -= cchExpand;
                    }
                }
                //
                // Next, append the display name.
                //
                StrCpyN(pszInsert, szDisplayName, cchRemain);
                TraceMsg(AC_GENERAL, "ACListISF::Next() Str=%s, _nPathIndex=%d", pszInsert, _nPathIndex);

                //
                // Finally, put the result into pwszPath.
                //
#ifdef UNICODE
                //
                // As if by magic, the string is already there in UNICODE
                //
#else   // ANSI
                if (MultiByteToWideChar(CP_ACP, 0, pszBegin, -1, pwszPath, MAX_PATH) == 0)
                {
                    ASSERT(0);
                    hr = E_FAIL;
                }
#endif  // ANSI

                if (SUCCEEDED(hr))
                {
                    rgelt[0] = pwszPath;
                    *pceltFetched = 1;
                }
            }
            else
                hr = E_OUTOFMEMORY;
        }
        ILFree(pidl);
    }

    return hr;
}


HRESULT CACLIShellFolder::_PassesFilter(LPCITEMIDLIST pidl)
{
    HRESULT hr = S_OK;
    DWORD dwAttributes = (SFGAO_FILESYSANCESTOR | SFGAO_FILESYSTEM);
    
    if ((ACLO_FILESYSONLY & _dwOptions) &&
        SUCCEEDED(_psf->GetAttributesOf(1, (LPCITEMIDLIST *) &pidl, &dwAttributes)) &&
        !(dwAttributes & (SFGAO_FILESYSANCESTOR | SFGAO_FILESYSTEM)))
    {
#ifdef DEBUG
        STRRET sr;

        if (SUCCEEDED(_psf->GetDisplayNameOf(pidl, 0, &sr)))
        {
            WCHAR szDisplayName[MAX_PATH];

            StrRetToStrN(szDisplayName, ARRAYSIZE(szDisplayName), &sr, pidl);
            TraceMsg(AC_GENERAL, "ACListISF::_PassesFilter() We are skipping\"%ls\" because it doesn't match the filter", szDisplayName);
        }
#endif // DEBUG

        hr = E_FAIL;        // Skip this item.
    }

    return hr;
}


HRESULT CACLIShellFolder::_Init(void)
{
    HRESULT hr = S_OK;

    if (!_pshuLocation)
    {
        _pshuLocation = new CShellUrl();
        if (!_pshuLocation)
            return E_OUTOFMEMORY;

        DbgRemoveFromMemList(_pshuLocation);
    }

    return hr;
}


HRESULT CACLIShellFolder::_SetLocation(LPCITEMIDLIST pidl)
{
    HRESULT hr;

    // Free old location
    ATOMICRELEASE(_peidl);
    ATOMICRELEASE(_psf);

    // Set to new location (Valid if NULL)
    Pidl_Set(&_pidl, pidl);
    if (_pidl)
    {
        hr = IEBindToObject(_pidl, &_psf);
        if (SUCCEEDED(hr))
        {
            DWORD grfFlags = (_fShowHidden ? SHCONTF_INCLUDEHIDDEN : 0) |
                             SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;

            hr = IShellFolder_EnumObjects(_psf, NULL, grfFlags, &_peidl);
            if (hr != S_OK) 
            {
                hr = E_FAIL;    // S_FALSE -> empty enumerator
            }
        }
    }
    else
        hr = E_OUTOFMEMORY;

    if (FAILED(hr))
    {
        // Clear if we could not get all the info
        ATOMICRELEASE(_peidl);
        ATOMICRELEASE(_psf);
        Pidl_Set(&_pidl, NULL);
    }

    //
    // NOTE: This is necessary because this memory is alloced in a ACBG thread, but not
    //       freed until the next call to Reset() or the destructor, which will 
    //       happen in the main thread or another ACBG thread.
    //
    DbgRemoveFromMemList(_peidl);
    DbgRemoveFromMemList(_psf);
    return hr;
}


HRESULT CACLIShellFolder::_TryNextPath(void)
{
    HRESULT hr = S_FALSE;
    if (0 == _nPathIndex)
    {
        _nPathIndex = 1;
        if (_pidlCWD && IsFlagSet(_dwOptions, ACLO_CURRENTDIR))
        {
            hr = _SetLocation(_pidlCWD);
            if (SUCCEEDED(hr))
            {
                goto done;
            }
        }
    }

    if (1 == _nPathIndex)
    {
        _nPathIndex = 2;
        if(IsFlagSet(_dwOptions, ACLO_DESKTOP))
        {
            //  we used to autocomplete g_pidlRoot in the rooted explorer
            //  case, but this was a little weird.  if we want to add this,
            //  we should add ACLO_ROOTED or something.
            
            //  use the desktop...
            hr = _SetLocation(&s_idlNULL);
            if (SUCCEEDED(hr))
            {
                goto done;
            }
        }
    }

    if (2 == _nPathIndex)
    {
        _nPathIndex = 3;
        if (IsFlagSet(_dwOptions, ACLO_MYCOMPUTER))
        {
            LPITEMIDLIST pidlMyComputer;
            SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &pidlMyComputer);

            if (pidlMyComputer)
            {
                hr = _SetLocation(pidlMyComputer);
                ILFree(pidlMyComputer);
                if (SUCCEEDED(hr))
                {
                    goto done;
                }
            }
        }
    }

    // Also search favorites
    if (3 == _nPathIndex)
    {
        _nPathIndex = 4;
        if (IsFlagSet(_dwOptions, ACLO_FAVORITES))
        {
            LPITEMIDLIST pidlFavorites;
            SHGetSpecialFolderLocation(NULL, CSIDL_FAVORITES, &pidlFavorites);

            if (pidlFavorites)
            {
                hr = _SetLocation(pidlFavorites);
                ILFree(pidlFavorites);
                if (SUCCEEDED(hr))
                {
                    goto done;
                }
            }
        }
    }

    if (FAILED(hr))
        hr = S_FALSE;  // This is how we want our errors returned.

done:

    return hr;
}


//================================
// *** IShellService Interface ***

/****************************************************\
    FUNCTION: SetOwner

    DESCRIPTION:
        Update the connection to the Browser window so
    we can always get the PIDL of the current location.
\****************************************************/
HRESULT CACLIShellFolder::SetOwner(IUnknown* punkOwner)
{
    HRESULT hr = S_OK;
    IBrowserService * pbs = NULL;

    ATOMICRELEASE(_pbs);

    if (punkOwner)
        hr = punkOwner->QueryInterface(IID_IBrowserService, (LPVOID *) &pbs);

    if (EVAL(SUCCEEDED(hr)))
        _pbs = pbs;

    return S_OK;
}


/* Constructor / Destructor / CreateInstance */

CACLIShellFolder::CACLIShellFolder()
{
    DllAddRef();
    ASSERT(!_peidl);
    ASSERT(!_psf);
    ASSERT(!_pbs);
    ASSERT(!_pidl);
    ASSERT(!_pidlCWD);
    ASSERT(!_fExpand);
    ASSERT(!_pshuLocation);
    ASSERT(!_szExpandStr);

    _cRef = 1;

    // Default search paths
    _dwOptions = ACLO_CURRENTDIR | ACLO_MYCOMPUTER;
}

CACLIShellFolder::~CACLIShellFolder()
{
    ATOMICRELEASE(_peidl);
    ATOMICRELEASE(_psf);
    ATOMICRELEASE(_pbs);

    Pidl_Set(&_pidl, NULL);
    Pidl_Set(&_pidlCWD, NULL);
    Pidl_Set(&_pidlInFolder, NULL);
    Str_SetPtr(&_szExpandStr, NULL);

    if (_pshuLocation)
        delete _pshuLocation;
    DllRelease();
}

HRESULT CACLIShellFolder_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    *ppunk = NULL;

    CACLIShellFolder *paclSF = new CACLIShellFolder();
    if (paclSF)
    {
        *ppunk = SAFECAST(paclSF, IEnumString *);
        return S_OK;
    }

    return E_OUTOFMEMORY;
}
