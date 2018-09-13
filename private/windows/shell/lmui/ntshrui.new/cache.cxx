//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1995.
//
//  File:       cache.cxx
//
//  Contents:   Functions to manage a cache of shares
//
//  History:    11-Apr-95    BruceFo  Created
//              21-Aug-95    BruceFo  Created CShareCache class to clean up
//                                      resource usage of resources protected
//                                      by critical section.
//
//----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

#include "critsec.hxx"
#include "cache.hxx"
#include "dllmain.hxx"
#include "shrinfo.hxx"
#include "strhash.hxx"
#include "util.hxx"

//////////////////////////////////////////////////////////////////////////////

#if DBG == 1
VOID
DumpNetEnum(
    IN LPVOID pBufShares,
    IN ULONG entriesRead
    );
#endif // DBG == 1

//////////////////////////////////////////////////////////////////////////////

CShareCache g_ShareCache;   // the main share cache

//////////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
//
//  Member:     CShareCache::CShareCache
//
//  Synopsis:   Constructor.
//
//  History:    21-Aug-95    BruceFo  Created
//
//--------------------------------------------------------------------------

CShareCache::CShareCache(
    VOID
    )
    :
    m_cShares(0),
    m_pBufShares(NULL),
    m_pHash(NULL)
{
    InitializeCriticalSection(&m_csBuf);
}


//+-------------------------------------------------------------------------
//
//  Member:     CShareCache::~CShareCache
//
//  Synopsis:   Destructor
//
//  History:    21-Aug-95    BruceFo  Created
//
//--------------------------------------------------------------------------

CShareCache::~CShareCache()
{
    Delete();
}


//+-------------------------------------------------------------------------
//
//  Member:     CShareCache::Delete
//
//  Synopsis:   Gets rid of cached memory.
//
//  History:    21-Aug-95    BruceFo  Created
//
//--------------------------------------------------------------------------

VOID
CShareCache::Delete(
    VOID
    )
{
    CTakeCriticalSection t(&m_csBuf);
    if (NULL != m_pBufShares)
    {
        NetApiBufferFree(m_pBufShares);
    }
    m_pBufShares = NULL;
    delete m_pHash;
    m_pHash = NULL;
    m_cShares = 0;
}


//+-------------------------------------------------------------------------
//
//  Member:     CShareCache::IsPathShared
//
//  Synopsis:   See ::IsPathShared.
//
//  History:    21-Aug-95    BruceFo  Created
//
//--------------------------------------------------------------------------

BOOL
CShareCache::IsPathShared(
    LPCTSTR lpPath,
    BOOL fRefresh
    )
{
    BOOL bOldSharingEnabled = g_fSharingEnabled;
    BOOL bRet = FALSE;

    {
        // scope the critical section taking

        CTakeCriticalSection t(&m_csBuf);

        // For plug and play: if the server service starts
        // or stops, we get a refresh call.  If sharing is not currently
        // enabled but a refresh is request, see if sharing has just become
        // available.

        if (fRefresh)
        {
            appDebugOut((DEB_TRACE, "Forced cache refresh!\n"));

            RefreshNoCritSec(NULL);
        }

        if (CacheOK())
        {
            appAssert(NULL != m_pHash);
            bRet = m_pHash->IsMember(lpPath);
        }
        else
        {
            // the server doesn't seem to be running...
            bRet = FALSE;
        }
    }

    if (bOldSharingEnabled != g_fSharingEnabled)
    {
        // The server either came up or went down, and we refreshed based on
        // that fact. Force the shell/explorer to redraw *all* views.

        appDebugOut((DEB_TRACE, "Forcing the shell to redraw *all* views!\n"));

        SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
    }

    return bRet;
}



//+-------------------------------------------------------------------------
//
//  Member:     CShareCache::Refresh
//
//  Synopsis:   Refreshes the cache of shares
//
//  History:    21-Aug-95    BruceFo  Created
//
//  Note:       Sets g_fSharingEnabled
//
//--------------------------------------------------------------------------

VOID
CShareCache::Refresh(
    IN PWSTR pszServer
    )
{
    CTakeCriticalSection t(&m_csBuf);
    RefreshNoCritSec(pszServer);
}


//+-------------------------------------------------------------------------
//
//  Member:     CShareCache::RefreshNoCritSec
//
//  Synopsis:   Refreshes the cache of shares: the critical section must
//              already taken!
//
//  History:    18-Aug-95    BruceFo  Created
//
//  Note:       Sets g_fSharingEnabled
//
//--------------------------------------------------------------------------

VOID
CShareCache::RefreshNoCritSec(
    IN PWSTR pszServer
    )
{
    Delete();

    DWORD entriesRead, totalEntries;
    DWORD err = ::NetShareEnum(
                        pszServer,
                        502,
                        &m_pBufShares,
                        0xffffffff,     // no buffer limit; get them all!
                        &entriesRead,
                        &totalEntries,
                        NULL);  // no resume handle 'cause we're getting all
    if (err != NERR_Success)
    {
        appDebugOut((DEB_ERROR,
            "Error enumerating shares: 0x%08lx\n",
            err));

        m_pBufShares = NULL;    // just in case NetShareEnum munged it
        Delete();
    }
    else
    {
        appAssert(entriesRead == totalEntries);
        m_cShares = entriesRead;
    }

    if (m_cShares > 0)
    {
        //
        // Now, create a hash table and put all the shares into it (strings are
        // cached; don't copy any data)
        //

        m_pHash = new CStrHashTable(m_cShares * 2 - 1);
        if ((NULL == m_pHash) || FAILED(m_pHash->QueryError()))
        {
            // out of memory; delete everything
            Delete();
        }
        else
        {
            SHARE_INFO_502* pShareBase = (SHARE_INFO_502 *)m_pBufShares;
            SHARE_INFO_502* pShare;

            for (UINT iShare = 0; iShare < m_cShares; iShare++)
            {
                pShare = &pShareBase[iShare];

                // Looking for all disk shares, including all special shares
                // (X$, ADMIN$) except IPC$.
                if (pShare->shi502_type != STYPE_DISKTREE)
                {
                    if ((pShare->shi502_type == STYPE_SPECIAL) // X$, ADMIN$, IPC$
                        && (0 == _wcsicmp(g_szIpcShare, pShare->shi502_netname))
                        )
                    {
                        continue;
                    }
                }

                HRESULT hr = m_pHash->Insert(pShare->shi502_path);
                if (FAILED(hr))
                {
                    // out of memory; delete everything
                    Delete();
                    break;
                }
            }
        }

#if DBG == 1
        if (NULL != m_pHash)
        {
            // if everything hasn't been deleted because of a memory problem...
            m_pHash->Print();
        }
#endif // DBG == 1

    }

    g_fSharingEnabled = CacheOK();
}


//+-------------------------------------------------------------------------
//
//  Member:     CShareCache::IsShareNameUsed
//
//  Synopsis:   Returns TRUE if the share name in question is already used
//
//  History:    4-Apr-95    BruceFo  Created
//
//--------------------------------------------------------------------------

BOOL
CShareCache::IsShareNameUsed(
    IN PWSTR pszShareName
    )
{
    CTakeCriticalSection t(&m_csBuf);

    if (!CacheOK())
    {
        return FALSE;
    }

    SHARE_INFO_502* pShareBase = (SHARE_INFO_502 *)m_pBufShares;
    SHARE_INFO_502* pShare;

    for (UINT iShare = 0; iShare < m_cShares; iShare++)
    {
        pShare = &pShareBase[iShare];
        if (0 == _wcsicmp(pszShareName, pShare->shi502_netname))
        {
            return TRUE;
        }
    }

    return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Member:     CShareCache::IsExistingShare
//
//  Synopsis:   Finds out if a share name is already in use with a different
//              path.
//
//  Arguments:  [pszShareName] - name of share being replaced
//              [pszPath] - path to compare against
//              [pszOldPath] - If not null, filled with path of the share,
//                             if found
//
//  Returns:    Returns TRUE if found and the paths are different,
//              FALSE otherwise
//
//  History:    4-May-95   BruceFo     Stolen
//
//--------------------------------------------------------------------------

BOOL
CShareCache::IsExistingShare(
    IN PCWSTR pszShareName,
    IN PCWSTR pszPath,
    OUT PWSTR pszOldPath
    )
{
    appAssert(NULL != pszShareName);

    CTakeCriticalSection t(&m_csBuf);

    if (!CacheOK())
    {
        return FALSE;
    }

    SHARE_INFO_502* pShareBase = (SHARE_INFO_502 *)m_pBufShares;
    SHARE_INFO_502* pShare;

    for (UINT iShare = 0; iShare < m_cShares; iShare++)
    {
        pShare = &pShareBase[iShare];
        if (0 == _wcsicmp(pszShareName, pShare->shi502_netname))
        {
            if (pszOldPath != NULL)
            {
                wcscpy(pszOldPath, pShare->shi502_path);
            }

            return TRUE;
        }
    }

    return FALSE;
}


//+-------------------------------------------------------------------------
//
//  Member:     CShareCache::ConstructList
//
//  Synopsis:   Construct a list of shares for a particular path
//
//  Arguments:
//
//  Returns:    hresult
//
//  History:    21-Aug-95   BruceFo     Created
//
//--------------------------------------------------------------------------

HRESULT
CShareCache::ConstructList(
    IN PCWSTR          pszPath,
    IN OUT CShareInfo* pShareList,
    OUT ULONG*         pcShares
    )
{
    CTakeCriticalSection t(&m_csBuf);

    SHARE_INFO_502* pShareBase = (SHARE_INFO_502 *)m_pBufShares;
    SHARE_INFO_502* pShare;

    HRESULT hr;
    ULONG cShares = 0;

    for (UINT iShare = 0; iShare < m_cShares; iShare++)
    {
        pShare = &pShareBase[iShare];

        if (0 == _wcsicmp(pszPath, pShare->shi502_path))
        {
            // Looking for all disk shares, including all special shares
            // (X$, ADMIN$) except IPC$.
            if (pShare->shi502_type != STYPE_DISKTREE)
            {
                if ((pShare->shi502_type == STYPE_SPECIAL) // X$, ADMIN$, IPC$
                    && (0 == _wcsicmp(g_szIpcShare, pShare->shi502_netname))
                    )
                {
                    continue;
                }
            }

            //
            // We found one!
            //

            appDebugOut((DEB_ITRACE,
                "ConstructList: adding %ws\n",
                pShare->shi502_netname));

            CShareInfo* pNewInfo = new CShareInfo();
            if (NULL == pNewInfo)
            {
                return E_OUTOFMEMORY;
            }

            hr = pNewInfo->InitInstance();
            if (FAILED(hr))
            {
                delete pNewInfo;
                return hr;
            }

            // We can't point into the data protected by a critical section,
            // so we must copy it.
            hr = pNewInfo->Copy(pShare);
            if (FAILED(hr))
            {
                delete pNewInfo;
                return hr;
            }

            pNewInfo->InsertBefore(pShareList); // add to end of list

            ++cShares;
        }
    }

    *pcShares = cShares;
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Member:     CShareCache::ConstructParentWarnList
//
//  Synopsis:   Construct a new list of shares that are children or descendants
//              of the path passed in.
//
//  Arguments:  [pszPath] - the prefix path to check for
//              [ppShareList] - new share list, if success. Caller must delete
//                  it using 'delete' on each element. This list is
//                  doubly-linked with a dummy head node. NOTE: As an
//                  optimization, this is set to NULL if there is no share.
//                  This avoids allocating and deleting memory unless there
//                  is something to warn the user about.
//
//  Returns:    hresult
//
//  History:    21-Aug-95   BruceFo     Created
//
//--------------------------------------------------------------------------

HRESULT
CShareCache::ConstructParentWarnList(
    IN PCWSTR        pszPath,
    OUT CShareInfo** ppShareList
    )
{
    CTakeCriticalSection t(&m_csBuf);

    HRESULT hr;
    CShareInfo* pShareList = NULL;
    SHARE_INFO_502* pShareBase = (SHARE_INFO_502 *)m_pBufShares;
    SHARE_INFO_502* pShare;
    INT cchPath = wcslen(pszPath);

    for (UINT iShare = 0; iShare < m_cShares; iShare++)
    {
        pShare = &pShareBase[iShare];

        PWSTR pszSharePath = pShare->shi502_path;
        INT cchSharePath = wcslen(pszSharePath);

        if (cchSharePath >= cchPath)
        {
            // BUGBUG - the following won't work with LFN/shortname differences

            // BUGBUG: we're doing a prefix match of the current directory
            // name on the set of share names. This could be expensive with
            // a linear search!

            if (0 == _wcsnicmp(pszSharePath, pszPath, cchPath)
                && (    *(pszSharePath + cchPath) == TEXT('\\')
                     || *(pszSharePath + cchPath) == TEXT('\0')
                   )
               )
            {
                appDebugOut((DEB_TRACE,
                    "ConstructParentWarnList, share %ws, file %ws. Found a prefix!\n",
                    pszSharePath, pszPath));

                if (NULL == pShareList)
                {
                    // do the lazy dummy head node creation if this is the
                    // first prefix match

                    pShareList = new CShareInfo();  // dummy head node
                    if (NULL == pShareList)
                    {
                        return E_OUTOFMEMORY;
                    }
                }

                CShareInfo* pNewInfo = new CShareInfo();
                if (NULL == pNewInfo)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    hr = pNewInfo->InitInstance();
                    if (SUCCEEDED(hr))
                    {
                        // We can't point into the data protected by a
                        // critical section, so we must copy it.
                        hr = pNewInfo->Copy(pShare);
                    }
                }

                if (FAILED(hr))
                {
                    delete pNewInfo;
                    DeleteShareInfoList(pShareList, TRUE);

                    return hr;
                }

                pNewInfo->InsertBefore(pShareList); // add to end of list
            }
        }
    }

    *ppShareList = pShareList;
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Member:     CShareCache::CacheOK
//
//  Synopsis:   Returns TRUE if the cache contains valid data.
//
//  History:    24-Sep-95    BruceFo  Created
//
//  Note:       The critical section must be held when calling this function
//
//--------------------------------------------------------------------------

BOOL
CShareCache::CacheOK(
    VOID
    )
{
    // either both are valid or both are invalid
    appAssert(
        ((NULL != m_pHash) && (NULL != m_pBufShares)) ||
        ((NULL == m_pHash) && (NULL == m_pBufShares))
        );

    return (NULL != m_pHash);
}

#if DBG == 1
VOID
CShareCache::Dump(
	VOID
	)
{
   	appDebugOut((DEB_TRACE, "Cache dump=====================\n"));
	DumpNetEnum(m_pBufShares, m_cShares);
	m_pHash->Print();
}
#endif // DBG == 1


#if DBG == 1

//+-------------------------------------------------------------------------
//
//  Function:   DumpNetEnum
//
//  Synopsis:   Dumps an array of SHARE_INFO_502 structures.
//
//  History:    4-Apr-95    BruceFo  Created
//
//--------------------------------------------------------------------------

VOID
DumpNetEnum(
    IN LPVOID pBufShares,
    IN ULONG entriesRead
    )
{
    SHARE_INFO_502* pBase = (SHARE_INFO_502*) pBufShares;

    appDebugOut((DEB_TRACE,
        "DumpNetEnum: %d entries\n",
        entriesRead));

    for (ULONG i = 0; i < entriesRead; i++)
    {
        SHARE_INFO_502* p = &(pBase[i]);

        appDebugOut((DEB_TRACE | DEB_NOCOMPNAME,
"\t Share name: %ws\n"
"\t       Type: %d (0x%08lx)\n"
"\t    Comment: %ws\n"
"\tPermissions: %d (0x%08lx)\n"
"\t   Max uses: %d\n"
"\t       Path: %ws\n"
"\t   Password: %ws\n"
"\t   Reserved: %d\n"
"\t   Security? %ws\n"
"\n"
,
p->shi502_netname,
p->shi502_type, p->shi502_type,
p->shi502_remark,
p->shi502_permissions, p->shi502_permissions,
p->shi502_max_uses,
p->shi502_path,
(NULL == p->shi502_passwd) ? L"none" : p->shi502_passwd,
p->shi502_reserved,
(NULL == p->shi502_security_descriptor) ? L"No" : L"Yes"
));

    }
}

#endif // DBG == 1
