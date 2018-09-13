//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#define STRICT
#define _INC_OLE

#include <windows.h>
#include <shlapip.h>
#include <shlobj.h>
#include <shlobjp.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <shsemip.h>
#include <shellp.h>
#include <commdlg.h>
#include <commctrl.h>
#include <comctrlp.h>

//---------------------------------------------------------------------------
// Global to the app.
#define CCHSZSHORT      32
#define CCHSZNORMAL     256

#define MAXGROUPNAMELEN     30  // from progman

extern HINSTANCE g_hinst;
extern TCHAR g_szStartGroup[MAXGROUPNAMELEN + 1];
extern HKEY g_hkeyGrpConv;
extern const TCHAR c_szGroups[];
extern const TCHAR c_szNULL[];
extern const TCHAR c_szSettings[];
extern BOOL g_fDoingCommonGroups;
extern BOOL g_fDoProgmanDde;
extern BOOL g_fShowUI;

#define REGSTR_PATH_EXPLORER_SHELLFOLDERS REGSTR_PATH_EXPLORER TEXT("\\Shell Folders")


// This version of grpconv has to work on win95 and NT4, as well as memphis and NT5.
// Therefore, we have to undef a few things that get #defined to the xxxA and xxxW
// functions so the binary will link to the old shell32.nt4 and shell32.w95 libs.
#undef IsLFNDrive
WINSHELLAPI BOOL WINAPI IsLFNDrive(LPCTSTR pszPath);

#undef SHGetSpecialFolderPath
WINSHELLAPI BOOL WINAPI SHGetSpecialFolderPath(HWND hwndOwner, LPTSTR lpszPath, int nFolder, BOOL fCreate);

#undef PathFindFileName
LPTSTR WINAPI PathFindFileName(LPCTSTR pPath);

#undef PathAppend
BOOL WINAPI PathAppend(LPTSTR pPath, LPNCTSTR pMore);

#undef PathFileExists
BOOL WINAPI PathFileExists(LPCTSTR lpszPath);

#undef PathGetArgs
LPTSTR WINAPI PathGetArgs(LPCTSTR pszPath);

#undef PathUnquoteSpaces
void WINAPI PathUnquoteSpaces(LPTSTR lpsz);

#undef ILCreateFromPath
LPITEMIDLIST WINAPI ILCreateFromPath(LPCTSTR pszPath);

#undef PathRemoveFileSpec
BOOL WINAPI PathRemoveFileSpec(LPTSTR pFile);

#undef PathFindExtension
WINSHELLAPI LPTSTR WINAPI PathFindExtension(LPCTSTR pszPath);
                                                                        
#undef PathAddBackslash
LPTSTR WINAPI PathAddBackslash(LPTSTR lpszPath);

#undef PathIsRoot
BOOL  WINAPI PathIsRoot(LPCTSTR pPath);

#undef PathCombine
LPTSTR WINAPI PathCombine(LPTSTR lpszDest, LPCTSTR lpszDir, LPNCTSTR lpszFile);

#undef PathGetDriveNumber
int WINAPI PathGetDriveNumber(LPNCTSTR lpsz);

#undef PathRemoveBlanks
void WINAPI PathRemoveBlanks(LPTSTR lpszString);

#undef PathIsUNC
BOOL WINAPI PathIsUNC(LPTSTR pszPath);

// from shlguidp.h
DEFINE_GUID(IID_IShellLinkDataList, 0x45e2b4ae, 0xb1c3, 0x11d0, 0xb9, 0x2f, 0x0, 0xa0, 0xc9, 0x3, 0x12, 0xe1);


#ifndef UNICODE
// The current headers will #define this to IID_IShellLinkA if 
// unicode is not defined. This will prevent us from linking to
// the win95 shell32.lib (iedev\lib\chicago\*\shell32.w95) and
// so we just define it here for the ANSI case.
#undef IID_IShellLink
DEFINE_SHLGUID(IID_IShellLink, 0x000214EEL, 0, 0);
#endif
