#pragma once

#ifdef __cplusplus
extern "C" {
#endif

extern BOOL bIsWindows;
extern BOOL bIsOSVersionLessThanVista;

extern const LPCWSTR IDS_REACTOS;
extern const LPCWSTR IDS_MICROSOFT;
extern const LPCWSTR IDS_WINDOWS;

extern HINSTANCE hInst;
extern LPWSTR szAppName;
extern HWND hMainWnd;

#ifdef __cplusplus
} // extern "C"
#endif
