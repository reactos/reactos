#ifndef _DLLLOAD_H_
#define _DLLLOAD_H_

#ifdef __cplusplus
extern "C" {
#endif
//
// Temporarily delay load any post-w95 shell32 APIs
//

// (These alternative names are used to avoid the inconsistent DLL linkage
// errors we get in dllload.c if we use the real function name.)

BOOL    _ReadCabinetState( LPCABINETSTATE lpState, int iSize );
BOOL    _WriteCabinetState( LPCABINETSTATE lpState );
LONG    _PathProcessCommand( LPCTSTR lpSrc, LPTSTR lpDest, int iMax, DWORD dwFlags );
HRESULT _SHDefExtractIconA(LPCSTR pszIconFile, int iIndex, UINT uFlags,
                           HICON *phiconLarge, HICON *phiconSmall, 
                           UINT nIconSize);
int     _SHLookupIconIndexA(LPCSTR pszFile, int iIconIndex, UINT uFlags);

HRESULT _SHStartNetConnectionDialogA(HWND hwnd, LPCSTR pszRemoteName, DWORD dwType);
HRESULT _SHStartNetConnectionDialogW(HWND hwnd, LPCWSTR pszRemoteName, DWORD dwType);
BOOL    _SHGetNewLinkInfoA(LPCSTR pszLinkTo, LPCSTR pszDir, LPSTR pszName,
                             BOOL FAR * pfMustCopy, UINT uFlags);
BOOL WINAPI _DragQueryInfo(HDROP hDrop, LPDRAGINFO lpdi);
STDAPI_(LPNMVIEWFOLDER) _DDECreatePostNotify(LPNMVIEWFOLDER pnm);
STDAPI_(BOOL) _DDEHandleViewFolderNotify(IShellBrowser* psb, HWND hwnd, LPNMVIEWFOLDER lpnm);

// used in lib\cwndproc
BOOL    _SHChangeNotification_Unlock(LPSHChangeNotificationLock pshcnl);
LPSHChangeNotificationLock _SHChangeNotification_Lock(HANDLE hChangeNotification, DWORD dwProcId, LPITEMIDLIST **pppidl, LONG *plEvent);

//
//  WARNING!  These functions are available only on NT5 shell.  If you
//  call them with an IE4 shell or earlier, they will try to emulate
//  or possibly just fail outright.  Be prepared.
//
BOOL    _DAD_DragEnterEx2(HWND hwndTarget, const POINT ptStart, IDataObject *pdtObject);
BOOL WINAPI _ILGetPseudoNameW(LPCITEMIDLIST pidl, LPCITEMIDLIST pidlBase,
    LPWSTR pwzName, int fType);


#define ReadCabinetState            _ReadCabinetState
#define WriteCabinetState           _WriteCabinetState
#define SHChangeNotification_Lock   _SHChangeNotification_Lock
#define SHChangeNotification_Unlock _SHChangeNotification_Unlock
//SHStringFromGUIDA->shlwapi
#define SHDefExtractIconA           _SHDefExtractIconA
#define SHLookupIconIndexA          _SHLookupIconIndexA
#define SHUpdateImageA              _SHUpdateImageA
#define SHUpdateImageW              _SHUpdateImageW
#define SHHandleUpdateImage         _SHHandleUpdateImage
#define SHStartNetConnectionDialogA _SHStartNetConnectionDialogA
#define SHStartNetConnectionDialogW _SHStartNetConnectionDialogW
#define DAD_DragEnterEx2            _DAD_DragEnterEx2
#define SHGetNewLinkInfoA           _SHGetNewLinkInfoA
#define ILGetPseudoNameW            _ILGetPseudoNameW
#define DDECreatePostNotify         _DDECreatePostNotify
#define DDEHandleViewFolderNotify   _DDEHandleViewFolderNotify

#ifdef __cplusplus
};
#endif

#endif // _DLLLOAD_H_
