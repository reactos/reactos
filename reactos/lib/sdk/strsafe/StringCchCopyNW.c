#define STRSAFE_NO_CB_FUNCTIONS
#define StringCchCopyNW _StringCchCopyNW
#include <strsafe.h>

#undef StringCchCopyNW
HRESULT __stdcall
StringCchCopyNW(
    STRSAFE_LPWSTR pszDest,
    size_t cbDest,
    STRSAFE_LPCWSTR pszSrc,
    size_t cbSrc)
{
    /* Use the inlined version */
    return _StringCchCopyNW(pszDest, cbDest, pszSrc, cbSrc);
}
