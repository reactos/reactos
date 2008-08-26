#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbCopyNExW _StringCbCopyNExW
#include <strsafe.h>

#undef StringCbCopyNExW
HRESULT __stdcall
StringCbCopyNExW(
    STRSAFE_LPWSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCWSTR pszSrc,
    size_t cbMaxAppend,
    STRSAFE_LPWSTR *ppszDestEnd,
    size_t *pcbRemaining,
    STRSAFE_DWORD dwFlags)

{
    /* Use the inlined version */
    return _StringCbCopyNExW(pszDest, cbDest, pszSrc, cbMaxAppend, ppszDestEnd, pcbRemaining, dwFlags);
}
