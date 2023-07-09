/*
 *  shell.h
 *
 *  Header file for shell association database management functions
 */

#include <shellapi.h>   // make sure we don't conflict
#include "winuserp.h"
#include "wowshlp.h"

#ifndef PUBLIC_ONLY

// HANDLE hInstance;               // hinstance of DLL

#define SE_ERR_FNF                      2       // ShellExec() error returns
#define SE_ERR_PNF                      3
#define SE_ERR_OOM                      8


BOOL APIENTRY RegisterShellHook(HWND, BOOL);
#endif  // closes #ifndef PUBLIC_ONLY

//****************************************************************************
// THIS INFORMATION IS PUBLIC

#define CP_WINDOWS              1004        // windows code page

BOOL APIENTRY RegenerateUserEnvironment(PVOID *pPrevEnv,
                                        BOOL bSetCurrentEnv);


int     SheGetCurDrive(VOID);
int     SheSetCurDrive(int iDrive);

int     SheFullPathA(CHAR *fname, DWORD sizpath, CHAR *buf);
int     SheGetDirA(int iDrive, CHAR *str);
int     SheChangeDirA(register CHAR *newdir);

int     SheFullPathW(WCHAR *fname, DWORD sizpath, WCHAR *buf);
int     SheGetDirW(INT iDrive, WCHAR *str);
int     SheChangeDirW(register WCHAR *newdir);

BOOL SheGetDirExW(LPWSTR lpszCurDisk, LPDWORD lpcchCurDir,LPWSTR lpszCurDir);
INT SheChangeDirExW(register WCHAR *newdir);

INT SheChangeDirExA(register CHAR *newdir);

INT SheGetPathOffsetW(LPWSTR lpszDir);

HANDLE APIENTRY InternalExtractIconListA(HANDLE hInst, LPSTR lpszExeFileName, LPINT lpnIcons);
HANDLE APIENTRY InternalExtractIconListW(HANDLE hInst, LPWSTR lpszExeFileName, LPINT lpnIcons);

HICON APIENTRY ExtractAssociatedIconA(HINSTANCE hInst,LPSTR lpIconPath,LPWORD lpiIcon);
HICON APIENTRY ExtractAssociatedIconW(HINSTANCE hInst,LPWSTR lpIconPath,LPWORD lpiIcon);

HICON APIENTRY ExtractAssociatedIconExA(HINSTANCE hInst,LPSTR lpIconPath,LPWORD lpiIconIndex, LPWORD lpiIconId);
HICON APIENTRY ExtractAssociatedIconExW(HINSTANCE hInst,LPWSTR lpIconPath,LPWORD lpiIconIndex, LPWORD lpiIconId);

WORD APIENTRY ExtractIconResInfoA(HANDLE hInst,LPSTR lpszFileName,WORD wIconIndex,LPWORD lpwSize,LPHANDLE lphIconRes);
WORD APIENTRY ExtractIconResInfoW(HANDLE hInst,LPWSTR lpszFileName,WORD wIconIndex,LPWORD lpwSize,LPHANDLE lphIconRes);

VOID APIENTRY CheckEscapesA(LPSTR lpFileA, DWORD cch);
VOID APIENTRY CheckEscapesW(LPWSTR szFile, DWORD cch);

LPSTR APIENTRY SheRemoveQuotesA(LPSTR sz);
LPWSTR APIENTRY SheRemoveQuotesW(LPWSTR sz);

BOOL APIENTRY SheShortenPathA(LPSTR pPath, BOOL bShorten);
BOOL APIENTRY SheShortenPathW(LPWSTR pPath, BOOL bShorten);

BOOL SheConvertPathW(LPWSTR lpApp, LPWSTR lpFile, UINT cchCmdBuf);

DWORD ExtractVersionResource16W(LPCWSTR  lpwstrFilename, LPHANDLE lphData);

#ifndef UNICODE
#define RealShellExecute RealShellExecuteA
#define RealShellExecuteEx RealShellExecuteExA
#define SheFullPath SheFullPathA
#define SheGetDir SheGetDirA
#define SheChangeDir SheChangeDirA
#define InternalExtractIconList InternalExtractIconListA
#define ExtractAssociatedIcon ExtractAssociatedIconA
#define ExtractAssociatedIconEx ExtractAssociatedIconExA
#define ExtractIconResInfo ExtractIconResInfoA
#define CheckEscapes CheckEscapesA
#define SheRemoveQuotes SheRemoveQuotesA
#define SheShortenPath SheShortenPathA
#else
#define RealShellExecute RealShellExecuteW
#define RealShellExecuteEx RealShellExecuteExW
#define SheFullPath SheFullPathW
#define SheGetDir SheGetDirW
#define SheChangeDir SheChangeDirW
#define InternalExtractIconList InternalExtractIconListW
#define ExtractAssociatedIcon ExtractAssociatedIconW
#define ExtractAssociatedIconEx ExtractAssociatedIconExW
#define ExtractIconResInfo ExtractIconResInfoW
#define CheckEscapes CheckEscapesW
#define SheRemoveQuotes SheRemoveQuotesW
#define SheShortenPath SheShortenPathW
#endif //unicode

#undef ExpandEnvironmentStrings
#define ExpandEnvironmentStrings #error "Use SHExpandEnvironmentStrings instead"

