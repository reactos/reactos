#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchGetsExW _StringCchGetsExW
#include <strsafe.h>

#undef StringCchGetsExW
HRESULT __stdcall
StringCchGetsExW(
    STRSAFE_LPWSTR pszDest,
    size_t cbDest,
    STRSAFE_LPWSTR *ppszDestEnd,
    size_t *pcbRemaining,
    STRSAFE_DWORD dwFlags)
{
    /* Use the inlined version */
    return _StringCchGetsExW(pszDest, cbDest, ppszDestEnd, pcbRemaining, dwFlags);
}
