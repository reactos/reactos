#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbCatNExW _StringCbCatNExW
#include <strsafe.h>

#undef StringCbCatNExW
HRESULT __stdcall
StringCbCatNExW(
    STRSAFE_LPWSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCWSTR pszSrc,
    size_t cbMaxAppend,
    STRSAFE_LPWSTR *ppszDestEnd,
    size_t *pcbRemaining,
    STRSAFE_DWORD dwFlags)

{
    /* Use the inlined version */
    return _StringCbCatNExW(pszDest, cbDest, pszSrc, cbMaxAppend, ppszDestEnd, pcbRemaining, dwFlags);
}
