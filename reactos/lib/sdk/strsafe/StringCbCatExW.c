#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbCatExW _StringCbCatExW
#include <strsafe.h>

#undef StringCbCatExW
HRESULT __stdcall
StringCbCatExW(
    STRSAFE_LPWSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCWSTR pszSrc,
    STRSAFE_LPWSTR *ppszDestEnd,
    size_t *pcbRemaining,
    STRSAFE_DWORD dwFlags)

{
    /* Use the inlined version */
    return _StringCbCatExW(pszDest, cbDest, pszSrc, ppszDestEnd, pcbRemaining, dwFlags);
}
