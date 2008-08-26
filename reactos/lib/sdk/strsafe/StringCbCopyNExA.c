#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbCopyNExA _StringCbCopyNExA
#include <strsafe.h>

#undef StringCbCopyNExA
HRESULT __stdcall
StringCbCopyNExA(
    STRSAFE_LPSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCSTR pszSrc,
    size_t cbMaxAppend,
    STRSAFE_LPSTR *ppszDestEnd,
    size_t *pcbRemaining,
    STRSAFE_DWORD dwFlags)

{
    /* Use the inlined version */
    return _StringCbCopyNExA(pszDest, cbDest, pszSrc, cbMaxAppend, ppszDestEnd, pcbRemaining, dwFlags);
}
