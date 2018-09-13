//
// Copyright (c) 1996  Microsoft Corporation
//
// Module Name: Url History Interfaces
//
// Author:
//    Zeke Lucas (zekel)  10-April-96
//

// !!! Take NOTE: CUrlHistory *MUST* be thread safe! DANGER, WILL ROBINSON, DANGER!

#include "priv.h"
#include "sccls.h"
#include "ishcut.h"
#include <inetreg.h>
#include "iface.h"

#define DM_UHRETRIEVE   0
#define DM_URLCLEANUP   0
#define DM_HISTGENERATE 0
#define DM_HISTPROP     0
#define DM_HISTEXTRA    0
#define DM_HISTCOMMIT   0
#define DM_HISTSPLAT    0
#define DM_HISTMISS     0
#define DM_HISTNLS      0

#define DW_FOREVERLOW (0xFFFFFFFF)
#define DW_FOREVERHIGH (0x7FFFFFFF)

#ifdef UNICODE
    #define VT_LPTSTR    VT_LPWSTR
#else
    #define VT_LPTSTR    VT_LPSTR
#endif

inline UINT DW_ALIGNED(UINT i) {
    return ((i+3) & 0xfffffffc);
}

inline BOOL IS_DW_ALIGNED(UINT i) {
    return ((i & 3)==0);
}

// Old one (beta-2)
typedef struct _HISTDATAOLD
{
    WORD cbSize;
    DWORD dwFlags;
    WORD wTitleOffset;
    WORD aFragsOffset;
    WORD cFrags;            //right now the top five bits are used for Prop_MshtmlMCS
    WORD wPropNameOffset;    
    WORD wMCSIndex;
} HISTDATAOLD, *LPHISTDATAOLD;

// Forward reference
typedef struct HISTEXTRA* LPHISTEXTRA;

// Version 0.01
//
// PID_INTSITE_WHATSNEW         stored as a HISTEXTRA
// PID_INTSITE_AUTHOR           stored as a HISTEXTRA
// PID_INTSITE_LASTVISIT        from lpCEI->LastAccessTime
// PID_INTSITE_LASTMOD          from lpCEI->LastModifiedTime
// PID_INTSITE_VISITCOUNT       dwVisits
// PID_INTSITE_DESCRIPTION      stored as a HISTEXTRA
// PID_INTSITE_COMMENT          stored as a HISTEXTRA
// PID_INTSITE_FLAGS            dwFlags
// PID_INTSITE_CONTENTLEN       (never used)
// PID_INTSITE_CONTENTCODE      (never used)
// PID_INTSITE_RECURSE          (never used)
// PID_INTSITE_WATCH            dwWatch
// PID_INTSITE_SUBSCRIPTION     stored as a HISTEXTRA
// PID_INTSITE_URL              URL itself
// PID_INTSITE_TITLE            Title
// PID_INTSITE_FRAGMENT         Visited Fragment (private)
//

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// HACKHACK:  If you change this data structure, you must talk
//            to Adrian Canter (adrianc) -- we put a copy of it
//            in wininet\urlcache\401imprt.cxx to make importing
//            from old-style cache happen quick n' dirty
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
struct _HISTDATA_V001
{
    UINT  cbSize : 16;           // size of this header
    UINT  cbVer  : 16;           // version
    DWORD           dwFlags;    // PID_INTSITE_FLAGS (PIDISF_ flags)
    DWORD           dwWatch;    // PID_INTSITE_WATCH (PIDISM_ flags)
    DWORD           dwVisits;   // PID_INTSITE_VISITCOUNT
};

#define HISTDATA_VER    2

class CHistoryData : public _HISTDATA_V001
{
public:
    LPHISTEXTRA _GetExtra(void)  const {
        return (LPHISTEXTRA)(((BYTE*)this) + this->cbSize);
    } 

    const HISTEXTRA * _FindExtra(UINT idExtra) const;
    HISTEXTRA * _FindExtraForSave(UINT idExtra) {
        return (HISTEXTRA*)_FindExtra(idExtra);
    }
    void _GetTitle(LPTSTR szTitle, UINT cchMax) const;
    BOOL _HasFragment(LPCTSTR pszFragment) const;
    BOOL _IsOldHistory(void) const {
        return (cbSize==SIZEOF(HISTDATAOLD) && cbVer==0);
    };

    static CHistoryData* s_GetHistoryData(LPINTERNET_CACHE_ENTRY_INFO lpCEI);
    static CHistoryData* s_AllocateHeaderInfo(UINT cbExtra, const CHistoryData* phdPrev, ULONG* pcbTotal);

    HISTEXTRA* CopyExtra(HISTEXTRA* phextCur) const;
    UINT GetTotalExtraSize() const;
};


//
//  Right after HISTDATA (always at cbSize), we have optional (typically
// variable length) data which has following data structure. It may have
// more than one but always has a null-terimiator (cbExtra == 0).
//
struct HISTEXTRA
{
    UINT cbExtra : 16;
    UINT idExtra : 8;   // PID_INTSITE_*
    UINT vtExtra : 8;   // VT_*
    BYTE abExtra[1];    // abExtra[cbExtra-4];

    BOOL IsTerminator(void) const {
        return (this->cbExtra==0);
    }

    const HISTEXTRA* GetNextFast(void) const {
        return (LPHISTEXTRA)(((BYTE*)this) + this->cbExtra);
    }

    HISTEXTRA* GetNextFastForSave(void) const {
        return (LPHISTEXTRA)(((BYTE*)this) + this->cbExtra);
    }

    const HISTEXTRA* GetNext(void) const {
        if (this->cbExtra) {
            return (LPHISTEXTRA)(((BYTE*)this) + this->cbExtra);
        }
        return NULL;
    }
};


// We want to make sure that our history binary data is valid so
// we don't crash or something
BOOL ValidateHistoryData(LPINTERNET_CACHE_ENTRY_INFOA pcei)
{
    DWORD cb = 0;

    if (!pcei->lpHeaderInfo)
    {
        ASSERT(pcei->dwHeaderInfoSize==0);
        pcei->dwHeaderInfoSize = 0;
        return TRUE;
    }
    
    // First, let's check HISTDATA
    CHistoryData* phd = (CHistoryData*)pcei->lpHeaderInfo;
    if ((phd->cbSize!=sizeof(_HISTDATA_V001))
        ||
        (phd->cbSize > pcei->dwHeaderInfoSize))
    {
#ifdef DEBUG
        ASSERT(FALSE && "History data corruption detected. Contact AKABIR.");
#else
        // BUG BUG MUST REMOVE THIS FOR RETAIL RTM

        // REACTIVATE AFTER TECHBETA
//        WCHAR wch = *((PWSTR)NULL);
#endif
        pcei->dwHeaderInfoSize = 0;
        pcei->lpHeaderInfo = NULL;
        return FALSE;
    }

    cb += phd->cbSize;
    
    // Now, let's check HISTEXTRA
    LPHISTEXTRA phe = phd->_GetExtra();
    while (!phe->IsTerminator())
    {
        cb += phe->cbExtra;
        if (cb >= pcei->dwHeaderInfoSize)
        {
#ifdef DEBUG
            ASSERT(FALSE && "History data corruption detected. Contact AKABIR.");
#else
            // BUG BUG MUST REMOVE THIS FOR RTM

        // REACTIVATE AFTER TECHBETA
//            WCHAR wch = *((PWSTR)NULL);
#endif
            // Hmm. We're expecting more data than we got. Not good. Prune the rest off.
            // We're adding 1 for the terminator
            pcei->dwHeaderInfoSize = cb - phe->cbExtra + 4;
            phe->cbExtra = 0;
            return FALSE;
        }
        phe = phe->GetNextFastForSave();
    }

    // Add a DWORD for the terminator
    cb += sizeof(DWORD);
    ASSERT(pcei->dwHeaderInfoSize==cb);
    return TRUE;    
}

//
//  Typically, we need 200-300 bytes to retrieve a cached entry in this
// history database. To avoid allocating memory in 99% of cases, we
// allocate 500 bytes in the stack and call LocalAlloc only if we need
// more than that. 
//
#define DEFAULT_CEI_BUFFER_SIZE		(500 * sizeof(WCHAR))

const TCHAR c_szHistoryPrefix[] = TEXT("Visited: ");

struct CEI_PREALLOC {
    LPINTERNET_CACHE_ENTRY_INFO pcei;

    LPCTSTR pszFragment;
    TCHAR szPrefixedUrl[MAX_URL_STRING + ARRAYSIZE(c_szHistoryPrefix)];

    union {
#ifdef UNIX
        double alignOn8ByteBoundary;
#endif /* UNIX */
        INTERNET_CACHE_ENTRY_INFO cei;
        BYTE ab[DEFAULT_CEI_BUFFER_SIZE];
    };

    CEI_PREALLOC() : pcei(NULL), pszFragment(NULL) {}
    ~CEI_PREALLOC() {
        if (pcei && pcei != &cei) {
            TraceMsg(DM_TRACE, "CEI_PREALLOC::dtr freeing pcei");
            LocalFree(pcei);
        }
    }
};

#define VER_HISTDATA    1

typedef CHistoryData HISTDATA;

typedef HISTDATA* LPHISTDATA;

//  CUrlHistory manages the other interfaces and handles alot of the basic functions
class   CUrlHistory : public IUrlHistoryPriv
{
public:
    CUrlHistory (void);
    ~CUrlHistory(void);

    // IUnknown methods

    virtual STDMETHODIMP  QueryInterface(REFIID riid, PVOID *ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // IUrlHistoryStg methods
    STDMETHODIMP AddUrl(LPCWSTR pwszUrl, LPCWSTR pwszTitle, DWORD dwFlags);
    STDMETHODIMP DeleteUrl(LPCWSTR pwszUrl, DWORD dwFlags);
    STDMETHODIMP QueryUrl(LPCWSTR pwszUrl, DWORD dwFlags, LPSTATURL lpSTATURL);
    STDMETHODIMP BindToObject(LPCWSTR pwszUrl, REFIID riid, void **ppvOut);
    STDMETHODIMP EnumUrls(IEnumSTATURL **ppEnum);

    // IUrlHistoryStg2 methods
    STDMETHODIMP AddUrlAndNotify(LPCWSTR pwszUrl, LPCWSTR pwszTitle, DWORD dwFlags, BOOL fWriteHistory, IOleCommandTarget *poctNotify, IUnknown *punkSFHistory);
    STDMETHODIMP ClearHistory();

    // IUrlHistoryPriv methods
    STDMETHOD(QueryUrlA)(LPCSTR pszUrl, DWORD dwFlags, LPSTATURL lpSTATURL);
    STDMETHOD(CleanupHistory)(void);
    STDMETHOD_(DWORD,GetDaysToKeep)(void) { return s_GetDaysToKeep(); }
    STDMETHOD(GetProperty)(LPCTSTR pszUrl, PROPID pid, PROPVARIANT* pvarOut);
    STDMETHOD(GetUserName)(LPTSTR pszUserName, DWORD cchUserName);
    STDMETHOD(AddUrlAndNotifyCP)(LPCWSTR pwszUrl, LPCWSTR pwszTitle, DWORD dwFlags, BOOL fWriteHistory, IOleCommandTarget *poctNotify, IUnknown *punkSFHistory, UINT* pcodepage);

   
    static void  s_Init();
    static DWORD   s_GetDaysToKeep(void);
#if defined(ux10) && defined(UNIX)
    static void s_SetDaysToKeep(DWORD dwDays);
#endif


protected:

    void _WriteToHistory(LPCTSTR pszPrefixedurl,
                         FILETIME& ftExpires,
                         IOleCommandTarget *poctNotify,
                         IUnknown *punkSFHistory);

    friend class CEnumSTATURL;
    // friend class CUrlHObj;
    friend class IntsiteProp;

    static HRESULT s_CleanupHistory(void);
    static HRESULT s_EnumUrls(IEnumSTATURL **ppEnum);
    static HRESULT s_DeleteUrl(LPCWSTR pwszUrl, DWORD dwFlags);
    
    static void s_ConvertToPrefixedUrlW(  
                                IN LPCWSTR pwszUrl,
                                OUT LPTSTR pszPrefixedUrl,
                                IN DWORD cchPrefixedUrl, 
                                OUT LPCTSTR *ppszFragment
                              );

    static HRESULT s_QueryUrlCommon(
                          LPCTSTR lpszPrefixedUrl,
                          LPCTSTR lpszFragment,
                          DWORD dwFlags,
                          LPSTATURL lpSTATURL
                          );

    static void s_RetrievePrefixedUrlInfo(
                LPCTSTR lpszUrl, CEI_PREALLOC* pbuf);
    static BOOL s_CommitUrlCacheEntry(LPCTSTR pszPrefixedUrl, 
                        LPINTERNET_CACHE_ENTRY_INFO pcei);

    static BOOL s_IsCached(IN LPCTSTR pszUrl)
        { return ::GetUrlCacheEntryInfoEx(pszUrl, NULL, NULL, NULL, NULL, NULL, INTERNET_CACHE_FLAG_ALLOW_COLLISIONS); }

    static HISTDATA* s_GenerateHeaderInfo(
            IN LPCTSTR pszTitle, 
            IN HISTDATA* phdPrev,
            IN LPCTSTR pszFragment,
            OUT LPDWORD pcbHeader
            );

    static HRESULT s_GenerateSTATURL(IN LPINTERNET_CACHE_ENTRY_INFO lpCEI, IN DWORD dwFlags, OUT LPSTATURL lpsu);
    static void s_UpdateIcon(Intshcut* pintshcut, DWORD dwFlags);

    DWORD   _cRef;
    static TCHAR   s_szUserPrefix[INTERNET_MAX_USER_NAME_LENGTH + 1];
    static DWORD   s_cchUserPrefix ;
    static DWORD   s_dwDaysToKeep;
    
};


class CEnumSTATURL      : public IEnumSTATURL
{
public:

    CEnumSTATURL() : _cRef(1) {}
    ~CEnumSTATURL();

    // IUnknown methods

    STDMETHODIMP  QueryInterface(REFIID riid, PVOID *ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //  IEnumXXXX methods

    STDMETHODIMP Next (ULONG celt, LPSTATURL rgelt, ULONG * pceltFetched) ;
    STDMETHODIMP Skip(ULONG celt) ;
    STDMETHODIMP Reset(void) ;
    STDMETHODIMP Clone(IEnumSTATURL ** ppenum) ;

    //  IEnumSTATURL methods

    STDMETHODIMP SetFilter(LPCWSTR poszFilter, DWORD dwFlags) ;

private:

    HRESULT RetrieveFirstUrlInfo(void);
    HRESULT RetrieveNextUrlInfo(void);

    DWORD _cRef;

    //  search object parameters
    LPWSTR m_poszFilter;
    DWORD  m_dwFilter;

    HANDLE m_hEnum;
    TCHAR _szPrefixedUrl[MAX_URL_STRING];
    DWORD m_cchPrefixedUrl;
    LPCTSTR m_lpszFragment;
    LPINTERNET_CACHE_ENTRY_INFO m_lpCEI;
    DWORD m_cbCEI;

};


#if defined(ux10) && defined(UNIX)
//Work around for mmap limitation in hp-ux10. Change the corresponding
//value even in inetcpl/general.cpp
#define MAX_HISTORY_DAYS        30
#endif

#define FILETIME_SEC				10000000
#define SECS_PER_DAY				(60 * 60 * 24)

#define CCHHISTORYPREFIX (ARRAYSIZE(c_szHistoryPrefix) - 1)
#define CLEANUP_HISTORY_INTERVAL (24 * 60 * 60 * 1000) // One day, in milliseconds

DWORD g_tCleanupHistory = 0;


#define OFFSET_TO_LPTSTR(p, o)		( (LPTSTR) ( (LPBYTE) (p) + (o) ) )
#define OFFSET_TO_LPBYTE(p, o)		( (LPBYTE) ( (LPBYTE) (p) + (o) ) )
#define OFFSET_TO_LPWORD(p, o)		( (LPWORD) ( (LPBYTE) (p) + (o) ) )

#define LPTSTR_TO_OFFSET(p, s)		( (WORD) ( (LPTSTR) (s) - (LPTSTR) (p) ) )
#define LPBYTE_TO_OFFSET(p, b)		( (WORD) ( (LPBYTE) (b) - (LPBYTE) (p) ) )

// NOTE: BUGBUG chrisfra 3/26/97 , ext\cachevu\priv.h has a duplicate copy of this
// structure and uses it to access cache.  this needs to be covered by procedural or
// object interface and moved to a common location.

// This structure uses the flags bits as follows: if HFL_VERSIONED is true, then
// the rest of the flags word is the version.

#define HFL_VERSIONED (0x80000000)


//
// We store binary data in the lpHeaderInfo field.  CommitUrlCacheEntryW tries
// to convert this data to ansi and messes it up.  To get around this we thunk
// through to the A version CommitUrlCacheEntry.
//

BOOL
CommitUrlCacheEntryBinary(
    IN LPCWSTR  lpszUrlName,
    IN FILETIME ExpireTime,
    IN FILETIME LastModifiedTime,
    IN DWORD CacheEntryType,
    IN LPBYTE lpHeaderInfo,
    IN DWORD dwHeaderSize
)
{
    ASSERT(lpszUrlName);

    CHAR szUrl[MAX_URL_STRING + ARRAYSIZE(c_szHistoryPrefix)];

    SHUnicodeToAnsi(lpszUrlName, szUrl, ARRAYSIZE(szUrl));

    INTERNET_CACHE_ENTRY_INFOA cei;
    cei.lpHeaderInfo = (LPSTR)lpHeaderInfo;
    cei.dwHeaderInfoSize = dwHeaderSize;
    ValidateHistoryData(&cei);

    return CommitUrlCacheEntryA(szUrl, NULL, ExpireTime, LastModifiedTime,
                                CacheEntryType, lpHeaderInfo, dwHeaderSize,
                                NULL, NULL);
}

GetUrlCacheEntryInfoBinary(
    IN LPCWSTR lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize
    )
{
    ASSERT(lpszUrlName);

    BOOL fRet;

    CHAR szUrl[MAX_URL_STRING + ARRAYSIZE(c_szHistoryPrefix)];

    SHUnicodeToAnsi(lpszUrlName, szUrl, ARRAYSIZE(szUrl));

    //
    // Warning!  This doesn't convert any of thge string parameters in
    // lpCacheEntryInfo back to unicode.  History only uses
    // lpCacheEntryInfo->lpHeaderInfo so this isn't a problem.
    //

    fRet =  GetUrlCacheEntryInfoA(szUrl,
                                 (LPINTERNET_CACHE_ENTRY_INFOA)lpCacheEntryInfo,
                                 lpdwCacheEntryInfoBufferSize);

    //
    // Set unused out paramters to NULL incase someone tries to use them
    //

    lpCacheEntryInfo->lpszSourceUrlName = NULL;
    lpCacheEntryInfo->lpszLocalFileName = NULL;
    lpCacheEntryInfo->lpszFileExtension = NULL;

    if (fRet)
    {
        ValidateHistoryData((LPINTERNET_CACHE_ENTRY_INFOA)lpCacheEntryInfo);
    }
    return fRet;
}

//
// Warning!  This function converts cei structures for use by history.  It is
// not a generic conversion.  It converts the minimum data required by history.
//
int
CacheEntryInfoAToCacheEntryInfoW(
    LPINTERNET_CACHE_ENTRY_INFOA pceiA,
    LPINTERNET_CACHE_ENTRY_INFOW pceiW,
    int cbceiW
    )
{
    int nRet;

    ASSERT(pceiA->lpszSourceUrlName);
    int cchSourceUrlName = lstrlenA(pceiA->lpszSourceUrlName) + 1;

    int cbRequired = sizeof(INTERNET_CACHE_ENTRY_INFOA) +
                     pceiA->dwHeaderInfoSize + 
                     cchSourceUrlName * sizeof(WCHAR);

    if (cbRequired <= cbceiW)
    {
        ASSERT(sizeof(*pceiA) == sizeof(*pceiW));

        // Copy the structure.
        *pceiW = *(INTERNET_CACHE_ENTRY_INFOW*)pceiA;

        // Append the binary data.  Note dwHeaderInfoSize is already copied.
        pceiW->lpHeaderInfo = (LPWSTR)(pceiW + 1);
        memcpy(pceiW->lpHeaderInfo, pceiA->lpHeaderInfo, pceiA->dwHeaderInfoSize);

        // Append the source url name.
        pceiW->lpszSourceUrlName = (LPWSTR)((BYTE*)(pceiW + 1) + pceiW->dwHeaderInfoSize);
        SHAnsiToUnicode(pceiA->lpszSourceUrlName, pceiW->lpszSourceUrlName,
                        cchSourceUrlName);

        // Null out bogus pointers so we'll fault if someone deref's them
        pceiW->lpszLocalFileName = NULL;
        pceiW->lpszFileExtension = NULL;

        nRet = 0;
    }
    else
    {
        nRet = cbRequired;
    }

    return nRet;
}

HANDLE
FindFirstUrlCacheEntryBinary(
    IN LPCWSTR lpszUrlSearchPattern,
    OUT LPINTERNET_CACHE_ENTRY_INFOW lpFirstCacheEntryInfo,
    IN OUT LPDWORD lpdwFirstCacheEntryInfoBufferSize
    )
{
    ASSERT(NULL != lpszUrlSearchPattern);
    ASSERT(NULL != lpFirstCacheEntryInfo);
    ASSERT(NULL != lpdwFirstCacheEntryInfoBufferSize);

    HANDLE hRet;

    CHAR szPattern[MAX_PATH];

    ASSERT(lstrlenW(lpszUrlSearchPattern) < ARRAYSIZE(szPattern));
    SHUnicodeToAnsi(lpszUrlSearchPattern, szPattern, ARRAYSIZE(szPattern));

    BYTE ab[MAX_CACHE_ENTRY_INFO_SIZE];
    INTERNET_CACHE_ENTRY_INFOA* pceiA = (INTERNET_CACHE_ENTRY_INFOA*)ab;
    DWORD dwSize;
    BOOL fAllocated = FALSE;

    pceiA->dwStructSize = dwSize = sizeof(ab);

    hRet = FindFirstUrlCacheEntryA(szPattern, pceiA, &dwSize);

    if (NULL == hRet && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        pceiA = (INTERNET_CACHE_ENTRY_INFOA*)LocalAlloc(LPTR, dwSize);

        if (pceiA)
        {
            fAllocated = TRUE;

            pceiA->dwStructSize = dwSize;

            hRet = FindFirstUrlCacheEntryA(szPattern, pceiA, &dwSize);
            
            ASSERT(hRet || GetLastError() != ERROR_INSUFFICIENT_BUFFER);
        }
        else
        {
            SetLastError(ERROR_OUTOFMEMORY);
        }
    }
    
    if (hRet)
    {
        int nRet;

        ValidateHistoryData(pceiA);
        nRet = CacheEntryInfoAToCacheEntryInfoW(pceiA, lpFirstCacheEntryInfo,
                                                *lpdwFirstCacheEntryInfoBufferSize);

        if (nRet)
        {
            FindCloseUrlCache(hRet);
            hRet = NULL;
            *lpdwFirstCacheEntryInfoBufferSize = nRet;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        }
    }

    if (fAllocated)
        LocalFree(pceiA);

    return hRet;
}

BOOL
FindNextUrlCacheEntryBinary(
    IN HANDLE hEnumHandle,
    OUT LPINTERNET_CACHE_ENTRY_INFOW lpNextCacheEntryInfo,
    IN OUT LPDWORD lpdwNextCacheEntryInfoBufferSize
    )
{
    ASSERT(NULL != hEnumHandle);
    ASSERT(NULL != lpNextCacheEntryInfo);
    ASSERT(NULL != lpdwNextCacheEntryInfoBufferSize);

    BOOL fRet;

    BYTE ab[MAX_CACHE_ENTRY_INFO_SIZE];
    INTERNET_CACHE_ENTRY_INFOA* pceiA = (INTERNET_CACHE_ENTRY_INFOA*)ab;
    DWORD dwSize;
    BOOL fAllocated = FALSE;

    pceiA->dwStructSize = dwSize = sizeof(ab);

    fRet = FindNextUrlCacheEntryA(hEnumHandle, pceiA, &dwSize);
    
    if (!fRet && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        pceiA = (INTERNET_CACHE_ENTRY_INFOA*)LocalAlloc(LPTR, dwSize);

        if (pceiA)
        {
            fAllocated = TRUE;

            pceiA->dwStructSize = dwSize;

            fRet = FindNextUrlCacheEntryA(hEnumHandle, pceiA, &dwSize);
            
            ASSERT(fRet || GetLastError() != ERROR_INSUFFICIENT_BUFFER);
        }
        else
        {
            SetLastError(ERROR_OUTOFMEMORY);
        }
    }

    if (fRet)
    {
        int nRet;

        ValidateHistoryData(pceiA);
        nRet = CacheEntryInfoAToCacheEntryInfoW(pceiA, lpNextCacheEntryInfo,
                                                *lpdwNextCacheEntryInfoBufferSize);

        if (nRet)
        {
            fRet = FALSE;
            *lpdwNextCacheEntryInfoBufferSize = nRet;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        }
    }

    if (fAllocated)
        LocalFree(pceiA);
  
    return fRet;
}

#define DEFAULT_DAYS_TO_KEEP    21
static const TCHAR c_szRegValDaysToKeep[] = TEXT("DaysToKeep");
static const TCHAR c_szRegValDirectory[] = TEXT("Directory");

#ifdef UNIX
#define DIR_SEPARATOR_CHAR  TEXT('/')
#else
#define DIR_SEPARATOR_CHAR  TEXT('\\')
#endif

#ifdef UNICODE
#define SHGETFOLDERPATH "SHGetFolderPathW"
#else
#define SHGETFOLDERPATH "SHGetFolderPathA"
#endif

#undef SHGetFolderPath
typedef HRESULT (*PFNSHGETFOLDERPATH)(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPTSTR pszPath);
HMODULE g_hmodShfolder = NULL;


HRESULT SHGetFolderPath(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPTSTR pszPath)
{
    if (!g_hmodShfolder)
    {
        // if we are on NT5 skip shfolder and go straight to shell32.dll
        g_hmodShfolder = LoadLibrary( IsOS( OS_NT5 )? TEXT("shell32.dll") : TEXT("shfolder.dll"));
    }

    HRESULT hr = E_FAIL;
    if (g_hmodShfolder) 
    {
        PFNSHGETFOLDERPATH pfn = (PFNSHGETFOLDERPATH)GetProcAddress(g_hmodShfolder, SHGETFOLDERPATH);
        if (pfn)
        {
            hr = pfn(hwnd, csidl, hToken, dwFlags, pszPath);
        }
    }
    return hr;
}

HRESULT SHGetHistoryPIDL(LPITEMIDLIST *ppidlHistory)
{
    *ppidlHistory = NULL;

    TCHAR szHistory[MAX_PATH];

    szHistory[0] = 0;

    HRESULT hres = SHGetFolderPath(NULL, CSIDL_HISTORY | CSIDL_FLAG_CREATE, NULL, 0, szHistory);
    if (hres != S_OK)
    {
        GetHistoryFolderPath(szHistory, ARRAYSIZE(szHistory));
        PathRemoveFileSpec(szHistory);  // get the trailing slash
        PathRemoveFileSpec(szHistory);  // trim the "content.ie5" junk
    }

    if (szHistory[0])
    {
        TCHAR szIniFile[MAX_PATH];
        PathCombine(szIniFile, szHistory, TEXT("desktop.ini"));

        if (GetFileAttributes(szIniFile) == -1)
        {
            DWORD dwAttrib = GetFileAttributes(szHistory);
            dwAttrib &= ~FILE_ATTRIBUTE_HIDDEN;
            dwAttrib |=  FILE_ATTRIBUTE_SYSTEM;

            // make sure system, but not hidden
            SetFileAttributes(szHistory, dwAttrib);

            WritePrivateProfileString(TEXT(".ShellClassInfo"), TEXT("ConfirmFileOp"), TEXT("0"), szIniFile);
            WritePrivateProfileString(TEXT(".ShellClassInfo"), TEXT("CLSID"), TEXT("{FF393560-C2A7-11CF-BFF4-444553540000}"), szIniFile);
        }

        IShellFolder *psfDesktop;
        hres = SHGetDesktopFolder(&psfDesktop);
        if (SUCCEEDED(hres)) 
        {
            hres = psfDesktop->ParseDisplayName(NULL, NULL, 
                                    szHistory, NULL, ppidlHistory, NULL); 
            psfDesktop->Release();
        }
    }
    else
        hres = E_FAIL;
    return hres;
}

//
// This function is called from hist/hsfolder.cpp
//
HRESULT CUrlHistory::GetUserName(LPTSTR pszUserName, DWORD cchUserName)
{
    s_Init();
    
    if (cchUserName < s_cchUserPrefix)
    {
        return E_FAIL;
    }
    CopyMemory(pszUserName, s_szUserPrefix, (s_cchUserPrefix-1) * sizeof(TCHAR));
    pszUserName[s_cchUserPrefix-1] = 0;
    return S_OK;
}

#if defined (ux10) && defined(UNIX)
void CUrlHistory::s_SetDaysToKeep(DWORD dwDays)
{
    HKEY hk;
    DWORD dwDisp;

    DWORD Error = RegCreateKeyEx(
                                 HKEY_CURRENT_USER,
                                 REGSTR_PATH_URLHISTORY,
                                 0, NULL, 0,
                                 KEY_WRITE,
                                 NULL,
                                 &hk,
                                 &dwDisp);

    if(ERROR_SUCCESS != Error)
    {
        ASSERT(FALSE);
        return;
    }

    Error = RegSetValueEx(
                          hk,
                          c_szRegValDaysToKeep,
                          0,
                          REG_DWORD,
                          (LPBYTE) &dwDays,
                          sizeof(dwDays));

    ASSERT(ERROR_SUCCESS == Error);

    RegCloseKey(hk);

    return;

}
#endif

//
// This function is called from hist/hsfolder.cpp
//
DWORD CUrlHistory::s_GetDaysToKeep(void)
{
    HKEY hk;
    DWORD cbDays = SIZEOF(DWORD);
    DWORD dwDays = DEFAULT_DAYS_TO_KEEP;
    DWORD dwType;


    DWORD Error = RegOpenKeyEx(
                               HKEY_CURRENT_USER,
                               REGSTR_PATH_URLHISTORY,
                               0,
                               KEY_READ,
                               &hk);


    if(Error)
    {
        Error = RegOpenKeyEx(
                             HKEY_LOCAL_MACHINE,
                             REGSTR_PATH_URLHISTORY,
                             0,
                             KEY_READ,
                             &hk);
    }


    if(!Error)
    {
        Error = RegQueryValueEx(
                                hk,
                                c_szRegValDaysToKeep,
                                0,
                                &dwType,
                                (LPBYTE) &dwDays,
                                &cbDays);

        RegCloseKey(hk);
    }

    return dwDays;
}

IUrlHistoryPriv* g_puhUrlHistory = NULL;

void CUrlHistory_CleanUp()
{
    // Release will clean up the global
    ENTERCRITICAL;
    if (g_puhUrlHistory)
        g_puhUrlHistory->Release();
    LEAVECRITICAL;
}

STDAPI CUrlHistory_CreateInstance(IUnknown* pUnkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    HRESULT hr = E_OUTOFMEMORY;

    *ppunk = NULL;

    // !!! Take NOTE: CUrlHistory *MUST* be thread safe!

    // aggregation checking is handled in class factory
    ENTERCRITICAL;
    
    if (!g_puhUrlHistory) 
    {
        CUrlHistory *pcuh = new CUrlHistory;
        if (pcuh) 
        {
            g_puhUrlHistory = SAFECAST(pcuh, IUrlHistoryPriv *);
#ifdef DEBUG
            // The memory tracking code thinks this psf will leak 
            remove_from_memlist(g_puhUrlHistory);
#endif
        }
    }

    if (g_puhUrlHistory)
    {
        *ppunk = SAFECAST(g_puhUrlHistory, IUnknown*);
        g_puhUrlHistory->AddRef();
        hr = S_OK;
    }

    LEAVECRITICAL;

    return hr;
}





//
//  Public members of CUrlHistory
//

CUrlHistory::CUrlHistory(void) : _cRef(1)
{
    //
    // Update s_dwDaysToKeep for each call
    //
    s_dwDaysToKeep = s_GetDaysToKeep();
    
#if defined(ux10) && defined(UNIX)
//Work around for mmap limitation in hp-ux10
    if (s_dwDaysToKeep > MAX_HISTORY_DAYS)
    {
       s_dwDaysToKeep = MAX_HISTORY_DAYS;
       s_SetDaysToKeep(s_dwDaysToKeep);
    }
#endif

#ifdef DEBUG
    if (g_dwPrototype & 0x00000020) {
        s_CleanupHistory();
    }
#endif

    DllAddRef();
}

CUrlHistory::~CUrlHistory(void)
{
    DllRelease();
}

HRESULT LoadHistoryShellFolder(IUnknown *punk, IHistSFPrivate **ppsfpHistory)
{
    HRESULT hr;

    *ppsfpHistory = NULL;
    if (punk)
    {
        hr = punk->QueryInterface(IID_IHistSFPrivate, (void **)ppsfpHistory);
    }
    else
    {
        LPITEMIDLIST pidlHistory;

        hr = SHGetHistoryPIDL(&pidlHistory);
        if (SUCCEEDED(hr))
        {
            hr = SHBindToObject(NULL, IID_IHistSFPrivate, pidlHistory, (void **)ppsfpHistory);
            ILFree(pidlHistory);
        }
    }
    return hr;
}

//  ClearHistory on a per user basis.  moved from inetcpl to facilitate changes in
//  implementation.
HRESULT CUrlHistory::ClearHistory()
{
    HRESULT hr;
    IEnumSTATURL *penum;
    IHistSFPrivate *psfpHistory = NULL;

    hr = THR(EnumUrls(&penum));

    if (SUCCEEDED(hr))
    {
        penum->SetFilter(NULL, STATURL_QUERYFLAG_NOTITLE);

        ULONG cFetched;
        STATURL rsu[1] = {{sizeof(STATURL), NULL, NULL}};
        while (SUCCEEDED(penum->Next(1, rsu, &cFetched)) && cFetched)
        {
            ASSERT(rsu[0].pwcsUrl);

            hr = THR(DeleteUrl(rsu[0].pwcsUrl, URLFLAG_DONT_DELETE_SUBSCRIBED));

            OleFree(rsu[0].pwcsUrl);
            rsu[0].pwcsUrl = NULL;

            ASSERT(!rsu[0].pwcsTitle);
        }
        penum->Release();
    }
    hr = LoadHistoryShellFolder(NULL, &psfpHistory);
    if (SUCCEEDED(hr))
    {
        hr = psfpHistory->ClearHistory();
        psfpHistory->Release();
    }
    return hr;
}

extern void _FileTimeDeltaDays(FILETIME *pftBase, FILETIME *pftNew, int Days);

HRESULT CUrlHistory::s_CleanupHistory(void)
{
    TraceMsg(DM_URLCLEANUP, "CUH::s_CleanupHistory called");

    HRESULT hr;
    DWORD tCurrent = GetTickCount();

    if (!g_tCleanupHistory || (tCurrent > g_tCleanupHistory + CLEANUP_HISTORY_INTERVAL)) {
        g_tCleanupHistory = tCurrent;
    } else {
#ifdef DEBUG
        if (!(g_dwPrototype & 0x00000020))
#endif
        return S_OK;
    }

    SYSTEMTIME st;
    FILETIME ftNow;
    FILETIME ftOldest;
    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ftNow);
    //  Note, due to fact we show whole weeks, we need to keep item around an extra
    //  7 days.  Note, we force checking current days to keep so that we aren't out
    //  of synch with history view
    _FileTimeDeltaDays(&ftNow, &ftOldest, -((int)s_GetDaysToKeep() + 7));

    IEnumSTATURL * penum = NULL;
    if (SUCCEEDED(s_EnumUrls(&penum)))
    {
        STATURL rsu[1] = {{sizeof(STATURL), NULL, NULL}};
        ULONG cFetched = 0;

        penum->SetFilter(NULL, STATURL_QUERYFLAG_NOTITLE);

        while (S_OK == penum->Next(1, rsu, &cFetched))
        {
            ASSERT(cFetched);
            ASSERT(rsu[0].pwcsUrl);
            ASSERT(rsu[0].pwcsTitle==NULL);

#ifdef DEBUG
            TCHAR szUrl[MAX_URL_STRING];
            SHUnicodeToTChar(rsu[0].pwcsUrl, szUrl, ARRAYSIZE(szUrl));
#endif
            // check to see if expires is not special && ftLastUpdated is earlier
            // than we need
            if (CompareFileTime(&(rsu[0].ftLastUpdated), &ftOldest) < 0 &&
                (rsu[0].ftExpires.dwLowDateTime != DW_FOREVERLOW ||
                 rsu[0].ftExpires.dwHighDateTime != DW_FOREVERHIGH))
            {
                hr = THR(s_DeleteUrl(rsu[0].pwcsUrl, 0));
#ifdef DEBUG
                TraceMsg(DM_URLCLEANUP, "CUH::s_Cleanup deleting %s", szUrl);
#endif
            } else {
#ifdef DEBUG
                TraceMsg(DM_URLCLEANUP, "CUH::s_Cleanup keeping  %s", szUrl);
#endif
            }

            CoTaskMemFree(rsu[0].pwcsUrl);
            rsu[0].pwcsUrl = NULL;
            cFetched = 0;
            
            ASSERT(!rsu[0].pwcsTitle);
        }

        penum->Release();
    }
    else 
        ASSERT(FALSE);

    TraceMsg(DM_URLCLEANUP, "CUH::s_CleanupHistory (expensive!) just called");
    return S_OK;
}

HRESULT CUrlHistory::CleanupHistory()
{
    return CUrlHistory::s_CleanupHistory();
}


TCHAR CUrlHistory::s_szUserPrefix[INTERNET_MAX_USER_NAME_LENGTH + 1] = TEXT("");
DWORD CUrlHistory::s_cchUserPrefix = 0;
DWORD CUrlHistory::s_dwDaysToKeep = 0;


void CUrlHistory::s_Init(void)
{
    // Cache the user name only once per process
    if (!s_cchUserPrefix)
    {
        ENTERCRITICAL;
        // Maybe it changed since entering the crit sec.
        // This really happened to me (BryanSt)
        // We do it twice for perf reasons.
        if (!s_cchUserPrefix)
        {
            ASSERT(s_szUserPrefix[0] == '\0');
            s_cchUserPrefix = ARRAYSIZE(s_szUserPrefix);

            //  Get the current user or set to default
            ::GetUserName(s_szUserPrefix, &s_cchUserPrefix);

            StrCatBuff(s_szUserPrefix, TEXT("@"), ARRAYSIZE(s_szUserPrefix));
            s_cchUserPrefix = lstrlen(s_szUserPrefix);
        }

        LEAVECRITICAL;
    }

}


HRESULT CUrlHistory::QueryInterface(REFIID riid, PVOID *ppvObj)
{
    HRESULT hr = E_NOINTERFACE;


    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown) || 
        IsEqualIID(riid, IID_IUrlHistoryStg2) ||
        IsEqualIID(riid, IID_IUrlHistoryPriv) ||
         IsEqualIID(riid, IID_IUrlHistoryStg))
    {
        AddRef();
        *ppvObj = (LPVOID) SAFECAST(this, IUrlHistoryPriv *);
        hr = S_OK;

    }
    else if (IsEqualIID(riid, CLSID_CUrlHistory))
    {
        AddRef();
        *ppvObj = (LPVOID) this;
        hr = S_OK;
    }
    return hr;
}


ULONG CUrlHistory::AddRef(void)
{
    _cRef++;

    return _cRef;
}

ULONG CUrlHistory::Release(void)
{
    ASSERT(_cRef > 0);

    _cRef--;

    if (!_cRef)
    {
        //time to go bye bye
        ENTERCRITICAL;
        g_puhUrlHistory = NULL;
        LEAVECRITICAL;
        delete this;
        return 0;
    }

    return _cRef;
}

//
// Converts a normal URL to a URL with the correct cache prefix.     
// it also finds a fragment (local Anchor) if it exists in the URL.  
//                                                                   
// if the URL is invalid, then the returned lplpszPrefixedUrl is just
// the prefix.  this is used primarily for doing enumeration.        
//
void CUrlHistory::s_ConvertToPrefixedUrlW(
                                       IN LPCWSTR pszUrl,
                                       OUT LPTSTR pszPrefixedUrl,
                                       IN DWORD cchPrefixedUrl,
                                       OUT LPCTSTR *ppszFragment
                                       )
{
    //
    // Make it sure that s_cchUserPrefix is initialized.
    //
    s_Init();

    //  Prefix + UserPrefix + '@'

    ASSERT(pszPrefixedUrl && ppszFragment);

    //  clear the out params
    pszPrefixedUrl[0] = L'\0';
    *ppszFragment = NULL;


    //  if there is no URL, send back a default case
    //  this is just for EnumObjects
    if (!pszUrl || !*pszUrl)
    {
        wnsprintf(pszPrefixedUrl, cchPrefixedUrl, L"%s%s", c_szHistoryPrefix, s_szUserPrefix);
    }
    else
    {
        int slen;
        int nScheme;
        LPWSTR pszFragment;

        wnsprintf(pszPrefixedUrl, cchPrefixedUrl, L"%s%s", c_szHistoryPrefix, s_szUserPrefix);
        slen = lstrlen(pszPrefixedUrl);
        StrCpyN(pszPrefixedUrl + slen, pszUrl, MAX_URL_STRING-slen);

        // Only strip the anchor fragment if it's not JAVASCRIPT: or VBSCRIPT:, because a # could not an
        // anchor but a string to be evaluated by a script engine like #00ff00 for an RGB color.
        nScheme = GetUrlSchemeW(pszPrefixedUrl);      
        if (nScheme == URL_SCHEME_JAVASCRIPT || nScheme == URL_SCHEME_VBSCRIPT)
        {
            pszFragment = NULL;
        }
        else
        {
            //  locate local anchor fragment if possible
            pszFragment = StrChr(pszPrefixedUrl, L'#');
        }

        if(pszFragment)     
        {
            //  kill the '#' so that lpszPrefixedUrl is isolated
            *pszFragment = L'\0';
            *ppszFragment = pszFragment+1;
        }

        //  check for trailing slash and eliminate
        LPWSTR pszT = CharPrev(pszPrefixedUrl, pszPrefixedUrl + lstrlen(pszPrefixedUrl));
        if (pszT[0] == L'/') {
            TraceMsg(DM_HISTNLS, "CUH::s_Convert removing the trailing slash of %s", pszPrefixedUrl);
            ASSERT(lstrlen(pszT)==1);
            pszT[0] = L'\0';
        }
    }
}


//
// Basically a wrapper function for RetreiveUrlCacheEntryInfo 
// meant to be called with the prefixed Url.                  
// it handles allocating the buffer and reallocating if necessary.
//
void CUrlHistory::s_RetrievePrefixedUrlInfo(
        LPCTSTR pszUrl, CEI_PREALLOC* pbuf)
{
    TraceMsg(DM_UHRETRIEVE, "CURLHistory::s_RetrievePrefixUrlInfo called (%s)", pszUrl);

    s_Init();

    DWORD cbCEI = SIZEOF(pbuf->ab);
    pbuf->pcei = &pbuf->cei;

    BOOL fSuccess = GetUrlCacheEntryInfoBinary(pszUrl, pbuf->pcei, &cbCEI);

    if (!fSuccess) {
        pbuf->pcei = NULL;
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            TraceMsg(DM_TRACE, "CUH::s_RetrievePUI not enough buffer. Allocate! (%d)", cbCEI);
            pbuf->pcei = (LPINTERNET_CACHE_ENTRY_INFO)LocalAlloc(LPTR, cbCEI);
            if (pbuf->pcei) {
                fSuccess = GetUrlCacheEntryInfoBinary(pszUrl, pbuf->pcei, &cbCEI);
                if (!fSuccess) {
                    TraceMsg(DM_HISTMISS, "CUH::s_Retrieve (%s) failed %x (on second attempt)",
                             pszUrl, GetLastError());
                    LocalFree(pbuf->pcei);
                    pbuf->pcei = NULL;
                    SetLastError(ERROR_FILE_NOT_FOUND);
                } 
            }
        } else {
            TraceMsg(DM_HISTMISS, "CUH::s_Retrieve (%s) failed %x (on first attempt)",
                     pszUrl, GetLastError());
            SetLastError(ERROR_FILE_NOT_FOUND);
        }
    }
}

//
// Return the total of a double-null terminated string (including the
// terminating null).
//
UINT lstrzlen(LPCTSTR pszz)
{
    for (LPCTSTR psz=pszz; *psz; psz += lstrlen(psz) + 1) ;
    return (unsigned int)(psz+1-pszz);
}


    /*++

     Routine Description:

     this creates a buffer that holds a HISTDATA, and everything the HISTDATAs offsets
     point to.  it only sets those offsets that are passed in.

     Arguments:

     lpszTitle		Title to place in the buffer

     lpBase			this is the base of the offsets in aFrags

     aFrags			an array of offsets to the fragments to place in the buffer

     cFrags			number of fragments in aFrags

     lpszNewFrag		this is an additional fragment to add in the new buffer

     pcbHeader			this is a pointer to the final size of the buffer returned

     NOTE: any of the arguments except pcbHeader may be NULL.
     if lpBase is NULL, then aFrags must also be NULL. this is CALLERs responsibility!
     if a parameter is NULL, then it just isnt added to the buffer.

     Return Value:

     POINTER
     Success - a valid pointer to a buffer that must be freed.

     Failure - NULL. this only fails with ERROR_NOT_ENOUGH_MEMORY

     NOTE:  Caller must free the returned pointer.  *pcbHeader only set upon successful return.

     --*/

HISTDATA* CUrlHistory::s_GenerateHeaderInfo(
                                  IN LPCTSTR pszTitle,
                                  IN HISTDATA* phdPrev,		
                                  IN LPCTSTR pszFragment,
                                  OUT LPDWORD pcbHeader
                                  )
{
    DWORD cbHeader = 0;
    UINT cbHistExtra = 0;
    HISTEXTRA* phextPrev;

    // Get the size for title
    UINT cchTitle = 0;
    if (pszTitle[0]) {
        cchTitle = lstrlen(pszTitle) + 1;
        cbHistExtra += DW_ALIGNED(SIZEOF(HISTEXTRA) + (cchTitle * sizeof(TCHAR)));

        if (phdPrev && (phextPrev = phdPrev->_FindExtraForSave(PID_INTSITE_TITLE))!=NULL) {
            phextPrev->vtExtra = VT_EMPTY;
        }
    }

    // Get the size of fragments
    UINT cchFragsPrev = 0;
    UINT cchFragment = 0;
    if (pszFragment) {
        cchFragment = lstrlen(pszFragment) + 2;  // Double NULL terminated
        if (phdPrev && (phextPrev=phdPrev->_FindExtraForSave(PID_INTSITE_FRAGMENT))!=NULL) {
            cchFragsPrev = lstrzlen((LPCTSTR)phextPrev->abExtra) - 1; // lstrzlen includes both terminating nulls
                                                                      // -1 since cchFragment already accounts
                                                                      // for double terminating NULLs.
            ASSERT(cchFragsPrev != (UINT)-1);
            phextPrev->vtExtra = VT_EMPTY;
        }
        cbHistExtra += DW_ALIGNED(SIZEOF(HISTEXTRA) + (cchFragsPrev + cchFragment) * sizeof(TCHAR));
    }

    // Get the size of other extra
    if (phdPrev) {
        cbHistExtra += phdPrev->GetTotalExtraSize();
    }

    // Allocate it
    CHistoryData* phdNew = CHistoryData::s_AllocateHeaderInfo(
                                cbHistExtra, phdPrev,
                                &cbHeader);

    if (phdNew) {
        HISTEXTRA* phext = phdNew->_GetExtra();

        // Append title
        if (pszTitle[0]) {
            phext->cbExtra = DW_ALIGNED((cchTitle * sizeof(TCHAR)) + SIZEOF(HISTEXTRA));
            phext->idExtra = PID_INTSITE_TITLE;
            phext->vtExtra = VT_LPTSTR; 
            StrCpyN((LPTSTR)phext->abExtra, pszTitle, cchTitle);
            phext = phext->GetNextFastForSave();
        }

        // Append fragment     
        if (pszFragment) {
            // Copy pszFragment to the top.
            StrCpyN((LPTSTR)phext->abExtra, pszFragment, cchFragment);
            // Double NULL terminate.  Note cchFragment = strlen + 2
            *(((LPTSTR)phext->abExtra) + cchFragment - 1) = TEXT('\0');

            // Copy existing fragments if any
            if (cchFragsPrev) {
                ASSERT(phdPrev);
                phextPrev = phdPrev->_FindExtraForSave(PID_INTSITE_FRAGMENT);
                ASSERT(phextPrev);
                if (phextPrev) {
                    ASSERT(IS_DW_ALIGNED(phextPrev->cbExtra));
                    memcpy(phext->abExtra + ((cchFragment - 1) * sizeof(TCHAR)), phextPrev->abExtra,
                           (cchFragsPrev + 1) * sizeof(TCHAR));
                }
            }

            ASSERT(lstrzlen((LPCTSTR)phext->abExtra) == cchFragsPrev + cchFragment);
            phext->cbExtra += DW_ALIGNED(SIZEOF(HISTEXTRA) + (cchFragsPrev + cchFragment) * sizeof(TCHAR));
            phext->idExtra = PID_INTSITE_FRAGMENT;
            phext->vtExtra = VT_NULL;    // HACK (means internal)
            phext = phext->GetNextFastForSave();
        }

        // Migrate extra data from previous one
        if (phdPrev) {
            phext = phdPrev->CopyExtra(phext);
        }

        ASSERT( phext->cbExtra == 0); // terminator
        ASSERT( (LPBYTE)phdNew+cbHeader == (LPBYTE)phext+SIZEOF(DWORD) );
        ASSERT( cbHistExtra == phdNew->GetTotalExtraSize() );
    }

    *pcbHeader = cbHeader;

    TraceMsg(DM_HISTGENERATE, "CUH::s_GenerateHeader allocated %d bytes (%d extra)",
             cbHeader, cbHistExtra);

    return phdNew;
}

// BUGBUG: Move this to UTIL.CPP
LPWSTR AllocOleStrFromTChar(LPCTSTR psz)
{
    DWORD cch = lstrlen(psz) + 1;
    LPWSTR pwsz = (LPWSTR)CoTaskMemAlloc(cch * SIZEOF(WCHAR));
    if (pwsz) {
        SHTCharToUnicode(psz, pwsz, cch);
    }
    return pwsz;
}

HRESULT CUrlHistory::s_GenerateSTATURL(
                               IN LPINTERNET_CACHE_ENTRY_INFO lpCEI,
                               IN DWORD dwFlags,
                               OUT LPSTATURL lpSTATURL)
{
    ASSERT(lpCEI);
    ASSERT(lpSTATURL);

    if (!lpCEI || !lpSTATURL)
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    LPHISTDATA phd =  CHistoryData::s_GetHistoryData(lpCEI);
    LPCTSTR pszUrl = lpCEI->lpszSourceUrlName + s_cchUserPrefix + CCHHISTORYPREFIX;

    ZeroMemory(lpSTATURL, SIZEOF(STATURL));

    lpSTATURL->ftLastUpdated = lpCEI->LastModifiedTime;
    lpSTATURL->ftExpires = lpCEI->ExpireTime;
    lpSTATURL->ftLastVisited = lpCEI->LastSyncTime;

    if(dwFlags & STATURL_QUERYFLAG_ISCACHED)
    {
        if (s_IsCached(pszUrl))
            lpSTATURL->dwFlags |= STATURLFLAG_ISCACHED;
    }

    if (dwFlags & STATURL_QUERYFLAG_TOPLEVEL)
    {
        if (phd) {
            if (phd->dwFlags & PIDISF_HISTORY)
            {
                lpSTATURL->dwFlags |= STATURLFLAG_ISTOPLEVEL;
            }
        }
    }

    if (!(dwFlags & STATFLAG_NONAME))
    {
        if (!(dwFlags & STATURL_QUERYFLAG_NOURL))
        {
            //  set the Url
            lpSTATURL->pwcsUrl = AllocOleStrFromTChar(pszUrl);
            if (lpSTATURL->pwcsUrl == NULL) {
                hr = E_OUTOFMEMORY;
            }
        }

        if (!(dwFlags & STATURL_QUERYFLAG_NOTITLE))
        {
            //  is there a title to set?
            if (phd)
            {
                const HISTEXTRA* phextTitle = phd->_FindExtra(PID_INTSITE_TITLE);

                if (phextTitle && phextTitle->vtExtra == VT_LPTSTR) {
                    lpSTATURL->pwcsTitle = AllocOleStrFromTChar((LPCTSTR)phextTitle->abExtra);
                    if (lpSTATURL->pwcsTitle == NULL) {
                        if (lpSTATURL->pwcsUrl)
                            CoTaskMemFree(lpSTATURL->pwcsUrl);
                        lpSTATURL->pwcsUrl = NULL;
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
        }
    }

    ASSERT(SUCCEEDED(hr) || (lpSTATURL->pwcsUrl==NULL && lpSTATURL->pwcsTitle==NULL));
    return hr;
}

    /*++

     Routine Description:

     Places the specified URL into the history.

     If it does not exist, then it is created.  If it does exist it is overwritten.

     Arguments:

     pwszUrl			- The URL in question.

     pwszTitle	- pointer to the friendly title that should be associated
     with this URL. If NULL, no title will be added.

     dwFlags             - Sets options for storage type and durability
     Not implemented yet

     Return Value:

     HRESULT
     Success - S_OK

     Failure - E_ hresult

     --*/


HRESULT CUrlHistory::AddUrl(
                         IN LPCWSTR pwszUrl,			// Full URL to be added
                         IN LPCWSTR pwszTitle,	
                         IN DWORD dwFlags                // Storage options
                         )		
{
    return AddUrlAndNotify(pwszUrl, pwszTitle, dwFlags, TRUE, NULL, NULL);
}


BOOL CUrlHistory::s_CommitUrlCacheEntry(LPCTSTR pszPrefixedUrl, 
                        LPINTERNET_CACHE_ENTRY_INFO pcei)
{
    if (s_dwDaysToKeep==0) {
        s_dwDaysToKeep = s_GetDaysToKeep();
    }

    //
    //	prepare the expire time
    //
    SYSTEMTIME st;
    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &pcei->LastModifiedTime);

    //
    // Assume the normal expiration date ... we add 6 days to expiration date
    // to make sure when we show the oldest week, we'll still have data for days
    // that are past the s_dwDaysToKeep
    //
    LONGLONG llExpireHorizon = SECS_PER_DAY * (s_dwDaysToKeep + 6);
    llExpireHorizon *= FILETIME_SEC;
    pcei->ExpireTime.dwLowDateTime = pcei->LastModifiedTime.dwLowDateTime + (DWORD) (llExpireHorizon % 0xFFFFFFFF);
    pcei->ExpireTime.dwHighDateTime = pcei->LastModifiedTime.dwHighDateTime + (DWORD) (llExpireHorizon / 0xFFFFFFFF);

    //
    // Check if it's subscribed
    //
    CHistoryData* phd =  CHistoryData::s_GetHistoryData(pcei);
    if (phd && phd->_FindExtra(PID_INTSITE_SUBSCRIPTION)) {
        //
        // It's subscribed. Keep it forever (until unsubscribed). 
        //
        TraceMsg(DM_URLCLEANUP, "CUH::s_CommitUrlCacheEntry found subscription key %s", pszPrefixedUrl);
        pcei->ExpireTime.dwLowDateTime = DW_FOREVERLOW;
        pcei->ExpireTime.dwHighDateTime = DW_FOREVERHIGH;
    }

#ifdef DEBUG
    LPCTSTR pszTitle = TEXT("(no extra data)");
    if (phd) {
        const HISTEXTRA* phext = phd->_FindExtra(PID_INTSITE_TITLE);
        if (phext && phext->vtExtra==VT_LPTSTR) {
            pszTitle = (LPCTSTR)phext->abExtra;
        } else {
            pszTitle = TEXT("(no title property)");
        }

        TraceMsg(DM_HISTCOMMIT, "CURL::s_C calling Commit for %s with %s",
            pszPrefixedUrl, pszTitle);
    }
#endif

    return CommitUrlCacheEntryBinary(pszPrefixedUrl,	
                                     pcei->ExpireTime,	
                                     pcei->LastModifiedTime,			
                                     pcei->CacheEntryType | URLHISTORY_CACHE_ENTRY,
                                     (LPBYTE)pcei->lpHeaderInfo,
                                     pcei->dwHeaderInfoSize);
}

void CUrlHistory::_WriteToHistory(LPCTSTR pszPrefixedUrl, FILETIME& ftExpires, IOleCommandTarget *poctNotify, IUnknown *punkSFHistory)
{
    IHistSFPrivate *psfpHistory;
    HRESULT hr = LoadHistoryShellFolder(punkSFHistory, &psfpHistory);
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidlNotify = NULL;
        //
        //	prepare the local mod time
        //
        SYSTEMTIME st;
        GetLocalTime (&st);
    
        FILETIME ftLocModified; // new history written in "User Perceived Time"
        SystemTimeToFileTime(&st, &ftLocModified);
        hr = psfpHistory->WriteHistory(pszPrefixedUrl,
                                ftExpires,
                                ftLocModified,
                                poctNotify ? &pidlNotify : NULL);
        if (pidlNotify)
        {
            VARIANTARG var;
            InitVariantFromIDList(&var, pidlNotify);

            poctNotify->Exec(&CGID_Explorer, SBCMDID_SELECTHISTPIDL, OLECMDEXECOPT_PROMPTUSER, &var, NULL);

            ILFree(pidlNotify);
            VariantClear(&var);
        }
        psfpHistory->Release();
    }
    // if we made it to here, we win!
}

void CUrlHistory::s_UpdateIcon(Intshcut* pintshcut, DWORD dwFlags)
{
    TCHAR szPath[MAX_PATH];
    int niIcon;
    UINT uFlags;

    // mask off the other stuff so we get consistent results.
    dwFlags &= PIDISF_RECENTLYCHANGED;
    
    // Get the old icon location 
    pintshcut->GetIconLocationFromFlags(0, szPath, SIZECHARS(szPath),
        &niIcon, &uFlags, dwFlags);
    
    // property.
//    int icachedImage = SHLookupIconIndex(PathFindFileName(szPath), niIcon, uFlags);
    int icachedImage = Shell_GetCachedImageIndex(PathFindFileName(szPath), niIcon, uFlags);

    TraceMsg(DM_HISTSPLAT, "CUH::s_UpdateIcon splat flag is changed for %s (%d)",
            szPath, icachedImage);

    SHUpdateImage( PathFindFileName(szPath), niIcon, uFlags, icachedImage );
}

HRESULT CUrlHistory::AddUrlAndNotify(
                         IN LPCWSTR pwszUrl,			// Full URL to be added
                         IN LPCWSTR pwszTitle,	
                         IN DWORD dwFlags,                // Storage options
                         IN BOOL fWriteHistory,         // Write History ShellFolder
                         IN IOleCommandTarget *poctNotify,
                         IN IUnknown *punkSFHistory)
{
    return AddUrlAndNotifyCP(pwszUrl, pwszTitle, dwFlags, fWriteHistory,
                           poctNotify, punkSFHistory, NULL);
}




HRESULT CUrlHistory::AddUrlAndNotifyCP(
                         IN LPCWSTR pwszUrl,			// Full URL to be added
                         IN LPCWSTR pwszTitle,	
                         IN DWORD dwFlags,                // Storage options
                         IN BOOL fWriteHistory,         // Write History ShellFolder
                         IN IOleCommandTarget *poctNotify,
                         IN IUnknown *punkSFHistory,
                         UINT* pcodepage)		
{
    if (pcodepage) {
        *pcodepage = CP_ACP;    // this is default.
    }

    HRESULT hr = S_OK;
    LPCWSTR pwszTitleToStore = pwszTitle;

    //  check to make sure we got an URL
    if (!pwszUrl || !pwszUrl[0])
    {
        return E_INVALIDARG;
    }

    if (pwszTitleToStore && 0 == StrCmpIW(pwszTitleToStore, pwszUrl))
    {
        //  Suppress redundant title data
        pwszTitleToStore = NULL;
    }

    CEI_PREALLOC buf;
    INTERNET_CACHE_ENTRY_INFO cei = { 0 };

    // Wininet URL cache only supports 8-bit ANSI, so we need to encode any characters
    // which can't be converted by the system code page, in order to allow Unicode
    // filenames in History.  The URLs will remain in the encoded form through most of
    // the History code paths, with only the display and navigation code needing to be
    // aware of the UTF8

    LPCTSTR pszUrlSource = pwszUrl; // points to the URL that we decide to use

    TCHAR szEncodedUrl[MAX_URL_STRING];
    BOOL bUsedDefaultChar;
    
    // Find out if any of the chars will get scrambled.  We can use our szEncodedUrl 
    // buffer to store he multibyte result because we don't actually want it
    
    WideCharToMultiByte(CP_ACP, 0, pwszUrl, -1, 
        (LPSTR) szEncodedUrl, sizeof(szEncodedUrl), NULL, &bUsedDefaultChar);
    
    if (bUsedDefaultChar)
    {
        // one or more chars couldn't be converted, so we store the UTF8 escaped string
        StrCpyN(szEncodedUrl, pwszUrl, ARRAYSIZE(szEncodedUrl));
        ConvertToUtf8Escaped(szEncodedUrl, ARRAYSIZE(szEncodedUrl));
        pszUrlSource = szEncodedUrl;
    }

    s_ConvertToPrefixedUrlW(pszUrlSource, buf.szPrefixedUrl, ARRAYSIZE(buf.szPrefixedUrl), &buf.pszFragment);
    s_RetrievePrefixedUrlInfo(buf.szPrefixedUrl, &buf);

    LPHISTDATA phdPrev = NULL;

    TCHAR szTitle[MAX_PATH];
    szTitle[0] = '\0';

    //
    //	if there is already an entry for this Url, then we will reuse some of the
    //	settings.  retrieve the relevant info if possible.
    //
    if (buf.pcei)
    {
        cei = *buf.pcei;   // copy the existing one
        phdPrev = CHistoryData::s_GetHistoryData(buf.pcei);
        if (pwszTitle==NULL && phdPrev) {
            phdPrev->_GetTitle(szTitle, ARRAYSIZE(szTitle));
        }

        if (pcodepage && phdPrev) {
            //
            // NOTES: This is the best place get the codepage stored
            //  in this URL history.
            //
            const HISTEXTRA* phextCP =phdPrev->_FindExtra(PID_INTSITE_CODEPAGE);
            if (phextCP && phextCP->vtExtra == VT_UI4) {
                *pcodepage = *(DWORD*)phextCP->abExtra;
                TraceMsg(DM_TRACE, "CUH::AddAndNotify this URL has CP=%d",
                         *pcodepage);
            }
        }

    } else {
        cei.CacheEntryType = NORMAL_CACHE_ENTRY;
        ASSERT(cei.dwHeaderInfoSize == 0);
    }

    //
    //	search for a fragment if necessary
    //
    if (buf.pszFragment && phdPrev)
    {
        if (phdPrev->_HasFragment(buf.pszFragment)) {
            buf.pszFragment = NULL;
        }
    }

    // Override the title if specified.
    if (pwszTitleToStore) {
        // GetDisplayableTitle puts szTitle[0] = '\0' if it's
        // not displayable with shell codepage
        StrCpyN(szTitle, pwszTitleToStore, ARRAYSIZE(szTitle));
    } 

    CHistoryData* phdNew = s_GenerateHeaderInfo(
               szTitle, phdPrev, buf.pszFragment, &cei.dwHeaderInfoSize);

    if (phdNew) {
        cei.lpHeaderInfo = (LPTSTR)phdNew;

        if (fWriteHistory && !UrlIsNoHistoryW(pwszUrl)) {
            phdNew->dwFlags |= PIDISF_HISTORY;
        }

        BOOL fUpdateIcon = FALSE;
    
        if (phdNew->dwFlags & PIDISF_RECENTLYCHANGED) {
            fUpdateIcon = TRUE;
            phdNew->dwFlags &= ~PIDISF_RECENTLYCHANGED;
        }
    
        if (s_CommitUrlCacheEntry(buf.szPrefixedUrl, &cei))
        {
            if (fUpdateIcon) {
                TraceMsg(DM_HISTSPLAT, "CUH::AddAndNotify remove splat!");

                // BUGBUG: This is a temporary hack to make splat update
                //  work as bad as previously.
                Intshcut* pintshcut = new Intshcut();
                if (pintshcut) {
                    pintshcut->SetURL(pwszUrl ,0);
                    s_UpdateIcon(pintshcut, PIDISF_RECENTLYCHANGED);
                    pintshcut->Release();
                }
            }
    
            //
            //  When we have successfully updated the global history and
            // we have updated something in the HISTDATA, update the
            // date-based history as well.
            //
            //
            // Cache IShellFolder for the history folder if we don't have
            // it yet.
            //
            if (fWriteHistory && !UrlIsNoHistoryW(pwszUrl))
            {
                _WriteToHistory(buf.szPrefixedUrl, cei.ExpireTime, poctNotify, punkSFHistory);
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError ());
        }

        LocalFree(phdNew);
    }

#ifdef DEBUG
    if (g_dwPrototype & 0x00000020) {
        TCHAR szUrl[MAX_URL_STRING];
        SHUnicodeToTChar(pwszUrl, szUrl, ARRAYSIZE(szUrl));
        PROPVARIANT var = { 0 };
        HRESULT hrT = GetProperty(szUrl, PID_INTSITE_SUBSCRIPTION, &var);
        if (SUCCEEDED(hrT)) {
            TraceMsg(DM_TRACE, "CUH::AddAndNotify got property vt=%d lVal=%x",
                        var.vt, var.lVal);
            PropVariantClear(&var);
        } else {
            TraceMsg(DM_TRACE, "CUH::AddAndNotify failed to get property (%x)", hrT);
        }
    }
#endif

    return hr;
}


HRESULT CUrlHistory::QueryUrl(
                          IN LPCWSTR pwszUrl,
                          IN DWORD dwFlags,
                          OUT LPSTATURL lpSTATURL
                          )
{
    if (!pwszUrl || !pwszUrl[0])
    {
        return E_INVALIDARG;
    }

    if (lpSTATURL)
    {
        lpSTATURL->pwcsUrl = NULL;
        lpSTATURL->pwcsTitle = NULL;
    }

    LPCTSTR pszFragment;
    TCHAR szPrefixedUrl[MAX_URL_STRING];

    s_ConvertToPrefixedUrlW(pwszUrl, szPrefixedUrl, ARRAYSIZE(szPrefixedUrl), &pszFragment);
    return s_QueryUrlCommon(szPrefixedUrl, pszFragment, dwFlags, lpSTATURL);
}

HRESULT CUrlHistory::s_QueryUrlCommon(
                          IN LPCTSTR lpszPrefixedUrl,
                          LPCTSTR lpszFragment,
                          IN DWORD dwFlags,
                          OUT LPSTATURL lpSTATURL
                          )

    /*++

     Routine Description:

     Checks to see if Url is a valid History item

     Arguments:

     pwszUrl			- The URL in question.

     dwFlags             - Flags on the query

     lpSTATURL           - points to a STATURL storage structure
     If this is NULL, then a S_OK means the URL was found.


     Return Value:

     HRESULT
     Success		- S_OK, Item found and STATURL filled

     Failure		- valid E_ code
     HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) indicates the URL is not available


     --*/

{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    LPHISTDATA phd = NULL;
    CEI_PREALLOC buf;

    //
    //  if there is no data required, and there are no fragments
    //  we dont need to get a copy of the CEI
    //
    if(!lpSTATURL && !lpszFragment)
    {
        if(s_IsCached(lpszPrefixedUrl))
            hr = S_OK;
        else
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

        goto quit;
    }

    s_RetrievePrefixedUrlInfo(lpszPrefixedUrl, &buf);
    if (buf.pcei)
    {
        DEBUG_CODE(DWORD cbNHI = buf.pcei->dwHeaderInfoSize;)
        phd = CHistoryData::s_GetHistoryData(buf.pcei);
        hr = S_OK;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError ());
        goto quit;
    }


    //
    //	Need to check for local anchor fragments
    //
    if (lpszFragment)
    {
        if (phd && phd->_HasFragment(lpszFragment))
        {
            hr = S_OK;
        }
    } else {
        hr = S_OK;
    }

    //  check to see if we should fill the STATURL
    if (S_OK == hr && lpSTATURL) {
        hr = s_GenerateSTATURL(buf.pcei, dwFlags, lpSTATURL);
    }

quit:

    if (S_OK != hr && lpSTATURL)
    {
        if (lpSTATURL->pwcsUrl)
            LocalFree(lpSTATURL->pwcsUrl);

        if (lpSTATURL->pwcsTitle)
            LocalFree(lpSTATURL->pwcsTitle);
    }

    return hr;
}

HRESULT CUrlHistory::QueryUrlA(LPCSTR pszUrl, DWORD dwFlags, LPSTATURL lpSTATURL)
{
    TCHAR szPrefixedUrl[MAX_URL_STRING];
    LPCTSTR lpszFragment = NULL;
    HRESULT hr = S_OK;

    if (!pszUrl || !pszUrl[0]) {
        return E_INVALIDARG;
    }

    if (lpSTATURL)
    {
        lpSTATURL->pwcsUrl = NULL;
        lpSTATURL->pwcsTitle = NULL;
    }

    TCHAR szUrl[MAX_URL_STRING];

    SHAnsiToUnicode(pszUrl, szUrl, ARRAYSIZE(szUrl));
    CUrlHistory::s_ConvertToPrefixedUrlW(szUrl, szPrefixedUrl, ARRAYSIZE(szPrefixedUrl), &lpszFragment);

    return CUrlHistory::s_QueryUrlCommon(szPrefixedUrl, lpszFragment, dwFlags, lpSTATURL);
}

HRESULT CUrlHistory::s_DeleteUrl(LPCWSTR pwszUrl, DWORD dwFlags)
{
    DWORD Error = ERROR_SUCCESS;
    TCHAR szPrefixedUrl[MAX_URL_STRING];
    LPCTSTR lpszFragment;
    BOOL  fDoDelete = TRUE;
    
    if (!pwszUrl || !pwszUrl[0]) {
        return E_INVALIDARG;
    }

    s_ConvertToPrefixedUrlW(pwszUrl, szPrefixedUrl, ARRAYSIZE(szPrefixedUrl), &lpszFragment);

    // don't delete it if its not a subscription
    if (dwFlags & URLFLAG_DONT_DELETE_SUBSCRIBED) {
        CEI_PREALLOC buf;
        // query to find out if its a subscription
        s_RetrievePrefixedUrlInfo(szPrefixedUrl, &buf);
        if (buf.pcei &&
            //  Hack alert (chrisfra) avoid deleting subscriptions, etc!
            ((buf.pcei)->ExpireTime.dwLowDateTime  == DW_FOREVERLOW) &&
            ((buf.pcei)->ExpireTime.dwHighDateTime == DW_FOREVERHIGH))
        {
            fDoDelete = FALSE;
            // re-write it as a non-history item and just a subscription
            CHistoryData *phdPrev = CHistoryData::s_GetHistoryData(buf.pcei);
            if (phdPrev) // offset into pcei structure
            {
                phdPrev->dwFlags &= ~PIDISF_HISTORY;
                s_CommitUrlCacheEntry(szPrefixedUrl, buf.pcei);
            }
            else {
                // I'd rather return ERROR_OUT_OF_PAPER...
                Error = ERROR_FILE_NOT_FOUND;
            }
        }
    }

    if (fDoDelete) {
        if(!::DeleteUrlCacheEntry(szPrefixedUrl))
            Error = GetLastError();
    }

    return HRESULT_FROM_WIN32(Error);
}

HRESULT CUrlHistory::DeleteUrl(LPCWSTR pwszUrl, DWORD dwFlags)
{
    return s_DeleteUrl(pwszUrl, dwFlags);
}

HRESULT CUrlHistory::BindToObject (LPCWSTR pwszUrl, REFIID riid, void **ppvOut)
{
    *ppvOut = NULL;
    return E_NOTIMPL;
#if 0
    HRESULT hr;

    if(!ppvOut || !pwszUrl || !*pwszUrl)
        return E_INVALIDARG;

    *ppvOut = NULL;

    CUrlHObj *pcuho = new CUrlHObj (this);
    if (!pcuho)
        return E_OUTOFMEMORY;

    hr = pcuho->QueryInterface(riid, ppvOut);
    pcuho->Release();

    if (SUCCEEDED(hr))
    {
        hr = pcuho->Init(pwszUrl);
        if (FAILED(hr))
        {
            pcuho->Release();
            *ppvOut = NULL;
        }
    }
    
    return hr;
#endif
}

HRESULT CUrlHistory::s_EnumUrls(IEnumSTATURL **ppEnum)
{
    HRESULT hres = E_OUTOFMEMORY;
    *ppEnum = NULL;
    CEnumSTATURL *penum = new CEnumSTATURL();
    if (penum)
    {
        *ppEnum = (IEnumSTATURL *)penum;
        hres = S_OK;
    }
    return hres;
}

HRESULT CUrlHistory::EnumUrls(IEnumSTATURL **ppEnum)
{
    return s_EnumUrls(ppEnum);
}

HRESULT CUrlHistory::GetProperty(LPCTSTR pszURL, PROPID pid, PROPVARIANT* pvarOut)
{
    HRESULT hres = E_FAIL;  // assume error
    pvarOut->vt = VT_EMPTY;

    CEI_PREALLOC buf;
    CUrlHistory::s_ConvertToPrefixedUrlW(pszURL, buf.szPrefixedUrl, ARRAYSIZE(buf.szPrefixedUrl), &buf.pszFragment);
    CUrlHistory::s_RetrievePrefixedUrlInfo(buf.szPrefixedUrl, &buf);
    if (buf.pcei) {
        CHistoryData* phdPrev =  CHistoryData::s_GetHistoryData(buf.pcei);
        if (phdPrev) {
            const HISTEXTRA* phextPrev;

            switch(pid) {
            case PID_INTSITE_FLAGS:
                pvarOut->vt = VT_UI4;
                pvarOut->lVal = phdPrev->dwFlags;
                hres = S_OK;
                break;
        
            case PID_INTSITE_LASTVISIT:
                pvarOut->vt = VT_FILETIME;
                pvarOut->filetime = buf.pcei->LastAccessTime;
                hres = S_OK;
                break;

            case PID_INTSITE_LASTMOD:
                pvarOut->vt = VT_FILETIME;
                pvarOut->filetime = buf.pcei->LastModifiedTime;
                hres = S_OK;
                break;

            case PID_INTSITE_WATCH:
                pvarOut->vt = VT_UI4;
                pvarOut->lVal = phdPrev->dwWatch;
                hres = S_OK;
                break;

            case PID_INTSITE_VISITCOUNT:
                pvarOut->vt   = VT_UI4;
                pvarOut->lVal = buf.pcei->dwHitRate;
                hres = S_OK;
                break;

            default:
                phextPrev = phdPrev->_FindExtra(pid);
                LPCWSTR pwsz;

                if (phextPrev) {
                    WCHAR wszBuf[MAX_URL_STRING];

                    switch(phextPrev->vtExtra) {
                    case VT_UI4:
                    case VT_I4:
                        pvarOut->vt = phextPrev->vtExtra;
                        pvarOut->lVal = *(DWORD*)phextPrev->abExtra;
                        hres = S_OK;
                        break;

                    case VT_LPSTR:
                        AnsiToUnicode((LPCSTR)phextPrev->abExtra, wszBuf, ARRAYSIZE(wszBuf));
                        pwsz = wszBuf;
                        goto Return_LPWSTR;

                    case VT_LPWSTR:
                        pwsz = (LPWSTR)phextPrev->abExtra;
Return_LPWSTR:
                        int cch = lstrlenW(pwsz)+1;
                        pvarOut->pwszVal = (LPWSTR)CoTaskMemAlloc(cch * SIZEOF(WCHAR));
                        if (pvarOut->pwszVal) {
                            StrCpyNW(pvarOut->pwszVal, pwsz, cch);
                            pvarOut->vt = VT_LPWSTR;
                            hres = S_OK;
                        } else {
                            hres = E_OUTOFMEMORY;
                        }
                        break;
                    }
                }
                break;
            }
        }
    }
    return hres;
}

//
//  IEnumSTATURL methods
//
CEnumSTATURL::~CEnumSTATURL()
{
    if(m_lpCEI)
        LocalFree(m_lpCEI);

    if(m_hEnum)
        FindCloseUrlCache(m_hEnum);

    return;
}

HRESULT CEnumSTATURL::QueryInterface(REFIID riid, PVOID *ppvObj)
{
    if (!ppvObj)
        return E_INVALIDARG;

    *ppvObj = NULL;

    if (IsEqualIID(IID_IUnknown, riid))
    {
        *ppvObj = (IUnknown *) this;
        AddRef();
        return S_OK;
    }
    else if (IsEqualIID(IID_IEnumSTATURL, riid))
    {
        *ppvObj = (IEnumSTATURL *) this;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

ULONG CEnumSTATURL::AddRef(void)
{
    _cRef++;

    return _cRef;
}


ULONG CEnumSTATURL::Release(void)
{
    _cRef--;

    if (!_cRef)
    {
        delete this;
        return 0;
    }

    return _cRef;
}



HRESULT CEnumSTATURL::RetrieveFirstUrlInfo()
{

    HRESULT hr = S_OK;

    ASSERT(!m_lpCEI);

    m_cbCEI = DEFAULT_CEI_BUFFER_SIZE;
    m_lpCEI = (LPINTERNET_CACHE_ENTRY_INFO) LocalAlloc(LPTR, DEFAULT_CEI_BUFFER_SIZE);
    if (!m_lpCEI)
    {
        hr = E_OUTOFMEMORY;
        goto quit;
    }

    while (TRUE)
    {
        m_hEnum = FindFirstUrlCacheEntryBinary(_szPrefixedUrl,
                                                   m_lpCEI,
                                                   &m_cbCEI);

        if (!m_hEnum)
        {
            DWORD Error = GetLastError ();

            LocalFree(m_lpCEI);
            m_lpCEI = NULL;

            if (Error == ERROR_INSUFFICIENT_BUFFER)
            {
                m_lpCEI = (LPINTERNET_CACHE_ENTRY_INFO) LocalAlloc(LPTR, m_cbCEI);
                if (!m_lpCEI)
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }
            }
            else
            {
                if (ERROR_NO_MORE_ITEMS == Error)
                    hr = S_FALSE;
                else
                    hr = HRESULT_FROM_WIN32(Error);
                break;
            }
        }
        else break;
    }

quit:

    m_cbCEI = max(m_cbCEI, DEFAULT_CEI_BUFFER_SIZE);

    return hr;
}

// This function should not becaused if the previous call failed
// and ::Reset() was never called.
HRESULT CEnumSTATURL::RetrieveNextUrlInfo()
{
    HRESULT hr = S_OK;
    BOOL ok;

    ASSERT(m_hEnum);

    while (TRUE)
    {

        ok = FindNextUrlCacheEntryBinary(m_hEnum,
                                             m_lpCEI,
                                             &m_cbCEI);

        if (!ok)
        {
            DWORD Error = GetLastError ();

            if (m_lpCEI)
            {
                LocalFree(m_lpCEI);
                m_lpCEI = NULL;
            }

            if (Error == ERROR_INSUFFICIENT_BUFFER)
            {
                m_lpCEI = (LPINTERNET_CACHE_ENTRY_INFO) LocalAlloc(LPTR, m_cbCEI);
                if (!m_lpCEI)
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }
            }
            else
            {
                if (ERROR_NO_MORE_ITEMS == Error)
                    hr = S_FALSE;
                else
                    hr = HRESULT_FROM_WIN32(Error);
                break;
            }
        }
        else break;
    }

    m_cbCEI = max(m_cbCEI, DEFAULT_CEI_BUFFER_SIZE);

    return hr;
}




HRESULT CEnumSTATURL::Next(ULONG celt, LPSTATURL rgelt, ULONG * pceltFetched)
    /*++

     Routine Description:

     Searches through the History looking for URLs that match the search pattern,
     and copies the STATURL into the buffer.

     Arguments:



     Return Value:



     --*/

{
    HRESULT hr = S_OK;
    BOOL found = FALSE;
    LPHISTDATA phd = NULL;

    if(pceltFetched)
        *pceltFetched = 0;

    if(!celt)
        goto quit;

    if (!m_hEnum)
    {
        //must handle new enumerator
        CUrlHistory::s_ConvertToPrefixedUrlW(m_poszFilter, _szPrefixedUrl, ARRAYSIZE(_szPrefixedUrl), &m_lpszFragment);

        //loop until we get our first handle or bag out
        hr = RetrieveFirstUrlInfo();
        if (S_OK != hr || !m_lpCEI)
            goto quit;

        m_cchPrefixedUrl = lstrlen(_szPrefixedUrl);

        while(StrCmpN(_szPrefixedUrl, m_lpCEI->lpszSourceUrlName, m_cchPrefixedUrl))
        {
            hr = RetrieveNextUrlInfo();
            if(S_OK != hr || !m_lpCEI)
                goto quit;
        }
    }
    else
    {
        do
        {
            hr = RetrieveNextUrlInfo();
            if (S_OK != hr || !m_lpCEI)
                goto quit;

        } while(StrCmpN(_szPrefixedUrl, m_lpCEI->lpszSourceUrlName, m_cchPrefixedUrl));
    }

    hr = CUrlHistory::s_GenerateSTATURL(m_lpCEI, m_dwFilter, rgelt);

    if(SUCCEEDED(hr) && pceltFetched)
        (*pceltFetched)++;



quit:
    if (pceltFetched) {
        ASSERT((0 == *pceltFetched && (S_FALSE == hr || FAILED(hr))) ||
               (*pceltFetched && S_OK == hr));
    }

    return hr;


}

HISTEXTRA* CHistoryData::CopyExtra(HISTEXTRA* phextCur) const
{
    const HISTEXTRA* phext;
    for (phext = _GetExtra();
         !phext->IsTerminator();
         phext = phext->GetNextFast())
    {
        if (phext->vtExtra != VT_EMPTY) {
            TraceMsg(DM_HISTEXTRA, "CHD::CopyExtra copying vt=%d id=%d %d bytes",
                    phext->vtExtra, phext->idExtra, phext->cbExtra);
            memcpy(phextCur, phext, phext->cbExtra);
            phextCur = phextCur->GetNextFastForSave();
        } else {
            TraceMsg(DM_HISTEXTRA, "CHD::CopyExtra skipping vt=%d id=%d %d bytes",
                    phext->vtExtra, phext->idExtra, phext->cbExtra);
        }
    }

    return phextCur;
}

CHistoryData* CHistoryData::s_AllocateHeaderInfo(UINT cbExtra, const HISTDATA* phdPrev, ULONG* pcbTotal)
{
    DWORD cbTotal = SIZEOF(HISTDATA) + SIZEOF(DWORD) + cbExtra;

    LPHISTDATA phdNew = (LPHISTDATA)LocalAlloc(LPTR, cbTotal);
    if (phdNew) {
        if (phdPrev) {
            *phdNew = *phdPrev; // Copy all the field
        }
        phdNew->cbSize = SIZEOF(HISTDATA);
        phdNew->cbVer = HISTDATA_VER;
        *pcbTotal = cbTotal;
    }

    return phdNew;
}

//
// Returns the total size of extra data (exclude VT_EMPTY)
//
UINT CHistoryData::GetTotalExtraSize() const
{
    const HISTEXTRA* phext;
    UINT cbTotal = 0;
    for (phext = _GetExtra();
         !phext->IsTerminator();
         phext = phext->GetNextFast())
    {
        if (phext->vtExtra != VT_EMPTY) {
            cbTotal += phext->cbExtra;
        }
    }

    return cbTotal;
}

HRESULT CEnumSTATURL::Skip(ULONG celt)
{
    return E_NOTIMPL;
}

HRESULT CEnumSTATURL::Reset(void)
{
    if(m_hEnum)
    {
        FindCloseUrlCache(m_hEnum);
        m_hEnum = NULL;
    }

    if(m_poszFilter)
    {
        LocalFree(m_poszFilter);
        m_poszFilter = NULL;
    }

    if(m_lpCEI)
    {
        LocalFree(m_lpCEI);
        m_lpCEI = NULL;
    }

    m_dwFilter = 0;


    return S_OK;
}

HRESULT CEnumSTATURL::Clone(IEnumSTATURL ** ppenum)
{
    return E_NOTIMPL;
}

//  IEnumSTATURL methods

HRESULT CEnumSTATURL::SetFilter(LPCWSTR poszFilter, DWORD dwFlags)
{
    HRESULT hr = S_OK;

    if(poszFilter)
    {
        m_poszFilter = StrDupW(poszFilter);
        if (!m_poszFilter)
        {
            hr = E_OUTOFMEMORY;
            goto quit;
        }

    }

    m_dwFilter = dwFlags;

quit:

    return hr;

}

const HISTEXTRA * CHistoryData::_FindExtra(UINT idExtra) const
{
    for (const HISTEXTRA* phext = _GetExtra();
         !phext->IsTerminator();
         phext = phext->GetNextFastForSave())
    {
        if (phext->idExtra == idExtra) {
            return phext;
        }
    }

    return NULL;
}

CHistoryData* CHistoryData::s_GetHistoryData(LPINTERNET_CACHE_ENTRY_INFO lpCEI)
{
    CHistoryData* phd = (CHistoryData*)lpCEI->lpHeaderInfo;
    if (phd && phd->_IsOldHistory()) {
        TraceMsg(DM_TRACE, "CHistoryData::GetHistoryData found old header. Ignore");
        phd = NULL;
    }

    if (phd && phd->cbVer != HISTDATA_VER) {
        TraceMsg(DM_TRACE, "CHistoryData::GetHistoryData found old header (%d). Ignore",
                 phd->cbVer);
        phd = NULL;
    }

    return phd;
}

BOOL CHistoryData::_HasFragment(LPCTSTR pszFragment) const
{
    BOOL fHas = FALSE;
    const HISTEXTRA* phext = _FindExtra(PID_INTSITE_FRAGMENT);

    if (phext) {
        for (LPCTSTR psz=(LPCTSTR)(phext->abExtra); *psz ; psz += lstrlen(psz)+1) {
            if (StrCmp(psz, pszFragment)==0) {
                fHas = TRUE;
                break;
            }
        }
    }

    return fHas;
}

void CHistoryData::_GetTitle(LPTSTR szTitle, UINT cchMax) const
{
    szTitle[0] = '\0';
    const HISTEXTRA* phext = _FindExtra(PID_INTSITE_TITLE);
    if (phext && phext->vtExtra == VT_LPSTR) {
        StrCpyN(szTitle, (LPCTSTR)phext->abExtra, cchMax);
    }
}


#ifdef USE_NEW_HISTORYDATA
#include "urlprop2.cpp"
#endif // USE_NEW_HISTORYDATA
