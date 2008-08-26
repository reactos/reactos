#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbGetsExA _StringCbGetsExA
#include <strsafe.h>

#undef StringCbGetsExA
HRESULT __stdcall
StringCbGetsExA(
    STRSAFE_LPSTR pszDest,
    size_t cbDest,
    STRSAFE_LPSTR *ppszDestEnd,
    size_t *pcbRemaining,
    STRSAFE_DWORD dwFlags)
{
    /* Use the inlined version */
    return _StringCbGetsExA(pszDest, cbDest, ppszDestEnd, pcbRemaining, dwFlags);
}
