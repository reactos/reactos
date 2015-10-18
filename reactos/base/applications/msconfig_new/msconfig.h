#pragma once

#ifdef __cplusplus
extern "C" {
#endif

extern BOOL bIsWindows;
extern BOOL bIsOSVersionLessThanVista;

extern HINSTANCE hInst;
extern LPWSTR szAppName;
extern HWND hMainWnd;

#ifdef __cplusplus
} // extern "C"
#endif
