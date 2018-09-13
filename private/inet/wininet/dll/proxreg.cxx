#include "wininetp.h"
#include "autodial.h"

const CHAR szRegPathConnections[] = REGSTR_PATH_INTERNET_SETTINGS "\\Connections";
static const CHAR szRegValProxyEnabled[] = REGSTR_VAL_PROXYENABLE;
static const CHAR szLegacyAutoConfigURL[] = "AutoConfigURL";

// when we haven't looked up dial-up override for autodetect
#define UNKNOWN_AUTODETECT      ((DWORD)(-1))

// Internet Settings reg value do determine dial-up override for autodetect
static const CHAR szDialupAutodetect[] = "DialupAutodetect";

// base hkey we use for settings.  May be per user or per machine.
HKEY    g_hkeyBase = NULL;

// type for RtlConvertSidToUnicodeString, exported from ntdll.dll
typedef NTSTATUS (* PCONVERTSID)(
    PUNICODE_STRING UnicodeString,
    PSID Sid,
    BOOLEAN AllocateDestinationString
    );

// some winsock stacks fault if we do a gethostbyname(NULL).  If we come
// accross one of these, don't do any more autodetecting.
BOOL g_fGetHostByNameNULLFails = FALSE;

//
// IsConnectionMatch - a worker function to simply some logic elsewhere,
//  it just handles Connection Name Matching.
//

BOOL 
IsConnectionMatch(
    LPCSTR lpszConnection1,
    LPCSTR lpszConnection2)
{
    if ( lpszConnection1 == NULL && 
         lpszConnection2 == NULL) 
    {
        return TRUE;
    }

    if ( lpszConnection1 && lpszConnection2 &&
         stricmp(lpszConnection1, lpszConnection2) == 0 )
    {
        return TRUE;
    }

    return FALSE;
}

//
// Decide whether or not autodiscovery is turned based on IEAK setting.
// Only used when upgrading or creating settings for a new connectoid.
//
BOOL
EnableAutodiscoverForDialup(
    VOID
    )
{
    static DWORD dwDialUpAutodetect = UNKNOWN_AUTODETECT;

    if(UNKNOWN_AUTODETECT == dwDialUpAutodetect)
    {
        DWORD   dwType, dwSize = sizeof(DWORD);

        if(ERROR_SUCCESS !=
            SHGetValue(HKEY_CURRENT_USER, INTERNET_POLICY_KEY,
            szDialupAutodetect, &dwType, &dwDialUpAutodetect, &dwSize))
        {
            // not set
            dwDialUpAutodetect = 0;
        }
    }

    return (BOOL)dwDialUpAutodetect;
}

///////////////////////////////////////////////////////////////////////////
//
// CRegBlob implementation
//
///////////////////////////////////////////////////////////////////////////

CRegBlob::CRegBlob(
    BOOL fWrite
    )
{
    // initialize members
    _fWrite = fWrite;
    _fCommit = TRUE;
    _dwOffset = 0;
    _pBuffer = NULL;
    _dwBufferLimit = 0;
    _hkey = NULL;
}


CRegBlob::~CRegBlob(
    )
{
    Commit();

    if(_hkey)
        REGCLOSEKEY(_hkey);

    if(_pBuffer)
        GlobalFree(_pBuffer);

    // caller owns _pszValue pointer
}


DWORD
CRegBlob::Init(
    HKEY hBaseKey,
    LPCSTR pszSubKey,
    LPCSTR pszValue
    )
{
    long lRes;
    REGSAM  regsam = KEY_QUERY_VALUE;
    DWORD dwDisposition;

    // If we're writing, save reg value name and set access
    if(_fWrite)
    {
        _pszValue = pszValue;
        regsam = KEY_SET_VALUE;
    }

    lRes = REGCREATEKEYEX(hBaseKey, pszSubKey, 0, "", 0,
                regsam, NULL, &_hkey, &dwDisposition);
    if(lRes != ERROR_SUCCESS)
    {
        return lRes;
    }

    // figure out buffer size
    _dwBufferLimit = BLOB_BUFF_GRANULARITY;
    if(FALSE == _fWrite)
    {
        // get size of registry blob
        lRes = RegQueryValueEx(_hkey, pszValue, NULL, NULL, NULL, &_dwBufferLimit);
        if(lRes != ERROR_SUCCESS)
        {
            // nothing there - make zero size buffer
            _dwBufferLimit = 0;
        }
    }

    // allocate buffer if necessary
    if(_dwBufferLimit)
    {
        _pBuffer = (BYTE *)GlobalAlloc(GPTR, _dwBufferLimit);
        if(NULL == _pBuffer)
            return GetLastError();
    }

    // if we're reading, fill in buffer
    if(FALSE == _fWrite && _dwBufferLimit)
    {
        // read reg key
        DWORD dwSize = _dwBufferLimit;
        lRes = RegQueryValueEx(_hkey, pszValue, NULL, NULL, _pBuffer, &dwSize);
        if(lRes != ERROR_SUCCESS)
        {
            return lRes;
        }
    }

    // reset pointer to beginning of blob
    _dwOffset = 0;

    return 0;
}

DWORD
CRegBlob::Abandon(
    VOID
    )
{
    // don't commit changes when the time comes
    _fCommit = FALSE;

    return 0;
}

DWORD
CRegBlob::Commit(
    )
{
    long lres = 0;

    if(_fCommit && _fWrite && _pszValue && _pBuffer)
    {
        // save blob to reg key
        lres = RegSetValueEx(_hkey, _pszValue, 0, REG_BINARY, _pBuffer, _dwOffset);
    }

    return lres;
}


DWORD
CRegBlob::Encrpyt(
    )
{
    return 0;
}


DWORD
CRegBlob::Decrypt(
    )
{
    return 0;
}


DWORD
CRegBlob::WriteString(
    LPCSTR pszString
    )
{
    DWORD dwBytes, dwLen = 0;

    if(pszString)
    {
        dwLen = lstrlen(pszString);
    }

    dwBytes = WriteBytes(&dwLen, sizeof(DWORD));
    if(dwLen && dwBytes == sizeof(DWORD))
        dwBytes = WriteBytes(pszString, dwLen);

    return dwBytes;
}



DWORD
CRegBlob::ReadString(
    LPCSTR * ppszString
    )
{
    DWORD dwLen, dwBytes = 0;
    LPSTR lpszTemp = NULL;

    dwBytes = ReadBytes(&dwLen, sizeof(DWORD));
    if(dwBytes == sizeof(DWORD))
    {
        if(dwLen)
        {
            lpszTemp = (LPSTR)GlobalAlloc(GPTR, dwLen + 1);
            if(lpszTemp)
            {
                dwBytes = ReadBytes(lpszTemp, dwLen);
                lpszTemp[dwBytes] = 0;
            }
        }
    }

    *ppszString = lpszTemp;
    return dwBytes;
}


DWORD
CRegBlob::WriteBytes(
    LPCVOID pBytes,
    DWORD dwByteCount
    )
{
    BYTE * pNewBuffer;

    // can only do this on write blob
    if(FALSE == _fWrite)
        return 0;

    // grow buffer if necessary
    if(_dwBufferLimit - _dwOffset < dwByteCount)
    {
        pNewBuffer = (BYTE *)GlobalReAlloc(_pBuffer, _dwBufferLimit + BLOB_BUFF_GRANULARITY, GMEM_ZEROINIT);
        if(NULL == pNewBuffer)
        {
            // failed to get more memory
            return 0;
        }

        _pBuffer = pNewBuffer;
        _dwBufferLimit += BLOB_BUFF_GRANULARITY;
    }

    // copy callers data to buffer
    memcpy(_pBuffer + _dwOffset, pBytes, dwByteCount);
    _dwOffset += dwByteCount;

    // tell caller how much we wrote
    return dwByteCount;
}



DWORD
CRegBlob::ReadBytes(
    LPVOID pBytes,
    DWORD dwByteCount
    )
{
    DWORD   dwActual = _dwBufferLimit - _dwOffset;

    // can only do this on read blob
    if(_fWrite)
        return 0;

    // don't read past end of blob
    if(dwByteCount < dwActual)
        dwActual = dwByteCount;

    // copy bytes and increment offset
    if(dwActual > 0)
    {
        memcpy(pBytes, _pBuffer + _dwOffset, dwActual);
        _dwOffset += dwActual;
    }

    // tell caller how much we actually read
    return dwActual;
}


///////////////////////////////////////////////////////////////////////////
//
// Helpers to read and write proxy settings
//
///////////////////////////////////////////////////////////////////////////

//
// Maximum size of TOKEN_USER information.
//

#define SIZE_OF_TOKEN_INFORMATION                   \
    sizeof( TOKEN_USER )                            \
    + sizeof( SID )                                 \
    + sizeof( ULONG ) * SID_MAX_SUB_AUTHORITIES

#define MAX_SID_STRING 256


//
// Function Declarations
//

BOOL
InitClientUserString (
    LPWSTR pString
    )

/*++

Routine Description:

Arguments:

    pString - output string of current user

Return Value:

    TRUE = success,
    FALSE = fail

    Returns in pString a ansi string if the impersonated client's
    SID can be expanded successfully into  Unicode string. If the conversion
    was unsuccessful, returns FALSE.

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 Bool,
                 "InitClientUserString",
                 "%#x",
                 pString
                 ));

    HANDLE      TokenHandle;
    UCHAR       TokenInformation[ SIZE_OF_TOKEN_INFORMATION ];
    ULONG       ReturnLength;
    BOOL        Status;
    DWORD       dwLastError;
    UNICODE_STRING UnicodeString;
    HMODULE     hNtDll;
    PCONVERTSID pRtlConvertSid;

    //
    // get RtlConvertSideToUnicodeString entry point in NTDLL
    //
    hNtDll = LoadLibrary("ntdll.dll");
    if(NULL == hNtDll)
    {
        return FALSE;
    }

    pRtlConvertSid = (PCONVERTSID)GetProcAddress(hNtDll, "RtlConvertSidToUnicodeString");
    if(NULL == pRtlConvertSid)
    {
        FreeLibrary(hNtDll);
        DEBUG_LEAVE(FALSE);
        return FALSE;
    }

    //
    // We can use OpenThreadToken because this server thread
    // is impersonating a client
    //
    Status = OpenThreadToken(
                GetCurrentThread(),
                TOKEN_READ,
                TRUE,                // Open as self
                &TokenHandle
                );
    dwLastError = GetLastError();

    if( Status == FALSE )
    {
        Status = OpenProcessToken(
                    GetCurrentProcess(),
                    TOKEN_READ,
                    &TokenHandle
                    );
        dwLastError = GetLastError();

        if( Status == FALSE )
        {
            DEBUG_LEAVE(FALSE);
            return FALSE ;
        }
    }

    //
    // Notice that we've allocated enough space for the
    // TokenInformation structure. so if we fail, we
    // return a NULL pointer indicating failure
    //
    Status = GetTokenInformation( TokenHandle,
                                  TokenUser,
                                  TokenInformation,
                                  sizeof( TokenInformation ),
                                  &ReturnLength
                                   );
    dwLastError = GetLastError();
    CloseHandle( TokenHandle );

    if ( Status == FALSE ) {
        DEBUG_PRINT(DIALUP, INFO, ("GetTokenInformation failed: Error=%d\n",
                    dwLastError
                    ));
        DEBUG_LEAVE(FALSE);
        return FALSE;
    }

    //
    // Convert the Sid (pointed to by pSid) to its
    // equivalent Unicode string representation.
    //

    UnicodeString.Length = 0;
    UnicodeString.MaximumLength = MAX_SID_STRING;
    UnicodeString.Buffer = pString;

    Status = (*pRtlConvertSid)(
                 &UnicodeString,
                 ((PTOKEN_USER)TokenInformation)->User.Sid,
                 FALSE );
    FreeLibrary(hNtDll);

    if( !NT_SUCCESS( Status )){
        DEBUG_PRINT(DIALUP, INFO, ("RtlConvertSidToUnicodeString failed: Error=%d\n",
                    Status
                    ));
        DEBUG_LEAVE(FALSE);
        return FALSE;
    }

    DEBUG_LEAVE(TRUE);
    return TRUE;
}


HKEY
GetClientUserHandle(
    IN REGSAM samDesired
    )

/*++

Routine Description:

Arguments:

Returns:

---*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 Dword,
                 "GetClientUserHandle",
                 "%#x",
                 samDesired
                 ));

    HKEY   hKeyClient;
    WCHAR  String[MAX_SID_STRING];
    LONG   ReturnValue;

    if (!InitClientUserString(String)) {
        DEBUG_LEAVE(0);
        return NULL ;
    }

    //
    // We now have the Unicode string representation of the
    // local client's Sid we'll use this string to open a handle
    // to the client's key in  the registry.

    ReturnValue = RegOpenKeyExW( HKEY_USERS,
                                 String,
                                 0,
                                 samDesired,
                                 &hKeyClient );

    //
    // If we couldn't get a handle to the local key
    // for some reason, return a NULL handle indicating
    // failure to obtain a handle to the key
    //

    if ( ReturnValue != ERROR_SUCCESS )
    {
        DEBUG_PRINT(DIALUP, INFO, ("RegOpenKeyW failed: Error=%d\n",
                    ReturnValue
                    ));

        DEBUG_ERROR(DIALUP, ReturnValue);
        SetLastError( ReturnValue );

        DEBUG_LEAVE(0);
        return NULL;
    }

    DEBUG_LEAVE(hKeyClient);
    return( hKeyClient );
}


#define INTERNET_SETTINGS_KEY       "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"

HKEY
FindBaseProxyKey(
    VOID
    )

/*
** Determine whether proxy settings live in HKLM or HKCU
**
** Returns HKEY_CURRENT_USER or HKEY_LOCAL_MACHINE
**
** Checks \HKLM\SW\MS\Win\CV\Internet Settings\ProxySettingsPerUser.  If
** exists and is zero, use HKLM otherwise use HKCU.
*/

{
    HKEY hkeyBase = NULL;

    if(NULL == g_hkeyBase)
    {
        DWORD   dwType, dwValue, dwSize = sizeof(DWORD);

        if(ERROR_SUCCESS ==
            SHGetValue(HKEY_LOCAL_MACHINE, INTERNET_POLICY_KEY,
            TEXT("ProxySettingsPerUser"), &dwType, &dwValue, &dwSize) &&
            0 == dwValue)
        {
            SHInterlockedCompareExchange((void **)&g_hkeyBase, HKEY_LOCAL_MACHINE, 0);
        }
        else
        {
            //
            // Find an HKCU equivalent for this process
            //
            if(PLATFORM_TYPE_WIN95 == GlobalPlatformType ||
               NULL == (hkeyBase = GetClientUserHandle(KEY_QUERY_VALUE | KEY_SET_VALUE)))
            {
                SHInterlockedCompareExchange((void **)&g_hkeyBase, HKEY_CURRENT_USER, 0);
            }
            else  // !Win95 and hkeyBase was sucessfully returned
            {
                if (SHInterlockedCompareExchange((void **)&g_hkeyBase, hkeyBase, 0))
                {
                    RegCloseKey(hkeyBase); // race, free the copy
                }
            }
        }
    }

    return g_hkeyBase;
}


DWORD
ReadProxySettings(
    LPINTERNET_PROXY_INFO_EX pInfo
    )

{
    CRegBlob r(FALSE);
    LPCSTR  pszConnectionName;
    LPCSTR  pszSavedConnectionName;
    DWORD   error = ERROR_SUCCESS;
    long    lRes;
    DWORD   i;
    BOOL    fSave = FALSE, fLanConnection = FALSE;
    HKEY    hBaseKey;

    DEBUG_ENTER((DBG_DIALUP,
                 Dword,
                 "ReadProxySettings",
                 "%#x",
                 pInfo
                 ));

    // verify pInfo
    if(NULL == pInfo || pInfo->dwStructSize != sizeof(INTERNET_PROXY_INFO_EX))
    {
        DEBUG_LEAVE(ERROR_INVALID_PARAMETER);
        return ERROR_INVALID_PARAMETER;
    }

    // take mutex
    WaitForSingleObject(g_hProxyRegMutex, INFINITE);

    // figure out connection name  (NULL == 'network')
    pszConnectionName = pInfo->lpszConnectionName;
    pszSavedConnectionName = pInfo->lpszConnectionName;
    if(NULL == pszConnectionName || 0 == *pszConnectionName)
    {
        fLanConnection = TRUE;
        pszConnectionName = "DefaultConnectionSettings";
    }

    // figure out base key
    hBaseKey = FindBaseProxyKey();

    // initialize structure
    memset(pInfo, 0, sizeof(*pInfo));
    pInfo->dwStructSize = sizeof(*pInfo);
    pInfo->lpszConnectionName = pszSavedConnectionName;
    pInfo->dwFlags = PROXY_TYPE_DIRECT;

    // init blob
    lRes = r.Init(hBaseKey, szRegPathConnections, pszConnectionName);
    if(lRes)
    {
        error = lRes;
        goto quit;
    }

    // read fields from blob
    if(0 == r.ReadBytes(&pInfo->dwStructSize, sizeof(DWORD)) ||
         (pInfo->dwStructSize < sizeof(*pInfo)))
    {
        // blob didn't exist or in correct format - set default values
        pInfo->dwStructSize = sizeof(*pInfo);

        // If not on a lan connection and we're set to inherit lan settings
        if(!fLanConnection && GlobalUseLanSettings)
        {
            // Ensure we're reading the LAN proxy information.
            pInfo->lpszConnectionName = NULL;

            // Get proxy settings into pInfo
            ReadProxySettings(pInfo);

            // Restore connection name
            pInfo->lpszConnectionName = pszSavedConnectionName;
        }
        else
        {
#ifndef UNIX
            //
            // All lan connections and overridden dial-ups get autodetect
            //
            if(fLanConnection || EnableAutodiscoverForDialup())
            {
                pInfo->dwFlags |= PROXY_TYPE_AUTO_DETECT;
            }
#else
            ;
#endif /* UNIX */
        }
    }
    else
    {
        // read the rest of the blob
        r.ReadBytes(&pInfo->dwCurrentSettingsVersion, sizeof(DWORD));
        r.ReadBytes(&pInfo->dwFlags, sizeof(DWORD));
        r.ReadString(&pInfo->lpszProxy);
        r.ReadString(&pInfo->lpszProxyBypass);
        r.ReadString(&pInfo->lpszAutoconfigUrl);
        r.ReadBytes(&pInfo->dwAutoDiscoveryFlags, sizeof(DWORD));
        r.ReadString(&pInfo->lpszLastKnownGoodAutoConfigUrl);
        r.ReadBytes(&pInfo->ftLastKnownDetectTime, sizeof(FILETIME));

        // read interface ips
        r.ReadBytes(&pInfo->dwDetectedInterfaceIpCount, sizeof(DWORD));
        if(pInfo->dwDetectedInterfaceIpCount)
        {
            pInfo->pdwDetectedInterfaceIp = (DWORD *)GlobalAlloc(GPTR, sizeof(DWORD) * pInfo->dwDetectedInterfaceIpCount);
            if(pInfo->pdwDetectedInterfaceIp)
            {
                for(i=0; i<pInfo->dwDetectedInterfaceIpCount; i++)
                {
                    r.ReadBytes(&pInfo->pdwDetectedInterfaceIp[i], sizeof(DWORD));
                }
            }
        }

        r.ReadString(&pInfo->lpszAutoconfigSecondaryUrl);
        r.ReadBytes(&pInfo->dwAutoconfigReloadDelayMins, sizeof(DWORD));
    }

    // Netware hack.  Don't ever allow autodiscovery to be turned on if
    // we're running the netware client.  It faults and it isn't useful
    // anyway.
    //
    // Some other stacks may also fault.  If we find one, don't autodetect.
    if(GlobalRunningNovellClient32 || g_fGetHostByNameNULLFails)
    {
        pInfo->dwFlags &= ~PROXY_TYPE_AUTO_DETECT;

        // save this back so we don't ever try again
        fSave = TRUE;
    }

    DEBUG_PRINT(DIALUP, INFO, ("conn=%s, vers=%u, flag=%X, prox=%s, by=%s, acu=%s\n", 
                    pszConnectionName,
                    pInfo->dwCurrentSettingsVersion,
                    pInfo->dwFlags,
                    (pInfo->lpszProxy ? pInfo->lpszProxy : "<none>"),
                    (pInfo->lpszProxyBypass ? pInfo->lpszProxyBypass : "<none>"),
                    (pInfo->lpszAutoconfigUrl ? pInfo->lpszAutoconfigUrl : "<none>")
                    ));

    if(fSave)
    {
        WriteProxySettings(pInfo, TRUE);
    }

quit:
    // free mutex
    ReleaseMutex(g_hProxyRegMutex);
    DEBUG_LEAVE(error);
    return error;
}


DWORD
WriteProxySettings(
    LPINTERNET_PROXY_INFO_EX pInfo,
    BOOL fForceUpdate
    )

// serialize this.  Only write if dwCurrentSettingsVersion member has been
// incremented by exactly 1.

{
    DEBUG_ENTER((DBG_DIALUP,
                 Dword,
                 "WriteProxySettings",
                 "%#x, %B",
                 pInfo,
                 fForceUpdate                 
                 ));

    CRegBlob    r(TRUE);
    CRegBlob    CurrentVersion(FALSE);
    LPCSTR      pszConnectionName;
    long        lRes;
    DWORD       i;
    DWORD       dwCurrentVersion;
    DWORD       dwCurrentStructSize;
    DWORD       error = ERROR_SUCCESS;
    HKEY        hBaseKey;

    //
    // verify pInfo
    //
    if(NULL == pInfo || pInfo->dwStructSize != sizeof(INTERNET_PROXY_INFO_EX))
    {
        r.Abandon();
        DEBUG_LEAVE(ERROR_INVALID_PARAMETER);
        return ERROR_INVALID_PARAMETER;
    }

    // take mutex
    WaitForSingleObject(g_hProxyRegMutex, INFINITE);

    //
    // figure out connection name  (NULL == 'network')
    //
    pszConnectionName = pInfo->lpszConnectionName;
    if(NULL == pszConnectionName || 0 == *pszConnectionName)
    {
        pszConnectionName = "DefaultConnectionSettings";
    }

    //
    // For forced update changes, turn off this FLAG since,
    //  any major change in settings indicates a possible FIX
    //  for detection issues.
    //

    if ( fForceUpdate ) {
        pInfo->dwAutoDiscoveryFlags &= ~(AUTO_PROXY_FLAG_DETECTION_SUSPECT);
    }

    DEBUG_PRINT(DIALUP, INFO, ("conn=%s, vers=%u, flag=%X, a-flag=%X, prox=%s, by=%s, acu=%s\n", 
                    pszConnectionName,
                    pInfo->dwCurrentSettingsVersion,
                    pInfo->dwFlags,
                    pInfo->dwAutoDiscoveryFlags,
                    (pInfo->lpszProxy ? pInfo->lpszProxy : "<none>"),
                    (pInfo->lpszProxyBypass ? pInfo->lpszProxyBypass : "<none>"),
                    (pInfo->lpszAutoconfigUrl ? pInfo->lpszAutoconfigUrl : "<none>")
                    ));

    // figure out base key
    hBaseKey = FindBaseProxyKey();

    //
    // Get current version and verify another write hasn't happened
    //
    // Second dword is version
    //
    lRes = CurrentVersion.Init(hBaseKey, szRegPathConnections, pszConnectionName);
    if(lRes)
    {
        r.Abandon();
        error = lRes;
        goto quit;
    }

    dwCurrentStructSize = sizeof(*pInfo);
    dwCurrentVersion    = 0;

    CurrentVersion.ReadBytes(&dwCurrentStructSize, sizeof(DWORD)); // struct size
    CurrentVersion.ReadBytes(&dwCurrentVersion, sizeof(DWORD)); // actual version

    if(dwCurrentVersion != pInfo->dwCurrentSettingsVersion)
    {
        // someone else has written since this read.  If we aren't in force
        // mode, bail out
        if((fForceUpdate == FALSE) &&
           (dwCurrentStructSize >= sizeof(*pInfo)))
        {
            r.Abandon();
            error = ERROR_INVALID_DATA;
            goto quit;
        }

        // update to most recent version
        pInfo->dwCurrentSettingsVersion = dwCurrentVersion;
    }

    //
    // increment version
    //
    pInfo->dwCurrentSettingsVersion++;

    //
    // init blob
    //
    lRes = r.Init(hBaseKey, szRegPathConnections, pszConnectionName);
    if(lRes)
    {
        r.Abandon();
        error = lRes;
        goto quit;
    }

    //
    // write fields to blob
    //

    r.WriteBytes(&pInfo->dwStructSize, sizeof(DWORD)); // used for data format checking
    r.WriteBytes(&pInfo->dwCurrentSettingsVersion, sizeof(DWORD));
    r.WriteBytes(&pInfo->dwFlags, sizeof(DWORD));
    r.WriteString(pInfo->lpszProxy);
    r.WriteString(pInfo->lpszProxyBypass);
    r.WriteString(pInfo->lpszAutoconfigUrl);
    r.WriteBytes(&pInfo->dwAutoDiscoveryFlags, sizeof(DWORD));
    r.WriteString(pInfo->lpszLastKnownGoodAutoConfigUrl);
    r.WriteBytes(&pInfo->ftLastKnownDetectTime, sizeof(FILETIME));

    //
    // write interface ip list
    //
    r.WriteBytes(&pInfo->dwDetectedInterfaceIpCount, sizeof(DWORD));
    for(i=0; i<pInfo->dwDetectedInterfaceIpCount; i++)
    {
        r.WriteBytes(&pInfo->pdwDetectedInterfaceIp[i], sizeof(DWORD));
    }

    r.WriteString(pInfo->lpszAutoconfigSecondaryUrl);
    r.WriteBytes(&pInfo->dwAutoconfigReloadDelayMins, sizeof(DWORD));

quit:
    // free mutex
    ReleaseMutex(g_hProxyRegMutex);

    DEBUG_LEAVE(error);

    return error;
}

void
CleanProxyStruct(
    LPINTERNET_PROXY_INFO_EX pInfo
    )

{
    if(pInfo->lpszConnectionName)             GlobalFree((LPSTR) pInfo->lpszConnectionName);
    if(pInfo->lpszProxy)                      GlobalFree((LPSTR) pInfo->lpszProxy);
    if(pInfo->lpszProxyBypass)                GlobalFree((LPSTR) pInfo->lpszProxyBypass);
    if(pInfo->lpszAutoconfigUrl)              GlobalFree((LPSTR) pInfo->lpszAutoconfigUrl);
    if(pInfo->lpszLastKnownGoodAutoConfigUrl) GlobalFree((LPSTR) pInfo->lpszLastKnownGoodAutoConfigUrl);
    if(pInfo->pdwDetectedInterfaceIp)         GlobalFree(pInfo->pdwDetectedInterfaceIp);
    memset(pInfo, 0, sizeof(INTERNET_PROXY_INFO_EX));
    pInfo->dwFlags = PROXY_TYPE_DIRECT;
}


DWORD
SetPerConnOptions(
    HINTERNET hInternet,
    BOOL fIsAutoProxyThread,
    LPINTERNET_PER_CONN_OPTION_LISTA pList
    )
{
    INTERNET_PROXY_INFO_EX  info, info_temp;
    DWORD   i, dwError = ERROR_SUCCESS;
    BOOL fCommit = FALSE;
    LPSTR   pszCopy, pszNew;
    BOOL    fFreeCopy = FALSE;

    memset(&info, 0, sizeof(info));
    info.dwStructSize = sizeof(info);

    if ( hInternet == NULL )
    {
        //
        // If auto-proxy thread then try to get the settings
        //  from the auto-proxy thread
        //

        if ( ! fIsAutoProxyThread || 
             ! GlobalProxyInfo.GetAutoProxyThreadSettings(&info) ||
             ! IsConnectionMatch(info.lpszConnectionName, pList->pszConnection))
        {
            fFreeCopy = TRUE;            
            info.lpszConnectionName = pList->pszConnection;
            CheckForUpgrade();
            ReadProxySettings(&info);                        
        }
    }

    // loop through option list and set members
    for(i=0; i<pList->dwOptionCount; i++)
    {
        pszNew = NULL;

        switch(pList->pOptions[i].dwOption)
        {        
        case INTERNET_PER_CONN_PROXY_SERVER:
        case INTERNET_PER_CONN_PROXY_BYPASS:
        case INTERNET_PER_CONN_AUTOCONFIG_URL:
        case INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL:        
            // make a copy of the string passed in for these guys
            pszCopy = pList->pOptions[i].Value.pszValue;
            if(pszCopy)
            {
                pszNew = (LPSTR)GlobalAlloc(GPTR, lstrlen(pszCopy) + 1);
                if(pszNew)
                {
                    lstrcpy(pszNew, pszCopy);
                }
                else
                {
                    dwError = ERROR_NOT_ENOUGH_MEMORY;
                    pList->dwOptionError = i;
                }
            }
            break;
        }

        if(dwError)
        {
            fCommit = FALSE;
            break;
        }

        switch(pList->pOptions[i].dwOption)
        {
        case INTERNET_PER_CONN_FLAGS:
            info.dwFlags = pList->pOptions[i].Value.dwValue;
            fCommit = TRUE;
            break;
        case INTERNET_PER_CONN_AUTODISCOVERY_FLAGS:
            info.dwAutoDiscoveryFlags = pList->pOptions[i].Value.dwValue;
            fCommit = TRUE;
            break;
        case INTERNET_PER_CONN_AUTOCONFIG_RELOAD_DELAY_MINS:
            info.dwAutoconfigReloadDelayMins = pList->pOptions[i].Value.dwValue;
            fCommit = TRUE;
            break;
        case INTERNET_PER_CONN_PROXY_SERVER:
            if(info.lpszProxy)
                GlobalFree((LPSTR)info.lpszProxy);
            info.lpszProxy = pszNew;
            fCommit = TRUE;
            break;
        case INTERNET_PER_CONN_PROXY_BYPASS:
            if(info.lpszProxyBypass)
                GlobalFree((LPSTR)info.lpszProxyBypass);
            info.lpszProxyBypass = pszNew;
            fCommit = TRUE;
            break;
        case INTERNET_PER_CONN_AUTOCONFIG_URL:
            if(info.lpszAutoconfigUrl)
                GlobalFree((LPSTR)info.lpszAutoconfigUrl);
            info.lpszAutoconfigUrl = pszNew;
            fCommit = TRUE;
            break;
        case INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL:
            if(info.lpszAutoconfigSecondaryUrl)
                GlobalFree((LPSTR)info.lpszAutoconfigSecondaryUrl);
            info.lpszAutoconfigSecondaryUrl = pszNew;
            fCommit = TRUE;
            break;
        case INTERNET_PER_CONN_AUTOCONFIG_LAST_DETECT_TIME:
            dwError = ERROR_INTERNET_OPTION_NOT_SETTABLE;
            pList->dwOptionError = i;
            break;
        case INTERNET_PER_CONN_AUTOCONFIG_LAST_DETECT_URL:
            dwError = ERROR_INTERNET_OPTION_NOT_SETTABLE;
            pList->dwOptionError = i;
            break;
        default:
            dwError = ERROR_INVALID_PARAMETER;
            pList->dwOptionError = i;
            break;
        }

        if(dwError)
        {
            fCommit = FALSE;
            break;
        }
    }

    if(fCommit)
    {
        if ( hInternet == NULL ) 
        {

            memset(&info_temp, 0, sizeof(info_temp));
            info_temp.dwStructSize = sizeof(info_temp);

            if ( ! fIsAutoProxyThread ||
                 ! GlobalProxyInfo.GetAutoProxyThreadSettings(&info_temp) ||
                 ! IsConnectionMatch(info_temp.lpszConnectionName, pList->pszConnection) ||
                 ! GlobalProxyInfo.SetAutoProxyThreadSettings(&info))
                 
            { 
                WriteProxySettings(&info, TRUE);

                // update legacy settings with new values
                info.lpszConnectionName = LEGACY_SAVE_NAME;
                WriteLegacyProxyInfo(szRegPathInternetSettings, &info, TRUE);
                WriteProxySettings(&info, TRUE);
            }
        } 
        else 
        {
            GlobalProxyInfo.SetProxySettings(&info, TRUE);
        }
    }

    if ( fFreeCopy ) {
        info.lpszConnectionName = NULL; // we don't allocate this field
        CleanProxyStruct(&info);
    }

    return dwError;
}



DWORD
QueryPerConnOptions(
    HINTERNET hInternet,
    BOOL fIsAutoProxyThread,
    LPINTERNET_PER_CONN_OPTION_LISTA pList
    )
{
    INTERNET_PROXY_INFO_EX  info;
    LPCSTR   pszCopy;
    LPSTR    pszNew;
    DWORD    i, dwError = ERROR_SUCCESS;
    BOOL     fFreeCopy = FALSE;

    pList->dwOptionError = 0;

    memset(&info, 0, sizeof(info));
    info.dwStructSize = sizeof(info);
    info.lpszConnectionName = pList->pszConnection;

    if ( hInternet == NULL ) 
    {
        if ( ! fIsAutoProxyThread ||                          
             ! GlobalProxyInfo.GetAutoProxyThreadSettings(&info) ||
             ! IsConnectionMatch(info.lpszConnectionName, pList->pszConnection)) 
        { 
            CheckForUpgrade();
            ReadProxySettings(&info);
            fFreeCopy = TRUE;
        }
    }
    else 
    {
        GlobalProxyInfo.GetProxySettings(&info, FALSE);
    }

    // loop through option list and fill in members
    for(i=0; i<pList->dwOptionCount; i++)
    {
        pList->pOptions[i].Value.pszValue = NULL;
        pszCopy = NULL;

        switch(pList->pOptions[i].dwOption)
        {
        case INTERNET_PER_CONN_FLAGS:
            pList->pOptions[i].Value.dwValue = info.dwFlags;
            break;
        case INTERNET_PER_CONN_AUTODISCOVERY_FLAGS:
            pList->pOptions[i].Value.dwValue = info.dwAutoDiscoveryFlags;
            break;
        case INTERNET_PER_CONN_AUTOCONFIG_RELOAD_DELAY_MINS:
            pList->pOptions[i].Value.dwValue = info.dwAutoconfigReloadDelayMins;
            break;
        case INTERNET_PER_CONN_PROXY_SERVER:
            pszCopy = info.lpszProxy;
            break;
        case INTERNET_PER_CONN_PROXY_BYPASS:
            pszCopy = info.lpszProxyBypass;
            break;
        case INTERNET_PER_CONN_AUTOCONFIG_URL:
            pszCopy = info.lpszAutoconfigUrl;
            break;
        case INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL:
            pszCopy = info.lpszAutoconfigSecondaryUrl;
            break;
        case INTERNET_PER_CONN_AUTOCONFIG_LAST_DETECT_URL:
            pszCopy = info.lpszLastKnownGoodAutoConfigUrl;
            break;
        case INTERNET_PER_CONN_AUTOCONFIG_LAST_DETECT_TIME:
            *(LONGLONG *) &(pList->pOptions[i].Value.ftValue) = *(LONGLONG *) &(info.ftLastKnownDetectTime);
            break;

        default:
            dwError = ERROR_INVALID_PARAMETER;
            pList->dwOptionError = i;
            break;
        }

        // if this is a string value, make a copy of the string for the
        // caller
        if(pszCopy)
        {
            // make a copy of the string and stick it in the option
            pszNew = (LPSTR)GlobalAlloc(GPTR, lstrlen(pszCopy) + 1);
            if(pszNew)
            {
                lstrcpy(pszNew, pszCopy);
                pList->pOptions[i].Value.pszValue = pszNew;
            }
            else
            {
                dwError = ERROR_NOT_ENOUGH_MEMORY;
                pList->dwOptionError = i;
            }
        }

        if(dwError)
        {
            break;
        }
    }

    if (dwError)
    {
        // If an error has occurred, we should get rid of any strings that
        // we've allocated.
        for (i=0; i<pList->dwOptionError; i++)
        {
            switch(pList->pOptions[i].dwOption)
            {
            case INTERNET_PER_CONN_PROXY_SERVER:
            case INTERNET_PER_CONN_PROXY_BYPASS:
            case INTERNET_PER_CONN_AUTOCONFIG_URL:
            case INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL:
            case INTERNET_PER_CONN_AUTOCONFIG_LAST_DETECT_URL:
                if (pList->pOptions[i].Value.pszValue)
                {
                    GlobalFree(pList->pOptions[i].Value.pszValue);
                    pList->pOptions[i].Value.pszValue = NULL;
                }
                break;

            default:
                break;
            }
        }
    }

    if ( fFreeCopy ) {
        info.lpszConnectionName = NULL; // we don't allocate this field
        CleanProxyStruct(&info);
    }

    return dwError;
}


BOOL
ReadLegacyProxyInfo(
    IN LPCTSTR pszKey,
    LPINTERNET_PROXY_INFO_EX pProxy
    )

/*++

Routine Description:

    Reads legacy proxy information from a specified key.

Arguments:

    pszKey      - key from which to read proxy info
    pProxy      - pointer to PROXY structure to store info

Return Value:

    BOOL
        TRUE    - success
        FALSE   - failed

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 Bool,
                 "ReadLegacyProxyInfo",
                 "%#x (%q), %#x",
                 pszKey,
                 pszKey,
                 pProxy
                 ));

    BOOL    fSuccess = FALSE;
    HKEY    hKey;
    DWORD   dwSize, dwValue;

    pProxy->dwFlags = PROXY_TYPE_DIRECT;

    if(ERROR_SUCCESS == REGOPENKEYEX(FindBaseProxyKey(), pszKey, NULL,
                                KEY_READ, &hKey)) {
        // read enable
        dwSize = sizeof(DWORD);
        if(ERROR_SUCCESS != RegQueryValueEx(hKey, szRegValProxyEnabled,
                    NULL, NULL, (LPBYTE)&dwValue, &dwSize)) {
            dwValue = 0;
        }
        if(dwValue)
        {
            pProxy->dwFlags |= PROXY_TYPE_PROXY;
        }

        // read server
        dwSize = INTERNET_MAX_URL_LENGTH;
        pProxy->lpszProxy = (LPSTR)GlobalAlloc(GMEM_FIXED, dwSize);
        if(pProxy->lpszProxy)
        {
            if(ERROR_SUCCESS != RegQueryValueEx(hKey, REGSTR_VAL_PROXYSERVER,
                NULL, NULL, (LPBYTE)pProxy->lpszProxy, &dwSize) ||
                (dwSize == 0) ||
                (*pProxy->lpszProxy == '\0'))
            {
                GlobalFree((LPVOID)pProxy->lpszProxy);
                pProxy->lpszProxy = NULL;
            }
        }

        // read override
        dwSize = INTERNET_MAX_URL_LENGTH;
        pProxy->lpszProxyBypass = (LPSTR)GlobalAlloc(GMEM_FIXED, dwSize);
        if(pProxy->lpszProxyBypass)
        {
            if(ERROR_SUCCESS != RegQueryValueEx(hKey, REGSTR_VAL_PROXYOVERRIDE,
                NULL, NULL, (LPBYTE)pProxy->lpszProxyBypass, &dwSize) ||
                (dwSize == 0) ||
                (*pProxy->lpszProxyBypass == '\0'))
            {
                GlobalFree((LPVOID)pProxy->lpszProxyBypass);
                pProxy->lpszProxyBypass = NULL;
            }
        }
        

        // read autoconfig URL
        dwSize = INTERNET_MAX_URL_LENGTH;
        pProxy->lpszAutoconfigUrl = (LPSTR)GlobalAlloc(GMEM_FIXED, dwSize);
        if(pProxy->lpszAutoconfigUrl)
        {
            if(ERROR_SUCCESS != RegQueryValueEx(hKey, szLegacyAutoConfigURL,
                NULL, NULL, (LPBYTE)pProxy->lpszAutoconfigUrl, &dwSize) ||
                (dwSize == 0) ||
                (*pProxy->lpszAutoconfigUrl == '\0'))
            {
                // clear out
                GlobalFree((LPVOID)pProxy->lpszAutoconfigUrl);
                pProxy->lpszAutoconfigUrl = NULL;
            }
            else
            {
                // turn on if there's an URL
                pProxy->dwFlags |= PROXY_TYPE_AUTO_PROXY_URL;
            }
        }

        REGCLOSEKEY(hKey);
        fSuccess = TRUE;
    }

    DEBUG_PRINT(DIALUP, INFO, ("flag=%x, prox=%s, by=%s, acu=%s\n", 
                    pProxy->dwFlags,
                    (pProxy->lpszProxy ? pProxy->lpszProxy : "<none>"),
                    (pProxy->lpszProxyBypass ? pProxy->lpszProxyBypass : "<none>"),
                    (pProxy->lpszAutoconfigUrl ? pProxy->lpszAutoconfigUrl : "<none>")
                    ));

    DEBUG_LEAVE(fSuccess);
    return fSuccess;
}


BOOL
WriteLegacyProxyInfo(
    IN LPCTSTR pszKey,
    LPINTERNET_PROXY_INFO_EX pProxy,
    IN BOOL    fOverwrite
    )

/*++

Routine Description:

    Writes legacy proxy info a specified key

Arguments:

    pszKey      - key to write proxy inf to
    pProxy      - pointer to PROXY structure containing info to write
    fOverwrite  - If TRUE, overwrite existing info. otherwise only create

Return Value:

    BOOL
        TRUE    - success
        FALSE   - failed

--*/

{
    DEBUG_ENTER((DBG_DIALUP,
                 Bool,
                 "WriteLegacyProxyInfo",
                 "%#x (%q), %#x, %B",
                 pszKey,
                 pszKey,
                 pProxy,
                 fOverwrite
                 ));

    BOOL    fSuccess = FALSE;
    HKEY    hKey;
    DWORD   dwDisposition, dwValue;

    DEBUG_PRINT(DIALUP, INFO, ("flag=%x, prox=%s, by=%s, acu=%s\n", 
                    pProxy->dwFlags,
                    (pProxy->lpszProxy ? pProxy->lpszProxy : "<none>"),
                    (pProxy->lpszProxyBypass ? pProxy->lpszProxyBypass : "<none>"),
                    (pProxy->lpszAutoconfigUrl ? pProxy->lpszAutoconfigUrl : "<none>")
                    ));

    if(ERROR_SUCCESS == REGCREATEKEYEX(FindBaseProxyKey(), pszKey, 0, "", 0,
                KEY_WRITE, NULL, &hKey, &dwDisposition)) {

        fSuccess = TRUE;

        //
        // if we're not supposed to overwrite, check enable key.  If it
        // exists, bail
        //
        if(FALSE == fOverwrite) {
            DWORD dwEnable, dwSize = sizeof(DWORD);
            if(ERROR_SUCCESS == RegQueryValueEx(hKey, szRegValProxyEnabled,
                    NULL, NULL, (LPBYTE)&dwEnable, &dwSize)) {
                REGCLOSEKEY(hKey);
                DEBUG_PRINT(DIALUP, INFO, ("Overwrite not set.\n"));
                DEBUG_LEAVE(fSuccess);
                return fSuccess;
            }
        }

        // write enable
        dwValue = 0;
        if(pProxy->dwFlags & PROXY_TYPE_PROXY)
            dwValue = 1;
        DEBUG_PRINT(DIALUP, INFO, ("Setting legacy enabled=%d\n", dwValue));
        if(ERROR_SUCCESS != RegSetValueEx(hKey, szRegValProxyEnabled, 0,
                    REG_DWORD, (BYTE *)&dwValue, sizeof(DWORD)))
            fSuccess = FALSE;

        // write server
        if(pProxy->lpszProxy)
        {
            if(ERROR_SUCCESS != RegSetValueEx(hKey, REGSTR_VAL_PROXYSERVER, 0,
                    REG_SZ, (BYTE *)pProxy->lpszProxy,
                    lstrlen(pProxy->lpszProxy)))
                fSuccess = FALSE;
        }
        else
        {
            RegDeleteValue(hKey, REGSTR_VAL_PROXYSERVER);
            DEBUG_PRINT(DIALUP, INFO, ("Deleting legacy server\n"));
        }

        // write override
        if(pProxy->lpszProxyBypass)
        {
            if(ERROR_SUCCESS != RegSetValueEx(hKey, REGSTR_VAL_PROXYOVERRIDE, 0,
                    REG_SZ, (BYTE *)pProxy->lpszProxyBypass,
                    lstrlen(pProxy->lpszProxyBypass)))
                fSuccess = FALSE;
        }
        else
        {
            RegDeleteValue(hKey, REGSTR_VAL_PROXYOVERRIDE);
            DEBUG_PRINT(DIALUP, INFO, ("Deleting legacy override\n"));
        }

        // write autoconfig url
        if( (pProxy->dwFlags & PROXY_TYPE_AUTO_PROXY_URL) &&
             pProxy->lpszAutoconfigUrl)
        {
            if(ERROR_SUCCESS != RegSetValueEx(hKey, szLegacyAutoConfigURL, 0,
                REG_SZ, (BYTE *)pProxy->lpszAutoconfigUrl,
                lstrlen(pProxy->lpszAutoconfigUrl)))
            {
                fSuccess = FALSE;
            }
        }
        else
        {
            RegDeleteValue(hKey, szLegacyAutoConfigURL);
            DEBUG_PRINT(DIALUP, INFO, ("Deleting legacy autoconfig url\n"));
        }

        REGCLOSEKEY(hKey);
    }

    //
    // duplicate proxy enable reg settings to HKEY_CURRENT_CONFIG so 
    // shell doesn't blow away the setting when it migrates.
    //
    dwValue = (pProxy->dwFlags & PROXY_TYPE_PROXY) ? 1 : 0;
    SHSetValue(HKEY_CURRENT_CONFIG, pszKey, szRegValProxyEnabled,
        REG_DWORD, &dwValue, sizeof(DWORD));

    DEBUG_LEAVE(fSuccess);
    return fSuccess;
}
