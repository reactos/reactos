#ifdef POSTPOSTSPLIT

Verify_if_any_needs_to_be_loaded_from_browse_UI
   instead of duplicating it here
#endif

#define StrRetToStrN            _AorW_StrRetToStrN
extern BOOL _AorW_StrRetToStrN(LPTSTR szOut, UINT uszOut, LPSTRRET pStrRet, LPCITEMIDLIST pidl);

#define PathCleanupSpec         _AorW_PathCleanupSpec
#define SHCLSIDFromString       _AorW_SHCLSIDFromString
#define SHILCreateFromPath      _AorW_SHILCreateFromPath
#define SHSimpleIDListFromPath  _AorW_SHSimpleIDListFromPath
#define StrToOleStr             _AorW_StrToOleStr
//#define StrRetToStrNA           _AorW_StrRetToStrNA
//#define StrRetToStrNW           _AorW_StrRetToStrNW
#define OleStrToStrN            _AorW_OleStrToStrN
#define GetFileNameFromBrowse   _AorW_GetFileNameFromBrowse
#define OpenRegStream           _AorW_OpenRegStream
#define PathQualify             _AorW_PathQualify
#define PathProcessCommand      _AorW_PathProcessCommand
#define GetProcessDword         Win95_GetProcessDword
#define Win32DeleteFile         _AorW_Win32DeleteFile
#define PathYetAnotherMakeUniqueName    _AorW_PathYetAnotherMakeUniqueName
#define PathResolve             _AorW_PathResolve
#define Shell_GetCachedImageIndex _AorW_Shell_GetCachedImageIndex
#define SHRunControlPanel       _AorW_SHRunControlPanel
#define PickIconDlg             _AorW_PickIconDlg

// The following functions were originally only TCHAR versions
// in Win95, but now have A/W versions.  Since we still need to
// run on Win95, we need to treat them as TCHAR versions and
// undo the A/W #define
#ifdef ILCreateFromPath
#undef ILCreateFromPath
#endif
#define ILCreateFromPath        _AorW_ILCreateFromPath

#ifdef SHGetSpecialFolderPath
#undef SHGetSpecialFolderPath
#endif
#define SHGetSpecialFolderPath  _AorW_SHGetSpecialFolderPath

#ifdef IsLFNDrive
#undef IsLFNDrive
#endif
#define IsLFNDrive              _AorW_IsLFNDrive

// Define the prototypes for each of these forwarders...

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif /* __cplusplus */
extern int _AorW_Shell_GetCachedImageIndex(LPCTSTR pszIconPath, int iIconIndex, UINT uIconFlags);
extern int _WorA_Shell_GetCachedImageIndex(LPCWSTR pszIconPath, int iIconIndex, UINT uIconFlags);
extern int _AorW_SHRunControlPanel(LPCTSTR pszOrig_cmdline, HWND errwnd);
extern LPITEMIDLIST _AorW_ILCreateFromPath(LPCTSTR pszPath);
extern int _AorW_PathCleanupSpec(LPCTSTR pszDir, LPTSTR pszSpec);
extern void _AorW_PathQualify(LPTSTR pszDir);
extern LONG WINAPI _AorW_PathProcessCommand(LPCTSTR lpSrc, LPTSTR lpDest, int iDestMax, DWORD dwFlags);
extern HRESULT _AorW_SHCLSIDFromString(LPCTSTR lpsz, LPCLSID lpclsid);
extern BOOL _AorW_SHGetSpecialFolderPath(HWND hwndOwner, LPTSTR pszPath, int nFolder, BOOL fCreate);
extern HRESULT _AorW_SHILCreateFromPath(LPCTSTR pszPath, LPITEMIDLIST *ppidl, DWORD *rgfInOut);
extern LPITEMIDLIST _AorW_SHSimpleIDListFromPath(LPCTSTR pszPath);
extern BOOL _AorW_StrRetToStrNA(LPSTR pszOut, UINT uszOut, LPSTRRET pStrRet, LPCITEMIDLIST pidl);
extern BOOL _AorW_StrRetToStrNW(LPWSTR pwzOut, UINT uszOut, LPSTRRET pStrRet, LPCITEMIDLIST pidl);
extern int _AorW_OleStrToStrN(LPTSTR psz, int cchMultiByte, LPCOLESTR pwsz, int cchWideChar);
extern BOOL WINAPI _AorW_GetFileNameFromBrowse(HWND hwnd, LPTSTR szFilePath, UINT cchFilePath,
        LPCTSTR szWorkingDir, LPCTSTR szDefExt, LPCTSTR szFilters, LPCTSTR szTitle);
extern IStream * _AorW_OpenRegStream(HKEY hkey, LPCTSTR pszSubkey, LPCTSTR pszValue, DWORD grfMode);

extern DWORD Win95_GetProcessDword(DWORD idProcess, LONG iIndex);
extern BOOL _AorW_Win32DeleteFile(LPCTSTR lpszFileName);

extern BOOL _AorW_PathYetAnotherMakeUniqueName(LPTSTR  pszUniqueName,
                                         LPCTSTR pszPath,
                                         LPCTSTR pszShort,
                                         LPCTSTR pszFileSpec);

extern BOOL _AorW_PathResolve(LPTSTR lpszPath, LPCTSTR dirs[], UINT fFlags);

extern BOOL _AorW_IsLFNDrive(LPTSTR lpszPath);
extern int  _AorW_PickIconDlg(HWND hwnd, LPTSTR pszIconPath, UINT cchIconPath, int * piIconIndex);

#ifdef __cplusplus
}

#endif  /* __cplusplus */

