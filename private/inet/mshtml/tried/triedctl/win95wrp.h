// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved

#ifndef __WIN95WRP_CPP__

#ifndef __WIN95WRP_H__
#define __WIN95WRP_H__

#include <urlmon.h>
#include <wininet.h>

extern "C"{

BOOL WINAPI OAppendMenuW(HMENU hMenu, UINT uFlags, UINT uIDnewItem, LPCWSTR lpnewItem);
LRESULT WINAPI OCallWindowProcW(WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
DWORD WINAPI OCharLowerBuffW(LPWSTR lpsz, DWORD cchLength);
LPWSTR WINAPI OCharLowerW(LPWSTR lpsz);
LPWSTR WINAPI OCharPrevW(LPCWSTR lpszStart, LPCWSTR lpszCurrent);
BOOL WINAPI OCharToOemW(LPCWSTR lpszSrc, LPSTR lpszDst);
LPWSTR WINAPI OCharUpperW(LPWSTR lpsz);
BOOL WINAPI OCopyFileW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, BOOL bFailIfExists);
BOOL WINAPI OCreateDirectoryW(LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
HDC WINAPI OCreateDCW(LPCWSTR lpszDriver, LPCWSTR lpszDevice, LPCWSTR lpszOutput, CONST DEVMODEW *lpInitData);
BOOL WINAPI OCreateDirectoryExW(LPCWSTR lpTemplateDirectory, LPCWSTR lpNewDirectory, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
HDC WINAPI OCreateEnhMetaFileW(HDC hdc, LPCWSTR lpFileName, CONST RECT *lpRect, LPCWSTR lpDescription);
HANDLE WINAPI OCreateEventW(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName);
HANDLE WINAPI OCreateFileW(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
HFONT WINAPI OCreateFontIndirectW(CONST LOGFONTW * plfw);
HFONT OCreateFontW(int nHeight, int nWidth, int nEscapement, int nOrientation, int fnWeight, DWORD fdwItalic, DWORD fdwUnderline, DWORD fdwStrikeOut, DWORD fdwCharSet, DWORD fdwOutputPrecision, DWORD fdwClipPrecision, DWORD fdwQuality, DWORD fdwPitchAndFamily, LPCWSTR lpszFace);
HWND WINAPI OCreateMDIWindowW(LPWSTR lpClassName, LPWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HINSTANCE hInstance, LPARAM lParam);
HDC WINAPI OCreateMetaFileW(LPCWSTR lpstr);
HANDLE WINAPI OCreateSemaphoreW(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCWSTR lpName);
HWND WINAPI OCreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
HSZ WINAPI ODdeCreateStringHandleW(DWORD idInst, LPCWSTR psz, int iCodePage);
UINT WINAPI ODdeInitializeW(LPDWORD pidInst, PFNCALLBACK pfnCallback, DWORD afCmd, DWORD ulRes);
LRESULT WINAPI ODefFrameProcW(HWND hWnd, HWND hWndMDIClient, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI ODefMDIChildProcW(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI ODefWindowProcW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL WINAPI ODeleteFileW(LPCWSTR pwsz);
LRESULT WINAPI ODialogBoxIndirectParamW(HINSTANCE hInstance, LPCDLGTEMPLATEW hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
LRESULT WINAPI ODialogBoxParamW(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
LRESULT WINAPI ODispatchMessageW(CONST MSG *lpMsg);
int WINAPI ODrawTextW(HDC hDC, LPCWSTR lpString, int nCount, LPRECT lpRect, UINT uFormat);
int WINAPI ODrawTextExW(HDC hdc, LPWSTR pwsz, int cb, LPRECT lprect, UINT dwDTFormat, LPDRAWTEXTPARAMS lpDTParams);
DWORD WINAPI OExpandEnvironmentStringsW(LPCWSTR lpSrc, LPWSTR lpDst, DWORD nSize);
VOID WINAPI OFatalAppExitW(UINT uAction, LPCWSTR lpMessageText);
HANDLE WINAPI OFindFirstChangeNotificationW(LPCWSTR lpPathName, BOOL bWatchSubtree, DWORD dwNotifyFilter);
HANDLE WINAPI OFindFirstFileW(LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData);
BOOL WINAPI OFindNextFileW(HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData);
HRSRC WINAPI OFindResourceW(HINSTANCE hModule, LPCWSTR lpName, LPCWSTR lpType);
HWND WINAPI OFindWindowW(LPCWSTR lpClassName, LPCWSTR lpWindowName);
DWORD WINAPI OFormatMessageW(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId, DWORD dwLanguageId, LPWSTR lpBuffer, DWORD nSize, va_list *Arguments);
BOOL APIENTRY OGetCharABCWidthsFloatW(HDC hdc, UINT uFirstChar, UINT uLastChar, LPABCFLOAT lpABC);
BOOL APIENTRY OGetCharABCWidthsW(HDC hdc, UINT uFirstChar, UINT uLastChar, LPABC lpABC);
BOOL APIENTRY OGetCharWidthFloatW(HDC hdc, UINT iFirstChar, UINT iLastChar, PFLOAT pBuffer);
BOOL WINAPI OGetCharWidthW(HDC hdc, UINT iFirstChar, UINT iLastChar, LPINT lpBuffer);
BOOL WINAPI OGetClassInfoW(HINSTANCE hInstance, LPCWSTR lpClassName, LPWNDCLASSW lpWndClass);
BOOL WINAPI OGetClassInfoExW(HINSTANCE hInstance, LPCWSTR lpClassName, LPWNDCLASSEXW lpWndClass);
DWORD WINAPI OGetClassLongW(HWND hWnd, int nIndex);
DWORD WINAPI OSetClassLongW(HWND hWnd, int nIndex, LONG dwNewLong);
int WINAPI OGetClassNameW(HWND hWnd, LPWSTR lpClassName, int nMaxCount);
DWORD WINAPI OGetCurrentDirectoryW(DWORD nBufferLength, LPWSTR lpBuffer);
UINT WINAPI OGetDlgItemTextW(HWND hDlg, int nIDDlgItem, LPWSTR lpString, int nMaxCount);
DWORD WINAPI OGetFileAttributesW(LPCWSTR lpFileName);
DWORD WINAPI OGetFullPathNameW(LPCWSTR lpFileName, DWORD nBufferLength, LPWSTR lpBuffer, LPWSTR *lpFilePart);
DWORD WINAPI OGetGlyphOutlineW(HDC hdc, UINT uChar, UINT uFormat, LPGLYPHMETRICS lpgm, DWORD cbBuffer, LPVOID lpvBuffer, CONST MAT2 *lpmat2);
DWORD WINAPI OGetKerningPairsW(HDC hdc, DWORD nNumPairs, LPKERNINGPAIR lpkrnpair);
BOOL WINAPI OGetMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
DWORD WINAPI OGetModuleFileNameW(HINSTANCE hModule, LPWSTR pwszFilename, DWORD nSize);
HMODULE WINAPI OGetModuleHandleW(LPCWSTR lpModuleName);
UINT APIENTRY OGetOutlineTextMetricsW(HDC hdc, UINT cbData, LPOUTLINETEXTMETRICW lpOTM);
UINT WINAPI OGetPrivateProfileIntW(LPCWSTR lpAppName, LPCWSTR lpKeyName, INT nDefault, LPCWSTR lpFileName);
DWORD WINAPI OGetPrivateProfileStringW(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpDefault, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName);
int WINAPI OGetObjectW(HGDIOBJ hgdiobj, int cbBuffer, LPVOID lpvObject);
BOOL WINAPI OGetOpenFileNameW(LPOPENFILENAMEW lpofn);
UINT WINAPI OGetProfileIntW(LPCWSTR lpAppName, LPCWSTR lpKeyName, INT nDefault);
HANDLE WINAPI OGetPropW(HWND hWnd, LPCWSTR lpString);
BOOL WINAPI OGetSaveFileNameW(LPOPENFILENAMEW lpofn);
DWORD WINAPI OGetTabbedTextExtentW(HDC hDC, LPCWSTR lpString, int nCount, int nTabPositions, LPINT lpnTabStopPositions);
UINT WINAPI OGetTempFileNameW(LPCWSTR lpPathName, LPCWSTR lpPrefixString, UINT uUnique, LPWSTR lpTempFileName);
DWORD WINAPI OGetTempPathW(DWORD nBufferLength, LPWSTR lpBuffer);
BOOL APIENTRY OGetTextExtentPoint32W(HDC hdc, LPCWSTR pwsz, int cb, LPSIZE pSize);
BOOL APIENTRY OGetTextExtentPointW(HDC hdc, LPCWSTR pwsz, int cb, LPSIZE pSize);
BOOL APIENTRY OGetTextExtentExPointW(HDC hdc, LPCWSTR lpszStr, int cchString, int nMaxExtent, LPINT lpnFit, LPINT alpDx, LPSIZE pSize);
LONG WINAPI OGetWindowLongW(HWND hWnd, int nIndex);
BOOL WINAPI OGetTextMetricsW(HDC hdc, LPTEXTMETRICW lptm);
BOOL WINAPI OGetUserNameW(LPWSTR lpBuffer, LPDWORD nSize);
BOOL WINAPI OGetVolumeInformationW(LPCWSTR lpRootPathName, LPWSTR lpVolumeNameBuffer, DWORD nVolumeNameSize, LPDWORD lpVolumeSerialNumber, LPDWORD lpMaximumComponentLength, LPDWORD lpFileSystemFlags, LPWSTR lpFileSystemNameBuffer, DWORD nFileSystemNameSize);
int WINAPI OGetWindowTextLengthW(HWND hWnd);
int WINAPI OGetWindowTextW(HWND hWnd, LPWSTR lpString, int nMaxCount);
ATOM WINAPI OGlobalAddAtomW(LPCWSTR lpString);
UINT WINAPI OGlobalGetAtomNameW(ATOM nAtom, LPWSTR lpBuffer, int nSize);
BOOL WINAPI OGrayStringW(HDC hDC, HBRUSH hBrush, GRAYSTRINGPROC lpOutputFunc, LPARAM lpData, int nCount, int X, int Y, int nWidth, int nHeight);
BOOL WINAPI OInsertMenuW(HMENU hMenu, UINT uPosition, UINT uFlags, UINT uIDNewItem, LPCWSTR lpNewItem);
BOOL WINAPI OIsBadStringPtrW(LPCWSTR lpsz, UINT ucchMax);
BOOL WINAPI OIsCharAlphaNumericW(WCHAR wch);
BOOL WINAPI OIsCharAlphaW(WCHAR wch);
BOOL WINAPI OIsDialogMessageW(HWND hDlg, LPMSG lpMsg);
int WINAPI OLCMapStringW(LCID Locale, DWORD dwMapFlags, LPCWSTR lpSrcStr, int cchSrc, LPWSTR lpDestStr, int cchDest);
HACCEL WINAPI OLoadAcceleratorsW(HINSTANCE hInst, LPCWSTR lpTableName);
HBITMAP WINAPI OLoadBitmapW(HINSTANCE hInstance, LPCWSTR lpBitmapName);
HCURSOR WINAPI OLoadCursorW(HINSTANCE hInstance, LPCWSTR lpCursorName);
HICON WINAPI OLoadIconW(HINSTANCE hInstance, LPCWSTR lpIconName);
HINSTANCE WINAPI OLoadLibraryW(LPCWSTR pwszFileName);
HMODULE WINAPI OLoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
HMENU WINAPI OLoadMenuIndirectW(CONST MENUTEMPLATEW *lpMenuTemplate);
HMENU WINAPI OLoadMenuW(HINSTANCE hInstance, LPCWSTR lpMenuName);
int WINAPI OLoadStringW(HINSTANCE hInstance, UINT uID, LPWSTR lpBuffer, int nBufferMax);
LPWSTR WINAPI OlstrcatW(LPWSTR lpString1, LPCWSTR lpString2);
int WINAPI OlstrcmpiW(LPCWSTR lpString1, LPCWSTR lpString2);
int WINAPI OlstrcmpW(LPCWSTR lpString1, LPCWSTR lpString2);
LPWSTR WINAPI OlstrcpyW(LPWSTR lpString1, LPCWSTR lpString2);
LPWSTR WINAPI OlstrcpynW(LPWSTR lpString1, LPCWSTR lpString2, int iMaxLength);
int WINAPI OlstrlenW(LPCWSTR lpString);
UINT WINAPI OMapVirtualKeyW(UINT uCode, UINT uMapType);
int WINAPI OMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType);
int WINAPI OMessageBoxIndirectW(LPMSGBOXPARAMSW lpmbp);
BOOL WINAPI OModifyMenuW(HMENU hMnu, UINT uPosition, UINT uFlags, UINT uIDNewItem, LPCWSTR lpNewItem);
BOOL WINAPI OMoveFileExW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, DWORD dwFlags);
BOOL WINAPI OMoveFileW(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName);
HANDLE WINAPI OLoadImageW(HINSTANCE hinst, LPCWSTR lpszName, UINT uType, int cxDesired, int cyDesired, UINT fuLoad);
BOOL WINAPI OOemToCharW(LPCSTR lpszSrc, LPWSTR lpszDst);
VOID WINAPI OOutputDebugStringW(LPCWSTR lpOutputString);
BOOL WINAPI OPeekMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
BOOL WINAPI OPostMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL WINAPI OPostThreadMessageW(DWORD idThread, UINT Msg, WPARAM wParam, LPARAM lParam);
LONG APIENTRY ORegCreateKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);
LONG APIENTRY ORegCreateKeyW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult);
LONG APIENTRY ORegEnumKeyW(HKEY hKey, DWORD dwIndex, LPWSTR lpName, DWORD cbName);
LONG APIENTRY ORegEnumValueW(HKEY hKey, DWORD dwIndex, LPWSTR lpValueName, LPDWORD lpcbValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
LONG APIENTRY ORegOpenKeyW(HKEY hKey, LPCWSTR pwszSubKey, PHKEY phkResult);
LONG APIENTRY ORegDeleteKeyW(HKEY hKey, LPCWSTR pwszSubKey);
LONG APIENTRY ORegDeleteValueW(HKEY hKey, LPWSTR lpValueName);
ATOM WINAPI ORegisterClassW(CONST WNDCLASSW *lpWndClass);
ATOM WINAPI ORegisterClassExW(CONST WNDCLASSEXW * lpWndClass);
BOOL WINAPI OUnregisterClassW(LPCTSTR  lpClassName, HINSTANCE  hInstance);
UINT WINAPI ORegisterClipboardFormatW(LPCWSTR lpszFormat);
UINT WINAPI ORegisterWindowMessageW(LPCWSTR lpString);
LONG APIENTRY ORegOpenKeyExW(HKEY hKey, LPCTSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
LONG APIENTRY ORegQueryInfoKeyW(HKEY hKey, LPWSTR lpClass, LPDWORD lpcbClass, LPDWORD lpReserved, LPDWORD lpcSubKeys, LPDWORD lpcbMaxSubKeyLen, LPDWORD lpcbMaxClassLen, LPDWORD lpcValues, LPDWORD lpcbMaxValueNameLen, LPDWORD lpcbMaxValueLen, LPDWORD lpcbSecurityDescriptor, PFILETIME lpftLastWriteTime);
LONG APIENTRY ORegQueryValueW(HKEY hKey, LPCWSTR pwszSubKey, LPWSTR pwszValue, PLONG lpcbValue);
LONG APIENTRY ORegSetValueExW(HKEY hKey, LPCWSTR lpValueName, DWORD Reserved, DWORD dwType, CONST BYTE* lpData, DWORD cbData);
LONG APIENTRY ORegSetValueW(HKEY hKey, LPCWSTR lpSubKey, DWORD dwType, LPCWSTR lpData, DWORD cbData);
LONG APIENTRY ORegQueryValueExW(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
HANDLE WINAPI ORemovePropW(HWND hWnd, LPCWSTR lpString);
LRESULT WINAPI OSendDlgItemMessageW(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI OSendMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL WINAPI OSendNotifyMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL WINAPI OSetDlgItemTextW(HWND hDlg, int nIDDlgItem, LPCWSTR lpString);
BOOL WINAPI OSetFileAttributesW(LPCWSTR lpFileName, DWORD dwFileAttributes);
BOOL WINAPI OSetPropW(HWND hWnd, LPCWSTR lpString, HANDLE hData);
BOOL WINAPI OSetMenuItemInfoW(HMENU hMenu, UINT uItem, BOOL fByPosition, LPCMENUITEMINFOW lpcmii);
LONG WINAPI OSetWindowLongW(HWND hWnd, int nIndex, LONG dwNewLong); 
HHOOK WINAPI OSetWindowsHookExW(int idHook, HOOKPROC lpfn, HINSTANCE hmod, DWORD dwThreadId);
BOOL WINAPI OSetWindowTextW(HWND hWnd, LPCWSTR lpString);
LONG WINAPI OTabbedTextOutW(HDC hDC, int X, int Y, LPCWSTR lpString, int nCount, int nTabPositions, LPINT lpnTabStopPositions, int nTabOrigin);
// FOR OLE CTL: THIS MAGLES INTERFACE MEMBERS BY SAME NAME
//int WINAPI OTranslateAcceleratorW(HWND hWnd, HACCEL hAccTable, LPMSG lpMsg);
SHORT WINAPI OVkKeyScanW(WCHAR ch);
BOOL WINAPI OWinHelpW(HWND hWndMain, LPCWSTR lpszHelp, UINT uCommand, DWORD dwData);
BOOL WINAPI OWritePrivateProfileStringW(LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpString, LPCWSTR lpFileName);
int WINAPIV OwsprintfW(LPWSTR pwszOut, LPCWSTR pwszFormat, ...);
BOOL WINAPI OGetVersionExW(LPOSVERSIONINFOW lpVersionInformation);
LONG APIENTRY ORegEnumKeyExW(HKEY hKey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcbName, LPDWORD lpReserved, LPWSTR lpClass, LPDWORD lpcbClass, PFILETIME lpftLastWriteTime);
HANDLE WINAPI OCreateFileMappingW(HANDLE hFile, LPSECURITY_ATTRIBUTES lpFileMappingAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCWSTR lpName);
LRESULT WINAPI ODefDlgProcW(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
int WINAPI OGetLocaleInfoW(LCID Locale, LCTYPE LCType, LPWSTR lpLCData, int cchData);
BOOL WINAPI OSetLocaleInfoW(LCID Locale, LCTYPE LCType, LPCWSTR lpLCData);
HRESULT WINAPI OStgCreateDocfile(const WCHAR * pwcsName, DWORD grfMode, DWORD reserved, IStorage ** ppstgOpen);
int WINAPI OStartDocW(HDC hDC, CONST DOCINFOW * pdiDocW);
BOOL WINAPI OSystemParametersInfoW(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni);
LPWSTR WINAPI OCharNextW(LPCWSTR lpsz);

// Added by VK -- 12/1/98
HRESULT __stdcall OURLOpenBlockingStreamW(LPUNKNOWN pCaller, LPCWSTR wszURL, LPSTREAM *ppStream, DWORD dwReserved, LPBINDSTATUSCALLBACK lpfnCB);
BOOL WINAPI OInternetCreateUrlW(LPURL_COMPONENTSW lpUrlComponents, DWORD dwFlags, LPWSTR lpwszUrl, LPDWORD lpdwUrlLength);
BOOL WINAPI OInternetCrackUrlW(LPCWSTR lpwszUrl, DWORD dwUrlLength, DWORD dwFlags, LPURL_COMPONENTSW lpUrlComponents);
BOOL WINAPI ODeleteUrlCacheEntryW(LPCWSTR lpwszUrlName);

// Added by VK -- 8/10/99
HINTERNET WINAPI OInternetOpenW(LPCWSTR lpszAgent, DWORD dwAccessType, LPCWSTR lpszProxy, LPCWSTR lpszProxyBypass, DWORD dwFlags);
HINTERNET WINAPI OInternetOpenUrlW(HINTERNET hInternet, LPCWSTR lpszUrl, LPCWSTR lpszHeaders, DWORD dwHeadersLength, DWORD dwFlags, DWORD_PTR dwContext);

#ifdef DEBUG
int WINAPI ODlgDirListW(HWND hDlg, LPWSTR lpPathSpec, int nIDListBox, int nIDStaticPath, UINT uFileType);
int WINAPI ODlgDirListComboBoxW(HWND hDlg, LPWSTR lpPathSpec, int nIDComboBox, int nIDStaticPath, UINT uFiletype);
BOOL WINAPI ODlgDirSelectComboBoxExW(HWND hDlg, LPWSTR lpString, int nCount, int nIDComboBox);
BOOL WINAPI ODlgDirSelectExW(HWND hDlg, LPWSTR lpString, int nCount, int nIDListBox);
#endif

} // extern "C"

#define AppendMenuW OAppendMenuW
#define CallWindowProcW OCallWindowProcW
#define CharLowerBuffW OCharLowerBuffW
#define CharLowerW OCharLowerW
#define CharNextW OCharNextW
#define CharPrevW OCharPrevW
#define CharToOemW OCharToOemW
#define CharUpperW OCharUpperW
#define CopyFileW OCopyFileW
#define CreateDCW OCreateDCW
#define CreateDirectoryExW OCreateDirectoryExW
#define CreateDirectoryW OCreateDirectoryW
#define CreateEnhMetaFileW OCreateEnhMetaFileW
#define CreateEventW OCreateEventW
#define CreateFileMappingW OCreateFileMappingW
#define CreateFileW OCreateFileW
#define CreateFontIndirectW OCreateFontIndirectW
#define CreateFontW OCreateFontW
#define CreateMDIWindowW OCreateMDIWindowW
#define CreateMetaFileW OCreateMetaFileW
#define CreateSemaphoreW OCreateSemaphoreW
#define CreateWindowExW OCreateWindowExW
#define DdeCreateStringHandleW ODdeCreateStringHandleW
#define DdeInitializeW ODdeInitializeW
#define DefFrameProcW ODefFrameProcW
#define DefMDIChildProcW ODefMDIChildProcW
#define DefWindowProcW ODefWindowProcW
#define DeleteFileW ODeleteFileW
#define DialogBoxIndirectParamW ODialogBoxIndirectParamW
#define DialogBoxParamW ODialogBoxParamW
#define DispatchMessageW ODispatchMessageW
#define DrawTextExW ODrawTextExW
#define DrawTextW ODrawTextW
#define ExpandEnvironmentStringsW OExpandEnvironmentStringsW
#define FatalAppExitW OFatalAppExitW
#define FindFirstChangeNotificationW OFindFirstChangeNotificationW
#define FindFirstFileW OFindFirstFileW
#define FindNextFileW OFindNextFileW
#define FindResourceW OFindResourceW
#define FindWindowW OFindWindowW
#define FormatMessageW OFormatMessageW
#define GetCharABCWidthsFloatW OGetCharABCWidthsFloatW
#define GetCharABCWidthsW OGetCharABCWidthsW
#define GetCharWidthFloatW OGetCharWidthFloatW
#define GetCharWidthW OGetCharWidthW
#define GetClassInfoW OGetClassInfoW
#define GetClassInfoExW OGetClassInfoExW
#define GetClassLongW OGetClassLongW
#define GetClassNameW OGetClassNameW
#define GetCurrentDirectoryW OGetCurrentDirectoryW
#define GetDlgItemTextW OGetDlgItemTextW
#define GetFileAttributesW OGetFileAttributesW
#define GetFullPathNameW OGetFullPathNameW
#define GetGlyphOutlineW OGetGlyphOutlineW
#define GetKerningPairsW OGetKerningPairsW
#define GetLocaleInfoW OGetLocaleInfoW
#define GetMessageW OGetMessageW
#define GetModuleFileNameW OGetModuleFileNameW
#define GetModuleHandleW OGetModuleHandleW
#define GetObjectW OGetObjectW
#define GetOpenFileNameW OGetOpenFileNameW
#define GetOutlineTextMetricsW OGetOutlineTextMetricsW
#define GetPrivateProfileIntW OGetPrivateProfileIntW
#define GetPrivateProfileStringW OGetPrivateProfileStringW
#define GetProfileIntW OGetProfileIntW
#define GetPropW OGetPropW
#define GetSaveFileNameW OGetSaveFileNameW
#define GetTabbedTextExtentW OGetTabbedTextExtentW
#define GetTempFileNameW OGetTempFileNameW
#define GetTempPathW OGetTempPathW
#define GetTextExtentPoint32W OGetTextExtentPoint32W
#define GetTextExtentPointW OGetTextExtentPointW
#define GetTextExtentExPointW OGetTextExtentExPointW
#define GetTextMetricsW OGetTextMetricsW
#define GetUserNameW OGetUserNameW
#define GetVersionExW OGetVersionExW
#define GetVolumeInformationW OGetVolumeInformationW
#define GetWindowLongW OGetWindowLongW
#define GetWindowTextLengthW OGetWindowTextLengthW
#define GetWindowTextW OGetWindowTextW
#define GlobalAddAtomW OGlobalAddAtomW
#define GlobalGetAtomNameW OGlobalGetAtomNameW
#define GrayStringW OGrayStringW
#define InsertMenuW OInsertMenuW
#define IsBadStringPtrW OIsBadStringPtrW
#define IsCharAlphaNumericW OIsCharAlphaNumericW
#define IsCharAlphaW OIsCharAlphaW
#define IsDialogMessageW OIsDialogMessageW
#define LCMapStringW OLCMapStringW
#define LoadAcceleratorsW OLoadAcceleratorsW
#define LoadBitmapW OLoadBitmapW
#define LoadCursorW OLoadCursorW
#define LoadIconW OLoadIconW
#define LoadImageW OLoadImageW
#define LoadLibraryExW OLoadLibraryExW
#define LoadLibraryW OLoadLibraryW
#define LoadMenuIndirectW OLoadMenuIndirectW
#define LoadMenuW OLoadMenuW
#define LoadStringW OLoadStringW
#define lstrcatW OlstrcatW
#define lstrcmpiW OlstrcmpiW
#define lstrcmpW OlstrcmpW
#define lstrcpynW OlstrcpynW
#define lstrcpyW OlstrcpyW
#define lstrlenW OlstrlenW
#define MapVirtualKeyW OMapVirtualKeyW
#define MessageBoxW OMessageBoxW
#define MessageBoxIndirectW OMessageBoxIndirectW
#define ModifyMenuW OModifyMenuW
#define MoveFileExW OMoveFileExW
#define MoveFileW OMoveFileW
#define OemToCharW OOemToCharW
#define OutputDebugStringW OOutputDebugStringW
#define PeekMessageW OPeekMessageW
#define PostMessageW OPostMessageW
#define PostThreadMessageW OPostThreadMessageW
#define RegCreateKeyExW ORegCreateKeyExW
#define RegCreateKeyW ORegCreateKeyW
#define RegDeleteKeyW ORegDeleteKeyW
#define RegDeleteValueW ORegDeleteValueW
#define RegEnumKeyW ORegEnumKeyW
#define RegEnumValueW ORegEnumValueW
#define RegEnumKeyExW ORegEnumKeyExW
#define RegisterClassW ORegisterClassW
#define RegisterClassExW ORegisterClassExW
#define RegisterClipboardFormatW ORegisterClipboardFormatW
#define RegisterWindowMessageW ORegisterWindowMessageW
#define RegOpenKeyExW ORegOpenKeyExW
#define RegOpenKeyW ORegOpenKeyW
#define RegQueryInfoKeyW ORegQueryInfoKeyW
#define RegQueryValueExW ORegQueryValueExW
#define RegQueryValueW ORegQueryValueW
#define RegSetValueExW ORegSetValueExW
#define RegSetValueW ORegSetValueW
#define RemovePropW ORemovePropW
#define SendDlgItemMessageW OSendDlgItemMessageW
#define SendMessageW OSendMessageW
#define SendNotifyMessageW OSendNotifyMessageW
#define SetDlgItemTextW OSetDlgItemTextW
#define SetFileAttributesW OSetFileAttributesW
#define SetLocaleInfoW OSetLocaleInfoW
#define SetMenuItemInfoW OSetMenuItemInfoW
#define SetPropW OSetPropW
#define SetWindowLongW OSetWindowLongW
#define SetWindowsHookExW OSetWindowsHookExW
#define SetWindowTextW OSetWindowTextW
#define StartDocW OStartDocW
#define StgCreateDocfile OStgCreateDocfile
#define SystemParametersInfoW OSystemParametersInfoW
#define TabbedTextOutW OTabbedTextOutW
// #define TranslateAcceleratorW OTranslateAcceleratorW	FOR OLE CTL: THIS MAGLES INTERFACE MEMBERS BY SAME NAME
#define UnregisterClassW OUnregisterClassW
#define VkKeyScanW OVkKeyScanW
#define WinHelpW OWinHelpW
#define WritePrivateProfileStringW OWritePrivateProfileStringW
#define wsprintfW OwsprintfW

// Added by VK -- 12/1/98
#define URLOpenBlockingStreamW OURLOpenBlockingStreamW
#define InternetCreateUrlW OInternetCreateUrlW
#define InternetCrackUrlW OInternetCrackUrlW
#define DeleteUrlCacheEntryW ODeleteUrlCacheEntryW

// Added by VK -- 8/10/99
#define InternetOpenW OInternetOpenW
#define InternetOpenUrlW OInternetOpenUrlW


// These are the currently unsupported APIs
// These will assert in the debug version and map directly 
// to Windows in the retail version
#ifdef DEBUG
#define AbortSystemShutdownW OAbortSystemShutdownW
#define AccessCheckAndAuditAlarmW OAccessCheckAndAuditAlarmW
#define AddFontResourceW OAddFontResourceW
#define AddFormW OAddFormW
#define AddJobW OAddJobW
#define AddMonitorW OAddMonitorW
#define AddPortW OAddPortW
#define AddPrinterConnectionW OAddPrinterConnectionW
#define AddPrinterDriverW OAddPrinterDriverW
#define AddPrinterW OAddPrinterW
#define AddPrintProcessorW OAddPrintProcessorW
#define AddPrintProvidorW OAddPrintProvidorW
#define AdvancedDocumentPropertiesW OAdvancedDocumentPropertiesW
#define auxGetDevCapsW OauxGetDevCapsW
#define BackupEventLogW OBackupEventLogW
#define BeginUpdateResourceW OBeginUpdateResourceW
#define BuildCommDCBAndTimeoutsW OBuildCommDCBAndTimeoutsW
#define BuildCommDCBW OBuildCommDCBW
#define CallMsgFilterW OCallMsgFilterW
#define CallNamedPipeW OCallNamedPipeW
#define ChangeDisplaySettingsW OChangeDisplaySettingsW
#define ChangeMenuW OChangeMenuW
#define CharToOemBuffW OCharToOemBuffW
#define CharUpperBuffW OCharUpperBuffW
#define ChooseColorW OChooseColorW
#define ChooseFontW OChooseFontW
#define ClearEventLogW OClearEventLogW
#define CommConfigDialogW OCommConfigDialogW
#define CompareStringW OCompareStringW
#define ConfigurePortW OConfigurePortW
#define CopyAcceleratorTableW OCopyAcceleratorTableW
#define CopyEnhMetaFileW OCopyEnhMetaFileW
#define CopyMetaFileW OCopyMetaFileW
#define CreateAcceleratorTableW OCreateAcceleratorTableW
#define CreateColorSpaceW OCreateColorSpaceW
#define CreateDesktopW OCreateDesktopW
#define CreateDialogIndirectParamW OCreateDialogIndirectParamW
#define CreateDialogParamW OCreateDialogParamW
#define CreateICW OCreateICW
#define CreateMailslotW OCreateMailslotW
#define CreateMutexW OCreateMutexW
#define CreateNamedPipeW OCreateNamedPipeW
#define CreateProcessW OCreateProcessW
#define CreateProcessAsUserW OCreateProcessAsUserW
#define CreatePropertySheetPageW OCreatePropertySheetPageW
#define CreateScalableFontResourceW OCreateScalableFontResourceW
#define CreateStatusWindowW OCreateStatusWindowW
#define CreateWindowStationW OCreateWindowStationW
#define DceErrorInqTextW ODceErrorInqTextW
#define DdeQueryStringW   ODdeQueryStringW
#define DefDlgProcW ODefDlgProcW
#define DefineDosDeviceW ODefineDosDeviceW
#define DeleteFormW ODeleteFormW
#define DeleteMonitorW ODeleteMonitorW
#define DeletePortW ODeletePortW
#define DeletePrinterConnectionW ODeletePrinterConnectionW
#define DeletePrinterDriverW ODeletePrinterDriverW
#define DeletePrintProcessorW ODeletePrintProcessorW
#define DeletePrintProvidorW ODeletePrintProvidorW
#define DeviceCapabilitiesW ODeviceCapabilitiesW
#define DlgDirListComboBoxW ODlgDirListComboBoxW
#define DlgDirListW ODlgDirListW
#define DlgDirSelectComboBoxExW ODlgDirSelectComboBoxExW
#define DlgDirSelectExW ODlgDirSelectExW
#define DocumentPropertiesW ODocumentPropertiesW
#define DoEnvironmentSubstW ODoEnvironmentSubstW
#define DragQueryFileW ODragQueryFileW
#define DrawStateW ODrawStateW
#define EndUpdateResourceW OEndUpdateResourceW
#define EnumCalendarInfoW OEnumCalendarInfoW
#define EnumDateFormatsW OEnumDateFormatsW
#define EnumDesktopsW OEnumDesktopsW
#define EnumDisplaySettingsW OEnumDisplaySettingsW
#define EnumFontFamiliesExW OEnumFontFamiliesExW
#define EnumFontFamiliesW OEnumFontFamiliesW
#define EnumFontsW OEnumFontsW
#define EnumFormsW OEnumFormsW
#define EnumICMProfilesW OEnumICMProfilesW
#define EnumJobsW OEnumJobsW
#define EnumMonitorsW OEnumMonitorsW
#define EnumPortsW OEnumPortsW
#define EnumPrinterDriversW OEnumPrinterDriversW
#define EnumPrintersW OEnumPrintersW
#define EnumPrintProcessorDatatypesW OEnumPrintProcessorDatatypesW
#define EnumPrintProcessorsW OEnumPrintProcessorsW
#define EnumPropsExW OEnumPropsExW
#define EnumPropsW OEnumPropsW
#define EnumProtocolsW OEnumProtocolsW
#define EnumResourceLanguagesW OEnumResourceLanguagesW
#define EnumResourceNamesW OEnumResourceNamesW
#define EnumResourceTypesW OEnumResourceTypesW
#define EnumSystemCodePagesW OEnumSystemCodePagesW
#define EnumSystemLocalesW OEnumSystemLocalesW
#define EnumTimeFormatsW OEnumTimeFormatsW
#define EnumWindowStationsW OEnumWindowStationsW
#define ExtractAssociatedIconW OExtractAssociatedIconW
#define ExtractIconW OExtractIconW
#define ExtractIconExW OExtractIconExW
#define FillConsoleOutputCharacterW OFillConsoleOutputCharacterW
#define FindEnvironmentStringW OFindEnvironmentStringW
#define FindExecutableW OFindExecutableW
#define FindResourceExW OFindResourceExW
#define FindTextW OFindTextW
#define FindWindowExW OFindWindowExW
#define FoldStringW OFoldStringW
#define GetBinaryTypeW OGetBinaryTypeW
#define GetCharacterPlacementW OGetCharacterPlacementW
#define GetCharWidth32W OGetCharWidth32W
#define GetCommandLineW OGetCommandLineW
#define GetClipboardFormatNameW OGetClipboardFormatNameW
#define GetCompressedFileSizeW OGetCompressedFileSizeW
#define GetComputerNameW OGetComputerNameW
#define GetConsoleTitleW OGetConsoleTitleW
#define GetCurrencyFormatW OGetCurrencyFormatW
#define GetDateFormatW OGetDateFormatW
#define GetDefaultCommConfigW OGetDefaultCommConfigW
#define GetDiskFreeSpaceW OGetDiskFreeSpaceW
#define GetDriveTypeW OGetDriveTypeW
#define GetEnhMetaFileDescriptionW OGetEnhMetaFileDescriptionW
#define GetEnhMetaFileW OGetEnhMetaFileW
#define GetEnvironmentVariableW OGetEnvironmentVariableW
#define GetExpandedNameW OGetExpandedNameW
#define GetFileSecurityW OGetFileSecurityW
#define GetFileTitleW OGetFileTitleW
#define GetFileVersionInfoW OGetFileVersionInfoW
#define GetFileVersionInfoSizeW OGetFileVersionInfoSizeW
#define GetFormW OGetFormW
#define GetICMProfileW OGetICMProfileW
#define GetJobW OGetJobW
#define GetKeyboardLayoutNameW OGetKeyboardLayoutNameW
#define GetKeyNameTextW OGetKeyNameTextW
#define GetLogColorSpaceW OGetLogColorSpaceW
#define GetLogicalDriveStringsW OGetLogicalDriveStringsW
#define GetMenuItemInfoW OGetMenuItemInfoW
#define GetMenuStringW OGetMenuStringW
#define GetMetaFileW OGetMetaFileW
#define GetNameByTypeW OGetNameByTypeW
#define GetNamedPipeHandleStateW OGetNamedPipeHandleStateW
#define GetNumberFormatW OGetNumberFormatW
#define GetPrinterW OGetPrinterW
#define GetPrinterDataW OGetPrinterDataW
#define GetPrinterDriverDirectoryW OGetPrinterDriverDirectoryW
#define GetPrinterDriverW OGetPrinterDriverW
#define GetPrintProcessorDirectoryW OGetPrintProcessorDirectoryW
#define GetPrivateProfileSectionNamesW OGetPrivateProfileSectionNamesW
#define GetPrivateProfileSectionW OGetPrivateProfileSectionW
#define GetPrivateProfileStructW OGetPrivateProfileStructW
#define GetProfileSectionW OGetProfileSectionW
#define GetProfileStringW OGetProfileStringW
#define GetShortPathNameW OGetShortPathNameW
#define GetStartupInfoW OGetStartupInfoW
#define GetStringTypeExW OGetStringTypeExW
#define GetSystemDirectoryW OGetSystemDirectoryW
#define GetTextFaceW OGetTextFaceW
#define GetTimeFormatW OGetTimeFormatW
#define GetTypeByNameW OGetTypeByNameW
#define GetUserObjectInformationW OGetUserObjectInformationW
#define GetWindowsDirectoryW OGetWindowsDirectoryW
#define GlobalFindAtomW OGlobalFindAtomW
#define ImageList_LoadImageW OImageList_LoadImageW
#define ImmConfigureIMEW OImmConfigureIMEW
#define ImmEnumRegisterWordW OImmEnumRegisterWordW
#define ImmEscapeW OImmEscapeW
#define ImmGetCandidateListCountW OImmGetCandidateListCountW
#define ImmGetCandidateListW OImmGetCandidateListW
#define ImmGetCompositionFontW OImmGetCompositionFontW
#define ImmGetCompositionStringW OImmGetCompositionStringW
#define ImmGetConversionListW OImmGetConversionListW
#define ImmGetDescriptionW OImmGetDescriptionW
#define ImmGetGuideLineW OImmGetGuideLineW
#define ImmGetIMEFileNameW OImmGetIMEFileNameW
#define ImmGetRegisterWordStyleW OImmGetRegisterWordStyleW
#define ImmInstallIMEW OImmInstallIMEW
#define ImmIsUIMessageW OImmIsUIMessageW
#define ImmRegisterWordW OImmRegisterWordW
#define ImmSetCompositionFontW OImmSetCompositionFontW
#define ImmSetCompositionStringW OImmSetCompositionStringW
#define ImmUnregisterWordW OImmUnregisterWordW
#define InitiateSystemShutdownW OInitiateSystemShutdownW
#define InsertMenuItemW OInsertMenuItemW
#define IsCharLowerW OIsCharLowerW
#define IsCharUpperW OIsCharUpperW
#define I_RpcServerUnregisterEndpointW OI_RpcServerUnregisterEndpointW
#define joyGetDevCapsW OjoyGetDevCapsW
#define LoadCursorFromFileW OLoadCursorFromFileW
#define LoadKeyboardLayoutW OLoadKeyboardLayoutW
#define LogonUserW OLogonUserW
#define LZOpenFileW OLZOpenFileW
#define MapVirtualKeyExW OMapVirtualKeyExW
#define MIMEAssociationDialogW OMIMEAssociationDialogW
#define MultinetGetConnectionPerformanceW OMultinetGetConnectionPerformanceW
#define ObjectCloseAuditAlarmW OObjectCloseAuditAlarmW
#define ObjectOpenAuditAlarmW OObjectOpenAuditAlarmW
#define ObjectPrivilegeAuditAlarmW OObjectPrivilegeAuditAlarmW
#define OemToCharBuffW OOemToCharBuffW
#define OpenBackupEventLogW OOpenBackupEventLogW
#define OpenDesktopW OOpenDesktopW
#define OpenEventLogW OOpenEventLogW
#define OpenEventW OOpenEventW
#define OpenFileMappingW OOpenFileMappingW
#define OpenMutexW OOpenMutexW
#define OpenPrinterW OOpenPrinterW
#define OpenSemaphoreW OOpenSemaphoreW
#define OpenWindowStationW OOpenWindowStationW
#define PageSetupDlgW OPageSetupDlgW
#define PeekConsoleInputW OPeekConsoleInputW
#define PolyTextOutW OPolyTextOutW
#define PrintDlgW OPrintDlgW
#define PrinterMessageBoxW OPrinterMessageBoxW
#define PrivilegedServiceAuditAlarmW OPrivilegedServiceAuditAlarmW
#define PropertySheetW OPropertySheetW
#define QueryDosDeviceW OQueryDosDeviceW
#define ReadConsoleInputW OReadConsoleInputW
#define ReadConsoleOutputCharacterW OReadConsoleOutputCharacterW
#define ReadConsoleOutputW OReadConsoleOutputW
#define ReadConsoleW OReadConsoleW
#define ReadEventLogW OReadEventLogW
#define RegConnectRegistryW ORegConnectRegistryW
#define RegisterEventSourceW ORegisterEventSourceW
#define RegLoadKeyW ORegLoadKeyW
#define RegQueryMultipleValuesW ORegQueryMultipleValuesW
#define RegReplaceKeyW ORegReplaceKeyW
#define RegRestoreKeyW ORegRestoreKeyW
#define RegSaveKeyW ORegSaveKeyW
#define RegUnLoadKeyW ORegUnLoadKeyW
#define RemoveDirectoryW ORemoveDirectoryW
#define RemoveFontResourceW ORemoveFontResourceW
#define ReplaceTextW OReplaceTextW
#define ReportEventW OReportEventW
#define ResetDCW OResetDCW
#define ResetPrinterW OResetPrinterW
#define RpcBindingFromStringBindingW ORpcBindingFromStringBindingW
#define RpcBindingInqAuthClientW ORpcBindingInqAuthClientW
#define RpcBindingToStringBindingW ORpcBindingToStringBindingW
#define RpcEpRegisterNoReplaceW ORpcEpRegisterNoReplaceW
#define RpcMgmtEpEltInqNextW ORpcMgmtEpEltInqNextW
#define RpcMgmtInqServerPrincNameW ORpcMgmtInqServerPrincNameW
#define RpcNetworkInqProtseqsW ORpcNetworkInqProtseqsW
#define RpcNetworkIsProtseqValidW ORpcNetworkIsProtseqValidW
#define RpcNsBindingInqEntryNameW ORpcNsBindingInqEntryNameW
#define RpcProtseqVectorFreeW ORpcProtseqVectorFreeW
#define RpcServerInqDefaultPrincNameW ORpcServerInqDefaultPrincNameW
#define RpcServerUseProtseqEpW ORpcServerUseProtseqEpW
#define RpcServerUseProtseqIfW ORpcServerUseProtseqIfW
#define RpcServerUseProtseqW ORpcServerUseProtseqW
#define RpcStringBindingComposeW ORpcStringBindingComposeW
#define RpcStringBindingParseW ORpcStringBindingParseW
#define RpcStringFreeW ORpcStringFreeW
#define ScrollConsoleScreenBufferW OScrollConsoleScreenBufferW
#define SearchPathW OSearchPathW
#define SendMessageCallbackW OSendMessageCallbackW
#define SendMessageTimeoutW OSendMessageTimeoutW
#define SetClassLongW OSetClassLongW
#define SetComputerNameW OSetComputerNameW
#define SetConsoleTitleW OSetConsoleTitleW
#define SetCurrentDirectoryW OSetCurrentDirectoryW
#define SetDefaultCommConfigW OSetDefaultCommConfigW
#define SetEnvironmentVariableW OSetEnvironmentVariableW
#define SetFileSecurityW OSetFileSecurityW
#define SetFormW OSetFormW
#define SetICMProfileW OSetICMProfileW
#define SetJobW OSetJobW
#define SetPrinterDataW OSetPrinterDataW
#define SetPrinterW OSetPrinterW
#define SetUserObjectInformationW OSetUserObjectInformationW
#define SetVolumeLabelW OSetVolumeLabelW
#define SetWindowsHookW OSetWindowsHookW
#define SHBrowseForFolderW OSHBrowseForFolderW
#define Shell_NotifyIconW OShell_NotifyIconW
#define ShellAboutW OShellAboutW
#define ShellExecuteW OShellExecuteW
#define ShellExecuteExW OShellExecuteExW
#define SHFileOperationW OSHFileOperationW
#define SHGetFileInfoW OSHGetFileInfoW
#define SHGetNewLinkInfoW OSHGetNewLinkInfoW
#define SHGetPathFromIDListW OSHGetPathFromIDListW
#define sndPlaySoundW OsndPlaySoundW
#define StartDocPrinterW OStartDocPrinterW
#define StgCreateDocfile OStgCreateDocfile
#define TranslateURLW OTranslateURLW
#define UpdateICMRegKeyW OUpdateICMRegKeyW
#define URLAssociationDialogW OURLAssociationDialogW
#define UuidFromStringW OUuidFromStringW
#define VerFindFileW OVerFindFileW
#define VerInstallFileW OVerInstallFileW
#define VerLanguageNameW OVerLanguageNameW
#define VerQueryValueW OVerQueryValueW
#define VkKeyScanExW OVkKeyScanExW
#define WaitNamedPipeW OWaitNamedPipeW
#define waveInGetDevCapsW OwaveInGetDevCapsW
#define waveInGetErrorTextW OwaveInGetErrorTextW
#define waveOutGetDevCapsW OwaveOutGetDevCapsW
#define waveOutGetErrorTextW OwaveOutGetErrorTextW
#define wglUseFontBitmapsW OwglUseFontBitmapsW
#define wglUseFontOutlinesW OwglUseFontOutlinesW
#define WinExecErrorW OWinExecErrorW
#define WNetAddConnection2W OWNetAddConnection2W
#define WNetAddConnection3W OWNetAddConnection3W
#define WNetAddConnectionW OWNetAddConnectionW
#define WNetCancelConnection2W OWNetCancelConnection2W
#define WNetCancelConnectionW OWNetCancelConnectionW
#define WNetConnectionDialog1W OWNetConnectionDialog1W
#define WNetDisconnectDialog1W OWNetDisconnectDialog1W
#define WNetEnumResourceW OWNetEnumResourceW
#define WNetGetConnectionW OWNetGetConnectionW
#define WNetGetLastErrorW OWNetGetLastErrorW
#define WNetGetNetworkInformationW OWNetGetNetworkInformationW
#define WNetGetProviderNameW OWNetGetProviderNameW
#define WNetGetUniversalNameW OWNetGetUniversalNameW
#define WNetGetUserW OWNetGetUserW
#define WNetOpenEnumW OWNetOpenEnumW
#define WNetSetConnectionW OWNetSetConnectionW
#define WNetUseConnectionW OWNetUseConnectionW
#define WriteConsoleInputW OWriteConsoleInputW
#define WriteConsoleOutputCharacterW OWriteConsoleOutputCharacterW
#define WriteConsoleOutputW OWriteConsoleOutputW
#define WriteConsoleW OWriteConsoleW
#define WritePrivateProfileSectionW OWritePrivateProfileSectionW
#define WritePrivateProfileStructW OWritePrivateProfileStructW
#define WriteProfileSectionW OWriteProfileSectionW
#define WriteProfileStringW OWriteProfileStringW
#define wvsprintfW OwvsprintfW
#endif // DEBUG

#endif // __WIN95WRP_H__

#else // __WIN95WRP_CPP__

#undef AppendMenuW
#undef CallWindowProcW
#undef CharLowerBuffW
#undef CharLowerW
#undef CharNextW
#undef CharPrevW
#undef CharToOemW
#undef CharUpperW
#undef CopyFileW
#undef CreateDCW
#undef CreateDirectoryExW
#undef CreateDirectoryW
#undef CreateEnhMetaFileW
#undef CreateEventW
#undef CreateFileMappingW
#undef CreateFileW
#undef CreateFontIndirectW
#undef CreateFontW
#undef CreateMDIWindowW
#undef CreateMetaFileW
#undef CreateSemaphoreW
#undef CreateWindowExW
#undef DdeCreateStringHandleW
#undef DdeInitializeW
#undef DefFrameProcW
#undef DefMDIChildProcW
#undef DefWindowProcW
#undef DeleteFileW
#undef DialogBoxIndirectParamW
#undef DialogBoxParamW
#undef DispatchMessageW
#undef DrawTextExW
#undef DrawTextW
#undef ExpandEnvironmentStringsW
#undef FatalAppExitW
#undef FindFirstChangeNotificationW
#undef FindFirstFileW
#undef FindNextFileW
#undef FindResourceW
#undef FindWindowW
#undef FormatMessageW
#undef GetCharABCWidthsFloatW
#undef GetCharABCWidthsW
#undef GetCharWidthFloatW
#undef GetCharWidthW
#undef GetClassInfoW
#undef GetClassInfoExW
#undef GetClassLongW
#undef GetClassNameW
#undef GetCurrentDirectoryW
#undef GetDlgItemTextW
#undef GetFileAttributesW
#undef GetFullPathNameW
#undef GetGlyphOutlineW
#undef GetKerningPairsW
#undef GetLocaleInfoW
#undef GetMessageW
#undef GetModuleFileNameW
#undef GetModuleHandleW
#undef GetObjectW
#undef GetOpenFileNameW
#undef GetOutlineTextMetricsW
#undef GetPrivateProfileIntW
#undef GetPrivateProfileStringW
#undef GetProfileIntW
#undef GetPropW
#undef GetSaveFileNameW
#undef GetTabbedTextExtentW
#undef GetTempFileNameW
#undef GetTempPathW
#undef GetTextExtentPoint32W
#undef GetTextExtentPointW
#undef GetTextExtentExPointW
#undef GetTextMetricsW
#undef GetUserNameW
#undef GetVersionExW
#undef GetVolumeInformationW
#undef GetWindowLongW
#undef GetWindowTextLengthW
#undef GetWindowTextW
#undef GlobalAddAtomW
#undef GlobalGetAtomNameW
#undef GrayStringW
#undef InsertMenuW
#undef IsBadStringPtrW
#undef IsCharAlphaNumericW
#undef IsCharAlphaW
#undef IsDialogMessageW
#undef LCMapStringW
#undef LoadAcceleratorsW
#undef LoadBitmapW
#undef LoadCursorW
#undef LoadIconW
#undef LoadImageW
#undef LoadLibraryExW
#undef LoadLibraryW
#undef LoadMenuIndirectW
#undef LoadMenuW
#undef LoadStringW
#undef lstrcatW
#undef lstrcmpiW
#undef lstrcmpW
#undef lstrcpynW
#undef lstrcpyW
#undef lstrlenW
#undef MapVirtualKeyW
#undef MessageBoxW
#undef MessageBoxIndirectW
#undef ModifyMenuW
#undef MoveFileExW
#undef MoveFileW
#undef OemToCharW
#undef OutputDebugStringW
#undef PeekMessageW
#undef PostMessageW
#undef PostThreadMessageW
#undef RegCreateKeyExW
#undef RegCreateKeyW
#undef RegDeleteKeyW
#undef RegDeleteValueW
#undef RegEnumKeyW
#undef RegEnumValueW
#undef RegEnumKeyExW
#undef RegisterClassW
#undef RegisterClassExW
#undef RegisterClipboardFormatW
#undef RegisterWindowMessageW
#undef RegOpenKeyExW
#undef RegOpenKeyW
#undef RegQueryInfoKeyW
#undef RegQueryValueExW
#undef RegQueryValueW
#undef RegSetValueExW
#undef RegSetValueW
#undef RemovePropW
#undef SendDlgItemMessageW
#undef SendMessageW
#undef SendNotifyMessageW
#undef SetDlgItemTextW
#undef SetFileAttributesW
#undef SetLocaleInfoW
#undef SetMenuItemInfoW
#undef SetPropW
#undef SetWindowLongW
#undef SetWindowsHookExW
#undef SetWindowTextW
#undef StartDocW
#undef StgCreateDocfileW
#undef SystemParametersInfoW
#undef TabbedTextOutW
//#undef TranslateAcceleratorW	FOR OLE CTL: THIS MAGLES INTERFACE MEMBERS BY SAME NAME
#undef UnregisterClassW
#undef VkKeyScanW
#undef WinHelpW
#undef WritePrivateProfileStringW
#undef wsprintfW

// Added by VK -- 12/1/98
#undef URLOpenBlockingStreamW
#undef InternetCreateUrlW
#undef InternetCrackUrlW
#undef DeleteUrlCacheEntryW

// Added by VK -- 8/10/99
#undef InternetOpenW
#undef InternetOpenUrlW


// These are the currently unsupported APIs
// These will assert in the debug version and map directly 
// to Windows in the retail version
#ifdef DEBUG
#undef AbortSystemShutdownW
#undef AccessCheckAndAuditAlarmW
#undef AddFontResourceW
#undef AddFormW
#undef AddJobW
#undef AddMonitorW
#undef AddPortW
#undef AddPrinterConnectionW
#undef AddPrinterDriverW
#undef AddPrinterW
#undef AddPrintProcessorW
#undef AddPrintProvidorW
#undef AdvancedDocumentPropertiesW
#undef auxGetDevCapsW
#undef BackupEventLogW
#undef BeginUpdateResourceW
#undef BuildCommDCBAndTimeoutsW
#undef BuildCommDCBW
#undef CallMsgFilterW
#undef CallNamedPipeW
#undef ChangeDisplaySettingsW
#undef ChangeMenuW
#undef CharToOemBuffW
#undef CharUpperBuffW
#undef ChooseColorW
#undef ChooseFontW
#undef ClearEventLogW
#undef CommConfigDialogW
#undef CompareStringW
#undef ConfigurePortW
#undef CopyAcceleratorTableW
#undef CopyEnhMetaFileW
#undef CopyMetaFileW
#undef CreateAcceleratorTableW
#undef CreateColorSpaceW
#undef CreateDesktopW
#undef CreateDialogIndirectParamW
#undef CreateDialogParamW
#undef CreateICW
#undef CreateMailslotW
#undef CreateMutexW
#undef CreateNamedPipeW
#undef CreateProcessW
#undef CreateProcessAsUserW
#undef CreatePropertySheetPageW
#undef CreateScalableFontResourceW
#undef CreateStatusWindowW
#undef CreateWindowStationW
#undef DceErrorInqTextW
#undef DdeQueryStringW
#undef DefDlgProcW
#undef undefDosDeviceW
#undef DeleteFormW
#undef DeleteMonitorW
#undef DeletePortW
#undef DeletePrinterConnectionW
#undef DeletePrinterDriverW
#undef DeletePrintProcessorW
#undef DeletePrintProvidorW
#undef DeviceCapabilitiesW
#undef DlgDirListComboBoxW
#undef DlgDirListW
#undef DlgDirSelectComboBoxExW
#undef DlgDirSelectExW
#undef DocumentPropertiesW
#undef DoEnvironmentSubstW
#undef DragQueryFileW
#undef DrawStateW
#undef EndUpdateResourceW
#undef EnumCalendarInfoW
#undef EnumDateFormatsW
#undef EnumDesktopsW
#undef EnumDisplaySettingsW
#undef EnumFontFamiliesExW
#undef EnumFontFamiliesW
#undef EnumFontsW
#undef EnumFormsW
#undef EnumICMProfilesW
#undef EnumJobsW
#undef EnumMonitorsW
#undef EnumPortsW
#undef EnumPrinterDriversW
#undef EnumPrintersW
#undef EnumPrintProcessorDatatypesW
#undef EnumPrintProcessorsW
#undef EnumPropsExW
#undef EnumPropsW
#undef EnumProtocolsW
#undef EnumResourceLanguagesW
#undef EnumResourceNamesW
#undef EnumResourceTypesW
#undef EnumSystemCodePagesW
#undef EnumSystemLocalesW
#undef EnumTimeFormatsW
#undef EnumWindowStationsW
#undef ExtractAssociatedIconW
#undef ExtractIconW
#undef ExtractIconExW
#undef FillConsoleOutputCharacterW
#undef FindEnvironmentStringW
#undef FindExecutableW
#undef FindResourceExW
#undef FindTextW
#undef FindWindowExW
#undef FoldStringW
#undef GetBinaryTypeW
#undef GetCharacterPlacementW
#undef GetCharWidth32W
#undef GetCommandLineW
#undef GetClipboardFormatNameW
#undef GetCompressedFileSizeW
#undef GetComputerNameW
#undef GetConsoleTitleW
#undef GetCurrencyFormatW
#undef GetDateFormatW
#undef GetDefaultCommConfigW
#undef GetDiskFreeSpaceW
#undef GetDriveTypeW
#undef GetEnhMetaFileDescriptionW
#undef GetEnhMetaFileW
#undef GetEnvironmentVariableW
#undef GetExpandedNameW
#undef GetFileSecurityW
#undef GetFileTitleW
#undef GetFileVersionInfoW
#undef GetFileVersionInfoSizeW
#undef GetFormW
#undef GetICMProfileW
#undef GetJobW
#undef GetKeyboardLayoutNameW
#undef GetKeyNameTextW
#undef GetLogColorSpaceW
#undef GetLogicalDriveStringsW
#undef GetMenuItemInfoW
#undef GetMenuStringW
#undef GetMetaFileW
#undef GetNameByTypeW
#undef GetNamedPipeHandleStateW
#undef GetNumberFormatW
#undef GetPrinterW
#undef GetPrinterDataW
#undef GetPrinterDriverDirectoryW
#undef GetPrinterDriverW
#undef GetPrintProcessorDirectoryW
#undef GetPrivateProfileSectionNamesW
#undef GetPrivateProfileSectionW
#undef GetPrivateProfileStructW
#undef GetProfileSectionW
#undef GetProfileStringW
#undef GetShortPathNameW
#undef GetStartupInfoW
#undef GetStringTypeExW
#undef GetSystemDirectoryW
#undef GetTextFaceW
#undef GetTimeFormatW
#undef GetTypeByNameW
#undef GetUserObjectInformationW
#undef GetWindowsDirectoryW
#undef GlobalFindAtomW
#undef ImageList_LoadImageW
#undef ImmConfigureIMEW
#undef ImmEnumRegisterWordW
#undef ImmEscapeW
#undef ImmGetCandidateListCountW
#undef ImmGetCandidateListW
#undef ImmGetCompositionFontW
#undef ImmGetCompositionStringW
#undef ImmGetConversionListW
#undef ImmGetDescriptionW
#undef ImmGetGuideLineW
#undef ImmGetIMEFileNameW
#undef ImmGetRegisterWordStyleW
#undef ImmInstallIMEW
#undef ImmIsUIMessageW
#undef ImmRegisterWordW
#undef ImmSetCompositionFontW
#undef ImmSetCompositionStringW
#undef ImmUnregisterWordW
#undef InitiateSystemShutdownW
#undef InsertMenuItemW
#undef IsCharLowerW
#undef IsCharUpperW
#undef I_RpcServerUnregisterEndpointW
#undef joyGetDevCapsW
#undef LoadCursorFromFileW
#undef LoadKeyboardLayoutW
#undef LogonUserW
#undef LZOpenFileW
#undef MapVirtualKeyExW
#undef MIMEAssociationDialogW
#undef MultinetGetConnectionPerformanceW
#undef ObjectCloseAuditAlarmW
#undef ObjectOpenAuditAlarmW
#undef ObjectPrivilegeAuditAlarmW
#undef OemToCharBuffW
#undef OpenBackupEventLogW
#undef OpenDesktopW
#undef OpenEventLogW
#undef OpenEventW
#undef OpenFileMappingW
#undef OpenMutexW
#undef OpenPrinterW
#undef OpenSemaphoreW
#undef OpenWindowStationW
#undef PageSetupDlgW
#undef PeekConsoleInputW
#undef PolyTextOutW
#undef PrintDlgW
#undef PrinterMessageBoxW
#undef PrivilegedServiceAuditAlarmW
#undef PropertySheetW
#undef QueryDosDeviceW
#undef ReadConsoleInputW
#undef ReadConsoleOutputCharacterW
#undef ReadConsoleOutputW
#undef ReadConsoleW
#undef ReadEventLogW
#undef RegConnectRegistryW
#undef RegisterEventSourceW
#undef RegLoadKeyW
#undef RegQueryMultipleValuesW
#undef RegReplaceKeyW
#undef RegRestoreKeyW
#undef RegSaveKeyW
#undef RegUnLoadKeyW
#undef RemoveDirectoryW
#undef RemoveFontResourceW
#undef ReplaceTextW
#undef ReportEventW
#undef ResetDCW
#undef ResetPrinterW
#undef RpcBindingFromStringBindingW
#undef RpcBindingInqAuthClientW
#undef RpcBindingToStringBindingW
#undef RpcEpRegisterNoReplaceW
#undef RpcMgmtEpEltInqNextW
#undef RpcMgmtInqServerPrincNameW
#undef RpcNetworkInqProtseqsW
#undef RpcNetworkIsProtseqValidW
#undef RpcNsBindingInqEntryNameW
#undef RpcProtseqVectorFreeW
#undef RpcServerInqDefaultPrincNameW
#undef RpcServerUseProtseqEpW
#undef RpcServerUseProtseqIfW
#undef RpcServerUseProtseqW
#undef RpcStringBindingComposeW
#undef RpcStringBindingParseW
#undef RpcStringFreeW
#undef ScrollConsoleScreenBufferW
#undef SearchPathW
#undef SendMessageCallbackW
#undef SendMessageTimeoutW
#undef SetClassLongW
#undef SetComputerNameW
#undef SetConsoleTitleW
#undef SetCurrentDirectoryW
#undef SetDefaultCommConfigW
#undef SetEnvironmentVariableW
#undef SetFileSecurityW
#undef SetFormW
#undef SetICMProfileW
#undef SetJobW
#undef SetPrinterDataW
#undef SetPrinterW
#undef SetUserObjectInformationW
#undef SetVolumeLabelW
#undef SetWindowsHookW
#undef SHBrowseForFolderW
#undef Shell_NotifyIconW
#undef ShellAboutW
#undef ShellExecuteW
#undef ShellExecuteExW
#undef SHFileOperationW
#undef SHGetFileInfoW
#undef SHGetNewLinkInfoW
#undef SHGetPathFromIDListW
#undef sndPlaySoundW
#undef StartDocPrinterW
#undef StgCreateDocfileW
#undef TranslateURLW
#undef UpdateICMRegKeyW
#undef URLAssociationDialogW
#undef UuidFromStringW
#undef VerFindFileW
#undef VerInstallFileW
#undef VerLanguageNameW
#undef VerQueryValueW
#undef VkKeyScanExW
#undef WaitNamedPipeW
#undef waveInGetDevCapsW
#undef waveInGetErrorTextW
#undef waveOutGetDevCapsW
#undef waveOutGetErrorTextW
#undef wglUseFontBitmapsW
#undef wglUseFontOutlinesW
#undef WinExecErrorW
#undef WNetAddConnection2W
#undef WNetAddConnection3W
#undef WNetAddConnectionW
#undef WNetCancelConnection2W
#undef WNetCancelConnectionW
#undef WNetConnectionDialog1W
#undef WNetDisconnectDialog1W
#undef WNetEnumResourceW
#undef WNetGetConnectionW
#undef WNetGetLastErrorW
#undef WNetGetNetworkInformationW
#undef WNetGetProviderNameW
#undef WNetGetUniversalNameW
#undef WNetGetUserW
#undef WNetOpenEnumW
#undef WNetSetConnectionW
#undef WNetUseConnectionW
#undef WriteConsoleInputW
#undef WriteConsoleOutputCharacterW
#undef WriteConsoleOutputW
#undef WriteConsoleW
#undef WritePrivateProfileSectionW
#undef WritePrivateProfileStructW
#undef WriteProfileSectionW
#undef WriteProfileStringW
#undef wvsprintfW
#endif // DEBUG

#endif // __WIN95WRP_CPP__

