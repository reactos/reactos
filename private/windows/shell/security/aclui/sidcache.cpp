//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       sidcache.cpp
//
//  This file contains the implementation of a SID/Name cache.
//
//--------------------------------------------------------------------------

#include "aclpriv.h"

#if(_WIN32_WINNT >= 0x0500)
#include <dsgetdc.h>    // DsGetDcName
#include <iads.h>
#endif

#define SECURITY_WIN32
#include <security.h>   // TranslateName
#include <lm.h>         // NetApiBufferFree
#include <shlwapi.h>    // StrChr, StrRChr

// 10 minutes
#define SID_CACHE_AGE_LIMIT     (10*60*1000)

TCHAR const c_szNTProvider[]    = TEXT("WinNT://");
#define NTPROV_LEN              (ARRAYSIZE(c_szNTProvider)-1)

PSIDCACHE g_pSidCache = NULL;

PSIDCACHE GetSidCache()
{
    if (NULL == g_pSidCache)
    {
        // The cache starts with an extra ref here that will be released
        // during our DLL_PROCESS_DETACH
        g_pSidCache = new CSidCache;

        if (g_pSidCache)
        {
            g_pSidCache->AddRef();
        }
    }
    else
    {
        g_pSidCache->AddRef();
    }

    return g_pSidCache;
}

void FreeSidCache()
{
    if (g_pSidCache)
    {
        g_pSidCache->Release();
        g_pSidCache = NULL;
    }
}


//
// CSidCache implementation
//

CSidCache::CSidCache()
: m_pszCachedServer(NULL), m_pszCachedDomain(NULL),
  m_hInitThread(NULL), m_pszLastDc(NULL), m_pszLastDomain(NULL),
  m_cRef(1)
{
    HINSTANCE hInstThisDll;
    DWORD dwThreadID;

    ZeroMemory(m_dpaSidHashTable, SIZEOF(m_dpaSidHashTable));
    InitializeCriticalSection(&m_csHashTableLock);
    InitializeCriticalSection(&m_csDomainNameLock);
    InitializeCriticalSection(&m_csDcNameLock);

    // Give the thread we are about to create a ref to the dll,
    // so that the dll will remain for the lifetime of the thread
    hInstThisDll = LoadLibrary(c_szDllName);
    if (hInstThisDll != NULL)
    {
        // also do an AddRef() for the worker thread to release later
        AddRef();

        // Start a thread to cache the well-known and built-in SIDs
        m_hInitThread = CreateThread(NULL, 0, InitThread, this, 0, &dwThreadID);

        if (!m_hInitThread)
        {
            // Failed to create the thread, do cleanup
            FreeLibrary(hInstThisDll);
            Release();
        }
    }
}

ULONG
CSidCache::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG
CSidCache::Release()
{
    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

CSidCache::~CSidCache()
{
    int i;

    TraceEnter(TRACE_SIDCACHE, "CSidCache::~CSidCache");

    Lock();
    for (i = 0; i < BUCKET_COUNT; i++)
    {
        DestroyDPA(m_dpaSidHashTable[i]);
        m_dpaSidHashTable[i] = NULL;
    }
    Unlock();

    LockDomain();
    LocalFreeString(&m_pszCachedServer);
    LocalFreeString(&m_pszCachedDomain);
    UnlockDomain();

    LockDc();
    LocalFreeString(&m_pszLastDc);
    LocalFreeString(&m_pszLastDomain);
    UnlockDc();

    DeleteCriticalSection(&m_csHashTableLock);
    DeleteCriticalSection(&m_csDomainNameLock);
    DeleteCriticalSection(&m_csDcNameLock);

    TraceLeaveVoid();
}


BOOL
CSidCache::LookupSids(HDPA hSids,
                      LPCTSTR pszServer,
                      LPSECURITYINFO2 psi2,
                      PUSER_LIST *ppUserList)
{
    BOOL fResult = FALSE;

    TraceEnter(TRACE_SIDCACHE, "CSidCache::LookupSids");
    TraceAssert(hSids != NULL);

    if (NULL == hSids)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        TraceLeaveValue(FALSE);
    }

    if (NULL != ppUserList)
        *ppUserList = NULL;

    if (0 != DPA_GetPtrCount(hSids))
    {
        HDPA hEntryList = DPA_Create(4);

        if (NULL == hEntryList)
            TraceLeaveValue(FALSE);

        InternalLookupSids(hSids, pszServer, psi2, hEntryList);

        if (0 != DPA_GetPtrCount(hEntryList) && NULL != ppUserList)
            fResult = BuildUserList(hEntryList, pszServer, ppUserList);

        DPA_Destroy(hEntryList);
    }

    TraceLeaveValue(fResult);
}


BOOL
CSidCache::LookupSidsAsync(HDPA hSids,
                           LPCTSTR pszServer,
                           LPSECURITYINFO2 psi2,
                           HWND hWndNotify,
                           UINT uMsgNotify)
{
    BOOL fResult = FALSE;

    TraceEnter(TRACE_SIDCACHE, "CSidCache::LookupSids");
    TraceAssert(hSids != NULL);

    if (NULL == hSids)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        TraceLeaveValue(FALSE);
    }

    if (0 != DPA_GetPtrCount(hSids))
    {
        fResult = InternalLookupSids(hSids,
                                     pszServer,
                                     psi2,
                                     NULL,
                                     hWndNotify,
                                     uMsgNotify);
    }

    TraceLeaveValue(fResult);
}


#if(_WIN32_WINNT >= 0x0500)
BOOL
CSidCache::LookupNames(PDS_SELECTION_LIST pDsSelList,
                       LPCTSTR pszServer,
                       PUSER_LIST *ppUserList,
                       BOOL bStandalone)
{
    BOOL fResult = FALSE;
    HDPA hEntryList;

    TraceEnter(TRACE_SIDCACHE, "CSidCache::LookupNames");
    TraceAssert(pDsSelList != NULL);
    TraceAssert(ppUserList != NULL);

    if (NULL == pDsSelList)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        TraceLeaveValue(FALSE);
    }

    if (NULL != ppUserList)
        *ppUserList = NULL;

    hEntryList = DPA_Create(4);

    if (NULL == hEntryList)
        TraceLeaveValue(FALSE);

    InternalLookupNames(pDsSelList, pszServer, hEntryList, bStandalone);

    if (0 != DPA_GetPtrCount(hEntryList))
    {
        fResult = TRUE; // so far, so good

        if (NULL != ppUserList)
            fResult = BuildUserList(hEntryList, pszServer, ppUserList);
    }

    DPA_Destroy(hEntryList);

    TraceLeaveValue(fResult);
}
#endif // #if(_WIN32_WINNT >= 0x0500)


void
CSidCache::GetDomainName(LPCTSTR pszServer, LPTSTR pszDomain, ULONG cchDomain)
{
    TraceEnter(TRACE_SIDCACHE, "CSidCache::GetDomainName");
    TraceAssert(NULL != pszDomain);
    TraceAssert(0 != cchDomain);

    pszDomain[0] = TEXT('\0');

    LockDomain();

    if (m_pszCachedDomain == NULL ||
        (pszServer == NULL && m_pszCachedServer != NULL) ||
        (pszServer != NULL && (m_pszCachedServer == NULL ||
         CompareString(LOCALE_USER_DEFAULT,
                       0,
                       pszServer,
                       -1,
                       m_pszCachedServer,
                       -1) != CSTR_EQUAL)))
    {
        //
        // It's a different server than last time, so ask LSA
        // for the domain name.
        //
        LocalFreeString(&m_pszCachedDomain);
        LocalFreeString(&m_pszCachedServer);

        if (pszServer != NULL)
            LocalAllocString(&m_pszCachedServer, pszServer);

        LSA_HANDLE hLSA = GetLSAConnection(pszServer, POLICY_VIEW_LOCAL_INFORMATION);
        if (hLSA != NULL)
        {
            PPOLICY_ACCOUNT_DOMAIN_INFO pDomainInfo = NULL;

            LsaQueryInformationPolicy(hLSA,
                                      PolicyAccountDomainInformation,
                                      (PVOID*)&pDomainInfo);
            if (pDomainInfo != NULL)
            {
                CopyUnicodeString(&m_pszCachedDomain, &pDomainInfo->DomainName);
                LsaFreeMemory(pDomainInfo);

                Trace((TEXT("Domain for %s is %s"), pszServer, m_pszCachedDomain));
            }
            LsaClose(hLSA);
        }
        else if (NULL != pszServer) // use the server name
        {
            // Skip leading backslashes
            while (TEXT('\\') == *pszServer)
                pszServer++;

            LocalAllocString(&m_pszCachedDomain, pszServer);

            if (m_pszCachedDomain)
            {
                // If there is a period, truncate the name at that point so
                // that something like "nttest.microsoft.com" becomes "nttest"
                LPTSTR pszDot = StrChr(m_pszCachedDomain, TEXT('.'));
                if (pszDot)
                    *pszDot = TEXT('\0');
            }
        }
    }

    if (m_pszCachedDomain)
        lstrcpyn(pszDomain, m_pszCachedDomain, cchDomain);

    UnlockDomain();

    TraceLeaveVoid();
}


DWORD
_GetDcName(LPCTSTR pszServer, LPCTSTR pszDomain, LPTSTR *ppszDC)
{
    DWORD dwErr;

    if (!ppszDC)
        return ERROR_INVALID_PARAMETER;

    *ppszDC = NULL;

#if(_WIN32_WINNT >= 0x0500)
    PDOMAIN_CONTROLLER_INFO pDCInfo = NULL;
    TraceMsg("Calling DsGetDcName");
    dwErr = DsGetDcName(pszServer,
                        pszDomain,
                        NULL,
                        NULL,
                        DS_IS_FLAT_NAME,
                        &pDCInfo);
    if (ERROR_SUCCESS == dwErr)
    {
        TraceAssert(NULL != pDCInfo);
        LocalAllocString(ppszDC, pDCInfo->DomainControllerName);
        NetApiBufferFree(pDCInfo);
    }
#else
    LPTSTR pszDcName = NULL;
    // NetGetAnyDCName only works for trusted domains, but is faster
    // and returns either PDC or BDC.  NetGetDCName returns only PDC.
    TraceMsg("Calling NetGetAnyDCName / NetGetDCName");
    dwErr = NetGetAnyDCName(pszServer, pszDomain, (LPBYTE*)&pszDcName);
    if (ERROR_NO_SUCH_DOMAIN == dwErr)
        dwErr = NetGetDCName(pszServer, pszDomain, (LPBYTE*)&pszDcName);
    if (pszDcName)
    {
        LocalAllocString(ppszDC, pszDcName);
        NetApiBufferFree(pszDcName);
    }
#endif

    if (ERROR_SUCCESS == dwErr && !*ppszDC)
        dwErr = ERROR_OUTOFMEMORY;

    return dwErr;
}

void
CSidCache::GetDcName(LPCTSTR pszDomain, LPTSTR pszDC, ULONG cchDC)
{
    TraceEnter(TRACE_SIDCACHE, "CSidCache::GetDcName");
    TraceAssert(NULL != pszDC);
    TraceAssert(0 != cchDC);

    pszDC[0] = TEXT('\0');

    LockDc();

    if (m_pszLastDc == NULL ||
        (pszDomain == NULL && m_pszLastDomain != NULL) ||
        (pszDomain != NULL && (m_pszLastDomain == NULL ||
         CompareString(LOCALE_USER_DEFAULT,
                       0,
                       pszDomain,
                       -1,
                       m_pszLastDomain,
                       -1) != CSTR_EQUAL)))
    {
        //
        // It's a different domain than last time, so look for a DC
        //
        LocalFreeString(&m_pszLastDc);
        LocalFreeString(&m_pszLastDomain);

        if (pszDomain != NULL)
            LocalAllocString(&m_pszLastDomain, pszDomain);

        _GetDcName(NULL, pszDomain, &m_pszLastDc);

        Trace((TEXT("DC for %s is %s"), pszDomain, m_pszLastDc));
    }

    if (m_pszLastDc)
        lstrcpyn(pszDC, m_pszLastDc, cchDC);

    UnlockDc();

    TraceLeaveVoid();
}


BSTR
CSidCache::GetNT4DisplayName(LPCTSTR pszAccount,
                             LPCTSTR pszName,
                             LPCTSTR pszServer,
                             BOOL bStandalone)
{
    BSTR strResult = NULL;
    TCHAR szComputer[UNCLEN];
    LPTSTR pszT = NULL;
    PUSER_INFO_2 pui = NULL;

    if (!pszAccount || !*pszAccount || !pszName || !*pszName)
        return NULL;

    TraceEnter(TRACE_SIDCACHE, "CSidCache::GetNT4DisplayName");

    if (!bStandalone
        && (pszT = StrChr(pszAccount, TEXT('\\'))))
    {
        // Copy the domain name
        TCHAR szDomain[DNLEN];
        lstrcpyn(szDomain,
                 pszAccount,
                 min((size_t)(pszT - pszAccount + 1), ARRAYSIZE(szDomain)));

        // See if we can use pszServer for NetUserGetInfo
        TCHAR szAccountDomain[DNLEN];
        szAccountDomain[0] = TEXT('\0');
        GetDomainName(pszServer, szAccountDomain, ARRAYSIZE(szAccountDomain));

        if (lstrcmpi(szDomain, szAccountDomain))
        {
            // Different domain, find a DC
            szComputer[0] = TEXT('\0');
            GetDcName(szDomain, szComputer, ARRAYSIZE(szComputer));
            if (TEXT('\0') != szComputer[0])
                pszServer = szComputer;
        }
    }

    TraceMsg("Calling NetUserGetInfo");
    if (NERR_Success == NetUserGetInfo(pszServer, pszName, 2, (LPBYTE *)&pui)
        && NULL != pui->usri2_full_name
        && *pui->usri2_full_name)
    {
        strResult = SysAllocString(pui->usri2_full_name);
    }

    NetApiBufferFree(pui);

    Trace((TEXT("Returning Full Name '%s' for '%s'"), strResult, pszAccount));
    TraceLeaveValue(strResult);
}


int
CSidCache::HashSid(PSID pSid)
{
    DWORD dwHash = 0;

    if (NULL != pSid)
    {
        PBYTE pbSid    = (PBYTE)pSid;
        PBYTE pbEndSid = pbSid + GetLengthSid(pSid);

        while (pbSid < pbEndSid)
            dwHash += *pbSid++;
    }

    return dwHash % BUCKET_COUNT;
}


int CALLBACK
CSidCache::CompareSid(LPVOID p1, LPVOID p2, LPARAM lParam)
{
    int nResult = 0;
    PSID_CACHE_ENTRY pEntry1 = (PSID_CACHE_ENTRY)p1;
    PSID_CACHE_ENTRY pEntry2 = (PSID_CACHE_ENTRY)p2;
    PSID pSid1 = NULL;
    PSID pSid2 = NULL;

    if (pEntry1)
        pSid1 = pEntry1->pSid;
    else if (lParam)
        pSid1 = (PSID)lParam;

    if (pEntry2)
        pSid2 = pEntry2->pSid;

    if (pSid1 == NULL)
        nResult = -1;
    else if (pSid2 == NULL)
        nResult = 1;
    else
    {
        DWORD dwLength = GetLengthSid(pSid1);

        // Compare SID lengths
        nResult = dwLength - GetLengthSid(pSid2);

        if (nResult == 0)
        {
            // Lengths are equal, compare the bits
            PBYTE pbSid1 = (PBYTE)pSid1;
            PBYTE pbSid2 = (PBYTE)pSid2;

            // Could compare Identifier Authorities and SubAuthorities instead
            while (nResult == 0 && dwLength != 0)
            {
                dwLength--;
                nResult = *pbSid1++ - *pbSid2++;
            }
        }
    }

    return nResult;
}


PSID_CACHE_ENTRY
CSidCache::FindSid(PSID pSid)
{
    PSID_CACHE_ENTRY pEntry = NULL;
    int iBucket;

    TraceEnter(TRACE_SIDCACHE, "CSidCache::FindSid");
    TraceAssert(pSid != NULL);
    TraceAssert(IsValidSid(pSid));

    iBucket = HashSid(pSid);

    Lock();

    if (m_dpaSidHashTable[iBucket] != NULL)
    {
        int iEntry = DPA_Search(m_dpaSidHashTable[iBucket],
                                NULL,
                                0,
                                CompareSid,
                                (LPARAM)pSid,
                                DPAS_SORTED);
        if (iEntry != -1)
        {
            pEntry = (PSID_CACHE_ENTRY)DPA_FastGetPtr(m_dpaSidHashTable[iBucket],
                                                      iEntry);
            TraceAssert(pEntry != NULL);
            TraceAssert(EqualSid(pSid, pEntry->pSid));

            if (0 != pEntry->dwLastAccessTime)
            {
                DWORD dwCurrentTime = GetTickCount();

                if ((dwCurrentTime - pEntry->dwLastAccessTime) > SID_CACHE_AGE_LIMIT)
                {
                    // The entry has aged out, remove it.
                    Trace((TEXT("Removing stale entry: %s"), pEntry->pszName));
                    DPA_DeletePtr(m_dpaSidHashTable[iBucket], iEntry);
                    LocalFree(pEntry);
                    pEntry = NULL;
                }
                else
                    pEntry->dwLastAccessTime = dwCurrentTime;
            }
        }
    }

    Unlock();

    TraceLeaveValue(pEntry);
}


PSID_CACHE_ENTRY
CSidCache::MakeEntry(PSID pSid,
                     SID_NAME_USE SidType,
                     LPCTSTR pszName,
                     LPCTSTR pszLogonName)
{
    PSID_CACHE_ENTRY pEntry = NULL;
    ULONG cbSid;
    ULONG cbName = 0;
    ULONG cbLogonName = 0;

    TraceEnter(TRACE_SIDCACHE, "CSidCache::MakeEntry");
    TraceAssert(pSid != NULL);

    cbSid = GetLengthSid(pSid);
    if (NULL != pszName && *pszName)
        cbName = StringByteSize(pszName);
    if (NULL != pszLogonName && *pszLogonName)
        cbLogonName = StringByteSize(pszLogonName);

    pEntry = (PSID_CACHE_ENTRY)LocalAlloc(LPTR,
                                          SIZEOF(SID_CACHE_ENTRY)
                                           + cbSid
                                           + cbName
                                           + cbLogonName);
    if (pEntry != NULL)
    {
        PBYTE pData = (PBYTE)(pEntry+1);

        pEntry->SidType = SidType;

        pEntry->pSid = (PSID)pData;
        CopyMemory(pData, pSid, cbSid);
        pData += cbSid;

        if (0 != cbName)
        {
            pEntry->pszName = (LPCTSTR)pData;
            CopyMemory(pData, pszName, cbName);
            pData += cbName;
        }

        if (0 != cbLogonName)
        {
            pEntry->pszLogonName = (LPCTSTR)pData;
            CopyMemory(pData, pszLogonName, cbLogonName);
            //pData += cbLogonName;
        }

        // Well-known entries never age out
        if (SidTypeWellKnownGroup == SidType || IsAliasSid(pSid))
            pEntry->dwLastAccessTime = 0;
        else
            pEntry->dwLastAccessTime = GetTickCount();
    }

    TraceLeaveValue(pEntry);
}


BOOL
CSidCache::AddEntry(PSID_CACHE_ENTRY pEntry)
{
    BOOL fResult = FALSE;
    int iSidBucket;

    TraceEnter(TRACE_SIDCACHE, "CSidCache::AddEntry");
    TraceAssert(pEntry != NULL);

    if (NULL == pEntry)
        TraceLeaveValue(FALSE);

    iSidBucket = HashSid(pEntry->pSid);

    Lock();

    if (m_dpaSidHashTable[iSidBucket] == NULL)
        m_dpaSidHashTable[iSidBucket] = DPA_Create(4);

    if (NULL != m_dpaSidHashTable[iSidBucket])
    {
        DPA_AppendPtr(m_dpaSidHashTable[iSidBucket], pEntry);
        DPA_Sort(m_dpaSidHashTable[iSidBucket], CompareSid, 0);
        fResult = TRUE;
    }

    Unlock();

    TraceLeaveValue(fResult);
}


BOOL
CSidCache::BuildUserList(HDPA hEntryList,
                         LPCTSTR pszServer,
                         PUSER_LIST *ppUserList)
{
    ULONG cEntries;
    TCHAR szAliasDomain[MAX_PATH];
    PSID_CACHE_ENTRY pEntry;
    ULONG cb;
    ULONG cbAliasDomain = 0;
    PBYTE pData;
    ULONG i;

    TraceEnter(TRACE_SIDCACHE, "CSidCache::BuildUserList");
    TraceAssert(hEntryList != NULL);
    TraceAssert(ppUserList != NULL);

    cEntries = DPA_GetPtrCount(hEntryList);
    TraceAssert(0 != cEntries);

    //
    // This name replaces "BUILTIN" for Alias SIDs
    //
    GetDomainName(pszServer, szAliasDomain, ARRAYSIZE(szAliasDomain));
    cbAliasDomain = StringByteSize(szAliasDomain);

    //
    // Add the sizes
    //
    cb = SIZEOF(USER_LIST) + ((cEntries - 1) * SIZEOF(USER_INFO));
    for (i = 0; i < cEntries; i++)
    {
        pEntry = (PSID_CACHE_ENTRY)DPA_FastGetPtr(hEntryList, i);
        TraceAssert(NULL != pEntry);

        cb += GetLengthSid(pEntry->pSid);

        if (SidTypeAlias == pEntry->SidType)
            cb += cbAliasDomain;
        else if (pEntry->pszLogonName)
            cb += StringByteSize(pEntry->pszLogonName);

        if (pEntry->pszName)
            cb += StringByteSize(pEntry->pszName);
    }

    //
    // Allocate and build the return buffer
    //
    *ppUserList = (PUSER_LIST)LocalAlloc(LPTR, cb);

    if (NULL == *ppUserList)
        TraceLeaveValue(FALSE);

    (*ppUserList)->cUsers = cEntries;
    pData = (PBYTE)&(*ppUserList)->rgUsers[cEntries];

    for (i = 0; i < cEntries; i++)
    {
        pEntry = (PSID_CACHE_ENTRY)DPA_FastGetPtr(hEntryList, i);
        TraceAssert(NULL != pEntry);

        (*ppUserList)->rgUsers[i].SidType = pEntry->SidType;

        TraceAssert(NULL != pEntry->pSid);
        (*ppUserList)->rgUsers[i].pSid = (PSID)pData;
        cb = GetLengthSid(pEntry->pSid);
        CopyMemory(pData, pEntry->pSid, cb);
        pData += cb;

        if (SidTypeAlias == pEntry->SidType)
        {
            (*ppUserList)->rgUsers[i].pszLogonName = (LPCTSTR)pData;

            // Copy the "BUILTIN" domain name
            if (cbAliasDomain)
            {
                CopyMemory(pData, szAliasDomain, cbAliasDomain);
                pData += cbAliasDomain - SIZEOF(TCHAR);

                if (NULL != pEntry->pszName)
                    *(LPTSTR)pData = TEXT('\\');
                else
                    *(LPTSTR)pData = TEXT('\0');

                pData += SIZEOF(TCHAR);
            }
            // The rest of the name is copied below
        }
        else  if (NULL != pEntry->pszLogonName)
        {
            (*ppUserList)->rgUsers[i].pszLogonName = (LPCTSTR)pData;
            cb = StringByteSize(pEntry->pszLogonName);
            CopyMemory(pData, pEntry->pszLogonName, cb);
            pData += cb;
        }

        if (NULL != pEntry->pszName)
        {
            (*ppUserList)->rgUsers[i].pszName = (LPCTSTR)pData;
            cb = StringByteSize(pEntry->pszName);
            CopyMemory(pData, pEntry->pszName, cb);
            pData += cb;
        }
    }

    TraceLeaveValue(TRUE);
}


//
// Wrapper around sspi's TranslateName that automatically handles
// the buffer sizing
//
HRESULT
TranslateNameInternal(LPCTSTR pszAccountName,
                      EXTENDED_NAME_FORMAT AccountNameFormat,
                      EXTENDED_NAME_FORMAT DesiredNameFormat,
                      BSTR *pstrTranslatedName)
{
#if(_WIN32_WINNT >= 0x0500)
#if DBG
    //
    // These match up with the EXTENDED_NAME_FORMAT enumeration.
    // They're for debugger output only.
    //
    static const LPCTSTR rgpszFmt[] = { 
                                TEXT("NameUnknown"),
                                TEXT("FullyQualifiedDN"),
                                TEXT("NameSamCompatible"),
                                TEXT("NameDisplay"),
                                TEXT("NameDomainSimple"),
                                TEXT("NameEnterpriseSimple"),
                                TEXT("NameUniqueId"),
                                TEXT("NameCanonical"),
                                TEXT("NameUserPrincipal"),
                                TEXT("NameCanonicalEx"),
                                TEXT("NameServicePrincipal") };
#endif // DBG

    TraceEnter(TRACE_SIDCACHE, "TranslateNameInternal");
    Trace((TEXT("Calling TranslateName for \"%s\""), pszAccountName));
    Trace((TEXT("Translating %s -> %s"), 
              rgpszFmt[AccountNameFormat], rgpszFmt[DesiredNameFormat]));

    if (!pszAccountName || !*pszAccountName || !pstrTranslatedName)
        TraceLeaveResult(E_INVALIDARG);

    HRESULT hr = NOERROR;
    //
    // cchTrans is static so that if a particular installation's
    // account names are really long, we'll not be resizing the
    // buffer for each account.
    //
    static ULONG cchTrans = MAX_PATH;
    ULONG cch = cchTrans;

    *pstrTranslatedName = SysAllocStringLen(NULL, cch);
    if (NULL == *pstrTranslatedName)
        ExitGracefully(hr, E_OUTOFMEMORY, "Unable to allocate name buffer");

    **pstrTranslatedName = L'\0';

    //
    // TranslateName is delay-loaded from secur32.dll using the linker's
    // delay-load mechanism.  Therefore, wrap with an exception handler.
    //
    __try
    {
        while(!::TranslateName(pszAccountName,
                               AccountNameFormat,
                               DesiredNameFormat,
                               *pstrTranslatedName,
                               &cch))
        {
            if (ERROR_INSUFFICIENT_BUFFER == GetLastError())
            {
                Trace((TEXT("Resizing buffer to %d chars"), cch));
                if (!SysReAllocStringLen(pstrTranslatedName, NULL, cch))
                    ExitGracefully(hr, E_OUTOFMEMORY, "Unable to reallocate name buffer");

                **pstrTranslatedName = L'\0';
            }
            else
            {
                hr = E_FAIL;
                break;
            }
        }

        cchTrans = max(cch, cchTrans);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = E_FAIL;
    }

exit_gracefully:

    if (FAILED(hr))
    {
        SysFreeString(*pstrTranslatedName);
        *pstrTranslatedName = NULL;
    }

    TraceLeaveResult(hr);
#else
    return E_NOTIMPL;
#endif  // _WIN32_WINNT >= 0x0500
}


void
CSidCache::GetUserFriendlyName(LPCTSTR pszSamLogonName,
                               LPCTSTR pszSamAccountName,
                               LPCTSTR pszServer,
                               BOOL    bUseSamCompatibleInfo,
                               BOOL    bIsStandalone,
                               BSTR   *pstrLogonName,
                               BSTR   *pstrDisplayName)
{
    BSTR strFQDN = NULL;

    TraceEnter(TRACE_SIDCACHE, "CSidCache::GetUserFriendlyName");
    TraceAssert(NULL != pszSamLogonName);

    //
    // Start by getting the FQDN.  Cracking is most efficient when the
    // FQDN is the starting point.
    //
    // TranslateName takes a while to complete, so bUseSamCompatibleInfo
    // should be TRUE whenever possible, e.g. for local accounts on a non-DC
    // or anything where we know a FQDN doesn't exist.
    //
    if (!bUseSamCompatibleInfo &&
        FAILED(TranslateNameInternal(pszSamLogonName,
                                     NameSamCompatible,
                                     NameFullyQualifiedDN,
                                     &strFQDN)))
    {
        //
        // No FQDN available for this account.  Must be an NT4
        // account.  Return SAM-compatible info to the caller.
        //
        bUseSamCompatibleInfo = TRUE;
    }

    if (NULL != pstrLogonName)
    {
        *pstrLogonName = NULL;

        if (!bUseSamCompatibleInfo)
        {
            TranslateNameInternal(strFQDN,
                                  NameFullyQualifiedDN,
                                  NameUserPrincipal,
                                  pstrLogonName);
        }
    }

    if (NULL != pstrDisplayName)
    {
        *pstrDisplayName = NULL;

        if (bUseSamCompatibleInfo ||
            FAILED(TranslateNameInternal(strFQDN,
                                         NameFullyQualifiedDN,
                                         NameDisplay,
                                         pstrDisplayName)))
        {
            *pstrDisplayName = GetNT4DisplayName(pszSamLogonName,
                                                 pszSamAccountName,
                                                 pszServer,
                                                 bIsStandalone);
        }
    }

    SysFreeString(strFQDN);

    TraceLeaveVoid();
}


BOOL
CSidCache::InternalLookupSids(HDPA hSids,
                              LPCTSTR pszServer,
                              LPSECURITYINFO2 psi2,
                              HDPA hEntryList,
                              HWND hWndNotify,
                              UINT uMsgNotify)
{
    ULONG cSids;
    HDPA hUnknownSids;
    PSID_CACHE_ENTRY pEntry;
    ULONG i;

    TraceEnter(TRACE_SIDCACHE, "CSidCache::InternalLookupSids");
    TraceAssert(hSids != NULL);

    if (hSids == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        TraceLeaveValue(FALSE);
    }

    cSids = DPA_GetPtrCount(hSids);
    TraceAssert(0 != cSids);

    hUnknownSids = DPA_Create(4);

    if (NULL == hUnknownSids)
        TraceLeaveValue(FALSE);

    //
    // See if any exist in the cache already
    //
    for (i = 0; i < cSids; i++)
    {
        pEntry = FindSid((PSID)DPA_FastGetPtr(hSids, i));

        if (pEntry)
        {
            if (hWndNotify)
                PostMessage(hWndNotify, uMsgNotify, 0, (LPARAM)pEntry->pSid);
            else if (hEntryList)
                DPA_AppendPtr(hEntryList, pEntry);
        }
        else
            DPA_AppendPtr(hUnknownSids, DPA_FastGetPtr(hSids, i));
    }

    //
    // Call LSA to lookup any that we don't have cached
    //
    if (0 != DPA_GetPtrCount(hUnknownSids))
    {
        if (!psi2 ||
            FAILED(LookupSidsFromObject(hUnknownSids, psi2, hEntryList)))
        {
            LookupSidsHelper(hUnknownSids,
                             pszServer,
                             hEntryList,
                             hWndNotify,
                             uMsgNotify);
        }
    }

    DPA_Destroy(hUnknownSids);

    TraceLeaveValue(TRUE);
}

#if(_WIN32_WINNT >= 0x0500)
#include <adsnms.h>     // USER_CLASS_NAME, etc.
#else
#define COMPUTER_CLASS_NAME     TEXT("Computer")
#define USER_CLASS_NAME         TEXT("User")
#define GROUP_CLASS_NAME        TEXT("Group")
#define GLOBALGROUP_CLASS_NAME  TEXT("GlobalGroup")
#define LOCALGROUP_CLASS_NAME   TEXT("LocalGroup")
#endif

TCHAR const c_szForeignSecurityPrincipal[]  = TEXT("foreignSecurityPrincipal");

static const struct
{
    LPCTSTR pszClass;
    SID_NAME_USE sidType;
} c_aSidClasses[] =
{
    USER_CLASS_NAME,                    SidTypeUser,
    GROUP_CLASS_NAME,                   SidTypeGroup,
    GLOBALGROUP_CLASS_NAME,             SidTypeGroup,
    LOCALGROUP_CLASS_NAME,              SidTypeGroup,
    COMPUTER_CLASS_NAME,                SidTypeComputer,
    c_szForeignSecurityPrincipal,       SidTypeGroup,
};

SID_NAME_USE
GetSidType(PSID pSid, LPCTSTR pszClass)
{
    SID_NAME_USE sidType = SidTypeUnknown;

    TraceEnter(TRACE_SIDCACHE, "GetSidType");

    if (pSid)
    {
        TraceAssert(IsValidSid(pSid));

        if (EqualSystemSid(pSid, UI_SID_World) || IsCreatorSid(pSid))
            TraceLeaveValue(SidTypeWellKnownGroup);

        if (IsAliasSid(pSid))
            TraceLeaveValue(SidTypeAlias);

        if (*GetSidSubAuthorityCount(pSid) == 1 && IsNTAuthority(pSid))
        {
            DWORD sa = *GetSidSubAuthority(pSid, 0);
            if (sa && sa <= SECURITY_RESTRICTED_CODE_RID && sa != SECURITY_LOGON_IDS_RID)
                TraceLeaveValue(SidTypeWellKnownGroup);
            if (SECURITY_LOCAL_SYSTEM_RID == sa)
                TraceLeaveValue(SidTypeWellKnownGroup);
        }
    }

    if (pszClass)
    {
        // Didn't recognize the SID, try the class name
        for (int i = 0; i < ARRAYSIZE(c_aSidClasses); i++)
        {
            if (!lstrcmpi(pszClass, c_aSidClasses[i].pszClass))
                TraceLeaveValue(c_aSidClasses[i].sidType);
        }
        Trace((TEXT("Unexpected class type: %s"), pszClass));
    }

    // Don't know what type it is, so take a guess.  This is just
    // for picking an icon, so it doesn't matter too much.
    TraceLeaveValue(SidTypeUser); // SidTypeGroup would be just as valid
}

HRESULT
CSidCache::LookupSidsFromObject(HDPA hSids,
                                LPSECURITYINFO2 psi2,
                                HDPA hEntryList)
{
    HRESULT hr;
    ULONG cSids;
    LPDATAOBJECT pdoNames = NULL;
    STGMEDIUM medium = {0};
    FORMATETC fe = { (CLIPFORMAT)g_cfSidInfoList, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    PSID_INFO_LIST pSidList = NULL;
    UINT i;

    TraceEnter(TRACE_SIDCACHE, "CSidCache::LookupSidsFromObject");
    TraceAssert(hSids != NULL);
    TraceAssert(psi2 != NULL);

    cSids = DPA_GetPtrCount(hSids);
    TraceAssert(cSids != 0);

    hr = psi2->LookupSids(cSids, DPA_GetPtrPtr(hSids), &pdoNames);
    FailGracefully(hr, "ISecurityInformation2::LookupSids failed");

    hr = pdoNames->GetData(&fe, &medium);
    FailGracefully(hr, "Unable to get CFSTR_ACLUI_SID_INFO_LIST from DataObject");

    pSidList = (PSID_INFO_LIST)GlobalLock(medium.hGlobal);
    if (!pSidList)
        ExitGracefully(hr, E_FAIL, "Unable to lock stgmedium.hGlobal");

    TraceAssert(pSidList->cItems > 0);

    for (i = 0; i < pSidList->cItems; i++)
    {
        PSID_CACHE_ENTRY pEntry = MakeEntry(pSidList->aSidInfo[i].pSid,
                                            GetSidType(pSidList->aSidInfo[i].pSid,
                                                       pSidList->aSidInfo[i].pwzClass),
                                            pSidList->aSidInfo[i].pwzCommonName,
                                            pSidList->aSidInfo[i].pwzUPN);
        if (pEntry)
        {
            if (AddEntry(pEntry))
            {
                if (hEntryList)
                    DPA_AppendPtr(hEntryList, pEntry);
            }
            else
                LocalFree(pEntry);
        }
    }

exit_gracefully:

    if (pSidList)
        GlobalUnlock(medium.hGlobal);
    ReleaseStgMedium(&medium);
    DoRelease(pdoNames);

    TraceLeaveResult(hr);
}

BOOL
CSidCache::LookupSidsHelper(HDPA hSids,
                            LPCTSTR pszServer,
                            HDPA hEntryList,
                            HWND hWndNotify,
                            UINT uMsgNotify,
                            BOOL bSecondTry)
{
    BOOL fResult = FALSE;
    ULONG cSids;
    LSA_HANDLE hlsa = NULL;
    PLSA_REFERENCED_DOMAIN_LIST pRefDomains = NULL;
    PLSA_TRANSLATED_NAME pTranslatedNames = NULL;
    DWORD dwStatus;
    LPTSTR pszDeletedAccount = NULL;
    BOOL bIsDC = FALSE;
    BOOL bIsStandalone = FALSE;
    HDPA hUnknownSids = NULL;

    TraceEnter(TRACE_SIDCACHE, "CSidCache::LookupSidsHelper");
    TraceAssert(hSids != NULL);

    cSids = DPA_GetPtrCount(hSids);
    if (!cSids)
        TraceLeaveValue(FALSE);

    //
    // Call LSA to lookup SIDs for the names
    //
    hlsa = GetLSAConnection(pszServer, POLICY_LOOKUP_NAMES);
    if (NULL == hlsa && NULL != pszServer && !bSecondTry)
    {
        // Try the local machine
        pszServer = NULL;
        hlsa = GetLSAConnection(NULL, POLICY_LOOKUP_NAMES);
    }
    if (NULL == hlsa)
        TraceLeaveValue(FALSE);

    dwStatus = LsaLookupSids(hlsa,
                             cSids,
                             DPA_GetPtrPtr(hSids),
                             &pRefDomains,
                             &pTranslatedNames);

    bIsStandalone = IsStandalone(pszServer, &bIsDC);

    if (STATUS_SUCCESS == dwStatus || STATUS_SOME_NOT_MAPPED == dwStatus)
    {
        TraceAssert(pTranslatedNames);
        TraceAssert(pRefDomains);

        //
        // Build cache entries with NT4 style names
        //
        for (ULONG i = 0; i < cSids; i++)
        {
            BOOL bTryUPN = TRUE;
            BSTR strLogonName = NULL;
            BSTR strDisplayName = NULL;

            PLSA_TRANSLATED_NAME pLsaName = &pTranslatedNames[i];
            PLSA_TRUST_INFORMATION pLsaDomain = NULL;
            PSID pSid = DPA_FastGetPtr(hSids, i);

            TCHAR szAccountName[MAX_PATH];
            TCHAR szDomainName[MAX_PATH];

            BOOL bNoCache = FALSE;

            szAccountName[0] = TEXT('\0');
            szDomainName[0] = TEXT('\0');

            // Get the referenced domain, if any
            if (pLsaName->DomainIndex >= 0 && pRefDomains)
            {
                TraceAssert((ULONG)pLsaName->DomainIndex < pRefDomains->Entries);
                pLsaDomain = &pRefDomains->Domains[pLsaName->DomainIndex];
            }

            // Make NULL-terminated copies of the domain and account name strings
            CopyUnicodeString(szAccountName, ARRAYSIZE(szAccountName), &pLsaName->Name);
            if (pLsaDomain)
                CopyUnicodeString(szDomainName, ARRAYSIZE(szDomainName), &pLsaDomain->Name);

            // Some optimization to avoid TranslateName when possible
            if (!bIsDC)
            {
                if (bIsStandalone)
                {
                    // Non-DC, standalone, therefore no UPN
                    bTryUPN = FALSE;
                }
                else if (SidTypeUser == pLsaName->Use)
                {
                    TCHAR szTargetDomain[DNLEN];
                    szTargetDomain[0] = TEXT('\0');
                    GetDomainName(pszServer, szTargetDomain, ARRAYSIZE(szTargetDomain));
                    if (CSTR_EQUAL == CompareString(LOCALE_USER_DEFAULT,
                                                    NORM_IGNORECASE,
                                                    szTargetDomain,
                                                    -1,
                                                    szDomainName,
                                                    -1))
                    {
                        // Local account on non-DC, therefore no UPN
                        bTryUPN = FALSE;
                    }
                }
            }

            //
            // Build NT4 "domain\user" style name
            //
            if (szDomainName[0] != TEXT('\0'))
            {
                lstrcat(szDomainName, TEXT("\\"));
                lstrcat(szDomainName, szAccountName);
            }

            // What we've got so far is our baseline.
            // Adjust these based on SID type.
            LPTSTR pszName = szAccountName;
            LPTSTR pszLogonName = szDomainName;

            switch (pLsaName->Use)
            {
            case SidTypeUser:               // 1
                // Get "User Principal Name" etc.
                GetUserFriendlyName(pszLogonName,
                                    pszName,
                                    pszServer,
                                    !bTryUPN,
                                    bIsStandalone,
                                    &strLogonName,
                                    &strDisplayName);
                if (strLogonName)
                    pszLogonName = strLogonName;
                if (strDisplayName)
                    pszName = strDisplayName;
                break;

            case SidTypeGroup:              // 2
            case SidTypeDomain:             // 3
                // nothing
                break;

            case SidTypeAlias:              // 4
                if (!IsAliasSid(pSid))
                {
                    // Sometimes get SidTypeAlias for non-BUILTIN sids,
                    // e.g. Domain Local Groups. Treat these as groups
                    // so we don't replace the Domain name.
                    // Raid #383755
                    pLsaName->Use = SidTypeGroup;
                    break;
                }
                // else Fall Through
            case SidTypeWellKnownGroup:     // 5
                // No logon name for these
                pszLogonName = NULL;
                break;

            case SidTypeDeletedAccount:     // 6
                // Display "Account Deleted"
                if (!pszDeletedAccount)
                    LoadStringAlloc(&pszDeletedAccount, hModule, IDS_SID_DELETED);
                if (pszDeletedAccount)
                    pszName = pszDeletedAccount;
                pszLogonName = NULL;
                break;

            case SidTypeInvalid:            // 7
                bNoCache = TRUE;
                break;

            case SidTypeUnknown:            // 8
                // Some SIDs can only be looked up on a DC, so
                // if pszServer is not a DC, remember them and
                // look them up on a DC after this loop is done.
                if (!bSecondTry && !bIsStandalone && !bIsDC)
                {
                    if (!hUnknownSids)
                        hUnknownSids = DPA_Create(4);
                    if (hUnknownSids)
                        DPA_AppendPtr(hUnknownSids, pSid);
                }
                bNoCache = TRUE;
                break;

    #if(_WIN32_WINNT >= 0x0500)
            case SidTypeComputer:           // 9
                if (*pszName)
                {
                    // Strip the trailing '$'
                    int nLen = lstrlen(pszName);
                    if (nLen && pszName[nLen-1] == TEXT('$'))
                    {
                        pszName[nLen-1] = TEXT('\0');
                    }
                }
                break;
    #endif
            }

            if (!bNoCache)
            {
                //
                // Make a cache entry and save it
                //
                PSID_CACHE_ENTRY pEntry = MakeEntry(pSid,
                                                    pLsaName->Use,
                                                    pszName,
                                                    pszLogonName);
                if (pEntry)
                {
                    if (AddEntry(pEntry))
                    {
                        fResult = TRUE; // we added something to the cache

                        if (hWndNotify)
                            PostMessage(hWndNotify, uMsgNotify, 0, (LPARAM)pEntry->pSid);
                        else if (hEntryList)
                            DPA_AppendPtr(hEntryList, pEntry);
                    }
                    else
                        LocalFree(pEntry);
                }
            }

            if (strLogonName)
                SysFreeString(strLogonName);
            if (strDisplayName)
                SysFreeString(strDisplayName);
        }
    }
    else if (STATUS_NONE_MAPPED == dwStatus && !bSecondTry && !bIsStandalone && !bIsDC)
    {
        hUnknownSids = DPA_Clone(hSids, NULL);
    }

    // Cleanup
    if (pTranslatedNames)
        LsaFreeMemory(pTranslatedNames);
    if (pRefDomains)
        LsaFreeMemory(pRefDomains);
    LsaClose(hlsa);
    LocalFreeString(&pszDeletedAccount);

    if (hUnknownSids)
    {
        //
        // Some (or all) SIDs were unknown on the target machine,
        // try a DC for the target machine's primary domain.
        //
        // This typically happens for certain Alias SIDs, such
        // as Print Operators and System Operators, for which LSA
        // only returns names if the lookup is done on a DC.
        //
        LPTSTR pszDC = NULL;

        TraceAssert(!bSecondTry);

        // We don't bother trying if standalone, and don't
        // do this if the target machine is already a DC.
        TraceAssert(!bIsStandalone && !bIsDC);

        _GetDcName(pszServer, NULL, &pszDC);

        if (pszDC)
        {
            // Recurse
            if (LookupSidsHelper(hUnknownSids,
                                 pszDC,
                                 hEntryList,
                                 hWndNotify,
                                 uMsgNotify,
                                 TRUE))
            {
                fResult = TRUE;
            }
            LocalFree(pszDC);
        }

        DPA_Destroy(hUnknownSids);
    }

    TraceLeaveValue(fResult);
}


#if(_WIN32_WINNT >= 0x0500)

BSTR GetNT4AccountName(LPTSTR pszWinNTPath)
{
    // pszWinNTPath is expected to look like
    //   "WinNT://domain/user"
    // or
    //   "WinNT://domain/machine/user"
    //
    // The "WinNT://" part is optional.
    //
    // In either case, we want the last 2 elements,
    // e.g. "domain/user" and "machine/user".
    //
    // The approach is to find the next to last '/' and add 1.
    // If there are less than 2 slashes, return the original string.

    BSTR strResult = NULL;
    LPTSTR pszResult = pszWinNTPath;
    if (pszWinNTPath)
    {
        LPTSTR pszSlash = StrRChr(pszWinNTPath, pszWinNTPath + lstrlen(pszWinNTPath) - 1, TEXT('/'));
        if (pszSlash)
        {
            pszSlash = StrRChr(pszWinNTPath, pszSlash-1, TEXT('/'));
            if (pszSlash)
                pszResult = pszSlash + 1;
        }
    }

    if (pszResult)
    {
        strResult = SysAllocString(pszResult);
        if (strResult)
        {
            // At this point, there is at most one forward slash
            // in the string.  Convert it to a backslash.
            LPTSTR pszSlash = StrChr(strResult, TEXT('/'));
            if (pszSlash)
                *pszSlash = TEXT('\\');
        }
    }

    return strResult;
}

BOOL
_LookupName(LPCTSTR pszServer,
            LPCTSTR pszAccount,
            PSID *ppSid,
            SID_NAME_USE *pSidType)
{
    BOOL fResult = FALSE;
    BYTE buffer[sizeof(SID) + SID_MAX_SUB_AUTHORITIES*sizeof(ULONG)];
    PSID pSid = (PSID)buffer;
    DWORD cbSid = sizeof(buffer);
    TCHAR szDomain[MAX_PATH];
    DWORD cchDomain = ARRAYSIZE(szDomain);
    SID_NAME_USE sidType;

    fResult = LookupAccountName(pszServer,
                                pszAccount,
                                pSid,
                                &cbSid,
                                szDomain,
                                &cchDomain,
                                &sidType);
    if (fResult)
    {
        *ppSid = LocalAllocSid(pSid);
        if (*ppSid)
        {
            if (pSidType)
                *pSidType = sidType;
        }
        else
            fResult = FALSE;
    }

    return fResult;
}

BOOL
CSidCache::InternalLookupNames(PDS_SELECTION_LIST pDsSelList,
                               LPCTSTR pszServer,
                               HDPA hEntryList,
                               BOOL bStandalone)
{
    BOOL fResult = FALSE;
    ULONG cNames;
    HDPA hSids = NULL;
    PSID_CACHE_ENTRY pEntry;
    ULONG i;
    ULONG cNoSID = 0;
    HRESULT hrCom = E_FAIL;
    IADsPathname *pPath = NULL;

    TraceEnter(TRACE_SIDCACHE, "CSidCache::InternalLookupNames");
    TraceAssert(pDsSelList != NULL);
    TraceAssert(hEntryList != NULL);

    if (pDsSelList == NULL || hEntryList == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        TraceLeaveValue(FALSE);
    }

    cNames = pDsSelList->cItems;
    TraceAssert(cNames != 0);

    if (0 == cNames)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        TraceLeaveValue(FALSE);
    }

    hSids = DPA_Create(4);

    for (i = 0; i < cNames; i++)
    {
        PSID pSid = NULL;
        PSID pSidFree = NULL;
        LPVARIANT pvarSid = pDsSelList->aDsSelection[i].pvarFetchedAttributes;
        SID_NAME_USE sidType = SidTypeUnknown;
        BSTR strNT4Name = NULL;

        if (NULL == pvarSid || (VT_ARRAY | VT_UI1) != V_VT(pvarSid)
            || FAILED(SafeArrayAccessData(V_ARRAY(pvarSid), &pSid)))
        {
            // If there's no SID, then we can't use it in an ACL
            Trace((TEXT("No SID returned for %s"), pDsSelList->aDsSelection[i].pwzADsPath));

            // If it's the NT provider, try to lookup the SID by name
            if (CSTR_EQUAL == CompareString(LOCALE_USER_DEFAULT,
                                            0,
                                            c_szNTProvider,
                                            NTPROV_LEN,
                                            pDsSelList->aDsSelection[i].pwzADsPath,
                                            NTPROV_LEN))
            {
                strNT4Name = GetNT4AccountName(pDsSelList->aDsSelection[i].pwzADsPath + NTPROV_LEN);
                if (strNT4Name)
                {
                    Trace((TEXT("Using LSA to lookup SID for %s"), strNT4Name));
                    if (_LookupName(pszServer, strNT4Name, &pSidFree, &sidType))
                    {
                        pSid = pSidFree;
                    }
                }
            }

            if (NULL == pSid)
            {
                cNoSID++;
                continue;
            }
        }
        TraceAssert(NULL != pSid);

        // Is it already in the cache?
        pEntry = FindSid(pSid);
        if (pEntry)
        {
            DPA_AppendPtr(hEntryList, pEntry);
        }
        else
        {
            // Not cached, try to make an entry using the info returned
            // by the object picker.
            if (SidTypeUnknown == sidType)
                sidType = GetSidType(pSid, pDsSelList->aDsSelection[i].pwzClass);

            if (!lstrcmpi(c_szForeignSecurityPrincipal, pDsSelList->aDsSelection[i].pwzClass))
            {
                // Object picker returns non-localized names for these (the
                // DS Configuration Container is not localized). Look up the
                // localized name from LSA.  175278

                // This happens automatically below (pEntry is NULL).
            }
            else if (SidTypeAlias == sidType || SidTypeWellKnownGroup == sidType)
            {
                // Only need the name
                pEntry = MakeEntry(pSid,
                                   sidType,
                                   pDsSelList->aDsSelection[i].pwzName,
                                   NULL);
            }
            else if (pDsSelList->aDsSelection[i].pwzUPN && *pDsSelList->aDsSelection[i].pwzUPN)
            {
                // We have both name and UPN
                pEntry = MakeEntry(pSid,
                                   sidType,
                                   pDsSelList->aDsSelection[i].pwzName,
                                   pDsSelList->aDsSelection[i].pwzUPN);
            }
            else if (CSTR_EQUAL == CompareString(LOCALE_USER_DEFAULT,
                                                 0,
                                                 c_szNTProvider,
                                                 NTPROV_LEN,
                                                 pDsSelList->aDsSelection[i].pwzADsPath,
                                                 NTPROV_LEN))
            {
                // It's downlevel ("WinNT://blah")
                if (NULL == strNT4Name)
                    strNT4Name = GetNT4AccountName(pDsSelList->aDsSelection[i].pwzADsPath + NTPROV_LEN);
                if (strNT4Name)
                {
                    // We have the NT4 name, now look for a Friendly Name
                    BSTR strDisplay = GetNT4DisplayName(strNT4Name,
                                                        pDsSelList->aDsSelection[i].pwzName,
                                                        pszServer,
                                                        bStandalone);
                    pEntry = MakeEntry(pSid,
                                       sidType,
                                       strDisplay ? strDisplay : pDsSelList->aDsSelection[i].pwzName,
                                       strNT4Name);
                    SysFreeString(strDisplay);
                }
            }
            else
            {
                // It's not a downlevel, so it must be
                //   1. WellKnown/Universal (no ADsPath)
                // or
                //   2. Uplevel ("GC:" or "LDAP:") but
                //      has no UPN
                //
                // If it has an ADs path, try to get an
                // NT4 name such as "NTDEV\Domain Users".
                //
                // Note that wellknown things such "Authenticated User"
                // can fall under either 1 or 2 above, depending on what
                // scope it was selected from.  That's why we try to pick
                // them off higher up.
                TraceAssert(NULL == strNT4Name);
                if (pDsSelList->aDsSelection[i].pwzADsPath &&
                    *pDsSelList->aDsSelection[i].pwzADsPath)
                {
                    // DsCrackNames doesn't accept full ADs paths, so use
                    // IADsPathname to retrieve the DN (no provider/server).
                    if (FAILED(hrCom))
                        hrCom = CoInitialize(NULL);
                    if (!pPath)
                    {
                        CoCreateInstance(CLSID_Pathname,
                                         NULL,
                                         CLSCTX_INPROC_SERVER,
                                         IID_IADsPathname,
                                         (LPVOID*)&pPath);
                    }
                    if (pPath)
                    {
                        BSTR strT;
                        if (SUCCEEDED(pPath->Set(pDsSelList->aDsSelection[i].pwzADsPath,
                                                 ADS_SETTYPE_FULL)))
                        {
                            if (SUCCEEDED(pPath->Retrieve(ADS_FORMAT_X500_DN,
                                                          &strT)))
                            {
                                // Try to get an NT4 account name
                                TranslateNameInternal(strT,
                                                      NameFullyQualifiedDN,
                                                      NameSamCompatible,
                                                      &strNT4Name);
                                SysFreeString(strT);
                            }
                            if (!strNT4Name)
                            {
                                // Retrieve or CrackName failed. Try to build
                                // an NT4-style name from the server name.
                                if (SUCCEEDED(pPath->Retrieve(ADS_FORMAT_SERVER,
                                                              &strT)))
                                {
                                    TCHAR szNT4Name[MAX_PATH];
                                    GetDomainName(strT, szNT4Name, ARRAYSIZE(szNT4Name));
                                    PathAppend(szNT4Name, pDsSelList->aDsSelection[i].pwzName);
                                    strNT4Name = SysAllocString(szNT4Name);
                                    SysFreeString(strT);
                                }
                            }
                        }
                    }
                }
                pEntry = MakeEntry(pSid,
                                   sidType,
                                   pDsSelList->aDsSelection[i].pwzName,
                                   strNT4Name);
            }

            //
            // Do we have a cache entry yet?
            //
            if (pEntry)
            {
                if (AddEntry(pEntry))
                {
                    DPA_AppendPtr(hEntryList, pEntry);
                }
                else
                {
                    LocalFree(pEntry);
                    pEntry = NULL;
                }
            }

            if (!pEntry && hSids)
            {
                // Look up the SID the hard way
                Trace((TEXT("Using LSA to lookup %s"), pDsSelList->aDsSelection[i].pwzADsPath));
                PSID pSidCopy = LocalAllocSid(pSid);
                if (pSidCopy)
                {
                    DPA_AppendPtr(hSids, pSidCopy);
                }
            }
        }

        SysFreeString(strNT4Name);

        if (pSidFree)
            LocalFree(pSidFree);
        else
            SafeArrayUnaccessData(V_ARRAY(pvarSid));
    }

    TraceAssert(0 == cNoSID);

    //
    // Call LSA to lookup names for the SIDs that aren't cached yet
    //
    if (hSids && 0 != DPA_GetPtrCount(hSids))
        LookupSidsHelper(hSids, pszServer, hEntryList);

    if (NULL != hSids)
        DestroyDPA(hSids);

    DoRelease(pPath);

    if (SUCCEEDED(hrCom))
        CoUninitialize();

    TraceLeaveValue(TRUE);
}

#endif  // #if(_WIN32_WINNT >= 0x0500)


DWORD WINAPI
CSidCache::InitThread(LPVOID pvThreadData)
{
    PSIDCACHE pThis = (PSIDCACHE)pvThreadData;

    // Our caller already gave us a ref on the dll to prevent the race window where 
    // we are created but we the dll is freed before we can call LoadLibrary()
    // HINSTANCE hInstThisDll = LoadLibrary(c_szDllName);

    TraceEnter(TRACE_SIDCACHE, "CSidCache::InitThread");

    if (pThis)
    {
        // Lookup some well-known SIDs to pre-load the cache
        HDPA hSids;
        hSids = DPA_Create(COUNT_SYSTEM_SID_TYPES);
        if (hSids)
        {
            for (int i = 0; i < COUNT_SYSTEM_SID_TYPES; i++)
            {
                DPA_AppendPtr(hSids, QuerySystemSid((UI_SystemSid)i));
            }

            pThis->LookupSidsHelper(hSids, NULL, NULL, NULL, 0);

            DPA_Destroy(hSids);
        }

        pThis->Release();
    }

    TraceLeave();
    FreeLibraryAndExitThread(GetModuleHandle(c_szDllName), 0);
    return 0;
}
