#ifndef _URLSHELL_H_
#define _URLSHELL_H_

// This header is an extraction of private macros and prototypes that
// originally came from shellp.h and shellprv.h.  

#define DATASEG_READONLY   DATA_SEG_READ_ONLY

#ifndef DebugMsg                                                                /* ;Internal */
#define DM_TRACE    0x0001      // Trace messages                               /* ;Internal */
#define DM_WARNING  0x0002      // Warning                                      /* ;Internal */
#define DM_ERROR    0x0004      // Error                                        /* ;Internal */
#define DM_ASSERT   0x0008      // Assertions                                   /* ;Internal */

#define Assert(f)                                                               /* ;Internal */
#define AssertE(f)      (f)                                                     /* ;Internal */
#define AssertMsg   1 ? (void)0 : (void)                                        /* ;Internal */
#define DebugMsg    1 ? (void)0 : (void)                                        /* ;Internal */
#endif                                                                          /* ;Internal */

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

#define ENTERCRITICAL   Shell_EnterCriticalSection();
#define LEAVECRITICAL   Shell_LeaveCriticalSection();

void Shell_EnterCriticalSection(void);
void Shell_LeaveCriticalSection(void);

#define CCH_KEYMAX      64          // DOC: max size of a reg key (under shellex)

#define FillExecInfo(_info, _hwnd, _verb, _file, _params, _dir, _show) \
        (_info).hwnd            = _hwnd;        \
        (_info).lpVerb          = _verb;        \
        (_info).lpFile          = _file;        \
        (_info).lpParameters    = _params;      \
        (_info).lpDirectory     = _dir;         \
        (_info).nShow           = _show;        \
        (_info).fMask           = 0;            \
        (_info).cbSize          = SIZEOF(SHELLEXECUTEINFO);

// Define some registry caching apis.  This will allow us to minimize the
// changes needed in the shell code and still try to reduce the number of
// calls that we make to the registry.
LONG SHRegQueryValueA(HKEY hKey,LPCSTR lpSubKey,LPSTR lpValue,PLONG lpcbValue);
LONG SHRegQueryValueExA(HKEY hKey,LPCSTR lpValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData);

LONG SHRegQueryValueW(HKEY hKey,LPCWSTR lpSubKey,LPWSTR lpValue,PLONG lpcbValue);
LONG SHRegQueryValueExW(HKEY hKey,LPCWSTR lpValueName,LPDWORD lpReserved,LPDWORD lpType,LPBYTE lpData,LPDWORD lpcbData);

#define GCD_MUSTHAVEOPENCMD     0x0001
#define GCD_ADDEXETODISPNAME    0x0002  // must be used with GCD_MUSTHAVEOPENCMD
#define GCD_ALLOWPSUDEOCLASSES  0x0004  // .ext type extensions

// Only valid when used with FillListWithClasses
#define GCD_MUSTHAVEEXTASSOC    0x0008  // There must be at least one extension assoc

#define DECLAREWAITCURSOR  HCURSOR hcursor_wait_cursor_save
#define SetWaitCursor()   hcursor_wait_cursor_save = SetCursor(LoadCursor(NULL, IDC_WAIT))
#define ResetWaitCursor() SetCursor(hcursor_wait_cursor_save)

// indexes into the shell image lists (Shell_GetImageList) for default images
// If you add to this list, you also need to update II_LASTSYSICON!

#define II_DOCNOASSOC         0         // document (blank page) (not associated)
#define II_APPLICATION        2         // application (exe, com, bat)

WINSHELLAPI BOOL  WINAPI Shell_GetImageLists(HIMAGELIST *phiml, HIMAGELIST *phimlSmall);
WINSHELLAPI int   WINAPI Shell_GetCachedImageIndex(LPCTSTR pszIconPath, int iIconIndex, UINT uIconFlags);

WINSHELLAPI int   WINAPI PickIconDlg(HWND hwnd, LPSTR pszIconPath, UINT cbIconPath, int *piIconIndex);
WINSHELLAPI BOOL  WINAPI GetFileNameFromBrowse(HWND hwnd, LPSTR szFilePath, UINT cbFilePath, LPCSTR szWorkingDir, LPCSTR szDefExt, LPCSTR szFilters, LPCSTR szTitle);

WINSHELLAPI void WINAPI PathGetShortPath(LPTSTR pszLongPath);
WINSHELLAPI BOOL WINAPI PathYetAnotherMakeUniqueName(LPTSTR  pszUniqueName, LPCTSTR pszPath, LPCTSTR pszShort, LPCTSTR pszFileSpec);
extern DWORD WINAPI GetExeType(LPCTSTR pszFile);
WINSHELLAPI LPITEMIDLIST WINAPI SHSimpleIDListFromPath(LPCTSTR pszPath);
WINSHELLAPI void          WINAPI ILFree(LPITEMIDLIST pidl);

//
// Path processing function
//

#define PPCF_ADDQUOTES               0x00000001        // return a quoted name if required
#define PPCF_ADDARGUMENTS            0x00000003        // appends arguments (and wraps in quotes if required)
#define PPCF_NODIRECTORIES           0x00000010        // don't match to directories
#define PPCF_NORELATIVEOBJECTQUALIFY 0x00000020        // don't return fully qualified relative objects
#define PPCF_FORCEQUALIFY            0x00000040        // qualify even non-relative names

WINSHELLAPI LONG WINAPI PathProcessCommand( LPCTSTR lpSrc, LPTSTR lpDest, int iMax, DWORD dwFlags );

// PathResolve flags
#define PRF_VERIFYEXISTS            0x0001
#define PRF_TRYPROGRAMEXTENSIONS    (0x0002 | PRF_VERIFYEXISTS)
#define PRF_FIRSTDIRDEF             0x0004
#define PRF_DONTFINDLNK             0x0008      // if PRF_TRYPROGRAMEXTENSIONS is specified
WINSHELLAPI int   WINAPI PathResolve(LPTSTR lpszPath, LPCTSTR FAR dirs[], UINT fFlags);

//======Hash Item=============================================================
typedef struct _HashTable * PHASHTABLE;
#define PHASHITEM LPCTSTR

typedef void (CALLBACK *HASHITEMCALLBACK)(PHASHTABLE pht, LPCTSTR sz, UINT wUsage, DWORD param);

LPCTSTR      WINAPI FindHashItem  (PHASHTABLE pht, LPCTSTR lpszStr);
LPCTSTR      WINAPI AddHashItem   (PHASHTABLE pht, LPCTSTR lpszStr);
LPCTSTR      WINAPI DeleteHashItem(PHASHTABLE pht, LPCTSTR lpszStr);
LPCTSTR      WINAPI PurgeHashItem (PHASHTABLE pht, LPCTSTR lpszStr);
#define     GetHashItemName(pht, sz, lpsz, cch)  lstrcpyn(lpsz, sz, cch)

PHASHTABLE  WINAPI CreateHashItemTable(UINT wBuckets, UINT wExtra, BOOL fCaseSensitive);
void        WINAPI DestroyHashItemTable(PHASHTABLE pht);

void        WINAPI SetHashItemData(PHASHTABLE pht, LPCTSTR lpszStr, int n, DWORD dwData);
DWORD       WINAPI GetHashItemData(PHASHTABLE pht, LPCTSTR lpszStr, int n);
void *      WINAPI GetHashItemDataPtr(PHASHTABLE pht, LPCTSTR lpszStr);

void        WINAPI EnumHashItems(PHASHTABLE pht, HASHITEMCALLBACK callback, DWORD dwParam);

#ifdef DEBUG
void        WINAPI DumpHashItemTable(PHASHTABLE pht);
#endif

#ifndef SIZEOF
#define SIZEOF(a)                   sizeof(a)
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)                (sizeof(a)/sizeof(a[0]))
#endif

#define PATH_CCH_EXT                64

#endif // _URLSHELL_H_

