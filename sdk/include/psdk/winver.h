#ifndef VER_H
#define VER_H

#include <specstrings.h>
#include <verrsrc.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RC_INVOKED

DWORD WINAPI VerFindFileA(DWORD,LPCSTR,LPCSTR,LPCSTR,LPSTR,PUINT,LPSTR,PUINT);
DWORD WINAPI VerFindFileW(DWORD,LPCWSTR,LPCWSTR,LPCWSTR,LPWSTR,PUINT,LPWSTR,PUINT);
DWORD WINAPI VerInstallFileA(DWORD,LPCSTR,LPCSTR,LPCSTR,LPCSTR,LPCSTR,LPSTR,PUINT);
DWORD WINAPI VerInstallFileW(DWORD,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR,LPWSTR,PUINT);
DWORD WINAPI GetFileVersionInfoSizeA(LPCSTR,PDWORD);
DWORD WINAPI GetFileVersionInfoSizeW(LPCWSTR,PDWORD);
BOOL WINAPI GetFileVersionInfoA(LPCSTR,DWORD,DWORD,PVOID);
BOOL WINAPI GetFileVersionInfoW(LPCWSTR,DWORD,DWORD,PVOID);
DWORD WINAPI VerLanguageNameA(DWORD,LPSTR,DWORD);
DWORD WINAPI VerLanguageNameW(DWORD,LPWSTR,DWORD);
BOOL WINAPI VerQueryValueA(LPCVOID,LPCSTR,LPVOID*,PUINT);
BOOL WINAPI VerQueryValueW(LPCVOID,LPCWSTR,LPVOID*,PUINT);

DWORD
WINAPI
GetFileVersionInfoSizeExA(
  _In_ DWORD dwFlags,
  _In_ LPCSTR lpwstrFilename,
  _Out_ LPDWORD lpdwHandle);

DWORD
WINAPI
GetFileVersionInfoSizeExW(
  _In_ DWORD dwFlags,
  _In_ LPCWSTR lpwstrFilename,
  _Out_ LPDWORD lpdwHandle);

#ifdef UNICODE
#define VerFindFile VerFindFileW
#define VerQueryValue VerQueryValueW
#define VerInstallFile VerInstallFileW
#define GetFileVersionInfoSize GetFileVersionInfoSizeW
#define GetFileVersionInfo GetFileVersionInfoW
#define VerLanguageName VerLanguageNameW
#define VerQueryValue VerQueryValueW
#define GetFileVersionInfoSizeEx GetFileVersionInfoSizeExW
#else
#define VerQueryValue VerQueryValueA
#define VerFindFile VerFindFileA
#define VerInstallFile VerInstallFileA
#define GetFileVersionInfoSize GetFileVersionInfoSizeA
#define GetFileVersionInfo GetFileVersionInfoA
#define VerLanguageName VerLanguageNameA
#define VerQueryValue VerQueryValueA
#define GetFileVersionInfoSizeEx GetFileVersionInfoSizeExA
#endif /* UNICODE */

#endif /* !RC_INVOKED */

#ifdef __cplusplus
}
#endif

#endif /* VER_H */
