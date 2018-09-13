//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       sharecnx.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "sharecnx.h"

//
// This class is a simple cache of net share names and some status flags.
// Initially, the only status maintained is to remember if there's an
// an open net connection for the share.
// The motivation for the cache is to avoid expensive net calls while
// we're cruising through lists of files (i.e. deleting files from the
// cache).  After we delete a file from the cache, it is effectively
// unpinned.  Therefore, we like to notify the shell so it can remove
// the "pinned" icon overlay from the affected file(s).  However,
// we don't want to hit the net with a change notify if there
// isn't an open connection to a file's parent share.  Before we issue
// a change notify, we just query this cache for a file using
// IsOpenConnectionPathUNC().  If there's no entry for the file's share,
// one is added and the connection status is obtained.  If there's
// already an entry, then we just return the status.  The public APIs
// support a "refresh" flag if refresh is desired.
// Additional status information could easily be added for each entry
// if it's needed later.
// [brianau - 12/12/98]
//

//-----------------------------------------------------------------------------
// CShareCnxStatusCache member functions.
//-----------------------------------------------------------------------------

CShareCnxStatusCache::CShareCnxStatusCache(
    void
    ) : m_hdpa(NULL)
{

}



CShareCnxStatusCache::~CShareCnxStatusCache(
    void
    )
{
    if (NULL != m_hdpa)
    {
        //
        // Delete all the entries then destroy the DPA.
        //
        int cEntries = Count();
        for (int i = 0; i < cEntries; i++)
        {
            delete GetEntry(i);
        }
        DPA_Destroy(m_hdpa);
    }
}


//
// Returns address of entry or NULL if not found.
//
CShareCnxStatusCache::Entry *
CShareCnxStatusCache::FindEntry(
    LPCTSTR pszShare
    ) const
{
    if (NULL != m_hdpa)
    {
        int cEntries = Count();
        for (int i = 0; i < cEntries; i++)
        {
            Entry *pEntry = GetEntry(i);
            if (NULL != pEntry && NULL != pEntry->Share())
            {
                if (0 == lstrcmpi(pszShare, pEntry->Share()))
                {
                    //
                    // Aha, we found a match.
                    //
                    return pEntry;
                }
            }
        }
    }
    return NULL;
}

    
//
// Creates a new entry and adds it to the DPA of entries.
// If successful, returns address of new entry.
// Does not check for duplicate entry before adding new one.
//
CShareCnxStatusCache::Entry *
CShareCnxStatusCache::AddEntry(
    LPCTSTR pszShare,
    DWORD dwStatus
    )
{
    Entry *pEntry = NULL;

    if (NULL == m_hdpa)
    {
        //
        // Must be first addition.  Create the DPA.
        //
        m_hdpa = DPA_Create(8);
    }

    if (NULL != m_hdpa)
    {
        int iEntry = -1;
        pEntry = new Entry(pszShare, dwStatus);
        if (NULL != pEntry && pEntry->IsValid())
        {
            //
            // We have a valid entry. Add it to the DPA.
            //
            iEntry = DPA_AppendPtr(m_hdpa, pEntry);
        }
        if (-1 == iEntry)
        {
            //
            // One of the following bad things happened:
            //
            //   1. Entry creation failed.  Most likely couldn't alloc string.
            //   2. Failed to add entry to DPA (out of memory).
            // 
            // Either way, destroy the entry and set the entry ptr so we 
            // return NULL.
            //
            delete pEntry;
            pEntry = NULL;
        }
    }
    return pEntry;
}        



//
// Determine if the net share associated with a UNC path (file or folder)
// has an open connection on this machine.
//
// Returns:
//    S_OK          = Open connection.
//    S_FALSE       = No open connection.
//    E_OUTOFMEMORY
//
HRESULT
CShareCnxStatusCache::IsOpenConnectionPathUNC(
    LPCTSTR pszPathUNC,
    bool bRefresh       // [optional].  Default = false.
    )
{
    //
    // Trim the path back to just the UNC share name.
    // Call IsOpenConnectionShare() to do the actual work.
    //
    TCHAR szShare[MAX_PATH * 2];
    lstrcpyn(szShare, pszPathUNC, ARRAYSIZE(szShare));
    PathStripToRoot(szShare);
    return IsOpenConnectionShare(szShare);
}



//
// Determine if the net share has an open connection on this machine.
//
// Returns:
//    S_OK     = Open connection.
//    S_FALSE  = No open connection.
//
HRESULT
CShareCnxStatusCache::IsOpenConnectionShare(
    LPCTSTR pszShare, 
    bool bRefresh       // [optional].  Default = false.
    )
{
    DWORD dwStatus    = 0;
    HRESULT hr = GetShareStatus(pszShare, &dwStatus, bRefresh);
    if (SUCCEEDED(hr))
    {
        if (0 != (dwStatus & Entry::StatusOpenCnx))
            hr = S_OK;
        else
            hr = S_FALSE;
    }
    return hr;
}



//
// Returns:
//
//      E_INVALIDARG = Path was not a UNC share.
//      S_OK         = Status is valid.
//
HRESULT
CShareCnxStatusCache::GetShareStatus(
    LPCTSTR pszShare, 
    DWORD *pdwStatus,
    bool bRefresh       // [optional].  Default = false.
    )
{
    HRESULT hr = E_INVALIDARG;  // Assume share name isn't UNC.
    *pdwStatus = 0;

    if (PathIsUNCServerShare(pszShare))
    {
        //
        // We have a valid UNC "\\server\share" name string.
        //
        Entry *pEntry = FindEntry(pszShare);
        if (NULL == pEntry)
        {
            //
            // Cache miss.  Get the system status for the share and try to 
            // cache it.
            //
            hr = Entry::QueryShareStatus(pszShare, pdwStatus);
            if (SUCCEEDED(hr))
            {
                //
                // Note that we don't return any errors from the cache attempt.
                // The only problem of not caching the entry is that the next 
                // call to this function will need to re-query the system for
                // the information.  This makes the cache meaningless but it's
                // not worth failing the information request.  Just slows things
                // down a bit.
                //
                AddEntry(pszShare, *pdwStatus);
            }
        }
        else 
        {
            //
            // Cache hit.
            //
            hr = S_OK;
            if (bRefresh)
            {
                //
                // Caller want's fresh info.
                //
                hr = pEntry->Refresh();
            }
            *pdwStatus = pEntry->Status();
            if (SUCCEEDED(hr))
                hr = pEntry->LastResult();
        }
    }
    return hr;
}



//
// Returns number of entries in the cache.
//
int 
CShareCnxStatusCache::Count(
    void
    ) const
{
    return (NULL != m_hdpa) ? DPA_GetPtrCount(m_hdpa) : 0;
}


//-----------------------------------------------------------------------------
// CShareCnxStatusCache::Entry member functions.
//-----------------------------------------------------------------------------

CShareCnxStatusCache::Entry::Entry(
    LPCTSTR pszShare,
    DWORD dwStatus
    ) : m_pszShare(StrDup(pszShare)),
        m_dwStatus(dwStatus),
        m_hrLastResult(NOERROR)
{
    if (NULL == m_pszShare)
    {
        m_hrLastResult = E_OUTOFMEMORY;
    }
}



CShareCnxStatusCache::Entry::~Entry(
    void
    )
{
    delete[] m_pszShare;
}



//
// Obtain new status info for the entry.
// Returns true if no errors obtaining info, false if there were errors.
//
HRESULT
CShareCnxStatusCache::Entry::Refresh(
    void
    )
{
    m_dwStatus     = 0;
    m_hrLastResult = E_OUTOFMEMORY;

    if (NULL != m_pszShare)
        m_hrLastResult = QueryShareStatus(m_pszShare, &m_dwStatus);

    return m_hrLastResult;
}



//
// Static function for obtaining entry status info from the
// system.  Made this a static function so the cache object
// can obtain information before creating the entry.  In case
// entry creation fails, we still want to be able to return
// valid status info.
//
HRESULT
CShareCnxStatusCache::Entry::QueryShareStatus(
    LPCTSTR pszShare,
    DWORD *pdwStatus
    )
{
    HRESULT hr = NOERROR;
    *pdwStatus = 0;

    //
    // Check the open connection status for this share.
    //
    hr = ::IsOpenConnectionShare(pszShare);
    switch(hr)
    {
        case S_OK:
            //
            // Open connection found.
            //
            *pdwStatus |= StatusOpenCnx;
            break;

        case S_FALSE:
            hr = S_OK;
            break;

        default:
            break;
    }

    //
    // If any other status information is required in the future,
    // here's where you collect it from the system.
    //
    return hr;
}



//
// Simple alloc-and-copy-a-string helper.
// Use operator delete[] to free the returned string.
//
LPTSTR 
CShareCnxStatusCache::Entry::StrDup(
    LPCTSTR psz
    )
{
    LPTSTR pszNew = new TCHAR[lstrlen(psz) + 1];
    if (NULL != pszNew)
        lstrcpy(pszNew, psz);

    return pszNew;
}
