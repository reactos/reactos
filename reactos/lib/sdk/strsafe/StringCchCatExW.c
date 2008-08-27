#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchCatExW _StringCchCatExW
#include <strsafe.h>

#undef StringCchCatExW
HRESULT __stdcall
StringCchCatExW(
    STRSAFE_LPWSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCWSTR pszSrc,
    STRSAFE_LPWSTR *ppszDestEnd,
    size_t *pcbRemaining,
    STRSAFE_DWORD dwFlags)

{
    /* Use the inlined version */
    return _StringCchCatExW(pszDest, cbDest, pszSrc, ppszDestEnd, pcbRemaining, dwFlags);
}
