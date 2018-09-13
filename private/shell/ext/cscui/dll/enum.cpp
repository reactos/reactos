//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       enum.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include <shlwapip.h>   // QITAB, QISearch
#include <shsemip.h>    // ILFree(), etc

#include "folder.h"
#include "security.h"

//
// Create a single entry in the server status cache.
//
CServerStatusCache::CEntry::CEntry(
    LPCTSTR pszServer,
    DWORD dwStatus
    ) : m_pszServer(StrDup(pszServer)),
        m_dwStatus(dwStatus)
{

}


//
// Destroy a single entry in the server status cache.
//
CServerStatusCache::CEntry::~CEntry(
    void
    )
{
    delete[] m_pszServer;
}


//
// Helper for copying strings.
//
LPTSTR 
CServerStatusCache::CEntry::StrDup(
    LPCTSTR psz
    )
{
    LPTSTR pszNew = new TCHAR[lstrlen(psz) + 1];
    if (NULL != pszNew)
        lstrcpy(pszNew, psz);

    return pszNew;
}



//
// Destroy the server status cache.
//
CServerStatusCache::~CServerStatusCache(
    void
    )
{
    if (NULL != m_hdpa)
    {
        //
        // Delete each entry in the DPA then destroy the
        // DPA itself.
        //
        int cEntries = DPA_GetPtrCount(m_hdpa);
        for (int i = 0; i < cEntries; i++)
        {
            delete (CEntry *)DPA_GetPtr(m_hdpa, i);
        }
        DPA_Destroy(m_hdpa);
    }
}

//
// Add a share's status to the cache.  We strip the UNC path to it's
// bare server name then add the status to the cache.  If there's
// no existing entry we just add it.  If there is an existing entry,
// we bitwise OR the status bits in with the existing entry.  This way
// the status of the server is the summation of the status of all
// it's shares.
//
bool 
CServerStatusCache::AddShareStatus(
    LPCTSTR pszShare, 
    DWORD dwShareStatus
    )
{
    bool bResult = true;
    TCHAR szServer[MAX_PATH];
    CEntry *pEntry = FindEntry(ServerFromUNC(pszShare, szServer, ARRAYSIZE(szServer)));

    if (NULL != pEntry)
    {
        //
        // Found existing server entry for this share.  Merge in the
        // status bits for this share.
        //
        pEntry->AddStatus(dwShareStatus);
    }
    else
    {
        //
        // No existing entry for this share's server.
        //
        if (NULL == m_hdpa)
        {
            //
            // No DPA exists yet.  Create one.
            // We delay creation of the DPA until we really need one.
            //
            m_hdpa = DPA_Create(8);
        }
        if (NULL != m_hdpa)
        {
            //
            // We have a DPA.  Create a new entry for this share's server
            // and add it to the DPA.
            //

            pEntry = new CEntry(szServer, dwShareStatus);
            if (NULL != pEntry)
            {
                if (!pEntry->IsValid() || -1 == DPA_AppendPtr(m_hdpa, pEntry))
                {
                    //
                    // One of the following happened:
                    //      1. Failure allocating server name in CEntry obj.
                    //      2. Failure adding CEntry obj ptr to DPA.
                    //
                    delete pEntry;
                    bResult = false;
                }
            }
        }
        else
        {
            bResult = false; // DPA creation failed.
        }
    }
    return bResult;
}


//
// Obtain the CSC status bits for a given server.
// This function assumes the pszUNC arg is a valid UNC path.
//
DWORD 
CServerStatusCache::GetServerStatus(
    LPCTSTR pszUNC
    )
{
    DWORD dwStatus = 0;
    TCHAR szServer[MAX_PATH];
    
    CEntry *pEntry = FindEntry(ServerFromUNC(pszUNC, szServer, ARRAYSIZE(szServer)));
    if (NULL == pEntry)
    {
        //
        // No entry for this server.  Scan the CSC cache and pick up any new
        // servers added.  Since the lifetime of this server cache is only for a single
        // enumeration, we should have to do this only once.  However, if for some
        // reason, something gets added to the CSC cache while we're opening the viewer, 
        // this code path will pick up the new server entry.
        // 
        WIN32_FIND_DATA fd;
        DWORD dwStatus;
        CCscFindHandle hFind = CacheFindFirst(NULL, &fd, &dwStatus, NULL, NULL, NULL);
        if (hFind.IsValid())
        {
            do
            {
                AddShareStatus(fd.cFileName, dwStatus);
            }
            while(CacheFindNext(hFind, &fd, &dwStatus, NULL, NULL, NULL));
        }
        //
        // Now that we have rescanned the CSC cache, try it again.
        //
        pEntry = FindEntry(szServer);
    }
    return pEntry ? pEntry->GetStatus() : 0;
}


//
// Find a single entry in the server cache.
// Assumes pszServer is a raw server name (not UNC).
// Returns NULL if no match found.
//
CServerStatusCache::CEntry *
CServerStatusCache::FindEntry(
    LPCTSTR pszServer
    )
{
    CEntry *pEntry = NULL;
    if (NULL != m_hdpa)
    {
        int cEntries = DPA_GetPtrCount(m_hdpa);
        for (int i = 0; i < cEntries; i++)
        {
            CEntry *pe = (CEntry *)DPA_GetPtr(m_hdpa, i);
            if (0 == lstrcmpi(pe->GetServer(), pszServer))
            {
                pEntry = pe;
                break;
            }
        }
    }
    return pEntry;
}


LPTSTR 
CServerStatusCache::ServerFromUNC(
    LPCTSTR pszShare, 
    LPTSTR pszServer, 
    UINT cchServer
    )
{
    LPTSTR pszReturn = pszServer; // Remember for return.

    cchServer--;  // Leave room for terminating nul.

    while(*pszShare && TEXT('\\') == *pszShare)
        pszShare++;

    while(*pszShare && TEXT('\\') != *pszShare && cchServer--)
        *pszServer++ = *pszShare++;

    *pszServer = TEXT('\0');
    return pszReturn;
}



STDMETHODIMP COfflineFilesEnum::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(COfflineFilesEnum, IEnumIDList),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_ (ULONG) COfflineFilesEnum::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_ (ULONG) COfflineFilesEnum::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

COfflineFilesEnum::COfflineFilesEnum(DWORD grfFlags, COfflineFilesFolder *pfolder)
{
    _cRef = 1;

    //
    // The minimum size of the buffer must be MAX_PATH.
    // The enumeration code is designed to grow it as needed.
    //
    _cchPathBuf = MAX_PATH;
    _pszPath    = (LPTSTR)LocalAlloc(LMEM_FIXED, sizeof(TCHAR) * _cchPathBuf);
    if (NULL != _pszPath)
        *_pszPath = TEXT('\0');
    else
        _cchPathBuf = 0;

    _grfFlags = grfFlags,
    _pfolder = pfolder;
    _pfolder->AddRef();
    _dwServerStatus = 0;

    _hdsaFolderPathInfo = DSA_Create(sizeof(FolderPathInfo), 10);

    //
    // Determine if we should be showing system and/or hidden files.
    //
    _bShowHiddenFiles      = boolify(ShowHidden());
    _bShowSuperHiddenFiles = boolify(ShowSuperHidden());
    _bUserIsAdmin          = boolify(IsCurrentUserAnAdminMember());

    DllAddRef();
}

COfflineFilesEnum::~COfflineFilesEnum()
{
    if (_pfolder)
        _pfolder->Release();

    Reset();
    if (_hdsaFolderPathInfo)
    {
        int cPaths = DSA_GetItemCount(_hdsaFolderPathInfo);
        FolderPathInfo fpi;
        for (int i = 0; i < cPaths; i++)
        {
            if (DSA_GetItem(_hdsaFolderPathInfo, i, &fpi) && NULL != fpi.pszPath)
                LocalFree(fpi.pszPath);
        }
        DSA_Destroy(_hdsaFolderPathInfo);
    }
    if (NULL != _pszPath)
        LocalFree(_pszPath);

    DllRelease();
}

//
// Since we're not throwing exceptions, clients must call this after ctor 
// to verify allocations succeeded.
// 
bool
COfflineFilesEnum::IsValid(
    void
    ) const
{
    return (NULL != _hdsaFolderPathInfo) && (NULL != _pszPath);
}


bool 
COfflineFilesEnum::PopFolderPathInfo(
    FolderPathInfo *pfpi
    )
{ 
    bool bResult = false;
    TraceAssert(NULL != _hdsaFolderPathInfo);

    int iItem = DSA_GetItemCount(_hdsaFolderPathInfo) - 1;
    if ((0 <= iItem) && DSA_GetItem(_hdsaFolderPathInfo, iItem, pfpi))
    {
        DSA_DeleteItem(_hdsaFolderPathInfo, iItem);
        bResult = true;
    }
    return bResult;
}


//
// Build complete path to folder in a heap allocation and push it onto
// stack of saved folder paths.
// Returns false if memory can't be allocated for path.
//
bool
COfflineFilesEnum::SaveFolderPath(
    LPCTSTR pszRoot,
    LPCTSTR pszFolder
    )
{
    bool bResult = false;

    FolderPathInfo fpi;
    //
    // Length is "root" + '\' + "folder" + <nul>
    //
    fpi.cchPath = lstrlen(pszRoot) + lstrlen(pszFolder) + 2;
    fpi.pszPath = (LPTSTR)LocalAlloc(LPTR, MAX(fpi.cchPath, DWORD(MAX_PATH)) * sizeof(TCHAR));

    if (NULL != fpi.pszPath)
    {
        PathCombine(fpi.pszPath, pszRoot, pszFolder);
        if (PushFolderPathInfo(fpi))
            bResult = true;
        else
            LocalFree(fpi.pszPath);
    }

    return bResult;
}


//
// Increases the size of the _pszPath buffer by a specified amount.
// Original contents of buffer ARE NOT preserved.  
// Returns:
//      S_FALSE       - _pszPath buffer was large enough.  Not modified.
//      S_OK          - _pszPath points to new bigger buffer.
//      E_OUTOFMEMORY - _pszPath points to original unmodified buffer.
//
HRESULT
COfflineFilesEnum::GrowPathBuffer(
    INT cchRequired,
    INT cchExtra
    )
{
    HRESULT hres = S_FALSE;
    if (_cchPathBuf <= cchRequired)
    {
        LPTSTR pszNewBuf = (LPTSTR)LocalAlloc(LMEM_FIXED, sizeof(TCHAR) * (cchRequired + cchExtra));
        if (NULL != pszNewBuf)
        {
            if (NULL != _pszPath)
                LocalFree(_pszPath);
            _pszPath    = pszNewBuf;
            _cchPathBuf = cchRequired + cchExtra;
            hres = S_OK;
        }
        else
        {
            hres = E_OUTOFMEMORY; // Failure.  Orig buffer is left intact.
        }
    }
    return hres;
}


//
// Determine if user has access to view this file.
//
bool
COfflineFilesEnum::UserHasAccess(
    const CscFindData& cscfd
    )
{
    return _bUserIsAdmin || 
           CscAccessUser(cscfd.dwStatus) || 
           CscAccessGuest(cscfd.dwStatus);
}


//
// Centralize any item-exclusion logic in a single function.
//
bool 
COfflineFilesEnum::Exclude(
    const CscFindData& cscfd
    )
{
    //
    // BUGBUG:  Need to exclude files with particular extensions when _bShowSystemFiles
    //          is false.  The shell excludes [dll sys vxd 386 drv pnf] but this
    //          mechanism isn't exported from the shell.
    //          I think it should be.
    //          
    return ((FILE_ATTRIBUTE_DIRECTORY & cscfd.fd.dwFileAttributes) ||
            (FLAG_CSC_COPY_STATUS_LOCALLY_DELETED & cscfd.dwStatus) ||
           ((FILE_ATTRIBUTE_HIDDEN & cscfd.fd.dwFileAttributes) && !_bShowHiddenFiles) ||
           (IsHiddenSystem(cscfd.fd.dwFileAttributes) && !_bShowSuperHiddenFiles) ||
           !UserHasAccess(cscfd));
}


//
// If a folder is hidden and the current shell setting says to not show hidden files,
// don't enumerate any children of a folder.  Likewise for super hidden files and the
// "show super hidden files" setting.
//
bool
COfflineFilesEnum::OkToEnumFolder(
    const CscFindData& cscfd
    )
{
    return (_bShowHiddenFiles || (0 == (FILE_ATTRIBUTE_HIDDEN & cscfd.fd.dwFileAttributes))) &&
           (_bShowSuperHiddenFiles || !IsHiddenSystem(cscfd.fd.dwFileAttributes));
}



HRESULT COfflineFilesEnum::Next(ULONG celt, LPITEMIDLIST *rgelt, 
                                ULONG *pceltFetched)
{
    HRESULT hres;
    CscFindData cscfd;
    ULONG celtEnumed;

    //
    // If you've hit one of these asserts, you didn't call IsValid()
    // before using the enumerator.
    //
    TraceAssert(NULL != _pszPath);
    TraceAssert(NULL != _hdsaFolderPathInfo);

    //
    // This label is used to restart the enum if an item is excluded.
    //
enum_start:
    hres       = S_FALSE;
    celtEnumed = 0;
    ZeroMemory(&cscfd, sizeof(cscfd));

    if (!_hEnumShares.IsValid())
    {
        //
        // First time through.
        // Enumerate shares and files until we find a folder or file.
        //
        _hEnumShares = CacheFindFirst(NULL, &cscfd);
        if (_hEnumShares.IsValid())
        {
            _dwServerStatus = _ServerStatusCache.GetServerStatus(cscfd.fd.cFileName);
            do
            {
                //
                // Buffer attached to _pszPath is guaranteed to be at least
                // MAX_PATH so it's safe to copy cFileName[].
                //
                lstrcpy(_pszPath, cscfd.fd.cFileName);
                _hEnum = CacheFindFirst(_pszPath, &cscfd);
                if (_hEnum.IsValid())
                {
                    celtEnumed = 1;
                }
            }
            while(0 == celtEnumed && CacheFindNext(_hEnumShares, &cscfd));
        }
    }
    else
    {
        if (_hEnum.IsValid())
        {
            if (CacheFindNext(_hEnum, &cscfd))
            {
                //
                // Most common case.  Got next file in current folder.
                //
                celtEnumed = 1;
            }
            else
            {
                //
                // Enumeration exhausted for this folder.  If we have folder paths
                // saved on the stack, keep popping them until we find one containing
                // at least one file or folder.
                //
                FolderPathInfo fpi;
                while(SUCCEEDED(hres) && 0 == celtEnumed && PopFolderPathInfo(&fpi) && NULL != fpi.pszPath)
                {
                    _hEnum = CacheFindFirst(fpi.pszPath, &cscfd);
                    if (_hEnum.IsValid())
                    {
                        //
                        // The popped folder path is the only opportunity we have
                        // where a string could overflow the temp _pszPath buffer.
                        // If necesary, grow the buffer to hold the path.  Add
                        // room for an extra 100 chars to minimize re-growth.
                        // Buffer is not altered if required path length is 
                        // less than _cchPathBuf.
                        //
                        if (FAILED(GrowPathBuffer(fpi.cchPath, 100)))
                            hres = E_OUTOFMEMORY;

                        if (SUCCEEDED(hres))
                        {
                            lstrcpy(_pszPath, fpi.pszPath);
                            celtEnumed = 1;
                        }
                    }
                    LocalFree(fpi.pszPath);
                }
                if (SUCCEEDED(hres))
                {
                    while(0 == celtEnumed && CacheFindNext(_hEnumShares, &cscfd))
                    {
                        //
                        // No more saved folder paths.  This share is exhausted.
                        // Enumerate next share.  If next is empty, keep enumerating
                        // shares until we find one with content.  The buffer
                        // attached to _pszPath is guaranteed to be at least MAX_PATH
                        // so it's always safe to copy cFileName[].
                        //
                        _dwServerStatus = _ServerStatusCache.GetServerStatus(cscfd.fd.cFileName);
                        lstrcpy(_pszPath, cscfd.fd.cFileName);
                        _hEnum = CacheFindFirst(_pszPath, &cscfd);
                        if (_hEnum.IsValid())
                        {
                            celtEnumed = 1;
                        }
                    }
                }
            }
        }
    }

    if (celtEnumed)
    {
        if (FILE_ATTRIBUTE_DIRECTORY & cscfd.fd.dwFileAttributes)
        {
            if (OkToEnumFolder(cscfd))
            {
                //
                // Save the folder path on a stack.  This is how we enumerate
                // the cache item hierarcy as a flat list.  We'll pop these off
                // the stack on future calls to Next() when all children of the
                // current folder have been enumerated.
                //
                if (!SaveFolderPath(_pszPath, cscfd.fd.cFileName))
                {
                    //
                    // Path not saved.  Insufficient heap memory. 
                    // Abort the enumeration.
                    //
                    hres = E_OUTOFMEMORY;
                }
            }
        }

        if (SUCCEEDED(hres))
        {
            if (!Exclude(cscfd))
            {
                //
                // An IDList is composed of a fixed-length part and a variable-length
                // path+name buffer.
                // The path+name variable-length buffer is formatted as follows:
                //
                // dir1\dir2\dir3<nul>name<nul>
                //
                TCHAR szUNC[MAX_PATH];
                if (PathCombine(szUNC, _pszPath, cscfd.fd.cFileName))
                {
                    hres = COfflineFilesFolder::OLID_CreateFromUNCPath(szUNC,
                                                                       &cscfd.fd,
                                                                       cscfd.dwStatus,
                                                                       cscfd.dwPinCount,
                                                                       cscfd.dwHintFlags,
                                                                       _dwServerStatus,
                                                                       (LPOLID *)&rgelt[0]);

                }
            }
            else
            {
                //
                // This item is excluded from the enumeration.  Restart.
                // I normally don't like goto's but doing this with a loop
                // is just plain harder to understand.  The goto is quite
                // appropriate in this circumstance.
                //
                goto enum_start;
            }
        }
    }

    if (pceltFetched)
        *pceltFetched = celtEnumed;
    return hres;
}

HRESULT COfflineFilesEnum::Skip(ULONG celt)
{
    return E_NOTIMPL;
}

HRESULT COfflineFilesEnum::Reset()
{
    _hEnum.Close();
    _hEnumShares.Close();
    return S_OK;
}

HRESULT COfflineFilesEnum::Clone(IEnumIDList **ppenum)
{
    return E_NOTIMPL;
}



