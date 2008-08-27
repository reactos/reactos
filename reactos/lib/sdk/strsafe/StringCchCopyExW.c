#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchCopyExW _StringCchCopyExW
#include <strsafe.h>

#undef StringCchCopyExW
HRESULT __stdcall
StringCchCopyExW(
    STRSAFE_LPWSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCWSTR pszSrc,
    STRSAFE_LPWSTR *ppszDestEnd,
    size_t *pcbRemaining,
    STRSAFE_DWORD dwFlags)

{
    /* Use the inlined version */
    return _StringCchCopyExW(pszDest, cbDest, pszSrc, ppszDestEnd, pcbRemaining, dwFlags);
}
