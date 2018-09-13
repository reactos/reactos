/****************************** Module Header ******************************\
* Module Name: winutil.h
*
* Copyright (c) 1991, Microsoft Corporation
*
* Define windows utility functions
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/


//
// Exported function prototypes
//


BOOL
SetActiveDesktop(
    PTERMINAL           pTerm,
    ActiveDesktops      Desktop);

BOOL
SetReturnDesktop(
    PWINDOWSTATION      pWS,
    PWLX_DESKTOP        pDesktop);

HDESK
GetActiveDesktop(
    PTERMINAL           pTerm,
    BOOL *              pCloseWhenDone,
    BOOL *              pLocked);

BOOL
OpenIniFileUserMapping(
    PTERMINAL           pTerm
    );

VOID
CloseIniFileUserMapping(
    PTERMINAL           pTerm
    );

LPTSTR
AllocAndGetPrivateProfileString(
    LPCTSTR lpAppName,
    LPCTSTR lpKeyName,
    LPCTSTR lpDefault,
    LPCTSTR lpFileName
    );

#define AllocAndGetProfileString(App, Key, Def) \
            AllocAndGetPrivateProfileString(App, Key, Def, NULL)


BOOL
WritePrivateProfileInt(
    LPCTSTR lpAppName,
    LPCTSTR lpKeyName,
    UINT Value,
    LPCTSTR lpFileName
    );

#define WriteProfileInt(App, Key, Value) \
            WritePrivateProfileInt(App, Key, Value, NULL)


LPTSTR
AllocAndExpandEnvironmentStrings(
    LPCTSTR lpszSrc
    );

LPTSTR
AllocAndRegEnumKey(
    HKEY hKey,
    DWORD iSubKey
    );

LPTSTR
AllocAndRegQueryValueEx(
    HKEY hKey,
    LPTSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType
    );

BOOL
SetEnvironmentULong(
    PVOID * Env,
    LPTSTR Variable,
    ULONG_PTR Value
    );

BOOL
SetEnvironmentLargeInt(
    PVOID * Env,
    LPTSTR Variable,
    LARGE_INTEGER Value
    );

LPWSTR
EncodeMultiSzW(
    IN LPWSTR MultiSz
    );

VOID
CentreWindow(
    HWND    hwnd
    );

VOID
SetupSystemMenu(
    HWND hDlg
    );


//
// Memory macros
//

#define Alloc(c)        ((PVOID)LocalAlloc(LPTR, c))
#define ReAlloc(p, c)   ((PVOID)LocalReAlloc(p, c, LPTR | LMEM_MOVEABLE))
#define Free(p)         ((VOID)LocalFree(p))
#define FreeAndNull(p)  { LocalFree(p) ; p = NULL ; }
