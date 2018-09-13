/*****************************************************************************
 *
 *    ftppl.cpp - FTP LPITEMIDLIST List object
 *
 *****************************************************************************/

#include "priv.h"
#include "ftppl.h"
#include "ftpurl.h"

typedef struct tagINETENUM
{
    HINTERNET               hint;
    BOOL *                  pfValidhinst;
    LPVOID                  pvData;
    LPFNPROCESSITEMCB       pfnProcessItemCB;
    LPCITEMIDLIST           pidlRoot;
    HRESULT                 hr;
} INETENUM;


/*****************************************************************************\
     FUNCTION: RecursiveEnum
 
    DESCRIPTION:
        This function will pack the parameters needed during the enum.
\*****************************************************************************/
HRESULT CFtpPidlList::RecursiveEnum(LPCITEMIDLIST pidlRoot, LPFNPROCESSITEMCB pfnProcessItemCB, HINTERNET hint, LPVOID pvData)
{
    INETENUM inetEnum = {hint, NULL, pvData, pfnProcessItemCB, pidlRoot, S_OK};

    Enum(RecursiveProcessPidl, (LPVOID) &inetEnum);

    return inetEnum.hr;
}


// lParam can be: 0 == do a case sensitive search.  1 == do a case insensitive search.
int CFtpPidlList::ComparePidlName(LPVOID pvPidl1, LPVOID pvPidl2, LPARAM lParam)
{
    DWORD dwFlags = FCMP_NORMAL;

    if (lParam)
        dwFlags |= FCMP_CASEINSENSE;

    // return < 0 for pvPidl1 before pvPidl2.
    // return == 0 for pvPidl1 equals pvPidl2.
    // return > 0 for pvPidl1 after pvPidl2.
    return FtpItemID_CompareIDsInt(COL_NAME, (LPCITEMIDLIST)pvPidl1, (LPCITEMIDLIST)pvPidl2, dwFlags);
}


HRESULT CFtpPidlList::InsertSorted(LPCITEMIDLIST pidl)
{
    m_pfl->InsertSorted(ILClone(pidl), CFtpPidlList::ComparePidlName, FALSE /*Case Insensitive*/);
    return S_OK;
};


int CFtpPidlList::FindPidlIndex(LPCITEMIDLIST pidlToFind, BOOL fCaseInsensitive)
{
    return m_pfl->SortedSearch((LPVOID) pidlToFind, CFtpPidlList::ComparePidlName, (LPARAM)fCaseInsensitive, DPAS_SORTED);
}


LPITEMIDLIST CFtpPidlList::FindPidl(LPCITEMIDLIST pidlToFind, BOOL fCaseInsensitive)
{
    LPITEMIDLIST pidlFound = NULL;
    int nIndex = FindPidlIndex(pidlToFind, fCaseInsensitive);

    if (-1 != nIndex)
    {
        pidlFound = ILClone(GetPidl(nIndex));
    }

    return pidlFound;
}


HRESULT CFtpPidlList::CompareAndDeletePidl(LPCITEMIDLIST pidlToDelete)
{
    HRESULT hr = S_FALSE;
    int nIndex = FindPidlIndex(pidlToDelete, FALSE /*Case Insensitive*/);

    if (-1 != nIndex)
    {
        LPITEMIDLIST pidlCurrent = GetPidl((UINT)nIndex);
        if (EVAL(pidlCurrent))
        {
            ASSERT(0 == FtpItemID_CompareIDsInt(COL_NAME, pidlCurrent, pidlToDelete, FCMP_NORMAL));
            m_pfl->DeletePtrByIndex(nIndex);
            ILFree(pidlCurrent);    // Deallocate the memory
            hr = S_OK;  // Found and deleted.
        }
    }

    return hr;
}


void CFtpPidlList::Delete(int nIndex)
{
    LPITEMIDLIST pidlToDelete = GetPidl(nIndex);

    ILFree(pidlToDelete);   // Free the memory.
    m_pfl->DeletePtrByIndex(nIndex);
}


HRESULT CFtpPidlList::ReplacePidl(LPCITEMIDLIST pidlSrc, LPCITEMIDLIST pidlDest)
{
    HRESULT hr = S_FALSE;
    int nIndex = FindPidlIndex(pidlSrc, FALSE);

    if (-1 != nIndex)
    {
        LPITEMIDLIST pidlCurrent = GetPidl((UINT)nIndex);
        if (EVAL(pidlCurrent))
        {
            ASSERT(0 == FtpItemID_CompareIDsInt(COL_NAME, pidlCurrent, pidlSrc, FCMP_NORMAL));
            ILFree(pidlCurrent);    // Deallocate the memory
            m_pfl->DeletePtrByIndex(nIndex);
            InsertSorted(pidlDest);         // This function does the ILClone()
            hr = S_OK;  // Found and deleted.
        }
    }

    return hr;
}

void CFtpPidlList::AssertSorted(void)
{
#ifdef DEBUG
    // For perf reasons, we need to keep this list in order.
    // This is mainly because parse display name looks thru
    // the list, so we want that to be fast.
    for (int nIndex = (GetCount() - 2); (nIndex >= 0); nIndex--)
    {
        LPITEMIDLIST pidl1 = GetPidl((UINT)nIndex);
        LPITEMIDLIST pidl2 = GetPidl((UINT)nIndex + 1);

        // Assert that pidl1 comes before pidl2.
        if (!EVAL(0 >= FtpItemID_CompareIDsInt(COL_NAME, pidl1, pidl2, FCMP_NORMAL)))
        {
            TCHAR szPidl1[MAX_PATH];
            TCHAR szPidl2[MAX_PATH];

            if (FtpID_IsServerItemID(pidl1))
                FtpPidl_GetServer(pidl1, szPidl1, ARRAYSIZE(szPidl1));
            else
                FtpPidl_GetDisplayName(pidl1, szPidl1, ARRAYSIZE(szPidl1));

            if (FtpID_IsServerItemID(pidl2))
                FtpPidl_GetServer(pidl2, szPidl2, ARRAYSIZE(szPidl2));
            else
                FtpPidl_GetDisplayName(pidl2, szPidl2, ARRAYSIZE(szPidl2));

            TraceMsg(TF_ERROR, "CFtpPidlList::AssertSorted() '%s' & '%s' where found out of order", szPidl1, szPidl2);
        }
        // We do NOT need to free pidl1 or pidl2 because we get a pointer to someone else's copy.
    }

#endif // DEBUG
}


void CFtpPidlList::TraceDump(LPCITEMIDLIST pidl, LPCTSTR pszCaller)
{
#ifdef DEBUG
/*
    TCHAR szUrl[MAX_URL_STRING];

    UrlCreateFromPidl(pidl, SHGDN_FORPARSING, szUrl, ARRAYSIZE(szUrl), ICU_USERNAME, FALSE);
    TraceMsg(TF_PIDLLIST_DUMP, "CFtpPidlList::TraceDump() root is '%s', called from '%s'", szUrl, pszCaller);

    // Let's look at the contents.
    for (int nIndex = (GetCount() - 1); (nIndex >= 0); nIndex--)
    {
        LPITEMIDLIST pidlFull = ILCombine(pidl, GetPidl((UINT)nIndex));

        if (pidlFull)
        {
            UrlCreateFromPidl(pidlFull, SHGDN_FORPARSING, szUrl, ARRAYSIZE(szUrl), ICU_USERNAME, FALSE);
            TraceMsg(TF_PIDLLIST_DUMP, "CFtpPidlList::TraceDump() Index=%d, url=%s", nIndex, szUrl);
            ILFree(pidlFull);
        }
    }
*/
#endif // DEBUG
}

void CFtpPidlList::UseCachedDirListings(BOOL fUseCachedDirListings)
{
    // Normally we do two passes in the tree walker code.  The first
    // pass is to count up the time required to do the download. We
    // normally force WININET to not use cached results because someone
    // else could have changed the contents on the server.
    // On the second pass, we normally do the work (upload, download, delete)
    // and we want to use the cached results to get the perf advantage
    // and the results shouldn't be more than a minute out of date.

    if (fUseCachedDirListings)
        m_dwInetFlags = INTERNET_NO_CALLBACK;
    else
        m_dwInetFlags = (INTERNET_NO_CALLBACK | INTERNET_FLAG_RESYNCHRONIZE | INTERNET_FLAG_RELOAD);
}

BOOL CFtpPidlList::AreAllFolders(void)
{
    BOOL fAllFolder = TRUE;

    for (int nIndex = (GetCount() - 1); fAllFolder && (nIndex >= 0); nIndex--)
    {
        LPITEMIDLIST pidl = GetPidl((UINT)nIndex);
        if (EVAL(pidl))
            fAllFolder = FtpPidl_IsDirectory(pidl, TRUE);

        // We do NOT need to free pidl because we get a pointer to someone else's copy.
    }

    return fAllFolder;
}


BOOL CFtpPidlList::AreAllFiles(void)
{
    BOOL fAllFiles = TRUE;

    for (int nIndex = (GetCount() - 1); fAllFiles && (nIndex >= 0); nIndex--)
    {
        LPITEMIDLIST pidl = GetPidl((UINT)nIndex);
        if (EVAL(pidl))
            fAllFiles = !FtpPidl_IsDirectory(pidl, TRUE);

        // We do NOT need to free pidl because we get a pointer to someone else's copy.
    }

    return fAllFiles;
}


/*****************************************************************************
 *
 *    CFtpPidlList::_Fill
 *
 *    Fill a list with an array.
 *
 *    The elements in the array are copied rather than stolen.
 *
 *****************************************************************************/

HRESULT CFtpPidlList::_Fill(int cpidl, LPCITEMIDLIST rgpidl[])
{
    HRESULT hres = S_OK;

    for (int ipidl = 0; (ipidl < cpidl) && SUCCEEDED(hres); ipidl++)
    {
        ASSERT(IsValidPIDL(rgpidl[ipidl]));
        hres = InsertSorted(rgpidl[ipidl]);
    }

    return hres;
}


/*****************************************************************************
 *
 *    CFtpPidlList::GetPidlList
 *
 *****************************************************************************/

LPCITEMIDLIST * CFtpPidlList::GetPidlList(void)
{
    LPITEMIDLIST * ppidl;

    ppidl = (LPITEMIDLIST *) LocalAlloc(LPTR, sizeof(LPITEMIDLIST) * GetCount());
    if (ppidl)
    {
        int nIndex;

        for (nIndex = 0; nIndex < GetCount(); nIndex++)
        {
            // Later we can make this user ILClone() if we want to be able to wack on the
            // pidl list while this list is being used.
            ppidl[nIndex] = GetPidl(nIndex);
        }
    }

    return (LPCITEMIDLIST *) ppidl;
}


/*****************************************************************************
 *
 *    CFtpPidlList::FreePidlList
 *
 *****************************************************************************/

void CFtpPidlList::FreePidlList(LPCITEMIDLIST * ppidl)
{
    LocalFree(ppidl);
}


/*****************************************************************************
 *
 *    CFtpPidlList_Create
 *
 *    Start up a new pv list, with a recommended initial size and other
 *    callback info.
 *
 *****************************************************************************/

HRESULT CFtpPidlList_Create(int cpidl, LPCITEMIDLIST rgpidl[], CFtpPidlList ** ppflpidl)
{
    HRESULT hres = E_OUTOFMEMORY;
    CFtpPidlList * pflpidl;
    *ppflpidl = pflpidl = new CFtpPidlList();

    if (pflpidl)
    {
        hres = pflpidl->_Fill(cpidl, rgpidl);

        if (!EVAL(SUCCEEDED(hres)))
        {
            ASSERT(pflpidl->GetCount() == 0);
            IUnknown_Set(ppflpidl, NULL);
        }
    }

    return hres;
}


int CALLBACK PidlListDestroyCallback(LPVOID p, LPVOID pData)
{
    ILFree((LPITEMIDLIST) p);
    return 1;
}


/****************************************************\
    Constructor
\****************************************************/
CFtpPidlList::CFtpPidlList() : m_cRef(1)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_pfl);
    
    CFtpList_Create(100, PidlListDestroyCallback, 100, &m_pfl);
    ASSERT(m_pfl);      // This sucks
    UseCachedDirListings(FALSE);

    LEAK_ADDREF(LEAK_CFtpPidlList);
}


/****************************************************\
    Destructor
\****************************************************/
CFtpPidlList::~CFtpPidlList()
{
    AssertSorted();
    if (m_pfl)
        m_pfl->Release();

    DllRelease();
    LEAK_DELREF(LEAK_CFtpPidlList);
}


//===========================
// *** IUnknown Interface ***
//===========================

ULONG CFtpPidlList::AddRef()
{
    m_cRef++;
    return m_cRef;
}

ULONG CFtpPidlList::Release()
{
    ASSERT(m_cRef > 0);
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}

HRESULT CFtpPidlList::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = SAFECAST(this, IUnknown *);
    }
    else
    {
        TraceMsg(TF_FTPQI, "CFtpPidlList::QueryInterface() failed.");
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}


////////////////////////////////////////////////////////////////////
// Pild List Enum Helpers
////////////////////////////////////////////////////////////////////

/*****************************************************************************\
     FUNCTION: RecursiveProcessPidl
 
    DESCRIPTION:
        This function will will be called for each item in the initial Pidl List
    (before the recursion occurs).  This is a wrapper because the first list is
    a list of pidls.  The subsequent lists are of WIN32_FIND_DATA types.
\*****************************************************************************/
int RecursiveProcessPidl(LPVOID pvPidl, LPVOID pvInetEnum)
{
    LPCITEMIDLIST pidl = (LPCITEMIDLIST) pvPidl;
    INETENUM * pInetEnum = (INETENUM *) pvInetEnum;
    LPITEMIDLIST pidlFull = ILCombine(pInetEnum->pidlRoot, pidl);

    if (EVAL(pidlFull))
    {
        pInetEnum->hr = pInetEnum->pfnProcessItemCB((LPVOID) pInetEnum->pfnProcessItemCB, pInetEnum->hint, pidlFull, pInetEnum->pfValidhinst, pInetEnum->pvData);
        ILFree(pidlFull);
    }

    return (SUCCEEDED(pInetEnum->hr) ? TRUE : FALSE);
}


/*****************************************************************************\
     FUNCTION: _EnumFolderPrep
 
    DESCRIPTION:
        This function will step into the pszDir directory and enum all of it's
    contents.  For each item, it will call the callback function provided (pfnProcessItemCB).
    That callback function can then call EnumFolder() again (recursively) if
    there is a subfolder.

    NOTE:
        This function needs to first find all the items and then in a second
    loop call the callback function.  This is because the WININET FTP APIs
    only allow one enum to occur at a time, which may not happen if half way through
    enuming one dir, a recursive call starts enuming a sub dir.
\*****************************************************************************/
HRESULT _EnumFolderPrep(HINTERNET hint, LPCITEMIDLIST pidlFull, CFtpPidlList * pPidlList, CWireEncoding * pwe, LPITEMIDLIST * ppidlCurrFtpPath)
{
    HRESULT hr = S_OK;

    // 1. Get Current Directory (To restore later).
    hr = FtpGetCurrentDirectoryPidlWrap(hint, TRUE, pwe, ppidlCurrFtpPath);
    if (EVAL(SUCCEEDED(hr)))
    {
        CMultiLanguageCache cmlc;
        CWireEncoding we;

        if (!pwe)
            pwe = &we;

        // It's important that this is a relative CD.
        // 2. Change Directory Into the subdirectory.   
        hr = FtpSetCurrentDirectoryWrap(hint, TRUE, FtpPidl_GetLastItemWireName(pidlFull));
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidlItem;
            HINTERNET hInetFind = NULL;

            hr = FtpFindFirstFilePidlWrap(hint, TRUE, &cmlc, pwe, NULL, &pidlItem, pPidlList->m_dwInetFlags, NULL, &hInetFind);
            if (hInetFind)
            {
                do
                {
                    LPCWIRESTR pwireStr = FtpPidl_GetLastItemWireName(pidlFull);
                    if (IS_VALID_FILE(pwireStr))
                    {
                        // Store entire pidl (containing WIN32_FIND_DATA) so we can get
                        // the attributes and other info later.  Seeing if it's a dir
                        // is one need...
                        pPidlList->InsertSorted(pidlItem);
                    }

                    ILFree(pidlItem);
                    hr = InternetFindNextFilePidlWrap(hInetFind, TRUE, &cmlc, pwe, &pidlItem);
                }
                while (SUCCEEDED(hr));
            
                ILFree(pidlItem);
                InternetCloseHandle(hInetFind);
            }

            if (ERROR_NO_MORE_FILES == HRESULT_CODE(hr))
                hr = S_OK;
        }

        EVAL(SUCCEEDED(pwe->ReSetCodePages(&cmlc, pPidlList)));
    }

    return hr;
}


/*****************************************************************************\
     FUNCTION: _GetPathDifference
 
    DESCRIPTION:
        This function will step into the pszDir directory and enum all of it's
    contents.  For each item, it will call the callback function provided (pfnProcessItemCB).
    That callback function can then call EnumFolder() again (recursively) if
    there is a subfolder.

    NOTE:
        This function needs to first find all the items and then in a second
    loop call the callback function.  This is because the WININET FTP APIs
    only allow one enum to occur at a time, which may not happen if half way through
    enuming one dir, a recursive call starts enuming a sub dir.

    PARAMETERS:
        pszBaseUrl - This needs to be escaped.
        pszDir - This needs to be escaped.
        *ppszUrlPathDiff - This will be UnEscaped.
\*****************************************************************************/
void _GetPathDifference(LPCTSTR pszBaseUrl, LPCTSTR pszDir, LPTSTR * ppszUrlPathDiff)
{
    TCHAR szUrlPathDiff[MAX_URL_STRING];
    TCHAR szFullUrl[MAX_URL_STRING];
    DWORD cchSize = ARRAYSIZE(szFullUrl);

    // This is needed for this case:
    // pszBaseUrl="ftp://server/subdir1/", pszDir="/subdir1/subdir2/file.txt"
    // So, szUrlPathDiff="subdir2/file.txt" instead of pszDir
    //
    // ICU_NO_ENCODE is needed because Download Dlg may have paths with
    // spaces that can't be escaped.
    InternetCombineUrl(pszBaseUrl, pszDir, szFullUrl, &cchSize, ICU_NO_ENCODE);
    UrlGetDifference(pszBaseUrl, szFullUrl, szUrlPathDiff, ARRAYSIZE(szUrlPathDiff));

    // We will now use szFullUrl to store the UnEscaped version since these buffers
    // are so large.
    UnEscapeString(szUrlPathDiff, szFullUrl, ARRAYSIZE(szFullUrl));
    Str_SetPtr(ppszUrlPathDiff, szFullUrl);
}


/*****************************************************************************\
     FUNCTION: EnumFolder
 
    DESCRIPTION:
        This function will step into the pszDir directory and enum all of it's
    contents.  For each item, it will call the callback function provided (pfnProcessItemCB).
    That callback function can then call EnumFolder() again (recursively) if
    there is a subfolder.

    PARAMETERS:
        (pszBaseUrl=ftp://server/dir1/, pszDir=dir2, DirToEnum=ftp://server/dir1/dir2/)
        pszDir - This is the directory we are enumerating. (dir2)  It is relative to pszBaseUrl.
        hint - The current working directory will be set to pszBaseUrl.  _EnumFolderPrep will make it go into pszDir.

    NOTE:
        This function needs to first find all the items and then in a second
    loop call the callback function.  This is because the WININET FTP APIs
    only allow one enum to occur at a time, which may not happen if half way through
    enuming one dir, a recursive call starts enuming a sub dir.
\*****************************************************************************/
HRESULT EnumFolder(LPFNPROCESSITEMCB pfnProcessItemCB, HINTERNET hint, LPCITEMIDLIST pidlFull, CWireEncoding * pwe, BOOL * pfValidhinst, LPVOID pvData)
{
    CFtpPidlList * pPidlList;
    BOOL fValidhinst = TRUE;

    HRESULT hr = CFtpPidlList_Create(0, &pidlFull, &pPidlList);
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidlCurrFtpPath = NULL;

        hr = _EnumFolderPrep(hint, pidlFull, pPidlList, pwe, &pidlCurrFtpPath);
        if (SUCCEEDED(hr))
        {
            hr = S_OK;
            // 4. Process each file name, which may be recursive.
            // This loop and the while loop above need to be
            // separated because it's not possible to create
            // more than one FTP Find File handle based on the
            // same session.
            for (int nIndex = 0; SUCCEEDED(hr) && (nIndex < pPidlList->GetCount()); nIndex++)
            {
                LPITEMIDLIST pidlNewFull = ILCombine(pidlFull, pPidlList->GetPidl(nIndex));

                hr = pfnProcessItemCB(pfnProcessItemCB, hint, pidlNewFull, &fValidhinst, pvData);
                ILFree(pidlNewFull);
            }

            // 5. Go back to original directory (from Step 2)
            // The only time we don't want to return to the original directory is if
            // the hinst was freed in an wininet callback function.  We may cache the hinst
            // so we need the directory to be valid later.
            if (fValidhinst)
            {
                if (SUCCEEDED(hr))
                {
                    // We still want to reset the directory but we don't want to over write
                    // the original error message.
                    hr = FtpSetCurrentDirectoryPidlWrap(hint, TRUE, pidlCurrFtpPath, TRUE, TRUE);
                }
            }

            Pidl_Set(&pidlCurrFtpPath, NULL);
        }

        pPidlList->Release();
    }

    if (pfValidhinst)
        *pfValidhinst = fValidhinst;

    return hr;
}
