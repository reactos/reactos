//  File:       util.h
//
//  Contents:   Utility classes.
//
//  Classes:    CRefCount
//
//  Functions://
//  History: 
//
//----------------------------------------------------------------------------

#ifndef _URLZONE_UTIL_H_
#define _URLZONE_UTIL_H_

// Declarations for global variables.
extern BOOL g_bUseHKLMOnly;
extern BOOL g_bInit;

extern BOOL IsZonesInitialized();
extern HINSTANCE g_hInst;

// Cache of drive letter to drive type.
extern DWORD GetDriveTypeFromCacheA(LPCSTR psz);

// Is this file under the Cache directory.
extern BOOL IsFileInCacheDir(LPCWSTR pszFile);
 

// Replacement for ultoa, works with wide-chars.
extern BOOL DwToWchar (DWORD dw, LPWSTR lpwsz, int radix);

#ifdef unix
#undef offsetof
#endif /* unix */
#define offsetof(s,m) ( (SIZE_T) &( ((s*)NULL)->m) )
#define GETPPARENT(pmemb, struc, membname) ((struc*)(((char*)(pmemb))-offsetof(struc, membname)))

// Does this file bear the Mark of the Web
extern BOOL FileBearsMarkOfTheWeb(LPCTSTR pszFile, LPWSTR *ppszURLMark);

EXTERN_C HRESULT ZonesDllInstall(BOOL bInstall, LPCWSTR pwStr);

/////////////////////////////////////////////////////////////////////////////
// CRegKey

class CRegKey
{
public:
    CRegKey(BOOL bHKLMOnly);
    CRegKey();      
    ~CRegKey();

// Attributes
public:
    operator HUSKEY() const;
    HUSKEY m_hKey;
    BOOL m_bHKLMOnly;

// Operations
public:
    LONG SetValue(DWORD dwValue, LPCTSTR lpszValueName);                // DWORD
    LONG SetValue(LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL);     // STRING
    LONG SetBinaryValue(const BYTE *pb, LPCTSTR lpszValueName, DWORD dwCount);        // BINARY
    LONG SetValueOfType(const BYTE *pb, LPCTSTR lpszValueName, DWORD dwCount, DWORD dwType); // ANY TYPE

    LONG QueryValue(DWORD* pdwValue, LPCTSTR lpszValueName);
    LONG QueryValue(LPTSTR szValue, LPCTSTR lpszValueName, DWORD* pdwCount);
    LONG QueryBinaryValue(LPBYTE pb, LPCTSTR lpszValueName, DWORD *pdwCount);
    
    LONG QueryValueOrWild (DWORD* pdwValue, LPCTSTR lpszValueName)
    {
        if (ERROR_SUCCESS == QueryValue (pdwValue, lpszValueName))
            return ERROR_SUCCESS;
        else
            return QueryValue (pdwValue, TEXT("*"));
    }

    LONG SetKeyValue(LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL);
    static LONG WINAPI SetValue(HUSKEY hKeyParent, LPCTSTR lpszKeyName,
        LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL, BOOL bHKLMOnly = FALSE);

    inline LONG QuerySubKeyInfo (DWORD* pdwNumKeys, DWORD* pdwMaxLen, DWORD *pdwNumValues);

    LONG EnumKey(DWORD dwIndex, LPTSTR lpszName, DWORD* pcchName);
    LONG EnumValue(DWORD dwIndex, LPTSTR pszValueName, LPDWORD pcchValueNameLen, 
                    LPDWORD pdwType, LPVOID pvData, LPDWORD pcbData);

    LONG Create(
        HUSKEY hKeyParent, // OPTIONAL
        LPCTSTR lpszKeyName,
        REGSAM samDesired);
        
    LONG Open(
        HUSKEY hKeyParent,  // OPTIONAL
        LPCTSTR lpszKeyName,
        REGSAM samDesired);
        
    LONG Close();
    HUSKEY Detach();
    void Attach(HUSKEY hKey);
    LONG DeleteValue(LPCTSTR lpszValue);
    LONG DeleteEmptyKey(LPCTSTR pszSubKey);

private:
    inline DWORD RegSetFlags() const 
    { return m_bHKLMOnly ? SHREGSET_FORCE_HKLM : SHREGSET_FORCE_HKCU; } 

    inline SHREGDEL_FLAGS RegDelFlags() const
    { return m_bHKLMOnly ? SHREGDEL_HKLM : SHREGDEL_HKCU; } 

    inline SHREGENUM_FLAGS RegEnumFlags() const
    { return m_bHKLMOnly ? SHREGENUM_HKLM : SHREGENUM_DEFAULT; }
};

inline CRegKey::CRegKey(BOOL bHKLMOnly)
{m_hKey = NULL; m_bHKLMOnly = bHKLMOnly;}

inline CRegKey::CRegKey()
{
    TransAssert(g_bInit);
    m_bHKLMOnly = g_bUseHKLMOnly;
    m_hKey = NULL;
}

inline CRegKey::~CRegKey()
{Close();}

inline CRegKey::operator HUSKEY() const
{return m_hKey;}

inline LONG CRegKey::SetValue(DWORD dwValue, LPCTSTR lpszValueName)
{
    TransAssert(m_hKey != NULL);

    return SHRegWriteUSValue(m_hKey, lpszValueName, REG_DWORD,
        (LPVOID)&dwValue, sizeof(DWORD), RegSetFlags());
}

inline LONG CRegKey::SetValue(LPCTSTR lpszValue, LPCTSTR lpszValueName)
{
    TransAssert(lpszValue != NULL);
    TransAssert(m_hKey != NULL);

    return SHRegWriteUSValue(m_hKey, lpszValueName, REG_SZ,
        (LPVOID)lpszValue, (lstrlen(lpszValue)+1)*sizeof(TCHAR), RegSetFlags());
}

inline LONG CRegKey::SetBinaryValue(const BYTE *pb, LPCTSTR lpszValueName, DWORD dwCount)
{
    TransAssert(pb != NULL);
    TransAssert(m_hKey != NULL);
    return SHRegWriteUSValue(m_hKey, lpszValueName, REG_BINARY,
        (LPVOID)pb, dwCount, RegSetFlags());
}

inline LONG CRegKey::SetValueOfType(const BYTE *pb, LPCTSTR lpszValueName, DWORD dwCount, DWORD dwType)
{
    TransAssert(pb != NULL);
    TransAssert(m_hKey != NULL);

    return SHRegWriteUSValue(m_hKey, lpszValueName, dwType,
        (LPVOID)pb, dwCount, RegSetFlags());
}

inline LONG CRegKey::EnumKey(DWORD dwIndex, LPTSTR lpszName, LPDWORD pcchName)
{
    TransAssert(pcchName != NULL);

    return SHRegEnumUSKey(m_hKey, dwIndex, lpszName, pcchName, RegEnumFlags());
}

inline LONG CRegKey::EnumValue(DWORD dwIndex, LPTSTR pszValueName, LPDWORD pcchValueNameLen, 
                                  LPDWORD pdwType, LPVOID pvData, LPDWORD pcbData)
{
    // If these counts are all NULL, you will not get anything useful back.
    TransAssert(pcchValueNameLen != NULL || pdwType != NULL || pcbData != NULL);

    return SHRegEnumUSValue(m_hKey, dwIndex, pszValueName, pcchValueNameLen, pdwType,
                                pvData, pcbData, RegEnumFlags());
}

inline LONG CRegKey::QuerySubKeyInfo(DWORD *pdwNumKeys, DWORD *pdwMaxLen, DWORD *pdwNumValues)
{
    return SHRegQueryInfoUSKey (m_hKey, pdwNumKeys, pdwMaxLen,
        pdwNumValues, NULL, RegEnumFlags());
}
       
inline HUSKEY CRegKey::Detach()
{
    HUSKEY hKey = m_hKey;
    m_hKey = NULL;
    return hKey;
}

inline void CRegKey::Attach(HUSKEY hKey)
{
    TransAssert(m_hKey == NULL);
    m_hKey = hKey;
}

inline LONG CRegKey::DeleteValue(LPCTSTR lpszValue)
{
    TransAssert(m_hKey != NULL);
    return SHRegDeleteUSValue(m_hKey, lpszValue, RegDelFlags());
}

inline LONG CRegKey::DeleteEmptyKey(LPCTSTR pszSubKey)
{
    TransAssert(m_hKey != NULL);
    return SHRegDeleteEmptyUSKey(m_hKey, pszSubKey, RegDelFlags());
}

// Simple helper class to get an exclusive lock for the duration of a function. 
// WILL NOT BLOCK IF THE HANDLE PASSED IS NULL OR INVALID.

class CExclusiveLock 
{
public:
    CExclusiveLock(HANDLE hMutex);  // pass in a handle to a mutex.
    ~CExclusiveLock();
private:
    HANDLE m_hMutex;
    BOOL fOk;
};

inline CExclusiveLock::CExclusiveLock( HANDLE hMutex )
{
    fOk = FALSE;
    if ( hMutex )
    {
        m_hMutex = hMutex;
        DWORD dw = WaitForSingleObject(hMutex, INFINITE);
        if ( dw == WAIT_OBJECT_0)
            fOk = TRUE;
    }
}

inline CExclusiveLock::~CExclusiveLock( )
{
    if ( fOk )
    {
        ReleaseMutex(m_hMutex);
    }
}
                

// Helper class to create a shared memory object to share between processes. 

class CSharedMem
{
public:
    CSharedMem() { m_hFileMapping = NULL; m_lpVoidShared = NULL ; m_dwSize = 0; };
    ~CSharedMem( ) { Release( ); }
    BOOL Init(LPCSTR pszNamePrefix, DWORD dwSize);
    VOID Release( );
    // Offset into the shared memory section.
    // Always check return value since this can return NULL.
    LPVOID GetPtr (DWORD dwOffset); 

private:
    HANDLE m_hFileMapping ; 
    LPVOID m_lpVoidShared; 
    DWORD m_dwSize;
};

extern CSharedMem g_SharedMem;

// Shared memory related constants
#define SM_REGZONECHANGE_COUNTER    0   // Dword at offset 0
#define SM_SECMGRCHANGE_COUNTER     4   // Dword at offset 4

#define SM_SECTION_SIZE             8   // Total size of shared memory section.
#define SM_SECTION_NAME             "UrlZonesSM_"

// registry key paths - absolute
#define SZROOT          TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\")
#define SZZONES         SZROOT TEXT("Zones\\")
#define SZTEMPLATE      SZROOT TEXT("TemplatePolicies\\")
#define SZZONEMAP       SZROOT TEXT("ZoneMap\\")
#define SZCACHE         SZROOT TEXT("Cache")

#define SZPOLICIES      TEXT("Software\\Policies\\Microsoft\\Windows\\CurrentVersion\\Internet Settings")
#define SZHKLMONLY      TEXT("Security_HKLM_only")

// Entries to figure out if per user cache is allowed.
#define SZLOGON         TEXT("Network\\Logon")
#define SZUSERPROFILES  TEXT("UserProfiles")

// Cache location if per user cache is allowed.
#define SZSHELLFOLDER   TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders")
#define SZTIFS          TEXT("Cache")

// cache location if global
#define SZCACHECONTENT  SZROOT TEXT("Cache\\Content")
#define SZCACHEPATH     TEXT("CachePath")


// registry key paths - relative to SZZONES
#define SZZONESTANDARD      TEXT("Standard\\")
#define SZZONEUSERDEFINED   TEXT("User-Defined\\")

// registry key paths - relative to SZZONEMAP
#define SZDOMAINS           TEXT("Domains\\")
#define SZRANGES            TEXT("Ranges\\")
#define SZPROTOCOLS         TEXT("ProtocolDefaults\\")

// registry value names
#define SZINTRANETNAME      TEXT("IntranetName")
#define SZUNCASINTRANET     TEXT("UNCAsIntranet")
#define SZPROXYBYPASS       TEXT("ProxyBypass")
#define SZRANGE             TEXT(":Range")
#define SZRANGEPREFIX       TEXT("Range")

// Attributes to deal with "High", "Med", "Low" template policies
#define SZMINLEVEL              __TEXT("MinLevel")
#define SZRECLEVEL              __TEXT("RecommendedLevel")
#define SZCURRLEVEL             __TEXT("CurrentLevel")

// registry key path for allowed activex controls list; relative to HKEY_LOCAL_MACHINE _or_ HKEY_CURRENT_USER
#define ALLOWED_CONTROLS_KEY  TEXT("SOFTWARE\\Policies\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\AllowedControls")


#define CSTRLENW(str)       (sizeof(str)/sizeof(TCHAR) - 1)

#define MARK_PREFIX_SIZE                30
#define MARK_SUFFIX_SIZE                10
#define EXTRA_BUFFER_SIZE               1024
#define UNICODE_HEADER_SIZE             2

#endif // _URLZONE_UTIL_H_
 

