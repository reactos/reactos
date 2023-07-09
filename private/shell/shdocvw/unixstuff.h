/* Unix specific function prototypes */

#include <mainwin.h>
#include "iethread.h"
#include "shalias.h"


EXTERN_C void SelectMotifMenu(HDC hdc, const RECT* prc, BOOL bSelected);
EXTERN_C void PaintUnixMenuArrow(HDC hdc, const RECT* prc, DWORD data);
EXTERN_C void PaintMotifMenuArrow(HDC hdc, const RECT* prc, BOOL bSelected);
EXTERN_C void PaintWindowsMenuArrow(HDC hdc, const RECT* prc);
EXTERN_C int  GetUnixMenuArrowWidth();
EXTERN_C BOOL CheckAndExecNewsScript( HWND hwnd );
EXTERN_C BOOL OEHandlesMail( void );
EXTERN_C void unixGetWininetCacheLockStatus ( BOOL *pBool, char **ppsz);
EXTERN_C const GUID CLSID_MsgBand;
EXTERN_C void IEAboutBox( HWND hWnd);
EXTERN_C void MwExecuteAtExit();
EXTERN_C int  ConvertModuleNameToUnix( LPTSTR path );
EXTERN_C void UnixHelp(LPWSTR pszName, IShellBrowser* psb );
EXTERN_C HRESULT UnixRegisterBrowserInActiveSetup();

void ContentHelp( IShellBrowser* psb );
EXTERN_C HRESULT _UnixSendDocToOE(LPCITEMIDLIST pidl, UINT uiCodePage, DWORD grfKeyState);
EXTERN_C void UnixAdjustWindowSize( HWND hwnd, IETHREADPARAM* piei );
STDAPI  UnixSendDocToMailRecipient(LPCITEMIDLIST pidl, UINT uiCodePage, DWORD grfKeyState);
void IEFrame_NewWindow_SameThread(IETHREADPARAM* piei);

BOOL CheckForValidSourceFile(HWND hDlg, LPTSTR szPath, LPTSTR szDisplay );
EXTERN_C BOOL CheckAndDisplayEULA(void);

BOOL ImportBookmarksStartup(HINSTANCE hInstWithStr);

#define MAILLINK_OPTION             TEXT(" maillink ")
#define MAILTO_0_OPTION             TEXT(" mailto0 " )
#define READMAIL_OPTION             TEXT(" readmail ")
#define MAILPAGE_OPTION             TEXT(" mailpage ")
#define BROWSER_DEFAULT_WIDTH  800
#define BROWSER_DEFAULT_HEIGHT 600

STDAPI_(BOOL) IsLocalFolderPidl( LPCITEMIDLIST pidl );
STDAPI_(HDPA) GetGlobalAliasList();

STDAPI_(LONG) StrLenUnalignedA( UNALIGNED CHAR * str );
STDAPI_(LONG) StrLenUnalignedW( UNALIGNED WCHAR * str );

LPITEMIDLIST UrlToPidl(UINT uiCP, LPCTSTR pszUrl, LPCWSTR pszLocation); // from shdocfl.cpp
