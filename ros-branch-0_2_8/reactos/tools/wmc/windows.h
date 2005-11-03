#include <wchar.h>
typedef wchar_t WCHAR;
typedef unsigned short int WORD;
typedef unsigned int UINT;
typedef unsigned int DWORD;
typedef char CHAR;
typedef CHAR *LPSTR;
typedef WCHAR *LPWSTR;
typedef const CHAR *LPCSTR;
typedef const WCHAR *LPCWSTR;
typedef unsigned int *LPBOOL;

#ifndef __unix__
#define STDCALL __attribute__((stdcall))
#else
#define STDCALL
#endif

int STDCALL MultiByteToWideChar(
  UINT CodePage,
  DWORD dwFlags,
  LPCSTR lpMultiByteStr,
  int cbMultiByte,
  LPWSTR lpWideCharStr,
  int cchWideChar);

int STDCALL WideCharToMultiByte(
  UINT CodePage,
  DWORD dwFlags,
  LPCWSTR lpWideCharStr,
  int cchWideChar,
  LPSTR lpMultiByteStr,
  int cbMultiByte,
  LPCSTR lpDefaultChar,
  LPBOOL lpUsedDefaultChar);
