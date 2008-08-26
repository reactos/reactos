#define STRSAFE_NO_CCH_FUNCTIONS
#define StringCbCopyNW _StringCbCopyNW
#include <strsafe.h>

#undef StringCbCopyNW
HRESULT __stdcall
StringCbCopyNW(
    STRSAFE_LPWSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCWSTR pszSrc,
    size_t cbSrc)
{
    /* Use the inlined version */
    return _StringCbCopyNW(pszDest, cbDest, pszSrc, cbSrc);
}
