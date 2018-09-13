//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       wrapfns.h
//
//  Contents:   The list of Unicode functions wrapped for Win95.  Each
//              wrapped function should listed in alphabetical order with
//              the following format:
//
//      STRUCT_ENTRY(FunctionName, ReturnType, (Param list with args), (Argument list))
//
//              For example:
//
//      STRUCT_ENTRY(RegisterClass, ATOM, (CONST WNDCLASSW * pwc), (pwc))
//
//      For functions which return void, use the following:
//
//      STRUCT_ENTRY_VOID(FunctionName, (Param list with args), (Argument list))
//
//----------------------------------------------------------------------------

#ifndef _WRAPFNS_H_
#define _WRAPFNS_H_

#ifdef __cplusplus
extern "C" {
#endif

#define STRUCT_ENTRY(FnName, FnType, FnParamList, FnArgs) \
        EXTERN_C FnType WINAPI FnName##Wrap FnParamList;

#define STRUCT_ENTRY_VOID(FnName, FnParamList, FnArgs) \
        EXTERN_C void WINAPI FnName##Wrap FnParamList;


#ifdef  UNICODE
#ifndef WINNT

#define lstrcmpW        StrCmpW
#define lstrcmpiW       StrCmpIW
#define lstrcpyW        StrCpyW
#define lstrcpynW       StrCpyNW
#define lstrcatW        StrCatW


#define AppendMenuW     AppendMenuWrap
STRUCT_ENTRY(AppendMenu,
             BOOL,
             (HMENU hMenu, UINT uFlags, UINT uIDnewItem, LPCWSTR lpnewItem),
             (hMenu, uFlags, uIDnewItem, lpnewItem))

#define CallMsgFilterW  CallMsgFilterWrap
STRUCT_ENTRY(CallMsgFilter, BOOL, (LPMSG lpMsg, int nCode), (lpMsg, nCode))

#define CallWindowProcW CallWindowProcWrap
STRUCT_ENTRY(CallWindowProc,
             LRESULT,
             (WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam),
             (lpPrevWndFunc, hWnd, Msg, wParam, lParam))

#define CharLowerW      CharLowerWrap
STRUCT_ENTRY(CharLower, LPWSTR, (LPWSTR lpsz), (lpsz))

#define CharLowerBuffW  CharLowerBuffWrap
STRUCT_ENTRY(CharLowerBuff, DWORD, (LPWSTR lpsz, DWORD cch), (lpsz, cch))

#define CharNextW       CharNextWrap
STRUCT_ENTRY(CharNext, LPWSTR, (LPCWSTR lpsz), (lpsz))

#define CharPrevW       CharPrevWrap
STRUCT_ENTRY(CharPrev, LPWSTR, (LPCWSTR lpszStart, LPCWSTR lpszCurrent), (lpszStart, lpszCurrent))

#define CharToOemW      CharToOemWrap
STRUCT_ENTRY(CharToOem, BOOL, (LPCWSTR lpszSrc, LPSTR lpszDst), (lpszSrc, lpszDst))

#define CharUpperW      CharUpperWrap
STRUCT_ENTRY(CharUpper, LPWSTR, (LPWSTR lpsz), (lpsz))

#define CharUpperBuffW  CharUpperBuffWrap
STRUCT_ENTRY(CharUpperBuff, DWORD, (LPWSTR lpsz, DWORD cch), (lpsz, cch))

#define CompareStringW  CompareStringWrap
STRUCT_ENTRY(CompareString,
             int,
             (LCID  Locale, DWORD  dwCmpFlags, LPCTSTR  lpString1, int  cchCount1, LPCTSTR  lpString2, int  cchCount2),
             (Locale, dwCmpFlags, lpString1, cchCount1, lpString2, cchCount2))

#define CopyAcceleratorTableW   CopyAcceleratorTableWrap
STRUCT_ENTRY(CopyAcceleratorTable,
            int,
            (HACCEL hAccelSrc, LPACCEL lpAccelDst, int cAccelEntries),
            (hAccelSrc, lpAccelDst, cAccelEntries))

#define CreateAcceleratorTableW CreateAcceleratorTableWrap
STRUCT_ENTRY(CreateAcceleratorTable, HACCEL, (LPACCEL pAccel, int cEntries), (pAccel, cEntries))

#define CreateDCW       CreateDCWrap
STRUCT_ENTRY(CreateDC,
             HDC,
             (LPCWSTR lpszDriver, LPCWSTR lpszDevice, LPCWSTR lpszOutput, CONST DEVMODEW* lpInitData),
             (lpszDriver, lpszDevice, lpszOutput, lpInitData))

#define CreateDirectoryW    CreateDirectoryWrap
STRUCT_ENTRY(CreateDirectory,
             BOOL,
             (LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes),
             (lpPathName, lpSecurityAttributes))

#define CreateEventW    CreateEventWrap
STRUCT_ENTRY(CreateEvent,
             HANDLE,
             (LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName),
             (lpEventAttributes, bManualReset, bInitialState, lpName))

#define CreateFileW     CreateFileWrap
STRUCT_ENTRY(CreateFile,
             HANDLE,
             (LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile),
             (lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile))

#define CreateFileMappingW  CreateFileMappingWrap
STRUCT_ENTRY(CreateFileMapping,
             HANDLE,
             (HANDLE hFile, LPSECURITY_ATTRIBUTES lpFileMappingAttributes, DWORD flProtect, DWORD dwMaxSizeHigh, DWORD dwMaxSizeLow, LPCWSTR lpName),
             (hFile, lpFileMappingAttributes, flProtect, dwMaxSizeHigh, dwMaxSizeLow, lpName))

#define CreateFontW     CreateFontWrap
STRUCT_ENTRY(CreateFont,
             HFONT,
             (int nHeight, int nWidth, int nEscapement, int nOrientation, int fnWeight, DWORD fdwItalic, DWORD fdwUnderline, DWORD fdwStrikeOut, DWORD fdwCharSet, DWORD fdwOutputPrecision, DWORD fdwClipPrecision, DWORD fdwQuality, DWORD fdwPitchAndFamily, LPCWSTR lpszFace),
             (nHeight, nWidth, nEscapement, nOrientation, fnWeight, fdwItalic, fdwUnderline, fdwStrikeOut, fdwCharSet, fdwOutputPrecision, fdwClipPrecision, fdwQuality, fdwPitchAndFamily, lpszFace))

#define CreateFontIndirectW CreateFontIndirectWrap
STRUCT_ENTRY(CreateFontIndirect, HFONT, (CONST LOGFONTW * lpfw), (lpfw))

#define CreateICW       CreateICWrap
STRUCT_ENTRY(CreateIC,
             HDC,
             (LPCWSTR lpszDriver, LPCWSTR lpszDevice, LPCWSTR lpszOutput, CONST DEVMODEW* lpInitData),
             (lpszDriver, lpszDevice, lpszOutput, lpInitData))

#define CreateWindowExW CreateWindowExWrap
STRUCT_ENTRY(CreateWindowEx,
             HWND,
             (DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam),
             (dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam))

#define DefWindowProcW  DefWindowProcWrap
STRUCT_ENTRY(DefWindowProc,
             LRESULT,
             (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam),
             (hWnd, msg, wParam, lParam))

#define DeleteFileW     DeleteFileWrap
STRUCT_ENTRY(DeleteFile, BOOL, (LPCWSTR lpsz), (lpsz))

#define DispatchMessageW    DispatchMessageWrap
STRUCT_ENTRY(DispatchMessage, LRESULT, (CONST MSG * pMsg), (pMsg))

#define EnumFontFamiliesW   EnumFontFamiliesWrap
STRUCT_ENTRY(EnumFontFamilies,
             int,
             (HDC hdc, LPCWSTR lpszFamily, FONTENUMPROC lpEnumFontProc, LPARAM lParam),
             (hdc, lpszFamily, lpEnumFontProc, lParam))

#define EnumFontFamiliesExW EnumFontFamiliesExWrap
STRUCT_ENTRY(EnumFontFamiliesEx,
             int,
             (HDC hdc, LPLOGFONTW lpLogFont, FONTENUMPROC lpEnumFontProc, LPARAM lParam, DWORD dwFlags),
             (hdc, lpLogFont, lpEnumFontProc, lParam, dwFlags))

#define EnumResourceNamesW  EnumResourceNamesWrap
STRUCT_ENTRY(EnumResourceNames,
             BOOL,
             (HINSTANCE hModule, LPCWSTR lpType, ENUMRESNAMEPROC lpEnumFunc, LONG lParam),
             (hModule, lpType, lpEnumFunc, lParam))

#define FindFirstFileW  FindFirstFileWrap
STRUCT_ENTRY(FindFirstFile,
             HANDLE,
             (LPCWSTR lpFileName, LPWIN32_FIND_DATAW pwszFd),
             (lpFileName, pwszFd))

#define FindResourceExW   FindResourceExWrap
STRUCT_ENTRY(FindResourceEx,
             HRSRC,
             (HINSTANCE hModule, LPCWSTR lpType, LPCWSTR lpName, WORD wLang),
             (hModule, lpType, lpName, wLang))

// This is not a typo.  FindResource and FindResourceEx take their parameters
// in a DIFFERENT order!
#define FindResourceW(a,b,c) FindResourceExW(a,c,b,0)

#define FindWindowW FindWindowWrap
STRUCT_ENTRY(FindWindow,
             HWND,
             (LPCWSTR lpClassName, LPCWSTR lpWindowName),
             (lpClassName, lpWindowName))

#define FormatMessageW  FormatMessageWrap
STRUCT_ENTRY(FormatMessage,
             DWORD,
             (DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId, DWORD dwLanguageId, LPWSTR lpBuffer, DWORD nSize, va_list *Arguments),
             (dwFlags, lpSource, dwMessageId, dwLanguageId, lpBuffer, nSize, Arguments))

#define GetClassInfoW   GetClassInfoWrap
STRUCT_ENTRY(GetClassInfo,
             BOOL,
             (HINSTANCE hModule, LPCWSTR lpClassName, LPWNDCLASSW lpWndClassW),
             (hModule, lpClassName, lpWndClassW))

#define GetClassLongW   GetClassLongWrap
STRUCT_ENTRY(GetClassLong, DWORD, (HWND hWnd, int nIndex), (hWnd, nIndex))

#define GetClassNameW   GetClassNameWrap
STRUCT_ENTRY(GetClassName,
             int,
             (HWND hWnd, LPWSTR lpClassName, int nMaxCount),
             (hWnd, lpClassName, nMaxCount))

#define GetClipboardFormatNameW GetClipboardFormatNameWrap
STRUCT_ENTRY(GetClipboardFormatName,
             int,
             (UINT format, LPWSTR lpFormatName, int cchFormatName),
             (format, lpFormatName, cchFormatName))

#define GetCurrentDirectoryW    GetCurrentDirectoryWrap
STRUCT_ENTRY(GetCurrentDirectory,
             DWORD,
             (DWORD nBufferLength, LPWSTR lpBuffer),
             (nBufferLength, lpBuffer))

#define GetDateFormatW GetDateFormatWrap
STRUCT_ENTRY(GetDateFormat,
             int,
             (LCID Locale, DWORD dwFlags, CONST SYSTEMTIME *lpDate, LPCWSTR lpFormat, LPWSTR lpDateStr, int cchDate),
             (Locale, dwFlags, lpDate, lpFormat, lpDateStr, cchDate))

#define GetDlgItemTextW GetDlgItemTextWrap
STRUCT_ENTRY(GetDlgItemText,
             UINT,
             (HWND hWndDlg, int idControl, LPWSTR lpsz, int cchMax),
             (hWndDlg, idControl, lpsz, cchMax))

#define GetFileAttributesW  GetFileAttributesWrap
STRUCT_ENTRY(GetFileAttributes, DWORD, (LPCWSTR lpsz), (lpsz))

#define GetFullPathNameW    GetFullPathNameWrap
STRUCT_ENTRY(GetFullPathName,
            DWORD,
            ( LPCWSTR lpFileName,
              DWORD nBufferLength,
              LPWSTR lpBuffer,
              LPWSTR *lpFilePart),
            ( lpFileName,  nBufferLength,  lpBuffer, lpFilePart))

#define GetKeyNameTextW  GetKeyNameTextWrap
STRUCT_ENTRY(GetKeyNameText, int, (LONG lParam, LPWSTR lpsz, int nSize), (lParam, lpsz, nSize))

#define GetLocaleInfoW  GetLocaleInfoWrap
STRUCT_ENTRY(GetLocaleInfo,
             int,
             (LCID Locale, LCTYPE LCType, LPWSTR lpsz, int cchData),
             (Locale, LCType, lpsz, cchData))

#define GetMenuItemInfoW    GetMenuItemInfoWrap
STRUCT_ENTRY(GetMenuItemInfo,
             BOOL,
             (HMENU hMenu, UINT uItem, BOOL fByPosition, LPMENUITEMINFOW lpmii),
             (hMenu, uItem, fByPosition, lpmii))

#define GetMenuStringW  GetMenuStringWrap
STRUCT_ENTRY(GetMenuString,
             int,
             (HMENU hMenu, UINT uIDItem, LPWSTR lpString, int nMaxCount, UINT uFlag),
             (hMenu, uIDItem, lpString, nMaxCount, uFlag))

#define GetMessageW     GetMessageWrap
STRUCT_ENTRY(GetMessage,
             BOOL,
             (LPMSG lpMsg, HWND hWnd , UINT wMsgFilterMin, UINT wMsgFilterMax),
             (lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax))

#define GetModuleFileNameW  GetModuleFileNameWrap
STRUCT_ENTRY(GetModuleFileName,
             DWORD,
             (HINSTANCE hModule, LPWSTR pwszFilename, DWORD nSize),
             (hModule, pwszFilename, nSize))

#define GetNumberFormatW GetNumberFormatWrap
STRUCT_ENTRY(GetNumberFormat,
             int,
             (LCID Locale, DWORD dwFlags, LPCWSTR lpValue, CONST NUMBERFMTW *lpFormat, LPWSTR lpNumberStr, int cchNumber),
             (Locale, dwFlags, lpValue, lpFormat, lpNumberStr, cchNumber))

#define GetSystemDirectoryW GetSystemDirectoryWrap
STRUCT_ENTRY(GetSystemDirectory,
             UINT,
             (LPWSTR lpBuffer, UINT uSize),
             (lpBuffer, uSize))

#define SearchPathW     SearchPathWrap
STRUCT_ENTRY(SearchPath,
             DWORD,
             (LPCWSTR lpPathName, LPCWSTR lpFileName, LPCWSTR lpExtension, DWORD cchReturnBuffer,
             LPWSTR  lpReturnBuffer, LPWSTR *  plpfilePart),
             (lpPathName, lpFileName, lpExtension, cchReturnBuffer, lpReturnBuffer, plpfilePart))

#define GetModuleHandleW    GetModuleHandleWrap
STRUCT_ENTRY(GetModuleHandle, HMODULE, (LPCWSTR lpsz), (lpsz))

#define GetObjectW      GetObjectWrap
STRUCT_ENTRY(GetObject,
             int,
             (HGDIOBJ hgdiObj, int cbBuffer, LPVOID lpvObj),
             (hgdiObj, cbBuffer, lpvObj))

#define GetPrivateProfileIntW   GetPrivateProfileIntWrap
STRUCT_ENTRY(GetPrivateProfileInt,
             UINT,
             (LPCWSTR lpAppName, LPCWSTR lpKeyName, INT nDefault, LPCWSTR lpFileName),
             (lpAppName, lpKeyName, nDefault, lpFileName))

#define GetProfileIntW   GetProfileIntWrap
STRUCT_ENTRY(GetProfileInt,
             UINT,
             (LPCWSTR lpAppName, LPCWSTR lpKeyName, INT nDefault),
             (lpAppName, lpKeyName, nDefault))

#define GetProfileStringW   GetProfileStringWrap
STRUCT_ENTRY(GetProfileString,
             DWORD,
             (LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpDefault, LPWSTR lpBuffer, DWORD dwBuffersize),
             (lpAppName, lpKeyName, lpDefault, lpBuffer, dwBuffersize))

#define GetPropW        GetPropWrap
STRUCT_ENTRY(GetProp, HANDLE, (HWND hWnd, LPCWSTR lpString), (hWnd, lpString))

#define GetStringTypeExW    GetStringTypeExWrap
STRUCT_ENTRY(GetStringTypeEx, BOOL,
            (LCID lcid, DWORD dwInfoType, LPCTSTR lpSrcStr, int cchSrc, LPWORD lpCharType),
            (lcid, dwInfoType, lpSrcStr, cchSrc, lpCharType))

#define GetTempFileNameW    GetTempFileNameWrap
STRUCT_ENTRY(GetTempFileName,
             UINT,
             (LPCWSTR lpPathName, LPCWSTR lpPrefixString, UINT uUnique, LPWSTR lpTempFileName),
             (lpPathName, lpPrefixString, uUnique, lpTempFileName))

#define GetTempPathW    GetTempPathWrap
STRUCT_ENTRY(GetTempPath, DWORD, (DWORD nBufferLength, LPWSTR lpBuffer), (nBufferLength, lpBuffer))

#define GetTextFaceW    GetTextFaceWrap
STRUCT_ENTRY(GetTextFace,
             int,
             (HDC hdc, int cch, LPWSTR lpFaceName),
             (hdc, cch, lpFaceName))

#define GetTextMetricsW GetTextMetricsWrap
STRUCT_ENTRY(GetTextMetrics, BOOL, (HDC hdc, LPTEXTMETRICW lptm), (hdc, lptm))

#define GetTimeFormatW GetTimeFormatWrap
STRUCT_ENTRY(GetTimeFormat,
             int,
             (LCID Locale, DWORD dwFlags, CONST SYSTEMTIME *lpTime, LPCWSTR lpFormat, LPWSTR lpTimeStr, int cchTime),
             (Locale, dwFlags, lpTime, lpFormat, lpTimeStr, cchTime))

#define GetWindowLongW  GetWindowLongWrap
STRUCT_ENTRY(GetWindowLong, LONG, (HWND hWnd, int nIndex), (hWnd, nIndex))

#define GetWindowTextW  GetWindowTextWrap
STRUCT_ENTRY(GetWindowText, int, (HWND hWnd, LPWSTR lpString, int nMaxCount), (hWnd, lpString, nMaxCount))

#define GetWindowTextLengthW    GetWindowTextLengthWrap
STRUCT_ENTRY(GetWindowTextLength, int, (HWND hWnd), (hWnd))

#define GetWindowsDirectoryW GetWindowsDirectoryWrap
STRUCT_ENTRY(GetWindowsDirectory, UINT, (LPWSTR lpWinPath, UINT cch), (lpWinPath, cch))

#define GlobalAddAtomW  GlobalAddAtomWrap
STRUCT_ENTRY(GlobalAddAtom, ATOM, (LPCWSTR lpString), (lpString))

#define GrayStringW     GrayStringWrap
STRUCT_ENTRY(GrayString,
             BOOL,
             (HDC hDC, HBRUSH hBrush, GRAYSTRINGPROC lpOutputFunc, LPARAM lpData, int nCount, int x, int y, int nWidth, int nHeight),
             (hDC, hBrush, lpOutputFunc, lpData, nCount, x, y, nWidth, nHeight))

#define ImmGetCompositionStringW    ImmGetCompositionStringWrap
STRUCT_ENTRY(ImmGetCompositionString,
             LONG,
             (HIMC hIMC, DWORD dwIndex, LPVOID lpBuf, DWORD dwBufLen),
             (hIMC, dwIndex, lpBuf, dwBufLen))

#define ImmSetCompositionStringW    ImmSetCompositionStringWrap
STRUCT_ENTRY(ImmSetCompositionString,
             LONG,
             (HIMC hIMC, DWORD dwIndex, LPVOID lpComp, DWORD dwCompLen, LPVOID lpRead, DWORD dwReadLen),
             (hIMC, dwIndex, lpComp, dwCompLen, lpRead, dwReadLen))

#define InsertMenuW     InsertMenuWrap
STRUCT_ENTRY(InsertMenu,
             BOOL,
             (HMENU hMenu, UINT uPosition, UINT uFlags, UINT uIDNewItem, LPCWSTR lpNewItem),
             (hMenu, uPosition, uFlags, uIDNewItem, lpNewItem))

#define IsCharAlphaW    IsCharAlphaWrap
STRUCT_ENTRY(IsCharAlpha, BOOL, (WCHAR wch), wch);

#define IsCharAlphaNumericW IsCharAlphaNumericWrap
STRUCT_ENTRY(IsCharAlphaNumeric, BOOL, (WCHAR wch), wch);

#define IsCharUpperW    IsCharUpperWrap
STRUCT_ENTRY(IsCharUpper, BOOL, (WCHAR wch), wch);

#define IsCharLowerW    IsCharLowerWrap
STRUCT_ENTRY(IsCharLower, BOOL, (WCHAR wch), wch);

#define IsDialogMessageW    IsDialogMessageWrap
STRUCT_ENTRY(IsDialogMessage, BOOL, (HWND hWndDlg, LPMSG lpMsg), (hWndDlg, lpMsg))

#define LoadAcceleratorsW   LoadAcceleratorsWrap
STRUCT_ENTRY(LoadAccelerators, HACCEL, (HINSTANCE hInstance, LPCWSTR lpTableName), (hInstance, lpTableName))

#define LoadBitmapW     LoadBitmapWrap
STRUCT_ENTRY(LoadBitmap, HBITMAP, (HINSTANCE hInstance, LPCWSTR lpBitmapName), (hInstance, lpBitmapName))

#define LoadCursorW     LoadCursorWrap
STRUCT_ENTRY(LoadCursor, HCURSOR, (HINSTANCE hInstance, LPCWSTR lpCursorName), (hInstance, lpCursorName))

#define LoadIconW       LoadIconWrap
STRUCT_ENTRY(LoadIcon, HICON, (HINSTANCE hInstance, LPCWSTR lpIconName), (hInstance, lpIconName))

#define LoadImageW      LoadImageWrap
STRUCT_ENTRY(LoadImage, HANDLE, (HINSTANCE hInstance, LPCWSTR lpName, UINT uType, int cxDesired, int cyDesired, UINT fuLoad),
                       (hInstance, lpName, uType, cxDesired, cyDesired, fuLoad))

#define LoadLibraryW    LoadLibraryWrap
STRUCT_ENTRY(LoadLibrary, HINSTANCE, (LPCWSTR lpLibFileName), (lpLibFileName))

#define LoadLibraryExW  LoadLibraryExWrap
STRUCT_ENTRY(LoadLibraryEx,
             HINSTANCE,
             (LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags),
             (lpLibFileName, hFile, dwFlags))

#define LoadMenuW       LoadMenuWrap
STRUCT_ENTRY(LoadMenu, HMENU, (HINSTANCE hInstance, LPCWSTR lpMenuName), (hInstance, lpMenuName))

#define LoadStringW     LoadStringWrap
STRUCT_ENTRY(LoadString,
             int,
             (HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer, int nBufferMax),
             (hInstance, uID, lpBuffer, nBufferMax))

#define MapVirtualKeyW  MapVirtualKeyWrap
STRUCT_ENTRY(MapVirtualKey, UINT, (UINT uCode, UINT uMapType), (uCode, uMapType))

#define MessageBoxIndirectW MessageBoxIndirectWrap
STRUCT_ENTRY(MessageBoxIndirect,
            BOOL,
            (LPMSGBOXPARAMS lpMsgBoxParams),
            (lpMsgBoxParams))

#define ModifyMenuW     ModifyMenuWrap
STRUCT_ENTRY(ModifyMenu,
             BOOL,
             (HMENU hMenu, UINT uPosition, UINT uFlags, UINT uIDNewItem, LPCWSTR lpNewItem),
             (hMenu, uPosition, uFlags, uIDNewItem, lpNewItem))

#define GetCharacterPlacementW  GetCharacterPlacementWrap
STRUCT_ENTRY(GetCharacterPlacement, DWORD,
             (HDC hdc, LPCTSTR lpString, int nCount, int nMaxExtent,
              LPGCP_RESULTS lpResults, DWORD dwFlags),
             (hdc, lpString, nCount, nMaxExtent,
              lpResults, dwFlags))

#define CopyFileW       CopyFileWrap
STRUCT_ENTRY(CopyFile,
             BOOL,
             (LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, BOOL bFailIfExists),
             (lpExistingFileName, lpNewFileName, bFailIfExists))

#define MoveFileW       MoveFileWrap
STRUCT_ENTRY(MoveFile,
             BOOL,
             (LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName),
             (lpExistingFileName, lpNewFileName))

#define OemToCharW      OemToCharWrap
STRUCT_ENTRY(OemToChar,
             BOOL,
             (LPCSTR lpszSrc, LPWSTR lpszDst),
             (lpszSrc, lpszDst))

#define OutputDebugStringW  OutputDebugStringWrap
STRUCT_ENTRY_VOID(OutputDebugString,
                  (LPCWSTR lpOutputString),
                  (lpOutputString))

#define PeekMessageW    PeekMessageWrap
STRUCT_ENTRY(PeekMessage,
             BOOL,
             (LPMSG lpMsg, HWND hWnd , UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg),
             (lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg))

#define PostMessageW    PostMessageWrap
STRUCT_ENTRY(PostMessage,
             BOOL,
             (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam),
             (hWnd, Msg, wParam, lParam))

#define PostThreadMessageW  PostThreadMessageWrap
STRUCT_ENTRY(PostThreadMessage,
             BOOL,
             (DWORD idThread, UINT Msg, WPARAM wParam, LPARAM lParam),
             (idThread, Msg, wParam, lParam))

#define RegCreateKeyW   RegCreateKeyWrap
STRUCT_ENTRY(RegCreateKey,
             LONG,
             (HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult),
             (hKey, lpSubKey, phkResult))

#define RegCreateKeyExW RegCreateKeyExWrap
STRUCT_ENTRY(RegCreateKeyEx,
             LONG,
             (HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition),
             (hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition))

#define RegDeleteKeyW   RegDeleteKeyWrap
STRUCT_ENTRY(RegDeleteKey, LONG, (HKEY hKey, LPCWSTR pwszSubKey), (hKey, pwszSubKey))

#define RegEnumKeyW     RegEnumKeyWrap
STRUCT_ENTRY(RegEnumKey,
             LONG,
             (HKEY hKey, DWORD dwIndex, LPWSTR lpName, DWORD cbName),
             (hKey, dwIndex, lpName, cbName))

#define RegEnumKeyExW   RegEnumKeyExWrap
STRUCT_ENTRY(RegEnumKeyEx,
             LONG,
             (HKEY hKey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcbName, LPDWORD lpReserved, LPWSTR lpClass, LPDWORD lpcbClass, PFILETIME lpftLastWriteTime),
             (hKey, dwIndex, lpName, lpcbName, lpReserved, lpClass, lpcbClass, lpftLastWriteTime))

#define RegOpenKeyW     RegOpenKeyWrap
STRUCT_ENTRY(RegOpenKey, LONG, (HKEY hKey, LPCWSTR pwszSubKey, PHKEY phkResult), (hKey, pwszSubKey, phkResult))

#define RegOpenKeyExW   RegOpenKeyExWrap
STRUCT_ENTRY(RegOpenKeyEx,
             LONG,
             (HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult),
             (hKey, lpSubKey, ulOptions, samDesired, phkResult))

#define RegQueryInfoKeyW    RegQueryInfoKeyWrap
STRUCT_ENTRY(RegQueryInfoKey,
            LONG,
            (HKEY hKey, LPWSTR lpClass, LPDWORD lpcbClass, LPDWORD lpReserved, LPDWORD lpcSubKeys, LPDWORD lpcbMaxSubKeyLen,
             LPDWORD lpcbMaxClassLen,  LPDWORD lpcValues, LPDWORD lpcbMaxValueNameLen, LPDWORD lpcbMaxValueLen,
             LPDWORD lpcbSecurityDescriptor, PFILETIME lpftLastWriteTime),
            (hKey, lpClass, lpcbClass, lpReserved, lpcSubKeys, lpcbMaxSubKeyLen, lpcbMaxClassLen, lpcValues, lpcbMaxValueNameLen, lpcbMaxValueLen, lpcbSecurityDescriptor, lpftLastWriteTime))

#define RegQueryValueW  RegQueryValueWrap
STRUCT_ENTRY(RegQueryValue,
             LONG,
             (HKEY hKey, LPCWSTR pwszSubKey, LPWSTR pwszValue, PLONG lpcbValue),
             (hKey, pwszSubKey, pwszValue, lpcbValue))

#define RegQueryValueExW    RegQueryValueExWrap
STRUCT_ENTRY(RegQueryValueEx,
             LONG,
             (HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData),
             (hKey, lpValueName, lpReserved, lpType, lpData, lpcbData))

#define RegSetValueW    RegSetValueWrap
STRUCT_ENTRY(RegSetValue,
             LONG,
             (HKEY hKey, LPCWSTR lpSubKey, DWORD dwType, LPCWSTR lpData, DWORD cbData),
             (hKey, lpSubKey, dwType, lpData, cbData))

#define RegSetValueExW  RegSetValueExWrap
STRUCT_ENTRY(RegSetValueEx,
             LONG,
             (HKEY hKey, LPCWSTR lpValueName, DWORD Reserved, DWORD dwType, CONST BYTE* lpData, DWORD cbData),
             (hKey, lpValueName, Reserved, dwType, lpData, cbData))

#define RegisterClassW  RegisterClassWrap
STRUCT_ENTRY(RegisterClass, ATOM, (CONST WNDCLASSW * pwc), (pwc))

#define RegisterClipboardFormatW    RegisterClipboardFormatWrap
STRUCT_ENTRY(RegisterClipboardFormat, UINT, (LPCWSTR psz), (psz))

#define RegisterWindowMessageW  RegisterWindowMessageWrap
STRUCT_ENTRY(RegisterWindowMessage, UINT, (LPCWSTR psz), (psz))

#define RemovePropW     RemovePropWrap
STRUCT_ENTRY(RemoveProp, HANDLE, (HWND hwnd, LPCWSTR psz), (hwnd, psz))

#define SendDlgItemMessageW SendDlgItemMessageWrap
STRUCT_ENTRY(SendDlgItemMessage,
             LRESULT,
             (HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam),
             (hDlg, nIDDlgItem, Msg, wParam, lParam))

#define SendMessageW    SendMessageWrap
STRUCT_ENTRY(SendMessage,
             LRESULT,
             (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam),
             (hWnd, Msg, wParam, lParam))

#define SendNotifyMessageW  SendNotifyMessageWrap
STRUCT_ENTRY(SendNotifyMessage,
             BOOL,
             (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam),
             (hWnd, Msg, wParam, lParam))

#define SetCurrentDirectoryW    SetCurrentDirectoryWrap
STRUCT_ENTRY(SetCurrentDirectory, BOOL, (LPCWSTR psz), (psz))

#define SetDlgItemTextW SetDlgItemTextWrap
STRUCT_ENTRY(SetDlgItemText, BOOL, (HWND hwnd, int id, LPCWSTR psz), (hwnd, id, psz))

#define SetMenuItemInfoW    SetMenuItemInfoWrap
STRUCT_ENTRY(SetMenuItemInfo,
             BOOL,
             (HMENU hMenu, UINT uItem, BOOL fByPosition, LPCMENUITEMINFOW lpmii),
             (hMenu, uItem, fByPosition, lpmii))

#define SetPropW        SetPropWrap
STRUCT_ENTRY(SetProp, BOOL, (HWND hwnd, LPCWSTR psz, HANDLE hData), (hwnd, psz, hData))

#define SetWindowLongW  SetWindowLongWrap
STRUCT_ENTRY(SetWindowLong, LONG, (HWND hWnd, int nIndex, LONG dwNewLong), (hWnd, nIndex, dwNewLong))

#define SetWindowsHookExW   SetWindowsHookExWrap
STRUCT_ENTRY(SetWindowsHookEx, HHOOK, (int idHook, HOOKPROC lpfn, HINSTANCE hmod, DWORD dwThreadId),
                (idHook, lpfn, hmod, dwThreadId))

#define SetWindowTextW  SetWindowTextWrap
STRUCT_ENTRY(SetWindowText, BOOL, (HWND hWnd, LPCWSTR lpString), (hWnd, lpString))

#define StartDocW       StartDocWrap
STRUCT_ENTRY(StartDoc, int, (HDC hDC, const DOCINFO * lpdi), (hDC, lpdi))

#define SystemParametersInfoW   SystemParametersInfoWrap
STRUCT_ENTRY(SystemParametersInfo,
             BOOL,
             (UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni),
             (uiAction, uiParam, pvParam, fWinIni))

#define TranslateAcceleratorW   TranslateAcceleratorWrap
STRUCT_ENTRY(TranslateAccelerator,
             int,
             (HWND hWnd, HACCEL hAccTable, LPMSG lpMsg),
             (hWnd, hAccTable, lpMsg))

#define UnregisterClassW    UnregisterClassWrap
STRUCT_ENTRY(UnregisterClass, BOOL, (LPCWSTR psz, HINSTANCE hinst), (psz, hinst))

#define VkKeyScanW  VkKeyScanWrap
STRUCT_ENTRY(VkKeyScan, SHORT, (WCHAR ch), (ch))

#define WinHelpW    WinHelpWrap
STRUCT_ENTRY(WinHelp, BOOL, (HWND hwnd, LPCWSTR szFile, UINT uCmd, DWORD dwData), (hwnd, szFile, uCmd, dwData))

#define wsprintfW   wsprintfWrap
STRUCT_ENTRY(wsprintf, int, (LPWSTR psz, LPCWSTR pszFormat, ...), (psz, pszFormat, ...))

#define wvsprintfW  wvsprintfWrap
STRUCT_ENTRY(wvsprintf, int, (LPWSTR psz, LPCWSTR pszFormat, va_list va), (psz, pszFormat, va))

#endif  // !WINNT

// Even on NT we want to wrap these for PlugUI on NT4

#define CreateDialogIndirectParamW  CreateDialogIndirectParamWrap
STRUCT_ENTRY(CreateDialogIndirectParam,
             HWND,
             (HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hWndParent , DLGPROC lpDialogFunc, LPARAM dwInitParam),
             (hInstance, lpTemplate, hWndParent , lpDialogFunc, dwInitParam))

#undef CreateDialogParam
#define CreateDialogParam CreateDialogParam_WeNeedToWriteTheThunkAgain

#define DialogBoxIndirectParamW DialogBoxIndirectParamWrap
STRUCT_ENTRY(DialogBoxIndirectParam,
             INT_PTR,
             (HINSTANCE hInstance, LPCDLGTEMPLATEW hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam),
             (hInstance, hDialogTemplate, hWndParent, lpDialogFunc, dwInitParam))

#undef DialogBoxParam
#define DialogBoxParam DialogBoxParam_WeNeedToWriteTheThunkAgain

// And these are ML-specific

#define MLIsMLHInstance MLIsMLHInstanceWrap
BOOL MLIsMLHInstanceWrap(HINSTANCE hInst);

#define MLSetMLHInstance MLSetMLHInstanceWrap
HRESULT MLSetMLHInstanceWrap(HINSTANCE hInst, LANGID lidUI);

#define MLClearMLHinstance MLClearMLHinstanceWrap
HRESULT MLClearMLHinstanceWrap(HINSTANCE hInst);


#endif // UNICODE



#if defined(FONT_LINK) || defined(UNICODE_WIN9x)

#define DrawTextW   DrawTextWrap
STRUCT_ENTRY(DrawText,
            int,
            (HDC hDC, LPCWSTR lpString, int nCount, LPRECT lpRect, UINT uFormat),
            (hDC, lpString, nCount, lpRect, uFormat))

// We should use the shlwapi versions.
#define DrawTextExW DrawTextExPrivWrap
STRUCT_ENTRY(DrawTextExPriv,
            int,
            (HDC hDC, LPWSTR lpString, int nCount, LPRECT lpRect, UINT uFormat, LPDRAWTEXTPARAMS lpDTParams),
            (hDC, lpString, nCount, lpRect, uFormat, lpDTParams))

#define ExtTextOutW     ExtTextOutWrap
STRUCT_ENTRY(ExtTextOut,
             BOOL,
             (HDC hdc, int xp, int yp, UINT eto, CONST RECT *lprect, LPCWSTR lpwch, UINT cLen, CONST INT *lpdxp),
             (hdc, xp, yp, eto, lprect, lpwch, cLen, lpdxp))

#define GetCharWidthW   GetCharWidthWrap
STRUCT_ENTRY(GetCharWidth,
             BOOL,
             (HDC hdc, UINT uFirstChar, UINT uLastChar, LPINT lpnWidths),
             (hdc, uFirstChar, uLastChar, lpnWidths))

#define GetTextExtentPointW     GetTextExtentPointWrap
STRUCT_ENTRY(GetTextExtentPoint,
             BOOL,
             (HDC hdc, LPCWSTR pwsz, int cb, LPSIZE pSize),
             (hdc, pwsz, cb, pSize))

#define GetTextExtentPoint32W   GetTextExtentPoint32Wrap
STRUCT_ENTRY(GetTextExtentPoint32,
             BOOL,
             (HDC hdc, LPCWSTR pwsz, int cb, LPSIZE pSize),
             (hdc, pwsz, cb, pSize))

#define TextOutW        TextOutWrap
STRUCT_ENTRY(TextOut, BOOL, (HDC hdc, int xp, int yp, LPCWSTR lpwch, int cLen), (hdc, xp, yp, lpwch, cLen))

#endif  // FONT_LINK || UNICODE_WIN9x

#ifdef __cplusplus
} // extern "C"
#endif

#endif  // _WRAPFNS_H_
