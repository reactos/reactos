#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbCopyExA _StringCbCopyExA
#include <strsafe.h>

#undef StringCbCopyExA
HRESULT __stdcall
StringCbCopyExA(
    STRSAFE_LPSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCSTR pszSrc,
    STRSAFE_LPSTR *ppszDestEnd,
    size_t *pcbRemaining,
    STRSAFE_DWORD dwFlags)
{
    /* Use the inlined version */
    return _StringCbCopyExA(pszDest, cbDest, pszSrc, ppszDestEnd, pcbRemaining, dwFlags);
}
