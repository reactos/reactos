#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchCopyExA _StringCchCopyExA
#include <strsafe.h>

#undef StringCchCopyExA
HRESULT __stdcall
StringCchCopyExA(
    STRSAFE_LPSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCSTR pszSrc,
    STRSAFE_LPSTR *ppszDestEnd,
    size_t *pcbRemaining,
    STRSAFE_DWORD dwFlags)
{
    /* Use the inlined version */
    return _StringCchCopyExA(pszDest, cbDest, pszSrc, ppszDestEnd, pcbRemaining, dwFlags);
}
