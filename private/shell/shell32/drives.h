#ifndef _DRIVES_H_
#define _DRIVES_H_

#ifdef __cplusplus
extern "C" {
#endif

// "Public" exports from drivex.c
STDAPI CDrives_InvokeCommand(HWND hwnd, WPARAM wParam);
STDAPI_(UINT) CDrives_GetDriveType(int iDrive);
STDAPI_(void) CDrives_GetKeys(LPCIDDRIVE pidd, HKEY *keys);
STDAPI CDrives_GetHelpText(UINT offset, BOOL bWide, LPARAM lParam, UINT cch);
STDAPI_(IShellFolderViewCB*) CDrives_CreateSFVCB(IShellFolder* psf);
STDAPI CDrives_AddPages(void *lp, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);

STDAPI_(void) CDrives_Terminate(void);
STDAPI CDrives_DFMCallBackBG(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg,  WPARAM wParam, LPARAM lParam);
STDAPI CDrives_DFMCallBack(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg,  WPARAM wParam, LPARAM lParam);

STDAPI CDrivesIDLDropTarget_DragEnter(IDropTarget *pdropt, IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
STDAPI CDrivesIDLDropTarget_Drop(IDropTarget * pdropt, IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect);

#define MAX_LABEL_NTFS      32  // not including the NULL
#define MAX_LABEL_FAT       11  // not including the NULL

STDAPI_(UINT) GetMountedVolumeIcon(LPCTSTR pszMountPoint, LPTSTR pszModule, DWORD cchModule);
STDAPI SetDriveLabel(HWND hwnd, IUnknown* punkEnableModless, int iDrive, LPCTSTR pszDriveLabel);
STDAPI GetDriveComment(int iDrive, LPTSTR pszComment, int cchComment);
STDAPI GetDriveHTMLInfoTipFile(int iDrive, LPTSTR pszHTMLInfoTipFile, int cchHTMLInfoTipFile);
BOOL IsUnavailableNetDrive(int iDrive);
BOOL DriveIOCTL(LPTSTR pszDrive, int cmd, void *pvIn, DWORD dwIn, void *pvOut, DWORD dwOut);

#ifdef WINNT
BOOL ShowMountedVolumeProperties(LPCTSTR pszMountedVolume, HWND hwndParent);
#endif

// Globals from drivesx.c

extern const IDropTargetVtbl c_CDrivesDropTargetVtbl;
extern const ICONMAP c_aicmpDrive[];
extern const int c_nicmpDrives;

#ifdef __cplusplus
};
#endif

#endif // _DRIVES_H_

