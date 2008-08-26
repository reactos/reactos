#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbCatExA _StringCbCatExA
#include <strsafe.h>

#undef StringCbCatExA
HRESULT __stdcall
StringCbCatExA(
    STRSAFE_LPSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCSTR pszSrc,
    STRSAFE_LPSTR *ppszDestEnd,
    size_t *pcbRemaining,
    STRSAFE_DWORD dwFlags)

{
    /* Use the inlined version */
    return _StringCbCatExA(pszDest, cbDest, pszSrc, ppszDestEnd, pcbRemaining, dwFlags);
}
