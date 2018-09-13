#include "advapi.h"
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <aclapi.h>
#include <windows.h>
#include <wincrypt.h>
#include <sitesids.h>
#include <malloc.h>
#include <urlmon.h>

RTL_CRITICAL_SECTION SiteSidCacheLock;

// internal function declarations
HRESULT MakeSidFromHash(PSID *ppSid, BYTE *pbBuffer, DWORD cbBuffer);
void UpdateSiteSidUsage(
            HKEY    hCacheKey,
            LPCWSTR lpwszSiteSid,
            LPWSTR  lpwszSite,
            UINT    cbSiteBuffer);
void Base32Encode(LPVOID pvData, UINT cbData, LPWSTR pchData);

typedef HRESULT (STDAPICALLTYPE CREATESECURITYMANAGER) (
                                                        IServiceProvider *pSP, IInternetSecurityManager **ppSM, DWORD dwReserved);

CREATESECURITYMANAGER *pfnCoInternetCreateSecurityManager = CoInternetCreateSecurityManager;
HMODULE hUrlMon = 0;



PSID
APIENTRY
GetSiteSidFromToken(
                    IN HANDLE TokenHandle
                    )
{
    PTOKEN_GROUPS RestrictedSids = NULL;
    ULONG ReturnLength;
    NTSTATUS Status;
    PSID psSiteSid = NULL;


    Status = NtQueryInformationToken(
        TokenHandle,
        TokenRestrictedSids,
        NULL,
        0,
        &ReturnLength
        );
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        BaseSetLastNTError(Status);
        return NULL;
    }

    RestrictedSids = (PTOKEN_GROUPS) RtlAllocateHeap(RtlProcessHeap(), 0, ReturnLength);
    if (RestrictedSids == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return NULL;
    }

    Status = NtQueryInformationToken(
        TokenHandle,
        TokenRestrictedSids,
        RestrictedSids,
        ReturnLength,
        &ReturnLength
        );
    if (NT_SUCCESS(Status))
    {
        UINT i;
        SID_IDENTIFIER_AUTHORITY InternetSiteAuthority = SECURITY_INTERNETSITE_AUTHORITY;

        for (i = 0; i < RestrictedSids->GroupCount; i++) {

            if (RtlCompareMemory((PVOID) &((SID *) RestrictedSids->Groups[i].Sid)->IdentifierAuthority,
                (PVOID) &InternetSiteAuthority,
                sizeof(SID_IDENTIFIER_AUTHORITY)) == sizeof(SID_IDENTIFIER_AUTHORITY))
            {
                psSiteSid = RtlAllocateHeap(RtlProcessHeap(), 0, RtlLengthSid((RestrictedSids->Groups[i]).Sid));
                if (psSiteSid == NULL) {
                    SetLastError(ERROR_OUTOFMEMORY);
                }
                else {
                    RtlCopySid(RtlLengthSid((RestrictedSids->Groups[i]).Sid), psSiteSid, (RestrictedSids->Groups[i]).Sid);
                }

                break;
            }

        }
    }
    else
    {
        BaseSetLastNTError(Status);
    }

    RtlFreeHeap(RtlProcessHeap(), 0, RestrictedSids);
    return psSiteSid;
}


HRESULT GetRestrictedSids(
                          LPCWSTR pszSite,
                          SID_AND_ATTRIBUTES *pSidToRestrict,
                          ULONG *pCount)
{
    HRESULT hr = S_OK;
    ULONG   i = 0;
    long error;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    *pCount = 0;

    pSidToRestrict[0].Attributes = SE_GROUP_MANDATORY | SE_GROUP_ENABLED;
    pSidToRestrict[1].Attributes = SE_GROUP_MANDATORY | SE_GROUP_ENABLED;
    pSidToRestrict[2].Attributes = SE_GROUP_MANDATORY | SE_GROUP_ENABLED;


    //Get the site SID.
    pSidToRestrict[0].Sid = GetSiteSidFromUrl(pszSite);
    if(!pSidToRestrict[0].Sid)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    i++;

    //BUGBUG: Get the zone SID.

    //Get the restricted SID.
    error = RtlAllocateAndInitializeSid(&NtAuthority,
        1,
        SECURITY_RESTRICTED_CODE_RID,
        0, 0, 0, 0, 0, 0, 0,
        &pSidToRestrict[i].Sid);
    if(!error)
    {
        i++;
    }
    else
    {
        hr =  HRESULT_FROM_WIN32( GetLastError() );
    }

    if(FAILED(hr))
    {
        return hr;
    }


    *pCount = i;
    return hr;
}


HRESULT
APIENTRY
GetSiteNameFromSid(PSID pSid, LPWSTR *pwsSite)
{
    HRESULT         hr = S_OK;
    WCHAR           wsValueNameBuffer[MAX_MANGLED_SITE];
    LPWSTR          wsValueName = wsValueNameBuffer;
    HKEY            hCacheKey;
    DWORD           dwDisposition;
    WCHAR          *wszValue;
    ULONG           ulValueSize;
    DWORD           error;

    *pwsSite = NULL;

    error = RegCreateKeyExW(HKEY_LOCAL_MACHINE, SITE_SID_CACHE_REG_KEY,
                            0, NULL, 0, KEY_ALL_ACCESS,
                            NULL, &hCacheKey, &dwDisposition);

    if (ERROR_SUCCESS != error)
    {
        // Can't write, try read-only.  We won't be able to update the cache,
        // but we might be able to return the site name

        error = RegCreateKeyExW(HKEY_LOCAL_MACHINE, SITE_SID_CACHE_REG_KEY,
                                0, NULL, 0, KEY_QUERY_VALUE,
                                NULL, &hCacheKey, &dwDisposition);

        if (ERROR_SUCCESS != error) {
            return HRESULT_FROM_WIN32(error);
        }
    }

    GetMangledSiteSid(pSid, MAX_MANGLED_SITE, &wsValueName);
    ASSERT(wsValueName == wsValueNameBuffer);

    //Get the size and allocate memory for the site name.
    error = RegQueryValueExW(
                    hCacheKey,
                    wsValueName,
                    NULL,
                    NULL,
                    NULL,
                    &ulValueSize);

    if (ERROR_SUCCESS != error) {
        RegCloseKey(hCacheKey);
        return HRESULT_FROM_WIN32(error);
    }

    wszValue = (WCHAR *) LocalAlloc(0, ulValueSize);

    if (wszValue != NULL)
    {
        error = RegQueryValueExW(
                        hCacheKey,
                        wsValueName,
                        NULL,
                        NULL,
                        (BYTE *) wszValue,
                        &ulValueSize);

        if (ERROR_SUCCESS == error)
        {
            *pwsSite = wszValue;

            UpdateSiteSidUsage(hCacheKey, wsValueName, wszValue, ulValueSize);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(error);
            LocalFree(wszValue);
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    RegCloseKey(hCacheKey);

    return hr;
}

PSID APIENTRY
GetSiteSidFromUrl(LPCWSTR wsUrl)
{
    HRESULT hr;
    PSID pSid = NULL;
    IInternetSecurityManager *pIScManager;
    DWORD cbSecurityId = lstrlenW(wsUrl) * sizeof(WCHAR) + sizeof(DWORD);
    BYTE *pbSecurityId = (BYTE *) alloca(cbSecurityId);

    HCRYPTPROV hProv;
    HCRYPTHASH hHash;

    DWORD dwCount;
    DWORD dwHashLen;

    BYTE *pbBuffer;

    HKEY hCacheKey;
    DWORD dwDisposition;

    WCHAR wsValueNameBuffer[MAX_MANGLED_SITE];
    LPWSTR wsValueName = wsValueNameBuffer;

    ULONG ulValueSize = (lstrlenW(wsUrl) + 1) * sizeof(WCHAR);
    WCHAR *wsValue;
    NTSTATUS Status;
    DWORD    error;

    //Crack the URL to get the site name.
    hr = (*pfnCoInternetCreateSecurityManager)(NULL, &pIScManager, 0);
    if(SUCCEEDED(hr))
    {
        hr = pIScManager->GetSecurityId(wsUrl, pbSecurityId, &cbSecurityId, 0);

        if(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) == hr)
        {
            pbSecurityId = (BYTE *) alloca(cbSecurityId);
            hr = pIScManager->GetSecurityId(wsUrl, pbSecurityId, &cbSecurityId, 0);
        }

        //Remove the dwZone from the end of the pbSecurityId.
        cbSecurityId -=  sizeof(DWORD);

        pIScManager->Release();
    }

    if(FAILED(hr))
    {
        SetLastError(hr);
        return NULL;
    }

    // acquire security context - if this is unsuccessful,
    // the hashing functions cannot be called
    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
        return NULL;

    if (!CryptCreateHash(hProv, SITE_SID_HASH_ALGORITHM, 0, 0, &hHash))
    {
        CryptReleaseContext(hProv, 0);
        return NULL;
    }

    // hash the site name
    if (!CryptHashData(hHash, pbSecurityId, cbSecurityId, 0))
    {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return NULL;
    }
    // TODO: salt? - with local machine name?

    // get size of the hash value - for memory allocation
    dwCount = sizeof(DWORD);
    if (!CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE *) &dwHashLen, &dwCount, 0))
    {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return NULL;
    }

    pbBuffer = (BYTE *) LocalAlloc(0, dwHashLen);
    if (pbBuffer == NULL)
    {
        //out of memory
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        SetLastError(ERROR_OUTOFMEMORY);
        return NULL;
    }
    if (!CryptGetHashParam(hHash, HP_HASHVAL, pbBuffer, &dwHashLen, 0))
    {
        LocalFree(pbBuffer);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return NULL;
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    // make SID from the hash value in pbBuffer
    hr = MakeSidFromHash(&pSid, pbBuffer, dwHashLen);
    if (FAILED(hr))
    {
        LocalFree(pbBuffer);
        SetLastError(hr);
        return NULL;
    }

    // check if the SID is already in the SID cache
    // if it is, update the time of last use
    // if not, insert it into the cache, deleting the LRU
    // item if the cache is already full

    // whatever happens here, we already have the mapping
    // from URL to SID that we wanted, so we always return
    // success

    //
    // Convert the cracked site name to Unicode
    //

    __try
    {
        wsUrl = (WCHAR *) alloca(ulValueSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return pSid;        // alloca failed
    }

    Status = RtlMultiByteToUnicodeN((WCHAR *) wsUrl, ulValueSize, &dwCount, (char *) pbSecurityId, cbSecurityId);
    if (!(NT_SUCCESS(Status)))
    {
        return pSid;
    }
    ((WCHAR *)wsUrl)[dwCount / sizeof(WCHAR)] = L'\0';

    hCacheKey = NULL;
    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, SITE_SID_CACHE_REG_KEY, 0, NULL, 0,
        KEY_ALL_ACCESS, NULL, &hCacheKey, &dwDisposition) != ERROR_SUCCESS)
    {
        // cannot open/create the cache registry key
        return pSid;
    }

    GetMangledSiteSid(pSid, MAX_MANGLED_SITE, &wsValueName);
    ASSERT(wsValueName == wsValueNameBuffer);

    __try
    {
        wsValue = (WCHAR *) alloca(ulValueSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        RegCloseKey(hCacheKey); // alloca failed
        return pSid;
    }

    error = RegQueryValueExW(
                    hCacheKey,
                    wsValueName,
                    NULL,
                    NULL,
                    (BYTE *) wsValue,
                    &ulValueSize);

    if (ERROR_SUCCESS == error) {
        // this SID is already there in the cache
        // check for collision

        // if wsValue is "", we already know of a collision
        if (wsValue[0] != L'\0') {
            if (wcscmp(wsUrl, wsValue) != 0) {
                // COLLISION !!!

                // we handle collision by retaining the SID in
                // the cache and setting the site name to ""
                // so the wrong site name cannot be returned

                DbgPrint("Site SID Collision:\n\t%ws\n\t%ws\n", wsUrl,wsValue);

                WCHAR wcNull = L'\0';
                RegSetValueExW(hCacheKey, wsValueName, 0, REG_BINARY,
                    (CONST BYTE *) &wcNull, sizeof(WCHAR));
            }
            else {
                UpdateSiteSidUsage(
                        hCacheKey,
                        wsValueName,
                        wsValue,
                        ulValueSize);
            }
        }
    }
    else if (ERROR_FILE_NOT_FOUND == error) {
        UpdateSiteSidUsage(hCacheKey, wsValueName, (WCHAR *) wsUrl, 0);
   }

   RegCloseKey(hCacheKey);

   return pSid;
}

void UpdateSiteSidUsage(
            HKEY    hCacheKey,
            LPCWSTR lpwszSiteSid,
            LPWSTR  lpwszSite,
            UINT    cbSiteBuffer)
{
    UINT    cbSite           = wcslen(lpwszSite)*sizeof(WCHAR) + sizeof(L'\0');
    UINT    cbRequiredBuffer = cbSite + sizeof(LARGE_INTEGER);
    BOOL    bCheckOverflow   = (0 == cbSiteBuffer);

    LARGE_INTEGER Now;

    if (cbSiteBuffer < cbRequiredBuffer)
    {
        // Either this is a new cache entry or somebody mucked with the
        // cache data
        LPWSTR lpwsz;

        __try
        {
            lpwsz = (LPWSTR) alloca(cbRequiredBuffer);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return;     // alloca failed
        }

        wcscpy(lpwsz, lpwszSite);
        cbSiteBuffer = cbRequiredBuffer;
        lpwszSite = lpwsz;
    }

    // Tack the current time after the end of the string and write it out

    NtQuerySystemTime(&Now);
    * (UNALIGNED LARGE_INTEGER *) (((BYTE *) lpwszSite) + cbSite) = Now;

    RegSetValueExW(
            hCacheKey,
            lpwszSiteSid,
            NULL,
            REG_BINARY,
            (BYTE *) lpwszSite,
            cbSiteBuffer);

    if (!bCheckOverflow)
        return;

    // Check for cache overflow

#define _PTIME(i)  ((__int64 *) (pEntryInfo + i * cbEntryInfo))
#define _PVALUE(i) ((WCHAR *) (_PTIME(i) + 1))

    static volatile LONG lFlushing = 0;

    DWORD   error;
    DWORD   cEntries;
    DWORD   cTotalEntries;
    DWORD   cbEntryValue;
    DWORD   cbMaxEntryValue;
    BYTE   *pEntryValue;
    DWORD   cbMaxEntryName;
    DWORD   cbEntryInfo;
    BYTE   *pEntryInfo;
    UINT    iFreeEntry;
    UINT    iNewestEntry;
    UINT    i;
    DWORD   cb;

    error = RegQueryInfoKeyW(hCacheKey, NULL, NULL, NULL, NULL, NULL, NULL,
                             &cEntries, &cbMaxEntryName, &cbMaxEntryValue,
                             NULL, NULL);

    if (ERROR_SUCCESS != error
        || cEntries < SITE_SID_CACHE_SIZE_HIGH
        || 1 == InterlockedExchange((LONG *) &lFlushing, 1))
    {
        return;     // Cache is under limit or we are already flushing
    }

    cbMaxEntryName = (cbMaxEntryName + 1) * sizeof(WCHAR);
    cTotalEntries = cEntries - SITE_SID_CACHE_SIZE_LOW + 1;
    cbEntryInfo = sizeof(LARGE_INTEGER) + cbMaxEntryName;

    // Allocate space for the largest value size

    __try
    {
        pEntryValue = (BYTE *) alloca(cbMaxEntryValue);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return;
    }

    // Allocate space to hold the cache entries to delete (+1)

    do
    {
        pEntryInfo = (BYTE *) LocalAlloc(0, cbEntryInfo * cTotalEntries);

        if (NULL == pEntryInfo)
        {
            cTotalEntries = cTotalEntries / 2;

            if (cTotalEntries < 2)
            {
                lFlushing = 0;
                return;
            }
        }
    }
    while (NULL == pEntryInfo);

    iFreeEntry = 0;
    iNewestEntry = 0;
    i = 0;
    cb = cbEntryInfo - sizeof(LARGE_INTEGER);
    cEntries = 1;
    BOOL bNewNewest = FALSE;
    cbEntryValue = cbMaxEntryValue;

    while (ERROR_SUCCESS == RegEnumValueW(
                                hCacheKey,
                                i,
                                _PVALUE(iFreeEntry),
                                &cb,
                                NULL,
                                NULL,
                                pEntryValue,
                                &cbEntryValue))
    {
        ++i;
        cb = cbEntryInfo - sizeof(LARGE_INTEGER);
        cbEntryValue = cbMaxEntryValue;

        * _PTIME(iFreeEntry) = * (__int64 *) (wcschr((WCHAR *) pEntryValue, L'\0') + 1);

        if (cEntries < cTotalEntries)
        {
            ++cEntries;
            ++iFreeEntry;
            if (cEntries == cTotalEntries)
                bNewNewest = TRUE;
        }
        else if (*_PTIME(iFreeEntry) < *_PTIME(iNewestEntry))
        {
            UINT t = iNewestEntry;
            iNewestEntry = iFreeEntry;
            iFreeEntry = t;
            bNewNewest = TRUE;
        }

        if (bNewNewest)
        {
            bNewNewest = FALSE;

            for (DWORD j = 0; j < cEntries; j++)
            {
                if (j != iFreeEntry)
                    if (*_PTIME(j) > *_PTIME(iNewestEntry))
                        iNewestEntry = j;
            }
        }
    }

    if (i >= SITE_SID_CACHE_SIZE_HIGH)
    {
        for (DWORD j = 0; j < cEntries; j++)
        {
            if (j != iFreeEntry)
                RegDeleteValueW(hCacheKey, _PVALUE(j));
        }
    }

    LocalFree(pEntryInfo);

    lFlushing = 0;

#undef _PVALUE
#undef _PTIME
}



HRESULT MakeSidFromHash(PSID *ppSid, BYTE *pbBuffer, DWORD cbBuffer)
{
    HRESULT hr = S_OK;
    long error;
    DWORD aSubAuthorities[8];
    SID_IDENTIFIER_AUTHORITY siaAuthority = SITE_SID_CACHE_AUTHORITY;

    *ppSid = NULL;

    if(cbBuffer > sizeof(aSubAuthorities))
    {
        return E_INVALIDARG;
    }

    memset(aSubAuthorities, 0, sizeof(aSubAuthorities));
    memcpy(aSubAuthorities, pbBuffer, cbBuffer);

    error = RtlAllocateAndInitializeSid(&siaAuthority,
        (BYTE) ((cbBuffer + 3) / sizeof(DWORD)),
        aSubAuthorities[0],
        aSubAuthorities[1],
        aSubAuthorities[2],
        aSubAuthorities[3],
        aSubAuthorities[4],
        aSubAuthorities[5],
        aSubAuthorities[6],
        aSubAuthorities[7],
        ppSid);
    if(error)
    {
        hr = HRESULT_FROM_WIN32(error);
    }
    return hr;
}

HRESULT
APIENTRY
GetMangledSiteSid(PSID pSid, ULONG cchMangledSite, LPWSTR *ppwszMangledSite)
{
    SID_IDENTIFIER_AUTHORITY InternetSiteAuthority = SECURITY_INTERNETSITE_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY *InAuthority;

    InAuthority = RtlIdentifierAuthoritySid(pSid);

    if (NULL == InAuthority)
        return HRESULT_FROM_WIN32(ERROR_INVALID_SID);

    if (0 != memcmp(
                InAuthority,
                &InternetSiteAuthority,
                sizeof(InternetSiteAuthority)))
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_SID);
    }

    if (cchMangledSite < MAX_MANGLED_SITE)
    {
        *ppwszMangledSite = (WCHAR *) LocalAlloc(
                                            0,
                                            MAX_MANGLED_SITE * sizeof(WCHAR));
        if (NULL == *ppwszMangledSite)
            return E_OUTOFMEMORY;
    }

    // The value of MAX_MANGLED_SITE assumes 4 dwords
    ASSERT(4 == *RtlSubAuthorityCountSid(pSid));

    Base32Encode(
            RtlSubAuthoritySid(pSid, 0),
            *RtlSubAuthorityCountSid(pSid) * sizeof(DWORD),
            *ppwszMangledSite);

    // The output string should always be MAX_MANGLED_SITE - 1 chars long
    ASSERT(MAX_MANGLED_SITE - 1 == lstrlenW(*ppwszMangledSite));

    return S_OK;
}


ULONG
APIENTRY
GetSiteDirectoryA(
                  HANDLE hToken,
                  LPSTR lpBuffer,
                  ULONG nBufferLength
                  )

/*++

Routine Description:

ANSI thunk to GetSiteDirectoryW

--*/

{
    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    DWORD ReturnValue;
#if defined (FE_SB) // GetSiteDirectoryA(); local variable
    ULONG cbAnsiString;
#endif // FE_SB

    if ( nBufferLength > MAXUSHORT ) {
        nBufferLength = MAXUSHORT-2;
    }

    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    Unicode->Length = (USHORT)GetSiteDirectoryW(
        hToken,
        Unicode->Buffer,
        Unicode->MaximumLength
        );

#if defined (FE_SB) // GetSiteDirectoryA(): bug fix
    //
    // Unicode->Length contains the byte count of unicode string.
    // Original code does "UnicodeLength / sizeof(WCHAR)" to
    // get the size of corresponding ansi string.
    // This is correct in SBCS environment. However in DBCS
    // environment, it's definitely WRONG.
    //
    Status = RtlUnicodeToMultiByteSize( &cbAnsiString, Unicode->Buffer, Unicode->Length );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        ReturnValue = 0;
    }
    else {
        if ( nBufferLength > (DWORD)(cbAnsiString ) ) {
            AnsiString.Buffer = lpBuffer;
            AnsiString.MaximumLength = (USHORT)(nBufferLength+1);
            Status = BasepUnicodeStringTo8BitString(&AnsiString,Unicode,FALSE);

            if ( !NT_SUCCESS(Status) ) {
                BaseSetLastNTError(Status);
                ReturnValue = 0;
            }
            else {
                ReturnValue = AnsiString.Length;
            }
        }
        else {
            //
            // current spec says the length doesn't
            // include null terminate character.
            // this may be a bug but I would like
            // to make this same as US (see US original code).
            //
            ReturnValue = cbAnsiString + 1;
        }
    }
#else
    if ( nBufferLength > (DWORD)(Unicode->Length>>1) ) {
        AnsiString.Buffer = lpBuffer;
        AnsiString.MaximumLength = (USHORT)(nBufferLength+1);
        Status = RtlUnicodeStringToAnsiString(&AnsiString,Unicode,FALSE);
        if ( !NT_SUCCESS(Status) ) {
            BaseSetLastNTError(Status);
            ReturnValue = 0;
        }
        else {
            ReturnValue = AnsiString.Length;
        }
    }
    else {
        ReturnValue = ((Unicode->Length)>>1)+1;
    }
#endif // FE_SB
    return ReturnValue;
}

/***************************************************************************\
* GetUserSid
*
* Allocs space for the user sid, fills it in and returns a pointer. Caller
* The sid should be freed by calling DeleteUserSid.
*
* Note the sid returned is the user's real sid, not the per-logon sid.
*
* Returns pointer to sid or NULL on failure.
*
* History:
* 26-Aug-92 Davidc      Created.
\***************************************************************************/
PSID GetUserSid (HANDLE UserToken)
{
  PTOKEN_USER pUser;
  PSID pSid;
  DWORD BytesRequired = 200;
  NTSTATUS status;


  //
  // Allocate space for the user info
  //

  pUser = (PTOKEN_USER)LocalAlloc(LMEM_FIXED, BytesRequired);


  if (pUser == NULL) {
      //        DebugMsg((DM_WARNING, TEXT("GetUserSid: Failed to allocate %d bytes"),
      //                  BytesRequired));
      return NULL;
  }


  //
  // Read in the UserInfo
  //

  status = NtQueryInformationToken(
      UserToken,                 // Handle
      TokenUser,                 // TokenInformationClass
      pUser,                     // TokenInformation
      BytesRequired,             // TokenInformationLength
      &BytesRequired             // ReturnLength
      );

  if (status == STATUS_BUFFER_TOO_SMALL) {

      //
      // Allocate a bigger buffer and try again.
      //

      pUser = (PTOKEN_USER)LocalReAlloc(pUser, BytesRequired, LMEM_MOVEABLE);
      if (pUser == NULL) {
          //            DebugMsg((DM_WARNING, TEXT("GetUserSid: Failed to allocate %d bytes"),
          //                      BytesRequired));
          return NULL;
      }

      status = NtQueryInformationToken(
          UserToken,             // Handle
          TokenUser,             // TokenInformationClass
          pUser,                 // TokenInformation
          BytesRequired,         // TokenInformationLength
          &BytesRequired         // ReturnLength
          );

  }

  if (!NT_SUCCESS(status)) {
      //        DebugMsg((DM_WARNING, TEXT("GetUserSid: Failed to query user info from user token, status = 0x%x"),
      //                  status));
      LocalFree(pUser);
      return NULL;
  }


  BytesRequired = RtlLengthSid(pUser->User.Sid);
  pSid = LocalAlloc(LMEM_FIXED, BytesRequired);
  if (pSid == NULL) {
      //        DebugMsg((DM_WARNING, TEXT("GetUserSid: Failed to allocate %d bytes"),
      //                  BytesRequired));
      LocalFree(pUser);
      return NULL;
  }


  status = RtlCopySid(BytesRequired, pSid, pUser->User.Sid);

  LocalFree(pUser);

  if (!NT_SUCCESS(status)) {
      //        DebugMsg((DM_WARNING, TEXT("GetUserSid: RtlCopySid Failed. status = %d"),
      //                  status));
      LocalFree(pSid);
      pSid = NULL;
  }


  return pSid;
}

BOOL
  CreateSiteDirectory(
  LPCWSTR pszSiteDirectory,
  PSID psidUser,
  PSID psidSite)
{
  BOOL bRetVal = FALSE;
  SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
  PACL pAcl = NULL;
  PSID psidSystem = NULL;
  PSID psidAdmin = NULL;
  DWORD cbAcl, AceIndex, dwDisp;
  ACE_HEADER * lpAceHeader;
  SECURITY_DESCRIPTOR sd;
  SECURITY_ATTRIBUTES saSite;


  //
  // Get the system sid
  //

  if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
      0, 0, 0, 0, 0, 0, 0, &psidSystem)) {
      goto Exit;
  }


  //
  // Get the admin sid
  //

  if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
      DOMAIN_ALIAS_RID_ADMINS, 0, 0,
      0, 0, 0, 0, &psidAdmin)) {
      goto Exit;
  }

  //
  // Allocate space for the ACL
  //

  cbAcl = (2 * GetLengthSid (psidUser)) + (2 * GetLengthSid (psidSystem)) +
      (2 * GetLengthSid (psidAdmin)) + (2 * GetLengthSid (psidSite)) +
      sizeof(ACL) +
      (8 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));


  pAcl = (PACL) GlobalAlloc(GMEM_FIXED, cbAcl);
  if (!pAcl) {
      goto Exit;
  }

  if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
      goto Exit;
  }


  //
  // Add Aces for User, System, and Admin.  Non-inheritable ACEs first
  //

  AceIndex = 0;
  if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidUser)) {
      goto Exit;
  }


  AceIndex++;
  if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidSystem)) {
      goto Exit;
  }

  AceIndex++;
  if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidAdmin)) {
      goto Exit;
  }

  AceIndex++;
  if (!AddAccessAllowedAce(pAcl, ACL_REVISION, FILE_ALL_ACCESS, psidSite)) {
      goto Exit;
  }



  //
  // Now the inheritable ACEs
  //

  AceIndex++;
  if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidUser)) {
      goto Exit;
  }

  if (!GetAce(pAcl, AceIndex, (void **)&lpAceHeader)) {
      goto Exit;
  }

  lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


  AceIndex++;
  if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidSystem)) {
      goto Exit;
  }

  if (!GetAce(pAcl, AceIndex, (void **)&lpAceHeader)) {
      goto Exit;
  }

  lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


  AceIndex++;
  if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidAdmin)) {
      goto Exit;
  }

  if (!GetAce(pAcl, AceIndex, (void **)&lpAceHeader)) {
      goto Exit;
  }

  lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);

  AceIndex++;
  if (!AddAccessAllowedAce(pAcl, ACL_REVISION, GENERIC_ALL, psidSite)) {
      goto Exit;
  }

  if (!GetAce(pAcl, AceIndex, (void **)&lpAceHeader)) {
      goto Exit;
  }

  lpAceHeader->AceFlags |= (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE);


  //
  // Put together the security descriptor
  //

  if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
      goto Exit;
  }


  if (!SetSecurityDescriptorDacl(&sd, TRUE, pAcl, FALSE)) {
      goto Exit;
  }


  //
  // Add the security descriptor to the sa structure
  //

  saSite.nLength = sizeof(SECURITY_ATTRIBUTES);
  saSite.lpSecurityDescriptor = &sd;
  saSite.bInheritHandle = FALSE;

  //
  // Attempt to create the directory
  //

  bRetVal = CreateDirectoryW(pszSiteDirectory, &saSite);

Exit:

  //
  // Free the sids and acl
  //

  if (psidSystem) {
      FreeSid(psidSystem);
  }

  if (psidAdmin) {
      FreeSid(psidAdmin);
  }

  if (pAcl) {
      GlobalFree (pAcl);
  }
  return bRetVal;
}

ULONG
APIENTRY
GetSiteDirectoryW(
                  HANDLE hToken,
                  LPWSTR pszSiteDirectory,
                  ULONG  uSize)
{
    ULONG  cb = 0;
    PSID   psidUser = 0;
    PSID   psidSite = 0;
    WCHAR  szProfile[MAX_PATH + 1];
    LPWSTR pszProfile = szProfile;
    HANDLE hProcessToken = 0;
    long   error = 0;

    //Get the process token.
    if(!hToken)
    {
        if(OpenProcessToken(GetCurrentProcess(),
            TOKEN_QUERY,
            &hProcessToken))
        {
            hToken = hProcessToken;
        }
        else
        {
            return 0;
        }
    }

    //Get the path to the user profile directory.
    psidUser = GetUserSid(hToken);
    if(psidUser != NULL)
    {
        UNICODE_STRING          wstrUserSid = {0, 0, 0};

        error = RtlConvertSidToUnicodeString(&wstrUserSid,
            psidUser,
            TRUE);
        if(!error)
        {
            HKEY hkUserProfile;

            //Read the registry key.
            lstrcpyW(szProfile, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\");
            lstrcatW(szProfile, wstrUserSid.Buffer);

            error = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                szProfile,
                0,
                KEY_READ,
                &hkUserProfile);
            if(!error)
            {
                DWORD dwType;
                DWORD dwSize = sizeof(szProfile);

                error = RegQueryValueExW(hkUserProfile,
                    L"ProfileImagePath",
                    NULL,
                    &dwType,
                    (BYTE*)szProfile,
                    &dwSize );
                if(!error)
                {
                    if ( dwType == REG_EXPAND_SZ )
                    {
                        pszProfile = (LPWSTR) alloca((MAX_PATH + 1) * sizeof(WCHAR));
                        ExpandEnvironmentStringsW(szProfile,
                            pszProfile,
                            MAX_PATH);
                    }
                }

                RegCloseKey(hkUserProfile);
            }

            RtlFreeUnicodeString(&wstrUserSid);
        }
    }
    else
    {
        error = GetLastError();
    }


    //Get the path to the site directory.
    if(!error)
    {
        psidSite = GetSiteSidFromToken(hToken);

        if(psidSite != NULL)
        {
            WCHAR wszSiteNameBuffer[MAX_MANGLED_SITE];
            LPWSTR wszSiteName = wszSiteNameBuffer;

            GetMangledSiteSid(psidSite, MAX_MANGLED_SITE, &wszSiteName);
            ASSERT(wszSiteName == wszSiteNameBuffer);

            cb = sizeof(WCHAR) * (lstrlenW(pszProfile)
                                  + sizeof('\\')
                                  + lstrlenW(wszSiteName)
                                  + sizeof('\0'));

            if(uSize * 2 < cb)
            {
                return cb/2;
            }
            else
            {
                lstrcpyW(pszSiteDirectory, pszProfile);
                lstrcatW(pszSiteDirectory, L"\\");
                lstrcatW(pszSiteDirectory, wszSiteName);

                //Check if the directory already exists.
                if(GetFileAttributesW(pszSiteDirectory) == -1)
                {
                    CreateSiteDirectory(pszSiteDirectory, psidUser, psidSite);
                }
            }
            RtlFreeSid(psidSite);
        }
        else
        {
            error = GetLastError();
        }
    }

    if(error)
    {
        SetLastError(error);
    }

    if(hProcessToken)
    {
        CloseHandle(hProcessToken);
    }

    if(psidUser)
    {
        RtlFreeSid(psidUser);
    }

    return cb;
}


BOOL APIENTRY
IsProcessRestricted(void)

/*++

Routine Description:

    Checks if the current process is a restricted process.

Arguments:

    None.

Return Value:

    TRUE if the current process is a restricted process.
    FALSE if it isn't, or if the handle of the current process
    cannot be obtained

Notes:

--*/

{
    static long fIsRestricted = -1;

    if(-1 == fIsRestricted)
    {
        HANDLE hToken;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hToken))
        {
            fIsRestricted = IsTokenRestricted(hToken);
            CloseHandle(hToken);
        }
        else
        {
            fIsRestricted = 0;
        }
    }
    return fIsRestricted;
}

WINADVAPI
BOOL
WINAPI IsInSandbox(VOID)
{
    return IsProcessRestricted();

}

//+-------------------------------------------------------------------
//
//  Function:   CoInternetCreateSecurityManager
//
//  Synopsis:   Loads urlmon.dll and calls CoInternetCreateSecurityManager.
//
//  Returns:    S_OK, ERROR_MOD_NOT_FOUND
//
//--------------------------------------------------------------------
STDAPI CoInternetCreateSecurityManager(
                                       IN  IServiceProvider *pSP,
                                       OUT IInternetSecurityManager **ppSM,
                                       IN DWORD dwReserved)
{
    HRESULT hr = E_FAIL;

    if(!hUrlMon)
    {
        hUrlMon = LoadLibraryA("urlmon.dll");
    }

    if(hUrlMon != 0)
    {
        void *pfn = GetProcAddress(hUrlMon, "CoInternetCreateSecurityManager");
        if(pfn != NULL)
        {
            pfnCoInternetCreateSecurityManager = (CREATESECURITYMANAGER *) pfn;
            hr = (*pfnCoInternetCreateSecurityManager)(pSP, ppSM, dwReserved);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}



//+----------------------------------------------------------------------------
//
//  Function:   Base32Encode
//
//  Synopsis:   Convert the given data to base32
//
//  Notes:      Adapted from Mim64Encode in the mshtml project.
//
//              For 128 bit input (4 DWORDs) the output string will be
//              27 chars long (including the null terminator)
//
//-----------------------------------------------------------------------------

void Base32Encode(LPVOID pvData, UINT cbData, LPWSTR pchData)
{
    static const WCHAR alphabet[32] =
        { L'a', L'b', L'c', L'd', L'e', L'f', L'g', L'h',
          L'i', L'j', L'k', L'l', L'm', L'n', L'o', L'p',
          L'q', L'r', L's', L't', L'u', L'v', L'w', L'x',
          L'y', L'z', L'0', L'1', L'2', L'3', L'4', L'5' };

    int   shift = 0;    // The # of unprocessed bits in accum
    ULONG accum = 0;    // The unprocessed bits
    ULONG value;
    BYTE *pData = (BYTE *) pvData;

    // For each byte...

    while (cbData)
    {
        // Move the byte into the low bits of the accumulator

        accum = (accum << 8) | *pData++;
        shift += 8;
        --cbData;

        // Lop off the high 5 or 10 bits and write them out

        while ( shift >= 5 )
        {
            shift -= 5;
            value = (accum >> shift) & 0x1Fl;

            *pchData++ = alphabet[value];
        }
    }

    // If there are any remaining bits, push out one more char padded with 0's

    if (shift)
    {
        value = (accum << (5 - shift)) & 0x1Fl;

        *pchData++ = alphabet[value];
    }

    *pchData = L'\0';
}

