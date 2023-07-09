#ifndef __HELPERS_HPP__
#define __HELPERS_HPP__
#include <delaydll.h>

#ifdef __cplusplus
extern "C" {
#endif

// String type conversion routines
HRESULT Ansi2Unicode(const char *, wchar_t **);
HRESULT Unicode2Ansi(const wchar_t *src, char ** dest);

// String->CLSID conversion routines
HRESULT ConvertANSItoCLSID(const char *pszCLSID, CLSID * clsid);
HRESULT ConvertANSIProgIDtoCLSID(const char *progid, CLSID *pCLSID);
HRESULT ConvertFriendlyANSItoCLSID(char *pszCLSID, CLSID * clsid);

HRESULT RemoveDirectoryAndChildren(LPCSTR szDir);

HRESULT SetSize(SIZE *, long cx, long cy);

HRESULT CDLDupWStr( LPWSTR *pszwstrDst, LPCWSTR szwSrc );

#undef SAFERELEASE
#define SAFERELEASE(p) if ((p) != NULL) { (p)->Release(); (p) = NULL; };

#undef SAFEDELETE
#define SAFEDELETE(p) if ((p) != NULL) { delete (p); (p) = NULL; };

#undef SAFEREGCLOSEKEY
#define SAFEREGCLOSEKEY(p) if ((p) != NULL) { ::RegCloseKey(p); (p) = NULL; };

#undef SAFESYSFREESTRING
#define SAFESYSFREESTRING(p) if ((p) != NULL) { SysFreeString(p); (p) = NULL; };

#ifdef __cplusplus
}
#endif

#endif
