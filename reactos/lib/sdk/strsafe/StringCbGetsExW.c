#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbGetsExW _StringCbGetsExW
#include <strsafe.h>

#undef StringCbGetsExW
HRESULT __stdcall
StringCbGetsExW(
    STRSAFE_LPWSTR pszDest,
    size_t cbDest,
    STRSAFE_LPWSTR *ppszDestEnd,
    size_t *pcbRemaining,
    STRSAFE_DWORD dwFlags)
{
    /* Use the inlined version */
    return _StringCbGetsExW(pszDest, cbDest, ppszDestEnd, pcbRemaining, dwFlags);
}
