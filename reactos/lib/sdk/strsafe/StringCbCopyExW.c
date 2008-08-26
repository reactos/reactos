#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbCopyExW _StringCbCopyExW
#include <strsafe.h>

#undef StringCbCopyExW
HRESULT __stdcall
StringCbCopyExW(
    STRSAFE_LPWSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCWSTR pszSrc,
    STRSAFE_LPWSTR *ppszDestEnd,
    size_t *pcbRemaining,
    STRSAFE_DWORD dwFlags)

{
    /* Use the inlined version */
    return _StringCbCopyExW(pszDest, cbDest, pszSrc, ppszDestEnd, pcbRemaining, dwFlags);
}
