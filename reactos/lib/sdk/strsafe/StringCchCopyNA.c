#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchCopyNA _StringCchCopyNA
#include <strsafe.h>

#undef StringCchCopyNA
HRESULT __stdcall
StringCchCopyNA(
    STRSAFE_LPSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCSTR pszSrc,
    size_t cbSrc)
{
    /* Use the inlined version */
    return _StringCchCopyNA(pszDest, cbDest, pszSrc, cbSrc);
}
