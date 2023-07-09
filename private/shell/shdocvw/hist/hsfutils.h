#ifndef HSFUTILS_H__
#define HSFUTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

UINT    MergePopupMenu(HMENU *phMenu, UINT idResource, UINT uSubOffset, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast);

void    _StringFromStatus(LPTSTR lpszBuff, unsigned cbSize, unsigned uStatus, DWORD dwAttributes);
int     _CompareHCFolderPidl(LPCITEMIDLIST lpSP1, LPCITEMIDLIST lpSP2);
int     _CompareSize(LPCEIPIDL lpSP1, LPCEIPIDL lpSP2);
VOID _GetFileTypeInternal(LPCEIPIDL pidl, LPTSTR pszStr, UINT cchStr);

#ifndef UNIX
void    _CopyCEI(LPINTERNET_CACHE_ENTRY_INFO pdst, LPINTERNET_CACHE_ENTRY_INFO psrc, DWORD dwBuffSize);
#else
void    _CopyCEI(UNALIGNED INTERNET_CACHE_ENTRY_INFO * pdst, LPINTERNET_CACHE_ENTRY_INFO psrc, DWORD dwBuffSize);
#endif

LPCTSTR _StripContainerUrlUrl(LPCTSTR pszHistoryUrl);
LPCTSTR _StripHistoryUrlToUrl(LPCTSTR pszHistoryUrl);
int     _CompareHCURLs(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
void _GetURLDispName(LPCEIPIDL pcei, LPTSTR pszName, UINT cchName);
UNALIGNED const TCHAR *_GetURLTitle(LPCEIPIDL pcei);
BOOL _URLTitleIsURL(LPCEIPIDL pcei);
LPCTSTR _GetURLTitleForDisplay(LPCEIPIDL pcei, LPTSTR szBuf, DWORD cchBuf);
LPCTSTR _FindURLFileName(LPCTSTR pszURL);
LPCTSTR HCPidlToSourceUrl(LPCITEMIDLIST pidl);
BOOL    _ValidateIDListArray(UINT cidl, LPCITEMIDLIST *ppidl);
LPCEIPIDL _IsValid_IDPIDL(LPCITEMIDLIST pidl);
LPHEIPIDL _IsValid_HEIPIDL(LPCITEMIDLIST pidl);
LPCTSTR _GetDisplayUrlForPidl(LPCITEMIDLIST pidl, LPTSTR pszDisplayUrl, DWORD dwDisplayUrl);
LPCTSTR _GetUrlForPidl(LPCITEMIDLIST pidl);


void _GetURLHostFromUrl_NoStrip(LPCTSTR lpszUrl, LPTSTR szHost, DWORD dwHostSize, LPCTSTR pszLocalHost);
void _GetURLHost(LPINTERNET_CACHE_ENTRY_INFO pcei, LPTSTR szHost, DWORD dwHostSize, LPCTSTR pszLocalHost);
#define _GetURLHostFromUrl(lpszUrl, szHost, dwHostSize, pszLocalHost) \
        _GetURLHostFromUrl_NoStrip(_StripHistoryUrlToUrl(lpszUrl), szHost, dwHostSize, pszLocalHost)

// Forward declarations IContextMenu of helper functions
void    _GenerateEvent(LONG lEventId, LPITEMIDLIST pidlFolder, LPITEMIDLIST pidl, LPITEMIDLIST pidlNew);
int     _LaunchApp(HWND hwnd, LPCTSTR lpszPath);
int     _LaunchAppForPidl(HWND hwnd, LPITEMIDLIST pidl);
int     _GetCmdID(LPCSTR pszCmd);
HRESULT _CreatePropSheet(HWND hwnd, LPCEIPIDL pcei, int iDlg, DLGPROC pfnDlgProc);

// Forward declarations of IDataObject helper functions
LPCTSTR _FindURLFileName(LPCTSTR pszURL);
BOOL    _FilterUserName(LPINTERNET_CACHE_ENTRY_INFO pcei, LPCTSTR pszCachePrefix, LPTSTR pszUserName);
BOOL    _FilterPrefix(LPINTERNET_CACHE_ENTRY_INFO pcei, LPCTSTR pszCachePrefix);

LPCTSTR ConditionallyDecodeUTF8(LPCTSTR pszUrl, LPTSTR pszBuf, DWORD cchBuf);

INT_PTR CALLBACK HistoryConfirmDeleteDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CachevuWarningDlg(LPCEIPIDL pcei, UINT uIDWarning, HWND hwnd);

void    _GetCacheItemTitle(LPCEIPIDL pcei, LPTSTR pszTitle, DWORD cchBufferSize);
#ifndef UNIX
void    FileTimeToDateTimeStringInternal(LPFILETIME lpft, LPTSTR pszText, int cchText, BOOL fUsePerceivedTime);
#else
void    FileTimeToDateTimeStringInternal(UNALIGNED FILETIME * lpft, LPTSTR pszText, int cchText, BOOL fUsePerceivedTime);
#endif

#ifdef __cplusplus
};
#endif


#endif
