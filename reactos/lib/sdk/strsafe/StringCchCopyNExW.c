#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchCopyNExW _StringCchCopyNExW
#include <strsafe.h>

#undef StringCchCopyNExW
HRESULT __stdcall
StringCchCopyNExW(
    STRSAFE_LPWSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCWSTR pszSrc,
    size_t cbMaxAppend,
    STRSAFE_LPWSTR *ppszDestEnd,
    size_t *pcbRemaining,
    STRSAFE_DWORD dwFlags)

{
    /* Use the inlined version */
    return _StringCchCopyNExW(pszDest, cbDest, pszSrc, cbMaxAppend, ppszDestEnd, pcbRemaining, dwFlags);
}
