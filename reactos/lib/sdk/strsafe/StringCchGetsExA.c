#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchGetsExA _StringCchGetsExA
#include <strsafe.h>

#undef StringCchGetsExA
HRESULT __stdcall
StringCchGetsExA(
    STRSAFE_LPSTR pszDest,
    size_t cbDest,
    STRSAFE_LPSTR *ppszDestEnd,
    size_t *pcbRemaining,
    STRSAFE_DWORD dwFlags)
{
    /* Use the inlined version */
    return _StringCchGetsExA(pszDest, cbDest, ppszDestEnd, pcbRemaining, dwFlags);
}
