#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchCatExA _StringCchCatExA
#include <strsafe.h>

#undef StringCchCatExA
HRESULT __stdcall
StringCchCatExA(
    STRSAFE_LPSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCSTR pszSrc,
    STRSAFE_LPSTR *ppszDestEnd,
    size_t *pcbRemaining,
    STRSAFE_DWORD dwFlags)

{
    /* Use the inlined version */
    return _StringCchCatExA(pszDest, cbDest, pszSrc, ppszDestEnd, pcbRemaining, dwFlags);
}
