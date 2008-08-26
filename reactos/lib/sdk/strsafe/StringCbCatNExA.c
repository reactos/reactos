#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbCatNExA _StringCbCatNExA
#include <strsafe.h>

#undef StringCbCatNExA
HRESULT __stdcall
StringCbCatNExA(
    STRSAFE_LPSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCSTR pszSrc,
    size_t cbMaxAppend,
    STRSAFE_LPSTR *ppszDestEnd,
    size_t *pcbRemaining,
    STRSAFE_DWORD dwFlags)

{
    /* Use the inlined version */
    return _StringCbCatNExA(pszDest, cbDest, pszSrc, cbMaxAppend, ppszDestEnd, pcbRemaining, dwFlags);
}
