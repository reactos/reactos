#include "ParseInf.h"
#include "general.h"
#include <shlwapi.h>
#include <wininet.h>

//#define USE_SHORT_PATH_NAME    1

#define REG_PATH_IE_CACHE_LIST  TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ActiveX Cache")

#define cCachePathsMax 5    // maximum number of legacy caches + the current cache du jour


struct OCCFindData
{
    LPCLSIDLIST_ITEM m_pcliHead;
    LPCLSIDLIST_ITEM m_pcliTail;
    struct {
        TCHAR  m_sz[MAX_PATH];
        DWORD  m_cch;
    } m_aCachePath[cCachePathsMax];

    OCCFindData();
    ~OCCFindData();
 
    BOOL    IsCachePath( LPCTSTR szPath );

    // Control List operations
    HRESULT          AddListItem( LPCTSTR szFileName, LPCTSTR szCLSID, DWORD dwIsDistUnit );
    LPCLSIDLIST_ITEM TakeFirstItem(void);
};

DWORD CCacheLegacyControl::s_dwType = 1;
DWORD CCacheDistUnit::s_dwType = 2;

HRESULT CCacheLegacyControl::Init( HKEY hkeyCLSID, LPCTSTR szFile, LPCTSTR szCLSID )
{
    HRESULT hr = S_OK;
    LRESULT lResult;
    DWORD   dw;

    lstrcpyn(m_szFile, szFile, MAX_PATH);
    lstrcpyn(m_szCLSID, szCLSID, MAX_DIST_UNIT_NAME_LEN);

    // Get full user type name
    m_szName[0] = '\0';
    dw = LENGTH_NAME;
    lResult = RegQueryValue( hkeyCLSID,
                             m_szCLSID,
                             m_szName, 
                             (LONG*)&dw );
    // BUGBUG - if the fails, we should get a resource string (seanf 5/9/97 )
    // Get type lib id
    TCHAR szTypeLibValName[MAX_PATH];
    CatPathStrN( szTypeLibValName, szCLSID, HKCR_TYPELIB, MAX_PATH );

    dw = MAX_CLSID_LEN;
    lResult = RegQueryValue( hkeyCLSID, szTypeLibValName, m_szTypeLibID, (LONG*)&dw);
    if (lResult != ERROR_SUCCESS)
        (m_szTypeLibID)[0] = TEXT('\0');

    // Set Codebase
    m_szCodeBase[0] = '\0';
    m_szVersion[0] = '\0';
    hr = DoParse( m_szFile, m_szCLSID );

    return hr;
}

HRESULT CCacheDistUnit::Init( HKEY hkeyCLSID, LPCTSTR szFile, LPCTSTR szCLSID, HKEY hkeyDist, LPCTSTR szDU )
{
    HRESULT hr = S_OK;
    HKEY    hkeyDU;
    HKEY    hkeyDLInfo; // DownloadInformation subkey
    HKEY    hkeyVers;   // InstalledVersion subkey
    HKEY    hkeyCOM;    // subkey of HKCR\CLSID, used if outside of cache dir
    LRESULT lResult = ERROR_SUCCESS;
    DWORD   dw;
    TCHAR   szNameT[MAX_PATH];
    UINT    uiVerSize = 0;
    DWORD   dwVerSize = 0;
    DWORD   dwHandle = 0;
    BYTE   *pbBuffer = NULL;
    HANDLE  hFile;
    FILETIME ftLastAccess;
    BOOL bRunOnNT5 = FALSE;
    OSVERSIONINFO osvi;
    VS_FIXEDFILEINFO     *lpVSInfo = NULL;
    
    if ( szFile[0] == '\0' &&
         RegOpenKeyEx( hkeyCLSID, szCLSID, 0, KEY_READ, &hkeyCOM ) == ERROR_SUCCESS )
    {
        LONG lcb = MAX_PATH;
        lResult = RegQueryValue( hkeyCOM, INPROCSERVER, szNameT, &lcb );

        if ( lResult != ERROR_SUCCESS )
        {
            lcb = MAX_PATH;
            lResult = RegQueryValue( hkeyCOM, INPROCSERVER32, szNameT, &lcb );
        }

        if ( lResult != ERROR_SUCCESS )
        {
            lcb = MAX_PATH;
            lResult = RegQueryValue( hkeyCOM, INPROCSERVERX86, szNameT, &lcb );
        }

        if ( lResult != ERROR_SUCCESS )
        {
            lcb = MAX_PATH;
            lResult = RegQueryValue( hkeyCOM, LOCALSERVER, szNameT, &lcb );
        }

        if ( lResult != ERROR_SUCCESS )
        {
            lcb = MAX_PATH;
            lResult = RegQueryValue( hkeyCOM, LOCALSERVER32, szNameT, &lcb );
        }

        if ( lResult != ERROR_SUCCESS )
        {
            lcb = MAX_PATH;
            lResult = RegQueryValue( hkeyCOM, LOCALSERVERX86, szNameT, &lcb );
        }

        RegCloseKey( hkeyCOM );
    }
    else
        lstrcpyn( szNameT, szFile, MAX_PATH );

    if ( lResult != ERROR_SUCCESS ) // needed to find file path but couldn't
        szNameT[0] = '\0';
    
    hr = CCacheLegacyControl::Init( hkeyCLSID, szNameT, szCLSID );

    if ( FAILED(hr) )
        return hr;

    lResult = RegOpenKeyEx(hkeyDist, szDU, 0, KEY_READ, &hkeyDU);
    if (lResult != ERROR_SUCCESS)
        return E_FAIL;

    // Get CLSID
    lstrcpyn(m_szCLSID, szDU, MAX_DIST_UNIT_NAME_LEN);

    // Get full user type name - only override the control name if DU name is not empty
    dw = MAX_PATH;
    lResult = RegQueryValue(hkeyDU, NULL, szNameT, (LONG*)&dw);
    if ( lResult == ERROR_SUCCESS && szNameT[0] != '\0' )
    {
        lstrcpyn( m_szName, szNameT, LENGTH_NAME );
    }
    else if ( *m_szName == '\0' ) // worst case, if we still don't have a name, a GUID will suffice
        lstrcpyn( m_szName, szDU, LENGTH_NAME ); 

    // Get type lib id
    // Get type lib id
    TCHAR szTypeLibValName[MAX_PATH];
    CatPathStrN(szTypeLibValName, m_szCLSID, HKCR_TYPELIB, MAX_PATH);
    dw = MAX_CLSID_LEN;
    lResult = RegQueryValue( hkeyCLSID, szTypeLibValName, m_szTypeLibID, (LONG*)&dw);
    if (lResult != ERROR_SUCCESS)
        (m_szTypeLibID)[0] = TEXT('\0');

    m_szCodeBase[0] ='\0';
    lResult = RegOpenKeyEx(hkeyDU, REGSTR_DOWNLOAD_INFORMATION, 0, KEY_READ, &hkeyDLInfo);
    if (lResult == ERROR_SUCCESS)
    {
        dw = INTERNET_MAX_URL_LENGTH;
        HRESULT hrErr = RegQueryValueEx(hkeyDLInfo, REGSTR_DLINFO_CODEBASE,
                                        NULL, NULL,
                                        (unsigned char *)m_szCodeBase,
                                        &dw);
        RegCloseKey( hkeyDLInfo );
    }

    // Get Version from DU branch

    m_szVersion[0] ='\0';
    lResult = RegOpenKeyEx(hkeyDU, REGSTR_INSTALLED_VERSION, 0,
                           KEY_READ, &hkeyVers);
    if (lResult == ERROR_SUCCESS)
    {
        dw = VERSION_MAXSIZE;
        RegQueryValueEx(hkeyVers, NULL, NULL, NULL,
                        (LPBYTE)m_szVersion,
                        &dw);
        RegCloseKey(hkeyVers);
    }
    
    // The version specified in the COM branch is the definitive word on
    // what the version is. If a key exists in the COM branch, use the version
    // that is found inside the InProcServer/LocalServer.

    if (RegOpenKeyEx( hkeyCLSID, szCLSID, 0, KEY_READ, &hkeyCOM ) == ERROR_SUCCESS) {

        LONG lcb = MAX_PATH;
        lResult = RegQueryValue( hkeyCOM, INPROCSERVER32, szNameT, &lcb );

        if ( lResult != ERROR_SUCCESS )
        {
            lcb = MAX_PATH;
            lResult = RegQueryValue( hkeyCOM, INPROCSERVER, szNameT, &lcb );
        }

        if ( lResult != ERROR_SUCCESS )
        {
            lcb = MAX_PATH;
            lResult = RegQueryValue( hkeyCOM, INPROCSERVERX86, szNameT, &lcb );
        }

        if ( lResult != ERROR_SUCCESS )
        {
            lcb = MAX_PATH;
            lResult = RegQueryValue( hkeyCOM, LOCALSERVER32, szNameT, &lcb );
        }

        if ( lResult != ERROR_SUCCESS )
        {
            lcb = MAX_PATH;
            lResult = RegQueryValue( hkeyCOM, LOCALSERVER, szNameT, &lcb );
        }

        if ( lResult != ERROR_SUCCESS )
        {
            lcb = MAX_PATH;
            lResult = RegQueryValue( hkeyCOM, LOCALSERVERX86, szNameT, &lcb );
        }

        RegCloseKey( hkeyCOM );

        // HACK! GetFileVersionInfoSize and GetFileVersionInfo modify
        // the last access time of the file under NT5! This causes us
        // to retrieve the wrong last access time when removing expired
        // controls. This hack gets the last access time before the
        // GetFileVersionInfo calls, and sets it back afterwards.
        // See IE5 RAID #56927 for details. This code should be removed
        // when NT5 fixes this bug.
        
        osvi.dwOSVersionInfoSize = sizeof(osvi);
        GetVersionEx(&osvi);

        if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT && osvi.dwMajorVersion == 5) {
            bRunOnNT5 = TRUE;
        }

        if (bRunOnNT5) {
            hFile = CreateFile(szNameT, GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                       
            if (hFile != INVALID_HANDLE_VALUE) {
                GetFileTime(hFile, NULL, &ftLastAccess, NULL);
                CloseHandle(hFile);
            }
        }
        
        dwVerSize = GetFileVersionInfoSize((char *)szNameT, &dwHandle);
        pbBuffer = new BYTE[dwVerSize];
        if (!pbBuffer)
        {
            return E_OUTOFMEMORY;
        }
        if (GetFileVersionInfo((char *)szFile, 0, dwVerSize, pbBuffer))
        {
            if (VerQueryValue(pbBuffer, "\\", (void **)&lpVSInfo, &uiVerSize))
            {
                wsprintf(m_szVersion, "%d,%d,%d,%d", (lpVSInfo->dwFileVersionMS >> 16) & 0xFFFF
                                                   , lpVSInfo->dwFileVersionMS & 0xFFFF
                                                   , (lpVSInfo->dwFileVersionLS >> 16) & 0xFFFF
                                                   , lpVSInfo->dwFileVersionLS & 0xFFFF);
            }
        }
            
        delete [] pbBuffer;

        if (bRunOnNT5) {
            hFile = CreateFile(szNameT, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile != INVALID_HANDLE_VALUE) {
                SetFileTime(hFile, NULL, &ftLastAccess, NULL);
                CloseHandle(hFile);
            }
        }

    }
        
    RegCloseKey( hkeyDU );

    return DoParseDU( m_szFile, m_szCLSID);
}

HRESULT MakeCacheItemFromControlList( HKEY hkeyClass, // HKCR\CLSID
                                      HKEY hkeyDist,  // HKLM\SOFTWARE\MICROSOFT\Code Store Database\Distribution Units
                                      LPCLSIDLIST_ITEM pcli,
                                      CCacheItem **ppci )
{
    HRESULT hr = E_FAIL;

    *ppci = NULL;
    if ( pcli->bIsDistUnit )
    {
        CCacheDistUnit *pcdu = new CCacheDistUnit();
        if ( pcdu != NULL &&
             SUCCEEDED(hr = pcdu->Init( hkeyClass,
                                   pcli->szFile,
                                   pcli->szCLSID, 
                                   hkeyDist, 
                                   pcli->szCLSID)) )
            *ppci = pcdu;
        else
            hr = E_OUTOFMEMORY;
    } 
    else
    {
        CCacheLegacyControl     *pclc = new CCacheLegacyControl();
        if ( pclc != NULL &&
             SUCCEEDED(hr = pclc->Init( hkeyClass,
                                        pcli->szFile, 
                                        pcli->szCLSID )) )
            *ppci = pclc;
        else
            hr = E_OUTOFMEMORY;

    }

    return hr;
}

OCCFindData::OCCFindData() : m_pcliHead(NULL), m_pcliTail(NULL)
{
    LONG    lResult;
    HKEY  hkeyCacheList;

    for ( int i = 0; i < cCachePathsMax; i++ )
    {
        m_aCachePath[i].m_cch = 0;
        m_aCachePath[i].m_sz[0] = '\0';
    }

    // Unhook occache as a shell extension for the cache folders.
    lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            REG_PATH_IE_CACHE_LIST,
                            0x0,
                            KEY_READ,
                            &hkeyCacheList );

    if ( lResult == ERROR_SUCCESS ) {
        DWORD dwIndex;
        TCHAR szName[MAX_PATH];
        DWORD cbName;
        DWORD cbValue;

        for ( dwIndex = 0, cbName = sizeof(szName), cbValue = MAX_PATH * sizeof(TCHAR); 
              dwIndex < cCachePathsMax; 
              dwIndex++, cbName = sizeof(szName), cbValue = MAX_PATH * sizeof(TCHAR) )
        {
            lResult = RegEnumValue( hkeyCacheList, dwIndex,
                                    szName, &cbName, 
                                    NULL, NULL,
                                    (LPBYTE)m_aCachePath[dwIndex].m_sz, &cbValue );
            m_aCachePath[dwIndex].m_cch = lstrlen( m_aCachePath[dwIndex].m_sz );
        }
        // We leave this key in place because it is the only record we have of the
        // cache folders and would be useful to future installations of IE
        RegCloseKey( hkeyCacheList );
    }
}

OCCFindData::~OCCFindData()
{
    if ( m_pcliHead )
        RemoveList(m_pcliHead);
}

BOOL OCCFindData::IsCachePath( LPCTSTR szPath )
{
    BOOL fMatch = FALSE;

    for ( int i = 0; i < cCachePathsMax && !fMatch; i++ )
        fMatch = m_aCachePath[i].m_cch != 0 &&
                 LStrNICmp( szPath, m_aCachePath[i].m_sz, m_aCachePath[i].m_cch ) == 0;
    return fMatch;
}

HRESULT OCCFindData::AddListItem( LPCTSTR szFile, LPCTSTR szCLSID, DWORD dwIsDistUnit )
{
    HRESULT hr = S_OK;

    if ( m_pcliTail == NULL )
    {
        m_pcliTail = new CLSIDLIST_ITEM;
        if (m_pcliHead == NULL)
            m_pcliHead = m_pcliTail;
    }
    else
    {
        m_pcliTail->pNext = new CLSIDLIST_ITEM;
        m_pcliTail = m_pcliTail->pNext;
    }

    if ( m_pcliTail != NULL ) 
    {
        m_pcliTail->pNext = NULL;
        lstrcpyn(m_pcliTail->szFile, szFile, MAX_PATH);
        lstrcpyn(m_pcliTail->szCLSID, szCLSID, MAX_DIST_UNIT_NAME_LEN);
        m_pcliTail->bIsDistUnit = dwIsDistUnit;
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}

LPCLSIDLIST_ITEM OCCFindData::TakeFirstItem(void)
{
    LPCLSIDLIST_ITEM pcli = m_pcliHead;
 
    if (m_pcliHead != NULL)
    {
        m_pcliHead = m_pcliHead;
        m_pcliHead = m_pcliHead->pNext;
        if ( m_pcliHead == NULL )
            m_pcliTail = NULL;
    }

    return pcli;
}

BOOL IsDUDisplayable(HKEY hkeyDU)
{
    DWORD                 dwType = 0;
    DWORD                 dwSystem = 0;
    DWORD                 dwSize = 0;
    long                  lResult = 0;
    BOOL                  bRet;

    if (!hkeyDU) {
        bRet = FALSE;
        goto Exit;
    }

    if (IsShowAllFilesEnabled()) {
        bRet = TRUE;
        goto Exit;
    }
    
    dwSize = sizeof(DWORD);
    lResult = RegQueryValueEx(hkeyDU, VALUE_SYSTEM, NULL,
                              &dwType, (LPBYTE)&dwSystem, &dwSize);

    bRet = (lResult == ERROR_SUCCESS && dwSystem == TRUE) ? (FALSE) : (TRUE);

Exit:
    return bRet;
}

BOOL IsShowAllFilesEnabled()
{
    HKEY                        hkey = 0;
    BOOL                        bRet = FALSE;
    DWORD                       lResult = 0;
    DWORD                       dwType = 0;
    DWORD                       dwSize = sizeof(DWORD);
    DWORD                       dwShowAll = 0;

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_DIST_UNITS, 0,
                           KEY_READ, &hkey);

    if (lResult == ERROR_SUCCESS) {
        lResult = RegQueryValueEx(hkey, REGSTR_SHOW_ALL_FILES, NULL, &dwType,
                                  (LPBYTE)&dwShowAll, &dwSize);
    }

    if (lResult == ERROR_SUCCESS) {
        bRet = (dwShowAll != 0);
    }
                                  
    if (hkey) {
        RegCloseKey(hkey);
    }

    return bRet;
}

void ToggleShowAllFiles()
{
    HKEY                        hkey = 0;
    DWORD                       lResult = 0;
    DWORD                       dwShowAll = !IsShowAllFilesEnabled();

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_DIST_UNITS, 0,
                           KEY_ALL_ACCESS, &hkey);

    if (lResult == ERROR_SUCCESS) {
        RegSetValueEx(hkey, REGSTR_SHOW_ALL_FILES, 0, REG_DWORD,
                      (CONST BYTE *)&dwShowAll, sizeof(DWORD));
    }

    if (hkey) {
        RegCloseKey(hkey);
    }
}

LONG WINAPI FindFirstControl(
                         HANDLE& hFindHandle,
                         HANDLE& hControlHandle, 
                         LPCTSTR lpszCachePath /* = NULL */
                         )
{
    LONG                lResult = ERROR_SUCCESS;
    HRESULT             hr = S_OK;
    DWORD               dw = 0;
    HKEY                hKeyClass = NULL;
    HKEY                hKeyClsid = NULL;
    HKEY                hKeyMod = NULL;
    HKEY                hkeyMUEntry = NULL;
    HKEY                hKeyDist = NULL;
    TCHAR               szT[MAX_PATH];             // scratch buffer
    int                 cEnum = 0;
    CCacheItem          *pci = NULL;
    OCCFindData         *poccfd = new OCCFindData();
    LPCLSIDLIST_ITEM    pcli = NULL;
    TCHAR               szDUName[MAX_DIST_UNIT_NAME_LEN];
    
    if ( poccfd == NULL )
    {
        lResult = ERROR_NOT_ENOUGH_MEMORY;
        goto EXIT_FINDFIRSTCONTROL;
    }
    
    // Open up the HKCR\CLSID key.
    lResult = RegOpenKeyEx(HKEY_CLASSES_ROOT, HKCR_CLSID, 0, KEY_READ, &hKeyClass);
    if (lResult != ERROR_SUCCESS)
        goto EXIT_FINDFIRSTCONTROL;

    // Search for legacy controls found in the COM branch
    if ((lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_MODULE_USAGE,
                                0, KEY_READ, &hKeyMod)) != ERROR_SUCCESS)
        goto EXIT_FINDFIRSTCONTROL;

    // Enumerate the known modules and build up a list of the owners.
    // This is a search for legacy controls.
    while ((lResult = RegEnumKey(
                        hKeyMod,
                        cEnum++,
                        szT,
                        MAX_PATH)) == ERROR_SUCCESS)
    {
        TCHAR        szClient[MAX_CLIENT_LEN];

        lResult = RegOpenKeyEx( hKeyMod, szT, 0, KEY_READ, &hkeyMUEntry );

        Assert( lResult == ERROR_SUCCESS ); // Hey, we just got the key name

        DWORD dwType;

        dw = MAX_CLIENT_LEN * sizeof(TCHAR);

        // Fetch the module owner.
        lResult = RegQueryValueEx( hkeyMUEntry, VALUE_OWNER, NULL, &dwType, (LPBYTE)szClient, &dw );

        Assert( lResult == ERROR_SUCCESS ); // assert that mu entry is properly formed.
        // If the module owner is in the COM branch AND
        //    ( the owner lives in the cache OR it has an INF in the cache )
        // Then add the _owner_ to our list of legacy controls.
        // In the INF case, we may be looking at a control that was re-registered
        // outside of the cache.
        // If it doesn't have these properties, then it is either a DU module or
        // was installed by something other than MSICD. In either case, we'll skip it
        // at least for now.
        lResult = RegOpenKeyEx(hKeyClass, szClient, 0, KEY_READ, &hKeyClsid);
        if (lResult == ERROR_SUCCESS)
        {
            TCHAR szCLocation[MAX_PATH];     // Canonical path of control
            TCHAR szLocation[MAX_PATH];      // Location in COM CLSID reg tree.

            // Look for InprocServer[32] or LocalServer[32] key
            dw = MAX_PATH;
            lResult = RegQueryValue(hKeyClsid, INPROCSERVER32, szLocation, (PLONG)&dw);
            if (lResult != ERROR_SUCCESS)
            {
                dw = MAX_PATH;
                lResult = RegQueryValue(hKeyClsid, LOCALSERVER32, szLocation, (PLONG)&dw);
            }

            RegCloseKey(hKeyClsid);

            if ( lResult == ERROR_SUCCESS )
            {
                BOOL bAddOwner;

                // see if we've already got an entry for this one.
                for ( pcli = poccfd->m_pcliHead;
                      pcli != NULL && lstrcmp( szClient, pcli->szCLSID ) != 0;
                      pcli = pcli->pNext );
                
                if ( pcli == NULL ) // not found - possibly add new item
                {
                    // Canonicalize the path for use in comparisons with cache dirs
                    if ( OCCGetLongPathName(szCLocation, szLocation, MAX_PATH) == 0 )
                        lstrcpyn( szCLocation, szLocation, MAX_PATH );

                    // Is the owner in our cache?
                    bAddOwner = poccfd->IsCachePath( szCLocation );

                    if ( !bAddOwner )
                    {
                        // does it have an INF in our cache(s)?
                        // We'll appropriate szDCachePath
                        for ( int i = 0; i < cCachePathsMax && !bAddOwner; i++ )
                        {
                            if ( poccfd->m_aCachePath[i].m_sz != '\0' )
                            {
                                CatPathStrN( szT, poccfd->m_aCachePath[i].m_sz, PathFindFileName( szCLocation ), MAX_PATH);

                                // Note if another copy of the owner exists within the cache(s).
                                // This would be a case of re-registration.
                                if ( PathFileExists( szT ) )
                                {
                                    // add our version of the control.
                                    lstrcpyn( szCLocation, szT, MAX_PATH );
                                    bAddOwner = TRUE;
                                }
                                else
                                    bAddOwner =  PathRenameExtension( szT, INF_EXTENSION ) &&
                                                 PathFileExists( szT );
                            } // if cache path
                        } // for each cache directory
                    } // if check for cached INF

                    if ( bAddOwner ) {
                        HKEY                        hkeyDUCheck = 0;
                        char                        achBuf[MAX_REGPATH_LEN];

                        wnsprintfA(achBuf, MAX_REGPATH_LEN, "%s\\%s",
                                   REGSTR_PATH_DIST_UNITS, szClient);

                        lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                               achBuf, 0, KEY_READ,
                                               &hkeyDUCheck);

                        if (lResult != ERROR_SUCCESS) {
                            // This is a legacy control with no corresponding DU
                            poccfd->AddListItem( szCLocation, szClient, FALSE );
                        }
                        else {
                            if (IsDUDisplayable(hkeyDUCheck)) {
                                // Legacy control w/ DU keys that is displayable

                                poccfd->AddListItem( szCLocation, szClient, FALSE );
                            }
                            RegCloseKey(hkeyDUCheck);
                        }
                    }
                } // if owner we haven't seen before
            } // if owner has local or inproc server
        } // if owner has COM entry 
        RegCloseKey( hkeyMUEntry );
    } // while enumerating Module Usage
 
    // we're finished with module usage
    RegCloseKey(hKeyMod);

    // Now search distribution units

    // Check for duplicates - distribution units for controls we detected above

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_DIST_UNITS, 0,
                           KEY_READ, &hKeyDist);
    if (lResult == ERROR_SUCCESS)
    {
        cEnum = 0;
        // Enumerate distribution units and queue them up in the list
        while ((lResult = RegEnumKey(hKeyDist, cEnum++, szDUName,
                                     MAX_DIST_UNIT_NAME_LEN)) == ERROR_SUCCESS)
        {
            // We should only display DU's installed by code download.
            HKEY  hkeyDU;
            DWORD dwType;
            DWORD cb = MAX_PATH * sizeof(TCHAR);
            
            lResult = RegOpenKeyEx( hKeyDist, szDUName, 0, KEY_READ, &hkeyDU );
            Assert( lResult == ERROR_SUCCESS );

            if (!IsDUDisplayable(hkeyDU)) {
                continue;
            }

            szT[0] = '\0';
            lResult = RegQueryValueEx( hkeyDU, DU_INSTALLER_VALUE, NULL, &dwType, (LPBYTE)szT, &cb );
            
            Assert( lResult == ERROR_SUCCESS ); // properly-formed DU will have this
            Assert( dwType == REG_SZ );         // properly-formed DU's have a string here

            // Check for an installed version. We might just have a DU that has an AvailableVersion
            // but hasn't been installed yet.
            lResult = RegQueryValue( hkeyDU, REGSTR_INSTALLED_VERSION, NULL, NULL );

            RegCloseKey( hkeyDU );

            if ( lstrcmpi( szT, CDL_INSTALLER ) == 0 &&
                 lResult == ERROR_SUCCESS // from InstalledVersion RegQueryValue
               )
            {
                // If we can convert the unique name to a GUID, then this DU
                // may have already been added on the first pass through the
                // COM branch.
                CLSID       clsidDummy = CLSID_NULL;
                LPOLESTR    szDummyStr[MAX_CTRL_NAME_SIZE];
                BOOL        bFoundDuplicate = FALSE;

                MultiByteToWideChar(CP_ACP, 0, szDUName, -1, (LPOLESTR)szDummyStr, MAX_CTRL_NAME_SIZE);
                if ((CLSIDFromString((LPOLESTR)szDummyStr, &clsidDummy) == S_OK))
                {
                    for (pcli = poccfd->m_pcliHead;
                         pcli != NULL;
                         pcli = pcli->pNext)
                    {
                        if (!lstrcmpi(szDUName, pcli->szCLSID))
                        {
                            // Duplicate found. Use dist unit information to
                            // fill in additional fields if it is the first
                            // entry in the list
                            bFoundDuplicate = TRUE;
                            pcli->bIsDistUnit = TRUE;
                            break;
                        }
                    }                     
                }

                if (!bFoundDuplicate)
                {
                        // Okay we're looking at some sort of Java scenario. We have a distribution unit, but
                    // no corresponding entry in the COM branch. This generally means we've got a DU that
                    // consists of java packages. It can also mean that we're dealing with a java/code download
                    // backdoor introduced in IE3. In this case, an Object tag gets a CAB downloaded that
                    // installs Java classes and sets of a CLSID that invokes MSJava.dll on the class ( ESPN's
                    // sportszone control/applet works this way ). In the first case, we get the name
                    // squared-away when we parse the DU. In the latter case, we need to try and pick the name
                    // up from the COM branch.  
                    hr = poccfd->AddListItem( "", szDUName, TRUE );
                    if ( FAILED(hr) )
                    {
                        lResult = ERROR_NOT_ENOUGH_MEMORY;
                        goto EXIT_FINDFIRSTCONTROL;
                    }
                } // if no duplicate - add DU to the list
            } // if installed by MSICD
        } // while enumerating DU's
    } // if we can open the DU key.
    else
        lResult = ERROR_NO_MORE_ITEMS; // if no DU's then make due with our legacy controls, if any

    pcli = poccfd->TakeFirstItem();
    if ( pcli != NULL )
    {
        hr  = MakeCacheItemFromControlList( hKeyClass,
                                            hKeyDist,
                                            pcli,
                                            &pci );
        delete pcli;

        if ( FAILED(hr) )
            lResult = hr;
    }

    if (hKeyDist)
    {
        RegCloseKey(hKeyDist);
        hKeyDist = 0;
    }


    // Clean up

    if (lResult != ERROR_NO_MORE_ITEMS)
        goto EXIT_FINDFIRSTCONTROL;

    if (pci == NULL)
        lResult = ERROR_NO_MORE_ITEMS;
    else
    {
        lResult = ERROR_SUCCESS;
    }

    hFindHandle = (HANDLE)poccfd;
    hControlHandle = (HANDLE)pci;

EXIT_FINDFIRSTCONTROL:

    if (hKeyDist)
        RegCloseKey(hKeyDist);

    if (hKeyClass)
        RegCloseKey(hKeyClass);

    if (lResult != ERROR_SUCCESS)
    {
        if ( pci != NULL )
            delete pci;
        if ( poccfd != NULL )
            delete poccfd;
        hFindHandle = INVALID_HANDLE_VALUE;
        hControlHandle = INVALID_HANDLE_VALUE;
    }

    return lResult;
}


LONG WINAPI FindNextControl(
                         HANDLE& hFindHandle,
                         HANDLE& hControlHandle
                         )
{
    LONG         lResult = ERROR_SUCCESS;
    HRESULT      hr = S_OK;
    HKEY         hKeyClass = NULL;
    
    CCacheItem   *pci = NULL;
    OCCFindData  *poccfd = (OCCFindData *)hFindHandle;

    LPCLSIDLIST_ITEM pcli = poccfd->TakeFirstItem();
    hControlHandle = INVALID_HANDLE_VALUE;

    if (pcli == NULL)
    {
        lResult = ERROR_NO_MORE_ITEMS;
        goto EXIT_FINDNEXTCONTROL;
    }

    if ((lResult = RegOpenKeyEx(HKEY_CLASSES_ROOT, HKCR_CLSID, 0, KEY_READ, &hKeyClass)) != ERROR_SUCCESS)
        goto EXIT_FINDNEXTCONTROL;

    if ( pcli->bIsDistUnit )
    {
        HKEY hKeyDist;

        lResult = RegOpenKeyEx( HKEY_LOCAL_MACHINE, REGSTR_PATH_DIST_UNITS, 0,
                                KEY_READ, &hKeyDist);

        if ( lResult == ERROR_SUCCESS ) 
        {
            hr  = MakeCacheItemFromControlList( hKeyClass,
                                                hKeyDist,
                                                pcli,
                                                &pci );
            if ( FAILED(hr) )
                lResult = hr;

            RegCloseKey( hKeyDist );
        }
    }
    else
    {
        // This is not a distribution unit. Fill in CCachItem information
        // from the COM branch.
        hr  = MakeCacheItemFromControlList( hKeyClass,
                                            NULL,
                                            pcli,
                                            &pci );
        if ( FAILED(hr) )
            lResult = hr;
    }

    hControlHandle = (HANDLE)pci;

EXIT_FINDNEXTCONTROL:

    if (hKeyClass)
        RegCloseKey(hKeyClass);

    if (pcli != NULL)
    {
        delete pcli;
    }

    return lResult;
}

void WINAPI FindControlClose(HANDLE hFindHandle)
{
    if (hFindHandle == INVALID_HANDLE_VALUE ||
        hFindHandle == (HANDLE)0)
        return;

    delete (OCCFindData*)hFindHandle;
}

void WINAPI ReleaseControlHandle(HANDLE hControlHandle)
{
    if (hControlHandle == INVALID_HANDLE_VALUE ||
        hControlHandle == (HANDLE)0)
        return;

    delete (CCacheItem *)hControlHandle;
}

HRESULT WINAPI RemoveControlByHandle(
                         HANDLE hControlHandle,
                         BOOL bForceRemove /* = FALSE */
                         )
{
    return RemoveControlByHandle2( hControlHandle, bForceRemove, FALSE );
}



HRESULT WINAPI RemoveControlByName(
                         LPCTSTR lpszFile,
                         LPCTSTR lpszCLSID,
                         LPCTSTR lpszTypeLibID,
                         BOOL bForceRemove, /* = FALSE */
                         DWORD dwIsDistUnit /* = FALSE */
                         )
{
    return RemoveControlByName2( lpszFile, lpszCLSID, lpszTypeLibID, bForceRemove, dwIsDistUnit, FALSE);
}

LONG WINAPI GetControlDependentFile(
                             int iFile,
                             HANDLE hControlHandle,
                             LPTSTR lpszFile,
                             LPDWORD lpdwSize,
                             BOOL bToUpper /* = FALSE */
                             )
{
    CCacheItem *pci = (CCacheItem *)hControlHandle;

    if (iFile < 0 || lpszFile == NULL || lpdwSize == NULL)
        return ERROR_BAD_ARGUMENTS;

    // loop through the list of files to find the one indicated
    // by the given index.
    // this way is dumb but since a control does not depend on
    // too many files, it's ok
    CFileNode *pFileNode = pci->GetFirstFile();
    for (int i = 0; i < iFile && pFileNode != NULL; i++)
        pFileNode = pci->GetNextFile();

    if (pFileNode == NULL)
    {
        lpszFile[0] = TEXT('\0');
        lpdwSize = 0;
        return ERROR_NO_MORE_FILES;
    }

    // Make a fully qualified filename
    if (pFileNode->GetPath() != NULL)
    {
        CatPathStrN( lpszFile, pFileNode->GetPath(), pFileNode->GetName(), MAX_PATH);
    }
    else
    {
        lstrcpy(lpszFile, pFileNode->GetName());
    }

    if (FAILED(GetSizeOfFile(lpszFile, lpdwSize)))
        *lpdwSize = 0;

    // to upper case if required
    if (bToUpper)
        CharUpper(lpszFile);

    return ERROR_SUCCESS;
}

// determine if a control or one of its associated files can be removed
// by reading its SharedDlls count
BOOL WINAPI IsModuleRemovable(LPCTSTR lpszFile)
{
    TCHAR szFile[MAX_PATH];
    TCHAR szT[MAX_PATH];

    if (lpszFile == NULL)
        return FALSE;

    if ( OCCGetLongPathName(szFile, lpszFile, MAX_PATH) == 0 )
        lstrcpyn( szFile, lpszFile, MAX_PATH );

    // Don't ever pull something out of the system directory.
    // This is a "safe" course of action because it is not reasonable
    // to expect the user to judge whether yanking this file damage other
    // software installations or the system itself.
    GetSystemDirectory(szT, MAX_PATH);
    if (StrStrI(szFile, szT))
        return FALSE;

    // check moduleusage if a control is safe to remove
    if (LookUpModuleUsage(szFile, NULL, szT, MAX_PATH) != S_OK)
        return FALSE;

    // if we don't know who the owner of the module is, it's not
    // safe to remove
    if (lstrcmpi(szT, UNKNOWNOWNER) == 0)
        return FALSE;
    else
    {
        // check shareddlls if a control is safe to remove
        LONG cRef;

        HRESULT hr = SetSharedDllsCount( szFile, -1, &cRef );

        return cRef == 1;
    }
}

BOOL WINAPI GetControlInfo(
                      HANDLE hControlHandle, 
                      UINT nFlag,
                      LPDWORD lpdwData,
                      LPTSTR lpszData,
                      int nBufLen)
{
    if (hControlHandle == 0 || hControlHandle == INVALID_HANDLE_VALUE)
        return FALSE;

    BOOL bResult = TRUE;
    LPCTSTR lpStr = NULL;
    DWORD dw = 0;

    switch (nFlag)
    {
    case GCI_NAME:     // get friend name of control
        lpStr = ((CCacheItem *)hControlHandle)->m_szName;
        break;
    case GCI_FILE:     // get filename of control (with full path)
        lpStr = ((CCacheItem *)hControlHandle)->m_szFile;
        // if there is no file, but there is a package list, fake it
        // with the path to the first package's ZIP file.
        if ( *lpStr == '\0' )
        {
            CPNode *ppn = ((CCacheItem *)hControlHandle)->GetFirstPackage();
            if ( ppn != NULL )
                lpStr = ppn->GetPath();
        }

        if ( *lpStr == '\0' )
        {
            CPNode *pfn = ((CCacheItem *)hControlHandle)->GetFirstFile();
            if ( pfn != NULL )
                lpStr = pfn->GetPath();
        }

        break;
    case GCI_DIST_UNIT_VERSION:
        lpStr = ((CCacheItem *)hControlHandle)->m_szVersion;
        break;        
    case GCI_CLSID:    // get CLSID of control
        lpStr = ((CCacheItem *)hControlHandle)->m_szCLSID;
        break;
    case GCI_TYPELIBID:  // get TYPELIB id of control
        lpStr = ((CCacheItem *)hControlHandle)->m_szTypeLibID;
        break;
    case GCI_TOTALSIZE:  // get total size in bytes
        dw = ((CCacheItem *)hControlHandle)->GetTotalFileSize();
        break;
    case GCI_SIZESAVED:  // get total size restored if control is removed
        dw = ((CCacheItem *)hControlHandle)->GetTotalSizeSaved();
        break;
    case GCI_TOTALFILES:  // get total number of files related to control
        dw = (DWORD)(((CCacheItem *)hControlHandle)->GetTotalFiles());
        break;
    case GCI_CODEBASE:  // get CodeBase for control
        lpStr = ((CCacheItem *)hControlHandle)->m_szCodeBase;
        break;
    case GCI_ISDISTUNIT:
        dw = ((CCacheItem *)hControlHandle)->ItemType() == CCacheDistUnit::s_dwType;
        break;
    case GCI_STATUS:
        dw = ((CCacheItem *)hControlHandle)->GetStatus();
        break;
    case GCI_HAS_ACTIVEX:
        dw = ((CCacheItem *)hControlHandle)->GetHasActiveX();
        break;
    case GCI_HAS_JAVA:
        dw = ((CCacheItem *)hControlHandle)->GetHasJava();
        break;
    };

    if (nFlag == GCI_TOTALSIZE ||
        nFlag == GCI_SIZESAVED ||
        nFlag == GCI_TOTALFILES ||
        nFlag == GCI_ISDISTUNIT ||
        nFlag == GCI_STATUS ||
        nFlag == GCI_HAS_ACTIVEX ||
        nFlag == GCI_HAS_JAVA )
    {
        bResult = (lpdwData != NULL);
        if (bResult)
            *lpdwData = dw;
    }
    else
    {
        bResult = (lpszData != NULL && nBufLen > lstrlen(lpStr));
        if (bResult)
            lstrcpy(lpszData, lpStr);
    }

    return bResult;
}

///////////////////////////////////////////////////////////////////////////////
// API to be called by Advpack.dll

// Define list node to be used in a linked list of control
struct tagHANDLENODE;
typedef struct tagHANDLENODE HANDLENODE;
typedef HANDLENODE* LPHANDLENODE;
struct tagHANDLENODE
{
    HANDLE hControl;
    struct tagHANDLENODE* pNext;
};

// Given a handle to a control, get the control's last access time
// Result is stored in a FILETIME struct
HRESULT GetLastAccessTime(HANDLE hControl, FILETIME *pLastAccess)
{
    Assert(hControl != NULL && hControl != INVALID_HANDLE_VALUE);
    Assert(pLastAccess != NULL);

    HRESULT hr = S_OK;
    WIN32_FIND_DATA fdata;
    HANDLE h = INVALID_HANDLE_VALUE;
    LPCTSTR  lpszFile = NULL;
    CCacheItem *pci = (CCacheItem *)hControl;
    CPNode *ppn;

    if (pci->m_szFile[0] != 0)
        lpszFile = pci->m_szFile;
    else if ( (ppn = pci->GetFirstPackage()) != NULL )
        lpszFile = ppn->GetPath();
    else if ( (ppn = pci->GetFirstFile()) != NULL )
        lpszFile = ppn->GetPath();
        
    if ( lpszFile )
        h = FindFirstFile(lpszFile, &fdata);

    if (h == INVALID_HANDLE_VALUE)
    {
        SYSTEMTIME stNow;
        GetLocalTime(&stNow);
        SystemTimeToFileTime(&stNow, pLastAccess); 
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        // Convert file time to local file time, then file time to 
        // system time.  Set those fields to be ignored to 0, and
        // set system time back to file time.
        // FILETIME struct is used because API for time comparison
        // only works on FILETIME.

//        SYSTEMTIME sysTime;

        FindClose(h);
        FileTimeToLocalFileTime(&(fdata.ftLastAccessTime), pLastAccess);
    }

    return hr;
}

HRESULT WINAPI SweepControlsByLastAccessDate(
                              SYSTEMTIME *pLastAccessTime /* = NULL */,
                              PFNDOBEFOREREMOVAL pfnDoBefore /* = NULL */,
                              PFNDOAFTERREMOVAL pfnDoAfter /* = NULL */,
                              DWORD dwSizeLimit /* = 0 */
                              )
{
    LONG lResult = ERROR_SUCCESS;
    HRESULT hr = S_FALSE;
    DWORD dwSize = 0, dwTotalSize = 0;
    HANDLE hFind = NULL, hControl = NULL;
    LPHANDLENODE pHead = NULL, pCur = NULL;
    FILETIME timeLastAccess, timeRemovePrior;
    UINT cCnt = 0;
    TCHAR szFile[MAX_PATH];

    // ignore all fields except wYear, wMonth and wDay
    if (pLastAccessTime != NULL)
    {
        pLastAccessTime->wDayOfWeek = 0; 
        pLastAccessTime->wHour = 0; 
        pLastAccessTime->wMinute = 0; 
        pLastAccessTime->wSecond = 0; 
        pLastAccessTime->wMilliseconds = 0; 
    }

    // loop through all controls and put in a list the
    // ones that are accessed before the given date and
    // are safe to uninstall
    lResult = FindFirstControl(hFind, hControl);
    for (;lResult == ERROR_SUCCESS;
          lResult = FindNextControl(hFind, hControl))
    {
        // check last access time
        if (pLastAccessTime != NULL)
        {
            GetLastAccessTime(hControl, &timeLastAccess);
            SystemTimeToFileTime(pLastAccessTime, &timeRemovePrior);
            if (CompareFileTime(&timeLastAccess, &timeRemovePrior) > 0)
            {
                ReleaseControlHandle(hControl);
                continue;
            }
        }

        // check if control is safe to remove
        GetControlInfo(hControl, GCI_FILE, NULL, szFile, MAX_PATH);
        if (!IsModuleRemovable(szFile))
        {
            ReleaseControlHandle(hControl);
            continue;
        }

        // put control in a list
        if (pHead == NULL)
        {
            pHead = new HANDLENODE;
            pCur = pHead;
        }
        else
        {
            pCur->pNext = new HANDLENODE;
            pCur = pCur->pNext;
        }

        if (pCur == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto EXIT_REMOVECONTROLBYLASTACCESSDATE;
        }
        
        pCur->pNext = NULL;
        pCur->hControl = hControl;
        cCnt += 1;

        // calculate total size
        GetControlInfo(pCur->hControl, GCI_SIZESAVED, &dwSize, NULL, NULL);
        dwTotalSize += dwSize;
    }
        
    // quit if total size restored is less than the given amount
    if (dwTotalSize < dwSizeLimit)
        goto EXIT_REMOVECONTROLBYLASTACCESSDATE;

    // traverse the list and remove each control
    for (pCur = pHead; pCur != NULL; cCnt--)
    {
        hr = S_OK;
        pHead = pHead->pNext;

        // call callback function before removing a control
        if (pfnDoBefore == NULL || SUCCEEDED(pfnDoBefore(pCur->hControl, cCnt)))
        {
            hr = RemoveControlByHandle(pCur->hControl);

            // call callback function after removing a control, passing it the
            // result of the removal
            if (pfnDoAfter != NULL && FAILED(pfnDoAfter(hr, cCnt - 1)))
            {
                pHead = pCur;   // set pHead back to head of list
                goto EXIT_REMOVECONTROLBYLASTACCESSDATE;
            }
        }

        // release memory used by the control handle
        ReleaseControlHandle(pCur->hControl);
        delete pCur;
        pCur = pHead;
    }

EXIT_REMOVECONTROLBYLASTACCESSDATE:

    FindControlClose(hFind);

    // release memory taken up by the list
    for (pCur = pHead; pCur != NULL; pCur = pHead)
    {
        pHead = pHead->pNext;
        ReleaseControlHandle(pCur->hControl);
        delete pCur;
    }

    return hr;
}

HRESULT WINAPI RemoveExpiredControls(DWORD dwFlags, DWORD dwReserved)
{
    LONG lResult = ERROR_SUCCESS;
    HRESULT hr = S_FALSE;
    HANDLE hFind = NULL, hControl = NULL;
    LPHANDLENODE pHead = NULL, pCur = NULL;
    FILETIME ftNow, ftMinLastAccess, ftLastAccess;
    LARGE_INTEGER liMinLastAccess;
    SYSTEMTIME stNow;
    UINT cCnt = 0;

    GetLocalTime( &stNow );
    SystemTimeToFileTime(&stNow, &ftNow);

    // loop through all controls and put in a list the
    // ones that are accessed before the given date and
    // are safe to uninstall
    lResult = FindFirstControl(hFind, hControl);
    for (;lResult == ERROR_SUCCESS;
          lResult = FindNextControl(hFind, hControl))
    {
        CCacheItem *pci = (CCacheItem *)hControl;

        // Controls must have a last access time of at least ftMinLastAccess or they will
        // expire by default. If they have the Office Auto-expire set, then they may
        // have to pass a higher bar.

        liMinLastAccess.LowPart = ftNow.dwLowDateTime;
        liMinLastAccess.HighPart = ftNow.dwHighDateTime;
        // We add one to GetExpireDays to deal with bug  17151. The last access time
        // returned by the file system is truncated down to 12AM, so we need to 
        // expand the expire interval to ensure that this truncation does not cause
        // the control to expire prematurely.
        liMinLastAccess.QuadPart -= ((pci->GetExpireDays()+1) * 864000000000L); //24*3600*10^7
        ftMinLastAccess.dwLowDateTime = liMinLastAccess.LowPart;
        ftMinLastAccess.dwHighDateTime = liMinLastAccess.HighPart;

        GetLastAccessTime(hControl, &ftLastAccess); // ftLastAccess is a local file time

        if (CompareFileTime(&ftLastAccess, &ftMinLastAccess) >= 0)
        {
            ReleaseControlHandle(hControl);
            continue;
        }


        // put control in a list
        if (pHead == NULL)
        {
            pHead = new HANDLENODE;
            pCur = pHead;
        }
        else
        {
            pCur->pNext = new HANDLENODE;
            pCur = pCur->pNext;
        }

        if (pCur == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }
        
        pCur->pNext = NULL;
        pCur->hControl = hControl;
        cCnt += 1;
    }

    // traverse the list and remove each control
    for (pCur = pHead; pCur != NULL; cCnt--)
    {
        hr = S_OK;
        pHead = pHead->pNext;

        hr = RemoveControlByHandle2(pCur->hControl, FALSE, TRUE);

        // release memory used by the control handle
        ReleaseControlHandle(pCur->hControl);
        delete pCur;
        pCur = pHead;
    }

cleanup:

    FindControlClose(hFind);

    // release memory taken up by the list, if any left
    for (pCur = pHead; pCur != NULL; pCur = pHead)
    {
        pHead = pHead->pNext;
        ReleaseControlHandle(pCur->hControl);
        delete pCur;
    }

    return hr;
}

