//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       util.h
//
//--------------------------------------------------------------------------

#ifndef _UTIL_H_
#define _UTIL_H_

#ifndef _INC_CSCVIEW_CONFIG_H
#   include "config.h"
#endif

HRESULT GetRemotePath(LPCTSTR szInName, LPTSTR *pszOutName);
LPTSTR ULongToString(ULONG i, LPTSTR psz, ULONG cchMax);
VOID LocalFreeString(LPTSTR *ppsz);
BOOL LocalAllocString(LPTSTR *ppszDest, LPCTSTR pszSrc);
UINT SizeofStringResource(HINSTANCE hInstance, UINT idStr);
int LoadStringAlloc(LPTSTR *ppszResult, HINSTANCE hInstance, UINT idStr);
void ShellChangeNotify(LPCTSTR pszPath, WIN32_FIND_DATA *pfd, BOOL bFlush, LONG nEvent = 0);
inline void ShellChangeNotify(LPCTSTR pszPath, BOOL bFlush = FALSE, LONG nEvent = 0) {ShellChangeNotify(pszPath, NULL, bFlush, nEvent);}
HRESULT GetLinkTarget(LPCTSTR pszShortcut, HWND hwndOwner, LPTSTR *ppszTarget, PDWORD pdwAttr = NULL);
bool PathIsDotOrDotDot(LPCTSTR pszPath);
void CenterWindow(HWND hwnd, HWND hwndParent);
DWORD CSCUIRebootSystem(void);
HRESULT SHSimpleIDListFromFindData(LPCTSTR pszPath, const WIN32_FIND_DATA *pfd, LPITEMIDLIST *ppidl);
HRESULT RegisterForSyncAtLogonAndLogoff(DWORD dwMask, DWORD dwValue);
HRESULT IsRegisteredForSyncAtLogonAndLogoff(bool *pbLogon = NULL, bool *pbLogoff = NULL);
DWORD CscDelete(LPCTSTR pszPath);
HRESULT IsOpenConnectionShare(LPCTSTR pszShare);
HRESULT IsOpenConnectionPathUNC(LPCTSTR pszPathUNC);
BOOL IsCSCEnabled(void);
BOOL IsSyncInProgress(void);
BOOL IsPurgeInProgress(void);
BOOL IsWindowsTerminalServer(void);
void InvalidateTheDesktop(void);
HRESULT SHCreateFileSysBindCtx(const WIN32_FIND_DATA *pfd, IBindCtx **ppbc);
BOOL DeleteOfflineFilesFolderLink(HWND hwndParent = NULL);
BOOL ShowHidden(void);
BOOL ShowSuperHidden(void);
BOOL IsSyncMgrInitialized(void);
void SetSyncMgrInitialized(void);


//
// Info returned through CSCFindFirst[Next]File APIs.
//
struct CscFindData
{
    WIN32_FIND_DATA fd;
    DWORD           dwStatus;
    DWORD           dwPinCount;
    DWORD           dwHintFlags;
    FILETIME        ft;        
};

HANDLE CacheFindFirst(LPCTSTR pszPath, PSID psid, WIN32_FIND_DATA *pfd, DWORD *pdwStatus, DWORD *pdwPinCount, DWORD *pdwHintFlags, FILETIME *pft);

inline 
HANDLE CacheFindFirst(LPCTSTR pszPath, WIN32_FIND_DATA *pfd, DWORD *pdwStatus, DWORD *pdwPinCount, DWORD *pdwHintFlags, FILETIME *pft)
    { return CacheFindFirst(pszPath, (PSID)NULL, pfd, pdwStatus, pdwPinCount, pdwHintFlags, pft); }

inline 
HANDLE CacheFindFirst(LPCTSTR pszPath, CscFindData *p)
    { return CacheFindFirst(pszPath, &p->fd, &p->dwStatus, &p->dwPinCount, &p->dwHintFlags, &p->ft); }

inline 
HANDLE CacheFindFirst(LPCTSTR pszPath, PSID psid, CscFindData *p)
    { return CacheFindFirst(pszPath, psid, &p->fd, &p->dwStatus, &p->dwPinCount, &p->dwHintFlags, &p->ft); }

BOOL CacheFindNext(HANDLE hFind, WIN32_FIND_DATA *pfd, DWORD *pdwStatus, DWORD *pdwPinCount, DWORD *pdwHintFlags, FILETIME *pft);

inline
BOOL CacheFindNext(HANDLE hFind, CscFindData *p)
    { return CacheFindNext(hFind, &p->fd, &p->dwStatus, &p->dwPinCount, &p->dwHintFlags, &p->ft); }


inline bool IsHiddenSystem(DWORD dwAttr)
{
    return ((dwAttr & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) == (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM));
}


inline BOOL _PathIsSlow(DWORD dwSpeed) { return (dwSpeed && dwSpeed <= DWORD(CConfig::GetSingleton().SlowLinkSpeed())); }


typedef struct
{
    TCHAR    szVolume[80];         // Volume where CSC cache is stored.
    LONGLONG llBytesOnVolume;      // Disk size (bytes)
    LONGLONG llBytesTotalInCache;  // Size of cache (bytes)
    LONGLONG llBytesUsedInCache;   // Amount of cache used (bytes)
    DWORD    dwNumFilesInCache;    // Files in cache
    DWORD    dwNumDirsInCache;     // Directories in cache

} CSCSPACEUSAGEINFO;

void GetCscSpaceUsageInfo(CSCSPACEUSAGEINFO *psui);

typedef enum _enum_reason
{
    ENUM_REASON_FILE = 0,
    ENUM_REASON_FOLDER_BEGIN,
    ENUM_REASON_FOLDER_END
} ENUM_REASON;

typedef DWORD (WINAPI *PFN_CSCENUMPROC)(LPCTSTR, ENUM_REASON, DWORD, DWORD, DWORD, PWIN32_FIND_DATA, LPARAM);
DWORD _CSCEnumDatabase(LPCTSTR pszFolder, BOOL bRecurse, PFN_CSCENUMPROC pfnCB, LPARAM lpContext);

typedef DWORD (WINAPI *PFN_WIN32ENUMPROC)(LPCTSTR, ENUM_REASON, PWIN32_FIND_DATA, LPARAM);
DWORD _Win32EnumFolder(LPCTSTR pszFolder, BOOL bRecurse, PFN_WIN32ENUMPROC pfnCB, LPARAM lpContext);


//
// Statistical information about a particular network share in the CSC database.
//
typedef struct _CSCSHARESTATS
{
    int cTotal;
    int cPinned;
    int cModified;
    int cSparse;
    int cDirs;
    int cAccessUser;
    int cAccessGuest;
    int cAccessOther;
    bool bOffline;
    bool bOpenFiles;
} CSCSHARESTATS, *PCSCSHARESTATS;

typedef struct
{
    int cShares;
    int cTotal;
    int cPinned;
    int cModified;
    int cSparse;
    int cDirs;
    int cAccessUser;
    int cAccessGuest;
    int cAccessOther;
    int cSharesOffline;
    int cSharesWithOpenFiles;
} CSCCACHESTATS, *PCSCCACHESTATS;


//
// These flags indicate if the enumeration should stop when one or more associated 
// value's exceed 1.  This is useful when you're interested in 0 vs. !0 as opposed
// to an actual count.
// If multiple flags are set, the statistics enumeration continues until the
// values corresponding to ALL set unity flags are non-zero.
//
enum SHARE_STATS_UNITY_FLAGS { SSUF_NONE     = 0x00000000,   // This is the default.
                               SSUF_TOTAL    = 0x00000001,
                               SSUF_PINNED   = 0x00000002,
                               SSUF_MODIFIED = 0x00000004,
                               SSUF_SPARSE   = 0x00000008,
                               SSUF_DIRS     = 0x00000010,
                               SSUF_ACCUSER  = 0x00000020,
                               SSUF_ACCGUEST = 0x00000040,
                               SSUF_ACCOTHER = 0x00000080,
                               SSUF_ACCAND   = 0x00000100, // Must match all set access mask flags.
                               SSUF_ACCOR    = 0x00000200, // Match at least one access mask flag.
                               SSUF_ALL      = 0x000000FF };
//
// These flags indicate if any cache items should be excluded from the enumeration.
// By default, the value is 0 (everything included).  For perf reasons, we use the
// same flags defined in cscapi.h.
//
enum SHARE_STATS_EXCLUDE_FLAGS {
        SSEF_NONE               = 0x00000000,  // Default.  Include everything.
        SSEF_LOCAL_MOD_DATA     = FLAG_CSC_COPY_STATUS_DATA_LOCALLY_MODIFIED,
        SSEF_LOCAL_MOD_ATTRIB   = FLAG_CSC_COPY_STATUS_ATTRIB_LOCALLY_MODIFIED,
        SSEF_LOCAL_MOD_TIME     = FLAG_CSC_COPY_STATUS_TIME_LOCALLY_MODIFIED,
        SSEF_LOCAL_DELETED      = FLAG_CSC_COPY_STATUS_LOCALLY_DELETED,
        SSEF_LOCAL_CREATED      = FLAG_CSC_COPY_STATUS_LOCALLY_CREATED,
        SSEF_STALE              = FLAG_CSC_COPY_STATUS_STALE,
        SSEF_SPARSE             = FLAG_CSC_COPY_STATUS_SPARSE,
        SSEF_ORPHAN             = FLAG_CSC_COPY_STATUS_ORPHAN,
        SSEF_SUSPECT            = FLAG_CSC_COPY_STATUS_SUSPECT,
        SSEF_CSCMASK            = FLAG_CSC_COPY_STATUS_DATA_LOCALLY_MODIFIED |
                                  FLAG_CSC_COPY_STATUS_ATTRIB_LOCALLY_MODIFIED |
                                  FLAG_CSC_COPY_STATUS_TIME_LOCALLY_MODIFIED |
                                  FLAG_CSC_COPY_STATUS_LOCALLY_DELETED |
                                  FLAG_CSC_COPY_STATUS_LOCALLY_CREATED |
                                  FLAG_CSC_COPY_STATUS_STALE |
                                  FLAG_CSC_COPY_STATUS_SPARSE |
                                  FLAG_CSC_COPY_STATUS_ORPHAN |
                                  FLAG_CSC_COPY_STATUS_SUSPECT,
        SSEF_DIRECTORY          = 0x01000000,
        SSEF_FILE               = 0x02000000,
        SSEF_NOACCUSER          = 0x04000000,  // Exclude if no USER access.
        SSEF_NOACCGUEST         = 0x08000000,  // Exclude if no GUEST access.
        SSEF_NOACCOTHER         = 0x10000000,  // Exclude if no OTHER access.
        SSEF_NOACCAND           = 0x20000000   // Treat previous 3 flags as single mask.
        };

typedef struct
{
    DWORD dwExcludeFlags;  // [in] SSEF_XXXXX flags.
    DWORD dwUnityFlags;    // [in] SSUF_XXXXX flags.
    bool bAccessInfo;      // [in] Implied 'T' if unity or exclude access bits are set.
    bool bEnumAborted;     // [out]

} CSCGETSTATSINFO, *PCSCGETSTATSINFO;

BOOL _GetShareStatistics(LPCTSTR pszShare, PCSCGETSTATSINFO pi, PCSCSHARESTATS pss);
BOOL _GetCacheStatistics(PCSCGETSTATSINFO pi, PCSCCACHESTATS pcs);
BOOL _GetShareStatisticsForUser(LPCTSTR pszShare, PCSCGETSTATSINFO pi, PCSCSHARESTATS pss);
BOOL _GetCacheStatisticsForUser(PCSCGETSTATSINFO pi, PCSCCACHESTATS pcs);

// Higher level wrapper for IDA stuff
class CIDArray
{
private:
    STGMEDIUM       m_Medium;
    LPIDA           m_pIDA;
    LPSHELLFOLDER   m_psf;
    ULONG           m_cchFolder;
    LPTSTR          m_pszPath;
    LPTSTR          m_pszAlternatePath;
    bool            m_bSrcIsOfflineFilesFolder;

public:
    CIDArray() 
        : m_pIDA(NULL), 
          m_psf(NULL), 
          m_cchFolder(0), 
          m_pszPath(NULL), 
          m_pszAlternatePath(NULL),
          m_bSrcIsOfflineFilesFolder(false) { ZeroMemory(&m_Medium, sizeof(m_Medium)); }

    ~CIDArray();

    HRESULT Initialize(LPDATAOBJECT pdobj);

    UINT    Count()         { return (m_pIDA ? m_pIDA->cidl : 0); }
    HRESULT GetItemAttributes(UINT iItem, PDWORD pdwAttr);
    LPCTSTR GetItemPath(UINT iItem);

private:
    HRESULT GetItemName(LPSHELLFOLDER psf,
                        LPCITEMIDLIST pidl,
                        LPTSTR pszName,
                        UINT cchName,
                        SHGNO uFlags = SHGDN_FORPARSING);

    bool IsFromOfflineFilesFolder(LPIDA pida);
};


//
// Trivial class to ensure cleanup of FindFirst/FindNext handles.
// Perf should be as close as possible to a simple handle so most
// operations are defined inline.
// Implementation is in enum.cpp
//
class CCscFindHandle
{
    public:
        CCscFindHandle(HANDLE handle = INVALID_HANDLE_VALUE)
            : m_handle(handle), m_bOwns(INVALID_HANDLE_VALUE != handle) { }

        CCscFindHandle(const CCscFindHandle& rhs)
            : m_handle(INVALID_HANDLE_VALUE), m_bOwns(false)
            { *this = rhs; }

        ~CCscFindHandle(void)
            { Close(); }

        void Close(void);

        HANDLE Detach(void) const
            { m_bOwns = false; return m_handle; }

        void Attach(HANDLE handle)
            { Close(); m_handle = handle; m_bOwns = true; }

        operator HANDLE() const
            { return m_handle; }

        bool IsValid(void) const
            { return INVALID_HANDLE_VALUE != m_handle; }

        CCscFindHandle& operator = (HANDLE handle)
            { Attach(handle); return *this; }

        CCscFindHandle& operator = (const CCscFindHandle& rhs);

    private:
        mutable HANDLE m_handle;
        mutable bool   m_bOwns;
};



//
// Ensures CoInitialize/CoUninitialize is exception-safe.
//
class CCoInit
{
    public:
        CCoInit(void)
            : m_hr(CoInitialize(NULL)) { }

        ~CCoInit(void)
            { if (SUCCEEDED(m_hr)) CoUninitialize(); }

        HRESULT Result(void) const
            { return m_hr; }
    private:
        HRESULT m_hr;
};


// String formatting functions - *ppszResult must be LocalFree'd
DWORD FormatStringID(LPTSTR *ppszResult, HINSTANCE hInstance, UINT idStr, ...);
DWORD FormatString(LPTSTR *ppszResult, LPCTSTR pszFormat, ...);
DWORD vFormatStringID(LPTSTR *ppszResult, HINSTANCE hInstance, UINT idStr, va_list *pargs);
DWORD vFormatString(LPTSTR *ppszResult, LPCTSTR pszFormat, va_list *pargs);


void EnableDlgItems(HWND hwndDlg, const UINT* pCtlIds, int cCtls, bool bEnable);
void ShowDlgItems(HWND hwndDlg, const UINT* pCtlIds, int cCtls, bool bShow);


//
// We commonly use groups of CSC status flags together.
// Define them here so we're consistent throughout the project.
// Note that "time" is not considered a contributor to being "dirty".
//
#define FLAG_CSCUI_COPY_STATUS_LOCALLY_DIRTY        (FLAG_CSC_COPY_STATUS_DATA_LOCALLY_MODIFIED | \
                                                     FLAG_CSC_COPY_STATUS_ATTRIB_LOCALLY_MODIFIED | \
                                                     FLAG_CSC_COPY_STATUS_LOCALLY_DELETED | \
                                                     FLAG_CSC_COPY_STATUS_LOCALLY_CREATED)

//
// Some helper inlines for querying cache item access information.
//
inline bool CscCheckAccess(DWORD dwShareStatus, DWORD dwShift, DWORD dwAccessType)
{
    return 0 != ((dwShareStatus >> dwShift) & dwAccessType);
}

inline bool CscAccessUserRead(DWORD dwShareStatus)
{
    return CscCheckAccess(dwShareStatus, FLAG_CSC_USER_ACCESS_SHIFT_COUNT, FLAG_CSC_READ_ACCESS);
}

inline bool CscAccessUserWrite(DWORD dwShareStatus)
{
    return CscCheckAccess(dwShareStatus, FLAG_CSC_USER_ACCESS_SHIFT_COUNT, FLAG_CSC_WRITE_ACCESS);
}

inline bool CscAccessUser(DWORD dwShareStatus)
{
    return 0 != (dwShareStatus & FLAG_CSC_USER_ACCESS_MASK);
}

inline bool CscAccessGuestRead(DWORD dwShareStatus)
{
    return CscCheckAccess(dwShareStatus, FLAG_CSC_GUEST_ACCESS_SHIFT_COUNT, FLAG_CSC_READ_ACCESS);
}

inline bool CscAccessGuestWrite(DWORD dwShareStatus)
{
    return CscCheckAccess(dwShareStatus, FLAG_CSC_GUEST_ACCESS_SHIFT_COUNT, FLAG_CSC_WRITE_ACCESS);
}

inline bool CscAccessGuest(DWORD dwShareStatus)
{
    return 0 != (dwShareStatus & FLAG_CSC_GUEST_ACCESS_MASK);
}

inline bool CscAccessOtherRead(DWORD dwShareStatus)
{
    return CscCheckAccess(dwShareStatus, FLAG_CSC_OTHER_ACCESS_SHIFT_COUNT, FLAG_CSC_READ_ACCESS);
}

inline bool CscAccessOtherWrite(DWORD dwShareStatus)
{
    return CscCheckAccess(dwShareStatus, FLAG_CSC_OTHER_ACCESS_SHIFT_COUNT, FLAG_CSC_WRITE_ACCESS);
}

inline bool CscAccessOther(DWORD dwShareStatus)
{
    return 0 != (dwShareStatus & FLAG_CSC_OTHER_ACCESS_MASK);
}

inline bool CscCanUserMergeFile(DWORD dwStatus)
{
    return (0 == (FLAG_CSCUI_COPY_STATUS_LOCALLY_DIRTY & dwStatus)
            || CscAccessUserWrite(dwStatus)
            || CscAccessGuestWrite(dwStatus));
}

//
// template inlines avoid the side-effects of min/max macros.
//
template <class T>
inline const T&
MAX(const T& a, const T& b)
{
    return a > b ? a : b;
}

template <class T>
inline const T&
MIN(const T& a, const T& b)
{
    return a < b ? a : b;
}

class CWin32Handle
{
    public:
        CWin32Handle(HANDLE handle)
            : m_handle(handle) { }

        CWin32Handle(void)
            : m_handle(NULL) { }

        ~CWin32Handle(void)
            { Close(); }

        void Close(void)
            { if (m_handle) CloseHandle(m_handle); m_handle = NULL; }

        operator HANDLE() const
            { return m_handle; }

        HANDLE *HandlePtr(void)
            { DBGASSERT((NULL == m_handle)); return &m_handle; }

    private:
        HANDLE m_handle;

        //
        // Prevent copy.
        // This class is only intended for automatic handle cleanup.
        //
        CWin32Handle(const CWin32Handle& rhs);
        CWin32Handle& operator = (const CWin32Handle& rhs);
};



HRESULT DataObject_SetGlobal(IDataObject *pdtobj, CLIPFORMAT cf, HGLOBAL hGlobal);
HRESULT DataObject_SetDWORD(IDataObject *pdtobj, CLIPFORMAT cf, DWORD dw);
HRESULT DataObject_GetDWORD(IDataObject *pdtobj, CLIPFORMAT cf, DWORD *pdwOut);
HRESULT SetPreferredDropEffect(IDataObject *pdtobj, DWORD dwEffect);
DWORD   GetPreferredDropEffect(IDataObject *pdtobj);
HRESULT SetLogicalPerformedDropEffect(IDataObject *pdtobj, DWORD dwEffect);
DWORD   GetLogicalPerformedDropEffect(IDataObject *pdtobj);


#endif  // _UTIL_H_

