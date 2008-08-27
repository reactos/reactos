#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchCatNExW _StringCchCatNExW
#include <strsafe.h>

#undef StringCchCatNExW
HRESULT __stdcall
StringCchCatNExW(
    STRSAFE_LPWSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCWSTR pszSrc,
    size_t cbMaxAppend,
    STRSAFE_LPWSTR *ppszDestEnd,
    size_t *pcbRemaining,
    STRSAFE_DWORD dwFlags)

{
    /* Use the inlined version */
    return _StringCchCatNExW(pszDest, cbDest, pszSrc, cbMaxAppend, ppszDestEnd, pcbRemaining, dwFlags);
}
