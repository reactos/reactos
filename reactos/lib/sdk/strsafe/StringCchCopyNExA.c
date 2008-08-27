#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchCopyNExA _StringCchCopyNExA
#include <strsafe.h>

#undef StringCchCopyNExA
HRESULT __stdcall
StringCchCopyNExA(
    STRSAFE_LPSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCSTR pszSrc,
    size_t cbMaxAppend,
    STRSAFE_LPSTR *ppszDestEnd,
    size_t *pcbRemaining,
    STRSAFE_DWORD dwFlags)

{
    /* Use the inlined version */
    return _StringCchCopyNExA(pszDest, cbDest, pszSrc, cbMaxAppend, ppszDestEnd, pcbRemaining, dwFlags);
}
