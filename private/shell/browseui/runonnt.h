#define _SHELL32_

#define PathCleanupSpec         _AorW_PathCleanupSpec
#define SHCLSIDFromString       _use_GUIDFromString_instead
#define SHCLSIDFromStringA      _use_GUIDFromStringA_instead
#define SHILCreateFromPath      _AorW_SHILCreateFromPath
#define SHSimpleIDListFromPath  _AorW_SHSimpleIDListFromPath
#define StrRetToStrN            _AorW_StrRetToStrN
#define GetFileNameFromBrowse   _AorW_GetFileNameFromBrowse
#define PathQualify             _AorW_PathQualify
#define PathProcessCommand      _AorW_PathProcessCommand
#define GetProcessDword         Win95_GetProcessDword
#define Win32DeleteFile         _AorW_Win32DeleteFile
#define PathYetAnotherMakeUniqueName    _AorW_PathYetAnotherMakeUniqueName
#define PathResolve             _AorW_PathResolve
#define Shell_GetCachedImageIndex _AorW_Shell_GetCachedImageIndex
#define SHRunControlPanel       _AorW_SHRunControlPanel
#define PickIconDlg             _AorW_PickIconDlg
#define SHCreateDirectory       _AorW_SHCreateDirectory

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

#ifdef UNICODE                  // Wrapper needed only on UNICODE build
#undef ShellAbout
#define ShellAbout              _AorW_ShellAbout
#endif

// Define the prototypes for each of these forwarders...

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif /* __cplusplus */
extern int _WorA_Shell_GetCachedImageIndex(LPCWSTR pszIconPath, int iIconIndex, UINT uIconFlags);
extern int _AorW_Shell_GetCachedImageIndex(LPCTSTR pszIconPath, int iIconIndex, UINT uIconFlags);
extern int _AorW_SHRunControlPanel(LPCTSTR pszOrig_cmdline, HWND errwnd);
extern LPITEMIDLIST _AorW_ILCreateFromPath(LPCTSTR pszPath);
extern int _AorW_PathCleanupSpec(LPCTSTR pszDir, LPTSTR pszSpec);
extern void _AorW_PathQualify(LPTSTR pszDir);
extern LONG WINAPI _AorW_PathProcessCommand(LPCTSTR lpSrc, LPTSTR lpDest, int iDestMax, DWORD dwFlags);
extern BOOL _AorW_SHGetSpecialFolderPath(HWND hwndOwner, LPTSTR pszPath, int nFolder, BOOL fCreate);
extern HRESULT _AorW_SHILCreateFromPath(LPCTSTR pszPath, LPITEMIDLIST *ppidl, DWORD *rgfInOut);
extern LPITEMIDLIST _AorW_SHSimpleIDListFromPath(LPCTSTR pszPath);
extern BOOL _AorW_StrRetToStrN(LPTSTR szOut, UINT uszOut, LPSTRRET pStrRet, LPCITEMIDLIST pidl);
extern BOOL WINAPI _AorW_GetFileNameFromBrowse(HWND hwnd, LPTSTR szFilePath, UINT cchFilePath,
        LPCTSTR szWorkingDir, LPCTSTR szDefExt, LPCTSTR szFilters, LPCTSTR szTitle);

extern DWORD Win95_GetProcessDword(DWORD idProcess, LONG iIndex);
extern BOOL _AorW_Win32DeleteFile(LPCTSTR lpszFileName);

extern BOOL _AorW_PathYetAnotherMakeUniqueName(LPTSTR  pszUniqueName,
                                         LPCTSTR pszPath,
                                         LPCTSTR pszShort,
                                         LPCTSTR pszFileSpec);

extern BOOL _AorW_PathResolve(LPTSTR lpszPath, LPCTSTR dirs[], UINT fFlags);

extern BOOL _AorW_IsLFNDrive(LPTSTR lpszPath);
extern int  _AorW_PickIconDlg(HWND hwnd, LPTSTR pszIconPath, UINT cchIconPath, int * piIconIndex);
extern int  _AorW_SHCreateDirectory(HWND hwnd, LPCTSTR pszPath);
extern int  _AorW_ShellAbout(HWND hWnd, LPCTSTR szApp, LPCTSTR szOtherStuff, HICON hIcon);


//
//  This is the "RunOn95" section, which thunks UNICODE functions
//  back down to ANSI so we can run on Win95 in browser-only mode.
//

#ifdef UNICODE
#define ILCreateFromPathA       _ILCreateFromPathA
#define ILCreateFromPathW        ILCreateFromPath
extern LPITEMIDLIST _ILCreateFromPathA(LPCSTR pszPath);
#else
#define ILCreateFromPathA        ILCreateFromPath
#define ILCreateFromPathW       _ILCreateFromPathW
extern LPITEMIDLIST _ILCreateFromPathW(LPCWSTR pszPath);
#endif


#define OpenRegStream       SHOpenRegStream     // shlwapi.dll

//
//  Miracle of miracles - We don't need to wrap SHStartNetConnectionDialogW
//  because Win9x/IE4 actually implements the thunk!
//

//
//  You cannot send these messages because Win95 doesn't understand them.
//
#undef BFFM_SETSELECTIONW
#undef BFFM_SETSTATUSTEXTW


#ifdef __cplusplus
}

#endif  /* __cplusplus */
