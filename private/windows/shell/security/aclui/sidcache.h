//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       sidcache.h
//
//  This file contains definitions and prototypes for SID/Name cache.
//
//--------------------------------------------------------------------------

#ifndef _SIDCACHE_H_
#define _SIDCACHE_H_

#include <comctrlp.h>   // DPA

DWORD WaitOnThread(HANDLE *phThread);

#define BUCKET_COUNT    31

typedef struct _sid_cache_entry
{
    DWORD   dwLastAccessTime;
    SID_NAME_USE SidType;
    PSID    pSid;
    LPCTSTR pszName;
    LPCTSTR pszLogonName;
} SID_CACHE_ENTRY, *PSID_CACHE_ENTRY;


class CSidCache
{
private:
    HDPA m_dpaSidHashTable[BUCKET_COUNT];
    CRITICAL_SECTION m_csHashTableLock;
    CRITICAL_SECTION m_csDomainNameLock;
    CRITICAL_SECTION m_csDcNameLock;
    LPTSTR m_pszCachedServer;
    LPTSTR m_pszCachedDomain;
    HANDLE m_hInitThread;
    LPTSTR m_pszLastDc;
    LPTSTR m_pszLastDomain;
    LONG   m_cRef;

public:
    CSidCache();
    ~CSidCache();

    // used to control lifetime of the object
    ULONG AddRef();
    ULONG Release();

    BOOL LookupSids(HDPA hSids, LPCTSTR pszServer, LPSECURITYINFO2 psi2, PUSER_LIST *ppUserList);
    BOOL LookupSidsAsync(HDPA hSids, LPCTSTR pszServer, LPSECURITYINFO2 psi2, HWND hWndNotify, UINT uMsgNotify);
#if(_WIN32_WINNT >= 0x0500)
    BOOL LookupNames(PDS_SELECTION_LIST pDsSelList, LPCTSTR pszServer, PUSER_LIST *ppUserList, BOOL bStandalone);
#endif
    void GetDomainName(LPCTSTR pszServer, LPTSTR pszDomain, ULONG cchDomain);
    void GetDcName(LPCTSTR pszDomain, LPTSTR pszDC, ULONG cchDC);

    PSID_CACHE_ENTRY FindSid(PSID pSid);
    PSID_CACHE_ENTRY MakeEntry(PSID pSid,
                               SID_NAME_USE SidType,
                               LPCTSTR pszName,
                               LPCTSTR pszLogonName = NULL);
    BOOL AddEntry(PSID_CACHE_ENTRY pEntry);

    BOOL BuildUserList(HDPA hEntryList,
                       LPCTSTR pszServer,
                       PUSER_LIST *ppUserList);

private:
    int HashSid(PSID pSid);
    static int CALLBACK CompareSid(LPVOID p1, LPVOID p2, LPARAM lParam);

    void GetUserFriendlyName(LPCTSTR pszSamLogonName,
                             LPCTSTR pszSamAccountName,
                             LPCTSTR pszServer,
                             BOOL    bUseSamCompatibleInfo,
                             BOOL    bIsStandalone,
                             BSTR   *pstrLogonName,
                             BSTR   *pstrDisplayName);
    BSTR GetNT4DisplayName(LPCTSTR pszAccount,
                           LPCTSTR pszName,
                           LPCTSTR pszServer,
                           BOOL bStandalone);

    BOOL InternalLookupSids(HDPA hSids,
                            LPCTSTR pszServer,
                            LPSECURITYINFO2 psi2,
                            HDPA hEntryList,
                            HWND hWndNotify = NULL,
                            UINT uMsgNotify = 0);
    BOOL LookupSidsHelper(HDPA hSids,
                          LPCTSTR pszServer,
                          HDPA hEntryList,
                          HWND hWndNotify = NULL,
                          UINT uMsgNotify = 0,
                          BOOL bSecondTry = FALSE);
    HRESULT LookupSidsFromObject(HDPA hSids, LPSECURITYINFO2 psi2, HDPA hEntryList);

#if(_WIN32_WINNT >= 0x0500)
    BOOL InternalLookupNames(PDS_SELECTION_LIST pDsSelList,
                             LPCTSTR pszServer,
                             HDPA hEntryList,
                             BOOL bStandalone);
#endif

    static DWORD WINAPI InitThread(LPVOID pvThreadData);

    void Lock()     { EnterCriticalSection(&m_csHashTableLock); }
    void Unlock()   { LeaveCriticalSection(&m_csHashTableLock); }

    void LockDomain()   { EnterCriticalSection(&m_csDomainNameLock); }
    void UnlockDomain() { LeaveCriticalSection(&m_csDomainNameLock); }

    void LockDc()   { EnterCriticalSection(&m_csDcNameLock); }
    void UnlockDc() { LeaveCriticalSection(&m_csDcNameLock); }
};
typedef CSidCache *PSIDCACHE;

//
// Helper functions for creating/deleting the global SID Cache
//
PSIDCACHE GetSidCache();
void FreeSidCache();

#endif  // _SIDCACHE_H_
