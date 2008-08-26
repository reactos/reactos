#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbCopyNA _StringCbCopyNA
#include <strsafe.h>

#undef StringCbCopyNA
HRESULT __stdcall
StringCbCopyNA(
    STRSAFE_LPSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCSTR pszSrc,
    size_t cbSrc)
{
    /* Use the inlined version */
    return _StringCbCopyNA(pszDest, cbDest, pszSrc, cbSrc);
}
