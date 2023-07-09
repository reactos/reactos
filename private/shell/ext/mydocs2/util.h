#include <debug.h>
#include "dllload.h"
#include <shconv.h>    // for UNICODE/ANSI conversion macros

BOOL IsMyDocsHidden( VOID );
void SetFolderPath(LPCTSTR pszFolder, LPCTSTR pszPath);
BOOL IsWinstone97();
HRESULT ChangePersonalPath(LPCTSTR pszNewPath, LPCTSTR pszOldPath, BOOL *pfResetMyPics);
void ChangeMyPicsPath( LPCTSTR pszNewPath, LPCTSTR pszOldPath );
void ResetMyPics(LPCTSTR pszOldMyPics);
void InstallMyPictures();
DWORD IsPathGoodMyDocsPath( HWND hwnd, LPCTSTR pPath );
void RestoreMyDocsFolder(void);
void UpdateSendToFile(BOOL bDeleteOnly);
HRESULT GetMyDocumentsDisplayName( LPTSTR pPath, UINT cch );
BOOL PathIsLocalAndWriteable( LPTSTR pPath );
HRESULT CallRegInstall(HINSTANCE hInstance, LPCSTR szSection);
BOOL IsMyDocsIDList(LPCITEMIDLIST pidl);
LPITEMIDLIST MyDocsIDList(void);
void HideMyDocs();
void MyDocsUnmakeSystemFolder(LPCTSTR pPath);
DWORD MyDocsGetAttributes();
DWORD ComparePaths(LPCTSTR pszChild, LPCTSTR pszParent);


#define PATH_IS_GOOD            0x00000000
#define PATH_IS_DESKTOP         0x00000001
#define PATH_IS_PROFILE         0x00000002
#define PATH_IS_SYSTEM          0x00000003
#define PATH_IS_WINDOWS         0x00000004
#define PATH_IS_SHELLFOLDER     0x00000005
#define PATH_IS_MYDOCS          0x00000006
#define PATH_IS_NONDIR          0x00000007
#define PATH_IS_SENDTO          0x00000008
#define PATH_IS_RECENT          0x00000009
#define PATH_IS_HISTORY         0x0000000A
#define PATH_IS_COOKIES         0x0000000B
#define PATH_IS_PRINTHOOD       0x0000000C
#define PATH_IS_NETHOOD         0x0000000D
#define PATH_IS_STARTMENU       0x0000000E
#define PATH_IS_TEMPLATES       0x0000000F
#define PATH_IS_FAVORITES       0x00000010
#define PATH_IS_TEMP_INET       0x00000011
#define PATH_IS_FONTS           0x00000012
#define PATH_IS_APPDATA         0x00000013
#define PATH_IS_DRIVEROOT       0x00000014
#define PATH_IS_SYSDRIVEROOT    0x00000015
#define PATH_IS_ERROR           0x00000016
#define PATH_IS_NONEXISTENT     0x00000017
#define PATH_IS_READONLY        0x00000018

// This is a bit mask
#define PATH_IS_DIFFERENT   0x00000001
#define PATH_IS_EQUAL       0x00000002
#define PATH_IS_CHILD       0x00000004

#define MYDOCS_CLSID TEXT("{450d8fba-ad25-11d0-98a8-0800361b1103}") // CLSID_MyDocuments

