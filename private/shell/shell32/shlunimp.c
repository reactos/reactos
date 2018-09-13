// #include "shellprv.h"

#define  DebugPrintf  dprintf

#include <windef.h>
#include "stdarg.h"
#include <winerror.h>
#include <winbase.h>
#include <unimp.h>

APISTUB_1 (SheChangeDirA,              0)
APISTUB_1 (SheChangeDirW,              0)
APISTUB_3 (SheFullPathA,               0)
APISTUB_3 (SheFullPathW,               0)
APISTUB_2 (SheGetDirA,                 0)
APISTUB_2 (SheGetDirW,                 0)
APISTUB_0 (SheGetCurDrive,             0)
APISTUB_1 (SheSetCurDrive,             0)

#ifdef WINNT
APISTUB_2 (DuplicateIcon,              0)
APISTUB_10 (RealShellExecuteA,          0)
APISTUB_3  (FindExecutableW,            0)
APISTUB_6  (ShellExecuteW,              0)
APISTUB_3  (ExtractIconW,               0)
APISTUB_4  (InternalExtractIconW,       0)
APISTUB_3  (ExtractAssociatedIconW,     0)
APISTUB_10 (RealShellExecuteW,          0)
#endif
    
#ifndef UNICODE
APISTUB_4  (ShellAboutW,                0)
#endif
APISTUB_4  (DragQueryFileW,             0)
APISTUB_2  (DoEnvironmentSubstW,        0)
APISTUB_1  (FindEnvironmentStringW,     0)

APISTUB_2  (CheckEscapesA               ,0)     // 2 param. LPSTR,DWORD
APISTUB_2  (CheckEscapesW               ,0)     // 2 param. LPWSTR,DWORD
APISTUB_2  (CommandLineToArgvW          ,0)     // 2 param. WCHAR*,int*
APISTUB_6  (DragQueryFileAorW           ,0)     // 6 param. HDROP,UINT,PVOID,UINT,BOOL,BOOL
APISTUB_4  (ExtractAssociatedIconExW    ,0)     // 4 param. HINSTANCE,LPWSTR,LPWORD,LPWORD

#if !defined(WINNT) && !defined(UNICODE)
APISTUB_5  (ExtractIconExW              ,0)     // 5 param. LPWSTR,int,LPHICON,LPHICON,UINT
#endif

#ifdef WINNT
APISTUB_4  (ExtractAssociatedIconExA    ,0)     // 4 param. HINSTANCE,LPWSTR,LPWORD,LPWORD
APISTUB_5  (ExtractIconResInfoA         ,0)     // 5 param. HINSTANCE,LPSTR,WORD,LPWORD,LPHANDLE
APISTUB_5  (ExtractIconResInfoW         ,0)     // 5 param. HINSTANCE,LPWSTR,WORD,LPWORD,LPHANDLE
APISTUB_2  (ExtractVersionResource16W   ,0)     // 2 param. LPCWSTR,LPHANDLE
APISTUB_3  (InternalExtractIconListA    ,0)     // 3 param. HINSTANCE,LPSTR,LPINT
APISTUB_3  (InternalExtractIconListW    ,0)     // 3 param. HINSTANCE,LPWSTR,LPINT
APISTUB_2  (FreeIconList                ,0)     // 2 param. HANDLE,int
APISTUB_11 (RealShellExecuteExA         ,0)     // 11 param. HWND,LPCSTR,LPCSTR,LPCSTR,LPCSTR,LPSTR,LPCSTR,LPSTR,WORD,LPHANDLE,DWORD
APISTUB_11 (RealShellExecuteExW         ,0)     // 11 param. HWND,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR,LPWSTR,LPCWSTR,LPWSTR,WORD,LPHANDLE,DWORD
#endif
APISTUB_2  (RegenerateUserEnvironment   ,0)     // 2 param. PVOID *,BOOL
APISTUB_1  (SheChangeDirExA             ,0)     // 1 param. CHAR*
APISTUB_1  (SheChangeDirExW             ,0)     // 1 param. WCHAR*
APISTUB_3  (SheConvertPathW             ,0)     // 3 param. LPWSTR,LPWSTR,UINT
APISTUB_3  (SheGetDirExW                ,0)     // 3 param. LPWSTR,LPDWORD,LPWSTR
APISTUB_1  (SheGetPathOffsetW           ,0)     // 1 param. LPWSTR
APISTUB_1  (SheRemoveQuotesA            ,0)     // 1 param. LPSTR
APISTUB_1  (SheRemoveQuotesW            ,0)     // 1 param. LPWSTR
APISTUB_2  (SheShortenPathA             ,0)     // 2 param. LPSTR,BOOL
APISTUB_2  (SheShortenPathW             ,0)     // 2 param. LPWSTR,BOOL
APISTUB_3  (ShellHookProc               ,0)     // 3 param. INT,WPARAM,LPARAM
