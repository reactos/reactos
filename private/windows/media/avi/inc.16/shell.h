/*****************************************************************************\
*                                                                             *
* shell.h -  SHELL.DLL functions, types, and definitions		      *
*                                                                             *
* Copyright (c) 1993-1994, Microsoft Corp.	All rights reserved	      *
*                                                                             *
\*****************************************************************************/

#ifndef _INC_SHELL
#define _INC_SHELL

#include <commctrl.h>	// for ImageList_ and other this depends on
#include <shellapi.h>




#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif /* !RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

//====== Ranges for WM_NOTIFY codes ==================================

// Note that these are defined to be unsigned to avoid compiler warnings
// since NMHDR.code is declared as UINT.

// NM_FIRST - NM_LAST defined in commctrl.h (0U-0U) - (OU-99U)

// LVN_FIRST - LVN_LAST defined in commctrl.h (0U-100U) - (OU-199U)

// PSN_FIRST - PSN_LAST defined in prsht.h (0U-200U) - (0U-299U)

// HDN_FIRST - HDN_LAST defined in commctrl.h (0U-300U) - (OU-399U)

// TVN_FIRST - TVN_LAST defined in commctrl.h (0U-400U) - (OU-499U)

#define RDN_FIRST       (0U-500U)
#define RDN_LAST        (0U-519U)

// TTN_FIRST - TTN_LAST defined in commctrl.h (0U-520U) - (OU-549U)

#define SEN_FIRST       (0U-550U)
#define SEN_LAST        (0U-559U)

#define EXN_FIRST       (0U-1000U)  // shell explorer/browser
#define EXN_LAST        (0U-1199U)

#define MAXPATHLEN      MAX_PATH


#ifndef FO_MOVE //these need to be kept in sync with the ones in shlobj.h

#define FO_MOVE           0x0001
#define FO_COPY           0x0002
#define FO_DELETE         0x0003
#define FO_RENAME         0x0004

#define FOF_CREATEPROGRESSDLG      1
#define FOF_CONFIRMMOUSE           2
#define FOF_SILENT                 4  // don't create progress/report
#define FOF_RENAMEONCOLLISION      8
#define FOF_NOCONFIRMATION        16  // Don't prompt the user.
#define FOF_WANTMAPPINGHANDLE     32  // Fill in SHFILEOPSTRUCT.hNameMappings
                                      // Must be freed using SHFreeNameMappings

typedef WORD FILEOP_FLAGS;

#endif // FO_MOVE

// implicit parameters are:
//      if pFrom or pTo are unqualified names the current directories are
//      taken from the global current drive/directory settings managed
//      by Get/SetCurrentDrive/Directory
//
//      the global confirmation settings
typedef struct _SHFILEOPSTRUCT
{
	HWND		hwnd;
	UINT		wFunc;
	LPCSTR		pFrom;
	LPCSTR		pTo;
	FILEOP_FLAGS	fFlags;
	BOOL		fAnyOperationsAborted;
	LPVOID		hNameMappings;
} SHFILEOPSTRUCT, FAR *LPSHFILEOPSTRUCT;

int WINAPI SHFileOperation(LPSHFILEOPSTRUCT lpFileOp);
void WINAPI SHFreeNameMappings(HANDLE hNameMappings);

typedef struct _SHNAMEMAPPING
{
    LPSTR pszOldPath;
    LPSTR pszNewPath;
    int   cchOldPath;
    int   cchNewPath;
} SHNAMEMAPPING, FAR *LPSHNAMEMAPPING;

#define SHGetNameMappingCount(_hnm) \
	DSA_GetItemCount(_hnm)
#define SHGetNameMappingPtr(_hnm, _iItem) \
	(LPSHNAMEMAPPING)DSA_GetItemPtr(_hnm, _iItem)

/* util.c */

#define Shell_Initialize()	(TRUE)
#define Shell_Terminate() 	(TRUE)

#define STRREG_SHEX             "shellex"
#ifdef WIN32
//===================================================================
// Hotkey management API's

// Set the pending hotkey for the given app,
// The next top level window to be created by the given app will be given
// this hotkey.
BOOL WINAPI SHSetPendingHotkey(LPCSTR lpszPath, WORD wHotkey);

// Check the list of pending hotkeys and change the first occurence
// of lpszFrom to lpszTo
BOOL WINAPI SHChangePendingHotkey(LPCSTR lpszFrom, LPCSTR lpszTo);

// Delete all pending hotkeys.
void WINAPI SHDeleteAllPendingHotkeys(void);

// Set the hotkey for the given instance of an app.
BOOL WINAPI SHSetHotkeyByInstance(HINSTANCE hinst, WORD wHotkey);

// Delete a pending instance.
BOOL WINAPI SHDeletePendingHotkey(LPCSTR lpszPath);

// Get a pending hotkey given a path.
WORD WINAPI SHGetPendingHotkey(LPCSTR lpszPath);
#endif




typedef struct _SHELLEXECUTEINFO
{
	DWORD cbSize;
	HWND hwnd;
	LPCSTR lpVerb;
	LPCSTR lpFile;
	LPCSTR lpParameters;
	LPCSTR lpDirectory;
	LPCSTR lpClass;
	int nShow;
	LPVOID lpReserved;
	HINSTANCE hInstApp;
} SHELLEXECUTEINFO, FAR *LPSHELLEXECUTEINFO;

BOOL WINAPI ShellExecuteEx(LPSHELLEXECUTEINFO lpExecInfo);
void WINAPI WinExecError(HWND hwnd, int error, LPCSTR lpstrFileName, LPCSTR lpstrTitle);

// Hint flags for ShellFileClass
#define SFC_UNKNOWN         0x0000
#define SFC_FILE            0x0001
#define SFC_DIRECTORY       0x0002
#define SFC_SYSTEM          0x0004
#define SFC_TYPEMASK        (SFC_FILE|SFC_DIRECTORY|SFC_SYSTEM)
#define SFC_EXIST           0x4000  // We already know that this one exists
#define SFC_FULLYQUALIFIED  0x8000  // lpFile is fully qualified path.

int WINAPI ShellFileClass(LPCSTR lpFile, UINT wFlags,
        LPSTR lpszClassName, UINT cbClassName);
#define EIRESID(x) (-1 * (int)(x))
UINT WINAPI ExtractIconEx(LPCSTR lpszFile, int nIconIndex,
        HICON FAR *phiconLarge, HICON FAR *phiconSmall, UINT nIcons);

// Tray notification definitions
typedef struct _NOTIFYICONDATA
{
	DWORD cbSize;
	HWND hWnd;
	UINT uID;

	UINT uFlags;
	UINT uCallbackMessage;
	HICON hIcon;
	char szTip[64];
} NOTIFYICONDATA, *PNOTIFYICONDATA;




#define NIM_ADD		0x00000000
#define NIM_MODIFY	0x00000001
#define NIM_DELETE	0x00000002

#define NIF_MESSAGE	0x00000001
#define NIF_ICON	0x00000002
#define NIF_TIP		0x00000004

BOOL WINAPI Shell_NotifyIcon(DWORD dwMessage, PNOTIFYICONDATA lpData);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#ifndef RC_INVOKED
#pragma pack()
#endif  /* !RC_INVOKED */

#endif  // !_INC_SHELL
