#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchCatNExA _StringCchCatNExA
#include <strsafe.h>

#undef StringCchCatNExA
HRESULT __stdcall
StringCchCatNExA(
    STRSAFE_LPSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCSTR pszSrc,
    size_t cbMaxAppend,
    STRSAFE_LPSTR *ppszDestEnd,
    size_t *pcbRemaining,
    STRSAFE_DWORD dwFlags)

{
    /* Use the inlined version */
    return _StringCchCatNExA(pszDest, cbDest, pszSrc, cbMaxAppend, ppszDestEnd, pcbRemaining, dwFlags);
}
